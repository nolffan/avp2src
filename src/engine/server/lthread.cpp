// server.dll — LThread / LThreadQueue (reconstruction). Build /O1.
#include <string.h>
#include "servermgr.h"
#include "lthread.h"
#include "de_error.h"

#ifndef LT_SERVER_BUILD
// lithtech.exe 0x004b4040 — the thread-message bank itself (lthread.h's
// g_ThreadMessageBank; server.dll's is svnew.cpp's g_ThreadMsgBank, which
// the client build does not compile).  Its initialiser group is this TU's
// first code, 0x004440a0, ahead of g_ThreadCS's at 0x00444110 -- so it is
// defined first.  The inline constructor is 0x004440b0: vptr 0x004a2638,
// then sb_Init(&m_Bank, 0x20, 0x20).  The vtable is objbank.cpp's CObjectBank
// shape over this element -- slot 0 the deleting destructor 0x00444840
// (vptr, sb_FreeChunks(1), the base's 0x004a14c8), slot 1 Allocate
// 0x004447e0 (sb_Allocate expanded, then LThreadMessage's constructor
// 0x004441a0), slot 2 Free 0x004049c0 and slot 3 Init 0x00444820, both
// folded with objbank.cpp's 0x20-byte instantiation.  lthread.obj reaches it
// as g_ThreadMessageBank through link/aliases.rsp, as server.dll does.
class CThreadMsgBankBase
{
public:
	virtual ~CThreadMsgBankBase() {}
	virtual void* Allocate() = 0;
	virtual void  Free(void* pObj) = 0;
	virtual void  Init() = 0;
};

class CThreadMsgBank : public CThreadMsgBankBase
{
public:
	__forceinline CThreadMsgBank() { sb_Init(&m_Bank, sizeof(LThreadMessage), 0x20); }
	virtual ~CThreadMsgBank() { sb_FreeChunks(&m_Bank, 1); }

	virtual void* Allocate()
	{
		LThreadMessage* pRet = (LThreadMessage*)sb_Allocate(&m_Bank);

		if (pRet)
			pRet->LThreadMessage::LThreadMessage();

		return pRet;
	}

	virtual void Free(void* pObj)
	{
		sb_Free(&m_Bank, pObj);
	}

	virtual void Init()
	{
		sb_FreeChunks(&m_Bank, 1);
		sb_Init(&m_Bank, sizeof(LThreadMessage), 0x20);
	}

	StructBank m_Bank;                       // +0x04
	uint32     m_Unknown20;                  // +0x20
};

CThreadMsgBank g_ThreadMsgBank;              // 0x004b4040
#endif

// 0x100586e8 — the lock every LThreadQueue takes around the shared message
// bank (lthread.h declares it extern). Its dynamic-initialiser group is
// server.dll 0x1001555d (_$E head), 0x10015567 (mov ecx / jmp to the
// constructor below), 0x10015571 (atexit registrar) and 0x1001557d (the
// exit thunk into the destructor), directly in front of
// LCriticalSection::LCriticalSection at 0x10015587 -- which is what puts the
// definition above it.
LCriticalSection g_ThreadCS;

LCriticalSection::LCriticalSection()
{
	memset(&m_CS, 0, sizeof(m_CS));
	InitializeCriticalSection(&m_CS);
}

LCriticalSection::~LCriticalSection()
{
	DeleteCriticalSection(&m_CS);
}

void LCriticalSection::Enter()
{
	EnterCriticalSection(&m_CS);
}

void LCriticalSection::Leave()
{
	LeaveCriticalSection(&m_CS);
}

// 0x100355db — three bytes and a return, and nothing in it depends on the
// critical section at all: `push $0x1; pop %eax; ret`. The section is not
// checked because there is nothing to check — InitializeCriticalSection has
// no failure in this era's API — so the guard the two callers below write is
// a shape kept for the platforms where it could fail.
//
// Two things about the address. It is out of line and not expanded, which is
// what the real calls in Init and PostMessage prove (/O1 implies /Ob1, and a
// member defined outside the class body is never inlined). And it is a folded
// COMDAT: 0x100355db is also slot 0 of CWorldSimple's vtable and of several
// other tables, because every four-byte `return LTTRUE` in the image compiles
// to the same three bytes and the linker keeps one copy. So the address does
// not say which translation unit the definition was written in; the class's
// own is the only sensible home for it.
LTBOOL LCriticalSection::IsValid()
{
	return LTTRUE;
}


