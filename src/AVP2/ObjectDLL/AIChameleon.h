// ----------------------------------------------------------------------- //
//
// MODULE  : AIChameleon.h
//
// PURPOSE : Primal Hunt's chameleon: a CAIBase that takes on the texture
//           of the surface under it, stalks, and leaps at its target
//
// ----------------------------------------------------------------------- //

// PH: Third Law's class (PHObject.lto, RTTI .?AVCAIChameleon@@, vtable
// 0x1028acf4, class definition 0x102e39b0: "CAIChameleon", parent CAIBase,
// size 0x46c). The file name is ours: retail does not record it. Member
// and method names are guesses, from how the code uses them.

#ifndef __AI_CHAMELEON_H__
#define __AI_CHAMELEON_H__

#ifdef PRIMAL_HUNT

#include "AIBase.h"
#include "SurfaceMgr.h"
#include "GameTimer.h"

// PH: object flags of Primal Hunt's engine, not in the SDK's ltbasedefs.h
// (names ours; ChameleonTest.cpp has them too)
#define FLAG2_CHAMELEON2	(1<<11)
#define FLAG2_CAMOBLEND		(1<<13)

// PH: the members below with constructors are constructed before the
// vtable pointer is stored (0x101485b3..0x10148615), which is what tells
// them apart from the members the constructor's body sets.

// PH: how far the target is, and the nearest and farthest it has been
// (CAIBase has one at 0x188, set up the same)
struct CAIDistTracker
{
	CAIDistTracker()
	{
		m_fMin	= 9999999.0f;
		m_fMax	= -9999999.0f;
		m_fDist	= 0.0f;
		m_nAxes	= 5;
	}

	LTFLOAT	m_fMin;
	LTFLOAT	m_fMax;
	LTFLOAT	m_fDist;
	uint32	m_nAxes;	// which of x, y and z the distance measures (bits)
};

// PH: how long the player has been looking at the chameleon
struct CAIStareCheck
{
	CAIStareCheck()
	{
		m_fMaxTime	= 1.0f;
		m_fFOV		= 0.5183628f;
		m_fTime		= 0.0f;
	}

	LTFLOAT	m_fMaxTime;
	LTFLOAT	m_fFOV;		// radians either side of the player's aim
	LTFLOAT	m_fTime;
};

class CAIChameleon : public CAIBase
{
	public :

		CAIChameleon();
		~CAIChameleon();

	protected :

		enum CamoState
		{
			eCamoOff = 0,	// its own skin
			eCamoOn,		// the surface's texture
			eCamoFading		// fading back to its own skin
		};

		enum AttackState
		{
			eAttackAlert = 0,
			eAttackChase,
			eAttackSwipe,
			eAttackPounce,
			eAttackLeap
		};

		// GameBase
		uint32	OnPrecreate(void *pData, uint32 nInfo);
		uint32	OnInitialUpdate(uint32 nInfo);
		uint32	OnUpdate(LTFLOAT fFrameTime);
		uint32	OnModelStringKey(void *pData);
		void	OnSave(HMESSAGEWRITE hWrite);
		void	OnLoad(HMESSAGEREAD hRead);

		// CAIBase
		void	SetVisionMode(int nMode);
		void	UpdateIdle(LTFLOAT fFrameTime);
		void	UpdateAlert(LTFLOAT fFrameTime);
		LTBOOL	SetStateIdle(const AIBaseStateInfo* pInfo);
		LTBOOL	SetStateAlert(const AIBaseStateInfo* pInfo);
		LTBOOL	SetStateDead(const AIBaseStateInfo* pInfo);
		LTBOOL	SetStatePain(const AIBaseStateInfo* pInfo);
		void	HandleDamage(const DamageStruct& damage);
		LTBOOL	HasMoveTarget();
		LTBOOL	ReturnToLeash();
		void	HideHead();

	private :

		LTBOOL	FindSurface(const LTVector& vFrom, const LTVector& vDir, LTFLOAT fDist, IntersectInfo* pInfo);
		LTBOOL	CanBlendWith(SurfaceType eSurface);
		void	UpdateSurface(LTFLOAT fFrameTime);
		void	TakeTexture(const char* szTexture);
		void	FinishCamo();
		void	UpdateCamo(LTFLOAT fFrameTime);
		void	ResetHeadSkin();
		void	UpdateUncamo();

		LTBOOL	StartAlert();
		LTBOOL	StartChase();
		LTBOOL	StartSwipe();
		LTBOOL	StartPounce();
		LTBOOL	StartLeap();

		void	UpdateAttackAlert(LTFLOAT fFrameTime);
		void	UpdateChase(LTFLOAT fFrameTime);
		void	UpdateSwipe(LTFLOAT fFrameTime);
		void	UpdatePounce(LTFLOAT fFrameTime);
		void	UpdateLeap(LTFLOAT fFrameTime);

		LTBOOL	InSwipeRange();
		LTBOOL	CanLeap();

		LTBOOL	Swipe();

		void	StartToCrouch();
		void	StartCrouchIdle();
		void	UpdateToCrouch(LTFLOAT fFrameTime);
		void	UpdateCrouchIdle(LTFLOAT fFrameTime);

		LTFLOAT			m_fNextSurfaceCheck;	// 0x384
		CamoState		m_eCamoState;			// 0x388
		LTFLOAT			m_fCamoTime;			// 0x38c
		LTBOOL			m_bCamoDone;			// 0x390
		char			m_szTexture[128];		// 0x394
		LTBOOL			m_bResetHeadSkin;		// 0x414
		CAIDistTracker	m_TargetDist;			// 0x418
		LTBOOL			m_bWaitToAttack;		// 0x428
		LTFLOAT			m_fWaitAttackDist;		// 0x42c
		AttackState		m_eAttackState;			// 0x430
		LTFLOAT			m_fSwipeRange;			// 0x434
		CGameTimer		m_tmrSwipe;				// 0x438
		CGameTimer		m_tmrAlert;				// 0x43c
		CAIStareCheck	m_Stare;				// 0x440
		LTBOOL			m_bCrouchIdle;			// 0x44c
		CGameTimer		m_tmrToCrouch;			// 0x450
		CGameTimer		m_tmrIdleSound;			// 0x454
		LTFLOAT			m_fLeapMinDist;			// 0x458
		LTFLOAT			m_fLeapMaxDist;			// 0x45c
		CGameTimer		m_tmrLeap;				// 0x460
		CGameTimer		m_tmrNextLeap;			// 0x464
		CGameTimer		m_tmrPounce;			// 0x468
};

// 0x1014aad0: the swipe's ray filter; CTentacle uses it too
LTBOOL ChameleonSwipeFilterFn(HOBJECT hObj, void *pUserData);

#endif // PRIMAL_HUNT

#endif // __AI_CHAMELEON_H__
