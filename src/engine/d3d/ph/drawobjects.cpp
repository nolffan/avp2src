// *********************************************************************** //
//  ph/drawobjects.cpp -- Primal Hunt's d3d.ren, TU28
//  (0x10029390..0x1002a480), /O2.
//
//  The object handler table (0x1004da88, one entry per object type), the
//  per-frame object queue flush with its profiling counters, and the
//  sorted draw list: a function-local static CMoArray of (object, draw
//  function, distance) entries, sorted back to front, whose CMoArray
//  members are instantiated here first (0x10029a10..0x1002a460).
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


// The renderer's console variable (TU12's, constructor 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32		m_nValue;
	float		m_fValue;
	uint8		m_pad_08[0x20 - 0x08];
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


struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0x194 - 0x90];
	LTVector	m_Pos;                                  // +0x194
};

struct ViewParams
{
	uint8		m_pad_000[0x4b4];
	LTVector	m_Pos;                                  // +0x4b4
	uint8		m_pad_4c0[0x4d4 - 0x4c0];
	uint32		m_bDrawHidden;                          // +0x4d4
};

typedef void (*DrawObjectFn)(ViewParams* pContext, LTObject* pObject);

// The renderer's view of the engine's RenderStruct.
struct RenderStruct
{
	uint8		m_pad_00[0x24];
	void		(*m_pfnPoll)();                         // +0x24
	uint8		m_pad_28[0x60 - 0x28];
	uint32		m_SortedTicks;                          // +0x60
	uint32		m_ObjectTicks;                          // +0x64
	uint32		m_FlushTicks;                           // +0x68
};

struct SceneDesc
{
	uint8		m_pad_00[0x10];
	uint32*		m_pVisTicks;                            // +0x10
};

struct BaseObjectSet
{
	LTObject	**m_pObjects;                           // +0x00
	uint32		m_nObjects;                             // +0x04
	uint8		m_pad_08[0x20 - 0x08];
};

struct VisibleSet
{
	uint8			m_pad_000[0x164];
	BaseObjectSet	m_Set164;                           // +0x164
	uint8			m_pad_184[0x1a4 - 0x184];
	BaseObjectSet	m_Set1a4;                           // +0x1a4
	uint8			m_pad_1c4[0x204 - 0x1c4];
	BaseObjectSet	m_Set204;                           // +0x204
	uint8			m_pad_224[0x284 - 0x224];
	BaseObjectSet	m_Set284;                           // +0x284
	BaseObjectSet	m_Set2a4;                           // +0x2a4
	BaseObjectSet	m_Set2c4;                           // +0x2c4
	uint8			m_pad_2e4[0x304 - 0x2e4];
	BaseObjectSet	m_Set304;                           // +0x304
};

extern RenderStruct*	g_pStruct;                      // 0x1005df08
extern SceneDesc*	g_pSceneDesc;                   // 0x1005c14c (common_draw.cpp)
extern ViewParams		g_ViewParams;                 // 0x1005b790
void				(*g_pfnPostQueueHook)();               // 0x10072808

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0

extern void		d3d_DrawSolidWorldModels();
extern void		d3d_DrawSolidModels();
extern void		d3d_DrawSolidPolyGrids();
extern void		d3d_DrawSolidCanvases();                // 0x100237c4
extern void		d3d_EnablePunchThrough();
extern void		d3d_DrawChromakeyWorldModels();
extern void		d3d_DrawChromakeyModels();
extern void		d3d_DisablePunchThrough();
extern void		d3d_SetTranslucentObjectStates(LTBOOL bAdditive);
extern void		d3d_DrawParticleSystems();              // 0x1002a6d6
extern void		d3d_DrawTranslucentPolyGrids();
extern void		d3d_DrawLineSystems();
extern void		d3d_QueueTranslucentWorldModels();
extern void		d3d_QueueTranslucentModels();
extern void		d3d_DrawTranslucentCanvases();          // 0x100237f6
extern void		d3d_QueueTranslucentSprites();
extern void		d3d_DrawNoZSprites();
extern void		d3d_UnsetTranslucentObjectStates(LTBOOL bZWrite);


// ---------------------------------------------------------------------- //
//  The object handler table.
// ---------------------------------------------------------------------- //
#define NUM_OBJECTTYPES		11

