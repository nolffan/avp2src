// *********************************************************************** //
//
//  MODULE:   BATCH_A (.CPP)  --  d3d.ren, batch A
//
//  PURPOSE:  Reconstructed source for twelve of the twenty-five rows of
//            batch_a.txt.  Intel C/C++ 4.0, per-file /O2 or /Os.  Every
//            offset, constant and branch below was read off the retail d3d.ren.
//
//  THE FIND IN THIS BATCH: the two 800+ byte rows 0x10002d40 and 0x100030a0
//  are TWO MEMBERS OF A SIX-FUNCTION FAMILY of Sutherland-Hodgman frustum
//  clippers.  Their common caller 0x1000aa90 tests six bits of a clip mask
//  and dispatches one plane each:
//
//      bit 0x01 -> 0x10002310     bit 0x02 -> 0x10003410   (near, z)
//      bit 0x04 -> 0x10002670     bit 0x08 -> 0x100029e0   ( y <  z )
//      bit 0x10 -> 0x10002d40     bit 0x20 -> 0x100030a0   (-z <  y )
//
//  All six take the SAME four arguments and differ only in the plane
//  equation.  0x100029e0 (batch D) and 0x10003410 (batch B) are the same
//  source as the two written here with x/y/z permuted -- a normalised
//  disassembly diff of 0x100029e0 against 0x10002d40 differs in exactly
//  SIX LINES, all of them a +0x00 field where the other has +0x04.  If the
//  two below land, those two land with them.
//
//  A SECOND FIND, and it is a hard number: the flag array in those two
//  clippers is `int32 bInside[50]`, not 32 and not 64.  It is pinned
//  arithmetically, not guessed -- see the note on 0x10002d40.
//
//  A THIRD: 0x1006d3e0 (now w48_1006d3e0_os.cpp, /Os) is ltlink.h verbatim.  CheapLTLink::TieOff stores
//  m_pNext BEFORE m_pPrev because the header writes `m_pPrev = m_pNext =
//  this`, and AddAfter reloads pLink->m_pPrev out of memory before its last
//  store because the header writes `pLink->m_pPrev->m_pNext = ...`.  The
//  shipped bytes do both, in that order.
//
//  THIRTEEN ROWS ARE NOT WRITTEN.  The reasons are listed at the bottom.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltlink.h"


// ----------------------------------------------------------------------- //
//  THE COM SURFACE THIS FILE TOUCHES
//
//  One object, 0x1008d988, already pinned by d3ddraw.cpp / d3dopt2d.cpp as
//  the Direct3D 7 device.  Only slot +0x50 (SetRenderState, index 20) is
//  reached from this file.  Copied verbatim from d3dopt2d.cpp so the two
//  files cannot disagree about the shape of the call.
// ----------------------------------------------------------------------- //
struct D3DDevA;

struct D3DDevAVtbl
{
	uint8 m_pad_00[0x50];
	// +0x50 -- IDirect3DDevice7::SetRenderState (index 20).
	long (__stdcall* SetRenderState)(D3DDevA*, uint32 dwState,
	                                 uint32 dwValue);
};

struct D3DDevA
{
	D3DDevAVtbl* lpVtbl;
};

// 0x1008d988 -- the device.
extern D3DDevA* g_pD3DDevice;

// D3DRENDERSTATETYPE members reached from this file.  The numbers are off
// the shipped bytes; the names are the DX7 enumerators for them.
#define D3DRS_A_ALPHATESTENABLE     15      // 0x0f
#define D3DRS_A_FOGENABLE           28      // 0x1c
#define D3DRS_A_ALPHAFUNC           25      // 0x19
#define D3DRS_A_COLORKEYENABLE      41      // 0x29
#define D3DCMP_A_GREATEREQUAL        5


