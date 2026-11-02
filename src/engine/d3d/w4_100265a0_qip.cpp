// *********************************************************************** //
//
//  MODULE:   W4_100265A0_QIP (.CPP)  -- /O2 /Qip
//
//  PURPOSE:  0x10026670, which the d3d.ren link referenced by address and
//            nobody had written, and the converter it calls.
//
//  Flag set: /O2 /Qip.  0x10026670 calls 0x100265a8 with the source in
//  %eax and the destination in %edx and nothing pushed: the REGISTER
//  entry of icl's external two-entry pair whose stack entry
//  (`mov 0x4(%esp),%eax ; mov 0x8(%esp),%edx`) is 0x100265a0.  Caller and
//  callee must share a /Qip TU for that call to come out, so both are
//  written here.  Both bodies are followed by multi-byte lea-nops to the
//  next 16-byte boundary, the /O2 padding.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct LTMat { float m[4][4]; };

// Direct3D's D3DMATRIX, row-major _11.._44.
struct D3DMatrix { float m[4][4]; };


// ======================================================================= //
//  0x100265a0 -- LTMatrix (column vectors) to D3DMATRIX (row vectors):
//  the transpose.  The shipped body loads all sixteen source floats
//  (spilling seven) before the first store: sixteen scalar locals, all
//  read, then all written, is byte-identical.  One statement per
//  element was a near (89 bytes differ); copying *pIn to a local struct
//  is a `rep movsl` block copy (226 bytes), and a sixteen-float
//  D3DMATRIX-style constructor 320 bytes.
// ======================================================================= //
void sub_100265a0(const LTMat* pIn, D3DMatrix* pOut)
{
	float a00, a01, a02, a03, a10, a11, a12, a13, a20, a21, a22, a23, a30, a31, a32, a33;

	a00 = pIn->m[0][0];
	a01 = pIn->m[0][1];
	a02 = pIn->m[0][2];
	a03 = pIn->m[0][3];
	a10 = pIn->m[1][0];
	a11 = pIn->m[1][1];
	a12 = pIn->m[1][2];
	a13 = pIn->m[1][3];
	a20 = pIn->m[2][0];
	a21 = pIn->m[2][1];
	a22 = pIn->m[2][2];
	a23 = pIn->m[2][3];
	a30 = pIn->m[3][0];
	a31 = pIn->m[3][1];
	a32 = pIn->m[3][2];
	a33 = pIn->m[3][3];

	pOut->m[0][0] = a00;
	pOut->m[0][1] = a10;
	pOut->m[0][2] = a20;
	pOut->m[0][3] = a30;
	pOut->m[1][0] = a01;
	pOut->m[1][1] = a11;
	pOut->m[1][2] = a21;
	pOut->m[1][3] = a31;
	pOut->m[2][0] = a02;
	pOut->m[2][1] = a12;
	pOut->m[2][2] = a22;
	pOut->m[2][3] = a32;
	pOut->m[3][0] = a03;
	pOut->m[3][1] = a13;
	pOut->m[3][2] = a23;
	pOut->m[3][3] = a33;
}


// ======================================================================= //
//  0x10026670 -- hand the world, view and projection matrices to the
//  device: IDirect3DDevice7::SetTransform (vtable +0x2c) with
//  D3DTRANSFORMSTATE_WORLD (1), VIEW (2) and PROJECTION (3).  The device
//  global is reloaded for each call.
// ======================================================================= //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	void*   m_pad_00[11];                               // +0x00 .. +0x28
	long (__stdcall* SetTransform)(D3DDevice7*, uint32 nState,
	                               D3DMatrix* pMatrix);   // +0x2c
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

extern uint32 g_1008ba9c;                               // 0x1008ba9c  world
extern LTMat  g_Mat1008b95c;                            // 0x1008b95c  view
extern LTMat  g_1008b99c;                               // 0x1008b99c  projection

void sub_10026670()
{
	D3DMatrix mWorld;
	D3DMatrix mView;
	D3DMatrix mProj;

	sub_100265a0((const LTMat*)&g_1008ba9c, &mWorld);
	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, 1, &mWorld);
	sub_100265a0(&g_Mat1008b95c, &mView);
	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, 2, &mView);
	sub_100265a0(&g_1008b99c, &mProj);
	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, 3, &mProj);
}
