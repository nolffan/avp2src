// ----------------------------------------------------------------------- //
//
// MODULE  : AIBase.h
//
// PURPOSE : CAIBase - Third Law's lightweight AI for Primal Hunt (the bull,
//           the chameleon, the tentacles): a GameBase, not a CCharacter
//
// PH: no source of this survives. Written from PHObject.lto: CAIBase's RTTI
// (.?AVCAIBase@@, vtable 0x1028ab00, 56 slots), its class definition
// (0x102e31f0: "CAIBase", parent GameBase, size 0x384) and its code,
// 0x10141bd0..0x10146370. Member and method names are guesses, from how
// the code uses them.
//
// ----------------------------------------------------------------------- //

#ifndef __AI_BASE_H__
#define __AI_BASE_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"
#include "ltsmartlink.h"
#include "SenseLevels.h"
#include "Destructible.h"
#include "LinkList.h"
#include "LeashMgr.h"

class CAIBase;
class CPlayerObj;

// ----------------------------------------------------------------------- //
//	The parts of a CAIBase. Each is Third Law's own class; their code lies
//	outside 0x10141000..0x10148000 (the other ranges of the new code), so
//	only what CAIBase's code needs of them is declared here.
// ----------------------------------------------------------------------- //

// PH: CAIBase::m_Sight (0x50, 0x44 bytes); its code is AISight.cpp's
// (CanSee 0x1015af50, Save 0x1015b390, Load 0x1015b430)
class CAISight
{
	public :

		CAISight()
		{
			m_fFOV			= 180.0f;
			m_bEnabled		= LTTRUE;
			m_fUpdateRate	= 0.25f;
			m_fCosHalfFOV	= (LTFLOAT)cos(MATH_DEGREES_TO_RADIANS(m_fFOV) * 0.5f);
			m_fRange		= 2048.0f;
			m_fFudge		= 0.15f;
			m_fNextUpdate	= g_pLTServer->GetTime() + 0.25f;
		}

		// PH: it has one, empty: CAIBase's destructor (0x10142840)
		// counts it among the members it unwinds
		~CAISight() {}

		void	SetFOV(LTFLOAT fFOV)
		{
			m_fFOV			= fFOV;
			m_fCosHalfFOV	= (LTFLOAT)cos(MATH_DEGREES_TO_RADIANS(m_fFOV) * 0.5f);
		}

		// nFlags is unused: the ray always hits objects and skips the
		// non-solid ones
		LTBOOL	CanSee(HOBJECT hObj, const LTVector& vPos, const LTVector& vForward, int nFlags,
					   ObjectFilterFn fnFilter, PolyFilterFn fnPolyFilter, void* pUserData, LTFLOAT* pfDist);

		void	Save(HMESSAGEWRITE hWrite);
		void	Load(HMESSAGEREAD hRead);

		LTFLOAT			m_fUpdateRate;		// 0x50
		LTFLOAT			m_fNextUpdate;		// 0x54
		LTFLOAT			m_fFOV;				// 0x58
		LTFLOAT			m_fCosHalfFOV;		// 0x5c
		LTBOOL			m_bEnabled;			// 0x60
		LTFLOAT			m_fRange;			// 0x64
		IntersectInfo	m_Target;			// 0x68, what the last look hit (its constructor, 0x101447e0, is out of line)
		LTFLOAT			m_fFudge;			// 0x90
};

// PH: CAIBase::m_Animator (0xa0, 0x14 bytes); its code is 0x1014d920..
// PH (n2): one of a CAIAnimator's animation trackers (0x58 bytes, made by
// 0x1014db40); the first is the model's main tracker, the rest its own
struct AITracker
{
	AITracker()
	{
		m_pTracker	= LTNULL;
		m_nID		= -1;
	}

	LTAnimTracker*	m_pTracker;	// 0x00
	int				m_nID;		// 0x04
	LTAnimTracker	m_Tracker;	// 0x08
};

class CAIAnimator
{
	public :

		CAIAnimator();
		~CAIAnimator();

