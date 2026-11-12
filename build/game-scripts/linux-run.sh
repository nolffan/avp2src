#!/bin/sh
# @SCRIPT@ -- start @TITLE@ (built from source) with Wine.
#
#   ./@SCRIPT@ [engine args...]     the game, straight away
#   ./@SCRIPT@ --launcher           its launcher, @LAUNCHER@, instead
#   ./@SCRIPT@ --help               this text
#
# Aliens versus Predator 2 1.0.9.6 is in AVP2/ (linux-run-avp2.sh); Primal
# Hunt is in PrimalHunt/ (linux-run-ph.sh), and plays on AvP2's archives
# beside it.
# The game starts as its launcher would start it: the engine, lithtech.exe,
# with the command file the launcher writes (avp2cmds.txt: the rez stack,
# with custom/ -- the game DLLs built from source -- last), plus a 640x480
# window, plus your arguments, e.g.
#   ./@SCRIPT@ ++windowed 0          full screen
#   ./@SCRIPT@ +DisableSound 1       no sound
# (written to run-cmds.txt in the game's directory: the engine takes its
# arguments from the command file). The window: the renderer's full screen
# fails under many Wine setups.
#
# --launcher starts @LAUNCHER@ (built from src/launcher/), whose Display and
# Advanced pages set the renderer, the resolution and the engine's options;
# it rewrites avp2cmds.txt.
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
{ tr -d '\r\n' < avp2cmds.txt; printf ' ++screenwidth 640 ++screenheight 480 ++bitdepth 32 ++windowed 1'
  [ $# -eq 0 ] || printf ' %s' "$*"; printf '\r\n'; } > run-cmds.txt
exec "$WINE" lithtech.exe -cmdfile run-cmds.txt
