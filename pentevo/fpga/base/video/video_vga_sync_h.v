// ZX-Evo Base Configuration (c) NedoPC 2008,2009,2010,2011,2012,2013,2014
//
// generates horizontal vga sync, double the rate of TV horizontal sync

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

module video_vga_sync_h(

	input  wire clk,

	output reg  vga_hsync,

	output reg  scanout_start,

	input  wire hsync_start
);

	localparam HSYNC_END	= 10'd106;
	localparam SCANOUT_BEG	= 10'd156;

	localparam HPERIOD = 10'd896;



	reg [9:0] hcount;

	initial
	begin
		hcount = 9'd0;
		vga_hsync = 1'b0;
	end



	always @(posedge clk)
	begin
			if( hsync_start )
				hcount <= 10'd2;
			else if ( hcount==(HPERIOD-9'd1) )
				hcount <= 10'd0;
			else
				hcount <= hcount + 9'd1;
	end


	always @(posedge clk)
	begin
		if( !hcount )
			vga_hsync <= 1'b1;
		else if( hcount==HSYNC_END )
			vga_hsync <= 1'b0;
	end


	always @(posedge clk)
	begin
		if( hcount==SCANOUT_BEG )
			scanout_start <= 1'b1;
		else
			scanout_start <= 1'b0;
	end


endmodule

