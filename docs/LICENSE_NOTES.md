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
| `src/lithshared/` | the LithTech shared libraries, nolfsrc's: much of it (`stdlith/`, `CryptMgr/`, `RegMgr/`, `MFCStub/`, `ButeMgr/`, `lith/`) adapted there from Monolith's later public source release for *No One Lives Forever 2*, the rest written or reconstructed for nolfsrc (`ztools/`, `rezmgr/`, `gamespy/`, a stand-in for the GameSpy middleware); `ztools/ztools.h` is the *No One Lives Forever* source release's header, which AvP2's release lacks; `zlib/` is zlib | Monolith's code under the terms of the release it came from; zlib under the zlib licence; the rest as `src/engine/` |
| `src/engine/` | the LithTech engine (`lithtech.exe`, `server.dll`, `d3d.ren`): nolfsrc's reconstruction of NOLF's, written for that project from the retail NOLF binaries, being brought to Primal Hunt's. It implements Monolith's engine and interfaces | written for this project and nolfsrc; no licence is declared |
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
