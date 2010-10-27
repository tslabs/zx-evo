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
	
	output [7:0] sf_ra,
	input [7:0] sf_rd,
	output [7:0] sp_ra,
	input [5:0] sp_rd,
	
	output reg [5:0] spixel,
	output reg spx_en

	);

	reg [8:0] vline;
	reg l_sel;
	
	initial
	begin
//	sl_ra = 9'b111111111;
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
			sl_ra <= 9'b0;
		else if (pre_cend)
			sl_ra <= sl_ra + 9'b1;
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
	
	
	//processing sprite array
	parameter SpEn = 3'd0;
	parameter SpCR = 3'd0;
	parameter SpPal = 3'd0;
	parameter SpLoAddr = 3'd1;
	parameter SpMiAddr = 3'd2;
	parameter SpHiAddr = 3'd3;
	parameter SpLoX = 3'd4;
	parameter SpHiX = 3'd5;
	parameter SpXSz = 3'd5;
	parameter SpLoY = 3'd6;
	parameter SpHiY = 3'd7;
	parameter SpYSz = 3'd7;
	
	
	
	localparam VLINES = 9'd288;
	
	reg [5:0] sp_num;
	reg sp_word_rdy;
	reg [2:0] sp_mc;
	
	always @(posedge clk, posedge line_start)
	if (line_start)
	begin
		sp_num <= 6'd0;
		sp_word_rdy <= 1'b0;
		sp_mc <= 3'b0;
	
	
	
	
	
	
	end
	else
	begin
	
	
	
	
		sp_num <= 6'd0;
		sp_word_rdy <= 1'b0;
		sp_mc <= 3'b0;
	
	
	
	
	end
	
	
	
	
	//nulling of sprites address counters
	reg san_ws;
	reg [5:0] san_cnt;
	
	always @(posedge vblank, negedge clk)

	if (vblank)
	begin
		san_cnt <= 6'b0;
	end
	
	else if (!clk && !san_cnt[5])
	begin
		san_cnt <= san_cnt + 6'b1;
	end
	
	always @*
	begin
		san_ws <= (clk || !san_cnt[5]);
	end
	
	
	//incrementing sa_cnt
	reg [5:0] sac_cnt;
	reg [5:0] sac_wa;
	reg [13:0] sac_wd;
	reg sac_ws;
	
	
	
	
	
	//write to sa_cnt
	always @(posedge sac_ws, posedge san_ws)
	begin
		if (sac_ws)
		begin
			sa_ws <= sac_ws;
			sa_wa <= sac_wa[4:0];
			sa_wd <= sac_wd;
		end	
		else if (san_ws)
		begin
			sa_ws <= san_ws;
			sa_wa <= san_cnt[4:0];
			sa_wd <= 14'b0;
		end
		else
		begin
			sa_ws <= 1'b1;
		end
	end

	
	/*
	//1/25 frame color booster
	reg fld;
	always @(posedge vblank)
	begin
		fld <= !fld;
	end
	*/
	
	
	//Marlesonian ballet - part I
	

	
	
	always @(posedge clk)
	if (!spr_en)

	begin
	
	//read from sline
	case (l_sel)
	0:
	begin
		spixel <= sl_rd0[5:0];
		spx_en <= sl_rd0[6];
	end
	1:
	begin
		spixel <= sl_rd1[5:0];
		spx_en <= sl_rd1[6];
	end
	endcase
	
	
	//write to sline
	case (l_sel)
	0:
	begin
		sl_ws1 <= clk;
		sl_ws0 <= 1'b0;
	end
	1:
	begin
		sl_ws0 <= clk;
		sl_ws1 <= 1'b0;
	end
	endcase

	if (cend)
	begin
		sl_wa <= sl_ra;
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



	reg [8:0] sl_ra, sl_wa;
	reg [6:0] sl_wd0, sl_wd1;
	wire [6:0] sl_rd0, sl_rd1;

	reg [4:0] sa_ra, sa_wa;
	reg [13:0] sa_wd;
	wire [13:0] sa_rd;

	reg sl_ws0, sl_ws1, sa_ws;

	
	sline0 sline0(	.wraddress(sl_wa), .data(din), .wren(sl_ws0),
				.rdaddress(sl_ra), .q(sl_rd0)
			);

	sline1 sline1(	.wraddress(sl_wa), .data(din), .wren(sl_ws1),
				.rdaddress(sl_ra), .q(sl_rd1)
			);
			
	sacnt sacnt(	.wraddress(sa_wa), .data(sa_wd), .wren(sa_ws),
				.rdaddress(sa_ra), .q(sa_rd)
			);
			
endmodule
