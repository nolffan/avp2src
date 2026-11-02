// server.dll — TCPDriver, the TCP/IP network driver (reconstruction). Build /O1.
//
// Three of this file's functions are named by surviving check literals —
// StartQuery, HostSession and JoinSession — and the rest were named from the
// driver's vtable at 0x1004e94c, which is CBaseDriver's twenty-seven slots
// (servermgr.h) in order: TCPDriver adds no virtuals of its own, so every
// anonymous slot has a signature the SDK already declares. Every one of
// those twenty-seven slots now has a body here.
//
// Two allocator facts, both read off the call targets: 0x1000d25d is the
// one-argument `::operator new` forwarder and 0x1000d2ef is `dalloc` behind
// it, and every allocation in this file goes to the former.
//
// winsock2.h leads, and the order is not cosmetic: the shipped calls go
// through the import table (`call [__imp__htons@4]`, six bytes), which is
// what winsock2.h's WINSOCK_API_LINKAGE expands to once windows.h has
// defined DECLSPEC_IMPORT. Plain winsock.h — which is what windows.h pulls
// in by itself — declares the same functions without dllimport and gives a
// five-byte direct call to a linker thunk instead. Including winsock2.h
// first also stops windows.h reaching for winsock.h and colliding with it.
#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

// The same renames lthread.h undoes, undone one header earlier: with windows.h
// reached from here rather than from inside servermgr.h, server_interface.h is
// parsed while CopyFile still means CopyFileA and ServerInterface's pure
// virtual stops matching MyServerInterface's override.
#undef PostMessage
#undef CopyFile
#undef PlaySound
#undef GetClassName

// packet.h for CPacket::ReadType/WriteType, which lithtech.exe expands at
// four sites below. It has to come BEFORE servermgr.h: included after
// de_error.h it flips HandleConnPacket (0x10036ee5) to its reload-swap near
// (measured), the header-order sensitivity described at TCPMsgView.
#include "packet.h"
#include "servermgr.h"
#include "servermsg.h"
#include "de_error.h"

#define SOL_SOCKET     0xffff
#define SO_BROADCAST   0x0020

// The port the driver falls back to when the caller asks for none.
#define TCP_DEFAULTPORT 27888

// Set from the console: every "IP: ..." trace in the driver is gated on it
// (0x10058494, name ours).
extern int g_ShowNetInfo;

// The driver's own helpers (0x10036586, 0x100365c9, 0x10036002; names ours).
// MakeAddr fills a bind address for a port, OpenSocket binds a datagram socket
// and hands back the address it actually got, RecvFrom reads one datagram.
class TCPDriver;
int    tcp_MakeAddr(struct sockaddr_in* pAddr, unsigned short nPort);
SOCKET tcp_OpenSocket(TCPDriver* pDriver, struct sockaddr_in* pBindAddr,
                      struct sockaddr_in* pBoundAddr);
int    tcp_RecvFrom(TCPDriver* pDriver, SOCKET hSocket, void* pData, uint32 maxLen,
                    struct sockaddr_in* pFrom);

// The query-response record the driver fills in per host it asks (0xa4
// bytes; the constructor at 0x1003654a zeroes the first 0x80 and the four
// fields after it, names ours). The destructor is inline — the shipped code
// carries the free at every one of StartQuery's returns. The free is
// `::operator delete` (0x10028ace), not dfree, like every other release of
// m_pData in this file.
class TCPQueryResponse
{
public:
	// In-class, because lithtech.exe's /O2 build expands it wherever a
	// response is declared (StartQuery's `rep stos` at 0x00485082,
	// UpdateQuery's at 0x004855c1) and keeps the out-of-line copy
	// (0x00487df0) only for the array allocator; server.dll's /O1 calls it.
	TCPQueryResponse()                       // 0x1003654a
	{
		m_pData = LTNULL;                    // +0x98 -- shipped stores this
		m_Unknown9c = 0.0f;                  // +0x9c    one FIRST
		m_Unknown80 = 0;
		memset(m_Text, 0, sizeof(m_Text));
		m_UnknownA0 = 0.0f;
		m_Time = 0.0f;
	}
	~TCPQueryResponse() { if (m_pData) ::operator delete(m_pData); }
#ifndef LT_SERVER_BUILD
	void operator=(const TCPQueryResponse& other);   // lithtech 0x004853c0
#endif

	char               m_Text[0x80];         // 0x00
	uint32             m_Unknown80;          // 0x80  0xff for a broadcast sweep
	float              m_Time;               // 0x84  when the request went out
	struct sockaddr_in m_Addr;               // 0x88
	uint8*             m_pData;              // 0x98
	float              m_Unknown9c;          // 0x9c
	float              m_UnknownA0;          // 0xa0
};

// The packet's in-class accessors, as this file sees them (names ours).
// servermsg.h's ServerMessage exposes the fields bare, and that header is
// shared by every server TU, so the three inlines live on a view of the same
// object rather than on the class itself. They are not decoration: each is a
// measured source fact.
//
//   * GetData/GetDataSize: both tcp_RecvFrom call sites in ReceivePacket and
//     UpdateQuery materialise the buffer and its size into registers before
//     pushing (`mov 0x58(%esi),%eax / mov 0x54(%esi),%ecx / push %eax / push
//     %ecx` at 0x1003733f), which is what an inline accessor's return value
//     does; the bare members fold into `push 0x58(%esi)`. Taking them made
//     ReceivePacket byte-identical on both images and moved UpdateQuery 29
//     bytes closer.
//   * Reset: UpdateQuery's re-send loop stores 0x38 then 0x34 through ONE
//     load of msgOut.m_pMsg (0x10036773 .. 0x10036783). Two statements
//     reload the pointer between the stores (msgOut's address is taken, so
//     a word store may alias it); inside an inline the object is the
//     inline's own `this`, held in a register. The same change handed the
//     shared zero to %edi and the 0x1000 packet size to %esi, which is the
//     whole-function allocation the shipped build has.
//
// Declaring these as free inline functions instead flips HandleConnPacket
// (0x10036ee5) from identical to a two-instruction reload swap -- measured
// with two, three, four and five unused free inlines at the top of the
// file, and not with one, seven or eight -- so keep them as members here.
class TCPMsgView
{
public:
	uint8*  GetData()     { return ((ServerMessage*)this)->m_pData; }
	uint32  GetDataSize() { return ((ServerMessage*)this)->m_DataSize; }
	void    Reset()
	{
		((ServerMessage*)this)->m_Unknown38 = 1;
		((ServerMessage*)this)->m_Unknown34 = 1;
	}
};

// 0x10035efb — parse "a.b.c.d[:port]" into an address.
int tcp_BuildSockaddrFromString(char* pString, struct sockaddr_in* pAddr);

// The driver's response array, at +0x50: the collection of query responses.
// It is stdlith's `CMoArray<TCPQueryResponse>` — vptr, m_pArray, m_nElements
// and DefaultCache's two words, 0x14 bytes — and the three shipped call
// sites say so: StartQuery appends with `Insert2(m_nElements, resp,
// &g_MoAlloc)`, and both the query teardown and the destructor empty it with
// `SetSize2(0, &g_MoAlloc)`. Insert2 and the constructor live above the
// app-region cut (0x100380c9, 0x10037c4c), so they are declared, not
// defined; only the LAYOUT and the call shapes matter for them.
//
// SetSize2 IS DEFINED, as a template, because lithtech.exe needs its body.
// server.dll (/O1) calls it out of line from both ~TCPDriver (0x10036118) and
// EndQuery (0x10036c0a), but lithtech.exe (/O2) expands it into both:
// ~TCPDriver (0x00484cf0) carries SetSize2 and _DeleteAndDestroyArray inline
// and CALLS BaseDelete (0x00488170, `push n+cache; push p; push
// $g_DefAlloc`), while EndQuery (0x00485c40), one level shallower (Term
// straight into SetSize2), expands BaseDelete too and calls the element
// destructor as `push $0 ; call 0x00488230` -- the ??_G(0) VC6 emits for an
// explicit destructor call it declines to expand. Three things are needed
// for VC6's /O2 inline budget to stop at exactly those two places, and each
// was measured:
//   * the class is a TEMPLATE with SetSize2 / _DeleteAndDestroyArray out of
//     class, as CMoArray's are. (server.dll's /Os kept SetSize2 a call in
//     every form tried, in-class included, so it constrains nothing here.)
//   * the body is collection.h's CMoArray one verbatim, accessors and all:
//     `GetNumAllocatedElements()`, a `DefaultCache m_Cache` member and its
//     Get/SetCacheSize calls. With the fields read directly VC6 had budget
//     left and expanded everything down to the element destructor (153 and
//     112 bytes against 94 and 102).
//   * the destructor is `Term()`, as CMoArray's is, not a direct SetSize2.
// SetSize2's allocating arm is spelled with LNew_Array rather than through
// the _AllocateTArray member, which is DECLARED, as collection.h declares
// it, and not defined. Both the declaration and its absence have been
// load-bearing for HandleConnPacket (0x10036ee5), by the count sensitivity
// described at TCPMsgView above: without it the function was identical
// while servermgr.h's object map was a hand-written class, and with
// servermgr.h holding a CMoArray<CObjectMapEntry> (whose implicit
// instantiation declares that template's members in every TU) it is the
// declaration that keeps it identical; missing either way it is a
// two-instruction reload swap.
//
// IT IS A MEMBER, NOT A SECOND BASE, and that was measured rather than
// guessed. Every derived-to-base POINTER conversion cl 8447 emits at a
// non-zero base offset carries a null test -- `mov %ebx,%ecx / lea
// 0x50(%ebx),%eax / neg %ecx / sbb %ecx,%ecx / and %eax,%ecx` -- and all
// five spellings produce it: `this`, `(Arr*)this`, `&(Arr&)*this`, a named
// `Arr& r = *this`, and an assignment to a local `Arr* p = this`. A
// throwaway TU with the same class shape was compiled to check each one.
// The shipped UpdateQuery hands this array to tcp_FindResponse as a bare
// `lea 0x50(%ebx),%eax` (0x100368c2) and the shipped ~TCPDriver reaches it
// the same way, so the driver never performs that conversion -- the array
// is a member at +0x50, not a base. Making the switch is what took
// ~TCPDriver from a 67/65 near to byte-identical.
//
// StartQuery's `lea 0x50(%esi),%ecx / push 0x8(%ecx)` (0x100364a0), which
// this comment used to read as proof of a base this-adjust, is nothing of
// the kind: Append and Insert are inline, so `m_Query.Append(x)` also
// materialises the array pointer into %ecx and then reads m_nElements
// through it.
template<class T>
class TCPQueryArrayT
{
public:
	// 0x10037c4c — CMoArray's own constructor (Clear + Init); above the
	// app-region cut, so declared and not defined.
#ifdef LT_SERVER_BUILD
	TCPQueryArrayT();
#else
	// lithtech.exe 0x00484c10 (TCPDriver::TCPDriver) EXPANDS it: the vptr
	// store 0x004a3810, Clear's four zero stores, Init's Term as `call
	// 0x00487ac0` (SetSize2(0, &g_MoAlloc)), _InitArray's four stores and
	// SetSize(0) as the second SetSize2 call. collection.h's bodies,
	// transcribed like SetSize2 below; server.dll calls the constructor.
	TCPQueryArrayT() { Clear(); Init(); }
	LTBOOL Init(uint32 size = 0, uint32 cacheSize = CACHE_DEFAULT_SIZE);
	LTBOOL SetSize(uint32 newSize) { return SetSize2(newSize, &g_MoAlloc); }
	void   _InitArray(uint32 wantedCache);
	void   Clear()
	{
		m_pArray = 0;
		m_nElements = 0;
		m_Cache.SetCacheSize(0);
		m_Cache.SetWantedCache(0);
	}
#endif
	// NOT virtual, and the table at 0x1004e9bc says why: its two slots are
	// 0x1000ba8f and 0x1000b2b7, an out-parameter getter and a
	// compare-against-m_nElements -- neither is a `??_G`, so the class is
	// polymorphic on two ordinary virtuals and its destructor is a plain
	// inline. Declaring them (and not defining them, the way Insert2 below
	// is declared) is what keeps ~TCPDriver's
	// `movl $0x1004e9bc,(%ecx)` -- VC6 resets the vptr at the head of ANY
	// destructor of a class that has one -- without making this file emit a
	// scalar-deleting-destructor COMDAT that the shipped tcpdriver.obj's
	// linker discarded in favour of another TU's copy.
	virtual void   _v00(void* pOut);         // 0x1000ba8f
	virtual LTBOOL _v04(uint32* pIndex);     // 0x1000b2b7

	~TCPQueryArrayT() { Term(); }

	// 0x100380c9 and 0x10037f49 — the 0xa4-byte element instantiations of
	// CMoArray's insert and resize (lithtech.exe 0x00487e50 / 0x00487ac0).
	// SetSize2 and _DeleteAndDestroyArray are defined below the class.
	LTBOOL Insert2(uint32 iItem, const T& toInsert,
	               CMoAllocator* pAlloc);
	LTBOOL SetSize2(uint32 newSize, CMoAllocator* pAlloc);
	void   _DeleteAndDestroyArray(LAlloc* pAlloc, uint32 nElements);
	T*     _AllocateTArray(uint32 nElements, LAlloc* pAlloc);
	uint32 GetNumAllocatedElements() { return m_nElements + m_Cache.GetCacheSize(); }

	LTBOOL Insert(uint32 iItem, const T& toInsert)
		{ return Insert2(iItem, toInsert, &g_MoAlloc); }
	LTBOOL Append(const T& toAdd)
		{ return Insert(m_nElements, toAdd); }
	void   Term(CMoAllocator* pAlloc = &g_MoAlloc) { SetSize2(0, pAlloc); }

	// +0x00 is the vptr the two virtuals above declare; 0x1004e9bc is the
	// table. As a BASE, making this class polymorphic cost TCPDriver's
	// constructor its match (131 against 123), which is why it used to be
	// modelled as a plain `void* m_pVTable` field. As a MEMBER it costs
	// nothing: the constructor is out-of-line (0x10037c4c, above the
	// app-region cut) so TCPDriver's constructor stores nothing, while the
	// inline destructor is expanded into ~TCPDriver and emits the
	// `movl $0x1004e9bc,(%ecx)` at 0x10036146 the old model was missing.
	T*                m_pArray;              // +0x04
	uint32            m_nElements;           // +0x08
	DefaultCache      m_Cache;               // +0x0c  m_CacheSize, m_WantedCache
};

