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
	inout [15:0] rd,
	output [9:0] ra,
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

	output ide_dir,

	input ide_rdy,

	output ide_cs0_n,
	output ide_cs1_n,
	output ide_rs_n,
	output ide_rd_n,
	output ide_wr_n,

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

	wire f0, f1, c0, c1, c2, c3;
	wire [1:0] ay_mod;

	clock clock
	(
		.clk(fclk),
		.f0(f0), .f1(f1),
		.h0(h0), .h1(h1),
		.c0(c0), .c1(c1), .c2(c2), .c3(c3),
		.ay_clk(ay_clk),
		.ay_mod(sysconf[4:3])
	);


	wire dos;


	wire zpos,zneg;

	wire rst_n; // global reset

	wire rrdy;
	wire [15:0] rddata;

	wire [4:0] rompg;

	wire [7:0] zports_dout;
	wire zports_dataout;
	wire porthit;


	wire [39:0] kbd_data;
	wire [ 7:0] mus_data;
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

	wire [15:0] ideout;
	wire [15:0] idein;
	wire idedataout;


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








	assign ide_rs_n = rst_n;

	assign ide_d = idedataout ? ideout : 16'hZZZZ;
	assign idein = ide_d;

	assign ide_dir = ~idedataout;





	wire [7:0] peff7;


	wire romrw_en;
	wire cpm_n;
	wire fnt_wr;



	wire cpu_req,cpu_rnw,cpu_wrbsel,cpu_strobe;
	wire [20:0] cpu_addr;
	// wire [15:0] cpu_rddata;
	wire [7:0] cpu_wrdata;

	wire go;


	wire sd_start;
	wire [7:0] sd_dataout,sd_datain;


	wire tape_read; // data for tapein

	wire beeper_mux; // what is mixed to FPGA beeper output - beeper (0) or tapeout (1)

	wire [2:0] atm_scr_mode;

	wire atm_turbo;


	wire beeper_wr, covox_wr;



	wire [5:0] palcolor; // palette readback




	wire [1:0] int_turbo;
	wire cpu_next;
	wire cpu_stall;

	wire external_port;


	// fix ATM2-style ROM addressing for PENT-like ROM layout.
	// this causes compications when writing to the flashROM from Z80
	// and need to split and re-build old ATM romfiles before burning in
	// flash