// Only the first two payload dwords are cleared; the rest of the record is
// whatever the bank last held.
LThreadMessage::LThreadMessage()
{
	m_Data[0] = 0;
	m_Data[1] = 0;
}


// A record out of the bank, constructed in place. The bank hands back raw
// memory, so the construction is explicit.
//
// INLINE, and lithtech.exe is what proves it: PostMessage at 0x4442c0 carries
// this body rather than a call — sb_Allocate's free-list pop against the
// fixed global at 0x4b405c, the sb_Grow refill, and then `call 0x4441a0` for
// the LThreadMessage constructor. /Ob1 never expands a non-template member
// declared out-of-class, so an `inline` keyword somewhere is required.
//
// The two images disagree, and both are measured. server.dll's /O1 would
// expand an `inline` Allocate into PostMessage (232 bytes, not 0x1001568f's
// 219, and 0x10015b2e gone), so there it is a plain out-of-line member.
// lithtech.exe expands the body at 0x4442c0 but ALSO keeps a 58-byte
// out-of-line copy at 0x4447e0 (the bank's vtable slot at 0x004a263c), so
// an `inline` member there loses that copy. What gives both is the body in
// an inline helper, the member a one-line wrapper round it (so each image
// keeps its out-of-line copy), and PostMessage reaching the helper directly
// in the client build only. The helper is our stand-in for whatever the
// client's bank template really spelled.
inline LThreadMessage* tm_AllocateMessage(LThreadMessageBank* pBank)
{
	LThreadMessage* pMsg = (LThreadMessage*)sb_Allocate(&pBank->m_Bank);

	if (pMsg)
		pMsg->LThreadMessage::LThreadMessage();

	return pMsg;
}

LThreadMessage* LThreadMessageBank::Allocate()
{
	return tm_AllocateMessage(this);
}


// Every message in the list back to the bank. The walk runs backwards from
// the tail and stops when the previous link comes back round to it, which is
// how a circular list with no head pointer knows it is done.
//
// INLINE. lithtech.exe has no out-of-line copy at all: Flush at 0x444270 is
// 76 bytes and carries this whole walk, with the bank's free list folded to
// the fixed global 0x4b405c and hoisted out of the loop, then the two stores
// and PulseEvent. A free function defined in a .cpp and declared in a header
// is exactly the shape /Ob1 refuses to expand, so the keyword was in the
// shipped source; /Os declined the expansion at 56 bytes, which is why
// server.dll keeps 0x10015b65 and calls it from Flush.
//
// Flush is the only caller anywhere (netmgr.cpp has its own sibling,
// netbank_FreeMessages), so no other object needs the symbol.
inline void tm_FreeMessages(LMessageList* pList, LThreadMessageBank* pBank)
{
	LThreadMessage* pCur = pList->m_pTail;
	LThreadMessage* pFree;

	while (pCur)
	{
		pFree = pCur;
		pCur = (pCur->m_pPrev == pList->m_pTail) ? LTNULL : pCur->m_pPrev;

		sb_Free(&pBank->m_Bank, pFree);
	}

	pList->m_pTail = LTNULL;
	pList->m_nMessages = 0;
}


LThreadQueue::LThreadQueue()
{
	m_hEvent = CreateEvent(LTNULL, TRUE, FALSE, LTNULL);
}

// 0x100155ee / lithtech 0x4441e0 — 41 and 42 bytes, and ours was 34: seven
// short, which is exactly one `movl $vtbl,0x18(%esi)`. The two list stores are
// there in both images, but so is a vptr store into m_MessageList carrying
// LThreadMsgList's own table (0x1004d8ec in server.dll) — and a vptr store is
// something only a DESTRUCTOR emits. So the two stores are not written here:
// they are ~LThreadMsgList's body, expanded as the member teardown, and this
// function is just the flush and the handle. Only the most derived table is
// stored, which is why the destructor is declared on LThreadMsgList rather
// than on CMessageQueueBase (a base-level one would leave the base's table in
// +0x18 after the derived store was dead-store-eliminated).
//
// The two images schedule the three stores differently — server.dll puts the
// pair before the vptr (0x100155ff, 0x10015603, then 0x10015609), lithtech
// after it (0x4441f6, then 0x4441fd, 0x444200) — which is just /Os against
// /Ot on three independent stores.
LThreadQueue::~LThreadQueue()
{
	Flush();
	CloseHandle(m_hEvent);
}

