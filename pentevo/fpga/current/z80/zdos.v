
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
	output reg         vdos

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

    
    always @(posedge clk)
	if (rst)
		vdos <= 1'b0;
	else if (opfetch)
        vdos <= pre_vdos;


endmodule
