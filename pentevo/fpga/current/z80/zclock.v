// PentEvo project (c) NedoPC 2008-2011
//
// Z80 clocking module, also contains some wait-stating when 14MHz
//
// IDEAL:
// fclk    _/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zclk     /```\___/```\___/```\___/```````\_______/```````\_______/```````````````\_______________/```````````````\_______________/`
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zpos     `\___/```\___/```\___/```\___________/```\___________/```\___________________________/```\___________________________/```\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zneg     _/```\___/```\___/```\_______/```\___________/```\___________________/```\___________________________/```\________________

// clock phasing:
// cend must be zpos for 7mhz, therefore post_cbeg - zneg
// for 3.5 mhz, cend is both zpos and zneg (alternating)




// 14MHz rulez:
// 1. do variable stalls for memory access.
// 2. do fallback on 7mhz for external IO accesses
// 3. clock switch 14-7-3.5 only at RFSH



`include "../include/tune.v"

module zclock(

	input fclk,
	input rst_n,

	input zclk, // Z80 clock, buffered via act04 and returned back to the FPGA

	input rfsh_n, // switch turbo modes in RFSH part of m1


	output reg zclk_out, // generated Z80 clock - passed through inverter externally!

	output reg zpos,
	output reg zneg,


	input  wire zclk_stall,




	input [1:0] turbo, // 2'b00 -  3.5 MHz
	                   // 2'b01 -  7.0 MHz
	                   // 2'b1x - 14.0 MHz

	output reg [1:0] int_turbo, // internal turbo, switched on /RFSH


	// input signals for 14MHz external IORQ waits
	input  wire external_port,
	input  wire iorq_n,
	input  wire m1_n,


	input cbeg,
	input pre_cend // syncing signals, taken from arbiter.v and dram.v
);


	reg precend_cnt;
	wire h_precend_1; // to take every other pulse of pre_cend
	wire h_precend_2; // to take every other pulse of pre_cend

	reg [2:0] zcount; // counter for generating 3.5 and 7 MHz z80 clocks


	reg old_rfsh_n;


	wire stall;


	reg clk14_src; // source for 14MHz clock





	wire pre_zpos_35,
	     pre_zneg_35;

	wire pre_zpos_70,
	     pre_zneg_70;

	wire pre_zpos_140,
	     pre_zneg_140;




`ifdef SIMULATE
	initial // simulation...
	begin
		precend_cnt = 1'b0;
		int_turbo   = 2'b00;
		old_rfsh_n  = 1'b1;
		clk14_src   = 1'b0;

		zclk_out = 1'b0;
	end
`endif

	// switch clock only at predefined time
	always @(posedge fclk) if(zpos)
	begin
		old_rfsh_n <= rfsh_n;

		if( old_rfsh_n && !rfsh_n )
			int_turbo <= turbo;
	end




	// make 14MHz iorq wait
	reg [3:0] io_wait_cnt;
	
	reg io_wait;

	wire io;
	reg  io_r;

	assign io = (~iorq_n) & m1_n & external_port;

	always @(posedge fclk)
	if( zpos )
		io_r <= io;

	always @(posedge fclk, negedge rst_n)
	if( ~rst_n )
		io_wait_cnt <= 4'd0;
	else if( io && (!io_r) && zpos && int_turbo[1] )
		io_wait_cnt[3] <= 1'b1;
	else if( io_wait_cnt[3] )
		io_wait_cnt <= io_wait_cnt + 4'd1;

	always @(posedge fclk)
	case( io_wait_cnt )
		4'b1000: io_wait <= 1'b1;
		4'b1001: io_wait <= 1'b1;
		4'b1010: io_wait <= 1'b1;
		4'b1011: io_wait <= 1'b1;
		4'b1100: io_wait <= 1'b1;
		4'b1101: io_wait <= 1'b0;
		4'b1110: io_wait <= 1'b1;
		4'b1111: io_wait <= 1'b0;
		default: io_wait <= 1'b0;		
	endcase




	assign stall = zclk_stall | io_wait;



	// 14MHz clocking
	always @(posedge fclk)
	if( !stall )
		clk14_src <= ~clk14_src;
	//
	assign pre_zpos_140 =   clk14_src ;
	assign pre_zneg_140 = (~clk14_src);



	// take every other pulse of pre_cend (make half pre_cend)
	always @(posedge fclk) if( pre_cend )
		precend_cnt <= ~precend_cnt;

	assign h_precend_1 =  precend_cnt && pre_cend;
	assign h_precend_2 = !precend_cnt && pre_cend;


	assign pre_zpos_35 = h_precend_2;
	assign pre_zneg_35 = h_precend_1;

	assign pre_zpos_70 = pre_cend;
	assign pre_zneg_70 = cbeg;


	assign pre_zpos = int_turbo[1] ? pre_zpos_140 : ( int_turbo[0] ? pre_zpos_70 : pre_zpos_35 );
	assign pre_zneg = int_turbo[1] ? pre_zneg_140 : ( int_turbo[0] ? pre_zneg_70 : pre_zneg_35 );



	always @(posedge fclk)
	begin
		zpos <= (~stall) & pre_zpos & zclk_out;
	end

	always @(posedge fclk)
	begin
		zneg <= (~stall) & pre_zneg & (~zclk_out);
	end

	




	// make Z80 clock: account for external inversion and make some leading of clock
	// 9.5 ns propagation delay: from fclk posedge to zclk returned back any edge
	// (1/28)/2=17.9ns half a clock lead
	// 2.6ns lag because of non-output register emitting of zclk_out
	// total: 5.8 ns lead of any edge of zclk relative to posedge of fclk => ACCOUNT FOR THIS WHEN DOING INTER-CLOCK DATA TRANSFERS
	//

	always @(negedge fclk)
	begin
		if( zpos )
			zclk_out <= 1'b0;

		if( zneg )
			zclk_out <= 1'b1;
	end


endmodule

