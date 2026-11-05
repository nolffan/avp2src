"""What the scoreboard scores: the binaries of Aliens versus Predator 2's
last release, Primal Hunt 1.0.2.2 (its engine, launcher, dedicated server
and game DLLs), and those of AvP2 1.0.9.6 that Primal Hunt does not
replace (the game DLLs, launcher and dedicated server that play AvP2's own
campaigns) -- and where each one's own code, and ours, is.

tools/progress.py and tools/ratchet.py read this file and nothing else
about the project, so they know no binary by name: a binary is added here,
not in the tools. Plain data, with the evidence for every address beside
it (the CSVs in this directory are the rest of the evidence).

TARGETS, one dict per shipped binary, in the README's order:

  label      the binary's name, as the README and functions.csv show it
  what       what it is, for the README
  kind       "image": scored function by function against the shipped
             image's code; "resources": a resource-only image, scored
             resource by resource and left out of the total
  shipped    the retail binary, extracted by build/reference.sh
  stem       the prefix of its files here (<stem>-screen-exclude.csv, ...)
             and in out/reference/ (<stem>-functions.csv)
  built      our build of it: a path or glob under the repository, or None
             while it has no source. Its link is read from the response
             file beside it (<built>.rsp, and <built>.cmd for the
             directory the link ran in): every object and library of ours
             it names is scored, and its .map places our functions. A
             target without a build scores as all unwritten
  optional   True: a missing build is expected (no source yet, or another
             project's output still to come), not an error
  app        [(lo, hi)]: the image's own code -- everything the scoreboard
             counts. Above it is the C runtime (libcmt.lib, linked
             statically, and the import thunks) and the exception-handling
             funclets of .text$x: status `crt`, listed, not counted. A
             range after the first is code of the image's own that the
             linker placed above the runtime
  middleware [(lo, hi)]: third-party code linked inside `app`: status
             `middleware`, listed, not counted
  encrypted  [(lo, hi)]: code the retail image keeps encrypted (none in AvP2):
             status `encrypted`, listed, not counted -- nothing can be
             compared there
  third_party  link inputs of ours (file names) that build `middleware`:
             left out of our side, so they neither match nor count as
             unplaced
  cites      True when src/ cites this image's addresses in comments: a
             cited address of an unmatched function is `attempted`
  cited_src  the source trees whose comments cite it (default CITED_SRC):
             per target, because the DLLs share the 0x10000000 range
  purecall   the shipped address of the CRT's `_purecall` (every abstract
             vtable's filler), where tools/vtabcheck.py cannot find its
             `jmp [IAT]` thunk itself

The reconstruction tools (tools/tgt.py) read the same entries: a tool is
given a target by label or stem (`Lithtech`, `server`), and its image,
build, link inputs and regions come from here.

Optional per target, for what NOLF's scoreboard needs and this one does
not yet: icl (Intel C/C++ padding and /Qip names), linked_oracle (compare
relocation-free functions against our linked image too), lib_range,
lib_block, own_names / own_symbols (names that say a function's shipped
address) -- see tools/progress.py.

How the ranges were found: `tools/libscan.py <image> <lib>...` puts each
library's functions in the image. Every binary of AvP2's and Primal
Hunt's links the C runtime statically, as NOLF's do: `libcmt.lib`, and
MFC's `nafxcw.lib` in the launchers, the dedicated servers and
cdaudio.dll. The Rich headers say which VC6 tree's: the C objects of build
8047 (SP5's and later trees' CRT: msvc-6.00-8966's libraries were
searched) in all of Primal Hunt's, and in AvP2's launcher, game DLLs and
resource DLLs; of 8168 (msvc-6.00-8168's) in AvP2's dedicated server and
in Launch.dll, whose debug build links libcmtd.lib. The image's own code ends
where the runtime's first member starts: as in NOLF's engine, `_ftol`
(ftol.obj), or an import-thunk run (`jmp [IAT]`, 6 bytes each) just before
it. Library members are packed; code compiled /O2 ends padded to 16 bytes,
so a packed stretch just below the first hit (noted per target) may be
runtime too, or a library of Monolith's built /O1. Until a binary has a link
of ours, its boundary is the first hit: marked "provisional" below, to be
pinned from our .map (which names the library of every function) once there
is one, as nolfsrc did.
"""

