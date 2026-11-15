# The plan

This plan builds Aliens versus Predator 2 and its expansion Primal Hunt
from source. The plan is split into packages. After an interruption, you
can start each package again. The first aim is **a game that runs**, with
every binary built here. The second aim is **byte-identity** with the
retail binaries (the legacy build). `make progress` ratchets it. The modern
build (`AVP2SRC_MODERN`) comes after.

## What "the game" is

The last release is *Primal Hunt* 1.0.2.2 (Third Law, June 2002). Its
readme says that it "includes all of the changes made in Aliens Versus
Predator 2 patches up to version 1.0.9.6". Its engine (`lithtech.exe`
2002-05-20, `server.dll`, `d3d.ren`, `cdaudio.dll`), launcher, dedicated
server and game DLLs are all newer than those of AvP2 1.0.9.6 (February to
April 2002). No later patch for it is known. But Primal Hunt installs beside
AvP2. It needs AvP2's archives (`AVP2.REZ`, `SOUNDS.REZ`, the world
archives of the three campaigns). Also, "you cannot play Aliens Versus
Predator 2 singleplayer missions from within" it: the AvP2 campaigns belong
to AvP2's game DLLs. Thus, the target is:

- **Primal Hunt 1.0.2.2 (Europe)**, all of it: the engine, `cdaudio.dll`,
  `ltmsg.dll`, `Launch.dll`, `PrimalHunt.exe`, `AVP2XUp.dll`,
  `AVP2XServ.exe`, and its game DLLs (`PHCShell.dll`, `PHObject.lto`,
  `PHCRes.dll`, `PHSRes.dll`). In its `AVP2DLL.REZ` and `AVP2L.REZ`, these
  DLLs have the names `CSHELL.dll`, ....
- **AvP2 1.0.9.6 (Europe)**, the parts that Primal Hunt does not replace:
  its game DLLs (`CShell.dll`, `Object.lto`, `CRes.dll`, `SRes.dll`), its
  launcher (`AVP2.exe`, `AVP2Up.dll`) and its dedicated server
  (`AVP2Serv.exe`).

AvP2 1.0.9.6's engine is superseded. We keep it as a witness, not as a
target. Package 10 is to show if Primal Hunt's engine runs AvP2's 1.0.9.6
game DLLs (one engine for both games). If it does not, 1.0.9.6's engine
becomes a target again.

