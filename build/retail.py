# The retail game's layout, for tools/retailextract.py (it reads this file
# from build/; see its header): Aliens versus Predator 2: Primal Hunt
# (Europe), version 1.0.2.2, as rez/ installs it (rez/games/avp2-ph-eu,
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

# Bink and Smacker (the movies), Miles (mss32.dll and its MP3 decoder),
# and the Sierra updater the launcher can run (SierraUp.exe, SierraPt.dll
# and its language DLLs). Every binary of Monolith's and Third Law's links
# the C runtime statically: setup installs no Microsoft runtime DLL.
THIRD_PARTY_NAMES = {"binkw32.dll", "smackw32.dll", "mss32.dll", "mp3dec.asi", "mp3dec.as6",
                     "sierraup.exe", "sierrapt.dll", "sude.dll", "sues.dll", "sufr.dll",
                     "suit.dll"}

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
