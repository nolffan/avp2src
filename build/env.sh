# Source this to get the Visual C++ 6.0-under-Wine build environment. It runs
# inside the build container (docker/Dockerfile; `make shell` gives you one),
# where Wine is on PATH and the toolchains are ext/toolchains/ in this checkout
# (ext/toolchains/README.md). The compile and link command lines themselves are
# build/build.mk's.

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
for f in msvc-6.00-8168/BIN/cl.exe msvc-6.00-8966/BIN/link.exe dxsdk-8.1/include/d3d8.h; do
    if [ ! -f "$REPO/ext/toolchains/$f" ]; then
        echo "error: ext/toolchains/$f not found -- the toolchain is missing (ext/toolchains/README.md)" >&2
        return 1 2>/dev/null || exit 1
    fi
done
if ! command -v wine >/dev/null 2>&1; then
    echo "error: wine is not on PATH -- run this inside the build container (make shell)" >&2
    return 1 2>/dev/null || exit 1
fi
export WINEDEBUG=-all
export WINEDLLOVERRIDES="mscoree,mshtml="
# the variant macros ext/tools/releasesrc.py resolves, the directories the
# project files put their outputs in (every one of them a build output,
# never a source), and the project's own header directory ext/tools/mkdeps.py
# follows
export RELEASESRC_VARIANTS="AVP2SRC_MODERN AVP2SRC_DEBUG"
export RELEASESRC_OUTPUT_DIRS="built final release release8168 clientshelldll___win32_final object___win32_final primalhunt avp2release libs"
# ... and Launch.dll's Debug build (not every Debug/: STLport has a source
# directory of that name), and the libraries build/build.mk puts in the
# dedicated server's project directory (server.lib, and AVP2Serv.exe's own
# builds of ButeMgr, CryptMgr and StdLith)
export RELEASESRC_GENERATED='^(launch/Debug/|AVP2/AVP2Serv/[^/]+\.lib$)'
export MKDEPS_PROJECT_INCLUDE=src/avp2src
# ... and where the drives the compilers see are (build/build.mk: src/ is S:,
# ext/toolchains/ T:), for the /I directories the project files name on S:
export MKDEPS_DRIVES="s=src t=ext/toolchains"

# Extra flags for every compile and every link, for the debug and modern
# builds (build/variant.sh sets them). Empty otherwise -- and the release
# build's command lines are then exactly what the project files say.
export AVP2SRC_CFLAGS="${AVP2SRC_CFLAGS:-}"
export AVP2SRC_LDFLAGS="${AVP2SRC_LDFLAGS:-}"
export AVP2SRC_RCFLAGS="${AVP2SRC_RCFLAGS:-}"    # resource scripts: /d NAME
# They are only allowed in the trees build/variant.sh makes (out/build-*/):
# anywhere else they would put AVP2SRC_DEBUG or AVP2SRC_MODERN code into the
# objects `make progress` scores, which must only ever be the legacy build.
if [ -n "$AVP2SRC_CFLAGS$AVP2SRC_LDFLAGS$AVP2SRC_RCFLAGS" ]; then
    case "$REPO" in
        */out/build-*) ;;
        *) echo "error: AVP2SRC_CFLAGS / LDFLAGS / RCFLAGS are set, but this is not a" >&2
           echo "       build/variant.sh tree ($REPO). Unset them; use make debug/modern." >&2
           return 1 2>/dev/null || exit 1 ;;
    esac
fi

# The legacy build compiles src/ WITHOUT its AVP2SRC_MODERN / AVP2SRC_DEBUG
# blocks, not merely with them switched off: a block the compiler skips
# still moves everything below it, and VC6 lets the file's layout decide
# where some out-of-line inline functions go, so enough skipped blocks can
# change the retail binary (ext/tools/releasesrc.py; nolfsrc's lesson).
# build/compile.sh calls this first, with its own arguments: in a checkout it
# brings out/release-tree/ up to date -- src/ with those blocks resolved as
# the legacy build sees them, the same build/ and ext/tools/, this checkout's
# build outputs -- runs the script there, copies the build outputs back into
# this checkout's src/ (where `make progress` and the checks read them) and
# exits with the script's status. In a build/variant.sh tree (out/build-*/,
# which must see the blocks), or inside the release tree, it does nothing.
avp2src_release_tree() {
    [ -n "${AVP2SRC_IN_RELEASE_TREE:-}" ] && return 0
    case "$REPO" in */out/build-*) return 0 ;; esac
    local tree="$REPO/out/release-tree" script rc=0
    script="$(basename "${BASH_SOURCE[1]}")"
    mkdir -p "$tree"
    python3 "$REPO/ext/tools/releasesrc.py" sync "$REPO/src" "$tree/src" || exit 1
    python3 "$REPO/ext/tools/releasesrc.py" outputs "$REPO/src" "$tree/src" || exit 1
    rm -rf "$tree/build" "$tree/tools" "$tree/ext/tools"; rm -f "$tree/toolchains" "$tree/rez"
    tar -C "$REPO" -cf - --exclude='__pycache__' --exclude=.git build ext/tools | tar -C "$tree" -xf -
    [ -L "$tree/progress" ] || ln -s ../../progress "$tree/progress"
    [ -L "$tree/ext/toolchains" ] || ln -s ../../../ext/toolchains "$tree/ext/toolchains"
    [ -L "$tree/ext/rez" ] || ln -s ../../../ext/rez "$tree/ext/rez"
    ( cd "$tree" && AVP2SRC_IN_RELEASE_TREE=1 bash "build/$script" "$@" ) || rc=$?
    python3 "$REPO/ext/tools/releasesrc.py" outputs "$tree/src" "$REPO/src" || exit 1
    exit $rc
}

