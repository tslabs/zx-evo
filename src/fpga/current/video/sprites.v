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

// to do
// 2. Code reads from DRAM
// 3. Fix jerking while write to sfile
// 4. Optimize SPU SM steps


module sprites(

	input clk, spr_en, line_start, pre_vline,
	input [7:0] din,
	
	output reg [7:0] sf_ra, sp_ra,
	input [7:0] sf_rd,
	input [5:0] sp_rd,
	
	output reg [5:0] spixel,
	output reg spx_en,
	output reg [20:0] sp_addr,
	input [15:0] sp_dat,
	output reg sp_mrq,
	input sp_drdy,
	
	output reg test,
	output reg [5:0] sp_mc
	);

	reg [8:0] vline;
	reg l_sel;
	
	
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

	
// read/null sline
	reg [1:0] rsst;
	reg [8:0] sl_ra;
	reg sl_wsn;
	
	always @(posedge clk)
//	if (!spr_en)
	begin

	if (line_start)
	begin
		sl_ra <= 9'b0;
		sl_wsn <= 1'b0;
		rsst <= 2'd0;
	end
	else

	case (rsst)

0:	//read pixel, set null strobe
	begin
		spixel <= !l_sel ? sl_rd0[5:0] : sl_rd1[5:0];
		spx_en <= !l_sel ? sl_rd0[6] : sl_rd1[6];
		sl_wsn <= 1'b1;
		rsst <= 2'd1;
	end

1:	//reset null strobe
	begin
		sl_wsn <= 1'b0;
		rsst <= 2'd2;
	end

2:	//inc sl_ra
	begin
		sl_ra <= sl_ra + 9'b1;
		rsst <= 2'd3;
	end

3:	//dummy, go to step 0
		rsst <= 2'd0;

	endcase
	end	

	
// Marlezonskiy balet, part II
// Sprites processing

	localparam VLINES = 9'd288;
	
	reg [4:0] sp_num;	//number of currently processed sprite
//	reg [5:0] sp_mc;	//current state of state-machine
	reg [13:0] sa_offs; //offset from sprite address, words
	reg [8:0] sl_wa;
	reg [6:0] sl_wd;
	reg sl_ws, sl_we;

// sfile
	reg sp_act;
	reg sp_cres;
	reg [20:0] sp_adr;	//word address!!! 2MB x 16bit
	reg [8:0] sp_ypos;
	reg [6:0] sp_xsz;
	reg [6:0] sp_ysz;


