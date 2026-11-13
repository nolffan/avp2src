# Bugs in the retail game

Bugs in *Primal Hunt* (1.0.2.2) and *Aliens versus Predator 2* (1.0.9.6)
as they shipped, found while rebuilding them. Each is a mistake in the code
that a source change would fix. This is a living
list: add a bug when it is found, in the format below, whether or not it is
fixed yet.

The legacy build reproduces every one of them, on purpose: compiling to the
retail machine code means compiling its mistakes too. They are fixed in the
**modern build** (`make modern`, the `AVP2SRC_MODERN` define; `MODERN.md`):
each fix goes behind `#ifdef AVP2SRC_MODERN`, with the retail code kept in
the `#else`, so the legacy build is unchanged and still matches
(`make progress`).

An entry that is fixed says so under its heading, with the file the fix is
in and its `MODERN.md` id. One that is not says why, or which part of the
tree it waits for.

Every binary here is a reconstruction (`docs/PLAN.md`), so an entry is
listed only where our function is byte-identical to the retail one at the
address it names (`ext/tools/exematch.py`; for a string, the string is in the
retail image too): it describes what the retail code does, not a guess.
Addresses are as in the retail images (`build/reference.sh` extracts them
into `out/reference/`).

Many of NOLF's bugs (nolfsrc's `docs/RETAIL_BUGS.md`), above all the
engine's and the `lithshared` libraries', are likely to be here too, since
AvP2 runs on the same engine generation: check each against the retail
binary before listing it. Those AvP2 does not have go at the end, so nobody
looks again.

**Fixed so far** (in the modern build): every entry below is either fixed
or says why it is not. With an id: the world running fast above 100 frames
a second (M-10); a joystick button bound twice (M-7); the engine's
(M-B01..M-B09), the renderers' (M-B20..M-B26), the client shells' and
lithshared's (M-B40..M-B63), the object DLLs' and shared code's
(M-B70..M-B85) and the launchers' and Launch.dll's (M-B100..M-B107)
(MODERN.md). The flare crash that the controller work found was a
reconstruction error, not a retail bug, and is fixed in both builds (the
client's message-object vtable). The entries that say "Fixed in the modern
build" without an id are nolfsrc's engine fixes, imported with its code
and not yet checked here (MODERN.md, Where it stands).

## Format

```
### <What goes wrong, as a player or a programmer would put it>

<binary> <address> (<function>, <source file>). What the code does, and
what it should do; how it shows in the game, if it does.

Fixed in the modern build: <file>, MODERN.md <id>. (Or: not fixed, because ...)
```

## The bugs

### Above 100 frames a second the world runs fast

lithtech.exe and server.dll of both games, AvP2 1.0.9.6 and Primal Hunt
(`CServerMgr::Update`, src/engine/server/smgr.cpp; Primal Hunt
lithtech.exe 0x0048b5d0). The client calls the server's update once a
frame, and every call runs a game tick of the time since the last call
clamped to 0.01..0.2 s: a frame shorter than 10 ms still moves the world
10 ms on. So above 100 frames a second the game runs faster than real
time, (frame rate / 100) times -- AI, physics,
projectiles, timers, everything the server runs. Measured with the retail
AvP2 1.0.9.6 binaries (`ShowGameTime` against the wall clock, the first
Marine mission): 1.44 times real time at 139 frames a second, 2.37 at 237.
Retail machines rarely reached 100 then; today's do easily. (A dedicated
server limits its own loop, and is not affected.)

Fixed in the modern build: smgr.cpp, a tick runs only once 10 ms have
passed since the last; MODERN.md M-10.

### Binding a joystick button in Customize Controls leaves its old command bound too

CShell.dll of both games (`CFolderCustomControls::SetCurrentSelection`,
AvP2 0x1000a820, Primal Hunt 0x1000b380, and `Bind`, AvP2 0x1000af70,
Primal Hunt 0x1000bad0; src/AVP2/ClientShellDLL/FolderCustomControls.cpp;
read off the retail images, the functions not yet byte-identical). When a
control is pressed for a command, the page first takes it from whatever
command had it (`UnBind`), looking for it by its code, `"##<code>"`. That
is how `Bind` keeps a key or a mouse button, but a joystick button it
keeps by its name (`"Button 3"`), so the search never finds it: a joystick
button bound to a second command stays bound to the first as well, and
pressing it does both.

Fixed in the modern build: FolderCustomControls.cpp, a joystick's button
is looked for by its name; MODERN.md M-7.

### An unlisted Winsock error comes back as a number, not a string

lithtech.exe 0x004a22b0, server.dll 0x1004655c, AvP2 1.0.9.6 lithtech.exe
0x00498220 (`tcp_GetErrorText`, src/engine/server/tcpdriver.cpp). The
function looks the last Winsock error up in its 47-entry table and returns
the text; for a code not in the table it falls off the end (a bare `ret`
after the loop), so the caller receives the error code itself in `%eax`
and treats it as a `char*`. A message printed from it can crash or show
garbage.

Fixed in the modern build: tcpdriver.cpp, an unlisted code gives
"<unknown>".

### A world model whose cell grid fails to load leaks the model

lithtech.exe 0x0042b740, server.dll 0x1000b1a1, AvP2 1.0.9.6 lithtech.exe
0x00428ac0 and server.dll 0x1000af41 (`w_LoadWorldModel`,
src/engine/server/worldbsp.cpp). Every failure while reading a world model
deletes the half-built CWorldBsp and returns 2, except a failure of the
cell grid's Load, which returns 2 straight away (AvP2 server.dll 0x1000bcb4
jumps past the delete to 0x1000bdd2). The model and every array it owns
leak; only a corrupt world file reaches it.

Fixed in the modern build: worldbsp.cpp, that failure deletes the model too.

### The disconnect notice sends an uninitialised byte

lithtech.exe 0x004a1c40, server.dll 0x10046175, AvP2 1.0.9.6 lithtech.exe
0x00497c10 and server.dll 0x100416e1 (`TCPDriver::SendDisconnect`,
src/engine/server/tcpdriver.cpp). The notice is three bytes; the first two
are zeroed and the third is whatever the stack slot held (`push %ecx`
reserves it, nothing stores it), sent four times. Harmless unless the
receiver reads it.

Fixed in the modern build: tcpdriver.cpp, the third byte is zeroed.

### Texture reference counts are wiped on every texture sweep (probable)

lithtech.exe 0x00428940, AvP2 1.0.9.6 lithtech.exe 0x004261a0
(`tm_UnmarkTextures`, src/engine/lithtech/clienttexmgr.cpp). The unmark
pass clears SharedTexture's whole 16-bit word at +0x32 (`movw $0x0`),
which holds the reference count (15 bits) as well as the mark bit (bit
15). `tm_MarkUsedTextures` then keeps textures whose count is above zero
(`test $0x7fff`), and every count has just been zeroed, so a texture
referenced only through that count can be freed by the following sweep.
Not yet shown to happen in the game; the bytes alone do not say whether
it is deliberate. Reproduced: the retail build clears the mark and the
count as two statements, which VC6 merges into the one word store.
The count is a real reference count, not a per-sweep one: its only
writers are `LTTexMod::GetTextureHandle` (+1) and `ReleaseTextureHandle`
(-1, the texture data freed at zero; lttexmod.cpp), and its readers take
zero for "no handle" (`tm_IsValidTextureHandle`). So after a sweep a
texture-mod handle a game still holds is refused and its texture can be
freed under it. Neither game's DLLs ask for one, so it does not show in
AvP2 or Primal Hunt; a mod's would.

Fixed in the modern build: clienttexmgr.cpp, the sweep clears only the
mark; MODERN.md M-B01.

### A command line of more than 150 arguments overruns the argument table

lithtech.exe 0x00403d80 (`clu_dsi_BuildCommandLine`,
src/engine/lithtech/clientwin.cpp). The argument count is checked after
the increment with `cmp $0x96; jg`, so the 151st argument is copied into
`szArgs[150]`, one past the end of the frame's array, both from the real
command line and from a `-cmdfile`. A `-cmdfile` whose read fails also
leaks the buffer allocated for it.

