`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Sprite Processor
//
// Written by TS-Labs inc.
//
//
// TV Line Cycles - 448:
// Visible Area  - 360 * 288 pixels:
// 52 - border
// 256 - pixels
// 52 - border


module sprites(

	input clk, spr_en, hblank, vblank, cend, pre_cend, line_start,
	input pre_vline, s_reload,
	input [7:0] din,
	
	output reg [5:0] spixel,
	output reg spx_en

	);

	reg [8:0] vline;
	reg [8:0] r_adr, w_adr;
	reg [7:0] r_adrsf, w_adrsf;
	reg l_sel, w_stb0, w_stb1, w_stbsf;
	reg [6:0] w_dat0, w_dat1;
	reg [7:0] w_sf;
	wire [6:0] r_dat0, r_dat1;
	wire [7:0] r_sf;
	
	initial
	begin
//	r_adr = 9'b111111111;
	end
	
/*
	initial 
	begin 
		for (k = 0; k < 359 ; k = k + 1) 
		begin 
		mem[k] = 8'h40; 
		end 
	end
*/
	
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
	
	/*
	//For 1/25 color boost
	reg fld;
	always @(posedge vblank)
	begin
		fld <= !fld;
	end
	*/
	
	always @(posedge clk)
	if (!spr_en)

	begin

	//sline read
	case (l_sel)
	0:
	begin
		spixel <= r_dat0[5:0];
		spx_en <= r_dat0[6];
	end
	1:
	begin
		spixel <= r_dat1[5:0];
		spx_en <= r_dat1[6];
	end
	endcase
	
	//sline write
	
	case (l_sel)
	0:
	begin
		w_stb1 <= clk;
		w_stb0 <= 1'b0;
	end
	1:
	begin
		w_stb0 <= clk;
		w_stb1 <= 1'b0;
	end
	endcase

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


sline0 sline0(	.wraddress(w_adr), .data(din), .wren(w_stb0),
				.rdaddress(r_adr), .q(r_dat0)
			);

sline1 sline1(	.wraddress(w_adr), .data(din), .wren(w_stb1),
				.rdaddress(r_adr), .q(r_dat1)
			);
			
sfile sfile(	.wraddress(8'd0), .data(8'b1001100), .wren(1'b0),
				.rdaddress(8'd0), .q(r_sf)
			);
			
endmodule
