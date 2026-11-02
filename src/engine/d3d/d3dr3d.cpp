// *********************************************************************** //
//
//  MODULE:   BATCH D (.CPP) -- d3d.ren, the 25 largest bodies.
//
//  SIX functions are written here.  The other nineteen rows are accounted
//  for at the bottom of this file, each with the decode that got as far as
//  it got, because three of the four reasons they are not written are NOT
//  lack of time:
//
//    * FIVE rows (0x10001990, 0x10001fe0, 0x10002670, 0x100029e0 and
//      0x1001eb90) need /QIfist.  They are fully decoded below -- the four
//      clip functions are ONE template and the decode covers all of them
//      plus four siblings that are not on this listing -- but every one of
//      them converts a float to a byte with a bare `fistpl`, which the
//      flags engine.sh uses today cannot emit.  This is the single largest
//      lever in this batch: ~6.5KB of shipped code behind one flag.
//    * EIGHT rows make a REGISTER-ABI CALL: the callee takes its arguments
//      in %eax/%edx/%ecx and the caller sets them up instead of pushing.
//      That is icl's own convention between icl's own functions and no C
//      declaration selects it, so the caller cannot be spelled either.  The
//      listing filter removes register-ABI ENTRIES; it does not remove
//      their callers.
//    * TWO rows are register-ABI entries the filter missed.
//    * THREE rows are not function entries at all -- the enumeration landed
//      mid-body, one of them mid-INSTRUCTION.
//
//  Naming: `sub_<address>` throughout, so the address can be read
//  out of the symbol.  Types invented here are named after their address.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// ---------------------------------------------------------------------- //
//  The Direct3D 7 device, exactly as src/engine/d3d/d3dbatchb.cpp and
//  d3dopt2d.cpp model it.  Repeated because these are separate translation
//  units; the slot evidence is in those files and is not repeated.
// ---------------------------------------------------------------------- //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x50];
	// +0x50 -- IDirect3DDevice7::SetRenderState (index 20).
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);
	uint8 m_pad_54[0x38];
	// +0x8c -- IDirect3DDevice7::SetTexture (index 35).
	long (__stdcall* SetTexture)(D3DDevice7*, uint32 dwStage,
	                             void* lpTexture);
	uint8 m_pad_90[0x04];
	// +0x94 -- IDirect3DDevice7::SetTextureStageState (index 37).
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32 dwStage,
	                                       uint32 dwState, uint32 dwValue);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

// 0x1008d988 -- the device.  Same global d3ddraw.cpp / d3dopt2d.cpp use.
extern D3DDevice7* g_pD3DDevice;

// D3DRENDERSTATETYPE / D3DBLEND / D3DTEXTURESTAGESTATETYPE / D3DTEXTUREOP,
// spelled out because the toolchain ships DirectX 5's d3d.h.
#define D3DRENDERSTATE_SRCBLEND               19        // 0x13
#define D3DRENDERSTATE_DESTBLEND              20        // 0x14

#define D3DBLEND_ONE                           2
#define D3DBLEND_SRCALPHA                      5

#define D3DTSS_COLOROP                         1
#define D3DTSS_COLORARG1                       2
#define D3DTSS_COLORARG2                       3

#define D3DTOP_SELECTARG1                      2
#define D3DTOP_MODULATE                        4
#define D3DTOP_MODULATE2X                      5
#define D3DTOP_MODULATEALPHA_ADDCOLOR         18        // 0x12

#define D3DTA_DIFFUSE                          0
#define D3DTA_CURRENT                          1
#define D3DTA_TEXTURE                          2


// ======================================================================= //
//  0x1005a410 -- MOVED to d3dr5d.cpp: it is an /O2 function (`mov $0x1,%eax`,
//  a duplicated epilogue, an aligned loop head) and this file is /Os.
// ======================================================================= //


// ======================================================================= //
//  0x10040780   133 bytes of body + padding   1 caller (0x10026552)
//
//  The sibling of 0x10040810, which d3dr2a.cpp already reproduces.  Same
//  file, same globals, same two COM slots:
//
//      if (!bArg) { SRCBLEND = SRCALPHA ; DESTBLEND = ONE }
//      if (g_1008cb2c) SetTextureStageState(0, COLOROP,
//                                           MODULATEALPHA_ADDCOLOR)
//
//  WHAT PINS IT.  Every call is the measured __stdcall COM shape: a 12- or
//  16-byte outgoing area opened with `add $0xfffffff4/$0xfffffff0,%esp`,
//  `this` reloaded from 0x1008d988 into (%esp) for each call, `call
//  *0x50(%eax)` / `call *0x94(%eax)`, nothing popped afterwards.  0x13/0x14
//  are the two blend states d3dopt2d.cpp pins seven times over, and
//  (SRCALPHA, ONE) is a valid blend pair.  The second test is bit-for-bit
//  the body of 0x10040810 with the value 0x12 instead of 0x02.
//
//  The argument test is a NEGATIVE guard -- `mov 0x8(%ebp),%eax ; test ;
//  jne` jumps PAST the two SetRenderState calls -- so the source is
//  `if (!bArg)`, and the second `if` is a separate statement, not an else:
//  it is reached both ways.
// ======================================================================= //

