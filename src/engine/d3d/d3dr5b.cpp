// *********************************************************************** //
//
//  MODULE:   D3DR5B (.CPP)  --  d3d.ren "batch2"
//
//  PURPOSE:  22 of the 25 rows of batch2.txt, decoded instruction by
//            instruction from the retail d3d.ren.  Intel C/C++ 4.0.
//
//  THREE ROWS ARE NOT WRITTEN, and none of them is a bug in the decode --
//  all three are the SAME wrong-size-band mistake, and the listing row
//  simply ends where the next enumerated start happens to fall:
//
//    0x1007c890  listing says 99 bytes.  Its first branch is
//                `jae 0x1007c907`, which is already past the claimed end,
//                and the body runs to 0x1007c9c3 -- 307 bytes, four loop
//                nests, two allocator round trips.
//    0x1000a220  listing says 134.  Same shape, same callees
//                (0x1000a690 / 0x1000a670 / 0x1000a680), ends 0x1000a3c0:
//                416 bytes.
//    0x1002e820  listing says 137.  Ends past 0x1002e990: ~420 bytes.
//
//  All three are ONE template three times -- `CMoArray<T>::Remove(DWORD,
//  LAlloc*)` over element sizes 4 (0x1007c890), 0x10 (0x1000a220) and 0x0c
//  (0x1002e820); the element size is read straight off the copy loops
//  (`mov 0x4(%edx,%eax,4)`, `shl $0x4` twice, `lea 0x8(,%edx,8)` +
//  `lea 0x4(%eax,%edx,4)`).  They belong with d3dbatcha/d3dbatchd's
//  CMoArray family in a batch sized for 300-byte bodies, not here.
//
//  TU FLAG (the flag tells read off each body).  This file is /O2:
//  eighteen of the twenty-two bodies carry the /O2 outgoing-argument idiom
//  (`add $-N,%esp` then `mov ...,(%esp)`) and/or a DUPLICATED epilogue.
//  FOUR disagree and want /Os -- they cross-jump their second and later
//  exits and push their outgoing arguments:
//
//      0x1007e3c0   `eb d5` into the shared epilogue, no outgoing area
//      0x1007e030   `eb e9`, `eb df`, `eb ae`, and `push %eax` for the free
//      0x1007dfa4   `eb f5`, `eb cf`  (but its own COM call uses mov-arg,
//                   so this one is genuinely mixed -- see its banner)
//      0x10059ef0   `push $1 ; push %eax`, and `eb cb`
//
//  The first three are neighbours in the shipped image (0x1007dfa4,
//  0x1007e030, 0x1007e3c0) and are almost certainly one shipped /Os TU;
//  0x10059ef0 is on its own.  Those four belong in a split-out /Os file.
//
//  NOW /Os /Qip (build/engine.sh).  The file was built /Os, and the
//  templates it instantiates for 0x10059ef0 -- CMoArray<SavedVert589c4>'s
//  virtuals and ObjectBank<Node56ab4>'s members -- then came out INLINED
//  (GenAppend 356 bytes, GenRemoveAt 412 ...) where the shipped ones
//  (0x1005a0b0..0x1005a386, 0x10050d00..0x10050dd9) call each other out of
//  line: those COMDATs were code the shipped d3d.ren does not have.  Built
//  /Os /Qip, the stock dynarray.h / object_bank.h bodies are the shipped
//  bytes (GenFindElement, GenAppendList, GenCopyList, GenRemoveAll,
//  GenRemoveAt, GenAppend, GenGetAt, GenGetNext, Allocate, AllocVoid,
//  FreeVoid, Free, the node constructor), and every function of this file
//  that matched at /Os still matches.  (/O2 /Qip or /Qip_no_inlining
//  give other shapes; plain /Os inlines.)
//
//  TWO ROWS HAVE A REGISTER ENTRY AND WILL BE FOUR BYTES SHORT.
//  0x10043780 and 0x10026500 each begin with a four-byte stack shim
//  (`mov 0x4(%esp),%eax`) that falls through into the real prologue at
//  +4, and only SOME callers use the shim.  icl emits that pair when it
//  can see every caller of a static function; in a file that holds none of
//  the callers it will emit the stack form only.  The bodies below are
//  written to match from +4 (0x10043784 / 0x10026504).
//
//  ONE CALLEE IS REGISTER-ABI.  0x1007a680's callee 0x1007a470 takes the
//  object in %eax; that row has moved to d3dqip4.cpp (/Qip), together
//  with the callee, where all three functions of the family match.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"
#include "../../LT2/lithshared/stdlith/object_bank.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// (0x10021d60 AddDebugMessage and the 0x1007b8d0 context's sets went to
// d3dr5b_o2.cpp with the two functions that use them.)


// ---------------------------------------------------------------------- //
//  The renderer's allocator, spelled exactly as d3dbatcha.cpp spells it:
//  the object into %ecx, one argument pushed, the vptr out of (%ecx), the
//  slot out of the vtable and `call *%eax`.  0x10091df0 is stdlith's
//  g_DefAlloc; declared, never defined, so no vtable is emitted here.
//  Named as d3dr5a.cpp names it: see 0x1007e030 for why the name matters.
// ---------------------------------------------------------------------- //
struct IAllocator
{
	virtual void*	Slot00(uint32 nSize);               // vtable +0x00
	virtual void*	Alloc(uint32 nSize);                // vtable +0x04
	virtual void	Free(void* pMem);                   // vtable +0x08
};

extern IAllocator g_10091df0;                           // 0x10091df0


// ---------------------------------------------------------------------- //
//  The C-style COM interfaces this batch reaches.  Every one of them is
//  called the way d3dbatcha.cpp already records: the interface pointer
//  PUSHED as the first argument and the slot taken out of `(*p)`, which is
//  what separates a DirectX vtable call from the C++ virtual calls above.
// ---------------------------------------------------------------------- //

// IDirectDrawSurface7 -- only BltFast (+0x1c) is reached from here.
struct DDSurface;
struct DDSurfaceVtbl
{
	void* m_pad_00[7];                                  // +0x00 .. +0x18
	long (__stdcall* BltFast)(DDSurface*, uint32 dwX, uint32 dwY,
	                          DDSurface* pSrc, void* pSrcRect,
	                          uint32 dwTrans);           // +0x1c
};
struct DDSurface { DDSurfaceVtbl* lpVtbl; };

extern DDSurface* g_pOffscreen;                         // 0x1008bf68

// IDirect3DDevice7 -- SetTexture (+0x8c) and SetTextureStageState (+0x94).
struct D3DDevice;
struct D3DDeviceVtbl
{
	void* m_pad_00[35];                                 // +0x00 .. +0x88
	long (__stdcall* SetTexture)(D3DDevice*, uint32 nStage,
	                             void* pTexture);        // +0x8c
	long (__stdcall* SetTextureStageState)(D3DDevice*, uint32 nStage,
	                                       uint32 nState,
	                                       uint32 nValue);// +0x94
};
struct D3DDevice { D3DDeviceVtbl* lpVtbl; };

extern D3DDevice* g_pD3DDevice;                         // 0x1008d988

// IDirect3DVertexBuffer7 -- Lock is +0x0c.
struct D3DVertBuf;
struct D3DVertBufVtbl
{
	void* m_pad_00[3];                                  // +0x00 .. +0x08
	long (__stdcall* Lock)(D3DVertBuf*, uint32 dwFlags,
	                       void** ppData, uint32* pdwSize);// +0x0c
};
struct D3DVertBuf { D3DVertBufVtbl* lpVtbl; };

// The generic release-only interface the array teardown at 0x1007e030
// walks: it pushes the object and calls slot +0x08.
struct DDRelease;
struct DDReleaseVtbl
{
	void*  m_pad_00[2];                                 // +0x00 .. +0x04
	uint32 (__stdcall* Release)(DDRelease*);            // +0x08
};
struct DDRelease { DDReleaseVtbl* lpVtbl; };


// ---------------------------------------------------------------------- //
//  0x10030f30 -- the DDERR/D3DERR-to-string table (1968 bytes, decoded and
//  rejected by d3dneutral.cpp).  ONE argument: `mov 0x4(%esp),%eax`.  Its
//  four call sites all sit in the 0x10036xxx block and all four do the same
//  two things with it -- park the string in the global at 0x1008da30 and
//  test the HRESULT separately -- so the pair below is one source idiom.
//
//  The extra `push %edi` in front of the argument push is NOT a second
//  argument: at 0x100364f1 the same pair is followed by `add $0x4,%esp`,
//  cleaning one dword, so the leading push is icl reserving outgoing space.
// ---------------------------------------------------------------------- //
extern const char* __cdecl sub_10030f30(long hResult);  // 0x10030f30

extern const char* g_1008da30;                          // 0x1008da30


// ---------------------------------------------------------------------- //
//  0x10021f40 -- the console-variable node constructor, already pinned by
//  d3dr4b.cpp / d3dr4c.cpp: __thiscall, `ret $0x10`, four arguments, the
//  float arriving as a raw immediate because it is a float PARAMETER with a
//  constant argument.
// ---------------------------------------------------------------------- //
struct CConVar
{
	void sub_10021f40(const char* pszName, float fValue,
	                  uint32 nA, uint32 nB);            // 0x10021f40
};


