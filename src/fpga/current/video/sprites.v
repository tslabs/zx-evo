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


module sprites(

	input clk, spu_en, line_start, pre_vline,
	input cbeg, post_cbeg,
	input [7:0] din,
	output reg test,
	output wire [5:0] mcd,

//sfile	
	output wire [8:0] sf_ra,
	input [7:0] sf_rd,

//spram
	output reg [7:0] sp_ra,
	input [7:0] sp_rd,
	
//dram
	output reg [20:0] spu_addr,
	input [15:0] spu_data,
	output reg spu_req,
	input spu_strobe, spu_next,

//video
	output reg [5:0] spixel,
	output reg spx_en

	);

	reg [8:0] vline;
	reg l_sel;

	
//vcount
	always @(posedge clk)
	if (line_start)
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
	reg [8:0] sl_ra;
	reg sl_wsn;
	
	always @(posedge clk)
//	if (!spu_en)
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
	localparam VLINES = 9'd288;
	
	reg [5:0] num;		//number of currently processed sprite
	reg [2:0] sf_sa;	//sub-address for SFILE
	reg [1:0] cres;
	reg [7:0] xs;
	reg [7:0] yp;
	reg [8:0] lofs;
	reg [13:0] sdbuf;
	reg flipx;

	assign mcd = ms;
	
