// This module shifts in TV data into buffer and shift out VGA data
// Also, it generates blanking signal for VGA


module video_vga (

// clocks
	input wire clk,
	input wire c0,
	input wire c4,
	input wire q0,

// video data
	input  wire [7:0] vga_in,
	output wire [7:0] vga_out,

// mode controls	
	input wire hires,

// video controls
	input wire start_in,
	input wire start_out,
	input wire line_start,
    output wire hb

);


	reg [8:0] cnt_in;
	reg [8:0] cnt_out;
	reg sel;
	reg [3:0] temp;

	
	assign hb = (cnt_out >= 9'd360);
	
	
// data-in
	always @(posedge clk) if (c0)
		cnt_in <= start_in ? 0 : cnt_in + 1;

	always @(posedge clk) if (c4)
		temp <= vga_in[3:0];
		
	wire [7:0] video_in = hires ? {temp, vga_in[3:0]} : vga_in;		// in HI-RES mode line keeps two pixels 4 bits of data each
	
		
// data-out
	always @(posedge clk) if (q0)
		cnt_out <= start_out ? 0 : cnt_out + 1;

		
// line toggle
	always @(posedge clk) if (c0)
		if (line_start)
			sel <= ~sel;
	

	wire [9:0] lwr_adr = {sel, cnt_in};
	wire [9:0] lrd_adr = {~sel, cnt_out};
    
 // VGA buffer   
	video_vmem video_vmem(
		.clock		(clk		),
		.wraddress	(lwr_adr	),
		.data		(video_in	),
		.wren		(c0			),
	    .rdaddress	(lrd_adr	),
	    .q			(vga_out	)
	);


	
endmodule
