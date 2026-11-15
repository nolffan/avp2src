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

Needs docker and the submodules (`git submodule update --init`):
`ext/toolchains/` (the compilers), `ext/rez/` (which fetches the discs from
archive.org and installs AvP2 with its 1.0.9.6 patch, and Primal Hunt
beside it) and `ext/tools/` (the reconstruction tools); and for the modern
build, `ext/openmiles/` (OpenMiles) and `ext/ffmpeg/` (FFmpeg), its open
middleware.

```sh
make            # out/avp2-legacy-release/: both games, built here, with the retail data
make test       # boot each under Xvfb and Wine (build/playtest.sh; out/play/test/)
make progress   # score the build against the retail binaries
make help       # everything else
```

`make debug`, `make modern` and `make modern-debug` build the same games
with symbols and `AVP2SRC_DEBUG` logging, with `AVP2SRC_MODERN` (the modern
build: [`docs/MODERN.md`](docs/MODERN.md); so far its open middleware,
OpenMiles and a Bink player on FFmpeg in place of RAD's), or
both, into `out/avp2-legacy-debug/`, `out/avp2-modern-release/` and
`out/avp2-modern-debug/` (`build/variant.sh`, in a copy of the tree under
`out/build-<variant>/`); none of them is ever scored.

A game directory holds both games, each in a directory of its own, as
their setups install them:

- `AVP2/`: Aliens versus Predator 2 1.0.9.6 -- its engine (AvP2's own,
  1.0.9.6's: Primal Hunt's cannot run AvP2's game DLLs), launcher
  `AVP2.exe`, dedicated server `AVP2Serv.exe`, the game DLLs in `custom/`;
- `PrimalHunt/`: Primal Hunt 1.0.2.2 -- its engine, launcher
  `PrimalHunt.exe`, dedicated server `AVP2XServ.exe`, its game DLLs in
  `custom/`; it plays on AvP2's archives, from `../AVP2/`.

Every binary in them is built here (each directory's `MANIFEST.txt` says
where each file came from); from the retail games come only their data --
the `.rez` archives, with the retail game DLLs inside them zeroed, the
movies, the loose files the games read (`console.PCX`, `childmodel.map`,
Primal Hunt's `FontData.fnt`), the readmes, the shipped `autoexec.cfg` and
default profile -- and, in the legacy builds, the middleware we have no
source for: Bink, Smacker and Miles (the modern builds have open
replacements, built from `ext/` by `build/ext.sh`). Each starts as its launcher starts it, `lithtech.exe -cmdfile
avp2cmds.txt` (`build/avp2cmds.sh`: the rez stack, the game DLLs built
here last).

Each build also makes two archives of its game directory
(`build/package.sh`): `out/<name>-private.tar.gz`, with the retail games'
data, for your own machines only, and `out/<name>-public.tar.gz`, without
it, to give to other people. The public one carries scripts that copy the
data out of the player's own images of AvP2's two discs and Primal Hunt's
disc (European, or USA where the scripts list a USA set; a plain `.iso`,
redump's `.bin`/`.cue` or its `.zip`), AvP2's 1.0.9.6 patch from the
Primal Hunt disc, checking every file they write (`build/mkinstall.py`,
on `ext/tools/mkinstall.py`):

```sh
avp2-modern-release/linux-install.sh avp2-1.zip avp2-2.zip ph.zip      # Linux
avp2-modern-release/mac-install.command avp2-1.zip avp2-2.zip ph.zip   # a Mac, for CrossOver
```

On Windows, drop the three images onto `windows-install.bat`, which also
registers both games as their setups did. Each script also finds the
images in the game directory or the one above it by itself, and can be run
again.

Both archives carry the scripts that start the games
(`build/game-scripts/`): `linux-run-avp2.sh` and `linux-run-ph.sh` (each game under Wine, in
a prefix of its own, `~/.wine-avp2`, with the registry values the retail
setups wrote; `--launcher` for the launcher; `--fullscreen`, `--windowed`,
`--borderless` and `--res WxH` for the window, which is otherwise as the game
last left it; `--help`), and on Windows
`windows-run.bat [avp2|ph]`, or the launchers `AVP2\AVP2.exe` and
`PrimalHunt\PrimalHunt.exe`.

## Releases

GitHub Actions (`.github/workflows/public-archives.yml`) builds the four
public archives on every push and pull request, and checks the legacy
release with `make progress`. Pushing a tag `v*` makes a GitHub release of
it with the modern build's two public archives (`avp2src-<tag>.tar.gz` and
`avp2src-debug-<tag>.tar.gz`; a tag with a hyphen is a pre-release), and
every push to `master` whose builds pass replaces the pre-release
`nightly` with that commit's (`avp2src-nightly-<commit>.tar.gz`). The
release text is `.github/release_notes.md`. No retail data is in any of
them.

The retail game's bugs, as they are found, are listed in
[`docs/RETAIL_BUGS.md`](docs/RETAIL_BUGS.md); what the modern build changes
is [`docs/MODERN.md`](docs/MODERN.md).
