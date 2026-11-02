// server.dll — the server's ILTLightAnim (reconstruction). Build /O1.
#include "serverlightanim.h"
#include "servermsg.h"
#include "packet.h"
#include "de_error.h"

// 0x1002def5 — the interface's first method, and the only one that does not
// index the array itself: it hands the name straight to the world and turns
// the world's LTBOOL into an LTRESULT.
//
// The world object at CServerMgr+0x1a4 is POLYMORPHIC. The shipped code does
// `lea 0x1a4(%eax),%ecx` / `mov (%ecx),%eax` / `call *(%eax)`, so the first
// dword of the object is its vptr and this is virtual slot 0 — servermgr.h
// models CServerWorld as eight opaque bytes and is not this reconstruction's
// to change, so the slot is reached through the local view below.
//
// Read off the bytes: there is no error report at all (no dsi_OnReturnError,
// no g_DebugLevel test), the failure code is 0x3d = LT_NOTFOUND, and the
// out-parameter is set to -1 only on failure, with `orl $0xffffffff,(%esi)`
// — /O1's four-byte spelling of a -1 store. The reference parameter is
// loaded into a callee-saved register before the call and used again after
// it, which is what one reference used twice gives.
class CWorldLightAnims
{
public:
	// slot 0: nonzero if the world has an animation of that name, with its
	// handle written through the second argument (name ours).
	virtual LTBOOL FindLightAnim(const char* pName, HLIGHTANIM* phLightAnim) = 0;
};

LTRESULT ServerLightAnimLT::FindLightAnim(const char* pName, HLIGHTANIM& hLightAnim)
{
	if (((CWorldLightAnims*)&m_pServerMgr->m_World)->FindLightAnim(pName, &hLightAnim))
		return LT_OK;

	hLightAnim = (HLIGHTANIM)-1;

	return LT_NOTFOUND;
}

LTRESULT ServerLightAnimLT::GetNumFrames(HLIGHTANIM hLightAnim, uint32& nFrames)
{
	nFrames = 0;

	if (hLightAnim >= m_pServerMgr->m_nLightAnims)
		RETURN_ERROR_NAMED(2, "ServerLightAnimLT::GetNumFrames", LT_INVALIDPARAMS);

	nFrames = m_pServerMgr->m_pLightAnims[hLightAnim].m_nFrames;
	return LT_OK;
}

LTRESULT ServerLightAnimLT::GetLightAnimInfo(HLIGHTANIM hLightAnim, LAInfo& info)
{
	if (hLightAnim >= m_pServerMgr->m_nLightAnims)
		RETURN_ERROR_NAMED(2, "ServerLightAnimLT::GetLightAnimInfo", LT_INVALIDPARAMS);

	la_GetInfo(&m_pServerMgr->m_pLightAnims[hLightAnim], &info);
	return LT_OK;
}

LTRESULT ServerLightAnimLT::SetLightAnimInfo(HLIGHTANIM hLightAnim, LAInfo& info)
{
	if (hLightAnim >= m_pServerMgr->m_nLightAnims)
		RETURN_ERROR_NAMED(2, "ServerLightAnimLT::SetLightAnimInfo", LT_INVALIDPARAMS);

	// The comparison runs before the write, so the clients are only told about
	// a change that actually happened.
	uint32 dwChangeFlags;
	LightAnim* pAnim = &m_pServerMgr->m_pLightAnims[hLightAnim];
	LTBOOL bChanged = la_CompareInfo(pAnim, &info, &dwChangeFlags);

	la_SetInfo(pAnim, &info);

	if (bChanged)
		sm_LightAnimChanged(m_pServerMgr, hLightAnim, dwChangeFlags);

	return LT_OK;
}


// ---- the record and the caller's view ------------------------------------ //