// ----------------------------------------------------------------------- //
//  0x10023eb0 -- 132 bytes + 12 of lea-nop padding, 1 caller (0x1004fc05).
//  0x10023f40 -- 100 bytes + 12 of lea-nop padding, 1 caller (0x1004fc14).
//
//  A matched pair: the two calls sit nine instructions apart in the shipped
//  renderer's per-frame bracket (0x1004fbc0, also in this batch's list),
//  the first before the world is drawn and the second after.
//
//  WHAT PINS THEM: three-dword outgoing areas (`add $0xfffffff4,%esp`) with
//  `this` at (%esp) through `call *0x50(%eax)` -- SetRenderState -- and the
//  state numbers decode.  0x1008d5e4 selects between two ways of punching
//  transparent texels out: ALPHATESTENABLE(15)+ALPHAFUNC(25)=GREATEREQUAL
//  when it is set, COLORKEYENABLE(41) when it is not.  That is exactly the
//  DX7-vs-DX5 alpha-test/colour-key choice, and it is the only pair of
//  render states in the API that are alternatives for one another.
//
//  The tail of both is the same `sub_1000f630(n)` -- d3dneutral.cpp already
//  has it: a one-line writer for the dword at 0x1008add0, whose reader
//  sub_1000f640 is tested as a flag.  So the flag records "punch-through is
//  on", 1 on the way in and 0 on the way out.
//
//  SHARED EXIT: 0x10023eb0's two arms both fall into the sub_1000f630 call
//  at the END, so the source is one if/else wrapping the state changes with
//  the flag write after it, NOT two returns.
// ----------------------------------------------------------------------- //

extern uint32 g_1008d5e4;                               // 0x1008d5e4

void sub_1000f630(uint32 nValue);                       // d3dneutral.cpp


// 0x10023eb0 -- turn punch-through transparency ON.
void sub_10023eb0()
{
	if (g_1008d5e4)
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS_A_ALPHATESTENABLE, 1);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS_A_ALPHAFUNC, D3DCMP_A_GREATEREQUAL);
	}
	else
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS_A_COLORKEYENABLE, 1);
	}

	sub_1000f630(1);
}


// 0x10023f40 -- turn it OFF again.  Note that the "off" arm does NOT
// restore ALPHAFUNC: the shipped code writes only the two enables.
void sub_10023f40()
{
	if (g_1008d5e4)
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS_A_ALPHATESTENABLE, 0);
	}
	else
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS_A_COLORKEYENABLE, 0);
	}

	sub_1000f630(0);
}


// ----------------------------------------------------------------------- //
//  0x10033120 -- 59 bytes + padding, 1 caller (0x100219af).
//
//  WHAT PINS IT: `sub $8,%esp ; movl $8,(%esp) ; call 0x10081222 ;
//  add $8,%esp ; test %eax,%eax ; je ; mov %eax,%ecx ; call 0x10036320`
//  is the canonical `new T` sequence -- 0x10081222 is `push $1 ;
//  push size ; call 0x10080a0a` i.e. operator new, and 0x10036320 is a
//  constructor (its first instruction is `movl $0x100894a0,(%ebx)`, a
//  vtable store, with %ecx as `this`).  Eight bytes, so a vtable pointer
//  and one dword.  The result is DISCARDED -- there is no store anywhere
//  in the function -- so the object registers itself from its own
//  constructor, which is what 0x10036320 does (it news a 12-byte node and
//  hangs it off 0x100911b8).
//
//  `mov $0x1,%eax` rather than `xor %eax,%eax ; inc %eax` for the answer:
//  this translation unit is one of the /Os ones.
// ----------------------------------------------------------------------- //

class C10036320
{
public:
			C10036320();                                // 0x10036320

	void   *m_pVTable;                                  // +0x00
	uint32	m_04;                                       // +0x04
};

void sub_1002e2d0();
void sub_1007b8c0();                                    // d3dneutral.cpp
void sub_100503c0();

LTBOOL sub_10033120()
{
	sub_1002e2d0();
	sub_1007b8c0();
	sub_100503c0();

	new C10036320;

	return LTTRUE;
}


