FOR /F "tokens=*" %%g IN ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -products * -property installationPath') do (SET vspath=%%g)
call "%vspath%\Common7\Tools\VsDevCmd.bat"
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=win32 /t:rebuild /m
if %ERRORLEVEL% GEQ 1 goto done
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=x64 /t:rebuild /m
if %ERRORLEVEL% GEQ 1 goto done
msbuild EqualizerAPO.sln /p:Configuration=release /p:platform=ARM64 /t:rebuild /m
if %ERRORLEVEL% GEQ 1 goto done

if not exist build-Editor-Desktop_Qt_6_7_2_MSVC2022_32bit mkdir build-Editor-Desktop_Qt_6_7_2_MSVC2022_32bit
cd build-Editor-Desktop_Qt_6_7_2_MSVC2022_32bit
call "%vspath%\VC\Auxiliary\Build\vcvarsall.bat" x86
"C:\Qt\6.7.2\msvc2022\bin\qmake.exe" ..\Editor\Editor.pro -r "CONFIG+=release"
"C:\Qt\Tools\QtCreator\bin\jom\jom.exe" clean
"C:\Qt\Tools\QtCreator\bin\jom\jom.exe"
if %ERRORLEVEL% GEQ 1 goto done
cd..

if not exist build-Editor-Desktop_Qt_6_7_2_MSVC2022_64bit mkdir build-Editor-Desktop_Qt_6_7_2_MSVC2022_64bit
cd build-Editor-Desktop_Qt_6_7_2_MSVC2022_64bit
call "%vspath%\VC\Auxiliary\Build\vcvarsall.bat" x64
"C:\Qt\6.7.2\msvc2022_64\bin\qmake.exe" ..\Editor\Editor.pro -r "CONFIG+=release"
"C:\Qt\Tools\QtCreator\bin\jom\jom.exe" clean
"C:\Qt\Tools\QtCreator\bin\jom\jom.exe"
if %ERRORLEVEL% GEQ 1 goto done
cd..

if not exist build-Editor-Desktop_Qt_6_7_2_MSVC2019_ARM64 mkdir build-Editor-Desktop_Qt_6_7_2_MSVC2019_ARM64
cd build-Editor-Desktop_Qt_6_7_2_MSVC2019_ARM64
call "%vspath%\VC\Auxiliary\Build\vcvarsall.bat" x64_arm64
call "C:\Qt\6.7.2\msvc2019_arm64\bin\qmake.bat" ..\Editor\Editor.pro -r "CONFIG+=release"
"C:\Qt\Tools\QtCreator\bin\jom\jom.exe" clean
"C:\Qt\Tools\QtCreator\bin\jom\jom.exe"
if %ERRORLEVEL% GEQ 1 goto done
cd..

if defined ProgramFiles(x86) (
	set nsis="%ProgramFiles(x86)%\NSIS\makensis.exe"
) else (
	set nsis="%ProgramFiles%\NSIS\makensis.exe"
)

cd Setup
%nsis% Setup32.nsi
if %ERRORLEVEL% GEQ 1 goto done
%nsis% Setup64.nsi
if %ERRORLEVEL% GEQ 1 goto done
%nsis% SetupARM64.nsi
cd..

:done
pause
