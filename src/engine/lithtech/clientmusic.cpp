// *********************************************************************** //
//
//  MODULE:   CLIENTMUSIC (.CPP)
//
//  PURPOSE:  The engine's DirectMusic implementation.
//
// *********************************************************************** //

#include <string.h>

#include "clientmusic.h"


// Playing is asking for the level's starting intensity, immediately.
// The level-not-initialised refusal's callee, under a name of its own. It is
// the same 0x0043ef90 as dm_NotInitError (link/aliases.rsp), but it has to be
// a SECOND SYMBOL to the compiler: with both refusals calling one function,
// VC6 /O2 tail-merges ChangeIntensity's second refusal into the first (`push
// ; jmp`, 777 bytes) where the shipped 0x00440620 keeps both whole (798).
// No one-symbol spelling stops that merge (an inline wrapper, a cast or a
// local for the name, else-if, a literal return value: all 777). Two
// identical refusal functions that the linker folded into one would explain
// it; that is a guess, the second symbol is what the bytes need. Every
// function here uses it for the level test, so the eight sites stay alike.
void dm_NotLevelInitError(char* pFnName);

LTRESULT CLTDirectMusicMgr::Play()
{
	dm_Printf(4, "CLTDirectMusicMgr::Play\n");

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::Play");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_NotLevelInitError("CLTDirectMusicMgr::Play");
		return LT_ERROR;
	}

	return ChangeIntensity(m_nInitialIntensity, LTDMEnactImmediatly);
}

// The master volume is a performance-wide parameter, not a segment one, so
// this needs nothing but the performance — and the address of its own
// argument, which is why VC6 writes the register-held parameter back to its
// stack home first.
LTRESULT CLTDirectMusicMgr::SetVolume(const long nVolume)
{
	// AVP2: the level's VOLUMEOFFSET is added (0x0044c75f).
	long nSetVolume = m_nVolumeOffset + nVolume;

	dm_Printf(4, "CLTDirectMusicMgr::SetVolume nVolume=%i\n", nVolume);

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::SetVolume");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_NotLevelInitError("CLTDirectMusicMgr::SetVolume");
		return LT_ERROR;
	}

	return m_pPerformance->SetGlobalParam(GUID_PerfMasterVolume, &nSetVolume,
	                                      sizeof(long)) < 0;
}

// Stopping the secondary segments: all of them, or the one with this name.
//
// The two halves were clearly edited independently. The all-of-them loop asks
// each node whether it is playing and decides the enact type in an if/else
// with a call in each arm — which VC6 merges back into one, both values in
// the same register. The by-name loop keeps the enact type in a variable and
// then tests the wrong object: `pNode->m_pSegment->m_pDMSegment`, the loaded
// segment, where the other loop tests `pNode->m_pSegState`, the playing one.
// It works, but only because a node is never added to the list without a
// segment state; a node that could fail that test would be found again on the
// next pass and never removed.
//
// Two more quirks the shipped code carries (0x00440b4e..0x00440b8b), and
// both are what made these nears before:
// * nEnact is ONE function-scope variable, set from nStart once. A node whose
//   segment has its own stop type overwrites it, and every later node inherits
//   that value.
// * The all-of-them loop moves to the next node only inside the "is playing"
//   test. The unguarded tail is `test %eax,%eax`, reached straight from the
//   segment-state `je`. So a node with no segment state would spin forever;
//   that is the same list invariant the by-name loop relies on.
#ifdef AVP2SRC_MODERN
// fixed: StopSecondary and StopMotif test the wrong object when given a name
// (RETAIL_BUGS.md). One walk for both cases: each node is visited once, the
// next link read before the node can go back to the bank, and only a node
// with a segment state is stopped. Nothing is found twice, so a node that is
// not playing, or one left to its next-segment stop, cannot hang the loop.
static void dm_StopPlayingNodes(CLTDirectMusicMgr* pMgr, CDMPlayingList& List,
                                const char* sName, const LTDMEnactTypes nStart)
{
	CDMPlayingNode* pNode = List.m_pHead;

	while (pNode)
	{
		CDMPlayingNode* pNext   = pNode->m_pNext;
		CDMSegment*     pSegment = pNode->m_pSegment;
		LTDMEnactTypes  nEnact  = nStart;

		if (pNode->m_pSegState &&
		    (!sName || (pSegment && pSegment->GetName() &&
		                stricmp(pSegment->GetName(), sName) == 0)))
		{
			if (nStart == LTDMEnactDefault && pSegment)
				nEnact = pSegment->m_nStopEnact;

			if (nEnact != LTDMEnactNextSegment)
				List.StopNode(pMgr, pNode, nEnact, 0);
		}

		pNode = pNext;
	}
}
#endif
LTRESULT CLTDirectMusicMgr::StopSecondary(const char* sSecondarySegment,
                                          const LTDMEnactTypes nStart)
{
	CDMPlayingNode* pNode;
	LTDMEnactTypes  nEnact = nStart;

	dm_Printf(4, "CLTDirectMusicMgr::StopSecondary sSecondarySegment=%s nStart=%i\n",
	          sSecondarySegment, nStart);

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::StopSecondary");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_NotLevelInitError("CLTDirectMusicMgr::StopSecondary");
		return LT_ERROR;
	}

	EnterCriticalSection(&m_CriticalSection);

#ifdef AVP2SRC_MODERN
	dm_StopPlayingNodes(this, m_SecondaryList, sSecondarySegment, nStart);
#else
	if (!sSecondarySegment)
	{
		pNode = m_SecondaryList.m_pHead;

		while (pNode)
		{
			CDMPlayingNode* pNext = pNode->m_pNext;

			if (pNode->m_pSegState)
			{
				if (nStart == LTDMEnactDefault)
					nEnact = pNode->m_pSegment->m_nStopEnact;

				if (nEnact != LTDMEnactNextSegment)
					m_SecondaryList.StopNode(this, pNode, nEnact, 0);

				pNode = pNext;
			}
		}
	}
	else
	{
		while ((pNode = m_SecondaryList.FindByName(sSecondarySegment)) != LTNULL)
		{
			void*          pDMSegment;

			if (nStart == LTDMEnactDefault)
				nEnact = pNode->m_pSegment->m_nStopEnact;

			pDMSegment = pNode->m_pSegment->m_pDMSegment;

			if (pDMSegment && nEnact != LTDMEnactNextSegment)
				m_SecondaryList.StopNode(this, pNode, nEnact, 0);
		}
	}
#endif

	LeaveCriticalSection(&m_CriticalSection);
	return LT_OK;
}

