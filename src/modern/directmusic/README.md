# src/modern/directmusic/ -- the music, without installing DirectMusic

Both games' music is DirectMusic (the engine's `clientmusic.cpp`,
`dmloader.cpp`, `dmextra.cpp`; the console's `musictype`), not Miles. Wine's
own DirectMusic, which Proton uses too, cannot play it: the games run with
their sound effects and dialogue and no music. The usual fix is to install
Microsoft's DirectMusic into the Wine prefix with winetricks. The modern
build carries Microsoft's DirectMusic in each game directory instead, so the
music plays with any prefix as it comes (docs/MODERN.md, M-X4). nolfsrc did
the same for NOLF (its M-P4-12); this is that approach, checked against
AvP2's and Primal Hunt's engines.

## How

* **The DLLs.** `build/all.sh` step 2 (`ext/tools/retailextract.py
  openruntime`, asked to by `DIRECTMUSIC_MANIFESTS` in `build/retail.py` and
  `build/retail_avp2.py`) copies Microsoft's nine DirectMusic DLLs, DirectX
  9.0c's, from `ext/toolchains/directmusic-5.3.1.904/` (checked against its
  `SHA256SUMS`) into `AVP2/` and `PrimalHunt/` as `ms_dmband.dll`,
  `ms_dmcompos.dll`, `ms_dmime.dll`, `ms_dmloader.dll`, `ms_dmscript.dll`,
  `ms_dmstyle.dll`, `ms_dmsynth.dll`, `ms_dmusic.dll` and `ms_dswave.dll`
  (`MANIFEST.txt`: `THIRD-PARTY Microsoft DirectMusic`). Not Microsoft's
  `dsound.dll`: DirectMusic's synthesizer plays through the system's
  DirectSound (Wine's).
* **The manifest.** `lithtech.exe.manifest`, installed beside each game's
  `lithtech.exe`, is registration-free COM: it maps the 46 classes those
  DLLs serve to the files in the game directory, so every
  `CoCreateInstance` of one -- the engine's `CLSID_DirectMusicPerformance`
  (`CLTDirectMusicMgr::Init`), the classes our loader creates for the
  objects it loads (`CDMLoader::GetObject`, `dmextra.cpp`: segments,
  styles, DLS collections), and what the DirectMusic DLLs create of each
  other (tracks, the synthesizer port) -- reaches Microsoft's code without
  the registry. Neither game's `lithtech.exe` carries a manifest of its own
  (no `RT_MANIFEST`), so Windows and Wine both read this one. It is
  nolfsrc's, with the two classes left out that no DLL here implements
  (`DirectMusicSong` and `DirectMusicMelodyFormulationTrack`: their CLSIDs
  are in none of the nine files, so none can serve them; the scratch
  prefix nolfsrc read the list from had Wine's own registrations too).
* **The names.** A DLL called `dmime.dll` is loaded as Wine's builtin even
  from the game directory, unless the prefix overrides it; Wine has no
  builtin `ms_dmime.dll`, so it loads the file. None of the nine imports
  another by name.

Nothing in the engine changes, so the legacy build is untouched: it asks
the system for DirectMusic as retail does. On Windows the game directory's
copies are used in place of the system's (the same DirectX 9.0c
DirectMusic that Windows ships), and nothing is registered, so the
system's DirectMusic is not touched.

No `gm.dls`: the engine's own loader (`dmloader.cpp`) cannot give
DirectMusic the default GM collection (no `GUID_DefaultGMCollection`
anywhere in the engine), and the music's instruments are its own DLS
banks, loaded from the `.rez` archives through the loader's streams
(`DLSBANKDIRECTORY` and the banks a level's music control file names).

Without `ext/toolchains/directmusic-5.3.1.904/` the build says so and
installs neither the DLLs nor the manifest (a manifest naming missing
files would break DirectMusic on Windows); the music then needs
Microsoft's DirectMusic in the prefix, as with the legacy build.

## Checking the manifest

Each `<comClass>` CLSID must be in the DLL its `<file>` names (as a GUID,
in memory order): all 46 are. How the class list was read, from a scratch
prefix's registry after `regsvr32` of the nine, is in nolfsrc's
`src/modern/directmusic/README.md`.
