@echo off
set DEST=%1
xcopy /E /I /Y "%~dp0backend" "%DEST%\backend"
echo Backend files copied to %DEST%\backend