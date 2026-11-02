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

(none listed yet)

## NOLF's bugs AvP2 does not have

(none checked yet)
