// ----------------------------------------------------------------------- //
//
// MODULE  : FXStructs.cpp
//
// PURPOSE : Implementation of common FX structs/classes
//
// CREATED : 7/15/99
//
// (c) 1999-2000 Monolith Productions, Inc.  All Rights Reserved
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"
#include "FXStructs.h"
#include "CommonUtilities.h"

#define	SCALEFX_NAME				"Name"
#define	SCALEFX_TYPE				"Type"
#define	SCALEFX_FILE				"File"
#define SCALEFX_SKIN				"Skin"
#define SCALEFX_INITIALSCALE		"InitialScale"
#define SCALEFX_FINALSCALE			"FinalScale"
#define SCALEFX_USECOLORS			"UseColors"
#define SCALEFX_INITIALCOLOR		"InitialColor"
#define SCALEFX_FINALCOLOR			"FinalColor"
#define SCALEFX_INITIALALPHA		"InitialAlpha"
#define SCALEFX_FINALALPHA			"FinalAlpha"
#define SCALEFX_DIROFFSET			"DirOffset"
#define SCALEFX_DIRROFFSET			"DirROffset"
#define SCALEFX_DIRUOFFSET			"DirUOffset"
#define SCALEFX_VELOCITY			"Velocity"
#define SCALEFX_LIFETIME			"LifeTime"
#define SCALEFX_DELAYTIME			"DelayTime"
#define SCALEFX_LOOP				"Loop"
#define SCALEFX_ALIGNTOSURFACE		"AlignToSurface"
#define SCALEFX_SPRITENOZ			"NoZ"
#define SCALEFX_MODELREALLYCLOSE	"ReallyClose"
#define SCALEFX_ADDITIVE			"Additive"
#define SCALEFX_MULTIPLY			"Multiply"
#define SCALEFX_ROTATE				"Rotate"
#define SCALEFX_MINROTVEL			"MinRotVel"
#define SCALEFX_MAXROTVEL			"MaxRotVel"
#define SCALEFX_FACECAMERA			"FaceCamera"
#define SCALEFX_ROTAXIS				"RotationAxis"
#ifdef PRIMAL_HUNT
#define SCALEFX_SKIN1				"Skin1"
#define SCALEFX_SKIN2				"Skin2"
#define SCALEFX_SKIN3				"Skin3"
#define SCALEFX_ROTATIONY			"RotationY"
#define SCALEFX_ADDITIVEALPHA		"AdditiveAlpha"
#endif

#define	PSHOWERFX_NAME				"Name"
#define	PSHOWERFX_TEXTURE			"Texture"
#define	PSHOWERFX_MINPARTICLES		"MinParticles"
#define	PSHOWERFX_MAXPARTICLES		"MaxParticles"
#define PSHOWERFX_MINSIZE			"MinSize"
#define PSHOWERFX_MAXSIZE			"MaxSize"
#define	PSHOWERFX_COLOR1			"Color1"
#define	PSHOWERFX_COLOR2			"Color2"
#define	PSHOWERFX_MINVEL			"MinVel"
#define	PSHOWERFX_MAXVEL			"MaxVel"
#define	PSHOWERFX_DIROFFSET			"DirOffset"
#define	PSHOWERFX_MINDURATION		"MinDuration"
#define	PSHOWERFX_MAXDURATION		"MaxDuration"
#define	PSHOWERFX_FADETIME			"FadeTime"
#define	PSHOWERFX_EMISSIONRADIUS	"EmissionRadius"
#define	PSHOWERFX_RADIUS			"Radius"
#define	PSHOWERFX_GRAVITY			"Gravity"
#define PSHOWERFX_ADDITIVE			"Additive"
#define PSHOWERFX_MULTIPLY			"Multiply"
#define PSHOWERFX_LIQUID			"RemoveIfNotInLiquid"

