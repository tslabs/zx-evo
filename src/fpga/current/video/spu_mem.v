// SMEM: Sprites memory file
// 16384 bit = 512x16
// #debug!!!

module smem (
	input	[15:0]  data,
	input	[8:0]  rdaddress,
	input	[8:0]  wraddress,
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
		altdpram_component.widthad = 9,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "INCLOCK",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule


// SFILE: Sprites description file
// 2048 bit = 256x8

module sfile (
	input	[7:0]  data,
	input	[7:0]  rdaddress,
	input	[7:0]  wraddress,
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
		altdpram_component.widthad = 8,
		altdpram_component.wraddress_aclr = "OFF",
		altdpram_component.wraddress_reg = "INCLOCK",
		altdpram_component.wrcontrol_aclr = "OFF",
		altdpram_component.wrcontrol_reg = "INCLOCK";
endmodule


//palette SRAM for sprites 256x8
module spram (
	input	[7:0]  data,
	input	[7:0]  rdaddress,
	input	[7:0]  wraddress,
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
		altdpram_component.widthad = 8,
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


//counters for sprite address
//32 x 14 bit
//so, sprite can occupy max 128 words x 128 lines = 16kwords in DRAM
module sacnt (
        data,
        wraddress,
        wren,
        rdaddress,
        q);

        input   [13:0]  data;
        input   [4:0]  rdaddress;
        input   [4:0]  wraddress;
        input     wren;
        output  [13:0]  q;

        wire [13:0] sub_wire0;
        wire [13:0] q = sub_wire0[13:0];

        lpm_ram_dp      lpm_ram_dp_component (
                                .wren (wren),
                                .data (data),
                                .rdaddress (rdaddress),
                                .wraddress (wraddress),
                                .q (sub_wire0),
                                .rdclken (1'b1),
                                .rdclock (1'b1),
                                .rden (1'b1),
                                .wrclken (1'b1),
                                .wrclock (1'b1));
        defparam
                lpm_ram_dp_component.intended_device_family = "ACEX1K",
                lpm_ram_dp_component.lpm_indata = "UNREGISTERED",
                lpm_ram_dp_component.lpm_outdata = "UNREGISTERED",
                lpm_ram_dp_component.lpm_rdaddress_control = "UNREGISTERED",
                lpm_ram_dp_component.lpm_type = "LPM_RAM_DP",
                lpm_ram_dp_component.lpm_width = 14,
                lpm_ram_dp_component.lpm_widthad = 5,
                lpm_ram_dp_component.lpm_numwords = 32,
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";
endmodule
