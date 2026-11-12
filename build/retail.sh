# Sourced by build/all.sh, build/reference.sh and the Makefile: the retail
# games the build takes its data from, installed by the ext/rez/ submodule from
# the redump images of their discs (ext/rez/README.md), and whether they are the
# right ones:
#
#   Primal Hunt (Europe) 1.0.2.2, the last release, whose engine, launcher,
#     dedicated server and game DLLs are the newest (build/retail.py); and
#   Aliens versus Predator 2 (Europe) with the English 1.0.9.6 patch, its
#     last, whose game DLLs, launcher and dedicated server play AvP2's own
#     campaigns, and which Primal Hunt needs installed beside it
#     (build/retail_avp2.py).
#
# AvP2 had no GOTY edition.
#
# RETAIL_RELEASE       ext/rez/'s name for Primal Hunt's
# RETAIL_AVP2_RELEASE  ext/rez/'s name for AvP2's
# retail_dir           prints ext/rez/games/avp2-ph-eu, if it is installed
# retail_avp2_dir      prints ext/rez/games/avp2-eu-1.0.9.6, if it is installed
# retail_install  fetches the disc images from archive.org and installs
#                 both from them (and AvP2's patch, ext/rez/patches/), unless
#                 they are installed already
#                 (ext/rez/fetch-isos.sh, ext/rez/install-games.sh; `make` runs it)
# retail_discs    fetches the disc images the public archive's install
#                 scripts are made against, unless they are there: both
#                 games' European and USA releases' (build/mkinstall.py)
# retail_verify   checks every installed file against ext/rez/games.sha256 (the
#                 Makefile runs it before every build;
#                 AVP2_SKIP_RETAIL_CHECK=1 skips it)
RETAIL_RELEASE=avp2-ph-eu
RETAIL_AVP2_RELEASE=avp2-eu-1.0.9.6

retail_rez() {
    local here
    here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
    if [ ! -f "$here/ext/rez/install-games.sh" ]; then
        echo "error: ext/rez/ is empty -- the disc images and their installer are the ext/rez/" >&2
        echo "       submodule: run git submodule update --init (see ext/rez/README.md)" >&2
        return 1
    fi
    echo "$here/ext/rez"
}

_retail_check() {  # <release> <file>...: prints its directory if installed
    local rez d rel=$1 f
    shift
    rez="$(retail_rez)" || return 1
    d="$rez/games/$rel"
    if [ -f "$d/MANIFEST.txt" ]; then
        for f in "$@"; do [ -f "$d/$f" ] || d=; done
        [ -n "$d" ] && { echo "$d"; return 0; }
    fi
    echo "error: the retail game $rel is not installed in $rez/games/$rel" >&2
    echo "       run make retail (or, in make shell, build/retail.sh's retail_install)" >&2
    return 1
}

retail_dir() { _retail_check "$RETAIL_RELEASE" lithtech.exe PrimalHunt.exe AVP2X.REZ; }
retail_avp2_dir() { _retail_check "$RETAIL_AVP2_RELEASE" lithtech.exe AVP2.exe AVP2P1.REZ; }

retail_install() {
    local rez rel
    rez="$(retail_rez)" || return 1
    for rel in "$RETAIL_AVP2_RELEASE" "$RETAIL_RELEASE"; do
        [ -f "$rez/games/$rel/MANIFEST.txt" ] && continue
        echo "== fetching and installing $rel (ext/rez/: from archive.org, once)"
        "$rez/fetch-isos.sh" "$rel" && "$rez/install-games.sh" "$rel" || return 1
    done
}

RETAIL_DISCS="avp2-eu avp2-ph-eu avp2-usa avp2-ph-usa"
retail_discs() {
    local rez name missing=0
    rez="$(retail_rez)" || return 1
    while IFS= read -r name; do
        [ -f "$rez/isos/$name.iso" ] || missing=1
    done < <(python3 -I "$rez/tools/install.py" --discs $RETAIL_DISCS)
    [ $missing = 0 ] && return 0
    echo "== fetching the disc images of $RETAIL_DISCS (ext/rez/: the USA releases' are about 1.3 GB more, once)"
    "$rez/fetch-isos.sh" $RETAIL_DISCS
}

retail_verify() {
    local rez d rel
    rez="$(retail_rez)" || return 1
    retail_dir >/dev/null && retail_avp2_dir >/dev/null || return 1
    [ "${AVP2_SKIP_RETAIL_CHECK:-0}" = 1 ] && return 0
    # a stamp skips re-hashing 1.7 GB on every build while the files are unchanged
    local stamp key
    stamp="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/out/.retail-verified"
    key="$(cd "$rez/games" && find "$RETAIL_RELEASE/" "$RETAIL_AVP2_RELEASE/" -type f -printf '%p %s %T@\n' | sort | sha256sum | cut -d' ' -f1)"
    if [ -f "$stamp" ] && [ "$(cat "$stamp")" = "$key" ]; then
        return 0
    fi
    for rel in "$RETAIL_AVP2_RELEASE" "$RETAIL_RELEASE"; do
        echo "== checking $rez/games/$rel against ext/rez/games.sha256"
        if ! python3 -I "$rez/tools/install.py" --verify "$rez/games" "$rel"; then
            echo "error: the installed $rel does not match ext/rez/games.sha256." >&2
            echo "       Reinstall it: ext/rez/install-games.sh --force $rel" >&2
            return 1
        fi
    done
    mkdir -p "$(dirname "$stamp")" && echo "$key" > "$stamp"
}
