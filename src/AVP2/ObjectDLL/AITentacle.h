// ----------------------------------------------------------------------- //
//
// MODULE  : AITentacle.h
//
// PURPOSE : Primal Hunt's tentacle beast: a hidden CAIBase that sends its
//           tentacles (CTentacle) erupting out of the ground at its target
//
// ----------------------------------------------------------------------- //

// PH: Third Law's class (PHObject.lto, RTTI .?AVCAITentacle@@, vtable
// 0x1028ae08, class definition 0x102e3fa0: "CAITentacle", parent CAIBase,
// size 0x444). The file name is ours: retail does not record it. Member
// and method names are guesses, from how the code uses them.

#ifndef __AI_TENTACLE_H__
#define __AI_TENTACLE_H__

#ifdef PRIMAL_HUNT

#include "AIBase.h"
#include "GameTimer.h"

class CTentacle;

#define AITENTACLE_MAX_TENTACLES	32

class CAITentacle : public CAIBase
{
	friend class CTentacle;	// it calls ShakePlayer (0x1014d380)

	public :

		CAITentacle();
		~CAITentacle();

		LTBOOL	AddTentacle(CTentacle* pTentacle);

	protected :

		// GameBase
		uint32	OnPrecreate(void *pData, uint32 nInfo);
		uint32	OnInitialUpdate(uint32 nInfo);
		uint32	OnUpdate(LTFLOAT fFrameTime);
		void	OnSave(HMESSAGEWRITE hWrite);
		void	OnLoad(HMESSAGEREAD hRead);

		// CAIBase
		void	SetVisionMode(int nMode);
		void	HandleSound(HOBJECT hObj, const LTVector& vPos, LTFLOAT fVolume, LTFLOAT fRange, uint32 nType);
		LTBOOL	ChooseTarget();
		void	UpdateIdle(LTFLOAT fFrameTime);
		void	UpdateAlert(LTFLOAT fFrameTime);
		void	UpdateDying(LTFLOAT fFrameTime);
		LTBOOL	SetStateIdle(const AIBaseStateInfo* pInfo);
		LTBOOL	SetStateAlert(const AIBaseStateInfo* pInfo);
		LTBOOL	SetStateDying(const AIBaseStateInfo* pInfo);
		LTBOOL	SetStateDead(const AIBaseStateInfo* pInfo);

	private :

		LTBOOL	FindEruptPos(LTVector* pvPos, LTBOOL bInFront);
		LTBOOL	IsPosTaken(const LTVector& vPos, const LTVector& vDims);
		int		CountAttacking();
		LTBOOL	EruptTentacle(LTBOOL bInFront);
		void	ShakePlayer(const LTVector& vPos, const LTVector& vShake, LTFLOAT fRadius,
							LTFLOAT fDuration, LTFLOAT fMin, LTFLOAT fMax);
		LTBOOL	AllDead();

		CGameTimer	m_tmrRumble;							// 0x384
		CTentacle*	m_apTentacles[AITENTACLE_MAX_TENTACLES];	// 0x388
		int			m_nNumTentacles;						// 0x408
		int			m_nMaxAttacking;						// 0x40c
		LTBOOL		m_bJustLoaded;							// 0x410
		CGameTimer	m_tmrErupt;								// 0x414
		CGameTimer	m_tmrGiveUp;							// 0x418
		LTBOOL		m_bGivingUp;							// 0x41c
		LTFLOAT		m_fMinEruptTime;						// 0x420
		LTFLOAT		m_fMaxEruptTime;						// 0x424
		LTFLOAT		m_fTentacleHealth;						// 0x428
		LTFLOAT		m_fTentacleDamage;						// 0x42c
		LTFLOAT		m_fTentacleScale;						// 0x430
		LTBOOL		m_bFirstEruptInFront;					// 0x434
		LTVector	m_vPos;									// 0x438
};

LTBOOL			IsAITentacle(HOBJECT hObj);
CAITentacle*	GetAITentacle(HOBJECT hObj);

#endif // PRIMAL_HUNT

#endif // __AI_TENTACLE_H__
