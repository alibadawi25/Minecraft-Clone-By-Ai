@echo off
echo Running Minecraft Clone (Release)...
echo.

if not exist ".\build_mingw\bin\MinecraftClone.exe" (
    echo ERROR: Executable not found!
    echo Please build the project first using build_mingw.bat
    pause
    exit /b 1
)

.\build_mingw\bin\MinecraftClone.exe
