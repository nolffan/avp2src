// Split out of d3dr5c.cpp: these functions need the
// OTHER optimisation flag (/O2) from the rest of that file (measured:
// 0x1007c480 and 0x10050560 byte-identical here and not at /Os; 0x100038e0
// 95 bytes differ here against 195, with the shipped mov-saves and inline
// memset; 0x100437f0 keeps the duplicated epilogues).  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
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


extern CVarR5C g_1008d7a0;                              // UseD3DClip
extern CVarR5C g_1008d780;                              // MipMapBias
extern CVarR5C g_1008d760;                              // Trilinear
extern CVarR5C g_1008d740;                              // Anisotropic

// ---------------------------------------------------------------------- //
//  0x10033070 -- 162 bytes, four registrations.  In .data at 0x100890b0.
//  Every float here is 0.0f, so all four second arguments are `movl $0x0`.
// ---------------------------------------------------------------------- //


extern CVarR5C g_1008feb0;                              // PortalFlow
extern CVarR5C g_1008fed0;                              // PortalGraph
extern CVarR5C g_1008fef0;                              // PortalsOnly
extern CVarR5C g_1008ff10;                              // BlockersOnly

// ---------------------------------------------------------------------- //
//  0x1005dbf0 -- 162 bytes, four registrations.  In .data at 0x10089170.
// ---------------------------------------------------------------------- //


// ======================================================================= //
//  0x1003ac30 -- 177 bytes.  In .data at 0x100890c0.  A MODULE INIT:
//  two list heads, a struct bank, an atexit, one sub-object init and one
//  console variable.
//
//  WHAT PINS EACH PIECE:
//    * 0x1008e694 gets a literal 0 and 0x1008e698/0x1008e69c both get the
//      address 0x1008e698.  ltlink.h's LTList is `{DWORD m_nElements;
//      LTLink m_Head;}` and CheapLTLink is `{m_pPrev, m_pNext}`, so that is
//      m_nElements at +0x00 and m_Head tied off at +0x04 -- dl_InitList,
//      exactly.
//    * 0x1008e6a4/0x1008e6a8 both get 0x1008e6a4 and there is no count
//      beside them: a bare LTLink tied off.
//    * 0x1007fb20(0x1008e644, 0x4c, 0x20) is the struct bank init
//      d3dr4b.cpp already writes as `sub_1007fb20(&m_Pool, 0x28, 0x20)` --
//      element size then grow-by.  0x1008e640 is set to 0x10089510 first,
//      which is the bank's `this` slot ahead of it.
//    * 0x10080502 is atexit (`push 0x4(%esp) ; call _atexit ;
//      neg ; sbb ; neg ; dec`), so 0x1003acf0 is this module's terminator.
//    * 0x1007a0d0 is reached with `mov $0x1008daa0,%ecx` and nothing
//      pushed -- a thiscall no-argument member on a global.
//
//  THE COIN-FLIP HERE, and it is worth one build.  The two tie-offs are NOT
//  spelled the same way in the shipped code: 0x1008e698 is planted as an
//  IMMEDIATE twice (`c7 05 9c e6 08 10 98 e6 08 10` and
//  `c7 05 98 e6 08 10 98 e6 08 10`, two relocations) while 0x1008e6a4 is
//  planted from a SHARED register (`mov $0x1008e6a4,%eax` once, stored
//  twice).  09-codegen-patterns.md's "same function pointer in two slots"
//  entry reads that difference as two source expressions against one, so
//  the first list is written out by hand -- two separate `&m_Head`s -- and
//  the second uses TieOff(), which is `m_pPrev = m_pNext = this`.  IF THE
//  FIRST LIST COMES OUT WITH A SHARED REGISTER, the answer is that both are
//  dl_InitList(&g_1008e694) / dl_TieOff(&g_1008e6a4) and the immediate
//  duplication is icl's, not the source's.
// ======================================================================= //

extern LTList g_1008e694;                               // 0x1008e694
extern LTLink g_1008e6a4;                               // 0x1008e6a4
extern void*  g_1008e640;                               // 0x1008e640
extern uint8  g_10089510[];                             // 0x10089510
extern uint8  g_1008e644[];                             // 0x1008e644, the bank

extern CVarR5C g_1008da80;                              // S3TCEnable

