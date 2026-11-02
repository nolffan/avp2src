// *********************************************************************** //
//
//  MODULE:   D3DQIP2 (.CPP)  --  d3d.ren, the tier-Y seam
//
//  PURPOSE:  Tier-Y CALLERS, decoded instruction
//            by instruction from the retail d3d.ren.
//
//  THIS FILE MUST BE COMPILED /O2 /Qip.  Add it to D3D_QIP_TUS in
//  build/engine.sh next to d3dqip.cpp.
//
//  The frame-draw functions that used to open this
//  file -- 0x100256d0, 0x10026480, 0x10026500, 0x10029890, 0x1002cf40 --
//  now live in w28_100256d0_qipni.cpp with the rest of their shipped
//  translation unit (/O2 /Qip /Qip_no_inlining).  What is left here is the
//  texture-state group, 0x10038f50..0x1003ad30, which stays plain /Qip:
//  the shipped 0x1003ad30 has 0x10039420 inlined.  The banners below that
//  talk about "the five reconstructions" and the tier-Y seam describe the
//  file as it was.
//
//  WHAT THIS FILE WAS.  (Historical -- no placeholder is left here or in
//  d3dqip.cpp, whose `qip_Callee10040938` was the last.)  Everything named `qip_Callee<addr>` was a
//  PLACEHOLDER with the right signature and a deliberately wrong body; only
//  the functions named `sub_<addr>` / `C<addr>::sub_<addr>` were real
//  reconstructions.  That worked because the byte comparison masks all four bytes of a
//  call's relocation, so a tier-Y caller reproduces without its callee
//  being written.
//
//  THE PLACEHOLDER RECIPE, and why each part of it is load-bearing
//  (d3dqip.cpp measured all three failures):
//    * it must have a SIDE EFFECT -- a pure callee gets the call deleted
//      outright and the caller comes back as a bare `ret`.  Every stand-in
//      here ends `g_qip_nSink = v;`.
//    * it must be BIG and STRAIGHT-LINE -- a small body, or a loop, gets
//      inlined into the caller by /Qip and the caller is destroyed.
//      QIP_MIX x10 is ~80 statements of straight-line integer code.
//    * it must CONSUME EVERY PARAMETER, or /Qip drops the argument set-up
//      in the caller and the `lea`/`mov $K` in front of the call vanishes.
//    * its RETURN TYPE must match: a float return makes the caller emit
//      `fstp %st(0)` after the call and shifts every later displacement.
//      None of the shipped callers below has a `dd d8` after its register
//      call, so every void stand-in here is void on purpose.
//
//  HOW THE REGISTER ABI WAS READ OFF THE BYTES.  Argument 1 arrives in
//  %eax, 2 in %edx, 3 in %ecx; arguments 4 and up stay on the stack AT
//  THEIR ORIGINAL SLOTS, i.e. the register arguments leave HOLES.  The
//  caller therefore reserves the whole stack argument area and writes only
//  the tail of it.  Two sites nail that:
//
//      0x100231b5  add $0xffffffe8,%esp        ; 24 bytes for FIVE arguments
//                  mov %ebx,0xc(%esp)          ; argument 4
//                  mov %edx,0x10(%esp)         ; argument 5
//                  call 0x10022e9c             ; 1,2,3 in eax/edx/ecx
//      0x10022e9c  mov 0x24(%esp),%edi         ; = entry+0x14, argument 5
//                  mov 0x54(%esp),%ebx         ; = entry+0x10, argument 4
//
//  and the reservation size is the compiler's own alignment padding of
//  4*nargs up to the next multiple that leaves %esp congruent at the call:
//  8 bytes for one argument, 16 for three, 24 for five.  That is how the
//  arity of every stand-in below was counted.
//
//  LINKAGE.  A shipped callee with a stack shim in front of its register
//  entry (`mov 0x4(%esp),%eax ...`) had EXTERNAL linkage; one with no shim
//  had internal linkage.  0x10039210 is the proof that it does not change
//  the CALLER: it has no shim (0x1003920f is a `nop` and 0x1003920e is the
//  previous function's `ret`), so it is static, and its caller still writes
//  argument 4 at 0xc(%esp) with the three register holes left empty.  So
//  every stand-in here is `static`, which is also the one form d3dqip.cpp
//  proved end to end.
//
//  FLAGS.  Four of the five reconstructions are /O2 -- duplicated
//  epilogues and `add $-N,%esp` + `mov` for outgoing arguments.
//  C10023130::sub_10023130 is /Os and is the odd one out; its banner says
//  so and it wants a separate /Os + /Qip translation unit
//  It is left here so the address is not lost.
//
//  Naming: `sub_<address>` so the address can be read out of
//  the symbol.  `Obj<hex>` / `Rec<hex>` types are invented here.
//
// *********************************************************************** //

#include "ltbasedefs.h"

#include <math.h>
#include <string.h>


// ======================================================================= //
//  THE PLACEHOLDER MACHINERY
// ======================================================================= //

// The placeholder machinery (g_qip_nSink, QIP_MIX, QIP_BODY) is gone:
// every qip_Callee stand-in in this file now calls the real function (see
// each declaration), so nothing here runs a wrong body any more.


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  The Direct3D 7 device at 0x1008d988, repeated from
//  src/engine/d3d/d3dr2d.cpp (separate translation units, so it is
//  repeated rather than shared).  Slot numbering, all already pinned in
//  this repo: SetRenderState is index 20 (+0x50), GetRenderState is index
//  21 (+0x54), SetTexture is index 35 (+0x8c).  The
//  `mov (%eax),%edx ; call *0xNN(%edx)` with the object written as the
//  first stack argument is the function-pointer-struct model the rest of
//  src/engine/d3d uses for COM.
// ---------------------------------------------------------------------- //
struct D3DDevice7;

