@echo off
REM ============================================
REM iPhone Stock Management - Windows Build Script
REM ============================================
REM
REM Usage:
REM 1. Install Qt6 (6.5 or higher recommended)
REM 2. Install CMake 3.16 or higher
REM 3. Install Visual Studio 2019 or higher (with C++ tools)
REM 4. Open "x64 Native Tools Command Prompt for VS" from Start Menu
REM 5. Run this script in that command prompt
REM
REM ============================================

setlocal enabledelayedexpansion

echo ============================================
echo   iPhone Stock Management - Windows Build
echo ============================================
echo.

REM Check Qt environment
where qmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] qmake not found. Please ensure Qt6 is installed and added to PATH
    echo.
    echo Please set Qt6 path, for example:
    echo set PATH=C:\Qt\6.10.1\msvc2022_64\bin;%%PATH%%
    pause
    exit /b 1
)

REM Check CMake
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] cmake not found. Please install CMake 3.16 or higher
    pause
    exit /b 1
)

REM Show version info
echo [INFO] Detected tool versions:
qmake -v
cmake --version | findstr "cmake version"
echo.

REM Create build directory
if exist build-windows rmdir /s /q build-windows
mkdir build-windows
cd build-windows

echo [Step 1/4] Configuring CMake...
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed
    cd ..
    pause
    exit /b 1
)
echo.

echo [Step 2/4] Building project...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed
    cd ..
    pause
    exit /b 1
)
echo.

echo [Step 3/4] Creating release directory...
if not exist deploy mkdir deploy
copy bin\iPhoneStockManagement.exe deploy\ >nul 2>&1
if not exist bin\iPhoneStockManagement.exe (
    copy iPhoneStockManagement.exe deploy\ >nul 2>&1
)
echo.

echo [Step 4/4] Packaging Qt dependencies with windeployqt...
cd deploy
windeployqt iPhoneStockManagement.exe --release --no-translations
if %ERRORLEVEL% neq 0 (
    echo [WARN] windeployqt encountered issues, but may have partially completed
)
echo.

REM Copy SQL driver
echo [INFO] Copying SQL driver...
set QT_PLUGINS_DIR=
for /f "delims=" %%i in ('qmake -query QT_INSTALL_PLUGINS') do set QT_PLUGINS_DIR=%%i
if exist "!QT_PLUGINS_DIR!\sqldrivers" (
    if not exist sqldrivers mkdir sqldrivers
    copy "!QT_PLUGINS_DIR!\sqldrivers\qsqlite.dll" sqldrivers\ >nul 2>&1
)

cd ..\..

echo.
echo ============================================
echo   Build Complete!
echo ============================================
echo.
echo Output directory: build-windows\deploy\
echo Executable: build-windows\deploy\iPhoneStockManagement.exe
echo.
echo You can package the deploy folder into a ZIP or create an installer
echo.

pause
