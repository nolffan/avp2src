// *********************************************************************** //
//
//  MODULE:   COUNTER (.CPP) -- Primal Hunt's d3d.ren, TU14
//            (0x10013f96..0x10014012, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The QueryPerformanceCounter timers (LithTech's counter.cpp,
//            the renderer's own copy).  Only what the renderer references
//            survives the link: the divisor's initialiser, the
//            constructor and the start/end pair.
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


unsigned long cnt_NumTicksPerSecond()
{
	LARGE_INTEGER perSec;

	QueryPerformanceFrequency(&perSec);
	return (unsigned long)(perSec.QuadPart / g_CountDiv);
}


// 0x10013fb7
CounterFinal::CounterFinal(unsigned long startMode)
{
	if (startMode == CSTART_MICRO)
		StartMicro();
	else if (startMode == CSTART_MILLI)
		StartMS();
}


void CounterFinal::StartMS()
{
	LARGE_INTEGER *pInt;

	pInt = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(pInt);
}

unsigned long CounterFinal::EndMS()
{
	LARGE_INTEGER curCount, *pInCount, perSec;
	LONGLONG timeElapsed;

	pInCount = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(&curCount);

	QueryPerformanceFrequency(&perSec);
	timeElapsed = curCount.QuadPart - pInCount->QuadPart;
	return (unsigned long)((timeElapsed*(LONGLONG)1000) / perSec.QuadPart);
}

unsigned long CounterFinal::CountMS()
{
	return EndMS();
}


void CounterFinal::StartMicro()
{
	LARGE_INTEGER *pInt;

	pInt = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(pInt);
}

unsigned long CounterFinal::EndMicro()
{
	LARGE_INTEGER curCount, *pInCount;

	pInCount = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(&curCount);

	return (unsigned long)((curCount.QuadPart - pInCount->QuadPart) / g_CountDiv);
}

unsigned long CounterFinal::CountMicro()
{
	return EndMicro();
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
