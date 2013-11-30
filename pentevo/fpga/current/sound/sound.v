`include "../include/tune.v"

// Pentevo project (c) NedoPC 2011
//
// integrates sound features: tapeout, beeper and covox

module sound(

	input  wire	clk, f0,

	input  wire [7:0] din,

	input  wire	beeper_wr,
	input  wire	covox_wr,

	input  wire	beeper_mux, // output either tape_out or beeper

	output reg	sound_bit
	
);

	reg [8:0] ctr;
	reg [8:0] val;

	reg mx_beep_n_covox;

	reg beep_bit;
	reg beep_bit_old;

// port writes
	always @(posedge clk)
		if (covox_wr)
				val[7:0] <= din;
		else
		if (beeper_wr)
				val[8] <= {beeper_mux ? din[3] : din[4]};

// SD generator
	wire gte = val >= ctr;
	
	always @(posedge clk)
	begin
		sound_bit <= gte;
		ctr <= {9{gte}} - val + ctr;
	end

endmodule

