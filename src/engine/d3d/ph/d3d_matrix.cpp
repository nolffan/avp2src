// *********************************************************************** //
//
//  MODULE:   D3D_MATRIX (.CPP) -- Primal Hunt's d3d.ren, TU05
//            (0x10008c80..0x10009877, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The particle systems' draw (0x10008c90, the batch drawer
//            0x10009320 and the texture lookup between), and the object
//            transform set-up the other drawing modules call.  The name
//            is the map's guess: the object is mostly particles.
//
//            The particle draw (0x10008c90) binds the system's texture
//            through an inline copy of d3d_SetTexture, sets the texture
//            coordinate terms, and draws in batches of 128 through
//            0x10009320, which builds clipped screen-space quads and
//            draws them with drawparticles.cpp's shared index list.
//
//  NEAR:     0x10008c90 (706 against 832): retail keeps two unused locals
//            whose LTVector members it initialises with out-of-line
//            _CVector::Init calls (0x10009860), and calls
//            d3d_GetStageTexture out of line; here /Ob2 inlines both (the
//            dead locals then vanish). cl 8966's budget is depth-first in
//            source order, so retail's must have been spent elsewhere.
//            0x10009320 (1340 against 1337): the transform sums' term
//            order and the colour packing's scheduling.
//
// *********************************************************************** //

#include <d3d.h>
#include "ltbasedefs.h"


#define MAX_PARTICLE_BATCH  128

// The particle batch's vertex (FVF 0x1c4), and the batch drawer's view of
// a particle.
struct TLVertex
{
	float       m_X, m_Y, m_Z;                              // +0x00
	float       m_RHW;                                      // +0x0c
	uint32      m_Color;                                    // +0x10
	uint32      m_Specular;                                 // +0x14
	float       m_TU;                                       // +0x18
	float       m_TV;                                       // +0x1c
};

// A particle (the engine's PSParticle).
struct PSParticle
{
	uint8       m_Reserved00[0x0c];
	LTVector    m_Color;                                    // +0x0c
	float       m_Alpha;                                    // +0x18
	float       m_Size;                                     // +0x1c
	PSParticle* m_pNext;                                    // +0x20
	LTVector    m_Pos;                                      // +0x24
};

// A colour with its alpha byte set on its own.
union VColorA
{
	uint32      m_n;
	uint8       m_b[4];
};

// One particle's quad.
struct PSQuad
{
	TLVertex    m_Verts[4];
};

// A texture's per-stage device copy (d3d_texture.cpp's RTexture).
class RTexture
{
public:
	virtual void    Slot00();
	virtual void    Slot04();
	virtual void    Slot08();
	virtual uint32  GetWidth();                             // +0x0c
	virtual uint32  GetHeight();                            // +0x10

	float       m_fInvWidth;                                // +0x04
	float       m_fInvHeight;                               // +0x08
	LPDIRECTDRAWSURFACE7 m_pSurface;                        // +0x0c
	uint8       m_Reserved10[0x30 - 0x10];
	RTexture*   m_pNext;                                    // +0x30
	uint8       m_Reserved34[0x42 - 0x34];
	uint16      m_iStage;                                   // +0x42
	uint16      m_LOD;                                      // +0x44
};

class CRenderStateList
{
public:
	void        Restore();                                  // 0x10022a86
	void        Apply(CRenderStateList* pList, uint32 iStage); // 0x10022a97
};

struct SharedTexture
{
	uint8       m_Reserved00[0x0c];
	RTexture*   m_pRenderData;                              // +0x0c
	uint8       m_Reserved10[0x30 - 0x10];
	uint16      m_FrameCode;                                // +0x30
	uint8       m_Reserved32[0x34 - 0x32];
	CRenderStateList* m_pStateList;                         // +0x34
};

// The engine's rotation, as the transform reads it.
struct Rotation
{
	float   x;
	float   y;
	float   z;
	float   w;
};

// A particle system, as its draw reads it.

