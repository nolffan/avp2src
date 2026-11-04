// server.dll — ILTCSBase message writers (reconstruction). Build /O1.
#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x410880).
#include <string>
#endif
#include <math.h>
#include <stdio.h>
#include "servermsg.h"
#include "de_error.h"
#include "packet.h"
#ifdef AVP2_MODERN
// Declared rather than taken from <windows.h>, as avp2src_debug.h does.
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);
#endif

LTRESULT CLTServer::WriteToMessageByte(HMESSAGEWRITE_I hMessage, uint8 val)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageByte");

	return hMessage->WriteByte(val);
}

LTRESULT CLTServer::WriteToMessageWord(HMESSAGEWRITE_I hMessage, uint16 val)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageWord");

	return hMessage->WriteWord(val);
}

LTRESULT CLTServer::WriteToMessageDWord(HMESSAGEWRITE_I hMessage, uint32 val)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageDWord");

	return hMessage->WriteDWord(val);
}

LTRESULT CLTServer::WriteToMessageFloat(HMESSAGEWRITE_I hMessage, float val)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageFloat");

	return hMessage->WriteFloat(val);
}

LTRESULT CLTServer::WriteToMessageString(HMESSAGEWRITE_I hMessage, char* pStr)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageString");

	return hMessage->WriteString(pStr);
}

LTRESULT CLTServer::WriteToMessageHString(HMESSAGEWRITE_I hMessage, HSTRING hStr)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageHString");

	return hMessage->WriteHString(hStr);
}

LTRESULT CLTServer::WriteToMessageHMessageWrite(HMESSAGEWRITE_I hMessage, void* hData)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageHMessageWrite");

	return hMessage->WriteMessageHandle(hData);
}

LTRESULT CLTServer::WriteToMessageHMessageRead(HMESSAGEWRITE_I hMessage, void* hData)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageHMessageRead");

	return hMessage->WriteMessageHandle(hData);
}

LTRESULT CLTServer::WriteToMessageVector(HMESSAGEWRITE_I hMessage, LTVector* pVal)
{
	if (hMessage && pVal)
		return hMessage->WriteVector(pVal);

	RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageVector");
}

LTRESULT CLTServer::WriteToMessageCompVector(HMESSAGEWRITE_I hMessage, LTVector* pVal)
{
	if (hMessage && pVal)
		return hMessage->WriteCompVector(pVal);

	RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageCompVector");
}

LTRESULT CLTServer::WriteToMessageCompPosition(HMESSAGEWRITE_I hMessage, LTVector* pVal)
{
	if (hMessage && pVal)
		return hMessage->WriteCompPos(pVal);

	RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageCompPosition");
}

LTRESULT CLTServer::WriteToMessageRotation(HMESSAGEWRITE_I hMessage, LTRotation* pVal)
{
	if (hMessage && pVal)
		return hMessage->WriteRotation(pVal);

	RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageRotation");
}


// --- ILTCSBase readers -------------------------------------------------- //
// Value-returning readers: the out-param temp is a local; VC6 /O1 reuses the
// (dead) incoming argument slot for it, which is why these carry an ebp frame.

float CLTServer::ReadFromMessageFloat(HMESSAGEREAD_I hMessage)
{
	float val;

	if (!hMessage)
		RETURN_ERROR_VALUE("ILTCSBase::ReadFromMessageFloat", 0.0f);

	hMessage->ReadFloat(&val);
	return val;
}

uint8 CLTServer::ReadFromMessageByte(HMESSAGEREAD_I hMessage)
{
	uint8 val;

	if (!hMessage)
		RETURN_ERROR_VALUE("ILTCSBase::ReadFromMessageByte", 0);

	hMessage->ReadByte(&val);
	return val;
}

uint16 CLTServer::ReadFromMessageWord(HMESSAGEREAD_I hMessage)
{
	uint16 val;

	if (!hMessage)
		RETURN_ERROR_VALUE("ILTCSBase::ReadFromMessageWord", 0);

	hMessage->ReadWord(&val);
	return val;
}

uint32 CLTServer::ReadFromMessageDWord(HMESSAGEREAD_I hMessage)
{
	uint32 val;

	if (!hMessage)
		RETURN_ERROR_VALUE("ILTCSBase::ReadFromMessageDWord", 0);

	hMessage->ReadDWord(&val);
	return val;
}

HOBJECT CLTServer::ReadFromMessageObject(HMESSAGEREAD_I hMessage)
{
	HOBJECT val;

	if (!hMessage)
		RETURN_ERROR_VALUE("ILTCSBase::ReadFromMessageObject", NULL);

	hMessage->ReadObject(&val);
	return val;
}

HSTRING CLTServer::ReadFromMessageHString(HMESSAGEREAD_I hMessage)
{
	HSTRING val;

	if (!hMessage)
		RETURN_ERROR_VALUE("ILTCSBase::ReadFromMessageHString", NULL);

	hMessage->ReadHString(&val);
	return val;
}

void* CLTServer::ReadFromMessageHMessageRead(HMESSAGEREAD_I hMessage)
{
	void* val;

	if (!hMessage)
		RETURN_ERROR_VALUE("ILTCSBase::ReadFromMessageHMessageRead", NULL);

	val = NULL;
	hMessage->ReadMessage(&val);
	return val;
}

// Void readers with a caller-supplied out pointer.

void CLTServer::ReadFromMessageVector(HMESSAGEREAD_I hMessage, LTVector* pVal)
{
	if (hMessage && pVal)
	{
		hMessage->ReadVector(pVal);
		return;
	}

	RETURN_ERROR_VOID("ILTCSBase::ReadFromMessageVector");
}

void CLTServer::ReadFromMessageCompVector(HMESSAGEREAD_I hMessage, LTVector* pVal)
{
	if (hMessage && pVal)
	{
		hMessage->ReadCompVector(pVal);
		return;
	}

	RETURN_ERROR_VOID("ILTCSBase::ReadFromMessageCompVector");
}

void CLTServer::ReadFromMessageCompPosition(HMESSAGEREAD_I hMessage, LTVector* pVal)
{
	if (hMessage && pVal)
	{
		hMessage->ReadCompPos(pVal);
		return;
	}

	RETURN_ERROR_VOID("ILTCSBase::ReadFromMessageCompPosition");
}

void CLTServer::ReadFromMessageRotation(HMESSAGEREAD_I hMessage, LTRotation* pVal)
{
	if (hMessage && pVal)
	{
		hMessage->ReadRotation(pVal);
		return;
	}

	RETURN_ERROR_VOID("ILTCSBase::ReadFromMessageRotation");
}

// Message lifetime helpers.

void CLTServer::EndHMessageRead(HMESSAGEREAD_I hMessage)
{
	if (!hMessage)
		RETURN_ERROR_VOID("ILTCSBase::EndHMessageRead");

	hMessage->EndRead();
}

void CLTServer::EndHMessageWrite(HMESSAGEWRITE_I hMessage)
{
	if (!hMessage)
		RETURN_ERROR_VOID("ILTCSBase::EndHMessageWrite");

	hMessage->EndWrite();
}

void CLTServer::ResetRead(HMESSAGEREAD_I hMessage)
{
	if (!hMessage)
		RETURN_ERROR_VOID("ILTCSBase::ResetRead");

	hMessage->ResetRead();
}

// The remaining three ILTCSBase message methods.

LTRESULT CLTServer::WriteToMessageFormattedHString(HMESSAGEWRITE_I hMessage, char* pMsg, ...)
{
	va_list marker;

	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageFormattedHString");

	va_start(marker, pMsg);
	hMessage->WriteFormattedHString(pMsg, &marker);
	return LT_OK;
}

LTRESULT CLTServer::WriteToMessageObject(HMESSAGEWRITE_I hMessage, HOBJECT hObj)
{
	if (!hMessage)
		RETURN_ERROR_PARAM("ILTCSBase::WriteToMessageObject");

	return hMessage->WriteObject(hObj);
}

LTRESULT CLTServer::ReadFromLoadSaveMessageObject(HMESSAGEREAD_I hMessage, HOBJECT* pObj)
{
	if (hMessage && pObj)
		return hMessage->ReadObject(pObj);

	RETURN_ERROR_PARAM("ILTCSBase::ReadFromLoadSaveMessageObject");
}


// *********************************************************************** //
//  The message payload movers, the packet allocator and the three
//  compressed codecs. Everything below reads its field offsets, its bit
//  layout and its argument counts off the retail server.dll; where the
//  meaning of a neighbour could not be recovered it is named by its address.
// *********************************************************************** //


// 0x10058790 — the spare-message free list: a count at +0x00 and the head
// link at +0x04. 0x10026234 unlinks one entry, fixes the head if it was the
// one removed, and drops the count. It is netmgr.cpp's g_NetBankList, which
// that file defines with its constructor and destructor (the initialiser
// group at 0x10025f92), so this is the same class seen from this side,
// carrying the two list operations this file uses. The running total that
// only ever goes up is not a field of it: 0x1005879c is netmgr.cpp's own
// g_nUnknown79c (0x10058798 between them is that file's g_nNetBankRefs).
class CNetBankList
{
public:
	uint32  m_nCount;                        // +0x00
	LTLink* m_pHead;                         // +0x04

