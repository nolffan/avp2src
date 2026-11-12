// *********************************************************************** //
//  ph/drawparticles.cpp -- Primal Hunt's d3d.ren, TU29
//  (0x1002a480..0x1002a70f), /O1.
//
//  Particle systems: the PSSrcBlend / PSDestBlend console variables, the
//  shared particle index list (also read by TU05's batch drawer), the
//  OT_PARTICLESYSTEM entry of the object handler table (PreFrame and
//  ProcessObject at 0x1004db0c/0x1004db10), and the sorted draw.
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


// IDirect3DDevice7, the two slots used here (index 20 and 21).
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
};

extern CD3DDevice*	g_pD3DDevice;                       // 0x100638c8

#define D3DRS_SRCBLEND		19
#define D3DRS_DESTBLEND		20
#define D3DRS_FOGENABLE		28
#define D3DRS_FOGCOLOR		34

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


// The parts of LTObject drawn from here.
struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0xac - 0x90];
	LTVector	m_Scale;                                // +0x0ac
	uint8		m_pad_0b8[0x210 - 0xb8];
	LTVector	m_SystemCenter;                         // +0x210
	float		m_SystemRadius;                         // +0x21c
};

// The view (common_draw.cpp's), as far as it is read here: its frustum
// planes, and the really-close (the player-view) ones.
struct ViewParams
{
	uint8		m_pad_000[0x3d0];
	LTPlane		m_ClipPlanes[6];                        // +0x3d0 (0x1005bb60)
	LTPlane		m_ReallyClosePlanes[6];                 // +0x430 (0x1005bbc0)
};

struct VisibleSet;

class BaseObjectSet
{
public:
	void	Add(LTObject* pObject);                                 // 0x10023797
	void	Draw(ViewParams* pContext,
		         void (*pDrawFn)(ViewParams* pContext, LTObject* pObject)); // 0x100397e0
};

struct VisibleSet
{
	uint8		m_pad_000[0x2c4];
	BaseObjectSet	m_ParticleSystems;                      // +0x2c4
};

class ObjectDrawList
{
public:
	void	Add(LTObject* pObject,
		        void (*pDrawFn)(ViewParams* pContext, LTObject* pObject)); // 0x10029980
};

extern ObjectDrawList*	g_pObjectDrawList;           // 0x100713cc
extern ViewParams			g_ViewParams;             // 0x1005b790
extern int32				g_CV_DrawParticles;         // 0x1004a784

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0
extern LTBOOL		d3d_ClipSphere(LTVector* pCenter, float fRadius,
					               LTPlane* pPlanes, uint32* pOut);  // 0x1000b5eb (drawworld.cpp)
extern void			d3d_GetBlendStates(LTObject* pObject, uint32* pSrcBlend,
					                   uint32* pDestBlend, uint32* pFogEnable,
					                   uint32* pFogColor);                    // 0x10025de5
extern void			d3d_DrawParticles(LTObject* pSystem);         // 0x10008c90


#define PARTICLE_BATCH_SIZE		128


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
//  The object's .bss is laid out last-defined first: 0x10072810 PSDestBlend,
//  0x10072c30 PSSrcBlend, 0x10072c50 the indices, 0x10074250 the flag.
//  (0x10072830..0x10072c30 and 0x10072f50..0x10074250 hold more of this
//  object's globals that no code addresses directly.)
int32		g_bParticleIndicesInitted;                                   // 0x10074250
uint16		g_ParticleIndices[PARTICLE_BATCH_SIZE * 6];                  // 0x10072c50

CD3DConVar	g_CV_PSSrcBlend("PSSrcBlend", -1.0f, LTNULL, LTNULL);          // 0x1002a480 / 0x1002a485
CD3DConVar	g_CV_PSDestBlend("PSDestBlend", -1.0f, LTNULL, LTNULL);        // 0x1002a4a3 / 0x1002a4a8


// ---------------------------------------------------------------------- //
//  0x1002a4c6 -- the PreFrame handler: build the index list once.
// ---------------------------------------------------------------------- //
void d3d_ParticlePreFrame()
{
	uint32 i;
	uint16 curIndex;
	uint16 *pCurIndex;

	if (!g_bParticleIndicesInitted)
	{
		g_bParticleIndicesInitted = 1;

		curIndex = 0;
		pCurIndex = g_ParticleIndices;
		for (i = 0; i < PARTICLE_BATCH_SIZE; i++)
		{
			*pCurIndex++ = curIndex++;
			*pCurIndex++ = curIndex++;
			*pCurIndex++ = curIndex++;

			*pCurIndex++ = curIndex - 3;
			*pCurIndex++ = curIndex - 1;
			*pCurIndex++ = curIndex;
			++curIndex;
		}
	}
}


// ---------------------------------------------------------------------- //
//  0x1002a517 -- the ProcessObject handler.
// ---------------------------------------------------------------------- //
void d3d_ProcessParticles(LTObject* pObject)
{
	d3d_GetVisibleSet()->m_ParticleSystems.Add(pObject);
}


#define MAX(a, b)	((a) > (b) ? (a) : (b))

// ---------------------------------------------------------------------- //
//  0x1002a52e -- draw one particle system, with its blend and fog states.
// ---------------------------------------------------------------------- //
static void d3d_DrawParticleSystem(ViewParams* pContext, LTObject* pSystem)
{
	float fRadius;
	uint32 visOut;
	uint32 srcBlend, destBlend, fogEnable, fogColor;

	fRadius = MAX(pSystem->m_Scale.x, MAX(pSystem->m_Scale.y, pSystem->m_Scale.z)) * pSystem->m_SystemRadius;
	if (!d3d_ClipSphere(&pSystem->m_SystemCenter, fRadius,
		(pSystem->m_Flags & FLAG_REALLYCLOSE) ? g_ViewParams.m_ReallyClosePlanes : g_ViewParams.m_ClipPlanes, &visOut))
		return;

	pSystem->m_Flags |= 0x40000000;

	d3d_GetBlendStates(pSystem, &srcBlend, &destBlend, &fogEnable, &fogColor);

	StateSet ssSrcBlend(D3DRS_SRCBLEND, srcBlend);
	StateSet ssDestBlend(D3DRS_DESTBLEND, destBlend);
	StateSet ssFogEnable(D3DRS_FOGENABLE, fogEnable);
	StateSet ssFogColor(D3DRS_FOGCOLOR, fogColor);

	d3d_DrawParticles(pSystem);
}


static void d3d_QueueParticleSystem(ViewParams* pContext, LTObject* pSystem);

// ---------------------------------------------------------------------- //
//  0x1002a6d6 -- queue every visible particle system for the sorted draw.
// ---------------------------------------------------------------------- //
void d3d_DrawParticleSystems()
{
	if (g_CV_DrawParticles)
	{
		VisibleSet* pVisibleSet = d3d_GetVisibleSet();
		pVisibleSet->m_ParticleSystems.Draw(&g_ViewParams, d3d_QueueParticleSystem);
	}
}


// ---------------------------------------------------------------------- //
//  0x1002a6fa
// ---------------------------------------------------------------------- //
static void d3d_QueueParticleSystem(ViewParams* pContext, LTObject* pSystem)
{
	g_pObjectDrawList->Add(pSystem, d3d_DrawParticleSystem);
}
