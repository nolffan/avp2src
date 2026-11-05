// *********************************************************************** //
//  ph/drawpolygrid.cpp -- Primal Hunt's d3d.ren, TU31
//  (0x1002be40..0x1002ddb8), /O2 /Ob2.
//
//  PolyGrids: the EnvMapPolyGrids console variable, the triangle clip
//  test, the OT_POLYGRID handlers (0x1004db14..0x1004db24) and the solid
//  and translucent passes.
//
//  /Ob2: the console variable's initialiser is one function (0x1002be40),
//  not a jmp thunk and its body.
//
//  Not yet written here: the polygrid draw (0x1002be60, 5,473 bytes) and
//  0x1002d6b0.
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


struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0x97 - 0x90];
	uint8		m_ColorA;                               // +0x097
};

struct SceneContext
{
	uint8		m_pad_000[0x4d4];
	uint32		m_bDrawHidden;                          // +0x4d4
};

typedef void (*DrawObjectFn)(SceneContext* pContext, LTObject* pObject);

extern void		AddDebugMessage(int level, const char* pMsg, ...);  // 0x10013b13

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

	void		Draw(SceneContext* pContext, DrawObjectFn fn);  // 0x100397e0

	template<class T>
	void		DrawFiltered(SceneContext* pContext, DrawObjectFn fn, T* pTester, BaseObjectSet* pRejects)
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
	uint8			m_pad_000[0x264];
	BaseObjectSet	m_SolidPolyGrids;                   // +0x264
	uint8			m_pad_280[0x284 - 0x280];
	BaseObjectSet	m_TranslucentPolyGrids;             // +0x284
};

class ObjectDrawList
{
public:
	void	Add(LTObject* pObject, DrawObjectFn fn);    // 0x10029980
};

extern ObjectDrawList*	g_pObjectDrawList;              // 0x100713cc
extern SceneContext		g_SceneContext;                 // 0x1005b790
extern int32			g_CV_DrawPolyGrids;             // 0x1004a768
extern LTMatrix			g_mFullTransform;               // 0x1005b92c

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0
extern void			dfree(void* ptr);                   // 0x10013ac5

extern void d3d_DrawPolyGrid(SceneContext* pContext, LTObject* pObject);  // 0x1002be60


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_EnvMapPolyGrids("EnvMapPolyGrids", 1.0f, LTNULL, LTNULL);  // 0x100772f8  $E 0x1002be40


// ---------------------------------------------------------------------- //
//  0x1002d3d0 -- which of the frustum planes a triangle is wholly
//  outside (1), straddles (2, with the planes still to clip in
//  *pOutMask) or is inside (0).
// ---------------------------------------------------------------------- //
struct ClipVert
{
	float	x, y, z;
};

extern float g_NearZ;                                   // 0x1005b818
extern float g_FarZ;                                    // 0x1005b820

#define CLIP_NEAR(v)	((v)->z >= g_NearZ)
#define CLIP_LEFT(v)	(-(v)->z < (v)->x)
#define CLIP_TOP(v)		((v)->y < (v)->z)
#define CLIP_RIGHT(v)	((v)->x < (v)->z)
#define CLIP_BOTTOM(v)	(-(v)->z < (v)->y)
#define CLIP_FAR(v)		((v)->z <= g_FarZ)

#define CLIP_TEST(bit, test, remaining)                                  \
	if (nClipMask & (bit))                                               \
	{                                                                    \
		nInside = test(pV0) + test(pV1) + test(pV2);                     \
		if (nInside == 0)                                                \
			return 1;                                                    \
		if (nInside != 3)                                                \
		{                                                                \
			*pOutMask = nClipMask & (remaining);                         \
			return 2;                                                    \
		}                                                                \
	}

