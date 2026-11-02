// server.dll — the animation tracker (reconstruction). Build /O1.
//
// A tracker is a cursor into a model's animation list: where it was, where
// it is, and how far between the two the vertices should be set up. The
// "where" is a pair of frame locators — animation index, keyframe index and
// a time in milliseconds — and everything here maintains that pair.
//
// Two flags matter. Bit 3 says the tracker is allowed to sit on an animation
// index the model does not have, which is how a tracker survives being set
// up before its model is loaded; every bounds check here is skipped when it
// is set. Bit 0 marks the tracker as carrying an animation at all, and bit 1
// is set by the initialization regardless.
//
// The interpolation counter doubles as its own "not interpolating" value:
// 0xffff means the previous locator is not a real place to blend from, which
// is what a fresh tracker and a hard animation change both leave behind.
//
// The file name is ours.
#include "servermgr.h"
#include "commonmodel.h"
#include "ltanimtracker.h"

// 0x100012b6 — run the tracker forward by a time step (name ours).
void at_Advance(LTAnimTracker* pTracker, uint32 nTime, int nFlags);

// 0x100013f0 — what one keyframe does as the tracker crosses it. Defined at
// the foot of this file (name ours).
void at_DoKeyFrame(LTAnimTracker* pTracker, ModelAnim* pAnim, uint32 iKey);

// 0x10001490 — the first string keyframe at or after a given index on the
// tracker's current animation, with its index written through the third
// argument. Declared, not reconstructed (name ours).
AnimKeyFrame* at_FindNextStringKey(LTAnimTracker* pTracker, uint32 iStart,
                                   uint32* piFound);

// Defined below, and the same 0x100011c2 commonmodel.h declares as
// at_ResetAnim — the two names for one function are an artifact of the two
// files having been written from opposite ends.
void at_Reset(LTAnimTracker* pTracker);


// A tracker is valid when it has a model and both of its locators name an
// animation that model actually has.
//
// `inline`, and the images prove it. Every caller in lithtech.exe has this
// body expanded into it -- at_IsDone, at_SetTime, at_SetAnim, at_Advance and
// at_UpdatePercent all open with the three tests spelled out and no call, and
// lithtech.exe has no out-of-line copy of this function at all (the gap
// between at_IsDone's end and at_SetTime's start is five `nop`s). server.dll
// has the out-of-line copy at 0x100010ec and calls it from all five. Under
// /Ob1 the only thing that can be expanded is a function marked inline, so
// the declaration is inline in both builds and /Os simply declines it at 36
// bytes while /Ot accepts -- which is the whole modelanim /O2 gap.
inline LTBOOL LTAnimTracker::IsValid()
{
	Model* pModel = m_TimeRef.m_pModel;

	if (!pModel ||
	    m_TimeRef.m_Cur.m_iAnim >= pModel->m_AnimArray.m_nElements ||
	    m_TimeRef.m_Prev.m_iAnim >= pModel->m_AnimArray.m_nElements)
	{
		return LTFALSE;
	}

	return LTTRUE;
}

// The animation's length is the time on its last keyframe.
//
// This is the function that proved the keyframe count is `volatile`: the
// shipped code compares it against memory and then loads it a second time
// for the subscript, and nothing but a volatile field does that (see the
// note on the declaration in commonmodel.h).
//
// The two images really differ here. server.dll (0x1001690b) is
// `cmpl $0,0x10(%ecx) ; jbe ; mov 0x10(%ecx),%eax` -- two loads -- while
// lithtech.exe (0x445c30) loads the count ONCE (`mov 0x10(%ecx),%eax ; test`),
// which a volatile read can never give. Measured: reading the count through
// a non-volatile `*(uint32*)&m_nKeyFrames` makes lithtech identical and
// server.dll a 23-byte near (one load); a stand-in CMoArray with inline
// GetSize()/Last() does the same. So the volatile read stays for /O1 and
// lithtech reads it plain.
uint32 ModelAnim::GetLength()
{
#ifdef LT_SERVER_BUILD
	if (m_nKeyFrames > 0)
		return m_pKeyFrames[m_nKeyFrames - 1].m_Time;
#else
	uint32 nKeyFrames = *(uint32*)&m_nKeyFrames;

	if (nKeyFrames > 0)
		return m_pKeyFrames[nKeyFrames - 1].m_Time;
#endif

	return 0;
}