Fixed in the modern build: clientwin.cpp, both loops stop once the table
holds 150 arguments (the bound is tested at the top of each, which also
covers Primal Hunt's quoted argument that is stored without the test),
and the failed read frees its buffer; MODERN.md M-B02.

### A socket naming the node one past the end is accepted

lithtech.exe 0x0045eb20, server.dll 0x10024cb0 (`GetSocketTransform`,
src/engine/server/commonmodel.cpp). The socket's node index is tested
with `>` against the node count instead of `>=` (0x0045ebd8, server.dll
0x10024d43), so a socket on node N, where N is the count, reads one
matrix past the object's cache.

Fixed in the modern build: commonmodel.cpp, `>=`.

### GetNextWMPoly crashes on a world model without a BSP

lithtech.exe 0x00486920 (`si_GetNextWMPoly`, src/engine/server/smgr.cpp).
The BSP's `IsBusy` is called before the BSP pointer is tested
(0x004869a5), so a world model with no BSP crashes instead of returning
LT_INVALIDDATA.

Fixed in the modern build: smgr.cpp, the pointer is tested first.

### Four model-interface errors report the wrong function

lithtech.exe 0x0045f100/0x0045f180 (`GetTimeScale`/`SetTimeScale`) report
under "GetAllowTransition"/"SetAllowTransition", and 0x0045fb90/0x0045fbe0
(`GetNormalize`/`SetNormalize`) under "GetLooping"/"SetLooping" (strings
at 0x004e5b54..58 and 0x004e5b9c..a0): copied error blocks never renamed.
Only the debug output is wrong.

Not fixed: harmless, kept for byte identity.

### childmodel.map is never closed

server.dll 0x10041166 (`sm_LoadChildModelMap`, Primal Hunt only,
src/engine/server/smgr.cpp; lithtech.exe's copy 0x0048ca10). The file is
opened and read line by line into the child-model table, and the loop
ends (server.dll 0x100417ff) with no `fclose`: one handle leaks for every
world started.

Fixed in the modern build: smgr.cpp, the file is closed.

### Removing an attachment reads its record after freeing it

`si_RemoveAttachment` (Primal Hunt lithtech.exe 0x00488270, server.dll's
copy; src/engine/server/smgr.cpp) takes the child's object id from the
attachment record after `am_RemoveAttachment` has returned the record to
its bank. It works only because the bank's free writes nothing but the
record's first dword.

Fixed in the modern build: smgr.cpp, the id is read first.

### Setting a model's animation time on the server is not sent to clients

server.dll 0x1003a130, lithtech.exe 0x00481b20, AvP2 1.0.9.6 lithtech.exe
0x0047abd0 and server.dll 0x10037173 (`ServerModelLT::SetCurAnimTime`,
src/engine/server/servermodel.cpp). AvP2's `ILTModel::SetCurAnimTime`
gained a third argument, `bIncorporateScale`; the server's override kept
NOLF's two-argument signature, so it overrides nothing and lands in a slot
of its own past the interface (slot 37 of server.dll's vtable 0x100757b8,
where slot 33 is still `CommonModelLT::SetCurAnimTime`, 0x100252b9). A game
calling the interface gets the shared accessor: the time is set, but the
string keys are not re-walked and the object is not flagged (change flag
0x2000), so clients do not hear of the change. (The object DLLs do call
it: `CharacterAnimation.cpp` randomises a character's first animation
time, and carries the time over when it changes animation.)

Fixed in the modern build: servermodel.h/.cpp, the override takes the
interface's three arguments and hands `bIncorporateScale` on, so it is
the interface's slot 33; MODERN.md M-B03.

### A long wildcard in the CRC string list overruns two buffers

lithtech.exe 0x0048bac0, server.dll 0x10040836
(`CServerMgr::CreateStringCRC`, src/engine/server/smgr.cpp). A
wildcard pattern's prefix and suffix are copied into 0x80-byte buffers
with no length check, from tokens of up to 0x3ff characters.

Fixed in the modern build: smgr.cpp, a pattern that does not fit is
skipped.

### A world name of 255 characters or more is left unterminated

lithtech.exe 0x0048bd70, server.dll 0x10040a44
(`CServerMgr::CreateWorldCRC`, src/engine/server/smgr.cpp). The name is
copied with `strncpy` limited to 0xff into a 0x100-byte buffer that is
never cleared, so a name that long has no terminator.

Fixed in the modern build: smgr.cpp, the last byte is zeroed; MODERN.md
M-B04.

### The packet trace reads past a truncated flag record

server.dll 0x1002b584 (`net_LogPacket`, Primal Hunt's server only,
src/engine/server/netmgr.cpp). The FLG arm checks for eight bytes and
reads ten (0x1002bae2..0x1002bb1e), so a truncated record is read two
bytes past its end. Only the debug trace is affected.

Not fixed: debug output only.

### A segment hitting a box's positive face reports the wrong plane

The unoptimised segment-query module's box test (lithtech.exe
0x0043bdf3/0x0043bf15 and its server.dll twin; AvP2 1.0.9.6's too;
src/engine/server/worldisect.cpp). For the three positive faces the hit
plane's distance is taken from the box's minimum (`min.coord * +1`)
instead of its maximum, so the intersection reports a plane on the far
side of the box.

Fixed in the modern build: worldisect.cpp, the maximum.

### DirectMusic notifications leak on a segment that is not the newest

`dm_NotificationThread` (lithtech.exe 0x0044b550, AvP2 1.0.9.6
0x004477c0; src/engine/lithtech/clientmusic.cpp). A segment-almost-ended
notification for a secondary, a motif, or a primary that is not the newest
leaves the loop with `continue` and never frees the notification message.
A queue entry of an unknown type also never advances the walk.

Fixed in the modern build: clientmusic.cpp, the message is freed (the
unknown-type walk is not changed yet).

### A second peer-authentication message uses a freed client

lithtech.exe 0x00411000 (Primal Hunt's peer-auth handler, message 13;
AvP2 1.0.9.6 0x004101c0;
src/engine/lithtech/clientmgr3.cpp). The handler deletes
`g_pPeerAuthClient` and never clears it, so a second message 13 calls
`HandleRecvMsg` on freed memory.

Fixed in the modern build: clientmgr3.cpp, a null test and the pointer
cleared after the delete.

### A trailing +command reads past the argument list

lithtech.exe 0x00413760 (`cm_RunPlusCommands`, AvP2 1.0.9.6 0x00412110;
src/engine/lithtech/clientmgr2.cpp). Given an argument list, the
`+command` loop runs up to the count itself, where the process command
line's form stops one short, so a trailing `+name` reads one entry past
the end.

Fixed in the modern build: clientmgr2.cpp, the bound.

### The model ray test's level of detail reads through a null pointer

The ray-vs-triangle worker behind `IntersectRays` (CalcLOD: lithtech.exe
0x0045ff50.., server.dll 0x10025b5c..; src/engine/server/commonmodel.cpp).
The level-of-detail distance lookup reads through a null pointer when the
level is past the end of the distance array.

Not fixed: not a bug. The null read is there (`xor %eax,%eax; fadds
(%eax)` at lithtech.exe 0x0045ff7b, and at 0x0045ffbf and 0x0045fffc; the
same in server.dll), but no level reaches it: it is the inline lookup's
answer for a level past `n` (the array's count), and every level handed
to it is in 0..n -- both offsets are clamped to 0..n first (`cmp; jle` at
0x0045ff4a and 0x0045ff94, the negative one through `neg; jns`), and the
loop runs i = 0..n (`lea 0x1(%eax)`, `jb`). It is code the compiler kept
for a branch that cannot be taken.

### A world model pushing a no-slide object never corrects it

`wm_SeparateFromBox` (Primal Hunt server.dll 0x1002915d, lithtech.exe
0x004646c0; AvP2 1.0.9.6 lithtech.exe 0x0045f640, byte-identical, and
server.dll 0x100275bc; src/engine/server/commonphysics.cpp) tests `-|vSep| > 0`,
which is never true, so an object that may not slide always gets no
separating correction.

Fixed in the modern build: commonphysics.cpp, `fMag < 0.0f`.

The next three are in `Model::LoadChildModels` (Primal Hunt server.dll
0x100225d0, lithtech.exe 0x0045b470; AvP2 1.0.9.6 server.dll 0x100213fc;
src/engine/server/commonmodel.cpp). Our function is not yet byte-identical
to the retail one (server.dll differs in frame-slot order and one register
choice, lithtech.exe in which helpers are expanded), so each is cited by
the retail instructions it is read from.

### A model whose header will not read loads as a success

The reader answers a step number, not an `LTRESULT`: each failure stores
its step in a byte and the shared exit returns it. The first step, the
"Header" chunk, is step 0 (server.dll 0x10022639, `and %al,0xb(%ebp)`
with `%al` zero; lithtech.exe 0x0045b526; AvP2 1.0.9.6 0x1002145b), so a
model file whose header is missing or of an unknown version is released
and then reported as `LT_OK`. `se_LoadModelData` takes the zero as success
and hands back the empty model. (The other steps, 1 to 29, come back as
whatever `LTRESULT` has that number, which only muddles the "Error %d
loading model" message.)

Fixed in the modern build: commonmodel.cpp, step 0 answers `LT_ERROR`.

### A model table that cannot be allocated loads as a success

The same reader: when the table at `Model+0x8c` cannot be sized
(server.dll 0x100226ab, lithtech.exe 0x0045b5f7; AvP2 1.0.9.6
0x100214c7) it jumps straight to the success return, without reading the
model's pieces, nodes, child models or animations and without the release
every other failure gets.

Fixed in the modern build: commonmodel.cpp, it fails like the others.

### The child-model records leak when a model fails to load

The same reader reads each child's record into a `new` object (server.dll
0x1002292c), then deletes them all once the children are loaded
(0x10022c7e..0x10022cb2). Every failure in between leaves through the
unwind at 0x10022cb8, which frees the vectors and not the records in them,
and a record whose own read fails (0x10022960) is not in the vector yet.
The `new` is not checked for null either (0x1002294e stores through it).
Only a malformed or truncated model file reaches these paths.

Fixed in the modern build: commonmodel.cpp, the failure path deletes them.

### Linking two objects when the link bank cannot grow writes through null

`sm_CreateInterObjectLink` (Primal Hunt lithtech.exe 0x00447f70, server.dll
0x10018f02; src/engine/server/interlink.cpp). The link record comes from
`sb_Allocate` on the manager's bank, which answers null when the bank cannot
grow (lithtech.exe 0x00447fd2 jumps past the free-list pop on null), and the
type and both ends are stored through it unchecked; the node allocations
after it are not checked either. Out of memory, `CreateInterObjectLink` or
`GetSmartLinkBody` crashes instead of failing with `LT_ERROR`.

Fixed in the modern build: interlink.cpp, each of the three allocations is
checked, and a failure gives back what was already taken and answers
`LT_ERROR`; MODERN.md M-B05.

### A command-line argument of 200 characters or more is left unterminated

Launch.dll 0x10001820 (`GetLithTechCommandLine`, src/launch/launch.cpp),
which AvP2 and Primal Hunt ship from NOLF unchanged. Every argument it
hands the engine -- the real command line's, the rez files, and each word
of the launcher's saved commands -- is copied with `strncpy(pArgs[n], s,
nMaxArgLen)`, and lithtech.exe passes 200-byte slots and `nMaxArgLen` 200
(src/engine/lithtech/clientwin.cpp). A string of 200 characters or more is
copied without its terminator, and the engine reads it on into the next
slot.

Fixed in the modern build: launch.cpp, an argument too long for its slot
is cut to 199 characters and terminated; MODERN.md M-B100.

### Every rez-file check leaks a find handle

Launch.dll 0x100025c0 and 0x10002640 (`ExistRezFile`, `ExistRezDir`,
src/launch/RezFind.cpp). Each asks `_findfirst` about the path and never
gives the handle back with `_findclose`; `FindRezFiles` makes up to five
such checks every time the engine asks for its command line. Harmless in
practice (once per start), but a leak.

Fixed in the modern build: Launch.dll's RezFind.cpp, each handle is given
back (`_findclose`) once the check has its answer; MODERN.md M-B101. The
launchers' own `ExistRezFile` and `ExistRezDir` (src/launcher/RezFind.cpp)
are the same code and get the same fix, though theirs lie in the pages the
protection wrapper keeps encrypted, so it is not known from their bytes.

### Starting a CD play list while a track plays can skip its first track

cdaudio.dll 0x10001390 (the driver's window procedure;
src/engine/cdaudio/cdaudio.cpp). Every `MM_MCINOTIFY` is taken as "the
track finished" without checking wParam, so the notification for a play
that was superseded or stopped also advances the list: starting a play
list while a track is playing can skip to its second track.

Fixed in the modern build: cdaudio.cpp, only `MCI_NOTIFY_SUCCESSFUL` (the
track played to its end) moves the list on; MODERN.md M-B06.

### The CD driver's GetSongFromList always answers nothing

cdaudio.dll 0x10001600 (`GetSongFromList`, src/engine/cdaudio/cdaudio.cpp)
returns NULL whatever it is asked.

Fixed in the modern build: cdaudio.cpp, it answers the list's song (its
track number) at that index, counted from 0, and NULL past the end;
MODERN.md M-B07.

### Removing the playing CD list leaves a dangling pointer

cdaudio.dll 0x10001610 (`RemoveList`, src/engine/cdaudio/cdaudio.cpp) does
not clear the current-list pointer, so removing the list that is playing
leaves a pointer that `OnMCINotify` and `Resume` read later.

Fixed in the modern build: cdaudio.cpp, removing the current list clears
the pointer (the track playing plays on; at its end nothing follows, as
after the last track of a list); MODERN.md M-B08.

### The launcher's Low and High detail buttons give the opposite profile

AVP2.exe 0x00406a40, PrimalHunt.exe 0x00406d70 (`CLauncherDlg::LaunchGame`,
src/launcher/LauncherDlg.cpp). The detail dialog stores 0 for Low, 1 for
Medium and 2 for High (`CDetailSettingsDlg`'s handlers, AVP2.exe 0x00402170,
0x00402190, 0x00402150), and `LaunchGame` turns 0 into
` ++PerformanceConfig .DefaultHigh` and 2 into `.DefaultLow`. On the first
run (or after "restore defaults"), a player who picks Low detail starts with
the high-detail profile, and High with the low one.

Fixed in the modern build: LauncherDlg.cpp, Low gives `.DefaultLow` and
High `.DefaultHigh`; MODERN.md M-B102.

### The launcher writes the command line as a format string

AVP2.exe 0x00406a40, PrimalHunt.exe 0x00406d70 (`CLauncherDlg::LaunchGame`,
src/launcher/LauncherDlg.cpp). The command line for lithtech.exe goes to
avp2cmds.txt through `fprintf(pFile, sString)`, the command line itself as
the format, so a `%` in it -- the user's own commands from the options
dialog, the registry's update and content command lines -- is taken for a
conversion and garbles the file, or reads arguments that were never passed.
It should be `fputs` (or a `"%s"` format).

Fixed in the modern build: LauncherDlg.cpp, the command line is written
with `fputs`; MODERN.md M-B103.

### The launcher copies its command line into 1024 bytes of stack

AVP2.exe 0x00406a40, PrimalHunt.exe 0x00406d70 (`CLauncherDlg::LaunchGame`,
src/launcher/LauncherDlg.cpp). Before it writes avp2cmds.txt, the command
line it has built in a `CString` is copied with `strcpy` into `char
sString[1024]` on the stack, whatever its length. It holds the launcher's
own command line (`m_lpCmdLine`, as long as Windows allows), the saved
commands and the registry's update and content command lines (up to 259
characters each) besides the rez files and options, so a long enough
command line or set of commands overruns the stack frame.

Fixed in the modern build: LauncherDlg.cpp, the file is written from the
`CString` itself, with no copy; MODERN.md M-B107.

### The display dialog's close button does nothing

AVP2.exe, PrimalHunt.exe (`CDisplayDlg`, src/launcher/DisplayDlg.cpp). The
dialog's message map (AVP2.exe 0x00422a00) has entries for OK (1035) and
Cancel (1041) and none for its close button (1034), so clicking the "x" in
the display dialog's corner is ignored; NOLF2's launcher, the same code a
year on, adds `OnClose`.

Fixed in the modern build: DisplayDlg.cpp, the close button is mapped to
`OnCancel`, as the main dialog's close button is (`CLauncherDlg::OnClose`
cancels); MODERN.md M-B104.

### A launcher string that will not load hides the message it was for

AVP2.exe 0x00405970, PrimalHunt.exe 0x00405cc0 (`CLauncherApp::MessageBox`
by string ids, src/launcher/Launcher.cpp). When the caption's string fails to
load, the error is formatted into the text's `CString`, not the caption's, so
the box that follows shows the caption error as its message, with an empty
caption, and the message itself is lost.

Fixed in the modern build: Launcher.cpp, the caption's error goes into the
caption, as the text's goes into the text, so the box keeps its message;
MODERN.md M-B105.

### The launcher's already-running checks ignore the directory they build

AVP2.exe 0x00404f50, PrimalHunt.exe 0x004051a0 (`CLauncherApp::InitInstance`,
src/launcher/Launcher.cpp). It gets the current directory and appends a
backslash, then looks for running `LITHTECH.EXE` and `SETUP.EXE` by their
bare names (`sprintf(sExe, "LITHTECH.EXE")`), never using it: any process of
those names -- another LithTech game's included -- keeps the launcher from
starting.

Fixed in the modern build: Launcher.cpp, both checks look for the
process by the full path the directory was built for, so only this
directory's `lithtech.exe` or `setup.exe` stops the launcher; MODERN.md
M-B106.

### A dynamic lightmap is cleared only a quarter of the way (probable)

d3d.ren 0x10033512 and 0x10033652 (the 16- and 32-bit dynamic-light fills,
src/engine/d3d/ph/lmdynamic.cpp). Each clears the lightmap with
`memset(pData, 0, (m_Pitch * height) >> 2)`. The pitch is in bytes, so
that is a quarter of the buffer. The fill then writes only the pixels
inside the light's sphere, so below the cleared quarter, the texels outside
the sphere keep whatever the buffer held before. Probable: it rests on
`m_Pitch` being in bytes, as it is wherever else the renderer uses it.
(It is: `LMDrawDesc::Begin` takes it from the lock's `lPitch`, and both
fills divide it by the pixel size for their row step.) `Begin` has
already copied the polygon's lightmap into the buffer, or filled it black
where there is none, so it shows only on a polygon with a lightmap page:
the static lightmap's texels added below the light's quarter.

Fixed in the modern build: lmdynamic.cpp, both fills clear the whole
`m_Pitch * height`; MODERN.md M-B20.

### Unsorted translucent world models lose their blend states before drawing

d3d.ren 0x100318b0 (`d3d_DrawNoSortWorldModel`,
src/engine/d3d/ph/drawworldmodel.cpp). The world models whose first
surface is marked not to be sorted are queued last with this draw
function, which picks additive blend states (SRCALPHA/ONE for
`FLAG2_ADDITIVE` objects, ONE/ONE with a black fog colour otherwise) in
`StateSet`s scoped to its `if`/`else` arms. The states are restored as the
arms end, before the one draw call after them, so these world models are
drawn with whatever blend was set already (the ordinary translucent one),
not additively. Primal Hunt's only: AvP2's renderer holds its one set of
states over the draw.

Fixed in the modern build: drawworldmodel.cpp, the draw is made inside
each arm, with its states; MODERN.md M-B21.

### The renderer's box test builds three of a box's six faces at the wrong place

d3d.ren 0x10031fc5 (`i_BoundingBoxTest`,
src/engine/d3d/ph/fullintersectline.cpp). The segment-against-box test
takes every face's plane distance from the box's minimum corner, so the
planes of the +X, +Y and +Z faces are wrong. A segment through one of
those faces gets an intersection point and plane that are off. NOLF2's
engine still has the same code. (The point is right, the face's own
coordinate; the plane's distance is the wrong one: the box's width too
small along its normal.)

Fixed in the modern build: fullintersectline.cpp, each face's plane
distance from that face's own coordinate; MODERN.md M-B22.

### The model sun's variance reads the warble scale

d3d.ren 0x1000b67d (`d3d_InitModelSun`, src/engine/d3d/ph/drawmodel.cpp;
AvP2's the same). The table of sun intensities the models are lit by
spreads its sine by a clamped console value. The value it reads is
0x1005d8b4, the one the `WarbleScale` variable (common_stuff.cpp,
default 0.92) writes, not `ModelSunVariance`'s (this file's own, default
0.05): the sun's flicker follows the world-warble setting. Identical, in
both renderers.

Not fixed: not a bug. The table (`g_fModelSun`, named for the sun by us)
is the `ModelWarble` curve: only `d3d_WarbleTransform` reads it, scaling
each vertex between WarbleScale and 1, and RenderScene rebuilds it when
WarbleScale changes ("Rebuilding warble table"). `ModelSunVariance` is
the sun-visibility search's step, read there, and with it the table would
squash a model to 0.05 of its size. The source's comment is corrected.

### A lightmap page leaks its allocation bitmap when there is no lightmap format

d3d.ren 0x10034f32 (the lightmap pager's page allocator,
src/engine/d3d/ph/lmpaging.cpp). When the device offers no lightmap
texture format, the new page is freed but its allocation bitmap is not.
The surface-creation failure path just below frees both.

Fixed in the modern build: lmpaging.cpp, the bitmap is freed too;
MODERN.md M-B23.

### A dynamic-lightmap texture whose surface cannot be made locks a null surface

d3d.ren 0x1001ddea (`CD3DLMTexture::Init`,
src/engine/d3d/ph/d3d_optimizedsurface.cpp). When DirectDraw's
`CreateSurface` fails, Init carries on and locks the surface it never got,
calling through a null pointer.

Fixed in the modern build: d3d_optimizedsurface.cpp, Init fails there
(its caller already tests it); MODERN.md M-B24.

### The models' specular pass sets its texture-stage states and undoes them at once

d3d.ren 0x100254d8 (`CD3DModelDraw::DrawPasses`,
src/engine/d3d/ph/drawmodel_env.cpp). The specular pass writes its
stage-1 states (colour op ADD, texture and current arguments, clamp
addressing) as unnamed `StageStateSet` temporaries. Each one restores the
state it set as soon as it is made, so the pass draws with stage 1 as it
was before. These should be named objects that live until the draw is
done. Stage 1's colour op is DISABLE when the solid models are drawn
(`d3d_DrawSolidModels`), so the specular texture is bound and never
blended: no model shows its specular highlight. Only a model whose
command string says `SpecularEnable` takes this pass, and none of either
game's shipped models does.

Fixed in the modern build: drawmodel_env.cpp, named objects, as the bump
pass above it has; MODERN.md M-B25.

### A screen-space lightmap polygon is copied without checking its vertex count

d3d.ren 0x10036282 (`d3d_DrawScreenLMPoly`,
src/engine/d3d/ph/drawlightmaps.cpp). The polygon's vertices are copied
into a 64-entry stack buffer without testing the count. The world-space
lightmap draw beside it (0x10035e34) does test its count. A polygon of
more than 64 vertices overruns the stack. The lightmap-only draw
(0x100361d9, the same file) copies into a 64-entry buffer unchecked too;
our function is near, not identical (the loop's stores are scheduled
differently), so that one rests on our reading of it.

Fixed in the modern build: drawlightmaps.cpp, both test the count and
fail as 0x10035e34 does ("Error: vertex buffer overflow"); MODERN.md
M-B26.

### SVAR OldCloak writes through a null message if one cannot be made

PHObject.lto 0x10051a20 (`CCommandMgr::ProcessSVar`,
src/AVP2/ObjectDLL/CommandMgr.cpp). Setting the `OldCloak` server variable
with the `SVAR` object command sends its value to the clients: the message
pointer starts null and `CreateMessage`'s result is not checked, so if no
message can be made the float is written, and the message sent and
released, through a null pointer. It should give up when `CreateMessage`
fails.

Fixed in the modern build: CommandMgr.cpp, nothing is sent when no
message can be made; MODERN.md M-B70.

### The player-view fx cache writes and reads one type, past the end of each list

PHCShell.dll 0x100cb300 and 0x100cb3c0 (`PVFX::Write`, `PVFX::Read`,
src/AVP2/Shared/FXButeMgr.cpp). Primal Hunt's bute cache writes each
player-view fx's scale, dynamic light and sound fx types; each of the three
loops has a stray semicolon (`for (...);`), so it runs empty and then
writes the one entry indexed by the count -- one past the last type, and
past the end of the ten-entry array when the list is full. Reading back
does the same, so a PVFX loaded from the cache keeps only that one
(garbage) entry.

Fixed in the modern build: FXButeMgr.cpp, the loops without their
semicolons, and a count read back larger than its list taken as none;
MODERN.md M-B71. (No cache is ever written or read: `CGameButeMgr`'s
`WriteCache`/`ReadCache`, which every manager's calls first, return
false.)

### A scale fx loaded from the bute cache has an undefined additive-alpha flag

PHCShell.dll 0x10108670, 0x10108780, 0x10108960 (`CScaleFX::CScaleFX`,
`CScaleFX::Write`, `CScaleFX::Read`, src/AVP2/Shared/FXStructs.cpp).
Primal Hunt added `bAdditiveAlpha` ("AdditiveAlpha") to the scale fx
attributes; `Init` reads it, but the constructor does not clear it and the
cache's `Write`/`Read` leave it out, so a scale fx read from the cache
renders with additive alpha or not depending on what the allocation held.

Fixed in the modern build: FXStructs.cpp, the constructor clears it and
the cache writes and reads it, after the rest; MODERN.md M-B74.

### A cached projectile fx with class data of kind 0 crashes on load

PHCShell.dll 0x100c5800 (`PROJECTILEFX::Read`,
src/AVP2/Shared/FXButeMgr.cpp). Reading a projectile fx from Primal Hunt's
bute cache, a flag byte says class data follows and a second byte gives its
kind; the switch makes nothing for kind 0 (and a null pointer for an
unknown kind) and then calls `Read` through the pointer regardless. A cache
holding such a record dereferences null. (`Write` never writes kind 0 for a
real class, so it takes a damaged cache file.)

Fixed in the modern build: FXButeMgr.cpp, no class data made, none read
(kind 0 is written as its kind byte alone, so the file stays in step);
MODERN.md M-B72.

### A menu font draws every column of a column control left-justified

CShell.dll 0x10129d10 (`CLTGUIColumnTextCtrl::Render`,
src/lithshared/ltguimgr/ltguicolumntextctrl.cpp; LTGUIMgr, the release's
prebuilt `ltguimgr_LT2.lib`). With a menu font (three faces) the column is
drawn through `CLTGUIFont::DrawMenu`, which is passed the column's width
where its justification belongs; `DrawMenu` treats any value above 2 as
left, so a right- or centre-justified column draws left-justified from its
right-justify x.

Fixed in the modern build: ltguicolumntextctrl.cpp passes the column's
justification.

### Setting an attribute that only an expansion tag holds does not take

CShell.dll 0x1012e240, PHCShell.dll 0x10150850
(`CButeMgr::GetSymTabItem`, src/lithshared/ButeMgr/butemgr.cpp; the
release's prebuilt `ButeMgrNoMFC.lib`). AvP2's ButeMgr keeps the tags whose
names hold "EXPANSION" in a second tag table. When the main table has the
tag but not the attribute, `GetSymTabItem` (behind every `Set*`) finds the
second table's TableOfItems but searches the main table's again, twice,
and so creates the attribute in the auxiliary table; `FindSymTabItem`
(behind every `Get*`) looks in the second table before the auxiliary one,
so it keeps returning the old value. (That branch cannot be reached as
described: `Tag` refuses a tag already in either table, so no tag is in
both. What does happen is the case the branch was meant for: an expansion
tag is only in the second table, so `GetSymTabItem` takes it for a new
tag and creates the attribute in the new-tag table, behind the second
table's, and `Get*` keeps the old value all the same.)

Fixed in the modern build: butemgr.cpp, `GetSymTabItem` takes a tag
missing from the main table from the second one, so the attribute is
found and set where `FindSymTabItem` reads it, and a new attribute of an
expansion tag goes in the auxiliary table like any other existing tag's
(not in the new-tag table, which `Save` would write out as a second,
duplicate section). MODERN.md M-B40.

### The attribute tables leak on every Term

CShell.dll 0x1012bb10, PHCShell.dll 0x1014e1d0, Object.lto 0x10202320,
PHObject.lto 0x10221e20 (`CButeMgr::Term`, src/lithshared/ButeMgr/
butemgr.cpp). Term frees each tag's items and clears the four tag tables,
but never deletes the TableOfItems each tag owns. NOLF2's ButeMgr deletes
them.

Fixed in the modern build: butemgr.cpp, Term's traversal deletes each
tag's TableOfItems after its items (the callback is Term's alone).
MODERN.md M-B41. (Object.lto and PHObject.lto link the same library, so
the fix reaches the server's managers too.)

### GetString into a buffer leaves the buffer untouched when the symbol is missing

PHCShell.dll 0x101511b0 (`CButeMgr::GetString(tag, att, defVal, szResult,
maxLen)`, src/lithshared/ButeMgr/butemgr.cpp). When the attribute is not
found the default is returned in a CString and then dropped: only a found
string is copied into `szResult`, so the caller's buffer keeps whatever it
held, not `defVal`.

Fixed in the modern build: butemgr.cpp copies `defVal`.

### DPRINTF=COM2 sends the debug output to COM1's setting

The release's prebuilt `lith.lib`, DPRINTF.OBJ
(`dprintfinittype::dprintfinittype`, src/lithshared/lith/dprintf.cpp;
byte-identical to the prebuilt member). Reading the DPRINTF environment
variable, "COM2" selects `Com1`; NOLF's lith had `Com2` there (and `LPT1`
for "LPT2", which AvP2's has right). The release's build is `_WINDOWS`,
which then forces `Monochrome` whatever was read and compiles the serial
ports out, so it shows only in a console or DOS build.

Not fixed: no shipped image calls dprintf.

### A model with no model file crashes the server when its files are sent

server.dll 0x1003448c, AvP2 1.0.9.6 server.dll 0x10031aa9
(`sm_WriteObjectFiles`, src/engine/server/servercommon.cpp). The writer
sends each of an object's file identifiers, sending 0xffff for a missing
skin, but reads the model file's identifier through its pointer with no
null test: an object whose model file is missing dereferences null. NOLF's
three-identifier writer (0x10029087) had the same asymmetry.

Fixed in the modern build: servercommon.cpp, a missing model file sends
0xffff as a missing skin does.

### Inserting into a restriction list smears one entry and drops another

PHObject.lto 0x10032a9d (`CWeaponMgr::LoadWeaponButes`) and 0x1000bee5
(`CCharacterButeMgr::LoadButes`), the inlined `CMPRestrictList::Insert`
(src/AVP2/Shared/MPRestrictList.h). The lists of multiplayer classes and
weapons a server can restrict are kept sorted by ID; to make room for an
entry it moves the entries up from the bottom (`entries[j + 1] =
entries[j]` for j counting up), which copies the entry at the insertion
point over every entry above it, and it never raises the count, so the
last entry is lost. Only a bute file that lists the "_MP" sets or weapons
out of ID order is affected.
The server's own copies have it too: PHObject.lto 0x10155bc0 and
0x10155cf0 (`MPRestrict_AddClass`, `MPRestrict_AddWeapon`,
src/AVP2/ObjectDLL/MPRestrictions.cpp), filled from the game info.

Fixed in the modern build: MPRestrictList.h, ServerOptions.h
(`CMPAllowList`, the client's and the dedicated server's copy, PHCShell.dll
0x100a88d0) and MPRestrictions.cpp, the entries moved up from the top and
the count raised; MODERN.md M-B75.

### A cached string longer than its buffer throws every later read off

PHObject.lto 0x10154d00 and 0x101550f0 (`CMultiFile_FILE::ReadString`,
`CMultiFile_LITH::ReadString`, src/AVP2/Shared/MultiFile.cpp). A string
longer than the buffer is cut to fit, but the rest of it is left unread,
so everything read after it from the bute cache comes from the wrong
place. CCharacterButeMgr never reads its cache (its ReadCache, 0x1000e580,
first calls the base class's, which always fails); whether another manager
does is not checked yet.

Fixed in the modern build: MultiFile.cpp, the rest of the string is
skipped; MODERN.md M-B79. (No manager's cache is ever read: see the
player-view fx's entry.)

### Losing a carried turret drops the player's last one too

PHObject.lto 0x10116ac0 (`CPlayerObj::RemoveTurret`, called twice by
`TurretDestroyed`, 0x10116bb0; src/AVP2/ObjectDLL/PlayerObj.cpp). The
turret's slot is emptied and the count lowered before the slots above it
are moved down, and the move stops at the new count less one, so the last
turret is never moved down and falls off the list: a player carrying two
whose first is destroyed is left carrying none.

Fixed in the modern build: PlayerObj.cpp, every turret above the lost one
moves down and the slot left is cleared; MODERN.md M-B80.

### A fading fog volume takes its blue for its red

PHObject.lto 0x1015b9e0 (`VolumetricFog::OnUpdate`,
src/AVP2/ObjectDLL/VolumetricFog.cpp). While a FADEALPHA message fades
the volume, its color is set to (blue, green, blue) instead of (red,
green, blue), so a colored fog volume changes color as it fades; only a
grey one looks right.

Fixed in the modern build: VolumetricFog.cpp, (red, green, blue);
MODERN.md M-B81.

### A tentacle's attack and burrow updates are swapped

PHObject.lto 0x10156790 (`CTentacle::OnUpdate`,
src/AVP2/ObjectDLL/Tentacle.cpp). StartAttack (0x101575b0) sets state 4
and Burrow (0x101574b0) state 3, but the update switch runs the burrow
update (wait for the burrow timer, then hide) for state 4 and the attack
update (wait for the attack timer, then burrow) for state 3. An attack
ends on its first update, since the burrow timer ran out long ago, and a
burrow restarts on every update once the attack timer has run out.

Fixed in the modern build: Tentacle.cpp, each state runs its own update;
MODERN.md M-B82.

### A turret gun that can turn all the way round never wraps its yaw

PHObject.lto 0x1015a340 (`CTurretGun::Rotate`,
src/AVP2/ObjectDLL/TurretGun.cpp). With a Yaw Range of a full turn the
yaw is meant to be wrapped into one turn, but the ceil() that would do it
has its result thrown away, so the yaw grows without bound.

Fixed in the modern build: TurretGun.cpp, whole turns are taken off the
yaw (`fmod`); MODERN.md M-B83.

### A restriction list's "allowed" check reads one past its last entry

PHObject.lto 0x10155e20 and 0x10155e40 (`MPRestrict_IsClassAllowed`,
`MPRestrict_IsWeaponAllowed`, src/AVP2/ObjectDLL/MPRestrictions.cpp).
The index is checked with `i > count` instead of `i >= count`, so the
entry just past the list is read; it is always cleared, so it reads as
not allowed. Harmless with the callers' indices (from the Find
functions), but wrong.

Fixed in the modern build: MPRestrictions.cpp, `i >= count`; MODERN.md
M-B76.

### Deleting a track key leaves a duplicate that is freed twice

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrack::DeleteKey`,
src/lithshared/lithtrackmgr/LithTrack.cpp). The keys after the deleted one
move down a slot, but the last slot is never cleared, so two slots point at
the same key and `Term` deletes it twice.