// collection.h's CMoArray<T, C>::SetSize2 and _DeleteAndDestroyArray,
// transcribed (see the class comment for why they are here and not reached
// through the real template). The allocator is the same object under its
// two names, so the casts cost nothing.
template<class T>
LTBOOL TCPQueryArrayT<T>::SetSize2(uint32 newSize, CMoAllocator* pAlloc)
{
	if ((newSize == m_nElements) && (newSize != 0))
		return LTTRUE;

	if (m_pArray)
		_DeleteAndDestroyArray((LAlloc*)pAlloc, GetNumAllocatedElements());

	m_nElements = newSize;
	if (newSize > 0)
	{
		m_pArray = LNew_Array((LAlloc*)pAlloc, T, newSize + m_Cache.GetWantedCache());
		if (!m_pArray)
		{
			m_nElements = 0;
			m_Cache.SetCacheSize(0);
			return LTFALSE;
		}

		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
	}

	return LTTRUE;
}

template<class T>
void TCPQueryArrayT<T>::_DeleteAndDestroyArray(LAlloc* pAlloc, uint32 nElements)
{
	if (m_pArray)
	{
		LDelete_Array(pAlloc, m_pArray, nElements);
		m_pArray = 0;
	}

	m_Cache.SetCacheSize(0);
}

#ifndef LT_SERVER_BUILD
// collection.h's CMoArray<T, C>::Init and _InitArray, for the lithtech-only
// inline constructor above (0x00484c10).
template<class T>
LTBOOL TCPQueryArrayT<T>::Init(uint32 size, uint32 cacheSize)
{
	Term();
	_InitArray(cacheSize);

	return SetSize(size);
}

template<class T>
void TCPQueryArrayT<T>::_InitArray(uint32 wantedCache)
{
	m_pArray = 0;
	m_nElements = 0;

	m_Cache.SetWantedCache(wantedCache);
	m_Cache.SetCacheSize(0);
}
#endif

// A template of one instantiation. The name the rest of this file uses.
typedef TCPQueryArrayT<TCPQueryResponse> TCPQueryArray;

// The connect-response packet types (names ours, from what each branch says).
#define CONNMSG_ACCEPTED 5
#define CONNMSG_REJECTED 6
#define CONNMSG_BADGUID  7

// 0x10058488 — the port a joining client binds locally, and 0x1005847c the
// interface the console pinned the driver to, if any (names ours).
extern unsigned short g_JoinPort;
extern char*          g_pBindAddress;

// The driver's table of live connections, at +0x9c (names ours).
// One node in the query table: the same two-word link the net queues use,
// with the connection hung off it (names ours).
class TCPConnNode
{
public:
	TCPConnNode* m_pNext;                    // 0x00
	TCPConnNode* m_pPrev;                    // 0x04
	void*        m_pData;                    // 0x08
};

class TCPConnTable
{
public:
	// Both are bodies the driver's constructor and destructor expand
	// inline; the table is empty at each end of the driver's life.
	TCPConnTable()  { m_nItems = 0; m_pTail = LTNULL; }
	~TCPConnTable() { m_pTail = LTNULL; m_nItems = 0; }

	// 0x100261eb — file a connection in the query table. The list is
	// circular and holds only its tail, so appending is splicing in behind
	// that tail and the tail update is written as a test, exactly as the net
	// manager's message list does it; the only addition is the connection
	// the node carries.
	//
	// AN IN-CLASS INLINE, and lithtech.exe is what says so: it has no
	// out-of-line Add at all. JoinSession (0x00486e16 .. 0x00486e67) carries
	// the whole body expanded, and HandleConnPacket expands it too, down to
	// the splice (0x004862ac; its non-empty arm is a call to 0x00488130).
	// server.dll's /O1 keeps it the out-of-line COMDAT at 0x100261eb.
	//
	// The empty arm stores the tail BEFORE the count: `mov %eax,0xa0(%ebp) ;
	// incl 0x9c(%ebp)` at 0x00486e61. Written that way the server COMDAT is
	// byte-identical too (it was a 70/73 near with the count first, the
	// "shared tail merge" once blamed on the compiler), and JoinSession
	// loses its last Add-shaped difference.
	void Add(class TCPConnection* pConn, TCPConnNode* pNode)
	{
		TCPConnNode* pAt = m_pTail;

		if (pAt)
		{
			pAt->m_pNext->m_pPrev = pNode;
			pNode->m_pNext = pAt->m_pNext;
			pNode->m_pPrev = pAt;
			pAt->m_pNext = pNode;
			pNode->m_pData = (void*)pConn;

			m_nItems++;

			if (pNode->m_pPrev == m_pTail)
				m_pTail = pNode;
		}
		else
		{
			pNode->m_pPrev = pNode;
			pNode->m_pNext = pNode;
			pNode->m_pData = (void*)pConn;

			m_pTail = pNode;
			m_nItems++;
		}
	}
	void Remove(TCPConnNode* pNode);                            // 0x10026234

	uint32       m_nItems;                   // 0x00
	TCPConnNode* m_pTail;                    // 0x04
};

// 0x10036672 — the last winsock error as text (name ours).
char* tcp_GetLastErrorString();

// More fixed prototypes of the 0x1000e3f9 printer, alongside servermgr.h's
// dsi_PrintString/dsi_PrintStat: TCPDriver::Init batches one add esp,0x14
// across three consecutive trace lines, so none of the three sites was
// compiled varargs. Name ours.
void dsi_PrintStr(char* pFormat, char* pValue);
void dsi_PrintCount(char* pFormat, int nValue, char* pSuffix);
void dsi_PrintAddr(char* pFormat, uint32 b1, uint32 b2, uint32 b3, uint32 b4);

// One connection over the driver: NetConnection plus the socket it rides.
class TCPConnection : public NetConnection
{
public:
	TCPConnection() { m_Socket = 0; }

	// 0x10036daf — an ordinary member, not a compiler-generated delete;
	// defined `inline` further down, above every one of its callers (name
	// ours).
	void* Destroy(uint32 nFlags);            // 0x10036daf

	uint8              m_pad_b8[0xbc - 0xb8];
	struct sockaddr_in m_Addr;               // 0xbc  who is on the other end
	uint8              m_TableNode[0x0c];    // 0xcc  the connection table fills it
	SOCKET             m_Socket;             // 0xd8
	uint32             m_Unknown_dc;         // 0xdc
};

// CBaseDriver again, with the eight slots servermgr.h has as argument-less
// `_pNN` stubs typed the way the shipped TCP driver uses them, and with
// GetServiceList / UpdateSessionName / GetSessionName returning the LTRESULT
// their bodies actually set. Same 27 slots, same 0x50-byte layout;
// servermgr.h is shared with the client half and is left alone (see the
// report at the end of this file).
class CTCPDriverBase
{
public:
	// An inline EMPTY body, not pure. The shipped destructor's last
	// instruction is `movl $0x1004d870,(%esi)` -- the base's own vtable,
	// the same one our matching constructor stores through -- where a pure
	// declaration leaves VC6 no choice but to emit a call. Worth one
	// instruction and one residual block on ~TCPDriver; the class stays
	// abstract on its twenty-seven other pure slots.
	virtual ~CTCPDriverBase() {}
	virtual LTBOOL   Init() = 0;                                 // 0x04
	virtual void     Term() = 0;                                 // 0x08
	virtual void     Update() = 0;                               // 0x0c
#ifdef LT_SERVER_BUILD
	virtual void     _p10() = 0;                                 // 0x10
#else
	// lithtech.exe's body is `ret $4` (one argument); spelt that way only in
	// the client build, since the server's sensitivity to what this file
	// declares (see TCPMsgView) reaches even a pure slot's signature.
	virtual void     _p10(void* pUnused) = 0;                    // 0x10
#endif
	virtual LTRESULT GetServiceList(NetService** ppList) = 0;    // 0x14
	virtual int      SelectService(HNETSERVICE hService) = 0;    // 0x18
	// 0x1c: run a whole query and hand back what answered.
	virtual LTRESULT GetSessionList(NetSession** ppList, char* pAddresses) = 0;
	virtual LTRESULT StartQuery(char* pAddresses) = 0;           // 0x20
	virtual LTRESULT UpdateQuery() = 0;                          // 0x24
	virtual LTRESULT GetQueryResults(NetSession** ppList) = 0;   // 0x28
	virtual LTRESULT EndQuery() = 0;                             // 0x2c
	virtual LTRESULT HostSession(NetHost* pHostInfo) = 0;        // 0x30
	virtual LTRESULT JoinSession(NetSession* pSession) = 0;      // 0x34
	virtual LTRESULT UpdateSessionName(const char* sName) = 0;   // 0x38
	virtual LTRESULT GetSessionName(char* sName, uint32 nLen) = 0;  // 0x3c
	virtual LTBOOL   IsLobbyLaunched() = 0;                      // 0x40
	virtual LTBOOL   GetLobbyLaunchInfo(void** ppData) = 0;      // 0x44
#ifdef LT_SERVER_BUILD
	virtual void     _p48() = 0;                                 // 0x48
	virtual void     _p4c() = 0;                                 // 0x4c
#else
	virtual LTBOOL   _p48(void* pUnused) = 0;                    // 0x48
	virtual LTBOOL   _p4c() = 0;                                 // 0x4c
#endif
	virtual LTBOOL   GetTcpIpAddress(char* sAddress, uint32 dwBufferSize,
	                                 unsigned short& hostPort) = 0;  // 0x50
	virtual void     _p54() = 0;                                 // 0x54
	virtual LTRESULT SendTo(const void* pData, uint32 len,
	                        const char* sAddr, uint32 port) = 0; // 0x58
	virtual void     Disconnect(NetConnection* pConn, uint32 nReason,
	                            uint32 flags) = 0;               // 0x5c
	virtual LTBOOL   _p60(void* pData, uint32 nLen, uint32 nUnused,
	                      NetConnection* pConn) = 0;             // 0x60
	virtual LTBOOL   ReceivePacket(class ServerMessage* pMsg) = 0;  // 0x64
	virtual uint32   GetRecvSize() = 0;                          // 0x68

public:
	CTCPDriverBase() { m_Timeout = 120.0f; m_DriverFlags = 0; }

	// 0x10036060 strncpy's "internet" here; CLocalDriver::Init strcpy's
	// "local".
	char   m_Name[0x44 - 0x04];              // 0x04
	uint32 m_DriverFlags;                    // 0x44
	class CNetMgr* m_pNetMgr;                // 0x48
	float  m_Timeout;                        // 0x4c
};

// The internal record an HNETSERVICE names, as the TCP driver holds it: one
// virtual (the destructor, 0x100360db) and the owning driver. servermgr.h's
// CNetService has the vptr as `m_Unknown00`; GetServiceList proves what it
// is by storing `this` immediately after it (name ours).
class TCPNetService
{
public:
	virtual ~TCPNetService() {}              // 0x100360db is its ??_G

	class TCPDriver* m_pDriver;              // +0x04
};

class TCPDriver : public CTCPDriverBase
{
public:
	virtual LTRESULT HostSession(NetHost* pHostInfo);

	// The rest of the vtable, by slot (names ours where the SDK has none).
	virtual LTBOOL   Init();                                      // 0x04
	virtual void     Term();                                      // 0x08
	virtual LTRESULT GetServiceList(NetService** ppListHead);     // 0x14
	virtual LTRESULT GetSessionList(NetSession** ppList, char* pAddresses); // 0x1c
	virtual LTRESULT GetQueryResults(NetSession** ppList);        // 0x28
	virtual LTRESULT UpdateQuery();                               // 0x24
	virtual LTRESULT EndQuery();                                  // 0x2c
	virtual LTRESULT JoinSession(NetSession* pSession);           // 0x34
	virtual LTRESULT UpdateSessionName(const char* pName);        // 0x38
	virtual LTRESULT GetSessionName(char* pName, uint32 nLen);    // 0x3c
	virtual void     Disconnect(NetConnection* pConn, uint32 nReason, uint32 flags); // 0x5c
	virtual LTBOOL   _p60(void* pData, uint32 nLen, uint32 nUnused,
	                      NetConnection* pConn);                  // 0x60
	virtual LTBOOL   ReceivePacket(class ServerMessage* pMsg);    // 0x64
	virtual LTBOOL   GetTcpIpAddress(char* sAddress, uint32 dwBufferSize,
	                                 unsigned short& hostPort);   // 0x50

#ifndef LT_SERVER_BUILD
	// The slots this reconstruction has not reached, declared so that
	// tcp_Create's `new TCPDriver` (lithtech 0x004871e0, end of file) can
	// instantiate the class. Lithtech-only because server.dll's tcp_Create
	// lives in server/worldtree.cpp against its own copy of the class.
	virtual void     Update();                                    // 0x0c
	virtual void     _p10(void* pUnused);                         // 0x10
	virtual int      SelectService(HNETSERVICE hService);         // 0x18
	virtual LTBOOL   IsLobbyLaunched();                           // 0x40
	virtual LTBOOL   GetLobbyLaunchInfo(void** ppData);           // 0x44
	virtual LTBOOL   _p48(void* pUnused);                         // 0x48
	virtual LTBOOL   _p4c();                                      // 0x4c
	virtual void     _p54();                                      // 0x54
	virtual LTRESULT SendTo(const void* pData, uint32 len,
	                        const char* sAddr, uint32 port);      // 0x58
	virtual uint32   GetRecvSize();                               // 0x68
#endif

	// Not in the vtable: reached by direct call from Term and Disconnect.
	void     SendDisconnect(NetConnection* pConn, uint32 nReason); // 0x1003629d
	// 0x100362f5 — the live connection whose remote end matches, if any.
	class TCPConnection* FindConnection(struct sockaddr_in* pAddr);
	// 0x10036ee5 — a datagram that arrived with a zero first byte is a
	// connection-level message rather than game traffic.
	void     HandleConnPacket(class ServerMessage* pMsg,
	                          struct sockaddr_in* pFrom);

	// The two the interface reaches by name (query is a broadcast sweep, join
	// is the connect handshake).
	TCPDriver();                             // 0x10036060
	virtual ~TCPDriver();                    // 0x10036118

	LTRESULT StartQuery(char* pAddresses);
	LTRESULT JoinSession(struct sockaddr_in* pHostAddr);

	// 0x10036002 — one datagram in, if there is one waiting.
	friend int tcp_RecvFrom(TCPDriver*, SOCKET, void*, uint32, struct sockaddr_in*);

	// 0x100361d3 — drop both sockets and whatever the last query left.
	void CloseSockets(int bFlags);

	// 0x10036278 — one datagram to one address over the given socket.
	LTBOOL SendTo(SOCKET hSocket, void* pData, uint32 nLen, struct sockaddr_in* pTo);

