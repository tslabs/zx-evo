// This module fetches video data from DRAM


module video_fetch (

// clocks
	input wire clk,
	
// control
    input wire ptr,
   
// video data
	output  reg [31:0] dram_out,
	
// DRAM interface
	input  wire        video_strobe,
	input  wire [15:0] video_data
		
);


// fetching data
	always @(posedge clk) if (video_strobe)
		if (ptr)	// counter is already incremented by this time!
			dram_out[15: 0] <= video_data;	// 1st word is clocked
		else
			dram_out[31:16] <= video_data;	// 2nd word is clocked
	
    
	
endmodule