# Compile and link through build/build.mk, in this tree (run it after
# avp2src_release_tree): every listed target, AVP2SRC_JOBS at a time
# (default: one per CPU), each job's output kept together.
avp2src_make() {
    make -C "$REPO" -f build/build.mk --no-print-directory -Otarget \
        -j"${AVP2SRC_JOBS:-$(nproc)}" "$@"
}

# ---------------------------------------------------------------------------
# The tools by hand, for the one-object loop and experiments: the compiler,
# linker, librarian and resource compiler build/build.mk uses for the game
# DLLs, in the same environment -- src/ as drive S:, ext/toolchains/ as T:, the
# DirectX 8.1 SDK first on the INCLUDE and LIB paths -- so a command line
# copied out of an object's <object>.cmd gives the same object. (A compile's
# .cmd also starts with `cd <dir> &&`: run it from that directory.) They use
# their own Wine prefix, out/tools-wine, so they never touch the build's.
#
#   run_cl /nologo /O2 /c foo.cpp       cl 12.00.8168 (VC6 RTM), or CL_REV=8966 ...
#   run_link, run_lib, run_rc           link/lib 6.00.8447 (VC6 SP5's), rc (SP6's)
#   avp2src_wpath <path>                the Windows path the tools see
TC="$REPO/ext/toolchains"
DXSDK="$TC/dxsdk-8.1"
VC6="$TC/msvc-6.00-8966"
avp2src_wpath() {
    local p
    p="$(realpath -m "$1")"
    case "$p" in
        "$REPO/src"|"$REPO/src"/*)      p="s:${p#"$REPO/src"}" ;;
        "$TC"|"$TC"/*)                  p="t:${p#"$TC"}" ;;
        *)                              p="z:$p" ;;
    esac
    echo "${p//\//\\}"
}
avp2src_tools_env() {   # the prefix, made the first time, with S: and T:
    export WINEPREFIX="$REPO/out/tools-wine"
    if [ ! -f "$WINEPREFIX/.ready" ]; then
        mkdir -p "$WINEPREFIX"
        wine wineboot -i >/dev/null 2>&1 || true
        wineserver -w || true
        touch "$WINEPREFIX/.ready"
    fi
    ln -sfn "$REPO/src" "$WINEPREFIX/dosdevices/s:"
    ln -sfn "$TC" "$WINEPREFIX/dosdevices/t:"
    local v d
    v="$(avp2src_wpath "$VC6")"; d="$(avp2src_wpath "$DXSDK")"
    export WINEPATH="$v\\BIN"
    export INCLUDE="$d\\include;$v\\INCLUDE;$v\\MFC\\INCLUDE"
    export LIB="$d\\lib;$v\\LIB;$v\\MFC\\LIB"
}
# The variant flags go after the command's own (/Oy- must follow the /O2
# that implies it), but before a /link, after which the line is the linker's.
run_cl() {
    local a linked=0 args=()
    for a in "$@"; do
        if [ $linked = 0 ] && [ "${a,,}" = "/link" ]; then
            args+=($AVP2SRC_CFLAGS); linked=1
        fi
        args+=("$a")
    done
    [ $linked = 1 ] || args+=($AVP2SRC_CFLAGS)
    ( avp2src_tools_env && wine "$TC/msvc-6.00-${CL_REV:-8168}/BIN/cl.exe" "${args[@]}" )
}
run_link() { ( avp2src_tools_env && wine "$VC6/BIN/link.exe" $AVP2SRC_LDFLAGS "$@" ); }
run_lib()  { ( avp2src_tools_env && wine "$VC6/BIN/lib.exe" "$@" ); }
run_rc()   { ( avp2src_tools_env && wine "$TC/msvc-6.00-9782/Common/MSDev98/Bin/RC.EXE" $AVP2SRC_RCFLAGS "$@" ); }
