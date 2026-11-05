// *********************************************************************** //
//  ph/d3d_portal.cpp -- Primal Hunt's d3d.ren, TU32 (0x1002ddc0..0x1002de3c),
//  /O1.
//
//  All that survives of this object is its four console variables: the
//  link's /OPT:REF removed every function, so only the dynamic
//  initialisers (each a 5-byte jmp stub and a 26-byte body, the /Os shape)
//  are left, at 0x1002ddc0, 0x1002dddf, 0x1002ddfe and 0x1002de1d.
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

CD3DConVar g_CV_PortalFlow("PortalFlow", 0.0f, LTNULL, LTNULL);		// 0x1002ddc5
CD3DConVar g_CV_PortalGraph("PortalGraph", 0.0f, LTNULL, LTNULL);	// 0x1002dde4
CD3DConVar g_CV_PortalsOnly("PortalsOnly", 0.0f, LTNULL, LTNULL);	// 0x1002de03
CD3DConVar g_CV_BlockersOnly("BlockersOnly", 0.0f, LTNULL, LTNULL);	// 0x1002de22
