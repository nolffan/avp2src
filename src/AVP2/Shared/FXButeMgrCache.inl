#ifdef PRIMAL_HUNT

// ----------------------------------------------------------------------- //
//
//	Primal Hunt: the bute cache (MultiFile.h). These structures point to
//	others, and write them after themselves, each behind a byte saying
//	whether it is there (PHCShell.dll 0x100c5680..0x100c64ad).
//
// ----------------------------------------------------------------------- //

inline PROJECTILEFX::~PROJECTILEFX()
{
	// What Read built belongs to this one (PHObject.lto 0x1001f9d0)
	if (bFromFile == LTTRUE)
	{
		if (pClassData)
		{
			delete pClassData;
			pClassData = LTNULL;
		}
	}
}

LTBOOL PROJECTILEFX::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->WriteString(szName);
	pFile->WriteString(szFlareSprite);
	pFile->WriteString(szSound);
	pFile->WriteString(szClass);
	pFile->WriteString(szModel);
	pFile->WriteString(szSkin);
	pFile->WriteString(szToggleScaleFX);
	pFile->WriteString(szSocket);
	pFile->WriteBool(bNoLighting);
	pFile->WriteFloat(fModelAlpha);
	pFile->WriteInt(nAltVelocity);
	pFile->WriteInt(nVelocity);
	pFile->WriteFloat(fLifeTime);
	pFile->WriteInt(nFlags);
	pFile->WriteInt(nParticleTrail);
	pFile->WriteInt(nUWParticleTrail);
	pFile->WriteVector(vLightColor);
	pFile->WriteInt(nLightRadius);
	pFile->WriteInt(nSoundRadius);
	pFile->WriteFloat(fFlareScale);
	pFile->WriteDWord(dwObjectFlags);
	pFile->WriteVector(vModelScale);

	if (!pClassData)
	{
		pFile->WriteByte(LTFALSE);
	}
	else
	{
		pFile->WriteByte(LTTRUE);
		pClassData->Write(pFile);
	}

	return LTTRUE;
}

LTBOOL PROJECTILEFX::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->ReadString(szName, ARRAY_LEN(szName));
	pFile->ReadString(szFlareSprite, ARRAY_LEN(szFlareSprite));
	pFile->ReadString(szSound, ARRAY_LEN(szSound));
	pFile->ReadString(szClass, ARRAY_LEN(szClass));
	pFile->ReadString(szModel, ARRAY_LEN(szModel));
	pFile->ReadString(szSkin, ARRAY_LEN(szSkin));
	pFile->ReadString(szToggleScaleFX, ARRAY_LEN(szToggleScaleFX));
	pFile->ReadString(szSocket, ARRAY_LEN(szSocket));
	pFile->ReadBool(&bNoLighting);
	pFile->ReadFloat(&fModelAlpha);
	pFile->ReadInt(&nAltVelocity);
	pFile->ReadInt(&nVelocity);
	pFile->ReadFloat(&fLifeTime);
	pFile->ReadInt(&nFlags);
	pFile->ReadInt(&nParticleTrail);
	pFile->ReadInt(&nUWParticleTrail);
	pFile->ReadVector(&vLightColor);
	pFile->ReadInt(&nLightRadius);
	pFile->ReadInt(&nSoundRadius);
	pFile->ReadFloat(&fFlareScale);
	pFile->ReadDWord(&dwObjectFlags);
	pFile->ReadVector(&vModelScale);

	pClassData = LTNULL;

	LTBOOL bClassData = pFile->ReadByte();
	if (bClassData)
	{
		// RETAIL BUG: a class type of 0 leaves pClassData null, and it is
		// read through all the same.
		switch (pFile->ReadByte())
		{
			case 0:
			break;

			case 1:
				pClassData = new PROJECTILECLASSDATA;
			break;

			case 2:
				pClassData = new PROXCLASSDATA;
			break;

			case 3:
				pClassData = new SPIDERCLASSDATA;
			break;

			case 4:
				pClassData = new TRACKINGSADARCLASSDATA;
			break;

			case 5:
				pClassData = new PREDDISKCLASSDATA;
			break;

			default:
				pClassData = LTNULL;
			break;
		}

#ifdef AVP2SRC_MODERN
		// M-B72 (RETAIL_BUGS.md): no class data made, none read (kind 0
		// is written as its kind alone)
		if (pClassData)
			pClassData->Read(pFile);
#else
		pClassData->Read(pFile);
#endif
	}

	bFromFile = LTTRUE;

	return LTTRUE;
}

