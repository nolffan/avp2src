# AvP2 1.0.9.6's d3d.ren: its objects, in link order, against Primal Hunt's

The machine-readable map is `progress/avp2d3d-objects.csv`, and the function pairs with Primal Hunt are `progress/avp2d3d-pairs.csv`. The `out/plan/` scripts this cites are working files, not kept in the repository.

Analysis only (nothing in `src/`, `build/`, `progress/` changed). Scope: the
app range of `progress/targets.py`'s "AvP2 d3d.ren", 0x10001000..0x1003b710
of `out/reference/avp2/d3d.ren` (1,281 enumerated rows, 1,271 functions once
the enumeration's nine extent defects are folded). Companions:

| file | contents |
|---|---|
| `out/plan/avp2d3d-objects.csv` | one row per object: `start,end,object_guess,flags_guess,ph_counterpart,evidence` |
| `out/plan/avp2d3d-pairs.csv` | one row per AvP2 function: `avp2_addr,size,ph_addr,ph_symbol,ph_file,evidence,confidence` |
| `out/plan/avp2d3d/` | the scripts (order below), `a2view.txt` (the whole row alignment, AvP2 beside PH), `a2-tails.txt` (extent defects), `ph-rows.tsv` / `a2-rows.tsv` (the scoreboard's rows) |

## 1. The compiler is NOT Intel C/C++: it is Microsoft cl 12.00.8168

`targets.py` (`icl=True`), `progress/COMPILERS.md` and the brief call this
renderer Intel C/C++ 4.0 "as NOLF's". The binary says otherwise, four ways:

* **No Intel padding.** NOLF's (Intel) `d3d.ren` holds 795 lea-nop pads
  (`8d b4 26 00 00 00 00` x213, `8d bc 27 00 00 00 00` x368, `8d 76 00` x214).
  AvP2's holds **none**, nor Primal Hunt's. AvP2's functions are either
  16-aligned with **0x90** fill (VC6 `/O2` COMDATs) or packed (VC6 `/O1`), with
  **0xCC** linker fill at 12 `/O1`->`/O2` object boundaries -- exactly PH's
  (MSVC) shapes, and no four-byte `/Qip` register shims.
* **The Rich header records the objects.** NOLF's has a `(0, 0) x80` row --
  80 objects with no `@comp.id`, its Intel objects. AvP2's has **none**: every
  object is a Microsoft tool's, and the 73 C++ objects are **cl 12.00.8168**
  (VC6 RTM), the compiler of `AVP2Serv.exe` and of `CShell.dll`'s bulk.
* **Byte identity with an MSVC build.** 1,180 of AvP2's 1,255 renderer
  functions (177,925 of 236,166 bytes, 75 %) are byte-identical, relocations
  masked, to their Primal Hunt counterparts (MSVC 8966) -- impossible across
  Intel/MSVC.
* **Test compile.** `ph/*.cpp` compiled with **msvc-6.00-8168** at each file's
  PH flags and *without* `PRIMAL_HUNT` (`a2ccall.sh`): 1,200 of the objects'
  1,397 functions land identical on an AvP2 function (1,181 on PH). The
  8966-built `ph/` objects score the other way round: 1,216 on PH, 1,170 on
  AvP2. Per file the two compilers swap exactly where codegen changed between
  RTM and SP5: fullintersectline 16 AvP2 / 9 PH with 8168 against 9 / 16 with
  8966; lmsurfaces 11/7 against 7/11; drawobjects 22/18 against 18/22;
  drawmodel 105/102 against 102/105; pixelformat 72/70 against 70/72.

So "the Intel flag group" of every object is moot: each AvP2 object is a VC6
object, and its flags are the same `/O1` or `/O2` decision PH's map measured.
`targets.py` should lose `icl=True` (not changed here: out of scope).

## 2. The unidentified Rich-header rows

AvP2's Rich header: Linker 6.00.8168, C++ 8168 x73, C 8168 x120 (libcmt),
masm 6.13 x37, implib 5.12.8034 x6, Import0 x91, cvtres 5.00.1720, aliasobj 7291,
and the two rows to identify. NOLF's renderer has the same two rows.

* **tool 0x25 build 8755 x3 = the DirectX 8.0 SDK's `ddraw.lib` import objects.**
  Four DLLs are imported (DDRAW, WINMM, KERNEL32, USER32): 4 x (descriptor +
  thunk terminator) + one shared NULL descriptor = 9 import objects, and the
  header has 6 + 3. msvc-6.00-8168's `kernel32.lib`, `user32.lib` and
  `winmm.lib` each carry 3 objects of implib 8034 (two of each are linked: 6).
  The remaining three are DDRAW's. Primal Hunt has the same structure with the
  DirectX **8.1** SDK's `ddraw.lib`, whose three objects are `(25, 9210)`
  (`ext/toolchains/dxsdk-8.1/lib/ddraw.lib`, checked). 0x25 is the import-library
  tool id of the VC7 betas the DirectX 8.0 SDK was built with. Its thunks are
  at 0x1003b502..0x1003b514 (`DirectDrawCreate`, `...CreateEx`,
  `...EnumerateA`, in PH's order). **Medium-high** (the DX 8.0 SDK itself is
  not held to read its `@comp.id`).
* **C++ 13.00.8830 x1 = the DirectX 8.0 SDK's `dxguid.lib`.** The DX 8.1 SDK's
  `dxguid.lib` is one `(29, 9178)` object (checked), PH's one 13.00 object; the
  renderer's IIDs (IID_IDirect3D7 0x1004660c, IID_IDirectDraw7 0x1004662c,
  IID_IDirectDrawGammaControl 0x1004661c, IID_IDirect3DHALDevice 0x100465ec,
  IID_IDirect3DTnLHalDevice 0x100465bc) sit together in .rdata as that
  object's data. Data-only, no code. **Medium-high**, same caveat.

## 3. The objects

AvP2's link order is Primal Hunt's, object for object: the pairs are monotone
over the whole image, every object change falls where PH's does, every CC
fill (12 of them) is at a boundary, and each AvP2 object's codegen class
(aligned/packed) is its PH counterpart's. **Fifty runs of code**: 48 renderer
objects (PH's map less `fogvolume.cpp`, with TU01/TU02/TU03 kept apart as
PH's map has them), the ddraw.lib thunks and three release StdLith.lib
objects. 218 CRT initialisers (`.CRT$XCU` 0x10048004..0x10048368): PH has 219, and
per object the counts are equal everywhere except `fogvolume.cpp`'s one
(the "init." column).

Object count against the Rich header: 73 C++ objects = 3 StdLith (struct_bank,
l_allocator, dynarray; libscan-identified) + **70 renderer objects**, of which
46 have code (if TU01-TU03 are one `d3d_modelvb.obj`, as `ph/` has it) and
**24 have none left** (header-only, cvar-less, or wholly removed by
`/OPT:REF`). PH: 69 = 3 StdLith (debug) + 66 renderer objects, 47 with code
(`fogvolume.cpp` the extra), 19 without. So AvP2 links ~4 more code-less
objects than PH even though it lacks `fogvolume.cpp`: files PH dropped from the
project, unidentifiable from the image. *Guess.*

Codegen: "/O2" = every function 16-aligned with 0x90 fill; "/O1" = packed
(unaligned starts). "identical / changed / AvP2-only" counts AvP2's functions
in the object (identical = masked bytes equal to the PH counterpart).

| # | AvP2 range | fns | bytes | AvP2 codegen | init. | PH object | ph/ file | PH flags | identical / changed / AvP2-only | conf. |
|--:|---|--:|--:|---|--:|---|---|---|---|---|
| 1 | 0x10001000-0x10004620 | 42 | 13538 | /O2 | 12 | TU01 0x10001000-0x10004620 | d3d_modelvb.cpp | /O2 /Ob2 | 41 / 1 / 0 | high |
| 2 | 0x10004620-0x10004656 | 2 | 54 | /O1 | 0 | TU02 0x10004620-0x10004656 | d3d_modelvb.cpp | (no /Gy run, in d3d_modelvb) | 2 / 0 / 0 | high |
| 3 | 0x10004660-0x10007924 | 16 | 12904 | /O2 | 0 | TU03 0x10004660-0x10007924 | d3d_modelvb.cpp | /O2 /Ob2 | 12 / 4 / 0 | high |
| 4 | 0x10007930-0x10008cc9 | 24 | 5017 | /O1 | 0 | TU04 0x10007930-0x10008c72 | drawmodel_clip.cpp | /O1 /Ob2 | 23 / 1 / 0 | high |
| 5 | 0x10008cd0-0x100098c7 | 7 | 3020 | /O2 | 1 | TU05 0x10008c80-0x10009877 | d3d_matrix.cpp | /O2 /Ob2 | 7 / 0 / 0 | high |
| 6 | 0x100098d0-0x1000b349 | 43 | 6777 | /O1 | 7 | TU06 0x10009880-0x1000b2f9 | drawworld.cpp | /O1 | 43 / 0 / 0 | high |
| 7 | 0x1000b349-0x1000f15e | 129 | 15893 | /O1 | 16 | TU07 0x1000b2f9-0x1000f09f | drawmodel.cpp | /O1 | 117 / 11 / 1 | high |
| 8 | 0x1000f160-0x1000f39a | 4 | 545 | /O2 | 2 | TU09 0x1000ff00-0x10010138 | d3d_viewparams.cpp | /O2 /Ob2 | 3 / 1 / 0 | high |
| 9 | 0x1000f3a0-0x1001094c | 25 | 5548 | /O1 | 6 | TU10 0x10010140-0x100116e3 | common_draw.cpp | /O1 /Ob2 | 24 / 1 / 0 | high |
| 10 | 0x1001094c-0x1001116b | 16 | 2079 | /O1 | 2 | TU11 0x100116e3-0x10011eec | common_init.cpp | /O1 /Ob2 | 14 / 2 / 0 | high |
| 11 | 0x1001116b-0x10012eaf | 185 | 7492 | /O1 | 88 | TU12 0x10011eec-0x10013c30 | common_stuff.cpp | /O1 /Ob2 | 185 / 0 / 0 | high |
| 12 | 0x10012eaf-0x10013215 | 8 | 870 | /O1 | 0 | TU13 0x10013c30-0x10013f96 | conparse.cpp | /O1 | 8 / 0 / 0 | high |
| 13 | 0x10013215-0x10013291 | 4 | 124 | /O1 | 1 | TU14 0x10013f96-0x10014012 | counter.cpp | /O1 /Ob2 | 4 / 0 / 0 | high |
| 14 | 0x100132a0-0x10019341 | 75 | 24215 | /O2 | 18 | TU15 0x10014020-0x10019f51 | d3d_draw.cpp | /O2 /Ob2 | 71 / 3 / 1 | high |
| 15 | 0x10019350-0x10019b0a | 10 | 1978 | /O1 | 1 | TU16 0x10019f60-0x1001a71a | d3d_skyobjects.cpp | /O1 /Ob2 | 10 / 0 / 0 | high |
| 16 | 0x10019b10-0x1001bf03 | 25 | 9021 | mixed | 4 | TU17 0x1001a720-0x1001cb13 | d3d_init.cpp | /O2 | 24 / 1 / 0 | high |
| 17 | 0x1001bf10-0x1001da60 | 33 | 6992 | /O1 | 0 | TU18 0x1001cb20-0x1001e650 | d3d_optimizedsurface.cpp | /O1 | 31 / 2 / 0 | high |
| 18 | 0x1001da60-0x1001e59c | 25 | 2876 | /O1 | 1 | TU19 0x1001e650-0x1001f273 | d3d_surface.cpp | /O1 | 22 / 3 / 0 | high |
| 19 | 0x1001e5a0-0x10021d64 | 53 | 13887 | /O2 | 6 | TU20 0x1001f280-0x10022a44 | d3d_texture.cpp | /O2 /Ob2 | 52 / 1 / 0 | high |
| 20 | 0x10021d70-0x10021da6 | 3 | 54 | /O1 | 1 | TU21a 0x10022a50-0x10022a86 | d3d_savedstates.cpp | /O1 /Ob2 | 3 / 0 / 0 | high |
| 21 | 0x10021da6-0x100220f1 | 12 | 843 | /O1 | 0 | TU21b 0x10022a86-0x10022dd1 | d3d_statelist.cpp | /O1 | 12 / 0 / 0 | high |
| 22 | 0x100220f1-0x10022424 | 10 | 819 | /O1 | 3 | TU22 0x10022dd1-0x10023104 | dirtyrect.cpp | /O1 /Ob2 | 10 / 0 / 0 | high |
| 23 | 0x10022424-0x10022bc5 | 21 | 1953 | /O1 | 2 | TU23 0x10023104-0x100238a6 | draw_canvas.cpp | /O1 /Ob2 | 21 / 0 / 0 | high |
| 24 | 0x10022bc5-0x10023854 | 15 | 3215 | /O1 | 1 | TU24 0x100238a6-0x10024534 | drawgouraud.cpp | /O1 | 15 / 0 / 0 | high |
| 25 | 0x10023860-0x100241db | 9 | 2380 | /O2 | 0 | TU25 0x10024540-0x10024ebb | drawlight.cpp | /O2 | 9 / 0 / 0 | high |
| 26 | 0x100241e0-0x10025013 | 19 | 3635 | /O1 | 2 | TU26 0x10024ec0-0x10025e6c | drawmodel_env.cpp | /O1 | 16 / 3 / 0 | high |
| 27 | 0x10025013-0x10028592 | 30 | 13695 | /O1 | 6 | TU27 0x10025e6c-0x1002938b | drawmodelshadows.cpp | /O1 | 26 / 4 / 0 | high |
| 28 | 0x100285a0-0x10029659 | 23 | 4114 | /O2 | 2 | TU28 0x10029390-0x1002a480 | drawobjects.cpp | /O2 /Ob2 | 18 / 4 / 1 | high |
| 29 | 0x10029660-0x100298ef | 9 | 655 | /O1 | 2 | TU29 0x1002a480-0x1002a70f | drawparticles.cpp | /O1 | 9 / 0 / 0 | high |
| 30 | 0x100298ef-0x1002afcf | 71 | 5856 | /O1 | 5 | TU30 0x1002a70f-0x1002be3c | drawpolymgr.cpp | /O1 /Ob2 | 71 / 0 / 0 | high |
| 31 | 0x1002afd0-0x1002cff8 | 14 | 8071 | /O2 | 1 | TU31 0x1002be40-0x1002ddb8 | drawpolygrid.cpp | /O2 /Ob2 | 11 / 3 / 0 | high |
| 32 | 0x1002d000-0x1002d07f | 9 | 127 | /O1 | 4 | TU32 0x1002ddc0-0x1002de3c | d3d_portal.cpp | /O1 | 9 / 0 / 0 | high |
| 33 | 0x1002d080-0x1002d63e | 5 | 1430 | /O2 | 3 | TU33a 0x1002de40-0x1002e400 | d3d_drawsky.cpp | /O2 /Ob2 | 5 / 0 / 0 | high |
| 34 | 0x1002d640-0x1002f329 | 12 | 7290 | /O2 | 2 | TU33b 0x1002e400-0x100300e9 | drawsprite.cpp | /O2 /Ob2 | 10 / 2 / 0 | high |
| 35 | 0x1002f330-0x10030ba3 | 12 | 6182 | /O2 | 2 | TU34 0x100300f0-0x10031a21 | drawworldmodel.cpp | /O2 /Ob2 | 10 / 2 / 0 | high |
| 36 | 0x10030bb0-0x10031d18 | 16 | 4456 | /O1 | 3 | TU35a 0x10031a30-0x10032af0 | fullintersectline.cpp | /Os /Ob1 | 9 / 7 / 0 | high |
| 37 | 0x10031d18-0x10031e94 | 3 | 380 | /O1 | 0 | TU35b 0x10032af0-0x10032c6c | gdenum.cpp | /O1 | 3 / 0 / 0 | high |
| 38 | 0x10031e94-0x10032031 | 1 | 413 | (1 fn) | 0 | TU35c 0x10032c6c-0x10032e09 | geomroutines.cpp | /O1 | 1 / 0 / 0 | high |
| 39 | 0x10032031-0x100323f9 | 4 | 968 | /O1 | 0 | TU35d 0x10032e09-0x100331d1 | intersect_line.cpp | /O1 | 4 / 0 / 0 | high |
| 40 | 0x100323f9-0x100329aa | 16 | 1457 | /O1 | 3 | TU36 0x100331d1-0x10033782 | lmdynamic.cpp | /O1 | 16 / 0 / 0 | high |
| 41 | 0x100329b0-0x10033ff7 | 10 | 5654 | /O2 | 1 | TU37 0x10033790-0x10034de7 | lightanim.cpp | /O2 /Ob2 | 9 / 1 / 0 | high |
| 42 | 0x10034000-0x1003481f | 17 | 2079 | /O1 | 0 | TU38 0x10034df0-0x10035611 | lmpaging.cpp | /O1 | 16 / 1 / 0 | high |
| 43 | 0x1003481f-0x10034ebb | 13 | 1692 | /O1 | 3 | TU39 0x10035611-0x10035c7e | lmsurfaces.cpp | /O1 | 7 / 6 / 0 | high |
| 44 | 0x10034ebb-0x10035ce6 | 11 | 3627 | /O1 | 0 | TU40 0x10035c7e-0x10036a96 | drawlightmaps.cpp | /O1 | 10 / 1 / 0 | high |
| 45 | 0x10035ce6-0x10035de7 | 7 | 257 | /O1 | 3 | TU41a 0x10036a96-0x10036b97 | d3d_tables.cpp | /O1 | 7 / 0 / 0 | high |
| 46 | 0x10035de7-0x1003882e | 72 | 10823 | /O1 | 1 | TU41b 0x10036b97-0x100395de | pixelformat.cpp | /O1 | 70 / 2 / 0 | high |
| 47 | 0x10038830-0x1003a676 | 36 | 7497 | /O2 | 1 | TU42 0x100395e0-0x1003b3d6 | tagnodes.cpp | /O2 /Ob2 | 33 / 2 / 1 | high |
| 48 | 0x1003a680-0x1003b502 | 49 | 3714 | /O1 | 1 | TU43 0x1003b3e0-0x1003c136 | vertexbuffer.cpp | /O1 | 47 / 0 / 2 | high |
| 49 | 0x1003b502-0x1003b514 | 3 | 18 | library | 0 | IMP 0x1003c136-0x1003c148 | (ddraw.lib thunks) | lib | 0 / 3 / 0 | high |
| 50 | 0x1003b520-0x1003b707 | 13 | 422 | library | 1 | LIB 0x1003c150-0x1003c58b | (StdLith.lib, Debug) | lib | 0 / 13 / 0 | high |


Notes:
* Ranges are the first function's start to the last function's end; COMDAT
  members sit with their first user exactly as in PH (the `_CVector` operators
  `+`/`*` that PH emitted in `fogvolume.obj`, its first user, AvP2 emits in
  `drawpolygrid` 0x1002ce70/0x1002cec0 -- the next first user once fog
  volumes are gone; `_CVector::Dot` lands in `d3d_draw` 0x100187c0).
* TU17 (`d3d_init`) ends with one packed 30-byte function (0x1001bee5) after
  23 aligned ones, as PH's does.
* Confidence: every object is **high** (in-order pairs cover >= 90 % of its
  functions; boundaries coincide with PH's, CC fill and initialiser runs).
  The one judgement call is the inherited one: whether TU01..TU03 are one
  object (`ph/d3d_modelvb.cpp`) or three.

## 4. Function pairs

`out/plan/avp2d3d-pairs.csv`, 1,271 AvP2 functions (1,255 renderer + 3 thunks
+ 13 StdLith):

| confidence | functions | meaning |
|---|--:|---|
| high | 1,215 | identical masked bytes in link order (1,180), or similarity >= 0.9 in link order; the thunks; StdLith functions libscan names |
| medium | 34 | in-order 0.65..0.9 similarity, call-graph votes, identical twins of ICF-folded templates, StdLith pieces paired by shape |
| low | 16 | ICF-folded tiny template members/empty functions (identical bytes, placed elsewhere), and in-order pairs below 0.65 |
| none | 6 | AvP2-only (below) |

Method: masked-byte equality, then an order-preserving alignment (DP over a
similarity of mnemonic sequences, strings and imports, banded on the anchor
offsets), then within-object greedy re-pairing, then call-graph votes (a
paired caller's n-th call names the callee pair), then the extent defects
folded on both sides (AvP2: 0x1000eb6e, 0x10018974, 0x100195cd, 0x1001bb96,
0x10020600, 0x10022990, 0x1002a4c4, 0x1002df60, 0x100388c6, 0x10038d08; PH:
the ten of `docs/D3D_OBJECTS.md`). Symbols are our `ph/` objects' where their
bytes are the PH row's (1,207 rows), else the scoreboard's pairing (`nolf:`
prefix: a NOLF object's name for it, `[near]` when only near).

## 5. Primal Hunt's changes (what needs `#ifdef PRIMAL_HUNT`)

**PH functions absent from AvP2 (27; 4,421 bytes)**

* **`fogvolume.cpp`, the whole object** (PH TU08, 0x1000f09f..0x1000fef5): 14
  functions, 3,573 bytes, 1 CRT initialiser -- fog volumes (CFogBox,
  d3d_CalcFogVolume, d3d_ProcessPortalObject, ...). AvP2 has no such object.
  Its hooks are in the changed callers below (*guess* which: the world-model
  draw, the scene renderer, the model vertex setup).
* `d3d_draw.cpp`: `allocator<StageState>::deallocate` 0x100195e0 (330) -- PH's
  new STL `vector<StageState>`; its initialiser `$E55` grows 206 -> 411.
* `drawmodel_env.cpp`: `d3d_GenUVQuarter` 0x1002544b (30) and
  `d3d_GenUVEnvMapOffset` 0x10025469 (111), two new UV generators;
  `CD3DModelDraw::DrawPasses` grows 1,312 -> 1,527 to use them.
* 8 `CMoArray<>` template members (0x1001efaa, 0x1001efbf, 0x10029a10,
  0x1002b7d2, 0x1003ba5b, 0x1003ba65, 0x1003bb23, 0x1003be5f, 10..182 bytes):
  `/OPT:ICF` folding and first-use placement, no source change implied.
* PH's debug StdLith `LAlloc::LAlloc`/`~LAlloc` (0x1003c4b0, 0x1003c4d0): the
  release library inlines them. Not a source change.

**AvP2 functions with no PH counterpart (6; 1,123 bytes)**

* **0x100135c0 (961 bytes), `d3d_draw.cpp`**: reached only through a pointer
  that `d3d_RenderScene` (0x10017aa0) stores at 0x10017d15; it splits/clips
  polygons into the CMoArray lists (calls 0x1000dfb6/0x1000e011/0x1000e06c).
  Removed in PH (nothing in PH is > 0.32 similar). Its helper 0x1000e06c (51)
  pairs only by call graph with PH's `_CVector::operator-` 0x10019270.
* 5 tiny template members: 0x1000e0c1 (22) and 0x10039e10 (24) are
  `GenBegin`-shaped vtable slots, 0x10028fa0 (4) `GenGetSize`, 0x1003ad1f (17),
  0x1003b069 (95, `SetSize2`-shaped): ICF/placement differences.

**Changed functions (69 pairs not byte-identical)**, all in the CSV with their
evidence. The real source changes, by object:

* `d3d_modelvb`/`drawmodel`/`drawmodel_env`/`drawmodelshadows`: the
  `CD3DModelDraw` layout differs (AvP2 +0x24c/+0x1d0/+0x1f8/+0x208 against PH
  +0x260/+0x1d4/+0x1fc/+0x20c: PH adds members), `CModelVB<>::GetVertexSize`
  0x20/0x28 swapped between the two vertex formats; `SetupModelLight`-sized
  bodies at 0x1000c100, 0x1000ccd9 (1,639 vs 1,502), 0x1000d3a7 (2,129 vs
  2,266, PH adds 0.025/0.02 constants), DrawPasses (above), shadow projection
  0x100252c6 (3,395 vs 3,262).
* `d3d_viewparams` `d3d_GetDynamicLightAdd` (0.59 similar) and
  `drawlightmaps` `d3d_GetSegmentPolies` (0.60): restructured bodies (the
  "string `333?`" in their evidence is the 0.7f constant 0x3f333333 read as
  text: both images use it).
* `d3d_init`: `d3d_InitDevice` 0x1001acc0 (same size, 0.67 similar: strings
  moved) and `d3d_RenderCommand` 0x1001b8a0 (same 1,221 bytes, reworked);
  `d3d_texture`: `d3d_InitTextures` 0x1001ec50 and the uploader 0x10020360.
* `d3d_optimizedsurface` `d3d_WarpOptimized` (1,333 vs 1,301).
* `drawsprite` `d3d_DrawFlatSprite` 0x1002d860 (PH adds the -20/1.0 Z-bias
  constants), `drawworldmodel` `d3d_DrawNoSortWorldModel` (211 vs 369: PH's
  +0x8c & 4 test and fog volumes), `d3d_ProcessWorldModel`.
* `fullintersectline`: stack frames grow (sub esp 0x48 vs 0x3c...: an
  intersection struct differs), `i_IntersectSegment`, `i_QuickSphereTest`,
  `i_TestWorldModel`.
* `pixelformat` `ConvertDXTGeneric` x2: `jl` vs `jb` (a signed/unsigned
  type); `lmsurfaces`/`common_init`/`lightanim` small deltas.

Many "changed" pairs of `/O2` objects are only compiler-revision codegen
(the per-file swap of section 1): judge each against an 8168 build, not
against PH's bytes.

**The brief's known difference does not hold in d3d.ren.** PH 0x1003a1f0 and
0x1003a7f0 pair with AvP2 0x10039440 and 0x10039a40, and both pairs test
`+0x8c` with `test dh,1` (bit 0x100) -- the two functions are byte-identical,
masked. Neither image has a `+0x8c & 0x40` test anywhere in the renderer.
If the 0x40/0x100 difference exists, it is in the engine or game code, not
here (or the addresses belong to another image).

## 6. Recommended build layout

* **Compiler:** msvc-6.00-8168 `cl.exe` (and its headers/libs), *not* icl,
  for every renderer file; link 6.00.8168-era. `/MT /W3 /D WIN32 /D NDEBUG
  /D _WINDOWS`, no `/GX` (as PH). Link order = the table's order (PH's
  `D3D_PH_POS`), then DX 8.0 `ddraw.lib` (or our implib stand-in), then the
  release StdLith.lib (struct_bank, l_allocator, dynarray at 0x1003b520..), then
  libcmt from onexit.obj.
* **Files:** reuse each `ph/<file>.cpp` as the AvP2 object of the same row,
  same flags as its `.mk`, compiled without `/D PRIMAL_HUNT`:
  * `/O2 /Ob2`: d3d_modelvb (TU01/TU03; TU02's two functions are its `/Os`
    no-`/Gy` piece, as PH), d3d_matrix, d3d_viewparams, d3d_draw (+ the STL
    include), d3d_texture, drawobjects, drawpolygrid, d3d_drawsky, drawsprite,
    drawworldmodel, lightanim, tagnodes.
  * `/O2`: d3d_init, drawlight.
  * `/O1 /Ob2`: drawmodel_clip, common_draw, common_init, common_stuff, counter,
    d3d_skyobjects, d3d_savedstates, dirtyrect, draw_canvas, drawpolymgr.
  * `/O1`: drawworld, drawmodel, conparse, d3d_optimizedsurface, d3d_surface,
    d3d_statelist, drawgouraud, drawmodel_env, drawmodelshadows, drawparticles,
    d3d_portal, gdenum, geomroutines, intersect_line, lmdynamic, lmpaging,
    lmsurfaces, drawlightmaps, d3d_tables, pixelformat, vertexbuffer.
  * `/Os /Ob1`: fullintersectline.
  * **leave out** `fogvolume.cpp` (`#ifdef PRIMAL_HUNT` its callers' uses).
* **`#ifdef PRIMAL_HUNT` work**, from section 5: fog volumes (whole file and
  its hooks), the StageState vector in d3d_draw, the two UV generators and
  DrawPasses, the CD3DModelDraw / ModelVB layout, d3d_InitDevice /
  d3d_RenderCommand / d3d_InitTextures / the texture uploader, WarpOptimized,
  the sprite Z bias, DrawNoSortWorldModel, the intersection structs,
  ConvertDXTGeneric's type; and AvP2's own 0x100135c0 to write back in
  (`#ifndef PRIMAL_HUNT`).
* Expected start: 1,200 functions identical with no `#ifdef` written (the
  test compile), against today's 10.

## 7. Regenerating

From the worktree root (`out/plan/cache/` holds the pickles):

1. `ph-rows.tsv`/`a2-rows.tsv`: `a2dump.py "d3d.ren" ...` in the build
   container (needs the PH objects in `src/engine/d3d/Release/`).
2. `a2feat.py` for both images (lo/hi: 10001000 1003b710 and 10001000 1003c5e5).
3. `a2match.py`, `a2check.py`, `a2votes.py`, `a2extent.py ... > a2-tails.txt`,
   `phsyms.py`, `a2final.py` (pairs CSV), `a2objects.py` (objects CSV);
   `a2view.py > a2view.txt` for the side-by-side; `phtus.py` is PH's object
   table (from the `ph/` headers and `.mk` flags).
4. The compiler test: `build/compile.sh` once (makes `out/release-tree`),
   then `a2ccall.sh <tag>` in the container and `a2objcmp.py -q <objs>`.
   `a2gaps.py`, `a2tumap.py` were exploratory.
