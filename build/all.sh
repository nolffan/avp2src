#!/usr/bin/env bash
# THE build: Aliens versus Predator 2 and Primal Hunt in
# out/avp2-legacy-release/, with every binary compiled here and the data
# taken from the retail games.
#
#   make                    (on the host: builds the docker image, installs
#                            and checks the retail games, runs this in a
#                            container)
#   build/all.sh            (inside the build container, e.g. `make shell`)
#   make test               (then: play it -- build/playtest.sh)
#
# Needs: the toolchain (ext/toolchains/, see ext/toolchains/README.md), Wine (the
# build container has it), and the two retail games as build/discs.py
# installs them from your own disc images (`make retail`, build/retail.sh):
# Primal Hunt (Europe) 1.0.2.2, out/retail/avp2-ph-eu/, and AvP2 (Europe)
# with the English 1.0.9.6 patch, out/retail/avp2-eu-1.0.9.6/. It only
# reads them.
#
# The game directory holds both games, each in a directory of its own, as
# their setups install them (Primal Hunt installs beside AvP2, never over
# it, and plays on AvP2's archives; AvP2's campaigns are AvP2's own game
# DLLs', which need AvP2 1.0.9.6's engine: docs/PLAN.md, package 10):
#
#   AVP2/          AvP2 1.0.9.6: its engine (lithtech.exe, server.dll,
#                  d3d.ren, ltmsg.dll), launcher (AVP2.exe, Launch.dll,
#                  AVP2Up.dll), dedicated server (AVP2Serv.exe), the game
#                  DLLs in custom/, and its archives and data
#   PrimalHunt/    Primal Hunt 1.0.2.2: its engine (with cdaudio.dll),
#                  launcher (PrimalHunt.exe, Launch.dll, AVP2XUp.dll),
#                  dedicated server (AVP2XServ.exe), its game DLLs in
#                  custom/, and its own archives; AvP2's it reads from
#                  ../AVP2/ (avp2cmds.txt: build/avp2cmds.sh)
#                  (the modern build: AVP2.exe and PrimalHunt.exe start
#                  the game directly, and AVP2Launcher.exe and
#                  PrimalHuntLauncher.exe are the launchers; MODERN.md M-8)
#   linux-run-avp2.sh, linux-run-ph.sh, windows-run.bat   start a game (build/game-scripts/)
#   linux-server-avp2.sh, linux-server-ph.sh   start a dedicated server (build/game-scripts/)
#
# The steps, in order (each for both games):
#   1. resources          the games' data from the retail games: the .rez
#                         archives (with the retail game DLLs inside them
#                         zeroed), the movies, Primal Hunt's ServerData/, the
#                         loose files the games read (console.PCX,
#                         childmodel.map, Primal Hunt's FontData.fnt), the
#                         readmes, the shipped autoexec.cfg and default
#                         profile. No binary.
#   2. third-party runtime  Bink (binkw32.dll), Smacker (Smackw32.dll) and
#                         Miles (mss32.dll, its MP3 decoder MP3DEC.ASI/.AS6),
#                         from the retail games: licensed middleware with no
#                         source, which the engines import or load. The
#                         modern build (AVP2SRC_MODERN) takes none of them:
#                         OpenMiles' mss32.dll and our Bink player's
#                         binkw32.dll instead, built from ext/ by
#                         build/ext.sh (docs/MODERN.md, M-X1..M-X3); no
#                         Smacker (neither engine plays one) and no MP3
#                         decoder (OpenMiles decodes MP3 itself). It also
#                         installs our Direct3D 9 renderer, d3d9.ren
#                         (src/modern/render/; docs/MODERN.md, M-29), its
#                         default renderer; our d3d.ren is d3d7.ren there.
#   3. compile            every binary, by build/build.mk, all at once (one
#                         job per CPU; AVP2SRC_JOBS=<n> to change it) and only
#                         what changed since the last build.
#   4. assemble           the built binaries into each game's directory: the
#                         game DLLs in its custom/ rez directory, the last in
#                         its rez stack; the rest in its root.
#   5. audit              every PE file in the game directory listed with
#                         its origin; the build fails if any is unaccounted
#                         for or if a .rez archive still holds one.
#   6. package            out/avp2-legacy-release-private.tar.gz: the game
#                         directory as one file, and -public.tar.gz: the same
#                         without the retail games' files, which its install
#                         scripts copy out of the player's own disc images
#                         (build/package.sh, build/mkinstall.py).
set -e
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Wine and the toolchain live in the build container: on the host, use make.
if ! command -v wine >/dev/null 2>&1; then
    echo "error: no wine here -- run \`make\` from the repository root (it runs this" >&2
    echo "       script in the build container), or run it inside \`make shell\`." >&2
    exit 1
