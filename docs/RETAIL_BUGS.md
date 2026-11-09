# Bugs in the retail game

Bugs in *Primal Hunt* (1.0.2.2) and *Aliens versus Predator 2* (1.0.9.6)
as they shipped, found while rebuilding them. Each is a mistake in the code
that a source change would fix. This is a living
list: add a bug when it is found, in the format below, whether or not it is
fixed yet.

The legacy build reproduces every one of them, on purpose: compiling to the
retail machine code means compiling its mistakes too. They are fixed in the
**modern build** (`make modern`, the `AVP2_MODERN` define; `MODERN.md`):
each fix goes behind `#ifdef AVP2_MODERN`, with the retail code kept in
the `#else`, so the legacy build is unchanged and still matches
(`make progress`).

An entry that is fixed says so under its heading, with the file the fix is
in and its `MODERN.md` id. One that is not says why, or which part of the
tree it waits for.

Every binary here is a reconstruction (`docs/PLAN.md`), so an entry is
listed only where our function is byte-identical to the retail one at the
address it names (`tools/exematch.py`; for a string, the string is in the
retail image too): it describes what the retail code does, not a guess.
Addresses are as in the retail images (`build/reference.sh` extracts them
into `out/reference/`).

Many of NOLF's bugs (nolfsrc's `docs/RETAIL_BUGS.md`), above all the
engine's and the `lithshared` libraries', are likely to be here too, since
AvP2 runs on the same engine generation: check each against the retail
binary before listing it. Those AvP2 does not have go at the end, so nobody
looks again.

**Fixed so far** (in the modern build): none yet.

## Format

```
### <What goes wrong, as a player or a programmer would put it>

<binary> <address> (<function>, <source file>). What the code does, and
what it should do; how it shows in the game, if it does.

Fixed in the modern build: <file>, MODERN.md <id>. (Or: not fixed, because ...)
```

## The bugs

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

Not fixed in the modern build yet.

### A command line of more than 150 arguments overruns the argument table

lithtech.exe 0x00403d80 (`clu_dsi_BuildCommandLine`,
src/engine/lithtech/clientwin.cpp). The argument count is checked after
the increment with `cmp $0x96; jg`, so the 151st argument is copied into
`szArgs[150]`, one past the end of the frame's array, both from the real
command line and from a `-cmdfile`. A `-cmdfile` whose read fails also
leaks the buffer allocated for it.

Not fixed yet.

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
0x2000), so clients do not hear of the change.

Not fixed yet: the override's signature has to follow the interface's.

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

Not fixed yet.

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

Not fixed yet.

### A world model pushing a no-slide object never corrects it

`wm_SeparateFromBox` (Primal Hunt server.dll 0x1002915d, lithtech.exe
0x004646c0; src/engine/server/commonphysics.cpp) tests `-|vSep| > 0`,
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

Not fixed yet.

### A command-line argument of 200 characters or more is left unterminated

