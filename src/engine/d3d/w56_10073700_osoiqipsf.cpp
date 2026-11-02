// *********************************************************************** //
//
//  w56_10073700_osoiqipsf.cpp -- d3d.ren 0x10073700 (the world-tree
//  shadow-polygon collector), moved out of w10_10073400_osoiqip.cpp
//  .  /Os /Oi- /Qip /Qsfalign16 (w<N>_*_osoiqipsf.cpp).
//
//  Why its own file: the function holds doubles (the two CRT sqrt
//  calls), and under icl's default /Qsfalign8 it gets the 14-byte
//  stack-realigning entry (`push %ebx ; mov %esp,%ebx ; sub $8 ;
//  and $-16 ; add $8 ; jmp`) and a second `$$1` body.  The shipped
//  0x10073700 opens straight on `push %ebp ; mov %esp,%ebp ; mov $0x3878,
//  %eax ; call __chkstk` -- no realign -- while its old file-mate
//  0x10073500 (quat_Slerp, ltquatbase.cpp) DOES carry the realign entry,
//  byte-identical.  So the two cannot share a flag set, and the
//  collector is not ltquatbase.cpp code anyway.  /Oi- because it CALLS
//  sqrt (0x1007fc98); /Os by its padding.  /Qip changes nothing here
//  (no same-file caller); _osoiqipsf is the existing /Os /Oi- convention
//  with /Qsfalign16.
//  Measured: in w10 at /Os /Oi- /Qip it was a 14-byte stub + 1189-byte
//  body, 1078 bytes differing ("attempted"); here 1172 bytes (shipped
//  1269), the first 0x1b bytes identical.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>

// ======================================================================= //
//  0x10073700, 1269 bytes.  __cdecl (pObject, pQuery): the world-tree
//  segment walk that collects the polygons a shadow may fall on.  Never
//  called directly: big48180.cpp (0x10048505) and big4b720.cpp
//  (0x1004bfce) hand its ADDRESS to the world query at 0x1008beac+0x6c,
//  slot +0x0c, together with the query block, and the query calls it per
//  object.  %eax is never set on purpose, so it returns nothing.
//
//  THE WALK.  Only objects with +0x54 clear and render type 2 at +0xbc
//  (a world model) are walked.  The segment pQuery->m_vPos..m_vEnd is
//  pushed down the object's BSP (root from its +0x1a8 tree, vtable slot
//  +0x04) with an explicit 512-entry stack of {node, start, end}
//  (0x1c bytes each, -0x3840..-0x40 of the frame -- 0x3878 bytes of frame
//  is the __chkstk probe at 0x10073708):
//
//    * a node whose flags byte (+0x16) has bit 0 or 1 set ends the
//      branch -- pop the next segment, or stop when the stack is empty;
//    * both ends further in FRONT of the node's plane than the query
//      radius (+0x1c) go to side +0x08, both further BEHIND go to side
//      +0x04, without splitting;
//    * otherwise the segment straddles: split at t = d1 / (d1 - d2)
//      (t = 0 when |d1 - d2| <= 1e-5), and offer the node's polygon --
//      if it was drawn this frame (+0x42 against the frame code
//      0x1008bea8), the list has room (< 256), the eye point is more than
//      1.0 in front of it, it does not face along the segment (dot with
//      the direction < 0.7), and the split point is within the polygon's
//      radius plus the query radius of its centre -- then append it to
//      the list unless it is already there;
//    * then, stack permitting, continue down the near side (the side the
//      start point is on) and push the far side: with the halves of the
//      segment when 0 < t < 1, with the whole segment otherwise.
// ======================================================================= //
struct LTVec
{
	float  x, y, z;
};

struct WPlane73700
{
	LTVec	m_vNormal;                                  // +0x00
	float	m_fDist;                                    // +0x0c
};

struct WSurface73700
{
	uint8			m_pad_00[0x34];
	WPlane73700*	m_pPlane34;                         // +0x34
};

struct WPoly73700
{
	uint8			m_pad_00[0x18];
	LTVec			m_vCenter18;                        // +0x18
	float			m_fRadius24;                        // +0x24
	WSurface73700*	m_pSurface28;                       // +0x28
	uint8			m_pad_2c[0x42 - 0x2c];
	uint16			m_wFrameCode42;                     // +0x42
};

struct WNode73700
{
	WPoly73700*		m_pPoly00;                          // +0x00
	WNode73700*		m_pSides04[2];                      // +0x04 back, +0x08 front
	uint8			m_pad_0c[0x16 - 0x0c];
	uint8			m_Flags16;                          // +0x16
};

class WTree73700
{
public:
	virtual void			Slot00();
	virtual WNode73700*		GetRootNode();              // vtable +0x04
};

struct WObject73700
{
	uint8			m_pad_000[0x54];
	uint32			m_n54;                              // +0x054
	uint8			m_pad_058[0xbc - 0x58];
	uint8			m_nType0bc;                         // +0x0bc
	uint8			m_pad_0bd[0x1a8 - 0xbd];
	WTree73700*		m_pTree1a8;                         // +0x1a8
};

struct ShadowList73700                                  // 0x404 bytes
{
	WPoly73700*		m_apItems[256];                     // +0x000
	uint32			m_nCount;                           // +0x400
};

