#!/usr/bin/env bash
# The one-function loop: recompile one object and compare one of its
# functions with the retail one.
#
#   build/near.sh <target> <retail address> <object> <symbol substring>
#                                     (on the host; runs in the build container)
#
#   build/near.sh server.dll 0x10024f57 src/engine/server/Release/commonmodel.obj GetWeightSet
#
# <target> is a progress/targets.py label or stem (its retail image is the
# one compared against). The object is rebuilt through build/build.mk
# (build/compile.sh <object>), in the release tree as every build is, and
# ext/tools/fndiff.py prints what still differs. The worklist is
# `ext/tools/progress.py --nears=<target>` (ext/tools/guides/10_upgrading_nears.md).
set -e
cd "$(dirname "${BASH_SOURCE[0]}")/.."
[ $# -eq 4 ] || { sed -n '2,15p' "$0" >&2; exit 2; }
target=$1 addr=$2 obj=$3 sym=$4
# (the target's own function table: AvP2's images share their file names
# with Primal Hunt's, which fndiff.py would find first)
read -r image stem < <(python3 -I -c "
import sys; sys.path.insert(0, 'ext/tools'); import tgt
t = tgt.find(sys.argv[1]); print(t['shipped'], t['stem'])" "$target")
IMAGE=${IMAGE:-avp2src-build}
docker run --rm --init -u "$(id -u):$(id -g)" -e HOME=/tmp/home -v "$PWD:/repo" -w /repo "$IMAGE" bash -c '
    bash build/compile.sh "$1" | grep -E " error |fatal error" || true
    python3 ext/tools/fndiff.py "$2" "$1" "$3" --image="$4" --csv="out/reference/$5-functions.csv"' _ "$obj" "$addr" "$sym" "$image" "$stem"
