@echo off
setlocal enabledelayedexpansion

echo ============================================
echo    Minecraft Clone - MinGW64 Build Script
echo ============================================
echo.

REM Check if MinGW is available
where mingw32-make >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: MinGW64 not found in PATH!
    echo Please install MinGW64 and add it to your PATH.
    pause
    exit /b 1
)

REM Create build directory
set BUILD_DIR=build_mingw
if not exist %BUILD_DIR% (
    echo Creating build directory: %BUILD_DIR%
    mkdir %BUILD_DIR%
)

echo Entering build directory...
cd %BUILD_DIR%

echo Configuring project with CMake...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Check the error messages above.
    pause
    exit /b 1
)

echo.
echo Building project...
mingw32-make -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed!
    echo Check the error messages above.
    pause
    exit /b 1
)

echo.
echo ============================================
echo              BUILD SUCCESSFUL!
echo ============================================
echo.
echo Executable created: .\build_mingw\bin\MinecraftClone.exe
echo.
echo Run with: .\build_mingw\bin\MinecraftClone.exe
echo Or simply: .\run_mingw.bat
echo.
pause