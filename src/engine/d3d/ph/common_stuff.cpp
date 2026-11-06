// *********************************************************************** //
//
//  MODULE:   COMMON_STUFF (.CPP) -- Primal Hunt's d3d.ren, TU12
//            (0x10011eec..0x10013c30, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The renderer's console variables (88 records, TableFog ..
//            Dither) and the shared helpers after them: the allocator
//            wrappers, the console printers and the code that creates and
//            reads the variables through the engine's RenderStruct.
//
//  Flags: /O1 /Ob2.  Every record's constructor is expanded into its
//  initialiser (the 5-byte `jmp` stub and its body), while the
//  constructor's own body, defined here below the first record, stays out
//  of line for the other objects to call: /Ob2's auto-inlining.
//
// *********************************************************************** //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ltbasedefs.h"
#include "ph_decls.h"


// The engine's RenderStruct, as far as the renderer calls into it here.
typedef void* HLTPARAM;

struct RenderStruct
{
	uint8     m_Reserved00[0x10];
	void    (*RunConsoleString)(char* pString);             // +0x10
	void    (*ConsolePrint)(char* pMsg);                    // +0x14
	HLTPARAM (*GetParameter)(char* pName);                  // +0x18
	float   (*GetParameterValueFloat)(HLTPARAM hParam);     // +0x1c
	uint8     m_Reserved20[0x34 - 0x20];
	void*   (*Alloc)(uint32 size);                          // +0x34
	void    (*Free)(void* ptr);                             // +0x38
};


// A console variable: its value is read back into the int and/or float it
// names whenever the console variables are read.
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pName;                                    // +0x14
	HLTPARAM    m_hParam;                                   // +0x18
	CD3DConVar* m_pNext;                                    // +0x1c
};


// Round to nearest in the current FPU mode (LithTech's RoundFloatToInt).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}


void d3d_ReadExtraConsoleVariables();                       // 0x1001b010 (d3d_init.cpp)


// The main globals.
RenderStruct* g_pStruct;                                    // 0x1005df08
CD3DConVar*   g_pConVars;                                   // 0x1005df8c
int32         g_1005d4a8;                                   // 0x1005d4a8 (set by common_init.cpp)


// The console variables' destinations.
// initialised (.data 0x1004a72c..0x1004a798, in its order)
float g_CV_EnvPanSpeed = 0.0005f;                // 0x1004a72c
int32 g_CV_DrawWorld = 1;                        // 0x1004a730
float g_CV_ShadowZRange = 17.0f;                 // 0x1004a734
float g_CV_SkyScale = 1.0f;                      // 0x1004a738
float g_CV_Gamma = 100.0f;                       // 0x1004a73c
int32 g_CV_UseDX6Commands = 1;                   // 0x1004a740
int32 g_CV_LightAddPoly = 1;                     // 0x1004a744
int32 g_CV_FogR = 255;                           // 0x1004a748
int32 g_CV_FogG = 255;                           // 0x1004a74c
int32 g_CV_FogB = 255;                           // 0x1004a750
float g_CV_FogFarZ = 2000.0f;                    // 0x1004a754
int32 g_CV_DrawSky = 1;                          // 0x1004a758
int32 g_CV_EnableSky = 1;                        // 0x1004a75c
int32 g_CV_TextureModels = 1;                    // 0x1004a760
int32 g_CV_DrawSprites = 1;                      // 0x1004a764
int32 g_CV_DrawPolyGrids = 1;                    // 0x1004a768
int32 g_CV_DynamicLight = 1;                     // 0x1004a76c
int32 g_CV_LightModels = 1;                      // 0x1004a770
int32 g_CV_ModelFullbrite = 1;                   // 0x1004a774
int32 g_CV_ShadowLodOffset = 100;                // 0x1004a778
int32 g_CV_MaxModelShadows = 1;                  // 0x1004a77c
float g_CV_LodScale = 1.0f;                      // 0x1004a780
int32 g_CV_DrawParticles = 1;                    // 0x1004a784
int32 g_CV_DrawModels = 1;                       // 0x1004a788
int32 g_CV_DrawLineSystems = 1;                  // 0x1004a78c
int32 g_CV_LightMap = 1;                         // 0x1004a790
int32 g_CV_Dither = 1;                           // 0x1004a794
int32 g_CV_TintModels = 1;                       // 0x1004a798

