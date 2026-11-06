// *********************************************************************** //
//
//  MODULE:   COMMON_DRAW (.CPP) -- Primal Hunt's d3d.ren, TU10
//            (0x10010140..0x100116e3, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The renderer's shared draw state (the frame and view globals
//            every drawing module reads) and the view/frustum set-up.
//            Written so far: its initialisers, the frame-code helpers,
//            the dynamic light lists and the struct banks.  Not written:
//            0x100101f8..0x10011162, among them the 2766-byte view set-up
//            0x100104cb and the header COMDATs it calls out of line
//            (0x10010f99, 0x10010fc2, 0x10011021).
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"


// Five globals of classes whose constructors do nothing (their
// initialisers are a bare `ret`, 0x10010140..0x10010144).
struct CEmptyInit
{
	CEmptyInit() {}
};

CEmptyInit g_10010140;
CEmptyInit g_10010141;
CEmptyInit g_10010142;
CEmptyInit g_10010143;
CEmptyInit g_10010144;


// The view state shared by the drawing modules.
class ViewState
{
public:
	ViewState();                                            // 0x1001014f

	uint8       m_Reserved00[0x1c];
	uint32      m_1c;
	uint32      m_20;
	uint32      m_24;
	uint32      m_28;
};

ViewState g_ViewState;                                      // 0x1005b790

ViewState::ViewState()
{
	m_1c = 0;
	m_20 = 0;
	m_24 = 0;
	m_28 = 0;
}


// The world's blocks, as the frame-code reset walks them.
struct RenderPoly
{
	uint8       m_Reserved00[0x2c];
	uint16      m_FrameCode;                                // +0x2c
	uint16      m_Pad2e;
};

struct RenderLeaf
{
	uint8       m_Reserved00[0x46];
	uint16      m_FrameCode;                                // +0x46
};

struct RenderBlock
{
	uint8       m_Reserved00[0x88];
	RenderPoly* m_Polies;                                   // +0x88
	uint32      m_nPolies;                                  // +0x8c
	uint8       m_Reserved90[0xa0 - 0x90];
	RenderLeaf** m_Leaves;                                  // +0xa0
	uint32      m_nLeaves;                                  // +0xa4
};

struct WorldBsp
{
	uint8       m_Reserved00[0x04];
	RenderBlock* m_pBlock;                                  // +0x04
};

struct SceneWorld
{
	uint8       m_Reserved00[0x18c];
	WorldBsp**  m_WorldModels;                              // +0x18c
	uint32      m_nWorldModels;                             // +0x190
};

struct RenderWorld
{
	uint8       m_Reserved00[0x0c];
	uint16      m_FrameCode;                                // +0x0c
};

SceneWorld* g_pSceneWorld;                                  // 0x1005c208

void d3d_ClearBlockFrameCodes(RenderBlock* pBlock);

// 0x10010160 -- next frame code; when it wraps, clear every block's.
void d3d_IncFrameCode(RenderWorld* pWorld)
{
	uint32 i;

	if (pWorld->m_FrameCode == 0xFFFF)
	{
		pWorld->m_FrameCode = 1;

		for (i = 0; i < g_pSceneWorld->m_nWorldModels; i++)
		{
			d3d_ClearBlockFrameCodes(g_pSceneWorld->m_WorldModels[i]->m_pBlock);
		}
	}

	pWorld->m_FrameCode++;
}

// 0x100101aa
void d3d_ClearBlockFrameCodes(RenderBlock* pBlock)
{
	uint32 i;

	for (i = 0; i < pBlock->m_nPolies; i++)
		pBlock->m_Polies[i].m_FrameCode = 0;

	for (i = 0; i < pBlock->m_nLeaves; i++)
		pBlock->m_Leaves[i]->m_FrameCode = 0;
}


// ---------------------------------------------------------------------- //
//  The dynamic lights touching the frame's objects.
// ---------------------------------------------------------------------- //

// A link from an object to a light touching it.
struct LightLink
{
	LightLink*  m_pNext;                                    // +0x00
	uint8       m_Reserved04[0x10];
};

struct LitObject
{
	uint8       m_Reserved00[0x30];
	LightLink*  m_pLightLinks;                              // +0x30
};

// The frame's list of lit objects.
struct LitObjectRef
{
	LitObject*      m_pObject;                              // +0x00
	LitObjectRef*   m_pNext;                                // +0x04
};

StructBank      g_LightLinkBank;                            // 0x1005bcb8
StructBank      g_LitObjectBank;                            // 0x1005bcd8
StructBank      g_WorldPolyBank;                            // 0x1005d210
LitObjectRef*   g_pLitObjects;                              // 0x1005d24c
uint32          g_nLitObjects;                              // 0x1005bcac

void d3d_FreeLightLinks(LitObjectRef* pRef);

// 0x10011517
void d3d_FreeLitObjects()
{
	LitObjectRef *pCur, *pNext;

	g_nLitObjects = 0;
	pCur = g_pLitObjects;
	while (pCur)
	{
		g_nLitObjects++;
		d3d_FreeLightLinks(pCur);
		pNext = pCur->m_pNext;
		sb_Free(&g_LitObjectBank, pCur);
		pCur = pNext;
	}

	g_pLitObjects = LTNULL;
}

// 0x10011563
void d3d_FreeLightLinks(LitObjectRef* pRef)
{
	LightLink *pCur, *pNext;

	pCur = pRef->m_pObject->m_pLightLinks;
	while (pCur)
	{
		pNext = pCur->m_pNext;
		sb_Free(&g_LightLinkBank, pCur);
		pCur = pNext;
	}

	pRef->m_pObject->m_pLightLinks = LTNULL;
}

// 0x1001159b -- at the start of a frame.
void d3d_InitFrameState()
{
	d3d_FreeLitObjects();
}

// 0x100115a0
void d3d_InitWorldBanks()
{
	sb_Init2(&g_LightLinkBank, sizeof(LightLink), 0x40, 0x40);
	sb_Init2(&g_LitObjectBank, sizeof(LitObjectRef), 0x40, 0x40);
	sb_Init2(&g_WorldPolyBank, 0xc, 0x200, 0x800);
}

// 0x100115da
void d3d_TermWorldBanks()
{
	sb_Term(&g_LightLinkBank);
	sb_Term(&g_LitObjectBank);
	sb_Term(&g_WorldPolyBank);
}
