# avp2src

*Aliens versus Predator 2* (Monolith, 2001) and its expansion *Primal
Hunt* (Third Law, 2002), built from source.

> [!WARNING]
> **Work in progress.** The reconstruction is not complete: not every
> function compiles to the retail bytes yet (see
> [`progress/README.md`](progress/README.md)), and the modern build's
> features are still being added and tested. It is playable, though: both
> games, AvP2 and Primal Hunt, start and play on these builds. Expect bugs.

The game's last release is Primal Hunt 1.0.2.2: its engine, renderer and
server are the newest, and its game code is AvP2 1.0.9.6's (the last AvP2
patch) carried on. Primal Hunt plays only its own campaigns, so the game is
both: Primal Hunt's engine, launcher, dedicated server and game DLLs, and
AvP2 1.0.9.6's game DLLs, launcher and dedicated server, which play AvP2's.

Monolith released AvP2's game code in 2002 -- the client and server game
DLLs, their resource DLLs, the dedicated server, the engine SDK headers and
the `lithshared` libraries, at 1.0.9.6 -- but never the engine, and Primal
Hunt's code was never released at all. AvP2 runs on LithTech "Talon", the
engine generation of *No One Lives Forever* (2000), so the engine is
reconstructed from [nolfsrc](https://github.com/nolffan/nolfsrc)'s
reconstruction of NOLF's, brought to what Primal Hunt's binaries are,
function by function; Primal Hunt's game DLLs from Monolith's 1.0.9.6
source, brought to Third Law's. As for my other recomp. projects:

- **legacy**: Primal Hunt (Europe) 1.0.2.2 and Aliens versus Predator 2
  (Europe) 1.0.9.6, rebuilt so that they compile back to the retail
  binaries byte for byte, as far as that can be done -- with the compilers
  Monolith and Third Law used, Visual C++ 6.0 in the revisions each
  binary's Rich header names, under Wine;
- **modern**: the same source with fixes and new features (`AVP2SRC_MODERN`).

Compiling back to the retail bytes means compiling the retail game's bugs as
well; their fixes go behind `AVP2SRC_MODERN`, into the modern build only, so
the legacy build keeps matching the retail one.

## Progress

How much of each binary compiles to exactly the bytes of the retail game:
the table, with how it is scored, is in [`progress/README.md`](progress/README.md).

## Building

### Requirements

You must have Docker.

You must also have the submodules. To get them, do this command:

```sh
git submodule update --init
```

The submodules are:

- `ext/toolchains/`: the compilers.
- `ext/rez/`: the installer. It gets the disc images from archive.org. It
  installs AvP2, the AvP2 1.0.9.6 patch and Primal Hunt.
- `ext/tools/`: the reconstruction tools.
- `ext/openmiles/` and `ext/ffmpeg/`: the open-source audio and video
  libraries. Only the modern build uses them.

### Commands

```sh
make            # build both games into out/avp2-legacy-release/
make test       # start each game under Xvfb and Wine (results in out/play/test/)
make progress   # compare the build with the retail binaries
make help       # show all the commands
```

There are three more builds:

- `make debug`: the legacy build with symbols and `AVP2SRC_DEBUG` logs.
  The output is in `out/avp2-legacy-debug/`.
- `make modern`: the modern build, with `AVP2SRC_MODERN`. The output is in
  `out/avp2-modern-release/`. [`docs/MODERN.md`](docs/MODERN.md) gives
  the changes.
- `make modern-debug`: the modern build with symbols and logs. The output
  is in `out/avp2-modern-debug/`.

These three builds use a copy of the source tree in `out/build-<variant>/`
(`build/variant.sh`). `make progress` does not compare them with the
retail binaries.

### The game directory

Each build makes one game directory. The game directory has two
directories, the same as the retail setups make:

- `AVP2/`: Aliens versus Predator 2 1.0.9.6. It has its own engine,
  because the Primal Hunt engine cannot run the AvP2 game DLLs. It also
  has its launcher, the dedicated server `AVP2Serv.exe`, and the game DLLs
  in `custom/`.
- `PrimalHunt/`: Primal Hunt 1.0.2.2. It has its engine, its launcher,
  the dedicated server `AVP2XServ.exe`, and the game DLLs in `custom/`.
  It uses the AvP2 archives in `../AVP2/`.

This project builds all the binaries. The file `MANIFEST.txt` in each
directory gives the source of each file. Only these files come from the
retail games:

- The `.rez` archives. The build sets the retail game DLLs in them to
  zero.
- The movies.
- The other data files: `console.PCX`, `childmodel.map` and, for Primal
  Hunt, `FontData.fnt`.
- The readme files, the default `autoexec.cfg` and the default profile.
- Bink, Smacker and Miles, in the legacy builds only. There is no source
  for them. The modern builds use open-source replacements. `build/ext.sh`
  builds the replacements from `ext/`.

The launcher starts each game with this command:
`lithtech.exe -cmdfile avp2cmds.txt`. The file `avp2cmds.txt` gives the
archives and the game DLLs (`build/avp2cmds.sh`).

### The archives

Each build also makes two archives of its game directory
(`build/package.sh`):

- `out/<name>-private.tar.gz`: with the retail data. Use it on your own
  computers only.
- `out/<name>-public.tar.gz`: without the retail data. You can give it to
  other persons.

The public archive has install scripts (`build/mkinstall.py`). The scripts
copy the data from the player's own disc images: the two AvP2 discs and
the Primal Hunt disc. The European discs are the default. The scripts also
accept some USA discs. They accept a `.iso` file, or a redump `.bin`/`.cue`
or `.zip` file. They copy the AvP2 1.0.9.6 patch from the Primal Hunt disc.
They make sure that each file they write is correct.

To install on Linux, do this command:

```sh
avp2-modern-release/linux-install.sh avp2-1.zip avp2-2.zip ph.zip
```

To install on a Mac (for CrossOver), do this command:

```sh
avp2-modern-release/mac-install.command avp2-1.zip avp2-2.zip ph.zip
```

To install on Windows, drop the three images onto `windows-install.bat`.
This script also writes the registry values that the retail setups wrote.

If you do not give the images, the scripts look for them in the game
directory and in the directory above it. You can do the scripts again.

### Starting the games

Both archives have scripts that start the games (`build/game-scripts/`).

On Linux, use `linux-run-avp2.sh` or `linux-run-ph.sh`. Each script starts
the game under Wine, in its own Wine prefix (`~/.wine-avp2`). The prefix
has the registry values that the retail setups wrote. These are the
options:

- `--launcher`: start the launcher, not the game.
- `--fullscreen`: use full screen.
- `--windowed`: use a window.
- `--borderless`: use a window with no border, at the desktop resolution.
  Only the modern build has this mode.
- `--res WxH`: use this resolution, for example `--res 1280x720`.
- `--help`: show the options.

If you do not give a window option, the game uses the settings from the
last time.

On Windows, use `windows-run.bat avp2` or `windows-run.bat ph`. You can
also use the launchers:

- In the legacy build, the launchers are `AVP2\AVP2.exe` and
  `PrimalHunt\PrimalHunt.exe`.
- In the modern build, the launchers are `AVP2\AVP2Launcher.exe` and
  `PrimalHunt\PrimalHuntLauncher.exe`. `AVP2.exe` and `PrimalHunt.exe`
  start the game directly.

## Credits

This project builds on the work of others:

- **Monolith Productions**, for *Aliens versus Predator 2* and the LithTech
  engine, and, with **Fox Interactive**, for releasing AvP2's game source
  code (v1.0.9.6, 2002), which `src/AVP2/` and `src/LT2/` are; and for the
  *No One Lives Forever 2* source release, from which the launchers and much
  of `src/lithshared/` come
  ([`docs/LICENSE_NOTES.md`](docs/LICENSE_NOTES.md)).
- **Third Law Interactive**, for *Primal Hunt*, and **Sierra**, its
  publisher.
- **OpenMiles** (<https://github.com/maci0/openmiles>), the open
  reimplementation of the Miles Sound System that the modern build uses in
  place of Miles, with the miniaudio and TinySoundFont libraries it vendors.
- **FFmpeg** (<https://ffmpeg.org>), on which the modern build's Bink player
  is built; **zlib**; and **Capstone**, which the tools use to read the
  retail machine code.
- **Wine** and **Proton**, which run the game on Linux and the Steam Deck,
  and **OpenSpy** (<https://openspy.net>), whose master server replaces
  GameSpy's for the modern build's Internet games.
- **redump.org**, for the verified disc images the installers check
  against, and **archive.org**, which keeps them.
