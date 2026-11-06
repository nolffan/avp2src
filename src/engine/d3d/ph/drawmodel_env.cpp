// *********************************************************************** //
//  ph/drawmodel_env.cpp -- Primal Hunt's d3d.ren, TU26 (0x10024ec0..0x10025e6c), /O1.
//
//  Model drawing: the ModelSpecular and ModelTexture console variables,
//  the environment-map stage, the OT_MODEL ProcessObject handler, the
//  solid/chromakey/translucent model passes and the blend states shared
//  with the particle systems (0x10025de5).
//
//  Not yet written here: the model pass 0x100254d8.
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


CD3DConVar	g_CV_ModelSpecular("ModelSpecular", 1.0f, LTNULL, LTNULL);	// 0x1006d6a8  $E 0x10024ec0/0x10024ec5
CD3DConVar	g_CV_ModelTexture("ModelTexture", 1.0f, LTNULL, LTNULL);	// 0x1006d688  $E 0x10024edf/0x10024ee4


// IDirect3DDevice7, the slots used here.
typedef struct IDirect3DDevice7 CD3DDevice;	// g_pD3DDevice's real type, for its link name
struct IDirect3DDevice7
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
	virtual long __stdcall GetRenderState(uint32 state, uint32* pValue) = 0;
	virtual long __stdcall BeginStateBlock() = 0;
	virtual long __stdcall EndStateBlock(uint32*) = 0;
	virtual long __stdcall PreLoad(void*) = 0;
	virtual long __stdcall DrawPrimitive(uint32, uint32, void*, uint32, uint32) = 0;
	virtual long __stdcall DrawIndexedPrimitive(uint32, uint32, void*, uint32, uint16*, uint32, uint32) = 0;
	virtual long __stdcall SetClipStatus(void*) = 0;
	virtual long __stdcall GetClipStatus(void*) = 0;
	virtual long __stdcall DrawPrimitiveStrided(uint32, uint32, void*, uint32, uint32) = 0;
	virtual long __stdcall DrawIndexedPrimitiveStrided(uint32, uint32, void*, uint32, uint16*, uint32, uint32) = 0;
	virtual long __stdcall DrawPrimitiveVB(void*, uint32, uint32, uint32) = 0;
	virtual long __stdcall DrawIndexedPrimitiveVB(void*, uint32, uint32, uint16*, uint32, uint32) = 0;
	virtual long __stdcall ComputeSphereVisibility(void*, float*, uint32, uint32, uint32*) = 0;
	virtual long __stdcall GetTexture(uint32, void**) = 0;
	virtual long __stdcall SetTexture(uint32, void*) = 0;
	virtual long __stdcall GetTextureStageState(uint32 stage, uint32 type, uint32* pValue) = 0;
	virtual long __stdcall SetTextureStageState(uint32 stage, uint32 type, uint32 value) = 0;
};

extern CD3DDevice*	g_pD3DDevice;                       // 0x100638c8

#define D3DRS_FILLMODE		8
#define D3DRS_SRCBLEND		19
#define D3DRS_DESTBLEND		20
#define D3DRS_FOGENABLE		28
#define D3DRS_FOGCOLOR		34
#define D3DTSS_COLOROP		1
#define D3DTSS_COLORARG1	2
#define D3DTSS_COLORARG2	3
#define D3DTSS_ADDRESS		12
#define D3DTOP_DISABLE		1
#define D3DTOP_ADD			7
#define D3DTA_CURRENT		1
#define D3DTA_TEXTURE		2
#define D3DTADDRESS_CLAMP	3
#define D3DBLEND_ZERO		1
#define D3DBLEND_ONE		2
#define D3DBLEND_SRCCOLOR	3
#define D3DBLEND_SRCALPHA	5
#define D3DBLEND_INVSRCALPHA	6

// Sets a render state for its lifetime, restoring the old value after.
class StateSet
{
public:
	StateSet(uint32 state, uint32 val)
	{
		m_State = state;
		g_pD3DDevice->GetRenderState(state, &m_OldVal);
		g_pD3DDevice->SetRenderState(state, val);
	}

	~StateSet()
	{
		g_pD3DDevice->SetRenderState(m_State, m_OldVal);
	}

	uint32	m_State;
	uint32	m_OldVal;
};


struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0x97 - 0x90];
	uint8		m_ColorA;                               // +0x097
	uint8		m_pad_098[0xbc - 0x98];
	uint8		m_ObjectType;                           // +0x0bc
};

struct SceneContext;
typedef void (*DrawObjectFn)(SceneContext* pContext, LTObject* pObject);

class BaseObjectSet
{
public:
	void		Add(LTObject *pObject);                 // 0x10023797
	void		Draw(SceneContext* pContext, DrawObjectFn fn);  // 0x100397e0

	LTObject	**m_pObjects;                           // +0x00
	uint32		m_nObjects;                             // +0x04
	uint8		m_pad_08[0x20 - 0x08];
};

struct VisibleSet
{
	uint8			m_pad_000[0x144];
	BaseObjectSet	m_SolidModels;                      // +0x144
	BaseObjectSet	m_TranslucentModels;                // +0x164
	BaseObjectSet	m_ChromakeyModels;                  // +0x184
};

class ObjectDrawList
{
public:
	void	Add(LTObject* pObject, DrawObjectFn fn);    // 0x10029980
};

extern ObjectDrawList*	g_pObjectDrawList;              // 0x100713cc
extern SceneContext		g_SceneContext;                 // 0x1005b790
extern int32			g_CV_DrawModels;                // 0x1004a788
extern uint32			g_FillMode;                     // 0x1006dac8
extern uint32			g_EnvMapColorOp;                // 0x1006d680

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0


// ---------------------------------------------------------------------- //
//  The model drawer (TU07's CD3DModelDraw): what these members read.
// ---------------------------------------------------------------------- //
#define D3DRS_ZENABLE		7
#define D3DRS_ZWRITEENABLE	14
#define D3DRS_ALPHABLENDENABLE	27
#define D3DFILL_WIREFRAME	2
#define D3DPT_LINELIST		2
#define FVF_TLVERTEX		0x1c4
#define CLIPFLAGS_ALL		0x3f

struct TLVertex
{
	LTVector	m_Vec;                                  // +0x00
	float		rhw;                                    // +0x0c
	uint32		color;                                  // +0x10
	uint32		specular;                               // +0x14
	float		tu, tv;                                 // +0x18
};

struct ViewParams
{
	uint8		m_pad_000[0x15c];
	LTMatrix	m_mWorldToCamera;                      // +0x15c
};

struct SharedTexture
{
	uint8			m_pad_00[0x28];
	SharedTexture*	m_pEnvMap;                          // +0x28
};

struct ModelAnim
{
	uint8		m_pad_00[0x08];
	LTVector	m_vDims;                                // +0x08
	uint8		m_pad_14[0x20 - 0x14];
};

struct Model
{
	uint8		m_pad_000[0x168];
	ModelAnim*	m_Anims;                                // +0x168
};

struct ModelInstance
{
	uint8			m_pad_000[0x194];
	LTVector		m_Pos;                              // +0x194
	uint8			m_pad_1a0[0x1c0 - 0x1a0];
	SharedTexture*	m_pSkins[4];                        // +0x1c0
	uint8			m_pad_1d0[0x1fc - 0x1d0];
	Model*			m_pModel;                           // +0x1fc
	uint8			m_pad_200[0x20c - 0x200];
	uint16			m_iCurAnim;                         // +0x20c
};

extern void d3d_DrawModel(void* pParams, ModelInstance* pInstance);  // 0x1000b4d8 (drawmodel.cpp)

struct ModelVert
{
	uint8		m_pad_00[0x14];
	LTVector	m_vNormal;                              // +0x14
};

struct UVVert
{
	uint8		m_pad_00[0x18];
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c

	void SetUV(float fU, float fV) { m_fU = fU; m_fV = fV; }
};

class RTexture
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual LTBOOL	IsTranslucent();                    // +0x08
};

class CD3DModelDraw
{
public:
	void		DrawModelBox();                         // 0x10024efe
	void		SetupModelStates(LTBOOL* pTranslucent); // 0x10025193
	void		SetupSkin(uint32 iSkin);                // 0x10025269
	void		DrawPieces(uint32 a, uint32 b, LTBOOL bLOD);  // 0x10025acf

	void		DrawLOD();                              // 0x10005700
	void		DrawPiece(struct ModelVertSet* pSet);   // 0x100045a0 (d3d_modelvb.cpp)

