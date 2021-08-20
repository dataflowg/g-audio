@echo off
setlocal enabledelayedexpansion enableextensions
set CPP=
for %%x in (*.cpp) do set CPP=!CPP! %%x
set CPP=%CPP:~1%
set BUILD_PATH=lib
set BUILD_PATH_LINUX=%BUILD_PATH%\linux
set BUILD_PATH_MACOS=%BUILD_PATH%\macos
set LIB_PATH=..\LabVIEW\G-Audio\lib
set LIB_PATH_LINUX=%LIB_PATH%\linux
set LIB_PATH_MACOS=%LIB_PATH%\macos
mkdir %LIB_PATH%
mkdir %LIB_PATH_LINUX%
mkdir %LIB_PATH_MACOS%
@echo on

:: Windows
:: Currently using Visual Studio to build these
:: zig\zig.exe c++ -shared -o %BUILD_PATH%\g_audio_x86.dll %CPP% -target i386-windows-gnu
:: move /Y %BUILD_PATH%\g_audio_x86.dll %LIB_PATH%\g_audio_x86.dll
:: zig\zig.exe c++ -shared -o %BUILD_PATH%\g_audio_x64.dll %CPP% -target x86_64-windows-gnu
:: move /Y %BUILD_PATH%\g_audio_x64.dll %LIB_PATH%\g_audio_x64.dll

:: Linux
zig\zig.exe c++ -shared -fPIC -o %BUILD_PATH%\linux\g_audio_x64.so %CPP% -lm -lpthread -ldl -target x86_64-linux-gnu
move /Y %BUILD_PATH_LINUX%\g_audio_x64.so %LIB_PATH_LINUX%

:: MacOS
:: Note that MA_NO_COREAUDIO is defined here for two reasons:
:: 1) We don't have the CoreAudio headers to compile against
:: 2) #define-ing MA_NO_COREAUDIO in g_audio.h before including miniaudio.h doesn't work - suspect it's a zig compiler issue, as defining it on the command line here works
zig\zig.exe c++ -dynamiclib -o %BUILD_PATH%\macos\g_audio_x64.dylib %CPP% -target x86_64-macos-gnu -D MA_NO_COREAUDIO
move /Y %BUILD_PATH_MACOS%\g_audio_x64.dylib %LIB_PATH_MACOS%

@echo off
rmdir /S /Q %BUILD_PATH%

pause