Of these games, AvP2 is the closest to NOLF. It has the same engine
generation (LithTech "Talon", NOLF's engine of 2000, developed further). It
has the same compilers, the static C runtime, and the separate Direct3D
renderer `d3d.ren`. Its game code was released. Its engine was not
released, and nothing of Primal Hunt's was released. This table shows what
there is, and what we use it for:

| Source | What it is | For |
|---|---|---|
| The AvP2 source release (`../avp2/proj`, "Source Code v1.0.9.6", July 2002, with its EULA) | Monolith's own `AVP2` tree at 1.0.9.6 (`IDS_VERSION "1.0.9.6"`). It has ClientShellDLL, ObjectDLL, ClientRes, ServerRes, the dedicated server (AVP2Serv), Shared, and the workspace `AVP2.dsw` with VC6 projects (Debug, Release, Final, Demo). It also has `LT2`: the engine SDK headers (`sdk/inc`) and the `lithshared` libraries (ButeMgr, StdLith, CryptMgr, RezMgr, LTGuiMgr, GameSpy, WONAPI, zlib, ...). For these libraries, it has their headers and prebuilt builds (`lithshared/libs`, cl 8966), and the source of WONAPI and STLport only. It has no Primal Hunt code | AvP2's game DLLs, their resources and the dedicated server, as Monolith built them, and the libraries. It is also the base of Primal Hunt's game DLLs, which continue this code. We import it as it is, the same as nolfsrc's `src/game/` is NOLF's release |
| nolfsrc's engine (`../nolfsrc/src/engine`) | NOLF's engine (`lithtech.exe`, `server.dll`, `d3d.ren`), reconstructed to compile to the retail bytes of NOLF GOTY. Every function is written. 77 % of the code of `lithtech.exe` is identical, 85 % of the code of `server.dll`, and 28 % of the code of `d3d.ren` | The engine: Primal Hunt's engine is the same line two years later. It is the base for `lithtech.exe`, `server.dll` and `d3d.ren`. Its `clientwin.cpp` already calls the `GetLithTechCommandLine` of `Launch.dll` |
| nolf2src's and cjacksrc's engines (Jupiter) | The next engine generation, reconstructed | Where Talon's code survived into Jupiter, they show later shapes of the same functions. (The music driver of `cdaudio.dll` is the ancestor of Jupiter's `musicdriver.cpp`.) |
| AvP2 1.0.9.6's engine (`rez`'s `avp2-eu-1.0.9.6`) and the engine of the 1.0 release (`avp2-eu`) | The engines between NOLF's and Primal Hunt's | They show what changed between them. They are a second and a third witness for each engine function |
| lithtools' Talon tools (`../lithtools`, `legacy/`) | AvP2's DEdit and ModelEdit, rebuilt to their shipped bytes. Also the AvP2 tool releases (`rez`'s `avp2-tools`, `-update1`, `-update2`), binaries only. Primal Hunt's disc also has three AvP2 tools installers in `Tools/` | The engine-side code that the tools share (world, model and texture formats, the preprocessor) |
| The demo disc (`rez`'s `avp2-demo-usa`, image only) | The single-player and multiplayer demo installers (`avp2spdemo.exe`, `avp2mpdemo2.exe`) | The engines and game DLLs of the demos. The Demo configurations of the release tell what they were built from |
| The GameSpy SDK and WON's API (`lithshared/gamespy`, `lithshared/wonapi`) | In the source release: GameSpy's headers, and WONAPI's source and project files | The `middleware` blocks to put in the game DLLs and the engine |
| nolfsrc's and nolf2src's `lithshared` (`../nolfsrc/src/lithshared`, `../nolf2src/src/libs`) | The libraries that NOLF's release shipped prebuilt, reconstructed (nolfsrc). Also the NOLF2 release's own source and VC6 projects for most of them (StdLith, ButeMgr, CryptMgr, RegMgr, MFCStub, zlib, LTGUIMgr, RezMgr) | The libraries that AvP2's release ships only prebuilt |

We could not find an AvP2 or Primal Hunt engine source, PDB or map
anywhere (2026-11). We could not find any Primal Hunt game source.

## The retail games

`ext/rez/` installs both (`build/retail.sh`). They are the European
releases:

- `ext/rez/games/avp2-ph-eu`: Primal Hunt (Europe) 1.0.2.2, "Typical".
  `build/retail.py` gives its layout, for the tools. The European and USA
  releases are the same files. The Italian release is different only in
  its language archive, its launcher and its dedicated server. Its
  archives are:
  - `AVP2DLL.REZ` (its own `cshell.dll`, `object.lto`);
  - `AVP2L.REZ` (`cres.dll`, `sres.dll`);
  - `AVP2P.REZ`;
  - `AVP2P1.REZ` (1.0.9.6's, the same file);
  - `AVP2X.REZ` (Primal Hunt's worlds and data);
  - `MULTI.REZ`;
  - `DIALOGUE.REZ`.

  Its launcher builds the rez stack from these archives and AvP2's install
  (`InstallDir` in AvP2's registry key).
- `ext/rez/games/avp2-eu-1.0.9.6`: AvP2 (Europe) with the English 1.0.9.6
  patch (April 2002). `build/retail_avp2.py` gives its layout. This patch is
  the last one, and it includes the earlier ones. AvP2 had no GOTY edition.
  The patch put a new engine, renderer and dedicated server over the
  install. It replaced `AVP2DLL.REZ` and `AVP2L.REZ` whole. It added
  `AVP2P1.REZ` (data) last in the rez stack. The launcher, `ltmsg.dll`,
  `Launch.dll` and `AVP2Up.dll` are those of the discs.

| Binary | Of | What | Size | Functions (own code, enumerated) |
|---|---|---|---|---|
| `lithtech.exe` | PH | The engine. It uses Miles (`mss32.dll`) and DirectInput | 984 KB | 4,483 |
| `server.dll` | PH | The server of the engine | 596 KB | 3,136 |
| `d3d.ren` | PH | The Direct3D renderer (DirectDraw/D3D 7) | 356 KB | 1,301 |
| `cdaudio.dll` | PH | The CD-audio music driver of the engine (`CRedMgr`, MCI; MFC, static) | 100 KB | 90 |
| `ltmsg.dll` | PH | The message strings of the engine | 44 KB | resources |
| `Launch.dll` | both | NOLF's launcher DLL (1999, a Debug build). It holds the engine's `GetLithTechCommandLine` | 236 KB, nearly all of it the debug CRT | 43 |
| `PrimalHunt.exe` | PH | The launcher (MFC, static). `.cms_t`/`.cms_d` are the sections of a protection wrapper | 6.9 MB, nearly all of it resources | 357 |
| `AVP2XUp.dll` | PH | Resources for the Sierra updater | 620 KB | resources |
| `AVP2XServ.exe` | PH | The dedicated server (MFC, static). It imports `server.dll` | 1.2 MB | 354 |
| `PHCShell.dll` | PH | The game: client | 2.1 MB | 6,158 |
| `PHObject.lto` | PH | The game: server objects | 3.2 MB | 10,704 |
| `PHCRes.dll`, `PHSRes.dll` | PH | The resources of the game | 492 KB, 44 KB | resources |
| `AVP2.exe` | AvP2 | The launcher (MFC, static), with the same wrapper | 5.1 MB, nearly all of it resources | 351 |
| `AVP2Up.dll` | AvP2 | Resources for the Sierra updater ("AVP2 Customization DLL") | 244 KB | resources |
| `AVP2Serv.exe` | AvP2 | The dedicated server | 1.2 MB | 355 |
| `CShell.dll` | AvP2 | The game: client | 2.0 MB | 6,852 |
| `Object.lto` | AvP2 | The game: server objects | 3.1 MB | 11,189 |
| `CRes.dll`, `SRes.dll` | AvP2 | The resources of the game | 436 KB, 44 KB | resources |

There are 45,373 functions in all. Every binary is VC6, in four revisions
(`progress/COMPILERS.md`). All of Primal Hunt's binaries are SP5 (8966),
its renderer included. Nothing in either engine is protected or encrypted.
These parts are third-party, and this project does not build them:

- Bink;
- Smacker;
- Miles (with its MP3 decoder);
- the Sierra updater (`SierraUp.exe`, `SierraPt.dll`, and Primal Hunt's
  `SuDE.dll`, `SuES.dll`, `SuFR.dll`, `SuIT.dll`).

## Ground rules (every package)

- **Source.** One game tree builds both games. This tree is the source
  release, kept as Monolith wrote it. Where AvP2's retail 1.0.9.6 binaries
  do something different from the release, change the code in place. Mark
  the change `RETAIL:` with the retail address (`0x...`), as nolfsrc marks
  `GOTY:`. Put Primal Hunt's changes behind `PRIMAL_HUNT`. The Primal Hunt
  configurations of the projects define `PRIMAL_HUNT`. Mark these changes
  `PH:` with the address in `PHCShell.dll` or `PHObject.lto`. New files of
  Third Law's are new files. Engine code starts from nolfsrc's engine, and
  we bring it to Primal Hunt's engine. Where no code exists, write it from
  the retail disassembly, and cite the address.
- **Compiler.** Use the revision that the Rich header of each binary names:
  - 8966 for all of Primal Hunt's binaries. Use 8168 for the objects of
    its game DLLs and dedicated server where the header says so.
  - 8168 for AvP2's game DLLs and dedicated server.
  - 9044 for AvP2's launcher.
  - 8966 for the `lithshared` libraries.

  Prove the flags for each binary, as nolfsrc did.
- **Project files.** Use the release's VC6 `.dsw`/`.dsp` files
  (`ext/tools/dsp2mk.py`). Add a Primal Hunt configuration beside Release.
- **Legacy vs modern.** `AVP2SRC_MODERN` gates every fix and feature.
  `AVP2SRC_DEBUG` gates the logging of the debug build. The legacy build
  must continue to compile to the retail bytes.
- **Retail bugs.** Put every bug that you find into `RETAIL_BUGS.md`. If
  you fix a bug, fix it only behind `AVP2SRC_MODERN`.
- **Commits.** Make small commits. After each commit, the tree must build.

## Packages

| # | Package | Depends on | Status |
|---|---|---|---|
| 1 | `scaffold`: the submodules (`rez`, `toolchains`, `tools`), both installs in rez, the build image, `make retail`/`reference`/`progress`, and `targets.py` | -- | done (below) |
| 2 | `import`: the source release into `src/`, as it is (`src/AVP2`, `src/LT2`), with its EULA, and `docs/LICENSE_NOTES.md`, as in nolfsrc | 1 | done (below) |
| 3 | `build`: `build/env.sh`, `build/build.mk` and `build/compile.sh` (cjacksrc's, with the revision of each binary). `build/all.sh`, which assembles a game directory for each game (with the loose files that the games read: `console.PCX`, `childmodel.map`, Primal Hunt's `FontData.fnt`). `build/variant.sh` (debug, modern) | 2 | done (below) |
| 4 | `game`: AvP2's `CShell.dll`, `Object.lto`, `CRes.dll`, `SRes.dll` and `AVP2Serv.exe` from the release, with 8168. The `lithshared` libraries, rebuilt with 8966 (WONAPI from the release's source, the others from nolfsrc's and nolf2src's) and checked against the prebuilt ones. Which configuration shipped (Release or Final). GameSpy and WON, put in as `middleware` | 3 | |
| 5 | `ph-game`: Primal Hunt's game DLLs and `AVP2XServ.exe` from the same tree with `PRIMAL_HUNT`, with 8966. What Third Law added and changed, from the retail images. (Their strings name new classes: `CAITentacle`, `CAIChameleon`, `CMiniTurret`, `CTurretSpawner`, `CArtifactField`, dual pistols, the Corporate missions, ....) `PHCRes.dll`/`PHSRes.dll` from the release's `.rc` and the retail resources (`ext/tools/rsrcdiff.py`) | 4 | |
| 6 | `small`: `ltmsg.dll`, `AVP2XUp.dll` and `AVP2Up.dll` (resource scripts from the retail resources), and `Launch.dll` (a Debug build of NOLF's LauncherDLL) | 3 | `ltmsg.dll` done (below) |
| 7 | `engine`: `lithtech.exe`, `server.dll` and `d3d.ren` (8966) from nolfsrc's engine, toward Primal Hunt's engine. `cdaudio.dll` from the disassembly and the `redmgr.h` of `lithshared` | 3 | started (below) |
| 8 | `launcher`: `PrimalHunt.exe` and `AVP2.exe` from the retail disassembly and NOLF's launcher, without the wrapper. The rez stacks that they build | 3 | linked (below) |
| 9 | `boot`: the built binaries in both games under Wine, to the menu, to a level, and through a save and a load (`make test`, ext/tools/playtest.sh) | 4-8 | The harness is done. Both engines stop before WinMain (below) |
| 10 | `one engine`: AvP2's 1.0.9.6 game DLLs on Primal Hunt's engine, both retail and ours. If they do not run, 1.0.9.6's engine becomes a target | 9 | Answered: not as shipped (package 7). 1.0.9.6's engine is to be a target |
| 11 | `match`: byte-matching, module by module. `make progress` ratchets it | 4-8 | |
| 12 | `modern`: the modern build (`AVP2SRC_MODERN`, `MODERN.md`) | 9 | Built (`make modern`). No item is done yet |
| 13 | `release`: the `-private`/`-public` archives. Install scripts that use the player's own discs and patch (`ext/tools/mkinstall.py`). The GitHub Actions workflow, which builds the public archives with `make progress` on the legacy release | 9, 12 | Done (below). The workflow has not run on GitHub yet |

## Notes per package

### 1. scaffold

Done.

- `ext/rez/` installs `avp2-ph-eu` and `avp2-eu-1.0.9.6`
  (`ext/rez/tools/install.py`). `ext/rez/games.sha256` records both:
  - `avp2-ph-eu` is the European release, "Typical".
  - `avp2-eu-1.0.9.6` is the European release, "Typical", then the
    patch's `Update Files` (`Update Command Line` `-rez AVP2P1.REZ`).
- `ext/toolchains/` has every revision that the headers name:
  `msvc-6.00-8168`, `-8447`, `-8966`, `-9044`, and `icl-4.0`.
- `ext/tools/retailextract.py` reads Primal Hunt's layout from
  `build/retail.py`, and AvP2's layout from `build/retail_avp2.py`
  (`RETAIL_LAYOUT`). `build/reference.sh` runs it on both into one
  `out/reference/`. Primal Hunt's game DLLs are under `PH` names.
- `make reference` extracts the 20 retail binaries and enumerates them.
  `make progress` scores 45,373 functions, all unwritten.
- The `app` boundaries in `targets.py` are provisional. Each one is the
  first runtime member that `libscan.py` finds. When the binary links, pin
  each boundary from our own `.map`, as nolfsrc did. Below several of them,
  there is a stretch of packed code that can also be runtime. `targets.py`
  notes each one.
- Unknowns:
  - the objects without a `@comp.id` in `lithtech.exe`, `cdaudio.dll`, the
    launchers and the dedicated servers;
  - the wrapper of the launchers (`.cms_t`, `.cms_d`);
  - `PHCShell.dll`'s WONAPI. It is linked above the runtime. It matches the
    release's prebuilt library much less than 1.0.9.6's WONAPI does;
  - the 0x25/8755 and 13.00.8830 objects of AvP2 1.0.9.6's `d3d.ren` (now
    a witness, not a target).

### 2. import

Done. The release (`../avp2/proj`) is in `src/AVP2` and `src/LT2` as it
came. Every text file has LF line endings. (`credits.txt` keeps CRLF.
`CRes.dll` embeds it. See `.gitattributes`.) Its readme and Fox's EULA are
`src/MONOLITH-README.txt` and `src/MONOLITH-EULA.doc`. We did not include
the Miles and Bink binaries of `LT2/sdk/win/lib`. The prebuilt
`lithshared` libraries and `AVP2Serv/server.lib` are in
`progress/reference/`, with their `SHA256SUMS`. `docs/LICENSE_NOTES.md`
tells what is whose.

`LT2/lithshared` contains the headers of the libraries: ButeMgr, CryptMgr,
RegMgr, RezMgr, RedMgr (the `CRedMgr` of `cdaudio.dll`), StdLith,
LTGUIMgr, LithFontMgr, GameSpy, zlib, .... It contains the source of only
two: WONAPI (198 files, with its own VC6 projects) and STLport. The others
ship prebuilt only, as NOLF's did.

### 3. build

`build/build.mk` builds the release's own workspace
(`src/AVP2/AVP2.dsw`) through `ext/tools/dsp2mk.py`. `dsp2mk.py` expands
the `$(LT2_DIR)` of the project files to `S:\LT2` (`--macro`, `--drive`),
as Developer Studio does with the release's `SetBuildVars.bat`.
`build/compile.sh` builds in `out/release-tree/`, without the
`AVP2SRC_MODERN`/`AVP2SRC_DEBUG` blocks, as cjacksrc's does.

`make` (`build/all.sh`) builds both games into one game directory,
`out/avp2-legacy-release/`. The layout is the same as the retail setups
install:

- `AVP2/`: AvP2 1.0.9.6, with its own engine, launcher, server and game
  DLLs.
- `PrimalHunt/`: the same for Primal Hunt. Primal Hunt reads AvP2's
  archives from `../AVP2/`.

Each game has its game DLLs in `custom/`, the last in its rez stack
(`build/avp2cmds.sh`). The build makes the game directory from our
binaries and the retail data:

- the `.rez` archives, with the retail game DLLs set to zero;
- the movies;
- the loose files;
- the readmes;
- the shipped settings;
- Bink, Smacker and Miles, as the third-party runtime.

Then the build audits and packages the game directory
(`build/package.sh`): -private, and -public with the install scripts of
`build/mkinstall.py`.

`build/variant.sh` builds `make debug` (`AVP2SRC_DEBUG`, symbols),
`make modern` (`AVP2SRC_MODERN`) and `make modern-debug`. It builds them
in copies of the tree under `out/build-<variant>/`.

- Which configuration: **Final** for the game DLLs (`_FINAL`). The release
  keeps some diagnostics under `#ifndef _FINAL`. None of them is in the
  retail `Object.lto` of either game. The resource DLLs and the dedicated
  server have only a Release configuration.
- Which tools:
  - for the game DLLs, cl 8168, with the link (6.00.8447), headers and
    libraries of msvc-6.00-8966;
  - for the dedicated server, the tools of 8168 throughout;
  - rc of msvc-6.00-9782 (`progress/COMPILERS.md`).
- Until this project builds them, the build copies the prebuilt
  `lithshared` libraries and `server.lib` from `progress/reference/` to
  where the project files name them. The score does not count them on our
  side (`progress/targets.py`, `PREBUILT_LIBS`).

### 4. game

Started. We built the release as it came, as above. It scores `Object.lto`
89 %, `CShell.dll` 62 % and `AVP2Serv.exe` 60 % byte-identical, and every
resource of `CRes.dll` and `SRes.dll`. Most of the unwritten functions of
`CShell.dll` are those of the prebuilt libraries (they are not counted as
ours). Two functions of our client are in no form in the retail client
(unplaced): `CWeaponFX::CreateBulletTrail` and `OverlayMGR::GetRailMode`.
The release's source is dated July 2002, and retail 1.0.9.6's `CShell.dll`
is dated March 2002. Thus, the release can be a little newer than what
shipped.

`CShell.dll` is the output of two compilers (progress/COMPILERS.md). 83 of
its sources were rebuilt with SP5's cl 8966 after Monolith moved to it.
The other 105 are RTM's 8168. We proved each one when we compiled it both
ways (`build/build.mk`, `CSHELL_8966`). With them, every function of the
client's own code that the scorer can verify is identical, except the
destructor of CSoundButeMgr. (The scorer can verify all functions except
the short relocation-only ones, which it scores small.) That destructor is
8966's code in an 8168 object, as in `Object.lto`. In all: 4,412 -> 4,782.
Neither unplaced function was missing:

- `CreateBulletTrail` is 8966's code (with one more inline Init).
- Only a tail jump reaches `OverlayMGR::GetRailMode`. Thus, the
  enumeration had merged it into the function before it
  (`progress/extent-splits.csv`).

### 4. game: the lithshared libraries

Six of the libraries are built from nolfsrc's sources (`src/lithshared/`),
with cl 8966 and nolfsrc's flags. They go into the paths that the project
files link them from, so the game DLLs link ours (`build/build.mk`, "The
lithshared libraries, ours"). The scorer compares each one with the
release's prebuilt build (`progress/targets.py`, `LIBS`):

- CryptMgr and RegMgr: 100 %;
- StdLith: 99.2 %;
- MFCStub: 96.2 %;
- lith: 43.8 %;
- RezMgr: 0.3 %.

With them, `CShell.dll` is 62.7 % and `Object.lto` is 89.1 %.

RezMgr is now Monolith's (`src/lithshared/rezmgr`). It is the
rezmgr.cpp, rezhash.cpp and rezfile.cpp of NOLF2's release, which are the
three objects that the shipped builds hold, in AvP2's REZMGRLITE build.
RezMgr and StdLith are also built a second time as the release's Debug
builds (/MTd /ZI /Od). Primal Hunt's `server.dll` links these
(`build/build.mk`, "The debug builds"). With /ZI, every assert carries its
line relative to its function. Thus, the debug builds also fix the line
layout of the source. The release's prebuilt `Rezmgrfull.lib` is the full
build of the same file (/Z7, with COFF line numbers). It shows where the
code that REZMGRLITE leaves out was. All four builds (RezMgr and StdLith,
release and debug) are 100 %.

ButeMgr is NOLF2's (`src/lithshared/ButeMgr`). AvP2's parser is on NOLF2's
line (STLport symbol tables, the same four objects), but at an earlier
stage. It has CString accessors and a second tag table. Ours takes these
from AvP2's header (and the CString `AddString` from the release's
prebuilt build). ButeMgrNoMFC is 72.5 % byte-identical. Its error reports
(`File "%s", line %d`, an `EXPANSION` keyword) are still NOLF2's
(progress/data-known.txt).

Two more libraries are not NOLF's. They stay prebuilt (and out of our
score) until we reconstruct them from the release's builds:

- LTGUIMgr (menu fonts with three faces, DrawMenu, GetTextExtents);
- LithFontMgr (a CString for the previous string, a default colour).

By its score, lith is a different version too. These are prebuilt and not
ours to build: GameSpy's managers (middleware) and LithTrackMgr (headers
only). WONAPI's source is in the release, and we will build it from that
source.

### 5. ph-game

Third Law's game code is AvP2 1.0.9.6's code, continued. We looked at the
distinct strings of 8 characters or more:

- 65 % of those in Primal Hunt's client are also in 1.0.9.6's client.
- 58 % of those in its server objects are also in 1.0.9.6's server
  objects.
- The others are new (1,840 in the client, 3,403 in the server objects).

This is the method:

1. Build the `PRIMAL_HUNT` configuration from the release, unchanged.
2. Score it against `PHCShell.dll`/`PHObject.lto`. With 8966, we expect
   the codegen of unchanged files to match outright.
3. Work outward from what does not match.

Started. `src/AVP2/AVP2PH.dsw` and the `_PH` projects
(`build/phproject.py`) build the release's sources unchanged with cl 8966
and `PRIMAL_HUNT`. Then `PHObject.lto` is 46.7 % byte-identical and
`PHCShell.dll` is 26.1 %. 2,302 and 1,794 more functions are "near": the
same function, but changed (by Third Law's edits, or because one of the
objects is still 8168's). 1,109 and 61 of ours are unplaced (code that
Primal Hunt dropped or rewrote).

The dedicated servers. The Rich header of `AVP2Serv.exe` has no object of
cl 8966. The butemgr.lib (MFC's build), cryptmgr.lib and StdLith.lib that
it links were 8168's, not the release's prebuilt ones. Thus,
`build/build.mk` ("The dedicated servers") builds them with 8168 into the
project's directory. With them, all of the server's own code is
byte-identical. What remains is ButeMgr's code (the AvP2 version of it, as
for ButeMgrNoMFC).

`AVP2XServ.exe` is `ServerApp_PH.dsp` in `AVP2PH.dsw` (the server's
Release configuration with `PRIMAL_HUNT`). It is built with cl 8966
against SP5's MFC and runtime, the release's libraries and RegMgr32.lib.
All of its resources are byte-identical (the build reads the splash out of
the retail image). These parts are reconstructed so far:

- the class and weapon restrictions: the two lists of CServerOptions, the
  Weapon and Class sections of LoadFromBute, the tables and masks of
  FillInHostInfo, and the defaults of MPGameInfo. The server's lists are
  their own class, CMPAllowList, because any change to MPRestrictList.h
  moves unrelated PHObject and PHCShell code;
- its ButeMgr: MFC's build with cl 8966, ours (`ButeMgr` in OUR_LIBS);
- `AddResources` (AvP2's rez files from AvP2's install directory, which it
  reads from the registry);
- `OnInitDialog`;
- the default port (27890);
- Primal Hunt's default names and passwords.

Still to come: Third Law's class and weapon restrictions
(`CServerOptions`, `MPGameInfo`: `progress/data-known.txt`).
`PHCShell.dll`'s copy of `ServerOptions.cpp` shares them.

Round of 2026-11 (six agents): we reconstructed these layout changes of
Third Law's behind `PRIMAL_HUNT`:

- GameBase's ten new virtual message handlers;
- DamageStruct's position;
- CSpecialFX::Term made virtual;
- CharacterButes' DefaultEnvMap;
- the CMS_TURRET state;
- the bute cache (`Shared/MultiFile.h`, the Write and Read of every FX
  struct);
- the dual-muzzle weapons, the subtitle and self-destruct FX, carried
  turrets, and the CLTGUICtrl fields.

The game libraries are ours: WONAPI from the release's source, LTGUIMgr,
LithFontMgr and ButeMgr. The sources of CShell.dll are split between cl
8168 and cl 8966 (`CSHELL_8966`). Scores (identical functions):
PHCShell.dll 4,090, PHObject.lto 7,712, CShell.dll 5,748, Object.lto
8,513.

Still to do:

- Find which file defines the sine/cosine constants of AIUtils.h. Primal
  Hunt initialises them once, at 0x1021a540 after SimpleNodeMgr. They are
  the cause of about 1,070 of PHObject's unplaced functions.
- A `PRIMAL_HUNT` build of LTGUIMgr.
- Primal Hunt's WONAPI (the VC6-STL build). For this, its GameSpy managers
  must be rebuilt.
- The vtable-vote tie of the scorer (a table that ties gets no
  placement).
- The PHObject rows that the enumeration splits.
- The size of Door. (The other server-side layouts are in: the class and
  weapon tables of MPGameInfo, the "_MP" restriction lists of CWeaponMgr
  and CCharacterButeMgr (`Shared/MPRestrictList.h`), the WeaponRef of
  PickupButes, DT_BACKHAND, and the extra model file-name argument of
  ILTServer.)
- The bute cache's readers and writers of the other managers (FXButeMgr's
  Init, ReadCache and WriteCache). `Shared/MultiFile.cpp` and
  CCharacterButeMgr's are in.
- The class and weapon lists of CServerOptions (PHCShell.dll's
  ServerOptions.cpp, AVP2XServ.exe). CMPRestrictList is the layout.
- Primal Hunt's player turrets (CTurretGun, MID_PLAYER_TURRETS and
  MID_TURRET_USE: StartDeath, the use of the turret), and the footstep
  listeners (CCharacter::HandleFootstepKey, 0x1014baa0).

