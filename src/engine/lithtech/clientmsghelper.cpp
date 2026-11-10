// *********************************************************************** //
//
//  MODULE:   CLIENTMSGHELPER (.CPP)   (file name ours) -- lithtech.exe only
//
//  The client's message-object helper, vtable 0x004a1dcc: its constructor
//  and three of its four slots (AvP2 1.0.9.6 0x00415560 .. 0x00415664;
//  Primal Hunt 0x00419320 .. 0x00419410).  Its own translation unit: in
//  Primal Hunt the run sits between two <string> guard pairs and ends in
//  its own (0x00419420, 0x00419440).  Written in clientmgr2.cpp (the
//  constructor and slots 0 and 1) and csvis.cpp (slot 2) until this split;
//  their views are repeated here.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x419420).
#include <string>
#endif
#include <stdarg.h>

#include "ltbasedefs.h"


class CClientMgrView;

// clientmgr.h's m_nServerID: the id the server knows the object by, 0xffff
// for a client-only object.
class CClientObjIDView
{
public:
	uint8   m_pad_00[0xb8];
	uint16  m_nServerID;                          // 0x0b8
};

// The client manager as slot 2 reads it: the string module's record, at
// +0x61c in AvP2 and Primal Hunt (Primal Hunt 0x004193c2; NOLF's +0x618,
// csvis.cpp's CClientMgrQ414's m_pStringModule618).
class CClientMgrMsgQ414
{
public:
	uint8 m_pad_000[0x61c];
	void* m_pStringModule618;                     // 0x61c
};


class CClientMsgWriteView
{
public:
	virtual LTRESULT _p00()=0;
	// AVP2: iltmessage.h's ReadString, after Release (servermsg.h's
	// IMessageWrite has the same extra slot).
	virtual const char* ReadString()=0;                              // 0x04
	virtual LTRESULT _p01()=0;
	virtual LTRESULT ReadWord(uint16* pVal)=0;                       // 0x0c
	virtual LTRESULT _p03()=0;
	virtual LTRESULT _p04()=0; virtual LTRESULT _p05()=0;
	virtual LTRESULT _p06()=0; virtual LTRESULT _p07()=0;
	virtual LTRESULT _p08()=0; virtual LTRESULT _p09()=0;
	virtual LTRESULT _p10()=0; virtual LTRESULT _p11()=0;
	virtual LTRESULT _p12()=0; virtual LTRESULT _p13()=0;
	virtual LTRESULT _p14()=0; virtual LTRESULT _p15()=0;
	virtual LTRESULT WriteByte(uint8 val)=0;                         // 0x44
	virtual LTRESULT WriteWord(uint16 val)=0;                        // 0x48
};


class CClientMsgObjectHelper
{
public:
	CClientMsgObjectHelper(CClientMgrView* pClientMgr);         // 0x00415560

	virtual LTRESULT ReadObjectID(CClientMsgWriteView* pMsg,
	                              CClientObjIDView** ppObject);     // 0
	virtual LTRESULT WriteObjectID(CClientMsgWriteView* pMsg,
	                               CClientObjIDView* pObject);      // 1

	CClientMgrView* m_pClientMgr;                 // 0x04
};

// clienttexmgr.cpp's tbl_GetObject: the object in one of the manager's id
// slots, or null.  (Declared on this file's view of the manager; the real
// first parameter is clienttexmgr.cpp's CClientMgrTexBankView.)
void* tbl_GetObject(CClientMgrView* pClientMgr, uint16 nID);    // 0x00420520

// 0x00415560 — the constructor clientmgr3.cpp's factory calls for
// CClientMgr+0x4bc (`call 0x00415560` at 0x00410d94): the vptr 0x004a1dcc,
// then the manager, `this` back in eax, `ret $0x4`.
CClientMsgObjectHelper::CClientMsgObjectHelper(CClientMgrView* pClientMgr)
{
	m_pClientMgr = pClientMgr;
}

// 0x00415580 — slot 0: an object id off the message, resolved to the object.
//
// The server's ReadObjectID with the Verify() test and the object-list
// search taken out: the client looks the id up in its id table instead.
LTRESULT CClientMsgObjectHelper::ReadObjectID(CClientMsgWriteView* pMsg,
                                              CClientObjIDView** ppObject)
{
	uint16 nID;

	*ppObject = LTNULL;
	pMsg->ReadWord(&nID);

	if (nID != 0xffff)
		*ppObject = (CClientObjIDView*)tbl_GetObject(m_pClientMgr, nID);

	return LT_OK;
}

