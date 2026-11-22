<!--
The release text (.github/workflows/ci.yml, the job "release").
The job fills in {{TAG}}, {{ARCHIVE}} and {{REPO}}, and removes this comment.
-->
## Download

| File | What it is |
|---|---|
| `avp2src-{{ARCHIVE}}.tar.gz` | **This is what you want**: Aliens versus Predator 2 (1.0.9.6) and Primal Hunt (1.0.2.2), built from source. This is the modern build. |
| `avp2src-debug-{{ARCHIVE}}.tar.gz` | The same build with debug information. Use it to report bugs. |

**Work in progress.** The reconstruction is not complete. The games are playable, but expect bugs. [Where it stands](https://github.com/{{REPO}}/blob/{{TAG}}/docs/MODERN.md#where-it-stands) gives the status of the modern build.

**No game data is included.** The archive includes an installer for each platform. The installer copies the data of the games from your own images of the two Aliens versus Predator 2 discs and the Primal Hunt disc. It reads the images, but it does not run them. It reads the AvP2 1.0.9.6 patch from the Primal Hunt disc.

Supported images (the redump dumps of the three European discs; a plain `.iso`, a raw `.bin`/`.cue`, or a `.zip` that holds one):

- AvP2 disc 1: `Aliens Versus Predator 2 (Europe) (Disc 1)`
- AvP2 disc 2: `Aliens Versus Predator 2 (Europe) (Disc 2)`
- Primal Hunt: `Aliens Versus Predator 2 - Primal Hunt (Europe)`

The USA discs do not have all the files of the European release (the AvP2 launcher, readme and EULA are not the same), so the installer does not accept them.

## Install

**This is the full game, not a patch.** For a first installation, unpack it into a new, empty directory and run the installer. It holds the two games: `AVP2/` and `PrimalHunt/`. **To upgrade, unpack it over your installed game** (the same build, legacy or modern): put the files of its directory into your game's directory, e.g. `tar xzf <archive> --strip-components=1 -C <game directory>`. Do not run the installer again: the files from your discs, your saved games, your profiles and your settings stay. `sha256sum -c SHA256SUMS` in the game directory checks the installation.

### Windows

1. Unpack the archive. To upgrade, copy the files in its folder over the installed game instead, and skip step 2.
2. Drop the three disc images onto `windows-install.bat`.
3. Start `AVP2\AVP2.exe` or `PrimalHunt\PrimalHunt.exe`. To go directly into the game, use `windows-run.bat avp2` or `windows-run.bat ph`.

### Linux

There is no native Linux build. The games are Windows programs, and they run under Wine or Proton.

```sh
tar xzf avp2src-{{ARCHIVE}}.tar.gz
cd avp2src-{{ARCHIVE}}
./linux-install.sh /path/to/avp2-disc1.zip /path/to/avp2-disc2.zip /path/to/primalhunt.zip
./linux-run-avp2.sh      # or: ./linux-run-ph.sh for Primal Hunt
```

`./linux-run-avp2.sh --help` shows its options. The options include the launchers. The script uses its own Wine prefix, `~/.wine-avp2`.

### Mac

The games run in CrossOver (or in another Wine for macOS).

1. Unpack the archive.
2. Put the three disc images in its folder.
3. Double-click `mac-install.command`.
4. In a CrossOver bottle, use Run Command to run `AVP2/AVP2.exe` or `PrimalHunt/PrimalHunt.exe`.

macOS can refuse to open the script, because it is downloaded and it is not signed. If this occurs, allow it in System Settings > Privacy & Security ("Open Anyway"). Or run it in Terminal: `./mac-install.command disc1.zip disc2.zip primalhunt.zip`.

## What the modern build changes

[`MODERN.md`](https://github.com/{{REPO}}/blob/{{TAG}}/docs/MODERN.md) gives each change from the retail games, and the planned changes. [`RETAIL_BUGS.md`](https://github.com/{{REPO}}/blob/{{TAG}}/docs/RETAIL_BUGS.md) gives the bugs of the retail games. [`LICENSE_NOTES.md`](https://github.com/{{REPO}}/blob/{{TAG}}/docs/LICENSE_NOTES.md) tells who owns which part.

## Testing

The games are 32-bit Windows programs. We test them on Linux with Wine. Windows and the Mac had less testing. Please report problems in the [issues](https://github.com/{{REPO}}/issues).