	uint8			m_pad_000[0x4];
	ModelInstance*	m_pInstance;                        // +0x004
	uint8			m_pad_008[0x30 - 0x8];
	uint32			m_bEnvMap;                          // +0x030
	uint32			m_iSkinStage;                       // +0x034
	uint32			m_iDetailStage;                     // +0x038
	uint8			m_pad_03c[0x4c4 - 0x3c];
	uint32			m_bSetupSkins;                      // +0x4c4
	uint32			m_bTextured;                        // +0x4c8
	uint32			m_iCurSkin;                         // +0x4cc
	uint8			m_pad_4d0[0x590 - 0x4d0];
	LTMatrix		m_mEnvMap;                          // +0x590
	uint8			m_pad_5d0[0x5e8 - 0x5d0];
	uint32			m_nEnvMapPasses;                    // +0x5e8
	uint32			m_5ec;                              // +0x5ec
	uint8			m_pad_5f0[0x5f4 - 0x5f0];
	uint32			m_5f4;                              // +0x5f4
	uint8			m_pad_5f8[0x61c - 0x5f8];
	float			m_fEnvOffsetU;                      // +0x61c
	float			m_fEnvOffsetV;                      // +0x620
	float			m_fEnvScaleU;                       // +0x624
	float			m_fEnvScaleV;                       // +0x628
	uint8			m_pad_62c[0x894 - 0x62c];
	uint32			m_ModelFlags;                       // +0x894
	uint32			m_ModelFlags2;                      // +0x898
	uint8			m_pad_89c[0x8a4 - 0x89c];
	uint32			m_TextureFlags;                     // +0x8a4
	uint8			m_pad_8a8[0x8b4 - 0x8a8];
};

extern ViewParams	g_ViewParams;                       // 0x1005b790
extern uint32		g_BindTextureStage;                 // 0x100638c0
extern RTexture*	g_pCurTextures[];                   // 0x10067270
extern float		g_fTexelSizeU;                      // 0x100672a8
extern float		g_fTexelSizeV;                      // 0x100672ac
extern float		g_fTexelOffsetU;                    // 0x10052b70
extern float		g_fTexelOffsetV;                    // 0x10052b74
extern float		g_fTexelOffsetScale;                // 0x100638b4
extern float		g_fEnvMapOffsetU;                   // 0x100588c4
extern float		g_fEnvMapOffsetV;                   // 0x100588c8
extern int32		g_CV_TextureModels;                 // 0x1004a760
extern int32		g_CV_ModelFullbrite;                // 0x1004a774
extern int32		g_CV_ShowFullbriteModels;           // 0x1005df40
extern uint32		g_bFullbriteCapable;                // 0x100622a0

extern void		d3d_DisableTexture(uint32 iStage);      // 0x1000a22b
extern int		d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 flags);  // 0x100079e4
extern float	d3d_MatVMul_InPlace_H(LTMatrix* pMat, LTVector* pSrc);              // 0x1000868d
extern LTBOOL	d3d_ClipLine(TLVertex* pVerts, uint32 clipFlags);                   // 0x10016c60
extern void		d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams);            // 0x1002739d

void d3d_ResetFillMode();


// ---------------------------------------------------------------------- //
//  0x10024efe -- ModelBoxes: the model's animation box, in lines.
// ---------------------------------------------------------------------- //
static void d3d_DrawLine3D(LTVector* pSrc, LTVector* pDest, uint32 color);