struct DDSurface;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x20];
	long (__stdcall* SetRenderTarget)(D3DDevice7*, DDSurface* pTarget,
	                                  uint32 dwFlags);                // +0x20
	uint8 m_pad_24[0x50 - 0x24];
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);                 // +0x50
	long (__stdcall* GetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32* lpdwValue);              // +0x54
	uint8 m_pad_58[0x0c];
	long (__stdcall* DrawPrimitive)(D3DDevice7*, uint32 dwPrimType,
	                                uint32 dwVertexType, void* pVerts,
	                                uint32 dwCount, uint32 dwFlags);  // +0x64
	uint8 m_pad_68[0x24];
	long (__stdcall* SetTexture)(D3DDevice7*, uint32 dwStage,
	                             void* lpTexture);                    // +0x8c
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

extern uint32 g_1008d7d0;                               // current stage
extern void*  g_a1008e684[];                            // bound textures


// ======================================================================= //
//  0x10039420   462 bytes (0x10039420..0x100395ed)      1 caller
//
//  DECODED, every instruction.  Rebuild a 64x64 gamma ramp surface when the
//  gamma actually changed.
//
//      10039420  push %ebp / mov %esp,%ebp
//                mov $0x4060,%eax / call 0x100805a0        ; _chkstk
//                mov %edi,0x8(%esp) / mov %esi,0x4(%esp) / mov %ebx,(%esp)
//                flds 0x1008da6c / fcomps 0x8(%ebp)
//                fnstsw %ax / mov 0x8(%ebp),%edx / sahf
//                jne 0x10039458 / <epilogue>               ; equal -> return
//      10039458  mov 0x1008da74,%eax
//                mov %edx,0x1008da6c                       ; INTEGER store
//                test %eax,%eax / je 0x1003947c
//                mov 0x1008da74,%eax                       ; RELOADED
//                mov (%eax),%edx / push %eax / call *0x8(%edx)
//                movl $0x0,0x1008da74
//      1003947c  lea -0x4040(%ebp),%edi / xor %eax,%eax
//                mov $0x1000,%ecx / rep stos %eax,%es:(%edi)
//                flds 0x8(%ebp) / fstpl -0x10(%ebp)
//                xor %ebx,%ebx / xor %edi,%edi / xor %esi,%esi
//                mov $-4,-0x1c(%ebp) / mov $-0x100,-0x18(%ebp)
//      100394b0  mov %ebx,-0x40(%ebp) / movl $0,-0x3c(%ebp)
//                fildll -0x40(%ebp)                        ; (double)(unsigned)i
//                fmuls 0x100a9410                          ; * 0.015625f
//                fldl -0x10(%ebp) / fxch %st(1)
//                fstpl (%esp) / fstpl 0x8(%esp) / call 0x1007fe54   ; pow
//                fmuls 0x100a9414                          ; * 255.0f
//                fstpl -0x28(%ebp)  ... inline double->int ...
//      10039532  lea (%esi,%edi,1),%edx
//                mov %eax,%ecx / shl $0x8,%ecx / or %eax,%ecx
//                mov %eax,%edi / shl $0x10,%edi / or %edi,%ecx
//                test %ebx,%ebx / mov %ecx,-0x4040(%ebp,%edx,1)
//                jbe 0x1003958f
//                ... the SAME or-chain twice more, at -0x18+%edi and
//                    -0x1c+%esi ...
//      1003958f  -0x1c += 4 / -0x18 += 0x100 / inc %ebx
//                %esi += 0x100 / %edi += 4
//                cmp $0x40,%ebx / jb 0x100394b0
//                add $0xfffffff0,%esp / lea -0x4040(%ebp),%eax
//                mov $0x40,%edx / mov $0x40,%ecx
//                movl $0x100,0xc(%esp) / call 0x10039210
//                mov %eax,0x1008da74
//
//  WHAT PINS IT:
//    * `mov $0x4060,%eax ; call 0x100805a0` is _chkstk, so the frame is
//      0x4060 and the array at -0x4040 is 0x4000 bytes = 64*64 dwords.  The
//      `rep stos` of 0x1000 dwords zeroes exactly that array.
//    * the constants ARE the ramp: 0x100a9410 is 0x3c800000 = 0.015625f =
//      1/64 and 0x100a9414 is 0x437f0000 = 255.0f, and 0x1007fe54 opens
//      `mov 0x8(%esp),%ecx ; and $0x7ff00000,%ecx` on the SECOND double --
//      the exponent of the exponent -- so it is pow(x, y), not a one-
//      argument libm entry.  Both multiplies are `fmuls`, i.e. FLOAT
//      literals promoted in the x87, so the source writes them as floats.
//    * `fildll` of {i, 0} is icl's UNSIGNED int-to-double, so the loop
//      counter is unsigned; the back edge `cmp $0x40,%ebx ; jb` agrees.
//    * the three index expressions are %esi+%edi = i*0x100 + i*4,
//      (-0x100 + i*0x100) + i*4 and (-4 + i*4) + i*0x100 -- i.e. [i][i],
//      [i-1][i] and [i][i-1] of a uint32[64][64].
//    * the guard on the second pair is `test %ebx,%ebx ; jbe`, an UNSIGNED
//      `<=`, so the source wrote `if (i > 0)` and not `if (i != 0)`.
//    * the or-chain is emitted THREE times, once per store: icl did not CSE
//      it, so the expression really is written out three times.
//    * `mov 0x1008da74,%eax` appears TWICE around the release, so the
//      global is read twice -- there is no local copy.
//    * `mov %edx,0x1008da6c` stores the float with an INTEGER mov, which is
//      what this compiler does for float lvalue copies.
//    * `fcomps 0x1008da6c`-first means the global is the source's LEFT
//      operand.
//    * 0x10039210 has no shim (0x1003920f is a `nop`), so it is static; the
//      caller reserves 16 and writes argument 4 at 0xc(%esp), leaving the
//      three register holes -- FOUR arguments, returning a pointer.
//    * DUPLICATED epilogue (0x10039449 and 0x100395df) and `add $-N,%esp` +
//      `mov`: /O2.
//
//  RISK, stated plainly: the double->int32 at 0x100394e1..0x1003951f is
//  icl's INLINE truncating conversion (exponent extract, `shld $0xb`,
//  variable `shr`, `neg` for the sign), not a call to _ftol.  It is emitted
//  from a plain `(uint32)` cast; if this build's icl calls _ftol instead,
//  the whole tail shifts and the body below is a near, not a match.
//  COIN-FLIP on the frame: `nValue` is homed at -0x40(%ebp), the same slot
//  the fildll temporary uses on the next iteration; if the slot colouring
//  does not reproduce, drop the variable and write the or-chain against the
//  cast expression directly.
//  REMAINING RESIDUAL (near, 464 of 464 bytes): shipped puts the early
//  return's epilogue straight after the `jne`, ours sinks it to the end
//  behind a `je`.  (The g_1008da74 reload after the null test is now
//  reproduced by a volatile read at the test -- 4 -> 3 mnemonic blocks.
//  Measured with that in place, all still sink the return or
//  share one epilogue: goto-over, do/while(0) break, an empty then-arm,
//  braces, `!=` wrapping the body with a trailing return, a float local
//  for the old value, a volatile float read.  An int early return is laid
//  out inline elsewhere (0x1003b7f0, 0x1005d9c0); it is the float `==`
//  that icl predicts not-taken: a three-function probe TU at /O2 sinks
//  `if (gf == f) return;` but lays `if (gi == n) return;` and
//  `if (gf < f) return;` out inline.)  Measured without effect on
//  either, all at /O2 /Qip: `!=` wrapping the body, if/else, goto, a bool
//  temp, `!(a != b)`, an explicit trailing return, a local copy of the
//  pointer, a C++ virtual Release(), inline SafeRelease helpers taking the
//  pointer by value, and the two globals as one struct; flag sets /O2,
//  /Ox, /Oi-, /Qrcd, /Ob0, /Og-.  Swapping the compare operands flips the
//  fcomps and still sinks the return.  The two variants that DID produce
//  the inline-epilogue layout (a SafeRelease taking IUnk*& or IUnk**) also
//  make icl realign the stack (`and $-16,%esp` prologue), so they are not
//  the source either.
//  COIN-FLIP on pow: the shipped call passes TWO doubles (16 bytes of
//  outgoing area, `fstpl (%esp)` and `fstpl 0x8(%esp)`), so it is
//  pow(double,double) and not C++ math.h's `float pow(float,float)`
//  overload, which is what unadorned `pow(i * (1.0f/64.0f), fGamma)` would
//  select.  The explicit `(double)` casts below force the right overload
//  and should cost nothing, because a float-to-double widening of a value
//  already in %st(0) is a no-op with no /Op.  If they DO cost an `fstps`/
//  `flds` round trip, the alternative is to drop <math.h> and declare
//  `extern "C" double pow(double, double);` in this file, then write the
//  call with no casts at all.
// ======================================================================= //

