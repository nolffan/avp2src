// *********************************************************************** //
//
//  MODULE:   BATCH_D (.CPP)
//
//  PURPOSE:  Reconstructed d3d.ren functions -- batch D.
//
//  Compiler: Intel C/C++ 4.0 /O2, the same as the rest of src/engine/d3d.
//
//  THE ONE BIG FIND IN THIS BATCH: the LTVector and LTMatrix arithmetic in
//  d3d.ren is the SDK's own header source, verbatim.  src/LT2/sdk/inc/ltvector.h
//  and src/LT2/sdk/inc/ltmatrix.h are in this repo, and five of the functions
//  below are out-of-line copies of inline members declared there.  Every
//  operand order, every store order and every constant in the shipped bytes
//  agrees with those headers instruction for instruction:
//
//      0x10014720  MatMul            ltmatrix.h:172, stores in the header's
//                                    own order m[0][0] m[1][0] m[2][0]
//                                    m[3][0] m[0][1] ... m[3][3]
//      0x10015530  _CVector::Norm    ltvector.h:191
//      0x10015590  _CVector::Cross   ltvector.h:240 -- pinned by the SIGN of
//                                    each term (see the note on the function)
//      0x10015690  operator*(T)      ltvector.h:64
//      0x10015800  LTMatrix::op*     ltmatrix.h:801, NRVO'd into the caller's
//                                    return slot
//      0x10023330  _CVector::Mag     ltvector.h:39
//
//  They are written here as hand-rolled classes rather than by including the
//  headers because a symbol is mapped to an address through the
//  `sub_<addr>` in its name, and a template instantiation carries no such
//  name.  The bodies are copied character for character from the headers.
//
//  TWO THINGS THIS BATCH LEARNED ABOUT THE BUILD, both worth acting on:
//
//  1. d3d.ren's translation units DO NOT ALL SHARE ONE FLAG SET.  Norm
//     (0x10015530) computes its magnitude with `fstpl; call 0x1007fc98` --
//     0x1007fc98 is the CRT's double sqrt, the one that picks the exponent
//     field apart.  Mag (0x10023330) computes the SAME expression from the
//     SAME header line and emits a bare `fsqrt`.  Same source, same
//     compiler, different TU: one of them had sqrt as an intrinsic and the
//     other did not.  SETTLED since: this file is built /Os /Oi-
//     (intrinsics off, `#pragma function` is ignored by icl 4.0), which is
//     what Norm, Dist (0x100154b0), 0x1003b924's memset and 0x10021ed0's
//     strlen all need; Mag lives in d3dbatchd_o2.cpp (/O2, intrinsics on).
//
//  2. AT LEAST ONE TU WAS BUILT WITH /QIfist (icl's fistp-for-casts flag).
//     0x10015190 is `flds arg; fistpl local; mov local,%eax`: a float-to-int
//     conversion done with the CURRENT rounding mode and no control-word
//     dance, which no /O2 C cast produces.  It is a Round() helper with 21
//     callers, and 0x10023380 (not written here) INLINES it five
//     times -- each instance getting its own float spill slot and its own
//     int slot, which is exactly what inlining `return (int32)f;` looks
//     like.  0x10015190 is written below with __asm, which reproduces under
//     any flags; 0x10023380 cannot be written without the flag.
//
//  THREE OF THE TWENTY-FIVE ARE NOT WRITTEN, and none of them for want of
//  a decode:
//
//    0x10023380  304 bytes, 12 callers.  A vertex LERP: two floats at +0x18
//                and +0x1c interpolated normally, then five bytes (+0x12,
//                +0x11, +0x10, +0x13, +0x17 -- R, G, B, A and one more)
//                each `fstps temp; flds temp; fistpl n; mov n,%al`.  That is
//                0x10015190 INLINED five times, and it needs the /QIfist
//                build this file's header note describes.  Under the flags
//                engine.sh uses today, `(uint8)(float)` emits a control-word
//                dance and a helper call and cannot come out like this.
//                Everything else about it is decoded: it is
//                `dst->f = (b->f - a->f) * t + a->f` per field, __cdecl,
//                arguments (a, b, dst, t).
//
//    0x10073c08  492 bytes, and 0x1002dd6c 1380 bytes.  Both are REGISTER-ABI
//                bodies -- they take their arguments in %eax/%edx/%ecx and
//                never touch the stack for them -- so they are the far side
//                of one of icl's own internal calling sequences, like the
//                pair d3dmid.cpp records at 0x10028030/0x1002803c.  No C
//                function declaration produces that entry, and writing the
//                body would only reproduce if icl chose the same registers
//                for reasons this file cannot control.
//
//  Naming: `sub_<address>` throughout, so the address can be read
//  out of the symbol.  Anything named `Sub<hex>` or `C<hex>` is a type
//  invented here for a class whose real name is not known.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>
#include <string.h>
#include <ctype.h>


// ---------------------------------------------------------------------- //
//  Externals this batch reaches.
// ---------------------------------------------------------------------- //

// 0x10021d10 -- the renderer's one-argument warning printer (d3dblit.cpp and
// d3ddraw.cpp already name it).
extern void sub_10021d10(const char* pszMsg, ...);  // 0x10021d10 (was d3d_Warn/d3d_Print; icl treats a callee named *Warn* as cold)


// The Direct3D 7 device, called the way d3d.ren calls it: an explicit vtable
// pointer, `this` pushed as the first argument, __stdcall.  Slot +0x50 is
// SetRenderState and +0x8c is SetTexture -- +0x8c is index 35, which lands
// on SetTexture with the same count that puts BeginScene at +0x14 and
// SetTextureStageState at +0x94, both already pinned in d3ddraw.cpp.  Both
// call sites here confirm the arity from the other side: three pushed
// dwords and the callee pops all three (the frame is only restored by the
// following `mov (%esp),%ebx` if the callee cleaned up).
struct D3DDev7;