struct Obj1007a0d0
{
	void	sub_1007a0d0();                             // 0x1007a0d0
};

extern Obj1007a0d0 g_1008daa0;                          // 0x1008daa0

void sub_1007fb20(void* pBank, uint32 nElementSize, uint32 nGrowBy);
void __cdecl sub_1003acf0();                            // the atexit body



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



// ======================================================================= //
//  0x10027f70 -- 148 bytes, 1 caller (0x100268a7).  BUILD A 4x4 MATRIX
//  FROM THREE BASIS VECTORS AND HAND IT TO 0x1001e140 BY VALUE.
//
//  WHAT PINS IT:
//    * `ret $0xc` with %ecx live across the body: thiscall, three stack
//      arguments.  The caller sets them with `movl $0x1008bd1c,(%esp)`,
//      `movl $0x1008bd10,0x4(%esp)`, `movl $0x1008bd28,0x8(%esp)` and
//      `lea -0x4b0(%ebp),%ecx`, so all three are pointers to globals.
//    * `add $0xffffffc0,%esp` opens a SIXTY-FOUR byte outgoing area, and
//      0x1001e140 reads its arguments from 0x40(%esp) to 0x7c(%esp) after
//      four pushes and `sub $0x2c` -- sixteen consecutive dwords.  So
//      0x1001e140 takes a 4x4 float matrix BY VALUE, plus `this` in %ecx,
//      and it pops the 0x40 itself (the caller restores nothing).
//    * every element move is an integer `mov`, which is icl copying float
//      lvalues, not integer data.
//    * the three vectors are read at +0x00/+0x04/+0x08 and land in the
//      matrix transposed -- A,B,C across a row, one component per row --
//      so the three arguments are the COLUMNS of the rotation.
//    * `sub $0x58,%esp` is 0x40 of matrix plus 0xc of saved registers plus
//      0xc more; 0x100156e0 is the same function with an identity matrix
//      and reserves 0x48 for the same 0x40 with no saved registers.  So
//      the matrix IS a named local whose copy into the argument area icl
//      elided, and the frame reservation for it survives.
//    * the store order is 0,4,8,c then 0x10,0x14,0x1c,0x18 then
//      0x20,0x24,0x2c,0x28 then 0x30,0x34,0x38,0x3c: row-major, with the
//      row's zero hoisted one slot early twice.  That is a scheduler
//      choice inside a row, not a source order.
//    * `mov %ecx,%edx ; mov %edx,%ecx` round-trips `this` for the call and
//      is icl's spelling for it -- 0x100156e0 has the identical pair.
// ======================================================================= //

struct Vec10027f70
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

struct Mat10027f70
{
	float	m[4][4];
};

struct Obj10027f70
{
	void	sub_1001e140(Mat10027f70 mat);              // 0x1001e140
	void	sub_10027f70(const Vec10027f70* pRight,
						 const Vec10027f70* pUp,
						 const Vec10027f70* pForward);
};



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
	Node10036380*	m_pTop;                             // +0x08 of the mgr
};

struct Obj10036380
{
	uint8			m_pad_00[0x04];
	Mgr10036380*	m_p04;                              // +0x04

	Payload10036380*	sub_10036380(void* pA, void* pB);
};



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

struct Arr1007f810
{
	uint8			m_pad_00[0x04];                     // +0x00 vptr
	void*			m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	Cache1007f810	m_Cache;                            // +0x0c

	// Declared only -- all three are out of line in the image.
	uint32	sub_10009c90() const;                       // GetNumAllocated
	void	sub_10009cf0(LAllocR5C* pAlloc, uint32 nElements);
	void*	sub_10009cc0(uint32 nElements, LAllocR5C* pAlloc);

	LTBOOL	sub_1007f810(uint32 nNewSize, LAllocR5C* pAlloc);
};



