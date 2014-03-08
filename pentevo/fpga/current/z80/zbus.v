`include "../include/tune.v"

// PentEvo project (c) NedoPC 2008-2009
//
// manages ZX-bus IORQ-IORQGE stuff and free bus content
//
module zbus(

	input iorq,
	input iorq_n,
	input rd,

	output iorq1_n,
	output iorq2_n,

	input iorqge1,
	input iorqge2,

	input porthit,

	output drive_ff
);


	// assign iorq1_n = !iorq | porthit;	// iorq is masked my M1_n!
	assign iorq1_n = iorq_n;
	assign iorq2_n = iorq1_n || iorqge1;
	
	assign drive_ff = !iorq2_n && !iorqge2 && !porthit && rd;


endmodule