// 0x100129da — everything a light animation can be asked about. The shadow-map
// flag comes out with the rest even though nothing can put it back: it was
// fixed when the animation was baked.
void la_GetInfo(LightAnim* pAnim, LAInfo* pInfo)
{
	pInfo->m_bShadowMap = pAnim->m_bShadowMap;
	pInfo->m_iFrames[0] = pAnim->m_iFrames[0];
	pInfo->m_iFrames[1] = pAnim->m_iFrames[1];
	pInfo->m_fPercentBetween = pAnim->m_fPercentBetween;
	pInfo->m_fBlendPercent = pAnim->m_fBlendPercent;
	pInfo->m_vLightPos = pAnim->m_vLightPos;
	pInfo->m_vLightColor = pAnim->m_vLightColor;
	pInfo->m_fLightRadius = pAnim->m_fLightRadius;
}

// 0x10012ac3 — what about an animation the caller's LAInfo would change, as a
// bit per field, and whether that is anything at all.
//
// Nothing is compared exactly: every float goes through a quantization first,
// so a change too small for the wire format to carry is not a change. The
// frame pair is the exception — those are integers and compare directly.
//
// Read off the bytes rather than guessed:
//
//   * the quantizer is `(int)(x * 32.0f)` for the two percentages and a bare
//     `(int)x` for the radius. The 32.0f is loaded once into %st(1) and four
//     `fmul %st(1),%st` share it, which is VC6 commoning the constant across
//     four separate `* 32.0f` in the source, not one hoisted local.
//   * the position and colour go through 0x10012a59 with scales of 10.0 and
//     255.0 — the .rdata operands are doubles, and 255.0 is the range the SDK
//     header documents for the colour.
//   * the first flag is `movl $0x1,(%eax)` where the other five are `orl`.
//     That is VC6 folding `|= 1` into a store because the zeroing three
//     instructions earlier is still the last thing to have touched the word;
//     the later ones reload the pointer because __ftol clobbers eax.
//   * the position, colour and radius are only looked at for a shadow-mapped
//     animation (`cmpl $0x0,0x20(%esi)` jumps straight to the return),
//     matching la_SetInfo, which cannot change the flag itself.
LTBOOL la_CompareInfo(LightAnim* pAnim, LAInfo* pInfo, uint32* pChangeFlags)
{
	*pChangeFlags = 0;

	if (pAnim->m_iFrames[0] != pInfo->m_iFrames[0] ||
	    pAnim->m_iFrames[1] != pInfo->m_iFrames[1])
	{
		*pChangeFlags |= 1;
	}

	if ((int)(pAnim->m_fPercentBetween * 32.0f) !=
	    (int)(pInfo->m_fPercentBetween * 32.0f))
	{
		*pChangeFlags |= 2;
	}

	if ((int)(pAnim->m_fBlendPercent * 32.0f) !=
	    (int)(pInfo->m_fBlendPercent * 32.0f))
	{
		*pChangeFlags |= 4;
	}

	if (pAnim->m_bShadowMap)
	{
		if (la_QuantizedVecDiffers(&pAnim->m_vLightPos, &pInfo->m_vLightPos, 10.0f))
			*pChangeFlags |= 8;

		if (la_QuantizedVecDiffers(&pAnim->m_vLightColor, &pInfo->m_vLightColor, 255.0f))
			*pChangeFlags |= 0x10;

		if ((int)pAnim->m_fLightRadius != (int)pInfo->m_fLightRadius)
			*pChangeFlags |= 0x20;
	}

	return *pChangeFlags != 0;
}

// 0x10012a1c — and back the other way, without the shadow-map flag.
void la_SetInfo(LightAnim* pAnim, LAInfo* pInfo)
{
	pAnim->m_iFrames[0] = pInfo->m_iFrames[0];
	pAnim->m_iFrames[1] = pInfo->m_iFrames[1];
	pAnim->m_fPercentBetween = pInfo->m_fPercentBetween;
	pAnim->m_fBlendPercent = pInfo->m_fBlendPercent;
	pAnim->m_vLightPos = pInfo->m_vLightPos;
	pAnim->m_vLightColor = pInfo->m_vLightColor;
	pAnim->m_fLightRadius = pInfo->m_fLightRadius;
}

