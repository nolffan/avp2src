// server.dll — the sound instance's own two functions (reconstruction). /O1.
//
// A translation unit of its own in both images, just ahead of
// spritecontrol.cpp's: SoundInstance::Init and UpdateTimer (lithtech.exe
// 0x004a01f0, 0x004a0410; server.dll 0x1004551b, 0x100456a2) lie between
// <string> initializers of their own -- lithtech.exe's at 0x004a01c0, before
// them as /O2 lays a unit out, server.dll's at 0x1004575f, after them as /O1
// does -- and SpriteControl's code has its own (0x004a08c0, 0x100458e4).
#if defined(PRIMAL_HUNT)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit has
// <string>'s guarded initializer of std::ctype<unsigned short>::id
// (lithtech.exe guard 0x004f8014, atexit of the empty 0x00401790;
// server.dll guard byte 0x1008ec84, atexit of 0x1002eaeb).
#include <string>
#endif
#include "servermgr.h"
#include "de_error.h"

// *********************************************************************** //
//  Two functions of the SOUND manager, not of the sprite control.
//
//  0x100356f3 and 0x1003586c sit between commonphysics.cpp's sm_MoveObject
//  (0x100355fe) and SpriteControl::GetNumFrames above, so by address they
//  belong to a translation unit between the two that has no reconstruction
//  yet. They are kept together here — neither pushes a string literal, so
//  neither is sensitive to which file it is compiled in — and should move to
//  the sound manager's own file when that exists.
//
//  SoundInstance and sm_SetSoundChangeFlag are servermgr.h's; the two fields
//  it leaves inside m_pad_3c are reached through the cast below (name ours).
// *********************************************************************** //

// The fields of SoundInstance that servermgr.h leaves inside m_pad_1c and
// m_pad_3c. Every one of them is a straight copy of the PlaySoundInfo field
// of the same name (ltbasedefs.h), which is what names them: the request's
// m_nPriority, m_fOuterRadius, m_fInnerRadius, m_nVolume, m_fPitchShift,
// m_vPosition and m_UserData land at 0x3c, 0x40, 0x44, 0x48, 0x4c, 0x50 and
// 0x64 (AvP2; NOLF's 0x5c), in that order, and their widths agree byte for byte (two of them are
// single-byte stores). 0x1c takes Init's second argument. Names ours only in
// the sense that they are the SDK's.
class CSoundFields
{
public:
	uint8    m_pad_00[0x14];
	void*    m_hFile;                        // 0x14  Init's `hFile`
	uint8    m_pad_18[0x1c - 0x18];
	float    m_fStartTime;                   // 0x1c  Init's `fTime`
	uint8    m_pad_20[0x3c - 0x20];
	uint8    m_nPriority;                    // 0x3c
	uint8    m_pad_3d[0x40 - 0x3d];
	float    m_fOuterRadius;                 // 0x40
	float    m_fInnerRadius;                 // 0x44
	uint8    m_nVolume;                      // 0x48
	uint8    m_pad_49[0x4c - 0x49];
	float    m_fPitchShift;                  // 0x4c
	LTVector m_vPosition;                    // 0x50  what the clients last heard
	// AVP2: a float Init zeroes and the SDK's user sound type before the
	// user data (server.dll 0x1004553f `fstps 0x5c(%ebx)`, 0x100455b7
	// pInfo+0x134 -> +0x60, 0x100455e6 pInfo+0x138 -> +0x64).
	float    m_fUnknown5c;                   // 0x5c
	uint8    m_nUserSoundType;               // 0x60
	uint8    m_pad_61[0x64 - 0x61];
	uint32   m_UserData;                     // 0x64
};

// The record the sound is filed under in the manager's object-id map: the
// map index it carries is at +0x08 (servermgr.h types the member holding it
// as uint16* for exactly that; name ours).
class CSoundIDRec
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_nIndex;                         // +0x08
};

// The mixer's track record; the one field this side reads is the length in
// seconds (name ours).
class CSoundTrack
{
public:
	uint8 m_pad_00[0x10];
	float m_fLength;                         // +0x10
};

// 0x10031dfa — claim a slot in the manager's object-id map for the record
// handed in, either the id asked for or the next free one when that is
// 0xffff. Nonzero means it could not (name ours).
int  sm_ClaimObjectID(class CServerMgr* pMgr, void** ppRec, uint16 nID);