	// +0x50, and a MEMBER rather than a second base: every derived-to-base
	// POINTER conversion VC6 8447 emits carries the `neg/sbb/and` null test
	// (measured -- `this`, a C cast, a reference cast and a named reference
	// all produce it), and the shipped UpdateQuery hands the array to
	// tcp_FindResponse as a bare `lea 0x50(%ebx),%eax` with no test at all
	// (0x100368c2).  ~TCPDriver's note records the same residual.
	TCPQueryArray      m_Query;              // 0x050
	SOCKET             m_QuerySocket;        // 0x064
	float              m_QueryEndTime;       // 0x068
	struct sockaddr_in m_QueryAddr;          // 0x06c
	uint32             m_bHosting;           // 0x07c
	struct sockaddr_in m_HostAddr;           // 0x080
	SOCKET             m_Socket;             // 0x090
	uint32             m_MaxPlayers;         // 0x094
	// The advertised session name, kept in an exactly-sized heap block:
	// UpdateSessionName reuses it while the new name still fits and
	// reallocates otherwise, and CloseSockets throws it away.
	char*              m_pSessionName;       // 0x098
	TCPConnTable       m_ConnTable;          // 0x09c
	uint32             m_bWinsockUp;         // 0x0a4  this driver called WSAStartup
	TCPNetService      m_Service;            // 0x0a8  what our HNETSERVICE names
};

// The driver's own session record: NetSession plus the address to connect
// to. 0x10036ed3 forwards the interface's join to the private one by adding
// 0x10a0 to the session it is handed, and sizeof(NetSession) is 0x109c, so
// one word sits between them (names ours).
class TCPSession : public NetSession
{
public:
	TCPSession(class TCPDriver* pDriver) : m_pDriver(pDriver) {}

	class TCPDriver*   m_pDriver;            // 0x109c
	struct sockaddr_in m_Addr;               // 0x10a0
};


// ---- the three inline helpers, hoisted above every one of their callers -- //
//
// All three are `inline` and all three sit here rather than beside their
// siblings for the same reason: VC6's inliner is FORWARD-ONLY, so a body the
// shipped /O2 client expands has to have been seen before the expanding
// function was compiled. Each is proved inline the same way -- the shipped
// /O2 client has the body folded into its callers while the shipped /O1
// server keeps a standalone COMDAT and CALLS it from those same callers,
// which is only possible if the original marked it inline and /Os declined
// on size. None of the three can therefore move server.dll.

// 0x10036daf has exactly the shape VC6 gives a scalar deleting destructor —
// tear down, then free when bit 0 is set, returning this — but it is reached
// by a direct call with a literal 1, and the same 1 is shared with the field
// store above it. A compiler-generated delete would have to dispatch through
// a vtable, and this class has none (its base opens with a rate tracker, not
// a vptr), so it is an ordinary member here (name ours).
//
// Expanded at all three of the /O2 client's call sites -- inside
// JoinSession (0x00486dca), inside CloseSockets' copy of tcp_FreeQueryList
// (0x00484e0c) and inside Disconnect (0x00485ec1) -- and called at all three
// of the /O1 server's (0x100380b8, 0x10036da5 and JoinSession's).
inline void* TCPConnection::Destroy(uint32 nFlags)
{
	if (m_Socket)
		closesocket(m_Socket);

	Term();

	// `::operator delete(this)`, not `delete this`: the shipped code has the
	// bare deallocator call (push this / call 0x10028ace / pop ecx) with no
	// null test and no dispatch through the vtable, which is what the delete
	// EXPRESSION would add on a class with a virtual destructor. It is also
	// not dfree -- 0x10028ace is the four-instruction operator new/delete
	// forwarder in front of dfree at 0x1000d398.
	if (nFlags & 1)
		::operator delete(this);

	return this;
}

// 0x10038094 — drop whatever the last query collected (name ours). The walk
// is TCPDriver::Term's, with the connection destroyed instead of told to go
// away, and the table is left empty rather than merely drained.
//
// The /O1 body sits at 0x10038094, between the two CMoArray instantiations
// this object also carries (0x10037f49 and 0x100380c9) -- that is the
// COMDAT run, which is what says the function is inline and belongs to this
// translation unit rather than to whatever else lives around 0x10038000.
// CloseSockets calls it there; the /O2 client has it expanded (0x00484df1).
inline void tcp_FreeQueryList(TCPConnTable* pTable)
{
	TCPConnNode* pCur = pTable->m_pTail;

	while (pCur)
	{
		TCPConnection* pConn = (TCPConnection*)pCur->m_pData;
		pCur = (pCur->m_pPrev == pTable->m_pTail) ? LTNULL : pCur->m_pPrev;

		if (pConn)
			pConn->Destroy(1);
	}

	pTable->m_pTail = LTNULL;
	pTable->m_nItems = 0;
}

// 0x10026234 — unlink one node. The list is circular, so the unlink itself
// needs no null tests; only the tail pointer does, and it moves back one on
// the way out and goes null when the last node leaves.
//
// The /O1 body sits immediately after Add's (0x100261eb) in the same COMDAT
// run, and Disconnect calls it there (0x10036d9c); the /O2 client has it
// expanded into Disconnect (0x00485e79).
inline void TCPConnTable::Remove(TCPConnNode* pNode)
{
	pNode->m_pNext->m_pPrev = pNode->m_pPrev;
	pNode->m_pPrev->m_pNext = pNode->m_pNext;

	if (pNode == m_pTail)
		m_pTail = m_pTail->m_pPrev;

	m_nItems--;

	if (m_nItems == 0)
		m_pTail = LTNULL;
}


LTRESULT TCPDriver::HostSession(NetHost* pHostInfo)
{
	char               errStr[64];
	struct sockaddr_in boundAddr;

	CloseSockets(0);

	uint16 nPort = (uint16)pHostInfo->m_Port;
	if (nPort == 0)
		nPort = TCP_DEFAULTPORT;

	if (!tcp_MakeAddr(&m_HostAddr, nPort))
		return LT_ERROR;

	m_Socket = tcp_OpenSocket(this, &m_HostAddr, &boundAddr);
	if (m_Socket == 0)
	{
		sprintf(errStr, "%d", nPort);
		RETURN_ERROR_INFO(2, "TCPDriver::HostSession", LT_CANTBINDTOPORT, errStr);
	}

	if (g_ShowNetInfo)
	{
		dsi_ServerError("Hosting on %d.%d.%d.%d:%d",
			boundAddr.sin_addr.S_un.S_un_b.s_b1, boundAddr.sin_addr.S_un.S_un_b.s_b2,
			boundAddr.sin_addr.S_un.S_un_b.s_b3, boundAddr.sin_addr.S_un.S_un_b.s_b4,
			ntohs(boundAddr.sin_port));
	}

	m_bHosting = 1;
	m_MaxPlayers = pHostInfo->m_dwMaxPlayers;
	UpdateSessionName(pHostInfo->m_sName);
	return LT_OK;
}


// The broadcast sweep. Every address in the caller's list gets one request:
// "*" (optionally "*:port") means broadcast, anything else is a literal
// address. Both failures report the same thing and the shipped code has one
// copy of the block, at the end, with two jumps into it — so the success path
// is the nested one.
//
// Both arms that produce an address carry their own `m_Time = ...;
// Append(resp)` -- no `continue` past a shared tail. Written with the
// failure as `continue` and one shared append, VC6 keeps a single loop test
// and does not rotate the loop, so there is no preheader for the
// `mov $g_MoAlloc,%edi` hoist (0x10036416) and the duplicated latch
// (0x100364ad) is missing: 407 of 422 bytes. With the append in each arm
// VC6 cross-jumps the two copies back into one, rotates the loop, and the
// result is byte-identical. while, for, if + do-while, a hoisted bMore and
// cl 8168 were all measured earlier and none of them rotates.
LTRESULT TCPDriver::StartQuery(char* pAddresses)
{
	TCPQueryResponse resp;
	CommandParser    parser;
	int              bOn;

	parser.m_pPos = LTNULL;
	EndQuery();

	if (tcp_MakeAddr(&m_QueryAddr, 0))
	{
		m_QuerySocket = tcp_OpenSocket(this, &m_QueryAddr, &m_QueryAddr);
		if (m_QuerySocket != 0)
		{
			parser.m_pPos = pAddresses;
			while (parser.Next())
			{
				if (parser.m_nArgs > 0)
				{
					memset(&resp, 0, sizeof(resp));

					char* pAddr = parser.m_ArgPointers[0];
					if (*pAddr == '*')
					{
						char*  pColon = strchr(pAddr, ':');
						uint16 nPort;

						if (pColon)
							nPort = (uint16)atoi(pColon + 1);
						else
							nPort = TCP_DEFAULTPORT;

						resp.m_Addr.sin_family = AF_INET;
						resp.m_Addr.sin_port = htons(nPort);
						resp.m_Addr.sin_addr.S_un.S_addr = INADDR_BROADCAST;
						resp.m_Unknown80 = 0xff;

						resp.m_Time = dsi_GetTime();
						m_Query.Append(resp);
					}
					else if (tcp_BuildSockaddrFromString(pAddr, &resp.m_Addr))
					{
						resp.m_Time = dsi_GetTime();
						m_Query.Append(resp);
					}
				}
			}

			bOn = 1;
			setsockopt(m_QuerySocket, SOL_SOCKET, SO_BROADCAST, (char*)&bOn, sizeof(bOn));

			m_QueryEndTime = dsi_GetTime() - 2.0f;
			UpdateQuery();
			return LT_OK;
		}
	}

	RETURN_ERROR_INFO(2, "TCPDriver::StartQuery", LT_ERROR, "Can't bind to port");
}


// The connect handshake. One request goes out every 300ms and every datagram
// that comes back is examined until the host answers, ten seconds pass, or the
// host says no. Both message references are scoped, which is why the shipped
// code carries the same two releases at all six exits.
LTRESULT TCPDriver::JoinSession(struct sockaddr_in* pHostAddr)
{
	char               addrStr[128];
	struct sockaddr_in bindAddr;
	struct sockaddr_in sockName;
	struct sockaddr_in fromAddr;
	struct sockaddr_in hostAddr;
	CMessageRef        msgOut;
	CMessageRef        msgIn;
	int                nameLen;
	uint32             startTime;
	uint32             lastSend;
	uint32             now;
	SOCKET             hSocket;
	int                nRead;

	hostAddr = *pHostAddr;

	CloseSockets(0);

	if (!tcp_MakeAddr(&bindAddr, g_JoinPort))
		return LT_ERROR;

	hSocket = tcp_OpenSocket(this, &bindAddr, &bindAddr);

	sprintf(addrStr, "%d.%d.%d.%d:%d",
		pHostAddr->sin_addr.S_un.S_un_b.s_b1, pHostAddr->sin_addr.S_un.S_un_b.s_b2,
		pHostAddr->sin_addr.S_un.S_un_b.s_b3, pHostAddr->sin_addr.S_un.S_un_b.s_b4,
		ntohs(pHostAddr->sin_port));

	if (hSocket == 0)
		RETURN_ERROR_INFO(2, "TCPDriver::JoinSession", LT_CANTBINDTOPORT, addrStr);

	if (g_ShowNetInfo)
		dsi_ServerError("Joining on port %d", ntohs(bindAddr.sin_port));

	{
		CAbstractIO* pIO;
		msgOut.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
		if (pIO)
			pIO->Release();
	}

	msgOut.m_pMsg->m_pData[0] = 0;
	msgOut.m_pMsg->WriteByte(3);
	msgOut.m_pMsg->WriteByte(0);
	msgOut.m_pMsg->WriteData(&m_pNetMgr->m_Guid, sizeof(LTGUID));

	{
		CAbstractIO* pIO;
		msgIn.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
		if (pIO)
			pIO->Release();
	}

	startTime = timeGetTime();
	lastSend = startTime - 300;

	do
	{
		now = timeGetTime();
		if (now - lastSend >= 300)
		{
			if (g_ShowNetInfo)
				dsi_ServerError("IP: Sending conn request to %d.%d.%d.%d:%d",
					hostAddr.sin_addr.S_un.S_un_b.s_b1, hostAddr.sin_addr.S_un.S_un_b.s_b2,
					hostAddr.sin_addr.S_un.S_un_b.s_b3, hostAddr.sin_addr.S_un.S_un_b.s_b4,
					ntohs(hostAddr.sin_port));

			SendTo(hSocket, msgOut.m_pMsg->m_pData, msgOut.m_pMsg->m_Unknown34, &hostAddr);
			lastSend = now;
		}

		ServerMessage* pIn   = msgIn.m_pMsg;
		uint32         nSize = pIn->m_DataSize;
		uint8*         pData = pIn->m_pData;

		nRead = tcp_RecvFrom(this, hSocket, pData, nSize, &fromAddr);
		if (nRead != -1 && nRead != 0)
		{
			if (g_ShowNetInfo)
				dsi_ServerError("IP: Got PacketID %d (len %d) while connecting",
					msgIn.m_pMsg->m_pData[0], nRead);

			msgIn.m_pMsg->m_Unknown34 = (uint16)nRead;
			msgIn.m_pMsg->m_Unknown38 = 1;

			if (msgIn.m_pMsg->m_pData[0] == 0)
			{
				// Expanded in place in lithtech.exe (0x00486c1e ..): the
				// template spelling, server.dll's stand-in -- see the note
				// at UpdateQuery's second read.
#ifdef LT_SERVER_BUILD
				uint8 nType = msgIn.m_pMsg->ReadByte(0);
#else
				uint8 nType = ((CPacket*)msgIn.m_pMsg)->ReadType((uint8*)0);
#endif
				if (!(msgIn.m_pMsg->m_MsgFlags & MSGFLAG_READERROR))
				{
					if (nType == CONNMSG_ACCEPTED)
					{
						if (g_ShowNetInfo)
						{
							dsi_ServerError("IP: Connection to %d.%d.%d.%d:%d accepted",
								hostAddr.sin_addr.S_un.S_un_b.s_b1, hostAddr.sin_addr.S_un.S_un_b.s_b2,
								hostAddr.sin_addr.S_un.S_un_b.s_b3, hostAddr.sin_addr.S_un.S_un_b.s_b4,
								ntohs(hostAddr.sin_port));
							dsi_ServerError("recvfrom addr: %d.%d.%d.%d:%d",
								fromAddr.sin_addr.S_un.S_un_b.s_b1, fromAddr.sin_addr.S_un.S_un_b.s_b2,
								fromAddr.sin_addr.S_un.S_un_b.s_b3, fromAddr.sin_addr.S_un.S_un_b.s_b4,
								ntohs(fromAddr.sin_port));
						}

						if (!(msgIn.m_pMsg->m_MsgFlags & MSGFLAG_READERROR))
						{
							TCPConnection* pConn = new TCPConnection;
							if (pConn)
							{
								// +0xdc before +0x30: lithtech.exe's
								// order (0x00486dae / 0x00486db8);
								// server.dll's bytes are the same either way.
								pConn->m_Addr = fromAddr;
								pConn->m_Unknown_dc = 1;
								pConn->m_pDriver = (CBaseDriver*)this;

								if (m_pNetMgr->AddConnection(pConn))
								{
									m_ConnTable.Add(pConn, (TCPConnNode*)&pConn->m_TableNode);
									m_Socket = hSocket;

									if (g_ShowNetInfo)
									{
										int nRet;

										nameLen = sizeof(sockName);
										nRet = getsockname(hSocket, (struct sockaddr*)&sockName, &nameLen);
										if (nRet == 0)
										{
											dsi_ServerError("getsockname() returned %d.%d.%d.%d:%d",
												sockName.sin_addr.S_un.S_un_b.s_b1, sockName.sin_addr.S_un.S_un_b.s_b2,
												sockName.sin_addr.S_un.S_un_b.s_b3, sockName.sin_addr.S_un.S_un_b.s_b4,
												ntohs(sockName.sin_port));
										}
										else
										{
											dsi_ServerError("getsockname() returned error %d", nRet);
										}
									}

									return LT_OK;
								}

								pConn->Destroy(1);
							}
						}
					}
					else if (nType == CONNMSG_REJECTED)
					{
						if (g_ShowNetInfo)
							dsi_ServerError("IP: Connection to %d.%d.%d.%d:%d rejected",
								hostAddr.sin_addr.S_un.S_un_b.s_b1, hostAddr.sin_addr.S_un.S_un_b.s_b2,
								hostAddr.sin_addr.S_un.S_un_b.s_b3, hostAddr.sin_addr.S_un.S_un_b.s_b4,
								ntohs(hostAddr.sin_port));

						closesocket(hSocket);
						RETURN_ERROR_INFO(2, "TCPDriver::JoinSession", LT_REJECTED, addrStr);
					}
					else if (nType == CONNMSG_BADGUID)
					{
						if (g_ShowNetInfo)
							dsi_ServerError("IP: %d.%d.%d.%d:%d not using same app LTGUID",
								hostAddr.sin_addr.S_un.S_un_b.s_b1, hostAddr.sin_addr.S_un.S_un_b.s_b2,
								hostAddr.sin_addr.S_un.S_un_b.s_b3, hostAddr.sin_addr.S_un.S_un_b.s_b4,
								ntohs(hostAddr.sin_port));

						closesocket(hSocket);
						RETURN_ERROR_INFO(2, "TCPDriver::JoinSession", LT_NOTSAMEGUID, addrStr);
					}
				}
			}
		}
	}
	while (now - startTime < 10000);

	RETURN_ERROR_INFO(2, "TCPDriver::JoinSession", LT_TIMEOUT, addrStr);
}


