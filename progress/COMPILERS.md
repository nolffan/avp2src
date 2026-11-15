# Which compiler built what

Each retail PE has a Rich header. This header is the list that the linker
makes of the tool builds that made its objects
(`python3 ext/tools/richhdr.py <image>`). All the binaries are Visual C++
6.0, but not all of one revision. The binaries were built at different
times, from October 2001 (the AvP2 launcher) to June 2002 (the Primal Hunt
game DLLs). The headers show four back ends (`c2.dll` builds, which decide
the codegen):

* **8168** (VC6 RTM)
* **8447** (SP3)
* **8966** (SP5)
* **9044** (SP5 with the Processor Pack)

All four are in `ext/toolchains/`. Where byte matches have *proven* a
revision or a flag set, this file says so. Until then, the attribution
comes only from the header.

## Primal Hunt 1.0.2.2

Third Law built all its binaries with SP5 (8966), also the engine and the
renderer. The 1.0.9.6 builds of these were Processor Pack and RTM builds.

| Binary | Link date | Its own objects (C++ unless said) | Also | Built here with |
|---|---|---|---|---|
| `lithtech.exe` | 2002-05-20 | **8966** x165 | `libcmt.lib` (C 8047 x172, C++ 8047 x14, masm 6.13 x39). One C object of **8799**. One C++ object and one C object of cl 13.00.9178, and 4 import descriptors of implib 7.00.9210 (the libraries of a DirectX SDK). 4 objects without a `@comp.id`. | -- (no build yet) |
| `server.dll` | 2002-05-07 | **8966** x104 | `libcmt.lib` (C 8047 x166, C++ 8047 x14) | -- |
| `d3d.ren` | 2002-06-03 | **8966** x69 | `libcmt.lib` (C 8047 x121, C++ 8047 x8). One C++ object of cl 13.00.9178, and 3 import descriptors of implib 7.00.9210. | -- |
| `cdaudio.dll` | 2002-05-07 | **8966** x4 | MFC statically (`nafxcw.lib`, C++ 8569 x38), and `libcmt.lib` (C 8047 x76). 6 objects without a `@comp.id`. | -- |
| `ltmsg.dll` | 2002-03-26 | 8966 x1 | Resources and `libcmt.lib`. | -- |
| `PrimalHunt.exe` | 2002-06-06 | **8966** x19 | MFC statically (`nafxcw.lib`, C++ 8569 x55), and `libcmt.lib` (C 8047 x134). 18 objects without a `@comp.id`. The `.cms_t`/`.cms_d` sections of the wrapper, as in `AVP2.exe`. | As `AVP2.exe`, with cl 8966. All readable functions but one are identical. |
| `AVP2XUp.dll` | 2002-05-02 | 8966 x2 | Resources and `libcmt.lib`. | -- |
| `AVP2XServ.exe` | 2002-06-21 | **8966** x17. C++ 8168 x25, and C 8168 x1 and 8447 x1, come from the libraries of SP5 itself. (Our link has exactly those, and all its own objects are 8966.) | MFC statically (C++ 8569 x86), and `libcmt.lib` (C 8047 x167). 57 objects without a `@comp.id`. Import libraries of `lib` 5.12.8022 and 8034. | `ServerApp_PH` (the Release configuration of the server, with `PRIMAL_HUNT`). cl 8966, link 6.00.8447, the MFC and `libcmt.lib` of SP5, the lithshared libraries of the release, and RegMgr32.lib. Our link: 8966 x15. |
| `PHCShell.dll` | 2002-06-21 | **8966** x244, C++ 8168 x21. C 8966 x6, 8168 x1. | `libcmt.lib` (C 8047 x176, C++ 8047 x16) | -- |
| `PHObject.lto` | 2002-06-21 | **8966** x273, C++ 8168 x24. C 8168 x1. | `libcmt.lib` (C 8047 x153, C++ 8047 x16) | -- |
| `PHCRes.dll`, `PHSRes.dll` | 2002-06-21, -20 | 8966 x1 | Resources and `libcmt.lib`. | -- |

`Launch.dll` is the same file as the AvP2 `Launch.dll` (see below).

## Aliens versus Predator 2 1.0.9.6

Its engine (`lithtech.exe`, `server.dll`, `d3d.ren`, `ltmsg.dll`) is a
target again, because the AvP2 game DLLs cannot run on the Primal Hunt
engine (docs/PLAN.md, package 7).

