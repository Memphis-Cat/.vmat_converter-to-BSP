@echo off
setlocal

set "PROJECT_DIR=D:\VMAT"
set "REPO_URL=https://github.com/Memphis-Cat/.vmat_converter-to-BSP.git"

where git >nul 2>&1
if errorlevel 1 (
    echo ERROR: Git is not installed or is not available in PATH.
    echo Install Git for Windows, then run this file again.
    pause
    exit /b 1
)

if not exist "%PROJECT_DIR%" (
    mkdir "%PROJECT_DIR%"
    if errorlevel 1 (
        echo ERROR: Could not create "%PROJECT_DIR%".
        pause
        exit /b 1
    )
)

cd /d "%PROJECT_DIR%"
if errorlevel 1 (
    echo ERROR: Could not open "%PROJECT_DIR%".
    pause
    exit /b 1
)

if not exist ".git" (
    git init
    if errorlevel 1 goto :error

    git branch -M main
    if errorlevel 1 goto :error
)

git remote get-url origin >nul 2>&1
if errorlevel 1 (
    git remote add origin "%REPO_URL%"
    if errorlevel 1 goto :error
) else (
    git remote set-url origin "%REPO_URL%"
    if errorlevel 1 goto :error
)

if not exist "src" mkdir "src"
if not exist "external" mkdir "external"

echo.
echo Project folder configured:
echo   %PROJECT_DIR%
echo.
echo GitHub remote:
git remote -v
echo.
echo Next:
echo   1. Put your C++ files in D:\VMAT\src
echo   2. Run 05_add_source_sdk_2013.bat
echo   3. Run 02_upload_to_github.bat
pause
exit /b 0

:error
echo.
echo ERROR: Project setup failed.
pause
exit /b 1
