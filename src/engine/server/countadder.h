// *********************************************************************** //
//
//  MODULE:   COUNTADDER (.H)
//
//  PURPOSE:  The engine's timing counter and the scoped adder over it.
//            Moved out of servermgr.h, unchanged, so that the client TUs
//            that time their updates (clientmgr3.cpp, clientupdate.cpp,
//            clientshell.cpp) use the SAME class: Primal Hunt's
//            lithtech.exe keeps one out-of-line ~CountAdder for every
//            TU's unwinds (0x004056b0), where their private views of it
//            compiled copies of their own.
//
// *********************************************************************** //

#ifndef __COUNTADDER_H__
#define __COUNTADDER_H__

#include "ltbasetypes.h"

// The engine-internal counter is eight bytes — one QueryPerformanceCounter
// snapshot — not the SDK LTCounter's four dwords (header drift again). The
// constructor (0x10005f38) starts it for modes 1 and 2 and does nothing for
// 0; cnt_StartCounter/cnt_EndCounter (0x10005fe3/0x10005fec) are the C
// wrappers the server interface exposes. Names ours.
class CBaseCounter
{
public:
	CBaseCounter(int nMode);                 // 0x10005f38

	void   Start();                          // 0x10005f5d
	uint32 Read();                           // 0x10005f65  milliseconds
	uint32 ReadRaw();                        // 0x10005fb0  the fixed scale

	uint32 m_Data[2];
};
void   cnt_StartCounter(CBaseCounter* pCounter);
uint32 cnt_EndCounter(CBaseCounter* pCounter);

// Scoped timing: starts a counter on the way in and adds however long the
// block took to a running total on the way out. The engine leaves these in
// release builds — the totals are what the timing console commands report,
// and LT_SERVER_TIMERS, which guards some of them, is defined for both
// images. Names ours.
//
// The destructor is inline: lithtech.exe's /O2 expands it at the normal
// exits (`push; call cnt_EndCounter; mov 8(..),%ecx; add %eax,(%ecx)`, as
// at 0x00498a84), with an out-of-line copy for the unwinding (0x004056b0);
// server.dll's /O1 keeps it out of line (0x100020e8).
class CountAdder
{
public:
	CountAdder(uint32* pTotal) : m_Counter(0)
	{
		m_pTotal = pTotal;
		cnt_StartCounter(&m_Counter);
	}
	~CountAdder();

	CBaseCounter m_Counter;                  // 0x00
	uint32*      m_pTotal;                   // 0x08
};

inline CountAdder::~CountAdder()
{
	uint32 nElapsed = cnt_EndCounter(&m_Counter);

	*m_pTotal += nElapsed;
}

#endif  // __COUNTADDER_H__
