`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// DRAM arbiter. Shares DRAM between processor and video data fetcher
//
// Arbitration is made on full 8-cycle access blocks. Each cycle is defined by dram.v and consists of 4 fpga clocks.
// During each access block, there can be either no videodata access, 1 videodata access, 2, 4 or full 8 accesses.
// All spare cycles can be used by processor. If nobody uses memory in the given cycle, refresh cycle is performed
//
// In each access block, videodata accesses are spreaded all over the block so that processor receives cycle
// as fast as possible, until there is absolute need to fetch remaining video data
//
// Examples:
//
// |                 access block                  | 4 video accesses during block, no processor accesses. video accesses are done
// | vid | vid | vid | vid | ref | ref | ref | ref | as soon as possible, spare cycles are refresh ones
//
// |                 access block                  | 4 video accesses during block, processor requests access every other cycle
// | vid | prc | vid | prc | vid | prc | vid | prc |
//
// |                 access block                  | 4 video accesses, processor begins requesting cycles continously from second one
// | vid | prc | prc | prc | prc | vid | vid | vid | so it is given cycles while there is such possibility. after that processor
//                                                   can't access mem until the end of access block and stalls
//
// |                 access block                  | 8 video accesses, processor stalls, if it is requesting cycles
// | vid | vid | vid | vid | vid | vid | vid | vid |
//
// |                 access block                  | 2 video accesses, single processor request, other cycles are refresh ones
// | vid | vid | ref | ref | cpu | ref | ref | ref |
//
// |                 access block                  | 4 video accesses, single processor request, other cycles are refresh ones
// | vid | vid | cpu | vid | vid | ref | ref | ref |
//
// access block begins at any dram cycle, then blocks go back-to-back
//
// key signals are go and cpu_req, sampled at the end of each dram cycle. Must be set to the module
// one clock cycle earlier the clock of the beginning current dram cycle

module arbiter(

	input clk,
	input rst_n,

	// dram.v interface
      output     [20:0] dram_addr,   // address for dram access
      output reg        dram_req,    // dram request
      output reg        dram_rnw,    // Read-NotWrite
	input             dram_cbeg,   // cycle begin
	input             dram_rrdy,   // read data ready (coincides with cend)
	output      [1:0] dram_bsel,   // positive bytes select: bsel[1] for wrdata[15:8], bsel[0] for wrdata[7:0]
	input      [15:0] dram_rddata, // data just read
	output     [15:0] dram_wrdata, // data to be written


	output reg cend,      // regenerates this signal: end of DRAM cycle. cend is one-cycle positive pulse just before cbeg pulse
	output reg pre_cend,  // one clock earlier cend
	output reg post_cbeg, // one more earlier


	input go, // start video access blocks

	input [1:0] bw, // required bandwidth: 3'b00 - 1 video cycle per block
	                //                     3'b01 - 2 video accesses
	                //                     3'b10 - 4 video accesses
	                //                     3'b11 - 8 video accesses (stall of CPU)

	input  [20:0] video_addr,   // during access block, only when video_strobe==1
	output [15:0] video_data,   // read video data which is valid only during video_strobe==1 because video_data
	                            // is just wires to the dram.v's rddata signals
	output reg    video_strobe, // positive one-cycle strobe as soon as there is next video_data available.
	                            // if there is video_strobe, it coincides with cend signal
	output reg    video_next,   // on this signal you can change video_addr; it is one clock leading the video_strobe



	input cpu_req,cpu_rnw,
	input  [20:0] cpu_addr,
	input   [7:0] cpu_wrdata,
	input         cpu_wrbsel,

	output [15:0] cpu_rddata,
	output reg    cpu_stall,
	output  [4:0] cpu_waitcyc,
	output    reg cpu_strobe
);

	wire cbeg;

	reg [1:0] cctr; // DRAM cycle counter: 0 when cbeg is 1, then 1,2,3,0, etc...



	reg [2:0] blk_rem;  // remaining accesses in a block (7..0)
	reg [2:0] blk_nrem; // remaining for the next dram cycle

	reg [2:0] vid_rem;  // remaining video accesses in block (4..0)
	reg [2:0] vid_nrem; // for rhe next cycle


	wire [2:0] vidmax; // max number of video cycles in a block, depends on bw input



	localparam CYC_VIDEO = 2'b00; // do             there
	localparam CYC_CPU   = 2'b01; //   not     since     are   dependencies
	localparam CYC_FREE  = 2'b10; //      alter             bit

	reg [1:0] curr_cycle; // type of the cycle in progress
	reg [1:0] next_cycle; // type of the next cycle




	reg [4:0] wait_new,wait_cyc; // how many cycles to wait until end of CPU cycle: _new is for next _cyc
	reg [4:0] rw_add,vcyc_add;



	initial // simulation only!
	begin
		curr_cycle = CYC_FREE;
		blk_rem = 0;
		vid_rem = 0;
		cpu_stall = 0;
	end




	assign cbeg = dram_cbeg; // just alias

	// make cycle strobe signals
	always @(posedge clk)
	begin
		post_cbeg <= cbeg;
		pre_cend <= post_cbeg;
		cend <= pre_cend;
	end


	// track blk_rem counter: how many cycles left to the end of block (7..0)
	always @(posedge clk) if( cend )
	begin
		blk_rem <= blk_nrem;

		if( (blk_rem==3'd0) )
			cpu_stall <= (bw==2'd3) & go;
	end

	always @*
	begin
		if( (blk_rem==3'd0) && go )
			blk_nrem = 7;
		else
			blk_nrem = (blk_rem==0) ? 3'd0 : (blk_rem-3'd1);
	end



	// track vid_rem counter
	assign vidmax = (3'b001) << bw; // 1,2,4 or 8 - just to know how many cycles to perform

	always @(posedge clk) if( cend )
	begin
		vid_rem <= vid_nrem;
	end

	always @*
	begin
		if( go && (blk_rem==3'd0) )
			vid_nrem = cpu_req ? vidmax : (vidmax-3'd1);
		else
			if( next_cycle==CYC_VIDEO )
				vid_nrem = (vid_rem==3'd0) ? 3'd0 : (vid_rem-3'd1);
			else
				vid_nrem = vid_rem;
	end




	// next cycle decision
	always @*
	begin
		if( blk_rem==3'd0 )
		begin
			if( go )
			begin
				if( bw==2'b11 )
					next_cycle = CYC_VIDEO;
				else
					if( cpu_req )
						next_cycle = CYC_CPU;
					else
						next_cycle = CYC_VIDEO;
			end
			else // !go
			begin
				if( cpu_req )
					next_cycle = CYC_CPU;
				else
					next_cycle = CYC_FREE;
			end
		end
		else // blk_rem!=3'd0
		begin
			if( cpu_stall )
				next_cycle = CYC_VIDEO;
			else
			begin
				if( vid_rem==blk_rem )
					next_cycle = CYC_VIDEO;
				else
					if( cpu_req )
						next_cycle = CYC_CPU;
					else
						if( vid_rem==3'd0 )
							next_cycle = CYC_FREE;
						else
							next_cycle = CYC_VIDEO;
			end
		end
	end



	// just current cycle register
	always @(posedge clk) if( cend )
	begin
		curr_cycle <= next_cycle;
	end




	// route required data/etc. to and from the dram.v

	assign dram_wrdata[15:0] = { cpu_wrdata[7:0], cpu_wrdata[7:0] };
	assign dram_bsel[1:0] = { ~cpu_wrbsel, cpu_wrbsel };

	assign dram_addr = next_cycle[0] ? cpu_addr : video_addr;

	assign cpu_rddata = dram_rddata;
	assign video_data = dram_rddata;

	always @*
	begin
		if( next_cycle[1] ) // CYC_FREE
		begin
			dram_req = 1'b0;
			dram_rnw = 1'b1;
		end
		else // CYC_CPU or CYC_VIDEO
		begin
			dram_req = 1'b1;
			if( next_cycle[0] ) // CYC_CPU
				dram_rnw = cpu_rnw;
			else // CYC_VIDEO
				dram_rnw = 1'b1;
		end
	end



	// generation of strobes: for video and cpu
      // for cpu, write strobe is earlier than read one


	always @(posedge clk)
	begin
		if( (next_cycle==CYC_CPU) && cend && (!cpu_rnw) )
			cpu_strobe <= 1'b1;
		else if( (curr_cycle==CYC_CPU) && cpu_rnw && pre_cend )
			cpu_strobe <= 1'b1;
		else
			cpu_strobe <= 1'b0;


		if( (curr_cycle==CYC_VIDEO) && pre_cend )
			video_strobe <= 1'b1;
		else
			video_strobe <= 1'b0;

		if( (curr_cycle==CYC_VIDEO) && post_cbeg )
			video_next <= 1'b1;
		else
			video_next <= 1'b0;
	end



	// generate cpu_waitcyc

	always @*
	begin
		rw_add = cpu_rnw ? 5'd3 : 5'd0;

		if( (vid_rem==blk_rem) && (!cpu_stall) )
			vcyc_add[4:0] = {blk_rem[2:0],2'b00};
		else
			vcyc_add[4:0] = 5'd0;

		wait_new = rw_add + vcyc_add;
	end


	always @(posedge clk)
	begin
		if( (wait_cyc!=5'd0) )
		begin
			if( !cpu_stall )
				wait_cyc <= wait_cyc - 5'd1;
		end
		else // wait_cyc==0
		begin
			if( cend && cpu_req )
			begin
				wait_cyc <= wait_new;
			end
		end


	end


	assign cpu_waitcyc = wait_cyc;



endmodule

