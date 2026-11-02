// server.dll — CLocalDriver, the loopback driver (reconstruction).
//
// Build /Os WITHOUT /Og (build/engine.sh's OS_TUS). The whole file says so,
// not just Disconnect: every `this` access is reloaded from its spill slot,
// no common subexpression is ever shared, and no constant is propagated (the
// deleting destructor's flag argument arrives as a live `push $1; pop %eax;
// and $1,%eax`) — while the size peepholes /Os brings are all present
// (`push $imm`/`pop reg` for small constants, `andl $0,mem` for a zero
// store, `orl $-1,mem` for a -1 store, `pop ecx` per pushed argument).
//
// lithtech.exe builds the SAME file /Od (OD_TUS) -- and with /Oi. The /Od
// half is not in doubt (`push %ebp; mov %esp,%ebp` everywhere, `this`
// spilled to -0x4(%ebp) and reloaded before every store, `int3` padding
// where the rest of the image pads with `nop`), and /Oi is forced by three
// separate intrinsic expansions inside otherwise textbook /Od bodies:
// `strcpy` in Init (0x0043e57b), `memset` in the constructor (0x0043e452)
// and in _p78 (0x0043ea81), `memcpy` in ReceivePacket (0x0043e8b8) and _p78
// (0x0043eb26). /Od alone cannot produce those, so OD_FLAGS needs /Oi.
//
// DO NOT re-chase the twenty /O2 gap rows for this file. Seventeen of them are
// phantom: sixteen are CLocalDriverBase's in-class stubs, whose COMDATs the
// client's linker took from a /O2 TU -- the shipped CLocalDriver vtable at
// 0x004a244c points them at 0x0042d180/190/1a0/1b0/1c0 and 0x00432030/040,
// five-byte `xor %eax,%eax`/`mov $1,%eax` bodies with `nop` padding that no
// /Od object can ever supply -- and the seventeenth, ??_GCLocalDriverBase,
// has no address in lithtech.exe at all. The real rows are three: the
// constructor (0x0043e3e0), Init (0x0043e560) and ReceivePacket
// (0x0043e860), and all three are the intrinsics above.
//
// One stub does come out of this TU in the client, and it is the tell: slot
// 26 (GetRecvSize) points at 0x0043e4e0, a thirteen-byte /Od body inside the
// localdriver block, while slots 16 and 19 -- IsLobbyLaunched and _p4c, the
// same `return 0` with the same `ret` -- point at the foreign 0x00432040.
// Folding cannot separate identical bodies, so the shipped CLocalDriver
// OVERRIDES GetRecvSize in this .cpp rather than inheriting it. The bytes
// are the same either way and the address already matches, so it is
// recorded and not acted on.
#include "servermgr.h"
#include "servermsg.h"

// 0x10014d94 — the sub-object the driver carries two of (at +0x5c and
// +0x78). Its constructor clears three of its seven dwords, and not in
// declaration order (0x18, then 0x00, then 0x0c), so it is a body and not a
// member-initialiser list. Everything else about the record is written by
// the driver's own constructor below (names ours).
//
// The same record serves as the queue HEAD and as each queued packet: the
// driver's teardown walks m_pNext from the head at +0x5c until it comes back
// to the head, and destroys everything in between.
class CLocalQueue
{
public:
	CLocalQueue();                           // 0x10014d94
	~CLocalQueue();                          // 0x10014db4

	uint32       m_Unknown00;                // +0x00
	// Signed: both length compares in the free-list search are `jl`/`jle`.
	int          m_Len;                      // +0x04  the payload's capacity
	uint32       m_Unknown08;                // +0x08
	void*        m_pData;                    // +0x0c  released by ~CLocalQueue
	CLocalQueue* m_pRecvPrev;                // +0x10
	CLocalQueue* m_pRecvNext;                // +0x14
	CLocalQueue* m_pNext;                    // +0x18
};

CLocalQueue::CLocalQueue()
{
	m_pNext = LTNULL;
	m_Unknown00 = 0;
	m_pData = LTNULL;
}