struct ShadowQuery73700                                 // 0x20 bytes
{
	ShadowList73700*	m_pList;                        // +0x00
	LTVec				m_vPos;                         // +0x04
	LTVec				m_vEnd;                         // +0x10
	float				m_fRadius;                      // +0x1c
};

struct WStack73700                                      // 0x1c bytes
{
	WNode73700*		m_pNode;                            // +0x00
	LTVec			m_vStart;                           // +0x04
	LTVec			m_vEnd;                             // +0x10
};

extern uint16	g_1008bea8;                             // 0x1008bea8  frame code
extern LTVec	g_v1008bd34;                            // 0x1008bd34  eye point

inline LTVec VecSub73700(const LTVec* a, const LTVec* b)
{
	LTVec v;

	v.x = a->x - b->x;
	v.y = a->y - b->y;
	v.z = a->z - b->z;
	return v;
}

inline float DistToPlane73700(const WPlane73700* p, const LTVec* v)
{
	return p->m_vNormal.x*v->x + p->m_vNormal.y*v->y + p->m_vNormal.z*v->z - p->m_fDist;
}

void sub_10073700(WObject73700* pObject, ShadowQuery73700* pQuery)
{
	WStack73700		stack[512];
	WStack73700*	pTop;
	WStack73700*	pEnd;
	WNode73700*		pNode;
	LTVec			vStart, vEnd, vDir, vMid;
	float			fMag, d1, d2, fRadius;
	float			t;
	uint32			side, i, nCount;
	ShadowList73700* pList;
	WPoly73700*		pPoly;

	if (pObject->m_n54)
		return;

	if (pObject->m_nType0bc != 2)
		return;

	vStart = pQuery->m_vPos;
	vEnd = pQuery->m_vEnd;
	vDir = VecSub73700(&vEnd, &vStart);

	fMag = (float)sqrt(vDir.x*vDir.x + vDir.y*vDir.y + vDir.z*vDir.z);
	if (fMag != 0.0f)
	{
		float fInv = 1.0f / fMag;

		vDir.x *= fInv;
		vDir.y *= fInv;
		vDir.z *= fInv;
	}

	pTop = stack;
	pEnd = &stack[512];
	pNode = pObject->m_pTree1a8->GetRootNode();

	for (;;)
	{
		if (pNode->m_Flags16 & 3)
		{
			// End of this branch: take the next segment off the stack.
PopSegment:
			if (pTop == stack)
				return;

			pTop--;
			pNode = pTop->m_pNode;
			vStart = pTop->m_vStart;
			vEnd = pTop->m_vEnd;
			continue;
		}

		d1 = DistToPlane73700(pNode->m_pPoly00->m_pSurface28->m_pPlane34, &vStart);
		d2 = DistToPlane73700(pNode->m_pPoly00->m_pSurface28->m_pPlane34, &vEnd);
		fRadius = pQuery->m_fRadius;

		if (d1 > fRadius && d2 > fRadius)
		{
			pNode = pNode->m_pSides04[1];
			continue;
		}

		if (d1 < -fRadius && d2 < -fRadius)
		{
			pNode = pNode->m_pSides04[0];
			continue;
		}

		// The segment straddles the plane.
		if (d1 > 0.0f)
			side = 1;
		else
			side = 0;

		if ((d1 - d2) > 0.00001f || (d1 - d2) < -0.00001f)
			t = d1 / (d1 - d2);
		else
			t = 0.0f;

		vMid.x = vStart.x + (vEnd.x - vStart.x) * t;
		vMid.y = vStart.y + (vEnd.y - vStart.y) * t;
		vMid.z = vStart.z + (vEnd.z - vStart.z) * t;

		pPoly = pNode->m_pPoly00;
		if (pPoly->m_wFrameCode42 == g_1008bea8 &&
			pQuery->m_pList->m_nCount < 256 &&
			DistToPlane73700(pNode->m_pPoly00->m_pSurface28->m_pPlane34, &g_v1008bd34) > 1.0f)
		{
			WPlane73700* pPlane = pNode->m_pPoly00->m_pSurface28->m_pPlane34;

			if (pPlane->m_vNormal.x*vDir.x + pPlane->m_vNormal.y*vDir.y +
				pPlane->m_vNormal.z*vDir.z < 0.7f)
			{
				LTVec vOff;

				pPoly = pNode->m_pPoly00;
				vOff = VecSub73700(&vMid, &pPoly->m_vCenter18);

				if (sqrt(vOff.x*vOff.x + vOff.y*vOff.y + vOff.z*vOff.z) <
					pPoly->m_fRadius24 + pQuery->m_fRadius)
				{
					pList = pQuery->m_pList;
					nCount = pList->m_nCount;

					for (i = 0; i < nCount; i++)
					{
						if (pList->m_apItems[i] == pPoly)
							break;
					}

					if (i >= nCount)
					{
						pList->m_apItems[nCount] = pPoly;
						pQuery->m_pList->m_nCount++;
					}
				}
			}
		}

		// Stack full: this branch is dropped.
		if (pTop >= pEnd)
			goto PopSegment;

		if (t > 0.0f && t < 1.0f)
		{
			pTop->m_vStart = vMid;
			pTop->m_vEnd = vEnd;
			vEnd = vMid;
		}
		else
		{
			pTop->m_vStart = vStart;
			pTop->m_vEnd = vEnd;
		}

		pTop->m_pNode = pNode->m_pSides04[side == 0];
		pNode = pNode->m_pSides04[side];
		pTop++;
	}
}