#define	POLYDEBRISFX_NAME			"Name"
#define	POLYDEBRISFX_TEXTURE		"Texture"
#define	POLYDEBRISFX_STYLE			"Style"
#define	POLYDEBRISFX_MINDEBRIS		"MinDebris"
#define	POLYDEBRISFX_MAXDEBRIS		"MaxDebris"
#define	POLYDEBRISFX_MINBOUNCE		"MinBounce"
#define	POLYDEBRISFX_MAXBOUNCE		"MaxBounce"
#define	POLYDEBRISFX_MINCOLOR1		"MinColor1"
#define	POLYDEBRISFX_MAXCOLOR1		"MaxColor1"
#define POLYDEBRISFX_MINCOLOR2		"MinColor2"
#define POLYDEBRISFX_MAXCOLOR2		"MaxColor2"
#define	POLYDEBRISFX_ADDITIVE		"Additive"
#define	POLYDEBRISFX_MULTIPLY		"Multiply"
#define	POLYDEBRISFX_SHOWTRAIL		"ShowTrail"
#define POLYDEBRISFX_DIROFFSETONLY	"DirOffsetOnly"
#define	POLYDEBRISFX_INITIALALPHA	"InitialAlpha"
#define	POLYDEBRISFX_FINALALPHA		"FinalAlpha"
#define	POLYDEBRISFX_MINVEL			"MinVel"
#define	POLYDEBRISFX_MAXVEL			"MaxVel"
#define	POLYDEBRISFX_DIROFFSET		"DirOffset"
#define	POLYDEBRISFX_GRAVITYSCALE	"GravityScale"
#define	POLYDEBRISFX_MINDOFFSET		"MinDebrisOffset"
#define	POLYDEBRISFX_MAXDOFFSET		"MaxDebrisOffset"
#define	POLYDEBRISFX_MINDURATION	"MinDuration"
#define	POLYDEBRISFX_MAXDURATION	"MaxDuration"
#define	POLYDEBRISFX_MINWIDTH		"MinWidth"
#define	POLYDEBRISFX_MAXWIDTH		"MaxWidth"
#define	POLYDEBRISFX_MINLENGTH		"MinLength"
#define	POLYDEBRISFX_MAXLENGTH		"MaxLength"




static char s_aTagName[30];

/////////////////////////////////////////////////////////////////////////////
//
//	S C A L E  F X  Stuff...
//
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	BuildScaleFXList()
//
//	PURPOSE:	Build the list of scale fx structs
//
// ----------------------------------------------------------------------- //

