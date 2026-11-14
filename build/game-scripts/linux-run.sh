#!/bin/sh
# @SCRIPT@ -- start @TITLE@ (built from source) with Wine.
#
#   ./@SCRIPT@ [window options] [engine args...]   the game, straight away
#   ./@SCRIPT@ --launcher           its launcher, @LAUNCHER@, instead
#   ./@SCRIPT@ --help               this text
#
# Aliens versus Predator 2 1.0.9.6 is in AVP2/ (linux-run-avp2.sh); Primal
# Hunt is in PrimalHunt/ (linux-run-ph.sh), and plays on AvP2's archives
# beside it.
# The game starts as its launcher would start it: the engine, lithtech.exe,
# with the command file the launcher writes (avp2cmds.txt: the rez stack,
# with custom/ -- the game DLLs built from source -- last), plus your
# arguments, e.g.
#   ./@SCRIPT@ +DisableSound 1       no sound
# (written to run-cmds.txt in the game's directory: the engine takes its
# arguments from the command file).
#
# The window. The game starts as it was last left: the window mode and the
# resolution are its settings (autoexec.cfg), which the options below set
# for this run and every later one:
#   --fullscreen     full screen: the display changes to the resolution
#   --windowed       a window with a title bar, the resolution's size
#   --borderless     (the modern build) a borderless window covering the
#                    desktop at the desktop's own resolution
#   --res WxH        the resolution, e.g. --res 1280x720
# In the modern build Options > Display > "Window mode" chooses the same,
# and a first run starts in the borderless window at the desktop's
# resolution. The release build has no such option: its first run starts
# in a 640x480 window (Display sets the resolution), as its renderer's full
# screen fails under many Wine setups -- it changes the display's own mode,
# which X servers without that mode refuse (the modern build then falls
# back to the borderless window).
#
# --launcher starts @LAUNCHER@ (built from src/launcher/), whose Display and
# Advanced pages set the renderer, the resolution and the engine's options;
# it rewrites avp2cmds.txt. (In the modern build the launchers are
# AVP2Launcher.exe and PrimalHuntLauncher.exe, and AVP2.exe and
# PrimalHunt.exe start the game directly, as Launch would, with no registry
# needed.)
#
# Environment:
#   WINEPREFIX   the Wine prefix to use (default: $HOME/.wine-avp2, created
#                on first run, so your own prefix is left alone)
#   WINE         the wine binary (default: wine)
#   WINEDEBUG    Wine's log channels (default: fixme-all)
#
# The launchers expect what the retail setups left behind, and this script
# writes it into its Wine prefix: the registry values under
# HKLM\Software\Monolith Productions\Aliens vs. Predator 2\1.0 (InstallDir:
# AVP2/; "Update Command Line": the 1.0.9.6 patch's archive) and
# HKLM\Software\Third Law Interactive\Aliens versus Predator 2: Primal
# Hunt\1.0 (InstallDir: PrimalHunt/). Primal Hunt's launcher will not start
# without AvP2's InstallDir. On the first run each game's autoexec.cfg --
# its settings, rewritten on every exit -- starts as the shipped one.
#
# Sound: the games use the Miles Sound System (mss32.dll) through
# DirectSound, so they need Wine's audio working on this machine (winecfg >
# Audio).
set -e
cd "$(dirname "$0")"
HERE="$(pwd)"
LAUNCHER=
case "${1:-}" in
--help|-h) sed -n '2,/^set -e$/{/^set -e$/d;p}' "./${0##*/}" | sed 's/^# \{0,1\}//'; exit 0 ;;
--launcher) LAUNCHER=1; shift ;;
esac
GAME=@GAME@
MODERN=@MODERN@
# The window options (the header): engine arguments, ++ so that the engine
# saves them in autoexec.cfg
MODE=
while [ $# -gt 0 ]; do
    case "$1" in
    --fullscreen) MODE="$MODE ++windowed 0 ++WindowMode 0" ;;
    --windowed)   MODE="$MODE ++windowed 1 ++WindowMode 1" ;;
    --borderless)
        [ "$MODERN" = 1 ] || { echo "@SCRIPT@: --borderless is the modern build's (this one has --windowed and --fullscreen)" >&2; exit 2; }
        MODE="$MODE ++windowed 1 ++WindowMode 2" ;;
    --res)
        case "${2:-}" in
        [1-9]*x[1-9]*) ;;
        *) echo "@SCRIPT@: --res takes WIDTHxHEIGHT, e.g. --res 1280x720" >&2; exit 2 ;;
        esac
        W=${2%x*}; H=${2#*x}
        MODE="$MODE ++screenwidth $W ++screenheight $H ++bitdepth 32 ++GameScreenWidth $W ++GameScreenHeight $H ++GameBitDepth 32"
        shift ;;
    *) break ;;
    esac
    shift
