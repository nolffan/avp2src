# The retail game's layout, for ext/tools/retailextract.py (it reads this file
# from build/; see its header): Aliens versus Predator 2: Primal Hunt
# (Europe), version 1.0.2.2, as ext/rez/ installs it (ext/rez/games/avp2-ph-eu,
# build/retail.sh). Primal Hunt (Third Law, June 2002) is the last release
# of the game: it "includes all of the changes made in Aliens Versus
# Predator 2 patches up to version 1.0.9.6" (its readme), and its engine,
# renderer, server and game DLLs are the newest there are. No later patch
# is known. Its European and USA releases are the same files; the Italian
# one differs only in its language archive, launcher and dedicated server.
#
# Primal Hunt installs beside AvP2, never over it, and plays only its own
# campaigns: AvP2's are AvP2's game DLLs' (build/retail_avp2.py, read with
# RETAIL_LAYOUT=build/retail_avp2.py).

# Its own archives. The launcher builds the rez stack in code, from these
# and from AvP2's install (AvP2's InstallDir in the registry: AVP2.REZ,
# SOUNDS.REZ, ALIEN/MARINE/PREDATOR.REZ, by PrimalHunt.exe's strings), with
# a "PatchRez" registry value and "custom"; the order is the launcher's to
# show (docs/PLAN.md, package 7). phunrez.bat unpacks AVP2.REZ, SOUNDS.REZ,
# AVP2L.REZ, AVP2DLL.REZ and AVP2X.REZ, in that order.
REZ = ["AVP2P.REZ", "AVP2DLL.REZ", "AVP2L.REZ", "MULTI.REZ", "DIALOGUE.REZ",
       "AVP2X.REZ", "AVP2P1.REZ"]

DATA_DIRS = ["ServerData"]
RETAIL_COPIES = [("autoexec.cfg", "autoexec.cfg.retail"), ("Profiles/Default.prf", "Default.prf.retail")]

# THIRD-PARTY RUNTIME: Bink and Smacker (the movies) and Miles (mss32.dll
# and its MP3 decoder), which the engine imports or loads: licensed
# middleware with no source. Every binary of Monolith's and Third Law's
# links the C runtime statically: setup installs no Microsoft runtime DLL.
# The Sierra updater the launcher can run (SierraUp.exe, SierraPt.dll, its
# language DLLs) is left out: its servers are long gone, and nothing needs
# it to play.
THIRD_PARTY_NAMES = {"binkw32.dll", "smackw32.dll", "mss32.dll", "mp3dec.asi", "mp3dec.as6"}

# The engine's binaries, ours to build: the engine, its server, the
# Direct3D renderer, the message strings and the CD-audio music driver
# (cdaudio.dll, CRedMgr: lithshared's redmgr.h).
BUILT_ENGINE = ["lithtech.exe", "server.dll", "d3d.ren", "ltmsg.dll", "cdaudio.dll"]

# The retail images the checks compare against: the engine; the launcher,
# PrimalHunt.exe, with NOLF's launcher DLL Launch.dll (AvP2's, the same
# file) and AVP2XUp.dll (the Sierra updater's resources); the dedicated
# server AVP2XServ.exe; and Primal Hunt's game DLLs out of AVP2DLL.REZ and
# AVP2L.REZ, under names apart from AvP2's (build/retail_avp2.py).
REFERENCE = BUILT_ENGINE + ["PrimalHunt.exe", "Launch.dll", "AVP2XUp.dll", "AVP2XServ.exe"]
REFERENCE_GAME = [("cshell.dll", "PHCShell.dll", "AVP2DLL.REZ"),
                  ("object.lto", "PHObject.lto", "AVP2DLL.REZ"),
                  ("cres.dll", "PHCRes.dll", "AVP2L.REZ"),
                  ("sres.dll", "PHSRes.dll", "AVP2L.REZ")]

OPEN_RUNTIME = [
    ("binkw32.dll", "src/modern/binkw32 + ext/ffmpeg: our Bink player"),
    ("mss32.dll", "ext/openmiles: OpenMiles"),
]

