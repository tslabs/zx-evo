// This module fetches video data from DRAM


module video_fetch (

// clocks
	input wire clk,
	
// control
    input wire [1:0] ptr,
   
// video data
	output  reg [31:0] data_out,
	
// DRAM interface
	input  wire        video_strobe,
	input  wire [15:0] video_data
		
);


// fetching data
	always @(posedge clk)
		if (video_strobe)
			if (ptr[0])
				data_out[31:16] <= video_data;
			else
				data_out[15: 0] <= video_data;
	
    
	
endmodule