// *********************************************************************** //
//
//  w54_10048180_osoiqipsf.cpp -- big48180.cpp (0x10048180, the projected-
//  shadow pass) and big49bb0.cpp (0x10049bb0/0x10049bb4, the polygon it
//  draws for each found piece of geometry) in ONE translation unit, built
//  /Os /Oi- /Qip /Qsfalign16.
//
//  WHY ONE TU.  0x10049bb4's only caller is the `call 0x10049bb4` at
//  0x10049abc inside 0x10048180: it enters at the /Qip REGISTER entry
//  (the four bytes at 0x10049bb0 are the stack entry nothing calls), which
//  only a caller compiled in the same /Qip TU does.  The callee is named
//  sub_10049bb0 so the byte comparison maps the stack entry to 0x10049bb0 and the
//  register body sub_10049bb0$ to 0x10049bb4.
//
//  WHY THESE FLAGS.  Both shipped bodies save registers with `push`es after
//  _chkstk and neither has a single lea-nop loop-alignment pad (0x10048180
//  0x10048180..0x10049bb4 has none): the /Os shape.  /Qsfalign16 because
//  at /Os 0x10048180 (it holds a double) otherwise gets icl's 14-byte
//  stack-realigning entry and a second body, which the shipped code lacks.
//  Measured on this merged source (difflib over the normalised
//  instruction streams, matched/total):
//                               0x10048180        0x10049bb4
//      /O2 /Oi- (the old split)  0.315 (1441 ins)  0.275 (1706 ins)
//      /O2 /Oi- /Qip /Qsfalign16 0.337             0.275
//      /Os /Oi- /Qip /Qsfalign16 0.520 (1270 ins)  0.719 (1688 ins)
//  against the shipped 1516 and 1671 instructions.  /Oi- stays: the bodies
//  CALL the CRT sqrt (0x1007fc98).  Neither function was identical before
//  the move.
//
//  The two files' own headers follow unchanged; where they say "/O2" they
//  describe the measurements made before this merge.
//
// *********************************************************************** //

// *********************************************************************** //
//
//  MODULE:   BIG_48180 (.CPP)
//
//  PURPOSE:  One function -- 0x10048180, 6708 bytes, one caller.  This is
//            d3d.ren's PROJECTED-SHADOW pass: for each of up to eight shadow
//            directions it finds the world geometry the object's shadow can
//            fall on, builds a shadow-projection matrix plus a six-plane
//            clip volume, and hands each found piece of geometry to
//            0x10049bb4 to be drawn.
//
//  Compiler: Intel C/C++ 4.0.  /O2 -- the outgoing-argument idiom throughout
//            is `add $-N,%esp` followed by `mov %reg,K(%esp)` and never a run
//            of `push`es (the four pushes at 0x10048765 are a special case,
//            see the note on sub_10069ed0), and `&this->m_v484` is built with
//            `mov -0x174(%ebp),%edi ; add $0x484,%edi` rather than a `lea`.
//            Both are /O2 tells.
//
//  WHAT THE FUNCTION IS, and what proves it.  Four independent tells:
//
//    * the DEBUG STRING at 0x100a9f70, printed at the very end through
//      0x1008cbcc's slot +0x14:
//          "Find: %d, BSP: %d, Setup: %d, Draw: %d, Total: %d ticks"
//      -- a find/setup/draw pipeline with per-phase timers.
//
//    * the MATRIX built at -0x2c8(%ebp) between 0x10048f10 and 0x10049148 is
//      the textbook point-light shadow-projection matrix.  Reading the
//      sixteen stores off the bytes gives, with k = (n.b - d):
//          [ k-b.x*n.x   -b.x*n.y    -b.x*n.z    -b.x*-d ]
//          [ -b.y*n.x    k-b.y*n.y   -b.y*n.z    -b.y*-d ]
//          [ -b.z*n.x    -b.z*n.y    k-b.z*n.z   -b.z*-d ]
//          [ -n.x        -n.y        -n.z        k+d     ]
//      with b = the light point and (n,d) the receiving plane.
//
//    * the SIX PLANES at -0x478..-0x419 are a shadow frustum: a near plane
//      along the light direction, a far plane offset by m_fD8, and four side
//      planes each the cross product of a frustum-corner edge with the
//      right or up vector, normalised.
//
//    * the caller (0x10044111) gates it on `this->m_698 & 2` and the whole
//      body is bracketed by SetTexture(0, g_p1008da78) and
//      SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 1).
//
//  WHAT IS DECODED AND WHAT IS NOT.  Every region of the 6708 bytes is
//  decoded; there is no `// UNDECODED:` banner in this file.  The two
//  regions that looked opaque at first both resolved:
//
//    * 0x10048f10..0x10049148 -- the shadow matrix above.  The by-value
//      copies at -0x2644..-0x2660 and the destination pointer parked at
//      -0x2670 and RELOADED for all sixteen stores are the signature of an
//      INLINED helper taking two vectors by value; it is written below as
//      `MakeShadowMatrix` and marked `inline` so /Ob1 inlines it.  The
//      helper has no shipped address of its own -- there is no out-of-line
//      copy anywhere in the image -- so it cannot be named `sub_<addr>`.
//
//    * 0x10049148..0x10049a82 -- 2354 bytes of straight-line x87 with no
//      calls at all.  It is TWO INLINED COPIES of ltmatrix.h's MatMul.  The
//      proof is the store order: the sixteen results of the first land at
//      -0x208, -0x1f8, -0x1e8, -0x1d8, -0x204, -0x1f4, ... which is
//      m[0][0] m[1][0] m[2][0] m[3][0] m[0][1] m[1][1] ... -- the header's
//      own column-by-column order, and the second copy's stores at -0x1c8
//      follow the SAME sixteen-step order without a single exception.  The
//      accumulation inside each element runs last-term-first
//      (M1[0][3]*M2[3][0] before M1[0][0]*M2[0][0]), which is the module's
//      n-ary float fingerprint and matches the byte-exact 0x10014720.
//      Terms whose left factor is a compile-time zero collapse to
//      `fadds <0.0f from the pool>`; the 0.0f constants at 0x100a9e68 ..
//      0x100a9ec4 are exactly those folded terms and nothing else.
//
//  ONE DELIBERATE DEPARTURE, and it is about the INLINER, not the source.
//  d3d.ren calls the _CVector members out of line at most sites in this
//  function (operator+ 0x100155f0, operator- 0x10015640, operator*(T)
//  0x10015690, operator-() 0x10023350, Dot 0x10027f50, Cross 0x10015590,
//  Norm 0x10015530) but INLINES Cross and Norm at four sites each -- the
//  four side planes, where the whole cross/normalise/dot chain is one
//  straight run of x87 with a bare `call 0x1007fc98` (the CRT sqrt) in the
//  middle.  Reproducing icl's per-site inline decision is not something
//  source shape controls, so this file DECLARES the members without
//  defining them (guaranteeing the calls) and writes the four inlined sites
//  out longhand in the arithmetic the bytes show.  The four longhand blocks
//  are transcriptions, not guesses: every operand and every sign below is
//  read off the disassembly.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// ======================================================================= //
//  THE VECTOR CLASS
//
//  src/LT2/sdk/inc/ltvector.h's _CVector<float>, spelled out locally so that the
//  out-of-line members can carry their addresses in their names.  Every one
//  of them is DECLARED and NOT DEFINED here: this translation unit must not
//  be able to inline them, because the shipped function calls them.
// ======================================================================= //

struct LTVec
{
	// No constructors are declared: nothing in this function constructs a
	// vector from components (0x10015480 does that job by name, below), and
	// leaving the class a plain aggregate keeps `LTPln m_Planes[6]` and the
	// 0x100-byte block that holds it free of any implicit initialisation.

	// 0x10015480 -- the (x,y,z) constructor, out of line.  `ret $0xc` with
	// three stack floats stored to +0x8, +0x4, +0x0 of %ecx and %ecx
	// answered in %eax.  Written as a named member because a constructor's
	// mangled name has no room for an address.
	LTVec* sub_10015480(float mx, float my, float mz);

	void   sub_10015530(float nVal);            // 0x10015530  Norm
	LTVec  sub_10015590(const LTVec v) const;   // 0x10015590  Cross
	LTVec  sub_100155f0(const LTVec v) const;   // 0x100155f0  operator +
	LTVec  sub_10015640(const LTVec v) const;   // 0x10015640  operator -
	LTVec  sub_10015690(float v) const;         // 0x10015690  operator * (T)
	LTVec  sub_10023350() const;                // 0x10023350  operator - ()
	float  sub_10027f50(LTVec v) const;         // 0x10027f50  Dot

	float x;                                    // +0x00
	float y;                                    // +0x04
	float z;                                    // +0x08
};


// src/LT2/sdk/inc/ltplane.h's LTPlane: a normal then a distance, sixteen bytes.
// Pinned here by the stride of the six-element run at -0x478(%ebp): the
// normals land at -0x478, -0x468, -0x458, -0x448, -0x438, -0x428 and each
// one is followed four bytes later by a single `fstps` of a dot product.
struct LTPln
{
	LTVec m_Normal;                                     // +0x00
	float m_Dist;                                       // +0x0c
};


// src/LT2/sdk/inc/ltmatrix.h's LTMatrix.  Retyped rather than included so that
// MatMul below can be given a name of its own; the layout is the header's.
struct LTMat
{
	float m[4][4];
};


// ltmatrix.h:172, verbatim, and `inline` because the shipped code has TWO
// copies of it inlined back to back and no call.
#define M1 pMat1->m
#define M2 pMat2->m

inline void MatMul4(LTMat* pDest, LTMat* pMat1, LTMat* pMat2)
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
//  0x10048f10..0x10049148 -- THE SHADOW-PROJECTION MATRIX, inlined.
//
//  WHAT PINS IT.  Three things say this is an inlined function rather than
//  sixteen assignments written in place:
//
//   1. Fresh by-value copies of both vectors and the float are made into
//      slots that nothing else in the 6708 bytes touches:
//          -0x2660/-0x265c/-0x2658  <- -0x518/-0x514/-0x510  (vPoint)
//          -0x2650/-0x264c/-0x2648  <- -0x4e8/-0x4e4/-0x4e0  (vNormal)
//          -0x2644                  <- -0x4dc                (fDist)
//      Copying twelve bytes to feed a `fmuls` that could have read the
//      original is what a by-value parameter looks like and nothing else.
//   2. The destination is parked at -0x2670(%ebp) and RELOADED before every
//      single one of the sixteen stores -- a pointer parameter icl will not
//      keep in a register across the writes it may alias.
//   3. The sixteen stores go +0x00, +0x04, +0x08 ... +0x3c in strict order,
//      which is source order for m[0][0], m[0][1] ... m[3][3].
//
//  The value `k` (the FPU's %st(1) from 0x10048f92 all the way to the
//  `fadds` at 0x1004913f) is never spilled: it is a single expression
//  computed once and held on the x87 stack across all sixteen rows.
//
//  Every sign is off the bytes.  The three diagonal elements are the only
//  ones that reach `k` (`d8 e9`/`d8 ea` = Intel FSUBR st,st(i), i.e.
//  st0 = st(i) - st0, so `k - n*b` and not the other way round); the twelve
//  off-diagonal products are all `flds b.i ; fchs ; fmuls n.j`; the +0x0c
//  column is `flds b.i ; fchs ; flds d ; fchs ; fmulp`, i.e. the source
//  really does write TWO negations and not one multiply by +d; row three is
//  three bare `fchs`; and m[3][3] is `fadds` and not `fsubs`.
// ======================================================================= //

inline void MakeShadowMatrix(LTMat* pMat, LTVec vPoint, LTVec vNormal,
                             float fDist)
{
	float k = (vNormal.x*vPoint.x + vNormal.y*vPoint.y + vNormal.z*vPoint.z) - fDist;

	pMat->m[0][0] = k - vPoint.x * vNormal.x;
	pMat->m[0][1] = -vPoint.x * vNormal.y;
	pMat->m[0][2] = -vPoint.x * vNormal.z;
	pMat->m[0][3] = -vPoint.x * -fDist;

	pMat->m[1][0] = -vPoint.y * vNormal.x;
	pMat->m[1][1] = k - vPoint.y * vNormal.y;
	pMat->m[1][2] = -vPoint.y * vNormal.z;
	pMat->m[1][3] = -vPoint.y * -fDist;

	pMat->m[2][0] = -vPoint.z * vNormal.x;
	pMat->m[2][1] = -vPoint.z * vNormal.y;
	pMat->m[2][2] = k - vPoint.z * vNormal.z;
	pMat->m[2][3] = -vPoint.z * -fDist;

	pMat->m[3][0] = -vNormal.x;
	pMat->m[3][1] = -vNormal.y;
	pMat->m[3][2] = -vNormal.z;
	pMat->m[3][3] = k + fDist;
}


// ======================================================================= //
//  THE OBJECTS THIS FUNCTION TOUCHES
// ======================================================================= //

// The scene description hanging off `this->m_pDesc` (this+0x00).  Every
// offset below is a load in the entry block, 0x100481cd..0x1004827f:
//   +0xd4  a pointer, null-tested and never used -> a guard
//   +0xd8  float, the far-plane offset            (-0x150 / the -0x30 spill)
//   +0xdc  float, the shadow length               (-0x184 / -0x38)
//   +0xe0  float, the half-height                 (-0x148 / -0x158)
//   +0xe4  float, the half-width                  (-0x14c / -0x160)
//   +0xe8  LTVector, copied with `rep movsl` 3    (-0x25f8)
struct S10048180Desc
{
	uint8   m_Reserved00[0xd4];                         // +0x00..+0xd3
	void*   m_pD4;                                      // +0xd4
	float   m_fD8;                                      // +0xd8
	float   m_fDC;                                      // +0xdc
	float   m_fE0;                                      // +0xe0
	float   m_fE4;                                      // +0xe4
	LTVec   m_vE8;                                      // +0xe8
};


// The object being shadowed, `this->m_pObject` (this+0x04).  Only one field
// is reached: an LTVector at +0x18c.  It is read two ways in the shipped
// bytes -- through the accessor 0x100158b0 in the first loop (three calls)
// and as a plain `lea 0x18c(%reg)` in the second (three sites) -- so BOTH
// spellings are in the source and both are written below.
struct S10048180Obj
{
	// 0x100158b0 -- `push %esi ; lea 0x18c(%ecx),%eax ; add $0x4,%esp ;
	// ret`.  __thiscall, no arguments, answers &m_vPos.  The push/add pair
	// is icl's dead four-byte frame slot.
	LTVec* sub_100158b0();

	uint8  m_Reserved00[0x18c];                         // +0x00..+0x18b
	LTVec  m_vPos;                                      // +0x18c
};


// The per-direction result list the world query fills in.  Its shape is read
// straight off the frame: the array base is -0x25ec(%ebp), the stride is
// 0x404 (`imul $0x404,%ebx,%eax`), the count lives at +0x400 within the
// element (`-0x21ec(%ebp,%eax,1)`, and -0x21ec - -0x25ec == 0x400), and the
// draw loop indexes the element as dwords (`mov (%esi,%ebx,4),%edx`).  So
// 256 pointers followed by an unsigned count.  Eight of them fit exactly
// between -0x25ec and -0x5cc, which is where the frame's next local starts.
struct S10048180Found
{
	void*  m_pItems[256];                               // +0x000
	uint32 m_nCount;                                    // +0x400
};


// The 32-byte query block handed to the world traversal.  -0x263c(%ebp) is
// its address and the four fields are written in this order.
struct S10048180Query
{
	S10048180Found* m_pDest;                            // +0x00
	LTVec           m_vStart;                           // +0x04
	LTVec           m_vEnd;                             // +0x10
	float           m_fRadius;                          // +0x1c
};


// The 0x100-byte block at -0x518(%ebp) whose address is the first argument
// of every 0x10049bb4 call.  It is one contiguous run: the last plane ends
// at -0x419 and -0x518 - -0x418 is exactly 0x100.
struct S10048180Shadow
{
	LTVec  m_vStart;                                    // +0x00  -0x518
	LTVec  m_vUp;                                       // +0x0c  -0x50c
	LTVec  m_vRight;                                    // +0x18  -0x500
	LTVec  m_vNormal;                                   // +0x24  -0x4f4
	LTVec  m_vDir;                                      // +0x30  -0x4e8
	float  m_fDist;                                     // +0x3c  -0x4dc
	LTVec  m_vEnd;                                      // +0x40  -0x4d8
	LTVec  m_vCorner;                                   // +0x4c  -0x4cc
	float  m_fHalfH;                                    // +0x58  -0x4c0
	float  m_fHalfW;                                    // +0x5c  -0x4bc
	LTMat  m_mTex;                                      // +0x60  -0x4b8
	LTPln  m_Planes[6];                                 // +0xa0  -0x478
};


// ======================================================================= //
//  THE WORLD QUERY AT 0x1008beac + 0x6c
//
//      mov 0x1008beac,%ebx
//      lea 0x6c(%ebx),%esi        <- `this`
//      ...
//      mov 0x6c(%ebx),%ebx        <- the vptr, i.e. *(this)
//      mov 0xc(%ebx),%eax         <- slot 3
//      mov %esi,%ecx ; call *%eax
//
//  So 0x1008beac points at an object carrying an embedded interface at
//  +0x6c whose own first dword is a vtable pointer, and slot +0x0c is called
//  __thiscall with four pushed dwords which the callee pops.  Reaching it by
//  casting a byte pointer is what keeps icl from devirtualising -- the call
//  in the shipped bytes is indirect.
//
//  The arguments are read off 0x10048505..0x1004851e:
//      arg0  &m_pObject->m_vPos       (the answer of 0x100158b0)
//      arg1  0x10073700               a function address, materialised with
//                                     `mov $0x10073700,%edi`
//      arg2  &query
//      arg3  0
//  0x10073700 confirms the pairing from the other side: it is __cdecl, reads
//  its second argument at 0xc(%ebp) and immediately copies twelve bytes from
//  `0x4(%eax)` -- query.m_vStart.
// ======================================================================= //

