// PentEvo project (c) NedoPC 2008-2010
//
// just DOS signal control

`include "../include/tune.v"


module zdos(

	input  wire        fclk,
	input  wire        rst_n,
	input  wire        m1_on,

	input  wire        dos_turn_on,
	input  wire        dos_turn_off,
	input  wire        vdos_on,
	input  wire        vdos_off,

	input  wire        cpm_n,

	output reg         dos,
	output reg         vdos
    
);


// turn on and off dos
	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
    
`ifdef BOLEQ    
		dos <= 1'b0;    // boleq
`else
		dos <= 1'b1;     //atm
`endif

	end
	else // posedge fclk
	begin
    
`ifndef BOLEQ    
		if( !cpm_n )            // remove it!!! (boleq)
			dos <= 1'b1;
		else
`endif
        if( dos_turn_off )
			dos <= 1'b0;
		else if( dos_turn_on )
			dos <= 1'b1;
	end

    
// turn on and off virtdos
    always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
		vdos <= 1'b0;
	end
	else // posedge fclk
        if (vdos_on)
            vdos <= 1'b1;
        else
        if (vdos_off)
            vdos <= 1'b0;
            
    
endmodule


