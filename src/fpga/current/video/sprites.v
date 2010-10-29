`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Sprite Processor
//
// Written by TS-Labs inc.
//
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
// . Fix wrong pal reading
// . Refactor all FRAM files and access algos
// . Fix jerking while write to sfile
// . Optimize SPU SM steps
// . Code reads from DRAM


module sprites(

	input clk, spr_en, line_start, pre_vline,
	
	output reg [4:0] sp_num,	//number of currently processed sprite
	input [63:0] sf_rd,
	
	output reg [7:0] sp_ra,
	input [5:0] sp_rd,
	
	output [20:0] sp_addr = (adr + offs),
	output reg sp_mrq,
	input sp_drdy,
	input [15:0] sp_dat,

	output reg [5:0] spixel,
	output reg spx_en,
	
	output reg test,
	output reg [5:0] mc
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
	
//	reg [4:0] sp_num;	//number of currently processed sprite
//	reg [5:0] mc;	//current state of state-machine
	reg [13:0] offs; //offset from sprite address, words
	reg [6:0] xcnt;
	reg [8:0] sl_wa;
	reg [6:0] sl_wd;
	reg sl_ws, sl_we;
	
// sfile
	wire act = sf_rd[7];
	wire cres = sf_rd[6];
	wire [5:0] pal = sf_rd[5:0];
	wire [20:0] adr = sf_rd[28:8];	//word address!!! 2MB x 16bit
	wire [8:0] xpos = sf_rd[40:32];
	wire [6:0] xsz = sf_rd[47:41];
	wire [8:0] ypos = sf_rd[56:48];
	wire [6:0] ysz = sf_rd[63:57];


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
		mc <= 6'd0;
	else
	case (mc)

	0:	// SPU reset
		//set sprite number to 0
	begin
		test <= 1'b1;
		sp_num <= 5'd0;
		sa_ws <= 1'b0;
		sp_mrq <= 1'b0;
		sl_we <= 1'b0;
		mc <= 6'd1;
	end

	1:	// Begin of sprite[sp_num] processing
	begin
		sp_ra[7:2] <= pal[5:0]; //fix it for 4c and 16c!!!
		sl_wa[8:0] <= xpos;

		//check if sprite is active and visible on this line
		if (act && (vline >= ypos) && (vline < (ypos + ysz)))
		//yes: check if 1st line of sprite then null, else read from sacnt
		begin
			if (ypos == vline)
				offs <= 14'b0;
			else
				offs <= sa_rd;
		//branch for color resolution: 16c (step 12) ? 4c (step 25 - FIX IT !!!)
				mc <= cres ? 6'd12 : 6'd25;
		end
		else
		//no: go to next sprite processing (step 25)
		begin
			mc <= 6'd25;
		end
	end

	12: // Begin of horizontal loop for 16c
		//set sp_addr, assert sp_mrq
	begin
//		sp_addr <= (adr + offs);
		sp_mrq <= 1'b1;
		mc <= 6'd14;
	end

	14: //wait for data from DRAM
		//write pixel-1
	if (sp_drdy)
	begin
		sp_mrq <= 1'b0;
 		sp_ra[3:0] <= sp_dat[15:12];
		sl_wd = {!(sp_dat[15:12] == 4'b0), sp_rd[5:0]};
		sl_we <= 1'b1;
		mc <= 6'd16;
	end

	16: //write pixel-2
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sp_dat[11:8];
		sl_wd = {!(sp_dat[11:8] == 4'b0), sp_rd[5:0]};
		mc <= 6'd18;
	end

	18: //write pixel-3
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sp_dat[7:4];
		sl_wd = {!(sp_dat[7:4] == 4'b0), sp_rd[5:0]};
		mc <= 6'd20;
	end

	20: //write pixel-4
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= sp_dat[3:0];
		sl_wd = {!(sp_dat[3:0] == 4'b0), sp_rd[5:0]};
		mc <= 6'd21;
	end

	21:	//end of write to sline, inc offs, dec sp_xsz
	begin
		sl_wa <= sl_wa + 9'b1;
		sl_we <= 1'b0;
		offs <= offs + 14'b1;
		xcnt <= xcnt - 7'b1;
		mc <= 6'd24;
	end
	
	24:	// End of horizonal loop 16c
	begin
		//check if xsz=0
		if (xcnt == 7'b0)
		//yes: write offs
		begin
			sa_wd <= offs;
			sa_ws <= 1'b1;
			mc <= 6'd25;
		end
		else
		//no: go to begin of 16c loop (step 12)
		begin
			mc <= 6'd12;
		end
	end
	
	25:	//inc sp_num
	begin
		sa_ws <= 1'b0;
		sp_num <= sp_num + 5'b1;
		mc <= 6'd26;
	end

	26:	//check if sp_num=0
	begin
		if (sp_num == 5'b0)
		//yes: halt
		begin
			test <= 1'b0;
			mc <= 6'd63;
		end
		else
		//no: go to begin of sprite processing (step 1)
			mc <= 6'd1;
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


//counters for sprite address
//32 x 14 bit (sprite can occupy max 128 words x 128 lines = 16kwords)
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
                lpm_ram_dp_component.lpm_file = "sfile.mif",
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";


endmodule
