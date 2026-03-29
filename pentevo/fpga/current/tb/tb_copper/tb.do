
# cd z:/Work/Git/zx-evo-dev/pentevo/fpga/current/tb/tb_copper/

exec cmd.exe /c python.exe ../../../../tools/copper/copper_asm.py tb.cls copper.asm
catch {exec cmd.exe /c ..\\..\\..\\..\\tools\\sjasmplus\\sjasmplus.exe --lst=tb.lst tb.asm} err
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
configure wave -namecolwidth 200

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
add wave {sim:/tb/z80/nINT}
add wave {sim:/tb/a}
add wave {sim:/tb/d}
add wave {sim:/tb/top/zports/copper_ready}
add wave {sim:/tb/top/int_n}
add wave {sim:/tb/top/zint/int_start_cpr}
add wave {sim:/tb/top/zint/int_cpr}

add wave -divider COPPER
add wave {sim:/tb/top/copper/clk}
add wave {sim:/tb/top/copper/en}
add wave {sim:/tb/top/copper/pc}
add wave {sim:/tb/top/copper/cl_data}
add wave {sim:/tb/top/copper/sig_rdy}
add wave {sim:/tb/top/copper/sig_int}
add wave {sim:/tb/top/copper/ts_reg_wr}
add wave {sim:/tb/top/copper/ts_reg_addr}
add wave {sim:/tb/top/copper/ts_reg_data}
# add wave {sim:/tb/top/copper/cpu_xt_access}
# add wave {sim:/tb/top/copper/cpu_wr}
# add wave {sim:/tb/top/copper/cpu_data}
# add wave {sim:/tb/top/copper/cl_wr}
# add wave {sim:/tb/top/copper/cl_wr_addr}
# add wave {sim:/tb/top/copper/cl_wr_data}
add wave {sim:/tb/top/copper/ray_x}
add wave {sim:/tb/top/copper/ray_y}
add wave {sim:/tb/top/copper/line_start_s}
add wave {sim:/tb/top/copper/frame_start_s}

add wave -divider DMA
add wave {sim:/tb/top/dma/clk}
add wave {sim:/tb/top/dma/dma_act}
add wave {sim:/tb/top/dma/dram_addr}

run 300000ns

wave zoom range 5210ns 5310ns
