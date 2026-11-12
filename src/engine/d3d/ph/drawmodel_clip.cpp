// *********************************************************************** //
//
//  ph/drawmodel_clip.cpp -- Primal Hunt's d3d.ren, TU04:
//  0x10007930..0x10008c72, /O1.
//
//  World polygon drawing: the deferred polygons, texture binding, the
//  lightmapped polygon draw and its clipper, which is ph/d3d_clip.h's
//  ClipPoly over the 40-byte vertex (its out-of-line copies are this
//  object's: ClipPoly<ClipVert2> 0x10008779, ClipNear and ClipLeft
//  0x10008895, 0x100089cc, the edge functions 0x10008b01..0x10008c72).
//  The name is the map's guess.
//
//  The SDK and StdLith inlines this object calls out of line are its
//  COMDATs: sb_Allocate (0x10007e36), LTMatrix::Inverse (0x10008197),
//  LTMatrix::Apply4x4 (0x1000832e), _CVector::MagSqr (0x100085d3),
//  MatVMul_InPlace_H (0x1000868d), with a counter adder's destructor
//  (0x100083ae), each after the first function to use it.
//
//  The lightmapped polygon (0x10007b41) draws through the light tables
//  and the screen-scale terms, its dynamic lights either as vertex
//  colours (0x100083ec, LMDynamic off) or as small additive lightmap
//  textures (0x10007e5d, which brings Inverse, Apply4x4 and the counter
//  adder's destructor). g_fScreenScale*/g_fScreenCenter* are ordinary
//  globals: drawlightmaps.cpp (TU40) reads them.
//
//  Near, and why:
//    * 0x10007b41: size-exact; the fog StateSet and pVerts trade stack
//      slots (-0x14/-0xc), and the final g_nTLVertsUsed add uses eax for
//      retail's edx. No declaration order or scope moved the slots.
//    * 0x10007e5d: size-exact; the second dot product's term order
//      (up.y, up.z, up.x in retail), cl 8966's choice.
//    * 0x100079e4 (d3d_SetTexture): size-exact; retail tail-merges the two
//      d3d_SetTextureDirect calls into one block after the create path.
//
// *********************************************************************** //

#include <d3d.h>
#include <math.h>

#include "ltbasedefs.h"
#include "..\..\LT2\lithshared\stdlith\struct_bank.h"




// The render context's projection, and the texture size the half-texel
// offsets are scaled for.
struct SharedTexture;

struct RenderContext
{
	SharedTexture* m_pMainWorld;                            // +0x00
	float       m_fCenterX;                                 // +0x04
	float       m_fCenterY;                                 // +0x08
	float       m_fWidth;                                   // +0x0c
	float       m_fHeight;                                  // +0x10
};

// A texture stage's texel size (lmsurfaces.cpp's TexPair).
struct TexPair
{
	void        Init(float fU, float fV) { u = fU; v = fV; }

	float       u;
	float       v;
};

struct RenderPoly;

// A lightmapped polygon deferred until the end of the frame.
struct DeferredPiece
{
	RenderPoly* m_pPiece;                                   // +0x00
	uint8       m_pad_04[0x0c - 0x04];
	uint32      m_ClipFlags;                                // +0x0c
	DeferredPiece* m_pNext;                                 // +0x10
};

extern RenderContext*   g_pRenderContext;                   // 0x1005b778
extern TexPair          g_TexelSize[];                      // 0x100672a8
extern uint32           g_TextureStage;                     // 0x100622d0
extern uint32           g_iCurTextureStage;                 // 0x100638c0
extern uint32           g_ClipFlags;                        // 0x1005bd0c
extern StructBank       g_DeferredPolyBank;                 // 0x1005e1f0

int     d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD); // 0x100079e4
void    d3d_DrawDeferredPiece(RenderPoly* pPoly);           // 0x10007b41

float                   g_fScreenScaleX;                    // 0x10052bd8
float                   g_fScreenScaleY;                    // 0x10052bdc
float                   g_fScreenCenterX;                   // 0x10053fe0
float                   g_fScreenCenterY;                   // 0x10053fe4
static DeferredPiece*   g_pDeferredPieces;                  // 0x10053fe8


// 0x10007930
void d3d_SetupScreenScale()
{
	if (g_pRenderContext)
	{
		g_fScreenScaleX = 1.0f / g_pRenderContext->m_fWidth * g_TexelSize[g_TextureStage].u;
		g_fScreenScaleY = 1.0f / g_pRenderContext->m_fHeight * g_TexelSize[g_TextureStage].v;
		*(uint32*)&g_fScreenCenterX = *(uint32*)&g_pRenderContext->m_fCenterX;
		*(uint32*)&g_fScreenCenterY = *(uint32*)&g_pRenderContext->m_fCenterY;
	}
}