		void	Clear();
		void	Init(HOBJECT hObj);
		HMODELANIM PlayAnim(const char* szAnim, LTBOOL bLoop, uint32 nTracker);
		void	PlayAnim(HMODELANIM hAnim, LTBOOL bLoop, uint32 nTracker);
		HMODELANIM GetAnim(int nTracker);
		HMODELANIM FindAnim(const char* szAnim);
		uint32	AddTracker();
		AITracker* FindTracker(int nTracker);
		void	SetAllowTransition(int nTracker, LTBOOL bAllow);
		uint32	FindWeightSet(const char* szName);
		void	SetWeightSet(uint32 hWeightSet, uint32 nTracker);
		void	SetWeightSet(const char* szWeightSet, uint32 nTracker)
		{
			uint32 hWeightSet = FindWeightSet(szWeightSet);
			SetWeightSet(hWeightSet, nTracker);
		}
		uint32	GetPlaybackState(int nTracker);
		LTFLOAT	GetAnimLength(int nTracker);
		LTBOOL	IsLooping(int nTracker);

		void	Save(HMESSAGEWRITE hWrite);
		void	Load(HMESSAGEREAD hRead);

	protected :

		TLinkList<AITracker*>	m_Trackers;	// 0x00
		HOBJECT					m_hObject;	// 0x10
};

// PH: CAIBase::m_MoveMgr (0xb4, 0x2c bytes), vtable 0x1028b1bc;
// constructor 0x10154130
class CMoveMgr
{
	public :

		CMoveMgr();														// 0x10154130
		virtual ~CMoveMgr();											// 0x101541a0

		virtual void	Update(HOBJECT hObj, LTFLOAT fFrameTime);		// 0x101541b0
		virtual void	UpdateWalk(HOBJECT hObj, LTFLOAT fFrameTime);	// 0x10154490
		virtual void	UpdateFly(HOBJECT hObj, LTFLOAT fFrameTime);	// 0x101547e0 (folded)
		virtual void	UpdateSwim(HOBJECT hObj, LTFLOAT fFrameTime);	// 0x101547e0 (folded)

		void		Accelerate(const LTVector& vDesired, const LTVector& vVel, LTVector* pvNewVel,
						LTFLOAT fSpeed, LTFLOAT fAccel, LTFLOAT fTime);		// 0x10154200
		void		Decelerate(const LTVector& vVel, LTVector* pvNewVel, LTFLOAT fAccel, LTFLOAT fTime);	// 0x101543b0
		LTVector	GetPos(HOBJECT hObj);									// 0x101547f0
		void		Save(HMESSAGEWRITE hWrite);								// 0x10154830
		void		Load(HMESSAGEREAD hRead);								// 0x101548a0

		int			GetMoveType() const { return m_eMoveType; }
		void		SetMoveType(int eType) { m_eMoveType = eType; }
		void		SetDest(const LTVector& vDest) { m_vDest = vDest; }
		void		SetMinSpeed(LTFLOAT fSpeed) { m_fMinSpeed = fSpeed; }
		void		SetSpeed(LTFLOAT fSpeed) { m_fSpeed = fSpeed; }

		// Move types (UpdateWalk's switch, 0x10154570; names ours)
		enum
		{
			eMoveStop = 0,		// zero velocity
			eMoveSlowDown,		// decelerate to a stop
			eMoveApproach,		// toward m_vDest
			eMoveFlee,			// away from m_vDest
			eMoveStrafeLeft,
			eMoveStrafeRight,
			eMoveUnused6,
			eMoveUnused7,
			eMoveCoast,			// keep the velocity
		};

		LTVector	m_vDest;		// 0x04
		int			m_eMoveType;	// 0x10
		uint32		m_dw14;			// 0x14, saved; use unknown
		uint32		m_dw18;			// 0x18, Update's mode: 0 walk, 1 and 2 nothing
		LTFLOAT		m_fMinSpeed;	// 0x1c, the speed it moves at (320)
		LTFLOAT		m_fSpeed;		// 0x20, its acceleration (1280)
		LTBOOL		m_bInstant;		// 0x24, no acceleration (name ours)
		LTBOOL		m_bMoving;		// 0x28, standing on the world
};