// ---------------------------------------------------------------------- //
//  0x10050284 -- 172 bytes, 1 caller (0x1004fd10, which sums five counts
//  and hands the total in with %ecx = 0x1008ef20).  A GROW-ONLY WRAPPER
//  WHOSE SetSize2 IS FULLY INLINED, over a twelve-byte T.
//
//  WHAT PINS IT AS TWO NESTED CONDITIONS AND NOT ONE:
//  four branches come off the single `cmp 0x8(%ebx),%esi`.
//
//      cmp 0x8(%ebx),%esi
//      ja   0x100502a8       ; newSize > m_nElements -- the wrapper's guard
//      <epilogue>            ; falls straight out, %eax never set
//   0x100502a8:
//      jne  0x100502ae       ; the SAME compare, re-tested
//      test %esi,%esi
//      jne  <epilogue>       ; (newSize==m_nElements)&&(newSize!=0)
//
//  The second pair is SetSize2's own first statement, kept even though the
//  `ja` has just proved it false -- 09-codegen-patterns.md's "four branches
//  off one cmp means a condition the compiler could not satisfy but would
//  not delete".  And %eax is never set on any exit, so the wrapper returns
//  void and throws SetSize2's BOOL away.
//
//  WHAT PINS THE INLINING RATHER THAN A CALL: the allocator is reached
//  directly -- `mov 0x10091df0,%eax ; mov $0x10091df0,%ecx ;
//  mov 0x4(%eax),%eax ; call *%eax` -- with the element count multiplied
//  by twelve (`lea 0x0(,%esi,8),%edx ; lea (%edx,%esi,4),%edx`) in the
//  wrapper's own frame.  Template members are implicitly inline, so /Ob1
//  expands SetSize2, _DeleteAndDestroyArray, _AllocateTArray and
//  l_allocator.h's BaseNew/BaseDelete all the way down; they are written
//  out here in the class body so icl has the same licence.
//
//  WHAT SAYS T HAS NEITHER A CONSTRUCTOR NOR A DESTRUCTOR: BaseNew's
//  construction loop and BaseDelete's destruction loop are BOTH absent, and
//  with them GetNumAllocatedElements -- there is no `add` of the cache size
//  anywhere, because the only consumer of that count was the dead
//  destruction loop.  Compare 0x1007e2c0 below, where the same template
//  over a sixteen-byte T with a zeroing constructor keeps its loop.
//
//  WHAT PINS THE ALLOCATOR AS A POINTER, NOT AN OBJECT: `mov <global>,%eax`
//  for the vptr plus `mov $<global>,%ecx` for `this` plus an indirect
//  `call *%eax` is the compile-time-known POINTER shape.  A virtual called
//  on the object itself devirtualises to a direct call, which is not what
//  ships -- and `&g_DefAlloc` as SetSize2's default argument is a pointer
//  by construction.
//
//  The two dead `je`s at 0x10050307 and 0x1005030e are the same null test
//  BaseDelete and _DeleteAndDestroyArray each write; icl keeps both, which
//  is 09-codegen-patterns.md's "no folding of redundant null tests".
// ---------------------------------------------------------------------- //
struct T10050284
{
	uint32	m_v[3];                                     // twelve bytes
};

struct Cache10050284
{
	uint32	m_nCacheSize;                               // +0x00
	uint32	m_nWantedCache;                             // +0x04

	uint32	GetCacheSize() const	{ return m_nCacheSize; }
	void	SetCacheSize(uint32 n)	{ m_nCacheSize = n; }
	uint32	GetWantedCache() const	{ return m_nWantedCache; }
};

struct Arr10050284
{
	uint8			m_pad_00[0x04];                     // +0x00 vptr
	T10050284*		m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	Cache10050284	m_Cache;                            // +0x0c / +0x10

	uint32	GetNumAllocatedElements()
	{
		return m_nElements + m_Cache.GetCacheSize();
	}

	T10050284* _AllocateTArray(uint32 nElements, LAllocR5C* pAlloc)
	{
		T10050284*	ptr;

		ptr = (T10050284*)pAlloc->Alloc(sizeof(T10050284) * nElements);
		if (!ptr)
			return LTNULL;

		return ptr;
	}

