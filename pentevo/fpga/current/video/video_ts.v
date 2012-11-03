`include "../include/tune.v"

// This is the Tile Sprite Processing Unit

// Tiles map address:
//  bits    desc
//  20:13   tmpage
//  12:

// Graphics address:
//  bits    desc
//  20:13   Xgpage
//  15:7    line (bits 15:13 are added) - 512 lines
//  6:0     word within line - 128 words = 512 pixels

// - tiles - (256x192, offset0 = 128, offset1 = 0)
// clk			|—__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__— ~ —__——__——__——__——__——__——__——__——__——__——__——__—
// c0-3			|<c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3> ~ <c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3>
// layer		|<t0><t0><t0><t0><t0><t0><t0><t0><t0><t0><t0><t0><t0><t0><t0><t0><t0> ~ <t0><t0><t0><t1><t1><t1><t1><t1><t1><t1><t1><t1>
// start		|————________________________________________________________________ ~ ________________________________________________
// tile_end  	|____________________________________________________________________ ~ ________————____________________________________
// layer_end_r	|xxxx________________________________________________________________ ~ ____________————________________________________
// tile_start	|————________________________________________________________________ ~ ____________————________________________________
// tmb_valid	|____———————————————————————————————————————————————————————————————— ~ ————————————____————————————————————————————————
// tsr_rdy		|xxxx————____________________________________————____________________ ~ ________————____________________________————____
// yscrl_rd		|————————————————————————____________________————————________________ ~ ————————————————————____________________————____
// tile_go		|____————____________________________________————____________________ ~ ________————____________________________————____
// tpos_x		|<10><11><12><13><14><15><15><15><15><15><15><16><17><17><17><17><17> ~ <2F><30><31><00><01><01><01><01><01><01><02><02>
// tmb_raddr	|<10><11><12><13><14><15><15><15><15><15><15><16><17><17><17><17><17> ~ <2F><30><31><00><01><01><01><01><01><01><02><02>
// tpos_x_r		|xxxx<10><11><12><13><14><15><15><15><15><15><15><16><17><17><17><17> ~ <2E><2F><30><30><00><01><01><01><01><01><01><02>
// tmb_rdata	|xxxx<10><11><12><13><14><15><15><15><15><15><15><16><17><17><17><17> ~ <2E><2F><30><30><00><01><01><01><01><01><01><02>
// t_act		|xxxx————________________————————————————————————____———————————————— ~ ________————————____————————————————————————————
// t_next		|____————————————————————____________________————————________________ ~ ————————————____————____________________————____
// tx			|<00><01><02><03><04><05><05><05><05><05><05><06><07><07><07><07><07> ~ <1F><20><21><00><01><01><01><01><01><01><02><02>
// tx_r			|xxxx<00><01><02><03><04><05><05><05><05><05><05><06><07><07><07><07> ~ <1E><1F><20><20><00><01><01><01><01><01><01><02>
// tile_end		|xxxx________________________________________________________________ ~ ________————____________________________________
// tiles0		|xxxx———————————————————————————————————————————————————————————————— ~ ————————————____________________________________
// tiles1		|xxxx________________________________________________________________ ~ ____________————————————————————————————————————
//				|

// - sprites -
// clk			|—__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__— ~ —__——__——__——__——__—
// c0-3			|<c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3> ~ <c0><c1><c2><c3><c3>
// layer		|<s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s1><s1><s1><s1><s1> ~ <s1><s1><s1><s2><s2>
// start		|————________________________________________________________________________________________________ ~ ____________________
// sprites_end  |____________________________________________________————____________________________________________ ~ ____________————————  = sprites_done || (spr_skip && s_leap) || (sprites_go && s_leap_r)
// sprites_done |____________________________________________________________________________________________________ ~ ____________————————  = sreg == 7'd126
// sprites_done_r 	|xxxx________________________________________________________________________________________________ ~ ________________————
// layer_end_r	|xxxx____________________________________________________————________________________________________ ~ ____________————————
// sreg			|<00><01><02><03><06><09><0A><0B><0B><0B><0B><0C><0F><10><11><11><11><11><11><12><13><14><14><14><14> ~ <79><7A><7B><7E><7E>
// sf_data		|xxxx<00><01><02><03><06><09><0A><0B><0B><0B><0B><0C><0F><10><11><11><11><11><11><12><13><14><14><14> ~ <78><79><7A><7B><7E>
// sreg*		|xxxx<00><01><02><00><00><00><01><02><02><02><02><00><00><01><02><02><02><02><02><00><01><02><02><02> ~ <00><01><02><00><00>
// sf0_valid	|____————________————————————____________________————————________________________————________________ ~ ————________________
// sf1_valid	|________————________________————________________________————________________________————             ~ ____————____________
// sf2_valid	|____________————________________————————————————____________————————————————————________———————————— ~ ________————________
// s_visible	|xxxx————xxxxxxxx________————xxxxxxxxxxxxxxxxxxxx____————xxxxxxxxxxxxxxxxxxxxxxxx————xxxxxxxxxxxxxxxx ~ ————xxxxxxxx________
// sreg_next	|xxxx————————————________————————____________————____————————________________————————————____________ ~ ————————————________  = sprites_go || (sf0_valid && s_act) || sf1_valid
// spr_skip		|xxxx____________————————________________________————________________________________________________ ~ ____________________  = sf0_valid && !s_act
// tsr_rdy		|xxxx————————————____________________________————____________________________————____________________ ~ ________————________
// sprites_go	|____________————____________________________————____________________________————____________________ ~ ________————________  = sprites && sf2_valid && tsr_rdy
// s_leap		|xxxx____xxxxxxxx____________xxxxxxxxxxxxxxxxxxxx____————xxxxxxxxxxxxxxxxxxxxxxxx____xxxxxxxxxxxxxxxx ~ ————xxxxxxxxxxxxxxxx
// s_leap_r		|xxxxxxxx________________________________________________————————————————————————————________________ ~ ____————————————————


// to do:
// - fix Yscrolls array
// - draw DRAM cycles on-screen to determine how and what works
// - code a/r Ys for tiles
// - write global test for TS
// - DMA for Sfile


module video_ts (

// clocks
	input wire clk,

// video controls
	input wire start,
	input wire [8:0] line,      // 	= vcount - vpix_beg + 9'b1;

// video config
	input wire [7:0] tsconf,
    input wire [7:0] t0gpage,
    input wire [7:0] t1gpage,
    input wire [7:0] sgpage,
	input wire [5:0] num_tiles,
	input wire [8:0] t0x_offs,
	input wire [8:0] t1x_offs,
	input wire [2:0] t0y_offs,
	input wire [2:0] t1y_offs,
	input wire [1:0] t0_palsel,
	input wire [1:0] t1_palsel,

// SFYS interface
	input  wire  [7:0] sfile_addr_in,
	input  wire [15:0] sfile_data_in,
	input  wire sfile_we,
	input  wire [6:0] tys_data_x,
	input  wire tys_data_s,
	output wire [5:0] tys_data_f,

// TMbuf interface
	output wire [8:0] tmb_raddr,
	input wire [15:0] tmb_rdata,

// renderer interface
	output wire tsr_go,
	output wire [5:0] tsr_addr,     // graphics address within the line
    output wire [8:0] tsr_line,     // bitmap line
    output wire [7:0] tsr_page,     // bitmap 1st page
    output wire [8:0] tsr_x,        // addr in buffer (0-359 visibles)
    output wire [2:0] tsr_xs,       // size (8-64 pix)
    output wire tsr_xf,             // X flip
    output wire [3:0] tsr_pal,		// palette
	input wire tsr_rdy              // renderer is done and ready to receive a new task

);


// sprite descriptor fields
  // R0
	wire [8:0]  s_ycrd	= sfile_data_out[8:0];
	wire [2:0]  s_ysz	= sfile_data_out[11:9];
	wire        s_act	= sfile_data_out[13];
	wire        s_leap	= sfile_data_out[14];
	wire        s_yflp	= sfile_data_out[15];
  // R1
	wire [8:0]  s_xcrd	= sfile_data_out[8:0];
	wire [2:0]  s_xsz	= sfile_data_out[11:9];
	wire        s_xflp	= sfile_data_out[15];
  // R2
	wire [11:0] s_tnum	= sfile_data_out[11:0];
	wire [3:0]	s_pal	= sfile_data_out[15:12];


// tile descriptor fields
	wire [11:0] t_tnum	= tmb_rdata[11:0];
	wire [1:0]	t_pal	= tmb_rdata[13:12];
	wire        t_xflp	= tmb_rdata[14];
	wire        t_yflp	= tmb_rdata[15];


// config
	wire s_en = tsconf[7];
	wire t1_en = tsconf[6];
	wire t0_en = tsconf[5];
	wire t1z_en = tsconf[3];
	wire t0z_en = tsconf[2];
	wire t1ys_en = tsconf[1];
	wire t0ys_en = tsconf[0];


// TS renderer interface
    assign tsr_x = sprites ? sprites_x : tile_x;
    assign tsr_xs = sprites ? sprites_xs : tile_xs;
    assign tsr_xf = sprites ? sprites_xf : tile_xf;
    assign tsr_go = sprites ? sprites_go : tile_go;
    assign tsr_page = sprites ? sprites_page : tile_page;
    assign tsr_line = sprites ? sprites_line : tile_line;
    assign tsr_addr = sprites ? sprites_addr : tile_addr;
    assign tsr_pal = sprites ? sprites_pal : tile_pal;


// --- Tiles ---

    // TSR strobe for tiles
	wire tile_go = tiles && t_act && tmb_valid && tsr_rdy;

    // TSR values for tiles
    wire [7:0] tile_page = tiles0 ? t0gpage : t1gpage;
    wire [8:0] tile_line = {t_tnum[11:6], (t_line[2:0] ^ {3{t_yflp}})};
    wire [5:0] tile_addr = t_tnum[5:0];
    wire tile_xf = t_xflp;
    wire [2:0] tile_xs = 3'd0;
    wire [3:0] tile_pal = {tiles0 ? t0_palsel : t1_palsel, t_pal};

	// tile layers
	wire tile_start = layer_new && tiles;		// 1st cycle when tile layer is active
	// wire tile_end = (tiles0 && !t0_en) || (tiles1 && !t1_en)  || (tx == num_tiles);
	wire tile_done = tile_end && tiles1;

	reg tile_done_r;
	always @(posedge clk)
		if (start)
			tile_done_r <= 1'b0;
		else if (tile_done)
			tile_done_r <= 1'b1;

	// tile X processing
    wire t_act = |t_tnum || (tiles0 ? t0z_en : t1z_en);
	wire t_next = tile_go || (!t_act && tmb_valid);
	wire tmb_valid = !layer_new && !tile_done_r;

    // TMB address
    assign tmb_raddr = {t_line[4:3], tpos_x, tiles1};
	wire [5:0] tpos_x = layer_new ? tx_offs[8:3] : (tpos_x_r + t_next);		// X position in tilemap
    wire [8:0] tx_offs = tiles0 ? t0x_offs : t1x_offs;

	reg [5:0] tpos_x_r;
	always @(posedge clk)
        tpos_x_r <= tpos_x;

    // tile X coordinate
    wire [8:0] tile_x = {tx_r, 3'd0} - tx_offs[2:0];
	wire [5:0] tx = layer_new ? 6'd0 : (tx_r + t_next);		// X coordinate at the screen, higher bits (lower are offset finetune)

	reg [5:0] tx_r;
	always @(posedge clk)
        tx_r <= tx;

	// tile Y geometry
	wire [8:0] t_line = line + ty_offset;

	// tile Y scrollers
	wire [2:0] ty_offset = tys_en ? tys_data[2:0] : ty_offs;
	wire [2:0] ty_offs = tiles0 ? t0y_offs : t1y_offs;

	// SFYS addressing
	wire [2:0] tys_data = yscrl_rd_r ? sfile_data_out[2:0] : tys_data_r;
	wire yscrl_rd = tys_en && (tile_start || t_next);
	wire tys_en = tiles0 ? t0ys_en : t1ys_en;

	reg [2:0] tys_data_r;
	always @(posedge clk)
		if (yscrl_rd_r)
			tys_data_r <= sfile_data_out[2:0];

	reg yscrl_rd_r;
	always @(posedge clk)
		yscrl_rd_r <= yscrl_rd || tys_data_s;


// --- Sprites ---

    // TSR strobe for sprites
	wire sprites_go = sprites && sf2_valid && tsr_rdy;		// kick to renderer

	// TSR values for sprites
    wire [8:0] sprites_x = sprites_x_r;
    wire [2:0] sprites_xs = sprites_xs_r;
    wire sprites_xf = sprites_xf_r;
    wire [7:0] sprites_page = sgpage;
    wire [8:0] sprites_line = s_bmline;
    wire [5:0] sprites_addr = s_tnum[5:0];
    wire [3:0] sprites_pal = s_pal;

	// sprite layers control
	wire sprites_end = sprites && (sprites_done || (spr_skip && s_leap) || (sprites_go && s_leap_r));	// last tact of sprites layer
	wire sprites_done = sreg == 8'd254;		// the last sprite processing is finished

	reg sprites_done_r;
	always @(posedge clk)
		sprites_done_r <= sprites_done;

	// SFile registers control
	wire [7:0] sreg = start ? 8'd0 : sreg_r + (sreg_next ? 8'd1 : (spr_skip ? 8'd3 : 8'd0));
	
	wire sreg_next = sprites_go || (sf0_valid && s_visible) || sf1_valid;
	wire spr_skip = sf0_valid && !s_visible;

	reg [7:0] sreg_r;
	always @(posedge clk)
		sreg_r <= sreg;
	
	wire sf0_valid = !start && sf_valid_r[0] && !sprites_done_r;
	wire sf1_valid = !start && sf_valid_r[1];
	wire sf2_valid = !start && sf_valid_r[2];
	
	reg [2:0] sf_valid_r;
	always @(posedge clk)
		if (start)
			sf_valid_r <= 3'b001;
		else if (sreg_next)
			sf_valid_r <= {sf_valid_r[1:0], sf_valid_r[2]};
			
	// SFile regs latching
    reg [8:0] sprites_x_r;
    reg [5:0] s_bmline_offset_r;
    reg [2:0] sprites_xs_r;
    reg sprites_xf_r;
	reg s_leap_r;
	
	always @(posedge clk)
	begin
		if (sf0_valid)
		begin
			s_leap_r <= s_leap;
			s_bmline_offset_r <= s_bmline_offset;
		end

		if (sf1_valid)
		begin
			sprites_x_r <= s_xcrd;
			sprites_xs_r <= s_xsz;
			sprites_xf_r <= s_xflp;
		end
	end

	// sprite Y geometry
    wire [8:0] s_line = line - s_ycrd;				// visible line of sprite in current video line
    wire s_visible = (s_line <= s_ymax) && s_act;	// check if sprite line is within Y size and sprite is active
	wire [5:0] s_ymax = {s_ysz, 3'b111};
	
	wire [8:0] s_bmline = {s_tnum[11:6], 3'b0} + s_bmline_offset_r;
	wire [5:0] s_bmline_offset = s_yflp ? (s_ymax - s_line[5:0]) : s_line[5:0];


// Control of layer selectors
    wire layer_end = sprites_skip || tiles_skip || sprites_end || tile_end;		// last cycle of any layer
	wire layer_new = layer_end_r || start;								// first cycle of any layer
	wire sprites = (layer[0] || layer[2] || layer[4]) && s_en;			// sprite layer is active: layer selected and sprites enabled
	wire sprites_skip = (layer[0] || layer[2] || layer[4]) && !s_en;	// sprite layer must be skipped: layer selected and sprites disabled
	// wire tiles0 = layer[1] && t0_en;									// tile0 layer is active: layer selected and enabled
	// wire tiles1 = layer[3] && t1_en;									// tile1 layer is active: layer selected and enabled
	wire tiles0 = layer[1];									// tile0 layer is active: layer selected and enabled
	wire tiles1 = layer[3];									// tile1 layer is active: layer selected and enabled
	wire tile_end = tiles;
    wire tiles = tiles0 || tiles1;
	wire tiles_skip = (layer[1] && !t0_en) || (layer[3] && !t1_en);		// tile layer must be skipped: layer selected and disabled

	reg [4:0] layer;
	always @(posedge clk)
		if (start)
			layer <= 5'b00001;		// at the begin the sprites0 layer is selected
		else if (layer_end)
			layer <= {layer[4:0], 1'b0};	// when layer finished, move to next

	reg layer_end_r;
	always @(posedge clk)
		layer_end_r <= layer_end;


// SFile
	// wire [7:0] sfile_addr_out = {(yscrl_rd || tys_data_s), tys_data_s ? tys_data_x : (yscrl_rd ? {tiles1, tpos_x} : sreg)};
	wire [7:0] sfile_addr_out = sreg;
	wire [15:0] sfile_data_out;
	assign tys_data_f = sfile_data_out[8:3];

video_sfile video_sfile (
	.clock	    (clk),
	.wraddress	(sfile_addr_in),
	.data		(sfile_data_in),
	.wren		(sfile_we),
	.rdaddress	(sfile_addr_out),
	.q			(sfile_data_out)
);


endmodule
