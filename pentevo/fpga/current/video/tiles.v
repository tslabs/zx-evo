`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// Tile Mode
//
// (c)2011 TS-Labs
//

module tiles(

	input fclk,
	output [20:0] t_addr,
	input [15:0] t_data,
	
//Video

	
//XT
	input [7:0] vcfg,
	input [7:0] tp,
	input [7:0] tgp0,
	input [7:0] tgp1,
	input [7:0] tmctrl,
	input [7:0] hsint,
	
//YSTP
	output [8:0] yt_ra,
	input [7:0] yt_rd,

);

//regs - MAY BE reassigned!
	reg [2:0] tpal;		//palette (from tile)
	reg [10:0] tnum;	//tile number (from tile)
	reg xflp;			//hor flip (from tile)
	reg yflp;			//ver flip (from tile)
	reg [3:0] tpix;		//pixel
	reg tpsel;			//tiles plane select
	reg [5:0] tpx;		//tile plane X coordinate
	reg [5:0] tpy;		//tile plane Y coordinate
	reg [2:0] tpl;		//tile plane line
	reg [3:0] tcnt;		//word counter inside the tile
	reg [8:0] xscrl;	//X scroll
	reg [8:0] yscrl;	//Y scroll

//config bits
	wire [1:0] rres;
	wire [2:0] vmode;
	
	assign rres = vcfg[4:3];
	assign tpals = tmctrl[0];
	assign xss = tmctrl[1];
	assign yss = tmctrl[2];
//	assign xsm0 = tmctrl[4];
//	assign xsm1 = tmctrl[5];
//	assign ysm0 = tmctrl[6];
//	assign ysm1 = tmctrl[7];

//logics
	assign tm_en = (vcfg[2:0] == 3'b001) || (vcfg[2:0] == 3'b01x);
	assign tm1_en = (vcfg[2:0] == 3'b011);
	assign tmhr = (vcfg[2:0] == 3'b001);
	assign 
	
//addresses
	wire [20:0] tp_addr;
	wire [20:0] tg_addr;
	wire [7:0] tgp;
	wire [8:0] ys_addr;
	wire [8:0] p_addr;
	
	assign tp_addr = {tp, tpy, tpx, tpsel};			//tile plane DRAM
	assign tg_addr = {tgp, tnum[8:0], tcnt};		//tile gfx DRAM
	assign tgp = (~tpsel ? tgp0 : tgp1) + tnum[10:9];
	assign ys_addr = {1'b0, tpsel, tpx, ys_lh};		//Y scroll FPRAM
	assign p_addr = {1'b1, tpals, tpal, tpix};		//tile palette FPRAM
	
//muxers	
	assign yt_ra = ys_n_p ? ys_addr : p_addr;
	assign t_addr = tp_n_tg ? tp_addr : tg_addr;

	
//TBUF
	wire [8:0] tb_ra, tb_wa;
	wire [7:0] tb_rd;
	wire tb_we;

	tbuf tbuf(	.wraddress(tb_wa), .data(d), .rdaddress(tb_ra), .q(tb_rd), .wrclock(fclk), .wren(tb_we) );
			
endmodule