### 6. small

`ltmsg.dll` of both releases: `src/engine/ltmsg/ltmsg.rc` is read out of
the retail DLLs by `ext/tools/rsrc2rc.py`. (Primal Hunt's DLL and
1.0.9.6's DLL have the same resources.) The resource script is built
around nolfsrc's `ltmsg.cpp`. Primal Hunt's DLL is built with SP5's tools
into `Release/`. 1.0.9.6's DLL is built with RTM's tools into
`AvP2Release/`. Every resource is byte-identical in both.

### 7. engine

Started. nolfsrc's engine (`src/engine/lithtech`, `server`, `d3d`, at
nolfsrc e2a6e25) is compiled with Primal Hunt's cl 12.00.8966 and
nolfsrc's flags (`build/build.mk`, "The engine"), against AvP2's SDK
headers. Its `NOLFSRC_MODERN`/`NOLFSRC_DEBUG` blocks are renamed to ours,
and its debug headers are in `src/avp2src/`. All the files of the renderer
are `/O2`, because nolfsrc's flag groups are Intel C/C++'s. The output is
objects only. The links need nolfsrc's lithshared libraries and its
link-time tables, which name NOLF's addresses. Thus, `make progress`
scores the objects (`partial_objs`).

- Every object compiles against AvP2's SDK (403). This list shows what the
  SDK changed since NOLF, and how the code meets each change (`AVP2:`
  comments; `provisional` where the retail code must still confirm it):
  - DirectInput 7 under the `dinput.h` of the DirectX 8.1 SDK
    (`DIRECTINPUT_VERSION 0x0700`, as the retail call asks: 0x442c55);
  - the weight set moved from the tracker's `AnimTimeRef` to each key
    frame. `GetWeightSet`/`SetWeightSet` are from retail (server.dll
    0x10024f57, 0x10024fa1: during interpolation, the previous key frame
    keeps the old set). The transform, world-tree and anim-message uses
    take the set of the current key frame, provisionally;
  - the two new arguments of the key-frame callbacks, and the client
    shell's `OnModelKey` (the tracker's number) and `SetDisconnectCode`
    (no sub-reason): provisional;
  - `FLAG2_PLAYERCOLLIDE` is AvP2's `FLAG2_CYLINDERPHYSICS`, the same bit;
  - the new `GetCRC` of `ILTCommon` (CRC-32, 0x10016b42), and
    `GetPolyInfo` and `GetPolySurfaceFlags` (0x10039e3e, 0x10039ed4), from
    retail. They use Primal Hunt's world polygon layout (its surface at
    +0x2c, its vertices its own). `GetPolyTextureFlags` now reads this
    layout too;
  - the `UpdateEffect` of surface effects returns nothing: cast,
    provisionally;
  - the member pointers of `sdkforce.cpp` take AvP2's const signatures.
