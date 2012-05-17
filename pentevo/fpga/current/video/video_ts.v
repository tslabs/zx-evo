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
// clk			|—__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__——__— ~ —__——__——__——__—
// c0-3			|<c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3><c0><c1><c2><c3> ~ <c0><c1><c2><c3>
// layer		|<s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s0><s1><s1><s1><s1><s1> ~ <s1><s1><s1><s2>
// start		|————________________________________________________________________________________________________ ~ ________________
// sprt_end  	|____________________________________________________________________________————____________________ ~ ________————____  sprt_end = (sreg == 7'd126) || (spr_next && s_leap) || (sprt_go && s_leap_r);
// layer_end_r	|xxxx____________________________________________________________________________————________________ ~ ____________————
// sreg			|<00><01><02><03><06><09><0A><0B><0B><0B><0B><0C><0F><10><11><11><11><11><11><12><13><14><14><14><14> ~ <79><7A><7B><7E>
// sf_data		|xxxx<00><01><02><03><06><09><0A><0B><0B><0B><0B><0C><0F><10><11><11><11><11><11><12><13><14><14><14> ~ <78><79><7A><7B>
// sf0_valid	|____————________————————————____________________————————________________________————________________ ~ ————________————
// sf1_valid	|________————________________————________________________————________________________————             ~ ____————________
// sf2_valid	|____________————________________————————————————____________————————————————————________———————————— ~ ________————____
// s_act		|xxxx————xxxxxxxx________————xxxxxxxxxxxxxxxxxxxx____————xxxxxxxxxxxxxxxxxxxxxxxx————xxxxxxxxxxxxxxxx ~ ————xxxxxxxx____
// s_act_r		|xxxxxxxx————————————________————————————————————————____———————————————————————————————————————————— ~ ————————————————  (delete?)
// sreg_next	|xxxx————————————________————————____________————____————————________________————————————____________ ~ ————————————____  sreg_next = sprt_go || (sf0_valid && s_act) || sf1_valid;
// spr_next		|xxxx____________————————________________________————________________________________________________ ~ ____________————
// tsr_rdy		|xxxx————————————____________________________————____________________________————____________________ ~ ________————____
// sprt_go		|____________————____________________________————____________________________————____________________ ~ ________————____  sprt_go = sprites && sf2_valid && tsr_rdy;
// s_leap		|xxxx____xxxxxxxx____________xxxxxxxxxxxxxxxxxxxx____————xxxxxxxxxxxxxxxxxxxxxxxx____xxxxxxxxxxxxxxxx ~ ————xxxxxxxx____
// s_leap_r		|xxxxxxxx________________________________________________————————————————————————————________________ ~ ____————————————


// to do:
// - rethink logics when layers disabled (use s_en/tX_en, Luke)
// - test Xflip
// - code Yflip
// - code Yscroll
// - test Yscrolls array
// - draw DRAM cycles on-screen to determine how and what works
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
    // input wire [7:0] tmpage,
    input wire [7:0] t0gpage,
    input wire [7:0] t1gpage,
    input wire [7:0] sgpage,
	input wire [5:0] num_tiles,
	input wire [8:0] t0x_offs,
	input wire [8:0] t0y_offs,
	input wire [8:0] t1x_offs,
	input wire [8:0] t1y_offs,
	input wire [1:0] t0_palsel,
	input wire [1:0] t1_palsel,