struct D3DDev7Vtbl
{
	void* _slots00_4c[20];                              // +0x00..+0x4c
	long (__stdcall* SetRenderState)(D3DDev7*, uint32, uint32);   // +0x50
	void* _slots54_88[14];                              // +0x54..+0x88
	long (__stdcall* SetTexture)(D3DDev7*, uint32, void*);        // +0x8c
};

struct D3DDev7
{
	D3DDev7Vtbl* lpVtbl;
};

extern D3DDev7* g_pD3DDevice;                           // 0x1008d988


// 0x1008e684 -- the texture currently bound to each stage.  Two functions
// here share it: 0x1000efa0 writes it after a successful SetTexture and
// 0x100232b0 clears it, and both index it by the stage number.
extern void* g_pCurTexture[4];


// ---------------------------------------------------------------------- //
//  The vertex-buffer guard and its neighbour.
// ---------------------------------------------------------------------- //

// 0x1000a740 -- 38 bytes, 8 callers.  `cmpl $0x80,arg; jg` is a SIGNED
// compare, so the count is int32, and the taken arm is the complaint, so the
// guard is written the way the branch falls: the error returns early and the
// good answer is the fall-through `xor %eax,%eax; inc %eax`.  The string at
// 0x100a45a0 is "Error: vertex buffer overflow" and 0x10021d10 is the
// one-argument warning printer, called with a 0xc-byte outgoing area.
LTBOOL sub_1000a740(int32 nVerts)
{
	if (nVerts > 128)
	{
		sub_10021d10("Error: vertex buffer overflow");
		return LTFALSE;
	}

	return LTTRUE;
}


// 0x1000a700 -- 49 bytes, 10 callers.  Hands back where a piece of geometry's
// index data lives and how many entries it has, choosing between two
// representations on a global flag: either an out-of-line pointer at +0x4c
// with the count at +0x52, or an array embedded in the object at +0x54 with
// the count at +0x50.  `lea 0x54(%eax)` is what makes the second one
// embedded rather than another pointer, and `movzwl` makes both counts
// uint16 widened into a uint32 out-parameter.
//
// The if/else (rather than two guarded returns) is what puts the embedded
// case behind the `je` with both arms ending in their own `ret`.
struct Sub1000a700Geom
{
	uint8  _pad00[0x4c];
	void*  m_pIndices;                                  // +0x4c
	uint16 m_nEmbeddedCount;                            // +0x50
	uint16 m_nIndexCount;                               // +0x52
	uint8  m_Embedded[4];                               // +0x54
};

extern LTBOOL g_bUseIndexPointer;                       // 0x1008cb84

void sub_1000a700(Sub1000a700Geom* pGeom, void** ppOutData, uint32* pnOutCount)
{
	if (g_bUseIndexPointer)
	{
		*ppOutData  = pGeom->m_pIndices;
		*pnOutCount = pGeom->m_nIndexCount;
	}
	else
	{
		*ppOutData  = pGeom->m_Embedded;
		*pnOutCount = pGeom->m_nEmbeddedCount;
	}
}


// ---------------------------------------------------------------------- //
//  The allocate-array pair.  ONE SOURCE, TWO ELEMENT SIZES.
// ---------------------------------------------------------------------- //
//
//  0x1000a0e0 and 0x1001c450 are the same 40 bytes twice; the only
//  difference is `shl $0x4` against `shl $0x6`, so they are one template (or
//  one macro) at sizeof 0x10 and sizeof 0x40.  d3dhigh.cpp already found the
//  same thing for the bsearch family, and 0x1001c480 -- the 0x40-wide member
//  of THAT family -- sits immediately after 0x1001c450, which says the two
//  families belong to the same container class.
//
//  The allocation is a C++ virtual call, not a COM one: `this` stays in
//  %ecx across `call *%eax` and is never pushed, and the slot is at +0x04.
//
//  The `test %eax,%eax; je` that answers the same pointer either way is in
//  the source -- a plain `return p` emits no test at all.  Both arms share
//  one epilogue through %edx, which is icl tail-merging the two six-byte
//  `mov %ebp,%esp; pop %ebp; ret $8` sequences.
class Sub1000a0e0Alloc
{
public:
	virtual void  _Slot0();
	virtual void* Alloc(uint32 nSize);                  // vtable +0x04
};


// 0x1000a0e0 -- element size 0x10.
class C1000a0e0
{
public:
	void* sub_1000a0e0(uint32 nCount, Sub1000a0e0Alloc* pAlloc);

	uint32 m_nUnknown00;                                // +0x00, never read
};

// 0x1000a0e0 moved to d3dbatchd_o2.cpp: its shipped padding (41 bytes
// then a 7-byte lea-nop to a 16-byte boundary) is /O2 alignment.


// 0x1001c450 -- element size 0x40.  Byte for byte 0x1000a0e0 with a wider
// element.
class C1001c450
{
public:
	void* sub_1001c450(uint32 nCount, Sub1000a0e0Alloc* pAlloc);

	uint32 m_nUnknown00;                                // +0x00, never read
};

void* C1001c450::sub_1001c450(uint32 nCount, Sub1000a0e0Alloc* pAlloc)
{
	void* pMem = pAlloc->Alloc(nCount * 0x40);
	void* pRet;

	if (pMem)
		pRet = pMem;
	else
		pRet = LTNULL;

	return pRet;
}


