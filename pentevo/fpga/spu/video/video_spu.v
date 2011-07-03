`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Sprite Processor
//
// Written by TS-Labs inc.
// ver. 1.3
//
// TV Horizontal Line Cycles - 448:
// Visible Area  - 360 * 288 pixels:
//
// 		|		 48	|
//	---------------------
// 	52	| 256		|  52
// 		|  			|
// 		|  		192	|
//	---------------------
//		|		 48	|

// to do
//
// - optimize usage of summators
// - use SFILE instead of SACNT
// - code Z80_EN and ZX_EN
// - make PRAM for use in ZX

// Optimization guidelines
//
// - no spu_req should be asserted at HBLANK
// - check spu_en and turn off SPU at VBLANK
// - tune spu_next on 16c and 4c modes


module spu(

	input 				clk,
	input 				spu_en,
	input 				line_start,
	input 				pre_vline,
	input 				cbeg, post_cbeg,
	input		[7:0]	din,

	output reg			test,
	output wire	[5:0]	mcd,

// spram/sfile
	output reg	[8:0]	spsf_ra,
	input		[7:0]	spsf_rd,
	
// dram
	output reg	[20:0]	spu_addr,
	input		[15:0]	spu_data,
	output reg			spu_req,
	input				spu_strobe,
	input				spu_next,

// video
	output reg	[5:0]	spixel,
	output reg			spx_en

	);

	reg [8:0] vline;

	wire l_sel;
	wire vvis;

	assign vvis = !(vline[8:5] == 4'b1001);			//lines numbered 288..319
	assign l_sel = vline[0];

//vcount
	always @(posedge clk)
	if (line_start)
	begin
		if (pre_vline)
			vline <= 9'b0;
		else
			vline <= vline + 9'b1;
	end

// read/null sline
	reg [8:0] sl_ra;
	reg sl_wsn;
	
	always @(posedge clk)
	begin

	if (line_start)
	begin
		sl_ra <= 9'b0;
		sl_wsn <= 1'b0;
	end
	else

	if (cbeg)
	begin
		spixel <= !l_sel ? sl_rd0[5:0] : sl_rd1[5:0];	//read pixel
		spx_en <= !l_sel ? sl_rd0[6] : sl_rd1[6];
		sl_wsn <= 1'b1;			//set nulling write strobe
	end

	if (post_cbeg)
	begin
		sl_wsn <= 1'b0;			//reset nulling write strobe
		sl_ra <= sl_ra + 9'b1;	//inc sl_ra
	end

	end	

	
// Sprites processing
	
	reg [5:0] num;		//number of currently processed sprite
	reg [2:0] sf_sa;	//sub-address for SFILE
	reg [1:0] cres;
	reg [7:0] xs;
	reg [7:0] yp;
	reg [8:0] lofs;
	reg [13:0] sdbuf;
	reg flipx, flipy;
	reg [2:0] pxn;
	reg [1:0] pri;

	assign mcd = ms;
	
// Marlezonskiy balet

	reg [3:0] ms;		//current state of FSM

	localparam ms_halt = 4'd0;
	localparam ms_beg = 4'd1;
	localparam ms_st1 = 4'd2;
	localparam ms_st2 = 4'd3;
	localparam ms_st3 = 4'd4;
	localparam ms_st4 = 4'd5;
	localparam ms_st5 = 4'd6;
	localparam ms_st6 = 4'd7;
	localparam ms_st7 = 4'd8;
	localparam ms_st8 = 4'd9;
	localparam ms_lbeg = 4'd10;
	localparam ms_4c1 = 4'd11;
	localparam ms_16c1 = 4'd12;
	localparam ms_tc1 = 4'd13;
	localparam ms_eow = 4'd14;
	
	localparam r_xp = 3'd0;
	localparam r_xs = 3'd1;
	localparam r_yp = 3'd2;
	localparam r_ys = 3'd3;
	localparam r_cr = 3'd4;
	localparam r_al = 3'd5;
	localparam r_am = 3'd6;
	localparam r_ah = 3'd7;

	assign		sf_ra = {num, sf_sa};
	wire		s_act = !(sf_rd[1:0] == 2'b0);
	wire [5:0]	s_next = num + 6'd1;
	wire		s_last = (s_next == 5'd0);
	wire [8:0]	ypc = {sf_rd[0], yp};
	wire [8:0]	ysc = {sf_rd[7:1], 2'b0};
	wire		s_vis = ((vline >= ypc) && (vline < (ypc + ysc)));
	wire [7:0]	dec_xs = xs - 8'd1;
	wire		s_eox = (dec_xs == 8'b0);
	wire [7:0]	xsc = (cres == 2'b11) ? {sf_rd[7:1], 1'b0} : {1'b0, sf_rd[7:1]};
	wire [8:0]	lofsc = flipy ? (ypc - vline + ysc - 9'd1) : (vline - ypc);
	wire [20:0]	adr_ofs = {sf_rd[7:0], spu_addr[12:0]} + (xs * lofs);
	wire [20:0]	adr_next = spu_addr + 21'b1;
	wire [8:0]	sl_next = flipx ? (sl_wa - 9'd1) : (sl_wa + 9'd1);
	wire [8:0]	xsf = (cres == 2'b11) ? (xs * 9'd2) : ((cres == 2'b10) ? (xs[6:0] * 9'd4) : (xs[5:0] * 9'd8));
	wire [8:0]	xc = flipx ? ({sl_wa[8], sf_rd[7:0]} + xsf - 9'd1) : {sl_wa[8], sf_rd[7:0]};
	

// Here the states are processed on CLK event
	always @(posedge clk)
	if (line_start)
	//SPU reset
	begin
		test <= 1'b1;
		spu_req <= 1'b0;
		sl_we <= 1'b0;
		num <= 6'd0;
		sf_sa <= r_cr;
		ms <= (spu_en && (vvis || pre_vline)) ? ms_beg : ms_halt;
	end
	else
	case (ms)
	
	ms_beg:	// Begin of sprite[num] processing
	begin
		//check if sprite is active
		if (s_act)
		begin
			cres <= sf_rd[1:0];			//get CRES[1:0]
			pri <= sf_rd[3:2];			//get PRI[1:0]
			sp_ra[7:4] <= sf_rd[7:4];	//get PAL[3:0]
			sp_ra[8] <= 1'b0;				//FIX ME
			sf_sa <= r_am;
			ms <= ms_st1;
		end
		else
		begin
			if (!s_last)				//check if all 32 sprites done
		//no: next sprite processing
			begin
			num <= s_next;
			sf_sa <= r_cr;
			ms <= ms_beg;
			end
			else
		//yes: halt
			ms <= ms_halt;
		end
	end

	ms_st1:		//fucking obsolete cycle, if sprite invisible!!!!!!!!
	begin
		spu_addr[12:7] <= sf_rd[5:0];	//get ADR[13:8]
		flipx <= sf_rd[6];				//get FLIPX
		flipy <= sf_rd[7];				//get FLIPY
		sf_sa <= r_yp;
		ms <= ms_st2;
	end
	
	ms_st2:
	begin
		yp <= sf_rd[7:0];			//get Y[7:0]
		sf_sa <= r_ys;
		ms <= ms_st3;
	end

	ms_st3:
	begin
		//check if sprite is visible on this line
		if (s_vis)	//get Y[8], YS[6:0]
		//yes
		begin
			lofs <= lofsc;		//set number of line within sprite (0-xxx), flipped is necessary
			sf_sa <= r_xs;
			ms <= ms_st4;
		end
		else
		//no
		begin
			if (!s_last)	//check if all 32 sprites done
		//no: next sprite processing
			begin
				num <= s_next;
				sf_sa <= r_cr;	//inc sprite num, set addr for reg4
				ms <= ms_beg;
			end
			else
		//yes: halt
			ms <= ms_halt;
		end
	end

	ms_st4: 
	begin
		sl_wa[8] <= sf_rd[0];		//get X[8]
		xs <= xsc;					//get XS[6:0]
		sf_sa <= r_al;
		ms <= ms_st5;
	end
		
	ms_st5:
	begin
		//read SPReg5
		spu_addr[6:0] <= sf_rd[7:1];		//get ADR[7:1]
		sf_sa <= r_ah;
		ms <= ms_st6;
	end

	ms_st6:
	begin
		//read SPReg6
		spu_addr <= adr_ofs;		// get ADR[20:13], ADR = ADR + LOFS * XS - now we get an addr for 1st word of sprite line
		spu_req <= 1'b1;			// now we can assert MRQ
		sf_sa <= r_xp;
		ms <= ms_st7;
	end

	ms_st7:
	begin
		//read SPReg7
		sl_wa[8:0] <= xc;		//get X[7:0], flipped if necessary
		ms <= ms_lbeg;
	end

	ms_lbeg: //begin of loop
	//wait for data from DRAM
	begin
	if (spu_next)
		spu_addr <= adr_next;	//set spu_addr
	if (!spu_strobe)
		ms <= ms_lbeg;
	else
	begin
//		spu_req <= 1'b0;
		sdbuf <= spu_data[13:0];
		sl_we <= 1'b1;
		pxn <= 3'b0;
		sp_ra[3:0] <= (cres[1] == 1'b0) ? {2'b0, spu_data[15:14]} : spu_data[15:12];		//set paladdr for pix0

		case (cres)
		1:	begin	//4c
				ms <= ms_4c1;
			end
	
		2:	begin	//16c
				ms <= ms_16c1;
			end
		
		3:	begin	//true color
				ms <= ms_tc1;
			end
		endcase
	end
	end

	ms_4c1:	//write pix1@4c
	begin
		sl_wa <= sl_next;
		sp_ra[1:0] <= {sdbuf[(13-pxn*2)], sdbuf[(12-pxn*2)]};		//set paladdr for pix1
		pxn <= pxn + 3'b1;
		ms <= (pxn == 3'd6) ? ms_eow : ms_4c1;
	end

	ms_16c1: //write pix1@16c
	begin
		sl_wa <= sl_next;
		sp_ra[3:0] <= {sdbuf[11-pxn*4], sdbuf[10-pxn*4], sdbuf[9-pxn*4], sdbuf[8-pxn*4]};		//set paladdr for pix1
		pxn <= pxn + 3'b1;
		ms <= (pxn == 3'd2) ? ms_eow : ms_16c1;
	end

	ms_tc1:	//write pix1@true
	begin
	if (!s_eox)
		begin
		end
		sl_wa <= sl_next;
		ms <= ms_eow;
	end
	
	ms_eow:	//end of write to sline
	begin
		sl_we <= 1'b0;
		sl_wa <= sl_next;

		//check if xs=0
		if (!s_eox)
		//no: go to begin of loop
		begin
		xs <= dec_xs;
		ms <= ms_lbeg;
		end
		else
		//yes:
		begin
		spu_req <= 1'b0;
		if (!s_last)		//check if all 32 sprites done
		//no: next sprite processing
		begin
			num <= s_next;
			sf_sa <= r_cr;	//inc spnum, set addr for reg4
			ms <= ms_beg;
		end
		else
		//yes: halt
		begin
			ms <= ms_halt;
		end
		end
	end
	
	ms_halt: //idle state
		begin
			test <= 1'b0;
			ms <= ms_halt;
			spu_req <= 1'b0;
			sl_we <= 1'b0;
		end

	default: //idle state
		begin
			ms <= ms_halt;
		end

	endcase


	reg sl_we;
	reg [8:0] sl_wa;
	wire [7:0] sl_rd0, sl_rd1;

	wire		 pixs = (ms == ms_tc1);
	wire		 pixt = pixs ? !spu_data[14] : !sdbuf[6];
	wire [6:0]	 pixc = pixs ? {!spu_data[15], spu_data[13:8]} : {!sdbuf[7], sdbuf[5:0]};
	wire		 tcol = (cres == 2'b11);
	wire		 sl_wss = (tcol ? pixt : !sp_rd[6]) && sl_we;
	wire [6:0]	 sl_wds = tcol ? pixc : {!sp_rd[7], sp_rd[5:0]};
	
	sline0 sline0(	.wraddress(l_sel ? sl_wa : sl_ra),
					.data(l_sel ? sl_wds : 8'b0),
					.wren(l_sel ? sl_wss : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd0),
					.clock(clk)
				);

	sline1 sline1(	.wraddress(!l_sel ? sl_wa : sl_ra),
					.data(!l_sel ? sl_wds : 8'b0),
					.wren(!l_sel ? sl_wss : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd1),
					.clock(clk)
				);
endmodule
