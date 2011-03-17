// PentEvo project (c) NedoPC 2008-2009
//
// Z80 clocking module, also contains some wait-stating when 14MHz
//
//




//    FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
// CURRENTLY ONLY 3.5 and 7 MHz!!!! FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
//    FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME

//`include "../include/tune.v"

module zclock(

	input fclk,
	input rst_n,

	input zclk, // Z80 clock, buffered via act04 and returned back to the FPGA

	output reg zclk_out, // generated Z80 clock - passed through inverter externally!

	input [1:0] turbo, // 2'b00 -  3.5 MHz
	                   // 2'b01 -  7.0 MHz
	                   // 2'b1x - 14.0 MHz


	input pre_cend // syncing signal, taken from arbiter.v module
);


	reg precend_cnt;
	wire half_precend; // to take every other pulse of pre_cend

	reg [2:0] zcount; // counter for generating 3.5 and 7 MHz z80 clocks
	reg [1:0] int_turbo; // internal turbo, controlling muxes


	initial // simulation...
	begin
		precend_cnt = 1'b0;
	end


	// take every other pulse of pre_cend (make half pre_cend)
	always @(posedge fclk) if( pre_cend )
		precend_cnt <= ~precend_cnt;

	assign half_precend = precend_cnt && pre_cend;

	// phase zcount to take from it proper 3.5 or 7 MHz clock
	always @(posedge fclk)
	begin
		if( half_precend )
			zcount <= 3'd7;
		else
			zcount <= zcount - 3'd1;
	end

	// switch between 3.5 and 7 only at predefined times
	always @(posedge fclk) if( half_precend )
		int_turbo <= turbo;


	// make Z80 clock: account for external inversion and make some leading of clock
	// 9.5 ns propagation delay: from fclk posedge to zclk returned back any edge
	// (1/28)/2=17.9ns half a clock lead
	// 2.6ns lag because of non-output register emitting of zclk_out
	// total: 5.8 ns lead of any edge of zclk relative to posedge of fclk => ACCOUNT FOR THIS WHEN DOING INTER-CLOCK DATA TRANSFERS
	//
`ifdef SIMULATE
	always @(posedge fclk) // for simulation!
`else
	always @(negedge fclk) // normal working - see comment above
`endif
		if( int_turbo[0] ) // 7 MHz
			zclk_out <= ~zcount[1];
		else // 3.5 MHz
			zclk_out <= ~zcount[2];



endmodule

