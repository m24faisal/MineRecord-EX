@echo off
set DEST=%1

REM Resolve the project root (where .pro lives)
for %%I in ("%~dp0..") do set "PROJECT_DIR=%%~fI"

REM Go up one more level to MineRecordEX/
for %%I in ("%PROJECT_DIR%") do set "ROOT_DIR=%%~dpI"

set SRC=%ROOT_DIR%backend

echo [COPY] Source: %SRC%
echo [COPY] Destination: %DEST%

if not exist "%SRC%" (
    echo ERROR: Backend folder not found at %SRC%
    exit /b 1
)

xcopy /E /I /Y "%SRC%" "%DEST%\backend"
if errorlevel 1 (
    echo ERROR: Failed to copy backend.
    exit /b 1
)

echo SUCCESS: Backend copied to %DEST%\backend