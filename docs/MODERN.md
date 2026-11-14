# The modern build

`AVP2SRC_MODERN` is the project's define for everything that departs from the
retail game: bug fixes (`RETAIL_BUGS.md`), new features, and open-source
middleware in place of the proprietary libraries. It is set only in the
modern build (`make modern`, `make modern-debug`; `build/variant.sh`),
never in the legacy build, which stays the retail game as far as it can be
rebuilt and is the only build `make progress` scores. Every change goes
behind `#ifdef AVP2SRC_MODERN` with the retail code kept, unchanged, in the
`#else`. (`AVP2SRC_DEBUG` is the debug builds' logging and checks, independent
of it.)

This file is the inventory: what the modern build changes, what it could
change next, and what was looked at and left alone, and why. It follows
nolfsrc's modern build (`NOLFSRC_MODERN`) in its rules, and most of its
items will come from there, since AvP2's engine is NOLF's generation; the
items are AvP2's own.

## Rules for this work

* **Reimplement, never copy.** Fan fixes and patches are a fine source for
  *what* is wrong and *why*; their code is not copied here. The same goes
  for nolfsrc's NOLF fixes: the idea often carries over, since the engine is
  the same generation, but each change is made and checked here.
* **The original compilers.** The modern build uses the legacy build's
  toolchain (the VC6 revisions of `progress/COMPILERS.md`). The one
  exception is the open middleware (below), which is not the games' code:
  `build/ext.sh` builds it with Zig, as nolfsrc's, nolf2src's and
  cjacksrc's do -- OpenMiles is written in Zig, FFmpeg is C99 that VC6
  cannot compile, and `zig cc` (clang with its own mingw-w64) builds both
  and our Bink player for 32-bit Windows in the build image.
* **Open middleware.** The modern build has no RAD binaries: OpenMiles
  (`ext/openmiles`) stands in for Miles, our player on FFmpeg
  (`src/modern/binkw32/`, `ext/ffmpeg`) for Bink, and nothing for Smacker,
  which neither game plays. Its movies play (no `+DisableMovies`).
* **No disc check.** The modern build drops the launcher's protection
  wrappers and the games' own disc and CD-key checks.

## Where it stands

**Begun.** The define exists and the modern builds are made
(`make modern` builds `out/avp2-modern-release/`, `make modern-debug`
`out/avp2-modern-debug/`, both games each). Done: the frame timing
(M-10..M-14), the open middleware (M-X1..M-X3, with OpenMiles' fixes
M-X5 and M-X6), so a modern game directory holds no RAD or Miles binary,
the music through Microsoft's DirectMusic (M-X4), raw mouse input (M-6),
controllers through XInput (M-7), the launchers and the games started
directly (M-8), full screen, window and borderless window modes (M-9), widescreen, 4:3 cinematics and the field of view
(M-1..M-3), the menus and the HUD scaled to the screen (M-15, M-16),
Internet multiplayer through a GameSpy master server such as OpenSpy's,
without WON or a CD key (M-17..M-20), and every
retail bug docs/RETAIL_BUGS.md lists either fixed (M-B01..M-B108) or shown
not to need it (under Done); the rest of the items below are not.