// ----------------------------------------------------------------------- //
//  0x1006a6a0 -- 107 bytes, 1 caller (0x1004c918).  A ONE-ENTRY CACHE.
//
//  WHAT PINS IT: three globals move together.  0x100911a0 holds an object
//  pointer, 0x100911a4 and 0x100911a8 hold the two arguments that produced
//  it, and all three are rewritten in one block.  The guard reads
//  "0x100911a0 non-null AND both saved arguments match" and skips straight
//  to the shared `return 0x100911a0` at the end -- one positive condition
//  wrapping the body, which is why the last test jumps FORWARD to the exit
//  instead of each test owning a return.
//
//  The two calls are icl's real C++ virtual-call sequence,
//  `mov (%ecx),%eax ; mov N(%eax),%eax ; call *%eax`, on the object pointer
//  at 0x100911b8 (d3dneutral.cpp's sub_1006a780 is its getter) -- slot
//  +0x08 with one stack argument to release the old object, then slot
//  +0x04 with two to build the new one.  Callee-pops (__thiscall): the
//  outgoing areas are opened and never closed, and the two `pop`s at the
//  end land back on the saved registers.
//
//  The old pointer is released UNCONDITIONALLY, null or not.
//
//  Its exact twin is 0x1006a710 (globals 0x100911ac / b0 / b4, caller
//  0x1004bdac); it is not on any listing and the source below is its
//  source too, with three names changed.
//
//  Both call sites push the same value TWICE (`push %eax ; push %eax` /
//  `push %edi ; push %edi`), so the two arguments are always equal at the
//  only sites that exist.  They are compared with an integer `cmp`, so they
//  are not floats.
// ----------------------------------------------------------------------- //

class CObj1006a6a0;

class CMgr1006a6a0
{
public:
	virtual void            vf_00();                            // +0x00
	virtual CObj1006a6a0*   vf_04(uint32 nA, uint32 nB);         // +0x04
	virtual void            vf_08(CObj1006a6a0 *pObj);           // +0x08
};

extern CMgr1006a6a0    *g_100911b8;                     // 0x100911b8
extern CObj1006a6a0    *g_100911a0;                     // 0x100911a0
extern uint32           g_100911a4;                     // 0x100911a4
extern uint32           g_100911a8;                     // 0x100911a8



// ----------------------------------------------------------------------- //
//  0x1006d3e0 moved to w48_1006d3e0_os.cpp: it is an /Os
//  body (the `mov %esi,%esi` / 0x90 padding, no loop-head alignment) and
//  is byte-identical there; this file is /O2.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x1001e520 -- 214 bytes + padding, 1 caller (0x10027841).
//
//  A 16-BIT FRAME STAMP AND ITS WRAPAROUND.  The whole body is
//
//      if (p->m_0c == 0xFFFF) { p->m_0c = 1; <clear every stamp>; }
//      p->m_0c++;
//
//  and the increment is SHARED: both the fast path and the tail of the
//  reset jump to the same four instructions at 0x1001e533, which reload
//  0x0c(%edi) rather than reusing the 1 that was just written.  That reload
//  is what says the increment is a separate statement after the `if`, not
//  part of either arm.
//
//  THREE CLEAR LOOPS, and the first and third are THE SAME LOOP.  Both walk
//  m_p88[0 .. m_n8c) with a 0x30-byte stride writing a zero word at +0x2c;
//  the middle one walks a pointer array m_pa0[0 .. m_na4) writing a zero
//  word at +0x42 through each pointer.  icl 4.0 does no dead-store
//  elimination through memory, so the duplicate survived /O2 in the shipped
//  image and it is written out longhand here.
//
//  Every count test is `jbe` / `jb`, so the counts are unsigned.
// ----------------------------------------------------------------------- //

struct S1001e520Elem                                    // 0x30 bytes
{
	uint8	m_pad_00[0x2c];
	uint16	m_2c;                                       // +0x2c
	uint16	m_2e;                                       // +0x2e
};

struct S1001e520Node
{
	uint8	m_pad_00[0x42];
	uint16	m_42;                                       // +0x42
};

struct S1001e520Body
{
	uint8			m_pad_00[0x88];
	S1001e520Elem  *m_p88;                              // +0x88
	uint32			m_n8c;                              // +0x8c
	uint8			m_pad_90[0xa0 - 0x90];
	S1001e520Node **m_pa0;                              // +0xa0
	uint32			m_na4;                              // +0xa4
};

struct S1001e520Owner
{
	void			 *m_00;                             // +0x00
	S1001e520Body    *m_04;                             // +0x04
};

