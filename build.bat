if not exist build-Editor-Desktop_Qt_5_15_MSVC2019_64bit-Release mkdir build-Editor-Desktop_Qt_5_15_MSVC2019_64bit-Release
cd build-Editor-Desktop_Qt_5_15_MSVC2019_64bit-Release
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
"C:\Qt\5.15.0\msvc2019_64\bin\qmake.exe" "C:\Users\matty\Downloads\equalizerapo-code-r82-tags-1.2.1\equalizerapo-code-r82-tags-1.2.1\Editor\Editor.pro" -r -spec win32-msvc "CONFIG+=release"
"C:\Qt\Tools\QtCreator\bin\jom.exe" clean
"C:\Qt\Tools\QtCreator\bin\jom.exe"
if %ERRORLEVEL% GEQ 1 goto done
cd..

if defined ProgramFiles(x86) (
	set nsis="%ProgramFiles(x86)%\NSIS\makensis.exe"
) else (
	set nsis="%ProgramFiles%\NSIS\makensis.exe"
)

cd Setup
%nsis% Setup64.nsi
cd..

:done
pause