// 0x10001013 — set a tracker up against a model. Without a model there is
// nothing to bound the animation index against, so the index is dropped on
// the floor and only the clear happens (name ours).
//
// NEAR (122 of 122 bytes, 39 of 39 instructions), and what is left is
// register allocation only (the best register renaming leaves nothing structural;
// lithtech 0x00401050 shows the same esi/edx/bx vs edi/esi/cx rotation).
//  - the time-ref clear is written field by field with m_pModel stored third,
//    NOT as one inlined AnimTimeRef::Init: that is what puts shipped's
//    `mov %esi,0x1c(%eax)` between m_pUser1 and m_CurKey and the fldz after
//    the flag clears (both images). It cut the byte diff 32 -> 30 (server)
//    and 37 -> 25 (lithtech).
//  - the model block's store order is the scheduler's: three source orders
//    (flags before/after the sentinel, lithtech's order) emit the same code.
// cl 8168, 8966 and the 8804 default all emit what we emit. Also measured
// with no change on either image: the model block in server.dll's store
// order (sentinel, flags, both indices, both frames) with `m_Flags |= 1`
// in place of the local; the flags OR last; the bounds test as a nested
// if, as an empty-then if/else and as a ternary; the index in a local of
// its own (lithtech 6 bytes short); `m_Prev = m_Cur` after the current
// locator's stores (4 bytes long on both).
void at_Init(LTAnimTracker* pTracker, Model* pModel, uint32 iAnim)
{
	uint16 nFlags;

	pTracker->m_StringKeyCallback = LTNULL;
	pTracker->m_pUser1 = LTNULL;
	pTracker->m_TimeRef.m_pModel = pModel;
	pTracker->m_CurKey = 0;
	pTracker->m_Flags = 0;
	pTracker->m_InterpolationMS = 0;

	pTracker->m_TimeRef.m_Prev.Clear();
	pTracker->m_TimeRef.m_Cur.Clear();
	pTracker->m_TimeRef.m_Percent = 0.0f;

	if (pModel)
	{
		nFlags = pTracker->m_Flags;

		if (!(nFlags & 8) && iAnim >= pModel->m_AnimArray.m_nElements)
			iAnim = 0;

		pTracker->m_TimeRef.m_Cur.m_iAnim = (uint16)iAnim;
		pTracker->m_Flags = (uint16)(nFlags | 1);
		pTracker->m_TimeRef.m_Prev.m_iAnim = (uint16)iAnim;
		pTracker->m_TimeRef.m_Cur.m_iFrame = 0;
		pTracker->m_TimeRef.m_Prev.m_iFrame = 0;
		pTracker->m_InterpolationMS = 0xffff;
	}

	pTracker->m_Flags |= 2;
}

// 0x1000108d — the per-frame step, in whole milliseconds. A tracker that is
// not playing is not advanced at all; a playing one goes through the same
// stepper the explicit seeks use, with the flag argument set (name ours).
//
// The third argument is a literal 1 at this call site and a literal 0 at
// at_SetTime's, which is the only evidence for what it means: this is the
// step that is allowed to fire string keys and wrap a looping animation,
// the seek is not.
void at_UpdateTracker(LTAnimTracker* pTracker, uint32 nMS)
{
	if (pTracker->m_Flags & AT_PLAYING)
		at_Advance(pTracker, nMS, 1);
}