struct IUnk10039420;

struct IUnk10039420Vtbl
{
	uint8 m_pad_00[0x8];
	long (__stdcall* Release)(IUnk10039420*);           // +0x08
};

struct IUnk10039420
{
	IUnk10039420Vtbl* lpVtbl;
};

extern float          g_1008da6c;                       // 0x1008da6c
extern IUnk10039420*  g_1008da74;                       // 0x1008da74


// ======================================================================= //
//  0x10038f50 -- 440 bytes.  The lightmap page allocator 0x1003ad30
//  installs through 0x1006d460: (width, height, extra caps) -> a texture
//  record.  Create a texture surface of that size in the lightmap format
//  (0x1008e6c4's DDPIXELFORMAT) on the current stage (0x1008d7cc) through
//  IDirectDraw7::CreateSurface (0x1008bf74, vtable +0x18); pop a record
//  from the texture bank (0x1008e644 -- big38a90.cpp's RTexBank::Alloc,
//  inlined, refilling through 0x1007fbf0), construct it (0x1003abe4) and
//  fill it: surface, 1/width and 1/height, the size, self-reference, the
//  rest cleared.  No format, a failed create or an empty bank: NULL (the
//  surface released in the last case).
//
//  IDENTICAL (was 186 bytes differ).  Statement order
//  only: the two reciprocal stores LAST (icl computes both divides early
//  and stores them after the integer fields, as shipped); m_pSurface0c
//  before m_pSelf18; and the descriptor fields dwSize, dwWidth, dwCaps,
//  dwFlags, dwHeight, dwTextureStage -- all 720 orders of those six were
//  measured, twelve reach 6 bytes and only the tail order closes them.
// ======================================================================= //
struct DDSurf10038f50;

struct DDSurf10038f50Vtbl
{
	void*	m_pad_00[2];
	long (__stdcall* Release)(DDSurf10038f50*);         // +0x08
};

struct DDSurf10038f50
{
	DDSurf10038f50Vtbl*	lpVtbl;
};

struct DDSD10038f50
{
	uint32		dwSize;                                 // +0x00
	uint32		dwFlags;                                // +0x04
	uint32		dwHeight;                               // +0x08
	uint32		dwWidth;                                // +0x0c
	uint8		m_pad_10[0x48 - 0x10];
	uint32		ddpfPixelFormat[8];                     // +0x48
	uint32		dwCaps;                                 // +0x68
	uint8		m_pad_6c[0x78 - 0x6c];
	uint32		dwTextureStage;                         // +0x78
};

struct DDraw10038f50;

struct DDraw10038f50Vtbl
{
	void*	m_pad_00[6];
	long (__stdcall* CreateSurface)(DDraw10038f50*, DDSD10038f50* pDesc,
	                                DDSurf10038f50** ppSurf,
	                                void* pOuter);      // +0x18
};

struct DDraw10038f50
{
	DDraw10038f50Vtbl*	lpVtbl;
};

struct Fmt10038f50
{
	uint8		m_pad_00[0x0c];
	uint32		m_PF[8];                                // +0x0c DDPIXELFORMAT
};

struct RTexture
{
	void		sub_1003abe4();                         // 0x1003abe4 ctor

