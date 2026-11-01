# Which compiler built what

Every retail PE carries a Rich header, the linker's inventory of the tool
builds that produced its objects (`python3 tools/richhdr.py <image>`). For
AvP2 1.0.9.6 it is all Visual C++ 6.0, but not one revision: the binaries
were built at different times, from October 2001 to April 2002, and the
headers show four back ends (`c2.dll` builds, which decide the codegen):
**8168** (VC6 RTM), **8447** (SP3), **8966** (SP5) and **9044** (SP5 with
the Processor Pack), all in `toolchains/`. Where a revision or flag set has
been *proven* by byte-matching, this file says so; until then the
attribution is the header's own testimony.

| Binary | Link date | Monolith's objects (C++ unless said) | Also | Built here with |
|---|---|---|---|---|
| `lithtech.exe` | 2002-04-17 | **9044** x157, 8966 x8 | `libcmt.lib` (C 8047 x173, C++ 8047 x12, masm 6.13 x37); one C object of **8799**; one C++ and one C object of cl 13.00.9178 and 4 import descriptors of lib 7.00.9210 (a DirectX SDK's libraries); 4 without a `@comp.id` | -- (no build yet) |
| `server.dll` | 2002-04-17 | **9044** x96, 8966 x8 | `libcmt.lib` (C 8047 x163, C++ 8047 x12) | -- |
| `d3d.ren` | 2002-02-07 | C++ 8168 x73 | C 8168 x120 (`libcmt.lib`, RTM's); 3 objects of tool 0x25 build 8755 and one C++ of cl 13.00.8830, both unidentified; `cvtres` 5.00.1720 | -- |
| `ltmsg.dll` | 2001-02-05 | C++ 8168 x1, 8447 x1 | C 8168 x23 (`libcmt.lib`) | -- |
| `Launch.dll` | 1999-08-16 | C++ 8168 x10 | NOLF's launcher DLL, a Debug build (`libcmtd.lib`: C 8168 x85); one object without a `@comp.id` | -- |
| `AVP2.exe` | 2001-10-15 | **9044** x18, 8966 x1 | MFC statically (`nafxcw.lib`, C++ 8569 x55, SP5's MFC), `libcmt.lib` (C 8047 x134); 18 without a `@comp.id` | -- |
| `AVP2Up.dll` | 2001-08-17 | 8966 x2 | resources and `libcmt.lib` | -- |
| `AVP2Serv.exe` | 2002-02-07 | 8168 x136 | MFC statically (8168's), `libcmt.lib` (C 8168 x169); 57 objects without a `@comp.id`; import libraries of `lib` 5.12.8022 as well as 8034 | -- |
| `CShell.dll` | 2002-03-01 | **8168** x142, 8966 x114 | `libcmt.lib` (C 8047 x174), C 8168 x7 | -- |
| `Object.lto` | 2002-02-28 | **8168** x245, 8966 x12 | `libcmt.lib` (C 8047 x151), C 8168 x1 | -- |
| `CRes.dll`, `SRes.dll` | 2002-04-17 | **9044** x1 | resources and `libcmt.lib` | -- |

## Notes

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
