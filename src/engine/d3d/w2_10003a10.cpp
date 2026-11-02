// *********************************************************************** //
//
//  w2_10003a10.cpp -- the seven texture-coordinate generators at
//  0x10003a10..0x10003ba8 (written for the d3d.ren link).
//
//  All seven are code addresses materialised as immediates by
//  big43e10.cpp (`movl $0x10003a80,0x434(%esi)` and the argument stores
//  into sub_10044f74), and called through a pointer by 0x10008fd0
//  (0x1000933c / 0x100097fa: `mov %esi,%ecx ; push %edi ; mov %ebx,%edx ;
//  call *%eax`).  Every body reads %ecx and %edx live-in, takes ONE stack
//  dword and returns `ret $0x4`: that is __fastcall with three arguments.
//
//  The enumerator listed 0x10003a10, 0x10003a40 and 0x10003aa0 with size 0
//  (their only references are those immediates), but they are real bodies
//  of 38, 64 and 48 bytes, each ended by `ret $0x4` and padded to 16.
//
//  Two families, told apart by which register is the destination:
//
//    * the "uv" generators stored into +0x434 (0x10003a10, 0x10003a40,
//      0x10003a80, 0x10003aa0): %ecx = the OUTPUT vertex, %edx = the
//      source vertex (its uv at +0x18/+0x1c), stack = a uv pair;
//    * the "model uv" generators stored into +0x42c and handed to
//      0x10008fd0 (0x10003ad0, 0x10003ae0, 0x10003b50): %ecx = the model
//      drawer, %edx = the model vertex (normal at +0x14), stack = the
//      output vertex.
//
//  /O2: every function is padded to 16 with multi-byte lea-nops.
//
//  Six of the seven are byte-identical.  Two levers, both measured:
//    * the v sums are evaluated in double: `flds 0x1008ac94 ; fadds
//      0x4(%eax)` (global loaded first) is what `(double)g + pUV[1]`
//      gives, and no float spelling or operand order does;
//    * a value pair computed before either store is an inline setter's
//      two arguments (SetUV); a store between them is two assignments.
//  0x10003ae0 is NEAR (18 bytes): both rows of the 3x3 at +0x3d0 are
//  computed before either store (the shipped `fxch ; fstps ; fstps`), so
//  it is SetUV again -- over a LTVector temp, the shipped 12-byte frame,
//  filled by the SDK's LTMatrix::Apply3x3 (the rows at +0x3d0/+0x3e0 are
//  an LTMatrix's 16-byte rows).  58 -> 18 bytes.  Left: in
//  each row the shipped z term loads the vertex first (`flds 0x1c(%edx) ;
//  fmuls 0x3d8(%ecx)`) where ours loads the matrix; icl canonicalises
//  every float spelling tried (all term orders, LTVector Dot/VEC_DOT,
//  MatVMul_3x3, Apply3x3, raw float pointers, a z local, and 384
//  double-promotion variants of the v row) to the matrix-first order.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// 0x1008ac90 / 0x1008ac94 -- big47230.cpp's half-texel offsets.
extern float g_f1008ac90;                                   // 0x1008ac90
extern float g_f1008ac94;                                   // 0x1008ac94
// 0x1008ac74 -- the scale 0x10003a40 applies to the second uv pair.  Only
// read (0x10003a65, 0x10003a73); nothing in .text writes it by address.
extern float g_f1008ac74;                                   // 0x1008ac74


// The output vertex of the uv family: two texture pairs (+0x18 and +0x20),
// bigb910.cpp's 0x28-byte RVert layout.
struct UVVert3a10
{
	uint8   m_pad_00[0x18];                                 // +0x00
	float   m_fU18;                                         // +0x18
	float   m_fV1c;                                         // +0x1c
	float   m_fU20;                                         // +0x20
	float   m_fV24;                                         // +0x24

	// Both values are computed before either store in 0x10003a10 and both
	// loads precede both stores in 0x10003a80: an inline setter's two
	// arguments, not two assignments (0x10003a40 stores u first).
	void SetUV(float fU, float fV) { m_fU18 = fU; m_fV1c = fV; }
};

