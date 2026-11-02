// *********************************************************************** //
//
//  w24_10068c80_osqipni.cpp -- written for the d3d.ren link.
//
//  d3d.ren's copy of the engine's segment-query object callback, the
//  pieces w17_10069a10_osoi.cpp's 0x10069a10 (w_IntersectSegment) hands
//  to the root's segment walk:
//
//    0x10069190  w_IsectObjectCB -- per object: the two flag gates, the
//                bounding-sphere reject, the query's filter hook, and then
//                either the object's own geometry (0x10068c80) or the
//                world-box test, keeping the nearest hit in the query
//                globals.  server.dll's objbank.cpp has the same code
//                (0x1000f0e8 and the helpers it calls); here the sphere
//                test, w_IsectObject AND the box test are all inlined,
//                only the geometry test is called.
//    0x10068c80  w_IsectObjectGeometry -- a WorldModel/Container: the
//                segment taken into the object's space (MatVMul_H through
//                +0x230), walked with the query's poly test (0x10091154),
//                the hit taken back out (+0x1f0) and kept if nearest.
//
//  FLAGS /Os /Qip /Qip_no_inlining:
//    * 0x10069190 calls 0x10068c80 through the /Qip register entry --
//      `mov %edx,%eax ; call` with the dead 8-byte reserve, and 0x10068c80
//      reads its object from %eax with no stack entry in front of it: an
//      internal-linkage callee in the caller's /Qip TU;
//    * both copy their plane and point with `rep movsl` (the /Os struct
//      copy) and pad with 0x90s;
//    * with plain /Qip the single-caller static would be inlined into its
//      caller; the shipped code calls it.  The explicitly `inline`
//      helpers still expand under /Qip_no_inlining.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ----------------------------------------------------------------------- //
//  The query globals, 0x10091130..0x10091194, spelled as w17_10069a10_osoi.cpp
//  spells them.
// ----------------------------------------------------------------------- //

struct Vec10069a10
{
	float	x, y, z;
};

struct Surface10069a10
{
	uint8		m_pad_00[0x34];
	LTPlane*	m_pPlane34;                             // +0x34
	uint8		m_pad_38[0x3c - 0x38];
	uint16		m_nFlags3c;                             // +0x3c
};

struct Poly10069a10
{
	uint8				m_pad_00[0x28];
	Surface10069a10*	m_pSurface28;                   // +0x28
};

extern Vec10069a10		g_10091130;                     // 0x10091130 scaled direction
extern float			g_1009113c;                     // 0x1009113c start fraction
extern float			g_10091140;                     // 0x10091140 length
extern Vec10069a10		g_10091144;                     // 0x10091144 direction
extern IntersectQuery*	g_10091150;                     // 0x10091150
extern void*			g_10091154;                     // 0x10091154 poly test
extern float			g_10091158;                     // 0x10091158 range^2
extern Poly10069a10**	g_1009115c;                     // 0x1009115c hit poly
extern HOBJECT			g_10091160;                     // 0x10091160 hit object
extern LTPlane			g_10091164;                     // 0x10091164 hit plane
extern Vec10069a10		g_10091174;                     // 0x10091174 hit point
extern HPOLY			g_10091180;                     // 0x10091180 hit poly handle
extern uint8			g_10091184;                     // 0x10091184 !(flags & 2)
extern uint8			g_10091188;                     // 0x10091188 flags & 1

#define G_vScaledDir	(*(LTVector*)&g_10091130)
#define G_vDir			(*(LTVector*)&g_10091144)
#define G_vPoint		(*(LTVector*)&g_10091174)


// ----------------------------------------------------------------------- //
//  The object.  Offsets from the two functions below; they are
//  LTObject's (m_Flags +0x88, m_ObjectType +0xbc, the world box
//  +0xf0/+0xfc, m_Pos +0x18c, m_Radius +0x1a4) and WorldModelObject's
//  (the BSP +0x1a8, the forward transform +0x1f0, the inverse +0x230).
// ----------------------------------------------------------------------- //

struct Mat69190
{
	float	m[4][4];

	inline LTPlane TransformPlane(LTPlane& thePlane);
};

// The world BSP's poly test takes it as its first argument (the per-poly
// callbacks 0x10069060 / 0x10069110 call its +0x04 / +0x08 virtuals).
struct WorldBsp69190;

