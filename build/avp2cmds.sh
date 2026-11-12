#!/usr/bin/env bash
# A game directory's avp2cmds.txt, as a fresh build writes it:
#
#   build/avp2cmds.sh avp2     AVP2/avp2cmds.txt: Aliens versus Predator 2
#   build/avp2cmds.sh ph       PrimalHunt/avp2cmds.txt: Primal Hunt
#
# Both retail launchers (AVP2.exe, PrimalHunt.exe) write the engine's
# command line to avp2cmds.txt and run "lithtech.exe -cmdfile avp2cmds.txt"
# (src/launcher/LauncherDlg.cpp, LaunchGame); linux-run.sh and
# build/playtest.sh start the engine the same way with this one. Later
# archives override earlier ones; custom/ holds the game DLLs built from
# source, and no archive after it holds one (the retail ones are zeroed:
# build/all.sh, step 1).
#
#   AvP2: its launcher's order -- AVP2.REZ, the sound and the campaigns'
#     archives, MULTI.REZ, AVP2DLL.REZ, AVP2L.REZ, custom -- and then the
#     1.0.9.6 patch's "Update Command Line", -rez AVP2P1.REZ. (The launcher
#     also names avp2p.rez and avp2p2.rez, which no release installs.)
#   Primal Hunt: AvP2's archives, from AvP2's directory beside it (the
#     retail launcher finds them by AvP2's InstallDir in the registry), then
#     Primal Hunt's own: its patch data (AVP2P.REZ, AVP2P1.REZ), its worlds
#     and data (AVP2X.REZ), MULTI.REZ, AVP2DLL.REZ, AVP2L.REZ, DIALOGUE.REZ,
#     custom. What the retail launcher does here lies in the pages its
#     protection wrapper keeps encrypted (docs/PLAN.md, package 8); this is
#     its string pool's order (" -rez multi.rez", "dll.rez", "l.rez",
#     " -rez dialogue.rez", PatchRez, " -rez custom") after AvP2's.
#
# +DisableMovies skips the intro logos (RAD's Bink under Wine: as
# nolfsrc's). The display is the harness's (linux-run.sh, playtest.sh).
set -e
case "${1:-}" in
avp2)
    printf -- '-rez AVP2.REZ -rez SOUNDS.REZ -rez ALIEN.REZ -rez MARINE.REZ -rez PREDATOR.REZ -rez MULTI.REZ -rez AVP2DLL.REZ -rez AVP2L.REZ -rez custom -rez AVP2P1.REZ +DisableMovies 1\r\n' ;;
ph)
    A='..\AVP2'
    printf -- '-rez %s\\AVP2.REZ -rez %s\\SOUNDS.REZ -rez %s\\ALIEN.REZ -rez %s\\MARINE.REZ -rez %s\\PREDATOR.REZ -rez %s\\MULTI.REZ -rez AVP2P.REZ -rez AVP2P1.REZ -rez AVP2X.REZ -rez MULTI.REZ -rez AVP2DLL.REZ -rez AVP2L.REZ -rez DIALOGUE.REZ -rez custom +DisableMovies 1\r\n' \
        "$A" "$A" "$A" "$A" "$A" "$A" ;;
*)  echo "usage: build/avp2cmds.sh avp2|ph" >&2; exit 2 ;;
esac