	// 0x100261eb — put one back at the head of the list. The same function
	// serves the TCP driver's query table (tcpdriver.cpp), which is what
	// settles the argument order: the carried pointer first, the link
	// second (name ours).
	//
	// IN-CLASS: lithtech.exe's ServerMessage::_v0c (0x0045c1d0, 84 bytes)
	// carries this whole body expanded, with no call left in it. The splice
	// is tcpdriver.cpp's TCPConnTable::Add in LTLink's field names (m_pPrev
	// at +0, so `[[head]+4] = link` is head->m_pPrev->m_pNext) and the empty
	// arm is TieOff (m_pNext stored before m_pPrev, 0x0045c210). server.dll
	// still calls 0x100261eb out of line from _v0c (0x10026099): /Os
	// declines it, and tcpdriver.obj's copy is the one that matches there.
	void Add(void* pData, LTLink* pLink)     // 0x100261eb
	{
		LTLink* pAt = m_pHead;

		if (pAt)
		{
			pAt->m_pPrev->m_pNext = pLink;
			pLink->m_pPrev = pAt->m_pPrev;
			pLink->m_pNext = pAt;
			pAt->m_pPrev = pLink;
			pLink->m_pData = pData;

			m_nCount++;

			if (pLink->m_pNext == m_pHead)
				m_pHead = pLink;
		}
		else
		{
			pLink->TieOff();
			pLink->m_pData = pData;

			m_pHead = pLink;
			m_nCount++;
		}
	}

	// 0x10026234. Unlink one entry from the free list: the two splice
	// stores, then the head fixed up if it was the entry removed, then the
	// count. Both splice stores reload the link's own fields out of memory —
	// the link is address-exposed, so the second line cannot reuse the
	// first's registers — and there is no TieOff afterwards: the record is
	// about to be handed out, so its own pointers are left as they were.
	//
	// The last two lines are separate tests, not one: `decl (%ecx)` feeds
	// its own `jne`, so the emptied-list case is `m_nCount--; if (m_nCount
	// == 0)` and not a compare against one before the decrement.
	//
	// IN-CLASS, hence an /Ob1 expansion candidate, because lithtech.exe's
	// /O2 build has no standalone copy of it: msg_AllocPacket (0x0045c080,
	// 336 bytes) splices the list itself at 0x0045c09c..0x0045c0d4, and the
	// only other functions in that image that touch the list globals
	// (0x004b40d8 / 0x004b40dc) are the allocator, ServerMessage::_v0c and
	// two thirteen-byte accessors. server.dll's /O1 build calls it out of
	// line from inside msg_AllocPacket (0x1002603c) as well as from two
	// server-only TUs, so /Os declines the expansion at 45 bytes with the
	// true source and this object still emits the standalone COMDAT.
	void Remove(LTLink* pLink)
	{
		pLink->m_pPrev->m_pNext = pLink->m_pNext;
		pLink->m_pNext->m_pPrev = pLink->m_pPrev;

		if (pLink == m_pHead)
			m_pHead = m_pHead->m_pNext;

		m_nCount--;

		if (m_nCount == 0)
			m_pHead = LTNULL;
	}
};

extern CNetBankList g_NetBankList;         // 0x10058790 (netmgr.cpp)
extern uint32       g_nUnknown79c;         // 0x1005879c (netmgr.cpp)

// 0x10012ec5 — turn a position into the seven bytes 0x10012f49 sends: three
// words, each the fraction of the way across the reference box on that axis
// scaled by a constant, plus a byte at +6 that ORs the three high halves
// together. Nothing reads that byte (see lmsg_WriteCompPos) (name ours).
void lmsg_CompressPos(uint8* pOut, LTVector* pVec, CPosRef* pRef);

// 0x10012f49 — the three words out of that buffer, through WriteWord.
void lmsg_WriteCompPosWords(class IMessageWrite* pMsg, uint8* pData);

// 0x10015bcd — a rotation as a 4x4 (objtypes.h names the same address
// quat_ToMatrix2, and sharedcommon.cpp has the note: it is not
// servermath.h's quat_ToMatrix at 0x100117e2 but a second one).
void quat_ToMatrix2(LTRotation* pRot, LTMatrix* pMat);

// 0x100131c1 — the compressed rotation's bytes back into two vectors. Both
// out-parameters are filled; the *second* one is the primary decode (it is
// the one the function writes first and reads back), and the writer below
// only looks at the first. Cdecl, three arguments (name ours, from its
// shape).
//
// `inline` because lithtech.exe's /O2 build EXPANDS it into both its callers
// and keeps no standalone copy: 0x00435ad0 (sub_1001307f) runs the whole
// decode with no call at all, and so does 0x00435d90 (lmsg_ReadCompRotation,
// which is sub_10013382 expanded in turn). server.dll's /O1 build calls it at
// both sites -- 0x10013119 inside sub_1001307f and 0x100133b1 inside
// sub_10013382 -- so /Os declines the expansion at 198 bytes with the true
// source and this object still emits the standalone COMDAT there.
inline void sub_100131c1(uint8* pData, LTVector* pVecA, LTVector* pVecB);

// 0x1001307f — turn a rotation into the six bytes 0x10013287 sends.
void sub_1001307f(LTRotation* pRot, uint8* pOut);

// 0x10013287 — the compressed rotation's bytes, through WriteByte: three
// always, and three more only when the first byte is non-negative. Read as
// a *signed* byte (cmpb $0 / jl), so the top bit of byte 0 is the short-form
// flag (name ours).
void lmsg_WriteCompRotationBytes(class IMessageWrite* pMsg, uint8* pData);

// 0x10015ce2 — the matrix-to-quaternion arithmetic, with the rotation as
// its first argument (servermath.h declares the same address, and its note
// about the argument order is why it is named there too).
void sub_10015ce2(LTRotation* pRot, LTMatrix* pMat);

// 0x10013382 — the other end of that: six (or three) bytes back into a
// rotation.
//
// `inline` because lithtech.exe's /O2 build expands it into its ONE caller
// and keeps no standalone copy: lmsg_ReadCompRotation is 0x00435d90, 980
// bytes, three virtual ReadBytes and then this whole decode (with
// sub_100131c1 expanded inside it in the short-form arm). server.dll's /O1
// build calls it out of line at 0x10013378 -- the only call site there -- so
// /Os declines the expansion at 391 bytes and the standalone COMDAT survives.
inline void sub_10013382(uint8* pData, LTRotation* pRot);


// 0x1002417f. A fresh queue entry. The base clears the refcount and stores
// its own vtable; the write object at +0x08 stores its vtable from
// member-construction position; the packet data at +0x50 is constructed out
// of line (0x1000af9a); and the body is the two stores below and the Init.
//
// Init sets the flags word and the word at 0x3a itself, so the two stores
// here are redundant — the shipped code writes both twice. Kept because they
// are in the bytes: the stores sit ahead of the call, and one of them ahead
// of the derived vtable store, which is where a body statement lands.
//
// VC6 hands the object back in eax (`mov %esi,%eax`) even though nothing
// wants it, which is what makes this 62 bytes rather than 60.
//
// HOISTED above msg_AllocPacket and `inline` because lithtech.exe's /O2
// build expands the whole constructor into it -- 0x0045c0ea..0x0045c134 is
// the base's vtable store and refcount, the write object's vtable, the
// packet data's five-word construction, the derived vtable, the two body
// stores and then Init(0x1000,0x1000) still as a CALL -- and keeps no
// standalone copy. VC6's inliner is forward-only, so the hoist is as
// load-bearing as the keyword. server.dll's /O1 build calls it out of line
// from inside msg_AllocPacket (0x10026051) as well as from netmgr's record
// constructor (0x10024176), so /Os declines the expansion at 62 bytes with
// the true source and this object still emits the standalone COMDAT.
// (The header declares it without `inline`; only this TU constructs one, and
// the engine's objects are spliced rather than linked, so the mismatch has
// no effect beyond this object's own codegen.)
inline ServerMessage::ServerMessage()
{
	m_MsgFlags  = 0;
	m_Unknown3a = 1;

	// AVP2: 0x44c, the packet size (`mov $0x44c,%eax; push; push`: Primal
	// Hunt server.dll 0x1002de2e, AvP2 1.0.9.6's 0x1002c124).
	Init(0x44c, 0x44c);
}


// 0x10026020. Take a spare entry off the free list if there is one, else
// make a new one (0x64 bytes) and count it. The entry is Init'd, handed back
// through the out-parameter and AddRef'd, and the function returns the
// ADDRESS of the out-parameter — which is what CMessageRef::Init (0x1000ebab)
// wants, because that one dereferences its argument.
//
// Three separate `if (pMsg)` tests, not one: the count bump is inside the
// allocating arm (the reuse arm jumps past it), and the Init and the AddRef
// each guard on their own. The allocation itself is a plain `new`: the null
// arm materialises zero into the same register the constructor's return
// goes to, which is what VC6 emits for `new` on a class with a constructor.
#ifdef LT_SERVER_BUILD
void* msg_AllocPacket(CAbstractIO** ppIO, uint32 blockSize, uint32 growSize)
{
	ServerMessage* pMsg;

	if (g_NetBankList.m_nCount > 0)
	{
		pMsg = (ServerMessage*)g_NetBankList.m_pHead->m_pData;

		g_NetBankList.Remove(&pMsg->m_BankLink);
	}
	else
	{
		pMsg = new ServerMessage;

		if (pMsg)
			g_nUnknown79c++;
	}

	if (pMsg)
		pMsg->Init(blockSize, growSize);

	*ppIO = (CAbstractIO*)pMsg;

	if (pMsg)
		pMsg->AddRef();

	return ppIO;
}
#else
// lithtech.exe 0x0045c080 (336 bytes) -- the /O2 copy, IDENTICAL. Three
// facts from the shipped bytes, each a STAND-IN view below (names ours):
//  * it returns clientshell.cpp's CMsgRef holder BY VALUE (hidden pointer at
//    0x14(%esp), `mov %edi,%eax`); the void*/ppIO spelling is 357 bytes.
//  * the constructor is expanded (CAbstractIO vptr+refcount, LMessageImpl
//    vptr, the packet array's vptr 0x004a1ab8 and Clear(), derived vptr,
//    +0x3a/+0x40) but the array's Init(0, 0) (0x004283b0) and the message's
//    Init(0x1000, 0x1000) (0x00459cc0, CServerMessageInitQ12::Init below)
//    stay CALLS, while msg_AllocPacket's own Init is expanded with the real
//    CMoArray<uint8> Term()/Init() down to SetSize2 (0x0040bfe0) calls. With
//    collection.h's template at +0x50 and an inline Init /O2 expands every
//    ctor-level call too (448 bytes); the ctor-side Init out of line alone
//    gives 383. Hence the out-of-line Init plus the InitInline twin, and the
//    array as a view whose Init is out of line.
//  * the array view's Clear() must be the in-class helper (collection.h's
//    shape): four direct stores schedule the two `push %ebx` of Init(0, 0)
//    one store early (size-exact, 2 blocks).
class CPacketDataQG9
{
public:
	CPacketDataQG9()
	{
		Clear();
		Init();
	}