// The motifs, from their own list eight bytes along, and otherwise the same
// function down to the wrong-object test.
LTRESULT CLTDirectMusicMgr::StopMotif(const char* sMotifName, const LTDMEnactTypes nStart)
{
	CDMPlayingNode* pNode;
	LTDMEnactTypes  nEnact = nStart;

	dm_Printf(4, "CLTDirectMusicMgr::StopMotif sMotifName=%s nStart=%i\n",
	          sMotifName, nStart);

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::StopMotif");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_NotLevelInitError("CLTDirectMusicMgr::StopMotif");
		return LT_ERROR;
	}

	EnterCriticalSection(&m_CriticalSection);

#ifdef AVP2SRC_MODERN
	dm_StopPlayingNodes(this, m_MotifList, sMotifName, nStart);
#else
	if (!sMotifName)
	{
		pNode = m_MotifList.m_pHead;

		while (pNode)
		{
			CDMPlayingNode* pNext = pNode->m_pNext;

			if (pNode->m_pSegState)
			{
				if (nStart == LTDMEnactDefault)
					nEnact = pNode->m_pSegment->m_nStopEnact;

				if (nEnact != LTDMEnactNextSegment)
					m_MotifList.StopNode(this, pNode, nEnact, 0);

				pNode = pNext;
			}
		}
	}
	else
	{
		while ((pNode = m_MotifList.FindByName(sMotifName)) != LTNULL)
		{
			void*          pDMSegment;

			if (nStart == LTDMEnactDefault)
				nEnact = pNode->m_pSegment->m_nStopEnact;

			pDMSegment = pNode->m_pSegment->m_pDMSegment;

			if (pDMSegment && nEnact != LTDMEnactNextSegment)
				m_MotifList.StopNode(this, pNode, nEnact, 0);
		}
	}
#endif

	LeaveCriticalSection(&m_CriticalSection);
	return LT_OK;
}

// Playing a secondary segment: find the definition the control file gave that
// name, and either hand it to DirectMusic now or queue it for the next
// segment boundary. The segment flags always carry SECONDARY and CONTROL on
// top of whatever the enact type asks for.
//
// Both of this function's dead argument slots get reused — the segment name's
// for the critical section's address and the enact type's for the
// performance's vtable — which is why it reserves only one word of its own.
//
// AvP2's and Primal Hunt's shape (0x0044cb30): the enact type is resolved in
// the parameter itself (shipped keeps it in ebx from entry), an immediate
// start records its segment state through CDMPlayingList::Add (0x0044e460),
// and a next-segment start goes on m_PendingSecondaryList's tail through
// `new` rather than behind m_pQueueAfter. NOLF's queued behind
// m_pQueueAfter and filled the playing node inline.
LTRESULT CLTDirectMusicMgr::PlaySecondary(const char* sSecondarySegment,
                                          LTDMEnactTypes nStart)
{
	IDirectMusicSegmentState* pSegState = LTNULL;
	CDMSegment*    pSegment;

	dm_Printf(4, "CLTDirectMusicMgr::PlaySecondary sSecondarySegment=%s nStart=%i\n",
	          sSecondarySegment, nStart);

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::PlaySecondary");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_NotLevelInitError("CLTDirectMusicMgr::PlaySecondary");
		return LT_ERROR;
	}

	pSegment = m_SecondaryDefs.FindByName(sSecondarySegment);

	if (!pSegment)
		return LT_ERROR;

	if (!pSegment->m_pDMSegment)
		return LT_ERROR;

	if (nStart == LTDMEnactDefault)
		nStart = pSegment->m_nStopEnact;

	EnterCriticalSection(&m_CriticalSection);

	if (nStart != LTDMEnactNextSegment)
	{
		void* pDMSegment = pSegment->m_pDMSegment;
		m_pPerformance->PlaySegment(pDMSegment,
		                            EnactToSegFlags(nStart) | 0x280, 0, &pSegState);
		m_SecondaryList.Add(pSegState, pSegment);
	}
	else
	{
		CDMPendingNode* pPending = new CDMPendingNode(DMPENDING_PLAYSECONDARY);

		if (pPending)
		{
			pPending->m_pSegment = pSegment;
			// at the head: retail's call reaches CBaseList::InsertFirst
			// (Primal Hunt's lithtech.exe 0x0044cc7d and 0x0044cf1a,
			// AvP2's 0x00448e6d and 0x0044910a)
			m_PendingSecondaryList.AddHead(pPending);
		}
	}

	LeaveCriticalSection(&m_CriticalSection);
	return LT_OK;
}

// The motifs, from their own definition list and their own playing list, and
// with only SECONDARY in the flags where the secondary segments also take
// CONTROL. The queued record's type code is the one number that says which of
// the two this was.
//
// The same AvP2 shape as PlaySecondary (0x0044cdd0), with m_MotifList.

LTRESULT CLTDirectMusicMgr::PlayMotif(const char* sMotifName,
                                          LTDMEnactTypes nStart)
{
	IDirectMusicSegmentState* pSegState = LTNULL;
	CDMSegment*    pSegment;

	dm_Printf(4, "CLTDirectMusicMgr::PlayMotif sMotifName=%s nStart=%i\n",
	          sMotifName, nStart);

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::PlayMotif");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_NotLevelInitError("CLTDirectMusicMgr::PlayMotif");
		return LT_ERROR;
	}

	pSegment = m_MotifDefs.FindByName(sMotifName);

	if (!pSegment)
		return LT_ERROR;

	if (!pSegment->m_pDMSegment)
		return LT_ERROR;

	if (nStart == LTDMEnactDefault)
		nStart = pSegment->m_nStopEnact;

	EnterCriticalSection(&m_CriticalSection);

	if (nStart != LTDMEnactNextSegment)
	{
		void* pDMSegment = pSegment->m_pDMSegment;
		m_pPerformance->PlaySegment(pDMSegment,
		                            EnactToSegFlags(nStart) | 0x80, 0, &pSegState);
		m_MotifList.Add(pSegState, pSegment);
	}
	else
	{
		CDMPendingNode* pPending = new CDMPendingNode(DMPENDING_PLAYMOTIF);

		if (pPending)
		{
			pPending->m_pSegment = pSegment;
			// at the head: retail's call reaches CBaseList::InsertFirst
			// (Primal Hunt's lithtech.exe 0x0044cc7d and 0x0044cf1a,
			// AvP2's 0x00448e6d and 0x0044910a)
			m_PendingSecondaryList.AddHead(pPending);
		}
	}

	LeaveCriticalSection(&m_CriticalSection);
	return LT_OK;
}

