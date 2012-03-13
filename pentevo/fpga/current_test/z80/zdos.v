
`include "../include/tune.v"


module zdos(

	input  wire        clk,
	input  wire        zclk,
	input  wire        rst,
	input  wire        opfetch,

	input  wire        dos_on,
	input  wire        dos_off,
	input  wire        vdos_on,
	input  wire        vdos_off,

	output reg         dos,
	output reg         vdos,
	output wire        dos_stall

);


// turn on and off dos
	always @(posedge clk)
	if (rst)
		dos <= 1'b0;

	else if (dos_off)
			dos <= 1'b0;
	else if (dos_on)
			dos <= 1'b1;


// turn on and off virtdos
    reg pre_vdos;
    
    always @(posedge clk)
	if (rst)
		pre_vdos <= 1'b0;
	else if (vdos_on)
            pre_vdos <= 1'b1;
    else if (vdos_off)
            pre_vdos <= 1'b0;

    
    always @(negedge clk)
	if (rst)
		vdos <= 1'b0;
	else if (opfetch)
        vdos <= pre_vdos;


// stall Z80 clock during modes switching
    assign dos_stall = stall_start | stall_count[2];
    wire stall_start = dos_on | vdos_off;

	reg [2:0] stall_count;
	always @(posedge clk)
		if (stall_start)
		begin
			stall_count[2] <= 1'b1;
			stall_count[0] <= 1'b1;
		end
		else if (stall_count[2])
			stall_count[2:0] <= stall_count[2:0] + 3'd1;     // count: 000(stop) -> 101 -> 110 -> 111 -> 000(stop)


endmodule
