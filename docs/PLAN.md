# The plan

Aliens versus Predator 2 and its expansion Primal Hunt from source, split
into packages that can each be picked up again after an interruption. The
first aim is **a game that runs** with every binary built here; the second
is **byte-identity** with the retail binaries (the legacy build), ratcheted
by `make progress`. The modern build (`AVP2_MODERN`) comes after.

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
- **Legacy vs modern.** `AVP2_MODERN` gates every fix and feature, and
  `AVP2_DEBUG` the debug build's logging; the legacy build must keep
  compiling to the retail bytes.
- **Retail bugs.** Every bug found goes into `RETAIL_BUGS.md`, fixed (if at
  all) only behind `AVP2_MODERN`.
- **Commits.** Small commits, each leaving the tree building.

## Packages

| # | Package | Depends on | Status |
|---|---|---|---|
| 1 | `scaffold`: submodules (`rez`, `toolchains`, `tools`), both installs in rez, the build image, `make retail`/`reference`/`progress`, `targets.py` | -- | done (below) |
| 2 | `import`: the source release into `src/`, as it is (`src/AVP2`, `src/LT2`), with its EULA, and `docs/LICENSE_NOTES.md`, as nolfsrc's | 1 | done (below) |
| 3 | `build`: `build/env.sh`, `build/build.mk` and `build/compile.sh` (cjacksrc's, with each binary's revision), `build/all.sh` assembling a game directory for each game (with the loose files the games read: `console.PCX`, `childmodel.map`, Primal Hunt's `FontData.fnt`), `build/variant.sh` (debug, modern) | 2 | compiling (below); game directories and variants to do |
| 4 | `game`: AvP2's `CShell.dll`, `Object.lto`, `CRes.dll`, `SRes.dll` and `AVP2Serv.exe` from the release, with 8168; the `lithshared` libraries rebuilt with 8966 (WONAPI from the release's source, the rest from nolfsrc's and nolf2src's) and checked against the prebuilt ones; which configuration shipped (Release or Final); GameSpy and WON placed as `middleware` | 3 | |
| 5 | `ph-game`: Primal Hunt's game DLLs and `AVP2XServ.exe` from the same tree with `PRIMAL_HUNT`, with 8966: what Third Law added and changed, from the retail images (their strings name new classes: `CAITentacle`, `CAIChameleon`, `CMiniTurret`, `CTurretSpawner`, `CArtifactField`, dual pistols, the Corporate missions, ...); `PHCRes.dll`/`PHSRes.dll` from the release's `.rc` and the retail resources (`tools/rsrcdiff.py`) | 4 | |
| 6 | `small`: `ltmsg.dll`, `AVP2XUp.dll` and `AVP2Up.dll` (resource scripts from the retail resources), `Launch.dll` (a Debug build of NOLF's LauncherDLL) | 3 | |
| 7 | `engine`: `lithtech.exe`, `server.dll` and `d3d.ren` (8966) from nolfsrc's engine, toward Primal Hunt's; `cdaudio.dll` from the disassembly and `lithshared`'s `redmgr.h` | 3 | started (below) |
| 8 | `launcher`: `PrimalHunt.exe` and `AVP2.exe` from the retail disassembly and NOLF's launcher, without the wrapper; the rez stacks they build | 3 | |
| 9 | `boot`: the built binaries in both games under Wine, to the menu, a level, saving and loading (`make test`, tools/playtest.sh) | 4-8 | |
| 10 | `one engine`: AvP2's 1.0.9.6 game DLLs on Primal Hunt's engine, both retail and ours (if not, 1.0.9.6's engine becomes a target) | 9 | |
| 11 | `match`: byte-matching, module by module, ratcheted by `make progress` | 4-8 | |
| 12 | `modern`: the modern build (`AVP2_MODERN`, `MODERN.md`) | 9 | |
| 13 | `release`: `-private`/`-public` archives, install scripts from the player's own discs and patch (`tools/mkinstall.py`), and the GitHub Actions workflow building the public archives with `make progress` on the legacy release | 9, 12 | |

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
`out/release-tree/` (without the `AVP2_MODERN`/`AVP2_DEBUG` blocks, as
cjacksrc's). `make` compiles and links; `build/all.sh` does not yet
assemble a game directory, and `build/variant.sh` (`make debug`, `make
modern`) does not exist yet.

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

### 7. engine

Started. nolfsrc's engine (`src/engine/lithtech`, `server`, `d3d`, at
nolfsrc e2a6e25; its `NOLFSRC_MODERN`/`NOLFSRC_DEBUG` blocks renamed to ours,
its debug headers in `src/avp2src/`) is compiled with Primal Hunt's cl
12.00.8966 and nolfsrc's flags (`build/build.mk`, "The engine"; the
renderer's files all `/O2`, as nolfsrc's flag groups are Intel C/C++'s),
against AvP2's SDK headers. Objects only: the links need nolfsrc's
lithshared libraries and its link-time tables, which name NOLF's
addresses, so `make progress` scores the objects (`partial_objs`).

- 382 of 403 objects compile. The 21 that do not fail on the SDK's
  changes since NOLF: `AnimTimeRef` without `m_iWeightSet`, a key-frame
  callback with two more arguments, `LTPlane` and `LTBBox` methods made
  const and by reference, `FLAG2_PLAYERCOLLIDE`, ... (sdkforce, clientdsi,
  clientinput, commonmodel, modelanim, servermodel, smgr, worldtree, ...).
- As NOLF's: `lithtech.exe` 19.7 % byte-identical (1,443 functions, 545
  near), `server.dll` 16.0 % (887, 225 near), `d3d.ren` 3.9 % (138, 70
  near); 1,114, 649 and 794 of nolfsrc's functions in no form in Primal
  Hunt's images.
- Next: the 21 files to AvP2's SDK; the near functions; the flags per
  binary (Primal Hunt's server.dll need not be /O1 as NOLF's was); the
  links (lithshared rebuilt, package 4), then the tables.

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
