// This module maps z80 memory access to physical memories

`include "../include/tune.v"


module pager (

// clocks
    input wire clk,
    
// Z80 interface	
    input wire [15:0] za,
    input wire m1,
    input wire mreq,
    
// controls
	input wire [7:0] memconf,
	input wire [31:0] xt_page,
	input wire dos,
	input wire vdos,
    
  	output wire [7:0] page,
	output wire romnram,
    output wire rw_en,
  	output wire dos_on,
	output wire dos_off,
	output wire zclk_stall // stall Z80 clock during DOS turning on

		
);

    wire [1:0] win = za[15:14];
    wire win0 = ~|win;
    // assign romnram = win0 & ~memconf[3] & !vdos;
    assign romnram = win0 & ~memconf[3] & !(vdos & dos);
    assign rw_en = !win0 | memconf[1];
    assign page = xtpage[win];
    
    wire [7:0] xtpage[0:3];
    // assign xtpage[0] = vdos ? 8'hFF : {xt_page[7:2], memconf[2] ? xt_page[1:0] : {~dos, memconf[0]}};
    assign xtpage[0] = (vdos & dos) ? 8'hFF : {xt_page[7:2], memconf[2] ? xt_page[1:0] : {~dos, memconf[0]}};
    assign xtpage[1] = xt_page[15:8];
    assign xtpage[2] = xt_page[23:16];
    assign xtpage[3] = xt_page[31:24];
    
    
// DOS signal control
	assign dos_on = win0 && m1 && mreq && (za[13:8]==6'h3D) && memconf[0];
	assign dos_off = !win0 && m1 && mreq;
    // assign zclk_stall = dos_on | stall_count[2];
    assign zclk_stall = 0;

	reg [2:0] stall_count;
	always @(posedge clk)
	begin
		if (dos_on)
		begin
			stall_count[2] <= 1'b1;     // count: 000(stop) -> 101 -> 110 -> 111 -> 000(stop)
			stall_count[0] <= 1'b1;
		end
		else if( stall_count[2] )
		begin
			stall_count[2:0] <= stall_count[2:0] + 3'd1;
		end
    end
    

endmodule

