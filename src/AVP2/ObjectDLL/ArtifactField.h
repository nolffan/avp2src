// ----------------------------------------------------------------------- //
//
// MODULE  : ArtifactField.h
//
// PURPOSE : Primal Hunt's artifact field: a world model that spins and
//           pulses its alpha, faded on and off by trigger messages
//
// ----------------------------------------------------------------------- //

// PH: Third Law's class (PHObject.lto, RTTI .?AVCArtifactField@@, vtable
// 0x1028aef4, class name "CArtifactField"). The file name is ours: retail
// does not record it.

#ifndef __ARTIFACT_FIELD_H__
#define __ARTIFACT_FIELD_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"

class CArtifactField : public GameBase
{
	public :

		CArtifactField();
		~CArtifactField();

	protected :

		enum FieldState
		{
			eStateOff = 0,
			eStateTurningOn,
			eStateOn,
			eStateTurningOff
		};

		uint32	OnPrecreate(void *pData, uint32 nInfo);
		uint32	OnInitialUpdate(uint32 nInfo);
		uint32	OnUpdate(LTFLOAT fFrameTime);
		uint32	OnTrigger(HOBJECT hSender, const char* szMsg);

	private :

		void	TurnOn(LTFLOAT fTime);
		void	TurnOff(LTFLOAT fTime);

		void	UpdateField(LTFLOAT fFrameTime);
		void	UpdateOff(LTFLOAT fFrameTime);
		void	UpdateTurningOn(LTFLOAT fFrameTime);
		void	UpdateTurningOff(LTFLOAT fFrameTime);

		void	SetOff();
		void	SetOn();

		LTFLOAT		m_fAlphaCycleTime;	// 0x50
		LTFLOAT		m_fTime;			// 0x54
		LTFLOAT		m_fMinAlpha;		// 0x58
		LTFLOAT		m_fMaxAlpha;		// 0x5c
		// PH: a member with a constructor: retail stores these two before
		// the vtable pointer (0x1014e134), as it does a member object's
		struct Blend
		{
			Blend() { m_bAdditive = LTTRUE; m_bAdditiveAlpha = LTTRUE; }
			LTBOOL	m_bAdditive;
			LTBOOL	m_bAdditiveAlpha;
		};
		Blend		m_Blend;			// 0x60
		LTFLOAT		m_fSpinRate;		// 0x68: radians per second
		LTBOOL		m_bSurge;			// 0x6c
		FieldState	m_eState;			// 0x70
		LTBOOL		m_bStartOn;			// 0x74
		LTFLOAT		m_fTransitionEnd;	// 0x78
		LTFLOAT		m_fTransitionTime;	// 0x7c
		LTFLOAT		m_fFade;			// 0x80
};

#endif // PRIMAL_HUNT

#endif // __ARTIFACT_FIELD_H__