extern uint32 g_1008cb2c;                               // 0x1008cb2c

void sub_10040780(uint32 bArg)
{
	if (!bArg)
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
	}

	if (g_1008cb2c)
	{
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0,
			D3DTSS_COLOROP, D3DTOP_MODULATEALPHA_ADDCOLOR);
	}
}


// ======================================================================= //
//  0x10073210   396 bytes   2 callers (0x100264d7, 0x100339f3)
//
//  Six SetTextureStageState calls: stage 0 unconditionally, stage 1 behind
//  a two-way choice whose arms differ ONLY in the COLOROP value.
//
//      stage 0:  COLOROP = MODULATE, COLORARG1 = TEXTURE,
//                COLORARG2 = DIFFUSE
//      nOp = MODULATE ; if (g_100919b0) nOp = MODULATE2X
//      g_1008ca9c:  stage 1 COLOROP = SELECTARG1
//      else:        stage 1 COLOROP = nOp
//      both arms:   stage 1 COLORARG1 = TEXTURE, COLORARG2 = CURRENT
//
//  WHAT PINS IT.  Six identical __stdcall COM sequences on 0x1008d988 with
//  a 16-byte outgoing area and `call *0x94(%eax)`: (%esp) this, 0x4 stage,
//  0x8 state, 0xc value -- the slot and the argument positions d3dbatchb.cpp
//  already proves on 0x10011560.  Two reused-constant idioms confirm the
//  positions: the second stage-0 call puts ONE `mov $0x2,%eax` into both
//  0x8 and 0xc (COLORARG1 == D3DTA_TEXTURE == 2), and the stage-1 calls use
//  `xor %edx,%edx ; inc %edx` for the 1 that is both the stage and the
//  state.
//
//  THE THREE CALLS ARE DUPLICATED, NOT SHARED.  The shipped code emits the
//  COLORARG1/COLORARG2 pair twice, once per arm, and the arms join only at
//  the epilogue (`eb 73 jmp` out of the first).  A source that hoisted them
//  after the `if` would emit them once, so the source really does repeat
//  them inside both arms.
//
//  nOp is computed BEFORE the `if (g_1008ca9c)` test even though only the
//  else-arm uses it -- `mov g,%eax ; test ; mov $0x4,%eax ; je ; mov $0x5,
//  %eax` sits at 0x10073290, ahead of the branch -- so it is a statement of
//  its own ahead of the `if`, not a computation inside the else.
//
//  LEAST CERTAIN CALL, and the alternative: nOp is written here as a
//  default-then-override (`nOp = MODULATE; if (g) nOp = MODULATE2X;`)
//  because that is what `mov $0x4 ; je ; mov $0x5` looks like -- the 4 is
//  unconditional.  d3dbatchb.cpp records that a true if/else on this
//  compiler comes out as `mov $A ; jmp ; mov $B`, with a jump, which is NOT
//  what is here.  If this misses, invert it to
//      if (g_100919b0) nOp = D3DTOP_MODULATE2X; else nOp = D3DTOP_MODULATE;
// ======================================================================= //

extern uint32 g_100919b0;                               // 0x100919b0
extern uint32 g_1008ca9c;                               // 0x1008ca9c

void sub_10073210()
{
	uint32 nColorOp;

	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0,
		D3DTSS_COLOROP, D3DTOP_MODULATE);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0,
		D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0,
		D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	nColorOp = D3DTOP_MODULATE;
	if (g_100919b0)
		nColorOp = D3DTOP_MODULATE2X;

	if (g_1008ca9c)
	{
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1,
			D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1,
			D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1,
			D3DTSS_COLORARG2, D3DTA_CURRENT);
	}
	else
	{
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1,
			D3DTSS_COLOROP, nColorOp);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1,
			D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1,
			D3DTSS_COLORARG2, D3DTA_CURRENT);
	}
}