// ---------------------------------------------------------------------- //
//  0x1000eec0 -- `mov 0x20(%esp),%edx ; mov 0x1c(%esp),%ebx ;
//  mov 0x18(%esp),%eax ; mov %edx,0x8(%ecx) ; mov %ebx,0x4(%ecx) ;
//  mov %eax,(%ecx) ; ret $0xc`.  __thiscall, three dwords, stored to
//  +0x08 / +0x04 / +0x00 in that order.  That is LTVector::Init(x,y,z)
//  compiled OUT OF LINE -- which is why it must have its own declaration
//  here rather than the SDK's inline one: /Ob1 would expand the SDK copy.
// ---------------------------------------------------------------------- //
struct D3DVec3
{
	float x;                                            // +0x00
	float y;                                            // +0x04
	float z;                                            // +0x08

	void sub_1000eec0(float fX, float fY, float fZ);    // 0x1000eec0
};


// ======================================================================= //
//  0x10059210 -- 90 bytes.  Transform one texture-coordinate pair.
//
//      55 53                    %ebp and %ebx as ordinary registers: no
//                               frame pointer at all
//      8b 5c 24 14 / 8b 44 24 10 / 8b 54 24 0c   the three arguments
//      56                       icl's dead 4-byte slot (undone by
//                               `add $0x4,%esp`, never popped)
//      d9 81 98 07 / d8 89 90 07 / 8b 2a / d8 45 00 / d9 18
//      d9 81 9c 07 / d8 89 94 07 / 8b 12 / d8 42 08
//      d9 00 / d9 c9 / d9 58 04
//      d8 8c d9 70 07 / d9 40 04 / d9 c9 / d9 18
//      d8 8c d9 74 07 / d9 58 04
//      83 c4 04 / 5b / 5d / c2 0c 00
//
//  WHAT PINS IT:
//    * `ret $0xc` with %ecx read and never loaded: __thiscall, three
//      dwords.
//    * `mov (%edx),%ebp` and then `mov (%edx),%edx` -- the SAME dword read
//      TWICE, with an `fstps (%eax)` between them.  That is the store
//      through the out pointer forcing a reload, so the source names the
//      pointer member twice; it is not a cached local.
//    * `flds (%eax)` immediately after the second `fadds`, and
//      `flds 0x4(%eax)` after the first `fmuls`: both output components are
//      READ BACK, so the last two statements are multiplies INTO the
//      out-pair, not part of the two expressions above them.  Four
//      statements, not two.
//    * `fmuls 0x770(%ecx,%ebx,8)` fixes the element width at eight bytes
//      and the array base at this+0x770 -- the same array d3dhigh.cpp's
//      0x1005a050 already found and the same one 0x10059fc0 below uses.
// ======================================================================= //

struct TexPair
{
	float u;                                            // +0x00
	float v;                                            // +0x04
};

// The pointer 0x10059210's first argument points at: its one interesting
// field is the vector pointer at +0x00, read for `.x` and `.z`.
struct TexSrc
{
	D3DVec3* m_pVec;                                    // +0x00
};

// The 0x1005xxxx block's one big object.  Four of this batch's functions
// live on it and they corroborate each other: 0x10059210 uses 0x790..0x79c
// and the 0x770 pair array, 0x100598f0 uses 0x7a0..0x7a8, 0x10059300 uses
// 0x7ac..0x7b4 and 0x10059fc0 uses 0x770, 0x788 and 0x78c.
// __single_inheritance is not decoration: without it cl/icl represent a
// pointer to member of an INCOMPLETE class in the 16-byte general form and
// the call stops being `mov %ecx,this ; call *%eax`.
struct __single_inheritance CTexStage;
struct Color100598f0;
struct Vertex100598f0;

typedef void (CTexStage::*PFNTexStage)(void* pDest, TexPair* pPair,
                                       uint32 nIndex);

struct CTexStage
{
	uint8       m_pad_000[0x770];                       // +0x000
	TexPair     m_Pairs[3];                             // +0x770
	TexPair     m_Cur788;                               // +0x788
	float       m_f790;                                 // +0x790
	float       m_f794;                                 // +0x794
	float       m_f798;                                 // +0x798
	float       m_f79c;                                 // +0x79c
	float       m_f7a0;                                 // +0x7a0
	float       m_f7a4;                                 // +0x7a4
	float       m_f7a8;                                 // +0x7a8
	PFNTexStage m_pfn7ac;                               // +0x7ac
	PFNTexStage m_pfn7b0;                               // +0x7b0
	PFNTexStage m_pfn7b4;                               // +0x7b4

	void sub_10059210(TexSrc* pSrc, TexPair* pOut, uint32 nIndex);
	void sub_10059300(void* pDest, TexPair* pPairs, uint32 nUnused);
	void sub_100598f0(const Color100598f0* pSrc, Vertex100598f0* pDest);
	uint32 sub_10059fc0(void* pThing, uint32 nIndex);
};

void CTexStage::sub_10059210(TexSrc* pSrc, TexPair* pOut, uint32 nIndex)
{
	pOut->u = m_f798 * m_f790 + pSrc->m_pVec->x;
	pOut->v = m_f79c * m_f794 + pSrc->m_pVec->z;

	pOut->u *= m_Pairs[nIndex].u;
	pOut->v *= m_Pairs[nIndex].v;
}


// ======================================================================= //
//  0x10064640 and 0x100408c0 -- MOVED to d3dr5b_o2.cpp.  Both duplicate
//  their epilogue (the /O2 shape) where this file is /Os; 0x100408c0 is
//  identical there, 0x10064640 size-exact.
// ======================================================================= //

// ======================================================================= //
//  0x10015830 -- 102 bytes.  A constructor: three vectors zeroed through
//  the out-of-line Init, four dwords zeroed inline, `this` in %eax.
//
//      55 8b ec / 83 ec 18 / 57 53      (%edi pushed and never used --
//                                        icl's dead slot, undone by
//                                        `mov %ebp,%esp`)
//      8b d9 / 33 c0 / 83 c4 f4
//      89 03 / 89 43 04                 m_00 = m_04 = 0
//      89 44 24 04 / 89 44 24 08 / 8d 4b 08 / 89 04 24 / e8 <0x1000eec0>
//      ... twice more, at +0x14 and +0x20 ...
//      33 c0 / 89 43 30 / 89 43 2c
//      8b c3                            return this
//      5b / 8b e5 / 5d / c3
//
//  WHAT PINS IT:
//    * `mov %ebx,%eax` on the way out of a bare `ret` with no arguments is
//      a constructor returning `this`.
//    * three calls to 0x1000eec0 at +0x08, +0x14 and +0x20 -- TWELVE bytes
//      apart -- with three zero dwords each.  That is three LTVector
//      members and Init(0,0,0), and it is why the vector type here has its
//      own out-of-line Init instead of the SDK's inline one.
//    * `xor %eax,%eax` re-materialised before EACH of the three argument
//      groups: the zero register is spent and re-made per statement, which
//      is what separate statements look like.
//    * the two trailing stores go 0x30 then 0x2c.  That ordering is the
//      known zero-store-grouping class and is not source-order
//      controllable; both are written in ascending order below.
// ======================================================================= //
struct C10015830
{
	uint32   m_n00;                                     // +0x00
	uint32   m_n04;                                     // +0x04
	D3DVec3  m_v08;                                     // +0x08
	D3DVec3  m_v14;                                     // +0x14
	D3DVec3  m_v20;                                     // +0x20
	uint32   m_n2c;                                     // +0x2c
	uint32   m_n30;                                     // +0x30

	C10015830();
};

C10015830::C10015830()
{
	m_n00 = 0;
	m_n04 = 0;

	m_v08.sub_1000eec0(0.0f, 0.0f, 0.0f);
	m_v14.sub_1000eec0(0.0f, 0.0f, 0.0f);
	m_v20.sub_1000eec0(0.0f, 0.0f, 0.0f);

	m_n2c = 0;
	m_n30 = 0;
}


// 0x10043780 (body at 0x10043784) MOVED to w35_10043e10_qip.cpp,
// beside 0x10043660: its register-entry caller is 0x10043e10's cold
// tail (0x100442c7), and there both entries are identical.


