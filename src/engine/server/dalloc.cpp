// server.dll — the engine's allocator (reconstruction). Build /O1.
//
// Every engine allocation goes through here rather than through malloc
// directly, for three reasons visible in the code: the time spent inside the
// CRT is measured, the outstanding count and the lifetime totals are kept,
// and a failed allocation is retried once after asking every registered
// hook to give memory back. Only the second attempt is allowed to fail, and
// when it does the engine says so and stops.
//
// The scoped timer is the interesting part: it is a counter plus the address
// of the total to add into, constructed at the top of a block and destroyed
// at the bottom, so the measurement brackets exactly the call and nothing
// else. The second, post-flush malloc is deliberately outside it.
//
// The file name is ours.
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// windows.h renames these methods out from under the interface classes.
#undef PostMessage
#undef CopyFile
#undef PlaySound
#undef GetClassName

#include "servermgr.h"
#include "worldbsp.h"

// The allocator's books, all in .bss (names ours): how much time went into
// each side, how many blocks are outstanding, and how many were ever handed
// out or handed back. The two times are this file's alone. The counts and the
// hook chain below are also cleared by mem_Init (0x1000d268), so dsi.cpp
// defines those under these names.
uint32        g_AllocTime;                   // 0x10058380
uint32        g_FreeTime;                    // 0x10058384
extern uint32 g_nOutstanding;                // 0x10058378
extern uint32 g_nTotalAllocs;                // 0x10058374
extern uint32 g_nTotalFrees;                 // 0x1005836c

// The chain of "give memory back" hooks, walked from 0x10058370 when an
// allocation fails (name ours).
class CFlushHook
{
public:
	void        (*m_pFn)(void* pUser);       // +0x00
	void*       m_pUser;                     // +0x04
	CFlushHook* m_pNext;                     // +0x08
};

extern CFlushHook* g_pFlushHooks;            // 0x10058370

// 0x1000e443 — the engine's out-of-memory stop (name ours). lithtech.exe's
// dalloc calls the client's own, clientdsi.cpp's dsi_OnOutOfMemory
// (`call 0x0042e560` at 0x00429001), as dsi.cpp's new-handler already does;
// the server's is not in the client image at all.
#ifdef LT_SERVER_BUILD
void dsi_OutOfMemory();
#else
void dsi_OnOutOfMemory();
#endif


// One measured block. The counter is constructed cold and started by hand,
// which is what puts the mode-0 construction and the C start wrapper in that
// order (name ours).
class CTimedScope
{
public:
	CTimedScope(uint32* pTotal)
		: m_Counter(0)
	{
		m_pTotal = pTotal;
		cnt_StartCounter(&m_Counter);
	}

	~CTimedScope();                          // 0x10001ed7

	CBaseCounter m_Counter;                  // +0x00
	uint32*      m_pTotal;                   // +0x08
};


// 0x1000d35c — ask every hook to release what it can (name ours).
static void dalloc_Flush()
{
	CFlushHook* pCur = g_pFlushHooks;

	while (pCur)
	{
		pCur->m_pFn(pCur->m_pUser);
		pCur = pCur->m_pNext;
	}
}

// A zero-byte request is not an error and is not counted: it simply answers
// nothing.
void* dalloc(uint32 nSize)
{
	void* pRet;

	if (!nSize)
		return LTNULL;

	// lithtech.exe 0x00428fd0 has no timing: LT_SERVER_TIMERS is server-only.
#ifdef LT_SERVER_TIMERS
	{
		CTimedScope timer(&g_AllocTime);

		pRet = malloc(nSize);
	}
#else
	pRet = malloc(nSize);
#endif

	if (!pRet)
	{
		dalloc_Flush();

		pRet = malloc(nSize);

		if (!pRet)
#ifdef LT_SERVER_BUILD
			dsi_OutOfMemory();
#else
			dsi_OnOutOfMemory();
#endif
	}

	g_nOutstanding++;
	g_nTotalAllocs++;

	return pRet;
}

// 0x1000d374 — the cleared form. The clear is outside the timing, and a
// failed allocation is handed straight back rather than cleared (name ours).
void* dalloc_z(uint32 nSize)
{
	void* pRet = dalloc(nSize);

	if (pRet)
		memset(pRet, 0, nSize);

	return pRet;
}

// The books are updated before the release, not after: the count is what is
// outstanding once the caller has let go, whether or not the CRT has.
void dfree(void* pMem)
{
	if (pMem)
	{
		g_nOutstanding--;
		g_nTotalFrees++;

		// lithtech.exe 0x00429080: the bare free, as above.
#ifdef LT_SERVER_TIMERS
		{
			CTimedScope timer(&g_FreeTime);

			free(pMem);
		}
#else
		free(pMem);
#endif
	}
}


// The engine's own global operator new and operator delete: one-argument
// cdecl forwards to dalloc and dfree, which is why every `new` in the engine
// is counted and timed. lithtech.exe puts them here, directly after dfree and
// before the terminator leaves' initialisers: 0x004290b0 (`mov eax,[esp+4];
// push eax; call dalloc 0x428fd0; add esp,4; ret`) and 0x004290c0 (the same,
// into dfree 0x429080, `pop ecx`). server.dll's copies are 0x1000d25d and
// 0x10028ace (`push [esp+4]; call 0x1000d2ef / 0x1000d398; pop ecx; ret`),
// which worldbsp.cpp, saveio.cpp and tcpdriver.cpp already name as operator
// new and delete. Both images' calls to them are what reach these.
void* operator new(size_t nSize)
{
	return dalloc(nSize);
}

void operator delete(void* pMem)
{
	dfree(pMem);
}


// 0x10058388 and 0x100583a0 -- the world tree's two terminator leaves,
// file-scope objects of worldbsp.h's CWorldItem18, and what worldbsp.h's
// g_pWorldTreeFrontTerm / g_pWorldTreeBackTerm (0x10051a2c / 0x10051a30)
// point at. Their dynamic initialisers are 0x1000d3de/0x1000d3e3 and
// 0x1000d3f0/0x1000d3f5 (`push 1` / `push 2` into CWorldItem18::Init
// 0x10009250; lithtech.exe 0x004290d0/0x004290e0 and 0x00429120/0x00429130,
// Init expanded), .CRT$XCU slots 0x10051024 and 0x10051028 in that order.
// They sit here, between dfree and memtrack_Init, because that is where the
// shipped code puts their initialisers in both images (names ours).
static CWorldItem18 g_WorldTreeFrontTerm(1);
static CWorldItem18 g_WorldTreeBackTerm(2);

void* g_pWorldTreeFrontTerm = &g_WorldTreeFrontTerm;
void* g_pWorldTreeBackTerm  = &g_WorldTreeBackTerm;


// 0x10001ed7 — the measured block closes: whatever the counter says went by
// is added to the total the scope was given.
CTimedScope::~CTimedScope()
{
	uint32 nElapsed = cnt_EndCounter(&m_Counter);

	*m_pTotal += nElapsed;
}

// The application is told first, so it can put something on the screen that
// belongs to the game rather than to the engine; the engine then says so
// itself and stops. The SDK's own comment says the handler should exit and
// that the engine exits right after it if it does not — this is that.
#ifdef LT_SERVER_BUILD
void dsi_OutOfMemory()
{
	if (g_pServerMgr && g_pServerMgr->m_pAppHandler)
		g_pServerMgr->m_pAppHandler->OutOfMemory();

	MessageBox(GetDesktopWindow(), "Out of memory", "LithTech", 0);

	exit(0x8ae);
}
#endif
