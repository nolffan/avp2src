// *********************************************************************** //
//  ph/draw_canvas.cpp -- Primal Hunt's d3d.ren, TU23
//  (0x10023104..0x100238a6), /O1 /Ob2.
//
//  Canvases: the custom-draw manager handed to a canvas's draw function
//  (ILTCustomDraw, vtable 0x100483d0), which saves the device's states
//  around the call and translates the LT render states; the DrawCanvases
//  console variable; the OT_CANVAS entry of the object handler table
//  (0x1004db5c/0x1004db60) and the solid/translucent canvas passes.
//
//  /Ob2: the global's initialiser is one function (0x10023104, the
//  constructor's call folded into the registered thunk), and the two
//  recursive translators are expanded one level into SetState.
// *********************************************************************** //

#include <string.h>
#include "ltbasedefs.h"
#include "ph_decls.h"
#include "iltcustomdraw.h"


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
	virtual long __stdcall DrawPrimitive(uint32 type, uint32 fvf, void* pVerts,
	                                     uint32 nVerts, uint32 flags) = 0;
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

#define D3DRS_ZENABLE				7
#define D3DRS_ZWRITEENABLE			14
#define D3DRS_SRCBLEND				19
#define D3DRS_DESTBLEND				20
#define D3DRS_ALPHABLENDENABLE		27
#define D3DRS_FOGENABLE				28

#define D3DTSS_COLOROP				1
#define D3DTSS_ALPHAOP				4
#define D3DTSS_ADDRESS				12

#define D3DTOP_SELECTARG1			2
#define D3DTOP_SELECTARG2			3
#define D3DTOP_MODULATE				4
#define D3DTOP_ADD					7
#define D3DTOP_ADDSIGNED			8

#define D3DBLEND_ZERO				1
#define D3DBLEND_ONE				2
#define D3DBLEND_SRCCOLOR			3
#define D3DBLEND_INVSRCCOLOR		4
#define D3DBLEND_SRCALPHA			5
#define D3DBLEND_INVSRCALPHA		6

#define D3DTADDRESS_WRAP			1
#define D3DTADDRESS_CLAMP			3

#define D3DPT_TRIANGLEFAN			6
#define CANVASVERTEX_FORMAT			0x1c4	// XYZRHW | DIFFUSE | SPECULAR | TEX1

#define CF_SOLIDCANVAS				0x10


struct SharedTexture;
struct RenderTexture;
struct ViewParams;

struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0x188 - 0x90];
	uint8		m_ClientFlags;                          // +0x188
	uint8		m_pad_189[0x1b0 - 0x189];
	void		(*m_Fn)(ILTCustomDraw* pDraw, LTObject* hObj, void* pUser); // +0x1b0
	void*		m_pFnUserData;                          // +0x1b4
};

typedef LTObject Canvas;

struct ViewParams
{
	uint8		m_pad_000[0x88];
	float		m_NearZ;                                // +0x088
	uint8		m_pad_08c[0x2dc - 0x8c];
	LTMatrix	m_mReallyCloseToCamera;                 // +0x2dc
	uint8		m_pad_31c[0x4d4 - 0x31c];
	uint32		m_bDrawHidden;                          // +0x4d4
};

struct SharedTexture
{
	uint8			m_pad_00[0x34];
	RenderTexture*	m_pRenderData;                      // +0x34
};

struct RenderStruct
{
	void*			m_pad_00;
	SharedTexture*	(*GetSharedTexture)(const char* pFilename);  // +0x04
};

extern RenderStruct*	g_pStruct;                      // 0x1005df08

extern void		AddDebugMessage(int32 level, const char* pMsg, ...);  // 0x10013b13

typedef void (*DrawObjectFn)(ViewParams* pContext, LTObject* pObject);

class BaseObjectSet
{
public:
	void		Add(LTObject *pObject)
	{
		if(m_nObjects < m_nMaxObjects)
		{
			m_pObjects[m_nObjects] = pObject;
			m_nObjects++;
		}
		else
		{
			AddDebugMessage(1, "Set '%s' overflowed", m_pSetName);
		}
	}

	void		Draw(ViewParams* pContext, DrawObjectFn fn);  // 0x100397e0