// SFYS interface
	input  wire  [7:0] sfys_addr_in,
	input  wire [15:0] sfys_data_in,
	input  wire sfys_we,

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
	wire [8:0]  s_ycrd	= sfys_data_out[8:0];
	wire [2:0]  s_ysz	= sfys_data_out[11:9];
	wire        s_act	= sfys_data_out[13];
	wire        s_leap	= sfys_data_out[14];
	wire        s_yflp	= sfys_data_out[15];
  // R1
	wire [8:0]  s_xcrd	= sfys_data_out[8:0];
	wire [2:0]  s_xsz	= sfys_data_out[11:9];
	wire        s_xflp	= sfys_data_out[15];
  // R2
	wire [11:0] s_tnum	= sfys_data_out[11:0];
	wire [3:0]	s_pal	= sfys_data_out[15:12];


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
    assign tsr_x = sprites ? sprt_x : tile_x;
    assign tsr_xs = sprites ? sprt_xs : tile_xs;
    assign tsr_xf = sprites ? sprt_xf : tile_xf;
    assign tsr_go = sprites ? sprt_go : tile_go;
    assign tsr_page = sprites ? sprt_page : tile_page;
    assign tsr_line = sprites ? sprt_line : tile_line;
    assign tsr_addr = sprites ? sprt_addr : tile_addr;
    assign tsr_pal = sprites ? sprt_pal : tile_pal;


