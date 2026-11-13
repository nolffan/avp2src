#!/usr/bin/env bash
# measure.sh <name> <client cfg> [server bandwidth]: a dedicated server
# (the modern debug build's), a watching client and a running client;
# the watcher's netpos.log analysed
set -u
N=$1; CFG=$2; BW=${3:-}
W="$(cd "$(dirname "$0")/../.." && pwd)"
G=out/avp2-modern-debug
docker kill mp-srv mp-c1 mp-c2 >/dev/null 2>&1; sleep 2
DOCK_ENV="-e SRV_DLL=/repo/out/reference/avp2/server.dll ${BW:+-e SRV_BW=$BW}" $W/build/mptest/dock.sh mp-srv bash out/mp/srv.sh /repo/$G/AVP2 m-$N 330 1 > $W/build/mptest/srv-m-$N.out 2>&1 &
sleep 25
DOCK_ENV="-e PLAYTEST_NAME=m-$N-watch -e PLAYTEST_SCRIPT=build/mptest/c-watcher.txt -e PLAYTEST_CFG=/repo/$CFG" timeout 400 $W/build/mptest/dock.sh mp-c2 bash build/playtest.sh $G avp2 - 0 > $W/build/mptest/c2-m-$N.out 2>&1 &
sleep 20
DOCK_ENV="-e PLAYTEST_NAME=m-$N-run -e PLAYTEST_SCRIPT=build/mptest/c-runner.txt -e PLAYTEST_CFG=/repo/$CFG" timeout 400 $W/build/mptest/dock.sh mp-c1 bash build/playtest.sh $G avp2 - 0 > $W/build/mptest/c1-m-$N.out 2>&1
wait
python3 -I $W/build/mptest/netpos.py $W/out/play/m-$N-watch/AVP2/netpos.log
