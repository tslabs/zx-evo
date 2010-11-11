`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Sprite Processor
//
// Written by TS-Labs inc.
// ver. 2.0
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
// - optimize usage of summators
// - check spr_en and turn off SPU at VBLANK
// - Read spr_dat from DRAM
// - Fix jerking while write to sfile


module sprites(

	input clk, spr_en, line_start, pre_vline,
	input [7:0] din,
	output reg test,
	output wire [5:0] mcd,

//sfile	
	output reg [7:0] sf_ra,
	input [7:0] sf_rd,

//smem	//#debug!!!
	output reg [9:0] sm_ra,
	input [15:0] sm_rd,
	
//spram
	output reg [7:0] sp_ra,
	input [5:0] sp_rd,
	
//dram
	output reg [20:0] spr_addr,
//	input [15:0] spr_dat,
	output reg spr_mrq,
//	input spr_drdy,

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

	
// Sprites processing

	//#debug!!!
	reg [15:0] spr_dat;
	reg [13:0] sdbuf;
	reg spr_drdy;
	reg [3:0] mrq;
	reg [15:0] mdat;
	
	initial
	begin
		mrq <= 4'd0;
	end
	
	always @(posedge clk)
	case (mrq)
	0:	if (spr_mrq)
		begin
			spr_drdy <= 1'b0;
			sm_ra <= spr_addr[9:0];
			mrq <= 4'd1;
		end
	1:	begin
			mdat <= sm_rd;
			mrq <= 4'd2;
		end
	2:	mrq <= 4'd3;
	3:	mrq <= 4'd4;
	4:	mrq <= 4'd5;
	5:	mrq <= 4'd6;
	6:	mrq <= 4'd8;
	8:	begin
			spr_dat <= mdat;
			spr_drdy <= 1'b1;
			mrq <= 4'd9;
		end
	9:	if (!spr_mrq)
			mrq <= 4'd0;
	endcase
	
	
	localparam VLINES = 9'd288;
	
	reg [4:0] num;		//number of currently processed sprite
	reg [13:0] offs;	//offset from sprite address, words
	reg [5:0] pix;
	reg [1:0] cres;
	reg [20:0] adr;		//word address!!! 2MB x 16bit
	reg [6:0] xsz;
	reg [8:0] ypos;

	assign mcd = ms;
	
// Marlezonskiy balet, part I

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
	localparam ms_loop = 5'd9;
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

	wire s_vis, s_last, s_act, s_eox;
	wire [4:0] s_next;
	wire [20:0] adr_offs;
	wire [13:0] offs_next;
	wire [6:0] s_decx;
	assign s_act = !(sf_rd[7:6] == 2'b0);
	assign s_next = num + 5'd1;
	assign s_last = (s_next == 5'd0);
	assign s_vis = ((vline >= {sf_rd[0], ypos[7:0]}) && (vline < ({sf_rd[0], ypos[7:0]} + sf_rd[7:1])));
	assign adr_offs = adr + offs;
	assign offs_next = offs + 14'b1;
	assign s_decx = xsz - 7'd1;
	assign s_eox = (s_decx == 7'b0);

	
// Only checking for FSM reset and next state applying
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
		sa_we <= 1'b0;
		sl_we <= 1'b0;
		spr_mrq <= 1'b0;

		num <= 5'd0;			//set sprite number to 0
		sf_ra <= 8'd4;			//set addr for reg4
		ms <= ms_beg;
	end

	ms_beg:	// Begin of sprite[num] processing
	begin
		//check if sprite is active
		if (s_act)
		begin
			cres <= sf_rd[7:6];			//get CRES
			sp_ra[7:2] <= sf_rd[5:0];	//get PAL[5:0]
			sf_ra[2:0] <= 3'd2;			//set addr for reg2
			ms <= ms_st2;
		end
		else
		begin
			if (!s_last)				//check if all 32 sprites done
		//no: next sprite processing
			begin
			num <= s_next;
			sf_ra <= {s_next, 3'd4};	//next sprite num, set addr for reg4
			ms <= ms_beg;
			end
			else
		//yes: halt
			ms <= ms_halt;
		end
	end

	ms_st2:
	begin
		ypos[7:0] <= sf_rd[7:0];	//get YPOS[7:0]
		sf_ra[2:0] <= 3'd3;			//set addr for reg3
		ms <= ms_st3;
	end

	ms_st3:
	begin
		ypos[8] <= sf_rd[0];		//get YPOS[8]
		//check if sprite is visible on this line
		if (s_vis)
		//yes
		begin
			sf_ra[2:0] <= 3'd5;			//set addr for reg5
			ms <= ms_st4;
		end
		else
		//no
		begin
			if (!s_last)	//check if all 32 sprites done
		//no: next sprite processing
			begin
				num <= s_next;
				sf_ra <= {s_next, 3'd4};	//inc sprite num, set addr for reg4
				ms <= ms_beg;
			end
			else
		//yes: halt
			ms <= ms_halt;
		end
	end

	ms_st4: 
	begin
		adr[7:0] <= sf_rd[7:0];		//get ADR[7:0]
		sf_ra[2:0] <= 3'd6;			//set addr for reg6
		if (ypos == vline)			//check if 1st line of sprite
			offs <= 14'b0;			//yes: null offs
		else
			offs <= sa_rd;			//no: get offs from sacnt
		ms <= ms_st5;
	end

	ms_st5:
	begin
		adr[15:8] <= sf_rd[7:0];	//get ADR[15:8]
		sf_ra[2:0] <= 3'd7;			//set addr for reg7
		ms <= ms_st6;
	end

	ms_st6:
	begin
		adr[20:16] <= sf_rd[4:0];	//get ADR[20:16]
		sf_ra[2:0] <= 3'd0;			//set addr for reg0
		ms <= ms_st7;
	end

	ms_st7:
	begin
		sl_wa[7:0] <= sf_rd[7:0];	//get XPOS[7:0]
		sf_ra[2:0] <= 3'd1;			//set addr for reg1
		spr_addr <= adr_offs;		//set spr_addr

		spr_mrq <= 1'b1;			//assert spr_mrq
		offs <= offs_next;			//inc offs
		sa_wd <= offs_next;			//write next offs
		sa_we <= 1'b1;
		ms <= ms_st8;
	end

	ms_st8:
	begin
		sa_we <= 1'b0;
		sl_wa[8] <= sf_rd[0];		//get XPOS[8]
		xsz[6:0] <= sf_rd[7:1];		//get XSZ[6:0]
		ms <= ms_loop;
	end
	
	ms_loop: //begin of loop
	//wait for data from DRAM
	if (!spr_drdy)
		ms <= ms_loop;
	else
	begin
		spr_mrq <= 1'b0;			//deassert spr_mrq
		sdbuf <= spr_dat[13:0];
		//write pix0
		case (cres)

		1:	begin	//4c
				sp_ra[1:0] <= spr_dat[15:14];		//set paladdr for pix0
				sl_we <= !(spr_dat[15:14] == 2'b0);	//set transparency for pix0
				ms <= ms_4c1;
			end
	
		2:	begin	//16c
				sp_ra[3:0] <= spr_dat[15:12];		//set paladdr for pix0
				sl_we <= !(spr_dat[15:12] == 4'b0);	//set transparency for pix0
				ms <= ms_16c1;
			end
		
		3:	begin	//true color
				pix <= spr_dat[13:8];				//set pix0
				sl_we <= !spr_dat[15];				//set transparency for pix0
				ms <= ms_tc1;
			end

		endcase
	end

	ms_4c1:	//write pix1@4c
	begin
	if (!s_eox)
	begin
		spr_addr <= adr_offs;	//set spr_addr
		spr_mrq <= 1'b1;			//assert spr_mrq
		offs <= offs_next;		//inc offs
		sa_wd <= offs_next;		//write offs
		sa_we <= 1'b1;
	end
		sl_wa <= sl_wa + 9'b1;
		sp_ra[1:0] <= sdbuf[13:12];		//set paladdr for pix1
		sl_we <= !(sdbuf[13:12] == 2'b0);	//set transparency for pix1
		ms <= ms_4c2;
	end

	ms_4c2:	//write pix2@4c
	begin
		sa_we <= 1'b0;
		sl_wa <= sl_wa + 9'b1;
		sp_ra[1:0] <= sdbuf[11:10];		//set paladdr for pix2
		sl_we <= !(sdbuf[11:10] == 2'b0);	//set transparency for pix2
		ms <= ms_4c3;
	end

	ms_4c3:	//write pix3@4c
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[1:0] <= sdbuf[9:8];			//set paladdr for pix3
		sl_we <= !(sdbuf[9:8] == 2'b0);		//set transparency for pix3
		ms <= ms_4c4;
	end

	ms_4c4:	//write pix4@4c
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[1:0] <= sdbuf[7:6];			//set paladdr for pix4
		sl_we <= !(sdbuf[7:6] == 2'b0);		//set transparency for pix4
		ms <= ms_4c5;
	end

	ms_4c5:	//write pix5@4c
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[1:0] <= sdbuf[5:4];			//set paladdr for pix5
		sl_we <= !(sdbuf[5:4] == 2'b0);		//set transparency for pix5
		ms <= ms_4c6;
	end

	ms_4c6:	//write pix6@4c
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[1:0] <= sdbuf[3:2];			//set paladdr for pix6
		sl_we <= !(sdbuf[3:2] == 2'b0);		//set transparency for pix6
		ms <= ms_4c7;
	end

	ms_4c7:	//write pix7@4c
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[1:0] <= sdbuf[1:0];				//set paladdr for pix7
		sl_we <= !(sdbuf[1:0] == 2'b0);		//set transparency for pix7
		ms <= ms_eow;
	end

	ms_16c1: //write pix1@16c
	begin
	if (!s_eox)
	begin
		spr_addr <= adr_offs;	//set spr_addr
		spr_mrq <= 1'b1;			//assert spr_mrq
		offs <= offs_next;		//inc offs
		sa_wd <= offs_next;		//write offs
		sa_we <= 1'b1;
	end
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sdbuf[11:8];		//set paladdr for pix1
		sl_we <= !(sdbuf[11:8] == 4'b0);	//set transparency for pix1
		ms <= ms_16c2;
	end

	ms_16c2: //write pix2@16c
	begin
		sa_we <= 1'b0;
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sdbuf[7:4];			//set paladdr for pix2
		sl_we <= !(sdbuf[7:4] == 4'b0);	//set transparency for pix2
		ms <= ms_16c3;
	end

	ms_16c3: //write pix3@16c
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sdbuf[3:0];			//set paladdr for pix3
		sl_we <= !(sdbuf[3:0] == 4'b0);	//set transparency for pix3
		ms <= ms_eow;
	end

	ms_tc1:	//write pix1@true
	begin
	if (!s_eox)
		begin
			spr_addr <= adr_offs;		//set spr_addr
			spr_mrq <= 1'b1;			//assert spr_mrq
			offs <= offs_next;			//inc offs
			sa_wd <= offs_next;			//write offs
			sa_we <= 1'b1;
		end
		sl_wa <= sl_wa + 9'b1;
		pix <= sdbuf[5:0];				//set pix1
		sl_we <= sdbuf[7];				//set transparency for pix1
		ms <= ms_eow;
	end

	ms_eow:	//end of write to sline
	begin
		sa_we <= 1'b0;
		sl_we <= 1'b0;
		sl_wa <= sl_wa + 9'b1;

		//check if xsz=0
		if (!s_eox)
		//no: go to begin of loop
		begin
		xsz <= s_decx;
		ms <= ms_loop;
		end
		else
		//yes:
		begin
		if (!s_last)		//check if all 32 sprites done
		//no: next sprite processing
		begin
			num <= s_next;
			sf_ra <= {s_next, 3'd4};	//inc spnum, set addr for reg4
			ms <= ms_beg;
		end
		else
		//yes: halt
		begin
			test <= 1'b0;
			ms <= ms_halt;
		end
		end
	end
	
	ms_halt: //idle state
		begin
			test <= 1'b0;
			ms <= ms_halt;
		end

	default: //idle state
		begin
			ms <= ms_halt;
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


	reg [8:0] sl_wa;
//	reg [6:0] sl_wd;
	reg sl_ws, sl_we;
	wire [6:0] sl_rd0, sl_rd1;
	reg sl_ws0, sl_ws1;

	always @*
	begin
	if (!clk)
		begin
			sl_ws <= sl_we;
			sa_ws <= sa_we;
		end
	else
		begin
			sl_ws <= 1'b0;
			sa_ws <= 1'b0;
		end
	end

	
	sline0 sline0(	.wraddress(l_sel ? sl_wa : sl_ra),
					.data(l_sel ? {1'b1, (cres == 2'b11 ? pix : sp_rd[5:0])} : 7'b0),
					.wren(l_sel ? sl_ws : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd0)
				);

	sline1 sline1(	.wraddress(!l_sel ? sl_wa : sl_ra),
					.data(!l_sel ? {1'b1, (cres == 2'b11 ? pix : sp_rd[5:0])} : 7'b0),
					.wren(!l_sel ? sl_ws : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd1)
				);

	wire [13:0] sa_rd;
	reg [13:0] sa_wd;
	reg sa_ws, sa_we;

	sacnt sacnt(	.wraddress(num),
					.data(sa_wd),
					.wren(sa_ws),
					.rdaddress(num),
					.q(sa_rd)
				);
endmodule

// SMEM: Sprites memory file
// 16384 bit = 1024x16
// #debug!!!

module smem (
	data,
	rdaddress,
	wraddress,
	wrclock,
	wren,
	q);

	input	[15:0]  data;
	input	[8:0]  rdaddress;
	input	[8:0]  wraddress;
	input	  wrclock;
	input	  wren;
	output	[15:0]  q;

	wire [15:0] sub_wire0;
	wire [15:0] q = sub_wire0[15:0];

	altdpram	altdpram_component (
				.wren (wren),
				.inclock (wrclock),
				.data (data),
				.rdaddress (rdaddress),
				.wraddress (wraddress),
				.q (sub_wire0),
				.aclr (1'b0),
				.byteena (1'b1),
				.inclocken (1'b1),
				.outclock (1'b1),
				.outclocken (1'b1),
				.rdaddressstall (1'b0),
				.rden (1'b1),
				.wraddressstall (1'b0));
	defparam
		altdpram_component.indata_aclr = "OFF",
		altdpram_component.indata_reg = "UNREGISTERED",
		altdpram_component.intended_device_family = "ACEX1K",
		altdpram_component.lpm_type = "altdpram",
		altdpram_component.outdata_aclr = "OFF",
		altdpram_component.outdata_reg = "UNREGISTERED",
		altdpram_component.rdaddress_aclr = "OFF",
		altdpram_component.rdaddress_reg = "UNREGISTERED",
		altdpram_component.rdcontrol_aclr = "OFF",
		altdpram_component.rdcontrol_reg = "UNREGISTERED",
		altdpram_component.width = 16,
		altdpram_component.widthad = 9,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "UNREGISTERED",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule


// SFILE: Sprites description file
// 2048 bit = 256x8

module sfile (
	data,
	rdaddress,
	wraddress,
	wrclock,
	wren,
	q);

	input	[7:0]  data;
	input	[7:0]  rdaddress;
	input	[7:0]  wraddress;
	input	  wrclock;
	input	  wren;
	output	[7:0]  q;

	wire [7:0] sub_wire0;
	wire [7:0] q = sub_wire0[7:0];

	altdpram	altdpram_component (
				.wren (wren),
				.inclock (wrclock),
				.data (data),
				.rdaddress (rdaddress),
				.wraddress (wraddress),
				.q (sub_wire0),
				.aclr (1'b0),
				.byteena (1'b1),
				.inclocken (1'b1),
				.outclock (1'b1),
				.outclocken (1'b1),
				.rdaddressstall (1'b0),
				.rden (1'b1),
				.wraddressstall (1'b0));
	defparam
		altdpram_component.indata_aclr = "OFF",
		altdpram_component.indata_reg = "UNREGISTERED",
		altdpram_component.intended_device_family = "ACEX1K",
		altdpram_component.lpm_type = "altdpram",
		altdpram_component.outdata_aclr = "OFF",
		altdpram_component.outdata_reg = "UNREGISTERED",
		altdpram_component.rdaddress_aclr = "OFF",
		altdpram_component.rdaddress_reg = "UNREGISTERED",
		altdpram_component.rdcontrol_aclr = "OFF",
		altdpram_component.rdcontrol_reg = "UNREGISTERED",
		altdpram_component.width = 8,
		altdpram_component.widthad = 8,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "UNREGISTERED",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule


//palette SRAM for sprites 256x6
module spram (
	data,
	rdaddress,
	wraddress,
	wrclock,
	wren,
	q);

	input	[5:0]  data;
	input	[7:0]  rdaddress;
	input	[7:0]  wraddress;
	input	  wrclock;
	input	  wren;
	output	[5:0]  q;

	wire [5:0] sub_wire0;
	wire [5:0] q = sub_wire0[5:0];

	altdpram	altdpram_component (
				.wren (wren),
				.inclock (wrclock),
				.data (data),
				.rdaddress (rdaddress),
				.wraddress (wraddress),
				.q (sub_wire0),
				.aclr (1'b0),
				.byteena (1'b1),
				.inclocken (1'b1),
				.outclock (1'b1),
				.outclocken (1'b1),
				.rdaddressstall (1'b0),
				.rden (1'b1),
				.wraddressstall (1'b0));
	defparam
		altdpram_component.indata_aclr = "OFF",
		altdpram_component.indata_reg = "UNREGISTERED",
		altdpram_component.intended_device_family = "ACEX1K",
		altdpram_component.lpm_type = "altdpram",
		altdpram_component.outdata_aclr = "OFF",
		altdpram_component.outdata_reg = "UNREGISTERED",
		altdpram_component.rdaddress_aclr = "OFF",
		altdpram_component.rdaddress_reg = "UNREGISTERED",
		altdpram_component.rdcontrol_aclr = "OFF",
		altdpram_component.rdcontrol_reg = "UNREGISTERED",
		altdpram_component.width = 6,
		altdpram_component.widthad = 8,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "UNREGISTERED",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule


//counters for sprite address
//32 x 14 bit
//so, sprite can occupy max 128 words x 128 lines = 16kwords in DRAM
module sacnt (
        data,
        wraddress,
        wren,
        rdaddress,
        q);

        input   [13:0]  data;
        input   [4:0]  rdaddress;
        input   [4:0]  wraddress;
        input     wren;
        output  [13:0]  q;

        wire [13:0] sub_wire0;
        wire [13:0] q = sub_wire0[13:0];

        lpm_ram_dp      lpm_ram_dp_component (
                                .wren (wren),
                                .data (data),
                                .rdaddress (rdaddress),
                                .wraddress (wraddress),
                                .q (sub_wire0),
                                .rdclken (1'b1),
                                .rdclock (1'b1),
                                .rden (1'b1),
                                .wrclken (1'b1),
                                .wrclock (1'b1));
        defparam
                lpm_ram_dp_component.intended_device_family = "ACEX1K",
                lpm_ram_dp_component.lpm_indata = "UNREGISTERED",
                lpm_ram_dp_component.lpm_outdata = "UNREGISTERED",
                lpm_ram_dp_component.lpm_rdaddress_control = "UNREGISTERED",
                lpm_ram_dp_component.lpm_type = "LPM_RAM_DP",
                lpm_ram_dp_component.lpm_width = 14,
                lpm_ram_dp_component.lpm_widthad = 5,
                lpm_ram_dp_component.lpm_numwords = 32,
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";
endmodule


//sprite video buffer0
module sline0 (
        input   [6:0]  data,
        input   [8:0]  rdaddress,
        input   [8:0]  wraddress,
        input   		wren,
        output  [6:0]  q
);

        lpm_ram_dp      lpm_ram_dp_component (
                                .wren (wren),
                                .data (data),
                                .rdaddress (rdaddress),
                                .wraddress (wraddress),
                                .q (q),
                                .rdclken (1'b1),
                                .rdclock (1'b1),
                                .rden (1'b1),
                                .wrclken (1'b1),
                                .wrclock (1'b1));
        defparam
                lpm_ram_dp_component.intended_device_family = "ACEX1K",
                lpm_ram_dp_component.lpm_indata = "UNREGISTERED",
                lpm_ram_dp_component.lpm_outdata = "UNREGISTERED",
                lpm_ram_dp_component.lpm_rdaddress_control = "UNREGISTERED",
                lpm_ram_dp_component.lpm_type = "LPM_RAM_DP",
                lpm_ram_dp_component.lpm_width = 7,
                lpm_ram_dp_component.lpm_widthad = 9,
                lpm_ram_dp_component.lpm_numwords = 360,
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";
endmodule


//sprite video buffer1
module sline1 (
        input   [6:0]  data,
        input   [8:0]  rdaddress,
        input   [8:0]  wraddress,
        input   		wren,
        output  [6:0]  q
);

        lpm_ram_dp      lpm_ram_dp_component (
                                .wren (wren),
                                .data (data),
                                .rdaddress (rdaddress),
                                .wraddress (wraddress),
                                .q (q),
                                .rdclken (1'b1),
                                .rdclock (1'b1),
                                .rden (1'b1),
                                .wrclken (1'b1),
                                .wrclock (1'b1));
        defparam
                lpm_ram_dp_component.intended_device_family = "ACEX1K",
                lpm_ram_dp_component.lpm_indata = "UNREGISTERED",
                lpm_ram_dp_component.lpm_outdata = "UNREGISTERED",
                lpm_ram_dp_component.lpm_rdaddress_control = "UNREGISTERED",
                lpm_ram_dp_component.lpm_type = "LPM_RAM_DP",
                lpm_ram_dp_component.lpm_width = 7,
                lpm_ram_dp_component.lpm_widthad = 9,
                lpm_ram_dp_component.lpm_numwords = 360,
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";
endmodule