LTBOOL BuildScaleFXList(ScaleFXList & list, CButeMgr & buteMgr, char* pTagBase, IndexTable & iTable)
{
    if (!pTagBase) return LTFALSE;

	// Count elements then make room in the list
	int nNum = 0;
	sprintf(s_aTagName, "%s%d", pTagBase, nNum);

	while (buteMgr.Exist(s_aTagName))
	{
		++nNum;
		sprintf(s_aTagName, "%s%d", pTagBase, nNum);
	}

	if(nNum)
		list.reserve(nNum+1);

	// Now start over and load the list in order
	nNum = 0;
	sprintf(s_aTagName, "%s%d", pTagBase, nNum);

	// Read in the properties for each scale fx...

	while (buteMgr.Exist(s_aTagName))
	{
		CScaleFX* pFX = new CScaleFX;

		if (pFX && pFX->Init(buteMgr, s_aTagName))
		{
			pFX->nId = nNum;
			list.push_back(pFX);
		}
		else
		{
			delete pFX;
            return LTFALSE;
		}

		// Store away the name-index pair for quick look-up.
		iTable[pFX->szName] = nNum;

		++nNum;
		sprintf(s_aTagName, "%s%d", pTagBase, nNum);
	}

    return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CScaleFX::CScaleFX()
//
//	PURPOSE:	Constructor
//
// ----------------------------------------------------------------------- //

CScaleFX::CScaleFX()
{
	nId				= -1;
	eType			= SCALEFX_MODEL;
	fInitialAlpha	= 1.0f;
	fFinalAlpha		= 1.0f;
	fDirOffset		= 0.0f;
	fDirROffset		= 0.0f;
	fDirUOffset		= 0.0f;
	fLifeTime		= 0.0f;
	fDelayTime		= 0.0f;
	fMinRotVel		= 0.0f;
	fMaxRotVel		= 0.0f;
    bUseColors      = LTFALSE;
    bLoop           = LTFALSE;
    bAlignToSurface = LTFALSE;
    bNoZ            = LTFALSE;
    bReallyClose    = LTFALSE;
    bAdditive       = LTFALSE;
    bMultiply       = LTFALSE;
    bRotate         = LTFALSE;
    bFaceCamera     = LTFALSE;
	nRotationAxis	= 0;

	szName[0]		= '\0';
	szFile[0]		= '\0';
	szSkin[0]		= '\0';

	vInitialScale.Init();
	vFinalScale.Init();
	vInitialColor.Init();
	vFinalColor.Init();
	vVel.Init();

#ifdef PRIMAL_HUNT
	szSkin1[0]		= '\0';
	szSkin2[0]		= '\0';
	szSkin3[0]		= '\0';
	fRotationY		= 0.0f;
#endif
}

#ifdef PRIMAL_HUNT

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CScaleFX::Write / Read
//
//	PURPOSE:	Primal Hunt: the bute cache (MultiFile.h; PHCShell.dll
//				0x10108780, 0x10108960)
//
// ----------------------------------------------------------------------- //

LTBOOL CScaleFX::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->WriteByte(eType);
	pFile->WriteInt(nId);
	pFile->WriteString(szName);
	pFile->WriteString(szFile);
	pFile->WriteString(szSkin);
	pFile->WriteString(szSkin1);
	pFile->WriteString(szSkin2);
	pFile->WriteString(szSkin3);
	pFile->WriteVector(vInitialScale);
	pFile->WriteVector(vFinalScale);
	pFile->WriteBool(bUseColors);
	pFile->WriteBool(bLoop);
	pFile->WriteBool(bAlignToSurface);
	pFile->WriteBool(bNoZ);
	pFile->WriteBool(bReallyClose);
	pFile->WriteBool(bAdditive);
	pFile->WriteBool(bMultiply);
	pFile->WriteBool(bFaceCamera);
	pFile->WriteByte(nRotationAxis);
	pFile->WriteBool(bRotate);
	pFile->WriteVector(vInitialColor);
	pFile->WriteVector(vFinalColor);
	pFile->WriteFloat(fInitialAlpha);
	pFile->WriteFloat(fFinalAlpha);
	pFile->WriteFloat(fDirOffset);
	pFile->WriteFloat(fDirROffset);
	pFile->WriteFloat(fDirUOffset);
	pFile->WriteFloat(fMinRotVel);
	pFile->WriteFloat(fMaxRotVel);
	pFile->WriteVector(vVel);
	pFile->WriteFloat(fLifeTime);
	pFile->WriteFloat(fDelayTime);
	pFile->WriteFloat(fRotationY);

	// RETAIL BUG: bAdditiveAlpha is neither written nor read (nor set by
	// the constructor), so a scale fx read from the cache has whatever the
	// allocation held.

	return LTTRUE;
}

LTBOOL CScaleFX::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	eType = (ScaleFXType) pFile->ReadByte();
	pFile->ReadInt(&nId);
	pFile->ReadString(szName, ARRAY_LEN(szName));
	pFile->ReadString(szFile, ARRAY_LEN(szFile));
	pFile->ReadString(szSkin, ARRAY_LEN(szSkin));
	pFile->ReadString(szSkin1, ARRAY_LEN(szSkin1));
	pFile->ReadString(szSkin2, ARRAY_LEN(szSkin2));
	pFile->ReadString(szSkin3, ARRAY_LEN(szSkin3));
	pFile->ReadVector(&vInitialScale);
	pFile->ReadVector(&vFinalScale);
	pFile->ReadBool(&bUseColors);
	pFile->ReadBool(&bLoop);
	pFile->ReadBool(&bAlignToSurface);
	pFile->ReadBool(&bNoZ);
	pFile->ReadBool(&bReallyClose);
	pFile->ReadBool(&bAdditive);
	pFile->ReadBool(&bMultiply);
	pFile->ReadBool(&bFaceCamera);
	pFile->ReadByte(&nRotationAxis);
	pFile->ReadBool(&bRotate);
	pFile->ReadVector(&vInitialColor);
	pFile->ReadVector(&vFinalColor);
	pFile->ReadFloat(&fInitialAlpha);
	pFile->ReadFloat(&fFinalAlpha);
	pFile->ReadFloat(&fDirOffset);
	pFile->ReadFloat(&fDirROffset);
	pFile->ReadFloat(&fDirUOffset);
	pFile->ReadFloat(&fMinRotVel);
	pFile->ReadFloat(&fMaxRotVel);
	pFile->ReadVector(&vVel);
	pFile->ReadFloat(&fLifeTime);
	pFile->ReadFloat(&fDelayTime);
	pFile->ReadFloat(&fRotationY);

	return LTTRUE;
}

