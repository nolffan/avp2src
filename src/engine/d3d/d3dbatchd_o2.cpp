// Split out of d3dbatchd.cpp: these functions need the
// OTHER optimisation flag from the rest of that file.  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
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
//     other did not.  Mag is written below behind `#pragma intrinsic(sqrt)`
//     and Norm in front of it, so whichever way the default falls, Norm --
//     the one whose shape is certain -- is the one that keeps the default.
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

// Moved here from d3dbatchd.cpp (/Os): the body was already byte-identical
// there, but the shipped function is padded to 16 (41 bytes + a 7-byte
// lea-nop), which is this file's /O2 alignment; /Os pads to 4.
void* C1000a0e0::sub_1000a0e0(uint32 nCount, Sub1000a0e0Alloc* pAlloc)
{
	void* pMem = pAlloc->Alloc(nCount * 0x10);
	void* pRet;

	if (pMem)
		pRet = pMem;
	else
		pRet = LTNULL;

	return pRet;
}



// 0x1001c450 -- element size 0x40.  Byte for byte 0x1000a0e0 with a wider
// element.
class C1001c450
{
public:
	void* sub_1001c450(uint32 nCount, Sub1000a0e0Alloc* pAlloc);

	uint32 m_nUnknown00;                                // +0x00, never read
};



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


// 0x10015590 -- 94 bytes, 4 callers.  _CVector<T>::Cross, ltvector.h:240.
//
// WHAT PINS IT: the SIGN of every term.  objdump prints DE E9 as
// `fsubrp %st,%st(1)`, but DE E9 is Intel's FSUBP -- st(1) = st(1) - st(0).
// Read that way the three results are (v.y*z - v.z*y), (v.z*x - v.x*z) and
// (v.x*y - v.y*x), which is the header's order and NOT the other sign.  The
// operand order inside each product is scheduling, not source: the first
// term is `flds z; fmuls v.y` while the header writes `v.y * z`.


// 0x10015690 -- 68 bytes, 49 callers, the most-called function in the batch.
// _CVector<T>::operator*(T), ltvector.h:64.
//
// WHAT PINS IT: `ret $0x8` (return slot + one float) and the three products
// landing at +0x4, +0x8, +0xc of the temporary in x, y, z order.


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


// 0x10015800 -- 37 bytes, 14 callers.  LTMatrix::operator*, ltmatrix.h:801.
//
// WHAT PINS IT: `ret $0x8` is a __thiscall with a hidden return-slot
// pointer, and the pointer the caller supplied is handed STRAIGHT to MatMul
// as pDest -- `mRet` was elided into the return slot -- then answered in
// %eax.  The three arguments go into a preallocated 0x18-byte outgoing area
// as (return slot, this, &other), which is the order the source writes them.


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



// 0x1005a3c0 -- 28 bytes, 4 callers.  One argument, forwarded unchanged.
class C1005a3c0
{
public:
	void	sub_1005a3c0(uint32 nArg);
	void	sub_1007ee40(uint32 nArg);
};



// 0x1001e070 -- 38 bytes, 3 callers.  Forwards with a leading zero.  The
// 0x18-byte frame (`sub $0x10` then `add $-8`) is icl making room for two
// outgoing dwords.
class C1001e070
{
public:
	void	sub_1001e070(uint32 nArg);
	void	sub_1001dca0(uint32 nFlags, uint32 nArg);
};



// 0x1007ca30 -- 38 bytes, 3 callers.  Byte for byte 0x1001e070 with a
// different callee.
class C1007ca30
{
public:
	void	sub_1007ca30(uint32 nArg);
	void	sub_1007c760(uint32 nFlags, uint32 nArg);
};

// Moved from d3dbatchd.cpp: shipped pads 38 bytes with lea-nops to 48, the /O2 layout (/Os pads to 40).
void C1007ca30::sub_1007ca30(uint32 nArg)
{
	sub_1007c760(0, nArg);
}



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



// 0x100232b0 -- 72 bytes, 6 callers.  Unbinds a texture stage, but only if
// something is bound: the guard reads the same g_pCurTexture[] slot that
// 0x1000efa0 writes, and clearing the slot is the second half of the same
// `if`.  __cdecl -- the stage arrives on the stack and nothing is popped.
void sub_100232b0(uint32 nStage)
{
	if (g_pCurTexture[nStage])
	{
		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, nStage, LTNULL);
		g_pCurTexture[nStage] = LTNULL;
	}
}


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
class C1003abe4
{
public:
			C1003abe4();

	virtual void	_Slot0();

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



// ---------------------------------------------------------------------- //
//  _CVector<float>::Mag -- LAST IN THE FILE ON PURPOSE.
// ---------------------------------------------------------------------- //
//
//  0x10023330 is `(T)sqrt((x*x)+(y*y)+(z*z))`, ltvector.h:39, the same line
//  Norm inlines 0x8000 bytes earlier -- and it comes out as a bare `fsqrt`
//  where Norm's comes out as a call to the CRT.  Two translation units, two
//  answers, one source line, so one of them had sqrt as an intrinsic.
//
//  `#pragma intrinsic` runs from here to the end of the file, so it is put
//  after everything else: if icl ignores it, only this function is affected
//  and Norm above keeps the behaviour that is already known to be right.
//
//  Everything else about the function is settled: the sum accumulates in
//  reverse (z*z, then y*y, then x*x), the same reassociation MatMul shows,
//  and the `push %esi` / `add $0x4,%esp` around it is icl's dead slot.
#pragma intrinsic(sqrt)

// Moved from d3dbatchd.cpp: shipped pads 25 bytes to 32 (/O2 alignment).
float LTVec::sub_10023330() const
{
	return (float)sqrt((x * x) + (y * y) + (z * z));
}
