@echo off
setlocal

set "PROJECT_DIR=D:\VMAT"

if not exist "%PROJECT_DIR%" mkdir "%PROJECT_DIR%"
if not exist "%PROJECT_DIR%\src" (
    mkdir "%PROJECT_DIR%\src"
    echo Created "%PROJECT_DIR%\src".
) else (
    echo "%PROJECT_DIR%\src" already exists.
)

pause
