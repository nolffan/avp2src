// ----------------------------------------------------------------------- //
//
// MODULE  : m37_firerate.h
//
// PURPOSE : M-37 (docs/MODERN.md): the rate of fire of a weapon whose
//           fire animation does not loop does not depend on the frame
//           rate.
//
//           CWeaponModel::PlayFireAnimation starts the next shot's fire
//           animation only at the first update after the last one ended
//           (MS_PLAYDONE). Thus each shot took a whole number of frames,
//           and the time between the end of the animation and that update
//           was lost: the pistol fired 233 ms apart at 60 frames a second
//           and about 215 ms apart at 100 (M-13).
//
//           Now, when a fire animation starts soon after the last one ended
//           (the weapon can show another animation for a frame between
//           them), the time that the last one overran (its "debt") is
//           taken back:
//           the new animation plays faster by L / (L - debt) for its one
//           cycle (ILTModel::SetTimeScale), so it ends when it would have
//           ended without the lost time. The animation does not skip any
//           time, thus each model key (the FIRE_KEY that fires the shot,
//           the sound keys) still comes; the pistol's FIRE_KEY is at the
//           start of its animation, where a jump in time would miss it.
//           The time is the performance counter: the client's game time
//           (GetTime) follows the server in a multiplayer game, in steps.
//           The sums are in whole counts (64 bits): the renderer sets the
//           FPU to single precision, and a time of the counter in seconds
//           then has steps of 1/16 s.
//           A debt of 100 ms or more (or half the animation) is not a
//           continuous burst (the player let go of the button), and is not
//           taken back. Any other animation plays at the normal
//           rate again (M37_Update, each frame).
//
//           Included by WeaponModel.cpp (one time), after m13_firelog.h.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M37_FIRERATE_H__
#define __M37_FIRERATE_H__

#include <windows.h>

// The performance counter, and its counts in a second.
inline __int64 M37_Now()
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return now.QuadPart;
}

inline __int64 M37_Freq()
{
	static LARGE_INTEGER s_Freq = { 0 };
	if (!s_Freq.QuadPart) QueryPerformanceFrequency(&s_Freq);
	return s_Freq.QuadPart ? s_Freq.QuadPart : 1;
}

static __int64	s_nM37Start		= -1;		// the counter when the fire animation started
static __int64	s_nM37Length	= 0;		// its counts at the rate it plays
static uint32	s_nM37Anim		= (uint32)-1;	// the animation that plays faster, or -1
static LTBOOL	s_bM37Scaled	= LTFALSE;

// From PlayFireAnimation, just after it set and reset a fire animation.
// (bFollowsShot, the last animation was a fire animation that ended, is
// not used: between two shots the weapon can show another animation.)
inline void M37_OnFireAnimation(HOBJECT hObj, uint32 nAnim, LTBOOL bFollowsShot, LTBOOL bLooping)
{
	LTAnimTracker* pTracker = LTNULL;
	if (!hObj || g_pModelLT->GetMainTracker(hObj, pTracker) != LT_OK || !pTracker) return;

	g_pModelLT->SetTimeScale(pTracker, 1.0f);
	s_bM37Scaled = LTFALSE;
	s_nM37Anim = (uint32)-1;

	if (bLooping)
	{
		s_nM37Start = -1;
		return;
	}

	uint32 nLen = 0;
	g_pModelLT->GetCurAnimLength(pTracker, nLen);
	LTFLOAT fLength = (LTFLOAT)nLen / 1000.0f;
	__int64 nNow = M37_Now();
	LTFLOAT fScale = 1.0f;

	if (s_nM37Start >= 0 && fLength > 0.0f)
	{
		__int64 nDebt = nNow - (s_nM37Start + s_nM37Length);
		LTFLOAT fDebt = (LTFLOAT)nDebt / (LTFLOAT)M37_Freq();
		M13_FIRELOG("firelog-client.log", "debtus", (int)nAnim, (int)(fDebt * 1000000.0f));	// M-13 (debug)
		if (fDebt > 0.0f && fDebt < 0.1f && fDebt < 0.5f * fLength)
			fScale = fLength / (fLength - fDebt);
	}

	s_nM37Start = nNow;
	s_nM37Length = (__int64)((fLength / fScale) * (LTFLOAT)M37_Freq());
	M13_FIRELOG("firelog-client.log", "scale", (int)nAnim, (int)(fScale * 1000.0f));	// M-13 (debug)
	M13_FIRELOG("firelog-client.log", "lenus", (int)nAnim, (int)(fLength * 1000000.0f));	// M-13 (debug)

	if (fScale != 1.0f)
	{
		g_pModelLT->SetTimeScale(pTracker, fScale);
		s_bM37Scaled = LTTRUE;
		s_nM37Anim = nAnim;
	}
}

// Each frame: when the faster animation is no longer the one that plays,
// the model plays at the normal rate again.
inline void M37_Update(HOBJECT hObj)
{
	if (!s_bM37Scaled || !hObj) return;
	if (g_pLTClient->GetModelAnimation(hObj) == s_nM37Anim &&
		!(g_pLTClient->GetModelPlaybackState(hObj) & MS_PLAYDONE)) return;

	LTAnimTracker* pTracker = LTNULL;
	if (g_pModelLT->GetMainTracker(hObj, pTracker) == LT_OK && pTracker)
		g_pModelLT->SetTimeScale(pTracker, 1.0f);
	s_bM37Scaled = LTFALSE;
	s_nM37Anim = (uint32)-1;
}

#endif