// Marlezonskiy balet

	reg [5:0] ms;		//current and next states of FSM

	localparam ms_res = 5'd0;
	localparam ms_beg = 5'd1;
	localparam ms_st2 = 5'd2;
	localparam ms_st3 = 5'd3;
	localparam ms_st4 = 5'd4;
	localparam ms_st5 = 5'd5;
	localparam ms_st6 = 5'd6;
	localparam ms_st7 = 5'd7;
	localparam ms_st8 = 5'd8;
	localparam ms_lbeg = 5'd9;
	localparam ms_4c1 = 5'd10;
	localparam ms_4c2 = 5'd11;
	localparam ms_4c3 = 5'd12;
	localparam ms_4c4 = 5'd13;
	localparam ms_4c5 = 5'd14;
	localparam ms_4c6 = 5'd15;
	localparam ms_4c7 = 5'd16;
	localparam ms_16c1 = 5'd17;
	localparam ms_16c2 = 5'd18;
	localparam ms_16c3 = 5'd19;
	localparam ms_tc1 = 5'd20;
	localparam ms_eow = 5'd21;
	localparam ms_halt = 5'd31;
	
	localparam r_xp = 3'd0;
	localparam r_xs = 3'd1;
	localparam r_xv = 3'd2;
	localparam r_yp = 3'd3;
	localparam r_ys = 3'd4;
	localparam r_cr = 3'd5;
	localparam r_al = 3'd6;
	localparam r_ah = 3'd7;

	wire s_vis, s_last, s_act, s_eox;
	wire [5:0] s_next;
	wire [20:0] adr_next;
	wire [20:0] adr_ofs;
	wire [7:0] dec_xs;
	wire [8:0] ypc;
	wire [8:0] ysc;
	wire [7:0] xsc;
	wire [7:0] xsv;
	wire [8:0] lofsc;
	wire [8:0] sl_next;
	wire [8:0] xc;
	wire [8:0] xsf;
	assign sf_ra = {num, sf_sa};
	assign s_act = !(sf_rd[1:0] == 2'b0);
	assign s_next = num + 6'd1;
	assign s_last = (s_next == 5'd0);
	assign ypc = {sf_rd[7], yp};
	assign ysc = {sf_rd[5:0], 3'b0};
	assign s_vis = ((vline >= ypc) && (vline < (ypc + ysc)));
	assign adr_next = spu_addr + 21'b1;
	assign dec_xs = xs - 8'd1;
	assign s_eox = (dec_xs == 8'b0);
	assign xsc = (cres == 2'b11) ? {sf_rd[6:0], 1'b0} : {1'b0, sf_rd[6:0]};
	assign lofsc = sf_rd[6] ? (ypc - vline + ysc - 1) : (vline - ypc);
	assign adr_ofs = {sf_rd[7:0], spu_addr[12:0]} + (xs * lofs);
	assign sl_next = flipx ? (sl_wa - 9'b1) : (sl_wa + 9'b1);
	assign xsf = (cres == 2'b11) ? (xs * 2) : ((cres == 2'b10) ? (xs[6:0] * 4) : (xs[5:0] * 8));
	assign xc = flipx ? ({sl_wa[8], sf_rd[7:0]} + xsf - 1) : {sl_wa[8], sf_rd[7:0]};
	
	initial 
	ms = ms_res;

// Here the states are processed on CLK event
	always @(posedge clk, posedge line_start)
	if (line_start)
		ms = ms_res;
	else
	case (ms)
	
	ms_res:	// SPU reset
	begin
		test <= 1'b1;
		spu_req <= 1'b0;
		sl_we <= 1'b0;

		num <= 6'd0;
		sf_sa <= r_cr;
		ms <= ms_beg;
	end

	ms_beg:	// Begin of sprite[num] processing
	begin
		//check if sprite is active
		if (s_act)
		begin
			//read SPReg4
			cres <= sf_rd[1:0];			//get CRES[1:0]
			sp_ra[7:2] <= sf_rd[7:2];	//get PAL[5:0]
			sf_sa <= r_yp;
			ms <= ms_st2;
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

	ms_st2:
	begin
		//read SPReg2
		yp <= sf_rd[7:0];	//get Y[7:0]
		sf_sa <= r_ys;
		ms <= ms_st3;
	end

	ms_st3:
	begin
		//check if sprite is visible on this line
		if (s_vis)	//get Y[8], YS[5:0]
		//yes
		begin
			//read SPReg3
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
		sl_wa[8] <= sf_rd[7];		//get X[8]
		xs <= xsc;					//get XS[7:0]
		sf_sa <= r_al;
		ms <= ms_st5;
	end
		
	ms_st5:
	begin
		//read SPReg5
		spu_addr[12:0] <= {sf_rd[7:0], 5'b0};		//get ADR[12:0]
		sf_sa <= r_ah;
		ms <= ms_st6;
	end

	ms_st6:
	begin
		//read SPReg6
		spu_addr <= adr_ofs;		// get ADR[20:13], ADR = ADR + LOFS * XS - now we get an addr for 1st word of sprite line
		spu_req <= 1'b1;			// now we can assert MRQ
		sf_sa <= r_xv;
		ms <= ms_st7;
	end

	ms_st7:
	begin
		//read SPReg7
		xs <= xsc;					//get XSV[7:0]
		flipx <= sf_rd[7];			//get FLIPX
		sf_sa <= r_xp;
		ms <= ms_st8;
	end

	ms_st8:
	begin
		//read SPReg0
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
		//write pix0
		case (cres)

		1:	begin	//4c
				sp_ra[1:0] <= spu_data[15:14];		//set paladdr for pix0
				ms <= ms_4c1;
			end
	
		2:	begin	//16c
				sp_ra[3:0] <= spu_data[15:12];		//set paladdr for pix0
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
	if (!s_eox)
	begin
//		spu_req <= 1'b1;		//assert spu_req
//		spu_addr <= adr_next;	//set spu_addr
	end
		sl_wa <= sl_next;
		sp_ra[1:0] <= sdbuf[13:12];		//set paladdr for pix1
		ms <= ms_4c2;
	end

	ms_4c2:	//write pix2@4c
	begin
		sl_wa <= sl_next;
		sp_ra[1:0] <= sdbuf[11:10];		//set paladdr for pix2
		ms <= ms_4c3;
	end

	ms_4c3:	//write pix3@4c
	begin	
		sl_wa <= sl_next;
		sp_ra[1:0] <= sdbuf[9:8];			//set paladdr for pix3
		ms <= ms_4c4;
	end

	ms_4c4:	//write pix4@4c
	begin
		sl_wa <= sl_next;
		sp_ra[1:0] <= sdbuf[7:6];			//set paladdr for pix4
		ms <= ms_4c5;
	end

	ms_4c5:	//write pix5@4c
	begin
		sl_wa <= sl_next;
		sp_ra[1:0] <= sdbuf[5:4];			//set paladdr for pix5
		ms <= ms_4c6;
	end

	ms_4c6:	//write pix6@4c
	begin
		sl_wa <= sl_next;
		sp_ra[1:0] <= sdbuf[3:2];			//set paladdr for pix6
		ms <= ms_4c7;
	end

	ms_4c7:	//write pix7@4c
	begin
		sl_wa <= sl_next;
		sp_ra[1:0] <= sdbuf[1:0];				//set paladdr for pix7
		ms <= ms_eow;
	end

	ms_16c1: //write pix1@16c
	begin
	if (!s_eox)
	begin
	end
		sl_wa <= sl_next;
		sp_ra[3:0] <= sdbuf[11:8];		//set paladdr for pix1
		ms <= ms_16c2;
	end

	ms_16c2: //write pix2@16c
	begin
		sl_wa <= sl_next;
		sp_ra[3:0] <= sdbuf[7:4];			//set paladdr for pix2
		ms <= ms_16c3;
	end

	ms_16c3: //write pix3@16c
	begin
		sl_wa <= sl_next;
		sp_ra[3:0] <= sdbuf[3:0];			//set paladdr for pix3
		ms <= ms_eow;
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
	wire sl_wss, pixt, pixs, tcol;
	wire [5:0] pixc;
	reg [8:0] sl_wa;
	wire [6:0] sl_rd0, sl_rd1;
	wire [6:0] sl_wds;

	assign pixs = (ms == ms_tc1);
	assign pixt = pixs ? !spu_data[14] : !sdbuf[6];
	assign pixc = pixs ? spu_data[13:8] : sdbuf[5:0];
	assign tcol = (cres == 2'b11);
	assign sl_wss = (tcol ? pixt : !sp_rd[6]) && sl_we;
	assign sl_wds = {1'b1, (tcol ? pixc : sp_rd[5:0])};
	
	sline0 sline0(	.wraddress(l_sel ? sl_wa : sl_ra),
					.data(l_sel ? sl_wds : 7'b0),
					.wren(l_sel ? sl_wss : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd0),
					.wrclock(clk)
				);

	sline1 sline1(	.wraddress(!l_sel ? sl_wa : sl_ra),
					.data(!l_sel ? sl_wds : 7'b0),
					.wren(!l_sel ? sl_wss : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd1),
					.wrclock(clk)
				);
endmodule