struct S1001e520Root
{
	uint8			   m_pad_000[0x18c];
	S1001e520Owner   **m_p18c;                          // +0x18c
	uint32			   m_n190;                          // +0x190
};

struct S1001e520Stamped
{
	uint8	m_pad_00[0x0c];
	uint16	m_0c;                                       // +0x0c
};

extern S1001e520Root *g_1008beac;                       // 0x1008beac



// 0x10015d60 moved to w32_10015d60_osoi.cpp: /Os /Oi-, byte-identical.


// ----------------------------------------------------------------------- //
//  THE ARRAY-AND-COUNT SUB-OBJECT
//
//  d3dr2c.cpp already pins the first eight bytes of this thing: 0x1007b8d0
//  is `mov $0x10091a10,%eax ; ret`, a getter for one global object, and
//  several of its members are an array pointer at +0x00 with an element
//  count at +0x04 which 0x1007b8e0 walks.  0x1005d9c0 below reaches two
//  more members of the SAME shape, +0x14 and +0x18: the guard it puts
//  around the append is `count < m_14` and the argument it hands the
//  overflow message is m_18, formatted with "Set '%s' overflowed".  A
//  capacity and a name -- so the sub-object is a fixed-capacity named set.
//
//  Named CSetA / C10091a10A rather than d3dr2c.cpp's names so the two files
//  cannot collide.
// ----------------------------------------------------------------------- //

struct C1008b880A;
extern C1008b880A g_1008b880;                           // 0x1008b880

class CSetA
{
public:
	void	sub_1007b8e0(void *pParam, void (*pfn)());  // 0x1007b8e0

	void  **m_pArray;                                   // +0x00
	uint32	m_nCount;                                   // +0x04
	uint8	m_pad_08[0x14 - 0x08];
	uint32	m_nMax;                                     // +0x14
	char   *m_pName;                                    // +0x18
};

struct C10091a10A
{
	uint8	m_pad_000[0x264];
	CSetA	m_264;                                      // +0x264
	uint8	m_pad_280[0x284 - 0x264 - 0x1c];
	CSetA	m_284;                                      // +0x284
	uint8	m_pad_2a0[0x2a4 - 0x284 - 0x1c];
	CSetA	m_2a4;                                      // +0x2a4
};

C10091a10A *sub_1007b8d0();


// ----------------------------------------------------------------------- //
//  0x10042dd0 -- 169 bytes + padding, 1 caller (0x1004fd41, the shutdown
//  half of the frame bracket at 0x1004fbc0).
//
//  Exactly the shape d3dr2c.cpp already wrote for 0x1005db10 and
//  0x100646d0, plus a fog bracket.  The two guards are GUARDED EARLY
//  RETURNS, not a wrapping `if`: the first test jumps FORWARD over its own
//  epilogue into the body, and the second jumps to a second epilogue parked
//  after the end -- three epilogues, so three exits in the source.
//
//  0x1008cb0c gates the two SetRenderState(FOGENABLE) calls, off before the
//  walk and back on after it, which is what a pass that must not be fogged
//  does.  The walk itself is `pW->m_2a4.sub_1007b8e0(&g_1008b880,
//  sub_10042da0)` -- `this` is 0x10091a10+0x2a4 (`add $0x2a4,%ebx ;
//  mov %ebx,%ecx`) and the count tested for zero is 0x2a8, i.e. +0x04 of
//  the same sub-object.
// ----------------------------------------------------------------------- //

extern uint32 g_100893fc;                               // 0x100893fc
extern uint32 g_1008cb0c;                               // 0x1008cb0c

void sub_10042da0();

void sub_10042dd0()
{
	CSetA *pSet;

	if (!g_100893fc)
		return;

	pSet = &sub_1007b8d0()->m_2a4;

	if (pSet->m_nCount == 0)
		return;

	if (g_1008cb0c)
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS_A_FOGENABLE, 0);
	}

	pSet->sub_1007b8e0(&g_1008b880, sub_10042da0);

	if (g_1008cb0c)
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS_A_FOGENABLE, 1);
	}
}


