@echo off
chcp 65001 >nul 2>&1
echo.
echo   gen-clangd - Keil MDK IntelliSense Config Generator
echo   ====================================================
echo.

:: 转到项目根目录（tools/gen-clangd 的上两级）
cd /d "%~dp0..\.."

pwsh -ExecutionPolicy Bypass -File "tools\gen-clangd\gen-clangd.ps1"

echo.
pause
