#!/bin/sh
# @SCRIPT@ -- run a @TITLE@ dedicated server (@EXE@, built from
# source) with Wine.
#
#   ./@SCRIPT@ [options] [@EXE@ args...]
#
# Options:
#   --config NAME   start with the configuration ServerData/NAME.txt (the
#                   default: server). If the file does not exist, the script
#                   writes it with default settings first (below).
#   --wizard        show the setup dialog of the server: it lists the
#                   configurations in ServerData/ and starts the one that
#                   you select (when there is only one, it starts it
#                   immediately). The game's Multiplayer > Host menu saves
#                   the configurations there.
#   --headless      run without a display: under xvfb-run, a virtual X
#                   server (the default when $DISPLAY is not set)
#   --help          this text
# The script gives all other arguments to @EXE@:
#   -port N         the UDP port (overrides the PortID of the configuration)
#   -name TEXT      the name of the server in the server lists
#   -MasterServer HOST[:PORT]  (the modern build) the master server that
#                   lists the server (default master.openspy.net; none: no
#                   list)
# For example:
#   ./@SCRIPT@ --config server -name "My server" -port 27888
#
# The default configuration (ServerData/server.txt, when the script writes
# it): deathmatch, 8 players, port 27888, the deathmatch worlds of the
# game, and a random server-control password (SCPassword: a player who types
# it can control the server). The script shows the password. In the modern
# build the server is an Internet server (LANConnection = 0: it is on the
# Internet list and on the LAN list). In the legacy build it is LAN only
# (LANConnection = 1): a retail server that is not LAN only registers with
# WON, and without SierraUp.cfg it crashes when the world starts
# (docs/RETAIL_BUGS.md). Edit the file with a text editor, or make a
# different configuration with the game's Host menu.
#
# Environment:
#   WINEPREFIX   the Wine prefix (default: $HOME/.wine-avp2, the prefix of
#                linux-run-avp2.sh and linux-run-ph.sh. When headless, the
#                default is a prefix for the server of each game,
#                $HOME/.wine-avp2-server-@GAME@: the Wine desktop of a prefix
#                is on one display. A game in a prefix whose desktop is on
#                the virtual display of a server cannot use the mouse, and a
#                second server cannot start there.)
#   WINE         the wine binary (default: wine)
#   WINESERVER   the wineserver binary (default: wineserver)
#   WINEDEBUG    the log channels of Wine (default: -all)
#
# The server runs in @GAME@/. It loads the archives of the game and the
# game DLLs built from source (custom/), and it keeps its configurations in
# ServerData/. The modern build also writes its console to
# server-console.log. The server answers on UDP port 27888 (or the PortID of
# the configuration, or -port). Open this port in the firewall for players
# on other machines. For the Internet list, also forward this UDP port on
# your router to this machine. The server runs until you close its window
# or stop the process.
set -e
cd "$(dirname "$0")"
HERE="$(pwd)"
[ ! -f NOT-PLAYABLE.txt ] || { echo "@SCRIPT@: this directory was built without the retail games and does not play (NOT-PLAYABLE.txt): use its public archive" >&2; exit 1; }
GAME=@GAME@
EXE=@EXE@
MODERN=@MODERN@
LAN=1
[ "$MODERN" = 1 ] && LAN=0

CONFIG=
WIZARD=
HEADLESS=
case "${DISPLAY:-}" in "") HEADLESS=1 ;; esac
while [ $# -gt 0 ]; do
    case "$1" in
    --config)
        [ -n "${2:-}" ] || { echo "@SCRIPT@: --config takes a name, e.g. --config server" >&2; exit 2; }
        CONFIG="${2%.txt}"; shift ;;
    --wizard)   WIZARD=1 ;;
    --headless) HEADLESS=1 ;;
    --help|-h)  sed -n '2,/^set -e$/{/^set -e$/d;p}' "./${0##*/}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *)          break ;;
    esac
    shift
done
if [ -n "$WIZARD" ] && [ -n "$CONFIG" ]; then
    echo "@SCRIPT@: use --wizard or --config, not the two" >&2; exit 2
fi
if [ -n "$WIZARD" ] && [ -z "${DISPLAY:-}" ]; then
    echo "@SCRIPT@: the setup dialog needs a display -- use --config NAME" >&2; exit 2
fi
[ -n "$WIZARD" ] || CONFIG="${CONFIG:-server}"

for g in AVP2 $GAME; do
    [ -f "$g/AVP2L.REZ" ] || { echo "@SCRIPT@: the games' data is not here yet -- run ./linux-install.sh first" >&2; exit 1; }
done
[ -f "$GAME/$EXE" ] || { echo "@SCRIPT@: no $GAME/$EXE" >&2; exit 1; }
mkdir -p "$GAME/ServerData"