// 0x100343f2 — smgr.cpp's inline, repeated here word for word (an inline
// function may be defined in every TU that uses it, and identically) so
// that /O2 can expand it into SoundInstance::Init as lithtech.exe does.
// See smgr.cpp for the reading of the body.
inline LTRESULT sm_SetSoundTrackChangeFlags(CServerMgr* pServerMgr,
                                            SoundInstance* pSound, uint32 dwFlag)
{
	CObjectMapEntry* pEntry =
		&pServerMgr->m_Unknown8fc.m_pArray[((uint32*)pSound->m_pUnknown0c)[2]];

	if (pEntry->m_nFlags != 2 || pEntry->m_pRecord == LTNULL)
		RETURN_ERROR(1, "SetSoundTrackChangeFlags", LT_ERROR);

	if (pSound->m_Unknown2c == 0)
	{
		SoundInstance** ppHead = &pServerMgr->m_pChangedSounds;

		pSound->m_pNextChanged = *ppHead;
		*ppHead = pSound;
	}

	pSound->m_Unknown2c |= (uint16)dwFlag;

	return LT_OK;
}


// 0x10029b24 — smgr.cpp's inline SoundInstance::GetObject, repeated here
// word for word for lithtech.exe only: /O2 expands it into UpdateTimer
// (0x483972: `mov 0x38(%ecx),%eax`, `mov 0x4(%eax),%esi`, no call), while
// server.dll calls 0x10029b24 there (0x100358bd), so the server build keeps
// the out-of-line declaration.
#ifndef LT_SERVER_BUILD
inline LTObject* SoundInstance::GetObject()
{
	if (m_PlaySoundFlags & PLAYSOUND_ATTACHED)
	{
		if (!m_pObjectRef)
			return LTNULL;

		return m_pObjectRef->m_pObject;
	}

	if (m_PlaySoundFlags & PLAYSOUND_CLIENTLOCAL)
		return m_pObject;

	return LTNULL;
}
#endif

// 0x1003586c — run one frame off the timer and decide whether the sound is
// finished or has moved.
//
// Three things read off the bytes:
//
//   * the timer is an if/else whose two arms store the same member, NOT one
//     assignment of a conditional value. Both shapes sink the store to one
//     `fstps` after the join, and they differ in what happens next: a
//     ternary leaves its result live in st(0), so the `<= 0.0f` test two
//     statements later compares the register and the shipped `flds
//     0x20(%ebx)` reload disappears (194 bytes against 189, with a stray
//     `fstp %st(0)` on each early exit). Two complete assignment statements
//     leave nothing live, so the next read goes back to memory.
//   * `m_bKill = 1` is written twice in the source and tail-merged by the
//     compiler — the timer-expired arm jumps forward into the copy the
//     object-gone arm hosts, which is where VC6 puts the survivor.
//   * the flag word is read once and tested three times off the same
//     register, then read from memory again after GetObject, because the
//     call could have changed it. That is what writing the member at each
//     use gives; a local would have been spilled across the call.
//   * GETHANDLE and LOOP are two tests, not one `& 0x14`: lithtech.exe
//     keeps `flags & 0x10` in edi (0x483931) and reuses it for the
//     object-gone arm's GETHANDLE test and as the 0 it stores to m_fTimer
//     (0x483988). The change flag is sm_SetSoundTrackChangeFlags (server
//     calls 0x100343f2 at 0x1003591b, lithtech expands it at 0x4839e2).
void SoundInstance::UpdateTimer(float fFrameTime)
{
	if (m_fTimer > fFrameTime)
		m_fTimer = m_fTimer - fFrameTime;
	else
		m_fTimer = 0.0f;

	if (m_bKill)
		return;

	if (!(m_PlaySoundFlags & PLAYSOUND_GETHANDLE) && !(m_PlaySoundFlags & PLAYSOUND_LOOP) &&
	    m_fTimer <= 0.0f && !m_Unknown28)
	{
		m_bKill = 1;
		return;
	}

	if (!(m_PlaySoundFlags & 8))
		return;

	if (!(m_PlaySoundFlags & 3))
		return;

	LTObject* pObj = GetObject();
	if (!pObj)
	{
		if (m_PlaySoundFlags & 0x10)
			return;

		m_fTimer = 0.0f;
		m_bKill = 1;
		return;
	}

	CSoundFields* pPos = (CSoundFields*)this;

	if (pObj->m_Pos.x != pPos->m_vPosition.x ||
	    pObj->m_Pos.y != pPos->m_vPosition.y ||
	    pObj->m_Pos.z != pPos->m_vPosition.z)
	{
		pPos->m_vPosition = pObj->m_Pos;

		sm_SetSoundTrackChangeFlags(g_pServerMgr, this, 2);
	}
}