#endif // PRIMAL_HUNT

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CScaleFX::Init
//
//	PURPOSE:	Init the scale fx struct from the bute mgr
//
// ----------------------------------------------------------------------- //

LTBOOL CScaleFX::Init(CButeMgr & buteMgr, char* aTagName)
{
    if (!aTagName) return LTFALSE;

	eType			= (ScaleFXType) buteMgr.GetInt(aTagName, SCALEFX_TYPE);
    fInitialAlpha   = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_INITIALALPHA);
    fFinalAlpha     = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_FINALALPHA);
    fDirOffset      = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_DIROFFSET);
    fDirROffset     = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_DIRROFFSET);
    fDirUOffset     = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_DIRUOFFSET);
    fLifeTime       = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_LIFETIME);
    fDelayTime      = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_DELAYTIME);
    fMinRotVel      = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_MINROTVEL);
    fMaxRotVel      = (LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_MAXROTVEL);
    bUseColors      = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_USECOLORS);
    bLoop           = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_LOOP);
    bAlignToSurface = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_ALIGNTOSURFACE);
    bNoZ            = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_SPRITENOZ);
    bReallyClose    = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_MODELREALLYCLOSE);
    bAdditive       = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_ADDITIVE);
    bMultiply       = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_MULTIPLY);
    bRotate         = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_ROTATE);
    bFaceCamera     = (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_FACECAMERA);
    nRotationAxis   = buteMgr.GetInt(aTagName, SCALEFX_ROTAXIS);

	vInitialScale	= buteMgr.GetVector(aTagName, SCALEFX_INITIALSCALE);
	vFinalScale		= buteMgr.GetVector(aTagName, SCALEFX_FINALSCALE);
	vInitialColor	= buteMgr.GetVector(aTagName, SCALEFX_INITIALCOLOR);
	vInitialColor /= 255.0f;

	vFinalColor		= buteMgr.GetVector(aTagName, SCALEFX_FINALCOLOR);
	vFinalColor /= 255.0f;

	vVel			= buteMgr.GetVector(aTagName, SCALEFX_VELOCITY);

	CString str = buteMgr.GetString(aTagName, SCALEFX_FILE);
	if (!str.IsEmpty())
	{
		strncpy(szFile, (char*)(LPCSTR)str, ARRAY_LEN(szFile));
	}

	str = buteMgr.GetString(aTagName, SCALEFX_SKIN);
	if (!str.IsEmpty())
	{
		strncpy(szSkin, (char*)(LPCSTR)str, ARRAY_LEN(szSkin));
	}

#ifdef PRIMAL_HUNT
	str = buteMgr.GetString(aTagName, SCALEFX_SKIN1);
	if (!str.IsEmpty())
	{
		strncpy(szSkin1, (char*)(LPCSTR)str, ARRAY_LEN(szSkin1));
	}

	str = buteMgr.GetString(aTagName, SCALEFX_SKIN2);
	if (!str.IsEmpty())
	{
		strncpy(szSkin2, (char*)(LPCSTR)str, ARRAY_LEN(szSkin2));
	}

	str = buteMgr.GetString(aTagName, SCALEFX_SKIN3);
	if (!str.IsEmpty())
	{
		strncpy(szSkin3, (char*)(LPCSTR)str, ARRAY_LEN(szSkin3));
	}

	// PHCShell.dll 0x1010902b
	fRotationY		= MATH_DEGREES_TO_RADIANS((LTFLOAT) buteMgr.GetDouble(aTagName, SCALEFX_ROTATIONY, 0.0));
	bAdditiveAlpha	= (LTBOOL) buteMgr.GetInt(aTagName, SCALEFX_ADDITIVEALPHA, 0);
#endif

	str = buteMgr.GetString(aTagName, SCALEFX_NAME);
	if (!str.IsEmpty())
	{
		strncpy(szName, (char*)(LPCSTR)str, ARRAY_LEN(szName));
	}

    return LTTRUE;
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CScaleFX::Cache()
//
//	PURPOSE:	Cache all the resources associated with the CScaleFX.
//
// ----------------------------------------------------------------------- //

