// This module generates video for DAC


module video_out (

// clocks
	input wire clk,
	
// video controls
    input wire vga_on,
    input wire tv_blank,
    input wire vga_blank,
    input wire vga_line,

// video data
	input  wire [7:0] tvdata,
	input  wire [7:0] vgadata,
	input  wire [7:0] romconf,	//!!!
	input  wire [7:0] vpage,	//!!!
	input  wire [7:0] page00,	//!!!
	output reg  [1:0] vred,
    output reg  [1:0] vgrn,
    output reg  [1:0] vblu
);


// registering output colors
	
	always @(negedge clk)
	begin
		vred <= pred;
		vgrn <= pgrn;
		vblu <= pblu;
	end

	
// phase clocking
	reg [2:0] ph;
	
	always @(posedge clk)
		ph <= ph +1;


// TV/VGA mux
    wire [7:0] vdata = vga_on ? vgadata : tvdata;

	
// preparing PWM'ed output levels
    wire [14:0] vpixel;
    wire blank = vga_on ? vga_blank : tv_blank;
	wire [14:0] vpix = blank ? 0: vpixel;
	
	wire [1:0] cred = vpix[14:13];
	wire [2:0] ired = vpix[12:10];
	wire [1:0] cgrn = vpix[ 9: 8];
	wire [2:0] igrn = vpix[ 7: 5];
	wire [1:0] cblu = vpix[ 4: 3];
	wire [2:0] iblu = vpix[ 2: 0];
	
		
	wire [7:0] pwm[0:7];
	assign pwm[0] = 8'b00000000;
	assign pwm[1] = 8'b00000001;
	assign pwm[2] = 8'b01000001;
	assign pwm[3] = 8'b01000101;
	assign pwm[4] = 8'b10100101;
	assign pwm[5] = 8'b10100111;
	assign pwm[6] = 8'b11010111;
	assign pwm[7] = 8'b11011111;

	wire [2:0] phase = vga_on ? {vga_line, ph[1:0]} : {ph[2:0]};

	wire [1:0] pred = pwm[ired][phase] ? (cred == 2'b11) ? cred : cred + 2'b1 : cred;
	wire [1:0] pgrn = pwm[igrn][phase] ? (cgrn == 2'b11) ? cgrn : cgrn + 2'b1 : cgrn;
	wire [1:0] pblu = pwm[iblu][phase] ? (cblu == 2'b11) ? cblu : cblu + 2'b1 : cblu;
	

// CRAM
	video_cram video_cram(
		.clock		(clk		),
		.wraddress	(romconf   	),	//!!!
		.data		({vpage, page00}    ),	//!!!
		.wren		(1			),	//!!!
	    .rdaddress	(vdata      ),
	    .q			(vpixel    	)
	);

    
endmodule
