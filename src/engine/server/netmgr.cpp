// server.dll — CNetMgr (reconstruction). Build /O1.
#if defined(PRIMAL_HUNT)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x424890).
// Primal Hunt's server.dll the same (guard byte 0x1008ec84, atexit of
// 0x1002eaeb), so it is not limited to the engine's build.
#include <string>
#endif
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <new.h>
#include "servermgr.h"
#include "servermsg.h"
#include "packet.h"
#include "de_error.h"

// 0x10058778 — the net trace's timestamp counter, started at load (mode 1).
// Its dynamic initialiser is server.dll 0x10021179 (the zero-displacement
// jmp head) / 0x1002117e (push 1; mov ecx,&g_NetCounter; call the
// 0x10005f38 constructor; ret), the first code of this TU -- which is why
// the definition is first here. CBaseCounter has no destructor, so there
// is no atexit piece. server.dll only: lithtech.exe has no such
// initialiser anywhere (no `push 1; mov ecx,imm; call; ret` in its .text)
// and its trace lines push a literal 0 instead (see NET_TRACE_TIME).
// AVP2: lithtech.exe has it too -- PH's initialiser is 0x004678a0 (jmp
// head) / 0x004678b0 (push 1; mov $0x4f5770,%ecx; call 0x00426060).
CBaseCounter g_NetCounter(1);

// 0x1002136b / 0x00455310. The two scalar stores are in *this* order and not
// the other way round because lithtech.exe (size-exact, 242 of 242 bytes, and
// byte-identical for the first 213) puts `mov %ebx,0x100(%esi)` ahead of
// `mov %ebx,0xd4(%esi)` -- a straight transposition of two adjacent
// independent stores, which at /O2 (where the memset is expanded and the
// whole body is straight-line) follows source order. server.dll cannot read
// the order back: there the memset is a CALL, m_FrameTime goes out as an x87
// `fstps` whose position is pinned to the `fldz` the previous call leaves
// room for, and the m_pMainDriver store is sunk between two of memset's
// pushes.
CNetMgr::CNetMgr()
{
	m_pMainDriver = LTNULL;
	m_FrameTime = 0.0f;
	memset(&m_Guid, 0, sizeof(m_Guid));
	m_Flags = 0;
}

LTRESULT CNetMgr::Init()
{
	TermDrivers();

	AddDriver("dplay2");
	AddDriver("internet");

	return LT_OK;
}

// AVP2: the handler's notification slot (servermgr.h's CBaseMgr::_v10, at
// vtable +0x10) takes TWO arguments in both AvP2 1.0.9.6 and Primal Hunt,
// not NOLF's three: every call pushes the event and the name only (PH
// server.dll 0x1002b17d and 0x1002d408, lithtech.exe 0x004683c4 and
// 0x0046a589; AvP2 server.dll 0x10029725 and 0x1002b837), and the dead
// connection's name is the empty string, not the trace name buffer.
// servermgr.h is shared, so the call goes through this view of the same
// table (name ours).
class CNetHandlerQ556
{
public:
	virtual void _v00()=0;
	virtual void _v04()=0;
	virtual void _v08()=0;
	virtual void _v0c()=0;
	virtual void Notify(int nEvent, char* pName)=0;                 // 0x10
};

// 0x100213ee — the manager handed back. Term() is the whole body; the five
// sub-objects are the compiler's, expanded in reverse declaration order
// (m_QueueC4, m_QueueB8, m_Drivers, m_Connections, m_PacketBank).
CNetMgr::~CNetMgr()
{
	Term();
}


void CNetMgr::Term()
{
	TermDrivers();

	m_Name[0] = 0;
	m_pHandler = LTNULL;
	m_UnknownD8 = 0;

	TermQueue((LMessageList*)&m_QueueB8);
	TermConnections();
}

LTBOOL CNetMgr::SetNetName(char* pName)
{
#ifdef PRIMAL_HUNT
	// PH: the name cleared through the pointer strncpy is handed, before the
	// bank's grow size (server.dll 0x1002ad21); AvP2 1.0.9.6 stores the grow
	// size first (0x10029303).
	char* pDst = m_Name;

	m_UnknownD8 = 0;
	pDst[0] = 0;
	m_PacketBank.m_Bank.m_Unknown08 = 0x100;
	m_UnknownDC = 0;
	m_pTraceName = g_NetNameBuf;

	strncpy(pDst, pName, 100);
#else
	m_UnknownD8 = 0;
	m_Name[0] = 0;
	m_PacketBank.m_Bank.m_Unknown08 = 0x100;
	m_UnknownDC = 0;
	m_pTraceName = g_NetNameBuf;

	strncpy(m_Name, pName, 100);
#endif

	m_LastTime = dsi_GetTime();
	SetGuid(LTNULL);
	m_pMainDriver = LTNULL;

	return LTTRUE;
}

LTRESULT CNetMgr::GetServiceList(NetService*& pListHead)
{
	uint32 i;

	pListHead = LTNULL;
	for (i = 0; i < m_Drivers.m_nElements; i++)
		m_Drivers.m_pArray[i]->GetServiceList(&pListHead);

	return LT_OK;
}

LTRESULT CNetMgr::FreeServiceList(NetService* pListHead)
{
	while (pListHead)
	{
		NetService* pNext = pListHead->m_pNext;
		::operator delete(pListHead);
		pListHead = pNext;
	}

	return LT_OK;
}

LTRESULT CNetMgr::SelectService(HNETSERVICE hNetService)
{
	uint32 i = 0;
	uint32 nIndex = (uint32)-1;

	while (i < m_Drivers.m_nElements)
	{
		if (m_Drivers.m_pArray[i] == ((CNetService*)hNetService)->m_pDriver)
		{
			nIndex = i;
			break;
		}

		i++;
	}

	if (nIndex == (uint32)-1)
		return LT_NOTINITIALIZED;

	if (!((CNetService*)hNetService)->m_pDriver->SelectService(hNetService))
	{
		m_pMainDriver = ((CNetService*)hNetService)->m_pDriver;
		return LT_OK;
	}

	return LT_ERROR;
}

LTRESULT CNetMgr::UpdateSessionName(const char* sName)
{
	if (!m_pMainDriver)
		return LT_NOTINITIALIZED;

	return m_pMainDriver->UpdateSessionName(sName);
}

LTRESULT CNetMgr::GetTcpIpAddress(char* sAddress, uint32 dwBufferSize,
                                  unsigned short& hostPort)
{
	if (!m_pMainDriver)
	{
		sAddress[0] = 0;
		return LT_NOTINITIALIZED;
	}

	return m_pMainDriver->GetTcpIpAddress(sAddress, dwBufferSize, hostPort);
}

void CNetMgr::Disconnect(NetConnection* pConn, uint32 nReason)
{
	if (!pConn)
		return;

	RemoveConnection(pConn);
	DebugPrint(pConn, 1, "CNetMgr::Disconnect called, (reason %d).", nReason);

	pConn->m_pDriver->Disconnect(pConn, nReason);
}

void CNetMgr::Update(char* pTraceName, float fCurTime, int bAllowTimeout)
{
	CMessageRef msg;
	uint32 i;
	float fDelta;

	m_pTraceName = pTraceName;

	fDelta = fCurTime - m_LastTime;
	fDelta = LTCLAMP(fDelta, 0.001f, 0.3f);

	m_LastTime = fCurTime;
	m_FrameTime = fDelta;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	for (i = 0; i < m_Connections.GetSize(); i++)
	{
		NetConnection* pConn = m_Connections.m_pArray[i];

		// A connection flagged for death gets told three times, then dropped.
		if (pConn->m_Flags & 4)
		{
			{
				ServerMessage* pMsg = msg.m_pMsg;
				pMsg->m_Unknown38 = 1;
				pMsg->m_Unknown34 = 1;
			}
			msg.m_pMsg->m_pData[0] = 1;
			msg.m_pMsg->WriteByte(1);

			SendPacket(msg.m_pMsg, pConn, 0);
			SendPacket(msg.m_pMsg, pConn, 0);
			SendPacket(msg.m_pMsg, pConn, 0);

			Disconnect(pConn, 2);

			i = (uint32)-1;
			continue;
		}

		// Resend whatever has waited too long for its ack.
		CNetSentPacket* pCur = (CNetSentPacket*)pConn->m_SentList.m_pTail;
		while (pCur)
		{
			CNetSentPacket* pRec = pCur;
			pCur = (pCur->m_pNext == (CNetSentPacket*)pConn->m_SentList.m_pTail) ?
				LTNULL : pCur->m_pNext;

			pRec->m_ResendTimer += fDelta;
			if (pRec->m_ResendTimer >= g_NetResendTime)
			{
				SendPacketData(pRec->m_Data0C, pConn);
				((LMessageList*)&pConn->m_SentList)->Remove((LThreadMessage*)pRec);
				m_PacketBank.Free(pRec);
			}
		}

		pConn->m_SendPPS.Update(fDelta);
		pConn->m_SendBPS.Update(fDelta);
		pConn->m_RecvPPS.Update(fDelta);
		pConn->m_RecvBPS.Update(fDelta);

		pConn->m_Unknown98 += fDelta;
		pConn->m_AckTimer += fDelta;
		pConn->m_UnknownB4 -= fDelta;
		pConn->m_RecvWait += fDelta;

		if (!(pConn->m_Flags & 1))
		{
			if (pConn->m_AckTimer > g_NetAckTime)
			{
				DebugPrint(pConn, 2, "Sending ack  local: %d remote: %d.",
					pConn->m_PacketID - 1, pConn->m_RemoteAckID);

				msg.Init(msg_AllocPacket(0x44c, 0x44c));

				WriteAckPacket(pConn, msg.m_pMsg);
				SendPacket(msg.m_pMsg, pConn, 0);
			}
		}

		if (bAllowTimeout && g_NetTimeouts)
		{
			if (pConn->m_RecvWait > pConn->m_pDriver->m_Timeout)
			{
				i--;

				if (g_DebugLevel > 0)
					dsi_ServerError("Connection dead (timed out).");

				RemoveConnection(pConn);
				((CNetHandlerQ556*)m_pHandler)->Notify(5, g_EmptyString);
				pConn->m_pDriver->Disconnect(pConn, 3);
			}
		}
		else
		{
			pConn->m_RecvWait = 0.0f;
		}
	}

	m_RateE0.Update(fDelta);

	if (g_NetShowStats)
	{
		for (i = 0; i < m_Connections.m_nElements; i++)
		{
			NetConnection* pConn = m_Connections.m_pArray[i];
			uint32         nRecvQueued;
			uint32         nSendQueued;

			DebugPrint(pConn, 0, "Conn %d Info - Ping: %.2f", i, pConn->m_Ping);

			// AVP2: each queue size is read into a register ahead of its
			// line's float arguments (PH server.dll 0x1002b14b, 0x1002b17a),
			// so it is a local of its own.
			nRecvQueued = pConn->m_GuaranteedQueue.m_nMessages;
			DebugPrint(pConn, 0, "Conn %d Recv - Wait: %.1f, Q Size: %d, PPS: %.1f, BPS: %.1f",
				i, pConn->m_RecvWait, nRecvQueued,
				pConn->m_RecvPPS.m_Value / pConn->m_RecvPPS.m_Time,
				pConn->m_RecvBPS.m_Value / pConn->m_RecvBPS.m_Time);
			nSendQueued = pConn->m_SendQueue.m_nMessages;
			DebugPrint(pConn, 0, "Conn %d Send - Wait: %.1f, Q Size: %d, PPS: %.1f, BPS: %.1f",
				i, pConn->m_SendWait, nSendQueued,
				pConn->m_SendPPS.m_Value / pConn->m_SendPPS.m_Time,
				pConn->m_SendBPS.m_Value / pConn->m_SendBPS.m_Time);
		}
	}

	for (i = 0; i < m_Drivers.m_nElements; i++)
		m_Drivers.m_pArray[i]->Update();
}

void CNetMgr::StartGettingPackets()
{
	m_Flags |= 1;
}

void CNetMgr::EndGettingPackets()
{
	m_Flags &= ~1;
}

// Byte-identical on both images. Two spellings carry it:
//
// * The guaranteed loop runs on a LOCAL, `bDone`, which starts as the
//   connection's flag and is then set from TranslatePacket. The `if (bDone)
//   return` just before the back edge tells VC6 that bDone is zero there, so
//   the back edge skips the loop test and jumps straight to the
//   FindQueuedPacket pushes (0x10022930 `jmp 0x100228db`). No value is live
//   across a call, so nothing is spilled. The flag spelled out in the `while`
//   re-tests memory on every pass. A local that holds only the flag (never
//   reassigned) stays live across the loop and gets spilled (+9 bytes).
// * The memcpy destination is read through `uint8** ppData = &pMsg->m_pData`.
//   Taking the field's address stops VC6 hoisting the m_pData load above the
//   m_Unknown34 store. /O1 does not care, but /O2 (lithtech 0x004579d6) keeps
//   the store first and loads src, then dest. Same fix as WriteAckPacket below.
// AVP2: the trace writes into its caller's buffer and is handed the
// direction byte GetPacket was given (server.dll 0x1002c280: push the second
// argument, &szTrace, 1, the length, pMsg; `add $0x14,%esp`); servermgr.h
// declares the five-argument form.

LTBOOL CNetMgr::GetPacket(ServerMessage* pMsg, uint32 nUnused)
{
	CBaseDriver* pDriver;
	NetConnection* pConn;
	uint32 i;
#ifdef LT_SERVER_BUILD
	char szTrace[0x2000];                    // -0x2008(%ebp)
	LTBOOL bPrint;
#endif

	// Fresh arrivals straight off the drivers.
	for (i = 0; i < m_Drivers.m_nElements; i++)
	{
		pDriver = m_Drivers.m_pArray[i];
		while (pDriver->ReceivePacket(pMsg))
		{
			AddRecvBytes(pMsg->m_pConnection, pDriver->GetRecvSize() + pMsg->m_Unknown34);
			pMsg->m_Unknown38 = 1;

#ifdef LT_SERVER_BUILD
			// The packet trace is server.dll's alone: Primal Hunt's
			// lithtech.exe 0x00468d80 goes from the read reset straight to
			// TranslatePacket (0x00468dd0 -> 0x00468dd6).
			//
			// AVP2: the three category filters live here now, as strstr
			// tests on the finished line (server.dll 0x1002c2a4..0x1002c322:
			// "(UUP)", "(UPD)", "(NM )"), and the line goes out through
			// dsi_ServerError (0x10012297) only if none of them dropped it.
			if (g_NetLogPackets)
			{
				net_LogPacket(pMsg, pMsg->m_Unknown34, 1, szTrace, (uint8)nUnused);
				pMsg->m_Unknown38 = 1;

				bPrint = LTTRUE;

				if (!(g_NetLogPackets & 2) && strstr(szTrace, "(UUP)"))
					bPrint = LTFALSE;

				if (!(g_NetLogPackets & 4) && strstr(szTrace, "(UPD)"))
					bPrint = LTFALSE;

				if (!(g_NetLogPackets & 8) && strstr(szTrace, "(NM )"))
					bPrint = LTFALSE;

				if (bPrint)
					dsi_ServerError(szTrace);
			}
#endif

			if (TranslatePacket(pMsg, pMsg->m_pConnection, 1))
				return LTTRUE;
		}
	}

	for (i = 0; i < m_Connections.m_nElements; i++)
	{
		pConn = m_Connections.m_pArray[i];

		// Whatever already arrived and waited.
		CNetRecvPacket* pCur = (CNetRecvPacket*)pConn->m_RecvList.m_pTail;
		while (pCur)
		{
			CNetRecvPacket* pRec = pCur;
			pCur = (pCur->m_pNext == (CNetRecvPacket*)pConn->m_RecvList.m_pTail) ?
				LTNULL : pCur->m_pNext;

			memcpy(pMsg->m_pData, pRec->m_pData, pRec->m_Len);
			pMsg->m_Unknown34 = (uint16)pRec->m_Len;
			pMsg->m_Unknown38 = 1;
			pMsg->m_pConnection = pConn;

			pConn->m_RecvList.Remove((LThreadMessage*)pRec);
			FreeRecvPacket(pRec);

			if (TranslatePacket(pMsg, pConn, 0))
				return LTTRUE;
		}

		// Guaranteed packets become visible in order.
		int bDone = pConn->m_Flags & 1;
		while (!bDone)
		{
			CNetRecvPacket* pRec = (CNetRecvPacket*)FindQueuedPacket(&pConn->m_GuaranteedQueue,
			                                                         pConn->m_PacketID);
			if (!pRec)
				break;

			pMsg->m_pConnection = pConn;
			uint16 nLen = (uint16)pRec->m_Len;
			pMsg->m_Unknown34 = nLen;
			uint8** ppData = &pMsg->m_pData;
			memcpy(*ppData, pRec->m_pData, nLen);

			DebugPrint(pConn, 1, "Resolving guaranteed packet %d", pConn->m_PacketID);
			pMsg->m_Unknown38 = 1;

			bDone = TranslatePacket(pMsg, pConn, 0);
			if (bDone)
				return LTTRUE;
		}
	}

	return LTFALSE;
}


// 0x10058798 — how many times the packet bank has been asked for. Signed,
// because the term below opens with `jle` rather than `je`: a term arriving
// before any init leaves the counter negative and every later term returns
// early. Zeroed .bss in the shipped image; the address is masked in
// verification, so what matters is that it is one dword of its own.
static int g_nNetBankRefs;

// 0x10058790 (lithtech.exe 0x004b40d8) — the bank's list of live records: a
// count at +0x00 and the head link at +0x04, ONE file-scope object with a
// constructor and a destructor. The dynamic-initialiser group it emits is in
// both images: server.dll 0x10025f92 (the _$E head, call/jmp), 0x10025f9c
// (constructor body: +0x00 then +0x04 zeroed), 0x10025fab (atexit registrar)
// and 0x10025fb7 (destructor body: +0x04 then +0x00, the REVERSED order that
// says it is one object); lithtech.exe 0x0045bfd0, 0x0045bfe0, 0x0045bff0 and
// 0x0045c000, the same four. The two stores in each are goodlinklist.h's
// CGLinkedList constructor and RemoveAll, but with no vtable store, so not
// that class itself (name ours). servermsg.cpp keeps its spare messages on
// the same object (msg_AllocPacket, ServerMessage::_v0c), so it is not static.
class CNetBankList
{
public:
	CNetBankList()
	{
		m_nCount = 0;
		m_pHead  = LTNULL;
	}

	~CNetBankList()
	{
		m_pHead  = LTNULL;
		m_nCount = 0;
	}

	uint32  m_nCount;                        // +0x00
	LTLink* m_pHead;                         // +0x04
};

CNetBankList g_NetBankList;

// 0x1005879c — one more dword the term clears with it; servermsg.cpp's
// msg_AllocPacket counts into it every message it has to `new` (name ours).
uint32  g_nUnknown79c;

// What a banked record is destroyed through. The term reaches slot 0 with a
// flag of one, which is VC6's scalar deleting destructor -- so the record's
// class has a virtual destructor and this is a plain `delete`. Nothing names
// the class; only that one slot is established (name ours).
class INetBankItem
{
public:
	virtual ~INetBankItem();
};


// ---- the constructors and the bank free --------------------------------- //

// A tracker starts at zero with a tenth-of-a-millisecond floor and a two
// second window (the constants are what they are; the names are ours).
CRateTracker::CRateTracker()
{
	m_Time = 0.0001f;
	m_Unknown08 = 2.0f;
	m_Value = 0.0f;
}

// 0x10023fe7 and 0x10024097 are `CMoArray<T, DefaultCache>::CMoArray()`, and
// the two lists are now servermgr.h's typedefs onto collection.h's template,
// so the constructor, the destructor and every array body below came off this
// file with them. What used to stand here was a hand-written 34-byte
// constructor that could only get NEAR (the shipped build materialises the
// zero after caching `this` in esi and pushes Init's two arguments apart),
// because a non-template member declared out-of-class is the one construct
// /Ob1 never expands -- which is also why lithtech.exe could never match it:
// there (0x00455310) the whole constructor is expanded into CNetMgr's.

// CNetRecvPacket::Term (servermgr.h declares it, name ours) -- the bank
// record's teardown, which is an explicit destructor call on the record in
// its MESSAGE shape: three words of link, then a whole ServerMessage at
// +0x0c (0x0c + 0x64 = 0x70, the bank's element size). The message's
// destructor is its payload collection's (CMoArray<uint8, DefaultCache> at
// +0x50, vtable 0x004a1ab8 in lithtech.exe) and then CAbstractIO's vptr
// store (0x004a2b44), the pair servermsg.cpp's CNetMsgRecord and
// netobj.cpp's CNetMsgRecordQ458 carry.
//
// It was declared and never defined, so every caller CALLED it. Defined
// here, inline and above its one caller:
//   * server.dll /O1: CNetBank::Free (0x10024226) still calls the plain
//     destructor out of line, and that COMDAT (??1CNetMsgRecordB3) matches
//     0x100242c1 as servermsg.cpp's CNetMsgRecord does;
//   * lithtech.exe /O2: the bank's slot-2 wrapper (0x00459dc0) carries the
//     whole teardown inline and is IDENTICAL; the message's own destructor
//     comes out standalone and matches 0x00455bc0 as msgterm.cpp's does.
//
// The `Destroy()` level between Term and the explicit destructor call is
// measured, not decoration: without it /O2 has budget left in 0x00459dc0
// and expands SetSize2 too (73 bytes against 49).
//
// NEAR, one instruction: NetConnection::Term (0x004551c0, 252 of 253
// bytes). There VC6 expands netbank_FreeMessages, Free, Term and Destroy
// and stops at the record's destructor; shipped emits that stop as
// `push $0 ; call 0x0045a160` (??_G with a zero flag, VC6's spelling of an
// explicit destructor call it declines to expand), ours expands the ??_G
// shell as well and calls the message destructor on +0x0c. The caller's
// inline budget is spent one step later than shipped's. Not moved by: a
// flattened record (message fields directly on it -- 1 block here, but
// shipped CNetMgr::Update, 0x00455925, calls the message destructor on
// +0x0c, which only this member layout can produce), a template destroy
// helper, user-declared versus implicit
// destructors, a polymorphic CAbstractIO view, an extra inline level on the
// four queue views, a derived packet-data class, or a wrapper around the
// netbank_FreeMessages call.
// (the base's store: CAbstractIO's own destructor, as servermsg.cpp's view)
class CTermAbstractIOB3
{
public:
	~CTermAbstractIOB3() { ((CAbstractIO*)this)->CAbstractIO::~CAbstractIO(); }

