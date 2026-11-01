"""What the scoreboard scores: Aliens versus Predator 2's binaries, and
where each one's own code, and ours, is.

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
library's functions in the image. Every binary of AvP2's links the C
runtime statically, as NOLF's do: `libcmt.lib`, and MFC's `nafxcw.lib` in
the two executables. The Rich headers say which VC6 tree's: the C objects
of build 8047 (SP5's and later trees' CRT: msvc-6.00-8966's libraries were
searched) in the engine, the launcher, the game DLLs and the resource DLLs;
of 8168 (msvc-6.00-8168's) in d3d.ren, the dedicated server and
Launch.dll, whose debug build links libcmtd.lib. The image's own code ends
where the runtime's first member starts: as in NOLF's engine, `_ftol`
(ftol.obj), or an import-thunk run (`jmp [IAT]`, 6 bytes each) just before
it. Library members are packed; code compiled /O2 ends padded to 16 bytes,
so a packed stretch just below the first hit (noted per target) may be
runtime too, or a library of Monolith's built /O1. Until a binary has a link
of ours, its boundary is the first hit: marked "provisional" below, to be
pinned from our .map (which names the library of every function) once there
is one, as nolfsrc did.
"""

REFERENCE = "out/reference"     # build/reference.sh writes it
OUT = "out/progress"            # each run's functions.csv and summary.json
DATA = "progress"               # the recorded baseline and the evidence

# Every hex address cited in these source trees marks the address
# `attempted` in a target with `cites` (an address a reconstruction has
# been written for), unless the target names its own `cited_src`.
CITED_SRC = []

# No prebuilt static library ships with AvP2: every library is built
# from source and scored inside the binaries that link it.
LIBS = []
SHIPPED_LIB = None
SHIPPED_AS = {}
SHIPPED_ENV = None

DRIVER_OBJS = set()