// ======================================================================= //
//  0x1006d460   152 bytes   1 caller (0x1003ae65)
//
//  Pre-allocate the five block pools: for each of five size classes, ask the
//  supplied allocator for N blocks of that size and thread each one onto
//  that class's list head, then take one more block of a different flavour
//  and park it in the parallel pointer array.
//
//  WHAT PINS IT.  The insert is ltlink.h verbatim.  Five stores:
//
//      mov %eax,0x24(%eax)      m_pData   = pBlock          (+0x1c +8)
//      lea 0x1c(%eax),%ecx      pLink
//      mov %edx,0x1c(%eax)      m_pPrev   = pHead           (+0x1c +0)
//      mov 0x4(%edx),%edx       pHead->m_pNext
//      mov %edx,0x20(%eax)      m_pNext   = pHead->m_pNext  (+0x1c +4)
//      mov %ecx,(%edx)          pLink->m_pNext->m_pPrev = pLink
//      mov 0x1c(%eax),%eax      RELOAD pLink->m_pPrev
//      mov %ecx,0x4(%eax)       pLink->m_pPrev->m_pNext = pLink
//
//  which is CheapLTLink::AddAfter with `this` = the head, including the
//  right-to-left evaluation of its double assignment (the m_pNext->m_pPrev
//  store first) and including the RELOAD, which is icl declining to keep
//  pLink->m_pPrev in a register across a store through a pointer.  m_pData
//  is stored ahead of the insert, which is dl_AddAfter's order -- but there
//  is no `++m_nElements`, and the head array strides 0x0c (LTLink) not 0x10
//  (LTList), so this is dl_AddAfter's body without a list object.  Both
//  facts agree with d3dr2a.cpp's 0x1006d520 and d3dr2d.cpp's 0x1006d3a4,
//  which tie off the SAME five 12-byte heads at 0x10091968 and zero the same
//  five-dword array at 0x10091954.
//
//  Calling convention.  `ret $0x4` with the argument at 0x14(%esp) after
//  four pushes: __stdcall, one dword, and the dword is called through
//  `call *%edi` with three pushed arguments and NO cleanup afterwards -- the
//  next iteration re-opens the area with `add $0xc,%esp` -- so the callee is
//  __cdecl and takes three dwords.  The caller passes 0x10038f50.
//
//  The size is a LOCAL and the count is NOT: g_10089820[i] is read once per
//  outer iteration into %esi and survives both calls, while g_10089834[i] is
//  re-read at the bottom of the inner loop.  Both loop counters are
//  unsigned -- `test %eax,%eax / jbe`, `cmp %eax,%ebx / jb`, `cmp $0x5,%ebp
//  / jb`.
// ======================================================================= //

struct DLink1006d460
{
	DLink1006d460*	m_pPrev;                            // +0x00
	DLink1006d460*	m_pNext;                            // +0x04
	void*			m_pData;                            // +0x08

	// ltlink.h's CheapLTLink::AddAfter, verbatim.  Defined in the class
	// body so /Ob1 inlines it, which is how the shipped code has it.
	void AddAfter(DLink1006d460* pLink)
	{
		pLink->m_pPrev = this;
		pLink->m_pNext = m_pNext;
		pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink;
	}
};

// The allocated block.  Nothing here reads it except its embedded link.
struct C1006d460
{
	uint8			m_pad_00[0x1c];
	DLink1006d460	m_Link;                             // +0x1c
};

// 0x1008e6c4 is a POINTER -- d3dr3c.cpp's 0x10039110 reads a pixel format
// through it -- and the type matters here: icl predicts a null-pointer
// early return as the cold edge and hosts it as a two-byte `jmp` after the
// `ret` (d3d_UnlockSurface's shape), where an integer test lays the return
// epilogue out at the top.
struct Fmt10039110;
extern Fmt10039110*  g_1008e6c4;                        // 0x1008e6c4
extern uint32        g_10089820[5];                     // 0x10089820
extern uint32        g_10089834[5];                     // 0x10089834
extern void*         g_10091954[5];                     // 0x10091954
extern DLink1006d460 g_10091968[5];                     // 0x10091968

// __thiscall: both callers (0x1003ae5a, and 0x10039a2c for 0x1006d3e0 in
// d3dr3a.cpp) load %ecx = 0x10091950 first -- the object whose +0x04 is the
// five-dword array at 0x10091954 and whose +0x18 is the five heads at
// 0x10091968.  The body never reads %ecx (the members are reached at their
// absolute addresses), but the unused `this` is what costs the frame its
// extra four bytes: `sub $0x14`, where the same body as a free __stdcall
// function gets `sub $0x10`.  Measured: __stdcall 2 bytes differ, a
// __stdcall member 4, a plain member (__thiscall, callee pops the one
// stack argument: `ret $0x4`) identical.
class C10091950
{
public:
	void	sub_1006d460(void* (__cdecl* pfnAlloc)(uint32, uint32, uint32));
};

void C10091950::sub_1006d460(void* (__cdecl* pfnAlloc)(uint32, uint32,
                                                         uint32))
{
	uint32 i, j, nSize;
	C1006d460* pBlock;

	// IDENTICAL.  With g_1008e6c4 typed as the pointer it is,
	// `if (!g) return;` gives the shipped false edge (a `jmp` placed after
	// the `ret`); as a uint32 the same statement put the return epilogue
	// at the top and 109 bytes differed.  The frame is the __thiscall
	// note above.
	if (!g_1008e6c4)
		return;

	for (i = 0; i < 5; i++)
	{
		nSize = g_10089820[i];

		for (j = 0; j < g_10089834[i]; j++)
		{
			pBlock = (C1006d460*)pfnAlloc(nSize, nSize, 0x4000);

			if (pBlock)
			{
				pBlock->m_Link.m_pData = pBlock;
				g_10091968[i].AddAfter(&pBlock->m_Link);
			}
		}

		g_10091954[i] = pfnAlloc(nSize, nSize, 0x800);
	}
}


// 0x1001e210 moved to w32_1001e210.cpp: its loop head is aligned in the
// shipped bytes, so it is /O2, and it is byte-identical there.


