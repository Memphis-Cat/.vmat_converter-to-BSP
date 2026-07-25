@echo off
setlocal

set "PROJECT_DIR=D:\VMAT"
set "SDK_URL=https://github.com/ValveSoftware/source-sdk-2013.git"
set "SDK_PATH=external/source-sdk-2013"

cd /d "%PROJECT_DIR%"
if errorlevel 1 (
    echo ERROR: Could not open "%PROJECT_DIR%".
    pause
    exit /b 1
)

git rev-parse --is-inside-work-tree >nul 2>&1
if errorlevel 1 (
    echo ERROR: "%PROJECT_DIR%" is not a Git repository.
    echo Run 01_setup_project.bat first.
    pause
    exit /b 1
)

git config --file .gitmodules --get-regexp "submodule\..*\.path" 2>nul | findstr /i /c:"%SDK_PATH%" >nul
if not errorlevel 1 (
    echo Source SDK 2013 is already registered as a submodule.
    git submodule update --init --recursive
    pause
    exit /b 0
)

if exist "external\source-sdk-2013" (
    echo ERROR: "external\source-sdk-2013" already exists but is not registered as a submodule.
    echo Rename or remove that folder, then run this file again.
    pause
    exit /b 1
)

if not exist "external" mkdir "external"

echo Adding Source SDK 2013 as a Git submodule...
git submodule add "%SDK_URL%" "%SDK_PATH%"
if errorlevel 1 goto :error

echo.
echo SDK added. Run 02_upload_to_github.bat to upload the submodule reference.
pause
exit /b 0

:error
echo.
echo ERROR: Could not add Source SDK 2013.
pause
exit /b 1
