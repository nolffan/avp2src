// *********************************************************************** //
//
//  MODULE:   D3DR5B_O2 (.CPP)  --  the /O2 members of d3dr5b.cpp's batch
//
//  PURPOSE:  0x10064640 and 0x100408c0, moved out of d3dr5b.cpp (/Os).
//            Both DUPLICATE their epilogue into the overflow arm -- the /O2
//            shape; at /Os they cross-jump.  Measured: compiled /O2,
//            0x100408c0 is byte-identical (98 -> 102 bytes, 26 -> 0
//            differing) and 0x10064640 is size-exact (27 -> 10 differing);
//            nothing else in d3dr5b.cpp gains at /O2.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  0x10021d60 -- the renderer's debug printf: `if (nLevel > g_1008caac)
//  return;` then _vsnprintf into a 0x100 buffer and the console print at
//  (*0x1008cbcc)+0x14.  That is LithTech's AddDebugMessage(level, fmt,
//  ...), and the NAME IS MEASURED, NOT COSMETIC: icl 4.0 treats a call to
//  a function whose name contains "Warn" or "Error" as cold and sinks the
//  block holding it below the epilogue.  Declared `d3d_Warning`, the
//  overflow arm of d3dr5d.cpp's 0x10064b70 is sunk three times and this
//  file's 0x100408c0 comes out 7 blocks off; declared AddDebugMessage (or
//  sub_10021d60, the name used here because d3dr5a.cpp, big331e0.cpp and
//  big3a080.cpp already reference it, so the link gains no new symbol)
//  both are identical.  (Probed: Warn, d3d_Warnin, d3d_WarningX, Error sink;
//  Printf, Probe, W, Wa, Wxyz, aaa, zzz do not.)  Both call sites here plant the level as
//  a literal 1 at (%esp), the format at 0x4(%esp) and one pointer at
//  0x8(%esp), with no cleanup of its own: the outgoing area is part of the
//  caller's frame reservation, which is the /O2 shape.
// ---------------------------------------------------------------------- //
extern void sub_10021d60(uint32 nLevel, const char* pszFormat, ...);


// ---------------------------------------------------------------------- //
//  0x1007b8d0 -- `mov $0x10091a10,%eax ; ret`.  The module's one big
//  context singleton, already pinned by d3dr2a.cpp / d3dr3a.cpp.  Thirty-
//  nine callers.
//
//  The two "set" functions below give its layout at three places, and the
//  three agree field for field, which is what makes the struct below a
//  reading rather than a guess:
//      +0x00 elements   +0x04 count   +0x14 max   +0x18 name
//  at context offsets 0x1a4, 0x1c4 (0x10064640) and 0x244 (0x100408c0).
//  0x1a4 and 0x1c4 are 0x20 apart, which fixes the record at 0x20 bytes.
// ---------------------------------------------------------------------- //
struct D3DSet
{
	void**      m_pElements;                            // +0x00
	uint32      m_nCount;                               // +0x04
	uint8       m_pad_08[0x0c];                         // +0x08
	uint32      m_nMax;                                 // +0x14
	const char* m_pszName;                              // +0x18
	uint8       m_pad_1c[0x04];                         // +0x1c
};

struct D3DContext
{
	uint8   m_pad_000[0x1a4];                           // +0x000
	D3DSet  m_Set1a4;                                   // +0x1a4
	D3DSet  m_Set1c4;                                   // +0x1c4
	uint8   m_pad_1e4[0x60];                            // +0x1e4
	D3DSet  m_Set244;                                   // +0x244
	uint8   m_pad_264[0x100];                           // +0x264
};

extern D3DContext* sub_1007b8d0();                      // 0x1007b8d0


