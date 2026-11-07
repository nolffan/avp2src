// ----------------------------------------------------------------------- //
//
// MODULE  : LeashFace.h
//
// PURPOSE : CLeashFace - a convex face of a LeashVolume's hull
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x1014f6b0..0x1015024c. The file's and class's
//           names are ours (the object sits between ChameleonTest's and the
//           leash manager's in the link, and has no RTTI).
//
// ----------------------------------------------------------------------- //

#ifndef __LEASH_FACE_H__
#define __LEASH_FACE_H__

#ifdef PRIMAL_HUNT

#include "ltplane.h"
#include "LinkList.h"

#define LEASHFACE_MAX_VERTS		32

class CLeashFace
{
	public :

		CLeashFace() {}
		~CLeashFace();												// 0x1014f6b0

		void	CalcBounds();										// 0x1014f7a0
		LTBOOL	IsFacingNormal();									// 0x1014f840
		void	BuildEdgePlanes();									// 0x1014f9d0
		LTBOOL	Merge(CLeashFace* pOther);							// 0x1014fbe0
		int		RemoveCollinearVerts();								// 0x1014ff80

		TLinkList<LTVector*>	m_Verts;		// +0x00
		LTPlane					m_Plane;		// +0x10
		LTVector				m_vMin;			// +0x20
		LTVector				m_vMax;			// +0x2c
		TLinkList<LTPlane*>		m_EdgePlanes;	// +0x38, facing out of the face
};

#endif // PRIMAL_HUNT

#endif // __LEASH_FACE_H__
