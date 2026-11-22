# What is whose

This repository contains code from different origins under different terms.
This file tells which is which. It is not legal advice, and it grants
nothing.

## In this repository

| Path | Origin | Terms |
|---|---|---|
| `src/AVP2/` | **Monolith Productions**: *Aliens Versus Predator 2 Source Code v1.0.9.6* (July 2002). It contains the game code (ClientShellDLL, ObjectDLL, ClientRes, ServerRes, Shared) and the dedicated server (AVP2Serv). It is unmodified, except for these changes. The line endings are LF. (`credits.txt` keeps its CRLF, because `CRes.dll` embeds it byte for byte.) This project also makes these changes in place: what the retail 1.0.9.6 binaries do differently (`RETAIL:` comments); Primal Hunt's changes, which this project reconstructed from the retail Primal Hunt binaries (`PH:` comments, behind `PRIMAL_HUNT`); and the changes inside `#ifdef AVP2SRC_MODERN` blocks. These blocks are the fixes and features of the modern build, and they were written for this project. The legacy build compiles none of them | Two agreements came with the release, and both apply. One is Monolith's end-user licence, in `src/MONOLITH-README.txt` (section V). The other is the agreement of Fox Interactive and LithTech, in `src/MONOLITH-EULA.doc`. Read them. Among other things, they restrict redistribution of the source. They require that add-ons are free and that add-ons say Monolith does not make or support them. Fox's licence was granted for a term (to December 31, 2003 at the latest), and it was revocable |
| `src/LT2/` | **Monolith Productions**: the LithTech SDK headers (`sdk/inc`) and the headers of the `lithshared` libraries (ButeMgr, StdLith, CryptMgr, RegMgr, RezMgr, RedMgr, LTGUIMgr, LithFontMgr, GameSpy, zlib, ...). It has the source of only two of these libraries: WONAPI (Sierra's WON client) and STLport. All of it is from the same release, unmodified except for line endings. The Miles and Bink binaries that came with the release (`sdk/win/lib/miles`, `sdk/win/lib/bink`) are not included. The prebuilt libraries of the release are in `progress/reference/` | The same as `src/AVP2/`. zlib is under the zlib licence, and STLport is under its own notice, as their files say |
| `src/MONOLITH-README.txt`, `src/MONOLITH-EULA.doc` | The readme of the release (`readme.txt`) and Fox's agreement (`AvP2 Source EULA.doc`). They are unmodified, except for the line endings of the readme | The same as `src/AVP2/` |
| `progress/reference/LT2/lithshared/libs/`, `progress/reference/AVP2/AVP2Serv/server.lib` | **Monolith Productions**: the prebuilt `lithshared` libraries of the same release (Release and Debug builds), and the import library of the dedicated server for `server.dll`. They are unmodified. They are never built into anything. They are used only for comparison | The same as `src/AVP2/` |
| `src/lithshared/` | The LithTech shared libraries. `ButeMgr/` and `rezmgr/` are from Monolith's public source release for *No One Lives Forever 2* (through nolf2src), adapted to AvP2's headers. The rest is from nolfsrc. Much of it (`stdlith/`, `CryptMgr/`, `RegMgr/`, `MFCStub/`, `lith/`) was adapted in nolfsrc from that same release. The rest was written or reconstructed for nolfsrc (`gamespy/`, a stand-in for the GameSpy middleware). `zlib/` is zlib | Monolith's code is under the terms of the release that it came from. zlib is under the zlib licence. The rest is the same as `src/engine/` |
| `src/engine/` | The LithTech engine (`lithtech.exe`, `server.dll`, `d3d.ren`). It is nolfsrc's reconstruction of NOLF's engine, which was written for that project from the retail NOLF binaries. This project is bringing it to Primal Hunt's engine. It implements Monolith's engine and interfaces | Written for this project and nolfsrc. No licence is declared |
| `src/launch/` | `Launch.dll`, NOLF's launcher DLL, as AvP2 and Primal Hunt ship it. It was written for this project from the retail DLL. `RezFind.cpp` follows the `RezFind.cpp` of nolfsrc's launcher, which is Monolith's code (from the launcher of the *No One Lives Forever 2* source release). `cdcheck.cpp` follows the shape of nolfsrc's `cdcheck.cpp` | `RezFind.cpp` is Monolith's code under the terms of the release that it came from. The rest was written for this project. No licence is declared |
| `src/engine/cdaudio/`, `src/lithshared/redmgr/` | `cdaudio.dll`, the CD-audio music driver of the engine, and the `Redmgr.lib` that it links. This project reconstructed them from the retail Primal Hunt `cdaudio.dll` and the prebuilt `Redmgr.lib` of the release (against the release's `redmgr.h`). The interfaces are Monolith's. `musicmgr.h` is the music-driver interface of Monolith's public *No One Lives Forever 2* source release (`runtime/shared/src/musicmgr.h`, through nolf2src). It was brought back to the interface of this engine | Monolith's interfaces are under the terms of the releases that they came from. The rest was written for this project. No licence is declared |
| `src/launcher/` | The launchers, `AVP2.exe` and `PrimalHunt.exe`. The base is the launcher source of **Monolith Productions** from the public *No One Lives Forever 2* source release (its `Game/Launcher`, through nolf2src: the dialogs and controls, `Launcher.cpp`, `RezFind.cpp`, `Utils.cpp`, and the names in `resource.h`). It was brought back to the 2001 AvP2 launcher that it descends from, with the retail `AVP2.exe` and `PrimalHunt.exe` as the evidence. `DisplayMgr.cpp` is nolfsrc's reconstruction of NOLF's display-mode code (its `dlgdisplay.cpp`, from the retail `NOLF.exe`). `ext/tools/rsrc2rc.py` reads `launcher.rc` out of the retail images (Monolith's strings and dialogs). The art is in `assets/art/launcher/` (below) | Monolith's code is under the terms of the NOLF2 release. The rest was written for this project and nolfsrc. No licence is declared |
| `src/lithshared/RegMgr32/` | The registry class of the launchers, `CRegMgr32`. It is the `Libs/RegMgr32` of **Monolith Productions** from the same NOLF2 source release (through nolf2src). It is unmodified, except for line endings | The same as that release |
| `src/engine/ltmsg/` | `ltmsg.dll`. Its resource script is read out of the retail DLL by `ext/tools/rsrc2rc.py` (Monolith's strings). Its one-line source is nolfsrc's | The strings are Monolith's. The rest was written for this project |
| `src/AVP2/AVP2Up/` | `AVP2Up.dll` and `AVP2XUp.dll`, the resources of the Sierra updater for AvP2 and Primal Hunt. Their resource scripts are read out of the retail DLLs by `ext/tools/rsrc2rc.py`. The icons and bitmaps in `res/` were extracted from the DLLs byte for byte. Those for AvP2 are **Monolith Productions**'. Those for Primal Hunt are **Third Law Interactive**'s. They are game data, as the retail DLLs ship them. The few lines of source of the DLL (an empty `DllMain`, and the `StdAfx` files of the Visual C++ wizard) were written for this project | The resources are Monolith's and Third Law's, as the data of the games. The rest was written for this project |
| `assets/art/` | **Monolith Productions** and **Third Law Interactive**: the retail art that our launchers and the Primal Hunt dedicated server are compiled with, as the resources of the retail binaries hold it: the icons, cursor, bitmaps, sounds and animation of the AvP2 launcher (`AVP2.exe`, Monolith's) and of the Primal Hunt launcher (`PrimalHunt.exe`, Third Law's), and the splash bitmap of `AVP2XServ.exe` (Third Law's). About 12 MB. They are unchanged. They are committed so that every build, also one without the discs, compiles binaries that are complete. `make art` makes them from the retail binaries, and every build with the discs checks them (`assets/README.md`). Our binaries, and thus the public archives, carry them | Monolith's and Third Law's, as the data of the games: the games' end-user licences |
| `src/avp2src/` | The project's own headers (debug logging, crash backtraces). They come from nolfsrc's headers | Written for this project. No licence is declared |
| `src/modern/binkw32/` | The `binkw32.dll` of the modern build, a Bink player on FFmpeg. It is nolfsrc's (written for that project), taken unchanged, because AvP2's engine asks the same of it | Written for this project and nolfsrc. No licence is declared |
| `build/`, `ext/tools/`, `docker/`, `progress/` (except the paths above), `Makefile`, the documentation | Written for this project | No licence is declared |
| `ext/openmiles` (submodule) | **OpenMiles**, a clean-room reimplementation of the Miles Sound System. Upstream is at github.com/maci0/openmiles. The submodule is nolfsrc's fork, github.com/nolffan/openmiles (branch `miles6-fixes`). That branch is upstream's release with fixes. The fixes are GPL-3.0-only, the same as the code that they change. OpenMiles vendors miniaudio and TinySoundFont. `ext/patches/openmiles/` holds fixes for it that are not yet on that branch | GPL-3.0-only (its vendored libraries: MIT, MIT-0, zlib). See its `LICENSE` and `deps/README.md` |
| `ext/ffmpeg` (submodule) | **FFmpeg**. Upstream is at github.com/FFmpeg/FFmpeg | LGPL-2.1-or-later, as the modern build configures it (it enables no GPL or non-free parts). See its `LICENSE.md` |

## The retail data in this repository

This repository holds this retail data, and no other:

- `assets/art/`: the retail art that our launchers and the Primal Hunt
  dedicated server are compiled with (above; `assets/README.md`).
- The resources in `src/AVP2/AVP2Up/res/` (the Sierra updater's icons and
  bitmaps) and `src/engine/ltmsg/ltmsg.rc` (the engine's message strings),
  above.
- `progress/reference/`: Monolith's prebuilt `lithshared` libraries of the
  1.0.9.6 source release, for comparison only.

All other game data comes from the player's own discs. The build names
the discs only by their redump names. It does not fetch them.

## Not in this repository, taken from the retail games at build time

The build takes these files from Primal Hunt, and from AvP2 with its
1.0.9.6 patch. `make retail` installs them into `out/retail/` from your own
images of the three discs (`build/discs.py`, with the list
`build/retail-files.txt`, which holds no game data).

| What | Origin | Where it goes |
|---|---|---|
| The game data: the `.rez` archives, the configs, the text and the movies | Monolith Productions, Third Law Interactive / Fox Interactive / Sierra | The game directories in `out/` |
| `mss32.dll` and its `.asi`, `.as6` providers | Miles Sound System, RAD Game Tools | The legacy game directories (`out/avp2-legacy-*/`) |
| `binkw32.dll`, `Smackw32.dll` | Bink and Smacker, RAD Game Tools | The legacy game directories |

`make reference` extracts the retail binaries (`build/reference.sh`) into
`out/reference/`. We compare our build against them. They never run, and
they never go into a game directory.

## Not in this repository, provided by you

The compilers in `ext/toolchains/` (Microsoft Visual C++ 6.0; see
`ext/toolchains/README.md`) are under their own licences.
`ext/toolchains/directmusic-5.3.1.904/` also holds Microsoft's DirectMusic
DLLs from the DirectX 9.0c redistributable. The modern build installs
these DLLs in its game directories (see below).

## The modern build's middleware

The modern game directories (`out/avp2-modern-release/`,
`out/avp2-modern-debug/`) have no RAD binaries. In their place, each game
has these files:

- `mss32.dll`, built from OpenMiles (GPL-3.0-only).
- `binkw32.dll`, our player (`src/modern/binkw32/`), statically linked
  with FFmpeg (LGPL-2.1-or-later).

`build/ext.sh` builds both from the `ext/` submodules. There is no Smacker
and no MP3 decoder (`docs/MODERN.md`, M-X1..M-X3).

This repository only points at those projects. The terms are important
when someone passes on a built game directory. The GPL in particular can
conflict with a directory whose other binaries are built from Monolith's
source under its end-user licence. Find the answer to this before you
distribute a modern build. The LGPL asks that a person who receives
`binkw32.dll` can rebuild it against a modified FFmpeg. `build/ext.sh` and
the source here let a person do this.

Each modern game directory also has Microsoft's DirectMusic DLLs
(`ms_dmime.dll` and the others, from
`ext/toolchains/directmusic-5.3.1.904/`; `docs/MODERN.md`, M-X4). They are
there so that the music plays under Wine and Proton. They are under the
licence of the Microsoft DirectX End-User Runtime that they come from.
Check its terms before you pass on a game directory, or a modern public
archive, that contains them. `src/modern/directmusic/lithtech.exe.manifest`
points the games at these DLLs. It was written for this project and
nolfsrc.