// 0x004155c0 — slot 1: an object's server id onto the message, or 0xffff.
//
// The server's WriteObjectID with its Verify() split taken out: the client
// only ever has the one id.  `this` is never read.  Each arm has its own
// `call *0x44(%edx); xor %eax,%eax; ret $0x8`, so it is an if/else of two
// calls and one LT_OK.
LTRESULT CClientMsgObjectHelper::WriteObjectID(CClientMsgWriteView* pMsg,
                                               CClientObjIDView* pObject)
{
	if (pObject)
		pMsg->WriteWord(pObject->m_nServerID);
	else
		pMsg->WriteWord(0xffff);

	return LT_OK;
}



// ======================================================================= //
//  THE CLIENT'S MESSAGE-OBJECT HELPER -- vtable 0x004a1dcc
//
//  Slot 0 0x00415580 (2 args), slot 1
//  0x004155c0 (2), slot 2 0x004155f0 (3), slot 3 0x0043ec90.  That is
//  src/engine/server/servermessage.cpp's CMessageObjectHelper, the client's
//  copy: ReadObjectID / WriteObjectID / WriteFormattedString / GetWorld.
//  0x004155c0 is WriteObjectID without the server's Verify() split (one
//  `mov 0xb8(%eax),%ax`, else 0xffff), and 0x004155f0 below is the server's
//  0x100350bc with the client manager at +0x04 in place of the server
//  manager, and the module record read from the manager's +0x618 in place
//  of CClassMgr::m_pUnknown08.  Only slot 2 is written here (it is the one
//  handed to this file).  (Q414 names ours; the method name is the server
//  twin's.)
// ======================================================================= //

// servermsg.h's IMessageWrite, cut to the two slots this reaches.
class IMessageWriteQ414
{
public:
	virtual void _v00()=0;
	// AVP2: iltmessage.h's ReadString, which AvP2's SDK adds after Release:
	// every slot below is one later than NOLF's (servermsg.h's IMessageWrite)
	virtual const char* ReadString()=0;
	virtual void _v04()=0; virtual void _v08()=0;
	virtual void _v0c()=0; virtual void _v10()=0; virtual void _v14()=0;
	virtual void _v18()=0; virtual void _v1c()=0; virtual void _v20()=0;
	virtual void _v24()=0; virtual void _v28()=0; virtual void _v2c()=0;
	virtual void _v30()=0; virtual void _v34()=0; virtual void _v38()=0;
	virtual void _v3c()=0; virtual void _v40()=0;
	virtual LTRESULT WriteWord(uint16 val)=0;                  // 0x44
	virtual void _v48()=0; virtual void _v4c()=0; virtual void _v50()=0;
	virtual void _v54()=0; virtual void _v58()=0; virtual void _v5c()=0;
	virtual void _v60()=0; virtual void _v64()=0;
	virtual LTRESULT WriteRaw(void* pData, uint32 len)=0;      // 0x68
};

// servermessage.cpp's clm_GetRecordModule (0x00477f30, byte-identical in
// both images) and clientstreams.cpp's str_FormatMessage / str_FreeMessage.
void* clm_GetRecordModuleQ414(void* pRecord);                  // 0x00477f30
void* str_FormatMessageQ414(void* pModule, int messageCode,
                            va_list* pMarker, uint32* pLen);   // 0x00484780
void  str_FreeMessageQ414(void* pRaw);                         // 0x00484840

class CMessageObjectHelperQ414
{
public:
	LTRESULT WriteFormattedString(IMessageWriteQ414* pMsg,
	                              uint32 messageCode,
	                              va_list* pMarker);           // 0x004155f0

	void*           m_pVtable;                                 // 0x00
	CClientMgrMsgQ414* m_pClientMgr;                              // 0x04
};

// 0x004155f0 -- the server twin's body word for word, down to the length
// written over the pMarker PARAMETER slot (`lea 0xc(%esp),%eax` pushed as
// pLen, then `mov 0x10(%esp),%ecx` reading it back as the length).
LTRESULT CMessageObjectHelperQ414::WriteFormattedString(IMessageWriteQ414* pMsg,
                                                        uint32 messageCode,
                                                        va_list* pMarker)
{
	void* pRaw = str_FormatMessageQ414(
		clm_GetRecordModuleQ414(m_pClientMgr->m_pStringModule618),
		messageCode, pMarker, (uint32*)&pMarker);

	if (pRaw)
	{
		pMsg->WriteWord((uint16)(uint32)pMarker);
		pMsg->WriteRaw(pRaw, (uint32)pMarker);
		str_FreeMessageQ414(pRaw);
	}
	else
	{
		pMsg->WriteWord(0);
	}

	return LT_OK;
}
