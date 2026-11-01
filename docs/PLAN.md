# The plan

Aliens versus Predator 2 from source, split into packages that can each be
picked up again after an interruption. The first aim is **a game that
runs** with every binary built here; the second is **byte-identity** with
retail 1.0.9.6 (the legacy build), ratcheted by `make progress`. The modern
build (`AVP2_MODERN`) comes after.

AvP2 is the closest of these games to NOLF: the same engine generation
(LithTech "Talon", NOLF's engine of 2000 developed further), the same
compilers, the static C runtime, the separate Direct3D renderer `d3d.ren`.
Its game code was released; its engine was not. What there is, and what it
is good for:

| Source | What it is | For |
|---|---|---|
| The AvP2 source release (`../avp2/proj`, 2002, with its EULA) | Monolith's own `AVP2` tree at 1.0.9.6 (`IDS_VERSION "1.0.9.6"`): ClientShellDLL, ObjectDLL, ClientRes, ServerRes, the dedicated server (AVP2Serv), Shared, the workspace `AVP2.dsw` with VC6 projects (Debug, Release, Final, Demo); and `LT2`: the engine SDK headers (`sdk/inc`) and the `lithshared` libraries (ButeMgr, StdLith, CryptMgr, RezMgr, LTGuiMgr, GameSpy, WONAPI, zlib, ...) with source and prebuilt (`lithshared/libs`, cl 8966) | the game DLLs, their resources and the dedicated server, as Monolith built them, and the libraries. As nolfsrc's `src/game/` is NOLF's release, imported as it is |
| nolfsrc's engine (`../nolfsrc/src/engine`) | NOLF's engine -- `lithtech.exe`, `server.dll`, `d3d.ren` -- reconstructed to compile to NOLF GOTY's retail bytes (every function written; 77 % of `lithtech.exe`'s code identical, 85 % of `server.dll`'s, 28 % of `d3d.ren`'s) | the engine: AvP2's is the same line a year and a half on. The base for `lithtech.exe`, `server.dll` and `d3d.ren`, and its `clientwin.cpp` already calls `Launch.dll`'s `GetLithTechCommandLine` |
| The 1.0 release (`rez`'s `avp2-eu`) | the discs' own engine and renderer (the patch replaced them), and the same launcher, `ltmsg.dll`, `Launch.dll` and `AVP2Up.dll` | what the patches changed in the engine; a second witness for any engine function |
| lithtools' Talon tools (`../lithtools`, `legacy/`) | AvP2's DEdit and ModelEdit, rebuilt to their shipped bytes; the AvP2 tool releases (`rez`'s `avp2-tools`, `-update1`, `-update2`, binaries only) | the engine-side code the tools share (world, model and texture formats, the preprocessor), compiled with the same Processor Pack compiler (9044) |
| *Primal Hunt* (`rez`'s `avp2-ph-*`, 2002) | the expansion, by Third Law, on the same engine: its own `lithtech.exe`, `d3d.ren`, game DLLs and launcher (`PrimalHunt.exe`) | a later witness to the engine; a game of its own, not built here |
| The demo disc (`rez`'s `avp2-demo-usa`, image only) | the single- and multiplayer demo installers (`avp2spdemo.exe`, `avp2mpdemo2.exe`) | the demos' engines and game DLLs: the release's Demo configurations say what they were built from |
| The GameSpy SDK and WON's API (`lithshared/gamespy`, `lithshared/wonapi`) | in the source release | the `middleware` blocks to place in the game DLLs and the engine |

There is no AvP2 engine source, PDB or map anywhere that could be found
(2026-11).

## The retail game

`rez/games/avp2-eu-1.0.9.6` (`build/retail.sh`): the European discs with
the English 1.0.9.6 patch (April 2002), the last, which includes the earlier
ones (`rez`'s `avp2-eu-1.0.9.6`). AvP2 had no GOTY edition. The patch put a
new engine, renderer and dedicated server over the install, with
`AVP2DLL.REZ` (`cshell.dll`, `object.lto`) and `AVP2L.REZ` (`cres.dll`,
`sres.dll`) replaced whole and `AVP2P1.REZ` (data) added last in the rez
stack. The launcher, `ltmsg.dll`, `Launch.dll` and `AVP2Up.dll` are the
discs', the same in the European and USA releases.

| Binary | What | Size | Functions (own code, enumerated) |
|---|---|---|---|
| `lithtech.exe` | the engine; Miles (`mss32.dll`), DirectInput | 916 KB | 4,190 |
| `server.dll` | the engine's server | 544 KB | 2,876 |
| `d3d.ren` | the Direct3D renderer (DirectDraw/D3D 7) | 340 KB | 1,281 |
| `ltmsg.dll` | the engine's message strings | 32 KB | resources |
| `Launch.dll` | NOLF's launcher DLL (1999, a Debug build): the engine's `GetLithTechCommandLine` | 236 KB, nearly all the debug CRT | 43 |
| `AVP2.exe` | the launcher (MFC, static); `.cms_t`/`.cms_d`, a protection wrapper's | 5.1 MB, nearly all resources | 351 |
| `AVP2Up.dll` | resources for the Sierra updater ("AVP2 Customization DLL") | 244 KB | resources |
| `AVP2Serv.exe` | the dedicated server (MFC, static; imports `server.dll`) | 1.2 MB | 355 |
| `CShell.dll` | the game: client | 2.0 MB | 6,852 |
| `Object.lto` | the game: server objects | 3.1 MB | 11,189 |
| `CRes.dll`, `SRes.dll` | the game's resources | 436 KB, 44 KB | resources |

27,137 functions in all. Every binary is VC6, in four revisions:
`progress/COMPILERS.md`. Nothing in the 1.0.9.6 engine is protected or
encrypted. Third-party, and not built here: Bink, Smacker, Miles (with its
MP3 decoder) and the Sierra updater (`SierraUp.exe`, `SierraPt.dll`).

## Ground rules (every package)

- **Source.** Game code is the source release, kept as Monolith wrote it;
  what the retail binaries do differently is changed in place and marked
  `RETAIL:` with the retail address (`0x...`), as nolfsrc marks `GOTY:`.
  Engine code starts from nolfsrc's engine; where nothing exists, it is
  written from the retail disassembly, citing the address.
- **Compiler.** The revision each binary's Rich header names: 8168 for the
  game DLLs, the dedicated server and `d3d.ren` (or Intel C/C++, as NOLF's
  renderer), 9044 for the 1.0.9.6 engine and the launcher, 8966 for the
  `lithshared` libraries. Prove the flags per binary, as nolfsrc did.
- **Project files.** The release's VC6 `.dsw`/`.dsp` (`tools/dsp2mk.py`).
- **Legacy vs modern.** `AVP2_MODERN` gates every fix and feature, and
  `AVP2_DEBUG` the debug build's logging; the legacy build must keep
  compiling to the retail bytes.
- **Retail bugs.** Every bug found goes into `RETAIL_BUGS.md`, fixed (if at
  all) only behind `AVP2_MODERN`.
- **Commits.** Small commits, each leaving the tree building.

## Packages

| # | Package | Depends on | Status |
|---|---|---|---|
| 1 | `scaffold`: submodules (`rez`, `toolchains`, `tools`), the 1.0.9.6 install in rez, the build image, `make retail`/`reference`/`progress`, `targets.py` | -- | done (below) |
| 2 | `import`: the source release into `src/`, as it is (`src/AVP2`, `src/LT2`), with its EULA, and `docs/LICENSE-NOTES.md`, as nolfsrc's | 1 | |
| 3 | `build`: `build/env.sh`, `build/build.mk` and `build/compile.sh` (nolfsrc's, with each binary's revision), `build/all.sh` assembling a game directory (with the loose files the game reads: `console.PCX`, `childmodel.map`), `build/variant.sh` (debug, modern) | 2 | |
| 4 | `game`: `CShell.dll`, `Object.lto`, `CRes.dll`, `SRes.dll` and `AVP2Serv.exe` from the release, with 8168; the `lithshared` libraries rebuilt with 8966 and checked against the prebuilt ones; which configuration shipped (Release or Final); GameSpy and WON placed as `middleware` | 3 | |
| 5 | `small`: `ltmsg.dll` and `AVP2Up.dll` (resource scripts from the retail resources; `tools/rsrcdiff.py`), `Launch.dll` (a Debug build of NOLF's LauncherDLL) | 3 | |
| 6 | `engine`: `lithtech.exe`, `server.dll` (9044) and `d3d.ren` from nolfsrc's engine, toward AvP2's | 3 | |
| 7 | `launcher`: `AVP2.exe` from the retail disassembly and NOLF's launcher, without the wrapper | 3 | |
| 8 | `boot`: the built binaries in the game under Wine, to the menu, a level, saving and loading (`make test`, tools/playtest.sh) | 4-7 | |
| 9 | `match`: byte-matching, module by module, ratcheted by `make progress` | 4-7 | |
| 10 | `modern`: the modern build (`AVP2_MODERN`, `MODERN.md`) | 8 | |
| 11 | `release`: `-private`/`-public` archives, install scripts from the player's own discs and patch (`tools/mkinstall.py`), and the GitHub Actions workflow building the public archives with `make progress` on the legacy release | 8, 10 | |

## Notes per package

### 1. scaffold

Done.

- `rez/` installs `avp2-eu-1.0.9.6` (`rez/tools/install.py`: the European
  release, "Typical", then the patch's `Update Files`; `Update Command
  Line` `-rez AVP2P1.REZ`), recorded in `rez/games.sha256`.
- `toolchains/` has every revision the headers name: `msvc-6.00-8168`,
  `-8447`, `-8966`, `-9044`, and `icl-4.0`.
- `tools/retailextract.py` reads the retail layout from `build/retail.py`.
- `make reference` extracts the 12 retail binaries and enumerates them;
  `make progress` scores 27,137 functions, all unwritten.
- `targets.py`'s `app` boundaries are provisional (the first runtime
  member `libscan.py` finds); each is to be pinned from our own `.map` once
  the binary links, as nolfsrc did. Below several of them is a stretch of
  packed code that may be runtime too; `targets.py` notes each.
- Unknowns: `d3d.ren`'s 0x25/8755 and 13.00.8830 objects; the objects
  without a `@comp.id` in `lithtech.exe`, the launcher and the dedicated
  server; the launcher's wrapper (`.cms_t`, `.cms_d`).

### 10. modern

What the modern build is to have, besides the fixes in `RETAIL_BUGS.md`
(`MODERN.md` keeps the inventory as items land):

- widescreen support, with an option to keep 4:3 in the cinematics;
- field-of-view configuration;
- up to 16x anisotropic filtering and up to 8x antialiasing (nolfsrc's
  modern Direct3D 9 renderer is the way there);
- raw mouse input;
- XInput for controllers, in place of DirectInput joysticks;
- open-source replacements for the proprietary libraries: OpenMiles for
  Miles, our Bink player on FFmpeg for Bink, as nolfsrc's; Smacker's too;
- the launcher as `AVP2Launcher.exe`, with `AVP2.exe` starting the game
  directly, as if Launch had been clicked.
