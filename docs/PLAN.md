# The plan

Aliens versus Predator 2 and its expansion Primal Hunt from source, split
into packages that can each be picked up again after an interruption. The
first aim is **a game that runs** with every binary built here; the second
is **byte-identity** with the retail binaries (the legacy build), ratcheted
by `make progress`. The modern build (`AVP2SRC_MODERN`) comes after.

## What "the game" is

The last release is *Primal Hunt* 1.0.2.2 (Third Law, June 2002). Its
readme: it "includes all of the changes made in Aliens Versus Predator 2
patches up to version 1.0.9.6", and its engine (`lithtech.exe` 2002-05-20,
`server.dll`, `d3d.ren`, `cdaudio.dll`), launcher, dedicated server and
game DLLs are all newer than AvP2 1.0.9.6's (February to April 2002). No
later patch for it is known. But Primal Hunt installs beside AvP2, needs
AvP2's archives (`AVP2.REZ`, `SOUNDS.REZ`, the three campaigns' world
archives), and "you cannot play Aliens Versus Predator 2 singleplayer
missions from within" it: AvP2's campaigns are AvP2's game DLLs'. So the
target is:

- **Primal Hunt 1.0.2.2 (Europe)**, all of it: the engine, `cdaudio.dll`,
  `ltmsg.dll`, `Launch.dll`, `PrimalHunt.exe`, `AVP2XUp.dll`,
  `AVP2XServ.exe`, and its game DLLs (`PHCShell.dll`, `PHObject.lto`,
  `PHCRes.dll`, `PHSRes.dll`: `CSHELL.dll`, ... in its `AVP2DLL.REZ` and
  `AVP2L.REZ`);
- **AvP2 1.0.9.6 (Europe)**, what Primal Hunt does not replace: its game
  DLLs (`CShell.dll`, `Object.lto`, `CRes.dll`, `SRes.dll`), launcher
  (`AVP2.exe`, `AVP2Up.dll`) and dedicated server (`AVP2Serv.exe`).

AvP2 1.0.9.6's engine is superseded, and kept as a witness, not a target.
Whether Primal Hunt's engine runs AvP2's 1.0.9.6 game DLLs (one engine for
both games) is to be shown (package 10); if it does not, 1.0.9.6's engine
comes back as a target.

