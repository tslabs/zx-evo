// simulate fpga top-level with external dram, rom, z80
// (c) 2010-2012 NedoPC

`include "../include/tune.v"



//`define ZLOG 1



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

	wire res;                    //
	tri1 ziorq_n,zmreq_n,zrd_n,zwr_n,zm1_n,zrfsh_n; // connected to Z80

	tri1 int_n,wait_n,nmi_n;
	wire zint_n,zwait_n,znmi_n;

	wire [15:0] #((`Z80_DELAY_DOWN+`Z80_DELAY_UP)/2) za;
	wire [ 7:0] #((`Z80_DELAY_DOWN+`Z80_DELAY_UP)/2) zd;
//	wire [15:0] za;
//	wire [ 7:0] zd;

	wire [ 7:0] zd_dut_to_z80;
//	wire [ 7:0] zd_z80_to_dut;


	wire csrom, romoe_n, romwe_n;
	wire rompg0_n, dos_n;
	wire rompg2,rompg3,rompg4;

	wire [15:0] rd;
	wire [9:0] ra;
	wire rwe_n,rucas_n,rlcas_n,rras0_n,rras1_n;


	tri1 [15:0] ide_d;


	wire hsync,vsync;
	wire [1:0] red,grn,blu;



	// sdcard
	wire sdcs_n, sddo, sddi, sdclk;

	// avr
	wire spick, spidi, spido, spics_n;




	assign zwait_n = (wait_n==1'b0) ? 1'b0 : 1'b1;
	assign znmi_n = (nmi_n==1'b0) ? 1'b0 : 1'b1;
	assign zint_n = (int_n==1'b0) ? 1'b0 : 1'b1;






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
		 .rompg2(rompg2),
		 .rompg3(rompg3),
 		 .rompg4(rompg4),

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
	         .sddi(sddi),
	         .sddo(sddo),
	         .sdcs_n(sdcs_n),
	         .sdclk(sdclk),

	         // ATmega SPI
	         .spics_n(spics_n),
	         .spick(spick),
	         .spido(spido),
	         .spidi(spidi),

		 .vhsync(hsync),
		 .vvsync(vsync),
		 .vred(red),
		 .vgrn(grn),
		 .vblu(blu)

	       );




	assign zd_dut_to_z80 = tb.DUT.ena_ram ? tb.DUT.dout_ram : ( tb.DUT.ena_ports ? tb.DUT.dout_ports : ( tb.DUT.drive_ff ? 8'hFF : 8'bZZZZZZZZ ) );





	wire zrst_n = ~res;

	T80a z80( .RESET_n(zrst_n),
	          .CLK_n(clkz_in),
	          .WAIT_n(zwait_n),
	          .INT_n(zint_n),
	          .NMI_n(znmi_n),
	          .M1_n(zm1_n),
	          .RFSH_n(zrfsh_n),
	          .MREQ_n(zmreq_n),
	          .IORQ_n(ziorq_n),
	          .RD_n(zrd_n),
	          .WR_n(zwr_n),
	          .BUSRQ_n(1'b1),
	          .A(za),
//	          .D(zd),
	          .D_I(zd_dut_to_z80),
	          .D_O(zd)
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
	           .addr( {rompg4,rompg3,rompg2,dos_n, (~rompg0_n), za[13:0]} ),
	           .data(zd_dut_to_z80),
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

	defparam dramko1._init_ = 0;
	defparam dramko2._init_ = 0;



`ifndef GATE

	// trace rom page
	wire rma14,rma15;

	assign rma14 = DUT.page[0][0];
	assign rma15 = DUT.page[0][1];


	always @(rma14 or rma15)
	begin
//		$display("at time %t us",$time/1000000);

//		case( {rma15, rma14} )

//		2'b00: $display("BASIC 48");
//		2'b01: $display("TR-DOS");
//		2'b10: $display("BASIC 128");
//		2'b11: $display("GLUKROM");
//		default: $display("unknown");

//		endcase

//		$display("");
	end


	// trace ram page
	wire [5:0] rpag;

	assign rpag=DUT.page[3][5:0];

	always @(rpag)
	begin
//		$display("at time %t us",$time/1000000);

//		$display("RAM page is %d",rpag);

//		$display("");
	end



	// key presses/nmi/whatsoever
	initial
	begin
		#1;
		tb.DUT.zkbdmus.kbd = 40'd0;
		tb.DUT.zkbdmus.kbd[36] = 1'b1;
		@(negedge int_n);
		@(negedge int_n);
		tb.DUT.zkbdmus.kbd[36] = 1'b0;
	end
/*
	initial
	begin : gen_nmi

		reg [21:0] a;

		#1000000000;

		a = 22'h3FC066;

		put_byte(a,8'hF5); a=a+1;
		put_byte(a,8'hC5); a=a+1;
		put_byte(a,8'hD5); a=a+1;
		put_byte(a,8'hE5); a=a+1;

		put_byte(a,8'h10); a=a+1;
		put_byte(a,8'hFE); a=a+1;

		put_byte(a,8'h14); a=a+1;

		put_byte(a,8'h01); a=a+1;
		put_byte(a,8'hFE); a=a+1;
		put_byte(a,8'h7F); a=a+1;

		put_byte(a,8'hED); a=a+1;
		put_byte(a,8'h51); a=a+1;

		put_byte(a,8'hED); a=a+1;
		put_byte(a,8'h78); a=a+1;

		put_byte(a,8'h1F); a=a+1;

		put_byte(a,8'hDA); a=a+1;
		put_byte(a,8'h6A); a=a+1;
		put_byte(a,8'h00); a=a+1;

		put_byte(a,8'hE1); a=a+1;
		put_byte(a,8'hD1); a=a+1;
		put_byte(a,8'hC1); a=a+1;
		put_byte(a,8'hF1); a=a+1;

		put_byte(a,8'hD3); a=a+1;
		put_byte(a,8'hBE); a=a+1;

		put_byte(a,8'hED); a=a+1;
		put_byte(a,8'h45); a=a+1;


		@(posedge fclk);
		tb.DUT.slavespi.cfg0_reg_out[1] = 1'b1;
		@(posedge fclk);
		tb.DUT.slavespi.cfg0_reg_out[1] = 1'b0;

		#64000000;

		tb.DUT.zkbdmus.kbd[39] = 1'b1;
		@(negedge int_n);
		tb.DUT.zkbdmus.kbd[39] = 1'b0;
	end
*/

`endif








`ifdef ZLOG
	reg [ 7:0] old_opcode;
	reg [15:0] old_opcode_addr;

	wire [7:0] zdd = zd_dut_to_z80;

	reg was_m1;

	always @(zm1_n)
	if( zm1_n )
		was_m1 <= 1'b0;
	else
		was_m1 = 1'b1;

	always @(posedge (zmreq_n | zrd_n | zm1_n | (~zrfsh_n)) )
	if( was_m1 )
	begin
		if( (zdd!==old_opcode) || (za!==old_opcode_addr) )
		begin
			if( tb.DUT.z80mem.romnram )
//				$display("Z80OPROM: addr %x, opcode %x, time %t",za,zdd,$time);
				$display("Z80OPROM: addr %x, opcode %x",za,zdd);
			else
//				$display("Z80OPRAM: addr %x, opcode %x, time %t",za,zdd,$time);
				$display("Z80OPRAM: addr %x, opcode %x",za,zdd);
		end

		old_opcode      = zdd;
		old_opcode_addr = za;
	end

	always @(posedge (zmreq_n | zrd_n | (~zm1_n) | (~zrfsh_n)) )
	if( !was_m1 )
	begin
		if( tb.DUT.z80mem.romnram )
//			$display("Z80RDROM: addr %x, rddata %x, time %t",za,zdd,$time);
			$display("Z80RDROM: addr %x, rddata %x",za,zdd);
		else
//			$display("Z80RDRAM: addr %x, rddata %x, time %t",za,zdd,$time);
			$display("Z80RDRAM: addr %x, rddata %x",za,zdd);
	end

	always @(posedge (zmreq_n | zwr_n | (~zm1_n) | (~zrfsh_n)) )
	begin
		if( tb.DUT.z80mem.romnram )
//			$display("Z80WRROM: addr %x, wrdata %x, time %t",za,zd,$time);
			$display("Z80WRROM: addr %x, wrdata %x",za,zd);
		else
//			$display("Z80WRRAM: addr %x, wrdata %x, time %t",za,zd,$time);
			$display("Z80WRRAM: addr %x, wrdata %x",za,zd);
	end
`endif




	// turbo
`ifdef C7MHZ
	initial
		force tb.DUT.zclock.turbo = 2'b01;
`else
	`ifdef C35MHZ

		initial
			force tb.DUT.zclock.turbo = 2'b00;

	`endif
`endif





	// force fetch mode
//	initial
//	begin
//		force tb.DUT.dramarb.bw = 2'b11;
//
//		#(64'd2400000000);
//
//		release tb.DUT.dramarb.bw;
//	end








`ifndef NO_PIXER
	// picture out
	pixer pixer
	(
		.clk(fclk),

		.vsync(vsync),
		.hsync(hsync),
		.red(red),
		.grn(grn),
		.blu(blu)
	);
`endif


/*
	// time ticks
	always
	begin : timemark

		integer ms;

		ms = ($time/1000000);

//		$display("timemark %d ms",ms);

		#10000000.0; // 1 ms
	end
*/




	// init dram
	initial
	begin : init_dram
		integer i;

		for(i=0;i<4*1024*1024;i=i+1)
		begin
			put_byte(i,(i%257));
		end
	end






`ifdef SPITEST
	// spitest printing module
	// does not hurt at any time (yet), so attached forever

	spitest_print spitest_print(
		.sdclk (sdclk ),
		.sddi  (sddi  ),
		.sddo  (sddo  ),
		.sdcs_n(sdcs_n)
	);

	// spitest AVR imitator

	spitest_avr spitest_avr(
		.spick  (spick  ),
		.spics_n(spics_n),
		.spido  (spido  ),
		.spidi  (spidi  )
	);
`else
	assign sddi = 1'b1;

	assign spics_n = 1'b1;
	assign spick   = 1'b0;
	assign spido   = 1'b1;
`endif





	// set up breakpoint
	initial
	begin
		#(650_000_000); // wait 650ms = 650*1000*1000 ns

		@(posedge fclk);

		tb.DUT.zports.brk_ena  = 1'b1;
		tb.DUT.zports.brk_addr = 16'h0041;
	end











	task put_byte;

		input [21:0] addr;
		input [ 7:0] data;



		reg [19:0] arraddr;

		begin

			arraddr = { addr[21:12], addr[11:2] };

			case( addr[1:0] ) // chipsel, bytesel

			2'b00: tb.dramko1.array[arraddr][15:8] = data;
			2'b01: tb.dramko1.array[arraddr][ 7:0] = data;
			2'b10: tb.dramko2.array[arraddr][15:8] = data;
			2'b11: tb.dramko2.array[arraddr][ 7:0] = data;

			endcase
		end

	endtask





endmodule