// ======================================================================= //
//  0x10064640 -- 91 bytes.  Push one object onto whichever of the
//  context's two sets its flags select, or warn that the set is full.
//
//      53 / 83 ec 10            no frame pointer, 0x10 of outgoing area
//      e8 <0x1007b8d0>          the singleton
//      8b 5c 24 18              the one argument, read AFTER the call
//      8b 93 88 00 00 00 / f6 c2 10 / 74 07
//      05 c4 01 00 00 / eb 05 / 05 a4 01 00 00
//      8b 48 04 / 8b 50 14 / 3b ca / 73 0d
//      8b 10 / 89 1c 8a / ff 40 04
//      83 c4 10 / 5b / c3          <- epilogue copy 1
//      c7 44 24 04 <"Set '%s' overflowed">
//      8b 40 18 / c7 04 24 01 / 89 44 24 08
//      e8 <0x10021d60>
//      83 c4 10 / 5b / c3          <- epilogue copy 2
//
//  WHAT PINS IT:
//    * TWO epilogues, byte for byte the same, is the /O2 if/else shape:
//      one shared epilogue would have cross-jumped.  So this is if/else,
//      not `if (full) { warn; return; } store;`.
//    * `cmp %edx,%ecx` is Intel `cmp ecx,edx` with %ecx = the count read
//      from +0x04, so the count is the source's LEFT operand and the
//      guard tests `m_nCount < m_nMax`.  `jae` therefore skips to the
//      warning: the STORE is the `if` arm.
//    * the true arm of the flags test FALLS THROUGH to `add $0x1c4`, so
//      the set at +0x1c4 is the one the set bit selects.
//    * `add $0x1c4,%eax` on the singleton's return value rather than a
//      `lea`: the /O2 `&p->m` idiom, which is why the source names a
//      pointer for the chosen set and then uses it four times.
//
//  IDENTICAL: the name is read through a pointer to the field
//  (`ppszName = &pSet->m_pszName`), the "field loaded after a
//  neighbouring store" lever.  With the plain `pSet->m_pszName` argument
//  it was the near recorded below.
//
//  WAS NEAR at /O2, size-exact, one scheduling swap: shipped plants the format,
//  THEN loads pSet->m_pszName, THEN the level; ours plants the level before
//  the load.  (d3dr5d.cpp's 0x10064b70 and this file's 0x100408c0 get the
//  shipped order from the same statement.)  Not moved by: an inline
//  AddToSet(pSet, pObject) helper as d3dr5d writes it, the add written as
//  a D3DSet member, two helper calls on the two sets, a `?:` or a switch
//  for the choice, byte-offset arithmetic, a named pszName local, a
//  non-const name, a named level; nor by /G5 /G6 /Qrcd /Qip /Ox /Ob0.
// ======================================================================= //
struct Obj10064640
{
	uint8  m_pad_00[0x88];                              // +0x00
	uint32 m_nFlags88;                                  // +0x88
};

void sub_10064640(Obj10064640* pObject)
{
	D3DContext* pContext;
	D3DSet*     pSet;

	pContext = sub_1007b8d0();

	if (pObject->m_nFlags88 & 0x10)
		pSet = &pContext->m_Set1c4;
	else
		pSet = &pContext->m_Set1a4;

	if (pSet->m_nCount < pSet->m_nMax)
	{
		pSet->m_pElements[pSet->m_nCount] = pObject;
		pSet->m_nCount++;
	}
	else
	{
		const char* const* ppszName = &pSet->m_pszName;

		sub_10021d60(1, "Set '%s' overflowed", *ppszName);
	}
}


// ======================================================================= //
//  0x100408c0 -- 102 bytes.  The same push, guarded by a global switch and
//  addressed straight off the singleton with no pointer local.
//
//      83 ec 14 / a1 <0x100893dc> / 85 c0 / 75 04
//      83 c4 14 / c3                    <- the early return, INLINE
//      e8 <0x1007b8d0>
//      8b 90 48 02 / 8b 88 58 02 / 3b d1 / 73 1f
//      8b 88 44 02 / 89 7c 24 10 / 8b 7c 24 18 / 89 3c 91
//      ff 80 48 02 / 8b 7c 24 10
//      83 c4 14 / c3
//      c7 44 24 04 <"Set '%s' overflowed"> ... e8 <0x10021d60>
//      83 c4 14 / c3
//
//  WHAT PINS IT:
//    * `jne` PAST a complete epilogue is an early `return`, not an `if`
//      wrapping the body: an `if (g) { ... }` would have jumped forward to
//      one shared epilogue at the end.
//    * the set is addressed as 0x244(%eax) / 0x248 / 0x258 / 0x25c with no
//      pointer ever materialised, so the source spells the member chain at
//      each use.  Contrast 0x10064640, whose branch forces the local.
//    * %edi is saved and restored INTO THE FRAME (`mov %edi,0x10(%esp)`)
//      rather than pushed -- icl's own register-save style, and the reason
//      the frame is 0x14 rather than 0x10.
// ======================================================================= //
extern uint32 g_100893dc;                               // 0x100893dc

void sub_100408c0(void* pObject)
{
	D3DContext* pContext;

	if (!g_100893dc)
		return;

	pContext = sub_1007b8d0();

	if (pContext->m_Set244.m_nCount < pContext->m_Set244.m_nMax)
	{
		pContext->m_Set244.m_pElements[pContext->m_Set244.m_nCount] = pObject;
		pContext->m_Set244.m_nCount++;
	}
	else
	{
		sub_10021d60(1, "Set '%s' overflowed", pContext->m_Set244.m_pszName);
	}
}