	void*  m_pVtbl;                          // +0x00
	uint32 m_nRefCount;                      // +0x04
};

class CTermMessageB3
{
public:
	CTermAbstractIOB3             m_Base;            // +0x00
	uint8                         m_pad_08[0x50 - 0x08];
	CMoArray<uint8, DefaultCache> m_PacketData;      // +0x50
};

class CNetMsgRecordB3
{
public:
	void Destroy() { this->~CNetMsgRecordB3(); }

	uint32         m_Unknown00;              // +0x00  the bank's free-list next
	uint32         m_Unknown04;              // +0x04
	uint32         m_Unknown08;              // +0x08
	CTermMessageB3 m_Msg;                    // +0x0c
};

inline void CNetRecvPacket::Term()
{
	((CNetMsgRecordB3*)this)->Destroy();
}

// Returning a packet record tears it down first, then threads it onto the
// bank's free list — the inline free, not the out-of-line one.
//
// WHERE THIS LIVES IN THE TWO IMAGES, because the lithtech side was once
// recorded wrongly. server.dll 0x10024226 is this function and is what
// the object matches. lithtech.exe's counterpart is 0x00459dc0, 49 bytes,
// which is slot 2 of CNetBank's vftable (0x004a2b30) with this body AND the
// record teardown expanded into it; there is no standalone copy, and now
// that Free is `inline` this object emits none for lithtech.exe either. The
// 28-byte lithtech "match" at 0x00412890 that the byte comparison used to report for
// this symbol was a FALSE POSITIVE and belongs to another translation unit: the
// scalar deleting destructor at 0x00412ed0 restores vftable 0x004a1cf4 and
// then 0x004a14c8, and 0x004a14c8 is the abstract CBankBase table (all
// three slots _purecall, 0x004928e7) shared with clientwin.cpp's Clu banks.
// The byte agreement is real and meaningless: every bank in the family
// frees the same way, so a bank whose record has a NON-virtual destructor
// -- a direct call where the generic 0x00483470 does `call *(eax)` -- comes
// out byte-identical to this one outside the masked displacement.
inline void CNetBank::Free(void* pRec)
{
	((CNetRecvPacket*)pRec)->Term();
	sb_Free(&m_Bank, pRec);
}

// 0x10023fdb — slot 2 of the bank's vftable, which is a forwarder onto the
// worker above and not the worker itself: `push 0x4(esp) ; call Free ;
// ret $4`, twelve bytes. servermgr.h carries the vftable evidence.
//
// Free above is `inline` now, and this wrapper is where lithtech.exe
// collects it: 0x00459dc0 (49 bytes) is IDENTICAL, Free and the record
// teardown expanded down to the `call 0x0040bfe0` SetSize2. server.dll still
// calls Free out of line from here (0x10023fdf), from Update (0x10021784)
// and from netbank_FreeMessages (0x1002427f) -- /Os declines it at all three
// -- so both 0x10024226 and this 12-byte forwarder still match there. The
// objection this note used to raise (Free expanding at /O1) was measured
// and does not happen; the dependent it was waiting for, the record
// teardown, is CNetRecvPacket::Term above.
void CNetBank::FreeVoid(void* pRec)
{
	Free(pRec);
}

// Removing a driver is expected to shorten the list; if it does not, stop
// rather than spin.
void CNetMgr::TermDrivers()
{
	uint32 nCount;
	uint32 nBefore;

	nCount = m_Drivers.m_nElements;
	while (nCount > 0)
	{
		nBefore = nCount;
		RemoveDriver(m_Drivers.m_pArray[0]);
		nCount = m_Drivers.m_nElements;

		if (nBefore == nCount)
			break;
	}
}

void CNetMgr::TermConnections()
{
	// ReleaseQueued (0x10023542), not CloseConnection (0x10022bc8): the
	// queue this drains is the pending-record queue, and this was read off the
	// shipped call target rather than trusting the name.
	while (m_QueueC4.m_pTail)
		ReleaseQueued((CNetPending*)m_QueueC4.m_pTail);
}

// Both trackers are fed per packet: one counts bytes, the other counts the
// packet itself. The constant really is 1.0f — the packets-per-second
// tracker gains one per packet. (It read 0.001f here until the send side
// was reconstructed and the two `fadds` turned out to share one .rdata
// address; a float operand is a masked relocation, so the byte comparison cannot tell
// the two constants apart and had been reporting a match either way.)
void CNetMgr::AddRecvBytes(NetConnection* pConn, uint32 nBytes)
{
	if (!pConn)
		return;

	pConn->m_RecvPPS.m_Value += 1.0f;
	pConn->m_RecvBPS.m_Value += nBytes;
}


// With no driver selected there is no session to name, and the caller's
// buffer is emptied rather than left as it was.
LTRESULT CNetMgr::GetSessionName(char* sName, uint32 dwBufferSize)
{
	CBaseDriver* pDriver = m_pMainDriver;

	if (!pDriver)
	{
		sName[0] = 0;
		return LT_NOTINITIALIZED;
	}

	return pDriver->GetSessionName(sName, dwBufferSize);
}


// ---- the shared message list -------------------------------------------- //

// CMessageQueueBase::Append USED to be defined here, and the out-of-line copy
// still sits in this module's contribution in both images (server.dll
// 0x100242e2, lithtech 0x45a1c0). It is now in-class in lthread.h, because
// lithtech expands it two levels deep — inside a Prepend that is itself
// expanded — at 0x41d580 (GenAppend), 0x43a03a (GenAppendList) and 0x444384
// (PostMessage), which /Ob1 can only do for a definition the header carries.
//
// The COMDAT this module emits for it is the one the linker keeps: three
// callers here decline the expansion at /Ot (0x457bce, 0x45840a, 0x459227)
// and every caller declines at /Os.


// ---- one connection ----------------------------------------------------- //

// The driver pointer is deliberately left alone: whoever builds the
// connection fills it in before anything reaches it.
NetConnection::NetConnection()
	: m_Counter(0)
{
	m_Unknown68 = 0;
	m_Unknown64 = 0;
	m_Flags = 0;
}

// 0x1002425d — the CNetBank flavour of tm_FreeMessages: walk the list with
// the cursor-alias trick, hand each record to the bank's Free (a thiscall on
// the bank, so the bank is the second argument here and `this` there), and
// leave the list empty (name ours, from its shape).
//
// `inline`: lithtech.exe has no standalone copy -- NetConnection::Term
// (0x004551c0) carries it expanded, with Free expanded inside it. server.dll
// still calls 0x1002425d out of line (/Os declines it) and still matches.
inline void netbank_FreeMessages(LMessageList* pList, CNetBank* pBank)
{
	LThreadMessage* pCur = pList->m_pTail;
	LThreadMessage* pFree;

	while (pCur)
	{
		pFree = pCur;
		pCur = (pCur->m_pPrev == pList->m_pTail) ? LTNULL : pCur->m_pPrev;

		pBank->Free(pFree);
	}

	pList->m_pTail = LTNULL;
	pList->m_nMessages = 0;
}

// Two things NetConnection's declaration in servermgr.h does not carry.
//
// First, three fields it leaves in padding — +0x78, +0x80 and a three-float
// array at +0xa0 — plus a name for +0x74. The two counters are named by the
// format string NetConnection::Term prints them with, "total sent: %d,
// resent: %d", which is the only attestation either of them has;
// servermgr.h calls +0x74 m_RemoteAckID, and the string says it is the
// packets-sent total. The +0xa0 array is an array because
// CNetMgr::AddConnection zeroes it with a *loop* (offset 0xa0 to 0xac in
// steps of four) rather than three stores.
//
// The rest of the fields are servermgr.h's under servermgr.h's names; they
// are repeated here so the whole of AddConnection's reset can run through
// one view instead of alternating between two.
class NetConnectionResetView
{
public:
	uint8  m_pad_00[0x6c];
	uint32 m_Unknown6C;                      // 0x6c
	uint32 m_Flags;                          // 0x70
	uint32 m_nSent;                          // 0x74
	uint32 m_nResent;                        // 0x78
	uint32 m_Unknown7C;                      // 0x7c  (m_PacketID)
	uint32 m_Unknown80;                      // 0x80
	float  m_RecvWait;                       // 0x84
	float  m_SendWait;                       // 0x88
	uint32 m_nPacketsRecv;                   // 0x8c  bumped once per packet
	                                         //       accepted, and printed as
	                                         //       "Got packet %d"
	uint8  m_pad_90[0x94 - 0x90];
	float  m_AckTimer;                       // 0x94
	float  m_Unknown98;                      // 0x98
	float  m_Ping;                           // 0x9c
	float  m_UnknownA0[3];                   // 0xa0
	uint8  m_pad_ac[0xb4 - 0xac];            //       the connection's counter
	float  m_UnknownB4;                      // 0xb4  counts down; when it goes
	                                         //       negative WriteAckPacket
	                                         //       asks for a ping
	uint16 m_UnknownB8;                      // 0xb8  the ping sequence number,
	                                         //       one past servermgr.h's
	                                         //       last modelled field
};

// ServerMessage's dword writer and the room-left query. servermsg.h names
// the byte, word and block writers but neither of these (names ours — the
// second from the two places its answer is compared against, a length plus
// four and the 0x2c an ack packet needs; it is signed, since both compares
// are `jge`/`jl`).
class ServerMessageWriteView
{
public:
	void WriteDWord(uint32 nValue);          // 0x1000ecd6

	// IN-CLASS, and that is load-bearing: both images are /Ob1, so only an
	// in-class (or `inline`-marked) body is an expansion candidate at all.
	// server.dll's /Os declines it and CALLS 0x1000e845 (0x10022ed5 and
	// 0x10022f6c inside SendPacketData itself); lithtech.exe's /Ot accepts
	// it and expands it at both sites (0x004584c3 and 0x00458572). One
	// spelling, both images -- which is what says it is the right one.
	//
	// The (ServerMessage*)this form is copied from the proven CMsgSpaceView
	// in serverlightanim.cpp rather than padding this view class out.
	int GetSpaceLeft()                       // 0x1000e845
	{
		ServerMessage* pMsg = (ServerMessage*)this;
		int            nLeft;

		nLeft = pMsg->m_MaxDataLen - pMsg->m_Unknown38 - 7;

		if (nLeft < 0)
			nLeft = 0;

		return nLeft;
	}
};

// A receive record's last field, one dword past where servermgr.h's
// CNetRecvPacket stops: SendPacketData sets it to 1 on a record it queued
// but did not send, and the send-queue walk clears it once the record has
// gone out (name ours).
class CNetRecvPacketSendView : public CNetRecvPacket
{
public:
	uint32 m_bPending;                       // 0x1c
};

// CBaseDriver's slot 0x60 is the raw send: servermgr.h has it as `_p60` with
// no arguments because nothing had reached it. It takes the payload, its
// length, the spare capacity behind it and the connection, and returns
// whatever the driver made of it — which is SendPacketData's own return value
// (name ours).
class CBaseDriverSendView
{
public:
	virtual void _d00()=0; virtual void _d01()=0; virtual void _d02()=0; virtual void _d03()=0;
	virtual void _d04()=0; virtual void _d05()=0; virtual void _d06()=0; virtual void _d07()=0;
	virtual void _d08()=0; virtual void _d09()=0; virtual void _d10()=0; virtual void _d11()=0;
	virtual void _d12()=0; virtual void _d13()=0; virtual void _d14()=0; virtual void _d15()=0;
	virtual void _d16()=0; virtual void _d17()=0; virtual void _d18()=0; virtual void _d19()=0;
	virtual void _d20()=0; virtual void _d21()=0; virtual void _d22()=0; virtual void _d23()=0;
	virtual LTBOOL SendData(void* pData, uint32 nLen, uint32 nSpare,
	                        NetConnection* pConn)=0;                        // 0x60
};

// 0x100211e2 — a free function of one message: nonzero means the message can
// take more data as it stands, so the send path does not have to swap in a
// fresh one. Cdecl, one argument (name ours, from what the two call sites
// branch on).
int net_CanAppend(class ServerMessage* pMsg);

// The three-word scratch SendPacketData keeps a guaranteed packet's length
// and write position in while it appends the sequence number, so it can put
// them back afterwards. The restore appears at every one of the function's
// exits, which is what says it is a destructor and not a statement — and the
// three slots are contiguous (-0x18, -0x14, -0x10 of a 0x18-byte frame) in
// exactly this order, which is what says they are one object (names ours).
class CMsgLenSave
{
public:
	CMsgLenSave() { m_nLen = 0; m_nPos = 0; m_pMsg = LTNULL; }

	~CMsgLenSave()
	{
		if (m_pMsg)
		{
			m_pMsg->m_Unknown34 = (uint16)m_nLen;
			m_pMsg->m_Unknown38 = (uint16)m_nPos;
		}
	}

	class ServerMessage* m_pMsg;             // +0x00
	uint32               m_nLen;             // +0x04
	uint32               m_nPos;             // +0x08
};

// 0x10053364 — the ack timer past which an ack is piggybacked onto whatever
// is going out anyway; a plain initialised global (0.125f), with no console
// binding of its own, sitting immediately before g_NetAckTime (name ours).
// Defined here because both are this TU's own .data: they open it, directly
// ahead of the "Conn %p closing" literal NetConnection::Term prints.
float g_NetPiggybackTime = 0.125f;

// 0x10053368 — servermgr.h's g_NetAckTime, one third of a second
// (0x3eaaaaab).
float g_NetAckTime = 1.0f / 3.0f;

// CBaseMgr's slot 4 is the new-connection notification: the handler is given
// the connection and one bit off its flags, and answers whether it took it.
// servermgr.h types the slot as two `void*` because nothing had pinned the
// arguments down; this view gives it the shape the call site shows, with a
// placeholder in slot 0 for the destructor.
class CBaseMgrConnectView
{
public:
	virtual void   _v00();
	virtual LTBOOL OnNewConnection(NetConnection* pConn, uint32 nFlags);
};

// And the queues' destructors are not empty: each stores its vptr and then
// clears m_pTail and m_nMessages, in that order -- the same order
// CNetMgr::TermQueue and 0x1002425d empty a list in. They are the members'
// own destructors, run after the body (Primal Hunt server.dll 0x1002ab32..
// 0x1002ab63, in reverse declaration order), with their out-of-line copies
// at 0x1002daa3 (~LMessageList) and 0x1002db44 (~CNetSentQueue) that the
// exception unwinds jump to. servermgr.h gives CNetSentQueue its
// destructor; LMessageList's belongs in lthread.h
// (`~LMessageList() { m_pTail = LTNULL; m_nMessages = 0; }`), which is what
// also gives NetConnection's constructor (0x1002aa0b) its four EH states.

// g_NetDebugLevel is 0x100584ec, and the console name it is registered
// under is "TransportDebug" — the variable table at 0x10051f94 pairs that
// literal with this address. It doubles as the switch on the closing
// statistics here and as CNetMgr::DebugPrint's threshold.


// 0x1002126a (PH server.dll 0x1002aa95, lithtech.exe 0x004679f0) — the
// connection's destructor: tear its four queues down. Three are message lists the manager knows how to drain; the fourth,
// the awaiting-ack list, holds bank records instead and goes back to the
// manager's packet bank.
//
// The manager is re-read off the driver at every one of the four calls —
// `mov 0x30(%esi),%eax; mov 0x48(%eax),%ecx` four times over — so there is no
// local for it. Each queue's *address*, on the other hand, is used twice (the
// drain and the clear) and gets exactly one `lea` each, which is what says
// those four are locals.
//
// The resend ratio is a **float** divide of two `uint32` counters, and that
// is the one thing here that took probing rather than reading. Both operands
// go through the unsigned-to-64-bit store pair (low dword, then a zero high
// dword) into one shared 8-byte frame temp, but only the numerator gets a
// `fildll`: the divide is `fidivl` on the divisor's *low dword*, which
// leaves the zero it just wrote to the high dword dead. Written as a
// `double` divide instead, VC6 emits a second `fildll` and an `fdivrp` and
// the function is two bytes long; written with either operand signed it
// drops the dead store and comes out three bytes short. None of the four
// compiler revisions moves any of that, so the float is source, not
// scheduling — a new fingerprint: **a dead zero store into the high half of
// an x87 conversion temp, followed by `fidivl` on the low half, is a float
// divide by an unsigned**.
NetConnection::~NetConnection()
{
	LMessageList*  pGuaranteed;
	LMessageList*  pSend;
	LMessageList*  pRecv;
	CNetSentQueue* pSent;

	pGuaranteed = &m_GuaranteedQueue;
	m_pDriver->m_pNetMgr->TermQueue(pGuaranteed);

	pSend = &m_SendQueue;
	m_pDriver->m_pNetMgr->TermQueue(pSend);

	pRecv = &m_RecvList;
	m_pDriver->m_pNetMgr->TermQueue(pRecv);

	pSent = &m_SentList;
	netbank_FreeMessages((LMessageList*)pSent, &m_pDriver->m_pNetMgr->m_PacketBank);

	if (g_NetDebugLevel > 0)
	{
		dsi_ServerError("Conn %p closing - total sent: %d, resent: %d, ratio: %.2f",
			this,
			((NetConnectionResetView*)this)->m_nSent,
			((NetConnectionResetView*)this)->m_nResent,
			(float)((NetConnectionResetView*)this)->m_nResent /
			(float)((NetConnectionResetView*)this)->m_nSent);
	}

}


// Nothing has been heard for longer than the timeout, and the connection is
// past the handshake. The message goes out only when the net console
// variable asks for it.
LTBOOL NetConnection::CheckAckTimeout()
{
	if (m_Flags & 1)
		return LTFALSE;

	if (m_Unknown98 > g_AckTimeout)
	{
		if (g_NetShowTimeouts > 0)
			dsi_ServerError("Ack timeout (%.3f)", m_Unknown98);

		return LTTRUE;
	}

	return LTFALSE;
}


// ---- the drivers -------------------------------------------------------- //

// Two driver names are understood, and an unknown one simply produces
// nothing. A driver that fails its own initialization is destroyed rather
// than kept in the list.
CBaseDriver* CNetMgr::AddDriver(char* pDriverName)
{
	CBaseDriver* pDriver = LTNULL;

	if (strcmp(pDriverName, "local") == 0)
	{
		pDriver = ldrv_Create();
	}
	else if (strcmp(pDriverName, "internet") == 0)
	{
		pDriver = tcp_Create();
	}

	if (pDriver)
	{
		pDriver->m_pNetMgr = this;

		if (!pDriver->Init())
		{
			if (pDriver)
				delete pDriver;

			return LTNULL;
		}

		CDriverList* pList = &m_Drivers;

		pList->Insert2(pList->m_nElements, pDriver, &g_DefAlloc);
	}

	return pDriver;
}

// The driver is dropped from the list whether or not it was in it: a -1
// index is what the array takes to mean "nothing to remove".
//
// The search is dynarray.h's CMoArray::FindElement (dynarray.h:509), which
// collection.h declares and never defines; it is defined HERE rather than
// there because every `template class CMoArray<...>` explicit instantiation
// in the tree would then instantiate it too, and for struct element types
// with no operator== that does not compile. It is the lever for lithtech.exe
// 0x004561e0: written as an open loop, VC6's /O2 inline budget had room
// left to expand BaseNew inside Remove2 (`call *0x4(%edx)`); the FindElement
// expansion spends that room and BaseNew stays the 3-argument cdecl call
// (0x004131d0) the shipped function makes. That took the row from 321
// bytes / 11 blocks to size-exact 315 / 2 blocks. The residual is inside
// Remove2's second copy loop (`pNewArray[i] = m_pArray[i+1]`): shipped
// loads m_pArray into a spare register (ebp) and addresses through it, ours
// folds it into edi -- the same template body compiles to both forms in
// this very object (GenRemoveAt has ours, AddConnection's expansion has
// shipped's), so it is register allocation, not source. server.dll's
// 0x10021bbc is unchanged by the rewrite.
template<class T, class C>
uint32 CMoArray<T, C>::FindElement(const T x) const
{
	uint32 i, ret = BAD_INDEX;

	for (i = 0; i < m_nElements; i++)
	{
		if (m_pArray[i] == x)
		{
			ret = i;
			break;
		}
	}

	return ret;
}

void CNetMgr::RemoveDriver(CBaseDriver* pDriver)
{
	uint32 iFound = m_Drivers.FindElement(pDriver);

	if (pDriver == m_pMainDriver)
		m_pMainDriver = LTNULL;

	if (pDriver)
		delete pDriver;

	m_Drivers.Remove2(iFound, &g_DefAlloc);
}


// ---- connections and their queues --------------------------------------- //

// The notice goes out before the handler is told, and only a connection that
// was actually in the list is announced.
//
// The handler is told which connection died: the notification slot takes it
// as an argument, which is what the shipped call pushes.
//
// The search is CMoArray::FindElement, as in RemoveDriver above. An open
// loop gives the same /O1 bytes (0x10022bc8) but leaves lithtech.exe's /O2
// budget room to expand BaseNew inside Remove2 (`call *0x4(%edx)`, 338
// bytes); shipped 0x00458030 CALLS BaseNew (0x004131d0), 321 bytes, which
// the FindElement expansion reproduces exactly.
void CNetMgr::CloseConnection(NetConnection* pConn)
{
	uint32 iFound = m_Connections.FindElement(pConn);

	con_Printf("DisconnectNotify\n");

	if (iFound != BAD_INDEX)
	{
		if (m_pHandler)
			m_pHandler->_v08(pConn);

		m_Connections.Remove2(iFound, &g_DefAlloc);
	}
}