// ======================================================================= //
//  0x10050830 -- 110 bytes.  Build the static quad index list, once.
//
//      53 / 83 ec 0c / a1 <0x1008efd0> / 85 c0 / 75 5c
//      c7 05 <0x1008efd0> 01
//      33 c9 / ba <0x1008efd4> / 33 db
//      89 74 24 08 / 89 7c 24 04
//    L: 8d 41 03 / 43 / bf fd ff ff ff / 03 f8 / 8d 70 ff
//       66 89 42 0a / 66 89 7a 06 / 66 89 72 08
//       8d 41 02 / 8d 71 01 / 66 89 42 04 / 66 89 72 02 / 66 89 0a
//       83 c2 0c / 83 c1 04 / 81 fb 80 00 00 00
//       8b 7c 24 04 / 8b 74 24 08 / 72 bf
//      83 c4 0c / 5b / c3
//
//  WHAT PINS IT:
//    * six 16-bit stores per trip at +0x00,+2,+4,+6,+8,+0xa with the
//      values c, c+1, c+2, c, c+2, c+3 where c steps by four: that is the
//      quad-to-two-triangles index list, 0,1,2, 0,2,3.
//    * `add $0xc,%edx` and `add $0x4,%ecx` are two strength-reduced
//      induction variables off ONE source counter; the counter itself
//      survives in %ebx only as the trip count (`cmp $0x80,%ebx`), which
//      is what an INDEXED source loop leaves behind (loops want
//      the array indexed, not walked).  128 quads, 768 indices.
//    * `mov $0xfffffffd,%edi ; add %eax,%edi` re-derives c from c+3.  That
//      is icl reusing the `lea 0x3(%ecx)` it just made, not a source term.
//    * the guard jumps to the ONE epilogue, so the whole body is inside
//      `if (!flag)`, not behind an early return.
//    * the %esi/%edi saves are hoisted out of the loop but their RESTORES
//      sit inside it, every trip.  icl's own shrink-wrapping; nothing in
//      the source asks for it.
//    * IDENTICAL as written below: a walking `uint16* p` (+0xc per trip,
//      base in a register -- the indexed `g[i * 6 + k]` form addressed the
//      array absolutely, 116 bytes with push/pop saves), and the second
//      triangle's three values taken from a uint16 `v = c + 3`: the
//      truncation stops icl folding `v - 3` back to `c`, which is where
//      `mov $0xfffffffd,%edi ; add %eax,%edi` comes from (with a uint32 v
//      it folds).  `c += 4` before `p += 6` orders the two adds.
// ======================================================================= //
extern uint32 g_1008efd0;                               // 0x1008efd0
extern uint16 g_1008efd4[768];                          // 0x1008efd4

void sub_10050830()
{
	if (!g_1008efd0)
	{
		uint32  i;
		uint32  c;
		uint16  v;
		uint16* p;

		g_1008efd0 = 1;

		c = 0;
		p = g_1008efd4;
		for (i = 0; i < 128; i++)
		{
			p[0] = (uint16)c;
			p[1] = (uint16)(c + 1);
			p[2] = (uint16)(c + 2);
			v = c + 3;
			p[3] = (uint16)(v - 3);
			p[4] = (uint16)(v - 1);
			p[5] = (uint16)v;
			c += 4;
			p += 6;
		}
	}
}



// ======================================================================= //
//  0x1007e3c0 -- 112 bytes.  Find a two-key record in a 16-byte array and
//  remember where it was.   /Os.
//
//      55 8b ec / 83 ec 10 / 56 53 / 8b f1
//      8b 46 5c / 85 c0 / 75 54        the flush, out of line
//      33 d2 / 8b 46 24 / 85 c0 / 76 1e
//      8b 45 0c / 8b 5d 08 / 8b ce / 89 45 f4 / 33 c0
//    L: 8b 71 40 / 3b 1c 06 / 74 13
//    N: 42 / 83 c0 10 / 3b 51 24 / 72 ef
//      33 c0                            <- return 0
//    E: 5b 5e 8b e5 5d c2 08 00
//    M: 8b 71 40 / 89 7d f8 / 8b 7d f4 / 3b 7c 06 04 / 8b 7d f8 / 75 db
//      8b f1 / 33 c0 40 / 89 56 18 / c7 46 1c 00 / eb d5
//      8b ce / e8 <0x1007e7f4> / eb a3
//
//  WHAT PINS IT:
//    * `eb d5` -- the success arm CROSS-JUMPS into the failure arm's
//      epilogue instead of duplicating it.  That is the /Os
//      pattern, and with no outgoing area anywhere in the body it is the
//      only flag evidence this function offers.  `33 c0 40` for `return 1`
//      agrees.
//    * `mov 0x40(%ecx),%esi` appears TWICE, once per compare, so the two
//      key tests are two separate member expressions -- no cached array
//      pointer in the source.
//    * `cmp (%esi,%eax,1),%ebx` is Intel `cmp ebx,[...]`, so the PARAMETER
//      is the source's left operand in both compares.
//    * `add $0x10,%eax` beside `inc %edx` fixes the element at 16 bytes
//      with the keys at +0x00 and +0x04.
//    * the 0x5c test is a plain `if`, not a loop guard: its call returns
//      to the top of the search (`eb a3` back to `33 d2`).
// ======================================================================= //
// NEAR (84 -> 70 bytes differ).  Measured on a probe:
//    * the flush is out of line in the shipped code: `switch (m_n5c)
//      { case 0: break; default: ... }` (the sink-the-default lever) does
//      it, as does the first key test's switch for the out-of-line second
//      compare;
//    * the base at +0x40 is re-read for EACH compare: an inline
//      operator[] on an array subobject (as d3dr4b.cpp's 0x1007e7c0);
//    * the count at +0x24 is re-read every iteration: an inline GetSize()
//      on a SUBOBJECT at +0x20 (an accessor on the class itself, or the
//      bare member, is hoisted into %edi);
//    * the keys stay uint32: big17c14.cpp's D3DObject* first key puts i
//      in %edx and the offset in %eax as shipped, but is 72 bytes off
//      against these 70, and the aliases name this signature.
//  What is left: the shipped code hoists BOTH keys out of the loop (the
//  object into %ebx, the colour into a frame slot at -0xc, frame 0x10,
//  %edi borrowed around the second compare) where ours re-reads them from
//  the argument slots; `this` lives in %esi there, %ebx here.  Local
//  copies of the keys fold away; an inline Find(pObject, nColor) member
//  (return Find(..) / if (Find(..)) return 1;) is worse (76 / 72).
// 70 -> 22 bytes differ (the shipped length).  The earlier
// finding (w59_1007dd24_osoi.cpp): the shipped "frame-save" style -- %edi
// saved into a frame slot (-0x8) around its one short use, the colour
// key parked in -0xc, frame 0x10 -- is what icl emits for INLINED code.
// Here it takes two things together, each measured alone:
//    * m_Recs is the REAL StdLith CMoArray<Rec1007e3c0> (dynarray.h) at
//      +0x3c, so m_Recs[i] is the inlined operator[] -> Get() off m_pArray
//      at +0x40 (the class layout is w10_1007a280_os.cpp's D3DModelCache:
//      a CMoArray of vertex buffers at +0x04, this array at +0x3c);
//    * the flush test is an inline member holding the `switch` sink
//      (CheckFlush below).  Written in the body instead (as before), the
//      same CMoArray gives 98 bytes / 72 differ; the inline member with an
//      `if` gives 115 / 97.
// What is left (22 bytes, all in the loop's pre-header): icl schedules
// the two key loads and the -0xc store ABOVE the count guard's `jbe`
// (count in %ecx), where the shipped code tests the count in %eax first
// and loads the keys after it.  Measured without effect: nested ifs,
// `continue` tests, a while loop, operands swapped, Get(i) for [i], a
// Rec& / Rec* local (99 / 87 worse), an inline Find(a, b) member
// (if (Find(..)) return 1: same 22; return Find(..): 73), an inline
// Rec::Is(a, b) (87), an explicit count guard + do/while (90), a break
// then test after the loop (79), the stores swapped.
struct Rec1007e3c0                                      // 0x10 bytes
{
	uint32		m_nKeyA;                                // +0x00 (the object)
	uint32		m_nKeyB;                                // +0x04 (the colour)
	uint8		m_pad_08[0x08];                         // +0x08
};

struct Cnt1007e3c0
{
	uint32	GetSize() const { return m_nCount; }

	uint32	m_n00;                                      // +0x00
	uint32	m_nCount;                                   // +0x04
};

struct C1007e3c0
{
	uint8						m_pad_00[0x18];         // +0x00
	uint32						m_n18;                  // +0x18
	uint32						m_n1c;                  // +0x1c
	Cnt1007e3c0					m_Cnt;                  // +0x20 (count +0x24)
	uint8						m_pad_28[0x3c - 0x28];  // +0x28
	CMoArray<Rec1007e3c0>		m_Recs;                 // +0x3c (array +0x40)
	uint8						m_pad_50[0x5c - 0x50];  // +0x50
	uint32						m_n5c;                  // +0x5c

	void   sub_1007e7f4();                              // 0x1007e7f4
	uint32 sub_1007e3c0(uint32 nKeyA, uint32 nKeyB);

	void   CheckFlush() { switch (m_n5c) { case 0: break; default: sub_1007e7f4(); } }
};

uint32 C1007e3c0::sub_1007e3c0(uint32 nKeyA, uint32 nKeyB)
{
	uint32 i;

	CheckFlush();

	for (i = 0; i < m_Cnt.GetSize(); i++)
	{
		if (nKeyA == m_Recs[i].m_nKeyA && nKeyB == m_Recs[i].m_nKeyB)
		{
			m_n18 = i;
			m_n1c = 0;
			return 1;
		}
	}

	return 0;
}


