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

	output wire       sound_bit
	
);

	reg [7:0] ctr;
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
				val <= {din[3], {7{din[4]}}};


// PWM generator
	always @(posedge clk) if (f0)		// 14 MHz strobes, Fpwm = 54.7 kHz
		ctr <= ctr + 8'b1;

	assign sound_bit = ( ctr < val );


endmodule