AvP2 is the closest of these games to NOLF: the same engine generation
(LithTech "Talon", NOLF's engine of 2000 developed further), the same
compilers, the static C runtime, the separate Direct3D renderer `d3d.ren`.
Its game code was released; its engine was not, and nothing of Primal
Hunt's was. What there is, and what it is good for:

| Source | What it is | For |
|---|---|---|
| The AvP2 source release (`../avp2/proj`, "Source Code v1.0.9.6", July 2002, with its EULA) | Monolith's own `AVP2` tree at 1.0.9.6 (`IDS_VERSION "1.0.9.6"`): ClientShellDLL, ObjectDLL, ClientRes, ServerRes, the dedicated server (AVP2Serv), Shared, the workspace `AVP2.dsw` with VC6 projects (Debug, Release, Final, Demo); and `LT2`: the engine SDK headers (`sdk/inc`) and the `lithshared` libraries (ButeMgr, StdLith, CryptMgr, RezMgr, LTGuiMgr, GameSpy, WONAPI, zlib, ...): their headers and prebuilt builds (`lithshared/libs`, cl 8966), and the source of WONAPI and STLport only. No Primal Hunt code in it | AvP2's game DLLs, their resources and the dedicated server, as Monolith built them, and the libraries; the base of Primal Hunt's game DLLs, which are this code carried on. As nolfsrc's `src/game/` is NOLF's release, imported as it is |
| nolfsrc's engine (`../nolfsrc/src/engine`) | NOLF's engine -- `lithtech.exe`, `server.dll`, `d3d.ren` -- reconstructed to compile to NOLF GOTY's retail bytes (every function written; 77 % of `lithtech.exe`'s code identical, 85 % of `server.dll`'s, 28 % of `d3d.ren`'s) | the engine: Primal Hunt's is the same line two years on. The base for `lithtech.exe`, `server.dll` and `d3d.ren`, and its `clientwin.cpp` already calls `Launch.dll`'s `GetLithTechCommandLine` |
| nolf2src's and cjacksrc's engines (Jupiter) | the next engine generation, reconstructed | where Talon's code survived into Jupiter: later shapes of the same functions (`cdaudio.dll`'s music driver is Jupiter's `musicdriver.cpp`'s ancestor) |
| AvP2 1.0.9.6's engine (`rez`'s `avp2-eu-1.0.9.6`) and the 1.0 release's (`avp2-eu`) | the engines between NOLF's and Primal Hunt's | what changed on the way; a second and third witness for any engine function |
| lithtools' Talon tools (`../lithtools`, `legacy/`) | AvP2's DEdit and ModelEdit, rebuilt to their shipped bytes; the AvP2 tool releases (`rez`'s `avp2-tools`, `-update1`, `-update2`, binaries only; Primal Hunt's disc carries three AvP2 tools installers in `Tools/` too) | the engine-side code the tools share (world, model and texture formats, the preprocessor) |
| The demo disc (`rez`'s `avp2-demo-usa`, image only) | the single- and multiplayer demo installers (`avp2spdemo.exe`, `avp2mpdemo2.exe`) | the demos' engines and game DLLs: the release's Demo configurations say what they were built from |
| The GameSpy SDK and WON's API (`lithshared/gamespy`, `lithshared/wonapi`) | in the source release: GameSpy's headers, WONAPI's source and project files | the `middleware` blocks to place in the game DLLs and the engine |
| nolfsrc's and nolf2src's `lithshared` (`../nolfsrc/src/lithshared`, `../nolf2src/src/libs`) | the libraries NOLF's release shipped prebuilt, reconstructed (nolfsrc), and NOLF2's release's own source and VC6 projects for most of them (StdLith, ButeMgr, CryptMgr, RegMgr, MFCStub, zlib, LTGUIMgr, RezMgr) | the libraries AvP2's release ships only prebuilt |

There is no AvP2 or Primal Hunt engine source, PDB or map anywhere that
could be found (2026-11), nor any Primal Hunt game source.

## The retail games

Both are installed by `rez/` (`build/retail.sh`), the European releases:

- `rez/games/avp2-ph-eu`: Primal Hunt (Europe) 1.0.2.2, "Typical" (its
  layout, for the tools: `build/retail.py`). The European and USA releases
  are the same files; the Italian one differs in its language archive,
  launcher and dedicated server only. Its archives: `AVP2DLL.REZ` (its own
  `cshell.dll`, `object.lto`), `AVP2L.REZ` (`cres.dll`, `sres.dll`),
  `AVP2P.REZ`, `AVP2P1.REZ` (1.0.9.6's, the same file), `AVP2X.REZ`
  (Primal Hunt's worlds and data), `MULTI.REZ`, `DIALOGUE.REZ`. Its
  launcher builds the rez stack from these and AvP2's install
  (`InstallDir` in AvP2's registry key).
- `rez/games/avp2-eu-1.0.9.6`: AvP2 (Europe) with the English 1.0.9.6 patch
  (April 2002), the last, which includes the earlier ones (its layout:
  `build/retail_avp2.py`). AvP2 had no GOTY edition. The patch put a new
  engine, renderer and dedicated server over the install, with
  `AVP2DLL.REZ` and `AVP2L.REZ` replaced whole and `AVP2P1.REZ` (data)
  added last in the rez stack. The launcher, `ltmsg.dll`, `Launch.dll` and
  `AVP2Up.dll` are the discs'.

| Binary | Of | What | Size | Functions (own code, enumerated) |
|---|---|---|---|---|
| `lithtech.exe` | PH | the engine; Miles (`mss32.dll`), DirectInput | 984 KB | 4,483 |
| `server.dll` | PH | the engine's server | 596 KB | 3,136 |
| `d3d.ren` | PH | the Direct3D renderer (DirectDraw/D3D 7) | 356 KB | 1,301 |
| `cdaudio.dll` | PH | the engine's CD-audio music driver (`CRedMgr`, MCI; MFC, static) | 100 KB | 90 |
| `ltmsg.dll` | PH | the engine's message strings | 44 KB | resources |
| `Launch.dll` | both | NOLF's launcher DLL (1999, a Debug build): the engine's `GetLithTechCommandLine` | 236 KB, nearly all the debug CRT | 43 |
| `PrimalHunt.exe` | PH | the launcher (MFC, static); `.cms_t`/`.cms_d`, a protection wrapper's | 6.9 MB, nearly all resources | 357 |
| `AVP2XUp.dll` | PH | resources for the Sierra updater | 620 KB | resources |
| `AVP2XServ.exe` | PH | the dedicated server (MFC, static; imports `server.dll`) | 1.2 MB | 354 |
| `PHCShell.dll` | PH | the game: client | 2.1 MB | 6,158 |
| `PHObject.lto` | PH | the game: server objects | 3.2 MB | 10,704 |
| `PHCRes.dll`, `PHSRes.dll` | PH | the game's resources | 492 KB, 44 KB | resources |
| `AVP2.exe` | AvP2 | the launcher (MFC, static); the same wrapper | 5.1 MB, nearly all resources | 351 |
| `AVP2Up.dll` | AvP2 | resources for the Sierra updater ("AVP2 Customization DLL") | 244 KB | resources |
| `AVP2Serv.exe` | AvP2 | the dedicated server | 1.2 MB | 355 |
| `CShell.dll` | AvP2 | the game: client | 2.0 MB | 6,852 |
| `Object.lto` | AvP2 | the game: server objects | 3.1 MB | 11,189 |
| `CRes.dll`, `SRes.dll` | AvP2 | the game's resources | 436 KB, 44 KB | resources |

45,373 functions in all. Every binary is VC6, in four revisions:
`progress/COMPILERS.md` -- Primal Hunt's all SP5 (8966), its renderer
included. Nothing in either engine is protected or encrypted.
Third-party, and not built here: Bink, Smacker, Miles (with its MP3
decoder) and the Sierra updater (`SierraUp.exe`, `SierraPt.dll`, and
Primal Hunt's `SuDE.dll`, `SuES.dll`, `SuFR.dll`, `SuIT.dll`).

## Ground rules (every package)

- **Source.** One game tree, the source release, kept as Monolith wrote
  it, builds both games. What AvP2's retail 1.0.9.6 binaries do
  differently from the release is changed in place and marked `RETAIL:`
  with the retail address (`0x...`), as nolfsrc marks `GOTY:`. Primal
  Hunt's changes go behind `PRIMAL_HUNT` (defined by the projects'
  Primal Hunt configurations), marked `PH:` with the address in
  `PHCShell.dll` or `PHObject.lto`; new files of Third Law's are new files.
  Engine code starts from nolfsrc's engine and is brought to Primal
  Hunt's; where nothing exists, it is written from the retail disassembly,
  citing the address.
- **Compiler.** The revision each binary's Rich header names: 8966 for all
  of Primal Hunt's (with 8168 for the objects of its game DLLs and
  dedicated server whose header says so), 8168 for AvP2's game DLLs and
  dedicated server, 9044 for AvP2's launcher, 8966 for the `lithshared`
  libraries. Prove the flags per binary, as nolfsrc did.
- **Project files.** The release's VC6 `.dsw`/`.dsp` (`tools/dsp2mk.py`),
  with a Primal Hunt configuration added beside Release.
- **Legacy vs modern.** `AVP2SRC_MODERN` gates every fix and feature, and
  `AVP2SRC_DEBUG` the debug build's logging; the legacy build must keep
  compiling to the retail bytes.
- **Retail bugs.** Every bug found goes into `RETAIL_BUGS.md`, fixed (if at
  all) only behind `AVP2SRC_MODERN`.
- **Commits.** Small commits, each leaving the tree building.

## Packages

| # | Package | Depends on | Status |
|---|---|---|---|
| 1 | `scaffold`: submodules (`rez`, `toolchains`, `tools`), both installs in rez, the build image, `make retail`/`reference`/`progress`, `targets.py` | -- | done (below) |
| 2 | `import`: the source release into `src/`, as it is (`src/AVP2`, `src/LT2`), with its EULA, and `docs/LICENSE_NOTES.md`, as nolfsrc's | 1 | done (below) |
| 3 | `build`: `build/env.sh`, `build/build.mk` and `build/compile.sh` (cjacksrc's, with each binary's revision), `build/all.sh` assembling a game directory for each game (with the loose files the games read: `console.PCX`, `childmodel.map`, Primal Hunt's `FontData.fnt`), `build/variant.sh` (debug, modern) | 2 | done (below) |
| 4 | `game`: AvP2's `CShell.dll`, `Object.lto`, `CRes.dll`, `SRes.dll` and `AVP2Serv.exe` from the release, with 8168; the `lithshared` libraries rebuilt with 8966 (WONAPI from the release's source, the rest from nolfsrc's and nolf2src's) and checked against the prebuilt ones; which configuration shipped (Release or Final); GameSpy and WON placed as `middleware` | 3 | |
| 5 | `ph-game`: Primal Hunt's game DLLs and `AVP2XServ.exe` from the same tree with `PRIMAL_HUNT`, with 8966: what Third Law added and changed, from the retail images (their strings name new classes: `CAITentacle`, `CAIChameleon`, `CMiniTurret`, `CTurretSpawner`, `CArtifactField`, dual pistols, the Corporate missions, ...); `PHCRes.dll`/`PHSRes.dll` from the release's `.rc` and the retail resources (`tools/rsrcdiff.py`) | 4 | |
| 6 | `small`: `ltmsg.dll`, `AVP2XUp.dll` and `AVP2Up.dll` (resource scripts from the retail resources), `Launch.dll` (a Debug build of NOLF's LauncherDLL) | 3 | `ltmsg.dll` done (below) |
| 7 | `engine`: `lithtech.exe`, `server.dll` and `d3d.ren` (8966) from nolfsrc's engine, toward Primal Hunt's; `cdaudio.dll` from the disassembly and `lithshared`'s `redmgr.h` | 3 | started (below) |
| 8 | `launcher`: `PrimalHunt.exe` and `AVP2.exe` from the retail disassembly and NOLF's launcher, without the wrapper; the rez stacks they build | 3 | linked (below) |
| 9 | `boot`: the built binaries in both games under Wine, to the menu, a level, saving and loading (`make test`, tools/playtest.sh) | 4-8 | harness done; both engines stop before WinMain (below) |
| 10 | `one engine`: AvP2's 1.0.9.6 game DLLs on Primal Hunt's engine, both retail and ours (if not, 1.0.9.6's engine becomes a target) | 9 | answered: not as shipped (package 7); 1.0.9.6's engine to be targeted |
| 11 | `match`: byte-matching, module by module, ratcheted by `make progress` | 4-8 | |
| 12 | `modern`: the modern build (`AVP2SRC_MODERN`, `MODERN.md`) | 9 | built (`make modern`); no item done yet |
| 13 | `release`: `-private`/`-public` archives, install scripts from the player's own discs and patch (`tools/mkinstall.py`), and the GitHub Actions workflow building the public archives with `make progress` on the legacy release | 9, 12 | done (below); the workflow not yet run on GitHub |

## Notes per package

### 1. scaffold

Done.

- `rez/` installs `avp2-ph-eu` (`rez/tools/install.py`: the European
  release, "Typical") and `avp2-eu-1.0.9.6` (the European release,
  "Typical", then the patch's `Update Files`; `Update Command Line` `-rez
  AVP2P1.REZ`), both recorded in `rez/games.sha256`.
- `toolchains/` has every revision the headers name: `msvc-6.00-8168`,
  `-8447`, `-8966`, `-9044`, and `icl-4.0`.
- `tools/retailextract.py` reads Primal Hunt's layout from
  `build/retail.py`, and AvP2's from `build/retail_avp2.py`
  (`RETAIL_LAYOUT`); `build/reference.sh` runs it on both into one
  `out/reference/`, Primal Hunt's game DLLs under `PH` names.
- `make reference` extracts the 20 retail binaries and enumerates them;
  `make progress` scores 45,373 functions, all unwritten.
- `targets.py`'s `app` boundaries are provisional (the first runtime
  member `libscan.py` finds); each is to be pinned from our own `.map` once
  the binary links, as nolfsrc did. Below several of them is a stretch of
  packed code that may be runtime too; `targets.py` notes each.
- Unknowns: the objects without a `@comp.id` in `lithtech.exe`,
  `cdaudio.dll`, the launchers and the dedicated servers; the launchers'
  wrapper (`.cms_t`, `.cms_d`); `PHCShell.dll`'s WONAPI, linked above the
  runtime and matching the release's prebuilt library far less than
  1.0.9.6's does; AvP2 1.0.9.6's `d3d.ren`'s 0x25/8755 and 13.00.8830
  objects (a witness now, not a target).

### 2. import

Done. The release (`../avp2/proj`) is in `src/AVP2` and `src/LT2` as it
came, every text file with LF line endings (`credits.txt`, embedded in
`CRes.dll`, keeps CRLF: `.gitattributes`); its readme and Fox's EULA are
`src/MONOLITH-README.txt` and `src/MONOLITH-EULA.doc`. Left out: the Miles
and Bink binaries of `LT2/sdk/win/lib`. The prebuilt `lithshared`
libraries and `AVP2Serv/server.lib` are in `progress/reference/`, with
their `SHA256SUMS`. What is whose: `docs/LICENSE_NOTES.md`.

`LT2/lithshared` is the libraries' headers -- ButeMgr, CryptMgr, RegMgr,
RezMgr, RedMgr (the `CRedMgr` of `cdaudio.dll`), StdLith, LTGUIMgr,
LithFontMgr, GameSpy, zlib, ... -- with the source of only two: WONAPI (198
files, with its own VC6 projects) and STLport. The rest ship prebuilt
only, as NOLF's did.

### 3. build

`build/build.mk` builds the release's own workspace (`src/AVP2/AVP2.dsw`)
through `tools/dsp2mk.py`, which expands the project files' `$(LT2_DIR)`
to `S:\LT2` (`--macro`, `--drive`), as Developer Studio would with the
release's `SetBuildVars.bat`. `build/compile.sh` builds in
`out/release-tree/` (without the `AVP2SRC_MODERN`/`AVP2SRC_DEBUG` blocks, as
cjacksrc's). `make` (`build/all.sh`) builds both games into one game
directory, `out/avp2-legacy-release/`, laid out as the retail setups
install them -- `AVP2/` (AvP2 1.0.9.6: its own engine, launcher, server,
game DLLs) and `PrimalHunt/` (Primal Hunt's), each with its game DLLs in
`custom/`, the last in its rez stack (`build/avp2cmds.sh`; Primal Hunt
reads AvP2's archives from `../AVP2/`) -- from our binaries and the
retail data (`.rez` archives with the retail game DLLs zeroed, movies,
loose files, readmes, shipped settings; Bink, Smacker and Miles as
third-party runtime), audited and packaged (`build/package.sh`: -private,
and -public with the install scripts of `build/mkinstall.py`).
`build/variant.sh` builds `make debug` (`AVP2SRC_DEBUG`, symbols),
`make modern` (`AVP2SRC_MODERN`) and `make modern-debug` in copies of the
tree under `out/build-<variant>/`.

- Which configuration: **Final** for the game DLLs (`_FINAL`): none of the
  diagnostics the release keeps under `#ifndef _FINAL` is in either game's
  retail `Object.lto`. The resource DLLs and the dedicated server have
  only a Release configuration.
- Which tools: cl 8168 for the game DLLs, with msvc-6.00-8966's link
  (6.00.8447), headers and libraries; 8168's throughout for the dedicated
  server; rc of msvc-6.00-9782 (`progress/COMPILERS.md`).
- The prebuilt `lithshared` libraries and `server.lib` are copied from
  `progress/reference/` to where the project files name them, until they
  are built here, and left out of our side of the score
  (`progress/targets.py`, `PREBUILT_LIBS`).

### 4. game

Started. The release as it came, built as above, scores `Object.lto` 89 %,
`CShell.dll` 62 %, `AVP2Serv.exe` 60 % byte-identical, and every resource
of `CRes.dll` and `SRes.dll`. Most of `CShell.dll`'s unwritten functions are
the prebuilt libraries' (not counted as ours). Two of our client's
functions are in no form in retail's (unplaced): `CWeaponFX::CreateBulletTrail`
and `OverlayMGR::GetRailMode` -- the release's source is dated July 2002,
retail 1.0.9.6's `CShell.dll` March 2002, so the release may be a little
newer than what shipped.

`CShell.dll` is two compilers' (progress/COMPILERS.md): 83 of its sources
were rebuilt with SP5's cl 8966 after Monolith moved to it, the other 105
are RTM's 8168, each proven by compiling it both ways (`build/build.mk`,
`CSHELL_8966`). With them every function of the client's own code that the
scorer can verify (all but the short relocation-only ones it scores small) is
identical but CSoundButeMgr's destructor (8966's code, in an 8168 object,
as in `Object.lto`), 4,412 -> 4,782 in all. Neither unplaced function was
missing: `CreateBulletTrail` is 8966's code (one inline Init more), and
`OverlayMGR::GetRailMode` is reached only by a tail jump, so the
enumeration had merged it into the function before it
(`progress/extent-splits.csv`).

### 4. game: the lithshared libraries

Six of the libraries are built from nolfsrc's sources (`src/lithshared/`),
with cl 8966 and nolfsrc's flags, into the paths the project files link
them from, so the game DLLs link ours (`build/build.mk`, "The lithshared
libraries, ours"); each is scored against the release's prebuilt build
(`progress/targets.py`, `LIBS`): CryptMgr and RegMgr 100 %, StdLith 99.2 %,
MFCStub 96.2 %, lith 43.8 %, RezMgr 0.3 %. With them, `CShell.dll` is 62.7 %,
`Object.lto` 89.1 %.

RezMgr is now Monolith's (`src/lithshared/rezmgr`: NOLF2's release's
rezmgr.cpp, rezhash.cpp and rezfile.cpp, the three objects the shipped
builds hold, in AvP2's REZMGRLITE build), and it and StdLith are built a
second time as the release's Debug builds (/MTd /ZI /Od), which Primal
Hunt's `server.dll` links (`build/build.mk`, "The debug builds"). /ZI makes
every assert carry its line relative to its function, so the debug builds
fix the source's line layout too, and the release's prebuilt
`Rezmgrfull.lib` (the full build of the same file, /Z7, with COFF line
numbers) shows where the code REZMGRLITE leaves out stood. All four --
RezMgr and StdLith, release and debug -- are 100 %.

ButeMgr is NOLF2's (`src/lithshared/ButeMgr`): AvP2's parser is NOLF2's
line -- STLport symbol tables, the same four objects -- at an earlier stage,
with CString accessors and a second tag table, which ours takes from
AvP2's header (the CString `AddString` from the release's prebuilt build).
ButeMgrNoMFC is 72.5 % byte-identical; its error reporting (`File "%s",
line %d`, an `EXPANSION` keyword) is still NOLF2's (progress/data-known.txt).
Two more are not NOLF's and stay prebuilt (left out of our score) until
reconstructed from the release's builds: LTGUIMgr (menu fonts with three
faces, DrawMenu, GetTextExtents) and LithFontMgr (a CString for the
previous string, a default colour). lith is a
different version too, by its score. Prebuilt and not ours to build:
GameSpy's managers (middleware) and LithTrackMgr (headers only); WONAPI's
source is in the release, to be built from it.

### 5. ph-game

Third Law's game code is AvP2 1.0.9.6's carried on. Of the distinct
strings of 8 characters or more, 65 % of Primal Hunt's client's are also in
1.0.9.6's client, and 58 % of its server objects' in 1.0.9.6's; the rest
are new (1,840 in the client, 3,403 in the server objects). The way in: build the `PRIMAL_HUNT` configuration from the
release unchanged, score it against `PHCShell.dll`/`PHObject.lto` (with
8966 the codegen of unchanged files should match outright), and work
outward from what does not.

Started. `src/AVP2/AVP2PH.dsw` and the `_PH` projects (`build/phproject.py`)
build the release's sources unchanged with cl 8966 and `PRIMAL_HUNT`:
`PHObject.lto` is then 46.7 % byte-identical and `PHCShell.dll` 26.1 %,
with 2,302 and 1,794 more functions "near" (the same function, changed:
Third Law's edits, or one of the objects still 8168's) and 1,109 and 61
of ours unplaced (code Primal Hunt dropped or rewrote).

The dedicated servers. `AVP2Serv.exe`'s Rich header has no object of cl
8966: the butemgr.lib (MFC's build), cryptmgr.lib and StdLith.lib it links
were 8168's, not the release's prebuilt ones, so `build/build.mk` ("The
dedicated servers") builds them with 8168 into the project's directory;
with them all of the server's own code is byte-identical, and what is left
is ButeMgr's (the AvP2 version of it, as for ButeMgrNoMFC). `AVP2XServ.exe`
is `ServerApp_PH.dsp` in `AVP2PH.dsw` (the server's Release configuration
with `PRIMAL_HUNT`), cl 8966 against SP5's MFC and runtime, the release's
libraries and RegMgr32.lib; its resources are all byte-identical (the
splash read out of the retail image at build time). Reconstructed so far:
the class and weapon restrictions (CServerOptions' two lists, LoadFromBute's
Weapon and Class sections, FillInHostInfo's tables and masks, MPGameInfo's
defaults; the server's lists are their own class, CMPAllowList, since any
change to MPRestrictList.h moves unrelated PHObject and PHCShell code); its
ButeMgr, MFC's build with cl 8966, ours (`ButeMgr` in OUR_LIBS);
`AddResources` (AvP2's rez files from AvP2's install directory, read from
the registry), `OnInitDialog`, the default port (27890) and Primal Hunt's
default names and passwords. Still to come: Third Law's class and weapon
restrictions (`CServerOptions`, `MPGameInfo`: `progress/data-known.txt`),
shared with `PHCShell.dll`'s copy of `ServerOptions.cpp`.

Round of 2026-11 (six agents): Third Law's layout changes reconstructed
behind `PRIMAL_HUNT`:
- GameBase's ten new virtual message handlers;
- DamageStruct's position;
- CSpecialFX::Term made virtual;
- CharacterButes' DefaultEnvMap;
- the CMS_TURRET state;
- the bute cache (`Shared/MultiFile.h`, every FX struct's Write and Read);
- the dual-muzzle weapons, the subtitle and self-destruct FX, carried
  turrets, and the CLTGUICtrl fields.
The game libraries are ours: WONAPI from the release's source, LTGUIMgr,
LithFontMgr and ButeMgr. CShell.dll's sources are split between cl 8168
and cl 8966 (`CSHELL_8966`). Scores: PHCShell.dll 4,090, PHObject.lto
7,712, CShell.dll 5,748, Object.lto 8,513 identical.

Still to do:
- Find which file defines AIUtils.h's sine/cosine constants: Primal Hunt
  initialises them once, at 0x1021a540 after SimpleNodeMgr, and they
  account for about 1,070 of PHObject's unplaced functions.
- A `PRIMAL_HUNT` build of LTGUIMgr.
- Primal Hunt's WONAPI (the VC6-STL build), which needs its GameSpy
  managers rebuilt.
- The scorer's vtable-vote tie (a table that ties gets no placement).
- The PHObject rows the enumeration splits.
- Door's size (the other server-side layouts are in: MPGameInfo's class
  and weapon tables, the "_MP" restriction lists of CWeaponMgr and
  CCharacterButeMgr (`Shared/MPRestrictList.h`), PickupButes' WeaponRef,
  DT_BACKHAND, and ILTServer's extra model file-name argument).
- The bute cache's readers and writers of the other managers (FXButeMgr's
  Init, ReadCache and WriteCache); `Shared/MultiFile.cpp` and
  CCharacterButeMgr's are in.
- CServerOptions' class and weapon lists (PHCShell.dll's ServerOptions.cpp,
  AVP2XServ.exe): CMPRestrictList is the layout.
- Primal Hunt's player turrets (CTurretGun, MID_PLAYER_TURRETS and
  MID_TURRET_USE: StartDeath, the turret's use), and the footstep listeners
  (CCharacter::HandleFootstepKey, 0x1014baa0).

### 6. small

`ltmsg.dll`, both releases': `src/engine/ltmsg/ltmsg.rc` is read out of the
retail DLLs by `tools/rsrc2rc.py` (Primal Hunt's and 1.0.9.6's carry the
same resources), around nolfsrc's `ltmsg.cpp`; Primal Hunt's built with
SP5's tools into `Release/`, 1.0.9.6's with RTM's into `AvP2Release/`.
Every resource byte-identical in both.

### 7. engine

Started. nolfsrc's engine (`src/engine/lithtech`, `server`, `d3d`, at
nolfsrc e2a6e25; its `NOLFSRC_MODERN`/`NOLFSRC_DEBUG` blocks renamed to ours,
its debug headers in `src/avp2src/`) is compiled with Primal Hunt's cl
12.00.8966 and nolfsrc's flags (`build/build.mk`, "The engine"; the
renderer's files all `/O2`, as nolfsrc's flag groups are Intel C/C++'s),
against AvP2's SDK headers. Objects only: the links need nolfsrc's
lithshared libraries and its link-time tables, which name NOLF's
addresses, so `make progress` scores the objects (`partial_objs`).

- Every object compiles against AvP2's SDK (403). What the SDK changed
  since NOLF, and how it is met (`AVP2:` comments; `provisional` where the
  retail code is still to confirm it):
  - DirectInput 7 under the DirectX 8.1 SDK's `dinput.h`
    (`DIRECTINPUT_VERSION 0x0700`, as the retail call asks: 0x442c55);
  - the weight set moved from the tracker's `AnimTimeRef` to each key
    frame: `GetWeightSet`/`SetWeightSet` from retail (server.dll
    0x10024f57, 0x10024fa1: the previous key frame keeps the old set while
    interpolating); the transform, world-tree and anim-message uses take
    the current key frame's, provisionally;
  - the key-frame callbacks' two new arguments, and the client shell's
    `OnModelKey` (the tracker's number) and `SetDisconnectCode` (no
    sub-reason): provisional;
  - `FLAG2_PLAYERCOLLIDE` is AvP2's `FLAG2_CYLINDERPHYSICS`, the same bit;
  - `ILTCommon`'s new `GetCRC` (CRC-32, 0x10016b42), `GetPolyInfo` and
    `GetPolySurfaceFlags` (0x10039e3e, 0x10039ed4), from retail, with
    Primal Hunt's world polygon layout (its surface at +0x2c, its vertices
    its own), which `GetPolyTextureFlags` now reads too;
  - surface effects' `UpdateEffect` returns nothing: cast, provisionally;
  - `sdkforce.cpp`'s member pointers take AvP2's const signatures.
- **Primal Hunt's engine has an `ILTCommon` slot AvP2's SDK lacks**:
  `GetPolyTextureName` (slot 19; the client's error tag names it,
  lithtech.exe 0x00481280), which moves `GetPolyInfo` and
  `GetPolySurfaceFlags` one down. It is in `iltcommon.h` behind
  `PRIMAL_HUNT`, which the engine is now built with. AvP2 1.0.9.6's
  engine has no such slot (its `ServerCommonLT` table has 21), and AvP2's
  game calls both shifted functions (CharacterMovement.cpp, AINode.cpp,
  ...): so **AvP2's 1.0.9.6 game DLLs cannot run on Primal Hunt's engine
  as shipped**, and package 10 is answered -- AvP2 1.0.9.6's engine
  comes back as a target. Primal Hunt's own game DLLs use the 22-slot
  interface: PHCShell.dll 0x101050f0 calls `GetPolySurfaceFlags` through
  `[edx+0x54]`, slot 21, and matches once built with it.
- **Primal Hunt's `ILTServer` and `ILTClient` have three virtuals AvP2's
  SDK lacks** (server 128 slots against 125, client 113 against 110),
  found by aligning the two tables' targets instruction by instruction:
  two in `ILTCSBase` after `CopyFile` (slots 44 and 45, both sides),
  file queries through the file manager (`GetFileInfo` into a new
  `LTFileInfo`, and `DoesFileExist`: server 0x10037780 and 0x10037808;
  the names ours), and `GetPolyTextureName` forwarding to `ILTCommon`'s
  new slot 19 (server slot 126 after `GetPolyTextureFlags`; client slot
  108, before `StartQuery`). All three are in the SDK headers behind
  `PRIMAL_HUNT`, so Primal Hunt's game calls through `g_pLTServer` and
  `g_pLTClient` at slot 44 or later now land eight bytes on.
  `ILTServerSendView`'s senders move with them (slots 90-96).
- **Primal Hunt's file-find state is 0x104 bytes (MAX_PATH) bigger**:
  every frame holding one is 0x214 where AvP2's is 0x110
  (`fs_FindFileInTrees`, 0x10037836, was a near on displacement widths
  alone). The new field is not read by anything reconstructed yet.
- With the three virtuals and the AvP2 model/object layouts in: Primal
  Hunt's game went from 2,540 to 2,874 identical functions in
  PHCShell.dll (27.7 % to 36.7 % of bytes) and from 5,497 to 5,918 in
  PHObject.lto (47.5 % to 53.2 %); 893 functions newly identical across
  the targets, 52.5 % of all bytes. Ten recorded ones were given up:
  two `AIBM_*Sense` destructors whose base-vtable store maps to another
  table than retail's (Primal Hunt's AIButeMgr changes, package 5; they
  had matched by a coincidence of mapping), and eight small or
  initialiser functions in PHObject.lto and lithtech.exe whose placement
  moved once their neighbours matched.
- **The client side's NOLF layouts, moved to AvP2's** (lithtech.exe's
  near list, six object groups worked in parallel through
  `build/near.sh`): lithtech.exe 2,007 -> 2,409 identical functions
  (27.9 % -> 37.6 % of bytes), and AvP2 1.0.9.6's 2,102 -> 2,475 from
  the same source. Nearly every near was layout, not codegen, and AvP2
  1.0.9.6 shares the layout in every case checked against both images:
  - `CClientMgr`: +4 before `m_NetMgr` (0x510); the light scale at 0x76c
    then a new vertex tint at 0x778; the sound manager at 0x784 and
    0x910 bytes; +0x110 from the music manager (0x1094) on, so the
    connection is at 0x161c and the file manager at 0x1644; 0x22c8 in
    all.
  - `CLTClient`: the AvP2 SDK's function table (212 entries, with
    `SetGameInfo` and the three vertex-tint calls), the client manager
    at 0x3d0; `ClearScreen` takes the colour.
  - client objects: the change flags at 0x124, each type's own fields
    from 0x1b0; `RenderStruct` +8 from `ClearScreen` (0x8c);
    `SharedTexture` 0x40 bytes; the sound instance +8 from 0x38 and 0xc4
    bytes; `CLTDirectMusicMgr`'s three playing lists and VOLUMEOFFSET;
    `IClientShell`, `IWorldBsp` and the server shell each two or three
    slots longer; the message cache 0x44c.
  - Primal Hunt only (`PRIMAL_HUNT`): the client's default port 27890
    (its server.dll and both AvP2 images keep 27888); the find record's
    MAX_PATH; the model's extra texture at +0x1d0 and the +4 it puts on
    the object after it; `tm_FreeTexture`'s texture null test; the two
    ILTCSBase slots in every view of CLTClient's own vtable.