	template<class T>
	void		DrawFiltered(ViewParams* pContext, DrawObjectFn fn, T* pTester, BaseObjectSet* pRejects)
	{
		uint32 i;
		LTObject* pObject;

		pRejects->m_nObjects = 0;
		for (i=0; i < m_nObjects; i++)
		{
			pObject = m_pObjects[i];

			if (pObject->m_Flags & FLAG_VISIBLE)
			{
				if (pContext->m_bDrawHidden && (pObject->m_Flags2 & 1))
					continue;
			}
			else
			{
				if (pContext->m_bDrawHidden && !(pObject->m_Flags & 0x400))
					continue;
			}

			if (T::Filter(pObject))
				pRejects->Add(pObject);
			else
				fn(pContext, pObject);
		}
	}

	LTObject	**m_pObjects;                           // +0x00
	uint32		m_nObjects;                             // +0x04
	LTLink		m_Link;                                 // +0x08
	uint32		m_nMaxObjects;                          // +0x14
	const char	*m_pSetName;                            // +0x18
};

struct VisibleSet
{
	uint8			m_pad_000[0x2e4];
	BaseObjectSet	m_SolidCanvases;                    // +0x2e4
	uint8			m_pad_300[0x304 - 0x300];
	BaseObjectSet	m_TranslucentCanvases;              // +0x304
};

class ObjectDrawList
{
public:
	void	Add(LTObject* pObject, DrawObjectFn fn);    // 0x10029980
};

// TU21's render-state list (d3d_statelist.cpp, d3d_savedstates.cpp).
class CRenderStateList
{
public:
	void	Restore();                                  // 0x10022a86
	void	Apply(CRenderStateList* pList, uint32 iStage);  // 0x10022a97
};

extern CRenderStateList	g_SavedStates;              // 0x10069728
extern ObjectDrawList*	g_pObjectDrawList;           // 0x100713cc
extern ViewParams			g_ViewParams;             // 0x1005b790
extern uint32				g_iCurTextureStage;         // 0x100638c0 (d3d_init.cpp)
// A texture stage's texel size (d3d_texture.cpp's, one per stage).
struct TexPair
{
	float	u, v;
};

extern TexPair				g_TexelSize[];              // 0x100672a8 (d3d_texture.cpp)
extern uint32				g_ClipFlags;                // 0x1005bd0c
// The near clip distances (d3d_viewparams.cpp's console variables).
extern CD3DConVar			g_CV_NearZ;                 // 0x1005b748
extern CD3DConVar			g_CV_ReallyCloseNearZ;      // 0x1005a328

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0
extern void			d3d_DisableTexture(uint32 iStage);  // 0x1000a22b
extern int			d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 flags);  // 0x100079e4
struct TLVertex;
struct ClipVert;
struct TLVert;
extern LTBOOL		d3d_ClipTLVerts(TLVertex** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags);  // 0x1000aec6 (drawworld.cpp)
extern void			d3d_TransformToCamera(ClipVert* pVert, LTMatrix* pMat);  // 0x10008719 (drawworld.cpp)
extern LTBOOL		ClipPoly(uint32 nClipFlags, ClipVert** ppVerts, int32* pnVerts);  // 0x1000af61 (drawworld.cpp; d3d_clip.h's)
extern void			ProjectVertPushed(TLVert* pOut, LTVector* pPos, float fPush);  // 0x100062e0 (d3d_modelvb.cpp)


class CustomDrawMgr : public ILTCustomDraw
{
public:
	CustomDrawMgr();

	virtual LTRESULT	DrawPrimitive(LTVertex *pVerts, uint32 nVerts, uint32 flags);
	virtual LTRESULT	SetState(LTRState state, uint32 val);
	virtual LTRESULT	GetState(LTRState state, uint32 &val);
	virtual LTRESULT	SetTexture(const char *pTexture);
	virtual LTRESULT	GetTexelSize(float &fSizeU, float &fSizeV);

	void		DrawCanvas(Canvas *pCanvas);
	void		SaveStates(Canvas *pCanvas);
	void		RestoreStates();

