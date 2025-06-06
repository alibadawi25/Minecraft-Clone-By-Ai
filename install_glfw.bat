@echo off
echo Installing GLFW library...

set GLFW_URL=https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip
set EXTERNAL_DIR=c:\Users\aliba\OneDrive\Documents\EvilDungeons\external
set GLFW_DIR=%EXTERNAL_DIR%\glfw

if not exist "%EXTERNAL_DIR%" (
    mkdir "%EXTERNAL_DIR%"
)

if not exist "%GLFW_DIR%" (
    echo Downloading GLFW...
    powershell -Command "Invoke-WebRequest -Uri '%GLFW_URL%' -OutFile 'glfw.zip'"
    echo Extracting GLFW...
    powershell -Command "Expand-Archive -Path 'glfw.zip' -DestinationPath '%EXTERNAL_DIR%'"

    REM Rename the extracted folder to just 'glfw'
    if exist "%EXTERNAL_DIR%\glfw-3.3.8.bin.WIN64" (
        move "%EXTERNAL_DIR%\glfw-3.3.8.bin.WIN64" "%GLFW_DIR%"
    )

    del glfw.zip
    echo GLFW installed successfully!
) else (
    echo GLFW already exists at %GLFW_DIR%
)

pause
