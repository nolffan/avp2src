// *********************************************************************** //
//
//  w16_10042e80.cpp -- d3d.ren, written for the link.
//
//  0x10023380  the vertex LERP, out of line (304 bytes, 12 callers)
//  0x10042e80  the six-plane line clip, masked (1939 bytes, 1 caller)
//
//  0x10042e80 is called from 0x10047150 (w7_10040cc0.cpp) as
//  `sub_10042e80(v, 0x3f)`, and every one of its twelve clip arms CALLS
//  0x10023380 -- which no file defined (big44ff4/big5a960/big60010/
//  big61a30 carry it only inline), so it is written here too rather than
//  leave the link a new unresolved name.
//
//  FLAGS: /O2 /Qrcd (listed in build/engine.sh's D3D_RCD_TUS).  Both
//  functions pad /O2.  /Qrcd is for 0x10023380: its five float-to-byte
//  conversions ROUND (`fistpl` in the current mode) where a plain /O2 cast
//  truncates through the long shld/shr sequence -- measured 640 -> 298
//  bytes against the shipped 304.  0x10042e80 has no conversions and
//  compiles the same either way.
//
//  0x10023380 is IDENTICAL: the shipped conversion `fstps
//  f ; flds f ; fistpl n` is the RoundF inline-asm helper (see its note).
//
//  0x10042e80 is a NEAR at 15 bytes: everything but the spill order of
//  the LEFT plane's first endpoint (shipped spills x0 then z0, ours z0
//  then x0 -- two instructions swapped, then the same slots; the BOTTOM
//  plane, the same shape, does match).  Measured without effect: the
//  numerator's operand order, `x > -z`, `!(-z >= x)`, the arms swapped,
//  the near plane through a helper.
//  Re-measured: it is now BOTH `-z` planes, LEFT and BOTTOM (each
//  first endpoint spilled z0 then x0/y0, shipped the other way; TOP and
//  RIGHT match).  48 spellings of those two planes together -- three
//  compare forms x four numerators x four denominator groupings -- give
//  15 at best, and `v` declared then assigned changes nothing.
//  The inside tests as inline predicates (pointer or
//  reference, `-p->z < p->x`), and `0 < x + z`: all far worse (1495+).
//
// *********************************************************************** //

#include "ltbasedefs.h"

union VColor42e80
{
	uint32	m_n;
	uint8	m_b[4];
};

struct Vert42e80                                            // 0x20 bytes
{
	float		x;                                          // +0x00
	float		y;                                          // +0x04
	float		z;                                          // +0x08
	float		m_fRHW0c;                                   // +0x0c
	VColor42e80	m_Diffuse10;                                // +0x10
	VColor42e80	m_Specular14;                               // +0x14
	float		tu;                                         // +0x18
	float		tv;                                         // +0x1c
};

extern float		g_f1008b908;                            // 0x1008b908  near z
extern float		g_f1008b910;                            // 0x1008b910  far z


// ======================================================================= //
//  0x10023380 -- 304 bytes (`ret`, then a lea-nop run: /O2).  __cdecl
//  (a, b, dst, t): dst's texture coordinates and four colour bytes plus
//  the specular alpha interpolated from a to b at t.  Fields in the
//  shipped order: +0x18, +0x1c, then the bytes +0x12, +0x11, +0x10,
//  +0x13, +0x17, each `fstps temp ; flds temp ; fistpl n ; mov n,%al` --
//  the /Qrcd float-to-int (see big61a30.cpp, whose inline copy of this
//  body is the same text).
// ======================================================================= //
// IDENTICAL with the conversions through RoundF, the
// labelled fld/fistp inline-asm helper w1_10001000.cpp's LerpClipVert
// uses (the precedent): the shipped `fstps f ; flds f ; fistpl n` is the
// float parameter spilled for the asm's `fld f`, and the asm is what
// gives the function its %ebp frame.  This is LerpClipVert out of line.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

