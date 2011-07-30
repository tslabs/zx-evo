`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// Sprite Processor
//
// Written by TS-Labs inc.
// ver. 1.4
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


module spu(

	input 				clk,
	input 				spu_en,
	input 				pre_hline,
	input 				pre_vline,
	input 				cbeg, post_cbeg,
	input		[7:0]	din,

// DEBUG!!!
	output 				test,			// DEBUG!!!
	output 		[5:0]	ms,				// DEBUG!!!

// spram/sfile
	output 		[8:0]	spsf_ra,
	input		[7:0]	spsf_rd,
	
// dram
	output reg	[20:0]	spu_addr,
	output reg			spu_req,
	input		[15:0]	rd,
	input				dram_rd_valid,
	input				spu_strobe,
	input				spu_next,
	input				spu_cycle,

// video
	output reg	[5:0]	spixel,
	output reg	[1:0]	spixel_pri,
	output reg			spixel_en

	);


	assign test = ms_halt ? 1'b0 : 1'b1;		// DEBUG!!!
	
	assign spsf_ra = {pal_n_file, pal_n_file ? {spal, spix} : {snum, sreg}};	// address for SPSF
																				// $000-$0FF:	32 sprite descriptors 8 bytes each
																				// $100-$1FF:	16 sprite pallettes 16 bytes each


//					|       blank       |       blank       |      video0       |      video1       |      video2       |
//					|  1 |  2 |  3 |  4 |  1 |  2 |  3 |  4 |  1 |  2 |  3 |  4 |  1 |  2 |  3 |  4 |  1 |  2 |  3 |  4 |
// clk				|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_/|`\_ |`\_/|
// cbeg				|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|___ |/```|
// post_cbeg		|/```|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|___ |____|
// pre_cend     	|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\__ |____|
// cend         	|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|____|/`` |\___|
// pre_hline		|____|____|____|/```|\___|____|____|____|____|____|____|____|____|____|____|____|____|____|___ |____|
// sline_ctr		|xxxx|xxxx|xxxx|xxxx|0000|0000|0000|0000|1111|1111|1111|1111|2222|2222|2222|2222|3333|3333|3333|3333|
// pix|en select	|pppp|eeee|pppp|pppp|pppp|eeee|pppp|pppp|pppp|eeee|pppp|pppp|pppp|eeee|pppp|pppp|pppp|eeee|pppp|pppp|
// sline_raddr_ram	|----|----|----|----|----|/px0|/en0|/px0|/px0|/px1|/en1|/px1|/px1|/px2|/en2|/px2|/px2|/px3|/en3|/px3|
// read spixel		|----|----|----|----|----|----|/px0|----|----|----|/px1|----|----|----|/px2|----|----|----|/px3|----|
// read spixel_en	|----|----|----|----|----|----|----|/en0|----|----|----|/en1|----|----|----|/en2|----|----|----|/en3|
// sline_nul_ws		|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|____|/```|\___|____|___ |/```|

// Pixel addresses are 0-359 (360 bytes)
// Transparency bits addresses are 384-428 (360/8 = 45 bytes)
// Both are nulled after reading since hcounter runs as 0-447

// Bitmap address (bytes) is calculated as:
// 16c mode:	(256 bytes * 512 lines = 128kB = 8 pages)
// pppppp yyyyyyy xxxxxxx0 (xx)
// 64c mode:	(512 bytes * 512 lines = 256kB = 16 pages)
// pppppp yyyyyyy xxxxxxx0 (x)
// x = XPOSn, y = YPOSn, p = PBMPn, v = PBMPn + YPOSn