// *** DIE MASCHINE *** !!!
	
	always @*
	begin
		if (!clk)
			sl_ws <= sl_we;
		else
			sl_ws <= 1'b0;
	end

	always @(posedge clk)

	if (line_start)
		sp_mc <= 6'd0;
	else
	case (sp_mc)

	0:	// SPU reset
		//set sprite number to 0
	begin
		test <= 1'b1;
		sp_num <= 5'd0;
		sa_ws <= 1'b0;
		sp_mrq <= 1'b0;
		sl_we <= 1'b0;
		sp_mc <= 6'd1;
	end

	1:	// Begin of sprite[sp_num] processing
		//addr for SPReg0
	begin
		sf_ra <= {sp_num, 3'd0};
		sp_mc <= 6'd2;		
	end

	2: //read SPReg0, set addr for SPReg6
	begin
		sp_act <= sf_rd[7];
		sp_cres <= sf_rd[6];
		sp_ra[7:2] <= sf_rd[5:0];
		sf_ra <= {sp_num, 3'd6};
		sp_mc <= 6'd3;
	end

	3: //check if sprite is active
	if (sp_act)
		//yes: read SPReg6, set addr for SPReg7
	begin
		sp_ypos[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num, 3'd7};
		sp_mc <= 6'd4;
	end
	else
		//no: go to next sprite processing (step 25)
	begin
		sp_mc <= 6'd25;
	end

	4: //read SPReg7, set addr for SPReg1
	begin
		sp_ypos[8] <= sf_rd[0];
		sp_ysz[6:0] <= sf_rd[7:1];
		sf_ra <= {sp_num, 3'd1};
		sp_mc <= 6'd5;
	end

	5: //check if sprite is visible on this line
	if ((vline >= sp_ypos) && (vline < (sp_ypos + sp_ysz)))
		//yes: read SPReg1, set addr for SPReg2
	begin
		sp_adr[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num, 3'd2};
		sp_mc <= 6'd6;		
	end
	else
		//no: go to next sprite processing (step 25)
	begin
		sp_mc <= 6'd25;
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
		sf_ra <= {sp_num, 3'd3};
		sp_mc <= 6'd7;
	end

	7: //read SPReg3, set addr for SPReg4, end of sa_ws
	begin
		sp_adr[20:16] <= sf_rd[4:0];
		sf_ra <= {sp_num, 3'd4};
		sa_ws <= 1'b0;
		sp_mc <= 6'd8;
	end

	8: //read SPReg4, set addr for SPReg5, read sa_offs
	begin
		sl_wa[7:0] <= sf_rd[7:0];
		sf_ra <= {sp_num, 3'd5};
		sa_offs <= sa_rd;
		sp_mc <= 6'd9;
	end

	9: //read SPReg5, decide which (16c/4c) color mode to use
	begin
		sl_wa[8] <= sf_rd[0];
		sp_xsz[6:0] <= sf_rd[7:1];
	if (!sp_cres)
		sp_mc <= 6'd12;
	else
		sp_mc <= 6'd25;
	end
	
	12: // Begin of horizontal loop for 16c
		//set sp_addr, assert sp_mrq
	begin
		sp_addr <= (sp_adr + sa_offs);
		sp_mrq <= 1'b1;
		sp_mc <= 6'd14;
	end

	14: //wait for data from DRAM
		//write pixel-1
	if (sp_drdy)
	begin
		sp_mrq <= 1'b0;
 		sp_ra[3:0] <= sp_dat[15:12];
		sl_wd = {!(sp_dat[15:12] == 4'b0), sp_rd[5:0]};
		sl_we <= 1'b1;
		sp_mc <= 6'd16;
	end

	16: //write pixel-2
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sp_dat[11:8];
		sl_wd = {!(sp_dat[11:8] == 4'b0), sp_rd[5:0]};
		sp_mc <= 6'd18;
	end

	18: //write pixel-3
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sp_dat[7:4];
		sl_wd = {!(sp_dat[7:4] == 4'b0), sp_rd[5:0]};
		sp_mc <= 6'd20;
	end

	20: //write pixel-4
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sp_dat[3:0];
		sl_wd = {!(sp_dat[3:0] == 4'b0), sp_rd[5:0]};
		sp_mc <= 6'd21;
	end

	21:	//end of writes to sline, inc sa_offs, dec sp_xsz
	begin
		sl_wa <= sl_wa + 9'b1;
		sl_we <= 1'b0;
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
		//no: go to begin of 16c loop (step 12)
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
		begin
			test <= 1'b0;
			sp_mc <= 6'd63;
		end
		else
		//no: go to begin of sprite processing (step 1)
			sp_mc <= 6'd1;
	end

	endcase
	


/*
// 1/25 frame color booster
	reg fld;
	always @(posedge vblank)
	begin
		fld <= !fld;
	end
	*/


	wire [6:0] sl_rd0, sl_rd1;
	reg sl_ws0, sl_ws1;

	sline0 sline0(	.wraddress(l_sel ? sl_wa : sl_ra),
					.data(l_sel ? sl_wd : 7'b0),
					.wren(l_sel ? sl_ws : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd0)
				);

	sline1 sline1(	.wraddress(!l_sel ? sl_wa : sl_ra),
					.data(!l_sel ? sl_wd : 7'b0),
					.wren(!l_sel ? sl_ws : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd1)
				);

				
	wire [13:0] sa_rd;
	reg [13:0] sa_wd;
	reg sa_ws;

	sacnt sacnt(	.wraddress(sp_num),
					.data(sa_wd),
					.wren(sa_ws),
					.rdaddress(sp_num),
					.q(sa_rd)
				);
			
endmodule