- Header changes found and still to make (each blocks a handful of
  nears; evidence in the agents' notes beside the views):
  `servermgr.h`'s `sb_Free` wants AvP2's `if (pStruct && pBank)` guard;
  `sm_SetObjectChangeFlag`'s first test; SObjData +0xc in Primal Hunt
  (`m_StateLink` 0x48, NetFlags 0x5e; AvP2's 0x5a); `IWorldBsp`'s
  `GetWorldFlags` at 0x20 in the engine headers; `CServerMgr`'s
  `m_Unknown390` at 0x394; `ILTModel` at 38 slots; `ObjectCreateStruct`'s
  extra 128-byte name at +0x300 and `PlaySoundInfo`'s byte at +0x134
  (Primal Hunt); the collection templates' out-of-line allocator;
  `clientsound.h`; `CWorldSurface` 0x40 bytes and `CWorldPart` +4.
- **Primal Hunt's engine is built with `/GX` and against VC6's own STL**
  (progress/COMPILERS.md); AvP2 1.0.9.6's has neither (no exception
  frames, STLport first on the include path). Three consequences:
  - every Primal Hunt function with a destructible local has an exception
    frame, and VC6 drops a state store before a call whose callee's body
    it has already compiled, so definition order inside a file matters;
  - lithtech.exe's WON API is built from `LT2/lithshared/wonapi` against
    VC6's STL (its `basic_string` is 16 bytes, STLport's 12): built that
    way it covers 0x004aab40..0x004ba2a0, and the `WONAPI` row (the
    STLport build, as the game DLLs link it) is 2,261 of 2,263;
  - each Primal Hunt translation unit that includes VC6's `<string>` ends
    in a 44-byte guarded initializer of `std::ctype<unsigned short>::id`
    (guard 0x004f8014, `atexit(0x00401790)`), ~95 of them in lithtech.exe,
    and `<new>`'s `throw()` `operator delete` drops the frames of
    destructors that only delete. Which header carried `<string>` into
    them is not known; until it is, the reconstructed TUs that end in one
    include `<string>` themselves behind `PRIMAL_HUNT` (a stand-in).