	RTexture*		m_pNextFree;                        // +0x00 (vptr once built)
	float			m_fInvW04;                          // +0x04
	float			m_fInvH08;                          // +0x08
	DDSurf10038f50*	m_pSurface0c;                       // +0x0c
	uint32			m_nBytes10;                         // +0x10
	uint16			m_nStamp14;                         // +0x14
	uint16			m_nAlphaRef16;                      // +0x16
	RTexture*		m_pSelf18;                          // +0x18
	void*			m_pLink1c[2];                       // +0x1c its list link
	void*			m_pLinkData24;                      // +0x24
	uint16			m_nWidth28;                         // +0x28
	uint16			m_nHeight2a;                        // +0x2a
	void*			m_pShared2c;                        // +0x2c
	uint8			m_pad_30[0x40 - 0x30];
	uint16			m_n40;                              // +0x40
};

struct RTexBank
{
	uint32		m_n00;                                  // +0x00
	uint32		m_nElemSize04;                          // +0x04
	uint32		m_nGrowCount08;                         // +0x08
	uint8		m_Reserved0c[0x0c];                     // +0x0c..+0x17
	RTexture*	m_pFree18;                              // +0x18

	RTexture* Alloc();
};

extern "C" void sub_1007fbf0(RTexBank* pBank, uint32 nCount);

inline RTexture* RTexBank::Alloc()
{
	RTexture* pNode;

	pNode = m_pFree18;
	if (pNode == LTNULL)
	{
		sub_1007fbf0(this, m_nGrowCount08);

		pNode = m_pFree18;
		if (pNode == LTNULL)
			return LTNULL;
	}

	m_pFree18 = pNode->m_pNextFree;
	return pNode;
}

extern RTexBank			g_1008e644;                     // 0x1008e644
extern DDraw10038f50*	g_1008bf74;                     // 0x1008bf74
extern Fmt10038f50*		g_1008e6c4;                     // 0x1008e6c4
extern uint32			g_1008d7cc;                     // 0x1008d7cc

void* sub_10038f50(uint32 nWidth, uint32 nHeight, uint32 nCaps)
{
	DDSD10038f50	ddsd;
	DDSurf10038f50*	pSurf;
	RTexture*		pTex;
	Fmt10038f50*	pFmt;

	pFmt = g_1008e6c4;
	if (!pFmt)
		return LTNULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwWidth = nWidth;
	ddsd.dwCaps = nCaps | 0x1000;
	ddsd.dwFlags = 0x101007;
	ddsd.dwHeight = nHeight;
	ddsd.dwTextureStage = g_1008d7cc;
	memcpy(ddsd.ddpfPixelFormat, pFmt->m_PF, sizeof(ddsd.ddpfPixelFormat));

	if (g_1008bf74->lpVtbl->CreateSurface(g_1008bf74, &ddsd, &pSurf, LTNULL) != 0)
		return LTNULL;

	pTex = g_1008e644.Alloc();
	if (!pTex)
	{
		pSurf->lpVtbl->Release(pSurf);
		return LTNULL;
	}

	pTex->sub_1003abe4();

	pTex->m_pSurface0c = pSurf;
	pTex->m_pSelf18 = pTex;
	pTex->m_pShared2c = LTNULL;
	pTex->m_pLinkData24 = LTNULL;
	pTex->m_n40 = 0;
	pTex->m_nWidth28 = (uint16)nWidth;
	pTex->m_nHeight2a = (uint16)nHeight;
	pTex->m_nStamp14 = 0;
	pTex->m_fInvW04 = 1.0f / (float)nWidth;
	pTex->m_fInvH08 = 1.0f / (float)nHeight;

	return pTex;
}


// ======================================================================= //
//  0x10039210 -- 514 bytes (w16_10039210_qip.cpp; moved
//  here).  STATIC: the register ABI in the shipped bytes (pBits in %eax,
//  nWidth in %edx, nHeight in %ecx, nPitch on the stack; no stack entry)
//  is a static /Qip function whose three callers -- 0x10039420 below and
//  0x1003ad30 twice -- share its TU.
//
//  Makes a DirectDraw TEXTURE surface of nWidth x nHeight in the first
//  available renderer surface format (g_1008e6b0[0], else [2], else fail),
//  locks it, converts the caller's 32-bit pixels into it through the
//  pixel-format converter (0x1007a130 on 0x1008daa0, source format set by
//  0x1007a3b0 = 32-bit ARGB, destination by 0x10021010 from the format
//  record's DDPIXELFORMAT), unlocks it and returns it.  0 on any failure
//  (the surface is released if the Lock fails).  Each measured:
//    * the format pick as if / else if / else-return on the table slots
//      (not a pointer tested twice): shipped loads g_1008e6b0[0] into %eax
//      and copies it to %ebx on each arm;
//    * the pixel format copied with memcpy, not a struct assignment: the
//      shipped copy runs through %edi (`lea -0xbc(%ebp),%edi`) after the
//      two caps stores;
//    * the success path nested inside `CreateSurface(...) == 0` and
//      `Lock(...) == 0`, the Release after the inner block and one
//      `return 0` at the bottom: shipped jumps the CreateSurface failure
//      forward into the Lock failure's tail (`jne 0x10039343`) and makes
//      the Release a `push %eax` call.
// ======================================================================= //
struct DDPixelFormat
{
	uint32 dwSize;                                      // +0x00
	uint32 dwFlags;                                     // +0x04
	uint32 dwFourCC;                                    // +0x08
	uint32 dwRGBBitCount;                               // +0x0c
	uint32 dwRBitMask;                                  // +0x10
	uint32 dwGBitMask;                                  // +0x14
	uint32 dwBBitMask;                                  // +0x18
	uint32 dwRGBAlphaBitMask;                           // +0x1c
};                                                      // 0x20

struct DDSCaps2 { uint32 dwCaps, dwCaps2, dwCaps3, dwCaps4; };