LTRESULT LThreadQueue::Init()
{
	if (g_ThreadCS.IsValid() && m_CS.IsValid())
	{
		Flush();
		return LT_OK;
	}

	RETURN_ERROR(1, "LThreadQueue::Init", LT_NOTINITIALIZED);
}

// 0x10015671 / lithtech 0x444270. Thirty bytes in server.dll and seventy-six
// in lithtech.exe, and the gap is entirely tm_FreeMessages: /Ot expands it
// here, /Os calls it. Flush's own spelling is not in question — thirty bytes
// is well under the /Os threshold, so whatever distinguishes the two images
// is something Flush calls, and this is the only candidate.
void LThreadQueue::Flush()
{
	tm_FreeMessages(&m_MessageList, &g_ThreadMessageBank);
	PulseEvent(m_hEvent);
}

// 0x1001568f / lithtech 0x4442c0 — 219 bytes against 308, and the whole gap
// is inlining, not shape: the guard order, the two locks, the `if (pNewMsg)`
// around the copy and the trailing RETURN_ERROR are already the shipped ones,
// and the shipped /O2 body differs only by carrying Allocate's body (with
// sb_Allocate folded into it) and Prepend's (with Append's inside that) where
// ours has two calls. All three are inline: Allocate is marked so above, and
// Prepend and Append are in-class in lthread.h, which is the only place the
// keyword can go for them — netmgr.cpp calls Prepend five times and /Ot would
// leave lthread.obj with no COMDAT for those externals to reach.
LTRESULT LThreadQueue::PostMessage(LThreadMessage* pMsg)
{
	LThreadMessage* pNewMsg;

	if (!g_ThreadCS.IsValid())
		RETURN_ERROR(1, "LThreadQueue::PostMessage", LT_NOTINITIALIZED);

	m_CS.Enter();
	g_ThreadCS.Enter();

#ifdef LT_SERVER_BUILD
	pNewMsg = g_ThreadMessageBank.Allocate();
#else
	pNewMsg = tm_AllocateMessage(&g_ThreadMessageBank);
#endif
	if (pNewMsg)
	{
		*pNewMsg = *pMsg;
		m_MessageList.Prepend(pNewMsg);
		SetEvent(m_hEvent);
	}

	g_ThreadCS.Leave();
	m_CS.Leave();

	if (pNewMsg)
		return LT_OK;

	RETURN_ERROR(1, "LThreadQueue::PostMessage", LT_OUTOFMEMORY);
}


// 0x1001576a. Wait for the event, then take a message off. The wait's
// timeout is the flag turned into a dword by hand — `neg`/`sbb` gives 0 for
// false and -1 (INFINITE) for true — and a timed-out wait is the only path
// that does not look at the queue at all.
//
// Both failures answer LT_NOTFOUND (0x3d), and the two are written
// differently in the shipped bytes: the timeout's is a plain `push $0x3d;
// pop %eax`, and the dequeue's is the branchless two-constant form
// (`neg`/`sbb`, `and $0xc3,%al`, `add $0x3d,%eax`) VC6 emits for a `?:`
// between LT_OK and LT_NOTFOUND. The mask is only a byte wide because VC6
// narrows the immediate per operation — the full constant is 0xffffffc3,
// which is 0 - 0x3d.
LTRESULT LThreadQueue::GetMessage(LThreadMessage* pMsg, LTBOOL bWait)
{
	if (WaitForSingleObject(m_hEvent, bWait ? INFINITE : 0) == WAIT_TIMEOUT)
		return LT_NOTFOUND;

	return TakeMessage(pMsg) ? LT_OK : LT_NOTFOUND;
}