// ======================================================================= //
//  0x10059300 -- 116 bytes.  Three stage setters off three member function
//  pointers.
//
//      55 8b ec / 83 ec 14 / 57 56 53
//      83 c4 f4 / 8b 7d 08 / 8b 75 0c / 8b d9
//      c7 44 24 08 00 / 89 74 24 04 / 8b 83 ac 07 / 89 3c 24 / 8b cb / ff d0
//      83 c4 f4 / 8d 46 08 / c7 44 24 08 01 / ... 8b 83 b0 07 / ff d0
//      83 c4 f4 / 8d 76 10 / c7 44 24 08 02 / ... 8b 83 b4 07 / ff d0
//      5b 5e 5f 8b e5 5d c2 0c 00
//
//  WHAT PINS IT:
//    * `mov 0x7ac(%ebx),%eax ; mov %ebx,%ecx ; call *%eax` -- the callee
//      address comes out of the OBJECT, not out of a vtable, and `this`
//      goes in %ecx.  That is a pointer to member function, and it is why
//      the three slots are typed that way below rather than as plain
//      function pointers (a plain pointer would push the object).
//    * the three arguments are (arg1, &arg2[k], k) with k = 0, 1, 2 --
//      the second argument advances by EIGHT, which matches the pair type
//      the rest of this object uses.
//    * `ret $0xc` against only two arguments read: the third parameter is
//      dead.
//    * three `add $0xfffffff4,%esp` outgoing areas: /O2.
// ======================================================================= //
void CTexStage::sub_10059300(void* pDest, TexPair* pPairs, uint32 nUnused)
{
	(this->*m_pfn7ac)(pDest, &pPairs[0], 0);
	(this->*m_pfn7b0)(pDest, &pPairs[1], 1);
	(this->*m_pfn7b4)(pDest, &pPairs[2], 2);
}


// ======================================================================= //
//  0x1007e030 -- 108 bytes.  Release every element, then give the array
//  back.   /Os.
//
//      55 8b ec / 83 ec 10 / 53 / 8b d9
//      33 d2 / 8b 43 0c / 85 c0 / 76 12
//    L: 8b 43 08 / 8b 04 90 / 85 c0 / 75 3e
//    N: 42 / 8b 43 0c / 3b d0 / 72 ee
//    A: 8b 43 08 / 85 c0 / 75 0c
//    Z: c7 43 0c 00 / 5b 8b e5 5d c3
//       75 09 / c7 43 10 00 / eb e9
//       b9 <0x10091df0> / 74 08 / 8b 11 / 50 / 8b 42 08 / ff d0
//       c7 43 08 00 / eb df
//    R: 8b 4b 08 / 8b 0c 91 / 89 55 fc / 8b 11 / 51 / ff 52 08
//       8b 55 fc / eb ae
//
//  WHAT PINS IT:
//    * THREE conditional branches off ONE `test %eax,%eax` (`75 0c`,
//      `75 09`, `74 08`).  icl folds none of them, so the source really
//      does test the array pointer three times -- the same double-test
//      d3dbatcha.cpp already records for 0x1001cf00, with one more test
//      on the outside.  Do not "fix" it.
//    * the p==0 path stores ONLY +0x0c; +0x08 and +0x10 are inside the
//      guard.  That fixes +0x08 as the buffer, +0x10 as the capacity and
//      +0x0c as the element count, and it is the same field triple the
//      inlined teardown at 0x10059ef0 below uses.
//    * the element release PUSHES the element and takes slot +0x08 out of
//      `(*p)` -- a C COM Release, not a C++ virtual (a virtual would put
//      the object in %ecx and push nothing).
//    * `mov 0x8(%ebx),%eax` at the top of the loop and `mov 0x8(%ebx),%ecx`
//      again inside the release: two source-level member expressions.
//    * `eb e9`, `eb df`, `eb ae` and the pushed free argument: /Os.
//
//  IDENTICAL.  The object is the vertex-buffer class whose
//  destructor 0x1007dcc0 and 0x1007def0 sit beside it (0x1007def0 carries
//  an inlined copy of this very body), and +0x04 is a CMoArray (vptr
//  0x10089a60) of COM pointers: the loop is `GetSize()` / `operator[]`
//  and the tail is CMoArray::Term -- SetSize2 -> _DeleteAndDestroyArray ->
//  BaseDelete, dynarray.h's chain, written out here.  What it took:
//    * the chain must be its own template, class template over <T, C>
//      (a one-parameter template inlines the release into the loop);
//    * ICL'S BLOCK LAYOUT HERE DEPENDS ON NAMES.  Measured on a one-file
//      probe, identical source otherwise: the allocator type spelled
//      `IAllocator` (d3dr5a.cpp's spelling) lays the array test out of
//      line as shipped -- AAlloc, BAlloc, JAlloc, KAlloc, LAlloc, LAllocX,
//      MAlloc, ZAlloc, IA, IZ all give the inline `je` form; and with
//      the cache class spelled `DefaultCache` (dynarray.h's, used here)
//      the release goes out of line as shipped, where DefCache1007e270,
//      DCache, DefCache, Cache1 keep it inline.  The REAL dynarray.h
//      (LAlloc) gives the inline `je` form, as in 0x10059ef0, which
//      matches with it.  So the allocator here is d3dr5a's IAllocator (this
//      file's former CD3DAlloc, which nothing else used);
//    * the vtable read into a local before the call (`mov (%ecx),%edx ;
//      push %ecx ; call *0x8(%edx)`); calling through p->lpVtbl directly,
//      or a C++ __stdcall virtual, loads it into %ecx after the push.
// ======================================================================= //
template<class T>
inline void BaseDelete(IAllocator* pAlloc, T* ptr, uint32 nElements)
{
	T* pTemp;

	if (!ptr)
		return;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		pTemp->~T();
		++pTemp;
	}

	pAlloc->Free(ptr);
}

template<class T, class C>
struct MoArray
{
	void	Term(IAllocator* pAlloc = &g_10091df0)	{ SetSize2(0, pAlloc); }
	T&		operator[](const uint32 index) const	{ return Get(index); }
	LTBOOL	SetSize2(uint32 newSize, IAllocator* pAlloc);
	uint32	GetSize() const							{ return m_nElements; }
	T&		Get(uint32 index) const					{ return m_pArray[index]; }
	void	_DeleteAndDestroyArray(IAllocator* pAlloc, uint32 nElements);
	uint32	GetNumAllocatedElements() const			{ return m_nElements + m_Cache.GetCacheSize(); }

	void**	m_pVTable;                                  // +0x00
	T*		m_pArray;                                   // +0x04
	uint32	m_nElements;                                // +0x08
	C		m_Cache;                                    // +0x0c
};

template<class T, class C>
inline void MoArray<T,C>::_DeleteAndDestroyArray(IAllocator* pAlloc, uint32 nElements)
{
	if (m_pArray)
	{
		BaseDelete(pAlloc, m_pArray, nElements);
		m_pArray = LTNULL;
	}

	m_Cache.SetCacheSize(0);
}

template<class T, class C>
inline LTBOOL MoArray<T,C>::SetSize2(uint32 newSize, IAllocator* pAlloc)
{
	if ((newSize == m_nElements) && (newSize != 0))
	{
		return LTTRUE;
	}

	if (m_pArray)
	{
		_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
	}

	m_nElements = newSize;
	if (newSize > 0)
	{
		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
	}

	return LTTRUE;
}

struct C1007e030
{
	void*							m_pVTable;          // +0x00
	MoArray<DDRelease*, DefaultCache>	m_Array;        // +0x04 (array +0x08)

	void sub_1007e030();
};

void C1007e030::sub_1007e030()
{
	uint32 i;

	for (i = 0; i < m_Array.GetSize(); i++)
	{
		if (m_Array[i])
		{
			DDRelease*     p     = m_Array[i];
			DDReleaseVtbl* pVtbl = p->lpVtbl;

			pVtbl->Release(p);
		}
	}

	m_Array.Term();
}


// ======================================================================= //
//  0x1007a680 now lives in d3dqip4.cpp with its callees 0x1007a470 and
//  0x1007a580: the call hands the object over in %eax, which only a /Qip
//  TU that holds the callee as well can emit.  All three are identical
//  there.
// ======================================================================= //


