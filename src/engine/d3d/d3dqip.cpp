// *********************************************************************** //
//
//  d3dqip.cpp -- the world-model half of the dynamic-light leaf pass.
//
//  THIS FILE MUST BE COMPILED /Qip. See D3D_QIP_TUS in build/engine.sh.
//
//  0x10040cc0 (w7_10040cc0.cpp) queries the world for every object a
//  dynamic light's sphere touches, with 0x10040ca0 as the callback.  For
//  a world model (+0xbc == 2) the callback hands the light to 0x10040930,
//  which pushes the light down the model's BSP and hangs a light link on
//  every polygon the sphere touches:
//
//    0x10040ca0  30 bytes.  %edx = the object, %eax = the light, and a
//                call to 0x10040938 -- the REGISTER entry of 0x10040930
//                (%eax = light, %edx = model) -- pushing nothing.  A /Qip
//                register call, so both live in this TU.
//    0x10040930  872 bytes, the pair: `mov 0x4(%esp),%eax ; mov
//                0x8(%esp),%edx` then the register entry at 0x10040938.
//                External linkage (it has a stack entry), not inlined.
//
//  0x10040930 in C:
//    * the light's position into the model's space: ltmatrix.h's
//      MatVMul_H through the model's inverse transform (+0x230);
//    * a 1024-entry node stack; the walk starts at the model BSP's root
//      (+0x1a8, virtual +0x04);
//    * a node with flags byte (+0x16) bit 0 or 1 ends the branch: pop the
//      next node, or return when the stack is empty;
//    * the sphere wholly behind the node's plane goes to side +0x04,
//      wholly in front to side +0x08;
//    * otherwise the node's polygon is offered -- it must have been drawn
//      this frame (+0x42 against 0x1008bea8), a light flagged 0x40 must be
//      at least 0.01 in front of the plane, and the polygon's bounding
//      sphere (+0x18, +0x24) must touch the light's -- else the light is
//      counted rejected (0x1008bd78).  An accepted light is linked to the
//      polygon's list (+0x2c) unless it is already there: a link from the
//      0x1008bef4 bank {next, light, local position}, and a polygon whose
//      list was empty goes onto the frame's lit-polygon list 0x1008bf14
//      (node from the 0x1008bed8 bank) -- the list 0x1001eb20
//      (d3dr3b.cpp) clears;
//    * then the front side is pushed and the walk continues at the back.
//
//  The rejected-light counter is the one big2d1c0.cpp prints; the two
//  banks are the ones d3dr2b.cpp initialises and w4_1001e4a0_os.cpp
//  terms.
//
//  RESULTS: 0x10040930 / 0x10040938 (the pair) and
//  0x10040ca0 IDENTICAL.  What it took:
//    * the plane is reached through an inline Node::GetPlane() and the
//      polygon read again as pNode->m_pPoly00: with one spelling for both
//      icl CSEs the node's +0x00 load into %edi, where the shipped code
//      loads it twice (%eax for the plane, %edi for the polygon);
//    * the push is `*pTop = ...; pTop++;` and the pop `pTop--; pNode =
//      *pTop;` (each other spelling moves the pointer through a temp);
//    * the polygon test is an inline helper taking the distance and the
//      radius by value: written in line, icl gives the two x87 spills
//      (-0x10 / -0x18) the other way round, under every declaration and
//      statement order tried.
//
//  (Earlier the callee here was a deliberately wrong, recursive
//  placeholder `qip_Callee10040938`; the caller matched without it
//  because the byte comparison masks a call's relocation.)
// *********************************************************************** //
#include "ltbasedefs.h"

struct Vec10040930
{
	float	x, y, z;
};

struct Mat10040930
{
	float	m[4][4];
};

struct Plane10040930
{
	Vec10040930	m_Normal;                               // +0x00
	float		m_Dist;                                 // +0x0c
};

struct Surface10040930
{
	uint8			m_pad_00[0x34];
	Plane10040930*	m_pPlane34;                         // +0x34
};

struct Link10040930;