// 0x100157ec — the dequeue itself. The event is waited on a second time,
// with a zero timeout, so a wake with nothing behind it costs nothing; then
// the queue's own lock is taken, the tail is unlinked, its payload copied
// into the caller's storage if the caller gave any, and the record handed
// back to the global bank under the global lock.
//
// Two ResetEvents, and they answer different questions: the first is a wake
// with an empty queue, the second the queue going empty on this call. The
// event is manual-reset, so it stays signalled while anything is queued and
// a caller can drain it in a loop.
//
// In server.dll the zero the timeout pushes is the same zero the merged
// answer starts at and the same one the tail is compared against — one
// `xor %esi,%esi` serving three uses. It is also what the message assignment
// spends: `*pMsg = *pCur` is a `rep movsl` of eight dwords through %esi and
// %edi, which is why the success arm has to put the one back into %esi
// afterwards rather than incrementing what was there. All three uses are an
// artefact of the /O1 merge, not of the source — see below.
//
// The empty case falls through and the work is jumped over it, which is the
// shape of a negative guard hosting an early return (`if (!pCur) { ...;
// return LTFALSE; }`) with the dequeue trailing, not of an if/else.
//
// There is no answer variable either. server.dll's single exit at 0x1001586e
// is /O1 cross-jumping three returns into one — which is also what makes the
// zero do triple duty there, the timeout argument and the initial answer and
// the tail comparison all reading one `xor %esi,%esi` — and lithtech
// 0x4444a0 keeps the three apart: `push $0x0` as a literal for the wait, and
// three epilogues each loading its own constant (0x4444b8, 0x4444de,
// 0x444560) after its own `call Leave`.
//
// The bank's free is inline (servermgr.h): the record's first dword becomes
// the next-free link. There is no null test around it here because the tail
// was just proved non-null and nothing has intervened but calls VC6 can see
// through.
LTBOOL LThreadQueue::TakeMessage(LThreadMessage* pMsg)
{
	LThreadMessage* pCur;

	if (WaitForSingleObject(m_hEvent, 0) == WAIT_TIMEOUT)
		return LTFALSE;

	m_CS.Enter();

	pCur = m_MessageList.m_pTail;

	if (!pCur)
	{
		ResetEvent(m_hEvent);
		m_CS.Leave();

		return LTFALSE;
	}

	if (pMsg)
		*pMsg = *pCur;

	m_MessageList.Remove(pCur);

	g_ThreadCS.Enter();
	sb_Free(&g_ThreadMessageBank.m_Bank, pCur);
	g_ThreadCS.Leave();

	if (!m_MessageList.m_pTail)
		ResetEvent(m_hEvent);

	m_CS.Leave();

	return LTTRUE;
}


// The routine handed to CreateThread: run the thread's body, then its
// cleanup, through the virtuals.
static DWORD WINAPI thread_Main(LPVOID pParam)
{
	LThread* pThread = (LThread*)pParam;

	pThread->ThreadRun();
	pThread->ThreadTerm();

	return 0;
}

// The engine's 0-5 priority scale onto the Win32 constants.
static int thread_TranslatePriority(int priority)
{
	if (priority == 0)
		return THREAD_PRIORITY_IDLE;
	if (priority == 1)
		return THREAD_PRIORITY_LOWEST;
	if (priority == 2)
		return THREAD_PRIORITY_BELOW_NORMAL;
	if (priority == 3)
		return THREAD_PRIORITY_NORMAL;

	return (priority == 4) ? THREAD_PRIORITY_ABOVE_NORMAL : THREAD_PRIORITY_HIGHEST;
}

// The wait is unconditional despite the argument: the flag is accepted and
// never read.
LTRESULT LThread::Term(LTBOOL bWaitForThread)
{
	if (m_hThread)
	{
		SetEvent(m_hEvent);
		WaitForSingleObject(m_hThread, INFINITE);

		CloseHandle(m_hThread);
		CloseHandle(m_hEvent);

		m_hThread = LTNULL;
		m_hEvent = LTNULL;
	}

	return LT_OK;
}


// 0x1001587d — the thread's own construction. The two queues construct
// themselves; all this adds is a cleared handle and thread id.
LThread::LThread()
{
	m_hThread  = LTNULL;
	m_ThreadID = 0;
}