void CScaleFX::Cache()
{
#ifndef _CLIENTBUILD  // No caching on the client...

	switch (eType)
	{
		case SCALEFX_MODEL :
		{
			if (szFile[0])
			{
                g_pLTServer->CacheFile(FT_MODEL, szFile);
			}

			if (szSkin[0])
			{
                g_pLTServer->CacheFile(FT_TEXTURE, szSkin);
			}
		}
		break;

		case SCALEFX_SPRITE :
		{
			if (szFile[0])
			{
                g_pLTServer->CacheFile(FT_SPRITE, szFile);
			}
		}
		break;

		default :
		break;
	}

#endif  // !_CLIENTBUILD
}




/////////////////////////////////////////////////////////////////////////////
//
//	P S H O W E R  F X  Stuff...
//
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	BuildPShowerFXList()
//
//	PURPOSE:	Build the list of particle shower fx structs
//
// ----------------------------------------------------------------------- //

LTBOOL BuildPShowerFXList(PShowerFXList & list, CButeMgr & buteMgr,
						 char* pTagBase)
{
    if (!pTagBase) return LTFALSE;

	int nNum = 0;
	sprintf(s_aTagName, "%s%d", pTagBase, nNum);

	// Read in the properties for each scale fx...

	while (buteMgr.Exist(s_aTagName))
	{
		CPShowerFX* pFX = new CPShowerFX;

		if (pFX && pFX->Init(buteMgr, s_aTagName))
		{
			pFX->nId = nNum;
			list.AddTail(pFX);
		}
		else
		{
			delete pFX;
            return LTFALSE;
		}

		nNum++;
		sprintf(s_aTagName, "%s%d", pTagBase, nNum);
	}

    return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPShowerFX::CPShowerFX()
//
//	PURPOSE:	Constructor
//
// ----------------------------------------------------------------------- //

CPShowerFX::CPShowerFX()
{
	nId					= -1;

	nMinParticles		= 0;
	nMaxParticles		= 0;
	fMinSize			= 0.0f;
	fMaxSize			= 0.0f;
	fMinVel				= 0.0f;
	fMaxVel				= 0.0f;
	fDirOffset			= 0.0f;
	fMinDuration		= 0.0f;
	fMaxDuration		= 0.0f;
	fFadeTime			= 0.0f;
	fEmissionRadius		= 0.0f;
	fRadius				= 0.0f;
	fGravity			= 0.0f;
	szTexture[0]		= '\0';
	szName[0]			= '\0';
	vColor1.Init();
	vColor2.Init();

    bAdditive           = LTFALSE;
    bMultiply           = LTFALSE;
	bRemoveIfNotLiquid	= LTFALSE;
}

#ifdef PRIMAL_HUNT

// Primal Hunt: the bute cache (MultiFile.h)

LTBOOL CPShowerFX::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->WriteInt(nId);
	pFile->WriteString(szName);
	pFile->WriteString(szTexture);
	pFile->WriteInt(nMinParticles);
	pFile->WriteInt(nMaxParticles);
	pFile->WriteFloat(fMinSize);
	pFile->WriteFloat(fMaxSize);
	pFile->WriteVector(vColor1);
	pFile->WriteVector(vColor2);
	pFile->WriteFloat(fMinVel);
	pFile->WriteFloat(fMaxVel);
	pFile->WriteFloat(fDirOffset);
	pFile->WriteFloat(fMinDuration);
	pFile->WriteFloat(fMaxDuration);
	pFile->WriteFloat(fFadeTime);
	pFile->WriteFloat(fEmissionRadius);
	pFile->WriteFloat(fRadius);
	pFile->WriteFloat(fGravity);
	pFile->WriteBool(bAdditive);
	pFile->WriteBool(bMultiply);
	pFile->WriteBool(bRemoveIfNotLiquid);

	return LTTRUE;
}

LTBOOL CPShowerFX::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->ReadInt(&nId);
	pFile->ReadString(szName, ARRAY_LEN(szName));
	pFile->ReadString(szTexture, ARRAY_LEN(szTexture));
	pFile->ReadInt(&nMinParticles);
	pFile->ReadInt(&nMaxParticles);
	pFile->ReadFloat(&fMinSize);
	pFile->ReadFloat(&fMaxSize);
	pFile->ReadVector(&vColor1);
	pFile->ReadVector(&vColor2);
	pFile->ReadFloat(&fMinVel);
	pFile->ReadFloat(&fMaxVel);
	pFile->ReadFloat(&fDirOffset);
	pFile->ReadFloat(&fMinDuration);
	pFile->ReadFloat(&fMaxDuration);
	pFile->ReadFloat(&fFadeTime);
	pFile->ReadFloat(&fEmissionRadius);
	pFile->ReadFloat(&fRadius);
	pFile->ReadFloat(&fGravity);
	pFile->ReadBool(&bAdditive);
	pFile->ReadBool(&bMultiply);
	pFile->ReadBool(&bRemoveIfNotLiquid);

	return LTTRUE;
}

#endif // PRIMAL_HUNT

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPShowerFX::Init
//
//	PURPOSE:	Init the particle shower fx struct from the bute mgr
//
// ----------------------------------------------------------------------- //

LTBOOL CPShowerFX::Init(CButeMgr & buteMgr, char* aTagName)
{
    if (!aTagName) return LTFALSE;

	nMinParticles		= buteMgr.GetInt(aTagName, PSHOWERFX_MINPARTICLES);
	nMaxParticles		= buteMgr.GetInt(aTagName, PSHOWERFX_MAXPARTICLES);
	fMinSize			= (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_MINSIZE);
	fMaxSize			= (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_MAXSIZE);
    fMinVel             = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_MINVEL);
    fMaxVel             = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_MAXVEL);
    fDirOffset          = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_DIROFFSET);
    fMinDuration        = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_MINDURATION);
    fMaxDuration        = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_MAXDURATION);
    fFadeTime	        = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_FADETIME);
    fEmissionRadius     = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_EMISSIONRADIUS);
    fRadius             = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_RADIUS);
    fGravity            = (LTFLOAT) buteMgr.GetDouble(aTagName, PSHOWERFX_GRAVITY);
	vColor1				= buteMgr.GetVector(aTagName, PSHOWERFX_COLOR1);
	vColor2				= buteMgr.GetVector(aTagName, PSHOWERFX_COLOR2);
    bAdditive           = (LTBOOL) buteMgr.GetInt(aTagName, PSHOWERFX_ADDITIVE);
    bMultiply           = (LTBOOL) buteMgr.GetInt(aTagName, PSHOWERFX_MULTIPLY);
    bRemoveIfNotLiquid  = (LTBOOL) buteMgr.GetInt(aTagName, PSHOWERFX_LIQUID, 0);

	CString str = buteMgr.GetString(aTagName, PSHOWERFX_TEXTURE);
	if (!str.IsEmpty())
	{
		strncpy(szTexture, (char*)(LPCSTR)str, ARRAY_LEN(szTexture));
	}

	str = buteMgr.GetString(aTagName, PSHOWERFX_NAME);
	if (!str.IsEmpty())
	{
		strncpy(szName, (char*)(LPCSTR)str, ARRAY_LEN(szName));
	}

    return LTTRUE;
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPShowerFX::Cache()
//
//	PURPOSE:	Cache all the resources associated with the CPShowerFX.
//
// ----------------------------------------------------------------------- //

void CPShowerFX::Cache()
{
#ifndef _CLIENTBUILD  // No caching on the client...

	if (szTexture[0])
	{
        if (g_pLTServer->CacheFile(FT_SPRITE, szTexture) != LT_OK)
		{
            g_pLTServer->CacheFile(FT_TEXTURE, szTexture);
		}
	}

#endif  // !_CLIENTBUILD
}





/////////////////////////////////////////////////////////////////////////////
//
//	P O L Y D E B R I S  F X  Stuff...
//
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	BuildPShowerFXList()
//
//	PURPOSE:	Build the list of particle shower fx structs
//
// ----------------------------------------------------------------------- //

