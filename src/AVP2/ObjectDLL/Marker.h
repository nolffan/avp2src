// ----------------------------------------------------------------------- //
//
// MODULE  : Marker.h
//
// PURPOSE : CMarker - a model that marks a spot for a while, then goes
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10153c00..0x10153e36 (vtable 0x1028b10c).
//           The file's name, and the member names, are ours.
//
// ----------------------------------------------------------------------- //

#ifndef __MARKER_H__
#define __MARKER_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"

// What the marker's creator hands it
struct MarkerCreateStruct : public ObjectCreateStruct
{
	MarkerCreateStruct() { m_nType = 0; m_fLifeTime = 0.0f; }

	uint32		m_nType;		// +0x40c
	LTFLOAT		m_fLifeTime;	// +0x410
};

class CMarker : public GameBase
{
	public :

		CMarker() : GameBase(OT_MODEL) { m_nType = 0; m_fLifeTime = 0.0f; }	// in 0x10153c90

	protected :

		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);		// 0x10153d50
		virtual uint32	OnInitialUpdate(uint32 nInfo);				// 0x10153da0
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);				// 0x10153db0

		virtual void	Save(HMESSAGEWRITE hWrite);					// 0x10153dd0
		virtual void	Load(HMESSAGEREAD hRead);					// 0x10153e00

	private :

		uint32		m_nType;		// +0x50
		LTFLOAT		m_fLifeTime;	// +0x54
};

#endif // PRIMAL_HUNT

#endif // __MARKER_H__
