@echo off
setlocal
echo ========================================================
echo   Iniciando Consola de IA de ASIS Engine...
echo ========================================================
cd /d "%~dp0src\ai_module"
python ai_console.py
pause