// ======================================================================= //
//  0x10059ef0 -- 99 bytes.  DrawPolyMgr's destructor.   /Os.
//
//      55 8b ec / 83 ec 08              (the 8 is never used: dead slot)
//      b8 <0x1008fdf4>
//      c7 05 <0x1008fdf4> <0x100897d0>
//      83 c0 04 / 6a 01 / 50 / e8 <0x1007fba0> / 83 c4 08
//      a1 <0x1008fde0>
//      c7 05 <0x1008fdf4> <0x100894c0>
//      85 c0
//      c7 05 <0x1008fddc> <0x100897a0>
//      75 0e
//    Z: c7 05 <0x1008fde4> 00 / 8b e5 5d c3
//      74 19 / b9 <0x10091df0> / 74 08 / 8b 11 / 50 / 8b 42 08 / ff d0
//      c7 05 <0x1008fde0> 00
//      c7 05 <0x1008fde8> 00 / eb cb
//
//  IDENTICAL.  It is DrawPolyMgr's destructor thunk: the
//  atexit(0x10059ef0) that w27_10059c80_osoi.cpp's static initialiser
//  registers for the global at 0x1008f620, whose members with destructors
//  are the CMoArray<SavedVert589c4> at +0x7bc (0x1008fddc, vtable
//  0x100897a0) and the ObjectBank<Node56ab4> at +0x7d4 (0x1008fdf4,
//  vtables 0x100897d0 then BaseObjectBank's 0x100894c0) -- destroyed in
//  reverse order.  So it is written as that destructor, with the REAL
//  templates (dynarray.h, object_bank.h):
//    * `mov $K,%eax ; add $0x4,%eax` for &m_Bank is what icl emits for a
//      VIRTUAL destructor's member address (ObjectBank's ~ObjectBank is
//      virtual); every non-virtual spelling -- explicit vptr stores, an
//      inline Destruct(), a pointer local, a non-virtual ~T() on an extern
//      object -- folds it to `push $0x1008fdf8` (measured);
//    * the out-of-line `jne` layout of the array teardown, with the
//      `mov $0x10091df0,%ecx ; je ; mov (%ecx),%edx` free, is CMoArray's
//      own Term -> SetSize2 -> _DeleteAndDestroyArray (an inline copy of
//      that chain lays the branches out inline, `je`, measured);
//    * the frame (`push %ebp ; sub $0x8`) and the pushed arguments come
//      with an explicit `g.~T()` call; the same destructor through a
//      frameless caller has a different frame.
//  The element and node types are w27_10056ab0's, so the instantiations
//  (vtables, Term, Allocate ...) are the COMDATs that TU already emits.
// ======================================================================= //
// The saved vertex and the queued-poly node, as w27_10056ab0_osqipni.cpp
// (and w27_10059c80_osoi.cpp) define them, so that CMoArray<SavedVert589c4>
// and ObjectBank<Node56ab4> are the same instantiations as theirs.
struct W7Poly56c60;

struct SavedVert589c4                                   // 0x10 bytes
{
	LTVector	m_Pos;                                  // +0x00
	uint32		m_n0c;                                  // +0x0c
};

// One bucket list head / entry: {prev, next, data}.
struct Link56ab4
{
	Link56ab4*	m_pPrev;                                // +0x00
	Link56ab4*	m_pNext;                                // +0x04
	void*		m_pData;                                // +0x08
};

// ltlink.h's dl_Remove, out of line in the shipped build (0x1000a6e0);
// this spelling is aliased onto it in link/aliases.rsp.
void dl_Remove(Link56ab4* pLink);                       // 0x1000a6e0

// A queued-poly node (0x10050d00's layout): the polys hang off the head
// at +0x00; +0x0c is the bucket key; two links at +0x10 and +0x1c tie the
// node into its bucket and into the pass's node list.
struct Node56ab4
{
	// 0x10050d00, the constructor out of line (thiscall, returns this):
	// ObjectBank<>::Allocate (0x10050d30) calls it behind the placement
	// new's null test.  The two links first and `m_pNext04 = m_pPrev00`
	// last is the shipped store order (+0x04 and +0x00 first, then the
	// links' owners): BYTE-IDENTICAL at /Os /Qip.  The previous spelling
	// (the pair first, `m_pPrev00 = m_pNext04`) stored +0x24, +0x18, +0x00,
	// +0x04 first, 6 bytes off.  w27_10056ab0_osqipni.cpp keeps that older
	// spelling on purpose: there the constructor is expanded inside
	// 0x10056ab4, whose shipped stores come in the other order (1 byte off
	// with it, 13 with this one; measured, no spelling tried gives both).
	Node56ab4()
	{
		m_Link10.m_pPrev = m_Link10.m_pNext = &m_Link10;
		m_Link10.m_pData = this;
		m_Link1c.m_pPrev = m_Link1c.m_pNext = &m_Link1c;
		m_Link1c.m_pData = this;
		m_pNext04 = m_pPrev00 = (W7Poly56c60*)this;
	}

	// The node unlinks itself: ObjectBank<>::Free (0x10050d60) is this
	// destructor, two calls of the out-of-line dl_Remove (0x1000a6e0) on
	// the two links, then sb_Free's push onto the bank's free list (+0x1c)
	// -- BYTE-IDENTICAL.  Without it Free was the bare push (19 bytes) and
	// matched nothing.
	~Node56ab4()
	{
		dl_Remove(&m_Link10);
		dl_Remove(&m_Link1c);
	}

	W7Poly56c60*	m_pPrev00;                          // +0x00
	W7Poly56c60*	m_pNext04;                          // +0x04
	uint32			m_n08;                              // +0x08
	uint32			m_nKey0c;                           // +0x0c
	Link56ab4		m_Link10;                           // +0x10
	Link56ab4		m_Link1c;                           // +0x1c
};

// DrawPolyMgr (0x1008f620) as far as its destructor sees it: the members
// with destructors are the two at its tail.
struct DrawPolyMgr59ef0
{
	uint8						m_pad_000[0x7bc];       // +0x000
	CMoArray<SavedVert589c4>	m_SavedVerts;           // +0x7bc (0x1008fddc)
	uint32						m_nSaved7d0;            // +0x7d0
	ObjectBank<Node56ab4>		m_NodeBank;             // +0x7d4 (0x1008fdf4)
};

extern DrawPolyMgr59ef0 g_1008f620;                     // 0x1008f620

void sub_10059ef0()
{
	g_1008f620.~DrawPolyMgr59ef0();
}


// ======================================================================= //
//  0x10069110 -- 121 bytes.  Cast, hand back what was hit, and measure how
//  far away it was.
//
//      55 8b ec / 83 ec 28 / 56 53
//      8b 45 18 / 8b 4d 08 / c7 00 ff ff ff ff
//      8b 01 / 8b 40 04 / ff d0
//      83 c4 e8 / ...five outgoing dwords... / e8 <0x1006a630>
//      8b 55 0c / 83 c4 18 / 85 c0 / 89 02 / 74 25
//      d9 43 08 / d8 66 08 / d9 43 04 / d8 66 04 / d9 c9 / d8 c8
//      d9 03 / d8 26 / d9 ca / d8 c8 / de c1 / d9 c9 / d8 c8 / de c1
//      8b 45 14 / d9 18
//      5b 5e 8b e5 5d c3
//
//  WHAT PINS IT:
//    * seven stack arguments and a bare `ret`: __cdecl, seven dwords.
//    * `movl $0xffffffff,(%eax)` on the FIFTH argument before anything
//      else runs -- the out-parameter is seeded with -1 up front.
//    * `mov (%ecx),%eax ; mov 0x4(%eax),%eax ; call *%eax` with nothing
//      pushed is a C++ virtual with no arguments on the first parameter.
//    * `fmul %st(0),%st` three times: each difference is SQUARED as a
//      value that is already in a register, which is the squared-temporary
//      shape, not `(a-b)*(a-b)` re-evaluated.
//    * the frame is 0x28 and the only address taken is `lea -0x10(%ebp)`,
//      a SIXTEEN-byte out buffer that the callee fills with `rep movsl`
//      %ecx = 4.  0x28 - 0x10 = 0x18 = three floats at icl's eight-byte
//      float alignment, which is what pays for the three named
//      differences.  (The same 8-byte float spacing shows up at
//      0x100598f0's -0x38/-0x30/-0x28.)
//    * add order inside the sum is the scheduler's; do not
//      chase it.
// ======================================================================= //
struct Plane10069110
{
	uint8 m_pad_00[0x10];
};

struct Caster10069110
{
	virtual void  Slot00();                             // vtbl +0x00
	virtual void* Slot04();                             // vtbl +0x04
};

extern void* __cdecl sub_1006a630(void* pWorld, D3DVec3* pTo, uint32 nFlags,
                                 D3DVec3* pFrom, Plane10069110* pPlane);

void sub_10069110(Caster10069110* pCaster, void** ppHit, D3DVec3* pFrom,
                  float* pfDistSqr, int32* pnIndex, D3DVec3* pTo,
                  uint32 nFlags)
{
	Plane10069110 plane;
	float         fX;
	float         fY;
	float         fZ;

	*pnIndex = -1;

	*ppHit = sub_1006a630(pCaster->Slot04(), pTo, nFlags, pFrom, &plane);

	if (*ppHit)
	{
		fX = pTo->x - pFrom->x;
		fY = pTo->y - pFrom->y;
		fZ = pTo->z - pFrom->z;

		*pfDistSqr = fX * fX + fY * fY + fZ * fZ;
	}
}


// ======================================================================= //
//  0x1004b670, 0x1004faf0 and 0x1006aca0 -- 124 bytes each.  Three
//  console-variable definitions per function.
//
//  Each of the nine registrations is the shape d3dr4b.cpp / d3dr4c.cpp
//  already pinned on 0x1005db90 and 0x10068c50:
//
//      83 c4 f0                 sixteen bytes of outgoing area
//      33 c0                    the shared zero
//      c7 44 24 04 <bits>       the float, as a raw immediate
//      c7 04 24 <name>          the name
//      b9 <object>              `this`
//      89 44 24 08 / 89 44 24 0c    the two trailing zeros
//      e8 <0x10021f40>
//
//  WHAT PINS IT: three consecutive `add $0xfffffff0,%esp` areas with a
//  fresh `xor %eax,%eax` per group, and `ret` with no `ret N`, so these
//  are file-scope dynamic initialisers, not methods.  The float constants
//  read off the immediates: 0x43660000 = 230.0f, 0x3d4ccccd = 0.05f,
//  0x43000000 = 128.0f, 0x3f800000 = 1.0f, 0x40200000 = 2.5f,
//  0x41800000 = 16.0f, and three zeros.  Note the ZERO ones come out as
//  `c7 44 24 04 00000000` and NOT through the shared zero register: that
//  is a float parameter materialising its own immediate, which is exactly
//  what the 1.0f in 0x1005db90 does.
// ======================================================================= //
extern CConVar g_1008eea0;                              // 0x1008eea0
extern CConVar g_1008ee80;                              // 0x1008ee80
extern CConVar g_1008ee60;                              // 0x1008ee60