// 0x10007976 -- draw the deferred pieces and free them.
void d3d_DrawDeferredPieces()
{
	DeferredPiece *pCur, *pNext;

	if (!g_pDeferredPieces)
		return;

	d3d_SetTexture(g_pRenderContext->m_pMainWorld, g_iCurTextureStage, 0);
	d3d_SetupScreenScale();

	pCur = g_pDeferredPieces;
	while (pCur)
	{
		pNext = pCur->m_pNext;
		g_ClipFlags = pCur->m_ClipFlags;
		d3d_DrawDeferredPiece(pCur->m_pPiece);
		sb_Free(&g_DeferredPolyBank, pCur);
		pCur = pNext;
	}

	g_pDeferredPieces = LTNULL;
}


// ----------------------------------------------------------------------- //
//  Texture binding.
// ----------------------------------------------------------------------- //

// A texture in video memory (d3d_texture.cpp's RTexture), with what the
// binding reads.
class RTexture
{
public:
	void*       m_pVTable;                                  // +0x00
	float       m_fInvWidth;                                // +0x04
	float       m_fInvHeight;                               // +0x08
	LPDIRECTDRAWSURFACE7 m_pSurface;                        // +0x0c
	uint32      m_MemoryUse;                                // +0x10
	uint16      m_FrameCode;                                // +0x14
	uint16      m_AlphaRef;                                 // +0x16
	RTexture*   m_pTexture;                                 // +0x18
	LTLink      m_Link;                                     // +0x1c
	uint16      m_Width;                                    // +0x28
	uint16      m_Height;                                   // +0x2a
	SharedTexture* m_pSharedTexture;                        // +0x2c
	RTexture*   m_pNext;                                    // +0x30
	uint8       m_pad_34[0x42 - 0x34];                      // +0x34
	uint16      m_Stage;                                    // +0x42
	uint16      m_LOD;                                      // +0x44
};

class CRenderStateList;

struct SharedTexture
{
	uint8       m_pad_00[0x0c];                             // +0x00
	RTexture*   m_pRenderData;                              // +0x0c
	uint8       m_pad_10[0x30 - 0x10];                      // +0x10
	uint16      m_FrameCode;                                // +0x30
	uint8       m_pad_32[0x34 - 0x32];                      // +0x32
	CRenderStateList* m_pStateList;                         // +0x34
};

struct RenderStruct
{
	uint8       m_pad_00[0x48];                             // +0x00
	uint32      m_TextureMemoryUsed;                        // +0x48
};

extern RenderStruct*        g_pStruct;                      // 0x1005df08
extern LPDIRECT3DDEVICE7    g_pD3DDevice;                   // 0x100638c8
extern LTLink               g_RTextures;                    // 0x10068300
extern RTexture*            g_pCurTextures[];               // 0x10067270
extern uint16               g_CurFrameCode;                 // 0x1005d238
extern uint16               g_CurTextureFrameCode;          // 0x1005d250
extern uint32               g_nTextureChanges;              // 0x1005d22c

RTexture* d3d_CreateRenderTexture(SharedTexture* pTexture, uint32 iStage,
                                  LTBOOL bStageCopy);       // 0x10020cd0
void d3d_SetTextureDirect(RTexture* pTexture);              // 0x10007a89

// A texture's copy for a stage (d3d_matrix.cpp's d3d_GetStageTexture).
inline RTexture* d3d_FindStageTexture(RTexture* pTexture, uint8 iStage)
{
	while (pTexture)
	{
		if (pTexture->m_Stage == (uint16)iStage)
			return pTexture;

		pTexture = pTexture->m_pNext;
	}

	return LTNULL;
}

// 0x100079e4 -- bind a shared texture to a stage at a mip level: its copy
// for that stage, made (and chained to the others) if there is none yet.
int d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD)
{
	RTexture *pRenderData, *pCur;

	if (!pTexture)
		return LTFALSE;

	pRenderData = pTexture->m_pRenderData;
	pTexture->m_FrameCode = g_CurTextureFrameCode;

	pCur = LTNULL;
	if (pRenderData)
		pCur = d3d_FindStageTexture(pRenderData, (uint8)iStage);

	if (!pCur || pCur != g_pCurTextures[iStage])
	{
		if (!pCur)
		{
			pRenderData = pTexture->m_pRenderData;
			if (pRenderData)
			{
				pCur = d3d_CreateRenderTexture(pTexture, iStage, LTTRUE);
				if (!pCur)
					return LTFALSE;

				pCur->m_pNext = pRenderData->m_pNext;
				pRenderData->m_pNext = pCur;
			}
			else
			{
				pCur = d3d_CreateRenderTexture(pTexture, iStage, LTFALSE);
				if (!pCur)
					return LTFALSE;
			}
		}

		d3d_SetTextureDirect(pCur);
	}

	if (pCur->m_LOD != iLOD)
	{
		pCur->m_pSurface->SetLOD(iLOD);
		pCur->m_LOD = (uint16)iLOD;
	}

	return LTTRUE;
}

