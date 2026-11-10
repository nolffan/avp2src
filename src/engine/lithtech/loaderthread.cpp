// *********************************************************************** //
//
//  MODULE:   LOADERTHREAD (.CPP)   (file name ours) -- lithtech.exe only
//
//  The client's model-loader thread, CLoaderThread (vtable 0x004a1ddc):
//  its constructor, ProcessMessage and LoadModel (AvP2 1.0.9.6 0x00415660
//  .. 0x00415778; Primal Hunt 0x00419450 .. 0x00419660).  Its own
//  translation unit: in Primal Hunt the run follows the message helper's
//  <string> guard pair (0x00419420) and ends in its own (0x00419670,
//  0x00419690).  Split out of csvis.cpp (the constructor and LoadModel)
//  and clientmgr2.cpp (ProcessMessage); their views are repeated here.  The
//  client-manager thread view's IsRequestQueued between them (Primal Hunt
//  0x00419490) is still written in clientmgr3.cpp.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x419670).
#include <string>
#endif
#include "ltbasedefs.h"


class CClientMgrQ414;
class CClientMgrView;

// csvis.cpp's views, for LoadModel.
class CFileRecQ414
{
public:
	void* m_pData;                                             // 0x00
	// cmodelload.cpp's CFileRecQ474::m_pName, the interned path the loader
	// thread prints and loads by (0x004156f4, 0x00415705).
	uint8 m_pad_04[0x1c - 0x04];
	char* m_pName;                                             // 0x1c
};

void con_ConsolePrintQ414(char* pFormat, ...);                 // 0x0042e6f0

// clientmgr2.cpp's view of lthread.h's LThreadMessage, for ProcessMessage.
class CThreadMessageView
{
public:
	void*   m_pNext;                              // 0x00  (lthread.h)
	void*   m_pPrev;                              // 0x04  (lthread.h)
	uint32  m_nID;                                // 0x08  payload[0]
	uint32  m_Data0c;                             // 0x0c  payload[1]
	uint32  m_dResult;                            // 0x10  payload[2]
	void*   m_pFileRec;                           // 0x14  payload[3]
	void*   m_pModel;                             // 0x18  payload[4]
	uint32  m_Data1c;                             // 0x1c  payload[5]
};


// ======================================================================= //
//  CLoaderThread's constructor -- 0x00415660, 25 bytes
//
//  LThread's constructor (0x00444570), the vtable store (0x004a1ddc), and
//  +0x60 (the client manager ProcessMessage gates on) cleared; `this`
//  returned.  Byte for byte the shape of smgr.cpp's CServerThread
//  constructor (lithtech.exe 0x0047bec0, vtable 0x004a341c), and with the
//  vtable masked the two are indistinguishable to the scorer: while this one
//  was unwritten, ??0CServerThread was credited HERE and 0x0047bec0 showed
//  as unwritten.  The scalar-deleting destructor after it, 0x00415680, is
//  shared by both vtables (slot 0 of 0x004a1ddc and of 0x004a341c) -- the
//  linker folded the two identical ??_G bodies -- so it comes with this
//  class and with CServerThread alike.  The base and the slots are restated
//  from src/engine/server/lthread.h rather than included, so as to add
//  nothing else to this TU.  (Stand-in names; the class is CLoaderThread.)
// ======================================================================= //
class LThreadQ415
{
public:
	LThreadQ415();                                             // 0x00444570
	virtual ~LThreadQ415();                                    // 0x004445a0
	virtual void ThreadTerm();                                 // 0x004445c0
	virtual void ThreadRun();                                  // 0x00444760
	virtual void ProcessMessage(void* pMsg);

	uint8 m_pad_04[0x60 - 0x04];
};

class CLoaderThreadQ415 : public LThreadQ415
{
public:
	CLoaderThreadQ415();                                       // 0x00415660
	virtual void ProcessMessage(void* pMsg);                   // 0x004156a0

	CClientMgrQ414* m_pClientMgr;                              // 0x60
};

CLoaderThreadQ415::CLoaderThreadQ415()
{
	m_pClientMgr = LTNULL;
}


// 0x004156a0 -- the loader thread's side of the same conversation: slot 3
// (ProcessMessage) of CLoaderThread's vtable 0x004a1ddc.  Beside
// CLoaderThreadQ414::LoadModel (0x004156d0), which it calls.  thiscall, `ret $0x4`.  Gated on the manager at
// +0x60 (the field the constructor 0x00415660 zeroes), then the same two
// payload tests HandleThreadMessage makes, then LoadModel(payload[3]).  The
// request is cmodelload.cpp's cm_LoadModel (0x0047706f), which posts
// +0x08 = +0x10 = 0 and +0x14 = the file record.  Modelled without
// `virtual`: nothing here dispatches through the table.
class CLoaderThreadView
{
public:
	void ProcessMessage(CThreadMessageView* pMsg);                  // 0x004156a0
	void LoadModel(void* pFileRec);                                 // 0x004156d0