void CD3DModelDraw::DrawModelBox()
{
	ModelInstance* pInstance;
	LTVector* pPos;
	LTVector* pDims;
	LTVector box[8];
	int i;

	d3d_DisableTexture(g_BindTextureStage);

	pInstance = m_pInstance;
	pPos = &pInstance->m_Pos;
	pDims = &pInstance->m_pModel->m_Anims[pInstance->m_iCurAnim].m_vDims;

	box[0].Init(pPos->x - pDims->x, pPos->y + pDims->y, pPos->z - pDims->z);
	box[1].Init(pPos->x + pDims->x, pPos->y + pDims->y, pPos->z - pDims->z);
	box[2].Init(pPos->x + pDims->x, pPos->y + pDims->y, pPos->z + pDims->z);
	box[3].Init(pPos->x - pDims->x, pPos->y + pDims->y, pPos->z + pDims->z);
	box[4].Init(pPos->x - pDims->x, pPos->y - pDims->y, pPos->z - pDims->z);
	box[5].Init(pPos->x + pDims->x, pPos->y - pDims->y, pPos->z - pDims->z);
	box[6].Init(pPos->x + pDims->x, pPos->y - pDims->y, pPos->z + pDims->z);
	box[7].Init(pPos->x - pDims->x, pPos->y - pDims->y, pPos->z + pDims->z);

	StateSet ssZWrite(D3DRS_ZWRITEENABLE, LTFALSE);
	StateSet ssZEnable(D3DRS_ZENABLE, LTFALSE);

	for (i=0; i < 4; i++)
	{
		d3d_DrawLine3D(&box[i+4], &box[((i+1) % 4) + 4], 0xFFFFFFFF);
		d3d_DrawLine3D(&box[i], &box[(i+1) % 4], 0xFFFFFFFF);
		d3d_DrawLine3D(&box[i], &box[i+4], 0xFFFFFFFF);
	}
}


// ---------------------------------------------------------------------- //
//  0x10025107 -- one world-space line, transformed, clipped and drawn.
// ---------------------------------------------------------------------- //
static void d3d_DrawLine3D(LTVector* pSrc, LTVector* pDest, uint32 color)
{
	TLVertex verts[2];

	verts[0].color = verts[1].color = color;
	verts[0].m_Vec = *pSrc;
	verts[1].m_Vec = *pDest;

	d3d_MatVMul_InPlace_H(&g_ViewParams.m_mWorldToCamera, &verts[0].m_Vec);
	d3d_MatVMul_InPlace_H(&g_ViewParams.m_mWorldToCamera, &verts[1].m_Vec);

	if (d3d_ClipLine(verts, CLIPFLAGS_ALL))
	{
		d3d_ProjectTLVert(&verts[0], &g_ViewParams);
		d3d_ProjectTLVert(&verts[1], &g_ViewParams);
		g_pD3DDevice->DrawPrimitive(D3DPT_LINELIST, FVF_TLVERTEX, verts, 2, 0);
	}
}


// ---------------------------------------------------------------------- //
//  0x10025193 -- the model's texturing and fill states; whether its
//  texture is drawn translucent.
// ---------------------------------------------------------------------- //
void CD3DModelDraw::SetupModelStates(LTBOOL* pTranslucent)
{
	uint32 alphaBlend;
	LTBOOL bTranslucent;

	g_fTexelOffsetV = 0.0f;
	g_fTexelOffsetU = 0.0f;

	m_bTextured = LTFALSE;
	if (g_CV_TextureModels && (m_ModelFlags & 1))
		m_bTextured = LTTRUE;
	else
		d3d_DisableTexture(g_BindTextureStage);

	g_pD3DDevice->GetRenderState(D3DRS_FILLMODE, &g_FillMode);
	if (m_ModelFlags2 & 0x10)
		g_pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	g_pD3DDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);

	bTranslucent = LTFALSE;
	if (g_bFullbriteCapable && g_CV_ModelFullbrite && g_pCurTextures[g_BindTextureStage] &&
		g_pCurTextures[g_BindTextureStage]->IsTranslucent() && !alphaBlend)
	{
		bTranslucent = LTTRUE;
		if (g_CV_ShowFullbriteModels)
		{
			bTranslucent = LTFALSE;
			d3d_DisableTexture(g_BindTextureStage);
		}
	}

	*pTranslucent = bTranslucent;
}


