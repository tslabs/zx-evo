`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// manages ZX-bus IORQ-IORQGE stuff and free bus content
//
module zbus(

	input iorq,
	input rd,
	input m1,

	output iorq1_n,
	output iorq2_n,

	input iorqge1,
	input iorqge2,

	input porthit,

	output drive_ff
);


	assign iorq1_n = !iorq | porthit;
	assign iorq2_n = iorq1_n | iorqge1;
	assign drive_ff = ~(iorq2_n | iorqge2) & rd;


endmodule
