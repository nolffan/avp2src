// *********************************************************************** //
//
//  W31_10066D10_QIPNI (.CPP) -- /O2 /Qip /Qip_no_inlining.
//
//  big66f08.cpp (the two world-model draw callbacks 0x10066f00 and
//  0x10067aa0) and w8_10066d1c.cpp (their helpers 0x10066d10/0x10066d1c
//  and 0x10066e90) in ONE translation unit, as the shipped image has them:
//    * 0x10066e90 is a REGISTER-ONLY entry (the object in %eax, no stack
//      shim) called from 0x10067aa0 -- a /Qip static here;
//    * 0x10066d10 is a /Qip EXTERNAL two-entry pair (12-byte stack shim,
//      register body at 0x10066d1c) whose caller 0x10067aa0 enters at the
//      register body -- so it is named for the stack entry, sub_10066d10;
//    * 0x10066f00 is itself a /Qip two-entry pair (4-byte shim).
//  /Qip_no_inlining because plain /Qip inlines 0x10066e90 away.
//  Measured: 0x10066d10 and 0x10066e90 byte-identical (0x10066e90 was 67
//  bytes differing, the shim did not exist); 0x10066d1c 300 -> 256 bytes
//  differing; 0x10067aa0 3254 -> 3123; 0x10066f00 unchanged at 2547.
//  0x1001e360, which big66f08.cpp also held, moved to w31_1001e360.cpp:
//  under /Qip it became a two-entry pair and lost its match.
//
//  big66f08.cpp's banner follows unchanged.
//
// *********************************************************************** //
// *********************************************************************** //
//
//  MODULE:   BIG66F08 (.CPP)
//
//  PURPOSE:  0x10066f00 .. 0x1006883f -- the TWO WORLD-MODEL DRAW
//            CALLBACKS.  Reconstructed instruction by instruction from
//            the retail d3d.ren under the "structurally right, not
//            byte-exact" bar: same control flow, same callees in the same
//            order, same field offsets, same constants.  Nothing here is
//            guessed; every coin-flip is at the bottom with both spellings.
//
//  ---------------------------------------------------------------------- //
//  THE EXTENT.  The row was dispatched as "0x10066f08, 6,456 bytes, end
//  0x10068840".  The byte count is right and the boundaries are not: that
//  range is TWO functions, and the start is four bytes late.
//
//    * 0x10066f08 is the REGISTER entry of a /Qip two-entry pair.  The
//      stack entry is 0x10066f00:
//
//          10066f00  8b 44 24 04   mov 0x4(%esp),%eax
//          10066f04  8b 54 24 08   mov 0x8(%esp),%edx
//          10066f08  55            push %ebp          <- the stated start
//
//      and d3dr3c.cpp's 0x100688e0 already takes 0x10066f00's address:
//      `g_p1008ef3c->sub_100500a0(pItem, sub_10066f00)`.  So the function
//      is 0x10066f00 and it has TWO parameters, (pXForm, pObject).
//
//    * There are THREE `ret`s in the range: 0x10067a90, 0x10067b12 and
//      0x100687d5.  0x10067a90 is the end of the first function --
//      0x10067a91..0x10067a9f is `90` + two lea-nops, and 0x10067aa0 is a
//      fresh `push %ebp ; mov %esp,%ebp ; sub $0xd0,%esp` prologue.  It is
//      also 16-byte aligned and its address is taken as a constant at
//      0x10068871 (`movl $0x10067aa0,0x4(%esp)` into 0x1007b8e0), which
//      d3dr2a.cpp and d3dr2d.cpp already record.  So the boundary is real.
//
//    * The last `ret` at 0x100687d5 is NOT the end.  0x100687d6..0x10068837
//      is two /O2-sunk cold blocks, both ending `jmp 0x1006863d` back into
//      the body; 0x10068838..0x1006883f is `90` + a lea-nop and 0x10068840
//      is the next function (`push %ebp ; sub $0x8,%esp`, the registrar
//      that installs 0x10067aa0).
//
//    * 0x10067b12's `ret` is the SECOND function's early-out epilogue --
//      the frustum cull rejecting the model -- duplicated by /O2.  One
//      function, two epilogues; it is not a boundary.
//
//  True extents, then:
//      A. 0x10066f00 .. 0x10067a90   2,961 bytes   7 calls (3 direct)
//      B. 0x10067aa0 .. 0x10068837   3,480 bytes  13 calls (7 direct)
//
//  The dispatched 6,456 bytes are 2,953 of A (0x10066f08 on), 3,480 of B,
//  and 23 of padding: 0x10067a91..0x10067a9f and 0x10068838..0x1006883f.
//
//  ---------------------------------------------------------------------- //
//  THE CALL TEST IS WEAK HERE -- 423 and 268 bytes per call, six distinct
//  direct callees over 20 call sites.  What pins the decode is not the call
//  sequence but an INSTRUCTION CENSUS of the call-free x87, and it comes
//  out exact to the instruction in every region.  See "WHAT PINS THE
//  CALL-FREE CODE" below.
//
//  ---------------------------------------------------------------------- //
//  WHICH FLAG:  /O2 for both, on four independent tests --
//    * outgoing arguments are `add $-N,%esp` then `mov`, never `push`
//      (0x10066f8a, 0x10066fad, 0x10067b4d, 0x100687ad, ...);
//    * `&pObj->m_vPos18c` at 0x10067acf is `add $0x18c,%ebx` on the
//      pointer, not `lea 0x18c(%reg),%reg`;
//    * every exit writes its OWN `mov %ebp,%esp ; pop %ebp ; ret` --
//      0x10067b04 and 0x100687c7 are two full duplicated epilogues, no
//      cross-jump;
//    * the two `n == 0` / `n == 1` arms at 0x100687d6 and 0x10068804 are
//      sunk BELOW the epilogue and jump back.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  ltmatrix.h:172 MatMul, written out because BOTH sites in BOTH functions
//  carry no call.  Store order is the header's: column by column,
//  [0][0] [1][0] [2][0] [3][0] [0][1] ...  This is byte-for-byte the helper
//  big19d64.cpp and big17c14.cpp each derived independently; it is reused
//  here unchanged, not re-derived.
// ======================================================================= //
struct LTMat
{
	float  m[4][4];
};

struct LTVec
{
	float  x, y, z;
};

#define M1 pMat1->m
#define M2 pMat2->m

inline void d3d_MatMul(LTMat *pDest, LTMat *pMat1, LTMat *pMat2)
{
	pDest->m[0][0] = M1[0][0]*M2[0][0] + M1[0][1]*M2[1][0] + M1[0][2]*M2[2][0] + M1[0][3]*M2[3][0];
	pDest->m[1][0] = M1[1][0]*M2[0][0] + M1[1][1]*M2[1][0] + M1[1][2]*M2[2][0] + M1[1][3]*M2[3][0];
	pDest->m[2][0] = M1[2][0]*M2[0][0] + M1[2][1]*M2[1][0] + M1[2][2]*M2[2][0] + M1[2][3]*M2[3][0];
	pDest->m[3][0] = M1[3][0]*M2[0][0] + M1[3][1]*M2[1][0] + M1[3][2]*M2[2][0] + M1[3][3]*M2[3][0];

	pDest->m[0][1] = M1[0][0]*M2[0][1] + M1[0][1]*M2[1][1] + M1[0][2]*M2[2][1] + M1[0][3]*M2[3][1];
	pDest->m[1][1] = M1[1][0]*M2[0][1] + M1[1][1]*M2[1][1] + M1[1][2]*M2[2][1] + M1[1][3]*M2[3][1];
	pDest->m[2][1] = M1[2][0]*M2[0][1] + M1[2][1]*M2[1][1] + M1[2][2]*M2[2][1] + M1[2][3]*M2[3][1];
	pDest->m[3][1] = M1[3][0]*M2[0][1] + M1[3][1]*M2[1][1] + M1[3][2]*M2[2][1] + M1[3][3]*M2[3][1];

	pDest->m[0][2] = M1[0][0]*M2[0][2] + M1[0][1]*M2[1][2] + M1[0][2]*M2[2][2] + M1[0][3]*M2[3][2];
	pDest->m[1][2] = M1[1][0]*M2[0][2] + M1[1][1]*M2[1][2] + M1[1][2]*M2[2][2] + M1[1][3]*M2[3][2];
	pDest->m[2][2] = M1[2][0]*M2[0][2] + M1[2][1]*M2[1][2] + M1[2][2]*M2[2][2] + M1[2][3]*M2[3][2];
	pDest->m[3][2] = M1[3][0]*M2[0][2] + M1[3][1]*M2[1][2] + M1[3][2]*M2[2][2] + M1[3][3]*M2[3][2];

	pDest->m[0][3] = M1[0][0]*M2[0][3] + M1[0][1]*M2[1][3] + M1[0][2]*M2[2][3] + M1[0][3]*M2[3][3];
	pDest->m[1][3] = M1[1][0]*M2[0][3] + M1[1][1]*M2[1][3] + M1[1][2]*M2[2][3] + M1[1][3]*M2[3][3];
	pDest->m[2][3] = M1[2][0]*M2[0][3] + M1[2][1]*M2[1][3] + M1[2][2]*M2[2][3] + M1[2][3]*M2[3][3];
	pDest->m[3][3] = M1[3][0]*M2[0][3] + M1[3][1]*M2[1][3] + M1[3][2]*M2[2][3] + M1[3][3]*M2[3][3];
}

#undef M1
#undef M2


// ======================================================================= //
//  ltmatrix.h's MatVMul_H -- transform a point by a 4x4 and divide through
//  by the homogeneous w.  Written out for the TWO sites in 0x10067aa0 that
//  carry no call (0x10068432 and 0x1006853b).
//
//  WHAT PINS IT, exactly:  each site is 15 multiplies, 12 adds, 1 divide
//  and 3 stores, and that is the whole of it -- no other x87 opcode occurs
//  between the two calls that bracket it.  15 = 3 (the w row) + 3x3 (the
//  three output rows) + 3 (the reciprocal applied once per row); 12 = 2+1
//  for w and 2+1 for each row.  ONE `fdivrs 1.0f` for three rows is the
//  reciprocal held in a source variable, not three divides.
// ======================================================================= //
inline void d3d_MatVMulH(LTVec *pDest, LTMat *pMat, const LTVec *pSrc)
{
	float fOneOverW;

	fOneOverW = 1.0f / (pMat->m[3][0]*pSrc->x + pMat->m[3][1]*pSrc->y +
	                    pMat->m[3][2]*pSrc->z + pMat->m[3][3]);

	pDest->x = (pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y +
	            pMat->m[0][2]*pSrc->z + pMat->m[0][3]) * fOneOverW;
	pDest->y = (pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y +
	            pMat->m[1][2]*pSrc->z + pMat->m[1][3]) * fOneOverW;
	pDest->z = (pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y +
	            pMat->m[2][2]*pSrc->z + pMat->m[2][3]) * fOneOverW;
}


// ======================================================================= //
//  THE DIRECT3D 7 DEVICE AT 0x1008d988 -- the slice big4b720.cpp and
//  big16324.cpp model, cut to the two slots these two functions use.
//  Index = offset/4 against IDirect3DDevice7, anchored by the project's
//  +0x50 SetRenderState (20); +0x54 is 21, GetRenderState.  Both are
//  reached as `mov (%eax),%eax ; call *0x50(%eax)` -- ONE indirection with
//  the slot in the call's displacement, the COM shape, so this is NOT
//  spelled `virtual`.
// ======================================================================= //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	void*  m_pad_00[20];                                 // +0x00 .. +0x4c
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 nState,
	                                 uint32 nValue);      // +0x50
	long (__stdcall* GetRenderState)(D3DDevice7*, uint32 nState,
	                                 uint32* pValue);     // +0x54
	void*  m_pad_58[3];                                  // +0x58 .. +0x60
	long (__stdcall* DrawPrimitive)(D3DDevice7*, uint32 nType, uint32 nFVF,
	                                void* pVerts, uint32 nCount,
	                                uint32 nFlags);      // +0x64
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