	uint32		m_SavedAlphaBlendEnable;                // +0x04
	uint32		m_SavedZEnable;                         // +0x08
	uint32		m_SavedZWriteEnable;                    // +0x0c
	uint32		m_SavedSrcBlend;                        // +0x10
	uint32		m_SavedDestBlend;                       // +0x14
	uint32		m_SavedTexAddr;                         // +0x18
	uint32		m_SavedColorOp;                         // +0x1c
	uint32		m_SavedAlphaOp;                         // +0x20
	uint32		m_SavedFogEnable;                       // +0x24
	uint32		m_bReallyClose;                         // +0x28
	uint32		m_States[NUM_LTRSTATES];                // +0x2c
};


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CustomDrawMgr	g_CustomDrawMgr;                        // 0x10069e08  $E 0x10023104
CD3DConVar		g_CV_DrawCanvases("DrawCanvases", 1.0f, LTNULL, LTNULL);  // 0x10069de8  $E 0x1002310e/0x10023113


// ---------------------------------------------------------------------- //
//  0x1002312d
// ---------------------------------------------------------------------- //
CustomDrawMgr::CustomDrawMgr()
{
	memset(m_States, 0, sizeof(m_States));
	m_bReallyClose = 0;
}


// ---------------------------------------------------------------------- //
//  0x1002314e
// ---------------------------------------------------------------------- //
void CustomDrawMgr::DrawCanvas(Canvas *pCanvas)
{
	if (pCanvas->m_Fn)
	{
		g_SavedStates.Restore();
		SaveStates(pCanvas);
		pCanvas->m_Fn(this, pCanvas, pCanvas->m_pFnUserData);
		RestoreStates();
	}
}


// ---------------------------------------------------------------------- //
//  0x1002318e -- save the device's states and set the canvas defaults.
// ---------------------------------------------------------------------- //
void CustomDrawMgr::SaveStates(Canvas *pCanvas)
{
	g_pD3DDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &m_SavedAlphaBlendEnable);
	g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	m_States[LTRSTATE_ALPHABLENDENABLE] = 0;

	g_pD3DDevice->GetRenderState(D3DRS_ZENABLE, &m_SavedZEnable);
	g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, 1);
	m_States[LTRSTATE_ZREADENABLE] = 1;

	g_pD3DDevice->GetRenderState(D3DRS_ZWRITEENABLE, &m_SavedZWriteEnable);
	g_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, 1);
	m_States[LTRSTATE_ZWRITEENABLE] = 1;

	g_pD3DDevice->GetRenderState(D3DRS_SRCBLEND, &m_SavedSrcBlend);
	g_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_States[LTRSTATE_SRCBLEND] = LTBLEND_SRCALPHA;

	g_pD3DDevice->GetRenderState(D3DRS_DESTBLEND, &m_SavedDestBlend);
	g_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_States[LTRSTATE_DESTBLEND] = LTBLEND_INVSRCALPHA;

	g_pD3DDevice->GetRenderState(D3DRS_FOGENABLE, &m_SavedFogEnable);
	if ((pCanvas->m_Flags & FLAG_FOGDISABLE) && m_SavedFogEnable)
		g_pD3DDevice->SetRenderState(D3DRS_FOGENABLE, 0);

	g_pD3DDevice->GetTextureStageState(0, D3DTSS_ADDRESS, &m_SavedTexAddr);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
	m_States[LTRSTATE_TEXADDR] = LTTEXADDR_WRAP;

	g_pD3DDevice->GetTextureStageState(0, D3DTSS_COLOROP, &m_SavedColorOp);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	m_States[LTRSTATE_COLOROP] = LTOP_MODULATE;

	g_pD3DDevice->GetTextureStageState(0, D3DTSS_ALPHAOP, &m_SavedAlphaOp);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	m_States[LTRSTATE_ALPHAOP] = LTOP_SELECTDIFFUSE;

	m_bReallyClose = (pCanvas->m_Flags >> 6) & 1;

	d3d_DisableTexture(g_iCurTextureStage);
}


// ---------------------------------------------------------------------- //
//  0x10023325
// ---------------------------------------------------------------------- //
void CustomDrawMgr::RestoreStates()
{
	g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, m_SavedAlphaBlendEnable);
	g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, m_SavedZEnable);
	g_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, m_SavedZWriteEnable);
	g_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, m_SavedSrcBlend);
	g_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, m_SavedDestBlend);
	g_pD3DDevice->SetRenderState(D3DRS_FOGENABLE, m_SavedFogEnable);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, m_SavedTexAddr);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, m_SavedColorOp);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, m_SavedAlphaOp);
}