// ---------------------------------------------------------------------- //
//  0x10025269 -- bind a skin (and its environment map) for the pieces.
// ---------------------------------------------------------------------- //
void CD3DModelDraw::SetupSkin(uint32 iSkin)
{
	SharedTexture* pSkin;
	float fScale;

	if (!m_bSetupSkins)
		return;

	if (m_bTextured && iSkin < 4 && (pSkin = m_pInstance->m_pSkins[iSkin]) != LTNULL)
	{
		if (g_CV_ModelTexture.m_nValue && d3d_SetTexture(pSkin, m_iSkinStage, m_TextureFlags))
		{
			fScale = g_fTexelOffsetScale;
			g_fTexelOffsetU = g_fTexelSizeU * fScale;
			g_fTexelOffsetV = g_fTexelSizeV * fScale;
		}
		else
		{
			d3d_DisableTexture(g_BindTextureStage);
		}

		if (m_iDetailStage != (uint32)-1)
			d3d_SetTexture(pSkin, m_iDetailStage, m_TextureFlags);

		if (m_bEnvMap && pSkin->m_pEnvMap)
		{
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, g_EnvMapColorOp);
			d3d_SetTexture(pSkin->m_pEnvMap, 1, 0);
		}
		else if (m_nEnvMapPasses < 2)
		{
			g_pD3DDevice->GetTextureStageState(1, D3DTSS_COLOROP, &g_EnvMapColorOp);
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
			d3d_DisableTexture(1);
		}

		m_iCurSkin = iSkin;
	}
	else
	{
		d3d_DisableTexture(g_BindTextureStage);
	}
}


// ---------------------------------------------------------------------- //
//  0x10025397 / 0x1002541b -- stage 1 set for the environment map
//  (added, clamped), and put back.
// ---------------------------------------------------------------------- //
void d3d_SetEnvMapStage(uint32* pSaved)
{
	g_pD3DDevice->GetTextureStageState(1, D3DTSS_COLOROP, &pSaved[0]);
	g_pD3DDevice->GetTextureStageState(1, D3DTSS_ADDRESS, &pSaved[1]);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_ADD);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
	g_EnvMapColorOp = D3DTOP_ADD;
}

void d3d_UnsetEnvMapStage(uint32* pSaved)
{
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, pSaved[0]);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_ADDRESS, pSaved[1]);
}


// ---------------------------------------------------------------------- //
//  0x1002544b / 0x10025469 -- texture coordinate generators (the
//  drawer's per-vertex function pointers, as TU01's 0x10001370..).
// ---------------------------------------------------------------------- //
void __fastcall d3d_GenUVQuarter(UVVert* pOut, UVVert* pSrc, float* pUV)
{
	pOut->SetUV(pUV[0] * 0.25f, pUV[1] * 0.25f);
}

// NEAR: the terms of each row's sum come out in another order (retail
// v: y, x, z; u: y, z, x), as TU01's 0x10001420.
void __fastcall d3d_GenUVEnvMapOffset(CD3DModelDraw* pDraw, ModelVert* pVert, UVVert* pOut)
{
	pOut->SetUV(
		(pVert->m_vNormal.x * pDraw->m_mEnvMap.m[0][0]
		 + pVert->m_vNormal.y * pDraw->m_mEnvMap.m[0][1]
		 + pVert->m_vNormal.z * pDraw->m_mEnvMap.m[0][2])
			* pDraw->m_fEnvScaleU + pDraw->m_fEnvOffsetU + g_fEnvMapOffsetU,
		(pVert->m_vNormal.x * pDraw->m_mEnvMap.m[1][0]
		 + pVert->m_vNormal.y * pDraw->m_mEnvMap.m[1][1]
		 + pVert->m_vNormal.z * pDraw->m_mEnvMap.m[1][2])
			* pDraw->m_fEnvScaleV + pDraw->m_fEnvOffsetV + g_fEnvMapOffsetV);
}


// ---------------------------------------------------------------------- //
//  0x10025acf -- set up the states and skins, then draw the pieces.
// ---------------------------------------------------------------------- //
void CD3DModelDraw::DrawPieces(uint32 a, uint32 b, LTBOOL bLOD)
{
	LTBOOL bTranslucent;

	m_5f4 = a;
	m_5ec = b;
	m_bSetupSkins = LTTRUE;
	SetupModelStates(&bTranslucent);

	if (bLOD)
		DrawLOD();

	DrawPiece((ModelVertSet*)bTranslucent);
	d3d_ResetFillMode();
}


// ---------------------------------------------------------------------- //
//  0x10025b1b
// ---------------------------------------------------------------------- //
void d3d_ResetFillMode()
{
	g_pD3DDevice->SetRenderState(D3DRS_FILLMODE, g_FillMode);
}


// ---------------------------------------------------------------------- //
//  0x10025b2f -- the OT_MODEL ProcessObject handler.
// ---------------------------------------------------------------------- //
void d3d_ProcessModel(LTObject *pObject)
{
	VisibleSet* pSet = d3d_GetVisibleSet();

	if (pObject->m_ColorA != 0xFF)
		pSet->m_TranslucentModels.Add(pObject);
	else if (pObject->m_Flags2 & 2)
		pSet->m_ChromakeyModels.Add(pObject);
	else
		pSet->m_SolidModels.Add(pObject);
}