struct Poly10040930
{
	uint8				m_pad_00[0x18];
	Vec10040930			m_vCenter18;                    // +0x18
	float				m_fRadius24;                    // +0x24
	Surface10040930*	m_pSurface28;                   // +0x28
	Link10040930*		m_pLights2c;                    // +0x2c
	uint8				m_pad_30[0x42 - 0x30];
	uint16				m_wFrameCode42;                 // +0x42
};

struct Node10040930
{
	Poly10040930*	m_pPoly00;                          // +0x00
	Node10040930*	m_pSides04[2];                      // +0x04 back, +0x08 front
	uint8			m_pad_0c[0x16 - 0x0c];
	uint8			m_Flags16;                          // +0x16

	Plane10040930*	GetPlane()	{ return m_pPoly00->m_pSurface28->m_pPlane34; }
};

class Tree10040930
{
public:
	virtual void			Slot00();
	virtual Node10040930*	GetRootNode();              // vtable +0x04
};

// The world model (LTObject / WorldModelObject offsets, as
// w24_10068c80_osqipni.cpp spells them).
struct Obj10040ca0
{
	uint8			m_pad00[0xbc];
	uint8			m_bMode;                            // +0xbc  object type
	uint8			m_pad_bd[0x1a8 - 0xbd];
	Tree10040930*	m_pWorldBsp;                        // +0x1a8
	uint8			m_pad_1ac[0x230 - 0x1ac];
	Mat10040930		m_Transform230;                     // +0x230  inverse
};

// The dynamic light (w7_10040cc0.cpp's C1001e210).
struct Ctx10040938
{
	uint8			m_pad_00[0x88];
	uint32			m_Flags88;                          // +0x88
	uint8			m_pad_8c[0x18c - 0x8c];
	Vec10040930		m_Pos;                              // +0x18c
	uint8			m_pad_198[0x1a8 - 0x198];
	float			m_fRadius;                          // +0x1a8
};

// The light link a polygon keeps (0x14 bytes, 0x1008bef4's bank).
struct Link10040930
{
	Link10040930*	m_pNext;                            // +0x00
	Ctx10040938*	m_pLight;                           // +0x04
	Vec10040930		m_vPos;                             // +0x08
};

// The lit-polygon list entry (8 bytes, 0x1008bed8's bank); d3dr3b.cpp's
// tag, so the list head needs no alias.
struct Node1001eb20
{
	Poly10040930*	m_pPoly;                            // +0x00
	Node1001eb20*	m_pNext;                            // +0x04
};

// StdLith's StructBank, d3dr2b.cpp's spelling.
struct StructBank
{
	uint32 m_StructSize;                                // +0x00
	uint32 m_AlignedStructSize;                         // +0x04
	uint32 m_CacheSize;                                 // +0x08
	uint32 m_nPages;                                    // +0x0c
	uint32 m_nTotalObjects;                             // +0x10
	void*  m_PageHead;                                  // +0x14
	void*  m_FreeListHead;                              // +0x18
};

extern StructBank		g_1008bed8;                     // 0x1008bed8  list-node bank
extern StructBank		g_1008bef4;                     // 0x1008bef4  light-link bank
extern Node1001eb20*	g_pList1008bf14;                // 0x1008bf14  lit polygons
extern uint16			g_1008bea8;                     // 0x1008bea8  frame code
extern long				g_nRejectedLights;              // 0x1008bd78

// 0x1007fbf0 -- StdLith's sb_AllocateNewStructPage.
extern "C" int sub_1007fbf0(StructBank* pBank, uint32 nAllocations);

// struct_bank.h's sb_Allocate.
inline void* sb_Allocate10040930(StructBank* pBank)
{
	void* pRet;

	pRet = pBank->m_FreeListHead;
	if (!pRet)
	{
		sub_1007fbf0(pBank, pBank->m_CacheSize);
		pRet = pBank->m_FreeListHead;
		if (!pRet)
			return 0;
	}

	pBank->m_FreeListHead = *(void**)pRet;
	return pRet;
}