// 0x1002b0e0 moved with it, see w32_1001e210.cpp.


// *********************************************************************** //
//
//  WHAT WAS NOT WRITTEN, AND WHY.  Nineteen rows.  Everything below is read
//  off the shipped bytes; none of it needs to be re-derived.
//
// ----------------------------------------------------------------------- //
//  THE POLYGON CLIPPER FAMILY -- FOUR ROWS ON THIS LIST, EIGHT FUNCTIONS,
//  AND ALL OF THEM ARE BLOCKED ON /QIfist ALONE
//
//      0x10001990  800   32-byte vertices, plane  x == z
//      0x10001fe0  816   32-byte vertices, plane  z == g_1008b910
//      0x10002670  880   40-byte vertices, plane  x == -z
//      0x100029e0  864   40-byte vertices, plane  y == z
//
//  and off this listing, byte-identical in shape: 0x10001330 (832),
//  0x10001670 (800), 0x10001cb0 (816), 0x10002310 (864), plus a whole
//  second 40-byte family beginning at 0x10002d50.
//
//  THE BLOCKER, precisely.  Five colour bytes per emitted vertex are
//  interpolated and each one ends
//
//      fstps  tmpf ; flds tmpf ; fistpl tmpi ; mov tmpi,%dl ; mov %dl,N(%eax)
//
//  *** "BLOCKED ON /QIfist ALONE" IS WRONG TWICE OVER AND IS STRUCK. ***
//
//  First, icl 4.0 has no /QIfist.  It parses the switch as /QIf, matches it
//  against /QIfdiv, prints "ignoring option '-QIf'" and compiles exactly as
//  before -- so the earlier test that "proved" the flag changes nothing was
//  measuring an ignored switch.  The nearest real option is /Qrcd, "fast
//  float-to-int conversions".
//
//  Second, the per-TU inference was falsified.  This file used to argue that
//  because 0x1002ec60 does the full control-word dance while these sites are
//  bare, the two are in different translation units and the flag is per-TU.
//  0x1003e620 contains BOTH idioms in ONE function -- thirty bare `fistpl`
//  in its clipper, three full control-word sequences in its light block.
//  One function is one TU and one flag.  The difference tracks the CAST, a
//  32-bit convert off a float slot against a 64-bit one off a double.
//
//  So these eight functions are NOT waiting on a flag that someone has yet
//  to switch on.  Everything else about them is ordinary __cdecl code, and
//  what they actually need is the source shape that produces a 32-bit
//  convert off a float temporary.
//
//  THE SIGNATURE, from the caller at 0x1000f557-0x1000f601, which calls six
//  of them in a row through one flag word:
//
//      int32 f(void *pUnused, Vtx **ppVerts, int32 *pnVerts, Vtx **ppOut)
//
//  __cdecl, `add $0x38,%esp` at the caller.  arg0 is a DEAD parameter: the
//  caller passes &local, a different local for each plane (0x24, 0x28,
//  0x2c, 0x30 off %esp), and no body reads 0x8(%ebp).  arg1/arg2 are the
//  polygon in place -- 0x1000f60a writes them back out afterwards -- and
//  arg3 is a running allocation pointer shared by all six clips.
//
//  THE VERTEX, for the 32-byte family (the 40-byte one adds +0x20/+0x24):
//  +0x00 x, +0x04 y, +0x08 z, +0x0c untouched, +0x10..0x13 four colour
//  bytes, +0x14..0x16 untouched, +0x17 a fifth byte, +0x18 tu, +0x1c tv.
//
//  THE BODY, in full (offsets are 0x10001990's):
//
//      g_1008be44++;                            // a global counter
//
//      nTotal = 0;  pCode = nCodes;             // int nCodes[64] on frame
//      pCur = *ppVerts;
//      pEnd = pCur + *pnVerts;                  // shl $5 then add
//      while (pCur != pEnd) {                   // guard + do/while
//          n = (pCur->x < pCur->z);             // mov $1 / mov $0 / jb
//          nTotal += n;
//          *pCode = n;  pCode++;
//          pCur++;
//      }
//      if (nTotal == 0)         return LTFALSE; // the guard threads here
//      if (nTotal == *pnVerts)  return LTTRUE;  // wholly inside, untouched
//
//      nPrev  = *pnVerts - 1;
//      pStart = *ppOut;
//      pPrev  = *ppVerts + nPrev;
//      for (i = 0; i < *pnVerts; i++) {         // signed; *pnVerts re-read
//          pCur = *ppVerts + i;                 // *ppVerts re-read too
//          if (nCodes[nPrev])
//              *(*ppOut)++ = *pPrev;            // 8 mov pairs, not rep movs
//          if (nCodes[nPrev] != nCodes[i]) {    // nCodes[nPrev] RE-READ
//              d = (pCur->x - pPrev->x) - pCur->z + pPrev->z;
//              if (d < -0.00001f || d > 0.00001f)
//                  t = (pPrev->z - pPrev->x) / d;
//              else
//                  t = 0.0f;
//              (*ppOut)->y = (pCur->y - pPrev->y) * t + pPrev->y;
//              f = (pCur->z - pPrev->z) * t + pPrev->z;
//              (*ppOut)->x = f;                 // fst  then
//              (*ppOut)->z = f;                 // fstp -- x stored first
//              (*ppOut)->tu = (pCur->tu - pPrev->tu) * t + pPrev->tu;
//              (*ppOut)->tv = (pCur->tv - pPrev->tv) * t + pPrev->tv;
//              for each of +0x12, +0x11, +0x10, +0x13, +0x17:
//                  (*ppOut)->b = (uint8)((float)(pCur->b - pPrev->b) * t
//                                        + (float)pPrev->b);
//              (*ppOut)++;                      // addl $0x20,(%ecx)
//          }
//          nPrev = i;
//          pPrev = pCur;
//      }
//
//      *pnVerts = *ppOut - pStart;              // sub then sar $5
//      *ppVerts = pStart;
//      *ppOut  += *pnVerts;                     // *pnVerts RE-READ
//      return LTTRUE;
//
//  FOUR THINGS IN THAT ARE NOT GUESSES AND MUST NOT BE TIDIED:
//
//   1. The epsilon test is ONE `||`, not two `if`s.  The second comparison
//      jumps BACKWARDS into the first arm's divide (0x10001c6e -> 0x10001acf)
//      and only the fall-through loads 0.0f -- the exact shape of
//      d3d_Start3D.
//   2. `nCodes[nPrev]` is loaded twice, once for the copy test and once for
//      the crossing test, with the copy in between.  icl reloads it because
//      the struct copy might alias; the source reads it twice.
//   3. `*ppOut` is re-read after every group of stores, and `*ppVerts` at
//      the top of every iteration.  So the source works through the double
//      pointers and does NOT cache them in a local.  The copy is written
//      `*(*ppOut)++ = *pPrev` -- the pointer bump is emitted BEFORE the
//      eight stores, which post-increment permits and a separate statement
//      after the copy would not (icl would have to reload).
//   4. The tail advances *ppOut a SECOND time (`add %edi,(%eax)` after the
//      loop already stepped it once per emitted vertex).  That looks like a
//      bug in the original and it is in the shipped bytes; reproduce it.
//
//  THE FOUR PLANES, and the only lines that change between siblings:
//
//    0x10001990  n = (v->x < v->z)
//                d = (pCur->x - pPrev->x) - pCur->z + pPrev->z
//                t = (pPrev->z - pPrev->x) / d
//                order: y ; x = f ; z = f              (fst then fstp)
//    0x10001fe0  n = (v->z <= g_1008b910)              note <=, not <
//                d = pCur->z - pPrev->z
//                t = (g_1008b910 - pPrev->z) / d
//                order: x ; y ; z = g_1008b910         (an integer dword
//                move of the float global, icl's normal float copy)
//    0x10002670  n = (-v->z < v->x)                    (flds z ; fchs ; fcomps x)
//                d = (pCur->x - pPrev->x) + pCur->z - pPrev->z
//                t = (pPrev->x + pPrev->z) / -d ... emitted as FSUBP/fchs/FDIVRP
//                order: y ; z = f ; x = -f             (fld/fchs/fxch)
//                also interpolates +0x20 and +0x24
//    0x100029e0  n = (v->y < v->z)
//                d = (pCur->y - pPrev->y) - pCur->z + pPrev->z
//                t = (pPrev->z - pPrev->y) / d
//                order: x ; y = f ; z = f
//                also interpolates +0x20 and +0x24
//
//  For the 40-byte pair the stride arithmetic is `lea (%eax,%eax,4)` then
//  `,8` and the final pointer difference is a real `cltd ; idiv $0x28`,
//  which is a second confirmation that ppVerts/ppOut are TYPED pointers and
//  that the tail really is `*pnVerts = *ppOut - pStart`.
//
//  Read `de ea` as Intel FSUBP ST(2),ST and `de f1` as FDIVRP ST(1),ST;
//  objdump's AT&T naming inverts both, and getting them backwards inverts
//  the sign of t.
//
// ----------------------------------------------------------------------- //
//  0x1001eb90 (925 bytes, 1 caller) -- /QIfist, same blocker
//
//  Three bare `fistpl`s at 0x1001ecf0, 0x1001ed0d, 0x1001ed2a, each of the
//  form `flds C ; fmuls g ; fstps tmpf ; flds tmpf ; fistpl tmpi ; mov
//  tmpi,%al ; mov %al,0x1008be6x`, i.e. three `(uint8)(float)(C * g)`
//  conversions into adjacent globals.  It also reg-abi-calls, so it is
//  double-blocked.  The tail is a run of eleven zero stores into
//  0x1008be44..0x1008be5c -- the same counter block d3d_ResetCounters
//  touches -- preceded by `rep movsl` of three dwords out of a local, and
//  0x1008b880 is pushed to a helper.
//
// ----------------------------------------------------------------------- //
//  EIGHT ROWS THAT MAKE A REGISTER-ABI CALL, so the CALLER cannot be
//  spelled either.  In each case icl also opens a stack outgoing area it
//  then does not use, so no C call sequence can match.
//
//   0x1006a630 (104)  `lea 0x8(%esp),%edx ; call 0x1006a5f8` with %eax =
//        arg0.  DECODED anyway: it fills a 24-byte local {arg1, arg2, arg3,
//        0, 0, 0}, hands (&it, arg0) to 0x1006a5f8, answers 0 if that
//        fails, else copies 16 bytes from `local[5]->d[0x28]->d[0x34]` to
//        arg4 with `rep movsl` and answers local[5].  Four pushes, `ret`.
//   0x10015c70 (178)  `mov $0x1008b090,%eax ; mov $0x1008b874,%ecx ; mov
//        %esi,%edx ; call 0x10015b70`.  DECODED: __cdecl, one argument;
//        refuses unless obj->d[0x1dc] is non-null and BOTH obj->w[0x1e8]
//        and obj->w[0x1e0] are below obj->d[0x1dc]->d[0x16c]; then calls
//        the three-register helper, and on success 0x10017c14(obj) plus,
//        when 0x1008ca38 is set, a printf of "%s: %d ticks".  Ends with the
//        self-store `mov (%eax),%edx ; mov %edx,(%eax)` on
//        g_1008beb0->d[8], the same no-dead-store idiom sub_1001e210 has.
//   0x10039730 (287)  `mov $0x1,%edx ; call 0x10039850` with %eax = node.
//        DECODED: walks a list hung off arg0 through +0x04/+0x08, and for
//        each node not marked with bit 0 of +0x49 it drains a +0x30 chain
//        through the two-register helper, clears +0x2c, subtracts +0x10
//        from g_1008cbcc->d[0x50], clears g_1008e684/g_1008e688 if the node
//        is either of them, releases +0x0c through COM slot +0x08, unlinks
//        a second list at +0x1c/+0x20, then pushes the node onto the free
//        list at 0x1008e65c after first storing the vtable 0x100894d0 into
//        it and immediately overwriting it -- ANOTHER surviving dead store.
//        Finally ties off the head.
//   0x1002e350 (368)  `mov %edx,%eax ; mov -0x8(%ebp),%edx ; call
//        0x1002dd6c` -- and d3dbatchd.cpp already records 0x1002dd6c as a
//        register-ABI body.
//   0x10039d70 (479)  two of them: `lea -0x98(%ebp),%ecx ; call 0x1006d550`
//        and `mov %ebx,%eax ; xor %ecx,%ecx ; call 0x10039bdc`.
//   0x1000b6c0 (592)  `mov (%ebx),%eax ; call 0x1000b910`, and 0x1000b910
//        opens with `push %eax` to save that incoming register.
//   0x1003ad30 (1168) `mov $0x4520,%eax ; call 0x100805a0` (the allocator
//        takes its size in %eax) and `mov $0x10,%ecx ; call 0x10039210`,
//        which d3dbatcha.cpp already records as a register-ABI entry.  It
//        is the module's big init: 0x4520 bytes of frame, ties off the five
//        pools through 0x1006d520, memsets 0x1008e644, and calls
//        sub_1006d460 above with 0x10038f50 as its allocator.
//   0x1002b0e0's NEIGHBOUR 0x1002b1b0 is the same story; 0x1002b0e0 itself
//        is clean and is written above.
//
// ----------------------------------------------------------------------- //
//  TWO REGISTER-ABI ENTRIES THE FILTER MISSED
//
//   0x10066e90 (120)  first instruction after `sub $0x1c,%esp` is
//        `movzbl 0x94(%eax),%edx` and %eax is never loaded; the caller at
//        0x10067b35 does `mov -0x4(%ebp),%eax ; add $0xfffffff8,%esp ;
//        call`.  DECODED: it is `g_10091100 = obj->b[0x94] * (1/255.0f);
//        g_10091104 = obj->b[0x95] * (1/255.0f); g_10091108 = obj->b[0x96]
//        * (1/255.0f); g_1008be60 = obj->b[0x97];` -- a colour byte-quad
//        expanded to three floats plus a raw alpha byte.  Its caller is
//        another copy of sub_1005a410 (now in d3dr5d.cpp), inlined.
//   0x100256d0 (256)  `cmp $0x1,%eax` against a never-loaded %eax at
//        0x1002579d.  DECODED: it fills the four function-pointer slots
//        0x1008ce94/98/9c/a0 from a cascade over g_1008d96c, g_1008cacc,
//        g_1008caa4 and (g_10089400 && g_1008d978 && (g_1008beac->d[0x180]
//        & 4)), then, if the register argument is 1, forces 0x1008ce9c and
//        0x1008cea0 to 0x1003dc80, and finally forces 0x1008cea0 to
//        0x1003dc80 again when *g_1008beb8 is zero.
//
// ----------------------------------------------------------------------- //
//  THREE ROWS THAT ARE NOT FUNCTION ENTRIES
//
//   0x1001381d  lands MID-INSTRUCTION -- the byte at 0x1001381d is the
//        second half of the `ff db` at 0x1001381c.  The code around it is
//        the same colour-byte lerp the clipper family uses, with a
//        -0x1ea0(%ebp) frame, so it is the interior of a much larger
//        clipper.  Nothing calls 0x1001381d.
//   0x1001df9b  interior of the stdlith CMoArray grow path that begins at
//        0x1001dee0 (called from 0x1001e032, which supplies g_10091df0).
//        The visible fragment is the widen-and-copy loop over 16-bit
//        elements plus the free/realloc pair 0x1000a670 / 0x1000a680.
//        Nothing calls 0x1001df9b.
//   0x10050f20  interior of a 5KB function; the visible fragment is yet
//        another inlined copy of the clipper (`incl 0x1008be44`, the same
//        outcode loop, the same lerp) against a constant z plane held in
//        g_1008b908.  Nothing calls 0x10050f20.
//
// ----------------------------------------------------------------------- //
//  TWO ROWS DECODED BUT NOT WRITTEN
//
//   0x10020e80 (175 bytes, 1 caller 0x10020d0d) -- a projection setup.
//        Behind the same stack-realignment prologue sub_1001e210 has, with
//        five stack arguments:
//
//            p = arg0;
//            p->f[0x14] = arg1;   p->f[0x18] = arg2;
//            p->f[0x00] = 0;      p->f[0x04] = 0;   p->f[0x08] = 1.0f;
//            p->f[0x0c] = (float)tan(1.5707964f - 0.5f * arg3);
//            p->f[0x10] = (float)tan(1.5707964f - 0.5f * arg4);
//
//        i.e. cot(fovX/2) and cot(fovY/2); the constants at 0x100a5394 are
//        {PI/2, 0.5, PI/2, 0.5}.  NOT WRITTEN because the tangent is two
//        CRT calls, 0x1007fe30 then 0x1007fdd0, with the quotient taken as
//        `fdivrl` -- so either the source is sin(x)/cos(x) or icl expanded
//        tan() that way, and both spellings are guesses.  Both callees are
//        above 0x1007fa10 and take one double at (%esp).  The store order
//        is also scheduling-dominated: +0x18 and +0x14 are emitted early,
//        into the FP latency, and +0x08/+0x04/+0x00 come out last in that
//        order with the second tangent.
//
//   0x1002ec60 (724 bytes, 1 caller 0x1002f08c) -- per-vertex dynamic
//        lighting in DOUBLE precision, and the one row in this batch that
//        has no ABI or flag problem and was still declined.  It is worth
//        knowing that it does the FULL control-word dance around its three
//        `fistpll`s, which is what our flags emit, so it is not a /QIfist
//        function; the reason it is not here is that it keeps six or seven
//        live values on the x87 stack through a dozen `fxch`es, spills
//        eight doubles to 0x30..0x80(%esp), and calls the CRT's double sqrt
//        at 0x1007fc98 -- the schedule, not the algorithm, is the work.
//        What it does: obj->d[0x2c] gives a light list; for each light it
//        copies three doubles out, forms the squared distance to the vertex
//        against the light radius at +0x1a8, and on a hit adds
//        (1 - dist/radius)-weighted colour into three ints taken from the
//        vertex's own +0x10/+0x11/+0x12 bytes, doubling them when
//        g_1008cb98 is set and clamping each at 255.
//
// *********************************************************************** //