	virtual void _v00();

	LTBOOL Init(uint32 size = 0, uint32 cacheSize = CACHE_DEFAULT_SIZE); // 0x004283b0

	void Clear()
	{
		m_pArray = 0;
		m_nElements = 0;
		m_Cache.SetCacheSize(0);
		m_Cache.SetWantedCache(0);
	}

	uint8*       m_pArray;                                   // +0x04
	uint32       m_nElements;                                // +0x08
	DefaultCache m_Cache;                                    // +0x0c
};

class ServerMessageQG9 : public CAbstractIO
{
public:
	ServerMessageQG9()
	{
		m_Unknown3a = 1;
		m_MsgFlags  = 0;

		// AVP2: 0x44c (Primal Hunt lithtech.exe 0x0046f0ef).
		Init(0x44c, 0x44c);
	}

	// AddRef and Release (slots 1 and 2, 0x00455c00 / 0x00455c10) are
	// CAbstractIO's own, inherited, as in the shipped table 0x004a2b58.
	virtual void _v00();
	virtual void _v0c();

	// The constructor's call: the out-of-line copy, 0x00459cc0
	// (CServerMessageInitQ12::Init below).
	void Init(uint16 nMaxDataLen, uint16 nGrowSize);

	// msg_AllocPacket's own call site, which /O2 expands.
	void InitInline(uint16 nMaxDataLen, uint16 nGrowSize)
	{
		m_DataLen    = 1;
		m_MaxDataLen = nMaxDataLen;
		m_Pos        = 1;
		m_Unknown3a  = 1;
		m_MsgFlags   = 0;

		m_Write.m_pOwner = (ServerMessage*)this;

		if (nMaxDataLen > ((CMoArray<uint8>*)&m_Data)->GetSize())
		{
			((CMoArray<uint8>*)&m_Data)->Term();
			((CMoArray<uint8>*)&m_Data)->Init(nMaxDataLen, nGrowSize);
		}
		else
		{
			((CMoArray<uint8>*)&m_Data)->SetCacheSize(nGrowSize);
		}
	}

	LMessageImpl    m_Write;                                // 0x08
	uint16          m_DataLen;                              // 0x34
	uint16          m_MaxDataLen;                           // 0x36
	uint16          m_Pos;                                  // 0x38
	uint16          m_Unknown3a;                            // 0x3a
	void*           m_pConnection;                          // 0x3c
	uint32          m_MsgFlags;                             // 0x40
	LTLink          m_BankLink;                             // 0x44
	CPacketDataQG9  m_Data;                                 // 0x50 .. 0x63
};

// The four-byte holder the /O2 copy returns BY VALUE through the hidden
// pointer (0x0045c1b8 `mov 0x14(%esp),%edi ; mov %esi,(%edi)`, then the
// AddRef and `mov %edi,%eax`): clientshell.cpp's CMsgRef. (names ours)
class CMsgRefQG9
{
public:
	CMsgRefQG9(ServerMessageQG9* pMsg)
	{
		m_pMsg = pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();
	}

	~CMsgRefQG9()
	{
		if (m_pMsg)
			m_pMsg->Release();
	}

	ServerMessageQG9* m_pMsg;                               // 0x00
};

CMsgRefQG9 msg_AllocPacket(uint32 blockSize, uint32 growSize)
{
	ServerMessageQG9* pMsg;

	if (g_NetBankList.m_nCount > 0)
	{
		pMsg = (ServerMessageQG9*)g_NetBankList.m_pHead->m_pData;

		g_NetBankList.Remove(&pMsg->m_BankLink);
	}
	else
	{
		pMsg = new ServerMessageQG9;

		if (pMsg)
			g_nUnknown79c++;
	}

	if (pMsg)
		pMsg->InitInline(blockSize, growSize);

	return CMsgRefQG9(pMsg);
}
#endif


// CNetBankList::Remove (0x10026234) is now defined in-class above and
// ServerMessage::ServerMessage (0x1002417f) above msg_AllocPacket: the /O2
// build expands both into it and keeps no standalone copy of either. Both
// write-ups moved with the code.


// 0x1002608f. Slot 3 of the derived vtable, overriding the base's
// self-delete (0x100241cc) with a return to the bank: instead of freeing the
// object the last Release files it back on the free list msg_AllocPacket
// draws from, on the link the object carries at +0x44. Sixteen bytes, and
// there is no null test — the caller reached here through this very object's
// vtable.
//
// /O2: lithtech.exe's copy is 0x0045c1d0 (slot 3 of the 0x004a2b58 vtable),
// 84 bytes, with CNetBankList::Add EXPANDED -- the only calls left in it are
// none at all. Closing it therefore needs Add's BODY written here, in-class,
// and Add is still unreconstructed (server.dll 0x100261eb, 73 bytes, `todo`;
// tcpdriver.cpp names the same address for its query table). Both ends are
// on disk for whoever takes it: LTLink is m_pPrev/+0, m_pNext/+4, m_pData/+8,
// so the shipped bytes read
//
//     if (m_pHead) {
//         m_pHead->m_pPrev->m_pNext = pLink;          // [[edx]+4] = eax
//         pLink->m_pPrev = m_pHead->m_pPrev;          // m_pHead reloaded
//         pLink->m_pNext = m_pHead;
//         m_pHead->m_pPrev = pLink;
//         pLink->m_pData  = pData;
//         m_nCount++;
//         if (pLink->m_pNext == m_pHead) m_pHead = pLink;   // m_pHead reloaded
//     } else {
//         pLink->TieOff();                            // m_pNext then m_pPrev
//         pLink->m_pData = pData;
//         m_nCount++;
//         m_pHead = pLink;
//     }
//
// which is an AddBefore-shaped splice but matches neither ltlink.h's
// AddBefore nor dl_AddAfter in store ORDER, so it wants a build to settle and
// is not guessed at here. Marking Add in-class is /O1-safe on the same census
// argument as Remove: server.dll's _v0c calls it out of line at 0x10026099.
void ServerMessage::_v0c()
{
	g_NetBankList.Add(this, &m_BankLink);
}


// 0x100242c1. The net bank's record in its message shape: three words of
// link, and then a whole ServerMessage (0x0c + 0x64 = 0x70, which is the
// bank's element size exactly). Term gives the message's payload storage
// back; CNetBank::Free (0x10024226) calls it and then pushes the record onto
// the bank's own free list.
//
// The bytes are an explicit destructor call on that embedded message. The
// packet data's destructor inlines to its vptr store plus SetSize(0), and
// CAbstractIO's to the vptr store alone — the same pair ~ServerMessage
// (0x100241f4) emits, at the same offsets plus 0x0c. ServerMessage's own
// vptr store is absent from both, which is VC6 leaving out the vptr of a
// destructor it generated itself.
//
// Modelled with local views rather than by giving servermsg.h's ServerMessage
// a destructor: six other translation units include that header, and none of
// them needs one.
// The two vtables the teardown restores: the payload collection's
// (0x1004d4b0) and CAbstractIO's (0x1004dea0). CAbstractIO's is stored as a
// plain address rather than through virtuals of our own, because a vtable
// here would drag in a deleting destructor per view that the shipped image
// has no counterpart for. Relocations are masked, so the store is the same
// bytes either way.
//
// The payload collection needs no view: it IS collection.h's
// CMoArray<uint8, DefaultCache> (msgterm.cpp sets out the proof; packet.h's
// CPacket holds the same array at the same +0x50), and the template's own
// `~CMoArray() { Term(); }` is exactly that half of the teardown -- its
// vftable store (0x1004d4b0) and SetSize2(0, &g_DefAlloc) (0x1000be59).
extern void* g_AbstractIOVtbl;               // 0x1004dea0

class CTermAbstractIO
{
public:
	~CTermAbstractIO() { m_pVtbl = &g_AbstractIOVtbl; }

	void*  m_pVtbl;                          // +0x00
	uint32 m_nRefCount;                      // +0x04
};

// Member destruction runs in reverse declaration order, which is the order
// the shipped bytes have: the packet data's vptr store and SetSize first,
// then the base's vptr store.
class CTermMessage
{
public:
	~CTermMessage() {}

	CTermAbstractIO m_Base;                  // +0x00
	uint8           m_pad_08[0x50 - 0x08];
	CMoArray<uint8, DefaultCache> m_PacketData; // +0x50 .. +0x63
};

class CNetMsgRecord
{
public:
	~CNetMsgRecord();                        // 0x100242c1

	uint32       m_Unknown00;                // +0x00  the bank's free-list next
	uint32       m_Unknown04;                // +0x04
	uint32       m_Unknown08;                // +0x08
	CTermMessage m_Msg;                      // +0x0c
};