LTBOOL BuildPolyDebrisFXList(PolyDebrisFXList & list, CButeMgr & buteMgr,
							char* pTagBase)
{
    if (!pTagBase) return LTFALSE;

	int nNum = 0;
	sprintf(s_aTagName, "%s%d", pTagBase, nNum);

	// Read in the properties for each scale fx...

	while (buteMgr.Exist(s_aTagName))
	{
		CPolyDebrisFX* pFX = new CPolyDebrisFX;

		if (pFX && pFX->Init(buteMgr, s_aTagName))
		{
			pFX->nId = nNum;
			list.AddTail(pFX);
		}
		else
		{
			delete pFX;
            return LTFALSE;
		}

		nNum++;
		sprintf(s_aTagName, "%s%d", pTagBase, nNum);
	}

    return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPolyDebrisFX::CPolyDebrisFX()
//
//	PURPOSE:	Constructor
//
// ----------------------------------------------------------------------- //

CPolyDebrisFX::CPolyDebrisFX()
{
	nId				= -1;

	szName[0]		= '\0';
	szTexture[0]	= '\0';

	nStyle			= 0;
	nMinDebris		= 0;
	nMaxDebris		= 0;
	nMinBounce		= 0;
	nMaxBounce		= 0;
    bAdditive       = LTFALSE;
    bMultiply       = LTFALSE;
    bShowTrail      = LTFALSE;
    bDirOffsetOnly  = LTFALSE;
	fDirOffset		= 0.0f;
	fMinDuration	= 0.0f;
	fMaxDuration	= 0.0f;
	fMinWidth		= 0.0f;
	fMaxWidth		= 0.0f;
	fMinLength		= 0.0f;
	fMaxLength		= 0.0f;
	fInitialAlpha	= 0.0f;
	fFinalAlpha		= 0.0f;
	fGravityScale	= 1.0f;
	vMinColor1.Init();
	vMaxColor1.Init();
	vMinColor2.Init();
	vMaxColor2.Init();
	vMinVel.Init();
	vMaxVel.Init();
	vMinDOffset.Init();
	vMaxDOffset.Init();
}

#ifdef PRIMAL_HUNT

// Primal Hunt: the bute cache (MultiFile.h)

LTBOOL CPolyDebrisFX::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->WriteInt(nId);
	pFile->WriteString(szName);
	pFile->WriteString(szTexture);
	pFile->WriteBool(bAdditive);
	pFile->WriteBool(bMultiply);
	pFile->WriteBool(bShowTrail);
	pFile->WriteBool(bDirOffsetOnly);
	pFile->WriteInt(nMinDebris);
	pFile->WriteInt(nMaxDebris);
	pFile->WriteInt(nMinBounce);
	pFile->WriteInt(nMaxBounce);
	pFile->WriteInt(nStyle);
	pFile->WriteVector(vMinColor1);
	pFile->WriteVector(vMaxColor1);
	pFile->WriteVector(vMinColor2);
	pFile->WriteVector(vMaxColor2);
	pFile->WriteVector(vMinVel);
	pFile->WriteVector(vMaxVel);
	pFile->WriteVector(vMinDOffset);
	pFile->WriteVector(vMaxDOffset);
	pFile->WriteFloat(fDirOffset);
	pFile->WriteFloat(fMinDuration);
	pFile->WriteFloat(fMaxDuration);
	pFile->WriteFloat(fMinWidth);
	pFile->WriteFloat(fMaxWidth);
	pFile->WriteFloat(fMinLength);
	pFile->WriteFloat(fMaxLength);
	pFile->WriteFloat(fInitialAlpha);
	pFile->WriteFloat(fFinalAlpha);
	pFile->WriteFloat(fGravityScale);

	return LTTRUE;
}

LTBOOL CPolyDebrisFX::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->ReadInt(&nId);
	pFile->ReadString(szName, ARRAY_LEN(szName));
	pFile->ReadString(szTexture, ARRAY_LEN(szTexture));
	pFile->ReadBool(&bAdditive);
	pFile->ReadBool(&bMultiply);
	pFile->ReadBool(&bShowTrail);
	pFile->ReadBool(&bDirOffsetOnly);
	pFile->ReadInt(&nMinDebris);
	pFile->ReadInt(&nMaxDebris);
	pFile->ReadInt(&nMinBounce);
	pFile->ReadInt(&nMaxBounce);
	pFile->ReadInt(&nStyle);
	pFile->ReadVector(&vMinColor1);
	pFile->ReadVector(&vMaxColor1);
	pFile->ReadVector(&vMinColor2);
	pFile->ReadVector(&vMaxColor2);
	pFile->ReadVector(&vMinVel);
	pFile->ReadVector(&vMaxVel);
	pFile->ReadVector(&vMinDOffset);
	pFile->ReadVector(&vMaxDOffset);
	pFile->ReadFloat(&fDirOffset);
	pFile->ReadFloat(&fMinDuration);
	pFile->ReadFloat(&fMaxDuration);
	pFile->ReadFloat(&fMinWidth);
	pFile->ReadFloat(&fMaxWidth);
	pFile->ReadFloat(&fMinLength);
	pFile->ReadFloat(&fMaxLength);
	pFile->ReadFloat(&fInitialAlpha);
	pFile->ReadFloat(&fFinalAlpha);
	pFile->ReadFloat(&fGravityScale);

	return LTTRUE;
}