	uint8            m_pad_00[0x60];              // vptr + LThread
	CClientMgrView*  m_pClientMgr;                // 0x60
};

void CLoaderThreadView::ProcessMessage(CThreadMessageView* pMsg)
{
	if (m_pClientMgr)
	{
		if (pMsg->m_nID == 0 && pMsg->m_dResult == 0)
			LoadModel(pMsg->m_pFileRec);
	}
}


// ======================================================================= //
//  CLoaderThread::LoadModel -- 0x004156d0
//
//  THE NAME IS SHIPPED: "CLoaderThread::LoadModel(%s)" (0x004a86fc, pushed
//  at 0x004156f8).  The class is an LThread (src/engine/server/lthread.h):
//  its vtable 0x004a1ddc is slot 0 0x00415680 (scalar-deleting destructor,
//  calls 0x004445a0 ~LThread), slot 1 0x004445c0 ThreadTerm, slot 2
//  0x00444760 ThreadRun, slot 3 0x004156a0 ProcessMessage -- which gates on
//  +0x60, requires message words +0x08 and +0x10 zero and calls this with
//  word +0x14 (0x004156bd).  0x00415660 is the constructor (LThread ctor
//  0x00444570, vtable store, `+0x60 = 0`); clientmgr3.cpp declares it as
//  CCmgThread at the manager's +0x15a8.  The request this answers is the
//  one cmodelload.cpp's cm_LoadModel posts (0x0047706f: +0x08 = +0x10 = 0,
//  +0x14 = the file record) and the reply goes out on m_OutgoingQueue
//  (`lea 0x2c(%edi),%ecx`, 0x004442c0 LThreadQueue::PostMessage) carrying
//  the failure flag, the record and the model.
// ======================================================================= //

// lthread.h's LThreadMessage / LThreadQueue, restated.
class LThreadMessageQ414
{
public:
	LThreadMessageQ414();                                      // 0x004441a0

	LThreadMessageQ414* m_pNext;                               // 0x00
	LThreadMessageQ414* m_pPrev;                               // 0x04
	uint32              m_Data[6];                             // 0x08
};

class LThreadQueueQ414
{
public:
	LTRESULT PostMessage(LThreadMessageQ414* pMsg);            // 0x004442c0

	uint8 m_Opaque[0x28];
};

// cmodelload.cpp's cm_LoadModelDataQ474 (0x00477440).
class ModelQ414;
LTRESULT cm_LoadModelDataQ414(CClientMgrQ414* pMgr, char* pFilename,
                              CFileRecQ414* pFileRec,
                              ModelQ414** ppModel);            // 0x00477440

// The .bss word LoadModel tests before its trace line: the "DebugLoaders"
// console variable (lithtech's binding at 0x004aa0b0 points here), under
// dsi.cpp's name.
extern int g_CV_DebugLoaders;                                  // 0x004b3248

class CLoaderThreadQ414
{
public:
	void LoadModel(CFileRecQ414* pRec);                        // 0x004156d0

	void*            m_pVtable;                                // 0x00
	LThreadQueueQ414 m_IncomingQueue;                          // 0x04
	LThreadQueueQ414 m_OutgoingQueue;                          // 0x2c
	uint8            m_pad_54[0x60 - 0x54];
	CClientMgrQ414*  m_pClientMgr;                              // 0x60
};

void CLoaderThreadQ414::LoadModel(CFileRecQ414* pRec)
{
	LThreadMessageQ414 msg;
	ModelQ414*         pModel;
	LTRESULT           dResult;

	if (!m_pClientMgr)
		return;

	if (g_CV_DebugLoaders)
		con_ConsolePrintQ414("CLoaderThread::LoadModel(%s)", pRec->m_pName);

	dResult = cm_LoadModelDataQ414(m_pClientMgr, pRec->m_pName, pRec, &pModel);

	msg.m_Data[0] = dResult != LT_OK;
	msg.m_Data[2] = 0;
	msg.m_Data[3] = (uint32)pRec;
	msg.m_Data[4] = (uint32)pModel;
	m_OutgoingQueue.PostMessage(&msg);
}