void sub_10023380(Vert42e80* a, Vert42e80* b, Vert42e80* dst, float t)
{
	dst->tu = (b->tu - a->tu) * t + a->tu;
	dst->tv = (b->tv - a->tv) * t + a->tv;

	dst->m_Diffuse10.m_b[2]  = (uint8)RoundF((b->m_Diffuse10.m_b[2]  - a->m_Diffuse10.m_b[2])  * t + a->m_Diffuse10.m_b[2]);
	dst->m_Diffuse10.m_b[1]  = (uint8)RoundF((b->m_Diffuse10.m_b[1]  - a->m_Diffuse10.m_b[1])  * t + a->m_Diffuse10.m_b[1]);
	dst->m_Diffuse10.m_b[0]  = (uint8)RoundF((b->m_Diffuse10.m_b[0]  - a->m_Diffuse10.m_b[0])  * t + a->m_Diffuse10.m_b[0]);
	dst->m_Diffuse10.m_b[3]  = (uint8)RoundF((b->m_Diffuse10.m_b[3]  - a->m_Diffuse10.m_b[3])  * t + a->m_Diffuse10.m_b[3]);
	dst->m_Specular14.m_b[3] = (uint8)RoundF((b->m_Specular14.m_b[3] - a->m_Specular14.m_b[3]) * t + a->m_Specular14.m_b[3]);
}


// ======================================================================= //
//  0x10042e80 -- 1939 bytes (`ret`, then a lea-nop run: /O2, and the six
//  sunk arms below the one `mov $1,%eax ; ret`).  Clip the line
//  v[0]..v[1] (two 32-byte TL vertices, already transformed) in place
//  against the planes selected by nMask; 0 when the line is wholly
//  outside one of them, else 1.  The planes, in the order tested, with
//  their mask bits -- the same six, same predicates and same
//  interpolation formulas as big44ff4.cpp's inline clip:
//      0x01 NEAR   z >= g_f1008b908      0x04 LEFT    -z < x
//      0x08 TOP    y < z                 0x10 RIGHT   x < z
//      0x20 BOTTOM -z < y                0x02 FAR     z <= g_f1008b910
//  Each plane: first endpoint inside -> clip the second if it is outside;
//  first endpoint outside -> reject if the second is outside too, else
//  clip the first.  The interpolation parameter is a float (a double
//  local makes icl realign the stack, `and $-16,%esp`, which shipped does
//  not); the doubles in the shipped frame are icl's own spills.
// ======================================================================= //
// The side planes' interpolation, one helper per plane shape: the free
// coordinate and z LERPed from v[0] to v[1] into dst, the plane coordinate
// set from z (x = -z, x = z, y = z or y = -z), then the rest of the vertex
// through 0x10023380.  An inline taking dst as a POINTER, and it matters:
// shipped RELOADS dst->z from memory for the z LERP (`fsubrs 0x28(%edx)`
// in a v[1] arm, `fsubs 0x8(%edx)` in a v[0] arm) while reusing the other
// endpoint's spilled double -- the store through dst->x/y kills what icl
// had cached for that element.  Written out in place (as the near and far
// planes are, below), icl CSEs dst->z too and the spill slots, store order
// and first-endpoint spill order all come out different (11 blocks -> 0
// on the four side planes).  The near/far planes the other way round:
// through a helper they lose (an extra %ebx for the plane value).
inline void Clip42e80X(Vert42e80* v, Vert42e80* dst, float t, int bNeg)
{
	float f;

	dst->y = (v[1].y - v[0].y) * t + v[0].y;
	f      = (v[1].z - v[0].z) * t + v[0].z;
	dst->z = f;
	dst->x = bNeg ? -f : f;
	sub_10023380(&v[0], &v[1], dst, t);
}

inline void Clip42e80Y(Vert42e80* v, Vert42e80* dst, float t, int bNeg)
{
	float f;

	dst->x = (v[1].x - v[0].x) * t + v[0].x;
	f      = (v[1].z - v[0].z) * t + v[0].z;
	dst->z = f;
	dst->y = bNeg ? -f : f;
	sub_10023380(&v[0], &v[1], dst, t);
}

