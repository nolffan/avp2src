// *********************************************************************** //
//
//  MODULE:   D3DR5C (.CPP)  --  d3d.ren "batch3"
//
//  PURPOSE:  24 of the 25 rows of batch3.txt (148..276 bytes), decoded
//            instruction by instruction from the retail d3d.ren.  Intel C/C++
//            4.0.  Every body below carries a banner saying what pins it.
//
//  WHAT THIS BATCH IS.  Three families plus eleven singletons.
//
//  FAMILY 1 -- stdlith's CMoArray<T,C> / GenList<T> again, the same
//  template d3dr4a.cpp and d3dr4b.cpp already carry.  Four rows:
//
//      0x1007f810   CMoArray<T,C>::SetSize2(DWORD,LAlloc*)   T = 4 bytes
//      0x10050284   an EnsureSize wrapper whose SetSize2 is INLINED, T = 12
//      0x1007c480   GenList<T>::GenAppendList  (slot +0x24)   T = 4 bytes
//      0x10050560   GenList<T>::GenCopyList    (slot +0x20)   T = 12 bytes
//
//  The identification is not a shape argument.  0x1005f780 is
//  `push %esi ; mov 0x8(%ecx),%eax ; add $0x4,%esp ; ret` -- GetSize()
//  reading m_nElements at +0x08, already named in d3dr4b.cpp.  0x1000a670
//  is `mov 0x4(%ecx),%eax` (DefaultCache::GetWantedCache, +0x04),
//  0x1000a680 is `mov 0x8(%esp),%eax ; mov %eax,(%ecx)`
//  (DefaultCache::SetCacheSize, +0x00) and 0x1000a690 is
//  `mov (%ecx),%eax` (GetCacheSize).  0x10009c90 forms `this+0x0c`, calls
//  0x1000a690 and adds 0x8(%ebx) -- that is GetNumAllocatedElements()
//  verbatim.  0x10009cc0 shifts its first argument left twice and hands the
//  product to vtable slot +0x04 of its second, which is
//  `_AllocateTArray(n, pAlloc)` for a four-byte T through LAlloc::Alloc.
//  With those five named, 0x1007f810's body IS
//  src/LT2/lithshared/stdlith/dynarray.h's SetSize2, statement for statement,
//  and 0x1007c480 / 0x10050560 are the two list bodies whose source
//  d3dr4a.cpp already reproduces for the 64- and 16-byte instantiations.
//
//  IMPORTANT SIZE CORRECTION.  0x1007c480 is NOT 212 bytes and 0x10050560
//  is NOT 220.  Each has an out-of-line `{ Term(); return FALSE; }` arm
//  parked immediately past the enumerated end -- 0x1007c554 (reached only
//  by `jae` from 0x1007c50a) and 0x1005063c (reached only by `jae` from
//  0x100505da).  d3dr4c.cpp already records 0x1005063c as a bogus
//  enumeration row for exactly this reason; the same is true of 0x1007c554.
//  So both functions are ~40 bytes longer than the listing says and the
//  arm is part of the body written here, not a separate function.
//
//  FAMILY 2 -- the DirectX 7 VERTEX-BUFFER BATCHER whose destructor
//  d3dbatchb.cpp reconstructed at 0x1007dcc4.  Five rows:
//
//      0x1007e110   allocate/lock the current VB           (table 0x10089a54)
//      0x1007e1c0   DrawPrimitiveVB, no-flush path         (table 0x10089a44)
//      0x1007dd24   unlock + DrawPrimitiveVB + wrap        (called by the above)
//      0x1007e2c0   Init(IDirect3D7*)                      (table 0x10089a5c)
//      0x100038e0   create every VB up front               (tables 0x1008929c/0x100892dc)
//
//  The class layout is read off these five plus the two functions already
//  in the tree, and the COM slots are read off DX7's own vtable order:
//
//      +0x00  vptr (0x10089a20)          -- d3dbatchb.cpp
//      +0x04  CMoArray<IDirect3DVertexBuffer7*> m_VBs, i.e.
//             +0x04 vptr (0x10089a60), +0x08 m_pArray, +0x0c m_nElements,
//             +0x10/+0x14 DefaultCache  -- d3dbatchb.cpp, and 0x1007c870
//             (operator[]) is called on `this+0x04` by 0x100038e0
//      +0x18  m_iCurVB          -- 0x1007dec4 (d3dr2d.cpp) increments it
//      +0x1c  m_nCurVertex      -- 0x1007dec4 zeroes it; += the vertex count
//      +0x20  m_nVertsPerVB     -- the wrap bound in 0x1007dec4
//      +0x24  m_nNumVBs         -- the trip count of both create loops
//      +0x28  a flag: picks 0x2c2 vs 0x2c4 as the FVF
//      +0x2c  a flag: 0 adds D3DVBCAPS_SYSTEMMEMORY (0x800)
//      +0x30  "have a lock" ; +0x34 the locked pointer ; +0x38 cleared with it
//      +0x3c  a second CMoArray, element 16 bytes with a zeroing ctor
//      +0x58  the FVF used by 0x1007e110
//      +0x5c  non-null selects the flushing draw path
//      +0x60/+0x64/+0x68  a cached value, the IDirect3D7, the device
//
//  Slot +0x14 of the object at +0x64 is called with (desc, out, 0) and the
//  desc is a 16-byte block whose first dword is 0x10 -- that is
//  D3DVERTEXBUFFERDESC{dwSize,dwCaps,dwFVF,dwNumVertices} and
//  IDirect3D7::CreateVertexBuffer, whose slot is the sixth (0x14).  Slot
//  +0x7c of the object at +0x68 is called with five arguments and the
//  second is the vertex buffer -- IDirect3DDevice7::DrawPrimitiveVB, the
//  thirty-second slot (0x7c).  D3DVBCAPS_WRITEONLY is 0x10000 and
//  D3DVBCAPS_SYSTEMMEMORY is 0x800, which is exactly the two constants the
//  desc is built from.
//
//  FAMILY 3 -- console-variable registration.  Four rows, and the shape is
//  the one d3dr4b.cpp / d3dr4c.cpp already record for 0x1005db90 and
//  0x10068c50: `add $0xfffffff0,%esp`, a float bit pattern into 0x4(%esp),
//  the name into (%esp), the bank into %ecx, two zeros, `call 0x10021f40`.
//  Here they come four and seven to a function:
//
//      0x10014600   DetailTextures, DetailTextureAdd, DetailTextureAngle,
//                   FixSparkleys, LMFullBright, EnvMapWorld, LMAnim
//      0x10033070   UseD3DClip, MipMapBias, Trilinear, Anisotropic
//      0x1005dbf0   PortalFlow, PortalGraph, PortalsOnly, BlockersOnly
//      0x1003ac30   S3TCEnable, plus two list heads and a pool
//
//  All four are stored into a .data table (0x10089030, 0x100890b0,
//  0x10089170, 0x100890c0) rather than called, which is why they have no
//  callers -- they are slots of a per-module registry.
//
//  THE FLAG VERDICT.  Every function in this file wants /O2, on all three
//  of the /O2 tells:
//    * outgoing arguments are `add $-N,%esp` followed by `mov`, never
//      `push`, everywhere a call takes more than one argument;
//    * epilogues are DUPLICATED at second and later exits -- 0x100437f0
//      carries six copies of `add $0x10,%esp ; pop %ebx ; ret`,
//      0x1007c480 and 0x10050560 two copies each of an eight-instruction
//      epilogue, 0x10050284 two copies of a six-instruction one;
//    * `&p->m` for a call argument is `mov $K,%reg ; add %p,%reg`
//      (0x10009c90's own body, and 0x1007e2c0's `mov $0x10091df0,%ecx`).
//  The one counter-indication is `lea 0xc(%ebx),%ecx` in 0x1007f810 and
//  0x1001ddec's parent, which is the /Os spelling of `&m_Cache`; that is a
//  member address used as `this` for a thiscall rather than pushed as an
//  argument, and d3dr4b.cpp's already-matching bodies emit it at /O2 too.
//  So: /O2 for the whole file, and if the four Family-1 rows disagree they
//  are the ones to split out (they are the least certain).
//
//  MEASURED SINCE (supersedes the verdict above).  The file is built at
//  /Os /Oi- (build/engine.sh D3D_OS_OI_TUS): against plain /Os that takes
//  it from 4 to 5 identical and 2808 to 2689 differing bytes with nothing
//  lost -- 0x10021220's malloc stops counting as fresh memory, so its
//  global loads stay behind the node stores as shipped.  The rows the tells
//  above really do describe were split into d3dr5c_o2.cpp (plain /O2):
//  0x1007c480 and 0x10050560 are byte-identical there and 0x100038e0 /
//  0x100437f0 are far closer than here (duplicated epilogues, mov-saved
//  registers, the inline memset).  The rest stay here: at /O2 every one of
//  them gets worse.
//
//  BRANCH LAYOUT.  This icl predicts `if (p)` on a POINTER member as
//  taken and lays the arm out inline; the shipped bytes repeatedly host
//  such arms out of line (0x1007e110, 0x1007e1c0, 0x1007f810's
//  `if (m_pArray)`, 0x1007e2c0's inlined SetSize2).  Where the member is
//  plainly a flag or a slot word (0x1007e1c0's +0x5c, 0x1007e110's VB
//  slots) typing it as an integer reproduces the shipped layout; where
//  the test is inside stdlith's dynarray.h (0x1007f810, 0x1007e2c0) the
//  source is fixed and the layout residual is recorded as a near.
//
//  THE ONE ROW THAT IS NOT WRITTEN
//
//    0x1001ddec  is NOT A FUNCTION.  It opens `ff 4b 08` --
//                `decl 0x8(%ebx)` -- and then runs straight into
//                `pop %ebx ; pop %esi ; pop %edi ; mov %ebp,%esp ;
//                pop %ebp ; ret $0x8`.  It has no prologue, %ebx already
//                holds `this` and %ebp already frames the caller, and the
//                only reference to it in 551KB of .text is
//                `e9 56 ff ff ff` at 0x1001de91 -- a backward `jmp` from
//                inside the function that precedes it.  That function
//                starts at 0x1001dd80 and reaches 0x1001dea2; the block at
//                0x1001ddec is its `--m_nElements ; return TRUE;` tail,
//                hosted first and jumped back into, and 0x1001ddbb (the
//                target of the other backward jump at 0x1001de9d) is the
//                same function's loop head.  This is the seventh instance
//                of the shape a function enumerator has to
//                recognise: no prologue, and every reference a branch from
//                within the preceding function.
//
//  TWO ROWS ARE WRITTEN BUT CANNOT MATCH -- the calling-convention shim
//
//    0x10011450 and 0x10043660 are TWO-ENTRY functions.  Each begins with
//    a four-byte `mov 0x4(%esp),%eax` adapter and the body proper starts at
//    +4, and BOTH entries are really used: 0x1005b279 calls 0x10011450 and
//    0x10011cdb calls 0x10011454; 0x10032902 calls 0x10043660 and
//    0x1004429d calls 0x10043664.  The body never re-reads 0x8(%ebp), so
//    past the shim the argument lives in %eax.  No C source makes a
//    compiler emit an entry stub in front of a register-argument body --
//    d3dhigh.cpp records the same verdict for 0x10079f30 -- so the bodies
//    below are DECODE ONLY and will come out four bytes short with the
//    argument loaded from the frame.  Worth keeping anyway: they name eight
//    IDirect3DDevice7::SetTextureStageState calls and one save/restore
//    pair, and if a switch is found that makes icl 4.0 emit
//    the dual entry (it does emit a dual entry for the double-argument
//    realignment in 0x1001c1d0 below, so the mechanism exists) they are
//    ready.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltlink.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  The CRT entry points this file reaches.  Declared rather than included
//  so the file needs nothing beyond the two standard headers d3dr2d.cpp and
//  d3dmid.cpp already use in this directory.
//    0x10081222  `push $1 ; push size ; call _nh_malloc`  -- operator new
//    0x100809f8  `push __newmode ; push size ; call _nh_malloc` -- malloc
//    0x100809a0  memset      0x10080a90  strncpy
//    0x1007fdd0  `fldl 0x4(%esp) ; fsin` -- sin
//    0x10080502  `push 0x4(%esp) ; call _atexit ; neg/sbb/neg/dec` -- atexit
// ---------------------------------------------------------------------- //
#include <stdlib.h>
#include <string.h>


