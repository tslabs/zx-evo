`include "../include/tune.v"

// Pentevo project (c) NedoPC 2011
//
// integrates sound features: tapeout, beeper and covox

module sound(

	input  wire       clk, f0,

	input  wire [7:0] din,

	input  wire       beeper_wr,
	input  wire       covox_wr,

	input  wire       beeper_mux, // output either tape_out or beeper

	output reg       sound_bit
	
);

	reg [8:0] ctr;
	reg [7:0] val;

	reg mx_beep_n_covox;

	reg beep_bit;
	reg beep_bit_old;

// port writes
	always @(posedge clk)
		if (covox_wr)
				val <= din;
		else
		if (beeper_wr)
				val <= {8{beeper_mux ? din[3] : din[4]}};

// PWM generator
	always @(posedge clk)		// 28 MHz strobes, Fpwm = 54.7 kHz (two semi-periods)
		ctr <= ctr + 9'b1;

	always @(posedge clk)
		sound_bit <= ((ctr[8] ? ctr[7:0] : ~ctr[7:0]) < val);

endmodule