| Binary | Link date | Monolith's objects (C++ unless said) | Also | Built here with |
|---|---|---|---|---|
| `lithtech.exe` | 2002-04-17 | **9044** x157, 8966 x8 | `libcmt.lib` (C 8047 x173, C++ 8047 x12, masm 6.13 x37). One C object of **8799**. One C++ object and one C object of cl 13.00.9178, and 4 import descriptors of lib 7.00.9210 (the libraries of a DirectX SDK). 4 objects without a `@comp.id`. | cl 9044 for the engine, and for lith, RezMgr and StdLith (with their release flags). The prebuilt STLport `WONAPI.lib` of the release (8966). link 6.00.8447 with `/FORCE` (`avp2-engine-link`). |
| `server.dll` | 2002-04-17 | **9044** x96, 8966 x8 | `libcmt.lib` (C 8047 x163, C++ 8047 x12) | As `lithtech.exe`. It uses the release builds of lith, RezMgr and StdLith, not the debug builds that the Primal Hunt link uses. |
| `d3d.ren` | 2002-02-07 | C++ 8168 x73 | C 8168 x120 (`libcmt.lib`, the RTM version). 3 objects of tool 0x25 build 8755, and one C++ object of cl 13.00.8830. By analogy, these are the ddraw.lib and dxguid.lib of the DirectX 8.0 SDK. `cvtres` 5.00.1720. | The Primal Hunt source `src/engine/d3d/ph/`, with cl 8168 and the flags of each file. Without `PRIMAL_HUNT` and without `fogvolume.cpp` (`avp2-d3d`). |
| `ltmsg.dll` | 2001-02-05 | C++ 8168 x1, 8447 x1 | C 8168 x23 (`libcmt.lib`) | -- |
| `Launch.dll` | 1999-08-16 | C++ 8168 x10 | The launcher DLL of NOLF, a Debug build (`libcmtd.lib`: C 8168 x85). One object without a `@comp.id`. | -- |
| `AVP2.exe` | 2001-10-15 | **9044** x18, 8966 x1 | MFC statically (`nafxcw.lib`, C++ 8569 x55, the MFC of SP5), and `libcmt.lib` (C 8047 x134). 18 objects without a `@comp.id`. | cl 9044 for its 18 sources. All of them but `LauncherDlg.cpp` use the precompiled header. This is proven: without the precompiled header, the unwind states of the inlined MFC destructors are different. The 8966 object comes from RegMgr32. link 6.00.8447, and the MFC and `libcmt.lib` of SP5. All readable functions but two are identical. |
| `AVP2Up.dll` | 2001-08-17 | 8966 x2 | Resources and `libcmt.lib`. | -- |
| `AVP2Serv.exe` | 2002-02-07 | 8168 x136 | MFC statically (the MFC of 8168), and `libcmt.lib` (C 8168 x169). 57 objects without a `@comp.id`. Import libraries of `lib` 5.12.8022, and also of 8034. | cl, link and MFC 8168, Release. Its butemgr.lib (the MFC build), cryptmgr.lib and StdLith.lib are also built with cl 8168. The header has no 8966 object, and this shows that. Our link: 8168 x136, the same as retail. |
| `CShell.dll` | 2002-03-01 | **8168** x142, 8966 x114 | `libcmt.lib` (C 8047 x174), and C 8168 x7 | cl 8168 for 105 of its 188 sources, and cl 8966 for the other 83 (`build/build.mk`, `CSHELL_8966`). This is proven for each file: each file was compiled both ways. link 6.00.8447, and the headers and `libcmt.lib` of msvc-6.00-8966. The Final configuration. With this, the header of our link is C++ 8168 x127, 8966 x129 and C 8966 x6. The retail header is 142, 114 and C 8168 x7. The 15 extra C++ objects and the 6 extra C objects come from libraries. Thus some of the libraries that retail linked were 8168 builds. The 5-6 C objects and 3 C++ objects of GameSpyClientMgr can be part of the count. The 11 objects of LithTrackMgrClient are not part of it: they are 8966 builds (`@comp.id` 0x000b2306), and ours are now 8966 builds too. Of the library objects, eight are proven 8168 builds in the two AvP2 game DLLs, where the prebuilt libraries of the release have them as 8966 builds: ltguieditctrl, ltguicyclectrl, ltguicolumntextctrl, ltguimessagebox, LithTrackBarTrack, LithTrackList, LithTrackDrawObj and butemgr. GameSpyMgr in `Object.lto` is also one. Each of these has functions that are identical only with 8168 (`build/build.mk`, "AvP2 1.0.9.6's builds of the game DLLs' libraries"). The AvP2 links use these 8168 builds. The 8966 builds are scored against the prebuilt libraries. |
| `Object.lto` | 2002-02-28 | **8168** x245, 8966 x12 | `libcmt.lib` (C 8047 x151), and C 8168 x1 | As `CShell.dll`. With the release source as it is, 89 % is identical. `AINodeMgr.cpp` is one of the 8966 objects (`build/build.mk`, `CL_Object__AINodeMgr`). |
| `CRes.dll`, `SRes.dll` | 2002-04-17 | **9044** x1 | Resources and `libcmt.lib`. | As `CShell.dll`, with the rc of msvc-6.00-9782. All resources are identical. |

## Notes

