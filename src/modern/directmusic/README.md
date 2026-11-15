# src/modern/directmusic/ -- the music, without installing DirectMusic

The music of the two games uses DirectMusic, not Miles (the engine's
`clientmusic.cpp`, `dmloader.cpp`, `dmextra.cpp`; the console's
`musictype`). The DirectMusic of Wine cannot play it, and Proton also uses
the DirectMusic of Wine. Thus the games run with their sound effects and
dialogue, but with no music. The usual fix is to install Microsoft's
DirectMusic into the Wine prefix with winetricks. Instead, the modern build
puts Microsoft's DirectMusic in each game directory. Thus the music plays
with any prefix, without changes to it (docs/MODERN.md, M-X4). nolfsrc did
the same for NOLF (its M-P4-12). This is the same approach, checked against
the AvP2 and Primal Hunt engines.

## How

* **The DLLs.** Step 2 of `build/all.sh` copies the nine Microsoft
  DirectMusic DLLs of DirectX 9.0c. The step uses
  `ext/tools/retailextract.py openruntime`. `DIRECTMUSIC_MANIFESTS` in
  `build/retail.py` and `build/retail_avp2.py` tells it to do this. It
  copies the DLLs from `ext/toolchains/directmusic-5.3.1.904/` into
  `AVP2/` and `PrimalHunt/`, and checks them against the `SHA256SUMS` of
  that directory. The names of the copies are `ms_dmband.dll`,
  `ms_dmcompos.dll`, `ms_dmime.dll`, `ms_dmloader.dll`, `ms_dmscript.dll`,
  `ms_dmstyle.dll`, `ms_dmsynth.dll`, `ms_dmusic.dll` and `ms_dswave.dll`
  (`MANIFEST.txt`: `THIRD-PARTY Microsoft DirectMusic`). It does not copy
  Microsoft's `dsound.dll`, because the DirectMusic synthesizer plays
  through the DirectSound of the system (the DirectSound of Wine).
* **The manifest.** `lithtech.exe.manifest` is installed beside the
  `lithtech.exe` of each game. It is registration-free COM: it maps the 46
  classes that those DLLs serve to the files in the game directory. Thus
  each `CoCreateInstance` of one of these classes gets to Microsoft's code
  without the registry. These calls are:
  * the engine's `CLSID_DirectMusicPerformance` (`CLTDirectMusicMgr::Init`),
  * the classes that our loader creates for the objects it loads
    (`CDMLoader::GetObject`, `dmextra.cpp`: segments, styles, DLS
    collections),
  * the objects that the DirectMusic DLLs create of each other (tracks,
    the synthesizer port).

  The `lithtech.exe` of the two games has no manifest of its own (no
  `RT_MANIFEST`). Thus Windows and Wine both read this manifest. It is the
  nolfsrc manifest, but without two classes that no DLL here implements:
  `DirectMusicSong` and `DirectMusicMelodyFormulationTrack`. Their CLSIDs
  are in none of the nine files, so no file can serve them. (nolfsrc read
  the list from a scratch prefix that also had the registrations of Wine's
  own DLLs.)
* **The names.** Wine loads a DLL with the name `dmime.dll` as its builtin
  DLL, also from the game directory, unless the prefix overrides it. Wine
  has no builtin `ms_dmime.dll`, so it loads the file. None of the nine
  DLLs imports another by name.

Nothing in the engine changes. Thus the legacy build does not change: it
asks the system for DirectMusic, as retail does. On Windows, the game uses
the copies in the game directory in place of the copies of the system.
(These copies are the same DirectX 9.0c DirectMusic that Windows ships.)
The build registers nothing, so the DirectMusic of the system does not
change.

There is no `gm.dls`. The loader of the engine (`dmloader.cpp`) cannot give
DirectMusic the default GM collection: there is no
`GUID_DefaultGMCollection` in the engine. The instruments of the music are
its own DLS banks. The engine loads them from the `.rez` archives through
the streams of the loader (`DLSBANKDIRECTORY`, and the banks that the music
control file of a level names).

If `ext/toolchains/directmusic-5.3.1.904/` is not there, the build tells
you. Then it installs neither the DLLs nor the manifest, because a manifest
that names missing files would break DirectMusic on Windows. The music then
needs Microsoft's DirectMusic in the prefix, as with the legacy build.

## Checking the manifest

Each `<comClass>` CLSID must be in the DLL that its `<file>` names, as a
GUID in memory order. All 46 are. The nolfsrc file
`src/modern/directmusic/README.md` tells how the class list was read: from
the registry of a scratch prefix, after `regsvr32` of the nine DLLs.