// Everything still queued goes back to the allocator, payloads first. The
// manager's outstanding-bytes running total is wound back as it goes.
void CNetMgr::TermQueue(LMessageList* pQueue)
{
	CNetRecvPacket* pCur = (CNetRecvPacket*)pQueue->m_pTail;
	CNetRecvPacket* pRec;

	while (pCur)
	{
		pRec = pCur;
		pCur = (pCur->m_pNext == (CNetRecvPacket*)pQueue->m_pTail) ?
		           LTNULL : pCur->m_pNext;

		if (pRec->m_pData)
		{
			::operator delete(pRec->m_pData);
			m_UnknownD8 -= pRec->m_nAlloced;
		}

		::operator delete(pRec);
	}

	pQueue->m_pTail = LTNULL;
	pQueue->m_nMessages = 0;
}


// The app guid the drivers advertise; a null argument clears it.
void CNetMgr::SetGuid(LTGUID* pGuid)
{
	if (pGuid)
		memcpy(&m_Guid, pGuid, sizeof(m_Guid));
	else
		memset(&m_Guid, 0, sizeof(m_Guid));
}

// 0x10005fab — the counter's tail-jump forwarder to Read (defined in
// counter.cpp, which is where the whole class lives). DebugPrint reaches it
// rather than Read itself: the shipped call sets ecx to the counter and
// pushes nothing, and the target is five bytes of `jmp`. Named by its
// address; the forward is a masked relocation either way, so only the
// shipped disassembly separates the two.
class CBaseCounterReadView : public CBaseCounter
{
public:
	uint32 sub_10005fab();
};

// The "(%d)" in the trace line is the net counter's elapsed time in
// server.dll only. Both lithtech.exe copies (0x459bb0 CNetMgr::DebugPrint,
// 0x459b50 the connectionless overload) push a literal `$0x0` there -- no
// counter call, no counter reference at all -- so the executable's build
// passes a constant zero. The guard is the stand-in for however the original
// configured that out.
// AVP2: lithtech.exe reads the counter as server.dll does (PH 0x0046b0c9,
// `mov $0x4f5770,%ecx; call 0x004260f0`, the forwarder's jmp).
#define NET_TRACE_TIME() (((CBaseCounterReadView*)&g_NetCounter)->sub_10005fab())


// 0x100263e7 — age the window by one frame. A tracker with no window length
// set never rolls over; one that has runs its rollover as soon as the window
// is past.
void CRateTracker::Update(float fDelta)
{
	m_Time += fDelta;

	if (m_Unknown08 != 0.0f && m_Time > m_Unknown08)
		Rollover();
}


// 0x10026413 — the window closes and reopens a tenth as wide: the rate over
// the window that just ended is scaled into the new one, so a tracker that
// has been counting for two seconds carries a proportional head start rather
// than restarting at zero. Both the new window and the scale are the same
// 0.1f, loaded once and used twice — which is why the multiply takes its
// operand from the stack register rather than reloading the field that was
// just written.
void CRateTracker::Rollover()
{
	float fRate = m_Value / m_Time;
	float fWindow = 0.1f;

	m_Time  = fWindow;
	m_Value = fRate * fWindow;
}


// 0x10025fc6 — the packet bank is reference counted like the rest of the
// system layer, but nothing here builds it: the counter goes up and that is
// the whole body. Whatever allocates the bank does so on first use.
void netbank_Init()
{
	g_nNetBankRefs++;
}


// 0x10025fcd — the counterpart, and the only one of the two that does any
// work. The walk is circular: each step reads the record out of the link
// before advancing, and coming back to the head ends it -- which the shipped
// build spells branchlessly, `sub` then `neg` then `sbb` then `and`, turning
// "did we arrive back at the head" into a mask over the pointer rather than a
// jump. That is VC6's own doing from an ordinary comparison.
//
// The record is deleted through its own vtable and the link is not unlinked
// first, so the list is left pointing at freed memory until the three dwords
// below are cleared.
void netbank_Term()
{
	if (g_nNetBankRefs <= 0)
		return;

	if (--g_nNetBankRefs != 0)
		return;

	LTLink* pLink = g_NetBankList.m_pHead;

	while (pLink)
	{
		void* pItem = pLink->m_pData;

		pLink = pLink->m_pNext;

		pLink = (pLink == g_NetBankList.m_pHead) ? LTNULL : pLink;

		if (pItem)
			delete (INetBankItem*)pItem;
	}

	g_NetBankList.m_pHead  = LTNULL;
	g_NetBankList.m_nCount = 0;
	g_nUnknown79c = 0;
}


// 0x10023569 — the four pieces a pending record owns are released back to
// front, each through its own vtable. A piece that was never filled in is
// skipped (name ours).
//
// `inline`, and the proof is both-sided rather than a size guess. The shipped
// /O2 client EXPANDS this body into ReleaseQueued: lithtech.exe 0x00458d50 is
// byte-for-byte our ReleaseQueued for its first 0x44 bytes and then carries
// this loop verbatim -- `lea 0x18(%edi),%esi; mov $0x4,%ebx;
// mov -0x4(%esi),%ecx; sub $0x4,%esi; test %ecx,%ecx; je; mov (%ecx),%edx;
// call *0x8(%edx); dec %ebx; jne` -- with net_Free after it, so Term was an
// inline candidate in the true source. The shipped /O1 server CALLS it: the
// only `e8` in server.dll reaching 0x10023569 is at 0x10023559, inside
// ReleaseQueued's own 39 bytes, so /Os declined the expansion at that site
// WITH THAT SAME SOURCE. Our body is byte-identical at /O1 (0x10023569, 29 of
// 29), so the keyword cannot move server.dll: not this COMDAT, which the
// declined call site keeps alive, and not ReleaseQueued, which keeps its
// call. It is the 31-byte size that argues against marking it and the two
// shipped images that settle it.
inline void CNetPending::Term()
{
	CNetPiece** pp = &m_pPieces[4];
	int         i;

	for (i = 4; i; i--)
	{
		CNetPiece* pPiece = *(--pp);

		if (pPiece)
			pPiece->Release();
	}
}


// 0x100235a3 — everything still pending for a connection goes away with it.
// A record is also dropped when its own connection has already moved past
// the packet it was waiting on: it will never be answered.
void CNetMgr::RemoveConnection(NetConnection* pConn)
{
	CNetPending* pCur = (CNetPending*)m_QueueC4.m_pTail;
	CNetPending* pRec;

	while (pCur)
	{
		pRec = pCur;
		pCur = ((LThreadMessage*)pCur->m_pPrev == m_QueueC4.m_pTail) ?
		           LTNULL : (CNetPending*)pCur->m_pPrev;

		if (pRec->m_pConnection)
		{
			if (pRec->m_pConnection == pConn ||
			    pRec->m_PacketID < pRec->m_pConnection->m_PacketID)
			{
				ReleaseQueued(pRec);
			}
		}
	}
}


// ---- sending ------------------------------------------------------------- //
//
// Three things this side of the manager needs that servermgr.h does not
// model, all declared as local views and reached by casting, the way
// smgr.cpp reaches IObjectDeactivate.

// The record CNetBank hands out is 0x70 bytes, and the 0x64 after its three
// header dwords are a whole ServerMessage: 0x0c + 0x64 = 0x70 exactly, and
// every offset the send path touches lines up — the record's 0x40, 0x44 and
// 0x60 are the packet's m_Unknown34, m_Unknown38 and m_pData. servermgr.h's
// CNetSentPacket keeps that tail as an opaque byte run (m_Data0C); this is
// the same record with the packet named. What confirms it beyond arithmetic
// is CNetBank::Allocate (0x10024162), which runs ServerMessage's constructor
// (0x1002417f) on exactly this+0xc after the struct bank hands the record
// back.
class CNetSentPacketView
{
public:
	uint32              m_Unknown00;         // 0x00
	CNetSentPacketView* m_pNext;             // 0x04
	float               m_ResendTimer;       // 0x08
	ServerMessage       m_Packet;            // 0x0c .. 0x70
};

// The bank's allocate, which servermgr.h's CNetBank does not carry (only its
// Free does). Placement-constructs the packet above; a failed struct-bank
// allocation comes back as LTNULL and is *not* checked by its callers.
class CNetBankAllocView : public CNetBank
{
public:
	CNetSentPacketView* Allocate();          // 0x10024162
};

// servermgr.h has the send path's two signatures wrong — SendPacket and
// SendPacketData both return LTBOOL, not void (see the note on SendPacket) —
// and does not model the manager's send-byte accumulator or the member at
// +0xe0 it feeds. All three go on a view derived from CNetMgr: the base has
// no virtual functions, so deriving changes nothing about the layout or
// about `this`, and the redeclared members simply hide the base's. The
// symbols the byte comparison verifies are then this class's rather than CNetMgr's,
// which costs nothing — the verification is per function on machine code.
class CNetMgrSendView : public CNetMgr
{
public:
	LTBOOL AddConnection(NetConnection* pConn);                                 // 0x10022ade
	LTBOOL SendPacket(class ServerMessage* pMsg, NetConnection* pConn, uint8 n); // 0x10022990
	LTBOOL SendPacketData(class ServerMessage* pMsg, NetConnection* pConn);     // 0x10022d6c

	// The send-side counterpart of CNetMgr::AddRecvBytes: the same shape —
	// the packets-per-second tracker gains 1.0f and the bytes-per-second
	// tracker the byte count — plus a per-connection opt-out (the
	// connection's m_Flags bit 0) on a manager-wide float at CNetMgr+0xe0,
	// which is one dword past the last member servermgr.h knows about
	// (names ours).
	void   AddSentBytes(NetConnection* pConn, uint32 nBytes);                   // 0x10022a7b

	// 0x10023dc2 — CNetMgr::DebugPrint without the connection: the same
	// level test against g_NetDebugLevel, the same 500-byte _vsnprintf and
	// the same "(%d) %s%s" line, minus the per-connection filter that needs
	// a connection to read m_Flags off. An overload rather than a second
	// name, which is what the two symbols look like from the call sites.
	void   DebugPrint(int nLevel, char* pFormat, ...);                          // 0x10023dc2

	// Five more of the manager's own workers, all outside the run this file
	// covers and none with anything at the call site that names them, so
	// they keep their addresses. What the call sites do say:
	//   0x10023a8c(pConn, pMsg) — run after a packet has been accepted;
	//       nonzero means it consumed the packet and TranslatePacket is done.
	//   0x100235e8(pConn, pMsg) — the same contract, one stage earlier, and
	//       reached on the local path as well as the remote one.
	//   0x10022ac2(pConn) — the in-order step: hands the connection's
	//       current packet id to 0x10023a91 against the guaranteed queue and
	//       then increments it.
	//   0x10023c4d(nLen) — a receive record with room for nLen bytes; it
	//       reserves two extra (`inc esi` twice) and comes off the free list
	//       at CNetMgr+0xc0.
	//   0x10023b1f(pList, pRec) — insert a receive record into the
	//       guaranteed queue in id order; LTFALSE means it did not take it.
	int    sub_10023a8c(NetConnection* pConn, class ServerMessage* pMsg);
	int    sub_100235e8(NetConnection* pConn, class ServerMessage* pMsg);

	// The five more the internal-packet handler reaches, none of them named
	// by anything at the call sites. What the bytes say:
	//   0x10021c55(pMsg, pConn, 0) — send one message to one connection;
	//       three arguments and `ret 0xc`.
	//   0x10023ad7(pList, nID) — the twin of 0x10023a91 with the comparison
	//       the other way round (`jbe` where that one has `ja`) and the loop
	//       rotated: it releases everything queued *above* the id instead
	//       of at-or-below it.
	//   0x100233ad(nID, pConn) — find the fragment record collecting a
	//       given packet id for a connection, LTNULL if there is none.
	//   0x10023495() — a fresh fragment record.
	//   0x100233e3(pMsg, nIndex, pRec) — put one fragment into a record at
	//       its index. The index arrives as a byte (the call site pushes the
	//       whole dword its uint8 local sits in).
	//   0x10023474(pRec) — nonzero once every fragment has arrived.
	// 0x10021c55 — smgr.cpp reaches this one too, through its own view of the
	// same class, so the signature here is load-bearing across TUs.
	LTBOOL SendToConnection(class ServerMessage* pMsg, NetConnection* pConn,
	                        int nFlags);
	void   sub_1002294f(NetConnection* pConn);
	void   sub_10023ad7(LMessageList* pList, uint32 nID);
	class CNetFragRec* sub_100233ad(uint32 nID, NetConnection* pConn);
	class CNetFragRec* sub_10023495();
	LTBOOL sub_100233e3(class ServerMessage* pMsg, uint8 nIndex,
	                    class CNetFragRec* pRec);
	int    sub_10023474(class CNetFragRec* pRec);
	void   sub_10022ac2(NetConnection* pConn);

	// 0x10023a91 — release every record queued out of order whose id has
	// now been overtaken: walk the list from its tail, and for each record
	// with an id at or below the one handed in, take it off the list
	// (0x10015b00, reached with the list as `this`) and hand it back
	// (0x10023bb5, reached with the manager as `this`). It stops at the
	// first record with a higher id, because the queue is kept in id
	// order. `ret 8` and the manager spilled to a frame slot say member,
	// two arguments (name ours).
	void   sub_10023a91(LMessageList* pList, uint32 nID);
	CNetRecvPacket* sub_10023c4d(int nLen);
	LTBOOL sub_10023b1f(LMessageList* pList, CNetRecvPacket* pRec);

	// Four more, on the send side. Again nothing at the call sites names
	// them:
	//   0x10023d16(&ref) — fills a message reference with a fresh message
	//       to spill into.
	//   0x10023d5b(pDst, pSrc) — hands one message's contents to another.
	//   0x10023d73(pMsg, pData, nLen) — appends raw bytes to a message.
	//   0x10022c2f(pData, nLen, nSpare, pConn) — the local-connection send,
	//       taking exactly what the driver's slot 0x60 takes and returning
	//       the same thing.
	void   sub_10023d16(CMessageRef* pRef);
	void   sub_10023d5b(class ServerMessage* pDst, class ServerMessage* pSrc);
	void   sub_10023d73(class ServerMessage* pMsg, void* pData, uint32 nLen);
	LTBOOL sub_10022c2f(void* pData, uint32 nLen, uint32 nSpare, NetConnection* pConn);
};

// 0x10024162 — one sent-packet record out of the bank, constructed in
// place. The bank hands back raw memory, so the packet at +0x0c gets its
// constructor run explicitly; a failed allocation comes back as LTNULL
// without the constructor. The same shape as LThreadMessageBank::Allocate
// (0x10015b2e), down to the `add $4,%ecx` that reaches the embedded struct
// bank and the `pop %ecx` that cleans up sb_Allocate's one argument.
//
// `inline` in lithtech.exe only. That image has no standalone Allocate:
// SendPacket (0x00457ab0) carries it expanded, sb_Allocate's pop-and-grow
// included (`call 0x00490c20` sb_Grow, then the ServerMessage constructor
// call 0x00457c40), and CNetBank's slot 1 (0x00459c30, startload.cpp)
// carries another copy. server.dll keeps the out-of-line 0x10024162 and
// the `call` to it at 0x10022990, which /Os would not keep if this were
// inline there.
//
// PH: Primal Hunt's lithtech.exe calls it out of line (0x00468fd0 ->
// 0x0046b5c0, with an exception frame for the packet's constructor), where
// AvP2 1.0.9.6's expands it into SendPacket (0x00463cb5..0x00463cd5).
// That out-of-line copy is not this TU's: it sits with the bank's slots
// (CNetBank's slot 1, 0x0046b200, is a `jmp` onto it) and startload.cpp
// writes it, ServerMessage's constructor expanded, as CNetBankQ11::Allocate
// (near, 312 bytes to 312).  So Primal Hunt's lithtech.exe build only
// declares it here.
#if defined(LT_SERVER_BUILD) || !defined(PRIMAL_HUNT)
#if !defined(LT_SERVER_BUILD)
inline
#endif
CNetSentPacketView* CNetBankAllocView::Allocate()
{
	CNetSentPacketView* pRec = (CNetSentPacketView*)sb_Allocate(&m_Bank);

	if (pRec)
		new (pRec) CNetSentPacketView;

	return pRec;
}
#endif


// 0x10022ac2 — the in-order step, run when the packet just received is the
// one the connection was expecting. Everything still queued at or below the
// current id is released, and the id then advances.
//
// `this` is never loaded for the inner call: nothing between the entry and
// the call touches ecx, so VC6 leaves the incoming `this` in place. The
// null test on the connection is the function's own — its caller has
// already dereferenced pConn several times.
void CNetMgrSendView::sub_10022ac2(NetConnection* pConn)
{
	if (!pConn)
		return;

	sub_10023a91(&pConn->m_GuaranteedQueue, pConn->m_PacketID);

	pConn->m_PacketID++;
}


// The record the fragment reassembler collects a split packet in (names
// ours, from the offsets the handler touches): four fragment messages, the
// packet id they belong to and the connection they came from. Four, not a
// count — the replay loop is a literal `i = 4`.
// One of the four fragment slots. It is a counted reference, not a bare
// pointer: the reassembler fills a slot through CMessageRef::Init
// (0x1000ebab) and clears one through CMsgCursor::Init (0x10023e86), which
// is two names for the same four-byte record. Declared here with both entry
// points and no destructor — nothing in the image destroys a slot, and a
// destructor VC6 emitted would have no shipped counterpart (names ours).
class CNetFragSlot
{
public:
	CNetFragSlot() { m_pMsg = LTNULL; }

	// AVP2: the slots are destroyed with the record, CMessageRef's own
	// release: ~CNetFragRec is `__ehvec_dtor(this+8, 4, 4, ...)` (server.dll
	// 0x1002d144, lithtech.exe 0x0046a260), its element destructor the
	// tail-jump to Release (server.dll 0x10012b8b), and the constructor
	// builds the four through `__ehvec_ctor` with both (0x1002d0b5).
	~CNetFragSlot() { if (m_pMsg) m_pMsg->Release(); }

	// The clear is servermsg.h's CMsgCursor::Init (0x10023e86), reached
	// through a cast in CNetFragRec::Clear.
	class ServerMessage* Init(const class CMessageRef& other);   // 0x1000ebab

#ifndef LT_SERVER_BUILD
	// STAND-IN (lithtech.exe only): Set's body, inline, for CNetFragRec::Clear
	// -- see the note above Clear.
	ServerMessage* SetInline(ServerMessage* pMsg)
	{
		if (m_pMsg)
			m_pMsg->Release();

		m_pMsg = pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();

		return m_pMsg;
	}
#endif

	class ServerMessage* m_pMsg;             // 0x00
};

class CNetFragRec
{
public:
	CNetFragRec();                           // 0x10023505
	void Clear();                            // 0x1002351c

	// The same two link words CNetPending opens with — the record IS a
	// CNetPending; the manager's second queue holds nothing else.
	class LThreadMessage* m_pNext;           // 0x00
	class LThreadMessage* m_pPrev;           // 0x04
	class CNetFragSlot    m_pFrags[4];       // 0x08
	uint32                m_ID;              // 0x18
	NetConnection*        m_pConn;           // 0x1c
};

// The three CPacket member templates servermsg.h does not name on
// ServerMessage. 0x100138d6, 0x1000ebd0 and 0x100240f8 are
// CPacket::ReadType<uint8/uint16/uint32> (packet.cpp matches all three) and
// 0x1001393e, 0x1000ec3d and 0x1000ecd6 are the WriteType trio; the dummy
// pointer that carries the template argument is what every call site pushes
// as a literal zero. Declared here as ordinary members so this file can emit
// the calls without pulling packet.h in (names ours).
class ServerMessageTypeView
{
public:
	uint16 ReadWord(uint16* pDummy);         // 0x1000ebd0
	uint32 ReadDWord(uint32* pDummy);        // 0x100240f8
	// 0x100260d9 again, with a *uint16* length. servermsg.h types it uint32
	// because that is what LMessageImpl::ReadRawFL hands it, and a uint32
	// variable pushes the same dword either way; the sub-type-3 call site
	// here is the one that separates them — it pushes the raw dword its
	// uint16 local sits in without zero-extending, which a uint32
	// parameter would have forced.
	void   ReadData(void* pData, uint16 nBytes);
	// 0x1002616a with the same narrowing, and the same evidence: the
	// fragment replay computes its length with a 16-bit `dec %cx` and
	// pushes the dword it sits in.
	void   WriteData(void* pData, uint16 nBytes);
	// STAND-IN (name ours): WriteData through one inline level. See
	// sub_10023d73 for why the level is measured.
	void   WriteDataInline(void* pData, uint16 nBytes) { WriteData(pData, nBytes); }
	// STAND-IN (name ours): ReadData through the same one inline level, for
	// net_LogPacket's sub-packet copies, whose destination is loaded into a
	// register and pushed (server.dll 0x1002b773 `mov 0x54(%eax),%eax ; push
	// %eax`) where a direct call pushes the field.
	void   ReadDataInline(void* pData, uint16 nBytes) { ReadData(pData, nBytes); }
	void   WriteByte(uint8 nValue);          // 0x1001393e
	void   WriteWord(uint16 nValue);         // 0x1000ec3d
	void   WriteDWord(uint32 nValue);        // 0x1000ecd6
};


// 0x10022a7b — the send-side counterpart of CNetMgr::AddRecvBytes, with one
// extra term: the manager's own rate tracker at +0xe0 gains the byte count
// too, but only for a remote connection (the connection's m_Flags bit 0 is
// the local flag).
//
// The byte count is converted once and used twice — `fildll` of the dead
// dword pair the frame reserves, then `fld %st(0)` to duplicate it — which
// is what says the two `+= nBytes` are two expressions over one conversion
// and not two conversions. `this` survives in ecx across the whole function
// because the byte count is loaded into edx rather than ecx: the two
// `push %ecx` in the prologue are the /O1 stack reservation for the
// conversion's eight bytes, not arguments.
//
// The packets-per-second add goes through a named pointer to its tracker.
// Written as a bare `pConn->m_SendPPS.m_Value += 1.0f`, VC6 hoists the
// `testb $0x1,0x70(%eax)` above that store on both images. The shipped code
// has it after the `fildll` (0x10022aa0; /O2 0x457deb, where ours also
// loaded the flags into %dl). Through the pointer, VC6 cannot move the test
// above the store, and both images come out byte-identical. An inline
// tracker-add helper used on the PPS line gives the same result. Earlier
// negatives, none of which moved the test: an early `return`; an if/else with
// the bytes add in both arms (78 bytes); a `float` local for the conversion;
// all four compiler revisions; `(flags&1)==0`; `PPS = PPS + 1.0f`; the flag
// read through a byte cast or a view class.
void CNetMgrSendView::AddSentBytes(NetConnection* pConn, uint32 nBytes)
{
	if (!pConn)
		return;

	CRateTracker* pPPS = &pConn->m_SendPPS;
	pPPS->m_Value += 1.0f;
	pConn->m_SendBPS.m_Value += nBytes;

	if (!(pConn->m_Flags & 1))
		m_RateE0.m_Value += nBytes;
}


