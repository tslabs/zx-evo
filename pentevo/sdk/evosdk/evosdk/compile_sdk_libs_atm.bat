@echo off
path=tools\sdcc\bin;tools\sjasmplus

echo  define ATM >target.asm

sjasmplus lib_startup.asm
sjasmplus lib_sndpage.asm

as-z80 -o crt0.o crt0.s

ren lib_startup.exp startup.exp
exp2h startup.exp

sdcc -mz80 -I. -c evo.c

pause

del startup.exp
del evo.asm
del evo.lst
del evo.sym