// Stopping everything: the primary segment through DirectMusic, then the
// queue of pending requests, then both lists of playing segments.
//
// Only the primary stop honours the enact type. It is skipped for
// LTDMEnactNextSegment - the primary is left to run out on its own - and the
// pending queue is emptied regardless, so what was going to follow it never
// does. The secondaries and motifs are stopped immediately, whatever was
// asked; that is the helper's own choice, not this function's.
//
// The performance's vtable is read before the enact conversion is called and
// the performance itself reloaded for the call, which is how VC6 orders a
// virtual call whose last argument is itself a call. The pending loop tests
// the node it has just unlinked before pushing it back on the bank, because
// the bank's inline free has a null test of its own and VC6 does not carry
// the loop's condition across the call to prove it.
LTRESULT CLTDirectMusicMgr::Stop(const LTDMEnactTypes nStart)
{
	dm_Printf(4, "CLTDirectMusicMgr::Stop nStart=%i\n", nStart);

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::Stop");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_NotLevelInitError("CLTDirectMusicMgr::Stop");
		return LT_ERROR;
	}

	if (m_pPerformance && nStart != LTDMEnactNextSegment)
	{
		m_pPerformance->Stop(LTNULL, LTNULL, 0, EnactToSegFlags(nStart));
	}

	EnterCriticalSection(&m_CriticalSection);

	{
		CDMPendingNode* pPending = m_PendingList.m_pHead;

		while (pPending)
		{
			m_PendingList.Remove(pPending);
			g_PendingBank.Free(pPending);

			pPending = m_PendingList.m_pHead;
		}
	}

	// AVP2: the second pending list (0x0044c6a1) and the primary playing
	// list (0x0044c6cb) are emptied the same way.
	{
		CDMPendingNode* pPending = m_PendingSecondaryList.m_pHead;

		while (pPending)
		{
			m_PendingSecondaryList.Remove(pPending);
			g_PendingBank.Free(pPending);

			pPending = m_PendingSecondaryList.m_pHead;
		}
	}

	m_PrimaryList.StopAll(this, 0);
	m_SecondaryList.StopAll(this, 0);
	m_MotifList.StopAll(this, 0);

	m_pQueueAfter = LTNULL;

	LeaveCriticalSection(&m_CriticalSection);
	return LT_OK;
}

// Tearing a level down: silence everything inside the critical section, then
// give every record the level loaded back to its bank and free the tables.
//
// The order is the shipped one and it is not the tidy one. The queue of
// pending requests goes first, then the interface's own Stop, then the
// secondary and motif definitions and the two playing lists — all under the
// lock — and only after the lock is dropped do the bands, the two lists whose
// objects this function only ever releases, and the four allocations go. A
// level that was never initialised is not an error here, where every other
// entry point makes it one: the warning is a warning and the answer is LT_OK.
//
// Each list is emptied the same way: release what the record holds, unlink
// it, and push it onto its bank's free list — the bank's own inline fast
// path, whose null test survives right after a Remove that cannot have
// changed the pointer.
//
// The four frees all reach 0x004290c0, which is the engine's operator delete
// (the one-argument wrapper round dfree at 0x00429080), so they are deletes
// and not dfree calls. The array at 0xb8 is a `delete []` of a class with a
// destructor, and with exceptions off VC6 expands the vector destructor in
// place rather than calling it: the element count read from the word before
// the array, a pointer parked past the end and walked down with `sub $0x10`
// before each destructor call, `dec; js` on the count, and operator delete
// on the cookie word. Neither `for (i = n - 1; i >= 0; i--)` nor
// `while (--i >= 0)` over `pArray[i]` reproduces it — both start the pointer
// at the last element and decrement after the call. The compiler also emits
// the stand-alone iterator (`??_I`) as an unreferenced COMDAT, the way it
// emits `??_H` for `new T[n]`. The null tests round each free are the
// source's, not the compiler's: the store of the null sits inside them.
LTRESULT CLTDirectMusicMgr::TermLevel()
{
	dm_Printf(3, "CLTDirectMusicMgr::TermLevel\n");

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::TermLevel");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_Warning("WARNING! Level already terminated or never initialized. (CLTDirectMusicMgr::TermLevel)\n");
		return LT_OK;
	}

	EnterCriticalSection(&m_CriticalSection);

	ClearPendingList();
	Stop(LTDMEnactDefault);
	Unknown4426c0();

	m_SecondaryDefs.StopAll(this, 0);
	m_MotifDefs.StopAll(this, 0);
	m_PrimaryList.StopAll(this, 0);         // AVP2: 0x0044c424
	m_SecondaryList.StopAll(this, 0);
	m_MotifList.StopAll(this, 0);

	LeaveCriticalSection(&m_CriticalSection);

	{
		CDMBand* pBand = m_BandList.m_pHead;

		while (pBand)
		{
			pBand->m_pDMBandSegment->Release();
			pBand->m_pDMBand->Unload(m_pPerformance);
			m_BandList.Remove(pBand);
			g_BandBank.Free(pBand);

			pBand = m_BandList.m_pHead;
		}
	}

	{
		CDMUnknown58* pNode = m_Unknown58List.m_pHead;

		while (pNode)
		{
			pNode->m_pDMObject->Release();
			m_Unknown58List.Remove(pNode);

			if (pNode)
			{
				if (pNode->m_pName)
					delete [] pNode->m_pName;

				g_Unknown58Bank.Free(pNode);
			}

			pNode = m_Unknown58List.m_pHead;
		}
	}

	{
		CDMUnknown60* pNode = m_Unknown60List.m_pHead;

		while (pNode)
		{
			pNode->m_pDMObject->Release();
			m_Unknown60List.Remove(pNode);
			g_Unknown60Bank.Free(pNode);

			pNode = m_Unknown60List.m_pHead;
		}
	}

	if (m_pUnknownC4)
	{
		operator delete(m_pUnknownC4);
		m_pUnknownC4 = LTNULL;
	}

	if (m_pUnknownC8)
	{
		operator delete(m_pUnknownC8);
		m_pUnknownC8 = LTNULL;
	}

	if (m_aIntensities)
	{
		delete [] m_aIntensities;
		m_aIntensities = LTNULL;
	}

	if (m_aTransitions)
	{
		operator delete(m_aTransitions);
		m_aTransitions = LTNULL;
	}

	m_bLevelInitialized = LTFALSE;
	return LT_OK;
}