// ---------------------------------------------------------------------- //
//  0x100233c9
//
//  RETAIL BUG: when the clipper rejects a really-close (or untransformed)
//  primitive, g_ViewParams.m_NearZ is left holding g_CV_ReallyCloseNearZ.m_fValue: the early return
//  skips the restore.
// ---------------------------------------------------------------------- //
LTRESULT CustomDrawMgr::DrawPrimitive(LTVertex *pVerts, uint32 nVerts, uint32 flags)
{
	LTVertex *pClipped;
	int nClipped;
	float saved;
	int i;

	pClipped = pVerts;
	nClipped = nVerts;
	if (nClipped < 3)
		return LT_ERROR;

	g_ClipFlags = 0x3f;

	if (!m_bReallyClose && !(flags & 0x40))
	{
		if (d3d_ClipTLVerts((TLVertex**)&pClipped, &nClipped, &g_ViewParams, 0))
		{
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, CANVASVERTEX_FORMAT,
				pClipped, nClipped, 0);
		}
	}
	else
	{
		saved = g_ViewParams.m_NearZ;
		g_ViewParams.m_NearZ = g_CV_ReallyCloseNearZ.m_fValue;

		for (i=0; i < nClipped; i++)
			d3d_TransformToCamera((ClipVert*)&pClipped[i], &g_ViewParams.m_mReallyCloseToCamera);

		if (!ClipPoly(g_ClipFlags, (ClipVert**)&pClipped, (int32*)&nClipped))
			return LT_OK;

		for (i=0; i < nClipped; i++)
			ProjectVertPushed((TLVert*)&pClipped[i], (LTVector*)&pClipped[i], g_CV_NearZ.m_fValue);

		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, CANVASVERTEX_FORMAT,
			pClipped, nClipped, 0);

		g_ViewParams.m_NearZ = saved;
	}

	return LT_OK;
}


// ---------------------------------------------------------------------- //
//  0x100234e3
// ---------------------------------------------------------------------- //
uint32 ConvertBlend(uint32 blend)
{
	uint32 ret = D3DBLEND_ONE;

	if (blend == LTBLEND_SRCALPHA)
		ret = D3DBLEND_SRCALPHA;
	else if (blend == LTBLEND_INVSRCALPHA)
		ret = D3DBLEND_INVSRCALPHA;
	else if (blend == LTBLEND_SRCCOLOR)
		ret = D3DBLEND_SRCCOLOR;
	else if (blend == LTBLEND_INVSRCCOLOR)
		ret = D3DBLEND_INVSRCCOLOR;
	else if (blend == LTBLEND_ZERO)
		ret = D3DBLEND_ZERO;

	return ret;
}

// 0x10023670
static uint32 ConvertTexAddr(uint32 addr, uint32 def)
{
	switch (addr)
	{
		case LTTEXADDR_WRAP:		return D3DTADDRESS_WRAP;
		case LTTEXADDR_CLAMP:		return D3DTADDRESS_CLAMP;
		default:					return ConvertTexAddr(def, def);
	}
}

// 0x10023688
static uint32 ConvertOp(uint32 op, uint32 def)
{
	switch (op)
	{
		case LTOP_SELECTTEXTURE:	return D3DTOP_SELECTARG1;
		case LTOP_SELECTDIFFUSE:	return D3DTOP_SELECTARG2;
		case LTOP_MODULATE:			return D3DTOP_MODULATE;
		case LTOP_ADD:				return D3DTOP_ADD;
		case LTOP_ADDSIGNED:		return D3DTOP_ADDSIGNED;
		default:					return ConvertOp(def, def);
	}
}


// The device calls of SetState, through these: their arguments are worked
// out before the device pointer is read.
inline void SetRS(uint32 state, uint32 val)
{
	g_pD3DDevice->SetRenderState(state, val);
}

inline void SetTSS(uint32 type, uint32 val)
{
	g_pD3DDevice->SetTextureStageState(0, type, val);
}


