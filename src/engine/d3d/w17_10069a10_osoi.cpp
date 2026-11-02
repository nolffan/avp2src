// *********************************************************************** //
//
//  w17_10069a10_osoi.cpp -- 0x10069a10, the world segment intersector.
//  /Os /Oi- (see build/engine.sh).
//
//  WHY THESE FLAGS.  The shipped function is preceded by 0x90 padding
//  (the /Os pad), opens `push %edi ; push %edi` (the /Os frame-slot push),
//  and its magnitude is `fstpl (%esp) ; call 0x1007fc98` -- the CRT sqrt
//  CALLED, which is /Oi-.
//
//  WHAT IT DOES.  The caller (w3_10019644_osoi.cpp's 0x1001be00: query,
//  info, the world's intersect root at +0x6c, 0) hands over an
//  IntersectQuery.  The state block at 0x10091130..0x10091194 is the
//  current segment: its squared length + 1, the flags as two bytes, the
//  unit direction and length, the plane through the start perpendicular
//  to it, the per-node callback (0x10069060 when INTERSECT_HPOLY (4) is
//  set, else 0x10069110) and the best hit so far (object, plane, point,
//  poly).  A direction that does not normalise to a length^2 in [0.5, 2]
//  (a degenerate segment) answers FALSE.  The root's virtual +0x10 walks
//  the tree with 0x10069190; a hit fills the IntersectInfo -- point,
//  plane, object, poly, and the surface flags of the hit poly's surface
//  (0 with no poly).
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// The intersect root's walker (Src19d64Grid +0x6c): virtual +0x10 walks
// the segment from..to calling pfnNode on each node.
class CIntersectRoot
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual void	Slot0c();
	virtual void	Walk(LTVector* pFrom, LTVector* pTo, void* pfnNode,
	                     void* pUser1, void* pUser2);    // +0x10
};

struct Surface10069a10
{
	uint8		m_pad_00[0x3c];
	uint16		m_nFlags3c;                             // +0x3c
};

struct Poly10069a10
{
	uint8				m_pad_00[0x28];
	Surface10069a10*	m_pSurface28;                   // +0x28
};

// The per-poly callbacks and the node walker, in the spellings their
// definitions use: 0x10069110 is d3dr5b.cpp's, 0x10069060 is written
// below.
struct Caster10069110;
struct D3DVec3;
extern void sub_10069110(Caster10069110* pCaster, void** ppHit, D3DVec3* pFrom,
                         float* pfDistSqr, int32* pnIndex, D3DVec3* pTo,
                         uint32 nFlags);                // 0x10069110
extern void sub_10069190();                             // 0x10069190

struct D3DVec3
{
	float	x, y, z;
};

// w8_1006a5f0_osoiqip.cpp's segment query and its caster, the pieces used.
struct Query1006a5f0;
extern LTBOOL sub_1006a5f0(void* pNode, Query1006a5f0* pQuery);   // 0x1006a5f0

struct Query10069060
{
	D3DVec3*		m_pFrom;                            // +0x00
	D3DVec3*		m_pTo;                              // +0x04
	D3DVec3*		m_pIPoint;                          // +0x08
	void*			m_pFilter0c;                        // +0x0c
	void*			m_pUser10;                          // +0x10
	void*			m_pHit14;                           // +0x14
};

class CCaster10069060
{
public:
	virtual void	Slot00();
	virtual void*	GetWorld();                         // +0x04
	virtual int32	GetPolyIndex(void* pHit);           // +0x08
};

// The current segment (0x10091130..0x10091194).
// The three vectors are declared as a plain struct (the data-table
// generator's demangler does not take the _CVector template) and used as LTVectors.
struct Vec10069a10
{
	float	x, y, z;
};

extern Vec10069a10		g_10091130;                     // 0x10091130 plane normal
extern float			g_1009113c;                     // 0x1009113c plane dist
extern float			g_10091140;                     // 0x10091140 length
extern Vec10069a10		g_10091144;                     // 0x10091144 direction
extern IntersectQuery*	g_10091150;                     // 0x10091150
extern void*			g_10091154;                     // 0x10091154 node callback
extern float			g_10091158;                     // 0x10091158 length^2 + 1
extern Poly10069a10**	g_1009115c;                     // 0x1009115c hit poly
extern HOBJECT			g_10091160;                     // 0x10091160 hit object
extern LTPlane			g_10091164;                     // 0x10091164 hit plane
extern Vec10069a10		g_10091174;                     // 0x10091174 hit point

#define G_vNormal	(*(LTVector*)&g_10091130)
#define G_vDir		(*(LTVector*)&g_10091144)
#define G_vPoint	(*(LTVector*)&g_10091174)
extern HPOLY			g_10091180;                     // 0x10091180 hit poly handle
extern uint8			g_10091184;                     // 0x10091184 !(flags & 2)
extern uint8			g_10091188;                     // 0x10091188 flags & 1
extern uint32			g_10091190;                     // 0x10091190 query count
extern uint32			g_10091194;                     // 0x10091194