* **Primal Hunt's game DLLs mix the same two revisions, in other
  proportions.** Their 8966 objects are the `lithshared` libraries and
  also the code of Third Law. The libraries are the prebuilt libraries of
  the release. `ext/tools/libscan.py` places ButeMgr, CryptMgr, GameSpy,
  LithFontMgr, LTGUIMgr and StdLith functions in `PHCShell.dll`, as in the
  1.0.9.6 `CShell.dll`. It places far fewer WONAPI functions (about 280
  against 978), and these are linked above the runtime, not below it. A
  remainder of 21 C++ objects of 8168 in the client and 24 in the server
  objects is still the compiler of Monolith. The scoreboard is to show
  which files these are.
* **Primal Hunt's renderer is Microsoft's.** Its Rich header does not have
  the shape of the Intel C/C++ renderers of NOLF and 1.0.9.6: it is 8966
  throughout. The nolfsrc renderer, compiled with Intel C/C++, is a guide to
  the source of this renderer, but not to its codegen.

* **The game DLLs mix two revisions.** Their own code is 8168. Their 8966
  objects are the `lithshared` libraries. The source release has these
  libraries prebuilt in `LT2/lithshared/libs/{release,debug}/`
  (`ButeMgr.lib`, `CryptMgr.lib`, `GameSpyClientMgr.lib`, ...).
  `ext/tools/compid.py` gives each of their objects `@comp.id` build 8966.
  The prebuilt libraries are dated August 2002, after the DLLs. The
  scoreboard is to show if they are the same libraries that were linked.
* **The 1.0.9.6 engine is the Processor Pack's.** `lithtech.exe` and
  `server.dll` are mostly 9044 objects. 9044 is the compiler of the AvP2
  "Talon" DEdit and ModelEdit (`ext/toolchains/COMPILERS.md`). The stray C
  object of 8799 in `lithtech.exe` is the same one that the NOLF engine
  has, and there is no compiler for it here. The lithshared libraries of
  this engine are also 9044 builds. When lith, RezMgr and StdLith are
  compiled with 9044 and their release flags, they are byte-identical in
  the two images. For example, `CRezItm::InitRezItm` (lithtech.exe
  0x004afd00) inlines its `strcpy` as 9044 does at /O2, not as 8966 does.
  The prebuilt (8966) builds of the release are not byte-identical. The 8
  objects of 8966 are the WON objects. The prebuilt STLport `WONAPI.lib`
  of the release places more of its functions in the two images than the
  same source built with 9044 (840 against 689 in lithtech.exe). The Primal
  Hunt images have the same totals (165 and 104 C++ objects, all 8966).
* **Primal Hunt's engine is built with `/GX`; AvP2 1.0.9.6's is not.**
  The Primal Hunt `lithtech.exe` and `server.dll` have exception frames on
  functions with destructible locals. The same functions in AvP2 have no
  frames. Examples are lithtech.exe 0x00452310, `~CModelSub1D4`, and
  server.dll 0x1000c130, the `CWorldSurface` constructor. The AvP2
  function at 0x1000be60 has no frame. Measured results: `/GX` on the
  Primal Hunt objects increased the identical functions of `lithtech.exe`
  from 2,456 to 2,724, and of `server.dll` from 1,566 to 1,714. On the AvP2
  objects, `/GX` loses matches. The StdLith allocator code
  (`stdlithalloc.cpp`) is also built with `/GX` (~LAllocSimpleBlock at
  0x004bf320 has a frame).
* **Older binaries ship beside the patch's.** These files come from the
  discs, and they are the same in the European and USA releases:
  * `ltmsg.dll` (February 2001, but not the NOLF file),
  * `Launch.dll` (the NOLF file, 1999). Blood II ships an older
    `Launch.dll`, and Primal Hunt ships this one.
  * `AVP2Up.dll`,
  * the launcher.

  The patch replaced only the engine, the renderer, the dedicated server
  and the game DLLs.
* The AvP2 1.0.9.6 `d3d.ren` is Microsoft's, cl 8168. It is not Intel
  C/C++, as the NOLF renderer was. It does not have the lea padding of
  Intel (2 such sequences, against 313 in NOLF). Its C++ 8168 x73 are its
  own objects. The NOLF header has an unstamped row of 80 for its Intel
  objects, and the AvP2 header has none. It is the source of the Primal
  Hunt renderer, a few months earlier. It has the same objects in the same
  link order, without the fog volumes. 1,180 functions are byte-identical
  between the two images. The 0x25/8755 x3 and 13.00.8830 x1 rows are, by
  analogy with the DirectX 8.1 rows of Primal Hunt, the `ddraw.lib` import
  objects and `dxguid.lib` of the DirectX 8.0 SDK. The 8.0 SDK is not
  here, so this is not confirmed.
* The codegen knowledge of nolfsrc (its `guides/`, and the NOLF map in
  `ext/toolchains/COMPILERS.md`: 8168, 8447, Intel 4.0) is the nearest
  guide. The NOLF2 knowledge (8966) covers the libraries. Do not use
  either of them without a check.
