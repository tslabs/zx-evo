// part of NeoGS project (c) 2007-2008 NedoPC
//

// SPI mode 0 8-bit master module
//
// short diagram for speed=0 (Fclk/Fspi=2, no rdy shown)
//
// clk:     ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^  ^ (positive edges)
// counter: 00|00|00|10|11|12|13|14|15|16|17|18|19|1A|1B|1C|1D|1E|1F|00|00|00 // internal!
// sck:     ___________/``\__/``\__/``\__/``\__/``\__/``\__/``\__/``\_______
// sdo:     --------< do7 | do6 | do5 | do4 | do3 | do2 | do1 | do0 >-------
// sdi:     --------< di7 | di6 | di5 | di4 | di3 | di2 | di1 | di0 >-------
// bsync:   ________/`````\_________________________________________________
// start:   _____/``\_______________________________________________________
// din:     -----<IN>-------------------------------------------------------
// dout:     old old old old old old old old old old old old old | new new new
//
// data on sdo must be latched by slave on rising sck edge. data on sdo changes on falling edge of sck
//
// data from sdi is latched by master on positive edge of sck, while slave changes it on falling edge.
//  WARNING: slave must emit valid di7 bit BEFORE first pulse on sck!
//
// bsync is 1 while do7 is outting, otherwise it is 0
//
// start is synchronous pulse, which starts all transfer and also latches din data on the same clk edge
//  as it is registered high. start can be given anytime (only when speed=0),
//  so it is functioning then as synchronous reset. when speed!=0, there is global enable for majority of
//  flipflops in the module, so start can't be accepted at any time
//
// dout updates with freshly received data at the clk edge in which sck goes high for the last time, thus
//  latching last bit on sdi.
//
// sdo emits last bit shifted out after the transfer end
//
// when speed=0, data transfer rate could be as fast as one byte every 16 clk pulses. To achieve that,
//   start must be pulsed high simultaneously with the last high pulse of sck
//
// speed[1:0] determines Fclk/Fspi
//
//  speed | Fclk/Fspi
//  ------+----------
//  2'b00 | 2
//  2'b01 | 4
//  2'b10 | 8
//  2'b11 | 16
//
// for speed=0 you can start new transfer as fast as every 16 clks
// for speed=1 - every 34 clks.
// alternatively, you can check rdy output: it goes to 0 after start pulse and when it goes back to 1, you can
// issue another start at the next clk cycle. See spi2_modelled.png and .zip (modelsim project)
//
// warning: if using rdy-driven transfers and speed=0, new transfer will be started every 18 clks.
//  it is recommended to use rdy-driven transfers when speed!=0
//
// warning: this module does not contain asynchronous reset. Provided clk is stable, start=0
//  and speed=0, module returns to initial ready state after maximum of 18+8=26 clks. To reset module
//  to the known state from any operational state, set speed=0 and start=1 for 8 clks
//  (that starts Fclk/Fspi=2 speed transfer for sure), then remain start=0, speed=0 for at least 18 clks.

`include "../include/tune.v"


module spi(
// SPI wires
	input  wire       clk,      // system clk
	output wire       sck,      // SPI bus pins...
	output wire       sdo,      //
	input  wire       sdi,      //

// controls
	output wire       stb,      // ready strobe, 1 clock length
	// output wire       rdy,      // ready (idle) - when module can accept data
	output reg        bsync,    // for vs1001
	
// DMA interface
	input  wire       dma_req,
	input  wire [7:0] dma_din,
	
// Z80 interface
	input  wire       cpu_req,
	input  wire [7:0] cpu_din,

	output reg  [7:0] dout,
	
// configuration
	input  wire [1:0] speed,    // =2'b00 - sck full speed (1/2 of clk), =2'b01 - half (1/4 of clk), =2'b10 - one fourth (1/8 of clk), =2'b11 - one eighth (1/16 of clk)
	
	output reg [2:0] tst

);


	always @*
		if (stb)
			tst = 5;
		else if (start)
			tst = 3;
		else if (dma_req)
			tst = 1;
		else if (cpu_req)
			tst = 4;
		else tst = 0;
	
	wire req = cpu_req || dma_req;
	wire [7:0] din = dma_req ? dma_din : cpu_din;

	initial // for simulation only!
	begin
		counter = 5'b10000;
		shiftout = 8'd0;
		shiftout = 7'd0;
		bsync = 1'd0;
		dout = 1'b0;
	end


	// sdo is high bit of shiftout
	assign sdo = shiftout[7];

	wire ena_shout_load = (start || sck) & g_ena;     // enable load of shiftout register

	assign sck = counter[0];
	wire rdy = counter[4];         // =0 when transmission in progress
	assign stb = !stb_r && rdy;
	wire start = req && rdy;

	reg [6:0] shiftin; 	// shifting in data from sdi before emitting it on dout
	reg [4:0] counter; 	// handles transmission
	reg stb_r;
	always @(posedge clk)
	begin
		if (g_ena)
		begin
			if (start)
			begin
				counter <= 5'b0; 	// rdy = 0; sck = 0;
				bsync <= 1'b1; 		// begin bsync pulse
				stb_r <= 1'b0;
			end

			else
			begin
				if (!sck) // on the rising edge of sck
				begin
      	            shiftin[6:0] <= {shiftin[5:0], sdi};

					if (&counter[3:1] && !rdy)
						dout <= {shiftin[6:0], sdi}; // update dout at the last sck rising edge
				end

				else // on the falling edge of sck
				begin
					bsync <= 1'b0;
				end

				if (!rdy)
					counter <= counter + 5'd1;
					
				stb_r <= rdy;
			end
		end
	end


	// shiftout treatment is done so just to save LCELLs in acex1k
	reg [7:0] shiftout; // shifting out data to the sdo
	always @(posedge clk)
	begin
		if (ena_shout_load)
		begin
			if (start)
				shiftout <= din;
			else // sck
				shiftout[7:0] <= {shiftout[6:0], shiftout[0]}; // last bit remains after end of exchange
		end
	end


	// slow speeds - controlled by g_ena
	reg [2:0] wcnt;
	always @(posedge clk)
	begin
		if (|speed)
		begin
			if (start)
				wcnt <= 3'b001;
			else if (rdy)
				wcnt <= 3'b000;
			else
				wcnt <= wcnt + 3'd1;
		end
		else
			wcnt <= 3'b000;
	end


	wire g_ena = g_en[speed];
    wire g_en[0:3];
    assign g_en[0] = 1'b1;
    assign g_en[1] = ~|wcnt[0];
    assign g_en[2] = ~|wcnt[1:0];
    assign g_en[3] = ~|wcnt[2:0];


endmodule