#ifndef D3DRENDERSTATE_FOGENABLE
#define D3DRENDERSTATE_FOGENABLE   28                    // 0x1c
#endif


// ======================================================================= //
//  THE WORLD-MODEL BODY AT pObject->+0x1a8.
//
//  Every call on it is `mov (%ecx),%eax ; mov 0xNN(%eax),%eax ; call *%eax`
//  -- TWO indirections with the slot loaded into a register, which
//  big4b720.cpp records as icl's C++ virtual __thiscall (and which is why
//  the device above is not spelled that way).  Three slots are reached:
//
//      +0x00   0x1006861b, 0x10068632  int32, no arguments.  Called TWICE
//              in a row off the same object and compared first against 0
//              then against 1 -- two separate source tests, per the
//              "no folding of redundant tests" rule.
//      +0x04   0x100678e7             a pointer, handed straight to
//              0x10064cc0 as its `pNode`, i.e. the BSP root.
//      +0x20   0x10067ac5             a float, and the ONLY use is the
//              sphere radius in the frustum test at 0x10067ae0 (`fld
//              %st(0) ; fchs` right after the call makes -R live beside R).
//
//  and two (pointer, count) pairs sit in its data: +0x54/+0x58 for the
//  `n == 1` arm and +0xa0/+0xa4 for the `n == 0` arm, both handed to
//  0x10066d1c as its %edx and %ecx.  `test %ecx,%ecx ; jbe` in that callee
//  makes the count unsigned.
// ======================================================================= //
class WorldBody1a8
{
public:
	virtual int32   Slot00() = 0;                        // vtbl +0x00
	virtual void*   Slot04() = 0;                        // vtbl +0x04
	virtual void    Slot08() = 0;                        // vtbl +0x08
	virtual void    Slot0c() = 0;                        // vtbl +0x0c
	virtual void    Slot10() = 0;                        // vtbl +0x10
	virtual void    Slot14() = 0;                        // vtbl +0x14
	virtual void    Slot18() = 0;                        // vtbl +0x18
	virtual void    Slot1c() = 0;                        // vtbl +0x1c
	virtual float   Slot20() = 0;                        // vtbl +0x20

	uint8   m_pad_04[0x54 - 0x04];                       // +0x04
	void**  m_pList54;                                   // +0x54
	uint32  m_nCount58;                                  // +0x58
	uint8   m_pad_5c[0xa0 - 0x5c];                       // +0x5c
	void**  m_pLista0;                                   // +0xa0
	uint32  m_nCounta4;                                  // +0xa4
};


// ======================================================================= //
//  THE OBJECT -- the second parameter of both functions.  +0x88, +0x94 ..
//  +0x97, +0x18c and +0x1a8 are field for field big61a30.cpp's Obj61a30
//  and d3dr3d.cpp's sub_1001e210 class, which is why 0x10066e90 can be
//  handed `pObject` unchanged.  The two 4x4s at +0x1f0 and +0x230 are new
//  here; they are fixed by the MatMul at 0x1006715e reading +0x1f0 ..
//  +0x22c as a full sixteen and by the MatVMul_H at 0x10068432 reading
//  +0x230 .. +0x26c as a full sixteen.
// ======================================================================= //
class Object66f08
{
public:
	uint8          m_pad_00[0x88];                       // +0x00
	uint32         m_nFlags88;                           // +0x88
	uint8          m_pad_8c[0x94 - 0x8c];                // +0x8c
	uint8          m_nR94;                               // +0x94
	uint8          m_nG95;                               // +0x95
	uint8          m_nB96;                               // +0x96
	uint8          m_nA97;                               // +0x97
	uint8          m_pad_98[0x18c - 0x98];               // +0x98
	LTVec          m_vPos18c;                            // +0x18c
	uint8          m_pad_198[0x1a8 - 0x198];             // +0x198
	WorldBody1a8*  m_pBody1a8;                           // +0x1a8
	uint8          m_pad_1ac[0x1f0 - 0x1ac];             // +0x1ac
	LTMat          m_Mat1f0;                             // +0x1f0
	LTMat          m_Mat230;                             // +0x230
};


// ======================================================================= //
//  THE TRANSFORM OBJECT -- the first parameter of both functions, and the
//  0x4f8-byte global at 0x1008b880 that big26728.cpp sizes and that
//  d3dr2a.cpp hands to 0x1007b8e0 as the callback context.  Only the three
//  4x4s and the two points are touched here.
//
//  0x1001e360 is XForm66f08::sub_1001e360, defined below, __thiscall,
//  `ret $0xc`: it stores its three arguments to +0x4e8, +0x4e4, +0x4ec and
//  then recomputes +0x4f0 and +0x4f4 from the fog globals.  Both call sites
//  here open TWELVE bytes and copy a three-float local into them field by
//  field, so the argument is one point by value, not three scalars.
// ======================================================================= //
class XForm66f08
{
public:
	void   sub_1001e360(LTVec vPos);                     // 0x1001e360

	uint8  m_pad_000[0x15c];                             // +0x000
	LTMat  m_Mat15c;                                     // +0x15c
	LTMat  m_Mat19c;                                     // +0x19c
	LTMat  m_Mat1dc;                                     // +0x1dc
	uint8  m_pad_21c[0x4b4 - 0x21c];                     // +0x21c
	LTVec  m_v4b4;                                       // +0x4b4
	uint8  m_pad_4c0[0x4e4 - 0x4c0];                     // +0x4c0
	LTVec  m_v4e4;                                       // +0x4e4
	float  m_f4f0;                                       // +0x4f0
	uint32 m_n4f4;                                       // +0x4f4
};


// ======================================================================= //
//  THE SIX FRUSTUM PLANES AT 0x1008bc50 -- d3dr5a_o2.cpp's Plane1008bc50,
//  reused unchanged.  0x10067b29's `add $0x10,%edx` against 0x10067b2c's
//  `cmp $0x6,%esi` is the same six sixteen-byte records.
// ======================================================================= //
struct Plane1008bc50                                     // 0x10 bytes
{
	float  m_fA;                                         // +0x00
	float  m_fB;                                         // +0x04
	float  m_fC;                                         // +0x08
	float  m_fD;                                         // +0x0c
};


// ======================================================================= //
//  GLOBALS AND CALLEES -- every one is an address these two functions touch
// ======================================================================= //

extern D3DDevice7*   g_pD3DDevice;                       // 0x1008d988
extern Plane1008bc50 g_1008bc50[6];                      // 0x1008bc50

// The three floats and the byte 0x10066e90 writes; big2ef34.cpp already
// names 0x1008be60 as the diffuse alpha byte.
extern float         g_f10091100;                        // 0x10091100
extern float         g_f10091104;                        // 0x10091104
extern float         g_f10091108;                        // 0x10091108
extern uint8         g_b1008be60;                        // 0x1008be60

// 0x10066e90 -- 120 bytes, the register-ABI colour expansion d3dr3d.cpp
// already decoded: the object arrives in %eax and it writes exactly the
// four globals above.  A register-ABI call is still a call.
static void sub_10066e90(Object66f08* pObject)
{
	g_f10091100 = pObject->m_nR94 * (1.0f / 255.0f);
	g_f10091104 = pObject->m_nG95 * (1.0f / 255.0f);
	g_f10091108 = pObject->m_nB96 * (1.0f / 255.0f);
	g_b1008be60 = pObject->m_nA97;
}

// ======================================================================= //
//  0x10064cc0 -- the WORLD BSP WALK AND POLY DRAW (was big64cc0.cpp; moved
//  here).  A register-only /Qip static: its one caller,
//  0x10066f08 below, enters it with %eax, %edx, %ecx and there is no stack
//  entry in front of it, so caller and callee share this TU.  The third
//  argument is the six-plane clip mask (0x3f at the one call site), as
//  0x10066d1c takes it.  Its clipper is the 40-byte driver 0x1000aa90 over
//  the six plane bodies, inlined, and the colour bytes go through RoundF.
//  big64cc0.cpp's decode notes follow, unchanged except for the clipper.
//
//  MEASURED: 11,136 -> 8,128 bytes against the shipped 8,240,
//  the prologue, register ABI and BSP walk as shipped.  The frame is
//  0x3bc4 against 0x3bec: the shipped body has 33 inlined RoundF (the
//  three vertex-build colours as well as the clipper's thirty, their
//  float slots -0x3bd0..-0x3bc0 allocated LAST); at /O2 icl inlines the
//  thirty that come in with ClipPolyA and then calls RoundF out of line
//  for the three in the build loop (the caller's inline budget: with the
//  ClipPolyA call removed, all three inline; /Os /Qip /Qip_no_inlining
//  inlines all 33 but costs 0x10066f08 / 0x10067aa0).  Tried without
//  effect: a float temp per colour, `(float)` casts, a second RoundF, a
//  static inline colour helper (not inlined either), __forceinline,
//  the clipper defined after the function.
//  The order icl inlines in: probing with one extra RoundF,
//  a call site placed textually AFTER the ClipPolyA call is inlined and
//  one placed before the vertex build is not -- icl spends this caller's
//  budget on its call sites from the LAST to the first, so ClipPolyA eats
//  it before the build loop's three are reached.  Also without effect:
//  the six ClipEdge helpers as macros, the plane bodies typed (no void**
//  casts), a block of inline __asm in the loop (that one costs the /Qip
//  register entry: a function holding __asm itself gets a stack entry and
//  frame 0x3be0).
//  `pV = &(*ppVerts)[0]` in the six plane bodies
//  compiles to the same bytes here (the frame stays 0x3bc4).
// ======================================================================= //

// ======================================================================= //
//  THE VECTOR AND THE PLANE
// ======================================================================= //
// 0x10048150 -- 34 bytes, __thiscall, `ret $0xc` so the vector goes by
// value.  Body: `n.z*v.z + n.y*v.y + n.x*v.x - d`, the signed distance of a
// point to the plane.  d3dbatchb.cpp: "the same dot product with one extra".
struct SPlane
{
	float sub_10048150(LTVec v) const;                   // 0x10048150

	LTVec m_vNormal;                                     // +0x00
	float m_fDist;                                       // +0x0c
};


// ======================================================================= //
//  THE VERTEX -- 40 bytes, D3DFVF 0x2c4.
//
//  Every offset below is a load or a store in these bytes:
//    +0x00..+0x08  the position (three dword moves at 0x10064ec2..0x10064ecf,
//                  interpolated by all six clip blocks)
//    +0x0c         never written here; sub_1000d940 fills it (`fsts 0xc`)
//    +0x10..+0x13  b, g, r, a -- written at 0x10064f68/0x10064f31/
//                  0x10064f00/0x10064f72, rewritten by the light pass at
//                  0x10065360/0x1006535a/0x1006535d, interpolated at
//                  0x1006569a/0x10065640/0x1006560c/0x100656e1
//    +0x14..+0x17  the specular dword sub_10066cf0 fills; only its top byte
//                  +0x17 is interpolated (0x10065731)
//    +0x18/+0x1c   texture pair one (0x10064f7b/0x10064f7e, and the three
//                  accessors 0x100647c0/d0/e0)
//    +0x20/+0x24   texture pair two -- 0x1001e950 writes it through the two
//                  pointers handed in at 0x10064fa8/0x10064fac, and the
//                  clip blocks interpolate it (0x100655b7/0x100655c5)
// ======================================================================= //
struct RVert
{
	float sub_100647c0() const;                          // 0x100647c0 get U
	float sub_100647d0() const;                          // 0x100647d0 get V
	void  sub_100647e0(float fU, float fV);              // 0x100647e0 set

	LTVec  m_vPos;                                       // +0x00
	float  m_fRHW;                                       // +0x0c
	uint8  m_b;                                          // +0x10
	uint8  m_g;                                          // +0x11
	uint8  m_r;                                          // +0x12
	uint8  m_a;                                          // +0x13
	uint8  m_sb;                                         // +0x14
	uint8  m_sg;                                         // +0x15
	uint8  m_sr;                                         // +0x16
	uint8  m_sa;                                         // +0x17
	float  m_fU;                                         // +0x18
	float  m_fV;                                         // +0x1c
	float  m_fU2;                                        // +0x20
	float  m_fV2;                                        // +0x24
};