// 0x10007a89 -- put a texture on its stage: first use this frame moves it
// to the end of the texture list (most recently used) and counts its
// memory.
void d3d_SetTextureDirect(RTexture* pTexture)
{
	g_pCurTextures[pTexture->m_Stage] = pTexture;

	if (pTexture->m_FrameCode != g_CurFrameCode)
	{
		dl_Remove(&pTexture->m_Link);
		dl_Insert(g_RTextures.m_pPrev, &pTexture->m_Link);
		g_pStruct->m_TextureMemoryUsed += pTexture->m_MemoryUse;
		pTexture->m_FrameCode = g_CurFrameCode;
	}

	g_pD3DDevice->SetTexture(pTexture->m_Stage & 0xFF, pTexture->m_pSurface);
	if (pTexture->m_AlphaRef)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, pTexture->m_AlphaRef);

	g_TexelSize[pTexture->m_Stage].Init(pTexture->m_fInvWidth, pTexture->m_fInvHeight);
	g_nTextureChanges++;
}


// ----------------------------------------------------------------------- //
//  The lightmapped polygon.
// ----------------------------------------------------------------------- //

#define MAX_LMPOLY_VERTS    128
#define FVF_TLVERTEX        (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

// A queued polygon (d3d_draw.cpp's DeferredPoly).
struct QueuedPoly
{
	void*       m_pPoly;                                    // +0x00
	uint32      m_iFirstVert;                               // +0x04
	uint32      m_nVerts;                                   // +0x08
	uint32      m_ClipFlags;                                // +0x0c
	QueuedPoly* m_pNext;                                    // +0x10
	uint32      m_Type;                                     // +0x14
};

// A polygon vertex as the world stores it, and the TL vertex drawn.
struct SPolyVertex
{
	LTVector*   m_pVec;                                     // +0x00
	float       m_U;                                        // +0x04
	float       m_V;                                        // +0x08
	uint8       m_pad_0c[0x14 - 0x0c];
	uint8       m_Color[3];                                 // +0x14 (b, g, r)
	uint8       m_pad_17;
};

struct TLVertex
{
	LTVector    m_Vec;                                      // +0x00
	float       m_RHW;                                      // +0x0c
	uint8       m_Color[4];                                 // +0x10 (b, g, r, a)
	uint32      m_Specular;                                 // +0x14
	float       m_TU;                                       // +0x18
	float       m_TV;                                       // +0x1c
};

struct RenderPlane
{
	float       DistTo(LTVector& vec) { return (m_Normal.x*vec.x + m_Normal.y*vec.y + m_Normal.z*vec.z) - m_Dist; }

	LTVector    m_Normal;                                   // +0x00
	float       m_Dist;                                     // +0x0c
};

// A dynamic light (its +0x38 slot answers the light's radius).
class LightObject
{
public:
	virtual void    Slot00();
	virtual void    Slot04();
	virtual void    Slot08();
	virtual void    Slot0c();
	virtual void    Slot10();
	virtual void    Slot14();
	virtual void    Slot18();
	virtual void    Slot1c();
	virtual void    Slot20();
	virtual void    Slot24();
	virtual void    Slot28();
	virtual void    Slot2c();
	virtual void    Slot30();
	virtual void    Slot34();
	virtual float   GetRadius(LightObject* pObj);           // +0x38

	uint8       m_pad_004[0x94 - 0x04];
	uint8       m_ColorR;                                   // +0x94
	uint8       m_ColorG;                                   // +0x95
	uint8       m_ColorB;                                   // +0x96
	uint8       m_pad_097[0x1b0 - 0x97];
	float       m_LightRadius;                              // +0x1b0
};

// A dynamic light touching a polygon, and where it is (lmdynamic.cpp's
// LMDynamicLight).
struct LMDynamicLight
{
	LMDynamicLight* m_pNext;                                // +0x00
	LightObject*    m_pLight;                               // +0x04
	LTVector        m_vPos;                                 // +0x08
};

struct RenderPoly
{
	uint8       m_pad_00[0x14];
	uint16      m_PageFlags;                                // +0x14
	uint8       m_pad_16[0x28 - 0x16];
	RenderPlane* m_pPlane;                                  // +0x28
	void*       m_pSurface;                                 // +0x2c
	LMDynamicLight* m_pLights;                              // +0x30
	uint8       m_pad_34[0x4c - 0x34];
	uint8       m_LMWidth;                                  // +0x4c
	uint8       m_LMHeight;                                 // +0x4d
	uint8       m_pad_4e[0x50 - 0x4e];
	SPolyVertex* m_pTJuncVerts;                             // +0x50
	uint16      m_nVerts;                                   // +0x54
	uint16      m_nTJuncVerts;                              // +0x56
	SPolyVertex m_Verts[1];                                 // +0x58
};