struct ObjectHandler
{
	// One-time module init and term.
	void		(*m_ModuleInit)();
	void		(*m_ModuleTerm)();
	uint32		m_bModuleInitted;

	// This is called before each frame.
	void		(*m_PreFrameFn)();

	// This is called as objects are tagged visible.
	void		(*m_ProcessObjectFn)(LTObject *pObject);
};

extern void d3d_InitModels();                           // 0x1000b722 (drawmodel.cpp)
extern void d3d_TermModels();                           // 0x1000b75a (drawmodel.cpp)
extern void d3d_InitModelLists();                       // 0x1000b65e (drawmodel.cpp)
extern void d3d_ProcessModel(LTObject *pObject);        // 0x10025b2f
extern void d3d_NullPreFrame();                         // 0x10030120
extern void d3d_ProcessWorldModel(LTObject *pObject);   // 0x10030130
extern void d3d_ProcessSprite(LTObject *pObject);       // 0x1002f7b0
extern void d3d_ProcessLight(LTObject *pObject);        // 0x10024540
extern void d3d_ParticlePreFrame();                     // 0x1002a4c6
extern void d3d_ProcessParticles(LTObject *pObject);    // 0x1002a517
extern void d3d_PolyGridModuleInit();                   // 0x1002dc70
extern void d3d_PolyGridModuleTerm();                   // 0x1002dc90
extern void d3d_ProcessPolyGrid(LTObject *pObject);     // 0x1002daf0
extern void d3d_ProcessLineSystem(LTObject *pObject);   // 0x100249c0
extern void d3d_CanvasPreFrame();                       // 0x1002377f
extern void d3d_ProcessCanvas(LTObject *pObject);       // 0x10023780

ObjectHandler g_ObjectHandlers[NUM_OBJECTTYPES] =       // 0x1004da88
{
	// OT_NORMAL
	LTNULL, LTNULL, 0, LTNULL, LTNULL,
	// OT_MODEL
	d3d_InitModels, d3d_TermModels, 0, d3d_InitModelLists, d3d_ProcessModel,
	// OT_WORLDMODEL
	LTNULL, LTNULL, 0, d3d_NullPreFrame, d3d_ProcessWorldModel,
	// OT_SPRITE
	LTNULL, LTNULL, 0, d3d_NullPreFrame, d3d_ProcessSprite,
	// OT_LIGHT
	LTNULL, LTNULL, 0, LTNULL, d3d_ProcessLight,
	// OT_CAMERA
	LTNULL, LTNULL, 0, LTNULL, LTNULL,
	// OT_PARTICLESYSTEM
	LTNULL, LTNULL, 0, d3d_ParticlePreFrame, d3d_ProcessParticles,
	// OT_POLYGRID
	d3d_PolyGridModuleInit, d3d_PolyGridModuleTerm, 0, d3d_NullPreFrame, d3d_ProcessPolyGrid,
	// OT_LINESYSTEM
	LTNULL, LTNULL, 0, d3d_NullPreFrame, d3d_ProcessLineSystem,
	// OT_CONTAINER
	LTNULL, LTNULL, 0, LTNULL, d3d_ProcessWorldModel,
	// OT_CANVAS
	LTNULL, LTNULL, 0, d3d_CanvasPreFrame, d3d_ProcessCanvas,
};


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_SortProfile("SortProfile", 0.0f, LTNULL, LTNULL);     // 0x100717d0  $E 0x10029390
CD3DConVar	g_CV_DrawSorted("DrawSorted", 1.0f, LTNULL, LTNULL);       // 0x100713a8  $E 0x100293b0


// ---------------------------------------------------------------------- //
//  0x100293d0
// ---------------------------------------------------------------------- //
void d3d_InitObjectModules()
{
	uint32 i;

	for (i=0; i < NUM_OBJECTTYPES; i++)
	{
		if (g_ObjectHandlers[i].m_ModuleInit)
			g_ObjectHandlers[i].m_ModuleInit();

		g_ObjectHandlers[i].m_bModuleInitted = 1;
	}
}


