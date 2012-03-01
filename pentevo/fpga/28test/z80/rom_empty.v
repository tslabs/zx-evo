
`include "../include/tune.v"

module rom(
	input wire [15:0] a,
	output reg [7:0] d

);


	always @*
	case (a)
16'h0000:	d = 8'hF3;	// di
16'h0001:	d = 8'h01;  // ld bc, #FE
16'h0002:	d = 8'hFE;  //
16'h0003:	d = 8'h00;  //
16'h0004:	d = 8'h3C;	// inc a
16'h0005:	d = 8'hED;	// out (c), a
16'h0006:	d = 8'h79;	//
16'h0007:	d = 8'h18;  // jr #0004
16'h0008:	d = 8'hFB;	//
default:	d = 8'hFF;	//
	endcase


endmodule
