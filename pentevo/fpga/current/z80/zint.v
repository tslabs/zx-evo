
`include "../include/tune.v"

module zint
(
	input  wire clk,
	input  wire res,
	input  wire int_start_frm,
	input  wire int_start_dma,
	input  wire vdos,
	input  wire intack,
	
	input wire [2:0] im2v_frm,
	input wire [2:0] im2v_dma,
	input wire [7:0] intmask,
	output wire [7:0] im2vect,
	
	output reg  int_n
);

	// In VDOS INTs are focibly disabled.
	// For Frame INT its generation is blocked, it will be lost.
	// For DMA INT only its output is blocked, so DMA ISR will will be processed as soon as returned from VDOS.
	
	assign int_n = int_all ? 1'b0 : 1'bZ;
	wire int_all = int_frm || (int_dma && !vdos);
	
	wire dis_int_frm = !intmask[0];
	wire dis_int_dma = !intmask[1];

// IM2 Vector priority
	assign im2vect = {int_sel ? vec_frm : vec_dma, 1'b1};
	wire [6:0] vec_frm = {4'b1111, im2v_frm};
	wire [6:0] vec_dma = {4'b1110, im2v_dma};

// INT source latch
	reg intack_r;
	always @(posedge clk)
		intack_r <= intack;
	
	wire intack_s = intack && !intack_r;
	
	reg int_sel;
	always @(posedge clk)
		if (intack_s)
			int_sel <= int_frm;

// INT generating
	reg int_frm;
	always @(posedge clk)
		if (res || dis_int_frm)
			int_frm <= 1'b0;
		else if (int_start_frm && !vdos)
			int_frm <= 1'b1;
		else if (intack_s)
			int_frm <= 1'b0;

	reg int_dma;
	always @(posedge clk)
		if (res || dis_int_dma)
			int_dma <= 1'b0;
		else if (int_start_dma)
			int_dma <= 1'b1;
		else if (intack_s && !int_frm)
			int_dma <= 1'b0;

endmodule
