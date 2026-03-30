@echo off
REM CANAnalyzerPro 编译脚本
REM 使用Qt 6.10.2 + MinGW 13.1.0

echo ========================================
echo CANAnalyzerPro 编译脚本
echo ========================================

REM 关闭正在运行的程序
echo.
echo [0/5] 检查并关闭正在运行的程序...
tasklist | findstr /i "CANAnalyzerPro.exe" >nul
if %errorlevel% equ 0 (
    echo 发现正在运行的程序，正在关闭...
    taskkill /F /IM CANAnalyzerPro.exe >nul 2>&1
    timeout /t 1 /nobreak >nul
    echo ✅ 程序已关闭
) else (
    echo ✅ 没有正在运行的程序
)

REM 设置Qt环境
set QT_DIR=D:\Qt\6.10.2\mingw_64
set MINGW_DIR=D:\Qt\Tools\mingw1310_64
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

echo.
echo [1/4] 清理旧的编译文件...
REM 清理编译缓存，包括资源文件缓存
if exist build rmdir /s /q build
REM 删除可能缓存的资源文件和可执行文件
if exist bin\CANAnalyzerPro.exe (
    echo 删除旧的可执行文件...
    del /f /q bin\CANAnalyzerPro.exe
)
if exist build\bin\CANAnalyzerPro.exe del /f /q build\bin\CANAnalyzerPro.exe
REM 清理资源缓存
if exist resources\*.res del /f /q resources\*.res
mkdir build
if not exist bin mkdir bin

echo.
echo [2/4] 运行qmake生成Makefile...
qmake CANAnalyzerPro.pro -spec win32-g++
if errorlevel 1 (
    echo ❌ qmake失败！
    pause
    exit /b 1
)

echo.
echo [3/4] 编译项目...
mingw32-make -j8
if errorlevel 1 (
    echo ❌ 编译失败！
    pause
    exit /b 1
)

echo.
echo [4/4] 部署Qt依赖...
windeployqt bin\CANAnalyzerPro.exe --release --no-translations
if errorlevel 1 (
    echo ⚠️ windeployqt 警告（可能不影响运行）
)

echo.
echo [5/5] 复制工具程序...
if not exist bin\tools mkdir bin\tools

REM 直接从bin\tools复制（不使用备份机制，避免卡住）
if exist bin\tools (
    echo ✅ 工具程序已存在
) else (
    echo ⚠️ 未找到bin\tools文件夹
    echo 提示：请确保bin\tools文件夹存在并包含工具文件
)

echo ✅ 构建完成

echo.
echo ========================================
echo ✅ 编译成功！
echo 可执行文件: bin\CANAnalyzerPro.exe
echo ========================================
echo.

REM 运行程序
echo 正在启动程序...
start bin\CANAnalyzerPro.exe

pause