// 0x1003e280 is the CRT's memset (lithtech/ccobj.cpp reads its return
// value correctly: the destination), a call in server.dll because /O1
// carries no /Oi, and `rep stos` in lithtech.exe's /O2.

// 0x10051c8c — how many packets the guaranteed queue will hold before the
// insert starts dropping them. A plain initialised global (32), signed, with
// no console binding (name ours).
extern int g_NetMaxQueued;


// 0x1002118b — the two-byte checksum the remote packets carry in their last
// two bytes; cdecl, and both the return and the length are uint16. The
// length being uint16 is what the call site proves: the shipped code loads
// the packet length with a 16-bit `mov 0x34(%edi),%cx`, subtracts two from
// the *whole* of ecx and pushes ecx, so the high half of the pushed dword is
// whatever was in the register before. A uint32 parameter would have forced a
// `movzwl` first. The callee truncates it straight back (`mov %dx,%cx`), so
// nothing is wrong — but it does mean the argument only ever carried sixteen
// meaningful bits. Name ours.
uint16 net_Checksum(uint8* pData, uint16 nLen)
{
	uint16 nSum   = 0;
	uint16 nWords = nLen >> 1;
	uint16 nRem   = nLen - nWords * 2;

	// Every intermediate is sixteen bits wide, and the bytes say so twice
	// over: the sum is accumulated with 32-bit `add` instructions over
	// registers whose upper halves are never cleaned (`mov (%edi),%bx`
	// leaves garbage above bx), and the odd trailing byte is sign-extended
	// to a *word* (`movsbw`) rather than to a dword. A uint32 accumulator
	// would force a `movzwl` at every load and a `movsbl` at the last one.
	//
	// Both loops are the post-decrement form: `i = n; while (i--)` gives the
	// `add $0xffff` / `movzwl` / `inc` trip-count arithmetic and the
	// zero-guard that goes with it, where a `for` collapses to one `movzwl`
	// of the limit. The -1 is materialised into esi once and
	// shared by both.
	while (nWords--)
	{
		nSum += *(uint16*)pData;

		pData += 2;
	}

	while (nRem--)
	{
		nSum += *(char*)pData;

		pData++;
	}

	return nSum;
}

// g_NetDropRate is 0x100584bc, registered under the console name "DropRate"
// (the variable table at 0x10052368 pairs the literal with this address). It
// is a percentage: a packet is dropped when a fresh 0..100 random draw comes
// out under it.
extern float g_NetDropRate;

// g_NetResendTime is 0x100584b8, and the console name it is registered
// under is "LatencySim" — the variable table at 0x10052354 pairs that
// literal with this address. Above a millisecond the send path stops
// sending and starts queueing, which is what the name describes.


// 0x10022ade — take a new connection into the manager's list.
//
// Everything about the connection's timing and sequencing state is reset
// first, then the connection goes on the end of the list and the handler is
// asked whether it wants it; a refusal takes the entry straight back off
// again. The two lists' counts are read differently at the two calls — the
// insert reads it through the list pointer, the remove through `this` — and
// that asymmetry is in the bytes, not a slip.
//
// The list pointer is a local — one `lea` serves both the insert and the
// remove — but it has to be *assigned* just before the insert rather than
// initialised at its declaration: initialised, VC6 hoists the `lea` to the
// top of the function, which rotates ebx and edi and costs nothing in size
// but every byte of the two calls' register operands.
//
// Every one of the fourteen resets reloads `pConn` from its parameter slot,
// because `Insert2` takes its element by const reference: the parameter is
// address-taken, so the compiler has to assume each store could alias it.
// That is also why the three-float run at +0xa0 stays a loop — VC6 strength-reduces the index to
// a byte offset that starts at 0xa0 and stops at 0xac — instead of being
// unrolled into three stores like the rest.
//
// lithtech.exe 0x00457e40 is BYTE-IDENTICAL through the CConnListO2 view
// below. Shipped CALLS Insert2 out of line (0x0045a240, `push $g_DefAlloc;
// push &pConn; push m_nElements`) -- which is also why every reset reloads
// pConn from its slot -- and then expands Remove2 completely, BaseNew and
// the release included; through the real template /O2 expanded both, each
// with BaseNew and BaseDelete left as calls (581 bytes). The view declares
// Insert2 without a body and restates Remove2 with collection.h's own
// helper levels (_AllocateTArray, _DeleteAndDestroyArray, the DefaultCache
// accessors -- without the helpers the dead SetCacheSize(0) store before
// the reseat is eliminated, 474 bytes).
#ifndef LT_SERVER_BUILD
// STAND-IN (lithtech.exe only): CConnList seen through a same-layout view
// whose Insert2 is DECLARED ONLY (so it stays a call) and whose Remove2 is
// an in-class copy of collection.h's, helper for helper.  Shipped
// 0x00457e40 CALLS Insert2 out of line (0x00457ef0 -> 0x0045a240) and
// expands Remove2 down to the allocator's own virtuals (0x00457f4c
// `call *0x4(%edx)`, 0x00457fc5 `call *0x8(%edx)`); through the real
// template /O2 does the opposite at depth 3.  server.dll keeps the template.
class CConnListO2
{
public:
	LTBOOL Insert2(uint32 index, NetConnection* const& toInsert, LAlloc* pAlloc);

	inline uint32 GetNumAllocatedElements() {return m_nElements + m_Cache.GetCacheSize();}

	NetConnection** _AllocateTArray(uint32 nElements, LAlloc* pAlloc)
	{
		NetConnection** tPtr = LNew_Array(pAlloc, NetConnection*, nElements);

		return tPtr;
	}

	void _DeleteAndDestroyArray(LAlloc* pAlloc, uint32 nElements)
	{
		if (m_pArray)
		{
			LDelete_Array(pAlloc, m_pArray, nElements);
			m_pArray = 0;
		}

		m_Cache.SetCacheSize(0);
	}

	void Remove2(uint32 index, LAlloc* pAlloc)
	{
		uint32          i, newSize, newAllocSize;
		NetConnection** pNewArray;
		LTBOOL          bSlideDown;

		bSlideDown = LTTRUE;
		if (m_Cache.GetCacheSize() >= (m_Cache.GetWantedCache() * 2))
		{
			newSize = m_nElements - 1;
			newAllocSize = newSize + m_Cache.GetWantedCache();
			pNewArray = _AllocateTArray(newAllocSize, pAlloc);

			if (pNewArray || newAllocSize == 0)
			{
				for (i = 0; i < index; i++)
					pNewArray[i] = m_pArray[i];

				for (i = index; i < m_nElements - 1; i++)
					pNewArray[i] = m_pArray[i + 1];

				_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
				m_pArray = pNewArray;

				m_Cache.SetCacheSize(m_Cache.GetWantedCache());
				bSlideDown = LTFALSE;
			}
		}

		if (bSlideDown)
		{
			m_Cache.SetCacheSize(m_Cache.GetCacheSize() + 1);

			for (i = index; i < m_nElements - 1; i++)
				m_pArray[i] = m_pArray[i + 1];
		}

		--m_nElements;
	}

	void*           m_pVtbl;                 // +0x00
	NetConnection** m_pArray;                // +0x04
	uint32          m_nElements;             // +0x08
	DefaultCache    m_Cache;                 // +0x0c
};
#endif

LTBOOL CNetMgrSendView::AddConnection(NetConnection* pConn)
{
	CConnList* pList;
	uint32     i;

	con_Printf("NewConnectionNotify\n");

	if (!m_pHandler)
		return LTFALSE;

	((NetConnectionResetView*)pConn)->m_Unknown80 = 0;
	((NetConnectionResetView*)pConn)->m_Unknown7C = 0;
	((NetConnectionResetView*)pConn)->m_nSent = 0;
	((NetConnectionResetView*)pConn)->m_SendWait = 0.0f;
	((NetConnectionResetView*)pConn)->m_RecvWait = 0.0f;
	((NetConnectionResetView*)pConn)->m_Unknown6C = 0;
	((NetConnectionResetView*)pConn)->m_UnknownB4 = 0.0f;
	((NetConnectionResetView*)pConn)->m_Ping = 0.0f;

	for (i = 0; i < 3; i++)
		((NetConnectionResetView*)pConn)->m_UnknownA0[i] = 0.0f;

	((NetConnectionResetView*)pConn)->m_Unknown98 = 0.0f;
	((NetConnectionResetView*)pConn)->m_AckTimer = 0.0f;
	((NetConnectionResetView*)pConn)->m_nResent = 0;

	pList = &m_Connections;
#ifdef LT_SERVER_BUILD
	pList->Insert2(pList->m_nElements, pConn, &g_DefAlloc);
#else
	((CConnListO2*)pList)->Insert2(pList->m_nElements, pConn, &g_DefAlloc);
#endif

	if (((CBaseMgrConnectView*)m_pHandler)->OnNewConnection(pConn,
			((NetConnectionResetView*)pConn)->m_Flags & 1))
		return LTTRUE;

#ifdef LT_SERVER_BUILD
	pList->Remove2(m_Connections.m_nElements - 1, &g_DefAlloc);
#else
	((CConnListO2*)pList)->Remove2(m_Connections.m_nElements - 1, &g_DefAlloc);
#endif

	return LTFALSE;
}


// 0x100219db — build the acknowledgement/NAK packet for one connection.
//
// The message is rewound to a single byte, its type byte is poked in
// directly (0x41), and then everything the far end needs is appended: a ping
// request when the connection's countdown at +0xb4 has gone negative, the
// last packet id received (or -1 if none has been), the sent count, and up to
// eight NAKs.
//
// The NAKs come from two places, which is why the byte cap is tested in three
// of them. The guaranteed queue holds the packets that arrived out of order,
// in id order, so every gap between the id expected next and the id of the
// record in hand is a missing frame; then, past the end of the queue,
// everything from there up to +0x80 is missing too. `nCount` caps the whole
// lot at eight, and the shipped code re-tests it once more between the two
// loops on a path where it has just been tested — the redundant compare at
// 0x10021af5 is the second loop's own entry guard, unreachable-but-emitted
// because the queue-empty path jumps straight past it into the loop.
//
// Both parameter slots are reused: the connection's holds the queue cursor
// and the message's holds first the `rand()` result and then `nCount`, since
// both parameters are cached in registers on the first line. `this` goes to
// a real frame slot because the varargs DebugPrint needs it on the stack and
// every other call here clobbers ecx.
//
// The ping timer is reset to `((float)rand() / 32767.0f) * 3.0f`, and the
// parentheses are load-bearing: written flat as `rand() / 32767.0f * 3.0f`,
// VC6 turns the division into a reciprocal multiply and then folds it with
// the 3.0f into one constant, and the function comes out six bytes short with
// a single `fmuls`. Parenthesised, the two `fmuls` the shipped code has
// survive — a division does not reassociate out of its own parentheses.
//
// Byte-identical. The header byte is written through `uint8** ppData =
// &pMsg->m_pData`. With the field's address taken, VC6 can no longer move
// the m_pData load (and the `push %edi` that goes with it) above the two
// `movw $1` stores, and the shipped order at 0x100219e8-f7 follows. Spelled
// `pMsg->m_pData[0]`, the load moves up. Earlier attempts that left it
// there: a chained store, `*pMsg->m_pData`, a scoped `uint8*` local, a
// CPacket view, a static inline helper, a copied pMsg pointer, and cl
// 8168/8966. A `volatile` access fixed the load but let the push float.
//
// Two things did move, and both are worth keeping. The initialisation order
// of `nID` and `nCount` decides whether VC6 materialises a shared zero
// register: with `nCount = 0` written *first* it keeps a zero in edi for the
// whole prologue, spends it on the two `WriteByte(0)` pushes and the
// packet-id test, and the function is a byte short; with `nID` first the zero
// never appears and every use is an immediate, which is what ships. And the
// second NAK loop needs its `if (nCount >= 8) break;` written out even though
// the `if (nCount < 8)` above it says the same thing — without it VC6
// shares the entry test with the back edge and drops six bytes.
//
// lithtech.exe 0x00455c50 is byte-identical too. Two source facts close it.
// (1) The last three writes (the sent count and the two NAK writes) are
// packet.h's CPacket::WriteType<uint32> spelled directly. /O2 expands those
// three with Insert2 left as a call (0x00455d3b, 0x00455e2a, 0x00455f25). It
// CALLS the first five, through the ServerMessage/view members (0x004159f0,
// 0x0042fa50, 0x004303c0). Spelled WriteType, all eight are candidates and
// /O2 spends its budget on the first seven (1512 bytes). server.dll calls
// every one, so both spellings reach the same COMDATs there.
// (2) The second loop is `if (nCount < 8) { while (nID < m_Unknown80) ... }`.
// With the old `while (nCount < 8) { if (nID >= ...) break; ... }`, /O2
// jumped back to the top test. Shipped rotates the loop and re-tests
// m_Unknown80 at the bottom (0x00455fb2), and this spelling does the same.
// server.dll is identical either way.
void CNetMgr::WriteAckPacket(NetConnection* pConn, ServerMessage* pMsg)
{
	CNetRecvPacket* pCur;
	CNetRecvPacket* pRec;
	uint32          nID;
	uint32          nCount;

	pMsg->m_Unknown38 = 1;
	pMsg->m_Unknown34 = 1;
	uint8** ppData = &pMsg->m_pData;
	(*ppData)[0] = 0x41;

	pMsg->WriteByte(0);

	if (((NetConnectionResetView*)pConn)->m_UnknownB4 < 0.0f)
	{
		((NetConnectionResetView*)pConn)->m_UnknownB8++;

		pConn->m_Counter.Start();

		((NetConnectionResetView*)pConn)->m_UnknownB4 =
			((float)rand() / 32767.0f) * 3.0f;

		pMsg->WriteByte(1);
		pMsg->WriteWord(((NetConnectionResetView*)pConn)->m_UnknownB8);
	}
	else
	{
		pMsg->WriteByte(0);
	}

	((ServerMessageWriteView*)pMsg)->WriteDWord(
		pConn->m_PacketID ? pConn->m_PacketID - 1 : (uint32)-1);

	((CPacket*)pMsg)->WriteType(
		(uint32)((NetConnectionResetView*)pConn)->m_nSent);

	pCur = (CNetRecvPacket*)pConn->m_GuaranteedQueue.m_pTail;
	nID = pConn->m_PacketID;
	nCount = 0;

	while (pCur)
	{
		pRec = pCur;
		pCur = (pCur->m_pNext == (CNetRecvPacket*)pConn->m_GuaranteedQueue.m_pTail) ?
		           LTNULL : pCur->m_pNext;

		if (pRec)
		{
			while (nID < pRec->m_ID && nCount < 8)
			{
				((CPacket*)pMsg)->WriteType(nID);

				((CNetMgrSendView*)this)->DebugPrint(1,
					"NAK sent on packet frame %d", nID);

				nID++;
				nCount++;
			}

			nID++;
		}

		if (nCount >= 8)
			break;
	}

	if (nCount < 8)
	{
		while (nID < ((NetConnectionResetView*)pConn)->m_Unknown80)
		{
			((CPacket*)pMsg)->WriteType(nID);

			((CNetMgrSendView*)this)->DebugPrint(1,
				"NAK sent on packet frame %d", nID);

			nCount++;

			if (nCount >= 8)
				break;

			nID++;
		}
	}

	pConn->m_AckTimer = 0.0f;
}


// 0x10022d6c — the send itself. servermgr.h has it as `void`; it is not —
// every exit leaves a value in esi that the shared epilogue moves to eax, so
// it is LTBOOL like SendPacket above.
//
// A guaranteed packet has its length and write position saved off and the
// sequence number appended, and the saved pair is put back on the way out —
// which is why the restore appears at both exits: the two returns hand back
// different values, so there is no shared tail to hang one copy on.
//
// The two message references are scoped (servermsg.h's CMessageRef), so their
// Releases are the destructors and appear at both exits for free.
//
// NEAR (766 of 787 bytes, 273 of 277 instructions). Every block of the
// mnemonic diff is one fault: the shipped build keeps the saved-message
// pointer (`save.m_pMsg`) in ebx across the whole function and reloads `this`
// from its spill slot at each of the six member calls, where ours keeps
// `this` in ebx and leaves the saved pointer in memory. That is worth exactly
// the twenty-one bytes: `mov -0x4(%ebp),%ecx` against `mov %ebx,%ecx` six
// times, `push -0x4(%ebp)` against `push %ebx` twice, and one
// `mov -0x4(%ebp),%eax; testb $0x1,(%eax)` against `testb $0x1,(%ebx)`. It
// also cascades: with ebx taken, the shipped build has to park the queued
// record in the dead `pMsg` parameter slot and reload it after every call,
// and its `push $1; pop %esi` lands in esi where ours lands in eax.
//
// Probed and negative: the three saved values as three plain locals with the
// restore written out at both exits (byte-for-byte the same as the destructor
// form above, which is why the destructor is kept — the contiguous
// -0x18/-0x14/-0x10 run is what argues for it); the queued record merged into
// the loop's own record variable; `save` declared before the two message
// references and after them; the pointer assigned before the two lengths
// rather than after; and cl 8168 and 8966 as well as 8447. All 766.
//
// Three readings worth keeping. The local-connection test is written twice —
// once as the last conjunct of the piggyback guard and once at the head of
// the send choice — and VC6 propagates the first one's failure straight into
// the second, which is why one `testb $0x1,0x70(%edi)` jumps all the way to
// the driver send. `pMsg` is a reassigned parameter: the two spill paths
// replace it with the fresh message they made, and the register that holds it
// is later reused for the return value. And the record's pending flag and the
// `return LTTRUE` beside it share one materialised `1`.
// 0x10022c2f — the local-connection send, and the fragmenter behind it.
// Anything that fits one packet (AVP2: 0x44c bytes, `cmp $0x44d; jae` at
// Primal Hunt server.dll 0x1002c740 and lithtech.exe 0x0046952d, AvP2
// server.dll 0x1002ac9f; NOLF's bound was 0x555) goes straight out through
// the driver's slot 0x60 and this function is a forward. Anything larger is split into at most four
// pieces and each piece sent as its own packet (name ours).
//
// Read off the bytes, and two of these are the reason the function is worth
// having: the ORIGINAL packet id is the last four bytes of the payload, not a
// field -- `mov -0x4(%ebx,%esi,1),%eax` before nLen is reduced by four -- so
// the id trails the data on the wire and the fragmenter has to lift it out
// and re-send it with every piece. And the piece size is `(nLen >> 2) + 1`
// computed ONCE before the loop, which is what makes four pieces enough: the
// clamp to nLen only ever shortens the last one.
//
// Each fragment reuses one message: m_Unknown38 and m_Unknown34 are both
// rewound to 1 (past the header byte m_pData[0] = 1 written before the loop),
// then the sub-id 4, the fragment number ORed with 0x30, the original packet
// id and the slice. The driver gets the message's own buffer with its spare
// computed as m_DataSize - m_Unknown34, exactly as SendPacketData does.
//
// The nEnd == nOffset arm cannot fire on the first pass (the piece size is at
// least 1) and reports through the connectionless DebugPrint, which is what
// says the failure is about the message rather than the peer.
//
// nFrag is UNSIGNED, and both images say so: the /O1 back edge is
// `cmpl $0x4,-0x4(%ebp) ; jb` (0x10022d32) and the /O2 one is
// `cmp $0x4,%eax ; jb` (0x004582a4). An `int` gives `jge`. Size-neutral in
// both images, so it is a correctness fix, not a size one.
//
// Byte-identical on both images. Four source facts decide it, and each was
// read off a specific site:
//
//  * `pOut` and `nRoom` are named locals, assigned just before the driver
//    send, m_DataSize FIRST (0x10022d09 loads 0x58 before 0x54 and 0x34;
//    0x00458285 likewise). Without the named pointer local the /O1 frame
//    has no -0x10 slot for m_pData, VC6 keeps the loop's end offset in ebx
//    instead of the payload base, and the message is homed in nSpare's slot
//    rather than pData's -- the whole thirteen-block residual this function
//    used to carry. With it, the base goes to ebx (0x10022c5d), the message
//    to +0x8 and nEnd to +0x10 exactly as shipped.
//  * A `for` loop, not a do-while: the shipped back edge is `jb` to the head
//    with a `jmp` past it to the exit (0x10022d36/0x10022d3c; 0x004582ab).
//  * The clamp is the ternary `nEnd = (nOffset + nPiece < nLen) ? ... : nLen`
//    (0x00458229: nLen in ebx, the sum compared against it and moved over).
//  * `nOffset = 0` is written above nPiece (the `xor %edi,%edi` at
//    0x10022c92 precedes the stack cleanup) and `m_pData[0] = 1` above
//    bResult, which is what keeps /O2 from reusing bResult's 1 in cl for the
//    byte store (`movb $0x1,(%edx)` at 0x00458207).
LTBOOL CNetMgrSendView::sub_10022c2f(void* pData, uint32 nLen, uint32 nSpare,
                                     NetConnection* pConn)
{
	uint8*         pBytes;
	uint32         nPacketID;
	uint32         nOffset;
	uint32         nEnd;
	uint32         nPiece;
	uint32         nFrag;
	LTBOOL         bResult;
	uint8*         pOut;
	uint32         nRoom;

	if (nLen < 0x44d)
	{
		return ((CBaseDriverSendView*)pConn->m_pDriver)->SendData(
			pData, nLen, nSpare, pConn);
	}

	pBytes    = (uint8*)pData;
	nPacketID = *(uint32*)(pBytes + nLen - 4);
	nLen     -= 4;

	CNetMgr::DebugPrint(pConn, 2, "Fragmenting packet %d", nPacketID);

	CMessageRef msg = msg_AllocPacket(0x44c, 0x44c);

	msg.m_pMsg->m_pData[0] = 1;

	nOffset = 0;
	nPiece  = (nLen >> 2) + 1;
	bResult = LTTRUE;

	for (nFrag = 0; nFrag < 4; nFrag++)
	{
		nEnd = (nOffset + nPiece < nLen) ? nOffset + nPiece : nLen;

		if (nEnd == nOffset)
		{
			DebugPrint(0, "Unable to complete fragment upload!");
			break;
		}

		{
			// A block-local copy, and it is worth three bytes and one
			// residual block: the shipped code has `mov 0x8(%ebp),%eax`
			// once and both stores off it, where writing msg.m_pMsg-> twice
			// reloads between them -- the slot is address-exposed
			// (msg went to msg_AllocPacket as its return slot), so a
			// member expression cannot be kept. Scoped to these two statements only,
			// because the shipped code reloads again immediately after
			// for WriteByte's `this`.
			ServerMessage* pM = msg.m_pMsg;

			pM->m_Unknown38 = 1;
			pM->m_Unknown34 = 1;
		}

		msg.m_pMsg->WriteByte(4);
		msg.m_pMsg->WriteByte((uint8)(nFrag | 0x30));
		((ServerMessageWriteView*)msg.m_pMsg)->WriteDWord(nPacketID);
		msg.m_pMsg->WriteData(pBytes + nOffset, nEnd - nOffset);

		nRoom = msg.m_pMsg->m_DataSize;
		pOut  = msg.m_pMsg->m_pData;

		if (!((CBaseDriverSendView*)pConn->m_pDriver)->SendData(
			pOut, msg.m_pMsg->m_Unknown34, nRoom - msg.m_pMsg->m_Unknown34, pConn))
		{
			bResult = LTFALSE;
			break;
		}

		nOffset = nEnd;
	}

	return bResult;
}


