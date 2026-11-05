// *********************************************************************** //
//
//  MODULE:   COMMON_DRAW (.CPP) -- Primal Hunt's d3d.ren, TU10
//            (0x10010140..0x100116e3, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The renderer's shared draw state (the frame and view globals
//            every drawing module reads) and the view/frustum set-up.
//            Only the start of the object is written so far: its
//            initialisers and the frame-code helpers.
//
// *********************************************************************** //

#include "ltbasedefs.h"


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
