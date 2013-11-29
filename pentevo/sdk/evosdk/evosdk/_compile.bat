rem скрипт сборки проекта

if not defined output goto end
if %output%=="" goto end
if %title%=="" goto end

set error=1

PATH=..\evosdk\tools\sdcc\bin;..\evosdk
set temp=_temp_

rem создаЄм временную директорию дл€ компил€ции

mkdir %temp%

rem создаЄм список палитр

set palette._dummy_=:

echo rem palette>%temp%\palette.lst
FOR /F "tokens=2* delims=.=" %%A IN ('SET palette') DO ECHO %%B>>%temp%\palette.lst

rem создаЄм список изображений

set image._dummy_=:

echo rem image>%temp%\image.lst
FOR /F "tokens=2* delims=.=" %%A IN ('SET image') DO ECHO %%B>>%temp%\image.lst

rem создаЄм список спрайтов

set sprite._dummy_=:

echo rem sprite>%temp%\sprite.lst
FOR /F "tokens=2* delims=.=" %%A IN ('SET sprite') DO ECHO %%B>>%temp%\sprite.lst

rem создаЄм список музыки

set music._dummy_=:

echo rem music>%temp%\music.lst
for /F "tokens=2* delims=.=" %%a in ('set music') do echo %%b>>%temp%\music.lst

rem создаЄм список сэмплов

set sample._dummy_=:

echo rem sample>%temp%\sample.lst
FOR /F "tokens=2* delims=.=" %%A IN ('SET sample') DO ECHO %%B>>%temp%\sample.lst

rem создаЄм resources.h с идентификаторами ресурсов

makeresh "%temp%\image.lst" "%temp%\palette.lst" "%temp%\music.lst" "%temp%\sample.lst" "%temp%\sprite.lst" "%soundfx%"

rem компилируем исходник на C

sdcc -mz80 --code-loc 0x0006 --data-loc 0 --no-std-crt0 -I..\evosdk ..\evosdk\crt0.o ..\evosdk\evo.o --opt-code-speed main.c -o %temp%\out.ihx

if ERRORLEVEL 1 goto clean

rem вызываем компил€тор ресурсов
rem он создаЄт набор бинарных файлов по одному на банк пам€ти
rem плюс скрипты дл€ сжати€ файлов megalz и сборки образа диска

evoresc "%temp%\out.ihx" "..\evosdk\startup.bin" "%soundfx%" "%temp%\music.lst" "%temp%\palette.lst" "%temp%\image.lst" "%temp%\sample.lst" "%temp%\sprite.lst"

if ERRORLEVEL 1 goto clean

rem переходим во временную директорию

cd %temp%

rem пакуем файлы

copy ..\..\evosdk\getsize.bat >nul
call compress.bat

rem собираем загрузчик

copy ..\..\evosdk\loader.asm loader.asm >nul
copy ..\..\evosdk\unmegalz.asm unmegalz.asm >nul
copy ..\..\evosdk\target.asm target.asm >nul
..\..\evosdk\tools\sjasmplus\sjasmplus.exe loader.asm >nul

rem собираем образ и делаем его моноблочным

call createscl.bat

cd ..

copy %temp%\disk.scl %output% >nul
..\evosdk\monoscl %output%

set error=0

rem удал€ем временную директорию

:clean

rd /s /q %temp%

:end

if %error%==1 pause