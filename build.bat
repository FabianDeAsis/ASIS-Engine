@echo off
setlocal
echo ===================================================
echo   ASIS Engine - Compilador Automatico (CMake + MSVC)
echo ===================================================

set CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
set TOOLCHAIN=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake

if not exist "%CMAKE_EXE%" (
    echo [ERROR] No se encontro CMake en la instalacion de Visual Studio 2026.
    pause
    exit /b 1
)

cd /d "%~dp0"

:: Cerrar cualquier instancia activa de ASIS_Engine para que el linker pueda sobrescribir el .exe
taskkill /f /im ASIS_Engine.exe >nul 2>&1

echo [1/2] Configurando proyecto con CMake y vcpkg...
"%CMAKE_EXE%" -B build -S . -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" -DCMAKE_GENERATOR_PLATFORM=x64
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Fallo la configuracion de CMake.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [2/2] Compilando en modo Release...
"%CMAKE_EXE%" --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Fallo la compilacion.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ===================================================
echo   Compilacion exitosa: build\Release\ASIS_Engine.exe
echo ===================================================
pause