// PH: CAIBase::m_OrientMgr (0xe0, 0x28 bytes), vtable 0x1028b348;
// constructor 0x101552e0
class COrientationMgr
{
	public :

		COrientationMgr();											// 0x101552e0
		~COrientationMgr();											// 0x10155340 (not virtual: the vtable has one slot)

		virtual void	Update(HOBJECT hObj, LTFLOAT fFrameTime);	// 0x10155350

		void		Save(HMESSAGEWRITE hWrite);						// 0x10155550
		void		Load(HMESSAGEREAD hRead);						// 0x101555a0

		LTBOOL		GetFacePos() const { return m_bFacePos; }
		void		SetFacePos(LTBOOL bFacePos) { m_bFacePos = bFacePos; }
		LTVector	GetForward() const { return m_vForward; }
		void		SetFaceTarget(const LTVector& vPos) { m_vFacePos = vPos; }
		void		SetTurnRate(LTFLOAT fRate) { m_fTurnRate = fRate; }

		// m_bFacePos is Update's mode (names ours): keep the heading, turn
		// to m_vFacePos, or turn to the velocity
		enum { eOrientForward = 0, eOrientFacePos, eOrientVelocity };

		LTBOOL		m_bFacePos;		// 0x04
		LTVector	m_vForward;		// 0x08
		LTVector	m_vFacePos;		// 0x14
		LTBOOL		m_bEnabled;		// 0x20, turn in the horizontal only
		LTFLOAT		m_fTurnRate;	// 0x24, radians a second
};

// PH: CAIBase::m_Leash (0x110, 0x4c bytes): the AI's place in its leash
// group (LeashMgr.h); its code is 0x1014b610..0x1014b8c5 (AILeash.cpp)
class CLeash
{
	public :

		CLeash();
		~CLeash();

		LTBOOL			IsValid();
		LTBOOL			Update(const LTVector& vPos);
		void			Save(HMESSAGEWRITE hWrite);
		void			Load(HMESSAGEREAD hRead);
		CLeashNode*		GetNode(int nID);
		CLeashNode*		FindNode(const LTVector& vPos);
		LTBOOL			FindPath(const LTVector& vFrom, const LTVector& vTo, int* pnFrom, int* pnVia, int* pnTo);
		LTBOOL			GetPosToward(int nFrom, int nTo, const LTVector& vPos, LTVector& vOut, LTFLOAT fRadius);
		LTBOOL			GetPosAway(int nFrom, int nTo, const LTVector& vPos, LTVector& vOut, LTFLOAT fRadius);

		char			m_szGroup[32];	// 0x00
		HLEASHGROUP		m_hGroup;		// 0x20
		LTBOOL			m_bOutside;		// 0x24
		LTVector		m_vOutsidePos;	// 0x28
		int				m_nCurNode;		// 0x34
		int				m_nLastNode;	// 0x38
		int				m_nHomeNode;	// 0x3c
		LTVector		m_vHomePos;		// 0x40
};

// PH: CAIBase::m_Sounds (0x1f0, 0x30 bytes); its code is 0x1014d660..
class CAISounds
{
	public :

		CAISounds()
		{
			m_fSoundEnd = -1.0f;
			m_fNextPain = -1.0f;
			m_fNextAlert = -1.0f;
			m_dwFlags	= 50;
			m_fVolume	= 1.0f;
			m_fRadius	= 30.0f;
			m_nVolume	= 100;
			m_fPitch	= 0.0f;
			m_fInner	= 256.0f;
			m_fOuter	= 3072.0f;
			m_nPriority	= 128;
			m_hObject	= LTNULL;
		}

		HLTSOUND	PlaySound(const char* szSound, const LTVector& vPos, int nVolume, LTFLOAT fPitch,
							  uint8 nPriority, LTFLOAT fInner, LTFLOAT fOuter, uint32 dwFlags);
		HLTSOUND	PlayVoice(const char* szSound);
		HLTSOUND	PlayPain(const char* szSound);
		HLTSOUND	PlayAlert(const char* szSound);

