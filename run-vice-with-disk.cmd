@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0"
set "BIN=%ROOT%bin"
set "KAPER=%BIN%\kaper.prg"
set "HJAELP=%BIN%\hjaelp.prg"
set "D64=%BIN%\kaper.d64"
set "LOG=%BIN%\d64-build.log"
set "VICE=C:\apps\GTK3VICE-3.10-win64\bin\x64sc.exe"
set "C1541=C:\apps\GTK3VICE-3.10-win64\bin\c1541.exe"

set "ERR="
if not exist "%C1541%" set "ERR=C1541 not found at %C1541%"
if not exist "%VICE%" set "ERR=VICE not found at %VICE%"
if not exist "%KAPER%" set "ERR=Missing program file %KAPER%"
if not exist "%HJAELP%" set "ERR=Missing program file %HJAELP%"
if defined ERR goto :fail

rem Only format when the image is missing: an existing image may hold the
rem persisted REC.DAT high-score file, which a reformat would wipe.
if not exist "%D64%" (
  "%C1541%" -format "kaptajn kaper,01" d64 "%D64%" > "%LOG%" 2>&1
  if errorlevel 1 (
    set "ERR=Failed to create disk image. See %LOG%"
    goto :fail
  )
)

rem Scratch first so -write cannot create duplicate entries. On a freshly
rem formatted image there is nothing to delete, so ignore the exit code here.
"%C1541%" -attach "%D64%" -delete kaper -delete hjaelp >> "%LOG%" 2>&1

"%C1541%" -attach "%D64%" -write "%KAPER%" kaper -write "%HJAELP%" hjaelp >> "%LOG%" 2>&1
if errorlevel 1 (
  set "ERR=Failed to write program files to disk image. See %LOG%"
  goto :fail
)

rem Autostart the disk entry matching the PRG built from the active source file.
set "AUTOSTART=kaper"
set "ARGS="
for %%A in (%*) do (
  if /i "%%~xA"==".prg" (
    set "AUTOSTART=%%~nA"
  ) else (
    set "ARGS=!ARGS! "%%~A""
  )
)

"%VICE%" -drive8type 1541 -8 "%D64%" -autostartprgmode 1 -autostart "%D64%:!AUTOSTART!"%ARGS%
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
