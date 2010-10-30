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
	output reg test,
	output reg [5:0] mc,

//sfile	
	output reg [7:0] sf_ra,
	input [7:0] sf_rd,

//spram
	output reg [7:0] sp_ra,
	input [5:0] sp_rd,
	
//dram
	output reg [20:0] spr_addr,
	input [15:0] spr_dat,
	output reg spr_mrq,
	input spr_drdy,

//video
	output reg [5:0] spixel,
	output reg spx_en

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
	
	reg [4:0] num;		//number of currently processed sprite
//	reg [5:0] mc;		//current state of state-machine
	reg [13:0] offs;	//offset from sprite address, words

// sfile
	reg cres, en;
	reg [20:0] adr;	//word address!!! 2MB x 16bit
	reg [6:0] xsz;
	reg [8:0] ypos;

	
// *** DIE MASCHINE ***
// JA, JA! DAS IST FANTASTISCH !!!
	
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
	begin
		test <= 1'b1;
		sa_ws <= 1'b0;
		spr_mrq <= 1'b0;
		sl_we <= 1'b0;

		num <= 5'd0;			//set sprite number to 0
		sf_ra <= 8'd4;			//set addr for reg4
		mc <= 6'd2;
	end

	2:	// Begin of sprite[num] processing
	begin
		//check if sprite is active
		if (sf_rd[7])
		//yes:
		begin
			cres <= sf_rd[6];			//read CRES
			sp_ra[7:2] <= sf_rd[5:0];	//read PAL[5:0]
			sf_ra[2:0] <= 3'd2;			//set addr for reg2
			mc <= 6'd3;
		end

		else

		//no:
		begin
			if ((num + 5'b1) == 5'b0)	//check if all 32 sprites done
		//yes: halt
			begin
				test <= 1'b0;
				mc <= 6'd63;
			end

			else

		//no: next sprite processing
			begin
			num <= num + 5'b1;
			sf_ra <= {(num + 5'b1), 3'd4};	//inc spnum, set addr for reg4
			end
		end
	end

	3:
	begin
		ypos[7:0] <= sf_rd[7:0];	//read YPOS[7:0]
		sf_ra[2:0] <= 3'd3;			//set addr for reg3
		mc <= 6'd4;
	end

	4:
	begin
		ypos[8] <= sf_rd[0];
		//check if sprite is visible on this line
		if ((vline >= {sf_rd[0], ypos[7:0]}) && (vline < ({sf_rd[0], ypos[7:0]} + sf_rd[7:1])))
		//yes
		begin
			sf_ra[2:0] <= 3'd5;			//set addr for reg5
			mc <= 6'd5;
		end
		else
		//no
		begin
			if ((num + 5'b1) == 5'b0)	//check if all 32 sprites done
			//yes: halt
			begin
				test <= 1'b0;
				mc <= 6'd63;
			end
			else
			//no: next sprite processing
			begin
				num <= num + 5'b1;
				sf_ra <= {(num + 5'b1), 3'd4};	//inc spnum, set addr for reg4
				mc <= 6'd2;
			end
		end
	end

	5: 
	begin
		adr[7:0] <= sf_rd[7:0];		//read ADR[7:0]
		sf_ra[2:0] <= 3'd6;			//set addr for reg6

		if (ypos == vline)			//check if 1st line of sprite
			offs <= 14'b0;			//yes: null offs
		else
			offs <= sa_rd;			//no: read offs

		mc <= 6'd6;
	end

	6:
	begin
		adr[15:8] <= sf_rd[7:0];	//read ADR[15:8]
		sf_ra[2:0] <= 3'd7;			//set addr for reg7
		mc <= 6'd7;
	end

	7:
	begin
		adr[20:16] <= sf_rd[4:0];	//read ADR[20:16]
		sf_ra[2:0] <= 3'd0;			//set addr for reg0
		mc <= 6'd8;
	end

	8:
	begin
		sl_wa[7:0] <= sf_rd[7:0];	//read XPOS[7:0]
		sf_ra[2:0] <= 3'd1;			//set addr for reg1
		mc <= 6'd9;
	end

	9:
	begin
		sl_wa[8] <= sf_rd[0];		//read XPOS[8]
		xsz[6:0] <= sf_rd[7:1];		//read XSZ[6:0]
	
	if (!cres)
		mc <= 6'd12;	//use 16c
	else
		mc <= 6'd25;	//use 4c
	end
	
	12: // Begin of horizontal loop for 16c
		//set spr_addr, assert spr_mrq
	begin
		spr_addr <= (adr + offs);
		spr_mrq <= 1'b1;
		mc <= 6'd14;
	end

	14: //wait for data from DRAM
	if (spr_drdy)
		//write pix0
	begin
		spr_mrq <= 1'b0;
 		sp_ra[3:0] <= spr_dat[15:12];		//set paladdr for pix0
		en <= !(spr_dat[15:12] == 4'b0);	//set transparency for pix0
		sl_we <= 1'b1;
		mc <= 6'd16;
	end

	16: //write pix1
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= spr_dat[11:8];		//set paladdr for pix1
		en <= !(spr_dat[11:8] == 4'b0);	//set transparency for pix1
		mc <= 6'd18;
	end

	18: //write pix2
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= spr_dat[7:4];		//set paladdr for pix2
		en <= !(spr_dat[7:4] == 4'b0);	//set transparency for pix2
		mc <= 6'd20;
	end

	20: //write pix3
	begin
		sl_wa <= sl_wa + 9'b1;
		sp_ra[3:0] <= spr_dat[3:0];		//set paladdr for pix3
		en <= !(spr_dat[3:0] == 4'b0);	//set transparency for pix3
		mc <= 6'd21;
	end

	21:	//end of write to sline, inc offs, dec xsz
	begin
		sl_we <= 1'b0;
		sl_wa <= sl_wa + 9'b1;
		offs <= offs + 14'b1;
		xsz <= xsz - 7'b1;
		mc <= 6'd24;
	end
	
	24:	// End of horizonal loop 16c
	begin
		//check if xsz=0
		if (xsz == 7'b0)
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
	
	25:
	begin
		if ((num + 5'b1) == 5'b0)	//check if all 32 sprites done
	//yes: halt
		begin
			test <= 1'b0;
			mc <= 6'd63;
		end
		else
	//no: next sprite processing
		begin
			num <= num + 5'b1;
			sf_ra <= {(num + 5'b1), 3'd4};	//inc spnum, set addr for reg4
			mc <= 6'd2;
		end
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


	sline0 sline0(	.wraddress(l_sel ? sl_wa : sl_ra),
					.data(l_sel ? {en, sp_rd[5:0]} : 7'b0),
					.wren(l_sel ? sl_ws : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd0)
				);

	sline1 sline1(	.wraddress(!l_sel ? sl_wa : sl_ra),
					.data(!l_sel ? {en, sp_rd[5:0]} : 7'b0),
					.wren(!l_sel ? sl_ws : sl_wsn),
					.rdaddress(sl_ra),
					.q(sl_rd1)
				);

	wire [13:0] sa_rd;
	reg [13:0] sa_wd;
	reg sa_ws;

	sacnt sacnt(	.wraddress(num),
					.data(sa_wd),
					.wren(sa_ws),
					.rdaddress(num),
					.q(sa_rd)
				);
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
                lpm_ram_dp_component.lpm_file = "sfile.mif",
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
//                lpm_ram_dp_component.lpm_file = "SLINE.MIF",
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
                lpm_ram_dp_component.lpm_file = "sline.mif",
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";


endmodule


//sprite descripting array
module sfile (
        data,
        wraddress,
        wren,
        rdaddress,
        q);

        input   [7:0]  data;
        input   [7:0]  rdaddress;
        input   [7:0]  wraddress;
        input     wren;
        output  [7:0]  q;

        wire [7:0] sub_wire0;
        wire [7:0] q = sub_wire0[7:0];

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
                lpm_ram_dp_component.lpm_width = 8,
                lpm_ram_dp_component.lpm_widthad = 8,
                lpm_ram_dp_component.lpm_numwords = 256,
                lpm_ram_dp_component.lpm_file = "sfile.mif",
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";


endmodule


//palette SRAM for sprites
module spram (
        data,
        wraddress,
        wren,
        rdaddress,
        q);

        input   [5:0]  data;
        input   [7:0]  rdaddress;
        input   [7:0]  wraddress;
        input     wren;
        output  [5:0]  q;

        wire [5:0] sub_wire0;
        wire [5:0] q = sub_wire0[5:0];

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
                lpm_ram_dp_component.lpm_width = 6,
                lpm_ram_dp_component.lpm_widthad = 8,
                lpm_ram_dp_component.lpm_numwords = 256,
                lpm_ram_dp_component.lpm_file = "spram.mif",
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";


endmodule
