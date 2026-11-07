// ----------------------------------------------------------------------- //
//
// MODULE  : TentHole.h
//
// PURPOSE : CTentHole - the hole a CTentacle bursts out of: it opens, waits
//           while the tentacle is out, then closes and fades away
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (vtable 0x1028b3b0, ClassDef 0x102e4c40)
//
// ----------------------------------------------------------------------- //

#ifndef __TENT_HOLE_H__
#define __TENT_HOLE_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"
#include "GameTimer.h"

class CTentHole : public GameBase
{
	public :

		CTentHole();
		virtual ~CTentHole();

		static CTentHole* Spawn(const LTVector & vPos, const LTVector & vNormal, LTFLOAT fScale);

		void	Hide();				// 0x101585c0
		void	Open();				// 0x101585e0
		void	Close();			// 0x10158630
		void	FadeOut();			// 0x10158680

	protected :

		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);
		virtual uint32	OnInitialUpdate(uint32 nInfo);
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);

	private :

		enum State
		{
			eOpening = 0,
			eHidden,
			eOpen,
			eFading
		};

		CGameTimer	m_tmrClose;			// 0x50
		CGameTimer	m_tmrRemove;		// 0x54
		int			m_eState;			// 0x58
		CGameTimer	m_tmrOpen;			// 0x5c
		LTVector	m_vPos;				// 0x60
		LTVector	m_vUp;				// 0x6c
		LTFLOAT		m_fScale;			// 0x78
};

LTBOOL		IsTentHole(HOBJECT hObj);
CTentHole*	GetTentHole(HOBJECT hObj);

#endif // PRIMAL_HUNT

#endif // __TENT_HOLE_H__
