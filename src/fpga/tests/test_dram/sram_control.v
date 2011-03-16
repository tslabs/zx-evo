/*
  read sequence

clk   ``\____/````\____/` ..... _/````\____/````\____/` ..... _/````\____/````\____/`
             |         |         |         |         |         |         |
start XXXX```````````\__ ....... ____________________________________________________
             |         |         |         |         |         |         |
rnw   XXXXXX```XXXXXXXXX ....... XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
             |         | some    |         |         |         |         |
ready XXXXXXX\__________ clocks __/``````````````````  ....... ```````````\__________
                         before                                |         |
rdat  ------------------ ready  -< cell 0  | cell 1  | ....... |last cell>-----------
             |         |         |         |         |         |         |
stop  XXXXXXX\__________ ....... _____________________ ....... ___________/``````````
                                                                            ^all operations stopped until next start strobe



  write sequence

clk   ``\____/````\____/` ..... _/````\____/````\____/````\____/````\____/````\____/````\____/````\____/
             |         | some    |         | some    |         |         |         |         |         |
start XXXX```````````\__ ....... _____________ .... ______________ .... ________________________________
             |         | clocks  |         | clocks  |         |         |         |         |         |
rnw   XXXXXX___XXXXXXXXX ....... XXXXXXXXXXXXX .... XXXXXXXXXXXXXX .... XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
             |         | before  |         | before  |         |         |         |         |         |
ready XXXXXXX\__________ ....... _/`````````\_ .... __/`````````\_ .... __/`````````\___________________
             |         | first   |         | next    |         |         |         |         |         |
wdat  XXXXXXXXXXXXXXXXXXXXXXXXXXXX< cell 0  >X .... XX< cell 1  >X .... XX<last cell>XXXXXXXXXXXXXXXXXXX
             |         | ready   |         | ready   |         |         |         |         |         |
stop  XXXXXXX\__________ ....... _____________ .... ______________ .... ____________/```````````````````
             |         | strobe  |         | strobe  |         |         |         |         |         |




clk   ``\____/````\____/````\____/````\____/````\____/````\____/````\____/````\____/````\____/````\____/````\____/````\____/``
             |         |         |         |         |         |         |         |         |         |         |         |
ready __________________/`````````\___________________/`````````\___________________/`````````\___________________/`````````\_
             |         |         |         |         |         |         |         |         |         |         |         |
wdat           cell 0             | cell 1                      | cell 2                      | cell 3                      |
             |         |         |         |         |         |         |         |         |         |         |         |
sram_adr XXXXXXXXXXXXXXXXXXXXXXXXX| 0                           | 1                           | 2                           |
             |         |         |         |         |         |         |         |         |         |         |         |
sram_dat XXXXXXXXXXXXXXXXXXXXXXXXX| cell 0                      | cell 1                      | cell 2                      |
             |         |         |         |         |         |         |         |         |         |         |         |
sram_we_n```````````````````````````````````\_________/```````````````````\_________/```````````````````\_________/``````````
             | BEG     | PRE1    | PRE2    |         |         |         |         |         |         |         |         |
             |         |         | CYC1    | CYC2    | CYC3    | CYC1    | CYC2    | CYC3    | CYC1    | CYC2    | CYC3    |






*/


module sram_control(

	clk,
	clk2, //latching of SRAM data out

	start, // initializing input, address=0

	stop, // when all addresses are done, nothing will happen after stop is set, need another start signal

	rnw, // 1 - read, 0 - write sequence (latched when start=1)

	ready, // strobe. when writing, one mean that data from wdat written to the memory (2^SRAM_ADDR_SIZE strobes total)
	       // when reading, one mean that data read from memory is on rdat output (2^SRAM_ADDR_SIZE strobes total)


	wdat, // input, data to be written to memory
	rdat, // output, data last read from memory



	SRAM_DQ,   // sram inout databus

	SRAM_ADDR, // sram address bus

	SRAM_UB_N, // sram control signals
	SRAM_LB_N, //
	SRAM_WE_N, //
	SRAM_CE_N, //
	SRAM_OE_N  //
);

