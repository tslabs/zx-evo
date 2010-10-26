// synopsys translate_off
`timescale 1 ps / 1 ps
// synopsys translate_on

module pram (
        data,
        wraddress,
        wren,
        rdaddress,
        q);

        input   [5:0]  data;
        input   [7:0]  rdaddress;
        input   [7:0]  wraddress;
        input     wren;
        output  [5:0]  q;

        wire [7:0] sub_wire0;
        wire [7:0] q = sub_wire0[7:0];

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
                lpm_ram_dp_component.lpm_width = 6,
                lpm_ram_dp_component.lpm_widthad = 8,
                lpm_ram_dp_component.lpm_numwords = 256,
                lpm_ram_dp_component.lpm_file = "spram.mif",
                lpm_ram_dp_component.lpm_wraddress_control = "UNREGISTERED",
                lpm_ram_dp_component.rden_used = "FALSE",
                lpm_ram_dp_component.use_eab = "ON";


endmodule