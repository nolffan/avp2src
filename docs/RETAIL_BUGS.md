# Bugs in the retail game

This file gives the bugs in *Primal Hunt* (1.0.2.2) and *Aliens versus
Predator 2* (1.0.9.6) as they shipped. We found them when we rebuilt the
games. Each bug is a mistake in the code that a source change can fix. This
list is not complete. When you find a bug, add it in the format below. Add
it before or after you fix it.

The legacy build has each of these bugs, on purpose. The legacy build
compiles to the retail machine code, thus it also compiles the retail
mistakes. The **modern build** fixes the bugs (`make modern`, the
`AVP2SRC_MODERN` define; `MODERN.md`). Each fix is in an
`#ifdef AVP2SRC_MODERN` block, and the retail code stays in the `#else`
block. Thus the legacy build does not change and it still matches the
retail binaries (`make progress`).

When an entry is fixed, the text below its heading says so. It gives the
file that has the fix and its `MODERN.md` id. When an entry is not fixed,
the text gives the reason, or the part of the tree that it waits for.

Each binary here is a reconstruction (`docs/PLAN.md`). Thus we list an
entry only where our function is byte-identical to the retail function at
the address that the entry gives (`ext/tools/exematch.py`). For a string,
the string is also in the retail image. Thus each entry tells what the
retail code does. It is not a guess. The addresses are the addresses in
the retail images. `build/reference.sh` extracts the images into
`out/reference/`.

Possibly, many of the bugs of NOLF (nolfsrc's `docs/RETAIL_BUGS.md`) are
also in AvP2. This is most probable for the bugs in the engine and in the
`lithshared` libraries, because AvP2 uses the same engine generation.
Before you list such a bug, examine the retail binary for it. Put the NOLF
bugs that AvP2 does not have at the end of the file. Then nobody examines
them again.

**Fixed so far** (in the modern build): each entry below is fixed, or it
gives the reason why it is not fixed. These fixes have an id (MODERN.md):

- The world runs fast above 100 frames a second (M-10).
- A joystick button is bound two times (M-7).
- The turn of the mouse look changes with the frame rate (M-6).
- A window is placed for a frame that it does not have (M-9).
- The engine bugs (M-B01..M-B09).
- The renderer bugs (M-B20..M-B26).
- The bugs in the client shells and in lithshared (M-B40..M-B63, M-B108).
- The bugs in the object DLLs and in the shared code (M-B70..M-B85).
- The bugs in the launchers and in Launch.dll (M-B100..M-B107).

The controller work found a flare crash. That crash was a reconstruction
error, not a retail bug. Both builds have the fix (the message-object
vtable of the client). Some entries say "Fixed in the modern build" and
have no id. These are the engine fixes of nolfsrc. We imported them with
the nolfsrc code, and we did not examine them here yet (MODERN.md, Where
it stands).

## Format

Use this template for each entry:

```
### <What goes wrong, in the words of a player or a programmer>

<binary> <address> (<function>, <source file>). What the code does, and
what it must do. How the bug shows in the game, if it shows.

Fixed in the modern build: <file>, MODERN.md <id>. (Or: not fixed, because ...)
```

## The bugs

### A window is placed and sized for a frame it does not have

d3d.ren of both games (`d3d_Init`: AvP2 1.0.9.6 0x10010b22, Primal Hunt
0x100118be; src/engine/d3d/ph/common_init.cpp). The bug also involves the
window of lithtech.exe (`clu_dsi_RunClientApp`, clientwin.cpp:
`CreateWindowExA` style 0x10810000 at AvP2 0x00403172, Primal Hunt
0x004034e2).

When the game runs in a window (`Windowed 1`), the renderer centres the
window. These are the problems:

- The renderer calculates the top of the window from the *widths* of the
  desktop and the mode. Thus the top is the same value as the left. The
  code does `sub; sub; shr $1` one time and stores the result two times
  (AvP2 0x10010d04, Primal Hunt 0x10011a99).
- The calculation is unsigned. Thus, if the mode is wider than the
  desktop, the window goes 2^31 pixels off, not to 0.
- The renderer then makes the rectangle larger with
  `AdjustWindowRect(WS_OVERLAPPEDWINDOW)` (0xcf0000). This adds a caption
  and a sizing frame. But the window of the engine is `WS_BORDER` only,
  with no title bar. The client shell changes the style only for full
  screen (`HookWindow`).

Thus the client area is larger than the mode by the caption and the frame.
The game stretches the picture onto the client area. The clicks in the
menus go to the wrong position, by the same difference. You cannot move
the window.

Fixed in the modern build: the client shell places the window after each
start of the renderer. The window gets a title bar and the client area of
the mode, centred in the work area. Or, in borderless mode, the window
covers the desktop. src/modern/cshell/m9_window.h; MODERN.md M-9. (The
renderers do not change. The client shell puts the window in the correct
position before the game draws anything.)

### Alt+Tab freezes the picture

lithtech.exe of both games, AvP2 1.0.9.6 and Primal Hunt (`clu_WndProc`,
`WM_ACTIVATEAPP`, src/engine/lithtech/clientwin.cpp; Primal Hunt
lithtech.exe 0x004034b0).

When the game loses the focus, it stops the renderer (`r_TermRender`). It
also holds each sound that plays (the release of the sound manager,
`sub_481e70`: with a 3D provider, `PauseSounds`). When the game gets the
focus again, it starts the renderer again and releases the sounds.

Under Wine (9.0, wined3d), the renderer and the sounds do not come back:

- The renderer starts again but it draws nothing. With the retail
  binaries in a window, the window is black.
- The held sounds never play to their end. Thus a thing that waits for a
  sound waits for ever. An example is the line of dialogue in the opening
  cinematic.

The game continues to run behind the picture (it saves, and it accepts
keys). But the picture stays on its last frame. (Single player pauses
during this time, as intended.)

Fixed in the modern build for a window and a borderless window: the
renderer stays on and the sounds are not held when the focus changes
(clientwin.cpp); MODERN.md M-21. Full screen continues to work as in the
retail game.

### Above 100 frames a second the world runs fast

lithtech.exe and server.dll of both games, AvP2 1.0.9.6 and Primal Hunt
(`CServerMgr::Update`, src/engine/server/smgr.cpp; Primal Hunt
lithtech.exe 0x0048b5d0).

The client calls the update of the server one time each frame. Each call
runs a game tick for the time since the last call. The code clamps this
time to 0.01..0.2 s. Thus a frame shorter than 10 ms still moves the world
10 ms forward.

Thus, above 100 frames a second, the game runs faster than real time, by
(frame rate / 100) times. This applies to all that the server runs: AI,
physics, projectiles and timers. We measured this with the retail AvP2
1.0.9.6 binaries. We compared `ShowGameTime` with the wall clock, in the
first Marine mission. The results:

- At 139 frames a second, the game ran at 1.44 times real time.
- At 237 frames a second, the game ran at 2.37 times real time.

At the time of the retail game, computers did not usually get to 100
frames a second. Today's computers easily do. (A dedicated server limits
its own loop. Thus the bug does not affect it.)

Fixed in the modern build: smgr.cpp, a tick runs only when 10 ms have
gone since the last tick; MODERN.md M-10.

### The mouse look turns further at a higher frame rate

lithtech.exe of both games (`input_ReadActions`, AvP2 0x0043f490, Primal
Hunt 0x00443260; src/engine/lithtech/clientinput.cpp). We read this from
the retail AvP2 image: one event for each read for the mouse at
0x0043f593, and the smoothing at 0x0043f747..0x0043f792. Our function is
not byte-identical yet.

One time each frame, the engine reads the buffered mouse data of
DirectInput, one event at a time. After each event, it adds the current
value of each object into the axis offsets. The game turns the view by
these offsets (`GetAxisOffsets`).

A mouse axis keeps its last motion until a tenth of `InputRate`
milliseconds of `GetTickCount` go without an event. The code smooths this
motion towards the mean of the last three motions, by `InputRate`
percent. The code sets the axis to zero only after it adds the axis in
that frame. Thus the code adds the motion of one event:

- in the frame of the event,
- again in the next frame,
- again for each other event that it reads between them (an event of the
  other axis, or of a button).

Thus the turn of the view for a count of mouse motion depends on the frame
rate. It also depends on how the motion is divided into events. It does
not depend only on the motion.

We measured this with `OldMouseLook 1` in the modern build (this is the
retail path, with no changes). We used the first Marine mission of AvP2,
sensitivity 7 (the default profile), and 200 counts of motion in moves of
20 counts. The results:

- 78-87 degrees at 20-40 frames a second.
- 109-114 degrees at 30-55 frames a second.
- 121-139 degrees at 60-140 frames a second.

The scale of the sensitivity itself is 49 degrees for 200 counts.

Fixed in the modern build: the mouse look reads the motion of the mouse
itself (raw input), not the offsets of the engine. Thus a count gives the
same turn at all frame rates (src/modern/cshell/m6_mouse.h); MODERN.md
M-6.

### Binding a joystick button in Customize Controls leaves its old command bound too

CShell.dll of both games (`CFolderCustomControls::SetCurrentSelection`,
AvP2 0x1000a820, Primal Hunt 0x1000b380, and `Bind`, AvP2 0x1000af70,
Primal Hunt 0x1000bad0; src/AVP2/ClientShellDLL/FolderCustomControls.cpp).
We read this from the retail images. The functions are not byte-identical
yet.

When you push a control for a command, the page first removes the control
from the command that had it (`UnBind`). It finds the control by its
code, `"##<code>"`. `Bind` keeps a key or a mouse button by its code. But
it keeps a joystick button by its name (`"Button 3"`). Thus the search
never finds a joystick button. When you bind a joystick button to a
second command, it stays bound to the first command too. When you push
the button, it does both commands.

