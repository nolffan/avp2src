// ----------------------------------------------------------------------- //
//
// MODULE  : p1_frametime.h
//
// PURPOSE : The modern build's frame timing (docs/MODERN.md, M-10), the
//           animation clock: a frame's time in the whole milliseconds the
//           engine's animation and sprite trackers advance by.
//
//           Retail takes (int)(frame time * 1000) every frame and drops
//           what is left over: up to a millisecond lost a frame, nothing
//           at 60 frames a second's 16.7 ms worth noticing, but at 240
//           (4.2 ms a frame) animations ran about a twentieth slow, and
//           at 400 (2.5 ms) a fifth.  Here what is left over is kept and
//           added to the next frame, so the milliseconds handed out add up
//           to the time that passed.
//
//           (The engine's sources include this as
//           ..\..\modern\engine\p1_frametime.h: the name and the P1_
//           prefix are nolfsrc's, whose engine blocks these are; the
//           implementation is ours.)
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __P1_FRAMETIME_H__
#define __P1_FRAMETIME_H__

// A frame's whole milliseconds; *pCarry holds the fraction from one call
// to the next (one carry per caller: each is its own clock).
inline unsigned long P1_FrameMs(float fFrameTime, float* pCarry)
{
	float fTotal = fFrameTime * 1000.0f + *pCarry;
	unsigned long nWhole;

	if (fTotal <= 0.0f)
	{
		*pCarry = 0.0f;
		return 0;
	}

	nWhole = (unsigned long)fTotal;
	*pCarry = fTotal - (float)nWhole;
	return nWhole;
}

// The same for code called once per object in a server tick: every object
// of one tick must advance by the same amount, so the first call of a tick
// (told by the tick's game time) works the milliseconds out and the rest
// of that tick get them again.
struct P1TickMs
{
	float			m_fTickTime;	// the game time of the tick worked out
	float			m_fCarry;
	unsigned long	m_nMs;
};

inline unsigned long P1_TickMs(P1TickMs* pTick, float fFrameTime, float fTickTime)
{
	if (pTick->m_fTickTime != fTickTime)
	{
		pTick->m_fTickTime = fTickTime;
		pTick->m_nMs = P1_FrameMs(fFrameTime, &pTick->m_fCarry);
	}

	return pTick->m_nMs;
}

#endif // __P1_FRAMETIME_H__
