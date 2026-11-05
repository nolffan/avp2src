// *********************************************************************** //
//  ph/drawmodel_env.cpp -- Primal Hunt's d3d.ren, TU26 (0x10024ec0..0x10025e6c), /O1.
//
//  Model drawing: the ModelSpecular and ModelTexture console variables,
//  the environment-map stage, the OT_MODEL ProcessObject handler, the
//  solid/chromakey/translucent model passes and the blend states shared
//  with the particle systems (0x10025de5).
//
//  Not yet written here: 0x10024efe..0x10025269, 0x1002544b, 0x10025469,
//  the model pass 0x100254d8 and 0x10025acf.
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
extern void			d3d_DrawModel(SceneContext* pContext, LTObject* pObject);  // 0x1000b4d8


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
			pSet->Draw(&g_SceneContext, d3d_DrawModel);
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
			pSet->Draw(&g_SceneContext, d3d_DrawModel);
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

	d3d_DrawModel(&g_SceneContext, pObject);
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
