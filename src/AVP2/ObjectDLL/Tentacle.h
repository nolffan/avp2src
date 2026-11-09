// ----------------------------------------------------------------------- //
//
// MODULE  : Tentacle.h
//
// PURPOSE : CTentacle - one of a CAITentacle's tentacles: it erupts from the
//           ground under its target through a CTentHole, lashes at it, and
//           burrows back
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (vtable 0x1028b354, ClassDef 0x102e4978)
//
// ----------------------------------------------------------------------- //

#ifndef __TENTACLE_H__
#define __TENTACLE_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"
#include "ltsmartlink.h"
#include "GameTimer.h"
#include "AIBase.h"

struct DamageStruct;

class CTentacle : public GameBase
{
	public :

		CTentacle();
		virtual ~CTentacle();

		// Our CAITentacle sends us after a target (PHObject.lto 0x1014d363)
		void	Erupt(HOBJECT hTarget);

		// Our CAITentacle asks whether we're down (not shown); the same
		// body as Turret::CanShootThrough, folded with it at 0x10092d30
		LTBOOL	IsHidden();

		virtual LTBOOL CanShootThrough()
		{
			return m_eState == eHidden || m_eState == eDead || m_eState == eDying;
		}

	protected :

		virtual uint32	ObjectMessageFn(HOBJECT hSender, uint32 messageID, HMESSAGEREAD hRead);

		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);
		virtual uint32	OnInitialUpdate(uint32 nInfo);
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);
		virtual uint32	OnModelStringKey(void *pData);
		virtual void	OnSave(HMESSAGEWRITE hWrite);
		virtual void	OnLoad(HMESSAGEREAD hRead);

	private :

		friend class CAITentacle;
		friend CTentacle* CreateTentacle(HOBJECT, const LTVector &, LTFLOAT, LTFLOAT, LTFLOAT);

		enum State
		{
			eHidden = 0,
			eWaiting,
			eErupting,
			eBurrowing,
			eAttacking,
			eUnused,
			eDying,
			eDead,
			ePain
		};

		void	UpdateHidden(LTFLOAT fFrameTime);
		void	UpdateWaiting(LTFLOAT fFrameTime);
		void	UpdateErupting(LTFLOAT fFrameTime);
		void	UpdateAttacking(LTFLOAT fFrameTime);
		void	UpdateBurrowing(LTFLOAT fFrameTime);
		void	UpdateUnused(LTFLOAT fFrameTime);
		void	UpdateDying(LTFLOAT fFrameTime);
		void	UpdateDead(LTFLOAT fFrameTime);
		void	UpdatePain(LTFLOAT fFrameTime);

		void	Hide();
		void	Erupt(const LTVector & vPos, const LTVector & vNormal);
		void	FaceTarget(const LTVector & vPos);
		void	Burrow();
		void	StartAttack();
		void	Die();
		void	Dead();
		void	Pain();
		void	ShowModel();
		void	HideModel();

		LTBOOL	FindGround(LTVector & vPos, LTVector & vNormal);
		void	HandleDamage(const DamageStruct & damage);
		LTBOOL	DoMeleeAttack();

		LTSmartLink		m_hOwner;			// 0x50: our CAITentacle
		int				m_eState;			// 0x58
		CGameTimer		m_tmrErupt;			// 0x5c
		CGameTimer		m_tmrBurrow;		// 0x60
		CGameTimer		m_tmrAttack;		// 0x64
		CGameTimer		m_tmrWait;			// 0x68
		CGameTimer		m_tmrPain;			// 0x6c
		CAIAnimator		m_Anim;				// 0x70
		LTSmartLink		m_hTarget;			// 0x84
		LTBOOL			m_bJoinOwner;		// 0x8c
		COrientationMgr	m_Orientation;		// 0x90
		LTSmartLink		m_hHole;			// 0xb8
		CAISounds		m_Sounds;			// 0xc0
		LTFLOAT			m_fHitPoints;		// 0xf0
		LTFLOAT			m_fDamage;			// 0xf4
		int				m_ePainState;		// 0xf8
		LTFLOAT			m_fScale;			// 0xfc
		LTVector		m_vNormal;			// 0x100
		LTBOOL			m_bDeath1;			// 0x10c: died the Death1 way, and hides
};

LTBOOL		IsTentacle(HOBJECT hObj);
CTentacle*	CreateTentacle(HOBJECT hOwner, const LTVector & vPos, LTFLOAT fHitPoints,
						   LTFLOAT fDamage, LTFLOAT fScale);

#endif // PRIMAL_HUNT

#endif // __TENTACLE_H__