// .bss
int32 g_CV_NoLMPages;                            // 0x1005d42c
int32 g_CV_ModelProfile;                         // 0x1005da08
int32 g_CV_MaxModelLights;                       // 0x1005dad4
int32 g_CV_MaxTexAspectRatio;                    // 0x1005d4f4
int32 g_CV_32BitTextures;                        // 0x1005d8c4
int32 g_CV_32BitLightMaps;                       // 0x1005da0c
int32 g_CV_MaxTextureSize;                       // 0x1005d998
int32 g_CV_DrawGuns;                             // 0x1005d310
int32 g_CV_Saturate;                             // 0x1005d384
int32 g_CV_Bilinear;                             // 0x1005d2c4
int32 g_CV_EnvMapEnable;                         // 0x1005d678
int32 g_CV_FixTJunc;                             // 0x1005dbb4
float g_CV_LightSaturate;                        // 0x1005d8b0
int32 g_CV_FilterOptimized;                      // 0x1005dbb0
int32 g_CV_OptimizeSurfaces;                     // 0x1005d750
int32 g_CV_ShowSkySplits;                        // 0x1005dad0
float g_CV_WarbleSpeed;                          // 0x1005dea0
float g_CV_WarbleScale;                          // 0x1005d8b4
int32 g_CV_EnvMapAll;                            // 0x1005d868
float g_CV_EnvScale;                             // 0x1005d5a8
int32 g_CV_TripleBuffer;                         // 0x1005df10
int32 g_CV_InvertHack;                           // 0x1005df14
int32 g_CV_Force1Pass;                           // 0x1005df18
int32 g_CV_ModelWarble;                          // 0x1005df1c
int32 g_CV_MMXRast;                              // 0x1005df28
int32 g_CV_RGBRast;                              // 0x1005df20
int32 g_CV_RefRast;                              // 0x1005df24
int32 g_CV_TnLRast;                              // 0x1005df2c
int32 g_CV_Force2Pass;                           // 0x1005df30
int32 g_CV_FogEnable;                            // 0x1005df34
float g_CV_FogNearZ;                             // 0x1005df38
float g_CV_SkyFogNearZ;                          // 0x1005d8b8
float g_CV_SkyFogFarZ;                           // 0x1005d7d8
int32 g_CV_MipmapOffset;                         // 0x1005df44
int32 g_CV_LockPVS;                              // 0x1005df3c
int32 g_CV_ShowFullbriteModels;                  // 0x1005df40
int32 g_CV_BumpMap;                              // 0x1005df48
int32 g_CV_ShowFillInfo;                         // 0x1005df4c
int32 g_CV_DrawAll;                              // 0x1005df50
int32 g_CV_ShowSplits;                           // 0x1005df54
int32 g_CV_FastLight;                            // 0x1005df58
int32 g_CV_ShowTextureCounts;                    // 0x1005df5c
int32 g_CV_LodOffset;                            // 0x1005df60
int32 g_CV_Wireframe;                            // 0x1005df64
int32 g_CV_ModelBoxes;                           // 0x1005df68
int32 g_CV_RenderDebug;                          // 0x1005df6c
int32 g_CV_ShowPolyCounts;                       // 0x1005df70
int32 g_CV_DrawFlat;                             // 0x1005df74
int32 g_CV_LightmapsOnly;                        // 0x1005df78
int32 g_CV_GroupOffset[10];                      // 0x1005d7dc

// The render states d3d_ReadExtraConsoleVariables (d3d_init.cpp) last set.
struct FogColor
{
	uint8   r, g, b;
};