		LTFLOAT		m_fSoundEnd;	// 0x00: when the last voice ends
		LTFLOAT		m_fNextPain;	// 0x04
		LTFLOAT		m_fNextAlert;	// 0x08
		LTFLOAT		m_fRadius;		// 0x0c
		LTFLOAT		m_fVolume;		// 0x10
		uint32		m_dwFlags;		// 0x14: PLAYSOUND_ flags
		int			m_nVolume;		// 0x18
		LTFLOAT		m_fPitch;		// 0x1c
		uint8		m_nPriority;	// 0x20
		LTFLOAT		m_fInner;		// 0x24
		LTFLOAT		m_fOuter;		// 0x28
		HOBJECT		m_hObject;		// 0x2c
};

// PH: CAIBase::m_StuckCheck (0x224, 0x24 bytes), inline only
struct CStuckCheck
{
	CStuckCheck()
	{
		m_vPos.x = m_vPos.y = m_vPos.z = 0.0f;
		m_fMinDist	= 32.0f;
		m_fInterval	= 0.5f;
		m_fTime		= 0.0f;
		m_bStuck	= LTFALSE;
		m_bEnabled	= LTFALSE;
		m_nAxes		= 5;
	}

	void Save(HMESSAGEWRITE hWrite)
	{
		*hWrite << m_vPos;
		*hWrite << m_fTime;
		*hWrite << m_fInterval;
		*hWrite << m_fMinDist;
		hWrite->WriteByte(m_bStuck);
		hWrite->WriteByte(m_bEnabled);
		hWrite->WriteByte(m_nAxes);
	}

	void Load(HMESSAGEREAD hRead)
	{
		*hRead >> m_vPos;
		m_fTime		= hRead->ReadFloat();
		m_fInterval	= hRead->ReadFloat();
		m_fMinDist	= hRead->ReadFloat();
		m_bStuck	= hRead->ReadByte();
		m_bEnabled	= hRead->ReadByte();
		m_nAxes		= hRead->ReadByte();
	}

	LTVector	m_vPos;			// 0x00
	LTFLOAT		m_fTime;		// 0x0c
	LTFLOAT		m_fInterval;	// 0x10
	LTFLOAT		m_fMinDist;		// 0x14
	LTBOOL		m_bStuck;		// 0x18
	LTBOOL		m_bEnabled;		// 0x1c
	uint8		m_nAxes;		// 0x20
};

// PH: CAIBase::m_Avoid (0x248, 0x3c bytes); its code is AISight.cpp's
// (Update 0x1015abd0, CheckBlocked 0x1015acd0, Steer 0x1015ad10)
class CAvoidance
{
	public :

		CAvoidance()
		{
			// RETAIL BUG: Reset() reads m_nState before anything has set it
			// (CAIBase::CAIBase, 0x1014261d); harmless only because the
			// result is overwritten when the avoidance first starts
			Reset();
			m_bActive	= LTFALSE;
			m_fCheckDist	= 320.0f;
			m_fAvoidDist	= 650.0f;
		}

		void	Reset()
		{
			if (m_nState && m_nState < 4)
				m_nNextState = m_nState;
			else
				m_nNextState = 1;

			m_nState = 0;
		}

		void	Update();
		LTBOOL	CheckBlocked(HOBJECT hObj, CMoveMgr* pMoveMgr);
		void	Steer(HOBJECT hObj, CMoveMgr* pMoveMgr, LTFLOAT fTime);

		int			m_nState;		// 0x00
		LTVector	m_vDir;			// 0x04
		LTVector	m_vRight;		// 0x10
		LTFLOAT		m_fTime;		// 0x1c
		LTVector	m_vPos;			// 0x20
		int			m_nNextState;	// 0x2c
		LTBOOL		m_bActive;		// 0x30
		LTFLOAT		m_fCheckDist;	// 0x34
		LTFLOAT		m_fAvoidDist;	// 0x38
};

// PH: CAIBase::m_Memory (0x290, 0xb4 bytes): who the AI has sensed;
// its code is 0x1014f080..
class CAIMemory
{
	public :

		CAIMemory();										// 0x1014f080
		~CAIMemory();										// 0x1014f100

