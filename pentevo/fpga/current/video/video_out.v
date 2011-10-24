// This module generates video for DAC


module video_out (

// clocks
	input wire clk,
	
// video controls
    input wire vga_on,

// video data
	input  wire [7:0] tvdata,
	input  wire [7:0] vgadata,
	output reg  [1:0] vred,
    output reg  [1:0] vgrn,
    output reg  [1:0] vblu
);


	always @(posedge clk)
	begin
		vred <= vga_on ? vgadata[7:6] : tvdata[7:6];
		vgrn <= vga_on ? vgadata[5:4] : tvdata[5:4];
		vblu <= vga_on ? vgadata[3:2] : tvdata[3:2];
	end

	
	
endmodule
