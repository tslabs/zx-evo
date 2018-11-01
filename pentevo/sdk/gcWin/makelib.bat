@echo off
set path=%path%;%SDCC%\bin\

if not exist lib md lib

if not exist obj md obj
del /q /s obj >nul

sdcc.exe --std-c11 -mz80 --no-std-crt0 --opt-code-speed -c src\gcWin.c -o obj\gcWin.rel
if errorlevel 1 pause & exit

sdcc.exe --std-c11 -mz80 --no-std-crt0 --opt-code-speed -c src\keyboard.c -o obj\keyboard.rel
if errorlevel 1 pause & exit

sdcclib lib\gcwin.lib obj\gcwin.rel obj\keyboard.rel
