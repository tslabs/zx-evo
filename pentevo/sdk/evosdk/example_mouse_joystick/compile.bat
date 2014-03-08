@echo off

rem имя SCL файла

set output=mouse_joystick.scl

rem сообщение, которое отображается при загрузке
rem 32 символа, стандартный шрифт

set title=" MOUSE AND JOYSTICK DEMO"

rem список изображений, откуда брать палитры
rem в программе они вызываются по автоматически генерируемым
rem идентификаторам в файле resources.h
rem нумерация после точки должна быть возрастающей

set palette.0=back.bmp

rem список изображений, откуда брать графику

set image.0=back.bmp

rem спрайты

set sprite.0=arrow.bmp

rem набор звуковых эффектов, если нужен
rem он может быть только один

set soundfx=

rem музыка, нужное число треков

set music.0=

call ..\evosdk\_compile.bat
@if %error% ==0 ..\evosdk\tools\unreal_evo\emullvd %output%