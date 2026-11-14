// ----------------------------------------------------------------------- //
//
// MODULE  : CAutoTargetMgr.h
//
// PURPOSE : CAutoTargetMgr class definition
//
// CREATED : 6/15/2000
//
// ----------------------------------------------------------------------- //

#ifndef __AUTO_TARGET_MGR_H__
#define __AUTO_TARGET_MGR_H__


class CPlayerStats;
class CSFXMgr;

enum TargetingType
{
	TT_NORMAL = 0,
	TT_PRED_SHOULDER_CANNON,
	TT_ALIEN,
	TT_MARINE_SMARTGUN,
	TT_MARINE_SADAR,
	TT_PRED_DISK,
	TT_FACEHUGGER,
	TT_CHESTBURSTER,
	TT_PREDALIEN,
	TT_MARINE_SADAR_NORMAL,
	TT_RUNNER,
};

enum AlienWeaponMode
{
	AWM_NORMAL = 0,
	AWM_HEAD_BITE,
	AWM_TAIL,
	AWM_TEAR,
};

enum FacehuggerWeaponMode
{
	FHWM_NORMAL = 0
};

enum ChestbursterWeaponMode
{
	CBWM_NORMAL = 0
};

enum PredalienWeaponMode
{
	PAWM_NORMAL = 0,
#ifdef PRIMAL_HUNT
	PAWM_TEAR,				// (Primal Hunt's tear is set 1: PHCShell 0x10035f25)
	PAWM_TAIL,
#else
	PAWM_TAIL,
	PAWM_TEAR,
#endif
};

enum RunnerWeaponMode
{
	RWM_NORMAL = 0,
	RWM_HEAD_BITE,
	RWM_TAIL,
	RWM_TEAR,
};

class CAutoTargetMgr
{
public:
	//ctor/dtor
	CAutoTargetMgr	();
	~CAutoTargetMgr	();

	void			UpdateTargets();
	void			Init( TargetingType eType );
	TargetingType	GetTargetingType () { return m_eType; }
	HOBJECT			GetAlienTarget() { return m_hBiteOrPounceTarget;}
	void			ResetSounds();

private:

	void	UpdateAlienTargeting		();
	void	UpdatePredShoulderCannon	();
	void	UpdateMarineSmartgun		();
	void	UpdateMarineSADAR			();
	void	UpdateFacehuggerTargeting	();
	void	UpdateChestbursterTargeting	();
	void	UpdatePredalienTargeting	();
	void	UpdateRunnerTargeting		();

	LTBOOL	IsValidAlienTarget			(	HOBJECT hTarget			);
	LTBOOL	IsValidFacehuggerTarget		(	HOBJECT hTarget			);

	LTBOOL	CheckForLiveAlienTarget		(	HOBJECT		hTarget, 
											LTVector	vTargetLoc, 
											LTVector	vPlayerPos, 
											int			nNormalRngSqr,
											int			nBiteRngSqr,
											WEAPON*		pBiteWep,
											LTVector	vF			);

	LTBOOL	CheckForDeadAlienTarget		(	LTVector	vPlayerPos,
											int			nBiteRngSqr,
											LTVector	vF,
											WEAPON*		pBiteWep);

	LTBOOL	CheckForTearTarget(IntersectInfo IInfo, LTVector vPos, uint32 nRngSq);

	LTBOOL	CheckForLiveFacehuggerTarget(	HOBJECT		hTarget,
											LTVector	vTargetLoc,
											LTVector	vPlayerPos,
											int			nNormalRngSqr,
											WEAPON*		pLungeWep,
											LTVector	vF			);

#ifdef PRIMAL_HUNT
	// Primal Hunt: written inline, so UpdateChestbursterTargeting (the body
	// that is UpdatePredalienTargeting in AvP2, AutoTargetMgr.cpp) expands it
	// (it has no out-of-line copy in PHCShell.dll)
	LTBOOL	CheckForLivePredalienTarget	(	HOBJECT		hTarget, 
											LTVector	vTargetLoc, 
											LTVector	vPlayerPos, 
											int			nNormalRngSqr,
											LTVector	vF			)	{ return LTFALSE; }
#else
	LTBOOL	CheckForLivePredalienTarget	(	HOBJECT		hTarget, 
											LTVector	vTargetLoc, 
											LTVector	vPlayerPos, 
											int			nNormalRngSqr,
											LTVector	vF			);
#endif

	LTBOOL	CheckForLungeBounds(	LTVector	vPlayerPos, 
									int			nNormalRngSqr,
									BARREL*		pLungeBarrel,
									LTVector	vF);
	TargetingType	m_eType;

	HOBJECT			m_hBiteOrPounceTarget;
	LTVector		m_vPounceTargetLoc;
	HOBJECT			m_hPotentialTarget;
	LTFLOAT			m_fNewTargetStartTime;
	LTBOOL			m_bLockSoundPlayed;

	CSFXMgr*		m_psfxMgr;
	CPlayerStats*	m_pStats;	
	HLTSOUND		m_hLockedLoopSound;
	LTFLOAT			m_fLastBiteSound;
};	

#endif // __AUTO_TARGET_MGR_H__