LTRESULT LThread::Start(int priority)
{
	if (m_IncomingQueue.Init() == LT_OK && m_OutgoingQueue.Init() == LT_OK)
	{
		m_hEvent = CreateEvent(LTNULL, TRUE, FALSE, LTNULL);
		m_hThread = CreateThread(LTNULL, 0, thread_Main, this, CREATE_SUSPENDED, &m_ThreadID);
		if (!m_hThread)
			RETURN_ERROR(1, "LThread::Start", LT_ERROR);

		SetThreadPriority(m_hThread, thread_TranslatePriority(priority));
		ResumeThread(m_hThread);
		return LT_OK;
	}
	else
	{
		m_IncomingQueue.Flush();
		m_OutgoingQueue.Flush();
	}

	return LT_ERROR;
}


// 0x100158be — nothing of its own to release. The whole body is what VC6
// emits for an empty destructor on a class with two members that have one:
// the vptr store, then the two queues destroyed in REVERSE declaration order
// (the outgoing at +0x2c before the incoming at +0x04), which is the rule
// that dates member teardown after the vptr store rather than before it.
LThread::~LThread()
{
}


// 0x100157a3 / lithtech 0x444440 — peek, not take. The wait is the same
// zero-timeout poll TakeMessage does, and the lock is the same, but nothing
// is unlinked and nothing goes back to the bank: the caller sees the message
// and retires it separately.
//
// There is NO result variable and the unlock is written TWICE. server.dll
// hides that — /O1 cross-jumps the two `m_CS.Leave(); return X;` blocks into
// one, carries the answer in %esi and loads it into %eax at the join — but
// lithtech.exe keeps both: two `call Leave` sites (0x444471 and 0x44448e),
// each followed by its own constant into %eax, and a third `mov $0x3d,%eax`
// for the timed-out wait that shares nothing with either. Three returns with
// three producers is a source with three returns.
//
// The empty case falls through and the copy is jumped forward past its
// return, which is the negative guard hosting an early return where the
// statement stands: `if (!pCur) { ...; return; }` with the copy
// trailing, not an if/else.
LTRESULT LThreadQueue::PeekMessage(LThreadMessage* pMsg)
{
	LThreadMessage* pCur;

	if (WaitForSingleObject(m_hEvent, 0) == WAIT_TIMEOUT)
		return LT_NOTFOUND;

	m_CS.Enter();

	pCur = m_MessageList.m_pTail;

	if (!pCur)
	{
		m_CS.Leave();

		return LT_NOTFOUND;
	}

	*pMsg = *pCur;

	m_CS.Leave();

	return LT_OK;
}


// 0x100158d9 — slot 0x04. The default teardown throws away whatever is still
// queued in either direction; a derived thread that has more to do overrides
// it and calls this first.
void LThread::ThreadTerm()
{
	m_IncomingQueue.Flush();
	m_OutgoingQueue.Flush();
}


// 0x10015a2b — slot 0x08, the thread body. It sleeps on two handles at once:
// the thread's own event, which is how Term wakes it to quit, and the
// incoming queue's, which is how PostMessage wakes it to work. Index zero is
// the quit, so the loop condition is simply that the wait did not return it.
//
// A message is PEEKED, dispatched, and only then retired — so a handler that
// looks at the queue sees its own message still on it, and a crash in one
// leaves the message behind rather than losing it.
void LThread::ThreadRun()
{
	LThreadMessage msg;
	HANDLE         handles[2];

	handles[0] = m_hEvent;
	handles[1] = m_IncomingQueue.m_hEvent;

	while (WaitForMultipleObjects(2, handles, FALSE, INFINITE) != 0)
	{
		if (m_IncomingQueue.PeekMessage(&msg) == LT_OK)
		{
			ProcessMessage(&msg);
			m_IncomingQueue.TakeMessage(LTNULL);
		}
	}
}

// 0x10015a94 — slot 0x0c, the base's default: a bare `ret $0x4`. The vtable
// LThread's constructor stores (0x1004d91c) points slot 3 here, so a thread
// that does not override it ignores every message.
void LThread::ProcessMessage(LThreadMessage* pMsg)
{
}
