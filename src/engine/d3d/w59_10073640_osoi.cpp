// *********************************************************************** //
//
//  w59_10073640_osoi.cpp -- d3d.ren, /Os /Oi- (as d3dr5c.cpp, where this
//  function was written first; moved here later).
//
//  0x10073640 -- 163 bytes, thiscall, no arguments, 1 caller (0x1007cae8).
//  IS THIS TWO-TEXTURE RECORD USABLE?  What pins each test is in
//  d3dr5c.cpp's old banner (kept there).
//
//  Still ATTEMPTED (the first-guard layout below shifts every
//  byte after offset 8), but 160 bytes against 163 (was 135), 4 mnemonic
//  blocks, instruction match 0.634 -> 0.876.  (`return !(m_f14 > 1.0f)` or
//  a `goto fail` tail give exactly 163 bytes, but a setcc-style tail.)
//  The +0x168/+0x16c pair is a REAL StdLith CMoArray<Rec> at +0x164
//  (vptr +0x164, m_pArray +0x168, m_nElements +0x16c), and the record's
//  +0x10 count is a second CMoArray's m_nElements (at +0x08 of the
//  record's object).  With dynarray.h's inlined GetSize()/operator[] icl
//  produces the shipped allocation by itself: `push %ebx ; sub $0x18`,
//  %edi/%esi saved into 0x10/0x14(%esp) around their one use each, the
//  owner reloaded from `this` for the two count tests and the first load
//  kept in %eax for the two record tests, and the double spill of +0x14
//  -- what d3dr5c.cpp had needed four volatile reads to approach and
//  never got (the frame-save style).
//
//  LEFT: the block layout of the first guard.  The shipped code sends
//  every `return 0` FORWARD to one block hosted after the `return 1`
//  (`je`/`jae` to 0x100736d8, single epilogue), with the `> 1.0f` test's
//  own copy after the epilogue; icl here lays the first guard's return
//  inline after the test, with its own epilogue.  Measured without
//  effect: `if (pWorld) {...}`, nested positive tests, one `||` guard,
//  `&&` return expressions, a result variable (worse: frame 0x10), a
//  `goto fail`, `== LTNULL`, an integer-typed owner field, an inline
//  wrapper method (worse), and the flag sets /O2, /Os, /O2 /Qip,
//  /Os /Qip /Qip_no_inlining, /Os /Qsfalign16, /Os /G5.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"

// The record's object: a CMoArray at +0x08, so its m_nElements is +0x10.
struct Sub10073640
{
	uint8	m_pad_00[0x08];
	CMoArray<uint32>	m_Arr;                          // +0x08: +0x10 count
};

struct Rec10073640
{
	Sub10073640*	m_p00;                              // +0x00
	uint8			m_pad_04[0x20 - 0x04];              // 32-byte stride
};

struct World10073640
{
	uint8			m_pad_000[0x164];
	CMoArray<Rec10073640>	m_Recs;                     // +0x164
};

struct Item10073640
{
	World10073640*	m_pWorld;                           // +0x00
	uint16			m_nRec0;                            // +0x04
	uint16			m_nSub0;                            // +0x06
	uint8			m_pad_08[0x0c - 0x08];
	uint16			m_nRec1;                            // +0x0c
	uint16			m_nSub1;                            // +0x0e
	uint8			m_pad_10[0x14 - 0x10];
	float			m_f14;                              // +0x14

	LTBOOL	sub_10073640() const;
};

//  The guards as NESTED POSITIVE TESTS with the remaining
//  ones in one `&&` chain that ends `m_f14 >= 0.0f`, the `> 1.0f` test
//  inside, and ONE `return LTFALSE` at the end: every guard now jumps
//  FORWARD to a shared false block as shipped (`je`/`jae` past the
//  body), 162 bytes against 163, 21 bytes differing (was 144), positional
//  agreement 0.073 -> 0.870.  Still not paired: the shipped tail is
//  `return 1` (`xor ; inc ; jmp`), the shared false block with the
//  epilogue, and the `> 1.0f` test's OWN false copy sunk after it
//  (`xor ; jmp`), so the row ends in a jmp; icl here merges that false
//  return into the shared one and places it before `return 1`, so ours
//  ends `ret` and the tail check refuses the pairing.  Same
//  bytes: `else return LTTRUE`, `goto` to a second false label, the
//  `>= 0` test as its own nested if, `|| !pWorld`, non-const.  Worse
//  (back to the inline first guard, 144): `if (m_f14 <= 1.0f) return
//  LTTRUE` in any form, `return !(m_f14 > 1.0f)`, a ?: tail, the
//  `>= 0 && <= 1` both in the chain, a result variable, the server.dll
//  twin's separate guards with one `||` (commonmodel.cpp 0x10016410).
//  No flag set changes the first-guard layout (all ten conventions).
LTBOOL Item10073640::sub_10073640() const
{
	World10073640*	pWorld;
	uint32			n0, n1;

	pWorld = m_pWorld;
	if (pWorld)
	{
		n0 = m_nRec0;
		if (n0 < m_pWorld->m_Recs.GetSize())
		{
			n1 = m_nRec1;
			if (n1 < m_pWorld->m_Recs.GetSize() &&
			    m_nSub0 < pWorld->m_Recs[n0].m_p00->m_Arr.GetSize() &&
			    m_nSub1 < pWorld->m_Recs[n1].m_p00->m_Arr.GetSize() &&
			    m_f14 >= 0.0f)
			{
				if (m_f14 > 1.0f)
					return LTFALSE;
				return LTTRUE;
			}
		}
	}
	return LTFALSE;
}
