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

**Not implemented yet.** The define exists and the modern builds are made
(`make modern` builds `out/avp2-modern-release/`, `make modern-debug`
`out/avp2-modern-debug/`, both games each), but none of the items below is
done, and the retail middleware (Bink, Smacker, Miles) is still beside them.
What the modern build does compile differently today is only what came in
with nolfsrc's code: the `AVP2SRC_MODERN` blocks of its engine
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
| M-9 | From nolfsrc's modern build, where AvP2 has the same code: windowed modes at any size, a frame limiter, Internet play without the dead GameSpy and WON master servers | not implemented yet |

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

(nothing yet)