// 0x10014db4 — the payload goes back to the allocator. Not dfree: 0x10028ace
// is the operator-delete forwarder in front of it.
CLocalQueue::~CLocalQueue()
{
	if (m_pData)
	{
		void* pData = m_pData;
		::operator delete(pData);
		m_pData = LTNULL;
	}
}

// The base's default bodies are server.dll's alone. In lithtech.exe the
// CLocalDriver vtable (0x004a244c) points every inherited default at the
// client's /O2 copies -- 0x0042d180/190/1a0/1b0/1c0 (clientworld.cpp's
// CBaseDriverDefaultsView), 0x00432030, 0x00432040 and 0x00459f70 -- and no
// slot at a body of this /Od translation unit, so the client build only
// DECLARES them and src/engine/lithtech/link/aliases.rsp resolves each one
// onto that copy. Compiled here they were eighteen /Od COMDATs (sixteen of
// them 15-18 bytes of frame around a one-instruction body) that the
// shipped client does not have.
#ifdef LT_SERVER_BUILD
#define LDB_RETURN(r)   { return r; }
#define LDB_EMPTY       {}
#else
#define LDB_RETURN(r)   ;
#define LDB_EMPTY       ;
#endif

// The base half of the driver, spelled out here rather than reached through
// servermgr.h's CBaseDriver: that declaration is shared with the client half
// and has eight of the twenty-seven slots as argument-less `_pNN` stubs,
// where the local driver's slots 0x10 and 0x60 take arguments. Same 27 slots
// and same 0x50-byte layout. What the constructor needs from it besides the
// slots is the base's *body* — the two stores the shipped code inlines ahead
// of the derived vptr: CBaseDriver's own m_DriverFlags (+0x44) and
// m_Timeout (+0x4c).
class CLocalDriverBase
{
public:
	CLocalDriverBase() { m_Timeout = 120.0f; m_DriverFlags = 0; }
	virtual ~CLocalDriverBase() {}

	// Pure: the base's own vtable (0x1004d870) has _purecall (0x1003ea24)
	// in slots 1-3 and 23-26 -- Init, Term, Update, Disconnect, SendPacket,
	// ReceivePacket and GetRecvSize. Declared-but-undefined, they left the
	// base vtable referencing seven symbols nothing defines.
	virtual LTBOOL   Init() = 0;                                 // 0x04
	virtual void     Term() = 0;                                 // 0x08
	virtual void     Update() = 0;                               // 0x0c
	// 0x10014ebe — the base's default Connect does nothing. It is not in
	// CLocalDriver's vtable (0x1004d7f0 overrides it with 0x10015193) but it
	// is slot 4 of the base's own table (0x1004d880) and of the TCP driver's
	// (0x1004e95c), which inherits it: `ret $0x4`, one argument, no result.
	virtual void     Connect(CBaseDriver* pDriver)          LDB_EMPTY