// ---------------------------------------------------------------------- //
//  0x10023512
// ---------------------------------------------------------------------- //
LTRESULT CustomDrawMgr::SetState(LTRState state, uint32 val)
{
	if ((uint32)state >= NUM_LTRSTATES)
		return LT_ERROR;

	switch (state)
	{
		case LTRSTATE_ALPHABLENDENABLE:
			SetRS(D3DRS_ALPHABLENDENABLE, val);
			break;
		case LTRSTATE_ZREADENABLE:
			SetRS(D3DRS_ZENABLE, val);
			break;
		case LTRSTATE_ZWRITEENABLE:
			SetRS(D3DRS_ZWRITEENABLE, val);
			break;
		case LTRSTATE_SRCBLEND:
			SetRS(D3DRS_SRCBLEND, ConvertBlend(val));
			break;
		case LTRSTATE_DESTBLEND:
			SetRS(D3DRS_DESTBLEND, ConvertBlend(val));
			break;
		case LTRSTATE_TEXADDR:
			SetTSS(D3DTSS_ADDRESS, ConvertTexAddr(val, LTTEXADDR_WRAP));
			break;
		case LTRSTATE_COLOROP:
			SetTSS(D3DTSS_COLOROP, ConvertOp(val, LTOP_MODULATE));
			break;
		case LTRSTATE_ALPHAOP:
			SetTSS(D3DTSS_ALPHAOP, ConvertOp(val, LTOP_SELECTTEXTURE));
			break;
	}

	m_States[state] = val;
	return LT_OK;
}


// ---------------------------------------------------------------------- //
//  0x100236b5
// ---------------------------------------------------------------------- //
LTRESULT CustomDrawMgr::GetState(LTRState state, uint32 &val)
{
	if ((uint32)state >= NUM_LTRSTATES)
		return LT_ERROR;

	val = m_States[state];
	return LT_OK;
}


// ---------------------------------------------------------------------- //
//  0x100236d2
// ---------------------------------------------------------------------- //
LTRESULT CustomDrawMgr::SetTexture(const char *pTexture)
{
	SharedTexture *pShared;
	RenderTexture *pRenderTexture;

	if (!pTexture)
	{
		d3d_DisableTexture(g_iCurTextureStage);
		return LT_OK;
	}

	pShared = g_pStruct->GetSharedTexture(pTexture);
	if (!pShared)
	{
		d3d_DisableTexture(g_iCurTextureStage);
		return LT_ERROR;
	}

	if (!d3d_SetTexture(pShared, g_iCurTextureStage, 0))
	{
		d3d_DisableTexture(g_iCurTextureStage);
		return LT_ERROR;
	}

	g_SavedStates.Restore();
	pRenderTexture = pShared->m_pRenderData;
	if (pRenderTexture)
		g_SavedStates.Apply((CRenderStateList*)pRenderTexture, g_iCurTextureStage);

	return LT_OK;
}


// ---------------------------------------------------------------------- //
//  0x10023753
// ---------------------------------------------------------------------- //
LTRESULT CustomDrawMgr::GetTexelSize(float &fSizeU, float &fSizeV)
{
	fSizeU = g_TexelSize[0].u;
	fSizeV = g_TexelSize[0].v;
	return LT_OK;
}


// ---------------------------------------------------------------------- //
//  Global functions.
// ---------------------------------------------------------------------- //
class Tester_Canvas
{
public:
	static bool Filter(LTObject *pObj) {
		return !(pObj->m_ClientFlags & CF_SOLIDCANVAS); }
};

// 0x10023770
void d3d_DrawCanvasCB(ViewParams* pContext, LTObject *pCanvas)
{
	g_CustomDrawMgr.DrawCanvas(pCanvas);
}

// 0x1002377f
void d3d_CanvasPreFrame()
{
}

// 0x10023780
void d3d_ProcessCanvas(LTObject *pObject)
{
	d3d_GetVisibleSet()->m_SolidCanvases.Add(pObject);
}

// 0x100237c4
void d3d_DrawSolidCanvases()
{
	if (g_CV_DrawCanvases.m_nValue)
	{
		VisibleSet* pSet = d3d_GetVisibleSet();
		Tester_Canvas tester;

		pSet->m_SolidCanvases.DrawFiltered(&g_ViewParams, d3d_DrawCanvasCB, &tester,
			&pSet->m_TranslucentCanvases);
	}
}

static void d3d_QueueCanvas(ViewParams* pContext, LTObject* pCanvas);

// 0x100237f6
void d3d_DrawTranslucentCanvases()
{
	if (g_CV_DrawCanvases.m_nValue)
		d3d_GetVisibleSet()->m_TranslucentCanvases.Draw(&g_ViewParams, d3d_QueueCanvas);
}

// 0x1002381c
static void d3d_QueueCanvas(ViewParams* pContext, LTObject* pCanvas)
{
	g_pObjectDrawList->Add(pCanvas, d3d_DrawCanvasCB);
}