// Setting up a level: read its control file, size the intensity and
// transition tables from it, and fill them.
//
// The control file is read through a reader derived from lithshared's, whose
// four file-access virtuals go through the engine's file streams instead of
// stdio. It lives on the stack for the whole function and is the whole frame,
// which is why the full path, when one has to be built, is parked in the dead
// slot of sDefine2.
//
// The path is built only when the control file name carries no directory of
// its own: then it is the working directory, a backslash unless the directory
// already ends in one (or a slash, or a colon), and the name. If the reader
// then refuses the file the function returns without freeing that path - the
// path is only released at 0x0043fecb, on the success side of that test, and
// the failure block returns at 0x0043fec8 without touching it. The leak is
// shipped, and reproduced.
//
// The trace runs at level three where the play calls run at four, and the
// format string misspells the last parameter as "sDeine3".
//
// One block short of identical. The shipped code reports a refused control
// file from two blocks, not one: an inline copy at 0x0043fea6 for the built
// path and a second at 0x00440025 for the name that already had a directory,
// each with its own dm_Error call, its own destructor call and its own
// return, and the four earlier error returns cross-jump into the second of
// them. Every compiler this tree can run merges the first into the second and
// leaves a jump, whatever shape the two failures are written in - an else
// against an early return, a shared flag tested after the branch, a goto to
// one block, the literal spelled out twice, a local static array - and
// whatever the pooling and optimisation flags. The residual is that one block:
// 1340 bytes against 1373, 446 instructions against 454.
// The intensity's and the transition's setters, as InitLevel reaches them
// (clientmusic.h declares the fields only; names ours).
class CDMIntensityQ528 : public CDMIntensity
{
public:
	void SetUnknown00(int n) { m_nUnknown00 = n; }
	void SetUnknown04(int n) { m_nUnknown04 = n; }
};

class CDMTransitionQ528 : public CDMTransition
{
public:
	void SetEnact(LTDMEnactTypes nEnact) { m_nEnact = nEnact; }
	void SetManual(int bManual)          { m_bManual = bManual; }
	void SetDMSegment(void* pSegment)    { m_pDMSegment = pSegment; }
};

LTRESULT CLTDirectMusicMgr::InitLevel(const char* sWorkingDirectory, const char* sControlFileName,
                                      const char* sDefine1, const char* sDefine2,
                                      const char* sDefine3)
{
	int i;
	static char sReadError[] = "ERROR! Unable to read control file. (CLTDirectMusicMgr::InitLevel)\n";
#ifdef PRIMAL_HUNT
	static char sSeparator[] = "\\";
#endif

#ifdef AVP2SRC_MODERN
	// fixed: "sDefine3", not "sDeine3" (RETAIL_BUGS.md)
	dm_Printf(3, "CLTDirectMusicMgr::InitLevel sWorkingDirectory=%s sControlFileName=%s sDefine1=%s sDefine2=%s sDefine3=%s\n",
	          sWorkingDirectory, sControlFileName, sDefine1, sDefine2, sDefine3);
#else
	dm_Printf(3, "CLTDirectMusicMgr::InitLevel sWorkingDirectory=%s sControlFileName=%s sDefine1=%s sDefine2=%s sDeine3=%s\n",
	          sWorkingDirectory, sControlFileName, sDefine1, sDefine2, sDefine3);
#endif

	CDMControlFileMgr controlFile;

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::InitLevel");
		return LT_ERROR;
	}

	if (m_bLevelInitialized)
	{
		dm_Error("ERROR! InitLevel has already been called and not terminated. (CLTDirectMusicMgr::InitLevel)\n");
		return LT_ERROR;
	}

	if (!sControlFileName)
	{
		dm_Error("ERROR! Control file name not valid. (CLTDirectMusicMgr::InitLevel)\n");
		return LT_ERROR;
	}

	if (!sWorkingDirectory)
	{
		dm_Error("ERROR! Working directory not valid. (CLTDirectMusicMgr::InitLevel)\n");
		return LT_ERROR;
	}

	SetWorkingDirectory(sWorkingDirectory, 1);

	if (sDefine1)
		controlFile.AddDefine(sDefine1);
	if (sDefine2)
		controlFile.AddDefine(sDefine2);
	if (sDefine3)
		controlFile.AddDefine(sDefine3);

	if (strchr(sControlFileName, '\\') == LTNULL && strchr(sControlFileName, '/') == LTNULL
	    && strchr(sControlFileName, ':') == LTNULL)
	{
		char* sFullName = new char[strlen(sWorkingDirectory) + strlen(sControlFileName) + 2];

		if (!sFullName)
			return LT_ERROR;

		strcpy(sFullName, sWorkingDirectory);

		if (strlen(sWorkingDirectory) != 0)
		{
			int nLen = strlen(sWorkingDirectory) - 1;

			if (sWorkingDirectory[nLen] != '\\' && sWorkingDirectory[nLen] != '/'
			    && sWorkingDirectory[nLen] != ':')
#ifdef PRIMAL_HUNT
				strcat(sFullName, sSeparator);
#else
				// AvP2 1.0.9.6 appends a literal (0x004481b0: the two bytes
				// of "\\" stored as one word).
				strcat(sFullName, "\\");
#endif
		}

		strcat(sFullName, sControlFileName);

		// The shipped code keeps a whole second copy of this failure block
		// inline here instead of jumping to the one below; the reconstruction
		// cannot stop the cross-jumper merging them. See the head of the
		// function.
		if (!controlFile.Init(sFullName))
		{
			dm_Error(sReadError);
#ifdef AVP2SRC_MODERN
			delete [] sFullName;	// fixed: not leaked (RETAIL_BUGS.md)
#endif
			return LT_ERROR;
		}

		delete [] sFullName;
	}
	else
	{
		if (!controlFile.Init(sControlFileName))
		{
			dm_Error(sReadError);
			return LT_ERROR;
		}
	}

	m_nNumIntensities = 0;
	m_nInitialIntensity = 0;
	m_nInitialVolume = 0;
	// AVP2: zeroed here (AvP2 1.0.9.6's 0x00448214), then read from the
	// control file below.
	m_nVolumeOffset = 0;
	m_nNumPChannels = 64;
	m_nNumVoices = 64;
	m_nSynthSampleRate = 44100;

	controlFile.GetKeyVal(LTNULL, "NUMINTENSITIES", m_nNumIntensities);
	controlFile.GetKeyVal(LTNULL, "INITIALINTENSITY", m_nInitialIntensity);
	controlFile.GetKeyVal(LTNULL, "INITIALVOLUME", m_nInitialVolume);
	// PH 0x0044c047..0x0044c07a, AvP2 1.0.9.6 0x0044827f.
	controlFile.GetKeyVal(LTNULL, "VOLUMEOFFSET", m_nVolumeOffset);
	controlFile.GetKeyVal(LTNULL, "PCHANNELS", m_nNumPChannels);
	controlFile.GetKeyVal(LTNULL, "VOICES", m_nNumVoices);
	controlFile.GetKeyVal(LTNULL, "SYNTHSAMPLERATE", m_nSynthSampleRate);

	if (m_PortParams.dwVoices != m_nNumVoices
	    || m_PortParams.dwAudioChannels != m_nNumPChannels
	    || m_PortParams.dwSampleRate != m_nSynthSampleRate)
	{
		CreatePort();
	}

	{
		CControlFileKey* pKey = controlFile.GetKey(LTNULL, "DLSBANKDIRECTORY");

		if (pKey)
		{
			CControlFileWord* pWord = pKey->GetFirstWord();

			if (pWord)
				dm_SetSearchDirectory(m_pLoader, CLSID_DirectMusicCollection, pWord->GetVal(), 0);
		}
	}

	ReadReverbParameters(&controlFile);

	if (m_nNumIntensities < 1)
	{
		dm_Error("ERROR! Number of intensities is not valid. NumIntensities = %i (CLTDirectMusicMgr::InitLevel)\n",
		         m_nNumIntensities);
		controlFile.Term();
		return LT_ERROR;
	}

	m_aIntensities = new CDMIntensity[m_nNumIntensities + 1];

	// The defaults go in through one-line setters: shipped forms each
	// element's address in a register for every store, base reloaded each
	// time (`mov 0xcc(%ebp),%esi; mov %ecx,%eax; add %esi,%eax; mov
	// %ebx,(%eax)`, 0x0044c1cf..), an inlined member call's `this`, where a
	// plain field store folds into a scale-1 SIB store.
	for (i = 0; i <= m_nNumIntensities; i++)
	{
		((CDMIntensityQ528*)&m_aIntensities[i])->SetUnknown00(0);
		((CDMIntensityQ528*)&m_aIntensities[i])->SetUnknown04(0);
	}

	m_nNumTransitions = (m_nNumIntensities + 1) * (m_nNumIntensities + 1);
	m_aTransitions = new CDMTransition[m_nNumTransitions];

	for (i = 0; i < m_nNumTransitions; i++)
	{
		((CDMTransitionQ528*)&m_aTransitions[i])->SetEnact(LTDMEnactNextMeasure);
		((CDMTransitionQ528*)&m_aTransitions[i])->SetManual(1);
		((CDMTransitionQ528*)&m_aTransitions[i])->SetDMSegment(LTNULL);
	}

	if (!m_aIntensities || !m_aTransitions)
	{
		dm_Error("ERROR! Unable to allocate memory for Intensity and Transition arrays. (CLTDirectMusicMgr::InitLevel)\n",
		         m_nNumIntensities);
		controlFile.Term();

		if (m_aIntensities)
		{
			delete [] m_aIntensities;
			m_aIntensities = LTNULL;
		}

		if (m_aTransitions)
		{
			delete [] m_aTransitions;
			m_aTransitions = LTNULL;
		}

		return LT_ERROR;
	}

	ReadDLSBanks(&controlFile);
	ReadStyles(&controlFile);
	ReadIntensities(&controlFile);
	ReadSecondarySegments(&controlFile);
	ReadMotifs(&controlFile);
	ReadTransitions(&controlFile);

	m_bLevelInitialized = LTTRUE;

	controlFile.Term();

	// AVP2: the level's initial volume is applied here, through the
	// interface (`call *0x20(%eax)` at 0x0044c2d9; AvP2 1.0.9.6's 0x004484d5).
	SetVolume(m_nInitialVolume);

	m_pQueueAfter = LTNULL;
	m_nCurIntensity = 0;

	return LT_OK;
}

