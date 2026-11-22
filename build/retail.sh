# Sourced by build/all.sh, build/reference.sh and the Makefile: the retail
# games the build takes its data from, installed from your own images of
# their three discs by build/discs.py, and whether they are the right ones:
#
#   Primal Hunt (Europe) 1.0.2.2, the last release, whose engine, launcher,
#     dedicated server and game DLLs are the newest (build/retail.py); and
#   Aliens versus Predator 2 (Europe) with the English 1.0.9.6 patch, its
#     last, whose game DLLs, launcher and dedicated server play AvP2's own
#     campaigns, and which Primal Hunt needs installed beside it
#     (build/retail_avp2.py). The patch is a file on the Primal Hunt disc.
#
# AvP2 had no GOTY edition.
#
# RETAIL_RELEASE       the install directory of Primal Hunt
# RETAIL_AVP2_RELEASE  the install directory of AvP2
# AVP2_RETAIL          where both are installed: out/retail/ by default
#                      (build/variant.sh sets it to the checkout's, for its
#                      tree)
# retail_dir           prints out/retail/avp2-ph-eu, if it is installed
# retail_avp2_dir      prints out/retail/avp2-eu-1.0.9.6, if it is installed
# retail_install  installs both from the disc images in isos/ (or
#                 $AVP2SRC_ISOS), unless they are installed already from the
#                 same build/retail-files.txt (`make retail` runs it; every
#                 build does)
# retail_verify   checks every installed file against build/retail-files.txt
#                 (the Makefile runs it before every build;
#                 AVP2_SKIP_RETAIL_CHECK=1 skips it)
RETAIL_RELEASE=avp2-ph-eu
RETAIL_AVP2_RELEASE=avp2-eu-1.0.9.6

retail_repo() { (cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd); }

retail_root() { echo "${AVP2_RETAIL:-$(retail_repo)/out/retail}"; }

_retail_check() {  # <release> <file>...: prints its directory if installed
    local root d rel=$1 f
    shift
    root="$(retail_root)"
    d="$root/$rel"
    if [ -f "$root/.retail-files" ]; then
        for f in "$@"; do [ -f "$d/$f" ] || d=; done
        [ -n "$d" ] && { echo "$d"; return 0; }
    fi
    echo "error: the retail game $rel is not installed in $root/$rel" >&2
    echo "       run make retail (it needs your three disc images in isos/: see README.md, Building)" >&2
    return 1
}

retail_dir() { _retail_check "$RETAIL_RELEASE" lithtech.exe PrimalHunt.exe AVP2X.REZ; }
retail_avp2_dir() { _retail_check "$RETAIL_AVP2_RELEASE" lithtech.exe AVP2.exe AVP2P1.REZ; }

retail_install() {
    local root list
    root="$(retail_root)"
    list="$(retail_repo)/build/retail-files.txt"
    if [ -f "$root/.retail-files" ] && cmp -s "$root/.retail-files" "$list"; then
        return 0
    fi
    echo "== installing $RETAIL_AVP2_RELEASE and $RETAIL_RELEASE from your disc images (build/discs.py, once)"
    python3 "$(retail_repo)/build/discs.py" install "$root"
}

retail_verify() {
    local root
    root="$(retail_root)"
    retail_dir >/dev/null && retail_avp2_dir >/dev/null || return 1
    [ "${AVP2_SKIP_RETAIL_CHECK:-0}" = 1 ] && return 0
    # a stamp skips re-hashing 1.7 GB on every build while the files are unchanged
    local stamp key
    stamp="$(retail_repo)/out/.retail-verified"
    key="$( { cat "$(retail_repo)/build/retail-files.txt";
              cd "$root" && find "$RETAIL_RELEASE/" "$RETAIL_AVP2_RELEASE/" -type f -printf '%p %s %T@\n' | sort; } |
            sha256sum | cut -d' ' -f1)"
    if [ -f "$stamp" ] && [ "$(cat "$stamp")" = "$key" ]; then
        return 0
    fi
    echo "== checking $root against build/retail-files.txt"
    if ! python3 "$(retail_repo)/build/discs.py" verify "$root"; then
        echo "error: the installed retail games do not match build/retail-files.txt." >&2
        echo "       Install them again: rm -rf $root && make retail" >&2
        return 1
    fi
    mkdir -p "$(dirname "$stamp")" && echo "$key" > "$stamp"
}
