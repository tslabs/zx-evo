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


//    FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
// CURRENTLY ONLY 3.5 and 7 MHz!!!! FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
//    FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME

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


	input cbeg,
	input pre_cend // syncing signals, taken from arbiter.v and dram.v
);


	reg precend_cnt;
	wire h_precend_1; // to take every other pulse of pre_cend
	wire h_precend_2; // to take every other pulse of pre_cend

	reg [2:0] zcount; // counter for generating 3.5 and 7 MHz z80 clocks
	reg [1:0] int_turbo; // internal turbo, controlling muxes


	reg old_rfsh_n;



	wire pre_zpos,pre_zneg;


`ifdef SIMULATE
	initial // simulation...
	begin
		precend_cnt = 1'b0;
		int_turbo   = 2'b00;
		old_rfsh_n  = 1'b1;
	end
`endif

	// switch between 3.5 and 7 only at predefined time
	always @(posedge fclk) if(zpos)
	begin
		old_rfsh_n <= rfsh_n;

		if( old_rfsh_n && !rfsh_n )
			int_turbo <= turbo;
	end



	// take every other pulse of pre_cend (make half pre_cend)
	always @(posedge fclk) if( pre_cend )
		precend_cnt <= ~precend_cnt;

	assign h_precend_1 =  precend_cnt && pre_cend;
	assign h_precend_2 = !precend_cnt && pre_cend;



	assign pre_zpos = (pre_cend && int_turbo[0]) || (h_precend_2 && !int_turbo[0]);
	assign pre_zneg = (cbeg && int_turbo[0]) || (h_precend_1 && !int_turbo[0]);


	// probably have here fix for not pulsing again zneg,
	// if zpos was blocked by zclk_stall, and vice versa
	always @(posedge fclk)
	begin
		zpos <= (~zclk_stall) & pre_zpos;
	end

	always @(posedge fclk)
	begin
		zneg <= (~zclk_stall) & pre_zneg;
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