// ======================================================================= //
//  THE WORLD DATA
//
//  SPolyVert is 0x18 bytes: the walk in the build loop is `add $0x18,%edi`
//  at 0x10064fcd and the four fields it touches are +0x00 (a pointer,
//  dereferenced for three floats), +0x04 and +0x08 (the texture pair,
//  moved with integer `mov`s because they are float lvalues) and the three
//  colour bytes +0x14/+0x15/+0x16.
// ======================================================================= //
struct SPolyVert
{
	LTVec* m_pPos;                                       // +0x00
	float  m_fU;                                         // +0x04
	float  m_fV;                                         // +0x08
	uint8  m_pad_0c[8];                                  // +0x0c .. +0x13
	uint8  m_nB;                                         // +0x14
	uint8  m_nG;                                         // +0x15
	uint8  m_nR;                                         // +0x16
	uint8  m_pad_17;                                     // +0x17
};

// pPoly->m_p28->m_p30.  +0x28 is a second texture handed to stage 1 at
// 0x100669ac; +0x2c is the env-map marker tested at 0x10064e08 and again
// (with the opposite polarity) at 0x10066986.
struct STexture
{
	uint8  m_pad_00[0x28];                               // +0x00 .. +0x27
	void*  m_p28;                                        // +0x28
	void*  m_p2c;                                        // +0x2c
};

struct SSurface
{
	uint8      m_pad_00[0x30];                           // +0x00 .. +0x2f
	STexture*  m_p30;                                    // +0x30
	SPlane*    m_p34;                                    // +0x34
	uint32     m_n38;                                    // +0x38
	uint8      m_pad_3c[6];                              // +0x3c .. +0x41
	uint16     m_u42;                                    // +0x42
};

struct SLightObj
{
	uint8  m_pad_00[0x94];                               // +0x00 .. +0x93
	uint8  m_nR;                                         // +0x94
	uint8  m_nG;                                         // +0x95
	uint8  m_nB;                                         // +0x96
	uint8  m_pad_97[0x111];                              // +0x97 .. +0x1a7
	float  m_fRadius;                                    // +0x1a8
};

struct SLight
{
	SLight*    m_pNext;                                  // +0x00
	SLightObj* m_pObject;                                // +0x04
	LTVec      m_vPos;                                   // +0x08
};

// 0x1004b630 -- 11 bytes of body then five 0x90 pads, three callers
// (d3dbatchc.cpp).  `mov 0x28(%ecx),%ecx ; mov 0x34(%ecx),%eax`, i.e.
// `return this->m_p28->m_p34` -- the polygon's plane.  The `push %esi` /
// `add $0x4,%esp` around it is the dead 4-byte stack slot.
struct SPoly
{
	SPlane* sub_1004b630() const;                        // 0x1004b630

	uint8      m_pad_00[0x28];                           // +0x00 .. +0x27
	SSurface*  m_p28;                                    // +0x28
	SLight*    m_p2c;                                    // +0x2c
	uint8      m_pad_30[0x1c];                           // +0x30 .. +0x4b
	SPolyVert* m_p4c;                                    // +0x4c
	uint16     m_u50;                                    // +0x50
	uint16     m_u52;                                    // +0x52
	SPolyVert  m_aVerts[1];                              // +0x54
};

struct SNode
{
	SPoly*  m_pPoly;                                     // +0x00
	SNode*  m_pSides[2];                                 // +0x04, +0x08
	uint8   m_pad_0c[0xa];                               // +0x0c .. +0x15
	uint8   m_nFlags;                                    // +0x16
};

// 0x1007b8d0's singleton (d3dr2a.cpp/d3dr5a_o2.cpp: `mov $0x10091a10,%eax
// ; ret`).  Only the registry slice is reached: 32 pairs at +0x40 and the
// count at +0x140, and 0x40 + 32*8 == 0x140 exactly.
struct SLMAnimEntry
{
	SPoly* m_pPoly;                                      // +0x00
	void*  m_pOwner;                                     // +0x04
};

struct SLMAnimMgr
{
	uint8         m_pad_00[0x40];                        // +0x00 .. +0x3f
	SLMAnimEntry  m_aEntries[32];                        // +0x40 .. +0x13f
	uint32        m_nCount;                              // +0x140
};


#define D3DPT_TRIANGLEFAN                     6
// XYZRHW | DIFFUSE | SPECULAR | TEX2 -- 12+4+4+4+8+8 = 40 = 0x28.
#define D3DFVF_WORLDVERTEX                0x2c4


// ======================================================================= //
//  GLOBALS -- every one of them is an address this function touches.
// ======================================================================= //

extern D3DDevice7* g_pD3DDevice;                         // 0x1008d988

// The clipper's state, all four already named by d3dr3a/d3dr3c/big16324.
extern uint32      g_1008b874;                           // clip flags
extern RVert*      g_1008b87c;                           // scratch cursor
extern uint32      g_1008d7a0;                           // UseD3DClip
extern uint32      g_1008be44;                           // clipped-poly count
extern float       g_1008b908;                           // near plane
extern float       g_1008b910;                           // far plane

// 0x1008b880 -- the fixed transform object d3dlow.cpp, d3dr2a.cpp,
// big16324.cpp and big124a4.cpp all see as the second argument of this
// pair of helpers.
extern uint8       g_1008b880[];                         // 0x1008b880

// 0x1008bd34 -- the eye point.  Read as a 12-byte vector at 0x10066c4e
// (three dwords into an outgoing argument block) and as the first argument
// of 0x1001e950 at 0x10064f8c.  d3dr3b.cpp reaches the same address as a
// position; big16324.cpp calls it g_v1008bd34.
extern LTVec       g_v1008bd34;                          // 0x1008bd34

// 0x1008cb84 / 0x1008cb98 -- big21f90.cpp's console variables.
extern int32       g_CV_FixTJunc;                        // 0x1008cb84
extern int32       g_CV_Saturate;                        // 0x1008cb98

// 0x1008ae00 -- big124a4.cpp names it EnvMapWorld.
extern uint32      g_1008ae00;                           // 0x1008ae00

// 0x1008d950 -- d3dr3b.cpp: the ValidateDevice result.
extern uint32      g_1008d950;                           // 0x1008d950

// 0x1008d958 -- the lightmap-animation gate.  Only ever tested.
extern uint32      g_1008d958;                           // 0x1008d958

// 0x1008aea0 and 0x1008e684 -- two more gates on the second texture stage.
// d3dr2d.cpp sees 0x1008e684 as g_a1008e684[nStage]; this function reads
// the dword at index 0 only.
extern uint32      g_1008aea0;                           // 0x1008aea0
extern uint32      g_1008e684;                           // 0x1008e684

// 0x1008d7d0 -- d3dqip2.cpp/d3dqip3.cpp's "current texture stage".
extern uint32      g_1008d7d0;                           // 0x1008d7d0

// 0x1008be48 -- bumped once per dynamic light considered.
extern uint32      g_1008be48;                           // 0x1008be48

// 0x1008be60 -- the alpha byte every built vertex gets.  BSS, so the value
// is not in the file; it is moved as a byte (`mov 0x1008be60,%al`).
extern uint8       g_1008be60;                           // 0x1008be60

// 0x10091100 / 0x10091104 / 0x10091108 -- three floats the source colour
// bytes are scaled by.  BSS: the addresses are read off `fmuls` operands
// at 0x10064ee2, 0x10064f13 and 0x10064f44, the values are not in the file.
extern float       g_f10091100;                          // 0x10091100
extern float       g_f10091104;                          // 0x10091104
extern float       g_f10091108;                          // 0x10091108

// 0x1008e664 / 0x1008e668 -- the texture-coordinate scale pair.  The same
// two big16324.cpp reads as independent scalars.
extern float       g_f1008e664;                          // 0x1008e664
extern float       g_f1008e668;                          // 0x1008e668


// ======================================================================= //
//  THE CALLEES -- 17 distinct, all placed by earlier work.
// ======================================================================= //

// 0x1007b8d0 -- d3dr2a.cpp/d3dr5a_o2.cpp: `mov $0x10091a10,%eax ; ret`.
extern void* sub_1007b8d0();

// 0x1001e950 -- d3dbatchc.cpp: 340 bytes of hand-scheduled x87 computing a
// reflection.  __cdecl, five arguments, read at 0x14/0x18/0x1c/0x20/0x24 of
// its own frame after four pushes: (eye, vertex, plane, &out_u, &out_v).
extern void sub_1001e950(const LTVec* pEye, RVert* pVert, const SPlane* pPlane,
                         float* pfU, float* pfV);

// 0x1000d9b0 -- d3dlow.cpp: transform a vector by the 3x4 at +0x15c of the
// second argument.  __cdecl.
extern void sub_1000d9b0(LTVec* pVec, uint8* pXForm);

// 0x1000d940 -- the screen projection: 1/z through the +0x31c..+0x330
// block of the second argument, the same shape as d3dlow.cpp's 0x10014580
// and with the same two __cdecl arguments.  It is the one that also writes
// +0x0c of the vertex (`fsts 0xc(%eax)` at 0x1000d961).
extern void sub_1000d940(RVert* pVert, uint8* pXForm);

// 0x10066cf0 -- 24 bytes, THIS function is its only caller.  Whole body:
// `mov 0x8(%ebp),%ecx ; mov 0x1008ce8c,%eax ; mov $0x14,%edx ;
// add %ecx,%edx ; call *%eax`, i.e. the register-ABI indirect call
// big17c14.cpp pinned as `void (__fastcall*)(LTVec*, uint32*)`, applied to
// (&pVert->m_vPos, &pVert->m_sb).  __cdecl at this level.
extern void sub_10066cf0(RVert* pVert);

// 0x10015320 -- d3dbatcha.cpp: lookup-or-create over the list off
// argument0+0x0c keyed on the low byte of argument 1; binds a texture to a
// stage and answers non-zero on success.  __cdecl, three arguments.
extern int32 sub_10015320(void* pTex, uint32 nStage, uint32 nFlag);

// 0x10011560 and 0x100115b0 -- d3dbatcha.cpp/d3dbatchb.cpp: the two
// SetTextureStageState pairs, no arguments.
extern void sub_10011560();
extern void sub_100115b0();

// 0x100232b0 -- d3dbatchd.cpp: unbinds a texture stage.  __cdecl.
extern void sub_100232b0(uint32 nStage);

// 0x10025680 -- d3dmid.cpp, 32 bytes.  __cdecl, one argument.
extern void sub_10025680(void* p);

// 0x10021d10 -- the renderer's one-argument warning printer (d3ddraw.cpp,
// big17c14.cpp).  __cdecl.
extern void sub_10021d10(const char* pMsg);


// Round to nearest in the current FPU mode (w1_10001000.cpp's helper;
// 0x10015190 out of line): icl inlines a function holding __asm as
// `fstps f ; flds f ; fistpl n` -- the clip passes' thirty bare `fistpl`.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// ======================================================================= //
//  THE CLIPPER for the 40-byte vertex -- d3dr3b.cpp's driver 0x1000aa90
//  over the six plane bodies w1_10002310.cpp / w44_10002d40.cpp compile
//  byte-identical out of line, all INLINED into the per-triangle
//  path of 0x1005a960 (0x1005b3c0 .. 0x1005ca20).  Named by plane, not by address: in this
//  /Qip TU icl also emits each helper out of line (unreferenced; the link
//  drops them), and an address in the name would make the byte comparison report the
//  copies as the shipped rows.  Planes in order: 0x10002310 near,
//  0x10002670 left, 0x100029e0 top, 0x10002d40 right, 0x100030a0 bottom,
//  0x10003410 far; ClipPolyA is 0x1000aa90.  (Copied from
//  w40_1005a960_qipni.cpp, with this file's global names.)
// ======================================================================= //
#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

