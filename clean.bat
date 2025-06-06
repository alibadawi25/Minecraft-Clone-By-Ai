@echo off
echo Cleaning build directories...

if exist build\ (
    echo Removing build\
    rmdir /s /q build
)

if exist build_mingw\ (
    echo Removing build_mingw\
    rmdir /s /q build_mingw
)

if exist build_debug\ (
    echo Removing build_debug\
    rmdir /s /q build_debug
)

if exist build_vs\ (
    echo Removing build_vs\
    rmdir /s /q build_vs
)

echo.
echo All build directories cleaned!
pause
