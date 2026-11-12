#!/bin/bash
# gen4.sh: redraft the four engine images' full-length vtables
# (src/engine/{lithtech,server}/link/vtables-{ph,avp2}.txt) from the linked
# images, their scores and the shipped tables (build/vtables/mkvtables.py).
# Run from anywhere, after a link of all four; the drafts land in
# out/vtables/ and replace the spec files only when nothing is MISSING.
W=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p "$W/out/vtables"
rc=0
for i in lithtech:ph:"lithtech.exe" lithtech:avp2:"AvP2 lithtech.exe" server:ph:"server.dll" server:avp2:"AvP2 server.dll"; do
  IFS=: read d g label <<< "$i"
  link=$W/src/engine/$d/link
  python3 "$W/tools/vtabcheck.py" "$label" --all --aliases="$link/aliases-$g.rsp" > "$W/out/vtables/vtabcheck.$d-$g.txt" 2>&1
  if python3 "$W/build/vtables/mkvtables.py" "$label" "$W/out/vtables/vtabcheck.$d-$g.txt" "$W/out/vtables/vtables-$d-$g.txt" --map="$W/build/vtables/map-$d-$g.txt"; then
    cp "$W/out/vtables/vtables-$d-$g.txt" "$link/vtables-$g.txt"
  else
    rc=1
  fi
done
exit $rc
