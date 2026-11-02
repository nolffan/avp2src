// *********************************************************************** //
//
//  w32_1001e210.cpp -- 0x1001e210 (+ its aligned second entry 0x1001e21e),
//  the dynamic-light accumulator, and 0x1002b0e0, the colour-scale
//  selector.  /O2.
//
//  Moved out of d3dr3d.cpp (/Os).  The shipped loop head is aligned
//  (`lea 0x0(%esi),%esi ; lea 0x0(%edi),%edi` at 0x1001e254), the /O2 tell.
//  Byte-identical at /O2 with the switch test written as an early return:
//  as `if (g_100893e0) { loop }` icl loads pObject ahead of the zero-count
//  test (10 bytes differ); the shipped code loads it after.  (d3dr3d.cpp's
//  /Os build was 227 bytes differing.)
//
//  0x1002b0e0 moved with it, measured: it is byte-identical here at /O2
//  too, and it LOST its match in d3dr3d.cpp once 0x1001e210 left -- its
//  two `* 1.0f` multiplies (shipped `fmuls`) are folded away unless an
//  earlier function in the TU has already put 1.0f in the literal pool.
//
// *********************************************************************** //

#include "ltbasedefs.h"

// ======================================================================= //
//  0x1001e210   330 bytes of body   2 callers (0x1005f850, 0x10062af8)
//
//  Accumulate the dynamic lights that reach one object into a colour vector.
//  Zero the answer; then, if the dynamic-light switch is on, walk the
//  g_1008bd88[] array of g_1008bd84 light pointers and, for each light whose
//  radius squared exceeds the squared distance to the object, add its colour
//  scaled by `(1 - dist/radius) * 0.7`.
//
//  WHAT PINS IT.
//
//   * The distance is emitted z-term, y-term, x-term and each difference is
//     squared with `fmul %st(0),%st`.  That is one n-ary expression under
//     the measured last-term-backwards rule, not three statements: three
//     statements would compute x first.
//   * `flds 0x1a8(%edx) ; fld %st(0) ; fmul %st(0),%st ; fcomp %st(2)`
//     keeps the radius live on the x87 stack across the whole `if` body and
//     divides by it later with `fdiv %st(1),%st`, so the radius is a source
//     LOCAL read once, and the test really is `r * r > distSqr`.
//   * `fld %st(1) ; fsqrt` takes the square root of the SAME distSqr the
//     test used, as a bare instruction -- this TU has sqrt as an intrinsic,
//     the same split d3dbatchd.cpp records between Mag (fsqrt) and Norm
//     (a call to the CRT's double sqrt).  Hence the #pragma below.
//   * Each colour byte is turned into `c - (255 - c)` LONGHAND -- `mov %eax,
//     %edi ; neg %edi ; add $0xff,%edi ; sub %edi,%eax` -- and NOT folded to
//     `2c - 255`.  icl kept it, so the source has that subexpression.
//   * pOut->x, ->y and ->z are all loaded before the first store, then each
//     accumulated and stored in turn, which is three `+=` statements icl
//     hoisted the loads out of.
//   * `mov 0x88(%edx),%eax ; mov %eax,0x88(%edx)` is a self-assignment that
//     survived /O2.  It is written out longhand because icl does no
//     dead-store elimination through memory (two demonstrations);
//     it is also why the loop reloads g_1008bd84 immediately afterwards.
//
//  Calling convention: plain `ret`, two dwords at 0x8(%ebx)/0xc(%ebx) behind
//  icl's own stack-realignment prologue (`push %ebx ; mov %esp,%ebx ; sub
//  $8 ; and $-16 ; add $8 ; jmp` over the aligned second entry).  That
//  prologue is the compiler's, not the source's.
//
//  Field map, and it is the SAME class for the object and for the light --
//  both are read at +0x18c: +0x88 dword, +0x94/95/96 colour bytes,
//  +0x18c LTVector position, +0x1a8 float radius.
// ======================================================================= //

struct C1001e210
{
	uint8		m_pad_00[0x88];
	uint32		m_n88;                                  // +0x88
	uint8		m_pad_8c[0x08];
	uint8		m_nColorR;                              // +0x94
	uint8		m_nColorG;                              // +0x95
	uint8		m_nColorB;                              // +0x96
	uint8		m_pad_97[0xf5];
	LTVector	m_Pos;                                  // +0x18c
	uint8		m_pad_198[0x10];
	float		m_fRadius;                              // +0x1a8
};

extern uint32     g_100893e0;                           // 0x100893e0
extern uint32     g_1008bd84;                           // 0x1008bd84
extern C1001e210* g_1008bd88[];                         // 0x1008bd88

#pragma intrinsic(sqrt)

void sub_1001e210(C1001e210* pObject, LTVector* pColor)
{
	uint32 i;
	C1001e210* pLight;
	float fDistSqr, fRadius, fScale;

	pColor->x = 0.0f;
	pColor->y = 0.0f;
	pColor->z = 0.0f;

	if (!g_100893e0)
		return;

	for (i = 0; i < g_1008bd84; i++)
	{
		pLight = g_1008bd88[i];

		fDistSqr =
			(pLight->m_Pos.x - pObject->m_Pos.x) *
			(pLight->m_Pos.x - pObject->m_Pos.x) +
			(pLight->m_Pos.y - pObject->m_Pos.y) *
			(pLight->m_Pos.y - pObject->m_Pos.y) +
			(pLight->m_Pos.z - pObject->m_Pos.z) *
			(pLight->m_Pos.z - pObject->m_Pos.z);

		fRadius = pLight->m_fRadius;

		if (fRadius * fRadius > fDistSqr)
		{
			fScale = (1.0f - (float)sqrt(fDistSqr) / fRadius) * 0.7f;

			pColor->x += (float)(pLight->m_nColorR -
			                     (255 - pLight->m_nColorR)) * fScale;
			pColor->y += (float)(pLight->m_nColorG -
			                     (255 - pLight->m_nColorG)) * fScale;
			pColor->z += (float)(pLight->m_nColorB -
			                     (255 - pLight->m_nColorB)) * fScale;

			pLight->m_n88 = pLight->m_n88;
		}
	}
}