#ifndef LT_SERVER_BUILD
// lithtech.exe 0x004853c0 (118 bytes) -- the out-of-line copy of the
// element's deep-copy assignment, sitting between StartQuery and
// tcp_MakeAddr: StartQuery's expanded Insert2 calls it five times
// (0x004851f9 .. 0x004852e7), as do tcpqarray.cpp's early Insert2 /
// GenAppend / GenRemoveAt sites. Free the old block, copy all 0xa4 bytes
// (`rep movsl $0x29`), then strlen/new/strcpy the SOURCE's block; void, ret
// $4. The server twin is worldtree.cpp's TCPQueryResponse_A::operator=
// (0x10037c9c). In the shipped build it is an inline member left out of
// line; tcpdriver.cpp has no Insert2 body to call it from, so it is
// defined here as a plain member (stand-in for the COMDAT).
void TCPQueryResponse::operator=(const TCPQueryResponse& other)
{
	if (m_pData)
		::operator delete(m_pData);

	memcpy(this, &other, sizeof(TCPQueryResponse));

	if (other.m_pData)
	{
		m_pData = (uint8*)::operator new(strlen((char*)other.m_pData) + 1);
		strcpy((char*)m_pData, (char*)other.m_pData);
	}
}
#endif


// ---- the driver's own helpers ------------------------------------------- //

// A bind address for one port. The console can pin the interface to bind on;
// with nothing set the address stays INADDR_ANY.
int tcp_MakeAddr(struct sockaddr_in* pAddr, unsigned short nPort)
{
	memset(pAddr, 0, sizeof(*pAddr));

	pAddr->sin_family = AF_INET;
	pAddr->sin_port = htons(nPort);

	if (g_pBindAddress)
		pAddr->sin_addr.S_un.S_addr = inet_addr(g_pBindAddress);
	else
		pAddr->sin_addr.S_un.S_addr = 0;

	return 1;
}

// One datagram out; true only when the whole thing went.
LTBOOL TCPDriver::SendTo(SOCKET hSocket, void* pData, uint32 nLen,
                         struct sockaddr_in* pTo)
{
	return sendto(hSocket, (const char*)pData, nLen, 0,
	              (const struct sockaddr*)pTo, sizeof(*pTo)) == (int)nLen;
}

// Drop everything the driver holds. Winsock itself is only shut down when the
// caller asks for it, and only if this driver was the one that started it.
void TCPDriver::CloseSockets(int bTerm)
{
	tcp_FreeQueryList(&m_ConnTable);

	if (m_Socket)
	{
		closesocket(m_Socket);
		m_Socket = 0;
	}

	EndQuery();

	if (m_pSessionName)
	{
		delete m_pSessionName;
		m_pSessionName = LTNULL;
	}

	if (bTerm && m_bWinsockUp)
	{
		WSACleanup();
		m_bWinsockUp = 0;
	}
}


// A bound, non-blocking datagram socket. The caller can ask what address it
// actually got; passing the same address in and out is how the query sweep
// reuses one buffer.
SOCKET tcp_OpenSocket(TCPDriver* pDriver, struct sockaddr_in* pBindAddr,
                      struct sockaddr_in* pBoundAddr)
{
	SOCKET        hSocket;
	unsigned long bNonBlocking;
	int           nRet;
	int           nLen;

	hSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		con_Printf("socket() returned SOCKET_ERROR.  Last error: %s\n",
		           tcp_GetLastErrorString());
		return 0;
	}

	bNonBlocking = 1;
	nRet = ioctlsocket(hSocket, FIONBIO, &bNonBlocking);
	if (nRet != 0)
	{
		dsi_ServerError("ioctlsocket FIONBIO (nonblocking) returned %d", nRet);
	}
	else
	{
		nRet = bind(hSocket, (const struct sockaddr*)pBindAddr, sizeof(*pBindAddr));
		if (nRet == 0)
		{
			if (pBoundAddr)
			{
				nLen = sizeof(*pBoundAddr);
				getsockname(hSocket, (struct sockaddr*)pBoundAddr, &nLen);
			}

			return hSocket;
		}

		con_Printf("bind() returned %d.  Last error: %s.\n", nRet, tcp_GetLastErrorString());
	}

	closesocket(hSocket);
	return 0;
}


// ======================================================================= //
//  0x00484a80 — "a.b.c.d[:port]" or "host[:port]" to a sockaddr_in
// ======================================================================= //
//
//  MOVED HERE from src/engine/lithtech/clientstreams.cpp, which is in
//  lithtech.exe's source list only -- so while the body lived there,
//  server.dll's 0x10035efb was never compiled even though THIS file
//  declares it (it used to spell the name tcp_ParseAddr) and StartQuery
//  below calls it.  The shipped name is in the function's own error
//  string, so the declaration was renamed rather than a second added.
//
//  Call sites 0x00485198 (StartQuery), 0x00487100 (vtable slot 0x54) and
//  0x00487186 (slot 0x58, SendTo) — all three of tcpdriver.cpp's, which is
//  what places the home.
//
//  Read off the bytes:
//
//    * cdecl, two arguments, `ret` with no pop; 1 on success and 0 on the
//      one failure.
//    * ONE local, `char szAddr[256]`, which is the whole 0x100 frame.
//    * the colon branch copies with memcpy and terminates by hand; the no-
//      colon branch uses strncpy with 0xff and terminates at [0xff]. Both
//      terminators are written from the byte register holding the shared
//      zero, so the literal is 0 and not '\0'-typed.
//    * the default port 0x6cf0 = 27888 is materialised in the no-colon
//      branch only.
//    * the leading-digit test uses SIGNED byte compares (`jl`/`jg`), so the
//      buffer is `char` and not `unsigned char`.
//    * the numeric path is ntohl(inet_addr(..)) and the host is put back
//      with htonl at the bottom, so the value is carried in HOST order
//      across the join — which is also why the name path assembles its four
//      bytes most-significant first.
//    * the name path's byte is loaded into `cl` with ecx pre-zeroed, so the
//      element is widened from a single unsigned byte.
//    * the four-byte assembly is a bottom-tested loop (`shift < 32`), and
//      the shift amount is computed from the OLD shift before the `+= 8`.
//    * the whole sockaddr is zeroed as four dwords before sin_family, which
//      is memset of sizeof(*pAddr), not four stores.
//
//  WSOCK32 is imported by ordinal: #10 inet_addr, #14 ntohl, #52
//  gethostbyname, #8 htonl, #9 htons.
//  NEAR against server.dll: 255 of 263 bytes, 92 instructions against 93,
//  and the first 0x43 bytes are byte-identical.  ONE residual, and every
//  other difference follows from it: the shipped build gives nPort a MEMORY
//  home in the dead first-parameter slot (`mov %eax,0x8(%ebp)` and
//  `movl $0x6cf0,0x8(%ebp)`), which frees %ebx for nAddr and lets the
//  host-name loop index the byte array as `(%edx,%eax,1)`; ours keeps nPort
//  in %ebx, so the loop strength-reduces the index into a pointer walk and
//  the whole loop rotates one register.  Three spellings were measured and
//  all three emit the SAME 255 bytes -- `unsigned short nPort` with the cast
//  on atoi, nPort declared last, and `for (i = 0; i < 4; i++)` with
//  `24 - i * 8` in place of the nShift induction variable -- so the choice
//  is the register allocator's and no source shape reached here.
//  The real residual is that the shipped loop is NOT strength-reduced in
//  either image (i kept as an index, `(%edx,%eax,1)` here and
//  `(%esi,%edi,1)` in lithtech); the extra register it costs is what spills
//  nPort. Also measured, all 255 unless noted: int i, int nShift, swapped
//  for-init order, do-while, pre-tested while, a named byte pointer,
//  `*(p + i)`, a named byte temporary, i block-scoped; uint8/uint16 i stop
//  the reduction and spill nPort but add a movzx (267/266); testing `i < 4`
//  stops it but spills nShift instead (261).
//  Also measured, all 255/10 blocks: `i` post-incremented inside the
//  subscript (for, do-while), `i++` as a body statement, a named uint32
//  byte, the byte read through an inline helper (still 255; the helper
//  leaves a COMDAT), `(unsigned char*)` cast on the base, `h_addr`, a
//  goto loop (261: VC6 peels it), and CL_REV 8168 and 8966 (255).
int tcp_BuildSockaddrFromString(char* pString, struct sockaddr_in* pAddr)
{
	char            szAddr[256];
	char*           pColon;
	uint32          nAddr;
	struct hostent* pHost;
	uint32          i;
	uint32          nShift;
	uint32          nPort;

	pColon = strchr(pString, ':');

	if (pColon)
	{
		memcpy(szAddr, pString, pColon - pString);
		szAddr[pColon - pString] = 0;

		nPort = atoi(pColon + 1);
	}
	else
	{
		strncpy(szAddr, pString, sizeof(szAddr) - 1);
		szAddr[sizeof(szAddr) - 1] = 0;

		nPort = 27888;
	}

	if (szAddr[0] >= '0' && szAddr[0] <= '9')
	{
		nAddr = ntohl(inet_addr(szAddr));
	}
	else
	{
		pHost = gethostbyname(szAddr);

		if (!pHost)
		{
			con_Printf("gethostbyname() returned LTNULL in tcp_BuildSockaddrFromString\n");

			return 0;
		}

		nAddr = 0;

		for (i = 0, nShift = 0; nShift < 32; i++, nShift += 8)
			nAddr |= ((uint32)(unsigned char)pHost->h_addr_list[0][i])
			         << (24 - nShift);
	}

	memset(pAddr, 0, sizeof(*pAddr));

	pAddr->sin_family      = AF_INET;
	pAddr->sin_addr.s_addr = htonl(nAddr);
	pAddr->sin_port        = htons(nPort);

	return 1;
}


// One datagram in. Nothing waiting is not an error, so the would-block case
// stays quiet even with tracing on.
int tcp_RecvFrom(TCPDriver* pDriver, SOCKET hSocket, void* pData, uint32 maxLen,
                 struct sockaddr_in* pFrom)
{
	int nFromLen = sizeof(*pFrom);
	int nRead;

	nRead = recvfrom(hSocket, (char*)pData, maxLen, 0,
	                 (struct sockaddr*)pFrom, &nFromLen);

	if (nRead != 0 && nRead != SOCKET_ERROR)
		return nRead;

	if (g_ShowNetInfo > 1)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			dsi_ServerError("recvfrom returned error %d (max packet size %d)",
			                nRead, maxLen);
		}
	}

	return -1;
}


// ---- winsock errors ------------------------------------------------------ //

// 0x10036672 — the last winsock error as text. A linear walk of a fixed
// table; an error the table does not know about is simply unnamed (names
// ours).
class WSAErrorText
{
public:
	int   m_nCode;
	char* m_pText;
};

// 0x100552d8 — the table, 47 eight-byte entries in .data (the walk's end
// test is 0x10055450). Each name is the winsock macro's own spelling, in
// the shipped order (WSAEDISCON sits ahead of the three startup errors).
#define WSA_ERR(x) { x, #x }
WSAErrorText g_WSAErrors[47] =
{
	WSA_ERR(WSAEINTR),
	WSA_ERR(WSAEBADF),
	WSA_ERR(WSAEACCES),
	WSA_ERR(WSAEFAULT),
	WSA_ERR(WSAEINVAL),
	WSA_ERR(WSAEMFILE),
	WSA_ERR(WSAEWOULDBLOCK),
	WSA_ERR(WSAEINPROGRESS),
	WSA_ERR(WSAEALREADY),
	WSA_ERR(WSAENOTSOCK),
	WSA_ERR(WSAEDESTADDRREQ),
	WSA_ERR(WSAEMSGSIZE),
	WSA_ERR(WSAEPROTOTYPE),
	WSA_ERR(WSAENOPROTOOPT),
	WSA_ERR(WSAEPROTONOSUPPORT),
	WSA_ERR(WSAESOCKTNOSUPPORT),
	WSA_ERR(WSAEOPNOTSUPP),
	WSA_ERR(WSAEPFNOSUPPORT),
	WSA_ERR(WSAEAFNOSUPPORT),
	WSA_ERR(WSAEADDRINUSE),
	WSA_ERR(WSAEADDRNOTAVAIL),
	WSA_ERR(WSAENETDOWN),
	WSA_ERR(WSAENETUNREACH),
	WSA_ERR(WSAENETRESET),
	WSA_ERR(WSAECONNABORTED),
	WSA_ERR(WSAECONNRESET),
	WSA_ERR(WSAENOBUFS),
	WSA_ERR(WSAEISCONN),
	WSA_ERR(WSAENOTCONN),
	WSA_ERR(WSAESHUTDOWN),
	WSA_ERR(WSAETOOMANYREFS),
	WSA_ERR(WSAETIMEDOUT),
	WSA_ERR(WSAECONNREFUSED),
	WSA_ERR(WSAELOOP),
	WSA_ERR(WSAENAMETOOLONG),
	WSA_ERR(WSAEHOSTDOWN),
	WSA_ERR(WSAEHOSTUNREACH),
	WSA_ERR(WSAENOTEMPTY),
	WSA_ERR(WSAEPROCLIM),
	WSA_ERR(WSAEUSERS),
	WSA_ERR(WSAEDQUOT),
	WSA_ERR(WSAESTALE),
	WSA_ERR(WSAEREMOTE),
	WSA_ERR(WSAEDISCON),
	WSA_ERR(WSASYSNOTREADY),
	WSA_ERR(WSAVERNOTSUPPORTED),
	WSA_ERR(WSANOTINITIALISED),
};
#undef WSA_ERR

