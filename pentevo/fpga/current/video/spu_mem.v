// SFILE: Sprites description file
// 512x8

module sfile (
	input	[7:0]  data,
	input	[8:0]  rdaddress,
	input	[8:0]  wraddress,
	input	  wrclock,
	input	  wren,
	output	[7:0]  q
);

	altdpram	altdpram_component (
				.wren (wren),
				.inclock (wrclock),
				.data (data),
				.rdaddress (rdaddress),
				.wraddress (wraddress),
				.q (q),
				.aclr (1'b0),
				.byteena (1'b1),
				.inclocken (1'b1),
				.outclock (1'b1),
				.outclocken (1'b1),
				.rdaddressstall (1'b0),
				.rden (1'b1),
				.wraddressstall (1'b0));
	defparam
		altdpram_component.indata_aclr = "OFF",
		altdpram_component.indata_reg = "INCLOCK",
		altdpram_component.intended_device_family = "ACEX1K",
		altdpram_component.lpm_type = "altdpram",
		altdpram_component.outdata_aclr = "OFF",
		altdpram_component.outdata_reg = "UNREGISTERED",
		altdpram_component.rdaddress_aclr = "OFF",
		altdpram_component.rdaddress_reg = "UNREGISTERED",
		altdpram_component.rdcontrol_aclr = "OFF",
		altdpram_component.rdcontrol_reg = "UNREGISTERED",
		altdpram_component.width = 8,
		altdpram_component.widthad = 9,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "INCLOCK",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule


//palette SRAM for sprites 256x8
module spram (
	input	[7:0]  data,
	input	[8:0]  rdaddress,
	input	[8:0]  wraddress,
	input	  wrclock,
	input	  wren,
	output	[7:0]  q
);

	altdpram	altdpram_component (
				.wren (wren),
				.inclock (wrclock),
				.data (data),
				.rdaddress (rdaddress),
				.wraddress (wraddress),
				.q (q),
				.aclr (1'b0),
				.byteena (1'b1),
				.inclocken (1'b1),
				.outclock (1'b1),
				.outclocken (1'b1),
				.rdaddressstall (1'b0),
				.rden (1'b1),
				.wraddressstall (1'b0));
	defparam
		altdpram_component.indata_aclr = "OFF",
		altdpram_component.indata_reg = "INCLOCK",
		altdpram_component.intended_device_family = "ACEX1K",
		altdpram_component.lpm_type = "altdpram",
		altdpram_component.outdata_aclr = "OFF",
		altdpram_component.outdata_reg = "UNREGISTERED",
		altdpram_component.rdaddress_aclr = "OFF",
		altdpram_component.rdaddress_reg = "UNREGISTERED",
		altdpram_component.rdcontrol_aclr = "OFF",
		altdpram_component.rdcontrol_reg = "UNREGISTERED",
		altdpram_component.width = 8,
		altdpram_component.widthad = 9,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "INCLOCK",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule


//sprite video buffer0
module sline0 (
	input	[6:0]  data,
	input	[8:0]  rdaddress,
	input	[8:0]  wraddress,
	input	  wrclock,
	input	  wren,
	output	[6:0]  q
);

	altdpram	altdpram_component (
				.wren (wren),
				.inclock (wrclock),
				.data (data),
				.rdaddress (rdaddress),
				.wraddress (wraddress),
				.q (q),
				.aclr (1'b0),
				.byteena (1'b1),
				.inclocken (1'b1),
				.outclock (1'b1),
				.outclocken (1'b1),
				.rdaddressstall (1'b0),
				.rden (1'b1),
				.wraddressstall (1'b0));
	defparam
		altdpram_component.indata_aclr = "OFF",
		altdpram_component.indata_reg = "INCLOCK",
		altdpram_component.intended_device_family = "ACEX1K",
		altdpram_component.lpm_type = "altdpram",
		altdpram_component.outdata_aclr = "OFF",
		altdpram_component.outdata_reg = "UNREGISTERED",
		altdpram_component.rdaddress_aclr = "OFF",
		altdpram_component.rdaddress_reg = "UNREGISTERED",
		altdpram_component.rdcontrol_aclr = "OFF",
		altdpram_component.rdcontrol_reg = "UNREGISTERED",
		altdpram_component.width = 7,
		altdpram_component.widthad = 9,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "INCLOCK",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
 endmodule


//sprite video buffer1
module sline1 (
	input	[6:0]  data,
	input	[8:0]  rdaddress,
	input	[8:0]  wraddress,
	input	  wrclock,
	input	  wren,
	output	[6:0]  q
);

	altdpram	altdpram_component (
				.wren (wren),
				.inclock (wrclock),
				.data (data),
				.rdaddress (rdaddress),
				.wraddress (wraddress),
				.q (q),
				.aclr (1'b0),
				.byteena (1'b1),
				.inclocken (1'b1),
				.outclock (1'b1),
				.outclocken (1'b1),
				.rdaddressstall (1'b0),
				.rden (1'b1),
				.wraddressstall (1'b0));
	defparam
		altdpram_component.indata_aclr = "OFF",
		altdpram_component.indata_reg = "INCLOCK",
		altdpram_component.intended_device_family = "ACEX1K",
		altdpram_component.lpm_type = "altdpram",
		altdpram_component.outdata_aclr = "OFF",
		altdpram_component.outdata_reg = "UNREGISTERED",
		altdpram_component.rdaddress_aclr = "OFF",
		altdpram_component.rdaddress_reg = "UNREGISTERED",
		altdpram_component.rdcontrol_aclr = "OFF",
		altdpram_component.rdcontrol_reg = "UNREGISTERED",
		altdpram_component.width = 7,
		altdpram_component.widthad = 9,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "INCLOCK",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule
