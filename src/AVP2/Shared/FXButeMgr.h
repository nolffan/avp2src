// ----------------------------------------------------------------------- //
//
// MODULE  : FXButeMgr.h
//
// PURPOSE : FXButeMgr definition - Controls attributes of all weapons
//
// CREATED : 12/09/99
//
// (c) 1999-2000 Monolith Productions, Inc.  All Rights Reserved
//
// ----------------------------------------------------------------------- //

#ifndef __FX_BUTE_MGR_H__
#define __FX_BUTE_MGR_H__

#include "GameButeMgr.h"
#include "ltbasetypes.h"
#include "FXStructs.h"
#include "SurfaceDefs.h"
#include "ContainerCodes.h"

#ifdef PRIMAL_HUNT
#include "MultiFile.h"
#endif

#ifdef  _CLIENTBUILD

class CSpecialFX;
class CBaseScaleFX;
class CDynamicLightFX;
class CSoundFX;

#endif

class CFXButeMgr;
extern CFXButeMgr* g_pFXButeMgr;

#ifdef _WIN32
	#define FXBMGR_DEFAULT_FILE			"Attributes\\Fx.txt"
#else
	#define FXBMGR_DEFAULT_FILE			"Attributes/FX.txt"
#endif

#define FXBMGR_INVALID_ID			255

#define FXBMGR_MAX_FILE_PATH		64
#define FXBMGR_MAX_NAME_LENGTH		64

#define	IMPACT_MAX_DEBRISFX_TYPES	3
#define	IMPACT_MAX_SCALEFX_TYPES	10
#define	IMPACT_MAX_PEXPLFX_TYPES	5
#define	IMPACT_MAX_DLIGHTFX_TYPES	5
#define	IMPACT_MAX_PDEBRISFX_TYPES	3
#define IMPACT_MAX_PSHOWERFX_TYPES	3

#define FIRE_MAX_BEAMFX				5

#define	PV_MAX_SCALEFX_TYPES	10
#define	PV_MAX_DLIGHTFX_TYPES	10
#define PV_MAX_SOUNDFX_TYPES	10


#ifdef PRIMAL_HUNT
// Primal Hunt: the projectile class data are written to and read from the
// bute cache through a common base, which carries the kind of class data
// (1 PROJECTILECLASSDATA, 2 PROXCLASSDATA, 3 SPIDERCLASSDATA,
// 4 TRACKINGSADARCLASSDATA, 5 PREDDISKCLASSDATA: the constructors at
// PHCShell.dll 0x100c59c7..0x100cb1a0). The member's name is ours.
struct PROJBASEFX
{
	PROJBASEFX(int nType = 0)
	{
		nClassType = nType;
	}

	virtual LTBOOL	Write(CMultiFile* pFile)
	{
		if (!pFile) return LTFALSE;
		pFile->WriteByte(nClassType);
		return LTTRUE;
	}

	// Reads only the kind: PROJECTILEFX::Read reads one to know which
	// class data to make (PHCShell.dll 0x100cf1f0, called from the
	// FXButeMgr's cache reading)
	virtual LTBOOL	Read(CMultiFile* pFile)
	{
		if (!pFile) return LTFALSE;
		nClassType = pFile->ReadByte();
		return LTTRUE;
	}

	int		nClassType;
};

struct PROJECTILECLASSDATA : public PROJBASEFX
#else
struct PROJECTILECLASSDATA
#endif
{
#ifdef PRIMAL_HUNT
	PROJECTILECLASSDATA(int nType = 1) : PROJBASEFX(nType)
#else
	PROJECTILECLASSDATA()
#endif
	{
		szName[0] = '\0';
	}

#ifdef PRIMAL_HUNT
	// The kind, then the name; Read leaves the kind to its caller
	// (PHCShell.dll 0x100c5b10, 0x100c5b50)
	virtual LTBOOL	Write(CMultiFile* pFile)
	{
		if (!PROJBASEFX::Write(pFile)) return LTFALSE;
		pFile->WriteString(szName);
		return LTTRUE;
	}

	virtual LTBOOL	Read(CMultiFile* pFile)
	{
		if (!pFile) return LTFALSE;
		pFile->ReadString(szName, FXBMGR_MAX_NAME_LENGTH);
		return LTTRUE;
	}
#endif

    virtual LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	virtual void	Cache(CFXButeMgr* pFXButeMgr) {}

	char	szName[FXBMGR_MAX_NAME_LENGTH];
};

typedef CTList<PROJECTILECLASSDATA*> ProjClassDataList;


struct PROXCLASSDATA : public PROJECTILECLASSDATA
{
	PROXCLASSDATA();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	char	szArmSound[FXBMGR_MAX_FILE_PATH];
 	char	szActivateSound[FXBMGR_MAX_FILE_PATH];

	int		nActivateRadius;
	int		nArmSndRadius;
	int		nActivateSndRadius;

    LTFLOAT  fArmDelay;
    LTFLOAT  fActivateDelay;
};


struct SPIDERCLASSDATA : public PROJECTILECLASSDATA
{
	SPIDERCLASSDATA();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	char	szArmSound[FXBMGR_MAX_FILE_PATH];
 	char	szActivateSound[FXBMGR_MAX_FILE_PATH];
	int		nActivateSndRadius;

	int		nActivateRadius;
	int		nDetonateRadius;
	int		nArmSndRadius;

    LTFLOAT  fArmDelay;
};


struct TRACKINGSADARCLASSDATA : public PROJECTILECLASSDATA
{
	TRACKINGSADARCLASSDATA();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);

	LTFLOAT	fTurnRadius;
};


struct PREDDISKCLASSDATA : public PROJECTILECLASSDATA
{
	PREDDISKCLASSDATA();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);

    int	nNormRetrieveCost;
    int nTeleRetrieveCost;
};


struct PARTICLETRAILFX
{
	PARTICLETRAILFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

	LTBOOL	Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szTexture[FXBMGR_MAX_FILE_PATH];

	LTVector	vMinColor;
	LTVector	vMaxColor;
	LTVector	vMinDrift;
	LTVector	vMaxDrift;
	LTFLOAT		fLifetime;
	LTFLOAT		fFadetime;
	LTVector	vMinEmitRange;
	LTVector	vMaxEmitRange;
	LTFLOAT		fEmitDistance;
	int			nEmitAmount;
	LTFLOAT		fStartScale;
	LTFLOAT		fEndScale;
	LTFLOAT		fStartAlpha;
	LTFLOAT		fEndAlpha;
	LTFLOAT		fRadius;
	LTFLOAT		fGravity;
	LTBOOL		bAdditive;
	LTBOOL		bMultiply;
	LTBOOL		bIgnorWind;
};

typedef CTList<PARTICLETRAILFX*> ParticleTrailFXList;


struct PROJECTILEFX
{
	PROJECTILEFX();

#ifdef PRIMAL_HUNT
	// Primal Hunt: what Read built belongs to this one (PHObject.lto
	// 0x1001f9d0)
	~PROJECTILEFX();
#endif

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

#ifdef PRIMAL_HUNT
	// Primal Hunt: set when Read built the structures this one points to
	// (PROJECTILEFX::Read, PHCShell.dll 0x100c5adb); the constructor clears
	// it. Its name is ours.
	LTBOOL		bFromFile;
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szFlareSprite[FXBMGR_MAX_FILE_PATH];
	char		szSound[FXBMGR_MAX_FILE_PATH];
	char		szClass[FXBMGR_MAX_FILE_PATH];
	char		szModel[FXBMGR_MAX_FILE_PATH];
	char		szSkin[FXBMGR_MAX_FILE_PATH];
	char		szToggleScaleFX[FXBMGR_MAX_FILE_PATH];
	char		szSocket[FXBMGR_MAX_FILE_PATH];

	LTBOOL		bNoLighting;
    LTFLOAT		fModelAlpha;

	int			nAltVelocity;
	int			nVelocity;
    LTFLOAT		fLifeTime;
	int			nFlags;
	int			nParticleTrail;
	int			nUWParticleTrail;
    LTVector	vLightColor;
	int			nLightRadius;
	int			nSoundRadius;
    LTFLOAT		fFlareScale;
    uint32		dwObjectFlags;
    LTVector	vModelScale;

	// Data specific to our class (i.e., szClass)...
	PROJECTILECLASSDATA* pClassData;
};

typedef CTList<PROJECTILEFX*> ProjectileFXList;




struct PUSHERFX;
struct IMPACTFX
{
	IMPACTFX();

#ifdef PRIMAL_HUNT
	// Primal Hunt: what Read built belongs to this one (PHObject.lto
	// 0x1001ff30)
	~IMPACTFX();
#endif

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

#ifdef PRIMAL_HUNT
	// Primal Hunt: set when Read built the structures this one points to
	// (PROJECTILEFX::Read, PHCShell.dll 0x100c5adb); the constructor clears
	// it. Its name is ours.
	LTBOOL		bFromFile;
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szSound[FXBMGR_MAX_FILE_PATH];
	char		szMark[FXBMGR_MAX_FILE_PATH];
	char		szSoundDir[FXBMGR_MAX_FILE_PATH];

	int			nSoundRadius;
	int			nSoundBute;
	int			nAISoundRadius;
	int			nFlags;
    LTFLOAT     fMarkScale;
    LTVector    vTintColor;
    LTFLOAT     fTintRampUp;
    LTFLOAT     fTintRampDown;
    LTFLOAT     fTintMaxTime;
	LTFLOAT		fTintRadius;
    LTBOOL      bDoSurfaceFX;

	PUSHERFX*	pPusherFX;

	int			nNumDebrisFXTypes;
	int			aDebrisFXTypes[IMPACT_MAX_DEBRISFX_TYPES];

	int			nNumScaleFXTypes;
	int			aScaleFXTypes[IMPACT_MAX_SCALEFX_TYPES];

	int			nNumPExplFX;
	int			aPExplFXTypes[IMPACT_MAX_PEXPLFX_TYPES];

	int			nNumDLightFX;
	int			aDLightFXTypes[IMPACT_MAX_DLIGHTFX_TYPES];

	int			nNumPolyDebrisFX;
	int			aPolyDebrisFXTypes[IMPACT_MAX_PDEBRISFX_TYPES];

	int			nNumPShowerFX;
	int			aPShowerFXTypes[IMPACT_MAX_PSHOWERFX_TYPES];
};

typedef CTList<IMPACTFX*> ImpactFXList;

// ImpactFX Create Struct

struct IFXCS
{
	IFXCS()
	{
		vPos.Init();
		vDir.Init();
		vSurfNormal.Init();
		rSurfRot.Init();
		eSurfType		= ST_UNKNOWN;
		eCode			= CC_NO_CONTAINER;
        bPlaySound      = LTFALSE;
		fTintRange		= 0.0f;
	}

#ifdef PRIMAL_HUNT
	// Primal Hunt: the create structure writes and reads itself
	// (PHCShell.dll 0x100efd00, 0x100efda0)
	virtual LTBOOL	Write(CMultiFile* pFile)
	{
		if (!pFile) return LTFALSE;

		pFile->WriteVector(vPos);
		pFile->WriteVector(vDir);
		pFile->WriteVector(vSurfNormal);
		pFile->WriteFloat(rSurfRot.m_Quat[0]);
		pFile->WriteFloat(rSurfRot.m_Quat[1]);
		pFile->WriteFloat(rSurfRot.m_Quat[2]);
		pFile->WriteFloat(rSurfRot.m_Quat[3]);
		pFile->WriteByte(eSurfType);
		pFile->WriteByte(eCode);
		pFile->WriteBool(bPlaySound);
		pFile->WriteFloat(fTintRange);

		return LTTRUE;
	}

	virtual LTBOOL	Read(CMultiFile* pFile)
	{
		if (!pFile) return LTFALSE;

		pFile->ReadVector(&vPos);
		pFile->ReadVector(&vDir);
		pFile->ReadVector(&vSurfNormal);
		pFile->ReadFloat(&rSurfRot.m_Quat[0]);
		pFile->ReadFloat(&rSurfRot.m_Quat[1]);
		pFile->ReadFloat(&rSurfRot.m_Quat[2]);
		pFile->ReadFloat(&rSurfRot.m_Quat[3]);
		eSurfType = (SurfaceType)pFile->ReadByte();
		eCode = (ContainerCode)pFile->ReadByte();
		pFile->ReadBool(&bPlaySound);
		pFile->ReadFloat(&fTintRange);

		return LTTRUE;
	}
#endif

    LTVector         vPos;           // Position of FX
    LTVector         vDir;           // Direction for scale fx
    LTVector         vSurfNormal;    // Normal of surface of impact
    LTRotation       rSurfRot;       // Rotation aligned with surface normal
	SurfaceType		 eSurfType;		// Type of surface impacting on
	ContainerCode	 eCode;			// Container code fx is currently in
    LTBOOL           bPlaySound;     // Should the sound be played
    LTFLOAT          fTintRange;     // Max range for screen tints
};

struct BEAMFX;
struct FIREFX
{
	FIREFX();

#ifdef PRIMAL_HUNT
	// Primal Hunt: what Read built belongs to this one (PHObject.lto
	// 0x100206d0)
	~FIREFX();
#endif

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

#ifdef PRIMAL_HUNT
	// Primal Hunt: set when Read built the structures this one points to
	// (PROJECTILEFX::Read, PHCShell.dll 0x100c5adb); the constructor clears
	// it. Its name is ours.
	LTBOOL		bFromFile;
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	 Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szShellModel[FXBMGR_MAX_FILE_PATH];
	char		szShellSkin[FXBMGR_MAX_FILE_PATH];

	int			nFlags;
    LTVector    vShellScale;

	int			nNumBeamFX;
	BEAMFX*		pBeamFX[FIRE_MAX_BEAMFX];
};

typedef CTList<FIREFX*> FireFXList;


struct PEXPLFX
{
	PEXPLFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szFile[FXBMGR_MAX_FILE_PATH];
    LTVector     vPosOffset;
	int			nNumPerPuff;
	int			nNumEmitters;
	int			nNumSteps;
    LTBOOL       bCreateDebris;
    LTBOOL       bRotateDebris;
    LTBOOL       bIgnoreWind;
    LTBOOL       bDoBubbles;
    LTBOOL       bAdditive;
    LTBOOL       bMultiply;
    LTVector     vColor1;
    LTVector     vColor2;
    LTVector     vMinVel;
    LTVector     vMaxVel;
    LTVector     vMinDriftVel;
    LTVector     vMaxDriftVel;
    LTFLOAT      fLifeTime;
    LTFLOAT      fFadeTime;
    LTFLOAT      fOffsetTime;
    LTFLOAT      fRadius;
    LTFLOAT      fGravity;
};

typedef CTList<PEXPLFX*> PExplFXList;


struct DLIGHTFX
{
	DLIGHTFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	int		nId;

	char	szName[FXBMGR_MAX_NAME_LENGTH];

    LTVector vColor;
    LTFLOAT  fMinRadius;
    LTFLOAT  fMaxRadius;
    LTFLOAT  fMinTime;
    LTFLOAT  fMaxTime;
    LTFLOAT  fRampUpTime;
    LTFLOAT  fRampDownTime;
    LTFLOAT  fWashoutAmount;
};

typedef CTList<DLIGHTFX*> DLightFXList;


struct PVFX
{
	PVFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	int		nId;

	char	szName[FXBMGR_MAX_NAME_LENGTH];
	char	szSocket[FXBMGR_MAX_NAME_LENGTH];

	int		nNumScaleFXTypes;
	int		aScaleFXTypes[PV_MAX_SCALEFX_TYPES];

	int		nNumDLightFX;
	int		aDLightFXTypes[PV_MAX_DLIGHTFX_TYPES];

	int		nNumSoundFX;
	int		aSoundFXTypes[PV_MAX_SOUNDFX_TYPES];
};

typedef CTList<PVFX*> PVFXList;


struct CParticleMuzzleFX
{
	CParticleMuzzleFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	int			nId;
	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szFile[FXBMGR_MAX_FILE_PATH];
    LTFLOAT		fStartScale;
    LTFLOAT		fEndScale;
    LTFLOAT		fRadius;
    LTFLOAT		fLength;
    LTFLOAT		fDuration;
	int			nNumParticles;
    LTVector	vStartColor;
    LTVector	vEndColor;
    LTFLOAT		fStartAlpha;
    LTFLOAT		fEndAlpha;
    LTBOOL		bAdditive;
    LTBOOL		bMultiply;
	LTFLOAT		fMinVertVel;
	LTFLOAT		fMaxVertVel;
	LTFLOAT		fMinHorizVel;
	LTFLOAT		fMaxHorizVel;
};

typedef CTList<CParticleMuzzleFX*> ParticleMuzzleFXList;


struct CMuzzleFX
{
	CMuzzleFX();

#ifdef PRIMAL_HUNT
	// Primal Hunt: what Read built belongs to this one (PHObject.lto
	// 0x10021080)
	~CMuzzleFX();
#endif

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

#ifdef PRIMAL_HUNT
	// Primal Hunt: set when Read built the structures this one points to
	// (PROJECTILEFX::Read, PHCShell.dll 0x100c5adb); the constructor clears
	// it. Its name is ours.
	LTBOOL		bFromFile;
#endif

    LTBOOL   Init(CButeMgr & buteMgr, char* aTagName);
	void	Cache(CFXButeMgr* pFXButeMgr);

	char	szName[FXBMGR_MAX_NAME_LENGTH];
	int		nId;

	float	fDuration;

	CParticleMuzzleFX*	pPMuzzleFX;
	CScaleFX*			pScaleFX;
	DLIGHTFX*			pDLightFX;
};

typedef CTList<CMuzzleFX*> MuzzleFXList;

struct TRACERFX
{
	TRACERFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL      Init(CButeMgr & buteMgr, char* aTagName);
	void		Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szTexture[FXBMGR_MAX_FILE_PATH];

	int			nFrequency;
	float		fVelocity;
	float		fWidth;
	float		fInitialAlpha;
	float		fFinalAlpha;
    LTVector    vColor;

#ifdef PRIMAL_HUNT
	// Primal Hunt: TRACERFX is 0xf4 bytes (CFXButeMgr::Init, PHObject.lto
	// 0x10014966). Init (0x1001a480) reads these; the constructor leaves
	// them be, and the bute cache doesn't keep them. Names ours, after
	// the attributes'.
	LTBOOL		bSingleSegment;
	float		fHeadVelocity;
	char		szFlare[FXBMGR_MAX_FILE_PATH];
	float		fFlareScale;
#endif
};

typedef CTList<TRACERFX*> TracerFXList;


struct BEAMFX
{
	BEAMFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL      Init(CButeMgr & buteMgr, char* aTagName);
	void		Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szTexture[FXBMGR_MAX_FILE_PATH];

	float		fDuration;
	float		fWidth;
	float		fInitialAlpha;
	float		fFinalAlpha;
    LTVector    vColor;
    LTBOOL      bAlignUp;
    LTBOOL      bAlignFlat;
};

typedef CTList<BEAMFX*> BeamFXList;


struct SOUNDFX
{
	SOUNDFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL      Init(CButeMgr & buteMgr, char* aTagName);
	void		Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szFile[FXBMGR_MAX_FILE_PATH];

	LTBOOL		bLoop;
	LTFLOAT		fRadius;
	LTFLOAT		fPitchShift;
};

typedef CTList<SOUNDFX*> SoundFXList;


struct PUSHERFX
{
	PUSHERFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL      Init(CButeMgr & buteMgr, char* aTagName);
	void		Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];

	LTFLOAT		fRadius;
	LTFLOAT		fStartDelay;
	LTFLOAT		fDuration;
	LTFLOAT		fStrength;
};

typedef CTList<PUSHERFX*> PusherFXList;


struct SMOKEFX
{
	SMOKEFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL      Init(CButeMgr & buteMgr, char* aTagName);
	void		Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szFile[FXBMGR_MAX_NAME_LENGTH];

	LTVector	vColor1;
	LTVector	vColor2;
	LTVector	vMinDrift;
	LTVector	vMaxDrift;
	LTFLOAT		fLifetime;
	LTFLOAT		fEmitRadius;
	LTFLOAT		fRadius;
	LTFLOAT		fCreateDelta;
	LTBOOL		bAdjustAlpha;
	LTFLOAT		fStartAlpha;
	LTFLOAT		fEndAlpha;
	LTBOOL		bAdjustScale;
	LTFLOAT		fStartScale;
	LTFLOAT		fEndScale;
	LTFLOAT		fMinParticleLife;
	LTFLOAT		fMaxParticleLife;
	uint8		nNumParticles;
	LTBOOL		bAdditive;
	LTBOOL		bMultiply;
	LTBOOL		bIgnorWind;
	LTBOOL		bUpdatePos;
};

typedef CTList<SMOKEFX*> SmokeFXList;


struct STEAMFX
{
	STEAMFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL      Init(CButeMgr & buteMgr, char* aTagName);
	void		Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];

	int			nSmokeFX;
	int			nSoundFX;

	LTFLOAT		fDamageDist;
	LTFLOAT		fDamage;
};

typedef CTList<STEAMFX*> SteamFXList;


struct FIREOBJFX
{
	FIREOBJFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL      Init(CButeMgr & buteMgr, char* aTagName);
	void		Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];

	int			nInnerFireFX;
	int			nOuterFireFX;
	int			nSmokeFX;
	int			nSoundFX;

	LTFLOAT		fSizeRadiusScale;

	LTBOOL		bCreateLight;
	LTVector	vLightColor;
	LTFLOAT		fLightRadius;
	LTFLOAT		fLightPhase;
	LTFLOAT		fLightFreq;
	LTVector	vLightOffset;

	LTBOOL		bCreateSparks;
};

typedef CTList<FIREOBJFX*> FireObjFXList;


struct DRIPPEROBJFX
{
	DRIPPEROBJFX();

#ifdef PRIMAL_HUNT
	virtual LTBOOL	Write(CMultiFile* pFile);
	virtual LTBOOL	Read(CMultiFile* pFile);
#endif

    LTBOOL      Init(CButeMgr & buteMgr, char* aTagName);
	void		Cache(CFXButeMgr* pFXButeMgr);

	int			nId;

	char		szName[FXBMGR_MAX_NAME_LENGTH];
	char		szFile[FXBMGR_MAX_NAME_LENGTH];

	LTVector	vMinColor;
	LTVector	vMaxColor;
	LTFLOAT		fAlpha;
	LTBOOL		bAdditive;
	LTBOOL		bMultiply;
	LTFLOAT		fMinSize;
	LTFLOAT		fMaxSize;

	int			nMinDripAmount;
	int			nMaxDripAmount;
	LTFLOAT		fMinDripFrequency;
	LTFLOAT		fMaxDripFrequency;
	LTFLOAT		fMinDripTime;
	LTFLOAT		fMaxDripTime;
	LTFLOAT		fDripDistance;
	LTFLOAT		fDripSpeed;
	LTFLOAT		fAccelerateTime;

	LTFLOAT		fDripTrailLength;
	LTFLOAT		fDripTrailDensity;

	LTFLOAT		fWiggleAmount;
	LTFLOAT		fWiggleFrequency;

	LTBOOL		bCylinderShape;

	LTFLOAT		fImpactFXPercent;
	int			nImpactFX;
};

typedef CTList<DRIPPEROBJFX*> DripperObjFXList;



class CFXButeMgr : public CGameButeMgr
{
	public :

		CFXButeMgr();
		~CFXButeMgr();

        LTBOOL           Init(ILTCSBase *pInterface, const char* szAttributeFile=FXBMGR_DEFAULT_FILE);
		void			Term();

        void            Reload(ILTCSBase *pInterface);

#ifdef PRIMAL_HUNT
		// Primal Hunt: the bute cache (MultiFile.h; PHObject.lto 0x1001c000,
		// 0x1001c350)
		virtual LTBOOL	WriteCache(const char* szFile, CMultiFile* pFile);
		virtual LTBOOL	ReadCache(const char* szFile, CMultiFile* pFile);
#endif

#ifdef _CLIENTBUILD
        CSpecialFX* CreateScaleFX(CScaleFX* pScaleFX, LTVector vPos,
            LTVector vDir, LTVector* pvSurfaceNormal, LTRotation* prRot,
            CBaseScaleFX* pFX=LTNULL);

        CSpecialFX* CreatePShowerFX(CPShowerFX* pPShowerFX, LTVector vPos,
            LTVector vDir, LTVector vSurfaceNormal);

		CSpecialFX*	CreatePolyDebrisFX(CPolyDebrisFX* pPolyDebrisFX,
            LTVector vPos, LTVector vDir, LTVector vSurfaceNormal);

        CSpecialFX* CreatePExplFX(PEXPLFX* pPExplFX, LTRotation rSurfaceRot,
            SurfaceType eSurfaceType, LTVector vPos, ContainerCode eCode);

        CSpecialFX* CreateDLightFX(DLIGHTFX* pDLightFX, LTVector vPos,
            CDynamicLightFX* pFX=LTNULL);

        CSpecialFX* CreateBeamFX(BEAMFX* pBeamFX, LTVector vStartPos,
			LTVector vEndPos);

		void CreateImpactFX(IMPACTFX* pImpactFX, IFXCS & cs);

		CSpecialFX* CreateSoundFX(SOUNDFX* pSoundFX, LTVector vPos,
			CSoundFX* pFX);

		void CreatePusherFX(PUSHERFX* pPusherFX, LTVector vPos);

#endif // _CLIENTBUILD

		PUSHERFX*		GetPusherFX(int nFXId);
		PUSHERFX*		GetPusherFX(char* pName);

#ifndef _CLIENTBUILD
		int				GetNumSoundFX() const { return m_SoundFXList.GetLength(); }
        LTBOOL          ReadSoundFXProp(char* pPropName, uint8 & nSoundFXId);
#endif // _CLIENTBUILD

		SOUNDFX*		GetSoundFX(int nSoundFXId);
		SOUNDFX*		GetSoundFX(char* pName);

		TRACERFX*		GetTracerFX(int nTracerFXId);
		TRACERFX*		GetTracerFX(char* pName);

		BEAMFX*			GetBeamFX(int nBeamFXId);
		BEAMFX*			GetBeamFX(char* pName);

		CMuzzleFX*		GetMuzzleFX(int nMuzzleFXId);
		CMuzzleFX*		GetMuzzleFX(char* pName);

		CParticleMuzzleFX*	GetParticleMuzzleFX(int nParticleMuzzleFXId);
		CParticleMuzzleFX*	GetParticleMuzzleFX(char* pName);

		PVFX*			GetPVFX(int nPVFXId);
		PVFX*			GetPVFX(char* pName);

		CScaleFX*		GetScaleFX(int nScaleFXId);
		CScaleFX*		GetScaleFX(char* pName);

		CPShowerFX*		GetPShowerFX(int nPShowerFXId);
		CPShowerFX*		GetPShowerFX(char* pName);

		CPolyDebrisFX*	GetPolyDebrisFX(int nPolyDebrisFXId);
		CPolyDebrisFX*	GetPolyDebrisFX(char* pName);

		PEXPLFX*		GetPExplFX(int nPExplFXId);
		PEXPLFX*		GetPExplFX(char* pName);

		DLIGHTFX*		GetDLightFX(int nDLightFXId);
		DLIGHTFX*		GetDLightFX(char* pName);

#ifndef _CLIENTBUILD
		int				GetNumImpactFX() const { return m_ImpactFXList.GetLength(); }
        LTBOOL          ReadImpactFXProp(char* pPropName, uint8 & nImpactFXId);
#endif // _CLIENTBUILD

		IMPACTFX*		GetImpactFX(int nImpactFXId);
		IMPACTFX*		GetImpactFX(char* pName);

		PARTICLETRAILFX*	GetParticleTrailFX(int nParticleTrailFXId);
		PARTICLETRAILFX*	GetParticleTrailFX(char* pName);

		PROJECTILEFX*	GetProjectileFX(int nProjFXId);
		PROJECTILEFX*	GetProjectileFX(char* pName);

		PROJECTILECLASSDATA*	GetProjectileClassData(char* pName);

		FIREFX*			GetFireFX(int nFireFXId);
		FIREFX*			GetFireFX(char* pName);

#ifndef _CLIENTBUILD
		int				GetNumSmokeFX() const { return m_SmokeFXList.GetLength(); }
        LTBOOL          ReadSmokeFXProp(char* pPropName, uint8 & nSmokeFXId);
#endif // _CLIENTBUILD

		SMOKEFX*		GetSmokeFX(int nSmokeFXId);
		SMOKEFX*		GetSmokeFX(char* pName);

#ifndef _CLIENTBUILD
		int				GetNumSteamFX() const { return m_SteamFXList.GetLength(); }
        LTBOOL          ReadSteamFXProp(char* pPropName, uint8 & nSteamFXId);
#endif // _CLIENTBUILD

		STEAMFX*		GetSteamFX(int nSteamFXId);
		STEAMFX*		GetSteamFX(char* pName);

#ifndef _CLIENTBUILD
		int				GetNumFireObjFX() const { return m_FireObjFXList.GetLength(); }
        LTBOOL          ReadFireObjFXProp(char* pPropName, uint8 & nFireObjFXId);
#endif // _CLIENTBUILD

		FIREOBJFX*		GetFireObjFX(int nFireObjFXId);
		FIREOBJFX*		GetFireObjFX(char* pName);

#ifndef _CLIENTBUILD
		int				GetNumDripperObjFX() const { return m_DripperObjFXList.GetLength(); }
        LTBOOL          ReadDripperObjFXProp(char* pPropName, uint8 & nDripperObjFXId);
#endif // _CLIENTBUILD

		DRIPPEROBJFX*	GetDripperObjFX(int nDripperObjFXId);
		DRIPPEROBJFX*	GetDripperObjFX(char* pName);


	protected :

		ParticleTrailFXList		m_ParticleTrailFXList;	// All particle trail fx types
		ProjectileFXList		m_ProjectileFXList;	// All projectile fx types
		ProjClassDataList		m_ProjClassDataList;// All projectile class data
		ImpactFXList			m_ImpactFXList;		// All impact fx types
		FireFXList				m_FireFXList;		// All fire fx types
		ScaleFXList				m_ScaleFXList;		// All scale fx types
		PExplFXList				m_PExplFXList;		// All particle explosion fx types
		DLightFXList			m_DLightFXList;		// All dynamic light fx types
		PShowerFXList			m_PShowerFXList;	// All particle shower fx
		PolyDebrisFXList		m_PolyDebrisFXList; // All poly debris fx
		PVFXList				m_PVFXList;			// All player-view fx
		ParticleMuzzleFXList	m_PartMuzzleFXList; // All particle muzzle fx
		MuzzleFXList			m_MuzzleFXList;		// All weapon muzzle fx
		TracerFXList			m_TracerFXList;		// All weapon tracer fx
		BeamFXList				m_BeamFXList;		// All weapon beam fx
		SoundFXList				m_SoundFXList;		// All sound fx
		PusherFXList			m_PusherFXList;		// All pusher fx
		SmokeFXList				m_SmokeFXList;		// All smoke fx
		SteamFXList				m_SteamFXList;		// All steam fx
		FireObjFXList			m_FireObjFXList;	// All fireobj fx
		DripperObjFXList		m_DripperObjFXList;	// All dripperobj fx

	private:

		IndexTable				m_ScaleFXTable;		// Scale FX Hash Map
};

////////////////////////////////////////////////////////////////////////////
//
// CFXButeMgrPlugin is used to help facilitate populating the DEdit object
// properties that use FXButeMgr
//
////////////////////////////////////////////////////////////////////////////
#ifndef _CLIENTBUILD

#include "iobjectplugin.h"

class CFXButeMgrPlugin : public IObjectPlugin
{
	public:

        virtual LTRESULT PreHook_EditStringList(
			const char* szRezPath,
			const char* szPropName,
			char* const * aszStrings,
            uint32* pcStrings,
            const uint32 cMaxStrings,
            const uint32 cMaxStringLength);

        LTBOOL PopulateImpactFXStringList(char* const * aszStrings, uint32* pcStrings,
            const uint32 cMaxStrings, const uint32 cMaxStringLength);

        LTBOOL PopulateSmokeFXStringList(char* const * aszStrings, uint32* pcStrings,
            const uint32 cMaxStrings, const uint32 cMaxStringLength);

        LTBOOL PopulateSteamFXStringList(char* const * aszStrings, uint32* pcStrings,
            const uint32 cMaxStrings, const uint32 cMaxStringLength);

        LTBOOL PopulateFireFXStringList(char* const * aszStrings, uint32* pcStrings,
            const uint32 cMaxStrings, const uint32 cMaxStringLength);

        LTBOOL PopulateDripperFXStringList(char* const * aszStrings, uint32* pcStrings,
            const uint32 cMaxStrings, const uint32 cMaxStringLength);

        LTBOOL PopulateSoundFXStringList(char* const * aszStrings, uint32* pcStrings,
            const uint32 cMaxStrings, const uint32 cMaxStringLength);

};

#endif // _CLIENTBUILD

#endif // __FX_BUTE_MGR_H__