LTBOOL   g_bFogStatesSet;                        // 0x1005df80
LTBOOL   g_bDitherStateSet;                      // 0x1005df84
LTBOOL   g_bFilterStatesSet;                     // 0x1005df88
int32    g_LastFogEnable;                        // 0x1005d560
int32    g_LastFogR, g_LastFogG, g_LastFogB;     // 0x1005dadc..
float    g_LastFogNearZ;                         // 0x1005db68
float    g_LastFogFarZ;                          // 0x1005d380
int32    g_LastTableFog;                         // 0x1005dd78
int32    g_LastDither;                           // 0x1005d970
int32    g_LastBilinear;                         // 0x1005d484
FogColor g_FogColor;                             // 0x1005dad8
float    g_FogColorR, g_FogColorG, g_FogColorB;  // 0x1005d478..


// ======================================================================= //
//  The console variables, in the order of their initialisers.
// ======================================================================= //

CD3DConVar g_CV_TableFogVar("TableFog", 0.0f, LTNULL, LTNULL);

CD3DConVar::CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue)
{                                                           // 0x10011f36
	m_nValue   = 0;
	m_fValue   = 0.0f;
	m_fDefault = fDefault;
	m_pnValue  = pnValue;
	m_pfValue  = pfValue;
	m_pName    = pName;
	m_hParam   = LTNULL;

	m_pNext    = g_pConVars;
	g_pConVars = this;
}