char* tcp_GetErrorText()
{
	int    nErr = WSAGetLastError();
	uint32 i;

	for (i = 0; i < 47; i++)
	{
		if (g_WSAErrors[i].m_nCode == nErr)
			return g_WSAErrors[i].m_pText;
	}

	return "<unknown>";
}


// ---- the driver's own vtable, slot by slot ------------------------------ //

// 0x10036159 — slot 0x04. Winsock is started once per driver and the version
// asked for is 1.1. The failure is reported through the debug printer
// (0x1000d926) and the three lines of inventory through the console one
// (0x1000e3f9) — two different functions, and the trace lines are the fixed
// prototypes, which is what batches all three cleanups into one add.
LTBOOL TCPDriver::Init()
{
	WSADATA wsaData;
	int     nRet;

	nRet = WSAStartup(0x0101, &wsaData);
	if (nRet == 0)
	{
		m_bWinsockUp = 1;

		if (g_ShowNetInfo)
		{
			dsi_PrintString("TCP/IP initialized");
			dsi_PrintStr("Description: %s", wsaData.szDescription);
			dsi_PrintStr("Status: %s", wsaData.szSystemStatus);
		}

		return LTTRUE;
	}

	con_Printf("WSAStartup() returned %d\n", nRet);
	return LTFALSE;
}


// 0x1003623e — slot 0x08. Every live connection is told the driver is going
// away and then the sockets follow. The table is circular and holds only its
// tail, so the walk goes backwards through the prev links with the
// cursor-alias idiom, and the tail is re-read each time round because
// SendDisconnect could in principle change it.
void TCPDriver::Term()
{
	TCPConnNode* pCur = m_ConnTable.m_pTail;

	while (pCur)
	{
		void* pConn = pCur->m_pData;
		pCur = (pCur->m_pPrev == m_ConnTable.m_pTail) ? LTNULL : pCur->m_pPrev;

		SendDisconnect((NetConnection*)pConn, 0);
	}

	CloseSockets(1);
}


// 0x1003629d — the disconnect notice: a two-byte header of zeros and the
// reason as four separate bytes, sent four times because nothing
// acknowledges it. The header bytes are written one at a time (two `andb`s,
// not a single `movw`), and the reason likewise, so the six bytes are a
// uint8 array in the source rather than a struct.
void TCPDriver::SendDisconnect(NetConnection* pConn, uint32 nReason)
{
	uint8  packet[6];
	uint32 i;

	packet[0] = 0;
	packet[1] = 0;
	packet[2] = (uint8)nReason;
	packet[3] = (uint8)(nReason >> 8);
	packet[4] = (uint8)(nReason >> 16);
	packet[5] = (uint8)(nReason >> 24);

	for (i = 0; i < 4; i++)
		SendTo(m_Socket, packet, sizeof(packet), &((TCPConnection*)pConn)->m_Addr);
}


// 0x100362f5 — which live connection a datagram came from. The port is
// compared first and the address second; 0x100372d7 is the same walk with
// the two the other way round, and each has to be written the way it
// shipped.
TCPConnection* TCPDriver::FindConnection(struct sockaddr_in* pAddr)
{
	TCPConnNode* pTail = m_ConnTable.m_pTail;
	TCPConnNode* pCur  = m_ConnTable.m_pTail;

	while (pCur)
	{
		TCPConnection* pConn = (TCPConnection*)pCur->m_pData;
		pCur = (pCur->m_pPrev == pTail) ? LTNULL : pCur->m_pPrev;

		if (pConn->m_Addr.sin_port == pAddr->sin_port &&
		    pConn->m_Addr.sin_addr.S_un.S_addr == pAddr->sin_addr.S_un.S_addr)
		{
			return pConn;
		}
	}

	return LTNULL;
}


// 0x1003633c — slot 0x14. One service, described from scratch every time it
// is asked for and prepended to whatever list the caller already has. The
// handle it hands out is the driver's own embedded service record, whose
// only field is the way back to the driver.
LTRESULT TCPDriver::GetServiceList(NetService** ppListHead)
{
	// `::operator new`, not dalloc: 0x1000d25d is the one-argument
	// forwarder in front of the real allocator at 0x1000d2ef.
	NetService* pService = (NetService*)::operator new(sizeof(NetService));

	if (!pService)
		return LT_ERROR;

	m_Service.m_pDriver = this;

	pService->m_handle = (HNETSERVICE)&m_Service;
	pService->m_dwFlags = 1;
	memset(&pService->m_guidService, 0, sizeof(LTGUID));

	char* pName = pService->m_sName;
	strncpy(pName, "Internet TCP/IP", MAX_NETSERVICENAME_LEN - 1);
	pName[MAX_NETSERVICENAME_LEN - 1] = 0;

	pService->m_pNext = *ppListHead;
	*ppListHead = pService;

	return LT_OK;
}


// 0x10036c0a — slot 0x2c. The query socket goes and the responses collected
// against it go with it. Identical in both images; lithtech.exe's
// 0x00485c40 expands the whole release (see TCPQueryArray).
LTRESULT TCPDriver::EndQuery()
{
	if (m_QuerySocket)
	{
		closesocket(m_QuerySocket);
		m_QuerySocket = 0;
	}

	m_Query.Term();

	return LT_OK;
}


// 0x10036cb6 — slot 0x38. The name lives in an exactly-sized block, so a
// name no longer than the one already there is copied straight in and
// anything longer costs a free and a fresh allocation.
//
// One result, set on every arm (the allocation failure included, as an
// `else` assignment) and returned once. Written as early returns, the two
// `strcpy; return LT_OK` tails cross-jump the other way: the shipped build
// keeps the tail in the FIRST arm and jumps backward into it (0x10036d17),
// early returns host it at the last. Byte-identical on both images.
LTRESULT TCPDriver::UpdateSessionName(const char* pName)
{
	LTRESULT dResult;

	if (m_pSessionName && strlen(m_pSessionName) >= strlen(pName))
	{
		strcpy(m_pSessionName, pName);
		dResult = LT_OK;
	}
	else
	{
		::operator delete(m_pSessionName);

		m_pSessionName = (char*)::operator new(strlen(pName) + 1);
		if (m_pSessionName)
		{
			strcpy(m_pSessionName, pName);
			dResult = LT_OK;
		}
		else
		{
			dResult = LT_ERROR;
		}
	}

	return dResult;
}


// 0x10036c88 — slot 0x3c. The buffer is emptied first, so a driver with no
// name answers an empty string rather than leaving the caller's buffer
// alone.
LTRESULT TCPDriver::GetSessionName(char* pName, uint32 nLen)
{
	pName[0] = 0;

	if (m_pSessionName)
	{
		strncpy(pName, m_pSessionName, nLen);
		pName[nLen - 1] = 0;
	}

	return LT_OK;
}


// 0x10036ed3 — slot 0x34. The interface's session record carries the address
// to connect to past the end of the SDK's NetSession, and the join proper
// takes just that address.
LTRESULT TCPDriver::JoinSession(NetSession* pSession)
{
	return JoinSession(&((TCPSession*)pSession)->m_Addr);
}


// 0x10036d22 — slot 0x5c. The manager hears about it first, then the remote
// end, then the connection comes out of the driver's table and is destroyed.
void TCPDriver::Disconnect(NetConnection* pConn, uint32 nReason, uint32 flags)
{
	if (pConn)
	{
		if (g_ShowNetInfo)
		{
			dsi_ServerError("TCPDriver::Disconnect (%d) on %d.%d.%d.%d:%d",
				nReason,
				((TCPConnection*)pConn)->m_Addr.sin_addr.S_un.S_un_b.s_b1,
				((TCPConnection*)pConn)->m_Addr.sin_addr.S_un.S_un_b.s_b2,
				((TCPConnection*)pConn)->m_Addr.sin_addr.S_un.S_un_b.s_b3,
				((TCPConnection*)pConn)->m_Addr.sin_addr.S_un.S_un_b.s_b4,
				ntohs(((TCPConnection*)pConn)->m_Addr.sin_port));
		}

		m_pNetMgr->CloseConnection(pConn);
		// The notice carries `flags`, not the `nReason` the trace above
		// printed: the shipped push is the third parameter's slot.
		SendDisconnect(pConn, flags);

		m_ConnTable.Remove((TCPConnNode*)&((TCPConnection*)pConn)->m_TableNode);
		((TCPConnection*)pConn)->Destroy(1);
	}
}


// 0x100373fa — slot 0x60. A raw send to one connection, with the three
// preconditions AND-merged: the shipped code funnels all three failures into
// one `return LTFALSE` sitting at the very end of the function.
LTBOOL TCPDriver::_p60(void* pData, uint32 nLen, uint32 nUnused,
                       NetConnection* pConn)
{
	if (pConn && m_Socket && ((TCPConnection*)pConn)->m_Unknown_dc)
	{
		return SendTo(m_Socket, pData, nLen,
		              &((TCPConnection*)pConn)->m_Addr);
	}

	return LTFALSE;
}


// 0x100372d7 — the same table walk as FindConnection with the address
// compared before the port, and reached as a free function (the driver is a
// stack argument, not `this`). Name ours.
TCPConnection* tcp_FindConnection(TCPDriver* pDriver, struct sockaddr_in* pAddr)
{
	TCPConnNode* pTail = pDriver->m_ConnTable.m_pTail;
	TCPConnNode* pCur  = pDriver->m_ConnTable.m_pTail;

	while (pCur)
	{
		TCPConnection* pConn = (TCPConnection*)pCur->m_pData;
		pCur = (pCur->m_pPrev == pTail) ? LTNULL : pCur->m_pPrev;

		if (pConn->m_Addr.sin_addr.S_un.S_addr == pAddr->sin_addr.S_un.S_addr &&
		    pConn->m_Addr.sin_port == pAddr->sin_port)
		{
			return pConn;
		}
	}

	return LTNULL;
}


// 0x10058480 — the address the console pinned the local end to, if any; and
// 0x10058494's g_ShowNetInfo again (name ours).
extern char* g_pLocalAddress;

// NEAR (430 of 434 bytes, 148 of 150 instructions, 3 mnemonic blocks, all
// in the address-assembly loop; the frame, the prologue, every call and the
// whole tail now agree).
//
// What closed the rest: the four address bytes and the port live in a LOCAL
// `struct sockaddr_in` -- the trace string even calls this
// SetupLocalSockaddr. The shipped frame is 0x100 of host name, the
// sockaddr at -0x10 (sin_port at -0xe, sin_addr at -0xc) and the spilled
// `this` at -0x14: 0x114. Ours used to park a byte array and the port in the
// dead parameter slots with a 0x104 frame, and that one difference had
// rotated every register in the function.
//
// What is left is the loop, which in the shipped build (0x1003755b) and its
// lithtech.exe twin (0x004868f2) is NOT strength-reduced: an index
// (`(%eax,%edx,1)`, `inc %edx`), a by-8 int (`add $0x8,%edi ; cmp
// $0x20,%edi ; jl`) and the shift recomputed from it every pass (`push
// $0x18 ; pop %ecx ; sub %edi,%ecx`). Every spelling tried here reduces
// something. `nShift` counted DOWN from 24 with `i < 4` as the test (below)
// is the closest: index and prologue as shipped, only the shift is kept as
// its own induction variable (`sub $0x8,%ecx`) and the test is on i.
// Measured, all worse: the old `while (nShift < 32) { ... << (24 - nShift);
// nShift += 8; i++; }` (431, 19 blocks -- the shift collapses into one
// down-counter and a walking pointer, and the allocation rotates again),
// the same as `for`/`do`, with `i++` in the subscript, and through a named
// byte pointer; `24 - i * 8` and `(3 - i) * 8` (430, 5 blocks); `24 - (i <<
// 3)` (436, VC6 re-associates it to `(3 - i) << 3`); int i or a signed
// cast anywhere (19 blocks); nShift as uint8/uint16/short/char (432, 17);
// `(uint8)(24 - nShift)` (430, 14 -- stops the shift reduction but walks
// the pointer and does byte arithmetic); `& 0xff` on a char load (434/149,
// 15); a whole-loop inline helper (not expanded at /O1). This is the SAME
// unreduced loop tcp_BuildSockaddrFromString (0x10035efb) is still a near
// on, so whatever spelling closes one should close the other.
//
// 0x10037430 — slot 0x50. Hands back the local machine's first IP device as
// text, and the hosting port (or 0xffff when this driver is not hosting).
// The console can override the address outright, in which case nothing is
// looked up at all.
//
// NOTE the return values: this answers **1** on success and **0** on
// failure, which is LTBOOL and not the LTRESULT the SDK's ILT*::
// GetTcpIpAddress declares — CNetMgr::GetTcpIpAddress passes the driver's
// answer straight out, so the interface reports LT_ERROR when it worked. It
// is in the shipped bytes; see BUGS.
LTBOOL TCPDriver::GetTcpIpAddress(char* sAddress, uint32 dwBufferSize,
                                  unsigned short& hostPort)
{
	char               sHostName[256];
	struct hostent*    pHost;
	uint32             i;
	int                nShift;
	uint32             nAddr;
	int                nDevices;
	int                nRet;
	struct sockaddr_in localAddr;

	if (m_bHosting)
		hostPort = ntohs(m_HostAddr.sin_port);
	else
		hostPort = NO_HOST_PORT;

	if (g_pLocalAddress && g_pLocalAddress[0])
	{
		strncpy(sAddress, g_pLocalAddress, dwBufferSize);
		sAddress[dwBufferSize - 1] = 0;
		return LTTRUE;
	}

	nRet = gethostname(sHostName, sizeof(sHostName));
	if (nRet != 0)
	{
		con_Printf("gethostname() returned %d\n", nRet);
		return LTFALSE;
	}

	pHost = gethostbyname(sHostName);
	if (!pHost)
	{
		con_Printf("gethostbyname() returned LTNULL in tcp_BuildSockaddrFromString\n");
		return LTFALSE;
	}

	for (nDevices = 0; nDevices < 10000; nDevices++)
	{
		if (!pHost->h_addr_list[nDevices])
			break;
	}

	if (nDevices == 0)
	{
		dsi_PrintString("No IP devices found!");
		return LTFALSE;
	}

	if (g_ShowNetInfo)
	{
		dsi_PrintCount("---- %d IP device%s ----", nDevices,
		               (nDevices > 1) ? "s" : "");

		for (i = 0; (int)i < nDevices; i++)
		{
			uint8* pDev = (uint8*)pHost->h_addr_list[i];
			dsi_PrintAddr("%d.%d.%d.%d", pDev[0], pDev[1], pDev[2], pDev[3]);
		}
	}

	nAddr = 0;
	nShift = 24;
	for (i = 0; i < 4; i++)
	{
		nAddr |= ((uint8*)pHost->h_addr_list[0])[i] << nShift;
		nShift -= 8;
	}

	localAddr.sin_addr.s_addr = ntohl(nAddr);
	localAddr.sin_port = m_HostAddr.sin_port;

	sprintf(sAddress, "%d.%d.%d.%d",
		localAddr.sin_addr.S_un.S_un_b.s_b1, localAddr.sin_addr.S_un.S_un_b.s_b2,
		localAddr.sin_addr.S_un.S_un_b.s_b3, localAddr.sin_addr.S_un.S_un_b.s_b4);

	if (g_ShowNetInfo)
	{
		dsi_ServerError("SetupLocalSockaddr: %d.%d.%d.%d:%d",
			localAddr.sin_addr.S_un.S_un_b.s_b1, localAddr.sin_addr.S_un.S_un_b.s_b2,
			localAddr.sin_addr.S_un.S_un_b.s_b3, localAddr.sin_addr.S_un.S_un_b.s_b4,
			ntohs(localAddr.sin_port));
	}

	return LTTRUE;
}


