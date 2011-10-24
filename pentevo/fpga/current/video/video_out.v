// This module generates video for DAC


module video_out (

// clocks
	input wire clk,
	
// video controls
    input wire vga_on,
    input wire vga_line,

// video data
	input  wire [7:0] tvdata,
	input  wire [7:0] vgadata,
	output reg  [1:0] vred,
    output reg  [1:0] vgrn,
    output reg  [1:0] vblu
);


// registering output colors
	
	always @(posedge clk)
	begin
		vred <= pred;
		vgrn <= pgrn;
		vblu <= pblu;
	end

	
// phase clocking
	reg [1:0] ph;
	
	always @(posedge clk)
		ph <= ph +1;
	
	
// preparing PWM'ed output levels
	
	wire [1:0] red = vga_on ? vgadata[7:6] : tvdata[7:6];
	wire [1:0] grn = vga_on ? vgadata[5:4] : tvdata[5:4];
	wire [1:0] blu = vga_on ? vgadata[3:2] : tvdata[3:2];
	wire [1:0] iii = vga_on ? vgadata[1:0] : tvdata[1:0];
		
	wire [1:0] r0 = red;
	wire [1:0] g0 = grn;
    wire [1:0] b0 = blu;
	wire [1:0] r1 = (red == 2'b11) ? red : red + 2'b1;
    wire [1:0] g1 = (grn == 2'b11) ? grn : grn + 2'b1;
	wire [1:0] b1 = (blu == 2'b11) ? blu : blu + 2'b1;

	wire [7:0] pwm[0:3];
	assign pwm[0] = 8'b00000000;
	assign pwm[1] = 8'b01000001;
	assign pwm[2] = 8'b10100101;
	assign pwm[3] = 8'b11010111;

	wire [2:0] phase = vga_on ? {vga_line, ph} : {1'b0, ph};
	
	wire [1:0] pred = pwm[iii][phase] ? r1 : r0;
	wire [1:0] pgrn = pwm[iii][phase] ? g1 : g0;
	wire [1:0] pblu = pwm[iii][phase] ? b1 : b0;
	

	
endmodule