done
[ -f "$GAME/autoexec.cfg" ] || FIRST=1
# The release build's first run: a 640x480 window (the header). The modern
# build's chooses for itself (the borderless window).
if [ -n "${FIRST:-}" ] && [ "$MODERN" != 1 ]; then
    case "$MODE" in *++windowed*) ;; *) MODE=" ++windowed 1$MODE" ;; esac
    case "$MODE" in *++screenwidth*) ;; *) MODE="$MODE ++screenwidth 640 ++screenheight 480 ++bitdepth 32" ;; esac
fi
WINEPREFIX="${WINEPREFIX:-$HOME/.wine-avp2}"
WINEDEBUG="${WINEDEBUG:-fixme-all}"
export WINEPREFIX WINEDEBUG
WINE="${WINE:-wine}"
command -v "$WINE" >/dev/null 2>&1 || { echo "@SCRIPT@: '$WINE' not found -- install Wine (32-bit support needed)" >&2; exit 1; }
for g in AVP2 PrimalHunt; do
    [ -f "$g/AVP2L.REZ" ] || { echo "@SCRIPT@: the games' data is not here yet -- run ./linux-install.sh first" >&2; exit 1; }
    [ -f "$g/autoexec.cfg" ] || cp "$g/autoexec.cfg.retail" "$g/autoexec.cfg"
    mkdir -p "$g/Save" "$g/Profiles"
    [ -f "$g/Profiles/Default.prf" ] || cp "$g/Default.prf.retail" "$g/Profiles/Default.prf"
done

[ -d "$WINEPREFIX/dosdevices" ] || "$WINE" wineboot -i >/dev/null 2>&1 || true
# Both registry views: in a 64-bit prefix `wine reg` writes the 64-bit one,
# while the launchers, 32-bit programs, read HKLM\Software through the
# 32-bit one (Wow6432Node). (A 32-bit prefix has one view; the /reg:64 write
# then fails, harmlessly.)
reg() { "$WINE" reg add "$@" /reg:32 /f >/dev/null 2>&1
        "$WINE" reg add "$@" /reg:64 /f >/dev/null 2>&1 || true; }
AVP2KEY='HKLM\Software\Monolith Productions\Aliens vs. Predator 2\1.0'
PHKEY='HKLM\Software\Third Law Interactive\Aliens versus Predator 2: Primal Hunt\1.0'
reg "$AVP2KEY" /v InstallDir /t REG_SZ /d "$("$WINE" winepath -w "$HERE/AVP2" 2>/dev/null)"
reg "$AVP2KEY" /v 'Update Command Line' /t REG_SZ /d '-rez AVP2P1.REZ'
reg "$PHKEY" /v InstallDir /t REG_SZ /d "$("$WINE" winepath -w "$HERE/PrimalHunt" 2>/dev/null)"

cd "$GAME"
[ -z "$LAUNCHER" ] || exec "$WINE" @LAUNCHER@ "$@"
{ tr -d '\r\n' < avp2cmds.txt; printf '%s' "$MODE"
  [ $# -eq 0 ] || printf ' %s' "$*"; printf '\r\n'; } > run-cmds.txt
exec "$WINE" lithtech.exe -cmdfile run-cmds.txt