// 0x100010a7 — has the current animation run out? A tracker that is still
// playing has not, whatever its time says; the end is one past the last
// keyframe's time (name ours).
LTBOOL at_IsDone(LTAnimTracker* pTracker)
{
	ModelAnim* pAnim;

	if (pTracker->IsValid())
	{
		pAnim = pTracker->m_TimeRef.m_pModel->
		            m_AnimArray.m_pArray[pTracker->m_TimeRef.m_Cur.m_iAnim].m_pAnim;

		if (!(pTracker->m_Flags & 2) &&
		    pTracker->m_TimeRef.m_Cur.m_Time ==
		        pAnim->m_pKeyFrames[pAnim->m_nKeyFrames - 1].m_Time + 1)
		{
			return LTTRUE;
		}
	}

	return LTFALSE;
}

// 0x10001110 — put the tracker at a given time in its current animation. A
// tracker that is not on a real animation, or that is being sent back to the
// start, is wound all the way back first; then the current locator is reset
// and the step runs it forward (name ours).
void at_SetTime(LTAnimTracker* pTracker, uint32 nTime, LTBOOL bInterpolate)
{
	if (!pTracker->IsValid() || !bInterpolate)
	{
		pTracker->m_TimeRef.m_Cur.m_Time = 0;
		pTracker->m_TimeRef.m_Prev.m_Time = 0;
		pTracker->m_TimeRef.m_Cur.m_iFrame = 0;
		pTracker->m_TimeRef.m_Prev.m_iFrame = 0;
		pTracker->m_CurKey = 0;
	}

	pTracker->m_TimeRef.m_Cur.m_Time = 0;
	pTracker->m_TimeRef.m_Cur.m_iFrame = 0;
	pTracker->m_CurKey = 0;

	at_Advance(pTracker, nTime, 0);
}

// 0x10001153 — change which animation the tracker is on. Interpolating keeps
// the old locator to blend from; not interpolating throws it away and says
// so with the counter's sentinel (name ours).
//
// Two store orders come from lithtech's /O2 copy (0x004011c0), server.dll's
// /O1 copy being identical either way: the percentage is zeroed before the
// animation index is stored, and the non-interpolating arm sets the sentinel
// before the locator copy (shipped writes 0x1a between the two dword stores).
LTBOOL at_SetAnim(LTAnimTracker* pTracker, uint32 iAnim, LTBOOL bInterpolate)
{
	if (!(pTracker->m_Flags & 8) && iAnim >= pTracker->m_TimeRef.m_pModel->m_AnimArray.m_nElements)
		return LTFALSE;

	pTracker->m_TimeRef.m_Prev = pTracker->m_TimeRef.m_Cur;

	pTracker->m_TimeRef.m_Percent = 0.0f;
	pTracker->m_TimeRef.m_Cur.m_iAnim = (uint16)iAnim;
	pTracker->m_CurKey = 0;
	pTracker->m_TimeRef.m_Cur.m_iFrame = 0;
	pTracker->m_TimeRef.m_Cur.m_Time = 0;

	if (bInterpolate && pTracker->IsValid())
	{
		pTracker->m_InterpolationMS = 0;
	}
	else
	{
		pTracker->m_InterpolationMS = 0xffff;
		pTracker->m_TimeRef.m_Prev = pTracker->m_TimeRef.m_Cur;
	}

	return LTTRUE;
}

// The tracker's two frame locators, as this file needs them. commonmodel.h
// records the layout in a comment (m_TimeRef 0x1c, m_pModel 0x1c, m_Prev
// 0x20, m_Cur 0x28) without declaring it, and the SDK's ltanimtracker.h names
// the fields m_iAnim / m_iFrame on a FrameLocator. Modelled locally rather
// than re-laying out a header five other TUs include (names the SDK's).
class CFrameLocatorView
{
public:
	uint16 m_iAnim;                          // 0x00
	uint16 m_iFrame;                         // 0x02
	uint32 m_Time;                           // 0x04
};

class CTrackerKeyView
{
public:
	class AnimKeyFrame* GetCurKeyFrame();    // 0x10001541
	class AnimKeyFrame* GetPrevKeyFrame();   // 0x10001562

	uint8             m_pad_00[0x1c];
	class Model*      m_pModel;              // 0x1c
	CFrameLocatorView m_Prev;                // 0x20
	CFrameLocatorView m_Cur;                 // 0x28
};

