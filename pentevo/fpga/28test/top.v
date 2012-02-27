`include "../include/tune.v"

module top(

	// clocks
	input  wire fclk,
	output wire clkz_out = f1,
	input  wire clkz_in,

	// z80
	input wire iorq_n,
	input wire mreq_n,
	input wire rd_n,
	input wire wr_n,
	input wire m1_n,
	input wire rfsh_n,
	output wire int_n = 1'b1,
	output wire nmi_n = 1'b1,
	output wire wait_n = 1'b1,
	output wire res = ~rst_n,

	inout wire [7:0] d,
	input wire [15:0] a,

	// zxbus and related
	output wire csrom = 0,
	output wire romoe_n = 1,
	output wire romwe_n = 1,

	output wire rompg0_n = 1,
	output wire dos_n = 1, // aka rompg1
	output wire rompg2 = 0,
	output wire rompg3 = 0,
	output wire rompg4 = 0,

	input wire iorqge1,
	input wire iorqge2,
	output wire iorq1_n = 1'b1,
	output wire iorq2_n = 1'b1,

	// DRAM
	inout  wire [15:0] dram_rd = 16'hZZ,
	output wire [9:0] dram_ra = 10'b0,
	output wire rwe_n   = 1'b1,
	output wire rucas_n = 1'b1,
	output wire rlcas_n = 1'b1,
	output wire rras0_n = 1'b1,
	output wire rras1_n = 1'b1,

	// video
	output wire [1:0] vred,
	output wire [1:0] vgrn,
	output wire [1:0] vblu,
	output wire vhsync,
	output wire vvsync,
	output wire vcsync,

	// AY control and audio/tape
	output ay_clk = 0,
	output ay_bdir = 0,
	output ay_bc1 = 0,

	output beep = 0,

	// IDE
	output wire [2:0] ide_a = 0,
	inout  wire [15:0] ide_d = 16'hZZ,
	output wire ide_dir = 1,
	output wire ide_cs0_n = 1,
	output wire ide_cs1_n = 1,
	output wire ide_rs_n = 1,
	output wire ide_rd_n = 1,
	output wire ide_wr_n = 1,
	input  wire ide_rdy,

	// VG93 and diskdrive
	output wire vg_clk = 0,
	output wire vg_cs_n = 1,
	output wire vg_res_n = 0,
	output wire vg_hrdy = 0,
	output wire vg_rclk = 0,
	output wire vg_rawr = 0,
	output wire [1:0] vg_a = 0, // disk drive selection
	output wire vg_wrd = 0,
	output wire vg_side = 0,

	input wire step,
	input wire vg_sl,
	input wire vg_sr,
	input wire vg_tr43,
	input wire rdat_b_n,
	input wire vg_wf_de,
	input wire vg_drq,
	input wire vg_irq,
	input wire vg_wd,

	// serial links (atmega-fpga, sdcard)
	output wire sdcs_n,
	output wire sddo,
	output wire sdclk,
	input  wire sddi,

	input  wire spics_n,
	input  wire spick,
	input  wire spido,
	output wire spidi = 0,
	output wire spiint_n = 1
);


	wire f0, f1, h0, h1, c0, c1, c2, c3;

	clock clock
	(
		.clk(fclk),
		.f0(f0), .f1(f1),
		.h0(h0), .h1(h1),
		.c0(c0), .c1(c1), .c2(c2), .c3(c3)
	);


	wire [7:0] rom_rd_data;
	assign d = memrd ? rom_rd_data : 8'hZZ;
	
	rom rom (
		.a(a),
		.d(rom_rd_data)
	);


	video_top video_top(

		.clk(fclk),
		.f0(f0), .f1(f1),
		.c0(c0), .c1(c1), .c2(c2), .c3(c3),

		.vred(vred),
		.vgrn(vgrn),
		.vblu(vblu),
		.hsync(vhsync),
		.vsync(vvsync),
		.csync(vcsync),

		.vga_on(cfg_vga_on),

        .zborder_wr     (iorq & wr),
        .d              (d)
	);


	// config signals
	wire [7:0] not_used;
	wire cfg_vga_on;

	slavespi slavespi(
		.fclk(fclk),

		.spics_n(spics_n), .spidi(spidi),
		.spido(spido), .spick(spick),
		
		.genrst(genrst),
		.gluclock_addr(0),
		.comport_addr (0),
		.wait_write(0),
		.wait_read(wait_read),
		.wait_rnw(1),
		.wait_end(wait_end),
		.config0( { not_used[7:4], beeper_mux, tape_read, not_used[1], cfg_vga_on} )
	);


	wire rst;
	wire iorq;
	wire iorq_s;
	wire mreq;
	wire m1;
	wire rfsh;
	wire rd;
	wire wr;
	wire rdwr;
	wire iord;
	wire iowr;
	wire iowr_s;
	wire iorw;
	wire memrd;
	wire memwr;
	wire memwr_s;
	wire memrw;

    zsignals zsignals(
                .clk      (fclk),
				.rst_n    (rst_n),
                .iorq_n   (iorq_n),
                .mreq_n   (mreq_n),
                .m1_n     (m1_n),
                .rfsh_n   (rfsh_n),
                .rd_n     (rd_n),
                .wr_n     (wr_n),

                .rst      (rst),
                .iorq     (iorq),
                .iorq_s   (iorq_s),
                .mreq     (mreq),
                .m1       (m1),
                .rfsh     (rfsh),
                .rd       (rd),
                .wr       (wr),
                .rdwr     (rdwr),
                .iord     (iord),
                .iowr     (iowr),
                .iowr_s   (iowr_s),
                .iorw     (iorw),
                .memrd    (memrd),
                .memwr    (memwr),
                .memwr_s  (memwr_s),
                .memrw    (memrw)
                );


	wire genrst;
	wire rst_n; // global reset

	resetter myrst( .clk(fclk),
	                .rst_in_n(~genrst),
	                .rst_out_n(rst_n) );
	defparam myrst.RST_CNT_SIZE = 6;


endmodule