// /O2: lithtech.exe has NO standalone copy of this teardown. CNetBank::Free
// is 0x00459dc0, 49 bytes, and carries it inline -- `lea 0x5c(%esi),%ecx`,
// the packet data's vtable (0x004a1ab8), SetSize(0,&bank) at 0x0040bfe0, the
// CAbstractIO vtable (0x004a2b44) at +0xc, and then the record pushed onto
// the bank's own free list at [edi+0x1c]. So this row can never close from
// here whatever we do to the stand-in: the shipped destructor is generated
// inside netmgr.cpp's Free, and closing 0x00459dc0 is that file's edit (and
// needs ServerMessage to have a real destructor in servermsg.h, which the
// note above deliberately declines for six other TUs' sake). The 33-byte
// COMDAT this driver emits is still what matches server.dll at 0x100242c1,
// so leave it exactly as it is.
CNetMsgRecord::~CNetMsgRecord()
{
}

// 0x100241f4 — the same teardown in its scalar deleting form, which is slot
// zero of ServerMessage's own vtable (0x1004de00). VC6 emits it only for an
// explicit destructor call, so this driver is what asks for it; like
// packet.cpp's instantiation forcer it exists in no shipped binary. If
// ServerMessage ever gains a real destructor in servermsg.h, that is where
// this address belongs and this can go.
void msg_TermMessage(CTermMessage* pMsg)
{
	pMsg->~CTermMessage();
}


// 0x10010da9 — ServerMessage::Init lives in servermsg.h now, `inline` and
// above its callers, because lithtech.exe's /O2 build expands it into
// CNetMgrSendView::SendPacket (0x00457ab0) and three more. server.dll's /O1
// build still calls 0x10010da9 out of line and this object still emits the
// standalone COMDAT for it — /Os declines the body. The near write-up moved
// to the header with the code.


// 0x1002609f — read a string out of the payload WITHOUT copying it: the
// answer is a pointer into the packet's own bytes at the read cursor, and
// the cursor is then walked past the string and its terminator. The address
// computed in the prologue is not a dead `lea` — the caller at 0x1002a103
// pushes the returned pointer straight into its next call, so it is the
// return value, held in %eax across the whole walk.
//
// Three stops, and the shipped order is the source's: the declared maximum
// (a sixteen-bit compare, both operands uint16), the allocation (the cursor
// zero-extended and compared against the full dword — NOT the sixteen-bit
// narrowing ReadData below uses, and the difference is visible: `movzwl
// %dx,%esi` + `cmp 0x58(%ecx),%esi` against ReadData's `cmp` on the word),
// and the NUL. Every step writes the cursor back, so the walk
// leaves the cursor correct even where it stops early, and the final
// unconditional bump is what steps past the terminator.
//
// servermsg.h's ServerMessage does not declare this method and that header
// is not this reconstruction's to change, so it is written against a view
// of the same layout, exactly as ILTServerSendView is. WHAT SHOULD REPLACE
// THIS: `char* ReadStringInPlace();` on ServerMessage beside ReadData, and
// the view can go.
//
// Written on packet.h's CPacket (the same layout: m_MaxDataLen 0x36, m_Pos
// 0x38, the CMoArray<uint8> payload at 0x50 with m_pArray 0x54 and
// m_nElements 0x58) through CMoArray's own accessors -- `m_Data[i]` and
// `m_Data.GetSize()` -- exactly as lithtech.exe's copy (cchandler.cpp's
// CPacketQ478::ReadStringQ478, 0x0045c280) is written. IDENTICAL (58 bytes).
// The flat view this used to be written on (raw uint8* m_pData / uint32
// m_DataSize members) was a NEAR at 60 bytes: VC6 rotated the loop and
// hoisted m_pData / m_DataSize into a preheader; through the accessors the
// loop stays unrotated with m_MaxDataLen hoisted, as shipped. Going through
// a `CPacket*` cast of `this` instead of deriving gave 58/58 with only the
// SIB order of the NUL test reversed. The class is a view deriving from
// CPacket because packet.h is not this reconstruction's to change.
class CMsgStringReader : public CPacket
{
public:
	char* ReadStringInPlace();               // 0x1002609f
};

char* CMsgStringReader::ReadStringInPlace()
{
	char* pStart = (char*)&m_Data[m_Pos];
	while (m_Pos < m_MaxDataLen)
	{
		uint32 nPos = m_Pos;
		if (nPos >= m_Data.GetSize())
			break;
		if (m_Data[nPos] == 0)
			break;
		m_Pos++;
	}
	m_Pos++;
	return pStart;
}


// 0x100260d9. Copy bytes out of the payload at the read cursor, clipped
// twice: once against m_MaxDataLen and once against the allocation. Both
// clips are the same three lines with a different limit, and the second one
// compares against the allocation *narrowed to sixteen bits* — the compare
// is `movzwl` of the low half and a signed `jl`, while the subtraction that
// follows works on the full dword. That narrowing is in the shipped bytes,
// not a spelling of ours; a uint32 comparison would be unsigned (`jb`),
// which is exactly what CPacket::WriteType's own bound check emits.
//
// nPos is an int, not a uint16: the sums are compared with `jl`, and the
// value is stored to (and reloaded from) the now-dead nBytes parameter slot
// as a full dword.
// 0x1002a1da — pop the packet's last byte.
//
// Reads from the END and shortens m_DataLen, where every other reader walks
// m_Pos forward -- so a packet is written and read from both ends. The
// emptiness test is `m_DataLen - m_Pos`, i.e. "is there an unread byte
// anywhere", so the two cursors do meet in the middle.
//
// Two details are forced. The compare is UNSIGNED (`jb`), and two uint16
// fields subtract as ints, so the cast to uint32 is load-bearing -- without it
// the compare is signed and the byte differs. And the test has to be written
// POSITIVELY, with the zero return last, or VC6 hosts that return ahead of the
// read and inverts the branch: the shared-exit rule again.
//
// /O2: lithtech.exe has NO standalone copy of this. Its one caller expands it
// -- sm_GiveMessageToShell is 0x00469390 and carries the whole body inline at
// 0x004693b6..0x004693e7 -- and no other function in that image loads 0x34,
// tests `cmp $1`, and indexes m_pData at -1. So the shipped source defines it
// IN-CLASS in servermsg.h, and the payoff is smgr.cpp's function, not this
// symbol, which simply stops existing. It cannot be fixed from this file:
// marking the out-of-line definition here `inline` would hide it from smgr.cpp
// (which is the only caller) and drop the COMDAT entirely, costing server.dll
// its 43-byte match at 0x1002a1da for nothing. Moving the body into
// servermsg.h is the edit; server.dll keeps the address either way, because
// smgr.obj would then emit the COMDAT that servermsg.obj emits today.
uint8 ServerMessage::ReadByteFromEnd(uint8 nDummy)
{
	if ((uint32)(m_Unknown34 - m_Unknown38) >= 1)
	{
		uint8 nRet = m_pData[m_Unknown34 - 1];

		m_Unknown34--;

		return nRet;
	}

	return 0;
}


void ServerMessage::ReadData(void* pData, uint32 nBytes)
{
	uint16 nCount = (uint16)nBytes;
	uint16 nPos   = m_Unknown38;
	uint16 nLimit = m_MaxDataLen;

	if (nPos + nCount >= nLimit)
		nCount = nLimit - nPos;

	{
		uint32 nAlloc = m_DataSize;

#ifdef AVP2_MODERN
		// fixed: ServerMessage::ReadData and WriteData bound-check a
		// truncated size (RETAIL_BUGS.md). The allocation is compared at its
		// full 32 bits, as CPacket::WriteType does.
		if (nPos + nCount >= nAlloc)
#else
		if (nPos + nCount >= (uint16)nAlloc)
#endif
			nCount = (uint16)(nAlloc - nPos);
	}

	memcpy(pData, &m_pData[nPos], nCount);

	m_Unknown38 += nCount;
}