struct DDSurfaceDesc2
{
	uint32        dwSize;                               // +0x00
	uint32        dwFlags;                              // +0x04
	uint32        dwHeight;                             // +0x08
	uint32        dwWidth;                              // +0x0c
	int32         lPitch;                               // +0x10
	uint32        dwBackBufferCount;                    // +0x14
	uint32        dwMipMapCount;                        // +0x18
	uint32        dwAlphaBitDepth;                      // +0x1c
	uint32        dwReserved;                           // +0x20
	void*         lpSurface;                            // +0x24
	uint32        ddckCKDestOverlay[2];                 // +0x28
	uint32        ddckCKDestBlt[2];                     // +0x30
	uint32        ddckCKSrcOverlay[2];                  // +0x38
	uint32        ddckCKSrcBlt[2];                      // +0x40
	DDPixelFormat ddpfPixelFormat;                      // +0x48
	DDSCaps2      ddsCaps;                              // +0x68
	uint32        dwTextureStage;                       // +0x78
};                                                      // 0x7c

struct DDSurface7;

struct DDSurface7Vtbl
{
	void* m_pad_00[2];                                  // +0x00..+0x04
	long (__stdcall* Release)(DDSurface7*);             // +0x08
	void* m_pad_0c[4];                                  // +0x0c..+0x18
	long (__stdcall* BltFast)(DDSurface7*, uint32 dwX, uint32 dwY,
	                          DDSurface7* lpDDSrcSurface, void* lpSrcRect,
	                          uint32 dwTrans);          // +0x1c
	void* m_pad_20[4];                                  // +0x20..+0x2c
	long (__stdcall* GetAttachedSurface)(DDSurface7*, DDSCaps2* lpDDSCaps,
	                                     DDSurface7** lplpDDAttached);
	                                                    // +0x30
	void* m_pad_34[9];                                  // +0x34..+0x54
	long (__stdcall* GetSurfaceDesc)(DDSurface7*,
	                                 DDSurfaceDesc2* lpDDSurfaceDesc);
	                                                    // +0x58
	void* m_pad_5c[2];                                  // +0x5c..+0x60
	long (__stdcall* Lock)(DDSurface7*, void* lpDestRect,
	                       DDSurfaceDesc2* lpDDSurfaceDesc, uint32 dwFlags,
	                       void* hEvent);               // +0x64
	void* m_pad_68[6];                                  // +0x68..+0x7c
	long (__stdcall* Unlock)(DDSurface7*, void* lpRect);// +0x80
};
struct DDSurface7 { DDSurface7Vtbl* lpVtbl; };

struct DDraw7;
struct DDraw7Vtbl
{
	void* m_pad_00[6];                                  // +0x00..+0x14
	long (__stdcall* CreateSurface)(DDraw7*,
	                                DDSurfaceDesc2* lpDDSurfaceDesc,
	                                DDSurface7** lplpDDSurface,
	                                void* pUnkOuter);   // +0x18
};
struct DDraw7 { DDraw7Vtbl* lpVtbl; };

extern DDraw7* g_pDirectDraw;                           // 0x1008bf74

class PFormat
{
public:
	// 0x1007a3b0 -- Slot00(3, 0xff000000, 0xff0000, 0xff00, 0xff).
	// d3dbatchd.cpp already records this address as a thiscall member.
	void            sub_1007a3b0();                     // 0x1007a3b0

	virtual uint32  Slot00(uint32 nType, uint32 nAlphaMask, uint32 nRedMask,
	                       uint32 nGreenMask, uint32 nBlueMask);
};                                                      // 0x38

struct ConvertRequest
{
	// 0x10079fd0.  Called as a thiscall on a stack local, `this` in %ecx.
	// Written as an explicit member rather than a constructor because
	// there is no body to give it and because a real ctor would make icl
	// generate vptr stores for the two embedded formats.
	ConvertRequest* Construct();                        // 0x10079fd0

	PFormat* m_pSrcFormat;                              // +0x00
	void*    m_pSrcData;                                // +0x04
	uint32   m_nSrcPitch;                               // +0x08
	uint32   m_nUnknown0c;                              // +0x0c  ctor: 0
	PFormat* m_pDstFormat;                              // +0x10
	void*    m_pDstData;                                // +0x14
	uint32   m_nDstPitch;                               // +0x18
	uint32   m_nWidth;                                  // +0x1c
	uint32   m_nHeight;                                 // +0x20
	uint32   m_nUnknown24;                              // +0x24  ctor: 0
	uint8    m_SrcFormat[0x38];                         // +0x28
	uint8    m_DstFormat[0x38];                         // +0x60
};                                                      // 0x98

class FormatMgr
{
public:
	uint32 ConvertPixels(ConvertRequest* pRequest);     // 0x1007a130
};
extern FormatMgr g_1008daa0;                            // 0x1008daa0

struct D3DFormatRec
{
	uint32        m_nUnknown00[3];                      // +0x00..+0x0b
	DDPixelFormat m_ddpf;                               // +0x0c..+0x2b
	uint32        m_nPixelShift;                        // +0x2c
	uint32        m_nPixelSize;                         // +0x30
};

extern D3DFormatRec* g_1008e6b0[7];                     // 0x1008e6b0

// 0x10021010 -- copy a DDPIXELFORMAT into a PFormat (big3a080.cpp).
extern void sub_10021010(const DDPixelFormat* pPF, PFormat* pFormat);

#define DDSD_CAPS                       0x00000001
#define DDSD_HEIGHT                     0x00000002
#define DDSD_WIDTH                      0x00000004
#define DDSD_PIXELFORMAT                0x00001000
#define DDSCAPS_TEXTURE                 0x00001000
#define DDSCAPS2_40090                  0x00040090      // as shipped

