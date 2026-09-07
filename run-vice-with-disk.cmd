@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0"
set "BIN=%ROOT%bin"
set "KAPER=%BIN%\kaper.prg"
set "D64=%BIN%\kaper.d64"
set "LOG=%BIN%\d64-build.log"
set "VICE=C:\apps\GTK3VICE-3.10-win64\bin\x64sc.exe"
set "C1541=C:\apps\GTK3VICE-3.10-win64\bin\c1541.exe"

set "ERR="
if not exist "%C1541%" set "ERR=C1541 not found at %C1541%"
if not exist "%VICE%" set "ERR=VICE not found at %VICE%"
if not exist "%KAPER%" set "ERR=Missing program file %KAPER%"
if defined ERR goto :fail

"%C1541%" -format "kaptajn kaper,01" d64 "%D64%" -attach "%D64%" -write "%KAPER%" kaper > "%LOG%" 2>&1
if errorlevel 1 (
  set "ERR=Failed to create disk image. See %LOG%"
  goto :fail
)

rem Pass through all args from the extension except the .prg autostart file
set "ARGS="
for %%A in (%*) do (
  if /i not "%%~xA"==".prg" set "ARGS=!ARGS! "%%~A""
)

"%VICE%" -drive8type 1541 -8 "%D64%" -autostartprgmode 1 -autostart "%D64%"%ARGS%
if errorlevel 1 (
  set "ERR=VICE failed to start. See messages above."
  goto :fail
)
exit /b 0

:fail
echo.
echo ERROR: %ERR%
echo.
pause
exit /b 1