// The two sends at the bottom each RETURN their result: both images carry
// the CMsgLenSave restore and both CMessageRef releases twice, once behind
// each send (server.dll 0x10023029 `jmp` into a shared tail only because /O1
// merges it; lithtech.exe 0x0045865c / 0x004586b4 two full epilogues). A
// `bSent = ...; ... return bSent;` gives one copy (lithtech 971 bytes against
// 1026). The save's stores are length, message, position: server.dll stores
// the message (-0x18) between the other two (0x10022da7).
//
// The two sends take the buffer and the size through the named locals pOut
// and nRoom, m_DataSize FIRST -- sub_10022c2f's own spelling of the same
// send (0x10023014/0x1002302b load 0x58 before 0x54 and 0x34, and the
// driver path keeps m_pData in a frame slot across the vtable load).
// Spelled inline it was 781 of 787 with three blocks.
//
// NEAR on both images. lithtech.exe 0x004582f0 (1045 of 1026, 12 blocks;
// 17 before the locals): shipped declines the whole Prepend splice at
// 0x004583f9 (`call 0x0045a210` InsertBefore, `call 0x0045a1c0` Append)
// where ours expands both -- the /O2 budget again, see lthread.h's
// Append. server.dll 0x10022d6c: 787 of 787, 277 instructions, ONE
// residual: shipped homes pOut in the dead pMsg slot (+0x8, which pCur and
// the returned packet reference also share) and ours in pConn's (+0xc, nLen's); 0x10023037 and
// 0x10023042. Not moved by the declaration order of any of the locals
// (pOut first, last, beside pCur, nLen at function scope), by pOut/nRoom
// block-scoped in either arm, by assigning pOut before nRoom, or by the
// packet out-parameter at function scope (worse: 14 instructions; measured
// under the old out-parameter spelling of msg_AllocPacket).
LTBOOL CNetMgrSendView::SendPacketData(ServerMessage* pMsg, NetConnection* pConn)
{
	CMessageRef             refSpill;
	CMessageRef             refAck;
	CMsgLenSave             save;
	CNetRecvPacketSendView* pRec;
	CNetRecvPacketSendView* pCur;
	uint8*                  pOut;
	uint32                  nRoom;
	LTBOOL                  bSent;   // unused since the sends return directly; deleting it costs lithtech one block

	if (pMsg->m_pData[0] & 0x80)
	{
		save.m_nLen = pMsg->m_Unknown34;
		save.m_pMsg = pMsg;
		save.m_nPos = pMsg->m_Unknown38;

		((ServerMessageWriteView*)pMsg)->WriteDWord(
			((NetConnectionResetView*)pConn)->m_nSent);
	}

	if (pMsg->m_MsgFlags & 6)
	{
		if (pMsg->m_MsgFlags & 2)
			dsi_ServerError("*** Packet read overflow, disconnecting ***");
		else
			dsi_ServerError("*** Packet write overflow, disconnecting ***");

		pConn->m_Flags |= 4;
		pMsg->m_MsgFlags &= ~6;

		return LTFALSE;
	}

	pRec = LTNULL;

	if (pMsg->m_pData[0] & 0x80)
	{
		if (!(pConn->m_Flags & 1))
		{
			pRec = (CNetRecvPacketSendView*)sub_10023c4d(pMsg->m_Unknown34);

			memcpy(pRec->m_pData, pMsg->m_pData, pMsg->m_Unknown34);

			pRec->m_Len = pMsg->m_Unknown34;
			pRec->m_ID = ((NetConnectionResetView*)pConn)->m_nSent;

			((LMessageList*)&pConn->m_SendQueue)->Prepend((LThreadMessage*)pRec);
		}

		((NetConnectionResetView*)pConn)->m_nSent++;
	}

	if ((pMsg->m_MsgFlags & 8) && pRec)
	{
		pRec->m_bPending = 1;

		return LTTRUE;
	}
	{
		CNetMgr::DebugPrint(pConn, 5, "Sent packet %d to connection %p (packet ID %d (%d), length %d).",
			((NetConnectionResetView*)pConn)->m_nSent, pConn,
			pMsg->m_pData[0] & 0x3f, pMsg->m_pData[0], pMsg->m_Unknown34);

		if (pMsg->m_MsgFlags & 0x10)
		{
			pCur = (CNetRecvPacketSendView*)pConn->m_SendQueue.m_pTail;

			while (pCur)
			{
				pRec = pCur;
				pCur = (pCur->m_pNext == (CNetRecvPacket*)pConn->m_SendQueue.m_pTail) ?
				           LTNULL : (CNetRecvPacketSendView*)pCur->m_pNext;

				if (pRec->m_bPending)
				{
					uint32 nLen = pRec->m_Len;

					if ((int)(nLen + 4) < ((ServerMessageWriteView*)pMsg)->GetSpaceLeft())
					{
						if (!net_CanAppend(pMsg))
						{
							sub_10023d16(&refSpill);
							sub_10023d5b(refSpill.m_pMsg, pMsg);

							pMsg = refSpill.m_pMsg;
						}

						sub_10023d73(pMsg, pRec->m_pData, pRec->m_Len);
					}
					else
					{
						sub_10022c2f(pRec->m_pData, nLen, pRec->m_nAlloced - nLen, pConn);
					}

					// Either way it is no longer pending: both arms reach
					// the clear (AvP2 1.0.9.6 server.dll 0x1002af7f jumps
					// to it, Primal Hunt's 0x1002ca47 too).
					pRec->m_bPending = 0;
				}
			}
		}

		if (pConn->m_AckTimer > g_NetPiggybackTime && !(m_Flags & 1) &&
		    !(pConn->m_Flags & 1) &&
		    ((ServerMessageWriteView*)pMsg)->GetSpaceLeft() >= 0x2c)
		{
			CNetMgr::DebugPrint(pConn, 2, "Piggybacking ack local: %d, remote: %d.",
				((NetConnectionResetView*)pConn)->m_nSent, pConn->m_PacketID);

			refAck.Init(msg_AllocPacket(0x44c, 0x44c));

			WriteAckPacket(pConn, refAck.m_pMsg);

			if (net_CanAppend(pMsg))
			{
				sub_10023d5b(pMsg, refAck.m_pMsg);
			}
			else
			{
				sub_10023d16(&refSpill);
				sub_10023d5b(refSpill.m_pMsg, pMsg);
				sub_10023d5b(refSpill.m_pMsg, refAck.m_pMsg);

				pMsg = refSpill.m_pMsg;
			}
		}

		if (!(pConn->m_Flags & 1) && (pMsg->m_pData[0] & 0x80))
		{
			nRoom = pMsg->m_DataSize;
			pOut  = pMsg->m_pData;

			return sub_10022c2f(pOut, pMsg->m_Unknown34,
				nRoom - pMsg->m_Unknown34, pConn);
		}
		else
		{
			nRoom = pMsg->m_DataSize;
			pOut  = pMsg->m_pData;

			return ((CBaseDriverSendView*)pConn->m_pDriver)->SendData(
				pOut, pMsg->m_Unknown34,
				nRoom - pMsg->m_Unknown34, pConn);
		}
	}
}


// 0x10022990 — put one packet on the wire, or park it if latency simulation
// is on.
//
// servermgr.h had this as `void`; it is not. Both exits leave a value in eax
// — a literal 1 from the queued path and SendPacketData's own answer from the
// real one — and the `ret 0xc` epilogue is shared between them, so the return
// type is LTBOOL. SendPacketData is likewise not void (below).
//
// The last statement is written out in both arms rather than once after the
// branch: the shipped code has `mov eax,[esi+0x54]; mov [eax],cl` twice, and
// it has to, because the two arms return different values and there is no
// shared tail to hang it on. It restores the first payload byte, which
// SendPacketData overwrites with the wire header.
//
// The result local lives in the *parameter* slot the now-dead pConn
// occupied — the frame has no `sub esp` at all — which is the usual /O1
// reuse and needs no help from the source.
// AVP2: the record's packet grows by 0x44c, the engine's packet size
// (server.dll 0x1002c4b5, lithtech.exe 0x00469033 `push $0x44c`).
LTBOOL CNetMgrSendView::SendPacket(ServerMessage* pMsg, NetConnection* pConn, uint8 n)
{
	LTBOOL bSent;

	if (g_NetResendTime > 0.001f)
	{
		CNetSentPacketView* pRec = ((CNetBankAllocView*)&m_PacketBank)->Allocate();

		pRec->m_Packet.Init(pMsg->m_Unknown34 + 0x32, 0x44c);

		memcpy(pRec->m_Packet.m_pData, pMsg->m_pData, pMsg->m_Unknown34);
		pRec->m_Packet.m_Unknown34 = pMsg->m_Unknown34;
		pRec->m_Packet.m_Unknown38 = pMsg->m_Unknown38;

		((LMessageList*)&pConn->m_SentList)->Prepend((LThreadMessage*)pRec);

		pRec->m_ResendTimer = 0.0f;

		*pMsg->m_pData = (uint8)n;

		return LTTRUE;
	}

	bSent = SendPacketData(pMsg, pConn);
	if (bSent)
		AddSentBytes(pConn, pConn->m_pDriver->GetRecvSize() + pMsg->m_Unknown34);

	*pMsg->m_pData = (uint8)n;

	return bSent;
}


// 0x1002307f — take one arrived packet apart: drop it if the drop-rate knob
// says so, check its checksum, put it in sequence if it is guaranteed, and
// strip the trailer so the caller sees only the payload. Nonzero means the
// packet is ready to be handed out.
//
// Three of the readings here are worth writing down.
//
// The connection's flag word is read *once* into a local for the first two
// tests (local, then has-checksum) and then re-read from memory at the three
// later tests, which is why there is a local at all: one `mov 0x70(%esi),%eax`
// serving two `test %al` and three separate `testb $0x2,0x70(%esi)`.
//
// The sequence-number comparison is a three-armed if/else chain against the
// same pair, and VC6 keeps the third arm's branch even though it can prove it
// (`jne`, then `jae`, then a `cmp`/`jbe` whose fall-through is unreachable).
// That third `nID > m_PacketID` has to be written out; an `else` in its place
// deletes the compare.
//
// The trailer length is computed twice from the same expression, once for the
// bounds check and once for the strip, in two different registers — so it is
// two expressions in the source and not one local. All three of those are
// written as an assignment followed by a conditional override rather than as
// a `?:`: `(flags & 2) ? 6 : 4` is `4 | (flags & 2)` and VC6 knows it, so the
// conditional-expression form comes out as `and $2` / `or $4` with no branch
// at all and the function loses ten bytes. The shipped code materialises the
// default and jumps over the override, which is what
// `n = 4; if (flags & 2) n = 6;` gives.
//
// The lengths are `uint16` locals throughout, and the 16-bit compares
// (`mov 0x34(%edi),%dx; cmp %bx,%dx`) are the tell: a `uint32` local forces a
// `movzwl` at the load and widens every compare.
//
// lithtech.exe 0x458700 (973, IDENTICAL) settled two more:
//   * each arm carries its OWN `if (len < nStrip) return 0;` (0x458a5f and
//     0x458a9c, each with its own `jae`); one shared test after the if/else
//     gave a `jmp` into a single compare. server.dll is the same either way.
//   * no `nType` local: the first byte is read through pData at each use.
//     With a uint8 local /O2 put it in %al and spilled it across the flag
//     test; shipped keeps it in %dl (0x4588ad) and reuses %edi for nID.
//
// PH: an empty CMessageRef opens the frame, as in sub_10023d73: server.dll
// 0x1002cbdd zeroes -0x14(%ebp) and enters EH state 0, the unwind at
// 0x100713bc being `lea -0x14(%ebp),%ecx; jmp 0x10012b8b` (~CMessageRef).
// The packet's first byte is a uint8 local after the internal handler
// (0x1002cd59 `mov (%eax),%cl`, reused as %cl for the frame-count warning
// and the unguaranteed print).
//
// NEAR (PH server.dll 839 of 837 bytes; lithtech.exe 0x00469b40 987 of
// 988): register allocation in the guaranteed arm. Shipped keeps `this` in
// %ebx throughout and spills the packet id into pMsg's dead home
// (0x8(%ebp)), reloading m_pData for the id read; ours takes %ebx for the
// trailer length and the id and pushes `this` from its spill. One shared
// trailer test after the if/else changed nothing on server.dll and cost
// lithtech.exe a byte.
int CNetMgr::TranslatePacket(ServerMessage* pMsg, NetConnection* pConn, int bRaw)
{
	CMessageRef cUnused;
	uint32 nFlags;
	uint32 nID;
	uint16 nStrip;
	uint8  nType;
	CNetRecvPacket* pRec;

	if (g_NetDropRate > 0.0001f && bRaw)
	{
		if (pConn->m_Flags & 1)
		{
			dsi_ServerError("Warning: using DropRate on local connection");
			dsi_ServerError("         MUST use ForceRemote in addition");
		}

		if ((((float)rand() / 32767.0f) * 100.0f) < g_NetDropRate)
		{
			uint32 nDropID;

			m_UnknownDC++;

			nDropID = (uint32)-1;

			if (pMsg->m_pData[0] & 0x80)
				nDropID = *(uint32*)&pMsg->m_pData[pMsg->m_Unknown34 - 4];

			DebugPrint(pConn, 1, "Dropping packet %lu", nDropID);

			return 0;
		}
	}

	nFlags = pConn->m_Flags;

	pConn->m_RecvWait = 0.0f;

	if (nFlags & 1)
	{
		((NetConnectionResetView*)pConn)->m_nPacketsRecv++;

		DebugPrint(pConn, 4, "Got packet %d from connection %p (packet ID %d, length %d).",
			((NetConnectionResetView*)pConn)->m_nPacketsRecv, pConn,
			pMsg->m_pData[0] & 0x3f, pMsg->m_Unknown34);

		// lithtech.exe 0x458801/0x458812: both refusals `jne` to ONE return-0
		// block, the one the final strip check (0x458aa5) ends in; with two
		// separate ifs /O2 gives each its own epilogue (+18 bytes). server.dll
		// cannot take the merged form: its single exit is unaffected, but the
		// extra join flips which of the `DebugPrint(pConn, 1..2, ...); return 0`
		// tails VC6 cross-jumps into (shipped 0x100231d3 / 0x100232ea both go
		// to the drop arm; the merged spelling sends the drop arm to them).
		// Measured with ||, `!a && !b` and nested positive guards: all three
		// close lithtech and all three lose server. Hence the image split.
#ifdef LT_SERVER_BUILD
		if (((CNetMgrSendView*)this)->sub_10023a8c(pConn, pMsg))
			return 0;

		if (((CNetMgrSendView*)this)->sub_100235e8(pConn, pMsg))
			return 0;
#else
		if (((CNetMgrSendView*)this)->sub_10023a8c(pConn, pMsg) ||
			((CNetMgrSendView*)this)->sub_100235e8(pConn, pMsg))
			return 0;
#endif

		if (pMsg->m_pData[0] & 0x80)
			pMsg->m_Unknown34 -= 4;

		return 1;
	}

	if (nFlags & 2)
	{
		uint16 nLen = pMsg->m_Unknown34;
		uint16 nCRC;

		if (nLen < 2)
			return 0;

		nCRC = *(uint16*)&pMsg->m_pData[nLen - 2];

		if (nCRC != net_Checksum(pMsg->m_pData, nLen - 2))
		{
			DebugPrint(pConn, 1, "Bad CRC packet .. first byte %d.",
				(int)(char)pMsg->m_pData[0]);

			return 0;
		}
	}

	if (((CNetMgrSendView*)this)->sub_100235e8(pConn, pMsg))
		return 0;

	nType = pMsg->m_pData[0];

	if (nType & 0x80)
	{
		uint16 nTail = 4;

		if (pConn->m_Flags & 2)
			nTail = 6;

		if (pMsg->m_Unknown34 < nTail)
			return 0;

		nID = *(uint32*)&pMsg->m_pData[pMsg->m_Unknown34 - nTail];

		if (nID == pConn->m_PacketID)
		{
			DebugPrint(pConn, 2, "Received guaranteed packet %d", nID);

			((CNetMgrSendView*)this)->sub_10022ac2(pConn);
		}
		else if (nID < pConn->m_PacketID)
		{
			DebugPrint(pConn, 1, "Ignoring duplicate packet %d/%d",
				nID, pConn->m_PacketID);

			return 0;
		}
		else if (nID > pConn->m_PacketID)
		{
			if (nID > pConn->m_PacketID + 0x100)
				DebugPrint(pConn, 1, "Warning! Suspicious frame count %d on msg ID %d",
					nID, nType);

			if (FindQueuedPacket(&pConn->m_GuaranteedQueue, nID))
			{
				DebugPrint(pConn, 2, "Received duplicate out of order packet %d", nID);

				return 0;
			}

			DebugPrint(pConn, 2, "Received out of order packet %d", nID);

			pRec = ((CNetMgrSendView*)this)->sub_10023c4d(pMsg->m_Unknown34);
			pRec->m_Len = pMsg->m_Unknown34;

			memcpy(pRec->m_pData, pMsg->m_pData, pMsg->m_Unknown34);

			pRec->m_ID = nID;

			if (!((CNetMgrSendView*)this)->sub_10023b1f(&pConn->m_GuaranteedQueue, pRec))
				FreeRecvPacket(pRec);

			pConn->m_AckTimer += g_NetAckTime * 0.5f;

			return 0;
		}

		DebugPrint(pConn, 4, "Got packet %d from %p (packet ID %d (%d), length %d).",
			pConn->m_PacketID, pConn, pMsg->m_pData[0] & 0x3f,
			pMsg->m_pData[0], pMsg->m_Unknown34);

		if (((CNetMgrSendView*)this)->sub_10023a8c(pConn, pMsg))
			return 0;

		nStrip = 4;

		if (pConn->m_Flags & 2)
			nStrip = 6;

		if (pMsg->m_Unknown34 < nStrip)
			return 0;
	}
	else
	{
		DebugPrint(pConn, 4, "Got packet from %p (packet ID %d, length %d).",
			pConn, nType & 0x3f, pMsg->m_Unknown34);

		nStrip = 0;

		if (pConn->m_Flags & 2)
			nStrip = 2;

		if (pMsg->m_Unknown34 < nStrip)
			return 0;
	}

	pMsg->m_Unknown34 -= nStrip;

	((NetConnectionResetView*)pConn)->m_nPacketsRecv++;

	return 1;
}


// ---- growing and shrinking ----------------------------------------------- //
//
// `Insert2` (0x1002431f) and `Remove2` (0x1000cd8e) used to be hand-written
// here as InsertAt/RemoveAt and were both NEAR -- 193 bytes against 191 with
// the item count reloaded rather than kept in ecx, and 182 against 182 with
// two adjacent instructions transposed. They are collection.h's template
// members now, which collection.cpp already matches at both addresses
// exactly, and it is Remove2 that lithtech.exe expands whole into
// CNetMgr::RemoveDriver at 0x004561e0.

// 0x10058780 — the list every CUnknown390 links itself into (objtypes.cpp's
// un390_Init / un390_Term reach it by extern). A file-scope DECLARE_LTLINK:
// its dynamic initialiser is server.dll 0x10024486 (the _$E head, a
// zero-displacement jmp) and 0x1002448b (the body, ltlink.h's TieOff
// expanded on a constant `this`: m_pNext stored before m_pPrev, as the
// chained assignment gives). Both sit directly in front of 0x1002449b below,
// which is what places the definition here.
DECLARE_LTLINK(g_Un390List)