static IUnk10039420* sub_10039210(void* pBits, uint32 nWidth, uint32 nHeight,
                           uint32 nPitch)
{
	ConvertRequest  req;                                // ebp-0x19c
	DDSurfaceDesc2  ddsd;                               // ebp-0x104
	DDSurface7*     pSurf;                              // ebp-0x88
	DDSurfaceDesc2  lockDesc;                           // ebp-0x84
	D3DFormatRec*   pFmtRec;

	req.Construct();

	if (g_1008e6b0[0])
		pFmtRec = g_1008e6b0[0];
	else if (g_1008e6b0[2])
		pFmtRec = g_1008e6b0[2];
	else
		return LTNULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize          = sizeof(ddsd);
	ddsd.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.dwHeight        = nHeight;
	ddsd.dwWidth         = nWidth;
	ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_40090;
	memcpy(&ddsd.ddpfPixelFormat, &pFmtRec->m_ddpf, sizeof(DDPixelFormat));

	if (g_pDirectDraw->lpVtbl->CreateSurface(g_pDirectDraw, &ddsd, &pSurf,
	                                         LTNULL) == 0)
	{
		memset(&lockDesc, 0, sizeof(lockDesc));
		lockDesc.dwSize = sizeof(lockDesc);

		if (pSurf->lpVtbl->Lock(pSurf, LTNULL, &lockDesc, 0, LTNULL) == 0)
		{
			req.m_pSrcFormat->sub_1007a3b0();
			req.m_pSrcData  = pBits;
			req.m_nSrcPitch = nPitch;
			sub_10021010(&pFmtRec->m_ddpf, req.m_pDstFormat);
			req.m_pDstData  = lockDesc.lpSurface;
			req.m_nDstPitch = lockDesc.lPitch;
			req.m_nWidth    = nWidth;
			req.m_nHeight   = nHeight;
			g_1008daa0.ConvertPixels(&req);

			pSurf->lpVtbl->Unlock(pSurf, LTNULL);

			return (IUnk10039420*)pSurf;
		}

		pSurf->lpVtbl->Release(pSurf);
	}

	return LTNULL;
}


// The or-chain is emitted THREE times in the shipped code, and a plain
// expression written three times is CSE'd into one; going through an inline
// helper is what keeps the three copies apart (measured: 432 -> 464 bytes,
// the chains then match instruction for instruction).
inline uint32 Gray10039420(int32 v)
{
	return v | (v << 8) | (v << 16);
}

// NEAR: one block.  The shipped body returns for an
// unchanged gamma on the FALL-THROUGH (`jne body` then an in-line epilogue);
// icl here sends the equal case to a duplicated epilogue after the body.
// The same compare inlined into 0x1003ad30 IS out of line in the shipped
// code, so it is not the float `==` prediction alone.  Measured with no
// effect: operands swapped, `!(!=)`, `if (!=) { body }` (one shared
// epilogue, 10 lines off), an empty then-arm, goto forms, the definition
// moved below 0x1003ad30.  A cold-named inline helper in the body (icl's
// name-based prediction) made it worse, not better.
void sub_10039420(float fGamma)
{
	uint32  nBuf[64][64];
	int32   nValue;
	uint32  i;

	if (g_1008da6c == fGamma)
		return;

	g_1008da6c = fGamma;

	// The null test reads the global through a volatile lvalue: shipped
	// RELOADS g_1008da74 after the test (a plain read is CSE'd into %eax).
	if (*(IUnk10039420* volatile*)&g_1008da74)
	{
		g_1008da74->lpVtbl->Release(g_1008da74);
		g_1008da74 = LTNULL;
	}

	memset(nBuf, 0, sizeof(nBuf));

	for (i = 0; i < 64; ++i)
	{
		// (float) on pow's result: the * 255.0f is `fmuls`, a FLOAT operand;
		// multiplying the double result directly gives `fmull`.
		nValue = (int32)((float)pow((double)(i * (1.0f / 64.0f)),
		                            (double)fGamma) * 255.0f);

		nBuf[i][i] = Gray10039420(nValue);

		if (i > 0)
		{
			nBuf[i - 1][i] = Gray10039420(nValue);
			nBuf[i][i - 1] = Gray10039420(nValue);
		}
	}

	g_1008da74 = sub_10039210(nBuf, 64, 64, 0x100);
}


// ======================================================================= //
//  0x1003ad30 -- 1168 bytes.  The renderer's texture-state start-up:
//  clear the format records (0x1008e6b0, seven), the bound textures
//  (0x1008e684, four) and the texture lists, reset the lightmap page
//  heads (0x1006d520), re-make the texture bank (sb_Term2 / sb_Init2,
//  0x4c-byte records), enumerate the device's texture formats through
//  0x10037cb0 and pick them (0x10037f30, FALSE on failure), install the
//  page allocator 0x10038f50 (0x1006d460), create the lightmap scratch
//  surface (0x10039110) and the clamp table (0x1006c4d0); then rebuild
//  the 16x16 spotlight falloff texture -- alpha = (1 - (d / 7.5)^3) *
//  255.9 over the distance from the centre, white -- and the gamma ramp
//  at 5.0 (0x10039420, which /Qip expands here).
//
//  WHY HERE.  0x10039210 is register-only (no stack entry) and this
//  calls it the register way (0x1003afe4: %eax/%edx/%ecx and 0xc(%esp));
//  its other caller is 0x10039420 above, so all three are one TU.  When
//  0x10039210 is written it belongs in this file as a static, ahead
//  of 0x10039420.  The float-to-int and pow temporaries are doubles, which
//  gives it icl's 14-byte stack-aligning entry.
//
//  NEAR.  The shipped body, 0x1003ad3e (behind the
//  14-byte stack-realigning entry at 0x1003ad30, which falls into it at
//  0x1003ad41), is matched block for block except for that entry: icl
//  here gives this function NO realigning prologue, so %ebx is free and
//  the loops allocate one register differently.  Measured with probes:
//  the entry appears for a pow() loop, a float-array store, the whole
//  function minus the falloff loop; it disappears as soon as a NESTED loop
//  converts a float to int (a 16x16 `(int)(x * f)` store is enough),
//  whatever the array shape (2-D, flat, pointer walk) or loop order.
//  Source fixes that did reproduce shipped idioms, kept: the exponent at
//  0x100894b0 is a TU static (loaded once per row and held as a double
//  across pow, where an extern is reloaded per pixel); fY computed inside
//  the x loop (shipped keeps fY, not fY*fY, as the hoisted double); the
//  three 0xff components through a variable (three separate `or`s, as
//  shipped, where literals fold to one `or $0xffffff`).
// ======================================================================= //
class LTLink;
class LTList;
struct D3DFormatRec;

// ConvertRequest: defined above 0x10039210.

class C1006d520
{
public:
	void	sub_1006d520();                             // 0x1006d520
};

