`include "../include/tune.v"

// PentEvo project (c) NedoPC 2010-2011
//
// generates horizontal vga sync, double the rate of TV horizontal sync

module video_vga_sync_h(

	input  wire clk,

	output reg  vga_hsync,

	output reg  scanout_start,

	input  wire hsync_start
);

	localparam HSYNC_END	= 10'd106;
	localparam SCANOUT_BEG	= 10'd159;

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

