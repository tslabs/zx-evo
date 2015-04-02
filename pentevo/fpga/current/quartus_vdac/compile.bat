
@echo off
setlocal enabledelayedexpansion

set QP=c:\Tools\PROG\Qua9.0\quartus\bin
if (%1)==() (set st=1) else (set st=%1)

echo Synthesis
start /b /wait /low %QP%/quartus_map pentevo -c top >map.log

for /l %%i in (%st%,1,1000) do call :flow %%i

echo Giving up!..
goto :exit

:flow
echo Fitter seed = %1
start /b /wait /low %QP%/quartus_fit pentevo -c top --seed=%1 >fit.log
if %errorlevel% gtr 0 (
  echo Error
  goto :ret
)

start /b /wait /low %QP%/quartus_asm pentevo -c top >asm.log
start /b /wait /low %QP%/quartus_tan pentevo -c top --speed=3 >tan.log

for /f "tokens=2,3,4,5 delims=;. " %%a in (top.tan.rpt) do if %%a==Setup: if %%b=='fclk' (
  if %%c==-0 (set s=-1) else if %%c==0 (set s=1) else (set s=%%c)
  call :chk %%c%%d !s!
  goto :ret
)
 
:chk
echo Slack = %1

if %2 lss 0 (
  call :copy slack%1
  goto :ret
) else (
  echo Success
  goto :exit
)

:copy
if exist %1 goto :ret
md %1
copy *.qsf %1 >nul
copy *.rbf %1 >nul
copy *.sof %1 >nul
copy *.rpt %1 >nul
copy *.log %1 >nul
copy *.summary %1 >nul
goto :ret

:exit
pause
exit

:ret
