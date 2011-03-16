module test_compile(

	input clk,
	input rst_n, // shut down accesses, remain refresh

	output [9:0] ra, // to the DRAM pins
	inout  [15:0] rd, // .              .
	                     // .              .
	output rwe_n,    // .              .
	output rucas_n,  // .              .
	output rlcas_n,  // .              .
	output rras0_n,  // .              .
	output rras1_n,  // to the DRAM pins


	input go,
	input [1:0] bw,
	input  [20:0] video_addr,
	output [15:0] video_data,
	output video_strobe,
	input cpu_req,cpu_rnw,
	input  [20:0] cpu_addr,
	input   [7:0] cpu_wrdata,
	input         cpu_wrbsel,
	output [15:0] cpu_rddata,
	output cpu_stall,
	output [4:0] cpu_waitcyc,
	output cpu_strobe,

	output dram_cbeg
);


wire [20:0] dram_addr;
wire dram_req;
wire dram_rnw;
//wire dram_cbeg;
wire dram_rrdy;
wire [1:0] dram_bsel;
wire [15:0] dram_rddata;
wire [15:0] dram_wrdata;



	dram dramko( .clk(clk), .rst_n(rst_n), .ra(ra), .rd(rd), .rwe_n(rwe_n),
	             .rucas_n(rucas_n), .rlcas_n(rlcas_n), .rras0_n(rras0_n), .rras1_n(rras1_n),
	             .addr(dram_addr), .req(dram_req), .rnw(dram_rnw), .cbeg(dram_cbeg), .rrdy(dram_rrdy),
	             .rddata(dram_rddata), .wrdata(dram_wrdata), .bsel(dram_bsel) );

	arbiter arbitko( .clk(clk), .rst_n(rst_n),
	                 .dram_addr(dram_addr), .dram_req(dram_req), .dram_rnw(dram_rnw), .dram_cbeg(dram_cbeg), .dram_rrdy(dram_rrdy),
	                 .dram_rddata(dram_rddata), .dram_wrdata(dram_wrdata), .dram_bsel(dram_bsel),
	                 .go(go), .bw(bw), .video_addr(video_addr), .video_data(video_data), .video_strobe(video_strobe),
	                 .cpu_req(cpu_req), .cpu_rnw(cpu_rnw), .cpu_addr(cpu_addr), .cpu_wrdata(cpu_wrdata), .cpu_wrbsel(cpu_wrbsel),
	                 .cpu_rddata(cpu_rddata), .cpu_stall(cpu_stall), .cpu_waitcyc(cpu_waitcyc), .cpu_strobe(cpu_strobe) );




endmodule