Fixed in the modern build: LithTrack.cpp, the last slot moved from is
cleared. MODERN.md M-B42.

### Deleting a track leaves a duplicate that is freed twice

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackList::Delete(int)`,
src/lithshared/lithtrackmgr/LithTrackList.cpp). The same as the key list:
the last slot is never cleared after the tracks move down.

Fixed in the modern build: LithTrackList.cpp, the same. MODERN.md M-B43.

### The track editor's "Font Type" command never changes the font

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackMgr::Ext_Type`,
src/lithshared/lithtrackmgr/LithTrackMgr.cpp). It assigns the new type to
its own pointer parameter instead of the setting, so `TrackEdit Font Type
<x>` rebuilds the same font.

Fixed in the modern build: LithTrackMgr.cpp, the new type goes in the
setting itself (`m_lfCS.szFontName`, the only one passed), from a copy
that outlives the command's tokens. MODERN.md M-B44.

### The track editor's help text ignores the bar's width

LithTrackMgrClient.lib, linked into CShell.dll
(`LithTrackBarHelp::SetupLines`, src/lithshared/lithtrackmgr/LithTrackBarHelp.cpp).
All five `CreateTextItem` calls pass the width where the justification
goes, so the width is the default 0: the help is never wrapped to the bar,
and a width whose low byte is 1 or 2 centres or right-justifies the lines.