	// The base's DEFAULTS, and the eight bodies behind them (0x10014ebe -
	// 0x10014f16). This class declared every slot and defined none, so all
	// eight were missing; CLocalDriver's vtable at 0x1004d7f0 is what maps
	// them, and every argument count agrees with the declaration here, which
	// is what makes the assignment safe.
	//
	// The pattern is uniform: a slot that answers LTRESULT defaults to
	// LT_ERROR (`push $1; pop %eax`) -- "this driver does not do that" --
	// and one that answers LTBOOL or a count defaults to zero.
	//
	// GetServiceList is LTRESULT, not the void this file had: slot 0x14
	// takes one argument and sets %eax to 1, and tcpdriver.cpp's view of the
	// same base already declares it LTRESULT.
	virtual LTRESULT GetServiceList(NetService** ppList)  LDB_RETURN(LT_ERROR)
	virtual int      SelectService(HNETSERVICE hService)  LDB_RETURN(1)
	virtual LTRESULT _p1c(void* a, void* b)               LDB_RETURN(LT_ERROR)
	virtual LTRESULT _p20(void* a)                        LDB_RETURN(LT_ERROR)
	virtual LTRESULT _p24()                               LDB_RETURN(LT_ERROR)
	virtual LTRESULT _p28(void* a)                        LDB_RETURN(LT_ERROR)
	virtual LTRESULT _p2c()                               LDB_RETURN(LT_ERROR)
	virtual LTRESULT HostSession(NetHost* pHostInfo)      LDB_RETURN(LT_ERROR)
	virtual LTRESULT _p34(void* a)                        LDB_RETURN(LT_ERROR)
	virtual LTRESULT UpdateSessionName(const char* sName) LDB_RETURN(LT_ERROR)
	virtual LTRESULT GetSessionName(char* sName, uint32 nLen)
	                                                      LDB_RETURN(LT_ERROR)
	virtual LTBOOL   IsLobbyLaunched()                    LDB_RETURN(LTFALSE)
	virtual LTBOOL   GetLobbyLaunchInfo(void** ppData)    LDB_RETURN(LTFALSE)
	virtual LTRESULT _p48(void* a)                        LDB_RETURN(LT_ERROR)
	virtual LTBOOL   _p4c()                               LDB_RETURN(LTFALSE)
	virtual LTBOOL   GetTcpIpAddress(char* sAddress, uint32 dwBufferSize,
	                                 unsigned short& hostPort)
	                                                      LDB_RETURN(LTFALSE)
	virtual LTBOOL   _p54(void* a)                        LDB_RETURN(LTFALSE)
	virtual LTRESULT SendTo(const void* pData, uint32 len,
	                        const char* sAddr, uint32 port)
	                                                      LDB_RETURN(LT_ERROR)
	virtual void     Disconnect(NetConnection* pConn, uint32 nReason,
	                            uint32 flags) = 0;               // 0x5c
	virtual LTBOOL   SendPacket(void* pData, uint32 nLen, uint32 nUnused,
	                            NetConnection* pConn) = 0;       // 0x60
	virtual LTBOOL   ReceivePacket(class ServerMessage* pMsg) = 0;  // 0x64
	virtual uint32   GetRecvSize() = 0;                          // 0x68

public:
	// 0x10014fb0 writes "local" here, and the TCP driver's constructor
	// strncpy's "internet" into the same field.
	char           m_Name[0x44 - 0x04];      // +0x04
	uint32         m_DriverFlags;            // +0x44
	class CNetMgr* m_pNetMgr;                // +0x48
	float          m_Timeout;                // +0x4c
};

// The loopback driver. Its own four extra virtuals sit past CBaseDriver's
// twenty-seven, at 0x6c..0x78, and every one of them is reached only on the
// PEER driver at +0x54 — the two ends of a loopback pair each hold the
// other (names ours).
class CLocalDriver : public CLocalDriverBase
{
public:
	CLocalDriver();                          // 0x10014de0
	virtual ~CLocalDriver();                 // 0x10014f77

	virtual LTBOOL   Init();                 // 0x04  0x10014fb0
	virtual void     Term();                 // 0x08  0x10014fcf
	virtual void     Update();               // 0x0c  0x10015101
	virtual void     Connect(CBaseDriver* pDriver);  // 0x10  0x10015193
	virtual void     Disconnect(NetConnection* pConn, uint32 nReason,
	                            uint32 flags);       // 0x5c  0x100151c1
	virtual LTBOOL   SendPacket(void* pData, uint32 nLen, uint32 nUnused,
	                            NetConnection* pConn);  // 0x60  0x100151f0

	virtual void     _p6c(CLocalDriver* pPeer);  // 0x6c  0x100152d6
	virtual LTBOOL   ReceivePacket(class ServerMessage* pMsg);  // 0x64  0x10015222
	// 0x68 -- the override the header comment describes: slot 26 of
	// 0x1004d7f0 is the shared `return 0` (0x10014ef1), the base's is
	// _purecall.
	virtual uint32   GetRecvSize()           { return 0; }
	virtual void     _p70();                 // 0x70  0x100152ef
	virtual void     _p74(CLocalDriver* pPeer);  // 0x74  0x10015316
	virtual void     _p78(void* pData, int nLen);     // 0x78  0x10015379