fi
export WINEDEBUG=-all

cd "$REPO"
source build/retail.sh
# AVP2_NODISCS=1 (make NODISCS=1): without the retail games. Every binary is
# compiled as always (the art is assets/art/'s), but steps 1 and 2 copy no
# game data and no Bink, Smacker or Miles: they write only the MANIFEST.txt
# rows (build/nodiscs.py, from build/retail-files.txt). The game directory
# is out/<name>-nodiscs/, with NOT-PLAYABLE.txt in it; it does not play.
# Its public archive, out/<name>-public.tar.gz, is the one a build with the
# retail games makes (build/package.sh).
NODISCS=
[ "${AVP2_NODISCS:-0}" = 1 ] && NODISCS=1
NAME="$(basename "${AVP2_GAME_OUT:-out/avp2-legacy-release}")"
OUT="${AVP2_GAME_OUT:-out/avp2-legacy-release${NODISCS:+-nodiscs}}"
if [ -n "$NODISCS" ]; then
    NAME="${NAME%-nodiscs}"
    RETAIL_PH= RETAIL_AVP2=
else
    RETAIL_PH="$(retail_dir)"
    RETAIL_AVP2="$(retail_avp2_dir)"
    retail_verify
    # build/retail-ranges.txt (what the install scripts zero), and the retail
    # art the launchers and Primal Hunt's server are compiled with, committed
    # in assets/art/: still what the retail games give
    echo "== build/retail-ranges.txt and assets/art/ against the retail games"
    python3 build/discs.py ranges check "$(dirname "$RETAIL_PH")"
    python3 build/art.py check "$(dirname "$RETAIL_PH")"
fi
T0=$(date +%s)
step() { echo; echo "######## $* ($(( $(date +%s) - T0 ))s)"; }
MODERN=
case " ${AVP2SRC_CFLAGS:-} " in *AVP2SRC_MODERN*) MODERN=1 ;; esac

# The two games: their directory, retail install, layout (build/retail*.py),
# the loose files they read, and what we build for them.
P=src/AVP2
E=src/engine
BUILT_AVP2=(
    "$E/lithtech/AvP2Release/lithtech.exe"   lithtech.exe
    "$E/server/AvP2Release/server.dll"       server.dll
    "$E/d3d/AvP2Release/d3d.ren"             d3d.ren
    "$E/ltmsg/AvP2Release/ltmsg.dll"         ltmsg.dll
    src/launch/Debug/Launch.dll              Launch.dll
    src/launcher/Release/AVP2.exe            AVP2.exe
    "$P/AVP2Up/Release/AVP2Up.dll"           AVP2Up.dll
    "$P/AVP2Serv/Release/AVP2Serv.exe"       AVP2Serv.exe
    "$P/ClientShellDLL/final/CShell.dll"     custom/cshell.dll
    "$P/ObjectDLL/final/Object.lto"          custom/object.lto
    "$P/ClientRes/Release/CRes.dll"          custom/cres.dll
    "$P/ServerRes/Release/SRes.dll"          custom/sres.dll
)
BUILT_PH=(
    "$E/lithtech/Release/lithtech.exe"       lithtech.exe
    "$E/server/Release/server.dll"           server.dll
    "$E/d3d/Release/d3d.ren"                 d3d.ren
    "$E/ltmsg/Release/ltmsg.dll"             ltmsg.dll
    "$E/cdaudio/Release/cdaudio.dll"         cdaudio.dll
    src/launch/Debug/Launch.dll              Launch.dll
    src/launcher/PrimalHunt/PrimalHunt.exe   PrimalHunt.exe
    "$P/AVP2Up/PrimalHunt/AVP2XUp.dll"       AVP2XUp.dll
    "$P/AVP2Serv/PrimalHunt/AVP2XServ.exe"   AVP2XServ.exe
    "$P/ClientShellDLL/PrimalHunt/CShell.dll" custom/cshell.dll
    "$P/ObjectDLL/PrimalHunt/Object.lto"     custom/object.lto
    "$P/ClientRes/PrimalHunt/CRes.dll"       custom/cres.dll
    "$P/ServerRes/PrimalHunt/SRes.dll"       custom/sres.dll
)
# The modern build's default renderer is d3d9.ren (src/modern/render/,
# docs/MODERN.md M-29); our d3d.ren ships beside it as d3d7.ren. (The engine
# and the launcher load d3d9.ren for a config that names d3d.ren, and do not
# list a d3d.ren that an older modern build left.)
if [ -n "$MODERN" ]; then
    BUILT_AVP2[5]=d3d7.ren
    BUILT_PH[5]=d3d7.ren
