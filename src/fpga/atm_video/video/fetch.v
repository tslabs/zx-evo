`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// fetches and outs video data
//
// currently only 256x192 standard zx and p16c modes supported
//
//

module fetch(

	input clk,


	input cend,

	input line_start,
	input vpix,       // hpix not needed! fetch.v has its own count from line_start to the hpix window opening

	input int_start,  // used as initializer for counters



	input [1:0] vmode, // 2'b00 - standard ZX, 2'b01 - hardware multicolor, 2'b1x - p16c

	input screen, // screen 0 (pg5) or screen 1 (pg7)


	// controlling data fetch (go to dram/arbiter.v)
	output reg  [20:0] video_addr,
	input       [15:0] video_data,
	input              video_strobe,
	input              video_next,
	output      [ 1:0] bw,
	output reg         go,



	output [3:0] pixel
);


	localparam START_FETCH = 6'd36;


// begin data fetching 16 cycles before actual start if hpix, (52-16)=36 cycles after line_start.

	reg [15:0] fbuf [0:3]; // reading memory data

	reg [7:0] shift [0:7]; // shifting out pixel data



	reg [7:0] vcnt; // vertical line counter (0-191)

	reg [3:0] hcnt; // horizontal word counter (0-15)
	reg [1:0] dcnt; // displacement or pix/attr counter
	reg [1:0] ddcnt; // dcnt saved for data reception


	reg [5:0] scnt; // start-fetch counter

	wire wordsync; // synchronize words output: every 16th dram cycle
	reg [3:0] wcnt; // word (16 cycles) counter

	reg [4:0] fcnt; // fetches counter; counts from 0 to 16 as each 16cycles passes, at the end stops fetching by issuing go_end


	wire go_start,go_end;


	reg [3:0] pixnumber; // pixel number; 0 - leftmost, 15 - rightmost


	wire flash;
	reg [4:0] flashctr;


	reg [3:0] zxcolor;





	initial
	begin
		go = 1'b0;

		vcnt = 8'd0;
		hcnt = 4'd0;
		dcnt = 2'd0;
		ddcnt = 2'd0;
		scnt = 6'd0;
		wcnt = 4'd0;
		fcnt = 5'd0;
		pixnumber = 4'd0;
		flashctr = 6'd0;
	end










	// flash generator
	always @(posedge clk) if( int_start )
	begin
		flashctr <= flashctr + 5'd1;
	end
	assign flash = flashctr[4];


	// fetchmode generator

	assign bw = vmode[1] ? 2'b01 : 2'b00;




	// vertical counter
	always @(posedge clk)
	begin
		if( int_start )
			vcnt <= 8'hFF;
		else if( line_start && vpix ) // line-start will also happen in the first pixel line, so we initialize counter with 0xFF
			vcnt <= vcnt + 8'h01;
	end


	// start-fetch counter
	always @(posedge clk) if( cend )
	begin
		if( line_start && vpix )
			scnt <= (START_FETCH - 6'd2);
		else if( scnt!=6'd0)
			scnt <= scnt - 6'd1;
	end
	assign go_start = (scnt==6'd1) & cend; // start go signal 1 cycle before actual data will begin to fetch


	//wordsync generation and synchronizing
	always @(posedge clk) if( cend )
	begin
		if( go_start )
			wcnt <= 4'd0;
		else
			wcnt <= wcnt + 4'd1;
	end
	assign wordsync = (wcnt==4'd1) & cend;

	// fetch counter
	always @(posedge clk)
	begin
		if( go_start )
			fcnt <= 5'd0;
		else if( wordsync && (~fcnt[4]) )
			fcnt <= fcnt + 5'd1;
	end
	assign go_end = fcnt[4] & (wcnt==4'd15) & cend;


	// go
	always @(posedge clk)
		if( go_start )
			go <= 1'b1;
		else if( go_end )
			go <= 1'b0;



	// horizontal address counters: init and increment
	always @(posedge clk)
	begin
		if( line_start ) // initialize counters
		begin
			hcnt <= 4'd0;
			dcnt <= 2'd0;
		end
		else if( video_next )
		begin
			ddcnt <= dcnt;

			dcnt <= dcnt + 2'd1;

			if( !vmode[1] ) // normal ZX standard
			begin
				if( dcnt[0] )
					hcnt <= hcnt + 4'd1;
			end
			else // vmode[1] - p16c mode
			begin
				if( dcnt==2'd3 )
					hcnt <= hcnt + 4'd1;
			end
		end
	end


	// store fetched data
	always @(posedge clk) if( video_strobe )
	begin
		if( !vmode[1] ) // ZX mode
			fbuf[{1'b0,ddcnt[0]}] <= video_data;
		else // p16c mode
			fbuf[ddcnt[1:0]] <= video_data;
	end


	// sequence video addresses
	always @*
	begin
		video_addr[20:14] = { 6'b000001,screen }; // common part of address

		if( !vmode[1] ) // ZX mode
		begin
			if( !dcnt[0] ) // pixel addr
				video_addr[13:0] = { 2'b10, vcnt[7:6], vcnt[2:0], vcnt[5:3], hcnt[3:0] };
			else // attr addr
			begin
				if( !vmode[0] ) // normal ZX
					video_addr[13:0] = { 5'b10110, vcnt[7:3], hcnt[3:0] };
				else // hardware multicolor
					video_addr[13:0] = { 2'b11, vcnt[7:6], vcnt[2:0], vcnt[5:3], hcnt[3:0] };
			end
		end
		else // p16c mode
			begin
				video_addr[13:0] = { dcnt[0], dcnt[1], vcnt[7:6], vcnt[2:0], vcnt[5:3], hcnt[3:0] };
			end
	end

	// pass read data to pixel engine
	always @(posedge clk) if( wordsync )
	begin
		shift[0] <= fbuf[0][15:8];
		shift[1] <= fbuf[0][ 7:0];
		shift[2] <= fbuf[1][15:8];
		shift[3] <= fbuf[1][ 7:0];
		shift[4] <= fbuf[2][15:8];
		shift[5] <= fbuf[2][ 7:0];
		shift[6] <= fbuf[3][15:8];
		shift[7] <= fbuf[3][ 7:0];
	end

	// count pixels to be out
	always @(posedge clk) if( cend )
		if( wordsync )
			pixnumber <= 4'd0;
		else
			pixnumber <= pixnumber + 4'd1;





	// out pixels

	reg [7:0] pixbyte,attrbyte;

	reg [3:0] pix0,pix1;


	always @*
	begin
		attrbyte = shift[ { 2'b01, pixnumber[3] } ][7:0];
		pix0 = { attrbyte[6],attrbyte[5:3] };
		pix1 = { attrbyte[6],attrbyte[2:0] };

		if( !vmode[1] ) // ZX mode
		begin
			// attribute is taken

			// pixels
			pixbyte = shift[ { 2'b00, pixnumber[3] } ][7:0];

			zxcolor = ( pixbyte[(~pixnumber[2:0])] ^ (flash & attrbyte[7]) ) ? pix1 : pix0;
		end
		else // p16c mode
		begin
			pixbyte = shift[ { pixnumber[2:1], pixnumber[3] } ][7:0];

			zxcolor = pixnumber[0] ? { pixbyte[7],pixbyte[5:3] } : { pixbyte[6],pixbyte[2:0] };
		end
	end

        assign pixel = zxcolor;






endmodule


