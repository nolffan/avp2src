// *********************************************************************** //
//
//  MODULE:   W15_100235D0 (.CPP)  -- /O2
//
//  PURPOSE:  0x100235d0, the volumetric (height-layer) fog routine that
//            0x1002b0c0 (d3dr3d.cpp) installs in the function pointer
//            0x1008ce8c (`movl $0x100235d0,0x1008ce8c` at 0x1002b179).
//            __fastcall like its siblings 0x100234d0/e0 and 0x10023570
//            (w4_100234d0.cpp): the position arrives in %ecx, the output
//            colour in %edx, and the fog factor goes to the alpha byte.
//
//  The fog is a horizontal layer between g_f1008d708 (bottom) and
//  g_f1008d6e8 (top) whose density runs linearly from g_f1008d6c8 to
//  g_f1008d6a8 across it and is constant outside.  The point is classed
//  1 above the layer, 0 below it, 2 inside -- the classes 0x1001e360
//  (big66f08.cpp) gives the eye, whose class and density are 0x1008bd74 /
//  0x1008bd70.  Same class: one trapezoid, (d(point) + d(eye)) x distance.
//  Eye above: the ray is split where it crosses the top plane, the upper
//  part at the constant top density.  Otherwise the ray is split at the
//  top plane the other way round.  The sum is scaled by g_1008d5d8,
//  clamped to [0, g_1008d688] and stored as 255 - fog in the alpha byte.
//
//  NEAR.  What is settled, each measured:
//    * sqrtf, not (float)sqrt(): a double sqrt makes icl give the function
//      its 16-byte stack-realignment prologue (the `push %ebx ; mov
//      %esp,%ebx ; and $-16,%esp` shim of 0x10023930), which the shipped
//      function does not have; /Qsfalign16 would remove it too, sqrtf is
//      the spelling that needs no flag.  VC6's C++ math.h makes sqrtf an
//      inline over sqrt, but icl takes the name as its float intrinsic.
//    * the byte conversion is RoundF (`fstps ; flds ; fistpl`, see
//      w1_10001000.cpp), which also gives the %ebp frame;
//    * the eye/point locals loaded up front (they sit on the x87 stack for
//      the whole function, so the eye x spills straight away, as shipped);
//    * the class computed as a value and compared (`cmp %eax,%edx ; je`),
//      then `nMode != 1`, gives the shipped dispatch and block order.
//  What is left is x87 scheduling inside the three arms (which operand
//  is loaded first, where each spill lands).
//
// *********************************************************************** //

#include <math.h>
#include "ltbasedefs.h"

struct LTVec { float x, y, z; };

extern LTVec  g_v1008bd34;                              // 0x1008bd34  eye point
extern float  g_1008bd70;                               // 0x1008bd70  fog density at the eye
extern int32  g_1008bd74;                               // 0x1008bd74  the eye's layer region
extern float  g_f1008d708;                              // 0x1008d708  layer bottom
extern float  g_f1008d6e8;                              // 0x1008d6e8  layer top
extern float  g_f1008d6c8;                              // 0x1008d6c8  density at/below the bottom
extern float  g_f1008d6a8;                              // 0x1008d6a8  density at/above the top
extern float  g_1008d5d8;                               // 0x1008d5d8  distance scale
extern float  g_f1008d5dc;                              // 0x1008d5dc
extern float  g_f1008d5e0;                              // 0x1008d5e0
extern float  g_1008d688;                               // 0x1008d688  maximum fog

// Round to nearest in the current FPU mode (0x10015190 out of line).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}


// ======================================================================= //
//  0x100235d0 -- 860 bytes.
// ======================================================================= //
void __fastcall sub_100235d0(const LTVec* pPos, uint8* pOut)
{
	float  px, py, pz, ex, ey, ez, fBottom, fTop;
	float  t, ix, iz, fDensity, fTopDensity, fFog;
	int32  nRegion, nMode;

	nMode = g_1008bd74;
	px = pPos->x;
	py = pPos->y;
	pz = pPos->z;
	ex = g_v1008bd34.x;
	ey = g_v1008bd34.y;
	ez = g_v1008bd34.z;
	fTop    = g_f1008d6e8;
	fBottom = g_f1008d708;

	if (py >= fTop)
		nRegion = 1;
	else if (fBottom >= py)
		nRegion = 0;
	else
		nRegion = 2;

	if (nRegion != nMode)
	{
		if (nMode != 1)
			{
				t  = (fTop - ey) / (py - ey);
				ix = (px - ex) * t + ex;
				iz = (pz - ez) * t + ez;
				fTopDensity = g_f1008d6a8;

				if (ey <= fBottom)
					fDensity = g_f1008d6c8;
				else if (ey >= fTop)
					fDensity = g_f1008d6a8;
				else
					fDensity = (ey - fBottom) * g_f1008d5dc * g_f1008d5e0 + g_f1008d6c8;

				fFog = sqrtf((ex - ix) * (ex - ix) + (ey - fTop) * (ey - fTop) + (ez - iz) * (ez - iz)) *
				       (fDensity + fTopDensity) * g_1008d5d8 +
				       fTopDensity * 2.0f *
				       sqrtf((ix - px) * (ix - px) + (iz - pz) * (iz - pz) + (fTop - py) * (fTop - py)) *
				       g_1008d5d8;
			}
		else
			{
				t  = (fTop - ey) / (py - ey);
				ix = (px - ex) * t + ex;
				iz = t * (pz - ez) + ez;
				fTopDensity = g_f1008d6a8;

				if (py <= fBottom)
					fDensity = g_f1008d6c8;
				else if (py >= fTop)
					fDensity = g_f1008d6a8;
				else
					fDensity = (py - fBottom) * g_f1008d5dc * g_f1008d5e0 + g_f1008d6c8;

				fFog = sqrtf((ex - ix) * (ex - ix) + (ez - iz) * (ez - iz) + (ey - fTop) * (ey - fTop)) *
				       (2.0f * g_f1008d6a8) * g_1008d5d8 +
				       (fDensity + fTopDensity) *
				       sqrtf((ix - px) * (ix - px) + (iz - pz) * (iz - pz) + (fTop - py) * (fTop - py)) *
				       g_1008d5d8;
			}
	}
	else
		{
			if (fBottom >= py)
				fDensity = g_f1008d6c8;
			else if (fTop <= py)
				fDensity = g_f1008d6a8;
			else
				fDensity = (py - fBottom) * g_f1008d5dc * g_f1008d5e0 + g_f1008d6c8;

			fFog = (fDensity + g_1008bd70) *
			       sqrtf((ex - px) * (ex - px) + (ey - py) * (ey - py) + (ez - pz) * (ez - pz)) *
			       g_1008d5d8;
		}
	if (fFog > g_1008d688)
		fFog = g_1008d688;
	else if (fFog < 0.0f)
		fFog = 0.0f;

	pOut[3] = 255 - (uint8)RoundF(fFog);
}
