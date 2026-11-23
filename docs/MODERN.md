# The modern build

`AVP2SRC_MODERN` is the project's define for all changes from the retail
game. These changes are:

- Bug fixes (`RETAIL_BUGS.md`).
- New features.
- Open-source middleware in place of the proprietary libraries.

Only the modern build sets the define (`make modern`, `make modern-debug`;
`build/variant.sh`). The legacy build never sets it. The legacy build stays
the retail game, as near as we can rebuild it. `make progress` scores only
the legacy build. Each change goes behind `#ifdef AVP2SRC_MODERN`. The
retail code stays in the `#else`, with no change. (`AVP2SRC_DEBUG` gives
the logs and checks of the debug builds. It is independent of
`AVP2SRC_MODERN`.)

This file is the inventory. It gives:

- The changes that the modern build makes.
- The changes that it can make next.
- The items that we examined and did not change, and the reasons.

This file uses the rules of the modern build of nolfsrc
(`NOLFSRC_MODERN`). Most of its items will come from nolfsrc, because the
AvP2 engine is of the same generation as the NOLF engine. But the items
are the items of AvP2.

## Rules for this work

* **Reimplement, never copy.** Fan fixes and patches are a good source of
  information about *what* is wrong and *why*. We do not copy their code.
  The same rule applies to the NOLF fixes of nolfsrc. The idea is
  frequently the same, because the engine is of the same generation. But
  we make and check each change here.
* **The original compilers.** The modern build uses the toolchain of the
  legacy build (the VC6 revisions of `progress/COMPILERS.md`). There is
  one exception: the open middleware (below). It is not the code of the
  games. `build/ext.sh` builds it with Zig, as nolfsrc, nolf2src and
  cjacksrc do. The reasons are:
  - OpenMiles is written in Zig.
  - FFmpeg is C99, and VC6 cannot compile it.
  - `zig cc` (clang with its own mingw-w64) builds both, and our Bink
    player, for 32-bit Windows in the build image.
* **Open middleware.** The modern build has no RAD binaries:
  - OpenMiles (`ext/openmiles`) replaces Miles.
  - Our player on FFmpeg (`src/modern/binkw32/`, `ext/ffmpeg`) replaces
    Bink.
  - Nothing replaces Smacker, because the two games do not play Smacker
    videos.

  The movies of the modern build play (no `+DisableMovies`).
* **No disc check.** The modern build removes the protection wrappers of
  the launcher. It also removes the disc checks and the CD-key checks of
  the games.

## Where it stands

**Most of the modern build is done.** `make modern` builds
`out/avp2-modern-release/`, and `make modern-debug` builds
`out/avp2-modern-debug/`. Each build has the two games, and both games are
playable. These items are done:

- The frame timing (M-10..M-14).
- The open middleware (M-X1..M-X3, with the OpenMiles fixes M-X5 and
  M-X6). Thus, a modern game directory has no RAD or Miles binary.
- The music through the Microsoft DirectMusic (M-X4).
- Raw mouse input (M-6).
- Controllers through XInput (M-7).
- The launchers, and the games that start directly (M-8).
- The full screen, window and borderless window modes (M-9).
- Alt+Tab in a window or a borderless window (M-21).
- Widescreen, 4:3 cinematics and the field of view (M-1..M-3).
- The menus and the HUD scaled to the screen (M-15, M-16).
- Internet multiplayer through a GameSpy master server, for example the
  OpenSpy server, without WON or a CD key (M-17..M-20).
- A console in single player (M-25).
- Dedicated servers on our own `server.dll`, and scripts that start them
  on Linux (M-26).
- All the retail bugs in docs/RETAIL_BUGS.md. Each is fixed
  (M-B01..M-B109), or we showed that it does not need a fix (under Done).

These items are not done yet:

- Up to 16x anisotropic filtering (M-4) and up to 8x antialiasing (M-5).
  Both need the Direct3D 9 renderer of nolfsrc.
- Alt+Tab in full screen. Full screen keeps the retail focus code (M-21).

A variant build makes its own engines (`lithtech.exe`, `server.dll`,
`d3d.ren`, for the two games). `build/all.sh` builds `engine-link`,
`d3d-link` and `avp2-d3d-link` in a `build/variant.sh` tree. Before M-10,
it did not do this. Thus, the modern build used the engines of the
release build, without the modern blocks of the engine. When we compiled
the engines, one block needed a file from nolfsrc: the M-P7-01 and M-P7-09
blocks of `clientinput.cpp` call the `##gamepad` (XInput) of nolfsrc.
Now, `src/modern/engine/p7_gamepad.h` is the `##gamepad` of AvP2 (M-7,
under Done). Thus, we keep those blocks. The modern engine does not use
the DirectInput joysticks. It reads the pad through XInput. Apart from
the pad, it makes only the keyboard and the mouse (the crash fix of
M-P7-09).

When the code of the modern build is more than a few lines, it is in
`src/modern/` (`engine/` for the engine, `cshell/` for the client game
DLLs). This code is in headers. The retail files include these headers
inside their `AVP2SRC_MODERN` blocks. Thus, the file lists of the game
projects are the retail file lists, and they stay the same.

Apart from the items that are done, the modern build compiles other
code differently. This code came in with the code of nolfsrc. It is the
`AVP2SRC_MODERN` blocks of:

- The engine (`src/engine/`).
- The launcher display code (`src/launcher/DisplayMgr.cpp`).
- The libraries (`src/lithshared/`: ButeMgr, GameSpy, LithFontMgr,
  LTGUIMgr).

We changed the name of these blocks from `NOLFSRC_MODERN`. They still
refer to the `MODERN.md` ids of nolfsrc (M-P6-02, ...). They are the NOLF
fixes, and we did not check them against AvP2 yet. Some of them use
modern files of nolfsrc: the engine block and the launcher block that
name `d3d9.ren`, the Direct3D 9 renderer of nolfsrc. This project did
not build that renderer then. Now it builds its own `d3d9.ren` (M-29),
which is not the default renderer yet. We removed these two blocks (M-8 and M-1). We will check each
block, and keep or remove it, when the items are done.

The id of each item is a series and a number (M-1, ..., and M-X for the
middleware, as in nolf2src). We never use an id again.

## Planned

We will add these items when the games run. Before each item, we check
the code of AvP2 and Primal Hunt (`docs/PLAN.md`, package 12). All the
items: not implemented yet.