- **Primal Hunt's engine has an `ILTCommon` slot that AvP2's SDK does not
  have**: `GetPolyTextureName` (slot 19; the client's error tag names it,
  lithtech.exe 0x00481280). It moves `GetPolyInfo` and
  `GetPolySurfaceFlags` one slot down. It is in `iltcommon.h` behind
  `PRIMAL_HUNT`, and the engine is now built with `PRIMAL_HUNT`. AvP2
  1.0.9.6's engine has no such slot (its `ServerCommonLT` table has 21).
  AvP2's game calls both shifted functions (CharacterMovement.cpp,
  AINode.cpp, ...). Thus, **AvP2's 1.0.9.6 game DLLs cannot run on Primal
  Hunt's engine as shipped**. This answers package 10: AvP2 1.0.9.6's
  engine becomes a target again. Primal Hunt's own game DLLs use the
  22-slot interface. PHCShell.dll 0x101050f0 calls `GetPolySurfaceFlags`
  through `[edx+0x54]`, slot 21, and it matches when we build it with this
  interface.
- **Primal Hunt's `ILTServer` and `ILTClient` have three virtuals that
  AvP2's SDK does not have** (server 128 slots against 125, client 113
  against 110). We found them when we aligned the targets of the two
  tables instruction by instruction:
  - two in `ILTCSBase` after `CopyFile` (slots 44 and 45, on both sides).
    They are file queries through the file manager: `GetFileInfo` into a
    new `LTFileInfo`, and `DoesFileExist` (server 0x10037780 and
    0x10037808; the names are ours);
  - `GetPolyTextureName`, which forwards to the new slot 19 of
    `ILTCommon` (server slot 126 after `GetPolyTextureFlags`; client slot
    108, before `StartQuery`).

  All three are in the SDK headers behind `PRIMAL_HUNT`. Thus, Primal
  Hunt's game calls through `g_pLTServer` and `g_pLTClient` at slot 44 or
  later now land eight bytes on. The senders of `ILTServerSendView` move
  with them (slots 90-96).
