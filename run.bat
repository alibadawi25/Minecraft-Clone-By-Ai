@echo off
echo Running Minecraft Clone (Release)...
echo.

if not exist ".\build\bin\Release\MinecraftClone.exe" (
    echo ERROR: Executable not found!
    echo Please build the project first using build.bat
    pause
    exit /b 1
)

.\build\bin\Release\MinecraftClone.exe
