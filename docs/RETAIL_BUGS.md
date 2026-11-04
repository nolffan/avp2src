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

## NOLF's bugs AvP2 does not have

(none checked yet)