class LTObject
{
public:
	uint8       m_Reserved000[0x88];
	uint8       m_Flags;                                    // +0x088
	uint8       m_Reserved089[0x94 - 0x89];
	uint8       m_ColorR;                                   // +0x094
	uint8       m_ColorG;                                   // +0x095
	uint8       m_ColorB;                                   // +0x096
	uint8       m_ColorA;                                   // +0x097
	uint8       m_Reserved098[0x9c - 0x98];
	Rotation    m_Rotation;                                 // +0x09c
	LTVector    m_Scale;                                    // +0x0ac
	uint8       m_Reserved0b8[0x194 - 0xb8];
	LTVector    m_SystemCenter;                             // +0x194
	uint8       m_Reserved1a0[0x1d0 - 0x1a0];
	PSParticle* m_pFirstParticle;                           // +0x1d0
	uint8       m_Reserved1d4[0x1f0 - 0x1d4];
	SharedTexture* m_pCurTexture;                           // +0x1f0
	uint8       m_Reserved1f4[0x230 - 0x1f4];
	int32       m_nParticles;                               // +0x230
};

// The scene's view (common_draw.cpp's g_ViewParams).
struct ViewParams
{
	uint8       m_Reserved000[0x1c];
	int32       m_Left;                                     // +0x01c
	int32       m_Top;                                      // +0x020
	int32       m_Right;                                    // +0x024
	int32       m_Bottom;                                   // +0x028
	float       m_fScreenMinX;                              // +0x02c
	float       m_fScreenMaxX;                              // +0x030
	float       m_fScreenMinY;                              // +0x034
	float       m_fScreenMaxY;                              // +0x038
	uint8       m_Reserved03c[0x88 - 0x3c];
	float       m_fNearZ;                                   // +0x088
	float       m_fFarZ;                                    // +0x08c
	uint8       m_Reserved090[0x94 - 0x90];
	float       m_fParticleScale;                           // +0x094
	uint8       m_Reserved098[0x15c - 0x98];
	LTMatrix    m_mWorldToCamera;                           // +0x15c
	LTMatrix    m_mProjection;                              // +0x19c
};

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
	uint8       m_Reserved00[0x14];
	uint32*     m_pTicks_Render_ParticleSystems;            // +0x14
};

// A texture stage's texel size (lmsurfaces.cpp's TexPair).
struct TexPair
{
	float       u;
	float       v;
};

extern LPDIRECT3DDEVICE7 g_pD3DDevice;                      // 0x100638c8
extern SceneDesc*       g_pSceneDesc;                       // 0x1005c14c
extern uint32           g_iCurTextureStage;                 // 0x100638c0
extern uint16           g_CurTextureFrameCode;              // 0x1005d250
extern RTexture*        g_pCurTextures[];                   // 0x10067270
extern TexPair          g_TexelSize[];                      // 0x100672a8
extern CRenderStateList g_SavedStates;                      // 0x10069728
extern ViewParams     g_ViewParams;                     // 0x1005b790
extern ViewParams     g_ReallyCloseContext;               // 0x1005b910
extern void             (__fastcall *g_pfnSetupSpecular)(TLVertex* pVert, uint32* pSpecular);  // 0x1005e1c4
extern uint32           g_nParticlesDrawn;                  // 0x1005b770
extern uint16           g_ParticleIndices[];                // 0x10072c50

RTexture*   d3d_CreateRenderTexture(SharedTexture* pTexture, uint32 iStage, LTBOOL bStageCopy); // 0x10020cd0
void        d3d_SetTextureDirect(RTexture* pTexture);       // 0x10007a89
RTexture*   d3d_GetStageTexture(RTexture* pTexture, uint8 iStage); // 0x10009300
void        d3d_SetupTransformation(LTVector* pPos, Rotation* pRot, LTVector* pScale, LTMatrix* pMat); // 0x100091f0
PSParticle* d3d_DrawParticleBatch(LTObject* pSystem, PSParticle* pParticle, int32 nParticles,
                                  LTMatrix* pMat, LTBOOL bScreenSize, float fSizeScale); // 0x10009320


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //

// The particle colour scale (0x10053ff0): its initialiser is the empty
// constructor's, 0x10008c80.
LTVector    g_vParticleColorScale;                          // 0x10053ff0

// The particle texture's coordinate terms, set for each system's texture,
// and its fog.
float       g_fParticleUScale;                              // 0x10053ffc
float       g_fParticleUOffset;                             // 0x10054000
uint32      g_ParticleSpecular;                             // 0x10054004
float       g_fParticleVScale;                              // 0x10054008
PSQuad      g_ParticleQuads[MAX_PARTICLE_BATCH / 4 * 5];    // 0x1005400c (unused)
float       g_fParticleVOffset;                             // 0x1005540c


