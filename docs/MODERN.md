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
  toolchain (the VC6 revisions of `progress/COMPILERS.md`).
* **No disc check.** The modern build drops the launcher's protection
  wrappers and the games' own disc and CD-key checks.

## Where it stands

**Begun.** The define exists and the modern builds are made
(`make modern` builds `out/avp2-modern-release/`, `make modern-debug`
`out/avp2-modern-debug/`, both games each). The frame timing is done
(M-10..M-14, under Done); the rest of the items below are not, and the
retail middleware (Bink, Smacker, Miles) is still beside them.

A variant build makes its own engines (`lithtech.exe`, `server.dll`,
`d3d.ren`, both games'): `build/all.sh` builds `engine-link`, `d3d-link`
and `avp2-d3d-link` in a `build/variant.sh` tree. Before M-10 it did not,
and the modern build shipped the release build's engines, without any of
the engine's modern blocks. Compiling them showed one that needs a file
of nolfsrc's: `clientinput.cpp`'s M-P7-01 and M-P7-09 blocks call
nolfsrc's `##gamepad` (XInput); `src/modern/engine/p7_gamepad.h` stands in
for it until M-7, with no pad, so the modern engine leaves DirectInput's
joysticks out and looks for the keyboard and mouse only (M-P7-09's crash
fix), as those blocks do.

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
| M-7 | Controllers through XInput, in place of DirectInput joysticks | not implemented yet |
| M-8 | The launchers as `AVP2Launcher.exe` and `PrimalHuntLauncher.exe`, with `AVP2.exe` and `PrimalHunt.exe` starting the game directly, as if Launch had been clicked in the launcher (no registry, no disc check) | not implemented yet |
| M-9 | From nolfsrc's modern build, where AvP2 has the same code: windowed modes at any size, Internet play without the dead GameSpy and WON master servers (its frame limiter is M-13) | not implemented yet |

## Open middleware

The proprietary libraries, replaced by open-source ones in the modern build
only (the legacy build keeps the retail DLLs, from the player's own discs).
Not implemented yet: the modern build still takes all three from the retail
games (`build/all.sh`, step 2).

| Id | Retail | Replacement | Status |
|---|---|---|---|
| M-X1 | RAD's Bink, `binkw32.dll` (the intro movies) | our Bink player on FFmpeg, as nolfsrc's and nolf2src's | not implemented yet |
| M-X2 | RAD's Smacker, `Smackw32.dll` | an open Smacker decoder (FFmpeg's) behind the same exports | not implemented yet |
| M-X3 | Miles, `mss32.dll` (with its MP3 decoder `MP3DEC.ASI`) | OpenMiles, as nolfsrc's | not implemented yet |

## Done

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

## New console variables

| Variable | Type | Default | What | Id |
|---|---|---|---|---|
| `FramerateLimit` | int | 240 | the client's frames a second at most (0: no limit), single player and multiplayer, not while loading; Options, Graphics, "Frame rate limit" (60, 120, 144, 240, None); saved in autoexec.cfg | M-13 |
