@echo off
for %%i in (*.png) do (
  squishpng.exe -i %%i %%~ni.dxt
  dxt2dxp.exe %%~ni.dxt
  del %%~ni.dxt
)
pause