class C10091950
{
public:
	void	sub_1006d460(void* (__cdecl* pfnAlloc)(uint32, uint32, uint32));
};

struct StructBank;

struct IDev1003ad30;
struct IDev1003ad30Vtbl
{
	void*	m_pad_00[4];
	long (__stdcall* EnumTextureFormats)(IDev1003ad30*,
	                                     long (__stdcall*)(void*, void*),
	                                     void* pContext);  // +0x10
};

struct IDev1003ad30
{
	IDev1003ad30Vtbl*	lpVtbl;
};

struct LinkPair1003ad30
{
	void*	m_pPrev;                                    // +0x00
	void*	m_pNext;                                    // +0x04
};

struct List1003ad30
{
	uint32				m_nElements;                    // +0x00
	LinkPair1003ad30	m_Head;                         // +0x04
};

extern D3DFormatRec*	g_1008e6b0[7];                  // 0x1008e6b0
extern void*			g_a1008e684[];                  // 0x1008e684
extern LinkPair1003ad30	g_1008e6a4;                     // 0x1008e6a4
extern LinkPair1003ad30	g_1008e634;                     // 0x1008e634
extern List1003ad30		g_1008e694;                     // 0x1008e694
extern C1006d520		g_10091950;                     // 0x10091950
extern uint32			g_b1008da70;                    // 0x1008da70
extern IDev1003ad30*	g_1008d988;                     // 0x1008d988
extern IUnk10039420*	g_p1008da78;                    // 0x1008da78
// 0x100894b0 = 3.0, the falloff exponent.  Read once in the whole image
// (here), and a TU STATIC: the shipped loop loads it once per ROW and
// keeps it as a double across the pow calls, which icl does only when it
// can prove pow cannot write it; an extern float is reloaded every pixel
// (measured).
static float			g_f100894b0 = 3.0f;             // 0x100894b0

struct MemBank;
extern void   sub_1007fba0(MemBank* pBank, uint32 bFlag);        // 0x1007fba0 sb_Term2
extern int    sub_1007fb70(StructBank* pBank, uint32 nStructSize,
                           uint32 nCacheSize, uint32 nPrealloc); // 0x1007fb70 sb_Init2
extern long __stdcall sub_10037cb0(void* pFormat, void* pContext);   // 0x10037cb0
extern LTBOOL sub_10037f30();                                  // 0x10037f30
extern void*  sub_10038f50(uint32 a, uint32 b, uint32 c);      // 0x10038f50
extern LTBOOL sub_10039110();                                  // 0x10039110
extern void   sub_1006c4d0();                                  // 0x1006c4d0

// The missing piece is icl's /Qsfalign8 realign entry (the
// shipped 0x1003ad30 opens with it and frames on %ebx; ours, 1091 bytes,
// is otherwise the shipped body save for the registers that frees).  Probe
// TU, /O2 /Qip: a double in a doubly nested loop gets the entry ($$1
// body); the SAME nest that also stores into a local array (`a[y][x] =`,
// `p[y*16+x] =`, `*p++ =`, a struct member array, a static helper inlined
// by /Qip, do/while, variable bounds, any double/float spelling, even
// `a[y][x] = 0` beside the double) leaves an EMPTY `$$1` -- the entry is
// planned and then dropped.  Separate nests keep it, as does a single
// loop storing into the array, or a nest whose double work is in the
// OUTER body only.  At /Os the plain nest keeps it.  On this function:
// `double` result locals, the conversion without the float cast, double
// fX/fY, fY hoisted to the outer loop, a while inner loop, uint32 x/y --
// all still no entry (and the same bytes).
LTBOOL sub_1003ad30()
{
	ConvertRequest	req;
	uint32			nFalloff[16][16];
	int32			x, y;
	float			fX, fY, fDist;
	int32			nAlpha;
	uint32			i;
	uint32			nWhite;

	memset(g_1008e6b0, 0, sizeof(g_1008e6b0));

	for (i = 0; i < 4; i++)
		g_a1008e684[i] = LTNULL;

	g_1008e6a4.m_pPrev = g_1008e6a4.m_pNext = &g_1008e6a4;
	g_1008e634.m_pNext = g_1008e634.m_pPrev = &g_1008e634;
	g_1008e694.m_nElements = 0;
	g_1008e694.m_Head.m_pNext = g_1008e694.m_Head.m_pPrev = &g_1008e694.m_Head;

	g_10091950.sub_1006d520();

	sub_1007fba0((MemBank*)&g_1008e644, 1);
	sub_1007fb70((StructBank*)&g_1008e644, 0x4c, 0x40, 0);

	g_b1008da70 = 1;
	g_1008d988->lpVtbl->EnumTextureFormats(g_1008d988, sub_10037cb0, LTNULL);

	if (!sub_10037f30())
		return LTFALSE;

	((C10091950*)&g_10091950)->sub_1006d460(sub_10038f50);
	sub_10039110();
	sub_1006c4d0();

	req.Construct();

	if (g_p1008da78)
	{
		g_p1008da78->lpVtbl->Release(g_p1008da78);
		g_p1008da78 = LTNULL;
	}

	nWhite = 0xff;

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			fX = (float)x - 7.5f;
			fY = (float)y - 7.5f;
			fDist = (float)sqrt(fX * fX + fY * fY + 0.0f);
			if (fDist > 7.5f)
				fDist = 7.5f;

			nAlpha = (int32)((1.0f - (float)pow((double)(fDist * (1.0f / 7.5f)), (double)g_f100894b0)) * 255.9f);
			nFalloff[y][x] = ((uint32)(uint8)nAlpha << 24) | (nWhite << 16) |
			                 (nWhite << 8) | nWhite;
		}
	}

	g_p1008da78 = sub_10039210(nFalloff, 16, 16, 0x40);

	sub_10039420(5.0f);

	return LTTRUE;
}


