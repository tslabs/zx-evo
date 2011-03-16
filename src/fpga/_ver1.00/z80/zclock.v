// PentEvo project (c) NedoPC 2008-2009
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



`ifdef SIMULATE
	initial // simulation...
	begin
		precend_cnt = 1'b0;
		int_turbo   = 2'b00;
		old_rfsh_n  = 1'b1;
	end
`endif

	// take every other pulse of pre_cend (make half pre_cend)
	always @(posedge fclk) if( pre_cend )
		precend_cnt <= ~precend_cnt;

	assign h_precend_1 =  precend_cnt && pre_cend;
	assign h_precend_2 = !precend_cnt && pre_cend;

/*	// phase zcount to take from it proper 3.5 or 7 MHz clock
	always @(posedge fclk)
	begin
		if( half_precend )
			zcount <= 3'd7;
		else
			zcount <= zcount - 3'd1;
	end
*/

	// switch between 3.5 and 7 only at predefined time
	always @(posedge fclk) if(zpos)
	begin
		old_rfsh_n <= rfsh_n;

		if( old_rfsh_n && !rfsh_n )
			int_turbo <= turbo;
	end

/*	always @(posedge fclk) if( h_precend_1 )
		int_turbo <= turbo;
*/

	always @(posedge fclk)
	begin
		if( (pre_cend && int_turbo[0]) || (h_precend_2 && !int_turbo[0]) )
			zpos <= 1'b1;
		else
			zpos <= 1'b0;
	end

	always @(posedge fclk)
	begin
		if( (cbeg && int_turbo[0]) || (h_precend_1 && !int_turbo[0]) )
			zneg <= 1'b1;
		else
			zneg <= 1'b0;
	end




	// make Z80 clock: account for external inversion and make some leading of clock
	// 9.5 ns propagation delay: from fclk posedge to zclk returned back any edge
	// (1/28)/2=17.9ns half a clock lead
	// 2.6ns lag because of non-output register emitting of zclk_out
	// total: 5.8 ns lead of any edge of zclk relative to posedge of fclk => ACCOUNT FOR THIS WHEN DOING INTER-CLOCK DATA TRANSFERS
	//
/*	always @(negedge fclk)
		if( int_turbo[0] ) // 7 MHz
			zclk_out <= ~zcount[1];
		else // 3.5 MHz
			zclk_out <= ~zcount[2];
*/

	always @(negedge fclk)
	begin
		if( zpos )
			zclk_out <= 1'b0;

		if( zneg )
			zclk_out <= 1'b1;
	end


endmodule

