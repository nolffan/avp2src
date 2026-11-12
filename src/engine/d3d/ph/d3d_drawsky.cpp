// *********************************************************************** //
//  ph/d3d_drawsky.cpp -- Primal Hunt's d3d.ren, TU33, first object
//  (0x1002de40..0x1002e3fd), /O2.
//
//  The sky: the screen extent of the frame's sky-portal polygons (each
//  transformed, clipped and projected, the min/max kept), and the sky
//  draw that renders the sky world into that rectangle.  The map's TU33
//  holds two objects: this file's three initialisers (0x1002de40..
//  0x1002de70) open it, and drawsprite.cpp's two (0x1002e400, 0x1002e410)
//  open the next.
//
//  AvP2 1.0.9.6's counterpart is big5dca0.cpp (the extent, its clipper
//  inlined) and w17_1005f580.cpp.
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


// The profiling counters (TU14: 0x10013fb7, 0x10013fd5, 0x10013fe0).
#define CSTART_NONE			0

class CounterFinal
{
public:
					CounterFinal(unsigned long startMode=CSTART_NONE);
	unsigned long	m_Data[2];
};

typedef CounterFinal Counter;

void			cnt_StartCounterFinal(CounterFinal &cCounter);
unsigned long	cnt_EndCounterFinal(CounterFinal &cCounter);

inline void cnt_StartCounter(Counter &cCounter) { cnt_StartCounterFinal(cCounter); }
inline unsigned long cnt_EndCounter(Counter &cCounter) { return cnt_EndCounterFinal(cCounter); }

class CountAdder
{
	public:
		CountAdder(uint32 *pNum)
		{
			m_pNum = pNum;
			cnt_StartCounter(m_Counter);
		}
		~CountAdder()
		{
			*m_pNum += cnt_EndCounter(m_Counter);
		}
		Counter m_Counter;
		uint32 *m_pNum;
};


// ---------------------------------------------------------------------- //
//  The structures, as this file reads them.
// ---------------------------------------------------------------------- //
struct ViewParams
{
	uint8		m_pad_000[0x9c];
	LTMatrix	m_mCamera;                              // +0x09c
	uint8		m_pad_0dc[0x15c - 0xdc];
	LTMatrix	m_mWorldToCamera;                       // +0x15c
	LTMatrix	m_mFullTransform;                       // +0x19c
	uint8		m_pad_1dc[0x4c0 - 0x1dc];
	LTVector	m_SkyViewPos;                           // +0x4c0
};

// The view rectangle the sky is projected into (TU10's 0x10010383).
struct ViewBox
{
	float		m_fOffX;                                // +0x00
	float		m_fOffY;                                // +0x04
	float		m_fFocal;                               // +0x08
	float		m_fHalfW;                               // +0x0c
	float		m_fHalfH;                               // +0x10
	float		m_fNear;                                // +0x14
	float		m_fFar;                                 // +0x18
};

struct WorldVertex
{
	LTVector*	m_Vec;                                  // +0x00
	uint8		m_pad_04[0x18 - 0x04];
};

struct WorldPoly
{
	uint8		m_pad_00[0x54];
	uint16		m_nVerts;                               // +0x54
	uint8		m_pad_56[0x58 - 0x56];
	WorldVertex	m_Verts[1];                             // +0x58
};

struct SceneWorld
{
	uint8		m_pad_000[0x1a0];
	WorldPoly**	m_SkyPortals;                           // +0x1a0
	uint32		m_nSkyPortals;                          // +0x1a4
};

struct VisibleSet
{
	uint8		m_pad_000[0x2c];
	WorldPoly**	m_pSkyPortalPolies;                     // +0x2c
	uint8		m_pad_030[0x3c - 0x30];
	uint32		m_nSkyPortalPolies;                     // +0x3c
};

// The 32-byte clip vertex (TU01's ClipVert).
struct ClipVert
{
	LTVector	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	uint32		m_Diffuse;                              // +0x10
	uint32		m_Specular;                             // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
};

#define MAX_SKYPORTAL_VERTS		40

