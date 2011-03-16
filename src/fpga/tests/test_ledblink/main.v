module main(

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


	wire zclk; // z80 clock for short
	reg [2:0] zclk_gen; // make 3.5 mhz clock

	wire rst_n; // global reset

	wire rrdy;
	wire cbeg;
	wire [15:0] rddata;

	wire [4:0] rompg;

	wire [7:0] zports_dout;
	wire zports_dataout;
	wire porthit;

	wire [4:0] keys;
	wire tape_in;

	wire [15:0] ideout;
	wire [15:0] idein;


	wire [7:0] zmem_dout;
	wire zmem_dataout;


	wire [7:0] sd_dout_to_zports;
	wire start_from_zports;
	wire sd_inserted;
	wire sd_readonly;


	reg [3:0] ayclk_gen;


	wire [7:0] received;
	wire [7:0] tobesent;


	wire intrq,drq;
	wire vg_wrFF;





	// Z80 clock control
	assign zclk = clkz_in;
	always @(posedge fclk)
		zclk_gen <= zclk_gen + 3'd1;
	assign clkz_out = zclk_gen[2];


/*	// RESETTER
	resetter myrst( .clk(fclk),
	                .rst_in1_n(1'b1),
	                .rst_in2_n(1'b1),
	                .rst_out_n(rst_n) );
	defparam myrst.RST_CNT_SIZE = 6;
*/

	dram mydram( .clk(fclk),
	             .rst_n(1'b1),
	             .ra(ra),
	             .rd(rd),
	             .rwe_n(rwe_n),
	             .rras0_n(rras0_n),
	             .rras1_n(rras1_n),
	             .rucas_n(rucas_n),
	             .rlcas_n(rlcas_n),
	             .req(1'b0),
	             .rnw(1'b1)//,
	             //.rrdy(rrdy),
	             //.cbeg(cbeg),
	             //.bsel({a[0],~a[0]}),
	             //.wrdata({d,~d}),
	             //.rddata(rddata),
	             /*.addr({a[5:0],a[15:1]})*/ );

	assign int_n=1'b1;
	assign nmi_n=1'b1;
	assign wait_n=1'b1;
	assign res=1'b1;
	assign d=8'hZZ;


	assign csrom=1'b0;
	assign romoe_n=1'b1;
	assign romwe_n=1'b1;

	assign iorq1_n=1'b1;
	assign iorq2_n=1'b1;


	assign rd=16'hZZZZ;


	assign ay_bdir=1'b0;
	assign ay_bc1=1'b0;


	assign ide_d=16'hZZZZ;
	assign ide_dir=1'b1;

	assign ide_rs_n = 1'b0;
	assign ide_cs1_n = 1'b1;
	assign ide_rd_n = 1'b1;
	assign ide_wr_n = 1'b1;

	assign vg_cs_n=1'b1;
	assign vg_res_n=1'b0;


	assign sdcs_n=1'b1;

	assign spiint_n=1'b1;



//AY control
	always @(posedge fclk)
	begin
		ayclk_gen <= ayclk_gen + 4'd1;
	end

	assign ay_clk = ayclk_gen[3];




	reg [23:0] counter;
	reg blinker;


	always @(posedge fclk)
	begin
		if( !counter )
		begin
			counter <= 24'd13999999;
			blinker <= ~blinker;
		end
		else
			counter <= counter - 24'd1;
	end


	assign ide_cs0_n = blinker;



endmodule
