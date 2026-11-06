// *********************************************************************** //
//  ph/d3d_savedstates.cpp -- Primal Hunt's d3d.ren, TU21, first object
//  (0x10022a50..0x10022a85), /O1 /Ob2.
//
//  The global render-state list (0x10069728): its initialiser, its
//  implicit constructor and its destructor's atexit thunk.  The class's
//  members are d3d_statelist.cpp's, the next object: the initialiser is
//  one body (/Ob2), while those members call each other where /Ob2 would
//  have inlined them (SetState's SaveState), so the two are compiled
//  apart.
// *********************************************************************** //

#include <d3d.h>
#include <vector>

#include "ltbasedefs.h"
#include "ph_decls.h"


extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8


struct RenderState
{
	D3DRENDERSTATETYPE			m_State;                // +0x00
	uint32						m_Value;                // +0x04
};

struct StageState
{
	uint32						m_Stage;                // +0x00
	D3DTEXTURESTAGESTATETYPE	m_State;                // +0x04
	uint32						m_Value;                // +0x08
};

class CRenderStateList
{
public:
						~CRenderStateList();            // 0x1001a4cc

	void				Restore();
	void				Apply(CRenderStateList *pList, uint32 iStage);
	void				SetStageState(StageState *pState, uint32 iStage);
	void				SetState(RenderState *pState);
	void				SaveState(RenderState *pState);
	void				SaveStageState(StageState *pState, uint32 iStage);
	void				RestoreStates();
	void				RestoreStageStates();

	std::vector<RenderState>	m_States;               // +0x00
	std::vector<StageState>		m_StageStates;          // +0x0c
};


CRenderStateList g_SavedStates;                         // 0x10069728  $E 0x10022a50