void sub_1004b670()
{
	g_1008eea0.sub_10021f40("ModelShadowAlpha",  230.0f, 0, 0);
	g_1008ee80.sub_10021f40("ModelShadowOffset", 0.05f,  0, 0);
	g_1008ee60.sub_10021f40("ModelShadowProj",   0.0f,   0, 0);
}


extern CConVar g_1008ef00;                              // 0x1008ef00
extern CConVar g_1008eee0;                              // 0x1008eee0
extern CConVar g_1008eec0;                              // 0x1008eec0

void sub_1004faf0()
{
	g_1008ef00.sub_10021f40("ModelShadowProjRes",  128.0f, 0, 0);
	g_1008eee0.sub_10021f40("ModelShadowProjLOD",  1.0f,   0, 0);
	g_1008eec0.sub_10021f40("ModelShadowProjShow", 0.0f,   0, 0);
}


extern CConVar g_10091200;                              // 0x10091200
extern CConVar g_100911e0;                              // 0x100911e0
extern CConVar g_100911c0;                              // 0x100911c0

void sub_1006aca0()
{
	g_10091200.sub_10021f40("LMDynamic",      1.0f,  0, 0);
	g_100911e0.sub_10021f40("LMDynamicScale", 2.5f,  0, 0);
	g_100911c0.sub_10021f40("LMDynamicSize",  16.0f, 0, 0);
}


// ======================================================================= //
//  0x10059fc0 -- 128 bytes.  Refresh one cached pair, then scale it.
//
//      55 53 / 83 ec 14 / 8b e9         no frame pointer
//      8b 44 24 20 / 8b 40 28 / 8b 40 30 / 85 c0 / 74 61
//      8b 40 28 / 85 c0 / 74 5a
//      8b 5c 24 24 / 89 04 24 / 89 5c 24 04
//      8b c3 / 03 c0 / 03 c0            nIndex * 4
//      8d 94 98 70 07 00 00 / 8d 84 98 74 07 00 00
//      03 d5 / 03 c5                    + this
//      89 54 24 08 / 89 44 24 0c / e8 <0x10039f90>
//      85 c0 / 74 26
//      d9 84 dd 70 07 / d8 0d <0x1008fe3c> / d9 9d 88 07
//      d9 84 dd 74 07 / d8 0d <0x1008fe3c> / d9 9d 8c 07
//      83 c4 14 / 33 c0 / 5b 5d c2 08 00
//
//  WHAT PINS IT:
//    * this is d3dhigh.cpp's 0x1005a050 with a body attached: the same
//      helper (0x10039f90), the same `&m_Pairs[n].u` / `&m_Pairs[n].v`
//      pair of out-parameters, and the same this+0x770 base.  d3dhigh
//      already recorded 0x10039f90 as "fills two out-dwords from
//      pObject->m_p0c; returns 1 on the fast path".
//    * `mov $K` then `add %ebp` for BOTH addresses is the /O2 `&p->m`
//      idiom; `mov 0x28(%eax),%eax` three deep with only the SECOND and
//      THIRD results tested is what the guards below spell.
//    * ONE `xor %eax,%eax` at the very end, reached by three `je`: every
//      path returns zero, so the guards nest and there is a single
//      `return 0` at the bottom.
//    * `flds 0x770(%ebp,%ebx,8)` reads the pair back out of memory after
//      the call wrote it, which is why the multiply is a separate
//      statement and not folded into the helper's result.
// ======================================================================= //
extern uint32 sub_10039f90(void* pObject, uint32 nIndex, uint32* pOutA,
                           uint32* pOutB);              // 0x10039f90

extern float g_1008fe3c;                                // 0x1008fe3c

struct Node10059fc0
{
	uint8        m_pad_00[0x28];                        // +0x00
	Node10059fc0* m_p28;                                // +0x28
	uint8        m_pad_2c[0x04];                        // +0x2c
	Node10059fc0* m_p30;                                // +0x30
};

uint32 CTexStage::sub_10059fc0(void* pThing, uint32 nIndex)
{
	Node10059fc0* pA;
	Node10059fc0* pB;

	pA = ((Node10059fc0*)pThing)->m_p28->m_p30;

	if (pA)
	{
		pB = pA->m_p28;

		if (pB)
		{
			if (sub_10039f90(pB, nIndex,
			                 (uint32*)&m_Pairs[nIndex].u,
			                 (uint32*)&m_Pairs[nIndex].v))
			{
				m_Cur788.u = m_Pairs[nIndex].u * g_1008fe3c;
				m_Cur788.v = m_Pairs[nIndex].v * g_1008fe3c;
			}
		}
	}

	return 0;
}


// ======================================================================= //
//  0x1007dfa4 -- 130 bytes.  Lock the dynamic vertex buffer, once.
//
//      55 8b ec / 83 ec 18 / 56 53 / 8b d9
//      8b 43 34 / 85 c0 / 75 46
//      8b 43 1c / 85 c0 / 74 07 / b8 21 10 00 00 / eb 05 / b8 21 20 00 00
//      8b 53 08 / 8b 4b 18 / 83 c4 f0 / 8b 14 8a
//      c7 44 24 0c 00 / 8d 4b 38 / 89 44 24 04 / 89 14 24 / 89 4c 24 08
//      8b 12 / ff 52 0c
//      85 c0 / 7d 0d
//      33 c0 / 5b 5e 8b e5 5d c3
//      8b 43 38 / eb f5
//      8b 03 / 8b 53 38 / 8b 40 18 / 8b 73 1c / 8b cb / 89 55 fc / ff d0
//      0f af c6 / 8b 55 fc / 03 c2
//      c7 43 34 01 / 89 43 38 / eb cf
//
//  WHAT PINS IT:
//    * 0x1021 and 0x2021 are DDLOCK_WAIT|DDLOCK_WRITEONLY|NOOVERWRITE and
//      ...|DISCARDCONTENTS.  The interface is reached at slot +0x0c with
//      (flags, ppData, NULL), which is IDirect3DVertexBuffer7::Lock, and
//      the object comes out of an array at +0x08 indexed by +0x18.
//    * the sign test is `jge`, so the source tests the HRESULT for
//      failure with `< 0`, not against a named constant.
//    * `mov (%ebx),%eax ; mov 0x18(%eax),%eax ; mov %ebx,%ecx ; call *%eax`
//      -- a C++ virtual on `this`, slot 6, no arguments.  Its result is
//      multiplied by +0x1c and ADDED to the value of +0x38 that was read
//      BEFORE the call and spilled to -0x4(%ebp): the call may write
//      +0x38, so the old value has to survive it.  That spill is the
//      evidence for a named local holding the locked pointer.
//    * `movl $1,0x34(%ebx)` is emitted BEFORE `mov %eax,0x38(%ebx)`, so
//      the flag store is the earlier SOURCE statement and the pointer
//      store cannot be the expression that produced %eax -- hence the
//      local.
//    * MIXED FLAGS, and this is the honest reading: the COM call uses the
//      /O2 outgoing area (`add $0xfffffff0,%esp` + four `mov`) while both
//      extra exits CROSS-JUMP (`eb f5`, `eb cf`), which is the /Os column.
//      `lea 0x38(%ebx),%ecx` is not evidence either way -- 0x1007c890
//      contains both spellings of the same address.
//    * IDENTICAL at /Os once the address arithmetic goes through an inline
//      helper taking (base, count): both are read before the virtual call,
//      the base spilled to -0x4(%ebp) as an inlined parameter.  Written in
//      the body (`m_pLockData + GetVertexSize() * m_nCurVert`, with or
//      without named locals) the loads follow the call or take %esi/%edi.
// ======================================================================= //
struct CVertBufSet
{
	// +0x00 is the compiler's own vptr -- the class is virtual (slot
	// +0x18 is called on `this`), so no explicit m_pVTable here.
	uint8        m_pad_04[0x04];                        // +0x04
	D3DVertBuf** m_pBuffers;                            // +0x08
	uint8        m_pad_0c[0x0c];                        // +0x0c
	uint32       m_nCurBuffer;                          // +0x18
	uint32       m_nCurVert;                            // +0x1c
	uint8        m_pad_20[0x14];                        // +0x20
	uint32       m_bLocked;                             // +0x34
	uint8*       m_pLockData;                           // +0x38

	virtual void   Slot00();                            // vtbl +0x00
	virtual void   Slot04();                            // vtbl +0x04
	virtual void   Slot08();                            // vtbl +0x08
	virtual void   Slot0c();                            // vtbl +0x0c
	virtual void   Slot10();                            // vtbl +0x10
	virtual void   Slot14();                            // vtbl +0x14
	virtual uint32 GetVertexSize();                     // vtbl +0x18

