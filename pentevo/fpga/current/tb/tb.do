
vlog +incdir+../quartus_vdac2 +define+SIMULATE -work tsconf -sv -stats=none -f files.opt
vsim -gui -novopt tsconf.tb

view wave
delete wave *
# restart -f

configure wave -gridperiod 10ps
configure wave -griddelta 12
configure wave -gridcolor #303030
configure wave -timecolor cyan
configure wave -vectorcolor #B3FFB3

# add wave {sim:/tb/*}
add wave {sim:/tb/clk}

add wave -divider Z80
add wave {sim:/tb/z80/CLK}
add wave {sim:/tb/z80/nM1}
add wave {sim:/tb/z80/nIORQ}
add wave {sim:/tb/z80/nMREQ}
add wave {sim:/tb/z80/nRD}
add wave {sim:/tb/z80/nWR}
add wave {sim:/tb/z80/nWAIT}
add wave {sim:/tb/a}
add wave {sim:/tb/d}
# add wave {sim:/tb/z80/*}
# add wave {sim:/tb/top/zint/*}
# add wave {sim:/tb/top/zwait/*}

# add wave -divider CACHE
# add wave {sim:/tb/top/zmem/cache_hit}
# add wave {sim:/tb/top/zmem/cpu_req}
# add wave {sim:/tb/top/arbiter/curr_cycle}

add wave -divider AVR
add wave {sim:/tb/avr/*}

add wave -divider SLAVESPI
add wave {sim:/tb/top/slavespi/wait_addr}
add wave {sim:/tb/top/slavespi/status_in}
add wave {st_addr {sim:/tb/top/slavespi/status_in[6:2]}}
# add wave {sim:/tb/top/slavespi/sck_01}
# add wave {sim:/tb/top/slavespi/sdo}
# add wave {sim:/tb/top/slavespi/regnum}
add wave {sim:/tb/top/slavespi/*}

# add wave -divider DRAM
# add wave {sim:/tb/top/dram/*}

# add wave -divider ARBITER
# add wave {sim:/tb/top/arbiter/curr_cycle}
# add wave {sim:/tb/top/arbiter/*}

# add wave -divider misc
# add wave {sim:/tb/top/ena_ram}
# add wave {sim:/tb/top/ena_ports}
# add wave {sim:/tb/top/dout_ram}
# add wave {sim:/tb/top/dout_ports}
# add wave {sim:/tb/top/intack}
# add wave {sim:/tb/top/im2vect}
# add wave {sim:/tb/top/drive_ff}

# add wave -divider misc
# add wave {sim:/tb/rom_addr}
# add wave {sim:/tb/top/zports/peff7}
# add wave {sim:/tb/top/zports/gluclock_on}
# add wave {sim:/tb/top/zports/wait_start_gluclock}
# add wave {sim:/tb/top/wait_n}

# add wave -divider TOP
# add wave {sim:/tb/top/*}

run 3500ns