// Changing intensity is the function that decides what plays next. It empties
// the pending queue and rebuilds it for the new intensity: the transition's
// segment first if there is one, then every segment of the new intensity, then
// the intensity's two numbers as requests of their own. Only one thing is
// handed to DirectMusic straight away - the transition segment, or failing
// that the intensity's first segment - and only when the enact type is not
// "next segment" (NOLF tested EnactToSegFlags' QUEUE flag; AvP2 tests the
// enact type itself).
//
// Intensity zero is a stop, and reported as an error afterwards; so is a
// change with no performance to play on. The queue cursor is left on the first
// queued request only when that request is the segment already sent to
// DirectMusic, otherwise it is cleared.
//
// The four pending nodes are allocated with `new`, which is why only the type
// store is guarded by the allocation's null test: the constructor runs on
// success and the assignments after it run regardless, so a failed allocation
// here writes through null. The shipped code has the same four unguarded
// stores, and the first of them keeps two copies of the whole tail - the null
// arm materialises the zero with `xor %eax,%eax` and then stores through it -
// which is what says `new` and not a written-out allocate-and-test.
//
// NOLF: byte-identical (798). It was 777 while both not-initialised blocks called
// one symbol: VC6 found the two tails identical and cross-jumped the second
// block into the first from the call onwards, where the shipped build emits
// it whole. The level test now calls dm_NotLevelInitError (see its
// declaration at the top of this file).
//
// AvP2 / Primal Hunt (0x0044c7e0, byte-identical, 847): the same three
// shapes at the same four sites -- AllocPop for PLAYTRANSITION (through
// CDMPendingAllocPopQ514), Alloc for PLAYSEGMENT and INTENSITY00, and
// AllocExpanded (now plain `new CDMPendingNode`) for INTENSITY04.  The
// notes that follow are NOLF's build (its addresses):
// * PLAYTRANSITION and PLAYSEGMENT (0x00440755, 0x00440805) are
//   CDMBank::AllocPop: Pop out of line on both paths, folded into one call.
// * INTENSITY00 (0x00440847) is CDMBank::Alloc: the fast path written out,
//   and a Pop CALL after the Grow. PlaySecondary and PlayMotif have the same
//   shape (0x00440a25).
// * INTENSITY04 (0x004408a6) is the same fast path, but the pop after the Grow
//   is expanded inline, with its own null test.
// Each site stores its type behind the allocator's null test, so all four are
// `new`. The last two therefore go through their own operator new. The two
// classes below are those operators, and nothing else. What the original
// source spelled here is not known. It may be one inline Pop that VC6
// expanded at some sites and not others: an inline Pop in AllocPop gives the
// full inline shape at every site until there are six or more sites, and the
// earliest sites lose it first, always both pops of a site together.
class CDMPendingAllocQ440 : public CDMPendingNode
{
public:
	CDMPendingAllocQ440(uint32 nType) : CDMPendingNode(nType) {}
	void* operator new(unsigned int nSize) { return g_PendingBank.Alloc(); }
};