class I1008beac6c
{
public:
	virtual void Slot00() = 0;
	virtual void Slot04() = 0;
	virtual void Slot08() = 0;
	virtual void Slot0c(LTVec* pPos, void* pfnCallback,
	                    S10048180Query* pQuery, uint32 nFlags) = 0;
};

extern uint8* g_p1008beac;                              // 0x1008beac

// 0x10073700 -- the per-object callback the query invokes.  Only its address
// is used; `mov $0x10073700,%edi` is a DIR32 relocation, not a call.
extern void sub_10073700();


// ======================================================================= //
//  THE DIRECT3D 7 DEVICE AT 0x1008d988
//
//  Same object d3ddraw.cpp, d3dopt2d.cpp and d3dbatchd.cpp already pin, and
//  the same explicit-vtable __stdcall shape.  Three slots are used here and
//  all three agree with the IDirect3DDevice7 numbering that d3ddraw.cpp
//  established (BeginScene at +0x14, SetRenderState at +0x50):
//      +0x50  SetRenderState  (index 20)
//      +0x88  GetTexture      (index 34)
//      +0x8c  SetTexture      (index 35)
// ======================================================================= //

struct D3DDev7;

struct D3DDev7Vtbl
{
	void* _slots00_4c[20];                              // +0x00..+0x4c
	long (__stdcall* SetRenderState)(D3DDev7*, uint32, uint32);   // +0x50
	void* _slots54_84[13];                              // +0x54..+0x84
	long (__stdcall* GetTexture)(D3DDev7*, uint32, void**);       // +0x88
	long (__stdcall* SetTexture)(D3DDev7*, uint32, void*);        // +0x8c
};

struct D3DDev7
{
	D3DDev7Vtbl* lpVtbl;
};

extern D3DDev7* g_pD3DDevice;                           // 0x1008d988

#define D3DRENDERSTATE_ALPHABLENDENABLE      27         // 0x1b


// 0x1000adc0, reconstructed in d3dbatcha.cpp: the scoped render-state
// saver's set-and-remember.  It stores the state id at +0x00, calls
// GetRenderState into +0x04 and then SetRenderState with the new value.  The
// matching restore at 0x10049b21 is written out in line below, which is what
// the shipped bytes show -- there is no destructor call.
class C1000adc0
{
public:
	C1000adc0* sub_1000adc0(uint32 nState, uint32 nValue);

	uint32 m_nState00;                                  // +0x00
	uint32 m_nOld04;                                    // +0x04
};


// The engine's RenderStruct at 0x1008cbcc.  Slot +0x14 is a __cdecl
// variadic console printer: `mov 0x1008cbcc,%eax ; call *0x14(%eax)` with
// six dwords pushed and `add $0x18,%esp` afterwards.
struct RenderStructCallbacks
{
	uint8 m_Reserved00[0x14];                           // +0x00..+0x13
	void  (*ConsolePrint)(const char* pszMsg, ...);     // +0x14
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// ======================================================================= //
//  FILE-SCOPE STATE
// ======================================================================= //

// 0x100893ec -- how many shadow directions are live.  SIGNED: the clamp at
// 0x100481a1 is `cmp $0x8,%eax ; jge / jle` and the zero test at 0x10049b96
// is `test %eax,%eax ; jl ; je`.
extern int32 g_n100893ec;

extern void* g_p1008d960;                               // 0x1008d960
extern void* g_p1008da78;                               // 0x1008da78, the
                                                        // shadow texture
extern void* g_p1008ee60;                               // 0x1008ee60
extern void* g_p1008ca38;                               // 0x1008ca38, the
                                                        // timing cvar

// 0x1008ee50 / 0x1008ee54 -- a function-local static and its one-shot guard
// byte.  `mov 0x1008ee50,%al ; test $0x1,%al ; jne past ; or $0x1,%al ;
// mov %al,0x1008ee50 ; <construct>` is exactly the guard icl emits for a
// block-scope static with a non-trivial constructor.  It is written out by
// hand below because the constructor has to be called by a name carrying its
// address (0x10015480) for the harness to place it.
extern uint8 g_b1008ee50;
extern LTVec g_v1008ee54;

// 0x100a9f70.
extern const char g_sz100a9f70[];


// ======================================================================= //
//  0x10069ed0 -- 728 bytes.  Builds a vector perpendicular to the first
//  argument.
//
//  __cdecl and THREE arguments, both read off the callee: it takes them from
//  0x4/0x8/0xc(%esp) before pushing anything, and when the second is null it
//  substitutes a local (0.0f, 1.0f, 0.0f) at 0x1006a18a and rejoins.  The
//  third is the output -- everything after the entry writes through it.
//
//  The call site pushes FOUR dwords (0x10048765..0x10048775).  The first
//  push is not an argument: it reserves the sixteen-byte outgoing area that
//  the Cross call immediately after it needs (`ret $0x10`), and the code
//  refills 0x0(%esp) and 0x4(%esp) for that call without ever restoring the
//  stack.  %edi at that push is -0x4e8(%ebp) only because `rep movsl` left
//  it there; nothing reads it.
// ======================================================================= //

extern void sub_10069ed0(LTVec* pIn, LTVec* pHint, LTVec* pOut);


// ======================================================================= //
//  THE CLASS
//
//  Only three members are reached.  this+0x00 and this+0x04 are the two
//  pointers above; this+0x484 is walked with a stride of twelve for
//  `nNumDirs` iterations (`mov -0x174(%ebp),%edx ; add $0x484,%edx` then
//  `add $0xc,%ecx` each turn), so it is an LTVector array.  Element [0] is
//  the only one this function writes.
// ======================================================================= //

struct Shadow49bb0;
struct Item49bb0;

class C10048180
{
public:
	void sub_10048180();

	// 0x1004b720 -- 11204 bytes, the alternative shadow path.  __thiscall
	// (`mov %ecx,%esi` at 0x1004b74a) with one pushed argument.
	void sub_1004b720(int32 nNumDirs);

	// 0x10049bb4 -- draws one found piece of geometry.  __thiscall with two
	// pushed arguments, which the callee reads at 0x8(%ebp) and 0xc(%ebp).
	void sub_10049bb0(Shadow49bb0* pShadow, Item49bb0* pItem);

