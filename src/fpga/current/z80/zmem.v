`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Z80 memory manager: routes ROM/RAM accesses, makes wait-states for 14MHz or stall condition, etc.
//
//
// fclk    _/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zclk     /```\___/```\___/```\___/```````\_______/```````\_______/```````````````\_______________/```````````````\_______________/`
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zpos     `\___/```\___/```\___/```\___________/```\___________/```\___________________________/```\___________________________/```\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zneg     _/```\___/```\___/```\_______/```\___________/```\___________________/```\___________________________/```\________________

module zmem(

	input fclk,
	input rst_n,

	input zpos, //
	input zneg, // strobes which show positive and negative edges of zclk; this is to stay in single clock domain

	input cend,  // DRAM cycle end
	input pre_cend, // pre cycle end


	input [15:0] za,

	input [7:0] zd_in, // won't emit anything to Z80 bus, data bus mux is another module
	output reg [7:0] zd_out, // output to Z80 bus

	output zd_ena, // out bus to the Z80

	input m1_n,
	input rfsh_n,
	input mreq_n,
	input iorq_n,
	input rd_n,
	input wr_n,




	input  wire        win0_romnram, // four windows, each 16k,
	input  wire        win1_romnram, // ==1 - there is rom,
	input  wire        win2_romnram, // ==0 - there is ram
	input  wire        win3_romnram, //

	input  wire [ 7:0] win0_page, // which 16k page is in given window
	input  wire [ 7:0] win1_page, //
	input  wire [ 7:0] win2_page, //
	input  wire [ 7:0] win3_page, //


	input  wire        romrw_en,


	output reg  [ 4:0] rompg, // output for ROM paging
	output wire        romoe_n,
	output wire        romwe_n,
	output wire        csrom,


	output cpu_req,
	output cpu_rnw,
	output [20:0] cpu_addr,
	output [7:0] cpu_wrdata,
	output cpu_wrbsel,

	input [15:0] cpu_rddata,
	input cpu_strobe

);


	wire [1:0] win;
	reg [7:0] page;
	reg romnram;

	wire ramreq;

	wire ramwr,ramrd;

	reg ramrd_reg,ramwr_reg,ramrd_prereg;


	// make paging
	assign win[1:0] = za[15:14];

	always @*
	case( win )
		2'b00: begin
			page    = win0_page;
			romnram = win0_romnram;
		end

		2'b01: begin
			page    = win1_page;
			romnram = win1_romnram;
		end

		2'b10: begin
			page    = win2_page;
			romnram = win2_romnram;
		end

		2'b11: begin
			page    = win3_page;
			romnram = win3_romnram;
		end
	endcase


	// rom paging - only half a megabyte addressing.
	always @*
	begin
		rompg[4:0] = page[4:0];
	end




	assign romwe_n = wr_n | mreq_n | (~romrw_en);
	assign romoe_n = rd_n | mreq_n;

	assign csrom = romnram; // positive polarity!



	// DRAM accesses

	assign ramreq = (~mreq_n) && (~romnram) && rfsh_n;

	assign ramrd = ramreq & (~rd_n);
	assign ramwr = ramreq & (~wr_n);


	assign zd_ena = ramrd;
	assign cpu_wrdata = zd_in;

	assign cpu_wrbsel = za[0];
	assign cpu_addr[20:0] = { page[7:0], za[13:1] };

	always @* if( cpu_strobe ) // WARNUNG! ACHTING! LATCH!!!
		zd_out <= cpu_wrbsel ? cpu_rddata[7:0] : cpu_rddata[15:8];


//	always @(posedge fclk) if( pre_cend )
//		ramrd_prereg <= ramrd;
//	assign cpu_rnw = ramrd_prereg; // is it correct???
//
// removed because it could be source of problems for NMOS Z80
//
// new one:
//
	assign cpu_rnw = ramrd;


	always @(posedge fclk) if( cend )
	begin
		ramrd_reg <= ramrd;
		ramwr_reg <= ramwr;
	end

	assign cpu_req = ( ramrd & (~ramrd_reg) ) | ( ramwr & (~ramwr_reg) );



endmodule