class Obj69190
{
public:
	virtual void	_v00();
	virtual void	_v04();
	virtual void	_v08();
	virtual void	_v0c();
	virtual void	_v10();
	virtual void	_v14();
	virtual void	_v18();
	virtual void	_v1c();
	virtual void	_v20();
	virtual void	_v24();
	virtual void	_v28();
	virtual void	_v2c();
	virtual void	_v30();
	virtual LTBOOL	TakesPartInIsect();                 // +0x34

	uint8			m_pad_04[0x88 - 0x04];
	uint32			m_Flags;                            // +0x88
	uint8			m_pad_8c[0xbc - 0x8c];
	uint8			m_ObjectType;                       // +0xbc
	uint8			m_pad_bd[0xf0 - 0xbd];
	LTVector		m_BBoxMin;                          // +0xf0
	LTVector		m_BBoxMax;                          // +0xfc
	uint8			m_pad_108[0x18c - 0x108];
	LTVector		m_Pos;                              // +0x18c
	uint8			m_pad_198[0x1a4 - 0x198];
	float			m_Radius;                           // +0x1a4
	WorldBsp69190*	m_pWorldBsp;                        // +0x1a8
	uint8			m_pad_1ac[0x1f0 - 0x1ac];
	Mat69190		m_Transform1f0;                     // +0x1f0
	Mat69190		m_Transform230;                     // +0x230
};

// The per-poly test's shape: the eighth argument is a byte.
typedef void (*PolyTest69190)(WorldBsp69190* pWorldBsp, Poly10069a10*** pppPoly,
                              LTVector* pPoint, float* pDist, HPOLY* pHPoly,
                              LTVector* pFrom, LTVector* pTo, uint8 bUnused);


// ----------------------------------------------------------------------- //
//  ltmatrix.h's inline transforms, over the object's matrices.
// ----------------------------------------------------------------------- //

inline float MatVMul_H(LTVector* pDest, Mat69190* pMat, LTVector* pSrc)
{
	float one_over_w = 1.0f / (pMat->m[3][0]*pSrc->x + pMat->m[3][1]*pSrc->y + pMat->m[3][2]*pSrc->z + pMat->m[3][3]);

	pDest->x = one_over_w * (pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z + pMat->m[0][3]);
	pDest->y = one_over_w * (pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z + pMat->m[1][3]);
	pDest->z = one_over_w * (pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z + pMat->m[2][3]);

	return one_over_w;
}

inline float MatVMul_InPlace_H(Mat69190* pMat, LTVector* pSrc)
{
	float one_over_w = 1.0f / (pMat->m[3][0]*pSrc->x + pMat->m[3][1]*pSrc->y + pMat->m[3][2]*pSrc->z + pMat->m[3][3]);
	LTVector temp;

	temp.x = one_over_w * (pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z + pMat->m[0][3]);
	temp.y = one_over_w * (pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z + pMat->m[1][3]);
	temp.z = one_over_w * (pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z + pMat->m[2][3]);

	*pSrc = temp;
	return one_over_w;
}

inline void MatVMul_3x3(LTVector* pDest, Mat69190* pMat, LTVector* pSrc)
{
	pDest->x = pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z;
	pDest->y = pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z;
	pDest->z = pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z;
}


// ltmatrix.h's TransformPlane as the shipped build's header had it (see
// objbank.cpp's 0x1000f74e): the start point is NOT transformed.
inline LTPlane Mat69190::TransformPlane(LTPlane& thePlane)
{
	LTVector startPt;
	LTPlane ret;

	startPt = thePlane.m_Normal * thePlane.m_Dist;
	MatVMul_3x3(&ret.m_Normal, this, &thePlane.m_Normal);
	ret.m_Dist = ret.m_Normal.Dot(startPt);

	return ret;
}


