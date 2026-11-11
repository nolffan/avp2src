// *********************************************************************** //
//
//  MODULE:   D3D_LOADGAMMA (.CPP) -- Primal Hunt's d3d.ren, between TU17
//            and TU18 (0x1001ca30..0x1001cb13; AvP2 1.0.9.6's 0x1001be20..)
//
//  PURPOSE:  The "in a 3D frame" query and the gamma ramp loader
//            d3d_init.cpp's device bring-up and LOADGAMMA command call.
//
//  A unit of its own, /O2 (16-aligned, `add esp` clean-ups), between
//  d3d_init.cpp and d3d_optimizedsurface.cpp's /O1 run.  The evidence:
//    * d3d_init.cpp is /Ob2 (its console variables' initialisers are
//      single functions), and /Ob2 expands the loader into d3d_InitDevice
//      and d3d_RenderCommand when it is defined in that file; retail
//      calls it from both (0x1001c2bb, 0x1001c928).
//    * The loader is not its unit's first function: compiled first it
//      shares one epilogue and one QueryInterface call (166 bytes), and
//      behind any other function it is retail's 211 bytes.  d3d_IsIn3D
//      is the function ahead of it in the image, so it is here; whether
//      more of d3d_init.cpp's tail (d3d_GetInfoFlags, d3d_Start3D,
//      d3d_End3D, 0x1001c980..) belongs to this unit the bytes do not
//      say -- they are the same on either side.
//
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <math.h>
#include <stdio.h>
#include "ltbasedefs.h"

extern LPDIRECTDRAWSURFACE7 g_pPrimarySurface;              // 0x1005d2b0 (common_init.cpp)
extern LTBOOL               g_bRunWindowed;                 // 0x1005d538 (common_init.cpp)
extern IDirectDrawSurface7* g_pFrontBuffer;                 // 0x1005d2ac (common_init.cpp)
extern LTBOOL               g_bIn3D;                        // 0x100638d8 (d3d_init.cpp)

// 0x1001ca30
LTBOOL d3d_IsIn3D()
{
	return g_bIn3D;
}

// 0x1001ca40 -- LOADGAMMA: set the primary surface's gamma ramp from a
// file (the three 256-entry tables of a DDGAMMARAMP, as written).
LTBOOL d3d_LoadGammaRamp(const char* pFilename)
{
	DDGAMMARAMP ramp;
	LPDIRECTDRAWGAMMACONTROL pGamma;
	FILE* fp;

	if (!pFilename)
		return LTFALSE;

	fp = fopen(pFilename, "r");
	if (!fp)
		return LTFALSE;

	if (fread(&ramp, sizeof(ramp), 1, fp) != 1)
	{
		fclose(fp);
		return LTFALSE;
	}

	fclose(fp);

	if (g_bRunWindowed)
	{
		if (g_pFrontBuffer->QueryInterface(IID_IDirectDrawGammaControl, (void**)&pGamma) != DD_OK)
			return LTFALSE;
	}
	else
	{
		if (g_pPrimarySurface->QueryInterface(IID_IDirectDrawGammaControl, (void**)&pGamma) != DD_OK)
			return LTFALSE;
	}

	return pGamma->SetGammaRamp(0, &ramp) == DD_OK;
}
