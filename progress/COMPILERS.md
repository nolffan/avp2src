# Which compiler built what

Every retail PE carries a Rich header, the linker's inventory of the tool
builds that produced its objects (`python3 tools/richhdr.py <image>`). It is
all Visual C++ 6.0, but not one revision: the binaries were built at
different times, from October 2001 (AvP2's launcher) to June 2002 (Primal
Hunt's game DLLs), and the headers show four back ends (`c2.dll` builds,
which decide the codegen): **8168** (VC6 RTM), **8447** (SP3), **8966**
(SP5) and **9044** (SP5 with the Processor Pack), all in `toolchains/`.
Where a revision or flag set has been *proven* by byte-matching, this file
says so; until then the attribution is the header's own testimony.

## Primal Hunt 1.0.2.2

Third Law's builds are SP5's (8966) throughout, the engine and the renderer
included, where 1.0.9.6's were the Processor Pack's and RTM's.

| Binary | Link date | Its own objects (C++ unless said) | Also | Built here with |
|---|---|---|---|---|
| `lithtech.exe` | 2002-05-20 | **8966** x165 | `libcmt.lib` (C 8047 x172, C++ 8047 x14, masm 6.13 x39); one C object of **8799**; one C++ and one C object of cl 13.00.9178 and 4 import descriptors of implib 7.00.9210 (a DirectX SDK's libraries); 4 without a `@comp.id` | -- (no build yet) |
| `server.dll` | 2002-05-07 | **8966** x104 | `libcmt.lib` (C 8047 x166, C++ 8047 x14) | -- |
| `d3d.ren` | 2002-06-03 | **8966** x69 | `libcmt.lib` (C 8047 x121, C++ 8047 x8); one C++ object of cl 13.00.9178 and 3 import descriptors of implib 7.00.9210 | -- |
| `cdaudio.dll` | 2002-05-07 | **8966** x4 | MFC statically (`nafxcw.lib`, C++ 8569 x38), `libcmt.lib` (C 8047 x76); 6 without a `@comp.id` | -- |
| `ltmsg.dll` | 2002-03-26 | 8966 x1 | resources and `libcmt.lib` | -- |
| `PrimalHunt.exe` | 2002-06-06 | **8966** x19 | MFC statically (`nafxcw.lib`, C++ 8569 x55), `libcmt.lib` (C 8047 x134); 18 without a `@comp.id`; the wrapper's `.cms_t`/`.cms_d`, as `AVP2.exe` | as `AVP2.exe`, with cl 8966 (every readable function but one identical) |
| `AVP2XUp.dll` | 2002-05-02 | 8966 x2 | resources and `libcmt.lib` | -- |
| `AVP2XServ.exe` | 2002-06-21 | **8966** x17; C++ 8168 x25 and C 8168 x1, 8447 x1 are SP5's own libraries' (our link has exactly those, its own objects all 8966) | MFC statically (C++ 8569 x86), `libcmt.lib` (C 8047 x167); 57 without a `@comp.id`; import libraries of `lib` 5.12.8022 and 8034 | `ServerApp_PH` (the server's Release with `PRIMAL_HUNT`): cl 8966, link 6.00.8447, SP5's MFC and `libcmt.lib`, the release's lithshared libraries and RegMgr32.lib (our link: 8966 x15) |
| `PHCShell.dll` | 2002-06-21 | **8966** x244, C++ 8168 x21; C 8966 x6, 8168 x1 | `libcmt.lib` (C 8047 x176, C++ 8047 x16) | -- |
| `PHObject.lto` | 2002-06-21 | **8966** x273, C++ 8168 x24; C 8168 x1 | `libcmt.lib` (C 8047 x153, C++ 8047 x16) | -- |
| `PHCRes.dll`, `PHSRes.dll` | 2002-06-21, -20 | 8966 x1 | resources and `libcmt.lib` | -- |

`Launch.dll` is the same file as AvP2's (below).

## Aliens versus Predator 2 1.0.9.6

Its engine (`lithtech.exe`, `server.dll`, `d3d.ren`, `ltmsg.dll`) is
superseded by Primal Hunt's and not a target; its rows stay as a witness.

| Binary | Link date | Monolith's objects (C++ unless said) | Also | Built here with |
|---|---|---|---|---|
| `lithtech.exe` | 2002-04-17 | **9044** x157, 8966 x8 | `libcmt.lib` (C 8047 x173, C++ 8047 x12, masm 6.13 x37); one C object of **8799**; one C++ and one C object of cl 13.00.9178 and 4 import descriptors of lib 7.00.9210 (a DirectX SDK's libraries); 4 without a `@comp.id` | -- (no build yet) |
| `server.dll` | 2002-04-17 | **9044** x96, 8966 x8 | `libcmt.lib` (C 8047 x163, C++ 8047 x12) | -- |
| `d3d.ren` | 2002-02-07 | C++ 8168 x73 | C 8168 x120 (`libcmt.lib`, RTM's); 3 objects of tool 0x25 build 8755 and one C++ of cl 13.00.8830, both unidentified; `cvtres` 5.00.1720 | -- |
| `ltmsg.dll` | 2001-02-05 | C++ 8168 x1, 8447 x1 | C 8168 x23 (`libcmt.lib`) | -- |
| `Launch.dll` | 1999-08-16 | C++ 8168 x10 | NOLF's launcher DLL, a Debug build (`libcmtd.lib`: C 8168 x85); one object without a `@comp.id` | -- |
| `AVP2.exe` | 2001-10-15 | **9044** x18, 8966 x1 | MFC statically (`nafxcw.lib`, C++ 8569 x55, SP5's MFC), `libcmt.lib` (C 8047 x134); 18 without a `@comp.id` | cl 9044 for its 18 sources, with the precompiled header (all but `LauncherDlg.cpp`: proven, the inlined MFC destructors' unwind states differ without it); the 8966 object is RegMgr32's; link 6.00.8447, SP5's MFC and `libcmt.lib` (every readable function but two identical) |
| `AVP2Up.dll` | 2001-08-17 | 8966 x2 | resources and `libcmt.lib` | -- |
| `AVP2Serv.exe` | 2002-02-07 | 8168 x136 | MFC statically (8168's), `libcmt.lib` (C 8168 x169); 57 objects without a `@comp.id`; import libraries of `lib` 5.12.8022 as well as 8034 | cl, link and MFC 8168, Release; its butemgr.lib (MFC's build), cryptmgr.lib and StdLith.lib built with cl 8168 too, as the header's lack of any 8966 object says (our link: 8168 x136, as retail) |
| `CShell.dll` | 2002-03-01 | **8168** x142, 8966 x114 | `libcmt.lib` (C 8047 x174), C 8168 x7 | cl 8168; link 6.00.8447, headers and `libcmt.lib` of msvc-6.00-8966; Final (62 % identical as the release has it) |
| `Object.lto` | 2002-02-28 | **8168** x245, 8966 x12 | `libcmt.lib` (C 8047 x151), C 8168 x1 | as `CShell.dll` (89 % identical as the release has it) |
| `CRes.dll`, `SRes.dll` | 2002-04-17 | **9044** x1 | resources and `libcmt.lib` | as `CShell.dll`, rc of msvc-6.00-9782 (every resource identical) |

## Notes

* **Primal Hunt's game DLLs mix the same two revisions, in other
  proportions.** Their 8966 objects are both the `lithshared` libraries (the release's
  prebuilt ones: `tools/libscan.py` places ButeMgr, CryptMgr, GameSpy,
  LithFontMgr, LTGUIMgr and StdLith functions in `PHCShell.dll` as in
  1.0.9.6's `CShell.dll`; WONAPI's far fewer -- about 280 against 978 --
  and linked above the runtime, not below it) and Third Law's own code; a residue of 21 C++
  objects of 8168 in the client and 24 in the server objects is still
  Monolith's compiler. Which files those are is for the scoreboard to show.
* **Primal Hunt's renderer is Microsoft's.** Its Rich header has none of
  the shape of NOLF's and 1.0.9.6's Intel C/C++ renderers: 8966 throughout.
  nolfsrc's renderer, Intel-compiled, is a guide to its source, not to its
  codegen.

* **The game DLLs mix two revisions.** Their own code is 8168's; their 8966
  objects are the `lithshared` libraries, which the source release ships
  prebuilt in `LT2/lithshared/libs/{release,debug}/` (`ButeMgr.lib`,
  `CryptMgr.lib`, `GameSpyClientMgr.lib`, ...): `tools/compid.py` gives
  every one of their objects `@comp.id` build 8966. Whether those prebuilt
  libraries are the very ones linked (they are dated August 2002, after the
  DLLs) is for the scoreboard to show.
* **The 1.0.9.6 engine is the Processor Pack's.** `lithtech.exe` and
  `server.dll` are mostly 9044 objects, the compiler of the AvP2 "Talon"
  DEdit and ModelEdit (`toolchains/COMPILERS.md`). The stray C object of
  8799 in `lithtech.exe` is the one NOLF's engine has too, for which no
  compiler is held. The 8966 objects are likely `lithshared` again.
* **Primal Hunt's engine is built with `/GX`; AvP2 1.0.9.6's is not.**
  Primal Hunt's `lithtech.exe` and `server.dll` carry exception frames on
  functions with destructible locals where AvP2's same functions have none
  (lithtech.exe 0x00452310, `~CModelSub1D4`; server.dll 0x1000c130, the
  `CWorldSurface` constructor, against AvP2's frameless 0x1000be60).
  Measured: `/GX` on Primal Hunt's objects took `lithtech.exe` from 2,456
  to 2,724 identical functions and `server.dll` from 1,566 to 1,714; on
  AvP2's it loses matches. StdLith's allocator code (`stdlithalloc.cpp`)
  is built with it too (~LAllocSimpleBlock at 0x004bf320 has a frame).
* **Older binaries ship beside the patch's.** `ltmsg.dll` (February 2001,
  but not NOLF's file), `Launch.dll` (NOLF's, 1999; Blood II ships an
  older one, Primal Hunt this one), `AVP2Up.dll` and the launcher are the
  discs' own, the same in the European and USA releases; the patch replaced
  only the engine, the renderer, the dedicated server and the game DLLs.
* `d3d.ren`'s header is the shape of NOLF's renderer's, which nolfsrc
  proved to be Intel C/C++ 4.0 (a Rich header records only Microsoft
  tools); the 0x25/8755 and 13.00.8830 rows are to be identified.
* nolfsrc's codegen lore (its `guides/`, and `toolchains/COMPILERS.md`'s
  NOLF map: 8168, 8447, Intel 4.0) is the nearest; NOLF2's (8966) covers
  the libraries. Neither carries over unchecked.