// ---------------------------------------------------------------------- //
//  _CVector<float>, out of line.  Bodies copied from src/LT2/sdk/inc/ltvector.h.
// ---------------------------------------------------------------------- //
//
//  This is the SDK's `_CVector<T>` at T=float, with the members renamed to
//  their addresses so they can be placed.  The class shape matters as
//  much as the bodies: the do-nothing default constructor is declared in the
//  header too, and Cross takes its vector BY VALUE (`ret $0x10` pops the
//  return-slot pointer plus twelve bytes of vector, and `operator*(T)`'s
//  `ret $0x8` pops the pointer plus one float).
//
//  All three value-returning members build the answer in a 16-byte local and
//  then `rep movsl` three dwords into the caller's slot, which is what
//  `return _CVector<T>(a, b, c)` looks like when the temporary is not
//  elided.
class LTVec
{
public:
			LTVec() {}
			LTVec(float mx, float my, float mz) {x=mx; y=my; z=mz;}

	void	sub_10015530(float nVal);                   // Norm
	LTVec	sub_10015590(const LTVec v) const;          // Cross
	LTVec	sub_10015690(float v) const;                // operator * (T)
	float	sub_10023330() const;                       // Mag

	float	x;
	float	y;
	float	z;
};


// 0x10015530 -- 92 bytes, 7 callers.  _CVector<T>::Norm, ltvector.h:191,
// with Mag() inlined into it.
//
// WHAT PINS IT: the magnitude leaves the x87 stack as a DOUBLE (`fstpl`) and
// goes to 0x1007fc98, which is the CRT's `sqrt` -- so the source really is
// `(T)sqrt(...)` and not an intrinsic here.  The zero test loads 0.0f from
// 0x100a49b4 and the divide is `fdivrs nVal` (memory over stack), i.e.
// nVal/mag and not mag/nVal.  The early return has to `fstp %st(0)` to drop
// the magnitude the divide would otherwise have consumed, which is why that
// arm is three instructions instead of a bare jump.
void LTVec::sub_10015530(float nVal)
{
	float inv;
	float mag = (float)sqrt((x * x) + (y * y) + (z * z));

	if (mag == 0.0f)
		return;

	inv = nVal / mag;
	x = x * inv;
	y = y * inv;
	z = z * inv;
}


// 0x10015590 -- 94 bytes, 4 callers.  _CVector<T>::Cross, ltvector.h:240.
//
// WHAT PINS IT: the SIGN of every term.  objdump prints DE E9 as
// `fsubrp %st,%st(1)`, but DE E9 is Intel's FSUBP -- st(1) = st(1) - st(0).
// Read that way the three results are (v.y*z - v.z*y), (v.z*x - v.x*z) and
// (v.x*y - v.y*x), which is the header's order and NOT the other sign.  The
// operand order inside each product is scheduling, not source: the first
// term is `flds z; fmuls v.y` while the header writes `v.y * z`.
LTVec LTVec::sub_10015590(const LTVec v) const
{
	return LTVec(
					((v.y * z) - (v.z * y)),
				   ((v.z * x) - (v.x * z)),
				   ((v.x * y) - (v.y * x)) 
					);
}


// 0x10015690 -- 68 bytes, 49 callers, the most-called function in the batch.
// _CVector<T>::operator*(T), ltvector.h:64.
//
// WHAT PINS IT: `ret $0x8` (return slot + one float) and the three products
// landing at +0x4, +0x8, +0xc of the temporary in x, y, z order.
LTVec LTVec::sub_10015690(float v) const
{
	return LTVec(x * v, y * v, z * v);
}


// 0x10015190 -- 17 bytes, 21 callers.  Round-to-nearest float-to-int.
//
// `flds 0x8(%ebp); fistpl -0x4(%ebp); mov -0x4(%ebp),%eax` uses the CURRENT
// rounding mode; a C cast truncates and would have to save, edit and reload
// the control word first.  Two ways to get this shape, and only one of them
// works under the flags this project builds with:
//
//   * `return (int32)f;` compiled /QIfist.  That is almost certainly what
//     Monolith wrote -- 0x10023380 inlines this same helper five times, and
//     a function containing __asm cannot be inlined.
//   * the __asm below, which emits it under any flags.
//
// The __asm is also what forces the `push %ebp; mov %esp,%ebp` frame and the
// `mov %ebp,%esp` epilogue, both of which the shipped bytes have; the local
// is the 4-byte slot icl opens with `push %esi`.
int32 sub_10015190(float f)
{
	int32 n;

	__asm fld   f
	__asm fistp n

	return n;
}


// ---------------------------------------------------------------------- //
//  LTMatrix multiply, out of line.  Bodies from src/LT2/sdk/inc/ltmatrix.h.
// ---------------------------------------------------------------------- //

// A stand-in for LTMatrix -- same single member, same layout.  (LTMatrix
// itself cannot be used: its operator* would mangle without an address in
// the name and could not be placed.)
class LTMat
{
public:
	LTMat	sub_10015800(LTMat &other);                 // operator *

	float	m[4][4];
};