// 0x10036060 — build the driver. CBaseDriver's own constructor is expanded
// inline ahead of the response array's, which is a call: the 120-second idle
// timeout arrives as an integer immediate (0x42f00000) because /O1 stores a
// float constant without touching the FPU.
TCPDriver::TCPDriver()
{
	char* pName = m_Name;

	m_bHosting = 0;
	m_bWinsockUp = 0;
	m_Socket = 0;
	m_pSessionName = LTNULL;
	m_MaxPlayers = 0;
	m_QuerySocket = 0;

	strncpy(pName, "internet", sizeof(m_Name) - 1);
	pName[sizeof(m_Name) - 1] = 0;

	m_DriverFlags = NETDRIVER_TCPIP;
}

// 0x10036118 — IDENTICAL. The teardown is the whole body, and the
// connection table's destructor, the service record's vptr and the response
// array's release follow it in reverse declaration order. Both of the two
// residuals this used to carry came from modelling the array as a second
// BASE: the missing `movl $0x1004e9bc,(%ecx)` (a base with an out-of-line
// constructor never stores its vptr from here; a MEMBER with an inline
// virtual destructor does), and the `neg`/`sbb`/`and` null test cl 8447 puts
// on every derived-to-base pointer conversion. Making TCPQueryArray a
// polymorphic member at +0x50 removed both at once. See its class comment.
//
// lithtech.exe 0x00484cf0 — IDENTICAL too, now that TCPQueryArray has a
// SetSize2 body for /O2 to expand (the `movl $0x004a3810,0x50(%esi)` and
// the inline release around `call 0x00488170`).
TCPDriver::~TCPDriver()
{
	Term();
}

// 0x10036c32 — slot 0x1c. Run a whole query and hand back what answered:
// start it, poll for three and a half seconds, collect, stop. An error from
// either the start or a poll is returned as it stands.
LTRESULT TCPDriver::GetSessionList(NetSession** ppList, char* pAddresses)
{
	LTRESULT dResult;
	float    fStart;

	*ppList = LTNULL;

	dResult = ((CTCPDriverBase*)this)->StartQuery(pAddresses);
	if (dResult != LT_OK)
		return dResult;

	fStart = dsi_GetTime();

	while (dsi_GetTime() - fStart < 3.5f)
	{
		dResult = ((CTCPDriverBase*)this)->UpdateQuery();
		if (dResult != LT_OK)
			return dResult;
	}

	((CTCPDriverBase*)this)->GetQueryResults(ppList);
	((CTCPDriverBase*)this)->EndQuery();

	return LT_OK;
}

// 0x10036a3c — which collected response came from a given address, or -1.
// The comparison is a memcmp over the whole sockaddr_in, not a field pair
// like the connection-table walks (name ours).
uint32 tcp_FindResponse(struct sockaddr_in* pAddr, TCPQueryArray* pArray)
{
	uint32 i;

	for (i = 0; i < pArray->m_nElements; i++)
	{
		if (memcmp(pAddr, &pArray->m_pArray[i].m_Addr, sizeof(*pAddr)) == 0)
			return i;
	}

	return (uint32)-1;
}


// ===========================================================================
//  0x10036a80 — slot 0x28.  Turn whatever answered the sweep into the SDK's
//  session list: one TCPSession per response that has a name, driver-owned,
//  chained newest-first through the list head the caller hands in.
//
//  WHAT NAMES IT.  The driver's vtable at 0x1004e94c holds 0x10036a80 in
//  slot 0x28, and servermgr.h's CBaseDriver declares that slot
//  `GetQueryResults(NetSession** ppList)`.  Everything else follows from
//  ltbasedefs.h's NetSession layout, which the 325 bytes walk field by
//  field: 0x08 m_guidApp, 0x18 m_guidInst, 0x30 m_Ping, 0x34 m_HostIP,
//  0x54 m_HostPort, 0x58 m_sName, 0x1098 m_pNext, and 0x109c/0x10a0 the two
//  fields TCPSession adds past the end of it.
//
//  Three things read off the bytes rather than guessed:
//
//   * 0x10b0 is sizeof(TCPSession) exactly (0x109c + the driver word + a
//     16-byte sockaddr_in), and the allocation is `new` with the standard
//     /O1 shape -- allocate, test, construct, re-test.  The construction is
//     NetSession's inline `{Clear();}`: store the base vtable 0x1004e9f0,
//     CALL Clear (0x10036bc5, which this file now emits as its own COMDAT
//     and which matches), then store TCPSession's own 0x1004e9ec.
//   * `pSession->m_guidInst.a = (uint32)pResp` (0x10036b4a, `mov %esi,(%edi)`
//     with %edi still the &m_guidInst the memset above used).  The instance
//     GUID is zeroed and then its first dword is overwritten with the
//     address of the response record -- the driver stamping which slot the
//     session came out of.  Odd, and in the bytes.
//
//  IDENTICAL, 325 bytes and 99 instructions, and the last two levers were
//  both about statement order: the driver word has to be set by a
//  CONSTRUCTOR (`new TCPSession(this)` with `TCPSession(TCPDriver* p) :
//  m_pDriver(p) {}`) rather than by a store after the `new`, which is what
//  puts `mov %edi,0x109c(%ebx)` ahead of the derived vtable store at
//  0x10036ae1; and `m_guidInst.a` has to be written BETWEEN the two
//  memsets, not after them.  Written after, VC6 sinks it past the second
//  memset's call.
//   * the dotted address is formatted out of pRESP's copy of the address
//     (0x8c..0x8f of the record) while the port is read back out of the
//     SESSION's copy (0x10a2), which is the asymmetry at 0x10036b51 and
//     0x10036b7f.
// ===========================================================================
LTRESULT TCPDriver::GetQueryResults(NetSession** ppList)
{
	uint32            i;
	TCPQueryResponse* pResp;
	TCPSession*       pSession;
	char*             pName;

	*ppList = LTNULL;

	for (i = 0; i < m_Query.m_nElements; i++)
	{
		pResp = &m_Query.m_pArray[i];

		if (pResp->m_pData)
		{
			pSession = new TCPSession(this);

			if (pSession)
			{
				pSession->m_Addr = pResp->m_Addr;

				pName = pSession->m_sName;
				strncpy(pName, (char*)pResp->m_pData,
				        MAX_NETSESSIONNAME_LEN - 1);
				pName[MAX_NETSESSIONNAME_LEN - 1] = 0;

				pSession->m_Ping = (uint32)(pResp->m_UnknownA0 * 1000.0f);

				memset(&pSession->m_guidInst, 0, sizeof(LTGUID));
				pSession->m_guidInst.a = (uint32)pResp;
				memset(&pSession->m_guidApp, 0, sizeof(LTGUID));

				sprintf(pSession->m_HostIP, "%d.%d.%d.%d",
					pResp->m_Addr.sin_addr.S_un.S_un_b.s_b1,
					pResp->m_Addr.sin_addr.S_un.S_un_b.s_b2,
					pResp->m_Addr.sin_addr.S_un.S_un_b.s_b3,
					pResp->m_Addr.sin_addr.S_un.S_un_b.s_b4);

				pSession->m_HostPort = ntohs(pSession->m_Addr.sin_port);

				pSession->m_pNext = *ppList;
				*ppList = pSession;
			}
		}
	}

	return LT_OK;
}


// 0x10037320 — vtable slot 0x64: one game datagram off the driver's socket,
// with everything that is not game traffic dealt with on the way. LTTRUE only
// when the message came from a connection this driver already knows; a
// datagram from anywhere else is handed to the manager and the loop goes back
// for another (the name is the header's).
//
// Read off the bytes:
//
//   * the receive is in the loop's CONDITION, which is why the call appears
//     twice -- once peeled into the prologue and once at the bottom jumping
//     back to the test. Both -1 and 0 end it.
//   * the four address bytes go into the pMsg parameter's slot, one byte at a
//     time through %al. pMsg lives in %esi from the first instruction, so
//     that slot is dead, and the four-byte array the manager wants is built
//     in it rather than in the frame -- the frame is exactly the sockaddr_in.
//   * the length written to the message is the receive's return truncated to
//     sixteen bits.
//
// BYTE-IDENTICAL on both images (218 bytes here, 254 at 0x00486640 in
// lithtech.exe). It was a 211-byte near until two source facts went in:
//
//   * the buffer and its size go to tcp_RecvFrom through TCPMsgView's inline
//     accessors, which is what materialises them into %eax/%ecx before the
//     pushes. That also stops VC6 cross-jumping the loop's two copies of the
//     receive into one: the head copy keeps `&from` in %ebx (pushed and
//     popped around that call alone), the tail copy uses %edx.
//   * the port is converted into a local BEFORE the handler call. Passed
//     inline as the call's last argument, VC6 loaded the handler's vtable
//     ahead of the ntohs call and kept it in %ebx; as its own statement the
//     handler chain is re-read after the call (0x100373b6), as shipped, and
//     the byte copies free %eax to go through %al.
LTBOOL TCPDriver::ReceivePacket(ServerMessage* pMsg)
{
	struct sockaddr_in  from;
	uint8               senderAddr[4];
	int                 nRecv;
	TCPConnection*      pConn;
	uint16              nPort;

	if (!m_Socket)
		return LTFALSE;

	while ((nRecv = tcp_RecvFrom(this, m_Socket,
	                             ((TCPMsgView*)pMsg)->GetData(),
	                             ((TCPMsgView*)pMsg)->GetDataSize(),
	                             &from)) != -1 &&
	       nRecv != 0)
	{
		pMsg->m_Unknown34 = (uint16)nRecv;

		// A zero first byte is a connection-level message, not game traffic.
		if (pMsg->m_pData[0] == 0)
		{
			HandleConnPacket(pMsg, &from);
		}
		else
		{
			pConn = FindConnection(&from);

			if (pConn)
			{
				pMsg->m_pConnection = (NetConnection*)pConn;
				return LTTRUE;
			}

			if (m_pNetMgr->m_pHandler)
			{
				senderAddr[0] = ((uint8*)&from.sin_addr)[0];
				senderAddr[1] = ((uint8*)&from.sin_addr)[1];
				senderAddr[2] = ((uint8*)&from.sin_addr)[2];
				senderAddr[3] = ((uint8*)&from.sin_addr)[3];

				nPort = ntohs(from.sin_port);
				m_pNetMgr->m_pHandler->ProcessPacket((CPacket*)pMsg,
				                                     senderAddr, nPort);
			}
		}
	}

	return LTFALSE;
}



// ---------------------------------------------------------------------------
//  Views onto objects whose real declarations are local to other translation
//  units.  Same idiom, and deliberately the same CLASS NAMES, as
//  sharedcommon.cpp's CMsgStringWriter and servermsg.cpp's CMsgStringReader:
//  the mangled names have to agree with the one object that defines each.
// ---------------------------------------------------------------------------

// 0x1002609f — servermsg.cpp defines this one (CMsgStringReader).  Returns a
// pointer INTO the packet's own bytes at the read cursor and walks the cursor
// past the string; nothing is copied, which is why UpdateQuery has to strcpy
// the result into its own block before the packet is reused.
class CMsgStringReader
{
public:
	char* ReadStringInPlace();               // 0x1002609f
};

// 0x10026137 — sharedcommon.cpp defines this one (CMsgStringWriter).
class CMsgStringWriter
{
public:
	void WriteString(char* pStr);            // 0x10026137
};

// 0x100240f8 — netmgr.cpp's view of CPacket::ReadType<uint32>.  Same class
// name so the symbol matches netmgr.obj's.
class ServerMessageTypeView
{
public:
	uint32 ReadDWord(uint32* pDummy);        // 0x100240f8
};

// 0x1002294f — netmgr.cpp's CNetMgrSendView::sub_1002294f: hand everything
// parked on a connection's send queue to the local-connection pipe.  Called
// here with `mov 0x48(%ebx),%ecx / push %esi / call` at 0x10037199, i.e. as a
// member of the net manager with the new connection as its one argument.
class CNetMgrSendView
{
public:
	void sub_1002294f(class NetConnection* pConn);   // 0x1002294f
};


