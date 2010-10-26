`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
//Sprite Processor
//
//Written by TS-Labs inc.
//
//
//TV Line Cycles - 448:
//Visible Area  - 360 * 288 pixels:
//52 - border
//256 - pixels
//52 - border


module sprites(

	input clk, spr_en, hblank, vblank, cend, pre_cend, line_start,
	input pre_vline, s_reload,
	
	output reg [5:0] spixel,
	output reg spx_en

	);

	reg [8:0] vline;
	reg [8:0] r_adr, w_adr;
	reg l_sel, w_stb0, w_stb1;
	reg [6:0] w_dat0, w_dat1;
	wire [6:0] r_dat0, r_dat1;

	initial
	begin
//	r_adr = 9'b111111111;
	end
	
	//hcount
	always @(posedge clk, posedge line_start)
	begin
		if (line_start)
			r_adr <= 9'b0;
		else if (pre_cend)
			r_adr <= r_adr + 9'b1;
	end
	
	//vcount
	always @(posedge line_start)
	begin
		if (pre_vline)
		begin
			vline <= 9'b0;
			l_sel <= 1'b0;
		end
		else
		begin
			vline <= vline + 9'b1;
			l_sel <= ~l_sel;
		end
	end
	
	always @(posedge clk)
	if (!spr_en)

	begin

	//sline read
		if (l_sel)
		begin
			spixel <= r_dat0[5:0];
			spx_en <= r_dat0[6];
		end
		
		else
		begin
			spixel <= r_dat1[5:0];
			spx_en <= r_dat1[6];
		end
	
	//sline write
	
	if (!l_sel)
		w_stb0 <= clk;
	else
		w_stb1 <= clk;

	if (cend)
	begin
		w_adr <= r_adr;
	end
	
	//sprites file reload
/*
		if  (s_reload)
		begin
//			spx_en <= 1;
//			spixel <= sl_adr[5:0];
		end
		
	//begin of process
		else if (pre_vline)
		begin
//			spx_en <= 1;
//			spixel <= sl_adr[7:2];
		end
		
		else
			spx_en <= 0;
*/
			end
	
//	else spx_en <= 1'b0;


sline0 sline0(	.wraddress(9'd52), .data(7'b1110000), .wren(w_stb0),
				.rdaddress(r_adr), .q(r_dat0)
			);

sline1 sline1(	.wraddress(9'd0), .data(7'b1001100), .wren(w_stb1),
				.rdaddress(r_adr), .q(r_dat1)
			);
			
endmodule