// The model drawer (big8fd0.cpp's CD3DModelDraw): only the members the
// three model-uv generators read.
struct ModelDraw3a10
{
	uint8   m_pad_000[0x3d0];                               // +0x000
	LTMatrix m_Mat3d0;                                      // +0x3d0
	uint8   m_pad_410[0x45c - 0x410];                       // +0x410
	float   m_fOffU45c;                                     // +0x45c
	float   m_fOffV460;                                     // +0x460
	float   m_fScaleU464;                                   // +0x464
	float   m_fScaleV468;                                   // +0x468
	uint8   m_pad_46c[0x470 - 0x46c];                       // +0x46c
	float   m_fScale470;                                    // +0x470
	uint8   m_pad_474[0x674 - 0x474];                       // +0x474
	LTVector m_vLightDir674;                                // +0x674
};

// big8fd0.cpp's ModelVert: the normal at +0x14.
struct ModelVert3a10
{
	uint8   m_pad_00[0x14];                                 // +0x00
	LTVector m_vNormal14;                                   // +0x14
};


// 0x10003a10 -- uv pair plus the half-texel offset, both computed before
// either store.
extern "C" void __fastcall sub_10003a10(UVVert3a10* pOut, UVVert3a10* pSrc,
                                        float* pUV)
{
	pOut->SetUV(pUV[0] + g_f1008ac90,
	            (float)((double)g_f1008ac94 + pUV[1]));
}

// 0x10003a40 -- as 0x10003a10, and the second pair is the first scaled by
// g_f1008ac74 (both RE-READ from the output vertex).
extern "C" void __fastcall sub_10003a40(UVVert3a10* pOut, UVVert3a10* pSrc,
                                        float* pUV)
{
	pOut->m_fU18 = pUV[0] + g_f1008ac90;
	pOut->m_fV1c = (float)((double)g_f1008ac94 + pUV[1]);
	pOut->m_fU20 = pOut->m_fU18 * g_f1008ac74;
	pOut->m_fV24 = pOut->m_fV1c * g_f1008ac74;
}

// 0x10003a80 -- copy the source vertex's uv.
extern "C" void __fastcall sub_10003a80(UVVert3a10* pOut, UVVert3a10* pSrc,
                                        float* pUV)
{
	pOut->SetUV(pSrc->m_fU18, pSrc->m_fV1c);
}

// 0x10003aa0 -- offset uv pair into +0x18, the source's uv into +0x20.
extern "C" void __fastcall sub_10003aa0(UVVert3a10* pOut, UVVert3a10* pSrc,
                                        float* pUV)
{
	float fU = pUV[0] + g_f1008ac90;
	pOut->m_fU18 = fU;
	pOut->m_fV1c = (float)((double)g_f1008ac94 + pUV[1]);
	pOut->m_fU20 = pSrc->m_fU18;
	pOut->m_fV24 = pSrc->m_fV1c;
}

// 0x10003ad0 -- the "no texture coordinates" generator: does nothing.
extern "C" void __fastcall sub_10003ad0(ModelDraw3a10* pDraw,
                                        ModelVert3a10* pVert, UVVert3a10* pOut)
{
}

// 0x10003ae0 -- environment map: the normal through the 3x3 rows at
// +0x3d0 / +0x3e0, then scale (+0x464/+0x468) and offset (+0x45c/+0x460).
extern "C" void __fastcall sub_10003ae0(ModelDraw3a10* pDraw,
                                        ModelVert3a10* pVert, UVVert3a10* pOut)
{
	LTVector vT;

	pDraw->m_Mat3d0.Apply3x3(pVert->m_vNormal14, vT);
	pOut->SetUV(vT.x * pDraw->m_fScaleU464 + pDraw->m_fOffU45c,
	            vT.y * pDraw->m_fScaleV468 + pDraw->m_fOffV460);
}

// 0x10003b50 -- the normal against the light direction at +0x674; a
// positive dot product scaled by +0x470 goes into both u and v, anything
// else gives zero.
extern "C" void __fastcall sub_10003b50(ModelDraw3a10* pDraw,
                                        ModelVert3a10* pVert, UVVert3a10* pOut)
{
	float fDot = pVert->m_vNormal14.Dot(pDraw->m_vLightDir674);
	if (fDot > 0.0f)
		pOut->m_fU18 = pOut->m_fV1c = fDot * pDraw->m_fScale470;
	else
		pOut->m_fU18 = pOut->m_fV1c = 0.0f;
}
