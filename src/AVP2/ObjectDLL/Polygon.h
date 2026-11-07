// ----------------------------------------------------------------------- //
//
// MODULE  : Polygon.h
//
// PURPOSE : CPolygon - a convex polygon kept by value, clipped by planes
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10155610..0x10155b45. The file's and class's
//           names are ours (the object sits between COrientationMgr's and
//           CTentacle's in the link, and has no RTTI). The leash volumes
//           cut their portals with it.
//
// ----------------------------------------------------------------------- //

#ifndef __POLYGON_H__
#define __POLYGON_H__

#ifdef PRIMAL_HUNT

#include "ltplane.h"

#define POLYGON_MAX_VERTS	32

class CPolygon
{
	public :

		LTBOOL	Clip(LTPlane* pPlane, LTFLOAT fEpsilon);							// 0x10155650
		void	GetBounds(LTVector* pvMin, LTVector* pvMax);						// 0x10155900
		LTBOOL	GetSpan(LTVector* pvStart, LTVector* pvEnd, LTFLOAT* pfHeight);	// 0x101559b0

		int			m_nVerts;
		LTVector	m_Verts[POLYGON_MAX_VERTS];
};

#endif // PRIMAL_HUNT

#endif // __POLYGON_H__