// 0x1002616a. The other direction, with one clip instead of two and a grow
// loop instead of a second clip: the payload is extended a byte at a time
// (the payload array's Insert2 at its own end -- packet.h's CMoArray<uint8>,
// reached through a cast because ServerMessage keeps the array's fields
// under its own names) until the write fits. The loop's first test uses the
// cursor already in a register and every later one re-reads it, because
// the insert could have moved anything.
//
// IDENTICAL, 129 of 129 bytes. The count starts as the argument and the
// clip ASSIGNS over it, reading both members afresh (`nCount = m_MaxDataLen
// - m_Unknown38`, not the two locals): that is what makes VC6 keep the
// argument's own copy in its dead parameter slot (`mov %edi,0xc(%ebp)`) and
// give the un-clipped path a reload from there (`jmp` + `mov 0xc(%ebp),%ebx`),
// the eight bytes every if/else, `?:` and inner-scope spelling came out
// short by (121). The subtraction over the raw 16-bit loads (`sub %edx,%eax`)
// falls out of the same spelling. Measured in a scratch TU against this
// header; the parameter's width (uint16 or uint32) does not change the body.
//
// One thing already settled: the *parameter* is sixteen bits
// wide at the call site. netmgr.cpp's fragment replay computes its length
// with a 16-bit `dec %cx` and pushes the raw dword the value sits in, with
// no zero-extension -- which a uint32 parameter would have forced. It is
// declared `uint32` here because that is what LMessageImpl::WriteRaw and
// ReadRawFL hand it, and the netmgr views carry the narrow declarations
// locally.
//
// lithtech.exe 0x0045c390: NEAR (363 vs ours 366). The one residual is the
// inliner: shipped CALLS BaseNew<uint8> (0x0040c060) and BaseDelete
// (0x004152b0) inside the expanded Insert2, ours expands both. Same
// residual as clientmgr3.cpp's CCmgMsg::WriteType (0x004159f0). Neither
// Insert (one level deeper) nor an inline grow helper moved it.
void ServerMessage::WriteData(void* pData, uint32 nBytes)
{
	uint16 nPos   = m_Unknown38;
	uint16 nLimit = m_MaxDataLen;
	uint16 nCount = (uint16)nBytes;

	if (nPos + nCount >= nLimit)
#ifdef AVP2_MODERN
	{
		nCount = (uint16)(m_MaxDataLen - m_Unknown38);
		// fixed: ServerMessage::WriteData clips the count, then grows the
		// buffer anyway (RETAIL_BUGS.md). The clip is the packet-size cap
		// and stays; the lost tail is reported (to OutputDebugString, the
		// first few times), so a message that outgrows a packet shows.
		static int s_nReported = 0;
		if (s_nReported < 8)
		{
			char sMsg[128];
			s_nReported++;
			sprintf(sMsg, "ServerMessage::WriteData: %u bytes at %u of a %u-byte message; %u written\n",
				nBytes, (uint32)nPos, (uint32)nLimit, (uint32)nCount);
			OutputDebugStringA(sMsg);
		}
	}
#else
		nCount = (uint16)(m_MaxDataLen - m_Unknown38);
#endif

	// GetSize(), not the bare m_DataSize: lithtech.exe's /O2 copy reads the
	// dword and masks it (0x0045c3ce / 0x0045c4ad `mov 0x58 ; and $0xffff`)
	// where a bare member narrows to a 16-bit load.
#ifdef AVP2_MODERN
	while (m_Unknown38 + nCount >= ((CMoArray<uint8>*)&m_PacketData)->GetSize())
#else
	while (m_Unknown38 + nCount >= (uint16)((CMoArray<uint8>*)&m_PacketData)->GetSize())
#endif
	{
		uint8            dummy  = 0;
		CMoArray<uint8>* pPData = (CMoArray<uint8>*)&m_PacketData;

		pPData->Insert2(pPData->GetSize(), dummy, &g_DefAlloc);
	}

	memcpy(&m_pData[m_Unknown38], pData, nCount);

	m_Unknown38 += nCount;
	m_Unknown34 += nCount;
}


// 0x10012ec5. Pack a position into the seven bytes the writer sends: each
// axis becomes the fraction of the way across the reference box, scaled by
// 65536, truncated to an integer and sent as its low sixteen bits; the
// seventh byte ORs the three *high* halves together and nothing ever reads
// it (lmsg_ReadCompPos rebuilds the position from the three words alone).
//
// The 65536.0f is loaded once, before anything else, and kept on the FPU
// stack across all three conversions — one `.rdata` load for three uses,
// with the `fstp %st(0)` at the end dropping it. The three `(int)` casts are
// `_ftol` calls, and the shifts are unsigned, so the three results are
// uint32 locals and not ints.
void lmsg_CompressPos(uint8* pOut, LTVector* pVec, CPosRef* pRef)
{
	uint32 nX = (uint32)(int)((pVec->x - pRef->m_vMin.x) * pRef->m_vScale.x * 65536.0f);
	uint32 nY = (uint32)(int)((pVec->y - pRef->m_vMin.y) * pRef->m_vScale.y * 65536.0f);
	uint32 nZ = (uint32)(int)((pVec->z - pRef->m_vMin.z) * pRef->m_vScale.z * 65536.0f);

	*(uint16*)&pOut[0] = (uint16)nX;
	*(uint16*)&pOut[2] = (uint16)nY;
	*(uint16*)&pOut[4] = (uint16)nZ;

	pOut[6] = (uint8)((nZ >> 16) | (nY >> 16) | (nX >> 16));
}


// 0x100131c1. The three-byte short form back into a forward vector and the
// up vector that goes with it.
//
// The forward vector is the same decode the long form (sub_10013382's else
// arm) uses — 1/63 less one on the two bytes the writer produced as an
// unsigned 0..126 over -1..1, 1/127 on the third — read
// out of bytes 1, 0 and 2 in that order, which is the order the shipped
// `movsbl`s take them in.
//
// The up vector is not in the message at all. It starts as the forward
// vector displaced fifty units along +y and is then made perpendicular to it
// by subtracting its projection: one Gram-Schmidt step. The fifty is
// arbitrary — any displacement not parallel to the forward vector leaves the
// same direction once the projection is gone — and nothing here normalises,
// because the caller does.
//
// The FPU shape follows from the header: `MagSqr` is a call (0x1000552d)
// and `operator*` / `operator+=` are inline. 1/127 and 2.0 are each loaded
// once and kept on the stack across all three conversions; the third
// conversion drops the 2.0 and multiplies the integer straight into 1/127
// with `fimull`.
//
// Three shapes in here are load-bearing and each cost a probe:
//
//   * the derived vector is built with `Init(x, y, z)`, not three
//     assignments. Init takes its three floats by value and is inlined, so
//     VC6 evaluates the arguments right to left onto the FPU stack and then
//     stores them left to right — push z, push y, push x, store x, store y,
//     store z. That is why the forward z leaves the FPU with a non-popping
//     `fsts`: it is still st(0) when the up vector wants it. Three separate
//     assignments instead give a `flds`/`fstps` pair for x, a computed y,
//     and an INTEGER `mov` pair for the z copy — seven bytes over.
//
//   * the projection's dot product is three separate accumulating
//     statements, not one three-term sum. VC6 REASSOCIATES a
//     `a*b + c*d + e*f` written as one expression into a fixed order of
//     its own (it emitted z, y, x here whether the source said x,y,z or
//     y,z,x or x + (y + z) — source term order does not reach it at all),
//     and that order is not the shipped one. Written as `f = ...;
//     f += ...; f += ...;` the order is the source's again, and the
//     accumulator stays on the FPU stack, so it costs no frame slot. This
//     is a general fingerprint, not a local trick.
//
//   * the copy of the up vector into the frame's only local is the
//     `Dot`-by-value copy the SDK header implies, kept here as an explicit
//     local because the accumulating form needs to name its components.
//     That is the server.dll spelling only: lithtech.exe wants the SDK's
//     Dot itself (see the LT_SERVER_BUILD split in the body).
inline void sub_100131c1(uint8* pData, LTVector* pUp, LTVector* pForward)
{
	LTVector v;
	float    fScale;

	// AVP2: the two unsigned bytes are 1/63 less one (0x004d556c, 1.0 at
	// 0x004d4ab0, lithtech.exe 0x00441c70/0x00441c82), NOLF's 1/127 less a
	// half, doubled, is gone; AvP2 1.0.9.6 agrees.
	pForward->x = (char)pData[1] * (1.0f / 63.0f) - 1.0f;
	pForward->y = (char)pData[0] * (1.0f / 63.0f) - 1.0f;
	pForward->z = (char)pData[2] * (1.0f / 127.0f);

	pUp->Init(pForward->x, pForward->y + 50.0f, pForward->z);

#ifdef LT_SERVER_BUILD
	// server.dll /O1: the three accumulating statements over the by-value
	// copy (the `movsl` x3 at 0x10013232) give the shipped x, z, y order;
	// the SDK's Dot expanded here comes out z, y, x (0x10013235..0x10013248).
	v = *pUp;

	fScale  = v.x * pForward->x;
	fScale += v.z * pForward->z;
	fScale += v.y * pForward->y;
#else
	// lithtech.exe /O2: the SDK's by-value Dot.  lmsg_ReadCompRotation
	// (0x00435d90) CALLS it -- `call 0x436790`, ltvector.h's Dot, with the
	// twelve-byte argument built at 0x00436033..0x00436050 -- and
	// sub_1001307f (0x00435ad0) expands it with its terms in the shipped
	// z, x, y order; the accumulating form above gives x, z, y there.
	// A genuine per-image difference in term order (VC6 reassociates), so
	// the spelling is split.
	fScale = pForward->Dot(*pUp);
#endif

	fScale = -(fScale / pForward->MagSqr());

	*pUp += *pForward * fScale;
}


