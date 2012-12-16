`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Z80 memory manager: routes ROM/RAM accesses, makes wait-states for 14MHz or stall condition, etc.
//
//
// clk    _/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zclk     /```\___/```\___/```\___/```````\_______/```````\_______/```````````````\_______________/```````````````\_______________/`
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zpos     `\___/```\___/```\___/```\___________/```\___________/```\___________________________/```\___________________________/```\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zneg     _/```\___/```\___/```\_______/```\___________/```\___________________/```\___________________________/```\________________

module zmem(

	input  wire clk,
	input  wire rst,

	input  wire zpos, //
	input  wire zneg, // strobes which show positive and negative edges of zclk

	input  wire c0,
	input  wire c1,
	input  wire c2,
	input  wire c3,

	input  wire [15:0] za,
	input  wire [ 7:0] zd_in,
	output wire [ 7:0] zd_out,  // output to Z80 bus
	output wire zd_ena,         // output to Z80 bus enable

	input  wire opfetch,
	input  wire iorq_s,
	input  wire memwr_s,
	input  wire mreq,
    input  wire memrd,
    input  wire memwr,

	input  wire [ 1:0] turbo, 	  // 2'b00 - 3.5,
	                              // 2'b01 - 7.0,
	                              // 2'b1x - 14.0
	input wire [7:0] page,
	input wire rw_en,
	input wire cache_en,
	input wire csrom,

	output wire        romoe_n,
	output wire        romwe_n,

	input wire dos_on,
	input wire vdos_on,
	input wire vdos_off,
	
	input wire testkey,		// DEBUG!!!

	output wire        cpu_req,
	output wire        cpu_rnw,
	output wire [20:0] cpu_addr,
	output wire [ 7:0] cpu_wrdata,
	output wire        cpu_wrbsel,
	input  wire [15:0] cpu_rddata,
	input  wire        cpu_next,
	input  wire        cpu_strobe,
	input  wire        cpu_latch,
	output wire        cpu_stall,    // for zclock
	
	input wire intt,
	output wire [2:0] tst
);

	assign tst = {ramwrc_s, cpu_req, intt};
	// assign tst = ttt;
	// reg [2:0] ttt;
	// always@*
		// if (cpu_req)
			// ttt = 4;
		// else
			// ttt = 0;

// address, data in and data out
	assign cpu_wrbsel = za[0];
	assign cpu_addr[20:0] = {page[7:0], za[13:1]};
	assign cpu_wrdata = zd_in;
	wire [15:0] mem_d = cpu_latch ? cpu_rddata : cache_d;
	assign zd_out = ~cpu_wrbsel ? mem_d[7:0] : mem_d[15:8];


// Z80 controls
	assign romoe_n = !memrd;
	assign romwe_n = !(memwr && rw_en);

	wire ramreq = mreq && !csrom;
	wire ramrd = memrd && !csrom;
	wire ramwr = memwr && !csrom && rw_en;
	assign zd_ena = memrd && !csrom;


// strobe the beginnings of DRAM cycles
	wire dram_beg = (!cache_hit || memwr) && zneg && r_mreq_s_n;
	wire r_mreq_s_n = r_mreq_r_n && ramreq;

	reg r_mreq_r_n;
	always @(posedge clk)
		if (zneg)
			r_mreq_r_n <= !mreq;

	assign cpu_stall = turbo14 ? stall14 : stall357;


// 7/3.5MHz support
	wire cpureq_357 = (ramrd_s && !cache_hit) || ramwr_s;
	wire stall357 = cpureq_357 && !cpu_next;
	wire ramwr_s = ramwr && !ramwr_r;
	wire ramrd_s = ramrd && !ramrd_r;

	reg ramrd_r, ramwr_r;
	always @(posedge clk)
		if (c3 && !cpu_stall)
		begin
			ramrd_r <= ramrd;
			ramwr_r <= ramwr;
		end