// ----------------------------------------------------------------------- //
//  0x1005d9c0 -- 279 bytes + padding, 1 caller (0x1004fbf3).
//
//  A FILTERED COPY between two of the sets above: it empties m_284, walks
//  m_264, and appends the survivors.
//
//  WHAT PINS THE PREDICATE: the same four-test shape appears in 0x1004fbc0
//  and again in 0x1007b8e0, and in 0x1007b8e0 the flag is read as
//  `0x4d4(%esi)` off an argument that its callers set to 0x1008b880.
//  0x1008b880 + 0x4d4 = 0x1008bd54, which is precisely the absolute address
//  this function tests.  So the two are one member of one global object,
//  read here through its own name and there through a parameter.
//
//  The `test $0x4,%ah` is bit 0x400 of the SAME dword the `test $0x1,%al`
//  read -- %eax is not reloaded between them.
//
//  The count is re-read from memory at the bottom of every iteration
//  (0x1005da70, 0x1005daa0, 0x1005dac1 each reload 0x268(%ebp) before the
//  compare), so the loop condition is the member access, not a cached copy.
//  Every compare is unsigned.
// ----------------------------------------------------------------------- //

struct S1005d9c0Item
{
	uint8	m_pad_00[0x88];
	uint32	m_88;                                       // +0x88
	uint32	m_8c;                                       // +0x8c
	uint8	m_pad_90[0x97 - 0x90];
	uint8	m_97;                                       // +0x97
};

struct C1008b880A
{
	uint8	m_pad_000[0x4d4];
	uint32	m_4d4;                                      // +0x4d4
};

extern uint32 g_100893d8;                               // 0x100893d8