CD3DConVar g_CV_NoLMPagesVar("NoLMPages", 0.0f, &g_CV_NoLMPages, LTNULL);
CD3DConVar g_CV_ModelProfileVar("ModelProfile", 0.0f, &g_CV_ModelProfile, LTNULL);
CD3DConVar g_CV_MaxModelLightsVar("MaxModelLights", 4.0f, &g_CV_MaxModelLights, LTNULL);
CD3DConVar g_CV_MaxTexAspectRatioVar("MaxTexAspectRatio", 8.0f, &g_CV_MaxTexAspectRatio, LTNULL);
CD3DConVar g_CV_32BitTexturesVar("32BitTextures", 0.0f, &g_CV_32BitTextures, LTNULL);
CD3DConVar g_CV_32BitLightMapsVar("32BitLightMaps", 0.0f, &g_CV_32BitLightMaps, LTNULL);
CD3DConVar g_CV_MaxTextureSizeVar("MaxTextureSize", 16384.0f, &g_CV_MaxTextureSize, LTNULL);
CD3DConVar g_CV_DrawGunsVar("DrawGuns", 1.0f, &g_CV_DrawGuns, LTNULL);
CD3DConVar g_CV_TintModelsVar("TintModels", 1.0f, &g_CV_TintModels, LTNULL);
CD3DConVar g_CV_SaturateVar("Saturate", 0.0f, &g_CV_Saturate, LTNULL);
CD3DConVar g_CV_BilinearVar("Bilinear", 1.0f, &g_CV_Bilinear, LTNULL);
CD3DConVar g_CV_EnvMapEnableVar("EnvMapEnable", 0.0f, &g_CV_EnvMapEnable, LTNULL);
CD3DConVar g_CV_FixTJuncVar("FixTJunc", 0.0f, &g_CV_FixTJunc, LTNULL);
CD3DConVar g_CV_LightSaturateVar("LightSaturate", 1.0f, LTNULL, &g_CV_LightSaturate);
CD3DConVar g_CV_FilterOptimizedVar("FilterOptimized", 0.0f, &g_CV_FilterOptimized, LTNULL);
CD3DConVar g_CV_OptimizeSurfacesVar("OptimizeSurfaces", 0.0f, &g_CV_OptimizeSurfaces, LTNULL);
CD3DConVar g_CV_ShowSkySplitsVar("ShowSkySplits", 0.0f, &g_CV_ShowSkySplits, LTNULL);
CD3DConVar g_CV_WarbleSpeedVar("WarbleSpeed", 25.0f, LTNULL, &g_CV_WarbleSpeed);
CD3DConVar g_CV_WarbleScaleVar("WarbleScale", 0.92f, LTNULL, &g_CV_WarbleScale);
CD3DConVar g_CV_EnvMapAllVar("EnvMapAll", 0.0f, &g_CV_EnvMapAll, LTNULL);
CD3DConVar g_CV_EnvPanSpeedVar("EnvPanSpeed", 0.0005f, LTNULL, &g_CV_EnvPanSpeed);
CD3DConVar g_CV_EnvScaleVar("EnvScale", 1.0f, LTNULL, &g_CV_EnvScale);
CD3DConVar g_CV_DrawWorldVar("DrawWorld", 1.0f, &g_CV_DrawWorld, LTNULL);
CD3DConVar g_CV_ShadowZRangeVar("ShadowZRange", 17.0f, LTNULL, &g_CV_ShadowZRange);
CD3DConVar g_CV_SkyScaleVar("SkyScale", 1.0f, LTNULL, &g_CV_SkyScale);
CD3DConVar g_CV_GammaVar("Gamma", 100.0f, LTNULL, &g_CV_Gamma);
CD3DConVar g_CV_GroupOffset0Var("GroupOffset0", 0.0f, &g_CV_GroupOffset[0], LTNULL);
CD3DConVar g_CV_GroupOffset1Var("GroupOffset1", 0.0f, &g_CV_GroupOffset[1], LTNULL);
CD3DConVar g_CV_GroupOffset2Var("GroupOffset2", 0.0f, &g_CV_GroupOffset[2], LTNULL);
CD3DConVar g_CV_GroupOffset3Var("GroupOffset3", 0.0f, &g_CV_GroupOffset[3], LTNULL);
CD3DConVar g_CV_GroupOffset4Var("GroupOffset4", 0.0f, &g_CV_GroupOffset[4], LTNULL);
CD3DConVar g_CV_GroupOffset5Var("GroupOffset5", 0.0f, &g_CV_GroupOffset[5], LTNULL);
CD3DConVar g_CV_GroupOffset6Var("GroupOffset6", 0.0f, &g_CV_GroupOffset[6], LTNULL);
CD3DConVar g_CV_GroupOffset7Var("GroupOffset7", 0.0f, &g_CV_GroupOffset[7], LTNULL);
CD3DConVar g_CV_GroupOffset8Var("GroupOffset8", 0.0f, &g_CV_GroupOffset[8], LTNULL);
CD3DConVar g_CV_GroupOffset9Var("GroupOffset9", 0.0f, &g_CV_GroupOffset[9], LTNULL);
CD3DConVar g_CV_TripleBufferVar("TripleBuffer", 0.0f, &g_CV_TripleBuffer, LTNULL);
CD3DConVar g_CV_UseDX6CommandsVar("UseDX6Commands", 1.0f, &g_CV_UseDX6Commands, LTNULL);
CD3DConVar g_CV_InvertHackVar("InvertHack", 0.0f, &g_CV_InvertHack, LTNULL);
CD3DConVar g_CV_Force1PassVar("Force1Pass", 0.0f, &g_CV_Force1Pass, LTNULL);
CD3DConVar g_CV_LightAddPolyVar("LightAddPoly", 1.0f, &g_CV_LightAddPoly, LTNULL);
CD3DConVar g_CV_ModelWarbleVar("ModelWarble", 0.0f, &g_CV_ModelWarble, LTNULL);
CD3DConVar g_CV_MMXRastVar("MMXRast", 0.0f, &g_CV_MMXRast, LTNULL);
CD3DConVar g_CV_RGBRastVar("RGBRast", 0.0f, &g_CV_RGBRast, LTNULL);
CD3DConVar g_CV_RefRastVar("RefRast", 0.0f, &g_CV_RefRast, LTNULL);
CD3DConVar g_CV_TnLRastVar("TnLRast", 0.0f, &g_CV_TnLRast, LTNULL);
CD3DConVar g_CV_Force2PassVar("Force2Pass", 0.0f, &g_CV_Force2Pass, LTNULL);
CD3DConVar g_CV_FogEnableVar("FogEnable", 0.0f, &g_CV_FogEnable, LTNULL);
CD3DConVar g_CV_FogRVar("FogR", 255.0f, &g_CV_FogR, LTNULL);
CD3DConVar g_CV_FogGVar("FogG", 255.0f, &g_CV_FogG, LTNULL);
CD3DConVar g_CV_FogBVar("FogB", 255.0f, &g_CV_FogB, LTNULL);
CD3DConVar g_CV_FogNearZVar("FogNearZ", 0.0f, LTNULL, &g_CV_FogNearZ);
CD3DConVar g_CV_FogFarZVar("FogFarZ", 2000.0f, LTNULL, &g_CV_FogFarZ);
CD3DConVar g_CV_SkyFogNearZVar("SkyFogNearZ", 0.0f, LTNULL, &g_CV_SkyFogNearZ);
CD3DConVar g_CV_SkyFogFarZVar("SkyFogFarZ", 2000.0f, LTNULL, &g_CV_SkyFogFarZ);
CD3DConVar g_CV_MipmapOffsetVar("MipmapOffset", 0.0f, &g_CV_MipmapOffset, LTNULL);
CD3DConVar g_CV_LockPVSVar("LockPVS", 0.0f, &g_CV_LockPVS, LTNULL);
CD3DConVar g_CV_ShowFullbriteModelsVar("ShowFullbriteModels", 0.0f, &g_CV_ShowFullbriteModels, LTNULL);
CD3DConVar g_CV_BumpMapVar("BumpMap", 0.0f, &g_CV_BumpMap, LTNULL);
CD3DConVar g_CV_DrawSkyVar("DrawSky", 1.0f, &g_CV_DrawSky, LTNULL);
CD3DConVar g_CV_EnableSkyVar("EnableSky", 1.0f, &g_CV_EnableSky, LTNULL);
CD3DConVar g_CV_TextureModelsVar("TextureModels", 1.0f, &g_CV_TextureModels, LTNULL);
CD3DConVar g_CV_ShowFillInfoVar("ShowFillInfo", 0.0f, &g_CV_ShowFillInfo, LTNULL);
CD3DConVar g_CV_DrawAllVar("DrawAll", 0.0f, &g_CV_DrawAll, LTNULL);
CD3DConVar g_CV_DrawSpritesVar("DrawSprites", 1.0f, &g_CV_DrawSprites, LTNULL);
CD3DConVar g_CV_DrawPolyGridsVar("DrawPolyGrids", 1.0f, &g_CV_DrawPolyGrids, LTNULL);
CD3DConVar g_CV_DrawParticlesVar("DrawParticles", 1.0f, &g_CV_DrawParticles, LTNULL);
CD3DConVar g_CV_DrawModelsVar("DrawModels", 1.0f, &g_CV_DrawModels, LTNULL);
CD3DConVar g_CV_DrawLineSystemsVar("DrawLineSystems", 1.0f, &g_CV_DrawLineSystems, LTNULL);
CD3DConVar g_CV_ShowSplitsVar("ShowSplits", 0.0f, &g_CV_ShowSplits, LTNULL);
CD3DConVar g_CV_DynamicLightVar("DynamicLight", 1.0f, &g_CV_DynamicLight, LTNULL);
CD3DConVar g_CV_FastLightVar("FastLight", 0.0f, &g_CV_FastLight, LTNULL);
CD3DConVar g_CV_LightModelsVar("LightModels", 1.0f, &g_CV_LightModels, LTNULL);
CD3DConVar g_CV_ModelFullbriteVar("ModelFullbrite", 1.0f, &g_CV_ModelFullbrite, LTNULL);
CD3DConVar g_CV_ShowTextureCountsVar("ShowTextureCounts", 0.0f, &g_CV_ShowTextureCounts, LTNULL);
CD3DConVar g_CV_ShadowLodOffsetVar("ShadowLodOffset", 100.0f, &g_CV_ShadowLodOffset, LTNULL);
CD3DConVar g_CV_MaxModelShadowsVar("MaxModelShadows", 1.0f, &g_CV_MaxModelShadows, LTNULL);
CD3DConVar g_CV_LodScaleVar("LodScale", 1.0f, LTNULL, &g_CV_LodScale);
CD3DConVar g_CV_LodOffsetVar("LodOffset", 0.0f, &g_CV_LodOffset, LTNULL);
CD3DConVar g_CV_WireframeVar("Wireframe", 0.0f, &g_CV_Wireframe, LTNULL);
CD3DConVar g_CV_ModelBoxesVar("ModelBoxes", 0.0f, &g_CV_ModelBoxes, LTNULL);
CD3DConVar g_CV_RenderDebugVar("RenderDebug", 0.0f, &g_CV_RenderDebug, LTNULL);
CD3DConVar g_CV_ShowPolyCountsVar("ShowPolyCounts", 0.0f, &g_CV_ShowPolyCounts, LTNULL);
CD3DConVar g_CV_LightMapVar("LightMap", 1.0f, &g_CV_LightMap, LTNULL);
CD3DConVar g_CV_DrawFlatVar("DrawFlat", 0.0f, &g_CV_DrawFlat, LTNULL);
CD3DConVar g_CV_LightmapsOnlyVar("LightmapsOnly", 0.0f, &g_CV_LightmapsOnly, LTNULL);
CD3DConVar g_CV_DitherVar("Dither", 1.0f, &g_CV_Dither, LTNULL);


