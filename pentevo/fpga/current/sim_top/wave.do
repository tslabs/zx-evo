onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /tb/fclk
add wave -noupdate /tb/clkz_out
add wave -noupdate /tb/zrst_n
add wave -noupdate /tb/clkz_in
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/zclock/clk14_src
add wave -noupdate /tb/DUT/zclock/zclk_stall
add wave -noupdate /tb/DUT/zclock/pre_zpos_35
add wave -noupdate /tb/DUT/zclock/pre_zneg_35
add wave -noupdate /tb/DUT/zclock/pre_zpos_70
add wave -noupdate /tb/DUT/zclock/pre_zneg_70
add wave -noupdate /tb/DUT/zclock/pre_zpos_140
add wave -noupdate /tb/DUT/zclock/pre_zneg_140
add wave -noupdate /tb/DUT/zclock/pre_zpos
add wave -noupdate /tb/DUT/zclock/pre_zneg
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/zclock/zpos
add wave -noupdate /tb/DUT/zclock/zneg
add wave -noupdate /tb/z80/BUSRQ_n
add wave -noupdate /tb/z80/BUSAK_n
add wave -noupdate /tb/DUT/z80mem/r_mreq_n
add wave -noupdate /tb/clkz_in
add wave -noupdate /tb/DUT/external_port
add wave -noupdate /tb/iorq_n
add wave -noupdate /tb/mreq_n
add wave -noupdate /tb/rd_n
add wave -noupdate /tb/wr_n
add wave -noupdate /tb/m1_n
add wave -noupdate /tb/rfsh_n
add wave -noupdate /tb/int_n
add wave -noupdate /tb/nmi_n
add wave -noupdate /tb/wait_n
add wave -noupdate -radix hexadecimal /tb/za
add wave -noupdate -radix hexadecimal /tb/zd
add wave -noupdate -radix hexadecimal /tb/zd_dut_to_z80
add wave -noupdate /tb/csrom
add wave -noupdate /tb/romoe_n
add wave -noupdate /tb/romwe_n
add wave -noupdate -divider <NULL>
add wave -noupdate -radix hexadecimal /tb/z80/u0/IR
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/z80mem/memrd
add wave -noupdate /tb/DUT/z80mem/memwr
add wave -noupdate /tb/DUT/z80mem/opfetch
add wave -noupdate /tb/DUT/z80mem/dram_beg
add wave -noupdate /tb/DUT/z80mem/stall14_ini
add wave -noupdate /tb/DUT/z80mem/stall14_cyc
add wave -noupdate /tb/DUT/z80mem/stall14_fin
add wave -noupdate /tb/DUT/z80mem/stall14_cycrd
add wave -noupdate /tb/DUT/z80mem/cpu_next
add wave -noupdate /tb/DUT/z80mem/cpu_stall
add wave -noupdate /tb/DUT/z80mem/cpu_req
add wave -noupdate /tb/DUT/z80mem/pending_cpu_req
add wave -noupdate /tb/DUT/z80mem/cpu_strobe
add wave -noupdate /tb/DUT/z80mem/cpu_rnw
add wave -noupdate /tb/DUT/z80mem/cpu_rnw_r
add wave -noupdate /tb/DUT/z80mem/cend
add wave -noupdate -divider <NULL>
add wave -noupdate -radix hexadecimal /tb/z80/u0/IR
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/dram/rras0_n
add wave -noupdate /tb/DUT/dram/rras1_n
add wave -noupdate /tb/DUT/dram/rucas_n
add wave -noupdate /tb/DUT/dram/rlcas_n
add wave -noupdate /tb/DUT/dram/cbeg
add wave -noupdate -radix hexadecimal /tb/DUT/dram/int_addr
add wave -noupdate -radix hexadecimal /tb/DUT/dram/int_wrdata
add wave -noupdate /tb/DUT/dram/int_bsel
add wave -noupdate -divider <NULL>
add wave -noupdate -radix hexadecimal {/tb/DUT/instantiate_atm_pagers[0]/atm_pager/page}
add wave -noupdate {/tb/DUT/instantiate_atm_pagers[0]/atm_pager/romnram}
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/z80/RESET_n
add wave -noupdate /tb/z80/CLK_n
add wave -noupdate /tb/z80/RFSH_n
add wave -noupdate /tb/z80/M1_n
add wave -noupdate /tb/z80/MREQ_n
add wave -noupdate /tb/z80/RD_n
add wave -noupdate /tb/z80/WR_n
add wave -noupdate -radix hexadecimal /tb/z80/A
add wave -noupdate -radix hexadecimal /tb/z80/D
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {447762029900 ps} 0} {{Cursor 2} {166348107189 ps} 0}
configure wave -namecolwidth 353
configure wave -valuecolwidth 172
configure wave -justifyvalue left
configure wave -signalnamewidth 0
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 17800
configure wave -gridperiod 35600
configure wave -griddelta 8
configure wave -timeline 0
configure wave -timelineunits ns
update
WaveRestoreZoom {447760689667 ps} {447763370133 ps}
