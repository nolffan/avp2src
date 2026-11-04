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

## NOLF's bugs AvP2 does not have

(none checked yet)
