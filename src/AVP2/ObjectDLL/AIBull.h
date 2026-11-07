// ----------------------------------------------------------------------- //
//
// MODULE  : AIBull.h
//
// PURPOSE : CAIBull - Primal Hunt's charging beast, the buhlagh
//
// PH: no source of this survives. Written from PHObject.lto: CAIBull's RTTI
// (.?AVCAIBull@@, vtable 0x1028ac00), its class definition (0x102e33e8:
// "CAIBull", parent CAIBase, size 0x3c0) and its code, 0x10146370..0x101486e0.
// Member and method names are guesses.
//
// ----------------------------------------------------------------------- //

#ifndef __AI_BULL_H__
#define __AI_BULL_H__

#ifdef PRIMAL_HUNT

#include "AIBase.h"

class CAIBull : public CAIBase
{
	public :

		enum BullState
		{
			eBullWalk = 0,
			eBullCharge,
			eBullRam,
		};

		CAIBull();
		virtual ~CAIBull();

		// GameBase
		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);
		virtual uint32	OnInitialUpdate(uint32 nInfo);
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);
		virtual uint32	OnTouchNotify(void *pData, LTFLOAT fData);
		virtual uint32	OnModelStringKey(void *pData);
		virtual void	OnSave(HMESSAGEWRITE hWrite);
		virtual void	OnLoad(HMESSAGEREAD hRead);

		// CAIBase
		virtual void	SetVisionMode(int nMode);
		virtual void	UpdateIdle(LTFLOAT fFrameTime);
		virtual void	UpdateAlert(LTFLOAT fFrameTime);
		virtual LTBOOL	SetStateIdle(const AIBaseStateInfo* pInfo);
		virtual LTBOOL	SetStateAlert(const AIBaseStateInfo* pInfo);
		virtual LTBOOL	SetStateDying(const AIBaseStateInfo* pInfo);
		virtual void	HandleDamage(const DamageStruct& damage);
		virtual LTBOOL	SetStateMoving(const AIBaseStateInfo* pInfo);
		virtual LTBOOL	HasMoveTarget();
		virtual LTBOOL	ReturnToLeash();

	protected :

		void	UpdateWalk(LTFLOAT fFrameTime);
		void	UpdateCharge(LTFLOAT fFrameTime);
		void	UpdateRam(LTFLOAT fFrameTime);
		void	StartWalk();
		void	StartCharge();
		void	StartRam();
		void	LowerHead();
		void	RaiseHead();

	protected :

		int			m_eBullState;			// 0x384
		LTFLOAT		m_fMinDist;				// 0x388
		LTFLOAT		m_fMaxDist;				// 0x38c
		LTFLOAT		m_fDist;				// 0x390
		uint32		m_nDistAxes;			// 0x394
		uint32		m_nHeadTracker;			// 0x398
		LTBOOL		m_bHeadDown;			// 0x39c
		LTFLOAT		m_fNextChargeTime;		// 0x3a0
		LTFLOAT		m_fChargeSpeed;			// 0x3a4
		LTFLOAT		m_fWalkSpeed;			// 0x3a8
		LTFLOAT		m_fChargeTurnRate;		// 0x3ac
		LTFLOAT		m_fTurnRate;			// 0x3b0
		LTFLOAT		m_fNextSightSound;		// 0x3b4
		LTFLOAT		m_fNextIdleSound;		// 0x3b8
		LTFLOAT		m_fNextImpactTime;		// 0x3bc
};

#endif // PRIMAL_HUNT

#endif // __AI_BULL_H__
