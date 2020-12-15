@echo off
setlocal enabledelayedexpansion enableextensions
set CPP=
for %%x in (*.cpp) do set CPP=!CPP! %%x
set CPP=%CPP:~1%
set BUILD_PATH=lib
set LIB_PATH=..\LabVIEW\G-Audio\lib
@echo on

:: Windows
:: Currently using Visual Studio to build these
:: zig\zig.exe c++ -shared -o %BUILD_PATH%\g_audio_x86.dll %CPP% -target i386-windows-gnu
:: zig\zig.exe c++ -shared -o %BUILD_PATH%\g_audio_x64.dll %CPP% -target x86_64-windows-gnu

:: Linux
zig\zig.exe c++ -shared -o %BUILD_PATH%\g_audio_x64.so %CPP% -target x86_64-linux-gnu

:: MacOS
:: zig\zig.exe c++ -shared -o %BUILD_PATH%\g_audio_x64.so %CPP% -target x86_64-macos-gnu

move %BUILD_PATH%\* %LIB_PATH%

@echo off
rmdir lib

pause