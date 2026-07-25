@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
msbuild Nyx.sln /p:Configuration=Release /p:Platform=x64 /m
echo.
if %errorlevel% equ 0 (
    echo [+] Build succeeded - Nyx External.exe
) else (
    echo [!] Build failed
)
pause