// ---------------------------------------------------------------------- //
//  The particle system draw.
// ---------------------------------------------------------------------- //

// d3d_texture.cpp's d3d_SetTexture, as a header inline: bind a shared
// texture's copy for a stage, made (and chained) if there is none yet.
inline LTBOOL d3d_BindTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD)
{
	RTexture *pRenderData, *pCur;

	pRenderData = pTexture->m_pRenderData;
	pTexture->m_FrameCode = g_CurTextureFrameCode;

	pCur = LTNULL;
	if (pRenderData)
		pCur = d3d_GetStageTexture(pRenderData, (uint8)iStage);

	if (pCur)
	{
		if (pCur != g_pCurTextures[iStage])
			d3d_SetTextureDirect(pCur);
	}
	else
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

		d3d_SetTextureDirect(pCur);
	}

	if (pCur->m_LOD != iLOD)
	{
		pCur->m_pSurface->SetLOD(iLOD);
		pCur->m_LOD = (uint16)iLOD;
	}

	return LTTRUE;
}

// Take the texture off a stage.
inline void d3d_UnbindTexture(uint32 iStage)
{
	if (g_pCurTextures[iStage])
	{
		g_pD3DDevice->SetTexture(iStage, LTNULL);
		g_pCurTextures[iStage] = LTNULL;
	}
}

// Two locals the draw sets up and never reads.
struct PSDrawUnusedA
{
	PSDrawUnusedA()
	{
		m_a = m_b = m_c = 0;
		m_d = m_e = m_f = m_g = 0;
		m_v1.Init();
		m_v2.Init();
	}

	uint32      m_a, m_b, m_c;
	LTVector    m_v1;
	LTVector    m_v2;
	uint32      m_d, m_e, m_f, m_g;
};

struct PSDrawUnusedB
{
	PSDrawUnusedB()
	{
		m_a = m_b = m_c = 0;
		m_v.Init();
		m_d = m_e = 0;
		m_f = -1;
		m_g = 0;
	}

	uint32      m_a, m_b, m_c;
	LTVector    m_v;
	uint32      m_d, m_e;
	int32       m_f;
	uint32      m_g;
};

// 0x10008c90 -- draw a particle system: its texture bound (and the
// texture coordinate terms set from it), its transform into view space,
// then its particles in batches of MAX_PARTICLE_BATCH.
void d3d_DrawParticles(LTObject* pSystem)
{
	LTMatrix mTransform, mFull;
	PSDrawUnusedA unusedA;
	PSDrawUnusedB unusedB;
	SharedTexture* pTexture;
	RTexture* pRTexture;
	PSParticle* pParticle;
	int32 nBatches, nLeft, i;

	CountAdder cntParticles(g_pSceneDesc->m_pTicks_Render_ParticleSystems);

	pTexture = pSystem->m_pCurTexture;
	if (pTexture && d3d_BindTexture(pTexture, g_iCurTextureStage, 0))
	{
		g_fParticleUScale = g_TexelSize[0].u * 2.0f;
		pRTexture = g_pCurTextures[g_iCurTextureStage];
		g_fParticleUOffset = ((float)pRTexture->GetWidth() - 2.0f) * g_TexelSize[0].u;
		g_fParticleVScale = g_TexelSize[0].v * 2.0f;
		g_fParticleVOffset = ((float)pRTexture->GetHeight() - 2.0f) * g_TexelSize[0].v;

		g_SavedStates.Restore();
		if (pTexture->m_pStateList)
			g_SavedStates.Apply(pTexture->m_pStateList, g_iCurTextureStage);
	}
	else
	{
		g_fParticleUScale = g_fParticleVScale = g_fParticleUOffset = g_fParticleVOffset = 0.0f;
		d3d_UnbindTexture(g_iCurTextureStage);
	}

	d3d_SetupTransformation(&pSystem->m_SystemCenter, &pSystem->m_Rotation, &pSystem->m_Scale, &mTransform);

	if (pSystem->m_Flags & FLAG_REALLYCLOSE)
		MatMul(&mFull, &g_ReallyCloseContext.m_mWorldToCamera, &mTransform);
	else
		MatMul(&mFull, &g_ViewParams.m_mWorldToCamera, &mTransform);

	g_pfnSetupSpecular((TLVertex*)&pSystem->m_SystemCenter, &g_ParticleSpecular);

	g_vParticleColorScale.x = 1.0f / 255.0f;
	g_vParticleColorScale.y = 1.0f / 255.0f;
	g_vParticleColorScale.z = 1.0f / 255.0f;

	nBatches = pSystem->m_nParticles / MAX_PARTICLE_BATCH;
	nLeft = pSystem->m_nParticles - nBatches * MAX_PARTICLE_BATCH;

	pParticle = pSystem->m_pFirstParticle;
	for (i=0; i < nBatches; i++)
		pParticle = d3d_DrawParticleBatch(pSystem, pParticle, MAX_PARTICLE_BATCH, &mFull, LTTRUE, 0.0f);

	if (nLeft)
		d3d_DrawParticleBatch(pSystem, pParticle, nLeft, &mFull, LTTRUE, 0.0f);
}