	S10048180Desc* m_pDesc;                             // +0x00
	S10048180Obj*  m_pObject;                           // +0x04
	uint8          m_Reserved08[0x47c];                 // +0x08..+0x483
	LTVec          m_vDirs[8];                          // +0x484
};


// ======================================================================= //
//  0x10048180 -- 6708 bytes, one caller (0x10044111).
//
//  REGION BY REGION, and what pins each.
//
//  0x10048180..0x100481f7  ENTRY AND GUARDS.  `mov $0x2690,%eax ; call
//      0x100805a0` is _chkstk for a 9872-byte frame.  The count is clamped
//      to eight, then four guards, then the g_p1008ee60 fork into
//      sub_1004b720.  See the COIN FLIP note at the bottom about the exact
//      spelling of the clamp.
//
//  0x100481f8..0x100482df  THE CACHED DESCRIPTION.  Four floats and a vector
//      are lifted out of m_pDesc once (icl keeps each float twice, an
//      integer copy for passing and an `fstpl` double spill for arithmetic
//      -- one source variable each), the block-scope static is constructed
//      on first entry, m_vDirs[0] is set from it and normalised.
//
//  0x100482e0..0x10048585  THE FIND LOOP.  For each direction: zero the
//      result list, build a start point and a normalised direction, build
//      the query block, and run the world traversal.  The loop is written
//      indexed and icl strength-reduces it to three induction variables
//      (i*0x404 for the lists, i*0xc for the local array, and a pointer
//      into m_vDirs), as icl does for an indexed loop.
//
//  0x10048586..0x100485f5  DEVICE STATE.  Save texture stage 0, bind the
//      shadow texture, and turn alpha blending on through the saver.
//
//  0x100485f6..0x10049a82  THE SETUP LOOP, and 92% of the bytes.  Per
//      direction with a non-empty result list: the shadow block, the four
//      frustum corners, the six clip planes, the shadow matrix and the two
//      matrix multiplies that turn it into a texture matrix.
//
//  0x10049a84..0x10049acc  THE DRAW LOOP.  The count is RE-READ from memory
//      on every turn (`mov -0x21ec(%ebp,%edi,1),%eax` inside the loop), so
//      the source names the member in the condition and does not cache it.
//
//  0x10049afc..0x10049b8b  RESTORE AND REPORT.
//
//  0x10049b8c..0x10049bab  The three cross-jumped exit trampolines and the
//      two sunk test blocks.
// ======================================================================= //

void C10048180::sub_10048180()
{
	uint32           nFindTime = 0;                     // -0x25fc
	uint32           nSetupTime = 0;                    // -0x2600
	uint32           nBSPTime = 0;                      // -0x2604
	S10048180Found   found[8];                          // -0x25ec
	LTVec            vLocalDirs[8];                     // -0x59c
	S10048180Query   query;                             // -0x263c
	S10048180Shadow  shadow;                            // -0x518
	C1000adc0        stateSave;                         // -0x520
	void*            pOldTexture;                       // -0x524
	int32            i;
	uint32           j;
	uint32           nTotal;

	int32 nNumDirs = LTMIN(g_n100893ec, 8);

	if (nNumDirs <= 0)
		return;

	// 0x100481b3..0x100481e6 -- three separate null guards, each with its
	// own `test %eax,%eax ; je`, all cross-jumping to the one exit at
	// 0x10049b91.
	if (!g_p1008d960)
		return;

	if (!g_p1008da78)
		return;

	if (!m_pDesc->m_pD4)
		return;

	// 0x100481df -- the other shadow renderer.  `push %edi ; mov %ebx,(%esp)`
	// is the /O2 one-argument outgoing area, and %ecx still holds `this`
	// from the entry block, so this is __thiscall.
	if (g_p1008ee60)
	{
		sub_1004b720(nNumDirs);
		return;
	}

	// 0x100481f8..0x1004827f.  m_pDesc is loaded TWICE here (`mov (%edx),%eax`
	// followed by `mov (%edx),%edx`) -- two source reads of the member, not
	// one cached pointer.
	{
		float fD8 = m_pDesc->m_fD8;                     // -0x150 / -0x30
		float fDC = m_pDesc->m_fDC;                     // -0x184 / -0x38
		float fE0 = m_pDesc->m_fE0;                     // -0x148 / -0x158
		float fE4 = m_pDesc->m_fE4;                     // -0x14c / -0x160
		LTVec vE8 = m_pDesc->m_vE8;                     // -0x25f8, rep movsl

		// 0x10048281 -- the block-scope static and its guard byte.  The flag
		// is set BEFORE the constructor runs, which is what the shipped
		// order (`or $0x1,%al ; mov %al,0x1008ee50 ; ... ; call`) shows.
		if (!(g_b1008ee50 & 1))
		{
			g_b1008ee50 |= 1;
			g_v1008ee54.sub_10015480(0.0f, -1.0f, 0.0f);
		}

		// 0x100482af -- struct assignment, `rep movsl` of three dwords, then
		// Norm(1.0f) with the 1.0f pushed as `movl $0x3f800000,(%esp)`.
		m_vDirs[0] = g_v1008ee54;
		m_vDirs[0].sub_10015530(1.0f);

		nTotal = 0;

		// ------------------------------------------------------------- //
		//  0x100482e0..0x10048573 -- THE FIND LOOP.
		// ------------------------------------------------------------- //
		for (i = 0; i < nNumDirs; i++)
		{
			// -0x168, computed once in the loop preheader and reused by both
			// loops as a double spill.  See the COIN FLIP note.
			float fLength = fDC + fD8;

			LTVec  vStart;                              // -0x261c
			LTVec  vDir;                                // -0x2610
			LTVec* pPos;

			query.m_pDest = &found[i];
			found[i].m_nCount = 0;

			// 0x1004834e -- through the accessor, not the member.  Three
			// calls to 0x100158b0 in this loop, none in the next.  The
			// accessor's answer is taken into a variable because the shipped
			// code calls it BEFORE the operator*(T) whose result it then
			// subtracts (`call 0x100158b0 ; ... ; mov %eax,%ebx ;
			// call 0x10015690 ; mov %ebx,%ecx ; call 0x10015640`), and a
			// one-expression spelling would have icl evaluate the argument
			// first and swap the two.
			pPos   = m_pObject->sub_100158b0();
			vStart = pPos->sub_10015640(m_vDirs[i].sub_10015690(fDC));

			pPos = m_pObject->sub_100158b0();
			vLocalDirs[i] = pPos->sub_100155f0(vE8).sub_10015640(vStart);
			vLocalDirs[i].sub_10015530(1.0f);

			vDir = vLocalDirs[i];

			query.m_vStart  = vStart;
			query.m_vEnd    =
			        query.m_vStart.sub_100155f0(vDir.sub_10015690(fLength));
			query.m_fRadius = LTMAX(fE0, fE4);

			// 0x100484f5 -- a third call to the accessor for the query.
			((I1008beac6c*)(g_p1008beac + 0x6c))->Slot0c(
			        m_pObject->sub_100158b0(),
			        (void*)sub_10073700,
			        &query,
			        0);

			nTotal += found[i].m_nCount;
		}

		if (nTotal == 0)
			return;

		// ------------------------------------------------------------- //
		//  0x10048586..0x100485ed -- DEVICE STATE.
		// ------------------------------------------------------------- //
		g_pD3DDevice->lpVtbl->GetTexture(g_pD3DDevice, 0, &pOldTexture);
		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 0, g_p1008da78);

		stateSave.sub_1000adc0(D3DRENDERSTATE_ALPHABLENDENABLE, 1);

		// ------------------------------------------------------------- //
		//  0x100485f6..0x10049afb -- THE SETUP LOOP.
		// ------------------------------------------------------------- //
		for (i = 0; i < nNumDirs; i++)
		{
			float fLength = fDC + fD8;

			LTVec vC0;                                  // -0x3c4
			LTVec vC1;                                  // -0x3a0
			LTVec vC2;                                  // -0x37c
			LTVec vC3;                                  // -0x358
			LTVec vTmp;

			LTMat mShadow;                              // -0x2c8
			LTMat mBasis;                               // -0x288
			LTMat mScale;                               // -0x248
			LTMat mTemp;                                // -0x208
			LTMat mFinal;                               // -0x1c8

			// 0x10048625 -- the whole body is skipped when nothing was
			// found.  A separate test from the draw loop's own condition
			// below, which is why the count is tested twice.
			if (found[i].m_nCount == 0)
				continue;

			// 0x10048645/0x1004864b, in that store order.
			shadow.m_fHalfW = fE4;
			shadow.m_fHalfH = fE0;

			// 0x10048657 -- `lea 0x18c(%ecx),%ecx` on m_pObject, i.e. the
			// member and not the accessor, for all three reads in this loop.
			shadow.m_vStart = m_pObject->m_vPos.sub_10015640(
			        vLocalDirs[i].sub_10015690(fDC));
			shadow.m_vDir   = vLocalDirs[i];
			shadow.m_vEnd   = shadow.m_vStart.sub_100155f0(
			        vLocalDirs[i].sub_10015690(fLength));
			shadow.m_fDist  = shadow.m_vDir.sub_10027f50(shadow.m_vEnd);

			shadow.m_vNormal = vLocalDirs[i];

			sub_10069ed0(&shadow.m_vNormal, LTNULL, &shadow.m_vRight);
			shadow.m_vUp = shadow.m_vNormal.sub_10015590(shadow.m_vRight);

			// --------------------------------------------------------- //
			//  0x100487b3..0x100489d8 -- the four frustum corners and the
			//  one corner kept in the block.  Note that these read the
			//  cached fE0/fE4 and not shadow.m_fHalfH/m_fHalfW: the muls
			//  are `mov -0x148(%ebp),%edx` and `mov -0x14c(%ebp),%edx`.
			// --------------------------------------------------------- //
			vC0 = shadow.m_vEnd.sub_10015640(shadow.m_vUp.sub_10015690(fE0));
			vC1 = shadow.m_vEnd.sub_100155f0(shadow.m_vUp.sub_10015690(fE0));
			vC2 = shadow.m_vEnd.sub_100155f0(shadow.m_vRight.sub_10015690(fE4));
			vC3 = shadow.m_vEnd.sub_10015640(shadow.m_vRight.sub_10015690(fE4));

			// The fifth corner is TWO statements, not one chained
			// expression: the shipped order is mul(up), sub, mul(right),
			// sub, and a chain would have icl push the second product
			// first and emit mul(right) ahead of mul(up).
			vTmp = shadow.m_vEnd.sub_10015640(shadow.m_vUp.sub_10015690(fE0));
			shadow.m_vCorner =
			        vTmp.sub_10015640(shadow.m_vRight.sub_10015690(fE4));

			// --------------------------------------------------------- //
			//  0x100489da..0x10048ab1 -- planes 0 and 1, the near and far
			//  caps.  Both distances are Dot calls against the object's own
			//  position, and the far cap's plane is the negated normal
			//  pushed out along it by m_fD8.
			// --------------------------------------------------------- //
			shadow.m_Planes[0].m_Normal = shadow.m_vNormal;
			shadow.m_Planes[0].m_Dist   =
			        shadow.m_Planes[0].m_Normal.sub_10027f50(m_pObject->m_vPos);

			shadow.m_Planes[1].m_Normal = shadow.m_vNormal.sub_10023350();
			shadow.m_Planes[1].m_Dist   =
			        shadow.m_Planes[1].m_Normal.sub_10027f50(
			                m_pObject->m_vPos.sub_100155f0(
			                        shadow.m_vNormal.sub_10015690(fD8)));

			// --------------------------------------------------------- //
			//  0x10048ab2..0x10048f0f -- the four SIDE planes.
			//
			//  Written longhand because icl inlined Cross and Norm here in
			//  the shipped bytes (see the header note); the natural
			//  spelling of the first would be
			//
			//      shadow.m_Planes[2].m_Normal =
			//          (vC0 - shadow.m_vStart).Cross(shadow.m_vRight);
			//      shadow.m_Planes[2].m_Normal.Norm(1.0f);
			//      shadow.m_Planes[2].m_Dist =
			//          VEC_DOT(shadow.m_Planes[2].m_Normal, shadow.m_vStart);
			//
			//  and this is that, expanded.  Every term is off the bytes:
			//  planes 2 and 3 are (corner - start) x axis and planes 4 and 5
			//  are axis x (corner - start), which is the opposite winding --
			//  the `de ea` FSUBP operand order at 0x10048ada versus
			//  0x10048c02 is what separates them.
			//
			//  The array indices are the order the STORES appear in:
			//  -0x458 (plane 2), then -0x438 (plane 4), then -0x448
			//  (plane 3), then -0x428 (plane 5).
			// --------------------------------------------------------- //

			// plane 2 -- (vC0 - start) x right, then normalise.
			{
				float ax = vC0.x - shadow.m_vStart.x;
				float ay = vC0.y - shadow.m_vStart.y;
				float az = vC0.z - shadow.m_vStart.z;
				float mag;

				shadow.m_Planes[2].m_Normal.x =
				        shadow.m_vRight.y * az - shadow.m_vRight.z * ay;
				shadow.m_Planes[2].m_Normal.y =
				        shadow.m_vRight.z * ax - shadow.m_vRight.x * az;
				shadow.m_Planes[2].m_Normal.z =
				        shadow.m_vRight.x * ay - shadow.m_vRight.y * ax;

				mag = (float)sqrt(
				        shadow.m_Planes[2].m_Normal.x * shadow.m_Planes[2].m_Normal.x +
				        shadow.m_Planes[2].m_Normal.y * shadow.m_Planes[2].m_Normal.y +
				        shadow.m_Planes[2].m_Normal.z * shadow.m_Planes[2].m_Normal.z);

				if (mag != 0.0f)
				{
					float inv = 1.0f / mag;

					shadow.m_Planes[2].m_Normal.x = shadow.m_Planes[2].m_Normal.x * inv;
					shadow.m_Planes[2].m_Normal.y = shadow.m_Planes[2].m_Normal.y * inv;
					shadow.m_Planes[2].m_Normal.z = shadow.m_Planes[2].m_Normal.z * inv;
				}

				shadow.m_Planes[2].m_Dist =
				        shadow.m_Planes[2].m_Normal.x * shadow.m_vStart.x +
				        shadow.m_Planes[2].m_Normal.y * shadow.m_vStart.y +
				        shadow.m_Planes[2].m_Normal.z * shadow.m_vStart.z;
			}

			// plane 4 -- right x (vC1 - start), then normalise.
			{
				float ax = vC1.x - shadow.m_vStart.x;
				float ay = vC1.y - shadow.m_vStart.y;
				float az = vC1.z - shadow.m_vStart.z;
				float mag;

				shadow.m_Planes[4].m_Normal.x =
				        ay * shadow.m_vRight.z - az * shadow.m_vRight.y;
				shadow.m_Planes[4].m_Normal.y =
				        az * shadow.m_vRight.x - ax * shadow.m_vRight.z;
				shadow.m_Planes[4].m_Normal.z =
				        ax * shadow.m_vRight.y - ay * shadow.m_vRight.x;

				mag = (float)sqrt(
				        shadow.m_Planes[4].m_Normal.x * shadow.m_Planes[4].m_Normal.x +
				        shadow.m_Planes[4].m_Normal.y * shadow.m_Planes[4].m_Normal.y +
				        shadow.m_Planes[4].m_Normal.z * shadow.m_Planes[4].m_Normal.z);

				if (mag != 0.0f)
				{
					float inv = 1.0f / mag;

					shadow.m_Planes[4].m_Normal.x = shadow.m_Planes[4].m_Normal.x * inv;
					shadow.m_Planes[4].m_Normal.y = shadow.m_Planes[4].m_Normal.y * inv;
					shadow.m_Planes[4].m_Normal.z = shadow.m_Planes[4].m_Normal.z * inv;
				}

				shadow.m_Planes[4].m_Dist =
				        shadow.m_Planes[4].m_Normal.x * shadow.m_vStart.x +
				        shadow.m_Planes[4].m_Normal.y * shadow.m_vStart.y +
				        shadow.m_Planes[4].m_Normal.z * shadow.m_vStart.z;
			}

			// plane 3 -- (vC2 - start) x up, then normalise.
			{
				float ax = vC2.x - shadow.m_vStart.x;
				float ay = vC2.y - shadow.m_vStart.y;
				float az = vC2.z - shadow.m_vStart.z;
				float mag;

				shadow.m_Planes[3].m_Normal.x =
				        shadow.m_vUp.y * az - shadow.m_vUp.z * ay;
				shadow.m_Planes[3].m_Normal.y =
				        shadow.m_vUp.z * ax - shadow.m_vUp.x * az;
				shadow.m_Planes[3].m_Normal.z =
				        shadow.m_vUp.x * ay - shadow.m_vUp.y * ax;

				mag = (float)sqrt(
				        shadow.m_Planes[3].m_Normal.x * shadow.m_Planes[3].m_Normal.x +
				        shadow.m_Planes[3].m_Normal.y * shadow.m_Planes[3].m_Normal.y +
				        shadow.m_Planes[3].m_Normal.z * shadow.m_Planes[3].m_Normal.z);

				if (mag != 0.0f)
				{
					float inv = 1.0f / mag;

					shadow.m_Planes[3].m_Normal.x = shadow.m_Planes[3].m_Normal.x * inv;
					shadow.m_Planes[3].m_Normal.y = shadow.m_Planes[3].m_Normal.y * inv;
					shadow.m_Planes[3].m_Normal.z = shadow.m_Planes[3].m_Normal.z * inv;
				}

				shadow.m_Planes[3].m_Dist =
				        shadow.m_Planes[3].m_Normal.x * shadow.m_vStart.x +
				        shadow.m_Planes[3].m_Normal.y * shadow.m_vStart.y +
				        shadow.m_Planes[3].m_Normal.z * shadow.m_vStart.z;
			}

			// plane 5 -- up x (vC3 - start), then normalise.
			{
				float ax = vC3.x - shadow.m_vStart.x;
				float ay = vC3.y - shadow.m_vStart.y;
				float az = vC3.z - shadow.m_vStart.z;
				float mag;

				shadow.m_Planes[5].m_Normal.x =
				        ay * shadow.m_vUp.z - az * shadow.m_vUp.y;
				shadow.m_Planes[5].m_Normal.y =
				        az * shadow.m_vUp.x - ax * shadow.m_vUp.z;
				shadow.m_Planes[5].m_Normal.z =
				        ax * shadow.m_vUp.y - ay * shadow.m_vUp.x;

				mag = (float)sqrt(
				        shadow.m_Planes[5].m_Normal.x * shadow.m_Planes[5].m_Normal.x +
				        shadow.m_Planes[5].m_Normal.y * shadow.m_Planes[5].m_Normal.y +
				        shadow.m_Planes[5].m_Normal.z * shadow.m_Planes[5].m_Normal.z);

				if (mag != 0.0f)
				{
					float inv = 1.0f / mag;

					shadow.m_Planes[5].m_Normal.x = shadow.m_Planes[5].m_Normal.x * inv;
					shadow.m_Planes[5].m_Normal.y = shadow.m_Planes[5].m_Normal.y * inv;
					shadow.m_Planes[5].m_Normal.z = shadow.m_Planes[5].m_Normal.z * inv;
				}

				shadow.m_Planes[5].m_Dist =
				        shadow.m_Planes[5].m_Normal.x * shadow.m_vStart.x +
				        shadow.m_Planes[5].m_Normal.y * shadow.m_vStart.y +
				        shadow.m_Planes[5].m_Normal.z * shadow.m_vStart.z;
			}

			// --------------------------------------------------------- //
			//  0x10048f10..0x10049148 -- the shadow projection.
			// --------------------------------------------------------- //
			MakeShadowMatrix(&mShadow, shadow.m_vStart, shadow.m_vDir,
			                 shadow.m_fDist);

			// --------------------------------------------------------- //
			//  0x10049148..0x100492fb -- the two constant matrices.
			//
			//  mBasis rows 0 and 1 are the up and right vectors with the
			//  projection of the kept corner as the translation; rows 2 and
			//  3 are left as identity.  The two dot products are computed
			//  z, y, x and negated (`fchs`), and the results land at
			//  mBasis[0][3] (-0x27c) and mBasis[1][3] (-0x26c).
			//
			//  mScale is a pure diagonal: 1.0f/(2.0f*m_fHalfH) at [0][0]
			//  (`flds 2.0f ; fmuls -0x4c0 ; fdivrs 1.0f`) and
			//  1.0f/(2.0f*m_fHalfW) at [1][1] (the same shape on -0x4bc).
			//  Note that these two DO read the struct copies and not the
			//  cached fE0/fE4, which is why both spellings are in the file.
			//
			//  All thirty-two elements are written, the constant ones with
			//  `mov %eax,` (a zero register) and `mov %edx,` (0x3f800000),
			//  so the source assigns every element rather than calling
			//  Identity() first.
			// --------------------------------------------------------- //
			mBasis.m[0][0] = shadow.m_vUp.x;
			mBasis.m[0][1] = shadow.m_vUp.y;
			mBasis.m[0][2] = shadow.m_vUp.z;
			mBasis.m[0][3] = -(shadow.m_vUp.x * shadow.m_vCorner.x +
			                   shadow.m_vUp.y * shadow.m_vCorner.y +
			                   shadow.m_vUp.z * shadow.m_vCorner.z);

			mBasis.m[1][0] = shadow.m_vRight.x;
			mBasis.m[1][1] = shadow.m_vRight.y;
			mBasis.m[1][2] = shadow.m_vRight.z;
			mBasis.m[1][3] = -(shadow.m_vRight.x * shadow.m_vCorner.x +
			                   shadow.m_vRight.y * shadow.m_vCorner.y +
			                   shadow.m_vRight.z * shadow.m_vCorner.z);

			mBasis.m[2][0] = 0.0f;
			mBasis.m[2][1] = 0.0f;
			mBasis.m[2][2] = 1.0f;
			mBasis.m[2][3] = 0.0f;

			mBasis.m[3][0] = 0.0f;
			mBasis.m[3][1] = 0.0f;
			mBasis.m[3][2] = 0.0f;
			mBasis.m[3][3] = 1.0f;

			mScale.m[0][0] = 1.0f / (2.0f * shadow.m_fHalfH);
			mScale.m[0][1] = 0.0f;
			mScale.m[0][2] = 0.0f;
			mScale.m[0][3] = 0.0f;

			mScale.m[1][0] = 0.0f;
			mScale.m[1][1] = 1.0f / (2.0f * shadow.m_fHalfW);
			mScale.m[1][2] = 0.0f;
			mScale.m[1][3] = 0.0f;

			mScale.m[2][0] = 0.0f;
			mScale.m[2][1] = 0.0f;
			mScale.m[2][2] = 1.0f;
			mScale.m[2][3] = 0.0f;

			mScale.m[3][0] = 0.0f;
			mScale.m[3][1] = 0.0f;
			mScale.m[3][2] = 0.0f;
			mScale.m[3][3] = 1.0f;

			// --------------------------------------------------------- //
			//  0x100492ac..0x10049a82 -- the two inlined multiplies and the
			//  sixteen-dword `rep movsl` into the block.  The operand order
			//  is pinned by which factor's row index varies: mTemp[0][0]
			//  accumulates mScale[0][k]*mBasis[k][0], and mFinal[0][0]
			//  accumulates mTemp[0][k]*mShadow[k][0].
			// --------------------------------------------------------- //
			MatMul4(&mTemp, &mScale, &mBasis);
			MatMul4(&mFinal, &mTemp, &mShadow);

			shadow.m_mTex = mFinal;

			// --------------------------------------------------------- //
			//  0x10049a84..0x10049acc -- the draw loop.  The count is a
			//  fresh load each turn (`mov -0x21ec(%ebp,%edi,1),%eax` sits
			//  INSIDE the loop, after the call), so the condition names the
			//  member and not a cached copy.
			// --------------------------------------------------------- //
			for (j = 0; j < found[i].m_nCount; j++)
				sub_10049bb0((Shadow49bb0*)&shadow, (Item49bb0*)found[i].m_pItems[j]);
		}

		// ------------------------------------------------------------- //
		//  0x10049afc..0x10049b45 -- put the device back.  Two separate
		//  reads of 0x1008d988, one per statement.
		// ------------------------------------------------------------- //
		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 0, pOldTexture);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		                                     stateSave.m_nState00,
		                                     stateSave.m_nOld04);

		// 0x10049b46 -- the timing report.  The fourth and fifth arguments
		// are `mov %edx,` of a materialised zero and the first three are
		// memory loads of the three counters; see the COIN FLIP note.
		if (g_p1008ca38)
		{
			g_pStruct->ConsolePrint(g_sz100a9f70,
			                        nFindTime, nBSPTime, nSetupTime, 0, 0);
		}
	}
}


// ======================================================================= //
//  COIN FLIPS -- three, each with the exact alternative spelling.
//
//  1. THE CLAMP AT 0x100481a1.  Written above as
//         int32 nNumDirs = LTMIN(g_n100893ec, 8);
//         if (nNumDirs <= 0) return;
//     The shipped bytes have MORE than that: three branches off the one
//     `cmp $0x8,%eax` (`jge` into the body with %ebx preset to 8, then `jle`
//     into a block sunk to 0x10049b96) with a `mov $0x8,%ebx` at 0x100481ae
//     that has no reachable predecessor -- the "deleted arm of an
//     unsatisfiable guard" -- and the sunk block splits the zero test into
//     `test %eax,%eax ; jl ret ; je ret`.  The literal reading is
//
//         int32 nNumDirs = LTMIN(g_n100893ec, 8);
//         if (g_n100893ec > 8)
//             nNumDirs = 8;
//         if (nNumDirs < 0)
//             return;
//         if (nNumDirs == 0)
//             return;
//
//     -- a second, redundant clamp (which is what leaves the dead arm) and a
//     `<= 0` written as two comparisons (which is what leaves two branches
//     off one `test`).  Both alternatives give the same call sequence; the
//     second is the one to try if the block graph is being scored.
//
//  2. fLength (-0x168).  Written above as a `float` local inside each loop.
//     The shipped slot is a DOUBLE (`fstpl -0x168` once, then `fldl -0x168 ;
//     fstps 0x4(%esp)` at each of the two uses), which is either a `double`
//     variable or the expression `(fDC + fD8)` written out at both use sites
//     and common-subexpression-eliminated by icl.  The alternative is
//
//         double fLength = fDC + fD8;      /* hoisted above both loops */
//
//  3. THE LAST TWO PRINTF ARGUMENTS.  Five values are pushed; the first
//     three come from the three counters in memory and the last two from a
//     materialised zero register.  Written above as literal `0, 0`.  The
//     alternative is two further counters, `nDrawTime` and `nTotalTime`,
//     that icl constant-propagated because nothing in the function ever
//     writes them -- which would also explain why the format string names
//     five phases and only three have slots.  Nothing in the bytes decides
//     between the two.
// ======================================================================= //




// ======================================================================= //
//  TYPES.  Every offset below is a load or a store in the shipped bytes
//  and is cited by address.
// ======================================================================= //

// The vector.  Three of its members are reached from this function and all
// three are DECLARED AND NOT DEFINED on purpose: written as arithmetic they
// emit no call at all, and three of this function's ten direct calls are
// these (a tiny accessor spelled as a field read makes no
// call).  The names carry the shipped addresses because relocations are
// masked and the address is the only identity a nameless helper has.
struct Vec49bb0
{
	// 0x10015690 -- operator*(float).  Site 0x10049c6e: `add $-0x8,%esp`
	// = hidden return pointer at (%esp) plus one float at 0x4(%esp), and
	// `mov %eax,%ecx` uses 0x1004b630's answer as `this`.
	Vec49bb0	sub_10015690(float f) const;            // 0x10015690
	// 0x1004b640 -- operator+=.  Site 0x10049c8b: `add $-0xc,%esp` then
	// `rep movsl %ecx=3` of the twelve-byte operand, no hidden return,
	// `this` = &aVerts[i].m_vPos out of the -0xa0 slot.
	void		sub_1004b640(const Vec49bb0 v);         // 0x1004b640

	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

union VColor49bb0
{
	uint32	m_n;
	uint8	m_b[4];
};

// ---------------------------------------------------------------------- //
//  THE WORKING VERTEX, 0x24 bytes.  The stride is `add $0x24` as a byte
//  offset at 0x10049c9c and 0x10049df2, as a pointer at 0x10049e2a,
//  0x10049eb9, 0x1004b3cb and 0x1004b47e, and once per clip pass; the
//  ` x36` form `add %eax,%eax ; add %eax,%eax ; lea (%eax,%eax,8)` appears
//  seven times (0x10049d2a, then once per pass).  `mov $0x9 ; rep movsl`
//  at 0x10049f4e and five more places copies the record whole (9 dwords =
//  0x24), and `mov $0x24,%ecx ; ... idiv %ecx` at 0x1004a1ec and five more
//  divides a pointer difference by it.
//
//  It is Vert4e3e4 exactly: THREE interpolated floats (+0x18/+0x1c/+0x20)
//  and FIVE interpolated bytes (+0x10/+0x11/+0x12/+0x13/+0x17), which is
//  what the census's `fld`-into-`fistp` runs count.
//
//  +0x0c is READ at 0x1004b413 and never written by this function; the
//  0x1004b5c0 loop at 0x1004b3c6 is what fills it (`fsts 0xc(%eax)` at
//  0x1004b5e1, the 1.0f/z from 0x100a9f5c -- which is what bounds this
//  function's own literal run above at 0x100a9f54: 0x100a9f58 is that
//  division's dead 1.0f partner and 0x100a9f5c is its live one).
// ---------------------------------------------------------------------- //
struct Vert49bb0                                        // 0x24 bytes
{
	Vec49bb0	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	VColor49bb0	m_Diffuse10;                            // +0x10
	VColor49bb0	m_Specular14;                           // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
	float		m_fQ;                                   // +0x20
};

// ---------------------------------------------------------------------- //
//  THE D3D VERTEX, 0x20 bytes.  FVF 0x1c4 = D3DFVF_XYZRHW | DIFFUSE |
//  SPECULAR | TEX1 (the literal at 0x1004b4b0) and 0x20 is the stride
//  `lea 0x20(%edx),%eax` at 0x1004b464.  The copy loop at 0x1004b3ee
//  writes +0x00..+0x0b by `rep movsl %ecx=3`, then +0x0c, +0x10 and +0x14
//  as integer moves (0x1004b423, 0x1004b43c, 0x1004b44f) and +0x18, +0x1c
//  as the two perspective-divided texture coordinates (0x1004b46d,
//  0x1004b481).
// ---------------------------------------------------------------------- //
struct D3DVert49bb0                                     // 0x20 bytes
{
	Vec49bb0	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	VColor49bb0	m_Diffuse;                              // +0x10
	VColor49bb0	m_Specular;                             // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
};

// ---------------------------------------------------------------------- //
//  THE SOURCE VERTEX.  Stride 0x18 (`add $0x18,%ebx` at 0x10049ca6) and
//  +0x00 is a POINTER: 0x10049c3b does `mov (%edx,%ebx,1),%esi` and then
//  `rep movsl %ecx=3` out of it.  Nothing else in the record is touched
//  here, so +0x04..+0x17 is named by its size only.  Identical to
//  big4e3e4.cpp's SrcVert4e3e4 and big6d910.cpp's SrcVert6d910.
// ---------------------------------------------------------------------- //
struct SrcVert49bb0                                     // 0x18 bytes
{
	Vec49bb0*	m_pPos00;                               // +0x00
	uint8		m_pad_04[0x14];                         // +0x04 .. +0x17
};

// ---------------------------------------------------------------------- //
//  0x1004b630 -- d3dbatchc.cpp, `mov 0x28(%ecx),%ecx ; mov 0x34(%ecx),
//  %eax`, three callers.  big4b720.cpp names the two hops
//  ShadowItem::m_pPoly28->m_pPlane34; its answer is used HERE as the
//  `this` of operator* (0x10049c6c `mov %eax,%ecx`), so what it returns is
//  the address of the plane's normal.  DECLARED AND NOT DEFINED: written
//  as `pItem->m_pPoly28->m_pPlane34` it would emit no call.
// ---------------------------------------------------------------------- //
struct Item49bb0
{
	Vec49bb0*	sub_1004b630();                         // 0x1004b630

