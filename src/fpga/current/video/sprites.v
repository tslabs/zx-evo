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

	input clk, spr_en, hblank, vblank, cend, pre_cend, line_start, pre_vline,
	input [7:0] din,
	
	output reg [7:0] sf_ra, sp_ra,
	input [7:0] sf_rd,
	input [5:0] sp_rd,
	
	output reg [5:0] spixel,
	output reg spx_en,
	output reg [20:0] sp_addr,
	input [15:0] sp_dat,
	output reg sp_mrq,
	input sp_drdy
	);

	reg [8:0] vline;
	reg l_sel, sl_ws;
	
	
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

	
//read from sline
	always @(posedge clk)
	if (!spr_en)
	begin
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
	end	

	
	
// Marlezonskiy balet, part II
// *** DIE MASCHINE *** !!!

	localparam VLINES = 9'd288;
	
	reg [4:0] sp_num;	//number of currently processed sprite
	reg [7:0] sp_mc;	//current state of state-machine
	reg [13:0] sa_offs; //offset from sprite address, words

// sfile
	reg sp_act;
	reg sp_cres;
	reg [20:0] sp_adr;	//word address!!! 2MB x 16bit
	reg [8:0] sp_ypos;
	reg [6:0] sp_xsz;
	reg [6:0] sp_ysz;