- Open headers from the third round (evidence beside the local views
  that stand in for them): `msg_AllocPacket` returns its reference by
  value (smgr.cpp's `msg_AllocPacketQ512` bridges it) and packets are
  0x44c; `CAbstractIO` has a virtual destructor; `CNetMgr::Disconnect`
  and `GetPacket` take a second argument; `WMMoveState` is 0x6c;
  `CModelLoadRequest` 0x24; the poly owner's AvP2 layout; `CWorldBsp`
  0x154; `CMessageBank`'s record 0x5c (Primal Hunt 0x60); `CountAdder`'s
  destructor inline; `CommonModelLT`'s 37-slot order and
  `SetCurAnimTime`'s third argument; `CSoundInstance::m_pFilter` at
  +0x4c; `worldsim.h`'s three changed signatures.
- Open: interlink.cpp (Primal Hunt's inter-object link module, its own
  translation unit) takes its list nodes from the shared 0xc-byte bank;
  retail's workers use a bank of their own (lithtech.exe 0x005f3830,
  server.dll 0x10110b38). Byte-identical code naming the wrong data:
  recorded in progress/vtab-known.txt until the module has its bank.
- Scored: `lithtech.exe` 24.3 % byte-identical (1,764 functions, 732
  near), `server.dll` 23.2 % (1,222, 391 near), `d3d.ren` 3.9 % (139, 70
  near).
