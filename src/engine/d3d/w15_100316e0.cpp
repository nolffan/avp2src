// *********************************************************************** //
//
//  MODULE:   W15_100316E0 (.CPP)  -- /O2
//
//  PURPOSE:  0x100316e0, push the fog, dither and texture-filter console
//            variables into the D3D device.  0x10021e50 (w15_10021e50_os.cpp)
//            calls it every frame after re-reading the variables.
//
//  The fog block is skipped while nothing has changed since the last call
//  (the copies at 0x1008ca74..0x1008ca8c, near/far compared to within
//  0.001), the dither state likewise (0x1008ca48/4c); the filter states
//  are set every time.  Fog is forced off when the device has no table fog
//  (raster caps bit 0x100 at 0x1008d920) or the range is empty.  The fog
//  colour is kept as floats 0..1 (0x1008caf4..fc) and as 255 x (2c - 1)
//  (0x1008cae0..e8).
//
//  Render states: 0x23 FOGTABLEMODE (LINEAR when TableFog is set),
//  0x22 FOGCOLOR, 0x24/0x25 FOGTABLESTART/END (the float bits), 0x1c
//  FOGENABLE, 0x1a DITHERENABLE.  Stage states on stages 0 and 1: 0x11
//  MINFILTER / 0x10 MAGFILTER (point or linear from Bilinear, 3 and 0x15
//  MAXANISOTROPY when Anisotropic is set), 0x12 MIPFILTER (Trilinear),
//  0x13 MIPMAPLODBIAS (MipMapBias' float bits).
//
//  /O2 (lea-nop padding).  The colour bytes are C casts -- the shipped
//  code has the twelve-instruction truncation, not /Qrcd.
//
// *********************************************************************** //

#include <math.h>
#include "ltbasedefs.h"

// IDirect3DDevice7 as the C++ interface d3d.h declares it: the device
// pointer is loaded ahead of the arguments of each call (`this` first),
// which a C vtable struct does not reproduce.
struct D3DDevice7
{
	virtual void __stdcall v00();
	virtual void __stdcall v04();
	virtual void __stdcall v08();
	virtual void __stdcall v0c();
	virtual void __stdcall v10();
	virtual void __stdcall v14();
	virtual void __stdcall v18();
	virtual void __stdcall v1c();
	virtual void __stdcall v20();
	virtual void __stdcall v24();
	virtual void __stdcall v28();
	virtual void __stdcall v2c();
	virtual void __stdcall v30();
	virtual void __stdcall v34();
	virtual void __stdcall v38();
	virtual void __stdcall v3c();
	virtual void __stdcall v40();
	virtual void __stdcall v44();
	virtual void __stdcall v48();
	virtual void __stdcall v4c();
	virtual long __stdcall SetRenderState(uint32 dwState, uint32 dwValue);             // +0x50
	virtual void __stdcall v54();
	virtual void __stdcall v58();
	virtual void __stdcall v5c();
	virtual void __stdcall v60();
	virtual void __stdcall v64();
	virtual void __stdcall v68();
	virtual void __stdcall v6c();
	virtual void __stdcall v70();
	virtual void __stdcall v74();
	virtual void __stdcall v78();
	virtual void __stdcall v7c();
	virtual void __stdcall v80();
	virtual void __stdcall v84();
	virtual void __stdcall v88();
	virtual void __stdcall v8c();
	virtual void __stdcall v90();
	virtual long __stdcall SetTextureStageState(uint32 dwStage, uint32 dwState,
	                                          uint32 dwValue);             // +0x94
};

