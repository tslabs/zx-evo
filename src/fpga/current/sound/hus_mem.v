// HFILE: DACs description file
// 512x8 (32x16x8)

module hfile (
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


// HVOL: DACs description file
// 64x8 (32x2x8)

module hvol (
	input	[7:0]  data,
	input	[5:0]  rdaddress,
	input	[5:0]  wraddress,
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
		altdpram_component.widthad = 6,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "INCLOCK",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule


// HCNT: DACs description file
// 128x16 (32x4x16)

module hcnt (
	input	[15:0]  data,
	input	[6:0]  rdaddress,
	input	[6:0]  wraddress,
	input	  wrclock,
	input	  wren,
	output	[15:0]  q
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
		altdpram_component.width = 16,
		altdpram_component.widthad = 7,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "INCLOCK",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule
