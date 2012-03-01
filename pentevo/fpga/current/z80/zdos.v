// PentEvo project (c) NedoPC 2008-2010
//
// just DOS signal control

`include "../include/tune.v"


module zdos(

	input  wire        fclk,
	input  wire        rst,
	input  wire        m1,

	input  wire        dos_on,
	input  wire        dos_off,
	input  wire        vdos_on,
	input  wire        vdos_off,

	output reg         dos,
	output reg         vdos

);


// turn on and off dos
	always @(posedge fclk)
	if (rst)
	begin
		dos <= 1'b0;
	end

	else
	begin
        if (dos_off)
			dos <= 1'b0;
		else
        if (dos_on)
			dos <= 1'b1;
	end


// turn on and off virtdos
    reg pre_vdos;
    
    always @(posedge fclk)
	if (rst)
		pre_vdos <= 1'b0;
	else if (vdos_on)
            pre_vdos <= 1'b1;
    else if (vdos_off)
            pre_vdos <= 1'b0;

    
    always @(posedge fclk)
	if (rst)
		vdos <= 1'b0;
	else if (m1)
            vdos <= pre_vdos;


endmodule
