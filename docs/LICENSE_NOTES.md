# What is whose

This repository mixes code from different origins under different terms.
This file says which is which; it is not legal advice and grants nothing.

## In this repository

| Path | Origin | Terms |
|---|---|---|
| `src/AVP2/` | **Monolith Productions**: *Aliens Versus Predator 2 Source Code v1.0.9.6* (July 2002): the game code (ClientShellDLL, ObjectDLL, ClientRes, ServerRes, Shared) and the dedicated server (AVP2Serv), unmodified except for line endings (LF; `credits.txt`, embedded byte for byte in `CRes.dll`, keeps its CRLF) and for the changes this project makes in place: what the retail 1.0.9.6 binaries do differently (`RETAIL:` comments), Primal Hunt's changes, reconstructed for this project from the retail Primal Hunt binaries (`PH:` comments, behind `PRIMAL_HUNT`), and changes inside `#ifdef AVP2_MODERN` blocks (the modern build's fixes and features, written for this project; the legacy build compiles none of them) | Two agreements came with the release, and both apply: Monolith's end-user licence, in `src/MONOLITH-README.txt` (section V), and Fox Interactive's and LithTech's, in `src/MONOLITH-EULA.doc`. Read them: among other things they restrict redistribution of the source, require add-ons to be free and to say they are not made or supported by Monolith, and Fox's licence was granted for a term (to December 31, 2003 at the latest) and revocable |
| `src/LT2/` | **Monolith Productions**: the LithTech SDK headers (`sdk/inc`) and the `lithshared` libraries' headers (ButeMgr, StdLith, CryptMgr, RegMgr, RezMgr, RedMgr, LTGUIMgr, LithFontMgr, GameSpy, zlib, ...), with the source of two of them only, WONAPI (Sierra's WON client) and STLport, from the same release, unmodified except for line endings. The Miles and Bink binaries that came with it (`sdk/win/lib/miles`, `sdk/win/lib/bink`) are left out; its prebuilt libraries are in `progress/reference/` | as `src/AVP2/`; zlib under the zlib licence and STLport under its own notice, as their files say |
| `src/MONOLITH-README.txt`, `src/MONOLITH-EULA.doc` | the release's readme (`readme.txt`) and Fox's agreement (`AvP2 Source EULA.doc`), unmodified but for the readme's line endings | as `src/AVP2/` |
| `progress/reference/LT2/lithshared/libs/`, `progress/reference/AVP2/AVP2Serv/server.lib` | **Monolith Productions**: the prebuilt `lithshared` libraries of the same release (Release and Debug builds), and the dedicated server's import library for `server.dll`, unmodified; never built into anything, only compared against | as `src/AVP2/` |
| `src/lithshared/` | the LithTech shared libraries: `ButeMgr/` and `rezmgr/` are from Monolith's public source release for *No One Lives Forever 2* (by way of nolf2src), brought to AvP2's headers; the rest is nolfsrc's -- much of it (`stdlith/`, `CryptMgr/`, `RegMgr/`, `MFCStub/`, `lith/`) adapted there from that same release, the rest written or reconstructed for nolfsrc (`gamespy/`, a stand-in for the GameSpy middleware); `zlib/` is zlib | Monolith's code under the terms of the release it came from; zlib under the zlib licence; the rest as `src/engine/` |
| `src/engine/` | the LithTech engine (`lithtech.exe`, `server.dll`, `d3d.ren`): nolfsrc's reconstruction of NOLF's, written for that project from the retail NOLF binaries, being brought to Primal Hunt's. It implements Monolith's engine and interfaces | written for this project and nolfsrc; no licence is declared |
| `src/launch/` | `Launch.dll`, NOLF's launcher DLL as AvP2 and Primal Hunt ship it: written for this project from the retail DLL; `RezFind.cpp` follows nolfsrc's launcher's `RezFind.cpp`, which is Monolith's (from the *No One Lives Forever 2* source release's launcher), and `cdcheck.cpp` the shape of nolfsrc's `cdcheck.cpp` | `RezFind.cpp` Monolith's code under the terms of the release it came from; the rest written for this project, no licence is declared |
| `src/engine/cdaudio/`, `src/lithshared/redmgr/` | `cdaudio.dll`, the engine's CD-audio music driver, and the `Redmgr.lib` it links: reconstructed for this project from the retail Primal Hunt `cdaudio.dll` and the release's prebuilt `Redmgr.lib` (against the release's `redmgr.h`), Monolith's interfaces. `musicmgr.h` is the music-driver interface of Monolith's public *No One Lives Forever 2* source release (`runtime/shared/src/musicmgr.h`, by way of nolf2src), brought back to this engine's | Monolith's interfaces under the terms of the releases they came from; the rest written for this project; no licence is declared |
| `src/launcher/` | the launchers, `AVP2.exe` and `PrimalHunt.exe`: **Monolith Productions**' launcher source from the public *No One Lives Forever 2* source release (its `Game/Launcher`, by way of nolf2src: the dialogs and controls, `Launcher.cpp`, `RezFind.cpp`, `Utils.cpp`, `resource.h`'s names), brought back to the 2001 AvP2 launcher it descends from, from the retail `AVP2.exe` and `PrimalHunt.exe`; `DisplayMgr.cpp` is nolfsrc's reconstruction of NOLF's display-mode code (its `dlgdisplay.cpp`, from the retail `NOLF.exe`); `launcher.rc` is read out of the retail images by `tools/rsrc2rc.py` (Monolith's strings and dialogs; the art is not in the tree) | Monolith's code under the terms of the NOLF2 release; the rest written for this project and nolfsrc; no licence is declared |
| `src/lithshared/RegMgr32/` | the launchers' registry class, `CRegMgr32`: **Monolith Productions**' `Libs/RegMgr32` from the same NOLF2 source release (by way of nolf2src), unmodified but for line endings | as that release's |
| `src/engine/ltmsg/` | `ltmsg.dll`: its resource script read out of the retail DLL by `tools/rsrc2rc.py` (Monolith's strings), and nolfsrc's one-line source | the strings Monolith's; the rest written for this project |
| `src/avp2src/` | the project's own headers (debug logging, crash backtraces), from nolfsrc's | written for this project; no licence is declared |
| `build/`, `tools/`, `docker/`, `progress/` (but the above), `Makefile`, the documentation | written for this project | no licence is declared |

## Not in this repository, taken from the retail games at build time

The build takes these from Primal Hunt and from AvP2 with its 1.0.9.6
patch as the `rez/` submodule installs them, from the redump images of
their discs that it fetches from archive.org (`rez/README.md`).

| What | Origin | Where it goes |
|---|---|---|
| the game data: `.rez` archives, configs, text, movies | Monolith Productions, Third Law Interactive / Fox Interactive / Sierra | the game directories in `out/` |
| `mss32.dll` and its `.asi`, `.as6` providers | Miles Sound System, RAD Game Tools | the game directories |
| `binkw32.dll`, `Smackw32.dll` | Bink and Smacker, RAD Game Tools | the game directories |

`make reference` extracts the retail binaries (`build/reference.sh`) into
`out/reference/`, to compare our build against; they are never run and
never go into a game directory.

## Not in this repository, provided by you

The compilers in `toolchains/` (Microsoft Visual C++ 6.0; see
`toolchains/README.md`), under their own licences.