- **Primal Hunt's file-find state is 0x104 bytes (MAX_PATH) bigger**:
  every frame that holds one is 0x214, where AvP2's is 0x110.
  (`fs_FindFileInTrees`, 0x10037836, was a near only because of the
  displacement widths.) Nothing that is reconstructed yet reads the new
  field.
- With the three virtuals and the AvP2 model/object layouts in, Primal
  Hunt's game changed as follows:
  - PHCShell.dll went from 2,540 to 2,874 identical functions (27.7 % to
    36.7 % of bytes);
  - PHObject.lto went from 5,497 to 5,918 (47.5 % to 53.2 %).

  893 functions are newly identical across the targets, 52.5 % of all
  bytes. We gave up ten recorded ones:
  - two `AIBM_*Sense` destructors. Their base-vtable store maps to a table
    other than retail's (Primal Hunt's AIButeMgr changes, package 5). They
    had matched by a coincidence of mapping;
  - eight small or initialiser functions in PHObject.lto and lithtech.exe.
    Their placement moved when their neighbours matched.
- **The client side's NOLF layouts, moved to AvP2's** (the near list of
  lithtech.exe, six object groups worked in parallel through
  `build/near.sh`): lithtech.exe 2,007 -> 2,409 identical functions
  (27.9 % -> 37.6 % of bytes). AvP2 1.0.9.6's went 2,102 -> 2,475 from
  the same source. Nearly every near was layout, not codegen. In every
  case that we checked against both images, AvP2 1.0.9.6 shares the
  layout:
  - `CClientMgr`: +4 before `m_NetMgr` (0x510). The light scale is at
    0x76c, then a new vertex tint is at 0x778. The sound manager is at
    0x784 and is 0x910 bytes. From the music manager (0x1094) on, there is
    +0x110, so the connection is at 0x161c and the file manager is at
    0x1644. The total is 0x22c8.
  - `CLTClient`: the function table of the AvP2 SDK (212 entries, with
    `SetGameInfo` and the three vertex-tint calls), and the client
    manager at 0x3d0. `ClearScreen` takes the colour.
  - client objects: the change flags are at 0x124, and the own fields of
    each type start at 0x1b0. `RenderStruct` is +8 from `ClearScreen`
    (0x8c). `SharedTexture` is 0x40 bytes. The sound instance is +8 from
    0x38, and it is 0xc4 bytes. `CLTDirectMusicMgr` has three playing
    lists and VOLUMEOFFSET. `IClientShell`, `IWorldBsp` and the server
    shell are each two or three slots longer. The message cache is 0x44c.
  - Primal Hunt only (`PRIMAL_HUNT`):
    - the client's default port 27890 (its server.dll and both AvP2
      images keep 27888);
    - the MAX_PATH of the find record;
    - the model's extra texture at +0x1d0, and the +4 that it puts on the
      object after it;
    - the texture null test of `tm_FreeTexture`;
    - the two ILTCSBase slots in every view of CLTClient's own vtable.