#endif // PRIMAL_HUNT

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPolyDebrisFX::Init
//
//	PURPOSE:	Init the particle shower fx struct from the bute mgr
//
// ----------------------------------------------------------------------- //

LTBOOL CPolyDebrisFX::Init(CButeMgr & buteMgr, char* aTagName)
{
    if (!aTagName) return LTFALSE;

	nStyle			= buteMgr.GetInt(aTagName, POLYDEBRISFX_STYLE);
	nMinDebris		= buteMgr.GetInt(aTagName, POLYDEBRISFX_MINDEBRIS);
	nMaxDebris		= buteMgr.GetInt(aTagName, POLYDEBRISFX_MAXDEBRIS);
	nMinBounce		= buteMgr.GetInt(aTagName, POLYDEBRISFX_MINBOUNCE);
	nMaxBounce		= buteMgr.GetInt(aTagName, POLYDEBRISFX_MAXBOUNCE);
    bAdditive       = (LTBOOL) buteMgr.GetInt(aTagName, POLYDEBRISFX_ADDITIVE);
    bMultiply       = (LTBOOL) buteMgr.GetInt(aTagName, POLYDEBRISFX_MULTIPLY);
    bShowTrail      = (LTBOOL) buteMgr.GetInt(aTagName, POLYDEBRISFX_SHOWTRAIL);
    bDirOffsetOnly  = (LTBOOL) buteMgr.GetInt(aTagName, POLYDEBRISFX_DIROFFSETONLY);
    fDirOffset      = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_DIROFFSET);
    fGravityScale   = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_GRAVITYSCALE);
    fMinDuration    = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_MINDURATION);
    fMaxDuration    = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_MAXDURATION);
    fInitialAlpha   = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_INITIALALPHA);
    fFinalAlpha     = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_FINALALPHA);
    fMinWidth       = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_MINWIDTH);
    fMaxWidth       = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_MAXWIDTH);
    fMinLength      = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_MINLENGTH);
    fMaxLength      = (LTFLOAT) buteMgr.GetDouble(aTagName, POLYDEBRISFX_MAXLENGTH);
	vMinColor1		= buteMgr.GetVector(aTagName, POLYDEBRISFX_MINCOLOR1);
	vMaxColor1		= buteMgr.GetVector(aTagName, POLYDEBRISFX_MAXCOLOR1);
	vMinColor2		= buteMgr.GetVector(aTagName, POLYDEBRISFX_MINCOLOR2);
	vMaxColor2		= buteMgr.GetVector(aTagName, POLYDEBRISFX_MAXCOLOR2);
	vMinVel			= buteMgr.GetVector(aTagName, POLYDEBRISFX_MINVEL);
	vMaxVel			= buteMgr.GetVector(aTagName, POLYDEBRISFX_MAXVEL);
	vMinDOffset		= buteMgr.GetVector(aTagName, POLYDEBRISFX_MINDOFFSET);
	vMaxDOffset		= buteMgr.GetVector(aTagName, POLYDEBRISFX_MAXDOFFSET);

	CString str = buteMgr.GetString(aTagName, POLYDEBRISFX_NAME);
	if (!str.IsEmpty())
	{
		strncpy(szName, (char*)(LPCSTR)str, ARRAY_LEN(szName));
	}

	str = buteMgr.GetString(aTagName, POLYDEBRISFX_TEXTURE);
	if (!str.IsEmpty())
	{
		strncpy(szTexture, (char*)(LPCSTR)str, ARRAY_LEN(szTexture));
	}

    return LTTRUE;
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPolyDebrisFX::Cache()
//
//	PURPOSE:	Cache all the resources associated with the CPolyDebrisFX.
//
// ----------------------------------------------------------------------- //

void CPolyDebrisFX::Cache()
{
#ifndef _CLIENTBUILD  // No caching on the client...

	if (szTexture[0])
	{
		g_pLTServer->CacheFile(FT_TEXTURE, szTexture);
	}

#endif  // !_CLIENTBUILD
}