// ======================================================================= //
//  0x10035c30   489 bytes.  RESET THE TEXTURE STAGES AND SET UP A
//                           DIFFUSE-ALPHA BLEND FOR THIS SURFACE.
//
//  IT IS A __thiscall MEMBER, and that is not inferred from the missing
//  argument -- its address is the FOURTH entry of the eleven-pointer run
//  at .data 0x10089480, which is a VTABLE: 0x10035c00 (slot +0x08) is
//  `mov 0x4(%ecx),%ebx; ... ret $0x8` and 0x10035bd0 (slot +0x1c) is
//  `mov 0xc(%ecx),%eax; if(p) p->lpVtbl->Release(p); +0xc = +0x8 = +0x4 =
//  0`, so the whole run is %ecx-taking members of one class and the class
//  is { +0x00 vptr, +0x04 w, +0x08 h, +0x0c texture }.  This function
//  reads only +0x0c.
//
//  THE D3D7 CALLS, all on g_pD3DDevice (0x1008d988):
//    +0x50 SetRenderState        (IDirect3DDevice7 slot 20)
//    +0x8c SetTexture            (slot 35)
//    +0x94 SetTextureStageState  (slot 37)
//  and every literal below is in the disassembly:
//    stage loop 0..7:  SetTexture(i, NULL)
//                      SetTextureStageState(i, 1, 1)   COLOROP  = DISABLE
//                      SetTextureStageState(i, 4, 1)   ALPHAOP  = DISABLE
//    then on stage 0:  SetTexture(0, m_pTexture)
//                      (0, 0x0c, 3)   ADDRESS   = CLAMP
//                      (0, 2,    2)   COLORARG1 = TEXTURE
//                      (0, 3,    0)   COLORARG2 = DIFFUSE
//                      (0, 1,  0x0c)  COLOROP   = BLENDDIFFUSEALPHA
//                      (0, 5,    2)   ALPHAARG1 = TEXTURE
//                      (0, 6,    0)   ALPHAARG2 = DIFFUSE
//                      (0, 4,    3)   ALPHAOP   = SELECTARG2
//                      SetRenderState(0x1b, 1)  ALPHABLENDENABLE = TRUE
//                      SetRenderState(0x13, 1)  SRCBLEND  = ZERO
//                      SetRenderState(0x14, 3)  DESTBLEND = SRCCOLOR
//
//  WHAT PINS THE SHAPE:
//   * `mov 0xc(%ecx),%esi` is executed ONCE, before the loop, and parked
//     in -0x4(%ebp) across it -- one member read into a local, not a
//     member expression at the point of use (which would reload, since
//     the intervening indirect calls can alias).
//   * `mov 0x1008d988,%ebx` is likewise executed ONCE and %ebx feeds all
//     FOURTEEN calls, while `mov (%ebx),%eax` (the vtable) is reloaded
//     at every one of them.  icl cannot hoist a global pointer across
//     indirect calls, so the DEVICE is a local in the source and the
//     vtable is not -- writing `g_pD3DDevice->lpVtbl->X(g_pD3DDevice,..)`
//     the way d3dbatchc.cpp does would reload the global fourteen times.
//     The local is read AFTER the member, which is the declaration order
//     below.
//   * `xor %edi,%edi` / `inc %edi` / `cmp $0x8,%edi` / `jl` is a SIGNED
//     `for (i = 0; i < 8; i++)`; `jb` would say unsigned.
//   * the three calls in the loop all pass %esi, which is reloaded from
//     %edi at the top of every iteration -- the loop counter itself is
//     the stage index, no separate variable.
//   * the single exit's `xor %eax,%eax; inc %eax` is `return TRUE`.
//
//  FLAGS: moved here from d3dr5d.cpp (/O2), where it was 512 bytes with
//  477 differing: `xor %eax,%eax; inc %eax` for 1, pushed callee-saved
//  registers and the unaligned loop head are all /Os, and at /Os the
//  source below is byte-identical with no other change.
//
//  The blend values: 0x13 <- 1 is SRCBLEND = D3DBLEND_ZERO and 0x14 <- 3
//  is DESTBLEND = D3DBLEND_SRCCOLOR (d3d.h: ZERO 1, ONE 2, SRCCOLOR 3,
//  INVSRCCOLOR 4, SRCALPHA 5).  The d3dr5d.cpp copy named the 3
//  "SRCALPHA"; the value, which is all the bytes show, is unchanged.
// ======================================================================= //

