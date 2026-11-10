// *********************************************************************** //
//
//  MODULE:   COUNTER (.CPP) -- Primal Hunt's d3d.ren, TU14
//            (0x10013f96..0x10014012, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The QueryPerformanceCounter timers (LithTech's counter.cpp,
//            the renderer's own copy).  Only what the renderer references
//            survives the link: the divisor's initialiser, the
//            constructor and the start/end pair.  The rest (EndMS,
//            CountMS, CountMicro, cnt_NumTicksPerSecond) the link's
//            /OPT:REF dropped, and the members the survivors inline have
//            no copy of their own: they are inline here, so our
//            /OPT:NOREF link has none either.
//
// *********************************************************************** //

#include <windows.h>


#define CSTART_NONE     0
#define CSTART_MILLI    1
#define CSTART_MICRO    2

class CounterFinal
{
public:
	                CounterFinal(unsigned long startMode=CSTART_NONE);

	void            StartMS();
	unsigned long   EndMS();
	unsigned long   CountMS();

	void            StartMicro();
	unsigned long   EndMicro();
	unsigned long   CountMicro();

	unsigned long   m_Data[2];
};


static unsigned __int64 g_CountDiv = 1;                     // 0x1004ac58


// Win2K has a VERY high counter frequency but it's more useful if it's in
// microseconds.
class CountDivSetter
{
public:
	CountDivSetter()
	{
		LARGE_INTEGER perSec;

		QueryPerformanceFrequency(&perSec);
		g_CountDiv = 1;
	}
};
static CountDivSetter __g_CountDivSetter;                   // 0x10013f96


inline void CounterFinal::StartMS()
{
	LARGE_INTEGER *pInt;

	pInt = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(pInt);
}

inline void CounterFinal::StartMicro()
{
	LARGE_INTEGER *pInt;

	pInt = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(pInt);
}

inline unsigned long CounterFinal::EndMicro()
{
	LARGE_INTEGER curCount, *pInCount;

	pInCount = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(&curCount);

	return (unsigned long)((curCount.QuadPart - pInCount->QuadPart) / g_CountDiv);
}


// 0x10013fb7
CounterFinal::CounterFinal(unsigned long startMode)
{
	if (startMode == CSTART_MICRO)
		StartMicro();
	else if (startMode == CSTART_MILLI)
		StartMS();
}


// 0x10013fd5
void cnt_StartCounterFinal(CounterFinal &cCounter)
{
	cCounter.StartMicro();
}

// 0x10013fe0
unsigned long cnt_EndCounterFinal(CounterFinal &cCounter)
{
	return cCounter.EndMicro();
}
