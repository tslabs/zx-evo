// This module latches all port parameters for video from Z80

`include "../include/tune.v"


module video_ports (

// clocks
	input wire clk,

// Z80 controls
	input wire [ 7:0] d,

// ZX controls
    input wire        res,    
    
// port write strobes
    input wire        zborder_wr,
    input wire        border_wr,
    input wire        zvpage_wr,
    input wire        vpage_wr,
    input wire        vconf_wr,
    input wire        x_offsl_wr,
    input wire        x_offsh_wr,
    input wire        y_offsl_wr,
    input wire        y_offsh_wr,
    input wire        tsconf_wr,
    input wire        palsel_wr,
    input wire        tgpage_wr,
    input wire        hint_beg_wr ,
    input wire        vint_begl_wr,
    input wire        vint_begh_wr,

// video parameters
	output reg [7:0] border,
	output reg [7:0] vpage, 
	output reg [7:0] vconf, 
	output reg [8:0] x_offs,
	output reg [8:0] y_offs,
	output reg [3:0] palsel,
	output reg [7:0] hint_beg,
	output reg [8:0] vint_beg,
	output reg [7:0] tsconf,
	output reg [4:0] tgpage
	
 );
 
 
    always @(posedge clk)
		if (res)
		begin
			vpage       <= 8'h05;
   			vconf       <= 8'h00;
			x_offs      <= 9'b0;
			y_offs      <= 9'b0;
			tsconf[7:5] <= 3'b0;
			palsel      <= 4'hF;
			hint_beg    <= 8'd2;	// pentagon default
			vint_beg    <= 9'd0;
        end
        
        else
        begin
            if (zborder_wr  )   border          <= {5'b11110, d[2:0]};
            if (border_wr   )   border          <= d;
            if (zvpage_wr	)   vpage           <= {6'b000001, d[3], 1'b1};
            if (vpage_wr	)   vpage           <= d;
            if (vconf_wr    )   vconf           <= d;
			if (x_offsl_wr  )   x_offs[7:0]     <= d;
			if (x_offsh_wr  )   x_offs[8]       <= d[0];
			if (y_offsl_wr  )   y_offs[7:0]     <= d;
			if (y_offsh_wr  )   y_offs[8]       <= d[0];
			if (tsconf_wr   )   tsconf          <= d;
			if (palsel_wr   )   palsel          <= d[3:0];
			if (tgpage_wr   )   tgpage          <= d[7:3];
			if (hint_beg_wr )   hint_beg        <= d;
			if (vint_begl_wr)   vint_beg[7:0]   <= d;
			if (vint_begh_wr)   vint_beg[8]     <= d[0];
        end

 
 endmodule