struct SClipVertA                                       // 0x28 bytes
{
	LTVector v;                                         // +0x00  x, y, z
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

static inline void LerpClipVertA(SClipVertA* a, SClipVertA* b, SClipVertA* dst, float t)
{
	dst->m_18 = (b->m_18 - a->m_18) * t + a->m_18;
	dst->m_1c = (b->m_1c - a->m_1c) * t + a->m_1c;
	dst->m_20 = (b->m_20 - a->m_20) * t + a->m_20;
	dst->m_24 = (b->m_24 - a->m_24) * t + a->m_24;
	dst->m_12 = (uint8)RoundF((b->m_12 - a->m_12) * t + a->m_12);
	dst->m_11 = (uint8)RoundF((b->m_11 - a->m_11) * t + a->m_11);
	dst->m_10 = (uint8)RoundF((b->m_10 - a->m_10) * t + a->m_10);
	dst->m_13 = (uint8)RoundF((b->m_13 - a->m_13) * t + a->m_13);
	dst->m_17 = (uint8)RoundF((b->m_17 - a->m_17) * t + a->m_17);
}

static inline float ClipEdgeNearA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_1008b908;

	return t;
}

static inline float ClipEdgeLeftA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->x - a->x) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x + a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = -fZ;

	return t;
}

static inline float ClipEdgeTopA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->y - a->y) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = fZ;

	return t;
}

static inline float ClipEdgeRightA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->x - a->x) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x - a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = fZ;

	return t;
}

static inline float ClipEdgeBottomA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->y - a->y) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y + a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = -fZ;

	return t;
}

static inline float ClipEdgeFarA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_1008b910;

	return t;
}

// 0x10002310
static inline int32 ClipPlaneNearA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.z >= g_1008b908);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeNearA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x10002670
static inline int32 ClipPlaneLeftA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->v.z < pV->v.x);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeLeftA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x100029e0
static inline int32 ClipPlaneTopA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.y < pV->v.z);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeTopA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x10002d40
static inline int32 ClipPlaneRightA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.x < pV->v.z);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeRightA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x100030a0
static inline int32 ClipPlaneBottomA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->v.z < pV->v.y);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeBottomA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x10003410
static inline int32 ClipPlaneFarA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.z <= g_1008b910);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeFarA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}

