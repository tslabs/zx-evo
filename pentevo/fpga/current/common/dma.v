// This module serves direct DRAM-to-device data transfer

// to do
// - probably add the extra 8 bit counter for number of bursts


module dma (

// clocks
	input wire clk,
	input wire c2,
	input wire rst_n,

// controls	
	input  wire [8:0] dmaport_wr,
	output wire dma_act,
	output reg  dma_wait,

// Z80
	input  wire [7:0] zdata,
	input  wire rfsh_n,

// DRAM interface
	output wire [20:0] dram_addr,
	input  wire [15:0] dram_rddata,
	output wire [15:0] dram_wrdata,
	output wire        dram_req,
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

	wire dma_wnr_p = zdata[7];
	wire dma_zwait_p = zdata[6];
	wire [2:0] device_p = zdata[2:0];

	wire [8:0] dma_wr = dmaport_wr & {9{!dma_act}};    // blocking of DMA regs write strobes while DMA active
           
    wire dma_saddrl 	= dma_wr[0];
    wire dma_saddrh 	= dma_wr[1];
    wire dma_saddrx 	= dma_wr[2];
    wire dma_daddrl 	= dma_wr[3];
    wire dma_daddrh 	= dma_wr[4];
    wire dma_daddrx 	= dma_wr[5];
    wire dma_len    	= dma_wr[6];
    wire dma_launch_int = dma_wr[7];
    wire dma_num    	= dma_wr[8];

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

	assign dma_act = ~ctr[8];

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
        
        else
        if (state_rd & sd_stb_int)
        begin
            if (bsel)
                data[7:0] <= sd_rddata;
            else
                data[15:8] <= sd_rddata;
        end

        else
        if (state_rd & ide_stb_int)
            data <= ide_rddata;
    end

	
// states processing
	reg [2:0] device;
    reg dma_wnr;
    reg dma_zwait;
    reg phase;               // 0 - read / 1 - write
    reg bsel;                // 0 - lsb / 1 - msb
    
	always @(posedge clk)
	if (dma_launch & c2)			// write to DMACtrl - launch of DMA burst
	begin
		device    <= device_p;
		dma_wnr   <= dma_wnr_p;
		dma_zwait <= dma_zwait_p;
		phase <= 1'b0;
		bsel <= 1'b0;
	end
	
	else
    begin
        if (phase_end)
            phase <= ~phase;
        // else
        if (byte_switch)
            bsel <= ~bsel;
    end
	
    
// counter processing	
	reg [8:0] ctr;
	reg [7:0] b_len;
	
	always @(posedge clk)
    if (!rst_n)
	begin
		ctr[8] <= 1'b1;         // on RESET DMA is OFF
	end
    else
    begin
		if (dma_len)			// setting by write to DMALen
        begin
            b_len <= zdata;
        end
		if (dma_launch & c2)			// launch of DMA burst - write to DMACtrl
        begin
            ctr[8] <= 1'b0;
            ctr[7:0] <= b_len;
        end
		if (cyc_end)			// decrement on successfull cycle processing
			ctr <= ctr - 9'b1;
	end


// address processing
    reg [20:0] s_addr;
    reg [20:0] d_addr;
    
	always @(posedge clk)
	begin
		if (dma_saddrl)						// setting by write to DMASAddrL
			s_addr[6:0] <= zdata[7:1];
		if (dma_saddrh)						// setting by write to DMASAddrH
			s_addr[12:7] <= zdata[5:0];
		if (dma_saddrx)						// setting by write to DMASAddrX
			s_addr[20:13] <= zdata;
            
		if (dma_daddrl)						// setting by write to DMADAddrL
			d_addr[6:0] <= zdata[7:1];
		if (dma_daddrh)						// setting by write to DMADAddrH
			d_addr[12:7] <= zdata[5:0];
		if (dma_daddrx)						// setting by write to DMADAddrX
			d_addr[20:13] <= zdata;
            
		if (dram_next & state_rd)			// increment RAM source addr
			s_addr <= s_addr + 21'b1;
		if (dram_next & state_wr)			// increment RAM dest addr
			d_addr <= d_addr + 21'b1;
	end

    
// Z80 wait
    reg dma_launch_en;
    wire dma_launch = dma_launch_int & dma_launch_en;
    assign dma_wait = dma_act & dma_zwait;
    
    always @(posedge clk)
    begin
        if (dma_act)
            dma_launch_en <= 0;
        if (!rfsh_n)
            dma_launch_en <= 1;
    end
    
    
endmodule
