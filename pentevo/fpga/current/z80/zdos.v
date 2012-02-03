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
	input  wire        virt_vg_hit,

	input  wire        cpm_n,

	output reg         dos,
	output reg         virtdos
    
);


// turn on and off dos
	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
	begin
		dos = 1'b1;
	end
	else // posedge fclk
	begin
		if( !cpm_n )
			dos <= 1'b1;
		else if( dos_turn_off )
			dos <= 1'b0;
		else if( dos_turn_on )
			dos <= 1'b1;
	end

    
// turn on and off virtdos
    // always @(posedge fclk)
        // if (virt_vg_hit)
            // virtdos <= 1'b1;
        // else
        // if (virtdos & m2)
            // virtdos <= 1'b0;
            

// checking for 
    // always @(posedge fclk)
    // begin
        
    // end
    
    
endmodule


