// part of NewGS project (c) 2007 NedoPC
//
// ramtest

module main(

	clk_fpga,  // clocks
	clk_24mhz, //

	clksel0, // clock selection
	clksel1, //

	warmres_n, // warm reset


	d, // Z80 data bus
	a, // Z80 address bus

	iorq_n,   // Z80 control signals
	mreq_n,   //
	rd_n,     //
	wr_n,     //
	m1_n,     //
	int_n,    //
	nmi_n,    //
	busrq_n,  //
	busak_n,  //
	z80res_n, //


	mema14,   // memory control
	mema15,   //
	mema16,   //
	mema17,   //
	mema18,   //
	ram0cs_n, //
	ram1cs_n, //
	ram2cs_n, //
	ram3cs_n, //
	romcs_n,  //
	memoe_n,  //
	memwe_n,  //


	zxid,        // zxbus signals
	zxa,         //
	zxa14,       //
	zxa15,       //
	zxiorq_n,    //
	zxmreq_n,    //
	zxrd_n,      //
	zxwr_n,      //
	zxcsrom_n,   //
	zxblkiorq_n, //
	zxblkrom_n,  //
	zxgenwait_n, //
	zxbusin,     //
	zxbusena_n,  //


	dac_bitck, // audio-DAC signals
	dac_lrck,  //
	dac_dat,  //


	sd_clk, // SD card interface
	sd_cs,  //
	sd_do,  //
	sd_di,  //
	sd_wp,  //
	sd_det, //


	ma_clk, // control interface of MP3 chip
	ma_cs,
	ma_do,
	ma_di,

	mp3_xreset, // data interface of MP3 chip
	mp3_req,    //
	mp3_clk,    //
	mp3_dat,    //
	mp3_sync,   //

	led_diag
);


// input-output description

	input clk_fpga;
	input clk_24mhz;

	output clksel0; reg clksel0;
	output clksel1; reg clksel1;


	input warmres_n;

	inout  [7:0] d;// reg [7:0] d;
///////////////////////////////////////////////////////////	input [15:0] a;
	output [15:0] a; wire [15:0] a;

	input iorq_n;
	input mreq_n;
	input rd_n;
	input wr_n;
	input m1_n;
	output int_n; wire int_n;
	output nmi_n; wire nmi_n;
	output busrq_n;	wire busrq_n;
	input busak_n;
	output z80res_n; reg z80res_n;


	output mema14; wire mema14;
	output mema15; wire mema15;
	output mema16; wire mema16;
	output mema17; wire mema17;
	output mema18; wire mema18;
	output ram0cs_n; wire ram0cs_n;
	output ram1cs_n; wire ram1cs_n;
	output ram2cs_n; wire ram2cs_n;
	output ram3cs_n; wire ram3cs_n;
	output romcs_n; wire romcs_n;
	output memoe_n; wire memoe_n;
	output memwe_n; wire memwe_n;


	inout [7:0] zxid; wire [7:0] zxid;
	input [7:0] zxa;
	input zxa14;
	input zxa15;
	input zxiorq_n;
	input zxmreq_n;
	input zxrd_n;
	input zxwr_n;
	input zxcsrom_n;
	output zxblkiorq_n; wire zxblkiorq_n;
	output zxblkrom_n; wire zxblkrom_n;
	output zxgenwait_n; wire zxgenwait_n;
	output zxbusin; wire zxbusin;
	output zxbusena_n; wire zxbusena_n;


	output dac_bitck; wire dac_bitck;
	output dac_lrck; wire dac_lrck;
	output dac_dat; wire dac_dat;


	output sd_clk; wire sd_clk;
	output sd_cs; wire sd_cs;
	output sd_do; wire sd_do;
	input sd_di;
	input sd_wp;
	input sd_det;


	output ma_clk; wire ma_clk;
	output ma_cs; wire ma_cs;
	output ma_do; wire ma_do;
	input ma_di;

	output mp3_xreset; wire mp3_xreset;
	input mp3_req;
	output mp3_clk; wire mp3_clk;
	output mp3_dat; wire mp3_dat;
	output mp3_sync; wire mp3_sync;

	output led_diag;

	always @* clksel0 <= 1'b0;
	always @* clksel1 <= 1'b0;

	always @* z80res_n <= 1'b0;

	assign busrq_n = 1'b1;
	assign int_n = 1'b1;
	assign nmi_n = 1'b1;

	assign romcs_n = 1'b1;

	assign zxid=8'bZZZZZZZZ;
	assign zxblkrom_n=1'b1;
	assign zxgenwait_n=1'b1;
	assign zxbusin=1'b1;
	assign zxbusena_n=1'b1;

	assign dac_bitck = 1'b1;
	assign dac_lrck = 1'b1;
	assign dac_dat = 1'b1;

	assign sd_clk = 1'b0;
	assign sd_cs = 1'b1;
	assign sd_do = 1'b0;

	assign ma_clk = 1'b0;
	assign ma_cs = 1'b1;
	assign ma_do = 1'b0;
	assign mp3_xreset = 1'b0;
	assign mp3_clk = 1'b0;
	assign mp3_dat = 1'b0;
	assign mp3_sync= 1'b0;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


	wire rst_zx = (zxa[7:0]==8'h33) & (~zxiorq_n) & (~zxwr_n);
	assign zxblkiorq_n = ~(zxa[7:0]==8'h33);

	wire rst_n;
	resetter myreset( .clk(clk_fpga), .rst_in_n( warmres_n & (~rst_zx) ), .rst_out_n(rst_n) );


	wire sel0,sel1;
	wire ramce;

	mem_tester mytst( .clk(clk_fpga), .rst_n(rst_n), .led(led_diag),
	                  .SRAM_DQ(d), .SRAM_ADDR( {sel1,sel0,mema18,mema17,mema16,mema15,mema14,a[13:0]} ),
	                  .SRAM_WE_N( memwe_n ), .SRAM_OE_N( memoe_n ), .SRAM_CE_N( ramce ) );
	defparam mytst.SRAM_ADDR_SIZE = 21;


	assign ram0cs_n = ( {sel1,sel0}==2'd0 )?ramce:1'b1;
	assign ram1cs_n = ( {sel1,sel0}==2'd1 )?ramce:1'b1;
	assign ram2cs_n = ( {sel1,sel0}==2'd2 )?ramce:1'b1;
	assign ram3cs_n = ( {sel1,sel0}==2'd3 )?ramce:1'b1;






endmodule