// --- Tiles ---

    // TSR strobe for tiles
	wire tile_go = tiles && t_act && tmb_valid && tsr_rdy;

    // TSR values for tiles
    wire [7:0] tile_page = tiles0 ? t0gpage : t1gpage;
    wire [8:0] tile_line = {t_tnum[11:6], line[2:0]};	// add here Y scroll and flip!!!
    wire [5:0] tile_addr = t_tnum[5:0];
    wire tile_xf = t_xflp;
    wire [2:0] tile_xs = 3'd0;
    wire [3:0] tile_pal = {tiles0 ? t0_palsel : t1_palsel, t_pal};

	// X processing
    wire t_act = |t_tnum || (tiles0 ? t0z_en : t1z_en);
	wire t_next = tile_go || (!t_act && tmb_valid);
	wire tmb_valid = !layer_new;

    // TMB address
    assign tmb_raddr = {line[4:3], tpos_x, tiles1};
	wire [5:0] tpos_x = layer_new ? tx_offs[8:3] : (tpos_x_r + t_next);	// X position in tilemap
    wire [8:0] tx_offs = tiles0 ? t0x_offs : t1x_offs;

	reg [5:0] tpos_x_r;
	always @(posedge clk)
        tpos_x_r <=  tpos_x;

    // X coordinate
    wire [8:0] tile_x = {tx_r, 3'd0} - tx_offs[2:0];
	wire [5:0] tx = layer_new ? 6'd0 : (tx_r + t_next);	// X coordinate at the screen, higher bits (lower are offset finetune)

	reg [5:0] tx_r;
	always @(posedge clk)
        tx_r <= tx;

	// SFYS addressing
	wire [15:0] yscrl = yscrl_rd_r ? sfys_data_out : sfys_data_out_r;
	wire yscrl_rd = tys_en && (layer_new || t_next);
	wire tys_en = tiles0 ? t0ys_en : t1ys_en;
	
	reg [15:0] sfys_data_out_r;
	always @(posedge clk)
		if (yscrl_rd_r)
			sfys_data_out_r <= sfys_data_out;
	
	reg yscrl_rd_r;
	always @(posedge clk)
		yscrl_rd_r <= yscrl_rd;
		
	// tile layers
	wire tile_start = layer_new && tiles;	// 1st cycle when layer is active
	wire tile_end = (tx == num_tiles);


// --- Sprites ---

    // TSR strobe for sprites
	wire sprt_go = sprites && sf2_valid && tsr_rdy;

	// TSR values for tiles
    wire [8:0] sprt_x = sprt_x_r;
    wire [2:0] sprt_xs = sprt_xs_r;
    wire sprt_xf = sprt_xf_r;
    wire [7:0] sprt_page = sgpage;
    wire [8:0] sprt_line = line;     // debug!!!
    wire [5:0] sprt_addr = s_tnum[5:0];
    wire [3:0] sprt_pal = s_pal;

	// sprite layers
	wire sprt_end = (sreg == 7'd126) || (spr_next && s_leap) || (sprt_go && s_leap_r);

	// SFile regs counter
	wire [6:0] sreg = start ? 7'd0 : sreg_r + (sreg_next ? 7'd1 : (spr_next ? 7'd3 : 7'd0));

	reg [6:0] sreg_r;
	always @(posedge clk)
		sreg_r <= sreg;

	// SFile regs processing
	wire spr_next = sf0_valid && !s_act;
	wire sreg_next = sprt_go || (sf0_valid && s_act) || sf1_valid;

	// SFile regs validity
	wire sf0_valid = !start && !yscrl_rd_r && sf_valid_r[0];
	wire sf1_valid = !start && !yscrl_rd_r && sf_valid_r[1];
	wire sf2_valid = !start && !yscrl_rd_r && sf_valid_r[2];

	reg [2:0] sf_valid_r;
	always @(posedge clk)
		if (start)
			sf_valid_r <= 3'b001;

		else if (sreg_next)
			sf_valid_r <= {sf_valid_r[1:0], sf_valid_r[2]};

	// SFile regs latching
    reg [8:0] sprt_x_r;
    reg [2:0] sprt_xs_r;
    reg sprt_xf_r;
	// reg s_act_r;
	reg s_leap_r;
	always @(posedge clk)
	begin
		if (sf0_valid)
		begin
			// s_act_r <= s_act;
			s_leap_r <= s_leap;
		end

		if (sf1_valid)
		begin
			sprt_x_r <= s_xcrd;
			sprt_xs_r <= s_xsz;
			sprt_xf_r <= s_xflp;
		end
	end


	// sprite geometry
    // wire [8:0] line_of_sprite = line - s_ycrd;
    wire sprite_visible = 1;     // debug!!!
	// wire sprite_visible = ; // here calculate if sprite is visible in the current line
	// wire [6:0] s_ysiz = (s_ysz + 1) * 8;		// Y size in lines


// layers
	wire [2:0] layer = start ? lnxt[BEG] : (layer_end_r ? lnxt[layer_r] : layer_r);
	wire layer_new = layer_end_r || start;
    wire layer_end = (sprites && sprt_end) || (tiles && tile_end);
	wire sprites = (layer == S0) || (layer == S1) || (layer == S2);
    wire tiles = tiles0 || tiles1;
    wire tiles0 = (layer == T0);
    wire tiles1 = (layer == T1);

	reg [2:0] layer_r;
	reg layer_end_r;
	always @(posedge clk)
		begin
			layer_r <= layer;
			layer_end_r <= layer_end;
		end


	localparam BEG = 3'd0;
	localparam S0 = 3'd1;
	localparam T0 = 3'd2;
	localparam S1 = 3'd3;
	localparam T1 = 3'd4;
	localparam S2 = 3'd5;
	localparam END = 3'd6;
	localparam FND = 3'd7;

	wire [2:0] lnxt[0:7];
	assign lnxt[BEG] = s_en ? S0 : t0_en ? T0 : t1_en ? T1 : END;
	assign lnxt[S0]  = t0_en ? T0 : s_en ? S1 : t1_en ? T1 : END;
	assign lnxt[T0]  = s_en ? S1 : t1_en ? T1 : END;
	assign lnxt[S1]  = t1_en ? T1 : s_en ? S2 : END;
	assign lnxt[T1]  = s_en ? S2 : END;
	assign lnxt[S2]  = END;
	assign lnxt[END] = END;
	assign lnxt[FND] = END;


// SFile / YScrolls
	wire [7:0] sfys_addr_out = {yscrl_rd, yscrl_rd ? tpos_x : sreg};
	wire [15:0] sfys_data_out;

video_sf_ys video_sf_ys (
	.clock	    (clk),
	.wraddress	(sfys_addr_in),
	.data		(sfys_data_in),
	.wren		(sfys_we),
	.rdaddress	(sfys_addr_out),
	.q			(sfys_data_out)
);


endmodule