fi
# The modern build's launchers (docs/MODERN.md M-8): AVP2.exe and
# PrimalHunt.exe start the game directly; these are the launchers
if [ -n "$MODERN" ]; then
    BUILT_AVP2+=(src/launcher/Release/AVP2Launcher.exe        AVP2Launcher.exe)
    BUILT_PH+=(src/launcher/PrimalHunt/PrimalHuntLauncher.exe PrimalHuntLauncher.exe)
fi
LOOSE_AVP2=(console.PCX childmodel.map readme.txt readme1096.txt changelog.txt EULA.txt)
LOOSE_PH=(console.PCX childmodel.map FontData.fnt readme.txt license.txt)

# loose <retail dir> <game dir> <file>...: retail data outside the archives
# and data directories, as it is (MANIFEST.txt: data <release>:<file>)
loose() {
    local r=$1 g=$2 f
    shift 2
    for f in "$@"; do
        if [ -n "$NODISCS" ]; then
            printf 'data %s:%s\t%s\n' "$(basename "$r")" "$f" "$f" >> "$g/MANIFEST.txt"
            continue
        fi
        [ -f "$r/$f" ] || { echo "error: $r/$f missing" >&2; exit 1; }
        if [ ! -f "$g/$f" ] || ! cmp -s "$r/$f" "$g/$f"; then
            cp "$r/$f" "$g/$f.part" && mv "$g/$f.part" "$g/$f"
        fi
        printf 'data %s:%s\t%s\n' "$(basename "$r")" "$f" "$f" >> "$g/MANIFEST.txt"
    done
}