parameter SRAM_DATA_SIZE = 16;
parameter SRAM_ADDR_SIZE = 18;


	input clk;
	input clk2;

	input start,rnw;

	output stop;
	reg    stop;

	output ready;
	reg    ready;

	input [SRAM_DATA_SIZE-1:0] wdat;

	output [SRAM_DATA_SIZE-1:0] rdat;
	reg    [SRAM_DATA_SIZE-1:0] rdat;


	inout [SRAM_DATA_SIZE-1:0] SRAM_DQ;
	reg   [SRAM_DATA_SIZE-1:0] SRAM_DQ;

	output [SRAM_ADDR_SIZE-1:0] SRAM_ADDR;
	wire   [SRAM_ADDR_SIZE-1:0] SRAM_ADDR;

	output SRAM_UB_N,SRAM_LB_N,SRAM_WE_N,SRAM_CE_N,SRAM_OE_N;
	reg    SRAM_UB_N,SRAM_LB_N,SRAM_WE_N,SRAM_CE_N,SRAM_OE_N;


	reg [SRAM_DATA_SIZE-1:0] wdat2;
	reg dbin; //data bus direction control

	reg  [SRAM_ADDR_SIZE:0] sram_addr_ctr; // one bit bigger to have stop flag
	wire [SRAM_ADDR_SIZE:0] sram_addr_nxt; // next sram address


	reg [SRAM_DATA_SIZE-1:0] rdat2;

	assign SRAM_ADDR = sram_addr_ctr[SRAM_ADDR_SIZE-1:0];

	assign sram_addr_nxt = sram_addr_ctr + 1;


	// data bus control
	always @*
	begin
		if( dbin )
			SRAM_DQ <= 'hZ;
		else // !dbin
			SRAM_DQ <= wdat2;
	end

	always @(posedge clk2) // clk2!!!! late latching
	begin
		rdat2 <= SRAM_DQ;
	end

	always @(posedge clk)
	begin
		rdat <= rdat2;
	end


	always @(posedge clk)
	begin
		if( ready ) wdat2 <= wdat;
	end





	reg [3:0] curr_state,next_state;

	parameter START_STATE = 4'd00; // reset state

	parameter INIT_STATE  = 4'd01; // initialization state

	parameter READ_BEG    = 4'd02; // read branch: prepare signals
	parameter READ_PRE    = 4'd13;
	parameter READ_CYCLE  = 4'd03; // read in progress: increment address, set ready, out data, do so until all addresses done
	parameter READ_POST   = 4'd14;
	parameter READ_END    = 4'd04; // read end: deassert some signals, go to stop state

	parameter WRITE_BEG   = 4'd05; // prepare signals
	parameter WRITE_PRE1  = 4'd06; // assert ready
	parameter WRITE_PRE2  = 4'd07; // capture wdat, negate ready, NO INCREMENT address, next state is WRITE_CYC2
	parameter WRITE_CYC1  = 4'd08; // capture wdat, negate ready, increment address
	parameter WRITE_CYC2  = 4'd09; // assert SRAM_WE_N, go to WRITE_END if sram_addr_nxt is out of memory region
	parameter WRITE_CYC3  = 4'd10; // negate SRAM_WE_N, assert ready (wdat will be captured in WRITE_CYC1)
	parameter WRITE_END   = 4'd11; // deassert sram control signals, go to STOP_STATE


	parameter STOP_STATE  = 4'd12; // full stop state



	// FSM states
	always @*
	begin
		case( curr_state )

////////////////////////////////////////////////////////////////////////
		START_STATE:
			next_state = INIT_STATE;


////////////////////////////////////////////////////////////////////////
		INIT_STATE:
		begin
			if( rnw ) // read
				next_state = READ_BEG;
			else // !rnw - write
				next_state = WRITE_BEG;
		end