		LTBOOL	Add(HOBJECT hObj);							// 0x1014f110
		LTBOOL	Forget(HOBJECT hObj);						// 0x1014f1d0
		void	Save(HMESSAGEWRITE hWrite);					// 0x1014f2f0
		void	Load(HMESSAGEREAD hRead);					// 0x1014f350
		LTBOOL	IsAware(HOBJECT hObj);						// 0x1014f400
		void	AddStimulus(HOBJECT hObj, int nSense, LTFLOAT fAmount);	// 0x1014f440
		void	Update(LTFLOAT fFrameTime);					// 0x1014f4a0
		HOBJECT	GetBest(int nSense);						// 0x1014f510
		void	ClearAll();									// 0x1014f570
		void	Clear(HOBJECT hObj);						// 0x1014f5a0
		void	SetRange(int nSense, LTFLOAT fRange);		// 0x1014f5f0
		void	SetDecay(int nSense, LTFLOAT fDecay);		// 0x1014f620
		LTFLOAT	GetStimulus(HOBJECT hObj, int nSense);		// 0x1014f650

	protected :

		struct Memory
		{
			LTSmartLink		m_hObject;
			CSenseLevels	m_Senses;
		};

		Memory	m_Memories[4];	// 0x00
		int		m_nCount;		// 0xb0
};

// PH: a spear stuck in a CAIBase (0x28 bytes); Save and Load are
// 0x1014e8d0 and 0x1014e940 (AttachedSpears.cpp)
struct SpearInfo
{
	SpearInfo()
	{
		hSpear = LTNULL;
		rRot.Init();
		vPos.Init(0.0f, 0.0f, 0.0f);
		nNode = -1;
	}

	void		Save(HMESSAGEWRITE hWrite);
	void		Load(HMESSAGEREAD hRead);

	LTSmartLink	hSpear;		// 0x00
	LTRotation	rRot;		// 0x08
	LTVector	vPos;		// 0x18
	int			nNode;		// 0x24
};

// PH: CAIBase::m_Spears (0x374, 0x10 bytes); its code is 0x1014ea00..
// (AttachedSpears.cpp)
class CSpearList : public TLinkList<SpearInfo*>
{
	public :

		CSpearList();
		~CSpearList();

		LTBOOL	Add(SpearInfo* pSpear);
		void	Save(HMESSAGEWRITE hWrite);
		void	Load(HMESSAGEREAD hRead);
		void	RemoveDead();
};

// PH: the state a CAIBase is told to go to (CAIBase::SetState, slot 38)
struct AIBaseStateInfo
{
	AIBaseStateInfo(int state = 0, int next = 0)
	{
		eState = state;
		vPos = LTVector(0.0f, 0.0f, 0.0f);
		nParam = next;
	}

	int			eState;
	LTVector	vPos;
	int			nParam;
};

// ----------------------------------------------------------------------- //

class CAIBase : public GameBase
{
	// PH: CTentacle sends its melee damage through DamageObject, with
	// itself as 'this' (PHObject.lto 0x10157e19)
	friend class CTentacle;

	public :

		enum State
		{
			eStateIdle = 0,
			eStateAlert,
			eStateDying,
			eStateDead,
			eStateMoving,
			eStatePain,
		};

		CAIBase(uint8 nType = OT_NORMAL);
		virtual ~CAIBase();

