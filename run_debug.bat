@echo off
echo Running Minecraft Clone (Debug)...
echo.

if not exist ".\build_debug\bin\MinecraftClone.exe" (
    echo ERROR: Debug executable not found!
    echo Please build the debug version first using build_debug.bat
    pause
    exit /b 1
)

.\build_debug\bin\MinecraftClone.exe
