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
add wave -noupdate /tb/DUT/zports/port_rd_fclk
add wave -noupdate /tb/DUT/zports/port_wr_fclk
add wave -noupdate /tb/DUT/zports/shadow
add wave -noupdate -radix hexadecimal /tb/DUT/zports/loa
add wave -noupdate -radix hexadecimal /tb/DUT/zports/a
add wave -noupdate /tb/DUT/zports/sdcfg_wr
add wave -noupdate /tb/DUT/zports/sddat_rd
add wave -noupdate /tb/DUT/zports/sddat_wr
add wave -noupdate /tb/DUT/zports/sd_start
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/zx_sdcs_n_val
add wave -noupdate /tb/DUT/zx_sdcs_n_stb
add wave -noupdate /tb/DUT/zx_sd_start
add wave -noupdate -radix hexadecimal /tb/DUT/zx_sd_datain
add wave -noupdate -radix hexadecimal /tb/DUT/zx_sd_dataout
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/avr_lock_claim
add wave -noupdate /tb/DUT/avr_lock_grant
add wave -noupdate /tb/DUT/avr_sdcs_n
add wave -noupdate /tb/DUT/avr_sd_start
add wave -noupdate -radix hexadecimal /tb/DUT/avr_sd_datain
add wave -noupdate -radix hexadecimal /tb/DUT/avr_sd_dataout
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/spihub/sd_start
add wave -noupdate -radix hexadecimal /tb/DUT/spihub/sd_datain
add wave -noupdate -radix hexadecimal /tb/DUT/spihub/sd_dataout
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/sdcs_n
add wave -noupdate /tb/DUT/sdclk
add wave -noupdate /tb/DUT/sddo
add wave -noupdate /tb/DUT/sddi
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/spics_n
add wave -noupdate /tb/DUT/spick
add wave -noupdate /tb/DUT/spido
add wave -noupdate /tb/DUT/spidi
add wave -noupdate -divider <NULL>
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
add wave -noupdate /tb/DUT/dram/rras0_n
add wave -noupdate /tb/DUT/dram/rras1_n
add wave -noupdate /tb/DUT/dram/rucas_n
add wave -noupdate /tb/DUT/dram/rlcas_n
add wave -noupdate /tb/DUT/dram/rwe_n
add wave -noupdate -radix hexadecimal /tb/DUT/dram/ra
add wave -noupdate -radix hexadecimal /tb/DUT/dram/rd
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
add wave -noupdate /tb/DUT/video_top/clk
add wave -noupdate /tb/DUT/video_top/cbeg
add wave -noupdate /tb/DUT/video_top/post_cbeg
add wave -noupdate /tb/DUT/video_top/pre_cend
add wave -noupdate /tb/DUT/video_top/cend
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/video_top/int_start
add wave -noupdate /tb/DUT/video_top/line_start
add wave -noupdate /tb/DUT/video_top/hpix
add wave -noupdate /tb/DUT/video_top/vpix
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/video_top/fetch_start
add wave -noupdate /tb/DUT/video_top/fetch_end
add wave -noupdate /tb/DUT/video_top/fetch_sync
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/video_top/video_go
add wave -noupdate -radix hexadecimal /tb/DUT/video_top/video_addr
add wave -noupdate /tb/DUT/video_top/video_next
add wave -noupdate -radix hexadecimal /tb/DUT/video_top/video_data
add wave -noupdate /tb/DUT/video_top/video_strobe
add wave -noupdate /tb/DUT/video_top/video_bw
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/video_top/video_addrgen/frame_init
add wave -noupdate /tb/DUT/video_top/video_addrgen/gnext
add wave -noupdate -radix hexadecimal /tb/DUT/video_top/video_addrgen/gctr
add wave -noupdate /tb/DUT/video_top/video_addrgen/ldaddr
add wave -noupdate -radix hexadecimal /tb/DUT/video_top/video_addrgen/video_addr
add wave -noupdate -divider <NULL>
add wave -noupdate -radix hexadecimal /tb/DUT/video_top/pic_bits
add wave -noupdate -divider <NULL>
add wave -noupdate -radix unsigned /tb/DUT/video_top/vred
add wave -noupdate -radix unsigned /tb/DUT/video_top/vgrn
add wave -noupdate -radix unsigned /tb/DUT/video_top/vblu
add wave -noupdate -divider <NULL>
add wave -noupdate /tb/DUT/znmi/set_nmi
add wave -noupdate /tb/DUT/znmi/set_nmi_r
add wave -noupdate /tb/DUT/znmi/set_nmi_now
add wave -noupdate /tb/DUT/znmi/pending_nmi
add wave -noupdate /tb/DUT/znmi/nmi_count
add wave -noupdate /tb/DUT/znmi/gen_nmi
add wave -noupdate /tb/DUT/znmi/imm_nmi
add wave -noupdate /tb/DUT/znmi/imm_nmi_r
add wave -noupdate /tb/DUT/znmi/imm_nmi_now
add wave -noupdate -radix hexadecimal /tb/DUT/zbreak/a
add wave -noupdate -radix hexadecimal /tb/DUT/zbreak/brk_addr
add wave -noupdate /tb/DUT/zbreak/brk_ena
add wave -noupdate -divider <NULL>
add wave -noupdate -divider <NULL>
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
WaveRestoreCursors {{Cursor 1} {650522851854 ps} 0}
configure wave -namecolwidth 241
configure wave -valuecolwidth 40
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
WaveRestoreZoom {624869038311 ps} {658487099296 ps}
