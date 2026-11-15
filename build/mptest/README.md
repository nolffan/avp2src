# Multiplayer tests (docs/MODERN.md, M-17..M-20)

These scripts test the multiplayer of the modern build in containers. There
is a server and two clients, each under Xvfb and Wine.

- `dock.sh` -- runs a command in the build image, with this checkout at
  /repo. `DOCK_ENV` gives the extra docker options.
- `srv.sh` -- starts a dedicated server (AVP2Serv.exe or AVP2XServ.exe) in
  `out/mp/run/<name>/`. It copies the binaries of the server and links its
  data.
- `masterstub.py` -- a local GameSpy master. It does the heartbeat, the
  `\secure\` challenge and the `\validate\` check, the list request, and
  the status queries.
- `gsprobe.py` -- sends one validated list request to a master (for
  example master.openspy.net). Use it to see if a game is registered and
  listed.
- `measure.sh` -- starts a server, one client that runs in circles and one
  client that watches. It measures how smoothly the other player moves. It
  uses `netpos.py`, which reads the `netpos.log` of the watcher. The
  `NetPosLog` of the modern debug build writes this log.
- `c-*.txt` -- the playtest scripts of the clients, in the format of
  build/playtest.sh.

Until our own server.dll loads a world (docs/MODERN.md, M-17..M-20, open
items), the servers use the retail server.dll (`SRV_DLL`).
