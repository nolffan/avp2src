// *********************************************************************** //
//  ph/drawgouraud.cpp -- Primal Hunt's d3d.ren, TU24
//  (0x100238a6..0x10024534), /O1.
//
//  Gouraud world polygons and the multipass gouraud path (the
//  MultipassGouraud console variable): queueing a polygon, filling the
//  TL vertices with the gamma-corrected vertex colours, and the blend
//  setup.
//
//  Not yet written here: the multipass draw (0x100238e2), the
//  poly-to-vertex-buffer passes (0x10023c66, 0x10024079, 0x1002438e,
//  0x10024443) and the queue flushes (0x100242c2, 0x1002430a).
// *********************************************************************** //

#include "ltbasedefs.h"


// The renderer's console variable (TU12's, constructor 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32		m_nValue;
	float		m_fValue;
	float		m_fDefault;
	int32*		m_pnValue;
	float*		m_pfValue;
	const char*	m_pName;
	void*		m_hParam;
	CD3DConVar*	m_pNext;
};

// IDirect3DDevice7's SetRenderState (index 20).
class CD3DDevice
{
public:
	virtual long __stdcall QueryInterface(void*, void**) = 0;
	virtual uint32  __stdcall AddRef() = 0;
	virtual uint32  __stdcall Release() = 0;
	virtual long __stdcall GetCaps(void*) = 0;
	virtual long __stdcall EnumTextureFormats(void*, void*) = 0;
	virtual long __stdcall BeginScene() = 0;
	virtual long __stdcall EndScene() = 0;
	virtual long __stdcall GetDirect3D(void**) = 0;
	virtual long __stdcall SetRenderTarget(void*, uint32) = 0;
	virtual long __stdcall GetRenderTarget(void**) = 0;
	virtual long __stdcall Clear(uint32, void*, uint32, uint32, float, uint32) = 0;
	virtual long __stdcall SetTransform(uint32, void*) = 0;
	virtual long __stdcall GetTransform(uint32, void*) = 0;
	virtual long __stdcall SetViewport(void*) = 0;
	virtual long __stdcall MultiplyTransform(uint32, void*) = 0;
	virtual long __stdcall GetViewport(void*) = 0;
	virtual long __stdcall SetMaterial(void*) = 0;
	virtual long __stdcall GetMaterial(void*) = 0;
	virtual long __stdcall SetLight(uint32, void*) = 0;
	virtual long __stdcall GetLight(uint32, void*) = 0;
	virtual long __stdcall SetRenderState(uint32 state, uint32 value) = 0;
};

extern CD3DDevice*	g_pD3DDevice;                       // 0x100638c8

#define D3DRS_SRCBLEND		19
#define D3DRS_DESTBLEND		20
#define D3DBLEND_ONE		2
#define D3DBLEND_SRCALPHA	5


// A polygon vertex as the world stores it, and the TL vertex drawn.
struct SPolyVertex
{
	LTVector*	m_pVec;                                 // +0x00
	float		m_U;                                    // +0x04
	float		m_V;                                    // +0x08
	uint8		m_pad_0c[0x14 - 0x0c];
	uint8		m_Color[3];                             // +0x14 (b, g, r)
	uint8		m_pad_17;
};

struct TLVertex
{
	void		SetUV(float u, float v)	{ m_TU = u; m_TV = v; }

	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
	uint8		m_Color[4];                             // +0x10 (b, g, r, a)
	uint32		m_Specular;                             // +0x14
	float		m_TU;                                   // +0x18
	float		m_TV;                                   // +0x1c
};

struct QueuedPoly
{
	uint8		m_pad_00[0x0c];
	uint32		m_ClipFlags;                            // +0x0c
};

