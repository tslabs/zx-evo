
# cd y:/Work/Git/zx-evo-dev/pentevo/fpga/current/tb/

catch {exec cmd.exe /c ..\\tools\\sjasmplus.exe --lst=tb.lst tb.asm} err
echo $err
exec cmd.exe /c python.exe -c {with open('tb.bin', 'rb') as f, open('tb.hex', 'w') as o: o.write(' '.join(hex(b)[2:].zfill(2) for b in f.read()))}

vlog +incdir+../../quartus_vdac2_2.0_50k +define+SIMULATE -work tsconf -sv -stats=none -incr -f files.opt
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

add wave -divider SPI
add wave {sim:/tb/top/sdcs_n}
add wave {sim:/tb/top/sdclk}
add wave {sim:/tb/top/sddo}
add wave {sim:/tb/top/sddi}

run 500ns

wave zoom range 0ns 300ns