	// An inline helper: its arguments are evaluated BEFORE the virtual
	// call, and the base pointer, as an inlined parameter, is spilled to
	// -0x4(%ebp) across it while the count takes %esi -- the shipped
	// figure.  Written inline in the body, both loads either follow the
	// call (reloaded from the object) or take %esi/%edi with no spill.
	uint8* GetVertPtr(uint8* pBase, uint32 nVert)	{ return pBase + nVert * GetVertexSize(); }

	uint8* sub_1007dfa4();
};

uint8* CVertBufSet::sub_1007dfa4()
{
	uint32 nFlags;
	uint8* pData;

	if (m_bLocked)
		return m_pLockData;

	if (m_nCurVert)
		nFlags = 0x1021;
	else
		nFlags = 0x2021;

	if (m_pBuffers[m_nCurBuffer]->lpVtbl->Lock(m_pBuffers[m_nCurBuffer],
	                                           nFlags,
	                                           (void**)&m_pLockData,
	                                           LTNULL) < 0)
	{
		return LTNULL;
	}

	pData = GetVertPtr(m_pLockData, m_nCurVert);

	m_bLocked   = 1;
	m_pLockData = pData;

	return pData;
}


// ======================================================================= //
//  0x10059590 -- 131 bytes.  Bind a texture to a stage, accounting for it
//  once per frame.
//
//      55 8b ec / 83 ec 14 / 56 53
//      8b 45 08 / 8b 58 44 / 85 db / 74 6d
//      8b 75 0c / 3b 1c b5 <0x1008e684> / 74 54
//      8b 43 20 / 85 c0 / 74 4d
//      0f b7 05 <0x1008bea8> / 8b 53 18 / 3b d0 / 74 15
//        a1 <0x1008cbcc> / 8b 53 14 / 01 50 4c
//        0f b7 05 <0x1008bea8> / 89 43 18
//      83 c4 f4 / a1 <0x1008d988> / 89 74 24 04 / 89 04 24
//      8b 53 1c / 89 54 24 08 / 8b 00 / ff 90 8c 00 00 00
//      89 1c b5 <0x1008e684>
//      33 d2 / 42 / eb 03
//      33 d2 / 42
//      8b c2 / 5b 5e 8b e5 5d c2 0c 00
//      33 d2 / eb f2
//
//  WHAT PINS IT:
//    * `mov %edx,%eax` at the ONE exit with all three answers built in
//      %edx: a result VARIABLE, not three `return` statements (those set
//      %eax directly).  That is the "if/else through a single
//      result variable" lever, and it is why the body below assigns and
//      falls out rather than returning early.
//    * TWO copies of `33 d2 / 42` for the two `1` answers, so icl did not
//      merge them: two separate assignments in the source.
//    * both early tests jump to the SAME `33 d2 / 42`, which is the `||`.
//    * 0x1008bea8 is read as a WORD (`movzwl`) and stored to +0x18 as a
//      dword -- a uint16 frame counter widening on assignment, read twice.
//    * slot +0x8c of the device is SetTexture(stage, texture); the stage
//      index also subscripts the currently-bound table at 0x1008e684.
//    * `ret $0xc` with only two arguments read: the third is dead.
//    * IDENTICAL as a __thiscall member (see below): the frame was the
//      only residual.
// ======================================================================= //
struct Tex10059590
{
	uint8  m_pad_00[0x14];                              // +0x00
	uint32 m_nBytes14;                                  // +0x14
	uint32 m_nLastFrame18;                              // +0x18
	void*  m_pSurface1c;                                // +0x1c
	uint32 m_n20;                                       // +0x20
};

struct Owner10059590
{
	uint8        m_pad_00[0x44];                        // +0x00
	Tex10059590* m_pTex44;                              // +0x44
};

struct Counters10059590
{
	uint8  m_pad_00[0x4c];                              // +0x00
	uint32 m_n4c;                                       // +0x4c
};

extern Counters10059590* g_1008cbcc;                    // 0x1008cbcc
extern uint16            g_1008bea8;                    // 0x1008bea8
extern Tex10059590*      g_1008e684[];                  // 0x1008e684

// A __thiscall MEMBER, not a __stdcall free function: icl gives the
// register parameter `this` a dead four-byte frame slot (as every thiscall
// member here that ignores `this` shows with `push %esi ... add $0x4`), and
// that is the shipped `sub $0x14` against the free function's `sub $0xc`
// (the other four are alignment).  The address sits in the pointer table
// at 0x10089720 beside 0x10059fc0, which takes `this` in %ecx
// (`mov %ecx,%ebp`).  `this` is never read, so the class is a stand-in.
// Measured: __fastcall with two unused register arguments gives
// `sub $0x18`; __stdcall `sub $0xc`; this, the shipped `sub $0x14`.
struct C10059590
{
	uint32 sub_10059590(Owner10059590* pOwner, uint32 nStage, uint32 nUnused);
};

uint32 C10059590::sub_10059590(Owner10059590* pOwner, uint32 nStage, uint32 nUnused)
{
	uint32       nResult;
	Tex10059590* pTex;

	pTex = pOwner->m_pTex44;

	if (!pTex)
	{
		nResult = 0;
	}
	else if (pTex != g_1008e684[nStage] && pTex->m_n20)
	{
		if (pTex->m_nLastFrame18 != g_1008bea8)
		{
			g_1008cbcc->m_n4c += pTex->m_nBytes14;
			pTex->m_nLastFrame18 = g_1008bea8;
		}

		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, nStage,
		                                 pTex->m_pSurface1c);

		g_1008e684[nStage] = pTex;

		nResult = 1;
	}
	else
	{
		nResult = 1;
	}

	return nResult;
}


// ======================================================================= //
//  0x100598f0 -- 133 bytes.  Scale three colour channels into a packed
//  BGRA and stamp the global alpha on top.
//
//      55 8b ec / 83 ec 40 / 57 56 53
//      8b 7d 08 / 8b f1
//      0f b6 47 16 / d9 86 a0 07 / 89 45 e8 / db 45 e8 / de c9
//      d9 5d c8 / d9 45 c8 / db 5d dc
//      8b 5d 0c / 8a 45 dc / 88 43 12
//      ...the same four instructions again for +0x15 -> +0x11 and
//         +0x14 -> +0x10...
//      a0 <0x1008be60> / 88 43 13
//      5b 5e 5f 8b e5 5d c2 08 00
//
//  IDENTICAL (was 212 of 222 bytes differing).  The
//  `fstps tmp ; flds tmp ; fistpl` per channel, with the %ebp frame it
//  costs, is not a cast: it is RoundF (w4_100234d0.cpp), the `fld ; fistp`
//  helper, inlined -- its float parameter is the round trip.  A (uint8)
//  cast truncates through a long sequence at /Os and through `fistpll` at
//  /Os /Qrcd (113 differing there), volatile floats do not help.
//
//  WHAT PINS IT (the earlier reading, kept for the record):
//    * `fstps -0x38(%ebp)` immediately followed by `flds -0x38(%ebp)` is a
//      round trip through a FLOAT lvalue -- the single-precision rounding
//      icl inserts when the product is assigned to a `float` before it is
//      converted.  Without a named float the sequence would be
//      fld/fild/fmulp/fistpl with no store-reload at all.
//    * THREE different slots for that round trip (-0x38, -0x30, -0x28,
//      eight bytes apart, which is icl's float alignment) and three more
//      for the `fistpl` results: three separate float variables, not one
//      reused.
//    * `-0x18(%ebp)` is written and `fildl`-ed three times: that one IS a
//      compiler scratch for the byte-to-float conversion, shared.
//    * `movzbl` on the sources and a byte store on the destinations fixes
//      both sides as uint8; +0x13 comes from a global byte with no
//      arithmetic at all.
// ======================================================================= //
struct Color100598f0
{
	uint8 m_pad_00[0x14];                               // +0x00
	uint8 m_r14;                                        // +0x14
	uint8 m_g15;                                        // +0x15
	uint8 m_b16;                                        // +0x16
};

struct Vertex100598f0
{
	uint8 m_pad_00[0x10];                               // +0x00
	uint8 m_b10;                                        // +0x10
	uint8 m_g11;                                        // +0x11
	uint8 m_r12;                                        // +0x12
	uint8 m_a13;                                        // +0x13
};

extern uint8 g_1008be60;                                // 0x1008be60

// Round to nearest in the current FPU mode: w4_100234d0.cpp's RoundF, the
// __asm `fld ; fistp` helper whose out-of-line copy is 0x10015190.  Inline
// assembly as the measured last resort, on that precedent.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

void CTexStage::sub_100598f0(const Color100598f0* pSrc, Vertex100598f0* pDest)
{
	pDest->m_r12 = (uint8)RoundF(m_f7a0 * pSrc->m_b16);
	pDest->m_g11 = (uint8)RoundF(m_f7a4 * pSrc->m_g15);
	pDest->m_b10 = (uint8)RoundF(m_f7a8 * pSrc->m_r14);

	pDest->m_a13 = g_1008be60;
}


