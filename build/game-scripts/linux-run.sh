#!/bin/sh
# linux-run.sh -- start Aliens versus Predator 2 or Primal Hunt (built from
# source) with Wine.
#
#   ./linux-run.sh [avp2|ph] [engine args...]     the game, straight away
#   ./linux-run.sh --launcher [avp2|ph]           its launcher instead
#   ./linux-run.sh --help                         this text
#
# avp2 (the default) is Aliens versus Predator 2 1.0.9.6, in AVP2/; ph is
# Primal Hunt, in PrimalHunt/, which plays on AvP2's archives beside it.
# The game starts as its launcher would start it: the engine, lithtech.exe,
# with the command file the launcher writes (avp2cmds.txt: the rez stack,
# with custom/ -- the game DLLs built from source -- last), plus a 640x480
# window, plus your arguments, e.g.
#   ./linux-run.sh avp2 ++windowed 0       full screen
#   ./linux-run.sh ph +DisableSound 1      Primal Hunt, no sound
# (written to run-cmds.txt in the game's directory: the engine takes its
# arguments from the command file). The window: the renderer's full screen
# fails under many Wine setups.
#
# --launcher starts AVP2.exe or PrimalHunt.exe, the launchers (built from
# src/launcher/), whose Display and Advanced pages set the renderer, the
# resolution and the engine's options; they rewrite avp2cmds.txt.
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
GAME=AVP2
case "${1:-}" in
avp2|AVP2) shift ;;
ph|PH|primalhunt|PrimalHunt) GAME=PrimalHunt; shift ;;
esac
WINEPREFIX="${WINEPREFIX:-$HOME/.wine-avp2}"
WINEDEBUG="${WINEDEBUG:-fixme-all}"
export WINEPREFIX WINEDEBUG
WINE="${WINE:-wine}"
command -v "$WINE" >/dev/null 2>&1 || { echo "linux-run.sh: '$WINE' not found -- install Wine (32-bit support needed)" >&2; exit 1; }
for g in AVP2 PrimalHunt; do
    [ -f "$g/AVP2L.REZ" ] || { echo "linux-run.sh: the games' data is not here yet -- run ./linux-install.sh first" >&2; exit 1; }
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
if [ -n "$LAUNCHER" ]; then
    if [ "$GAME" = AVP2 ]; then exec "$WINE" AVP2.exe "$@"; fi
    exec "$WINE" PrimalHunt.exe "$@"
fi
{ tr -d '\r\n' < avp2cmds.txt; printf ' ++screenwidth 640 ++screenheight 480 ++bitdepth 32 ++windowed 1'
  [ $# -eq 0 ] || printf ' %s' "$*"; printf '\r\n'; } > run-cmds.txt
exec "$WINE" lithtech.exe -cmdfile run-cmds.txt
