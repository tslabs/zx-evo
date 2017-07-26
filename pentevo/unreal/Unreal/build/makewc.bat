
set temp=c:\temp_unreal_compile

if exist %temp% rd /s /q %temp%
md %temp%

..\..\tools\7z\7z.exe e -y -bd -o"%temp%" .\cfg\hdd\empty_100MB.7z
..\..\tools\mtools\imgcpy.exe %temp%\wc.img=c:\wc
..\..\tools\mtools\imgcpy.exe ..\..\soft\wc\exe\boot.$C %temp%\wc.img=c:\boot.$c
for %%i in (..\..\soft\wc\exe\wc\*.*) do ..\..\tools\mtools\imgcpy.exe %%i %temp%\wc.img=c:\wc\%%~ni%%~xi

copy /b /y %temp%\wc.img "%1"
rd /s /q %temp%
