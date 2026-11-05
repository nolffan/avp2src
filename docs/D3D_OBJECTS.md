# Primal Hunt's d3d.ren: its translation units, in link order

The machine-readable map is `progress/d3d-objects.csv`. The `out/plan/` scripts and tables this cites are working files, not kept in the repository; `d3dtus.txt` and the pipeline regenerate them.

Analysis only: nothing in `src/`, `build/` or `progress/` was changed. Scope:
the app code 0x10001000..0x1003c5e5 of `out/reference/d3d.ren` (1,301
enumerated rows). Machine-readable companions:

| file | contents |
|---|---|
| `out/plan/d3d-objects.csv` | one row per object: `start,end,object_guess,evidence` |
| `out/plan/d3d-functions-tu.csv` | every row of the image: addr, size, TU, scorer status, our paired symbol, extent-defect note |
| `out/plan/d3d-unplaced.csv` | our 806 functions the scorer places nowhere, by file |
| `out/plan/d3dtus.txt` | the TU boundaries themselves (start row index, flags, name, confidence); edit this and rerun the scripts |
| `out/plan/d3dbase.py`, `d3dfuncs.py`, `d3ddatamap.py`, `d3dcuts.py`, `d3dview2.py`, `d3dtustats.py`, `d3dmapgen.py` | the pipeline (run in that order; `d3dmapdump.py` runs in the build container first to write `d3d-rows.tsv`) |
| `out/plan/d3d-datamap.tsv`, `d3d-funcs.tsv`, `d3d-strfiles.tsv` | every referenced .rdata/.data/.bss address with its users; per-function evidence; strings vs NOLF2 sources |

Scoreboard (quickscore, unchanged by this work): 139 identical, 70 near, 73
small, 44 attempted, 976 unwritten, 806 unplaced (328 crt).

## How the boundaries were found (and what each instrument proved)

1. **Padding tells /Ot from /Os, per function** (verified by compiling test
   files with the 8966 cl in the container, `out/plan/t/`):
   * `/O2` (and `/Ox`) put every function in a 16-aligned COMDAT that the
     **compiler pads with 0x90**; `/O1` (`/Os`) COMDATs are **align 1, no
     padding**, so `/Os` functions sit back to back at odd addresses.
   * Where an `/Os` run is followed by `/Ot` code the **linker** fills with
     0xCC. So a CC gap is always an object boundary (an `/Os` object
     followed by an `/Ot` one), and a function that starts unaligned is
     always `/Os` code.
   * Result: 315 rows are `/Ot`, 966 `/Os`, 20 ambiguous (aligned and
     ending exactly on a 16-byte boundary). The flag never changes inside a
     TU, so every `/Ot`<->`/Os` change is a TU boundary.
2. **The CRT initialiser table** `__xc_a` 0x1004a000 .. `__xc_z` 0x1004a370
   (219 entries): one per global with a dynamic initialiser, in link order.
   In `/Ot` objects an initialiser is one 22/25-byte COMDAT; in `/Os` ones a
   5-byte `jmp` stub plus a 26/30-byte body (both shapes reproduced by the
   test compiles). Runs of entries mark the top of a file's globals (40
   groups).
3. **.data string pool in address order.** The 427 referenced `.data`
   strings are first used in increasing row order. The one inversion is
   rows 619/620, which are one function. So the strings both name the
   module and confirm the order. (The shared initialised tables do not
   follow it: the cvar defaults 0x1004a72c..0x1004a798 and the
   function-pointer tables 0x1004da98..0x1004db64.)
4. **.bss in link order.** VC6 C++ never emits communals (tested: `int
   g;` goes to the object's `.bss`), so each object's `.bss` is one block in
   link order. Items read by a single function are not necessarily the
   function's own (an extern read once looks the same), but the blocks
   step up monotonically through the TU list below. Exception: static
   arrays inside inline/template functions are COMDAT `.bss`, placed
   after the runtime's, in reverse order (0x10099fc0..0x1009a960 for rows
   52..59, 0x100999a0..0x10099ee0 for rows 850..862). They show those rows
   are inline/template code (and those rows are called from several TUs).
5. **Dead code.** Under `/Gy` (`/O1`, `/O2`) `/OPT:REF` removes every
   function nothing references. A function nothing calls or points at
   shows a **non-`/Gy`** object. Exports (`RenderDLLSetup`,
   `GetSupportedModes`, `FreeModeList`, rows 322..324) and extent defects
   aside, only rows 42..43 (and possibly 634) qualify.
6. Our scorer pairs (identical/near) give NOLF addresses (our symbols are
   named for them). NOLF's address order follows PH's TU order (with
   COMDAT helpers placed at their first user), so the TU order is NOLF's
   `.dsp` order.

### Codegen per TU: what is settled

* **`/GX`: absent everywhere.** No function sets up an EH frame (`mov
  eax,fs:[0]`: 0 hits), there is no `__ehhandler` stub, and no FuncInfo.
  The "EH-looking" `.rdata` tables 0x10048140.., 0x10048204.. are
  **vtables** (12-slot list interfaces, 8-slot vertex-buffer classes). Unlike
  PH's lithtech.exe/server.dll, the renderer is built without `/GX`.
* **`/Ot` TUs are `/O2`**: they have intrinsic `memset`/`strlen` (`rep
  stosd`, `repnz scasb`: TU20 has 45 inline `strlen`s) and no calls to the
  CRT's `memset`/`memcpy`/`strlen`.
* **`/Os` TUs are `/O1`**: they call the CRT's `memset` (0x1003d040),
  `memcpy` (0x1003cb80), `strlen` (0x1003d5f0), `strcmp` (0x1003cfb0), so
  no `/Oi`. (`/O1` = `/Og /Os /Oy /Ob1 /Gs /Gf /Gy`.)
* **`/Oy`: on in both kinds.** Leaf functions address their arguments off
  `esp` in every TU that has a leaf function. VC6 `/O1` still builds `ebp`
  frames for functions with locals and loops (tested), so `ebp` frames in
  `/Os` TUs are expected. The `ebp` frames in `/Ot` rows (29, 33, 44,
  52..59, 88, 539, 540, 572, 818, 1008, 1009, 1078, 1080) are `/O2`'s own:
  rows 29 and 33 are byte-identical to our `/O2` build.
* **`/Ob`**: no evidence either way. Take the `/O1`/`/O2` default, `/Ob1`.
* **`/Gy`**: implied by `/O1`/`/O2` and confirmed by the absence of dead
  code, except TU02 (rows 42..43: two functions nothing references, packed
  with no padding, which needs a non-COMDAT `/Os` `.text`) and possibly
  TU18 (row 634, 30 bytes, no reference).
* **Mixed object, the StdLith allocator (LIB below)**: VC7 Debug shape
  (`push ebp; mov ebp,esp; sub esp,0x40; push ebx; push esi; push edi`).
  This is the Rich header's one cl 13.00.9178 object. It defines g_DefAlloc
  (0x1009961c, read by every `CMoArray`/`LAlloc` user). None of our
  toolchains is 13.00 (msvc-7.10-3077 is 13.10).

The current build compiles every d3d file `/O2` (`D3D_BASE`). Per the
table below, about two thirds of the code (TU04, TU06..TU08, TU10..TU14,
TU16, TU18, TU19, TU21..TU24, TU26, TU27, TU29, TU30, TU32, TU35, TU36,
TU38..TU41, TU43) is `/O1`.

### Extent defects found on the way (the enumeration's rows)