// 0x1001307f. Pack a rotation into the six bytes the writer sends. The
// rotation becomes a 4x4 and only two of its columns are used: column 2
// (forward) for the first three bytes and column 1 (up) for the last three.
//
// The two encodings differ, which is in the bytes and not a slip: bytes 0
// and 1 are `(v + 1.0f) * 63.0f`, rounded — an unsigned 0..126 over the
// range -1..1 — while bytes 2 to 5 are `v * 127.0f`, rounded, a signed
// -127..127. (NOLF truncated `* 63.9f` / `* 127.9f`; AvP2 rounds.)
//
// The second half is a round-trip check. The six bytes are decoded again and
// the vector that comes back is normalised and dotted against the real up
// vector; if the two are (anti)parallel to within a thousandth the encoding
// is degenerate and the first one or two bytes are negated. `-0.999f` takes
// both bytes and `+0.999f` only the second, and the shared negation of
// byte 0 is one block the first arm jumps into.
//
// Two details worth keeping. The normalised z is never stored back into the
// decoded vector — it goes straight into the dot product from the FPU stack,
// where x and y are stored because they are read again — so the source
// multiplies it inline rather than assigning it. And negating a byte is
// written as an assignment whose value is a conditional expression: both
// arms leave the result in %al and there is one store, where two separate
// assignments would have stored an immediate for the zero case.
void sub_1001307f(LTRotation* pRot, uint8* pOut)
{
	LTMatrix mat;
	LTVector vOther;
	LTVector vUp;
	LTVector vCheck;
	float    fMagSqr;
	float    fInv;
	float    fDot;

	quat_ToMatrix2(pRot, &mat);

	vUp.x = mat.m[0][1];
	vUp.y = mat.m[1][1];
	vUp.z = mat.m[2][1];

	// AVP2: rounded, not truncated: 63.0f/127.0f and a `+ 0.5f` ahead of
	// every _ftol (lithtech.exe 0x00441bb9..0x00441c53, server.dll
	// 0x1001829d..0x10018318; AvP2 1.0.9.6's 0x0043de79 agrees).
	pOut[0] = (uint8)(int)((mat.m[1][2] + 1.0f) * 63.0f + 0.5f);
	pOut[1] = (uint8)(int)((mat.m[0][2] + 1.0f) * 63.0f + 0.5f);
	pOut[2] = (uint8)(int)(mat.m[2][2] * 127.0f + 0.5f);
	pOut[3] = (uint8)(int)(mat.m[0][1] * 127.0f + 0.5f);
	pOut[4] = (uint8)(int)(mat.m[1][1] * 127.0f + 0.5f);
	pOut[5] = (uint8)(int)(mat.m[2][1] * 127.0f + 0.5f);

	sub_100131c1(pOut, &vCheck, &vOther);

	fMagSqr = vCheck.x * vCheck.x + vCheck.y * vCheck.y + vCheck.z * vCheck.z;

	if (fMagSqr > 0.1f)
	{
		fInv = 1.0f / (float)sqrt(fMagSqr);

		vCheck.x *= fInv;
		vCheck.y *= fInv;
		vCheck.z *= fInv;

		fDot = vCheck.z * vUp.z + vCheck.y * vUp.y + vCheck.x * vUp.x;

		if (fDot <= 0.999f)
		{
			if (fDot >= -0.999f)
				return;

			pOut[1] = (uint8)(pOut[1] == 0 ? -1 : -(char)pOut[1]);
		}

		pOut[0] = (uint8)(pOut[0] == 0 ? -1 : -(char)pOut[0]);
	}
}


// 0x10013287. The compressed rotation on the wire: three bytes through
// WriteByte (vtable slot 0x40) always, and three more only when the first
// one is non-negative read as a *signed* char — the top bit of byte 0 is the
// short-form flag, which is why the test is `cmpb $0` / `jl` and not a mask.
void lmsg_WriteCompRotationBytes(IMessageWrite* pMsg, uint8* pData)
{
	pMsg->WriteByte(pData[0]);
	pMsg->WriteByte(pData[1]);
	pMsg->WriteByte(pData[2]);

	if ((char)pData[0] >= 0)
	{
		pMsg->WriteByte(pData[3]);
		pMsg->WriteByte(pData[4]);
		pMsg->WriteByte(pData[5]);
	}
}


// 0x10012f49. The compressed position on the wire: three words out of the
// buffer, each through the message's WriteWord (vtable slot 0x44). The
// buffer's seventh byte — the one lmsg_CompressPos ORs the three high
// halves into — is never sent.
//
// The words are 16-bit loads into cx and pushed as whole dwords
// (`mov (%edi),%cx; push %ecx`), which is the uint16-parameter shape: the
// high half of each pushed dword is whatever the register held before.
void lmsg_WriteCompPosWords(IMessageWrite* pMsg, uint8* pData)
{
	pMsg->WriteWord(((uint16*)pData)[0]);
	pMsg->WriteWord(((uint16*)pData)[1]);
	pMsg->WriteWord(((uint16*)pData)[2]);
}


// 0x10012f79. Compress the position into a seven-byte buffer, then send the
// three words of it. The seventh byte the compressor builds is never sent.
void lmsg_WriteCompPos(LMessageImpl* pMsg, LTVector* pVec, CPosRef* pRef)
{
	uint8 data[7];

	lmsg_CompressPos(data, pVec, pRef);

	lmsg_WriteCompPosWords((IMessageWrite*)pMsg, data);
}


// 0x100132da. The same two-step for a rotation, six bytes wide.
void lmsg_WriteCompRotation(LMessageImpl* pMsg, LTRotation* pRot)
{
	uint8 data[6];

	sub_1001307f(pRot, data);

	lmsg_WriteCompRotationBytes((IMessageWrite*)pMsg, data);
}


// 0x10013382. The compressed rotation's bytes back into a rotation.
//
// The first byte's sign chooses the format, exactly as
// lmsg_WriteCompRotationBytes' `cmpb $0` does at the other end: negative is
// the three-byte short form, non-negative the six-byte long form. The short
// form takes the two sign bits back out of bytes 0 and 1 — negating each
// byte *in place*, which the caller's buffer sees — and remembers whether
// the second one was set, because that one means the decoded up vector has
// to be flipped. The long form just scales the six signed bytes: 1/127 for
// all of them, with an extra `- 0.5` and doubling on the two that were
// written as an unsigned 0..127 over -1..1.
//
// Then the same orthonormalisation either way: right = fwd x up, fwd = up x
// right, both normalised, up = right x fwd, and the three axes go into a
// matrix that is turned back into a quaternion. Cross and Norm are calls,
// so each cross product costs a twelve-byte by-value copy and
// a twelve-byte copy back out of the operator's return temporary — which is
// the single -0x30 slot the frame reserves for all four of them.
inline void sub_10013382(uint8* pData, LTRotation* pRot)
{
	LTMatrix mat;
	LTVector vRight;
	LTVector vForward;
	LTVector vUp;
	LTBOOL   bFlip;

	if ((char)pData[0] < 0)
	{
		pData[0] = -pData[0];

		bFlip = LTFALSE;

		if ((char)pData[1] < 0)
		{
			pData[1] = -pData[1];

			bFlip = LTTRUE;
		}

		sub_100131c1(pData, &vUp, &vForward);

		vUp.Norm();
		vForward.Norm();

		if (bFlip)
			vUp = -vUp;
	}
	else
	{
		// AVP2: 1/63 less one, as in sub_100131c1.
		vForward.y = (char)pData[0] * (1.0f / 63.0f) - 1.0f;
		vForward.x = (char)pData[1] * (1.0f / 63.0f) - 1.0f;
		vForward.z = (char)pData[2] * (1.0f / 127.0f);

		vUp.x = (char)pData[3] * (1.0f / 127.0f);
		vUp.y = (char)pData[4] * (1.0f / 127.0f);
		vUp.z = (char)pData[5] * (1.0f / 127.0f);
	}

	vRight   = vForward.Cross(vUp);
	vForward = vUp.Cross(vRight);

	vRight.Norm();
	vForward.Norm();

	vUp = vRight.Cross(vForward);

	mat.SetBasisVectors(&vRight, &vUp, &vForward);

	sub_10015ce2(pRot, &mat);
}


// 0x100132fc. Read the compressed rotation back: three bytes always, and
// three more only when the first one is non-negative as a SIGNED byte — the
// same short-form flag the writer's 0x10013287 tests. The byte the reader
// hands the message is a local of its own that VC6 parks in the top byte of
// the dead pMsg parameter slot.
//
// lithtech.exe 0x00441e80: NEAR (1016 vs ours 970). The decode now matches;
// the one residual is the inliner: shipped expands the long-form path's
// first Norm (vRight's, 0x00442012..0x00442044) and CALLS the second
// (0x00442053), ours calls both.
void lmsg_ReadCompRotation(LMessageImpl* pMsg, LTRotation* pRot)
{
	char data[6];

	{ uint8 b; ((IMessageRead*)pMsg)->ReadByte(&b); data[0] = b; }
	{ uint8 b; ((IMessageRead*)pMsg)->ReadByte(&b); data[1] = b; }
	{ uint8 b; ((IMessageRead*)pMsg)->ReadByte(&b); data[2] = b; }

	if (data[0] >= 0)
	{
		{ uint8 b; ((IMessageRead*)pMsg)->ReadByte(&b); data[3] = b; }
		{ uint8 b; ((IMessageRead*)pMsg)->ReadByte(&b); data[4] = b; }
		{ uint8 b; ((IMessageRead*)pMsg)->ReadByte(&b); data[5] = b; }
	}

	sub_10013382((uint8*)data, pRot);
}


// A corner component read as a value. See lmsg_ReadCompPos: it is what
// stops VC6 folding the corners into the FPU arithmetic (name ours).
inline float lmsg_Get(const float* p) { return *p; }