Fixed in the modern build: FolderCustomControls.cpp, the search finds a
joystick button by its name; MODERN.md M-7.

### An unlisted Winsock error comes back as a number, not a string

lithtech.exe 0x004a22b0, server.dll 0x1004655c, AvP2 1.0.9.6 lithtech.exe
0x00498220 (`tcp_GetErrorText`, src/engine/server/tcpdriver.cpp).

The function finds the last Winsock error in its table of 47 entries and
returns the text. For a code that is not in the table, the function
continues past the end of the loop to a bare `ret`. Thus the caller gets
the error code itself in `%eax`, and uses it as a `char*`. A message that
the game prints from it can crash or show garbage.

Fixed in the modern build: tcpdriver.cpp, an unlisted code gives
"<unknown>".

### A world model whose cell grid fails to load leaks the model

lithtech.exe 0x0042b740, server.dll 0x1000b1a1, AvP2 1.0.9.6 lithtech.exe
0x00428ac0 and server.dll 0x1000af41 (`w_LoadWorldModel`,
src/engine/server/worldbsp.cpp).

When the read of a world model fails, the function usually deletes the
incomplete CWorldBsp and returns 2. But when the Load of the cell grid
fails, the function returns 2 immediately (AvP2 server.dll 0x1000bcb4
jumps past the delete to 0x1000bdd2). Thus the model and all its arrays
leak. Only a corrupt world file causes this failure.

Fixed in the modern build: worldbsp.cpp, this failure also deletes the
model.

### The disconnect notice sends an uninitialised byte

lithtech.exe 0x004a1c40, server.dll 0x10046175, AvP2 1.0.9.6 lithtech.exe
0x00497c10 and server.dll 0x100416e1 (`TCPDriver::SendDisconnect`,
src/engine/server/tcpdriver.cpp).

The notice has three bytes. The code sets the first two bytes to zero.
The third byte is the value that was in the stack slot before: `push %ecx`
reserves the slot, and nothing writes to it. The code sends the notice
four times. This is harmless unless the receiver reads the third byte.

Fixed in the modern build: tcpdriver.cpp, the third byte is set to zero.

### The 3D voice count can come from an uninitialised value

lithtech.exe 0x0049c60d, AvP2 1.0.9.6 lithtech.exe 0x00492b7d
(`CSoundMgrView::Init`, src/engine/lithtech/clientsoundmgr.cpp).

