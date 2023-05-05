@echo off
if not defined DevEnvDir (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

mkdir bin\x64-debug

SET files=src/platform/platform_win32.cpp src/core/assert.cpp src/core/logger.cpp
SET includes=/Isrc /I%VULKAN_SDK%/Include /Ic:/src
SET links=/link /LIBPATH:%VULKAN_SDK%\Lib /LIBPATH:C:\src\lib\debug vulkan-1.lib user32.lib kernel32.lib ole32.lib gdi32.lib
SET defines=/D _DEBUG /D PLATFORM_WINDOWS

cl /std:c++17 /MDd /Zi /EHsc /Febin\x64-debug\ /Fdbin\x64-debug\ /Fobin\x64-debug\ %includes% %defines% %files% %links%