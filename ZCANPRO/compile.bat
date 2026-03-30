@echo off
set QT_DIR=D:\Qt\6.10.2\mingw_64
set MINGW_DIR=D:\Qt\Tools\mingw1310_64
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

echo Running qmake...
qmake CANAnalyzerPro.pro -spec win32-g++
if errorlevel 1 exit /b 1

echo Running make...
mingw32-make -j8
if errorlevel 1 exit /b 1

echo Build success.