// ======================================================================= //
//  0x10069060 -- 168 bytes.  The INTERSECT_HPOLY node callback: run the
//  node's world segment query (0x1006a5f0) with the current IntersectQuery
//  as its filter; a hit hands back the poly, its index (the caster's
//  +0x08) and the squared distance from the query's start to the hit
//  point, a miss -1 and NULL.  d3dr5b.cpp's 0x10069110 is the object
//  sibling with the same seven arguments.
//
//  NEAR, 8 bytes (was 46).  The distance is (from - hit)
//  summed through a local copy of the query pointer taken BEFORE *ppHit is
//  stored (the shipped code loads 0x10091150 there), and the six query
//  stores are in the one order of all 720 that comes closest.  What is
//  left is the scheduling of the 0x10091150 load against the +0x10 and
//  +0x08 stores: shipped stores +0x00, +0x10, loads the global, stores
//  +0x08, +0x04, +0x0c; ours loads it right after +0x00.  Measured and
//  worse: one field stored through a pointer (28+), a volatile read of the
//  global (136), a constructor taking the six values in any of the best
//  forty orders (8 at best), a constructor zeroing +0x14 only (24+), and
//  the flag sets /Os, /Os /Oi- /Qip, /O2 /Oi-, /Os /Oi- /G5.  Also: an
//  aggregate initialiser `= { pFrom, pTo, pIPoint, g_10091150, pCaster,
//  LTNULL }` copies through a template-like sequence (189 off).
// ======================================================================= //
void sub_10069060(CCaster10069060* pCaster, void** ppHit, D3DVec3* pIPoint,
                  float* pfDistSqr, int32* pnIndex, D3DVec3* pFrom,
                  D3DVec3* pTo)
{
	Query10069060	query;

	query.m_pUser10 = pCaster;
	query.m_pFilter0c = g_10091150;
	query.m_pIPoint = pIPoint;
	query.m_pTo = pTo;
	query.m_pFrom = pFrom;
	query.m_pHit14 = LTNULL;

	if (!sub_1006a5f0(pCaster->GetWorld(), (Query1006a5f0*)&query))
	{
		*pnIndex = -1;
		*ppHit = LTNULL;
	}
	else
	{
*pnIndex = pCaster->GetPolyIndex(query.m_pHit14);
IntersectQuery* pQ = g_10091150;
*ppHit = query.m_pHit14;
*pfDistSqr = (pQ->m_From.x - query.m_pIPoint->x) * (pQ->m_From.x - query.m_pIPoint->x) +
			(pQ->m_From.y - query.m_pIPoint->y) * (pQ->m_From.y - query.m_pIPoint->y) +
			(pQ->m_From.z - query.m_pIPoint->z) * (pQ->m_From.z - query.m_pIPoint->z);
	}
}


// ======================================================================= //
//  0x10069a10 -- NEAR, 70 bytes (was 431).  The query's
//  flags are read ONCE, right after the query store, into a local: the
//  shipped code loads +0x24 into %eax before the range-squared store and
//  tests both bits off that one load (`sete %dl` / `setne %cl`), where
//  reading the member at each test reloads it after the first byte store
//  (a char store may alias it) and shifts every later instruction.  The
//  two byte stores in the order g_10091184, g_10091188 (72; the other
//  order 76).  fDot is declared before fInv: the declaration order picks
//  the operands of the commutative `fInv * fDot` (shipped `fldl fInv ;
//  fmull fDot`; the other order, or `fDot * fInv`, swaps them -- the same
//  lever closed 0x1006a860), 72 -> 70.  What is left is scheduling: the
//  shipped code stores g_10091188 before g_10091184 and interleaves the
//  vDir subtraction with them, puts the g_10091150 store after the first
//  `flds`, the -1 store after the two zero stores, and loads g_10091180
//  before the +0x1c store; all 720 orders of the six locals measured, 70
//  at best.  Unchanged by: the three hit-state stores in any of six
//  orders, the flag stores after the vDir subtraction (405) or split
//  around the hit-state stores, `(nFlags & 2) == 0`, the query store
//  after the flags read, `!((nFlags >> 1) & 1)` (411; server.dll's
//  w_IntersectSegment twin's spelling), the m_hPoly copy first.
// ======================================================================= //
extern "C" LTBOOL sub_10069a10(IntersectQuery* pQuery, IntersectInfo* pInfo,
                               CIntersectRoot* pRoot, uint32 nUnused)
{
	LTVector	vDir;
	LTVector	vNormal;
	float		fMagSqr;
	float		fDot;
	float		fInv;
	uint32		nFlags;

	g_10091190++;
	g_10091150 = pQuery;
	nFlags = pQuery->m_Flags;

	g_10091158 = (pQuery->m_From - pQuery->m_To).MagSqr() + 1.0f;
	g_10091184 = !(nFlags & 2);
	g_10091188 = (nFlags & 1) != 0;

	vDir = pQuery->m_To - pQuery->m_From;

	g_1009115c = LTNULL;
	g_10091160 = LTNULL;
	g_10091180 = INVALID_HPOLY;

	G_vDir = vDir;
	g_10091140 = (float)sqrt(G_vDir.MagSqr());
	G_vDir *= 1.0f / g_10091140;

	fMagSqr = G_vDir.MagSqr();
	if (fMagSqr < 0.5f || fMagSqr > 2.0f)
		return LTFALSE;

	fDot = G_vDir.Dot(pQuery->m_From);
	fInv = 1.0f / fMagSqr;
	vNormal = G_vDir * fInv;
	G_vNormal = vNormal;
	g_1009113c = fInv * fDot;

	if (pQuery->m_Flags & 4)
		g_10091154 = (void*)sub_10069060;
	else
		g_10091154 = (void*)sub_10069110;

	pRoot->Walk(&pQuery->m_From, &pQuery->m_To, (void*)sub_10069190,
	            LTNULL, LTNULL);

	*(volatile uint32*)&g_10091194 = *(volatile uint32*)&g_10091194;

	if (!g_10091160)
		return LTFALSE;

	pInfo->m_Point = G_vPoint;
	pInfo->m_Plane = g_10091164;
	pInfo->m_hObject = g_10091160;
	pInfo->m_hPoly = g_10091180;

	if (g_1009115c)
		pInfo->m_SurfaceFlags = (*g_1009115c)->m_pSurface28->m_nFlags3c;
	else
		pInfo->m_SurfaceFlags = 0;

	return LTTRUE;
}