A variant build makes its own engines (`lithtech.exe`, `server.dll`,
`d3d.ren`, both games'): `build/all.sh` builds `engine-link`, `d3d-link`
and `avp2-d3d-link` in a `build/variant.sh` tree. Before M-10 it did not,
and the modern build shipped the release build's engines, without any of
the engine's modern blocks. Compiling them showed one that needs a file
of nolfsrc's: `clientinput.cpp`'s M-P7-01 and M-P7-09 blocks call
nolfsrc's `##gamepad` (XInput). `src/modern/engine/p7_gamepad.h` is now
AvP2's own `##gamepad` (M-7, under Done), so those blocks are kept: the
modern engine leaves DirectInput's joysticks out, reads the pad through
XInput, and creates only the keyboard and mouse besides (M-P7-09's crash
fix).

Code of the modern build's own that is more than a few lines lives in
`src/modern/` (`engine/` for the engine's, `cshell/` for the client game
DLLs'), as headers the retail files include inside their
`AVP2SRC_MODERN` blocks: the game projects' file lists are the retail
ones, and stay so.

Besides the items done, what the modern build compiles differently is
what came in with nolfsrc's code: the `AVP2SRC_MODERN` blocks of its engine
(`src/engine/`), launcher display code (`src/launcher/DisplayMgr.cpp`) and
libraries (`src/lithshared/`: ButeMgr, GameSpy, LithFontMgr, LTGUIMgr),
renamed from `NOLFSRC_MODERN` and still citing nolfsrc's `MODERN.md` ids
(M-P6-02, ...). They are NOLF's fixes, not yet checked against AvP2, and
some assume nolfsrc's modern files (the engine and the launcher named
`d3d9.ren`, nolfsrc's Direct3D 9 renderer, which is not built here; both
dropped, M-8 and M-1): each
is to be checked, kept or dropped, as the items land.

Each item's id is a series and a number (M-1, ..., and M-X for the
middleware, as nolf2src's); an id is never reused.

## Planned

To bring in once the games run (each checked against AvP2's and Primal
Hunt's code first; `docs/PLAN.md`, package 12). Every one: not implemented
yet.

| Id | What | Status |
|---|---|---|
| M-1 | Widescreen: menus, HUD and field of view at any aspect ratio | done (under Done) |
| M-2 | An option to keep the cinematics at 4:3 (pillarboxed) on a wide screen | done (under Done) |
| M-3 | A field-of-view setting | done (under Done) |
| M-4 | Up to 16x anisotropic filtering, by way of nolfsrc's modern Direct3D 9 renderer | not implemented yet |
| M-5 | Up to 8x antialiasing (the same renderer) | not implemented yet |
| M-6 | Raw mouse input (WM_INPUT), in place of DirectInput's mouse | done (under Done) |
| M-7 | Controllers through XInput, in place of DirectInput joysticks | done (under Done) |
| M-8 | The launchers as `AVP2Launcher.exe` and `PrimalHuntLauncher.exe`, with `AVP2.exe` and `PrimalHunt.exe` starting the game directly, as if Launch had been clicked in the launcher (no registry, no disc check) | done (under Done) |
| M-9 | Window modes: full screen, a window at any size, or a borderless window at the desktop's resolution, chosen in Options (its frame limiter is M-13; Internet play without the dead GameSpy and WON servers is M-17..M-20) | done (under Done) |

## Open middleware

The proprietary libraries, replaced by open-source ones in the modern build
only (the legacy build keeps the retail DLLs, from the player's own discs).
`build/ext.sh` builds the replacements from the `ext/` submodules into
`out/ext/` (once, for both modern variants; each piece again only when its
inputs change), and `build/all.sh` step 2 installs them in each game's
directory in place of the retail files (`MANIFEST.txt` origin `EXT`;
`retailextract.py openruntime`, the list in `build/retail.py` and
`build/retail_avp2.py`, `OPEN_RUNTIME`), removing any retail middleware an
earlier build left. The public archives' install scripts copy only what
`MANIFEST.txt` says came from the discs (`build/package.sh`,
`build/mkinstall.py`), so a modern archive's take no RAD or Miles file from
them (40 files, from 36 on the discs, all game data).
CI fetches `ext/openmiles` and `ext/ffmpeg` for the modern builds only
(`.github/workflows/public-archives.yml`).

Pins: `ext/ffmpeg` at FFmpeg `n8.1.3` (1041abd, shallow), `ext/openmiles`
at our fork's `miles6-fixes` (725c6ff): nolfsrc's, so the family builds one
OpenMiles and one FFmpeg. AvP2 1.0.9.6 and Primal Hunt ship the same
`mss32.dll` (Miles 6.0i) and `binkw32.dll` as NOLF's GOTY edition.

Checked with `build/playtest.sh` on `out/avp2-modern-release/`, its
`PLAYTEST_MOVIES=1` (the movies played) and `PLAYTEST_AUDIO` (each sound
stream the game opened recorded through ALSA's file plugin, one raw file
per format), under Xvfb and Wine in the build container.

| Id | Retail | Replacement | Files | Checked | Status |
|---|---|---|---|---|---|
| M-X1 | RAD's Bink 1.0h, `binkw32.dll` (the intro movies) | our Bink player on FFmpeg's Bink decoders: the thirteen `_BinkXxx@n` entry points `lithtech.exe` looks up by name (`binkvideo.cpp`, `BinkVideoMgr::Init`; it imports nothing from the DLL), with the four `BINK` fields and the four surface formats it reads. It is nolfsrc's player unchanged -- this project family's own code, and AvP2's engine asks exactly what NOLF's does of it (the same names, fields, surface formats, `BINKSNDTRACK` texture videos and end-of-movie test) -- copied into this tree rather than shared, so either can change. Sound through `waveOut`, not Miles | `src/modern/binkw32/` (`binkw32.c`, `binkw32.def`), `build/ext.sh`; `build/avp2cmds.sh` (`open`: no `+DisableMovies`), `build/all.sh`, `build/package.sh` | AvP2: `build/playtest-movies.txt` -- Fox Interactive, Sierra, LithTech and Monolith (`Movies/*.bik`) play through in turn with their sound, then the main menu. Primal Hunt plays no Bink movie at all (its client shell shows four `Interface\intro_*.pcx` splash screens instead, `NextSplashScreen`, and it installs no `Movies/`); its engine loads our DLL at startup like AvP2's and the same script runs to its menu | done |
| M-X2 | RAD's Smacker, `Smackw32.dll` | none: dropped. Neither game plays a Smacker video. Neither engine imports `Smackw32.dll`; it is loaded only by the Smacker video manager's `Init` (`videomgr.cpp`), and the only video manager either engine ever makes is Bink's -- the factory (`0x0048a770` in Primal Hunt's) is called once, with `"BINK"` (`clientmgr3.cpp`; the retail images push the `"SMACKER"` string only inside the factory). No `.smk` file is in either game's archives or directories; `CWinUtil::GetMoviesPath`'s `intro.smk` is only a file it looks for. With no DLL there is nothing to load: no import to satisfy and no engine change | `build/all.sh` (not installed, and removed from an older modern build) | both games run their movies/splash screens, menus and a level with no `Smackw32.dll` in the directory | done |
| M-X3 | Miles 6.0i, `mss32.dll`, with its MP3 decoder `MP3DEC.ASI`/`.AS6` | OpenMiles' `mss32.dll` with the Miles 6.0 export table; it decodes MP3 itself (`AIL_decompress_ASI`, which the engine's sound loader calls for every MP3 `WAVE` -- most of the music, dialogue and many effects), so no `.asi` is installed. Both engines import 89 functions from it by name (`_AIL_...@n`); OpenMiles exports every one. The game never names a 3D provider (`InitSoundInfo::m_sz3DProvider` stays empty), so positional sound is the engine's own over Miles' 2D samples, as with retail (which ships no `.m3d`); filters (`AIL_enumerate_filters`) are OpenMiles' built-in one | `ext/openmiles` (submodule), `build/ext.sh` (`ext/patches/openmiles/*.patch` would be applied to a copy; none needed) | AvP2 `build/playtest-avp2-marine.txt` and Primal Hunt `build/playtest-ph-corporate.txt`: the menus' sounds, the first mission's dialogue (MP3), effects and ambience recorded, no crash, quicksave/quickload. Music is DirectMusic's (`musictype`, `ILTDirectMusicMgr`), not Miles', and is not part of this item | done |
| M-X4 | Microsoft's DirectMusic, which the retail games expect Windows to have (the in-game music: `musictype ima`, `CLTDirectMusicMgr`), and which under Wine is Wine's own: it loads the music but plays silence (and Proton's is the same) | Microsoft's own DirectMusic (DirectX 9.0c), carried in each game directory: the nine DLLs from `ext/toolchains/directmusic-5.3.1.904/` (checksummed) as `ms_<name>.dll`, and `lithtech.exe.manifest`, a registration-free COM manifest mapping the 46 classes they serve to those files, so the engine's `CoCreateInstance`s, and the DLLs' of each other, reach them with any prefix as it comes and nothing registered (on Windows the system's DirectMusic is untouched); the `ms_` names keep Wine from substituting its builtins. nolfsrc's M-P4-12, checked against AvP2's and Primal Hunt's engines: they ask DirectMusic for the same things as NOLF's (the performance, a default synth port with `SetDirectSound(NULL)`, and, through the engine's own loader, segments, styles, bands and DLS collections out of the `.rez` archives; never the GM set, so no `gm.dls`). The manifest is nolfsrc's less two classes no DLL here implements. No engine change | `ext/tools/retailextract.py` (`openruntime`, `DIRECTMUSIC_MANIFESTS`; tools 314d71c), `build/retail.py`, `build/retail_avp2.py`, `src/modern/directmusic/` (`README.md`, `lithtech.exe.manifest`); `docs/LICENSE_NOTES.md` | Under Xvfb and Wine 9.0 in the build container, a fresh prefix each run, DirectMusic's DirectSound stream recorded (`PLAYTEST_AUDIO`, the 48 kHz one; `+LTDMConsoleOutput 5 +ErrorLog 1` lists the segments loaded and the intensities played). Primal Hunt, `build/playtest-ph-corporate.txt`, retail data: `EXP1Theme` loads and goes to intensity 2 (`Ambient*.sgt`) in exh1s2, and the stream carries the music (peak 0.14, RMS 0.005-0.014 by phrase). AvP2, `build/playtest-avp2-marine.txt`: the main menu has no DirectMusic music (the server's `CMusicMgr` starts it per level), and the opening, `m_open`, asks only for intensities 1 and 10 (`Silence.sgt`; its music is wave tracks through Miles), so the stream is silent there by design; with `M1Theme` sent to intensity 2 (`Ambient1.sgt`, by a test-only `Attributes/music.txt` in `custom/`) it plays with ours (peak 0.14) and is silent with Wine's (peak 0.00005, the same run without the manifest). No crash; sound effects, dialogue and Miles' wave tracks unchanged | done |
| M-X5 | Miles 6.0i's MP3 decoder gives the source's own format | OpenMiles fix (fork, `miles6-fixes` 40aa1d0): `AIL_decompress_ASI` -- which the engine's sound loader calls for every MP3 `WAVE` (`CSoundDataQ47C::InitFromCompressed`, `sndinst2.cpp`, then re-parses the RIFF image it gets back) -- and OpenMiles' built-in ASI stream provider asked miniaudio for 16-bit stereo at 44100 Hz, so every MP3 came back upmixed and resampled to that, with a header to match. AvP2's MP3s are 1866 mono 44100 Hz (`SOUNDS.REZ`: dialogue and effects), 99 stereo 44100 Hz (`AVP2.REZ`) and two stereo 22050 Hz; Primal Hunt's 197 mono and 6 stereo 44100 Hz (`DIALOGUE.REZ`, `AVP2X.REZ`): the mono ones held twice their memory as stereo, the 22050 Hz ones four times. Now decoded at the source's channels and rate, as Miles' decoder does. OpenMiles' own change, general (NOLF has the same, and 38 mono 22050 Hz ones), not a game change | `ext/openmiles` (submodule pin) | a Windows harness (`zig cc`, Wine) calling the built `mss32.dll` on MP3 data taken from `SOUNDS.REZ`: mono 44100 Hz before 2 ch / 44100 Hz, now 1 ch / 44100 Hz (62208 frames, as its `fact` chunk); stereo 22050 Hz before 44100 Hz (1438848 frames), now 22050 Hz (719424, as its `fact`). OpenMiles' tests (two new: a mono 22050 Hz WAV and MP3 come back so; the ASI stream's attributes). In both games (`PLAYTEST_AUDIO`, the scripts of M-X3) the dialogue and effects play, the mix's level as before (AvP2 RMS -22.0 / -21.9 dBFS, Primal Hunt -25.9 / -25.8), no crash, quicksave/quickload | done |
| M-X6 | Miles 6.0i saturates its mix into the output: a hard clip at full scale | OpenMiles fix (fork, `miles6-fixes` 725c6ff): miniaudio's engine mixes in f32 and handed the device each block as it was, so sounds summing past full scale reached Wine's float device unclipped (AvP2's first Marine mission: peak 1.21, 567 samples over 1.0). Miles 6.0i sums into 32-bit integers and saturates them (its `mss32.dll`'s mixer copy, `0x2111f017` on: `packssdw`, or compares against `0x7FFF`/`-0x8000`), no soft knee; OpenMiles now clips each mixed block to [-1, 1] in the engine's process hook, a NaN to silence | `ext/openmiles` (submodule pin) | AvP2 `build/playtest-avp2-marine.txt` recorded before/after: peak 1.2115 (567 samples over 1.0) -> 1.0000 (none over), RMS unchanged; Primal Hunt `build/playtest-ph-corporate.txt` peaks 0.69, unchanged. OpenMiles' test: four loud sounds at once sit flat at 1.0 (3.66 without the clip) | done |

## Done

M-X1, M-X2, M-X3 (the open middleware, above); M-X4 (the music, above); M-X5, M-X6 (OpenMiles fixes).
M-1..M-3, M-6, M-7, M-8, M-9, M-15, M-16, M-17..M-20 (below); the retail-bug fixes M-B01..M-B09 (the engine), M-B20..M-B26 (the renderers), M-B40..M-B63 and M-B108 (the client shells and lithshared), M-B70..M-B85 (the object DLLs and shared code), M-B100..M-B107 (the launchers and Launch.dll), below.

The frame timing (the user's report on the first modern build: "the high
framerate means all the timing is wrong"). nolfsrc found the same in NOLF
(its M-P1-01, -02, -05, -23, -25); each finding was checked here against
AvP2's and Primal Hunt's own code and measured in their games before it
was taken, and the code is ours.

| Id | What | Files | Status |
|---|---|---|---|
| M-10 | The world kept to real time at any frame rate (RETAIL_BUGS.md: "Above 100 frames a second the world runs fast") | `src/engine/server/smgr.cpp` (`CServerMgr::Update`) | done |
| M-11 | Animations lose no time to whole milliseconds | `src/modern/engine/p1_frametime.h`; `src/engine/lithtech/clientmgr2.cpp`, `clientupdate.cpp`; `src/engine/server/servermodel.cpp` | done |
| M-12 | The clocks from the performance counter | `src/engine/lithtech/ltnew2.cpp`; `src/AVP2/Shared/WinUtil.cpp`; `src/AVP2/ClientShellDLL/InterfaceMgr.cpp` | done |
| M-13 | The frame-rate limit: 60, 120, 144, 240 (the default) or none, in Options, Graphics, single player and multiplayer | `src/modern/cshell/m10_timing.h`, `m10_framelimit.h`; `GameClientShell.cpp` (`PostUpdate`), `FolderDisplay.cpp`; `src/AVP2/ClientRes/ClientRes.rc`, `ClientRes.h` (31000-31003) | done |
| M-14 | Amounts the game applied once a frame, made per 60th of a second | `src/AVP2/Shared/CharacterMovement.cpp`; `src/AVP2/ClientShellDLL/client_physics.cpp`, `FireFX.cpp` | done |

**M-10.** `CServerMgr::Update` runs a game tick every time it is called --
every client frame in single player and on a listen server -- and a tick
takes at least 10 ms of game time (`m_FrameTime` clamped to 0.01..0.2). So
above 100 frames a second the world ran at (frame rate / 100) times real
time: measured with the retail AvP2 1.0.9.6 binaries
(`ShowGameTime`, against the wall clock), 1.44 times real time at 139 fps
and 2.37 at 237; the first modern build, 2.97 at 297. A tick now runs only
once 10 ms have passed since the last one (time that runs backwards, as a
new world re-bases the clock, ticks at once); the packets and the network
are still serviced every call, and a paused server or a variable-step one
is as it was. The dedicated servers' own loops already hold them to their
`MIN_SERVER_UPDATE` (AVP2Serv's `ThreadUpdate`), and they get the same
gate. Measured (AvP2, 5 s windows of game time against real time):
1.000 at 60 fps, 0.999-1.002 at 120, 0.998-1.001 at 230 (limit 240) and
0.998-1.002 at 230-287 (no limit); Primal Hunt, 1.000 at 60 and
0.998-1.002 at 110-210 (limit 240).

**M-11.** The engine advances every animation tracker by the frame's time
in whole milliseconds, `(int)(frame time * 1000)`, and drops the rest:
models and sprites on the client (`cs_UpdateAnimations`, and the sprites, the
particle systems' sprites, sprite objects and models' sprite skins in
`clientupdate.cpp`) and models on
the server (a tick's milliseconds, once a tick). Up to a millisecond a
frame, about half of one on average: little at 60, up to an eighth of the
time at 240 (4.2 ms frames), a
sixth at 300 -- animations, and with them AvP2's weapons, whose firing is
paced by their animations (`CWeaponModel::UpdateFiring`), ran slow. The
fraction is now carried into the next frame (`P1_FrameMs`, `P1_TickMs`).

**M-12.** The engine's clock (`time_GetTime`, the frame times) counted
`timeGetTime`'s whole milliseconds, so at a few milliseconds a frame the
frame times jittered by a millisecond (a quarter of a frame at 240); it is
the performance counter now. The game's own clock (`CWinUtil::GetTime`:
the menus' and loading screen's model animations) was `GetTickCount`'s
10-16 ms steps -- at a high frame rate most frames lasted no time and the
odd one a whole step -- and is the performance counter too; the menus'
model animations then carry their fraction of a millisecond as M-11 does
(`CInterfaceMgr::UpdateModelAnimations`).

**M-13.** `FramerateLimit` (below): each client frame, once drawn
(`CGameClientShell::PostUpdate`), waits until 1/FramerateLimit of a second
has passed since the previous frame was let go: it sleeps in 1 ms steps
(`timeBeginPeriod(1)`, from winmm.dll, which the engine has loaded) while
more than 2 ms remain, then spins; frames keep the cadence, and after a
frame longer than a whole period the count starts again from it. It holds
in multiplayer as in single player (what made multiplayer wrong at a high
frame rate was M-10 and M-11, put right for both), and not while a world
loads (the game state is `GS_LOADINGLEVEL`). In the menus: Options,
Graphics (`CFolderDisplay`), "Frame rate limit", between Resolution and
Performance, in both games (one source, `FolderDisplay.cpp`; their
folders' layouts are their own, and alike here): 60, 120, 144, 240 or
None. The folder's page holds two lines above its note on performance;
it is a line taller, and the note a line lower, else Performance went
to a second page. A limit set in the console
or autoexec.cfg that is not one of them shows as the next higher choice,
and is kept unless another is chosen. Measured: 60.0, 119.4-120.0 and
223.7-234.7 frames a second at the limits 60, 120 and 240 in AvP2 (in
the container, where 240 is near the machine's most; no limit,
230-287), 59.8-60.7 at 60 in Primal Hunt. The option was checked in both
games' menus: it shows the limit set, a click cycles it, and leaving the
folder writes it to autoexec.cfg.

**M-14.** The game code's amounts applied once a frame, at whatever rate,
are made the amount of a 60th of a second, at which the game was tuned
(at 60 frames a second each is what it was):
- `CharacterMovement::Update`: a velocity under 3 units a second is
  zeroed each frame; movement starts from rest by one frame's
  acceleration, which a short frame makes small, so the threshold is the
  60 fps frame's share. And a frame's move under 0.1 units was dropped
  (on the client): 6 units a second at 60, 24 at 240.
- The crouching and ladder states' "don't skid" damping, half (three
  quarters) of the velocity each frame: four times as strong at 240.
- `client_physics.cpp` (debris, shells): the speed at which a bounce comes
  to rest grows with the square root of the frame rate (116 units a
  second at 60, 232 at 240).
- `FireFX`: one chance in ten a frame of sparks.
The rest of the movement is time-based already (the engine's physics
integrates friction analytically), and measured so (a quicksave of the
first Marine and Corporate missions, walking 3 s, a jump, the starting
melee weapon held down 4 s; the probe's lines were temporary):

| Game, limit | frames a second in play | walking | jump: height, time in the air | swings apart |
|---|---|---|---|---|
| AvP2, 60 | 35-49 | 300.8 u/s | 78.9, 1.00 s | 346 ms |
| AvP2, 120 | 47-82 | 300.1 | 77.7, 1.03 | 333 |
| AvP2, 240 | 44-65 | 300.3 | 77.6, 1.01 | 346 |
| AvP2, none | 48-71 | 300.3 | 77.3, 1.00 | 336 |
| Primal Hunt, 60 | 60 | 300.7 | 77.9, 1.00 | 329 |
| Primal Hunt, 240 | 148-183 | 300.1 | 76.6, 1.05 | 337 |
| Primal Hunt, none | 67-89 | 300.2 | 77.3, 1.03 | 335 |

(The container, shared with other builds, drew no more than that in
play; the higher rates were reached in the levels' intros, where the
world's speed was measured.)

Checked against the retail game: the legacy build is unchanged (every
change is in an `AVP2SRC_MODERN` block or in `src/modern/`;
`ext/tools/releasesrc.py strip` gives each touched file as it was), and
`make progress` keeps every recorded function.

Not done: the per-frame review covered the movement, physics and effects
that nolfsrc's NOLF findings pointed at; other client effects that step
once a frame without the frame time have not all been looked at, nor
measured above about 80 frames a second in play.

### M-1..M-3: widescreen, 4:3 cinematics, the field of view

nolfsrc's NOLF items were the guide (its M-P0-01 Hor+ field of view,
M-P1-07 the resolution list, M-P3-08 4:3 cinematics, M-P3-21 the zoom
beside the 4:3 area); each was checked against AvP2's and Primal Hunt's
own code, which differ from NOLF's where it matters, and the code is ours.

| Id | What | Files | Status |
|---|---|---|---|
| M-1 | Widescreen: any shape of screen in the resolution list; the view widened to it (Hor+: the vertical field of view kept, the horizontal one widened by the screen's aspect ratio over 4:3), the player-view weapons and the camera overlays with it, a scope's overlay kept 4:3 with black beside it; the modern engine's own mode taken (it asked for nolfsrc's `d3d9.ren`, so every modern game ran at 640x480x16) | `src/modern/engine/m1_horplus.h`, `src/modern/cshell/m1_widescreen.h`; `src/engine/lithtech/clientmgr2.cpp` (`cm_RenderCamera`, `r_GetRequestedMode`), `clientinterface.cpp` (`Get3DCameraPt`); `src/engine/d3d/ph/drawmodel.cpp`; `FolderDisplay.cpp`, `OverlayMGR.cpp`/`.h`, `ViewModeMGR.cpp`/`.h`, `InterfaceMgr.cpp`, `AutoTargetMgr.cpp` | done |
| M-2 | `RestrictCinematicsTo4x3`: the cinematics in the centred 4:3 area of a wide screen, black beside it; Options, Graphics, "Cinematics" (Widescreen, 4:3) | `m1_widescreen.h`; `InterfaceMgr.cpp` (`Draw`), `FolderDisplay.cpp`; `ClientRes.rc`, `ClientRes.h` (31200-31206) | done |
| M-3 | `FieldOfView`: the player's horizontal field of view at 4:3, 75 to 120 degrees (90, retail's, the default); Options, Graphics, "Field of view" | `m1_widescreen.h`; `src/AVP2/Shared/PlayerMovement.cpp`, `CameraMgrFX.cpp` (`CameraMgrFX_ZoomUpdate`), `FolderDisplay.cpp`; `ClientRes.rc`, `ClientRes.h` | done |

**M-1.** Retail gives every camera a field of view for a 4:3 screen (the
player's is the character's `FOV` in CharacterButes.txt: 90 by 78 degrees
for a human or a Predator, 95 by 85 for an Alien, 75 by 65 for an
Exosuit, the same in both games; a cinematic camera's is its object's, 75
by 60 by default) and draws it across whatever screen there is, so a 16:9
screen showed the 4:3 picture stretched sideways; and Options, Graphics
listed only modes of exactly 4:3 (`CFolderDisplay::GetRendererData`). The
modern build lists every mode of at least 640x480, and the engine widens
the horizontal field of view by the screen's aspect ratio over 4:3,
`fovX' = 2 atan(tan(fovX/2) * aspect/(4/3))`, keeping the vertical one:
where it hands a camera to the renderer (`cm_RenderCamera`) and where it
turns a screen point into a direction (`Get3DCameraPt`), so the picture
and what the game measures against it agree, while the cameras, and what
the client reads back from them, keep the game's 4:3 values. nolfsrc's
M-P0-01 does the sum in NOLF's renderer from the scene rectangle; here
the factor is the screen's, because AvP2 letterboxes a cinematic by
shrinking the camera's rectangle with the same field of view
(`CameraMgrFX_WidescreenUpdate`, and the Predator's vision change narrows
it): a 4:3 screen keeps every view exactly as retail draws it.

The player-view weapons have a field of view of their own: the client
sets `ExtraFOVXOffset` each frame to `ViewModelBaseFOVX` (95) less the
camera's (`CWeaponModel`), and the renderer adds the scene's (`DrawModel`,
`m_bFovScale`). With the scene's widened that sum is off, so the renderer
works the model's field of view out at 4:3 as retail does (the scene's
narrowed back by the screen's factor) and widens it the same way: the
weapon keeps its 4:3 shape and size on any screen, and still ignores the
player's field of view (M-3).

The camera overlays (`OverlayMGR`: the vision modes' frames, the net, the
fire, the Evac and orientation ones) are sprites sized to the view through
`Get3DCameraPt` (`GetCameraScale`), so they now cover a wide screen,
stretched with it as retail's are stretched to 4:3 (the Predator's zoom
overlay, `predzoom.spr`, is one of them: it is up only while the zoom
changes, `CPlayerStats`). The rail gun's scope (`railgunoverlay.spr`,
up while it is zoomed) keeps its 4:3 shape in the middle, and the screen
beside the 4:3 area is filled black while it is up and the view is first
person (`InterfaceMgr::Draw`; nolfsrc's M-P3-21 found the same gap in
NOLF's sunglasses zoom).

What the client places on the screen from the world goes through
`Get3DCameraPt` too (`GetScreenPos`: the Predator's target lock,
`PredTargetFx`, and the auto-targeting), so it lands right on a wide
screen. The auto-targeting takes a target only inside a box of 30% to
70% of the screen's width and height (`CAutoTargetMgr`); across a wide
screen that is a wider angle than retail's, so the box is now that share
of the centred 4:3 area's width (`M1_InTargetBox`), the same angle.

The menus are retail's 640x480 page, centred and unscaled at any
resolution, which serves any shape of screen as it is; the HUD is
anchored to the screen's corners and edges (`CHudElement`'s
`m_ptOffset` from each corner, the screen's centre for the centred ones)
and is placed right on any shape too, at its retail size.

Options, Graphics shows the mode in use (the folder showed the mode it
had last set itself, `GameScreenWidth`/`Height`/`BitDepth`, which a
command line's mode -- AVP2.exe's -- is not: it showed another entry and
leaving the folder switched to it).

Also found: the modern engine's `r_GetRequestedMode` (nolfsrc's block)
asked for `d3d9.ren`, nolfsrc's Direct3D 9 renderer, in place of
`d3d.ren`. AvP2 ships none, so the engine's mode, and the client shell's
first fallback (640x480x16, the same DLL) failed, and every modern game
ran in its last fallback, 640x480x16 through `d3d.ren`, whatever mode was
asked for (`OnEngineInitialized`; the debug output says "Couldn't set
render mode", then "Setting render mode to D3D Emulation mode"). The
block is dropped.

Checked under Xvfb and Wine (`build/playtest.sh`, `PLAYTEST_RES`): AvP2's
first Marine mission standing at its start, 1024x768 and 1280x720. The
4:3 shot scaled to 720 lines against centre crops of the 16:9 one, 2% of
width apart: the best match is the centre 960 columns (RMSE 0.014; the
crops either side 0.026), so the 16:9 view shows the 4:3 one in its
middle at the same scale, with the vertical field of view unchanged, and
its horizontal one is 106.3 degrees (Hor+: 2 atan(tan 45 x 4/3) =
106.26). The weapon is the same size in both. At 2560x1080 (21:9) the
best match is the centre 1440 columns (RMSE 0.012; either side 0.025):
121.3 degrees (Hor+: 121.28). Primal Hunt's first Corporate mission
("Zeta Site"), 1024x768 against 1280x720: the centre 960 (RMSE 0.029;
either side 0.070), 106.3 degrees. Shown at 1280x720 in both games: the
main menu and Options, Graphics (the page centred), the Marine's,
Predator's, Alien's, Corporate's and Predalien's HUDs in their corners,
the Predator's zoom and three vision modes, the Alien's and the
Predalien's vision, their overlays across the whole screen. The rail
gun's scope was not reached in play (it is the Marine's last weapon), so
its 4:3 shape and bars are checked from the code only.

**M-2.** `RestrictCinematicsTo4x3` 1 (0 by default): while the cinematic
camera is active, the screen beside its centred 4:3 area is filled black
after the scene is drawn (`CInterfaceMgr::Draw`, after retail's own
borders), so a wide screen shows what a 4:3 one does; the subtitles are
drawn over it, inside the area. nolfsrc's M-P3-08 has the same name and
meaning. Checked in AvP2's opening cinematic at 1280x720: 160 columns
black at each side (mean 0 in the left 150 columns) with it, the full
width without; once the player is in control, nothing beside the area is
filled (mean 0.056). Primal Hunt's Predalien opening at 1280x720 the
same (mean 0 in the left 150 columns).

**M-3.** `FieldOfView` (degrees, 60..130; the menu offers 75 to 120 in
fives): the horizontal field of view at 4:3 of a human or a Predator.
Every character's own is scaled with it, both ways alike, so each keeps
its shape: `tan(fov'/2) = tan(fov/2) x tan(FieldOfView/2)` (an Alien's 95
by 85 at 90 is 114.4 by 105.1 at 110). It is set where the client gives
the player's cameras the character's field of view
(`PlayerMovement::HandleMessage_Physics_Update`, `SetMPButes`), and at
once when the option changes. A wide screen widens it again (M-1). The
zoom (the Predator's, the rail gun's) magnifies over the camera's own
field of view (`CameraMgrFX_ZoomUpdate`); its magnitude is made one over
retail's (times the setting's factor, which comes in over the first
doubling), so a zoomed view is retail's whatever the setting. The
cinematics keep their own, and the weapons theirs (M-1). Checked at
1024x768 in the same place as M-1: the shot at 90 matches the centre
716x537 of the one at 110 (RMSE 0.019; either side 0.032), which makes
the latter's horizontal field of view 110.1 degrees.

The menus and the HUD kept their retail size in pixels, so on a large
screen they were small (the 640x480 page a third of a 1920x1080 screen's
height): they are scaled now, M-15 and M-16 (below). Windowed and
borderless-window modes are M-9's.

### M-15, M-16: the menus and the HUD scaled to the screen

nolfsrc's M-P3-01 (the menus in the 4:3 area, scaled) and M-P3-12 (HUD
scaling) were read for what to do; how is AvP2's own, because its
interface is built differently. NOLF's HUD is a handful of hand-placed
draws in `PlayerStats.cpp`, which nolfsrc scaled one by one, and it left
all text at its bitmap font's size. AvP2's HUD is data: `CHudMgr` builds
each species' HUD from the layout (`CHudElement`s with an offset from a
corner, an edge or the centre, and some forty draw and update functions:
the health, armour and energy bars, the Marine's EKG and ammo counters,
the Predator's energy and skull counter, the Alien's health, the
Exosuit's, the pickup list, the air and battery meters, the boss meter,
Primal Hunt's Predalien and Corporate HUDs and turret ammo), beside the
motion tracker (`CMotionDetector`), the crosshairs (`CCrosshairMgr`), the
messages (`CMessageMgr`), subtitles, teletype, objectives and weapon
choosers, with text in bitmap fonts drawn a glyph at a time
(`lithbitmapfont.cpp`). Scaling each draw would touch hundreds of places.

So each is drawn on a smaller virtual screen scaled onto the real one (a
"space", `src/modern/cshell/m15_uiscale.h`). The client shell puts
wrappers into the engine's `ILTClient` table once (`M15_Install`,
`OnEngineInitialized`; the engine's own back at `OnEngineTerm`): while a
space is entered, `GetSurfaceDims` of the screen surface answers the
space's size, and every blit onto the screen surface -- `Draw*` (made a
`Scale*`), `Scale*`, `Warp*`, `Transform*` (the origin scaled, the
destination kept at its offset from it, the scales multiplied) and
`FillRect` -- has its destination scaled. The game's layout code then
runs unchanged: an element anchored to the bottom-right corner lands in
the corner, larger, and its text with it. Outside a space, for any other
destination, or where a space's scale is 1, every call goes through
untouched: at 640x480 always, so a 640x480 game is retail's, draw for
draw. `CInterfaceResMgr` reports the 640x480 page's offsets, ratios and
screen size in the space being drawn (its retail values otherwise). The
space is per thread (the loading screen draws from its own).

* The menu space (M-15): the page fills the screen's height (`MenuScale`
  100, percent of that; never wider than the screen, never below 1, 0 is
  retail's unscaled page), centred: 2.25 at 1080 lines, the page
  1440x1080 in the middle of 1920 or 2560. Drawn in it: the folders
  (`UpdateFolderState`), the message box (and centred in it,
  `CMessageBox::Show`), the loading screen (its own thread), the pause
  notice, the mission-failure screen and Primal Hunt's unscaled splash
  screens. The mouse's point is taken into it (`CInterfaceMgr::On*Button*`,
  `OnMouseMove`, `OnMouseWheel`), and the folders' keys, focus and
  updates run in it for input only (the page's offsets, not the screen's
  size, so a folder command that sets a camera rectangle from the
  screen still gets the real one; `CInterfaceMgr::ScreenDimsChanged`
  runs in screen pixels whatever called it): the cursor clicks a
  control where it is drawn. The menus' 3D parts (the background sprite,
  the scale FX, the character models; `CreateInterfaceBackground`,
  `CBaseFolder::CreateScaleFX`/`CreateCharFX`, `FolderPlayer`,
  `FolderPlayerJoin`, `LoadingScreen`) are pushed back by the menu
  space's height over 480 (`M15_MenuDepth`) where retail used its X
  ratio, so with M-1's Hor+ they line up with the page (1 when it fills
  the height). The cursor is the system's, at its own size.