inline void* CDMBankPopInlineQ440(CDMBank* pBank)
{
	void* pRecord = pBank->m_pFree;

	if (pRecord)
		pBank->m_pFree = *(void**)pRecord;

	return pRecord;
}

class CDMPendingAllocInlineQ440 : public CDMPendingNode
{
public:
	CDMPendingAllocInlineQ440(uint32 nType) : CDMPendingNode(nType) {}
	void* operator new(unsigned int nSize)
	{
		void* pRecord = g_PendingBank.m_pFree;

		if (pRecord)
		{
			g_PendingBank.m_pFree = *(void**)pRecord;
			return pRecord;
		}

		if (!g_PendingBank.Grow())
			return LTNULL;

		return CDMBankPopInlineQ440(&g_PendingBank);
	}
};

// The first allocation's shape: Pop out of line on both paths (AllocPop).
class CDMPendingAllocPopQ514 : public CDMPendingNode
{
public:
	CDMPendingAllocPopQ514(uint32 nType) : CDMPendingNode(nType) {}
	void* operator new(unsigned int nSize) { return g_PendingBank.AllocPop(); }
};

LTRESULT CLTDirectMusicMgr::ChangeIntensity(const int nNewIntensity,
                                            const LTDMEnactTypes nStart)
{
	CDMTransition*  pTransition;
	CDMSegment*     pSegment;
	CDMPendingNode* pPending;
	LTDMEnactTypes  nEnact;
	uint32          dwFlags;
	IDirectMusicSegmentState* pSegState = LTNULL;

	dm_Printf(4, "CLTDirectMusicMgr::ChangeIntensity nNewIntensity=%i nStart=%i\n",
	          nNewIntensity, nStart);

	if (!m_bInitialized)
	{
		dm_NotInitError("CLTDirectMusicMgr::ChangeIntensity");
		return LT_ERROR;
	}

	if (!m_bLevelInitialized)
	{
		dm_NotLevelInitError("CLTDirectMusicMgr::ChangeIntensity");
		return LT_ERROR;
	}

	if (!m_pPerformance)
		return LT_ERROR;

	if (nNewIntensity == 0)
	{
		Stop(nStart);
		return LT_ERROR;
	}

	if (nNewIntensity <= 0 || nNewIntensity > m_nNumIntensities)
		return LT_ERROR;

	EnterCriticalSection(&m_CriticalSection);

	if (m_nCurIntensity == nNewIntensity)
	{
		LeaveCriticalSection(&m_CriticalSection);
		dm_Printf(4, "CLTDirectMusicMgr::ChangeIntensity new intensity same as old nothing to do, exiting.\n");
		return LT_OK;
	}

	pTransition = GetTransition(m_nCurIntensity, nNewIntensity);

	if (!pTransition)
	{
		LeaveCriticalSection(&m_CriticalSection);
		dm_Error("ERROR! Invalid transition found, exiting. (CLTDirectMusicMgr::ChangeIntensity)\n");
		return LT_ERROR;
	}

	ClearPendingList();

	if (nStart == LTDMEnactInvalid)
		nEnact = pTransition->m_nEnact;
	else
		nEnact = nStart;

	dwFlags = EnactToSegFlags(nEnact);

	// AVP2: a next-segment start is tested on the enact type itself, and an
	// immediate one records its segment state on the primary list
	// (CDMPlayingList::Add, 0x0044e460), as PlaySecondary does.
	if (pTransition->m_pDMSegment)
	{
		if (nEnact == LTDMEnactNextSegment)
		{
			pPending = new CDMPendingAllocPopQ514(DMPENDING_PLAYTRANSITION);
			pPending->m_pSegment = (CDMSegment*)pTransition;
			m_PendingList.AddTail(pPending);
		}
		else
		{
			m_pPerformance->PlaySegment(pTransition->m_pDMSegment, dwFlags, 0, &pSegState);
			m_PrimaryList.Add(pSegState, LTNULL);
		}
	}
	else if (nEnact != LTDMEnactNextSegment)
	{
		pSegment = m_aIntensities[nNewIntensity].m_Segments.m_pHead;

		if (pSegment && pSegment->m_pDMSegment)
		{
			m_pPerformance->PlaySegment(pSegment->m_pDMSegment, dwFlags, 0, &pSegState);
			m_PrimaryList.Add(pSegState, pSegment);
		}
	}

	pSegment = m_aIntensities[nNewIntensity].m_Segments.m_pHead;

	while (pSegment)
	{
		if (pSegment->m_pDMSegment)
		{
			pPending = new CDMPendingAllocQ440(DMPENDING_PLAYSEGMENT);
			pPending->m_pSegment = pSegment;
			m_PendingList.AddTail(pPending);
		}

		pSegment = pSegment->m_pNext;
	}

	pPending = new CDMPendingAllocQ440(DMPENDING_INTENSITY00);
	pPending->m_nValue = m_aIntensities[nNewIntensity].m_nUnknown00;
	m_PendingList.AddTail(pPending);

	if (m_aIntensities[nNewIntensity].m_nUnknown04 > 0)
	{
		pPending = new CDMPendingNode(DMPENDING_INTENSITY04);
		pPending->m_nValue = m_aIntensities[nNewIntensity].m_nUnknown04;
		m_PendingList.AddTail(pPending);
	}

	if (!pTransition->m_pDMSegment && nEnact != LTDMEnactNextSegment)
		m_pQueueAfter = m_PendingList.m_pHead;
	else
		m_pQueueAfter = LTNULL;

	m_nCurIntensity = nNewIntensity;

	LeaveCriticalSection(&m_CriticalSection);
	return LT_OK;
}


// The thread's one allocation (0x0043f6dc..0x0043f70e) is the fast path
// written out AND the pop after the Grow expanded, ChangeIntensity's
// INTENSITY04 shape (CDMPendingAllocInlineQ440) on the playing bank.
// STAND-IN, name ours.
inline void* dmPlayingAllocInlineQ440()
{
	void* pRecord = g_PlayingBank.m_pFree;

	if (pRecord)
	{
		g_PlayingBank.m_pFree = *(void**)pRecord;
		return pRecord;
	}

	if (!g_PlayingBank.Grow())
		return LTNULL;

	return CDMBankPopInlineQ440(&g_PlayingBank);
}

