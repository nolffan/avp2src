// *********************************************************************** //
//
//  MODULE:   D3DMISC (.CPP)
//
//  PURPOSE:  The functions in d3d.ren whose shape is COMPILER-NEUTRAL.
//
//  d3d.ren was built with a compiler this project does not have (see
//  d3ddraw.cpp for the evidence). Almost every function it contains carries
//  one of three fingerprints cl does not emit: a frame pointer that /O2
//  omits, `xor %eax,%eax; inc %eax` for `return 1`, or a dummy 4-byte stack
//  slot opened with `push %esi` and closed with `add $0x4,%esp`.
//
//  A minority carry none of the three. The scan that finds them is worth
//  repeating whenever the enumeration changes -- of 1196 enumerated
//  functions exactly thirteen are small and free of all three:
//
//      reject any body containing 55 8b ec  (push %ebp; mov %esp,%ebp)
//                                 33 c0 40  (xor %eax,%eax; inc %eax)
//                                 83 c4 04 / 83 ec  (the dummy slot)
//
//  BUT the filter is necessary, not sufficient, and the boundary is sharper
//  than "no fingerprints". What actually reproduces is a function with NO
//  SCHEDULING FREEDOM -- three or four instructions whose order is forced.
//  Measured here:
//
//      d3d_IsInOptimized2D    6 / 6     BYTE-IDENTICAL
//      d3d_Slot_c0            3 / 3     BYTE-IDENTICAL
//      d3d_ResetCounters     23 / 23    BYTE-IDENTICAL
//      d3d_GetSurfaceInfo    48 / 48    size-exact, 11 bytes: pure register
//                                       naming, edx vs ecx for the out-pointer.
//                                       NOT closeable -- EVERY function in the
//                                       blit family takes its first pointer
//                                       into %edx, so this is the module-wide
//                                       compiler difference and no source
//                                       shape flips it. Do not chase it.
//      d3d_TransposeRotation 41 / 43    right instruction KINDS, wrong
//                                       schedule -- the shipped build keeps
//                                       three temps live across pairs and
//                                       spends %ebx to do it
//
//  So the two larger ones are decoded and correct and will stay near until
//  the compiler is known; they are kept because they cost nothing and because
//  they are two more measuring sticks. The transpose is a good one: getting
//  the instruction kinds right required knowing the swap is done on BIT
//  PATTERNS, not floats.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltmatrix.h"


extern LTBOOL g_bInOptimized2D;                         // 0x1008d7c4


// 0x10034c60 -- RenderStruct slot +0x9c, six bytes. An earlier pass declined
// to name this slot for want of evidence; the body settles it. It returns
// 0x1008d7c4, which is the same global End3D tests before it closes a batch
// (0x10032ebe) and the one StartOptimized2D refuses on (0x10034951). So the
// slot is IsInOptimized2D, and it is the exact counterpart of IsIn3D at
// +0x90. Nothing in lithtech.exe calls it, which is why the client side
// could not name it.
LTBOOL d3d_IsInOptimized2D()
{
	return g_bInOptimized2D;
}


// 0x10032e40 -- RenderStruct slot +0xc0, three bytes: `xor %eax,%eax; ret`.
// A slot that always refuses. Two dwords are pushed at its one call site and
// the answer is discarded, so the arguments are unrecoverable and it is named
// after its slot.
uint32 d3d_Slot_c0()
{
	return 0;
}


// 0x10036920 -- RenderStruct slot +0xd0, 48 bytes. Three OUT parameters, each
// separately null-checked, filled from the surface's +0x10, +0x0c and +0x14
// IN THAT ORDER -- so the second output is the lower offset, which is why
// this is written as three statements rather than a struct copy. The whole
// body is guarded on the surface itself.
// The object is a D3DRenderData (0x8c bytes; see d3dblit.cpp), whose +0x04 is
// an embedded DDSURFACEDESC2. So these three are that descriptor's fields:
// +0x0c is dwHeight, +0x10 dwWidth and +0x14 lPitch. The "second output takes
// the lower offset" oddity is nothing more than dwHeight sitting below
// dwWidth in DDSURFACEDESC2 -- the caller asks for width, height, pitch.
struct D3DRenderDataHead
{
	uint32 m_pSurface;                                  // +0x00
	uint32 m_nDescSize;                                 // +0x04  dwSize
	uint32 m_nDescFlags;                                // +0x08  dwFlags
	uint32 m_nHeight;                                   // +0x0c  dwHeight
	uint32 m_nWidth;                                    // +0x10  dwWidth
	uint32 m_nPitch;                                    // +0x14  lPitch
};

void d3d_GetSurfaceInfo(D3DRenderDataHead* pSurface, uint32* pOutWidth,
                        uint32* pOutHeight, uint32* pOutPitch)
{
	if (pSurface)
	{
		if (pOutWidth)
			*pOutWidth = pSurface->m_nWidth;

		if (pOutHeight)
			*pOutHeight = pSurface->m_nHeight;

		if (pOutPitch)
			*pOutPitch = pSurface->m_nPitch;
	}
}


// 0x1001e420 -- transpose the 3x3 rotation part of an LTMatrix in place.
// The offsets say 4x4: m[0][1] is +0x04 against m[1][0] at +0x10, m[0][2] is
// +0x08 against m[2][0] at +0x20, and m[1][2] is +0x18 against m[2][1] at
// +0x24. The translation column and the bottom row are left alone.
//
// Written as three temp-swaps because that is what the shipped scheduling
// shows: each pair loads BOTH halves before storing either, and the second
// store of a pair is deferred past the next pair's loads.
// The swap is done on the BIT PATTERNS, not on floats: the shipped code moves
// the elements with integer `mov`s. Writing `float fTemp` gets x87 loads and
// stores instead, which is a different function.
void d3d_TransposeRotation(LTMatrix* pMat)
{
	uint32* pM = (uint32*)pMat->m;
	uint32  nTemp;

	nTemp  = pM[1];
	pM[1]  = pM[4];
	pM[4]  = nTemp;

	nTemp  = pM[2];
	pM[2]  = pM[8];
	pM[8]  = nTemp;

	// The third pair is written the other way round from the first two:
	// shipped loads +0x18 before +0x24 and writes +0x24 first.
	nTemp  = pM[9];
	pM[9]  = pM[6];
	pM[6]  = nTemp;
}


// 0x1001eab0 -- clear four adjacent counters, highest address first, all
// from one zeroed register.
uint32 g_nCounter9c;                                    // 0x1008b89c
uint32 g_nCounterA0;                                    // 0x1008b8a0
uint32 g_nCounterA4;                                    // 0x1008b8a4
uint32 g_nCounterA8;                                    // 0x1008b8a8

void d3d_ResetCounters()
{
	g_nCounterA8 = 0;
	g_nCounterA4 = 0;
	g_nCounterA0 = 0;
	g_nCounter9c = 0;
}