# A configuration with default settings (the header), in the format the
# game's Host menu saves (ServerOptions.cpp)
crlf() { sed 's/$/\r/'; }
if [ -n "$CONFIG" ] && [ ! -f "$GAME/ServerData/$CONFIG.txt" ]; then
    SCPW="$(od -An -N6 -tx1 /dev/urandom | tr -d ' \n')"
    {
        printf '[Settings]\nServerName = "@TITLE@"\nPassword = ""\nSCPassword = "%s"\n' "$SCPW"
        printf 'Locked = 0\nDedicated = 1\nLANConnection = %s\nGameType = 1\nPortID = 27888\n' "$LAN"
        printf 'Bandwidth = 10000000\nBandwidth30 = 1\n\n'
        if [ -f "$GAME/ServerData/default.txt" ]; then
            # Primal Hunt: its own default configuration (its worlds, and
            # the lists of the classes and weapons that its server allows),
            # without its [Settings]
            tr -d '\r' < "$GAME/ServerData/default.txt" | awk '
                /^\[/ { skip = ($0 == "[Settings]") }
                !skip'
        else
            printf '[Deathmatch]\nMaxTotalPlayers = 8\nFragLimit = 25\nScoreLimit = 0\nTimeLimit = 15\n'
            i=0
            for w in DM_Compound DM_Depot DM_Verloc DM_Colony DM_Bunker DM_Auriga DM_Outpost4 \
                     DM_Reservoir DM_Leadworks DM_Quarantine DM_ALesserFate "DM_Lucifer'sFate"; do
                printf 'Map%02d = "Worlds\\Multi\\DM\\%s"\n' $i "$w"
                i=$((i + 1))
            done
        fi
    } | crlf > "$GAME/ServerData/$CONFIG.txt"
    echo "@SCRIPT@: wrote $GAME/ServerData/$CONFIG.txt (default settings; the server-control password is $SCPW)"
fi

if [ -n "$HEADLESS" ]; then
    WINEPREFIX="${WINEPREFIX:-$HOME/.wine-avp2-server-$GAME}"
else
    WINEPREFIX="${WINEPREFIX:-$HOME/.wine-avp2}"
fi
WINEDEBUG="${WINEDEBUG:--all}"
export WINEPREFIX WINEDEBUG
WINE="${WINE:-wine}"
command -v "$WINE" >/dev/null 2>&1 || { echo "@SCRIPT@: '$WINE' not found -- install Wine (32-bit support needed)" >&2; exit 1; }
if [ -n "$HEADLESS" ]; then
    command -v xvfb-run >/dev/null 2>&1 || { echo "@SCRIPT@: no display, and no xvfb-run (install xvfb) to make one" >&2; exit 1; }
    # When a headless server stops by force (Ctrl+C, kill), the Wine server
    # of its prefix can stay alive, with its desktop on a virtual display
    # that is gone. The next server in that prefix then crashes when it
    # starts ("Unhandled page fault"). If no server of this game runs, stop
    # that Wine server first. (The [A] stops pgrep from finding itself.)
    PATTERN="$(echo "@EXE@" | sed 's/^./[&]/')"
    if ! pgrep -f "$PATTERN" >/dev/null 2>&1; then
        "${WINESERVER:-wineserver}" -k >/dev/null 2>&1 || true
    fi
fi
# A command on the display: the virtual one when headless (24-bit: the
# default screen of xvfb-run has 8-bit colour)
ondisplay() {
    if [ -n "$HEADLESS" ]; then
        xvfb-run -a -s "-screen 0 1024x768x24" "$@"
    else
        "$@"
    fi
}

# The registry values of the retail setups, as linux-run-avp2.sh writes
# them: Primal Hunt's server finds AvP2's archives by AvP2's InstallDir.
# Both views (linux-run-avp2.sh tells why).
[ -d "$WINEPREFIX/dosdevices" ] || ondisplay "$WINE" wineboot -i >/dev/null 2>&1 || true
reg() { "$WINE" reg add "$@" /reg:32 /f >/dev/null 2>&1
        "$WINE" reg add "$@" /reg:64 /f >/dev/null 2>&1 || true; }
AVP2KEY='HKLM\Software\Monolith Productions\Aliens vs. Predator 2\1.0'
PHKEY='HKLM\Software\Third Law Interactive\Aliens versus Predator 2: Primal Hunt\1.0'
reg "$AVP2KEY" /v InstallDir /t REG_SZ /d "$("$WINE" winepath -w "$HERE/AVP2" 2>/dev/null)"
reg "$AVP2KEY" /v 'Update Command Line' /t REG_SZ /d '-rez AVP2P1.REZ'
reg "$PHKEY" /v InstallDir /t REG_SZ /d "$("$WINE" winepath -w "$HERE/PrimalHunt" 2>/dev/null)"
# The registry commands above run with no display, and they start a Wine
# server whose desktop has no display. Wait until it stops: when headless,
# a server program that connects to it crashes ("Unhandled page fault").
if [ -n "$HEADLESS" ]; then
    "${WINESERVER:-wineserver}" -w >/dev/null 2>&1 || true
fi

# The retail bug of the header, in a configuration of the Host menu
if [ "$MODERN" != 1 ] && [ -n "$CONFIG" ] && tr -d '\r ' < "$GAME/ServerData/$CONFIG.txt" | grep -qx 'LANConnection=0'; then
    echo "@SCRIPT@: warning: ServerData/$CONFIG.txt has LANConnection = 0. In the legacy build such a server crashes when the world starts (no SierraUp.cfg; set LANConnection = 1, or use the modern build)" >&2
fi

cd "$GAME"
if [ -n "$CONFIG" ]; then
    set -- -config "$CONFIG" "$@"
fi
ondisplay "$WINE" $EXE "$@"
