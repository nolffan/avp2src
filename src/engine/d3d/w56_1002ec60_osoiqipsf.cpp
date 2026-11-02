// *********************************************************************** //
//
//  MODULE:   W56_1002EC60_OSOIQIPSF (.CPP)  --  0x1002ec60, per-vertex
//            dynamic lighting of a 32-byte TL vertex run.  /Os /Oi- /Qip
//            /Qsfalign16.  A NEAR.
//
//  Renamed from w5_1002ec60_osoi.cpp onto the _osoiqipsf
//  convention.  /Qsfalign16 drops the 14-byte realign stub (see
//  below); under /Os /Oi- the function was stub + 699-byte $$1 body, 619
//  bytes off, and could not be paired ("attempted"); here it is
//  709 bytes against the shipped 724, 25 bytes off (a near).  /Qip changes
//  nothing (no same-file caller; identical object with and without).
//
//  One caller, 0x1002f08c (big2ef34.cpp), which declares it
//  `extern "C" void sub_1002ec60(Obj2ef34*, Vert2ef34*, uint32)`.  The third
//  argument is never read (nothing touches 0xb8(%esp)); the count comes
//  from the object's own +0x50 WORD.  big3e620.cpp carries the same body
//  INLINED (its light pass) and reads it the same way.
//
//  WHAT THE BYTES SAY:
//   * pObj->+0x2c is a singly linked light list: node +0x00 next,
//     +0x04 the light, +0x08 an LTVector copied out whole (`rep movsl`,
//     ecx=3) into a frame local that the plane test reads back.
//   * the light's colour is three bytes at +0x94/+0x95/+0x96 and its
//     radius a float at +0x1a8.
//   * g_1008be48 is incremented once per light.
//   * pObj->+0x28->+0x34 is a plane {normal, dist}; the light's distance to
//     it is taken, negated when negative (`fcomp` against the 0.0f at
//     0x100a674c, `jbe` over `fchs`), and a light at or beyond its radius
//     is skipped.
//   * each colour channel becomes `c - (255.0f - c)`: `flds 255.0f; fsub
//     st(2); fsubp st(2)` -- DE EA is Intel's FSUBP, st(2) = st(2) - st(0).
//   * the vertex test is `distSq < radius*radius`; the weight is
//     `1.0f - sqrt(distSq) * (1.0f / radius)` with sqrt CALLED (0x1007fc98,
//     the CRT's double sqrt), and each channel adds `(int)(delta * weight)`
//     to the vertex's own byte (+0x12 red, +0x11 green, +0x10 blue),
//     doubled when g_1008cb98 is set, clamped to 0..255.  `dec; jne` over
//     a count read once is the do/while on nLeft.
//   * the pool {255,255,255, 0, 1(dead),1, 1} at 0x100a6740 is the three
//     255s, the zero, the division's pair and the weight's 1.0.  All float
//     constants (`flds`), so the variables are float; the `fstpl` double
//     slots are icl's x87 spills.
//
//  THE FLAG, measured on this body:
//   * the sqrt CALL is /Oi-;
//   * the `fnstcw / or $0xc00 / fldcw / fistpll / fldcw` conversions, the
//     `rep movsl` of the position and the 0x9c frame all come out at /Os
//     and NOT at /O2 (which scalarises the copy and emits the soft
//     shld/0x41e double-to-int sequence instead); the tail padding
//     `8d 76 00` + 0x90s is /Os's.
//
//  THE RESIDUAL: icl adds a 14-byte stack-realignment entry stub
//  (`push %ebx; mov %esp,%ebx; ... and $-16,%esp`) and addresses the
//  argument off %ebx.  It is caused by the double-argument sqrt call
//  (removing the call removes it) and survives every source spelling tried
//  (float/double temporaries, a private sqrt prototype, a separate sqrt
//  statement).  Compiled with /Os /Oi- /Qsfalign- instead, the stub goes and
//  the function is 705 bytes against the shipped 709 with 8 mnemonic
//  blocks left (register choice for the g_1008be48 increment and the
//  placement of the next-light block).  /Qsfalign- is not one of the flag
//  sets, so it is recorded here and not applied.
//
//  /Qsfalign16 (conventions _ossf /
//  _osoiqipsf) removes the stub the same way, and with three source
//  changes the body is 709 bytes against 709 and 25 bytes off:
//    * the vertex pointer is re-read from the argument at the TOP of each
//      light (`mov 0xb4(%esp),%ebp` right after the position copy) into
//      its own variable, copied into the walking pointer only before the
//      vertex loop (`mov %ebp,%ebx`) -- pFirst / pVert;
//    * the radius test encloses the rest (`if (fDist < radius) {...}`,
//      not `continue`), and the vertex loop is `for (; nLeft > 0; nLeft--)`
//      -- `test ; ja` into it, `dec ; jne` at the bottom, the next-light
//      step after it -- not a guarded do/while;
//  What is left: the shipped frame is 0x9c, ours 0x8c (every slot the
//  same; the shipped function reserves at least one more, never touched),
//  and `fldl radius ; fmul` scheduled after the vertex-pointer copy.
//  Tried without moving the frame: a padding double / extra LTVector
//  locals, the delta and the colour as LTVectors (MagSqr, DistSqr, a
//  scaled colour temporary), LTPlane::DistTo, separate vertex-colour
//  ints, the radius statements reordered, (double) sqrt; `/ fRadius`
//  for `* fInvRadius` SHRINKS it by 16.  Under the file's own /Os /Oi-
//  the body is 699 bytes, 619 off (was 606); kept for the structure.
//  Not renamed onto a sfalign convention: no identical to gain yet.
//
//  The "+16 frame = icl's /Qip out-of-line copy of an
//  inline" reading (0x1000ac10's, now closed that way in big0ae20.cpp):
//  NOT reproduced for this body.  At /Os /Oi- /Qip (and /Qsfalign16) this
//  body as an inline free function or inline member, reached through an
//  inline free or in-class wrapper from an ordinary function, is inlined
//  and NO copy is emitted (also with one argument only, with sqrt
//  removed, with a local array).  Inverse's copy appears for the same
//  wrapper shapes, also as a free function with three arguments, but
//  disappears when its elimination loop is removed -- so the trigger is
//  something in the body, not the call shape, and it is not identified.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>

