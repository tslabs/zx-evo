// (c) 2010 NedoPC
//
// fpga SPI slave device.

`include "tune.v"

// In:
//  SCS_n = 1 - reg number
//  SCS_n = 0 - data in
//
// Out:
//  SCS_n = 1 - status
//  SCS_n = 0 - data out

// Registers:
//
//  0x10 - ZX keyboard data
//  0x11 - ZX keyboard stop bit
//
//  0x20 - ZX mouse X coordinate register
//  0x21 - ZX mouse Y coordinate register
//  0x22 - ZX mouse Y coordinate register
//  0x23 - Kempston joystick register
//
//  0x30 - ZX reset register
//
//  0x40 - ZX all data for wait registers
//  0x41 - ZX Gluk address register
//  0x42 - ZX Kondratiev's rs232 address register
//
//  0x50 - ZX configuration register

module slavespi
(
	// System
    input  wire fclk,
	input  wire rst_n,

	// Slave SPI
    input  wire spics_n, // avr SPI iface
	output wire spidi,   // to AVR
	input  wire spido,   // from AVR
	input  wire spick,

	input  wire [ 7:0] status_in,   // to indicate wait port source

	// ZX Keyboard
    output wire [ 7:0] kbd_out,
	output reg  [ 2:0] kbd_out_sel,
	output wire        kbd_stb,

	// ZX Mouse
    output wire [ 7:0] mus_out,
	output wire        mus_xstb,
	output wire        mus_ystb,
	output wire        mus_btnstb,
	output wire        kj_stb,

    // Glucklock and Comport
	input  wire [ 7:0] gluclock_addr,
	input  wire [ 2:0] comport_addr,

	// Configuration
    output wire [ 7:0] config0, // config bits for overall system
    output wire        cfg_stb,
    output wire        tape_in_bit,

	input  wire [ 7:0] wait_write,
	output wire [ 7:0] wait_read,
	output wire        wait_end,

	output wire       genrst, // positive pulse, causes Z80 reset
	output wire [1:0] rstrom  // number of ROM page to reset to
);

`ifdef SIMULATE
	initial
	begin
		force wait_read = 8'h00;
	end
`endif

	// re-sync SPI

	reg [2:0] spics_n_sync;
	reg [1:0] spido_sync;
	reg [2:0] spick_sync;

	always @(posedge fclk)
	begin
		spics_n_sync[2:0] <= { spics_n_sync[1:0], spics_n };
		spido_sync  [1:0] <= { spido_sync    [0], spido   };
		spick_sync  [2:0] <= { spick_sync  [1:0], spick   };
	end

	wire scs_n = spics_n_sync[1]; // scs_n - synchronized CS_N
	wire sdo   = spido_sync[1];
//	wire sck   = spick_sync[1];

	wire scs_n_01 = (~spics_n_sync[2]) &   spics_n_sync[1] ;
	wire scs_n_10 =   spics_n_sync[2]  & (~spics_n_sync[1]);

	wire sck_01 = (~spick_sync[2]) &   spick_sync[1] ;
//	wire sck_10 =   spick_sync[2]  & (~spick_sync[1]);

	reg [7:0] regnum;
	reg [7:0] shift_out;
	reg [7:0] data_in;

	// register selectors
	wire sel_kbdreg, sel_kbdstb, sel_musxcr, sel_musycr, sel_musbtn, sel_kj;
	wire sel_rstreg, sel_waitreg, sel_gluadr, sel_comadr, sel_cfg0;

	// keyboard register
	reg [6:0] kbd_reg;

	// mouse register
	reg [7:0] mouse_buf;

	// reset register
	reg [7:0] rst_reg;

	// wait data out register
	reg [7:0] wait_reg;
	reg [7:0] cfg0_reg_in, cfg0_reg_out;    // one for shifting, another for value storing

`ifdef SIMULATE
	initial
	begin
		rst_reg[5:4] = 2'b00;
		cfg0_reg_out[7:0] = 8'd0;
	end
