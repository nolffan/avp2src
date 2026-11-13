# Multiplayer tests (docs/MODERN.md, M-17..M-20)

Scripts for testing the modern build's multiplayer in containers: a server
and two clients, each under Xvfb and Wine.

- `dock.sh` -- run a command in the build image with this checkout at /repo
  (the extra docker options in `DOCK_ENV`).
- `srv.sh` -- start a dedicated server (AVP2Serv.exe or AVP2XServ.exe) in
  `out/mp/run/<name>/`, its binaries copied and its data linked.
- `masterstub.py` -- a local GameSpy master: heartbeat, `\secure\` challenge
  and `\validate\` check, list request, status queries.
- `gsprobe.py` -- one validated list request to a master (e.g.
  master.openspy.net), to see whether a game is registered and listed.
- `measure.sh` -- a server, one client running in circles and one watching:
  how smoothly the other player moves (with `netpos.py`, which reads the
  watcher's `netpos.log`, written by the modern debug build's `NetPosLog`).
- `c-*.txt` -- the clients' playtest scripts (build/playtest.sh's format).

Until our own server.dll loads a world (docs/MODERN.md, M-17..M-20, open
items), the servers run on retail's server.dll (`SRV_DLL`).
