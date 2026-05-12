@echo off
:: bootstrap.bat -- build jenuroix_compiler.exe once.
:: After that, run jenuroix_compiler.exe directly.
:: Place this file next to jenuroix_compiler.cpp (project root).

setlocal
set "ROOT=%~dp0"
set "SRC=%ROOT%jenuroix_compiler.cpp"
set "OUT=%ROOT%jenuroix_compiler.exe"

if not exist "%SRC%" (
    echo [ERROR] jenuroix_compiler.cpp not found next to this script.
    echo         Expected: %SRC%
    pause & exit /b 1
)

echo.
echo  Jenuroix Compiler - bootstrap
echo  Compiling jenuroix_compiler.cpp -^> jenuroix_compiler.exe
echo.

:: -- Try 1: MSVC cl.exe --------------------------------------------------------
where cl >nul 2>nul
if %errorlevel% equ 0 (
    echo [OK] Using MSVC cl.exe
    cl /EHsc /std:c++20 /O2 /W3 "%SRC%" /Fe:"%OUT%" shell32.lib
    if %errorlevel% equ 0 goto done
    echo [ERR] MSVC build failed, trying g++...
)

:: -- Try 2: g++ (MinGW / MSYS2) -----------------------------------------------
where g++ >nul 2>nul
if %errorlevel% equ 0 (
    echo [OK] Using g++
    g++ -std=c++20 -O2 -Wall "%SRC%" -o "%OUT%" -lshell32
    if %errorlevel% equ 0 goto done
    echo [ERR] g++ build failed, trying clang++...
)

:: -- Try 3: clang++ -----------------------------------------------------------
where clang++ >nul 2>nul
if %errorlevel% equ 0 (
    echo [OK] Using clang++
    clang++ -std=c++20 -O2 "%SRC%" -o "%OUT%" -lshell32
    if %errorlevel% equ 0 goto done
    echo [ERR] clang++ build failed.
)

echo.
echo [ERROR] No C++ compiler found.
echo  Install one of:
echo    Visual Studio 2022  (Desktop development with C++)
echo    MSYS2 + MinGW-w64   https://www.msys2.org/
echo    LLVM clang++        https://releases.llvm.org/
echo.
pause
exit /b 1

:done
echo.
echo [OK] Done!  jenuroix_compiler.exe is ready.
echo.
echo Usage:
echo   jenuroix_compiler.exe            (interactive menu)
echo   jenuroix_compiler.exe game       (build + launch)
echo   jenuroix_compiler.exe editor     (build editor)
echo   jenuroix_compiler.exe clean      (delete build/)
echo   jenuroix_compiler.exe vs         (open in Visual Studio)
echo.
pause
