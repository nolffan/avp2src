// ----------------------------------------------------------------------- //
//
// MODULE  : m1_widescreen.h
//
// PURPOSE : The modern build's widescreen settings on the game side
//           (docs/MODERN.md, M-1..M-3), which the Display options
//           (FolderDisplay.cpp), the player's camera (PlayerMovement.cpp,
//           CameraMgrFX.cpp) and the interface (InterfaceMgr.cpp,
//           OverlayMGR.cpp) share.  The widening itself (Hor+) is the
//           engine's (src/modern/engine/m1_horplus.h).
//
//           FieldOfView (M-3): the player's horizontal field of view at
//           4:3, in degrees, for a human or a Predator, whose
//           CharacterButes.txt FOV is 90 (the default, retail's); every
//           character's own (an Alien's 95 by 85, an Exosuit's 75 by 65)
//           is scaled with it, both ways alike, so each keeps its shape:
//               tan(fov' / 2) = tan(fov / 2) * tan(FieldOfView / 2)
//           A wide screen then widens the horizontal one again (Hor+).
//           The zoom views keep retail's magnified field of view
//           (CameraMgrFX_ZoomUpdate), and the cinematics their own.
//
//           RestrictCinematicsTo4x3 (M-2): 1 draws black bars beside the
//           centred 4:3 area while a cinematic plays, so a wide screen
//           shows what a 4:3 one does (nolfsrc's name and meaning, its
//           M-P3-08).
//
//           Both saved in autoexec.cfg (set with "+").
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M1_WIDESCREEN_H__
#define __M1_WIDESCREEN_H__

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define M3_FOV_VAR				"FieldOfView"
#define M3_FOV_DEFAULT			90
#define M3_FOV_MIN				60
#define M3_FOV_MAX				130
#define M2_CINEMATICS_VAR		"RestrictCinematicsTo4x3"

// The menu's choices (degrees, at 4:3).
static const int g_nM3FovChoices[] = { 75, 80, 85, 90, 95, 100, 105, 110, 115, 120 };
#define M3_NUM_FOV_CHOICES		(sizeof(g_nM3FovChoices) / sizeof(g_nM3FovChoices[0]))

// A saved integer variable, made with its default the first time it is
// asked for.
inline int M1_GetIntVar(const char* pName, int nDefault)
{
	HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar((char*)pName);
	if (!hVar)
	{
		char szCmd[96];
		sprintf(szCmd, "+%s %d", pName, nDefault);
		g_pLTClient->RunConsoleString(szCmd);
		hVar = g_pLTClient->GetConsoleVar((char*)pName);
		if (!hVar)
			return nDefault;
	}

	return (int)g_pLTClient->GetVarValueFloat(hVar);
}

inline void M1_SetIntVar(const char* pName, int nValue)
{
	char szCmd[96];
	sprintf(szCmd, "+%s %d", pName, nValue);
	g_pLTClient->RunConsoleString(szCmd);
}

// FieldOfView, kept to M3_FOV_MIN..M3_FOV_MAX.
inline int M3_GetFov()
{
	int nFov = M1_GetIntVar(M3_FOV_VAR, M3_FOV_DEFAULT);
	if (nFov < M3_FOV_MIN) nFov = M3_FOV_MIN;
	if (nFov > M3_FOV_MAX) nFov = M3_FOV_MAX;
	return nFov;
}

// The factor FieldOfView puts on a field of view's tangent (1 at 90).
inline float M3_FovScale()
{
	return (float)tan(MATH_DEGREES_TO_RADIANS((float)M3_GetFov()) * 0.5f);
}

// A field of view (degrees), scaled for FieldOfView.
inline float M3_ScaleFovDegrees(float fFov, float fScale)
{
	if (fScale == 1.0f || fFov <= 0.0f || fFov >= 180.0f)
		return fFov;

	return MATH_RADIANS_TO_DEGREES(2.0f * (float)atan(tan(MATH_DEGREES_TO_RADIANS(fFov) * 0.5f) * fScale));
}

// The menu's choice for a field of view: its own, or the nearest.
inline int M3_FovChoice(int nFov)
{
	int i, nBest = 0;

	for (i = 1; i < (int)M3_NUM_FOV_CHOICES; i++)
	{
		if (abs(g_nM3FovChoices[i] - nFov) < abs(g_nM3FovChoices[nBest] - nFov))
			nBest = i;
	}

	return nBest;
}