// The renderer's console variable (TU12's, constructor 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32       m_nValue;
	float       m_fValue;
	float       m_fDefault;
	int32*      m_pnValue;
	float*      m_pfValue;
	const char* m_pName;
	void*       m_hParam;
	CD3DConVar* m_pNext;
};

class CRenderStateList
{
public:
	void        Restore();                                  // 0x10022a86
	void        Apply(CRenderStateList* pList, uint32 iStage); // 0x10022a97
};

// The fog colour (d3d_init.cpp's), red, green and blue.
struct FogColor
{
	uint8       m_Color[3];
};

// The scene's view (common_draw.cpp's g_ViewParams).
struct ViewParams
{
	uint8       m_pad_000[0x88];                            // +0x000
	float       m_NearZ;                                    // +0x088
	uint8       m_pad_08c[0x90 - 0x8c];                     // +0x08c
	float       m_FarZ2;                                    // +0x090 (the far clip)
	uint8       m_pad_094[0x15c - 0x94];                    // +0x094
	LTMatrix    m_mWorldToCamera;                           // +0x15c
	uint8       m_pad_19c[0x1dc - 0x19c];                   // +0x19c
	LTMatrix    m_mFullTransform;                           // +0x1dc
};

extern uint8            g_LightTableR[256];                 // 0x1005fa9c
extern uint8            g_LightTableG[256];                 // 0x1005fb9c
extern uint8            g_LightTableB[256];                 // 0x1005fc9c
extern uint8            g_VertexAlpha;                      // 0x1005d20c
extern void             (__fastcall *g_pfnCalcFog)(const LTVector* pPos, uint32* pSpecular);  // 0x1005e1c4 (d3d_draw.cpp)
extern FogColor         g_FogColor;                         // 0x1005dad8
extern int32            g_CV_FixTJunc;                      // 0x1005dbb4
extern int32            g_CV_Saturate;                      // 0x1005d384
extern CD3DConVar       g_CV_LMDynamic;                     // 0x1007c220
extern CD3DConVar       g_CV_LMDynamicScale;                // 0x1007c200
extern CD3DConVar       g_CV_LMDynamicSize;                 // 0x1007c640
extern ViewParams     g_ViewParams;                     // 0x1005b790
extern CD3DConVar       g_CV_UseD3DClip;                    // 0x100622b0 (d3d_init.cpp)

#include "d3d_clip.h"

extern CRenderStateList g_SavedStates;                      // 0x10069728
struct LMBucket;
extern LMBucket*        g_LightmapPolyList;                 // 0x1005fda0 (drawlightmaps.cpp)
extern uint32           g_nTLVertsUsed;                     // 0x1005e27c
extern uint32           g_nTLVertPool;                      // 0x1005fe00
extern TLVertex*        g_TLVertPool;                       // 0x1005e294
extern uint32           g_nDrawPrimCalls;                   // 0x1005c144
extern uint32           g_nSkyLightsApplied;                // 0x1005c164

void        dd_ConsolePrint(const char* pMsg, ...);         // 0x10013ade
uint32      d3d_GammaRGB(uint8 r, uint8 g, uint8 b);        // 0x10014340
LTBOOL      d3d_GrowTLVertPool(uint32 nWanted);             // 0x10014900
QueuedPoly* d3d_QueuePoly(void* pPoly, void* pList, uint32 nType); // 0x10007ddb
void        d3d_DrawLMPolyLights(RenderPoly* pPoly, TLVertex* pVerts, uint32 nVerts); // 0x10007e5d
void        d3d_LightLMPoly(RenderPoly* pPoly, ClipVert2* pVerts, uint32 nVerts);    // 0x100083ec
LTBOOL      d3d_ClipLMVerts(ClipVert2** ppVerts, int32* pnVerts, ViewParams* pContext, uint32 flags); // 0x100085f2

// Save a render state, set it, and put it back when the scope closes.
class StateSet
{
public:
	StateSet(D3DRENDERSTATETYPE state, uint32 val)
	{
		m_State = state;
		g_pD3DDevice->GetRenderState(state, (unsigned long*)&m_OldVal);
		g_pD3DDevice->SetRenderState(state, val);
	}

	~StateSet()
	{
		g_pD3DDevice->SetRenderState(m_State, m_OldVal);
	}

	D3DRENDERSTATETYPE  m_State;
	uint32              m_OldVal;
};