Each of these rows is the tail of the row before it (mid-instruction
start, or a branch target inside the previous function). Merge them
(`progress/extent-splits.csv`/screen-exclude, when someone takes these TUs):
189 (0x1000be43: row 188 at 0x1000be38 is 11 bytes and does not return;
189 opens with `fld [eax+0xb8]` on an eax 188 set), 253
(0x1000eaaf, `mov ecx,[ebp-4]` at entry), 578 (0x1001855d, mid-instruction
of 0x10018520's `je`), 587 (0x10019434, target of 0x10019400's `je`), 620
(0x1001bb46, mid-instruction), 715 (0x1001fb59, a store in the middle of
0x1001f930), 953 (0x1002b2e4, mid-instruction), 1016 (0x1002f97f, after
0x1002f970's prologue), 1203 (0x10039704, mid-instruction). So 0x1000be38 is one
519-byte function, 0x10018520 one of 2,671, 0x10019400 333, 0x1001b8d0 2,597, 0x1001f930
1,151, 0x1002f970 1,105, 0x10039600 415.

## The TUs

Flags: `/O2` = `/Ot` codegen (16-aligned, 0x90-padded COMDATs, intrinsics);
`/O1` = `/Os` codegen (packed, CRT calls). "Proposed name" is a **guess**
unless the confidence says high: no shipped renderer file names survive
(no `__FILE__`, assert or PDB path in any of the three d3d.ren). Names
come from the strings and from NOLF2's descendant files
(`render_a/src/sys/d3d`: `common_init.cpp` holds "VisibleSet::Init
failed" and "nullrender", `common_stuff.cpp` holds "%s %f", `d3d_draw.cpp`
holds "Sky Portals: %d" and "Triangles: %d", and so on;
`out/plan/d3d-strfiles.tsv`). Counts are rows with the extent defects
merged. The "our files" column lists the files whose functions the scorer
pairs here; small/attempted pairings are weak.

| TU | range | bytes | funcs | flags (codegen) | proposed name | conf. | ident/near/small/att/todo | our files (placed functions) | NOLF addr. of paired fns |
|---|---|--:|--:|---|---|---|---|---|---|
| TU01 | 0x10001000-0x10004620 | 13856 | 42 | /O2 (/Ot /Oi /Oy /Gy /GF) | model vertex-buffer/TnL renderer (e.g. d3d_modelvb.cpp) | medium | 15/8/5/2/12 | w57_10009860.cpp 11, w2_10003a10.cpp 5, w14_10003bb0.cpp 2, w9_1006f540_os.cpp 2, w14_10006840.cpp 2 +6 more | 0x10003a10-0x10009ae0 |
| TU02 | 0x10004620-0x10004656 | 54 | 2 | /Os /Oy, no /Gy (unreferenced code kept) | (2-function object, dead code) | medium | 0/0/0/0/2 |  |  |
| TU03 | 0x10004660-0x10007924 | 12996 | 16 | /O2 (/Ot /Oi /Oy /Gy /GF) | model TnL transforms / VB fill (continuation) | low | 2/3/0/1/10 | d3dr4b.cpp 1, d3dr4a.cpp 1, w22_1007dbd0_os.cpp 1, w57_100037f0.cpp 1, d3dr5c_o2.cpp 1 | 0x100037b0-0x1007dbd0 |
| TU04 | 0x10007930-0x10008c72 | 4930 | 23 | /O1 (/Os /Oy /Gy, no /Oi) | drawmodel_clip.cpp (model clipping) | low | 1/3/0/1/18 | w24_10068c80_osqipni.cpp 1, big24090.cpp 1, w14_10011980_osoiqip.cpp 1, w28_1002ef30_osqipni.cpp 1 |  |
| TU05 | 0x10008c80-0x10009877 | 3063 | 7 | /O2 (/Ot /Oi /Oy /Gy /GF) | d3d_matrix.cpp (transform helpers) | low | 3/0/1/1/2 | d3dneutral.cpp 1, w43_10017c10_osoiqipni.cpp 1, d3dbatchd.cpp 1, d3dr2a.cpp 1 | 0x1000eec0-0x100151b0 |
| TU06 | 0x10009880-0x1000b2f9 | 6777 | 43 | /O1 (/Os /Oy /Gy, no /Oi) | drawworld.cpp (world polys, lightmaps, detail textures) | medium | 0/2/9/2/30 | d3dblit.cpp 1, d3dr4a.cpp 1, d3dr4b.cpp 1, d3dr4c.cpp 1, w18_10037570.cpp 1 +6 more | 0x10011500-0x10011560 |
| TU07 | 0x1000b2f9-0x1000f09f | 15782 | 128 | /O1 (/Os /Oy /Gy, no /Oi) | drawmodel.cpp (models) | medium | 7/4/19/9/89 | w57_10009860.cpp 8, w57_1002db00.cpp 8, w3_10014580_os.cpp 3, d3dr2a.cpp 2, w43_10017c10_osoiqipni.cpp 2 +7 more | 0x10014940-0x10050790 |
| TU08 | 0x1000f09f-0x1000fef5 | 3670 | 17 | /O1 (/Os /Oy /Gy, no /Oi) | (unknown, static helpers + 1 global) | low | 3/0/1/0/13 | w24_10068c80_osqipni.cpp 2, w57_1002db00.cpp 1, w6_10034f10_osoi.cpp 1 |  |
| TU09 | 0x1000ff00-0x10010138 | 568 | 4 | /O2 (/Ot /Oi /Oy /Gy /GF) | (near-Z / view setup) | medium | 0/1/0/0/3 | w31_1001e360.cpp 1 | 0x1001e360-0x1001e360 |
| TU10 | 0x10010140-0x100116e3 | 5539 | 25 | /O1 (/Os /Oy /Gy, no /Oi) | d3d_globals / common_draw.cpp (shared render globals) | medium | 2/1/7/0/15 | d3dneutral.cpp 3, w17_10023930.cpp 1, w28_100256d0_qipni.cpp 1, w57_1006d500_os.cpp 1, d3dr4b.cpp 1 +3 more | 0x1001e8d0-0x100234b0 |
| TU11 | 0x100116e3-0x10011eec | 2057 | 16 | /O1 (/Os /Oy /Gy, no /Oi) | common_init.cpp | high | 0/3/1/0/12 | w12_7fae0.cpp 1, d3dbatcha.cpp 1, d3dr5c.cpp 1, w26_10020fd0_osoiqip.cpp 1 | 0x100210d0-0x10021220 |
| TU12 | 0x10011eec-0x10013c30 | 7492 | 185 | /O1 (/Os /Oy /Gy, no /Oi) | common_stuff.cpp (console variables + helpers) | high | 0/0/9/9/167 | w57_1002db00.cpp 6, w57_1003b1c0.cpp 2, w57_1006d500_os.cpp 1 |  |
| TU13 | 0x10013c30-0x10013f96 | 870 | 8 | /O1 (/Os /Oy /Gy, no /Oi) | (utility, unknown) | low | 0/1/0/1/6 | w15_10022e90_osoiqip.cpp 1 | 0x100230d0-0x100230d0 |
| TU14 | 0x10013f96-0x10014012 | 124 | 4 | /O1 (/Os /Oy /Gy, no /Oi) | counter.cpp (QueryPerformanceCounter timers) | high | 0/0/0/0/4 |  |  |
| TU15 | 0x10014020-0x10019f51 | 24369 | 75 | /O2 (/Ot /Oi /Oy /Gy /GF) | d3d_draw.cpp (world/BSP draw, frame stats) | high | 26/6/4/3/36 | w57_1002db00.cpp 16, w28_100256d0_qipni.cpp 7, d3dr3a.cpp 2, d3dbatchd.cpp 2, w4_10023c40_os.cpp 1 +8 more | 0x1000a700-0x1002e350 |
| TU16 | 0x10019f60-0x1001a71a | 1978 | 10 | /O1 (/Os /Oy /Gy, no /Oi) | (vertex/sky helper, unknown) | low | 0/0/0/0/10 |  |  |
| TU17 | 0x1001a720-0x1001ca36 | 8982 | 23 | /O2 (/Ot /Oi /Oy /Gy /GF) | d3d_init.cpp (device init, display modes, console cmds) | medium | 9/2/2/0/10 | d3ddraw.cpp 4, d3dneutral.cpp 2, w5_10033ba0.cpp 2, d3dmisc.cpp 2, d3dr4a.cpp 1 +2 more | 0x10032600-0x1006c4a0 |
| TU18 | 0x1001ca40-0x1001e650 | 7184 | 35 | /O1 (/Os /Oy /Gy, no /Oi) | d3d_optimizedsurface.cpp | medium | 2/4/1/0/27 | w81_10034ed0_osg5.cpp 2, w38_100322c0_qip.cpp 1, d3dneutral.cpp 1, w18_1001d180_os.cpp 1, d3dqip3.cpp 1 +1 more | 0x10032f30-0x100362a4 |
| TU19 | 0x1001e650-0x1001f273 | 3107 | 26 | /O1 (/Os /Oy /Gy, no /Oi) | d3d_surface.cpp (surfaces, screenshot) | medium | 2/0/0/1/23 | d3dmisc.cpp 1, d3dblit.cpp 1 |  |
| TU20 | 0x1001f280-0x10022a44 | 14276 | 53 | /O2 (/Ot /Oi /Oy /Gy /GF) | d3d_texture.cpp | high | 14/4/3/1/31 | w6_10039730_qip.cpp 4, d3dr5b.cpp 2, d3dr4b.cpp 2, d3dqip2.cpp 2, w26_10020fd0_osoiqip.cpp 1 +10 more | 0x100375b0-0x1005db90 |
| TU21 | 0x10022a50-0x10022dd1 | 897 | 15 | /O1 (/Os /Oy /Gy, no /Oi) | (unknown) | low | 0/0/0/1/14 |  |  |
| TU22 | 0x10022dd1-0x10023104 | 819 | 10 | /O1 (/Os /Oy /Gy, no /Oi) | (unknown) | low | 0/0/0/0/10 |  |  |
| TU23 | 0x10023104-0x100238a6 | 1954 | 23 | /O1 (/Os /Oy /Gy, no /Oi) | draw_canvas.cpp | medium | 0/1/0/0/22 | d3dr2b.cpp 1 | 0x1003b8e0-0x1003b8e0 |
| TU24 | 0x100238a6-0x10024534 | 3214 | 14 | /O1 (/Os /Oy /Gy, no /Oi) | drawgouraud/multipass (unknown) | low | 0/0/0/1/13 |  |  |
| TU25 | 0x10024540-0x10024ebb | 2427 | 9 | /O2 (/Ot /Oi /Oy /Gy /GF) | (sprites? unknown) | low | 3/2/0/0/4 | d3dr5a_o2.cpp 2, d3dqip.cpp 1, d3dr4a_o2.cpp 1, w27_1005dae0.cpp 1 | 0x10040ca0-0x100646a0 |
| TU26 | 0x10024ec0-0x10025e6c | 4012 | 21 | /O1 (/Os /Oy /Gy, no /Oi) | drawmodel_env.cpp? (ModelSpecular, ModelTexture) | low | 0/0/0/2/19 |  |  |
| TU27 | 0x10025e6c-0x1002938b | 13599 | 31 | /O1 (/Os /Oy /Gy, no /Oi) | drawmodelshadows.cpp | medium | 0/1/0/1/29 | w54_10048180_osoiqipsf.cpp 1 |  |
| TU28 | 0x10029390-0x1002a479 | 4329 | 26 | /O2 (/Ot /Oi /Oy /Gy /GF) | drawobjects/sort (DrawSorted) | medium | 15/3/0/1/7 | w7_100500a0_qip.cpp 13, d3dr2a.cpp 1, d3dr2b_o2.cpp 1, d3dr2d.cpp 1, d3dr5b.cpp 1 +1 more | 0x1004fb80-0x1007f150 |
| TU29 | 0x1002a480-0x1002a70f | 655 | 9 | /O1 (/Os /Oy /Gy, no /Oi) | drawparticles.cpp | medium | 0/0/0/0/9 |  |  |
| TU30 | 0x1002a70f-0x1002be3c | 5933 | 74 | /O1 (/Os /Oy /Gy, no /Oi) | drawpolymgr.cpp | medium | 5/7/2/0/60 | d3dr5b.cpp 3, d3dhigh.cpp 3, d3dr4c.cpp 1, w17_1001c760_os.cpp 1, d3dr4a.cpp 1 +5 more | 0x1001dc50-0x1005a050 |
| TU31 | 0x1002be40-0x1002ddb8 | 8056 | 11 | /O2 (/Ot /Oi /Oy /Gy /GF) | drawpolygrid.cpp | high | 5/2/1/0/3 | d3dr4c.cpp 1, w8_1005d650.cpp 1, d3dr5a_o2.cpp 1, d3dr3a.cpp 1, d3dr2c.cpp 1 +3 more | 0x1005d650-0x10068c50 |
| TU32 | 0x1002ddc0-0x1002de3c | 124 | 8 | /O1 (/Os /Oy /Gy, no /Oi) | (portal cvars only) | medium | 0/0/0/0/8 |  |  |
| TU33 | 0x1002de40-0x100300e9 | 8873 | 17 | /O2 (/Ot /Oi /Oy /Gy /GF) | d3d_drawsky.cpp | medium | 2/1/1/1/12 | d3dmisc.cpp 1, d3dr5b_o2.cpp 1, d3dr4b.cpp 1, w16_10064720.cpp 1 | 0x1003b760-0x10064720 |
| TU34 | 0x100300f0-0x10031a21 | 6449 | 13 | /O2 (/Ot /Oi /Oy /Gy /GF) | drawworldmodel.cpp | high | 2/1/0/0/10 | d3dr2d.cpp 1, w31_10066d10_qipni.cpp 1, d3dr2a.cpp 1 | 0x10067aa0-0x10068890 |
| TU35 | 0x10031a30-0x100331d1 | 6049 | 24 | /O1 (/Os /Oy /Gy, no /Oi) | d3d_device enumeration (unknown) | low | 0/1/0/1/22 | d3dr3b.cpp 1 | 0x10069e40-0x10069e40 |
| TU36 | 0x100331d1-0x10033782 | 1457 | 16 | /O1 (/Os /Oy /Gy, no /Oi) | lightmap dynamic (unknown) | low | 1/1/1/1/12 | d3dneutral.cpp 1, d3dr3b.cpp 1, d3dr3a_os.cpp 1 | 0x1006a6a0-0x1006a710 |
| TU37 | 0x10033790-0x10034de7 | 5719 | 10 | /O2 (/Ot /Oi /Oy /Gy /GF) | lightanim (LMAnimStatic) | medium | 1/1/0/1/7 | d3dr3b_o2.cpp 1, w17_1006ad20_qip.cpp 1 | 0x1006c4d0-0x1006c510 |
| TU38 | 0x10034df0-0x10035611 | 2081 | 17 | /O1 (/Os /Oy /Gy, no /Oi) | lightmap paging | medium | 0/0/1/0/16 | d3dneutral.cpp 1 |  |
| TU39 | 0x10035611-0x10035c7e | 1645 | 13 | /O1 (/Os /Oy /Gy, no /Oi) | (unknown) | low | 0/0/0/1/12 |  |  |
| TU40 | 0x10035c7e-0x10036a96 | 3608 | 11 | /O1 (/Os /Oy /Gy, no /Oi) | (unknown) | low | 0/1/0/0/10 | d3dr2b.cpp 1 | 0x100733a0-0x100733a0 |
| TU41 | 0x10036a96-0x100395de | 11080 | 79 | /O1 (/Os /Oy /Gy, no /Oi) | (dispatch tables, unknown) | low | 1/1/2/1/74 | d3dbatchd.cpp 1, d3dr4c.cpp 1, d3dbatchb.cpp 1, d3dr2c_os.cpp 1 | 0x1007a180-0x1007a3b0 |
| TU42 | 0x100395e0-0x1003b3d6 | 7670 | 33 | /O2 (/Ot /Oi /Oy /Gy /GF) | visible set / drawobjects (VS_*) | medium | 16/4/2/0/11 | d3dblit.cpp 7, d3dbatchc_o2.cpp 2, d3dqip4.cpp 2, w57_1007bb20_qipni.cpp 1, d3dr4a_o2.cpp 1 +9 more | 0x1007a580-0x1007f560 |
| TU43 | 0x1003b3e0-0x1003c136 | 3414 | 45 | /O1 (/Os /Oy /Gy, no /Oi) | (base classes, lists; unknown) | low | 0/1/1/1/42 | d3dr4b.cpp 1, d3dbatcha.cpp 1 | 0x1007ee40-0x1007ee40 |
| IMP | 0x1003c136-0x1003c148 | 18 | 3 | library | DirectX SDK import thunks (ddraw.lib etc.) | high | 0/0/0/0/3 |  |  |
| LIB | 0x1003c150-0x1003c526 | 982 | 14 | library | the cl 13.00.9178 object: StdLith l_allocator.cpp (LAlloc, g_DefAlloc 0x1009961c), built unoptimised | medium | 0/0/0/0/14 |  |  |
| CRT | 0x1003c530-0x1003c5e5 | 181 | 4 | library | runtime | high | 0/0/0/0/4 |  |  |

### Notes per TU (evidence and doubts)

* **TU01 (rows 0..41)**: starts with `??_H` (the vector-constructor
  iterator COMDAT) and 12 initialisers: 8 cvars (ModelMinTri,
  ModelDetailTextureScale, SpecularPowerTest/ScaleTest, ModelVBSize/Count/
  Cache/CacheDelay) and 4 vertex-buffer objects whose vtables sit at
  0x10048140..0x100481bf. Their base-class slots are TU43's
  (0x1003b45b..0x1003b611), their own slots rows 46..50. .bss
  0x100515d0..0x10052bb8. Clip/lerp helpers 28..36 (`LerpClipVert`,
  `ClipEdgeNear`, `ClipEdgeLeftA`, `LerpSClipVertA`) and `CD3DModelDraw`
  pieces (rows 37, 39 = NOLF 0x10008340/0x10008680).
* **TU02 (42..43)**: two 27-byte `push esi; mov edx,offset f; mov eax,offset
  f; ... call 0x10004270; ret 4` wrappers around TU01 functions (0x10003b60,
  0x10003e00). Nothing references them, and they are packed. A separate
  `/Os` object without `/Gy` is the only reading consistent with both. So
  rows 44..59 are a third object (TU03), not TU01's tail.
* **TU03 (44..59)**: 44 (4,241 bytes) and 45 (2,612, called from TU26/27),
  the VB classes' own methods 46..50, and 52..59: eight 655..693-byte
  transform variants with function-static arrays in COMDAT `.bss` (so they
  are inline/template, called from TU01, TU31 and TU33).
* **TU04 (60..82)**: "vertex buffer overflow"; `ClipEdgeTop/RightA/Bottom`
  (NOLF big24090, w14_10011980, w28_1002ef30); .bss 0x10052bd8..0x10053fe8.
  Its row 61 is called from TU15.
* **TU05 (83..89)**: one empty (1-byte) initialiser; `MatMul` (85); 88 uses
  `_chkstk` for a 0x4080-byte frame. .bss 0x10053ff0..0x1005540c. The name is
  a pure guess.
* **TU06 (90..132)**: 7 cvars (DetailTextures, DetailTextureAdd,
  DetailTextureAngle, FixSparkleys, LMFullBright, EnvMapWorld, LMAnim);
  .bss 0x10055410..0x10056908.
* **TU07 (133..262)**: 16 initialisers (model cvars LightModelSprites,
  ModelLODOffset, ModelZoomScale, ModelLODBlend*, ExtraFOV[XY]Offset,
  ModelFovTest, ModelSun*, ModelSaturation, DrawModelsRigid,
  ModelCacheRigid, ModelUseTnL and two objects whose constructors are rows
  172 and 178). 213..262 are mostly 12-slot list-interface vtable methods
  (vtables 0x10048204, 0x10048234, 0x10048264). .bss 0x10056910..0x1005888c,
  one block. 130 rows; **it may be two objects**, but no .bss or padding
  evidence splits it.
* **TU08 (263..279)**: a global (initialiser 269/270) plus helpers 263..268,
  which only 267/274/276 call, and `_CVector` operator COMDATs 277..279.
  Boundary 262|263: 262 is `BaseNew` called from TU07 only. .bss
  0x100590d0..0x1005a324.
* **TU09 (280..283)**: cvars NearZ/ReallyCloseNearZ (cvar objects
  0x1005a328, 0x1005b748). Owns .bss 0x1005a328..~0x1005b76f (big arrays).
  Row 283 is called from TU33.
* **TU10 (284..308)**: five empty initialisers and one real one (289,
  constructing 0x1005b790). Owns the most-shared .bss block
  0x1005b790..0x1005d240 (the renderer's frame/view globals). The five
  1-byte initialisers *could* instead be five code-less objects (a header's
  trivially-constructed static), which would count towards the Rich
  header's 69.
* **TU11 (309..324)**: holds the three exports (`GetSupportedModes` 322,
  `FreeModeList` 323, `RenderDLLSetup` 324) and the device-selection
  strings. NOLF2 splits these between `common_init.cpp` and
  `d3d_shell.cpp`. .bss 0x1005d258..0x1005d2a8.
* **TU12 (325..509)**: 88 cvars (TableFog .. Dither), then the shared helpers
  502..509 ("d3drender.dll: out of memory", "%s %f"). Its .bss
  0x1005d2c8..0x1005df8c holds every cvar object and g_pStruct-like
  0x1005df08.
* **TU13 (510..517)**: an in-place upper-case loop (510, called from TU11) and
  the 512..517 cluster (NOLF 0x100230d0). Whether it is TU12's tail or its
  own object is undecided: no padding change, no .bss of its own.
* **TU14 (518..521)**: its initialiser calls `QueryPerformanceFrequency`
  into .data 0x1004ac58/0x1004ac5c, right after TU12's last string. The
  three functions after it are a counter start/stop pair, called from all
  over (67..1215).
* **TU15 (522..598)**: 18 initialisers (VFog*, DrawWorldTree,
  DrawTerrainSections, AlphaTest, DrawPortals, ShowPortalBounds,
  PortalLightmap, ShowTexInfo, RenderToFront), world/BSP drawing (NOLF
  0x10023c40..0x10024020 and 0x100256d0..0x1002e350, two NOLF windows), and
  the frame-statistics printer at 0x10018520. .bss 0x1005df98..0x1005fe60:
  the private items of rows 561..566 interleave with the cvars of 522..537,
  so it is one object.
* **TU16 (599..608)**: one empty initialiser; 603 is called from TU33.
* **TU17 (609..632)**: the DirectDraw/D3D error-string function (609),
  cvars UseD3DClip/MipMapBias/Trilinear/Anisotropic, card detection
  (PERMEDIA/POWERVR), z-buffer, device creation (0x1001b8d0, "lithtech.gam"),
  `d3d_CreateWorld`/`DestroyWorld`/`Start3D`/`End3D`, console commands
  (LISTDEVICES, LOADGAMMA, ...). .bss 0x10062280..~0x100638dc (includes the
  device pointer 0x100638c8). 609 (no .bss, before the initialisers)
  could be its own object.
* **TU18 (633..667)**: optimised 2D surfaces (`d3d_GetOptimized2DBlend`/
  `Color`). .bss 0x100638e0..0x10066168. Row 634 is unreferenced.
* **TU19 (668..693)**: LockOnFlip, surface lock/unlock, the "nonoptimized
  surface" warning, ScreenShot. .bss 0x10066170..0x100661a0.
* **TU20 (694..747)**: S3TCEnable; texture formats ("FORMAT_* texture format
  missing"), texture upload (`r_TransferTexture`). .bss 0x100661a8..0x10068320.
* **TU21..TU24 (748..809)**: one `/Os` run split by its four initialiser
  groups and four steps of .bss (0x10069728 | 0x10069740..0x10069de0 |
  0x10069de8..0x10069e08 | 0x1006b658..0x1006d678). TU23 has DrawCanvases.
  TU24 has MultipassGouraud. The cuts at 763/773/796 are at the
  initialisers. Medium confidence on the cuts, low on the names.
* **TU25 (810..818)**: a short `/O2` object between two `/Os` runs (CC pad
  before it). NOLF 0x10040ca0..0x10043a30 and 0x100646a0.
* **TU26/TU27 (819..870)**: one `/Os` run. Cut at 840 (the
  ModelShadowAlpha initialisers). TU26 .bss 0x1006d680..0x1006dac8. But
  row 831 (TU26) privately reads 0x1006eacc/0x1006ead0, inside TU27's
  block, so **it may be one object**. TU27 holds the shadow projector
  ("Invalid texture size for shadows").
* **TU28 (871..896)**: SortProfile/DrawSorted, plus the `CMoArray` list
  template members it instantiates first (880..896).
* **TU29 (897..905)**: PSSrcBlend/PSDestBlend (particle systems). .bss
  0x10072810..0x10074250.
* **TU30 (906..980)**: DetailTextureScale, DrawPolyMgr, TestGouraud,
  TestLightmap, "Material '%s', pass %d, stage %d", the
  `DrawPolyMgr::DrawPolyAdditionalPass` assert. Small-function tables in
  .data 0x1004dbe8..0x1004dc84. .bss 0x10074258..0x100746d8.
* **TU31 (981..991)**: EnvMapPolyGrids. Row 982 is 5,473 bytes.
* **TU32 (992..999)**: only the four portal cvars' initialisers survive:
  a file whose code `/OPT:REF` removed entirely.
* **TU33 (1000..1017)**: AllSkyPortals and counter globals (0x1007946c..).
  1015+1016 are one function. 1017 is called only from 1009 and is put
  here, not in TU34.
* **TU34 (1018..1030)**: DrawWorldModels. .bss 0x1007ad48..0x1007b170.
* **TU35/TU36 (1031..1070)**: one `/Os` run. Cut at 1055 by .bss
  (0x1007c178..0x1007c1d8 | 0x1007c1e4..0x1007c640). TU35 holds DirectDraw
  device enumeration (`DirectDrawEnumerateExA`). TU36 holds LMDynamic*.
* **TU37 (1071..1080)**: LMAnimStatic. .bss 0x1007d85e..0x1007dd60.
* **TU38..TU41 (1081..1200)**: one `/Os` run cut by .bss steps
  (0x1007f180..0x1008058c | 0x10080590..0x10081abc | 0x10082ebc..0x10087af8 |
  0x10087b00..0x10097e40) and the initialiser groups at 1098, 1104 and
  1123..1129. TU38 holds lightmap paging ("Lightmaps paged in"). TU41 has
  function-pointer dispatch tables in .data 0x1004dfc8..0x1004e108 over
  rows 1131..1165, and two ~2 KB workers (1199, 1200).
* **TU42 (1201..1234)**: the visible-set lists (VS_CANVASES ..
  VS_MODELS, "GetVisBSP returned LTNULL"), plus `CMoArray<DWORD>` members
  first instantiated here. .bss 0x10099240..0x10099544.
* **TU43 (1235..1279)**: the base classes of TU01's vertex-buffer objects
  (constructor 1235 and destructor 1237, called from TU01's initialisers)
  and list code. .bss 0x100995a8..0x10099614. The last renderer object.
* **IMP (1280..1282)**: `jmp [__imp_...]` stubs of the DirectX import
  libraries (the Rich header's three implib-7.00 descriptors).
* **LIB (1283..1296)**: see above (cl 13.00.9178, unoptimised; StdLith
  allocator).
* **CRT (1297..1300)**: libcmt code below `onexit.obj`, as `targets.py`
  says.

### How many objects: 43 found, the Rich header says 69

The Rich header counts every C++ 8966 object the link was given. 43
objects with code are delimited here (TU01..TU43). The other ~26 are,
in some mix:
* **objects whose code `/OPT:REF` removed entirely.** TU32 is one that kept
  its cvars. Ones with no dynamic initialiser leave nothing, or only
  .bss, behind. No unreferenced initialised .data exists (checked), so
  such objects had no initialised data either.
* **splits this map could not prove**: TU07 (130 rows), TU12/TU13, TU17
  (609 alone?), TU26/TU27 (one?), TU41 (79 rows), TU43, and the 1-byte
  initialisers of TU10 (five possible code-less objects).

## Retail functions with no counterpart in our source (unwritten), by TU

All 967 rows, with sizes, are in `out/plan/d3d-functions-tu.csv` (status
`todo`). The five largest of each TU:

| TU | unwritten functions | bytes | largest (addr:size) |
|---|--:|--:|---|
| TU01 | 12 | 11087 | 0x10002050:2915, 0x10002bc0:2827, 0x100036d0:1160, 0x10003e00:1057, 0x10001530:1032 |
| TU02 | 2 | 54 | 0x10004620:27, 0x1000463b:27 |
| TU03 | 10 | 8252 | 0x10005700:2612, 0x100073b0:693, 0x10007670:692, 0x10006e40:689, 0x10007100:688 |
| TU04 | 18 | 4310 | 0x10007e5d:826, 0x10007b41:666, 0x100083ec:487, 0x10008197:407, 0x100089cc:309 |
| TU05 | 2 | 1367 | 0x10009320:1337, 0x10009300:30 |
| TU06 | 30 | 6339 | 0x10009969:1223, 0x1000a870:955, 0x1000a257:565, 0x1000a4e8:469, 0x1000a6bd:435 |
| TU07 | 89 | 11410 | 0x1000c080:1653, 0x1000cc91:1502, 0x1000c818:1145, 0x1000baae:501, 0x1000de0e:292 |
| TU08 | 13 | 3568 | 0x1000f755:670, 0x1000f217:585, 0x1000fc52:578, 0x1000fa33:528, 0x1000f09f:295 |
| TU09 | 3 | 346 | 0x10010010:296, 0x1000ff00:25, 0x1000ff20:25 |
| TU10 | 15 | 5367 | 0x100104cb:2766, 0x10011162:949, 0x10011059:265, 0x10010383:251, 0x100101f8:249 |
| TU11 | 12 | 1701 | 0x100118be:720, 0x10011d72:378, 0x10011cc5:143, 0x10011bb7:125, 0x100117bb:69 |
| TU12 | 167 | 7055 | 0x10013bcd:99, 0x10013b53:84, 0x10012014:76, 0x10012065:76, 0x10012154:76 |
| TU13 | 6 | 665 | 0x10013d5a:221, 0x10013e37:201, 0x10013cfe:92, 0x10013cac:82, 0x10013c30:48 |
| TU14 | 4 | 124 | 0x10013fe0:50, 0x10013f96:33, 0x10013fb7:30, 0x10013fd5:11 |
| TU15 | 36 | 16169 | 0x10016c60:3949, 0x100162a0:2365, 0x100154c0:672, 0x10017bd0:652, 0x10016030:609 |
| TU16 | 10 | 1978 | 0x10019f61:524, 0x1001a2a1:495, 0x1001a533:487, 0x1001a16e:307, 0x1001a490:60 |
| TU17 | 10 | 5758 | 0x1001a720:2147, 0x1001c4b0:1221, 0x1001b010:1089, 0x1001b8d0:630, 0x1001b4c0:436 |
| TU18 | 27 | 6830 | 0x1001d890:1301, 0x1001d4fa:918, 0x1001cd43:671, 0x1001ddea:638, 0x1001cff4:579 |
| TU19 | 23 | 2801 | 0x1001eb09:620, 0x1001ed75:531, 0x1001f19c:191, 0x1001e871:175, 0x1001f0ed:175 |
| TU20 | 31 | 10628 | 0x10021040:1871, 0x10021f70:1731, 0x10020720:1334, 0x10021790:1132, 0x10020cd0:824 |
| TU21 | 14 | 843 | 0x10022ccc:261, 0x10022b60:73, 0x10022be9:72, 0x10022a97:70, 0x10022ba9:64 |
| TU22 | 10 | 819 | 0x10022e31:367, 0x10022ffc:172, 0x10022fa0:92, 0x100230a8:84, 0x10022dde:37 |
| TU23 | 22 | 1916 | 0x1002318e:407, 0x10023512:350, 0x100233c9:282, 0x10023325:164, 0x100236d2:129 |
| TU24 | 13 | 2715 | 0x100238e2:900, 0x10023c66:794, 0x10024443:241, 0x1002438e:181, 0x1002430a:132 |
| TU25 | 4 | 2180 | 0x10024a60:1115, 0x10024590:592, 0x10024800:399, 0x10024540:74 |
| TU26 | 19 | 2455 | 0x10024efe:521, 0x10025c72:371, 0x10025269:302, 0x10025193:214, 0x10025b66:158 |
| TU27 | 29 | 13269 | 0x10027e40:5451, 0x1002611f:3262, 0x10027b95:683, 0x10025ed1:590, 0x100273f4:311 |
| TU28 | 7 | 1906 | 0x10029450:761, 0x100297a0:467, 0x10029c20:328, 0x1002a050:170, 0x10029980:133 |
| TU29 | 9 | 655 | 0x1002a52e:424, 0x1002a4c6:81, 0x1002a6d6:36, 0x1002a485:30, 0x1002a4a8:30 |
| TU30 | 60 | 4981 | 0x1002aee2:845, 0x1002b4b3:553, 0x1002bc2a:257, 0x1002adba:234, 0x1002bd2b:219 |
| TU31 | 3 | 6779 | 0x1002be60:5473, 0x1002d6b0:1074, 0x1002dcd0:232 |
| TU32 | 8 | 124 | 0x1002ddc5:26, 0x1002dde4:26, 0x1002de03:26, 0x1002de22:26, 0x1002ddc0:5 |
| TU33 | 12 | 7235 | 0x1002e630:2715, 0x1002f0d0:1749, 0x1002de90:1002, 0x1002fdd0:793, 0x1002e420:517 |
| TU34 | 10 | 5177 | 0x10031150:1888, 0x10030220:822, 0x10030e50:764, 0x10030560:716, 0x100318b0:369 |
| TU35 | 22 | 5919 | 0x10031fc5:1752, 0x10031a5d:690, 0x10032822:634, 0x10032e09:522, 0x10032c6c:413 |
| TU36 | 12 | 1206 | 0x100333c0:338, 0x10033652:236, 0x10033512:235, 0x100332db:128, 0x10033277:100 |
| TU37 | 7 | 5328 | 0x10034000:1720, 0x10033a20:1293, 0x100346c0:811, 0x10034bf0:503, 0x100349f0:502 |
| TU38 | 16 | 2078 | 0x1003508d:533, 0x10035389:492, 0x10034df0:322, 0x10034f32:269, 0x1003559e:115 |
| TU39 | 12 | 1609 | 0x10035616:548, 0x100358b3:396, 0x10035a3f:262, 0x10035b7b:133, 0x10035c24:90 |
| TU40 | 10 | 3567 | 0x100366da:956, 0x10035e34:474, 0x10035c7e:438, 0x1003655d:381, 0x10036282:362 |
| TU41 | 74 | 10938 | 0x10038592:2095, 0x10038dc1:2077, 0x10036c3a:349, 0x1003768c:244, 0x10037e5c:217 |
| TU42 | 11 | 4658 | 0x10039aa0:977, 0x1003a2f0:953, 0x10039eb0:820, 0x1003a890:685, 0x10039910:363 |
| TU43 | 42 | 3335 | 0x1003b862:275, 0x1003bf15:257, 0x1003c016:219, 0x1003bc52:187, 0x1003bda4:187 |
| IMP | 3 | 18 | 0x1003c136:6, 0x1003c13c:6, 0x1003c142:6 |
| LIB | 14 | 897 | 0x1003c300:207, 0x1003c150:156, 0x1003c240:150, 0x1003c1f0:68, 0x1003c4f0:54 |
| CRT | 4 | 181 | 0x1003c530:91, 0x1003c5ad:56, 0x1003c596:23, 0x1003c58b:11 |
| total | 967 | 196597 | |

## Our functions retail no longer has

The scorer places 806 of our functions nowhere (`out/plan/d3d-unplaced.csv`).
"Unplaced" is not proof of absence, since an Intel-shaped function rarely
pairs with MSVC code. 127 of our files (379 functions, 204,041 bytes) have
nothing placed at all. The ones certainly gone:

* **NOLF's profiling code.** Every string these functions print is absent
  from PH's image: `CD3DModelDraw` NOLF 0x10008a40 ("DrawTris: %d"),
  0x10008e10 ("SetupVerts: %d"), 0x10017c10 ("Model::SetupTransforms failed
  for %s.", "SetupModelLight: %d (%d lights)"), 0x10015c70 ("%s: %d ticks"),
  `C100500a0` 0x1004fea4 ("Sort: %d ticks"), and the "Shadow -> Find: %d,
  Setup: %d, Texture" line of `CShadowRenderer` 0x1004b720.
* `d3d.cpp` compiles to an empty object for Primal Hunt.

Files with nothing placed, the largest first (NOLF address range from
their symbol names: these NOLF modules are the candidates for "removed in
PH", or for "rewritten past recognition"):

| our file | functions | bytes | NOLF range |
|---|--:|--:|---|
| w29_10073c00_osoiqip.cpp | 32 | 15040 | 0x10073c00-0x10079f80 |
| w63_10060010_qip.cpp | 7 | 11458 | 0x1005f7d0-0x10061a30 |
| w40_1005a960_qipni.cpp | 5 | 9197 | 0x1005a630-0x1005d3e0 |
| w34_1003e620_osoiqipsf.cpp | 10 | 7289 | 0x1003dd50-0x10040850 |
| w54_1000ae20_osoiqip.cpp | 10 | 7159 | 0x100030a0-0x1000b910 |
| w30_1000f020_osqipni.cpp | 13 | 6973 | 0x1000f020-0x100124a0 |
| big4b720.cpp | 1 | 6836 | 0x1004b720 |
| w31_10044ff0_qipni.cpp | 2 | 5789 | 0x10044ff4-0x10047150 |
| big6d910.cpp | 3 | 5544 | 0x1006d730-0x1006d910 |
| big40e10.cpp | 1 | 5226 | 0x10040e10 |
| (117 more) | 295 | 123530 | see `d3d-unplaced.csv` |

Unplaced functions per file (all 203 files):

| our file | unplaced functions | bytes | placed elsewhere |
|---|--:|--:|--:|
| w29_10073c00_osoiqip.cpp | 32 | 15040 | 0 |
| d3dr5b.cpp | 25 | 3277 | 6 |
| d3dr4a.cpp | 21 | 1356 | 6 |
| d3dr4b.cpp | 20 | 1184 | 8 |
| d3dr5a.cpp | 18 | 850 | 0 |
| d3dr4c.cpp | 17 | 397 | 6 |
| d3dbatchd.cpp | 14 | 697 | 5 |
| d3dr2b.cpp | 14 | 366 | 4 |
| d3dbatcha.cpp | 13 | 365 | 3 |
| w30_1000f020_osqipni.cpp | 13 | 6973 | 0 |
| w43_10017c10_osoiqipni.cpp | 13 | 13292 | 3 |
| d3dblit.cpp | 12 | 2198 | 10 |
| d3dr2a.cpp | 12 | 269 | 7 |
| d3dr2c.cpp | 12 | 315 | 2 |
| w28_100256d0_qipni.cpp | 12 | 8660 | 8 |
| w6_10039730_qip.cpp | 12 | 5037 | 4 |
| d3dr5c.cpp | 10 | 1289 | 1 |
| w34_1003e620_osoiqipsf.cpp | 10 | 7289 | 0 |
| w54_1000ae20_osoiqip.cpp | 10 | 7159 | 0 |
| d3dbatchb.cpp | 9 | 288 | 2 |
| d3dlow.cpp | 9 | 451 | 0 |
| d3dr2d.cpp | 9 | 229 | 2 |
| w3_10014580_os.cpp | 9 | 2352 | 3 |
| d3dbatchc.cpp | 8 | 345 | 1 |
| d3dqip4.cpp | 8 | 2945 | 2 |
| d3dr2d_os.cpp | 8 | 280 | 0 |
| w14_10011980_osoiqip.cpp | 8 | 6290 | 1 |
| w38_10028030_qip.cpp | 8 | 4252 | 0 |
| w4_1001c8d0_os.cpp | 8 | 2181 | 0 |
| big5dca0.cpp | 7 | 4896 | 0 |
| d3dr3c.cpp | 7 | 1000 | 1 |
| w23_10050e60_os.cpp | 7 | 4534 | 0 |
| w23_100525b0_os.cpp | 7 | 4840 | 0 |
| w26_10020fd0_osoiqip.cpp | 7 | 1920 | 3 |
| w54_10048180_osoiqipsf.cpp | 7 | 9327 | 1 |
| w63_10060010_qip.cpp | 7 | 11458 | 0 |
| big24090.cpp | 6 | 3840 | 2 |
| big3c40.cpp | 6 | 3690 | 0 |
| d3dr5d.cpp | 6 | 1448 | 0 |
| w13_1006c6e0_osoiqip.cpp | 6 | 2166 | 0 |
| w18_1003bb10_os.cpp | 6 | 4203 | 0 |
| w53_10070d10_os.cpp | 6 | 4643 | 0 |
| w57_10053e20_os.cpp | 6 | 3690 | 0 |
| w57_100553d0_os.cpp | 6 | 3861 | 1 |
| w57_1007bb20_qipni.cpp | 6 | 968 | 2 |
| w9_1006f540_os.cpp | 6 | 4023 | 2 |
| big051a0.cpp | 5 | 3785 | 1 |
| w10_1007cab4_os.cpp | 5 | 1871 | 0 |
| w11_1007e9f0_osoi.cpp | 5 | 1095 | 1 |
| w17_1006ad20_qip.cpp | 5 | 3905 | 1 |
| w1_10001000.cpp | 5 | 3227 | 0 |
| w22_1007dbd0_os.cpp | 5 | 242 | 1 |
| w27_10056ab0_osqipni.cpp | 5 | 7311 | 1 |
| w31_10066d10_qipni.cpp | 5 | 5148 | 1 |
| w40_1005a960_qipni.cpp | 5 | 9197 | 0 |
| w55_10016324_os.cpp | 5 | 4640 | 0 |
| d3dbatchc_o2.cpp | 4 | 92 | 2 |
| d3dbatchd_o2.cpp | 4 | 114 | 0 |
| d3dr3b.cpp | 4 | 985 | 3 |
| d3dr3d.cpp | 4 | 598 | 0 |
| w10_1007a280_os.cpp | 4 | 1104 | 1 |
| w14_10006840.cpp | 4 | 6347 | 2 |
| w19_1003b6f0_osoiqip.cpp | 4 | 1186 | 1 |
| w1_10002310.cpp | 4 | 2785 | 0 |
| w24_10059380_os.cpp | 4 | 906 | 0 |
| w24_10068c80_osqipni.cpp | 4 | 3030 | 4 |
| w2_1000a110.cpp | 4 | 942 | 0 |
| w4_10021cb0_os.cpp | 4 | 220 | 0 |
| w53_10033f30_osoiqip.cpp | 4 | 2077 | 0 |
| w57_10009860.cpp | 4 | 124 | 20 |
| w5_1002e4c0.cpp | 4 | 1222 | 0 |
| big47230.cpp | 3 | 1343 | 0 |
| big6d910.cpp | 3 | 5544 | 0 |
| d3dmid.cpp | 3 | 141 | 0 |
| d3dopt2d.cpp | 3 | 722 | 0 |
| d3dr2a_os.cpp | 3 | 133 | 0 |
| d3dr2c_os.cpp | 3 | 108 | 1 |
| d3dr3c_os.cpp | 3 | 648 | 0 |
| d3dr5c_o2.cpp | 3 | 601 | 1 |
| w10_10073400_osoiqip.cpp | 3 | 668 | 0 |
| w10_1007c250.cpp | 3 | 443 | 0 |
| w18_1001d180_os.cpp | 3 | 374 | 2 |
| w25_10037bf0_qipni.cpp | 3 | 2101 | 0 |
| w27_10015e80_osoiqip.cpp | 3 | 1091 | 2 |
| w28_1002ef30_osqipni.cpp | 3 | 4671 | 1 |
| w29_10079fd0_osoiqip.cpp | 3 | 332 | 0 |
| w31_10032a00_qip.cpp | 3 | 509 | 0 |
| w35_10043e10_qip.cpp | 3 | 3067 | 0 |
| w38_100322c0_qip.cpp | 3 | 2345 | 1 |
| w4_1001e4a0_os.cpp | 3 | 464 | 1 |
| w6_10034f10_osoi.cpp | 3 | 2765 | 2 |
| w7_10040cc0.cpp | 3 | 1087 | 0 |
| w7_100500a0_qip.cpp | 3 | 1351 | 15 |
| w8_1006a5f0_osoiqip.cpp | 3 | 1213 | 0 |
| big1ef30.cpp | 2 | 5878 | 1 |
| big729c0.cpp | 2 | 1548 | 0 |
| d3dhigh.cpp | 2 | 104 | 4 |
| d3dneutral.cpp | 2 | 186 | 12 |
| d3dqip2.cpp | 2 | 800 | 2 |
| d3dqip3.cpp | 2 | 58 | 1 |
| d3dr3a_os.cpp | 2 | 336 | 1 |
| d3dr3b_o2.cpp | 2 | 642 | 1 |
| d3dr4b_o2.cpp | 2 | 65 | 0 |
| d3dr5d_os.cpp | 2 | 589 | 0 |
| w12_7fae0.cpp | 2 | 30 | 2 |
| w14_10008a40_qipni.cpp | 2 | 830 | 0 |
| w15_1000d9f0_os.cpp | 2 | 243 | 0 |
| w15_10022e90_osoiqip.cpp | 2 | 489 | 1 |
| w17_10069a10_osoi.cpp | 2 | 948 | 0 |
| w18_10035a70_osoiqip.cpp | 2 | 72 | 0 |
| w18_10035e20_osoi.cpp | 2 | 1288 | 0 |
| w26_100371e0_osoi.cpp | 2 | 715 | 0 |
| w27_10059c80_osoi.cpp | 2 | 639 | 0 |
| w31_10044ff0_qipni.cpp | 2 | 5789 | 0 |
| w32_1001e210.cpp | 2 | 449 | 0 |
| w44_10002d40.cpp | 2 | 1391 | 0 |
| w45_1001d810_osoi.cpp | 2 | 578 | 0 |
| w4_1001cd40_osoi.cpp | 2 | 681 | 0 |
| w4_100234d0.cpp | 2 | 248 | 1 |
| w4_100265a0_qip.cpp | 2 | 340 | 0 |
| w54_1000daa0_qipsf.cpp | 2 | 4187 | 0 |
| w57_1003b1c0.cpp | 2 | 108 | 2 |
| w57_1007e830_osoiqip.cpp | 2 | 258 | 1 |
| w59_1007dd24_osoi.cpp | 2 | 287 | 0 |
| w6_1003b9c4_os.cpp | 2 | 636 | 0 |
| w7_1004b5c0_os.cpp | 2 | 121 | 0 |
| w8_10069ca0_osoiqip.cpp | 2 | 335 | 0 |
| w9_1006d210_osoi.cpp | 2 | 711 | 0 |
| wC1_r5c.cpp | 2 | 255 | 0 |
| big21f90.cpp | 1 | 2173 | 0 |
| big2b1b0.cpp | 1 | 4598 | 0 |
| big40e10.cpp | 1 | 5226 | 0 |
| big4b720.cpp | 1 | 6836 | 0 |
| big4e3e4.cpp | 1 | 4727 | 0 |
| big73140.cpp | 1 | 164 | 0 |
| big7cc90.cpp | 1 | 483 | 0 |
| big8fd0.cpp | 1 | 1938 | 0 |
| d3dbatcha_o2.cpp | 1 | 96 | 0 |
| d3dbatchb_os.cpp | 1 | 76 | 1 |
| d3ddraw_os.cpp | 1 | 20 | 0 |
| d3dmid_os.cpp | 1 | 35 | 0 |
| d3dmisc.cpp | 1 | 41 | 4 |
| d3dqip.cpp | 1 | 717 | 1 |
| d3dr3a.cpp | 1 | 100 | 4 |
| d3dr4a_o2.cpp | 1 | 24 | 3 |
| d3dr5a_o2.cpp | 1 | 46 | 6 |
| d3dr5b_o2.cpp | 1 | 82 | 1 |
| w12_15950_os.cpp | 1 | 84 | 0 |
| w12_1c7e0_os.cpp | 1 | 128 | 0 |
| w12_1d370.cpp | 1 | 13 | 0 |
| w12_21c50_osoi.cpp | 1 | 85 | 0 |
| w12_36320_os.cpp | 1 | 65 | 0 |
| w12_374a0.cpp | 1 | 24 | 0 |
| w12_3b3d0_os.cpp | 1 | 543 | 0 |
| w15_10021e50_os.cpp | 1 | 109 | 0 |
| w15_100235d0.cpp | 1 | 776 | 0 |
| w15_100316e0.cpp | 1 | 1078 | 0 |
| w16_10042e80.cpp | 1 | 1215 | 0 |
| w16_1004fea4_qip.cpp | 1 | 423 | 0 |
| w17_1001c760_os.cpp | 1 | 18 | 1 |
| w17_10020e80_osoi.cpp | 1 | 105 | 0 |
| w17_10023930.cpp | 1 | 229 | 1 |
| w17_1005f580.cpp | 1 | 271 | 0 |
| w18_1001dc70_os.cpp | 1 | 20 | 0 |
| w18_10037570.cpp | 1 | 13 | 1 |
| w19_10050450.cpp | 1 | 202 | 0 |
| w19_10050e10_os.cpp | 1 | 52 | 1 |
| w22_1007a3f0_os.cpp | 1 | 109 | 0 |
| w22_1007c580.cpp | 1 | 175 | 0 |
| w22_1007dde0_os.cpp | 1 | 265 | 1 |
| w24_10059980_osoi.cpp | 1 | 564 | 0 |
| w26_100262f0.cpp | 1 | 268 | 0 |
| w26_10031e10.cpp | 1 | 939 | 0 |
| w26_1007be90.cpp | 1 | 426 | 0 |
| w27_10050900_os.cpp | 1 | 607 | 0 |
| w29_10027f70.cpp | 1 | 74 | 0 |
| w2_10003a10.cpp | 1 | 101 | 6 |
| w2_10008da0.cpp | 1 | 356 | 0 |
| w2_1000a770.cpp | 1 | 96 | 0 |
| w2_1000a8a0_os.cpp | 1 | 467 | 0 |
| w2_1000d940_os.cpp | 1 | 91 | 0 |
| w2_1000f2c0_os.cpp | 1 | 468 | 0 |
| w32_10015d60_osoi.cpp | 1 | 193 | 0 |
| w34_10021dc0_ossf.cpp | 1 | 120 | 0 |
| w35_1004e360_os.cpp | 1 | 35 | 0 |
| w35_1007dc54_os.cpp | 1 | 108 | 0 |
| w3_1000a7d0_os.cpp | 1 | 183 | 0 |
| w3_10016234_os.cpp | 1 | 210 | 0 |
| w44_10001670.cpp | 1 | 643 | 0 |
| w48_1006d3e0_os.cpp | 1 | 106 | 0 |
| w4_1001eb90_os.cpp | 1 | 955 | 0 |
| w4_10023fb0.cpp | 1 | 34 | 1 |
| w56_1002ec60_osoiqipsf.cpp | 1 | 552 | 0 |
| w56_10073700_osoiqipsf.cpp | 1 | 1118 | 0 |
| w59_10073640_osoi.cpp | 1 | 123 | 0 |
| w5_10030f30.cpp | 1 | 1527 | 0 |
| w7_10047d80_os.cpp | 1 | 871 | 0 |
| w7_1005a550.cpp | 1 | 165 | 0 |
| w8_1005f9a0.cpp | 1 | 246 | 0 |
| w8_1005fb70.cpp | 1 | 984 | 0 |
| w8_10064800.cpp | 1 | 552 | 0 |
| w8_10069ed0_osoi.cpp | 1 | 783 | 0 |
| w8_1006a860_os.cpp | 1 | 989 | 1 |