// 0x10014720 -- 388 bytes, 5 callers.  MatMul, ltmatrix.h:172, emitted out
// of line because it is far too big to inline at the one call site below.
//
// WHAT PINS IT: the store order.  The sixteen results land at +0x00, +0x10,
// +0x20, +0x30, +0x04, +0x14, +0x24, +0x34, +0x08 ... +0x3c -- that is
// m[0][0] m[1][0] m[2][0] m[3][0] m[0][1] ..., column by column, which is
// the header's source order and nothing a scheduler would invent.  Inside
// each result the four products accumulate LAST TERM FIRST
// (a.m03*b.m30 first, a.m00*b.m00 last); icl reassociates the sum, and it
// does the same thing to _CVector::Mag two functions down.
//
// The signature is `(pDest, pMat1, pMat2)` read straight off the argument
// slots: %ecx = 0x4(%esp) is the destination, %edx = 0x8(%esp) is the left
// operand (its rows are indexed) and %eax = 0xc(%esp) the right.
void sub_10014720(LTMat* pDest, LTMat* pMat1, LTMat* pMat2)
{
	pDest->m[0][0] = pMat1->m[0][0]*pMat2->m[0][0] + pMat1->m[0][1]*pMat2->m[1][0] + pMat1->m[0][2]*pMat2->m[2][0] + pMat1->m[0][3]*pMat2->m[3][0];
	pDest->m[1][0] = pMat1->m[1][0]*pMat2->m[0][0] + pMat1->m[1][1]*pMat2->m[1][0] + pMat1->m[1][2]*pMat2->m[2][0] + pMat1->m[1][3]*pMat2->m[3][0];
	pDest->m[2][0] = pMat1->m[2][0]*pMat2->m[0][0] + pMat1->m[2][1]*pMat2->m[1][0] + pMat1->m[2][2]*pMat2->m[2][0] + pMat1->m[2][3]*pMat2->m[3][0];
	pDest->m[3][0] = pMat1->m[3][0]*pMat2->m[0][0] + pMat1->m[3][1]*pMat2->m[1][0] + pMat1->m[3][2]*pMat2->m[2][0] + pMat1->m[3][3]*pMat2->m[3][0];

	pDest->m[0][1] = pMat1->m[0][0]*pMat2->m[0][1] + pMat1->m[0][1]*pMat2->m[1][1] + pMat1->m[0][2]*pMat2->m[2][1] + pMat1->m[0][3]*pMat2->m[3][1];
	pDest->m[1][1] = pMat1->m[1][0]*pMat2->m[0][1] + pMat1->m[1][1]*pMat2->m[1][1] + pMat1->m[1][2]*pMat2->m[2][1] + pMat1->m[1][3]*pMat2->m[3][1];
	pDest->m[2][1] = pMat1->m[2][0]*pMat2->m[0][1] + pMat1->m[2][1]*pMat2->m[1][1] + pMat1->m[2][2]*pMat2->m[2][1] + pMat1->m[2][3]*pMat2->m[3][1];
	pDest->m[3][1] = pMat1->m[3][0]*pMat2->m[0][1] + pMat1->m[3][1]*pMat2->m[1][1] + pMat1->m[3][2]*pMat2->m[2][1] + pMat1->m[3][3]*pMat2->m[3][1];

	pDest->m[0][2] = pMat1->m[0][0]*pMat2->m[0][2] + pMat1->m[0][1]*pMat2->m[1][2] + pMat1->m[0][2]*pMat2->m[2][2] + pMat1->m[0][3]*pMat2->m[3][2];
	pDest->m[1][2] = pMat1->m[1][0]*pMat2->m[0][2] + pMat1->m[1][1]*pMat2->m[1][2] + pMat1->m[1][2]*pMat2->m[2][2] + pMat1->m[1][3]*pMat2->m[3][2];
	pDest->m[2][2] = pMat1->m[2][0]*pMat2->m[0][2] + pMat1->m[2][1]*pMat2->m[1][2] + pMat1->m[2][2]*pMat2->m[2][2] + pMat1->m[2][3]*pMat2->m[3][2];
	pDest->m[3][2] = pMat1->m[3][0]*pMat2->m[0][2] + pMat1->m[3][1]*pMat2->m[1][2] + pMat1->m[3][2]*pMat2->m[2][2] + pMat1->m[3][3]*pMat2->m[3][2];

	pDest->m[0][3] = pMat1->m[0][0]*pMat2->m[0][3] + pMat1->m[0][1]*pMat2->m[1][3] + pMat1->m[0][2]*pMat2->m[2][3] + pMat1->m[0][3]*pMat2->m[3][3];
	pDest->m[1][3] = pMat1->m[1][0]*pMat2->m[0][3] + pMat1->m[1][1]*pMat2->m[1][3] + pMat1->m[1][2]*pMat2->m[2][3] + pMat1->m[1][3]*pMat2->m[3][3];
	pDest->m[2][3] = pMat1->m[2][0]*pMat2->m[0][3] + pMat1->m[2][1]*pMat2->m[1][3] + pMat1->m[2][2]*pMat2->m[2][3] + pMat1->m[2][3]*pMat2->m[3][3];
	pDest->m[3][3] = pMat1->m[3][0]*pMat2->m[0][3] + pMat1->m[3][1]*pMat2->m[1][3] + pMat1->m[3][2]*pMat2->m[2][3] + pMat1->m[3][3]*pMat2->m[3][3];
}


// 0x10015800 -- 37 bytes, 14 callers.  LTMatrix::operator*, ltmatrix.h:801.
//
// WHAT PINS IT: `ret $0x8` is a __thiscall with a hidden return-slot
// pointer, and the pointer the caller supplied is handed STRAIGHT to MatMul
// as pDest -- `mRet` was elided into the return slot -- then answered in
// %eax.  The three arguments go into a preallocated 0x18-byte outgoing area
// as (return slot, this, &other), which is the order the source writes them.
LTMat LTMat::sub_10015800(LTMat &other)
{
	LTMat mRet;

	sub_10014720(&mRet, this, &other);
	return mRet;
}