* The HUD space (M-16): `HUDScale` percent (75 by default) of the
  screen's height over 480, never below 1 or wider than the screen: 1.69
  at 1080 lines, 1.125 at 720, 1 at 640x480 and 800x600. Drawn in it
  (`CInterfaceMgr::Draw`): the HUD (`CHudMgr`, the motion tracker), the
  scope's mask, the crosshairs (the smart gun's target, a point of the
  screen, taken into the space), the objectives and their notice, the
  weapon choosers, the credits, the timer, the subtitles; and the
  messages and chat line (`CMessageMgr`), teletype, story items and
  Primal Hunt's turret ammo. Laid out in it: `CHudMgr::Init` (each
  element's place, the tracker's), the pickup list, a message's lines, a
  subtitle's place. A new `HUDScale` (or screen) lays the HUD out again
  on the next frame (`M15_Refresh` in `CInterfaceMgr::PreUpdate`).
* Left in screen pixels: what is placed against the 3D view or the
  camera (the cinematic borders, M-1/M-2's 4:3 bars, the overlays, the
  Predator's target lock and vision wipe), and the multiplayer
  scoreboard and its class menus (`MultiplayerClientMgr`), which already
  scale themselves: retail picks their fonts by the screen's width (its
  largest set above 1024).

The scale is smooth, not a whole number: the page must fill the height
(2.25 at 1080 lines; a whole 2 would leave bands, 3 would not fit), and
the art is painted 16-bit bitmaps and antialiased fonts, not pixel art,
which nearest-neighbour at 2.25 would draw with uneven columns. The
renderer draws the interface in its optimized 2D mode, as textured quads
filtered by its `Bilinear` variable (on by default): bilinear. The space's
size is rounded to whole pixels and its two factors taken from it, so
its edges are exactly the screen's.