////////////////////////////////////////////////////////////////////////
		READ_BEG:
			next_state = READ_PRE;

		READ_PRE:
			next_state = READ_CYCLE;

		READ_CYCLE:
			if( !sram_addr_ctr[SRAM_ADDR_SIZE] )
				next_state = READ_CYCLE;
			else
				next_state = READ_POST;

		READ_POST:
			next_state = READ_END;

		READ_END:
			next_state = STOP_STATE;



////////////////////////////////////////////////////////////////////////
		WRITE_BEG:
			next_state = WRITE_PRE1;

		WRITE_PRE1:
			next_state = WRITE_PRE2;

		WRITE_PRE2:
			next_state = WRITE_CYC2;


		WRITE_CYC1:
			next_state = WRITE_CYC2;

		WRITE_CYC2:
			if( !sram_addr_nxt[SRAM_ADDR_SIZE] )
				next_state = WRITE_CYC3;
			else
				next_state = WRITE_END;

		WRITE_CYC3:
			next_state = WRITE_CYC1;

		WRITE_END:
			next_state = STOP_STATE;



////////////////////////////////////////////////////////////////////////
		STOP_STATE:
			next_state = STOP_STATE;




////////////////////////////////////////////////////////////////////////
		default:
			next_state = STOP_STATE;

		endcase

	end



	// FSM flip-flops
	always @(posedge clk)
	begin
		if( start )
			curr_state <= START_STATE;
		else
			curr_state <= next_state;
	end


	// FSM outputs
	always @(posedge clk)
	begin
		case( next_state )

////////////////////////////////////////////////////////////////////////
		INIT_STATE:
		begin
			stop <= 1'b0;

			SRAM_UB_N <= 1'b1;
			SRAM_LB_N <= 1'b1;
			SRAM_CE_N <= 1'b1;
			SRAM_OE_N <= 1'b1;
			SRAM_WE_N <= 1'b1;

			dbin <= 1'b1;

			sram_addr_ctr <= 0;

			ready <= 1'b0;
		end



////////////////////////////////////////////////////////////////////////
		READ_BEG:
		begin
			SRAM_UB_N <= 1'b0;
			SRAM_LB_N <= 1'b0;
			SRAM_CE_N <= 1'b0;
			SRAM_OE_N <= 1'b0;
		end

		READ_PRE:
		begin
                  sram_addr_ctr <= sram_addr_nxt;
		end

		READ_CYCLE:
		begin
			ready <= 1'b1;

                  sram_addr_ctr <= sram_addr_nxt;
		end

		READ_POST:
		begin
			ready <= 1'b0; // in read sequence, ready and data are 2 cycles past the actual read.
		end

		READ_END:
		begin
			SRAM_UB_N <= 1'b1;
			SRAM_LB_N <= 1'b1;
			SRAM_CE_N <= 1'b1;
			SRAM_OE_N <= 1'b1;

			ready <= 1'b0;
		end




////////////////////////////////////////////////////////////////////////
		WRITE_BEG:
		begin
			SRAM_UB_N <= 1'b0;
			SRAM_LB_N <= 1'b0;
			SRAM_CE_N <= 1'b0;

			dbin <= 1'b0;
		end

		WRITE_PRE1:
		begin
			ready <= 1'b1;
		end

		WRITE_PRE2:
		begin
			ready <= 1'b0;
		end


		WRITE_CYC1:
		begin
			ready <= 1'b0;

			sram_addr_ctr <= sram_addr_nxt;
		end

		WRITE_CYC2:
		begin
			SRAM_WE_N <= 1'b0;
		end

		WRITE_CYC3:
		begin
			SRAM_WE_N <= 1'b1;

			ready <= 1'b1;
		end

		WRITE_END:
		begin
			ready <= 1'b0;

			SRAM_WE_N <= 1'b1;
			SRAM_UB_N <= 1'b1;
			SRAM_LB_N <= 1'b1;
			SRAM_CE_N <= 1'b1;
		end


////////////////////////////////////////////////////////////////////////
		STOP_STATE:
		begin
			stop <= 1'b1;
		end

		endcase
	end


endmodule