// ---------------------------------------------------------------------- //
//  0x10025b66
// ---------------------------------------------------------------------- //
void d3d_DrawSolidModels()
{
	if (g_CV_DrawModels)
	{
		BaseObjectSet* pSet = &d3d_GetVisibleSet()->m_SolidModels;
		if (pSet->m_nObjects)
		{
			g_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
			pSet->Draw(&g_SceneContext, (DrawObjectFn)d3d_DrawModel);
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		}
	}
}


// ---------------------------------------------------------------------- //
//  0x10025c04
// ---------------------------------------------------------------------- //
void d3d_DrawChromakeyModels()
{
	if (g_CV_DrawModels)
	{
		BaseObjectSet* pSet = &d3d_GetVisibleSet()->m_ChromakeyModels;
		if (pSet->m_nObjects)
			pSet->Draw(&g_SceneContext, (DrawObjectFn)d3d_DrawModel);
	}
}


static void d3d_DrawTranslucentModel(SceneContext* pContext, LTObject* pObject);

// ---------------------------------------------------------------------- //
//  0x10025c2f
// ---------------------------------------------------------------------- //
void d3d_QueueTranslucentModels()
{
	uint32 i;

	if (g_CV_DrawModels)
	{
		BaseObjectSet* pSet = &d3d_GetVisibleSet()->m_TranslucentModels;
		if (pSet->m_nObjects)
		{
			for (i=0; i < pSet->m_nObjects; i++)
				g_pObjectDrawList->Add(pSet->m_pObjects[i], d3d_DrawTranslucentModel);
		}
	}
}


void d3d_GetBlendStates(LTObject* pObject, uint32* pSrcBlend, uint32* pDestBlend,
	uint32* pFogEnable, uint32* pFogColor);

// ---------------------------------------------------------------------- //
//  0x10025c72
// ---------------------------------------------------------------------- //
static void d3d_DrawTranslucentModel(SceneContext* pContext, LTObject* pObject)
{
	uint32 srcBlend, destBlend, fogEnable, fogColor;

	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

	d3d_GetBlendStates(pObject, &srcBlend, &destBlend, &fogEnable, &fogColor);
	if (pObject->m_Flags2 & 0x4000)
	{
		srcBlend = D3DBLEND_SRCALPHA;
		destBlend = D3DBLEND_ONE;
	}

	StateSet ssSrcBlend(D3DRS_SRCBLEND, srcBlend);
	StateSet ssDestBlend(D3DRS_DESTBLEND, destBlend);
	StateSet ssFogEnable(D3DRS_FOGENABLE, fogEnable);
	StateSet ssFogColor(D3DRS_FOGCOLOR, fogColor);

	d3d_DrawModel(&g_SceneContext, (ModelInstance*)pObject);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
}


// ---------------------------------------------------------------------- //
//  0x10025de5 -- the blend and fog states for a translucent object.
// ---------------------------------------------------------------------- //
void d3d_GetBlendStates(LTObject* pObject, uint32* pSrcBlend, uint32* pDestBlend,
	uint32* pFogEnable, uint32* pFogColor)
{
	g_pD3DDevice->GetRenderState(D3DRS_FOGCOLOR, pFogColor);
	g_pD3DDevice->GetRenderState(D3DRS_FOGENABLE, pFogEnable);

	if ((pObject->m_Flags & FLAG_FOGDISABLE) && pObject->m_ObjectType != OT_MODEL)
		*pFogEnable = 0;

	if (pObject->m_Flags2 & 4)
	{
		*pDestBlend = *pSrcBlend = D3DBLEND_ONE;
		*pFogColor = 0;
	}
	else if (pObject->m_Flags2 & 8)
	{
		*pSrcBlend = D3DBLEND_ZERO;
		*pDestBlend = D3DBLEND_SRCCOLOR;
		*pFogColor = 0xFFFFFFFF;
	}
	else
	{
		*pSrcBlend = D3DBLEND_SRCALPHA;
		*pDestBlend = D3DBLEND_INVSRCALPHA;
	}
}