// ---------------------------------------------------------------------- //
//  0x10091df0 -- stdlith's g_DefAlloc (l_allocator.h), declared exactly as
//  d3dr4b.cpp declares it and never defined, so no vtable is emitted here.
//  Slot +0x04 is Alloc(DWORD) and slot +0x08 is Free(void*): 0x10009cc0
//  reaches +0x04 with a byte count and 0x1007dcc4 reaches +0x08 with the
//  buffer.
// ---------------------------------------------------------------------- //
class LAllocR5C
{
public:
	virtual				~LAllocR5C();                   // vtable +0x00
	virtual void*		Alloc(uint32 nSize);            // vtable +0x04
	virtual void		Free(void* pMem);               // vtable +0x08
};

extern LAllocR5C g_10091df0;


// ---------------------------------------------------------------------- //
//  genlist.h's GenListPos.  Two dwords, plain old data -- the `pos =
//  theList.GenBegin()` assignment in the two list bodies below is two
//  `mov`s, not a call.  Spelled the way d3dr4a.cpp spells it so that the
//  by-value GenRemoveAt/GenBegin ABI comes out the same.
// ---------------------------------------------------------------------- //
struct BaseGenListPosR5C
{
	uint32	m_Index;
};

struct GenListPosR5C : public BaseGenListPosR5C
{
	GenListPosR5C()					{}
	GenListPosR5C(uint32 nIndex)	{ m_Index = nIndex; }

	BaseGenListPosR5C	m_SubIndex;
};


// ---------------------------------------------------------------------- //
//  The console-variable bank.  0x10021f40 is `ret $0x10`; it stores its
//  four arguments at +0x08..+0x14 of `this`, zeroes +0x00/+0x04, links
//  itself onto a global chain and returns `this`.  d3dr4b.cpp and
//  d3dr4c.cpp already carry the one-variable version of this shape.
// ---------------------------------------------------------------------- //
struct CVarR5C
{
	CVarR5C*	sub_10021f40(const char* pName, float fVal,
							 uint32 nA, uint32 nB);     // 0x10021f40
};


