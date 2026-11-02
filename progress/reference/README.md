# Monolith's prebuilt libraries

`LT2/lithshared/libs/` here is the `libs/` directory of Monolith's *Aliens
Versus Predator 2 Source Code v1.0.9.6* release (July 2002), as it came:
the shared "lithshared" libraries the game links -- StdLith, CryptMgr,
RegMgr, RezMgr, RedMgr, MFCStub, ButeMgr, the GUI and font managers
(`ltguimgr_LT2`, `LithFontMgr_LT2`), the GameSpy managers, WONAPI, zlib,
... -- each in its Release and its Debug build (`cl` 8966,
`progress/COMPILERS.md`). As NOLF's release, AvP2's ships most of them as
binaries and headers only; WONAPI and STLport alone come with source
(`src/LT2/lithshared/`). `AVP2/AVP2Serv/server.lib` is the dedicated
server's import library for the engine's `server.dll`, which the release
kept beside the server's project. `SHA256SUMS` lists every file. They are
under the release's licences, as `src/AVP2/` is (`docs/LICENSE_NOTES.md`).

Nothing is built from them. They are what our libraries, rebuilt from the
release's source, are to be checked against (`docs/PLAN.md`, package 4):
the game DLLs of both AvP2 1.0.9.6 and Primal Hunt place functions of
these very libraries (`tools/libscan.py`), and a static library holds more
than any one program takes from it, so the library itself is the only
witness to the rest.