// The clip planes (TU06's ClipNear 0x1000b07d and ClipLeft 0x1000b1bc,
// TU03's ClipTop 0x100063e0, ClipRight 0x10006670, ClipBottom 0x10006900,
// ClipFar 0x10006ba0).
template<class V> LTBOOL ClipNear(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipLeft(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipTop(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipRight(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipBottom(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipFar(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);

struct SceneDesc
{
	uint8		m_pad_00[0xb8];
	int32		m_nSkyPortals;                          // +0xb8
	uint8		m_pad_bc[0xd4 - 0xbc];
	float		m_fSkyFarZ;                             // +0xd4
};

struct RenderStruct
{
	uint8		m_pad_00[0x6c];
	uint32		m_SkyTicks;                             // +0x6c
};

extern RenderStruct*	g_pStruct;                      // 0x1005df08
extern ViewParams		g_ViewParams;                 // 0x1005b790
extern SceneWorld*		g_pSceneWorld;                  // 0x1005c208
extern SceneDesc*		g_pSceneDesc;                   // 0x1005c14c (common_draw.cpp)
extern CD3DConVar		g_CV_UseD3DClip;                // 0x100622b0 (d3d_init.cpp)
extern void*			g_pClipScratch;                 // 0x1005bd14
extern uint32			g_nSkyPortalPoliesDrawn;        // 0x1005c150
extern int32			g_CV_DrawSky;                   // 0x1004a758
extern int32			g_CV_EnableSky;                 // 0x1004a75c

extern float			g_CV_SkyScale;                  // 0x1004a738

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0
extern void		d3d_InitViewBox(ViewBox *pBox, float fNearZ, float fFarZ, ViewParams *pContext,
					float fMinX, float fMinY, float fMaxX, float fMaxY);  // 0x10010383
extern int32	d3d_InitSceneContext(ViewParams *pContext, ViewBox *pBox,
					float fMinX, float fMinY, float fMaxX, float fMaxY,
					LTMatrix *pCamera, LTVector vScale);         // 0x100104cb
extern void		d3d_DrawSkyObjects();                   // 0x1001a2a1


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //

// The sky's frame counters, cleared when the renderer starts.
struct SkyCounters
{
				SkyCounters()
				{
					m_nPolies = 0;
					m_nTris = 0;
					m_nVerts = 0;
					m_nObjects = 0;
				}

	uint32		m_nPolies;
	uint32		m_nTris;
	uint32		m_nVerts;
	uint32		m_nObjects;
};

LTVector		g_vSkyCenter;                           // $E 0x1002de40
SkyCounters		g_SkyCounters;                          // 0x1007946c  $E 0x1002de50
CD3DConVar		g_CV_AllSkyPortals("AllSkyPortals", 0.0f, LTNULL, LTNULL);  // 0x10079428  $E 0x1002de70

float			g_SkyMinY;                              // 0x10079448
float			g_SkyMinX;                              // 0x1007944c
float			g_SkyMaxY;                              // 0x100787a0
float			g_SkyMaxX;                              // 0x100787a4

ViewParams	g_SkyContext;                           // 0x10079450


// Clip a polygon against the planes in nClipFlags, into the clip scratch
// buffer. FALSE when nothing is left.
inline LTBOOL d3d_ClipSkyPoly(uint32 nClipFlags, ClipVert** ppVerts, int32* pnVerts)
{
	ClipVert* pOut;
	ClipVert* pVerts;
	int32 nVerts;
	uint8 nearPlane, leftPlane, topPlane, rightPlane, bottomPlane, farPlane;

	if (g_CV_UseD3DClip.m_nValue)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut = (ClipVert*)g_pClipScratch;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!ClipNear(&nearPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x04)
	{
		if (!ClipLeft(&leftPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x08)
	{
		if (!ClipTop(&topPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x10)
	{
		if (!ClipRight(&rightPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x20)
	{
		if (!ClipBottom(&bottomPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x02)
	{
		if (!ClipFar(&farPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;
	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x1002de90 -- the screen rectangle the visible sky portals cover.
//  TRUE when it is more than a pixel wide and high.
// ---------------------------------------------------------------------- //
LTBOOL d3d_GetSkyPortalExtents()
{
	VisibleSet *pVisibleSet;
	WorldPoly **ppPoly, *pPoly;
	uint32 nPolies, iPoly, nClipFlags;
	ClipVert verts[MAX_SKYPORTAL_VERTS];
	ClipVert *pVerts, *pVert, *pEnd;
	int32 nVerts, i;
	LTVector vScreen;

	pVisibleSet = d3d_GetVisibleSet();

	g_SkyMinY = 10000.0f;
	g_SkyMinX = 10000.0f;
	g_SkyMaxY = -10000.0f;
	g_SkyMaxX = -10000.0f;

	if (g_CV_AllSkyPortals.m_nValue && g_pSceneWorld)
	{
		ppPoly = g_pSceneWorld->m_SkyPortals;
		nPolies = g_pSceneWorld->m_nSkyPortals;
		nClipFlags = 0x3d;
	}
	else
	{
		ppPoly = pVisibleSet->m_pSkyPortalPolies;
		nPolies = pVisibleSet->m_nSkyPortalPolies;
		nClipFlags = 0x3f;
	}

	for (iPoly=0; iPoly < nPolies; iPoly++)
	{
		pPoly = ppPoly[iPoly];

		for (i=0; i < pPoly->m_nVerts; i++)
			MatVMul_H(&verts[i].m_vPos, &g_ViewParams.m_mWorldToCamera, pPoly->m_Verts[i].m_Vec);

		pVerts = verts;
		nVerts = pPoly->m_nVerts;
		if (!d3d_ClipSkyPoly(nClipFlags, &pVerts, &nVerts))
			continue;

		pEnd = &pVerts[nVerts];
		for (pVert=pVerts; pVert != pEnd; pVert++)
		{
			MatVMul_H(&vScreen, &g_ViewParams.m_mFullTransform, &pVert->m_vPos);

			if (g_SkyMinX >= vScreen.x)
				g_SkyMinX = vScreen.x;

			if (g_SkyMinY >= vScreen.y)
				g_SkyMinY = vScreen.y;

			if (g_SkyMaxX <= vScreen.x)
				g_SkyMaxX = vScreen.x;

			if (g_SkyMaxY <= vScreen.y)
				g_SkyMaxY = vScreen.y;
		}

		g_nSkyPortalPoliesDrawn++;
	}

	return (g_SkyMaxX - g_SkyMinX) > 0.9f && (g_SkyMaxY - g_SkyMinY) > 0.9f;
}


// ---------------------------------------------------------------------- //
//  0x1002e280 -- draw the sky world through the rectangle the sky
//  portals cover.
// ---------------------------------------------------------------------- //
void d3d_DrawSky()
{
	ViewBox box;
	LTMatrix mCamera;
	LTVector vPos;

	CountAdder cntSky(&g_pStruct->m_SkyTicks);

	if (!g_CV_DrawSky || !g_CV_EnableSky)
		return;

	if (g_pSceneDesc->m_nSkyPortals <= 0)
		return;

	if (!d3d_GetSkyPortalExtents())
		return;

	d3d_InitViewBox(&box, 0.01f, g_pSceneDesc->m_fSkyFarZ, &g_ViewParams,
		g_SkyMinX, g_SkyMinY, g_SkyMaxX, g_SkyMaxY);

	mCamera = g_ViewParams.m_mCamera;
	vPos = g_ViewParams.m_SkyViewPos;
	mCamera.m[0][3] = vPos.x;
	mCamera.m[1][3] = vPos.y;
	mCamera.m[2][3] = vPos.z;

	d3d_InitSceneContext(&g_SkyContext, &box, g_SkyMinX, g_SkyMinY, g_SkyMaxX, g_SkyMaxY,
		&mCamera, LTVector(g_CV_SkyScale, g_CV_SkyScale, g_CV_SkyScale));

	d3d_DrawSkyObjects();
}