// l_sel controls which buffer is active:
// 0th - is displayed and nulled
// 1st - is rendered


	wire vvis = (vline[8:0] < 9'd288);				// lines >= 288 are invisible
	wire hvis = (sline_ctr[8:0] < 9'd360);			// pixels >= 360 are invisible
	// wire vvis = (vline[8:5] < 4'd9);					// lines >= 288 are invisible
	// wire hvis = (sline_ctr[8:3] < 6'd45);			// pixels >= 360 are invisible


//	vcount ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	reg [8:0] vline;
	wire l_sel = vline[0];							// the active buffer selection
	
	always @(posedge clk)
	if (pre_hline)
		if (pre_vline)
			vline <= 9'b0;
		else
			vline <= vline + 9'b1;


//	hcount ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	reg [8:0] sline_ctr;
	
	always @(posedge clk)
	if (pre_hline)
		sline_ctr <= 9'b0;
	else if (cbeg)
		sline_ctr <= sline_ctr + 9'b1;

	
//	Reading and nulling sline -----------------------------------------------------------------------------------------------------------------------------------------------------------------------
	wire [8:0] sline_raddr = pre_cend ? sline_tsp : sline_pix;
	wire sline_pix = sline_ctr;
	wire sline_tsp = {3'b110, sline_ctr[8:3]};										// addr for transparency bit = 384 + ctr/8
	wire sline_nul_ws = cbeg;														// nulling strobe
	
	always @(posedge clk)
	begin
		if (pre_cend)
		begin
			spixel <= !l_sel ? sl_rd0[5:0] : sl_rd1[5:0];							// pixel
			spixel_pri <= !l_sel ? sl_rd0[7:6] : sl_rd1[7:6];						// pixel priority
		end
	
		if (cend)
			spixel_en <= !l_sel ? sl_rd0[sline_ctr[2:0]] : sl_rd1[sline_ctr[2:0]];	// pixel transparency
	end	

	
//	Memory fetcher ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	// fetch address/counter reload and increment
	reg [8:0] fetch_cnt;
	wire spu_next = spu_cycle && pre_cend;

	always @(posedge clk)
	if (pre_hline)
		fetch_cnt <= 9'd0;
		
	else if (fetch_go)
	begin
		fetch_cnt <= fetch_num;
		spu_addr <= fetch_addr;
	end
	
	else if (spu_next)
	begin
		fetch_cnt <= fetch_cnt - 9'd1;
		spu_addr <= spu_addr + 21'd1;
	end
		
		
	// dram request
	always @(posedge clk)
	if (fetch_go)
		spu_req <= 1'b1;
		
	else if (~|fetch_cnt || pre_hline)
		spu_req <= 1'b0;

		
	// data fetching
	reg [15:0] spu_data;
	wire spu_data_valid = spu_cycle && dram_rd_valid;
	
	always @(posedge clk)
	if (spu_data_valid)
		spu_data <= rd;

		
//	SPU FSM -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	// FSM states
	localparam MS_HALT	= 4'd0;
	localparam MS_ST0	= 4'd1;
	localparam MS_ST1	= 4'd2;
	localparam MS_ST2	= 4'd3;
	localparam MS_ST3	= 4'd4;
	localparam MS_ST4	= 4'd5;
	localparam MS_ST5	= 4'd6;
	localparam MS_ST6	= 4'd7;
	localparam MS_ST7	= 4'd8;
	localparam MS_ST8	= 4'd9;
	localparam MS_ST9	= 4'd10;
	localparam MS_ST10	= 4'd11;
	localparam MS_ST11	= 4'd12;
	localparam MS_ST12	= 4'd13;
	localparam MS_ST13	= 4'd14;
	localparam MS_ST14	= 4'd15;
	
	
	// Regs for SFILE parameters
	reg [8:0]	xpos_r;
	reg [8:0]	ypos_r;
	reg [5:0]	xsiz_r;
	reg [5:0]	ysiz_r;
	reg 		xflp_r;
	reg 		yflp_r;
	reg [8:0]	xbmp_r;
	reg [8:0]	ybmp_r;
	reg [5:0]	pbmp_r;
	reg [3:0]	spal_r;
	reg [1:0]	spri_r;
	reg [1:0]	cres_r;


	// SFILE fields
	wire [7:0]	sf_xpos_l	= spsf_rd[7:0];		// reg0
	wire 		sf_xflp		= spsf_rd[7];		// reg1
	wire [5:0]	sf_xsiz		= spsf_rd[6:1];		// reg1
	wire 		sf_xpos_h	= spsf_rd[0];		// reg1
	wire [7:0]	sf_ypos_l	= spsf_rd[7:0];		// reg2
	wire 		sf_yflp		= spsf_rd[7];		// reg3
	wire [5:0]	sf_ysiz		= spsf_rd[6:1];		// reg3
	wire 		sf_ypos_h	= spsf_rd[0];		// reg3
	wire [7:0]	sf_xbmp_l	= spsf_rd[7:0];		// reg4
	wire [7:0]	sf_ybmp_l	= spsf_rd[7:0];		// reg5
	wire 		sf_xbmp_h	= spsf_rd[0];		// reg6
	wire 		sf_pbmp		= spsf_rd[6:1];		// reg6
	wire 		sf_ybmp_h	= spsf_rd[7];		// reg6
	wire [3:0]	sf_spal		= spsf_rd[7:4];		// reg7
	wire [1:0]	sf_spri		= spsf_rd[3:2];		// reg7
	wire [1:0]	sf_cres		= spsf_rd[1:0];		// reg7

	
	// State dependent values
	// If we have the data on the output of SFILE at current FSM state, we can use it immediately,
	// else we take it from the dedicated register
	wire [8:0]	xpos	= (sreg == 3'd1) ? {sf_xpos_h, xpos_r[7:0]} : xpos_r;
	wire [8:0]	xsiz	= (sreg == 3'd1) ? {sf_xsiz, 3'b0} : {xsiz_r, 3'b0};
	wire 		xflp	= (sreg == 3'd1) ? sf_flp : xflp_r;
	wire [8:0]	ypos	= (sreg == 3'd3) ? {sf_ypos_h, ypos_r[7:0]} : ypos_r;
	wire [8:0]	ysiz	= (sreg == 3'd3) ? {sf_ysiz, 3'b0} : {ysiz_r, 3'b0};
	wire 		yflp	= (sreg == 3'd3) ? sf_flp : yflp_r;
	wire [8:0]	xbmp	= (sreg == 3'd6) ? {sf_xbmp_h, xbmp_r[7:0]} : xbmp_r;
	wire [8:0]	ybmp	= (sreg == 3'd6) ? {sf_ybmp_h, ybmp_r[7:0]} : ybmp_r;
	wire [5:0]	pbmp	= (sreg == 3'd6) ? sf_pbmp : pbmp_r;
	wire [3:0]	spal	= (sreg == 3'd7) ? sf_spal : spal_r;
	wire [1:0]	spri	= (sreg == 3'd7) ? sf_spri : spri_r;
	wire [1:0]	cres	= (sreg == 3'd7) ? sf_cres : cres_r;


	// Regs clocking
	reg [4:0] snum_r;
	
	always @(posedge clk)
	begin
		if (sreg == 3'd0)	xpos_r[7:0]	<= sf_xpos_l;
		if (sreg == 3'd1)	xpos_r[8]	<= sf_xpos_h;
		if (sreg == 3'd1)	xflp_r		<= sf_xflp;
		if (sreg == 3'd1)	xsiz_r		<= sf_xsiz;
		if (sreg == 3'd2)	ypos_r[7:0]	<= sf_ypos_l;
		if (sreg == 3'd3)	ypos_r[8]	<= sf_ypos_h;
		if (sreg == 3'd3)	yflp_r		<= sf_yflp;
		if (sreg == 3'd3)	ysiz_r		<= sf_ysiz;
		if (sreg == 3'd4)	xbmp_r[7:0]	<= sf_xbmp_l;
		if (sreg == 3'd5)	ybmp_r[7:0]	<= sf_ybmp_l;
		if (sreg == 3'd6)	xbmp_r[8]	<= sf_xbmp_h;
		if (sreg == 3'd6)	ybmp_r[8]	<= sf_ybmp_h;
		if (sreg == 3'd6)	pbmp_r		<= sf_pbmp;
		if (sreg == 3'd7)	spal_r		<= sf_spal;
		if (sreg == 3'd7)	spri_r		<= sf_spri;
		if (sreg == 3'd7)	cres_r		<= sf_cres;
		
		snum_r <= snum;
	end


	// Precalculated values
	wire [ 7:0]	bmp_page		= 8'd0;
	wire		sprite_active	= |cres;
	wire		sprite_visible	= ((vline >= ypos) && (vline < (ypos + ysiz)));
	wire		sprite_last		= |snum_next;
	wire [ 4:0]	snum_next		= snum_r + 5'b1;

	wire [20:0] bmp_addr		= cres[0] ? bmp_addr64c : bmp_addr16c;
	wire [20:0] bmp_addr16c		= {bmp_haddr16c, bmp_laddr16c};
	wire [ 7:0] bmp_haddr16c	= {{pbmp, 2'b0} + ybmp[8:6]};
	wire [12:0] bmp_laddr16c	= {ybmp[5:0], xbmp[8:2]};
	wire [20:0] bmp_addr64c		= {bmp_haddr64c, bmp_laddr64c};
	wire [ 7:0] bmp_haddr64c	= {{pbmp, 2'b0} + ybmp[8:5]};
	wire [12:0] bmp_laddr64c	= {ybmp[4:0], xbmp[8:1]};
		
		
	// FSM state control
	reg [3:0] ms;			// current state of FSM
	
	always @(posedge clk)
	if !(spu_en && (vvis || pre_vline))
		ms <= MS_HALT;
	else if (pre_hline)
		ms <= MS_ST0;
	else
		ms <= ms_next;

	
	// ms_next
	reg [3:0] ms_next;		// next state of FSM
	
	always @*
	case (ms)
MS_HALT:	ms_next = MS_HALT;
MS_ST0:		ms_next = sprite_active ? MS_ST1 : (!sprite_last ? MS_ST0 : MS_HALT);
MS_ST1:		ms_next = MS_ST2;
MS_ST2:		ms_next = sprite_visible ? MS_ST3 : (!sprite_last ? MS_ST0 : MS_HALT);
MS_ST3:		ms_next = MS_ST4;
MS_ST4:		ms_next = MS_ST5;
MS_ST5:		ms_next = MS_ST6;
MS_ST6:		ms_next = MS_ST7;
MS_ST7:		ms_next = MS_ST8;
MS_ST8:		ms_next = MS_ST9;
default:	ms_next = MS_HALT;
	endcase
	
	
	// sreg
	reg [2:0] sreg;			// number of current processed SFILE register (0-7)
	
	always @*
	case (ms)
MS_ST0:		sreg = sprite_active ? 3'd2 : 3'd7;
MS_ST1:		sreg = 3'd3;
MS_ST2:		sreg = sprite_visible ? 3'd0 : 3'd7;
MS_ST3:		sreg = 3'd1;
MS_ST4:		sreg = 3'd4;
MS_ST5:		sreg = 3'd5;
MS_ST6:		sreg = 3'd6;
default:	sreg = 3'd7;
	endcase

	
	// snum
	reg [4:0] snum;			// number of current processed sprite (0-31)
	
	always @*
	case (ms)
MS_HALT:	snum = 5'b0;
MS_ST0:		snum = sprite_active ? snum_r : snum_next;
MS_ST2:		snum = sprite_visible ? snum_r : snum_next;
default:	snum = snum_r;
	endcase

	
	// fetch control
	wire fetch_go = (ms == MS_ST7);
	wire [20:0] fetch_addr = bmp_addr;
	
	

// -------------------------------------------------	
	
	
	reg [8:0] lofs;
	reg [2:0] pxn;
	reg [1:0] pri;


	wire [5:0]	s_next = num + 6'd1;
	wire		s_last = (s_next == 5'd0);
	wire [7:0]	dec_xsiz = xs - 8'd1;
	wire		s_eox = (dec_xsiz == 8'b0);
	wire [7:0]	xsc = (cres == 2'b11) ? {sf_rd[7:1], 1'b0} : {1'b0, sf_rd[7:1]};
	wire [8:0]	lofsc = flipy ? (ypc - vline + ysc - 9'd1) : (vline - ypc);
	wire [20:0]	adr_ofs = {sf_rd[7:0], spu_addr[12:0]} + (xs * lofs);
	wire [20:0]	adr_next = spu_addr + 21'b1;
	wire [8:0]	sl_next = flipx ? (sline_waddr - 9'd1) : (sline_waddr + 9'd1);
	wire [8:0]	xsf = (cres == 2'b11) ? (xs * 9'd2) : ((cres == 2'b10) ? (xs[6:0] * 9'd4) : (xs[5:0] * 9'd8));
	wire [8:0]	xc = flipx ? ({sline_waddr[8], sf_rd[7:0]} + xsf - 9'd1) : {sline_waddr[8], sf_rd[7:0]};


	case (ms)
	
	ms_st1:		//fucking obsolete cycle, if sprite invisible!!!!!!!!
	begin
		spu_addr[12:7] <= sf_rd[5:0];	//get ADR[13:8]
		sf_sa <= r_yp;
		ms <= ms_st2;
	end
	
	ms_st2:
	begin
		yp <= sf_rd[7:0];			//get Y[7:0]
		sf_sa <= r_ysiz;
		ms <= ms_st3;
	end

	ms_st3:
	begin
		//check if sprite is visible on this line
		if (s_vis)	//get Y[8], YS[6:0]
		//yes
		begin
			lofs <= lofsc;		//set number of line within sprite (0-xxx), flipped is necessary
			sf_sa <= r_xsiz;
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
				ms <= ms_st0;
			end
			else
		//yes: halt
			ms <= ms_halt;
		end
	end

	ms_st4: 
	begin
		sline_waddr[8] <= sf_rd[0];		//get X[8]
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
		sline_waddr[8:0] <= xc;		//get X[7:0], flipped if necessary
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
		sline_waddr <= sl_next;
		sp_ra[1:0] <= {sdbuf[(13-pxn*2)], sdbuf[(12-pxn*2)]};		//set paladdr for pix1
		pxn <= pxn + 3'b1;
		ms <= (pxn == 3'd6) ? ms_eow : ms_4c1;
	end

	ms_16c1: //write pix1@16c
	begin
		sline_waddr <= sl_next;
		sp_ra[3:0] <= {sdbuf[11-pxn*4], sdbuf[10-pxn*4], sdbuf[9-pxn*4], sdbuf[8-pxn*4]};		//set paladdr for pix1
		pxn <= pxn + 3'b1;
		ms <= (pxn == 3'd2) ? ms_eow : ms_16c1;
	end

	ms_tc1:	//write pix1@true
	begin
	if (!s_eox)
		begin
		end
		sline_waddr <= sl_next;
		ms <= ms_eow;
	end
	
	ms_eow:	//end of write to sline
	begin
		sl_we <= 1'b0;
		sline_waddr <= sl_next;

		//check if xs=0
		if (!s_eox)
		//no: go to begin of loop
		begin
		xs <= dec_xsiz;
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
			ms <= ms_st0;
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
	reg [8:0] sline_waddr;
	wire [7:0] sl_rd0, sl_rd1;

	wire		 pixs = (ms == ms_tc1);
	wire		 pixt = pixs ? !spu_data[14] : !sdbuf[6];
	wire [6:0]	 pixc = pixs ? {!spu_data[15], spu_data[13:8]} : {!sdbuf[7], sdbuf[5:0]};
	wire		 tcol = (cres == 2'b11);
	wire		 sline_ws = (tcol ? pixt : !sp_rd[6]) && sl_we;
	wire [6:0]	 sline_wdata = tcol ? pixc : {!sp_rd[7], sp_rd[5:0]};
	
	sline0 sline0(	.wraddress(l_sel ? sline_waddr : sline_raddr),
					.data(l_sel ? sline_wdata : 8'b0),
					.wren(l_sel ? sline_ws : sline_nul_ws),
					.rdaddress(sline_raddr),
					.q(sl_rd0),
					.clock(clk)
				);

	sline1 sline1(	.wraddress(!l_sel ? sline_waddr : sline_raddr),
					.data(!l_sel ? sline_wdata : 8'b0),
					.wren(!l_sel ? sline_ws : sline_nul_ws),
					.rdaddress(sline_raddr),
					.q(sl_rd1),
					.clock(clk)
				);
endmodule