//	wire [1:0] adr_fix;
//	assign adr_fix = ~{ rompg[0], rompg[1] };
//	assign rompg0_n = ~adr_fix[0];
//	assign dos_n    =  adr_fix[1];
//	assign rompg2   =  1'b0;//rompg[2];
//	assign rompg3   =  1'b0;//rompg[3];
//	assign rompg4   =  1'b0;//rompg[4];

	assign rompg0_n = ~rompg[0];
	assign dos_n    =  rompg[1];
	assign rompg2   =  rompg[2];
	assign rompg3   =  rompg[3];
	assign rompg4   =  rompg[4];

	wire [3:0] zclk_stall;

	zclock zclock
	(
		.fclk(fclk), .rst_n(rst_n), .zclk(zclk), .rfsh_n(rfsh_n), .zclk_out(clkz_out),
		.zpos(zpos), .zneg(zneg),
		.turbo( {atm_turbo,~(peff7[4])}), .c2(c2), .c0(c0),
		.zclk_stall( cpu_stall | (|zclk_stall)), .int_turbo(int_turbo),
		.external_port(external_port), .iorq_n(iorq_n), .m1_n(m1_n)
	);



	wire [7:0] dout_ram;
	wire ena_ram;
	wire [7:0] dout_ports;
	wire ena_ports;


	wire [7:0] border;

	wire drive_ff;


	wire       atm_palwr;
	wire [5:0] atm_paldata;

	wire int_start;


	// data bus out: either RAM data or internal ports data or 0xFF with unused ports
	assign d = ena_ram ? dout_ram : ( ena_ports ? dout_ports : ( drive_ff ? 8'hFF : 8'bZZZZZZZZ ) );




	zbus zxbus( .iorq_n(iorq_n), .rd_n(rd_n), .wr_n(wr_n), .m1_n(m1_n),
	            .iorq1_n(iorq1_n), .iorq2_n(iorq2_n), .iorqge1(iorqge1), .iorqge2(iorqge2),
	            .porthit(porthit), .drive_ff(drive_ff) );




	/////////////////////////////////////
	// ATM memory pagers instantiation //
	/////////////////////////////////////

	wire atmF7_wr_fclk;
	// wire [3:0] atmF7_wr = atmF7_wr_fclk & (1 << a[15:14]);
	
	wire rampage_wr;	// ports #10AF-#13AF
	// wire [3:0] rampg_wr = rampage_wr & (1 << a[9:8]);

	wire pager_off;

	wire        pent1m_ROM;
	wire [ 5:0] pent1m_page;
	wire        pent1m_ram0_0;
	wire        pent1m_1m_on;

	wire [ 7:0] memconf;

	wire [3:0] dos_turn_off,
	           dos_turn_on;

	wire [ 7:0] page [0:3];
	wire [ 3:0] romnram;

	// for reading back data via xxBE port
	wire [ 7:0] rd_pages [0:7];
	wire [ 7:0] rd_ramnrom;
	wire [ 7:0] rd_dos7ffd;

	// by now you can only use page setting for RAM at #0000, if ROM, the page is not altered
	wire [3:0] xt_override1 = {xt_override[3:1], xt_override[0] & memconf[3]};
	wire [3:0] xt_shadow = {{2{memconf[4]}}, 2'b0};
	// wire [3:0] xt_shadow = {4{memconf[4]}};
	
	generate

		genvar i;

		for(i=0;i<4;i=i+1)
		begin : instantiate_atm_pagers

			atm_pager #( .ADDR(i) )
			          atm_pager( .rst_n(rst_n),
			                     .fclk (fclk),
			                     .zpos (zpos),
			                     .zneg (zneg),

			                     .za(a),
			                     .zd(d),
			                     .mreq_n(mreq_n),
			                     .rd_n  (rd_n),
			                     .m1_n  (m1_n),

			                     .pager_off(pager_off),

			                     .pent1m_ROM   (pent1m_ROM),
			                     .pent1m_page  (pent1m_page),
			                     .pent1m_ram0_0(pent1m_ram0_0),
			                     .pent1m_1m_on (pent1m_1m_on),

								 .xt_rampage (xt_ramp[i]),
								 .xt_rampagsh (xt_ramp[i[0]+4]),
								 // .xt_rompage (xt_rompage),
								 .xt_override(xt_override1[i]),
								 .xt_shadow(xt_shadow[i]),
								 // .xt_override(xt_override[i]),
								 // .memconf	(memconf),

			                     .in_nmi(in_nmi),

			                     .atmF7_wr(atmF7_wr_fclk),
								 // .rampg_wr(rampage_wr),

			                     .dos(dos),

			                     .dos_turn_on (dos_turn_on[i]),
			                     .dos_turn_off(dos_turn_off[i]),

			                     .zclk_stall(zclk_stall[i]),

			                     .page   (page[i]),
			                     .romnram(romnram[i]),


			                     .rd_page0  (rd_pages[i  ]),
			                     .rd_page1  (rd_pages[i+4]),

			                     .rd_ramnrom( {rd_ramnrom[i+4], rd_ramnrom[i]}),
			                     .rd_dos7ffd( {rd_dos7ffd[i+4], rd_dos7ffd[i]} )
			                   );
		end

	endgenerate


	///////////////////////////
	// DOS signal controller //
	///////////////////////////

	zdos zdos( .rst_n(rst_n),

	           .fclk(fclk),

	           .dos_turn_on ( |dos_turn_on),
	           .dos_turn_off( |dos_turn_off),

	           .cpm_n(cpm_n),

	           .dos(dos)
	         );




	///////////////////////////
	// Z80 memory controller //
	///////////////////////////

	zmem z80mem
	(
		.fclk (fclk),
		.rst_n(rst_n),
		
		.zpos(zpos),
		.zneg(zneg),

		.c0     (c0),
		.c1(c1),
		.c2 (c2),
		.c3     (c3),
		
		.za    (a),
		.zd_in (d),
		.zd_out(dout_ram), 
		.zd_ena(ena_ram), 
		.m1_n  (m1_n),
		.rfsh_n(rfsh_n), 
		.iorq_n(iorq_n), 
		.mreq_n(mreq_n),
		.rd_n  (rd_n), 
		.wr_n  (wr_n),

		.win_page({page[3], page[2], page[1], page[0]}),
		.win_romnram(romnram),

		.romrw_en(romrw_en),

		.rompg  (rompg),
		.romoe_n(romoe_n),
		.romwe_n(romwe_n),
		.csrom  (csrom),

		.cpu_req   (cpu_req),
		.cpu_rnw   (cpu_rnw),
		.cpu_wrbsel(cpu_wrbsel),
		.cpu_strobe(cpu_strobe),
		.cpu_addr  (cpu_addr),
		.cpu_wrdata(cpu_wrdata),
		.cpu_rddata(dram_rddata),
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
	           .rrdy(drrdy),
	           .rddata(dram_rddata),
	           .wrdata(dram_wrdata),
	           .bsel(dbsel),

	           .ra(ra),
	           .rd(rd),
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

	wire [20:0] dma_addr;
	wire [15:0] dma_wrdata;
	wire dma_req;
	wire dma_rnw;
	wire dma_next;
	wire dma_strobe;
					 
	wire [20:0] ts_addr;
	wire ts_req;
	wire ts_next;
	wire ts_strobe;

	arbiter dramarb(
					 .clk(fclk),
	                 .c2(c2),
	                 .c3(c3),

	                 .rst_n(rst_n),

	                 .dram_addr(daddr),
	                 .dram_req(dreq),
	                 .dram_rnw(drnw),
	                 .dram_rrdy(drrdy),
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
	                 .video_next(video_next),
					 
	                 .dma_addr		(dma_addr),
	                 .dma_wrdata	(dma_wrdata),
	                 .dma_req		(dma_req),
	                 .dma_rnw		(dma_rnw),
					 .dma_next		(dma_next),
	                 .dma_strobe	(dma_strobe),
					 
					 // .ts_req		(ts_req),
					 .ts_req		(0),	//debug
					 .ts_addr		(ts_addr),
					 .ts_next		(ts_next),
					 .ts_strobe		(ts_strobe)
	);

					 
					 
	video_top video_top(

		.clk(fclk),
		.zclk(zclk),
		.f0(f0), .f1(f1),
		.h0(h0), .h1(h1),
		.c0(c0), .c1(c1), .c2(c2), .c3(c3),
		// .t0(t0),	//debug!!!

		.vred(vred),
		.vgrn(vgrn),
		.vblu(vblu),
		.hsync(vhsync),
		.vsync(vvsync),
		.csync(vcsync),

		.border(border),
		.vpage(vpage),
		.vconf(vconf),
		.x_offs(x_offs),
		.y_offs(y_offs),
		.y_offs_wr(y_offs_wr),
		.p7ffd_wr(p7ffd_wr),
		.tsconf(tsconf),
		.tgpage(tgpage),
		
		.hint_beg(hint_beg),
		.vint_beg(vint_beg),
		
		.vga_on(cfg_vga_on),

		.video_addr     (video_addr),
		.video_bw		(video_bw),
		.video_go       (go),
		.dram_rddata    (dram_rddata),
		.video_strobe   (video_strobe),
		.video_next     (video_next),

		.ts_req			(ts_req),
		.ts_addr		(ts_addr),
		.ts_next		(ts_next),
		.ts_strobe		(ts_strobe),
		
		.a(a),
		.cram_data_in({d[6:0], zmd}),
		.sfys_data_in({d[7:0], zmd}),
		.cram_we(cram_we),
		.sfys_we(sfys_we),
		.int_start(int_start)

	);


	slavespi slavespi(
		.fclk(fclk), .rst_n(rst_n),

		.spics_n(spics_n), .spidi(spidi),
		.spido(spido), .spick(spick),
		.status_in({/* wait_rnw */ wr_n, waits[6:0]}), .genrst(genrst),
		.rstrom(rstrom), .kbd_out(kbd_data),
		.kbd_stb(kbd_stb), .mus_out(mus_data),
		.mus_xstb(mus_xstb), .mus_ystb(mus_ystb),
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
	                 .kbd_in(kbd_data), .kbd_stb(kbd_stb),
	                 .mus_in(mus_data), .mus_xstb(mus_xstb),
	                 .mus_ystb(mus_ystb), .mus_btnstb(mus_btnstb),
	                 .kj_stb(kj_stb), .kj_data(kj_port_data),
	                 .zah(a[15:8]), .kbd_data(kbd_port_data),
	                 .mus_data(mus_port_data)
	               );

				   
		wire [7:0]	   zmd	;
				   
zmaps zmaps(
					.mreq_n(mreq_n),
					.wr_n(wr_n),
					.a(a),
					.d(d),
					.zclk(zclk),

					.fmaddr(fmaddr),

					.zmd(zmd),
					
					.cram_we(cram_we),
					.sfys_we(sfys_we)
				);
				
		
		wire cram_we;
		wire sfys_we;
		
		wire [7:0] vconf;
		wire [7:0] vpage;
		wire [8:0] x_offs;
		wire [8:0] y_offs;
		wire [7:0] tsconf;
		
		wire [47:0] xt_rampage;
		wire [4:0] xt_rompage;
		wire [3:0] xt_override;	// crotch!!!
		
		wire [4:0] dmaport_wr;
		wire y_offs_wr;
		
		wire [7:0] xt_ramp[0:5];
		assign {xt_ramp[5], xt_ramp[4], xt_ramp[3], xt_ramp[2], xt_ramp[1], xt_ramp[0]} = xt_rampage;
		
		wire [4:0] fmaddr;
		wire [4:0] tgpage;
		
		wire [7:0] sysconf;
		wire [7:0] hint_beg;
		wire [8:0] vint_beg;
		wire [7:0] im2vect ;
		

	zports zports( .zclk(zclk), .fclk(fclk), .rst_n(rst_n), .zpos(zpos), .zneg(zneg),
	               .din(d), .dout(dout_ports), .dataout(ena_ports),
	               .a(a), .iorq_n(iorq_n), .rd_n(rd_n), .wr_n(wr_n), .porthit(porthit),
	               .ay_bdir(ay_bdir), .ay_bc1(ay_bc1), .border(border),
	               .peff7(peff7), .mreq_n(mreq_n), .m1_n(m1_n), .dos(dos),
	               .rstrom(rstrom), .vg_intrq(intrq), .vg_drq(drq), .vg_wrFF(vg_wrFF),
	               .vg_cs_n(vg_cs_n), .sd_start(sd_start), .sd_dataout(sd_dataout),
	               .sd_datain(sd_datain), .sdcs_n(sdcs_n),
	               .idein(idein), .ideout(ideout), .idedataout(idedataout),
	               .ide_a(ide_a), .ide_cs0_n(ide_cs0_n), .ide_cs1_n(ide_cs1_n),
	               .ide_wr_n(ide_wr_n), .ide_rd_n(ide_rd_n),
	               // .t0(t0), //debug!!!
					
					.vconf		(vconf),
					.vpage		(vpage),
					.x_offs		(x_offs),
					.y_offs		(y_offs),
					.tsconf		(tsconf),
					
					.xt_rampage (xt_rampage),
					.xt_rompage	(xt_rompage),
					.xt_override(xt_override),
					
					.fmaddr		(fmaddr),
					.tgpage		(tgpage),
					
					.sysconf	(sysconf),
					.memconf	(memconf),
					.hint_beg	(hint_beg),
					.vint_beg	(vint_beg),
					.im2vect	(im2vect),
					
					.dmaport_wr (dmaport_wr),
                    .y_offs_wr(y_offs_wr),
                    .p7ffd_wr(p7ffd_wr),
					
	               .keys_in(kbd_port_data),
	               .mus_in (mus_port_data),
	               .kj_in  (kj_port_data),

	               .tape_read(tape_read),

	               .gluclock_addr(gluclock_addr),
	               .comport_addr (comport_addr),
	               .wait_start_gluclock(wait_start_gluclock),
	               .wait_start_comport (wait_start_comport),
	               .wait_rnw  (wait_rnw),
	               .wait_write(wait_write),
`ifndef SIMULATE
	               .wait_read (wait_read),
`else
	               .wait_read(8'hFF),
`endif
					.atmF7_wr_fclk(atmF7_wr_fclk),
					// .rampage_wr (rampage_wr),
			
					.atm_scr_mode(atm_scr_mode),
					.atm_turbo   (atm_turbo),
					.atm_pen     (pager_off),
					.atm_cpm_n   (cpm_n),
					.atm_pen2    (atm_pen2),
			
					.romrw_en(romrw_en),
			
					.pent1m_ram0_0(pent1m_ram0_0),
					.pent1m_1m_on (pent1m_1m_on),
					.pent1m_page  (pent1m_page),
					.pent1m_ROM   (pent1m_ROM),
			
					.atm_palwr  (atm_palwr),
					.atm_paldata(atm_paldata),
			
					.beeper_wr(beeper_wr),
					.covox_wr (covox_wr),
			
					.fnt_wr(fnt_wr),
					.clr_nmi(clr_nmi),
			
			
					.pages(~{ rd_pages[7], rd_pages[6],
							rd_pages[5], rd_pages[4],
							rd_pages[3], rd_pages[2],
							rd_pages[1], rd_pages[0] }),
			
					.ramnroms( rd_ramnrom),
					.dos7ffds( rd_dos7ffd),
			
					.palcolor(palcolor),
			
					.external_port(external_port),
			
			
					.set_nmi(set_nmi[1])
	);

	
	wire dma_act;
	wire dma_zwait;
	
	dma dma(
		.clk		(fclk),
        .rst_n		(rst_n),
		
		.zdata		(d),
		.dmaport_wr	(dmaport_wr),
		.dma_act	(dma_act),
		.dma_zwait	(dma_zwait),
		
		.dma_addr	(dma_addr),
		.dma_rnw	(dma_rnw),
		.dma_req	(dma_req),
		.dma_rddata	(dram_rddata),
		.dma_wrdata	(dma_wrdata),
		.dma_next	(dma_next),
		.dma_stb	(dma_strobe)
	);
	

	zint zint(
		.fclk(fclk),
		.zpos(zpos),
		.zneg(zneg),

		.int_start(int_start),

		.iorq_n(iorq_n),
		.m1_n  (m1_n),

		.int_n(int_n)
	);

	znmi znmi
	(
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




	zwait zwait( .wait_start_gluclock(wait_start_gluclock),
	             .wait_start_comport (wait_start_comport),
	             .wait_end(wait_end),
	             .rst_n(rst_n),
	             .wait_n(wait_n),
	             .waits(waits),
	             .spiint_n(spiint_n) );




	wire [1:0] vg_ddrv;
	assign vg_a[0] = vg_ddrv[0] ? 1'b1 : 1'b0; // possibly open drain?
	assign vg_a[1] = vg_ddrv[1] ? 1'b1 : 1'b0;

	vg93 vgshka( .zclk(zclk), .rst_n(rst_n), .fclk(fclk), .vg_clk(vg_clk),
	             .vg_res_n(vg_res_n), .din(d), .intrq(intrq), .drq(drq), .vg_wrFF(vg_wrFF),
	             .vg_hrdy(vg_hrdy), .vg_rclk(vg_rclk), .vg_rawr(vg_rawr), .vg_a(vg_ddrv),
	             .vg_wrd(vg_wrd), .vg_side(vg_side), .step(step), .vg_sl(vg_sl), .vg_sr(vg_sr),
	             .vg_tr43(vg_tr43), .rdat_n(rdat_b_n), .vg_wf_de(vg_wf_de), .vg_drq(vg_drq),
	             .vg_irq(vg_irq), .vg_wd(vg_wd) );




	spi2 zspi( .clk(fclk), .sck(sdclk), .sdo(sddo), .sdi(sddi), .start(sd_start),
	           .speed(2'b00),	// this is 14 MHz at 28 Mhz Altera clock
			   .din(sd_datain), .dout(sd_dataout) );





	  //////////////////////////////////////
	 // sound: beeper, tapeout and covox //
	//////////////////////////////////////

	sound sound(

		.clk(fclk), .f0(f0),

		.din(d),

		.beeper_wr(beeper_wr),
		.covox_wr (covox_wr),

		.beeper_mux(beeper_mux),

		.sound_bit(beep)
	);


endmodule