- AvP2 1.0.9.6's engine is a target again (`AvP2 lithtech.exe`, `AvP2
  server.dll`, `AvP2 d3d.ren`, `AvP2 ltmsg.dll`; retail images in
  `out/reference/avp2/`), built from the same source without
  `PRIMAL_HUNT` into `<binary>/AvP2Release/`: the engine and server with
  cl 9044, as their Rich headers say. It starts closer to nolfsrc than
  Primal Hunt's does: `lithtech.exe` 28.1 %, `server.dll` 26.9 %. Its
  renderer is NOT Intel C/C++ as NOLF's was (this plan said so at first;
  `progress/COMPILERS.md`): it is cl 8168, Primal Hunt's renderer's source
  four months earlier. It has the same objects in the same link order,
  without the fog volumes, and 1,180 functions byte-identical between the
  two images (docs/AVP2_D3D_OBJECTS.md). So it is built from Primal Hunt's
  `src/engine/d3d/ph/` with 8168, without `PRIMAL_HUNT` (`avp2-d3d`),
  which took it from 10 identical to 857, objects only. It now links
  (`avp2-d3d-link`: the AvP2Release/ph objects in Primal Hunt's TU order,
  link 6.00.8168, the release StdLith.lib after the DirectDraw thunks),
  so the scoreboard places what we write by link order as it does Primal
  Hunt's: 857 -> 1,046 identical. Done since: DrawPasses and its two UV
  generators, d3d_ProcessWorldModel and d3d_DrawNoSortWorldModel behind
  `#ifdef PRIMAL_HUNT` (identical in AvP2). Still to do:
  - Primal Hunt's changes, each behind `#ifdef PRIMAL_HUNT`: the fog
    volumes, the StageState vector, the
    CD3DModelDraw layout, d3d_InitDevice, d3d_RenderCommand,
    d3d_InitTextures, the warp, the intersection structures, and
    ConvertDXTGeneric's type;
  - AvP2's own 0x100135c0 (a polygon splitter Primal Hunt dropped);