// ---------------------------------------------------------------------- //
//  Thin __thiscall members.
// ---------------------------------------------------------------------- //
//
//  Four functions here do nothing but hand their argument to another
//  __thiscall member of the same object.  They all carry the same icl
//  signature -- `mov %ecx,%eax; ...; mov %eax,%ecx`, a this-pointer round
//  trip through %eax that survives even when the offset added to it is zero
//  -- and they all put the outgoing arguments at the BOTTOM of the frame,
//  in the slot icl opened with a dead `push %edi` or an `add $-8,%esp`.

// 0x1001dc30 -- 17 bytes, 3 callers.  `&m_pData[nIndex]`, with the element
// size fixed at TWO by `lea (%ecx,%eax,2)`; 0x1001e0a0 next door is the
// identical function at element size four.
class Sub1001dc30Array
{
public:
	uint16*	sub_1001dc30(uint32 nIndex);

	uint32	m_n00;
	uint16*	m_pData;                                    // +0x04
};

uint16* Sub1001dc30Array::sub_1001dc30(uint32 nIndex)
{
	return m_pData + nIndex;
}


// 0x1005a3c0 -- 28 bytes, 4 callers.  One argument, forwarded unchanged.
class C1005a3c0
{
public:
	void	sub_1005a3c0(uint32 nArg);
	void	sub_1007ee40(uint32 nArg);
};

void C1005a3c0::sub_1005a3c0(uint32 nArg)
{
	sub_1007ee40(nArg);
}


// 0x1001e070 -- 38 bytes, 3 callers.  Forwards with a leading zero.  The
// 0x18-byte frame (`sub $0x10` then `add $-8`) is icl making room for two
// outgoing dwords.
class C1001e070
{
public:
	void	sub_1001e070(uint32 nArg);
	void	sub_1001dca0(uint32 nFlags, uint32 nArg);
};

void C1001e070::sub_1001e070(uint32 nArg)
{
	sub_1001dca0(0, nArg);
}


// 0x1007ca30 -- 38 bytes, 3 callers.  Byte for byte 0x1001e070 with a
// different callee.
class C1007ca30
{
public:
	void	sub_1007ca30(uint32 nArg);
	void	sub_1007c760(uint32 nFlags, uint32 nArg);
};

// Body in d3dbatchd_o2.cpp: shipped pads 38 bytes with lea-nops to 48, the /O2 layout (/Os pads to 40).


// 0x1007a3b0 -- 62 bytes, 4 callers.  A C++ VIRTUAL call, not a COM one:
// `this` is left in %ecx and never pushed, and the slot taken is +0x00.
// The five constants are an A8R8G8B8 channel description -- a count of 3
// followed by the alpha, red, green and blue masks -- laid into the
// outgoing area at +0x00, +0x04, +0x08, +0x0c and +0x10 in that order,
// which is the argument order.  (The STORES come out 0x10, 0x00, 0x0c,
// 0x08, 0x04; that is the scheduler, and the offsets are what count.)
class C1007a3b0
{
public:
	virtual void	Slot0(uint32 nCount, uint32 nMask0, uint32 nMask1,
					      uint32 nMask2, uint32 nMask3);

	void	sub_1007a3b0();
};

void C1007a3b0::sub_1007a3b0()
{
	Slot0(3, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
}


// ---------------------------------------------------------------------- //
//  Direct3D device state.
// ---------------------------------------------------------------------- //

// 0x1005a600 -- 40 bytes, 16 callers.  Pushes one cached render state.  The
// object is nothing but the (state, value) pair it replays, and +0x50 on the
// device is SetRenderState.
class C1005a600
{
public:
	void	sub_1005a600();

	uint32	m_nState;                                   // +0x00
	uint32	m_nValue;                                   // +0x04
};

void C1005a600::sub_1005a600()
{
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, m_nState, m_nValue);
}


// 0x100232b0 -- 72 bytes, 6 callers.  Unbinds a texture stage, but only if
// something is bound: the guard reads the same g_pCurTexture[] slot that
// 0x1000efa0 writes, and clearing the slot is the second half of the same
// `if`.  __cdecl -- the stage arrives on the stack and nothing is popped.


// 0x1000efa0 -- 124 bytes, 3 callers.  Binds a texture to a stage and counts
// it against this frame's texture budget.
//
// WHAT PINS IT: three guarded returns sharing one `xor %eax,%eax; inc %eax`
// -- no data at all, already bound, or not resident -- with the failure
// return sitting past the end and jumping back into the register-restore
// run.  The frame counter at 0x1008bea8 is read TWICE, once for the compare
// and once for the store, because the `+=` through g_pStats could have
// aliased it; that reload is in the shipped bytes and falls out of writing
// the two statements in the obvious order.  It is a uint16 widened into a
// uint32 field (`movzwl` into a full-width compare).
struct Sub1000efa0Data
{
	uint8	_pad00[0x14];
	uint32	m_nBytes;                                   // +0x14
	uint32	m_nFrameCode;                               // +0x18
	void*	m_pD3DTexture;                              // +0x1c
	uint32	m_nResident;                                // +0x20
};

struct Sub1000efa0Texture
{
	uint8	_pad00[0x44];
	Sub1000efa0Data* m_pData;                           // +0x44
};

struct Sub1000efa0Stats
{
	uint8	_pad00[0x4c];
	uint32	m_nTextureBytes;                            // +0x4c
};

extern uint16 g_nFrameCode;                             // 0x1008bea8
extern Sub1000efa0Stats* g_pStats;                      // 0x1008cbcc