// A console-variable record (big21f90.cpp's CD3DConVar): the value as an
// int at +0x00 and as a float at +0x04.
struct CVarR5C
{
	int32   m_nValue;                                   // +0x00
	float   m_fValue;                                   // +0x04
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988
extern uint32      g_1008d920;                          // 0x1008d920  raster caps
extern int32       g_CV_FogEnable;                      // 0x1008cb0c  FogEnable
extern float       g_CV_FogNearZ;                       // 0x1008cb08  FogNearZ
extern float       g_100893c8;                          // 0x100893c8  FogFarZ
extern int32       g_CV_FogColor[3];                    // 0x100893bc  FogR, FogG, FogB
extern uint32      g_1008ca54;                          // 0x1008ca54  TableFog
extern int32       g_CV_Dither;                         // 0x10089404  Dither
extern int32       g_CV_Bilinear;                       // 0x1008ca98  Bilinear
extern CVarR5C     g_1008d740;                          // 0x1008d740  Anisotropic
extern CVarR5C     g_1008d760;                          // 0x1008d760  Trilinear
extern CVarR5C     g_1008d780;                          // 0x1008d780  MipMapBias

// The fog colour, as floats 0..1 and as 255 x (2c - 1).
extern float       g_f1008caf4;                         // 0x1008caf4
extern float       g_f1008caf8;                         // 0x1008caf8
extern float       g_f1008cafc;                         // 0x1008cafc
extern float       g_1008cae0;                          // 0x1008cae0
extern float       g_1008cae4;                          // 0x1008cae4
extern float       g_1008cae8;                          // 0x1008cae8

// What was last sent.
extern uint32      g_1008ca8c;                          // 0x1008ca8c  fog state valid
extern int32       g_1008ca88;                          // 0x1008ca88  FogEnable
extern int32       g_1008ca7c;                          // 0x1008ca7c  FogR
extern int32       g_1008ca80;                          // 0x1008ca80  FogG
extern int32       g_1008ca84;                          // 0x1008ca84  FogB
extern float       g_1008ca78;                          // 0x1008ca78  FogNearZ
extern float       g_1008ca74;                          // 0x1008ca74  FogFarZ
extern uint32      g_1008ca50;                          // 0x1008ca50  TableFog
extern uint32      g_1008ca4c;                          // 0x1008ca4c  dither state valid
extern int32       g_1008ca48;                          // 0x1008ca48  Dither
extern uint32      g_1008ca44;                          // 0x1008ca44  filter state valid
extern int32       g_1008ca40;                          // 0x1008ca40  Bilinear

#define D3DRENDERSTATE_DITHERENABLE      26
#define D3DRENDERSTATE_FOGENABLE         28
#define D3DRENDERSTATE_FOGCOLOR          34
#define D3DRENDERSTATE_FOGTABLEMODE      35
#define D3DRENDERSTATE_FOGTABLESTART     36
#define D3DRENDERSTATE_FOGTABLEEND       37
#define D3DFOG_NONE                      0
#define D3DFOG_LINEAR                    3
#define D3DTSS_MAGFILTER                 16
#define D3DTSS_MINFILTER                 17
#define D3DTSS_MIPFILTER                 18
#define D3DTSS_MIPMAPLODBIAS             19
#define D3DTSS_MAXANISOTROPY             21
#define D3DPRASTERCAPS_FOGTABLE          0x100

#define RGBA_MAKE(r, g, b, a) \
	((uint32)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

#define SETRS(state, value) \
	g_pD3DDevice->SetRenderState((state), (value))
#define SETTSS(stage, state, value) \
	g_pD3DDevice->SetTextureStageState((stage), (state), (value))


// ======================================================================= //
//  0x100316e0 -- 1827 bytes.
// ======================================================================= //
void sub_100316e0()
{
	if (!g_pD3DDevice)
		return;

	if (!(g_1008d920 & D3DPRASTERCAPS_FOGTABLE))
		g_CV_FogEnable = 0;

	if (g_CV_FogNearZ == g_100893c8)
		g_CV_FogEnable = 0;

	if (!g_1008ca8c ||
	    g_1008ca88 != g_CV_FogEnable ||
	    g_1008ca7c != g_CV_FogColor[0] ||
	    g_1008ca80 != g_CV_FogColor[1] ||
	    g_1008ca84 != g_CV_FogColor[2] ||
	    fabs(g_1008ca78 - g_CV_FogNearZ) > 0.001f ||
	    fabs(g_1008ca74 - g_100893c8) > 0.001f ||
	    g_1008ca50 != g_1008ca54)
	{
		g_f1008caf4 = g_CV_FogColor[0] * (1.0f / 255.0f);
		g_f1008caf8 = g_CV_FogColor[1] * (1.0f / 255.0f);
		g_f1008cafc = g_CV_FogColor[2] * (1.0f / 255.0f);

		if (g_1008ca54)
			SETRS(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
		else
			SETRS(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);

		g_1008ca50 = g_1008ca54;

		SETRS(D3DRENDERSTATE_FOGCOLOR,
		      RGBA_MAKE((int32)(g_f1008caf4 * 255.0f),
		                (int32)(g_f1008caf8 * 255.0f),
		                (int32)(g_f1008cafc * 255.0f), 0xff));
		SETRS(D3DRENDERSTATE_FOGTABLESTART, *(uint32*)&g_CV_FogNearZ);
		SETRS(D3DRENDERSTATE_FOGTABLEEND, *(uint32*)&g_100893c8);
		SETRS(D3DRENDERSTATE_FOGENABLE, g_CV_FogEnable);

		g_1008cae0 = (g_f1008caf4 - 0.5f) * 2.0f * 255.0f;
		g_1008cae4 = (g_f1008caf8 - 0.5f) * 2.0f * 255.0f;
		g_1008cae8 = (g_f1008cafc - 0.5f) * 2.0f * 255.0f;

		g_1008ca88 = g_CV_FogEnable;
		g_1008ca7c = g_CV_FogColor[0];
		g_1008ca80 = g_CV_FogColor[1];
		g_1008ca84 = g_CV_FogColor[2];
		g_1008ca78 = g_CV_FogNearZ;
		g_1008ca74 = g_100893c8;
		g_1008ca8c = 1;
	}

	if (!g_1008ca4c || g_1008ca48 != g_CV_Dither)
	{
		if (g_pD3DDevice)
		{
			SETRS(D3DRENDERSTATE_DITHERENABLE, g_CV_Dither);
			g_1008ca4c = 1;
			g_1008ca48 = g_CV_Dither;
		}
	}

	if (g_pD3DDevice)
	{
		if (!g_1008d740.m_nValue)
		{
			SETTSS(0, D3DTSS_MINFILTER, g_CV_Bilinear ? 2 : 1);
			SETTSS(0, D3DTSS_MAGFILTER, g_CV_Bilinear ? 2 : 1);
			SETTSS(1, D3DTSS_MINFILTER, g_CV_Bilinear ? 2 : 1);
			SETTSS(1, D3DTSS_MAGFILTER, g_CV_Bilinear ? 2 : 1);
		}
		else
		{
			SETTSS(0, D3DTSS_MINFILTER, 3);
			SETTSS(0, D3DTSS_MAGFILTER, 3);
			SETTSS(0, D3DTSS_MAXANISOTROPY, g_1008d740.m_nValue);
			SETTSS(1, D3DTSS_MINFILTER, 3);
			SETTSS(1, D3DTSS_MAGFILTER, 3);
			SETTSS(1, D3DTSS_MAXANISOTROPY, g_1008d740.m_nValue);
		}

		SETTSS(0, D3DTSS_MIPFILTER, g_1008d760.m_nValue ? 3 : 2);
		SETTSS(0, D3DTSS_MIPMAPLODBIAS, *(uint32*)&g_1008d780.m_fValue);
		SETTSS(1, D3DTSS_MIPFILTER, g_1008d760.m_nValue ? 3 : 2);
		SETTSS(1, D3DTSS_MIPMAPLODBIAS, *(uint32*)&g_1008d780.m_fValue);

		g_1008ca44 = 1;
		g_1008ca40 = g_CV_Bilinear;
	}
}
