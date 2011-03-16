// simulate fpga top-level with external dram, rom, z80
// (c) 2010 NedoPC

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

	wire int_n,res;                    //
	tri1 ziorq_n,zmreq_n,zrd_n,zwr_n,zm1_n,zrfsh_n; // connected to Z80

	tri1 wait_n,nmi_n;
	wire zwait_n,znmi_n;

	wire [15:0] za;
	wire [7:0] zd;


	wire csrom, romoe_n, romwe_n;
	wire rompg0_n, dos_n;

	wire [15:0] rd;
	wire [9:0] ra;
	wire rwe_n,rucas_n,rlcas_n,rras0_n,rras1_n;


	tri1 [15:0] ide_d;



	assign zwait_n = (wait_n==1'b0) ? 1'b0 : 1'b1;
	assign znmi_n = (nmi_n==1'b0) ? 1'b0 : 1'b1;



	initial
	begin

		fclk = 1'b0;

		forever #`HALF_CLK_PERIOD fclk = ~fclk;
	end


	assign #`ZCLK_DELAY clkz_in = ~clkz_out;







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
	         .rompg0_n(rompg0_n),
	         .dos_n(dos_n),

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
	          .WAIT_n(zwait_n),
	          .INT_n(int_n),
	          .NMI_n(znmi_n),
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
	reg  mreq_wr_n;
	wire iorq_wr_n, full_wr_n;
	//
	// first, assure there is no X's at the start
	//
	initial
	begin
		m1_n      = 1'b1;
		rfsh_n    = 1'b1;
		mreq_n    = 1'b1;
		iorq_n    = 1'b1;
		rd_n      = 1'b1;
		wr_n      = 1'b1;
		mreq_wr_n = 1'b1;
	end
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
	//
	// special handling for broken T80 WR_n
	//	
	always @(negedge clkz_in)
		mreq_wr_n <= zwr_n;
	//
	assign iorq_wr_n = ziorq_n | (~zrd_n) | (~zm1_n);
	//
	assign full_wr_n = mreq_wr_n & iorq_wr_n;
	//
	// this way glitches won't affect state of wr_n
	always @(full_wr_n)
		if( !full_wr_n )
			#`Z80_DELAY_DOWN wr_n <= full_wr_n;
		else
			#`Z80_DELAY_UP wr_n <= full_wr_n;





	// ROM model
	rom romko(
	           .addr( {dos_n, (~rompg0_n), za[13:0]} ),
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



`ifndef GATE

	// trace rom page
	wire rma14,rma15;

	assign rma14 = DUT.page[0][0];
	assign rma15 = DUT.page[0][1];


	always @(rma14 or rma15)
	begin
		$display("at time %t us",$time/10000);

		case( {rma15, rma14} )

		2'b00: $display("BASIC 48");
		2'b01: $display("TR-DOS");
		2'b10: $display("BASIC 128");
		2'b11: $display("GLUKROM");
		default: $display("unknown");

		endcase

		$display("");
	end


	// trace ram page
	wire [5:0] rpag;

	assign rpag=DUT.page[3][5:0];

	always @(rpag)
	begin
		$display("at time %t us",$time/10000);

		$display("RAM page is %d",rpag);

		$display("");
	end



	// emulate key presses
	initial
	begin
		tb.DUT.zkbdmus.kbd = 40'd0;
		
		#600000000;

		@(negedge int_n);

		tb.DUT.zkbdmus.kbd[13] = 1'b1;

		@(negedge int_n);
		@(negedge int_n);

		tb.DUT.zkbdmus.kbd[13] = 1'b0;
		
//		$stop;
	end



`endif


	// time ticks
	always
	begin : timemark

		integer ms;

		ms = ($time/1000000);

		$display("timemark %d ms",ms);

		#10000000.0; // 1 ms
	end









endmodule


