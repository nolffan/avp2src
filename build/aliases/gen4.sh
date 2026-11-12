#!/bin/bash
# gen4.sh <suffix> [mkaliases options]: check the four images against out/aliases/<img>.<suffix> scores and rewrite their alias files
W=$(cd "$(dirname "$0")/../.." && pwd)
suf=$1; shift
for i in lt:ph:lithtech:"Primal Hunt's lithtech.exe" alt:avp2:lithtech:"AvP2 1.0.9.6's lithtech.exe" sv:ph:server:"Primal Hunt's server.dll" asv:avp2:server:"AvP2 1.0.9.6's server.dll"; do
  IFS=: read im g d label <<< "$i"
  rsp=$W/src/engine/$d/link/aliases-$g.rsp
  python3 -I $W/build/aliases/aliascheck.py $im out/aliases/$im.$suf --also=$rsp --out=$W/out/aliases/chk.$im.$suf.tsv
  python3 -I $W/build/aliases/mkaliases.py $W/out/aliases/chk.$im.$suf.tsv "$label" $rsp --byhand=$W/build/aliases/byhand.$im.rsp --skip=$W/build/aliases/skip.$im.txt "$@" 2> $W/out/aliases/drop.$im.txt
  head -1 $W/out/aliases/drop.$im.txt
done