// 0x10012a59 — the position/colour half of the comparison above: quantize
// both vectors by the same scale and answer whether any component lands on a
// different integer.
//
// The scale is a double, not a float: `fmull 0x10(%ebp)` reads eight bytes
// and the caller pushes eight, so `pA->x * fScale` widens the component
// rather than narrowing the scale.
//
// One shared `push $1; pop %eax` serves all three failures, which is the
// OR-merged condition and not three separate returns.
int la_QuantizedVecDiffers(LTVector* pA, LTVector* pB, double fScale)
{
	if ((int)(pA->x * fScale) != (int)(pB->x * fScale) ||
	    (int)(pA->y * fScale) != (int)(pB->y * fScale) ||
	    (int)(pA->z * fScale) != (int)(pB->z * fScale))
	{
		return 1;
	}

	return 0;
}


// 0x1002896d — is there already a queued change for this animation on this
// client? The queue is short and unsorted, so it is a linear walk; the index
// is written only on a hit and the answer is a plain int (`xor %eax,%eax` for
// the miss, `push $1; pop %eax` for the hit).
//
// Read off the bytes: the count is loaded once into %ecx and the guard is
// `test %ecx,%ecx; jbe`, which is the rotated `for` — the walk itself is the
// indexed source form strength-reduced to `add $0xc` before the loop and
// `add $0x4` inside it. The compare is sixteen bits on both sides with the
// record's field as the destination operand, and the field is written first
// in the source comparison. The PARAMETER is 32 bits, cast at the compare:
// lithtech's /O2 copy (0x00466610) hoists it as `mov 0xc(%esp),%esi` (a
// uint16 parameter gives `mov 0xc(%esp),%si`, one byte longer), and the
// caller passes its HLIGHTANIM uncast (server.dll identical either way).
int la_FindClientChange(Client* pClient, uint32 hLightAnim, uint32* pnIndex)
{
	CLAClient* pLAClient = (CLAClient*)pClient;
	uint32     i;

	for (i = 0; i < pLAClient->m_nChanges; i++)
	{
		if (pLAClient->m_Changes[i].m_hLightAnim == (uint16)hLightAnim)
		{
			*pnIndex = i;

			return 1;
		}
	}

	return 0;
}


// 0x100289a2 — every client that is listening gets told, by having the change
// queued on it. An animation index the world does not have is dropped before
// the walk even starts.
//
// Read off the bytes: the client's queue is searched first, so a second change
// to the same animation ORs into the entry already there rather than adding a
// second one; and the overflow test is `>= 0x40` *before* the append, so a
// full queue is flushed by 0x10026755 and then appended to anyway — the flush
// is what makes room.
void sm_LightAnimChanged(CServerMgr* pServerMgr, HLIGHTANIM hLightAnim,
                         uint32 dwChangeFlags)
{
	if (hLightAnim >= pServerMgr->m_nLightAnims)
		return;

	LTLink* pCur = pServerMgr->m_ClientList.m_Head.m_pNext;

	while (pCur != &pServerMgr->m_ClientList.m_Head)
	{
		CLAClient* pClient = (CLAClient*)pCur->m_pData;
		uint32     nIndex;

		if (la_FindClientChange((Client*)pClient, hLightAnim, &nIndex))
		{
			pClient->m_Changes[nIndex].m_ChangeFlags |= dwChangeFlags;
		}
		else
		{
			if (pClient->m_nChanges >= MAX_CLIENTLACHANGES)
				sm_Unknown26755(pServerMgr, (Client*)pClient);

			pClient->m_Changes[pClient->m_nChanges].m_hLightAnim = hLightAnim;
			pClient->m_Changes[pClient->m_nChanges].m_ChangeFlags = dwChangeFlags;

			pClient->m_nChanges++;
		}

		pCur = pCur->m_pNext;
	}
}