// 0x10001541 and 0x10001562 — the keyframe each of the tracker's two time
// references is sitting on. One walk apiece: the animation index selects a
// 32-byte AnimInfo out of the model's table (the `shl $5` is that stride),
// its first field is the ModelAnim, and the key index then selects a 20-byte
// AnimKeyFrame out of that animation's array.
//
// The pair differ only in which reference they read -- m_Cur at +0x28 for the
// first, m_Prev at +0x20 for the second -- and they are two functions rather
// than one taking a reference because each is 33 bytes of its own in the
// image, 33 apart. Names ours; nothing in the image names either.
//
// Both are `inline`, on the same evidence as LTAnimTracker::IsValid: neither
// exists out of line in lithtech.exe, and the one caller -- at_UpdatePercent,
// itself expanded into at_Advance -- carries both walks spelled out, sharing
// one load of the model's animation table between them. server.dll has them
// at 0x10001541 and 0x10001562 and calls both. /Os declines at 33 bytes, /Ot
// accepts. They have to sit above at_UpdatePercent for that: VC6's inliner is
// forward-only, and a body it has not read yet it cannot expand.
inline AnimKeyFrame* CTrackerKeyView::GetCurKeyFrame()
{
	ModelAnim* pAnim = m_pModel->m_AnimArray.m_pArray[m_Cur.m_iAnim].m_pAnim;

	return &pAnim->m_pKeyFrames[m_Cur.m_iFrame];
}

inline AnimKeyFrame* CTrackerKeyView::GetPrevKeyFrame()
{
	ModelAnim* pAnim = m_pModel->m_AnimArray.m_pArray[m_Prev.m_iAnim].m_pAnim;

	return &pAnim->m_pKeyFrames[m_Prev.m_iFrame];
}


// 0x100014e5 — the blend fraction between the two keyframes the tracker sits
// between: how far the current time is past the previous frame, over the gap
// between the two frames. A tracker sitting on one frame, or one whose
// animation indices are out of range, has no fraction and gets zero.
//
// Read off the bytes rather than guessed:
//
//   * the zero arm is emitted after the body and jumped forward into, and
//     the body jumps over it — an if/else with the positive test first, not
//     two guarded returns (which host the shared block at the last test).
//   * the two frame pointers come back from GetPrevFrame and GetCurFrame in
//     that order, and the previous frame's time is loaded once into a
//     register and subtracted from both numerator and denominator.
//   * both operands go through the unsigned-to-float sequence — the value
//     stored to the low dword of an eight-byte slot and zero to the high one
//     — so both differences are `uint32`. The divisor's `fild` is then
//     peepholed into `fidivl`, which reads only the low dword, and VC6
//     leaves the now-dead store of the high dword standing. That dead store
//     is in the shipped bytes too, so it is the compiler's and not ours.
//
// `inline`, and above at_Advance so that VC6 can see it there. lithtech.exe
// has no out-of-line copy: its only caller, at_Advance, ends with this whole
// body spelled out -- the locator compare, IsValid's three tests, both frame
// walks and the divide -- sharing at_Advance's own registers and its zero.
// server.dll has it at 0x100014e5 and calls it, so /Os declines it and /Ot
// takes it, the same split as IsValid.
inline void at_UpdatePercent(LTAnimTracker* pTracker)
{
	AnimKeyFrame* pPrev;
	AnimKeyFrame* pCur;

	if (pTracker->m_TimeRef.m_Prev.m_iFrame != pTracker->m_TimeRef.m_Cur.m_iFrame &&
	    pTracker->IsValid())
	{
		pPrev = ((CTrackerKeyView*)pTracker)->GetPrevKeyFrame();
		pCur  = ((CTrackerKeyView*)pTracker)->GetCurKeyFrame();

		pTracker->m_TimeRef.m_Percent =
		    (float)(pTracker->m_TimeRef.m_Cur.m_Time - pPrev->m_Time) /
		    (float)(pCur->m_Time - pPrev->m_Time);
	}
	else
	{
		pTracker->m_TimeRef.m_Percent = 0.0f;
	}
}