// ---------------------------------------------------------------------- //
//  0x10021d60 -- the renderer's warning printf.  d3ddraw.cpp already names
//  it; every call site here hands it a level, a format and one argument, so
//  it is the varargs one.
// ---------------------------------------------------------------------- //
void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage (was d3d_Warning*; icl treats a callee named *Warn*/*Error* as cold)


// ======================================================================= //
//  FAMILY 3 -- CONSOLE-VARIABLE REGISTRATION
//
//  Each registration is, verbatim:
//
//      83 c4 f0                     add  $0xfffffff0,%esp
//      33 c0                        the shared integer zero
//      c7 44 24 04 <bits>           arg2 = the float, as an immediate
//      c7 04 24 <name>              arg1 = the name
//      b9 <bank>                    this
//      89 44 24 08 / 89 44 24 0c    arg3 = arg4 = 0
//      e8 <0x10021f40>
//
//  and where the float is 0.0f the second argument is a `movl $0x0` of its
//  own rather than another use of %eax, which is what a float parameter
//  taking an integer zero register cannot do.  The `xor %eax,%eax` is
//  re-emitted before every call, so the zeros are per-statement and not
//  hoisted -- four (or seven) separate statements, not a loop over a table.
// ======================================================================= //

extern CVarR5C g_1008aea0;                              // DetailTextures
extern CVarR5C g_1008ae80;                              // DetailTextureAdd
extern CVarR5C g_1008ae60;                              // DetailTextureAngle
extern CVarR5C g_1008ae40;                              // FixSparkleys
extern CVarR5C g_1008ae20;                              // LMFullBright
extern CVarR5C g_1008ae00;                              // EnvMapWorld
extern CVarR5C g_1008ade0;                              // LMAnim

// ---------------------------------------------------------------------- //
//  0x10014600 -- 276 bytes, seven registrations.  In .data at 0x10089030.
// ---------------------------------------------------------------------- //
void sub_10014600()
{
	g_1008aea0.sub_10021f40("DetailTextures", 1.0f, 0, 0);
	g_1008ae80.sub_10021f40("DetailTextureAdd", 1.0f, 0, 0);
	g_1008ae60.sub_10021f40("DetailTextureAngle", 0.0f, 0, 0);
	g_1008ae40.sub_10021f40("FixSparkleys", 0.0f, 0, 0);
	g_1008ae20.sub_10021f40("LMFullBright", 0.0f, 0, 0);
	g_1008ae00.sub_10021f40("EnvMapWorld", 1.0f, 0, 0);
	g_1008ade0.sub_10021f40("LMAnim", 1.0f, 0, 0);
}


extern CVarR5C g_1008d7a0;                              // UseD3DClip
extern CVarR5C g_1008d780;                              // MipMapBias
extern CVarR5C g_1008d760;                              // Trilinear
extern CVarR5C g_1008d740;                              // Anisotropic

// ---------------------------------------------------------------------- //
//  0x10033070 -- 162 bytes, four registrations.  In .data at 0x100890b0.
//  Every float here is 0.0f, so all four second arguments are `movl $0x0`.
// ---------------------------------------------------------------------- //
void sub_10033070()
{
	g_1008d7a0.sub_10021f40("UseD3DClip", 0.0f, 0, 0);
	g_1008d780.sub_10021f40("MipMapBias", 0.0f, 0, 0);
	g_1008d760.sub_10021f40("Trilinear", 0.0f, 0, 0);
	g_1008d740.sub_10021f40("Anisotropic", 0.0f, 0, 0);
}


extern CVarR5C g_1008feb0;                              // PortalFlow
extern CVarR5C g_1008fed0;                              // PortalGraph
extern CVarR5C g_1008fef0;                              // PortalsOnly
extern CVarR5C g_1008ff10;                              // BlockersOnly

// ---------------------------------------------------------------------- //
//  0x1005dbf0 -- 162 bytes, four registrations.  In .data at 0x10089170.
// ---------------------------------------------------------------------- //
void sub_1005dbf0()
{
	g_1008feb0.sub_10021f40("PortalFlow", 0.0f, 0, 0);
	g_1008fed0.sub_10021f40("PortalGraph", 0.0f, 0, 0);
	g_1008fef0.sub_10021f40("PortalsOnly", 0.0f, 0, 0);
	g_1008ff10.sub_10021f40("BlockersOnly", 0.0f, 0, 0);
}


// ======================================================================= //
//  0x1003ac30 -- the module init (S3TCEnable, two list heads, a pool) --
//  is compiled at /O2 in wC1_r5c.cpp; its banner went with it.
// ======================================================================= //


// ======================================================================= //
//  0x1001c1d0 -- 239 bytes.  THE WAVE TABLE AGAIN, PLUS A TAIL.
//
//  Its first 0x97 bytes are the same source d3dr3a.cpp already reproduces
//  at 0x10015d60: the dual entry (`push %ebx ; mov %esp,%ebx ; sub $8 ;
//  and $0xfffffff0,%esp ; add $8 ; jmp +3` with a non-realigning second
//  entry three bytes in), `fildl` of the loop counter, two `fmuls`,
//  `fstpl (%esp) ; call 0x1007fdd0` (fsin), the clamp of the global at
//  0x1008cb7c against two 0.0f literals and two 1.0f literals, the
//  `fmul %st,%st(2) ; faddp ; faddp` association and the second loop's
//  `& 31` forward differences.  THE EIGHT CONSTANTS ARE READ OUT OF THE FILE
//  rather than assumed to match: .data1+0xe18 is 0x3d000000 =
//  0.03125, +0xe1c is 0x40c90fdb = 6.2831855, +0xe20/+0xe24 are 0.0,
//  +0xe28 is 1.0, +0xe2c is 0.5, +0xe38/+0xe3c are 1.0 -- the same eight,
//  in the same order, as .data1+0xa68.. at 0x10015d60.  So this really is
//  the same source emitted into a second object, and no re-derivation is
//  needed.  Nothing about the dual entry is a blocker: icl emits it for the
//  realignment a double-argument call forces, which is why d3dr3a.cpp's
//  copy is written as ordinary C.
//
//  WHAT IS NEW is the tail, which is why this row is 239 bytes and not 180:
//
//      33 c0                        the shared zero
//      ba <0x1008b084>              hoisted, used twice
//      a3 <0x1008b750> / a3 <0x1008b74c>     two counts = 0
//      b8 <0x1008b078>              hoisted, used twice
//      89 15 <0x1008b088>           link2.m_pNext = &link2
//      a3 <0x1008b078> / a3 <0x1008b07c>     link1 tied off
//      89 15 <0x1008b084>           link2.m_pPrev = &link2
//      e8 <0x10009ae0>
//
//  Both links plant their address from a SHARED register, so both are
//  `m_pPrev = m_pNext = this` -- TieOff() -- unlike 0x1003ac30's first
//  list above.  The two dwords at 0x1008b74c/0x1008b750 are far away from
//  either link, so they are separate globals and not either list's
//  m_nElements.  0x10009ae0 is a bare `call` with nothing pushed and is
//  also reached from 0x10008b19.
// ======================================================================= //

#define MATH_R5C_TWOPI  6.283185307f

extern float g_1008cb7c;                                // 0x1008cb7c
extern float g_1008b754[32];                            // 0x1008b754
extern float g_1008b7d4[32];                            // 0x1008b7d4

extern LTLink  g_1008b078;                              // 0x1008b078
extern LTLink  g_1008b084;                              // 0x1008b084
extern uint32  g_1008b74c;                              // 0x1008b74c
extern uint32  g_1008b750;                              // 0x1008b750

void sub_10009ae0();

//  IDENTICAL (was 111 bytes off).  Three source facts:
//    * the angle is its own statement, `fAngle = i / 32.0f` then
//      `sin(fAngle * 2PI)`: the shipped two `fmuls` (1/32, then 2PI).  In
//      one expression icl reassociates them into a single `fmuls` of
//      2PI/32 -- as a float local it does not;
//    * the clamp tests the GLOBAL and assigns fMin in each arm, the in-range
//      arm last (`fld %st(0)` there, a literal pushed in the other two);
//      `fMin = g; if (fMin < 0) ...` copies before the branch;
//    * the sum adds the global, not the clamped fMin: the shipped `faddp`
//      consumes the global's own register, which the clamp arms leave
//      under the pushed literal.
void sub_1001c1d0()
{
	int32	i;
	float	fSin;
	float	fMin;
	float	fAmp;
	float	fAngle;

	for (i = 0; i < 32; i++)
	{
		fAngle = (float)i / 32.0f;
		fSin = (float)sin(fAngle * MATH_R5C_TWOPI);

		if (g_1008cb7c < 0.0f)
			fMin = 0.0f;
		else if (g_1008cb7c > 1.0f)
			fMin = 1.0f;
		else
			fMin = g_1008cb7c;

		fAmp = (1.0f - fMin) * 0.5f;

		g_1008b7d4[i] = fSin * fAmp + fAmp + g_1008cb7c;
	}

	for (i = 0; i < 32; i++)
		g_1008b754[i] = g_1008b7d4[(i + 1) & 31] - g_1008b7d4[i];

	g_1008b74c = 0;
	g_1008b750 = 0;

	g_1008b078.TieOff();
	g_1008b084.TieOff();

	sub_10009ae0();
}


// ----------------------------------------------------------------------- //
//  0x10027f70 -- MOVED to w29_10027f70.cpp: an /O2 function
//  (mov-saved registers, lea padding) calling LTMatrix::Init's sixteen
//  floats; identical there.
// ----------------------------------------------------------------------- //


// ======================================================================= //
//  0x10036380 -- 156 bytes.  In .data at 0x100894a4.  PUSH A NODE ONTO A
//  LIST, INITIALISE ITS PAYLOAD, AND UNDO IT IF THE INITIALISATION FAILS.
//
//  WHAT PINS IT:
//    * 0x10081222 is `push $1 ; push size ; call 0x10080a0a` -- the CRT's
//      operator new, not malloc (malloc is 0x100809f8 and pushes
//      __newmode).  0x10036254 is the twelve-byte node's CONSTRUCTOR: it
//      news 0x10 bytes, runs 0x10035bb4 on it, then stores that pointer at
//      +0x00, its first argument at +0x04 and its second at +0x08 and
//      returns `this`.
//    * the allocation's null arm materialises zero into %eax and the store
//      to `m_p04->m_pTop` happens on BOTH paths, with only the constructor
//      call guarded.  09-codegen-patterns.md's "new with a constructor"
//      entry says that split is `new`, and not a written-out
//      allocate-then-test.
//    * the arguments to the constructor are `m_p04` and `m_p04->m_pTop`
//      read BEFORE the store, so the node is threaded onto the front of
//      the list: `m_pTop = new Node(owner, m_pTop)`.
//    * `movsbl %al,%eax` after `call *0x4(vtbl)` -- the payload's second
//      virtual slot returns a signed char.
//    * 0x10035ab0 is `<destructor body> ; test $0x1,0x8(%ebp) ; jne ->
//      operator delete` -- a scalar deleting destructor -- and it is
//      reached with a literal 1, so the failure arm is `delete`.
//    * the failure arm re-reads `m_p04` and `m_p04->m_pTop` from memory and
//      does NOT clear the member afterwards.  Both are source facts.
// ======================================================================= //

struct Payload10036380
{
	virtual void	Slot00();                           // vtable +0x00
	virtual char	Slot04(void* pA, void* pB);         // vtable +0x04
};

struct Node10036380
{
					Node10036380(void* pOwner,
								 Node10036380* pNext);  // 0x10036254, ctor
	void			sub_10035ab0(uint32 nFlags);        // 0x10035ab0, dtor

	Payload10036380*	m_pPayload;                     // +0x00
	void*				m_pOwner;                       // +0x04
	Node10036380*		m_pNext;                        // +0x08
};

struct Mgr10036380
{
	uint8			m_pad_00[0x08];
	Node10036380*	m_pTop;                             // +0x08 of the mgr
};

//  NEAR (94 -> 62).  m_pTop really is at +0x08 of the manager -- the pad
//  was missing.  Left: register choice only (the payload lands in %eax in
//  the shipped code, %ecx here); ternary, if/else, an inline accessor and
//  the two return spellings all emit the same bytes.
struct Obj10036380
{
	uint8			m_pad_00[0x04];
	Mgr10036380*	m_p04;                              // +0x04

	Payload10036380*	sub_10036380(void* pA, void* pB);
};

Payload10036380* Obj10036380::sub_10036380(void* pA, void* pB)
{
	Payload10036380*	pPayload;

	m_p04->m_pTop = new Node10036380(m_p04, m_p04->m_pTop);

	pPayload = m_p04->m_pTop ? m_p04->m_pTop->m_pPayload : LTNULL;

	if (!pPayload->Slot04(pA, pB))
	{
		if (m_p04->m_pTop)
			m_p04->m_pTop->sub_10035ab0(1);

		pPayload = LTNULL;
	}

	return pPayload;
}


// ======================================================================= //
//  FAMILY 1 -- CMoArray<T,C>
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  0x1007f810 -- 160 bytes, 2 callers (0x1007f8cb, 0x1007fa6b).
//  `CMoArray<T,C>::SetSize2(DWORD newSize, LAlloc *pAlloc)` with a
//  four-byte T, from dynarray.h, statement for statement:
//
//      cmp 0x8(%ebx),%esi / jne / test %esi,%esi / je -> xor;inc
//                              if((newSize==m_nElements)&&(newSize!=0))
//                                  return TRUE;
//      mov 0x4(%ebx),%eax / test / jne
//                              if(m_pArray)
//      call 0x10009c90         GetNumAllocatedElements()
//      call 0x10009cf0         _DeleteAndDestroyArray(pAlloc, that)
//      mov %esi,0x8(%ebx)      m_nElements = newSize;
//      test %esi,%esi / ja     if(newSize > 0)
//      call 0x1000a670         m_Cache.GetWantedCache()
//      add %eax,%esi           newSize + that
//      call 0x10009cc0         _AllocateTArray(that, pAlloc)
//      mov %eax,0x4(%ebx)      m_pArray = it
//      test %eax,%eax / je     if(!m_pArray)
//      call 0x1000a670 / call 0x1000a680
//                              m_Cache.SetCacheSize(m_Cache.GetWantedCache())
//    failure arm:
//      xor %eax,%eax / mov %eax,(%esp) / mov %eax,0x8(%ebx) / call 0x1000a680
//                              m_nElements=0; m_Cache.SetCacheSize(0);
//                              return FALSE;
//
//  WHAT PINS THE OPERAND ORDER: `cmp 0x8(%ebx),%esi` puts newSize in the
//  cmp DESTINATION, which is the header's `newSize == m_nElements` and not
//  the other way round.  `ja` (not `jg`) on `newSize > 0` is the unsigned
//  compare a DWORD gives.
//
//  WHAT PINS THE ARGUMENT ORDER OF 0x10009cf0: the pAlloc pointer goes to
//  (%esp) and the count to 0x4(%esp), i.e. `_DeleteAndDestroyArray(pAlloc,
//  n)` -- the header's order.  0x10009cc0's are the other way round,
//  `_AllocateTArray(n, pAlloc)`, also the header's.
//
//  The dead `push %edi` in the prologue (pushed, never popped, never read)
//  is icl's four-byte dummy slot; it is reused as the one-argument
//  outgoing area at 0x1007f880.
// ---------------------------------------------------------------------- //
struct Cache1007f810
{
	uint32	sub_1000a670() const;                       // GetWantedCache
	void	sub_1000a680(uint32 nSize);                 // SetCacheSize
};

//  IDENTICAL (was 80 bytes off, size-exact but for the layout).  The
//  shipped code hosts `if (m_pArray) _DeleteAndDestroyArray` past the
//  epilogue, falls through `ja` into the allocation, and sinks the
//  allocation-failure arm; as ifs (and under /Os, /O1, /Os /Oi-, /Os /Qip,
//  /G4-/G6, /Ob0) icl keeps all three inline.  icl sinks a `switch`'s
//  non-zero arm out of line, and each of the three conditions written as a
//  switch places its block as shipped:
//    * `switch ((uint32)m_pArray)` for the delete (`m_pArray != LTNULL`
//      materialises a bool here, 171 bytes);
//    * `switch (nNewSize > 0)` for the allocation -- still the `ja`;
//    * `switch (m_pArray == LTNULL)` for the failure arm (`!m_pArray` is
//      the same; `switch ((uint32)m_pArray) { case 0: ... }` is not).
//  0x1007e2c0 below needed the first two over its inlined SetSize2 (the
//  delete there as `switch (m_pArray != LTNULL)`: each site its own form).
struct Arr1007f810
{
	uint8			m_pad_00[0x04];                     // +0x00 vptr
	void*			m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	Cache1007f810	m_Cache;                            // +0x0c

	// Declared only -- all three are out of line in the image.
	uint32	sub_10009c90();                             // GetNumAllocated
	void	sub_10009cf0(LAllocR5C* pAlloc, uint32 nElements);
	void*	sub_10009cc0(uint32 nElements, LAllocR5C* pAlloc);

	LTBOOL	sub_1007f810(uint32 nNewSize, LAllocR5C* pAlloc);
};

LTBOOL Arr1007f810::sub_1007f810(uint32 nNewSize, LAllocR5C* pAlloc)
{
	if ((nNewSize == m_nElements) && (nNewSize != 0))
	{
		return LTTRUE;
	}

	switch ((uint32)m_pArray)
	{
		case 0: break;
		default:
			sub_10009cf0(pAlloc, sub_10009c90());
	}

	m_nElements = nNewSize;
	switch (nNewSize > 0)
	{
	case 0: break;
	default:
		m_pArray = sub_10009cc0(nNewSize + m_Cache.sub_1000a670(), pAlloc);
		switch (m_pArray == LTNULL)
		{
		case 0: break;
		default:
			m_nElements = 0;
			m_Cache.sub_1000a680(0);
			return LTFALSE;
		}

		m_Cache.sub_1000a680(m_Cache.sub_1000a670());
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x10050284 -- the grow-only wrapper over an inlined SetSize2 -- is
//  compiled at /O2 in wC1_r5c.cpp; its banner went with it.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1007c480 -- 212 bytes AS ENUMERATED, really 0x1007c480..0x1007c554+
//  including its out-of-line failure arm.  In .data at 0x10089a14, i.e.
//  vtable slot +0x24 of the 0x100899f0 group -- `GenAppendList`.  T is four
//  bytes: GenGetNext comes back in %eax and the store is `mov %edi,(%eax)`.
//
//  WHAT SEPARATES IT FROM GenCopyList, exactly as d3dr4a.cpp records for
//  0x1001c4c0: `call 0x1005f780` (GetSize) comes FIRST, its result is added
//  to `theList.GenGetSize()`, and the resize is 0x1007c730 -- which is
//  `sub_1007c290(newSize, 0, &g_DefAlloc)`, i.e. InternalNiceSetSize with
//  bFast = 0, the NiceSetSize wrapper -- not the SetSize wrapper.  The loop
//  counter therefore starts at prevSize and there is no `xor` before the
//  loop.
//
//  0x1007c870 is operator[]: it forwards one dword to 0x10009d40 (Get) and
//  the store writes THROUGH the returned %eax, so it returns a reference.
// ---------------------------------------------------------------------- //
// T is a bare pointer, not a one-member struct: icl returns a struct in
// memory (hidden pointer, frame +8) where the shipped GenGetNext comes back
// in %eax.  With this 0x1007c480 is byte-identical at /O2 (d3dr5c_o2.cpp).
typedef void* T1007c480;

struct GenList1007c480
{
	virtual GenListPosR5C	GenBegin() const = 0;                       // +0x00
	virtual uint32			GenIsValid(const GenListPosR5C& pos) const = 0;
	virtual T1007c480		GenGetNext(GenListPosR5C& pos) const = 0;   // +0x08
	virtual T1007c480		GenGetAt(GenListPosR5C& pos) const = 0;
	virtual uint32			GenAppend(T1007c480& toAppend) = 0;
	virtual void			GenRemoveAt(GenListPosR5C pos) = 0;
	virtual void			GenRemoveAll() = 0;
	virtual uint32			GenGetSize() const = 0;                     // +0x1c
	virtual uint32			GenCopyList(const GenList1007c480& other) = 0;
	virtual uint32			GenAppendList(const GenList1007c480& other) = 0;
	virtual uint32			GenFindElement(const T1007c480& toFind,
										   GenListPosR5C& thePos) const = 0;
	virtual void			GenSetCacheSize(uint32 nSize) = 0;
};

struct Arr1007c480
{
	// vptr, m_pArray, m_nElements and the two-dword DefaultCache -- 0x14
	// bytes, which is what puts Batch100038e0's +0x20 where it belongs.
	uint8		m_pad_00[0x14];

	// Declared only.
	uint32		sub_1005f780() const;                   // GetSize
	uint32		sub_1007c730(uint32 nNewSize);          // NiceSetSize
	void		sub_1007ca30(LAllocR5C* pAlloc = &g_10091df0);  // Term
	T1007c480&	sub_1007c870(uint32 nIndex) const;      // operator[]

	uint32		sub_1007c480(const GenList1007c480& theList);
};



// ---------------------------------------------------------------------- //
//  0x10050560 -- 220 bytes AS ENUMERATED, really 0x10050560..0x1005063c+
//  including the out-of-line failure arm d3dr4c.cpp already flags.  In
//  .data at 0x100896b0 AND 0x100896e0 -- two vtable groups share it -- and
//  the slot is +0x20, `GenCopyList`.
//
//  T IS TWELVE BYTES, and that is read off the assignment: GenGetNext's
//  return copies out of -0x14/-0x10/-0xc with THREE `mov`s rather than a
//  `rep movsl`, which is icl's field-by-field return-value copy below the
//  sixteen-byte threshold (0x1007c480 above has a four-byte T in %eax;
//  d3dr4a.cpp's 64-byte instantiation gets `rep movsl` with %ecx = 0x10).
//
//  The resize is 0x1002e7f0 -- `sub_1002e710(newSize, &g_DefAlloc)`, the
//  SetSize wrapper -- and it is called BEFORE anything else with
//  `theList.GenGetSize()`, so this is GenCopyList and not GenAppendList.
//  The counter is zeroed with `xor %edi,%edi` in the GenBegin block.
// ---------------------------------------------------------------------- //
struct T10050560
{
	uint32	m_v[3];
};

struct GenList10050560
{
	virtual GenListPosR5C	GenBegin() const = 0;                       // +0x00
	virtual uint32			GenIsValid(const GenListPosR5C& pos) const = 0;
	virtual T10050560		GenGetNext(GenListPosR5C& pos) const = 0;   // +0x08
	virtual T10050560		GenGetAt(GenListPosR5C& pos) const = 0;
	virtual uint32			GenAppend(T10050560& toAppend) = 0;
	virtual void			GenRemoveAt(GenListPosR5C pos) = 0;
	virtual void			GenRemoveAll() = 0;
	virtual uint32			GenGetSize() const = 0;                     // +0x1c
	virtual uint32			GenCopyList(const GenList10050560& other) = 0;
	virtual uint32			GenAppendList(const GenList10050560& other) = 0;
	virtual uint32			GenFindElement(const T10050560& toFind,
										   GenListPosR5C& thePos) const = 0;
	virtual void			GenSetCacheSize(uint32 nSize) = 0;
};

struct Arr10050560
{
	// Declared only.
	uint32		sub_1005f780() const;                   // GetSize
	uint32		sub_1002e7f0(uint32 nNewSize);          // SetSize
	void		sub_10050710(LAllocR5C* pAlloc = &g_10091df0);  // Term
	T10050560&	sub_100507e0(uint32 nIndex) const;      // operator[]

	uint32		sub_10050560(const GenList10050560& theList);
};



// ======================================================================= //
//  FAMILY 2 -- THE VERTEX-BUFFER BATCHER
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  The two DirectX 7 interfaces this family reaches, cut down to the slots
//  the bytes prove.  Both are C-style COM: `this` is pushed as the first
//  argument and the callee pops (`ff 50 14` with no stack restore after).
// ---------------------------------------------------------------------- //
struct D3DVertexBuffer7R5C;
struct D3DVertexBuffer7R5CVtbl
{
	void*	m_Slot00[4];                                        // +0x00..+0x0c
	long	(__stdcall* Unlock)(D3DVertexBuffer7R5C*);           // +0x10
};
struct D3DVertexBuffer7R5C
{
	D3DVertexBuffer7R5CVtbl*	lpVtbl;
};

struct D3D7R5C;
struct D3DVertexBufferDescR5C
{
	uint32	dwSize;                                             // +0x00
	uint32	dwCaps;                                             // +0x04
	uint32	dwFVF;                                              // +0x08
	uint32	dwNumVertices;                                      // +0x0c
};
struct D3D7R5CVtbl
{
	void*	m_Slot00[5];                                        // +0x00..+0x10
	long	(__stdcall* CreateVertexBuffer)(D3D7R5C*,
						D3DVertexBufferDescR5C*,
						D3DVertexBuffer7R5C**, uint32);         // +0x14
};
struct D3D7R5C
{
	D3D7R5CVtbl*	lpVtbl;
};

struct D3DDevice7R5C;
struct D3DDevice7R5CVtbl
{
	void*	m_Slot00[31];                                       // +0x00..+0x78
	long	(__stdcall* DrawPrimitiveVB)(D3DDevice7R5C*, uint32,
						D3DVertexBuffer7R5C*, uint32, uint32,
						uint32);                                // +0x7c
};
struct D3DDevice7R5C
{
	D3DDevice7R5CVtbl*	lpVtbl;
};

#define D3DVBCAPS_R5C_SYSTEMMEMORY	0x00000800
#define D3DVBCAPS_R5C_WRITEONLY		0x00010000


// ---------------------------------------------------------------------- //
//  0x1007dd24 -- 188 bytes, 1 caller (0x1007e254, which is 0x1007e1c0
//  below).  UNLOCK THE CURRENT VERTEX BUFFER, DRAW IT, AND WRAP.
//
//  WHAT PINS IT:
//    * `ret $0xc`, `this` in %ecx: thiscall with three arguments -- the
//      device, a primitive type and a vertex count.
//    * two guards share ONE exit block that sits between them and the
//      body: `mov 0x30(%ecx),%eax ; test ; je -> 0x1007dd40` where
//      0x1007dd40 is a two-byte backward `jmp` into the block, then
//      `mov 0x34(%ecx),%eax ; test ; jne -> body` falling into the block.
//      That is the guarded-return form -- `if(!a) return 0; if(!b) return
//      0;` -- with the shared answer hosted at the LAST guard and the first
//      trampolining into it (09-codegen-patterns.md, "which copy of a
//      tail-merged block survives").
//    * `push %edx ; mov (%edx),%edx ; call *0x10(%edx)` with `this` as the
//      ONLY argument: IDirect3DVertexBuffer7::Unlock, slot five (0x10).
//    * `xor %eax,%eax` then `mov %eax,0x38(%ecx)` then
//      `mov %eax,0x34(%ecx)` -- the lock pointer and its companion cleared
//      from one zero register, emitted high-to-low, which is the
//      zero-store grouping icl always does.
//    * the five-argument `call *0x7c(%eax)` is DrawPrimitiveVB and its
//      arguments are, in order, the primitive type, `m_VBs.m_pArray[m_iCurVB]`,
//      m_nCurVertex, the caller's count and a literal 0 for dwFlags.
//    * the HRESULT is routed through %edx -- `mov %eax,%edx` before the
//      `jae`, `mov %edx,%eax` after -- which is a DISTINCT result variable
//      from the call's own result, so it is a named local returned at the
//      end rather than `return pDev->DrawPrimitiveVB(...)`.
//    * `add 0x1c(%ecx),%edx ; mov %edx,0x1c(%ecx) ; cmp 0x20(%ecx),%edx`
//      re-reads the stored value for the compare, so the `+=` is its own
//      statement.  0x1007dec4 (d3dr2d.cpp) is the wrap.
// ---------------------------------------------------------------------- //
struct Batch1007dd24
{
	uint8					m_pad_00[0x08];             // +0x00 two vptrs
	D3DVertexBuffer7R5C**	m_pVBs;                     // +0x08
	uint8					m_pad_0c[0x18 - 0x0c];
	uint32					m_iCurVB;                   // +0x18
	uint32					m_nCurVertex;               // +0x1c
	uint32					m_nVertsPerVB;              // +0x20
	uint8					m_pad_24[0x30 - 0x24];
	uint32					m_n30;                      // +0x30
	void*					m_pLocked;                  // +0x34
	uint32					m_n38;                      // +0x38

	void	sub_1007dec4();                             // the wrap
	long	sub_1007dd24(D3DDevice7R5C* pDev, uint32 nPrimType,
						 uint32 nCount);
};

//  MOVED to w59_1007dd24_osoi.cpp, same flags, where it is
//  BYTE-IDENTICAL: m_VBs is there the real dynarray.h CMoArray, whose
//  inlined operator[] gives the shipped frame-save style, and the wrap is a
//  `switch` with the call as the sunk default arm.  The struct above stays
//  for 0x1007e1c0's declaration.


// ---------------------------------------------------------------------- //
//  0x1007e1c0 -- 163 bytes.  In .data at 0x10089a44.  DRAW, EITHER
//  DIRECTLY OR THROUGH THE FLUSHING PATH AT 0x1007dd24.
//
//  WHAT PINS IT:
//    * `ret $0xc`, thiscall, three arguments; the first is stored into
//      +0x68 and is then used as a COM object -- `mov (%eax),%eax ;
//      call *0x7c(%eax)` -- so it is the device and +0x68 caches it.
//    * `mov 0x18(%ebx),%edx ; imul $0x10,%edx,%edx ;
//      mov %edi,0xc(%ecx,%esi,1)` with %ecx = 0x40(%ebx) writes +0x0c of
//      element m_iCurVB of the SIXTEEN-BYTE array at +0x40, from the value
//      at +0x60.  The `imul $0x10` (rather than a shift) is icl's index
//      scaling for a 16-byte element.
//    * `mov 0x5c(%ebx),%ecx ; test ; jne` picks the path: non-null goes to
//      0x1007dd24, null does the draw here.  Both arms end
//      `mov %eax,%edx ; mov %edx,%eax`, the distinct-result-variable
//      routing, so there is ONE returned local assigned in two arms.
//    * the direct arm's DrawPrimitiveVB takes m_nCurVertex from +0x1c and
//      then does `add %esi,0x1c(%ebx)` -- read-modify-write straight on the
//      member, so no local holds it.
//    * the 0x1007dd24 arm saves +0x18 into -0xc(%ebp) BEFORE the call and
//      writes it straight back afterwards.  0x1007dd24 can change +0x18
//      (through 0x1007dec4), so that is a deliberate save/restore in the
//      source, and icl kept the store because it does no dead-store
//      elimination through memory.
// ---------------------------------------------------------------------- //
struct Elem1007e1c0
{
	uint32	m_00;
	uint32	m_04;
	uint32	m_08;
	uint32	m_0c;
};

//  IDENTICAL (was 79 bytes off).  +0x5c is an integer flag, and the test
//  is a `switch` with the direct draw as `case 0` and the flush as the
//  default arm: icl sinks the default arm past the epilogue and both arms
//  join at the one `return hr` (`mov %eax,%edx ; mov %edx,%eax`), with
//  iSave in the frame slot -- what no if/else, goto or early-return
//  arrangement gave.  The last 41 bytes were the DrawPrimitiveVB argument
//  stores: the vertex buffer and m_nCurVertex read into locals first make
//  icl load both before storing the arguments last-to-first, as shipped
//  (the buffer alone leaves 23 bytes; `m_pDev` for pDev, a C++ interface
//  or `LTNULL` for the flags change nothing).
struct Batch1007e1c0
{
	uint8					m_pad_00[0x08];
	D3DVertexBuffer7R5C**	m_pVBs;                     // +0x08
	uint8					m_pad_0c[0x18 - 0x0c];
	uint32					m_iCurVB;                   // +0x18
	uint32					m_nCurVertex;               // +0x1c
	uint8					m_pad_20[0x40 - 0x20];
	Elem1007e1c0*			m_pElems;                   // +0x40
	uint8					m_pad_44[0x5c - 0x44];
	uint32					m_p5c;                      // +0x5c, a flag: as a pointer
													// the flush arm is laid out inline
	uint32					m_n60;                      // +0x60
	uint8					m_pad_64[0x68 - 0x64];
	D3DDevice7R5C*			m_pDev;                     // +0x68

	long	sub_1007dd24(D3DDevice7R5C* pDev, uint32 nPrimType,
						 uint32 nCount);
	long	sub_1007e1c0(D3DDevice7R5C* pDev, uint32 nPrimType,
						 uint32 nCount);
};

long Batch1007e1c0::sub_1007e1c0(D3DDevice7R5C* pDev, uint32 nPrimType,
								 uint32 nCount)
{
	long	hr;

	m_pDev = pDev;

	m_pElems[m_iCurVB].m_0c = m_n60;

	switch (m_p5c)
	{
		case 0:
		{
			D3DVertexBuffer7R5C*	pVB = m_pVBs[m_iCurVB];
			uint32					nVertex = m_nCurVertex;

			hr = pDev->lpVtbl->DrawPrimitiveVB(pDev, nPrimType, pVB, nVertex,
											   nCount, 0);
			m_nCurVertex += nCount;
			break;
		}
		default:
		{
			uint32 iSave = m_iCurVB;
			hr = sub_1007dd24(pDev, nPrimType, nCount);
			m_iCurVB = iSave;
		}
	}
	return hr;
}

// ---------------------------------------------------------------------- //
//  0x1007e110 -- 165 bytes.  In .data at 0x10089a54.  CREATE THE CURRENT
//  VERTEX BUFFER IF IT DOES NOT EXIST YET, THEN RUN 0x1007dfa4.
//
//  WHAT PINS IT:
//    * a bare `ret`: no arguments at all, `this` in %ecx.
//    * `mov 0x8(%ebx),%eax ; mov 0x18(%ebx),%edx ;
//      mov (%eax,%edx,4),%eax ; test ; jne` -- the guard is
//      `m_VBs.m_pArray[m_iCurVB]` and BOTH arms end in
//      `this->sub_1007dfa4()`, with the shared call duplicated at
//      0x1007e1ac rather than merged.  That plus the returned literal 0 on
//      every path is why the whole body sits inside one `if`.
//    * `push %edi ; lea -0x14(%ebp),%eax ; push $0x10 ; push $0 ; push %eax ;
//      call 0x100809a0 ; ... add $0x10,%esp` is memset(&desc, 0, 16) as a
//      real CALL, with the arguments PUSHED -- the only pushed call in the
//      file, and what the CRT's own prototype gets.
//    * dwSize = 0x10 and dwCaps = 0x10000, and +0x2c == 0 adds 0x800:
//      D3DVBCAPS_WRITEONLY, plus D3DVBCAPS_SYSTEMMEMORY when the flag is
//      clear.  dwFVF comes from +0x58 and dwNumVertices from +0x20.
//    * the caps update is `mov -0x10(%ebp),%eax ; or $0x800,%eax ;
//      mov %eax,-0x10(%ebp)` -- read, or, write back through the frame,
//      i.e. `desc.dwCaps |= 0x800;` on the struct and not on a local.
//    * `call *0x14(%eax)` on the object at +0x64 with (desc, &out, 0):
//      IDirect3D7::CreateVertexBuffer.  `test %eax,%eax ; jge` is the
//      HRESULT test, so the source wrote `< 0` (or FAILED()).
//    * the store order after success -- +0x0c from -0xc(%ebp), then +0x08
//      from -0x8, then the desc fields -- has dwSize written LAST
//      (`movl $0x10,-0x14(%ebp)` after `movl $0x10000,-0x10(%ebp)`), which
//      is the memset's slot being refilled and is scheduling, not order.
// ---------------------------------------------------------------------- //
//  BYTE-IDENTICAL.  Both 0x1007dfa4 tails `jmp` to the `pop %ebx` PAST the
//  failure arm's `xor %eax,%eax`, so %eax is 0x1007dfa4's own result: both
//  arms are `return sub_1007dfa4();`.  The desc stores come out dwCaps
//  first only when the source writes dwSize first (icl swaps the pair).
struct Batch1007e110
{
	uint8					m_pad_00[0x08];
	uint32*					m_pVBs;                     // +0x08 -- slot words: as
													// pointers the create arm is hosted
	uint8					m_pad_0c[0x18 - 0x0c];
	uint32					m_iCurVB;                   // +0x18
	uint8					m_pad_1c[0x20 - 0x1c];
	uint32					m_nVertsPerVB;              // +0x20
	uint8					m_pad_24[0x2c - 0x24];
	uint32					m_bVidMem;                  // +0x2c
	uint8					m_pad_30[0x58 - 0x30];
	uint32					m_nFVF;                     // +0x58
	uint8					m_pad_5c[0x64 - 0x5c];
	D3D7R5C*				m_pD3D;                     // +0x64

	uint32	sub_1007dfa4();                             // 0x1007dfa4
	uint32	sub_1007e110();
};

uint32 Batch1007e110::sub_1007e110()
{
	D3DVertexBufferDescR5C	desc;
	D3DVertexBuffer7R5C*	pVB;

	if (!m_pVBs[m_iCurVB])
	{
		memset(&desc, 0, sizeof(desc));

		desc.dwSize = sizeof(desc);
		desc.dwCaps = D3DVBCAPS_R5C_WRITEONLY;

		if (!m_bVidMem)
			desc.dwCaps |= D3DVBCAPS_R5C_SYSTEMMEMORY;

		desc.dwFVF = m_nFVF;
		desc.dwNumVertices = m_nVertsPerVB;

		if (m_pD3D->lpVtbl->CreateVertexBuffer(m_pD3D, &desc, &pVB, 0) < 0)
			return 0;

		m_pVBs[m_iCurVB] = (uint32)pVB;

		return sub_1007dfa4();
	}

	return sub_1007dfa4();
}


// ---------------------------------------------------------------------- //
//  0x1007e2c0 -- 252 bytes.  In .data at 0x10089a5c.  INSTALL THE
//  IDirect3D7, SIZE THE PER-BUFFER RECORD ARRAY AND CLEAR EVERYTHING.
//
//  WHAT PINS IT:
//    * `ret $0x4`, thiscall, one argument, stored at +0x64 -- the same slot
//      0x1007e110 above uses as the IDirect3D7.
//    * the block from 0x1007e2d7 to 0x1007e3ba is SetSize2 INLINED over the
//      collection at +0x3c (m_pArray +0x40, m_nElements +0x44, cache
//      +0x48/+0x4c), with the new size read from +0x24: the same
//      `newSize == m_nElements && newSize != 0` pair, the same
//      _DeleteAndDestroyArray with two kept null tests, the same
//      `newSize + GetWantedCache()` and the same failure arm that zeroes
//      m_nElements and the cache size.
//    * `shl $0x4,%edi` makes the element sixteen bytes, and unlike
//      0x10050284 the CONSTRUCTION LOOP SURVIVES:
//        test %esi,%esi / je      while(nElements)
//        xor %ecx,%ecx / dec %edx --nElements
//        test %eax,%eax / jne     the placement `new`'s own null test
//        four stores of %ecx      T's constructor: four dwords to zero
//        add $0x10,%eax           ++pTemp
//        test %edx,%edx / jne
//      That is l_allocator.h's BaseNew verbatim, and the tautological
//      `test %eax,%eax` inside it is the guarded inline
//      09-codegen-patterns.md documents -- write it away and the function
//      comes out short.
//    * the trailing loop re-reads +0x40 AND +0x24 every iteration and
//      steps a byte offset by 0x10 while a separate index walks
//      m_VBs.m_pArray, so it is one loop over two arrays with the bound
//      written as the member.
//    * the element clear emits +0x04, +0x08, +0x0c, +0x00 while the
//      constructor emits +0x00, +0x04, +0x08, +0x0c (see below).
// ---------------------------------------------------------------------- //
//  IDENTICAL (was 231 bytes off).  The shipped code hosts the inlined arms
//  of SetSize2 past the epilogue, and icl sinks the non-zero arm of a
//  `switch` out of line where it keeps an `if` arm as the fall-through (the
//  lever that closed 0x1007ebe0 and 0x1007eb80).  Each piece was measured:
//    * BaseNew's placement-new null test as `switch ((uint32)pTemp)` with
//      the four stores written out (a Clear() call in the arm re-forms the
//      pointer through `and $-1`).
//    * SetSize2's `if (nNewSize > 0)` as `switch (nNewSize > 0)` -- still
//      emitted as the `ja`, but now out of line (194 bytes off as an if).
//    * _DeleteAndDestroyArray's `if (m_pArray)` as
//      `switch (m_pArray != LTNULL)`; `switch ((uint32)m_pArray)` loses the
//      flags for the inner test (`and $-1,%eax`), and so does `!m_pArray`.
//    * the inner test is l_allocator.h's BaseDelete (early return, the
//      trivial destructor loop, Free), not a bare `if (p) Free(p)`: only
//      with the dead loop does icl hoist `mov $g_10091df0,%ecx` above the
//      `je` and load the vtable through %ecx as shipped.
//    * _AllocateTArray calls BaseNew rather than containing its loop: the
//      by-value nElements copy is then the counter (edx) and the original
//      the guard (esi), with the block pointer in %edi and no spill.
//    * the trailing loop's bound is a volatile read of +0x24 (85 bytes off
//      without: icl keeps the SetSize argument in %esi and reloads it on
//      each arm instead of at the loop head).
//    * the clear loop's store order +4,+8,+c,+0 comes from Clear() written
//      +0, +c, +8, +4; icl emits it reversed but for the first.
struct Elem1007e2c0
{
	uint32	m_00;
	uint32	m_04;
	uint32	m_08;
	uint32	m_0c;

	void	Clear()	{ m_00 = 0; m_0c = 0; m_08 = 0; m_04 = 0; }

			Elem1007e2c0()	{ Clear(); }
};

struct Cache1007e2c0
{
	uint32	m_nCacheSize;                               // +0x00
	uint32	m_nWantedCache;                             // +0x04

	uint32	GetCacheSize() const	{ return m_nCacheSize; }
	void	SetCacheSize(uint32 n)	{ m_nCacheSize = n; }
	uint32	GetWantedCache() const	{ return m_nWantedCache; }
};

inline Elem1007e2c0* BaseNew1007e2c0(LAllocR5C* pAlloc, Elem1007e2c0* pDummy,
									 uint32 nElements)
{
	Elem1007e2c0	*ptr, *pTemp;

	ptr = (Elem1007e2c0*)pAlloc->Alloc(sizeof(Elem1007e2c0) * nElements);
	if (!ptr)
		return LTNULL;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		switch ((uint32)pTemp)
		{
			case 0: break;
			default:
				pTemp->m_00 = 0;
				pTemp->m_04 = 0;
				pTemp->m_08 = 0;
				pTemp->m_0c = 0;
		}
		++pTemp;
	}

	return ptr;
}

template<class T>
inline void BaseDelete1007e2c0(LAllocR5C* pAlloc, T* ptr,
							   uint32 nElements)
{
	T	*pTemp;

	if (!ptr)
		return;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		++pTemp;
	}

	pAlloc->Free(ptr);
}

struct Arr1007e2c0
{
	uint8			m_pad_00[0x04];                     // the vptr
	Elem1007e2c0*	m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	Cache1007e2c0	m_Cache;                            // +0x0c / +0x10

	uint32 GetNumAllocatedElements()
	{
		return m_nElements + m_Cache.GetCacheSize();
	}

	Elem1007e2c0* _AllocateTArray(uint32 nElements, LAllocR5C* pAlloc)
	{
		Elem1007e2c0* tPtr = BaseNew1007e2c0(pAlloc, (Elem1007e2c0*)0, nElements);

		return tPtr;
	}

	void _DeleteAndDestroyArray(LAllocR5C* pAlloc, uint32 nElements)
	{
		switch (m_pArray != LTNULL)
		{
			case 0: break;
			default:
				BaseDelete1007e2c0(pAlloc, m_pArray, nElements);
				m_pArray = LTNULL;
		}

		m_Cache.SetCacheSize(0);
	}

	LTBOOL SetSize2(uint32 nNewSize, LAllocR5C* pAlloc)
	{
		if ((nNewSize == m_nElements) && (nNewSize != 0))
		{
			return LTTRUE;
		}

		if (m_pArray)
		{
			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
		}

		m_nElements = nNewSize;
		switch (nNewSize > 0)
		{
		case 0: break;
		default:
			m_pArray = _AllocateTArray(nNewSize + m_Cache.GetWantedCache(),
									   pAlloc);
			if (!m_pArray)
			{
				m_nElements = 0;
				m_Cache.SetCacheSize(0);
				return LTFALSE;
			}

			m_Cache.SetCacheSize(m_Cache.GetWantedCache());
		}

		return LTTRUE;
	}

	LTBOOL SetSize(uint32 nNewSize)
	{
		return SetSize2(nNewSize, &g_10091df0);
	}
};

struct Batch1007e2c0
{
	uint8					m_pad_00[0x08];
	D3DVertexBuffer7R5C**	m_pVBs;                     // +0x08
	uint8					m_pad_0c[0x24 - 0x0c];
	uint32					m_nNumVBs;                  // +0x24
	uint8					m_pad_28[0x3c - 0x28];
	Arr1007e2c0				m_Recs;                     // +0x3c
	uint8					m_pad_50[0x64 - 0x50];
	D3D7R5C*				m_pD3D;                     // +0x64

	LTBOOL	sub_1007e2c0(D3D7R5C* pD3D);
};

LTBOOL Batch1007e2c0::sub_1007e2c0(D3D7R5C* pD3D)
{
	uint32	i;

	m_pD3D = pD3D;

	m_Recs.SetSize(m_nNumVBs);

	for (i = 0; i < *(volatile uint32*)&m_nNumVBs; i++)
	{
		m_Recs.m_pArray[i].Clear();
		m_pVBs[i] = LTNULL;
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x100038e0 -- 240 bytes.  In .data at 0x1008929c AND 0x100892dc.
//  CREATE EVERY VERTEX BUFFER UP FRONT.
//
//  WHAT PINS IT:
//    * `ret $0x4`, thiscall, one argument -- and the argument is the COM
//      object the loop calls slot +0x14 on, so it is an IDirect3D7 handed
//      in rather than read from +0x64.  `this` is spilled to -0x8(%ebp)
//      because the loop body clobbers %ecx.
//    * the descriptor is cleared through a POINTER --
//      `lea -0x1c(%ebp),%edi ; xor %eax,%eax ; mov %eax,(%edi) ;
//      mov %eax,0x4(%edi) ; ...` -- four stores, no call.  That is icl
//      expanding memset of a constant sixteen bytes inline, where
//      0x1007e110 above got the CRT call for the same statement; the two
//      are in different translation units.  THE COIN-FLIP: if this comes
//      out with a `call` where the shipped code has the four stores, the
//      answer is `D3DVERTEXBUFFERDESC desc = {0};` or four explicit
//      `desc.field = 0;` lines instead of memset.
//    * dwFVF is 0x2c2 when +0x28 is non-zero and 0x2c4 when it is zero --
//      two `movl $imm,-0x14(%ebp)` arms of one if/else, not a `?:` (a
//      conditional expression would put both constants through one slot).
//    * `mov $0x1,%esi` before the loop and `xor %esi,%esi` on the failure
//      arm: a BOOL result variable initialised TRUE and cleared, then
//      returned at the end -- `mov %esi,%eax` is the only thing the exit
//      block does.
//    * the loop tests `%ebx >= 0x24(%ecx)` at the BOTTOM and again tests
//      %esi there, and on the way out of either exit it calls 0x1007e030
//      when %esi is clear.  So the source is a counted loop whose body
//      breaks out on failure, with one cleanup call after it guarded by
//      the flag.
//    * `mov -0x8(%ebp),%ecx ; add $0x4,%ecx ; call 0x1007c870` is
//      `m_VBs[i]`, the collection at +0x04 reached through operator[] --
//      an OUT-OF-LINE call here where 0x1007e2c0 above indexes m_pArray
//      inline.  That is the per-function inlining budget, not two helpers.
// ---------------------------------------------------------------------- //
struct Batch100038e0
{
	uint8			m_pad_00[0x04];
	Arr1007c480		m_VBs;                              // +0x04
	uint8			m_pad_18[0x20 - 0x18];
	uint32			m_nVertsPerVB;                      // +0x20
	uint32			m_nNumVBs;                          // +0x24
	uint32			m_n28;                              // +0x28
	uint32			m_bVidMem;                          // +0x2c

	void	sub_1007e030();                             // 0x1007e030
	LTBOOL	sub_100038e0(D3D7R5C* pD3D);
};



// ======================================================================= //
//  0x10073640 -- 163 bytes, 1 caller (0x1007cae8).  IS THIS TWO-TEXTURE
//  RECORD USABLE?
//
//  WHAT PINS IT:
//    * a bare `ret` with `this` in %ecx: thiscall, no arguments, and
//      `xor %eax,%eax ; inc %eax` for the true answer.
//    * SIX guarded `return LTFALSE`s in a row, five sharing one exit block
//      at 0x100736d8 and the sixth getting its own two-instruction copy at
//      0x100736df, with `return LTTRUE` falling through at 0x100736d3.
//      That is a run of `if(bad) return LTFALSE;` statements ending in
//      `return LTTRUE;`.
//    * the two indices are `movzwl 0x4(%ecx)` and `movzwl 0xc(%ecx)`:
//      sixteen-bit fields, compared unsigned (`jae`) against
//      `0x16c(m_p00)`.
//    * `mov 0x168(%eax),%esi ; shl $0x5,%edx ; mov (%esi,%edx,1),%esi`
//      makes 0x168 an array of THIRTY-TWO-byte records whose first dword
//      is a pointer; the sub-index at +0x06 / +0x0e is then bounded by
//      `0x10` of THAT pointer.
//    * the float at +0x14 is loaded ONCE and spilled as a DOUBLE
//      (`fstpl 0x8(%esp)` / `fldl 0x8(%esp)`) between the two compares --
//      icl freeing the x87 stack, and the reason the first compare is
//      `flds <const> ; fcomp %st(1)` (the constant is st0 and the member
//      is st1) rather than the other way round.  Read the sense off that:
//      `ja` after comparing st0 = 0.0f against st1 = the member is taken
//      when 0.0f > member, so the written test is `member < 0.0f`.  The
//      second is a plain `fcomps` against st0 = the member, `ja` when
//      member > 1.0f.
//    * BOTH CONSTANTS are read out of .data1: +0x79ac is 0x00000000 and
//      +0x79b0 is 0x3f800000, two separate literals -- 0.0f and 1.0f, an
//      alpha or blend-weight range check.
//
//  THE COIN-FLIP: the single load with a double spill could also come from
//  `float f = m_f14;` used twice.  The member is written twice, which
//  is what icl's own CSE does to it; if the shipped `fstpl`/`fldl` pair
//  turns into `fstps`/`flds` of a frame slot, the local is the answer.
// ======================================================================= //
struct Sub10073640
{
	uint8	m_pad_00[0x10];
	uint32	m_10;                                       // +0x10
};

struct Rec10073640
{
	Sub10073640*	m_p00;                              // +0x00
	uint8			m_pad_04[0x20 - 0x04];              // 32-byte stride
};

struct World10073640
{
	uint8			m_pad_000[0x168];
	Rec10073640*	m_pRecs;                            // +0x168
	uint32			m_nRecs;                            // +0x16c
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

//  NEAR (was 122 bytes against 163, now 135: the instruction stream is the
//  shipped one but for register saving).  The shipped code keeps
//  m_pWorld, m_nRec0 and m_nRec1 in registers across statements but
//  RE-LOADS m_pWorld in the two count checks and m_pRecs in the two
//  sub-index checks; plain members CSE all of it.  Those four reads are
//  `*(T* volatile*)&` and everything that must survive them is a local
//  (a volatile access is a barrier for every member load).
//  LEFT: the shipped function saves %edi/%esi INTO THE FRAME around their
//  two short uses (`mov %edi,0x10(%esp)` ... `mov 0x10(%esp),%edi`, frame
//  0x18) where ours pushes %esi/%ebp in the prologue (frame 0x10) -- the
//  save style d3dhigh.cpp and d3dr5b.cpp record as unreproduced.  /O2,
//  /O2 /Oi-, /O2 /Qip, /O1, /Op, /Oy-, /Ob0 and switch-form tests do not
//  produce it here.
//  MEASURED: the frame-save style IS reachable -- one `||` guard
//  over inline helpers taking the pointer (`RecSub(World*, i)` returning
//  `w->m_pRecs[i].m_p00`) gives the shipped `sub $0x18`, the
//  `mov %edi,0x10(%esp)` saves around the sub-index tests, the m_pRecs
//  reload per test and the shared `return 0` hosted at the end (160 of
//  163 bytes, 128 differing, 10 mnemonic blocks: not kept, being more
//  bytes off than this one).  What it leaves: the count tests CSE
//  m_pWorld->m_nRecs (shipped reloads m_pWorld from `this` each time --
//  an inline member, an inline over `this`, and a volatile read all
//  measured, none reloads without breaking the rest), and the `> 1.0f`
//  test's own `return 0` copy (a ternary, `<=`, a one-case switch: no).
//  MOVED to w59_10073640_osoi.cpp, same flags: there the
//  +0x164 table is the real dynarray.h CMoArray, which gives the shipped
//  frame-save allocation and length (163 bytes); see that file.


// ----------------------------------------------------------------------- //
//  0x1007a014 -- MOVED to w29_10079fd0_osoiqip.cpp as
//  ConvertRequest::IsValid: it is the /Qip register entry of the pair at
//  0x1007a010, which icl emits only in a /Qip TU holding its caller
//  ConvertPixels 0x1007a130.  Identical there, written from lithtech.exe's
//  FMConvertRequest::IsValid (the volatile reloads this file needed are
//  an inline over the PFormat pointer).
// ----------------------------------------------------------------------- //


// ======================================================================= //
//  0x1007a700 -- MOVED to w49_1007a700.cpp: an /O2
//  function, identical there.
// ======================================================================= //


// ======================================================================= //
//  0x10021220 -- 176 bytes.  Its address is planted as an immediate at
//  0x10021345.  RECORD A MODE IN A 0x214-BYTE NODE AND PUSH IT ON A CHAIN.
//
//  WHAT PINS IT:
//    * `ret $0x8` with the first argument read out of 0xc(%esp) before the
//      frame is set up and NOTHING in %ecx: __stdcall, two arguments, and
//      the first is a pointer whose +0x08, +0x0c and +0x54 are read.
//    * `cmp $0x10,%eax ; je ; cmp $0x20,%eax ; jne <out>` on +0x54 --
//      a bit depth, 16 or 32, and everything else is silently ignored while
//      the function still returns TRUE (`xor %eax,%eax ; inc %eax` sits
//      after the merge, so BOTH paths reach it).
//    * 0x100809f8 is malloc, not operator new (it pushes __newmode at
//      0x10091e60; operator new is 0x10081222 and pushes a literal 1).  So
//      the allocation is `malloc(0x214)` and its failure is tested by hand.
//    * 0x10080a90 is strncpy -- three arguments, and both calls pass 0x7f,
//      which with the 0x80-byte gaps between +0x104, +0x184 and +0x204
//      says two 128-byte char fields copied one short of full.
//    * `mov 0x1008bf20,%eax ; lea 0x24(%eax),%eax` LOADS 0x1008bf20 and
//      then offsets it, so 0x1008bf20 is a POINTER VARIABLE and the two
//      sources are its +0x24 and +0x88.
//    * the tail `mov 0x1008bf24,%eax ; mov %eax,0x210(%ebx) ;
//      mov %ebx,0x1008bf24` is a singly-linked push: next = head; head =
//      node.
// ======================================================================= //
//  BYTE-IDENTICAL at /Os /Oi- only: with intrinsics on, icl treats malloc's
//  result as unaliased and hoists the 0x1008bf20 / 0x1008bf24 loads above
//  the node stores, which the shipped code does not.  `#pragma
//  function(malloc)` and an inline malloc wrapper do not do it; the flag does.
struct Mode10021220
{
	uint8	m_pad_00[0x08];
	uint32	m_08;                                       // +0x08
	uint32	m_0c;                                       // +0x0c
	uint8	m_pad_10[0x54 - 0x10];
	uint32	m_nBitDepth;                                // +0x54
};

struct Node10021220
{
	uint32			m_00;                               // +0x00
	uint8			m_pad_04[0x104 - 0x04];
	char			m_szName[0x80];                     // +0x104
	char			m_szDesc[0x80];                     // +0x184
	uint32			m_204;                              // +0x204
	uint32			m_208;                              // +0x208
	uint32			m_20c;                              // +0x20c
	Node10021220*	m_pNext;                            // +0x210
};

struct Src10021220
{
	uint8	m_pad_00[0x24];
	char	m_szName[0x64];                             // +0x24
	char	m_szDesc[0x80];                             // +0x88
};

extern Src10021220*	g_1008bf20;                         // 0x1008bf20
extern Node10021220*	g_1008bf24;                     // 0x1008bf24

LTBOOL __stdcall sub_10021220(Mode10021220* pMode, uint32 nUnused)
{
	Node10021220*	pNode;

	if (pMode->m_nBitDepth == 0x10 || pMode->m_nBitDepth == 0x20)
	{
		pNode = (Node10021220*)malloc(sizeof(Node10021220));
		if (pNode)
		{
			pNode->m_00 = 1;

			strncpy(pNode->m_szName, g_1008bf20->m_szName, 0x7f);
			strncpy(pNode->m_szDesc, g_1008bf20->m_szDesc, 0x7f);

			pNode->m_204 = pMode->m_0c;
			pNode->m_208 = pMode->m_08;
			pNode->m_20c = pMode->m_nBitDepth;

			pNode->m_pNext = g_1008bf24;
			g_1008bf24 = pNode;
		}
	}

	return LTTRUE;
}


// ======================================================================= //
//  0x100437f0 -- 256 bytes.  In .data at 0x100895d4.  ADD A POLYGON TO ONE
//  OF THREE FIXED-CAPACITY NAMED SETS.
//
//  WHAT PINS IT:
//    * a bare `ret` and the argument read from 0x18(%esp) after
//      `push %ebx ; sub $0x10,%esp`: __cdecl, one argument.
//    * 0x1007b8d0 is `mov $0x10091a10,%eax ; ret` -- the singleton getter
//      d3dneutral.cpp already reconstructed -- and it is called FIRST, so
//      the object pointer is a local, not re-fetched.
//    * the three sets are at +0x144, +0x164 and +0x184 of that object and
//      each is used at +0x00 (array), +0x04 (count), +0x14 (capacity) and
//      +0x18 (name).  That is EXACTLY d3dr3a.cpp's CSetA layout, derived
//      independently there from 0x1005d9c0's use of +0x264/+0x284/+0x2a4,
//      and the message string is the same one: "Set '%s' overflowed".
//    * the selector is `movzbl 0x97(%edx)` against 0xff first and then
//      `test $0x2,%cl` on +0x8c -- the same two fields d3dr3a.cpp's
//      S1005d9c0Item declares at +0x8c and +0x97.
//    * SIX epilogues, one per leaf, every one a full
//      `add $0x10,%esp ; pop %ebx ; ret`.  Nothing is tail-merged, which is
//      the /O2 duplicate-the-epilogue tell, and it is also why the
//      overflow test is written as an if/else rather than a guard.
//    * the append is `mov %edx,(%ebx,%ecx,4)` then
//      `incl 0x188(%eax)` -- the count is re-read from memory for the
//      increment, so `m_nCount++` is its own statement after the store.
//
//  Written with the set's own inline member so that one source site serves
//  three call sites, which is what a class-body member gives /Ob1.
// ======================================================================= //
struct Poly100437f0
{
	uint8	m_pad_00[0x8c];
	uint32	m_8c;                                       // +0x8c
	uint8	m_pad_90[0x97 - 0x90];
	uint8	m_97;                                       // +0x97
};

class CSet100437f0
{
public:
	void	Add(Poly100437f0* pPoly)
	{
		if (m_nCount < m_nMax)
		{
			m_pArray[m_nCount] = pPoly;
			m_nCount++;
		}
		else
		{
			sub_10021d60(1, "Set '%s' overflowed", m_pName);
		}
	}

	Poly100437f0  **m_pArray;                           // +0x00
	uint32			m_nCount;                           // +0x04
	uint8			m_pad_08[0x14 - 0x08];
	uint32			m_nMax;                             // +0x14
	char		   *m_pName;                            // +0x18
};

struct C10091a10R5C
{
	uint8			m_pad_000[0x144];
	CSet100437f0	m_144;                              // +0x144
	uint8			m_pad_160[0x164 - 0x144 - 0x1c];
	CSet100437f0	m_164;                              // +0x164
	uint8			m_pad_180[0x184 - 0x164 - 0x1c];
	CSet100437f0	m_184;                              // +0x184
};

C10091a10R5C* sub_1007b8d0();                        // 0x1007b8d0



// ----------------------------------------------------------------------- //
//  0x10079b90 -- MOVED to w29_10073c00_osoiqip.cpp, the pixel
//  converter family's TU, beside its 8- and 16-bit twins 0x100799b0 /
//  0x10079aa0: lithtech's fm_WritePixel32, a converter built on the stack.
//  The no-op `and $0xffffffff` this banner could not explain is the
//  `(T*)((uint32)p & 0xffffffff)` idiom of the whole family.
// ----------------------------------------------------------------------- //


// ======================================================================= //
//  THE TWO SHIM ROWS -- DECODE ONLY (see the file banner)
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  The IDirect3DDevice7 texture-stage slots.  +0x90 is
//  GetTextureStageState and +0x94 is SetTextureStageState -- the
//  thirty-seventh and thirty-eighth slots -- and both are called with
//  (stage, state, value) after `this`.
// ---------------------------------------------------------------------- //
struct D3DDevTSSR5C;
struct D3DDevTSSR5CVtbl
{
	void*	m_Slot00[36];                                       // +0x00..+0x8c
	long	(__stdcall* GetTextureStageState)(D3DDevTSSR5C*,
					uint32 dwStage, uint32 dwState, uint32* pdwValue);  // +0x90
	long	(__stdcall* SetTextureStageState)(D3DDevTSSR5C*,
					uint32 dwStage, uint32 dwState, uint32 dwValue);    // +0x94
};
struct D3DDevTSSR5C
{
	D3DDevTSSR5CVtbl*	lpVtbl;
};

extern D3DDevTSSR5C*	g_1008d988;                     // 0x1008d988
extern uint32			g_1008ae80_flag;                // 0x1008ae80
extern uint32			g_1008edf0;                     // 0x1008edf0

// ---------------------------------------------------------------------- //
//  0x10011450 -- MOVED to w14_10011980_osoiqip.cpp: it is an
//  external /Qip pair (stack entry + register entry 0x10011454), which icl
//  emits only in a /Qip TU holding its caller 0x10011980.  Identical there.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x10043660 / 0x10043664 -- MOVED to w35_10043e10_qip.cpp:
//  the /Qip two-entry pair's register entry is called only by 0x10043e10,
//  so it lives in that TU (/O2 /Qip), where it is identical.
// ---------------------------------------------------------------------- //