`endif

	// register number
	always @(posedge fclk)
	begin
		if (scs_n_01)
		begin
			regnum <= 8'd0;
		end

		else if (scs_n && sck_01)
		begin
			regnum[7:0] <= { sdo, regnum[7:1] };
		end
	end

	// send data to avr
	always @*
	begin
		if (sel_waitreg)
			data_in = wait_write;
		else if (sel_gluadr)
			data_in = gluclock_addr;
		else if (sel_comadr)
			data_in = comport_addr;
		else data_in = 8'hFF;
	end
	always @(posedge fclk)
	begin
		if (scs_n_01 || scs_n_10)   // both edges
		begin
			shift_out <= scs_n ? status_in : data_in;
		end
		else if (sck_01)
		begin
			shift_out[7:0] <= { 1'b0, shift_out[7:1] };
		end
	end
	assign spidi = shift_out[0];

	// reg number decoding
	assign sel_kbdreg = ((regnum[7:4]==4'h1) && !regnum[0]); // $10
	assign sel_kbdstb = ((regnum[7:4]==4'h1) &&  regnum[0]); // $11
	assign sel_musxcr = ((regnum[7:4]==4'h2) && (regnum[1:0]==2'b00)); // $20
	assign sel_musycr = ((regnum[7:4]==4'h2) && (regnum[1:0]==2'b01)); // $21
	assign sel_musbtn = ((regnum[7:4]==4'h2) && (regnum[1:0]==2'b10)); // $22
	assign sel_kj     = ((regnum[7:4]==4'h2) && (regnum[1:0]==2'b11)); // $23
	assign sel_rstreg = ((regnum[7:4]==4'h3)) ; // $30
	assign sel_waitreg = ((regnum[7:4]==4'h4) && (regnum[1:0]==2'b00)); // $40
	assign sel_gluadr  = ((regnum[7:4]==4'h4) && (regnum[1:0]==2'b01)); // $41
	assign sel_comadr  = ((regnum[7:4]==4'h4) && (regnum[1:0]==2'b10)); // $42
	assign sel_cfg0 = (regnum[7:4]==4'h5); // $50

	assign kbd_stb = kbd_bit_stb && &kbd_out_cnt[2:0];
	assign kbd_out_sel = kbd_out_cnt[5:3];
	wire kbd_bit_stb = !scs_n && sel_kbdreg && sck_01;
	wire kbdstb = sel_kbdstb && scs_n_01;	// this requires to change zx.c for good: kbdstb should be issued BEFORE kbd reg transfer, NOT after

	reg [5:0] kbd_out_cnt;
	always @(posedge fclk)
		if (kbdstb)
			kbd_out_cnt <= 6'b0;
		else if (kbd_bit_stb)
			kbd_out_cnt <= kbd_out_cnt + 6'b1;

	// registers data-in
    assign cfg_stb = scs_n_01 && sel_cfg0;
    assign tape_in_bit = cfg0_reg_in[2];
    	always @(posedge fclk)
	begin
		if (kbd_bit_stb)
			kbd_reg[6:0] <= { sdo, kbd_reg[6:1] };

		if (!scs_n && (sel_musxcr || sel_musycr || sel_musbtn || sel_kj) && sck_01)
			mouse_buf[7:0] <= { sdo, mouse_buf[7:1] };

		if (!scs_n && sel_rstreg && sck_01)
			rst_reg[7:0] <= { sdo, rst_reg[7:1] };

		if (!scs_n && sel_waitreg && sck_01)
			wait_reg[7:0] <= { sdo, wait_reg[7:1] };

		if (!scs_n && sel_cfg0 && sck_01)
			cfg0_reg_in[7:0] <= { sdo, cfg0_reg_in[7:1] };

		if (cfg_stb)
			cfg0_reg_out <= cfg0_reg_in;
	end

	// output data
	assign kbd_out = {sdo, kbd_reg};
	// assign kbd_stb = sel_kbdstb && scs_n_01;

	assign mus_out    = mouse_buf;
	assign mus_xstb   = sel_musxcr && scs_n_01;
	assign mus_ystb   = sel_musycr && scs_n_01;
	assign mus_btnstb = sel_musbtn && scs_n_01;
	assign kj_stb     = sel_kj     && scs_n_01;

	assign genrst = sel_rstreg && scs_n_01;
	assign rstrom = rst_reg[5:4];

	assign wait_read = wait_reg;
	assign wait_end = sel_waitreg && scs_n_01;

	assign config0 = cfg0_reg_out;

endmodule
