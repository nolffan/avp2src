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
the music through Microsoft's DirectMusic (M-X4), and controllers through
XInput (M-7) (under Done); the rest of the items below are not.

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
some assume nolfsrc's modern files (the engine and the launcher name
`d3d9.ren`, nolfsrc's Direct3D 9 renderer, which is not built here): each
is to be checked, kept or dropped, as the items land.

Each item's id is a series and a number (M-1, ..., and M-X for the
middleware, as nolf2src's); an id is never reused.

## Planned

To bring in once the games run (each checked against AvP2's and Primal
Hunt's code first; `docs/PLAN.md`, package 12). Every one: not implemented
yet.

| Id | What | Status |
|---|---|---|
| M-1 | Widescreen: menus, HUD and field of view at any aspect ratio | not implemented yet |
| M-2 | An option to keep the cinematics at 4:3 (pillarboxed) on a wide screen | not implemented yet |
| M-3 | A field-of-view setting | not implemented yet |
| M-4 | Up to 16x anisotropic filtering, by way of nolfsrc's modern Direct3D 9 renderer | not implemented yet |
| M-5 | Up to 8x antialiasing (the same renderer) | not implemented yet |
| M-6 | Raw mouse input (WM_INPUT), in place of DirectInput's mouse | not implemented yet |
| M-7 | Controllers through XInput, in place of DirectInput joysticks | done (under Done) |
| M-8 | The launchers as `AVP2Launcher.exe` and `PrimalHuntLauncher.exe`, with `AVP2.exe` and `PrimalHunt.exe` starting the game directly, as if Launch had been clicked in the launcher (no registry, no disc check) | not implemented yet |
| M-9 | From nolfsrc's modern build, where AvP2 has the same code: windowed modes at any size, Internet play without the dead GameSpy and WON master servers (its frame limiter is M-13) | not implemented yet |

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
left's Marine binding, or the keyboard's F) crashes the modern AvP2 and
Primal Hunt engines in `lmsg_ReadCompPos` (servermsg.cpp), to be looked
into separately.

## New console variables

| Variable | Type | Default | What | Id |
|---|---|---|---|---|
| `FramerateLimit` | int | 240 | the client's frames a second at most (0: no limit), single player and multiplayer, not while loading; Options, Graphics, "Frame rate limit" (60, 120, 144, 240, None); saved in autoexec.cfg | M-13 |
| `PadLookSpeed` | float | 1 | the controller's right stick: turn rate, times 180 degrees a second across and 120 up and down; Controller, "Look speed" | M-7 |
| `PadInvertY` | int | 0 | the right stick's up looks down; Controller, "Invert look" | M-7 |
| `PadLookCurve` | float | 2 | the right stick's response: its deflection past the dead zone raised to this (1: linear) | M-7 |
| `PadLookDeadZone` | float | 0.15 | the right stick's radial dead zone (0..0.9 of its travel); Controller | M-7 |
| `PadMoveDeadZone` | float | 0.2 | the left stick's radial dead zone; Controller | M-7 |
