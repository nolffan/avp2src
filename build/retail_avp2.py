# AvP2's own layout, for ext/tools/retailextract.py with
# RETAIL_LAYOUT=build/retail_avp2.py (build/retail.py is Primal Hunt's):
# Aliens versus Predator 2 (Europe) with the English 1.0.9.6 patch, as ext/rez/
# installs it (ext/rez/games/avp2-eu-1.0.9.6, build/retail.sh). Primal Hunt's
# engine supersedes this one; what is AvP2's alone, and so still a target,
# is its game -- the DLLs that play its campaigns -- and its launcher and
# dedicated server. Its engine stays a second witness, not a target.

# The archives, under their installed names. The patch replaced
# AVP2DLL.REZ (cshell.dll, object.lto) and AVP2L.REZ (cres.dll, sres.dll)
# whole, and added AVP2P1.REZ (data only), which its setup puts last in the
# rez stack ("Update Command Line": -rez AVP2P1.REZ). Our copies of the two
# with binaries have every PE entry zeroed.
REZ = ["ALIEN.REZ", "AVP2.REZ", "AVP2DLL.REZ", "AVP2L.REZ", "MARINE.REZ",
       "MULTI.REZ", "PREDATOR.REZ", "SOUNDS.REZ", "AVP2P1.REZ"]

DATA_DIRS = ["Movies"]
RETAIL_COPIES = [("autoexec.cfg", "autoexec.cfg.retail"), ("Profiles/Default.prf", "Default.prf.retail")]

# Bink and Smacker (the movies) and Miles (mss32.dll and its MP3 decoder),
# as build/retail.py's; the Sierra updater is left out.
THIRD_PARTY_NAMES = {"binkw32.dll", "smackw32.dll", "mss32.dll", "mp3dec.asi", "mp3dec.as6"}

# AvP2 1.0.9.6's engine is built here too (Primal Hunt's cannot run AvP2's
# game DLLs: docs/PLAN.md, package 10), but scored from out/reference/avp2/
# (build/reference.sh), not through this layout.
BUILT_ENGINE = []

# The retail images the checks compare against: the launcher, AVP2.exe,
# and AVP2Up.dll (the Sierra updater's resources); the dedicated server;
# and the 1.0.9.6 game DLLs out of AVP2DLL.REZ and AVP2L.REZ. Launch.dll is
# the same file as Primal Hunt's.
REFERENCE = ["AVP2.exe", "AVP2Up.dll", "AVP2Serv.exe"]
REFERENCE_GAME = [("cshell.dll", "CShell.dll", "AVP2DLL.REZ"),
                  ("object.lto", "Object.lto", "AVP2DLL.REZ"),
                  ("cres.dll", "CRes.dll", "AVP2L.REZ"),
                  ("sres.dll", "SRes.dll", "AVP2L.REZ")]

OPEN_RUNTIME = [
    ("binkw32.dll", "src/modern/binkw32 + ext/ffmpeg: our Bink player"),
    ("mss32.dll", "ext/openmiles: OpenMiles"),
]

# Microsoft's DirectMusic in the modern build's game directory, with the
# manifest that points lithtech.exe's COM at it (src/modern/directmusic/).
DIRECTMUSIC_MANIFESTS = [("src/modern/directmusic/lithtech.exe.manifest", "lithtech.exe.manifest")]
