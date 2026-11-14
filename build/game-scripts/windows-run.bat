@echo off
rem windows-run.bat -- start Aliens versus Predator 2 or Primal Hunt (built
rem from source) straight from its command file, as its launcher would.
rem
rem   windows-run.bat [avp2|ph] [window options] [engine args...]
rem
rem avp2 (the default) is AvP2 1.0.9.6, in AVP2\; ph is Primal Hunt, in
rem PrimalHunt\, which plays on AvP2's archives beside it. The engine,
rem lithtech.exe, starts with the game's avp2cmds.txt (the rez stack, with
rem custom\ -- the game DLLs built from source -- last) and your arguments,
rem written to run-cmds.txt. The launchers (AVP2\AVP2.exe,
rem PrimalHunt\PrimalHunt.exe) are the other way in; they need the registry
rem values windows-install.bat writes. In the modern build AVP2\AVP2.exe and
rem PrimalHunt\PrimalHunt.exe start the game directly, as the launcher's
rem Launch would, and need no registry; its launchers are
rem AVP2\AVP2Launcher.exe and PrimalHunt\PrimalHuntLauncher.exe.
rem
rem The game starts as it was last left (autoexec.cfg: in the modern build
rem Options, Display, "Window mode"; a first run there is a borderless
rem window at the desktop's resolution, in the release build full screen).
rem The window options set it for this run and the later ones:
rem   --fullscreen   full screen at the resolution
rem   --windowed     a window with a title bar, the resolution's size
rem   --borderless   (the modern build) a borderless window covering the
rem                  desktop at its own resolution
rem   --res WxH      the resolution, e.g. --res 1280x720
setlocal
set "GAME=AVP2"
if /i "%~1"=="avp2" shift
if /i "%~1"=="ph" (set "GAME=PrimalHunt" & shift)
cd /d "%~dp0%GAME%" || exit /b 1
if not exist autoexec.cfg copy autoexec.cfg.retail autoexec.cfg >nul
if not exist Profiles\Default.prf (mkdir Profiles 2>nul & copy Default.prf.retail Profiles\Default.prf >nul)
if not exist Save mkdir Save
set "ARGS="
:opts
if /i "%~1"=="--fullscreen" (set "ARGS=%ARGS% ++windowed 0 ++WindowMode 0" & shift & goto opts)
if /i "%~1"=="--windowed" (set "ARGS=%ARGS% ++windowed 1 ++WindowMode 1" & shift & goto opts)
if /i "%~1"=="--borderless" (set "ARGS=%ARGS% ++windowed 1 ++WindowMode 2" & shift & goto opts)
if /i not "%~1"=="--res" goto args
for /f "tokens=1,2 delims=x" %%a in ("%~2") do set "ARGS=%ARGS% ++screenwidth %%a ++screenheight %%b ++bitdepth 32 ++GameScreenWidth %%a ++GameScreenHeight %%b ++GameBitDepth 32"
shift
shift
goto opts
:args
if "%~1"=="" goto run
set "ARGS=%ARGS% %1"
shift
goto args
:run
set /p CMDS=<avp2cmds.txt
>run-cmds.txt echo %CMDS%%ARGS%
start "" lithtech.exe -cmdfile run-cmds.txt