| Id | What | Files | Status |
|---|---|---|---|
| M-15 | The menus scaled: the 640x480 page fills the screen's height (`MenuScale`, percent, 100; 0 retail's size), centred, with its 3D parts, the message box, loading, pause, failure and splash screens; the cursor's clicks taken onto it; Options, Graphics, "Menu size" (Unscaled, 50%..100%) | `src/modern/cshell/m15_uiscale.h`; `InterfaceMgr.cpp`, `InterfaceResMgr.h`, `GameClientShell.cpp` (`OnEngineInitialized`, `OnEngineTerm`), `BaseFolder.cpp`, `FolderPlayer.cpp`, `FolderPlayerJoin.cpp`, `LoadingScreen.cpp`, `MessageBox.cpp`, `FolderDisplay.cpp`; `ClientRes.rc`, `ClientRes.h` (31800-31805) | done |
| M-16 | The HUD scaled: every species' HUD, the motion tracker, crosshairs, messages, subtitles, teletype, objectives, weapon choosers and story items by `HUDScale` percent (75) of the screen's height over 480, never below retail's size, anchored as retail anchors them; Options, Graphics, "HUD size" (Unscaled, 50%..100%) | `m15_uiscale.h`; `InterfaceMgr.cpp` (`Draw`, `PreUpdate`), `HudMgr.cpp` (`Init`, `AddPickupIcon`), `CrosshairMgr.cpp`, `MessageMgr.cpp`, `Subtitle.cpp`, `TeleType.cpp`, `StoryElement.cpp`, `ClientTurretMgr.cpp`, `FolderDisplay.cpp`; `ClientRes.rc`, `ClientRes.h` | done |

Checked under Xvfb and Wine (`build/playtest.sh`, `PLAYTEST_RES`,
`PLAYTEST_MAP=modern` -- the playtest's page map for the centred 4:3
area scaled by the height, which is now where the page is), defaults
(`MenuScale` 100, `HUDScale` 75):

* AvP2 at 1920x1080: the main menu, Options and Graphics drawn 2.25
  times, the page 1440 wide in the middle, the 3D backgrounds and the
  facehugger egg with it; the cursor clicked Options and Graphics at their
  page places and, hovering on Graphics at the page's (120, 238),
  highlighted Performance; the new Menu size and HUD size lines show
  100% and 75%. At 2560x1080 Single Player's page in the middle 1440
  columns, the Marine's head beside it. At 640x480 the pages as retail
  (scale 1: every call passed through).
* AvP2's first Marine, Predator and Alien missions at 1920x1080, and the
  Marine's at 1280x720: each HUD in its corners at 1.69 (1.125), the
  motion tracker and its range, the health/armour numbers, the flare
  count, the Predator's health and energy columns and skull, the Alien's
  health bar, the "objectives updated" notices, the training message,
  the Predator's teletype, the crosshairs.
* Primal Hunt at 1920x1080: the main menu and its folders (as AvP2's),
  the first Corporate mission's HUD (its motion tracker, health, armour,
  flares); at 2560x1080 the Predalien's; at 1280x720 the Predator's.
* A cinematic's subtitles (`Subtitles 1`, `RestrictCinematicsTo4x3 1`):
  AvP2's first Marine mission's opening at 1920x1080, the line drawn at
  1.69 in the centred 4:3 area.
* Not reached: the multiplayer scoreboard (left at retail's own
  font-by-width scaling, unchanged), the rail gun's scope mask, Primal
  Hunt's turret ammo and the boss meter (from the code: they are drawn
  in the HUD space like the rest).

Also: the folders' paging and focus handed the cursor's screen point
where they take a page point (M-B108, below).

### M-6: raw mouse input

Retail turns the view by the engine's axis offsets
(`ILTClient::GetAxisOffsets`), built once a frame from DirectInput's
buffered mouse data, with the cursor put back in the middle of the
desktop every frame (`CursorCenter 1`). How far a count of mouse motion
turns the view there depends on the frame rate and on how the motion is
cut into events (RETAIL_BUGS.md, "The mouse look turns further at a
higher frame rate"). The modern build reads the mouse's own motion
instead, in both games. nolfsrc did this for NOLF (its M-P1-03, the raw
mouse look, and M-P1-04, the Mouse options); its design -- `WM_INPUT` on
the window the client shell already subclasses, VC6's missing raw-input
declarations written out and the two functions looked up in user32.dll,
the cursor hidden and confined in play and given back in the menus,
`OldMouseLook` and `NoRawInput` -- was taken as the model, and its file
read for it; the code is AvP2's own, made against AvP2's and Primal
Hunt's hooks, which differ: the look is `PlayerMovement`'s (an offset of
1 is 90 degrees of yaw about the character's own up axis, so the Alien's
wall walking, and 45 of pitch), read twice a frame (`UpdateRotation`,
`UpdateAiming`) and by the camera editor (`CameraMgrFX`), and Primal
Hunt's turrets aim by it; the scale is AvP2's own
(`SetMouseSensitivityScale`: the sensitivity, the Predator's zoom and the
EMP's slowing); and NOLF's halving of its scale did not carry over (the
gain below is measured here).

| Id | What | Files | Status |
|---|---|---|---|
| M-6 | The mouse look from raw input (`WM_INPUT`), a count the same turn at any frame rate; the cursor hidden and kept to the window in play, given back in the menus; `OldMouseLook 1` the retail look, `NoRawInput 1` the cursor's motion | `src/modern/cshell/m6_mouse.h`; `GameClientShell.cpp` (`HookedWindowProc`, `Update`, `OnEvent`, `SetMouseSensitivityScale`, `UnhookWindow`), `InterfaceMgr.cpp` (`UseCursor`), `src/AVP2/Shared/PlayerMovement.cpp` (`UpdateRotation`, `UpdateAiming`), `CameraMgrFX.cpp` | done |
| M-6 | Controls, Mouse: "Legacy mouse look" (`OldMouseLook`), and "Mouse smoothing" explained as the legacy look's | `FolderMouse.cpp`; `src/AVP2/ClientRes/ClientRes.rc`, `ClientRes.h` (31600-31602) | done |

**The look.** The mouse's `WM_INPUT` (relative counts; an absolute
device's -- a tablet, a remote desktop -- turned into motion) is summed
between frames and taken once a frame (`M6Mouse_Update`, in
`CGameClientShell::Update` beside the controller's read), so both of
`PlayerMovement`'s reads see the same frame's look, as they do the
engine's offsets. A count is the retail scale's offset,
`(0.166 + MouseSensitivity / 2.75) * scale * 0.001` across and 1.1 times
that up and down, times 2: the retail look turned about twice its own
scale at 60 frames a second or less (below), the frame rates it was
played at, so at the default sensitivity (7) a count turns 0.49 degrees
across and 0.27 up and down. The engine's offsets are not read in this
look: they hold only the mouse's axes (M-7's sticks are read by the
client shell, and added as before), and DirectInput still gives the
mouse's buttons and wheel to the bindings. `InputRate` (Mouse smoothing)
applies only to the legacy look.

**The cursor.** In play (`CInterfaceMgr::UseCursor(FALSE)`) the cursor is
hidden, clipped to the window's client area and kept in its middle (so a
pointer-made raw input, as Wine's without a clip or a remote desktop's,
never meets an edge), and `CursorCenter` is 0; in the menus it is shown
and given the retail clip back (the full-screen clip, none in a window).
While another application is in front (`WM_ACTIVATEAPP`) the clip is let
go and the motion ignored; a new display mode (`LTEVENT_RENDERINIT`)
clips to the new window. `OldMouseLook 1` is the retail look and
`CursorCenter 1`, unchanged; `NoRawInput 1` reads the cursor's motion
from the window's middle each frame, as is also done if raw input cannot
be registered. Either can be changed in play (the console); the look
follows the next frame.

**The Mouse page** (Options, Controls, Mouse; both games, one source):
"Legacy mouse look" (No/Yes, `OldMouseLook`) under the sensitivity, with
the help "No: the mouse's own motion, the same at any frame rate. Yes:
the original mouse look."; "Mouse smoothing" keeps its name and slider,
with the help "Smooths the legacy mouse look. The mouse's own motion is
not smoothed."; Restore defaults turns the legacy look off. The page has
room for the line (five controls, as shown).

**Checked** under Wine in the build container (Ubuntu 24.04's Wine 9.0,
Xvfb): `xdotool`'s relative motion reaches the game as `WM_INPUT` (Wine
makes raw input from the X pointer's motion), so the playtests move the
mouse by known counts (`turn`): `build/playtest-avp2-mouse.txt`,
`-mouse-predator.txt`, `-mouse-alien.txt` and Primal Hunt's
`build/playtest-ph-mouse*.txt` go through the menus with the cursor to
each species' first mission and turn 200 counts across and back, 100 up
and down, in 20-count and in 5-count moves; the Predator's then zoom
(Page Up) and turn again, the Alien's turn with wall walking on (G). A
temporary probe logged the player's view each frame and the counts the
game took (its lines were removed):

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

-- every move, in every run, to the third decimal: the turn is the
counts', whatever the frame rate or the moves' size, and 200 counts
right and 200 left come back to where they started. The retail look
(`OldMouseLook 1`, the same scripts): AvP2's Marine 0.39-0.43 degrees a
count at 20-40 frames a second, 0.55-0.57 at 30-55, 0.61-0.70 at 60-140;
Primal Hunt's Corporate 0.32-0.58 at 32-46, and 200 counts right and
back left 29 degrees off where it began. The other machines' builds
loaded the container's host throughout: no run reached more than about
140 frames a second in play (none in raw input above 77), so the 240
limit was measured at what the host gave.

`build/playtest-mouse-menus.txt`, both games: Options, Controls, Mouse
with the cursor; Legacy mouse look turned on (its help shown); the first
Marine (Corporate) mission, where 200 counts right and back turned 82 and 96 degrees in AvP2, 36 and 92 in Primal Hunt; the
pause menu with the cursor, back to the Mouse page, Legacy mouse look
off, Resume; and 200 counts then turned 97.6 degrees right and 97.6 back in both. The setting is saved (`"OldMouseLook" "0"` in autoexec.cfg).

Not done: a real mouse on Windows (the raw input path is the same; under
Wine, raw input made from a real device was not tried), and
`RIDEV_NOLEGACY` (the mouse's own messages are kept, for DirectInput's
buttons and the menus).

### M-9: window modes

The user: "I don't see a way to set fullscreen mode." Two causes. The run
scripts (`linux-run-avp2.sh`, `linux-run-ph.sh`, from
`build/game-scripts/linux-run.sh`) appended `++screenwidth 640
++screenheight 480 ++bitdepth 32 ++windowed 1` to every run, so every game
started in a 640x480 window whatever Display had been set to (M-1's wide
modes included), and the games have no window mode in their options:
retail's only window is the command line's `++windowed 1`, placed by the
renderer, wrongly (RETAIL_BUGS.md, "A window is placed and sized for a
frame it does not have"). nolfsrc's M-P1-06 and M-P1-22 (NOLF's Display
"Window mode" and `p1_window.cpp`, NOLF's run script) were read for the
design; AvP2's and Primal Hunt's client shells, engines and both
`d3d.ren`s were checked against it and the code is ours: the hook points
are the same (the client shell's `HookWindow` and `LTEVENT_RENDERINIT`,
the renderers reading `Windowed` when they start, the engine shutting the
renderer down on `WM_ACTIVATEAPP`), the Display folder is AvP2's
(settings applied on leaving it, its own `SetRenderer`), and three things
NOLF's did not need were found under Wine here (a framed window as large
as the desktop: Windows' tracking size, and Wine drawing a window whose
client area runs off the screen black; a full screen the display cannot
take at start-up).

| Id | What | Files | Status |
|---|---|---|---|
| M-9 | Options, Graphics (the Display folder), "Window mode": Full screen, Window, Borderless window. The engine's `Windowed` (what both renderers read when they start) and `WindowMode` (2: that window borderless), both saved; the mode is set again when it changes. After every renderer start (a mode set, the focus regained) the window is put right: full screen as `HookWindow` makes it (no frame, topmost, the mode's size); a window with a title bar and the mode's size as its client area, centred in the work area (one as large as the desktop with its client area on the screen and the frame off it; `WM_GETMINMAXINFO` lets it be larger than the screen); a borderless window `WS_POPUP` at 0,0 at the desktop's own resolution (`EnumDisplaySettings`' registry mode), the resolution list greyed for it, and a mode set from elsewhere changed to the desktop's on the next frame. A first run (no `WindowMode`) starts borderless unless it asks for a window. A full screen that fails at start-up (the display cannot take the mode) comes up as the borderless window, before retail's 640x480x16 fallback; one chosen in the folder that fails goes back to the window mode it was | `src/modern/cshell/m9_window.h`; `GameClientShell.cpp` (`OnEngineInitialized`, `OnEvent`, `Update`, `HookedWindowProc`), `FolderDisplay.cpp` (`Build`, `OnCommand`, `SetRenderer`, `OnFocus`); `src/AVP2/ClientRes/ClientRes.rc`, `ClientRes.h` (31900-31904) | done |
| M-9 | The run scripts keep the game's own window settings: `linux-run-avp2.sh` / `linux-run-ph.sh` pass a mode only when asked -- `--fullscreen`, `--windowed`, `--borderless` (the modern build's), `--res WxH` (as `++` arguments, so the game saves them) -- or, in the release build, on the first run (no `autoexec.cfg`): a 640x480 window, as before. `windows-run.bat` (which never added a mode) takes the same options | `build/game-scripts/linux-run.sh`, `windows-run.bat`; `build/all.sh` (`@MODERN@`) | done |