inline LTBOOL M2_RestrictCinematics()
{
	return M1_GetIntVar(M2_CINEMATICS_VAR, 0) != 0;
}

// Black beside the centred 4:3 area of the screen (nothing on a screen
// that is not wider than 4:3).
inline void M1_FillBeside4x3(HSURFACE hScreen)
{
	uint32 nWidth, nHeight;
	g_pLTClient->GetSurfaceDims(hScreen, &nWidth, &nHeight);

	uint32 nArea = (nHeight * 4) / 3;
	if (nArea >= nWidth)
		return;

	LTRect rcFill;
	rcFill.top		= 0;
	rcFill.bottom	= (int)nHeight;
	rcFill.left		= 0;
	rcFill.right	= (int)((nWidth - nArea) / 2);
	g_pLTClient->FillRect(hScreen, &rcFill, SETRGB(0,0,0));

	rcFill.left		= rcFill.right + (int)nArea;
	rcFill.right	= (int)nWidth;
	g_pLTClient->FillRect(hScreen, &rcFill, SETRGB(0,0,0));
}

// The screen's aspect ratio over 4:3, as the engine widens by
// (m1_horplus.h).
inline float M1_ScreenWideFactor()
{
	uint32 nWidth, nHeight;
	g_pLTClient->GetSurfaceDims(g_pLTClient->GetScreenSurface(), &nWidth, &nHeight);

	if (nWidth == 0 || nHeight == 0)
		return 1.0f;

	return ((float)nWidth * 3.0f) / ((float)nHeight * 4.0f);
}

// The auto-targeting's box (AutoTargetMgr.cpp): retail takes a target
// between 30% and 70% of the screen's width and height; across a wide
// screen that is a wider angle than at 4:3, so the box is that much of the
// centred 4:3 area's width, the same angle as retail's.
inline LTBOOL M1_InTargetBox(int x, int y, uint32 nWidth, uint32 nHeight)
{
	float fArea = (float)nHeight * 4.0f / 3.0f;
	if (fArea > (float)nWidth)
		fArea = (float)nWidth;

	float fLeft = ((float)nWidth - fArea) * 0.5f;

	return x > fLeft + fArea*0.3f && x < fLeft + fArea*0.7f
		&& y > nHeight*0.3 && y < nHeight*0.7;
}

// The player's cameras' field of view (first and third person): the
// character's own (CharacterButes.txt's FOV, degrees) scaled for
// FieldOfView.  (Callers have GameClientShell.h.)
inline void M3_SetPlayerFov(CPoint ptFOV)
{
	if (ptFOV.x <= 0 || ptFOV.y <= 0)
		return;

	float fScale = M3_FovScale();
	LTFLOAT fFovX = M3_ScaleFovDegrees((LTFLOAT)ptFOV.x, fScale);
	LTFLOAT fFovY = M3_ScaleFovDegrees((LTFLOAT)ptFOV.y, fScale);

	g_pGameClientShell->GetCameraMgr()->SetCameraFOV(g_pGameClientShell->GetPlayerCamera(), fFovX, fFovY);
	g_pGameClientShell->GetCameraMgr()->SetCameraFOV(g_pGameClientShell->Get3rdPersonCamera(), fFovX, fFovY);
}

// Again, for the character the player is (FieldOfView changed).
inline void M3_ReapplyPlayerFov()
{
	if (!g_pGameClientShell || !g_pGameClientShell->GetPlayerMovement())
		return;

	const CharacterButes* pButes = g_pGameClientShell->GetPlayerMovement()->GetCharacterButes();
	if (pButes)
		M3_SetPlayerFov(pButes->m_ptFOV);
}

// A zoom's magnification over the player's field of view, made one over
// retail's (FieldOfView 90): the zoomed view is then retail's whatever
// FieldOfView.  The factor comes in over the first doubling, so the zoom
// still starts from the unzoomed view.
inline LTFLOAT M3_ZoomMagnitude(LTFLOAT fMagnitude)
{
	float fScale = M3_FovScale();
	if (fScale == 1.0f || fMagnitude <= 1.0f)
		return fMagnitude;

	float fIn = fMagnitude - 1.0f;
	if (fIn > 1.0f)
		fIn = 1.0f;

	return fMagnitude * (LTFLOAT)pow(fScale, fIn);
}

#endif // __M1_WIDESCREEN_H__