	uint32         m_Unknown50;              // +0x50  set by the peer, cleared
	                                         //        by the next Update
	CLocalDriver*  m_pPeer;                  // +0x54  the other end
	NetConnection* m_pConn;                  // +0x58
	CLocalQueue    m_Queue5C;                // +0x5c  the received-packet list
	CLocalQueue    m_Queue78;                // +0x78
	uint32         m_Unknown94;              // +0x94  bytes held in m_Queue5C
	uint32         m_Unknown98;              // +0x98
	uint32         m_Unknown9C;              // +0x9c
};

// NetConnection is 0xbc bytes, not the 0xb8 servermgr.h's declaration adds
// up to: 0x10015316 allocates 0xbc for one. The missing dword sits after
// m_UnknownB4, and servermgr.h is shared with the client half and READ-ONLY
// here, so the local driver's allocation goes through a derived view.
class CLocalNetConnection : public NetConnection
{
public:
	// Declared, never defined: 0x10021200 is NetConnection's own
	// constructor, which is what the shipped `new` calls.
	CLocalNetConnection();

	uint32 m_UnknownB8;                      // +0xb8
};

// What 0x1002126a really is. servermgr.h declares it `NetConnection::Term`,
// but the local driver's two `delete` sites reach it as the DESTRUCTOR: each
// expands the compiler's deleting destructor inline and the direct call
// inside it lands on that address. servermgr.h is shared with the client
// half and left alone, so the delete goes through this one-line view.
class NetConnectionD
{
public:
	~NetConnectionD();                       // 0x1002126a
};

// 0x1003e280 is plain `memset` -- the CRT's, not an engine helper. The early
// return at its tail reads `mov 0x8(%esp),%eax` only because `edi` has been
// pushed by then, so that slot is the FIRST argument; both exits hand back
// the destination. Spelling it `memset` is what lets the client's /Oi expand
// it to `rep stos` (0x0043e452 and 0x0043ea81) while server.dll, built /Os
// with no /Oi, still emits the call the shipped bytes show.

// 0x10014de0 — build the loopback driver. The base's 120-second idle timeout
// is overwritten with 1000000.0f, which is the whole of what makes a local
// connection never time out.
CLocalDriver::CLocalDriver()
{
	m_Queue78.m_pRecvNext = &m_Queue78;
	m_Queue78.m_pRecvPrev = m_Queue78.m_pRecvNext;

	memset(&m_Queue5C, 0, sizeof(m_Queue5C));

	m_Queue5C.m_pNext = &m_Queue5C;
	m_Queue5C.m_Unknown08 = (uint32)-1;
	m_Queue5C.m_Len = -1;
	m_Queue5C.m_Unknown00 = 2;

	m_Unknown9C = 0;
	m_Unknown98 = 0;
	m_Unknown94 = 0;

	m_pPeer = LTNULL;
	m_pConn = LTNULL;

	m_Timeout = 1000000.0f;
	m_Unknown50 = 0;
}

// 0x10014f77 — the teardown is the whole of the destructor body; the two
// queue members and the base's empty destructor supply the rest.
CLocalDriver::~CLocalDriver()
{
	Term();
}

// 0x10014fb0 — slot 0x04. strcpy, not strncpy: the name is a literal and
// the buffer is the driver's own.
LTBOOL CLocalDriver::Init()
{
	strcpy(m_Name, "local");
	return LTTRUE;
}

// 0x10014fcf — slot 0x08. Everything the driver holds goes: the queued
// packets (with their byte count), the two list heads back to empty, the
// peer (which is told, after the link is broken, so it cannot come back
// through this driver), and the connection.
void CLocalDriver::Term()
{
	CLocalQueue* pCur = m_Queue5C.m_pNext;

	while (pCur != &m_Queue5C)
	{
		CLocalQueue* pRec = pCur;
		pCur = pCur->m_pNext;

		m_Unknown94 -= pRec->m_Len;

		delete pRec;
	}

	m_Queue5C.m_pNext = &m_Queue5C;
	m_Queue78.m_pRecvNext = &m_Queue78;
	m_Queue78.m_pRecvPrev = m_Queue78.m_pRecvNext;

	m_Unknown9C = 0;
	m_Unknown98 = 0;

	if (m_pPeer)
	{
		CLocalDriver* pPeer = m_pPeer;
		m_pPeer = LTNULL;
		pPeer->_p70();
	}

	if (m_pConn)
	{
		delete (NetConnectionD*)m_pConn;
		m_pConn = LTNULL;
	}
}

