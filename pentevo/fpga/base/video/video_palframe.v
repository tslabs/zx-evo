// ZX-Evo Base Configuration (c) NedoPC 2008,2009,2010,2011,2012,2013,2014
//
// mix up border and pixels, add palette and blanks

/*
    This file is part of ZX-Evo Base Configuration firmware.

    ZX-Evo Base Configuration firmware is free software:
    you can redistribute it and/or modify it under the terms of
    the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ZX-Evo Base Configuration firmware is distributed in the hope that
    it will be useful, but WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ZX-Evo Base Configuration firmware.
    If not, see <http://www.gnu.org/licenses/>.
*/

`include "tune.v"

module video_palframe(

	input  wire        clk, // 28MHz clock


	input  wire        hpix,
	input  wire        vpix,

	input  wire        hblank,
	input  wire        vblank,

	input  wire        hsync_start,
	input  wire        vsync,

	input  wire [ 3:0] pixels,
	input  wire [ 3:0] border,

	input  wire        border_sync,
	input  wire        border_sync_ena,
	// ulaplus related
	input  wire [ 1:0] up_palsel,
	input  wire [ 2:0] up_paper,
	input  wire [ 2:0] up_ink,
	input  wire        up_pixel,

	input  wire        up_ena,
	input  wire        up_palwr,
	input  wire [ 5:0] up_paladdr,
	input  wire [ 7:0] up_paldata,

	input  wire        atm_palwr,
	input  wire [ 5:0] atm_paldata,


	output wire [ 5:0] palcolor, // just for palette readback

	output wire [ 7:0] color
);
	reg [7:0] palette_read;	

	wire [ 3:0] zxcolor;
	wire [ 5:0] up_color;
	wire [ 8:0] palette_color;

	reg [3:0] synced_border;
	reg vsync_r;
	reg [1:0] ctr_14;
	reg ctr_h;
	reg ctr_v;

	always @(posedge clk)
	if( border_sync )
		synced_border <= border;

	assign zxcolor = (hpix&vpix) ? pixels : (border_sync_ena ? synced_border : border);

	assign up_color = (hpix&vpix) ? {up_palsel,~up_pixel,up_pixel?up_ink:up_paper} : {3'd0,border[2:0]};

	assign palette_color = up_ena ? {3'b100,up_color} : {5'd0,zxcolor};


	// palette
	reg [7:0] palette [0:511]; // let quartus instantiate it as RAM

	always @(posedge clk)
	begin
		if( atm_palwr || up_palwr )
		begin : palette_write
			reg [8:0] pal_addr;
			pal_addr = atm_palwr ? { 5'd0, zxcolor } : { 3'b100, up_paladdr };

			palette[pal_addr] <= atm_palwr ? {atm_paldata[3:2],1'b0,atm_paldata[5:4],1'b0,atm_paldata[1:0]} : up_paldata;
		end

		palette_read <= palette[palette_color];
	end


	assign palcolor = {palette_read[4:3],palette_read[7:6], palette_read[1:0]};




	// make 3bit palette
	always @(posedge clk)
		vsync_r <= vsync;
	//
	wire vsync_start = vsync && !vsync_r;
	//
	initial ctr_14 = 2'b00;
	always @(posedge clk)
		ctr_14 <= ctr_14+2'b01;
	//
	initial ctr_h = 1'b0;
	always @(posedge clk) if( hsync_start )
		ctr_h <= ~ctr_h;
	//
	initial ctr_v = 1'b0;
	always @(posedge clk) if( vsync_start )
		ctr_v <= ~ctr_v;


	wire plus1 = ctr_14[1] ^ ctr_h ^ ctr_v;



	wire [1:0] red;
	wire [1:0] grn;
	wire [1:0] blu;

	video_palframe_mk3bit red_color
	(
		.plus1    (plus1            ),
		.color_in (palette_read[7:5]),
		.color_out(red              )
	);
	//
	video_palframe_mk3bit grn_color
	(
		.plus1    (plus1            ),
		.color_in (palette_read[4:2]),
		.color_out(grn              )
	);
	//
	assign blu = palette_read[1:0];

`ifdef IDE_VDAC
	assign color = (hblank | vblank) ? 8'd0 : {palette_read[4:2], palette_read[7:5], palette_read[1:0]};
`else
	assign color = (hblank | vblank) ? 8'd0 : {grn, 1'b0, red, 1'b0, blu};
`endif


endmodule

module video_palframe_mk3bit
(
	input  wire       plus1,

	input  wire [2:0] color_in,
	output reg  [1:0] color_out
);

	always @*
	case( color_in )
		3'b000:  color_out <= 2'b00;
		3'b001:  color_out <= plus1 ? 2'b01 : 2'b00;
		3'b010:  color_out <= 2'b01;
		3'b011:  color_out <= plus1 ? 2'b10 : 2'b01;
		3'b100:  color_out <= 2'b10;
		3'b101:  color_out <= plus1 ? 2'b11 : 2'b10;
		default: color_out <= 2'b11;
	endcase

endmodule

