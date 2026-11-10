// lithtech.exe — a block of seven counters and a word after them
// (reconstruction; names ours).
#ifndef __COUNTERBLOCK_H__
#define __COUNTERBLOCK_H__

#include "ltbasedefs.h"

// The one instance is ltcounters.cpp's global (AvP2 1.0.9.6 0x004decb4,
// Primal Hunt 0x004efe40), and nothing in either image reads it: its
// initializer stores the word at +0x1c and tail-jumps into Clear (AvP2
// 0x00404a00 -> 0x00423a00, Primal Hunt 0x004050e0 -> 0x00426170), which
// counter.cpp defines, so the call is never expanded.
class CCounterBlockQ423
{
public:
	CCounterBlockQ423()
	{
		m_Unknown1C = 0;
		Clear();
	}

	void Clear();                            // 0x00423a00 / 0x00426170

	uint32 m_Unknown00;
	uint32 m_Unknown04;
	uint32 m_Unknown08;
	uint32 m_Unknown0C;
	uint32 m_Unknown10;
	uint32 m_Unknown14;
	uint32 m_Unknown18;
	uint32 m_Unknown1C;
};

#endif