extern unsigned long g_1008be48;    // 0x1008be48 -- per-light stamp
extern unsigned long g_1008cb98;    // 0x1008cb98 -- double the lit colour

struct Plane1002ec60
{
	LTVector m_vNormal;                                 // +0x00
	float m_fDist;                                      // +0x0c
};

struct Holder1002ec60
{
	uint8          m_pad_00[0x34];
	Plane1002ec60* m_pPlane;                            // +0x34
};

struct Light1002ec60
{
	uint8 m_pad_00[0x94];
	uint8 m_nR;                                         // +0x94
	uint8 m_nG;                                         // +0x95
	uint8 m_nB;                                         // +0x96
	uint8 m_pad_97[0x1a8 - 0x97];
	float m_fRadius;                                    // +0x1a8
};

struct LightNode1002ec60
{
	LightNode1002ec60* m_pNext;                         // +0x00
	Light1002ec60*     m_pLight;                        // +0x04
	LTVector           m_vPos;                          // +0x08
};

struct Obj1002ec60
{
	uint8              m_pad_00[0x28];
	Holder1002ec60*    m_pHolder;                       // +0x28
	LightNode1002ec60* m_pLights;                       // +0x2c
	uint8              m_pad_30[0x50 - 0x30];
	uint16             m_nVerts;                        // +0x50
};

struct Vert1002ec60
{
	float m_fX, m_fY, m_fZ;                             // +0x00
	uint32 m_n0c;                                       // +0x0c
	uint8 m_nB;                                         // +0x10
	uint8 m_nG;                                         // +0x11
	uint8 m_nR;                                         // +0x12
	uint8 m_nA;                                         // +0x13
	uint32 m_n14, m_n18, m_n1c;                         // +0x14
};

extern "C" void sub_1002ec60(Obj1002ec60* pObj, Vert1002ec60* pVerts, uint32 nUnused)
{
	LightNode1002ec60* pNode;
	Light1002ec60*     pLight;
	Plane1002ec60*     pPlane;
	Vert1002ec60*      pVert;
	Vert1002ec60*      pFirst;
	LTVector           vPos;
	uint32             nLeft;
	int32              r, g, b, vr, vg, vb;
	float              fR, fG, fB;
	float              fDist, fRadius, fInvRadius, fRadiusSq;
	float              dx, dy, dz, fDistSq, fScale;

	for (pNode = pObj->m_pLights; pNode; pNode = pNode->m_pNext)
	{
		pLight = pNode->m_pLight;
		vPos = pNode->m_vPos;
		pFirst = pVerts;

		r = pLight->m_nR;
		g = pLight->m_nG;
		b = pLight->m_nB;

		g_1008be48++;

		pPlane = pObj->m_pHolder->m_pPlane;
		fDist = pPlane->m_vNormal.Dot(vPos) - pPlane->m_fDist;

		fR = r - (255.0f - r);
		fG = g - (255.0f - g);
		fB = b - (255.0f - b);

		if (fDist < 0.0f)
			fDist = -fDist;

		if (fDist < pLight->m_fRadius)
		{

		fRadius = pLight->m_fRadius;
		fInvRadius = 1.0f / fRadius;
		fRadiusSq = fRadius * fRadius;

		nLeft = pObj->m_nVerts;
		pVert = pFirst;
		for (; nLeft > 0; nLeft--)
		{
			dx = pVert->m_fX - vPos.x;
			dy = pVert->m_fY - vPos.y;
			dz = pVert->m_fZ - vPos.z;
			fDistSq = dx * dx + dy * dy + dz * dz;

			if (fDistSq < fRadiusSq)
			{
				fScale = 1.0f - (float)sqrt(fDistSq) * fInvRadius;

				vr = pVert->m_nR + (int32)(fR * fScale);
				vg = pVert->m_nG + (int32)(fG * fScale);
				vb = pVert->m_nB + (int32)(fB * fScale);

				if (g_1008cb98)
				{
					vr += vr;
					vg += vg;
					vb += vb;
				}

				if (vr > 255) vr = 255; else if (vr < 0) vr = 0;
				if (vg > 255) vg = 255; else if (vg < 0) vg = 0;
				if (vb > 255) vb = 255; else if (vb < 0) vb = 0;

				pVert->m_nR = (uint8)vr;
				pVert->m_nG = (uint8)vg;
				pVert->m_nB = (uint8)vb;
			}

			pVert++;
		}
		}
	}
}