// ===========================================================================
//  0x1003669d — slot 0x24.  One turn of the query sweep: re-send the request
//  to everything StartQuery queued if a second has gone by, then drain the
//  query socket, then throw away the payload of anything that has not
//  answered in thirty seconds.
//
//  THE PROTOCOL, read off this function and its answering half below, and
//  cross-checked against StartQuery and JoinSession, which already match:
//
//      byte 0        0            "this is a connection-level datagram", the
//                                 marker ReceivePacket tests
//      byte 1        1            TCPSUB_QUERY   (out, this function)
//                    2            TCPSUB_REPLY   (in,  this function)
//                    3            TCPSUB_CONNECT (out, JoinSession)
//                    0            TCPSUB_DISCONNECT (SendDisconnect)
//      query   ->    guid[16], slot
//      reply   <-    <length-prefixed session name>, slot
//
//  `slot` is the ping cookie: the sender's index into m_SendTimes, echoed
//  back unchanged by the responder.
//
//  CONTROL FLOW (shipped addresses):
//      0x1003669d  prologue; msgOut and msgIn constructed, then resp's ctor
//      0x100366bd  if (!m_QuerySocket)
//      0x100366c2    RETURN_ERROR(1, "UpdateQuery", LT_NOTINITIALIZED)
//                    — the literal at 0x10055894 is what named this function
//      0x100366f0    ...its three destructors, then `push $0x49 / pop %eax`
//      0x1003671e  if (dsi_GetTime() - m_QueryEndTime >= 1.0f) {
//      0x1003673b    msgOut = msg_AllocPacket(...)
//      0x1003675e    msgOut.m_pMsg->m_pData[0] = 0
//      0x10036773    for (i = 0; i < m_nElements; i++)  — the re-send loop
//      0x1003681a    m_QueryEndTime = dsi_GetTime(); }
//      0x10036824  msgIn = msg_AllocPacket(...)
//      0x10036847  while ((nRead = tcp_RecvFrom(...)) != -1)  — peeled head
//      0x1003686f    the reply body
//      0x1003699d    the loop's tail copy of the receive
//      0x100369c3  for (i = 0; i < m_nElements; i++)  — the 30s sweep
//      0x10036a0e  the three destructors, `xor %eax,%eax`, epilogue
//
//  FRAME (`sub $0xc4,%esp`; %ebx = this, %edi = the shared zero, %esi
//  pushed/popped around the middle region only):
//      -0xc4   TCPQueryResponse   resp        (0xa4; m_pData is -0x2c)
//      -0x20   struct sockaddr_in from
//      -0x10   uint8              nSlot
//      -0x0c   CMessageRef        msgOut
//      -0x08   CMessageRef        msgIn
//      -0x04   CAbstractIO* pIO / uint32 i / char* pName  (three scopes, one
//              slot; the /O2 twin splits them across 0x0c/0x18/0x20(esp), so
//              the sharing is /O1's packing and not a source fact)
//
//  DECLARATION ORDER is fixed at the top by CONSTRUCTION order, which is
//  visible: 0x100366b2/b5 zero -0xc and -0x8 (the two CMessageRef ctors) and
//  only then does 0x100366b8 call TCPQueryResponse::TCPQueryResponse on
//  -0xc4.  So msgOut, msgIn, resp — even though resp is the farthest from
//  %ebp.  See the OPEN ITEMS note about the rest of the order.
//
//  NEAR, 927 of 927 bytes / 304 of 304 instructions, 3 mnemonic blocks
//  (six instructions), was 887/24. Frame, register allocation, block layout
//  and both copies of the receive loop's condition now match. Two changes
//  (see TCPMsgView): the re-send loop's two header stores go through an
//  inline Reset(), and the receive goes through GetData()/GetDataSize().
//  Reset alone is what moved the allocation -- the old notes were right that
//  every residual hung off one decision, and the decision was the
//  msgOut.m_pMsg reload between the two stores.
//
//  What is left is the operand order of the strength-reduced `m_pArray +
//  offset`, three sites: shipped `mov 0x54(%ebx),%esi ; add %edi,%esi`
//  (0x10036776), `lea 0x88(%eax,%edi,1)` (0x100367e3) and `mov
//  0x54(%ebx),%esi ; add -0x4(%ebp),%esi` (0x100369cb); ours puts the
//  offset first in each. The /O2 twin has OUR order (`mov %ebp,%esi ; add
//  %edx,%esi` at 0x00485698). Measured and unchanged: `m_pArray + i`,
//  `&m_Query[i]` through an in-class operator[] (which also flips
//  HandleConnPacket), `&m_Query.GetArray()[i]`, a byte-pointer cast of the
//  address, a `TCPQueryArray* pQuery = &m_Query` copy, a `TCPDriver*
//  pDriver = this` copy, an `m_pArray` local loaded before the subscript,
//  GetSize() as the bound, and permutations of the local declarations.
//  Earlier, also no better: the loop pointer declared in the loop's own
//  scope (identical), and `uint8 nType` for `uint32` (worse -- the shipped
//  `movzbl %al,%eax` at 0x1003688b is the byte's promotion for `== 2`).
//
//  The lithtech.exe twin (0x00485590, 1196 bytes) is a size-exact NEAR:
//  /O2 expands the TCPQueryResponse constructor (the `rep stos` at
//  0x004855c1) and the second CPacket::ReadType<uint8> (see there), and the
//  one byte left is the expanded read's address operand order -- shipped
//  `mov (%eax,%edx,1),%al` (m_Pos as base) at 0x004857fc against our
//  `(%edx,%eax,1)`.
// ===========================================================================
LTRESULT TCPDriver::UpdateQuery()
{
	// Constructed in this order — 0x100366b2, 0x100366b5, 0x100366b8 — and
	// destroyed in the reverse of it at both exits (0x100366f0 / 0x10036a0e:
	// resp's `if (m_pData) ::operator delete`, then msgIn's release, then
	// msgOut's).
	CMessageRef        msgOut;
	CMessageRef        msgIn;
	TCPQueryResponse   resp;

	struct sockaddr_in from;
	uint8              nSlot;
	uint32             i;
	uint32             iResp;
	uint32             nType;
	char*              pName;
	int                nRead;
	TCPQueryResponse*  pResp;

	// Level 1, and the name is a bare literal ("UpdateQuery", 0x10055894,
	// pushed as an immediate at 0x100366dd -- not through a static char*).
	// 0x49 = 73 = LT_NOTINITIALIZED; the shipped code even reports it as the
	// string "73" (0x10052a80), which is RETURN_ERROR and not
	// RETURN_ERROR_SYM.
	if (m_QuerySocket == 0)
		RETURN_ERROR(1, "UpdateQuery", LT_NOTINITIALIZED);

	// `fsubs 0x68(%ebx) / fcomps 1.0f / jb` at 0x10036724 -- `jb` is the
	// inversion of `>=`, not of `>`.  StartQuery primes m_QueryEndTime with
	// `dsi_GetTime() - 2.0f`, so the first turn always re-sends.
	if (dsi_GetTime() - m_QueryEndTime >= 1.0f)
	{
		// The same scoped-pIO shape JoinSession already matches.
		{
			CAbstractIO* pIO;
			msgOut.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
			if (pIO)
				pIO->Release();
		}

		// `andb $0x0,(%eax)` at 0x10036767 -- /O1's zero store; the /O2 twin
		// has the plain `movb $0x0,(%ecx)` at 0x00485680.
		msgOut.m_pMsg->m_pData[0] = 0;

		for (i = 0; i < m_Query.m_nElements; i++)
		{
			// %esi carries `&m_pArray[i]` across both writer calls at
			// 0x10036776/0x1003677b, and every field in the ping block is
			// reached through it (`0x80(%esi)`, `(%esi,%eax,4)`) -- only a
			// named local does that.  The SendTo below subscripts the array
			// AGAIN, which is why 0x100367e0 reloads m_pArray.
			pResp = &m_Query.m_pArray[i];

			// Written every iteration because the one packet is reused:
			// 0x38 first, then 0x34 (0x1003677d/0x10036783, and the same
			// order at 0x0048569f/0x004856a3), both through one load of
			// the packet pointer -- an inline, see TCPMsgView.
			((TCPMsgView*)msgOut.m_pMsg)->Reset();
			msgOut.m_pMsg->WriteByte(1);
			msgOut.m_pMsg->WriteData(&m_pNetMgr->m_Guid, sizeof(LTGUID));

			// m_Unknown80 is READ TWICE in the shipped code -- once into
			// %eax for the 0xff test and the increment (0x100367a4), once
			// as a byte into %cl for the mask (0x100367b6) -- which is the
			// "a member named twice in source is loaded twice" rule, and is
			// why the mask is written as its own statement rather than as
			// `m_Unknown80++ & 0x1f`.  The byte-wide load is the uint32
			// field / uint8 local truncation.
			if (pResp->m_Unknown80 == 0xff)
			{
				// `or %al,-0x10(%ebp)` at 0x100367b1: /Os stores all-ones
				// through the 0xff already sitting in %al rather than
				// spending a 4-byte `movb $0xff`.  The /O2 twin has the
				// plain `mov %al,0x14(%esp)` at 0x004856cc.
				nSlot = 0xff;
			}
			else
			{
				nSlot = (uint8)(pResp->m_Unknown80 & 0x1f);
				pResp->m_Unknown80++;
				// `fstps (%esi,%eax,4)` at 0x100367d2 -- a float array at
				// offset 0 of the record, i.e. m_Text reinterpreted.  See
				// the RENAMES block: this is m_SendTimes[nSlot].
				((float*)pResp->m_Text)[nSlot] = dsi_GetTime();
			}

			// `push -0x10(%ebp)` at 0x100367d5: a uint8 argument, pushed as
			// the dword it sits in.  Not our invention -- HandleConnPacket
			// does the same at 0x10037003 and the /O2 twin at 0x004856f8.
			msgOut.m_pMsg->WriteByte(nSlot);

			// m_pArray is re-read here (0x100367e0) rather than reusing the
			// `m_pArray + i*0xa4` the block above computed: the two uses are
			// in different basic blocks, so VC6's block-local CSE drops it.
			// That is the evidence that the source subscripts the array
			// again instead of naming a pointer -- a named pointer would
			// have survived in %esi.
			SendTo(m_QuerySocket, msgOut.m_pMsg->m_pData,
			       msgOut.m_pMsg->m_Unknown34, &m_Query.m_pArray[i].m_Addr);
		}

		m_QueryEndTime = dsi_GetTime();
	}

	{
		CAbstractIO* pIO;
		msgIn.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
		if (pIO)
			pIO->Release();
	}

	// The receive is in the CONDITION, which is what puts one copy of the
	// call in the prologue (0x1003684a) and one at the bottom (0x100369b0)
	// jumping back into the body -- the shape ReceivePacket's note already
	// describes.  Unlike ReceivePacket this one ends on -1 ONLY; a
	// zero-length datagram keeps the loop going.
	while ((nRead = tcp_RecvFrom(this, m_QuerySocket,
	                             ((TCPMsgView*)msgIn.m_pMsg)->GetData(),
	                             ((TCPMsgView*)msgIn.m_pMsg)->GetDataSize(),
	                             &from)) != -1)
	{
		msgIn.m_pMsg->m_Unknown34 = (uint16)nRead;
		msgIn.m_pMsg->m_Unknown38 = 1;

		nType = msgIn.m_pMsg->ReadByte(0);

		if (!(msgIn.m_pMsg->m_MsgFlags & MSGFLAG_READERROR))
		{
			// 2 = TCPSUB_REPLY, the answer HandleConnPacket writes at
			// 0x10036fdb.  Spelled as the literal because tcpdriver.cpp's
			// CONNMSG_* block only names 5/6/7; if that block grows a
			// TCPSUB_* set, this is TCPSUB_QUERYREPLY.
			if (nType == 2)
			{
				// Points INTO the packet, so it has to be copied out below
				// before the packet is reused (0x100368a1).
				pName = ((CMsgStringReader*)msgIn.m_pMsg)->ReadStringInPlace();
				// The same CPacket::ReadType<uint8> as the ReadByte stand-in
				// above, spelled as the template: lithtech.exe's /O2 build
				// EXPANDS this second read in place (0x004857d5 ..
				// 0x00485842, the "Packet read overrun!" report inline)
				// while it calls the first one out of line (0x004857af ->
				// 0x00459f80). server.dll calls 0x100138d6 for both, and
				// there the template spelling is kept out: instantiating
				// ReadType<uint8> in this TU flips HandleConnPacket
				// (0x10036ee5) to its two-instruction reload swap -- the
				// count sensitivity described at TCPMsgView.
#ifdef LT_SERVER_BUILD
				nSlot = msgIn.m_pMsg->ReadByte(0);
#else
				nSlot = ((CPacket*)msgIn.m_pMsg)->ReadType((uint8*)0);
#endif

				if (!(msgIn.m_pMsg->m_MsgFlags & MSGFLAG_READERROR))
				{
					// `lea 0x50(%ebx),%eax` at 0x100368c2 is the
					// TCPQueryArray base subobject -- the implicit
					// derived-to-base conversion, not a member's address.
					iResp = tcp_FindResponse(&from, &m_Query);

					if (iResp == (uint32)-1)
					{
						memset(&resp, 0, sizeof(resp));
						resp.m_Addr = from;

						if (!m_Query.Append(resp))
							continue;

						// m_nElements re-read after the insert
						// (0x10036915), then the second -1 test at
						// 0x10036919 -- redundant against an array that
						// just grew, but it is in the bytes.
						iResp = m_Query.m_nElements - 1;
						if (iResp == (uint32)-1)
							continue;
					}

					// A NAMED pointer, unlike the re-send loop above: %esi
					// carries it across dsi_GetTime, ::operator delete,
					// strlen, ::operator new and strcpy (0x10036927 to
					// 0x10036997) with no reload, which only a named local
					// does.
					pResp = &m_Query.m_pArray[iResp];

					pResp->m_Time = dsi_GetTime();

					// Not dfree: 0x10028ace is the operator-delete
					// forwarder, the same one CloseSockets uses.
					if (pResp->m_pData)
						::operator delete(pResp->m_pData);

					pResp->m_pData = (uint8*)::operator new(strlen(pName) + 1);
					if (pResp->m_pData)
						strcpy((char*)pResp->m_pData, pName);

					// The ping.  The 0xff test is unreachable after the
					// 0x20 test -- and both are in the shipped code, in
					// this order (0x1003696b, 0x10036971), so both are in
					// the source.
					if (nSlot < 0x20 && nSlot != 0xff)
					{
						if (((float*)pResp->m_Text)[nSlot] == 0.0f)
						{
							// `fldz` at 0x1003698c.
							pResp->m_UnknownA0 = 0.0f;
						}
						else
						{
							// The address of the slot, not its value, is
							// what %edi carries across the call
							// (`lea (%esi,%eax,4),%edi` at 0x10036984),
							// which is what naming the subscript twice
							// gives.
							pResp->m_UnknownA0 = dsi_GetTime() -
								((float*)pResp->m_Text)[nSlot];
						}
					}
				}
			}
		}
	}

	// Thirty seconds (0x41f00000 at 0x10051c6c), and `fcomps / jbe` means
	// strictly greater.  Only the payload goes; the record itself stays in
	// the array so the responder keeps its place and its send times.
	for (i = 0; i < m_Query.m_nElements; i++)
	{
		pResp = &m_Query.m_pArray[i];
		if (dsi_GetTime() - pResp->m_Time > 30.0f)
		{
			if (pResp->m_pData)
			{
				::operator delete(pResp->m_pData);
				pResp->m_pData = LTNULL;
			}
		}
	}

	return LT_OK;
}


