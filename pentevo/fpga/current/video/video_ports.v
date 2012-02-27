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
    input wire zborder_wr,
    input wire border_wr,
    input wire zvpage_wr,
    input wire vpage_wr,
    input wire vconf_wr,
    input wire gx_offsl_wr,
    input wire gx_offsh_wr,
    input wire gy_offsl_wr,
    input wire gy_offsh_wr,
    input wire t0x_offsl_wr,
    input wire t0x_offsh_wr,
    input wire t0y_offsl_wr,
    input wire t0y_offsh_wr,
    input wire t1x_offsl_wr,
    input wire t1x_offsh_wr,
    input wire t1y_offsl_wr,
    input wire t1y_offsh_wr,
    input wire tsconf_wr,
    input wire palsel_wr,
    input wire tmpage_wr,
    input wire t0gpage_wr,
    input wire t1gpage_wr,
    input wire sgpage_wr,
    input wire hint_beg_wr ,
    input wire vint_begl_wr,
    input wire vint_begh_wr,

// video parameters
	output reg [7:0] border,
	output reg [7:0] vpage,
	output reg [7:0] vconf,
	output reg [8:0] gx_offs,
	output reg [8:0] gy_offs,
	output reg [8:0] t0x_offs,
	output reg [8:0] t0y_offs,
	output reg [8:0] t1x_offs,
	output reg [8:0] t1y_offs,
	output reg [7:0] palsel,
	output reg [7:0] hint_beg,
	output reg [8:0] vint_beg,
	output reg [7:0] tsconf,
	output reg [7:0] tmpage,
	output reg [7:0] t0gpage,
	output reg [7:0] t1gpage,
	output reg [7:0] sgpage

 );


    always @(posedge clk)
		if (res)
		begin
			vpage       <= 8'h05;
   			vconf       <= 8'h00;
			gx_offs     <= 9'b0;
			gy_offs     <= 9'b0;
			t0x_offs    <= 9'b0;
			t0y_offs    <= 9'b0;
			t1x_offs    <= 9'b0;
			t1y_offs    <= 9'b0;
			tsconf[7:5] <= 3'b0;
			palsel      <= 8'h0F;
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
			if (gx_offsl_wr	)   gx_offs[7:0]    <= d;
			if (gx_offsh_wr	)   gx_offs[8]      <= d[0];
			if (gy_offsl_wr	)   gy_offs[7:0]    <= d;
			if (gy_offsh_wr	)   gy_offs[8]      <= d[0];
			if (t0x_offsl_wr)   t0x_offs[7:0]   <= d;
			if (t0x_offsh_wr)   t0x_offs[8]     <= d[0];
			if (t0y_offsl_wr)   t0y_offs[7:0]   <= d;
			if (t0y_offsh_wr)   t0y_offs[8]     <= d[0];
			if (t1x_offsl_wr)   t1x_offs[7:0]   <= d;
			if (t1x_offsh_wr)   t1x_offs[8]     <= d[0];
			if (t1y_offsl_wr)   t1y_offs[7:0]   <= d;
			if (t1y_offsh_wr)   t1y_offs[8]     <= d[0];
			if (tsconf_wr   )   tsconf          <= d;
			if (palsel_wr   )   palsel          <= d;
			if (tmpage_wr	)   tmpage          <= d;
			if (t0gpage_wr	)   t0gpage         <= d;
			if (t1gpage_wr	)   t1gpage         <= d;
			if (sgpage_wr	)   sgpage          <= d;
			if (hint_beg_wr )   hint_beg        <= d;
			if (vint_begl_wr)   vint_beg[7:0]   <= d;
			if (vint_begh_wr)   vint_beg[8]     <= d[0];
        end


 endmodule