// Sprites processing
	always @(posedge clk, posedge line_start)
	if (line_start)
		sp_mc <= 6'b0;
	else
	
	begin
	case (sp_mc)

	0:	// SPU reset
		//set sprite number to 0
	begin
		sp_num <= 5'd0;
		sa_ws <= 1'b0;
		sp_mrq <= 1'b0;
		sp_mc <= 6'd1;
		sl_ws0 <= 1'b0;
		sl_ws1 <= 1'b0;
	end

	1:	// Begin of sprite[sp_num] processing
		//set sa_ra, sa_wa, addr for SPReg0
	begin
		sa_ra <= sp_num;
		sa_wa <= sp_num;
		sf_ra <= {sp_num[4:0], 3'd0};
		sp_mc <= 6'd2;		
	end

	2: //read SPReg0, set addr for SPReg6
	begin
		sp_act <= sf_rd[7];
		sp_cres <= sf_rd[6];
		sp_ra[7:2] <= sf_rd[5:0];
		sf_ra <= {sp_num[4:0], 3'd6};
		sp_mc <= 6'd3;
	end

	3: //check if sprite is active
	if (sp_act)
		//yes: read SPReg6, set addr for SPReg7
	begin
		sp_ypos[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num[4:0], 3'd7};
		sp_mc <= 6'd4;
	end
	else
		//no: go to next sprite processing (step ??)
	begin
		sp_mc <= 6'd??;
	end

	4: //read SPReg7, set addr for SPReg1
	begin
		sp_ypos[8] <= sf_rd[0];
		sp_ysz[6:0] <= sf_rd[7:1];
		sf_ra <= {sp_num[4:0], 3'd1};
		sp_mc <= 6'd5;
	end

	5: //check if sprite is visible on this line
	if ((vline >= sp_ypos) && (vline < (sp_ypos + sp_ysz)))
		//yes: read SPReg1, set addr for SPReg2
	begin
		sp_adr[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num[4:0], 3'd2};
		sp_mc <= 6'd6;		
	end
	else
		//no: go to next sprite processing (step ??)
	begin
		sp_mc <= 6'd??;
	end

	6: //read SPReg2, set addr for SPReg3, null sa_offs
	begin
		//check if 1st line of sprite
		if (sp_ypos == vline)
		//yes: null sa_offs[sp_num]
		begin
			sa_wd <= 14'b0;
			sa_ws <= 1'b1;
		end
		sp_adr[15:8] <= sf_rd[7:0];
		sf_ra <= {sp_num[4:0], 3'd3};
		sp_mc <= 6'd7;
	end

	7: //read SPReg3, set addr for SPReg4, end of sa_ws
	begin
		sp_adr[20:16] <= sf_rd[4:0];
		sf_ra <= {sp_num[4:0], 3'd4};
		sa_ws <= 1'b0;
		sp_mc <= 6'd8;
	end

	8: //read SPReg4, set addr for SPReg5, read sa_offs
	begin
		sl_wa[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num[4:0], 3'd5};
		sa_offs <= sa_rd;
		sp_mc <= 6'd9;
	end

	9: //read SPReg5, decide which (16c/4c) color mode to use
	begin
		sl_wa[8] <= sf_rd[0];
		sp_xsz[6:0] <= sf_rd[7:1];
	if (sp_cres)
		sp_mc <= 6'd12;
	else
		sp_mc <= 6'd13;
	end
	
	12: // Begin of horizontal loop for 16c
		//set sp_addr, assert sp_mrq
	begin
		sp_addr <= (sp_adr + sa_offs);
		sp_mrq <= 1'b1;
		sp_mc <= 6'd14;
	end

	14: //wait for data from DRAM
		//set addr for SPRAM of pixel-1
	if (sp_drdy)
	begin
		sp_ra[3:0] <= sp_dat[15:12];
		sp_mc <= 6'd16;
	end

	16: //write pixel-1
	begin
		if (l_sel)
		begin
			sl_wd1 <= {!(sp_dat[15:12] == 4'b0), sp_rd[5:0]};
			sl_ws1 <= 1'b1;
		end
		else
		begin
			sl_wd0 <= {!(sp_dat[15:12] == 4'b0), sp_rd[5:0]};
			sl_ws0 <= 1'b1;
		end
		sp_mc <= 6'd17;
	end

	17:	//set addr for SPRAM of pixel-2, inc sl_wa
	begin
		sl_ws0 <= 1'b0;
		sl_ws1 <= 1'b0;
		sp_ra[3:0] <= sp_dat[11:8];
		sl_wa <= sl_wa + 9'b1;
		sp_mc <= 6'd18;
	end

	18: //write pixel-2
	begin
		if (l_sel)
		begin
			sl_wd1 <= {!(sp_dat[11:8] == 4'b0), sp_rd[5:0]};
			sl_ws1 <= 1'b1;
		end
		else
		begin
			sl_wd0 <= {!(sp_dat[11:8] == 4'b0), sp_rd[5:0]};
			sl_ws0 <= 1'b1;
		end
		sp_mc <= 6'd19;
	end

	19:	//set addr for SPRAM of pixel-3, inc sl_wa
	begin
		sl_ws0 <= 1'b0;
		sl_ws1 <= 1'b0;
		sp_ra[3:0] <= sp_dat[7:4];
		sl_wa <= sl_wa + 9'b1;
		sp_mc <= 6'd20;
	end

	20: //write pixel-3
	begin
		if (l_sel)
		begin
			sl_wd1 <= {!(sp_dat[7:4] == 4'b0), sp_rd[5:0]};
			sl_ws1 <= 1'b1;
		end
		else
		begin
			sl_wd0 <= {!(sp_dat[7:4] == 4'b0), sp_rd[5:0]};
			sl_ws0 <= 1'b1;
		end
		sp_mc <= 6'd21;
	end

	21:	//set addr for SPRAM of pixel-4, inc sl_wa
	begin
		sl_ws0 <= 1'b0;
		sl_ws1 <= 1'b0;
		sp_ra[3:0] <= sp_dat[3:0];
		sl_wa <= sl_wa + 9'b1;
		sp_mc <= 6'd22;
	end

	22: //write pixel-4
	begin
		if (l_sel)
		begin
			sl_wd1 <= {!(sp_dat[3:0] == 4'b0), sp_rd[5:0]};
			sl_ws1 <= 1'b1;
		end
		else
		begin
			sl_wd0 <= {!(sp_dat[3:0] == 4'b0), sp_rd[5:0]};
			sl_ws0 <= 1'b1;
		end
		sp_mc <= 6'd23;
	end

	23:	//inc sl_wa, inc sa_offs, dec sp_xsz
	begin
		sl_ws0 <= 1'b0;
		sl_ws1 <= 1'b0;
		sl_wa <= sl_wa + 9'b1;
		sa_offs <= sa_offs + 14'b1;
		sp_xsz <= sp_xsz - 7'b1;
		sp_mc <= 6'd24;
	end
	
	24:	// End of horizonal loop 16c
	begin
		//check if xsz=0
		if (sp_xsz == 7'b0)
		//yes: write sa_offs
		begin
			sa_wd <= sa_offs;
			sa_ws <= 1'b1;
			sp_mc <= 6'd25;
		end
		else
		//no: go to begin of 16c loop
		begin
			sp_mc <= 6'd12;
		end
	end
	
	25:	//inc sp_num
	begin
		sa_ws <= 1'b0;
		sp_num <= sp_num + 5'b1;
		sp_mc <= 6'd26;
	end

	26:	//check if sp_num=0
	begin
		if (sp_num == 5'b0)
		//yes: halt
			sp_mc <= 6'd63;
		else
		//no: go to begin of sprite processing
			sp_mc <= 6'd26;
	end

	endcase
	end
	


/*
// 1/25 frame color booster
	reg fld;
	always @(posedge vblank)
	begin
		fld <= !fld;
	end
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
