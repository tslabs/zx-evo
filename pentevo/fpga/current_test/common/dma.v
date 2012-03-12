// This module serves direct DRAM-to-device data transfer

// to do
// - probably add the extra 8 bit counter for number of bursts

`include "../include/tune.v"


module dma (

// clocks
	input wire clk,
	input wire c2,
	input wire rst_n,

// controls
	input  wire [8:0] dmaport_wr,
	output wire dma_act,

// Z80
	input  wire [7:0] zdata,
	input  wire rfsh_n,

// DRAM interface
	output wire [20:0] dram_addr,
	input  wire [15:0] dram_rddata,
	output wire [15:0] dram_wrdata,
	output wire        dram_req,
	output reg         dma_zwt,
	output wire        dram_rnw,
	input  wire        dram_next,

// SD interface
	input  wire  [7:0] sd_rddata,
	output wire  [7:0] sd_wrdata,
	output wire        sd_req,
	output wire        sd_rnw,
	input  wire        sd_stb,

// IDE interface
	input  wire [15:0] ide_rddata,
	output wire [15:0] ide_wrdata,
	output wire        ide_req,
	output wire        ide_rnw,
	input  wire        ide_stb

);


// target device
//  001 - RAM
//  010 - SD
//  011 - IDE

// mode:
//  0 - device to RAM (read from device)
//  1 - RAM to device (write to device)

	wire [8:0] dma_wr = dmaport_wr & {9{!dma_act}};    // blocking of DMA regs write strobes while DMA active

    wire dma_saddrl = dma_wr[0];
    wire dma_saddrh = dma_wr[1];
    wire dma_saddrx = dma_wr[2];
    wire dma_daddrl = dma_wr[3];
    wire dma_daddrh = dma_wr[4];
    wire dma_daddrx = dma_wr[5];
    wire dma_len    = dma_wr[6];
    wire dma_launch = dma_wr[7];
    wire dma_num    = dma_wr[8];

	wire dv_ram = device == 3'b001;
	wire dv_sd  = device == 3'b010;
	wire dv_ide = device == 3'b011;

    wire bs_sd  = dma_act & dv_sd ;
    wire bs_ide = dma_act & dv_ide;

    wire [0:1] bs_dma = {bs_sd ,
                         bs_ide};


// states logic

// !R/W  phase  RAM  DEV
// device-RAM
//   0     0     0    1
//   0     1     1    0
//   1     0     1    0
//   1     0     0    1
// RAM-RAM
//   x     0     1    0
//   x     1     1    0

	assign dma_act = ~n_ctr[8];

    wire state_rd = ~phase;
    wire state_wr = phase;

    wire state_dev = !dv_ram & (dma_wnr ^ !phase);
    wire state_mem = dv_ram | (dma_wnr ^ phase);

    assign dram_addr = state_rd ? s_addr : d_addr;
    assign dram_wrdata = data;
    assign dram_req = dma_act & state_mem;
    assign dram_rnw = state_rd;

	wire dev_req = dma_act & state_dev;
	wire dev_rnw = state_rd;

    assign sd_wrdata = bsel ? data[15:8] : data[7:0];
    assign sd_req = dev_req & dv_sd;
    assign sd_rnw = dev_rnw;
	wire sd_stb_int = sd_stb;

    assign ide_wrdata = data;
    assign ide_req = dev_req & dv_ide;
    assign ide_rnw = dev_rnw;
	wire ide_stb_int = ide_stb;

    wire dev_stb = (dv_sd & sd_stb_int & bsel) |
                   (dv_ide & ide_stb_int);

    wire phase_end = (state_mem & dram_next) | (state_dev & dev_stb);
    wire cyc_end = phase & phase_end;

    wire byte_switch = (dv_sd & sd_stb_int);


// data aquiring
    reg [15:0] data;

    always @(posedge clk)
    begin
        if (state_rd & dram_next)
        begin
            data <= dram_rddata;
        end

        else if (state_rd & sd_stb_int)
        begin
            if (bsel)
                data[7:0] <= sd_rddata;
            else
                data[15:8] <= sd_rddata;
        end

        else if (state_rd & ide_stb_int)
            data <= ide_rddata;
    end


// states processing
	reg [2:0] device;
    reg dma_wnr;
    reg dma_salgn;
    reg dma_dalgn;
    reg dma_asz;
    reg phase;               // 0 - read / 1 - write
    reg bsel;                // 0 - lsb / 1 - msb

	always @(posedge clk)
	if (dma_launch)			// write to DMACtrl - launch of DMA burst
	begin
		dma_wnr <= zdata[7];
		dma_zwt <= zdata[6];
		dma_salgn <= zdata[5];
		dma_dalgn <= zdata[4];
		dma_asz <= zdata[3];
		device <= zdata[2:0];
		phase <= 1'b0;
		bsel <= 1'b0;
	end

	else
    begin
        if (phase_end)
            phase <= ~phase;
        if (byte_switch)
            bsel <= ~bsel;
    end


// counter processing
	reg [7:0] b_len;        // length of burst
	reg [7:0] b_num;        // number of bursts
	reg [7:0] b_ctr;        // counter for cycles in burst
	reg [8:0] n_ctr;        // counter for bursts

    wire [7:0] b_ctr_next = next_burst ? b_len : b_ctr_dec[7:0];
    wire [8:0] b_ctr_dec = {1'b0, b_ctr[7:0]} - 9'b1;
    wire [8:0] n_ctr_dec = n_ctr - next_burst;
    wire next_burst = b_ctr_dec[8];

	always @(posedge clk)
    if (!rst_n)
		n_ctr[8] <= 1'b1;       // disable DMA on RESET

    else
		if (dma_launch)			// launch of DMA burst
        begin
            b_ctr <= b_len;
            n_ctr <= {1'b0, b_num};
        end

		else if (cyc_end)		// cycle processed
        begin
			b_ctr <= b_ctr_next;
            n_ctr <= n_ctr_dec;
        end


// loading of burst parameters
    always @(posedge clk)
    begin
		if (dma_len)
            b_len <= zdata;

		if (dma_num)
            b_num <= zdata;
    end


// address processing

    // source
    wire [20:0] s_addr_next = {s_addr_next_h[13:1], s_addr_next_m, s_addr_next_l[6:0]};
    wire [13:0] s_addr_next_h = s_addr[20:7] + s_addr_add_h;
    wire [1:0] s_addr_add_h = dma_salgn ? {next_burst & dma_asz, next_burst & !dma_asz} : {s_addr_inc_l[8], 1'b0};
    wire s_addr_next_m = dma_salgn ? (dma_asz ? s_addr_next_l[7] : s_addr_next_h[0]) : s_addr_inc_l[7];
    wire [7:0] s_addr_next_l = (dma_salgn & next_burst) ? s_addr_r : s_addr_inc_l[7:0];
    wire [8:0] s_addr_inc_l = {1'b0, s_addr[7:0]} + 9'b1;

    reg [20:0] s_addr;      // current source address
    reg [7:0] s_addr_r;     // source lower address

	always @(posedge clk)
		if (dram_next & state_rd)			// increment RAM source addr
			s_addr <= s_addr_next;

        else
        begin
            if (dma_saddrl)
            begin
                s_addr[6:0] <= zdata[7:1];
                s_addr_r[6:0] <= zdata[7:1];
            end

            if (dma_saddrh)
            begin
                s_addr[12:7] <= zdata[5:0];
                s_addr_r[7] <= zdata[0];
            end

            if (dma_saddrx)
                s_addr[20:13] <= zdata;
        end

    // destination
    wire [20:0] d_addr_next = {d_addr_next_h[13:1], d_addr_next_m, d_addr_next_l[6:0]};
    wire [13:0] d_addr_next_h = d_addr[20:7] + d_addr_add_h;
    wire [1:0] d_addr_add_h = dma_dalgn ? {next_burst & dma_asz, next_burst & !dma_asz} : {d_addr_inc_l[8], 1'b0};
    wire d_addr_next_m = dma_dalgn ? (dma_asz ? d_addr_next_l[7] : d_addr_next_h[0]) : d_addr_inc_l[7];
    wire [7:0] d_addr_next_l = (dma_dalgn & next_burst) ? d_addr_r : d_addr_inc_l[7:0];
    wire [8:0] d_addr_inc_l = {1'b0, d_addr[7:0]} + 9'b1;

    reg [20:0] d_addr;      // current dest address
    reg [7:0] d_addr_r;     // dest lower address

	always @(posedge clk)
		if (dram_next & state_wr)			// increment RAM dest addr
			d_addr <= d_addr_next;
        else
        begin
            if (dma_daddrl)
            begin
                d_addr[6:0] <= zdata[7:1];
                d_addr_r[6:0] <= zdata[7:1];
            end

            if (dma_daddrh)
            begin
                d_addr[12:7] <= zdata[5:0];
                d_addr_r[7] <= zdata[0];
            end

            if (dma_daddrx)
                d_addr[20:13] <= zdata;
        end


endmodule
