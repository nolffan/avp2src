# The retail game's layout, for tools/retailextract.py (it reads this file
# from build/; see its header): Aliens versus Predator 2 (Europe) with the
# English 1.0.9.6 patch, as rez/ installs it (rez/games/avp2-eu-1.0.9.6,
# build/retail.sh).

# The archives, under their installed names. The patch replaced
# AVP2DLL.REZ (cshell.dll, object.lto) and AVP2L.REZ (cres.dll, sres.dll)
# whole, and added AVP2P1.REZ (data only), which its setup puts last in the
# rez stack ("Update Command Line": -rez AVP2P1.REZ). Our copies of the two
# with binaries have every PE entry zeroed.
REZ = ["ALIEN.REZ", "AVP2.REZ", "AVP2DLL.REZ", "AVP2L.REZ", "MARINE.REZ",
       "MULTI.REZ", "PREDATOR.REZ", "SOUNDS.REZ", "AVP2P1.REZ"]

DATA_DIRS = ["Movies"]
# Setup installs no default player profile (Profiles/ is empty).
RETAIL_COPIES = [("autoexec.cfg", "autoexec.cfg.retail")]

# Bink and Smacker (the movies), Miles (mss32.dll and its MP3 decoder),
# and the Sierra updater the launcher can run (SierraUp.exe, SierraPt.dll).
# Every binary of Monolith's links the C runtime statically: setup installs
# no Microsoft runtime DLL.
THIRD_PARTY_NAMES = {"binkw32.dll", "smackw32.dll", "mss32.dll", "mp3dec.asi", "mp3dec.as6",
                     "sierraup.exe", "sierrapt.dll"}

# The engine's binaries, ours to build, as NOLF's: the engine, its server,
# the Direct3D renderer and the message strings.
BUILT_ENGINE = ["lithtech.exe", "server.dll", "d3d.ren", "ltmsg.dll"]

# The retail images the checks compare against: the engine; the launcher,
# AVP2.exe, with NOLF's launcher DLL Launch.dll and AVP2Up.dll (the Sierra
# updater's resources); the dedicated server; and the 1.0.9.6 game DLLs out
# of AVP2DLL.REZ and AVP2L.REZ.
REFERENCE = BUILT_ENGINE + ["AVP2.exe", "Launch.dll", "AVP2Up.dll", "AVP2Serv.exe"]
REFERENCE_GAME = [("cshell.dll", "CShell.dll", "AVP2DLL.REZ"),
                  ("object.lto", "Object.lto", "AVP2DLL.REZ"),
                  ("cres.dll", "CRes.dll", "AVP2L.REZ"),
                  ("sres.dll", "SRes.dll", "AVP2L.REZ")]

OPEN_RUNTIME = [
    ("binkw32.dll", "src/modern/binkw32 + ext/ffmpeg: our Bink player"),
    ("mss32.dll", "ext/openmiles: OpenMiles"),
]