| Id | What | Status |
|---|---|---|
| M-1 | Widescreen: menus, HUD and field of view at all aspect ratios | done (under Done) |
| M-2 | An option to keep the cinematics at 4:3 (pillarboxed) on a wide screen | done (under Done) |
| M-3 | A field-of-view setting | done (under Done) |
| M-4 | Up to 16x anisotropic filtering, through the modern Direct3D 9 renderer of nolfsrc | done (under Done, M-29 stage 4) |
| M-5 | Up to 8x antialiasing (the same renderer) | done (under Done, M-29 stage 4) |
| M-6 | Raw mouse input (WM_INPUT), in place of the DirectInput mouse | done (under Done) |
| M-7 | Controllers through XInput, in place of DirectInput joysticks | done (under Done) |
| M-8 | The launchers are `AVP2Launcher.exe` and `PrimalHuntLauncher.exe`. `AVP2.exe` and `PrimalHunt.exe` start the game directly, as if you clicked Launch in the launcher (no registry, no disc check) | done (under Done) |
| M-9 | Window modes: full screen, a window at all sizes, or a borderless window at the resolution of the desktop. You select the mode in Options. (M-13 is the frame limiter of this item. M-17..M-20 give Internet play without the dead GameSpy and WON servers) | done (under Done) |
| M-21 | Alt+Tab in a window or a borderless window: the game comes back as it was. (The retail focus code stopped the renderer and held all the sounds. Under Wine, the renderer and the sounds did not come back, and the picture stopped) | done (under Done) |
| M-22 | The launchers stay open while the game runs, so that the Steam overlay and Steam Remote Play work (GitHub issue #5) | done (under Done) |
| M-23 | No lock of the back buffer before each flip (`LockOnFlip` 0 by default), for a higher frame rate under Wine and Proton (GitHub issue #2) | done (under Done) |
| M-24 | `VSync`: in full screen, 1 (the default) is the retail flip, which waits for the vertical blank, so the Steam overlay works; 0 copies the frame with no wait, so the frame rate is not locked to the refresh rate, but under Proton the Steam overlay does not show | done (under Done) |
| M-27 | The engine is DPI-aware on Windows, so the game fills the screen and the clicks hit the buttons with display scaling above 100% (GitHub issue #6). Not yet confirmed on a scaled Windows display | done (under Done) |
| M-25 | A console in single player, as in nolfsrc (its M-P2-01..M-P2-04, -05, -08, -09, -14, -15): the ~ key or Shift+Esc opens it over the top half of the screen, and the game pauses. It runs console commands and cheat codes. A new ToggleConsole control in Customize Controls. Retail AvP2 has no console that a player can open | done (under Done) |
| M-26 | Dedicated servers on Linux, as in nolfsrc: `linux-server-avp2.sh` and `linux-server-ph.sh` start `AVP2Serv.exe` or `AVP2XServ.exe` under Wine, with a display or headless (`xvfb-run`). The servers run on our own `server.dll` | done (under Done) |
| M-28 | The console parser does not end a command at a character above 127, so a bind command with a translated device name works. Not yet confirmed on Windows | done (under Done) |
| M-29 | A Direct3D 9 renderer, `d3d9.ren`, beside `d3d.ren`: the device at all resolutions (GitHub issue #6), the Steam overlay at all frame rates under Proton (GitHub issue #2), and the renderer of M-4 and M-5 | under way (under Done) |

## Open middleware

The modern build replaces the proprietary libraries with open-source
libraries. The legacy build keeps the retail DLLs, from the discs of the
player. The replacements work as follows:

- `build/ext.sh` builds the replacements from the `ext/` submodules into
  `out/ext/`. It does this one time for the two modern variants. It
  builds each part again only when its inputs change.
- `build/all.sh` step 2 installs the replacements in the directory of
  each game, in place of the retail files (`MANIFEST.txt` origin `EXT`;
  `retailextract.py openruntime`, the list in `build/retail.py` and
  `build/retail_avp2.py`, `OPEN_RUNTIME`). It removes retail middleware
  that an earlier build installed.
- The install scripts of the public archives copy only the files that
  `MANIFEST.txt` shows as files from the discs (`build/package.sh`,
  `build/mkinstall.py`). Thus, the install script of a modern archive
  copies no RAD or Miles file from the discs. It copies 40 files, from 36
  files on the discs. All of them are game data.
- CI gets `ext/openmiles` and `ext/ffmpeg` only for the modern builds
  (`.github/workflows/ci.yml`).

Pins: `ext/ffmpeg` is at FFmpeg `n8.1.3` (1041abd, shallow).
`ext/openmiles` is at `miles6-fixes` (fd41544) of our fork. This branch is
the OpenMiles branch of nolfsrc, so that the family builds one OpenMiles
and one FFmpeg. AvP2 needs two more fixes on it than NOLF (M-X7).
AvP2 1.0.9.6 and Primal Hunt have the same `mss32.dll` (Miles 6.0i) and
`binkw32.dll` as the NOLF GOTY edition.

We checked the replacements with `build/playtest.sh` on
`out/avp2-modern-release/`, under Xvfb and Wine in the build container.
We used these options of the script:

- `PLAYTEST_MOVIES=1`: the movies played.
- `PLAYTEST_AUDIO`: the script recorded each sound stream that the game
  opened, through the file plugin of ALSA. It made one raw file for each
  format.

| Id | Retail | Replacement | Files | Checked | Status |
|---|---|---|---|---|---|
| M-X1 | RAD Bink 1.0h, `binkw32.dll` (the intro movies) | Our Bink player on the Bink decoders of FFmpeg. It has the thirteen `_BinkXxx@n` entry points that `lithtech.exe` finds by name (`binkvideo.cpp`, `BinkVideoMgr::Init`; the engine imports nothing from the DLL). It has the four `BINK` fields and the four surface formats that the engine reads. It is the player of nolfsrc, with no change. It is the code of this project family. The AvP2 engine asks the same things of it as the NOLF engine (the same names, fields, surface formats, `BINKSNDTRACK` texture videos and end-of-movie test). We copied it into this tree and did not share it, so that each copy can change. The sound goes through `waveOut`, not Miles | `src/modern/binkw32/` (`binkw32.c`, `binkw32.def`), `build/ext.sh`; `build/avp2cmds.sh` (`open`: no `+DisableMovies`), `build/all.sh`, `build/package.sh` | AvP2: `build/playtest-movies.txt`. Fox Interactive, Sierra, LithTech and Monolith (`Movies/*.bik`) play fully, one after the other, with their sound. Then the main menu shows. Primal Hunt plays no Bink movie. Its client shell shows four `Interface\intro_*.pcx` splash screens (`NextSplashScreen`), and it installs no `Movies/`. Its engine loads our DLL at startup, as the AvP2 engine does. The same script runs to its menu | done |
| M-X2 | RAD Smacker, `Smackw32.dll` | None: removed. The two games play no Smacker video. The two engines do not import `Smackw32.dll`. Only the `Init` of the Smacker video manager loads it (`videomgr.cpp`). The two engines make only the Bink video manager. The game calls the factory (`0x0048a770` in the Primal Hunt engine) one time, with `"BINK"` (`clientmgr3.cpp`). The retail images push the `"SMACKER"` string only inside the factory. There is no `.smk` file in the archives or directories of the two games. `CWinUtil::GetMoviesPath` only looks for the file `intro.smk`. When there is no DLL, there is nothing to load. Thus, there is no import to satisfy and no engine change | `build/all.sh` (it does not install the DLL, and it removes the DLL of an older modern build) | The two games run their movies or splash screens, their menus and a level with no `Smackw32.dll` in the directory | done |
| M-X3 | Miles 6.0i, `mss32.dll`, with its MP3 decoder `MP3DEC.ASI`/`.AS6` | The `mss32.dll` of OpenMiles, with the Miles 6.0 export table. It decodes MP3 itself (`AIL_decompress_ASI`). The sound loader of the engine calls this function for each MP3 `WAVE`: most of the music, the dialogue and many effects. Thus, we install no `.asi`. The two engines import 89 functions from it by name (`_AIL_...@n`). OpenMiles exports all of them. The game never names a 3D provider (`InitSoundInfo::m_sz3DProvider` stays empty). Thus, the engine makes the positional sound itself over the 2D samples of Miles, as in retail (retail has no `.m3d`). The filters (`AIL_enumerate_filters`) are the built-in filter of OpenMiles | `ext/openmiles` (submodule), `build/ext.sh` (the build applies `ext/patches/openmiles/*.patch` to a copy; none is necessary) | AvP2 `build/playtest-avp2-marine.txt` and Primal Hunt `build/playtest-ph-corporate.txt`. The scripts recorded the sounds of the menus, the dialogue of the first mission (MP3), the effects and the ambience. No crash. Quicksave and quickload work. The music comes from DirectMusic (`musictype`, `ILTDirectMusicMgr`), not from Miles. It is not part of this item | done |
| M-X4 | The Microsoft DirectMusic. The retail games expect it in Windows (the in-game music: `musictype ima`, `CLTDirectMusicMgr`). Under Wine, it is the DirectMusic of Wine. That DirectMusic loads the music but plays silence. The DirectMusic of Proton is the same | The Microsoft DirectMusic (DirectX 9.0c), in each game directory. It has these parts. The nine DLLs from `ext/toolchains/directmusic-5.3.1.904/` (with checksums), as `ms_<name>.dll`. And `lithtech.exe.manifest`, a registration-free COM manifest. The manifest maps the 46 classes of the DLLs to those files. Thus, the `CoCreateInstance` calls of the engine, and of the DLLs to each other, find them with any prefix as it is, and nothing is registered. On Windows, the system DirectMusic does not change. The `ms_` names prevent Wine from using its builtins in their place. This is M-P4-12 of nolfsrc, checked against the AvP2 and Primal Hunt engines. These engines ask DirectMusic for the same things as the NOLF engine: the performance and a default synth port with `SetDirectSound(NULL)`. Through the loader of the engine, they also ask for segments, styles, bands and DLS collections from the `.rez` archives. They never ask for the GM set, so there is no `gm.dls`. The manifest is the manifest of nolfsrc, without two classes that no DLL here implements. No engine change | `ext/tools/retailextract.py` (`openruntime`, `DIRECTMUSIC_MANIFESTS`; tools 314d71c), `build/retail.py`, `build/retail_avp2.py`, `src/modern/directmusic/` (`README.md`, `lithtech.exe.manifest`); `docs/LICENSE_NOTES.md` | Under Xvfb and Wine 9.0 in the build container, with a new prefix for each run. We recorded the DirectSound stream of DirectMusic (`PLAYTEST_AUDIO`, the 48 kHz stream). `+LTDMConsoleOutput 5 +ErrorLog 1` lists the segments that load and the intensities that play. Primal Hunt, `build/playtest-ph-corporate.txt`, retail data: `EXP1Theme` loads and goes to intensity 2 (`Ambient*.sgt`) in exh1s2. The stream has the music (peak 0.14, RMS 0.005-0.014, by phrase). AvP2, `build/playtest-avp2-marine.txt`: the main menu has no DirectMusic music (the `CMusicMgr` of the server starts it for each level). The opening, `m_open`, asks only for the intensities 1 and 10 (`Silence.sgt`; its music is wave tracks through Miles). Thus, the stream is silent there, by design. We sent `M1Theme` to intensity 2 (`Ambient1.sgt`, with a test-only `Attributes/music.txt` in `custom/`). It plays with our DirectMusic (peak 0.14). It is silent with the DirectMusic of Wine (peak 0.00005, the same run without the manifest). No crash. The sound effects, the dialogue and the wave tracks of Miles do not change | done |
| M-X5 | The MP3 decoder of Miles 6.0i gives the format of the source | OpenMiles fix (fork, `miles6-fixes` 40aa1d0). The sound loader of the engine calls `AIL_decompress_ASI` for each MP3 `WAVE` (`CSoundDataQ47C::InitFromCompressed`, `sndinst2.cpp`). Then it parses again the RIFF image that it gets back. `AIL_decompress_ASI` and the built-in ASI stream provider of OpenMiles asked miniaudio for 16-bit stereo at 44100 Hz. Thus, each MP3 came back upmixed and resampled to that format, with a header for that format. The AvP2 MP3s are: 1866 mono 44100 Hz (`SOUNDS.REZ`: dialogue and effects), 99 stereo 44100 Hz (`AVP2.REZ`) and two stereo 22050 Hz. The Primal Hunt MP3s are: 197 mono and 6 stereo 44100 Hz (`DIALOGUE.REZ`, `AVP2X.REZ`). The mono MP3s used two times their memory as stereo. The 22050 Hz MP3s used four times their memory. Now, the decoder uses the channels and the rate of the source, as the Miles decoder does. This is a general change of OpenMiles, not a game change (NOLF has the same problem, and 38 mono 22050 Hz MP3s) | `ext/openmiles` (submodule pin) | A Windows harness (`zig cc`, Wine) calls the built `mss32.dll` on MP3 data from `SOUNDS.REZ`. Mono 44100 Hz: before, 2 ch / 44100 Hz; now, 1 ch / 44100 Hz (62208 frames, as its `fact` chunk). Stereo 22050 Hz: before, 44100 Hz (1438848 frames); now, 22050 Hz (719424, as its `fact`). The OpenMiles tests pass. There are two new tests: a mono 22050 Hz WAV and MP3 come back in that format, and the attributes of the ASI stream. In the two games (`PLAYTEST_AUDIO`, the scripts of M-X3), the dialogue and effects play. The level of the mix is the same as before (AvP2 RMS -22.0 / -21.9 dBFS, Primal Hunt -25.9 / -25.8). No crash. Quicksave and quickload work | done |
| M-X6 | Miles 6.0i saturates its mix into the output: a hard clip at full scale | OpenMiles fix (fork, `miles6-fixes` 725c6ff). The miniaudio engine mixes in f32. It gave each block to the device with no change. Thus, when sounds added up to more than full scale, they went to the float device of Wine without a clip (the first Marine mission of AvP2: peak 1.21, 567 samples over 1.0). Miles 6.0i adds into 32-bit integers and saturates them (the mixer copy in its `mss32.dll`, from `0x2111f017`: `packssdw`, or compares against `0x7FFF`/`-0x8000`). It has no soft knee. Now, OpenMiles clips each mixed block to [-1, 1] in the process hook of the engine. It changes a NaN to silence | `ext/openmiles` (submodule pin) | AvP2 `build/playtest-avp2-marine.txt`, recorded before and after: peak 1.2115 (567 samples over 1.0) -> 1.0000 (none over). The RMS does not change. Primal Hunt `build/playtest-ph-corporate.txt`: the peaks are 0.69, with no change. The OpenMiles test: four loud sounds at the same time stay flat at 1.0 (3.66 without the clip) | done |
| M-X7 | Two Miles calls that the AvP2 engine makes and the NOLF engine does not | OpenMiles fixes (fork, `miles6-fixes` cc5d90c, fd41544). First: `AIL_3D_provider_attribute` now writes "Maximum supported samples" (64) and "EAX environment selection" (-1, no EAX). Before, it left both values as they were, and the engine reads both into uninitialised variables (M-B109). With a 3D provider open, the number of 3D voices was random. Second: `AIL_open_stream` now accepts a file handle name with four backslashes. The AvP2 and Primal Hunt engines write four (their format string is `5c 5c 5c 5c 25 64`), the NOLF engine writes two, and Miles 6 accepts both. Before, OpenMiles accepted only two, so the streamed sounds of the retail engines did not open. GitHub issue #1 | Unit tests in OpenMiles. The Miles trace (Wine +snoop) of the first Marine mission, with both builds against the retail binaries (GitHub issue #1) | done |

## Done

M-X1, M-X2, M-X3 (the open middleware, above); M-X4 (the music, above); M-X5, M-X6 (OpenMiles fixes).
M-1..M-3, M-6, M-7, M-8, M-9, M-15, M-16, M-17..M-20, M-25, M-26 (below); the retail-bug fixes M-B01..M-B09 (the engine), M-B20..M-B26 (the renderers), M-B40..M-B63 and M-B108 (the client shells and lithshared), M-B70..M-B87 (the object DLLs and shared code), M-B100..M-B107 (the launchers and Launch.dll), below.

The frame timing. The user reported this on the first modern build: "the
high framerate means all the timing is wrong". nolfsrc found the same
problem in NOLF (its M-P1-01, -02, -05, -23, -25). Before we used each
finding, we checked it against the code of AvP2 and Primal Hunt, and we
measured it in their games. The code is our code.

| Id | What | Files | Status |
|---|---|---|---|
| M-10 | The world stays at real time at all frame rates (RETAIL_BUGS.md: "Above 100 frames a second the world runs fast") | `src/engine/server/smgr.cpp` (`CServerMgr::Update`) | done |
| M-11 | Animations lose no time to whole milliseconds | `src/modern/engine/p1_frametime.h`; `src/engine/lithtech/clientmgr2.cpp`, `clientupdate.cpp`; `src/engine/server/servermodel.cpp` | done |
| M-12 | The clocks come from the performance counter | `src/engine/lithtech/ltnew2.cpp`; `src/AVP2/Shared/WinUtil.cpp`; `src/AVP2/ClientShellDLL/InterfaceMgr.cpp` | done |
| M-13 | The frame-rate limit: 60, 120, 144, 240 (the default) or none, in Options, Graphics, for single player and multiplayer | `src/modern/cshell/m10_timing.h`, `m10_framelimit.h`; `GameClientShell.cpp` (`PostUpdate`), `FolderDisplay.cpp`; `src/AVP2/ClientRes/ClientRes.rc`, `ClientRes.h` (31000-31003) | done |
| M-14 | The game applied some amounts one time each frame. Now, these amounts are for each 60th of a second | `src/AVP2/Shared/CharacterMovement.cpp`; `src/AVP2/ClientShellDLL/client_physics.cpp`, `FireFX.cpp` | done |

**M-10.** `CServerMgr::Update` runs a game tick each time that the game
calls it. The game calls it on each client frame in single player and on
a listen server. A tick takes a minimum of 10 ms of game time
(`m_FrameTime` is clamped to 0.01..0.2). Thus, above 100 frames a second,
the world ran at (frame rate / 100) times real time. We measured this
with the retail AvP2 1.0.9.6 binaries (`ShowGameTime`, against the wall
clock):

- 1.44 times real time at 139 fps.
- 2.37 at 237 fps.
- 2.97 at 297 fps in the first modern build.

Now, a tick runs only when 10 ms have passed since the last tick. When
the time goes backwards (a new world sets the clock again), a tick runs
immediately. The update still services the packets and the network on
each call. A paused server and a variable-step server work as before. The
loops of the dedicated servers already keep them to their
`MIN_SERVER_UPDATE` (the `ThreadUpdate` of AVP2Serv). They also get the
same gate. We measured the result (5 s windows of game time against real
time):

- AvP2: 1.000 at 60 fps, 0.999-1.002 at 120, 0.998-1.001 at 230 (limit
  240) and 0.998-1.002 at 230-287 (no limit).
- Primal Hunt: 1.000 at 60, and 0.998-1.002 at 110-210 (limit 240).

**M-11.** The engine moves each animation tracker forward by the time of
the frame in whole milliseconds, `(int)(frame time * 1000)`. It discards
the remainder. This applies to:

- The models and the sprites on the client (`cs_UpdateAnimations`, and
  in `clientupdate.cpp` the sprites, the sprites of the particle systems,
  the sprite objects and the sprite skins of the models).
- The models on the server (the milliseconds of a tick, one time each
  tick).

The loss is up to a millisecond each frame, and about half a millisecond
on average. This is small at 60 fps. At 240 fps (4.2 ms frames), it is up
to an eighth of the time. At 300 fps, it is a sixth. Thus, the animations
ran slow. The AvP2 weapons also ran slow, because their animations
control the rate of fire (`CWeaponModel::UpdateFiring`). Now, the engine
keeps the remainder for the next frame (`P1_FrameMs`, `P1_TickMs`).

**M-12.** The clock of the engine (`time_GetTime`, the frame times)
counted the whole milliseconds of `timeGetTime`. Thus, at a few
milliseconds a frame, the frame times had a jitter of a millisecond (a
quarter of a frame at 240 fps). Now, the clock is the performance
counter. The clock of the game (`CWinUtil::GetTime`: the model animations
of the menus and of the loading screen) used the 10-16 ms steps of
`GetTickCount`. At a high frame rate, most frames lasted no time, and
some frames lasted a full step. Now, this clock is also the performance
counter. The model animations of the menus keep their fraction of a
millisecond, as M-11 does (`CInterfaceMgr::UpdateModelAnimations`).

**M-13.** `FramerateLimit` (below) works as follows:

- After the game draws each client frame (`CGameClientShell::PostUpdate`),
  it waits until 1/FramerateLimit of a second has passed since it
  released the previous frame.
- While more than 2 ms remain, it sleeps in 1 ms steps
  (`timeBeginPeriod(1)`, from winmm.dll, which the engine has loaded).
  Then it spins.
- The frames keep the cadence. After a frame that is longer than a full
  period, the count starts again from that frame.
- The limit applies in multiplayer and in single player. (M-10 and M-11
  caused the multiplayer problems at a high frame rate. They are fixed
  for the two modes.)
- The limit does not apply while a world loads (the game state is
  `GS_LOADINGLEVEL`).

In the menus, the setting is in Options, Graphics (`CFolderDisplay`),
"Frame rate limit". It is between Resolution and Performance, in the two
games (one source, `FolderDisplay.cpp`; the layouts of their folders are
their own, but they are the same here). The choices are 60, 120, 144, 240
or None. The page of the folder has two lines above its note about
performance. We made the page one line taller and moved the note one line
down. If not, Performance went to a second page. A limit from the console
or autoexec.cfg that is not one of the choices shows as the next higher
choice. The game keeps that limit until you select a different one.

Measured: 60.0, 119.4-120.0 and 223.7-234.7 frames a second at the limits
60, 120 and 240 in AvP2. (This was in the container, where 240 is near the
maximum of the machine. With no limit: 230-287.) In Primal Hunt: 59.8-60.7
at 60. We checked the option in the menus of the two games. It shows the
limit that is set. A click changes it to the next choice. When you leave
the folder, the game writes it to autoexec.cfg.

**M-14.** The game code applied some amounts one time each frame, at all
rates. Now, these amounts are the amount for a 60th of a second, because
the developers tuned the game at that rate. (At 60 frames a second, each
amount is the same as before.) The amounts are:
- `CharacterMovement::Update`: each frame, it sets a velocity of less
  than 3 units a second to zero. Movement starts from rest with the
  acceleration of one frame, and a short frame makes this small. Thus,
  the threshold is now the share of a 60 fps frame. Also, it discarded a
  move of less than 0.1 units in a frame (on the client): 6 units a
  second at 60, 24 at 240.
- The "don't skid" damping of the crouching and ladder states. It
  removed half (three quarters) of the velocity each frame. At 240, it
  was four times as strong.
- `client_physics.cpp` (debris, shells): the speed at which a bounce
  stops increases with the square root of the frame rate (116 units a
  second at 60, 232 at 240).
- `FireFX`: one chance in ten each frame of sparks.

The other parts of the movement already use the time (the physics of the
engine integrates friction analytically). We measured this. We used a
quicksave of the first Marine and Corporate missions. We walked for 3 s,
did a jump, and held down the starting melee weapon for 4 s. (The lines
of the probe were temporary.)

| Game, limit | frames a second in play | walking | jump: height, time in the air | swings apart |
|---|---|---|---|---|
| AvP2, 60 | 35-49 | 300.8 u/s | 78.9, 1.00 s | 346 ms |
| AvP2, 120 | 47-82 | 300.1 | 77.7, 1.03 | 333 |
| AvP2, 240 | 44-65 | 300.3 | 77.6, 1.01 | 346 |
| AvP2, none | 48-71 | 300.3 | 77.3, 1.00 | 336 |
| Primal Hunt, 60 | 60 | 300.7 | 77.9, 1.00 | 329 |
| Primal Hunt, 240 | 148-183 | 300.1 | 76.6, 1.05 | 337 |
| Primal Hunt, none | 67-89 | 300.2 | 77.3, 1.03 | 335 |

(Other builds also used the container, so it drew no more frames than
these in play. The higher rates occurred in the intros of the levels,
where we measured the speed of the world.)

We checked the result against the retail game. The legacy build does not
change: each change is in an `AVP2SRC_MODERN` block or in `src/modern/`.
`ext/tools/releasesrc.py strip` gives each changed file as it was. Also,
`make progress` keeps all the recorded functions.

Not done: the review of each frame looked at the movement, the physics
and the effects that the NOLF findings of nolfsrc showed. We did not look
at all the other client effects that step one time each frame without
the frame time. We did not measure them above about 80 frames a second in
play.

### M-1..M-3: widescreen, 4:3 cinematics, the field of view

The NOLF items of nolfsrc were the guide:

- Its M-P0-01: the Hor+ field of view.
- Its M-P1-07: the resolution list.
- Its M-P3-08: 4:3 cinematics.
- Its M-P3-21: the zoom next to the 4:3 area.

We checked each item against the code of AvP2 and Primal Hunt. That code
is different from the NOLF code in the important places. The code is our
code.

| Id | What | Files | Status |
|---|---|---|---|
| M-1 | Widescreen. The resolution list has screens of all shapes. The view is wider for the screen (Hor+: the vertical field of view stays the same, and the horizontal field of view increases by the aspect ratio of the screen over 4:3). The player-view weapons and the camera overlays also change. The overlay of a scope stays 4:3, with black next to it. The game uses the mode of the modern engine (the engine asked for the `d3d9.ren` of nolfsrc, so all modern games ran at 640x480x16) | `src/modern/engine/m1_horplus.h`, `src/modern/cshell/m1_widescreen.h`; `src/engine/lithtech/clientmgr2.cpp` (`cm_RenderCamera`, `r_GetRequestedMode`), `clientinterface.cpp` (`Get3DCameraPt`); `src/engine/d3d/ph/drawmodel.cpp`; `FolderDisplay.cpp`, `OverlayMGR.cpp`/`.h`, `ViewModeMGR.cpp`/`.h`, `InterfaceMgr.cpp`, `AutoTargetMgr.cpp` | done |
| M-2 | `RestrictCinematicsTo4x3`: the cinematics are in the centred 4:3 area of a wide screen, with black next to it. Options, Graphics, "Cinematics" (Widescreen, 4:3) | `m1_widescreen.h`; `InterfaceMgr.cpp` (`Draw`), `FolderDisplay.cpp`; `ClientRes.rc`, `ClientRes.h` (31200-31206) | done |
| M-3 | `FieldOfView`: the horizontal field of view of the player at 4:3, 75 to 120 degrees (the default is 90, the retail value). Options, Graphics, "Field of view" | `m1_widescreen.h`; `src/AVP2/Shared/PlayerMovement.cpp`, `CameraMgrFX.cpp` (`CameraMgrFX_ZoomUpdate`), `FolderDisplay.cpp`; `ClientRes.rc`, `ClientRes.h` | done |

**M-1.** Retail gives each camera a field of view for a 4:3 screen:

- The field of view of the player is the `FOV` of the character in
  CharacterButes.txt. It is 90 by 78 degrees for a human or a Predator,
  95 by 85 for an Alien, and 75 by 65 for an Exosuit. It is the same in
  the two games.
- The field of view of a cinematic camera is the field of view of its
  object, 75 by 60 by default.

Retail draws this field of view across the full screen. Thus, a 16:9
screen showed the 4:3 picture stretched horizontally. Also, Options,
Graphics listed only modes of exactly 4:3
(`CFolderDisplay::GetRendererData`).

The modern build lists all the modes of 640x480 or more. The engine
increases the horizontal field of view by the aspect ratio of the screen
over 4:3, `fovX' = 2 atan(tan(fovX/2) * aspect/(4/3))`. It keeps the
vertical field of view. It does this in two places:

- Where it gives a camera to the renderer (`cm_RenderCamera`).
- Where it changes a screen point into a direction (`Get3DCameraPt`).

Thus, the picture and the measurements of the game against it agree. The
cameras, and the values that the client reads back from them, keep the
4:3 values of the game. M-P0-01 of nolfsrc does the calculation in the
NOLF renderer, from the scene rectangle. Here, the factor comes from the
screen. The reason is that AvP2 letterboxes a cinematic: it makes the
rectangle of the camera smaller, with the same field of view
(`CameraMgrFX_WidescreenUpdate`; the vision change of the Predator also
makes it narrower). On a 4:3 screen, each view is exactly as retail draws
it.

The player-view weapons have their own field of view. Each frame, the
client sets `ExtraFOVXOffset` to `ViewModelBaseFOVX` (95) minus the field
of view of the camera (`CWeaponModel`). The renderer adds the field of
view of the scene (`DrawModel`, `m_bFovScale`). Because the field of view
of the scene is wider, this sum is incorrect. Thus, the renderer
calculates the field of view of the model at 4:3, as retail does (it
makes the field of view of the scene narrower again by the factor of the
screen). Then it makes it wider in the same way. The weapon keeps its 4:3
shape and size on all screens. It still ignores the field of view of the
player (M-3).

The camera overlays (`OverlayMGR`: the frames of the vision modes, the
net, the fire, the Evac and orientation overlays) are sprites. Their size
comes from the view through `Get3DCameraPt` (`GetCameraScale`). Thus, they
now cover a wide screen. They are stretched with it, as retail stretches
them to 4:3. The zoom overlay of the Predator, `predzoom.spr`, is one of
them. It shows only while the zoom changes (`CPlayerStats`). The scope of
the rail gun (`railgunoverlay.spr`, which shows while the rail gun is
zoomed) keeps its 4:3 shape in the middle. While the scope shows and the
view is first person, the game fills the screen next to the 4:3 area
with black (`InterfaceMgr::Draw`). M-P3-21 of nolfsrc found the same gap
in the sunglasses zoom of NOLF.

The client also uses `Get3DCameraPt` to put items from the world on the
screen (`GetScreenPos`: the target lock of the Predator, `PredTargetFx`,
and the auto-targeting). Thus, these items are in the correct position on
a wide screen. The auto-targeting accepts a target only inside a box of
30% to 70% of the width and height of the screen (`CAutoTargetMgr`).
Across a wide screen, that is a wider angle than in retail. Thus, the box
is now that share of the width of the centred 4:3 area
(`M1_InTargetBox`). This gives the same angle.

The menus are the retail 640x480 page. The page is centred and is not
scaled at any resolution. It works on all shapes of screen with no
change. The HUD is anchored to the corners and the edges of the screen
(the `m_ptOffset` of `CHudElement` from each corner, or from the centre
of the screen for the centred elements). Thus, it is also in the correct
position on all shapes, at its retail size.

Options, Graphics shows the mode that is in use. Before, the folder showed
the last mode that it set itself (`GameScreenWidth`/`Height`/`BitDepth`).
A mode from a command line (as from AVP2.exe) is not that mode. Thus, the
folder showed a different entry, and when you left the folder, the game
changed to that entry.

We also found a problem in the modern engine. Its `r_GetRequestedMode`
(the block of nolfsrc) asked for `d3d9.ren`, the Direct3D 9 renderer of
nolfsrc, in place of `d3d.ren`. AvP2 has no `d3d9.ren`. Thus, the mode of
the engine failed. The first fallback of the client shell (640x480x16, the
same DLL) also failed. All modern games ran in their last fallback,
640x480x16 through `d3d.ren`, for all requested modes
(`OnEngineInitialized`; the debug output says "Couldn't set render mode",
then "Setting render mode to D3D Emulation mode"). We removed the block.

We checked the result under Xvfb and Wine (`build/playtest.sh`,
`PLAYTEST_RES`):

- AvP2, the first Marine mission, at its start, at 1024x768 and
  1280x720. We scaled the 4:3 shot to 720 lines and compared it with
  centre crops of the 16:9 shot, at steps of 2% of the width. The best
  match is the centre 960 columns (RMSE 0.014; the crops on each side
  0.026). Thus, the 16:9 view shows the 4:3 view in its middle at the
  same scale. The vertical field of view does not change. The horizontal
  field of view is 106.3 degrees (Hor+: 2 atan(tan 45 x 4/3) = 106.26).
  The weapon has the same size in the two shots.
- AvP2 at 2560x1080 (21:9): the best match is the centre 1440 columns
  (RMSE 0.012; each side 0.025). The field of view is 121.3 degrees
  (Hor+: 121.28).
- Primal Hunt, the first Corporate mission ("Zeta Site"), 1024x768
  against 1280x720: the centre 960 (RMSE 0.029; each side 0.070), 106.3
  degrees.
- At 1280x720 in the two games, we looked at: the main menu and Options,
  Graphics (the page is centred), and the HUDs of the Marine, Predator,
  Alien, Corporate and Predalien in their corners. We also looked at the
  zoom and three vision modes of the Predator, and the vision of the
  Alien and the Predalien. Their overlays cover the full screen.

We did not get to the scope of the rail gun in play (it is the last
weapon of the Marine). Thus, we checked its 4:3 shape and bars only from
the code.

**M-2.** `RestrictCinematicsTo4x3` 1 (0 by default): while the cinematic
camera is active, the game fills the screen next to the centred 4:3 area
with black, after it draws the scene (`CInterfaceMgr::Draw`, after the
retail borders). Thus, a wide screen shows the same picture as a 4:3
screen. The game draws the subtitles over it, inside the area. M-P3-08 of
nolfsrc has the same name and meaning. We checked it in the opening
cinematic of AvP2 at 1280x720. With the option, there are 160 black
columns on each side (mean 0 in the left 150 columns). Without the
option, the picture has the full width. When the player has control, the
game fills nothing next to the area (mean 0.056). The Predalien opening
of Primal Hunt at 1280x720 is the same (mean 0 in the left 150 columns).

**M-3.** `FieldOfView` (degrees, 60..130; the menu gives 75 to 120 in
steps of five) is the horizontal field of view at 4:3 of a human or a
Predator. The field of view of each character changes with it, in the two
directions equally, so each character keeps its shape:
`tan(fov'/2) = tan(fov/2) x tan(FieldOfView/2)`. For example, the 95 by
85 of an Alien at 90 is 114.4 by 105.1 at 110. The client sets it where
it gives the field of view of the character to the cameras of the player
(`PlayerMovement::HandleMessage_Physics_Update`, `SetMPButes`). It also
sets it immediately when the option changes. A wide screen makes it
wider again (M-1).

The zoom (of the Predator and of the rail gun) magnifies over the field
of view of the camera (`CameraMgrFX_ZoomUpdate`). Its magnitude is now one
over the retail magnitude, times the factor of the setting. This factor
applies over the first doubling. Thus, a zoomed view is the retail view
at all settings. The cinematics keep their own field of view, and the
weapons keep theirs (M-1). We checked it at 1024x768 in the same place as
M-1. The shot at 90 matches the centre 716x537 of the shot at 110 (RMSE
0.019; each side 0.032). Thus, the horizontal field of view of the shot
at 110 is 110.1 degrees.

The menus and the HUD kept their retail size in pixels. Thus, they were
small on a large screen (the 640x480 page was a third of the height of a
1920x1080 screen). Now, the game scales them: M-15 and M-16 (below). The
window and borderless window modes are in M-9.

### M-15, M-16: the menus and the HUD scaled to the screen

We read M-P3-01 of nolfsrc (the menus in the 4:3 area, scaled) and
M-P3-12 (HUD scaling) to learn what to do. The method is our own for
AvP2, because the AvP2 interface has a different structure.

The NOLF HUD is a small number of draws, each placed by hand in
`PlayerStats.cpp`. nolfsrc scaled them one by one, and it kept all text at
the size of its bitmap font. The AvP2 HUD is data. `CHudMgr` makes the HUD
of each species from the layout. The layout has `CHudElement`s with an
offset from a corner, an edge or the centre, and about forty draw and
update functions. These include:

- The health, armour and energy bars.
- The EKG and ammo counters of the Marine.
- The energy and skull counter of the Predator.
- The health of the Alien, and the HUD of the Exosuit.
- The pickup list, the air and battery meters, and the boss meter.
- The Predalien and Corporate HUDs of Primal Hunt, and the turret ammo.

There are also the motion tracker (`CMotionDetector`), the crosshairs
(`CCrosshairMgr`), the messages (`CMessageMgr`), the subtitles, the
teletype, the objectives and the weapon choosers. The text is in bitmap
fonts, and the game draws it one glyph at a time (`lithbitmapfont.cpp`).
To scale each draw, we must change hundreds of places.

Thus, the game draws each item on a smaller virtual screen and scales it
onto the real screen (a "space", `src/modern/cshell/m15_uiscale.h`). It
works as follows:

- The client shell puts wrappers into the `ILTClient` table of the engine
  one time (`M15_Install`, `OnEngineInitialized`). At `OnEngineTerm`, it
  puts back the functions of the engine.
- While a space is active, `GetSurfaceDims` of the screen surface gives
  the size of the space.
- The wrappers scale the destination of each blit onto the screen
  surface. The blits are: `Draw*` (changed to a `Scale*`), `Scale*`,
  `Warp*`, `Transform*` (the wrapper scales the origin, keeps the
  destination at its offset from the origin, and multiplies the scales)
  and `FillRect`.
- The layout code of the game then runs with no change. An element that
  is anchored to the bottom-right corner goes into the corner, larger,
  and its text also.
- Outside a space, for other destinations, or where the scale of a space
  is 1, each call goes through with no change. This is always true at
  640x480. Thus, a 640x480 game is the retail game, draw for draw.
- `CInterfaceResMgr` gives the offsets, ratios and screen size of the
  640x480 page in the space that the game draws. If no space is active,
  it gives its retail values.
- The space is for each thread (the loading screen draws from its own
  thread).

The spaces are:

* The menu space (M-15). The page fills the height of the screen and is
  centred. `MenuScale` is 100 (percent of that height). The page is never
  wider than the screen, and the scale is never less than 1. The value 0
  gives the retail page with no scale. At 1080 lines, the scale is 2.25,
  and the page is 1440x1080 in the middle of 1920 or 2560.
  - The game draws these in the menu space: the folders
    (`UpdateFolderState`), the message box (centred in the space,
    `CMessageBox::Show`), the loading screen (on its own thread), the
    pause notice, the mission-failure screen and the splash screens of
    Primal Hunt, which have no scale.
  - The game changes the point of the mouse into the space
    (`CInterfaceMgr::On*Button*`, `OnMouseMove`, `OnMouseWheel`). The keys,
    the focus and the updates of the folders run in the space, but only
    for input. They use the offsets of the page, not the size of the
    screen. Thus, a folder command that sets a camera rectangle from the
    screen still gets the real screen. `CInterfaceMgr::ScreenDimsChanged`
    runs in screen pixels for all callers. Thus, the cursor clicks a
    control where the game draws it.
  - The 3D parts of the menus are the background sprite, the scale FX
    and the character models (`CreateInterfaceBackground`,
    `CBaseFolder::CreateScaleFX`/`CreateCharFX`, `FolderPlayer`,
    `FolderPlayerJoin`, `LoadingScreen`). The game moves them back by the
    height of the menu space over 480 (`M15_MenuDepth`), where retail used
    its X ratio. Thus, with the Hor+ of M-1, they align with the page (1
    when the page fills the height).
  - The cursor is the cursor of the system, at its own size.
* The HUD space (M-16). Its scale is `HUDScale` percent (75 by default) of
  the height of the screen over 480. It is never less than 1 and never
  wider than the screen. It is 1.69 at 1080 lines, 1.125 at 720, and 1 at
  640x480 and 800x600.
  - The game draws these in the HUD space (`CInterfaceMgr::Draw`): the
    HUD (`CHudMgr`, the motion tracker), the mask of the scope, the
    crosshairs (the game changes the target of the smart gun, a point of
    the screen, into the space), the objectives and their notice, the
    weapon choosers, the credits, the timer and the subtitles. Also the
    messages and the chat line (`CMessageMgr`), the teletype, the story
    items and the turret ammo of Primal Hunt.
  - The game makes these layouts in the HUD space: `CHudMgr::Init` (the
    place of each element and of the tracker), the pickup list, the
    lines of a message and the place of a subtitle.
  - When `HUDScale` (or the screen) changes, the game makes the layout of
    the HUD again on the next frame (`M15_Refresh` in
    `CInterfaceMgr::PreUpdate`).
* These stay in screen pixels:
  - The items that are placed against the 3D view or the camera: the
    cinematic borders, the 4:3 bars of M-1/M-2, the overlays, and the
    target lock and vision wipe of the Predator.
  - The multiplayer scoreboard and its class menus
    (`MultiplayerClientMgr`). They already scale themselves: retail
    selects their fonts by the width of the screen (its largest set is
    above 1024).

The scale is smooth, not a whole number. The reasons are:

- The page must fill the height (2.25 at 1080 lines). A whole 2 leaves
  bands, and 3 does not fit.
- The art is painted 16-bit bitmaps and antialiased fonts, not pixel art.
  Nearest-neighbour at 2.25 draws it with columns that are not equal.

The renderer draws the interface in its optimized 2D mode, as textured
quads. Its `Bilinear` variable (on by default) sets the filter: bilinear.
The game rounds the size of the space to whole pixels and calculates the
two factors from it. Thus, the edges of the space are exactly the edges of
the screen.

| Id | What | Files | Status |
|---|---|---|---|
| M-15 | The menus are scaled. The 640x480 page fills the height of the screen (`MenuScale`, percent, 100; 0 is the retail size), centred. This includes its 3D parts, the message box, and the loading, pause, failure and splash screens. The game changes the clicks of the cursor onto the page. Options, Graphics, "Menu size" (Unscaled, 50%..100%) | `src/modern/cshell/m15_uiscale.h`; `InterfaceMgr.cpp`, `InterfaceResMgr.h`, `GameClientShell.cpp` (`OnEngineInitialized`, `OnEngineTerm`), `BaseFolder.cpp`, `FolderPlayer.cpp`, `FolderPlayerJoin.cpp`, `LoadingScreen.cpp`, `MessageBox.cpp`, `FolderDisplay.cpp`; `ClientRes.rc`, `ClientRes.h` (31800-31805) | done |
| M-16 | The HUD is scaled. The HUD of each species, the motion tracker, the crosshairs, the messages, the subtitles, the teletype, the objectives, the weapon choosers and the story items scale by `HUDScale` percent (75) of the height of the screen over 480. They are never smaller than the retail size. They are anchored as retail anchors them. Options, Graphics, "HUD size" (Unscaled, 50%..100%) | `m15_uiscale.h`; `InterfaceMgr.cpp` (`Draw`, `PreUpdate`), `HudMgr.cpp` (`Init`, `AddPickupIcon`), `CrosshairMgr.cpp`, `MessageMgr.cpp`, `Subtitle.cpp`, `TeleType.cpp`, `StoryElement.cpp`, `ClientTurretMgr.cpp`, `FolderDisplay.cpp`; `ClientRes.rc`, `ClientRes.h` | done |

We checked the result under Xvfb and Wine (`build/playtest.sh`,
`PLAYTEST_RES`, `PLAYTEST_MAP=modern`), with the defaults (`MenuScale`
100, `HUDScale` 75). `PLAYTEST_MAP=modern` is the page map of the playtest
for the centred 4:3 area scaled by the height. That is now the position of
the page.

* AvP2 at 1920x1080:
  - The game drew the main menu, Options and Graphics at 2.25 times. The
    page is 1440 wide, in the middle. The 3D backgrounds and the
    facehugger egg are also scaled.
  - The cursor clicked Options and Graphics at their page positions.
    When it hovered on Graphics at the page position (120, 238),
    Performance became highlighted.
  - The new Menu size and HUD size lines show 100% and 75%.
  - At 2560x1080, the page of Single Player is in the middle 1440
    columns, with the head of the Marine next to it.
  - At 640x480, the pages are as in retail (scale 1: all calls went
    through).
* AvP2, the first Marine, Predator and Alien missions at 1920x1080, and
  the Marine mission at 1280x720. Each HUD is in its corners at 1.69
  (1.125). We looked at the motion tracker and its range, the
  health/armour numbers, the flare count, the health and energy columns
  and skull of the Predator, the health bar of the Alien, the "objectives
  updated" notices, the training message, the teletype of the Predator
  and the crosshairs.
* Primal Hunt at 1920x1080: the main menu and its folders (as in AvP2),
  and the HUD of the first Corporate mission (its motion tracker, health,
  armour, flares). At 2560x1080, the HUD of the Predalien. At 1280x720,
  the HUD of the Predator.
* The subtitles of a cinematic (`Subtitles 1`,
  `RestrictCinematicsTo4x3 1`): the opening of the first Marine mission
  of AvP2 at 1920x1080. The game drew the line at 1.69 in the centred
  4:3 area.
* We did not get to these: the multiplayer scoreboard (it keeps the retail
  font-by-width scaling, with no change), the scope mask of the rail gun,
  the turret ammo of Primal Hunt and the boss meter. From the code, the
  game draws them in the HUD space, as the other items.

Also: the paging and the focus of the folders gave the screen point of
the cursor where they use a page point (M-B108, below).

### M-6: raw mouse input

Retail turns the view by the axis offsets of the engine
(`ILTClient::GetAxisOffsets`). The engine makes them one time each frame
from the buffered mouse data of DirectInput. It puts the cursor back in
the middle of the desktop on each frame (`CursorCenter 1`). Thus, the
turn for a count of mouse motion depends on the frame rate and on how the
motion is divided into events (RETAIL_BUGS.md, "The mouse look turns
further at a higher frame rate"). The modern build reads the motion of
the mouse itself, in the two games.

nolfsrc did this for NOLF (its M-P1-03, the raw mouse look, and M-P1-04,
the Mouse options). We used its design as the model, and we read its file
for it. The design is:

- `WM_INPUT` on the window that the client shell already subclasses.
- The raw-input declarations that VC6 does not have, written out, and the
  two functions found in user32.dll.
- The cursor is hidden and confined in play, and released in the menus.
- `OldMouseLook` and `NoRawInput`.

The code is the code of AvP2. We made it for the hooks of AvP2 and
Primal Hunt, which are different:

- The look is in `PlayerMovement`. An offset of 1 is 90 degrees of yaw
  about the up axis of the character (this includes the wall walking of
  the Alien), and 45 degrees of pitch.
- The game reads the look two times each frame (`UpdateRotation`,
  `UpdateAiming`), and the camera editor (`CameraMgrFX`) also reads it.
- The turrets of Primal Hunt aim by it.
- The scale is the scale of AvP2 (`SetMouseSensitivityScale`: the
  sensitivity, the zoom of the Predator and the slowing of the EMP).
- The NOLF scale is half, but this does not apply here (we measured the
  gain below here).

| Id | What | Files | Status |
|---|---|---|---|
| M-6 | The mouse look from raw input (`WM_INPUT`). A count gives the same turn at all frame rates. The cursor is hidden and kept in the window in play, and released in the menus. `OldMouseLook 1` gives the retail look. `NoRawInput 1` gives the motion of the cursor | `src/modern/cshell/m6_mouse.h`; `GameClientShell.cpp` (`HookedWindowProc`, `Update`, `OnEvent`, `SetMouseSensitivityScale`, `UnhookWindow`), `InterfaceMgr.cpp` (`UseCursor`), `src/AVP2/Shared/PlayerMovement.cpp` (`UpdateRotation`, `UpdateAiming`), `CameraMgrFX.cpp` | done |
| M-6 | Controls, Mouse: "Legacy mouse look" (`OldMouseLook`). The help of "Mouse smoothing" now says that it applies to the legacy look | `FolderMouse.cpp`; `src/AVP2/ClientRes/ClientRes.rc`, `ClientRes.h` (31600-31602) | done |

**The look.** The game adds the `WM_INPUT` of the mouse between frames.
These are relative counts. The game changes the input of an absolute
device (a tablet, a remote desktop) into motion. The game uses the sum
one time each frame (`M6Mouse_Update`, in `CGameClientShell::Update`, next
to the read of the controller). Thus, the two reads of `PlayerMovement`
see the look of the same frame, as they do with the offsets of the
engine.

A count is the offset of the retail scale, times 2:
`(0.166 + MouseSensitivity / 2.75) * scale * 0.001` across, and 1.1 times
that up and down. The factor 2 is because the retail look turned about
two times its own scale at 60 frames a second or less (below). These are
the frame rates at which people played the game. Thus, at the default
sensitivity (7), a count turns 0.49 degrees across and 0.27 up and down.

This look does not read the offsets of the engine. They hold only the axes
of the mouse. (The client shell reads the sticks of M-7 and adds them as
before.) DirectInput still gives the buttons and the wheel of the mouse
to the bindings. `InputRate` (Mouse smoothing) applies only to the legacy
look.

**The cursor.** In play (`CInterfaceMgr::UseCursor(FALSE)`):

- The cursor is hidden.
- The cursor is clipped to the client area of the window and kept in its
  middle. Thus, raw input that comes from a pointer (as from Wine without
  a clip, or from a remote desktop) never touches an edge.
- `CursorCenter` is 0.

In the menus, the cursor shows, and it gets the retail clip back (the
full-screen clip, or no clip in a window). While a different application
is in front (`WM_ACTIVATEAPP`), the game releases the clip and ignores the
motion. When the display mode changes (`LTEVENT_RENDERINIT`), the game
clips to the new window.

`OldMouseLook 1` is the retail look and `CursorCenter 1`, with no change.
`NoRawInput 1` reads the motion of the cursor from the middle of the
window on each frame. The game also does this if it cannot register raw
input. You can change the two settings in play (in the console). The look
changes on the next frame.

**The Mouse page** (Options, Controls, Mouse; the two games, one source):

- "Legacy mouse look" (No/Yes, `OldMouseLook`) is under the sensitivity.
  Its help is "No: the mouse's own motion, the same at any frame rate.
  Yes: the original mouse look."
- "Mouse smoothing" keeps its name and slider. Its help is "Smooths the
  legacy mouse look. The mouse's own motion is not smoothed."
- Restore defaults sets the legacy look to off.
- The page has space for the line (five controls, as shown).

**Checked** under Wine in the build container (the Wine 9.0 of Ubuntu
24.04, Xvfb). The relative motion of `xdotool` gets to the game as
`WM_INPUT` (Wine makes raw input from the motion of the X pointer). Thus,
the playtests move the mouse by known counts (`turn`). The scripts are
`build/playtest-avp2-mouse.txt`, `-mouse-predator.txt`,
`-mouse-alien.txt` and the `build/playtest-ph-mouse*.txt` of Primal Hunt.
Each script does these steps:

- It goes through the menus with the cursor to the first mission of each
  species.
- It turns 200 counts across and back, and 100 up and down, in 20-count
  moves and in 5-count moves.
- The Predator scripts then zoom (Page Up) and turn again.
- The Alien scripts turn with wall walking on (G).

A temporary probe recorded the view of the player on each frame and the
counts that the game used. (We removed its lines.)

| Game, species, limit | frames a second in play | degrees a count across | up and down |
|---|---|---|---|
| AvP2, Marine, 60 | 12-40 | 0.488 | 0.268 |
| AvP2, Marine, 240 | 22-37 | 0.488 | 0.268 |
| AvP2, Predator, 60 | 20-35 | 0.488; zoomed 0.390 | 0.268 |
| AvP2, Predator, 240 | 40-77 | 0.488; zoomed 0.390 | 0.268 |
| AvP2, Alien (facehugger), 60 | 32-45 | 0.488, wall walking too | 0.268 |
| Primal Hunt, Corporate, 60 | 23-57 | 0.488 | 0.268 |
| Primal Hunt, Corporate, 240 | 25-56 | 0.488 | 0.268 |
| Primal Hunt, Predator, 60 | 40-49 | 0.488; zoomed 0.390 | 0.268 |
| Primal Hunt, Predalien (facehugger), 60 | 60 | 0.488, wall walking too | 0.268 |

These values are for each move, in each run, to the third decimal. Thus,
the turn comes only from the counts, at all frame rates and for all sizes
of move. 200 counts right and 200 left come back to the start position.

The retail look (`OldMouseLook 1`, the same scripts):

- The Marine of AvP2: 0.39-0.43 degrees a count at 20-40 frames a second,
  0.55-0.57 at 30-55, and 0.61-0.70 at 60-140.
- The Corporate of Primal Hunt: 0.32-0.58 at 32-46. 200 counts right and
  back left ended 29 degrees from the start position.

The builds of the other machines put a load on the host of the container
all the time. No run got more than about 140 frames a second in play (no
raw input run got more than 77). Thus, we measured the 240 limit at the
rate that the host gave.

`build/playtest-mouse-menus.txt`, in the two games:

1. It goes to Options, Controls, Mouse with the cursor.
2. It sets Legacy mouse look to on (its help shows).
3. In the first Marine (Corporate) mission, 200 counts right and back
   turned 82 and 96 degrees in AvP2, and 36 and 92 in Primal Hunt.
4. It goes to the pause menu with the cursor, back to the Mouse page,
   sets Legacy mouse look to off, and clicks Resume.
5. Then 200 counts turned 97.6 degrees right and 97.6 back in the two
   games.

The game saves the setting (`"OldMouseLook" "0"` in autoexec.cfg).

Not done:

- A real mouse on Windows. (The raw input path is the same. Under Wine,
  we did not try raw input from a real device.)
- `RIDEV_NOLEGACY`. The game keeps the messages of the mouse, for the
  buttons of DirectInput and for the menus.

### M-9: window modes

The user reported: "I don't see a way to set fullscreen mode." There were
two causes:

- The run scripts (`linux-run-avp2.sh`, `linux-run-ph.sh`, from
  `build/game-scripts/linux-run.sh`) added `++screenwidth 640
  ++screenheight 480 ++bitdepth 32 ++windowed 1` to each run. Thus, each
  game started in a 640x480 window, for all settings of Display
  (including the wide modes of M-1).
- The games have no window mode in their options. The only retail window
  is from the `++windowed 1` of the command line. The renderer places it
  incorrectly (RETAIL_BUGS.md, "A window is placed and sized for a frame
  it does not have").

We read M-P1-06 and M-P1-22 of nolfsrc (the Display "Window mode" of
NOLF, and `p1_window.cpp`, the NOLF run script) for the design. We
checked the client shells, the engines and the two `d3d.ren`s of AvP2 and
Primal Hunt against it. The code is our code. The hook points are the
same:

- The `HookWindow` and `LTEVENT_RENDERINIT` of the client shell.
- The renderers read `Windowed` when they start.
- The engine stops the renderer on `WM_ACTIVATEAPP`.

The Display folder is the folder of AvP2 (it applies the settings when you
leave it, and it has its own `SetRenderer`). We found three problems under
Wine here that NOLF did not have:

- A framed window as large as the desktop: the tracking size of Windows.
- Wine draws a window black when its client area goes off the screen.
- A full screen that the display cannot accept at start-up.

| Id | What | Files | Status |
|---|---|---|---|
| M-9 | Options, Graphics (the Display folder), "Window mode": Full screen, Window, Borderless window. The engine has `Windowed` (the two renderers read it when they start) and `WindowMode` (2: that window is borderless). The game saves the two. It sets the mode again when it changes. After each renderer start (a mode set, or the focus comes back), the game corrects the window. Full screen: as `HookWindow` makes it (no frame, topmost, the size of the mode). A window: with a title bar, and the size of the mode as its client area, centred in the work area. (A window as large as the desktop has its client area on the screen and the frame off the screen. `WM_GETMINMAXINFO` lets it be larger than the screen.) A borderless window: `WS_POPUP` at 0,0, at the resolution of the desktop (the registry mode of `EnumDisplaySettings`). The resolution list is grey for it. When a different part sets a mode, the game changes it to the mode of the desktop on the next frame. A first run (no `WindowMode`) starts borderless, if it does not ask for a window. If full screen fails at start-up (the display cannot accept the mode), the game starts as the borderless window, before the retail 640x480x16 fallback. If a full screen that you select in the folder fails, the game goes back to the previous window mode | `src/modern/cshell/m9_window.h`; `GameClientShell.cpp` (`OnEngineInitialized`, `OnEvent`, `Update`, `HookedWindowProc`), `FolderDisplay.cpp` (`Build`, `OnCommand`, `SetRenderer`, `OnFocus`); `src/AVP2/ClientRes/ClientRes.rc`, `ClientRes.h` (31900-31904) | done |
| M-9 | The run scripts keep the window settings of the game. `linux-run-avp2.sh` / `linux-run-ph.sh` give a mode only when you ask for one: `--fullscreen`, `--windowed`, `--borderless` (of the modern build), `--res WxH` (as `++` arguments, so the game saves them). In the release build, they also give a mode on the first run (no `autoexec.cfg`): a 640x480 window, as before. `windows-run.bat` (which never added a mode) accepts the same options | `build/game-scripts/linux-run.sh`, `windows-run.bat`; `build/all.sh` (`@MODERN@`) | done |
| M-21 | In a window or a borderless window (the `Windowed` 1 of the engine), when the game loses the focus, it does not stop the renderer or hold the sounds. When the focus comes back, it does not start them again (RETAIL_BUGS.md: "Alt+Tab freezes the picture"). The focus events, the input that the game clears and the single-player pause are the same as retail. Full screen keeps the retail behaviour. Checked under Xvfb and Wine 9.0, `build/playtest-avp2-focus.txt` (the `blur` / `wmfocus` of the harness: the focus goes to a different X client, then comes back as from a window manager). Before: the opening cinematic stopped on its frame permanently, and the game continued to run and save behind it (the retail binaries in a window: a black window). After: the cinematic continues, the picture moves and the keys work. In play, the motion tracker and the view move again | `src/engine/lithtech/clientwin.cpp` (`clu_WndProc`, `M21_KeepRenderer`) | done |

**Full screen under Wine.** The full screen of the two renderers is the
exclusive mode of DirectDraw, with `SetDisplayMode` to the selected
resolution. Under Wine, that changes the mode of the X display. This
fails if the X server does not have that mode. Xvfb has only its own
size. Thus, there (and on X servers or Wayland compositors that do not
have the mode), `NtUserChangeDisplaySettings` returns -2
(`DISP_CHANGE_BADMODE`). Then `SetDisplayMode` fails, and the renderer
also fails. At the resolution of the desktop, full screen works
(measured: the two games, 1280x720 on a 1280x720 Xvfb, the window is
`WS_VISIBLE`, topmost, 0,0 1280x720). For this reason, the borderless
window is the mode of the first run, and the fallbacks above exist. The
run scripts of the release build keep their window.

**The run scripts.** The release build has no window option of its own.
Thus, its first run is still a 640x480 window (the full screen of its
renderer fails under many Wine setups). Its subsequent runs use the
settings that it saved. `--fullscreen`, `--windowed` and `--res` change
them. The scripts refuse `--borderless` for this build. We checked each
option in the two builds with a replacement for `wine` that prints the
command file of the engine. We checked `windows-run.bat` under the `cmd`
of Wine, with its command file for each option. (We used a copy without
its `cd /d ... || exit /b 1`, because the `cmd` of Wine 9.0 stops at that
line. The script had that line before.)

We checked the result under Xvfb and Wine 9.0 in the build container, on
a 1280x720 display, in the two games. A test harness ran the
`linux-run-*.sh` of the game directories under `xvfb-run`. It measured the
window as X sees it (`xdotool`), and as Windows sees it (a small Win32
program that lists its style, rectangles and state). It closed the game
with `WM_CLOSE`, so that the game saves `autoexec.cfg`. The results:

* A first run, no options: borderless, `WS_POPUP`, 0,0 1280x720 (the
  client area is the same). The menus are scaled to 720 lines (M-15). The
  game saved `Windowed 1`, `WindowMode 2`, 1280x720.
* `--windowed --res 800x600` (AvP2), `--res 1024x576` (Primal Hunt): a
  title bar, the client area 800x600 at 240,73 (1024x576 at 128,85),
  centred. The main menu is in it. The next run with no options is the
  same (the game saved it).
* `--windowed --res 1280x720`: the client area is 0,0 1280x720, and the
  frame is off the screen. (Before, Windows cut the window to the screen,
  with a 1280x700 client area. Wine drew it black, as it did all windows
  that went off the screen.)
* `--fullscreen` at 800x600 (AvP2) / 1024x576 (Primal Hunt): the display
  refused the mode. The game started borderless at 1280x720 and saved
  that. `--fullscreen --res 1280x720`: full screen, topmost.
* In the menus (AvP2, Options, Graphics, with the cursor): Borderless
  window -> Window -> Full screen -> Borderless window. The game applied
  each mode when the script left the folder, and we measured it. After
  each mode, the cursor went into the folder again at its page position
  (the client area of the window at 0,0, the page mapping of M-15 with no
  change). The resolution is grey in the borderless window. The game
  saved the choice.
* The focus (a different program, the Notepad of Wine, comes to the
  front):
  - Borderless: the window stays in its position, behind the other
    program.
  - Full screen: the DirectDraw of Wine minimises the window (measured:
    iconic), as the DirectDraw of Windows does. Thus, no black window
    stays over the desktop. The placement restores the window when the
    focus comes back. (To bring it back, you need a window manager or a
    taskbar. The container does not have them.)
* In play: the first Marine mission of AvP2
  (`build/playtest-avp2-marine.txt`) in a 1024x576 window on a 1280x720
  desktop, and the first Corporate mission of Primal Hunt
  (`build/playtest-ph-corporate.txt`) in the borderless window at
  1280x720. The script clicked the menus at their page positions, played
  the mission, did a quicksave and a quickload. The HUD is scaled (M-16).

Not done:

- A window manager under Xvfb (the build image has none). Thus, we did not
  see Alt+Tab back into a minimised full screen, or the frame of the
  window as a desktop draws it.
- A full screen that fails when you select it in the folder. (Xvfb lists
  only its own resolution, and full screen accepts that resolution.)

### M-7: controllers through XInput

Retail reads one DirectInput joystick. You set its axes on the Joystick
page. There is no radial dead zone, the two triggers of an Xbox pad are on
one axis, and there is nothing for the menus. The modern build reads an
XInput pad in place of the joystick, in the two games. nolfsrc did this
for NOLF (its M-P7-01..06 and M-P7-09). Its design and its device in the
engine are the code of this project family, and we used them as the
model. But we made each hook for the code of AvP2 and Primal Hunt, which
is different:

- The bindings are in the profile of the player, for each species, not in
  `defctrls.cfg`.
- The movement is `PlayerMovement`/`CharacterMovement`, not `CMoveMgr`.
- The look of the mouse is axis offsets of 90 degrees of yaw and 45
  degrees of pitch.
- The folders are the folders of AvP2 (the Joystick page becomes the
  Controller page).

| Id | What | Files | Status |
|---|---|---|---|
| M-7 | `##gamepad`: the pad as the joystick of the engine, read through XInput (xinput1_4, 1_3 or 9_1_0, loaded at run time) | `src/modern/engine/p7_gamepad.h`, `m7_padread.h`; `src/engine/lithtech/clientinput.cpp` (the four hooks of nolfsrc, as imported) | done |
| M-7 | Default bindings, for each species, in the joystick column of the profile | `src/modern/cshell/m7_padbindings.h`; `ProfileMgr.cpp` | done |
| M-7 | Analog look (right stick) and movement (left stick) | `src/modern/cshell/m7_pad.h`; `GameClientShell.cpp` (`Update`, `SetMouseSensitivityScale`), `PlayerMovement.cpp`, `CharacterMovement.cpp` | done |
| M-7 | The menus with the pad | `m7_pad.h`; `GameClientShell.cpp` | done |
| M-7 | Controls, Controller (was Joystick) | `src/modern/cshell/m7_folderjoystick.h`; `FolderJoystick.cpp`; `ClientRes.rc`/`.h` (31100-31108; `IDS_JOYSTICK` "Controller", `IDS_DEVICE_JOYSTICK` "Pad") | done |
| M-7 | Customize Controls: when you bound a joystick button to a second command, the first command kept it (RETAIL_BUGS.md) | `FolderCustomControls.cpp` | done |

**The device.** `##gamepad` is an object behind `IDirectInputDevice2A`.
It answers the joystick calls of the engine from the state of XInput (the
first connected pad of four). It asks about empty slots a maximum of one
time each second. It has:

- Six absolute axes: `Left`/`Right Stick X`/`Y` (-1000..1000, Y down is
  positive) and `Left`/`Right Trigger Axis` (0..1000).
- Sixteen buttons: `A`, `B`, `X`, `Y Button`, `Left`/`Right Bumper`,
  `Back`, `Start`, `Left`/`Right Stick Click`,
  `D-Pad Up`/`Down`/`Left`/`Right`, and `Left`/`Right Trigger` past the
  threshold of XInput.

It buffers the data in the data format that the engine sets. It is
acquired only while the game is in the foreground. It is the only
joystick. `clientinput.cpp` does not use the DirectInput joysticks (under
Wine, the same pad is one of them). It enumerates no devices (the crash
fix of M-P7-09). `JoystickDisable` does not apply to it. When
`AVP2SRC_PADTEST` names a file, the pad uses the state in that file
(`m7_padread.h`). This is the test controller of the playtest.

**Bindings.** A profile keeps each command as `key@mouse@joystick`. The
joystick column is for the pad. The game binds it when the pad is
connected. ("Use joystick" was for the DirectInput joysticks, and it does
not control the pad now. The game keeps the value of the profile as it
was, for retail.) Some profiles have no `PadLayout` in `[Basic]`:
`Default.prf` (thus also Restore defaults of Customize Controls), and all
profiles made before. Such a profile gets the defaults where its column
is empty and the button is not in use. The game saves it with
`PadLayout = 1`, so the changes of the player stay.

The default bindings are:

- All species: right trigger Fire, left trigger Alt-Fire, A Jump, left
  stick click Run, right stick click Center View, Back Objectives /
  Scores, d-pad up Next Vision Mode.
- Marine (and the Corporate of Primal Hunt): B Duck, X Activate, Y
  Reload, right/left bumper Next/Previous Weapon, d-pad down Shoulder
  Lamp, left Flare, right Hacking Device.
- Predator: B Duck, X Activate, Y Reload, bumpers the weapons, d-pad down
  Cloak, left Zoom Out, right Zoom In.
- Alien (Predalien): B Duck, X Pounce, Y Wallwalk Toggle, left bumper
  Wallwalk.
- Start is the menu (not bound).

You bind the buttons in Customize Controls as keys ("Pad A Button"). The
game never binds the sticks as axes (`ApplyJoystick` does not write the
retail axis bindings now), because the client shell reads them. The game
enables the device of the pad before it binds its buttons
(`ApplyBindings`).

**Look.** The right stick gives a turn rate:

- It has a radial dead zone (`PadLookDeadZone`).
- The game changes the rest of its travel to 0..1 and raises it to
  `PadLookCurve`.
- The rate is 180 degrees a second across and 120 up and down, times
  `PadLookSpeed` and the frame time.
- The game adds it to the axis offsets of the mouse in
  `PlayerMovement::UpdateRotation`/`UpdateAiming`. Thus, it also turns the
  turrets of Primal Hunt, it keeps to the limits that multiplayer gives
  the mouse, and it looks when mouse look is on or off.
- It becomes slower with the zoom of the Predator, as the mouse does (the
  scale of `SetMouseSensitivityScale`).
- `PadInvertY` inverts its pitch. The invert of the mouse also applies.

**Movement.** The left stick past `PadMoveDeadZone` sets the movement
flags by octant (the animations and the server see these). Its direction
is the direction of the acceleration
(`CharacterMovement::UpdateAcceleration`, client only). Past
three-quarters of its travel, the player runs. Below that, the player
walks at that share of the walk speed, with a minimum of a third (the
speed limit of the walking state). When you hold Run, it inverts this.
RunLock does not apply to the stick.

**Menus.** Outside play:

- The d-pad or the left stick gives the arrow keys (they repeat after
  0.4 s, every 0.12 s).
- A gives Enter.
- B and Start give the menu key.
- The right bumper gives Tab.

In play, Start is the menu key. A skips a cinematic or a line of dialogue
(the cinematic skip key), and it continues after a single-player death.
The keys go to the `OnKeyDown`/`OnKeyUp` of the client shell, as the
engine sends a key. While Customize Controls waits for a control, it
ignores keys. Thus, when you push a pad button, it binds that button.

**The Controller page** (Options, Controls; the retail Joystick page, at
the same place and with the same layout) has:

- Look speed (25-300%).
- Invert look.
- Look dead zone and Move dead zone (0-50%).
- Restore defaults (these settings, and the default bindings of the pad
  in place of its column of the profile).

**Checked** under Wine in the build container, in the two games, with the
test controller (`pad`/`tap` of `build/playtest.sh`, the file that
`AVP2SRC_PADTEST` names). We could not use a real or virtual pad. The
XInput of Wine gets its data from its HID backends (udev/hidraw/evdev or
SDL), and these need a kernel device. The `/dev/uinput` of the host
belongs to root. Thus, we ran the XInput path only with no pad connected.
The games run as before with the `xinput1_4.dll` of Wine there and no test
pad. The code after the read of the state of the pad is the same for the
two paths.

`build/playtest-avp2-pad.txt` and `build/playtest-ph-pad.txt` do these
steps:

1. They go through the main menu with the d-pad.
2. They select Single Player, Marine (Corporate) and Launch with A.
3. They skip the intro cinematic with A.
4. Then they play. A temporary probe recorded the player on each frame.
   (We removed its lines.)

| | AvP2 | Primal Hunt |
|---|---|---|
| right stick full right, 1 s | 184 degrees | 177 |
| half right (curve 2, dead zone 0.15), 1 s | 31.4 | 30.9 |
| full up, 0.27 s | 32.4 up | 35.3 (0.29 s) |
| right stick click (Center View) | pitch back to 0 | 0 |
| left stick half forward | 61 u/s (walk, scaled) | 63 |
| full right / forward-left (run) | 242 (a wall) / 300 | 297 / 300 |
| right trigger | Fire on 2.1 s (the blade swings) | 2.2 s |
| A in play | a jump (+70 units) | a jump |
| Start, then B | the pause menu, then play (when a training message shows, Start removes it first, as Escape does) | the same |
| d-pad down (Shoulder Lamp) | the lamp on (its meter shows) | the same |

These results are at 60-150 frames a second.

`build/playtest-pad-menus.txt`, in the two games:

1. It goes to Options, Controls with the d-pad and A.
2. Customize Controls lists the defaults ("Pad Right Trigger", ...).
3. It binds Fire to Y through the capture. Then the profile has
   `Fire = " @##3@Y Button"`, and Reload does not have Y, in all species.
4. It sets the look speed of the Controller page one step up (saved:
   `PadLookSpeed 1.25`).
5. It goes back to the main menu with B.

Not done:

- Rumble (nothing in the two games asks for force feedback).
- Button prompts in hints.
- A real pad (Windows, or Wine with a kernel device).

We also found a problem that is not in the pad code. When the player
threw a flare (the Marine binding of d-pad left, or F on the keyboard),
the modern AvP2 and Primal Hunt engines crashed. This is not a modern
change and not a retail bug. The legacy build crashed in the same way,
and the retail binaries do not crash (`build/playtest-avp2-flare.txt`,
`playtest-ph-flare.txt`). It was a reconstruction error, and we fixed it
in the two builds:

- The message-object helper of the client (`clientmsghelper.cpp`, the
  `m_pObjectIO` of each message that the client reads) compiles a
  two-slot vtable.
- The link puts a full-length table in its place
  (`src/engine/lithtech/link/vtables-avp2.txt`, `vtables-ph.txt`;
  `build/vtables/map-lithtech-*.txt`). That table had no entry for it.
- Thus, the `WeaponSoundFX` message of a projectile reads a compressed
  position (`ReadCompPos`, its slot 3, `GetPosRef`). It called the next
  table in `.rdata` (`LThread::ThreadTerm`) on it.

### Retail-bug fixes: the engine (M-B01..M-B09)

These are the fixes for the bugs in `RETAIL_BUGS.md`. Each entry there
names its id. Each fix is an `AVP2SRC_MODERN` block with the retail code
in its `#else`, or modern-only lines that the retail code does not have.

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-B01 | The texture sweep clears only its mark, not the reference count of the texture | `src/engine/lithtech/clienttexmgr.cpp` (`tm_UnmarkTextures`) | From the code. Only the Get/ReleaseTextureHandle of LTTexMod write the count, and the DLLs of the two games do not call them. Thus, nothing in play gets to it. The two games load and play their first missions as before (the flare scripts; the sweep runs at each load) | done |
| M-B02 | The command line: the engine keeps a maximum of 150 arguments, from the real command line and from `-cmdfile`. When a `-cmdfile` read fails, the engine releases its buffer | `src/engine/lithtech/clientwin.cpp` (`clu_dsi_BuildCommandLine`) | AvP2, `build/playtest.sh` to the main menu, with `PLAYTEST_ARGS` of 70 `+x<n> 1` pairs (163 arguments in the command file). The legacy build crashes in `clu_dsi_BuildCommandLine` (it reads argument text as a pointer, then it overwrites the handler chain of the stack). The modern build runs, and it discards the arguments after the 150th | done |
| M-B03 | `ServerModelLT::SetCurAnimTime` overrides the three-argument function of the interface. Thus, a call from a game flags the object for its clients | `src/engine/server/servermodel.h`, `servermodel.cpp` | The `ServerModelLT` tables of the four modern engines (the `lithtech.exe` and `server.dll` of the two games): slot 33 is now `ServerModelLT::SetCurAnimTime`, and there is no slot 37. The two games load and play their first missions (`build/playtest-avp2-flare.txt`, `playtest-ph-flare.txt`) | done |
| M-B04 | The world name of `CreateWorldCRC` always has a terminator | `src/engine/server/smgr.cpp` | From the code (you cannot make a name of 255 characters or more in play) | done |
| M-B05 | `sm_CreateInterObjectLink` checks its three allocations, and fails with `LT_ERROR` | `src/engine/server/interlink.cpp` | From the code (out of memory only) | done |
| M-B06 | The CD driver moves its play list forward only when a track played to its end (`MCI_NOTIFY_SUCCESSFUL`). It does not do this when a different play replaces the play, or when the play stops | `src/engine/cdaudio/cdaudio.cpp` (the `cdaudio.dll` of Primal Hunt) | From the code. The two games do not play CD audio in our tests (Wine has no Red Book drive; the music is DirectMusic and Miles). Thus, the tests do not get to the driver | done |
| M-B07 | The `GetSongFromList` of the CD driver gives the song at the index | `src/engine/cdaudio/cdaudio.cpp` | From the code. The two games do not play CD audio in our tests (Wine has no Red Book drive; the music is DirectMusic and Miles). Thus, the tests do not get to the driver | done |
| M-B08 | The CD driver forgets the current play list when the play list is removed | `src/engine/cdaudio/cdaudio.cpp` | From the code. The two games do not play CD audio in our tests (Wine has no Red Book drive; the music is DirectMusic and Miles). Thus, the tests do not get to the driver | done |
| M-B09 | `sm_DetachClient` allocates the message that it sends | `src/engine/lithtech/ccobj.cpp`, `src/engine/server/smgr.cpp` | From the code. Only `ILTServer::AttachClient` gives a view client to a client, and the DLLs of the two games do not call it. Thus, play never gets to the detach | done |

### Retail-bug fixes: the renderers (M-B20..M-B26)

These are the `RETAIL_BUGS.md` entries in `src/engine/d3d/ph/` (the
`d3d.ren` of the two games, if the row does not give a different file).
We first made sure of each entry against the retail code. A pure
addition (a test, a free) is an `#ifdef` only. All other changes keep the
retail lines in the `#else`. One entry was not a bug ("The model sun's
variance reads the warble scale": the table is the table of
ModelWarble). We did not fix it.

| Id | What | Files | Status |
|---|---|---|---|
| M-B20 | The renderer clears all of a dynamic lightmap, not a quarter (RETAIL_BUGS.md: "A dynamic lightmap is cleared only a quarter of the way") | `lmdynamic.cpp` (`d3d_FillDynamicLM16`, `d3d_FillDynamicLM32`) | done |
| M-B21 | The renderer draws the unsorted translucent world models of Primal Hunt with their additive states ("Unsorted translucent world models lose their blend states before drawing") | `drawworldmodel.cpp` (`d3d_DrawNoSortWorldModel`) | done |
| M-B22 | The box test puts the +X, +Y and +Z planes at their faces ("The renderer's box test builds three of a box's six faces at the wrong place") | `fullintersectline.cpp` (`DO_PLANE_TEST_X/Y/Z`) | done |
| M-B23 | The renderer releases the bitmap of a lightmap page when there is no lightmap format ("A lightmap page leaks its allocation bitmap ...") | `lmpaging.cpp` (`d3d_AddLMPage`) | done |
| M-B24 | `CD3DLMTexture::Init` fails when it cannot make its surface. It does not lock null ("A dynamic-lightmap texture whose surface cannot be made locks a null surface") | `d3d_optimizedsurface.cpp` | done |
| M-B25 | The specular pass of the models keeps its stage-1 states during its draw ("The models' specular pass sets its texture-stage states and undoes them at once") | `drawmodel_env.cpp` (`CD3DModelDraw::DrawPasses`) | done |
| M-B26 | The screen-space and lightmap-only polygon draws compare the vertex count with their 64-entry buffers ("A screen-space lightmap polygon is copied without checking its vertex count") | `drawlightmaps.cpp` (`d3d_DrawScreenLMPoly`, `d3d_DrawLightmapOnly`) | done |

Checked: the modern build compiles them (`make modern`), and the two
games run on it (`build/playtest-avp2-marine.txt` and
`build/playtest-ph-corporate.txt`: the first mission of each loaded,
played, saved and loaded again, and drew as before). You cannot see these
bugs in the usual play of the shipped games. Thus, each fix depends on the
code:

- M-B20 shows only on a polygon with a lightmap page that has dynamic
  light. (If not, `LMDrawDesc::Begin` fills the buffer with black, so the
  old three quarters are black.)
- M-B21 shows only on a Primal Hunt world model whose first surface is
  unsorted.
- M-B22 is the plane that `IntersectSegment` returns for the box of an
  object (n.p = d: a +X face at MaxBox.x has d = MaxBox.x, -X has
  d = -MinBox.x, which is `planeCoord * normalDirection` for all six).
- M-B23 and M-B24 need a device with no lightmap format, or a
  CreateSurface that fails.
- M-B25 needs a model with `SpecularEnable` in its command string. No
  shipped model of the two games has it.
- M-B26 needs a polygon of more than 64 vertices.

### Retail-bug fixes: the client shells and the lithshared libraries (M-B40..M-B63)

Each row is a `RETAIL_BUGS.md` entry (its heading is in the second
column). Each fix is behind `AVP2SRC_MODERN`, with the retail code in the
`#else`. The fixes in the libraries apply to all the DLLs that link them,
in the two games:

- ButeMgr: the client shells and the object DLLs.
- GameSpyMgr: the object DLLs.
- GameSpyClientMgr and LithTrackMgrClient: the client shells.

| Id | What (RETAIL_BUGS.md) | Files | Checked | Status |
|---|---|---|---|---|
| M-B40 | Setting an attribute that only an expansion tag holds does not take | `src/lithshared/ButeMgr/butemgr.cpp` (`GetSymTabItem`) | Test program (below): AvP2 retail gives 2 after we set 20, modern gives 20 | done |
| M-B41 | The attribute tables leak on every Term | `butemgr.cpp` (`TraverseTableOfTagsDeleteSymTab`) | Test program: retail +2749 bytes of heap for each Parse/Term of three tags, modern +0 | done |
| M-B42 | Deleting a track key leaves a duplicate that is freed twice | `src/lithshared/lithtrackmgr/LithTrack.cpp` | Code (the key delete of the editor) | done |
| M-B43 | Deleting a track leaves a duplicate that is freed twice | `LithTrackList.cpp` | Code | done |
| M-B44 | The track editor's "Font Type" command never changes the font | `LithTrackMgr.cpp` (`Ext_Type`) | Code | done |
| M-B45 | The track editor's help text ignores the bar's width | `LithTrackBarHelp.cpp` | Code | done |
| M-B46 | The track editor's draw object always reports failure | `LithTrackDrawObj.cpp` | Code (its caller ignores the result) | done |
| M-B47 | A GameSpy secure query can overrun the server's buffer | `src/lithshared/gamespy/gamespymgr/GameSpyMgr.cpp` (`OnSecureQuery`) | Code | done |
| M-B48 | The LAN server list searches an unterminated reply | `src/lithshared/gamespy/gamespyclientmgr/gserverlist.c` (`ServerListLANList`) | Code | done |
| M-B49 | Sorting the server list by an unknown mode uses an uninitialised comparator | `gserverlist.c` (`ServerListSort`) | Code | done |
| M-B50 | The server browser never checks or closes sierraup.cfg | `GameSpyClientMgr.cpp` (`SetupDirServers`) | Code | done |
| M-B51 | Refreshing LAN servers removes other games' servers by the wrong port | `GameSpyClientMgr.cpp` (`RefreshLANServers`) | Code | done |
| M-B52 | The "nelno" cheat's message isn't terminated | `src/AVP2/ClientShellDLL/CheatMgr.cpp` | Code | done |
| M-B53 | Naming the console's actions leaks a string per command | `GameClientShell.cpp` (`OnEngineInitialized`) | Code | done |
| M-B54 | The cloaked weapon's alpha floor writes the red channel | `WeaponModel.cpp` (`UpdateWeaponModel`) | Code | done |
| M-B55 | A mounted turret with a full circle of yaw never wraps its yaw | `ClientTurretMgr.cpp` (`CClientTurret::Rotate`) | Code | done |
| M-B56 | The turret recoil pushes the first-person weapon off by 0.95 on every axis | `CameraMgrFX.cpp` (`CameraMgrFX_RecoilUpdate`) | Code | done |
| M-B57 | Putting the weapon away for a turret hides muzzle flashes it may not have | `WeaponModel.cpp` (`Holster`) | Code | done |
| M-B58 | The host options take check-box commands past the end of their boxes | `FolderHostOptions.cpp` (`OnCommand`) | Code | done |
| M-B59 | The host options' check boxes leak their bitmaps | `FolderHostOptions.cpp`, `.h` (`Term`, `DeleteBoxBitmaps`) | Code | done |
| M-B60 | The server list's no-entry surface is never deleted | `FolderJoin.cpp` | Code | done |
| M-B61 | The Corporate motion detector settings check the Marine's tag | `LayoutMgr.cpp` (the seventeen `GetMD...`/`...SurfaceName` functions) | Code. The layout of Primal Hunt has the two tags, so nothing changes with its data | done |
| M-B62 | The Marine weapon chooser's dual pistols check has no null test | `WeaponChooser.cpp` (`IsDualPistols`) | Code | done |
| M-B63 | A tracer whose Init fails removes three garbage flare handles | `TracerFX.h` (the constructor) | Code | done |
| M-B108 | The paging of the folders, `ForceMouseUpdate` and the focus of a group control gave `OnMouseMove` the screen point of the cursor (`CInterfaceMgr::GetCursorPos`), where it uses a page point. Now they give the page point (with M-15) | `BaseFolder.cpp` (`NextPage`, `PreviousPage`, `ForceMouseUpdate`), `GroupCtrl.cpp` (`OnSelChange`) | From the code. At 640x480, the two points are the same | done |
| M-B109 | When a 3D provider is open, the engine reads "Maximum supported samples" into an uninitialised local variable and uses it as the number of 3D voices (RETAIL_BUGS.md: "The 3D voice count can come from an uninitialised value"). Now the value starts at 0, so a provider that does not answer gives no 3D voices, not a random number | `clientsoundmgr.cpp` (`CSoundMgrView::Init`) | From the code. OpenMiles answers the attribute now (M-X7) | done |

**Checked.** `make modern` builds the two games with these fixes. We
checked the ButeMgr fixes with a test program (`cl` 8966 under Wine,
linked with the `ButeMgrNoMFC.lib` of each build). The program does these
steps:

1. It parses a buffer with `[Weapon]`, `[Weapon_EXPANSION]` and
   `[Other_EXPANSION]`.
2. It does `SetInt` on each, then `GetInt` back.
3. It does 100 `Term`s and `Parse`s, and it walks the CRT heap before
   and after.

The legacy library reads the old value of the expansion tag (2 for 20),
and it holds 274880 bytes more after the 100. The modern library reads 20
and holds no more bytes.

The other fixes correct crashes, leaks and bounds on paths that the
retail data and menus do not get to in a scripted run. Examples are: a
missing `sierraup.cfg`, a LAN reply that fills its buffer, a weapon ID
with no weapon, the yaw of a turret past a full turn, the deletes and
font command of the track editor, and a tracer whose Init fails. We
checked each from the code, and the games still run (`make modern`, then
the first Marine mission of AvP2 under Wine). The layout of Primal Hunt
has the two motion-detector tags, so M-B61 changes nothing with its data.

### Retail-bug fixes: the object DLLs and the shared code (M-B70..M-B87)

These are fixes of `RETAIL_BUGS.md` entries in the object DLLs and in the
code that they share with the client (`src/AVP2/ObjectDLL/`,
`src/AVP2/Shared/`). Each entry is under the heading of the entry there.
Each fix is behind `AVP2SRC_MODERN`, with the retail code in the `#else`.
The fixes are in Primal Hunt, if the row does not say a different game
(the code is the code of Primal Hunt).

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-B70 | `SVAR OldCloak` sends nothing when it cannot make a message. It does not write through null | `ObjectDLL/CommandMgr.cpp` | Code (`CreateMessage` fails only when the engine has no more messages) | done |
| M-B71 | The player-view fx cache writes and reads all the types, not one past the last | `Shared/FXButeMgr.cpp` (`PVFX::Write`, `Read`) | Code. The game never writes or reads a bute cache (`CGameButeMgr::WriteCache`/`ReadCache` return false first) | done |
| M-B72 | A cached projectile fx with class data of kind 0 (or of an unknown kind) reads no class data. It does not read through null | `Shared/FXButeMgr.cpp` (`PROJECTILEFX::Read`) | Code (as M-B71) | done |
| M-B73 | A tracer without SingleSegment is multi-segment. Its value does not come from the old contents of its memory | `Shared/FXButeMgr.cpp` (`TRACERFX::TRACERFX`) | Code. All shipped tracers name SingleSegment | done |
| M-B74 | The AdditiveAlpha of a scale fx starts cleared, and the bute cache keeps it | `Shared/FXStructs.cpp` (`CScaleFX`) | Code (as M-B71) | done |
| M-B75 | An insert into a restriction list moves the entries up from the top, and it counts the new entry | `Shared/MPRestrictList.h`, `Shared/ServerOptions.h`, `ObjectDLL/MPRestrictions.cpp` | Code. The shipped bute files list the "_MP" sets and weapons in ID order. Thus, only a custom file, or the game info of a server that is not in order, gets to it. Host test (below): when we add the IDs 5, 1, 3, 2, 4, the result is 1 2 4 (retail) and 1 2 3 4 5 (modern) | done |
| M-B76 | The "allowed" checks of the server for classes and weapons accept only the entries of the list | `ObjectDLL/MPRestrictions.cpp` | Code. The indices of the callers come from the Find functions | done |
| M-B77 | `CMPRestrictList::IsAllowed` accepts only the entries of the list (the shared header of the two games; the multiplayer class checks of the client) | `Shared/MPRestrictList.h` | Code (as M-B76). Host test: `IsAllowed(32)` on a full list gives false | done |
| M-B78 | `CMPRestrictList::SetAllowed`, `GetId` and `GetName` accept only the entries of the list. Thus, the host options do not write past a full list | `Shared/MPRestrictList.h` | Host test: on a full list, `GetId(32)` gave the count, 32 (retail). Now it gives -1, and `SetAllowed(32)` writes nothing | done |
| M-B79 | When a cached string is too long for its buffer, the reader moves past all of it | `Shared/MultiFile.cpp` (`CMultiFile_FILE`, `CMultiFile_LITH::ReadString`) | Host test: a 16-character string read into 8 bytes, then an int written as 42. Retail reads 1094268983, and modern reads 42, through the two classes. The game reads no cache in play (as M-B71) | done |
| M-B80 | When the player loses a carried turret, the player keeps the other turrets | `ObjectDLL/PlayerObj.cpp` (`CPlayerObj::RemoveTurret`) | Code. With two turrets, when the player loses the first, the second now stays (before, none stayed) | done |
| M-B81 | A fog volume that fades keeps its red | `ObjectDLL/VolumetricFog.cpp` | Code | done |
| M-B82 | The attack and the burrow of a tentacle each run their own update. An attack lasts its second, then the tentacle burrows | `ObjectDLL/Tentacle.cpp` | Code | done |
| M-B83 | A turret gun with a full turn of yaw wraps the yaw | `ObjectDLL/TurretGun.cpp` | Code | done |
| M-B84 | When the target of a facehugger is not a character, its attack does not crash | `ObjectDLL/SimpleAIAlien.cpp` | Code | done |
| M-B85 | A tracer without a Flare has no flares. It does not get sprite names from the old contents of memory (we found this here) | `Shared/FXButeMgr.cpp` (`TRACERFX::TRACERFX`) | Code. The Standard and Laser tracers have no Flare | done |
| M-B86 | The server runs `MID_CONSOLE_COMMAND` (its command manager) and `MID_CONSOLE_TRIGGER` (a message to the objects of a name) only from the local client: the player in single player, or the player who hosts the game. AvP2 ran both from each client, Primal Hunt ran the trigger from each client (RETAIL_BUGS.md: "A remote client can run commands and triggers on the server") | `ObjectDLL/GameServerShell.cpp` (`HandleConsoleCmdMsg`, `HandleConsoleTriggerMsg`) | Code. In single player, the client is the local client (the `Trigger` console program still works) | done |
| M-B87 | The dialogue ID string gets its terminator at its last byte, not one byte after its end (RETAIL_BUGS.md: "The dialogue ID string is terminated one byte after its end") | `ObjectDLL/Character.cpp` (`CCharacter::PlayDialogue`) | Code | done |

**Checked.** The modern builds of the two games compile (`make modern`).
They play `build/playtest-ph-corporate.txt` to the end (the first Marine
mission of AvP2 and the first Corporate mission of Primal Hunt, quicksave
and quickload; no crash). None of these fixes has an effect that the
shipped games show:

- The game never reads or writes the bute caches.
- All shipped tracers name SingleSegment.
- All VolumetricFog objects in the levels of Primal Hunt (EXA1S1, EXH2S1,
  EXP1S1, EXP2S1, EXP2S2) have Use_Fog_Color off. Thus, they are white
  and fade in the same way with or without the fix.
- No script got to the tentacles (EXH2S1, EXP1S1), the carried turrets
  or the turret guns.

We checked these from the code. We ran the restriction lists and the
strings of the cache in a host test: the `MultiFile.cpp` and
`MPRestrictList.h` of the tree, built with g++ (with stub types for the
types of the engine), with and without `AVP2SRC_MODERN`.

Not fixed, from the entries of these files: "AvP2's player code casts its
attachments without testing the cast". The cast cannot fail. The entry
gives the reason.

### M-8: the launchers, and the game straight from AVP2.exe / PrimalHunt.exe

nolfsrc did the same for NOLF (`NOLF.exe` and `NOLFLauncher.exe`, the
objects of one launcher with two entry points). The AvP2 launchers are
MFC applications. Thus, here there is one object,
`src/modern/launcher/m8_entry.cpp`, compiled two times. It identifies the
program (`g_bM8Direct`).

| Id | What | Files | Status |
|---|---|---|---|
| M-8 | `AVP2.exe` and `PrimalHunt.exe` start the game directly, as Launch does, and wait for it (their exit code is the exit code of the game). `AVP2Launcher.exe` and `PrimalHuntLauncher.exe` are the launchers. The programs do not need the registry or a disc | `src/modern/launcher/` (`m8_entry.cpp`, `m8_launch.h`); `src/launcher/Launcher.cpp`, `LauncherDlg.cpp`, `LauncherDlg.h`, `DisplayMgr.cpp`; `build/build.mk` (`launcher_exe`, `launcher_modern`), `build/all.sh`; `build/game-scripts/linux-run.sh`, `windows-run.bat`, `windows-install.ps1`, `mac-install.command` | done |
| M-22 | After Launch, the launcher windows (`AVP2Launcher.exe`, `PrimalHuntLauncher.exe`) hide and wait for the game. They exit when the game exits. The command-line starts of the launcher (host, join, find, play, server) do the same. Before, the launcher exited when the engine started. Steam watches the program that it starts. When that program exits, Steam takes the game as ended, and the overlay and Remote Play stop (GitHub issue #5). While the launcher waits, it handles the messages of its window (`MsgWaitForMultipleObjects`). The launcher starts the dedicated server with `CreateProcess`, not `WinExec`, so that it has a process to wait for. `AVP2.exe` and `PrimalHunt.exe` already waited (M-8) | `src/launcher/LauncherDlg.cpp` (`LaunchGame`, `M22WaitForGame`) | `build/playtest-launcher.txt` with `AVP2Launcher.exe`. We listed the processes every 5 s. While the engine ran, the launcher process stayed. The same with `AVP2Launcher.exe -GOSERVER`: while `AVP2SERV.EXE` ran, the launcher process stayed | done |
| M-23 | `LockOnFlip` is 0 by default. Before each flip, `d3d_SwapBuffers` locked a 4x4 corner of the back buffer, so that the card finished the frame (on 2001 hardware). Under Wine and Proton, a lock of a render target copies the surface back to memory and stops the card on every frame (GitHub issue #2). The console variable `LockOnFlip 1` puts the lock back. This is a part of issue #2 only: the DirectX 7 renderer stays slow under Wine. The full fix is a Direct3D 9 renderer (M-4, M-5) | `src/engine/d3d/ph/d3d_surface.cpp` (`g_CV_LockOnFlip`) | Under Xvfb (software OpenGL), `build/playtest-avp2-marine.txt` with the Wine `+fps` trace, the mean of each 10 s: 136, 121, 97, 113, 96, 99, 118 fps with the lock; 167, 182, 120, 142, 126, 115, 162 fps without it | done |
| M-24 | `VSync` is a new console variable, 1 by default. In full screen, `VSync 1` is the retail flip (`DDFLIP_WAIT`): each flip waits for the vertical blank, so the frame rate cannot go above the refresh rate. `VSync 0` copies the full-screen frame with a `Blt` from the back buffer, as in a window, and no flip: the frame-rate limit (M-13) sets the rate. Under Proton the Steam overlay shows only with the retail flip. The player's tests in Steam: a flip with `DDFLIP_NOVSYNC` (692150a) and the `Blt` (5742a6b) both hid the overlay, and `+VSync 1` showed it. So `VSync 1` is the default. The cause is probably the DirectDraw path of Wine's wined3d, which Proton uses for DirectX 7: a Direct3D 9 renderer (M-4, M-5) runs through DXVK, where the overlay works at any frame rate. A window and a borderless window copy each frame with `Blt` and do not wait | `src/engine/d3d/ph/d3d_surface.cpp` (`g_CV_VSync`, `d3d_SwapBuffers`) | Under Xvfb, full screen (`WindowMode` 0), `VSync` 0 and 1: the game runs and draws. In Steam (the player): the overlay with `VSync 1` | done |
| M-27 | Before it makes a window, `lithtech.exe` declares the process DPI-aware: per-monitor (v2) (`SetProcessDpiAwarenessContext`) where Windows has it, then per-monitor (`SetProcessDpiAwareness`), then system-wide (`SetProcessDPIAware`). Each is looked up at run time, because the oldest is from Windows Vista. Retail declares nothing, so with display scaling above 100% Windows virtualises the game: the game showed in a box in a corner, and its cursor and its clicks missed the buttons (RETAIL_BUGS.md: "With display scaling above 100%, the game shows in a box in a corner"; GitHub issue #6). This is nolfsrc's M-P1-30. A Windows 11 test at 125% showed the fault again: the picture covered 2048x1152 of a 2560x1440 screen. Windows ignores the call if the process already has a window or a compatibility setting. Thus `lithtech.exe.manifest` also declares the process DPI-aware (`dpiAware` true/pm, `dpiAwareness` PerMonitorV2, PerMonitor). Windows reads the manifest when it starts the process, before any code runs. For each new display mode, the game writes one console line: "Display: mode ..., client ..., desktop ..., current ..., metrics ..., DPI awareness ..., DPI ...". With `+ErrorLog 1` the line also goes to `error.log`. A DPI awareness of 0 shows that Windows still scales the game. Nobody has confirmed yet that the manifest corrects the fault | `src/engine/lithtech/clientwin.cpp` (`M27_DeclareDpiAware`, `clu_WinMainQ501`), `src/modern/directmusic/lithtech.exe.manifest`, `src/modern/cshell/m9_window.h` (`M9Window_Report`) | Under Wine 9.0: the game reaches its menu, and Wine traces `SetProcessDPIAware` (the fallback that Wine has). With the manifest, both games reach their menus, the line shows DPI awareness 2 (per-monitor) and DPI 96, and Microsoft's DirectMusic DLLs still load. Not yet checked on a scaled Windows display | done |
| M-28 | The console parser (`parse_Token`) gives `iscntrl` an `unsigned char`. Retail gave it a signed `char`, thus a character above 127 was a negative index into the CRT's type table, and some of these characters ended the command (RETAIL_BUGS.md: "The console ends a command at some characters above 127"). The profile code binds each control with a console command that holds the DirectInput device name, which Windows translates. This is nolfsrc's M-P1-33, its console half only: AvP2 binds the mouse buttons by number (`##3`), not by their DirectInput names, thus the fixed button names of nolfsrc are not necessary | `src/engine/server/console.cpp` (`PARSE_ISCNTRL`, `parse_Token`) | The legacy build is unchanged (`make progress`). Under Wine, both games reach their menus. Not yet confirmed on a Windows whose device names have such characters | done |

**The direct start** goes through the code of the launcher, as its
`-GOGAME` does, but it does not show the dialog. `InitInstance` reads the
usual data and calls `LaunchGame(ACTION_PLAY)`. This function writes
`avp2cmds.txt` and starts `lithtech.exe -cmdfile avp2cmds.txt`. Thus, the
settings of the launcher apply to it where the registry holds them (its
Options, the saved commands). Its own arguments go last on the command
line of the game, as the arguments of the launcher do. It does not ask
for a detail level (the game keeps its own settings). It shows no dialog,
if nothing is missing.

**Without the registry** (the two programs):

- The program first makes its own directory the current directory.
- If there is no `InstallDir`, the program uses that directory as the
  game directory.
- If there is no launcher record, only the first run of the game
  (`NumRuns` in autoexec.cfg) shows the detail dialog, not each Play.
- The patch of AvP2 names the 1.0.9.6 archive in the registry ("Update
  Command Line"). When the registry does not name one, the program uses
  the archive in the directory (`-rez AVP2P1.REZ`).
- Primal Hunt plays on the AvP2 next to it, `..\AVP2` (the layout that the
  build installs), if `AVP2.REZ` is there. If not, it plays on the AvP2
  of the registry.
- The rez stack of Primal Hunt is the stack of `build/avp2cmds.sh`: the
  six archives of AvP2 from the AvP2 directory, then its own `MULTI`,
  `AVP2DLL`, `AVP2L`, `DIALOGUE`, `AVP2P`, `AVP2P1` and `AVP2X`, a
  registry `PatchRez`, and `custom`. This is the order of its dedicated
  server (M-26).
- The launcher still keeps its settings in the registry when the key is
  there. Nothing creates the key.

(The reconstructed retail code looks for `AVP2.rez` in the directory of
Primal Hunt and never names `AVP2X.REZ`. The retail launcher does
something different there, but that code is in the pages that its
protection wrapper keeps encrypted.)

**No disc check**: the Play of AvP2 does not call `CD_VerifyGame` now
(the Play of Primal Hunt never did). Also, the display code of the
launcher does not map `d3d.ren` to `d3d9.ren` now (a block that came in
with the `DisplayMgr.cpp` of nolfsrc). This build has no `d3d9.ren`, and
the mapping caused the launcher not to find the configured renderer.

**Checked** under Wine in the build container, with `build/playtest.sh`,
`PLAYTEST_LAUNCH` and a new Wine prefix (no registry values):

- `build/playtest-direct.txt` with `AVP2.exe +DisableMovies 1` and
  `PrimalHunt.exe +DisableMovies 1`. Each program writes the command line
  above (`... -rez custom -rez AVP2p.rez -rez AVP2p2.rez -rez AVP2P1.REZ ...
  +DisableMovies 1` for AvP2; `-rez "Z:\...\AVP2\AVP2.REZ" ... -rez
  DIALOGUE.REZ -rez custom ... +DisableMovies 1` for Primal Hunt). The game
  gets to its main menu (v1.0.9.6, v1.0.2.2F).
- `build/playtest-launcher.txt` with `AVP2Launcher.exe` and
  `PrimalHuntLauncher.exe`. The launcher opens. Display lists the renderer
  and the modes. Its close button closes it (M-B104). Play asks for the
  detail level (the first run of the game). Low starts the game
  (`++PerformanceConfig .DefaultLow`, M-B102; the same rez stack as the
  direct start). The game runs (AvP2 to its main menu, Primal Hunt
  through its first splash screen in the time that the script gives it).

The launchers and command lines of the release build do not change.
`make progress` keeps all the recorded functions of `AVP2.exe`,
`PrimalHunt.exe` and `Launch.dll` (184, 187 and 28 identical, as before).

### Retail-bug fixes: the launchers and Launch.dll (M-B100..M-B107)

Each bug is in `RETAIL_BUGS.md`. The Checked column gives how we checked
each fix.

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-B100 | Launch.dll copied an argument of 200 characters or more into its 200-byte slot without a terminator. Now, it cuts the argument to 199 characters and adds a terminator. (The `strncpy` calls at the call sites stay as they were, but they go to one bounded copy) | `src/launch/launch.cpp` | A Windows harness (`zig cc`, Wine) calls `GetLithTechCommandLine` as `lithtech.exe` does (150 slots of 200 bytes), with a 250-character argument. The retail slot has no terminator (it continues into the next slot). The slot of the modern build holds 199 characters and its terminator | done |
| M-B101 | Launch.dll (and the copy of the same code in the launchers): each rez-file check leaked its `_findfirst` handle. Now, it closes the handle | `src/launch/RezFind.cpp`, `src/launcher/RezFind.cpp` | The same harness, with the `FindFirstFileA`/`FindClose` imports of Launch.dll patched to counters, ten calls. Retail opened 20 handles and closed none. Modern opened 20 and closed 20 | done |
| M-B102 | The Low and High detail buttons of the launchers gave the opposite profile | `src/launcher/LauncherDlg.cpp` | `build/playtest-launcher.txt`: Low writes `++PerformanceConfig .DefaultLow` | done |
| M-B103 | The launchers wrote the command line as the format of `fprintf`. Now, they use `fputs` | `src/launcher/LauncherDlg.cpp` | From the code (there is no `%` in the command line of the test). The file that the playtests read back is the command line as built | done |
| M-B104 | The close button of the display dialog did nothing. Now, it cancels | `src/launcher/DisplayDlg.cpp` | `build/playtest-launcher.txt`: a click on it goes back to the launcher | done |
| M-B105 | When the caption of a launcher string did not load, the message was lost. Now, the error of the caption goes into the caption | `src/launcher/Launcher.cpp` | From the code. All the captions that the launchers give are in their string tables, so play does not get to the path | done |
| M-B106 | The already-running checks of the launchers matched all `LITHTECH.EXE` or `SETUP.EXE` processes. Now, they match only the processes of this directory, by full path | `src/launcher/Launcher.cpp` | Under Wine, with the notepad of Wine running as `LITHTECH.EXE` from a different directory. The `AVP2.exe` of the release build does not open (and it opens without the notepad). The modern `AVP2Launcher.exe` opens. `ExistProcess` compares a name with a `\` against the full module path of each process. Thus, the engine of this directory still stops it | done |
| M-B107 | The launchers copied the command line into 1024 bytes of stack (`strcpy`), at all lengths (new, `RETAIL_BUGS.md`). Now, they write it from the `CString` | `src/launcher/LauncherDlg.cpp` | From the code. The command files of the playtests are complete | done |

### M-17..M-20: Internet multiplayer

The retail games played on the Internet through two services. The two
services do not exist now:

- **WON** (of Sierra). It held the server list, the CD-key certificates,
  the messages of the day and the version check.
- **GameSpy**. Its master (`master<n>.gamespy.com`) received the
  heartbeats of the servers.

The retail code (the two games) works as follows:

* A server that is not LAN-only registers with the directory of WON
  (`InitSierraMultiplayer`, GameSpyMgr.lib: the addresses from
  SierraUp.cfg, then the auth servers, a certificate, and `AddServiceOp`
  every 90 s). It also sends GameSpy heartbeats to
  `master0.gamespy.com:27900` from its game port. Neither service decides
  anything: a server with no WON never refuses a player.
* The browser (GameSpyClientMgr.lib) reads the Internet list from **the
  directory of WON** (`/AliensVsPredator2`, Primal Hunt
  `/avp2primalhunt`), not from the master of GameSpy. Then it queries each
  server itself (the `\status\` of GameSpy, UDP).
  - The Multiplayer Internet folder first asks for a CD key (the `CDKey`
    of WON, checked locally; `VALIDCDKEY` passes).
  - Then it reads sierraup.cfg and the messages of the day (blocking,
    30 s), and it runs the version check. If the check fails, Internet
    play stays off (`SetLANOnly`).
  - Find Internet Games first gets a WON certificate
    (`SetupAuthentication`, blocking). If this fails, it refuses with the
    error.
  - Tested (the retail engine and game DLLs from `out/reference/`, in the
    container): without a key, the Host and Find links of the folder stay
    off. With `VALIDCDKEY`, the game crashes when it reads the missing
    sierraup.cfg (M-B50).
  - Thus, when WON does not answer, you cannot use the Internet half of
    the menus. The LAN half works.
* When a client joins a server, the engine starts the peer authentication
  of WON if the client shell gives it an auth context
  (`StartPeerAuthQ548`, packet 13). The server answers it (`Sub48f570`),
  and it marks the client in two fields that nothing reads. If the server
  has no auth context of its own (a LAN server, or no WON), it sends an
  empty answer. Nothing is enforced: a retail client with no certificate
  joins a server with no certificate (tested: LAN). The file check of the
  engine (below) is the item that controls a join.

The modern build, the two games: M-P6-02 and M-P6-13 of nolfsrc were the
model (the protocol notes, the `MasterServer` variable and its values,
the `\secure\` answer first, and the two-container test with a master
stub). We read the files of nolfsrc for it. The code of AvP2 is its own
code. Its GameSpy libraries are the real SDK. The libraries of nolfsrc
were replacements. Thus, here we keep the retail protocol code and point
it to a different server. The list comes from the master of GameSpy in
place of the directory of WON.

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-17 | The master server. `MasterServer` is a client console variable (`+MasterServer host[:port]`; empty: `master.openspy.net`; `none`: no Internet list and no reports). The browser asks it for the list (the `ServerListUpdate2` of the GameSpy SDK: TCP 28900, `\secure\` answered with the key of the game, `\list\cmp\`), with a 6 s limit on the connection and on each read. The server of a hosted game, in this process or in `AVP2Serv.exe`/`AVP2XServ.exe` started from it, reads it from the environment (`AVP2_MASTERSERVER`). A dedicated server that you start by hand reads it from `-MasterServer <host[:port]>`. The server resolves it one time. It sends a heartbeat to it every 90 s from the game port (the retail `\heartbeat\`, with `\gamename\`). It answers its `\secure\` first in a reply (the `\validate\` of OnSecureQuery, as in retail). It sends `\statechanged\2` when it stops. It prints `Master server: <name>` (or the reason that there is none) to its console | `src/lithshared/gamespy/gamespymgr/GameSpyMgr.cpp`, `gamespyclientmgr/GameSpyClientMgr.cpp`, `gamespyclientmgr/gserverlist.c`; `src/AVP2/ObjectDLL/MultiplayerMgr.cpp` (`Setup`); `src/AVP2/ClientShellDLL/FolderMulti.cpp`, `FolderJoin.cpp`; `src/AVP2/AVP2Serv/ServerDlg.cpp` (`ParseCommandLine`); `src/modern/cshell/m17_master.h` | Containers on one network, with a local master (`masterstub.py`, below). The dedicated servers of the two games send heartbeats to it. They answer `\secure\` with a `\validate\` that it accepts (the AvP2 key `Df3M6Z`, the Primal Hunt key `P4fR9w`), and they answer its `\status\` (name, map, players). The client of each game lists its server through Find Internet Games, and joins and plays. Against master.openspy.net, the master accepted and listed a modern AvP2 server (an independent list request, `gsprobe.py`, saw it). The stub did not get the `\statechanged\2` when the dedicated server stopped (the socket of the engine is closed at that time). The master removes a silent server itself | done |
| M-18 | No WON and no CD key. The Multiplayer Internet folder goes directly to its links (no key dialog, no directory servers, no messages of the day and no version check). It has no Update CD key link. Find Internet Games gets no certificate. The client shell gives no auth context to the engine, so a join starts no WON peer authentication. The servers do not register with WON now (thus they never read SierraUp.cfg: RETAIL_BUGS.md) | `GameSpyClientMgr.cpp` (`Init`), `GameSpyMgr.cpp` (`Init`, `UpdateHeartbeat`); `FolderMulti.cpp` (`Render`, `OnCommand`, `OnFocus`) | The Internet folder (shot), the joins above, the two games | done |
| M-19 | The other players move smoothly. Before, there were three problems. Each client sent its position 4 (standing), 8 (moving) or 12 (in the air) times a second (`ClientUpdateLowSendRate` 0.25, `ClientUpdateSendRate` 0.125, `ClientUpdateHighSendRate` 0.08 s). It asked for 6 world updates a second (the `UpdateRate` of the first run; the connection speeds of Player setup are 4, 6, 9, 13, 19). And the server limits the rate of a client to the rate that its bandwidth allows. The default of a new server (16000, the 56k of the host options) allows only the slowest rate, 4. Thus, each update moved another player by a step that prediction did not expect. Now: the three send rates are 0.03 s (a message only when something changed, so about 30 a second while the player moves). The `UpdateRate` of the first run is 30, and the game increases a lower value of an older config one time (`UpdateRate30`). The fastest choice of Player setup (T3) is 30 (the four other choices are the retail choices), and the game increases an older profile to it one time (`Connection30`). The bandwidth of a new server is the top choice of the host options (10000000). The game increases an older config saved with the retail default one time (`Bandwidth30`; other choices stay). That top choice allows the top rate (retail limited all servers to the fourth). The game keeps all the bandwidth choices of the host and all the speed choices of the player | `src/AVP2/Shared/PlayerMovement.cpp` (the send rates), `src/AVP2/ClientShellDLL/GameClientShell.cpp` (the `UpdateRate` of the first run, the increase), `ProfileMgr.cpp` (`kConnectSpeeds`, the increase of the profile, the limit from the server bandwidth), `src/AVP2/Shared/MultiplayerMgrDefs.h` (`DEFAULT_BANDWIDTH`), `ServerOptions.cpp` (the increase of the config); `src/modern/cshell/m19_netposlog.h` (the measurement aid, debug build only) | Measured with the modern debug build (`NetPosLog 1`: on each frame, the watching client records where it sees the other player). A dedicated server, one client that ran in circles for 47 s, and a different client that watched at about 70 frames a second. With the retail rates (send rates 0.25/0.125/0.08, the server at its old default bandwidth, so 4 updates a second), the frame steps of the runner were more than two times their median 7.9 times a second (0.6% of frames without movement). With the new defaults: 0.3 times a second (0.3%). The config of the watcher had the retail `UPDATERATE 6`. After the run, it was 30 | done |
| M-20 | The console of the dedicated servers. The game text (chat, names, the lines of the engine) and the typed command do not go as `printf` formats now. The server cuts a line to 497 characters and adds a terminator. It also writes each line to `server-console.log` in the game directory (with the time; the first line of each run empties the file) | `ServerDlg.cpp` (`WriteConsoleString`, `UpdateUI`, `OnConsoleSend`), `src/modern/server/m20_serverlog.h` | A player sent the chat `hello from the runner %n%s%n%s`. The retail AVP2Serv.exe crashed (page fault in its `_vsnprintf`). The modern server printed the text. Also the logs of all the tests above | done |

**Other problems that we found and fixed (not modern-only).** Each is a
reconstruction error, so we fixed it in the two builds:

* **Primal Hunt's GameSpy name and key.** `MultiplayerMgrDefs.h` gave the
  two games the name and key of AvP2 (`avp2`, `Df3M6Z`). The retail
  PHObject.lto and PHCShell.dll of Primal Hunt hold `avp2ph` and
  `P4fR9w`, and OpenSpy knows these. The servers and the browser of Primal
  Hunt now use them (`#elif defined(PRIMAL_HUNT)`).
* **Multiplayer prediction crashed our engines.** `predtick.cpp` is the
  prediction of other objects on the client for each frame. Only
  multiplayer runs it. It read CClientMgr and LTObject with the NOLF
  layout:
  - The time at 0x125c (AvP2 0x136c).
  - The interfaces at 0x4c0/0x61c (0x4c4/0x620).
  - The OnObjectMove/OnObjectRotate of the shell two slots too early.
  - All the prediction fields from the friction on, 4 bytes too low.

  All our clients crashed at their first frame in the world of a server
  (page fault at 0x004ab9dd, `sub_461270Q1S`). Now, the views use the AvP2
  offsets, as predobj.cpp already did, and as AvP2 1.0.9.6 itself
  (0x0046e250) reads them. The displacements of the function are now the
  set that retail has.
* **AvP2's dedicated server found none of our game DLLs.** AVP2Serv.exe
  looks for `custom.rez`, not for the `custom/` directory where a game
  directory keeps them. Thus, it stopped ("Unable to load the game
  binaries"). Now, `build/all.sh` writes `customrez.txt`
  (`Rez0 = "custom"`). AVP2Serv.exe reads this file for more archives.

**Closed later (M-26).** The tests above ran the dedicated servers on the
retail `server.dll`, because our `server.dll` did not load a world. Also,
a Primal Hunt server on `DM_Compound` refused a Primal Hunt client ("world
version is different"). M-26 fixes the two.

**Retail players.** The engine compares the CRC of `CShell.dll` and of
the attribute files on the server with the CRC on the client
(`SetCRCString`, `DoLoadWorld`). Our `CShell.dll` is not the retail file.
Thus, when a retail client joins a modern server, the server disconnects
it with "Some game resources differ from the server" (tested). A retail
server will also disconnect a modern client. Apart from this, the
handshake is the retail handshake. Thus, the two can play if the modern
build gives the retail CRC. That is a decision, and we did not make it
here (M-P6-04 of nolfsrc found the same). The master server is shared. A
modern server is in the list that a retail-patched browser reads, and the
opposite is also true, under the same game names.

**The test** (`out/mp/`, not kept in the tree):

- `dock.sh` runs the build image on a docker network.
- `srv.sh` runs a dedicated server under Xvfb and Wine from a game
  directory (a config in ServerData/, `-config`). For the run, it adds
  the install key of Primal Hunt for the AvP2 archives to the Wine
  prefix. It removes the key after the run.
- `masterstub.py` is the master (UDP 27900, TCP 28900). It records each
  heartbeat, challenge, validate, status reply and list.
- The clients are `build/playtest.sh` with scripts (Multiplayer LAN or
  Internet, Find, the first server, Join, Space, chat, run), and
  `PLAYTEST_CFG` for `MasterServer`.
- `gsprobe.py` sends one list request to a master.

### M-25: the console

Retail AvP2 and Primal Hunt have no console that a player can open: the
~ key does nothing. The engine has a console, but the client shells never
show it. nolfsrc gave NOLF a console (its M-P2-01..M-P2-15, after the
NOLF Modernizer). We took its design for the two games. The code is our
code, for the interface code of AvP2 (`CInterfaceMgr`, the folders, the
HUD space of M-16). We did not take the big heads (M-P2-10), the trigger
boxes (M-P2-11) or the speed-run addresses (M-P2-12) of nolfsrc.

The console is in single player only. It does not open in multiplayer,
during a loading screen, in Customize Controls (a key can wait for a
control there), when a control of a folder has the input (an edit box),
in a dialogue, or while the chat line or a message box is open.

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-25 | The console (nolfsrc M-P2-01). The ToggleConsole control or Shift+Esc opens it over the top half of the screen (nolfsrc M-P2-15: a keyboard can have a different key to the left of 1). Esc or the control closes it. It shows the console output of the engine and of the game, an edit line with a cursor, the commands typed before (Up, Down) and the lines before (Page Up, Page Down, Home, End; 512 lines). Opened in play, it pauses the game (the server, the sounds and the input, as the pause key does) and shows a copy of the last frame, as the pause screen does: no frame of the world runs, thus the player does not move. Opened in a menu, the menu stays below it. While it is open, it gets each key and each character, and the game gets no command. It is drawn in the HUD space (M-16), thus its text is as large as the HUD text | `src/modern/cshell/m25_console.h`; `GameClientShell.cpp` (`OnEngineInitialized`, `OnEngineTerm`, `OnKeyDown`, `OnKeyUp`, `OnCommandOn`), `InterfaceMgr.cpp` (`OnChar`, `Update`, `PostUpdate`), `InterfaceMgr.h` | Under Xvfb and Wine, in the first Marine mission (AvP2) and the first Corporate mission (Primal Hunt): `build/playtest-avp2-console.txt`, `build/playtest-ph-console.txt`. The ~ key opened the console with the engine output in it. `GameTime` two times, 3 s apart, gave the same time (2.984152): the game was paused. Page Up showed the earlier lines and the note "13 more lines (Page Down)". Up recalled the last command. The ~ key closed it, and the game continued. Shift+Esc opened it, Esc closed it (not the pause menu), and the game time had moved on | done |
| M-25 (Enter) | Enter runs the line (nolfsrc M-P2-02). A cheat code (CheatMgr.cpp) runs as a cheat, with the cheat sound. Else the client runs the line as a console command. If the first word is not a command of the client (thus a variable, or a command of the server), the client also sends the line to the server (`MID_M25_CONSOLE_COMMAND`, 250), so that server variables can be set. (nolfsrc sent each line. The server then made a variable of each client command.) | `m25_console.h` (`M25Submit`); `src/AVP2/Shared/MsgIDs.h` | The same runs: `mpcanthurtme` printed "God Mode: ON"; `GetPos` and `Help` ran on the client | done |
| M-25 (control) | A new command, `COMMAND_ID_TOGGLE_CONSOLE` (79), the action `ToggleConsole`, the last control of the shared list of Customize Controls ("Console"; nolfsrc M-P2-03). The client adds the action (`AddAction ToggleConsole 79`) before the profile binds its key: AvP2 has its other actions in autoexec.cfg, Primal Hunt adds each action of `g_CommandArray` itself. A profile keeps the bindings of each command, by the action name. The retail `Default.prf` (which "Restore defaults" reads too) and the profiles from before do not have `ToggleConsole`. A profile that has never had the console key gets the key to the left of 1 (DirectInput `##41`), if the control has no key and no other control uses `##41` (no retail default does). `ConsoleKey` 1 in the `[Basic]` section of the profile records this; after that, the choice of the player stays. The console finds the virtual key of the binding from its `##<DIK>` name. A key with no virtual key that it knows still opens the console, through the command | `src/AVP2/Shared/CommandIDs.h`; `ClientUtilities.cpp` (`g_CommandArray`); `src/modern/cshell/m25_consolekey.h`, `ProfileMgr.cpp` (`LoadBindings`, `Save`); `ClientRes.rc`, `ClientRes.h` (32000, 32001) | `build/playtest-avp2-console-rebind.txt`: Customize Controls lists "Console" last in Shared, with the key to the left of 1 for a new profile. The ~ key there did not open the console. The control took the O key. In play, ~ did not open the console after that, and O opened and closed it. The ~ runs above use the default key of a new profile | done |
| M-25 (programs) | Console programs (nolfsrc M-P2-04, -09, -14). `Help` lists the programs of the game and the commands of the engine, sorted (`ILTClient::EnumConsoleCommands`). `Cheats` lists the cheat codes of CheatMgr.cpp (its table keeps them encoded) and what each code does (our descriptions, from the code). `GetPos` prints the position of the player as `SetPos x y z`. `SetPos x y z` (single player) sends `MID_M25_SETPOS` (251): the server moves the player object (`CPlayerObj::Teleport`, as the teleport cheat), then sends the client its new position, so the player does not cross the space between the two positions | `m25_console.h` | The runs above: `Help` (the game programs, 38 engine commands), `Cheats` (both games; Primal Hunt adds its two codes), `GetPos`. `build/playtest-avp2-console-setpos.txt`: `GetPos` gave 320.0 42.9 -1538.0, `SetPos 320 42.9 -1420` moved the player forward, and `GetPos` then gave 317.2 43.8 -1428.5 (the player was put down on the floor); the view after the close was from the new position | done |
| M-25 (backdrop) | `ConsoleBackdrop` (nolfsrc M-P2-05): 0 (the default) `console.pcx` of the game directory (a facehugger: the art of the retail console of the engine; both games have it), 1 black. If `console.pcx` does not load, the backdrop is black | `m25_console.h` (`M25Backdrop`) | The runs above: the facehugger of `console.pcx` behind the text. `CreateSurfaceFromBitmap` does not find the file (it is not in a .rez file), thus the console reads the PCX itself (`M25LoadPcx`, as the engine opens it with `fopen`). It draws the backdrop before the optimized 2D mode, where black is transparent | done |
| M-25 (server) | The server half (nolfsrc M-P2-08). The server runs `MID_M25_CONSOLE_COMMAND` and `MID_M25_SETPOS` ONLY from the local client (`CIF_LOCAL`: the player in single player, or the player who hosts a listen server), and `SetPos` only in single player. It ignores the message from each other client and writes that to its console. (The Modernizer ran its console message from each client: a security fault. M-B86 makes the retail `MID_CONSOLE_COMMAND` and `MID_CONSOLE_TRIGGER` local too) | `src/modern/object/m25_server.h`; `ObjectDLL/GameServerShell.cpp` (`OnMessage`) | Code | done |
| M-25 (engine) | The engine gives each console line to the game (`ILTClient::SetConsoleOutputFn`) and lists its commands (`EnumConsoleCommands`). The bodies were in `clientconsole.cpp` (from nolfsrc, M-P0-02), but the ILTClient table of the AvP2 engines did not have them. Now `iltclient.h` declares the three modern methods after `EndQuery`, the last retail slot, and the `%ifdef AVP2SRC_MODERN` block of `link/vtables-avp2.txt` and `vtables-ph.txt` puts them in the table. `build.mk` gives `--define AVP2SRC_MODERN` to `mkvtabobj.py` in the modern build only: the release `vtables.obj` does not change | `src/LT2/sdk/inc/iltclient.h`; `src/engine/lithtech/link/vtables-avp2.txt`, `vtables-ph.txt`; `build/build.mk` | The modern link report of `lithtech.exe` (both games) has no unresolved `CLTClientModern` symbol, and `vtables.obj` names the three. The console output and `Help` in the runs above come through these slots. `make progress` passes (the release build) | done |

### M-26: dedicated servers on Linux, on our own server.dll

nolfsrc has `linux-server.sh`, a script that starts the dedicated server
of NOLF under Wine (its docs/INSTALL.md, "A dedicated server"). M-26 gives
the same to AvP2 and Primal Hunt. Before, the dedicated servers did not
run on our `server.dll`. Thus, the multiplayer tests used the retail
`server.dll`, and the public archive cannot hold that file. M-26 also
fixes this. Except for the scripts, each fix is a reconstruction error.
Thus, we fixed it in the two builds.

**How the servers take their settings.** `AVP2Serv.exe` (AvP2) and
`AVP2XServ.exe` (Primal Hunt) read one configuration,
`ServerData\<name>.txt`, in the game directory (`ServerDlg.cpp`, `DoHost`,
`FixupConfigPath`). `-config <name>` names it. Without `-config`, the
server shows its dialog: a list of the files in `ServerData\`. The dialog
starts the only file at once. With no file, it stops ("A configuration
file must be setup inside the game menus"). The Host menu of the game
writes these files (`ServerOptionMgr.cpp`). `-port <n>` and `-name <text>`
override the port and the name of the file. The modern build adds
`-MasterServer` (M-17). Primal Hunt's server reads the archives of AvP2
from the `InstallDir` of the AvP2 key in the registry (`AddResources`).
Primal Hunt ships one file, `ServerData\default.txt` (port 27890, the four
deathmatch worlds of Primal Hunt, and the classes and the weapons that its
server allows). AvP2 ships no file.

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-26 (scripts) | `linux-server-avp2.sh` and `linux-server-ph.sh`, from one template, as the `linux-run-*.sh` scripts. Each starts the server of its game under Wine in its game directory. `--config <name>` (default `server`) starts it with `ServerData/<name>.txt`. When this file does not exist, the script writes it: deathmatch, 8 players, port 27888, the deathmatch worlds of the game (Primal Hunt: its `default.txt`, with new settings), and a random server-control password, which the script shows (the retail default is `AVP2` or `AVP2PH`, and a player who types it controls the server). The modern build writes an Internet server (`LANConnection = 0`). The legacy build writes a LAN-only server: a legacy server that is not LAN only crashes when the world starts (RETAIL_BUGS.md, "Hosting an Internet game without SierraUp.cfg crashes the server"). The script also warns when a legacy configuration has `LANConnection = 0`. `--wizard` shows the dialog of the server (the dialog needs a display). `--headless` runs the server under `xvfb-run` (a 24-bit virtual screen), in a Wine prefix of its own (`~/.wine-avp2-server-AVP2` or `~/.wine-avp2-server-PrimalHunt`). When no server of the game runs, a headless start first stops a Wine server that a server stopped by force left on its prefix: the next start there crashed ("Unhandled page fault"), also when it was the other game's server. This is the default when `$DISPLAY` is not set. `--help` shows the header. The script gives all other arguments to the server. It writes the registry values of `linux-run-*.sh` into its prefix, so that Primal Hunt's server finds the archives of AvP2. `build/all.sh` writes the two scripts (`serverscript`). Thus, the two archives hold them (`build/package.sh` packs the whole directory) | `build/game-scripts/linux-server.sh`, `build/all.sh` | In the build container, without `$DISPLAY`, on a copy of the game directory: `linux-server-avp2.sh -name ls-le` and `linux-server-ph.sh -name ls-lf` (the legacy build) wrote `server.txt` and started headless. A client of ours (`build/playtest.sh`, `c-runner.txt` without the `%n` chat) in a second container found each server in Find LAN Games, joined, spawned, sent a chat line and ran (Primal Hunt: `DM_Aizin`, from `default.txt`; AvP2: `DM_Compound`). The modern build, with `-MasterServer none`: the two servers (an Internet configuration, `LANConnection = 0`) were also on the LAN list. The client joined each one, and `server-console.log` shows "New client" and the chat line. Without `$DISPLAY` and without `--config`, the legacy server with `LANConnection = 0` crashed with the retail page fault on a read of 0x34 (thus the LAN-only default of the legacy build) | done |
| M-26 (server.dll) | Our `server.dll` runs a dedicated server. These were the faults, in both games unless the text says one: (1) `TCPDriver`'s vtable (`link/vtables-avp2.txt`, `vtables-ph.txt`) had `_purecall` in slot 3, `Update`. `CNetMgr::Update` calls it each frame, so the server stopped with "pure virtual function call" before the world. The shipped slot (AvP2 0x10042138, Primal Hunt 0x1004076f) is the one bare `ret` of the image, which `/OPT:ICF` folds all empty functions onto: now `_dsi_OnReturnError` (dsi.cpp, also a bare `ret`). (2) AvP2: `link/aliases-avp2.rsp` bound `CWDArray04::SetSize`, the resize of the light animations of a world (`CWorldData::LoadObjects`), to the resize of `CMoArray<CCollElem04F>`. Its bytes are the same with the relocations masked, but its array new has a stride of 4 bytes. The shipped resize (0x1000f43d) calls the array new of the 0x60-byte record (0x100106ed). Thus the animations overran their array, `LoadObjects` failed, and the server showed "Unable to load the selected map". Now the binding is `CMoArray<CWorldAnimRecQ>`'s resize, as nolfsrc's and Primal Hunt's. (3) The references that the link left unresolved: the 0x60-byte record's constructor `CWorldAnimRecQ::CWorldAnimRecQ` (both; a call to address 0 in each array new of the light animations) is bound to `CCollElem60::CCollElem60` (collection.cpp), whose bytes are the shipped constructor's (AvP2 0x1000a9d1, Primal Hunt 0x1000abcf). `CWorldTable::CWorldTable` (Primal Hunt; a call to 0 in the constructor of each world) is bound to the template's constructor, as in AvP2. `sm_Sub34688` (AvP2: the save code's object check) is bound to `sm_CheckObjectIntegrityQ474`, which is byte-identical to its shipped target (0x1003ec10), as in Primal Hunt. (4) `mkaliasobj.py` keeps the first binding of a name. Eleven bank slots of each alias file (`AllocVoid` and `FreeVoid` of the file, hash, link-node, thread-message, transfer and packet banks) had an early binding before the correct one of the "k2" section. In AvP2, four `AllocVoid` slots went to a `FreeVoid` (`ret 4`). The early lines are gone. How we found them: a copy of `server.dll` with `_purecall` patched to fault gave the caller (`CNetMgr::Update`). Then `winedbg` breakpoints on the error hook (`dsi_OnReturnError`) and on the returns of `CServerMgr::LoadWorld` gave `CWorldData::LoadObjects` | `src/engine/server/link/vtables-avp2.txt`, `vtables-ph.txt`, `aliases-avp2.rsp`, `aliases-ph.rsp` | The two links have no unresolved reference now. Legacy build, `build/mptest/srv.sh` without `SRV_DLL`: the AvP2 and Primal Hunt servers loaded `DM_Compound` ("Loaded world", "Running world"). A client of ours in a second container joined each server, spawned, sent a chat line and ran (`build/playtest.sh`, `c-runner.txt` without the `%n` chat line: the legacy `AVP2Serv.exe` crashes on that line, M-20). `make progress`: every recorded function is still identical; the counts of `server.dll` and `AvP2 server.dll` did not change | done |
| M-26 (DM_Compound) | A Primal Hunt client could not join a Primal Hunt server on `DM_Compound` ("world version is different"). `MULTI.REZ` and `AVP2P1.REZ` of Primal Hunt hold two versions of `DM_Compound` and `Over_Compound`. The server (`AVP2XServ.exe`, `AddResources`) loads Primal Hunt's `MULTI.REZ`, `AVP2DLL.REZ`, `AVP2L.REZ` and `DIALOGUE.REZ` before `AVP2P.REZ`, `AVP2P1.REZ` and `AVP2X.REZ`. The client loaded `AVP2P.REZ`, `AVP2P1.REZ` and `AVP2X.REZ` first, thus it had the other version. Now `build/avp2cmds.sh` and the modern launcher (M-8) use the order of the server. The retail launcher's own code is encrypted; the order of its string pool (`multi.rez`, `dll.rez`, `l.rez`, `dialogue.rez`, `PatchRez`, `custom`) agrees | `build/avp2cmds.sh`; `src/launcher/LauncherDlg.cpp` (`s_aM8Rez`, modern only) | Legacy build: a Primal Hunt server of ours on `DM_Compound` and a Primal Hunt client of ours: the client joined, spawned and ran | done |

### M-29: the Direct3D 9 renderer

`d3d9.ren` is a render DLL on the fixed-function pipeline of Direct3D 9
(`src/modern/render/`, its README.md). It is nolfsrc's renderer (its
M-P0-04, M-P0-05, M-P0-06, M-P1-18), changed to the contract and the data
of AvP2 and Primal Hunt. The reference for each change is our `d3d.ren`
source (`src/engine/d3d/ph/`). `build/ext.sh` builds it with `zig c++`, and
the modern builds install it beside `d3d.ren` (`MANIFEST.txt` origin
`EXT`; the modern-debug build installs the copy with the debug checks).
`d3d.ren` stays the default renderer. `+RenderDll d3d9.ren +CardDesc
Direct3D9` selects `d3d9.ren`.

| Id | What | Files | Status |
|---|---|---|---|
| M-29 (stage 1) | The device and 2D. The contract of AvP2 (`ltrender.h`): the renderer's 38 slots from +0x70, `RebindWorld`, `Clear` with a colour, `GetTexture` with its second argument, a `SharedTexture` of 0x40 bytes. A windowed Direct3D 9 device on the window of the engine; for full screen, the renderer first changes the display mode. The mode list has every mode of the display at 16 and 32 bits, from 640x480. Clear and present; the surfaces of the engine and every blit; Optimized 2D with its blend modes; `FillScreenRect`; `BlitFromScreen`; screenshots (F8); DTX textures (the mipmaps as `d3d.ren` selects them). The screen lock keeps the screen in a shadow buffer: it reads the back buffer only when the device drew after the last lock, and it writes it back before the next draw. (The text of the engine onto the screen locks the screen for each string. The console locks it hundreds of times in one frame, and a read of the back buffer for each lock stopped the console.) The movies play through DirectDraw stand-ins. `VSync` (0 here) and `Antialiasing`. The engine's fast `FillRect` (nolfsrc's M-P0-03) called the slot +0xb8, which is `RenderScene` in AvP2; it now calls `GetRenderObject` at +0xc0 | `src/modern/render/`; `build/ext.sh`, `build/all.sh`, `build/retail.py`, `build/retail_avp2.py`; `src/engine/lithtech/warpblit.cpp` (`ci_ModernFillScreen`) | Under Xvfb and Wine 9.0: both games to the menu, the menus, the console, F8, the intro movies; full screen, a window and a borderless window at 1280x720; 2560x1440 and 3840x2160 (borderless and full screen); `VSync 1` with 4x antialiasing. The 3D parts of the menus (their backgrounds) are not drawn yet (stage 3). Not yet confirmed on Windows or under Proton |
| M-29 (stage 2) | The world, as `d3d.ren` draws it with its default two passes: lightmaps (all of them light animations, composed into atlases), the sky-pan texture, vertex-lit polys through the world colour's ramps, the vertex light animations, dynamic lights (into the lightmaps, and as small added lightmaps, `LMDynamic`), detail textures and environment maps, world models, the sky, the light scale and add passes, and the mirrors (stencil). The fog is `d3d.ren`'s for each vertex: by distance, the vertical fog of AvP2 with its retail cut, the fog volumes of Primal Hunt. The renderer draws such vertices pre-transformed, because Direct3D 9 takes the specular alpha as the fog only for those (under Wine, the untransformed path gave fog everywhere). The sky polys are lit at every vertex drawn (RETAIL_BUGS.md, "With FixTJunc, the dynamic lights do not light all the vertices of a sky polygon") | `src/modern/render/` (`ltworld.h`, `mr_scene.cpp`, `mr_lightmap.cpp`, `mr_objects.cpp`), `d3d9/d3d9_device.cpp` (explicit fog, stencil) | Under Xvfb and Wine: the first Marine and Predator levels of AvP2 and the first Corporate level of Primal Hunt match `d3d.ren` in series of screenshots (a shot each second, the frames of the same moment compared: the cutscenes and the light animations do not run at the same times in two runs). The mirrors are not yet checked: AvP2's `m3s4` and `A2s3` and Primal Hunt's `EXH3S0`, `EXH3S1`, `EXH1S1` and `EXA2S1` have them, but the test harness cannot yet get a player to one. Not yet confirmed on Windows or under Proton |
| M-29 (stage 3) | The objects, as `d3d.ren` draws them. Models: the node matrices of the engine, skinned and blended between LODs on the CPU; each vertex lit in model space from its bind pose (the light grid times the world colour, the sun with its sky test, up to `MaxModelLights` point and spot lights, each `(r^2 - d^2) N.L` times its colour), after the model hook of the game (the vision modes) changes the colour, the add and the flags; the distance sprite; the environment map pass, the specular pass and the detail texture (Primal Hunt: the map of the instance, the cloak, the turning offset). The really-close view (the weapon of the player): its own field of view, at 4:3 then made wider as `d3d.ren` does (M-1). Sprites, particle systems, poly grids, line systems, canvases. The planar and the projected shadows of models (the image of the model made on the CPU). The sun test walks the BSP of the vis query (the first `WorldData` is not the main world). A retail bug fixed: RETAIL_BUGS.md, "A model with no lower LODs gets no projected shadow" | `src/modern/render/` (`mr_models.cpp`, `mr_sprites.cpp`, `mr_effects.cpp`, `mr_shadows.cpp`, `mr_objects.cpp`), `d3d9/d3d9_device.cpp` (two combiner operations) | Under Xvfb and Wine: the menus, the loading screens, the intro of the first Marine level, the first minutes of the first Marine and Predator levels at 640x480 and at 1280x720 match `d3d.ren` in series of screenshots. The Predator's vision modes tint the models and the screen. One difference stays: a vision mode turns the sky off (`DrawSky` 0), and the game clears only the depth on each frame. With `d3d.ren` the area of the sky shows an old frame with the screen passes built up on it (RETAIL_BUGS.md, "A vision mode shows an old frame where the sky must be"); `d3d9.ren` shows the area dark. The user will review this look in the game later. The shadows, the poly grids, the cloak of Primal Hunt and the mirrors are not yet checked in play. Not yet confirmed on Windows or under Proton |
| M-29 (stage 4) | M-4 and M-5 in the menus, as nolfsrc has them (its M-P1-09, M-P1-18). Options, Graphics, Custom Performance, General: "Texture filtering" (None, Bilinear, Trilinear, Aniso 2x, 4x, 8x, 16x) takes the place of the Trilinear toggle and writes `Bilinear`, `Trilinear` and `Anisotropic`, only when the choice changes. "Antialiasing" (Off, 2x, 4x, 8x) writes `Antialiasing`. When you leave the Graphics folder, a new texture filtering rebinds the textures, and a new antialiasing starts the renderer again (as a change of triple buffering does). `d3d.ren` reads `Anisotropic` too, but it has no antialiasing | `src/AVP2/ClientShellDLL/FolderPerformance.cpp`, `FolderPerformance.h`, `FolderDisplay.cpp`; `src/AVP2/ClientRes/ClientRes.rc`, `ClientRes.h` | Under Xvfb and Wine with `d3d9.ren`: the two choices, the values in autoexec.cfg (`Anisotropic` 16, `Antialiasing` 8), the renderer started again with 4x antialiasing (the most that the test device has). make progress: ALL 50555 identical, 905 near, both PASS lines. Not yet confirmed on Windows or under Proton |

## New console variables

| Variable | Type | Default | What | Id |
|---|---|---|---|---|
| `WindowMode` | int | (none: a first run, then 2) | With `Windowed` 1: 2 gives a borderless window at the resolution of the desktop. Other values give a window with a title bar. Options, Graphics, "Window mode". Saved in autoexec.cfg. A config without it starts borderless, if `Windowed` is not 1 | M-9 |
| `Windowed` | int | (engine) | The existing variable of the engine (the two renderers read it when they start). Now, "Window mode" also sets it: 0 is full screen, 1 is a window (of the two kinds). The game sets the mode again when it changes | M-9 |
| `MenuScale` | int | 100 | The page of the menus, as a percent of the height of the screen that it fills (0: the retail size; never smaller than the retail size, never wider than the screen). Options, Graphics, "Menu size". Saved in autoexec.cfg | M-15 |
| `HUDScale` | int | 75 | The scale of the HUD, as a percent of the height of the screen over 480 (0: the retail size; never smaller than the retail size). Options, Graphics, "HUD size". Saved in autoexec.cfg | M-16 |
| `MasterServer` | string | (empty = `master.openspy.net`) | The GameSpy master server. The game reads the Internet list from it, and a hosted Internet game reports to it (host or host:port; `none`: neither). `+MasterServer`, saved in autoexec.cfg. A dedicated server uses `-MasterServer` | M-17 |
| `UpdateRate30`, `Connection30` (profile), `Bandwidth30` (server config) | flags | -- | These show that the one-time increases of M-19 are done | M-19 |
| `NetPosLog` | int | 0 | (Modern debug build only.) 1: on each multiplayer frame, the game writes the positions of the other characters to netpos.log | M-19 |
| `FieldOfView` | int | 90 | The horizontal field of view of the player at 4:3, in degrees (60..130). The field of view of each character changes with it. A wide screen makes it wider again. Options, Graphics, "Field of view" (75..120). Saved in autoexec.cfg | M-3 |
| `RestrictCinematicsTo4x3` | int | 0 | 1: black next to the centred 4:3 area while a cinematic plays. Options, Graphics, "Cinematics" (Widescreen, 4:3). Saved in autoexec.cfg | M-2 |
| `FramerateLimit` | int | 240 | The maximum frames a second of the client (0: no limit), in single player and multiplayer, but not while a world loads. Options, Graphics, "Frame rate limit" (60, 120, 144, 240, None). Saved in autoexec.cfg | M-13 |
| `PadLookSpeed` | float | 1 | The right stick of the controller: the turn rate, times 180 degrees a second across and 120 up and down. Controller, "Look speed" | M-7 |
| `PadInvertY` | int | 0 | Up on the right stick looks down. Controller, "Invert look" | M-7 |
| `PadLookCurve` | float | 2 | The response of the right stick: the game raises its deflection past the dead zone to this value (1: linear) | M-7 |
| `PadLookDeadZone` | float | 0.15 | The radial dead zone of the right stick (0..0.9 of its travel). Controller | M-7 |
| `PadMoveDeadZone` | float | 0.2 | The radial dead zone of the left stick. Controller | M-7 |
| `Antialiasing` | int | 0 | `d3d9.ren`: samples for each pixel (0 off, 2, 4, 8), read when the renderer starts | M-29 |
| `ConsoleBackdrop` | int | 0 | The backdrop of the console: 0 `console.pcx` (the art of the retail console of the engine), 1 black | M-25 |
| `OldMouseLook` | int | 0 | The retail mouse look (the DirectInput offsets of the engine, `CursorCenter 1`) in place of raw input. Controls, Mouse, "Legacy mouse look". Saved in autoexec.cfg | M-6 |
| `NoRawInput` | int | 0 | The new mouse look from the motion of the cursor (the game puts the cursor back in the middle of the window on each frame), in place of raw input. Saved in autoexec.cfg | M-6 |
