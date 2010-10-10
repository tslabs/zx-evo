`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
//Sprite Processor
//
//Written by TS-Labs inc.
//
//
//TV Line Cycles - 448:
//Visible Area  - 360 pixels:
//52 - border
//256 - pixels
//52 - border


module sprites(

	input clk, spr_en, hblank, vblank, cend, line_start,
	input pre_vline, s_reload,
	
	output reg [5:0] spixel,
	output reg spx_en

	);

//	reg [8:0] sl_adr;
	reg [8:0] vline;
//	reg l_sel;

sline sline(	.clk(clk), 
				.w_sel(w_sel), .w_adr(w_adr), .w_dat(w_dat), .w_stb(w_stb),
				.r_sel(r_sel), .r_adr(r_adr), .r_dat(r_dat)
			);

		
	//hcount
	always @(posedge clk) if (cend)
	begin
	if (line_start)
	r_adr <= 9'b0;
	else
	r_adr <= r_adr + 9'b1;
	end
	
	//vcount
	always @(posedge clk) if (line_start)
	begin
	if (pre_vline)
	begin
		vline <= 9'b0;
		r_sel <= 1'b0;
		w_sel <= 1'b1;
	end
	else
	begin
		vline <= vline + 9'b1;
		r_sel <= ~r_sel;
		w_sel <= ~w_sel;
	end
	end
	
	always @(posedge clk)
	if (!spr_en)

	begin

	//sline read
	if (cend)
	begin
		spixel <= r_dat[5:0];
		spx_en <= r_dat[6];
	end
	
	//sline write
	
	wr_stb <= ~wr_stb;

	if (cend)
	begin
		w_adr <= r_adr;
		w_adr[7:2] => w_dat[5:0];
		w_adr[8] => w_dat[6];
	end
	
	//sprites file reload
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
	end
	
//	else spx_en <= 1'b0;

endmodule


module sline(

	input  wire        clk,

	input  wire w_sel,
	input  wire [8:0] w_adr,
	input  wire [6:0] w_dat,
	input  wire w_stb,

	input  wire r_sel,
	input  wire [8:0] r_adr,
	output reg  [6:0] r_dat
);

	reg [6:0] sln0 [0:359];
	reg [6:0] sln1 [0:359];

	always @(posedge clk)
	begin
		if (w_stb)
		begin
			if (!w_sel)
			sln0[w_adr] <= w_dat;
			else
			sln1[w_adr] <= w_dat;
		end

		if (!r_sel)
			r_dat <= sln0[r_adr];
		else
			r_dat <= sln1[r_adr];
	end
endmodule