// 0x100012b6 — the step. Four things happen here, in this order: the blend
// counter is run up and either produces a percentage (and nothing else) or
// expires and hands its overshoot on as the step; the current time is moved
// and either clamped one past the end or left to be wrapped; every keyframe
// the step crossed is dispatched; and a looping animation that ran off the
// last keyframe is folded back with a modulo.
//
// An invalid tracker still accumulates time — nothing else can be done with
// it, and the time is what makes it valid again once its model arrives.
//
// Read off the bytes rather than guessed:
//  - the flag tested is bit 1, the same AT_LOOPING that GetLooping reports,
//    and it decides clamp-versus-wrap. That is the evidence for the name.
//  - the leftover after the blend expires is computed from the *member*
//    again (`movzwl 0x1a(%esi)`) after at_Reset has run, not from the sum
//    already in a register.
//  - the keyframe walk's bound is the last index, so its two tests are
//    `> nKeyFrames - 1` and `<= nKeyFrames - 1`, not `>=`/`<` against the
//    count; VC6 keeps the last index in a register and gets the count back
//    with an `inc` for the wrap test.
//  - the clamp compares against `nLength + 1` twice and VC6 commons it into
//    one `lea 0x1(%ecx),%edx`, so the `+ 1` is in the source expression.
//
// Three statement orderings had to be read out of the bytes, each worth the
// whole match:
//  - in the one-keyframe case the key is zeroed first, then the percentage,
//    then the time. With the percentage stored last VC6
//    cross-jumps this block's `fldz`/`fstps` into the blend percentage's
//    and the function comes out ten bytes short; with the percentage first
//    /O1 is identical but lithtech's /O2 copy (0x00401370) stores it before
//    m_CurKey where shipped has `mov %ebp,0x14(%esi)` first.
//  - the blend percentage is an if/else with the division first and the zero
//    second; reversing the arms puts the division out of line.
//  - the loop wrap takes the modulo first and zeroes the key after, and VC6
//    then hoists that store into the divide's shadow — which is exactly
//    where the shipped code has it. Zeroing the key first in the source
//    puts the store before the `xor %edx,%edx`, three bytes out.
void at_Advance(LTAnimTracker* pTracker, uint32 nTime, int nFlags)
{
	ModelAnim* pAnim;
	uint32     nKeyFrames;
	uint32     iLast;
	uint32     nLength;
	uint32     nNewTime;

	if (!pTracker->IsValid())
	{
		pTracker->m_TimeRef.m_Cur.m_Time += nTime;
		return;
	}

	pAnim = pTracker->m_TimeRef.m_pModel->
	            m_AnimArray.m_pArray[pTracker->m_TimeRef.m_Cur.m_iAnim].m_pAnim;

	if (pTracker->m_InterpolationMS != 0xffff)
	{
		pTracker->m_InterpolationMS = (uint16)(pTracker->m_InterpolationMS + nTime);

		if (pTracker->m_InterpolationMS > pAnim->m_nInterpolationTime)
		{
			at_Reset(pTracker);
			nTime = pTracker->m_InterpolationMS - pAnim->m_nInterpolationTime;
			pTracker->m_InterpolationMS = 0xffff;
		}
		else
		{
			if (pAnim->m_nInterpolationTime != 0)
				pTracker->m_TimeRef.m_Percent =
				    (float)pTracker->m_InterpolationMS / (float)pAnim->m_nInterpolationTime;
			else
				pTracker->m_TimeRef.m_Percent = 0.0f;

			return;
		}
	}

	nKeyFrames = pAnim->m_nKeyFrames;

	if (nKeyFrames <= 1)
	{
		pTracker->m_CurKey = 0;
		pTracker->m_TimeRef.m_Percent = 0.0f;
		pTracker->m_TimeRef.m_Cur.m_Time = 0;
		return;
	}

	iLast = nKeyFrames - 1;
	nLength = pAnim->m_pKeyFrames[iLast].m_Time;

	pTracker->m_TimeRef.m_Prev.m_Time = pTracker->m_TimeRef.m_Cur.m_Time;
	nNewTime = pTracker->m_TimeRef.m_Cur.m_Time + nTime;
	pTracker->m_TimeRef.m_Cur.m_Time = nNewTime;

	if (!(pTracker->m_Flags & AT_LOOPING))
	{
		if (nNewTime >= nLength + 1)
			nNewTime = nLength + 1;

		pTracker->m_TimeRef.m_Cur.m_Time = nNewTime;
	}

	while (pTracker->m_CurKey <= iLast)
	{
		if (pTracker->m_TimeRef.m_Cur.m_Time <=
		        pAnim->m_pKeyFrames[pTracker->m_CurKey].m_Time)
		{
			break;
		}

		if (nFlags)
			at_DoKeyFrame(pTracker, pAnim, pTracker->m_CurKey);

		pTracker->m_CurKey++;
	}

	if (pTracker->m_CurKey == iLast + 1 && (pTracker->m_Flags & AT_LOOPING))
	{
#ifdef AVP2_MODERN
		// fixed: an animation whose keyframes are all at time 0 wraps to 0
		// instead of dividing by zero (RETAIL_BUGS.md)
		pTracker->m_TimeRef.m_Cur.m_Time =
		    nLength ? pTracker->m_TimeRef.m_Cur.m_Time % nLength : 0;
#else
		pTracker->m_TimeRef.m_Cur.m_Time =
		    pTracker->m_TimeRef.m_Cur.m_Time % nLength;
#endif
		pTracker->m_CurKey = 0;
	}

	if (!nFlags)
	{
		pTracker->m_TimeRef.m_Cur.m_iFrame = (uint16)pTracker->m_CurKey;
		pTracker->m_TimeRef.m_Prev.m_iFrame = (uint16)pTracker->m_CurKey;
	}

	at_UpdatePercent(pTracker);
}

