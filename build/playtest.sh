#!/usr/bin/env bash
# The runtime test, for AvP2 and Primal Hunt: ext/tools/playtest.sh (shared with
# nolf2src, whose game names it assumes) run on one game of a game
# directory.
#
#   build/playtest.sh [gamedir] [avp2|ph] [world] [seconds]
#                                    (in the build container; `make test`
#                                    runs it for both games)
#
# gamedir is a whole game directory, as build/all.sh assembles it (default
# out/avp2-legacy-release); avp2 (the default) tests AVP2/ in it, ph
# PrimalHunt/. world and seconds, the PLAYTEST_* settings, the input scripts
# and the results are ext/tools/playtest.sh's (its header), with AvP2's names in
# place of NOLF2's: the engine lithtech.exe, the command file avp2cmds.txt
# (build/avp2cmds.sh); "-" for world (the default) starts on the main menu.
# The results are in out/play/<name>/AVP2/ or .../PrimalHunt/, <name> being
# $PLAYTEST_NAME or the game directory's name. Primal Hunt reads AvP2's
# archives from ../AVP2/: beside its run directory that is the game
# directory's AVP2/ (a link), or AvP2's own run directory if it was tested
# first. Two settings of its own: PLAYTEST_MOVIES=1 plays the intro movies
# (ext/tools/playtest.sh otherwise always adds +DisableMovies 1;
# build/playtest-movies.txt), and PLAYTEST_AUDIO=<prefix> (an absolute path)
# records what the game plays where ext/tools/playtest.sh plays it into
# ALSA's null device (no sound card: the container): ALSA's file plugin in
# front of the null device writes each stream, raw, to
# <prefix>-<rate>-<channels>-<format>.raw (streams of one format appended to
# one file). The null device takes data faster than it would play, so the
# recording holds what was played, not when. The adapted copy of
# ext/tools/playtest.sh is written to
# out/play/playtest-avp2.sh each run; ext/tools/ is not changed.
set -u
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAMEDIR="${1:-out/avp2-legacy-release}"
WHICH="${2:-avp2}"
case "$WHICH" in
    avp2) SUB=AVP2 ;;
    ph)   SUB=PrimalHunt ;;
    *)    echo "usage: build/playtest.sh [gamedir] [avp2|ph] [world] [seconds]" >&2; exit 2 ;;
esac
shift 2 2>/dev/null || shift $#
case "$GAMEDIR" in /*) GAME="$GAMEDIR" ;; *) GAME="$REPO/$GAMEDIR" ;; esac
GAME="${GAME%/}"
NAME="${PLAYTEST_NAME:-$(basename "$GAME")}"
mkdir -p "$REPO/out/play/$NAME"
if [ "$SUB" = PrimalHunt ] && [ ! -e "$REPO/out/play/$NAME/AVP2" ]; then
    ln -s "$(python3 -c 'import os,sys; print(os.path.relpath(sys.argv[1], sys.argv[2]))' \
        "$GAME/AVP2" "$REPO/out/play/$NAME")" "$REPO/out/play/$NAME/AVP2"
fi
OUT="$REPO/out/play/playtest-avp2.sh"
sed -e 's/nolf2cmds\.txt/avp2cmds.txt/g' \
    -e 's/Lithtech\.exe/lithtech.exe/g' \
    -e 's#out/nolf2-legacy-release#out/avp2-legacy-release/AVP2#g' \
    -e 's#Worlds\\\\RetailSinglePlayer\\\\C01S01#-#' \
    -e 's#^source .*/layout\.sh"$#REPO="'"$REPO"'"#' \
    -e "s/unhandled exception|page fault/unhandled exception|unhandled illegal instruction|unhandled page fault|page fault/g" \
    -e 's/ +DisableMovies 1\${PLAYTEST_ARGS/$([ -n "${PLAYTEST_MOVIES:-}" ] || printf " +DisableMovies 1")${PLAYTEST_ARGS/' \
    -e '/pcm\.!default { type null }/a\    [ -z "${PLAYTEST_AUDIO:-}" ] || printf "</usr/share/alsa/alsa.conf>\\npcm.!default { type file slave.pcm null file \\"%s-%%r-%%c-%%f.raw\\" format raw truncate false }\\n" "$PLAYTEST_AUDIO" > asound.conf' \
    "$REPO/ext/tools/playtest.sh" > "$OUT"
PLAYTEST_NAME="$NAME/$SUB" exec bash "$OUT" "$GAME/$SUB" "$@"
