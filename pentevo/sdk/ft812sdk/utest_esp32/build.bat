
set PRJ=utest_esp32

call ..\lib\sdk\build_trd.bat %PRJ%
REM call run.bat
REM python ../../../tools/rs232mnt_py/rs232mnt.py -a %PRJ%.trd -com COM9 -baud 230400