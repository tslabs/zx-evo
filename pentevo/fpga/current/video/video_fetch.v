// This module fetches video data from DRAM


module video_fetch (

// clocks
	input wire clk,
	
// control
    input wire [3:0] f_sel,
    input wire [1:0] b_sel,
    input wire fetch,
   
// video data
   	output reg [31:0] data,
	
// DRAM interface
	input  wire        video_strobe,
	input  wire [15:0] video_data
		
);


// fetching data
	reg [31:0] dram_in;
	
	always @(posedge clk) if (video_strobe)
	begin
		if (f_sel[0]) dram_in[ 7: 0] <= b_sel[0] ? video_data[15:8] : video_data[ 7:0];
		if (f_sel[1]) dram_in[15: 8] <= b_sel[1] ? video_data[15:8] : video_data[ 7:0];
		if (f_sel[2]) dram_in[23:16] <= video_data[ 7:0];
		if (f_sel[3]) dram_in[31:24] <= video_data[15:8];
	end
	
	
	always @(posedge clk) if (fetch)
			data <= dram_in;

endmodule