// ---- 0x1002449b, RECONSTRUCTED (385 of 385, byte-identical) ------------- //
//
// servermgr.h's CUnknown390 (the manager's +0x390) constructor. It was never
// a codegen problem, it was a layout one, and the layout it needed was
// already in src/engine/server/objbank.cpp -- CObjMgrView, the same view
// whose destructor (0x1002ca7c) that file matches exactly. The constructor
// now lives beside it there; what follows is only the evidence, kept because
// this is where it was first written down.
//
// The body is twelve sub-objects and an array. Eleven of the sub-objects are
// the vptr-over-StructBank shape CNetBank and CMessageBank have, spaced 0x24
// apart, each built as `vptr = <table>; sb_Init2(&bank, structSize, 0x20,
// growSize)` with 0x20 for the cache size every time:
//
//   offset  vtable      structSize  growSize
//   0x68    0x1004df58  0x1a8       0x20
//   0x8c    0x1004df48  0x248       0x100
//   0xb0    0x1004df38  0x270       0x20
//   0xd4    0x1004df28  0x1c8       0x80
//   0xf8    0x1004df18  0x1ac       0x20
//   0x11c   0x1004df08  0x1d0       0x20
//   0x140   0x1004def8  0x25c       0x20
//   0x164   0x1004dee8  0x11f8      0x20
//   0x188   0x1004ded8  0x21c       0x20
//   0x1ac   0x1004dec8  0x274       0x20
//   0x1d0   0x1004deb8  0x1b4       0x20
//
// objbank.cpp's eleven CObjectBank<T> instantiations reproduce every one of
// those structSize immediates out of sizeof(T), so the grow size is the only
// thing the constructor has to be told.
//
// Then an array of eleven sixteen-byte records at 0x220, built by a real loop
// (`mov $0xb,%edx` down-counted, stride 0x10): each record's first dword is
// cleared and the LTLink at its +0x04 is tied off to itself. That is an array
// of eleven LTLists -- ltlink.h's own default constructor, one member array,
// one loop.
//
// THE POINTER AT OFFSET 0x00 IS NOT A vptr, and that is what had this
// blocked. The constructor writes it TWICE -- 0x1004df68 before the eleven
// banks and 0x1004deb0 after them -- and the two cannot be a base/derived
// pair: 0x1004df68 is four slots ({0x1003ea24, 0x1003ea24, 0x10024690,
// 0x10024afc}, the first two _purecall) and 0x1004deb0 is two
// ({0x1002461c, 0x10017b0e}, the second folded onto Model::GetFilename), and
// no derived table is shorter than its base's. The destructor decides it: a
// polymorphic class restores its own vftable at the top of its destructor,
// and 0x1002ca7c has no store to (%esi) anywhere. So offset zero is plain
// data written by hand, exactly as servermgr.h's CUnknown8FC already does,
// and the only trick is that the FIRST store has to precede eleven member
// constructors -- which a one-member base class with an inline constructor
// puts there and a body statement never could.

// ---- a two-dword list, built here because this is where its bytes land -- //
//
// 0x100243f6 — the smallest of the sub-objects the bigger engine records hang
// off themselves: two dwords, a head and a back-pointer, and a constructor
// whose whole body is one call to the initialiser with a null owner. It sits
// at Model+0x14 (commonmodel.h's note lists that whole run of thirteen
// sub-object constructors) and at +0x28 of the object whose vtable is
// 0x1004d344 — and that second call site passes its own `this` as the owner,
// which is what says the constructor's zero is a default and not the only
// value the field ever takes. Named for the offset it occupies in Model, the
// way worldbsp.h names CWorldTreeSub28.
//
// Only the constructor is reconstructed here; the initialiser is declared and
// never defined, which is all the per-object verification needs. It also has
// to stay out of line — /O1 implies /Ob1, so a body written inside the class
// declaration would be expanded into the constructor and the call would
// disappear.
//
// What the other three functions of the family do, read off the shipped
// bytes so the next attempt does not have to re-read them:
//   0x10024409  Init(pOwner): `m_pHead = NULL; m_pOwner = pOwner;`, returns
//               LTTRUE (`push $1; pop eax`), `ret 4`.
//   0x10024404  the destructor — a bare `jmp` to 0x10024419, so its body is
//               exactly one tail call.
//   0x10024419  calls 0x10024431(0, &nOne) with a `1` in a stack local.
//   0x10024431  walks the chain: each node at m_pHead carries an inner list
//               at its +0x00 and the next node at its +0x08; each inner node
//               carries a function pointer at +0x00 and its own next at
//               +0x04, and every function is called cdecl with
//               (this, pNode, arg1, arg2). Returns 0.
class CModelSub14
{
public:
	CModelSub14();                           // 0x100243f6
	LTBOOL Init(void* pOwner);               // 0x10024409
	void   sub_10024419();                   // 0x10024419
	LTBOOL Walk(void* pArg1, void* pArg2);   // 0x10024431 (netobj.cpp's WalkQ458)

	void*  m_pHead;                          // 0x00
	void*  m_pOwner;                         // 0x04
};

CModelSub14::CModelSub14()
{
	Init(LTNULL);
}

// 0x10024409 — the initialiser the constructor calls: an empty chain and the
// owner the caller named, and it answers LTTRUE unconditionally.
LTBOOL CModelSub14::Init(void* pOwner)
{
	m_pHead  = LTNULL;
	m_pOwner = pOwner;

	return LTTRUE;
}

// 0x10024419 — the destructor's whole body (0x10024404 is a bare `jmp` here)
// and Model's own release at +0x14: walk the chain with a null first
// argument and the address of a stack `1` as the second — `movl $1,-4(%ebp)`
// then `lea -4(%ebp),%eax ; push %eax ; push $0`. The walk's answer is
// dropped. The walk itself is 0x10024431, reconstructed in netobj.cpp's
// holding pen as CModelSub14Q458::WalkQ458.
void CModelSub14::sub_10024419()
{
	uint32 nOne = 1;

	Walk(LTNULL, &nOne);
}


// ---- the fragment reassembler and the message helpers -------------------- //

// 0x100211e2 — nonzero when the message in hand is a guaranteed packet whose
// first sub-type is 3 (the appendable sub-packet stream), so the send path
// can keep writing into it instead of starting a fresh one. Cdecl, one
// argument, and it never looks at anything but the first two payload bytes.
int net_CanAppend(ServerMessage* pMsg)
{
	uint8* pData = pMsg->m_pData;

	if ((pData[0] & 0x3f) == 1 && pData[1] == 3)
		return 1;

	return 0;
}


// 0x10023474 — nonzero once all four fragments of a split packet have
// arrived. `this` is never read; the record is the only argument.
int CNetMgrSendView::sub_10023474(CNetFragRec* pRec)
{
	uint32 i;

	for (i = 0; i < 4; i++)
	{
		if (!pRec->m_pFrags[i].m_pMsg)
			return 0;
	}

	return 1;
}


// NEAR (56 of 54 bytes, one instruction over): ours rotates the loop —
// peeling the head test and duplicating it into the back edge — where the
// shipped build keeps one shared test that both the entry and the two `jne`
// back edges reach. Same class as CNetMgr::FindQueuedPacket and
// sub_10023c4d below, and the rule behind all three is in the note on
// sub_10023a91: cl 8447 rotates a loop whose back edge is *conditional* and
// leaves one whose back edge is an unconditional `jmp` alone.
//
// 0x100233ad — find the record collecting a given packet id for a given
// connection. The manager's second queue is walked from its tail with the
// cursor alias, and a record has to match on both the id and the connection:
// two ids can be in flight at once and two connections can be sending the
// same id.
CNetFragRec* CNetMgrSendView::sub_100233ad(uint32 nID, NetConnection* pConn)
{
	CNetFragRec* pTail = (CNetFragRec*)m_QueueC4.m_pTail;
	// RE-READ THE MEMBER, do not copy pTail. Written `pCur = pTail;`
	// VC6 tests the tail and sinks the copy; the shipped bytes copy first
	// and test the copy. Proven in-tree by tcpdriver.cpp's tcp_FindConnection,
	// which reads the member twice and is byte-identical.
	CNetFragRec* pCur = (CNetFragRec*)m_QueueC4.m_pTail;
	CNetFragRec* pRec;

	while (pCur)
	{
		pRec = pCur;
		pCur = (pCur->m_pPrev == (LThreadMessage*)pTail) ?
		           LTNULL : (CNetFragRec*)pCur->m_pPrev;

		if (pRec->m_ID == nID && pRec->m_pConn == pConn)
			return pRec;
	}

	return LTNULL;
}


// 0x10023d73 — append a length-prefixed block to a message: the length as a
// word and then the bytes. `this` is unread, so the message it writes to is
// the first argument rather than a member.
//
// Both images are byte-identical. The word goes through packet.h's
// CPacket::WriteType<uint16> directly: server.dll calls it (0x1000ec3d, the
// same target the old WriteWord declaration named), and lithtech.exe
// 0x004599f0 expands it. The block goes through a one-line inline forwarder
// onto WriteData (0x1002616a / 0x0045c390). That forwarder is not decoration.
// /O2 spends its inline budget on the LAST sites first, and expanding the
// forwarder after the WriteType is what keeps Insert2 a call inside the
// expanded write (0x00459a31 -> 0x00412f50). Without it Insert2 expands too
// (365 bytes against 195).
//
// PH: an empty CMessageRef lives across both writes. Nothing ever lands in it,
// so its closing Release folds away, but under /GX it still owns the frame:
// server.dll 0x1002d953 (`and -0x10(%ebp),0` then EH state 0, the unwind at
// 0x10071450 being `lea -0x10(%ebp),%ecx; jmp 0x10012b8b`, ~CMessageRef) and
// lithtech.exe 0x0046aef0 (the null at 0x8(%esp), state byte 0 before the
// expanded write's Insert2 call).
void CNetMgrSendView::sub_10023d73(ServerMessage* pMsg, void* pData, uint32 nLen)
{
	CMessageRef cUnused;

	((CPacket*)pMsg)->WriteType((uint16)nLen);

	((ServerMessageTypeView*)pMsg)->WriteDataInline(pData, (uint16)nLen);
}


// 0x10023d5b — the same append with the block taken from another message's
// payload: everything written into pSrc so far goes onto pDst as one sub-
// packet. `this` survives in ecx across the call because nothing in between
// touches it.
void CNetMgrSendView::sub_10023d5b(ServerMessage* pDst, ServerMessage* pSrc)
{
	sub_10023d73(pDst, pSrc->m_pData, pSrc->m_Unknown34);
}


// 0x10023d16 — start a fresh internal packet in the reference handed in: a
// message off the packet allocator, its first payload byte set to the
// internal packet id (1), and the sub-type written straight after it (3, the
// appendable sub-packet stream).
#ifndef LT_SERVER_BUILD
// STAND-IN (lithtech.exe only, name ours): the header byte and the sub-type
// written by one inline helper. lithtech.exe 0x004598d0 expands WriteType
// but CALLS Insert2 (0x00459956 -> 0x00412f50); with the two writes in the
// function body /O2 has budget left and expands Insert2 as well (413 bytes).
// One inline level around both writes spends it, as a destructor after the
// write does in smgr.cpp's sm_BroadcastPrintf (0x00476010).
inline void NetStartPacket(CMessageRef* pRef, uint8 nID, uint8 nSubType)
{
	pRef->m_pMsg->m_pData[0] = nID;

	((CPacket*)pRef->m_pMsg)->WriteType(nSubType);
}
#endif

// 0x10023d16 — start a fresh internal packet in the reference handed in: a
// message off the packet allocator, its first payload byte set to the
// internal packet id (1), and the sub-type written straight after it (3, the
// appendable sub-packet stream).
void CNetMgrSendView::sub_10023d16(CMessageRef* pRef)
{
	pRef->Init(msg_AllocPacket(0x44c, 0x44c));

#ifdef LT_SERVER_BUILD
	pRef->m_pMsg->m_pData[0] = 1;

	((CPacket*)pRef->m_pMsg)->WriteType((uint8)3);
#else
	NetStartPacket(pRef, 1, 3);
#endif
}


// 0x10023dc2 — CNetMgr::DebugPrint without the connection: the same level
// test and the same trace line, minus the per-connection quiet filter that
// needs a connection to read m_Flags off. An overload rather than a second
// name, which is what the two symbols look like from the call sites.
void CNetMgrSendView::DebugPrint(int nLevel, char* pFormat, ...)
{
	char    msg[500];
	va_list marker;

	if (nLevel > g_NetDebugLevel)
		return;

	va_start(marker, pFormat);
	_vsnprintf(msg, sizeof(msg) - 1, pFormat, marker);

	dsi_ServerError("(%d) %s%s", NET_TRACE_TIME(), m_pTraceName, msg);
}


// 0x10051c78 — the queue depth past which the send side starts refusing
// unguaranteed traffic; a plain initialised global (70) registered with the
// console as "ConnTroubleCount2" (its table entry at 0x10052214 names it).
extern int g_NetConnTroubleCount2;

// 0x10021c55 — send one message to one connection, with the wire header's
// two top bits taken from the caller.
//
// The refusal is the interesting half: with neither the "must go" bit nor
// the local-connection flag set, a connection with more than 70 entries
// backed up on *either* queue is answered LTTRUE without anything going out.
// The two `&&` tests fall through into the send below them and the two `||`
// tests jump forward into the shared `return LTTRUE`, which is the whole of
// how the branch polarity reads back.
//
// The header byte is saved into the now-dead connection parameter slot as a
// *byte*, which is what says SendPacket's third argument is a uint8: the
// call pushes the whole dword that slot sits in, upper three bytes and all,
// with no zero extension.
LTBOOL CNetMgrSendView::SendToConnection(ServerMessage* pMsg, NetConnection* pConn,
                                     int nFlags)
{
	uint8 nType;

	if (!pConn)
		return LTFALSE;

	nFlags &= 0xc0;

	if (!(nFlags & 0x80) && !(pConn->m_Flags & 1) &&
	    (pConn->m_GuaranteedQueue.m_nMessages > (uint32)g_NetConnTroubleCount2 ||
	     pConn->m_SendQueue.m_nMessages > (uint32)g_NetConnTroubleCount2))
	{
		return LTTRUE;
	}

	pConn->m_SendWait = 0.0f;

	nType = pMsg->m_pData[0];

	pMsg->m_pData[0] = (uint8)((nType & 0x3f) | nFlags);

	return SendPacket(pMsg, pConn, nType);
}


// 0x1002294f — hand everything parked on a connection's send queue to the
// local-connection pipe. Each record carries its own payload, the length
// written into it and the capacity it was given, and the pipe takes the
// length and the slack separately. The records are not taken off the queue
// here: the walk is the cursor alias and the list is left as it was.
void CNetMgrSendView::sub_1002294f(NetConnection* pConn)
{
	CNetRecvPacket* pCur;
	CNetRecvPacket* pRec;

	if (!pConn)
		return;

	pCur = (CNetRecvPacket*)pConn->m_SendQueue.m_pTail;

	while (pCur)
	{
		pRec = pCur;
		pCur = (pCur->m_pNext == (CNetRecvPacket*)pConn->m_SendQueue.m_pTail) ?
		           LTNULL : pCur->m_pNext;

		sub_10022c2f(pRec->m_pData, pRec->m_Len,
		             pRec->m_nAlloced - pRec->m_Len, pConn);
	}
}


// lithtech.exe has no standalone copy of the constructor or of Clear:
// sub_10023495's /O2 twin (0x00458c00) expands both, and the slot release
// inside Clear with them (0x00458c7a / 0x00458cc8: `mov (%edi),%ecx ; test ;
// call *0x8(%eax) ; movl $0,(%edi)`, the cursor Init with a null argument).
// server.dll calls 0x10023505 and 0x1002351c out of line, and Clear calls
// 0x10023e86. So both are `inline` only for the client image, and Clear
// reaches the slot through an inline twin of the cursor Init there.
#ifdef LT_SERVER_BUILD
#define NETFRAG_INLINE
#else
#define NETFRAG_INLINE inline
#endif

// 0x1002351c — empty a record without freeing it, so it can be handed
// straight back out. Each slot is released through the cursor Init, then the
// packet id and the connection go.
NETFRAG_INLINE void CNetFragRec::Clear()
{
	uint32 i;

	for (i = 0; i < 4; i++)
#ifdef LT_SERVER_BUILD
		((CMsgCursor*)&m_pFrags[i])->Init(LTNULL);
#else
		m_pFrags[i].SetInline(LTNULL);
#endif

	m_ID    = 0;
	m_pConn = LTNULL;
}


// 0x10023505 — a fresh fragment record: the four slots start empty and the
// rest of the record is cleared by the same routine that recycles one.
NETFRAG_INLINE CNetFragRec::CNetFragRec()
{
	Clear();
}


// 0x10023542 — take a pending record off the queue and delete it (name
// ours). AVP2: the record is a CNetFragRec, and deleting it runs its
// destructor -- the four slots' releases -- before the free (server.dll
// 0x1002d11d: the list removal, `call 0x1002d144`, then operator delete;
// lithtech.exe 0x0046a200 the same with the removal expanded).
void CNetMgr::ReleaseQueued(CNetPending* pRec)
{
	if (pRec)
	{
		((LMessageList*)&m_QueueC4)->Remove((LThreadMessage*)pRec);

		delete (CNetFragRec*)pRec;
	}
}


// 0x10023495 — get a record to collect a split packet in. The manager keeps
// at most six of them: with more than that already queued the oldest (the
// head, one past the tail on the circular list) is taken off, emptied and
// reused, and otherwise a new one is allocated. Either way the record goes
// on the back of the queue, so "oldest" really is the least recently issued.
//
// The counting walk reads the tail through a pointer to the field, and the
// recycle arm reads the field itself. That is what the bytes say: both
// images hoist the tail into a register for the loop (server 0x1002349b,
// lithtech 0x458c00) and then RE-READ m_QueueC4.m_pTail for the arm
// (0x100234be, 0x458c2f). With the field named directly at both places VC6
// reused the loop's register in the arm, and the esi/edi pair for `this`
// and the alias temp came out transposed (server 106/112). Earlier measured
// negatives: a pTail local, a `pCur = pTail` copy, block-scoped pCur, a
// `for` over LThreadMessage*, nCount++ before the advance, the reversed
// ternary, the arm through an (LMessageList*) cast, a pList local (110),
// GenListPos with qualified GenBegin/GenGetNext (114), an in-class
// CountFrags (declined at /Ob1; 106 forced). lithtech used to get the reload
// from a stand-in inline helper; this one body is identical on both images.
CNetFragRec* CNetMgrSendView::sub_10023495()
{
	CNetFragRec*     pRec;
	CNetFragRec*     pCur;
	uint32           nCount = 0;
	LThreadMessage** ppTail = &m_QueueC4.m_pTail;

	pCur = (CNetFragRec*)*ppTail;

	while (pCur)
	{
		pCur = (pCur->m_pPrev == *ppTail) ?
		           LTNULL : (CNetFragRec*)pCur->m_pPrev;

		nCount++;
	}

	if (nCount > 5)
	{
		pRec = (CNetFragRec*)((CNetFragRec*)m_QueueC4.m_pTail)->m_pNext;

		((LMessageList*)&m_QueueC4)->Remove((LThreadMessage*)pRec);

		pRec->Clear();
	}
	else
	{
		pRec = new CNetFragRec;
	}

#ifdef AVP2_MODERN
	// fixed: the fragment-record allocator passes a null to the list splice
	// (RETAIL_BUGS.md). Out of memory, there is no record to queue.
	if (pRec)
		((LMessageList*)&m_QueueC4)->Append((LThreadMessage*)pRec);
#else
	((LMessageList*)&m_QueueC4)->Append((LThreadMessage*)pRec);
#endif

	return pRec;
}


// 0x100233e3 — put one fragment into a record at its index. A slot that is
// already filled means a duplicate, and the whole call is refused.
//
// Fragment zero is special: the packet's own header byte is read off the
// wire and written into the fragment's payload, so that when the four are
// replayed the first one carries the reassembled packet's type. The rest of
// the arriving message — everything from its read position to its write
// position — is then appended to the fragment.
//
// The now-dead record parameter's slot holds the allocator's returned
// temporary (`lea 0x10(%ebp)`, Primal Hunt server.dll 0x1002cf82), which is
// why the frame has no locals of its own.
//
// Two facts the bytes had to give up. The first fragment's message is read
// into a local *before* the byte is read off the wire — written as one
// statement the load sinks past the call — and the two `movw $1` stores
// share one load of the slot while the WriteData that follows them re-reads
// it, because a store through the loaded pointer could have overwritten the
// slot itself. One named local for the first pair and the raw expression for
// the third use is the only shape that gives both.
//
// The header byte is read into its own `uint8` local before the store. Written
// as one assignment, server.dll swapped esi and edi for the whole function:
// the slot went in esi and the message in edi, where shipped has the slot in
// edi (0x100233f4) and the message in esi (0x1002342a). It was a 145/145 near
// that looked like pure allocation. Byte-identical on both images now
// (lithtech 0x458b10, 180 bytes). Negatives, no change on server: the
// declarations reordered; `pRec->m_pFrags + nIndex`; `!nIndex`;
// `&m_pData[...]`; the source message through a copy, or its data pointer
// through a local; the length passed without the (uint16) cast.
LTBOOL CNetMgrSendView::sub_100233e3(ServerMessage* pMsg, uint8 nIndex,
                                     CNetFragRec* pRec)
{
	CNetFragSlot*  pSlot = &pRec->m_pFrags[nIndex];
	ServerMessage* pDst;

	if (pSlot->m_pMsg)
		return LTFALSE;

	((CMessageRef*)pSlot)->Init(msg_AllocPacket(0x44c, 0x44c));

	if (nIndex == 0)
	{
		ServerMessage* pFrag = pRec->m_pFrags[0].m_pMsg;
		uint8 nType = pMsg->ReadByte(0);

		pFrag->m_pData[0] = nType;
	}

	pDst = pSlot->m_pMsg;

	pDst->m_Unknown38 = 1;
	pDst->m_Unknown34 = 1;

	((ServerMessageTypeView*)pSlot->m_pMsg)->WriteData(
		pMsg->m_pData + pMsg->m_Unknown38,
		(uint16)(pMsg->m_Unknown34 - pMsg->m_Unknown38));

	return LTTRUE;
}