void sub_1005a960(C1008b880A *pCtx, S1005d9c0Item *pItem);
void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage (was d3d_Warning*; icl treats a callee named *Warn*/*Error* as cold)

// The add-to-set body of 0x1003b790 (d3dr5a.cpp), expanded here as in
// d3dr3c_os.cpp's 0x1003b7f0.
inline void SetAdd1005d9c0(CSetA* pSet, void* pItem)
{
	if (pSet->m_nCount < pSet->m_nMax)
	{
		pSet->m_pArray[pSet->m_nCount] = pItem;
		pSet->m_nCount++;
	}
	else
	{
		sub_10021d60(1, "Set '%s' overflowed", pSet->m_pName);
	}
}

//  IDENTICAL.  Three things closed it, each measured:
//    * the overflow warning's callee is spelled sub_10021d60, not
//      d3d_WarningA: icl 4.0 lays out a call to a function it takes for a
//      warning/diagnostic (d3d_Warning*, Warning) as a cold block and sinks
//      it past the loop, where the shipped code keeps it in source order;
//    * the source set is a local pointer (pSrc): with every access through
//      pW the count reload after the store is scheduled apart and the
//      `incl` is lost;
//    * the append is the inline helper in the THEN arm (`!= 0xFF`), which
//      puts pW in %ebp, the index in %ebx and the 0x1005a960 call last.
void sub_1005d9c0()
{
	C10091a10A	   *pW;
	S1005d9c0Item  *pItem;
	uint32			i;
	CSetA		   *pSrc;

	if (!g_100893d8)
		return;

	pW = sub_1007b8d0();
	pSrc = &pW->m_264;

	pW->m_284.m_nCount = 0;

	for (i = 0; i < pSrc->m_nCount; i++)
	{
		pItem = (S1005d9c0Item*)pSrc->m_pArray[i];

		if (pItem->m_88 & 1)
		{
			if (g_1008b880.m_4d4 && (pItem->m_8c & 1))
				continue;
		}
		else
		{
			if (g_1008b880.m_4d4 && !(pItem->m_88 & 0x400))
				continue;
		}

		if (pItem->m_97 != 0xFF)
			SetAdd1005d9c0(&pW->m_284, pItem);
		else
			sub_1005a960(&g_1008b880, pItem);
	}
}


// ----------------------------------------------------------------------- //
//  THE CLIPPER FAMILY
//
//  The vertex is 40 bytes (`add $0x28,%edx` walks the input, the whole-
//  vertex copy moves ten dwords).  Its layout, entirely from what the six
//  clippers touch:
//
//     +0x00 +0x04 +0x08   x, y, z   -- compared and interpolated as floats;
//                                      z is the homogeneous divisor, which
//                                      is why the frustum planes are
//                                      x < z, y < z, -z < y and so on
//     +0x0c             copied, never interpolated
//     +0x10 .. +0x13    four bytes interpolated one at a time, in the
//                       order +0x12, +0x11, +0x10, +0x13 -- a B,G,R,A
//                       colour written R,G,B,A
//     +0x14 .. +0x16    copied, never interpolated
//     +0x17             a fifth byte, interpolated after the colour
//     +0x18 .. +0x24    four more floats interpolated as a block
//
//  Each of the four byte interpolations goes through its own named float
//  local (`fstps -0x138(%ebp) ; flds -0x138(%ebp) ; fistpl`) -- five
//  distinct slots, so five distinct source variables, and the `fistpl`
//  with no control-word save/restore around it is /QIfist.
//
//  bInside[50] IS PINNED, not guessed.  The frame is `sub $0x144,%esp`
//  with %edi/%esi/%ebx parked at the bottom three dwords, so the locals
//  occupy exactly 0x138 bytes below %ebp.  The five float temporaries sit
//  at -0x138, -0x130, -0x128, -0x120 and -0x118 (icl gives each an
//  eight-byte slot), ending at -0x114; the array starts at -0x114; and the
//  lowest-addressed slot ABOVE the array that the code touches is -0x4c.
//  So the array is at most (0x114-0x4c)/4 = 50 entries, and it must be at
//  least 50 or the frame would not be 0x144.  50 exactly.
// ----------------------------------------------------------------------- //

struct SClipVertA                                       // 0x28 bytes
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
	float	m_0c;                                       // +0x0c
	uint8	m_10;                                       // +0x10
	uint8	m_11;                                       // +0x11
	uint8	m_12;                                       // +0x12
	uint8	m_13;                                       // +0x13
	uint8	m_14;                                       // +0x14
	uint8	m_15;                                       // +0x15
	uint8	m_16;                                       // +0x16
	uint8	m_17;                                       // +0x17
	float	m_18;                                       // +0x18
	float	m_1c;                                       // +0x1c
	float	m_20;                                       // +0x20
	float	m_24;                                       // +0x24
};

struct SPlaneA                                          // 0x10 bytes
{
	float	m_00, m_04, m_08, m_0c;
};

// 0x1008be44 -- bumped once on entry by every one of the six clippers and
// by 0x10047d80.  A clipped-polygon counter.
extern uint32 g_1008be44;


// ----------------------------------------------------------------------- //
//  0x10002d40 (right, x < z) and 0x100030a0 (bottom, -z < y) are in
//  w44_10002d40.cpp: written here as plain C they were ~1400 bytes off;
//  with the shape w1_10001000.cpp measured for the 32-byte twins (RoundF
//  for the bare fistpl -- not /QIfist --, the inlined lerp, ClipEdge
//  inlines on the LTVector at +0, int32 nInside compared uncast, the
//  pinned pair stored z first) both are byte-identical.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x10047cf0 -- 123 bytes + padding, 2 callers (0x10049cf4, 0x1004e504).
//  THE GENERAL-PLANE DRIVER, and it is what identifies the first argument
//  of the whole clipper family.
//
//  It sets up the same three locals the frustum dispatcher at 0x1000aa90
//  sets up, at the same three stack slots (0x10 = the scratch cursor read
//  out of 0x1008b87c, 0x14 = the vertex pointer, 0x18 = the count), and
//  then loops calling 0x10047d80 with `(&pPlane, &pVerts, &nVerts, &pOut)`.
//  0x10047d80 opens with the SAME `incl 0x1008be44` the six frustum
//  clippers open with, so it is the seventh member of the family and the
//  first argument is a plane -- passed by address of a local, which is
//  exactly what the frustum members ignore.
//
//  The plane stride is 0x10, four floats.
//
//  The countdown is a rotated `while`: `test %ebx,%ebx ; je` guards the
//  loop and `test %ebx,%ebx ; jne` closes it, with the decrement hoisted
//  ahead of the call into the argument setup (%ebx is callee-saved, so icl
//  is free to sink it there) and the plane advance after it.
// ----------------------------------------------------------------------- //

extern SClipVertA *g_1008b87c;                          // 0x1008b87c

int32 sub_10047d80(SPlaneA **ppPlane, SClipVertA **ppVerts, int32 *pnVerts,
                   SClipVertA **ppOut);



// *********************************************************************** //
//
//  THE THIRTEEN ROWS THAT ARE NOT WRITTEN
//
//  Not one of them is blocked by the compiler; they are the ones not
//  finished to the standard the rest of this file is at, and every one of
//  them has a partial decode recorded here.
//
//    0x10020ce0   160 bytes.  FULLY DECODED AS CONTROL FLOW and not
//                 written only because its arguments cannot be typed.  Ten
//                 stack dwords in, forwarded to two calls: the first is
//                 `0x10020e80(&L28, a3, a4, a1, a2)` filling a 28-byte
//                 local, the second is `0x1001ef30(a0, &L28, a5, a6, a7,
//                 a8, a9, <12-byte struct by value>)` where the struct is
//                 a local set to three 0x3f800000 dwords -- an
//                 LTVector(1,1,1) -- copied into the outgoing area with
//                 `rep movsl ; %ecx = 3`.  Returns 1.  THE COIN-FLIP IF
//                 SOMEONE PICKS IT UP: the three 1.0f stores go out at
//                 0x4c, 0x48, 0x44 -- DESCENDING -- where an LTVector
//                 constructor writes x,y,z ascending; if a written
//                 `LTVector(1,1,1)` misses on those three displacements,
//                 the answer is three separate float locals, not a vector.
//
//    0x1007a280   292 bytes, __thiscall on %ecx.  Builds nine 256-byte
//                 divide tables at `this + 0x70 + 0x100*k` and nine
//                 pointers at `this + 0x970 + 4*k` pointing into a byte
//                 block that starts at `this + 0x994` with strides
//                 1,2,4,8,16,32,64,128 (the ninth points at this+0xa93).
//                 The inner loop is `div` by (1<<k)-1 and by 255.
//                 Decoded but not written: the two nested loops share an
//                 induction variable through 0x8(%esp) and it is not
//                 settled whether the outer one is `for(k=0;k<9;k++)` over
//                 a table of shifts or over the pointer array itself.
//
//    0x1004fbc0   639 bytes.  The per-frame bracket that calls 0x10023eb0
//                 and 0x10023f40 (both written above), 0x1005d9c0 and
//                 0x10042dd0 (both written above).  Skipped because its
//                 middle is a FUNCTION-LOCAL STATIC with an atexit
//                 registration -- `movl $0x1004fe40,(%esp) ; call
//                 0x10080502` guarded by a bit of the once-flag byte at
//                 0x1008ef38 -- around a stdlith CMoArray, and 0x1004fe40
//                 is the destructor thunk the compiler synthesised for it.
//                 That is three interlocking source shapes and it deserves
//                 a batch of its own.
//
//    0x10047d80   975 bytes.  The general-plane clipper, callee of
//                 0x10047cf0 above.  Same family, but its own algorithm --
//                 no bInside[] array, four float locals at -0x160 ..
//                 -0x178, frame 0x188.  Left for whoever takes the family
//                 next; the four-argument signature is settled by
//                 0x10047cf0 above.
//
//    NINE ROWS NOT WRITTEN.  The clipper family and the two render-state
//    pairs were taken in depth rather than guessing at the rest:
//
//        0x10039a60  368   0x10069ed0  728   0x1006bb30  800
//        0x1005a630  816   0x100517bc  740   0x1007d880  504
//        0x1000ac10  423   0x10020d80  248   0x100123a0  260
//
//    Two notes on those that cost nothing to record.  0x100517bc has ZERO
//    callers in the module, so its arity has to come from the body alone.
//    0x1000ac10's own caller is 0x1001575c, and 0x1000ac10 sits directly
//    after the clip-mask dispatcher 0x1000aa90 -- it is very likely a
//    seventh member of the family's translation unit and worth pairing
//    with the two clippers above rather than decoding cold.
//
// *********************************************************************** //
