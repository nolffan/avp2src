#!/usr/bin/env bash
# srv.sh <game dir (…/AVP2 or …/PrimalHunt)> <run name> <seconds> <lan 0|1> [server args...]
# A dedicated server (AVP2Serv.exe / AVP2XServ.exe) under Xvfb + Wine, in
# out/mp/run/<run name>/ (binaries copied, data linked), on the config
# ServerData/mptest.txt: deathmatch, port 27888, one world. Writes wine.log,
# shot.png and (modern) server-console.log there.
set -u
GAME="$1"; NAME="$2"; SECS="$3"; LAN="$4"; shift 4
REPO=/repo
RUN="$REPO/out/mp/run/$NAME"
rm -rf "$RUN"; mkdir -p "$RUN/ServerData"
for f in "$GAME"/*; do
    b=$(basename "$f")
    case "$b" in ServerData|Save|Profiles|autoexec.cfg) continue ;; esac
    if [ -f "$f" ] && [ "$(head -c 2 "$f" | tr -d '\0')" = MZ ]; then cp "$f" "$RUN/$b"; else ln -s "$f" "$RUN/$b"; fi
done
cp "$GAME/autoexec.cfg.retail" "$RUN/autoexec.cfg"
[ -n "${SRV_DLL:-}" ] && cp "$SRV_DLL" "$RUN/server.dll"
# SRV_EXE: another server program; SRV_CUSTOM: a directory to use as custom/
[ -n "${SRV_EXE:-}" ] && cp "$SRV_EXE" "$RUN/$(basename "$SRV_EXE")"
[ -n "${SRV_CUSTOM:-}" ] && rm -f "$RUN/custom" && ln -s "$SRV_CUSTOM" "$RUN/custom"
mkdir -p "$RUN/Save" "$RUN/Profiles"
# AvP2's server reads no custom/ directory unless customrez.txt names it
[ -f "$GAME/AVP2Serv.exe" ] && [ ! -f "$GAME/customrez.txt" ] && printf '[Rez]\r\nRez0 = "custom"\r\n' > "$RUN/customrez.txt"
if [ -f "$GAME/AVP2XServ.exe" ]; then EXE=AVP2XServ.exe; else EXE=AVP2Serv.exe; fi
# Primal Hunt reads AvP2's archives from ../AVP2
[ "$EXE" = AVP2XServ.exe ] && [ ! -e "$REPO/out/mp/run/AVP2" ] && ln -s "$(dirname "$GAME")/AVP2" "$REPO/out/mp/run/AVP2"
printf '[Settings]\r\nServerName = "%s"\r\nPassword = ""\r\nSCPassword = ""\r\nLocked = 0\r\nDedicated = 1\r\nLANConnection = %s\r\nGameType = 1\r\nPortID = 27888\r\n%s\r\n[Deathmatch]\r\nMaxTotalPlayers = 8\r\nFragLimit = 0\r\nScoreLimit = 0\r\nTimeLimit = 0\r\nMap00 = "%s"\r\n' \
    "${SRV_NAME:-mptest-$NAME}" "$LAN" "${SRV_BW:+Bandwidth = $SRV_BW
Bandwidth30 = 1
}" "${SRV_MAP:-Worlds\\Multi\\DM\\DM_Compound}" > "$RUN/ServerData/mptest.txt"
# Primal Hunt's servers list the classes they allow; allow the first 31
if [ "$EXE" = AVP2XServ.exe ]; then
    printf '\r\n[Class]\r\nnum_classes = 31\r\n' >> "$RUN/ServerData/mptest.txt"
    for i in $(seq 0 30); do printf 'Name%d = "c%d"\r\nAllowed%d = 1\r\nID%d = %d\r\n' $i $i $i $i $i >> "$RUN/ServerData/mptest.txt"; done
fi
REGKEY='HKLM\SOFTWARE\Monolith Productions\Aliens vs. Predator 2\1.0'
cat > "$RUN/runner.sh" <<EOF
cd "$RUN"
# Primal Hunt's server finds AvP2's archives by AvP2's install key (removed after)
[ "$EXE" = AVP2XServ.exe ] && wine reg add '$REGKEY' /v InstallDir /d 'Z:$(dirname "$GAME" | tr / '\\\\')\\AVP2' /f >/dev/null 2>&1
wine $EXE -config mptest $* > wine.log 2>&1 &
P=\$!
for i in \$(seq 1 $SECS); do sleep 1; kill -0 \$P 2>/dev/null || break; done
import -window root shot.png 2>/dev/null
kill -0 \$P 2>/dev/null && echo ALIVE > result.txt || echo EXITED > result.txt
[ "$EXE" = AVP2XServ.exe ] && wine reg delete 'HKLM\SOFTWARE\Monolith Productions' /f >/dev/null 2>&1
wineserver -k; sleep 2
EOF
cd "$RUN" || exit 1
export WINEDEBUG=${SRV_WINEDEBUG:--all,+seh} WINEDLLOVERRIDES=winedbg.exe=d
xvfb-run -a -s "-screen 0 800x600x24" bash runner.sh
cat result.txt
