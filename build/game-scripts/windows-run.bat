@echo off
rem windows-run.bat -- start Aliens versus Predator 2 or Primal Hunt (built
rem from source) straight from its command file, as its launcher would.
rem
rem   windows-run.bat [avp2|ph] [engine args...]
rem
rem avp2 (the default) is AvP2 1.0.9.6, in AVP2\; ph is Primal Hunt, in
rem PrimalHunt\, which plays on AvP2's archives beside it. The engine,
rem lithtech.exe, starts with the game's avp2cmds.txt (the rez stack, with
rem custom\ -- the game DLLs built from source -- last) and your arguments,
rem written to run-cmds.txt. The launchers (AVP2\AVP2.exe,
rem PrimalHunt\PrimalHunt.exe) are the other way in; they need the registry
rem values windows-install.bat writes.
setlocal
set "GAME=AVP2"
if /i "%~1"=="avp2" shift
if /i "%~1"=="ph" (set "GAME=PrimalHunt" & shift)
cd /d "%~dp0%GAME%" || exit /b 1
if not exist autoexec.cfg copy autoexec.cfg.retail autoexec.cfg >nul
if not exist Profiles\Default.prf (mkdir Profiles 2>nul & copy Default.prf.retail Profiles\Default.prf >nul)
if not exist Save mkdir Save
set "ARGS="
:args
if "%~1"=="" goto run
set "ARGS=%ARGS% %1"
shift
goto args
:run
set /p CMDS=<avp2cmds.txt
>run-cmds.txt echo %CMDS%%ARGS%
start "" lithtech.exe -cmdfile run-cmds.txt
