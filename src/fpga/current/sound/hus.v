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
	output wire [20:0] hus_addr,
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
	
	//FSM states
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
	localparam mc_ms_l = 5'd22;
	localparam mc_ms_r = 5'd23;
	localparam mc_mi_l = 5'd24;
	localparam mc_mi_r = 5'd25;
	localparam mc_mv_l = 5'd26;
	localparam mc_mv_r = 5'd27;
	localparam mc_mem = 5'd28;
	localparam mc_mem2 = 5'd29;
	localparam mc_end = 5'd30;
	localparam mc_halt = 5'd31;
	
	//HFILE regs
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
	
	//HCNT regs
	localparam c_asl = 3'd0;
	localparam c_ash = 3'd1;
	localparam c_al = 3'd2;
	localparam c_am = 3'd3;
	localparam c_ah = 3'd4;
	localparam c_ii = 3'd6;
	localparam c_dp = 3'd7;

	assign hf_ra = {hnum, hfr};
	assign hv_ra = {hnum, lr};
	assign hf_hwa = {hnum, 4'b0};
	assign hus_addr = adr[21:1];
	
	reg [23:0] adr;
	reg [15:0] sadr;
	reg [7:0] samp;
	reg [4:0] hnum;			//num of current channel
	reg [3:0] hfr;			//reg of HFILE
	reg [2:0] cfr;			//reg of HCNT
	reg lr;					//reg of HVOL
	reg [4:0] mc;
	reg [1:0] lmode;		//current loop mode
	reg reload;
	
	wire [4:0] hnumext;
	wire dac_last;
	wire act;
	wire [7:0] adrc;
	wire [8:0] mul1;
	wire [7:0] mul2;
	wire [16:0] mul;
	wire mms, mmi, mmv;
	
	assign mul = mul1 * mul2;
	assign mms = ((mc == mc_ms_l) | (mc == mc_ms_r));
	assign mmi = ((mc == mc_mi_l) | (mc == mc_mi_r));
	assign mmv = ((mc == mc_mv_l) | (mc == mc_mv_r));
	assign mul1 = mmv ? {hv_rd, 1'b0} : (mmi ? {1'b0, hc_rd} : (9'd256 - hc_rd));
	assign mul2 = samp;
	assign act = hf_rd[7];
	assign hnum_next = hnum + 5'b1;
	assign dac_last = (hnum_next == 5'b0);
	assign adrc = reload ? hc_rd[7:0] : hf_rd[7:0];
	

always @(posedge clk)
	if (dac_stb)
	begin
//initialization of HUS vars on reset
		hnum <= 5'b0;			//number of channel <= 0
		ldac <= 16'd0;		//DAC summators <= 0
		rdac <= 16'd0;
		//HFILE
		hfr <= r_ctr;
		cfr <= c_asl;
		//FSM
		mc = hus_en ? mc_beg : mc_halt;		//if HUS disabled - go to HALT
	end
	else
	case (mc)

mc_beg:	
	begin
		if (act)
		//channel is active
		begin
			reload <= hf_rd[6];			//get RELOAD
			lmode <= hf_rd[1:0];		//get LMODE
			sadr[7:0] <= hc_rd[7:0];	//get SADR [7:0]
			hf_hwe <= 1'b1;				//strobe to null RELOAD
			cfr <= c_ash;
			mc <= mc_s2;
		end
		else
		//channel is inactive
		if (!dac_last)
		//there are also channels to process
		begin
			hnum <= hnum_next;
			mc = mc_beg;
		end
		else
		//no more channels
			mc <= mc_out;
	end

mc_s2:
	begin
		sadr[15:8] <= hc_rd[7:0];	//get SADR [15:8]
		hf_hwe <= 1'b0;
		hfr <= r_sl;
		cfr <= c_al;
		mc <= mc_s3;
	end
	
mc_s3:
	begin
		adr [7:0] <= adrc;		//get ADR[7:0]
		hfr <= r_sm;
		cfr <= c_am;
		mc <= mc_s4;
	end
	
mc_s4:
	begin
		adr [15:8] <= adrc;		//get ADR[15:8]
		hfr <= r_sh;
		cfr <= c_ah;
		mc <= mc_s5;
	end
	
mc_s5:
	begin
		adr [23:16] <= adrc;	//get ADR[23:16]
		hus_req <= 1'b1;
		mc <= mc_mem;
	end

mc_mem:
	begin
		if (!hus_next)
			mc <= mc_mem;
		else
			mc <= mc_mem2;
mc_mem2:
	begin
		samp <= adr[0] ? hus_data[15:8] : hus_data[7:0];
	end

mc_end:
	if (!dac_last)
	//there are also channels to process
	begin
		hnum <= hnum_next;
		hfr <= r_ctr;
		cfr <= c_asl;
		mc = mc_beg;
	end
	else
	//no more channels
	mc <= mc_out;

mc_out:
	begin
	end
	
mc_halt:
	mc <= mc_halt;

default:	
	mc <= mc_halt;

	endcase
	

//HCNT
	wire [7:0] hc_rd, hc_wd;
	wire hc_we;

	hcnt hcnt(	.wraddress({hnum, cfr}), .data(hc_wd), .rdaddress({hnum, cfr}), .q(hc_rd), .wrclock(fclk), .wren(hc_we) );

endmodule