TARGETS = [
    # ---- the engine ------------------------------------------------------
    # The 1.0.9.6 patch's engine (2002-04-17). Unlike the disc's, it carries
    # no protection: its sections are the linker's own (.text, .rdata,
    # .data, .rsrc), with every import called through the IAT.
    dict(label="lithtech.exe", what="the engine", kind="image",
         shipped=REFERENCE + "/lithtech.exe", stem="lithtech",
         built="src/engine/lithtech/Release/lithtech.exe", optional=True,
         cites=True, cited_src=["src/engine"],
         # provisional: libcmt.lib from ftol.obj's __ftol (0x4b4330), 350
         # of its members placed in one run to initcon.obj (0x4c3675), as
         # NOLF's engine's CRT begins at _ftol; 80 bytes of packed code
         # from 0x4b42e0 sit just below it
         app=[(0x00401000, 0x004b4330)]),
    dict(label="server.dll", what="the server", kind="image",
         shipped=REFERENCE + "/server.dll", stem="server",
         built="src/engine/server/Release/server.dll", optional=True,
         cites=True, cited_src=["src/engine"],
         # provisional: libcmt.lib from __ftol (0x10057ba0), 346 members to
         # initcon.obj; 80 bytes of packed code from 0x10057b50 below it
         app=[(0x10001000, 0x10057ba0)]),
    dict(label="d3d.ren", what="the Direct3D renderer", kind="image",
         shipped=REFERENCE + "/d3d.ren", stem="d3d",
         built="src/engine/d3d/Release/d3d.ren", optional=True,
         cites=True, cited_src=["src/engine"],
         # 2002-02-07, older than the patch's engine. Its Rich header is
         # NOLF's renderer's shape (C 8168 x120, C++ 8168 x73, and three
         # objects of an unknown tool 0x25/8755): NOLF's was Intel C/C++
         # 4.0 (nolfsrc), whose objects a Rich header does not record; to
         # be shown here. Provisional: a packed `push [esp+4]; call; pop;
         # ret` thunk at 0x1003b710 after padding, then msvc-6.00-8168's
         # libcmt.lib from onexit.obj (0x1003b76a), 237 members
         app=[(0x10001000, 0x1003b710)]),
    dict(label="ltmsg.dll", what="the engine's messages", kind="resources",
         shipped=REFERENCE + "/ltmsg.dll", stem="ltmsg",
         built="src/engine/ltmsg/Release/ltmsg.dll", optional=True),
    dict(label="Launch.dll", what="the engine's command-line helper", kind="image",
         shipped=REFERENCE + "/Launch.dll", stem="Launch",
         built="src/launch/Debug/Launch.dll", optional=True,
         # NOLF's launcher DLL (C:\Proj\Nolf\LauncherDLL\Debug\Launch.pdb,
         # linked 1999-08-16), shipped again: a Debug build, linked with
         # the debug CRT. The engine loads it for GetLithTechCommandLine,
         # its one export (nolfsrc's src/engine/lithtech/clientwin.cpp).
         # Provisional: libcmtd.lib from mbscat.obj (0x10002bfc); its 242
         # members run to 0x1001809b, so nearly all of the image is the
         # runtime
         app=[(0x10001000, 0x10002bfc)]),
    # ---- the launcher and the dedicated server ------------------------
    dict(label="AVP2.exe", what="the launcher", kind="image",
         shipped=REFERENCE + "/AVP2.exe", stem="AVP2",
         built="src/AVP2/Launcher/Release/AVP2.exe", optional=True,
         # the disc's (2001-10-15; the patch does not replace it), MFC
         # linked statically (nafxcw.lib, C++ 8569 x55). Its entry point
         # (0x43f8c7) is in .cms_t (0x433000..0x448000), a section a
         # protection wrapper added along with .cms_d; it is not encrypted
         # (6.8 bits of entropy a byte, as .text). Provisional: libcmt.lib
         # from output.obj (0x40d40b), 128 members to 0x413d5c, and
         # nafxcw.lib from 0x417942 to 0x420880; 955 bytes of packed code
         # from 0x40d050 below the first hit
         app=[(0x00401000, 0x0040d40b)]),
    dict(label="AVP2Up.dll", what="the Sierra updater's resources",
         kind="resources", shipped=REFERENCE + "/AVP2Up.dll", stem="AVP2Up",
         built="src/AVP2/AVP2Up/Release/AVP2Up.dll", optional=True),
    dict(label="AVP2Serv.exe", what="the dedicated server", kind="image",
         shipped=REFERENCE + "/AVP2Serv.exe", stem="AVP2Serv",
         built="src/AVP2/AVP2Serv/Release/AVP2Serv.exe", optional=True,
         # MFC linked statically (msvc-6.00-8168's nafxcw.lib). Provisional:
         # nafxcw.lib from wincore.obj (0x40ff04) to 0x417f46, libcmt.lib
         # from 0x417f50 to 0x422e3f, nafxcw.lib again from 0x428492
         app=[(0x00401000, 0x0040ff04)]),
    # ---- the game ------------------------------------------------------
    dict(label="CShell.dll", what="the game: client", kind="image",
         shipped=REFERENCE + "/CShell.dll", stem="CShell",
         built="src/AVP2/ClientShellDLL/Release/CShell.dll", optional=True,
         # provisional: 14 import thunks from 0x10172ae6, then libcmt.lib
         # from typinfo.obj (0x10172b3a), 364 members to initcon.obj; 4.9
         # KB of packed code from 0x101717a0 below the thunks
         app=[(0x10001000, 0x10172ae6)]),
    dict(label="Object.lto", what="the game: server objects", kind="image",
         shipped=REFERENCE + "/Object.lto", stem="Object",
         built="src/AVP2/ObjectDLL/Release/Object.lto", optional=True,
         # provisional: libcmt.lib from typinfo.obj (0x10235bb9), 285
         # members; 2.8 KB of packed code from 0x102350f0 below it
         app=[(0x10001000, 0x10235bb9)]),
    dict(label="CRes.dll", what="the game's client resources",
         kind="resources", shipped=REFERENCE + "/CRes.dll", stem="CRes",
         built="src/AVP2/ClientRes/Release/CRes.dll", optional=True),
    dict(label="SRes.dll", what="the game's server resources",
         kind="resources", shipped=REFERENCE + "/SRes.dll", stem="SRes",
         built="src/AVP2/ServerRes/Release/SRes.dll", optional=True),
]