inline IMPACTFX::~IMPACTFX()
{
	// What Read built belongs to this one (PHObject.lto 0x1001ff30)
	if (bFromFile == LTTRUE)
	{
		if (pPusherFX)
		{
			delete pPusherFX;
			pPusherFX = LTNULL;
		}
	}
}

LTBOOL IMPACTFX::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	int i;

	pFile->WriteString(szName);
	pFile->WriteString(szSound);
	pFile->WriteString(szMark);
	pFile->WriteString(szSoundDir);
	pFile->WriteInt(nSoundRadius);
	pFile->WriteInt(nSoundBute);
	pFile->WriteInt(nAISoundRadius);
	pFile->WriteInt(nFlags);
	pFile->WriteFloat(fMarkScale);
	pFile->WriteVector(vTintColor);
	pFile->WriteFloat(fTintRampUp);
	pFile->WriteFloat(fTintRampDown);
	pFile->WriteFloat(fTintMaxTime);
	pFile->WriteFloat(fTintRadius);
	pFile->WriteBool(bDoSurfaceFX);

	if (!pPusherFX)
	{
		pFile->WriteByte(LTFALSE);
	}
	else
	{
		pFile->WriteByte(LTTRUE);
		pPusherFX->Write(pFile);
	}

	pFile->WriteInt(nNumDebrisFXTypes);
	for (i=0; i < nNumDebrisFXTypes; i++)
	{
		pFile->WriteInt(aDebrisFXTypes[i]);
	}

	pFile->WriteInt(nNumScaleFXTypes);
	for (i=0; i < nNumScaleFXTypes; i++)
	{
		pFile->WriteInt(aScaleFXTypes[i]);
	}

	pFile->WriteInt(nNumPExplFX);
	for (i=0; i < nNumPExplFX; i++)
	{
		pFile->WriteInt(aPExplFXTypes[i]);
	}

	pFile->WriteInt(nNumDLightFX);
	for (i=0; i < nNumDLightFX; i++)
	{
		pFile->WriteInt(aDLightFXTypes[i]);
	}

	pFile->WriteInt(nNumPolyDebrisFX);
	for (i=0; i < nNumPolyDebrisFX; i++)
	{
		pFile->WriteInt(aPolyDebrisFXTypes[i]);
	}

	pFile->WriteInt(nNumPShowerFX);
	for (i=0; i < nNumPShowerFX; i++)
	{
		pFile->WriteInt(aPShowerFXTypes[i]);
	}

	return LTTRUE;
}

LTBOOL IMPACTFX::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	int i = 0;

	pFile->ReadString(szName, ARRAY_LEN(szName));
	pFile->ReadString(szSound, ARRAY_LEN(szSound));
	pFile->ReadString(szMark, ARRAY_LEN(szMark));
	pFile->ReadString(szSoundDir, ARRAY_LEN(szSoundDir));
	pFile->ReadInt(&nSoundRadius);
	pFile->ReadInt(&nSoundBute);
	pFile->ReadInt(&nAISoundRadius);
	pFile->ReadInt(&nFlags);
	pFile->ReadFloat(&fMarkScale);
	pFile->ReadVector(&vTintColor);
	pFile->ReadFloat(&fTintRampUp);
	pFile->ReadFloat(&fTintRampDown);
	pFile->ReadFloat(&fTintMaxTime);
	pFile->ReadFloat(&fTintRadius);
	pFile->ReadBool(&bDoSurfaceFX);

	pPusherFX = LTNULL;
	if (pFile->ReadByte() == LTTRUE)
	{
		pPusherFX = new PUSHERFX;
		pPusherFX->Read(pFile);
	}

	pFile->ReadInt(&nNumDebrisFXTypes);
	for (i=0; i < nNumDebrisFXTypes; i++)
	{
		pFile->ReadInt(&aDebrisFXTypes[i]);
	}

	pFile->ReadInt(&nNumScaleFXTypes);
	for (i=0; i < nNumScaleFXTypes; i++)
	{
		pFile->ReadInt(&aScaleFXTypes[i]);
	}

	pFile->ReadInt(&nNumPExplFX);
	for (i=0; i < nNumPExplFX; i++)
	{
		pFile->ReadInt(&aPExplFXTypes[i]);
	}

	pFile->ReadInt(&nNumDLightFX);
	for (i=0; i < nNumDLightFX; i++)
	{
		pFile->ReadInt(&aDLightFXTypes[i]);
	}

	pFile->ReadInt(&nNumPolyDebrisFX);
	for (i=0; i < nNumPolyDebrisFX; i++)
	{
		pFile->ReadInt(&aPolyDebrisFXTypes[i]);
	}

	pFile->ReadInt(&nNumPShowerFX);
	for (i=0; i < nNumPShowerFX; i++)
	{
		pFile->ReadInt(&aPShowerFXTypes[i]);
	}

	bFromFile = LTTRUE;

	return LTTRUE;
}