// 0x10007b41 -- draw a lightmapped polygon: its vertices lit through the
// light tables, the second texture coordinates (the lightmap page's) from
// the screen-scale terms, clipped and drawn with the fog colour, then its
// dynamic lights; and queued for the lightmap pass, which draws the
// polygon's own texture coordinates.
void d3d_DrawDeferredPiece(RenderPoly* pPoly)
{
	int32 nVerts, i;
	ClipVert2* pVerts;
	ClipVert2 verts[MAX_LMPOLY_VERTS];
	ClipVert2* pVert;
	SPolyVertex* pPolyVerts;
	LTVector* pVec;
	uint32 n;
	TLVertex *pOut, *pTL;
	QueuedPoly* pQueued;
	CRenderStateList* pStateList;

	if (g_CV_FixTJunc)
	{
		pPolyVerts = pPoly->m_pTJuncVerts;
		nVerts = pPoly->m_nTJuncVerts;
	}
	else
	{
		pPolyVerts = pPoly->m_Verts;
		nVerts = pPoly->m_nVerts;
	}

	if (nVerts > MAX_LMPOLY_VERTS)
	{
		dd_ConsolePrint("Error: vertex buffer overflow");
		return;
	}

	StateSet fogState(D3DRENDERSTATE_FOGCOLOR, d3d_GammaRGB(g_FogColor.m_Color[0], g_FogColor.m_Color[1], g_FogColor.m_Color[2]));

	pVert = verts;
	for (i = nVerts; i > 0; i--)
	{
		pVec = pPolyVerts->m_pVec;
		pVert->m_vPos.x = pVec->x;
		pVert->m_vPos.y = pVec->y;
		pVert->m_vPos.z = pVec->z;
		pVert->m_Diffuse.m_b[2] = g_LightTableR[pPolyVerts->m_Color[2]];
		pVert->m_Diffuse.m_b[1] = g_LightTableG[pPolyVerts->m_Color[1]];
		pVert->m_Diffuse.m_b[0] = g_LightTableB[pPolyVerts->m_Color[0]];
		pVert->m_Diffuse.m_b[3] = g_VertexAlpha;
		g_pfnCalcFog((const LTVector*)pVert, &pVert->m_Specular.m_n);
		pVert->SetUV((pVec->x + g_fScreenCenterX) * g_fScreenScaleX,
		             (pVec->z + g_fScreenCenterY) * g_fScreenScaleY);
		pVert->m_fU2 = pPolyVerts->m_U;
		pVert->m_fV2 = pPolyVerts->m_V;
		pPolyVerts++;
		pVert++;
	}

	pVerts = verts;
	if (!g_CV_LMDynamic.m_nValue)
		d3d_LightLMPoly(pPoly, pVerts, nVerts);

	if (!d3d_ClipLMVerts(&pVerts, &nVerts, &g_ViewParams, 0))
		return;

	if (g_nTLVertsUsed + nVerts > g_nTLVertPool)
	{
		if (!d3d_GrowTLVertPool(g_nTLVertPool + nVerts + 1500))
			return;
	}

	pOut = &g_TLVertPool[g_nTLVertsUsed];
	pVert = pVerts;
	pTL = pOut;
	for (i = nVerts; i; i--)
	{
		*pTL = *(TLVertex*)pVert;
		pTL++;
		pVert++;
	}

	g_SavedStates.Restore();
	pStateList = g_pRenderContext->m_pMainWorld->m_pStateList;
	if (pStateList)
		g_SavedStates.Apply(pStateList, g_iCurTextureStage);

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_TLVERTEX, pOut, nVerts, 0);
	g_SavedStates.Restore();

	if (pPoly->m_pLights)
		d3d_DrawLMPolyLights(pPoly, pOut, nVerts);

	pQueued = d3d_QueuePoly(pPoly, (QueuedPoly**)&g_LightmapPolyList, 0);
	pQueued->m_iFirstVert = g_nTLVertsUsed;
	pQueued->m_nVerts = nVerts;
	pQueued->m_ClipFlags = g_ClipFlags;
	pQueued->m_Type = 3;

	pVert = pVerts;
	pTL = pOut;
	for (n = nVerts; n; n--)
	{
		pTL->m_TU = pVert->m_fU2;
		pTL->m_TV = pVert->m_fV2;
		pTL++;
		pVert++;
	}

	g_nTLVertsUsed += nVerts;
	g_nDrawPrimCalls++;
}


// ----------------------------------------------------------------------- //
//  Polygon queues.
// ----------------------------------------------------------------------- //

// A texture's queue of polygons, one per kind, chained on a list.
struct TexQueue
{
	void*       m_pTexture;                                 // +0x00
	QueuedPoly* m_pPolys;                                   // +0x04
	TexQueue*   m_pNext;                                    // +0x08
};

struct QueueTexture
{
	uint8       m_pad_00[0x1c];                             // +0x00
	TexQueue*   m_Queues[4];                                // +0x1c
};

struct QueueSurface
{
	uint8       m_pad_00[0x2c];                             // +0x00
	QueueTexture* m_pTexture;                               // +0x2c
};

