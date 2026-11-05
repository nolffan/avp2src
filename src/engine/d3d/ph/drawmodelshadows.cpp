// *********************************************************************** //
//  ph/drawmodelshadows.cpp -- Primal Hunt's d3d.ren, TU27 (0x10025e6c..0x1002938b), /O1.
//
//  Model shadows (ModelShadowAlpha, ModelShadowOffset, ModelShadowProj).
//  Only the console variables are written so far.
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


CD3DConVar	g_CV_ModelShadowAlpha("ModelShadowAlpha", 230.0f, LTNULL, LTNULL);	// 0x1006ead8  $E 0x10025e6c/0x10025e71
CD3DConVar	g_CV_ModelShadowOffset("ModelShadowOffset", 0.05f, LTNULL, LTNULL);	// 0x1006ef28  $E 0x10025e8f/0x10025e94
CD3DConVar	g_CV_ModelShadowProj("ModelShadowProj", 0.0f, LTNULL, LTNULL);	// 0x1006eaf8  $E 0x10025eb2/0x10025eb7


// ---------------------------------------------------------------------- //
//  The rest of the object's console variables, defined further down the
//  file (their initialisers, from 0x10027b34, follow the code above).
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_ModelShadowProjRes("ModelShadowProjRes", 128.0f, LTNULL, LTNULL);	// 0x1006ff88  $E 0x10027b34/0x10027b39
CD3DConVar	g_CV_ModelShadowProjLOD("ModelShadowProjLOD", 1.0f, LTNULL, LTNULL);	// 0x1006ff68  $E 0x10027b57/0x10027b5c
CD3DConVar	g_CV_ModelShadowProjShow("ModelShadowProjShow", 0.0f, LTNULL, LTNULL);	// 0x1006ff48  $E 0x10027b76/0x10027b7b