import os

REFERENCE = "out/reference"     # build/reference.sh writes it
OUT = "out/progress"            # each run's functions.csv and summary.json
DATA = "progress"               # the recorded baseline and the evidence

# Every hex address cited in these source trees marks the address
# `attempted` in a target with `cites` (an address a reconstruction has
# been written for), unless the target names its own `cited_src`.
CITED_SRC = []

# The lithshared libraries we build (build/build.mk, "The lithshared
# libraries, ours"), into the paths the project files link them from: each
# scored object by object against the release's prebuilt build of it
# (progress/reference/, checked against shipped-libs.sha256), which holds
# more than any one image takes from it.
OUR_LIBS = ["StdLith", "CryptMgr", "lith", "RezMgr", "RegMgr", "MFCStub", "ButeMgrNoMFC"]
LIBS = [(n, "src/LT2/lithshared/libs/Release/%s.lib" % n) for n in OUR_LIBS]
# WONAPI.lib, built from the release's own source (build/build.mk,
# "WONAPI.lib"): the release's prebuilt build of it, scored against that,
# beside the copy the game DLLs still link (so it stays in PREBUILT_LIBS
# below). Being a LIBS label also makes Primal Hunt's engine's build of it
# (VC6's STL, not STLport: lithtech.exe's partial_objs) library code there,
# placed where it is linked and never unplaced where it is not.
LIBS.append(("WONAPI", "src/LT2/lithshared/libs/Release/wonapi/WONAPI.lib"))
# Redmgr.lib, reconstructed for cdaudio.dll (build/build.mk, "cdaudio.dll"):
# scored against the release's prebuilt build; the game links the prebuilt
# one still, so it stays in PREBUILT_LIBS.
LIBS.append(("Redmgr", "src/lithshared/redmgr/Release/Redmgr.lib"))
# The debug builds of StdLith and RezMgr (build/build.mk, "The debug
# builds"), which Primal Hunt's server.dll links: each scored against the
# release's prebuilt debug build of it (libs/debug/).
DEBUG_LIBS = ["StdLith", "RezMgr"]
LIBS += [(n + "Debug", "src/LT2/lithshared/libs/Debug/%s.lib" % n) for n in DEBUG_LIBS]
SHIPPED_LIB = "LT2/lithshared/libs/release/{}.lib"
SHIPPED_AS = {n + "Debug": "../debug/%s.lib" % n for n in DEBUG_LIBS}
SHIPPED_ENV = "AVP2_SOURCE"     # build/progress.sh: progress/reference

DRIVER_OBJS = {"sdkforce.obj"}

# The release's prebuilt lithshared libraries (and the dedicated server's
# import library for server.dll), which our links take from
# progress/reference/ until each is built from source here
# (build/build.mk, "The prebuilt libraries"): Monolith's code, left out of
# our side, so that what they put in our images neither matches nor counts
# as ours. A library comes off this list when we build it.
PREBUILT_LIBS = sorted(f for f in os.listdir(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "reference/LT2/lithshared/libs/release"))
    if f[:-4] not in OUR_LIBS) + ["server.lib"]