// ---------------------------------------------------------------------- //
//  0x10029400
// ---------------------------------------------------------------------- //
void d3d_TermObjectModules()
{
	uint32 i;

	for (i=0; i < NUM_OBJECTTYPES; i++)
	{
		if (g_ObjectHandlers[i].m_ModuleTerm && g_ObjectHandlers[i].m_bModuleInitted)
			g_ObjectHandlers[i].m_ModuleTerm();

		g_ObjectHandlers[i].m_bModuleInitted = 0;
	}
}


// ---------------------------------------------------------------------- //
//  0x10029430
// ---------------------------------------------------------------------- //
void d3d_InitObjectQueues()
{
	int i;

	for (i=0; i < NUM_OBJECTTYPES; i++)
	{
		if (g_ObjectHandlers[i].m_PreFrameFn)
			g_ObjectHandlers[i].m_PreFrameFn();
	}
}


// ---------------------------------------------------------------------- //
//  The sorted draw list.
// ---------------------------------------------------------------------- //
// The element's (empty) constructor is what brings GenAppend and
// GenRemoveAt (0x10029aa0, 0x10029c20) out as retail's, as in TU30.
// The inliner's budget is spent from the end of a function backwards:
// Add's Set and operator[] after the Append (the in-place path second)
// leave Insert2 out of line, and d3d_FlushObjectQueues' draw loop indexing
// the list at each use, with SetSize (not SetSize2), leaves the static's
// Clear and Init out of line, as retail has them. Still near: Sort, where
// retail calls _CVector's constructor out of line.
struct ObjectDrawEntry
{
	ObjectDrawEntry() {}

	void	Set(LTObject* pObject, DrawObjectFn fn)
	{
		m_pObject = pObject;
		m_Fn = fn;
	}

	LTObject*		m_pObject;                          // +0x00
	DrawObjectFn	m_Fn;                               // +0x04
	float			m_fDistSqr;                         // +0x08
};

class ObjectDrawList;
ObjectDrawList*	g_pObjectDrawList;                      // 0x100713cc

class ObjectDrawList : public CMoArray<ObjectDrawEntry>
{
public:
	ObjectDrawList() : m_nUsed(0)
	{
		g_pObjectDrawList = this;
	}

	void	Sort(ViewParams* pContext);
	void	Add(LTObject* pObject, DrawObjectFn fn);

	uint32	m_nUsed;                                    // +0x14
};

// The shell sort's gaps and the shifts that count its comparisons.
uint32	g_SortGaps[10] = { 512, 256, 128, 64, 32, 16, 8, 4, 2, 1 };    // 0x1004db64
uint32	g_SortShifts[10] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };          // 0x1004db8c


// ---------------------------------------------------------------------- //
//  0x10029450 -- the end of the visible-object pass: flush every queue,
//  then draw the sorted list back to front.
// ---------------------------------------------------------------------- //
void d3d_FlushObjectQueues()
{
	uint32 i;

	g_pStruct->m_pfnPoll();

	{
		CountAdder cntObjects(&g_pStruct->m_ObjectTicks);
		{
			CountAdder cntVis(g_pSceneDesc->m_pVisTicks);
			d3d_DrawSolidWorldModels();
			g_pStruct->m_pfnPoll();
		}
	}

	{
		CountAdder cntSorted(&g_pStruct->m_SortedTicks);
		d3d_DrawSolidModels();
		g_pStruct->m_pfnPoll();
	}

	d3d_DrawSolidPolyGrids();
	g_pStruct->m_pfnPoll();
	d3d_DrawSolidCanvases();
	d3d_EnablePunchThrough();
	d3d_DrawChromakeyWorldModels();
	d3d_DrawChromakeyModels();
	d3d_DisablePunchThrough();

	if (g_pfnPostQueueHook)
		g_pfnPostQueueHook();

	{
		CountAdder cntFlush(&g_pStruct->m_FlushTicks);

		static ObjectDrawList s_DrawList;

		uint32 nObjects = d3d_GetVisibleSet()->m_Set304.m_nObjects +
		                  d3d_GetVisibleSet()->m_Set2c4.m_nObjects +
		                  d3d_GetVisibleSet()->m_Set2a4.m_nObjects +
		                  d3d_GetVisibleSet()->m_Set284.m_nObjects +
		                  d3d_GetVisibleSet()->m_Set204.m_nObjects +
		                  d3d_GetVisibleSet()->m_Set1a4.m_nObjects +
		                  d3d_GetVisibleSet()->m_Set164.m_nObjects;
		if (nObjects > s_DrawList.GetSize())
			s_DrawList.SetSize(nObjects);

		d3d_SetTranslucentObjectStates(0);
		d3d_DrawParticleSystems();
		g_pStruct->m_pfnPoll();
		d3d_DrawTranslucentPolyGrids();
		g_pStruct->m_pfnPoll();
		d3d_DrawLineSystems();
		g_pStruct->m_pfnPoll();
		d3d_QueueTranslucentWorldModels();
		g_pStruct->m_pfnPoll();
		d3d_QueueTranslucentModels();
		g_pStruct->m_pfnPoll();
		d3d_DrawTranslucentCanvases();
		d3d_QueueTranslucentSprites();

		if (s_DrawList.m_nUsed)
		{
			s_DrawList.Sort(&g_ViewParams);

			for (i=0; i < s_DrawList.m_nUsed; i++)
			{
				if (g_ViewParams.m_bDrawHidden)
				{
					if (s_DrawList[i].m_pObject->m_Flags2 & 1)
						continue;
				}

				if ((s_DrawList[i].m_pObject->m_Flags & FLAG_VISIBLE) ||
				    (g_ViewParams.m_bDrawHidden && (s_DrawList[i].m_pObject->m_Flags & 0x400)))
					s_DrawList[i].m_Fn(&g_ViewParams, s_DrawList[i].m_pObject);
			}
		}

		s_DrawList.m_nUsed = 0;

		d3d_DrawNoZSprites();
		d3d_UnsetTranslucentObjectStates(1);
	}
}