	uint8		m_pad_00[0x28];                         // +0x00 .. +0x27
	void*		m_pPoly28;                              // +0x28
};

// The plane, 0x10 bytes -- d3dr3a.cpp's SPlaneA.  Only the array base and
// the stride matter here: 0x10049cd5 passes pShadow+0xa0 and 0x10047cf0
// walks it with `add $0x10,%ebp`.
struct Plane49bb0
{
	float	m_00, m_04, m_08, m_0c;
};

// ---------------------------------------------------------------------- //
//  THE SHADOW BLOCK, 0x100 bytes.  big48180.cpp built this at -0x518(%ebp)
//  and handed its address over as the first argument; it is that file's
//  S10048180Shadow, carried over field for field so the two agree.  This
//  function touches exactly TWO of its members:
//
//    +0x60  m_mTex     rows 0, 1 and 3 at 0x10049d7e..0x10049ddd:
//                      +0x60/+0x64/+0x68/+0x6c, +0x70/+0x74/+0x78/+0x7c
//                      and +0x90/+0x94/+0x98/+0x9c.  ROW 2 (+0x80..+0x8c)
//                      IS NEVER READ, exactly as in 0x1004e3e4.
//    +0xa0  m_Planes   0x10049cd5 `lea 0xa0(%ebx),%edx`, with the literal
//                      6 at 0x10049cdb.
// ---------------------------------------------------------------------- //
struct Shadow49bb0
{
	Vec49bb0	m_vStart;                               // +0x00
	Vec49bb0	m_vUp;                                  // +0x0c
	Vec49bb0	m_vRight;                               // +0x18
	Vec49bb0	m_vNormal;                              // +0x24
	Vec49bb0	m_vDir;                                 // +0x30
	float		m_fDist;                                // +0x3c
	Vec49bb0	m_vEnd;                                 // +0x40
	Vec49bb0	m_vCorner;                              // +0x4c
	float		m_fHalfH;                               // +0x58
	float		m_fHalfW;                               // +0x5c
	float		m_mTex[4][4];                           // +0x60
	Plane49bb0	m_Planes[6];                            // +0xa0 .. +0xff
};

// ---------------------------------------------------------------------- //
//  The device (0x1008d988), as d3dr3a.cpp and big4e3e4.cpp type it.  Slot
//  0x64 is the only one called: `mov (%edx),%edx ; call *0x64(%edx)` at
//  0x1004b4c4 with six outgoing dwords, which is
//  IDirect3DDevice3::DrawPrimitive(this, type, fvf, verts, count, flags).
//  A COM method, so __stdcall: it pops its own six arguments, and the
//  shipped call (0x1004b4c6) is followed by no stack adjustment. Declared
//  without it, the caller popped them again: its saved registers came back
//  from the wrong slots, which crashed the shadow pass.
// ---------------------------------------------------------------------- //
struct D3DDevice49bb0;

struct D3DDeviceVtbl49bb0
{
	uint8	m_pad_00[0x64];                             // +0x00 .. +0x63
	int32	(__stdcall *DrawPrimitive)(D3DDevice49bb0* pThis, uint32 nType,
	                         uint32 nFVF, void* pVerts, uint32 nCount,
	                         uint32 nFlags);            // +0x64
};

struct D3DDevice49bb0
{
	D3DDeviceVtbl49bb0*	m_pVtbl;                        // +0x00
};

// ---------------------------------------------------------------------- //
//  `this`.  NOT ONE FIELD OF IT IS TOUCHED.  The caller loads %ecx at
//  0x10049aaf (`mov -0x174(%ebp),%ecx`, its own saved `this`); the
//  register entry does not spill %ecx and every read of %ecx in the body
//  follows a write (the first is `mov $0x3,%ecx` at 0x10049c44).  It stays
//  a member function because that is what the call site's %ecx says it is,
//  and because big4b720.cpp and big48180.cpp already declare 0x10048180
//  and 0x1004b720 as members of one class.  big4b720.cpp calls the class
//  CShadowRenderer and big48180.cpp calls it C10048180; the former name is
//  used here so this file agrees with big4e3e4.cpp's sibling.
// ---------------------------------------------------------------------- //
// (the class is big48180's C10048180, declared above)


// ======================================================================= //
//  GLOBALS, by address.  Each is named for its address because none of
//  them has an initialiser in the image -- they all live in the
//  uninitialised .data1/_DATA sections -- so no VALUE can be quoted for
//  any of them, only the address and the width of the access.
// ======================================================================= //

// 0x1008b874 -- the clip mask.  `movl $0x3f,0x1008b874` at 0x10049e10 and
// read back at 0x10049e33; 0x1000a770 (reached through 0x1000d9b0) is what
// clears bits in it.  Same global big4e3e4.cpp, big6d910.cpp and
// big124a4.cpp use.
extern uint32			g_n1008b874;                    // 0x1008b874

// 0x1008b87c -- the clip scratch cursor.  d3dr3a.cpp declares it
// `SClipVertA* g_1008b87c`; 0x10047cf0 reads it into its own local at
// 0x10047d07 and this function reads it ONCE, at 0x10049e51, storing the
// same value into both pOut and pOutStart (0x10049e5c and 0x10049e62).
extern Vert49bb0*		g_p1008b87c;                    // 0x1008b87c

// 0x1008b880 -- the transform object handed to both transform helpers.
// Materialised as `mov $0x1008b880,%edi` at 0x10049e1e and 0x1004b3bf --
// hoisted out of each loop into %edi, once per loop.
extern uint8			g_1008b880[];                   // 0x1008b880

// The near and far clip constants.  Compared at 0x10049eac (`fcomps` then
// `jae`) and 0x1004b08c (`fcomps` then `jbe`), and stored AS INTEGER MOVES
// into the crossing vertex at 0x10049fc9 and 0x1004b19a.
extern float			g_f1008b908;                    // 0x1008b908
extern float			g_f1008b910;                    // 0x1008b910

// 0x1008be44 -- the clipped-polygon counter, `incl` once per plane pass at
// 0x10049e84, 0x1004a21a, 0x1004a5c5, 0x1004a968, 0x1004acea, 0x1004b068
// (six of the function's fifteen `inc`s).
extern uint32			g_n1008be44;                    // 0x1008be44

// 0x1008d7a0 -- UseD3DClip.  Tested at 0x10049e3b with a 32-bit `mov` and
// `test %edx,%edx`; big6d910.cpp types it `void*` and reads it in exactly
// the same place for exactly the same `nClip &= 1` narrowing.
extern void*			g_p1008d7a0;                    // 0x1008d7a0

// 0x1008d988 -- the device.
extern D3DDevice49bb0*	g_p1008d988;                    // 0x1008d988

// 0x1008ee84 -- the distance the shadow polygon is lifted off its own
// plane.  Loaded as an INTEGER at 0x10049c5f (`mov 0x1008ee84,%ecx`) and
// written to 0x4(%esp) as operator*'s float argument, which is what a
// float lvalue passed by value looks like.  big4e3e4.cpp declares the same
// address the same way.
//
// It is the current value of the console variable `ModelShadowOffset`:
// 0x1004b670 constructs `CConVar g_1008ee80` with the name at 0x100a9fd0
// and the default 0x3d4ccccd = 0.05f, and 0x1008ee84 is that object + 4.
// 0x10021f40 (the CConVar constructor) zeroes +0x00 and +0x04, parks the
// default float at +0x08, the name at +0x14, and links the object into the
// list at 0x1008bf90 through +0x1c -- so +0x04 is the CURRENT float value,
// written by the console and not by the constructor.
extern float			g_f1008ee84;                    // 0x1008ee84

// 0x1008eea0 -- the shadow alpha, read as ONE BYTE at 0x10049d70
// (`mov 0x1008eea0,%dl`) and stored straight into the vertex's +0x13.
// This is `CConVar g_1008eea0` ("ModelShadowAlpha", default 230.0f, built
// at 0x1004b676) at OFFSET ZERO -- i.e. the +0x00 slot the constructor
// zeroes, which by the same reading as 0x1008ee84 above is the console
// variable's current INTEGER value.  Declared here as a `uint8` because
// that is the width of the access; a `uint32`/`int32` global read with a
// `(uint8)` cast is indistinguishable (width-reduction of an
// already-partial load -- BOTH compilers do this), and nothing in these
// bytes chooses between the two.
extern uint8			g_b1008eea0;                    // 0x1008eea0


// ======================================================================= //
//  NEIGHBOURS.  Each prototype is read off the call site in these bytes.
// ======================================================================= //

// 0x1000a700 -- d3dbatchd.cpp.  (geometry, &data, &count); the three
// outgoing slots at 0x10049bd7..0x10049bde are pItem, &pData, &nVerts, and
// `add $0xc,%esp` at 0x10049be7 cleans three dwords.  big572b4.cpp and
// big4e3e4.cpp declare it identically.
extern "C" void  sub_1000a700(void* pGeom, SrcVert49bb0** ppOutData,
                              int32* pnOutCount);

// 0x1000a740 -- d3dbatchd.cpp, the 128-vertex guard, `cmpl $0x80,arg ;
// jg`, SIGNED.  ONE dword, and it is the only argument in this function
// that is pushed rather than written into a reserved outgoing slot
// (0x10049bf0 `push %eax`, cleaned by `add $0x8,%esp` at 0x10049bf6 --
// the extra dword is icl's reserved slot from the previous call).
extern "C" int32 sub_1000a740(int32 nVerts);

// 0x10047cf0 -- d3dr3a.cpp, THE GENERAL-PLANE DRIVER.  Its own body reads
// arg1 into %edi and dereferences it as the vertex pointer, arg2 into %esi
// as the count, arg3 into %ebp as a 0x10-stride plane array and arg4 into
// %ebx as the countdown -- so the four outgoing slots at
// 0x10049cdb..0x10049cf1 are (&pVerts, &nVerts, m_Planes, 6).  Two
// callers, 0x10049cf4 (this one) and 0x1004e504.
extern "C" int32 sub_10047cf0(Vert49bb0** ppVerts, int32* pnVerts,
                              Plane49bb0* pPlanes, int32 nPlanes);

// 0x1000d9b0 -- d3dlow.cpp, __cdecl (vertex, transform object).  Forwards
// to 0x1000a770 with `pXForm + 0x15c`.  Two pushes at 0x10049e23/0x10049e24
// in the order (pXForm, pVert), cleaned by `add $0x8,%esp`.
extern "C" void  sub_1000d9b0(Vert49bb0* pVert, uint8* pXForm);

// 0x1004b5c0 -- d3dr2*.cpp's screen projection, __cdecl (vertex, transform
// object).  `flds 0x100a9f5c (1.0f) ; fdivs 0x8(%eax) ; ... ; fsts
// 0xc(%eax)` writes the vertex's +0x0c and then scales x, y and z through
// the object's +0x31c..+0x330.  Pushed in the same (pXForm, pVert) order
// at 0x1004b3c4/0x1004b3c5.
extern "C" void  sub_1004b5c0(Vert49bb0* pVert, uint8* pXForm);

// 0x100805d0 -- the CRT memcpy.  Four outgoing slots' worth of setup at
// 0x10049d14..0x10049d31 and `add $0x10,%esp` after; the size is
// `nVerts * 36` built with `add %eax,%eax ; add %eax,%eax ;
// lea (%eax,%eax,8),%eax`.  d3dblit.cpp declares it this way.
extern void* memcpy(void* pDst, const void* pSrc, uint32 nBytes);


// ======================================================================= //
//  THE CLIP FLAG BOUND.  READ OFF THE FRAME, not assumed: the six passes'
//  flag arrays sit at -0x15ec, -0x1510, -0x1434, -0x1358, -0x127c and
//  -0x11a0 (the six `lea` sites 0x10049e91, 0x1004a227, 0x1004a5d2,
//  0x1004a975, 0x1004acf7, 0x1004b077), i.e. 0xdc apart, and pass 1's five
//  `fistpl` destinations are -0x1524, -0x1520, -0x151c, -0x1518, -0x1514
//  (0x1004a03e and four more).  -0x15ec + 0xc8 = -0x1524 exactly, so the
//  array is 0xc8 = 200 bytes = FIFTY int32 and the remaining 0x14 of the
//  0xdc block is those five temps.  The same fifty d3dr3a.cpp,
//  big6d910.cpp and big4e3e4.cpp all read off their own frames.
// ======================================================================= //
#define CLIP49BB0_MAX	50


// ---------------------------------------------------------------------- //
//  THE PER-CROSSING INTERPOLATION, as a macro so that no call can be
//  emitted for it -- there is no `call` anywhere between 0x10049e2a and
//  0x1004b3c5.  THREE floats and FIVE bytes, in this order, read off pass
//  1 at 0x10049fd2..0x1004a1a3 and diffed against the other five:
//
//    +0x18  0x10049fe1   +0x1c  0x10049ff3   +0x20  0x1004a005
//    +0x12  0x1004a056   +0x11  0x1004a0a7   +0x10  0x1004a0f8
//    +0x13  0x1004a149   +0x17  0x1004a19b
//
//  (The six passes' byte-store sites are `mov %dl,0xNN(%eax)` and the
//  census counts SIX of each of +0x10, +0x11, +0x12, +0x13 and +0x17 --
//  thirty stores, five bytes and not four and not six.)
//
//  The byte differences are taken in the INTEGER unit (`movzbl ; movzbl ;
//  sub` at 0x1004a008) through ONE shared int32 slot at -0xc0(%ebp), which
//  is written 60 times and `fildl`-ed 60 times and is the whole of that
//  slot's traffic.  The result goes out through a named float slot and a
//  bare `fistpl` (`fstps -0x28f0(%ebp) ; flds -0x28f0(%ebp) ; fistpl
//  -0x1524(%ebp)` at 0x1004a032), which is /QIfist -- there is NO
//  fnstcw/fldcw dance anywhere in this function's 6,668 bytes (census:
//  fnstcw 0, fldcw 0), unlike 0x1004e3e4 which has exactly one in its
//  region D.  Under plain /O2 these thirty sites will differ in bytes, and
//  they contain no calls.
// ---------------------------------------------------------------------- //
// Round to nearest in the current FPU mode: w1_10001000.cpp's helper
// (0x10015190 out of line).  Inlined, it is the shipped `fstps t ;
// flds t ; fistpl n`.
// Measured: at this file's /O2 /Oi- 8808 -> 6520 bytes
// (shipped 6664), 8232 -> 5937 differing.  Still not paired: the shipped
// function is a /Qip pair (stack entry 0x10049bb0) called at 0x10049abc
// from inside 0x10048180 (big48180.cpp), and its prologue is the /Os one
// (`push %edi ; push %edi ; push %esi ; push %ebx`): at /Os /Oi- /Qip it
// opens as shipped but with a 0x2918 frame against 0x2910 (one extra
// 8-byte spill, -0x8: the vertex loop's counter).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

#define CLIP49BB0_LERP                                                       \
	pOut->m_fU = (pWalk->m_fU - pPrev->m_fU) * fT + pPrev->m_fU;             \
	pOut->m_fV = (pWalk->m_fV - pPrev->m_fV) * fT + pPrev->m_fV;             \
	pOut->m_fQ = (pWalk->m_fQ - pPrev->m_fQ) * fT + pPrev->m_fQ;             \
                                                                             \
	fRed   = (pWalk->m_Diffuse10.m_b[2] - pPrev->m_Diffuse10.m_b[2]) * fT    \
	       + pPrev->m_Diffuse10.m_b[2];                                      \
	pOut->m_Diffuse10.m_b[2] = (uint8)RoundF(fRed);                                  \
	fGreen = (pWalk->m_Diffuse10.m_b[1] - pPrev->m_Diffuse10.m_b[1]) * fT    \
	       + pPrev->m_Diffuse10.m_b[1];                                      \
	pOut->m_Diffuse10.m_b[1] = (uint8)RoundF(fGreen);                                \
	fBlue  = (pWalk->m_Diffuse10.m_b[0] - pPrev->m_Diffuse10.m_b[0]) * fT    \
	       + pPrev->m_Diffuse10.m_b[0];                                      \
	pOut->m_Diffuse10.m_b[0] = (uint8)RoundF(fBlue);                                 \
	fAlpha = (pWalk->m_Diffuse10.m_b[3] - pPrev->m_Diffuse10.m_b[3]) * fT    \
	       + pPrev->m_Diffuse10.m_b[3];                                      \
	pOut->m_Diffuse10.m_b[3] = (uint8)RoundF(fAlpha);                                \
	fSpec  = (pWalk->m_Specular14.m_b[3] - pPrev->m_Specular14.m_b[3]) * fT  \
	       + pPrev->m_Specular14.m_b[3];                                     \
	pOut->m_Specular14.m_b[3] = (uint8)RoundF(fSpec);


// ---------------------------------------------------------------------- //
//  THE CLIPPER: the clip driver and its six plane bodies
//  -- big051a0.cpp's (0x100051a0 over 0x10002310 .. 0x10003410) on this
//  36-byte vertex -- INLINED, as 0x10049bb4 has them.  There is no
//  out-of-line 36-byte copy in the image, so the plane bodies are named
//  for the 40-byte ones they are written after.
// ---------------------------------------------------------------------- //
#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

static inline float ClipEdgeNear36(Vec49bb0* a, Vec49bb0* b, Vec49bb0* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_f1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b908;

	return t;
}


static inline float ClipEdgeLeft36(Vec49bb0* a, Vec49bb0* b, Vec49bb0* dst)
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


static inline float ClipEdgeTop36(Vec49bb0* a, Vec49bb0* b, Vec49bb0* dst)
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


static inline float ClipEdgeFar36(Vec49bb0* a, Vec49bb0* b, Vec49bb0* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_f1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b910;

	return t;
}


static inline float ClipEdgeRight36(Vec49bb0* a, Vec49bb0* b, Vec49bb0* dst)
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


static inline float ClipEdgeBottom36(Vec49bb0* a, Vec49bb0* b, Vec49bb0* dst)
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


static inline void LerpVert49bb0(Vert49bb0* a, Vert49bb0* b, Vert49bb0* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	dst->m_fQ = (b->m_fQ - a->m_fQ) * t + a->m_fQ;
	dst->m_Diffuse10.m_b[2] = (uint8)RoundF((b->m_Diffuse10.m_b[2] - a->m_Diffuse10.m_b[2]) * t + a->m_Diffuse10.m_b[2]);
	dst->m_Diffuse10.m_b[1] = (uint8)RoundF((b->m_Diffuse10.m_b[1] - a->m_Diffuse10.m_b[1]) * t + a->m_Diffuse10.m_b[1]);
	dst->m_Diffuse10.m_b[0] = (uint8)RoundF((b->m_Diffuse10.m_b[0] - a->m_Diffuse10.m_b[0]) * t + a->m_Diffuse10.m_b[0]);
	dst->m_Diffuse10.m_b[3] = (uint8)RoundF((b->m_Diffuse10.m_b[3] - a->m_Diffuse10.m_b[3]) * t + a->m_Diffuse10.m_b[3]);
	dst->m_Specular14.m_b[3] = (uint8)RoundF((b->m_Specular14.m_b[3] - a->m_Specular14.m_b[3]) * t + a->m_Specular14.m_b[3]);
}


// Each plane body opens `pV = &(*ppVerts)[0];` rather than
// `pV = *ppVerts;` -- w23_100525b0_os.cpp's finding: it keeps the
// inlined driver's pVerts a memory slot, as the shipped 0x10049bb4 has it
// (normalised instruction diff 249 -> 239; 0x10048180 unchanged).
static inline LTBOOL Plane36_10002310(uint32* pUnused, Vert49bb0** ppVerts, int32* pnVerts, Vert49bb0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert49bb0*  pOutStart;
	Vert49bb0*  pV;
	Vert49bb0*  pEnd;
	Vert49bb0*  pPrev;
	Vert49bb0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.z >= g_f1008b908);

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
				float t = ClipEdgeNear36(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert49bb0(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL Plane36_10002670(uint32* pUnused, Vert49bb0** ppVerts, int32* pnVerts, Vert49bb0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert49bb0*  pOutStart;
	Vert49bb0*  pV;
	Vert49bb0*  pEnd;
	Vert49bb0*  pPrev;
	Vert49bb0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->m_vPos.z < pV->m_vPos.x);

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
				float t = ClipEdgeLeft36(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert49bb0(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL Plane36_100029e0(uint32* pUnused, Vert49bb0** ppVerts, int32* pnVerts, Vert49bb0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert49bb0*  pOutStart;
	Vert49bb0*  pV;
	Vert49bb0*  pEnd;
	Vert49bb0*  pPrev;
	Vert49bb0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.y < pV->m_vPos.z);

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
				float t = ClipEdgeTop36(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert49bb0(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL Plane36_10002d40(uint32* pUnused, Vert49bb0** ppVerts, int32* pnVerts, Vert49bb0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert49bb0*  pOutStart;
	Vert49bb0*  pV;
	Vert49bb0*  pEnd;
	Vert49bb0*  pPrev;
	Vert49bb0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.x < pV->m_vPos.z);

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
				float t = ClipEdgeRight36(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert49bb0(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL Plane36_100030a0(uint32* pUnused, Vert49bb0** ppVerts, int32* pnVerts, Vert49bb0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert49bb0*  pOutStart;
	Vert49bb0*  pV;
	Vert49bb0*  pEnd;
	Vert49bb0*  pPrev;
	Vert49bb0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->m_vPos.z < pV->m_vPos.y);

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
				float t = ClipEdgeBottom36(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert49bb0(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL Plane36_10003410(uint32* pUnused, Vert49bb0** ppVerts, int32* pnVerts, Vert49bb0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert49bb0*  pOutStart;
	Vert49bb0*  pV;
	Vert49bb0*  pEnd;
	Vert49bb0*  pPrev;
	Vert49bb0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.z <= g_f1008b910);

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
				float t = ClipEdgeFar36(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert49bb0(pPrev, pCur, *ppOut, t);

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


// ======================================================================= //
//  THE CLIPPER, as the shipped code is built: 0x100051a0 is
//  the 40-byte-vertex twin of d3dr3c_os.cpp's 0x1000f4b0 -- the same
//  UseD3DClip test, the same output bank, the same six planes in the same
//  order -- with the six plane bodies 0x10002310 .. 0x10003410 (byte-
//  identical out of line in w1_10002310.cpp / w44_10002d40.cpp) INLINED.
//  The plane bodies take &pVerts / &nVerts / &pOut, so those three keep
//  frame slots and are stored and reloaded around every plane -- which is
//  the shipped `mov -0xd0(%ebp),%eax ; ... ; mov %eax,-0xd0(%ebp)` round
//  trip of pOut in every vertex copy, and the RoundF temporaries give the
//  thirty `fstps t ; flds t ; fistpl n` colour conversions.  The earlier
//  macro-expanded passes (the plane bodies written out in place) are in
//  git history; they kept pOut in a register.
//
//  The driver is itself an INLINE body under the exported function: the
//  shipped code has ONE epilogue (`mov %edx,%eax` at 0x100066f4) that
//  every exit reaches with the answer in %edx -- `mov $0x1,%edx ; jmp`
//  for the UseD3DClip early-out, `xor %edx,%edx ; jmp` for each plane's
//  reject -- which is how icl lowers an inlined callee's several
//  `return`s.  Written with the driver's returns in the exported function
//  itself, /O2 gives seven epilogues (0.966 matched); with a result
//  variable and gotos, the variable goes to the frame (0.980); as an
//  inline driver: 1509 of 1523 instructions matched, same count (0.991).
// ======================================================================= //
static inline LTBOOL ClipPoly49bb0(uint32 nClipFlags, Vert49bb0** ppVerts,
                                     int32* pnVerts)
{
	Vert49bb0*  pOut;
	Vert49bb0*  pVerts;
	int32       nVerts;
	uint32      anUnused[6];

	if (g_p1008d7a0)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut   = g_p1008b87c;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!Plane36_10002310(&anUnused[0], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x04)
	{
		if (!Plane36_10002670(&anUnused[1], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x08)
	{
		if (!Plane36_100029e0(&anUnused[2], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x10)
	{
		if (!Plane36_10002d40(&anUnused[3], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x20)
	{
		if (!Plane36_100030a0(&anUnused[4], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (!(nClipFlags & 0x02) ||
	    Plane36_10003410(&anUnused[5], &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}

	return LTFALSE;
}


// ---------------------------------------------------------------------- //
//  THE SIX CLIP PASSES.  Character for character big4e3e4.cpp's
//  CLIP4E3E4_HEAD/TAIL -- which is character for character big6d910.cpp's
//  CLIP6D910_HEAD/TAIL -- because it is character for character the same
//  source.  The only edits are the identifier prefix.
//
//  Each `if (nClip & BIT)` is a `test` on the SAME slot, -0x64(%ebp):
//    0x10049e71 test $0x1,%cl        0x1004a207 test $0x4,%al
//    0x1004a5b2 test $0x8,%al        0x1004a955 test $0x10,%al
//    0x1004acd7 test $0x20,%al       0x1004b055 test $0x2,%al
//
//  `if (nInside == 0)` rejects the whole polygon.  Here the shipped target
//  is the epilogue ITSELF, 0x1004b4c9, not a trampoline -- twelve `je
//  0x1004b4c9`, two per pass.  The FIRST of each pair is the `cmp %edi,%ebx
//  ; je` guard at the TOP of the classify loop (0x10049ea1 and five more):
//  icl rotated the loop and folded "the loop never ran, therefore nInside
//  is still zero", exactly as d3dr3a.cpp records for the standalone
//  0x10002d40.  The SECOND is the real `test %eax,%eax ; je` after it
//  (0x10049ed5 and five more).
//
//  The classify loop is emitted pointer-walking (`add $0x24,%edx ;
//  cmp %edi,%edx` at 0x10049eb9/0x10049ecd) and the edge loop
//  index-walking on the flag array (`mov -0x15ec(%ebp,%eax,4),%eax` at
//  0x10049f56) and pointer-walking on the vertices; both are written that
//  way below.
//
//  The tail's `pOut = pOut + nIn` DOUBLE-ADVANCES the cursor -- pOut is
//  already past what was just written and *nIn more* vertices are added to
//  it: `sub %edx,%eax ; cltd ; idiv $0x24` then
//  `lea (%eax,%eax,1),%edx ; add %edx,%edx ; lea (%edx,%edx,8),%edx ;
//  add %ebx,%edx` at 0x1004a1de..0x1004a1fe.  d3dr3a.cpp and big4e3e4.cpp
//  record the same double advance.  It is what the bytes say -- and see the
//  `lea` census row for the proof that pass 6's copy of it is DEAD.
// ---------------------------------------------------------------------- //
#define CLIP49BB0_HEAD(BIT, INSIDE)                                          \
	if (nClip & (BIT))                                                       \
	{                                                                        \
		int32       abInside[CLIP49BB0_MAX];                                 \
		float       fRed, fGreen, fBlue, fAlpha, fSpec;                      \
		Vert49bb0  *pEnd, *pWalk, *pPrev;                                    \
		int32      *pFlag;                                                   \
		int32       nInside, nFlag, i, j;                                    \
		float       fDen, fT, fVal;                                          \
                                                                             \
		g_n1008be44++;                                                       \
                                                                             \
		nInside = 0;                                                         \
		pFlag   = abInside;                                                  \
		pEnd    = pIn + nIn;                                                 \
                                                                             \
		for (pWalk = pIn; pWalk != pEnd; pWalk++)                            \
		{                                                                    \
			nFlag    = (INSIDE) ? 1 : 0;                                     \
			nInside += nFlag;                                                \
			*pFlag   = nFlag;                                                \
			pFlag++;                                                         \
		}                                                                    \
                                                                             \
		if (nInside == 0)                                                    \
			return;                                                          \
                                                                             \
		if (nInside != nIn)                                                  \
		{                                                                    \
			pOutStart = pOut;                                                \
                                                                             \
			j     = nIn - 1;                                                 \
			pPrev = pIn + (nIn - 1);                                         \
                                                                             \
			for (i = 0; i < nIn; i++)                                        \
			{                                                                \
				pWalk = &pIn[i];                                             \
                                                                             \
				if (abInside[j])                                             \
				{                                                            \
					*pOut = *pPrev;                                          \
					pOut++;                                                  \
				}                                                            \
                                                                             \
				if (abInside[j] != abInside[i])                              \
				{

#define CLIP49BB0_TAIL                                                       \
					CLIP49BB0_LERP                                           \
					pOut++;                                                  \
				}                                                            \
                                                                             \
				pPrev = pWalk;                                               \
				j     = i;                                                   \
			}                                                                \
                                                                             \
			pIn  = pOutStart;                                                \
			nIn  = (int32)(pOut - pOutStart);                                \
			pOut = pOut + nIn;                                               \
		}                                                                    \
	}


// ======================================================================= //
//  0x10049bb0 (stack entry) / 0x10049bb4 (register entry) .. 0x1004b5bb
//  6,668 bytes.  One caller, 0x10049abc.
//
//  __thiscall with two stack arguments (`ret $0x8` at 0x1004b4cf, the
//  arguments read at 0x8(%ebp) and 0xc(%ebp)).  `this` is NEVER READ.
// ======================================================================= //
void C10048180::sub_10049bb0(Shadow49bb0* pShadow, Item49bb0* pItem)
{
	// ------------------------------------------------------------------ //
	//  THE FRAME.  `mov $0x2910,%eax ; call 0x100805a0` at 0x10049bb7 --
	//  10,512 bytes -- and it DECOMPOSES EXACTLY.  Every slot below is
	//  dumped from the disassembly, not assumed:
	//
	//    -0x28f0 .. -0x2805   THIRTY float temps, 4 bytes each on an
	//                         8-byte stride (icl aligns a float temp to 8
	//                         and lets the next non-float local fill the
	//                         pad).  = 6 passes x {fRed,fGreen,fBlue,
	//                         fAlpha,fSpec}.  Pass 1 uses -0x28f0/-0x28e8/
	//                         -0x28e0/-0x28d8/-0x28d0 (the five `fstps` at
	//                         0x1004a032, 0x1004a083, 0x1004a0d4,
	//                         0x1004a125, 0x1004a176) and pass 6 uses
	//                         -0x2828/-0x2820/-0x2818/-0x2810/-0x2808
	//                         (0x1004b1f7, 0x1004b242, 0x1004b28d,
	//                         0x1004b2d8, 0x1004b323).  ALL THIRTY `fstps`
	//                         are enumerated in the census below and they
	//                         walk the run in strict pass order with no
	//                         gap and no repeat.  -0x28f0 + 29*8 = -0x2808
	//                         exactly.  0x2910 - 0x28f0 = 0x20 of bottom
	//                         alignment, the same 0x20 0x1004e3e4 has.
	//    -0x2804              SrcVert49bb0* pData   (out-param, 0x10049bcb)
	//                         -- it occupies the last float temp's 4-byte
	//                         alignment pad
	//    -0x2800              int32 nVerts          (out-param, 0x10049bd1)
	//    -0x27fc .. -0x1601   Vert49bb0 aVerts[128] = 0x1200 bytes.
	//                         THE 128 IS READ OFF THE FRAME (0x1200 / 0x24)
	//                         and it is the same 128 that 0x1000a740
	//                         enforces.
	//    -0x15fc              Vert49bb0* pVerts     (0x10049cc6)
	//    -0x15f8 .. -0x15ed   ONE 12-byte vector temp: icl's hidden return
	//                         slot for operator*(float) (0x10049c59, read
	//                         back at 0x10049c76).  0x1004e3e4 has THREE
	//                         of these because it also calls operator- and
	//                         Mag(); this function calls neither, and the
	//                         two missing twelve-byte slots are exactly the
	//                         difference between the two frames once the
	//                         thirty float temps are lined up.
	//    -0x15ec .. -0x10c5   SIX 0xdc plane blocks = 0x528.  Flag arrays
	//                         at -0x15ec, -0x1510, -0x1434, -0x1358,
	//                         -0x127c, -0x11a0; 0xdc = 0xc8 (int32[50]) +
	//                         0x14 (five int32 `fistpl` temps).
	//    -0x10c4 .. -0x00c5   D3DVert49bb0 aOut[128] = 0x1000 bytes,
	//                         32-byte FVF 0x1c4 vertices.  Base at
	//                         0x1004b3ee and 0x1004b4a7; the five field
	//                         addresses -0x10b8/-0x10b4/-0x10b0/-0x10ac/
	//                         -0x10a8 are aOut[0]+0x0c/+0x10/+0x14/+0x18/
	//                         +0x1c (0x1004b417..0x1004b445).  THE SECOND
	//                         128 IS READ OFF THE FRAME (0x1000 / 0x20).
	//    -0x00c4 .. -0x0001   0xc4 of scalars.  Fourteen of them are
	//                         referenced and all fourteen are identified:
	//                           -0xc0  int32   the shared byte-lerp temp
	//                                          (60 stores, 60 `fildl`)
	//                           -0xac  pOut          (73 refs); in region B
	//                                  it holds the cached `pItem`
	//                                  (0x10049c21)
	//                           -0xa8  pPrev; in region B, the loop counter
	//                                  (0x10049c09)
	//                           -0xa4  j; in region B, the 0x24-stride byte
	//                                  offset into aVerts (0x10049c1b)
	//                           -0xa0  nIn; in region B, the cached
	//                                  &aVerts[i] (0x10049c3e)
	//                           -0x9c  i
	//                           -0x98 .. -0x70  the SIX 8-byte fT spills,
	//                                  one per pass: pass 1 -0x98
	//                                  (0x10049f9f), pass 2 -0x90, pass 3
	//                                  -0x88, pass 4 -0x80, pass 5 -0x78,
	//                                  pass 6 -0x70 (0x1004b179).  Each
	//                                  carries exactly 10 `fmull` (2
	//                                  coordinate + 3 float + 5 byte lerps)
	//                                  and exactly 2 `fstpl` (the inline
	//                                  arm and the sunk arm), which is what
	//                                  identifies them.  icl also parks the
	//                                  pointer/int locals of OTHER passes in
	//                                  these same slots (pOutStart of pass 1
	//                                  is -0x70, 0x10049e62) -- non-
	//                                  overlapping live ranges, the same
	//                                  slot sharing 0x1004e3e4 records for
	//                                  its own -0x48.
	//                           -0x68  pIn
	//                           -0x64  nFlags / nClip (two stores at
	//                                  0x10049e38 and 0x10049e48, six reads,
	//                                  one per pass)
	//                         -0x60 .. -0x04 (0x5c bytes) is NEVER
	//                         REFERENCED -- see the UNDECODED note in the
	//                         trailer; it is reserved scalar space, not a
	//                         missing object.
	// ------------------------------------------------------------------ //
	SrcVert49bb0*	pData;              // -0x2804
	int32			nVerts;             // -0x2800
	Vert49bb0		aVerts[128];        // -0x27fc .. -0x1601
	Vert49bb0*		pVerts;             // -0x15fc

	Vert49bb0*		pIn;                // -0x68
	Vert49bb0*		pOut;               // -0xac
	Vert49bb0*		pOutStart;          // -0x70 in pass 1, and elsewhere
	int32			nIn;                // -0xa0
	uint32			nFlags;             // -0x64
	uint32			nClip;              // -0x64
	int32			i;

	D3DVert49bb0	aOut[128];          // -0x10c4 .. -0xc5

	// ================================================================== //
	//  REGION A -- 0x10049bb4 .. 0x10049c00.  The geometry, and the only
	//  early exit that is not the clipper's.
	//
	//  `mov $0x2910,%eax ; call 0x100805a0` is the stack probe on the
	//  10,512-byte frame; it is a benign shipped-only row
	//  and NOT to be chased with /Ge.
	//
	//  The prologue is `push %ebp ; mov %esp,%ebp ; <chkstk> ; push %edi ;
	//  push %edi ; push %esi ; push %ebx ; add $-0x10,%esp`.  There are
	//  FOUR pushes and only THREE callee-saved registers to save: the
	//  extra `push %edi` at 0x10049bc1 is icl's spare outgoing dword, and
	//  the epilogue (0x1004b4c9) pops exactly three registers and then
	//  restores %esp from %ebp, which discards it.  The outgoing
	//  arithmetic across regions A and C balances on that:
	//  `add $-0x10` (four dwords) - three written and `add $0xc` -
	//  `push %eax` - `add $0x8` returns %esp to where the four pushes
	//  left it.
	//
	//  0x10049bfb `test %eax,%eax ; je 0x1004b4d4` and 0x1004b4d4 is
	//  `jmp 0x1004b4c9`, the epilogue -- a sunk two-byte trampoline.
	// ================================================================== //
	sub_1000a700(pItem, &pData, &nVerts);

	if (!sub_1000a740(nVerts))
		return;

	// ================================================================== //
	//  REGION B -- 0x10049c01 .. 0x10049cbc.  Copy the polygon's own
	//  positions into aVerts and lift each one off the polygon plane.
	//
	//  0x10049c4b `mov -0xac(%ebp),%ecx ; call 0x1004b630` -- the two-hop
	//  accessor, called INSIDE the loop, once per vertex, not hoisted --
	//  then 0x10049c6e operator*(g_f1008ee84) into the hidden return slot
	//  -0x15f8, then 0x10049c8b operator+= with the twelve bytes copied
	//  into the outgoing area by `rep movsl %ecx=3`.
	//
	//  The induction variables are the counter at -0xa8, a 0x24-stride
	//  byte offset at -0xa4 into aVerts and a 0x18-stride byte offset in
	//  %ebx into pData -- two indexed walks, written indexed.  The bound
	//  is a FRESH LOAD of -0x2800 each turn (0x10049ca9, inside the loop),
	//  so the condition names nVerts and not a cached copy -- which it
	//  must, since &nVerts was passed to 0x1000a700 and the slot is
	//  address-exposed.
	//
	//  `pItem` is copied to -0xac(%ebp) once, at 0x10049c21, before the
	//  loop: the incoming argument slot is not re-read inside the body.
	// ================================================================== //
	for (i = 0; i < nVerts; i++)
	{
		aVerts[i].m_vPos = *pData[i].m_pPos00;
		aVerts[i].m_vPos.sub_1004b640(
			pItem->sub_1004b630()->sub_10015690(g_f1008ee84));
	}

	// ================================================================== //
	//  REGION C -- 0x10049cbd .. 0x10049d48.  Clip the lifted polygon to
	//  the shadow volume's own six planes.
	//
	//  0x10049cdb sets the fourth outgoing slot to the literal 6 and
	//  0x10049cd5 passes `pShadow + 0xa0`; on failure (`test %eax,%eax ;
	//  je 0x1004b4d2`) 0x1004b4d2 is the second two-byte trampoline into
	//  the epilogue.
	//
	//  0x10049d04: the clipper may have left the result in ITS OWN
	//  scratch, so the pointer is compared against aVerts and the result
	//  copied back when it differs.  `add %eax,%eax ; add %eax,%eax ;
	//  lea (%eax,%eax,8),%eax` at 0x10049d2a..0x10049d2e is nVerts * 36.
	//
	//  pShadow is read out of its argument slot ONCE, at 0x10049cc3
	//  (`mov 0x8(%ebp),%ebx`), and stays in %ebx from here to the end of
	//  region D.
	// ================================================================== //
	pVerts = aVerts;

	if (!sub_10047cf0(&pVerts, &nVerts, pShadow->m_Planes, 6))
		return;

	if (pVerts != aVerts)
	{
		memcpy(aVerts, pVerts, nVerts * sizeof(Vert49bb0));
		pVerts = aVerts;
	}

	// ================================================================== //
	//  REGION D -- 0x10049d49 .. 0x10049dfc.  Per vertex: the colours and
	//  the three projective texture coordinates.
	//
	//  THIS IS THE ONE REGION THAT DIFFERS FROM 0x1004e3e4, and the
	//  difference is a simplification in both directions:
	//
	//    * NO per-vertex alpha computation.  0x1004e3e4 spends
	//      operator-, Mag(), a hoisted 1.0f/fRadius, a zero clamp, a
	//      g_f1008eea4 scale and the module's only fnstcw/fldcw dance on
	//      it.  Here the alpha is ONE BYTE LOAD of g_b1008eea0
	//      (0x10049d70) stored straight to +0x13 (0x10049d7a).  That is
	//      why this function has no `fdivs`, no `fcomp` beyond the
	//      eighteen the clipper needs, no fnstcw and no fldcw, and why its
	//      frame carries one twelve-byte vector temp where 0x1004e3e4
	//      carries three.
	//    * THE POSITION IS READ DIRECTLY, not through operator[].
	//      0x1004e3e4 makes NINE calls to 0x1004e2f0 here, three per
	//      matrix row; this function's matrix rows load
	//      `(%eax,%ecx,1)`, `0x4(%eax,%ecx,1)` and `0x8(%eax,%ecx,1)`
	//      inline (0x10049d84, 0x10049d76, 0x10049d68 and so on).  That
	//      alone is nine of the eleven calls' worth of difference between
	//      the two functions.
	//    * THE TWO COLOURS ARE THE OTHER WAY ROUND.  0x1004e3e4 writes
	//      diffuse 0xffffffff and specular 0; this writes diffuse
	//      0x00000000 with its alpha byte replaced, and specular
	//      0xffffffff.  `xor %edx,%edx` at 0x10049d66 (re-issued each turn
	//      because %dl is clobbered by the alpha load) feeds the dword
	//      store at 0x10049d6c; `mov $0xffffffff,%edi` at 0x10049d5b is
	//      hoisted OUT of the loop and feeds 0x10049de3.
	//
	//  The three rows are 0, 1 and 3 of m_mTex -- +0x60..+0x6c,
	//  +0x70..+0x7c and +0x90..+0x9c.  ROW 2 IS NEVER READ.  Each row is
	//  three `fmul`s and a translate: row 0 is 3 `faddp`, rows 1 and 3 are
	//  2 `faddp` + 1 `fadd` (of the translate from memory), which is icl
	//  scheduling the same four-term sum two ways in one loop body -- add
	//  order in an FPU sequence is not source-visible.
	//
	//  The store ORDER in the bytes is +0x10, +0x13, +0x18, +0x1c, +0x14,
	//  +0x20: the specular's integer store at 0x10049de3 is scheduled into
	//  row 3's FPU latency, one instruction before row 3's `fstps`.  It is
	//  written LAST below; see the coin-flip note in the trailer.
	// ================================================================== //
	{
		Vert49bb0*	pVert;

		for (i = 0; i < nVerts; i++)
		{
			pVert = &pVerts[i];

			pVert->m_Diffuse10.m_n    = 0;
			pVert->m_Diffuse10.m_b[3] = g_b1008eea0;

			pVert->m_fU = pShadow->m_mTex[0][0] * pVert->m_vPos.x
			            + pShadow->m_mTex[0][1] * pVert->m_vPos.y
			            + pShadow->m_mTex[0][2] * pVert->m_vPos.z
			            + pShadow->m_mTex[0][3];

			pVert->m_fV = pShadow->m_mTex[1][0] * pVert->m_vPos.x
			            + pShadow->m_mTex[1][1] * pVert->m_vPos.y
			            + pShadow->m_mTex[1][2] * pVert->m_vPos.z
			            + pShadow->m_mTex[1][3];

			pVert->m_fQ = pShadow->m_mTex[3][0] * pVert->m_vPos.x
			            + pShadow->m_mTex[3][1] * pVert->m_vPos.y
			            + pShadow->m_mTex[3][2] * pVert->m_vPos.z
			            + pShadow->m_mTex[3][3];

			// 0x10049de3 -- a dword store of the -1 held in %edi.
			pVert->m_Specular14.m_n = 0xffffffff;
		}
	}

	// ================================================================== //
	//  REGION E -- 0x10049dfd .. 0x10049e50.  Camera space, and the clip
	//  mask.
	//
	//  0x10049e10 writes the literal 0x3f into g_n1008b874 and 0x10049e33
	//  reads it back after the loop; icl kept the 0x3f in %eax
	//  (0x10049e09, BEFORE the store) so the zero-trip path skips the
	//  reload -- which is why the `mov $0x3f,%eax` is where it is and not
	//  a second constant.  The loop is the rotated post-decrement shape
	//  (`test %edx,%edx ; je 0x10049e38` then `dec %esi ; jne 0x10049e23`),
	//  which big6d910.cpp and big4e3e4.cpp both write as `while (nLeft--)`
	//  in exactly this place for exactly this call.
	//
	//  0x10049e3b: UseD3DClip.  When it is set only the near plane is
	//  done, and if that bit is clear the whole clipper AND the
	//  `pVerts = pIn ; nVerts = nIn` epilogue at 0x1004b383 are skipped by
	//  the jump to 0x1004b3ad.
	//
	//  Both `nFlags` and `nClip` live in -0x64: 0x10049e38 stores the mask
	//  and 0x10049e48 stores the narrowed copy back over it.  Two
	//  statements, one slot.
	// ================================================================== //
	{
		Vert49bb0*	pWalk;
		int32		nLeft;

		g_n1008b874 = 0x3f;

		pWalk = pVerts;
		nLeft = nVerts;

		while (nLeft--)
		{
			sub_1000d9b0(pWalk, g_1008b880);
			pWalk++;
		}

		nFlags = g_n1008b874;
	}

	if (!ClipPoly49bb0(nFlags, &pVerts, &nVerts))
		return;

	// ================================================================== //
	//  REGION G -- 0x1004b3ad .. 0x1004b4c8.  Screen space, the D3D
	//  vertices, and the draw.
	//
	//  The 0x1004b5c0 loop is the same rotated post-decrement as region
	//  E's (`test %eax,%eax ; je 0x1004b3d4` then `dec %esi ;
	//  jne 0x1004b3c4`); it writes each vertex's +0x0c (the RHW, 1.0f/z
	//  through the 1.0f at 0x100a9f5c) and scales x, y and z, which is the
	//  only reason +0x0c is defined by the time it is copied out below.
	//
	//  The copy loop at 0x1004b3ee walks aOut with a 0x20 byte offset at
	//  -0x78 and pVerts with a 0x24 byte offset in %ebx: twelve bytes of
	//  position by `rep movsl %ecx=3` (0x1004b408), then three integer
	//  moves out of +0x0c, +0x10 and +0x14 (0x1004b423, 0x1004b43c,
	//  0x1004b44f), then the two texture coordinates divided by +0x20
	//  (`fld 0x18(%eax,%ebx,1) ; fdivs 0x20(%eax,%ebx,1)` at 0x1004b45b
	//  and the same pair at 0x1004b476).  +0x20 IS LOADED TWICE, once per
	//  divide -- two `fdivs`, not one reciprocal, so the source divides
	//  twice.  Note also that the loop counter is a separate variable at
	//  -0x70 (`inc %ecx` at 0x1004b463) from the two byte offsets, and
	//  that the bound is re-read from -0x2800 each turn (0x1004b484).
	//
	//  0x1004b4c4 `mov (%edx),%edx ; call *0x64(%edx)` with six outgoing
	//  dwords: the device (0x1004b495/0x1004b4ad), 6 (D3DPT_TRIANGLEFAN,
	//  0x1004b4bc), 0x1c4 (XYZRHW | DIFFUSE | SPECULAR | TEX1,
	//  0x1004b4b0), aOut (0x1004b4a7/0x1004b4b8), the count
	//  (0x1004b4a3, the %eax still holding nVerts from 0x1004b484) and 0
	//  (0x1004b49b).  This is the function's only indirect call and the
	//  last of its eleven.  0x1008d988 is read ONCE.
	// ================================================================== //
	{
		Vert49bb0*	pWalk;
		int32		nLeft;

		pWalk = pVerts;
		nLeft = nVerts;

		while (nLeft--)
		{
			sub_1004b5c0(pWalk, g_1008b880);
			pWalk++;
		}
	}

	for (i = 0; i < nVerts; i++)
	{
		aOut[i].m_vPos     = pVerts[i].m_vPos;
		aOut[i].m_fRHW     = pVerts[i].m_fRHW;
		aOut[i].m_Diffuse  = pVerts[i].m_Diffuse10;
		aOut[i].m_Specular = pVerts[i].m_Specular14;
		aOut[i].m_fU       = pVerts[i].m_fU / pVerts[i].m_fQ;
		aOut[i].m_fV       = pVerts[i].m_fV / pVerts[i].m_fQ;
	}

	g_p1008d988->m_pVtbl->DrawPrimitive(g_p1008d988, 6, 0x1c4, aOut,
	                                    nVerts, 0);
}


// *********************************************************************** //
//
//  WHAT THIS DECODE CLAIMS, AND WHAT IT DOES NOT
//
// *********************************************************************** //
//
//  ------------------------------------------------------------------- //
//  1. THE CALL SEQUENCE.  Predicted before any build.
//  ------------------------------------------------------------------- //
//
//      0x10049bb0..0x1004b5bc against our sub_10049bb4.
//
//  The shipped sequence, in order, with the address of each site:
//
//      0x10049bbc  0x100805a0   _chkstk       <- SHIPPED ONLY, expected
//      0x10049be2  0x1000a700   region A
//      0x10049bf1  0x1000a740   region A
//      0x10049c51  0x1004b630   \
//      0x10049c6e  0x10015690    >  region B, one loop body
//      0x10049c8b  0x1004b640   /
//      0x10049cf4  0x10047cf0   region C
//      0x10049d35  0x100805d0   region C, memcpy
//      0x10049e25  0x1000d9b0   region E
//      0x1004b3c6  0x1004b5c0   region G
//      0x1004b4c6  *0x64(%edx)  region G, INDIRECT
//
//  The source above issues those nine direct calls in exactly that order
//  and one indirect in that position, so the expected figure is
//
//      shipped calls 11   ours 10
//      in agreement  10
//      shipped-only   1   ours-only 0
//
//  and the single shipped-only row is `_chkstk` at 0x10049bbc, a known
//  benign residual.  DO NOT add /Ge to close it -- measured on
//  0x10060010 it trades one known residual for nineteen unknown ones.
//
//  The /Qip two-entry offset does NOT cost a row here, because the range
//  starts at the stack entry: the shipped call at 0x10049abc goes to
//  0x10049bb4 and our object's symbol is one function, so the comparison resolves
//  it by name.  (If the row were left at 0x10049bb4 as the enumeration has
//  it, nothing would change either -- the four stack-entry bytes contain
//  no call.  That is precisely why this defect went unnoticed.)
//
//  THREE THINGS THAT COULD MAKE THE FIGURE WORSE, all recognisable:
//
//    * memcpy at 0x10049d35.  If icl expands it inline (`rep movsb`)
//      instead of calling, that is one more shipped-only row at that
//      address.  The size is a runtime variable (nVerts * 36), so a call
//      is the likely outcome, and it is what big4e3e4.cpp predicted for
//      the identical statement.
//    * THE THREE ACCESSOR CALLS.  0x1004b630, 0x10015690 and 0x1004b640
//      are the reason those three are DECLARED AND NOT DEFINED in this
//      file.  If any of them ever acquires a body here, its call vanishes
//      and the figure drops to 9 or 8 of 11 -- exactly the failure seen
//      for 0x1005dca0.  Region B is three of the ten calls we
//      make; spelling it `aVerts[i].m_vPos += *pItem->m_pPoly28->
//      m_pPlane34 * g_f1008ee84;` with real definitions would emit NONE
//      of them.
//    * REGION D IS NOT A SOURCE OF CALLS HERE and must not become one.
//      0x1004e3e4's region D makes eleven calls (operator-, Mag() and
//      nine operator[]); this one makes zero, because the shipped bytes
//      read `(%eax,%ecx,1)`, `0x4(%eax,%ecx,1)` and `0x8(%eax,%ecx,1)`
//      directly.  Writing `pVert->m_vPos.sub_1004e2f0(0)` here -- copying
//      the sibling too faithfully -- would invent nine ours-only rows.
//
//  ------------------------------------------------------------------- //
//  2. THE /Oi- JUDGEMENT.  This file DECLINES the flag, with a reason.
//  ------------------------------------------------------------------- //
//
//  The CRT-intrinsic gap is closed by `/O2 /Oi-`, and
//  build/engine.sh carries a D3D_OI_TUS list for the files where it was
//  measured.  THIS FILE SHOULD NOT BE ADDED TO IT, and the reason is
//  read off the census rather than assumed:
//
//    * THERE IS NO CRT MATH IN THIS FUNCTION TO RECOVER.  The census has
//      no `fsqrt`, no `fabs`, no `fptan`, no `fpatan` and no call to
//      0x1007fc98 / 0x1007fc90 / 0x1007fdf4 / 0x1007fe1c -- unlike
//      0x10048180 next door, whose four inlined Norm sites do carry a
//      `call 0x1007fc98`.  The flag has nothing to buy here.
//    * IT HAS SOMETHING TO LOSE.  This function contains NINE `rep movsl`
//      that are STRUCT ASSIGNMENTS, not memcpy calls: two twelve-byte
//      copies in region B (0x10049c49 and 0x10049c83), six thirty-six-byte
//      whole-vertex copies in the clip passes (0x10049f4e and five more),
//      and one twelve-byte copy in region G (0x1004b408).  But
//      /Oi- "can turn open-coded 32-byte copies into ours-only memcpy
//      rows"; nine such copies is the largest exposure of any function in
//      this family, and every one of them would be an ours-only row.
//
//  So: build this file at plain `/O2 /Qip`.  IF the first build shows a
//  shipped-only row at 0x10049d35 and nothing else, THEN /Oi- is the fix
//  for that one row and the nine `rep movsl` must be re-checked in the
//  same build -- 10 of 11 with one memcpy row is a better result than
//  11 of 11 with nine invented ones.
//
//  ------------------------------------------------------------------- //
//  3. HOW MUCH OF THE FUNCTION THE CALL CHECK WITNESSES
//  ------------------------------------------------------------------- //
//
//  17.0% of it -- 1,135 of 6,668 bytes -- and every byte of it is at the
//  two ends.  The five call-free stretches are listed by address in the
//  banner at the top of this file; the big one is 5,532 bytes, 83.0% of
//  the function, and it contains regions E (tail), F entire, both reject
//  trampolines and all six sunk epsilon arms.
//
//  WHAT CARRIES THAT INSTEAD, in decreasing order of strength:
//
//  ------------------------------------------------------------------- //
//  3a. THE INSTRUCTION CENSUS, WITH NO RESIDUE.
//  ------------------------------------------------------------------- //
//
//  1,672 instructions, 44 distinct mnemonics.  Every row is decomposed by
//  operand below and every one of them closes exactly.  A seventh plane, a
//  fourth interpolated float, a sixth interpolated byte, a 49th or 51st
//  flag, a 64- or 256-vertex array, a missing `-fVal` store or a missing
//  `pOut++` would each move at least three of these rows.
//
//    fld      114 = 10 (region D: 4 position loads + 6 matrix elements
//                        per turn -- 0x10049d68, 0x10049d76, 0x10049d81,
//                        0x10049d87, 0x10049d95, 0x10049da2, 0x10049dab,
//                        0x10049db6, 0x10049dc6, 0x10049dd2)
//                 + 2 (region G: the two dividends, 0x1004b45b/0x1004b476)
//                 + 6 (the six classify loads)
//                 + 12 (six fDen, two loads each)
//                 + 6 (six inline epsilon constants)
//                 + 4 (extra numerator loads, passes 2/3/4/5 --
//                      `fld 0x8(%edx)` before the FADDP/FSUBP)
//                 + 12 (six passes x 2 coordinate lerps)
//                 + 2 (`fld %st(0)` for the two duplicate-and-negate
//                      stores, 0x1004a372 and 0x1004ae2f)
//                 + 18 (six passes x 3 float lerps)
//                 + 30 (thirty byte-lerp reloads of the float temp)
//                 + 12 (six sunk arms: 1 epsilon + 1 zero each)
//    fstp      95 = 14 (position replacements: 2+3+2+2+3+2 by pass -- the
//                        threes are passes 2 and 5, whose `-fVal` needs a
//                        third store, and passes 1 and 6 store z with an
//                        INTEGER mov so they have only two)
//                 + 18 (six passes x 3 float lerps)
//                 + 30 (six passes x 5 byte lerps, the `fstps` into the
//                        thirty float temps -- ALL THIRTY ENUMERATED in
//                        the frame note: 0x1004a032/083/0d4/125/176,
//                        0x1004a3dd/42e/47f/4d0/521, 0x1004a780/7d1/822/
//                        873/8c4, 0x1004ab0e/b5c/baa/bf8/c46,
//                        0x1004ae8f/edd/f2b/f79/fc7, 0x1004b1f7/242/28d/
//                        2d8/323)
//                 + 6 (six inline `fstpl` of fT)
//                 + 6 (six sunk `fstpl` of fT)
//                 + 16 (x87 stack cleanups in the sunk arms: 2,3,3,3,3,2)
//                 + 3 (region D's three matrix rows)
//                 + 2 (region G's two divides)
//    fmul      69 = 9 (region D: 3 per matrix row x 3 rows)
//                 + 60 (six passes x 10 -- 2 coordinate + 3 float + 5
//                        byte lerps, which is exactly the 10 `fmull` on
//                        each of the six fT slots)
//    fadd      36 = 2 (region D: rows 1 and 3 add the translate from
//                        memory; row 0 uses a third `faddp`)
//                 + 4 (fDen third terms: `fadds 0x8(%ebx)` in passes 2/5,
//                        `fadds 0x8(%edx)` in passes 3/4)
//                 + 12 (six passes x 2 coordinate lerps)
//                 + 18 (six passes x 3 float lerps)
//    faddp     39 = 7 (region D: 3 in row 0, 2 each in rows 1 and 3)
//                 + 30 (six passes x 5 byte lerps)
//                 + 2 (numerators of passes 2 and 5, `de c2`)
//    fsub      40 = 6 (six fDen `fsub %st,%st(1)`)
//                 + 4 (fDen fourth terms: `fsubs 0x8(%edx)` in passes 2/5,
//                        `fsubs 0x8(%ebx)` in passes 3/4)
//                 + 12 (six passes x 2 coordinate lerps)
//                 + 18 (six passes x 3 float lerps)
//    fsubr      2 = passes 1 and 6 numerators (`fsubrs 0x1008b908` at
//                    0x10049f97, `fsubrs 0x1008b910` at 0x1004b171)
//    fsubp      2 = passes 3 and 4 numerators (`de ea` at 0x1004a6eb and
//                    0x1004aa8b)
//    fdivrp     6 = the six fT divisions (`de f1`)
//    fdiv       2 = region G's u/q and v/q -- TWO divides, and +0x20 is
//                    loaded twice, so no reciprocal was formed
//    fchs       8 = 2 classify negations (passes 2 and 5, 0x1004a242 and
//                    0x1004ad12) + 4 numerator negations (passes 2/3/4/5,
//                    0x1004a343, 0x1004a6ef, 0x1004aa8f, 0x1004ae0d) + 2
//                    duplicate-and-negate stores (0x1004a374, 0x1004ae31)
//    fld/fst    2 = `fst 0x4(%ecx)` at 0x1004a71a and `fst (%ecx)` at
//                    0x1004aab5 -- passes 3 and 4, which store ONE value
//                    to two coordinates with no negation
//    fxch      11 = 3 (region D) + 6 (one per pass, before the divide)
//                 + 2 (the two duplicate-and-negate stores)
//    fcomp     18 = 6 classify + 6 inline epsilon + 6 sunk epsilon
//    fnstsw    18, sahf 18 = the same eighteen comparisons.  There is no
//                    nineteenth: 0x1004e3e4's extra one is its `fAlpha <
//                    0.0f` clamp, which this function does not have.
//    fnstcw     0, fldcw 0 = /QIfist.  The thirty `fistpl` sites do no
//                    control-word dance, and unlike 0x1004e3e4 there is
//                    not one anywhere in the function, because region D
//                    performs no float-to-int conversion at all.
//    fild      60 = six passes x 5 bytes x 2 (current and previous)
//    fistp     30 = six passes x 5 bytes
//    movzx     60 = the same sixty, and BY OFFSET it is 12 x +0x10,
//                    12 x +0x11, 12 x +0x12, 12 x +0x13, 12 x +0x17 --
//                    FIVE bytes, not four and not six.  The thirty stores
//                    that consume them are enumerated in the LERP note.
//    cdq        6, idiv 6 = the six `(pOut - pOutStart) / 0x24`
//                    (0x1004a1ec, 0x1004a597, 0x1004a93a, 0x1004acbc,
//                    0x1004b03a, 0x1004b390)
//    sub       36 = 30 byte-lerp integer differences (6 `sub %edx,%eax`,
//                    the first of each pass, plus 24 `sub %eax,%ecx`)
//                 + 6 pointer differences for the six divides (5
//                    `sub %ebx,%edx` plus pass 6's `sub %edi,%eax`)
//    inc       15 = 6 `incl 0x1008be44` + 6 `inc %eax` (the six edge
//                    loops' i++) + region B's `inc %edx` + region D's
//                    `inc %esi` + region G's `inc %ecx`
//    dec        2 = the two rotated post-decrement loops (region E
//                    0x10049e30, region G 0x1004b3d1)
//    xor       16 = 3 (region B's three zero inductions) + 1 (region D's
//                    counter) + 2 (region D's byte offset and the diffuse
//                    zero) + 6 (one per pass, `nInside = 0`) + 1 (pass 1's
//                    `i = 0`; passes 2..6 spell it `mov $0x0`) + 3
//                    (region G)
//    and        1 = `nClip &= 1` at 0x10049e45
//    test      32 = 1 (region A) + 1 (region C) + 1 (region B's zero-trip
//                    guard) + 1 (region D's) + 2 (region E: nVerts == 0
//                    and UseD3DClip) + 24 (six passes x 4: the bit test,
//                    `nInside == 0`, the edge loop's zero-trip guard, and
//                    `if (abInside[j])`) + 2 (region G's two loops)
//    cmp       34 = 1 (region B latch) + 1 (pVerts != aVerts) + 1
//                    (region D latch) + 30 (six passes x 5: the rotated
//                    classify guard, the classify latch, `nInside != nIn`,
//                    `abInside[j] != abInside[i]`, and the edge latch) + 1
//                    (region G latch)
//    je        43 = 6 + 36 + 1, and all forty-three are enumerated:
//                    0x1004b4d4 (region A), 0x1004b4d2 and 0x10049d49
//                    (region C), 0x10049e38 / 0x10049e51 / 0x1004b3ad
//                    (region E), then SIX PER PASS (the bit test, the
//                    rotated classify guard into the epilogue, the real
//                    `nInside == 0` into the epilogue, `nInside == nIn`,
//                    `if (abInside[j])` and `abInside[j] != abInside[i]`),
//                    then 0x1004b3d4 (region G)
//    jle        9, jl 9 = the head and latch of the nine counted loops:
//                    region B, region D, six edge loops, region G's copy
//    jne        8 = 2 post-decrement latches + 6 classify latches
//    jb        10 = 4 classify (passes 2/3/4/5) + 6 sunk second-epsilon
//    jbe        7 = 6 inline epsilon + pass 6's classify
//    jae        1 = pass 1's classify.  ONE `jae` and ONE `jbe` classify
//                    in the whole function, which is what makes the near
//                    plane `>=` and the far plane `<=`.
//    jmp        8 = 2 reject trampolines + 6 sunk-arm returns
//    rep        9 = 2 (region B: the position copy and the operator+=
//                    argument) + 6 (the six passes' whole-vertex copies,
//                    `mov $0x9,%ecx`) + 1 (region G)
//    call      11, ret 1, pop 4, push 10 (5 prologue + `push %eax` for
//                    0x1000a740 + 2 per __cdecl helper loop)
//    lea       55, AND THIS IS THE ROW THAT LOOKS LIKE NOISE AND IS NOT.
//                    By operand, all fifty-five:
//                      region A   2  &pData, &nVerts
//                      region B   3  aVerts, and the hidden return slot
//                                    twice (write and read-back)
//                      region C   8  aVerts x3, &nVerts, m_Planes,
//                                    &pVerts, the compare against aVerts,
//                                    and one `(%eax,%eax,8)` for x36
//                      region F  33  pass 1 FIVE, passes 2..5 SIX each,
//                                    PASS 6 FOUR
//                      region G   8  aOut base x2, the five aOut field
//                                    addresses, and `0x20(%edx)`
//                      padding    1  `lea 0x0(%esi),%esi` at 0x1004b5b9
//                    5 + 6x4 + 4 = 33, and 2+3+8+33+8+1 = 55 with nothing
//                    left over.
//
//                    THE 5 / 6 / 4 SPLIT IS THE LOAD-BEARING PART, and it
//                    says two separate things that no call, constant or
//                    frame slot shows:
//
//                      * PASS 1 HAS FIVE because it spells `nIn - 1` as
//                        `mov $-1,%edx ; add %eax,%edx` (0x10049eee) where
//                        passes 2..6 use `lea -0x4(,%eax,4),%edx`
//                        (0x1004a282, 0x1004a62c, 0x1004a9ce, 0x1004ad53,
//                        0x1004b0ca).  Same value, one lea
//                        fewer.  0x1004e3e4 records the identical
//                        asymmetry in the identical place.
//                      * PASS 6 HAS FOUR because IT HAS NO TAIL LEAS.
//                        Passes 1..5 each end with `lea (%eax,%eax,1),%edx
//                        ; add %edx,%edx ; lea (%edx,%edx,8),%edx ;
//                        add %ebx,%edx` -- the `pOut = pOut + nIn` double
//                        advance -- and pass 6's tail (0x1004b383) does
//                        the `idiv` and then stores pVerts and nVerts
//                        WITHOUT rebuilding pOut.  So the source's sixth
//                        `pOut = pOut + nIn` IS DEAD and icl deleted it,
//                        exactly as found for 0x100051a0's
//                        `lea` 60 = 12/6/12/10.  It is also why
//                        `add (%eax,%eax,1)` occurs 5 times and not 6.
//
//    mov      608.  NOT decomposed in full, and deliberately so: it is
//                    overwhelmingly the register allocator's pointer and
//                    index traffic and carries no structural claim.  The
//                    sub-rows that DO carry one are all closed:
//                      60  `mov %eax,-0xc0(%ebp)` -- the byte-lerp integer
//                          temp, 60 stores against 60 `fildl`, and that is
//                          the WHOLE of that slot's traffic
//                      30  `mov %dl,0xNN(%eax)` -- the byte-lerp stores,
//                          six each at +0x10/+0x11/+0x12/+0x13/+0x17
//                      30  `mov -0x15xx(%ebp),%dl` -- the `fistpl`
//                          read-backs, one per byte lerp
//                      18  `mov $0x24,%reg` = 6 (`mov $0x24,%eax` for the
//                          copy branch's pOut++) + 6 (`mov $0x24,%ecx` for
//                          the LERP branch's pOut++) + 6 (the six idiv
//                          divisors, five in %ecx and pass 6's in %edx).
//                          TWO `pOut++` per pass, which is the macro's
//                          shape and not one advance
//                       7  `mov $0xffffffff,%reg` = pass 1's `j = nIn-1`
//                          + five passes' `j = nIn-1` + region D's
//                          specular white (0x10049d5b, hoisted out of the
//                          loop)
//                      11  `mov $0x0,%reg` = the six classify loops' false
//                          flag + five passes' `i = 0`
//                       6  `mov $0x1,%reg` = the six classify loops' true
//                          flag
//                       6  `mov $0x9,%ecx` = the six whole-vertex copies
//                       3  `mov $0x3,%ecx` = the three twelve-byte copies
//                       2  `mov 0x1008b908/0x1008b910,%edx` = the two
//                          INTEGER plane replacements (0x10049fc9,
//                          0x1004b19a)
//                       2  `mov $0x1008b880,%edi` = the transform object,
//                          hoisted once per helper loop
//                       1  `mov $0x2910,%eax` = the _chkstk size
//    add      101 = 14 `add %eax,%eax` (the seven x36 multiplies' x4
//                    halves: one in region C and one per pass)
//                 + 7 `add %edx,%edx` (pass 1's pPrev x4, twice, plus the
//                    five live tails' x2)
//                 + 12 `add %eax,%edi` (six pEnd, six copy-branch pOut++)
//                 + 9 `add $0x24,%ebx` (six edge loops + regions E, G and
//                    G's copy loop)
//                 + 6 `add $0x24,%edx` (the six classify loops' pWalk++)
//                 + 6 `add $0x4,%ecx` (the six classify loops' pFlag++)
//                 + 12 esp adjustments (three `add $-0x10`, one `$-0x8`,
//                    one `$-0xc`, one `$-0x18`, three `add $0x8`, two
//                    `add $0x10`, one `add $0xc`)
//                 + 35 index arithmetic (11 `add %ecx,%eax`, 6
//                    `add %edx,%esi`, 6 `add %esi,%ebx`, 5
//                    `add %ebx,%edx`, 2 `add %eax,%edi`, and five
//                    singletons)
//
//  ------------------------------------------------------------------- //
//  3b. THE FRAME, DECOMPOSED EXACTLY.
//  ------------------------------------------------------------------- //
//
//      0x2910 = 10,512
//             =    32   bottom alignment          (-0x2910 .. -0x28f1)
//             +   240   30 float temps x 8        (-0x28f0 .. -0x2805)
//             +     4   pData (in the last pad)   (-0x2804)
//             +     4   nVerts                    (-0x2800)
//             + 4,608   Vert49bb0 aVerts[128]     (-0x27fc .. -0x1601)
//             +     4   pVerts                    (-0x15fc)
//             +    12   one vector temp           (-0x15f8 .. -0x15ed)
//             + 1,320   6 clip blocks x 0xdc      (-0x15ec .. -0x10c5)
//             + 4,096   D3DVert49bb0 aOut[128]    (-0x10c4 .. -0x00c5)
//             +   196   scalars                   (-0x00c4 .. -0x0001)
//             = 10,512
//
//  BOTH ARRAY BOUNDS ARE READ OFF THAT, not assumed: 128 from 4,608/36 and
//  independently from 4,096/32; FIFTY from 0xdc - 0x14 = 0xc8 = 200/4.
//  0x1004e3e4's frame is the same decomposition minus two twelve-byte
//  vector temps and minus 0x18 of scalars (0x28d8 against 0x2910), which
//  is exactly the region-D difference.
//
//  ------------------------------------------------------------------- //
//  3c. THE LITERAL POOL, read in order.
//  ------------------------------------------------------------------- //
//
//  The lower bound is PROVED FROM THE OTHER SIDE.  0x10048180 (the
//  immediately preceding function) loads 0x100a9de8..0x100a9ec4 densely,
//  and then exactly four more slots: 0x100a9ecc, 0x100a9ed4, 0x100a9edc
//  and 0x100a9ee4 -- which by icl's dead-1.0f rule are the LIVE members of four
//  `1.0f / x` division pairs whose dead partners are 0x100a9ec8,
//  0x100a9ed0, 0x100a9ed8 and 0x100a9ee0.  It loads NOTHING at or above
//  0x100a9ee8.  The upper bound is 0x100a9f58: 0x1004b5c0 loads
//  0x100a9f5c (its 1.0f/z), so 0x100a9f58 is that division's dead partner
//  and belongs to the next function.  Our run is therefore
//  0x100a9ee8 .. 0x100a9f54 and it reads as a transcript:
//
//    0x100a9ee8   0.0f     DEAD -- never loaded anywhere in .text
//    0x100a9eec   1.0f     DEAD
//    0x100a9ef0   1.0f     DEAD  \  region G's two divides (see below)
//    0x100a9ef4   1.0f     DEAD  /
//    0x100a9ef8  -1e-5     LIVE  pass 1 inline epsilon   (0x10049f84)
//    0x100a9efc   1.0f     DEAD  pass 1's division
//    0x100a9f00  -1e-5     LIVE  pass 2                  (0x1004a32b)
//    0x100a9f04   1.0f     DEAD  pass 2's division
//    0x100a9f08  -1e-5     LIVE  pass 3                  (0x1004a6d7)
//    0x100a9f0c   1.0f     DEAD  pass 3's division
//    0x100a9f10  -1e-5     LIVE  pass 4                  (0x1004aa77)
//    0x100a9f14   1.0f     DEAD  pass 4's division
//    0x100a9f18  -1e-5     LIVE  pass 5                  (0x1004adf5)
//    0x100a9f1c   1.0f     DEAD  pass 5's division
//    0x100a9f20  -1e-5     LIVE  pass 6                  (0x1004b15e)
//    0x100a9f24   1.0f     DEAD  pass 6's division
//    0x100a9f28  +1e-5     LIVE  pass 6 sunk arm         (0x1004b596)
//    0x100a9f2c   0.0f     LIVE  pass 6 sunk arm         (0x1004b5ab)
//    0x100a9f30  +1e-5 / 0x100a9f34  0.0f   pass 5 sunk  (0x1004b571/588)
//    0x100a9f38  +1e-5 / 0x100a9f3c  0.0f   pass 4 sunk  (0x1004b54c/563)
//    0x100a9f40  +1e-5 / 0x100a9f44  0.0f   pass 3 sunk  (0x1004b524/53b)
//    0x100a9f48  +1e-5 / 0x100a9f4c  0.0f   pass 2 sunk  (0x1004b4fc/513)
//    0x100a9f50  +1e-5 / 0x100a9f54  0.0f   pass 1 sunk  (0x1004b4d6/4eb)
//
//  FOUR THINGS COME OUT OF THAT.
//
//    (i)   IT SETTLES THE EPSILON SPELLING.  The six inline `-1e-5` are
//          contiguous at 0x100a9ef8..0x100a9f20 and the six `+1e-5` are in
//          a SEPARATE run at 0x100a9f28..0x100a9f50, so the second
//          comparison's literal was NOT emitted next to the first -- it
//          went out with the sunk block.  That is what a short-circuit
//          `||` whose second arm is sunk looks like, and it is how
//          d3dr3a.cpp read the same shape on the standalone 0x10002d40.
//          The `+1e-5`/`0.0f` pairs DESCEND, pass 6 lowest, because the
//          six sunk blocks' literals were allocated in reverse of their
//          emission order -- and the sunk blocks are emitted in reverse
//          pass order (0x1004b4d6 is pass 1 and 0x1004b596 is pass 6),
//          which the pool independently confirms.
//    (ii)  IT COUNTS THE DIVISIONS.  icl emits a dead 1.0f for
//          every float division.  This function has EIGHT (six `fdivrp`
//          plus region G's two `fdiv`), and the pool has six dead 1.0f
//          interleaved with the passes' epsilons plus three leading ones.
//          Two of the three leading ones are region G's -- 0x1004e3e4's
//          pool has its region-G pair in exactly the same anomalous
//          position, before the passes rather than after them -- and the
//          THIRD, together with the dead 0.0f at 0x100a9ee8, is not
//          accounted for.  See UNDECODED below; it is 8 bytes of pool and
//          no bytes of code, and guessing a statement to produce it would
//          be worse than saying so.
//    (iii) THERE IS NO 230.0f AND NO 0.05f IN OUR RUN.  Both live at
//          0x100a9f60 and 0x100a9f64 -- they are 0x1004b670's console-
//          variable defaults, not this function's constants, which is the
//          independent check that region D's alpha really is a byte load
//          of a live console value and not a folded literal.
//    (iv)  REGION D CONTRIBUTES NOTHING TO THE POOL.  No 0.0f clamp, no
//          1.0f numerator, no alpha scale -- which is the pool's own
//          confirmation that region D is the arithmetic written above and
//          not 0x1004e3e4's.
//
//  ------------------------------------------------------------------- //
//  3d. THE SIBLING DIFF, and a warning about x87 mnemonics.
//  ------------------------------------------------------------------- //
//
//  Region F was diffed against src/engine/d3d/big4e3e4.cpp plane by plane
//  -- the six bits in the same order (0x01, 0x04, 0x08, 0x10, 0x20, 0x02),
//  the same two globals, the same fDen expressions, the same numerator
//  signs, the same store orders, the same `fst`-versus-`fld/fchs/fxch`
//  split between passes 3/4 and 2/5, the same double advance, the same
//  fifty-flag bound, the same 0x24 vertex with three floats and five
//  bytes.  Not one expression differs.  d3dr3a.cpp's two standalone family
//  members (0x10002d40 "x < z", 0x100030a0 "-z < y") were used as the
//  third opinion on passes 4 and 5.
//
//  EVERY x87 REGISTER FORM WAS RE-VERIFIED ON A RUNNING CPU rather than
//  read off a mnemonic, because binutils swaps the
//  DE E0/E8/F0/F8 names and four decodes have been caught by it.  The
//  measurements (a three-deep x87 stack, `.byte` encodings, values printed
//  back):
//
//      de f1   st(1) <- st(0) / st(1), pop     (Intel FDIVRP)
//      de e1   st(1) <- st(0) - st(1), pop     (Intel FSUBRP)
//      de ea   st(2) <- st(2) - st(0), pop     (Intel FSUBP)
//
//  i.e. the reg field of the modrm byte selects the OPERATION (4 = the
//  reversed form, 5 = the plain one) and the rm field selects st(i).  The
//  three encodings this function uses are `de f1` (x6), `de ea` (x2) and
//  `de c2` (x2), and every sign in region F was derived from the measured
//  semantics and then cross-checked against the crossing algebra of the
//  plane in question -- e.g. for the plane y = z, t must be
//  (prev.z - prev.y) / fDen, which is what `de ea` + `fchs` produces and
//  is NOT what the swapped reading produces.  objdump's `-M intel` output
//  happens to use the Intel names correctly here; its AT&T output does
//  not, and GNU `as`'s `.intel_syntax` accepts the swapped spellings.
//
//  ------------------------------------------------------------------- //
//  4. UNDECODED
//  ------------------------------------------------------------------- //
//
//  NO CODE IS UNDECODED.  Every byte of 0x10049bb0..0x1004b5bb is covered
//  by a region above:
//
//      0x10049bb0 .. 0x10049bb3   the /Qip stack entry            (4)
//      0x10049bb4 .. 0x10049c00   region A                       (77)
//      0x10049c01 .. 0x10049cbc   region B                      (188)
//      0x10049cbd .. 0x10049d48   region C                      (140)
//      0x10049d49 .. 0x10049dfc   region D                      (180)
//      0x10049dfd .. 0x10049e50   region E                       (84)
//      0x10049e51 .. 0x1004b3ac   region F                    (5,468)
//      0x1004b3ad .. 0x1004b4c8   region G                      (284)
//      0x1004b4c9 .. 0x1004b4d1   the epilogue                    (9)
//      0x1004b4d2 .. 0x1004b4d5   the two reject trampolines       (4)
//      0x1004b4d6 .. 0x1004b5b8   the six sunk epsilon arms     (227)
//      0x1004b5b9 .. 0x1004b5bb   one lea-nop pad                 (3)
//                                                              ------
//                                                               6,668
//
//  TWO NON-CODE GAPS ARE MARKED, and both are stated rather than guessed:
//
//  // UNDECODED: 0x100a9ee8 (0.0f) and 0x100a9eec (1.0f), 8 bytes of
//  //            .rdata.  Neither is loaded anywhere in
//  //            0x10001000..0x10085000 -- checked by scanning the whole
//  //            .text disassembly for both addresses -- and no statement
//  //            written above accounts for either.  They sit between
//  //            0x10048180's last division pair and the two dead 1.0f
//  //            that region G's two divides claim, so the most likely
//  //            reading is a NINTH division somewhere in the original
//  //            source of this function that icl folded away entirely
//  //            (the pool carries literals icl later folded and
//  //            never loads) together with its 0.0f companion.  Nothing
//  //            in the bytes decides it, and inventing a statement to
//  //            produce them would be worse than this note.  The same
//  //            residue, minus the 0.0f, is open in big4e3e4.cpp,
//  //            big051a0.cpp, big3c40.cpp, big299a8.cpp, big40e10.cpp and
//  //            big28030.cpp; the division rule accounts for all
//  //            but this one slot.
//
//  // UNDECODED: -0x60(%ebp) .. -0x04(%ebp), 92 bytes of FRAME.  Not one
//  //            instruction in the 6,668 bytes references any address in
//  //            that range (verified by extracting every `ebp-0xNN`
//  //            operand in the function: the fourteen live scalars are
//  //            -0x64, -0x68, -0x70, -0x78, -0x80, -0x88, -0x90, -0x98,
//  //            -0x9c, -0xa0, -0xa4, -0xa8, -0xac and -0xc0, and there is
//  //            nothing above -0x64).  It is 23 dwords, or 11 eight-byte
//  //            slots plus a dword, of reserved scalar space.  The
//  //            arithmetic in 3b above is exact only because this space
//  //            is counted as scalars; if it is instead a local this
//  //            decode has missed, that local is never read and never
//  //            written, so no statement can be recovered for it.
//  //            0x1004e3e4's frame has no such hole (its scalars run
//  //            -0x70..-0x0c and all are used), so this is not a
//  //            property of the family.
//
//  ------------------------------------------------------------------- //
//  5. WHAT COULD NOT BE PINNED
//  ------------------------------------------------------------------- //
//
//  1. `this`.  The caller sets %ecx at 0x10049aaf and the body never reads
//     it.  Nothing in these bytes says which class this member belongs to
//     beyond big4b720.cpp's and big48180.cpp's naming, and nothing in
//     these bytes could.
//
//  2. THE /Qip STACK ENTRY'S DEAD REGISTER COPY.  0x10049bb0's
//     `mov 0x4(%esp),%eax` puts the first argument in %eax and the
//     register entry clobbers %eax two instructions later.  Both entries
//     take the same two stack arguments and both `ret $0x8`; what the
//     register copy was FOR cannot be read off these four bytes.  It is
//     decoded as the /Qip pair because that is icl's known
//     two-entry shape, because it falls through rather than branching, and
//     because the only caller is same-TU and enters at +4.
//
//  3. THE STORAGE CLASS OF fT.  All six are spilled to 8-byte slots
//     (`fstpl` to write, `fmull` to read), which is how icl spills the x87
//     stack; the standalone sibling 0x10002d40 keeps the same value in a
//     register and d3dr3a.cpp calls it `float`.  `float` is written here
//     for that reason.  `double` is the alternative, it costs no calls
//     either way, and neither spelling can be wrong about the structure.
//
//  4. THE VALUES OF THE GLOBALS.  g_f1008b908, g_f1008b910, g_f1008ee84
//     and g_b1008eea0 all live in the image's uninitialised data, so no
//     constant can be dumped for any of them.  What CAN be said is where
//     two of them come from: g_f1008ee84 is `ModelShadowOffset` + 4 and
//     g_b1008eea0 is `ModelShadowAlpha` + 0, both console variables
//     constructed at 0x1004b670 with defaults 0.05f and 230.0f -- and 230
//     is a plausible byte alpha, which is the only corroboration
//     available.  Likewise the contents of g_1008b880 beyond "the object
//     both transform helpers index" (+0x15c for 0x1000d9b0's 3x4,
//     +0x31c..+0x330 for 0x1004b5c0's projection).
//
//  5. THE WIDTH OF g_b1008eea0.  See the declaration: a `uint8` global and
//     a `uint32`/`int32` global read with a `(uint8)` cast produce the
//     same `mov mem,%dl`.  The CConVar layout read off 0x10021f40 says
//     +0x00 is a dword the constructor zeroes, which slightly favours the
//     cast; the access width says byte.  Nothing decides it and it is
//     structurally irrelevant.
//
//  6. Item49bb0's +0x00..+0x27.  Only +0x28 is reached (through
//     0x1004b630, which this file does not define), so the pad is named by
//     its size only, carried over from big4b720.cpp.
//
//  ------------------------------------------------------------------- //
//  6. GENUINE COIN-FLIPS.  Both spellings written out so one build settles
//     each.  NONE of them moves the call sequence.
//  ------------------------------------------------------------------- //
//
//  A. THE EPSILON TEST.  Written, following d3dr3a.cpp, big6d910.cpp and
//     big4e3e4.cpp, as the short-circuit
//
//         if (fDen < -0.00001f || fDen > 0.00001f)  fT = <divide>;
//         else                                      fT = 0.0f;
//
//     The alternative is the negated form
//
//         if (fDen > -0.00001f && fDen < 0.00001f)  fT = 0.0f;
//         else                                      fT = <divide>;
//
//     which emits the SAME instructions in the same places.  THE POOL
//     PREFERS THE FIRST, for the reason given in 3c(i): the six inline
//     `-1e-5` form one contiguous run and the six `+1e-5` another, i.e.
//     the second comparison's literal was emitted with the sunk block.
//     This is the strongest evidence available and it is not a build.
//
//  B. `pVert->m_Specular14.m_n = 0xffffffff;` IS WRITTEN LAST in region D.
//     The shipped store is at 0x10049de3, between row 3's `fadds` and row
//     3's `fstps` -- i.e. scheduled into the FPU latency of the statement
//     that follows it.  icl is free to sink or hoist a store to an address
//     it can prove distinct, so the source could equally have it FIRST,
//     beside the diffuse stores:
//
//         pVert->m_Diffuse10.m_n    = 0;
//         pVert->m_Specular14.m_n   = 0xffffffff;
//         pVert->m_Diffuse10.m_b[3] = g_b1008eea0;
//
//     The `mov $0xffffffff,%edi` that feeds it is hoisted out of the loop
//     (0x10049d5b, before the zero-trip guard) either way, so the
//     hoisting is not evidence.  0x1004e3e4 writes its specular last for
//     the same kind of reason and records the same flip; one build
//     settles it and no literal or call distinguishes them.
//
//  C. THE DIFFUSE INITIALISATION.  `m_Diffuse10.m_n = 0` followed by
//     `m_Diffuse10.m_b[3] = ...` reproduces 0x10049d6c's dword store of
//     the zero in %edx followed by 0x10049d7a's byte store at +0x13.  Four
//     separate byte stores would not (the dword store is one instruction),
//     but a dword store through the union followed by a byte store through
//     a different member is not the only spelling of that pair -- e.g.
//     `m_Diffuse10.m_n = ((uint32)g_b1008eea0) << 24;` would fold to a
//     shift and one store, which the bytes do NOT have, so that one is
//     excluded; `memset`-like spellings are not.
//
//  D. pOutStart's SCOPE.  It is declared at function scope above,
//     following big4e3e4.cpp.  The shipped frame gives it a DIFFERENT slot
//     in each pass (pass 1's is -0x70, and the other five land in other
//     passes' fT slots), which is equally consistent with a block-scoped
//     `Vert49bb0* pOutStart;` inside each CLIP49BB0_HEAD.  Neither costs a
//     call; the function-scope form is kept so this file and big4e3e4.cpp
//     stay diffable.
//
//  E. REGION D's LOOP CURSOR.  Written as `pVert = &pVerts[i];` with the
//     three matrix rows reading `pVert->m_vPos.x/.y/.z`.  The shipped code
//     reloads pVerts at the top of every turn (0x10049d60) and indexes
//     with a 0x24-stride byte offset in %ecx, which is what an indexed
//     source loop over an address-exposed pointer gives; writing
//     `pVerts[i].m_vPos.x` at each of the nine uses would also give it.
//     The indexed form is the one to write, which both of these are.
//
// *********************************************************************** //