Launch.dll 0x10001820 (`GetLithTechCommandLine`, src/launch/launch.cpp),
which AvP2 and Primal Hunt ship from NOLF unchanged. Every argument it
hands the engine -- the real command line's, the rez files, and each word
of the launcher's saved commands -- is copied with `strncpy(pArgs[n], s,
nMaxArgLen)`, and lithtech.exe passes 200-byte slots and `nMaxArgLen` 200
(src/engine/lithtech/clientwin.cpp). A string of 200 characters or more is
copied without its terminator, and the engine reads it on into the next
slot.

Not fixed yet (Launch.dll has no modern build).

### Every rez-file check leaks a find handle

Launch.dll 0x100025c0 and 0x10002640 (`ExistRezFile`, `ExistRezDir`,
src/launch/RezFind.cpp). Each asks `_findfirst` about the path and never
gives the handle back with `_findclose`; `FindRezFiles` makes up to five
such checks every time the engine asks for its command line. Harmless in
practice (once per start), but a leak.

Not fixed yet (Launch.dll has no modern build).

### Starting a CD play list while a track plays can skip its first track

cdaudio.dll 0x10001390 (the driver's window procedure;
src/engine/cdaudio/cdaudio.cpp). Every `MM_MCINOTIFY` is taken as "the
track finished" without checking wParam, so the notification for a play
that was superseded or stopped also advances the list: starting a play
list while a track is playing can skip to its second track.

Not fixed yet.

### The CD driver's GetSongFromList always answers nothing

cdaudio.dll 0x10001600 (`GetSongFromList`, src/engine/cdaudio/cdaudio.cpp)
returns NULL whatever it is asked.

Not fixed yet.

### Removing the playing CD list leaves a dangling pointer

cdaudio.dll 0x10001610 (`RemoveList`, src/engine/cdaudio/cdaudio.cpp) does
not clear the current-list pointer, so removing the list that is playing
leaves a pointer that `OnMCINotify` and `Resume` read later.

Not fixed yet.

### The launcher's Low and High detail buttons give the opposite profile

AVP2.exe 0x00406a40, PrimalHunt.exe 0x00406d70 (`CLauncherDlg::LaunchGame`,
src/launcher/LauncherDlg.cpp). The detail dialog stores 0 for Low, 1 for
Medium and 2 for High (`CDetailSettingsDlg`'s handlers, AVP2.exe 0x00402170,
0x00402190, 0x00402150), and `LaunchGame` turns 0 into
` ++PerformanceConfig .DefaultHigh` and 2 into `.DefaultLow`. On the first
run (or after "restore defaults"), a player who picks Low detail starts with
the high-detail profile, and High with the low one.

Not fixed yet: the launchers have no modern build yet.

### The launcher writes the command line as a format string

AVP2.exe 0x00406a40, PrimalHunt.exe 0x00406d70 (`CLauncherDlg::LaunchGame`,
src/launcher/LauncherDlg.cpp). The command line for lithtech.exe goes to
avp2cmds.txt through `fprintf(pFile, sString)`, the command line itself as
the format, so a `%` in it -- the user's own commands from the options
dialog, the registry's update and content command lines -- is taken for a
conversion and garbles the file, or reads arguments that were never passed.
It should be `fputs` (or a `"%s"` format).

Not fixed yet: the launchers have no modern build yet.

### The display dialog's close button does nothing

AVP2.exe, PrimalHunt.exe (`CDisplayDlg`, src/launcher/DisplayDlg.cpp). The
dialog's message map (AVP2.exe 0x00422a00) has entries for OK (1035) and
Cancel (1041) and none for its close button (1034), so clicking the "x" in
the display dialog's corner is ignored; NOLF2's launcher, the same code a
year on, adds `OnClose`.

Not fixed yet: the launchers have no modern build yet.

### A launcher string that will not load hides the message it was for

AVP2.exe 0x00405970, PrimalHunt.exe 0x00405cc0 (`CLauncherApp::MessageBox`
by string ids, src/launcher/Launcher.cpp). When the caption's string fails to
load, the error is formatted into the text's `CString`, not the caption's, so
the box that follows shows the caption error as its message, with an empty
caption, and the message itself is lost.

Not fixed yet: the launchers have no modern build yet.

### The launcher's already-running checks ignore the directory they build

AVP2.exe 0x00404f50, PrimalHunt.exe 0x004051a0 (`CLauncherApp::InitInstance`,
src/launcher/Launcher.cpp). It gets the current directory and appends a
backslash, then looks for running `LITHTECH.EXE` and `SETUP.EXE` by their
bare names (`sprintf(sExe, "LITHTECH.EXE")`), never using it: any process of
those names -- another LithTech game's included -- keeps the launcher from
starting.

Not fixed yet: the launchers have no modern build yet.

### A dynamic lightmap is cleared only a quarter of the way (probable)

d3d.ren 0x10033512 and 0x10033652 (the 16- and 32-bit dynamic-light fills,
src/engine/d3d/ph/lmdynamic.cpp). Each clears the lightmap with
`memset(pData, 0, (m_Pitch * height) >> 2)`. The pitch is in bytes, so
that is a quarter of the buffer. The fill then writes only the pixels
inside the light's sphere, so below the cleared quarter, the texels outside
the sphere keep whatever the buffer held before. Probable: it rests on
`m_Pitch` being in bytes, as it is wherever else the renderer uses it.

Not fixed yet (the renderer has no modern build).

### Unsorted translucent world models lose their blend states before drawing

d3d.ren 0x100318b0 (`d3d_DrawNoSortWorldModel`,
src/engine/d3d/ph/drawworldmodel.cpp). The world models whose first
surface is marked not to be sorted are queued last with this draw
function, which picks additive blend states (SRCALPHA/ONE for
`FLAG2_ADDITIVE` objects, ONE/ONE with a black fog colour otherwise) in
`StateSet`s scoped to its `if`/`else` arms. The states are restored as the
arms end, before the one draw call after them, so these world models are
drawn with whatever blend was set already (the ordinary translucent one),
not additively.

Not fixed yet (the renderer has no modern build).

### The renderer's box test builds three of a box's six faces at the wrong place

d3d.ren 0x10031fc5 (`i_BoundingBoxTest`,
src/engine/d3d/ph/fullintersectline.cpp). The segment-against-box test
takes every face's plane distance from the box's minimum corner, so the
planes of the +X, +Y and +Z faces are wrong. A segment through one of
those faces gets an intersection point and plane that are off. NOLF2's
engine still has the same code.

Not fixed yet (the renderer has no modern build).

### A lightmap page leaks its allocation bitmap when there is no lightmap format

d3d.ren 0x10034f32 (the lightmap pager's page allocator,
src/engine/d3d/ph/lmpaging.cpp). When the device offers no lightmap
texture format, the new page is freed but its allocation bitmap is not.
The surface-creation failure path just below frees both.

Not fixed yet (the renderer has no modern build).

### A dynamic-lightmap texture whose surface cannot be made locks a null surface

d3d.ren 0x1001ddea (`CD3DLMTexture::Init`,
src/engine/d3d/ph/d3d_optimizedsurface.cpp). When DirectDraw's
`CreateSurface` fails, Init carries on and locks the surface it never got,
calling through a null pointer.

Not fixed yet (the renderer has no modern build).

### The models' specular pass sets its texture-stage states and undoes them at once

d3d.ren 0x100254d8 (`CD3DModelDraw::DrawPasses`,
src/engine/d3d/ph/drawmodel_env.cpp). The specular pass writes its
stage-1 states (colour op ADD, texture and current arguments, clamp
addressing) as unnamed `StageStateSet` temporaries. Each one restores the
state it set as soon as it is made, so the pass draws with stage 1 as it
was before. These should be named objects that live until the draw is
done.

Not fixed yet (the renderer has no modern build).

### A screen-space lightmap polygon is copied without checking its vertex count

d3d.ren 0x10036282 (`d3d_DrawScreenLMPoly`,
src/engine/d3d/ph/drawlightmaps.cpp). The polygon's vertices are copied
into a 64-entry stack buffer without testing the count. The world-space
lightmap draw beside it (0x10035e34) does test its count. A polygon of
more than 64 vertices overruns the stack.

Not fixed yet (the renderer has no modern build).

### SVAR OldCloak writes through a null message if one cannot be made

PHObject.lto 0x10051a20 (`CCommandMgr::ProcessSVar`,
src/AVP2/ObjectDLL/CommandMgr.cpp). Setting the `OldCloak` server variable
with the `SVAR` object command sends its value to the clients: the message
pointer starts null and `CreateMessage`'s result is not checked, so if no
message can be made the float is written, and the message sent and
released, through a null pointer. It should give up when `CreateMessage`
fails.

Not fixed yet (Primal Hunt's game DLLs have no modern build yet).

### The player-view fx cache writes and reads one type, past the end of each list

PHCShell.dll 0x100cb300 and 0x100cb3c0 (`PVFX::Write`, `PVFX::Read`,
src/AVP2/Shared/FXButeMgr.cpp). Primal Hunt's bute cache writes each
player-view fx's scale, dynamic light and sound fx types; each of the three
loops has a stray semicolon (`for (...);`), so it runs empty and then
writes the one entry indexed by the count -- one past the last type, and
past the end of the ten-entry array when the list is full. Reading back
does the same, so a PVFX loaded from the cache keeps only that one
(garbage) entry.

Not fixed yet.

### A scale fx loaded from the bute cache has an undefined additive-alpha flag

PHCShell.dll 0x10108670, 0x10108780, 0x10108960 (`CScaleFX::CScaleFX`,
`CScaleFX::Write`, `CScaleFX::Read`, src/AVP2/Shared/FXStructs.cpp).
Primal Hunt added `bAdditiveAlpha` ("AdditiveAlpha") to the scale fx
attributes; `Init` reads it, but the constructor does not clear it and the
cache's `Write`/`Read` leave it out, so a scale fx read from the cache
renders with additive alpha or not depending on what the allocation held.

Not fixed yet.

### A cached projectile fx with class data of kind 0 crashes on load

PHCShell.dll 0x100c5800 (`PROJECTILEFX::Read`,
src/AVP2/Shared/FXButeMgr.cpp). Reading a projectile fx from Primal Hunt's
bute cache, a flag byte says class data follows and a second byte gives its
kind; the switch makes nothing for kind 0 (and a null pointer for an
unknown kind) and then calls `Read` through the pointer regardless. A cache
holding such a record dereferences null. (`Write` never writes kind 0 for a
real class, so it takes a damaged cache file.)

Not fixed yet.

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
so it keeps returning the old value.

Not fixed yet.

### The attribute tables leak on every Term

CShell.dll 0x1012bb10, PHCShell.dll 0x1014e1d0, Object.lto 0x10202320,
PHObject.lto 0x10221e20 (`CButeMgr::Term`, src/lithshared/ButeMgr/
butemgr.cpp). Term frees each tag's items and clears the four tag tables,
but never deletes the TableOfItems each tag owns. NOLF2's ButeMgr deletes
them.

Not fixed yet.

### GetString into a buffer leaves the buffer untouched when the symbol is missing

PHCShell.dll 0x101511b0 (`CButeMgr::GetString(tag, att, defVal, szResult,
maxLen)`, src/lithshared/ButeMgr/butemgr.cpp). When the attribute is not
found the default is returned in a CString and then dropped: only a found
string is copied into `szResult`, so the caller's buffer keeps whatever it
held, not `defVal`.

Fixed in the modern build: butemgr.cpp copies `defVal`.

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

Not fixed yet (Primal Hunt's lists, MPRestrictList.h).

### A cached string longer than its buffer throws every later read off

PHObject.lto 0x10154d00 and 0x101550f0 (`CMultiFile_FILE::ReadString`,
`CMultiFile_LITH::ReadString`, src/AVP2/Shared/MultiFile.cpp). A string
longer than the buffer is cut to fit, but the rest of it is left unread,
so everything read after it from the bute cache comes from the wrong
place. CCharacterButeMgr never reads its cache (its ReadCache, 0x1000e580,
first calls the base class's, which always fails); whether another manager
does is not checked yet.

Not fixed yet.

### Losing a carried turret drops the player's last one too

PHObject.lto 0x10116ac0 (`CPlayerObj::RemoveTurret`, called twice by
`TurretDestroyed`, 0x10116bb0; src/AVP2/ObjectDLL/PlayerObj.cpp). The
turret's slot is emptied and the count lowered before the slots above it
are moved down, and the move stops at the new count less one, so the last
turret is never moved down and falls off the list: a player carrying two
whose first is destroyed is left carrying none.

Not fixed yet.

### A fading fog volume takes its blue for its red

PHObject.lto 0x1015b9e0 (`VolumetricFog::OnUpdate`,
src/AVP2/ObjectDLL/VolumetricFog.cpp). While a FADEALPHA message fades
the volume, its color is set to (blue, green, blue) instead of (red,
green, blue), so a colored fog volume changes color as it fades; only a
grey one looks right.

Not fixed yet.

### A tentacle's attack and burrow updates are swapped

PHObject.lto 0x10156790 (`CTentacle::OnUpdate`,
src/AVP2/ObjectDLL/Tentacle.cpp). StartAttack (0x101575b0) sets state 4
and Burrow (0x101574b0) state 3, but the update switch runs the burrow
update (wait for the burrow timer, then hide) for state 4 and the attack
update (wait for the attack timer, then burrow) for state 3. An attack
ends on its first update, since the burrow timer ran out long ago, and a
burrow restarts on every update once the attack timer has run out.

Not fixed yet.

### A turret gun that can turn all the way round never wraps its yaw

PHObject.lto 0x1015a340 (`CTurretGun::Rotate`,
src/AVP2/ObjectDLL/TurretGun.cpp). With a Yaw Range of a full turn the
yaw is meant to be wrapped into one turn, but the ceil() that would do it
has its result thrown away, so the yaw grows without bound.

Not fixed yet.

### A restriction list's "allowed" check reads one past its last entry

PHObject.lto 0x10155e20 and 0x10155e40 (`MPRestrict_IsClassAllowed`,
`MPRestrict_IsWeaponAllowed`, src/AVP2/ObjectDLL/MPRestrictions.cpp).
The index is checked with `i > count` instead of `i >= count`, so the
entry just past the list is read; it is always cleared, so it reads as
not allowed. Harmless with the callers' indices (from the Find
functions), but wrong.

Not fixed yet.

### Deleting a track key leaves a duplicate that is freed twice

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrack::DeleteKey`,
src/lithshared/lithtrackmgr/LithTrack.cpp). The keys after the deleted one
move down a slot, but the last slot is never cleared, so two slots point at
the same key and `Term` deletes it twice.

