@echo off
setlocal
cd /d "%~dp0"

if exist "build\Release\ASIS_Engine.exe" (
    echo Iniciando ASIS Engine...
    cd build\Release
    ASIS_Engine.exe
) else (
    echo [ERROR] No se encuentra el ejecutable. Ejecuta primero build.bat.
    pause
)
