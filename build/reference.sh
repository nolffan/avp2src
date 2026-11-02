#!/usr/bin/env bash
# Extract the retail binaries into out/reference/, with their function and
# vtable tables (<stem>-functions.csv, <stem>-vtables.csv), for the checks
# that compare our build against them: from Primal Hunt (build/retail.py),
# the engine (lithtech.exe, server.dll, d3d.ren, ltmsg.dll, cdaudio.dll),
# the launcher PrimalHunt.exe with Launch.dll and AVP2XUp.dll, the
# dedicated server AVP2XServ.exe and its game DLLs (PHCShell.dll,
# PHObject.lto, PHCRes.dll, PHSRes.dll); from AvP2 1.0.9.6
# (build/retail_avp2.py), its launcher AVP2.exe with AVP2Up.dll, its
# dedicated server AVP2Serv.exe and its game DLLs (CShell.dll, Object.lto,
# CRes.dll, SRes.dll). Nothing in the build reads them, and nothing in
# out/reference/ is ever run.
#
#   build/reference.sh            (in the build container, or on a host with
#                                  Python 3; no Wine involved)
set -e
cd "$(dirname "${BASH_SOURCE[0]}")/.."
source build/retail.sh
python3 tools/retailextract.py reference "$(retail_dir)" out/reference
RETAIL_LAYOUT=build/retail_avp2.py \
    python3 tools/retailextract.py reference "$(retail_avp2_dir)" out/reference
# ... and AvP2 1.0.9.6's own engine, which AvP2's game DLLs need (Primal
# Hunt's has an ILTCommon slot more: docs/PLAN.md, package 10), under
# out/reference/avp2/ beside Primal Hunt's of the same names
mkdir -p out/reference/avp2
for f in lithtech.exe server.dll d3d.ren ltmsg.dll; do
    cp "$(retail_avp2_dir)/$f" out/reference/avp2/$f
    echo "== $(basename "$(retail_avp2_dir)"):$f -> out/reference/avp2/$f"
done

# The function table of each image (tools/enumfuncs.py --csv): addr, size,
# how the start was found, and the name an error-tag literal or export gives
# it. The reconstruction tools read these (fndiff.py's sizes, tagmap.py,
# uncovered.py, ...); like the images, they are derived, never committed.
for pair in server:server.dll lithtech:lithtech.exe d3d:d3d.ren cdaudio:cdaudio.dll \
            PrimalHunt:PrimalHunt.exe Launch:Launch.dll AVP2XServ:AVP2XServ.exe \
            PHCShell:PHCShell.dll PHObject:PHObject.lto \
            AVP2:AVP2.exe AVP2Serv:AVP2Serv.exe CShell:CShell.dll Object:Object.lto \
            AVP2lithtech:avp2/lithtech.exe AVP2server:avp2/server.dll AVP2d3d:avp2/d3d.ren; do
    python3 tools/enumfuncs.py "out/reference/${pair#*:}" \
        --csv "out/reference/${pair%%:*}-functions.csv" | tail -1
done
# ... and its vtables, one row per slot (tools/vtabaudit.py), which
# tools/vtabcheck.py compares our linked images against.
for pair in server:server.dll lithtech:lithtech.exe AVP2server:avp2/server.dll AVP2lithtech:avp2/lithtech.exe; do
    python3 tools/vtabaudit.py "out/reference/${pair#*:}" \
        --csv="out/reference/${pair%%:*}-vtables.csv" | tail -1
done