**Full screen under Wine.** Both renderers' full screen is DirectDraw's
exclusive mode with `SetDisplayMode` to the chosen resolution. Under Wine
that changes the X display's own mode, which fails unless the X server
offers that mode: Xvfb offers only its own size, so there (and on X
servers or Wayland compositors without the mode) `NtUserChangeDisplaySettings`
returns -2 (`DISP_CHANGE_BADMODE`), `SetDisplayMode` fails and so does the
renderer. At the desktop's own resolution full screen works (measured:
both games, 1280x720 on a 1280x720 Xvfb, the window `WS_VISIBLE`, topmost,
0,0 1280x720). Hence the borderless window as the first run's mode, and
the fallbacks above; the release build's run scripts keep their window.

**The run scripts.** The release build has no window option of its own,
so its first run is still a 640x480 window (its renderer's full screen
fails under many Wine setups) and its later runs are what it saved;
`--fullscreen`, `--windowed` and `--res` change it (`--borderless` is
refused). Checked with a stand-in for `wine` printing the engine's command
file, each option in both builds; `windows-run.bat` under Wine's `cmd`, its
command file for each option (on a copy without its `cd /d ... || exit /b
1`, at which Wine 9.0's `cmd` stops: a line the script had before).

Checked under Xvfb and Wine 9.0 in the build container (a test harness
running the game directories' own `linux-run-*.sh` under `xvfb-run`, the
window measured as X sees it, `xdotool`, and as Windows sees it, a small
Win32 program listing its style, rectangles and state; the game closed with
`WM_CLOSE` so that it saves `autoexec.cfg`), on a 1280x720 display, both
games:

* A first run, no options: borderless, `WS_POPUP`, 0,0 1280x720 (client
  the same), the menus scaled to 720 lines (M-15); saved `Windowed 1`,
  `WindowMode 2`, 1280x720.
* `--windowed --res 800x600` (AvP2), `--res 1024x576` (Primal Hunt): a
  title bar, client 800x600 at 240,73 (1024x576 at 128,85), centred; the
  main menu in it; the next run with no options the same (saved).
* `--windowed --res 1280x720`: the client area 0,0 1280x720, the frame
  off the screen (before: Windows cut the window to the screen, a
  1280x700 client area, and Wine drew it black, as it did any window
  running off the screen).
* `--fullscreen` at 800x600 (AvP2) / 1024x576 (Primal Hunt): the display
  refused the mode, the game came up borderless at 1280x720 and saved
  that; `--fullscreen --res 1280x720`: full screen, topmost.
* In the menus (AvP2, Options, Graphics, the cursor): Borderless window
  -> Window -> Full screen -> Borderless window, each applied on leaving
  the folder and measured, the folder entered again by the cursor at its
  page place after each (the window's client area at 0,0, M-15's page
  mapping unchanged); the resolution greyed in the borderless window;
  the choice saved.
* The focus (another program, Wine's Notepad, brought to the front):
  borderless, the window stays where it is, behind it; full screen, Wine's
  DirectDraw minimises the window (measured: iconic), as Windows' does, so
  no black window is left over the desktop; the placement restores it
  when the focus comes back (bringing it back needs a window manager or a
  taskbar, which the container has not).
* In play: AvP2's first Marine mission (`build/playtest-avp2-marine.txt`)
  in a 1024x576 window on a 1280x720 desktop, and Primal Hunt's first
  Corporate mission (`build/playtest-ph-corporate.txt`) in the borderless
  window at 1280x720: the menus clicked at their page places, the
  mission played, quicksaved and quickloaded, the HUD scaled (M-16).

Not done: a window manager under Xvfb (none in the build image), so
Alt+Tab back into a minimised full screen, and the window's frame as a
desktop draws it, were not seen; a full screen that fails when chosen in
the folder (Xvfb lists only its own resolution, which full screen takes).

### M-7: controllers through XInput

Retail reads one DirectInput joystick, with its axes set up on the
Joystick page (no radial dead zone, an Xbox pad's two triggers on one
axis, nothing for the menus). The modern build reads an XInput pad
instead, in both games. nolfsrc did this for NOLF (its M-P7-01..06 and
M-P7-09); its design and its engine-side device are this project
family's own code and were taken as the model, but every hook was made
against AvP2's and Primal Hunt's own code, which differs: the bindings
live in the player's profile per species, not in `defctrls.cfg`; the
movement is `PlayerMovement`/`CharacterMovement`, not `CMoveMgr`; the
mouse's look is axis offsets of 90 degrees of yaw and 45 of pitch; and
the folders are AvP2's (the Joystick page becomes the Controller page).

| Id | What | Files | Status |
|---|---|---|---|
| M-7 | `##gamepad`: the pad as the engine's joystick, read through XInput (xinput1_4, 1_3 or 9_1_0, loaded at run time) | `src/modern/engine/p7_gamepad.h`, `m7_padread.h`; `src/engine/lithtech/clientinput.cpp` (nolfsrc's four hooks, as imported) | done |
| M-7 | Default bindings, per species, in the profile's joystick column | `src/modern/cshell/m7_padbindings.h`; `ProfileMgr.cpp` | done |
| M-7 | Analog look (right stick) and movement (left stick) | `src/modern/cshell/m7_pad.h`; `GameClientShell.cpp` (`Update`, `SetMouseSensitivityScale`), `PlayerMovement.cpp`, `CharacterMovement.cpp` | done |
| M-7 | The menus with the pad | `m7_pad.h`; `GameClientShell.cpp` | done |
| M-7 | Controls, Controller (was Joystick) | `src/modern/cshell/m7_folderjoystick.h`; `FolderJoystick.cpp`; `ClientRes.rc`/`.h` (31100-31108; `IDS_JOYSTICK` "Controller", `IDS_DEVICE_JOYSTICK` "Pad") | done |
| M-7 | Customize Controls: a joystick button bound to a second command left the first (RETAIL_BUGS.md) | `FolderCustomControls.cpp` | done |

**The device.** `##gamepad` is an object behind `IDirectInputDevice2A`
that answers the engine's joystick calls from XInput's state (the first
connected pad of four; empty slots asked about at most once a second):
six absolute axes (`Left`/`Right Stick X`/`Y`, -1000..1000, Y down
positive; `Left`/`Right Trigger Axis`, 0..1000) and sixteen buttons (`A`,
`B`, `X`, `Y Button`, `Left`/`Right Bumper`, `Back`, `Start`,
`Left`/`Right Stick Click`, `D-Pad Up`/`Down`/`Left`/`Right`,
`Left`/`Right Trigger` past XInput's threshold), buffered in whatever
data format the engine sets, acquired only while the game has the
foreground. It is the only joystick: `clientinput.cpp` leaves
DirectInput's out (under Wine the same pad is one of them) and
enumerates no devices (M-P7-09's crash fix); `JoystickDisable` does not
apply to it. With `AVP2SRC_PADTEST` naming a file, the pad is that file's
state instead (`m7_padread.h`): the playtest's test controller.

**Bindings.** A profile keeps each command as `key@mouse@joystick`;
the joystick column is the pad's, bound whenever the pad is there
("Use joystick" was about DirectInput's joysticks and no longer gates
it; the profile's value is kept as it was, for retail). A profile
without `PadLayout` in `[Basic]` -- `Default.prf`, so Customize Controls'
Restore defaults too, and every profile made before -- gets the
defaults where its column is empty and the button is not taken, and is
saved with `PadLayout = 1`, so the player's own changes stay. Shared:
right trigger Fire, left trigger Alt-Fire, A Jump, left stick click Run,
right stick click Center View, Back Objectives / Scores, d-pad up Next
Vision Mode. Marine (and Primal Hunt's Corporate): B Duck, X Activate, Y
Reload, right/left bumper Next/Previous Weapon, d-pad down Shoulder
Lamp, left Flare, right Hacking Device. Predator: B Duck, X Activate, Y
Reload, bumpers the weapons, d-pad down Cloak, left Zoom Out, right
Zoom In. Alien (Predalien): B Duck, X Pounce, Y Wallwalk Toggle, left
bumper Wallwalk. Start is the menu (not bound). The buttons are bound in
Customize Controls like keys ("Pad A Button"); the sticks are never bound
as axes (`ApplyJoystick` no longer writes the retail axis bindings),
since the client shell reads them. The pad's device is enabled before
its buttons are bound (`ApplyBindings`).

**Look.** The right stick past a radial dead zone (`PadLookDeadZone`),
the rest of its travel rescaled to 0..1 and raised to `PadLookCurve`, is
a turn rate: 180 degrees a second across and 120 up and down, times
`PadLookSpeed` and the frame time, added to the mouse's axis offsets in
`PlayerMovement::UpdateRotation`/`UpdateAiming` (so it turns Primal
Hunt's turrets too, keeps to what multiplayer allows the mouse, and
looks whether or not mouse look is on). It slows with the Predator's
zoom as the mouse does (`SetMouseSensitivityScale`'s scale).
`PadInvertY` turns its pitch over; the mouse's invert applies on top.

**Movement.** The left stick past `PadMoveDeadZone` sets the movement
flags by octant (what the animations and the server see), and its
direction is the acceleration's (`CharacterMovement::UpdateAcceleration`,
client only); past three-quarters of its travel it runs, below that it
walks at that share of the walk speed, a third at least (the walking
state's speed cap). Run held turns that over; RunLock does not apply to
the stick.

**Menus.** Outside play the d-pad or the left stick is the arrow keys
(repeating after 0.4 s, every 0.12 s), A Enter, B and Start the menu key,
the right bumper Tab. In play Start is the menu key; A skips a cinematic
or a line of dialogue (the cinematic skip key) and goes on from a single
player death. The keys go to the client shell's `OnKeyDown`/`OnKeyUp`,
as the engine delivers a key; Customize Controls ignores keys while it
waits for a control, so pressing a pad button binds it.

**The Controller page** (Options, Controls; retail's Joystick page, the
same place and layout): Look speed (25-300%), Invert look, Look dead
zone and Move dead zone (0-50%), and Restore defaults (those, and the
pad's default bindings in place of its column of the profile).

**Checked** under Wine in the build container, both games, with the
test controller (`build/playtest.sh`'s `pad`/`tap`, the file
`AVP2SRC_PADTEST` names). No real or virtual pad could be used: Wine's
XInput is fed by its HID backends (udev/hidraw/evdev or SDL), which need
a kernel device, and the host's `/dev/uinput` is root's -- so the XInput
path was run only with no pad connected (the games run as before with
Wine's `xinput1_4.dll` there and no test pad); what follows the read of
the pad's state is the same code for both. `build/playtest-avp2-pad.txt` and `build/playtest-ph-pad.txt`: the
main menu with the d-pad, Single Player, Marine (Corporate) and Launch
with A, the intro cinematic skipped with A, then in play (a temporary
probe logged the player each frame; its lines were removed):

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
| Start, then B | the pause menu, then play (with a training message up, Start dismisses it first, as Escape does) | the same |
| d-pad down (Shoulder Lamp) | the lamp on (its meter shows) | the same |

at 60-150 frames a second. `build/playtest-pad-menus.txt`, both games:
Options, Controls with the d-pad and A; Customize Controls lists the
defaults ("Pad Right Trigger", ...); Fire bound to Y through the capture
(the profile then has `Fire = " @##3@Y Button"` and Reload's Y gone, in
every species); the Controller page's look speed a step up (saved:
`PadLookSpeed 1.25`); B back to the main menu.

Not done: rumble (nothing in either game asks for force feedback),
button prompts in hints, and a real pad (Windows, or Wine with a kernel
device). Found on the way, not the pad's: throwing a flare (the d-pad
left's Marine binding, or the keyboard's F) crashed the modern AvP2 and
Primal Hunt engines. Not a modern change and not a retail bug: the
legacy build crashed the same way and the retail binaries do not
(`build/playtest-avp2-flare.txt`, `playtest-ph-flare.txt`). It was a
reconstruction error, fixed in both builds: the client's message-object
helper (`clientmsghelper.cpp`, the `m_pObjectIO` of every message the
client reads) compiles a two-slot vtable, and the full-length table the
link puts in its place (`src/engine/lithtech/link/vtables-avp2.txt`,
`vtables-ph.txt`; `build/vtables/map-lithtech-*.txt`) had no entry for
it, so a projectile's `WeaponSoundFX` message, which reads a compressed
position (`ReadCompPos`, its slot 3, `GetPosRef`), called the next table
in `.rdata` instead (`LThread::ThreadTerm`) on it.

### Retail-bug fixes: the engine (M-B01..M-B09)

The bugs `RETAIL_BUGS.md` lists, fixed: each entry there names its id.
Every fix is an `AVP2SRC_MODERN` block with the retail code in its
`#else`, or modern-only lines the retail code does not have.

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-B01 | The texture sweep clears only its mark, not the texture's reference count | `src/engine/lithtech/clienttexmgr.cpp` (`tm_UnmarkTextures`) | from the code: the count's only writers are LTTexMod's Get/ReleaseTextureHandle, which neither game's DLLs call, so nothing in play reaches it; both games load and play their first missions as before (the flare scripts; the sweep runs at every load) | done |
| M-B02 | The command line: at most 150 arguments stored, from the real command line and from `-cmdfile`; a failed `-cmdfile` read frees its buffer | `src/engine/lithtech/clientwin.cpp` (`clu_dsi_BuildCommandLine`) | AvP2, `build/playtest.sh` to the main menu with `PLAYTEST_ARGS` of 70 `+x<n> 1` pairs (163 arguments in the command file): the legacy build crashes in `clu_dsi_BuildCommandLine` (it reads argument text as a pointer, then the stack's handler chain is overwritten); the modern build runs, the arguments past the 150th dropped | done |
| M-B03 | `ServerModelLT::SetCurAnimTime` overrides the interface's three-argument one, so a game's call flags the object for its clients | `src/engine/server/servermodel.h`, `servermodel.cpp` | the four modern engines' `ServerModelLT` tables (both games' `lithtech.exe` and `server.dll`): slot 33 is now `ServerModelLT::SetCurAnimTime`, and there is no slot 37; both games load and play their first missions (`build/playtest-avp2-flare.txt`, `playtest-ph-flare.txt`) | done |
| M-B04 | `CreateWorldCRC`'s world name always terminated | `src/engine/server/smgr.cpp` | from the code (a name of 255 characters or more cannot be made in play) | done |
| M-B05 | `sm_CreateInterObjectLink` checks its three allocations, and fails with `LT_ERROR` | `src/engine/server/interlink.cpp` | from the code (out of memory only) | done |
| M-B06 | The CD driver moves its play list on only when a track played to its end (`MCI_NOTIFY_SUCCESSFUL`), not when a play is superseded or stopped | `src/engine/cdaudio/cdaudio.cpp` (Primal Hunt's `cdaudio.dll`) | from the code: neither game in our tests plays CD audio (Wine has no Red Book drive; the music is DirectMusic and Miles), so the driver is not reached | done |
| M-B07 | The CD driver's `GetSongFromList` answers the song at the index | `src/engine/cdaudio/cdaudio.cpp` | from the code: neither game in our tests plays CD audio (Wine has no Red Book drive; the music is DirectMusic and Miles), so the driver is not reached | done |
| M-B08 | The CD driver forgets the current play list when it is removed | `src/engine/cdaudio/cdaudio.cpp` | from the code: neither game in our tests plays CD audio (Wine has no Red Book drive; the music is DirectMusic and Miles), so the driver is not reached | done |
| M-B09 | `sm_DetachClient` allocates the message it sends | `src/engine/lithtech/ccobj.cpp`, `src/engine/server/smgr.cpp` | from the code: only `ILTServer::AttachClient` gives a client a view client, and neither game's DLLs call it, so play never reaches the detach | done |

### Retail-bug fixes: the renderers (M-B20..M-B26)

The `RETAIL_BUGS.md` entries in `src/engine/d3d/ph/` (both games'
`d3d.ren`, unless the row says otherwise), each confirmed against the
retail code first. A pure addition (a test, a free) is an `#ifdef` alone;
anything else keeps retail's lines in the `#else`. One entry was not a
bug ("The model sun's variance reads the warble scale": the table is
ModelWarble's), and is not fixed.

| Id | What | Files | Status |
|---|---|---|---|
| M-B20 | A dynamic lightmap cleared all the way, not a quarter (RETAIL_BUGS.md: "A dynamic lightmap is cleared only a quarter of the way") | `lmdynamic.cpp` (`d3d_FillDynamicLM16`, `d3d_FillDynamicLM32`) | done |
| M-B21 | Primal Hunt's unsorted translucent world models drawn with their additive states ("Unsorted translucent world models lose their blend states before drawing") | `drawworldmodel.cpp` (`d3d_DrawNoSortWorldModel`) | done |
| M-B22 | The box test's +X, +Y and +Z planes at their faces ("The renderer's box test builds three of a box's six faces at the wrong place") | `fullintersectline.cpp` (`DO_PLANE_TEST_X/Y/Z`) | done |
| M-B23 | A lightmap page's bitmap freed when there is no lightmap format ("A lightmap page leaks its allocation bitmap ...") | `lmpaging.cpp` (`d3d_AddLMPage`) | done |
| M-B24 | `CD3DLMTexture::Init` fails when its surface cannot be made, not locking null ("A dynamic-lightmap texture whose surface cannot be made locks a null surface") | `d3d_optimizedsurface.cpp` | done |
| M-B25 | The models' specular pass keeps its stage-1 states over its draw ("The models' specular pass sets its texture-stage states and undoes them at once") | `drawmodel_env.cpp` (`CD3DModelDraw::DrawPasses`) | done |
| M-B26 | The screen-space and lightmap-only polygon draws test the vertex count against their 64-entry buffers ("A screen-space lightmap polygon is copied without checking its vertex count") | `drawlightmaps.cpp` (`d3d_DrawScreenLMPoly`, `d3d_DrawLightmapOnly`) | done |

Checked: the modern build compiles them (`make modern`) and both games
run on it (`build/playtest-avp2-marine.txt` and
`build/playtest-ph-corporate.txt`: the first mission of each loaded,
played, saved and reloaded, rendered as before). None of them can be seen in the shipped
games' ordinary play, so each rests on the code: M-B20 shows only on a
polygon with a lightmap page that is lit dynamically (`LMDrawDesc::Begin`
fills the buffer black otherwise, so the stale three quarters are
black); M-B21 on a Primal Hunt world model whose first surface is
unsorted; M-B22 is the plane `IntersectSegment` returns for an object's
box (n.p = d: a +X face at MaxBox.x has d = MaxBox.x, -X d = -MinBox.x,
which is `planeCoord * normalDirection` for all six); M-B23 and M-B24 need
a device with no lightmap format or a failing CreateSurface; M-B25 only a
model with `SpecularEnable` in its command string, which no shipped model
of either game has; M-B26 a polygon of more than 64 vertices.

### Retail-bug fixes: the client shells and the lithshared libraries (M-B40..M-B63)

Each a `RETAIL_BUGS.md` entry (its heading in the second column), fixed
behind `AVP2SRC_MODERN` with the retail code in the `#else`; the
libraries' fixes reach every DLL that links them (ButeMgr: the client
shells and object DLLs; GameSpyMgr: the object DLLs; GameSpyClientMgr and
LithTrackMgrClient: the client shells), in both games.

| Id | What (RETAIL_BUGS.md) | Files | Checked | Status |
|---|---|---|---|---|
| M-B40 | Setting an attribute that only an expansion tag holds does not take | `src/lithshared/ButeMgr/butemgr.cpp` (`GetSymTabItem`) | test program (below): AvP2 retail 2 after setting 20, modern 20 | done |
| M-B41 | The attribute tables leak on every Term | `butemgr.cpp` (`TraverseTableOfTagsDeleteSymTab`) | test program: retail +2749 bytes of heap a Parse/Term of three tags, modern +0 | done |
| M-B42 | Deleting a track key leaves a duplicate that is freed twice | `src/lithshared/lithtrackmgr/LithTrack.cpp` | code (the editor's key delete) | done |
| M-B43 | Deleting a track leaves a duplicate that is freed twice | `LithTrackList.cpp` | code | done |
| M-B44 | The track editor's "Font Type" command never changes the font | `LithTrackMgr.cpp` (`Ext_Type`) | code | done |
| M-B45 | The track editor's help text ignores the bar's width | `LithTrackBarHelp.cpp` | code | done |
| M-B46 | The track editor's draw object always reports failure | `LithTrackDrawObj.cpp` | code (its caller ignores the result) | done |
| M-B47 | A GameSpy secure query can overrun the server's buffer | `src/lithshared/gamespy/gamespymgr/GameSpyMgr.cpp` (`OnSecureQuery`) | code | done |
| M-B48 | The LAN server list searches an unterminated reply | `src/lithshared/gamespy/gamespyclientmgr/gserverlist.c` (`ServerListLANList`) | code | done |
| M-B49 | Sorting the server list by an unknown mode uses an uninitialised comparator | `gserverlist.c` (`ServerListSort`) | code | done |
| M-B50 | The server browser never checks or closes sierraup.cfg | `GameSpyClientMgr.cpp` (`SetupDirServers`) | code | done |
| M-B51 | Refreshing LAN servers removes other games' servers by the wrong port | `GameSpyClientMgr.cpp` (`RefreshLANServers`) | code | done |
| M-B52 | The "nelno" cheat's message isn't terminated | `src/AVP2/ClientShellDLL/CheatMgr.cpp` | code | done |
| M-B53 | Naming the console's actions leaks a string per command | `GameClientShell.cpp` (`OnEngineInitialized`) | code | done |
| M-B54 | The cloaked weapon's alpha floor writes the red channel | `WeaponModel.cpp` (`UpdateWeaponModel`) | code | done |
| M-B55 | A mounted turret with a full circle of yaw never wraps its yaw | `ClientTurretMgr.cpp` (`CClientTurret::Rotate`) | code | done |
| M-B56 | The turret recoil pushes the first-person weapon off by 0.95 on every axis | `CameraMgrFX.cpp` (`CameraMgrFX_RecoilUpdate`) | code | done |
| M-B57 | Putting the weapon away for a turret hides muzzle flashes it may not have | `WeaponModel.cpp` (`Holster`) | code | done |
| M-B58 | The host options take check-box commands past the end of their boxes | `FolderHostOptions.cpp` (`OnCommand`) | code | done |
| M-B59 | The host options' check boxes leak their bitmaps | `FolderHostOptions.cpp`, `.h` (`Term`, `DeleteBoxBitmaps`) | code | done |
| M-B60 | The server list's no-entry surface is never deleted | `FolderJoin.cpp` | code | done |
| M-B61 | The Corporate motion detector settings check the Marine's tag | `LayoutMgr.cpp` (the seventeen `GetMD...`/`...SurfaceName` functions) | code; Primal Hunt's layout has both tags, so nothing changes with its data | done |
| M-B62 | The Marine weapon chooser's dual pistols check has no null test | `WeaponChooser.cpp` (`IsDualPistols`) | code | done |
| M-B63 | A tracer whose Init fails removes three garbage flare handles | `TracerFX.h` (the constructor) | code | done |
| M-B108 | The folders' paging, `ForceMouseUpdate` and a group control's focus handed `OnMouseMove` the cursor's screen point (`CInterfaceMgr::GetCursorPos`) where it takes a page point; now the page's (with M-15) | `BaseFolder.cpp` (`NextPage`, `PreviousPage`, `ForceMouseUpdate`), `GroupCtrl.cpp` (`OnSelChange`) | from the code; at 640x480 the two are the same | done |

**Checked.** `make modern` builds both games with them. The ButeMgr
fixes with a test program (`cl` 8966 under Wine, linking each build's
`ButeMgrNoMFC.lib`): a buffer with `[Weapon]`, `[Weapon_EXPANSION]` and
`[Other_EXPANSION]` parsed, `SetInt` on each, `GetInt` back, then 100
`Term`s and `Parse`s with the CRT heap walked before and after. The
legacy library reads the expansion tag's old value (2 for 20) and holds
274880 bytes more after the 100; the modern one reads 20 and holds none
more. The rest fix crashes, leaks and bounds on paths that the retail
data and menus do not reach in a scripted run (a missing `sierraup.cfg`,
a LAN reply that fills its buffer, a weapon ID with no weapon, a turret's
yaw past a full turn, the track editor's deletes and font command, a
tracer whose Init fails, ...): each was checked from the code, and the
games still run (`make modern`, then AvP2's first Marine mission under
Wine). Primal Hunt's layout has both motion-detector tags, so M-B61
changes nothing with its data.

### Retail-bug fixes: the object DLLs and the shared code (M-B70..M-B85)

Fixes of `RETAIL_BUGS.md` entries in the object DLLs and the code they
share with the client (`src/AVP2/ObjectDLL/`, `src/AVP2/Shared/`), each
under the entry's heading there. Each is behind `AVP2SRC_MODERN` with the
retail code in the `#else`. Primal Hunt's unless it says otherwise (the
code is Primal Hunt's own).

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-B70 | `SVAR OldCloak` sends nothing when no message can be made, instead of writing through null | `ObjectDLL/CommandMgr.cpp` | code (`CreateMessage` fails only when the engine is out of messages) | done |
| M-B71 | The player-view fx cache writes and reads every type, not one past the last | `Shared/FXButeMgr.cpp` (`PVFX::Write`, `Read`) | code: no bute cache is ever written or read (`CGameButeMgr::WriteCache`/`ReadCache` return false first) | done |
| M-B72 | A cached projectile fx with class data of kind 0 (or an unknown kind) reads none, instead of reading through null | `Shared/FXButeMgr.cpp` (`PROJECTILEFX::Read`) | code (as M-B71) | done |
| M-B73 | A tracer without SingleSegment is multi-segment, not what its memory held | `Shared/FXButeMgr.cpp` (`TRACERFX::TRACERFX`) | code: every shipped tracer names SingleSegment | done |
| M-B74 | A scale fx's AdditiveAlpha starts cleared and is kept by the bute cache | `Shared/FXStructs.cpp` (`CScaleFX`) | code (as M-B71) | done |
| M-B75 | Inserting into a restriction list moves the entries up from the top and counts the new one | `Shared/MPRestrictList.h`, `Shared/ServerOptions.h`, `ObjectDLL/MPRestrictions.cpp` | code: the shipped bute files list the "_MP" sets and weapons in ID order, so only a custom one or a server's game info out of order reaches it. Host test (below): IDs 5, 1, 3, 2, 4 added give 1 2 4 (retail), 1 2 3 4 5 (modern) | done |
| M-B76 | The server's class and weapon "allowed" checks take only the list's entries | `ObjectDLL/MPRestrictions.cpp` | code: the callers' indices come from the Find functions | done |
| M-B77 | `CMPRestrictList::IsAllowed` takes only the list's entries (both games' shared header; the client's multiplayer class checks) | `Shared/MPRestrictList.h` | code (as M-B76); host test: `IsAllowed(32)` on a full list false | done |
| M-B78 | `CMPRestrictList::SetAllowed`, `GetId` and `GetName` take only the list's entries: the host options no longer write past a full list | `Shared/MPRestrictList.h` | host test: on a full list `GetId(32)` gave the count, 32 (retail), now -1, and `SetAllowed(32)` writes nothing | done |
| M-B79 | A cached string too long for its buffer is read past whole | `Shared/MultiFile.cpp` (`CMultiFile_FILE`, `CMultiFile_LITH::ReadString`) | host test: a 16-character string read into 8 bytes, then an int written as 42: retail reads 1094268983, modern 42, through both classes; no cache is read in play (as M-B71) | done |
| M-B80 | Losing a carried turret keeps the others | `ObjectDLL/PlayerObj.cpp` (`CPlayerObj::RemoveTurret`) | code: two turrets, the first lost, now leaves the second (it left none) | done |
| M-B81 | A fading fog volume keeps its red | `ObjectDLL/VolumetricFog.cpp` | code | done |
| M-B82 | A tentacle's attack and burrow each run their own update: an attack lasts its second, then the tentacle burrows | `ObjectDLL/Tentacle.cpp` | code | done |
| M-B83 | A turret gun with a full turn of yaw wraps it | `ObjectDLL/TurretGun.cpp` | code | done |
| M-B84 | A facehugger whose target is not a character no longer crashes its attack | `ObjectDLL/SimpleAIAlien.cpp` | code | done |
| M-B85 | A tracer without a Flare has no flares, not sprite names from leftover memory (found here) | `Shared/FXButeMgr.cpp` (`TRACERFX::TRACERFX`) | code: the Standard and Laser tracers have no Flare | done |

**Checked.** Both games' modern builds compile (`make modern`) and play
`build/playtest-ph-corporate.txt` to the end (AvP2's Marine and Primal
Hunt's Corporate first missions, quicksave and quickload; no crash). None of
these has an effect the shipped games show: the bute caches are never
read or written; every shipped tracer names SingleSegment; every
VolumetricFog in Primal Hunt's levels (EXA1S1, EXH2S1, EXP1S1, EXP2S1,
EXP2S2) has Use_Fog_Color off, so it is white and fades the same either
way; the tentacles (EXH2S1, EXP1S1), carried turrets and turret guns were
not reached by a script. Those are checked from the code. The restriction
lists and the cache's strings were run in a host test: the tree's
`MultiFile.cpp` and `MPRestrictList.h` built with g++ (stub types for the
engine's), with and without `AVP2SRC_MODERN`.

Not fixed, of these files' entries: "AvP2's player code casts its
attachments without testing the cast" (the cast cannot fail; the entry
says why).

### M-8: the launchers, and the game straight from AVP2.exe / PrimalHunt.exe

nolfsrc did the same for NOLF (`NOLF.exe` and `NOLFLauncher.exe`, one
launcher's objects with two entry points); AvP2's launchers are MFC
applications, so here it is one object, `src/modern/launcher/m8_entry.cpp`,
compiled twice, that tells the two programs apart (`g_bM8Direct`).

| Id | What | Files | Status |
|---|---|---|---|
| M-8 | `AVP2.exe` and `PrimalHunt.exe` start the game directly, as Launch does, and wait for it (their exit code is the game's); `AVP2Launcher.exe` and `PrimalHuntLauncher.exe` are the launchers. Neither needs the registry or a disc | `src/modern/launcher/` (`m8_entry.cpp`, `m8_launch.h`); `src/launcher/Launcher.cpp`, `LauncherDlg.cpp`, `LauncherDlg.h`, `DisplayMgr.cpp`; `build/build.mk` (`launcher_exe`, `launcher_modern`), `build/all.sh`; `build/game-scripts/linux-run.sh`, `windows-run.bat`, `windows-install.ps1`, `mac-install.command` | done |

**The direct start** goes through the launcher's own code, as its `-GOGAME`
does, without showing the dialog: `InitInstance` reads what it always reads
and calls `LaunchGame(ACTION_PLAY)`, which writes `avp2cmds.txt` and starts
`lithtech.exe -cmdfile avp2cmds.txt`. So the launcher's settings, where the
registry holds them (its Options, saved commands), apply to it, and its own
arguments go last on the game's command line, as the launcher's do. It
does not ask for a detail level (the game keeps its own settings) and
shows no dialog unless something is missing.

**Without the registry** (both programs): the program first makes its own
directory the current one. With no `InstallDir` the game is taken to be
there; with no launcher record only the game's own first run (autoexec.cfg's
`NumRuns`) brings up the detail dialog, not every Play. AvP2's 1.0.9.6
archive, which its patch names in the registry ("Update Command Line"), is
taken from the directory (`-rez AVP2P1.REZ`) when the registry does not
name one. Primal Hunt plays on the AvP2 beside it, `..\AVP2` (the layout the
build installs), if `AVP2.REZ` is there, and on the registry's AvP2
otherwise; its rez stack is `build/avp2cmds.sh`'s: AvP2's six archives from
AvP2's directory, then its own `AVP2P`, `AVP2P1`, `AVP2X`, `MULTI`,
`AVP2DLL`, `AVP2L` and `DIALOGUE`, a registry `PatchRez`, `custom`. (The
reconstructed retail code looks for `AVP2.rez` in Primal Hunt's own
directory and never names `AVP2X.REZ`: what the retail launcher does there
lies in the pages its protection wrapper keeps encrypted.) The launcher
still keeps its settings in the registry when the key is there; nothing
creates it.

**No disc check**: AvP2's Play no longer calls `CD_VerifyGame` (Primal
Hunt's never did). And the launcher's display code no longer maps
`d3d.ren` to `d3d9.ren` (a block that came in with nolfsrc's
`DisplayMgr.cpp`): this build has no `d3d9.ren`, and the mapping left the
configured renderer unfound.

**Checked** under Wine in the build container, `build/playtest.sh` with
`PLAYTEST_LAUNCH` and a new Wine prefix (no registry values):
`build/playtest-direct.txt` with `AVP2.exe +DisableMovies 1` and
`PrimalHunt.exe +DisableMovies 1`: each writes the command line above
(`... -rez custom -rez AVP2p.rez -rez AVP2p2.rez -rez AVP2P1.REZ ...
+DisableMovies 1` for AvP2; `-rez "Z:\...\AVP2\AVP2.REZ" ... -rez
DIALOGUE.REZ -rez custom ... +DisableMovies 1` for Primal Hunt) and the game
reaches its main menu (v1.0.9.6, v1.0.2.2F). `build/playtest-launcher.txt`
with `AVP2Launcher.exe` and `PrimalHuntLauncher.exe`: the launcher opens,
Display lists the renderer and the modes, its close button closes it
(M-B104), Play asks for the detail level (the game's first run), Low starts
the game (`++PerformanceConfig .DefaultLow`, M-B102; the same rez stack as
the direct start's) and it runs (AvP2 to its main menu, Primal Hunt
through its first splash screen in the time the script gives it). The
release build's launchers and command lines are unchanged: `make progress`
keeps every recorded function of `AVP2.exe`, `PrimalHunt.exe` and
`Launch.dll` (184, 187 and 28 identical, as before).

### Retail-bug fixes: the launchers and Launch.dll (M-B100..M-B107)

Each in `RETAIL_BUGS.md`; checked as the table says.

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-B100 | Launch.dll: an argument of 200 characters or more was copied into its 200-byte slot unterminated; now cut to 199 and terminated (the call sites' `strncpy` left as they were, redirected to one bounded copy) | `src/launch/launch.cpp` | a Windows harness (`zig cc`, Wine) calling `GetLithTechCommandLine` as `lithtech.exe` does (150 slots of 200 bytes) with a 250-character argument: retail's slot is not terminated (runs on into the next), the modern build's holds 199 characters and its terminator | done |
| M-B101 | Launch.dll (and the launchers' copy of the same code): every rez-file check leaked its `_findfirst` handle; now closed | `src/launch/RezFind.cpp`, `src/launcher/RezFind.cpp` | the same harness, with Launch.dll's `FindFirstFileA`/`FindClose` imports patched to counters, ten calls: retail 20 handles opened, none closed; modern 20 and 20 | done |
| M-B102 | The launchers' Low and High detail buttons gave the opposite profile | `src/launcher/LauncherDlg.cpp` | `build/playtest-launcher.txt`: Low writes `++PerformanceConfig .DefaultLow` | done |
| M-B103 | The launchers wrote the command line as `fprintf`'s format; now `fputs` | `src/launcher/LauncherDlg.cpp` | from the code (no `%` in the test's command line); the file the playtests read back is the command line as built | done |
| M-B104 | The display dialog's close button did nothing; now cancels | `src/launcher/DisplayDlg.cpp` | `build/playtest-launcher.txt`: a click on it returns to the launcher | done |
| M-B105 | A launcher string whose caption would not load lost the message; the caption's error now goes into the caption | `src/launcher/Launcher.cpp` | from the code: every caption the launchers pass is in their string tables, so the path is not reached in play | done |
| M-B106 | The launchers' already-running checks matched any `LITHTECH.EXE` or `SETUP.EXE`; now this directory's, by full path | `src/launcher/Launcher.cpp` | under Wine, with Wine's notepad running as `LITHTECH.EXE` from another directory: the release build's `AVP2.exe` does not open (and opens without it), the modern `AVP2Launcher.exe` opens; `ExistProcess` compares a name with a `\` against each process's full module path, so this directory's engine still stops it | done |
| M-B107 | The launchers copied the command line into 1024 bytes of stack (`strcpy`), whatever its length (new, `RETAIL_BUGS.md`); now written from the `CString` | `src/launcher/LauncherDlg.cpp` | from the code; the playtests' command files are whole | done |

### M-17..M-20: Internet multiplayer

The retail games played on the Internet through two services, both gone:
**WON** (Sierra's), which held the server list, the CD-key certificates,
the messages of the day and the version check, and **GameSpy**, whose
master (`master<n>.gamespy.com`) took the servers' heartbeats. In the
retail code (both games):

* A server not LAN-only registers with WON's directory
  (`InitSierraMultiplayer`, GameSpyMgr.lib: addresses from SierraUp.cfg,
  then auth servers, a certificate, `AddServiceOp` every 90 s) and sends
  GameSpy heartbeats to `master0.gamespy.com:27900` from its game port.
  Neither decides anything: a server with no WON never refuses a player.
* The browser (GameSpyClientMgr.lib) reads the Internet list from **WON's
  directory** (`/AliensVsPredator2`, Primal Hunt `/avp2primalhunt`), not
  from GameSpy's master, then queries each server itself (GameSpy's
  `\status\`, UDP). The Multiplayer Internet folder first wants a CD key
  (WON's `CDKey`, checked locally; `VALIDCDKEY` passes), then reads
  sierraup.cfg, the messages of the day (blocking, 30 s) and runs the
  version check; a failed check leaves Internet play off (`SetLANOnly`),
  and Find Internet Games first fetches a WON certificate
  (`SetupAuthentication`, blocking) and refuses with its error. Tested
  (the retail engine and game DLLs from `out/reference/`, in the
  container): without a key the folder's Host and Find links stay off;
  with `VALIDCDKEY` the game crashes reading the missing sierraup.cfg
  (M-B50). So a dead WON leaves the Internet half of the menus unusable;
  the LAN half works.
* Joining any server, the engine starts WON's peer authentication when
  the client shell hands it an auth context (`StartPeerAuthQ548`, packet
  13); the server answers it (`Sub48f570`) and marks the client in two
  fields nothing reads, and without an auth context of its own (a LAN
  server, or no WON) sends an empty answer. Nothing is enforced: a retail
  client with no certificate joins a server with none (tested: LAN).
  What does gate a join is the engine's file check (below).

The modern build, both games (nolfsrc's M-P6-02 and M-P6-13 were the
model -- the protocol notes, the `MasterServer` variable and its values,
`\secure\` answered first, the two-container test with a master stub --
and nolfsrc's files were read for it; AvP2's code is its own: its GameSpy
libraries are the real SDK's, which nolfsrc's stand-ins were not, so here
the retail protocol code is kept and pointed elsewhere, and the list comes
from GameSpy's master in place of WON's directory):

| Id | What | Files | Checked | Status |
|---|---|---|---|---|
| M-17 | The master server. `MasterServer` (client console variable, `+MasterServer host[:port]`; empty: `master.openspy.net`; `none`: no Internet list and no reports): the browser asks it for the list (the GameSpy SDK's own `ServerListUpdate2`: TCP 28900, `\secure\` answered with the game's key, `\list\cmp\`), with a 6 s limit on the connection and each read; a hosted game's server, in this process or `AVP2Serv.exe`/`AVP2XServ.exe` started from it, reads it from the environment (`AVP2_MASTERSERVER`), a dedicated server started by hand from `-MasterServer <host[:port]>`; the server resolves it once, heartbeats it every 90 s from the game port (retail's `\heartbeat\`, with `\gamename\`), answers its `\secure\` first in a reply (OnSecureQuery's `\validate\`, retail's), says `\statechanged\2` on shutdown, and prints `Master server: <name>` (or why none) to its console | `src/lithshared/gamespy/gamespymgr/GameSpyMgr.cpp`, `gamespyclientmgr/GameSpyClientMgr.cpp`, `gamespyclientmgr/gserverlist.c`; `src/AVP2/ObjectDLL/MultiplayerMgr.cpp` (`Setup`); `src/AVP2/ClientShellDLL/FolderMulti.cpp`, `FolderJoin.cpp`; `src/AVP2/AVP2Serv/ServerDlg.cpp` (`ParseCommandLine`); `src/modern/cshell/m17_master.h` | containers on one network, a local master (`masterstub.py`, below): both games' dedicated servers heartbeat it, answer `\secure\` with a `\validate\` it accepts (AvP2's key `Df3M6Z`, Primal Hunt's `P4fR9w`) and its `\status\` (name, map, players); each game's client lists its server through Find Internet Games and joins and plays; against master.openspy.net, a modern AvP2 server was accepted and listed (an independent list request, `gsprobe.py`, saw it). The `\statechanged\2` was not seen at the stub when the dedicated server was stopped (the engine's socket is closed by then): the master drops a silent server itself | done |
| M-18 | No WON and no CD key: the Multiplayer Internet folder goes straight to its links (no key dialog, no directory servers, messages of the day or version check), has no Update CD key link, and Find Internet Games fetches no certificate; the client shell hands the engine no auth context, so joining starts no WON peer authentication; servers register with WON no more (so never read SierraUp.cfg: RETAIL_BUGS.md) | `GameSpyClientMgr.cpp` (`Init`), `GameSpyMgr.cpp` (`Init`, `UpdateHeartbeat`); `FolderMulti.cpp` (`Render`, `OnCommand`, `OnFocus`) | the Internet folder (shot), the joins above, both games | done |
| M-19 | Other players move smoothly. Each client sent its position 4 (standing), 8 (moving) or 12 (in the air) times a second (`ClientUpdateLowSendRate` 0.25, `ClientUpdateSendRate` 0.125, `ClientUpdateHighSendRate` 0.08 s) and asked for 6 world updates a second (the first run's `UpdateRate`; Player setup's connection speeds 4, 6, 9, 13, 19); and a client's rate is held to what the server's bandwidth allows, where a new server's default (16000, the host options' 56k) allows only the slowest, 4: every update moved another player a step prediction had not foreseen. Now all three send rates are 0.03 s (a message only when something changed, so about 30 a second while moving), the first run's `UpdateRate` is 30 and an older config's lower one is raised once (`UpdateRate30`), Player setup's fastest choice (T3) is 30 (the four others are retail's) and an older profile is raised to it once (`Connection30`), a new server's bandwidth is the host options' top choice (10000000) and an older config saved with retail's default is raised once (`Bandwidth30`; any other choice stays), and that top choice allows the top rate (retail capped every server at the fourth). The host's choices of bandwidth and the player's of speed are all kept | `src/AVP2/Shared/PlayerMovement.cpp` (the send rates), `src/AVP2/ClientShellDLL/GameClientShell.cpp` (the first run's `UpdateRate`, the raise), `ProfileMgr.cpp` (`kConnectSpeeds`, the profile's raise, the server bandwidth's cap), `src/AVP2/Shared/MultiplayerMgrDefs.h` (`DEFAULT_BANDWIDTH`), `ServerOptions.cpp` (the config's raise); `src/modern/cshell/m19_netposlog.h` (the measuring aid, debug build only) | measured with the modern debug build (`NetPosLog 1`: the watching client logs each frame where it sees the other player), a dedicated server, one client running in circles for 47 s, another watching at about 70 frames a second: with retail's rates (send rates 0.25/0.125/0.08, the server at its old default bandwidth, so 4 updates a second) the runner's frame steps were more than twice their median 7.9 times a second (0.6% of frames without movement); with the new defaults 0.3 times a second (0.3%). The watcher's config, retail's `UPDATERATE 6`, came out of the run as 30 | done |
| M-20 | The dedicated servers' console: the game's text (chat, names, the engine's lines) and the typed command no longer passed as `printf` formats; a line cut to 497 characters and terminated; every line also in `server-console.log` in the game directory (timed; emptied by each run's first line) | `ServerDlg.cpp` (`WriteConsoleString`, `UpdateUI`, `OnConsoleSend`), `src/modern/server/m20_serverlog.h` | a player chatting `hello from the runner %n%s%n%s`: retail's AVP2Serv.exe crashed (page fault in its `_vsnprintf`), the modern one printed the text; the logs of every test above | done |

**What else was found and put right (not modern-only).** Each is a
reconstruction error, so it is fixed in both builds:

* **Primal Hunt's GameSpy name and key.** `MultiplayerMgrDefs.h` gave both
  games AvP2's (`avp2`, `Df3M6Z`); retail Primal Hunt's PHObject.lto and
  PHCShell.dll hold `avp2ph` and `P4fR9w`, which OpenSpy knows. Primal
  Hunt's servers and browser now use them (`#elif defined(PRIMAL_HUNT)`).
* **Multiplayer prediction crashed our engines.** `predtick.cpp` (the
  client's per-frame prediction of other objects, which only multiplayer
  runs) read CClientMgr and LTObject with NOLF's layout: the time at
  0x125c (AvP2 0x136c), the interfaces at 0x4c0/0x61c (0x4c4/0x620), the
  shell's OnObjectMove/OnObjectRotate two slots early, and every
  prediction field from the friction on 4 bytes low. Every client of
  ours crashed at its first frame in a server's world (page fault at
  0x004ab9dd, `sub_461270Q1S`); the views now have AvP2's places, as
  predobj.cpp's already did and AvP2 1.0.9.6's own (0x0046e250) reads
  them: the function's displacements are now the set retail's has.
* **AvP2's dedicated server found none of our game DLLs.** AVP2Serv.exe
  looks for `custom.rez`, not the `custom/` directory a game directory
  keeps them in, and stopped ("Unable to load the game binaries");
  `build/all.sh` now writes `customrez.txt` (`Rez0 = "custom"`), the file
  it reads for more archives.

**Still open.** Our `server.dll` (the dedicated server's engine) links
with two unresolved references and stops with "pure virtual function
call" when a world loads; the tests ran the dedicated servers on the
retail `server.dll` (with our AVP2Serv.exe/AVP2XServ.exe and game DLLs).
A Primal Hunt server on `DM_Compound` refuses a Primal Hunt client
("world version is different"): the world is in AvP2's MULTI.REZ and
AVP2P1.REZ, and the server's archive order differs from the client's;
the Primal Hunt tests used `DM_Aizin`.

**Retail players.** The engine compares the server's CRC of `CShell.dll`
and the attribute files with the client's (`SetCRCString`,
`DoLoadWorld`), and our `CShell.dll` is not retail's: a retail client
joining a modern server is disconnected with "Some game resources differ
from the server" (tested), and a modern client would be by a retail
server. The handshake is otherwise retail's, so the two would play if
the modern build reported retail's CRC; that is a decision, not made
here (nolfsrc's M-P6-04 found the same). The master server is shared:
a modern server is in the list a retail-patched browser reads, and the
other way round, under the same game names.

**The test** (`out/mp/`, not kept in the tree): `dock.sh` runs the build
image on a docker network; `srv.sh` a dedicated server under Xvfb and
Wine from a game directory (a config in ServerData/, `-config`; Primal
Hunt's install key for AvP2's archives added to the Wine prefix for the
run and removed after); `masterstub.py` the master (UDP 27900, TCP
28900, logging each heartbeat, challenge, validate, status reply and
list); the clients are `build/playtest.sh` with scripts (Multiplayer
LAN or Internet, Find, the first server, Join, Space, chat, run) and
`PLAYTEST_CFG` for `MasterServer`; `gsprobe.py` one list request to a
master.

## New console variables

| Variable | Type | Default | What | Id |
|---|---|---|---|---|
| `WindowMode` | int | (none: a first run, then 2) | with `Windowed` 1: 2 a borderless window at the desktop's resolution, otherwise a window with a title bar; Options, Graphics, "Window mode"; saved in autoexec.cfg. A config without it starts borderless unless `Windowed` is 1 | M-9 |
| `Windowed` | int | (engine) | the engine's existing variable (what both renderers read when they start), now set by "Window mode" too: 0 full screen, 1 a window (either kind); the mode is set again when it changes | M-9 |
| `MenuScale` | int | 100 | the menus' page, percent of filling the screen's height (0: retail's size; never below it, never wider than the screen); Options, Graphics, "Menu size"; saved in autoexec.cfg | M-15 |
| `HUDScale` | int | 75 | the HUD's scale, percent of the screen's height over 480 (0: retail's size; never below it); Options, Graphics, "HUD size"; saved in autoexec.cfg | M-16 |
| `MasterServer` | string | (empty = `master.openspy.net`) | the GameSpy master server the Internet list is read from and a hosted Internet game reports to (host or host:port; `none`: neither); `+MasterServer`, saved in autoexec.cfg; a dedicated server's is `-MasterServer` | M-17 |
| `UpdateRate30`, `Connection30` (profile), `Bandwidth30` (server config) | flags | -- | M-19's one-time raises done | M-19 |
| `NetPosLog` | int | 0 | (modern debug build only) 1: each multiplayer frame, where the other characters are, to netpos.log | M-19 |
| `FieldOfView` | int | 90 | the player's horizontal field of view at 4:3, degrees (60..130; every character's scaled with it; a wide screen widens it again); Options, Graphics, "Field of view" (75..120); saved in autoexec.cfg | M-3 |
| `RestrictCinematicsTo4x3` | int | 0 | 1: black beside the centred 4:3 area while a cinematic plays; Options, Graphics, "Cinematics" (Widescreen, 4:3); saved in autoexec.cfg | M-2 |
| `FramerateLimit` | int | 240 | the client's frames a second at most (0: no limit), single player and multiplayer, not while loading; Options, Graphics, "Frame rate limit" (60, 120, 144, 240, None); saved in autoexec.cfg | M-13 |
| `PadLookSpeed` | float | 1 | the controller's right stick: turn rate, times 180 degrees a second across and 120 up and down; Controller, "Look speed" | M-7 |
| `PadInvertY` | int | 0 | the right stick's up looks down; Controller, "Invert look" | M-7 |
| `PadLookCurve` | float | 2 | the right stick's response: its deflection past the dead zone raised to this (1: linear) | M-7 |
| `PadLookDeadZone` | float | 0.15 | the right stick's radial dead zone (0..0.9 of its travel); Controller | M-7 |
| `PadMoveDeadZone` | float | 0.2 | the left stick's radial dead zone; Controller | M-7 |
| `OldMouseLook` | int | 0 | the retail mouse look (the engine's DirectInput offsets, `CursorCenter 1`) in place of raw input; Controls, Mouse, "Legacy mouse look"; saved in autoexec.cfg | M-6 |
| `NoRawInput` | int | 0 | the new mouse look from the cursor's motion (put back in the window's middle each frame) instead of raw input; saved in autoexec.cfg | M-6 |
