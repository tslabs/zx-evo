`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Hummer Ultra Sound
//
// Written by TS-Labs inc.
// ver. 0.0

module	hus(

	input clk, hus_en, li_en,
		
//dram
	output reg [20:0] hus_addr,
	input [15:0] hus_data,
	output reg hus_req,
	input hus_strobe, hus_next,
	
//hfile
	output wire [8:0] hf_ra,
	input [7:0] hf_rd,
	output wire [8:0] hf_hwa,
	output reg hf_hwe,
	
//hvol
	output wire [5:0] hv_ra,
	input [7:0] hv_rd,
	
//DAC
	input dac_stb,
	output reg [15:0] ldac, rdac		//summators for L_AUDIO & R_AUDIO
	
	);
	
	assign hf_ra = {hnum, hfr};
	assign hv_ra = {hnum, lr};
	assign hf_hwa = {hnum, 4'b0};
	
	
//State Machine

	reg [4:0] hnum;		//num of current channel
	reg [3:0] hfr;			//reg of HFILE
	reg [1:0] cfr;			//reg of HCNT
	reg lr;					//reg of HVOL
	reg [4:0] mc;
	reg [1:0] lmode;		//current loop mode
	reg a0, reload;
	
	localparam mc_beg = 5'd0;
	localparam mc_out = 5'd1;
	localparam mc_s2 = 5'd2;
	localparam mc_s3 = 5'd3;
	localparam mc_s4 = 5'd4;
	localparam mc_s5 = 5'd5;
	localparam mc_s6 = 5'd6;
	localparam mc_s7 = 5'd7;
	localparam mc_s8 = 5'd8;
	localparam mc_s9 = 5'd9;
	localparam mc_halt = 5'd31;
	
	localparam r_ctr = 4'd0;
	localparam r_sl = 4'd1;
	localparam r_sm = 4'd2;
	localparam r_sh = 4'd3;
	localparam r_el = 4'd4;
	localparam r_em = 4'd5;
	localparam r_eh = 4'd6;
	localparam r_lsl = 4'd7;
	localparam r_lsm = 4'd8;
	localparam r_lsh = 4'd9;
	localparam r_lel = 4'd10;
	localparam r_lem = 4'd11;
	localparam r_leh = 4'd12;
	localparam r_ifl = 4'd13;
	localparam r_ifh = 4'd14;
	localparam r_ii = 4'd15;
	
	localparam c_as = 2'd0;
	localparam c_al = 2'd1;
	localparam c_ah = 2'd2;
	localparam c_ii = 2'd3;

	wire [4:0] hnumext;
	wire dac_last;
	wire act;
	
	assign act = hf_rd[7];
	assign hnumext = hnum + 5'b1;
	assign dac_last = (hnumext == 5'b0);
	
always @(posedge clk)
	if (dac_stb)
	begin
//initialization of HUS vars by reset
		hnum <= 0;			//number of channel <= 0
		ldac <= 13'd0;		//DAC summators <= 0
		rdac <= 13'd0;
		//HFILE
		hfr <= r_ctr;
		//HCNT
		cfr <= c_al;
		//FSM
		mc = mc_beg;
	end
	else
	case (mc)

mc_beg:	
	begin
		if (act)
		//channel is active
		begin
			//HFILE
			reload <= hf_rd[6];
			lmode <= hf_rd[1:0];
			hfr <= r_sl;
			//HCNT
			hus_addr[14:0] <= hc_rd[15:1];
			a0 <= hc_rd[0];
			cfr <= c_ah;
			//FSM
			mc <= mc_s2;
		end
		else
		//channel is inactive
		if (!dac_last)
		//there are also channels to process
		begin
			hnum <= hnumext;
			mc = mc_beg;
		end
		else
		//no more channels
			mc <= mc_out;
	end

mc_out:
	begin
	end
	
mc_halt:
	mc <= mc_halt;

default:	
	mc <= mc_halt;

	endcase
	

//HCNT
	wire [15:0] hc_rd, hc_wd;
	wire hc_we;

	hcnt hcnt(	.wraddress({hnum, cfr}), .data(hc_wd), .rdaddress({hnum, cfr}), .q(hc_rd), .wrclock(fclk), .wren(hc_we) );

endmodule


