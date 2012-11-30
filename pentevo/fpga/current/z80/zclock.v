// PentEvo project (c) NedoPC 2008-2011
//
// Z80 clocking module, also contains some wait-stating when 14MHz
//
// IDEAL:
// fclk    _/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zclk     /```\___/```\___/```\___/```````\_______/```````\_______/```````````````\_______________/```````````````\_______________/`
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zpos     `\___/```\___/```\___/```\___________/```\___________/```\___________________________/```\___________________________/```\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zneg     _/```\___/```\___/```\_______/```\___________/```\___________________/```\___________________________/```\________________

// clock phasing:
// c3 must be zpos for 7mhz, therefore c1 - zneg
// for 3.5 mhz, c3 is both zpos and zneg (alternating)


// 14MHz rulez:
// 1. do variable stalls for memory access.
// 2. do fallback on 7mhz for external IO accesses
// 3. clock switch 14-7-3.5 only at RFSH


`include "../include/tune.v"

module zclock(

	input fclk,
	input zclk, // Z80 clock, buffered via act04 and returned back to the FPGA
	output reg zclk_out, // generated Z80 clock - passed through inverter externally!
	input c0, c2,
    
	input rst,
	input rfsh_n, // switch turbo modes in RFSH part of m1
	input  wire iorq_s,
	input  wire external_port,

	output reg zpos,
	output reg zneg,

// stall enables and triggers
	input  wire cpu_stall,
	input  wire ide_stall,
	input  wire dos_on,
	input  wire dos_off,

	input [1:0] turbo, // 2'b00 -  3.5 MHz
	                   // 2'b01 -  7.0 MHz
	                   // 2'b1x - 14.0 MHz

	// output reg [1:0] int_turbo // internal turbo, switched on /RFSH
	output wire [1:0] int_turbo // internal turbo, switched on /RFSH
    
);

	reg c2_cnt;
	wire h_c2_1; // to take every other pulse of c2
	wire h_c2_2; // to take every other pulse of c2

	reg [2:0] zcount; // counter for generating 3.5 and 7 MHz z80 clocks


	// reg old_rfsh_n;


	reg clk14_src; // source for 14MHz clock


`ifdef SIMULATE
	initial // simulation...
	begin
		c2_cnt = 1'b0;
		int_turbo   = 2'b00;
		old_rfsh_n  = 1'b1;
		clk14_src   = 1'b0;

		zclk_out = 1'b0;
	end
`endif

	// switch clock only at predefined time
	// always @(posedge fclk) if(zpos)
	// begin
		// old_rfsh_n <= rfsh_n;

		// if( old_rfsh_n && !rfsh_n )
			// int_turbo <= turbo;
	// end
		assign int_turbo = turbo;


// wait generator
	wire stall = cpu_stall || dos_io_stall || ide_stall;
    wire dos_io_stall = stall_start || !stall_count_end;
    wire stall_start = dos_stall || io_stall;
    wire dos_stall = dos_on || vdos_off;
    wire io_stall = iorq_s && external_port && int_turbo[1];
	wire stall_count_end = stall_count[3];

	reg [3:0] stall_count;
	always @(posedge clk)
		if (stall_start)
		begin
			if (dos_stall)
				stall_count <= 4'd4;	// 4 tacts 28MHz (1 tact 7MHz)
			else
				stall_count <= 4'd0;	// 8 tacts 28MHz (1 tact 3.5MHz)
		end
		else if (!stall_count_end)
			stall_count <= stall_count + 3'd1;


	// 14MHz clocking
	always @(posedge fclk)
	if( !stall )
		clk14_src <= ~clk14_src;
	//
	wire pre_zpos_140 =   clk14_src ;
	wire pre_zneg_140 = (~clk14_src);


	// take every other pulse of c2 (make half c2)
	always @(posedge fclk) if( c2 )
		c2_cnt <= ~c2_cnt;

	assign h_c2_1 =  c2_cnt && c2;
	assign h_c2_2 = !c2_cnt && c2;

	wire pre_zpos_35 = h_c2_2;
	wire pre_zneg_35 = h_c2_1;

	wire pre_zpos_70 = c2;
	wire pre_zneg_70 = c0;

	wire pre_zpos = int_turbo[1] ? pre_zpos_140 : ( int_turbo[0] ? pre_zpos_70 : pre_zpos_35 );
	wire pre_zneg = int_turbo[1] ? pre_zneg_140 : ( int_turbo[0] ? pre_zneg_70 : pre_zneg_35 );


	always @(posedge fclk)
	begin
		zpos <= (~stall) & pre_zpos & zclk_out;
	end

	always @(posedge fclk)
	begin
		zneg <= (~stall) & pre_zneg & (~zclk_out);
	end


	// make Z80 clock: account for external inversion and make some leading of clock
	// 9.5 ns propagation delay: from fclk posedge to zclk returned back any edge
	// (1/28)/2=17.9ns half a clock lead
	// 2.6ns lag because of non-output register emitting of zclk_out
	// total: 5.8 ns lead of any edge of zclk relative to posedge of fclk => ACCOUNT FOR THIS WHEN DOING INTER-CLOCK DATA TRANSFERS

	always @(negedge fclk)
	begin
		if (zpos)
			zclk_out <= 1'b0;

		if (zneg)
			zclk_out <= 1'b1;
	end


endmodule