struct QueuePolyHdr
{
	uint8       m_pad_00[0x2c];                             // +0x00
	QueueSurface* m_pSurface;                               // +0x2c
};

extern StructBank       g_ClipPointBank;                    // 0x1005e730

// 0x10007ddb -- queue a polygon on its texture's queue of that kind,
// starting the queue (and chaining it on pList) on the texture's first.
QueuedPoly* d3d_QueuePoly(void* pPoly, void* pList, uint32 nType)
{
	QueueTexture* pTexture = ((QueuePolyHdr*)pPoly)->m_pSurface->m_pTexture;
	TexQueue** ppQueue = &pTexture->m_Queues[nType];
	TexQueue* pQueue = *ppQueue;
	QueuedPoly* pQueued;

	if (!pQueue)
	{
		pQueue = (TexQueue*)sb_Allocate(&g_ClipPointBank);
		pQueue->m_pTexture = pTexture;
		*ppQueue = pQueue;
		pQueue->m_pPolys = LTNULL;
		pQueue->m_pNext = *(TexQueue**)pList;
		*(TexQueue**)pList = pQueue;
	}

	pQueued = (QueuedPoly*)sb_Allocate(&g_DeferredPolyBank);
	pQueued->m_pNext = pQueue->m_pPolys;
	pQueue->m_pPolys = pQueued;
	pQueued->m_pPoly = pPoly;
	return pQueued;
}

// ----------------------------------------------------------------------- //
//  A lightmapped polygon's dynamic lights.
// ----------------------------------------------------------------------- //

// The profiling counters (TU14: 0x10013fb7, 0x10013fd5, 0x10013fe0).
#define CSTART_NONE         0

class CounterFinal
{
public:
	CounterFinal(unsigned long startMode=CSTART_NONE);
	unsigned long   m_Data[2];
};

typedef CounterFinal Counter;

void            cnt_StartCounterFinal(CounterFinal& cCounter);
unsigned long   cnt_EndCounterFinal(CounterFinal& cCounter);

inline void cnt_StartCounter(Counter& cCounter) { cnt_StartCounterFinal(cCounter); }
inline unsigned long cnt_EndCounter(Counter& cCounter) { return cnt_EndCounterFinal(cCounter); }

// 0x100083ae is its destructor's out-of-line copy.
class CountAdder
{
public:
	CountAdder(uint32* pNum)
	{
		m_pNum = pNum;
		cnt_StartCounter(m_Counter);
	}
	~CountAdder()
	{
		*m_pNum += cnt_EndCounter(m_Counter);
	}

	Counter     m_Counter;
	uint32*     m_pNum;
};

struct SceneDesc
{
	uint8       m_pad_00[0x18];
	uint32*     m_pTicks_Render_LMLights;                   // +0x18
};

extern SceneDesc*       g_pSceneDesc;                       // 0x1005c14c

// pixelformat.cpp's PFormat, as far as it is used here.
// pixelformat.cpp's (TU41)
enum BPPIdent
{
	BPP_8P=0,
	BPP_8,
	BPP_16,
	BPP_32,
	BPP_S3TC_DXT1,
	BPP_S3TC_DXT3,
	BPP_S3TC_DXT5,
	BPP_32P,
	NUM_BIT_TYPES
};

class PFormat
{
public:
	virtual void    Init(BPPIdent Type, uint32 aMask, uint32 rMask, uint32 gMask, uint32 bMask);

	uint32      m_eType;
	uint32      m_Masks[4];
	uint32      m_nBits[4];
	uint32      m_FirstBits[4];
};

struct WorldPoly;
struct LMPoly;
struct LMTextureList;

// A lightmap being drawn (lmsurfaces.cpp's).
struct LMDrawDesc
{
	LTBOOL      Begin(WorldPoly* pPoly, LTBOOL bCopy, uint32 width, uint32 height); // 0x100358b3
	LTBOOL      End(LTBOOL bChanged);                       // 0x10035a3f

	void*       m_pData;                                    // +0x00
	uint32      m_Pitch;                                    // +0x04
	uint32      m_iSize;                                    // +0x08
	PFormat     m_Format;                                   // +0x0c
	uint32      m_Width;                                    // +0x44
	uint32      m_Height;                                   // +0x48
	LMTextureList* m_pList;                                 // +0x4c
	void*       m_pScratch;                                 // +0x50
};

void    d3d_GetLMAxes(int iBasis, LTVector* pNormal, LTVector* pRight, LTVector* pUp); // 0x10035860
LTBOOL  d3d_DrawDynamicLM(LMDrawDesc* pDesc, LMPoly* pPoly, LMDynamicLight* pLight, float fScale); // 0x100333c0

