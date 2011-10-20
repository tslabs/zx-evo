`include "../include/tune.v"

// Pentevo project (c) NedoPC 2011
//
// integrates sound features: tapeout, beeper and covox

module sound(

	input  wire       clk, q0,

	input  wire [7:0] din,


	input  wire       beeper_wr,
	input  wire       covox_wr,

	input  wire       beeper_mux, // output either tape_out or beeper


	output wire       sound_bit
);

	reg [6:0] ctr;
	reg [7:0] val;

	reg mx_beep_n_covox;

	reg beep_bit;
	reg beep_bit_old;

	wire covox_bit;




	always @(posedge clk) if (q0)
	begin
/*		if( beeper_wr ) */
                                if( beeper_wr && (beep_bit!=beep_bit_old) )
			mx_beep_n_covox <= 1'b1;
		else if( covox_wr )
			mx_beep_n_covox <= 1'b0;
	end

	always @(posedge clk) if (q0) if( beeper_wr ) beep_bit_old <= beep_bit;

	always @(posedge clk) if (q0)
	if( beeper_wr )
		beep_bit <= beeper_mux ? din[3] /*tapeout*/ : din[4] /*beeper*/;


	always @(posedge clk) if (q0)
	if( covox_wr )
		val <= din;

	always @(negedge clk) if (q0)
		ctr <= ctr + 6'd1;

	assign covox_bit = ( {ctr,clk} < val );


	bothedge trigger
	(
		.clk( clk ), .q0(q0),

		.d( mx_beep_n_covox ? beep_bit : covox_bit ),

		.q( sound_bit )
	);



endmodule




// both-edge trigger emulator
module bothedge(

	input  wire clk, q0,

	input  wire d,

	output wire q

);
	reg trgp, trgn;

	assign q = trgp ^ trgn;

	always @(posedge clk) if (q0)
	if( d!=q )
		trgp <= ~trgp;

	always @(negedge clk) if (q0)
	if( d!=q )
		trgn <= ~trgn;

endmodule