Not fixed yet.

### Deleting a track leaves a duplicate that is freed twice

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackList::Delete(int)`,
src/lithshared/lithtrackmgr/LithTrackList.cpp). The same as the key list:
the last slot is never cleared after the tracks move down.

Not fixed yet.

### The track editor's "Font Type" command never changes the font

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackMgr::Ext_Type`,
src/lithshared/lithtrackmgr/LithTrackMgr.cpp). It assigns the new type to
its own pointer parameter instead of the setting, so `TrackEdit Font Type
<x>` rebuilds the same font.

Not fixed yet.

### The track editor's help text ignores the bar's width

LithTrackMgrClient.lib, linked into CShell.dll
(`LithTrackBarHelp::SetupLines`, src/lithshared/lithtrackmgr/LithTrackBarHelp.cpp).
All five `CreateTextItem` calls pass the width where the justification
goes, so the width is the default 0: the help is never wrapped to the bar,
and a width whose low byte is 1 or 2 centres or right-justifies the lines.

Not fixed yet.

### The track editor's draw object always reports failure

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackDrawObj::Init`,
src/lithshared/lithtrackmgr/LithTrackDrawObj.cpp). It returns `DFALSE` even
when its line system was created.

Not fixed yet.

### A GameSpy secure query can overrun the server's buffer

GameSpyMgr.lib, linked into Object.lto and PHObject.lto
(`CGameSpyMgr::OnSecureQuery`, src/lithshared/gamespy/gamespymgr/GameSpyMgr.cpp).
The `secure` value from the network packet is copied with `strcpy` into a
64-byte buffer with no length check; a long value overruns the stack.

Not fixed yet.

### The LAN server list searches an unterminated reply

GameSpyClientMgr.lib, linked into CShell.dll and PHCShell.dll
(`ServerListLANList`, src/lithshared/gamespy/gamespyclientmgr/gserverlist.c).
The reply buffer is searched with `strstr` without being null-terminated
first (the query loop does terminate its replies), so a full buffer is read
past its end.

Not fixed yet.

### Sorting the server list by an unknown mode uses an uninitialised comparator

GameSpyClientMgr.lib (`ServerListSort`, gserverlist.c). The switch on the
compare mode has no default case, so an unknown mode sorts with whatever
the comparator variable holds.

Not fixed yet.

### The server browser never checks or closes sierraup.cfg

GameSpyClientMgr.lib (`CGameSpyClientMgr::SetupDirServers`,
src/lithshared/gamespy/gamespyclientmgr/GameSpyClientMgr.cpp). The `fopen`
result is used unchecked, so a missing file crashes, and the file is never
closed. Primal Hunt's build (PHCShell.dll 0x10138830, byte-identical)
checks the open but still never closes the file.

Not fixed yet.

### Refreshing LAN servers removes other games' servers by the wrong port

GameSpyClientMgr.lib (`CGameSpyClientMgr::RefreshLANServers`). Servers of
other games are removed by their game port, but `ServerListRemoveServer`
looks them up by query port. A server it misses stays in the list while
the count still drops, so the servers after it go unchecked.

Not fixed yet.

### A tracer without SingleSegment takes whatever its memory held

PHObject.lto 0x1001a480 (`TRACERFX::Init`, src/AVP2/Shared/FXButeMgr.cpp).
Primal Hunt's tracer gains SingleSegment, HeadVelocity, Flare and FlareScale,
but its constructor never sets `bSingleSegment`, so a tracer whose bute
has no SingleSegment attribute keeps the allocation's leftover value.

Not fixed yet.

### The "nelno" cheat's message isn't terminated

PHCShell.dll 0x10076180 (`CheatMgr::Cheat_Nelno`, name ours,
src/AVP2/ClientShellDLL/CheatMgr.cpp; byte-identical). The scrambled
"You need more training to defeat me!" is a 37-byte local array with no
terminating zero, so `ScrambleString` (Scramble.cpp) unscrambles on into
the three padding bytes and whatever follows them on the stack, until it
meets a zero or fills the 255-character buffer; the message can end in
garbage.

Not fixed yet.

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

Not fixed yet: the modern build's turret fixes are still to come.

### The turret recoil pushes the first-person weapon off by 0.95 on every axis

PHCShell.dll 0x10069e80 (`CameraMgrFX_RecoilUpdate`, name ours,
src/AVP2/ClientShellDLL/CameraMgrFX.cpp). While the recoil effect runs and
the player is not at a turret, the weapon model is moved by the camera's
recoil offset *plus* 0.95 on each axis (`vPos += 0.95f`), where scaling the
offset by 0.95 was surely meant: the weapon jumps up, right and forward by
nearly a unit for the effect's life.

Not fixed yet.

### Putting the weapon away for a turret hides muzzle flashes it may not have (probable)

PHCShell.dll 0x100b9f00 (`CWeaponModel::Holster`, name ours,
src/AVP2/ClientShellDLL/WeaponModel.cpp). For each of the two muzzle
flashes the function calls `Hide()` before testing the pointer, then
deletes it; `Hide` writes the flash's members, so a weapon with no flash
on a muzzle writes through a null pointer.

Not fixed yet.