// 0x100584c4 — whether a local connection announces itself as one (name
// ours): with it off the connection's flag word gets bit 0 set before the
// manager is told about it.
extern int g_LocalNoFlag;

// 0x10015101 — slot 0x0c. One deferred connect per frame: the flag the peer
// set is cleared, the connection is handed to the manager, and a manager
// that refuses it leaves everything to the teardown below.
void CLocalDriver::Update()
{
	if (m_Unknown50)
	{
		m_Unknown50 = 0;

		if (!g_LocalNoFlag)
			m_pConn->m_Flags = 1;

		if (m_pNetMgr->AddConnection(m_pConn))
			return;

		delete (NetConnectionD*)m_pConn;

		m_pConn = LTNULL;
		m_pPeer = LTNULL;
	}
}

// 0x10015193 — slot 0x10. Tie two loopback drivers together: the peer is
// told about this end first, then this end takes the peer.
void CLocalDriver::Connect(CBaseDriver* pDriver)
{
	CLocalDriver* pPeer = (CLocalDriver*)pDriver;

	pPeer->_p6c(this);
	_p74(pPeer);
}

// 0x100151c1 — slot 0x5c.
void CLocalDriver::Disconnect(NetConnection* pConn, uint32 nReason, uint32 flags)
{
	con_Printf("CLocalDriver::Disconnect\n");

	m_pNetMgr->CloseConnection(pConn);
	Term();
}

// 0x100151f0 — slot 0x60. The send is the peer's receive; with no peer there
// is nothing to send to. The third and fourth parameters are unread.
LTBOOL CLocalDriver::SendPacket(void* pData, uint32 nLen, uint32 nUnused,
                                NetConnection* pConn)
{
	if (m_pPeer)
	{
		m_pPeer->_p78(pData, nLen);
		return LTTRUE;
	}

	return LTFALSE;
}

CBaseDriver* ldrv_Create()
{
	return (CBaseDriver*)new CLocalDriver;
}


// 0x1003ed10 is the CRT's `memcpy` (the aligned/unaligned jump-table body).
// It was previously declared locally to keep the intrinsic from expanding it
// -- correct for server.dll's /Os, but the client's copy of this TU is built
// with /Oi and the shipped bytes DO expand it, at 0x0043e8b8 and 0x0043eb26.
// Under /Os with no /Oi the spelling below still emits the call, and the
// relocation is masked, so server.dll is unaffected either way.

// 0x10015222 — slot 0x64. Take the record at the head of the arrived list,
// copy it into the caller's message, unlink it and mark it free again. The
// three preconditions are AND-merged, so the last one jumps into the body.
LTBOOL CLocalDriver::ReceivePacket(ServerMessage* pMsg)
{
	CLocalQueue* pRec = m_Queue78.m_pRecvNext;

	if (!m_pConn || pRec == &m_Queue78 || m_Unknown50)
		return LTFALSE;

	{
		uint8* pData = pMsg->m_pData;
		memcpy(pData, pRec->m_pData, pRec->m_Unknown08);
	}

	pMsg->m_Unknown34 = (uint16)pRec->m_Unknown08;
	pMsg->m_pConnection = m_pConn;

	pRec->m_pRecvPrev->m_pRecvNext = pRec->m_pRecvNext;
	pRec->m_pRecvNext->m_pRecvPrev = pRec->m_pRecvPrev;

	pRec->m_Unknown00 &= 0xfffffffe;

	m_Unknown98--;
	return LTTRUE;
}

// 0x100152d6 — slot 0x6c. The peer's half of Connect, and nothing but a
// forward.
void CLocalDriver::_p6c(CLocalDriver* pPeer)
{
	_p74(pPeer);
}

