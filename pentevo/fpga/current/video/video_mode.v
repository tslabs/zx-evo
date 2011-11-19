// This module decodes video modes


module video_mode (

// video config
	input wire [7:0] vconfig,
	input wire [7:0] scr_page,
	
// video parameters
	output wire [8:0] hpix_beg,
	output wire [8:0] hpix_end,
	output wire [8:0] vpix_beg,
	output wire [8:0] vpix_end,
	output wire [8:0] go_beg,
	output wire [8:0] go_end,
    
// video counters
    input wire [8:0] cnt_col,
    input wire [8:0] cnt_row,
	
// mode controls
	output wire hires,
	output wire [1:0] render_mode,
    
// DRAM interface	
    output wire [20:0] video_addr,
    output wire [ 2:0] video_bw_need,
    output wire [ 2:0] video_bw_total
	
);


    wire [2:0] vmod = vconfig[2:0];
	wire [1:0] rres = vconfig[4:3];

// Modes
    localparam M_ZX = 3'h0;		// ZX
    localparam M_GS = 3'h1;		// Giga (ZX*2)
    localparam M_02 = 3'h2;		// ZX hi-res (for test)
    localparam M_03 = 3'h3;		// 
    localparam M_04 = 3'h4;		// 
    localparam M_HC = 3'h5;		// 16c
    localparam M_XC = 3'h6;		// 256c
    localparam M_TX = 3'h7;		// Text
    
// Render modes (affect 'video_render.v')
    localparam R_ZX = 2'h0;
    localparam R_HC = 2'h1;
    localparam R_XC = 2'h2;
    localparam R_03 = 2'h3;

    
// addresses
    wire [20:0] v_addr[0:7];

    assign v_addr[M_ZX] = addr_zx;
    assign v_addr[M_GS] = addr_giga;
    assign v_addr[M_02] = addr_zx;
    assign v_addr[M_03] = addr_zx;
    assign v_addr[M_04] = addr_zx;
    assign v_addr[M_HC] = addr_16c;
    assign v_addr[M_XC] = addr_256c;
    assign v_addr[M_TX] = addr_text;
    
    assign video_addr = v_addr[vmod];

 
// fetch window
	wire [8:0] g_beg[0:7];
	wire [8:0] g_end[0:7];

// !!! check these values !!!    
    assign g_beg[M_ZX] = hpix_beg - 16;
    assign g_beg[M_GS] = hpix_beg - 16;
    assign g_beg[M_02] = hpix_beg - 8;
    assign g_beg[M_03] = hpix_beg - 16;
    assign g_beg[M_04] = hpix_beg - 16;
    assign g_beg[M_HC] = hpix_beg - 8;
    assign g_beg[M_XC] = hpix_beg - 5;
    assign g_beg[M_TX] = hpix_beg - 8;

    assign g_end[M_ZX] = hpix_end - 18;
    assign g_end[M_GS] = hpix_end - 18;
    assign g_end[M_02] = hpix_end - 10;
    assign g_end[M_03] = hpix_end - 18;
    assign g_end[M_04] = hpix_end - 18;
    assign g_end[M_HC] = hpix_end - 10;
    assign g_end[M_XC] = hpix_end - 6;
    assign g_end[M_TX] = hpix_end - 10;

    assign go_beg = g_beg[vmod];
    assign go_end = g_end[vmod];


// DRAM bandwidth usage
    wire [2:0] bwu[0:7];
    wire [2:0] bwt[0:7];
    
    assign bwu[M_ZX] = 3'd1;	// 1 of 8
    assign bwt[M_ZX] = 3'd0;
	
    assign bwu[M_GS] = 3'd2;	// 2 of 8
    assign bwt[M_GS] = 3'd0;
	
    assign bwu[M_02] = 3'd2;	// 2 of 8 (ZX Hi-res test)
    assign bwt[M_02] = 3'd0;
	
    assign bwu[M_03] = 3'd4;	// 4 of 8 (test)
    assign bwt[M_03] = 3'd0;
	
    assign bwu[M_04] = 3'd0;	// 8 of 8 (test)
    assign bwt[M_04] = 3'd0;
	
    assign bwu[M_HC] = 3'd2;	// 2 of 8
    assign bwt[M_HC] = 3'd0;
	
    assign bwu[M_XC] = 3'd2;	// 2 of 4
    assign bwt[M_XC] = 3'd4;
	
    assign bwu[M_TX] = 3'd3;	// 3 of 8
    assign bwt[M_TX] = 3'd0;
    
    assign video_bw_need  = bwu[vmod];
    assign video_bw_total = bwt[vmod];

	
// pixelrate
	wire [7:0] pixrate = 8'b10000100;
	assign hires = pixrate[vmod];

	
// render mode
    wire [1:0] r_mode[0:7];
    
    assign r_mode[M_ZX] = R_ZX;
    assign r_mode[M_GS] = R_ZX;
    assign r_mode[M_02] = R_ZX;
    assign r_mode[M_03] = R_ZX;
    assign r_mode[M_04] = R_ZX;
    assign r_mode[M_HC] = R_HC;
    assign r_mode[M_XC] = R_XC;
    assign r_mode[M_TX] = R_ZX;    
    
	assign render_mode = r_mode[vmod];
	
	
// raster resolution
	wire [8:0] hp_beg[0:3];
	wire [8:0] hp_end[0:3];
	wire [8:0] vp_beg[0:3];
	wire [8:0] vp_end[0:3];

	assign hp_beg[0] = 9'd140;	// 256 (88-52-256-52)
	assign hp_beg[1] = 9'd108;	// 320 (88-20-320-20)
	assign hp_beg[2] = 9'd108;	// 320 (88-20-320-20)
	assign hp_beg[3] = 9'd88;	// 360 (88-0-360-0  )

	assign hp_end[0] = 9'd396;	// 256
	assign hp_end[1] = 9'd428;	// 320
	assign hp_end[2] = 9'd428;	// 320
	assign hp_end[3] = 9'd448;	// 360

	assign vp_beg[0] = 9'd080;	// 192 (32-48-192-32)
	assign vp_beg[1] = 9'd076;	// 200 (32-44-200-44)
	assign vp_beg[2] = 9'd056;	// 240 (32-24-240-24)
	assign vp_beg[3] = 9'd032;	// 288 (32-0-288-0  )

	assign vp_end[0] = 9'd272;	// 192
	assign vp_end[1] = 9'd276;	// 200
	assign vp_end[2] = 9'd296;	// 240
	assign vp_end[3] = 9'd320;	// 288

	assign hpix_beg = hp_beg[rres];
	assign hpix_end = hp_end[rres];
	assign vpix_beg = vp_beg[rres];
	assign vpix_end = vp_end[rres];

	
// ZX
	wire [20:0] addr_zx = cnt_col[0] ? addr_zx_gfx : addr_zx_atr;
	wire [20:0] addr_zx_gfx = {scr_page, 1'b0, cnt_row[7:6], cnt_row[2:0], cnt_row[5:3], cnt_col[4:1]};
	wire [20:0] addr_zx_atr = {scr_page, 4'b0110, cnt_row[7:3], cnt_col[4:1]};

    
// Gigascreen
    wire [20:0] addr_giga = 0;

    
// 16c
	wire [20:0] addr_16c = {scr_page[7:3], cnt_row, cnt_col[6:0]};


// 256c
	wire [20:0] addr_256c = {scr_page[7:4], cnt_row, cnt_col[7:0]};


// Textmode
    wire [20:0] addr_text = 0;
    
    
	
endmodule