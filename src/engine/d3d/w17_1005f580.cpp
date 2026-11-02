// *********************************************************************** //
//
//  w17_1005f580.cpp -- 0x1005f580, the sky pass driver.  /O2: multi-byte
//  nop padding after it, an %esp frame with the outgoing arguments
//  preallocated (`sub $0x94,%esp`, no pushes).
//
//  Called with no arguments from d3dqip4.cpp's 0x1007b580 after the world
//  walk.  DrawSky on, sky objects present (the scene's +0xa0) and
//  0x1005dca0 (big5dca0.cpp: the sky portals' screen extents into
//  0x1008ff30..0x1008ff3c) finding some: build the sky lens for that
//  rectangle from the main transform with a 0.01 near plane and the
//  scene's far plane (0x10020d80), set the sky transform 0x1008ff60 up
//  from it with the camera matrix moved to the sky view position
//  (0x1008bd40) and SkyScale on all three axes (0x1001ef30), then draw
//  the sky (0x10030a60).
//
//  NEAR (measured): instruction-for-instruction the shipped
//  function; only the first call's argument block differs in registers --
//  shipped loads the scene pointer and the four extents through %edx and
//  takes &lens in %ecx, never touching %eax there; ours uses %eax/%edx.
//  Unchanged by: a pScene local (at the top or at the call), nested ifs /
//  one `||` guard (both worse), the far plane as uint32 or a local, the
//  near plane as (float)0.01 or a local, the locals' order, an LTVec
//  scale local, a struct-returning 0x10020d80 (it does return pOut in
//  %eax; worse), an unrelated function compiled before it, and this
//  function appended to big5dca0.cpp beside 0x1005dca0.
//  Also unchanged (same 423 bytes, %eax/%edx): the
//  0x1005dca0 result in a local, `== 0`, every early-out as one nested
//  `if`, 0x1005dca0 declared int / bool (bool worse), &lens through a
//  local or a reference parameter, the far/near planes as locals, the
//  four extents as one global struct, 0x10020d80 returning pOut (as a
//  pointer: same; its result passed on to 0x1001ef30: worse).
// *********************************************************************** //

#include "ltbasedefs.h"

struct LTVec
{
	LTVec() {}
	LTVec(float mx, float my, float mz) { x = mx; y = my; z = mz; }
	float x, y, z;
};

struct LTMat
{
	float m[4][4];
};

struct D3DLens
{
	float	m_fOffX;                                    // +0x00
	float	m_fOffY;                                    // +0x04
	float	m_fFocal;                                   // +0x08
	float	m_fHalfW;                                   // +0x0c
	float	m_fHalfH;                                   // +0x10
	float	m_fNear;                                    // +0x14
	float	m_fFar;                                     // +0x18
};

struct D3DXForm;

void  sub_10020d80(D3DLens* pOut, float fNear, float fFar, D3DXForm* pXForm,
                   float fMinX, float fMinY, float fMaxX, float fMaxY);
int32 sub_1001ef30(D3DXForm* pXForm, D3DLens* pLens,
                   float fMinX, float fMinY, float fMaxX, float fMaxY,
                   LTMat* pMatCam, LTVec vScale);
LTBOOL sub_1005dca0(void);                              // 0x1005dca0
void  sub_10030a60();                                   // 0x10030a60

struct Scene1005f580
{
	uint8		m_pad_00[0xa0];
	int32		m_nSkyObjs_a0;                          // +0xa0
	uint8		m_pad_a4[0xbc - 0xa4];
	float		m_fFar_bc;                              // +0xbc
};

extern long				g_CV_DrawSky;                   // 0x100893cc
extern float			g_CV_SkyScale;                  // 0x100893ac
extern Scene1005f580*	g_1008beb0;                     // 0x1008beb0
extern D3DXForm			g_1008b880;                     // 0x1008b880 main transform
extern LTMat			g_Mat1008b91c;                  // 0x1008b91c its camera matrix
extern LTVec			g_v1008bd40;                    // 0x1008bd40 sky view position
extern D3DXForm			g_1008ff60;                     // 0x1008ff60 sky transform
extern float			g_f1008ff30;                    // 0x1008ff30 sky extents
extern float			g_f1008ff34;                    // 0x1008ff34
extern float			g_f1008ff38;                    // 0x1008ff38
extern float			g_f1008ff3c;                    // 0x1008ff3c

void sub_1005f580()
{
	D3DLens	lens;
	LTMat	mat;

	if (!g_CV_DrawSky)
		return;

	if (g_1008beb0->m_nSkyObjs_a0 <= 0)
		return;

	if (!sub_1005dca0())
		return;

	sub_10020d80(&lens, 0.01f, g_1008beb0->m_fFar_bc, &g_1008b880,
	             g_f1008ff3c, g_f1008ff38, g_f1008ff34, g_f1008ff30);

	mat = g_Mat1008b91c;
	mat.m[0][3] = g_v1008bd40.x;
	mat.m[1][3] = g_v1008bd40.y;
	mat.m[2][3] = g_v1008bd40.z;

	sub_1001ef30(&g_1008ff60, &lens, g_f1008ff3c, g_f1008ff38, g_f1008ff34,
	             g_f1008ff30, &mat,
	             LTVec(g_CV_SkyScale, g_CV_SkyScale, g_CV_SkyScale));

	sub_10030a60();
}
