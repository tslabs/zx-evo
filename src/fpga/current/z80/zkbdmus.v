// (c) 2010 NedoPC
//
// MUXes mouse and kbd data in two single databusses for zports

module zkbdmus(

	input  wire        fclk,
	input  wire        rst_n,


	input  wire [39:0] kbd_in,  // key bits
	input  wire        kbd_stb, // and strobe

	input  wire [ 7:0] mus_in,
	input  wire        mus_xstb,
	input  wire        mus_ystb,
	input  wire        mus_btnstb,
	input  wire        kj_stb,


	input  wire [7:0] zah,

	output wire [ 4:0] kbd_data,
	output wire [ 7:0] mus_data,
	output reg  [ 4:0] kj_data
);

	reg [39:0] kbd;
	reg [ 7:0] musx,musy,musbtn;

	wire [4:0] keys [0:7]; // key matrix

	reg [4:0] kout; // wire AND



	// store data from slavespi
	//
    always @(posedge fclk)
    begin
		if( kbd_stb )
			kbd <= kbd_in;

		if( mus_xstb )
			musx <= mus_in;

		if( mus_ystb )
			musy <= mus_in;

		if( mus_btnstb )
			musbtn <= mus_in;

		if( kj_stb )
			kj_data <= mus_in[4:0];
    end


	// make keys
	//
	assign keys[0][4:0] = { kbd[00],kbd[08],kbd[16],kbd[24],kbd[32] };
	assign keys[1][4:0] = { kbd[01],kbd[09],kbd[17],kbd[25],kbd[33] };
	assign keys[2][4:0] = { kbd[02],kbd[10],kbd[18],kbd[26],kbd[34] };
	assign keys[3][4:0] = { kbd[03],kbd[11],kbd[19],kbd[27],kbd[35] };
	assign keys[4][4:0] = { kbd[04],kbd[12],kbd[20],kbd[28],kbd[36] };
	assign keys[5][4:0] = { kbd[05],kbd[13],kbd[21],kbd[29],kbd[37] };
	assign keys[6][4:0] = { kbd[06],kbd[14],kbd[22],kbd[30],kbd[38] };
	assign keys[7][4:0] = { kbd[07],kbd[15],kbd[23],kbd[31],kbd[39] };
	//
	always @*
	begin
		kout = 5'b11111;

		kout = kout & ({5{zah[0]}} | (~keys[0]));
		kout = kout & ({5{zah[1]}} | (~keys[1]));
		kout = kout & ({5{zah[2]}} | (~keys[2]));
		kout = kout & ({5{zah[3]}} | (~keys[3]));
		kout = kout & ({5{zah[4]}} | (~keys[4]));
		kout = kout & ({5{zah[5]}} | (~keys[5]));
		kout = kout & ({5{zah[6]}} | (~keys[6]));
		kout = kout & ({5{zah[7]}} | (~keys[7]));
	end
	//
	assign kbd_data = kout;

	// make mouse
	// FADF - buttons, FBDF - x, FFDF - y
	//
	assign mus_data = zah[0] ? ( zah[2] ? musy : musx ) : musbtn;



endmodule

