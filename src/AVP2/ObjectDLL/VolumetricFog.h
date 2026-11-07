// ----------------------------------------------------------------------- //
//
// MODULE  : VolumetricFog.h
//
// PURPOSE : VolumetricFog - a world model that the client draws as a volume
//           of fog, with its own color and density and an alpha that can be
//           faded by message
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (vtable 0x1028b4b8, ClassDef 0x102e4ed0)
//
// ----------------------------------------------------------------------- //

#ifndef __VOLUMETRIC_FOG_H__
#define __VOLUMETRIC_FOG_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"

class VolumetricFog : public GameBase
{
	public :

		VolumetricFog();
		virtual ~VolumetricFog();

	protected :

		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);
		virtual uint32	OnInitialUpdate(uint32 nInfo);
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);
		virtual uint32	OnTrigger(HOBJECT hSender, const char* szMsg);

	private :

		LTBOOL		m_bFading;			// 0x50
		LTFLOAT		m_fFadeTimer;		// 0x54
		LTFLOAT		m_fFadeTime;		// 0x58
		LTFLOAT		m_fStartAlpha;		// 0x5c
		LTFLOAT		m_fEndAlpha;		// 0x60
		LTVector	m_vFogColor;		// 0x64
		LTFLOAT		m_fFogDensity;		// 0x70
};

#endif // PRIMAL_HUNT

#endif // __VOLUMETRIC_FOG_H__
