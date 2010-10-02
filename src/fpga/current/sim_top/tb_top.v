// simulate fpga top-level with external dram, rom, z80

`include "../include/tune.v"


`define HALF_CLK_PERIOD (17.8)

`define ZCLK_DELAY      (9.5)

// toshibo
//`define Z80_DELAY_DOWN  (17.0)
//`define Z80_DELAY_UP    (22.0)

// z0840008
`define Z80_DELAY_DOWN   34
`define Z80_DELAY_UP     30

module tb;

	reg fclk;

	wire clkz_out,clkz_in;

	reg iorq_n,mreq_n,rd_n,wr_n; // has some delays relative to z*_n (below)
	reg m1_n,rfsh_n;             //

	wire int_n,nmi_n,wait_n,res;                    //
	tri1 ziorq_n,zmreq_n,zrd_n,zwr_n,zm1_n,zrfsh_n; // connected to Z80

	assign nmi_n = 1'b1;
	assign wait_n = 1'b1;


	wire [15:0] za;
	wire [7:0] zd;


	wire csrom, romoe_n, romwe_n;


	wire [15:0] rd;
	wire [9:0] ra;
	wire rwe_n,rucas_n,rlcas_n,rras0_n,rras1_n;


	tri1 [15:0] ide_d;



	initial
	begin

		fclk = 1'b0;

		forever #`HALF_CLK_PERIOD fclk = ~fclk;
	end


	assign #`ZCLK_DELAY clkz_in = ~clkz_out; // 9.4ns







	top DUT( .fclk(fclk),
	         .clkz_out(clkz_out),
	         .clkz_in(clkz_in),

               // z80
	         .iorq_n(iorq_n),
	         .mreq_n(mreq_n),
	         .rd_n(rd_n),
	         .wr_n(wr_n),
	         .m1_n(m1_n),
	         .rfsh_n(rfsh_n),
	         .int_n(int_n),
	         .nmi_n(nmi_n),
	         .wait_n(wait_n),
	         .res(res),
	         //
	         .d(zd),
	         .a(za),

	         // ROM
	         .csrom(csrom),
	         .romoe_n(romoe_n),
	         .romwe_n(romwe_n),

	         // DRAM
	         .rd(rd),
	         .ra(ra),
	         .rwe_n(rwe_n),
	         .rucas_n(rucas_n),
	         .rlcas_n(rlcas_n),
	         .rras0_n(rras0_n),
	         .rras1_n(rras1_n),

	         // ZX-bus
	         .iorqge1(1'b0),
	         .iorqge2(1'b0),

	         // IDE
	         .ide_d(ide_d),
	         .ide_rdy(1'b1),

	         // VG93
	         .step(1'b0),
	         .vg_sl(1'b0),
	         .vg_sr(1'b0),
	         .vg_tr43(1'b0),
	         .rdat_b_n(1'b1),
	         .vg_wf_de(1'b0),
	         .vg_drq(1'b1),
	         .vg_irq(1'b1),
	         .vg_wd(1'b0),

	         // SDcard SPI
	         .sddi(1'b1),

	         // ATmega SPI
	         .spics_n(1'b1),
	         .spick(1'b0),
	         .spido(1'b1)

	       );


	wire zrst_n = ~res;

	T80a z80( .RESET_n(zrst_n),
	          .CLK_n(clkz_in),
	          .WAIT_n(wait_n),
	          .INT_n(int_n),
	          .NMI_n(nmi_n),
	          .M1_n(zm1_n),
	          .RFSH_n(zrfsh_n),
	          .MREQ_n(zmreq_n),
	          .IORQ_n(ziorq_n),
	          .RD_n(zrd_n),
	          .WR_n(zwr_n),
	          .BUSRQ_n(1'b1),
	          .A(za),
	          .D(zd)
	        );

	// now make delayed versions of signals
	//
	always @(zm1_n)
		if( zm1_n )
			m1_n <= #`Z80_DELAY_UP zm1_n;
		else
			m1_n <= #`Z80_DELAY_DOWN zm1_n;
	//
	always @(zrfsh_n)
		if( zrfsh_n )
			rfsh_n <= #`Z80_DELAY_UP zrfsh_n;
		else
			rfsh_n <= #`Z80_DELAY_DOWN zrfsh_n;
	//
	always @(zmreq_n)
		if( zmreq_n )
			mreq_n <= #`Z80_DELAY_UP zmreq_n;
		else
			mreq_n <= #`Z80_DELAY_DOWN zmreq_n;
	//
	always @(ziorq_n)
		if( ziorq_n )
			iorq_n <= #`Z80_DELAY_UP ziorq_n;
		else
			iorq_n <= #`Z80_DELAY_DOWN ziorq_n;
	//
	always @(zrd_n)
		if( zrd_n )
			rd_n <= #`Z80_DELAY_UP zrd_n;
		else
			rd_n <= #`Z80_DELAY_DOWN zrd_n;
	//
	// special handling for broken WR_n
	always @(negedge clkz_in)
		if( zwr_n )
			wr_n <= #`Z80_DELAY_UP zwr_n;
		else
			wr_n <= #`Z80_DELAY_DOWN zwr_n;


	// ROM model
	rom romko(
	           .addr( {/*dos_n, (~rompg0_n),*/ 2'b00, za[13:0]} ),
	           .data(zd),
	           .ce_n( romoe_n | (~csrom) )
	         );

	// DRAM model
	drammem dramko1(
	                 .ma(ra),
	                 .d(rd),
	                 .ras_n(rras0_n),
	                 .ucas_n(rucas_n),
	                 .lcas_n(rlcas_n),
	                 .we_n(rwe_n)
	               );
	//
	drammem dramko2(
	                 .ma(ra),
	                 .d(rd),
	                 .ras_n(rras1_n),
	                 .ucas_n(rucas_n),
	                 .lcas_n(rlcas_n),
	                 .we_n(rwe_n)
	               );
	defparam dramko1._verbose_ = 0;
	defparam dramko2._verbose_ = 0;



endmodule


