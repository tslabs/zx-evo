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
// 		52 - border
// 		256 - pixels
// 		52 - border


module sprites(

	input clk, spr_en, hblank, vblank, cend, pre_cend, line_start,
	input pre_vline, s_reload,
	input [7:0] din,
	
	output reg [7:0] sf_ra, sp_ra,
	input [7:0] sf_rd,
	input [5:0] sp_rd,
	
	output reg [5:0] spixel,
	output reg spx_en

	);

	reg [8:0] vline;
	reg l_sel, sl_ws;
	
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
	
	
	// processing of the sprite array
		
	localparam VLINES = 9'd288;
	
	reg [5:0] sp_num;
	reg [7:0] sp_mc;
	reg sac_ws, san_ws;
	reg [7:0] sac_wd;
	reg [13:0] sa_cnt;

// sfile
	reg sp_act;
	reg sp_cres;
	reg [5:0] sp_pal;
	reg [20:0] sp_addr;		//word address!!! 2MB x 16bit
	reg [8:0] sp_xpos;
	reg [8:0] sp_ypos;
	reg [6:0] sp_xsz;
	reg [6:0] sp_ysz;

	
// Marlezonskiy balet, part I
// *** DIE MASCHINE *** !!!

	always @(posedge clk, posedge line_start)
	if (line_start)
		sp_mc <= 8'b0;
	else
	
	begin
	case (sp_mc)

	0:	//reset SPU, set sprite number to 0
	begin
		sp_num <= 6'd0;
	end

	1:	//set addr for SPReg0
	begin
		sf_ra <= {sp_num[4:0], 3'd0};
	end

	2: //read SPReg0, set addr for SPReg1
	begin
		sp_act <= sf_rd[7];
		sp_cres <= sf_rd[6];
		sp_pal <= sf_rd[5:0];
		sf_ra <= {sp_num[4:0], 3'd1};
	end

	3: //read SPReg1, set addr for SPReg2
	begin
		sp_addr[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num[4:0], 3'd2};
	end

	4: //read SPReg2, set addr for SPReg3
	begin
		sp_addr[15:8] <= sf_rd[7:0];
		sf_ra <= {sp_num[4:0], 3'd3};
	end

	5: //read SPReg3, set addr for SPReg4
	begin
		sp_addr[20:16] <= sf_rd[4:0];
		sf_ra <= {sp_num[4:0], 3'd4};
	end

	6: //read SPReg4, set addr for SPReg5
	begin
		sp_xpos[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num[4:0], 3'd5};
	end

	7: //read SPReg5, set addr for SPReg6
	begin
		sp_xpos[8] <= sf_rd[0];
		sp_xsz[6:0] <= sf_rd[7:1];
		sf_ra <= {sp_num[4:0], 3'd6};
	end

	8: //read SPReg6, set addr for SPReg7
	begin
		sp_ypos[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num[4:0], 3'd7};
	end

	9: //read SPReg7, reset sa_cnt to 0
	begin
		sp_ypos[8] <= sf_rd[0];
		sp_ysz[6:0] <= sf_rd[7:1];
	end


	
	
//	default:		//halt
	endcase
	
	if (!sp_mc[7])
	begin
		sp_mc <= sp_mc + 8'b1;
	end
	end
/*	
	
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
	
	
	
	
	

	//1/25 frame color booster
	reg fld;
	always @(posedge vblank)
	begin
		fld <= !fld;
	end
	*/
	
	
// Marlezonskiy balet, part II	

//read from sline
	always @(posedge clk)
//	if (!spr_en)
//	begin
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
//	end	

	
//write to sline
	always @(posedge sl_ws)
	case (l_sel)
	0:
	begin
		sl_ws1 <= sl_ws;
		sl_ws0 <= 1'b0;
	end
	1:
	begin
		sl_ws0 <= sl_ws;
		sl_ws1 <= 1'b0;
	end
	endcase

//write to sacnt
	always @(posedge sac_ws, posedge san_ws)
	begin
		if (sac_ws)
		begin
			sa_ws <= sac_ws;
			sa_wa <= sp_num[4:0];
			sa_wd <= sa_cnt;
		end	
		else if (san_ws)
		begin
			sa_ws <= san_ws;
			sa_wa <= sp_num[4:0];
			sa_wd <= 14'b0;
		end
		else
		begin
			sa_ws <= 1'b1;
		end
	end

	
	
	//	if (cend)
//	begin
//		sl_wa <= sl_ra;
//	end

//	end

//	else spx_en <= 1'b0;



/*
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
*/
			



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
