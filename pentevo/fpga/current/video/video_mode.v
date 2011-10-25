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
    
// DRAM interface	
    output wire [20:0] video_addr,
    output wire [ 1:0] video_bw
	
);

// Modes:
// 0 - ZX
// 1 - Giga
// 2 - 
// 3 - 
// 4 - 
// 5 - (ZX hi-res)
// 6 - 256c
// 7 - Text


    wire [2:0] vmod = vconfig[2:0];
	wire [1:0] rres = vconfig[4:3];

// addresses
    wire [20:0] v_addr[0:7];

    assign v_addr[0] = addr_zx;
    assign v_addr[1] = addr_giga;
    assign v_addr[2] = addr_zx;
    assign v_addr[3] = addr_zx;
    assign v_addr[4] = addr_zx;
    assign v_addr[5] = addr_zx;
    assign v_addr[6] = addr_256c;
    assign v_addr[7] = addr_text;
    
    assign video_addr = v_addr[vmod];

 
// ZX
	wire [20:0] addr_zx = addr_zx_gfx[0] ? addr_zx_gfx[21:1] : addr_zx_atr[21:1];
    
	wire [21:0] addr_zx_gfx = {scr_page, 1'b0, cnt_row[7:6], cnt_row[2:0], cnt_row[5:3], cnt_col[4:0]};
	wire [21:0] addr_zx_atr = {scr_page, 4'b0110, cnt_row[7:3], cnt_col[4:0]};

    
// Gigascreen
    wire [20:0] addr_giga = 0;

    
// 256c
	wire [20:0] addr_256c = {scr_page[7:4], cnt_row, cnt_col};


// Textmode
    wire [20:0] addr_text = 0;
    
    
// fetch window
	wire [8:0] g_beg[0:7];
	wire [8:0] g_end[0:7];
    
    assign g_beg[0] = hpix_beg - 16;
    assign g_beg[1] = hpix_beg - 16;
    assign g_beg[2] = hpix_beg - 16;
    assign g_beg[3] = hpix_beg - 16;
    assign g_beg[4] = hpix_beg - 16;
    assign g_beg[5] = hpix_beg - 8;
    assign g_beg[6] = hpix_beg - 16;
    assign g_beg[7] = hpix_beg - 8;

    assign g_end[0] = hpix_end - 18;
    assign g_end[1] = hpix_end - 18;
    assign g_end[2] = hpix_end - 18;
    assign g_end[3] = hpix_end - 18;
    assign g_end[4] = hpix_end - 18;
    assign g_end[5] = hpix_end - 10;
    assign g_end[6] = hpix_end - 18;
    assign g_end[7] = hpix_end - 10;

    assign go_beg = g_beg[vmod];
    assign go_end = g_end[vmod];


// DRAM bandwidth
    wire [1:0] bw[0:7];
    
    assign bw[0] = 2'b00;
    assign bw[1] = 2'b01;
    assign bw[2] = 2'b01;
    assign bw[3] = 2'b10;
    assign bw[4] = 2'b11;
    assign bw[5] = 2'b01;
    assign bw[6] = 2'b10;
    assign bw[7] = 2'b10;
    
    assign video_bw = bw[vmod];

	
// pixelrate
	wire [7:0] pixrate = 8'b10100000;
	assign hires = pixrate[vmod];
	
	
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

	
	
endmodule