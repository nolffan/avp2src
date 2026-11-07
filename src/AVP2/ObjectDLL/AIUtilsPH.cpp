// ----------------------------------------------------------------------- //
//
// MODULE  : AIUtilsPH.cpp
//
// PURPOSE : CAIBase's sound player (CAISounds) and animation player
//           (CAIAnimator)
//
// ----------------------------------------------------------------------- //

// PH: Third Law's code, PHObject.lto 0x1014d660..0x1014ddff, linked
// between CAITentacle's file and CArtifactField's. The file's name is a
// guess (retail does not record it; the new files link in the order of
// their names); the classes are declared in AIBase.h.

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAISounds::PlaySound()
//
//	PURPOSE:	Play a sound at a position
//
// ----------------------------------------------------------------------- //

// PH: 0x1014d660. RETAIL BUG: dwFlags is never used; the sound takes this
// player's flags.
HLTSOUND CAISounds::PlaySound(const char* szSound, const LTVector& vPos, int nVolume, LTFLOAT fPitch,
							  uint8 nPriority, LTFLOAT fInner, LTFLOAT fOuter, uint32 dwFlags)
{
	PlaySoundInfo psi;
	PLAYSOUNDINFO_INIT(psi);

	psi.m_dwFlags = m_dwFlags;
	if (nVolume != 100)
	{
		psi.m_dwFlags |= PLAYSOUND_CTRL_VOL;
	}
	psi.m_nVolume = (uint8)nVolume;

	if (fPitch != 0.0f)
	{
		psi.m_dwFlags |= PLAYSOUND_CTRL_PITCH;
		psi.m_fPitchShift = fPitch;
	}

	psi.m_nPriority		= nPriority;
	psi.m_fInnerRadius	= fInner;
	psi.m_fOuterRadius	= fOuter;
	psi.m_vPosition		= vPos;

	strncpy(psi.m_szSoundName, szSound, _MAX_PATH + 1);
	psi.m_szSoundName[_MAX_PATH] = '\0';

	g_pLTServer->PlaySound(&psi);

	return psi.m_hSound;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAISounds::Play()
//
//	PURPOSE:	Play a sound on the AI, and note when it will end
//
// ----------------------------------------------------------------------- //

// PH: 0x1014d750
HLTSOUND CAISounds::PlayVoice(const char* szSound)
{
	PlaySoundInfo psi;
	PLAYSOUNDINFO_INIT(psi);

	psi.m_dwFlags = m_dwFlags;
	if (m_nVolume != 100)
	{
		psi.m_dwFlags |= PLAYSOUND_CTRL_VOL;
	}
	psi.m_nVolume = (uint8)m_nVolume;

	if (m_fPitch != 0.0f)
	{
		psi.m_dwFlags |= PLAYSOUND_CTRL_PITCH;
		psi.m_fPitchShift = m_fPitch;
	}

	psi.m_nPriority		= m_nPriority;
	psi.m_fInnerRadius	= m_fInner;
	psi.m_fOuterRadius	= m_fOuter;
	psi.m_hObject		= m_hObject;

	if (m_hObject)
	{
		psi.m_dwFlags |= PLAYSOUND_ATTACHED;
	}

	if (psi.m_dwFlags & PLAYSOUND_AMBIENT)
	{
		g_pLTServer->GetObjectPos(m_hObject, &psi.m_vPosition);
	}

	strncpy(psi.m_szSoundName, szSound, _MAX_PATH + 1);
	psi.m_szSoundName[_MAX_PATH] = '\0';

	g_pLTServer->PlaySound(&psi);

	LTFLOAT fDuration;
	g_pLTServer->SoundMgr()->GetSoundDuration(psi.m_hSound, fDuration);
	m_fSoundEnd = g_pLTServer->GetTime() + fDuration;

	return psi.m_hSound;
}

// PH: 0x1014d880
HLTSOUND CAISounds::PlayPain(const char* szSound)
{
	if (g_pLTServer->GetTime() < m_fNextPain) return LTNULL;
	if (g_pLTServer->GetTime() < m_fSoundEnd) return LTNULL;

	HLTSOUND hSound = PlayVoice(szSound);
	m_fNextPain = m_fSoundEnd;
	return hSound;
}

// PH: 0x1014d8d0
HLTSOUND CAISounds::PlayAlert(const char* szSound)
{
	if (g_pLTServer->GetTime() < m_fNextAlert) return LTNULL;
	if (g_pLTServer->GetTime() < m_fSoundEnd) return LTNULL;

	HLTSOUND hSound = PlayVoice(szSound);
	m_fNextAlert = m_fSoundEnd;
	return hSound;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIAnimator::~CAIAnimator()
//
// ----------------------------------------------------------------------- //

// PH: 0x1014d920 (CSpearList's, the same, is folded into it)
CAIAnimator::CAIAnimator()
{
}

// PH: 0x1014d940
CAIAnimator::~CAIAnimator()
{
	Clear();
}

// PH: 0x1014d9c0
void CAIAnimator::Clear()
{
	m_Trackers.DeleteAll();
}

// PH: 0x1014da00
void CAIAnimator::Init(HOBJECT hObj)
{
	Clear();
	m_hObject = hObj;
	AddTracker();
}

// PH: 0x1014da20
HMODELANIM CAIAnimator::PlayAnim(const char* szAnim, LTBOOL bLoop, uint32 nTracker)
{
	HMODELANIM hAnim = FindAnim(szAnim);
	PlayAnim(hAnim, bLoop, nTracker);
	return hAnim;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIAnimator::PlayAnim()
//
//	PURPOSE:	Play an animation on a tracker (only fix its looping if it
//				is already playing)
//
// ----------------------------------------------------------------------- //

// PH: 0x1014da50
void CAIAnimator::PlayAnim(HMODELANIM hAnim, LTBOOL bLoop, uint32 nTracker)
{
	AITracker* pTracker = FindTracker(nTracker);
	if (!pTracker) return;

	if (GetAnim(nTracker) == hAnim)
	{
		if ((g_pModelLT->GetLooping(pTracker->m_pTracker) == LT_YES) != bLoop)
		{
			g_pModelLT->SetLooping(pTracker->m_pTracker, bLoop);
		}
		return;
	}

	g_pModelLT->SetLooping(pTracker->m_pTracker, bLoop);
	g_pModelLT->SetCurAnim(pTracker->m_pTracker, hAnim);
	g_pModelLT->SetPlaying(pTracker->m_pTracker, LTTRUE);
}

// PH: 0x1014dae0
HMODELANIM CAIAnimator::GetAnim(int nTracker)
{
	AITracker* pTracker = FindTracker(nTracker);
	if (!pTracker) return INVALID_MODEL_ANIM;

	HMODELANIM hAnim;
	if (g_pModelLT->GetCurAnim(pTracker->m_pTracker, hAnim) != LT_OK) return INVALID_MODEL_ANIM;

	return hAnim;
}

// PH: 0x1014db20
HMODELANIM CAIAnimator::FindAnim(const char* szAnim)
{
	return g_pLTServer->GetAnimIndex(m_hObject, (char*)szAnim);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIAnimator::AddTracker()
//
//	PURPOSE:	Add a tracker: the first is the model's main tracker
//
// ----------------------------------------------------------------------- //

// PH: 0x1014db40
uint32 CAIAnimator::AddTracker()
{
	if (!m_hObject) return -1;

	AITracker* pTracker = new AITracker;

	if (m_Trackers.GetCount() == 0)
	{
		g_pModelLT->GetMainTracker(m_hObject, pTracker->m_pTracker);
		pTracker->m_nID = 0;
		m_Trackers.AddHead(pTracker);
		return pTracker->m_nID;
	}

	pTracker->m_pTracker = &pTracker->m_Tracker;
	pTracker->m_nID = (int)&pTracker->m_Tracker;

	if (g_pModelLT->AddTracker(m_hObject, &pTracker->m_Tracker) != LT_OK)
	{
		delete pTracker;
		return -1;
	}

	m_Trackers.AddTail(pTracker);
	return pTracker->m_nID;
}

// PH: 0x1014dc40
AITracker* CAIAnimator::FindTracker(int nTracker)
{
	for (TLinkList<AITracker*>::Node* pNode = m_Trackers.GetHead(); pNode != m_Trackers.GetEnd(); pNode = pNode->m_pNext)
	{
		if (pNode->m_Data && pNode->m_Data->m_nID == nTracker)
		{
			return pNode->m_Data;
		}
	}

	return LTNULL;
}

// PH: 0x1014dc70
void CAIAnimator::SetAllowTransition(int nTracker, LTBOOL bAllow)
{
	AITracker* pTracker = FindTracker(nTracker);
	if (!pTracker) return;

	g_pModelLT->SetAllowTransition(pTracker->m_pTracker, bAllow);
}

// PH: 0x1014dca0
uint32 CAIAnimator::FindWeightSet(const char* szName)
{
	HMODELWEIGHTSET hSet = INVALID_MODEL_WEIGHTSET;
	g_pModelLT->FindWeightSet(m_hObject, (char*)szName, hSet);
	return hSet;
}

// PH: 0x1014dcd0
void CAIAnimator::SetWeightSet(uint32 hSet, uint32 nTracker)
{
	AITracker* pTracker = FindTracker(nTracker);
	if (!pTracker) return;

	g_pModelLT->SetWeightSet(pTracker->m_pTracker, hSet);
}

// PH: 0x1014dd00
uint32 CAIAnimator::GetPlaybackState(int nTracker)
{
	AITracker* pTracker = FindTracker(nTracker);
	if (!pTracker) return 0;

	uint32 dwState;
	g_pModelLT->GetPlaybackState(pTracker->m_pTracker, dwState);
	return dwState;
}

// PH: 0x1014dd30
LTFLOAT CAIAnimator::GetAnimLength(int nTracker)
{
	AITracker* pTracker = FindTracker(nTracker);
	if (!pTracker) return 0.0f;

	uint32 dwLength;
	g_pModelLT->GetCurAnimLength(pTracker->m_pTracker, dwLength, LTTRUE);
	return (LTFLOAT)dwLength * 0.001f;
}

// PH: 0x1014dd90
LTBOOL CAIAnimator::IsLooping(int nTracker)
{
	AITracker* pTracker = FindTracker(nTracker);
	if (!pTracker) return LTFALSE;

	return g_pModelLT->GetLooping(pTracker->m_pTracker) == LT_YES;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIAnimator::Save() / Load()
//
//	PURPOSE:	Nothing: the trackers are set up again by the AI. Both are
//				empty, and folded with the other empty one-argument
//				functions (CAIBase::OnSave calls Save at 0x10143709,
//				OnLoad calls Load at 0x10143249)
//
// ----------------------------------------------------------------------- //

void CAIAnimator::Save(HMESSAGEWRITE hWrite)
{
}

void CAIAnimator::Load(HMESSAGEREAD hRead)
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	AIDebugPoint()
//
//	PURPOSE:	The AI's debugging hook: compiled out, it does nothing
//				(folded with the other "xor eax,eax; ret" functions, at
//				0x10153e40)
//
// ----------------------------------------------------------------------- //

int AIDebugPoint(int nType, const LTVector& vPos, const AIDebugColor& color, LTFLOAT fSize)
{
	return 0;
}

#endif // PRIMAL_HUNT
