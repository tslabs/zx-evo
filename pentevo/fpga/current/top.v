`include "../include/tune.v"

// Pentevo project (c) NedoPC 2008-2011
//
// top-level

module top(

	// clocks
	input fclk,
	output clkz_out,
	input clkz_in,

	// z80
	input iorq_n,
	input mreq_n,
	input rd_n,
	input wr_n,
	input m1_n,
	input rfsh_n,
	output int_n,
	output nmi_n,
	output wait_n,
	output res,

	inout [7:0] d,
	input [15:0] a,

	// zxbus and related
	output csrom,
	output romoe_n,
	output romwe_n,

	output rompg0_n,
	output dos_n, // aka rompg1
	output rompg2,
	output rompg3,
	output rompg4,

	input iorqge1,
	input iorqge2,
	output iorq1_n,
	output iorq2_n,

	// DRAM
	inout [15:0] dram_rd,
	output [9:0] dram_ra,
	output rwe_n,
	output rucas_n,
	output rlcas_n,
	output rras0_n,
	output rras1_n,

	// video
	output [1:0] vred,
	output [1:0] vgrn,
	output [1:0] vblu,

	output vhsync,
	output vvsync,
	output vcsync,

	// AY control and audio/tape
	output ay_clk,
	output ay_bdir,
	output ay_bc1,

	output beep,

	// IDE
	output [2:0] ide_a,
	inout [15:0] ide_d,
	output ide_dir,         // rnw
	output ide_cs0_n,
	output ide_cs1_n,
	output ide_rd_n,
	output ide_wr_n,
	output ide_rs_n,
	input ide_rdy,

	// VG93 and diskdrive
	output vg_clk,

	output vg_cs_n,
	output vg_res_n,

	output vg_hrdy,
	output vg_rclk,
	output vg_rawr,
	output [1:0] vg_a, // disk drive selection
	output vg_wrd,
	output vg_side,

	input step,
	input vg_sl,
	input vg_sr,
	input vg_tr43,
	input rdat_b_n,
	input vg_wf_de,
	input vg_drq,
	input vg_irq,
	input vg_wd,

	// serial links (atmega-fpga, sdcard)
	output sdcs_n,
	output sddo,
	output sdclk,
	input sddi,

	input spics_n,
	input spick,
	input spido,
	output spidi,
	output spiint_n
);

	wire f0, f1, h0, h1, c0, c1, c2, c3;
	wire [1:0] ay_mod;

	clock clock
	(
		.clk(fclk),
		.f0(f0), .f1(f1),
		.h0(h0), .h1(h1),
		.c0(c0), .c1(c1), .c2(c2), .c3(c3),
		.ay_clk(ay_clk),
		// .ay_mod(sysconf[4:3])
		.ay_mod(2'b00)
	);


	wire dos;
	wire vdos;


	wire zpos,zneg;

	wire rst_n; // global reset

	wire [4:0] rompg = page[4:0];

	wire [7:0] zports_dout;
	wire zports_dataout;
	wire porthit;


	// wire [39:0] kbd_data;
	wire [7:0] kbd_data;
	wire [2:0] kbd_data_sel;
	wire [7:0] mus_data;
	wire kbd_stb,mus_xstb,mus_ystb,mus_btnstb,kj_stb;

	wire [ 4:0] kbd_port_data;
	wire [ 4:0] kj_port_data;
	wire [ 7:0] mus_port_data;

	wire [7:0] wait_read,wait_write;
	wire wait_rnw;
	wire wait_start_gluclock;
	wire wait_start_comport;
	wire wait_end;
	wire [7:0] gluclock_addr;
	wire [2:0] comport_addr;
	wire [6:0] waits;

	// config signals
	wire [7:0] not_used;
	wire cfg_vga_on;
	wire [1:0] set_nmi;

	// nmi signals
	wire gen_nmi;
	wire clr_nmi;
	wire in_nmi;

	wire tape_in;

	wire [7:0] zmem_dout;
	wire zmem_dataout;

	wire [7:0] received;
	wire [7:0] tobesent;

	wire intrq,drq;
	wire vg_wrFF;

	wire [1:0] rstrom;

	wire zclk = clkz_in;


	// RESETTER
	wire genrst;

	resetter myrst( .clk(fclk),
	                .rst_in_n(~genrst),
	                .rst_out_n(rst_n) );
	defparam myrst.RST_CNT_SIZE = 6;



	assign nmi_n=gen_nmi ? 1'b0 : 1'bZ;

	assign res= ~rst_n;

	wire cpu_req, cpu_rnw, cpu_wrbsel, cpu_strobe;
	wire [20:0] cpu_addr;
	wire [7:0] cpu_wrdata;

	wire go;

	wire sd_start;
	wire [7:0] sd_dataout,sd_datain;


	wire tape_read; // data for tapein

	wire beeper_mux; // what is mixed to FPGA beeper output - beeper (0) or tapeout (1)

	wire beeper_wr, covox_wr;

	wire [1:0] int_turbo;
	wire cpu_next;
	wire cpu_stall;

	wire external_port;


	assign rompg0_n = ~rompg[0];
	assign dos_n    =  rompg[1];
	assign rompg2   =  rompg[2];
	assign rompg3   =  rompg[3];
	assign rompg4   =  rompg[4];

	wire dos_stall;
	wire ide_stall;

	zclock zclock
	(
		.fclk(fclk),
        .zclk(zclk),
        .c0(c0),
        .c2(c2),
        .rst(rst),
        .rfsh_n(rfsh_n),
        .iorq(iorq),
        .zclk_out(clkz_out),
		.zpos(zpos),
        .zneg(zneg),
		.turbo(turbo),
		// .turbo(2'b01),
		// .turbo(2'b00),
		.zclk_stall(cpu_stall || dos_stall || ide_stall),
        .int_turbo(int_turbo),
		.external_port(external_port)
	);


    wire [1:0] turbo =  sysconf[1:0];
	wire [7:0] border;
	wire int_start;

	wire [7:0] dout_ram;
	wire [7:0] dout_ports;
	wire [7:0] im2vect;
	wire ena_ram;
	wire ena_ports;
	wire drive_ff;

	assign d = ena_ram ? dout_ram : (ena_ports ? dout_ports : (intack ? im2vect : (drive_ff ? 8'hFF : 8'bZZZZZZZZ)));
	// assign d = ena_ram ? dout_ram : (ena_ports ? dout_ports : (drive_ff ? 8'hFF : 8'bZZZZZZZZ));
	// assign d = ena_ram ? dout_ram : (ena_ports ? dout_ports : (intack ? im2vect : (drive_ff ? 8'h00 : 8'bZZZZZZZZ)));
	// assign d = ena_ram ? dout_ram : (ena_ports ? dout_ports : 8'bZZZZZZZZ);

	zbus zxbus(
        .iorq(iorq),
        .rd(rd),
        .iorq1_n(iorq1_n),
        .iorq2_n(iorq2_n),
        .iorqge1(iorqge1),
        .iorqge2(iorqge2),
        .porthit(porthit),
        .drive_ff(drive_ff)
        );


	wire rampage_wr;	    // ports #10AF-#13AF
	wire [7:0] memconf;
	wire [7:0] xt_ramp[0:3];
	wire [7:0] page;
	wire vdos_on, vdos_off;
	wire dos_on, dos_off;
    wire romnram;
    wire rw_en;

    pager pager(
        .za         (a),
        .opfetch_s  (opfetch_s),
        .memconf    (memconf),
        .xt_page    (xt_page),
        .dos        (dos),
        .vdos       (vdos),

        .page       (page),
        .romnram    (romnram),
        .rw_en      (rw_en),
        .dos_on     (dos_on),
        .dos_off    (dos_off)

    );



	///////////////////////////
	// DOS signal controller //
	///////////////////////////

	zdos zdos(
	           .clk(fclk),
	           .zclk(zclk),
			   .rst(rst),
               .opfetch(opfetch),

	           .dos_on  (dos_on),
	           .dos_off (dos_off),
			   .vdos_on (vdos_on),
               .vdos_off(vdos_off),

	           .dos(dos),
	           .vdos(vdos),
               .dos_stall(dos_stall)
	         );




	///////////////////////////
	// Z80 memory controller //
	///////////////////////////

    wire m1_on;
    wire m1_off;

	zmem z80mem
	(
		.fclk (fclk),
		.rst(rst),

		.zpos(zpos),
		.zneg(zneg),

		.c0    (c0),
		.c1    (c1),
		.c2    (c2),
		.c3    (c3),

		.za    (a),
		.zd_in (d),
		.zd_out(dout_ram),
		.zd_ena(ena_ram),

		.opfetch(opfetch),
		.iorq(iorq),
		.mreq(mreq),
        .memrd(memrd),
        .memwr(memwr),
		.rd(rd),
		.wr(wr),

		.page   (page),
		.romnram(romnram),

		.rw_en(rw_en),
		.romoe_n(romoe_n),
		.romwe_n(romwe_n),
		.csrom  (csrom),

		.vdos_on   (vdos_on),
		.vdos_off  (vdos_off),
        .dos_on    (dos_on),

		.cpu_req   (cpu_req),
		.cpu_rnw   (cpu_rnw),
		.cpu_wrbsel(cpu_wrbsel),
		.cpu_strobe(cpu_strobe),
		.cpu_addr  (cpu_addr),
		.cpu_wrdata(cpu_wrdata),
		// .cpu_rddata(dram_rddata),    // registered
		.cpu_rddata(dram_rd),        // raw
		.cpu_stall (cpu_stall),
		.cpu_next  (cpu_next),

		.int_turbo(int_turbo)
	);


	wire [20:0] daddr;
	wire dreq;
	wire drnw;
	wire [15:0] dram_rddata;
	wire [15:0] dram_wrdata;
	wire [1:0] dbsel;


	dram dram( .clk(fclk),
	           .rst_n(rst_n),

	           .addr(daddr),
	           .req(dreq),
	           .rnw(drnw),
	           .c0(c0),
	           .c1(c1),
	           .c2(c2),
	           .c3(c3),
	           .rddata(dram_rddata),
	           .wrdata(dram_wrdata),
	           .bsel(dbsel),

	           .ra(dram_ra),
	           .rd(dram_rd),
	           .rwe_n(rwe_n),
	           .rucas_n(rucas_n),
	           .rlcas_n(rlcas_n),
	           .rras0_n(rras0_n),
	           .rras1_n(rras1_n)
	         );


	wire [4:0] video_bw;

	wire [20:0] video_addr;
	wire video_strobe;
	wire video_next;
	wire video_pre_next;
	wire next_video;

	wire [20:0] dma_addr;
	wire [15:0] dma_wrdata;
	wire dma_req;
	wire dma_zwt;
	wire dma_rnw;
	wire dma_next;
	wire dma_strobe;

	wire [20:0] ts_addr;
	wire ts_req;
	wire ts_zwt;
	wire ts_pre_next;
	wire ts_next;

	arbiter dramarb(
					 .clk(fclk),
	                 // .c0(c0),
	                 .c1(c1),
	                 .c2(c2),
	                 .c3(c3),

	                 .rst_n(rst_n),
	                 .int_n(int_n),

	                 .dram_addr(daddr),
	                 .dram_req(dreq),
	                 .dram_rnw(drnw),
	                 .dram_bsel(dbsel),
	                 .dram_wrdata(dram_wrdata),

	                 //.cpu_waitcyc(cpu_waitcyc),
	                 .cpu_addr		(cpu_addr),
	                 .cpu_wrdata	(cpu_wrdata),
	                 .cpu_req		(cpu_req),
	                 .cpu_rnw		(cpu_rnw),
	                 .cpu_wrbsel	(cpu_wrbsel),
					 .cpu_next		(cpu_next),
	                 .cpu_strobe	(cpu_strobe),

	                 .go(go),
	                 .video_bw(video_bw),
	                 .video_addr(video_addr),
	                 .video_strobe(video_strobe),
	                 .video_pre_next(video_pre_next),
	                 .video_next(video_next),
	                 .next_video(next_video),

	                 .dma_addr		(dma_addr),
	                 .dma_wrdata	(dma_wrdata),
	                 .dma_req		(dma_req),
	                 .dma_zwt		(dma_zwt),
	                 .dma_rnw		(dma_rnw),
					 .dma_next		(dma_next),

					 .ts_req		(ts_req),
					 .ts_zwt		(ts_zwt),
					 .ts_addr		(ts_addr),
					 .ts_pre_next	(ts_pre_next),
					 .ts_next		(ts_next)
	);


        wire border_wr;
        wire zborder_wr;
        wire zvpage_wr;
        wire vpage_wr;
        wire vconf_wr;
        wire gx_offsl_wr;
        wire gx_offsh_wr;
        wire gy_offsl_wr;
        wire gy_offsh_wr;
        wire t0x_offsl_wr;
        wire t0x_offsh_wr;
        wire t0y_offsl_wr;
        wire t0y_offsh_wr;
        wire t1x_offsl_wr;
        wire t1x_offsh_wr;
        wire t1y_offsl_wr;
        wire t1y_offsh_wr;
        wire palsel_wr;
        wire hint_beg_wr;
        wire vint_begl_wr;
        wire vint_begh_wr;
        wire tsconf_wr;
        wire tmpage_wr;
        wire t0gpage_wr;
        wire t1gpage_wr;
        wire sgpage_wr;

	video_top video_top(

		.clk(fclk),
        .res(res),
		.f0(f0), .f1(f1),
		.h0(h0), .h1(h1),
		.c0(c0), .c1(c1), .c2(c2), .c3(c3),

		.vred(vred),
		.vgrn(vgrn),
		.vblu(vblu),
		.hsync(vhsync),
		.vsync(vvsync),
		.csync(vcsync),

		.vga_on(cfg_vga_on),

        .border_wr      (border_wr),
        .zborder_wr     (zborder_wr),
		.zvpage_wr	    (zvpage_wr),
		.vpage_wr	    (vpage_wr),
		.vconf_wr	    (vconf_wr),
		.gx_offsl_wr	(gx_offsl_wr),
		.gx_offsh_wr	(gx_offsh_wr),
		.gy_offsl_wr	(gy_offsl_wr),
		.gy_offsh_wr	(gy_offsh_wr),
		.t0x_offsl_wr	(t0x_offsl_wr),
		.t0x_offsh_wr	(t0x_offsh_wr),
		.t0y_offsl_wr	(t0y_offsl_wr),
		.t0y_offsh_wr	(t0y_offsh_wr),
		.t1x_offsl_wr	(t1x_offsl_wr),
		.t1x_offsh_wr	(t1x_offsh_wr),
		.t1y_offsl_wr	(t1y_offsl_wr),
		.t1y_offsh_wr	(t1y_offsh_wr),
		.palsel_wr	    (palsel_wr),
		.hint_beg_wr    (hint_beg_wr),
		.vint_begl_wr   (vint_begl_wr),
		.vint_begh_wr   (vint_begh_wr),
		.tsconf_wr	    (tsconf_wr),
		.tmpage_wr	    (tmpage_wr),
		.t0gpage_wr	    (t0gpage_wr),
		.t1gpage_wr	    (t1gpage_wr),
		.sgpage_wr	    (sgpage_wr),

		.video_addr     (video_addr),
		.video_bw		(video_bw),
		.video_go       (go),
		.dram_rddata    (dram_rddata),      // reg'ed, should be latched by c3
   		.dram_rdata     (dram_rd),               // raw, should be latched by c2
		.video_strobe   (video_strobe),
		.video_next     (video_next),
		.video_pre_next (video_pre_next),
		.next_video     (next_video),

		.ts_req			(ts_req),
		.ts_zwt			(ts_zwt),
		.ts_pre_next	(ts_pre_next),
		.ts_addr		(ts_addr),
		.ts_next		(ts_next),

		.a              (a),
        .d              (d),
		.zmd            (zmd),
		.zma            (zma),
		.cram_we        (cram_we),
		.sfile_we        (sfile_we),
		.int_start      (int_start)

	);


	slavespi slavespi(
		.fclk(fclk), .rst_n(rst_n),

		.spics_n(spics_n), .spidi(spidi),
		.spido(spido), .spick(spick),
		.status_in({/* wait_rnw */ wr_n, waits[6:0]}), .genrst(genrst),
		.rstrom(rstrom),
		.kbd_out(kbd_data), .kbd_out_sel(kbd_data_sel), .kbd_stb(kbd_stb),
		.mus_out(mus_data), .mus_xstb(mus_xstb), .mus_ystb(mus_ystb),
		.mus_btnstb(mus_btnstb), .kj_stb(kj_stb),
		.gluclock_addr(gluclock_addr),
		.comport_addr (comport_addr),
		.wait_write(wait_write),
		.wait_read(wait_read),
		.wait_rnw(wait_rnw),
		.wait_end(wait_end),
		.config0( { not_used[7:4], beeper_mux, tape_read, set_nmi[0], cfg_vga_on} )
	);

	zkbdmus zkbdmus( .fclk(fclk), .rst_n(rst_n),
	                 .kbd_in(kbd_data), .kbd_in_sel(kbd_data_sel), .kbd_stb(kbd_stb),
	                 .mus_in(mus_data), .mus_xstb(mus_xstb),
	                 .mus_ystb(mus_ystb), .mus_btnstb(mus_btnstb),
	                 .kj_stb(kj_stb), .kj_data(kj_port_data),
	                 .zah(a[15:8]), .kbd_data(kbd_port_data),
	                 .mus_data(mus_port_data)
	);


	wire [15:0]	   zmd;
	wire [7:0]	   zma;

    zmaps zmaps(
				.clk        	(fclk),
				.memwr_s    	(memwr_s),
				.a          	(a),
				.d          	(d),
				.fmaddr     	(fmaddr),
				.zmd        	(zmd),
				.zma        	(zma),
                .dma_wraddr   	(dma_wraddr),
                .dma_data   	(dma_data),
                .dma_cram_we	(dma_cram_we),
                .dma_sfile_we	(dma_sfile_we),
				.cram_we    	(cram_we),
				.sfile_we    	(sfile_we)
	);


	wire rst;
	wire m1;
	wire rfsh;
	wire rd;
	wire wr;
	wire iorq;
	wire iorq_s;
	wire iorq_s2;
	wire mreq;
	wire mreq_s;
	wire rdwr;
	wire iord;
	wire iowr;
	wire iorw;
	wire iord_s;
	wire iowr_s;
	wire iorw_s;
	wire memrd;
	wire memwr;
	wire memrw;
	wire memrd_s;
	wire memwr_s;
	wire memrw_s;
	wire opfetch;
	wire opfetch_s;
	wire intack;
	wire intack_s;

    zsignals zsignals(
                .clk        (fclk),
                .zpos       (zpos),
				.rst_n      (rst_n),
                .iorq_n     (iorq_n),
                .mreq_n     (mreq_n),
                .m1_n       (m1_n),
                .rfsh_n     (rfsh_n),
                .rd_n       (rd_n),
                .wr_n       (wr_n),

                .rst        (rst),
                .m1         (m1),
                .rfsh       (rfsh),
                .rd         (rd),
                .wr         (wr),
                .iorq       (iorq),
                .iorq_s     (iorq_s),
                .iorq_s2    (iorq_s2),
                .mreq       (mreq),
                .mreq_s     (mreq_s),
                .rdwr       (rdwr),
                .iord       (iord),
                .iowr       (iowr),
                .iorw       (iorw),
                .iord_s     (iord_s),
                .iowr_s     (iowr_s),
                .iorw_s     (iorw_s),
                .memrd      (memrd),
                .memwr      (memwr),
                .memrw      (memrw),
                .memrd_s    (memrd_s),
                .memwr_s    (memwr_s),
                .memrw_s    (memrw_s),
                .opfetch    (opfetch),
                .opfetch_s  (opfetch_s),
                .intack     (intack),
                .intack_s   (intack_s)
                );


		wire cram_we;
		wire sfile_we;


		wire [31:0] xt_page;

		wire [8:0] dmaport_wr;
		wire [4:0] fmaddr;

		wire [7:0] sysconf;
		wire [3:0] fddvirt;


	// AY control
	// reg pre_bc1,pre_bdir;
	// always @*
	// begin
		// pre_bc1 = 1'b0;
		// pre_bdir = 1'b0;

		// if( a[7:0]==8'hFD )
		// begin
			// if( a[15:14]==2'b11 )
			// begin
				// pre_bc1=1'b1;
				// pre_bdir=1'b1;
			// end
			// else if( a[15:14]==2'b10 )
			// begin
				// pre_bc1=1'b0;
				// pre_bdir=1'b1;
			// end
		// end
	// end

	// assign ay_bc1  = pre_bc1  & (~iorq_n) & ((~rd_n)|(~wr_n));
	// assign ay_bdir = pre_bdir & (~iorq_n) & (~wr_n);


    wire [15:0] z80_ide_out;
    wire z80_ide_cs0_n;
    wire z80_ide_cs1_n;
    wire z80_ide_req;
    wire z80_ide_rnw;

	zports zports(
                    .zclk       (zclk),
                    .clk        (fclk),

                    .din        (d),
                    .dout       (dout_ports),
                    .dataout    (ena_ports),
                    .a          (a),

                    .rst        (rst),
                    .opfetch    (opfetch),

                    .rd         (rd),
                    .wr         (wr),
                    .rdwr       (rdwr),

                    .iorq       (iorq),
                    .iord       (iord),
                    .iowr       (iowr),
                    .iorw       (iorw),

                    .iorq_s     (iorq_s),
                    .iorq_s2    (iorq_s2),
                    .iord_s     (iord_s),
                    .iowr_s     (iowr_s),
                    .iorw_s     (iorw_s),

                    .ay_bdir        (ay_bdir),
                    .ay_bc1         (ay_bc1),

                    .rstrom         (rstrom),
                    .vg_intrq       (intrq),
                    .vg_drq         (drq),
                    .vg_cs_n        (vg_cs_n),
                    .vg_wrFF        (vg_wrFF),

                    .sd_start       (sd_start),
                    .sd_dataout     (sd_dataout),
                    .sd_datain      (sd_datain),
                    .sdcs_n         (sdcs_n),

                    .ide_in         (ide_d),
                    .ide_out        (z80_ide_out),
                    .ide_cs0_n      (z80_ide_cs0_n),
                    .ide_cs1_n      (z80_ide_cs1_n),
                    .ide_req        (z80_ide_req),
                    .ide_stb        (ide_din_stb),
                    .ide_ready      (ide_ready),
                    .ide_stall      (ide_stall),

                    .border_wr      (border_wr),
                    .zborder_wr     (zborder_wr),
					.zvpage_wr	    (zvpage_wr),
					.vpage_wr	    (vpage_wr),
					.vconf_wr	    (vconf_wr),
					.gx_offsl_wr	(gx_offsl_wr),
					.gx_offsh_wr	(gx_offsh_wr),
					.gy_offsl_wr	(gy_offsl_wr),
					.gy_offsh_wr	(gy_offsh_wr),
					.t0x_offsl_wr	(t0x_offsl_wr),
					.t0x_offsh_wr	(t0x_offsh_wr),
					.t0y_offsl_wr	(t0y_offsl_wr),
					.t0y_offsh_wr	(t0y_offsh_wr),
					.t1x_offsl_wr	(t1x_offsl_wr),
					.t1x_offsh_wr	(t1x_offsh_wr),
					.t1y_offsl_wr	(t1y_offsl_wr),
					.t1y_offsh_wr	(t1y_offsh_wr),
					.palsel_wr	    (palsel_wr),
					.hint_beg_wr    (hint_beg_wr),
					.vint_begl_wr   (vint_begl_wr),
					.vint_begh_wr   (vint_begh_wr),
					.tsconf_wr	    (tsconf_wr),
					.tmpage_wr	    (tmpage_wr),
					.t0gpage_wr	    (t0gpage_wr),
					.t1gpage_wr	    (t1gpage_wr),
					.sgpage_wr	    (sgpage_wr),

					.xt_page    (xt_page),

					.fmaddr		(fmaddr),

					.sysconf	(sysconf),
					.memconf	(memconf),
					.im2vect	(im2vect),
					.fddvirt	(fddvirt),
                    .drive_sel  (vg_a),
                    .dos        (dos),
                    .vdos       (vdos),
                    .vdos_on    (vdos_on),
                    .vdos_off   (vdos_off),

					.dmaport_wr (dmaport_wr),
                    .dma_act	(dma_act),

                    .keys_in    (kbd_port_data),
                    .mus_in     (mus_port_data),
                    .kj_in      (kj_port_data),

                    .tape_read  (tape_read),

					.beeper_wr  (beeper_wr),
					.covox_wr   (covox_wr),

                    .gluclock_addr          (gluclock_addr),
                    .comport_addr           (comport_addr),
                    .wait_start_gluclock    (wait_start_gluclock),
                    .wait_start_comport     (wait_start_comport),
                    .wait_rnw               (wait_rnw),
                    .wait_read              (wait_read),
                    .wait_write             (wait_write),

                    .porthit    (porthit),
					.external_port  (external_port)
	);


	wire dma_act;

    wire [15:0] dma_data;
    wire [7:0] dma_wraddr;
	wire dma_cram_we;

	wire dma_sfile_we;

    wire [15:0] dma_ide_out;
    wire dma_ide_req;
    wire dma_ide_rnw;

	dma dma(
		.clk		(fclk),
		.c2		    (c2),
        .rst_n		(rst_n),

		.zdata		(d),
		.dmaport_wr	(dmaport_wr),
		.dma_act	(dma_act),

		.dram_addr	(dma_addr),
		.dram_rnw	(dma_rnw),
		.dram_req	(dma_req),
		.dma_zwt	(dma_zwt),
		.dram_rddata(dram_rd),
		.dram_wrdata(dma_wrdata),
		.dram_next	(dma_next),

        .data		(dma_data),
        .wraddr		(dma_wraddr),
		
        .cram_we    (dma_cram_we),
        .sfile_we   (dma_sfile_we),

        .ide_in     (ide_d),
        .ide_out    (dma_ide_out),
        .ide_req    (dma_ide_req),
        .ide_rnw    (dma_ide_rnw),
        .ide_stb    (ide_din_stb)
	);


	zint zint(
		.clk(fclk),
		.int_start(int_start),
		.vdos(vdos),
		.intack(intack),
		.int_n(int_n)
	);


	znmi znmi(
		.rst_n(rst_n),
		.fclk(fclk),
		.zpos(zpos),
		.zneg(zneg),

		.rfsh_n(rfsh_n),

		.int_start(int_start),

		.set_nmi(set_nmi),
		.clr_nmi(clr_nmi),

		.in_nmi (in_nmi),
		.gen_nmi(gen_nmi)
	);


	zwait zwait(
                 .wait_start_gluclock(wait_start_gluclock),
	             .wait_start_comport (wait_start_comport),
	             .wait_end(wait_end),
	             .rst_n(rst_n),
	             .wait_n(wait_n),
	             .waits(waits),
	             .spiint_n(spiint_n)
    );


	// wire [1:0] vg_ddrv;
	// assign vg_a[0] = vg_ddrv[0] ? 1'b1 : 1'b0; // possibly open drain?
	// assign vg_a[1] = vg_ddrv[1] ? 1'b1 : 1'b0;

	vg93 vgshka( .zclk(zclk), .rst_n(rst_n), .fclk(fclk), .vg_clk(vg_clk),
	             .vg_res_n(vg_res_n), .din(d), .intrq(intrq), .drq(drq), .vg_wrFF(vg_wrFF),
	             .vg_hrdy(vg_hrdy), .vg_rclk(vg_rclk), .vg_rawr(vg_rawr),
	             .vg_wrd(vg_wrd), .vg_side(vg_side), .step(step), .vg_sl(vg_sl), .vg_sr(vg_sr),
	             .vg_tr43(vg_tr43), .rdat_n(rdat_b_n), .vg_wf_de(vg_wf_de), .vg_drq(vg_drq),
	             .vg_irq(vg_irq), .vg_wd(vg_wd) );


	spi2 zspi( .clk(fclk), .sck(sdclk), .sdo(sddo), .sdi(sddi), .start(sd_start),
	           .speed(2'b00),	// this is 14 MHz at 28 Mhz Altera clock
			   .din(sd_datain), .dout(sd_dataout)
             );


	assign ide_rs_n = rst_n;
	wire [15:0] ide_out;
    wire ide_din_stb;
    wire ide_ready;

    ide ide(
            .clk        (fclk),
            .reset      (res),
            .din_stb    (ide_din_stb),
            .rdy        (ide_ready),

            .ide_d      (ide_d),
            .ide_a      (ide_a),
            .ide_dir    (ide_dir),
            .ide_cs0_n  (ide_cs0_n),
            .ide_cs1_n  (ide_cs1_n),
            .ide_rd_n   (ide_rd_n),
            .ide_wr_n   (ide_wr_n),

            .dma_out    (dma_ide_out),
            .dma_req    (dma_ide_req),
            .dma_rnw    (dma_ide_rnw),

            .z80_out     (z80_ide_out),
            .z80_a       (a[7:5]),
            .z80_cs0_n   (z80_ide_cs0_n),
            .z80_cs1_n   (z80_ide_cs1_n),
            .z80_req     (z80_ide_req),
            .z80_rnw     (!rd_n)
    );


	sound sound(
		.clk(fclk), .f0(f0),
		.din(d),
		.beeper_wr(beeper_wr),
		.covox_wr (covox_wr),
		.beeper_mux(beeper_mux),
		.sound_bit(beep)
        );


endmodule

