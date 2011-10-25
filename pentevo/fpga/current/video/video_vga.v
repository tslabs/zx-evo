// This module shifts in TV data into buffer and shift out VGA data
// Also, it generates blanking signal for VGA


module video_vga (

// clocks
	input wire clk,
	input wire c0,
	input wire q0,

// video data
	input  wire [7:0] vga_in,
	output wire [7:0] vga_out,

// video controls
	input wire start_in,
	input wire start_out,
	input wire line_start,
    output wire vga_blank

);


	reg [8:0] cnt_in;
	reg [8:0] cnt_out;
	reg sel;

	always @(posedge clk) if (c0)
		cnt_in <= start_in ? 0 : cnt_in + 1;
		
	always @(posedge clk) if (q0)
		cnt_out <= start_out ? 0 : cnt_out + 1;

	always @(posedge clk) if (c0)
		if (line_start)
			sel <= ~sel;

	assign vga_blank = (cnt_out >= 9'd360);

    
 // VGA buffer   
	video_vmem video_vmem(
		.clock		(clk			),
		.wraddress	({sel, cnt_in}	),
		.data		(vga_in			),
		.wren		(c0				),
	    .rdaddress	({~sel, cnt_out}),
	    .q			(vga_out		)
	);


	
endmodule
