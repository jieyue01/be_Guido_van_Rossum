@echo off
echo === CPY Build ===

set SRCS=src\main.c src\lexer.c src\parser.c src\ast.c src\interpreter.c src\value.c src\env.c
set OUT=cpy.exe
set CFLAGS=-Wall -std=c11 -O2

where gcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Using GCC
    gcc %CFLAGS% %SRCS% -o %OUT%
    goto :done
)

where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Using MSVC
    cl /nologo /W3 /O2 /Fe:%OUT% %SRCS%
    goto :done
)

echo ERROR: No C compiler found.
echo Install MinGW-w64: https://github.com/niXman/mingw-builds-binaries/releases
echo Or install Visual Studio Build Tools.
pause
exit /b 1

:done
if %ERRORLEVEL% EQU 0 (
    echo Build success: %OUT%
) else (
    echo Build failed.
)
if "%~1"=="-r" (
    echo.
    %OUT%
)