inline FIREFX::~FIREFX()
{
	// What Read built belongs to this one (PHObject.lto 0x100206d0)
	if (bFromFile == LTTRUE)
	{
		for (int i=0; i < FIRE_MAX_BEAMFX; i++)
		{
			if (pBeamFX[i])
			{
				delete pBeamFX[i];
				pBeamFX[i] = LTNULL;
			}
		}
	}
}

LTBOOL FIREFX::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->WriteInt(nId);
	pFile->WriteString(szName);
	pFile->WriteString(szShellModel);
	pFile->WriteString(szShellSkin);
	pFile->WriteInt(nFlags);
	pFile->WriteVector(vShellScale);

	pFile->WriteInt(nNumBeamFX);
	for (int i=0; i < nNumBeamFX; i++)
	{
		pBeamFX[i]->Write(pFile);
	}

	return LTTRUE;
}

LTBOOL FIREFX::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->ReadInt(&nId);
	pFile->ReadString(szName, ARRAY_LEN(szName));
	pFile->ReadString(szShellModel, ARRAY_LEN(szShellModel));
	pFile->ReadString(szShellSkin, ARRAY_LEN(szShellSkin));
	pFile->ReadInt(&nFlags);
	pFile->ReadVector(&vShellScale);

	pFile->ReadInt(&nNumBeamFX);
	memset(pBeamFX, 0, sizeof(pBeamFX));
	for (int i=0; i < nNumBeamFX; i++)
	{
		pBeamFX[i] = new BEAMFX;
		pBeamFX[i]->Read(pFile);
	}

	bFromFile = LTTRUE;

	return LTTRUE;
}

inline CMuzzleFX::~CMuzzleFX()
{
	// What Read built belongs to this one (PHObject.lto 0x10021080)
	if (bFromFile == LTTRUE)
	{
		if (pPMuzzleFX)
		{
			delete pPMuzzleFX;
			pPMuzzleFX = LTNULL;
		}

		if (pScaleFX)
		{
			delete pScaleFX;
			pScaleFX = LTNULL;
		}

		if (pDLightFX)
		{
			delete pDLightFX;
			pDLightFX = LTNULL;
		}
	}
}

LTBOOL CMuzzleFX::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->WriteInt(nId);
	pFile->WriteString(szName);
	pFile->WriteFloat(fDuration);

	if (!pPMuzzleFX)
	{
		pFile->WriteByte(LTFALSE);
		pPMuzzleFX = LTNULL;
	}
	else
	{
		pFile->WriteByte(LTTRUE);
		pPMuzzleFX->Write(pFile);
	}

	if (!pScaleFX)
	{
		pFile->WriteByte(LTFALSE);
		pScaleFX = LTNULL;
	}
	else
	{
		pFile->WriteByte(LTTRUE);
		pScaleFX->Write(pFile);
	}

	if (!pDLightFX)
	{
		pFile->WriteByte(LTFALSE);
		pDLightFX = LTNULL;
	}
	else
	{
		pFile->WriteByte(LTTRUE);
		pDLightFX->Write(pFile);
	}

	return LTTRUE;
}

LTBOOL CMuzzleFX::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->ReadInt(&nId);
	pFile->ReadString(szName, ARRAY_LEN(szName));
	pFile->ReadFloat(&fDuration);

	if (pFile->ReadByte() == LTTRUE)
	{
		pPMuzzleFX = new CParticleMuzzleFX;
		pPMuzzleFX->Read(pFile);
	}

	if (pFile->ReadByte() == LTTRUE)
	{
		pScaleFX = new CScaleFX;
		pScaleFX->Read(pFile);
	}

	if (pFile->ReadByte() == LTTRUE)
	{
		pDLightFX = new DLIGHTFX;
		pDLightFX->Read(pFile);
	}

	return LTTRUE;
}

#endif // PRIMAL_HUNT