- Header changes that we found and must still make. Each one blocks a few
  nears. The evidence is in the agents' notes beside the views:
  - `sb_Free` in `servermgr.h` needs AvP2's `if (pStruct && pBank)`
    guard;
  - the first test of `sm_SetObjectChangeFlag`;
  - SObjData +0xc in Primal Hunt (`m_StateLink` 0x48, NetFlags 0x5e;
    AvP2's 0x5a);
  - the `GetWorldFlags` of `IWorldBsp` at 0x20 in the engine headers;
  - the `m_Unknown390` of `CServerMgr` at 0x394;
  - `ILTModel` at 38 slots;
  - the extra 128-byte name of `ObjectCreateStruct` at +0x300, and the
    byte of `PlaySoundInfo` at +0x134 (Primal Hunt);
  - the out-of-line allocator of the collection templates;
  - `clientsound.h`;
  - `CWorldSurface` 0x40 bytes and `CWorldPart` +4.
- **Primal Hunt's engine is built with `/GX` and against VC6's own STL**
  (progress/COMPILERS.md). AvP2 1.0.9.6's engine has neither (no exception
  frames, and STLport first on the include path). This has three
  results:
  - Every Primal Hunt function with a destructible local has an exception
    frame. VC6 drops a state store before a call when it has already
    compiled the body of the callee. Thus, the order of the definitions
    in a file is important.
  - lithtech.exe's WON API is built from `LT2/lithshared/wonapi` against
    VC6's STL (its `basic_string` is 16 bytes, STLport's is 12). When it
    is built that way, it covers 0x004aab40..0x004ba2a0. The `WONAPI` row
    (the STLport build, as the game DLLs link it) is 2,261 of 2,263.
  - Each Primal Hunt translation unit that includes VC6's `<string>` ends
    in a 44-byte guarded initializer of `std::ctype<unsigned short>::id`
    (guard 0x004f8014, `atexit(0x00401790)`). There are ~95 of them in
    lithtech.exe. Also, the `throw()` `operator delete` of `<new>` drops
    the frames of destructors that only delete. We do not know which
    header carried `<string>` into these units. Until we know, the
    reconstructed TUs that end in such an initializer include `<string>`
    themselves behind `PRIMAL_HUNT` (a stand-in).
