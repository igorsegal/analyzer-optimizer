@echo off
chcp 65001 >nul
setlocal
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

echo.
echo === SPARTAK :: installer launcher ===
echo Root: %ROOT%
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install_spartak.ps1" -Root "%ROOT%"

echo.
echo Done. Press any key to exit.
pause >nul
endlocal