// 0x10028858 (sm_RemoveObjectFromSky) is NOT here: smgr.cpp already carries
// it, as the documented lea/displacement near. Four more spellings were tried
// against it from this file — the value temp, the compound assignment, the
// two-statement store, and cl 8168 — and all four reproduce smgr.cpp's exact
// two-block difference, so the near class stands.


// 0x10026584 — write one animation's changed fields into a message: the id as
// a word, the change bits as a byte, and then whichever of the frame pair,
// the two percentages, the position, the colour and the radius those bits ask
// for. Name ours.
//
// Read off the bytes:
//
//   * a non-shadow-mapped animation has bits 8/0x10/0x20 masked off before
//     the bit byte goes out (`and $0xffffffc7,%ebx`), which is the same rule
//     la_CompareInfo applies from the other end — so the receiver is never
//     told about a position, colour or radius it could not have changed.
//   * the frame pair is written as two WORDS out of uint32 fields: the loads
//     are `mov 0x34(%esi),%ax`, sixteen bits, so a frame index above 65535
//     would truncate on the wire.
//   * both percentages are scaled by 255.0f, not by the 32.0f la_CompareInfo
//     quantizes them with. The comparison is therefore eight times coarser
//     than the wire format it is protecting, so a change of between 1/255 and
//     1/32 of full scale is dropped rather than sent.
//   * the colour goes out as three raw bytes with no scale at all, which is
//     what says the stored vector is already 0..255.
//   * the position goes through the compressed-position codec against the
//     server's world (&pMgr->m_World, the CPosRef the codec measures a
//     position as a fraction of), and it is handed the message's embedded
//     LMessageImpl at +0x08, not the message itself.
//   * the radius is the only field written as a full float, through
//     CPacket::WriteType<float>.
void la_WriteChange(CServerMgr* pMgr, LightAnim* pAnim, uint32 hLightAnim,
                    ServerMessage* pMsg, uint32 nChangeFlags)
{
	if (!pAnim->m_bShadowMap)
		nChangeFlags &= ~0x38;

	pMsg->WriteWord(hLightAnim);
	pMsg->WriteByte(nChangeFlags);

	if (nChangeFlags & 1)
	{
		pMsg->WriteWord(pAnim->m_iFrames[0]);
		pMsg->WriteWord(pAnim->m_iFrames[1]);
	}

	if (nChangeFlags & 2)
		pMsg->WriteByte((uint8)(pAnim->m_fPercentBetween * 255.0f));

	if (nChangeFlags & 4)
		pMsg->WriteByte((uint8)(pAnim->m_fBlendPercent * 255.0f));

	if (nChangeFlags & 8)
	{
		lmsg_WriteCompPos(&pMsg->m_Write, &pAnim->m_vLightPos,
		                  (CPosRef*)&pMgr->m_World);
	}

	// The colour bytes go through CPacket::WriteType itself, not WriteByte:
	// lithtech.exe's /O2 build expands them (0x00462c4a..0x00462e68, each
	// with Insert2 left as a call to 0x00412f50) as it does the radius,
	// while the earlier writes stay calls (0x0042fa50, 0x004159f0) -- the
	// /O2 budget spent from the end backwards. Written as WriteType
	// throughout, all nine expand (1341 bytes against 861, measured).
	// server.dll calls 0x1001393e either way.
	if (nChangeFlags & 0x10)
	{
		((CPacket*)pMsg)->WriteType((uint8)pAnim->m_vLightColor.x);
		((CPacket*)pMsg)->WriteType((uint8)pAnim->m_vLightColor.y);
		((CPacket*)pMsg)->WriteType((uint8)pAnim->m_vLightColor.z);
	}

	if (nChangeFlags & 0x20)
		((CPacket*)pMsg)->WriteType(pAnim->m_fLightRadius);
}

// 0x1000e845 (GetSpaceLeft) and 0x10026755 (sm_Unknown26755) are in
// smgr.cpp, which is where both images put the latter: lithtech.exe's
// 0x00462fd0 sits between two of smgr.cpp's functions (0x00462ee0,
// 0x004630f0), as server.dll's 0x10026755 sits inside that file's run.