// 0x0043f520 -- the notification thread (name ours).  Init hands it to
// _beginthread (0x0043fa79 pushes its address) and Term stops it by setting
// m_bQuitThread under the lock and waiting on the thread handle.  Everything
// goes through the published singleton (0x004b3fe0), reloaded after every
// call.
//
// Poll the performance's event every 100 ms; for each segment notification:
// on SEGALMOSTEND run the pending queue from behind m_pQueueAfter, and on
// SEGEND drop the playing node the finished state belongs to.  pSegState is
// set only when a message carries one and never cleared (0x0043f532 zeroes
// %ebp once, outside both loops), which is the shipped behaviour.
//
// AVP2: Primal Hunt's lithtech.exe 0x0044b550 (1054 bytes) and AvP2 1.0.9.6's
// 0x004477c0 (1043) are the newer shape this body now has: the queue runs
// only for the primary segment's SEGALMOSTEND, types 3 and 0xb start at once
// onto the primary list (CDMPlayingList::Add, 0x0044e460), the waiting
// secondaries and motifs on m_PendingSecondaryList start after it, and
// SEGEND looks in the primary list first, stopping a primary with enact 0
// and the other two with LTDMEnactImmediatly.
//
// NEAR (1035 of 1054; AvP2 1020 of 1043): ours cross-jumps the type-3 arm
// into the type-0xb arm's primary-list Add (both end `push <state>; mov
// g_pDirectMusicMgr; lea 0x7c; call`), where shipped schedules the 0xb
// arm's manager load ahead of its pushes (`add $0x7c,%ecx`,
// 0x0044b79c..0x0044b7a8) so only the Remove is shared; the secondary loop
// then takes the other scratch registers. One function-scope state local
// for every arm instead of four block ones: 1031, worse.

// The singleton itself: zero-initialised, in .bss (0x004b3fe0).
CLTDirectMusicMgr* g_pDirectMusicMgr;

void __cdecl dm_NotificationThread(void* pParam)
{
	DMUS_NOTIFICATION_PMSG* pMsg;
	IDirectMusicSegmentState* pSegState = LTNULL;
	void* pSegment = LTNULL;
	CDMPendingNode* pNode;

	EnterCriticalSection(&g_pDirectMusicMgr->m_CriticalSection);

	while (!g_pDirectMusicMgr->m_bQuitThread)
	{
		LeaveCriticalSection(&g_pDirectMusicMgr->m_CriticalSection);
		WaitForSingleObject(g_pDirectMusicMgr->m_hNotifyEvent, 100);
		EnterCriticalSection(&g_pDirectMusicMgr->m_CriticalSection);

		while (g_pDirectMusicMgr->m_pPerformance->GetNotificationPMsg(&pMsg) == 0)
		{
			if (!g_pDirectMusicMgr->m_bQuitThread &&
			    pMsg->guidNotificationType == GUID_NOTIFICATION_SEGMENT)
			{
				if (pMsg->punkUser)
				{
					pSegState = pMsg->punkUser;
					if (pSegState->GetSegment(&pSegment) < 0)
						pSegment = LTNULL;
				}

				if (pMsg->dwNotificationOption == DMUS_NOTIFICATION_SEGALMOSTEND)
				{
					// AVP2: only the primary segment's end runs the queue: a
					// secondary's or a motif's, or any segment state but the
					// newest primary one, is passed over -- and its message
					// is never freed (0x0044b62f..0x0044b66a jump past the
					// FreePMsg at 0x0044b901 straight to the next fetch).
					// RETAIL BUG: those notifications leak.
					if (g_pDirectMusicMgr->m_SecondaryList.FindBySegState(pSegState))
					{
#ifdef AVP2SRC_MODERN
						g_pDirectMusicMgr->m_pPerformance->FreePMsg(pMsg);
#endif
						continue;
					}

					if (g_pDirectMusicMgr->m_MotifList.FindBySegState(pSegState))
					{
#ifdef AVP2SRC_MODERN
						g_pDirectMusicMgr->m_pPerformance->FreePMsg(pMsg);
#endif
						continue;
					}

					if (g_pDirectMusicMgr->m_PrimaryList.m_pHead &&
					    g_pDirectMusicMgr->m_PrimaryList.m_pHead->m_pSegState != pSegState)
					{
#ifdef AVP2SRC_MODERN
						g_pDirectMusicMgr->m_pPerformance->FreePMsg(pMsg);
#endif
						continue;
					}

					if (g_pDirectMusicMgr->m_pQueueAfter)
						pNode = g_pDirectMusicMgr->m_pQueueAfter->m_pNext;
					else
						pNode = g_pDirectMusicMgr->m_PendingList.m_pHead;

					while (pNode)
					{
						if (pNode->m_nType == 0 || pNode->m_nType == 1 ||
						    pNode->m_nType == 2 || pNode->m_nType == 6 ||
						    pNode->m_nType == DMPENDING_PLAYSECONDARY ||
						    pNode->m_nType == DMPENDING_PLAYMOTIF ||
						    pNode->m_nType == 7)
						{
							pNode = pNode->m_pNext;
						}
						else if (pNode->m_nType == DMPENDING_INTENSITY00)
						{
							if (pNode->m_nValue == 0)
								pNode = pNode->m_pNext;
							else
							{
								if (pNode->m_nValue > 0)
									pNode->m_nValue--;

								if (pNode->m_nValue == 0)
									pNode = pNode->m_pNext;
								else
									pNode = g_pDirectMusicMgr->m_PendingList.m_pHead;
							}
						}
						else if (pNode->m_nType == DMPENDING_INTENSITY04)
						{
							g_pDirectMusicMgr->ChangeIntensity(pNode->m_nValue, LTDMEnactInvalid);
							pNode = g_pDirectMusicMgr->m_PendingList.m_pHead;
						}
						else if (pNode->m_nType == DMPENDING_PLAYSEGMENT)
						{
							// AVP2: played at once (0x2000) and recorded on
							// the primary list, with its segment.
							IDirectMusicSegmentState* pNewState = LTNULL;

							g_pDirectMusicMgr->m_pPerformance->PlaySegment(
								pNode->m_pSegment->m_pDMSegment, 0x2000, 0, &pNewState);
							g_pDirectMusicMgr->m_PrimaryList.Add(pNewState, pNode->m_pSegment);
							// The node stays on the pending list and becomes
							// the queue mark (PH 0x0044b769, AvP2 1.0.9.6
							// 0x004479d1 jump past the transition's Remove).
							break;
						}
						else if (pNode->m_nType == DMPENDING_PLAYTRANSITION)
						{
							// AVP2: the same, with no segment to record.
							IDirectMusicSegmentState* pNewState = LTNULL;

							g_pDirectMusicMgr->m_pPerformance->PlaySegment(
								pNode->m_pSegment->m_pDMSegment, 0x2000, 0, &pNewState);
							g_pDirectMusicMgr->m_PrimaryList.Add(pNewState, LTNULL);
							g_pDirectMusicMgr->m_PendingList.Remove(pNode);
							pNode = LTNULL;
							break;
						}
						else if (pNode->m_nType == 0xa)
						{
							g_pDirectMusicMgr->m_pPerformance->Stop(
								pNode->m_pSegment->m_pDMSegment, LTNULL, 0, 0x2000);
							pNode = pNode->m_pNext;
						}
					}

					g_pDirectMusicMgr->m_pQueueAfter = pNode;

					// AVP2: then every secondary and motif that was waiting
					// for this boundary starts (0x0044b7bc..0x0044b869) and
					// leaves the list.
					pNode = g_pDirectMusicMgr->m_PendingSecondaryList.m_pHead;

					while (pNode)
					{
						CDMPendingNode* pNext;

						if (pNode->m_nType == DMPENDING_PLAYSECONDARY)
						{
							IDirectMusicSegmentState* pNewState = LTNULL;

							g_pDirectMusicMgr->m_pPerformance->PlaySegment(
								pNode->m_pSegment->m_pDMSegment, 0x2280, 0, &pNewState);
							g_pDirectMusicMgr->m_SecondaryList.Add(pNewState, pNode->m_pSegment);
						}

						if (pNode->m_nType == DMPENDING_PLAYMOTIF)
						{
							IDirectMusicSegmentState* pNewState = LTNULL;

							g_pDirectMusicMgr->m_pPerformance->PlaySegment(
								pNode->m_pSegment->m_pDMSegment, 0x2080, 0, &pNewState);
							g_pDirectMusicMgr->m_MotifList.Add(pNewState, pNode->m_pSegment);
						}

						pNext = pNode->m_pNext;
						g_pDirectMusicMgr->m_PendingSecondaryList.Remove(pNode);
						pNode = pNext;
					}
				}

				if (pMsg->dwNotificationOption == DMUS_NOTIFICATION_SEGEND && pSegState)
				{
					CDMPlayingNode* pPlaying =
						g_pDirectMusicMgr->m_PrimaryList.FindBySegState(pSegState);

					if (pPlaying)
					{
						g_pDirectMusicMgr->m_PrimaryList.StopNode(g_pDirectMusicMgr,
							pPlaying, LTDMEnactInvalid, 0);
					}
					else
					{
						pPlaying = g_pDirectMusicMgr->m_SecondaryList.FindBySegState(pSegState);

						if (pPlaying)
						{
							g_pDirectMusicMgr->m_SecondaryList.StopNode(g_pDirectMusicMgr,
								pPlaying, LTDMEnactImmediatly, 0);
						}
						else
						{
							pPlaying = g_pDirectMusicMgr->m_MotifList.FindBySegState(pSegState);

							if (pPlaying)
							{
								g_pDirectMusicMgr->m_MotifList.StopNode(g_pDirectMusicMgr,
									pPlaying, LTDMEnactImmediatly, 0);
							}
						}
					}
				}
			}

			g_pDirectMusicMgr->m_pPerformance->FreePMsg(pMsg);
		}
	}

	LeaveCriticalSection(&g_pDirectMusicMgr->m_CriticalSection);
	g_pDirectMusicMgr->m_pPerformance->SetNotificationHandle(LTNULL, 0);
	CloseHandle(g_pDirectMusicMgr->m_hNotifyEvent);
	_endthread();
}