// 0x10012f9e (lithtech.exe 0x004359c0, 261 bytes). The compressed position
// back out: three words, each the fraction (in units of 1/65536) of the way
// across the reference box on that axis. With no reference box there is
// nothing to be a fraction of, so a null pRef reads back as the zero vector
// -- three separate stores, not an Init -- and the words are not read at all.
//
// IDENTICAL on both images. Two source facts carry it:
//
//  * The three words land in a uint32 ARRAY, n[3], not three scalars. With
//    scalars VC6 copy-propagates the last `n3 = w` into its conversion,
//    knows the value is a zero-extended word and narrows `(float)(uint32)`
//    to a plain fildl (lithtech 0x00435a68 is a full fildll over the u64
//    temp; /O1's third conversion is the `fimull -0x8`). The array elements
//    are still enregistered, but the range is forgotten.
//  * Each word is read into its OWN block-scoped uint16. At /O1 that gives
//    the dead-parameter-slot targets 0x12, 0xa, 0x12 (0x10012fcf/dc/eb)
//    with the second value loaded lazily from 0xa after the third read; at
//    /O2 the three share 0x30(%esp) (0x004359ee/00/16), each captured
//    straight after its read. One shared `w` gives 0x12 three times at /O1;
//    a separate w2 at function or block scope inside the shared block gives
//    lithtech a second slot (0x28).
//
// Each component is min + (max - min) * fraction. The three fractions are
// one LTVector, not three floats: the /O1 frame is exactly 0x14 (vFrac.x
// -0x14, vFrac.y -0x10, vFrac.z never stored because the third product stays
// on the FPU stack, hence the closing bare fstp; the u64 conversion temp at
// -0x8). The corners are read through lmsg_Get: /O1's `flds max ; flds min ;
// fsubrp` and `flds min ; faddp` are what VC6 emits for the VALUE of an
// inline function, where bare member reads fold into `fsubs/fadds (%eax)`.
// /O2 folds them either way (`fsubs 0x144(%esi)`, as shipped).
void lmsg_ReadCompPos(LMessageImpl* pMsg, LTVector* pVec, CPosRef* pRef)
{
	uint32   n[3];
	LTVector vFrac;

	if (!pRef)
	{
		if (pVec)
		{
			pVec->x = 0.0f;
			pVec->y = 0.0f;
			pVec->z = 0.0f;
		}

		return;
	}

	{ uint16 w; ((IMessageRead*)pMsg)->ReadWord(&w); n[0] = w; }
	{ uint16 w; ((IMessageRead*)pMsg)->ReadWord(&w); n[1] = w; }
	{ uint16 w; ((IMessageRead*)pMsg)->ReadWord(&w); n[2] = w; }

	vFrac.x = (float)n[0] * (1.0f / 65536.0f);
	vFrac.y = (float)n[1] * (1.0f / 65536.0f);
	vFrac.z = (float)n[2] * (1.0f / 65536.0f);

	pVec->x = lmsg_Get(&pRef->m_vMin.x) + (lmsg_Get(&pRef->m_vMax.x) - lmsg_Get(&pRef->m_vMin.x)) * vFrac.x;
	pVec->y = lmsg_Get(&pRef->m_vMin.y) + (lmsg_Get(&pRef->m_vMax.y) - lmsg_Get(&pRef->m_vMin.y)) * vFrac.y;
	pVec->z = lmsg_Get(&pRef->m_vMin.z) + (lmsg_Get(&pRef->m_vMax.z) - lmsg_Get(&pRef->m_vMin.z)) * vFrac.z;
}


// The compressed-vector codec's sign: plus or minus one, from a compare
// against 0.0f. It is a value the code then compares against another
// component's, not a branch on the float itself, which is why the shipped
// bytes materialise 1 and -1 (`or $-1,%ecx` is VC6's four-byte `= -1`) and
// then `cmp` them (name ours).
#define LMSG_SIGN(val)  (((val) < 0.0f) ? -1 : 1)

// 0x10012d4e. A vector goes out as the component with the largest magnitude
// at full float precision, the other two as eighteen-bit fractions of it,
// and one flag byte:
//
//   bits 7-6  which axis kept its float: 0 = x, 1 = y, 2 = z
//   bit  5    the second component's sign differs from the first's
//   bits 4-3  the second fraction's top two bits
//   bit  2    the third component's sign differs from the first's
//   bits 1-0  the third fraction's top two bits
//
// and then WriteFloat, WriteWord, WriteWord, WriteByte in that order — so
// the low sixteen bits of each fraction ride in the words and the top two
// in the flags.
//
// Two things here are the shipped code's and not ours. fabs is a real call
// taking a double (the push/push/fstpl/call shape), so the magnitude
// comparisons run at double precision while fMax is stored as a float; and
// the scale is 262143.0 = 2^18 - 1 as a DOUBLE (fmull), while the reader
// multiplies by 2^-18 as a float. The two are not each other's inverse:
// the round trip loses a part in 262144, which is in the shipped constants.
//
// The flag byte itself lives in the dead pVec parameter slot, which is why
// the writes to it are byte-sized `andb $0`/`movb`/`orb` against 0xc(%ebp).
//
// /O2 NEAR, and allocation-side: lithtech.exe 0x00435760, 368 bytes / 129
// instructions against our 359 / 131. At /O2 fabs is an /Oi INTRINSIC on both
// sides (`d9 e1`), not the double-taking call /O1 emits, so the whole residual
// is which float local VC6 parks in the x87 stack. The shipped build keeps
// **f2** in st across both branches (`fld %st(0)`+`fabs` for the fraction and
// the same st(0) for LMSG_SIGN(f2)), and homes fMax and d in frame slots, so
// every compare is `fcomps 0x24(%esp)`, every division `fdivs 0x24(%esp)` and
// `fMax = d` a pair of integer moves between two slots; ours keeps **fMax** in
// st(2) (`fcomp %st(2)`, `fld %st(2)`/`fdivrp`), gives d no slot at all, and
// reloads f2 from memory. The mirror image, eight instructions apart, plus
// three pure register-choice blocks (%ecx/%edx, %eax/%ecx) at the tail.
// No source lever found: the roles are set by VC6's FPU-stack allocator, and
// the only spellings that would move it (assigning f2 before fMax, dropping
// the `(float)` on `fMax = d`) reorder statements inside a function that is
// byte-identical at /O1, which is the one thing not worth risking here.
// Left alone; do not re-derive. Also measured, all still 359/20 blocks
// on lithtech or costing the /O1 match: every declaration order, f1..f3 or
// n2/n3 as arrays, fMax or f2 as a one-element array, LMSG_SIGN as an inline
// function, `fMax < d`, a double d, and the compares on fabs() with no d.
void lmsg_WriteCompVector(LMessageImpl* pMsg, LTVector* pVec)
{
	float  fMax;
	float  f1, f2, f3;
	uint32 n2, n3;
	float  d;
	uint8  nFlags;

	fMax   = (float)fabs(pVec->x);
	nFlags = 0;
	f1     = pVec->x;
	f2     = pVec->y;
	f3     = pVec->z;

	d = fabs(pVec->y);

	if (d > fMax)
	{
		nFlags = 0x40;
		fMax   = (float)d;
		f1     = pVec->y;
		f2     = pVec->x;
	}

	d = fabs(pVec->z);

	if (d > fMax)
	{
		nFlags = 0x80;
		fMax   = (float)d;
		f1     = pVec->z;
		f2     = pVec->x;
		f3     = pVec->y;
	}

	n2 = (uint32)(fabs(f2) / fMax * 262143.0);

	nFlags |= ((n2 >> 16) & 3) << 3;

	if (LMSG_SIGN(f2) != LMSG_SIGN(f1))
		nFlags |= 0x20;

	n3 = (uint32)(fabs(f3) / fMax * 262143.0);

	nFlags |= (n3 >> 16) & 3;

	if (LMSG_SIGN(f3) != LMSG_SIGN(f1))
		nFlags |= 0x04;

	((IMessageWrite*)pMsg)->WriteFloat(f1);
	((IMessageWrite*)pMsg)->WriteWord(n2);
	((IMessageWrite*)pMsg)->WriteWord(n3);
	((IMessageWrite*)pMsg)->WriteByte(nFlags);
}



// *********************************************************************** //
//  0x1000deeb — the message formatter over the resource module.
//
//  servermgr.h names it dsi_SetupMessage and declares it `void`; the bytes
//  say otherwise — it returns 1 when it had to substitute a placeholder and
//  0 when it formatted something, so the declaration there is wrong and the
//  one below is the shipped shape. It is defined here rather than beside
//  that declaration because this file does not include servermgr.h.
// *********************************************************************** //

// Declared here rather than through windows.h, whose macros rename half the
// engine's own methods out from under it (dsi.cpp does the same). Both
// shipped calls go through the import table, which is what dllimport gives:
// LoadStringA is a plain `call *[__imp__]` and FormatMessageA's entry is
// loaded into a register once and called twice.
extern "C" {
__declspec(dllimport) unsigned long __stdcall FormatMessageA(unsigned long dwFlags,
    const void* lpSource, unsigned long dwMessageId, unsigned long dwLanguageId,
    char* lpBuffer, unsigned long nSize, va_list* Arguments);
__declspec(dllimport) int __stdcall LoadStringA(void* hInstance, unsigned int uID,
    char* lpBuffer, int cchBufferMax);
}

#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
#define FORMAT_MESSAGE_FROM_STRING     0x00000400
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0x00002000

// 0x10058430 — the resource module (ltmsg.dll) once it has been loaded:
// dsi.cpp's g_hMsgModule, which dsi_InitMessageModule (0x1000dff4) sets. An
// HMODULE there; spelled through the handle's tag here because this file
// keeps windows.h out.
extern struct HINSTANCE__* g_hMsgModule;

// 0x10051a38 — twenty-seven pairs mapping a message code to the string
// resource that carries its format. The count is what the shipped bound
// gives: the walk stops at 0x10051b10, which is 0xd8 bytes on, and the two
// placeholder literals begin there (names ours).
struct CMsgResEntry
{
	uint32 m_nCode;                          // +0x00
	uint32 m_nStringID;                      // +0x04
};

#define MSGRES_COUNT 27

// The shipped .data, pair for pair. The two placeholder literals and
// dsi.cpp's "ltmsg.dll" follow it directly (0x10051b10..0x10051b40).
CMsgResEntry g_MsgResTable[MSGRES_COUNT] =
{
	{ 0x31, 0x0f }, { 0x33, 0x10 }, { 0x17, 0x02 }, { 0x1b, 0x03 },
	{ 0x1c, 0x04 }, { 0x1d, 0x4e }, { 0x1e, 0x4c }, { 0x1f, 0x4d },
	{ 0x20, 0x52 }, { 0x23, 0x59 }, { 0x29, 0x0d }, { 0x2a, 0x07 },
	{ 0x2c, 0x5c }, { 0x2d, 0x0a }, { 0x2e, 0x0b }, { 0x2f, 0x08 },
	{ 0x18, 0x09 }, { 0x30, 0x05 }, { 0x34, 0x5d }, { 0x35, 0x2a },
	{ 0x37, 0x42 }, { 0x38, 0x5e }, { 0x28, 0x06 }, { 0x3a, 0x0e },
	{ 0x39, 0x0c }, { 0x4c, 0x11 }, { 0x4e, 0x5f },
};

