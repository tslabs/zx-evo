// This module generates video for DAC

`include "../include/tune.v"


module video_out (

// clocks
	input wire clk, zclk, f0, c3,
	
// video controls
    input wire vga_on,
    input wire tv_blank,
    input wire vga_blank,
    input wire vga_line,
	input wire [1:0] plex_sel_in,

// mode controls	
	input wire tv_hires,
	input wire vga_hires,
	input wire [3:0] palsel,

// Z80 pins
	input  wire [14:0] cram_data_in,
	input  wire [7:0] cram_addr_in,
	input  wire cram_we,
	
// video data
	input wire [7:0] vplex_in,
	input wire [7:0] vgaplex,
	output reg [1:0] vred,
    output reg [1:0] vgrn,
    output reg [1:0] vblu
);


// TV/VGA mux
	reg [7:0] vplex;
	always @(posedge clk) if (c3)
		vplex <= vplex_in;

	wire [7:0] plex = vga_on ? vgaplex : vplex;
	wire plex_sel = vga_on ? plex_sel_in[0] : plex_sel_in[1];
	wire [7:0] vdata = hires ? {palsel, plex_sel ? plex[3:0] : plex[7:4]} : plex;
    wire blank = vga_on ? vga_blank : tv_blank;
    wire hires = vga_on ? vga_hires : tv_hires;

	wire [14:0] vpix = blank1 ? 15'b0 : vpixel;
	// wire [14:0] vpix = blank ? 0 : {vdata[2], 4'b0, vdata[1], 4'b0, vdata[0], 4'b0};		//debug!!!
	
    reg blank1;         // GOVNOKOD!!!!!!!!!!!!!!!!!!!!!
    always @(posedge clk)
    begin
        blank1 <= blank;
    end
	
// color components extraction
	wire [1:0] cred = vpix[14:13];
	wire [2:0] ired = vpix[12:10];
	wire [1:0] cgrn = vpix[ 9: 8];
	wire [2:0] igrn = vpix[ 7: 5];
	wire [1:0] cblu = vpix[ 4: 3];
	wire [2:0] iblu = vpix[ 2: 0];

// prepare and clocking two phases of output
	reg [1:0] red0;
	reg [1:0] grn0;
	reg [1:0] blu0;
	reg [1:0] red1;
	reg [1:0] grn1;
	reg [1:0] blu1;
	
	always @(posedge clk)
	begin
		red0 <= !pwm[ired][{phase, 1'b0}] | &cred ? cred : cred + 2'b1;
		grn0 <= !pwm[igrn][{phase, 1'b0}] | &cgrn ? cgrn : cgrn + 2'b1;
		blu0 <= !pwm[iblu][{phase, 1'b0}] | &cblu ? cblu : cblu + 2'b1;
		red1 <= !pwm[ired][{phase, 1'b1}] | &cred ? cred : cred + 2'b1;
		grn1 <= !pwm[igrn][{phase, 1'b1}] | &cgrn ? cgrn : cgrn + 2'b1;
		blu1 <= !pwm[iblu][{phase, 1'b1}] | &cblu ? cblu : cblu + 2'b1;
	end
	

// output muxing for 56MHz PWM resolution
	assign vred = clk ? red1 : red0;
	assign vgrn = clk ? grn1 : grn0;
	assign vblu = clk ? blu1 : blu0;

	
// PWM phase
	reg [1:0] ph;
	always @(posedge clk)
		ph <= ph + 2'b1;
	
	wire [1:0] phase = {vga_on ? vga_line : ph[1], ph[0]};

	
// PWM	
	wire [7:0] pwm[0:7];
	assign pwm[0] = 8'b00000000;
	assign pwm[1] = 8'b00000001;
	assign pwm[2] = 8'b01000001;
	assign pwm[3] = 8'b01000101;
	assign pwm[4] = 8'b10100101;
	assign pwm[5] = 8'b10100111;
	assign pwm[6] = 8'b11010111;
	assign pwm[7] = 8'b11011111;


// CRAM
    wire [14:0] vpixel;
	
	video_cram video_cram(
		// .wrclock	(~zclk),
		// .wrclock	(clk),	// this should be zclk
		.wraddress	(cram_addr_in),
		.data		(cram_data_in),
		.wren		(cram_we),
		
		.clock	(clk),
		// .rdclock	(clk),
	    .rdaddress	(vdata),
		// .rden		(f0),
	    .q			(vpixel)
);

    
endmodule