LTBOOL sub_1000efa0(Sub1000efa0Texture* pTexture, uint32 nStage)
{
	Sub1000efa0Data* pData = pTexture->m_pData;

	if (!pData)
		return LTFALSE;

	if (pData == g_pCurTexture[nStage] || !pData->m_nResident)
		return LTTRUE;

	if (pData->m_nFrameCode != g_nFrameCode)
	{
		g_pStats->m_nTextureBytes += pData->m_nBytes;
		pData->m_nFrameCode = g_nFrameCode;
	}

	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, nStage,
	                                 pData->m_pD3DTexture);
	g_pCurTexture[nStage] = pData;

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  Run-length decode into a 1024-entry uint16 buffer.
// ---------------------------------------------------------------------- //

// 0x1006a790 -- 111 bytes, 5 callers.  Reads (uint16 value, optional uint8
// count) pairs and expands them.  Top bit of the value set means a count
// byte follows and the value's low 15 bits are the datum; clear means a
// single copy of the value as it stands.
//
// WHAT PINS IT: the budget.  %esi starts at 0x400 and every run subtracts
// its count, and the destination advances by TWO per element, so the
// destination is 1024 uint16s.  The test is `test %esi,%esi; jl`, signed, so
// the budget is int32 and the overrun answer is the `xor %eax,%eax` at the
// far end; the good answer is `xor %eax,%eax; inc %eax` reached by falling
// out of the loop.  The count is re-widened with `movzbl %cl` on every pass
// of the inner loop, which makes it a uint8 variable being decremented and
// not a loop counter.
//
// Both loops are the rotated form -- a guard before the body and the test at
// the bottom -- which is what `while` compiles to and what `do`/`while`
// would NOT (it would drop the guard).
//
// SIZE-EXACT NEAR (111 of 111, 9 bytes differ).  The prologue's extra copies
// (`mov %edi,%ebp` ... `mov %edx,%edi`) are the walk pointer being a COPY of
// pSrc declared before pEnd; the `movzwl ...,%ebx; mov %ebx,%edx` pair is the
// value read first and a raw copy taken for the test.  The one residual is
// inside the count branch: shipped `mov %edx,%ebx; and $0x7fff,%ebx; inc
// %ebp`, ours `and $0x7fff,%edx; inc %ebp; mov %edx,%ebx`.  Measured without
// effect: all six declaration orders at loop and function scope, uint16 /
// uint32 / int32 for the raw copy, `&= 0x7fff` on a fresh copy, `& ~0x8000`,
// a (uint16) cast, the and before/after the count read and the increment,
// and the flag sets /Os, /Os /Oi-, /O1, /Os /Qip, /Os /Qrcd, /Os /Ob0 (/O2
// is 144 bytes).
// Also measured, all worse: the count branch as a switch (61
// bytes differ), the count read through a saved pointer after the and
// (64), `nCount = 1` before an if with no else (68), a uint32/int32 value
// variable (122), uint32 raw with uint32 value (66).
// Unchanged at 9 -- the raw copy taken before `pCur += 2`,
// `pCur += sizeof(uint16)`; worse -- `nValue = nRaw = *p` / `nRaw = nValue
// = *p` / the raw read first and copied to the value (11), `nCount =
// *pCur++` (59), the value assigned in the else arm (25 / 28).
LTBOOL sub_1006a790(uint8* pSrc, uint32 nSize, uint16* pDest)
{
	uint8* pCur = pSrc;
	uint8* pEnd = pSrc + nSize;
	int32  nLeft = 1024;

	while (pCur < pEnd)
	{
		uint16 nRaw;
		uint16 nValue;
		uint8  nCount;

		nValue = *(uint16*)pCur;
		pCur += 2;
		nRaw = nValue;

		if (nRaw & 0x8000)
		{
			nCount = *pCur;
			nValue = nRaw & 0x7fff;
			++pCur;
		}
		else
		{
			nCount = 1;
		}

		nLeft -= nCount;
		if (nLeft < 0)
			return LTFALSE;

		while (nCount)
		{
			--nCount;
			*pDest = nValue;
			++pDest;
		}
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  Position + rotation + scale -> transform matrix.
// ---------------------------------------------------------------------- //

// 0x100151b0 -- 362 bytes, 4 callers.  Sanitises a quaternion, turns it into
// a matrix, scales the three basis columns and drops the translation in.
//
// WHAT PINS IT: the four range checks.  Each is a pair of constants --
// 100000.0f at 0x100a4914/0x91c/0x924/0x92c and -100000.0f at
// 0x100a4940/0x93c/0x938/0x934 -- and the replacement is 0.0f for the first
// three components and 1.0f for the fourth, which is what identifies the
// argument as a quaternion rather than a vector.  Each `||`'s second test is
// laid out COLD, past the end of the function and in reverse order, and
// jumps back into the body; that is the shape of `a > K || a < -K` with the
// zeroing as the guarded statement.
//
// The scaling runs down the columns -- m[0][0] m[1][0] m[2][0] then
// m[0][1] m[1][1] m[2][1] -- and the translation is copied with INTEGER
// moves, three plain dword loads and stores, which is what a float-to-float
// assignment with no arithmetic in it comes out as.
struct Sub100151b0Rot
{
	float	x;
	float	y;
	float	z;
	float	w;
};

// 0x10073400 -- quaternion to matrix; __cdecl, (rotation, matrix).
extern void sub_10073400(Sub100151b0Rot* pRot, LTMat* pMat);

void sub_100151b0(LTVec* pPos, Sub100151b0Rot* pRot, LTVec* pScale,
                  LTMat* pMat)
{
	if (pRot->x > 100000.0f || pRot->x < -100000.0f)
		pRot->x = 0.0f;

	if (pRot->y > 100000.0f || pRot->y < -100000.0f)
		pRot->y = 0.0f;

	if (pRot->z > 100000.0f || pRot->z < -100000.0f)
		pRot->z = 0.0f;

	if (pRot->w > 100000.0f || pRot->w < -100000.0f)
		pRot->w = 1.0f;

	sub_10073400(pRot, pMat);

	pMat->m[0][0] *= pScale->x;
	pMat->m[1][0] *= pScale->x;
	pMat->m[2][0] *= pScale->x;

	pMat->m[0][1] *= pScale->y;
	pMat->m[1][1] *= pScale->y;
	pMat->m[2][1] *= pScale->y;

	pMat->m[0][2] *= pScale->z;
	pMat->m[1][2] *= pScale->z;
	pMat->m[2][2] *= pScale->z;

	pMat->m[0][3] = pPos->x;
	pMat->m[1][3] = pPos->y;
	pMat->m[2][3] = pPos->z;
}


// ---------------------------------------------------------------------- //
//  A constructor.  THE LOWEST-CONFIDENCE ENTRY IN THIS FILE.
// ---------------------------------------------------------------------- //

// 0x1003abe4 -- 69 bytes, 3 callers.  Installs the vtable at 0x100894f0,
// ties an embedded list head off against itself and clears the rest.
//
// The DECODE is not in doubt: every store is read off the bytes, `lea
// 0x1c(%ecx)` proves the list head is embedded rather than a pointer, and
// 0x3f800000 at +0x34 is 1.0f.  The ORDER is the risk.  icl scheduled the
// two immediate stores (the vtable and the 1.0f) to the front and pushed
// +0x30 and +0x44 to the back, so the emitted order is not the source order
// and only two things read back cleanly:
//
//   * +0x20 stored BEFORE +0x1c, which is `m_pPrev = m_pNext = this-link`
//     evaluated right to left -- LithTech's dl_TieOff exactly;
//   * +0x2a stored BEFORE +0x28, the same chained-assignment shape.
//
// Everything else below is declaration order and a guess.  If this one
// misses, it is the one to drop.
//
// The vptr is an explicit member store of the shipped vtable at 0x100894f0
// (five slots, generated into d3ddata.cpp), as sub_1003b924 below does,
// rather than a `virtual` declaration: that made icl emit and install a
// one-slot vtable of its own, which a linked d3d.ren would then run.
extern void* g_vtbl_100894f0[];                         // 0x100894f0

class C1003abe4
{
public:
			C1003abe4();

	void**	m_pVtbl;                                    // +0x00

	uint8	_pad04[0x18];                               // +0x04..+0x1b
	void*	m_pPrev;                                    // +0x1c
	void*	m_pNext;                                    // +0x20
	uint32	_pad24;                                     // +0x24
	uint16	m_n28;                                      // +0x28
	uint16	m_n2a;                                      // +0x2a
	uint32	m_n2c;                                      // +0x2c
	uint32	m_n30;                                      // +0x30
	float	m_f34;                                      // +0x34
	uint32	_pad38[2];                                  // +0x38..+0x3f
	uint16	m_n40;                                      // +0x40
	uint16	m_n42;                                      // +0x42
	uint16	m_n44;                                      // +0x44
	uint8	m_b46;                                      // +0x46
	uint8	m_b47;                                      // +0x47
	uint8	m_b48;                                      // +0x48
	uint8	m_b49;                                      // +0x49
};

C1003abe4::C1003abe4()
{
	m_pVtbl = g_vtbl_100894f0;
	m_pPrev = m_pNext = (void*)&m_pPrev;
	m_n28 = m_n2a = 0;
	m_n2c = 0;
	m_f34 = 1.0f;
	m_n42 = m_n40 = 0;
	m_b46 = 0;
	m_b47 = 0;
	m_b48 = 0;
	m_b49 = 0;
	m_n30 = 0;
	m_n44 = 0;
}


// 0x10023330 (_CVector<float>::Mag) is in d3dbatchd_o2.cpp: the shipped
// function is padded to 16 with a lea-nop, which is /O2 alignment, and it
// emits a bare `fsqrt`, which is intrinsics ON.  Norm above calls the CRT
// sqrt, which is this file's /Os /Oi-.


// ---------------------------------------------------------------------- //
//  0x1003b924 -- 56 bytes, 1 caller.
//
//      mov  $0x2c,%eax ; add %ebx,%eax        ; icl's &this->m_a2c
//      movl $0x10089530,(%ebx)                ; the vptr
//      movl $0x0,0x4(%esp) ; mov %eax,(%esp) ; movl $0x20,0x8(%esp)
//      call 0x100809a0                        ; memset
//      movl $0x0,0x28(%ebx)
//      add  $0x18,%esp ; mov %ebx,%eax ; ret  ; answers `this`
//
//  WHAT PINS IT AS A CONSTRUCTOR: `mov %ebx,%eax` after the stack
//  adjustment with a plain `ret`, plus a vptr store ahead of everything
//  the body does.  0x10089530 is a run of code pointers in .data
//  (0x1003bb10, 0x1003d350, 0x1003d5c0, 0x1003d5f0, ...), i.e. a vtable,
//  so the class is polymorphic; four slots are declared to give it one.
//  0x100809a0 is the CRT memset (it splices the fill byte to a dword and
//  aligns before the rep).
//
//  SOURCE ORDER: memset first, then +0x28 = 0.  The vptr store is the
//  compiler's and comes first regardless.
//
//  The vptr is written as an ORDINARY MEMBER assignment rather than
//  by declaring four virtual functions, the same way d3dbatchb.cpp's
//  Obj1007dcc4 handles its two vptrs: an explicit store at the top of
//  the body lands in exactly the position the compiler's own store
//  would, and costs no undefined vtable.
//
//  FLAGS: the shipped build CALLS memset for a constant 32-byte clear, so
//  intrinsics are off, and it pads to 4 with `8d 76 00`, which is /Os.
//  Moved here from d3dr2d.cpp (/O2, rep stos) for this file's /Os /Oi-;
//  byte-identical over all 57 bytes.  The `mov 0x4(%esp),%eax` at
//  0x1003b920 is a /Qip stack entry in front of it (the real TU had /Qip
//  and a same-TU caller, 0x1003d910); this row is the +4 body only.
// ---------------------------------------------------------------------- //
extern uint8 g_10089530[];                              // the vtable

struct sub_1003b924
{
	uint8*  m_pVtbl;                                    // +0x00
	uint32  m_pad_04[9];                                // +0x04..+0x27
	uint32  m_n28;                                      // +0x28
	uint8   m_a2c[0x20];                                // +0x2c

	sub_1003b924();
};

sub_1003b924::sub_1003b924()
{
	m_pVtbl = g_10089530;

	memset(m_a2c, 0, sizeof(m_a2c));
	m_n28 = 0;
}


// ---------------------------------------------------------------------- //
//  0x10021ed0 -- 59 bytes, 2 callers.  Upper-casing a string in place.
//
//      call 0x10080d10 ; test %eax,%eax ; jbe END      ; strlen first
//   L: movsbl 0x0(%ebp,%ebx,1),%eax
//      call 0x10080d8b ; mov %al,0x0(%ebp,%ebx,1)
//      inc %ebx ; call 0x10080d10 ; cmp %eax,%ebx ; jb L
//
//  WHAT PINS IT: 0x10080d10 is the CRT strlen (the 0x7efefeff word-at-a-
//  time scan) and 0x10080d8b is the CRT toupper (`if 'a' <= c && c <=
//  'z' then c - 0x20`, behind the locale check at 0x10091edc).  strlen
//  is called AGAIN at the bottom of every iteration, so it really is in
//  the loop condition and not hoisted into a variable -- the shape below
//  is forced.  The load is `movsbl`, so the element is a signed char
//  widening into toupper's int; the store is `mov %al`, so the result is
//  narrowed straight back.  `jb`/`jbe` throughout: the index is
//  unsigned.
//
//  FLAGS: strlen is CALLED (intrinsics off) and the 59 bytes are followed
//  by 0x90 fill (/Os).  Moved here from d3dr2d.cpp, where /O2 expanded
//  strlen inline twice (112 bytes); /Os /Oi- is byte-identical.
// ---------------------------------------------------------------------- //
void sub_10021ed0(char* pStr)
{
	uint32 i;

	for (i = 0; i < strlen(pStr); ++i)
		pStr[i] = (char)toupper(pStr[i]);
}


// ---------------------------------------------------------------------- //
//  0x100154b0 -- 91 bytes, 2 callers.  ltvector.h:36, VERBATIM:
//
//      T Dist(const _CVector<T> &vec) const
//          { return (*this - vec).Mag(); }
//
//  WHAT PINS IT:
//
//    * `ret $0x4` with `this` in %ecx: ONE four-byte argument, so the
//      parameter is a reference, not the by-value vector the operator it
//      calls takes.
//    * `mov $0x3,%ecx ; rep movsl` copies twelve bytes from that
//      reference into the outgoing area at %esp+4, ahead of the return
//      slot at %esp+0 -- which is how a by-value _CVector argument is
//      passed to 0x10015640.
//    * 0x10015640 IS operator-: `ret $0x10` (return slot plus twelve
//      bytes), and it computes (%ecx)+0 minus the argument's +0, and so
//      on, so the argument is subtracted FROM `this` and not the other
//      way round.
//    * the tail is Mag inlined -- ltvector.h declares it in the class
//      body, so /Ob1 folds it -- and the x87 order is z*z, then y*y
//      added to it, then x*x added to that: x*x + (y*y + z*z), the
//      last-term-backwards association MatMul and Cross already show.
//    * `fstpl` into 0x1007fc98, the CRT's double sqrt.  That is Norm's
//      sqrt (0x10015530), not Mag's `fsqrt` (0x10023330), and this
//      function sits between them at 0x100154b0 -- so it belongs to
//      Norm's translation unit and gets NO intrinsic pragma.
//
//  Mag is the header's in-class member, and it inlines.  operator- is
//  declared out of line, because it must NOT inline -- the shipped code
//  calls it.
//
//  The difference is the callee's return slot used directly, no copy: that
//  is the header's own `(*this - vec).Mag()` on the temporary.  A NAMED
//  local (`VecDist d = ...; return sqrt(d.x*d.x ...)`) makes icl copy the
//  temporary with a second `rep movsl` and grow the frame to 0x30 (108
//  bytes).  Moved here from d3dr2d_os.cpp (/Os, intrinsics on): /Os /Oi-
//  with the header spelling is byte-identical.
// ---------------------------------------------------------------------- //
class VecDist
{
public:
			VecDist() {}
			VecDist(float mx, float my, float mz) { x = mx; y = my; z = mz; }

	// 0x10015640, _CVector<T>::operator-(const _CVector<T>) const.
	// Declared only.
	VecDist	sub_10015640(const VecDist v) const;

	float	sub_100154b0(const VecDist& vec) const;

	// ltvector.h's in-class Mag.
	float	Mag() const { return (float)sqrt((x*x)+(y*y)+(z*z)); }

	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

float VecDist::sub_100154b0(const VecDist& vec) const
{
	return sub_10015640(vec).Mag();
}