int d3d_ClassifyTriangle(uint32 nClipMask, uint32* pOutMask,
                         ClipVert* pV0, ClipVert* pV1, ClipVert* pV2)
{
	int nInside;

	CLIP_TEST(0x04, CLIP_LEFT,   0x3f)
	CLIP_TEST(0x10, CLIP_RIGHT,  0x3b)
	CLIP_TEST(0x08, CLIP_TOP,    0x2b)
	CLIP_TEST(0x20, CLIP_BOTTOM, 0x23)
	CLIP_TEST(0x01, CLIP_NEAR,   0x03)
	CLIP_TEST(0x02, CLIP_FAR,    0x02)

	return 0;
}


// ---------------------------------------------------------------------- //
//  0x1002daf0 -- the OT_POLYGRID ProcessObject handler.
// ---------------------------------------------------------------------- //
void d3d_ProcessPolyGrid(LTObject *pObject)
{
	d3d_GetVisibleSet()->m_SolidPolyGrids.Add(pObject);
}


class Tester_PolyGrid
{
public:
	static bool Filter(LTObject *pObj) {
		return pObj->m_ColorA != 0xFF; }
};

// ---------------------------------------------------------------------- //
//  0x1002db40 -- draw the opaque ones now, set the rest aside.
// ---------------------------------------------------------------------- //
void d3d_DrawSolidPolyGrids()
{
	if (g_CV_DrawPolyGrids)
	{
		VisibleSet* pSet = d3d_GetVisibleSet();
		Tester_PolyGrid tester;

		pSet->m_SolidPolyGrids.DrawFiltered(&g_SceneContext, d3d_DrawPolyGrid, &tester,
			&pSet->m_TranslucentPolyGrids);
	}
}


static void d3d_QueuePolyGrid(SceneContext* pContext, LTObject* pObject);

// ---------------------------------------------------------------------- //
//  0x1002dc20
// ---------------------------------------------------------------------- //
void d3d_DrawTranslucentPolyGrids()
{
	if (g_CV_DrawPolyGrids)
		d3d_GetVisibleSet()->m_TranslucentPolyGrids.Draw(&g_SceneContext, d3d_QueuePolyGrid);
}

// 0x1002dc50
static void d3d_QueuePolyGrid(SceneContext* pContext, LTObject* pObject)
{
	g_pObjectDrawList->Add(pObject, d3d_DrawPolyGrid);
}


// ---------------------------------------------------------------------- //
//  0x1002dc70 / 0x1002dc90 -- the module's init and term: the shared
//  vertex and index buffers.
// ---------------------------------------------------------------------- //
void*	g_pPolyGridVerts;                               // 0x100762e8
uint32	g_nPolyGridVerts;                               // 0x10075ee0
void*	g_pPolyGridIndices;                             // 0x100762f0
uint32	g_nPolyGridIndices;                             // 0x10075ee4

void d3d_PolyGridModuleInit()
{
	g_pPolyGridVerts = LTNULL;
	g_nPolyGridVerts = 0;
	g_pPolyGridIndices = LTNULL;
	g_nPolyGridIndices = 0;
}

void d3d_PolyGridModuleTerm()
{
	dfree(g_pPolyGridVerts);
	dfree(g_pPolyGridIndices);
	g_pPolyGridVerts = LTNULL;
	g_nPolyGridVerts = 0;
	g_pPolyGridIndices = LTNULL;
	g_nPolyGridIndices = 0;
}


// ---------------------------------------------------------------------- //
//  0x1002dcd0 -- project polygrid vertices (0x28 bytes apart) in place.
// ---------------------------------------------------------------------- //
struct PolyGridVert
{
	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
	uint8		m_pad_10[0x28 - 0x10];
};

void d3d_TransformPolyGridVerts(PolyGridVert* pVerts, uint32 nVerts)
{
	for (; nVerts; nVerts--)
	{
		pVerts->m_RHW = MatVMul_InPlace_H(&g_mFullTransform, &pVerts->m_Vec);
		pVerts++;
	}
}
