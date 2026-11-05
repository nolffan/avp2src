// *********************************************************************** //
//
//  ph/drawworld.cpp -- Primal Hunt's d3d.ren, TU06: 0x10009880..0x1000b2f9,
//  /O1.
//
//  World polygons: lightmaps, detail textures and the environment map
//  (the console variables DetailTextures..LMAnim).
//
// *********************************************************************** //

#include "ltbasedefs.h"


// A console variable (common_stuff.cpp's registrar, 0x10011f36).
struct CConVar
{
	CConVar(const char* pszName, float fDefault, int32* pnValue,
	        float* pfValue);                                // 0x10011f36

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pszName;                                  // +0x14
	void*       m_hVar;                                     // +0x18
	CConVar*    m_pNext;                                    // +0x1c
};


CConVar     g_CV_DetailTextures("DetailTextures", 1.0f, LTNULL, LTNULL);          // 0x10055478
CConVar     g_CV_DetailTextureAdd("DetailTextureAdd", 1.0f, LTNULL, LTNULL);      // 0x10055418
CConVar     g_CV_DetailTextureAngle("DetailTextureAngle", 0.0f, LTNULL, LTNULL);  // 0x100558e0
CConVar     g_CV_FixSparkleys("FixSparkleys", 0.0f, LTNULL, LTNULL);              // 0x10055458
CConVar     g_CV_LMFullBright("LMFullBright", 0.0f, LTNULL, LTNULL);              // 0x100554b8
CConVar     g_CV_EnvMapWorld("EnvMapWorld", 1.0f, LTNULL, LTNULL);                // 0x10055438
CConVar     g_CV_LMAnim("LMAnim", 1.0f, LTNULL, LTNULL);                          // 0x10055498

static uint32 g_nWorldFlags;                                                       // 0x10056908


// 0x10009959
void d3d_SetWorldFlags(uint32 nFlags)
{
	g_nWorldFlags = nFlags;
}

// 0x10009963
uint32 d3d_GetWorldFlags()
{
	return g_nWorldFlags;
}