// ======================================================================= //
//  The helpers.
// ======================================================================= //

// 0x10013a66 -- allocate through the engine, or the runtime before there
// is a RenderStruct.
void* dalloc(uint32 size)
{
	if (g_pStruct)
		return g_pStruct->Alloc(size);
	else
		return malloc(size);
}

// 0x10013a7f -- the same, zeroed; complains when it fails.
void* dalloc_z(uint32 size)
{
	void* ptr;

	ptr = dalloc(size);
	if (ptr)
	{
		memset(ptr, 0, size);
	}
	else
	{
		if (g_pStruct)
			g_pStruct->ConsolePrint("d3drender.dll: out of memory");
	}

	return ptr;
}

// 0x10013ac5
void dfree(void* ptr)
{
	if (g_pStruct)
		g_pStruct->Free(ptr);
	else
		free(ptr);
}

// 0x10013ade -- printf to the console.
void dd_ConsolePrint(const char* pMsg, ...)
{
	char msg[256];
	va_list marker;

	va_start(marker, pMsg);
	_vsnprintf(msg, 255, pMsg, marker);
	va_end(marker);

	g_pStruct->ConsolePrint(msg);
}

// 0x10013b13 -- the same, when RenderDebug is at least the message's level.
void AddDebugMessage(int32 debugLevel, const char* pMsg, ...)
{
	char msg[256];
	va_list marker;

	if (debugLevel <= g_CV_RenderDebug)
	{
		va_start(marker, pMsg);
		_vsnprintf(msg, 255, pMsg, marker);
		va_end(marker);

		g_pStruct->ConsolePrint(msg);
	}
}

