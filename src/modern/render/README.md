# src/modern/render/ -- the modern build's Direct3D 9 renderer

`d3d9.ren` is a render DLL that the engine can load in place of `d3d.ren`
(docs/MODERN.md, M-29). It draws with the fixed-function pipeline of
Direct3D 9. Proton runs Direct3D 9 on Vulkan (DXVK), and Wine runs it on
OpenGL (wined3d). The reasons for it:

- Modern drivers limit the render targets of Direct3D 7. At 2560x1440 our
  `d3d.ren` (DirectX 7) cannot make its device (GitHub issue #6).
  Direct3D 9 has no such limit.
- Under Proton, the Steam overlay shows with `d3d.ren` only with its
  retail flip, which locks the frame rate to the refresh rate (M-24,
  GitHub issue #2). Under DXVK the overlay shows at all frame rates.
- Real anisotropic filtering (M-4) and multisampled antialiasing (M-5).

## Where it comes from

The code is nolfsrc's renderer (`src/modern/render/` of nolfsrc, for No
One Lives Forever, whose engine is a close relative of this one), changed
to the engine of AvP2 and Primal Hunt. The reference for each change is
our own `d3d.ren` source, `src/engine/d3d/ph/`: the contract with the
engine (the `RenderStruct`, `ltrender.h`), the data, and what `d3d.ren`
draws. The core draws through `rdev.h`, a small fixed-function device in
the shape of OpenGL 1.x (state, texture units with combiners, matrix
stacks, immediate primitives). `d3d9/` implements `rdev.h` on Direct3D 9.
(nolfsrc also has an OpenGL backend; this port does not have it.)

`build/ext.sh` builds the DLL with `zig c++` (no VC6: nothing here has to
match a retail binary) into `out/ext/d3d9.ren`, and a copy with the debug
checks (`AVP2SRC_DEBUG`) into `out/ext/debug/d3d9.ren`. `build/all.sh`
installs it in the two game directories of the modern builds, beside
`d3d.ren` (the modern-debug build installs the debug copy).

## Using it

`d3d.ren` stays the default renderer. To use `d3d9.ren`, give the engine
`+RenderDll d3d9.ren +CardDesc Direct3D9`, or put these lines in
`autoexec.cfg`:

    "RenderDll" "d3d9.ren"
    "CardDesc" "Direct3D9"

A playtest:

    PLAYTEST_ARGS="+RenderDll d3d9.ren +CardDesc Direct3D9" \
        build/playtest.sh out/avp2-modern-release avp2 -

`RenderCommand info` prints the device in use. The console variables are
the names and defaults of `d3d.ren`, where it has the variable, and two of
ours:

- `VSync` (0): wait for the display's refresh at each frame. `d3d.ren`
  also reads `VSync` (M-24), with the default 1. The renderer that starts
  first in a session makes the variable.
- `Antialiasing` (0): samples for each pixel (0 off, 2, 4 or 8). The
  renderer draws into a multisampled target and resolves it before a flip
  or a read.

`RenderDebug` 3 and above also writes `d3d9.log` beside the game.

## The window modes

The renderer makes a windowed Direct3D 9 device on the engine's window.
For full screen it first changes the display mode
(`ChangeDisplaySettings`) and makes the window cover the display. The game
then puts the window right for the mode (M-9, `m9_window.h`): a window
with a title bar, or a borderless window at the size of the desktop. The
frame-rate limit (M-13) is the game's.

## Threads

The engine calls the renderer from more than one thread. While a level
loads, the loading screen draws whole frames from a thread of its own
(`LoadingScreen.cpp`, `CLoadingScreen::RunThread`), and the main thread
binds textures and makes surfaces at the same time. Thus each way into the
renderer (each `RenderStruct` slot, the `FillScreenRect` hook, and the
`Blt` and `BltFast` of the movie stand-ins) holds one recursive lock for
all of its call (`MRLock`, `mr.h`). The device is made with
`D3DCREATE_MULTITHREADED`. A new way in must hold the lock too.

## Debug checks

In the copy with `AVP2SRC_DEBUG`:

- `MR_CHECK` reports a bad request that the code then refuses or clips (a
  rectangle partly outside a texture or the screen), one time for each
  place in the code.
- `MR_ASSERT` reports what must never get to the device (a lock or a draw
  out of the bounds of its buffer, a draw without the lock), and then
  stops the game with an illegal instruction.
- The lock reports each time a thread must wait for another thread.

A report goes to the console, to stderr and to `d3d9.log`.

## Status

| stage | what | state |
|---|---|---|
| 1 | the device and 2D: the device on the engine's window (full screen, a window, a borderless window), the mode list, clear and present, the engine's surfaces and every blit, Optimized 2D, the screen lock, `FillScreenRect`, `BlitFromScreen`, screenshots, DTX textures, the movies (DirectDraw stand-ins), VSync | done: both games to the menu, the menus' 2D, the console, F8, the intro movies, under Xvfb and Wine (wined3d), in each window mode and at 2560x1440 and 3840x2160. The screen lock keeps a shadow of the screen (`mr_SyncScreen`): the text of the engine locks the screen for each string. The menus' 3D parts are stage 3 |
| 2 | the world: polygons, textures, lightmaps, visibility, world models, the sky, fog, mirrors and portals | done but for a check of the mirrors (no test level has one yet): the engine's visibility query; the polys as `d3d.ren` draws them with its default two passes (a lightmap, or the sky-pan texture times the vertex colours, then the texture over it, two times the product under `Saturate`; a fullbright texture a third time; `Force1Pass` one pass), vertex-lit polys through the world colour's gamma ramp, the vertex light animations, the light animations into lightmap atlases (32-bit runs, shadow-map masks, the layers added by their brightness), dynamic lights into the lightmaps and as small lightmaps added over the other polys (`LMDynamic`), detail textures and the world's environment maps; world models (solid, chromakey, translucent back to front with `WMAmbient`); the sky from its squashed camera, its polys lit by the lights that reach them; `d3d.ren`'s fog for each vertex (by distance, AvP2's vertical fog with its retail cut, Primal Hunt's fog volumes), drawn as pre-transformed vertices, whose specular alpha Direct3D 9 takes as the fog; the light scale (`InvertHack`) and light add passes; mirrors through the stencil. The first Marine and Predator levels of AvP2 and the first Corporate level of Primal Hunt match `d3d.ren` (out/play/d3d9-s2*, d3d7-s2*), but for the objects (stage 3) and the screens of two monitors in the Marine level |
| 3 | objects: models, sprites, particles, poly grids, line systems, canvases, dynamic lights, shadows | not started |
| 4 | anisotropic filtering and antialiasing in the Display menus | not started |

Not yet confirmed on Windows or under Proton.

## Files

| file | what |
|---|---|
| `ltrender.h` | the engine <-> renderer contract of AvP2: `RenderStruct` (15 engine callbacks, 38 renderer slots), `RenderStructInit`, `RMode`, `PFormat`, `RenderBlitRequest`, `SharedTexture`, `TextureData`; every size checked at compile time |
| `mr.h` | the core's own globals and declarations |
| `rdev.h` | the device under the core, which a backend implements |
| `mr_main.cpp` | exports, Init/Term, modes, clear and flip, console variables, the small slots |
| `mrplatform.h`, `mr_window.cpp` | display modes and the size of the engine's window (user32) |
| `mr_2d.cpp` | surfaces, blits, Optimized 2D, the screen lock |
| `mr_texture.cpp` | textures |
| `ltworld.h` | the engine's scene, world and object data of AvP2: `SceneDesc` (0x104 bytes), `MainWorld`, `WorldBsp`, `Node`, `Leaf`, `Surface`, `WorldPoly`, `LightAnim`, `LTObject` (0x1b0), the visibility query |
| `mrscene.h`, `mr_scene.cpp` | the world's slots, the camera, the fog, the world's polys, world models, the sky, mirrors, `RenderScene` |
| `mr_lightmap.cpp` | lightmaps: the light animations composed into atlases, dynamic lights into them |
| `mrobjects.h`, `mr_objects.cpp` | the visible set: objects by kind and the order they are drawn in |
| `mr_movie.cpp` | DirectDraw stand-ins for the movie players |
| `d3d9/d3d9_device.cpp` | `rdev.h` on Direct3D 9 |
| `d3d9/d3d9.def`, `d3d9/d3d9.rc` | the DLL's three exports; the launcher's name for it (string 5) |