	void _DeleteAndDestroyArray(LAllocR5C* pAlloc, uint32 nElements)
	{
		if (m_pArray)
		{
			if (m_pArray)
				pAlloc->Free(m_pArray);

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
		if (nNewSize > 0)
		{
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

	void	sub_10050284(uint32 nNewSize);
};



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

uint32 Arr1007c480::sub_1007c480(const GenList1007c480& theList)
{
	GenListPosR5C	pos;
	uint32			iCurOut, prevSize;

	prevSize = sub_1005f780();

	if(!sub_1007c730(prevSize + theList.GenGetSize()))
		return LTFALSE;

	iCurOut = prevSize;
	for(pos=theList.GenBegin(); theList.GenIsValid(pos); )
	{
		// BAD bug in theList!
		if(iCurOut >= sub_1005f780())
		{
			sub_1007ca30();
			return LTFALSE;
		}

		sub_1007c870(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}


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
	// Term.  The failure arm at 0x1005063c calls 0x1002ec30 (`movl
	// $0x10091df0,(%esp) ; mov %ebx,%ecx ; call 0x1002ec30`), d3dbatchc.cpp's
	// C1002ec30; this was declared at 0x10050710, which is not a function
	// (it is the middle of `mov 0x4(%eax),%edx` inside 0x100506f0).
	uint32		sub_1002ec30(LAllocR5C* pAlloc = &g_10091df0);
	T10050560&	sub_100507e0(uint32 nIndex) const;      // operator[]

	uint32		sub_10050560(const GenList10050560& theList);
};

uint32 Arr10050560::sub_10050560(const GenList10050560& theList)
{
	GenListPosR5C	pos;
	uint32			iCurOut;

	if(!sub_1002e7f0(theList.GenGetSize()))
		return LTFALSE;

	iCurOut = 0;
	for(pos=theList.GenBegin(); theList.GenIsValid(pos); )
	{
		// BAD bug in theList!
		if(iCurOut >= sub_1005f780())
		{
			sub_1002ec30();
			return LTFALSE;
		}

		sub_100507e0(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}


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
	void*					m_p5c;                      // +0x5c
	uint32					m_n60;                      // +0x60
	uint8					m_pad_64[0x68 - 0x64];
	D3DDevice7R5C*			m_pDev;                     // +0x68

	long	sub_1007dd24(D3DDevice7R5C* pDev, uint32 nPrimType,
						 uint32 nCount);
	long	sub_1007e1c0(D3DDevice7R5C* pDev, uint32 nPrimType,
						 uint32 nCount);
};



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
struct Batch1007e110
{
	uint8					m_pad_00[0x08];
	D3DVertexBuffer7R5C**	m_pVBs;                     // +0x08
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

	void	sub_1007dfa4();                             // 0x1007dfa4
	uint32	sub_1007e110();
};



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
//      constructor emits +0x00, +0x04, +0x08, +0x0c -- the same four
//      statements, reordered by the scheduler, which is why one Clear() is
//      written and used twice.
// ---------------------------------------------------------------------- //
struct Elem1007e2c0
{
	uint32	m_00;
	uint32	m_04;
	uint32	m_08;
	uint32	m_0c;

	void	Clear()	{ m_00 = 0; m_04 = 0; m_08 = 0; m_0c = 0; }

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
		Elem1007e2c0	*ptr, *pTemp;

		ptr = (Elem1007e2c0*)pAlloc->Alloc(sizeof(Elem1007e2c0) * nElements);
		if (!ptr)
			return LTNULL;

		pTemp = ptr;
		while (nElements)
		{
			--nElements;
			if (pTemp)
				pTemp->Clear();
			++pTemp;
		}

		return ptr;
	}

	void _DeleteAndDestroyArray(LAllocR5C* pAlloc, uint32 nElements)
	{
		if (m_pArray)
		{
			if (m_pArray)
				pAlloc->Free(m_pArray);

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
		if (nNewSize > 0)
		{
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
// LithTech's CMoArray<T> with its default cache policy -- d3dr4b.cpp
// decodes DefaultCache.  Only operator[] (0x1007c870) is reached here.
//
// The template's name is load-bearing, not cosmetic.  icl 4.0 lays out a
// call to a function whose (decorated) name contains "fault" -- or warn,
// err, fail, fatal, abort, exit, quit, halt, stop, trace, dump, alert,
// invalid, all measured in a probe TU -- as a COLD block, and
// ?$CMoArray@...@UDefaultCache@@ contains it.  So the m_VBs[i] store arm
// of 0x100038e0 is hosted past the epilogue, exactly as shipped; through
// the plain Arr1007c480 spelling it stayed in the loop (95 bytes differ).
struct DefaultCache;

template<class T, class C = DefaultCache>
class CMoArray
{
public:
	T&		operator[](uint32 nIndex) const;            // 0x1007c870

	uint8	m_pad_00[0x14];                             // Arr1007c480's 0x14
};

struct Batch100038e0
{
	uint8			m_pad_00[0x04];
	CMoArray<D3DVertexBuffer7R5C*>	m_VBs;              // +0x04
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



// ======================================================================= //
//  0x1007a014 -- 188 bytes, 1 caller (0x1007a13f).  IS THIS BLIT REQUEST
//  LEGAL?
//
//  WHAT PINS IT:
//    * `ret` with `this` in %ecx and a dead `push %esi` (pushed at entry,
//      released with `add $0x4,%esp` at both exits and never read) --
//      thiscall, no arguments, icl's four-byte dummy slot.
//    * six guarded `return LTFALSE`s off the members at +0x00, +0x04,
//      +0x10, +0x14, +0x1c and +0x20, the last two against -1, all sharing
//      the exit at 0x1007a05b, then `xor %eax,%eax ; inc %eax` for true.
//    * the magic values 4, 5 and 6 are tested for BOTH surfaces, and
//      d3dhigh.cpp already established (from the bytes-per-pixel table at
//      0x10089890, whose entries for 4, 5 and 6 are the only zeros) that
//      those three are the block-compressed formats.  Which is why the
//      guard that follows them is `(m_1c | m_20) & 3` -- a compressed
//      rectangle has to be 4-aligned.
//    * `or %ebx,%ebp` has +0x20 as the destination and +0x1c as the
//      source, so the source wrote `m_20 | m_1c` in that order.
//    * the last two tests re-read m_p00->m_04 and re-test `== 7` after the
//      `== 0` arm has already run (0x1007a0c7 jumps BACK to the `cmp $0x7`
//      at 0x1007a0b1).  Two separate `if`s, not an `if`/`else if`: an
//      `else if` would not re-test.
//    * 0x1007a0a0 is reached from FOUR places and 0x1007a065's
//      `test %ebx,%ebx` re-tests a value the path already knows -- that is
//      the `||` chain's second term being evaluated after the first has
//      failed, exactly as in 0x1007a700 below.
// ======================================================================= //
struct Surf1007a014
{
	uint8	m_pad_00[0x04];
	uint32	m_nFormat;                                  // +0x04
};

struct Blit1007a014
{
	Surf1007a014*	m_pSrc;                             // +0x00
	Surf1007a014*	m_pDst;                             // +0x04
	uint32			m_n08;                              // +0x08
	void*			m_p0c;                              // +0x0c
	void*			m_p10;                              // +0x10
	void*			m_p14;                              // +0x14
	uint32			m_n18;                              // +0x18
	uint32			m_n1c;                              // +0x1c
	uint32			m_n20;                              // +0x20

	LTBOOL	sub_1007a014() const;
};



// ======================================================================= //
//  0x1007a700 -- 164 bytes.  Its address is planted as an immediate at
//  0x1007b6b8 (a callback).  FILTER A LINKED LIST INTO A CALLER'S ARRAY.
//
//  WHAT PINS IT:
//    * a bare `ret` and `mov 0x10(%esp),%esi` before any frame: __cdecl
//      with three arguments, read at 0x10/0x14/0x18(%esp) after three
//      pushes.
//    * the walk is `mov (%esi),%ecx` then `cmp %esi,%ecx` -- ltlink.h puts
//      m_pPrev at +0x00 and m_pNext at +0x04, so this walks BACKWARDS from
//      the head, and `mov 0x8(%ecx),%edx` is m_pData at +0x08.
//    * `lea (%eax,%eax,4),%ebx ; mov %ebx,%ebp ; add %ebp,%ebp ;
//      mov 0x100895c0(%ebp,%ebx,2),%ebp` is 0x100895c0 + 20*index --
//      a table of TWENTY-byte records, and the index is
//      `movsbl 0xbc(%edx)`, a SIGNED char field.
//    * the predicate from +0x88 / +0x8c / 0x1008bd54 is the same one
//      d3dr3a.cpp reconstructed inside 0x1005d9c0, and 0x1008bd54 is
//      0x1008b880 + 0x4d4, the member 0x1007b8e0 reads through its own
//      parameter.  Here the ELSE arm is the other way round from
//      0x1005d9c0's -- `mov 0x1008bd54,%eax ; test ; je <continue>` means a
//      clear flag SKIPS -- so it is `if(!g || !(flags & 0x400)) continue;`.
//    * `test %ebx,%ebx ; jne <continue>` at 0x1007a765 re-tests `flags & 1`
//      on a path that has just proved it.  That is the if/else form's two
//      `continue`s being tail-merged: the then-arm's continue and the
//      else-arm's entry share one block, so the merged block has to re-ask
//      which arm it came from.  Written as one `||` it would not.
//    * the early `if (m_188) return;` really does leave the whole loop --
//      `pop %ebx ; pop %ebp ; pop %esi ; ret`, the function's own epilogue,
//      not the loop exit.
//    * the append reads the count, writes count+1 with
//      `mov $0x1,%esi ; add %ebp,%esi` (a NEW value, not an update),
//      and only then loads the array and stores through it.
// ======================================================================= //
struct Item1007a700
{
	uint8	m_pad_000[0x88];
	uint32	m_88;                                       // +0x88
	uint32	m_8c;                                       // +0x8c
	uint8	m_pad_90[0xbc - 0x90];
	char	m_bc;                                       // +0xbc
	uint8	m_pad_bd[0x188 - 0xbd];
	uint32	m_188;                                      // +0x188
};

struct Desc1007a700
{
	void*	m_p00;                                      // +0x00
	uint8	m_pad_04[0x14 - 0x04];                      // 20-byte stride
};

extern Desc1007a700	g_100895c0[];                       // 0x100895c0
extern uint32		g_1008bd54;                         // 0x1008bd54



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



// ======================================================================= //
//  0x10079b90 -- 219 bytes.  In .data at 0x100899bc.  PACK ONE PIXEL
//  THROUGH FOUR 256-BYTE CHANNEL TABLES.
//
//  WHAT PINS IT:
//    * a bare `ret` and `mov 0x18(%esp),%edi` after four pushes: __cdecl,
//      four arguments.
//    * `mov 0x18(%edi),%eax ; shl $0x8,%eax ; lea 0x70(%edx),%ecx ;
//      add %ecx,%eax` builds arg1 + 0x70 + (arg2->m_18 << 8) -- a 256-byte
//      table selected by a channel number -- and the same is done for
//      +0x1c, +0x20 and +0x24.  All four bases are computed BEFORE the
//      pixel is read, and all four are parked in the frame, so they are
//      four named locals in the source.
//    * the pixel is `mov (%ecx),%edx` off arg3 and is decomposed
//      `>> 0x18` (no mask -- the top byte needs none), `>> 0x10` then
//      `movzbl`, `>> 8` then `movzbl`, and `movzbl` alone.
//    * each table lookup is `movzbl` -- byte tables -- then `shl %cl` with
//      %cl from arg2's +0x28, +0x2c, +0x30 and +0x34 in that order, and the
//      four results are `or`ed into one accumulator left to right.
//    * the result is stored with `mov %esi,(%edx)` through arg4.
//
//  WHAT IS NOT EXPLAINED, and it is the reason to expect a near: arg3 and
//  arg4 are each passed through `and $0xffffffff,%reg` -- a no-op AND, six
//  bytes of it, twice.  No known source form produces that; it looks
//  like a mask whose width the front end folded to all-ones (a bitfield or
//  a `(uint32)` cast of something already 32 bits).  Everything else in the
//  function is straightforward and the shifts and tables are certain.
// ======================================================================= //
struct Fmt10079b90
{
	uint8	m_pad_00[0x18];
	uint32	m_nTab0;                                    // +0x18
	uint32	m_nTab1;                                    // +0x1c
	uint32	m_nTab2;                                    // +0x20
	uint32	m_nTab3;                                    // +0x24
	uint32	m_nShift0;                                  // +0x28
	uint32	m_nShift1;                                  // +0x2c
	uint32	m_nShift2;                                  // +0x30
	uint32	m_nShift3;                                  // +0x34
};



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
//  0x10011450 (stack entry) / 0x10011454 (register entry) -- 162 bytes.
//  SET STAGE 1'S COLOUR OP FOR ONE OF TWO DETAIL-TEXTURE MODES.
//
//  WHAT PINS IT:
//    * `cmp $0x1,%eax` then `cmp $0x2,%eax` with nothing in between and no
//      jump table: a two-case comparison chain on the argument, and the
//      default does nothing.
//    * 0x1008ae80 is the console variable "DetailTextureAdd" registered by
//      0x10014600 above.  Its value picks 8 over 4 as stage 1's COLOROP,
//      and 4 / 8 are D3DTOP_MODULATE / D3DTOP_ADDSIGNED.  The variable is
//      tested with `test %eax,%eax` on the dword at 0x1008ae80 -- the
//      NODE's first field, not the float at +0x08, so the flag is the
//      node's own +0x00.
//    * `mov 0x1008d988,%eax ; mov (%eax),%eax ; call *0x94(%eax)` is the
//      device loaded from a global POINTER, so `g_pDev->lpVtbl->...`.
//    * the second and third calls plant their constants as
//      `movl $imm,N(%esp)` while the first shares `%ecx = 1` for both the
//      stage and the state, which is icl reusing a materialised 1.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x10043660 (stack entry) / 0x10043664 (register entry) -- 275 bytes.
//  SAVE STAGE 1'S COLOUR AND ALPHA OPS AND INSTALL A FIXED COMBINE.
//
//  WHAT PINS IT:
//    * the argument is used as a DWORD* -- planted at 0x0c(%esp) for a
//      Get, then `add $0x4,%ebx` and planted again -- so it is a
//      two-element save area and the second slot is reached by advancing
//      the pointer, not by re-indexing it.
//    * two `call *0x90` (Get) then four `call *0x94` (Set), and the states
//      are 1 and 0xc for the Gets and 1, 2, 3 and 0xc for the Sets, with
//      values 7, 2, 1 and 3.  0xc is D3DTSS_ALPHAOP and 1 is
//      D3DTSS_COLOROP, so the pair saved is exactly the pair overwritten.
//    * the terminating `movl $0x7,0x1008edf0` records the installed mode
//      in a global.
// ---------------------------------------------------------------------- //


//  IDENTICAL once the overflow warning's callee is spelled sub_10021d60
//  (it was d3d_WarningR5C).  icl 4.0 takes a function named like a
//  warning (d3d_Warning*, Warning) for a cold diagnostic and sinks all
//  three calls to the end of the function; under a neutral name each
//  warning stays straight after its set's success epilogue, as shipped.
//  Measured in a probe TU: d3d_WarningV / Warning sink, sub_10021d60 /
//  xyz / dbgprint / Log do not -- varargs or not makes no difference.
void sub_100437f0(Poly100437f0* pPoly)
{
	C10091a10R5C*	pW;

	pW = sub_1007b8d0();

	if (pPoly->m_97 == 0xff)
	{
		if (pPoly->m_8c & 2)
			pW->m_184.Add(pPoly);
		else
			pW->m_144.Add(pPoly);
	}
	else
	{
		pW->m_164.Add(pPoly);
	}
}


//  IDENTICAL.  The loop condition is `i < m_nNumVBs && bOk` (the shipped
//  code increments and bound-checks before testing bOk, and the !bOk exit
//  lands straight on the 0x1007e030 call).  The store arm hosted past the
//  epilogue, and with it the swapped frame slots (this, the saved pVB),
//  came from m_VBs being the real CMoArray<> -- see the template above.
LTBOOL Batch100038e0::sub_100038e0(D3D7R5C* pD3D)
{
	D3DVertexBufferDescR5C	desc;
	D3DVertexBuffer7R5C*	pVB;
	uint32					i;
	LTBOOL					bOk;

	memset(&desc, 0, sizeof(desc));

	desc.dwSize = sizeof(desc);
	desc.dwCaps = D3DVBCAPS_R5C_WRITEONLY;

	if (m_n28)
		desc.dwFVF = 0x2c2;
	else
		desc.dwFVF = 0x2c4;

	if (!m_bVidMem)
		desc.dwCaps |= D3DVBCAPS_R5C_SYSTEMMEMORY;

	desc.dwNumVertices = m_nVertsPerVB;

	bOk = LTTRUE;
	for (i = 0; i < m_nNumVBs && bOk; i++)
	{
		if (pD3D->lpVtbl->CreateVertexBuffer(pD3D, &desc, &pVB, 0) < 0)
			bOk = LTFALSE;
		else
			m_VBs[i] = pVB;
	}

	if (!bOk)
		sub_1007e030();

	return bOk;
}
