// *********************************************************************** //
//
//  MODULE:   W4_100234D0 (.CPP)  -- /O2 /Qrcd (build/engine.sh D3D_RCD_TUS)
//
//  PURPOSE:  The per-vertex fog routines 0x1002b0e0 (d3dr3d.cpp) installs
//            in the two function pointers 0x1008ce88 / 0x1008ce8c, which
//            the d3d.ren link referenced by address and nobody had
//            written.  The enumeration lists them at size 0 because
//            nothing CALLS them directly -- their only references are the
//            `movl $fn,0x1008ce8x` stores at 0x1002b0e0/0x1002b13b/
//            0x1002b179/0x1002b198 -- so they are real functions, not
//            phantoms.
//
//  __fastcall: every caller (big17c14/big0f6f0/big299a8/big3e620/big2ef34/
//  big60010/big61a30) calls through the pointers with the position in
//  %ecx and the output colour in %edx, and the bodies read %ecx/%edx
//  live-in.  d3dr3d.cpp's installer spells them `void f()`; those
//  spellings are aliased onto these (link/aliases.rsp).
//
//  Flag set: /O2 /Qrcd -- multi-byte lea-nop padding and `mov %ebx,(%esp)`
//  saves are /O2.  The shipped conversion `fstps tmp ; flds tmp ; fistpl`
//  (with the %ebp frame and saved %ebx it costs) is not a cast at all: it
//  is RoundF, the __asm `fld ; fistp` helper whose out-of-line copy is
//  0x10015190 (d3dbatchd_o2.cpp, w1_10001000.cpp), inlined.  With it both
//  0x100234e0 and 0x10023570 are byte-identical (also at
//  plain /O2 -- /Qrcd no longer matters to this file), and it rounds as
//  the shipped code does.
//
// *********************************************************************** //

#include <math.h>
#include "ltbasedefs.h"

struct LTVec { float x, y, z; };

// Round to nearest in the current FPU mode (0x10015190 out of line).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

extern LTVec g_v1008bd64;                               // 0x1008bd64  camera position
extern float g_1008cb08;                                // 0x1008cb08  fog start
extern float g_100893c8;                                // 0x100893c8  fog end
extern float g_1008caec;                                // 0x1008caec  255/(end-start)
extern float g_1008cb00;                                // 0x1008cb00  z fog start
extern float g_1008cb04;                                // 0x1008cb04  z fog end
extern float g_1008d5d4;                                // 0x1008d5d4  255/(zend-zstart)


// ======================================================================= //
//  0x100234d0 -- fog off: nothing to do.
// ======================================================================= //
void __fastcall sub_100234d0(const LTVec* pPos, uint8* pOut)
{
}


// ======================================================================= //
//  0x100234e0 -- radial fog: the distance from the camera, mapped from
//  [start, end] to a fog factor 255..0 in the specular colour's alpha
//  byte; nearer than start is the whole dword 0xffffffff, beyond end is 0.
// ======================================================================= //
void __fastcall sub_100234e0(const LTVec* pPos, uint8* pOut)
{
	float fDist;

	fDist = (float)sqrt((pPos->x - g_v1008bd64.x) * (pPos->x - g_v1008bd64.x) +
	                    (pPos->y - g_v1008bd64.y) * (pPos->y - g_v1008bd64.y) +
	                    (pPos->z - g_v1008bd64.z) * (pPos->z - g_v1008bd64.z));

	if (fDist < g_1008cb08)
	{
		*(uint32*)pOut = 0xffffffff;
	}
	else if (fDist > g_100893c8)
	{
		*(uint32*)pOut = 0;
	}
	else
	{
		pOut[3] = 255 - (uint8)RoundF((fDist - g_1008cb08) * g_1008caec);
	}
}


// ======================================================================= //
//  0x10023570 -- the same on the vertex's z, into the alpha byte only.
// ======================================================================= //
void __fastcall sub_10023570(const LTVec* pPos, uint8* pOut)
{
	float fZ = pPos->z;

	if (fZ < g_1008cb04)
	{
		pOut[3] = 0xff;
	}
	else if (fZ > g_1008cb00)
	{
		pOut[3] = 0;
	}
	else
	{
		pOut[3] = 255 - (uint8)RoundF((fZ - g_1008cb04) * g_1008d5d4);
	}
}
