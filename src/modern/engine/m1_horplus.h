// ----------------------------------------------------------------------- //
//
// MODULE  : m1_horplus.h
//
// PURPOSE : The modern build's widescreen field of view (docs/MODERN.md,
//           M-1), "Hor+": the game gives every camera a field of view for
//           a 4:3 screen (the player's from CharacterButes.txt's FOV, 90 by
//           78 degrees for a human or a Predator, 95 by 85 for an Alien;
//           a cinematic camera's from its object, 75 by 60 by default), and
//           retail draws it across whatever screen there is, so a 16:9
//           screen shows the 4:3 picture stretched sideways.  Here the
//           vertical field of view is kept and the horizontal one widened
//           by the screen's aspect ratio over 4:3:
//
//               fovX' = 2 * atan(tan(fovX / 2) * (aspect / (4/3)))
//
//           At 4:3 that is fovX itself.  The factor is the SCREEN's (the
//           render mode's), not the camera rectangle's: AvP2's cinematics
//           letterbox by shrinking the camera's rectangle to three
//           quarters of the screen's height with the same field of view
//           (CameraMgrFX_WidescreenUpdate), and a 4:3 screen must keep
//           showing them as retail does.
//
//           The engine widens the field of view where it hands a camera
//           to the renderer (cm_RenderCamera) and where it turns a screen
//           point into a direction (Get3DCameraPt), so the picture and
//           everything the game measures against it (the camera overlays,
//           OverlayMGR) agree; the cameras themselves, and what the client
//           reads back from them, keep the game's 4:3 values.  The
//           renderer undoes and redoes it for the models with a field of
//           view of their own (the player-view weapons, drawmodel.cpp).
//
//           (nolfsrc's M-P0-01 does the same calculation in NOLF's
//           renderer, from the scene rectangle; the code is ours.)
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M1_HORPLUS_H__
#define __M1_HORPLUS_H__

#include <math.h>

// The screen's aspect ratio over 4:3 (1 for a 4:3 screen, or none).
inline float M1_WideFactor(unsigned long nWidth, unsigned long nHeight)
{
	if (nWidth == 0 || nHeight == 0)
		return 1.0f;

	return ((float)nWidth * 3.0f) / ((float)nHeight * 4.0f);
}

// A 4:3 horizontal field of view (radians) widened by fFactor.
inline float M1_HorPlusFovX(float fFovX, float fFactor)
{
	if (fFactor == 1.0f || fFovX <= 0.0f || fFovX >= 3.1415926f)
		return fFovX;

	return 2.0f * (float)atan(tan(fFovX * 0.5f) * fFactor);
}

#endif // __M1_HORPLUS_H__