// 14MHz support

	// wait tables:
	//
	// M1 opcode fetch, dram_beg coincides with:
	// c3:      +3
	// c2:      +4
	// c1:      +5
	// c0:      +6
	//
	// memory read, dram_beg coincides with:
	// c3:      +2
	// c2:      +3
	// c1:      +4
	// c0:      +5
	//
	// memory write: no wait
	//
	// special case: if dram_beg pulses 1 when cpu_next is 0,
	// unconditional wait has to be performed until cpu_next is 1, and
	// then wait as if dram_beg would coincide with c0

	wire turbo14 = turbo[1];

	wire stall14 = stall14_ini || stall14_cyc || stall14_fin;
	wire stall14_ini = dram_beg && (!cpu_next || opfetch || memrd);	// no wait at all in write cycles, if next dram cycle is available

	// memrd, opfetch - wait till c3 && cpu_next,
	// memwr - wait till cpu_next
	wire stall14_cyc = memwr ? !cpu_next : stall14_cycrd;

	reg stall14_cycrd;
	always @(posedge clk)
	if (rst)
		stall14_cycrd <= 1'b0;
	else
		if (cpu_next && c3)
			stall14_cycrd <= 1'b0;
		else if (dram_beg && (!c3 || !cpu_next) && (opfetch || memrd))
			stall14_cycrd <= 1'b1;

    wire [1:0] cc = &turbo ? {c1, c0} : {c2, c1};	// normal or overclock

	reg stall14_fin;
	always @(posedge clk)
	if (rst)
		stall14_fin <= 1'b0;
	else
	begin
		if (stall14_fin && ((opfetch && cc[0]) || (memrd && cc[1])))
			stall14_fin <= 1'b0;
		else if (cpu_next && c3 && cpu_req && (opfetch || memrd))
			stall14_fin <= 1'b1;
	end


// cpu request
	assign cpu_req = turbo14 ? (dram_beg || pending_cpu_req) : cpureq_357;
	assign cpu_rnw = turbo14 ? (dram_beg ? !memwr : cpu_rnw_r) : ramrd;

	reg pending_cpu_req;
	always @(posedge clk)
	if (rst)
		pending_cpu_req <= 1'b0;
	else if (cpu_next && c3)
		pending_cpu_req <= 1'b0;
	else if (dram_beg)
		pending_cpu_req <= 1'b1;

	reg cpu_rnw_r;
	always @(posedge clk)
	if( dram_beg )
		cpu_rnw_r <= !memwr;


// cache
	wire cache_hit = (cpu_hi_addr == cache_a) && cache_v && cache_en;
	// wire cache_hit = (cpu_hi_addr == cache_a) && cache_v && testkey;
	// wire cache_hit = (ch_addr[7:2] != 6'b011100) && (cpu_hi_addr == cache_a) && cache_v && testkey;
	wire cache_wr = ramwrc_s || cpu_strobe;
	wire ramwrc_s = memwr_s && !csrom && rw_en;
	wire [15:0] cache_d;
	wire cache_v;
	
	wire [12:0] cpu_hi_addr = {page[7:0], za[13:9]};
	wire [12:0] cache_a;
	wire [7:0] ch_addr = cpu_addr[7:0];
	// wire [14:0] cpu_hi_addr = {page[7:0], za[13:7]};
	// wire [14:0] cache_a;
	// wire [7:0] ch_addr = {2'b0, cpu_addr[5:0]};

cache_data cache_data (
		.clock (clk),
		.data (cpu_rddata),
		.rdaddress (ch_addr),
		.wraddress (ch_addr),
		.wren (cache_wr),
		.q (cache_d)
);

cache_addr cache_addr (
		.clock (clk),
		.data ({!ramwrc_s, cpu_hi_addr}),
		.rdaddress (ch_addr),
		.wraddress (ch_addr),
		.wren (cache_wr),
		.q ({cache_v, cache_a})
);

endmodule