TARGETS = [
    # ---- the engine: Primal Hunt's ---------------------------------------
    # Primal Hunt's engine (lithtech.exe 2002-05-20, server.dll 2002-05-07,
    # d3d.ren 2002-06-03) is the newest; it supersedes 1.0.9.6's
    # (2002-04-17), which is kept as a witness, not scored. No section is
    # protected: .text, .rdata, .data, .rsrc, every import through the IAT.
    # All of it is VC6 SP5 (C++ 8966), the renderer included.
    dict(label="lithtech.exe", what="the engine", kind="image",
         shipped=REFERENCE + "/lithtech.exe", stem="lithtech",
         # nolfsrc's engine compiled for Primal Hunt, and linked with /FORCE
         # for the scoreboard (build/build.mk, engine-link: `built`, its .rsp
         # and .map, through which the small functions are verified); its
         # objects alone (partial_objs) until that link has been made
         # and its WON client (0x004aab40..0x004ba2a0, the WON templates
         # the engine's own objects instantiate elsewhere): WONAPI.lib from
         # the release's source, with VC6's STL (build/build.mk, "WONAPI.lib")
         partial_objs=["src/engine/lithtech/Release/*.obj",
                       "src/engine/lithtech/Release/wonapi/WONAPI.lib",
                       # and the release lithshared libraries it links
                       # after WON: RezMgr, lith, then StdLith
                       "src/LT2/lithshared/libs/Release/RezMgr.lib",
                       "src/LT2/lithshared/libs/Release/lith.lib",
                       "src/LT2/lithshared/libs/Release/StdLith.lib"],
         # each library's block: their generic shapes (deleting
         # destructors, std::string's and the collections' members) recur
         # all over the engine and are not them
         lib_range={"WONAPI": (0x004aab40, 0x004ba2a0),
                    "RezMgr": (0x004ba2a0, 0x004bf4c0),
                    "lith": (0x004ba2a0, 0x004bf4c0),
                    "StdLith": (0x004ba2a0, 0x004bf4c0)},
         built="src/engine/lithtech/Release/lithtech.exe", optional=True,
         cites=True, cited_src=["src/engine"],
         # provisional: libcmt.lib (msvc-6.00-8966's) from ftol.obj's
         # __ftol (0x4bf918), 353 members to initcon.obj (0x4ceaeb), as
         # NOLF's engine's CRT begins at _ftol; and the 1,112 bytes of packed
         # code below it from 0x4bf4c0 are the C++ library's: libcp.lib's
         # string.obj (std::_Xlen/_Xran and the logic_error family, 17 of its
         # functions matched by exematch) and xlock.cpp's std::_Lockit
         app=[(0x00401000, 0x004bf4c0)]),
    dict(label="server.dll", what="the server", kind="image",
         shipped=REFERENCE + "/server.dll", stem="server",
         # nolfsrc's engine compiled for Primal Hunt, and linked with /FORCE
         # for the scoreboard (build/build.mk, engine-link: `built`, its .rsp
         # and .map, through which the small functions are verified); its
         # objects alone (partial_objs) until that link has been made
         # and the WON client, the same VC6-STL WONAPI.lib lithtech.exe
         # links (server.dll 0x10052030 is lithtech.exe 0x004aad60, byte
         # for byte), 0x10051e10 up to the runtime library's helpers at
         # 0x10060eec
         # and the debug builds of RezMgr and StdLith, interleaved in one
         # block from 0x1004a6e0 up to the WON client (build/build.mk, "The
         # debug builds")
         partial_objs=["src/engine/server/Release/*.obj",
                       "src/engine/lithtech/Release/wonapi/WONAPI.lib",
                       "src/LT2/lithshared/libs/Debug/RezMgr.lib",
                       "src/LT2/lithshared/libs/Debug/StdLith.lib"],
         lib_range={"WONAPI": (0x10051e10, 0x10060eec),
                    "RezMgr": (0x1004a6e0, 0x10051e10),
                    "StdLith": (0x1004a6e0, 0x10051e10)},
         built="src/engine/server/Release/server.dll", optional=True,
         cites=True, cited_src=["src/engine"],
         # provisional: libcmt.lib from __ftol (0x10061048), 354 members to
         # initcon.obj (0x100706db)
         app=[(0x10001000, 0x10061048)]),
    dict(label="d3d.ren", what="the Direct3D renderer", kind="image",
         shipped=REFERENCE + "/d3d.ren", stem="d3d",
         # nolfsrc's renderer compiled for Primal Hunt, and linked with
         # /FORCE for the scoreboard (build/build.mk, d3d-link: `built`,
         # its .rsp and .map, through which the small functions are
         # verified); its objects alone (partial_objs) until that link has
         # been made
         # and the debug build of StdLith it links after the import
         # thunks, as server.dll does (build/build.mk, "The debug builds")
         partial_objs=["src/engine/d3d/Release/*.obj",
                       "src/LT2/lithshared/libs/Debug/StdLith.lib"],
         lib_range={"StdLith": (0x1003c150, 0x1003c58b)},
         built="src/engine/d3d/Release/d3d.ren", optional=True,
         cites=True, cited_src=["src/engine"],
         # 2002-06-03. Unlike NOLF's and 1.0.9.6's (Intel C/C++'s shape),
         # its Rich header is Microsoft's throughout: C++ 8966 x69, one C++
         # object of cl 13.00.9178 (dxguid.lib's: the DirectX 8.1 SDK's
         # carries that @comp.id) and three import descriptors of implib
         # 7.00.9210 (the same SDK's ddraw.lib).
         # The renderer's objects end at 0x1003c136, where the linker put
         # the three DDRAW import thunks (jmp [0x10048008], [0x10048000],
         # [0x10048004]), then int3 padding to 0x1003c150. From there to
         # 0x1003c58b is the DEBUG StdLith.lib, unoptimised and int3-padded:
         # struct_bank.obj (sb_Init 0x1003c150 .. sb_AllocateNewStructPage
         # 0x1003c300), l_allocator.obj (LAlloc's vtable 0x10048654:
         # 0x1003c4f0, Alloc 0x1003c3d0, Free 0x1003c400; g_DefAlloc's
         # initialisers from 0x1003c430) and dynarray.obj
         # (MoArray_FindElementMemcmp 0x1003c530) -- tools/libscan.py finds
         # the release's prebuilt debug StdLith.lib there, 18 functions at
         # one address each, and our link's build of it is identical at
         # all 15 rows. Then libcmt.lib (msvc-6.00-8966's): delete.obj's
         # operator delete at 0x1003c58b (`push [esp+4]; call _free
         # (0x1003d36e); pop ecx; ret`), fpinit.obj's _fpmath (0x1003c596)
         # and _cfltcvt_init (0x1003c5ad) as our link's LIBCMT:fpinit.obj
         # (less __fpclear, which the retail /OPT:REF dropped), then
         # onexit.obj (0x1003c5e5), libscan's first unique hit of 250 to
         # 0x10047783. The thunks are the linker's, not code of the image's
         # own: left out, as the runtime is
         app=[(0x10001000, 0x1003c136), (0x1003c150, 0x1003c58b)]),
    dict(label="ltmsg.dll", what="the engine's messages", kind="resources",
         shipped=REFERENCE + "/ltmsg.dll", stem="ltmsg",
         built="src/engine/ltmsg/Release/ltmsg.dll", optional=True),
    dict(label="cdaudio.dll", what="the engine's CD-audio music driver", kind="image",
         shipped=REFERENCE + "/cdaudio.dll", stem="cdaudio",
         # src/engine/cdaudio/ (build/build.mk, "cdaudio.dll"): linked with
         # a response file and a map, so the scoreboard reads our image
         built="src/engine/cdaudio/Release/cdaudio.dll", optional=True,
         cites=True, cited_src=["src/engine"],
         # our link lays the image out as the retail one (every function of
         # cdaudio.obj and Redmgr.lib at its retail address; of StdLith's,
         # DefStdlithFree and LAlloc::Alloc trade places at
         # 0x100029c0/0x100029d0; .data and .bss with them). So its
         # relocation-free short functions are compared byte for byte with
         # our link, each only at its own address: the $E initializer thunks,
         # whose `call _atexit` names the runtime (no proven code of the
         # image's own names it), the empty ~CObject the unwind funclets
         # jump to, and OutMCIErr's `ret 8`.
         linked_oracle="same-address",
         # the libraries' blocks: Redmgr.lib (ours: src/lithshared/redmgr/)
         # from its OutMCIErr, then StdLith's l_allocator.obj (g_DefAlloc's
         # initializers) and dynarray.obj
         lib_range={"Redmgr": (0x10002390, 0x10002960),
                    "StdLith": (0x10002960, 0x10002a50)},
         # 2002-05-07; MusicDLLSetup and CRedMgr (lithshared's redmgr.h),
         # over MCI. MFC linked statically (nafxcw.lib, C++ 8569 x38). The
         # image's own code is cdaudio.obj, redmgr.obj and StdLith's two
         # members; it ends where nafxcw.lib's except.obj begins
         # (0x10002a50, CMemoryException's constructor: our .map), whose
         # packed inline members run to fixalloc.obj (0x10002aeb);
         # libcmt.lib from onexit.obj (0x10003123) to 0x10008b45, nafxcw.lib
         # again from 0x10008b5e
         app=[(0x10001000, 0x10002a50)]),
    dict(label="Launch.dll", what="the engine's command-line helper", kind="image",
         shipped=REFERENCE + "/Launch.dll", stem="Launch",
         built="src/launch/Debug/Launch.dll", optional=True,
         # NOLF's launcher DLL (C:\Proj\Nolf\LauncherDLL\Debug\Launch.pdb,
         # linked 1999-08-16), shipped again, by AvP2 and Primal Hunt alike
         # (the same file): a Debug build, linked with the debug CRT. The
         # engine loads it for GetLithTechCommandLine, its one export
         # (nolfsrc's src/engine/lithtech/clientwin.cpp). Ours links it
         # (build/build.mk, "Launch.dll") as Developer Studio linked a
         # Debug DLL: incrementally, so it has no map. The image's own
         # code is the three objects of src/launch/ (0x100010a0, after the
         # incremental link's jump table of 14 `jmp rel32` at
         # 0x10001005..0x1000104a, which the linker writes and orders by
         # its own symbol table, not by anything in the source), and
         # lithshared's CRegMgr (0x100026f0..0x10002bc7, a library's
         # member, compiled by VC5: no @comp.id), with the six import
         # thunks the objects' calls pulled in between (0x100026c0,
         # `jmp [IAT]`, kernel32.lib and user32.lib's). The runtime starts
         # at 0x10002bd0: strcpy, the first entry of libcmtd.lib's
         # strcat.obj (strcat at 0x10002be0); its members run to
         # 0x1001809b, so nearly all of the image is the runtime
         cites=True, cited_src=["src/launch"],
         app=[(0x100010a0, 0x100026c0), (0x100026f0, 0x10002bd0)]),
    # ---- Primal Hunt: the launcher and the dedicated server ------------
    dict(label="PrimalHunt.exe", what="Primal Hunt's launcher", kind="image",
         shipped=REFERENCE + "/PrimalHunt.exe", stem="PrimalHunt",
         built="src/launcher/PrimalHunt/PrimalHunt.exe", optional=True,
         # src/launcher/ compiled for Primal Hunt and linked (build/build.mk,
         # "The launchers' links"); RegMgr32.lib, the registry class, lies
         # outside the image's own code (as in AVP2.exe, below)
         third_party=["RegMgr32.lib"],
         # 2002-06-06, the shape of AVP2.exe: MFC linked statically
         # (nafxcw.lib, C++ 8569 x55) and the same wrapper's .cms_t and
         # .cms_d sections. The wrapper keeps the same five pages of .text
         # encrypted as in AVP2.exe, 0x408000..0x40d000 (they disassemble
         # to noise: (bad), in/out, iret by the dozen a page, where every
         # other page of .text decodes cleanly), so their rows are listed
         # and not counted: RezFind.cpp's (FindRezFiles at 0x4080a0, its
         # seven getters 0x408910..0x408970), StaticEx.cpp's and Utils.cpp's
         # are all in them, and the last of OptionsDlg.cpp (OnCtlColor,
         # 0x407fb0, breaks off at 0x408000; OnMouseMove follows it). The
         # plain code resuming at 0x40d000 is the C
         # runtime's: the tail of an SEH function whose filter and handler
         # funclets are the rows 0x40d038 (xor eax,eax; cmp [ebp+0xc],al;
         # setne al; ret) and 0x40d041 (mov esp,[ebp-0x18]; jmp), just below
         # libcmt.lib's first hit, frame.obj (0x40d049); 151 members to
         # 0x414a41, and nafxcw.lib from 0x418202 to 0x4211a6
         encrypted=[(0x00408000, 0x0040d000)],
         unreadable=r"(?i)^((RezFind|StaticEx|Utils)\.obj:|OptionsDlg\.obj:\?OnMouseMove@)",
         app=[(0x00401000, 0x0040d000)]),
    dict(label="AVP2XUp.dll", what="Primal Hunt's Sierra updater resources",
         kind="resources", shipped=REFERENCE + "/AVP2XUp.dll", stem="AVP2XUp",
         # src/AVP2/AVP2Up/ (build/build.mk, "AVP2Up.dll and AVP2XUp.dll"):
         # AVP2XUp.rc and an empty DllMain, linked with a response file and
         # a map; the whole image is the retail one but for its time stamp
         built="src/AVP2/AVP2Up/PrimalHunt/AVP2XUp.dll", optional=True),
    dict(label="AVP2XServ.exe", what="Primal Hunt's dedicated server", kind="image",
         shipped=REFERENCE + "/AVP2XServ.exe", stem="AVP2XServ",
         built="src/AVP2/AVP2Serv/PrimalHunt/AVP2XServ.exe", optional=True,
         # 2002-06-21: AVP2Serv.exe's shape, built as Primal Hunt's game
         # DLLs are (build/build.mk, "The dedicated servers"): its own
         # objects and the lithshared library members cl 8966 (x17; the
         # 8168 x25 are objects of SP5's own libraries, which our link
         # has too), MFC linked statically (C++ 8569 x86: SP5's
         # nafxcw.lib). The release's ButeMgr.lib is prebuilt, not ours.
         third_party=PREBUILT_LIBS,
         # From our link's map: the server's nine objects, then ButeMgr.lib
         # (from 0x4091c0) and RegMgr32.lib (0x410580..0x4108b0); MFC
         # starts at 0x4108b0 with dlgcore.obj (AfxGetMainWnd, then the two
         # 8-byte vcall thunks 0x4108c4 and 0x4108cc), wincore.obj at
         # 0x4108d4, to 0x418916; libcmt.lib from 0x418920 to 0x4245ff.
         # StdLith.lib (struct_bank.obj from 0x429bf0, l_allocator.obj) and
         # CryptMgr.lib (0x429dc0, 0x429dd0) are linked above the runtime,
         # just below nafxcw.lib's second run (from 0x429de0; libscan.py:
         # from timecore.obj, 0x429e4b): code of the image's own
         app=[(0x00401000, 0x004108b0), (0x00429bf0, 0x00429de0)]),
    # ---- Primal Hunt: the game -----------------------------------------
    # Third Law's, from Monolith's AvP2 1.0.9.6 code: no source was
    # released. Their own code is C++ 8966 (with 21 objects of 8168 in the
    # client, 24 in the server objects); the lithshared libraries are the
    # release's prebuilt ones, as for 1.0.9.6 (tools/libscan.py places
    # their functions in both).
    dict(label="PHCShell.dll", what="Primal Hunt's game: client", kind="image",
         shipped=REFERENCE + "/PHCShell.dll", stem="PHCShell",
         built="src/AVP2/ClientShellDLL/PrimalHunt/CShell.dll", optional=True,
         third_party=PREBUILT_LIBS,
         # provisional: libcmt.lib from typinfo.obj (0x101568f6), 342
         # members to mbschr.obj (0x10165774); then WONAPI.lib (Monolith's
         # WON client, lithshared) above the runtime to 0x1019bd50, and 25
         # more libcmt.lib members from strlwr.obj to initcon.obj
         # (0x1019cbab). 1.0.9.6's links WONAPI below its runtime
         # (0x1013d590..0x101709b0, 978 of the release's prebuilt
         # WONAPI.lib functions placed); here only about 280 are, so
         # Third Law's WONAPI may be another build of it. 9,974
         # bytes of packed code (138 functions) from 0x10154200 below the
         # first hit, where ButeMgr.lib's last run ends (0x101541ff)
         app=[(0x10001000, 0x101568f6)]),
    dict(label="PHObject.lto", what="Primal Hunt's game: server objects", kind="image",
         shipped=REFERENCE + "/PHObject.lto", stem="PHObject",
         built="src/AVP2/ObjectDLL/PrimalHunt/Object.lto", optional=True,
         third_party=PREBUILT_LIBS,
         # provisional: libcmt.lib from rand.obj (0x10255d1b), 292 members
         # to gmtime.obj (0x102635f9); 8,907 bytes of packed code (108
         # functions) from 0x10253a50 below it
         app=[(0x10001000, 0x10255d1b)]),
    dict(label="PHCRes.dll", what="Primal Hunt's client resources",
         kind="resources", shipped=REFERENCE + "/PHCRes.dll", stem="PHCRes",
         built="src/AVP2/ClientRes/PrimalHunt/CRes.dll", optional=True),
    dict(label="PHSRes.dll", what="Primal Hunt's server resources",
         kind="resources", shipped=REFERENCE + "/PHSRes.dll", stem="PHSRes",
         built="src/AVP2/ServerRes/PrimalHunt/SRes.dll", optional=True),
    # ---- AvP2 1.0.9.6: the launcher and the dedicated server ----------
    dict(label="AVP2.exe", what="AvP2's launcher", kind="image",
         shipped=REFERENCE + "/AVP2.exe", stem="AVP2",
         built="src/launcher/Release/AVP2.exe", optional=True,
         # src/launcher/ compiled for AvP2 and linked (build/build.mk, "The
         # launchers' links"). RegMgr32.lib, the registry class (the Rich
         # header's one object of 8966), is linked between the C runtime
         # and MFC, at 0x413d5c, outside the image's own code
         third_party=["RegMgr32.lib"],
         # the disc's (2001-10-15; the patch does not replace it), MFC
         # linked statically (nafxcw.lib, C++ 8569 x55). Its entry point
         # (0x43f8c7) is in .cms_t (0x433000..0x448000), a section a
         # protection wrapper added along with .cms_d. The wrapper keeps
         # five pages of .text encrypted, 0x408000..0x40d000: they
         # disassemble to noise ((bad), in/out, iret by the dozen a page,
         # where every other page decodes cleanly), the last of FindRezFiles
         # (0x407ca0) breaks off mid-instruction at 0x408000, and the
         # plain code resumes mid-function at 0x40d000. In them: the rest
         # of RezFind.cpp, StaticEx.cpp and Utils.cpp (their functions'
         # rows are known only from the calls into them: 0x408100..0x408150
         # the six rez getters, 0x408220 CStaticEx's constructor, 0x4086f0
         # ExistProcess) and the start of the C runtime. Listed, not
         # counted; our functions of those files are set aside (unreadable).
         # Provisional: libcmt.lib from output.obj (0x40d40b), 128 members to
         # 0x413d5c, and nafxcw.lib from 0x417942 to 0x420880; 955 bytes of
         # packed code from 0x40d050 below the first hit
         encrypted=[(0x00408000, 0x0040d000)],
         unreadable=r"(?i)^(RezFind\.obj:(?!\?FindRezFiles@)|StaticEx\.obj:|Utils\.obj:)",
         app=[(0x00401000, 0x0040d40b)]),
    dict(label="AVP2Up.dll", what="AvP2's Sierra updater resources",
         kind="resources", shipped=REFERENCE + "/AVP2Up.dll", stem="AVP2Up",
         # src/AVP2/AVP2Up/, AVP2Up.rc: as AVP2XUp.dll's, built into Release/
         built="src/AVP2/AVP2Up/Release/AVP2Up.dll", optional=True),
    dict(label="AVP2Serv.exe", what="AvP2's dedicated server", kind="image",
         shipped=REFERENCE + "/AVP2Serv.exe", stem="AVP2Serv",
         built="src/AVP2/AVP2Serv/Release/AVP2Serv.exe", optional=True,
         # Its butemgr.lib, cryptmgr.lib and StdLith.lib are ours, built
         # with its own cl 8168 (build/build.mk, "The dedicated servers"):
         # ButeMgr.lib is not the release's prebuilt one here
         third_party=[l for l in PREBUILT_LIBS if l.lower() != "butemgr.lib"],
         # MFC linked statically (msvc-6.00-8168's nafxcw.lib). From our
         # link's map: the server's nine objects, then ButeMgr (from
         # 0x409b40), CryptMgr (0x40fcf0) and StdLith (0x40fd10..0x40fedc);
         # MFC starts at 0x40fee0 with dlgcore.obj (AfxGetMainWnd, then the
         # two 8-byte vcall thunks 0x40fef4 and 0x40fefc), wincore.obj at
         # 0x40ff04, to 0x417f46; libcmt.lib from 0x417f50 to 0x422e3f,
         # nafxcw.lib again from 0x428492
         app=[(0x00401000, 0x0040fee0)]),
    # ---- AvP2 1.0.9.6: the game ----------------------------------------
    dict(label="CShell.dll", what="AvP2's game: client", kind="image",
         shipped=REFERENCE + "/CShell.dll", stem="CShell",
         built="src/AVP2/ClientShellDLL/final/CShell.dll", optional=True,
         third_party=PREBUILT_LIBS,
         # provisional: 14 import thunks from 0x10172ae6, then libcmt.lib
         # from typinfo.obj (0x10172b3a), 364 members to initcon.obj; 4.9
         # KB of packed code from 0x101717a0 below the thunks
         app=[(0x10001000, 0x10172ae6)]),
    dict(label="Object.lto", what="AvP2's game: server objects", kind="image",
         shipped=REFERENCE + "/Object.lto", stem="Object",
         built="src/AVP2/ObjectDLL/final/Object.lto", optional=True,
         third_party=PREBUILT_LIBS,
         # provisional: libcmt.lib from typinfo.obj (0x10235bb9), 285
         # members; 2.8 KB of packed code from 0x102350f0 below it
         app=[(0x10001000, 0x10235bb9)]),
    dict(label="CRes.dll", what="AvP2's client resources",
         kind="resources", shipped=REFERENCE + "/CRes.dll", stem="CRes",
         built="src/AVP2/ClientRes/Release/CRes.dll", optional=True),
    dict(label="SRes.dll", what="AvP2's server resources",
         kind="resources", shipped=REFERENCE + "/SRes.dll", stem="SRes",
         built="src/AVP2/ServerRes/Release/SRes.dll", optional=True),
    # ---- AvP2 1.0.9.6: the engine ------------------------------------
    # AvP2's game DLLs need it: Primal Hunt's engine has an ILTCommon slot
    # more (GetPolyTextureName), which moves GetPolyInfo and
    # GetPolySurfaceFlags (docs/PLAN.md, package 10). The patch's engine
    # (2002-04-17), unprotected, from out/reference/avp2/. The same source
    # as Primal Hunt's, without PRIMAL_HUNT (build/build.mk, "The engine").
    dict(label="AvP2 lithtech.exe", what="AvP2's engine", kind="image",
         shipped=REFERENCE + "/avp2/lithtech.exe", stem="AVP2lithtech",
         built="src/engine/lithtech/AvP2Release/lithtech.exe", optional=True,
         partial_objs=["src/engine/lithtech/AvP2Release/*.obj"],
         cites=True, cited_src=["src/engine"],
         # provisional: libcmt.lib from ftol.obj's __ftol (0x4b4330), 350
         # of its members placed in one run to initcon.obj (0x4c3675), as
         # NOLF's engine's CRT begins at _ftol; 80 bytes of packed code
         # from 0x4b42e0 sit just below it
         app=[(0x00401000, 0x004b4330)]),
    dict(label="AvP2 server.dll", what="AvP2's server", kind="image",
         shipped=REFERENCE + "/avp2/server.dll", stem="AVP2server",
         built="src/engine/server/AvP2Release/server.dll", optional=True,
         partial_objs=["src/engine/server/AvP2Release/*.obj"],
         cites=True, cited_src=["src/engine"],
         # provisional: libcmt.lib from __ftol (0x10057ba0), 346 members to
         # initcon.obj; 80 bytes of packed code from 0x10057b50 below it
         app=[(0x10001000, 0x10057ba0)]),
    dict(label="AvP2 d3d.ren", what="AvP2's Direct3D renderer", kind="image",
         shipped=REFERENCE + "/avp2/d3d.ren", stem="AVP2d3d",
         built="src/engine/d3d/AvP2Release/d3d.ren", optional=True,
         partial_objs=["src/engine/d3d/AvP2Release/*.obj"],
         cites=True, cited_src=["src/engine"],
         # built by Intel C/C++ 4.0, as nolfsrc's: its padding and /Qip names
         icl=True,
         # 2002-02-07, older than the patch's engine. Its Rich header is
         # NOLF's renderer's shape (C 8168 x120, C++ 8168 x73, and three
         # objects of an unknown tool 0x25/8755): NOLF's was Intel C/C++
         # 4.0 (nolfsrc), whose objects a Rich header does not record.
         # Provisional: a packed `push [esp+4]; call; pop; ret` thunk at
         # 0x1003b710 after padding, then msvc-6.00-8168's libcmt.lib from
         # onexit.obj (0x1003b76a), 237 members
         app=[(0x10001000, 0x1003b710)]),
    dict(label="AvP2 ltmsg.dll", what="AvP2's engine messages", kind="resources",
         shipped=REFERENCE + "/avp2/ltmsg.dll", stem="AVP2ltmsg",
         built="src/engine/ltmsg/AvP2Release/ltmsg.dll", optional=True),
]
