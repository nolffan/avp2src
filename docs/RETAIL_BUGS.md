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
Not yet shown to happen in the game. The retail shape is not yet
reproduced either (ours clears only the mark; a NEAR).

Not fixed yet: the retail build has to reproduce the store first.

## NOLF's bugs AvP2 does not have

(none checked yet)