Fixed in the modern build: LithTrackBarHelp.cpp, the lines are
left-justified and wrapped to the width. MODERN.md M-B45.

### The track editor's draw object always reports failure

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackDrawObj::Init`,
src/lithshared/lithtrackmgr/LithTrackDrawObj.cpp). It returns `DFALSE` even
when its line system was created.

Fixed in the modern build: LithTrackDrawObj.cpp, it reports success when
the line system was made. (Its one caller, `LithTrackMgr::Init`, ignores
the result, so nothing changes in play.) MODERN.md M-B46.

### A GameSpy secure query can overrun the server's buffer

GameSpyMgr.lib, linked into Object.lto and PHObject.lto
(`CGameSpyMgr::OnSecureQuery`, src/lithshared/gamespy/gamespymgr/GameSpyMgr.cpp).
The `secure` value from the network packet is copied with `strcpy` into a
64-byte buffer with no length check; a long value overruns the stack.

Fixed in the modern build: GameSpyMgr.cpp (gamespymgr/), the value is
cut to the buffer. MODERN.md M-B47.

### The LAN server list searches an unterminated reply

GameSpyClientMgr.lib, linked into CShell.dll and PHCShell.dll
(`ServerListLANList`, src/lithshared/gamespy/gamespyclientmgr/gserverlist.c).
The reply buffer is searched with `strstr` without being null-terminated
first (the query loop does terminate its replies), so a full buffer is read
past its end.

Fixed in the modern build: gserverlist.c, the reply is terminated at its
length (`recvfrom` is given one byte less than the buffer, for that).
MODERN.md M-B48.

### Sorting the server list by an unknown mode uses an uninitialised comparator

GameSpyClientMgr.lib (`ServerListSort`, gserverlist.c). The switch on the
compare mode has no default case, so an unknown mode sorts with whatever
the comparator variable holds.

Fixed in the modern build: gserverlist.c, an unknown mode leaves the list
unsorted. (The game passes only the four known modes.) MODERN.md M-B49.

### The server browser never checks or closes sierraup.cfg

GameSpyClientMgr.lib (`CGameSpyClientMgr::SetupDirServers`,
src/lithshared/gamespy/gamespyclientmgr/GameSpyClientMgr.cpp). The `fopen`
result is used unchecked, so a missing file crashes, and the file is never
closed. Primal Hunt's build (PHCShell.dll 0x10138830, byte-identical)
checks the open but still never closes the file.

Fixed in the modern build: GameSpyClientMgr.cpp, AvP2 checks the open
as Primal Hunt does, and both close the file. MODERN.md M-B50.

### Refreshing LAN servers removes other games' servers by the wrong port

GameSpyClientMgr.lib (`CGameSpyClientMgr::RefreshLANServers`). Servers of
other games are removed by their game port, but `ServerListRemoveServer`
looks them up by query port. A server it misses stays in the list while
the count still drops, so the servers after it go unchecked.

Fixed in the modern build: GameSpyClientMgr.cpp, a server is removed by
its query port (`ServerGetQueryPort`), which the list finds it by.
MODERN.md M-B51.

### A tracer without SingleSegment takes whatever its memory held

PHObject.lto 0x1001a480 (`TRACERFX::Init`, src/AVP2/Shared/FXButeMgr.cpp).
Primal Hunt's tracer gains SingleSegment, HeadVelocity, Flare and FlareScale,
but its constructor never sets `bSingleSegment`, so a tracer whose bute
has no SingleSegment attribute keeps the allocation's leftover value.

Fixed in the modern build: FXButeMgr.cpp, the constructor clears Primal
Hunt's four tracer members; MODERN.md M-B73.


### A tracer without a Flare names its flare sprites from uninitialised memory

PHCShell.dll 0x1012cc70 (`CTracerFX::Update`,
src/AVP2/ClientShellDLL/TracerFX.cpp; read off the retail image, the
function not yet byte-identical) and PHObject.lto 0x1001a480
(`TRACERFX::Init`, src/AVP2/Shared/FXButeMgr.cpp; byte-identical). Every
Primal Hunt tracer, single-segment or not, makes its three flares on its
first update (0x1012d0b7: `_snprintf(name, 127, szFlare, i + 1)`, then
`CreateFlare`, 0x1012da40, which skips an empty name). `TRACERFX`'s
constructor leaves `szFlare` as the allocation held, and Init sets it only
when the bute has a Flare, which only EnergyFlechetteTracer has: the
Standard and Laser tracers (FX.txt's TracerFX0 and 1) format their sprite
names from leftover heap bytes -- nothing when the first is a zero, else a
sprite load that fails, or worse for a `%s` among them.

Fixed in the modern build: FXButeMgr.cpp, the constructor clears it (with
M-B73's members); MODERN.md M-B85.
### The "nelno" cheat's message isn't terminated

PHCShell.dll 0x10076180 (`CheatMgr::Cheat_Nelno`, name ours,
src/AVP2/ClientShellDLL/CheatMgr.cpp; byte-identical). The scrambled
"You need more training to defeat me!" is a 37-byte local array with no
terminating zero, so `ScrambleString` (Scramble.cpp) unscrambles on into
the three padding bytes and whatever follows them on the stack, until it
meets a zero or fills the 255-character buffer; the message can end in
garbage.

Fixed in the modern build: CheatMgr.cpp, the array ends in a zero.
MODERN.md M-B52.

### Naming the console's actions leaks a string per command

PHCShell.dll 0x1007b811 (`CGameClientShell::OnEngineInitialized`,
src/AVP2/ClientShellDLL/GameClientShell.cpp; near, one `jmp` short).
Primal Hunt's start-up runs "AddAction <name> <id>" for every command,
taking each name from `FormatString`, and never frees the HSTRING it gets:
one string leaks for each command, every time the client shell starts.

Fixed in the modern build: GameClientShell.cpp, each name's string is
freed once its command has run. MODERN.md M-B53.

### The multiplayer class check takes an index one past the list

PHCShell.dll 0x10005aa0, 0x10014260, 0x10086cb0 and 0x10097520
(`CBaseFolder::IsCharacterAllowed`, `CFolderHost::IsCharacterAllowed`,
the client shell's check of the character butes' list (not yet written),
`MultiplayerClientMgr::IsClassAllowed`, names ours; byte-identical except
the unwritten one;
src/AVP2/Shared/MPRestrictList.h `CMPRestrictList::IsAllowed`). The bounds
test is `index > count`, not `>=`, so the entry one past the last would be
read. Harmless as shipped: the index always comes from a search of the
same list, which gives a real entry or -1.

Fixed in the modern build: MPRestrictList.h, `IsAllowed` tests `i >=
count`; MODERN.md M-B77.

### The cloaked weapon's alpha floor writes the red channel

PHCShell.dll 0x100b46c2 (`CWeaponModel::UpdateWeaponModel`,
src/AVP2/ClientShellDLL/WeaponModel.cpp). Primal Hunt keeps the first-person
weapon at least a little visible: with the model's second flags' bit 11 set
(part of the cloaked look, 0x1800) the floor is 0.35, else 0.2. The 0.35
branch compares and stores the colour's red (the first `GetObjectColor`
output), not its alpha, and skips the 0.2 alpha floor, so a cloaked
weapon's alpha is not floored at all and its red is raised instead. Near.

Fixed in the modern build: WeaponModel.cpp, the 0.35 floor is the
alpha's. MODERN.md M-B54.

## NOLF's bugs AvP2 does not have

(none checked yet)

### A mounted turret with a full circle of yaw never wraps its yaw

PHCShell.dll 0x10135480 (`CClientTurret::Rotate`, name ours,
src/AVP2/ClientShellDLL/ClientTurretMgr.cpp). For a turret whose yaw range
is a full turn (6.28 or more) the function computes
`floor(m_fYaw / 2pi)` and throws the result away, so the yaw is neither
wrapped nor clamped and grows without bound as the player turns. Harmless
in practice (the angle is only fed to a rotation), but the wrap was
plainly meant.

Fixed in the modern build: ClientTurretMgr.cpp, the yaw is wrapped into
[0, 2 pi) (`m_fYaw -= floor(m_fYaw / 2 pi) * 2 pi`). MODERN.md M-B55.

### The turret recoil pushes the first-person weapon off by 0.95 on every axis

PHCShell.dll 0x10069e80 (`CameraMgrFX_RecoilUpdate`, name ours,
src/AVP2/ClientShellDLL/CameraMgrFX.cpp). While the recoil effect runs and
the player is not at a turret, the weapon model is moved by the camera's
recoil offset *plus* 0.95 on each axis (`vPos += 0.95f`), where scaling the
offset by 0.95 was surely meant: the weapon jumps up, right and forward by
nearly a unit for the effect's life.

Fixed in the modern build: CameraMgrFX.cpp, the offset is scaled by 0.95
(`vPos *= 0.95f`). MODERN.md M-B56.

### Putting the weapon away for a turret hides muzzle flashes it may not have (probable)

PHCShell.dll 0x100b9f00 (`CWeaponModel::Holster`, name ours,
src/AVP2/ClientShellDLL/WeaponModel.cpp). For each of the two muzzle
flashes the function calls `Hide()` before testing the pointer, then
deletes it; `Hide` writes the flash's members, so a weapon with no flash
on a muzzle writes through a null pointer.

Fixed in the modern build: WeaponModel.cpp, only a flash that is there is
hidden. MODERN.md M-B57.

### The host options' class and weapon lists accept the index one past the last entry

PHCShell.dll 0x10018ba0 (`CFolderHostOptions::OnCommand`,
src/AVP2/ClientShellDLL/FolderHostOptions.cpp; the check is
`CMPRestrictList::SetAllowed`, src/AVP2/Shared/MPRestrictList.h, and
`GetId` and `IsAllowed` beside it share it). The index is tested with
`i > m_nNumEntries` where `>=` was meant, so the entry just past the last
one is written; with a full list (32 entries) that entry is the count
itself (`GetId` returns 32) and its allowed flag the four bytes after the
list, which `SetAllowed` writes.

Fixed in the modern build: MPRestrictList.h, `SetAllowed`, `GetId` and
`GetName` test `i >= count`; MODERN.md M-B78. (The commands past the last
check box are the host options' own entry.)

### The host options take check-box commands past the end of their boxes

PHCShell.dll 0x10018ba0 (`CFolderHostOptions::OnCommand`,
src/AVP2/ClientShellDLL/FolderHostOptions.cpp). A class box command is
accepted up to `CMD_CLASS_FIRST + 32`, but there are only 24 class boxes, and
a weapon box command up to `CMD_WEAPON_FIRST + 64` (inclusive) for 64 weapon
boxes, so a stray command indexes past `m_aClassBoxes` or `m_aWeaponBoxes`.

Fixed in the modern build: FolderHostOptions.cpp, a command is taken
only for a box there is (below `CMD_CLASS_FIRST + 24`, `CMD_WEAPON_FIRST +
64`). MODERN.md M-B58.

### The host options' check boxes leak their bitmaps

PHCShell.dll 0x10018690 (`CHostCheckBox::CHostCheckBox`, name ours,
src/AVP2/ClientShellDLL/FolderHostOptions.h). Each of the 88 check boxes
news a `CBitmapCtrl` that its (empty) destructor never deletes.

Fixed in the modern build: FolderHostOptions.cpp/.h, the folder's `Term`
(and destructor) takes each box's bitmap out of the fixed controls, which
hold it only while its page shows, and deletes it, so it is deleted once
whichever page was last shown. MODERN.md M-B59.

### The server list's no-entry surface is never deleted

PHCShell.dll 0x1001aed0 (`CFolderJoin::Build`,
src/AVP2/ClientShellDLL/FolderJoin.cpp). Build creates
`Interface\Exp\Menus\noentry.pcx` each time it runs; the destructor
deletes only the lock surface.

Fixed in the modern build: FolderJoin.cpp, the destructor deletes it (and
the constructor clears its handle, which it left unset). MODERN.md M-B60.

### The server list checks the wrong number against a server's class list

PHCShell.dll 0x1001dda0 (`CFolderJoin::IsClassAllowed`, name ours,
src/AVP2/ClientShellDLL/FolderJoin.cpp). To mark the servers that do not
allow the player's class it looks up the profile's `m_nChar[m_nRace]`, an
index into that race's list of characters, among the class list's
character-set IDs, so the no-entry mark is shown for the wrong servers.
(So it was thought; see below.)

Not fixed: not a bug. The profile's `m_nChar[race]` is the race's
character set, not an index into its characters: Choose Character
(`FolderPlayer.cpp`, `FolderPlayerJoin.cpp`) stores
`m_nCharList[race][index]` in it, and `ProfileMgr.cpp`'s defaults are
sets (`GetSetFromModelType`). So it is the class list's own ID, as
`CBaseFolder::IsCharacterAllowed` and `CFolderHost::IsCharacterAllowed`
take it.

### The Corporate motion detector settings check the Marine's tag

PHCShell.dll 0x1004d720 to 0x1004de80 (the `CLayoutMgr::GetMD...` and
`Get...SurfaceName` functions, src/AVP2/ClientShellDLL/LayoutMgr.cpp). For the
Corporate motion detector each tests that `HUDMotionDetector` exists, then
reads `HUDMotionDetector_Corporate`; a layout without the Corporate tag reads
nothing (or zeros) instead of falling back.

Fixed in the modern build: LayoutMgr.cpp, all seventeen test the
Corporate tag, so a layout without it falls back to the Marine's. (Primal
Hunt's own layout has both, so nothing changes with its data.) MODERN.md
M-B61.

### The Marine weapon chooser's dual pistols check has no null test

PHCShell.dll 0x10057790 (`CMarineWepChooser::IsDualPistols`, name ours,
src/AVP2/ClientShellDLL/WeaponChooser.cpp). It reads the weapon's name
without testing the `GetWeapon` result, so an invalid weapon ID reads through
a null pointer.

Fixed in the modern build: WeaponChooser.cpp, no weapon is not the dual
pistols. MODERN.md M-B62.


### A tracer whose Init fails removes three garbage flare handles

PHCShell.dll 0x1012d970 (`CTracerFX`'s destructor) and 0x1012dbe0
(`CTracerFX::RemoveFlares`, name ours, src/AVP2/ClientShellDLL/TracerFX.h and
TracerFX.cpp). The constructor leaves the three flare handles unset; Init
clears them only after it has the tracer's TRACERFX, and Term (from the
destructor) deletes every non-null one. A tracer whose Init fails before that
point hands three uninitialised handles to `DeleteObject`. Identical.

Fixed in the modern build: TracerFX.h, the constructor clears them.
MODERN.md M-B63.

### Detaching a client from another's view writes through a null message

`sm_DetachClient` (Primal Hunt lithtech.exe 0x00477200, server.dll
0x10032452; AvP2 1.0.9.6 lithtech.exe 0x00470db0, server.dll 0x1002fc62;
src/engine/lithtech/ccobj.cpp for the engine, src/engine/server/smgr.cpp for
server.dll). When the client has a view object to report, the detach message
is never allocated: the engine's copies and Primal Hunt's server.dll write
the header words through an empty `CMessageRef`, AvP2's server.dll through a
null `ServerMessage*` (the stores go to absolute addresses 0x38 and 0x34),
and the null message is then handed to `sm_SendToClient`. Its sibling
`sm_AttachClient` allocates with `msg_AllocPacket` first. Detaching such a
client crashes the server. All four copies are identical.

Fixed in the modern build: ccobj.cpp and smgr.cpp, the message is
allocated (`msg_AllocPacket(0x44c, 0x44c)`, as `sm_AttachClient` does)
before it is written; MODERN.md M-B09.

### A facehugger whose target is not a character crashes its attack

PHObject.lto 0x10214eb7 (`SimpleAIFacehugger::UpdateAttack`,
src/AVP2/ObjectDLL/SimpleAIAlien.cpp). Primal Hunt added a test that the
target is not already dead before rolling the facehug chance, but reads
`GetCharTargetPtr()->IsDead()` (the character's `m_damage` dead flag,
+0x2ac) before the null test of that same pointer a few lines on. A
facehugger attacking a target that is not a character (a `GetTarget()`
with no character pointer) dereferences null. Identical.

Fixed in the modern build: SimpleAIAlien.cpp, the dead test only for a
character (any other target rolls the chance as in AvP2, and the test
after it turns it down); MODERN.md M-B84.

### AvP2's player code casts its attachments without testing the cast

Object.lto 0x101cd9c7 (`CPlayerObj::ChangeWeapon`), 0x101cda78
(`DoWeaponChange`), 0x101cbb52 (`FullAmmoCheat`), 0x101cbc96
(`FullWeaponCheat`), 0x101ccc99 (`AquireDefaultWeapon`), 0x101cbf78 and
0x101cc5f3 (`Respawn`), 0x101cdb17 (`UpdateInterface`), 0x101d35c1
(`SendMPInterfaceUpdate`); 0x101ce4b2 (`HandleGameRestore`) and 0x101d2e20
(`GetPlayerWeapon`) do not even cast (src/AVP2/ObjectDLL/PlayerObj.cpp).
Each `dynamic_cast<CPlayerAttachments*>(m_pAttachments)` result is used
without a null test (some test `m_pAttachments` first, none the cast), so a
player whose attachments are not a `CPlayerAttachments` dereferences null.
Primal Hunt casts once and tests the result everywhere. All identical.

Not fixed: the cast cannot fail. A player's attachments are made only by
`CPlayerObj::CreateAttachments` (virtual: from `CCharacter`'s
`MID_PRECREATE`, and again after `ResetCharacter` destroys them), always as
a `CPlayerAttachments`, before any of these runs; only a failed `new` would
leave them null.
