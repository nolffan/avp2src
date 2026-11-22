# The retail art we commit

This directory holds one set of files: `art/`, the retail art that two of
our programs are compiled with. These files are retail data. They are
unchanged, as the resources of the retail binaries hold them.

With these files, the repository holds this retail data, and no other:

- `assets/art/` (below);
- the resources that `src/` already holds: Monolith's source release has
  the icons and bitmaps of `AVP2Serv.exe` and the cursors of `CRes.dll`;
  `src/AVP2/AVP2Up/res/` holds the icons and bitmaps of the Sierra
  updater's resource DLLs (`AVP2Up.dll`, `AVP2XUp.dll`), and
  `src/engine/ltmsg/ltmsg.rc` holds the engine's message strings
  (`docs/LICENSE_NOTES.md`);
- Monolith's prebuilt `lithshared` libraries in `progress/reference/`, for
  comparison only.

All other game data comes only from the player's own discs. The modern
build changes no retail data file, so there is no `assets/modern/`.

## art/

About 12 MB:

| Directory | What | Compiled into |
|---|---|---|
| `art/launcher/avp2/` | the art of the AvP2 launcher: its two icons (`launcher.ico`, `launcher2.ico`), its cursor (`launcher.cur`), its bitmaps (the background and the buttons, `launcher_*.bmp`), its sounds (`launcher_WAVE_*.bin`) and its animation (`launcher_AVI_221.bin`) | `AVP2.exe`, and the modern build's `AVP2Launcher.exe` |
| `art/launcher/ph/` | the same files of the Primal Hunt launcher | `PrimalHunt.exe`, and the modern build's `PrimalHuntLauncher.exe` |
| `art/avp2xserv/` | the splash bitmap of the Primal Hunt dedicated server (`splash.bmp`) | `AVP2XServ.exe` |

The source of each set is a retail binary: `AVP2.exe` (AvP2 1.0.9.6),
`PrimalHunt.exe` and `AVP2XServ.exe` (Primal Hunt 1.0.2.2). Monolith's
source release does not have this art: `src/launcher/launcher.rc` and
`src/AVP2/AVP2Serv/ServerApp.rc` (with `PRIMAL_HUNT`) include it from
their `res\` directories. It is committed so that every build compiles
complete binaries, also a build without the discs (`NODISCS=1`, the CI
build). Thus the binaries in the public archives are complete, and the
same as in a build with the discs.

- Every build copies the files into the directories that the resource
  scripts include from (`build/build.mk`: `src/launcher/<dir>/res/`,
  `src/AVP2/AVP2Serv/PrimalHunt/res/`).
- `make art` writes them again from the retail binaries (`build/art.py
  write`, with `ext/tools/rsrc2rc.py`).
- `make art-check`, and every build with the retail games, compares them
  with the retail binaries, and fails if they are different.

Do not edit these files.