- Open headers from the third round. The evidence is beside the local
  views that stand in for them:
  - `msg_AllocPacket` returns its reference by value (smgr.cpp's
    `msg_AllocPacketQ512` bridges it), and packets are 0x44c;
  - `CAbstractIO` has a virtual destructor;
  - `CNetMgr::Disconnect` and `GetPacket` take a second argument;
  - `WMMoveState` is 0x6c;
  - `CModelLoadRequest` is 0x24;
  - the AvP2 layout of the poly owner;
  - `CWorldBsp` is 0x154;
  - the record of `CMessageBank` is 0x5c (Primal Hunt 0x60);
  - the destructor of `CountAdder` is inline;
  - the 37-slot order of `CommonModelLT`, and the third argument of
    `SetCurAnimTime`;
  - `CSoundInstance::m_pFilter` at +0x4c;
  - the three changed signatures of `worldsim.h`.
- Open: interlink.cpp (Primal Hunt's inter-object link module, its own
  translation unit) takes its list nodes from the shared 0xc-byte bank.
  Retail's workers use a bank of their own (lithtech.exe 0x005f3830,
  server.dll 0x10110b38). The result is byte-identical code that names the
  wrong data. It is recorded in progress/vtab-known.txt until the module
  has its bank.
- Scored: `lithtech.exe` 24.3 % byte-identical (1,764 functions, 732
  near), `server.dll` 23.2 % (1,222, 391 near), `d3d.ren` 3.9 % (139, 70
  near).
- AvP2 1.0.9.6's engine is a target again (`AvP2 lithtech.exe`, `AvP2
  server.dll`, `AvP2 d3d.ren`, `AvP2 ltmsg.dll`; the retail images are in
  `out/reference/avp2/`). It is built from the same source without
  `PRIMAL_HUNT` into `<binary>/AvP2Release/`. The engine and the server
  use cl 9044, as their Rich headers say. It starts closer to nolfsrc than
  Primal Hunt's engine does: `lithtech.exe` 28.1 %, `server.dll` 26.9 %.

  Its renderer is NOT Intel C/C++ as NOLF's was. (This plan said so at
  first; see `progress/COMPILERS.md`.) It is cl 8168, and it is the source
  of Primal Hunt's renderer, four months earlier. It has the same objects
  in the same link order, without the fog volumes. 1,180 functions are
  byte-identical between the two images (docs/AVP2_D3D_OBJECTS.md). Thus,
  it is built from Primal Hunt's `src/engine/d3d/ph/` with 8168, without
  `PRIMAL_HUNT` (`avp2-d3d`). This took it from 10 identical to 857,
  objects only.

  It now links (`avp2-d3d-link`: the AvP2Release/ph objects in Primal
  Hunt's TU order, link 6.00.8168, the release StdLith.lib after the
  DirectDraw thunks). Thus, the scoreboard places what we write by link
  order, as it does for Primal Hunt's: 857 -> 1,046 identical. Done since
  then: DrawPasses and its two UV generators, d3d_ProcessWorldModel and
  d3d_DrawNoSortWorldModel, behind `#ifdef PRIMAL_HUNT` (identical in
  AvP2). Still to do:
  - Primal Hunt's changes, each behind `#ifdef PRIMAL_HUNT`: the fog
    volumes, the StageState vector, the CD3DModelDraw layout,
    d3d_InitDevice, d3d_RenderCommand, d3d_InitTextures, the warp, the
    intersection structures, and the type of ConvertDXTGeneric;
  - AvP2's own 0x100135c0 (a polygon splitter that Primal Hunt dropped);
- AvP2 1.0.9.6's `lithtech.exe` and `server.dll` link too
  (`avp2-engine-link`, part of `engine-link`). The links use:
  - the AvP2Release objects, in the order that their placed functions
    have in retail;
  - the release's STLport WON;
  - lith, RezMgr and StdLith, built with cl 9044 (`$(LAOUT)`). The
    release's prebuilt 8966 builds are not what retail holds
    (progress/COMPILERS.md).

  The result is 2,832 -> 3,335 and 1,811 -> 2,283 identical. Every lith,
  RezMgr and StdLith function in them is identical, except the short
  ones. Retail's engine objects also hold STLport template code (the range
  of commonmodel.obj, lithtech.exe 0x00457af0..0x00459162). These are
  `set<string>` members that WON's build matches byte for byte. WON's
  build also matches WON's `SmartPtr` and `RefCount` members at 0x00413b90
  and 0x004872f0. Ours have none, and WON's block (`lib_range`) no longer
  stands in for them.
