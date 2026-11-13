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
# build container has it), and the two retail games as the ext/rez/ submodule
# installs them (build/retail.sh): Primal Hunt (Europe) 1.0.2.2,
# ext/rez/games/avp2-ph-eu/, and AvP2 (Europe) with the English 1.0.9.6 patch,
# ext/rez/games/avp2-eu-1.0.9.6/. It only reads them.
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
#                         decoder (OpenMiles decodes MP3 itself).
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
OUT="${AVP2_GAME_OUT:-out/avp2-legacy-release}"
RETAIL_PH="$(retail_dir)"
RETAIL_AVP2="$(retail_avp2_dir)"
retail_verify
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
        [ -f "$r/$f" ] || { echo "error: $r/$f missing" >&2; exit 1; }
        if [ ! -f "$g/$f" ] || ! cmp -s "$r/$f" "$g/$f"; then
            cp "$r/$f" "$g/$f.part" && mv "$g/$f.part" "$g/$f"
        fi
        printf 'data %s:%s\t%s\n' "$(basename "$r")" "$f" "$f" >> "$g/MANIFEST.txt"
    done
}

# game <name> <dir> <retail dir> <layout>: steps 1 and 2 for one game
game_data() {
    local name=$1 g="$OUT/$2" r=$3 layout=$4
    shift 4
    step "1. resources ($name): game data from $r -> $g"
    RETAIL_LAYOUT=$layout python3 ext/tools/retailextract.py resources "$r" "$g"
    loose "$r" "$g" "$@"
    # autoexec.cfg and the default profile start as the shipped ones (the
    # games rewrite them); a rebuild keeps the player's.
    [ -f "$g/autoexec.cfg" ] || cp "$g/autoexec.cfg.retail" "$g/autoexec.cfg"
    mkdir -p "$g/Save" "$g/custom" "$g/Profiles"
    [ -f "$g/Profiles/Default.prf" ] || cp "$g/Default.prf.retail" "$g/Profiles/Default.prf"
    if [ -n "$MODERN" ]; then
        step "2. open runtime ($name): OpenMiles and our Bink player on FFmpeg (build/ext.sh)"
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
    else
        step "2. THIRD-PARTY RUNTIME ($name): Bink, Smacker and Miles from $r"
        RETAIL_LAYOUT=$layout python3 ext/tools/retailextract.py thirdparty "$r" "$g"
    fi
}
[ -z "$MODERN" ] || { step "0. open middleware: build/ext.sh (OpenMiles, FFmpeg, our Bink player)"; bash build/ext.sh; }
mkdir -p "$OUT/AVP2" "$OUT/PrimalHunt"
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
        -e "s|@LAUNCHER@|$4|g" build/game-scripts/linux-run.sh > "$OUT/$1"
    chmod +x "$OUT/$1"
}
rm -f "$OUT/linux-run.sh"     # (one script for both, before linux-run-avp2/-ph.sh)
L=; [ -z "$MODERN" ] || L=Launcher      # (the launchers' names: M-8)
runscript linux-run-avp2.sh AVP2 "Aliens versus Predator 2" "AVP2$L.exe"
runscript linux-run-ph.sh PrimalHunt "Aliens versus Predator 2: Primal Hunt" "PrimalHunt$L.exe"
sed 's/$/\r/' build/game-scripts/windows-run.bat > "$OUT/windows-run.bat"     # (CRLF)

step "3. compile: the workspaces (build/compile.sh: build/build.mk, ${AVP2SRC_JOBS:-$(nproc)} jobs)"
# A variant tree (build/variant.sh: its flags set) builds its own engines
# too -- lithtech.exe, server.dll and d3d.ren, both games' -- which `all`
# leaves alone (the release build's are the ones `make progress` scores):
# otherwise the tree would ship the release build's engines, and the
# modern build would have none of its engine changes.
if [ -n "${AVP2SRC_CFLAGS:-}${AVP2SRC_LDFLAGS:-}${AVP2SRC_RCFLAGS:-}" ]; then
    bash build/compile.sh all engine-link d3d-link avp2-d3d-link
else
    bash build/compile.sh all
fi

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
    python3 - "$g/MANIFEST.txt" <<'PY'
import os, sys
p = sys.argv[1]
rows = {}
for line in open(p).read().splitlines():
    if "\t" in line and not line.startswith("#"):
        origin, path = line.split("\t", 1)
        # a file an older build put here and this one removed
        if os.path.exists(os.path.join(os.path.dirname(p), path)):
            rows[path] = origin
with open(p, "w") as f:
    f.write("# origin<TAB>path, one row per file build/all.sh put here\n")
    for path in sorted(rows, key=str.lower):
        f.write(f"{rows[path]}\t{path}\n")
PY
}
step "4. assemble: the built binaries -> $OUT/AVP2, $OUT/PrimalHunt"
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
        AVP2|PrimalHunt|linux-run-avp2.sh|linux-run-ph.sh|windows-run.bat) ;;
        *) echo "error: $f: not a file the build puts there" >&2; exit 1 ;;
    esac
done

# (AVP2_PACKAGE=0 skips it: build/variant.sh packages its game directory
# itself, once the symbols are in)
if [ "${AVP2_PACKAGE:-1}" = 1 ]; then
    step "6. package: $OUT -> $OUT-private.tar.gz, -public.tar.gz"
    bash "$REPO/build/package.sh" "$OUT"
fi

step "done: $OUT ($(( $(date +%s) - T0 ))s)${MODERN:+ (the modern build)} -- play it with $OUT/linux-run-avp2.sh or linux-run-ph.sh, or test it with make test"
