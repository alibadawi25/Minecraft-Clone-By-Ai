@echo off
echo Installing GLM library...

set GLM_URL=https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip
set EXTERNAL_DIR=c:\Users\aliba\OneDrive\Documents\EvilDungeons\external
set GLM_DIR=%EXTERNAL_DIR%\glm

if not exist "%EXTERNAL_DIR%" (
    mkdir "%EXTERNAL_DIR%"
)

if not exist "%GLM_DIR%" (
    echo Downloading GLM...
    powershell -Command "Invoke-WebRequest -Uri '%GLM_URL%' -OutFile 'glm.zip'"
    echo Extracting GLM...
    powershell -Command "Expand-Archive -Path 'glm.zip' -DestinationPath '%EXTERNAL_DIR%'"
    del glm.zip
    echo GLM installed successfully!
) else (
    echo GLM already exists at %GLM_DIR%
)

pause
