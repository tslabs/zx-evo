// PentEvo project (c) NedoPC 2012
//
// SPI hub: arbitrating between AVR and Z80 accesses to SDcard via SPI.

`include "../include/tune.v"

module spihub(

	input  wire       fclk,
	input  wire       rst_n,

	// pins to SDcard
	output reg        sdcs_n,
	output wire       sdclk,
	output wire       sddo,
	input  wire       sddi,

	// zports SDcard iface
	input  wire       zx_sdcs_n_val,
	input  wire       zx_sdcs_n_stb,
	input  wire       zx_sd_start,
	input  wire [7:0] zx_sd_datain,
	output wire [7:0] zx_sd_dataout,

	// slavespi SDcard iface
	input  wire       avr_lock_in,
	output reg        avr_lock_out,
	input  wire       avr_sdcs_n,
	input  wire       avr_sd_start,
	input  wire [7:0] avr_sd_datain,
	output wire [7:0] avr_sd_dataout
);

	// spi2 module control
	wire [7:0] sd_datain;
	wire [7:0] sd_dataout;
	wire       sd_start;


	// single dataout to all ifaces
	assign zx_sd_dataout  = sd_dataout;
	assign avr_sd_dataout = sd_dataout;


	// spi2 module itself
	spi2 spi2(
		.clock(fclk),

		.sck(sdclk),
		.sdo(sddo ),
		.sdi(sddi ),

		.start(sd_start  ),
		.din  (sd_datain ),
		.dout (sd_dataout),

		.speed(2'b00)
	);


	// control locking/arbitrating between ifaces
	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
		avr_lock_out <= 1'b0;
	else // posedge fclk
	begin
		if( sdcs_n )
			avr_lock_out <= avr_lock_in;
	end



	// control cs_n to SDcard
	always @(posedge fclk, negedge rst_n)
	if( !rst_n )
		sdcs_n <= 1'b1;
	else // posedge fclk
	begin
		if( avr_lock_out )
			sdcs_n <= avr_sdcs_n;
		else // !avr_lock_out
			if( zx_sdcs_n_stb )
				sdcs_n <= zx_sdcs_n_val;
	end


	// control start and outgoing data to spi2
	assign sd_start = avr_lock_out ? avr_sd_start : zx_sd_start;
	//
	assign sd_datain = avr_lock_out ? avr_sd_datain : zx_sd_datain;


endmodule