// The whole function is a fallback chain, and the arm order is the shipped
// one: the table lookup first, then the module's own message 1 with the code
// as its single inserted argument, then a literal. Both literal paths are
// the same two statements, tail-merged onto the second.
int dsi_SetupMessage(char* pMsg, int maxLen, int messageCode, va_list marker)
{
	char   szFormat[500];
	uint32 args[4];
	uint32 i;

	pMsg[0] = 0;

	if (!g_hMsgModule)
	{
		sprintf(pMsg, "<missing resource DLL>");

		return 1;
	}

	uint32 nCode = messageCode & 0xffff;

	for (i = 0; i < MSGRES_COUNT; i++)
	{
		if (g_MsgResTable[i].m_nCode == nCode)
			goto found;
	}

fallback:
	args[0] = nCode;

	if (FormatMessageA(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_HMODULE,
	                   g_hMsgModule, 1, 0x400, pMsg, maxLen,
	                   (va_list*)args) > 0)
		return 0;

	// SETTLED: this is sprintf, and so is the "<missing resource DLL>" copy
	// above -- the two are tail-merged onto this one call, which is why
	// there is a single site to identify.
	//
	// The two CRT bodies leave no doubt. 0x1003e580 is strcpy: the classic
	// MSVC four-bytes-at-a-time loop with the 0x7efefeff / 0x81010100 pair.
	// 0x1003e670 is sprintf: it builds the fake FILE on its own frame
	// (`movl $0x42,-0x14(%ebp)` for the flags, `movl $0x7fffffff,-0x1c(%ebp)`
	// for the count), hands the format at 0xc(%ebp) and the varargs at
	// 0x10(%ebp) to _output (0x10043244), then null-terminates. The shipped
	// call at 0x1000df62 goes to 0x1003e670, and gp_SetFloatValue's
	// `sprintf(s, "%f", v)` pins the same address independently.
	//
	// An earlier version recorded that spelling this `sprintf` cost the
	// function its byte identity forty bytes earlier, with an extra
	// six-byte `mov <global>,%edi`, and left the conflict flagged as
	// unresolved. It is resolved, and the cause was neither the callee
	// identification nor the prototype: THE TWO LITERAL COPIES TAIL-MERGE
	// ONTO ONE CALL, and that version changed only this one, leaving the
	// "<missing resource DLL>" copy above spelled `strcpy`. Two different
	// callees cannot share a merged tail, so the merge broke and the second
	// site grew its own setup -- which is where the six bytes came from,
	// forty bytes away from the edit. Spell both `sprintf` and all 197
	// bytes are exact.
	//
	// The general form is worth keeping: when two call sites tail-merge in
	// the shipped code, they are one call in the source's eyes, and a
	// half-applied change to either reads as evidence about the callee when
	// it is really evidence about the merge.
	//
	// A two-argument sprintf and a strcpy compile to the same call shape
	// (cdecl, two pushes, two `pop %ecx`), so the masked oracle cannot tell
	// them apart -- a call-target check is the only thing that can, and it now
	// reports no merge for either.
	sprintf(pMsg, "<invalid resource DLL>");

	return 1;

	// Out of line, after the returns, because that is where the shipped
	// build has it: the search falls straight through into the fallback
	// above and this block is only ever reached by the forward jump, both
	// of whose failures come back the other way. Written inside the loop it
	// lands between the two and costs the loop an extra jump.
found:
	{
		// Read before the call, which is what lets the shipped build spend
		// the index register on the buffer's address.
		uint32 nStringID = g_MsgResTable[i].m_nStringID;

		// Unsigned, which the shipped `jbe` says: LoadStringA's own int
		// return would have compared with `jle`.
		uint32 nLen = LoadStringA(g_hMsgModule, nStringID, szFormat, 499);

		if (nLen > 0)
		{
			if (FormatMessageA(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0,
			                   pMsg, maxLen, &marker) > 0)
				return 0;
		}
	}

	goto fallback;
}


#ifndef LT_SERVER_BUILD
// lithtech.exe 0x00459cc0, 246 bytes -- ServerMessage::Init's OUT-OF-LINE
// copy. lithtech.exe's /O2 build expands the servermsg.h body at most call
// sites, but CNetBank's slot 1 (0x00459c30, startload.cpp) and the
// CSoundMgr constructor (0x0047f711) call it, and the shipped COMDAT expands
// the packet array's two resizes: SetSize2(0, &g_DefAlloc) in full, then
// CMoArray<uint8>::Init with its Term() left as a call (0x0040bfe0,
// `push $0x4b4e88 ; push 0`), _InitArray's four stores and SetSize2(n)
// expanded around BaseDelete (0x004152b0) and BaseNew (0x0040c060).
//
// servermsg.h's ServerMessage carries the array as a vptr-only stand-in
// (CMsgPacketData, whose SetSize/Init are declared out of line for
// server.dll's /O1 calls), so neither the header's body nor any lithtech TU
// emits this expansion. This is a STAND-IN: the same layout with the real
// collection.h CMoArray<uint8> at +0x50 (packet.h's CPacket view, whose
// m_Data it is), so /O2 spends its inline budget on the real template.
// Store order is lithtech's (0x00459cce..0x00459ce1): the three ones and the
// limit, the flags, then the owner. (names ours)
class CServerMessageInitQ12
{
public:
	void Init(uint16 nMaxDataLen, uint16 nGrowSize);        // 0x00459cc0

	uint8           m_pad_00[0x10];
	void*           m_pOwner;                               // 0x10  m_Write.m_pOwner
	uint8           m_pad_14[0x34 - 0x14];
	uint16          m_DataLen;                              // 0x34
	uint16          m_MaxDataLen;                           // 0x36
	uint16          m_Pos;                                  // 0x38
	uint16          m_Unknown3a;                            // 0x3a
	void*           m_pConnection;                          // 0x3c
	uint32          m_MsgFlags;                             // 0x40
	uint8           m_pad_44[0x50 - 0x44];
	CMoArray<uint8> m_Data;                                 // 0x50 .. 0x63
};

void CServerMessageInitQ12::Init(uint16 nMaxDataLen, uint16 nGrowSize)
{
	m_DataLen    = 1;
	m_MaxDataLen = nMaxDataLen;
	m_Pos        = 1;
	m_Unknown3a  = 1;
	m_MsgFlags   = 0;

	m_pOwner = this;

	if (nMaxDataLen > m_Data.GetSize())
	{
		m_Data.Term();
		m_Data.Init(nMaxDataLen, nGrowSize);
	}
	else
	{
		m_Data.SetCacheSize(nGrowSize);
	}
}
#endif


// 0x10013585 (91 bytes) — the object's facing as ONE byte: the rotation is
// turned into a matrix, the matrix gives up its basis vectors, and the
// forward vector's yaw goes out scaled to a byte.
//
// Read off the bytes, nothing here is guessed:
//
//   * the frame is exactly 0x64 = 64 + 3*12, so the four locals are a 4x4
//     matrix at -0x64 and three vectors at -0x24 / -0x18 / -0xc, in
//     GetBasisVectors' own right/up/forward argument order.
//   * 0x10015bcd is sharedcommon.h's quat_ConvertToMatrix and takes
//     (pRot, pMat) cdecl; 0x1001210b is ltmatrix.h's GetBasisVectors,
//     thiscall on the matrix, which server.dll CALLS -- /Os declines the
//     header's inline, the same split sb_Allocate and CMessageRef::Init
//     already record.
//   * the atan2 arguments are pushed -0x4 first and -0xc second
//     (0x100135ad / 0x100135b5), and cdecl pushes right to left, so the
//     FIRST argument is vForward.x and the second vForward.z -- the yaw,
//     not the pitch.
//   * 0x1004d7d0 is 0x4221b390 = 40.42535400390625 = 127/PI, and it is a
//     FLOAT constant: `fmuls`, not `fmull`, so it is folded from float
//     arithmetic and multiplies atan2's double straight off the stack.
//   * 0x1003e0f0 is __ftol and only %al is used, so the conversion target
//     is one byte wide; it lands in the dead pRot parameter slot, which is
//     allocator leftover and not a source fact.
//   * the byte goes out through CPacket::WriteType<uint8> itself: server.dll
//     calls it (0x1001393e), lithtech.exe EXPANDS it (0x0043620b..
//     0x004362ba, in its twin 0x004361e0).
//
// Here, not in smgr.cpp: both images put it in this file's run (server.dll
// between SetBasisVectors 0x10013509 and the end of the message code;
// lithtech.exe 0x004361e0 among 0x00435ad0 / 0x00435d90).
//
// lithtech.exe 0x004361e0 is byte-identical too, through a named CPacket*
// for the write: with the cast inline in the call, the expanded WriteType
// put m_Pos+1 in eax and m_MaxDataLen in edx where shipped has the reverse
// (219/219, three blocks). Without the named byte local server.dll's copy
// comes out 86 bytes.
void quat_ConvertToMatrix(LTRotation* pRot, LTMatrix* pMat);   // 0x10015bcd

void sub_10013585(ServerMessage* pMsg, LTRotation* pRot)
{
	LTMatrix mat;
	LTVector vRight, vUp, vForward;
	uint8    nYaw;

	quat_ConvertToMatrix(pRot, &mat);

	mat.GetBasisVectors(&vRight, &vUp, &vForward);

	nYaw = (uint8)((float)atan2(vForward.x, vForward.z) * (127.0f / MATH_PI));

	CPacket* pPacket = (CPacket*)pMsg;
	pPacket->WriteType(nYaw);
}