# --- the public archive's install scripts (build/mkinstall.py) -------------
# A game directory holds both games, each in a directory of its own as the
# retail setups install them (build/all.sh): AVP2/, Aliens versus Predator 2
# (Europe) with the English 1.0.9.6 patch, and PrimalHunt/, Primal Hunt
# (Europe). Their data is copied out of the player's own disc images -- the
# two discs of AvP2 (its InstallShield 6 cabinets span both: data1.cab,
# data2.cab on disc 1, data3.cab on disc 2; the campaigns' archives lie on
# disc 1 as they are) and Primal Hunt's one disc -- by
# build/game-scripts/linux-install.sh, windows-install.ps1 and
# mac-install.command with the list build/mkinstall.py writes. AvP2's
# 1.0.9.6 patch is on Primal Hunt's disc (Updates/avp2_en_096.exe, the same
# file as ext/rez/'s patches/avp2_en_096.exe), so the scripts read it from
# there and ask for the three discs alone. The European release is first;
# the USA one is the fallback, where its files are the same ones (mkinstall
# leaves out a set that does not have them all).
_AVP2 = "Aliens Versus Predator 2"
INSTALL_GAMES = [
    {"dir": "AVP2", "release": "avp2-eu-1.0.9.6", "layout": "build/retail_avp2.py",
     "regions": {"Europe": ("eu1", "eu2"), "USA": ("us1", "us2")},
     "update": "patches/avp2_en_096.exe"},
    {"dir": "PrimalHunt", "release": "avp2-ph-eu", "layout": "build/retail.py",
     "regions": {"Europe": ("pheu",), "USA": ("phus",)}},
]
INSTALL_DISCS = {
    "eu1": f"{_AVP2} (Europe) (Disc 1)",
    "eu2": f"{_AVP2} (Europe) (Disc 2)",
    "pheu": f"{_AVP2} - Primal Hunt (Europe)",
    "us1": f"{_AVP2} (USA) (Disc 1)",
    "us2": f"{_AVP2} (USA, Asia) (Disc 2)",
    "phus": f"{_AVP2} - Primal Hunt (USA)",
}
# where each Primal Hunt disc carries the 1.0.9.6 patch
INSTALL_UPDATE_ON = {"pheu": "Updates/avp2_en_096.exe", "phus": "Updates/avp2_en_096.exe"}
INSTALL_SETS = [("Europe", ["eu1", "eu2", "pheu"]), ("USA", ["us1", "us2", "phus"])]
# (the disc images they are made against: build/retail.sh, retail_discs)
INSTALL_TAG = "AVP2"
INSTALL_BAT_HEADER = r"""@echo off
rem windows-install.bat -- install the game data from your AvP2 and Primal Hunt discs.
rem
rem   Drop the images of the two discs of Aliens versus Predator 2 and the
rem   disc of Primal Hunt (the European or the USA releases) onto this
rem   file -- or put them in this folder or the one above it and
rem   double-click it. AvP2's 1.0.9.6 patch is read from the Primal Hunt
rem   disc. An image is a plain .iso, redump's .bin (or its .cue), or
rem   redump's .zip download; a .bin or .zip is first converted to a
rem   temporary .iso in this folder (up to 0.6 GB a disc, removed at the end).
rem
rem It checks each file's SHA-256, copies the games' data out of them into
rem this folder's AVP2 and PrimalHunt folders and checks every file it
rem writes. Nothing is installed or run from them. In the legacy build it
rem then registers both games as their setups did (the registry values the
rem retail launchers read: each game's folder, AvP2's patch archive).
setlocal
set "AVP2_SELF=%~f0"
set "AVP2_PS1=%TEMP%\avp2-install-%RANDOM%%RANDOM%.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$t=[IO.File]::ReadAllText($env:AVP2_SELF); $m='#'+'__AVP2_PS__'; $a=$t.IndexOf($m); $b=$t.IndexOf($m,$a+1); [IO.File]::WriteAllText($env:AVP2_PS1,$t.Substring($a,$b-$a))"
if errorlevel 1 goto failed
powershell -NoProfile -ExecutionPolicy Bypass -File "%AVP2_PS1%" "%~f0" %*
set "AVP2_RC=%errorlevel%"
del "%AVP2_PS1%" 2>nul
pause
exit /b %AVP2_RC%
:failed
echo Could not start PowerShell.
pause
exit /b 1"""

# Microsoft's DirectMusic in the modern build's game directory, with the
# manifest that points lithtech.exe's COM at it (src/modern/directmusic/).
DIRECTMUSIC_MANIFESTS = [("src/modern/directmusic/lithtech.exe.manifest", "lithtech.exe.manifest")]
