onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /tb/fclk
add wave -noupdate /tb/clkz_out
add wave -noupdate /tb/zrst_n
add wave -noupdate /tb/clkz_in
add wave -noupdate -divider <NULL>
add wave -noupdate -color Salmon /tb/DUT/zclock/zpos
add wave -noupdate -color Salmon /tb/DUT/zclock/zneg
add wave -noupdate /tb/z80/busrq_n
add wave -noupdate /tb/z80/busak_n
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
add wave -noupdate -radix hexadecimal -subitemconfig {{/tb/zd[7]} {-radix hexadecimal} {/tb/zd[6]} {-radix hexadecimal} {/tb/zd[5]} {-radix hexadecimal} {/tb/zd[4]} {-radix hexadecimal} {/tb/zd[3]} {-radix hexadecimal} {/tb/zd[2]} {-radix hexadecimal} {/tb/zd[1]} {-radix hexadecimal} {/tb/zd[0]} {-radix hexadecimal}} /tb/zd
add wave -noupdate /tb/csrom
add wave -noupdate /tb/romoe_n
add wave -noupdate /tb/romwe_n
add wave -noupdate -divider <NULL>
add wave -noupdate -radix hexadecimal {/tb/DUT/instantiate_atm_pagers[0]/atm_pager/page}
add wave -noupdate {/tb/DUT/instantiate_atm_pagers[0]/atm_pager/romnram}
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate -radix hexadecimal /tb/z80/u0/ir
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/zports/atmF7_wr_fclk
add wave -noupdate /tb/DUT/zports/portf7_wr
add wave -noupdate /tb/DUT/zports/portf7_rd
add wave -noupdate -radix hexadecimal /tb/DUT/zports/peff7_int
add wave -noupdate /tb/DUT/zports/atm77_wr_fclk
add wave -noupdate /tb/DUT/zports/zxevbf_wr_fclk
add wave -noupdate /tb/DUT/zports/shadow
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/video_top/int_start
add wave -noupdate /tb/DUT/video_top/vhsync
add wave -noupdate /tb/DUT/video_top/vvsync
add wave -noupdate /tb/DUT/video_top/vred
add wave -noupdate /tb/DUT/video_top/vgrn
add wave -noupdate /tb/DUT/video_top/vblu
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {267699653665 ps} 0} {{Cursor 2} {10594973100 ps} 0}
configure wave -namecolwidth 353
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
configure wave -timelineunits ns
update
WaveRestoreZoom {0 ps} {1050315 us}
