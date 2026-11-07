// ----------------------------------------------------------------------- //
//
// MODULE  : GameTimer.h
//
// PURPOSE : CGameTimer - a stopwatch on the server's game time (name
//           ours: Third Law's objects keep their times in these; each is
//           one float, -1 until started)
//
// CREATED : Primal Hunt (Third Law)
//
// ----------------------------------------------------------------------- //

#ifndef __GAME_TIMER_H__
#define __GAME_TIMER_H__

#ifdef PRIMAL_HUNT

class CGameTimer
{
	public :

		CGameTimer() { m_fTime = -1.0f; }

		void	Start(LTFLOAT fDuration)	{ LTFLOAT fTime = g_pLTServer->GetTime(); m_fTime = fTime + fDuration; }
		void	Start()						{ m_fTime = g_pLTServer->GetTime(); }
		LTBOOL	IsDone() const				{ return g_pLTServer->GetTime() >= m_fTime; }
		LTFLOAT	GetTimeLeft() const			{ LTFLOAT fTime = g_pLTServer->GetTime(); return m_fTime - fTime; }

		LTFLOAT	m_fTime;
};

#endif // PRIMAL_HUNT

#endif // __GAME_TIMER_H__
