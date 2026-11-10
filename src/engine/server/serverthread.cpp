// server.dll — the server's model-loading worker thread (reconstruction).
// Build /O1.
//
// Split from smgr.cpp: CServerThread's members are a translation unit of
// their own in both images (server.dll 0x10044f0f..0x10045067, lithtech.exe
// 0x00498290..0x00498460). The file name is ours.
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): this unit ends in <string>'s
// guarded initializer of std::ctype<unsigned short>::id (server.dll 0x10045067: guard 0x1008ec84, atexit of the empty
// 0x1002eaeb; lithtech.exe 0x00498460: guard 0x004f8014, atexit of the empty
// 0x00401790).
#if defined(PRIMAL_HUNT)
#include <string>
#endif
#include "servermgr.h"
#include "lthread.h"

// 0x1003512f — the worker thread's own construction; the base runs first and
// the manager is filled in later, by Init.
CServerThread::CServerThread()
{
	m_pServerMgr = LTNULL;
}

// 0x10035145 — which manager the thread works for.
LTBOOL CServerThread::Init(CServerMgr* pMgr)
{
	m_pServerMgr = pMgr;

	return LTTRUE;
}

// A critical section held for a scope (name ours): the constructor enters
// and the destructor leaves, which is what gives retail's /GX frame and the
// unwind entries that leave each lock (lithtech.exe 0x004982c0; the client
// loader thread's IsRequestQueued, 0x00419490, is the same code).
class CCSLockQ551
{
public:
	CCSLockQ551(LCriticalSection* pCS)
	{
		m_pCS = pCS;
		m_pCS->Enter();
	}

	~CCSLockQ551()
	{
		m_pCS->Leave();
	}

	LCriticalSection* m_pCS;
};

LTBOOL CServerThread::IsRequestQueued(void* pFile)
{
	LCriticalSection* pCS = &m_IncomingQueue.m_CS;
	CCSLockQ551 lock1(pCS);
	CCSLockQ551 lock2(&m_OutgoingQueue.m_CS);

	if (m_pServerMgr)
	{
		LThreadMessage* pCur;
		LThreadMessage* pRec;

		pCur = m_IncomingQueue.m_MessageList.m_pTail;

		while (pCur)
		{
			pRec = pCur;
			pCur = (pCur->m_pPrev == m_IncomingQueue.m_MessageList.m_pTail) ? LTNULL : pCur->m_pPrev;

			if ((void*)pRec->m_Data[3] == pFile)
				return LTTRUE;
		}

		pCur = m_OutgoingQueue.m_MessageList.m_pTail;

		while (pCur)
		{
			pRec = pCur;
			pCur = (pCur->m_pPrev == m_OutgoingQueue.m_MessageList.m_pTail) ? LTNULL : pCur->m_pPrev;

			if ((void*)pRec->m_Data[3] == pFile)
				return LTTRUE;
		}
	}

	return LTFALSE;
}


// serverengine.cpp's loader (0x1002b22c); serverengine.h is not included here.
LTRESULT se_LoadModelData(CServerMgr* pMgr, char* pFilename, void* pFile,
                          class Model** ppModel);

// 0x100351d9 / lithtech 0x47bfa0 — slot 0x0c of the thread's vftable: one
// load request off the incoming queue. The request is the same eight-dword
// record se_LoadModel posts (payload dwords 0 and 2 zero, the file handle in
// dword 3); anything else is ignored. The model is loaded here, on the
// worker, and the answer goes back on the outgoing queue in the shape
// OnModelLoaded reads: dword 0 is the failure flag (the `neg/sbb/neg` is
// `!= LT_OK`), dword 3 the file and dword 4 the model.
void CServerThread::ProcessMessage(LThreadMessage* pMsg)
{
	LThreadMessage reply;
	Model*         pModel;

	if (m_pServerMgr && pMsg->m_Data[0] == 0 && pMsg->m_Data[2] == 0)
	{
		void* pFile     = (void*)pMsg->m_Data[3];
		char* pFilename = fs_GetFileName(&m_pServerMgr->m_FileMgr, pFile);

		LTRESULT dResult = se_LoadModelData(m_pServerMgr, pFilename, pFile,
		                                    &pModel);

		reply.m_Data[0] = (dResult != LT_OK);
		reply.m_Data[2] = 0;
		reply.m_Data[3] = (uint32)pFile;
		reply.m_Data[4] = (uint32)pModel;

		m_OutgoingQueue.PostMessage(&reply);
	}
}
