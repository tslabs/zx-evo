// (c) 2010 NedoPC
//
// fpga SPI slave device.


`include "../include/tune.v"

module slavespi(

	input  wire fclk,
	input  wire rst_n,

	input  wire spics_n, // avr SPI iface
	output wire spidi,   //
	input  wire spido,   //
	input  wire spick,   //

	input  wire [ 7:0] status_in, // status bits to be shown to avr


	output wire [39:0] kbd_out,
	output wire        kbd_stb,

	output wire [ 7:0] mus_out,
	output wire        mus_xstb,
	output wire        mus_ystb,
	output wire        mus_btnstb,
	output wire        kj_stb,


	input  wire [ 7:0] gluclock_addr,

	input  wire [ 7:0] wait_write,
	output wire [ 7:0] wait_read,
	input  wire        wait_rnw,

	output wire        wait_end,

	output wire [ 7:0] config0, // config bits for overall system

	output wire       genrst, // positive pulse, causes Z80 reset
	output wire [1:0] rstrom  // number of ROM page to reset to
);

	// re-synchronize SPI
	//
	reg [2:0] spics_n_sync;
	reg [1:0] spido_sync;
	reg [2:0] spick_sync;
	//
	always @(posedge fclk)
	begin
		spics_n_sync[2:0] <= { spics_n_sync[1:0], spics_n };
		spido_sync  [1:0] <= { spido_sync    [0], spido   };
		spick_sync  [2:0] <= { spick_sync  [1:0], spick   };
	end
	//
	wire scs_n = spics_n_sync[1]; // scs_n - synchronized CS_N
	wire sdo   = spido_sync[1];
	wire sck   = spick_sync[1];
	//
	wire scs_n_01 = (~spics_n_sync[2]) &   spics_n_sync[1] ;
	wire scs_n_10 =   spics_n_sync[2]  & (~spics_n_sync[1]);
	//
	wire sck_01 = (~spick_sync[2]) &   spick_sync[1] ;
	wire sck_10 =   spick_sync[2]  & (~spick_sync[1]);


	reg [7:0] regnum; // register number holder

	reg [7:0] shift_out;

	reg [7:0] data_in;



	// register selectors
	wire sel_kbdreg, sel_kbdstb, sel_musxcr, sel_musycr, sel_musbtn, sel_kj;
	wire sel_rstreg, sel_waitreg, sel_gluadr, sel_cfg0;

	// keyboard register
	reg [39:0] kbd_reg;

	// mouse register
	reg [7:0] mouse_buf;

	// reset register
	reg [7:0] rst_reg;

	// wait data out register
	reg [7:0] wait_reg;

	//
	reg [7:0] cfg0_reg_in, cfg0_reg_out; // one for shifting, second for storing values



`ifdef SIMULATE
	initial
	begin
		rstrom = 2'b00;
		genrst = 1'b0;
	end
`endif


	// register number
	//
	always @(posedge fclk)
	begin
		if( scs_n_01 )
		begin
			regnum <= 8'd0;
		end
		else if( scs_n && sck_01 )
		begin
			regnum[7:0] <= { sdo, regnum[7:1] };
		end
	end


	// send data to avr
	//
	always @*
	begin
		if( sel_waitreg )
			data_in = wait_write; // TODO: add here selection of different input registers
		else if( sel_gluadr )
			data_in = gluclock_addr;
		else data_in = 8'hFF;
	end
	//
	always @(posedge fclk)
	begin
		if( scs_n_01 || scs_n_10 ) // both edges
		begin
			shift_out <= scs_n ? status_in : data_in;
		end
		else if( sck_01 )
		begin
			shift_out[7:0] <= { 1'b0, shift_out[7:1] };
		end
	end
	//
	assign spidi = shift_out[0];


	// reg number decoding
	//
	assign sel_kbdreg = ( (regnum[7:4]==4'h1) && !regnum[0] ); // $10
	assign sel_kbdstb = ( (regnum[7:4]==4'h1) &&  regnum[0] ); // $11
	//
	assign sel_musxcr = ( (regnum[7:4]==4'h2) && !regnum[1] && !regnum[0] ); // $20
	assign sel_musycr = ( (regnum[7:4]==4'h2) && !regnum[1] &&  regnum[0] ); // $21
	assign sel_musbtn = ( (regnum[7:4]==4'h2) &&  regnum[1] && !regnum[0] ); // $22
	assign sel_kj     = ( (regnum[7:4]==4'h2) &&  regnum[1] &&  regnum[0] ); // $23
	//
	assign sel_rstreg = ( (regnum[7:4]==4'h3) ) ; // $30
	//
	assign sel_waitreg = ( (regnum[7:4]==4'h4) && !regnum[0] ); // $40
	assign sel_gluadr  = ( (regnum[7:4]==4'h4) &&  regnum[0] ); // $41
	//
	assign sel_cfg0 = (regnum[7:4]==4'h5); // $50


	// registers data-in
	//
	always @(posedge fclk)
	begin
		if( !scs_n && sel_kbdreg && sck_01 )
			kbd_reg[39:0] <= { sdo, kbd_reg[39:1] };

		if( !scs_n && (sel_musxcr || sel_musycr || sel_musbtn || sel_kj) && sck_01 )
            mouse_buf[7:0] <= { sdo, mouse_buf[7:1] };

		if( !scs_n && sel_rstreg && sck_01 )
			rst_reg[7:0] <= { sdo, rst_reg[7:1] };

		if( !scs_n && sel_waitreg && sck_01 )
			wait_reg[7:0] <= { sdo, wait_reg[7:1] };

		if( !scs_n && sel_cfg0 && sck_01 )
			cfg0_reg_in[7:0] <= { sdo, cfg0_reg_in[7:1] };

		if( scs_n_01 && sel_cfg0 )
			cfg0_reg_out <= cfg0_reg_in;
	end


	// output data
	assign kbd_out = kbd_reg;
	assign kbd_stb = sel_kbdstb && scs_n_01;

	assign mus_out    = mouse_buf;
	assign mus_xstb   = sel_musxcr && scs_n_01;
	assign mus_ystb   = sel_musycr && scs_n_01;
	assign mus_btnstb = sel_musbtn && scs_n_01;
	assign kj_stb     = sel_kj     && scs_n_01;

	assign genrst = sel_rstreg && scs_n_01;
	assign rstrom = rst_reg[5:4];

	assign wait_read = wait_reg;
	assign wait_end = sel_waitreg && scs_n_01;

	assign config0 = cfg0_reg_out;

endmodule

