// ----------------------------------------------------------------------- //
//
// MODULE  : ChameleonTest.h
//
// PURPOSE : ChameleonTest - Definition
//
//           Primal Hunt (Third Law): a heavy predator model with the
//           chameleon's environment map, for looking at the cloak effect.
//           No source shipped; written from PHObject.lto (class
//           registration 0x1014edc0..0x1014eede, vtable 0x1028af40).
//
// ----------------------------------------------------------------------- //

#ifndef __CHAMELEON_TEST_H__
#define __CHAMELEON_TEST_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"

class ChameleonTest : public GameBase
{
	public :

		ChameleonTest() : GameBase(OT_MODEL) { m_fRotation = 0.0f; }	// in 0x1014ee50

	protected :

		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);		// 0x1014eee0
		virtual uint32	OnInitialUpdate(uint32 nInfo);				// 0x1014efb0
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);				// 0x1014efe0

	private :

		LTFLOAT	m_fRotation;	// +0x50 (name ours)
};

#endif // PRIMAL_HUNT

#endif // __CHAMELEON_TEST_H__