# game <name> <dir> <retail dir> <layout>: steps 1 and 2 for one game
# (NODISCS: <retail dir> is the release's name only, and nothing is copied)
game_data() {
    local name=$1 g="$OUT/$2" r=$3 layout=$4
    shift 4
    mkdir -p "$g/Save" "$g/custom" "$g/Profiles"
    if [ -n "$NODISCS" ]; then
        step "1. resources ($name): no game data (NODISCS=1), only its MANIFEST.txt rows"
        RETAIL_LAYOUT=$layout python3 build/nodiscs.py resources "$r" "$g"
        loose "$r" "$g" "$@"
    else
        step "1. resources ($name): game data from $r -> $g"
        RETAIL_LAYOUT=$layout python3 ext/tools/retailextract.py resources "$r" "$g"
        loose "$r" "$g" "$@"
        # autoexec.cfg and the default profile start as the shipped ones (the
        # games rewrite them); a rebuild keeps the player's.
        [ -f "$g/autoexec.cfg" ] || cp "$g/autoexec.cfg.retail" "$g/autoexec.cfg"
        [ -f "$g/Profiles/Default.prf" ] || cp "$g/Default.prf.retail" "$g/Profiles/Default.prf"
    fi
    if [ -n "$MODERN" ]; then
        step "2. open runtime ($name): OpenMiles, our Bink player on FFmpeg and our d3d9.ren (build/ext.sh)"
        # what an earlier build took from the retail game: the open runtime
        # replaces binkw32.dll and mss32.dll, and the rest is not needed
        # (its MANIFEST.txt rows go at step 4, with the files)
        local f
        for f in "$g"/*; do
            case "$(basename "$f" | tr '[:upper:]' '[:lower:]')" in
                smackw32.dll|mp3dec.asi|mp3dec.as6) rm -f "$f"; echo "   (removed $(basename "$f"))" ;;
            esac
        done
        RETAIL_LAYOUT=$layout python3 ext/tools/retailextract.py openruntime "${AVP2_EXT_OUT:-out/ext}" "$g"
        # d3d9.ren is built for each game (build/ext.sh): Primal Hunt's is
        # out/ext/ph/; modern-debug's have the debug checks
        local x="${AVP2_EXT_OUT:-out/ext}"
        [ "$name" = "Primal Hunt" ] && x="$x/ph"
        case "${AVP2SRC_CFLAGS:-}" in *AVP2SRC_DEBUG*) x="$x/debug" ;; esac
        if [ "$x" != "${AVP2_EXT_OUT:-out/ext}" ]; then
            cp "$x/d3d9.ren" "$g/d3d9.ren.part" && mv "$g/d3d9.ren.part" "$g/d3d9.ren"
            echo "   d3d9.ren  <-  $x/d3d9.ren"
        fi
    elif [ -n "$NODISCS" ]; then
        step "2. THIRD-PARTY RUNTIME ($name): no Bink, Smacker or Miles (NODISCS=1), only their MANIFEST.txt rows"
        RETAIL_LAYOUT=$layout python3 build/nodiscs.py thirdparty "$r" "$g"
    else
        step "2. THIRD-PARTY RUNTIME ($name): Bink, Smacker and Miles from $r"
        RETAIL_LAYOUT=$layout python3 ext/tools/retailextract.py thirdparty "$r" "$g"
    fi
}
[ -z "$MODERN" ] || { step "0. open middleware: build/ext.sh (OpenMiles, FFmpeg, our Bink player, d3d9.ren)"; bash build/ext.sh; }
mkdir -p "$OUT/AVP2" "$OUT/PrimalHunt"
if [ -n "$NODISCS" ]; then
    RETAIL_AVP2=$RETAIL_AVP2_RELEASE RETAIL_PH=$RETAIL_RELEASE
    cat > "$OUT/NOT-PLAYABLE.txt" <<'TXT'
This game directory was built without the retail games (make NODISCS=1).
Its binaries are complete, but it has no game data, and no Bink, Smacker
or Miles: it does not play. Its public archive (<name>-public.tar.gz) is
the one to play: unpack it, and its install scripts copy the games' data
out of your own disc images.
TXT
fi
game_data "AvP2 1.0.9.6" AVP2 "$RETAIL_AVP2" build/retail_avp2.py "${LOOSE_AVP2[@]}"
bash build/avp2cmds.sh avp2 ${MODERN:+open} > "$OUT/AVP2/avp2cmds.txt"
# customrez.txt: AvP2's dedicated server (AVP2Serv.exe, ServerDlg.cpp's
# AddResources) looks for custom.rez, not the custom/ directory our game
# DLLs are in (Primal Hunt's names custom/); the file it reads for more
# archives names it, or the server finds retail's zeroed binaries and stops
# ("Unable to load the game binaries")
printf '[Rez]\r\nRez0 = "custom"\r\n' > "$OUT/AVP2/customrez.txt"
printf 'ours build/all.sh (the dedicated server reads custom/)\tcustomrez.txt\n' >> "$OUT/AVP2/MANIFEST.txt"
game_data "Primal Hunt" PrimalHunt "$RETAIL_PH" build/retail.py "${LOOSE_PH[@]}"
bash build/avp2cmds.sh ph ${MODERN:+open} > "$OUT/PrimalHunt/avp2cmds.txt"
# linux-run-avp2.sh, linux-run-ph.sh: start AvP2 or Primal Hunt with the
# host's Wine, both from one template (build/game-scripts/linux-run.sh);
# windows-run.bat: either game straight from its avp2cmds.txt (usage in their
# headers). On Windows the launchers are the way in.
runscript() {   # <script> <game directory> <title> <launcher>
    sed -e "s|@SCRIPT@|$1|g" -e "s|@GAME@|$2|g" -e "s|@TITLE@|$3|g" \
        -e "s|@LAUNCHER@|$4|g" -e "s|@MODERN@|${MODERN:-0}|g" build/game-scripts/linux-run.sh > "$OUT/$1"
    chmod +x "$OUT/$1"
}
rm -f "$OUT/linux-run.sh"     # (one script for both, before linux-run-avp2/-ph.sh)
L=; [ -z "$MODERN" ] || L=Launcher      # (the launchers' names: M-8)
runscript linux-run-avp2.sh AVP2 "Aliens versus Predator 2" "AVP2$L.exe"
runscript linux-run-ph.sh PrimalHunt "Aliens versus Predator 2: Primal Hunt" "PrimalHunt$L.exe"
sed 's/$/\r/' build/game-scripts/windows-run.bat > "$OUT/windows-run.bat"     # (CRLF)
# linux-server-avp2.sh, linux-server-ph.sh: a dedicated server of AvP2 or
# Primal Hunt with the host's Wine, both from one template
# (build/game-scripts/linux-server.sh; docs/MODERN.md, M-26)
serverscript() {   # <script> <game directory> <title> <server program>
    sed -e "s|@SCRIPT@|$1|g" -e "s|@GAME@|$2|g" -e "s|@TITLE@|$3|g" \
        -e "s|@EXE@|$4|g" -e "s|@MODERN@|${MODERN:-0}|g" build/game-scripts/linux-server.sh > "$OUT/$1"
    chmod +x "$OUT/$1"
}
serverscript linux-server-avp2.sh AVP2 "Aliens vs. Predator 2" AVP2Serv.exe
serverscript linux-server-ph.sh PrimalHunt "Primal Hunt" AVP2XServ.exe

step "3. compile: the workspaces (build/compile.sh: build/build.mk, ${AVP2SRC_JOBS:-$(nproc)} jobs)"
# The engines too -- lithtech.exe, server.dll and d3d.ren, both games' --
# which `all` leaves alone. In the release tree they are the ones `make
# progress` scores, built the same way here; a fresh tree (CI's) has none
# until something links them ("src/engine/lithtech/AvP2Release/lithtech.exe
# was not built"). A variant tree (build/variant.sh: its flags set) builds
# its own with its flags, or it would ship the release build's engines, and
# the modern build would have none of its engine changes. And every
# lithshared library of ours (`libs`): `make progress` scores each, and
# RegMgr.lib is linked by nothing, so a fresh tree had none ("RegMgr:
# unwritten functions went up, 0 -> 14").
bash build/compile.sh all engine-link d3d-link avp2-d3d-link libs

# assemble <game dir> <built> <dest> ...
assemble() {
    local g="$OUT/$1" i
    shift
    local b=("$@")
    for ((i = 0; i < ${#b[@]}; i += 2)); do
        [ -f "${b[i]}" ] || { echo "error: ${b[i]} was not built" >&2; exit 1; }
        cp "${b[i]}" "$g/${b[i+1]}"
        printf 'built %s\t%s\n' "${b[i]}" "${b[i+1]}" >> "$g/MANIFEST.txt"
        echo "   $1/${b[i+1]}  <-  ${b[i]}"
    done
    # one row per file: the manifest is rewritten by the next step's tool
    python3 - "$g/MANIFEST.txt" "${NODISCS:-0}" <<'PY'
import os, sys
p, nodiscs = sys.argv[1], sys.argv[2] == "1"
rows = {}
for line in open(p).read().splitlines():
    if "\t" in line and not line.startswith("#"):
        origin, path = line.split("\t", 1)
        # a file an older build put here and this one removed (without the
        # retail games, their rows stay: their files are the install's)
        if os.path.exists(os.path.join(os.path.dirname(p), path)) or \
                (nodiscs and origin.startswith(("data ", "THIRD-PARTY "))):
            rows[path] = origin
with open(p, "w") as f:
    f.write("# origin<TAB>path, one row per file build/all.sh put here\n")
    for path in sorted(rows, key=str.lower):
        f.write(f"{rows[path]}\t{path}\n")
PY
}
step "4. assemble: the built binaries -> $OUT/AVP2, $OUT/PrimalHunt"
# (the modern build's d3d.ren is d3d7.ren: an older build's d3d.ren goes)
[ -z "$MODERN" ] || rm -f "$OUT/AVP2/d3d.ren" "$OUT/PrimalHunt/d3d.ren"
assemble AVP2 "${BUILT_AVP2[@]}"
assemble PrimalHunt "${BUILT_PH[@]}"

step "5. audit: every PE file in $OUT"
for g in AVP2 PrimalHunt; do
    echo "== $g/"
    python3 ext/tools/retailextract.py audit "$OUT/$g"
done
# nothing but the two games' directories and the run scripts at the top
for f in "$OUT"/*; do
    case "$(basename "$f")" in
        AVP2|PrimalHunt|linux-run-avp2.sh|linux-run-ph.sh|linux-server-avp2.sh|linux-server-ph.sh|windows-run.bat) ;;
        NOT-PLAYABLE.txt) [ -n "$NODISCS" ] || { echo "error: $f: a build with the retail games is playable" >&2; exit 1; } ;;
        *) echo "error: $f: not a file the build puts there" >&2; exit 1 ;;
    esac
done

# (AVP2_PACKAGE=0 skips it: build/variant.sh packages its game directory
# itself, once the symbols are in)
if [ "${AVP2_PACKAGE:-1}" = 1 ]; then
    step "6. package: $OUT -> $(dirname "$OUT")/$NAME-private.tar.gz, -public.tar.gz"
    bash "$REPO/build/package.sh" "$OUT" "$NAME"
fi

if [ -n "$NODISCS" ]; then
    step "done: $OUT ($(( $(date +%s) - T0 ))s)${MODERN:+ (the modern build)} -- built without the retail games: it does not play; its public archive does"
else
    step "done: $OUT ($(( $(date +%s) - T0 ))s)${MODERN:+ (the modern build)} -- play it with $OUT/linux-run-avp2.sh or linux-run-ph.sh, or test it with make test"
fi
