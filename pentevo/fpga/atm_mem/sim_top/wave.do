onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate -format Logic /tb/fclk
add wave -noupdate -format Logic /tb/clkz_out
add wave -noupdate -format Logic /tb/zrst_n
add wave -noupdate -format Logic /tb/clkz_in
add wave -noupdate -divider <NULL>
add wave -noupdate -color Salmon -format Logic /tb/DUT/zclock/zpos
add wave -noupdate -color Salmon -format Logic /tb/DUT/zclock/zneg
add wave -noupdate -format Logic /tb/z80/busrq_n
add wave -noupdate -format Logic /tb/z80/busak_n
add wave -noupdate -format Logic /tb/iorq_n
add wave -noupdate -format Logic /tb/mreq_n
add wave -noupdate -format Logic /tb/rd_n
add wave -noupdate -format Logic /tb/wr_n
add wave -noupdate -format Logic /tb/m1_n
add wave -noupdate -format Logic /tb/rfsh_n
add wave -noupdate -format Logic /tb/int_n
add wave -noupdate -format Logic /tb/nmi_n
add wave -noupdate -format Logic /tb/wait_n
add wave -noupdate -format Literal -radix hexadecimal /tb/za
add wave -noupdate -format Literal -radix hexadecimal /tb/zd
add wave -noupdate -format Logic /tb/csrom
add wave -noupdate -format Logic /tb/romoe_n
add wave -noupdate -format Logic /tb/romwe_n
add wave -noupdate -divider <NULL>
add wave -noupdate -format Literal -radix hexadecimal /tb/z80/u0/dinst
add wave -noupdate -format Literal -radix hexadecimal /tb/z80/u0/ir
add wave -noupdate -divider <NULL>
add wave -noupdate -format Literal -radix hexadecimal /tb/DUT/ra
add wave -noupdate -format Logic /tb/DUT/rwe_n
add wave -noupdate -format Logic /tb/DUT/rucas_n
add wave -noupdate -format Logic /tb/DUT/rlcas_n
add wave -noupdate -format Logic /tb/DUT/rras0_n
add wave -noupdate -format Logic /tb/DUT/rras1_n
add wave -noupdate -format Literal -radix hexadecimal /tb/DUT/rd
add wave -noupdate -color Salmon -format Logic /tb/DUT/zclock/zpos
add wave -noupdate -color Salmon -format Logic /tb/DUT/zclock/zneg
add wave -noupdate -format Logic /tb/DUT/zclock/zclk_out
add wave -noupdate -format Logic /tb/clkz_in
add wave -noupdate -format Logic /tb/DUT/dramarb/cpu_req
add wave -noupdate -format Logic /tb/DUT/dramarb/cpu_rnw
add wave -noupdate -format Logic /tb/DUT/z80mem/ramreq
add wave -noupdate -format Logic /tb/DUT/z80mem/ramwr
add wave -noupdate -format Logic /tb/DUT/z80mem/ramrd
add wave -noupdate -format Logic /tb/DUT/z80mem/ramrd_reg
add wave -noupdate -format Logic /tb/DUT/z80mem/ramwr_reg
add wave -noupdate -divider <NULL>
add wave -noupdate -format Literal -radix hexadecimal /tb/romko/addr
add wave -noupdate -format Literal -radix hexadecimal /tb/romko/data
add wave -noupdate -format Logic {/tb/DUT/instantiate_atm_pagers[0]/atm_pager/dos}
add wave -noupdate -format Literal {/tb/DUT/instantiate_atm_pagers[0]/atm_pager/page}
add wave -noupdate -format Logic {/tb/DUT/instantiate_atm_pagers[0]/atm_pager/pent1m_ROM}
add wave -noupdate -format Literal {/tb/DUT/instantiate_atm_pagers[0]/atm_pager/pent1m_page}
add wave -noupdate -format Literal /tb/DUT/z80mem/win0_page
add wave -noupdate -format Logic /tb/DUT/z80mem/win0_romnram
add wave -noupdate -format Literal /tb/DUT/z80mem/win3_page
add wave -noupdate -format Logic /tb/DUT/z80mem/win3_romnram
add wave -noupdate -format Literal /tb/DUT/z80mem/rompg
add wave -noupdate -format Literal /tb/DUT/adr_fix
add wave -noupdate -format Logic /tb/DUT/rompg0_n
add wave -noupdate -format Logic /tb/DUT/dos_n
add wave -noupdate -format Logic /tb/DUT/zclock/zclk_stall
add wave -noupdate -format Logic /tb/DUT/zdos/cpm_n
add wave -noupdate -format Logic /tb/DUT/zdos/dos_turn_off
add wave -noupdate -format Logic /tb/DUT/zdos/dos_turn_on
add wave -noupdate -format Literal /tb/DUT/dos_turn_off
add wave -noupdate -format Literal /tb/DUT/dos_turn_on
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {3221800 ps} 0} {{Cursor 2} {631252485400 ps} 0}
configure wave -namecolwidth 340
configure wave -valuecolwidth 40
configure wave -justifyvalue left
configure wave -signalnamewidth 0
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 178
configure wave -gridperiod 356
configure wave -griddelta 8
configure wave -timeline 0
update
WaveRestoreZoom {484745982900 ps} {638965298100 ps}
