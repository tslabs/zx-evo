`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
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

	input wire clk,
	input wire c1,
	input wire c2,
	input wire c3,
	input wire rst_n,

// dram.v interface
	output wire [20:0] dram_addr,   // address for dram access
	output wire        dram_req,    // dram request
	output wire        dram_rnw,    // Read-NotWrite
	output wire  [1:0] dram_bsel,   // positive bytes select: bsel[1] for wrdata[15:8], bsel[0] for wrdata[7:0]
	output wire [15:0] dram_wrdata, // data to be written

// CPU
	input wire [20:0] cpu_addr,
	input wire [ 7:0] cpu_wrdata,
	input wire        cpu_req,
	input wire        cpu_rnw,
	input wire        cpu_wrbsel,
	output reg        cpu_next,
    output reg        cpu_strobe,

// video
	input  [20:0] video_addr,   // during access block, only when video_strobe==1
	output wire   video_next,   // on this signal you can change video_addr; it is one clock leading the video_strobe
	input wire go, 				// start video access blocks
	output wire   video_strobe, // positive one-cycle strobe as soon as there is next video_data available.
	                            // if there is video_strobe, it coincides with c3 signal
	input wire [4:0] video_bw,
								// [4:3] - total cycles: 11 = 8 / 01 = 4 / 00 = 2
								// [2:0] - need cycles

// DMA
	input wire [20:0] dma_addr,
	input wire [15:0] dma_wrdata,
	input wire        dma_req,
	input wire        dma_zwt,
	input wire        dma_rnw,
	output reg        dma_next,

// TS engine
	input wire [20:0] ts_addr,
	input wire 	      ts_req,
	input wire 	      ts_zwt,
	output wire       ts_pre_next,
	output wire       ts_next

);

	reg stall;
	reg cpu_rnw_r;

	reg [2:0] blk_rem;  // remaining accesses in a block (7..0)
	reg [2:0] blk_nrem; // remaining for the next dram cycle

	reg [2:0] vid_rem;  // remaining video accesses in block (4..0)
	reg [2:0] vid_nrem; // for rhe next cycle


	wire [2:0] vidmax; // max number of video cycles in a block, depends on bw input


// DRAM access priority:
// - CPU
// - VIDEO
// - TS
// - DMA


	localparam CYC_CPU   = 4'b0001;
	localparam CYC_VIDEO = 4'b0010;
	localparam CYC_TS    = 4'b0100;
	localparam CYC_DMA   = 4'b1000;
	localparam CYC_FREE  = 4'b0000;

	reg [3:0] curr_cycle; // type of the cycle in progress
	reg [3:0] next_cycle; // type of the next cycle

	wire next_cpu = next_cycle[0];
	wire next_vid = next_cycle[1];
	wire next_ts  = next_cycle[2];
	wire next_dma = next_cycle[3];
	wire next_fre = ~|next_cycle;

	wire curr_cpu = curr_cycle[0];
	wire curr_vid = curr_cycle[1];
	wire curr_ts  = curr_cycle[2];
	wire curr_dma = curr_cycle[3];
	wire curr_fre = ~|curr_cycle;


	// assign c0 = dram_c0; // just alias

	wire bw_full = ~|{video_bw[4] & video_bw[2], video_bw[3] & video_bw[1], video_bw[0]}; // stall when 000/00/0
    wire blk_rem_0 = ~|blk_rem;

	// track blk_rem counter: how many cycles left to the end of block (7..0)
	always @(posedge clk) if (c3)
	begin
		blk_rem <= blk_nrem;
		if (blk_rem_0)
			stall <= bw_full & go;
	end

	always @*
	begin
		if (blk_rem_0 && go)
			blk_nrem = {video_bw[4:3], 1'b1};	// remaining 7/3/1
		else
			blk_nrem = blk_rem_0 ? 3'd0 : (blk_rem - 3'd1);
	end


// track vid_rem counter
	assign vidmax = {video_bw[2:0]};    // number of cycles for video access

	always @(posedge clk) if (c3)       // number of remain video cycles
		vid_rem <= vid_nrem;


	always @*
	begin
		if (go && blk_rem_0)
			vid_nrem = cpu_req ? vidmax : (vidmax - 3'd1);
		else
			if (next_vid)
				vid_nrem = ~|vid_rem ? 3'd0 : (vid_rem - 3'd1);
			else
				vid_nrem = vid_rem;
	end


// next cycle decision
    wire cpu_low = (ts_req & ts_zwt) | (dma_req & dma_zwt);
    wire other_req = cpu_req | ts_req | dma_req;
    wire [3:0] next_other = cpu_low ? (ts_req ? CYC_TS : CYC_DMA) : CYC_CPU;

	always @*
		if (blk_rem_0)      // video burst start or video idle
			if (go)             // video start
				if (bw_full)        // BW full - CPU stall
				begin
					cpu_next = 1'b0;
					next_cycle = CYC_VIDEO;
				end

				else                // BW not full - if CPU has low priority - video goes first
				begin
					cpu_next = !cpu_low;
					if (cpu_req & !cpu_low)
						next_cycle = CYC_CPU;
					else
						next_cycle = CYC_VIDEO;
				end

			else
			begin               // video idle
				cpu_next = !cpu_low;
				if (other_req)
					next_cycle = next_other;
				else
					next_cycle = CYC_FREE;
			end

		else                // video burst in progress
		if (stall | (vid_rem==blk_rem))     // stall CPU if there too few cycles for video left
		begin
			cpu_next = 1'b0;
			next_cycle = CYC_VIDEO;
		end

        else                    // there are spare cycles that can be used for others before video
		begin
			cpu_next = !cpu_low;
			if (other_req)
				next_cycle = next_other;
			else if (|vid_rem)
					next_cycle = CYC_VIDEO;
            else
                next_cycle = CYC_FREE;
		end


// current cycle registering
	always @(posedge clk) if (c3)
		curr_cycle <= next_cycle;


// signals for the dram.v
    // attention: now wrdata is latched at c0 of current cycle (NOT at c3 of previous as LVD did)
	assign dram_wrdata = curr_dma ? dma_wrdata : {2{cpu_wrdata[7:0]}};
	assign dram_bsel[1:0] = next_dma ? 2'b11 : {cpu_wrbsel, ~cpu_wrbsel};
	assign dram_addr = next_cpu ? cpu_addr : next_vid ? video_addr : next_ts ? ts_addr : dma_addr;
	assign dram_req = !next_fre;
    assign dram_rnw = next_cpu ? cpu_rnw : next_dma ? dma_rnw : 1'b1;


// generation of read strobes: for video and cpu
	always @(posedge clk) if (c3)
		cpu_rnw_r <= cpu_rnw;


	always @(posedge clk)
		if (curr_cpu && cpu_rnw_r && c2)
			cpu_strobe <= 1'b1;
		else
			cpu_strobe <= 1'b0;


	assign video_next = curr_vid & c2;
	assign video_strobe = curr_vid & c3;

	assign ts_pre_next = curr_ts & c1;
	assign ts_next = curr_ts & c2;

	assign dma_next = curr_dma & c2;


endmodule