// 0x100356f3 — build the instance from the request. LTFALSE means the sound
// never started: no request, an attachment with no object, or no room in the
// object-id map.
//
// Read off the bytes:
//
//   * the three failure returns are separate guarded returns, so the shared
//     `xor eax,eax` sits at the LAST of them — the id-map check falls
//     through into it and the other two jump back.
//   * the request's seven descriptive fields are copied one at a time, in
//     ltbasedefs.h's declaration order, two of them as single bytes.
//   * the position source is chosen inside BOTH arms of the attachment test,
//     because the `& (PLAYSOUND_AMBIENT|PLAYSOUND_3D)` guard is compiled
//     twice — once per arm — with the vector copy itself cross-jumped. The
//     guarded arm is the NOT-attached one: it falls through (`test $0x8,%al;
//     jne`), so the request's own position is the first arm in the source.
//   * the closing change-flag call is smgr.cpp's inline
//     sm_SetSoundTrackChangeFlags, not servermgr.h's out-of-line
//     declaration: lithtech.exe EXPANDS it here (0x0048384e..0x004838f3, the
//     "SetSoundTrackChangeFlags" literal and all) while server.dll calls it.
//     That is the /O2-takes, /O1-declines split smgr.cpp documents, and it
//     needs the inline body visible in this TU.
//   * `m_fDuration` is read straight back out of `st(0)` for the timer
//     (`fsts`, no pop), which is what one assignment followed by a read of
//     the same member gives; the LTMIN's second term is parked in the dead
//     `pInfo` parameter slot.
LTBOOL SoundInstance::Init(PlaySoundInfo* pInfo, float fTime, void* hFile, void* pTrack)
{
	CSoundFields* pFields = (CSoundFields*)this;

	if (!pInfo)
		return LTFALSE;

	m_Unknown2c = 0;
	m_Unknown28 = LTNULL;
	pFields->m_hFile = hFile;
	pFields->m_fUnknown5c = 0.0f;

	m_PlaySoundFlags = pInfo->m_dwFlags;

	LTObject* pObj;

	if (m_PlaySoundFlags & (PLAYSOUND_ATTACHED | PLAYSOUND_CLIENTLOCAL))
	{
		pObj = pInfo->m_hObject;
		if (!pObj)
		{
			// Timer first: /O2 stores the float zero as a plain dword and
			// keeps source order, 0x20 then 0x10 (lithtech 0x0048372d), as
			// in smgr.cpp's KillSound.
			m_fTimer = 0.0f;
			m_bKill = LTTRUE;
			return LTFALSE;
		}

		// An attached sound is linked to the object so the object's removal
		// takes the sound with it; a client-local one only remembers it.
		if (m_PlaySoundFlags & PLAYSOUND_ATTACHED)
			sm_CreateInterObjectLink(g_pServerMgr, pObj, (LTObject*)this, 2);
		else
			m_pObject = pObj;
	}
	else
	{
		m_pObject = LTNULL;
		pObj = LTNULL;
	}

	pFields->m_nPriority = pInfo->m_nPriority;
	pFields->m_fOuterRadius = pInfo->m_fOuterRadius;
	pFields->m_fInnerRadius = pInfo->m_fInnerRadius;
	pFields->m_nVolume = pInfo->m_nVolume;
	pFields->m_fPitchShift = pInfo->m_fPitchShift;
	pFields->m_nUserSoundType = pInfo->m_nUserSoundType;

	if (!(m_PlaySoundFlags & PLAYSOUND_ATTACHED))
	{
		if (m_PlaySoundFlags & (PLAYSOUND_AMBIENT | PLAYSOUND_3D))
			pFields->m_vPosition = pInfo->m_vPosition;
	}
	else
	{
		if (m_PlaySoundFlags & (PLAYSOUND_AMBIENT | PLAYSOUND_3D))
			pFields->m_vPosition = pObj->m_Pos;
	}

	pFields->m_UserData = pInfo->m_UserData;
	m_bKill = LTFALSE;

	if (pTrack)
	{
		m_hSample = pTrack;

		m_fDuration = ((CSoundTrack*)pTrack)->m_fLength;
		m_fTimer = m_fDuration + LTMIN(0.5f, m_fDuration * 0.5f);
	}
	else
	{
		m_hSample = LTNULL;
		m_fTimer = 0.0f;
		m_fDuration = 0.0f;
	}

	pFields->m_fStartTime = fTime;

	if (sm_ClaimObjectID(g_pServerMgr, (void**)&m_pUnknown0c, 0xffff))
		return LTFALSE;

	g_pServerMgr->m_Unknown8fc.m_pArray[((CSoundIDRec*)m_pUnknown0c)->m_nIndex].m_nFlags = 2;
	g_pServerMgr->m_Unknown8fc.m_pArray[((CSoundIDRec*)m_pUnknown0c)->m_nIndex].m_pRecord = this;

	sm_SetSoundTrackChangeFlags(g_pServerMgr, this, 3);

	return LTTRUE;
}
