// This module generates video for DAC


module video_out (

// clocks
	input wire clk,
	
// video controls
    input wire vga_on,
    input wire blank,
    input wire vga_blank,
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


// TV/VGA mux
    wire [7:0] vdata = vga_on ? vgadata : tvdata;

	
// preparing PWM'ed output levels
    wire [11:0] vpixel;
    wire [11:0] vpix = vga_on ? (vga_blank ? vpixel : 0) : (blank ? vpixel : 0);
	
	wire [3:0] red = vpix[11:8];
	wire [3:0] grn = vpix[ 7:4];
	wire [3:0] blu = vpix[ 3:0];
		
	wire [7:0] pwm[0:3];
	assign pwm[0] = 8'b00000000;
	assign pwm[1] = 8'b01000001;
	assign pwm[2] = 8'b10100101;
	assign pwm[3] = 8'b11010111;

	wire [2:0] phase = vga_on ? {vga_line, ph} : {1'b0, ph};
	
	wire [1:0] pred = pwm[red[1:0]][phase] ? (red[3:2] == 2'b11) ? red[3:2] : red[3:2] + 2'b1 : red[3:2];
	wire [1:0] pgrn = pwm[grn[1:0]][phase] ? (grn[3:2] == 2'b11) ? grn[3:2] : grn[3:2] + 2'b1 : grn[3:2];
	wire [1:0] pblu = pwm[blu[1:0]][phase] ? (blu[3:2] == 2'b11) ? blu[3:2] : blu[3:2] + 2'b1 : blu[3:2];
	

//!!!
	// wire [ 1:0]	zx_col		= vdata[3] ? 2'b11 : 2'b10;
	// wire [ 1:0]	zx_pixr		= vdata[1] ? zx_col : 2'b00;
	// wire [ 1:0]	zx_pixg		= vdata[2] ? zx_col : 2'b00;
	// wire [ 1:0]	zx_pixb		= vdata[0] ? zx_col : 2'b00;
	// wire [ 7:0]	vpixel  	= {zx_pixr, zx_pixg, zx_pixb, 2'b0};


// CRAM
	video_cram video_cram(
		.clock		(clk		),
		.wraddress	(0         	),
		.data		(0      	),
		.wren		(0			),
	    .rdaddress	(vdata      ),
	    .q			(vpixel    	)
	);

    
endmodule
