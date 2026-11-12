<!--
The release text (.github/workflows/public-archives.yml, the job "release"):
{{TAG}}, {{ARCHIVE}} and {{REPO}} are filled in, and this comment is left out.
-->
## Download

| File | What it is |
|---|---|
| `avp2src-{{ARCHIVE}}.tar.gz` | **This is what you want**: Aliens versus Predator 2 (1.0.9.6) and Primal Hunt (1.0.2.2) built from source, the modern build. |
| `avp2src-debug-{{ARCHIVE}}.tar.gz` | The same build with debug information. For bug reporting. |

**Work in progress.** The reconstruction is not finished, and the modern build's changes are not made yet: see [where it stands](https://github.com/{{REPO}}/blob/{{TAG}}/README.md#where-it-stands) before expecting a game that plays from start to end.

**No game data is included.** A platform specific installer is included that copies the games' data from your own images of the two discs of Aliens versus Predator 2 and the disc of Primal Hunt (read, not run). AvP2's 1.0.9.6 patch is read from the Primal Hunt disc.

Supported images, as redump names them (a plain `.iso`, redump's `.bin`/`.cue`, or redump's `.zip`):

- Europe: `Aliens Versus Predator 2 (Europe) (Disc 1)`, `Aliens Versus Predator 2 (Europe) (Disc 2)`, `Aliens Versus Predator 2 - Primal Hunt (Europe)`
- USA, where the installer lists a USA set: `Aliens Versus Predator 2 (USA) (Disc 1)`, `Aliens Versus Predator 2 (USA, Asia) (Disc 2)`, `Aliens Versus Predator 2 - Primal Hunt (USA)`

## Install

**This is the whole game, not a patch.** Unpack it into a new, empty directory, not over an earlier release or another installation. It holds both games: `AVP2/` and `PrimalHunt/`. Copy your old `Save` and `Profiles` folders into each to keep your saved games and profiles.

### Windows

Unpack the archive, drop the three disc images onto `windows-install.bat`, then start `AVP2\AVP2.exe` or `PrimalHunt\PrimalHunt.exe` (or `windows-run.bat avp2` / `windows-run.bat ph`, straight into the game).

### Linux

There's no native Linux build; the games are Windows programs, run under Wine or Proton.

```sh
tar xzf avp2src-{{ARCHIVE}}.tar.gz
cd avp2src-{{ARCHIVE}}
./linux-install.sh /path/to/avp2-disc1.zip /path/to/avp2-disc2.zip /path/to/primalhunt.zip
./linux-run-avp2.sh      # or: ./linux-run-ph.sh for Primal Hunt
```

`./linux-run-avp2.sh --help` lists its options (a Wine prefix of its own, `~/.wine-avp2`; the launchers).

### Mac

The games run in CrossOver (or another Wine for macOS). Unpack the archive, put the three disc images in its folder and double-click `mac-install.command`, then run `AVP2/AVP2.exe` or `PrimalHunt/PrimalHunt.exe` in a CrossOver bottle with Run Command. If macOS will not open the script (it is downloaded and not signed), allow it in System Settings > Privacy & Security ("Open Anyway"), or run it in Terminal: `./mac-install.command disc1.zip disc2.zip primalhunt.zip`.

## What the modern build changes

Every change from the retail games, and what is planned, is in [`MODERN.md`](https://github.com/{{REPO}}/blob/{{TAG}}/docs/MODERN.md); the retail games' own bugs are in [`RETAIL_BUGS.md`](https://github.com/{{REPO}}/blob/{{TAG}}/docs/RETAIL_BUGS.md). What is whose is in [`LICENSE_NOTES.md`](https://github.com/{{REPO}}/blob/{{TAG}}/docs/LICENSE_NOTES.md).

## Testing

The games are 32-bit Windows programs. They are tested on Linux with Wine; Windows and the Mac have had less testing. Please report problems in the [issues](https://github.com/{{REPO}}/issues).
