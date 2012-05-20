`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// DRAM arbiter. Shares DRAM between CPU, video data fetcher and other devices
//

// Arbitration is made on full 8-cycle access blocks. Each cycle is defined by dram.v and consists of 4 fpga clocks.
// During each access block, there can be either no videodata access, 1 videodata access, 2, 4 or 8 accesses.
// All spare cycles can be used by CPU or other devices. If no device uses memory in the given cycle, refresh cycle is performed.
//
// In each access block, videodata accesses are spreaded all over the block so that CPU receives cycle
// as fast as possible, until there is absolute need to fetch remaining video data.
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
// key signals are go and XXX_req, sampled at the end of each dram cycle. Must be set to the module at c3 clock cycle.

// CPU can have either normal or lower access priority to the DRAM.
// At the INT active (32 of 3.5MHz clocks) the priority is raised to normal, so that CPU won't miss its interrupt.
// This should be considered if dummy RAM access used for waiting for the end of DMA operation instead of status bit polling.
//
// DRAM access priority:
// Z80 normal       Z80 low
// - VIDEO          - VIDEO
// - CPU            - TS
// - TS             - DMA
// - DMA            - CPU


module arbiter(

	input wire clk,
	input wire c1,
	input wire c2,
	input wire c3,
	input wire rst_n,
	input wire int_n,

// dram.v interface
	output wire [20:0] dram_addr,   // address for dram access
	output wire        dram_req,    // dram request
	output wire        dram_rnw,    // Read-NotWrite
	output wire  [1:0] dram_bsel,   // positive bytes select: bsel[1] for wrdata[15:8], bsel[0] for wrdata[7:0]
	output wire [15:0] dram_wrdata, // data to be written

// video
	input  [20:0] video_addr,   // during access block, only when video_strobe==1
	output wire   video_next,   // on this signal you can change video_addr; it is one clock leading the video_strobe
	output wire   video_pre_next,
	input wire go, 				// start video access blocks
	output wire   video_strobe, // positive one-cycle strobe as soon as there is next video_data available.
	                            // if there is video_strobe, it coincides with c3 signal
	input wire [4:0] video_bw,
								// [4:3] - total cycles: 11 = 8 / 01 = 4 / 00 = 2
								// [2:0] - need cycles
	output wire next_video,
	
// CPU
	input wire [20:0] cpu_addr,
	input wire [ 7:0] cpu_wrdata,
	input wire        cpu_req,
	input wire        cpu_rnw,
	input wire        cpu_wrbsel,
	output reg        cpu_next,
    output wire       cpu_strobe,

// DMA
	input wire [20:0] dma_addr,
	input wire [15:0] dma_wrdata,
	input wire        dma_req,
	input wire        dma_zwt,
	input wire        dma_rnw,
	output reg        dma_next,

// TS
	input wire [20:0] ts_addr,
	input wire 	      ts_req,
	input wire 	      ts_zwt,
	output wire       ts_pre_next,
	output wire       ts_next

);


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
//	wire curr_fre = ~|curr_cycle;

	assign next_video = next_vid;
	

// track blk_rem counter:
// how many cycles left to the end of block (7..0)
	wire [2:0] blk_nrem = (blk_start && go) ? {video_bw[4:3], 1'b1} : (blk_start ? 3'd0 : (blk_rem - 3'd1));
	wire bw_full = ~|{video_bw[4] & video_bw[2], video_bw[3] & video_bw[1], video_bw[0]}; // stall when 000/00/0
    wire blk_start = ~|blk_rem;

	reg [2:0] blk_rem;       // remaining accesses in a block (7..0)
	reg stall;
	always @(posedge clk) if (c3)
	begin
		blk_rem <= blk_nrem;
		if (blk_start)
			stall <= bw_full & go;
	end


// track vid_rem counter
// how many video cycles left to the end of block (7..0)
	wire [2:0] vid_nrem = (go && blk_start) ? (other_req ? vidmax : (vidmax - 3'd1)) : (next_vid ? vid_nrem_next : vid_rem);
	wire [2:0] vidmax = {video_bw[2:0]};    // number of cycles for video access
	wire [2:0] vid_nrem_next = ~|vid_rem ? 3'd0 : (vid_rem - 3'd1);

	reg [2:0] vid_rem;      // remaining video accesses in block
	always @(posedge clk) if (c3)
		vid_rem <= vid_nrem;


// next cycle decision
    wire cpu_low = ((ts_req & ts_zwt) | (dma_req & dma_zwt)) & int_n;
    wire other_req = cpu_req | ts_req | dma_req;
    wire [3:0] next_other = cpu_low ? next_other_low : next_other_norm;
    wire [3:0] next_other_norm = cpu_req ? CYC_CPU : (ts_req ? CYC_TS : CYC_DMA);
    wire [3:0] next_other_low = ts_req ? CYC_TS : (dma_req ? CYC_DMA : CYC_CPU);
    wire video_only = stall | (vid_rem == blk_rem);

	always @*
		if (blk_start)      // video burst start
			if (go)             // video started
			begin
				cpu_next = bw_full ? 1'b0 : !cpu_low;
				next_cycle = bw_full ? CYC_VIDEO : (other_req ? next_other : CYC_VIDEO);
			end

			else                // video idle
			begin
				cpu_next = !cpu_low;
				next_cycle = other_req ? next_other : CYC_FREE;
			end

		else                // video burst in progress
		begin
			cpu_next = video_only ? 1'b0 : !cpu_low;
			next_cycle = video_only ? CYC_VIDEO : (other_req ? next_other : (|vid_rem ? CYC_VIDEO : CYC_FREE));
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

	reg cpu_rnw_r;
	always @(posedge clk) if (c3)
		cpu_rnw_r <= cpu_rnw;


// generation of read strobes: for video and cpu
    assign cpu_strobe = curr_cpu && cpu_rnw_r && c2;

	assign video_pre_next = curr_vid & c1;
	assign video_next = curr_vid & c2;
	assign video_strobe = curr_vid & c3;

	assign ts_pre_next = curr_ts & c1;
	assign ts_next = curr_ts & c2;

	assign dma_next = curr_dma & c2;


endmodule
