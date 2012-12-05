
`include "../include/tune.v"


module zdos(

	input  wire        clk,
	input  wire        rst,
	input  wire        opfetch,
	input  wire        opfetch_s,

	input  wire        dos_on,		// 1 clock strobe (driven by opfetch_s)
	input  wire        dos_off,			// from zpager.v
	input  wire        vdos_on,		// 1 clock strobe (driven by iorq_s)
	input  wire        vdos_off,		// from zports.v

	output wire        dos,
	output wire        vdos

);


// turn on and off dos
	assign dos = (dos_on || dos_off) ^^ dos_r;		// to make dos appear 1 clock earlier than dos_r

    reg dos_r;
	always @(posedge clk)
	if (rst)
		dos_r <= 1'b0;

	else if (dos_off)
			dos_r <= 1'b0;
	else if (dos_on)
			dos_r <= 1'b1;


// turn on and off virtdos
    // vdos turn on/off is delayed till next opfetch due to INIR that writes right after iord cycle
	assign vdos = opfetch ? pre_vdos : vdos_r;	// vdos appears as soon as first opfetch

    reg pre_vdos, vdos_r;
	always @(posedge clk)
	if (rst || vdos_off)
	begin
		pre_vdos <= 1'b0;
		vdos_r <= 1'b0;
	end
	else if (vdos_on)
		pre_vdos <= 1'b1;
	else if (opfetch_s)
		vdos_r <= pre_vdos;


endmodule