extern "C" int sub_10042e80(void* pVerts, uint32 nMask)
{
	Vert42e80*	v = (Vert42e80*)pVerts;
	float		fT;

	// ---- 0x01: NEAR, z >= g_f1008b908 --------------------------------- //
	if (nMask & 1)
	{
		if (v[0].z >= g_f1008b908)
		{
			if (v[1].z < g_f1008b908)
			{
				fT = (g_f1008b908 - v[0].z) / (v[1].z - v[0].z);

				v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
				v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
				v[1].z = g_f1008b908;

				sub_10023380(&v[0], &v[1], &v[1], (float)fT);
			}
		}
		else
		{
			if (v[1].z < g_f1008b908)
				return 0;

			fT = (g_f1008b908 - v[0].z) / (v[1].z - v[0].z);

			v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			v[0].z = g_f1008b908;

			sub_10023380(&v[0], &v[1], &v[0], (float)fT);
		}
	}

	// ---- 0x04: LEFT, -z < x ------------------------------------------- //
	if (nMask & 4)
	{
		if (-v[0].z < v[0].x)
		{
			if (-v[1].z >= v[1].x)
			{
				fT = -(v[0].z + v[0].x)
				   / ((v[1].x - v[0].x) + v[1].z - v[0].z);

				Clip42e80X(v, &v[1], fT, 1);
			}
		}
		else
		{
			if (-v[1].z >= v[1].x)
				return 0;

			fT = -(v[0].z + v[0].x)
			   / ((v[1].x - v[0].x) + v[1].z - v[0].z);

				Clip42e80X(v, &v[0], fT, 1);
		}
	}

	// ---- 0x08: TOP, y < z --------------------------------------------- //
	if (nMask & 8)
	{
		if (v[0].y < v[0].z)
		{
			if (v[1].y >= v[1].z)
			{
				fT = -(v[0].y - v[0].z)
				   / ((v[1].y - v[0].y) - v[1].z + v[0].z);

				Clip42e80Y(v, &v[1], fT, 0);
			}
		}
		else
		{
			if (v[1].y >= v[1].z)
				return 0;

			fT = -(v[0].y - v[0].z)
			   / ((v[1].y - v[0].y) - v[1].z + v[0].z);

				Clip42e80Y(v, &v[0], fT, 0);
		}
	}

	// ---- 0x10: RIGHT, x < z ------------------------------------------- //
	if (nMask & 0x10)
	{
		if (v[0].x < v[0].z)
		{
			if (v[1].x >= v[1].z)
			{
				fT = -(v[0].x - v[0].z)
				   / ((v[1].x - v[0].x) - v[1].z + v[0].z);

				Clip42e80X(v, &v[1], fT, 0);
			}
		}
		else
		{
			if (v[1].x >= v[1].z)
				return 0;

			fT = -(v[0].x - v[0].z)
			   / ((v[1].x - v[0].x) - v[1].z + v[0].z);

				Clip42e80X(v, &v[0], fT, 0);
		}
	}

	// ---- 0x20: BOTTOM, -z < y ----------------------------------------- //
	if (nMask & 0x20)
	{
		if (-v[0].z < v[0].y)
		{
			if (-v[1].z >= v[1].y)
			{
				fT = -(v[0].z + v[0].y)
				   / ((v[1].y - v[0].y) + v[1].z - v[0].z);

				Clip42e80Y(v, &v[1], fT, 1);
			}
		}
		else
		{
			if (-v[1].z >= v[1].y)
				return 0;

			fT = -(v[0].z + v[0].y)
			   / ((v[1].y - v[0].y) + v[1].z - v[0].z);

				Clip42e80Y(v, &v[0], fT, 1);
		}
	}

	// ---- 0x02: FAR, z <= g_f1008b910 ---------------------------------- //
	if (nMask & 2)
	{
		if (v[0].z <= g_f1008b910)
		{
			if (v[1].z > g_f1008b910)
			{
				fT = (g_f1008b910 - v[0].z) / (v[1].z - v[0].z);

				v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
				v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
				v[1].z = g_f1008b910;

				sub_10023380(&v[0], &v[1], &v[1], (float)fT);
			}
		}
		else
		{
			if (v[1].z > g_f1008b910)
				return 0;

			fT = (g_f1008b910 - v[0].z) / (v[1].z - v[0].z);

				v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
				v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
				v[0].z = g_f1008b910;

				sub_10023380(&v[0], &v[1], &v[0], (float)fT);
		}
	}

	return 1;
}