When the game opens a 3D provider, the engine asks the provider for its
"Maximum supported samples" attribute. The engine gives the address of a
local variable that it does not initialise. Then it uses the value as
the number of 3D voices (clamped to 255 and to the game's request). The
real Miles providers write the attribute, thus the bug does not show
with them. A provider that does not write the attribute (older OpenMiles
versions) gives a random voice count.

Fixed in the modern build: clientsoundmgr.cpp, the value starts at zero;
MODERN.md M-B109. OpenMiles also writes the attribute now (MODERN.md
M-X7).

### Texture reference counts are wiped on every texture sweep (probable)

lithtech.exe 0x00428940, AvP2 1.0.9.6 lithtech.exe 0x004261a0
(`tm_UnmarkTextures`, src/engine/lithtech/clienttexmgr.cpp).

The unmark pass clears the full 16-bit word of SharedTexture at +0x32
(`movw $0x0`). This word holds the reference count (15 bits) and the mark
bit (bit 15). `tm_MarkUsedTextures` then keeps the textures that have a
count above zero (`test $0x7fff`). But the pass has just set all the
counts to zero. Thus the next sweep can free a texture that only this
count refers to.

We did not see this occur in the game yet. The bytes alone do not show
if it is intentional. We reproduced it: the retail build clears the mark
and the count in two statements, and VC6 merges them into the one word
store.

The count is a real reference count. It is not a count for each sweep.
These are the only functions that write it:

- `LTTexMod::GetTextureHandle` adds 1.
- `ReleaseTextureHandle` removes 1. At zero, it frees the texture data
  (lttexmod.cpp).

The functions that read the count take zero to mean "no handle"
(`tm_IsValidTextureHandle`). Thus, after a sweep, the engine refuses a
texture-mod handle that a game still holds. It can also free the texture
of that handle while the game uses it. The DLLs of the two games do not
ask for such a handle. Thus the bug does not show in AvP2 or Primal Hunt.
The DLLs of a mod can show it.

Fixed in the modern build: clienttexmgr.cpp, the sweep clears only the
mark; MODERN.md M-B01.

### A command line of more than 150 arguments overruns the argument table

lithtech.exe 0x00403d80 (`clu_dsi_BuildCommandLine`,
src/engine/lithtech/clientwin.cpp).

The code checks the argument count after the increment, with
`cmp $0x96; jg`. Thus it copies the 151st argument into `szArgs[150]`.
This is one past the end of the array in the frame. This occurs for the
real command line and for a `-cmdfile`. Also, if the read of a `-cmdfile`
fails, the code leaks the buffer that it allocated for the file.

Fixed in the modern build: clientwin.cpp, both loops stop when the table
holds 150 arguments. Each loop tests the limit at its top. This also
covers the quoted argument of Primal Hunt, which the retail code stores
without the test. The failed read frees its buffer; MODERN.md M-B02.

### A socket naming the node one past the end is accepted

lithtech.exe 0x0045eb20, server.dll 0x10024cb0 (`GetSocketTransform`,
src/engine/server/commonmodel.cpp).

The code tests the node index of the socket against the node count with
`>`, not with `>=` (0x0045ebd8, server.dll 0x10024d43). Thus, for a socket
on node N, where N is the count, the code reads one matrix past the cache
of the object.

Fixed in the modern build: commonmodel.cpp, `>=`.

### GetNextWMPoly crashes on a world model without a BSP

lithtech.exe 0x00486920 (`si_GetNextWMPoly`, src/engine/server/smgr.cpp).

The code calls `IsBusy` of the BSP before it tests the BSP pointer
(0x004869a5). Thus a world model with no BSP crashes. It does not return
LT_INVALIDDATA.

Fixed in the modern build: smgr.cpp, the code tests the pointer first.

### Four model-interface errors report the wrong function

lithtech.exe 0x0045f100/0x0045f180 (`GetTimeScale`/`SetTimeScale`) report
under "GetAllowTransition"/"SetAllowTransition". 0x0045fb90/0x0045fbe0
(`GetNormalize`/`SetNormalize`) report under "GetLooping"/"SetLooping".
The strings are at 0x004e5b54..58 and 0x004e5b9c..a0. A programmer copied
the error blocks and did not change the names. Only the debug output is
wrong.

Not fixed: it is harmless, and we keep it for byte identity.

### childmodel.map is never closed

server.dll 0x10041166 (`sm_LoadChildModelMap`, Primal Hunt only,
src/engine/server/smgr.cpp; the copy in lithtech.exe is at 0x0048ca10).

The code opens the file and reads it, line by line, into the child-model
table. The loop ends (server.dll 0x100417ff) with no `fclose`. Thus one
handle leaks each time a world starts.

Fixed in the modern build: smgr.cpp, the code closes the file.

### Removing an attachment reads its record after freeing it

`si_RemoveAttachment` (Primal Hunt lithtech.exe 0x00488270, and the copy
in server.dll; src/engine/server/smgr.cpp).

The code reads the object id of the child from the attachment record
after `am_RemoveAttachment` gives the record back to its bank. It works
only because the free of the bank writes nothing but the first dword of
the record.

Fixed in the modern build: smgr.cpp, the code reads the id first.

### Setting a model's animation time on the server is not sent to clients

server.dll 0x1003a130, lithtech.exe 0x00481b20, AvP2 1.0.9.6 lithtech.exe
0x0047abd0 and server.dll 0x10037173 (`ServerModelLT::SetCurAnimTime`,
src/engine/server/servermodel.cpp).

In AvP2, `ILTModel::SetCurAnimTime` got a third argument,
`bIncorporateScale`. The override of the server kept the two-argument
signature of NOLF. Thus it overrides nothing. It goes into a slot of its
own after the interface: slot 37 of the vtable 0x100757b8 of server.dll.
Slot 33 is still `CommonModelLT::SetCurAnimTime` (0x100252b9).

Thus a game that calls the interface gets the shared accessor. The
accessor sets the time. But it does not walk the string keys again, and
it does not flag the object (change flag 0x2000). Thus the clients do not
get the change. (The object DLLs call this function.
`CharacterAnimation.cpp` sets the first animation time of a character to
a random value. It also keeps the time when the character changes its
animation.)

Fixed in the modern build: servermodel.h/.cpp, the override takes the
three arguments of the interface and passes `bIncorporateScale` on. Thus
it is in slot 33 of the interface; MODERN.md M-B03.

### A long wildcard in the CRC string list overruns two buffers

lithtech.exe 0x0048bac0, server.dll 0x10040836
(`CServerMgr::CreateStringCRC`, src/engine/server/smgr.cpp).

The code copies the prefix and the suffix of a wildcard pattern into
0x80-byte buffers with no length check. The tokens can have up to 0x3ff
characters.

Fixed in the modern build: smgr.cpp, the code skips a pattern that does
not fit.

### A world name of 255 characters or more is left unterminated

lithtech.exe 0x0048bd70, server.dll 0x10040a44
(`CServerMgr::CreateWorldCRC`, src/engine/server/smgr.cpp).

The code copies the name with `strncpy`, limited to 0xff, into a 0x100-byte
buffer. The code never clears the buffer. Thus a name of that length has
no terminator.

Fixed in the modern build: smgr.cpp, the last byte is set to zero;
MODERN.md M-B04.

### The packet trace reads past a truncated flag record

server.dll 0x1002b584 (`net_LogPacket`, only the server of Primal Hunt,
src/engine/server/netmgr.cpp).

The FLG arm checks for eight bytes and reads ten bytes
(0x1002bae2..0x1002bb1e). Thus, for a truncated record, the code reads two
bytes past its end. This affects only the debug trace.

Not fixed: it affects only the debug output.

### A segment hitting a box's positive face reports the wrong plane

The box test of the unoptimised segment-query module (lithtech.exe
0x0043bdf3/0x0043bf15 and its twin in server.dll; also in AvP2 1.0.9.6;
src/engine/server/worldisect.cpp).

For the three positive faces, the code takes the distance of the hit
plane from the minimum of the box (`min.coord * +1`). It must take it from
the maximum. Thus the intersection gives a plane on the far side of the
box.

Fixed in the modern build: worldisect.cpp, the maximum.

### DirectMusic notifications leak on a segment that is not the newest

`dm_NotificationThread` (lithtech.exe 0x0044b550, AvP2 1.0.9.6
0x004477c0; src/engine/lithtech/clientmusic.cpp).

A segment-almost-ended notification can come for one of these:

- a secondary segment,
- a motif,
- a primary segment that is not the newest.

For these, the code leaves the loop with `continue` and never frees the
notification message. Also, for a queue entry of an unknown type, the
walk never goes to the next entry.

Fixed in the modern build: clientmusic.cpp, the code frees the message.
(The walk for an unknown type does not change yet.)

### A second peer-authentication message uses a freed client

lithtech.exe 0x00411000 (the peer-auth handler of Primal Hunt, message 13;
AvP2 1.0.9.6 0x004101c0; src/engine/lithtech/clientmgr3.cpp).

The handler deletes `g_pPeerAuthClient` and never clears it. Thus a
second message 13 calls `HandleRecvMsg` on freed memory.

Fixed in the modern build: clientmgr3.cpp, a null test, and the code
clears the pointer after the delete.

### A trailing +command reads past the argument list

lithtech.exe 0x00413760 (`cm_RunPlusCommands`, AvP2 1.0.9.6 0x00412110;
src/engine/lithtech/clientmgr2.cpp).

When the function gets an argument list, the `+command` loop runs up to
and including the count itself. The form for the command line of the
process stops one before the count. Thus a `+name` at the end reads one
entry past the end.

Fixed in the modern build: clientmgr2.cpp, the limit.

### The model ray test's level of detail reads through a null pointer

The ray-vs-triangle worker of `IntersectRays` (CalcLOD: lithtech.exe
0x0045ff50.., server.dll 0x10025b5c..; src/engine/server/commonmodel.cpp).

The lookup of the level-of-detail distance reads through a null pointer
when the level is past the end of the distance array.

Not fixed: it is not a bug. The null read is in the code (`xor %eax,%eax;
fadds (%eax)` at lithtech.exe 0x0045ff7b, and at 0x0045ffbf and
0x0045fffc; the same in server.dll). But no level gets to it. The inline
lookup gives this result for a level past `n` (the count of the array).
But each level that the code gives to the lookup is in 0..n:

- The code clamps the two offsets to 0..n first (`cmp; jle` at 0x0045ff4a
  and 0x0045ff94). For the negative offset, it uses `neg; jns`.
- The loop runs i = 0..n (`lea 0x1(%eax)`, `jb`).

The compiler kept the code for a branch that cannot occur.

### A world model pushing a no-slide object never corrects it

`wm_SeparateFromBox` (Primal Hunt server.dll 0x1002915d, lithtech.exe
0x004646c0; AvP2 1.0.9.6 lithtech.exe 0x0045f640, byte-identical, and
server.dll 0x100275bc; src/engine/server/commonphysics.cpp).

The code tests `-|vSep| > 0`, which is never true. Thus an object that
must not slide never gets a separating correction.

Fixed in the modern build: commonphysics.cpp, `fMag < 0.0f`.

The next three bugs are in `Model::LoadChildModels` (Primal Hunt
server.dll 0x100225d0, lithtech.exe 0x0045b470; AvP2 1.0.9.6 server.dll
0x100213fc; src/engine/server/commonmodel.cpp). Our function is not
byte-identical to the retail function yet:

- In server.dll, the order of the frame slots and one register choice are
  different.
- In lithtech.exe, the helpers that the compiler expands are different.

Thus each entry gives the retail instructions that we read the bug from.

### A model whose header will not read loads as a success

The reader returns a step number, not an `LTRESULT`. Each failure stores
its step in a byte, and the shared exit returns it. The first step, the
"Header" chunk, is step 0 (server.dll 0x10022639, `and %al,0xb(%ebp)`
with `%al` zero; lithtech.exe 0x0045b526; AvP2 1.0.9.6 0x1002145b). Thus,
when the header of a model file is missing or has an unknown version, the
reader releases the model and then returns `LT_OK`.
`se_LoadModelData` takes the zero as a success and returns the empty
model. (The other steps, 1 to 29, come back as the `LTRESULT` that has
that number. This only makes the "Error %d loading model" message
confusing.)

Fixed in the modern build: commonmodel.cpp, step 0 returns `LT_ERROR`.

### A model table that cannot be allocated loads as a success

This is the same reader. When it cannot size the table at `Model+0x8c`
(server.dll 0x100226ab, lithtech.exe 0x0045b5f7; AvP2 1.0.9.6
0x100214c7), it jumps directly to the success return. It does not read
the pieces, nodes, child models or animations of the model. It also does
not do the release that each other failure does.

Fixed in the modern build: commonmodel.cpp, it fails as the other
failures do.

### The child-model records leak when a model fails to load

This is the same reader. It reads the record of each child into a `new`
object (server.dll 0x1002292c). When the children are loaded, it deletes
all the records (0x10022c7e..0x10022cb2). These are the problems:

- Each failure between these points goes through the unwind at
  0x10022cb8. The unwind frees the vectors, but not the records in them.
- When the read of a record fails (0x10022960), the record is not in the
  vector yet.
- The code does not check the `new` for null (0x1002294e stores through
  it).

Only a malformed or truncated model file gets to these paths.

Fixed in the modern build: commonmodel.cpp, the failure path deletes the
records.

### Linking two objects when the link bank cannot grow writes through null

`sm_CreateInterObjectLink` (Primal Hunt lithtech.exe 0x00447f70,
server.dll 0x10018f02; src/engine/server/interlink.cpp).

The link record comes from `sb_Allocate` on the bank of the manager.
`sb_Allocate` returns null when the bank cannot grow (lithtech.exe
0x00447fd2 jumps past the free-list pop on null). The code stores the type
and the two ends through the record with no check. It also does not check
the node allocations after the record. Thus, when there is no more
memory, `CreateInterObjectLink` or `GetSmartLinkBody` crashes. It must
fail with `LT_ERROR`.

Fixed in the modern build: interlink.cpp, the code checks each of the
three allocations. When one fails, the code gives back what it took and
returns `LT_ERROR`; MODERN.md M-B05.

### A command-line argument of 200 characters or more is left unterminated

Launch.dll 0x10001820 (`GetLithTechCommandLine`, src/launch/launch.cpp).
AvP2 and Primal Hunt use this DLL from NOLF with no changes.

The function copies each argument that it gives to the engine with
`strncpy(pArgs[n], s, nMaxArgLen)`. These arguments are:

- the arguments of the real command line,
- the rez files,
- each word of the saved commands of the launcher.

lithtech.exe gives slots of 200 bytes and `nMaxArgLen` 200
(src/engine/lithtech/clientwin.cpp). Thus a string of 200 characters or
more is copied without its terminator. The engine then reads past it into
the next slot.

Fixed in the modern build: launch.cpp, an argument that is too long for
its slot is cut to 199 characters and terminated; MODERN.md M-B100.

### Every rez-file check leaks a find handle

Launch.dll 0x100025c0 and 0x10002640 (`ExistRezFile`, `ExistRezDir`,
src/launch/RezFind.cpp).

Each function asks `_findfirst` about the path. It never gives the handle
back with `_findclose`. `FindRezFiles` does up to five of these checks
each time the engine asks for its command line. In practice this is
harmless, because it occurs one time for each start. But it is a leak.

Fixed in the modern build: RezFind.cpp of Launch.dll, each function gives
the handle back (`_findclose`) when the check has its result; MODERN.md
M-B101. The launchers have their own `ExistRezFile` and `ExistRezDir`
(src/launcher/RezFind.cpp). These are the same code and get the same fix.
But they are in the pages that the protection wrapper keeps encrypted.
Thus we do not know the bug from their bytes.

### Starting a CD play list while a track plays can skip its first track

cdaudio.dll 0x10001390 (the window procedure of the driver;
src/engine/cdaudio/cdaudio.cpp).

The driver takes each `MM_MCINOTIFY` to mean "the track finished". It
does not check wParam. Thus the notification for a play that was
replaced or stopped also moves the list forward. When you start a play
list while a track plays, the list can skip to its second track.

Fixed in the modern build: cdaudio.cpp, only `MCI_NOTIFY_SUCCESSFUL` (the
track played to its end) moves the list forward; MODERN.md M-B06.

### The CD driver's GetSongFromList always answers nothing

cdaudio.dll 0x10001600 (`GetSongFromList`, src/engine/cdaudio/cdaudio.cpp)
returns NULL for all requests.

Fixed in the modern build: cdaudio.cpp, it returns the song of the list
(its track number) at that index, counted from 0. Past the end, it
returns NULL; MODERN.md M-B07.

### Removing the playing CD list leaves a dangling pointer

cdaudio.dll 0x10001610 (`RemoveList`, src/engine/cdaudio/cdaudio.cpp).

The function does not clear the pointer to the current list. Thus, when
you remove the list that plays, a pointer stays. `OnMCINotify` and
`Resume` read this pointer later.

Fixed in the modern build: cdaudio.cpp, when you remove the current list,
the code clears the pointer. The track that plays continues to play. At
its end, no track follows, as after the last track of a list; MODERN.md
M-B08.

### The launcher's Low and High detail buttons give the opposite profile

AVP2.exe 0x00406a40, PrimalHunt.exe 0x00406d70 (`CLauncherDlg::LaunchGame`,
src/launcher/LauncherDlg.cpp).

The detail dialog stores 0 for Low, 1 for Medium and 2 for High (the
handlers of `CDetailSettingsDlg`, AVP2.exe 0x00402170, 0x00402190,
0x00402150). `LaunchGame` changes 0 into
` ++PerformanceConfig .DefaultHigh` and 2 into `.DefaultLow`. Thus, on the
first run (or after "restore defaults"), a player who selects Low detail
starts with the high-detail profile. A player who selects High starts
with the low-detail profile.

Fixed in the modern build: LauncherDlg.cpp, Low gives `.DefaultLow` and
High gives `.DefaultHigh`; MODERN.md M-B102.

### The launcher writes the command line as a format string

AVP2.exe 0x00406a40, PrimalHunt.exe 0x00406d70 (`CLauncherDlg::LaunchGame`,
src/launcher/LauncherDlg.cpp).

The launcher writes the command line for lithtech.exe to avp2cmds.txt
with `fprintf(pFile, sString)`. Thus the command line itself is the
format. The command line can contain a `%`, from these sources:

- the commands of the user in the options dialog,
- the update and content command lines in the registry.

The function takes the `%` as a conversion. This makes the file
incorrect, or reads arguments that the caller did not give. The code must
use `fputs` (or a `"%s"` format).

Fixed in the modern build: LauncherDlg.cpp, the launcher writes the
command line with `fputs`; MODERN.md M-B103.

### The launcher copies its command line into 1024 bytes of stack

AVP2.exe 0x00406a40, PrimalHunt.exe 0x00406d70 (`CLauncherDlg::LaunchGame`,
src/launcher/LauncherDlg.cpp).

The launcher builds the command line in a `CString`. Before it writes
avp2cmds.txt, it copies the command line with `strcpy` into
`char sString[1024]` on the stack. It does not examine the length. The
command line holds these items:

- the command line of the launcher itself (`m_lpCmdLine`, as long as
  Windows permits),
- the saved commands,
- the update and content command lines of the registry (up to 259
  characters each),
- the rez files and the options.

Thus a sufficiently long command line or set of commands overruns the
stack frame.

Fixed in the modern build: LauncherDlg.cpp, the launcher writes the file
directly from the `CString`, with no copy; MODERN.md M-B107.

### The display dialog's close button does nothing

AVP2.exe, PrimalHunt.exe (`CDisplayDlg`, src/launcher/DisplayDlg.cpp).

The message map of the dialog (AVP2.exe 0x00422a00) has entries for OK
(1035) and Cancel (1041). It has no entry for the close button (1034).
Thus the dialog ignores a click on the "x" in the corner of the display
dialog. The launcher of NOLF2 is the same code, one year later, and it
adds `OnClose`.

Fixed in the modern build: DisplayDlg.cpp, the close button goes to
`OnCancel`, as the close button of the main dialog does
(`CLauncherDlg::OnClose` cancels); MODERN.md M-B104.

### A launcher string that will not load hides the message it was for

AVP2.exe 0x00405970, PrimalHunt.exe 0x00405cc0 (`CLauncherApp::MessageBox`
by string ids, src/launcher/Launcher.cpp).

When the string of the caption does not load, the code formats the error
into the `CString` of the text, not into the `CString` of the caption.
Thus the box that follows shows the caption error as its message, with an
empty caption. The message itself is lost.

Fixed in the modern build: Launcher.cpp, the error of the caption goes
into the caption, as the error of the text goes into the text. Thus the
box keeps its message; MODERN.md M-B105.

### The launcher's already-running checks ignore the directory they build

AVP2.exe 0x00404f50, PrimalHunt.exe 0x004051a0 (`CLauncherApp::InitInstance`,
src/launcher/Launcher.cpp).

The function gets the current directory and adds a backslash. Then it
looks for a running `LITHTECH.EXE` and `SETUP.EXE` by their bare names
(`sprintf(sExe, "LITHTECH.EXE")`). It never uses the directory. Thus any
process with one of these names stops the launcher. This includes a
process of a different LithTech game.

Fixed in the modern build: Launcher.cpp, the two checks look for the
process by the full path of the directory. Thus only the `lithtech.exe`
or `setup.exe` of this directory stops the launcher; MODERN.md M-B106.

### A dynamic lightmap is cleared only a quarter of the way (probable)

d3d.ren 0x10033512 and 0x10033652 (the 16-bit and 32-bit dynamic-light
fills, src/engine/d3d/ph/lmdynamic.cpp).

Each fill clears the lightmap with
`memset(pData, 0, (m_Pitch * height) >> 2)`. The pitch is in bytes. Thus
this clears a quarter of the buffer. The fill then writes only the pixels
in the sphere of the light. Thus, below the cleared quarter, the texels
out of the sphere keep the old contents of the buffer.

This is probable: it is true only if `m_Pitch` is in bytes, as it is at
all other places where the renderer uses it. (It is in bytes:
`LMDrawDesc::Begin` takes it from the `lPitch` of the lock, and the two
fills divide it by the pixel size for their row step.)

`Begin` has already copied the lightmap of the polygon into the buffer.
Where there is no lightmap, it fills the buffer black. Thus the bug shows
only on a polygon with a lightmap page: the texels of the static lightmap
are added below the quarter of the light.

Fixed in the modern build: lmdynamic.cpp, the two fills clear all of
`m_Pitch * height`; MODERN.md M-B20.

### Unsorted translucent world models lose their blend states before drawing

d3d.ren 0x100318b0 (`d3d_DrawNoSortWorldModel`,
src/engine/d3d/ph/drawworldmodel.cpp).

The renderer puts last in the queue the world models whose first surface
has a "not sorted" mark. It draws them with this function. The function
selects additive blend states in `StateSet`s in the scope of its
`if`/`else` arms:

- SRCALPHA/ONE for `FLAG2_ADDITIVE` objects,
- ONE/ONE with a black fog colour for the other objects.

The states are restored when the arms end. This occurs before the one
draw call after the arms. Thus the renderer draws these world models with
the blend that was set before (the usual translucent blend), not
additively. Only Primal Hunt has this bug. The renderer of AvP2 holds its
one set of states during the draw.

Fixed in the modern build: drawworldmodel.cpp, each arm makes the draw,
with its states; MODERN.md M-B21.

### The renderer's box test builds three of a box's six faces at the wrong place

d3d.ren 0x10031fc5 (`i_BoundingBoxTest`,
src/engine/d3d/ph/fullintersectline.cpp).

The segment-against-box test takes the plane distance of each face from
the minimum corner of the box. Thus the planes of the +X, +Y and +Z faces
are wrong. For a segment through one of these faces, the intersection
point and the plane are incorrect. The engine of NOLF2 still has the same
code. (The point is correct, because it is the coordinate of the face
itself. The distance of the plane is wrong: it is too small by the width
of the box along its normal.)

Fixed in the modern build: fullintersectline.cpp, the code takes the
plane distance of each face from the coordinate of that face; MODERN.md
M-B22.

### The model sun's variance reads the warble scale

d3d.ren 0x1000b67d (`d3d_InitModelSun`, src/engine/d3d/ph/drawmodel.cpp;
the same in AvP2).

The renderer lights the models from a table of sun intensities. The table
spreads its sine by a clamped console value. The value that it reads is
0x1005d8b4. The `WarbleScale` variable (common_stuff.cpp, default 0.92)
writes this value. `ModelSunVariance` (from this file, default 0.05) does
not. Thus the flicker of the sun follows the world-warble setting. It is
identical in the two renderers.

Not fixed: it is not a bug. The table (`g_fModelSun`, our name, for the
sun) is the `ModelWarble` curve:

- Only `d3d_WarbleTransform` reads it. It scales each vertex between
  WarbleScale and 1.
- RenderScene builds it again when WarbleScale changes ("Rebuilding
  warble table").
- `ModelSunVariance` is the step of the sun-visibility search, and that
  search reads it. If the table used `ModelSunVariance`, the table would
  make a model 0.05 of its size.

We corrected the comment in the source.

### A lightmap page leaks its allocation bitmap when there is no lightmap format

d3d.ren 0x10034f32 (the page allocator of the lightmap pager,
src/engine/d3d/ph/lmpaging.cpp).

When the device has no lightmap texture format, the code frees the new
page but not its allocation bitmap. The failure path for the surface
creation, immediately below, frees the two.

Fixed in the modern build: lmpaging.cpp, the code also frees the bitmap;
MODERN.md M-B23.

### A dynamic-lightmap texture whose surface cannot be made locks a null surface

d3d.ren 0x1001ddea (`CD3DLMTexture::Init`,
src/engine/d3d/ph/d3d_optimizedsurface.cpp).

When `CreateSurface` of DirectDraw fails, Init continues. It locks the
surface that it did not get, and calls through a null pointer.

Fixed in the modern build: d3d_optimizedsurface.cpp, Init fails at that
point (its caller already tests the result); MODERN.md M-B24.

### The models' specular pass sets its texture-stage states and undoes them at once

d3d.ren 0x100254d8 (`CD3DModelDraw::DrawPasses`,
src/engine/d3d/ph/drawmodel_env.cpp).

The specular pass writes its stage-1 states as unnamed `StageStateSet`
temporaries. These states are colour op ADD, texture and current
arguments, and clamp addressing. Each temporary restores its state
immediately after the code makes it. Thus the pass draws with stage 1 as
it was before. These must be named objects that stay until the draw is
complete.

Stage 1's colour op is DISABLE when the renderer draws the solid models
(`d3d_DrawSolidModels`). Thus the specular texture is bound but never
blended, and no model shows its specular highlight. Only a model whose
command string has `SpecularEnable` uses this pass. No shipped model of
the two games has it.

Fixed in the modern build: drawmodel_env.cpp, named objects, as the bump
pass above it has; MODERN.md M-B25.

### A screen-space lightmap polygon is copied without checking its vertex count

d3d.ren 0x10036282 (`d3d_DrawScreenLMPoly`,
src/engine/d3d/ph/drawlightmaps.cpp).

The code copies the vertices of the polygon into a stack buffer of 64
entries. It does not test the count. The world-space lightmap draw near
it (0x10035e34) tests its count. A polygon of more than 64 vertices
overruns the stack.

The lightmap-only draw (0x100361d9, in the same file) also copies into a
buffer of 64 entries with no check. Our function for it is near, not
identical (the stores of the loop are scheduled differently). Thus that
part comes from our reading of the function.

Fixed in the modern build: drawlightmaps.cpp, the two functions test the
count and fail as 0x10035e34 does ("Error: vertex buffer overflow");
MODERN.md M-B26.

### SVAR OldCloak writes through a null message if one cannot be made

PHObject.lto 0x10051a20 (`CCommandMgr::ProcessSVar`,
src/AVP2/ObjectDLL/CommandMgr.cpp).

When the `SVAR` object command sets the `OldCloak` server variable, the
code sends its value to the clients. The message pointer starts as null,
and the code does not check the result of `CreateMessage`. Thus, if the
code cannot make a message, it writes the float through a null pointer.
It also sends and releases the message through the null pointer. The code
must stop when `CreateMessage` fails.

Fixed in the modern build: CommandMgr.cpp, the code sends nothing when it
cannot make a message; MODERN.md M-B70.

### The player-view fx cache writes and reads one type, past the end of each list

PHCShell.dll 0x100cb300 and 0x100cb3c0 (`PVFX::Write`, `PVFX::Read`,
src/AVP2/Shared/FXButeMgr.cpp).

The bute cache of Primal Hunt writes the scale, dynamic light and sound
fx types of each player-view fx. Each of the three loops has a stray
semicolon (`for (...);`). Thus each loop runs empty. Then the code writes
the one entry at the index of the count. This entry is one past the last
type. When the list is full, it is also past the end of the array of ten
entries. The read does the same. Thus a PVFX that the code loads from the
cache keeps only that one entry, which is garbage.

Fixed in the modern build: FXButeMgr.cpp, the loops have no semicolons.
When the count that the code reads back is larger than its list, the code
takes it as no entries; MODERN.md M-B71. (The code never writes or reads
a cache. Each manager first calls `WriteCache`/`ReadCache` of
`CGameButeMgr`, and these return false.)

### A scale fx loaded from the bute cache has an undefined additive-alpha flag

PHCShell.dll 0x10108670, 0x10108780, 0x10108960 (`CScaleFX::CScaleFX`,
`CScaleFX::Write`, `CScaleFX::Read`, src/AVP2/Shared/FXStructs.cpp).

Primal Hunt added `bAdditiveAlpha` ("AdditiveAlpha") to the attributes of
the scale fx. `Init` reads it. But the constructor does not clear it, and
the `Write`/`Read` of the cache do not include it. Thus, for a scale fx
from the cache, the additive alpha depends on the old contents of the
allocation.

Fixed in the modern build: FXStructs.cpp, the constructor clears the
flag, and the cache writes and reads it after the other values; MODERN.md
M-B74.

### A cached projectile fx with class data of kind 0 crashes on load

PHCShell.dll 0x100c5800 (`PROJECTILEFX::Read`,
src/AVP2/Shared/FXButeMgr.cpp).

When the code reads a projectile fx from the bute cache of Primal Hunt, a
flag byte tells if class data follows. A second byte gives the kind of
the class data. The switch makes nothing for kind 0, and a null pointer
for an unknown kind. Then it calls `Read` through the pointer in all
cases. Thus a cache with such a record dereferences null. (`Write` never
writes kind 0 for a real class. Thus only a damaged cache file can cause
this.)

Fixed in the modern build: FXButeMgr.cpp, for kind 0 the code makes no
class data and reads none. Kind 0 is written as its kind byte only. Thus
the reads of the file stay aligned; MODERN.md M-B72.

### A menu font draws every column of a column control left-justified

CShell.dll 0x10129d10 (`CLTGUIColumnTextCtrl::Render`,
src/lithshared/ltguimgr/ltguicolumntextctrl.cpp; LTGUIMgr, the prebuilt
`ltguimgr_LT2.lib` of the release).

With a menu font (three faces), the code draws the column through
`CLTGUIFont::DrawMenu`. It gives the width of the column where the
justification must go. `DrawMenu` takes all values above 2 as left. Thus
a right-justified or centre-justified column draws left-justified, from
its right-justify x.

Fixed in the modern build: ltguicolumntextctrl.cpp gives the
justification of the column.

### Setting an attribute that only an expansion tag holds does not take

CShell.dll 0x1012e240, PHCShell.dll 0x10150850
(`CButeMgr::GetSymTabItem`, src/lithshared/ButeMgr/butemgr.cpp; the
prebuilt `ButeMgrNoMFC.lib` of the release).

The ButeMgr of AvP2 keeps the tags whose names contain "EXPANSION" in a
second tag table. When the main table has the tag but not the attribute,
this occurs:

- `GetSymTabItem` (which each `Set*` uses) finds the TableOfItems of the
  second table. But it searches the TableOfItems of the main table again,
  two times. Thus it makes the attribute in the auxiliary table.
- `FindSymTabItem` (which each `Get*` uses) looks in the second table
  before the auxiliary table. Thus it continues to return the old value.

(The code cannot get to that branch as described. `Tag` refuses a tag
that is already in one of the tables. Thus no tag is in the two tables.
What occurs is the case that the branch was for. An expansion tag is only
in the second table. Thus `GetSymTabItem` takes it as a new tag, and
makes the attribute in the new-tag table, after the second table. `Get*`
still returns the old value.)

Fixed in the modern build: butemgr.cpp, when a tag is not in the main
table, `GetSymTabItem` takes it from the second table. Thus the code
finds and sets the attribute where `FindSymTabItem` reads it. A new
attribute of an expansion tag goes in the auxiliary table, as for all
other existing tags. It does not go in the new-tag table, because `Save`
writes that table as a second, duplicate section. MODERN.md M-B40.

### The attribute tables leak on every Term

CShell.dll 0x1012bb10, PHCShell.dll 0x1014e1d0, Object.lto 0x10202320,
PHObject.lto 0x10221e20 (`CButeMgr::Term`, src/lithshared/ButeMgr/
butemgr.cpp).

Term frees the items of each tag and clears the four tag tables. But it
never deletes the TableOfItems that each tag owns. The ButeMgr of NOLF2
deletes them.

Fixed in the modern build: butemgr.cpp, the traversal of Term deletes the
TableOfItems of each tag after its items (only Term uses the callback).
MODERN.md M-B41. (Object.lto and PHObject.lto link the same library. Thus
the fix also applies to the managers of the server.)

### GetString into a buffer leaves the buffer untouched when the symbol is missing

PHCShell.dll 0x101511b0 (`CButeMgr::GetString(tag, att, defVal, szResult,
maxLen)`, src/lithshared/ButeMgr/butemgr.cpp).

When the code does not find the attribute, it returns the default in a
CString and then discards it. The code copies only a found string into
`szResult`. Thus the buffer of the caller keeps its old contents, not
`defVal`.

Fixed in the modern build: butemgr.cpp copies `defVal`.

### DPRINTF=COM2 sends the debug output to COM1's setting

DPRINTF.OBJ in the prebuilt `lith.lib` of the release
(`dprintfinittype::dprintfinittype`, src/lithshared/lith/dprintf.cpp;
byte-identical to the prebuilt member).

When the code reads the DPRINTF environment variable, "COM2" selects
`Com1`. The lith of NOLF had `Com2` there. (It also had `LPT1` for
"LPT2", which the lith of AvP2 has correct.) The build of the release is
`_WINDOWS`. This build then forces `Monochrome`, whatever the code read,
and it does not compile the serial ports. Thus the bug shows only in a
console build or a DOS build.

Not fixed: no shipped image calls dprintf.

### A model with no model file crashes the server when its files are sent

server.dll 0x1003448c, AvP2 1.0.9.6 server.dll 0x10031aa9
(`sm_WriteObjectFiles`, src/engine/server/servercommon.cpp).

The writer sends each file identifier of an object. For a missing skin,
it sends 0xffff. But it reads the identifier of the model file through
its pointer with no null test. Thus an object with a missing model file
dereferences null. The three-identifier writer of NOLF (0x10029087) had
the same difference between the two cases.

Fixed in the modern build: servercommon.cpp, for a missing model file
the writer sends 0xffff, as for a missing skin.

### Inserting into a restriction list smears one entry and drops another

PHObject.lto 0x10032a9d (`CWeaponMgr::LoadWeaponButes`) and 0x1000bee5
(`CCharacterButeMgr::LoadButes`), the inlined `CMPRestrictList::Insert`
(src/AVP2/Shared/MPRestrictList.h).

A server can restrict multiplayer classes and weapons. The code keeps the
lists of these sorted by ID. To make space for an entry, the code moves
the entries up from the bottom (`entries[j + 1] = entries[j]` with j
counting up). These are the problems:

- This copies the entry at the insertion point over each entry above it.
- The code never increases the count. Thus the last entry is lost.

This affects only a bute file that lists the "_MP" sets or weapons out of
ID order.

The copies of the server also have the bug: PHObject.lto 0x10155bc0 and
0x10155cf0 (`MPRestrict_AddClass`, `MPRestrict_AddWeapon`,
src/AVP2/ObjectDLL/MPRestrictions.cpp). The code fills these from the
game info.

Fixed in the modern build: MPRestrictList.h, ServerOptions.h
(`CMPAllowList`, the copy of the client and of the dedicated server,
PHCShell.dll 0x100a88d0) and MPRestrictions.cpp. The code moves the
entries up from the top and increases the count; MODERN.md M-B75.

### A cached string longer than its buffer throws every later read off

PHObject.lto 0x10154d00 and 0x101550f0 (`CMultiFile_FILE::ReadString`,
`CMultiFile_LITH::ReadString`, src/AVP2/Shared/MultiFile.cpp).

The code cuts a string that is longer than the buffer to fit the buffer.
But it does not read the remaining part of the string. Thus each later
read from the bute cache comes from the wrong position.
CCharacterButeMgr never reads its cache. Its ReadCache (0x1000e580)
first calls the ReadCache of the base class, which always fails. We did
not yet examine if a different manager reads its cache.

Fixed in the modern build: MultiFile.cpp, the code skips the remaining
part of the string; MODERN.md M-B79. (No manager ever reads its cache:
refer to the entry for the player-view fx.)

### Losing a carried turret drops the player's last one too

PHObject.lto 0x10116ac0 (`CPlayerObj::RemoveTurret`, which
`TurretDestroyed` calls two times, 0x10116bb0;
src/AVP2/ObjectDLL/PlayerObj.cpp).

The code empties the slot of the turret and decreases the count before it
moves down the slots above it. The move stops at the new count minus one.
Thus the code never moves down the last turret, and the turret goes off
the list. When a player carries two turrets and the first is destroyed,
the player carries no turrets.

Fixed in the modern build: PlayerObj.cpp, each turret above the lost
turret moves down, and the code clears the empty slot; MODERN.md M-B80.

### A fading fog volume takes its blue for its red

PHObject.lto 0x1015b9e0 (`VolumetricFog::OnUpdate`,
src/AVP2/ObjectDLL/VolumetricFog.cpp).

While a FADEALPHA message fades the volume, the code sets its color to
(blue, green, blue), not to (red, green, blue). Thus a colored fog volume
changes color while it fades. Only a grey fog volume looks correct.

Fixed in the modern build: VolumetricFog.cpp, (red, green, blue);
MODERN.md M-B81.

### A tentacle's attack and burrow updates are swapped

PHObject.lto 0x10156790 (`CTentacle::OnUpdate`,
src/AVP2/ObjectDLL/Tentacle.cpp).

StartAttack (0x101575b0) sets state 4, and Burrow (0x101574b0) sets state
3. But the update switch runs the wrong update for each state:

- For state 4, it runs the burrow update. This update waits for the
  burrow timer, then hides.
- For state 3, it runs the attack update. This update waits for the
  attack timer, then burrows.

Thus an attack ends on its first update, because the burrow timer ended
a long time before. A burrow starts again on each update after the attack
timer ends.

Fixed in the modern build: Tentacle.cpp, each state runs its own update;
MODERN.md M-B82.

### A turret gun that can turn all the way round never wraps its yaw

PHObject.lto 0x1015a340 (`CTurretGun::Rotate`,
src/AVP2/ObjectDLL/TurretGun.cpp).

With a Yaw Range of a full turn, the code must wrap the yaw into one
turn. But the code discards the result of the ceil() that does this.
Thus the yaw increases with no limit.

Fixed in the modern build: TurretGun.cpp, the code removes the full turns
from the yaw (`fmod`); MODERN.md M-B83.

### A restriction list's "allowed" check reads one past its last entry

PHObject.lto 0x10155e20 and 0x10155e40 (`MPRestrict_IsClassAllowed`,
`MPRestrict_IsWeaponAllowed`, src/AVP2/ObjectDLL/MPRestrictions.cpp).

The code checks the index with `i > count`, not with `i >= count`. Thus
it reads the entry immediately after the list. This entry is always
clear, thus it reads as not allowed. With the indices of the callers
(from the Find functions), this is harmless. But it is wrong.

Fixed in the modern build: MPRestrictions.cpp, `i >= count`; MODERN.md
M-B76.

### Deleting a track key leaves a duplicate that is freed twice

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrack::DeleteKey`,
src/lithshared/lithtrackmgr/LithTrack.cpp).

The keys after the deleted key move down one slot. But the code never
clears the last slot. Thus two slots point to the same key, and `Term`
deletes it two times.

Fixed in the modern build: LithTrack.cpp, the code clears the last slot
that it moved from. MODERN.md M-B42.

### Deleting a track leaves a duplicate that is freed twice

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackList::Delete(int)`,
src/lithshared/lithtrackmgr/LithTrackList.cpp).

This is the same bug as in the key list. After the tracks move down, the
code never clears the last slot.

Fixed in the modern build: LithTrackList.cpp, the same fix. MODERN.md
M-B43.

### The track editor's "Font Type" command never changes the font

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackMgr::Ext_Type`,
src/lithshared/lithtrackmgr/LithTrackMgr.cpp).

The function puts the new type in its own pointer parameter, not in the
setting. Thus `TrackEdit Font Type <x>` builds the same font again.

Fixed in the modern build: LithTrackMgr.cpp, the new type goes in the
setting itself (`m_lfCS.szFontName`, the only setting that the code
gives). It comes from a copy that stays after the tokens of the command
are gone. MODERN.md M-B44.

### The track editor's help text ignores the bar's width

LithTrackMgrClient.lib, linked into CShell.dll
(`LithTrackBarHelp::SetupLines`, src/lithshared/lithtrackmgr/LithTrackBarHelp.cpp).

The five `CreateTextItem` calls give the width where the justification
must go. Thus the width is the default, 0. The code never wraps the help
to the bar. When the low byte of the width is 1 or 2, the lines are
centred or right-justified.

Fixed in the modern build: LithTrackBarHelp.cpp, the lines are
left-justified and wrapped to the width. MODERN.md M-B45.

### The track editor's draw object always reports failure

LithTrackMgrClient.lib, linked into CShell.dll (`LithTrackDrawObj::Init`,
src/lithshared/lithtrackmgr/LithTrackDrawObj.cpp).

The function returns `DFALSE`, also when it made its line system.

Fixed in the modern build: LithTrackDrawObj.cpp, the function returns
success when it made the line system. (Its one caller,
`LithTrackMgr::Init`, ignores the result. Thus nothing changes in the
game.) MODERN.md M-B46.

### A GameSpy secure query can overrun the server's buffer

GameSpyMgr.lib, linked into Object.lto and PHObject.lto
(`CGameSpyMgr::OnSecureQuery`, src/lithshared/gamespy/gamespymgr/GameSpyMgr.cpp).

The code copies the `secure` value from the network packet with `strcpy`
into a 64-byte buffer. It does no length check. A long value overruns the
stack.

Fixed in the modern build: GameSpyMgr.cpp (gamespymgr/), the code cuts
the value to the buffer. MODERN.md M-B47.

### The LAN server list searches an unterminated reply

GameSpyClientMgr.lib, linked into CShell.dll and PHCShell.dll
(`ServerListLANList`, src/lithshared/gamespy/gamespyclientmgr/gserverlist.c).

The code searches the reply buffer with `strstr`. It does not add a null
terminator first. (The query loop terminates its replies.) Thus the code
reads past the end of a full buffer.

Fixed in the modern build: gserverlist.c, the code terminates the reply
at its length. For this, `recvfrom` gets one byte less than the buffer.
MODERN.md M-B48.

### Sorting the server list by an unknown mode uses an uninitialised comparator

GameSpyClientMgr.lib (`ServerListSort`, gserverlist.c).

The switch on the compare mode has no default case. Thus, for an unknown
mode, the sort uses the old value of the comparator variable.

Fixed in the modern build: gserverlist.c, for an unknown mode, the list
stays unsorted. (The game gives only the four known modes.) MODERN.md
M-B49.

### The server browser never checks or closes sierraup.cfg

GameSpyClientMgr.lib (`CGameSpyClientMgr::SetupDirServers`,
src/lithshared/gamespy/gamespyclientmgr/GameSpyClientMgr.cpp).

The code uses the `fopen` result with no check. Thus a missing file
causes a crash. Also, the code never closes the file. The build of Primal
Hunt (PHCShell.dll 0x10138830, byte-identical) checks the open, but it
also never closes the file.

Fixed in the modern build: GameSpyClientMgr.cpp, AvP2 checks the open, as
Primal Hunt does, and the two close the file. MODERN.md M-B50.

### Refreshing LAN servers removes other games' servers by the wrong port

GameSpyClientMgr.lib (`CGameSpyClientMgr::RefreshLANServers`).

The code removes the servers of other games by their game port. But
`ServerListRemoveServer` finds them by their query port. When the code
does not find a server, the server stays in the list, but the count
still decreases. Thus the code does not check the servers after it.

Fixed in the modern build: GameSpyClientMgr.cpp, the code removes a
server by its query port (`ServerGetQueryPort`). The list finds the
server by this port. MODERN.md M-B51.

### Hosting an Internet game without SierraUp.cfg crashes the server

GameSpyMgr.lib, linked into Object.lto and PHObject.lto
(`InitSierraMultiplayer`, src/lithshared/gamespy/gamespymgr/GameSpyMgr.cpp).

A server that is not LAN-only registers with the directory of WON. It
reads the addresses of the directory from SierraUp.cfg. These are the
problems:

- The code never checks the `fopen` result (`fgets` on a null file).
- The code never closes the file.
- A `DirectoryServer` line without two quotes goes past the `strstr`
  results. A quoted address of 256 characters or more goes past the end
  of the address buffer.

Our game directories have no SierraUp.cfg. We tested this with the
retail AVP2Serv.exe, server.dll and game DLLs, and a config with
`LANConnection = 0`. The result was a page fault on a read of 0x34 when
the game starts. This is the same fault as in the browser (M-B50). When
the CD key is accepted, that fault also crashes the Multiplayer Internet
folder of the retail client.

Fixed in the modern build by M-18: the server does not register with WON
now (WON is gone). Thus the code does not read the file. MODERN.md M-18.

### The dedicated server prints the game's text as a format

AVP2Serv.exe and AVP2XServ.exe (`CServerDlg::UpdateUI`, `OnConsoleSend`,
src/AVP2/AVP2Serv/ServerDlg.cpp).

The code gives these texts to `WriteConsoleString` as its `printf`
format:

- each line that the game sends to the console of the server. This is
  the console output of the engine and of the game DLL, and each chat
  message, as "<< name >>  text".
- the command that the user types in the console.

When a player chats `%n%n%n%n`, or has that name, the server writes
through the values on the stack. `%s` reads through them.

Fixed in the modern build: ServerDlg.cpp, the text goes through `"%s"`.
MODERN.md M-20.

### A dedicated-server console line of 499 characters or more is not terminated

AVP2Serv.exe and AVP2XServ.exe (`CServerDlg::WriteConsoleString`).

The code formats the line with `_vsnprintf(str, 499, ...)` into 500
bytes. When the text fills the buffer, there is no terminator. The code
then adds `"\r\n"` with `strcat`. For a line of 498 characters or more,
this writes past the end of the buffer.

Fixed in the modern build: ServerDlg.cpp, the code cuts the line to 497
characters and terminates it. MODERN.md M-20.

### A tracer without SingleSegment takes whatever its memory held

PHObject.lto 0x1001a480 (`TRACERFX::Init`, src/AVP2/Shared/FXButeMgr.cpp).

The tracer of Primal Hunt gets SingleSegment, HeadVelocity, Flare and
FlareScale. But its constructor never sets `bSingleSegment`. Thus, when
the bute of a tracer has no SingleSegment attribute, the tracer keeps the
old value of the allocation.

Fixed in the modern build: FXButeMgr.cpp, the constructor clears the four
tracer members of Primal Hunt; MODERN.md M-B73.


### A tracer without a Flare names its flare sprites from uninitialised memory

PHCShell.dll 0x1012cc70 (`CTracerFX::Update`,
src/AVP2/ClientShellDLL/TracerFX.cpp; we read this from the retail image,
and the function is not byte-identical yet) and PHObject.lto 0x1001a480
(`TRACERFX::Init`, src/AVP2/Shared/FXButeMgr.cpp; byte-identical).

Each Primal Hunt tracer, single-segment or not, makes its three flares on
its first update. It does `_snprintf(name, 127, szFlare, i + 1)`
(0x1012d0b7), then `CreateFlare` (0x1012da40), which skips an empty name.
The constructor of `TRACERFX` does not set `szFlare`, thus it keeps the
old contents of the allocation. Init sets it only when the bute has a
Flare. Only EnergyFlechetteTracer has a Flare.

Thus the Standard and Laser tracers (TracerFX0 and 1 of FX.txt) format
their sprite names from old heap bytes. The result is one of these:

- nothing, when the first byte is a zero,
- a sprite load that fails,
- a worse result, if there is a `%s` in the bytes.

Fixed in the modern build: FXButeMgr.cpp, the constructor clears it (with
the members of M-B73); MODERN.md M-B85.
### The "nelno" cheat's message isn't terminated

PHCShell.dll 0x10076180 (`CheatMgr::Cheat_Nelno`, our name,
src/AVP2/ClientShellDLL/CheatMgr.cpp; byte-identical).

The scrambled "You need more training to defeat me!" is a local array of
37 bytes with no terminating zero. Thus `ScrambleString` (Scramble.cpp)
continues to unscramble into the three padding bytes and the bytes after
them on the stack. It stops when it finds a zero or when it fills the
buffer of 255 characters. Thus the message can end in garbage.

Fixed in the modern build: CheatMgr.cpp, the array ends in a zero.
MODERN.md M-B52.

### Naming the console's actions leaks a string per command

PHCShell.dll 0x1007b811 (`CGameClientShell::OnEngineInitialized`,
src/AVP2/ClientShellDLL/GameClientShell.cpp; near, one `jmp` short).

When Primal Hunt starts, it runs "AddAction <name> <id>" for each command.
It gets each name from `FormatString`, and it never frees the HSTRING
that it gets. Thus one string leaks for each command, each time the client
shell starts.

Fixed in the modern build: GameClientShell.cpp, the code frees the string
of each name after its command runs. MODERN.md M-B53.

### The multiplayer class check takes an index one past the list

PHCShell.dll 0x10005aa0, 0x10014260, 0x10086cb0 and 0x10097520. These are
`CBaseFolder::IsCharacterAllowed`, `CFolderHost::IsCharacterAllowed`, the
check of the client shell on the list of the character butes (not written
yet), and `MultiplayerClientMgr::IsClassAllowed` (our names). All are
byte-identical, but not the unwritten one
(src/AVP2/Shared/MPRestrictList.h `CMPRestrictList::IsAllowed`).

The limit test is `index > count`, not `>=`. Thus the code can read the
entry one past the last entry. As shipped, this is harmless. The index
always comes from a search of the same list, and the search gives a real
entry or -1.

Fixed in the modern build: MPRestrictList.h, `IsAllowed` tests
`i >= count`; MODERN.md M-B77.

### The cloaked weapon's alpha floor writes the red channel

PHCShell.dll 0x100b46c2 (`CWeaponModel::UpdateWeaponModel`,
src/AVP2/ClientShellDLL/WeaponModel.cpp).

Primal Hunt keeps the first-person weapon at least a little visible. When
bit 11 of the second flags of the model is set (part of the cloaked look,
0x1800), the floor is 0.35. If not, the floor is 0.2. The 0.35 branch
compares and stores the red of the colour (the first `GetObjectColor`
output), not its alpha. It also skips the 0.2 alpha floor. Thus the code
puts no floor on the alpha of a cloaked weapon, and it increases its red
instead. Near.

Fixed in the modern build: WeaponModel.cpp, the 0.35 floor applies to the
alpha. MODERN.md M-B54.

### A mounted turret with a full circle of yaw never wraps its yaw

PHCShell.dll 0x10135480 (`CClientTurret::Rotate`, our name,
src/AVP2/ClientShellDLL/ClientTurretMgr.cpp).

For a turret whose yaw range is a full turn (6.28 or more), the function
calculates `floor(m_fYaw / 2pi)` and discards the result. Thus the code
does not wrap or clamp the yaw, and the yaw increases with no limit while
the player turns. In practice this is harmless, because the code uses
the angle only for a rotation. But the wrap was clearly intended.

Fixed in the modern build: ClientTurretMgr.cpp, the code wraps the yaw
into [0, 2 pi) (`m_fYaw -= floor(m_fYaw / 2 pi) * 2 pi`). MODERN.md
M-B55.

### The turret recoil pushes the first-person weapon off by 0.95 on every axis

PHCShell.dll 0x10069e80 (`CameraMgrFX_RecoilUpdate`, our name,
src/AVP2/ClientShellDLL/CameraMgrFX.cpp).

This applies while the recoil effect runs and the player is not at a
turret. The code moves the weapon model by the recoil offset of the camera
*plus* 0.95 on each axis (`vPos += 0.95f`). Almost certainly, the intention was to scale
the offset by 0.95. Thus the weapon jumps up, right and forward by almost
one unit for the life of the effect.

Fixed in the modern build: CameraMgrFX.cpp, the code scales the offset by
0.95 (`vPos *= 0.95f`). MODERN.md M-B56.

### Putting the weapon away for a turret hides muzzle flashes it may not have (probable)

PHCShell.dll 0x100b9f00 (`CWeaponModel::Holster`, our name,
src/AVP2/ClientShellDLL/WeaponModel.cpp).

For each of the two muzzle flashes, the function calls `Hide()` before it
tests the pointer. Then it deletes the flash. `Hide` writes the members of
the flash. Thus, for a weapon with no flash on a muzzle, the code writes
through a null pointer.

Fixed in the modern build: WeaponModel.cpp, the code hides only a flash
that is there. MODERN.md M-B57.

### The host options' class and weapon lists accept the index one past the last entry

PHCShell.dll 0x10018ba0 (`CFolderHostOptions::OnCommand`,
src/AVP2/ClientShellDLL/FolderHostOptions.cpp). The check is
`CMPRestrictList::SetAllowed` (src/AVP2/Shared/MPRestrictList.h).
`GetId` and `IsAllowed` near it have the same check.

The code tests the index with `i > m_nNumEntries`, but `>=` was intended.
Thus the code writes the entry immediately after the last entry. With a
full list (32 entries), that entry is the count itself (`GetId` returns
32). Its allowed flag is the four bytes after the list, and `SetAllowed`
writes them.

Fixed in the modern build: MPRestrictList.h, `SetAllowed`, `GetId` and
`GetName` test `i >= count`; MODERN.md M-B78. (The next entry is about
the commands after the last check box, in the host options.)

### The host options take check-box commands past the end of their boxes

PHCShell.dll 0x10018ba0 (`CFolderHostOptions::OnCommand`,
src/AVP2/ClientShellDLL/FolderHostOptions.cpp).

The code accepts a class box command up to `CMD_CLASS_FIRST + 32`, but
there are only 24 class boxes. It accepts a weapon box command up to
`CMD_WEAPON_FIRST + 64` (inclusive), for 64 weapon boxes. Thus an
incorrect command gives an index past `m_aClassBoxes` or
`m_aWeaponBoxes`.

Fixed in the modern build: FolderHostOptions.cpp, the code accepts a
command only for a box that is there (below `CMD_CLASS_FIRST + 24`,
`CMD_WEAPON_FIRST + 64`). MODERN.md M-B58.

### The host options' check boxes leak their bitmaps

PHCShell.dll 0x10018690 (`CHostCheckBox::CHostCheckBox`, our name,
src/AVP2/ClientShellDLL/FolderHostOptions.h).

Each of the 88 check boxes makes a `CBitmapCtrl` with `new`. Its
destructor is empty, and it never deletes the `CBitmapCtrl`.

Fixed in the modern build: FolderHostOptions.cpp/.h, the `Term` of the
folder (and its destructor) removes the bitmap of each box from the fixed
controls and deletes it. The fixed controls hold the bitmap only while
its page shows. Thus the code deletes each bitmap one time, whichever page
showed last. MODERN.md M-B59.

### The server list's no-entry surface is never deleted

PHCShell.dll 0x1001aed0 (`CFolderJoin::Build`,
src/AVP2/ClientShellDLL/FolderJoin.cpp).

Build makes `Interface\Exp\Menus\noentry.pcx` each time it runs. The
destructor deletes only the lock surface.

Fixed in the modern build: FolderJoin.cpp, the destructor deletes it.
Also, the constructor clears its handle, which the retail code did not
set. MODERN.md M-B60.

### The server list checks the wrong number against a server's class list

PHCShell.dll 0x1001dda0 (`CFolderJoin::IsClassAllowed`, our name,
src/AVP2/ClientShellDLL/FolderJoin.cpp).

The function marks the servers that do not permit the class of the
player. To do this, it looks for the `m_nChar[m_nRace]` of the profile
among the character-set IDs of the class list. We thought that this value
was an index into the list of characters of that race. Thus we thought
that the no-entry mark showed for the wrong servers. (Refer to the text
below.)

Not fixed: it is not a bug. The `m_nChar[race]` of the profile is the
character set of the race. It is not an index into its characters:

- Choose Character (`FolderPlayer.cpp`, `FolderPlayerJoin.cpp`) stores
  `m_nCharList[race][index]` in it.
- The defaults of `ProfileMgr.cpp` are sets (`GetSetFromModelType`).

Thus it is the ID of the class list itself, as
`CBaseFolder::IsCharacterAllowed` and `CFolderHost::IsCharacterAllowed`
use it.

### The Corporate motion detector settings check the Marine's tag

PHCShell.dll 0x1004d720 to 0x1004de80 (the `CLayoutMgr::GetMD...` and
`Get...SurfaceName` functions, src/AVP2/ClientShellDLL/LayoutMgr.cpp).

For the Corporate motion detector, each function tests that
`HUDMotionDetector` exists. Then it reads `HUDMotionDetector_Corporate`.
Thus, for a layout without the Corporate tag, the function reads nothing
(or zeros). It does not use the fallback.

Fixed in the modern build: LayoutMgr.cpp, all seventeen functions test the
Corporate tag. Thus a layout without the Corporate tag uses the Marine's
tag as the fallback. (The layout of Primal Hunt has the two tags. Thus
nothing changes with its data.) MODERN.md M-B61.

### The Marine weapon chooser's dual pistols check has no null test

PHCShell.dll 0x10057790 (`CMarineWepChooser::IsDualPistols`, our name,
src/AVP2/ClientShellDLL/WeaponChooser.cpp).

The function reads the name of the weapon. It does not test the
`GetWeapon` result. Thus, for an incorrect weapon ID, it reads through a
null pointer.

Fixed in the modern build: WeaponChooser.cpp, when there is no weapon,
the result is "not the dual pistols". MODERN.md M-B62.


### A tracer whose Init fails removes three garbage flare handles

PHCShell.dll 0x1012d970 (the destructor of `CTracerFX`) and 0x1012dbe0
(`CTracerFX::RemoveFlares`, our name, src/AVP2/ClientShellDLL/TracerFX.h
and TracerFX.cpp).

The constructor does not set the three flare handles. Init clears them
only after it has the TRACERFX of the tracer. Term (from the destructor)
deletes each handle that is not null. Thus, when the Init of a tracer
fails before that point, the code gives three uninitialised handles to
`DeleteObject`. Identical.

Fixed in the modern build: TracerFX.h, the constructor clears the
handles. MODERN.md M-B63.

### Detaching a client from another's view writes through a null message

`sm_DetachClient` (Primal Hunt lithtech.exe 0x00477200, server.dll
0x10032452; AvP2 1.0.9.6 lithtech.exe 0x00470db0, server.dll 0x1002fc62;
src/engine/lithtech/ccobj.cpp for the engine, src/engine/server/smgr.cpp
for server.dll).

When the client has a view object to report, the code never allocates
the detach message:

- The copies in the engines and the server.dll of Primal Hunt write the
  header words through an empty `CMessageRef`.
- The server.dll of AvP2 writes them through a null `ServerMessage*`. The
  stores go to the absolute addresses 0x38 and 0x34.

The code then gives the null message to `sm_SendToClient`. The related
function `sm_AttachClient` first allocates with `msg_AllocPacket`. When
the server detaches such a client, it crashes. All four copies are
identical.

Fixed in the modern build: ccobj.cpp and smgr.cpp, the code allocates the
message (`msg_AllocPacket(0x44c, 0x44c)`, as `sm_AttachClient` does)
before it writes it; MODERN.md M-B09.

### A facehugger whose target is not a character crashes its attack

PHObject.lto 0x10214eb7 (`SimpleAIFacehugger::UpdateAttack`,
src/AVP2/ObjectDLL/SimpleAIAlien.cpp).

Primal Hunt added a test that the target is not already dead, before the
code rolls the facehug chance. The test reads
`GetCharTargetPtr()->IsDead()` (the dead flag of the `m_damage` of the
character, +0x2ac). But the null test of that same pointer is some lines
later. Thus, when a facehugger attacks a target that is not a character
(a `GetTarget()` with no character pointer), the code dereferences null.
Identical.

Fixed in the modern build: SimpleAIAlien.cpp, the code does the dead test
only for a character. For all other targets, the code rolls the chance as
in AvP2, and the test after it refuses the facehug; MODERN.md M-B84.

### AvP2's player code casts its attachments without testing the cast

Object.lto (src/AVP2/ObjectDLL/PlayerObj.cpp):

- 0x101cd9c7 (`CPlayerObj::ChangeWeapon`)
- 0x101cda78 (`DoWeaponChange`)
- 0x101cbb52 (`FullAmmoCheat`)
- 0x101cbc96 (`FullWeaponCheat`)
- 0x101ccc99 (`AquireDefaultWeapon`)
- 0x101cbf78 and 0x101cc5f3 (`Respawn`)
- 0x101cdb17 (`UpdateInterface`)
- 0x101d35c1 (`SendMPInterfaceUpdate`)

0x101ce4b2 (`HandleGameRestore`) and 0x101d2e20 (`GetPlayerWeapon`) do
not cast at all.

The code uses the result of each `dynamic_cast<CPlayerAttachments*>(m_pAttachments)`
with no null test. Some functions test `m_pAttachments` first, but none
tests the result of the cast. Thus, when the attachments of a player are
not a `CPlayerAttachments`, the code dereferences null. Primal Hunt casts
one time and tests the result at all places. All identical.

Not fixed: the cast cannot fail. Only `CPlayerObj::CreateAttachments`
makes the attachments of a player. This function is virtual. It runs from
the `MID_PRECREATE` of `CCharacter`, and again after `ResetCharacter`
destroys the attachments. It always makes a `CPlayerAttachments`, before
any of these functions runs. Only a failed `new` can make them null.

### A folder's page change re-selects with the cursor's screen point

CShell.dll and PHCShell.dll (`CBaseFolder::NextPage`, `PreviousPage`,
`ForceMouseUpdate`, src/AVP2/ClientShellDLL/BaseFolder.cpp;
`CGroupCtrl::OnSelChange`, GroupCtrl.cpp; we did not look up the
addresses).

Each function runs its `OnMouseMove` again with
`CInterfaceMgr::GetCursorPos()`. This returns the point of the cursor on
the screen, as the window gave it. (The function also calculates the
point on the page, but it discards it.) At all other places, the folders
get the point minus the offsets of the page
(`CInterfaceMgr::OnMouseMove`). Their hit tests compare this point with
controls at page positions.

At 640x480, the offsets are 0, and the two points are the same. At
1024x768, the page is at (192, 144). This has an effect after a page
change, and when the selection of the folder is forced. The code then
selects the control under a point that is that distance down and right of
the cursor (or no control). (`CListCtrl::Render` uses the same call against the screen
positions of its controls while it draws, and is correct.)

Fixed in the modern build: the callers remove the offsets of the page,
BaseFolder.cpp and GroupCtrl.cpp; MODERN.md M-B108. (With the scaled page
of M-15, the offsets are not 0 at any wide screen.)

## NOLF's bugs AvP2 does not have

(none checked yet)
