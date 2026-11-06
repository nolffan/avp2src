// *********************************************************************** //
//  ph/d3d_statelist.cpp -- Primal Hunt's d3d.ren, TU21
//  (0x10022a86..0x10022dd1), /O1.
//
//  A render-state list: render states and texture-stage states set
//  through it are saved first (their old values pushed on two STLport
//  vectors), so that Restore puts the device back as it was.  Its one
//  global instance (0x10069728) is d3d_savedstates.cpp's.  Its destructor (0x1001a4cc) and its render-state
//  vector's push_back (0x10019400) were instantiated first elsewhere (TU16,
//  TU15); the rest of the vectors' members are instantiated here
//  (0x10022c31..0x10022dce).
//
//  Built with STLport 4.0 (src/LT2/lithshared/stl) ahead of VC6's headers,
//  as the vectors' code shows (STLport's 12-byte vector, its
//  _M_insert_overflow and its thread-safe node allocator).
// *********************************************************************** //

#include <d3d.h>
#include <vector>

#include "ltbasedefs.h"


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


// 0x10022a86 -- put back everything saved.
void CRenderStateList::Restore()
{
	RestoreStates();
	RestoreStageStates();
}


// 0x10022a97 -- set another list's states, saving the old values here.
void CRenderStateList::Apply(CRenderStateList *pList, uint32 iStage)
{
	std::vector<RenderState>::iterator it, itEnd;
	std::vector<StageState>::iterator itStage, itStageEnd;

	itEnd = pList->m_States.end();
	for (it = pList->m_States.begin(); it != itEnd; it++)
		SetState(it);

	itStageEnd = pList->m_StageStates.end();
	for (itStage = pList->m_StageStates.begin(); itStage != itStageEnd; itStage++)
		SetStageState(itStage, iStage);
}


// 0x10022add
void CRenderStateList::SetStageState(StageState *pState, uint32 iStage)
{
	SaveStageState(pState, iStage);
	g_pD3DDevice->SetTextureStageState(iStage, pState->m_State, pState->m_Value);
}


// 0x10022b08
void CRenderStateList::SetState(RenderState *pState)
{
	SaveState(pState);
	g_pD3DDevice->SetRenderState(pState->m_State, pState->m_Value);
}


// 0x10022b27
void CRenderStateList::SaveState(RenderState *pState)
{
	uint32 value;
	RenderState saved;

	g_pD3DDevice->GetRenderState(pState->m_State, (unsigned long*)&value);

	saved.m_State = pState->m_State;
	saved.m_Value = value;
	m_States.push_back(saved);
}


// 0x10022b60
void CRenderStateList::SaveStageState(StageState *pState, uint32 iStage)
{
	uint32 value;
	StageState saved;

	g_pD3DDevice->GetTextureStageState(iStage, pState->m_State, (unsigned long*)&value);

	saved.m_Stage = iStage;
	saved.m_State = pState->m_State;
	saved.m_Value = value;
	m_StageStates.push_back(saved);
}


// 0x10022ba9
void CRenderStateList::RestoreStates()
{
	std::vector<RenderState>::iterator it, itEnd;

	if (!g_pD3DDevice)
		return;

	itEnd = m_States.end();
	for (it = m_States.begin(); it != itEnd; it++)
		g_pD3DDevice->SetRenderState(it->m_State, it->m_Value);

	m_States.clear();
}


// 0x10022be9
void CRenderStateList::RestoreStageStates()
{
	std::vector<StageState>::iterator it, itEnd;

	if (!g_pD3DDevice)
		return;

	itEnd = m_StageStates.end();
	for (it = m_StageStates.begin(); it != itEnd; it++)
		g_pD3DDevice->SetTextureStageState(it->m_Stage, it->m_State, it->m_Value);

	m_StageStates.clear();
}
