`include "../include/tune.v"

// Pentevo project (c) NedoPC 2010,2011,2012
//
// decoding mode setup: which border, which modes in one-hot style coding

module video_modedecode(

	input  wire        clk,

	input  wire [ 1:0] pent_vmode, // inputs as set by Z80 environment
	input  wire [ 2:0] atm_vmode,  //


	output reg         mode_atm_n_pent, // =1 - atm modes, =0 - pentagon modes (mainly for border and visible area changing)


	output reg         mode_zx, // standard ZX mode

	output reg         mode_p_16c,   // pentagon 16 colors
	output reg         mode_p_hmclr, // pentagon hardware multicolor


	output reg         mode_a_hmclr, // 640x200 atm hardware multicolor
	output reg         mode_a_16c,   // 320x200 atm 16 colors
	output reg         mode_a_text,  // 640x200 (80x25 symbols) atm text mode
	output reg         mode_a_txt_1page, // atm text mode in a single page (modifier for mode_a_text)

	output reg         mode_pixf_14, // =1: 14MHz pixelclock on (default is 7MHz).


	output reg  [ 1:0] mode_bw // required bandwidth: 2'b00 - 1/8, 2'b01 - 1/4,
	                           //                     2'b10 - 1/2, 2'b11 - 1
);

// values for pent_vmode and atm_vmode:

// pent:
// 2'b00 - standard ZX
// 2'b01 - hardware multicolor
// 2'b10 - pentagon 16 colors
// 2'b11 - not defined yet

// atm:
// 3'b011 - zx modes (pent_vmode is active)
// 3'b010 - 640x200 hardware multicolor
// 3'b000 - 320x200 16 colors
// 3'b110 - 80x25 text mode
// 3'b111 - 80x25 text mode (single page)
// 3'b??? (others) - not defined yet




	always @(posedge clk)
	begin
		case( atm_vmode )
			3'b010:  mode_atm_n_pent <= 1'b1;
			3'b000:  mode_atm_n_pent <= 1'b1;
			3'b110:  mode_atm_n_pent <= 1'b1;
			3'b111:  mode_atm_n_pent <= 1'b1;

			3'b011:  mode_atm_n_pent <= 1'b0;
			default: mode_atm_n_pent <= 1'b0;
		endcase


		case( atm_vmode )
			3'b010: mode_zx <= 1'b0;
			3'b000: mode_zx <= 1'b0;
			3'b110: mode_zx <= 1'b0;
			3'b111: mode_zx <= 1'b0;

			default: begin
				if( (pent_vmode==2'b00) || (pent_vmode==2'b11) )
					mode_zx <= 1'b1;
				else
					mode_zx <= 1'b0;
			end
		endcase


		
		if( (atm_vmode==3'b011) && (pent_vmode==2'b10) )
			mode_p_16c <= 1'b1;
		else
			mode_p_16c <= 1'b0;


		if( (atm_vmode==3'b011) && (pent_vmode==2'b01) )
			mode_p_hmclr <= 1'b1;
		else
			mode_p_hmclr <= 1'b0;


		if( atm_vmode==3'b010 )
			mode_a_hmclr <= 1'b1;
		else
			mode_a_hmclr <= 1'b0;
		

		if( atm_vmode==3'b000 )
			mode_a_16c <= 1'b1;
		else
			mode_a_16c <= 1'b0;
		

		if( (atm_vmode==3'b110) || (atm_vmode==3'b111) )
			mode_a_text <= 1'b1;
		else
			mode_a_text <= 1'b0;
		
		if( atm_vmode==3'b111 )
			mode_a_txt_1page <= 1'b1;
		else
			mode_a_txt_1page <= 1'b0;



		if( (atm_vmode==3'b010) || (atm_vmode==3'b110) || (atm_vmode==3'b111) )
			mode_pixf_14 <= 1'b1;
		else
			mode_pixf_14 <= 1'b0;



		if( (atm_vmode==3'b011) && (pent_vmode!=2'b10) )
			mode_bw <= 2'b00; // 1/8
		else
			mode_bw <= 2'b01; // 1/4


	end

endmodule