// 0x10007e5d -- add a lightmapped polygon's dynamic lights: each light's
// falloff is drawn into a small lightmap texture, which is added over the
// polygon with texture coordinates from the polygon's lightmap axes.
void d3d_DrawLMPolyLights(RenderPoly* pPoly, TLVertex* pVerts, uint32 nVerts)
{
	LMDynamicLight* pLight;
	LTVector right, up, vPos, vLightPos, vDelta;
	LTMatrix mInvTransform;
	uint32 oldAddress;
	LTBOOL bChanged;
	float fScale;
	TLVertex* pVert;
	uint32 i;

	if (!g_CV_LMDynamic.m_nValue)
		return;

	CountAdder cntLights(g_pSceneDesc->m_pTicks_Render_LMLights);

	pPoly->m_LMWidth = (uint8)g_CV_LMDynamicSize.m_nValue;
	pPoly->m_LMHeight = (uint8)g_CV_LMDynamicSize.m_nValue;
	d3d_GetLMAxes((pPoly->m_PageFlags >> 11) & 7, &pPoly->m_pPlane->m_Normal, &right, &up);

	StateSet blendState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	StateSet srcState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
	StateSet destState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
	StateSet fogState(D3DRENDERSTATE_FOGCOLOR, 0);

	g_pD3DDevice->GetTextureStageState(0, D3DTSS_ADDRESS, (unsigned long*)&oldAddress);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);

	mInvTransform = g_ViewParams.m_mFullTransform.MakeInverse();

	for (pLight = pPoly->m_pLights; pLight; pLight = pLight->m_pNext)
	{
		LMDrawDesc desc;

		if (!desc.Begin((WorldPoly*)pPoly, TRUE, 0, 0))
			continue;

		bChanged = d3d_DrawDynamicLM(&desc, (LMPoly*)pPoly, pLight, 1.0f);
		if (!desc.End(bChanged) || !bChanged)
			continue;

		fScale = 1.0f / (pLight->m_pLight->GetRadius(pLight->m_pLight) * g_CV_LMDynamicScale.m_fValue);

		pVert = pVerts;
		for (i = nVerts; i; i--)
		{
			mInvTransform.Apply4x4(pVert->m_Vec, vPos);
			vLightPos = pLight->m_vPos;
			vDelta = vPos - vLightPos;

			pVert->m_TU = right.Dot(vDelta) * fScale + 0.5f;
			pVert->m_TV = vDelta.Dot(up) * fScale + 0.5f;
			pVert++;
		}

		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_TLVERTEX, pVerts, nVerts, 0);
	}

	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, oldAddress);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	d3d_SetTexture(g_pRenderContext->m_pMainWorld, g_iCurTextureStage, 0);
}

// 0x100083bf -- the lightmapped polygon draw (g_pfnDrawLightmapPoly): the
// polygon is kept, with the clip flags, for d3d_DrawDeferredPieces.
void d3d_DrawLightmapPoly(RenderPoly* pPoly)
{
	DeferredPiece* pDeferred;

	pDeferred = (DeferredPiece*)sb_Allocate(&g_DeferredPolyBank);
	if (pDeferred)
	{
		pDeferred->m_ClipFlags = g_ClipFlags;
		pDeferred->m_pNext = g_pDeferredPieces;
		g_pDeferredPieces = pDeferred;
		pDeferred->m_pPiece = pPoly;
	}
}

// 0x100083ec -- add the dynamic lights touching a lightmapped polygon to
// its vertices' colours (LMDynamic off).
//
// RETAIL BUG: it lights pPoly->m_nVerts vertices and ignores nVerts, the
// count the caller filled (the T-junction vertex count when FixTJunc is
// set), as d3d_skyobjects.cpp's d3d_LightSkyPoly does.
void d3d_LightLMPoly(RenderPoly* pPoly, ClipVert2* pVerts, uint32 nVerts)
{
	LMDynamicLight* pLink;
	LightObject* pLight;
	LTVector vLightPos, vDelta;
	RenderPlane* pPlane;
	float r, g, b, fDist, fRadiusSqr, fInvRadius, fDistSqr, fScale;
	int32 newR, newG, newB;
	uint32 i;
	ClipVert2* pVert;

	for (pLink = pPoly->m_pLights; pLink; pLink = pLink->m_pNext)
	{
		pLight = pLink->m_pLight;
		vLightPos = pLink->m_vPos;

		r = (float)pLight->m_ColorR;
		g = (float)pLight->m_ColorG;
		b = (float)pLight->m_ColorB;
		r -= 255.0f - r;
		g -= 255.0f - g;
		b -= 255.0f - b;

		g_nSkyLightsApplied++;
		pPlane = pPoly->m_pPlane;
		fDist = pPlane->DistTo(vLightPos);
		if (fDist < 0.0f)
			fDist = -fDist;

		if (fDist >= pLight->m_LightRadius)
			continue;

		fRadiusSqr = pLight->m_LightRadius * pLight->m_LightRadius;
		fInvRadius = 1.0f / pLight->m_LightRadius;

		pVert = pVerts;
		for (i = pPoly->m_nVerts; i > 0; i--)
		{
			VEC_SUB(vDelta, pVert->m_vPos, vLightPos);
			fDistSqr = vDelta.MagSqr();
			if (fDistSqr < fRadiusSqr)
			{
				fDist = (float)sqrt(fDistSqr);
				fScale = 1.0f - fDist * fInvRadius;

				newR = (int32)(fScale * r) + pVert->m_Diffuse.m_b[2];
				newG = (int32)(fScale * g) + pVert->m_Diffuse.m_b[1];
				newB = (int32)(fScale * b) + pVert->m_Diffuse.m_b[0];

				if (g_CV_Saturate)
				{
					newR += newR;
					newG += newG;
					newB += newB;
				}

				if (newR > 255)
					newR = 255;
				else if (newR < 0)
					newR = 0;

				if (newG > 255)
					newG = 255;
				else if (newG < 0)
					newG = 0;

				if (newB > 255)
					newB = 255;
				else if (newB < 0)
					newB = 0;

				pVert->m_Diffuse.m_b[2] = (uint8)newR;
				pVert->m_Diffuse.m_b[1] = (uint8)newG;
				pVert->m_Diffuse.m_b[0] = (uint8)newB;
			}

			pVert++;
		}
	}
}

