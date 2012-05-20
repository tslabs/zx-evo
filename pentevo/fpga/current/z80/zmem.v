`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// Z80 memory manager: routes ROM/RAM accesses, makes wait-states for 14MHz or stall condition, etc.
//
//
// fclk    _/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\_/`\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zclk     /```\___/```\___/```\___/```````\_______/```````\_______/```````````````\_______________/```````````````\_______________/`
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zpos     `\___/```\___/```\___/```\___________/```\___________/```\___________________________/```\___________________________/```\
//          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// zneg     _/```\___/```\___/```\_______/```\___________/```\___________________/```\___________________________/```\________________

module zmem(

	input  wire fclk,
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
	input  wire iorq,
	input  wire mreq,
    input  wire memrd,
    input  wire memwr,
	input  wire rd,
	input  wire wr,

	input  wire [ 1:0] int_turbo, // 2'b00 - 3.5,
	                              // 2'b01 - 7.0,
	                              // 2'b1x - 14.0
	input wire [7:0] page,
	input wire rw_en,
	input wire romnram,

	output wire        romoe_n,
	output wire        romwe_n,
	output wire        csrom,

	input wire dos_on,
	input wire vdos_on,
	input wire vdos_off,

	output wire        cpu_req,
	output wire        cpu_rnw,
	output wire [20:0] cpu_addr,
	output wire [ 7:0] cpu_wrdata,
	output wire        cpu_wrbsel,
	input  wire [15:0] cpu_rddata,
	input  wire        cpu_next,
	input  wire        cpu_strobe,
	output wire        cpu_stall    // for zclock

);


	reg [15:0] rd_buf;

	reg [15:1] cached_addr;
	reg cached_addr_valid;

	reg stall14_cycrd;
	reg stall14_fin;
	reg r_mreq_n;
	reg pending_cpu_req;
	reg cpu_rnw_r;


	// this is for 7/3.5mhz
	wire cpureq_357 = ( ramrd & (~ramrd_reg) ) | ( ramwr & (~ramwr_reg) );
	reg ramrd_reg,ramwr_reg;


	assign romwe_n = !(wr & mreq & rw_en);
	assign romoe_n = !(rd & mreq);
	assign csrom = romnram; // positive polarity!


	// 7/3.5mhz support
	wire ramreq = mreq && !romnram;
	wire ramrd = ramreq & rd;
	wire ramwr = ramreq & wr & rw_en;

	always @(posedge fclk)
	if( c3 && (!cpu_stall) )
	begin
		ramrd_reg <= ramrd;
		ramwr_reg <= ramwr;
	end


	assign zd_ena = mreq & rd & !romnram;

	wire cache_hit = ( (za[15:1] == cached_addr[15:1]) && cached_addr_valid );


// strobe the beginnings of DRAM cycles
	always @(posedge fclk)
	if (zneg)
		r_mreq_n <= !mreq;
	//
	wire dram_beg = ( (!cache_hit) || memwr ) && zneg && r_mreq_n && (!romnram) && mreq;


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

	wire stall14_ini = dram_beg && ( (!cpu_next) || opfetch || memrd ); // no wait at all in write cycles, if next dram cycle is available


	// memrd, opfetch - wait till c3 & cpu_next,
	// memwr - wait till cpu_next
	wire stall14_cyc = memwr ? (!cpu_next) : stall14_cycrd;

	always @(posedge fclk)
	if (rst)
		stall14_cycrd <= 1'b0;
	else // posedge fclk
	begin
		if( cpu_next && c3 )
			stall14_cycrd <= 1'b0;
		else if( dram_beg && ( (!c3) || (!cpu_next) ) && (opfetch || memrd) )
			stall14_cycrd <= 1'b1;
	end

    
    wire [1:0] cc = &int_turbo ? {c1, c0} : {c2, c1};
    
	always @(posedge fclk)
	if (rst)
		stall14_fin <= 1'b0;
	else // posedge fclk
	begin
		// if (stall14_fin && ((opfetch & c2) || (memrd & c1)))     // normal
		// if (stall14_fin && ((opfetch & c1) || (memrd & c0)))     // overclock boost!!!
		if (stall14_fin && ((opfetch & cc[0]) || (memrd & cc[1])))
			stall14_fin <= 1'b0;
		else if( cpu_next && c3 && cpu_req && (opfetch || memrd) )
			stall14_fin <= 1'b1;
	end


	assign cpu_stall = int_turbo[1] ? (stall14_ini | stall14_cyc | stall14_fin) : (cpureq_357 && (!cpu_next));

	// cpu request
	assign cpu_req = int_turbo[1] ? (pending_cpu_req | dram_beg) : cpureq_357;
	assign cpu_rnw = int_turbo[1] ? (dram_beg ? (!memwr) : cpu_rnw_r) : ramrd;

	always @(posedge fclk)
	if (rst)
		pending_cpu_req <= 1'b0;
	else if( cpu_next && c3 )
		pending_cpu_req <= 1'b0;
	else if( dram_beg )
		pending_cpu_req <= 1'b1;
	//
	always @(posedge fclk)
	if( dram_beg )
		cpu_rnw_r <= !memwr;


	// address, data in and data out
	assign cpu_wrbsel = za[0];
	assign cpu_addr[20:0] = { page[7:0], za[13:1] };
	assign cpu_wrdata = zd_in;
	assign zd_out = ~cpu_wrbsel ? rd_buf[7:0] : rd_buf[15:8];

	always @* if( cpu_strobe ) // WARNING! ACHTUNG! LATCH!!!
		rd_buf <= cpu_rddata;

	wire io = iorq;
	reg io_r;
	always @(posedge fclk)
	if( zpos )
		io_r <= io;

	always @(posedge fclk)
	if (rst)
	begin
		cached_addr_valid <= 1'b0;
	end


	else
	begin
		if( (zneg && r_mreq_n && mreq && romnram)
		 || (zneg && r_mreq_n && memwr)
		 || (io && (!io_r) && zpos)
         || (vdos_off | vdos_on)
         || dos_on
        )
			cached_addr_valid <= 1'b0;
		else if( cpu_strobe )
			cached_addr_valid <= 1'b1;
	end

	always @(posedge fclk)
	if (rst)
	begin
		cached_addr <= 15'd0;
	end
	else if( cpu_strobe )
	begin
		cached_addr[15:1] <= za[15:1];
	end


endmodule