// ======================================================================= //
//  0x10068c80 -- 980 bytes (0x10068c80..0x10069054).  w_IsectObjectGeometry:
//  a WorldModel or a Container against the segment.  objbank.cpp's
//  0x1000f74e statement for statement; the /Qip register entry takes the
//  object in %eax.
//
//  NEAR: 972 bytes against 980, the same calls, stores and frame slots
//  from the poly-test call on.  What differs is register allocation in
//  the two MatVMul_H expansions: the shipped code saves %ebp too and
//  keeps the query's m_From in it (a copy of %eax) while reloading
//  0x10091150 for the second transform; and the take block re-derives
//  the poly's plane pointer three times (one chain per use inside the
//  inlined TransformPlane) where icl here computes it once.  Tried: a
//  plane local, TransformPlane with a reference parameter (as written).
// ======================================================================= //

static uint8 sub_10068c80(Obj69190* pObj)
{
	float			fDist;
	HPOLY			hPoly;
	LTVector		vPoint;
	LTPlane			Plane;
	LTVector		vLocal[2];

	Poly10069a10**	pPolyRef;

	MatVMul_H(&vLocal[0], &pObj->m_Transform230, &g_10091150->m_From);
	MatVMul_H(&vLocal[1], &pObj->m_Transform230, &g_10091150->m_To);

	((PolyTest69190)g_10091154)(pObj->m_pWorldBsp, &pPolyRef, &vPoint, &fDist,
	                            &hPoly, &vLocal[0], &vLocal[1], 1);

	if (!pPolyRef)
		return 0;

	MatVMul_InPlace_H(&pObj->m_Transform1f0, &vPoint);

	fDist = g_10091150->m_From.DistSqr(vPoint);

	if (fDist < g_10091158)
	{
		Plane = pObj->m_Transform1f0.TransformPlane(*(*pPolyRef)->m_pSurface28->m_pPlane34);

		g_10091160 = (HOBJECT)pObj;
		g_1009115c = pPolyRef;
		g_10091158 = fDist;
		g_10091180 = hPoly;
		g_10091164 = Plane;
		G_vPoint   = vPoint;

		return 1;
	}

	return 0;
}


// ----------------------------------------------------------------------- //
//  The inlined helpers of 0x10069190, in objbank.cpp's spellings.
// ----------------------------------------------------------------------- //

// The bounding-sphere reject (server 0x1000f650).
inline uint8 IsectObjectBounds69190(LTVector* pFrom, LTVector* pTo, Obj69190* pObj)
{
	LTVector vOn;
	float    t;

	t = G_vScaledDir.Dot(pObj->m_Pos) - g_1009113c;

	if (t >= -pObj->m_Radius && t <= g_10091140 + pObj->m_Radius)
	{
		vOn = g_10091150->m_From + G_vDir * t;

		return pObj->m_Radius * pObj->m_Radius > (vOn - pObj->m_Pos).MagSqr();
	}

	return 0;
}

inline LTBOOL IsGeometryObject69190(Obj69190* pObj)
{
	return pObj->m_ObjectType == OT_WORLDMODEL ||
	       pObj->m_ObjectType == OT_CONTAINER;
}