// 0x10013b53
HLTPARAM d3d_MaybeCreateCVar(const char* pName, float defaultVal)
{
	HLTPARAM hRet;
	char str[256];

	hRet = g_pStruct->GetParameter((char*)pName);
	if (hRet)
	{
		return hRet;
	}
	else
	{
		sprintf(str, "%s %f", pName, defaultVal);
		g_pStruct->RunConsoleString(str);
		return g_pStruct->GetParameter((char*)pName);
	}
}

// 0x10013ba7
void d3d_CreateConsoleVariables()
{
	CD3DConVar* pCur;

	for (pCur = g_pConVars; pCur; pCur = pCur->m_pNext)
	{
		pCur->m_hParam = d3d_MaybeCreateCVar(pCur->m_pName, pCur->m_fDefault);
	}
}

// 0x10013bcd
void d3d_ReadConsoleVariables()
{
	CD3DConVar* pCur;

	for (pCur = g_pConVars; pCur; pCur = pCur->m_pNext)
	{
		pCur->m_nValue = RoundFloatToInt(pCur->m_fValue =
			g_pStruct->GetParameterValueFloat(pCur->m_hParam));

		if (pCur->m_pfValue)
			*pCur->m_pfValue = pCur->m_fValue;

		if (pCur->m_pnValue)
			*pCur->m_pnValue = pCur->m_nValue;
	}

	d3d_ReadExtraConsoleVariables();

	if (g_1005d4a8)
		g_CV_LightMap = 0;
}