// 0x100152ef — slot 0x70. What the other end's teardown calls: drop the
// connection through the driver's own Disconnect so the manager hears about
// it too.
void CLocalDriver::_p70()
{
	if (m_pConn)
		Disconnect(m_pConn, 1, 0);
}

// 0x10015316 — slot 0x74. Take the peer and build the connection that rides
// it. Nothing happens if this end already has a peer.
void CLocalDriver::_p74(CLocalDriver* pPeer)
{
	if (!m_pPeer)
	{
		m_pConn = new CLocalNetConnection;
		m_pConn->m_pDriver = (CBaseDriver*)this;

		m_pPeer = pPeer;
		m_Unknown50 = 1;
	}
}

// 0x10015379 — slot 0x78. The peer's send, seen from the receiving end: find
// a spare record big enough, build one if there is none, copy the payload in
// and put the record on the arrived list.
//
// The spare list at +0x5c is kept in descending capacity order, which is why
// the search takes the first record that fits and the insert walks until the
// new length is no longer bigger than the next record's.
void CLocalDriver::_p78(void* pData, int nLen)
{
	// Byte-identical on both images (0x10015379 /Os, 0x0043e9f0 /Od /Oi).
	// What was wrong was the frame map, and it took two separate facts.
	//
	// VC6 does not lay named locals out in declaration order: it walks its
	// symbol table, so the SPELLING of the names picks the slots. With the
	// statements fixed, renaming the record pointer alone moves it through
	// every position in the named run (pA..pH, pL..pX last; pJ/pZ second;
	// pK third; pI/pY first -- swept with the old four-local body). pHolder is one of the few ordinary words that
	// lands it at -0x4 above pCur and pTail, as shipped; pRec, pPacket,
	// pQueue, pNode, pEntry and ~40 others do not. (pRec/pIter/pTop,
	// pRec/pNext/pTop and pRec/pPrev/pTop also match.) The shipped names are
	// unknowable; these just hash the same way.
	//
	// And the payload is `new uint8[nLen]`, not a named void* from
	// `::operator new` -- the array new-expression makes a TEMP for its
	// result, and temps are laid out after every named local in order of
	// appearance, which is what puts the payload slot (-0x14) below the
	// `new CLocalQueue` temp (-0x10). No named local can land there, in
	// any scope. Same call either way: `new uint8[]` is ??2 under VC6.
	CLocalQueue* pHolder;
	CLocalQueue* pCur = m_Queue5C.m_pNext;
	CLocalQueue* pTail;

	pHolder = LTNULL;

	while (pCur != &m_Queue5C)
	{
		if (!(pCur->m_Unknown00 & 3))
		{
			if (pCur->m_Len >= nLen)
			{
				pHolder = pCur;
				break;
			}
		}

		pCur = pCur->m_pNext;
	}

	if (!pHolder)
	{
		pHolder = new CLocalQueue;

		memset(pHolder, 0, sizeof(CLocalQueue));

		pHolder->m_Len = nLen;

		// 0x1000d25d is `::operator new`, the one-argument forwarder in
		// front of dalloc at 0x1000d2ef.
		pHolder->m_pData = new uint8[nLen];

		m_Unknown9C++;
		m_Unknown94 += nLen;

		pCur = &m_Queue5C;
		for (;;)
		{
			if (nLen <= pCur->m_pNext->m_Len || pCur->m_pNext == &m_Queue5C)
			{
				pHolder->m_pNext = pCur->m_pNext;
				pCur->m_pNext = pHolder;
				break;
			}

			pCur = pCur->m_pNext;
		}
	}

	memcpy(pHolder->m_pData, pData, nLen);
	pHolder->m_Unknown08 = nLen;

	pTail = m_Queue78.m_pRecvPrev;

	pHolder->m_pRecvNext = pTail->m_pRecvNext;
	pHolder->m_pRecvPrev = pTail;
	pTail->m_pRecvNext = pHolder;
	pHolder->m_pRecvNext->m_pRecvPrev = pHolder;

	pHolder->m_Unknown00 |= 1;

	m_Unknown98++;
}