// The world-box test (server 0x1000f1f7).  The positive faces report the
// box MINIMUM as the plane distance, as server.dll's copy does -- and here
// as `min * 1.0f`: every face's distance is an `fmuls` by a pool constant
// (-1.0f for the negative faces, 1.0f for the positive ones at 0x100ab094,
// 0x100ab080, 0x100ab06c), each after its Init's three and a division's
// dead 1.0f, which VC6 folded away in server.dll and icl does not.
inline uint8 IsectObjectShape69190(LTVector* pFrom, LTVector* pTo, Obj69190* pObj,
                                   LTVector* pPoint, LTPlane* pPlane)
{
	float fCross[2];
	float t;

	if (pFrom->x < pObj->m_BBoxMin.x)
	{
		if (pTo->x < pObj->m_BBoxMin.x)
			return 0;

		t = (pObj->m_BBoxMin.x - pFrom->x) / (pTo->x - pFrom->x);

		fCross[0] = (pTo->y - pFrom->y) * t + pFrom->y;

		if (fCross[0] > pObj->m_BBoxMin.y && fCross[0] < pObj->m_BBoxMax.y)
		{
			fCross[1] = (pTo->z - pFrom->z) * t + pFrom->z;

			if (fCross[1] > pObj->m_BBoxMin.z && fCross[1] < pObj->m_BBoxMax.z)
			{
				pPoint->x = pObj->m_BBoxMin.x;
				pPoint->y = fCross[0];
				pPoint->z = fCross[1];

				pPlane->m_Normal.Init(-1.0f, 0.0f, 0.0f);
				pPlane->m_Dist = pObj->m_BBoxMin.x * -1.0f;

				return 1;
			}
		}
	}
	else if (pFrom->x > pObj->m_BBoxMax.x)
	{
		if (pTo->x > pObj->m_BBoxMax.x)
			return 0;

		t = (pObj->m_BBoxMax.x - pFrom->x) / (pTo->x - pFrom->x);

		fCross[0] = (pTo->y - pFrom->y) * t + pFrom->y;

		if (fCross[0] > pObj->m_BBoxMin.y && fCross[0] < pObj->m_BBoxMax.y)
		{
			fCross[1] = (pTo->z - pFrom->z) * t + pFrom->z;

			if (fCross[1] > pObj->m_BBoxMin.z && fCross[1] < pObj->m_BBoxMax.z)
			{
				pPoint->x = pObj->m_BBoxMax.x;
				pPoint->y = fCross[0];
				pPoint->z = fCross[1];

				pPlane->m_Normal.Init(1.0f, 0.0f, 0.0f);
				pPlane->m_Dist = pObj->m_BBoxMin.x * 1.0f;

				return 1;
			}
		}
	}

	if (pFrom->y < pObj->m_BBoxMin.y)
	{
		if (pTo->y < pObj->m_BBoxMin.y)
			return 0;

		t = (pObj->m_BBoxMin.y - pFrom->y) / (pTo->y - pFrom->y);

		fCross[0] = (pTo->x - pFrom->x) * t + pFrom->x;

		if (fCross[0] > pObj->m_BBoxMin.x && fCross[0] < pObj->m_BBoxMax.x)
		{
			fCross[1] = (pTo->z - pFrom->z) * t + pFrom->z;

			if (fCross[1] > pObj->m_BBoxMin.z && fCross[1] < pObj->m_BBoxMax.z)
			{
				pPoint->y = pObj->m_BBoxMin.y;
				pPoint->x = fCross[0];
				pPoint->z = fCross[1];

				pPlane->m_Normal.Init(0.0f, -1.0f, 0.0f);
				pPlane->m_Dist = pObj->m_BBoxMin.y * -1.0f;

				return 1;
			}
		}
	}
	else if (pFrom->y > pObj->m_BBoxMax.y)
	{
		if (pTo->y > pObj->m_BBoxMax.y)
			return 0;

		t = (pObj->m_BBoxMax.y - pFrom->y) / (pTo->y - pFrom->y);

		fCross[0] = (pTo->x - pFrom->x) * t + pFrom->x;

		if (fCross[0] > pObj->m_BBoxMin.x && fCross[0] < pObj->m_BBoxMax.x)
		{
			fCross[1] = (pTo->z - pFrom->z) * t + pFrom->z;

			if (fCross[1] > pObj->m_BBoxMin.z && fCross[1] < pObj->m_BBoxMax.z)
			{
				pPoint->y = pObj->m_BBoxMax.y;
				pPoint->x = fCross[0];
				pPoint->z = fCross[1];

				pPlane->m_Normal.Init(0.0f, 1.0f, 0.0f);
				pPlane->m_Dist = pObj->m_BBoxMin.y * 1.0f;

				return 1;
			}
		}
	}

	if (pFrom->z < pObj->m_BBoxMin.z)
	{
		if (pTo->z < pObj->m_BBoxMin.z)
			return 0;

		t = (pObj->m_BBoxMin.z - pFrom->z) / (pTo->z - pFrom->z);

		fCross[0] = (pTo->x - pFrom->x) * t + pFrom->x;

		if (fCross[0] > pObj->m_BBoxMin.x && fCross[0] < pObj->m_BBoxMax.x)
		{
			fCross[1] = (pTo->y - pFrom->y) * t + pFrom->y;

			if (fCross[1] > pObj->m_BBoxMin.y && fCross[1] < pObj->m_BBoxMax.y)
			{
				pPoint->z = pObj->m_BBoxMin.z;
				pPoint->x = fCross[0];
				pPoint->y = fCross[1];

				pPlane->m_Normal.Init(0.0f, 0.0f, -1.0f);
				pPlane->m_Dist = pObj->m_BBoxMin.z * -1.0f;

				return 1;
			}
		}
	}
	else if (pFrom->z > pObj->m_BBoxMax.z)
	{
		if (pTo->z > pObj->m_BBoxMax.z)
			return 0;

		t = (pObj->m_BBoxMax.z - pFrom->z) / (pTo->z - pFrom->z);

		fCross[0] = (pTo->x - pFrom->x) * t + pFrom->x;

		if (fCross[0] > pObj->m_BBoxMin.x && fCross[0] < pObj->m_BBoxMax.x)
		{
			fCross[1] = (pTo->y - pFrom->y) * t + pFrom->y;

			if (fCross[1] > pObj->m_BBoxMin.y && fCross[1] < pObj->m_BBoxMax.y)
			{
				pPoint->z = pObj->m_BBoxMax.z;
				pPoint->x = fCross[0];
				pPoint->y = fCross[1];

				pPlane->m_Normal.Init(0.0f, 0.0f, 1.0f);
				pPlane->m_Dist = pObj->m_BBoxMin.z * 1.0f;

				return 1;
			}
		}
	}

	return 0;
}

