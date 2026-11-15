# Monolith's prebuilt libraries

`LT2/lithshared/libs/` here is the `libs/` directory of Monolith's *Aliens
Versus Predator 2 Source Code v1.0.9.6* release (July 2002), without
changes. It holds the shared "lithshared" libraries that the game links:

- StdLith, CryptMgr, RegMgr, RezMgr, RedMgr, MFCStub, ButeMgr,
- the GUI and font managers (`ltguimgr_LT2`, `LithFontMgr_LT2`),
- the GameSpy managers, WONAPI, zlib, ...

Each library is in its Release build and in its Debug build (`cl` 8966,
`progress/COMPILERS.md`). Like the NOLF release, the AvP2 release has most
of them as binaries and headers only. Only WONAPI and STLport come with
source (`src/LT2/lithshared/`).

`AVP2/AVP2Serv/server.lib` is the import library of the dedicated server
for the `server.dll` of the engine. The release kept it beside the project
of the server. `SHA256SUMS` lists all the files. The files are under the
licences of the release, as `src/AVP2/` is (`docs/LICENSE_NOTES.md`).

The build does not use these files. Our libraries are rebuilt from the
release source, and these files are the reference to check them against
(`docs/PLAN.md`, package 4). The game DLLs of AvP2 1.0.9.6 and of Primal
Hunt both have functions of these same libraries (`ext/tools/libscan.py`
places them). But a static library holds more than one program takes from
it. Thus the library itself is the only evidence for the rest.
