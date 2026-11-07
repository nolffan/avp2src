// ----------------------------------------------------------------------- //
//
// MODULE  : VolumetricFog.cpp
//
// PURPOSE : VolumetricFog - implementation
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x1015b530..0x1015bb10)
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "VolumetricFog.h"
#include <math.h>

// The user flags the client's fog volume reads: which of this object's
// values to use instead of WorldProperties' globals, and the density
// packed into bits 4..17.
#define VFOG_USE_COLOR			(1<<0)
#define VFOG_USE_DENSITY		(1<<1)
#define VFOG_DENSITY_MASK		0x0003fff0

// PH: the $E initializer 0x1015b530
BEGIN_CLASS(VolumetricFog)
	ADD_BOOLPROP_FLAG_HELP(Use_Fog_Color, LTFALSE, 0, "Set to TRUE to use the Fog_Color value on this object.  If FALSE, the global fog color from WorldProperties will be used.")
	ADD_COLORPROP_FLAG_HELP(Fog_Color, 0.0f, 0.0f, 0.0f, 0, "Specifies the color of the fog volume.  If two fog volumes are in the PVS at once, the color used is that of the last one drawn, so avoid having two differently colored fog volumes in the PVS at once.")
	ADD_BOOLPROP_FLAG_HELP(Use_Fog_Density, LTFALSE, 0, "Set to TRUE to use the Fog_Density value on this object.  If FALSE, the global VFogDensity from WorldProperties will be used.")
	ADD_REALPROP_FLAG_HELP(Fog_Density, 512.0f, 0, "The thickness of fog at which a vertex is completely fogged out (to the fog color).  For instance, if the value is 512.0, there must be 512 or more units of fog between the viewer and a vertex for the vertex to be completely fogged out.")
END_CLASS_DEFAULT(VolumetricFog, GameBase, NULL, NULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	VolumetricFog::VolumetricFog()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015b770
VolumetricFog::VolumetricFog() : GameBase(OT_WORLDMODEL)
{
	m_bFading = LTFALSE;
}

// PH: 0x1015b7b0
VolumetricFog::~VolumetricFog()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	VolumetricFog::OnPrecreate()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015b7c0
uint32 VolumetricFog::OnPrecreate(void *pData, uint32 nInfo)
{
	if (nInfo == PRECREATE_WORLDFILE)
	{
		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

		SAFE_STRCPY(pStruct->m_Filename, pStruct->m_Name);
		pStruct->m_Flags |= FLAG_VISIBLE | FLAG_GOTHRUWORLD | FLAG_FORCECLIENTUPDATE;
		pStruct->m_UserFlags = 0;

		LTBOOL bUseColor = LTFALSE;
		g_pLTServer->GetPropBool("Use_Fog_Color", &bUseColor);

		if (bUseColor == LTTRUE)
		{
			pStruct->m_UserFlags |= VFOG_USE_COLOR;
			g_pLTServer->GetPropColor("Fog_Color", &m_vFogColor);
			m_vFogColor *= MATH_ONE_OVER_255;
		}
		else
		{
			m_vFogColor.Init(1.0f, 1.0f, 1.0f);
		}

		LTBOOL bUseDensity = LTFALSE;
		g_pLTServer->GetPropBool("Use_Fog_Density", &bUseDensity);

		if (bUseDensity == LTTRUE)
		{
			pStruct->m_UserFlags |= VFOG_USE_DENSITY;
			g_pLTServer->GetPropReal("Fog_Density", &m_fFogDensity);
		}
		else
		{
			m_fFogDensity = 512.0f;
		}

		pStruct->m_Flags2 |= 0x8000;	// Talon's volumetric-fog world model flag
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	VolumetricFog::OnInitialUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015b920
uint32 VolumetricFog::OnInitialUpdate(uint32 nInfo)
{
	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		g_pLTServer->SetObjectColor(m_hObject, m_vFogColor.x, m_vFogColor.y, m_vFogColor.z, 1.0f);

		if (g_pLTServer->GetObjectUserFlags(m_hObject) & VFOG_USE_DENSITY)
		{
			LTFLOAT fDensity = m_fFogDensity;
			uint32 dwUserFlags = g_pLTServer->GetObjectUserFlags(m_hObject);
			dwUserFlags = (dwUserFlags & ~VFOG_DENSITY_MASK) | ((uint32)ceil(fDensity) << 2);
			g_pLTServer->SetObjectUserFlags(m_hObject, dwUserFlags);
		}
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	VolumetricFog::OnUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015b9e0
uint32 VolumetricFog::OnUpdate(LTFLOAT fFrameTime)
{
	if (m_bFading == LTTRUE)
	{
		LTFLOAT fAlpha;

		m_fFadeTimer += fFrameTime;
		if (m_fFadeTimer >= m_fFadeTime)
		{
			m_bFading = LTFALSE;
			fAlpha = m_fEndAlpha;
		}
		else
		{
			fAlpha = m_fStartAlpha + (m_fEndAlpha - m_fStartAlpha) * (m_fFadeTimer / m_fFadeTime);
			SetNextUpdate(0.001f);
		}

		// RETAIL BUG: the red channel is given the blue one, so a fading
		// fog volume turns magenta or cyan unless its color is grey
		g_pLTServer->SetObjectColor(m_hObject, m_vFogColor.z, m_vFogColor.y, m_vFogColor.z, fAlpha);
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	VolumetricFog::OnTrigger()
//
//	PURPOSE:	FADEALPHA <start> <end> <time>
//
// ----------------------------------------------------------------------- //

// PH: 0x1015ba60
uint32 VolumetricFog::OnTrigger(HOBJECT hSender, const char* szMsg)
{
	GameBase::OnTrigger(hSender, szMsg);

	ConParse parse;
	parse.Init(const_cast<char*>(szMsg));

	while (g_pLTServer->Common()->Parse(&parse) == LT_OK)
	{
		if (parse.m_nArgs && _stricmp(parse.m_Args[0], "FADEALPHA") == 0)
		{
			m_fStartAlpha	= (LTFLOAT)atof(parse.m_Args[1]);
			m_fEndAlpha		= (LTFLOAT)atof(parse.m_Args[2]);
			m_fFadeTime		= (LTFLOAT)atof(parse.m_Args[3]);
			m_bFading		= LTTRUE;
			m_fFadeTimer	= 0.0f;

			SetNextUpdate(0.001f);
		}
	}

	return 0;
}

#endif // PRIMAL_HUNT