// server 0x1000f12f.
inline uint8 IsectObject69190(LTVector* pFrom, LTVector* pTo, Obj69190* pObj)
{
	LTVector vPoint;
	LTPlane  Plane;
	float    fDistSqr;

	if (IsectObjectBounds69190(pFrom, pTo, pObj))
	{
		if (!g_10091150->m_FilterFn ||
		    g_10091150->m_FilterFn((HOBJECT)pObj, g_10091150->m_pUserData))
		{
			if (IsGeometryObject69190(pObj))
				return sub_10068c80(pObj);

			if (IsectObjectShape69190(pFrom, pTo, pObj, &vPoint, &Plane))
			{
				fDistSqr = vPoint.DistSqr(g_10091150->m_From);

				if (!g_10091160)
				{
					g_10091160 = (HOBJECT)pObj;
					g_10091158 = fDistSqr;
					g_10091164 = Plane;
					G_vPoint   = vPoint;
					g_10091180 = INVALID_HPOLY;

					return 1;
				}

				if (fDistSqr < g_10091158)
				{
					g_10091160 = (HOBJECT)pObj;
					g_10091158 = fDistSqr;
					g_10091164 = Plane;
					G_vPoint   = vPoint;
					g_10091180 = INVALID_HPOLY;

					return 1;
				}
			}
		}
	}

	return 0;
}


// ======================================================================= //
//  0x10069190 -- 2165 bytes.  w_IsectObjectCB (server 0x1000f0e8), the
//  per-object callback 0x10069a10 hands the root's segment walk.
//
//  Two gates: an object neither ray-hittable nor solid (+0x88 & 0x3000)
//  is skipped unless the query's bit 1 is clear (0x10091184); with bit 0
//  clear (0x10091188) the object's own +0x34 virtual is asked whether it
//  takes part.  Then the whole of w_IsectObject, inlined.
//
//  External linkage in a /Qip TU, yet ONE symbol (no stack-entry pair):
//  nothing in this TU calls it.
//
//  NEAR: 2168 bytes against 2165, 12 mnemonic blocks: every block, call
//  and store in order except that the -x face's success block is sunk
//  past the other faces where the shipped code keeps it in line (the
//  other five faces match), plus the two flag rejects' shared tail.
//  What got it from 2196 to 2168 (measured): the sphere reject written
//  POSITIVELY around the vOn test; the geometry test as one `||`
//  (`je arm ; cmp $9 ; jne shape`, where two `if`s sink the arm); the
//  bounds/filter/shape failures nested so they share ONE `return 0`
//  (the shipped `xor %edx,%edx` join); and the record block's stores in
//  the shipped order (object, range, plane, point, handle).
// ======================================================================= //

LTBOOL sub_10069190(Obj69190* pObj)
{
	if (!(pObj->m_Flags & (FLAG_RAYHIT | FLAG_SOLID)) && !g_10091184)
	{
		return LTFALSE;
	}
	else
	{
		if (!g_10091188)
		{
			if (!pObj->TakesPartInIsect())
				return LTFALSE;
		}

		return IsectObject69190(&g_10091150->m_From, &g_10091150->m_To, pObj);
	}
}