void d3d_RotationToMatrix(Rotation* pRot, LTMatrix* pMat);  // 0x1003655d


// 0x100091f0 -- position, rotation and scale to a transform: the rotation
// is sanitised first (a component out of range is reset).
void d3d_SetupTransformation(LTVector* pPos, Rotation* pRot, LTVector* pScale, LTMatrix* pMat)
{
	if (pRot->x > 100000.0f || pRot->x < -100000.0f)
		pRot->x = 0.0f;

	if (pRot->y > 100000.0f || pRot->y < -100000.0f)
		pRot->y = 0.0f;

	if (pRot->z > 100000.0f || pRot->z < -100000.0f)
		pRot->z = 0.0f;

	if (pRot->w > 100000.0f || pRot->w < -100000.0f)
		pRot->w = 1.0f;

	d3d_RotationToMatrix(pRot, pMat);

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


// 0x10009300 -- a texture's copy for a stage.
RTexture* d3d_GetStageTexture(RTexture* pTexture, uint8 iStage)
{
	while (pTexture)
	{
		if (pTexture->m_iStage == (uint16)iStage)
			return pTexture;

		pTexture = pTexture->m_pNext;
	}

	return LTNULL;
}


// Round to nearest in the FPU's mode (the colour bytes are `fstp t ; fld t
// ; fistp n`: LithTech's assembly helper).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// 0x10009320 -- draw up to MAX_PARTICLE_BATCH of a system's particles as
// screen-facing quads, clipped to the view: each particle into view space
// (pMat), tested against the near and far planes and the view cone,
// projected, sized and cut to the screen rectangle, its texture
// coordinates with it. Answers the particle after the batch.
PSParticle* d3d_DrawParticleBatch(LTObject* pSystem, PSParticle* pParticle, int32 nParticles,
                                  LTMatrix* pMat, LTBOOL bScreenSize, float fSizeScale)
{
	PSQuad quads[MAX_PARTICLE_BATCH];
	PSQuad* pQuad;
	LTVector vPos, vScreen;
	float r, g, b, a, fScreenScale, fSize, fRadius, rhw, fLimit;
	float x1, y1, x2, y2, u1, v1, u2, v2;
	float uStart, vStart, uEnd, vEnd;
	uint32 color, nVerts;
	VColorA alpha;
	int32 i;

	r = (float)pSystem->m_ColorR * g_vParticleColorScale.x;
	g = (float)pSystem->m_ColorG * g_vParticleColorScale.y;
	b = (float)pSystem->m_ColorB * g_vParticleColorScale.z;
	a = (float)pSystem->m_ColorA;

	uStart = g_fParticleUScale;
	vEnd = g_fParticleVOffset;
	vStart = g_fParticleVScale;
	uEnd = g_fParticleUOffset;

	alpha.m_n = 0;
	pQuad = quads;

	fScreenScale = (float)(g_ViewParams.m_Right - g_ViewParams.m_Left) * g_ViewParams.m_fParticleScale;
	fSize = fScreenScale * fSizeScale;

	if (bScreenSize != LTTRUE || !nParticles)
		return pParticle;

	for (i=0; i < nParticles; i++)
	{
		MatVMul_H(&vPos, pMat, &pParticle->m_Pos);

		fLimit = vPos.z * 1.2f;
		if (vPos.z > g_ViewParams.m_fNearZ && vPos.z < g_ViewParams.m_fFarZ &&
			vPos.x > -fLimit && vPos.x < fLimit && vPos.y < fLimit && vPos.y > -fLimit)
		{
			rhw = MatVMul_H(&vScreen, &g_ViewParams.m_mProjection, &vPos);

			if (fSizeScale == 0.0f)
				fSize = fScreenScale * pParticle->m_Size;

			fRadius = rhw * fSize;

			alpha.m_b[3] = (uint8)RoundFloatToInt(a * pParticle->m_Alpha);
			color = (((RoundFloatToInt(r * pParticle->m_Color.x) << 8) |
			          RoundFloatToInt(g * pParticle->m_Color.y)) << 8) |
			        RoundFloatToInt(b * pParticle->m_Color.z) | alpha.m_n;

			x1 = vScreen.x - fRadius;
			y1 = vScreen.y - fRadius;
			x2 = vScreen.x + fRadius;
			y2 = vScreen.y + fRadius;

			u1 = uStart;
			v1 = vStart;
			u2 = uEnd;
			v2 = vEnd;

			if (x1 < g_ViewParams.m_fScreenMinX)
			{
				u1 = (g_ViewParams.m_fScreenMinX - x1) / (x2 - x1) * (uEnd - uStart) + uStart;
				x1 = g_ViewParams.m_fScreenMinX;
			}

			if (y1 < g_ViewParams.m_fScreenMinY)
			{
				v1 = (g_ViewParams.m_fScreenMinY - y1) / (y2 - y1) * (vEnd - vStart) + vStart;
				y1 = g_ViewParams.m_fScreenMinY;
			}

			if (x2 > g_ViewParams.m_fScreenMaxX)
			{
				u2 = (g_ViewParams.m_fScreenMaxX - x1) / (x2 - x1) * (uEnd - u1) + u1;
				x2 = g_ViewParams.m_fScreenMaxX;
			}

			if (y2 > g_ViewParams.m_fScreenMaxY)
			{
				v2 = (g_ViewParams.m_fScreenMaxY - y1) / (y2 - y1) * (vEnd - v1) + v1;
				y2 = g_ViewParams.m_fScreenMaxY;
			}

			pQuad->m_Verts[0].m_X = x1;
			pQuad->m_Verts[0].m_Y = y1;
			pQuad->m_Verts[0].m_Z = vScreen.z;
			pQuad->m_Verts[0].m_RHW = rhw;
			pQuad->m_Verts[0].m_Color = color;
			pQuad->m_Verts[0].m_Specular = g_ParticleSpecular;
			pQuad->m_Verts[0].m_TU = u1;
			pQuad->m_Verts[0].m_TV = v1;

			pQuad->m_Verts[1].m_X = x2;
			pQuad->m_Verts[1].m_Y = y1;
			pQuad->m_Verts[1].m_Z = vScreen.z;
			pQuad->m_Verts[1].m_RHW = rhw;
			pQuad->m_Verts[1].m_Color = color;
			pQuad->m_Verts[1].m_Specular = g_ParticleSpecular;
			pQuad->m_Verts[1].m_TU = u2;
			pQuad->m_Verts[1].m_TV = v1;

			pQuad->m_Verts[2].m_X = x2;
			pQuad->m_Verts[2].m_Y = y2;
			pQuad->m_Verts[2].m_Z = vScreen.z;
			pQuad->m_Verts[2].m_RHW = rhw;
			pQuad->m_Verts[2].m_Color = color;
			pQuad->m_Verts[2].m_Specular = g_ParticleSpecular;
			pQuad->m_Verts[2].m_TU = u2;
			pQuad->m_Verts[2].m_TV = v2;

			pQuad->m_Verts[3].m_X = x1;
			pQuad->m_Verts[3].m_Y = y2;
			pQuad->m_Verts[3].m_Z = vScreen.z;
			pQuad->m_Verts[3].m_RHW = rhw;
			pQuad->m_Verts[3].m_Color = color;
			pQuad->m_Verts[3].m_Specular = g_ParticleSpecular;
			pQuad->m_Verts[3].m_TU = u1;
			pQuad->m_Verts[3].m_TV = v2;

			pQuad++;
		}

		pParticle = pParticle->m_pNext;
	}

	if (pQuad != quads)
	{
		nVerts = (TLVertex*)pQuad - (TLVertex*)quads;
		g_nParticlesDrawn += pQuad - quads;
		g_pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1,
			quads, nVerts, g_ParticleIndices, (pQuad - quads) * 6, 0);
	}

	return pParticle;
}