extern uint8	g_GammaTableR[256];                     // 0x1005f79c
extern uint8	g_GammaTableG[256];                     // 0x1005f89c
extern uint8	g_GammaTableB[256];                     // 0x1005f99c
extern uint8	g_LightTableR[256];                     // 0x1005fa9c
extern uint8	g_LightTableG[256];                     // 0x1005fb9c
extern uint8	g_LightTableB[256];                     // 0x1005fc9c
extern uint8	g_VertexAlpha;                          // 0x1005d20c
extern void		(__fastcall *g_pfnSetupSpecular)(TLVertex* pVert, uint32* pSpecular);  // 0x1005e1c4
extern uint32	g_ClipFlags;                            // 0x1005bd0c
extern float	g_fTexelSizeU;                          // 0x100672a8
extern float	g_fTexelSizeV;                          // 0x100672ac
extern void*	g_GouraudPolyList;                      // 0x1005e728

extern QueuedPoly*	d3d_QueuePoly(void* pPoly, void* pList, uint32 nType);  // 0x10007ddb


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_MultipassGouraud("MultipassGouraud", 1.0f, LTNULL, LTNULL);  // 0x1006b658  $E 0x100238a6/0x100238ab


// ---------------------------------------------------------------------- //
//  0x100238c5
// ---------------------------------------------------------------------- //
void d3d_QueueGouraudPoly(void* pPoly)
{
	d3d_QueuePoly(pPoly, &g_GouraudPolyList, 1)->m_ClipFlags = g_ClipFlags;
}


// ---------------------------------------------------------------------- //
//  0x10023f80 / 0x10023ffc -- fill TL vertices from a polygon's.
// ---------------------------------------------------------------------- //
void d3d_SetupGouraudVerts(TLVertex* pOut, SPolyVertex* pIn, int nVerts)
{
	int i;

	for (i=0; i < nVerts; i++)
	{
		pOut->m_Vec.x = pIn->m_pVec->x;
		pOut->m_Vec.y = pIn->m_pVec->y;
		pOut->m_Vec.z = pIn->m_pVec->z;
		pOut->m_Color[2] = g_GammaTableR[pIn->m_Color[2]];
		pOut->m_Color[1] = g_GammaTableG[pIn->m_Color[1]];
		pOut->m_Color[0] = g_GammaTableB[pIn->m_Color[0]];
		pOut->m_Color[3] = g_VertexAlpha;
		g_pfnSetupSpecular(pOut, &pOut->m_Specular);
		pOut->SetUV(pIn->m_U, pIn->m_V);
		pIn++;
		pOut++;
	}
}

void d3d_SetupLightmapVerts(TLVertex* pOut, SPolyVertex* pIn, int nVerts)
{
	int i;

	for (i=0; i < nVerts; i++)
	{
		pOut->m_Vec.x = pIn->m_pVec->x;
		pOut->m_Vec.y = pIn->m_pVec->y;
		pOut->m_Vec.z = pIn->m_pVec->z;
		pOut->m_Color[2] = g_LightTableR[pIn->m_Color[2]];
		pOut->m_Color[1] = g_LightTableG[pIn->m_Color[1]];
		pOut->m_Color[0] = g_LightTableB[pIn->m_Color[0]];
		pOut->m_Color[3] = g_VertexAlpha;
		g_pfnSetupSpecular(pOut, &pOut->m_Specular);
		pOut->SetUV(pIn->m_U, pIn->m_V);
		pIn++;
		pOut++;
	}
}


// ---------------------------------------------------------------------- //
//  0x1002426c -- scale texture coordinates to the bound texture's texels.
// ---------------------------------------------------------------------- //
void d3d_ScaleTexCoords(TLVertex* pVerts, uint32 nVerts)
{
	while (nVerts--)
	{
		pVerts->SetUV(g_fTexelSizeU * pVerts->m_TU, g_fTexelSizeV * pVerts->m_TV);
		pVerts++;
	}
}


// ---------------------------------------------------------------------- //
//  0x1002429c
// ---------------------------------------------------------------------- //
void d3d_SetGouraudBlend(LTBOOL bAdditive)
{
	if (!bAdditive)
	{
		g_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		g_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	}
}