// ltmatrix.h's MatVMul_H.
inline float MatVMul_H(Vec10040930* pDest, Mat10040930* pMat, Vec10040930* pSrc)
{
	float one_over_w = 1.0f / (pMat->m[3][0]*pSrc->x + pMat->m[3][1]*pSrc->y + pMat->m[3][2]*pSrc->z + pMat->m[3][3]);

	pDest->x = one_over_w * (pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z + pMat->m[0][3]);
	pDest->y = one_over_w * (pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z + pMat->m[1][3]);
	pDest->z = one_over_w * (pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z + pMat->m[2][3]);

	return one_over_w;
}

inline float DistToPlane10040930(const Plane10040930* p, const Vec10040930* v)
{
	return p->m_Normal.x*v->x + p->m_Normal.y*v->y + p->m_Normal.z*v->z - p->m_Dist;
}

inline LTBOOL LightTouches10040930(Ctx10040938* pLight, Poly10040930* pPoly,
                                   Vec10040930* pPos, float fDist, float fRadius)
{
	Vec10040930 vOff;

	if (pPoly->m_wFrameCode42 != g_1008bea8)
		return LTFALSE;
	if ((pLight->m_Flags88 & 0x40) && fDist < 0.01f)
		return LTFALSE;

	vOff.x = pPos->x - pPoly->m_vCenter18.x;
	vOff.y = pPos->y - pPoly->m_vCenter18.y;
	vOff.z = pPos->z - pPoly->m_vCenter18.z;

	return vOff.x*vOff.x + vOff.y*vOff.y + vOff.z*vOff.z <=
	       (pPoly->m_fRadius24 + fRadius) * (pPoly->m_fRadius24 + fRadius);
}

void sub_10040930(Ctx10040938* pLight, Obj10040ca0* pWM)
{
	Node10040930*	stack[1024];
	Node10040930**	pTop;
	Node10040930*	pNode;
	Poly10040930*	pPoly;
	Link10040930*	pLink;
	Node1001eb20*	pList;
	Vec10040930		vPos;
	float			fDist, fRadius;
	LTBOOL			bFound;

	MatVMul_H(&vPos, &pWM->m_Transform230, &pLight->m_Pos);

	pTop = stack;
	pNode = pWM->m_pWorldBsp->GetRootNode();

	for (;;)
	{
		while (pNode->m_Flags16 & 3)
		{
			if (pTop == stack)
				return;

			pTop--;
			pNode = *pTop;
		}

		fDist = DistToPlane10040930(pNode->GetPlane(), &vPos);
		fRadius = pLight->m_fRadius;

		if (fDist < -pLight->m_fRadius)
		{
			pNode = pNode->m_pSides04[0];
		}
		else if (fDist > fRadius)
		{
			pNode = pNode->m_pSides04[1];
		}
		else
		{
			pPoly = pNode->m_pPoly00;

			if (LightTouches10040930(pLight, pPoly, &vPos, fDist, fRadius))
			{
				bFound = LTFALSE;

				if (!pPoly->m_pLights2c)
				{
					pList = (Node1001eb20*)sb_Allocate10040930(&g_1008bed8);
					if (pList)
					{
						pList->m_pPoly = pPoly;
						pList->m_pNext = g_pList1008bf14;
						g_pList1008bf14 = pList;
					}
				}
				else
				{
					for (pLink = pPoly->m_pLights2c; pLink; pLink = pLink->m_pNext)
					{
						if (pLink->m_pLight == pLight)
						{
							bFound = LTTRUE;
							break;
						}
					}
				}

				if (!bFound)
				{
					pLink = (Link10040930*)sb_Allocate10040930(&g_1008bef4);
					if (pLink)
					{
						pLink->m_vPos = vPos;
						pLink->m_pLight = pLight;
						pLink->m_pNext = pPoly->m_pLights2c;
						pPoly->m_pLights2c = pLink;
					}
				}
			}
			else
			{
				g_nRejectedLights++;
			}

			*pTop = pNode->m_pSides04[1];
			pTop++;
			pNode = pNode->m_pSides04[0];
		}
	}
}

void sub_10040ca0(Obj10040ca0* pObj, Ctx10040938* pCtx)
{
	if (pObj->m_bMode == 2)
		sub_10040930(pCtx, pObj);
}