// 0x00440da0 and 0x00440ee0, 18 bytes each: iltdirectmusic.h's
// three-argument PlayMotif and StopMotif (vtable 0x004a2500 slots 12 and 13).
// The shipped build takes no style name, so each drops it and hands the rest
// to the two-argument overload -- through the vtable, `call *0x50(%eax)` /
// `call *0x54(%eax)`, because those are virtual (slots 20 and 21).
LTRESULT CLTDirectMusicMgr::PlayMotif(const char* sStyleName,
                                      const char* sMotifName,
                                      const LTDMEnactTypes nStart)
{
	return PlayMotif(sMotifName, nStart);
}

LTRESULT CLTDirectMusicMgr::StopMotif(const char* sStyleName,
                                      const char* sMotifName,
                                      const LTDMEnactTypes nStart)
{
	return StopMotif(sMotifName, nStart);
}


// 0x004420a0, 11 bytes: CDMSegment::GetName.  The allocated name when there
// is one, else the inline buffer -- `mov 0x4c(%ecx),%eax; test; jne` to the
// `ret`, then `lea 0xc(%ecx),%eax`.  Among the list helpers at
// 0x00441fe0..0x00442240 (both FindByNames call it twice, once to test and
// once for the compare).
const char* CDMSegment::GetName()
{
	if (m_pName)
		return m_pName;

	return m_szName;
}


#ifndef PRIMAL_HUNT
// 0x0044a2f0 and 0x0044a360 (AvP2 1.0.9.6): the two FindByNames, here after
// GetName as the image has them (Primal Hunt's are ltnew1.cpp's, before it).
// cl 9044 knows GetName leaves %ecx alone, so each second call for the
// compare goes out without reloading `this` (0x0044a310, 0x0044a37b).
CDMPlayingNode* CDMPlayingList::FindByName(const char* pName)
{
	if (pName)
	{
		CDMPlayingNode* pNode = m_pHead;

		while (pNode)
		{
			CDMSegment* pSegment = pNode->m_pSegment;

			if (pSegment && pSegment->GetName())
			{
				if (stricmp(pSegment->GetName(), pName) == 0)
					return pNode;
			}

			pNode = pNode->m_pNext;
		}
	}

	return LTNULL;
}

CDMSegment* CDMSegmentDefList::FindByName(const char* pName)
{
	if (pName)
	{
		CDMSegment* pSegment = m_pHead;

		while (pSegment)
		{
			if (pSegment->GetName())
			{
				if (stricmp(pSegment->GetName(), pName) == 0)
					return pSegment;
			}

			pSegment = pSegment->m_pNext;
		}
	}

	return LTNULL;
}
#endif
