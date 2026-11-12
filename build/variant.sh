#!/usr/bin/env bash
# The games built from the same source with project defines that the
# release build never has -- never for measuring: progress is only ever
# scored on the release build (`make`, `make progress`).
#
#   build/variant.sh debug         make debug          -> out/avp2-legacy-debug/
#   build/variant.sh modern        make modern         -> out/avp2-modern-release/
#   build/variant.sh modern-debug  make modern-debug   -> out/avp2-modern-debug/
#
# (on the host the make targets run this in the build container; inside
# `make shell` run it directly). The result is a game directory like
# out/avp2-legacy-release/ (build/all.sh: AVP2/, PrimalHunt/).
#
#   debug    AVP2SRC_DEBUG and symbols, for finding out why something goes
#            wrong. Compile + /Z7 /Oy- /D AVP2SRC_DEBUG (debug information,
#            frame pointers kept so a backtrace can walk the stack; every
#            other flag, the optimisation included, is the release one);
#            resource scripts + /d AVP2SRC_DEBUG;
#            link + /DEBUG /OPT:REF /INCREMENTAL:NO (a .pdb per binary, the
#            same unreferenced-code removal as release). The .pdb and .map of
#            every binary go beside it. src/avp2src/avp2src_debug.h's
#            AVP2SRC_LOG lines go to avp2src.log in the game's directory.
#   modern   AVP2SRC_MODERN: the modern build's fixes and features
#            (docs/MODERN.md; none yet). Compile + /D AVP2SRC_MODERN,
#            resource scripts + /d AVP2SRC_MODERN. Its middleware is still
#            the retail one (the open replacements are to come).
#   modern-debug   both.
#
# The flags reach every compile and link through build/env.sh's
# AVP2SRC_CFLAGS / AVP2SRC_LDFLAGS / AVP2SRC_RCFLAGS, which are empty for
# every release command -- and which build/env.sh refuses outside the trees
# this script makes, so they cannot reach the release build by accident.
#
# It never touches the release build. The source is copied (without any
# build output) into out/build-<variant>/ and built there by the same
# build/all.sh, so src/*/ -- what `make progress` scores -- and
# out/avp2-legacy-release/ are left exactly as they were. The copy keeps its
# own build outputs from one run to the next, and only changed sources are
# rewritten into it, so a rebuild redoes only what changed. The game data
# are extracted into out/<name>/ on its first run (hard links to the retail
# files where they can be) and kept after that. Last, the game directory is
# packaged as out/<name>-private.tar.gz and -public.tar.gz, like the release
# build's (build/package.sh).
set -e
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO"

VARIANT="${1:-}"
case "$VARIANT" in
    debug)        CFLAGS="/Z7 /Oy- /D AVP2SRC_DEBUG"
                  LDFLAGS="/DEBUG /OPT:REF /INCREMENTAL:NO"
                  RCFLAGS="/d AVP2SRC_DEBUG" ;;
    modern)       CFLAGS="/D AVP2SRC_MODERN"
                  LDFLAGS=""
                  RCFLAGS="/d AVP2SRC_MODERN" ;;
    modern-debug) CFLAGS="/Z7 /Oy- /D AVP2SRC_DEBUG /D AVP2SRC_MODERN"
                  LDFLAGS="/DEBUG /OPT:REF /INCREMENTAL:NO"
                  RCFLAGS="/d AVP2SRC_DEBUG /d AVP2SRC_MODERN" ;;
    *) echo "usage: build/variant.sh debug|modern|modern-debug" >&2; exit 2 ;;
esac
# the game directory it builds: out/avp2-<legacy|modern>-<release|debug>/
case "$VARIANT" in
    debug)        NAME=avp2-legacy-debug ;;
    modern)       NAME=avp2-modern-release ;;
    modern-debug) NAME=avp2-modern-debug ;;
esac

if ! command -v wine >/dev/null 2>&1; then
    echo "error: no wine here -- run \`make $VARIANT\` from the repository root, or" >&2
    echo "       run this inside \`make shell\`." >&2
    exit 1
fi

TREE="$REPO/out/build-$VARIANT"
echo "== $VARIANT tree: src/ build/ tools/ -> out/build-$VARIANT/ (sources only; its outputs kept)"
mkdir -p "$TREE"
python3 tools/releasesrc.py mirror "$REPO/src" "$TREE/src"
rm -rf "$TREE/build" "$TREE/tools"
tar -C "$REPO" -cf - --exclude='__pycache__' --exclude=.git build tools | tar -C "$TREE" -xf -
# the compilers, the retail games where build/retail.sh looks for them, and
# the prebuilt libraries build/build.mk takes from progress/reference/
[ -L "$TREE/toolchains" ] || ln -s ../../toolchains "$TREE/toolchains"
[ -L "$TREE/rez" ] || ln -s ../../rez "$TREE/rez"
[ -L "$TREE/progress" ] || ln -s ../../progress "$TREE/progress"

export AVP2SRC_CFLAGS="$CFLAGS"
export AVP2SRC_LDFLAGS="$LDFLAGS"
export AVP2SRC_RCFLAGS="$RCFLAGS"
export AVP2_GAME_OUT="$REPO/out/$NAME"
export AVP2_PACKAGE=0
bash "$TREE/build/all.sh"

case "$VARIANT" in *debug)
    echo
    echo "== symbols: each built binary's .pdb (and .map) beside it in out/$NAME/"
    # each MANIFEST.txt has `built <source>\t<installed path>` for every
    # binary we built; its .pdb is the link output's name with .pdb, and the
    # debugger looks for it by that name in the binary's own directory.
    for g in AVP2 PrimalHunt; do
        grep '^built ' "$AVP2_GAME_OUT/$g/MANIFEST.txt" | sort -u |
        while IFS=$'\t' read -r src dest; do
            src="${src#built }"
            pdb="$TREE/${src%.*}.pdb"
            if [ -f "$pdb" ]; then
                cp "$pdb" "$AVP2_GAME_OUT/$g/$(dirname "$dest")/"
                echo "   $g/$(dirname "$dest")/$(basename "$pdb")"
            else
                echo "   (no .pdb for $g/$dest)"
            fi
            # the linker's map, for addresses no .pdb names (avp2src_crash.h)
            map="$TREE/${src%.*}.map"
            [ -f "$map" ] && cp "$map" "$AVP2_GAME_OUT/$g/$(dirname "$dest")/"
        done
    done ;;
esac
echo
echo "== package: out/$NAME/ -> out/$NAME-private.tar.gz, -public.tar.gz"
bash "$TREE/build/package.sh" "out/$NAME"
echo "== done: out/$NAME/"
