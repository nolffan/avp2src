// *********************************************************************** //
//
//  w32_10015d60_osoi.cpp -- 0x10015d60, the 32-entry wave table.
//  /Os /Oi- (D3D_OS_OI_TUS by name).
//
// *********************************************************************** //

#include "ltbasedefs.h"

// ----------------------------------------------------------------------- //
//  0x10015d60 -- 180 bytes including the alternate entry, 1 caller
//  (0x1002d971).  BUILDS A 32-ENTRY WAVE TABLE AND ITS FORWARD DIFFERENCES.
//
//  WHAT PINS IT: `fstpl (%esp) ; call 0x1007fdd0`, and 0x1007fdd0 is
//  `fldl 0x4(%esp) ; fsin` -- the CRT's sin.  A double argument is why the
//  prologue realigns the stack (`and $0xfffffff0,%esp`) and why icl emits
//  the second, non-realigning entry at 0x10015d6e.
//
//  The constants decode without any ambiguity: 0.03125 = 1/32 and
//  6.2831855 = 2*PI, so the angle is (i/32) * 2PI over 32 samples.  The
//  clamp constants come in pairs -- 0.0 at .data1+0xa70 for the test and a
//  SECOND 0.0 at +0xa74 for the assignment, 1.0 at +0xa80 for the test and
//  a SECOND 1.0 at +0xa84 for the assignment.  icl does not pool duplicate
//  literals here, so those four are four literals in the source.
//
//  Both the load of the global and the clamp are INSIDE the loop: the call
//  to sin clobbers the compiler's knowledge of 0x1008cb7c, and the shipped
//  code recomputes fMin and fAmp all thirty-two times.
//
//  Association: `fSin * fAmp + fAmp + fMin` -- `fmul %st,%st(2)` then two
//  `faddp`s -- is ((fSin*fAmp) + fAmp) + fMin, left to right.  With
//  fMin in [0,1] that runs the table from fMin up to 1.0.
//
//  The second loop's `mov $0x1f,%ecx ; and %edx,%ecx` is `& 31` on the
//  incremented index, not a modulo: a signed `% 32` would need the sign
//  correction and there is none.  `fsubrs` (FSUBR m32) makes the difference
//  table[next] - table[i], not the other way round.
//
//  The store `fstps 0x1008b7d0(,%ebp,4)` after `inc %ebp` is the SAME slot
//  as g_1008b7d4[i] -- icl folded the -4 into the displacement.
//
//  Moved out of d3dr3a.cpp (/O2, 94 bytes differed) to /Os /Oi-.  The
//  shipped loop heads are not aligned (/Os), sin is CALLED (/Oi-), and
//  three source edits were each measured: the angle goes through a float
//  local (else icl folds 1/32 * 2PI into one fmuls), the clamp is the
//  SDK's LTCLAMP (the if/else chain dups the global up front), and the
//  table adds the UNCLAMPED global -- the shipped `faddp` chain ends on
//  the g_1008cb7c load, fMin feeds only the amplitude.
//
//  (Superseded coin-flip, kept for the record:)  The result of sin is used STRAIGHT off
//  the x87 stack -- `fmul %st,%st(2)` -- with no narrowing store anywhere
//  between the call and the final `fstps` into the table.  It is written
//  `float fSin = (float)sin(...)`, which is what the source most likely
//  said, on the evidence from the clippers below that icl 4.0 keeps float
//  locals at extended precision and narrows ONLY when a value feeds an
//  integer conversion (fDenom and fT there never touch memory; the five
//  colour floats do, and only because of the `(uint8)` cast).  IF THIS ONE
//  MISSES with an `fstps <slot> ; flds <slot>` pair sitting immediately
//  after `call 0x1007fdd0`, the answer is that icl DOES narrow on
//  assignment: change `float fSin` to `double fSin` and drop the `(float)`
//  cast.  Nothing else in the function changes.
// ----------------------------------------------------------------------- //


#define MATH_A_TWOPI    6.283185307f

extern float g_1008cb7c;                                // 0x1008cb7c
extern float g_1008b754[32];                            // 0x1008b754
extern float g_1008b7d4[32];                            // 0x1008b7d4

// sin comes from <math.h>, which ltbasetypes.h already pulls in.

void sub_10015d60()
{
	int32	i;
	float	fSin;
	float	fAngle;
	float	fMin;
	float	fAmp;

	for (i = 0; i < 32; i++)
	{
		fAngle = (float)i / 32.0f;
		fSin = (float)sin(fAngle * MATH_A_TWOPI);

		fMin = LTCLAMP(g_1008cb7c, 0.0f, 1.0f);

		fAmp = (1.0f - fMin) * 0.5f;

		g_1008b7d4[i] = fSin * fAmp + fAmp + g_1008cb7c;
	}

	for (i = 0; i < 32; i++)
		g_1008b754[i] = g_1008b7d4[(i + 1) & 31] - g_1008b7d4[i];
}