- AvP2 1.0.9.6's `lithtech.exe` and `server.dll` link too
  (`avp2-engine-link`, part of `engine-link`): the AvP2Release objects in
  the order their placed functions lie in retail, the release's STLport
  WON, and lith, RezMgr and StdLith built with cl 9044 (`$(LAOUT)`: the
  release's prebuilt 8966 builds are not what retail holds;
  progress/COMPILERS.md). 2,832 -> 3,335 and 1,811 -> 2,283 identical,
  every lith, RezMgr and StdLith function in them identical but the short
  ones. Retail's engine objects also hold STLport template code
  (commonmodel.obj's range, lithtech.exe 0x00457af0..0x00459162:
  `set<string>` members WON's build matches byte for byte, as it does
  WON's `SmartPtr` and `RefCount` members at 0x00413b90 and 0x004872f0);
  ours have none, and WON's block (`lib_range`) no longer stands in for
  them.
- `d3d.ren` links (`d3d-link`, as `engine-link`: /FORCE, a response file
  and a map), against the release's debug StdLith.lib, which retail links
  after the DirectDraw thunks as server.dll does; 154 identical.
  `docs/D3D_OBJECTS.md` maps its code into its translation units, in link
  order: about two thirds of the renderer is `/O1` (packed, unaligned
  functions), the rest `/O2`, no `/GX` anywhere -- where we build every
  file `/O2` from nolfsrc's Intel flag-group splits. Those translation
  units are being rebuilt in `src/engine/d3d/ph/`, one file each with its
  own flags (`ph/<name>.mk`), the nolfsrc originals kept for AvP2's Intel
  build behind `#ifndef PRIMAL_HUNT`. Round one wrote 28 of them, which
  took d3d.ren to 730 identical; round two 13 more (850); round three
  ten more and much of the rest (1,027); round four the big device and
  draw functions it could reach (1,071). Two of the map's
  TUs (TU21, TU33) are two objects each: two files share the position, in
  name order. There is no StdLith header difference: where retail calls
  CMoArray's or _CVector's small members out of line, it is VC6's
  inliner (in TU30, an element type's constructor), not the header. Retail is /Ob2 where a file's
  initialisers show it. TU35 is four objects (fullintersectline, gdenum, geomroutines,
  intersect_line). The renderer's shared draw state (0x1005b790) is
  `ViewParams g_ViewParams` in every file. `ph/ph_decls.h` holds the cross-file declarations
  proven against their definitions. Still open: the large unwritten draw
  functions (TU01/TU03's transform and lighting, TU06, TU07's DrawModel,
  TU10's d3d_InitSceneContext, TU15's RenderScene and its two big
  helpers, TU17's d3d_InitDevice, TU27's shadow draws, TU31's polygrid
  draw, TU33/TU34/TU37), and about 80 references that nolfsrc files
  still declare under NOLF's placeholder names.
- Next: the near functions; the provisional calls against retail; the
  flags per binary (Primal Hunt's server.dll need not be /O1 as NOLF's
  was); the links (lithshared rebuilt, package 4), then the tables.

### 8. launcher

Linked, both (`build/build.mk`, "The launchers' links": `compile.sh
launchers`). `src/launcher/` is the MFC launcher of Monolith's NOLF2 source
release brought back to the 2001 AvP2 one it descends from (with NOLF's
display-mode code, nolfsrc's), compiled as AvP2's (cl 9044, `Release/`)
and Primal Hunt's (cl 8966, `PRIMAL_HUNT`, `PrimalHunt/`) with the
project's precompiled header, against SP5's MFC; their resources are
byte-identical to the retail ones (the art read out of the retail images
at build time). Every readable function of both is byte-identical but a
few (`progress/targets.py`); five pages of each image's `.text` are kept
encrypted by the protection wrapper (`encrypted`), and the functions in
them (the rest of `RezFind.cpp`, `StaticEx.cpp`, `Utils.cpp`) are
unverifiable. To come: the modern launchers (below).

### 9. boot

`make test` (`build/playtest.sh`: tools/playtest.sh, adapted) runs each
game of a game directory under Xvfb and Wine, from its own run directory
(`out/play/test/AVP2/`, `.../PrimalHunt/`: the result, `wine.log`, a
screenshot). Both of our engines stop at once: `lithtech.exe` is linked
with `/FORCE` for the scoreboard (build.mk, "Primal Hunt's lithtech.exe
and server.dll, linked from our objects: not to run"), and every
unresolved symbol is the image base, so the first call to one jumps to
0x00400000 and dies on the header's bytes (illegal instruction at
0x0040000C). The first is in the C runtime's static initialisers, before
WinMain: `CConsoleView::CConsoleView` (our AvP2 lithtech.exe 0x00422140,
Primal Hunt's 0x0040a380) constructing the global `g_Console` calls
`CConsoleCritSec::CConsoleCritSec` (nolfsrc's 0x00444150), which no object
defines. The links leave 1,377 (AvP2's) and 1,379 (Primal Hunt's) symbols
unresolved in `lithtech.exe`, 316 and 311 in `server.dll`, 198 and 391 in
`d3d.ren` (`<dir>/<image>-link.txt`): the engines run only once those are
written or bound (nolfsrc's link-time stand-ins name NOLF's addresses).

The layout, the rez stacks and our game DLLs are shown sound with the
retail engines put in their place (`lithtech.exe`, `server.dll`,
`d3d.ren` of each retail game, in a scratch copy only): both games reach
their main menus, and AvP2 through its menus (`build/playtest-avp2-marine.txt`)
loads the first Marine mission, plays its cinematic and gives the player
control, with our `CShell.dll` and `Object.lto`.

### 12. modern

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
- the launchers as `AVP2Launcher.exe` and `PrimalHuntLauncher.exe`, with
  `AVP2.exe` and `PrimalHunt.exe` starting the game directly, as if Launch
  had been clicked.