		// GameBase
		virtual uint32	ObjectMessageFn(HOBJECT hSender, uint32 messageID, HMESSAGEREAD hRead);
		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);
		virtual uint32	OnInitialUpdate(uint32 nInfo);
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);
		virtual uint32	OnModelStringKey(void *pData);
		virtual void	OnSave(HMESSAGEWRITE hWrite);
		virtual void	OnLoad(HMESSAGEREAD hRead);
		virtual LTBOOL	CanShootThrough();

		// CAIBase (slots 18..55)
		virtual void	SetVisionMode(int nMode) {}										// 18
		virtual void	HandleFootstep(HOBJECT hObj, LTBOOL bLoud, LTFLOAT fVolume);			// 19
		virtual void	HandleSound(HOBJECT hObj, const LTVector& vPos, LTFLOAT fVolume, uint32 nSurfaceType, LTFLOAT fTime); // 20
		virtual LTBOOL	IsDead();															// 21
		virtual LTBOOL	ChooseTarget();														// 22
		virtual LTBOOL	CheckForPlayer();													// 23
		virtual LTBOOL	IsTargetGone();														// 24
		virtual void	SetTarget(HOBJECT hTarget);											// 25
		virtual void	OnTargetChanged(HOBJECT hNew, HOBJECT hOld);							// 26
		virtual void	UpdateState(LTFLOAT fFrameTime);									// 27
		virtual void	UpdateIdle(LTFLOAT fFrameTime);  									// 28
		virtual void	UpdateAlert(LTFLOAT fFrameTime);  									// 29
		virtual void	UpdateDying(LTFLOAT fFrameTime);									// 30
		virtual void	UpdateDead(LTFLOAT fFrameTime) {}									// 31
		virtual void	UpdatePain(LTFLOAT fFrameTime);										// 32
		virtual LTBOOL	SetStateIdle(const AIBaseStateInfo* pInfo);							// 33
		virtual LTBOOL	SetStateAlert(const AIBaseStateInfo* pInfo);						// 34
		virtual LTBOOL	SetStateDying(const AIBaseStateInfo* pInfo);						// 35
		virtual LTBOOL	SetStateDead(const AIBaseStateInfo* pInfo);							// 36
		virtual LTBOOL	SetStatePain(const AIBaseStateInfo* pInfo);							// 37
		virtual LTBOOL	SetState(const AIBaseStateInfo* pInfo);								// 38
		virtual LTBOOL	CanAttack() { return LTFALSE; }										// 39
		virtual void	HandleDamage(const DamageStruct& damage);							// 40
		virtual LTBOOL	CanReact(const DamageStruct& damage);								// 41
		virtual void	UpdateMoving(LTFLOAT fFrameTime);									// 42
		virtual LTBOOL	SetStateMoving(const AIBaseStateInfo* pInfo);						// 43
		virtual LTBOOL	MoveToNode(int nFrom, int nTo);										// 44
		virtual LTBOOL	MoveAway(int nFrom, int nTo);										// 45
		virtual LTBOOL	MoveToPos(int nNode, const LTVector& vPos);							// 46
		virtual void	UpdateMoveToward(LTFLOAT fFrameTime);								// 47
		virtual void	UpdateMoveAway(LTFLOAT fFrameTime);									// 48
		virtual void	UpdateMoveTo(LTFLOAT fFrameTime);									// 49
		virtual LTBOOL	HasMoveTarget();													// 50
		virtual LTVector GetMoveTarget();													// 51
		virtual LTBOOL	ReturnToLeash() { return LTFALSE; }								// 52
		virtual LTBOOL	IsHeadShot(const DamageStruct& damage);								// 53
		virtual void	HideHead();															// 54
		virtual LTBOOL	HandleTrigger(const char* szMsg);									// 55

		LTBOOL			IsPlayerInBox(const LTVector& vMin, const LTVector& vMax);
		LTBOOL			AddSpear(HOBJECT hSpear, const LTVector& vPos, const LTVector& vDir, const LTVector& vScale);

	protected :

		int				FindClosestNode(HOBJECT hObj, const LTVector& vPos);
		void			SetSkins(int nSet, ObjectCreateStruct* pStruct, const char* aszSkins[][4]);
		void			DamageObject(HOBJECT hObj, LTVector* pvDir, LTFLOAT fDuration, LTFLOAT fDamage, uint8 eType);
		HMODELANIM		FindPainAnim();

	protected :

		CAISight		m_Sight;				// 0x050
		int				m_eState;				// 0x094
		LTSmartLink		m_hTarget;				// 0x098
		CAIAnimator		m_Animator;				// 0x0a0
		CMoveMgr		m_MoveMgr;				// 0x0b4
		COrientationMgr	m_OrientMgr;			// 0x0e0
		LTFLOAT			m_fMaxHitPoints;		// 0x108
		LTFLOAT			m_fHitPoints;			// 0x10c
		CLeash			m_Leash;				// 0x110
		int				m_eMoveMode;			// 0x15c
		LTVector		m_vMoveTarget;			// 0x160
		LTVector		m_vMovePos;				// 0x16c
		int				m_nMoveFrom;			// 0x178
		int				m_nMoveNode;			// 0x17c
		int				m_nMoveTo;				// 0x180
		int				m_nLastMoveNode;		// 0x184
		LTFLOAT			m_fMoveMinDist;			// 0x188
		LTFLOAT			m_fMoveMaxDist;			// 0x18c
		LTFLOAT			m_fMoveDist;			// 0x190
		uint32			m_nMoveAxes;			// 0x194
		AIBaseStateInfo	m_NextState;			// 0x198
		AIBaseStateInfo	m_MoveState;			// 0x1ac
		LTBOOL			m_bFirstUpdate;			// 0x1c0
		char			m_szName[32];			// 0x1c4
		LTVector		m_vLastPos;				// 0x1e4
		CAISounds		m_Sounds;				// 0x1f0
		LTBOOL			m_nSoundSet;			// 0x220
		CStuckCheck		m_StuckCheck;			// 0x224
		CAvoidance		m_Avoid;				// 0x248
		LTFLOAT			m_fHeadDist;			// 0x284
		const char*		m_szHeadPiece;			// 0x288
		const char*		m_szHeadSocket;			// 0x28c
		CAIMemory		m_Memory;				// 0x290
		LTFLOAT			m_fFootstepHearingRange;	// 0x344
		LTFLOAT			m_fFootstepSensitivity;		// 0x348
		HMODELANIM		m_hPainAnim;			// 0x34c
		LTFLOAT			m_fNextPainTime;		// 0x350
		LTFLOAT			m_fPainDelayMin;		// 0x354
		LTFLOAT			m_fPainDelayMax;		// 0x358
		int				m_ePainState;			// 0x35c
		uint32			m_nPainMoveType;		// 0x360
		LTBOOL			m_bPainFacePos;			// 0x364
		uint32			m_nPainAnimTime;		// 0x368
		HMODELANIM		m_hPainLastAnim;		// 0x36c
		int				m_nPainChance;			// 0x370
		CSpearList		m_Spears;				// 0x374
};