//   12  0x1002220d  u16  ReadType      FLG word
//   13  0x10022218  u8   ReadType      FLG byte
//   14  0x10022223  u32  ReadType      FLG dword
//   15  0x10022393  u8   ReadType      UUP sub-packet length
//   16  0x1002251f  u8   ReadType      PIC sub-type
//
//  netmgr.cpp:1556 already declares `ServerMessageTypeView` with exactly
//  the ReadWord / ReadDWord / ReadData members sites 1-7 need, so nothing
//  new has to be added for them; `ReadByte` is servermsg.h:340's.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
//  FRAME, read off the prologue at 0x10021cbe
//
//    push %ebp ; mov %esp,%ebp ; mov $0x2110,%eax ; call 0x1003e740
//        -- _chkstk for 0x2110 of locals
//    push %ebx ; push %esi ; push %edi
//
//    -0x2110  char   str[0x2000]      (memset at 0x10021cce)
//    -0x0110  char   temp[0x100]
//    -0x0010  ...    the four scalar slots, see below
//
//  0x2000 + 0x100 + 0x10 = 0x2110 exactly, so there are FOUR dword slots
//  above temp and no more.  They are:
//      -0x04  nSubLen   (shared by the GRP, UPD and UUP blocks)
//      -0x08  UPD's  returned packet reference
//      -0x0c  UUP's  returned packet reference
//      -0x10  GRP's  returned packet reference
//  Three separate slots for msg_AllocPacket's three temporaries.
//
//  Everything else lives in registers or in the incoming parameter home
//  slots, which /O1 reuses once the parameters are dead:
//      %esi     pMsg        (loaded 0x10021ce1, live throughout)
//      %ebx     nLen        (loaded 0x10021d0c / 0x10021d33)
//      %edi     scratch zero / scratch one / nCount
//      0x08(%ebp)  nCount (GRP), the uint16 Init/ReadData arg (UPD, UUP),
//                  the FLG word
//      0x0c(%ebp)  nSubWord (GRP), nFlags (UPD record loop), the
//                  CMessageRef (UPD, UUP)
//      0x10(%ebp)  the CMessageRef (GRP)
//      0x13(%ebp)  nSubByte (UPD, UUP), the FLG byte
//  None of that is expressible in source; it is recorded so the next
//  reader does not mistake a parameter store for a parameter write.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
//  SIGNATURE
//
//  __cdecl, three arguments, caller-cleaned (`add $0xc,%esp` after each of
//  the three recursive calls, 0x10021e9d / 0x100220ad / 0x10022434).
//  External linkage: servermgr.h:2358 declares it and netmgr.cpp:330 calls
//  it across no TU boundary but through that declaration.
//
//  THE THIRD ARGUMENT IS A BYTE, NOT AN `int`.  0x10021cef and 0x10021d16
//  are both `cmpb $0x0,0x10(%ebp)` -- a byte compare, which an `int`
//  parameter could not produce (an int 0x100 would test false).  So
//  servermgr.h:2358's `int n` is wrong in type and should read
//  `uint8 bOut`; netmgr.cpp:330 passes the literal 1 and is unaffected
//  either way.  The header is not edited here.  Alternative spelling at this site: `LTBOOL`, but the engine
//  typedefs LTBOOL to a 32-bit type, so it would not give `cmpb`.
// ---------------------------------------------------------------------
// AVP2: the body below is Primal Hunt's server.dll 0x1002b584..0x1002c185,
// 3074 bytes, with its two jump tables to 0x1002c215 (AvP2 1.0.9.6's
// server.dll carries the same strings). The readings above are the older
// engine's (0x10021cbe); what changed:
//  * the line is the caller's (the fourth argument), and so is the direction
//    byte (the fifth); the recursion passes both on (0x1002b77c..0x1002b79e).
//    Only the outermost call clears it, and with sizeof(pStr) -- four bytes,
//    the size of the pointer, a leftover from when the buffer was this
//    function's own array. strcat only needs the first byte zeroed, so it
//    does no harm.
//  * a "<S2C_" / "<C2S_" / "<U2U_" prefix by direction (1, 2, anything
//    else; lower case for the nested calls), then "G> " / "U> ".
//  * the names are bracketed -- "(NM )  ", "(UPD) " -- and the counts print
//    as "|%d| ". The sub-packets are sized to the engine's 0x44c packet.
//  * the update records no longer stop at the first opaque field: each of
//    MI, CLR, SCL, POS, ROT and ATT prints the length left and zeroes it
//    (`xor %ebx,%ebx` after each), so the loop ends at its own test.
//  * the filters and the print moved to CNetMgr::GetPacket.
//  * the message classes: 0x64 "[PLU]" decodes its flag word, 0x6a "PLO",
//    0x71 "PLD", 0xbe "PHU", 0x66 "PIC" with nineteen pickup types.
//
// NEAR (3084 of 3074 bytes): register allocation in the "(UPD) " arm. The
// shipped build keeps nLen in %ebx through both of its loops and the
// sub-packet's reference in nLen's dead home (0xc(%ebp)); ours keeps the
// 16-bit sub-length in %bx there and nLen in memory, which costs a frame
// slot (0x114 against 0x110) and the ReadByte(0) zero in %edi. Also the
// "|REQ %d| " print keeps its own sprintf where shipped jumps into the
// shared one (0x1002b8a3 -> 0x1002bdaf). Tried: a block-scoped uint16 for
// the sub-length, nLen-- ahead of the arm's strcat (3088).
//
// Every read is packet.h's CPacket::ReadType (0x10018b0a byte, 0x10012bd6
// word, 0x1002dce9 dword).
//
// RETAIL BUG: the FLG arm checks for eight bytes and reads ten (a dword, a
// word and a dword, 0x1002bae2..0x1002bb1e), so a truncated record reads
// two bytes past its end. A debugging trace only; not fixed.
#ifdef LT_SERVER_BUILD
// server.dll only: neither lithtech.exe (Primal Hunt or AvP2 1.0.9.6) carries
// the trace -- no "(UUP)", "(NM )" or "<U2U_" in either image.
void net_LogPacket(ServerMessage* pMsg, uint32 nLen, uint8 bOut, char* pStr,
                   uint8 nDir)
{
	char         temp[0x100];
	uint32       nFlags;
	uint32       nCount;
	uint32       nSubLen;
	uint16       nSubWord;
	uint8        nSubByte;
	uint32       nFlagLong;
	uint16       nFlagWord;
	uint8        nMsgClass;

	// 0x1002b59a: no line, no trace.
	if (!pStr)
		return;

	if (bOut)
		memset(pStr, 0, sizeof(pStr));

	switch (nDir)
	{
		case 1:
			strcat(pStr, bOut ? "<S2C_" : "<s2c_");
			break;

		case 2:
			strcat(pStr, bOut ? "<C2S_" : "<c2s_");
			break;

		default:
			strcat(pStr, bOut ? "<U2U_" : "<u2u_");
			break;
	}

	if (pMsg->m_pData[0] & 0x80)
	{
		strcat(pStr, bOut ? "G> " : "g> ");

		nLen -= 4;
	}
	else
	{
		strcat(pStr, bOut ? "U> " : "u> ");
	}

	switch (pMsg->m_pData[0] & 0x3f)
	{
		case 0:
			strcat(pStr, "(TCP) ");
			break;

		case 1:
			strcat(pStr, "(NM )  ");

			if (nLen < 1)
				goto bad;
			if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
				goto bad;

			nLen--;

			switch (((CPacket*)pMsg)->ReadType((uint8*)0))
			{
				case 0:
					strcat(pStr, "ACK ");

					if (nLen < 1)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
						goto bad;

					nLen--;

					if (((CPacket*)pMsg)->ReadType((uint8*)0))
						strcat(pStr, "PRQ ");

					if (nLen < 8)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 8)
						goto bad;

					nLen -= 8;
					nCount = 0;

					((CPacket*)pMsg)->ReadType((uint32*)0);
					((CPacket*)pMsg)->ReadType((uint32*)0);

					while (pMsg->m_Unknown34 - pMsg->m_Unknown38 > 4)
					{
						if (nLen <= 4)
							break;

						nCount++;
						nLen -= 4;

						((CPacket*)pMsg)->ReadType((uint32*)0);
					}

					if (nCount)
					{
						sprintf(temp, "|REQ %d| ", nCount);
						strcat(pStr, temp);
					}
					break;

				case 1:
					strcat(pStr, "DIS ");
					break;

				case 2:
					strcat(pStr, "PNG ");
					break;

				case 3:
					strcat(pStr, "GRP ");

					nCount = 0;

					while (nLen >= 2)
					{
						if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 2)
							break;

						nLen -= 2;

						nSubWord = ((CPacket*)pMsg)->ReadType((uint16*)0);

						if (nSubWord)
						{
							nSubLen = nSubWord;

							if (nLen < nSubLen)
							{
								strcat(pStr, "BAD ");
								break;
							}

							if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < (int)nSubLen)
							{
								strcat(pStr, "BAD ");
								break;
							}

							nCount++;

							{
								CMessageRef ref;

								ref.Init(msg_AllocPacket(0x44c, 0x44c));
								ref.m_pMsg->Init(nSubWord, 0x44c);

								((ServerMessageTypeView*)pMsg)->ReadDataInline(
									ref.m_pMsg->m_pData, nSubWord);

								ref.m_pMsg->m_Unknown34 = nSubWord;
								ref.m_pMsg->m_Unknown38 = 1;

								net_LogPacket(ref.m_pMsg, nSubLen, 0, pStr, nDir);
							}
						}
					}

					sprintf(temp, "|%d| ", nCount);
					strcat(pStr, temp);
					break;

				default:
					goto unknown;
			}
			break;

		case 4:
			strcat(pStr, "(NPV) ");
			break;

		case 5:
			strcat(pStr, "(ULW) ");
			break;

		case 6:
			strcat(pStr, "(LW )  ");
			break;

		case 7:
			strcat(pStr, "(COI) ");
			break;

		case 8:
			strcat(pStr, "(UPD) ");

			nLen--;

			while (nLen)
			{
				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 == 0)
					break;

				nLen--;

				nSubByte = ((CPacket*)pMsg)->ReadType((uint8*)0);

				if (nSubByte == 0)
					break;

				nSubLen = nSubByte;

				if (nLen < nSubLen)
				{
					strcat(pStr, "BAD ");
					break;
				}

				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < (int)nSubLen)
				{
					strcat(pStr, "BAD ");
					break;
				}

				{
					CMessageRef ref;

					ref.Init(msg_AllocPacket(0x44c, 0x44c));

					nSubWord = nSubByte;

					ref.m_pMsg->Init(nSubWord, 0x44c);

					nLen -= nSubLen;

					((ServerMessageTypeView*)pMsg)->ReadDataInline(
						ref.m_pMsg->m_pData, nSubWord);

					ref.m_pMsg->m_Unknown34 = nSubWord;
					ref.m_pMsg->m_Unknown38 = 1;

					net_LogPacket(ref.m_pMsg, nSubLen, 0, pStr, nDir);
				}
			}

			while (pMsg->m_Unknown34 - pMsg->m_Unknown38 > 0 && nLen > 0)
			{
				nLen--;

				nFlags = ((CPacket*)pMsg)->ReadType((uint8*)0);

				if (nFlags & 0x80)
				{
					if (nLen < 1)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
						goto bad;

					nLen--;

					nFlags |= (uint32)((CPacket*)pMsg)->ReadType((uint8*)0) << 8;
				}

				if (nFlags != 0)
				{
					if (nLen < 2)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 2)
						goto bad;

					nLen -= 2;

					sprintf(temp, "|%04x| ",
					        ((CPacket*)pMsg)->ReadType((uint16*)0));
					strcat(pStr, temp);

					if (nFlags & 1)
					{
						strcat(pStr, "NEW ");
						continue;
					}

					if (nFlags & 0x2020)
					{
						sprintf(temp, "MI  |%d| ", nLen);
						strcat(pStr, temp);
						nLen = 0;
					}

					if (nFlags & 8)
					{
						strcat(pStr, "FLG ");

						if (nLen < 8)
							goto bad;
						if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 8)
							goto bad;

						nLen -= 10;

						// The third read is the last argument itself: it is
						// pushed straight from eax (0x1002bb1e..0x1002bb23).
						nFlagLong = ((CPacket*)pMsg)->ReadType((uint32*)0);
						nFlagWord = ((CPacket*)pMsg)->ReadType((uint16*)0);

						sprintf(temp, "|%08hx %04hx %08x| ",
						        nFlagLong, nFlagWord,
						        ((CPacket*)pMsg)->ReadType((uint32*)0));
						strcat(pStr, temp);
					}

					if (nFlags & 0x40)
					{
						sprintf(temp, "CLR |%d| ", nLen);
						strcat(pStr, temp);
						nLen = 0;
					}

					if (nFlags & 0x10)
					{
						sprintf(temp, "SCL |%d| ", nLen);
						strcat(pStr, temp);
						nLen = 0;
					}

					if (nFlags & 0x202)
					{
						sprintf(temp, "POS |%d| ", nLen);
						strcat(pStr, temp);
						nLen = 0;
					}

					if (nFlags & 0x404)
					{
						sprintf(temp, "ROT |%d| ", nLen);
						strcat(pStr, temp);
						nLen = 0;
					}

					if (nFlags & 0x100)
					{
						sprintf(temp, "ATT |%d| ", nLen);
						strcat(pStr, temp);
						nLen = 0;
					}
				}
				else
				{
					if (nLen < 1)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
						goto bad;

					nLen--;

					switch (((CPacket*)pMsg)->ReadType((uint8*)0))
					{
						case 0:
							sprintf(temp, "SND |%d| ", nLen);
							break;

						case 1:
							sprintf(temp, "STK |%d| ", nLen);
							break;

						case 3:
							sprintf(temp, "REM |%d| ", nLen);
							break;

						default:
							sprintf(temp, "UNK |%d| ", nLen);
							break;
					}

					strcat(pStr, temp);
					nLen = 0;
				}
			}
			break;

		case 0x0a:
			strcat(pStr, "(UUP) ");

			nLen--;

			while (nLen)
			{
				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 == 0)
					break;

				nLen--;

				nSubByte = ((CPacket*)pMsg)->ReadType((uint8*)0);

				if (nSubByte == 0)
					break;

				nSubLen = nSubByte;

				if (nLen < nSubLen)
				{
					strcat(pStr, "BAD ");
					break;
				}

				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < (int)nSubLen)
				{
					strcat(pStr, "BAD ");
					break;
				}

				{
					CMessageRef ref;

					ref.Init(msg_AllocPacket(0x44c, 0x44c));

					nSubWord = nSubByte;

					ref.m_pMsg->Init(nSubWord, 0x44c);

					nLen -= nSubLen;

					((ServerMessageTypeView*)pMsg)->ReadDataInline(
						ref.m_pMsg->m_pData, nSubWord);

					ref.m_pMsg->m_Unknown34 = nSubWord;
					ref.m_pMsg->m_Unknown38 = 1;

					net_LogPacket(ref.m_pMsg, nSubLen, 0, pStr, nDir);
				}
			}

			sprintf(temp, "|%d| ", nLen);
			strcat(pStr, temp);
			break;

		case 0x0c:
			strcat(pStr, "(YID) ");
			break;

		case 0x0b:
		case 0x0d:
			strcat(pStr, "(MSG) ");

			if (nLen < 1)
				goto bad;
			if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
				goto bad;

			nLen--;

			nMsgClass = pMsg->m_pData[nLen];

			switch (nMsgClass)
			{
				case 0x64:
					sprintf(temp, "[PLU] |%d| ", nLen);
					strcat(pStr, temp);

					if (nLen < 2)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 2)
						goto bad;

					nFlags = ((CPacket*)pMsg)->ReadType((uint16*)0);

					if (nFlags & 0x001) strcat(pStr, "POS ");
					if (nFlags & 0x002) strcat(pStr, "ROT ");
					if (nFlags & 0x004) strcat(pStr, "VEL ");
					if (nFlags & 0x008) strcat(pStr, "AIM ");
					if (nFlags & 0x010) strcat(pStr, "INP ");
					if (nFlags & 0x020) strcat(pStr, "SCA ");
					if (nFlags & 0x040) strcat(pStr, "DIM ");
					if (nFlags & 0x080) strcat(pStr, "FL1 ");
					if (nFlags & 0x100) strcat(pStr, "FL2 ");
					if (nFlags & 0x200) strcat(pStr, "MST ");
					if (nFlags & 0x400) strcat(pStr, "CTR ");
					break;

				case 0x66:
					strcat(pStr, "PIC ");

					if (nLen < 1)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
						goto bad;

					switch (((CPacket*)pMsg)->ReadType((uint8*)0))
					{
						case 1:  strcat(pStr, "AMO "); break;
						case 2:  strcat(pStr, "HEA "); break;
						case 3:  strcat(pStr, "ARM "); break;
						case 4:  strcat(pStr, "WEA "); break;
						case 5:  strcat(pStr, "WPI "); break;
						case 6:  strcat(pStr, "AIR "); break;
						case 7:  strcat(pStr, "ROC "); break;
						case 8:  strcat(pStr, "OUT "); break;
						case 9:  strcat(pStr, "OBJ "); break;
						case 10: strcat(pStr, "MOD "); break;
						case 11: strcat(pStr, "EMP "); break;
						case 12: strcat(pStr, "INV "); break;
						case 13: strcat(pStr, "MTX "); break;
						case 14: strcat(pStr, "MFA "); break;
						case 15: strcat(pStr, "MXH "); break;
						case 16: strcat(pStr, "MXA "); break;
						case 17: strcat(pStr, "FAC "); break;
						case 18: strcat(pStr, "CAN "); break;
						case 19: strcat(pStr, "BAT "); break;
						default: goto unknown;
					}
					break;

				case 0x6a:
					sprintf(temp, "PLO |%d|", nLen);
					strcat(pStr, temp);
					break;

				case 0x71:
					sprintf(temp, "PLD |%d|", nLen);
					strcat(pStr, temp);
					break;

				case 0xbe:
					strcat(pStr, "PHU ");
					break;

				default:
					sprintf(temp, "%2x |%d| ", nMsgClass, nLen);
					strcat(pStr, temp);
					break;
			}
			break;

		case 0x0e:
			strcat(pStr, "(GRP) ");
			break;

		case 0x0f:
			strcat(pStr, "(COF) ");
			break;

		case 0x10:
			strcat(pStr, "(CV ) ");
			break;

		case 0x11:
			strcat(pStr, "(SKY) ");
			break;

		case 0x12:
			strcat(pStr, "(ISE) ");
			break;

		case 0x13:
			strcat(pStr, "(PF ) ");
			break;

		case 0x15:
			strcat(pStr, "(PLL) ");
			break;

		case 0x17:
			strcat(pStr, "(THL) ");
			break;

		case 0x18:
			strcat(pStr, "(UL ) ");
			break;

		case 0x19:
			strcat(pStr, "(LMA) ");
			break;

		case 0x1a:
			strcat(pStr, "(GL ) ");
			break;

		case 0x32:
			strcat(pStr, "(FD ) ");
			break;

		case 0x33:
			strcat(pStr, "(ST ) ");
			break;

		case 0x34:
			strcat(pStr, "(CFT) ");
			break;

		case 0x35:
			strcat(pStr, "(FB ) ");
			break;

		case 0x36:
			strcat(pStr, "(FS ) ");
			break;

		case 0x38:
			strcat(pStr, "(GYS) ");
			break;

		default:
			sprintf(temp, "(UNK) |%d, %d|", pMsg->m_pData[0] & 0x3f, nLen);
			strcat(pStr, temp);
			break;
	}

	return;

unknown:
	strcat(pStr, "UNK ");
	return;

bad:
	strcat(pStr, "BAD ");
}
#endif // LT_SERVER_BUILD


// 0x10023a8c — the post-accept hook: five bytes of `return 0`. Two
// arguments and `ret 8`, and nothing in the body, so the connection and
// the message are both unread. Whatever it was meant to do (a per-packet
// handler notification, on the evidence of where TranslatePacket calls it —
// twice, on both the local and the guaranteed path, each time treating a
// nonzero answer as "the packet has been consumed") was gone by the time
// this shipped.
//
// Defined last, after TranslatePacket: AvP2's cl 9044 reads the register use
// of a function compiled earlier in the same file, and seeing this one leave
// ecx alone it drops the `mov %ebx,%ecx` AvP2's server.dll reloads `this`
// with before the next call (0x1002b1d0).
int CNetMgrSendView::sub_10023a8c(NetConnection* pConn, ServerMessage* pMsg)
{
	return 0;
}



