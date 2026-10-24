@echo off
setlocal

where mingw32-make >nul 2>&1
if %ERRORLEVEL% equ 0 (
    mingw32-make %*
    exit /b %ERRORLEVEL%
)

if exist "C:\mingw64\bin\mingw32-make.exe" (
    set "PATH=C:\mingw64\bin;%PATH%"
    "C:\mingw64\bin\mingw32-make.exe" %*
    exit /b %ERRORLEVEL%
)

:: Fallback to build.bat
call "%~dp0build.bat"
exit /b %ERRORLEVEL%