// 0x1000aa90 -- d3dr3b.cpp's driver for this 40-byte vertex.
static inline LTBOOL ClipPolyA(uint32 nFlags, void** ppVerts, uint32* pnVerts)
{
	void*  pScratch;
	void*  pVerts;
	uint32 nVerts;
	uint32 nOut0;
	uint32 nOut1;
	uint32 nOut2;
	uint32 nOut3;
	uint32 nOut4;
	uint32 nOut5;

	if (g_1008d7a0)
	{
		nFlags &= 0x01;
		if (!nFlags)
			return LTTRUE;
	}

	pScratch = g_1008b87c;
	pVerts   = *ppVerts;
	nVerts   = *pnVerts;

	if (nFlags & 0x01)
	{
		if (!ClipPlaneNearA(&nOut0, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x04)
	{
		if (!ClipPlaneLeftA(&nOut1, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x08)
	{
		if (!ClipPlaneTopA(&nOut2, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x10)
	{
		if (!ClipPlaneRightA(&nOut3, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x20)
	{
		if (!ClipPlaneBottomA(&nOut4, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x02)
	{
		if (!ClipPlaneFarA(&nOut5, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;
	return LTTRUE;
}


// ----------------------------------------------------------------------- //
//  0x10064cc0
//
//  Locals are declared in the order the frame lays them out, low address
//  first: aStack1 at -0x3acc, aStack2 at -0x2acc, aVerts at -0x1acc,
//  nVerts at -0x6cc, pVerts at -0x6c8, then the six clip groups from
//  -0x6b8 to -0x191 and the scalars above them.
// ----------------------------------------------------------------------- //
static void sub_10064cc0(uint8* pOwner, SNode* pNode, uint32 nClipMask)
{
	SNode*       aStack1[1024];                          // -0x3acc
	SNode*       aStack2[1024];                          // -0x2acc
	RVert        aVerts[128];                            // -0x1acc
	int32        nVerts;                                 // -0x6cc
	RVert*       pVerts;                                 // -0x6c8

	int32        nSP;                                    // -0xa0
	SPoly*       pPoly;                                  // -0xdc
	SSurface*    pSurf;
	STexture*    pTex;
	SLMAnimMgr*  pMgr;
	uint32       nEntries, iEntry;
	uint8        bEnvMap;                                // -0xe0
	SPolyVert*   pSrc;
	RVert*       pDst;
	int32        nLeft;
	SLight*      pLight;
	SLightObj*   pLightObj;
	SPlane*      pPlane;
	double       fLightX, fLightY, fLightZ;              // -0x100/-0x108/-0x110
	double       fRed, fGreen, fBlue;                    // -0x120/-0x128/-0x130
	double       fRadius, fRadiusSqr, fInvRadius;
	double       fDistSqr, fAtten;
	float        fDist;
	int32        nR, nG, nB;
	int32        nLightVerts;
	RVert*       pWalk;

	// 0x10064cff -- `xor %edx,%edx ; mov %edx,-0xa0(%ebp)`.
	nSP = 0;

	// 0x10064cf8 .. 0x10064d05.  The flag byte is read once and tested
	// TWICE off that one read (again at 0x10064d16), which the
	// "no folding of redundant tests" rule makes two source tests.
	if ((pNode->m_nFlags & 3) != 0)
		return;

	for (;;)
	{
		// =============================================================== //
		//  0x10064d16 / 0x10066c30 -- DESCEND.  Push the node and its far
		//  child, walk to the near one.  The whole block was sunk below the
		//  epilogue by /O2; it is a plain `while` here.
		// =============================================================== //
		while ((pNode->m_nFlags & 3) == 0)
		{
			int32 nSide;

			// 0x10066c40 .. 0x10066c78.  `sub_1004b630` is a __thiscall
			// accessor on the polygon returning its plane; `sub_10048150`
			// is __thiscall on that plane with the eye point by value.
			pPlane = pNode->m_pPoly->sub_1004b630();

			// 0x10066c6f -- `fcomps 0x100aaebc`, and 0x100aaebc is
			// 00000000, i.e. 0.0f.  `jb` takes the zero arm.
			if (pPlane->sub_10048150(g_v1008bd34) < 0.0f)
				nSide = 0;
			else
				nSide = 1;

			// 0x10066c7f .. 0x10066c91.
			aStack1[nSP] = pNode;
			aStack2[nSP] = pNode->m_pSides[nSide];
			nSP++;

			// 0x10066c92 -- `cmp $0x400,%ebx ; jl`.
			if (nSP >= 1024)
				break;

			// 0x10066cb0 -- `xor %edx,%edx ; test %eax,%eax ; sete %dl`,
			// then `mov 0x4(%esi,%edx,4),%esi`: the OTHER side.
			pNode = pNode->m_pSides[(nSide == 0) ? 1 : 0];
		}

		// =============================================================== //
		//  0x10064d1e -- POP.
		// =============================================================== //
		nSP--;
		pNode = aStack1[nSP];

		// 0x10064d32.
		if (pNode->m_pPoly != 0)
		{
			// 0x10064d3c.
			g_1008b874 = 0x3f;

			pPoly = pNode->m_pPoly;
			pSurf = pPoly->m_p28;

			// 0x10064d54 -- `test $0x4,%dl`.
			if ((pSurf->m_n38 & 4) == 0)
			{
				// ======================================================= //
				//  0x10064d5d .. 0x10064ddc -- THE LIGHTMAP-ANIMATION
				//  REGISTRY.  Registering a polygon ALWAYS skips its draw.
				// ======================================================= //
				if (g_1008d958 != 0 && pSurf->m_u42 != 0x7fff)
				{
					pMgr = (SLMAnimMgr*)sub_1007b8d0();
					nEntries = pMgr->m_nCount;

					// 0x10064d95 -- `cmp $0x20,%ebx ; jae`.
					if (nEntries < 32)
					{
						// 0x10064db0 -- the shipped loop is unrolled by
						// two; one source loop.
						for (iEntry = 0; iEntry < nEntries; iEntry++)
						{
							if (pMgr->m_aEntries[iEntry].m_pPoly == pPoly)
								goto next;
						}

						pMgr->m_aEntries[nEntries].m_pPoly = pPoly;
						pMgr->m_aEntries[nEntries].m_pOwner = pOwner + 0x1f0;
						pMgr->m_nCount++;
					}

					goto next;
				}

				// ======================================================= //
				//  0x10064df0 -- THE ENV-MAP GATE.
				// ======================================================= //
				bEnvMap = 0;

				pTex = pPoly->m_p28->m_p30;
				if (pTex != 0)
				{
					if (pTex->m_p2c != 0 && g_1008ae00 != 0)
						bEnvMap = 1;
					else
						bEnvMap = 0;
				}

				// ======================================================= //
				//  0x10064e28 .. 0x10064ff4 -- THE VERTEX BUILD.
				// ======================================================= //
				pDst = aVerts;

				if (g_CV_FixTJunc)
				{
					// 0x10064e44 / 0x10064e48.
					nVerts = pPoly->m_u52;
					pSrc   = pPoly->m_p4c;
				}
				else
				{
					// 0x10064e66 / 0x10064e6a.
					nVerts = pPoly->m_u50;
					pSrc   = pPoly->m_aVerts;
				}

				// 0x10064e79 -- `cmp $0x80,%eax ; jg 0x10066a80`.
				if (nVerts > 128)
				{
					sub_10021d10("Error: vertex buffer overflow");
					goto next;
				}

				nLeft = nVerts;

				while (nLeft--)
				{
					// 0x10064ec0 -- three dword moves through the pointer
					// at +0x00 of the source vertex.
					pDst->m_vPos = *pSrc->m_pPos;

					// 0x10064ed2 .. 0x10064f68.  Each one is
					// `movzbl ; mov ; fildl ; fmuls ; fstps t ; flds t ;
					// fistpl u ; mov u,%al ; mov %al,dst`, icl's
					// `(uint8)floatexpr`.
					pDst->m_r = (uint8)RoundF(pSrc->m_nR * g_f10091100);
					pDst->m_g = (uint8)RoundF(pSrc->m_nG * g_f10091104);
					pDst->m_b = (uint8)RoundF(pSrc->m_nB * g_f10091108);

					// 0x10064f6b / 0x10064f72.
					pDst->m_a = g_1008be60;

					// 0x10064f75 -- integer moves, float lvalues.
					pDst->m_fU = pSrc->m_fU;
					pDst->m_fV = pSrc->m_fV;

					// 0x10064f81 .. 0x10064fb5.  The two out pointers are
					// strength-reduced walkers in the image (-0xe8 and
					// -0xf0, both `add $0x28`); the source takes the
					// addresses of the second texture pair.
					if (bEnvMap)
					{
						sub_1001e950(&g_v1008bd34, pDst, pPoly->m_p28->m_p34,
						             &pDst->m_fU2, &pDst->m_fV2);
					}

					pDst++;
					pSrc++;
				}

				// 0x10064ff5.
				pVerts = aVerts;

				// ======================================================= //
				//  0x10065007 .. 0x10065394 -- THE DYNAMIC-LIGHT PASS.
				// ======================================================= //
				for (pLight = pPoly->m_p2c; pLight != 0;
				     pLight = pLight->m_pNext)
				{
					pLightObj = pLight->m_pObject;

					// 0x1006502a / 0x1006504a.
					g_1008be48++;

					// 0x10065038 .. 0x1006506e -- each component is loaded
					// TWICE (`flds 0x8(%ecx)` twice, and so on): once for
					// the plane distance and once into the 64-bit local.
					fLightX = pLight->m_vPos.x;
					fLightY = pLight->m_vPos.y;
					fLightZ = pLight->m_vPos.z;

					// 0x1006505b .. 0x1006509b.  Emitted last term first,
					// which is the "n-ary float expressions
					// associate last-term-backwards".
					pPlane = pPoly->m_p28->m_p34;
					fDist  = pPlane->m_vNormal.x * pLight->m_vPos.x
					       + pPlane->m_vNormal.y * pLight->m_vPos.y
					       + pPlane->m_vNormal.z * pLight->m_vPos.z
					       - pPlane->m_fDist;

					// 0x100650a6 / 0x100650e0 -- `flds 0x100aaecc` is
					// 0.0f, so this is fabs, with the `fchs` in the
					// fall-through arm.
					if (fDist < 0.0f)
						fDist = -fDist;

					// 0x100650aa .. 0x100650da.  READ EXACTLY AS WRITTEN:
					// `flds 255.0f ; fsub %st(i),%st ; DE E8+i`.  DE E8+i
					// is Intel's FSUBP (objdump prints it `fsubrp`), so
					// st(i) = st(i) - st(0) and the value stored is
					// c - (255.0f - c).  0x100aaec0/c4/c8 are all
					// 437f0000 = 255.0f.  See COIN-FLIP (2).
					fRed   = pLightObj->m_nR - (255.0f - pLightObj->m_nR);
					fGreen = pLightObj->m_nG - (255.0f - pLightObj->m_nG);
					fBlue  = pLightObj->m_nB - (255.0f - pLightObj->m_nB);

					// 0x100650e5 -- `fcomps 0x1a8(%edx) ; jae`.  The
					// member is read a second time at 0x100650ed.
					if (fDist >= pLightObj->m_fRadius)
						continue;

					fRadius = pLightObj->m_fRadius;

					// 0x10065100 .. 0x10065126.  0x100aaed4 is 1.0f.
					fInvRadius = 1.0f / fRadius;
					fRadiusSqr = fRadius * fRadius;

					// 0x10065112 -- the count is +0x50, NOT the +0x52 the
					// build loop may have used.  Read off the bytes.
					nLightVerts = pPoly->m_u50;

					if (nLightVerts == 0)
						continue;

					pWalk = aVerts;

					do
					{
						// 0x10065150 .. 0x10065182, z first.
						fDistSqr =
						    (pWalk->m_vPos.x - fLightX)
						        * (pWalk->m_vPos.x - fLightX)
						  + (pWalk->m_vPos.y - fLightY)
						        * (pWalk->m_vPos.y - fLightY)
						  + (pWalk->m_vPos.z - fLightZ)
						        * (pWalk->m_vPos.z - fLightZ);

						// 0x10065180 -- `fldl radius2 ; fcomp ; jbe`.
						if (fRadiusSqr > fDistSqr)
						{
							// 0x10065191 -- `fsqrt` INLINE, and
							// 0x100aaed8 is 1.0f.
							// (float): the shipped `fsubrs 0x100aaed8` takes the
							// 1.0f as a FLOAT constant, which icl keeps only
							// when the subtraction itself is float.
							fAtten = 1.0f - (float)(sqrt(fDistSqr) * fInvRadius);

							// 0x100651a5 .. 0x100652f8.  The `(int)` casts
							// are icl's inline double->int sequence.
							nR = pWalk->m_r + (int32)(fRed   * fAtten);
							nG = pWalk->m_g + (int32)(fGreen * fAtten);
							nB = pWalk->m_b + (int32)(fBlue  * fAtten);

							// 0x100652f8 / 0x1006530a -- `add X,X`.
							if (g_CV_Saturate)
							{
								nR = nR + nR;
								nG = nG + nG;
								nB = nB + nB;
							}

							// 0x1006531c .. 0x10065360, with the three
							// `< 0` arms sunk to 0x10066c10/0x10066c00/
							// 0x10066bf0.
							if (nR > 255)     nR = 255;
							else if (nR < 0)  nR = 0;

							if (nG > 255)     nG = 255;
							else if (nG < 0)  nG = 0;

							if (nB > 255)     nB = 255;
							else if (nB < 0)  nB = 0;

							pWalk->m_r = (uint8)nR;
							pWalk->m_g = (uint8)nG;
							pWalk->m_b = (uint8)nB;
						}

						pWalk++;
					}
					while (--nLightVerts);
				}

				// ======================================================= //
				//  0x10065395 .. 0x10065406 -- TRANSFORM AND CLIP FLAGS.
				// ======================================================= //
				g_1008b874 = 0x3f;

				pWalk = pVerts;
				nLeft = nVerts;

				while (nLeft--)
				{
					sub_1000d9b0(&pWalk->m_vPos, g_1008b880);
					sub_10066cf0(pWalk);
					pWalk++;
				}

				// =============================================================== //
				//  0x10065407 .. 0x100668bf -- the 40-byte clipper's driver
				//  0x1000aa90 with its six plane bodies, all inlined.
				// =============================================================== //
				if (!ClipPolyA(g_1008b874, (void**)&pVerts, (uint32*)&nVerts))
					goto clipped;

				// =============================================================== //
				//  0x100668c0 / 0x100668d8 -- THE DRAW.
				// =============================================================== //
				// 0x100668d8 .. 0x10066916.  The screen projection, one
				// call per vertex, __cdecl, against the same 0x1008b880.
				pWalk = pVerts;
				nLeft = nVerts;

				while (nLeft--)
				{
					sub_1000d940(pWalk, g_1008b880);
					pWalk++;
				}

				// 0x10066918 .. 0x100669c4 -- THE TEXTURE SETUP.  The
				// chain is re-read from pPoly each time in the image
				// (0x10066918 and again at 0x10066973), so it is spelled
				// out twice here rather than cached.
				// Branch polarity is the image's: 0x10066924 is
				// `test %eax,%eax ; je 0x10066bd0` and 0x1006694c is
				// `test %eax,%eax ; je 0x10066a60`, so in both cases the
				// THEN arm is the one that stays inline.
				pTex = pPoly->m_p28->m_p30;

				if (pTex != 0)
				{
					if (sub_10015320(pTex, g_1008d7d0, 0))
					{
						if (g_1008d950 != 0
						 && g_1008aea0 != 0
						 && g_1008e684 != 0
						 && pPoly->m_p28->m_p30->m_p28 != 0
						 && (pPoly->m_p28->m_p30->m_p2c == 0
						     || g_1008ae00 != 0)
						 && sub_10015320(pPoly->m_p28->m_p30->m_p28, 1, 0))
						{
							// 0x100669b8.
							sub_10011560();
						}
						else
						{
							// 0x100669c0 -- the shared false arm of all
							// six tests above (six `je 0x100669c0`).
							sub_100115b0();
						}
					}
					else
					{
						// 0x10066a60 -- sunk, and it is the one path that
						// skips the draw entirely.
						sub_100115b0();
						sub_10025680(pPoly);
						goto next;
					}
				}
				else
				{
					// 0x10066bd0 -- sunk.
					sub_100232b0(g_1008d7d0);
					sub_100115b0();
				}

				// 0x100669c5 .. 0x10066a1b -- the texture coordinates
				// scaled in place through the three one-line accessors.
				pWalk = pVerts;
				nLeft = nVerts;

				while (nLeft--)
				{
					pWalk->sub_100647e0(pWalk->sub_100647c0() * g_f1008e664,
					                    pWalk->sub_100647d0() * g_f1008e668);
					pWalk++;
				}

				// 0x10066a1d .. 0x10066a58 -- vtable +0x64, six arguments,
				// nothing unwound afterwards.
				g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice,
				                                    D3DPT_TRIANGLEFAN,
				                                    D3DFVF_WORLDVERTEX,
				                                    pVerts, nVerts, 0);

			clipped:
				// 0x10066a59 -- reached both by falling out of
				// DrawPrimitive and by all twelve of the clipper's "the
				// polygon is entirely outside" exits.
				sub_100115b0();
			}
		}

	next:
		// 0x10066a92 -- aStack2[nSP] is loaded BEFORE nSP is tested,
		// because both arms of the test need it.
		pNode = aStack2[nSP];

		// 0x10066a9f / 0x10066ab3.  The `nSP != 0` arm re-enters the
		// descend test at 0x10064d16 with the flag byte already in %al;
		// the `nSP == 0` arm tests it and returns when it is set.
		if (nSP == 0 && (pNode->m_nFlags & 3) != 0)
			break;
	}
}



// 0x10066d1c -- the world-model poly draw, register ABI: the object in
// %eax, the array in %edx, the count in %ecx, then TWO stack arguments at
// the callee's 0x34(%esp) and 0x38(%esp).  Those are the fourth and fifth
// parameters: icl reserves three shadow dwords for the register triple, so
// the caller's 0xc(%esp) and 0x10(%esp) are arguments 4 and 5.  The callee
// stores the fifth straight into 0x1008b874, which big26728.cpp already
// names "clip flags".
extern void  sub_10066d10(Object66f08* pObject, void** pList, uint32 nCount,
                          LTVec* pLightPos, uint32 nClipMask);

// 0x10026480 / 0x10026500 -- d3dqip3.cpp's two-entry mode switches, one
// argument each.  Both are called here through the STACK entry with a
// literal 0.
extern void  sub_10026480(int32 nMode);                  // 0x10026480
extern void  sub_10026500(uint32 nMode);                 // 0x10026500


// ======================================================================= //
//  0x1001e360 (XForm66f08::sub_1001e360, called above through the class)
//  now lives in w31_1001e360.cpp: see that file for why it cannot be in
//  this /Qip TU.
// ======================================================================= //


// ======================================================================= //
//  0x10066f00 .. 0x10067a90   2,961 bytes   DECODED, EVERY INSTRUCTION.
//
//  WHAT IT IS: draw ONE world model's BSP through the world walk, with the
//  view transform temporarily composed with that model's own matrix.
//  d3dr3c.cpp's 0x100688e0 registers it as the per-item callback of
//  0x100500a0, and its two parameters arrive in %eax (the transform object)
//  and %edx (the object) through the four-byte stack shim at 0x10066f00.
//
//  THE SHAPE, region by region:
//
//   0x10066f00..0x10066f20  the /Qip shim and the prologue.  0xd8 of frame.
//
//   0x10066f21..0x10066f89  the colour quad.  Three `movzbl ; mov ; fildl ;
//        fmuls 1/255.0f ; fstps` and one raw byte store -- exactly what
//        d3dr3d.cpp decoded 0x10066e90 to be, written out here rather than
//        called.  It is NOT the same three constants: 0x10066e90 uses
//        0x100aaf48/4c/50 and this copy uses 0x100aaf54/58/5c, six separate
//        1.0f/255.0f in one constant pool, which is what a second textual
//        copy in the same translation unit produces.
//
//   0x10066f8a..0x10066fd1  GetRenderState(FOGENABLE, &nOldFog) and, only
//        when it came back non-zero, SetRenderState(FOGENABLE, ...).  The
//        value is `sete` off `test $0x80,pObject->m_nFlags88`, so it is the
//        INVERSE of that bit.
//
//   0x10066fd2..0x10067158  save.  16 + 16 + 3 floats out of the transform
//        object into locals, then a zero point built with `xor %ecx,%ecx`
//        and copied into a twelve-byte outgoing block.
//
//   0x10067159              pXForm->sub_1001e360(vZero).
//
//   0x1006715e..0x100678e6  the TWO MatMuls.  Call-free, 1,928 bytes.
//
//   0x100678dc..0x100678f9  the BSP root out of vtable slot 1 and the walk.
//
//   0x100678fa..0x10067a5a  restore, and the point restored through
//        0x1001e360 again.
//
//   0x10067a60..0x10067a90  SetRenderState(FOGENABLE, nOldFog) -- and this
//        one is UNCONDITIONAL; there is no second `test` on nOldFog.
//
//  ---------------------------------------------------------------------- //
//  WHAT PINS THE CALL-FREE CODE.  1,928 of the 2,961 bytes -- 65% -- lie
//  between the 0x1001e360 at 0x10067159 and the indirect call at
//  0x100678e7, and the call sequence cannot see any of it.  An instruction census
//  over exactly that range says:
//
//        flds 129    fmuls 127    fmulp 1    faddp 96    fstps 32
//        fxch 16     mov 3        (nothing else at all)
//
//  Two 4x4 matrix multiplies are 2 x 16 outputs x (4 multiplies, 3 adds,
//  1 store) = 128 multiplies, 96 adds, 32 stores.  127 `fmuls` + 1 `fmulp`
//  is 128 multiplies; 129 `flds` + 127 memory operands of the `fmuls` is
//  256 = 128 x 2 operands, every one consumed; 96 `faddp` is 3 per output
//  for 32 outputs; 32 `fstps` is one per output.  There is NO SPARE
//  INSTRUCTION -- not one fadds, fsubs, fdivs, fcom or extra fstp -- so
//  the region cannot hold anything but those two multiplies.
//
//  Which two is fixed by simulating the x87 stack across the whole range
//  and reading off the 32 stored expressions.  Every one is a four-term
//  sum of the right products:
//
//     +0x15c[i][j] = SAVE[i][k] * pObject->m_Mat1f0[k][j]     (16/16)
//     +0x1dc[i][j] = +0x19c[i][k] * +0x15c[k][j]              (16/16)
//
//  where SAVE is the copy taken at 0x10066fd2 -- so the first multiply is
//  in place, aliased, which is why the copy has to exist and why every
//  operand of it comes out of the frame.
//
//  The two save/restore blocks census just as tightly.  0x10066fd2 is 81
//  `mov`s and nothing else: 16+16 for +0x15c, 16+16 for +0x1dc, 3+3 for
//  +0x4e4, 3 zero stores, 3+3 for the outgoing copy, `mov %esp,%eax` and
//  the `this`.  0x100678fa is 72: 16+16, 16+16, 3+3, %esp and `this`.
//  Both are exact.
// ======================================================================= //
void sub_10066f00(XForm66f08* pXForm, Object66f08* pObject)
{
	uint32  nOldFog;                                     // -0xb8
	LTMat   mSave15c;                                    // -0xb4
	LTVec   vSave4e4;                                    // -0x74
	LTVec   vZero;                                       // -0x68
	LTMat   mSave1dc;                                    // -0x44
	void*   pRoot;

	// 0x10066f21 .. 0x10066f89.
	g_f10091100 = pObject->m_nR94 * (1.0f / 255.0f);
	g_f10091104 = pObject->m_nG95 * (1.0f / 255.0f);
	g_f10091108 = pObject->m_nB96 * (1.0f / 255.0f);
	g_b1008be60 = pObject->m_nA97;

	// 0x10066f8a / 0x10066fcf.  `test %eax,%eax ; je` past the setter, so
	// the guard is on the value that came back, not on a second read.
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_FOGENABLE, &nOldFog);

	if (nOldFog)
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		                                     D3DRENDERSTATE_FOGENABLE,
		                                     (pObject->m_nFlags88 & 0x80) == 0);

	// 0x10066fd2 .. 0x10067135.
	mSave15c = pXForm->m_Mat15c;
	mSave1dc = pXForm->m_Mat1dc;
	vSave4e4 = pXForm->m_v4e4;

	// 0x10067136 .. 0x10067159.  One `xor %ecx,%ecx` feeding three stores.
	vZero.x = 0.0f;
	vZero.y = 0.0f;
	vZero.z = 0.0f;
	pXForm->sub_1001e360(vZero);

	// 0x1006715e .. 0x100678e6 -- the census above.
	d3d_MatMul(&pXForm->m_Mat15c, &mSave15c, &pObject->m_Mat1f0);
	d3d_MatMul(&pXForm->m_Mat1dc, &pXForm->m_Mat19c, &pXForm->m_Mat15c);

	// 0x100678dc .. 0x100678f9.  The result of slot 1 goes to %edx and the
	// object to %eax; 0x3f is all six frustum planes still live.
	pRoot = pObject->m_pBody1a8->Slot04();
	sub_10064cc0((uint8*)pObject, (SNode*)pRoot, 0x3f);

	// 0x100678fa .. 0x10067a5b.
	pXForm->m_Mat15c = mSave15c;
	pXForm->m_Mat1dc = mSave1dc;
	pXForm->sub_1001e360(vSave4e4);

	// 0x10067a60 .. 0x10067a90.  Unconditional.
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_FOGENABLE, nOldFog);
}


// ======================================================================= //
//  0x10067aa0 .. 0x10068837   3,480 bytes   DECODED, EVERY INSTRUCTION.
//
//  WHAT IT IS: the same job as 0x10066f00 for the OTHER kind of world
//  model -- the one whose geometry is a flat list rather than a BSP.  It
//  frustum-culls first, keeps the surviving clip mask, and drives
//  0x10066d1c with it and with the light position pulled back into the
//  model's own space.
//
//  It is __cdecl with two stack parameters, not the register pair its
//  neighbour has: 0x10067ab4 reads 0xc(%ebp) and 0x10067b9b reads 0x8(%ebp).
//  d3dr2a.cpp / d3dr2d.cpp already have the registration -- 0x10068890 does
//  `pList->sub_1007b8e0((Ctx1007b8e0*)g_1008b880, sub_10067aa0)` -- so the
//  first parameter is the 0x1008b880 transform object and the second the
//  list element, which is the same class 0x10066f00 takes second.
//
//  THE SHAPE, region by region:
//
//   0x10067aa0..0x10067ad9  prologue, 0xd0 of frame, the radius out of
//        vtable slot 8, `mov $0x3f,%edi` (all six planes live),
//        `add $0x18c,%ebx` (&pObject->m_vPos18c), `mov $0x1008bc50,%edx`
//        and `xor %esi,%esi`.
//
//   0x10067adc..0x10067b30  the SIX-PLANE SPHERE CULL.  `fld %st(0) ; fchs`
//        puts -R beside R before the loop and both stay live across all six
//        iterations.  One iteration is
//            dist = P->A*p.x + P->B*p.y + P->C*p.z - P->D
//            if (dist < -R)  return;            (0x10067afc `jae` continues)
//            if (dist >  R)  mask &= ~(1<<i);   (0x10067b18 `jbe` skips)
//        and the tail is `inc %esi ; add $0x10,%edx ; cmp $0x6,%esi ; jl`.
//        This is d3dr5a_o2.cpp's sub_1007b520 with a clip mask bolted on;
//        that function's decode of the identical plane record and the
//        identical `fcomp %st(1) ; jae` sense is reused here.
//
//   0x10067b31..0x10067b3f  `fstp ; fstp` (R and -R) and the call to
//        0x10066e90 -- the colour quad, out of line this time.
//
//   0x10067b40..0x10067b9a  the fog save/override, identical to its
//        neighbour's.  The `push %edi` at 0x10067b40 is the dead
//        four-byte slot: it tops the 8 already open up to the 12 the COM
//        call needs, and (%esp) is overwritten before the call.
//
//   0x10067b9b..0x10068431  the save and the TWO MatMuls, interleaved by
//        the scheduler.  Call-free, 2,199 bytes.
//
//   0x10068432..0x10068526  the +0x4e4 save and MatVMul_H into it.
//        Call-free.
//
//   0x10068527..0x1006853a  sub_1001e360(vNewPos) then sub_10026480(0).
//
//   0x1006853b..0x10068610  a second MatVMul_H, of +0x4b4 through the same
//        matrix.  Call-free.
//
//   0x10068611..0x1006864b  slot 0 twice, and the two draw arms -- both
//        SUNK below the epilogue at 0x100687d6 and 0x10068804, both
//        `jmp 0x1006863d` back.  Then sub_10026500(0).
//
//   0x1006864c..0x100687d5  restore, restore the point, restore the fog
//        state, epilogue.
//
//  ---------------------------------------------------------------------- //
//  WHAT PINS THE CALL-FREE CODE.  2,658 of the 3,480 bytes -- 76% -- are
//  call-free, in three runs.  All three census exactly.
//
//  (a) 0x10067b9b..0x10068431, the MatMul pair, 2,199 bytes:
//
//        flds 129   fmuls 127   fmulp 1   faddp 96   fstps 31   fxch 19
//        mov 69     (nothing else at all)
//
//      the same 128 multiplies / 96 adds as its neighbour.  31 rather than
//      32 stores because the last one, +0x1e8 at 0x1006843a, is scheduled
//      four bytes past the region into the block below.  The 69 `mov`s are
//      accounted to the instruction: 16 loads + 16 stores for the +0x15c
//      save, 16 + 15 for the +0x1dc save (its [0][0] is register-allocated
//      into %ebx from 0x10067d84 all the way to the store at 0x1006865b),
//      and the six pointer reloads at 0x10067b9b, 0x10067b9e, 0x10067d8d,
//      0x10067d90, 0x1006803a and 0x1006803c.  Simulating the stack gives
//      the same 32 expressions as its neighbour:
//          +0x15c[i][j] = SAVE[i][k] * pObject->m_Mat1f0[k][j]
//          +0x1dc[i][j] = +0x19c[i][k] * +0x15c[k][j]
//
//  (b) 0x10068432..0x10068526, 245 bytes:
//        fmuls 5 + fmul 8 + fmulp 2 = 15 multiplies
//        fadds 4 + faddp 8         = 12 adds
//        fdivrs 1   fstps 4   flds 13   fxch 5   mov 16
//      15 = 3 + 3x3 + 3 and 12 = 3 + 3x3 is MatVMul_H exactly, with the
//      single `fdivrs 1.0f` (0x100aaf70) reused by all three rows.  The
//      fourth `fstps` is the +0x1e8 that belongs to (a); the sixteen
//      `mov`s are 2 + 2 for the +0x4e4 copy, 1 + 1 for its z, three
//      pointer shuffles, 3 + 3 for the outgoing copy and the `this`.
//
//  (c) 0x1006853b..0x10068610, 214 bytes:
//        fmuls 12 + fmul 2 + fmulp 1 = 15 multiplies
//        fadds 4 + faddp 8           = 12 adds
//        fdivrs 1   fstps 3   flds 12   mov 2
//      the identical census, on +0x4b4 instead of +0x4e4.
//
//  (d) the cull loop 0x10067ada..0x10067b30 is 3 multiplies, 2 adds, one
//      `fsubs`, one `fcom`, one `fcomp` and three `fstp`s -- one plane, one
//      dot product, two comparisons, and the three pops of the early exit.
//
//  (e) the restore 0x1006864c..0x1006879f is 72 `mov`s and nothing else:
//      16 + 16 for +0x15c, 15 + 16 for +0x1dc, the pointer reload at
//      0x1006864c, 3 + 3 for the outgoing copy, `mov %esp,%eax` and the
//      `this` at 0x1006879d.
// ======================================================================= //
void sub_10067aa0(XForm66f08* pXForm, Object66f08* pObject)
{
	uint32           nOldFog;                            // -0xa8
	LTMat            mSave15c;                           // -0xa4
	LTVec            vSave4e4;                           // -0x64
	LTVec            vNewPos;                            // -0x58
	LTVec            vLightPos;                          // -0x4c
	LTMat            mSave1dc;                           // -0x40
	LTPlane*         pPlane;
	LTVector*        pPos;
	float            fRadius, fDist;
	uint32           nClipMask;
	int32            i;

	// 0x10067ab4 .. 0x10067b30.  The distance is the SDK's
	// LTPlane::DistTo (ltplane.h) on a walked LTPlane* and a pointer to the
	// position, as d3dr5a_o2.cpp's identical 0x1007b520 has it.  Written out
	// as a dot product icl hoisted the three position loads into x87
	// registers; through DistTo they are re-read off %ebx every iteration as
	// shipped, which frees the register that held pXForm: the clip mask goes
	// to %edi, pXForm is reloaded from 0x8(%ebp), the object sits at -0x4
	// and the frame is the shipped 0xd0.  attempted -> NEAR (first 259 bytes
	// identical, instruction match 0.42 -> 0.81).
	fRadius   = pObject->m_pBody1a8->Slot20();
	nClipMask = 0x3f;
	pPlane    = (LTPlane*)g_1008bc50;
	pPos      = (LTVector*)&pObject->m_vPos18c;

	for (i = 0; i < 6; i++)
	{
		fDist = pPlane->DistTo(*pPos);

		if (fDist < -fRadius)
			return;

		if (fDist > fRadius)
			nClipMask &= ~(1 << i);

		pPlane++;
	}

	// 0x10067b3b.
	sub_10066e90(pObject);

	// 0x10067b41 .. 0x10067b9a -- 0x10066f00's block, verbatim.
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_FOGENABLE, &nOldFog);

	if (nOldFog)
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		                                     D3DRENDERSTATE_FOGENABLE,
		                                     (pObject->m_nFlags88 & 0x80) == 0);

	// 0x10067b9b .. 0x10068431.
	mSave15c = pXForm->m_Mat15c;
	mSave1dc = pXForm->m_Mat1dc;

	d3d_MatMul(&pXForm->m_Mat15c, &mSave15c, &pObject->m_Mat1f0);
	d3d_MatMul(&pXForm->m_Mat1dc, &pXForm->m_Mat19c, &pXForm->m_Mat15c);

	// 0x10068432 .. 0x10068536.
	vSave4e4 = pXForm->m_v4e4;
	d3d_MatVMulH(&vNewPos, &pObject->m_Mat230, &pXForm->m_v4e4);
	pXForm->sub_1001e360(vNewPos);
	sub_10026480(0);

	// 0x1006853b .. 0x10068610.
	d3d_MatVMulH(&vLightPos, &pObject->m_Mat230, &pXForm->m_v4b4);

	// 0x10068611 .. 0x1006863c.  Slot 0 is called TWICE off the same
	// object -- two calls, not one result tested twice -- and both arms are
	// /O2-sunk below the epilogue.
	if (pObject->m_pBody1a8->Slot00() == 0)
	{
		// 0x10068804.
		sub_10066d10(pObject, pObject->m_pBody1a8->m_pLista0,
		             pObject->m_pBody1a8->m_nCounta4, &vLightPos, nClipMask);
	}
	else if (pObject->m_pBody1a8->Slot00() == 1)
	{
		// 0x100687d6.
		sub_10066d10(pObject, pObject->m_pBody1a8->m_pList54,
		             pObject->m_pBody1a8->m_nCount58, &vLightPos, nClipMask);
	}

	// 0x1006863d.
	sub_10026500(0);

	// 0x1006864c .. 0x100687d5.
	pXForm->m_Mat15c = mSave15c;
	pXForm->m_Mat1dc = mSave1dc;
	pXForm->sub_1001e360(vSave4e4);

	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_FOGENABLE, nOldFog);
}


// *********************************************************************** //
//
//  UNDECODED REGIONS: NONE.  Every instruction of 0x10066f00..0x10067a90
//  and 0x10067aa0..0x10068837 is placed.  The only bytes in the dispatched
//  range not written above are the twenty-three of inter-function padding
//  at 0x10067a91..0x10067a9f and 0x10068838..0x1006883f.
//
//  EXPECTED CALL SEQUENCE:
//
//      0x10066f00 .. 0x10067a91    7 shipped calls
//          indirect  GetRenderState        0x10066f98
//          indirect  SetRenderState        0x10066fcf
//          0x1001e360                      0x10067159
//          indirect  Slot04                0x100678e7
//          0x10064cc0                      0x100678f5
//          0x1001e360                      0x10067a5b
//          indirect  SetRenderState        0x10067a7f
//
//      0x10067aa0 .. 0x10068838   13 shipped calls
//          indirect  Slot20                0x10067ac5
//          0x10066e90                      0x10067b3b
//          indirect  GetRenderState        0x10067b5e
//          indirect  SetRenderState        0x10067b98
//          0x1001e360                      0x10068527
//          0x10026480                      0x10068536
//          indirect  Slot00                0x1006861b
//          indirect  Slot00                0x10068632
//          0x10026500                      0x10068647
//          0x1001e360                      0x100687a0
//          indirect  SetRenderState        0x100687c4
//          0x10066d1c                      0x100687f7   (the n==1 arm)
//          0x10066d1c                      0x1006882b   (the n==0 arm)
//
//  The two 0x10066d1c rows are the /O2-sunk arms: the shipped sequence
//  puts them LAST because the blocks live below the epilogue, while this
//  source issues them where the `if` is.  That is a known pattern,
//  exactly -- equal counts on both sides with the odd rows all from block
//  placement -- so expect two shipped-only and two ours-only rows there
//  and treat them as agreement.
//
//  CALL-FREE COVERAGE:
//      0x10066f00 -- 1,928 of 2,961 bytes (65%) are call-free, in one run.
//      0x10067aa0 -- 2,658 of 3,480 bytes (76%) are call-free, in three.
//      Together 4,586 of 6,441, 71% of the code the call test cannot see.
//  What stands in for the call test there is the instruction census in the
//  two banners above: 128 multiplies / 96 adds / 32 stores per MatMul pair
//  with no spare x87 opcode of any kind, and 15 multiplies / 12 adds /
//  1 divide / 3 stores per MatVMul_H, twice.  Every one of the 128 products
//  in each MatMul pair was read off an x87 stack simulation of the region
//  and matched to its (row, column) -- 32 of 32 output expressions correct
//  in both functions.
//
// ----------------------------------------------------------------------- //
//  COIN-FLIPS -- both spellings given, so one build settles each.
//
//  (0) SETTLED: THE FIRST MatMul READS THE SAVED COPY.  Both functions
//      multiply `m_Mat15c = m_Mat15c * m_Mat1f0` with the left operand read
//      out of mSave15c's frame slots (`fmuls -0xa4(%ebp)` ...), not out of
//      the object -- the save exists before the product and the product is
//      computed from it, which is also what makes dest == src safe.  Written
//      `d3d_MatMul(&pXForm->m_Mat15c, &mSave15c, &pObject->m_Mat1f0)`:
//      measured 227 -> 296 of 803 instructions in 0x10067aa0 and 328 -> 411
//      of 634 in 0x10066f00.  Both remain far from identical; the rest of
//      the residue is register choice (shipped keeps the clip mask in %edi
//      and reloads pXForm from 0x8(%ebp) each time, we pin pXForm in %edi
//      and spill the mask), an 8-byte larger frame, and the cull loop's
//      position loads, which icl hoists into x87 registers here and reads
//      through %ebx each iteration in the shipped code (a pointer local and
//      an inline distance helper were measured and change nothing).
//
//  (1) THE MATRIX SAVE AND RESTORE.  Written above as four struct
//      assignments:
//
//          mSave15c = pXForm->m_Mat15c;   ...   pXForm->m_Mat15c = mSave15c;
//
//      The shipped code copies field by field with integer `mov`s, which
//      the "struct assignment is always rep movsl" rule argues
//      against.  The reason to write it this way anyway is the frame:
//      mSave15c is a CONTIGUOUS ASCENDING block (+0x15c -> -0xb4, +0x198 ->
//      -0x78 in 0x10066f00) because the inlined MatMul takes its address,
//      while mSave1dc's slots are handed out DESCENDING (+0x1dc -> -0x8,
//      +0x218 -> -0x44) and in 0x10067aa0 its [0][0] is not given a slot at
//      all -- it lives in %ebx.  That is scalar replacement of an aggregate
//      whose address is never taken, and scalar replacement is exactly what
//      turns one `rep movsl` into sixteen `mov`s.  If icl will not do it,
//      the other spelling is sixteen element assignments each way:
//
//          for (i = 0; i < 4; i++) for (j = 0; j < 4; j++)
//              mSave15c.m[i][j] = pXForm->m_Mat15c.m[i][j];
//
//      or the same fully written out.  Nothing else in either body changes.
//
//  (2) THE COLOUR QUAD IN 0x10066f00.  Written out as three lines, because
//      the copy uses its OWN three 1.0f/255.0f constants (0x100aaf54/58/5c)
//      distinct from 0x10066e90's (0x100aaf48/4c/50) -- six copies of one
//      value in one constant pool, which is a second textual occurrence in
//      the same translation unit.  The alternative is that this really is
//      `sub_10066e90(pObject);` inlined at /O2 while the call at 0x10067b3b
//      survived on size grounds; that spelling costs one ours-only call row
//      at 0x10066f21, so the call sequence prefers the form written.
//
//  (3) THE ZERO POINT.  0x10067139/0x1006713e/0x10067141 store one zeroed
//      register to -0x60, -0x64 and -0x68 in that order -- z, then y, then
//      x.  Written x, y, z above.  If the order is source order and not
//      the scheduler's, the other spelling is
//
//          vZero.z = 0.0f;  vZero.y = 0.0f;  vZero.x = 0.0f;
//
//  (4) THE RECIPROCAL IN d3d_MatVMulH.  Written as a named local, because
//      ONE `fdivrs 1.0f` serves all three rows and the quotient stays in an
//      x87 slot across them.  If icl folds `x / w` three times to the same
//      thing, the other spelling drops fOneOverW and divides each row by
//      the w expression.
//
//  (5) THE THIRD ROW OF d3d_MatVMulH.  Rows 0 and 1 are built ((t3+t2)+t1)
//      at both sites, but row 2 comes out (t3 + (t1+t2)) at 0x10068511 and
//      ((t2+t1) + t3) at 0x1006860e.  Two different trees from one piece of
//      source text, so icl is reassociating and the source cannot be read
//      off the order; all three rows are written the same.  If icl does not
//      reassociate, row 2 at the first site wants
//
//          pDest->z = (pMat->m[2][2]*pSrc->z + pMat->m[2][0]*pSrc->x +
//                      pMat->m[2][1]*pSrc->y + pMat->m[2][3]) * fOneOverW;
//
//      and the two sites would then need two spellings, i.e. the helper
//      would have to be written out twice.
//
//  (6) THE SOURCE OF THE FIRST MatVMul_H.  Written as `&pXForm->m_v4e4`,
//      the member, with vSave4e4 a separate copy.  The bytes cannot tell
//      the two apart -- 0x1006847a and 0x10068432 read +0x4ec out of the
//      object BEFORE -0x5c is written at 0x100684bb, so icl propagated
//      across the copy -- and the alternative is
//
//          vSave4e4 = pXForm->m_v4e4;
//          d3d_MatVMulH(&vNewPos, &pObject->m_Mat230, &vSave4e4);
//
//  (7) THE THIRD ARGUMENT OF 0x10064cc0.  big64cc0.cpp records its %ecx as
//      reloaded and never read and offers a two-parameter signature as its
//      own coin-flip (1).  From this side the argument is real: 0x100678f0
//      loads it with 0x3f immediately before the call, and 0x10066d1c --
//      the sister draw for the other model kind -- takes the COMPUTED
//      six-plane mask in the same position and stores it into 0x1008b874,
//      which big26728.cpp already names "clip flags".  So 0x10064cc0 does
//      have a third parameter and it is the clip mask.  If big64cc0.cpp's
//      `void* pUnused` spelling is kept for link compatibility, the call
//      site here is `sub_10064cc0(pObject, pRoot, (void*)0x3f)`.
//
//  (8) THE CULL LOOP'S ADDRESSING.  Written as a walked `pPlane++` with a
//      separate `i` for the shift, because 0x10067b29 is `add $0x10,%edx`.
//      d3dr5a_o2.cpp's sibling 0x1007b520 -- same six planes, same dot
//      product, same `jae` sense, but no index to keep -- gets
//      base+displacement instead, which is what makes the walked form here
//      look like source and not strength reduction.  The alternative is to
//      index throughout: `g_1008bc50[i].m_fA * ...` with no pPlane.
//
//  (9) THE FOG OVERRIDE VALUE.  `test $0x80,%edx ; sete %cl` is written
//      `(pObject->m_nFlags88 & 0x80) == 0`.  The other spelling is
//      `!(pObject->m_nFlags88 & 0x80)`.
//
// ----------------------------------------------------------------------- //
//  BOTH FUNCTIONS WANT /O2, so this file does not need splitting.
// *********************************************************************** //

struct Plane10066d1c
{
	LTVec	m_Normal;                                   // +0x00
	float	m_Dist;                                     // +0x0c
};

// A polygon's surface: +0x30 its texture (NULL = untextured), +0x34 its
// plane, +0x38 the flag word (4 = invisible, 0x80 = two-pass, 0x8000),
// +0x42 the lightmap-animation index (0x7fff = none).
struct Surf10066d1c
{
	uint8			m_pad_00[0x30];
	void*			m_pTexture30;                       // +0x30
	Plane10066d1c*	m_pPlane34;                         // +0x34
	uint32			m_nFlags38;                         // +0x38
	uint8			m_pad_3c[0x42 - 0x3c];
	uint16			m_wLMAnim42;                        // +0x42
};

struct Poly10066d1c
{
	uint8			m_pad_00[0x28];
	Surf10066d1c*	m_pSurf28;                          // +0x28
	uint8			m_pad_2c[0x42 - 0x2c];
	uint16			m_wFrameCode42;                     // +0x42
};

// The 0x1007b8d0 record: a 32-entry (poly, transform) table at +0x40 and
// its count at +0x140.
struct LMEntry10066d1c
{
	Poly10066d1c*	m_pPoly;                            // +0x00
	float*			m_pMat;                             // +0x04
};

struct LMTable10066d1c
{
	uint8				m_pad_00[0x40];
	LMEntry10066d1c		m_Entries[0x20];                // +0x40
	uint32				m_nEntries;                     // +0x140
};

extern void* sub_1007b8d0();                            // 0x1007b8d0

extern uint32			g_1008b874;                     // clip flags
extern uint16			g_1008bea8;                     // 0x1008bea8 frame code
extern int32			g_nWorldPoliesProcessed;        // 0x1008be58
extern uint32			g_1008d958;                     // 0x1008d958

// The four draw entry points (big24090.cpp: 0x1002571f..0x10025748 install
// one of three implementations into all four).
extern void (*g_1008ce94)(Poly10066d1c* pPoly);        // 0x1008ce94  textured
extern void (*g_1008ce98)(Poly10066d1c* pPoly);        // 0x1008ce98  untextured
extern void (*g_1008ce9c)(Poly10066d1c* pPoly);        // 0x1008ce9c  flag 0x80
extern void (*g_1008cea0)(Poly10066d1c* pPoly);        // 0x1008cea0  flag 0x8000


void sub_10066d10(Object66f08* pObject, void** pList, uint32 nCount,
                  LTVec* pPos, uint32 nClipFlags)
{
	uint32			n;
	Poly10066d1c*	pPoly;
	Surf10066d1c*	pSurf;
	Plane10066d1c*	pPlane;
	LMTable10066d1c* pTable;
	float*			pMat;
	uint32			nEntries;
	uint32			i;

	g_1008b874 = nClipFlags;

	for (n = 0; n < nCount; n++)
	{
		pPoly = (Poly10066d1c*)pList[n];

		if (pPoly->m_wFrameCode42 == g_1008bea8)
			continue;

		pSurf = pPoly->m_pSurf28;
		pPoly->m_wFrameCode42 = g_1008bea8;
		pPlane = pSurf->m_pPlane34;

		// The facing test guards the rest of the body as a block:
		// written `if (dot <= 0.0001f) continue;` icl falls through
		// into the loop step (`ja` to the body); as an enclosing `if (dot >
		// 0.0001f) { ... }` the body falls through (`jbe`) and the step goes
		// out of line to the end, as shipped -- 0x10066d1c identical.
		if (pPos->x * pPlane->m_Normal.x + pPos->y * pPlane->m_Normal.y +
		    pPos->z * pPlane->m_Normal.z - pPlane->m_Dist > 0.0001f)
		{

		if (pSurf->m_nFlags38 & 4)
			continue;

		if (g_1008d958 && pSurf->m_wLMAnim42 != 0x7fff)
		{
			pTable = (LMTable10066d1c*)sub_1007b8d0();
			pMat = (float*)&pObject->m_Mat1f0;

			nEntries = pTable->m_nEntries;
			if (nEntries >= 0x20)
				continue;

			for (i = 0; i < nEntries; i++)
			{
				if (pTable->m_Entries[i].m_pPoly == pPoly)
					break;
			}

			if (i < nEntries)
				continue;

			pTable->m_Entries[nEntries].m_pPoly = pPoly;
			pTable->m_Entries[nEntries].m_pMat = pMat;
			pTable->m_nEntries++;
			continue;
		}

		g_nWorldPoliesProcessed++;

		pSurf = pPoly->m_pSurf28;
		if (pSurf->m_nFlags38 & 4)
			continue;

		if (!pSurf->m_pTexture30)
			g_1008ce98(pPoly);
		else if (pSurf->m_nFlags38 & 0x80)
			g_1008ce9c(pPoly);
		else if (pSurf->m_nFlags38 & 0x8000)
			g_1008cea0(pPoly);
		else
			g_1008ce94(pPoly);
		}
	}
}




// ======================================================================= //
//  0x10068910 -- MOVED HERE from w8_10068910.cpp, whose
//  banner said it is near by construction: its call to 0x10066f00 enters
//  the /Qip register entry 0x10066f08 (pair in %eax/%edx), so it shares
//  this TU.  Measured in this file: 0x10068910 byte-identical (was 92
//  bytes off), the 0x10066f00 stack shim byte-identical (new), 0x10066f08
//  2547 -> 1177 bytes differing; 0x10066d10 and 0x10066e90 still
//  identical.  Body unchanged from w8_10068910.cpp.
// ======================================================================= //
// Save a render state and set a new value; put the old one back on exit.
struct RenderStateSave10068910
{
	uint32	m_nState;                                   // +0x00
	uint32	m_nOld;                                     // +0x04

	RenderStateSave10068910(uint32 nState, uint32 nValue)
	{
		m_nState = nState;
		g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, m_nState, &m_nOld);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, m_nState, nValue);
	}

	~RenderStateSave10068910()
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, m_nState, m_nOld);
	}
};


// ======================================================================= //
//  0x10068910  320 bytes
// ======================================================================= //
void sub_10068910(XForm66f08* pXForm, Object66f08* pObject)
{
	RenderStateSave10068910 srcBlend(0x13, 2);          // D3DRENDERSTATE_SRCBLEND
	RenderStateSave10068910 destBlend(0x14, 2);         // D3DRENDERSTATE_DESTBLEND
	RenderStateSave10068910 fogColor(0x22, 0);          // D3DRENDERSTATE_FOGCOLOR

	sub_10066f00(pXForm, pObject);
}


// *********************************************************************** //
//
//  0x10064cc0 COIN-FLIPS (from big64cc0.cpp) -- both spellings given, so
//  one build settles each.  (3) and (4) concern the clip blocks, now the
//  inlined ClipPolyA above.
//
//  (1) THE THIRD ARGUMENT.  0x10064cd0 reloads %ecx out of the spill slot
//      at 0x3bf4(%esp) and nothing reads it before 0x10064d85 overwrites
//      it.  Written above as a declared-and-unused third parameter:
//
//          void sub_10064cc0(uint8* pOwner, SNode* pNode, void* pUnused)
//
//      If icl drops the reload for an unread parameter and the shipped
//      prologue therefore cannot be reproduced, the other reading is that
//      there is no third parameter at all and the `push %ecx` is the
//      caller's, in which case the signature is
//
//          void sub_10064cc0(uint8* pOwner, SNode* pNode)
//
//      Nothing else in the body changes either way; this is a prologue-only
//      difference and it does not touch the call sequence.
//
//  (2) THE LIGHT COLOUR EXPRESSION -- the one place where what the bytes
//      say is genuinely surprising, so it is spelled out.  At 0x100650aa,
//      0x100650b4 and 0x100650c6 the image does, three times:
//
//          d9 05 <255.0f>      flds   255.0f
//          d8 e3 / d8 e4 / d8 e1   fsub   %st(i),%st      st0 = 255.0f - c
//          de eb / de ec / de e9   DE E8+i                st(i) = st(i) - st0
//
//      DE E8+i is Intel's FSUBP ST(i),ST(0) -- objdump prints it `fsubrp`,
//      the GAS operand-reversal, and the same
//      reversal is confirmed two instructions later in every clip block,
//      where `de f1` (DE F0+1, Intel FDIVRP) has to be
//      `(near - prev.z) / fDenom` to agree with big16324.cpp's independent
//      decode of the identical block.  So the value that reaches the
//      multiply is
//
//          c - (255.0f - c)      i.e. 2*c - 255
//
//      for each of the light object's three colour bytes, and it is written
//      that way above.  It is not nonsense -- 2*c-255 maps a byte onto
//      [-255,+255], which is what a light that can darken as well as
//      brighten needs -- but it is unusual enough to name.  The alternative
//      spelling, if icl emits the pair in the other order from the source
//      above, is the algebraically equal
//
//          fRed = 2.0f * pLightObj->m_nR - 255.0f;
//
//      which should come out as `fild ; fadd %st,%st ; fsubs 255.0f` and
//      would be visibly different.  The one thing this is NOT is
//      `255.0f - c`: that would need FSUBRP (DE E0+i, `de e3`), and the
//      image has DE E8+i.
//
//  (3) THE 40-BYTE COPY.  `*pOut = *pPrev` in all six clip blocks, and
//      `pDst->m_vPos = *pSrc->m_pPos` in the build loop, come out in the
//      image as TEN and THREE individual dword move pairs (0x100654e3..
//      0x1006552a and 0x10064ec0..0x10064ecf), not `rep movsl`.  The
//      rule says struct assignment is always `rep movsl`, and
//      big16324.cpp's /Os copy of the same clip block does use
//      `mov $0x8,%ecx ; rep movsl`.  The reading taken here is that this
//      TU is /O2 and icl unrolls the copy at that level.  If it does not,
//      the spelling that produces the unrolled form is
//
//          memcpy(pOut, pPrev, sizeof(RVert));
//
//      (and `memcpy(&pDst->m_vPos, pSrc->m_pPos, sizeof(LTVec));`), which
//      icl expands inline for a constant size.  Try that only if the
//      struct assignment comes back as `rep movsl`.
//
//  (4) THE INTERPOLANT'S TYPE, inherited from big16324.cpp's coin-flip (2)
//      and unchanged here: all six blocks keep fT in %st and never spill
//      it in this image, so nothing proves the declared type.  `float` is
//      written to match the earlier decode; the alternative is the
//      one-word change `double fDenom, fT, fZ;` in all six blocks.
//
//  (5) THE DEAD 0x3f.  0x100653a1 materialises `mov $0x3f,%eax` and the
//      only reader is the zero-vertex path, because 0x100653e3 reloads the
//      value from 0x1008b874.  Written as
//
//          g_1008b874 = 0x3f;  ...  nClipFlags = g_1008b874;
//
//      exactly as big16324.cpp writes it, and with the same alternative
//
//          nClipFlags = 0x3f;  g_1008b874 = nClipFlags;
//
//      This is the SECOND of the two 0x3f stores; the first, at
//      0x10064d3c, has no such ambiguity and is a plain statement.
//
//  (6) THE LIGHT-PASS VERTEX COUNT.  0x10065112 reads pPoly->m_u50 (+0x50)
//      for the per-vertex light loop even when the build loop took its
//      count from +0x52 because g_CV_FixTJunc was set.  That is what the
//      bytes say and it is written that way; it is recorded here only
//      because it looks like an original bug and a future reader should
//      not "fix" it into `nVerts`.
//
//  (7) THE SQUARED DISTANCE.  0x10065150..0x10065178 computes each of the
//      three differences ONCE and squares it with `fmul %st(0),%st`.
//      Written as `(a - b) * (a - b)` three times, relying on icl to
//      common the subexpression.  If it does not, the spelling that
//      guarantees the single subtract is three named locals:
//
//          dx = pWalk->m_vPos.x - fLightX;  ...
//          fDistSqr = dx*dx + dy*dy + dz*dz;
//
//      -- but note the emission order is z, y, x, i.e. last term first,
//      which is the association rule applied to the sum written
//      x, y, z above.
//
//  (8) THE SHARED sub_100115b0() TAIL at 0x10066a59.  Twelve clipper exits
//      and the fall-through from DrawPrimitive all reach the same call.
//      Written with `goto clipped;` so the emitted sequence has exactly one
//      call site, as the image does.  The alternative, if a `goto` is
//      thought to be wrong for the original source, is a `return`-shaped
//      inner helper that the caller follows with sub_100115b0() -- but that
//      would put the call OUTSIDE the polygon loop, which the image does
//      not, so the goto is the reading with evidence behind it.
//
// *********************************************************************** //
