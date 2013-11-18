@echo off

rem имя SCL файла

set output=slideshow.scl

rem сообщение, которое отображается при загрузке
rem 32 символа, стандартный шрифт

set title=" SLIDESHOW IS LOADING"

rem список изображений, откуда брать палитры
rem в программе они вызываются по автоматически генерируемым
rem идентификаторам в файле resources.h
rem нумерация после точки должна быть возрастающей

set palette.0=gfx\pic1.bmp
set palette.1=gfx\pic2.bmp
set palette.2=gfx\pic3.bmp
set palette.3=gfx\pic4.bmp
set palette.4=gfx\pic5.bmp

rem список изображений, откуда брать графику

set image.0=gfx\pic1.bmp
set image.1=gfx\pic2.bmp
set image.2=gfx\pic3.bmp
set image.3=gfx\pic4.bmp
set image.4=gfx\pic5.bmp

rem спрайты

set sprite.0=

rem набор звуковых эффектов, если нужен
rem он может быть только один

set soundfx=

rem музыка, нужное число треков

set music.0=

call ..\evosdk\_compile.bat
@if %error% ==0 ..\evosdk\tools\unreal_evo\emullvd %output%