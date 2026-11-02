# The modern build

`AVP2_MODERN` is the project's define for everything that departs from the
retail game: bug fixes (`RETAIL_BUGS.md`), new features, and open-source
middleware in place of the proprietary libraries. It is set only in the
modern build (`make modern`, `make modern-debug`; `build/variant.sh`),
never in the legacy build, which stays the retail game as far as it can be
rebuilt and is the only build `make progress` scores. Every change goes
behind `#ifdef AVP2_MODERN` with the retail code kept, unchanged, in the
`#else`. (`AVP2_DEBUG` is the debug builds' logging and checks, independent
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

## Planned

To bring in once the game runs (each checked against AvP2's code first;
`docs/PLAN.md`, package 12):

* widescreen: menus, HUD and field of view at any aspect ratio, with an
  option to keep the cinematics at 4:3;
* a field-of-view setting;
* up to 16x anisotropic filtering and up to 8x antialiasing, by way of
  nolfsrc's modern Direct3D 9 renderer;
* raw mouse input;
* controllers through XInput, in place of DirectInput joysticks;
* OpenMiles in place of Miles (`mss32.dll`), our Bink player on FFmpeg in
  place of RAD's `binkw32.dll`, as nolfsrc's, and an open replacement for
  Smacker's `Smackw32.dll`;
* the launchers as `AVP2Launcher.exe` and `PrimalHuntLauncher.exe`, with
  `AVP2.exe` and `PrimalHunt.exe` starting the game directly, as if Launch
  had been clicked in the launcher;
* from nolfsrc's modern build, where AvP2 has the same code: windowed modes
  at any size, a frame limiter, Internet play without the dead GameSpy and
  WON master servers.

## Done

(nothing yet)
