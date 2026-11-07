// ----------------------------------------------------------------------- //
//
// MODULE  : ChameleonTest.cpp
//
// PURPOSE : ChameleonTest - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x1014edc0..0x1014f075.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "ChameleonTest.h"

BEGIN_CLASS(ChameleonTest)
END_CLASS_DEFAULT(ChameleonTest, GameBase, LTNULL, LTNULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ChameleonTest::OnPrecreate (0x1014eee0)
//
// ----------------------------------------------------------------------- //

uint32 ChameleonTest::OnPrecreate(void *pData, uint32 nInfo)
{
	GameBase::OnPrecreate(pData, nInfo);

	ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;
	if (!pStruct) return LTFALSE;

	pStruct->m_Flags = FLAG_VISIBLE | FLAG_SOLID;
	SAFE_STRCPY(pStruct->m_Filename, "models/characters/heavypred.abc");
	SAFE_STRCPY(pStruct->m_SkinNames[0], "skins/characters/predator/heavypred_torso.dtx");
	SAFE_STRCPY(pStruct->m_SkinNames[1], "skins/characters/predator/heavypred_legs.dtx");
	SAFE_STRCPY(pStruct->m_SkinNames[2], "skins/characters/predator/heavypred_head.dtx");
	SAFE_STRCPY(pStruct->m_SkinNames[3], "skins/characters/predator/heavypred_arms.dtx");

	pStruct->m_Flags |= FLAG_ENVIRONMENTMAP;
	pStruct->m_Flags2 = 0x1000;		// PH's FLAG2 bit 12 (name unknown)
	SAFE_STRCPY(pStruct->m_ExtraTextureName, "WorldTextures/exp/envmap/plasma1.dtx");

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ChameleonTest::OnInitialUpdate (0x1014efb0)
//
// ----------------------------------------------------------------------- //

uint32 ChameleonTest::OnInitialUpdate(uint32 nInfo)
{
	GameBase::OnInitialUpdate(nInfo);

	SetNextUpdate(0.001f);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ChameleonTest::OnUpdate (0x1014efe0)
//
// ----------------------------------------------------------------------- //

uint32 ChameleonTest::OnUpdate(LTFLOAT fFrameTime)
{
	uint32 dwFlags2;
	LTFLOAT r, g, b, a;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags2, dwFlags2);

	// PH's FLAG2 bits 11 and 12 (names unknown)
	if (dwFlags2 & 0x1800)
	{
		m_fRotation += fFrameTime * MATH_HALFPI;

		g_pLTServer->GetObjectColor(m_hObject, &r, &g, &b, &a);
		g_pLTServer->SetObjectColor(m_hObject, r, g, b, 0.0f);

		SetNextUpdate(0.001f);
	}

	return LTTRUE;
}

#endif // PRIMAL_HUNT