// 0x100235e8 — the internal packet handler, run before anything else looks
// at an arrived packet. Only packet id 1 is internal; everything else comes
// straight back out as zero so the caller hands it on. The first payload
// byte then selects one of five things (the names are the ones its own error
// string uses: "Error in SUB_ACK"):
//
//   0 — the acknowledgement. Optionally answers a ping request with a
//       sub-type 2 of its own, releases everything the far end has
//       acknowledged, and resends up to eight NAKed frames.
//   1 — a forced disconnect: the connection goes, the driver is told, and
//       the handler gets event 4 with the reason the packet carried.
//   2 — a ping reply. Only the sequence number the connection is waiting
//       for counts; the round trip goes into a three-deep ring and the ping
//       becomes their average.
//   3 — a group of packets, each length-prefixed, split back out into the
//       connection's receive list.
//   4 — one fragment of a split packet. Once all four have arrived the
//       original is rebuilt in place and the answer is zero, so the caller
//       hands the reassembled packet on.
//
// Everything except the completed reassembly answers one, meaning "handled;
// do not hand this packet out".
//
// Three readings worth writing down.
//
// The ping average is a shipped BUG: the three-sample ring is summed into
// pConn->m_Ping and then divided by the number of non-zero samples, but
// m_Ping is never cleared first — so it accumulates across every ping reply
// for the life of the connection and the number the stats line prints grows
// without bound. There is no store to +0x9c anywhere in the function, and
// AddConnection's reset is the only other one.
//
// The sub-type-4 arm opens with `RemoveConnection(LTNULL)`. The argument
// really is the zero the function has been carrying in edi since the
// prologue — nothing on the path to that arm loads the connection into it —
// so the walk runs with a null connection, which its own body survives but
// which cannot have been what was meant.
//
// And one codegen note: the "Invalid fragment count received!" arm ends with
// `cmp %edi,%ecx` where the shared exit block has `test %ecx,%ecx`. VC6
// cross-jumped that path into the *middle* of the reference's destructor,
// one instruction past its own test, using the zero already in edi to set
// the flags. Nothing in the source asks for that, and it reproduces.
//
// NEAR (1188 bytes against 1188, 406 instructions against 406, the same
// registers, the same frame and the same slot assignments): three bytes, one
// size-neutral schedule. The reassembly's `mov 0x54(%esi),%edx` (the
// destination packet's payload pointer) is hoisted above the two `movw $1`
// stores that the shipped build puts first (0x100239f5). Measured and
// negative: the stores through a CPacket view, the destination through
// `((CPacket*)pMsg)->m_Data.m_pData`, the stores in an inline member (a file-
// scope view class, and a local class, which /Ob1 does not expand), a walking
// CNetFragSlot pointer for the copy and the loop (+2 bytes), and volatile
// stores (this one keeps the load below the stores but moves the `lea` and the
// `add $0x14`). CNetMgr::WriteAckPacket has the same residual.
//
// The ping sum reads the ring through a named `float* pSamples` indexed by
// i. The evidence is 0x10023880 (`flds 0x9c(%esi) ; fadds (%edi)`) in server.dll
// and 0x00459142 (`flds (%ecx) ; fadds 0x9c(%esi)`) in lithtech.exe. The bare
// `m_UnknownA0[i]` gets lithtech right and server.dll commuted. A walking
// `*pSample++` gets server.dll right and lithtech commuted. Only the indexed
// pointer gets both. Using the same pointer for the shift loop above it is
// worse on both images.
//
// Everything else that was off has been closed and is worth recording,
// because each was a real source fact: the two length tests are *unsigned*
// (`(uint32)m_Unknown34 - (uint32)m_Unknown38`, giving `jb`/`jae` where an
// int difference gives `jl`/`jge`); the fragment-count byte is read once
// into a local and masked with `~0x0f`, not `0xf0`, and not read twice; the
// sub-type-3 length and the fragment replay length are both passed to
// ReadData/WriteData as *uint16* (the call sites push the raw dword their
// uint16 locals sit in, with no zero-extension, which a uint32 parameter
// would have forced); the two `movw $1` stores on the fresh ping packet
// share one load of `ref.m_pMsg` and so need a block-local pointer, while
// everything after them re-reads the reference; and `nValid` is assigned
// zero *after* the sample shift, or the zero VC6 is already holding in edi
// gets spilled to a frame slot and reloaded. All four revisions (8168,
// 8447, 8804, 8966) emit the same five bytes.
int CNetMgrSendView::sub_100235e8(NetConnection* pConn, ServerMessage* pMsg)
{
	CMessageRef ref;
#ifdef PRIMAL_HUNT
	// PH: a second reference, constructed (zeroed, EH state 1 -- server.dll
	// 0x1002d1cf..0x1002d1d6, lithtech.exe 0x0046a32a) and never assigned, so
	// VC6 drops its release from every exit.  AvP2 1.0.9.6 has no such slot
	// (`sub $0x50`).  (name ours)
	CMessageRef refUnused;
#endif
	uint32      nAcks[16];
	uint32      nAckID;
	uint32      nCount;
	uint32      i;
	uint8       nSubType;

	if ((pMsg->m_pData[0] & 0x3f) != 1)
		return 0;

	nSubType = pMsg->ReadByte(0);

	if (nSubType == 0)
	{
		((NetConnectionResetView*)pConn)->m_Unknown98 = 0.0f;

		if (pMsg->ReadByte(0))
		{
			uint16       nPingSeq = ((ServerMessageTypeView*)pMsg)->ReadWord(0);

			ref.Init(msg_AllocPacket(0x44c, 0x44c));

			ServerMessage* pNew = ref.m_pMsg;

			pNew->m_Unknown38 = 1;
			pNew->m_Unknown34 = 1;

			ref.m_pMsg->m_pData[0] = 1;

			ref.m_pMsg->WriteByte(2);
			ref.m_pMsg->WriteWord(nPingSeq);

			SendToConnection(ref.m_pMsg, pConn, 0);
		}

		nAckID = ((ServerMessageTypeView*)pMsg)->ReadDWord(0);

		((NetConnectionResetView*)pConn)->m_Unknown80 =
			((ServerMessageTypeView*)pMsg)->ReadDWord(0);

		sub_10023ad7(&pConn->m_GuaranteedQueue,
		             ((NetConnectionResetView*)pConn)->m_Unknown80);

		nCount = 0;

		while ((uint32)pMsg->m_Unknown34 - (uint32)pMsg->m_Unknown38 >= 4)
		{
			nAcks[nCount] = ((ServerMessageTypeView*)pMsg)->ReadDWord(0);

			nCount++;

			if (nCount >= 8)
				break;
		}

		if (nAckID != (uint32)-1)
		{
			sub_10023a91(&pConn->m_SendQueue, nAckID);

			CNetMgr::DebugPrint(pConn, 2, "Got an ack on local %d, remote %d",
				nAckID, ((NetConnectionResetView*)pConn)->m_Unknown80);
		}

		for (i = 0; i < nCount; i++)
		{
			CNetRecvPacket* pRec =
				(CNetRecvPacket*)FindQueuedPacket(&pConn->m_SendQueue, nAcks[i]);

			if (pRec)
			{
				if ((pRec->m_Unknown14 + i) % 5 == 0)
				{
					CNetMgr::DebugPrint(pConn, 1, "Resending frame %d", nAcks[i]);

					sub_10022c2f(pRec->m_pData, pRec->m_Len,
					             pRec->m_nAlloced - pRec->m_Len, pConn);
				}

				pRec->m_Unknown14++;
			}
			else
			{
				DebugPrint(1, "Error in SUB_ACK: missing requested packet %d",
					nAcks[i]);
			}
		}
	}
	else if (nSubType == 1)
	{
		CNetMgr::DebugPrint(pConn, 1, "Forced disconnect!");

		RemoveConnection(pConn);

		pConn->m_pDriver->Disconnect(pConn, 4);

		((CNetHandlerQ556*)m_pHandler)->Notify(4, g_EmptyString);
	}
	else if (nSubType == 2)
	{
		if (((ServerMessageTypeView*)pMsg)->ReadWord(0) ==
		        ((NetConnectionResetView*)pConn)->m_UnknownB8)
		{
			uint32 nValid;

			for (i = 0; i < 2; i++)
				((NetConnectionResetView*)pConn)->m_UnknownA0[i] =
					((NetConnectionResetView*)pConn)->m_UnknownA0[i + 1];

			((NetConnectionResetView*)pConn)->m_UnknownA0[2] =
				pConn->m_Counter.Read() * 0.001f;

			nValid = 0;

			float* pSamples = ((NetConnectionResetView*)pConn)->m_UnknownA0;

			for (i = 0; i < 3; i++)
			{
				((NetConnectionResetView*)pConn)->m_Ping += pSamples[i];

				if (pSamples[i] > 0.0f)
					nValid++;
			}

			if (nValid)
				((NetConnectionResetView*)pConn)->m_Ping /= nValid;
		}
	}
	else if (nSubType == 3)
	{
		while ((uint32)pMsg->m_Unknown34 - (uint32)pMsg->m_Unknown38 >= 2)
		{
			uint16 nSubLen = ((ServerMessageTypeView*)pMsg)->ReadWord(0);

			if (nSubLen <= pMsg->m_Unknown34 - pMsg->m_Unknown38)
			{
				CNetRecvPacket* pRec = sub_10023c4d(nSubLen);

				((ServerMessageTypeView*)pMsg)->ReadData(pRec->m_pData, nSubLen);

				pRec->m_Len = nSubLen;

				((LMessageList*)&pConn->m_RecvList)->Prepend((LThreadMessage*)pRec);
			}
			else
			{
				DebugPrint(0, "Invalid PACKETGROUP subpacket length");
			}
		}
	}
	else if (nSubType == 4)
	{
		uint8        nCountByte;
		uint8        nIndex;
		uint32       nFragID;
		CNetFragRec* pRec;

		RemoveConnection(LTNULL);

		nCountByte = pMsg->ReadByte(0);

		if ((nCountByte & ~0x0f) != 0x30)
		{
			CNetMgr::DebugPrint(pConn, 0, "Invalid fragment count received!");

			return 1;
		}

		nIndex = (uint8)(nCountByte & 0x0f);

		// packet.h's ReadType<uint32> spelled directly, like the WriteType of
		// the same id below. lithtech.exe expands both of them: see the
		// REPORT_ERROR pair at 0x004592ec/0x00459314, and the Insert2 call at
		// 0x00459400 inside the expanded write. The other reads and writes in
		// this function stay calls there. server.dll calls both either way.
		// lithtech NEAR, 1625 of 1622 bytes (it was 1374). Shipped declines
		// the Append inside the sub-type-3 Prepend (0x00459227 ->
		// 0x0045a1c0), and ours still expands it. The /O2 budget runs in
		// source order: a declared-only Append view frees budget, and the
		// Insert2 below then expands (1816). Writing the ping arm's header
		// through an inline helper expands its WriteType (1802). Spelling
		// the count byte as ReadType expands it (1723). So shipped spent more
		// before sub-type 3 than any spelling here does. The rest of the
		// diff is register allocation that follows from this.
		nFragID = ((CPacket*)pMsg)->ReadType((uint32*)0);

		pRec = sub_100233ad(nFragID, pConn);

		if (!pRec)
		{
			pRec = sub_10023495();

#ifdef AVP2_MODERN
			// No record: the fragment is dropped, like any packet the
			// reassembly cannot take.
			if (!pRec)
				return 1;

#endif
			pRec->m_ID    = nFragID;
			pRec->m_pConn = pConn;
		}

		sub_100233e3(pMsg, nIndex, pRec);

		if (sub_10023474(pRec))
		{
			CNetMgr::DebugPrint(pConn, 2, "Completed fragmented packet %d",
				pRec->m_ID);

			pMsg->m_Unknown38 = 1;
			pMsg->m_Unknown34 = 1;

			pMsg->m_pData[0] = pRec->m_pFrags[0].m_pMsg->m_pData[0];

			for (i = 0; i < 4; i++)
				((ServerMessageTypeView*)pMsg)->WriteData(
					pRec->m_pFrags[i].m_pMsg->m_pData + 1,
					pRec->m_pFrags[i].m_pMsg->m_Unknown34 - 1);

			((CPacket*)pMsg)->WriteType(nFragID);

			pMsg->m_Unknown38 = 1;

			ReleaseQueued((CNetPending*)pRec);

			return 0;
		}
	}
	else
	{
		CNetMgr::DebugPrint(pConn, 1, "Got an invalid internal packet!");
	}

	return 1;
}


// The rule these two settle, which decides four other loops in this file:
// cl 8447 reproduces the shipped *unrotated* loop — one test block that both
// the entry and the back edge reach — exactly when the back edge is an
// unconditional `jmp`, and rotates the loop (peeling the head test and
// duplicating it at the bottom) when the back edge is conditional. Both of
// these have an unconditional back edge and both match; FindQueuedPacket,
// sub_100233ad and sub_10023c4d end their bodies with a conditional
// break/continue, rotate, and are four-to-six-byte nears for it. No source
// shape moves them (while/continue, `for(;;)` with the test as a `break` or
// an early return, and a literal backward `goto` all give the rotated code),
// and neither does any held compiler revision.
//
// 0x10023a91 — release every record queued out of order whose id has now
// been overtaken. The queue is kept in id order, so the walk stops at the
// first record that belongs after the id handed in.
//
// The cursor-alias walk: the record in hand is taken out of the list before
// the cursor is advanced, so the advance reads the *record's* next pointer
// and re-reads the list's tail to decide whether it has come back round.
// The loop's back edge jumps to the `while` test rather than to the body,
// which is what says the test is shared and not duplicated.
void CNetMgrSendView::sub_10023a91(LMessageList* pList, uint32 nID)
{
	CNetRecvPacket* pCur = (CNetRecvPacket*)pList->m_pTail;
	CNetRecvPacket* pRec;

	while (pCur)
	{
		pRec = pCur;
		pCur = (pCur->m_pNext == (CNetRecvPacket*)pList->m_pTail) ?
		           LTNULL : pCur->m_pNext;

		if (pRec->m_ID > nID)
			break;

		pList->Remove((LThreadMessage*)pRec);

		FreeRecvPacket(pRec);
	}
}


// 0x10023ad7 — the same walk with the comparison the other way round: it
// releases everything queued *above* the id and keeps going to the end of
// the list rather than stopping. Because nothing breaks out of it, VC6
// rotates the loop (the test moves to the bottom) and shrink-wraps ebx into
// the body, which is the whole structural difference between the two.
void CNetMgrSendView::sub_10023ad7(LMessageList* pList, uint32 nID)
{
	CNetRecvPacket* pCur = (CNetRecvPacket*)pList->m_pTail;
	CNetRecvPacket* pRec;

	while (pCur)
	{
		pRec = pCur;
		pCur = (pCur->m_pNext == (CNetRecvPacket*)pList->m_pTail) ?
		           LTNULL : pCur->m_pNext;

		if (pRec->m_ID > nID)
		{
			pList->Remove((LThreadMessage*)pRec);

			FreeRecvPacket(pRec);
		}
	}
}


// 0x10023b1f — put a received guaranteed packet into the connection's
// out-of-order queue, in packet-id order. LTFALSE means the queue was full
// and the record was not taken.
//
// The walk runs from the tail with the usual cursor alias and stops at the
// first record whose id is above the new one, splicing in front of it; past
// the end of the queue the record goes on the front. The overflow test is
// written at both places — once at the head of the loop and once after it —
// but the report exists once: the in-loop test `break`s and the post-loop
// test carries the only DebugPrint, which is why the block sits between the
// post-loop test and the append. Writing the report (and its return) in both
// places is the same at /O1 but gives lithtech.exe (/O2) two copies of the
// tail (249 bytes against 219); the `break` spelling matches both images.
//
// The splice is LMessageList::InsertAfter expanded inline, the tail fix-up
// included; the append is a real call to Prepend. lthread.h has both, and
// this is the caller its note about the inline copy refers to.
//
// NEAR (150 bytes against 150, sixty-two instructions against sixty-two,
// the same frame and the same slots): two independent prologue stores are
// in the other order — the shipped build spills `this` to -0x8 before
// zeroing nCount at -0x4, ours zeroes first — and esi/edi are swapped
// throughout (ours holds the tail in esi and the cursor in edi, the shipped
// build the other way round). Nothing else differs. `nCount` has to be the
// first declaration with an initialiser or it lands in the dead pList
// parameter slot instead of a frame slot of its own, which was worth a byte
// and the whole slot layout; past that, all four revisions (8168, 8447,
// 8804, 8966) emit our order. Callee-saved rotation plus a store-order
// tie-break.
LTBOOL CNetMgrSendView::sub_10023b1f(LMessageList* pList, CNetRecvPacket* pRec)
{
	int             nCount;
	LThreadMessage* pTail;
	LThreadMessage* pCur;

	nCount = 0;
	pTail  = pList->m_pTail;
	// RE-READ THE MEMBER, do not copy pTail. Written `pCur = pTail;`
	// VC6 tests the tail and sinks the copy; the shipped bytes copy first
	// and test the copy. Proven in-tree by tcpdriver.cpp's tcp_FindConnection,
	// which reads the member twice and is byte-identical.
	pCur   = pList->m_pTail;

	while (pCur)
	{
		LThreadMessage* pAt;

		if (nCount >= g_NetMaxQueued)
			break;

		pAt  = pCur;
		pCur = (pCur->m_pPrev == pTail) ? LTNULL : pCur->m_pPrev;

		nCount++;

		if (pRec->m_ID < ((CNetRecvPacket*)pAt)->m_ID)
		{
			pList->InsertAfter(pAt, (LThreadMessage*)pRec);

			return LTTRUE;
		}
	}

	if (nCount >= g_NetMaxQueued)
	{
		DebugPrint(1, "Dropped guaranteed packet due to queue overflow");

		return LTFALSE;
	}

	pList->Prepend((LThreadMessage*)pRec);

	return LTTRUE;
}


// NEAR (106 of 106 bytes): an ebx/edi transposition, nothing else.
//
// 0x10023bb5 — a receive buffer goes back into the pool, which is kept in
// size order so a later request can take the smallest one that will do. The
// walk runs back from the largest and stops at the first buffer this one is
// not smaller than; running off the end means it is the smallest there is.
void CNetMgr::FreeRecvPacket(CNetRecvPacket* pRec)
{
	CNetRecvPacket* pTail;
	CNetRecvPacket* pCur;
	CNetRecvPacket* pAt = LTNULL;

	pRec->m_Len = 0;

	pTail = (CNetRecvPacket*)m_QueueB8.m_pTail;
	// RE-READ THE MEMBER, do not copy pTail. Written `pCur = pTail;`
	// VC6 tests the tail and sinks the copy; the shipped bytes copy first
	// and test the copy. Proven in-tree by tcpdriver.cpp's tcp_FindConnection,
	// which reads the member twice and is byte-identical.
	pCur = (CNetRecvPacket*)m_QueueB8.m_pTail;

	while (pCur)
	{
		pAt = pCur;
		pCur = (pCur->m_pNext == pTail) ? LTNULL : pCur->m_pNext;

		if (pRec->m_nAlloced >= pAt->m_nAlloced)
			break;
	}

	if (pCur)
	{
		((LMessageList*)&m_QueueB8)->InsertAfter((LThreadMessage*)pAt,
		                                         (LThreadMessage*)pRec);
	}
	else
	{
		((LMessageList*)&m_QueueB8)->Prepend((LThreadMessage*)pRec);
	}
}


// The queue is in sequence order, so a record past the one asked for says it
// is not there at all.
//
// NEAR (50 of 46 bytes): the shipped build folds the "not found" exit into
// the loop's own top test and reaches the loop back edge with the same
// compare that answers the two hits; ours rotates the loop, peels the first
// null test and pays a duplicated one at the bottom for it. Four source
// shapes give byte-for-byte the same rotated code: this while/continue/break
// form, the same with `break` spelt `return LTNULL`, `for(;;)` with the null
// test as an explicit early return, and a literal label with a backward
// `goto`. VC6 canonicalises the loop before it lays anything out, so the
// shape of the source is not the lever here.
void* CNetMgr::FindQueuedPacket(LMessageList* pList, uint32 nID)
{
	CNetRecvPacket* pTail = (CNetRecvPacket*)pList->m_pTail;
	// RE-READ THE MEMBER, do not copy pTail. Written `pCur = pTail;`
	// VC6 tests the tail and sinks the copy; the shipped bytes copy first
	// and test the copy. Proven in-tree by tcpdriver.cpp's tcp_FindConnection,
	// which reads the member twice and is byte-identical.
	CNetRecvPacket* pCur = (CNetRecvPacket*)pList->m_pTail;

	while (pCur)
	{
		CNetRecvPacket* pRec = pCur;

		pCur = (pCur->m_pNext == pTail) ? LTNULL : pCur->m_pNext;

		if (pRec->m_ID == nID)
			return pRec;

		if (pRec->m_ID > nID)
			break;
	}

	return LTNULL;
}


// 0x10023c4d — a receive record with room for nLen bytes, off the manager's
// free list at +0xb8 if one there is big enough and freshly allocated
// otherwise.
//
// Two extra bytes are always reserved (`inc %esi` twice, which is a byte
// shorter than `add $2`), and the length the record records is the reserved
// one, so a record's usable capacity is always two more than whatever asked
// for it. The comparison is signed on both sides — the record's own capacity
// field is an `int` — which is why the length arrives as one.
//
// The record and its payload are two separate allocations, both through
// `::operator new` (0x1000d25d, the forwarder in front of dalloc), and the record is
// cleared with memset rather than a constructor: the three
// calls' cleanups all batch into one `add $0x14,%esp`, so all three have
// fixed prototypes.
//
// The cursor re-reads the member instead of copying pTail, as sub_100233ad
// and tcp_FindConnection do. With `pCur = pTail`, VC6 tested the tail
// register, peeled that test off the loop and copied it to the back edge
// (117 bytes). The shipped code copies first and tests the copy
// (`mov %ecx,%eax ; test %eax,%eax`, 0x10023c5e). That test is the one both
// the entry and the `jl` back edge reach (0x10023c76), and the function is
// byte-identical. The earlier verdict here ("rotation, not reachable", with
// while/continue, for(;;), goto and four compiler revisions as negatives) was
// wrong. The `if (... < nLen) continue;` spelling also matches once the member
// is re-read. The lithtech twin (0x4597a0, 179 bytes) is byte-identical as
// well. It clears the record with an inline `rep stos` (0x4597e7): the same
// memset, expanded by /O2's /Oi, where server.dll's /O1 calls 0x1003e280.
CNetRecvPacket* CNetMgrSendView::sub_10023c4d(int nLen)
{
	CNetRecvPacketSendView* pRec;
	LThreadMessage*         pTail = m_QueueB8.m_pTail;
	LThreadMessage*         pCur  = m_QueueB8.m_pTail;

	nLen += 2;

	while (pCur)
	{
		pRec = (CNetRecvPacketSendView*)pCur;
		pCur = (pCur->m_pPrev == pTail) ? LTNULL : pCur->m_pPrev;

		if (pRec->m_nAlloced >= nLen)
		{
			((LMessageList*)&m_QueueB8)->Remove((LThreadMessage*)pRec);

			pRec->m_Unknown14 = 0;
			pRec->m_bPending  = 0;

			return pRec;
		}
	}

	pRec = (CNetRecvPacketSendView*)::operator new(sizeof(CNetRecvPacketSendView));

	memset(pRec, 0, sizeof(CNetRecvPacketSendView));

	pRec->m_pData    = (uint8*)::operator new(nLen);
	pRec->m_nAlloced = nLen;

	m_UnknownD8 += nLen;

	return pRec;
}


// The trace line carries the elapsed counter and whatever name the update is
// running under, so a client and a server log interleave readably. Quiet
// while the connection is still coming up unless the console asks for
// everything.
void CNetMgr::DebugPrint(NetConnection* pConn, int nLevel, char* pFormat, ...)
{
	char    msg[500];
	va_list marker;

	if (nLevel > g_NetDebugLevel)
		return;

	if (!g_NetDebugAll && (pConn->m_Flags & 1))
		return;

	va_start(marker, pFormat);
	_vsnprintf(msg, sizeof(msg) - 1, pFormat, marker);

	dsi_ServerError("(%d) %s%s", NET_TRACE_TIME(), m_pTraceName, msg);
}
