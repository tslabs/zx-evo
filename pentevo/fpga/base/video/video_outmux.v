// ZX-Evo Base Configuration (c) NedoPC 2008,2009,2010,2011,2012,2013,2014
//
// mux out VGA and TV signals and make final v|h syncs, DAC data, etc.

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

module video_outmux(

	input  wire        clk,


	input  wire        vga_on,


	input  wire [ 7:0] tvcolor,
	input  wire [ 7:0] vgacolor,

	input  wire        vga_hsync,
	input  wire        hsync,
	input  wire        vsync,


	output reg  [ 2:0] vred,
	output reg  [ 2:0] vgrn,
	output reg  [ 1:0] vblu,

	output reg         vhsync,
	output reg         vvsync,
	output reg         vcsync
);


	always @(posedge clk)
	begin
		vgrn[2:0] <= vga_on ? vgacolor[7:5] : tvcolor[7:5];
		vred[2:0] <= vga_on ? vgacolor[4:2] : tvcolor[4:2];
		vblu[1:0] <= vga_on ? vgacolor[1:0] : tvcolor[1:0];

		vhsync <= vga_on ? vga_hsync : hsync;
		vvsync <= vsync;

		vcsync <= ~(hsync ^ vsync);
	end



endmodule

