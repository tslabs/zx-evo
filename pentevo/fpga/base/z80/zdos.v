// ZX-Evo Base Configuration (c) NedoPC 2008,2009,2010,2011,2012,2013,2014
//
// just DOS signal control

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

module zdos(

	input  wire        fclk,
	input  wire        rst_n,


	input  wire        dos_turn_on,
	input  wire        dos_turn_off,

	input  wire        cpm_n,


	output reg         dos
);







	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
		dos = 1'b1;
	end
	else // posedge fclk
	begin
		if( !cpm_n )
			dos <= 1'b1;
		else if( dos_turn_off )
			dos <= 1'b0;
		else if( dos_turn_on )
			dos <= 1'b1;
	end







endmodule