// ======================================================================= //
//  0x1002b0e0   195 bytes of body + padding   1 caller (0x1002d3ca)
//
//  Recompute two colour-conversion scales and pick the pair of pixel
//  routines the current surface format wants.  Three exits, one function
//  pointer stored on every path.
//
//  WHAT PINS IT.  Nothing here is a call and nothing here is an argument:
//  every operand is a fixed global.  Two dword stores of code addresses
//  (0x1008ce88 and 0x1008ce8c always receive one of 0x10023570 / 0x100234d0
//  / 0x100235d0 / 0x100234e0) around a three-way test of 0x1008ca54 and
//  0x1008d644 is a dispatch-pair selection.
//
//  The x87 stack settles the store order.  The first block pushes
//  255/(cb00-cb04), then (893c8-cb08), then divides 255 by the latter; it
//  stores (893c8-cb08) to 0x1008caf0 first, so that difference is a SOURCE
//  LOCAL used twice.  The second block pushes 1/(d6e8-d708), then
//  255/d668, then (d6a8-d6c8) and stores top-down with one `fxch`, which
//  means the source stores 0x1008d5e0, then 0x1008d5dc, then 0x1008d5d8 --
//  a LIFO stack can only produce that push order for that store order.
//
//  THE `* 1.0f` IS REAL.  0x100a6140 and 0x100a614c are both 1.0f and both
//  are genuinely multiplied in.  icl did not fold them, which is itself the
//  proof that writing `* 1.0f` here reproduces: the same compiler that kept
//  them will keep ours.  (Every one of these functions carries a 16-byte
//  private constant block {-1e-5, 1.0, 1e-5, 0.0} or {255, 1, 1}; the 1.0f
//  is a real operand in this one.)
//
//  LEAST CERTAIN CALL, and the alternative: the three stores in the
//  g_1008d644 arm are ordered 0x1008d5e0, 0x1008d5dc, 0x1008d5d8 on the
//  x87-stack argument above.  Independent stores to distinct globals are
//  reorderable, so if this misses, try ascending order --
//      g_1008d5d8 = 255.0f / g_1008d668;
//      g_1008d5dc = 1.0f / (g_1008d6e8 - g_1008d708);
//      g_1008d5e0 = g_1008d6a8 - g_1008d6c8;
//  which is the only other spelling that puts all three values on the stack
//  at once.
// ======================================================================= //

// The two slots hold code addresses; their real signatures are not known
// here, and nothing in this function depends on them.
typedef void (*Pfn1002b0e0)();

extern Pfn1002b0e0 g_1008ce88;                          // 0x1008ce88
extern Pfn1002b0e0 g_1008ce8c;                          // 0x1008ce8c
extern uint32 g_1008ca54;                               // 0x1008ca54
extern uint32 g_1008d644;                               // 0x1008d644
extern float  g_1008cb00;                               // 0x1008cb00
extern float  g_1008cb04;                               // 0x1008cb04
extern float  g_1008cb08;                               // 0x1008cb08
extern float  g_100893c8;                               // 0x100893c8
extern float  g_1008caec;                               // 0x1008caec
extern float  g_1008caf0;                               // 0x1008caf0
extern float  g_1008d5d4;                               // 0x1008d5d4
extern float  g_1008d5d8;                               // 0x1008d5d8
extern float  g_1008d5dc;                               // 0x1008d5dc
extern float  g_1008d5e0;                               // 0x1008d5e0
extern float  g_1008d668;                               // 0x1008d668
extern float  g_1008d6a8;                               // 0x1008d6a8
extern float  g_1008d6c8;                               // 0x1008d6c8
extern float  g_1008d6e8;                               // 0x1008d6e8
extern float  g_1008d708;                               // 0x1008d708

void sub_10023570();
void sub_100234d0();
void sub_100234e0();
void sub_100235d0();

void sub_1002b0e0()
{
	float fRange;

	g_1008ce88 = sub_10023570;

	fRange = g_100893c8 - g_1008cb08;
	g_1008caf0 = fRange;
	g_1008d5d4 = (255.0f / (g_1008cb00 - g_1008cb04)) * 1.0f;
	g_1008caec = (255.0f / fRange) * 1.0f;

	if (g_1008ca54)
	{
		g_1008ce8c = sub_100234d0;
	}
	else if (g_1008d644)
	{
		g_1008ce8c = sub_100235d0;
		// The 34-byte near was BLOCK LAYOUT, not store order: early
		// `return`s put the ca54 arm last (`jne`); one if / else-if / else
		// chain puts it on the fall-through (`je`), as shipped.
		g_1008d5e0 = g_1008d6a8 - g_1008d6c8;
		g_1008d5dc = 1.0f / (g_1008d6e8 - g_1008d708);
		g_1008d5d8 = 255.0f / g_1008d668;
	}
	else
	{
		g_1008ce8c = sub_100234e0;
	}
}