// ======================================================================= //
//  0x10036130 and 0x100361c0 -- 141 bytes each.  The two directions of one
//  BltFast, and they are NOT the same function twice.
//
//      55 8b ec / 83 ec 38 / 57 57 56 53      (%edi pushed TWICE: the
//                                              second is icl's dead slot)
//      8b d9 / 8b 45 0c / 8b 53 0c
//      be <0x100a9178> / 89 55 f4 / b9 04 00 00 00 / 89 45 f8
//      8d 7d e4 / f3 a5                       the sixteen-byte template
//      8b 43 04 / 83 c4 e8 / 89 45 ec         rect.right  = m_04
//      8b 5b 08 / a1 <0x1008bf68> / 89 5d f0  rect.bottom = m_08
//      c7 44 24 14 10 00 00 00                dwTrans = DDBLTFAST_WAIT
//      8d 55 e4 / ...the five other outgoing dwords... / ff 50 1c
//      57 / 8b d8 / 53 / e8 <0x10030f30>
//      33 d2 / 83 c4 08 / 85 db / 0f 94 c2 / 0f be d2
//      a3 <0x1008da30> / 8b c2
//      5b 5e 5f 8b e5 5d c2 08 00
//
//  WHAT PINS THEM:
//    * `rep movsl` %ecx = 4 out of a SIXTEEN-BYTE ALL-ZERO blob, then two
//      stores at +0x08 and +0x0c of the copy.  Sixteen bytes, left/top/
//      right/bottom, and the blob is a per-declaration initialiser
//      template: 0x100a9178, 0x100a9188 and 0x100a9198 are three distinct
//      all-zero copies of it, one per declaration in that block, and
//      nothing else in the image references any of them.  Note that the
//      template writes right/bottom and the next two statements overwrite
//      them -- icl does no dead-store elimination through memory, which is
//      why both survive.
//    * slot +0x1c of IDirectDrawSurface7 is BltFast(dwX, dwY, pSrc,
//      pSrcRect, dwTrans); six outgoing dwords with the interface first is
//      the C COM shape.
//    * `sete` + `movsbl` -- a SIGN-extending byte widen, and there are
//      exactly four of these in the whole 551KB of .text, all four in this
//      block.  It is icl materialising a comparison as a value rather than
//      a branch, and the value is the function's answer.
//    * `push %edi` in front of `push %ebx` is NOT a second argument to
//      0x10030f30: at 0x100364f1 the identical pair is cleaned with
//      `add $0x4,%esp`, and 0x10030f30 itself reads only 0x4(%esp).
//    * THE DIFFERENCE between the two: 0x10036130 makes the module's
//      offscreen surface the DESTINATION (it goes to (%esp) and its vtable
//      is the one indexed) and this->m_0c the source; 0x100361c0 has them
//      the other way round.  Everything else, template address aside, is
//      byte for byte the same.
// ======================================================================= //
struct D3DRect
{
	int32 left;                                         // +0x00
	int32 top;                                          // +0x04
	int32 right;                                        // +0x08
	int32 bottom;                                       // +0x0c
};

struct CBlit10036130
{
	uint8      m_pad_00[0x04];                          // +0x00
	uint32     m_nWidth04;                              // +0x04
	uint32     m_nHeight08;                             // +0x08
	DDSurface* m_pSurface0c;                            // +0x0c

	int32 sub_10036130(uint32 nX, uint32 nY);
	int32 sub_100361c0(uint32 nX, uint32 nY);
};

int32 CBlit10036130::sub_10036130(uint32 nX, uint32 nY)
{
	DDSurface* pSrc = m_pSurface0c;
	D3DRect    rc = { 0, 0, 0, 0 };
	long       hResult;

	rc.right  = m_nWidth04;
	rc.bottom = m_nHeight08;

	hResult = g_pOffscreen->lpVtbl->BltFast(g_pOffscreen, nX, nY,
	                                        pSrc, &rc, 0x10);

	g_1008da30 = sub_10030f30(hResult);

	return (char)(hResult == 0);
}


int32 CBlit10036130::sub_100361c0(uint32 nX, uint32 nY)
{
	DDSurface* pDest = m_pSurface0c;
	D3DRect    rc = { 0, 0, 0, 0 };
	long       hResult;
	DDSurface* pSrc;

	rc.right  = m_nWidth04;
	rc.bottom = m_nHeight08;

	pSrc = g_pOffscreen;
	hResult = pDest->lpVtbl->BltFast(pDest, nX, nY, pSrc, &rc, 0x10);

	g_1008da30 = sub_10030f30(hResult);

	return (char)(hResult == 0);
}


// ======================================================================= //
//  0x10026500 (body at 0x10026504) -- MOVED to d3dqip2.cpp.
//  The shipped function is an external /O2 /Qip pair whose register entry
//  0x10026504 is called by 0x10029890 (0x10029901); icl emits the pair
//  only in a /Qip TU that holds that caller, and d3dqip2.cpp does.  The
//  body there is unchanged and byte-identical (stack shim and register
//  entry both).
// ======================================================================= //



// *********************************************************************** //
//
//  COIN FLIPS -- the spellings the bytes alone do not settle.
//  Each is one build's worth of work and each names the exact alternative.
//
//  1. 0x10036130 / 0x100361c0 -- SETTLED, both identical.  The widen is
//                `return (char)(hResult == 0);` (without the cast icl
//                widens unsigned), and the member surface is read into a
//                local BEFORE the rect is declared: that early load is
//                the spill at -0xc/-0x8 and the extra eight bytes of
//                frame (0x38, not 0x30).  0x100361c0 also takes the
//                offscreen surface into a local after the rect stores,
//                which is what orders its outgoing stores.
//
//  2. 0x10036130 / 0x100361c0 -- the sixteen-byte zero template.
//     Written:   `D3DRect rc = { 0, 0, 0, 0 };`, expecting icl to emit a
//                per-declaration initialiser blob and copy it with
//                `rep movsl` %ecx = 4.
//     Alternative: a file-scope `static const D3DRect g_rcZero = {0,0,0,0};`
//                and `D3DRect rc = g_rcZero;`.  If our build stores four
//                zero immediates instead of `be <addr> / b9 04 / f3 a5`,
//                use the alternative.  (Three distinct all-zero copies of
//                the blob exist -- 0x100a9178, 0x100a9188, 0x100a9198 --
//                one per declaration site, which is what argues for the
//                first form.)
//
//  3. 0x10059300 -- the three member function pointers.
//     Written:   pointers-to-member on a `__single_inheritance` class,
//                called as `(this->*m_pfn7ac)(...)`.
//     Alternative: if icl rejects the keyword or picks a wider
//                representation, declare the slots as plain
//                `void (*)(void*, TexPair*, uint32)` and call through a
//                one-line `__declspec(naked)`-free shim -- or, better,
//                move the class definition ABOVE the typedef so the
//                pointer type is formed against a COMPLETE class, which is
//                the portable way to get the four-byte form.
//
//  4. 0x1007dfa4 -- where the flag store sits.
//     Written:   `pData = m_pLockData + GetVertexSize() * m_nCurVert;
//                m_bLocked = 1; m_pLockData = pData; return pData;`
//                because the shipped code emits `movl $1,0x34` BEFORE
//                `mov %eax,0x38` and both AFTER the call.
//     Alternative: `m_pLockData += GetVertexSize() * m_nCurVert;
//                m_bLocked = 1; return m_pLockData;` -- that puts the
//                0x38 store first and, if it also drops the
//                `mov %edx,-0x4(%ebp)` spill, is wrong for a second
//                reason.
//
//  5. 0x10059ef0 -- SOLVED: it is DrawPolyMgr's virtual-
//                destructor teardown; neither of these spellings (both
//                fold to `push $0x1008fdf8`).  See the function.
//
//  6. 0x1007a680 -- SETTLED (moved to d3dqip4.cpp): neither of these.
//                It is `if (!(A && B && C && (X || Y))) return;` then the
//                call; the out-of-line call is the negated guard.
//
//  7. 0x10059590 -- SOLVED (a __thiscall member; see the function).
//     Was NEAR, one byte.  It is __stdcall (`ret $0xc`), and
//                the `&&` form with the work in the FIRST arm and the
//                second `nResult = 1` in the else puts the shared `33 d2
//                42` at the tail as shipped.  The only residual is the
//                frame: shipped `sub $0x14`, ours `sub $0xc`, eight dead
//                bytes no instruction touches.  Not moved by a device or
//                surface local, a result local, an inline helper, /O1,
//                /G5, /G6, /Ox, /Gs, /Oi- or /Qip.
//
//  8. 0x10069110 -- three named differences versus three squarings.
//     Written:   `float fX, fY, fZ;` then `fX*fX + fY*fY + fZ*fZ`,
//                because 0x28 of frame minus the sixteen-byte out buffer
//                is exactly three floats at icl's eight-byte float
//                alignment.
//     Alternative: no locals at all --
//                `*pfDistSqr = (pTo->x - pFrom->x) * (pTo->x - pFrom->x)
//                            + ...;`  If the frame comes out 0x18 instead
//                of 0x28, the locals are what pays for it and the written
//                form is right; if it stays 0x28 either way the locals
//                are free and the difference is elsewhere.
//
// *********************************************************************** //


// 0x10026500 -- MOVED to d3dqip2.cpp; see there.