// ===========================================================================
//  0x10036ee5 — a datagram whose first byte is zero is connection-level
//  traffic rather than game traffic, and this is everything that can be:
//  a query, a connect request, or a disconnect notice.  Reached by direct
//  call from ReceivePacket (0x10037320) and from nowhere else.
//
//  void, not LTBOOL — see change (2) at the top of this file.
//
//  CONTROL FLOW (shipped addresses):
//      0x10036ee5  prologue; msgOut constructed; m_Unknown38 = 1
//      0x10036f01  nType = pMsg->ReadByte(0)
//      0x10036f0d  if (read error) return          — jumps INTO the tail at
//                                                    0x100372c9
//      0x10036f19  if (m_bHosting) {
//      0x10036f25    if (tcp_FindConnection(this, pFrom)) return
//      0x10036f36    if (nType == 1) {             — the query
//      0x10036f40      guid check, trace, reply, return
//      0x1003702c    } else if (nType == 3) {      — the connect request
//      0x10037036      trace
//      0x1003706f      skip a byte, read the guid, read-error check
//      0x1003708f      bBadGuid = 0
//      0x10037093      guid check -> bBadGuid = 1, goto reject
//      0x100370ba      capacity check -> goto reject
//      0x100370cc      new TCPConnection, AddConnection
//      0x1003711e      accept: table, five accept packets, return
//      0x100371a7      AddConnection failed: destroy, fall through
//      0x100371bb      the reject packet (6 or 7), falls through
//                    } }
//      0x10037219  if (nType == 0) {               — the disconnect
//      0x10037223    FindConnection, notify, close, unlink, destroy
//                  }
//      0x100372c4  the shared tail: ~CMessageRef, epilogue, `ret $8`
//
//  FRAME (`sub $0x20,%esp`; %ebx = this, %edi = pMsg, %esi = pFrom):
//      -0x20   LTGUID          guid
//      -0x10   uint32          bBadGuid
//      -0x0c   TCPConnection*  pConn
//      -0x08   CMessageRef     msgOut
//      -0x01   uint8           nType
//      +0x08   the pMsg parameter slot, dead from the first instruction and
//              reused for nSub (0x10036fad) and for one pIO (0x10037134)
//      +0x0c   the pFrom parameter slot, likewise, reused for two pIOs
//              (0x10036fb0, 0x100371bb)
//      Both reuses are the register allocator's, not the source's; the /O2
//      twin keeps nSub in 0x28(%esp) (its own dead pMsg slot) and the pIOs
//      in real frame.
// ===========================================================================
void TCPDriver::HandleConnPacket(ServerMessage* pMsg, struct sockaddr_in* pFrom)
{
	CMessageRef    msgOut;
	TCPConnection* pConn;
	uint32         bBadGuid;
	LTGUID         guid;
	uint8          nType;
	uint8          nSub;
	uint32         i;

	// Byte 0 is the zero marker ReceivePacket already tested, so the read
	// starts at 1.
	pMsg->m_Unknown38 = 1;
	nType = pMsg->ReadByte(0);

	if (pMsg->m_MsgFlags & MSGFLAG_READERROR)
		return;

	if (m_bHosting)
	{
		// The FREE-function walk (0x100372d7), the one that compares the
		// address before the port -- not the member at 0x100362f5 the
		// disconnect arm below uses.  The driver goes in as a stack
		// argument: `push %esi / push %ebx / call` at 0x10036f25.
		if (tcp_FindConnection(this, pFrom))
			return;

		if (nType == 1)
		{
			pMsg->ReadData(&guid, sizeof(LTGUID));

			// OPERAND ORDER IS LOAD-BEARING and the two guid checks in this
			// function disagree with each other.  Here the local goes
			// first: `push $0x10 / push &m_Guid / push &guid` at
			// 0x10036f4d-0x10036f5b, and the /O2 twin's inline compare has
			// %esi = &guid, %edi = &m_Guid at 0x004860b4.  The nType == 3
			// arm below has it the other way round in both images.
			if (memcmp(&guid, &m_pNetMgr->m_Guid, sizeof(LTGUID)) != 0)
				return;

			if (g_ShowNetInfo)
			{
				dsi_ServerError("IP: Received query from %d.%d.%d.%d:%d, sending response",
					pFrom->sin_addr.S_un.S_un_b.s_b1, pFrom->sin_addr.S_un.S_un_b.s_b2,
					pFrom->sin_addr.S_un.S_un_b.s_b3, pFrom->sin_addr.S_un.S_un_b.s_b4,
					ntohs(pFrom->sin_port));
			}

			// The ping cookie, echoed back untouched below.
			nSub = pMsg->ReadByte(0);

			{
				CAbstractIO* pIO;
				msgOut.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
				if (pIO)
					pIO->Release();
			}

			// m_Unknown34 / m_Unknown38 are NOT set here the way
			// UpdateQuery's loop sets them: this packet is fresh, and
			// ServerMessage::Init (reached through msg_AllocPacket) has
			// already put 1 in both.
			msgOut.m_pMsg->m_pData[0] = 0;
			msgOut.m_pMsg->WriteByte(2);
			// `mov 0x98(%ebx),%eax / test / jne / mov $0x10056ff4,%eax /
			// push %eax` at 0x10036feb -- the ternary shape.  0x10056ff4 is
			// servermsg.h's g_EmptyString (lithtech 0x004b0ab4, which
			// de_error.h knows as g_ShutdownMessage; same buffer).
			((CMsgStringWriter*)msgOut.m_pMsg)->WriteString(
				m_pSessionName ? m_pSessionName : g_EmptyString);
			msgOut.m_pMsg->WriteByte(nSub);

			SendTo(m_Socket, msgOut.m_pMsg->m_pData,
			       msgOut.m_pMsg->m_Unknown34, pFrom);
			return;
		}
		else if (nType == 3)
		{
			if (g_ShowNetInfo)
			{
				dsi_ServerError("IP: Received conn request from %d.%d.%d.%d:%d, sending response",
					pFrom->sin_addr.S_un.S_un_b.s_b1, pFrom->sin_addr.S_un.S_un_b.s_b2,
					pFrom->sin_addr.S_un.S_un_b.s_b3, pFrom->sin_addr.S_un.S_un_b.s_b4,
					ntohs(pFrom->sin_port));
			}

			// JoinSession writes `WriteByte(3); WriteByte(0); WriteData(guid)`
			// -- this is the discard of that second byte.  The result is
			// dropped in both images (0x1003706f, 0x004861eb).
			pMsg->ReadByte(0);
			pMsg->ReadData(&guid, sizeof(LTGUID));

			// This arm checks for the read error; the query arm above does
			// not.  Both images agree.
			if (pMsg->m_MsgFlags & MSGFLAG_READERROR)
				return;

			bBadGuid = 0;

			// The reversed operand order: `push $0x10 / push &guid /
			// push &m_Guid` at 0x10037093-0x100370a1, and %esi = &m_Guid /
			// %edi = &guid in the /O2 inline at 0x00486208.
			if (memcmp(&m_pNetMgr->m_Guid, &guid, sizeof(LTGUID)) != 0)
			{
				bBadGuid = 1;
			}
			else if (m_ConnTable.m_nItems < m_MaxPlayers)
			{
				// NO null test after this, and the field stores below go
				// through the returned pointer unguarded (0x100370f9
				// onward): a null `new` faults.  It is in the shipped
				// bytes of both images -- see BUGS.
				pConn = new TCPConnection;

				pConn->m_Addr = *pFrom;
				// Zero here and 1 only once the connection is fully filed
				// (0x1003718f); JoinSession's client-side copy sets 1
				// straight away.
				pConn->m_Unknown_dc = 0;
				pConn->m_pDriver = (CBaseDriver*)this;

				if (m_pNetMgr->AddConnection(pConn))
				{
					m_ConnTable.Add(pConn, (TCPConnNode*)&pConn->m_TableNode);

					{
						CAbstractIO* pIO;
						msgOut.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
						if (pIO)
							pIO->Release();
					}

					msgOut.m_pMsg->m_pData[0] = 0;
					msgOut.m_pMsg->WriteByte(CONNMSG_ACCEPTED);

					// Five, not four: `push $0x5 / pop %edi` at 0x1003715f
					// spends ONE constant on both the payload byte and the
					// loop count, and the loop is `dec %edi / jne` from 5,
					// which is five passes.  The /O2 twin proves it is two
					// separate 5s in the source -- `push $0x5` at
					// 0x00486306 and `mov $0x5,%ebp` at 0x00486312.
					// (SendDisconnect, by contrast, really does send four.)
					for (i = 0; i < 5; i++)
					{
						SendTo(m_Socket, msgOut.m_pMsg->m_pData,
						       msgOut.m_pMsg->m_Unknown34, pFrom);
					}

					pConn->m_Unknown_dc = 1;
					((CNetMgrSendView*)m_pNetMgr)->sub_1002294f(pConn);
					return;
				}

				if (pConn)
					pConn->Destroy(1);
			}

			// The reject, shared by all three failures.  Reached by fallthrough
			// from the AddConnection failure and by `jmp` from the guid and
			// capacity checks (0x100370b5, 0x100370c6).
			{
				CAbstractIO* pIO;
				msgOut.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
				if (pIO)
					pIO->Release();
			}

			msgOut.m_pMsg->m_pData[0] = 0;
			// One call with a merged push in /O1 (`push $7 / jmp / push $6`
			// into one `call` at 0x100371f2-0x100371fb), which is the
			// ternary; /O2 duplicates the call into both arms instead.
			// AN if/else, NOT A TERNARY. The shipped code branches --
			// `je ; push $A ; jmp ; push $B` -- where a ternary over two
			// constants gives VC6 the branchless `setne %al ; add $X,%eax`
			// our build was emitting. That was the ONLY mnemonic block left
			// in this function.
			// lithtech.exe calls the first write (0x004863b4 -> 0x004159f0)
			// and expands the second in place (0x004863be .. 0x0048641d,
			// the "Packet write overrun!" report inline); server.dll calls
			// both. See the note at UpdateQuery's second read.
			if (bBadGuid)
				msgOut.m_pMsg->WriteByte(CONNMSG_BADGUID);
			else
#ifdef LT_SERVER_BUILD
				msgOut.m_pMsg->WriteByte(CONNMSG_REJECTED);
#else
				((CPacket*)msgOut.m_pMsg)->WriteType((uint8)CONNMSG_REJECTED);
#endif

			// Sent once, not five times.
			SendTo(m_Socket, msgOut.m_pMsg->m_pData,
			       msgOut.m_pMsg->m_Unknown34, pFrom);
		}
	}

	// Reached by fallthrough out of the reject above, by `jmp` from
	// `!m_bHosting` (0x10036f1f) and from `nType != 3` (0x10037030).
	if (nType == 0)
	{
		// The MEMBER walk this time (0x100362f5, port before address), not
		// the free one the hosting arm used.
		pConn = FindConnection(pFrom);

		if (pConn)
		{
			if (g_ShowNetInfo)
			{
				dsi_ServerError("Got TCPSUB_DISCONNECT from %d.%d.%d.%d:%d",
					pFrom->sin_addr.S_un.S_un_b.s_b1, pFrom->sin_addr.S_un.S_un_b.s_b2,
					pFrom->sin_addr.S_un.S_un_b.s_b3, pFrom->sin_addr.S_un.S_un_b.s_b4,
					ntohs(pFrom->sin_port));
			}

			// Character for character the notice netmgr.cpp already sends at
			// its own line 1750, down to g_EmptyString and the argument
			// order.  m_pHandler is read twice -- the guard's copy supplies
			// the vtable (`mov (%eax),%esi` at 0x1003727c) and the call
			// re-reads the field for `this` (0x10037292) -- which is the
			// two-mentions rule again.
			if (m_pNetMgr->m_pHandler)
			{
				// lithtech.exe expands the read in place (0x004864e3 ..);
				// see the note at UpdateQuery's second read.
#ifdef LT_SERVER_BUILD
				m_pNetMgr->m_pHandler->_v10(4, g_EmptyString,
					((ServerMessageTypeView*)pMsg)->ReadDWord(0));
#else
				m_pNetMgr->m_pHandler->_v10(4, g_EmptyString,
					((CPacket*)pMsg)->ReadType((uint32*)0));
#endif
			}

			m_pNetMgr->CloseConnection(pConn);
			m_ConnTable.Remove((TCPConnNode*)&pConn->m_TableNode);
			pConn->Destroy(1);
		}
	}
}


// ===========================================================================


#ifndef LT_SERVER_BUILD
// lithtech.exe 0x004871e0, 27 bytes -- servermgr.h's tcp_Create, the TCP
// driver's factory: `push $0xb0; call operator new; test; je; mov %eax,%ecx;
// jmp 0x00484c10` (the constructor tail-called), else 0. Its one caller is
// 0x0045609b, CNetMgr::AddDriver's `pDriver = tcp_Create();` after the
// "internet" compare. Lithtech-only here: server.dll's copy (0x10037c32) is
// already written in server/worldtree.cpp, and a second definition in the
// server build would claim another address of the same shape.
CBaseDriver* tcp_Create()
{
	return (CBaseDriver*)new TCPDriver;
}

// The TCP driver's trivial slots, read off lithtech.exe's vtable 0x004a37a4.
// Every one is a body the linker folded onto a shared one elsewhere in the
// image, which is what their addresses are:
//   0x0c Update               0x00490200  `ret`
//   0x10 _p10                 0x00459f70  `ret $4`  (one argument, unread)
//   0x18 SelectService        0x0042d1b0  `xor %eax,%eax; ret $4`
//   0x40 IsLobbyLaunched      0x00432040  `xor %eax,%eax; ret`
//   0x44 GetLobbyLaunchInfo   0x0042d1b0  `xor %eax,%eax; ret $4`
//   0x48 _p48                 0x0042d190  `mov $1,%eax; ret $4`
//   0x4c _p4c                 0x00432040  `xor %eax,%eax; ret`
//   0x68 GetRecvSize          0x00484cc0  `mov $0x1c,%eax; ret`
// (0x54 and 0x58 are tcpqarray.cpp's TCPDriverC4::ConnectTCP 0x004870f0 and
// SendToAddr 0x00487160; link/aliases.rsp joins them.)
void TCPDriver::Update()
{
}

void TCPDriver::_p10(void* pUnused)
{
}

int TCPDriver::SelectService(HNETSERVICE hService)
{
	return 0;
}

LTBOOL TCPDriver::IsLobbyLaunched()
{
	return LTFALSE;
}

LTBOOL TCPDriver::GetLobbyLaunchInfo(void** ppData)
{
	return LTFALSE;
}

LTBOOL TCPDriver::_p48(void* pUnused)
{
	return LTTRUE;
}

LTBOOL TCPDriver::_p4c()
{
	return LTFALSE;
}

uint32 TCPDriver::GetRecvSize()
{
	return 0x1c;
}
#endif
