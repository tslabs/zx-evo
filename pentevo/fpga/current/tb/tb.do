vlog +incdir+../quartus_vdac2 -work tsconf -sv -stats=none -f files.opt
# vsim -gui -novopt tsconf.tb

view wave

delete wave *
add wave {sim:/tb/*}
add wave {sim:/tb/clock/*}

restart -f
run 100ns