// ---------------------------------------------------------------------- //
//  0x100297a0 -- distance to the camera, then a shell sort, far first;
//  really-close objects sort after the rest.
// ---------------------------------------------------------------------- //
void ObjectDrawList::Sort(ViewParams* pContext)
{
	uint32 i, k;
	int j, n, a;
	uint32 nShift;
	int nGap;
	LTObject* pObject;
	ObjectDrawEntry* pArr;
	ObjectDrawEntry* pA;
	ObjectDrawEntry* pB;
	ObjectDrawEntry tmp;
	BOOL bSwap;

	for (i=0; i < m_nUsed; i++)
	{
		pObject = (*this)[i].m_pObject;

		if (!(pObject->m_Flags & FLAG_REALLYCLOSE))
			(*this)[i].m_fDistSqr = pObject->m_Pos.DistSqr(pContext->m_Pos);
		else
			(*this)[i].m_fDistSqr = pObject->m_Pos.MagSqr();
	}

	pArr = GetArray();

	for (k=0; k < 10; k++)
	{
		nGap = g_SortGaps[k];
		nShift = g_SortShifts[k];
		n = (m_nUsed - 1) >> nShift;

		for (j=0; j < n; j++)
		{
			a = j << nShift;
			pA = &pArr[a];
			pB = &pArr[a + nGap];

			if (!((pA->m_pObject->m_Flags ^ pB->m_pObject->m_Flags) & FLAG_REALLYCLOSE))
			{
				if (!(pA->m_fDistSqr < pB->m_fDistSqr))
					continue;
			}
			else
			{
				bSwap = (uint8)(pB->m_pObject->m_Flags & FLAG_REALLYCLOSE) < (uint8)(pA->m_pObject->m_Flags & FLAG_REALLYCLOSE);
				if (!bSwap)
					continue;
			}

			tmp = *pA;
			*pA = *pB;
			*pB = tmp;

			if (j)
				j -= 2;
		}
	}
}


// ---------------------------------------------------------------------- //
//  0x10029980 -- queue one object, or draw it now when sorting is off.
// ---------------------------------------------------------------------- //
void ObjectDrawList::Add(LTObject* pObject, DrawObjectFn fn)
{
	if (!g_CV_DrawSorted.m_nValue)
	{
		fn(&g_ViewParams, pObject);
		return;
	}

	if (GetSize() <= m_nUsed)
	{
		ObjectDrawEntry entry;

		entry.Set(pObject, fn);
		Append(entry);
	}
	else
	{
		(*this)[m_nUsed].Set(pObject, fn);
	}

	m_nUsed++;
}
