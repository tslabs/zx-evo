`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// DRAM controller. performs accesses to DRAM.
//
// state:          | RD1   | RD2   | RD3   | RD4   | WR1   | WR2   | WR3   | WR4   | RFSH1 | RFSH2 | RFSH3 | RFSH4 |
// clk: ___/```\___/```\___/```\___/```\___/```\___/```\___/```\___/```\___/```\___/```\___/```\___/```\___/```\___/```\__
//                 |      READ CYCLE               |      WRITE CYCLE              |      REFRESH CYCLE            |
// ras: ```````````````````\_______________/```````````````\_______________/```````````````````````\_______________/
// cas: ```````````````````````````\_______________/```````````````\_______________/```````\_______________/````````
// ra:                 |  row  | column|               |  row  | column|
// rd:     XXXXXXXXXXXXXXXXXXXXXXXXX<read data read|  write data write data write  |
// rwe:   `````````````````````````````````````````\_______________________________/````````````````````````````````
// req:  __/```````\_______________________/```````\________________________________________________________________
// rnw:  XX/```````\XXXXXXXXXXXXXXXXXXXXXXX\_______/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// c0:   __________/```````\_______________________/```````\_______________________/```````\_______________________/
// rrdy: __________________________________/```````\________________________________________________________________
// addr: XX< addr  >XXXXXXXXXXXXXXXXXXXXXXX< addr  >XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//wrdata:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX< write >XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//rddata:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX< read  >XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//
// comments:
// rucas_n, rlcas_n, rras0_n, rras1_n, rwe_n, ra[] could be made 'fast output register'
//
// rst_n is resynced before use and acts as req inhibit. so while in reset, dram regenerates and isn't corrupted

module dram(

	input clk,
	input rst_n, // shut down accesses, remain refresh

	output reg [9:0] ra, // to the DRAM pins
	inout     [15:0] rd, // .              .
	                     // .              .
	output reg rwe_n,    // .              .
	output reg rucas_n,  // .              .
	output reg rlcas_n,  // .              .
	output reg rras0_n,  // .              .
	output reg rras1_n,  // to the DRAM pins

	output reg [15:0] rddata, // data just read

	input [20:0] addr, // access address of 16bit word: addr[0] selects between rras0_n and rras1_n,
	                   // addr[10:1] goes to row address, addr[20:11] goes to column address

	input req,         // request for read/write cycle
	input rnw,         // READ/nWRITE (=1: read, =0: write)

	input c0, c1, c2, c3,


	input  [15:0] wrdata, // data to be written
	input   [1:0] bsel    // positive byte select for write: bsel[0] is for wrdata[7:0], bsel[1] is for wrdata[15:8]


);

	reg [1:0] rst_sync;
	wire reset;
	wire int_req;

	reg [20:0] int_addr;
	reg [15:0] int_wrdata;
	reg [1:0] int_bsel;

	reg rfsh_alt; // we must alternate chips in refresh cycles to lower total heating


// next cycle decision
	reg [1:0] state = 2'b0;

	localparam RFSH = 2'b00;	// Don't change these
	localparam RD   = 2'b01;	// because there are
	localparam WR   = 2'b10;	// bit dependencies
	
    wire idle = |state;
    wire read = state[0];
    wire write = state[1];
    
	wire [1:0] next_state = int_req ? rnw ? RD : WR : RFSH;
	
	always @(posedge clk) if (c3)
		state <= next_state;


// incoming data latching
	always @(posedge clk) if (c0)      // changed: now wrdata is latched 1 clk later - at c0 of current cycle (NOT at c3 of previous as before)
    begin
		int_wrdata <= wrdata;
    end
        
// incoming addr and bsel latching
	always @(posedge clk) if (c3)
	begin
		int_bsel   <= bsel;
		int_addr   <= addr;
	end

	
// WE control
	always @(posedge clk) if (c3)
		rwe_n <= ~next_state[1];


// RAS/CAS sequencing
	always @(posedge clk)
	begin
		if (c0)
			if (idle)
			begin
				rras0_n <= int_addr[0];
				rras1_n <= ~int_addr[0];
			end
			else
			begin
				rlcas_n <= 1'b0;
				rucas_n <= 1'b0;
			end
				
		if (c1)
			if (idle)
			begin
				rlcas_n <= write ? ~int_bsel[0] : 1'b0;
				rucas_n <= write ? ~int_bsel[1] : 1'b0;
			end
			else
			begin
				rras0_n <=  rfsh_alt;
				rras1_n <= ~rfsh_alt;
				rfsh_alt <= ~rfsh_alt;
			end
	
		if (c2)
			if (idle)
			begin
				rras0_n <= 1'b1;
				rras1_n <= 1'b1;
			end
			else
			begin
				rlcas_n <= 1'b1;
				rucas_n <= 1'b1;
			end
				
		if (c3)
		begin
			rras0_n <= 1'b1;
			rras1_n <= 1'b1;
			rucas_n <= 1'b1;
			rlcas_n <= 1'b1;
		end
	end

// row/column address multiplexing
	always @(negedge clk)		// here is a difficulty with fitting to pin fast output regs
		ra <= c0 ? int_addr[10:1] : int_addr[20:11];


// DRAM data bus control
	assign rd = rwe_n ? 16'hZZZZ : int_wrdata;


// read data from DRAM
	always @(posedge clk)
		if (c2 & read)
			rddata <= rd;


	// reset must be synchronous here in order to preserve
	// DRAM state while other modules reset, but we have only
	// asynchronous one globally. so we must re-synchronize it
	// and use it as 'DRAM operation enable'. when in reset,
	// controller ignores req signal and generates only refresh cycles
	always @(posedge clk)
		rst_sync[1:0] <= { rst_sync[0], ~rst_n };

	assign reset = rst_sync[1];

	assign int_req = req & (~reset);

	
endmodule
