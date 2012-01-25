`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2011
//
// DRAM arbiter. Shares DRAM between processor and video data fetcher
//



// 20.11.2011:
// arbiter has been re-factored so it uses N_cycles out of 4/8


// 14.06.2011:
// removed cpu_stall and cpu_waitcyc.
// changed cpu_strobe behavior (only strobes read data arrival now).
// added cpu_next signal (shows whether next DRAM cycle CAN be grabbed by CPU)
//
// Now it is a REQUIREMENT for 'go' signal only starting and ending on
// beginning of DRAM cycle (i.e. right after 'c3' strobe).
//


// 13.06.2011:
// ѕридЄтс€ потребовать, чтоб go устанавливалс€ сразу после c3 (у мен€ [lvd] это так).
// это дл€ того, чтобы процессор на 14мгц мог заранее и в любой момент знать, на
// сколько завейтитьс€. ¬место cpu_ack введем другой сигнал, который в течение всего
// драм-цикла будет показывать, чей может быть следующий цикл - процессора или только
// видео. ѕо сути это и будет также cpu_ack, но валидный в момент cpu_req (т.е.
// в момент c3) и ранее.

// 12.06.2011:
// проблема: если цпу просит цикл чтени€, а его дать не могут,
// то он должен держать cpu_req. однако, сн€ть он его может
// только по cpu_strobe, при этом также отправитс€ еще один
// запрос чтени€!!!
// решение: добавить сигнал cpu_ack, по которому узнаЄтс€, что
// арбитр зохавал запрос (записи или чтени€), который будет
// совпадать с нынешним cpu_strobe на записи (c0), а будущий
// cpu_strobe сделать только как строб данных на зохаванном
// запросе чтен€.
// это, возможно, позволит удалить вс€кие cpu_waitcyc...


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
	input wire c2,
	input wire c3,
	input wire rst_n,

// dram.v interface
	output wire [20:0] dram_addr,   // address for dram access
	output reg         dram_req,    // dram request
	output reg         dram_rnw,    // Read-NotWrite
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
	input wire        dma_rnw,
	output reg        dma_next,
    output reg        dma_strobe,
	
// TS engine	
	input wire [20:0] ts_addr,
	input wire 	      ts_req,
	output wire       ts_next,
	output wire       ts_strobe

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
// - video
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
	




	initial // simulation only!
	begin
		curr_cycle = CYC_FREE;
		blk_rem = 0;
		vid_rem = 0;
	end




	// assign c0 = dram_c0; // just alias

	wire bw_full = ~|{video_bw[4] & video_bw[2], video_bw[3] & video_bw[1], video_bw[0]}; // stall when 000/00/0

	// track blk_rem counter: how many cycles left to the end of block (7..0)
	always @(posedge clk) if( c3 )
	begin
		blk_rem <= blk_nrem;

		if( (blk_rem==3'd0) )
			stall <= bw_full & go;
	end

	always @*
	begin
		if( (blk_rem==3'd0) && go )
			blk_nrem = {video_bw[4:3], 1'b1};	// remaining 7/3/1
			// blk_nrem = 7;	
		else
			blk_nrem = (blk_rem==0) ? 3'd0 : (blk_rem-3'd1);
	end



	// track vid_rem counter
	assign vidmax = {video_bw[2:0]}; // number of cycles for video access
	// assign vidmax = (3'b001) << bw; // number of cycles to perform

	always @(posedge clk) if( c3 )
	begin
		vid_rem <= vid_nrem;
	end

	always @*
	begin
		if( go && (blk_rem==3'd0) )
			vid_nrem = cpu_req ? vidmax : (vidmax-3'd1);
		else
			if( next_vid )
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
				if( bw_full )
				begin
					cpu_next = 1'b0;

					next_cycle = CYC_VIDEO;
				end
				else
				begin
					cpu_next = 1'b1;

					if( cpu_req )
						next_cycle = CYC_CPU;
					else
						next_cycle = CYC_VIDEO;
				end
			end
			else // !go
			begin
				cpu_next = 1'b1;

				if( cpu_req )
					next_cycle = CYC_CPU;
				else
				if (ts_req)
					next_cycle = CYC_TS;
				else
				if (dma_req)
					next_cycle = CYC_DMA;
				else
					next_cycle = CYC_FREE;
			end
		end
		else // blk_rem!=3'd0
		begin
			if( stall )
			begin
				cpu_next = 1'b0;

				next_cycle = CYC_VIDEO;
			end
			else
			begin
				if( vid_rem==blk_rem )
				begin
					cpu_next = 1'b0;
	
					next_cycle = CYC_VIDEO;
				end
				else
				begin
					cpu_next = 1'b1;
	
					if( cpu_req )
						next_cycle = CYC_CPU;
					else
						if (|vid_rem)
							next_cycle = CYC_VIDEO;
						else
                            if (ts_req)
                                next_cycle = CYC_TS;
                            else
                            if (dma_req)
                                next_cycle = CYC_DMA;
                            else
                                next_cycle = CYC_FREE;
				end
			end
		end
	end




	// just current cycle registering
	always @(posedge clk) if( c3 )
	begin
		curr_cycle <= next_cycle;
	end




	// route required data/etc. to and from the dram.v

	assign dram_wrdata = curr_dma ? dma_wrdata : {2{cpu_wrdata[7:0]}};
	assign dram_bsel[1:0] = next_dma ? 2'b11 : {cpu_wrbsel, ~cpu_wrbsel};
	assign dram_addr = next_cpu ? cpu_addr : next_vid ? video_addr : next_ts ? ts_addr : dma_addr;
	// assign dram_addr = next_cpu ? cpu_addr : video_addr;


	always @*
	begin
		if( next_fre ) // CYC_FREE
		begin
			dram_req = 1'b0;
			dram_rnw = 1'b1;
		end
		else
		begin
			dram_req = 1'b1;
			if( next_cpu )
				dram_rnw = cpu_rnw;
            else
			if( next_dma )
				dram_rnw = dma_rnw;
			else
				dram_rnw = 1'b1;
		end
	end



	// generation of read strobes: for video and cpu


	always @(posedge clk) if( c3 )
		cpu_rnw_r <= cpu_rnw;


	always @(posedge clk)
	begin
		if( curr_cpu && cpu_rnw_r && c2 )
			cpu_strobe <= 1'b1;
		else
			cpu_strobe <= 1'b0;
	end


	assign video_next   = curr_vid & c2;
	assign video_strobe = curr_vid & c3;

	assign ts_next   = curr_ts & c2;
	assign ts_strobe = curr_ts & c3;

	assign dma_next   = curr_dma & c2;
	assign dma_strobe = curr_dma & c3;

	
endmodule