// ======================================================================= //
//  0x10023130   198 bytes (0x10023130..0x100231f5)      2 callers
//
//  *** THIS ONE IS /Os, NOT /O2.  IT DOES NOT BELONG IN THIS FILE. ***
//  It is here so the address is not lost; it belongs
//  in an /Os + /Qip translation unit of its own.  Three independent
//  markers, all three of them /Os tells:
//      * the second exit CROSS-JUMPS -- 0x100231f2 is `xor %eax,%eax ;
//        eb ac`, jumping back into the one epilogue, instead of duplicating
//        it the way 0x10026480 and 0x10039420 above both do;
//      * outgoing arguments are PUSHED -- `push %eax ; push %edx` in front
//        of both string compares;
//      * `&this->m_n100` is `lea 0x100(%ebx),%edx`, not `mov $K ; add`.
//  `xor %edi,%edi ; inc %edi` and `xor %eax,%eax ; inc %eax` for the
//  constant 1 are the fourth.  (The one `add $0xffffffe8,%esp` + `mov` is
//  NOT an /O2 marker: the register ABI forces it, because arguments 4 and 5
//  have to land at 0xc(%esp) and 0x10(%esp) over the register holes and
//  pushes cannot skip slots.)
//
//  DECODED, every instruction.  __thiscall, three arguments, `ret $0xc`.
//  A linear search of a token stream for a name.
//
//      10023130  push %ebp / mov %esp,%ebp / sub $0x20,%esp
//                push %edi / push %edi / push %esi / push %ebx
//                mov 0x10(%ebp),%edi / mov %ecx,%ebx
//                test %edi,%edi / jne 0x10023146
//                xor %edi,%edi / inc %edi
//      10023146  mov %ebx,%ecx / call 0x100230d4
//                test %eax,%eax / mov 0xc(%ebp),%esi
//                je 0x100231f2
//                xor %eax,%eax / mov %eax,-0x4(%ebp)
//      1002315d  cmp 0x100(%ebx),%edi / jg 0x100231ab
//                test %esi,%esi / je 0x10023183
//                mov 0x8(%ebp),%eax / mov (%ebx),%edx
//                push %eax / push %edx / call 0x10080910
//                xor %edx,%edx / add $0x8,%esp
//                test %eax,%eax / sete %dl / mov %edx,%eax / jmp 0x1002319b
//      10023183  ... the same, calling 0x100874b0 ...
//      1002319b  test %eax,%eax / je 0x100231ab
//                xor %eax,%eax / inc %eax
//      100231a2  pop %ebx / pop %esi / pop %edi
//                mov %ebp,%esp / pop %ebp / ret $0xc
//      100231ab  mov 0x104(%ebx),%eax / test %eax,%eax / je 0x100231f2
//                add $0xffffffe8,%esp
//                lea 0x100(%ebx),%edx / lea 0x104(%ebx),%ecx
//                mov %ebx,0xc(%esp) / mov %edx,0x10(%esp)
//                mov %ecx,%edx / lea 0x108(%ebx),%ecx
//                call 0x10022e9c / add $0x18,%esp
//                test %eax,%eax / jne 0x1002315d
//                mov -0x4(%ebp),%eax / mov %eax,0x104(%ebx)
//                jmp 0x1002315d
//      100231f2  xor %eax,%eax / jmp 0x100231a2
//
//  WHAT PINS IT:
//    * `ret $0xc` with %ecx live-in at 0x1002313d: __thiscall, three
//      arguments.  The extra `push %edi` at 0x10023136 is the dead 4-byte
//      stack slot this compiler emits.
//    * 0x10080910 is the CRT strcmp (`test $0x3,%edx` alignment probe, then
//      `mov (%edx),%eax ; cmp (%ecx),%al ; or %al,%al`) and 0x100874b0 is
//      _stricmp (it opens on the locale table at 0x10091ed4, the same one
//      d3dr2d.cpp's toupper sits behind).  Argument 2 non-zero picks the
//      case-SENSITIVE one, so it is the case-sensitivity flag.
//    * both arms end `sete %dl ; mov %edx,%eax` and then the join RETESTS
//      with `test %eax,%eax ; je`.  The value is materialised and tested
//      again, which is an if/else through a single result VARIABLE, not a
//      `?:` folded into the branch.
//    * `cmp 0x100(%ebx),%edi` puts the argument on the left, so the source
//      is `nIndex <= m_n100`; `jg` is SIGNED, so both are int32.
//    * 0x10022e9c has a THREE-mov shim at 0x10022e90, so it is external and
//      takes at least three arguments; it reads 0x24(%esp) before its own
//      `sub` (= entry+0x14, argument 5) and 0x54(%esp) after (= entry+0x10,
//      argument 4), so it takes FIVE, and the caller's 24-byte reservation
//      with writes only at 0xc and 0x10 matches exactly.  Its first
//      argument is dereferenced as `cmpb $0x20,0x0(%ebp)`, so +0x104 is a
//      char* and the function is a tokeniser.
//    * 0x100230d4 is `push %ebx ; sub $0x20,%esp ; mov %ecx,%ebx` and then
//      reads the SAME +0x100/+0x104/+0x108 triple: a sibling member of this
//      class, __thiscall, no arguments.
//
//  COIN-FLIP: -0x4(%ebp) is written once with zero and read once, into
//  +0x104.  A direct `m_p104 = LTNULL;` would have been `movl $0x0` at /Os;
//  the stack home says a source LOCAL, spelled below as `pNull`.  If that
//  does not reproduce, the alternative is `m_p104 = LTNULL;` with no local
//  and the declaration deleted.
// ======================================================================= //

class C10023130
{
public:
	LTBOOL  sub_10023130(const char* pName, uint32 bCaseSensitive,
	                     int32 nIndex);
	LTBOOL  sub_100230d4();                             // 0x100230d4

	char*   m_pText;                                    // +0x00
	uint8   m_pad_04[0xfc];                             // +0x04
	int32   m_n100;                                     // +0x100
	char*   m_p104;                                     // +0x104
	void*   m_p108;                                     // +0x108
};

// 0x10022e9c -- five arguments; 1,2,3 in %eax/%edx/%ecx and 4,5 at
// 0xc(%esp) and 0x10(%esp) over the register holes.  Returns a flag.
