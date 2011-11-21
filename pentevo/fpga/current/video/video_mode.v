// This module decodes video modes


module video_mode (

// video config
	input wire [7:0] vconfig,
	input wire [7:0] vpage,
	
// video parameters
	input  wire [8:0] x_offs,
	output wire [9:0] x_offs_mode,
	output wire [8:0] hpix_beg,
	output wire [8:0] hpix_end,
	output wire [8:0] vpix_beg,
	output wire [8:0] vpix_end,
	output wire [4:0] go_offs,
	output wire [3:0] fetch_sel,
	output wire	[1:0] fetch_bsl,

// video data
	input wire [15:0] txt_char,
    
// video counters
    input wire [7:0] cnt_col,
    input wire [8:0] cnt_row,
    input wire cptr,
	
// mode controls
	output wire hires,
	output wire [1:0] render_mode,
    
// DRAM interface	
    output wire [20:0] video_addr,
    output wire [ 3:0] video_bw
	
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
    
// Render modes (affects 'video_render.v')
    localparam R_ZX = 2'h0;
    localparam R_HC = 2'h1;
    localparam R_XC = 2'h2;
    localparam R_TX = 2'h3;

    
// fetch window
	wire [4:0] g_offs[0:7];
// these values are empiric!!! recheck them occasionally!
    assign g_offs[M_ZX] = 5'd18;
    assign g_offs[M_GS] = 5'd18;
    assign g_offs[M_02] = 5'd10;
    assign g_offs[M_03] = 5'd18;
    assign g_offs[M_04] = 5'd18;
    assign g_offs[M_HC] = 5'd10;
    assign g_offs[M_XC] = 5'd6;
    assign g_offs[M_TX] = 5'd10;
    assign go_offs = g_offs[vmod];


// X offset
	assign x_offs_mode = vmod == M_XC ? {x_offs[8:1], 1'b0, x_offs[0]} : {1'b0, x_offs[8:0]};

	
// DRAM bandwidth usage
    wire [3:0] bw[0:7];
    assign bw[M_ZX] = 4'b1001;	// 1 of 8 (ZX)
    assign bw[M_GS] = 4'b1010;	// 2 of 8 (Giga)
    assign bw[M_02] = 4'b1010;	// 2 of 8 (ZX Hi-res test)
    assign bw[M_03] = 4'b1100;	// 4 of 8 (test)
    assign bw[M_04] = 4'b0000;	// 4 of 4 (test)
    assign bw[M_HC] = 4'b1010;	// 2 of 8 (16c)
    assign bw[M_XC] = 4'b0010;	// 2 of 4 (256c)
    assign bw[M_TX] = 4'b1100;	// 4 of 8 (text)
    assign video_bw = bw[vmod];

	
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
    assign r_mode[M_TX] = R_TX;    
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

 
// ZX
	wire [20:0] addr_zx = {vpage, 1'b0, ~cnt_col[0] ? addr_zx_gfx : addr_zx_atr};
	wire [11:0] addr_zx_gfx = {cnt_row[7:6], cnt_row[2:0], cnt_row[5:3], cnt_col[4:1]};
	wire [11:0] addr_zx_atr = {3'b110, cnt_row[7:3], cnt_col[4:1]};

    
// Gigascreen
    wire [20:0] addr_giga = 0;

    
// 16c
	wire [20:0] addr_16c = {vpage[7:3], cnt_row, cnt_col[6:0]};


// 256c
	wire [20:0] addr_256c = {vpage[7:4], cnt_row, cnt_col[7:0]};


// Textmode
    wire [20:0] addr_text = {vpage[7:1], addr_tx[cnt_col[1:0]]};
	wire [13:0] addr_tx[0:3];
    assign addr_tx[0] = {1'b0, cnt_row[8:3], 1'b0, cnt_col[7:2]};	// char codes, data[15:0]
    assign addr_tx[1] = {1'b0, cnt_row[8:3], 1'b1, cnt_col[7:2]};	// char attributes, data[31:16]
    assign addr_tx[2] = {4'b1000, txt_char[7:0], cnt_row[2:1]};		// char0 graphics, data[7:0]
    assign addr_tx[3] = {4'b1000, txt_char[15:8], cnt_row[2:1]};	// char1 graphics, data[15:8]
    

// fetch selectors
	assign fetch_sel = vmod == M_TX ? f_txt_sel[cnt_col[1:0]] : {~cptr, ~cptr, cptr, cptr};
	assign fetch_bsl = vmod == M_TX ? f_txt_bsl[cnt_col[1:0]] : 2'b10;

// Attention: counter is already incremented at the time of video data fetching!
	wire [3:0] f_txt_sel[0:3];
	assign f_txt_sel[1] = 4'b0011;	// char
	assign f_txt_sel[2] = 4'b1100;	// attr
	assign f_txt_sel[3] = 4'b0001;	// gfx0
	assign f_txt_sel[0] = 4'b0010;	// gfx1
	
	wire [1:0] f_txt_bsl[0:3];
	assign f_txt_bsl[1] = 2'b10;			// char
	assign f_txt_bsl[2] = 2'b10;			// attr
	assign f_txt_bsl[3] = {2{cnt_row[0]}};	// gfx0
	assign f_txt_bsl[0] = {2{cnt_row[0]}};	// gfx1
	

endmodule