#define D3DTSS_ALPHAOP                         4
#define D3DTSS_ALPHAARG1                       5
#define D3DTSS_ALPHAARG2                       6
#define D3DTSS_ADDRESS                        12

#define D3DTOP_DISABLE                         1
#define D3DTOP_SELECTARG2                      3
#define D3DTOP_BLENDDIFFUSEALPHA              12

#define D3DTADDRESS_CLAMP                      3

#define D3DRENDERSTATE_ALPHABLENDENABLE       27        // 0x1b

#define D3DBLEND_ZERO                          1
#define D3DBLEND_SRCCOLOR                      3

class C10035c30
{
public:
	uint32 m_pVTable;                                   // +0x00
	uint32 m_nWidth;                                    // +0x04
	uint32 m_nHeight;                                   // +0x08
	void*  m_pTexture;                                  // +0x0c

	int32  sub_10035c30();                              // 0x10035c30
};

int32 C10035c30::sub_10035c30()
{
	void*       pTexture = m_pTexture;
	D3DDevice7* pDevice  = g_pD3DDevice;
	int32       i;

	for (i = 0; i < 8; i++)
	{
		pDevice->lpVtbl->SetTexture(pDevice, i, LTNULL);
		pDevice->lpVtbl->SetTextureStageState(pDevice, i,
			D3DTSS_COLOROP, D3DTOP_DISABLE);
		pDevice->lpVtbl->SetTextureStageState(pDevice, i,
			D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	pDevice->lpVtbl->SetTexture(pDevice, 0, pTexture);

	pDevice->lpVtbl->SetTextureStageState(pDevice, 0,
		D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
	pDevice->lpVtbl->SetTextureStageState(pDevice, 0,
		D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pDevice->lpVtbl->SetTextureStageState(pDevice, 0,
		D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	pDevice->lpVtbl->SetTextureStageState(pDevice, 0,
		D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA);
	pDevice->lpVtbl->SetTextureStageState(pDevice, 0,
		D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pDevice->lpVtbl->SetTextureStageState(pDevice, 0,
		D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	pDevice->lpVtbl->SetTextureStageState(pDevice, 0,
		D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);

	pDevice->lpVtbl->SetRenderState(pDevice,
		D3DRENDERSTATE_ALPHABLENDENABLE, 1);
	pDevice->lpVtbl->SetRenderState(pDevice,
		D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
	pDevice->lpVtbl->SetRenderState(pDevice,
		D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);

	return 1;
}