// 0x100011c2 — back to the start of whatever animation is current, with
// nothing to blend from (name ours).
//
// The Percent store is LAST: lithtech.exe's /O2 copy (0x00401250) keeps it
// there, and server.dll's /O1 copy is identical either way.
void at_Reset(LTAnimTracker* pTracker)
{
	pTracker->m_TimeRef.m_Prev.m_iAnim = pTracker->m_TimeRef.m_Cur.m_iAnim;
	pTracker->m_CurKey = 0;
	pTracker->m_TimeRef.m_Cur.m_iFrame = 0;
	pTracker->m_TimeRef.m_Prev.m_iFrame = 0;
	pTracker->m_TimeRef.m_Cur.m_Time = 0;
	pTracker->m_TimeRef.m_Prev.m_Time = 0;
	pTracker->m_TimeRef.m_Percent = 0.0f;
}


// 0x100013f0 — one keyframe fired. A type-1 keyframe calls the function it
// carries, and only when bit 2 of the tracker's flags is set; a type-0
// keyframe is a string key, handed to the tracker's own callback when the
// string is not empty, and then the tracker's two frame locators are moved
// onto whichever string key comes next. Any other type does nothing.
//
// Read off the bytes rather than guessed:
//
//   * the type byte is loaded once into %cl and tested twice — against 1
//     and then against 0 — so the source is an if / else-if on the field,
//     not a switch.
//   * the next string key's *index* comes back through the third argument
//     and is thrown away; the index the locators are set to is recomputed
//     as a pointer difference, which is the `sub 0xc(%ebx),%eax` and
//     `idiv $0x14` pair. The thrown-away index lives in the dead pTracker
//     parameter slot, which is why there is an `ebp` frame and no
//     `sub %esp`.
//   * the comparison is `m_Cur.m_Time <= pNext->m_Time` (the operand order
//     survives, and the `ja` takes the other arm), and the two arms differ
//     in which locator gets the old index: the `<=` arm leaves the previous
//     locator on this keyframe, the `>` arm moves both.
void at_DoKeyFrame(LTAnimTracker* pTracker, ModelAnim* pAnim, uint32 iKey)
{
	AnimKeyFrame* pKey = &pAnim->m_pKeyFrames[iKey];
	AnimKeyFrame* pNext;
	uint32        iNext;

	if (pKey->m_Type == 1)
	{
		if ((pTracker->m_Flags & 4) && pKey->m_pFn)
			pKey->m_pFn(pTracker, pKey);
	}
	else if (pKey->m_Type == 0)
	{
		if (pKey->m_pString[0] && pTracker->m_StringKeyCallback)
			pTracker->m_StringKeyCallback(pTracker, pKey);

		pNext = at_FindNextStringKey(pTracker, iKey + 1, &iNext);

		if (pNext)
		{
			if (pTracker->m_TimeRef.m_Cur.m_Time <= pNext->m_Time)
			{
				pTracker->m_TimeRef.m_Prev.m_iFrame = (uint16)iKey;
				pTracker->m_TimeRef.m_Cur.m_iFrame =
				    (uint16)(pNext - pAnim->m_pKeyFrames);
			}
			else
			{
				uint16 iFrame = (uint16)(pNext - pAnim->m_pKeyFrames);

				pTracker->m_TimeRef.m_Cur.m_iFrame  = iFrame;
				pTracker->m_TimeRef.m_Prev.m_iFrame = iFrame;
			}
		}
		else
		{
			pTracker->m_TimeRef.m_Cur.m_iFrame  = (uint16)iKey;
			pTracker->m_TimeRef.m_Prev.m_iFrame = (uint16)iKey;
		}
	}
}