// ----------------------------------------------------------------------- //
//  The lightmapped polygons' vertices: transformed and projected, or
//  taken to view space, clipped and then projected.
// ----------------------------------------------------------------------- //

struct ViewParams;

void d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams); // 0x1002739d

// 0x10008719 -- a vertex into view space (d3d_skyobjects.cpp calls this
// copy).
inline void d3d_TransformToCamera(TLVertex* pVert, LTMatrix* pMat)
{
	float x = pMat->m[0][0]*pVert->m_Vec.x + pMat->m[0][1]*pVert->m_Vec.y + pMat->m[0][2]*pVert->m_Vec.z + pMat->m[0][3];
	float y = pMat->m[1][0]*pVert->m_Vec.x + pMat->m[1][1]*pVert->m_Vec.y + pMat->m[1][2]*pVert->m_Vec.z + pMat->m[1][3];
	float z = pMat->m[2][0]*pVert->m_Vec.x + pMat->m[2][1]*pVert->m_Vec.y + pMat->m[2][2]*pVert->m_Vec.z + pMat->m[2][3];

	pVert->m_Vec.z = z;
	pVert->m_Vec.x = x;
	pVert->m_Vec.y = y;
}

// 0x100085f2 -- with nothing to clip against, the vertices go straight
// through the full transform; otherwise to view space, through the
// clipper and then projected.
LTBOOL d3d_ClipLMVerts(ClipVert2** ppVerts, int32* pnVerts, ViewParams* pContext, uint32 flags)
{
	ClipVert2* pVert;
	int32 nVerts;

	if (!g_ClipFlags)
	{
		pVert = *ppVerts;
		for (nVerts = *pnVerts; nVerts; nVerts--)
		{
			pVert->m_fRHW = MatVMul_InPlace_H(&pContext->m_mFullTransform, &pVert->m_vPos);
			pVert++;
		}
		return LTTRUE;
	}

	pVert = *ppVerts;
	for (nVerts = *pnVerts; nVerts; nVerts--)
	{
		d3d_TransformToCamera((TLVertex*)pVert, &pContext->m_mWorldToCamera);
		pVert++;
	}

	if (!ClipPoly(g_ClipFlags, ppVerts, pnVerts))
		return LTFALSE;

	pVert = *ppVerts;
	for (nVerts = *pnVerts; nVerts; nVerts--)
	{
		d3d_ProjectTLVert((TLVertex*)pVert, (ViewParams*)pContext);
		pVert++;
	}

	return LTTRUE;
}


// The out-of-line transform the /O2 files call (d3d_draw.cpp,
// drawmodelshadows.cpp, drawmodel_env.cpp): ltmatrix.h's
// MatVMul_InPlace_H, written once here, which the link folds with the
// inline's out-of-line copy (0x1000868d).
float d3d_MatVMul_InPlace_H(LTMatrix* pMat, LTVector* pSrc)
{
	float one_over_w = 1.0f / (pMat->m[3][0]*pSrc->x + pMat->m[3][1]*pSrc->y + pMat->m[3][2]*pSrc->z + pMat->m[3][3]);
	LTVector temp;

	temp.x = one_over_w * (pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z + pMat->m[0][3]);
	temp.y = one_over_w * (pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z + pMat->m[1][3]);
	temp.z = one_over_w * (pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z + pMat->m[2][3]);

	*pSrc = temp;
	return one_over_w;
}
