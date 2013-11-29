  Версия EVO SDCC/SDK для ZX Evo TSConf.
Позволяет собирать проекты от EVO SDCC/SDK для ZX Evo BASECONF. Для корректного запуска
эмулятора надо отредактировать последнюю строку в compile.bat:
@if %error% ==0 ..\evosdk\tools\unreal_evo\unreal %output%

  Кроме *.scl можно собирать SPG файлы. Для этого надо добавить в начало compile.bat 
следующие строки:
set outspg=filename.spg  ; имя для SPG файла
set spgpack = 2  ; метод упаковки SPG файла: 0 - без сжатия, 1 - MegaLZ, 2 - Hrust. Если строки нет, то
метод упаковки выбирается автоматически.