// 0x10001490 — the next string keyframe at or after an index, on whichever
// animation the tracker's current locator names. Read off the bytes:
//
//   * the animation is reached the same way GetCurKeyFrame reaches it — the
//     current locator's index selects a 32-byte AnimInfo (`shl $5`) out of
//     the model's table and its first field is the ModelAnim — and an
//     animation slot that is empty is the same answer as finding nothing.
//   * the keyframe count goes into a local before the loop. It has to: the
//     count is volatile (see commonmodel.h on ModelAnim), and the shipped
//     loop compares against a register (`cmp %esi,%ecx`) rather than
//     against memory, which only a copy produces.
//   * and the counter is initialised in a statement of its own, ABOVE that
//     copy, rather than in the `for` initialiser. That is the whole of the
//     last two bytes: written as `for (i = iStart; i < nKeys; i++)` the two
//     loads come out count-then-argument, and written as `i = iStart;`
//     followed by `nKeys = ...;` they come out argument-then-count, which is
//     the shipped order. Two adjacent loads, and their order is the order of
//     the two source statements that produce them.
//   * the walk itself is written indexed — the shipped `add $0x14,%edx` is
//     the strength reduction, and the found path re-reads m_pKeyFrames and
//     recomputes the address, which is what an indexed source gives.
//   * type 0 is the string key (the same encoding at_DoKeyFrame reads), and
//     both the "no animation" and the "ran off the end" answers share one
//     `xor %eax,%eax` exit, so they are one trailing `return LTNULL`.
//
// Name ours; nothing in the image names it.
AnimKeyFrame* at_FindNextStringKey(LTAnimTracker* pTracker, uint32 iStart,
                                   uint32* piFound)
{
	ModelAnim* pAnim;
	uint32     i, nKeys;

	pAnim = pTracker->m_TimeRef.m_pModel->m_AnimArray
	            .m_pArray[pTracker->m_TimeRef.m_Cur.m_iAnim].m_pAnim;

	if (pAnim)
	{
		i     = iStart;
		nKeys = pAnim->m_nKeyFrames;

		for (; i < nKeys; i++)
		{
			if (pAnim->m_pKeyFrames[i].m_Type == 0)
			{
				*piFound = i;

				return &pAnim->m_pKeyFrames[i];
			}
		}
	}

	return LTNULL;
}