- `d3d.ren` links (`d3d-link`, as `engine-link`: /FORCE, a response file
  and a map) against the release's debug StdLith.lib. Retail links it
  after the DirectDraw thunks, as server.dll does. 154 identical.
  `docs/D3D_OBJECTS.md` maps its code into its translation units, in link
  order. About two thirds of the renderer is `/O1` (packed, unaligned
  functions), and the rest is `/O2`. There is no `/GX` anywhere. But we
  build every file `/O2`, from nolfsrc's Intel flag-group splits.

  We are rebuilding those translation units in `src/engine/d3d/ph/`, one
  file each, with its own flags (`ph/<name>.mk`). The nolfsrc originals
  are kept for AvP2's Intel build behind `#ifndef PRIMAL_HUNT`. These are
  the rounds:
  - Round one wrote 28 of them, which took d3d.ren to 730 identical.
  - Round two wrote 13 more (850).
  - Round three wrote ten more and much of the rest (1,027).
  - Round four wrote the big device and draw functions that it could
    reach (1,071).

  Two of the TUs of the map (TU21, TU33) are two objects each: two files
  share the position, in name order. There is no StdLith header
  difference. Where retail calls the small members of CMoArray or _CVector
  out of line, the cause is VC6's inliner (in TU30, the constructor of an
  element type), not the header. Retail is /Ob2 where the initialisers of a
  file show it. TU35 is four objects (fullintersectline, gdenum,
  geomroutines, intersect_line).

  The shared draw state of the renderer (0x1005b790) is
  `ViewParams g_ViewParams` in every file. `ph/ph_decls.h` holds the
  cross-file declarations that are proven against their definitions.
  Still open:
  - the large unwritten draw functions (TU01/TU03's transform and
    lighting, TU06, TU07's DrawModel, TU10's d3d_InitSceneContext, TU15's
    RenderScene and its two big helpers, TU17's d3d_InitDevice, TU27's
    shadow draws, TU31's polygrid draw, TU33/TU34/TU37);
  - about 80 references that nolfsrc files still declare under NOLF's
    placeholder names.
- Next:
  - the near functions;
  - the provisional calls, against retail;
  - the flags of each binary (Primal Hunt's server.dll is possibly not
    /O1, as NOLF's was);
  - the links (lithshared rebuilt, package 4);
  - then the tables.

### 8. launcher

Linked, both (`build/build.mk`, "The launchers' links": `compile.sh
launchers`). `src/launcher/` is the MFC launcher of Monolith's NOLF2
source release, brought back to the 2001 AvP2 launcher that it descends
from (with NOLF's display-mode code, nolfsrc's). It is compiled two
times, with the project's precompiled header, against SP5's MFC:

- as AvP2's launcher (cl 9044, `Release/`);
- as Primal Hunt's launcher (cl 8966, `PRIMAL_HUNT`, `PrimalHunt/`).

Their resources are byte-identical to the retail ones (the build reads the
art out of the retail images). Every readable function of both is
byte-identical, except a few (`progress/targets.py`). The protection
wrapper keeps five pages of the `.text` of each image encrypted
(`encrypted`). Nobody can verify the functions in these pages (the rest
of `RezFind.cpp`, `StaticEx.cpp`, `Utils.cpp`). To come: the modern
launchers (below).

### 9. boot

`make test` (`build/playtest.sh`: ext/tools/playtest.sh, adapted) runs
each game of a game directory under Xvfb and Wine. It runs each game from
its own run directory (`out/play/test/AVP2/`, `.../PrimalHunt/`). The run
directory gets the result, `wine.log` and a screenshot.

Both of our engines stop immediately. `lithtech.exe` is linked with
`/FORCE` for the scoreboard (build.mk, "Primal Hunt's lithtech.exe and
server.dll, linked from our objects: not to run"). Every unresolved symbol
is the image base. Thus, the first call to one jumps to 0x00400000 and
crashes on the bytes of the header (illegal instruction at 0x0040000C).
The first such call is in the static initialisers of the C runtime,
before WinMain. `CConsoleView::CConsoleView` (our AvP2 lithtech.exe
0x00422140, Primal Hunt's 0x0040a380) constructs the global `g_Console`.
It calls `CConsoleCritSec::CConsoleCritSec` (nolfsrc's 0x00444150), which
no object defines.

The links leave these symbols unresolved (`<dir>/<image>-link.txt`):

- `lithtech.exe`: 1,377 (AvP2's) and 1,379 (Primal Hunt's);
- `server.dll`: 316 and 311;
- `d3d.ren`: 198 and 391.

The engines run only when these symbols are written or bound (nolfsrc's
link-time stand-ins name NOLF's addresses). Both `d3d.ren` now link with
none unresolved. Primal Hunt's links from the ph/ objects alone (as
AvP2's does). The relocations of the shipped image bind every reference to
its definition. What was missing is written. The few shipped objects that
C++ cannot name twice are in `src/engine/d3d/link/phviews.txt`.

Bound since then: each of the four exe/server links takes an alias file of
its own (`src/engine/{lithtech,server}/link/aliases-{ph,avp2}.rsp`, weak
externals made by ext/tools/mkaliasobj.py). The alias files were made from
nolfsrc's `aliases.rsp` in this way:

- The process read every referencing site that the scoreboard places back
  out of the retail image.
- It kept a binding only where the target of the binding is placed at the
  address that the sites reach. Or it retargeted the binding to what is
  placed there. The head of each file tells how.

nolfsrc's bindings that no placed site can confirm are also taken, in a
section of their own ("Unproven"). The links also take ADVAPI32 (WON's
registry helpers, which /OPT:NOREF keeps). This leaves these symbols
unresolved, and nearly all of them are functions that are not yet
written:

- `lithtech.exe`: 27 (Primal Hunt's) and 21 (AvP2's);
- `server.dll`: 16 and 14.

Not checked yet: some bindings bind a view class onto the constructor of
a real class, where the caller is not byte-identical. There are about 45
such bindings for each `lithtech.exe`, and 19 for each `server.dll`. For
these, it is not known if the size and member offsets of the view are
those of the real class.

We put the retail engines (`lithtech.exe`, `server.dll`, `d3d.ren` of
each retail game) in the place of ours, in a scratch copy only. This shows
that the layout, the rez stacks and our game DLLs are sound. Both games
reach their main menus. With our `CShell.dll` and `Object.lto`, AvP2 goes
through its menus (`build/playtest-avp2-marine.txt`) and loads the first
Marine mission. It plays the cinematic of the mission and gives control to
the player.

### 12. modern

The modern build is to have these items, in addition to the fixes in
`RETAIL_BUGS.md`. (`MODERN.md` keeps the inventory as the items land.)

- widescreen support, with an option to keep 4:3 in the cinematics;
- field-of-view configuration;
- up to 16x anisotropic filtering and up to 8x antialiasing (nolfsrc's
  modern Direct3D 9 renderer is the way to get there);
- raw mouse input;
- XInput for controllers, instead of DirectInput joysticks;
- open-source replacements for the proprietary libraries: OpenMiles for
  Miles, and our Bink player on FFmpeg for Bink, as in nolfsrc. Also a
  replacement for Smacker;
- the launchers as `AVP2Launcher.exe` and `PrimalHuntLauncher.exe`.
  `AVP2.exe` and `PrimalHunt.exe` start the game directly, as if the user
  clicked Launch.