LTVector	GetPerpendicular(const LTVector& vDir);
void		RotateAroundY(const LTVector& vIn, LTVector& vOut, LTFLOAT fAngle);
void		TurnToward(const LTVector& vCur, const LTVector& vTarget, const LTVector& vUp,
					   LTFLOAT fRate, LTFLOAT fTime, LTVector& vOut);
LTBOOL		PlanesEqual(const LTPlane& a, const LTPlane& b, LTFLOAT fNormalTol, LTFLOAT fDistTol);

LTFLOAT		GetDifficultyDamageFactor();
LTFLOAT		GetDifficultyHealthFactor();

LTBOOL		IsAIBase(HOBJECT hObj);
CAIBase*	GetAIBase(HOBJECT hObj);
LTBOOL		IsAIBaseOrTentacle(HOBJECT hObj);

// PH: in the tentacle's code (0x10156180)
LTBOOL		IsTentacle(HOBJECT hObj);

// PH: a colour for the AI debugging hook
struct AIDebugColor
{
	AIDebugColor(LTFLOAT fR, LTFLOAT fG, LTFLOAT fB, LTFLOAT fA) { r = fR; g = fG; b = fB; a = fA; }
	LTFLOAT r, g, b, a;
};

// PH: an AI debugging hook, compiled out (0x10153e40, "xor eax,eax; ret")
int			AIDebugPoint(int nType, const LTVector& vPos, const AIDebugColor& color, LTFLOAT fSize);

// PH: the list of every CAIBase (AILeash.cpp, 0x1014b9b0..0x1014bad6)
void		AIBaseList_Clear();
void		AIBaseList_Add(CAIBase* pAI);
CAIBase*	AIBaseList_GetFirst();
CAIBase*	AIBaseList_GetNext();
void		AIBaseList_SetVisionMode(int nMode);
void		AIBaseList_HandleSound(HOBJECT hObj, const LTVector& vPos, LTFLOAT fVolume, uint32 nSurfaceType, LTFLOAT fTime);

#endif // PRIMAL_HUNT

#endif // __AI_BASE_H__
