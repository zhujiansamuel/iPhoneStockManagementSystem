@echo off
REM ============================================
REM iPhone 库存管理系统 - Windows 打包脚本
REM ============================================
REM
REM 使用说明：
REM 1. 确保已安装 Qt6 (建议 6.5 或更高版本)
REM 2. 确保已安装 CMake 3.16 或更高版本
REM 3. 确保已安装 Visual Studio 2019 或更高版本（包含 C++ 工具）
REM 4. 在开始菜单中打开 "x64 Native Tools Command Prompt for VS"
REM 5. 在该命令提示符中运行此脚本
REM
REM ============================================

setlocal enabledelayedexpansion

echo ============================================
echo   iPhone 库存管理系统 - Windows 构建
echo ============================================
echo.

REM 检查 Qt 环境
where qmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [错误] 未找到 qmake，请确保 Qt6 已正确安装并添加到 PATH
    echo.
    echo 请设置 Qt6 路径，例如：
    echo set PATH=C:\Qt\6.5.0\msvc2019_64\bin;%%PATH%%
    pause
    exit /b 1
)

REM 检查 CMake
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [错误] 未找到 cmake，请安装 CMake 3.16 或更高版本
    pause
    exit /b 1
)

REM 显示版本信息
echo [信息] 检测到的工具版本：
qmake -v
cmake --version | findstr "cmake version"
echo.

REM 创建构建目录
if exist build-windows rmdir /s /q build-windows
mkdir build-windows
cd build-windows

echo [步骤 1/4] 配置 CMake...
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [错误] CMake 配置失败
    cd ..
    pause
    exit /b 1
)
echo.

echo [步骤 2/4] 编译项目...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo [错误] 编译失败
    cd ..
    pause
    exit /b 1
)
echo.

echo [步骤 3/4] 创建发布目录...
if not exist deploy mkdir deploy
copy bin\iPhoneStockManagement.exe deploy\ >nul 2>&1
if not exist bin\iPhoneStockManagement.exe (
    copy iPhoneStockManagement.exe deploy\ >nul 2>&1
)
echo.

echo [步骤 4/4] 使用 windeployqt 打包 Qt 依赖...
cd deploy
windeployqt iPhoneStockManagement.exe --release --no-translations
if %ERRORLEVEL% neq 0 (
    echo [警告] windeployqt 执行遇到问题，但可能已部分完成
)
echo.

REM 复制 SQL 驱动（如果需要）
echo [信息] 复制 SQL 驱动...
set QT_PLUGINS_DIR=
for /f "delims=" %%i in ('qmake -query QT_INSTALL_PLUGINS') do set QT_PLUGINS_DIR=%%i
if exist "!QT_PLUGINS_DIR!\sqldrivers" (
    if not exist sqldrivers mkdir sqldrivers
    copy "!QT_PLUGINS_DIR!\sqldrivers\qsqlite.dll" sqldrivers\ >nul 2>&1
)

cd ..\..

echo.
echo ============================================
echo   构建完成！
echo ============================================
echo.
echo 输出目录: build-windows\deploy\
echo 主程序: build-windows\deploy\iPhoneStockManagement.exe
echo.
echo 可以将 deploy 文件夹打包成 ZIP 或制作安装程序
echo.

pause
