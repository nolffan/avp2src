// *********************************************************************** //
//
//  MODULE:   Q478000 (.CPP)   — staging translation unit (name ours)
//
//  PURPOSE:  The CLIENT's server-to-client packet handlers out of
//            lithtech.exe, 0x00478000..0x0047c000, read off
//            the retail lithtech.exe ONLY.
//
//  HOW THE MODULE WAS IDENTIFIED (data first)
//  ------------------------------------------
//  The eight CF_ literals in .data (0x004ae570 CF_ATTACHMENTS, 0x004ae580
//  CF_COLORINFO, 0x004ae590 CF_MODELINFO, 0x004ae5a0 CF_SCALE, 0x004ae5ac
//  CF_FLAGS, 0x004ae5b8 CF_ROTATION, 0x004ae5c4 CF_POSITION, 0x004ae5d0
//  CF_NEWOBJECT) are NOT a table.  Every one of them is a `push imm32` in
//  ONE function, 0x00479160, and each push sits behind its own
//  `test $bit,%bl` — a linear chain of eight `if (nFlags & CF_x)
//  con_Print("CF_x");` statements, bits 0x01,0x02,0x04,0x08,0x10,0x20,0x40
//  and 0x0100 in that order.  Those bit values are exactly
//  src/engine/server/servermodel.h's CF_NEWOBJECT..CF_ATTACHMENTS, so the
//  client and the server share the change-flag header.  0x00479160 is a
//  debug trace, gated on a console int at 0x004b32dc (== 2 prints the whole
//  set, == 1 prints only the CF_NEWOBJECT line).
//
//  The rest of the run names itself in check literals:
//      0x004ae4c4 "OnMessageGroupPacket"     0x004ae504 "OnUpdatePacket"
//      0x004ae514 "ReadObjectSubPacket"      0x004ae610 "OnSkyDef"
//      0x004ae61c "OnPortalFlagsPacket"      0x004ae630 "OnNetProtocolVersionPacket"
//      0x004ae650 "OnThreadLoadPacket"
//  and none of those strings is in the retail server.dll.  This is the
//  client message dispatch, i.e. clientmgr.cpp, exactly as the banner in
//  src/engine/lithtech/clientsoundinst.cpp predicted.
//
//  THE DISPATCH TABLE — the single most useful thing in the run.
//  0x00477fe0 (207 bytes, NOT one of this run's rows) zeroes 0x100 dwords at
//  0x004b48d0 and then stores nineteen handlers into it.  Index =
//  (slot - 0x004b48d0) / 4, i.e. the server-to-client message id:
//
//      id  4  0x0047b2b0     id 14  0x00478320     id 21  0x0047af20
//      id  5  0x00478300     id 15  0x0047a4e0     id 23  0x0047b380
//      id  6  0x004781d0     id 16  0x0047a790     id 24  0x0047b520
//      id  7  0x0047ac80     id 17  0x0047a7d0     id 25  0x0047b700
//      id  8  0x00478510     id 18  0x0047ac40     id 26  0x0047a970
//      id 10  0x0047a1f0     id 19  0x0047ad20
//      id 12  0x004780b0     id 13  0x0047a480
//
//  Every one of them is __cdecl with two arguments and returns an LTRESULT,
//  which is what fixes the signature of all the "gap" rows below without
//  having to guess at one: the first argument is the CClientShell
//  (src/engine/lithtech/clientshell.cpp already has its layout: m_nConnID
//  0x34, m_Unknown38 0x38, m_bLocal 0x3c, m_Unknown6c 0x6c, m_pClientMgr
//  0x78) and the second is the CPacket being read.
//  The STORE ORDER in 0x00477fe0 is source order and is 12, 6, 5, 8, 10, 13,
//  14, 15, 16, 17, 26, 18, 7, 19, 21, 4, 23, 24, 25 — recorded here because
//  whoever reconstructs 0x00477fe0 will need it.
//
//  CPacket ITSELF IS ALREADY WRITTEN.  src/engine/server/packet.h carries
//  ReadType/WriteType as member templates with m_DataLen 0x34,
//  m_MaxDataLen 0x36, m_Pos 0x38, m_Flags 0x40, m_Data 0x50 (m_pData 0x54,
//  m_AllocSize 0x58).  Every inlined read in this file was checked against
//  the shipped bytes field by field:
//      mov 0x38(%esi),%cx      pos  = m_Pos
//      mov 0x58(%esi),%edi     m_Data.m_AllocSize
//      lea N(%eax),%edx        end  = pos + sizeof(T)
//      cmp %edi,%edx ; jae     !(end <  m_Data.m_AllocSize)
//      mov 0x34(%esi),%di      m_DataLen
//      cmp %edi,%edx ; ja      !(end <= m_DataLen)
//      or  $0x2,m_Flags        PACKETFLAG_READOVERFLOW
//  which is packet.h's body verbatim.  Nothing here re-derives it.
//
//  GROUND TRUTH: the retail lithtech.exe.  Every address in a comment was
//  read from it.  Names that are ours say so.
//
// *********************************************************************** //

#include <string.h>

#include "ltbasedefs.h"
#include "ltcodes.h"

#include "de_error.h"
#include "packet.h"
#include "iltclient.h"


// ---- what the handlers reach outside this run --------------------------- //

// 0x0041b8a0 — ccclient.cpp / clientmgr3.cpp already name it: colour first,
// then the debug level, then a printf format.
void con_PrintColored(uint32 nColor, int nLevel, char* pFormat, ...);
// 0x0041b8c0 — clientconsole.cpp: the plain white printer.
void con_Print(char* pFormat, ...);

// The trace colour the client's packet chatter uses; 0x006464fa is pushed
// literally at 0x004781b6 and is not a relocation, so the VALUE matters.
#define PACKETTRACE_COLOR 0x006464fa


class CClientMgrQ478;
class CFileMgrQ478;
class CWorldNodeQ478;
class CModelQ478;

// CPacket with the two OUT-OF-LINE readers this run uses.  packet.h is not
// this reconstruction's to change, so they are added on a derived view: no
// data members and no virtuals, so the layout, `this` and the inherited
// ReadType expansion are all unchanged.
//
//   0x0045c2d0  __thiscall, `ret $0x8`.  Clamps the length against
//               m_MaxDataLen - m_Pos (its prologue reads 0x38 and 0x36),
//               so it is a bulk read.  Its length parameter is sixteen bits:
//               the caller at 0x00478415 pushes a register whose upper half
//               is stale.  (name ours)
//   0x00459f80  __thiscall, `ret $0x4`, one argument, and only %al of the
//               result is ever used.  (name by address)
class CPacketQ478 : public CPacket
{
public:
	void  ReadData(void* pDest, uint16 nLen);                   // 0x0045c2d0
	uint8 Sub459f80(int nFlag);                                 // 0x00459f80

	// THE OUT-OF-LINE ReadType INSTANTIATIONS. packet.h defines ReadType in
	// class, so every use of the template EXPANDS under /Ob1 — but the
	// shipped image CALLS a reader at fifteen sites and expands at six, and
	// the split is not arbitrary: everything before the attachment loop in
	// 0x00478b40 is a call and everything inside it is an expansion. Declared
	// here with the template's own shape (one dummy pointer, __thiscall,
	// `ret $0x4`) so the call sites that call can be spelled as calls. Same
	// convention smgr.cpp already uses for the writers.
	//
	// Sub459f80 above is the uint8 instantiation under its older name.
	uint8  ReadTypeOOL(uint8*  dummy);                          // 0x00459f80
	uint16 ReadTypeOOL(uint16* dummy);                          // 0x0042f950
	uint32 ReadTypeOOL(uint32* dummy);                          // 0x0042f9d0
	float  ReadTypeOOL(float*  dummy);                          // 0x0047bc50

	// 0x0045c280 -- __thiscall, no arguments, plain `ret`; the string
	// reader (server.dll's 0x1002609f).  Only the returned pointer is used.
	// (name ours)
	char*  ReadStringQ478();                                    // 0x0045c280

	// Inlined at 0x0047a488..0x0047a4b2: one byte taken off the END of the
	// packet.  `(m_DataLen - m_Pos) cmp $1 ; jb` is an UNSIGNED compare
	// against sizeof, the byte is `m_pData[m_DataLen - 1]`, the length
	// drops by one, and the short arm stores a zero.  (name ours)
	uint8  PopByteQ478()
	{
		uint8 val;
		if ((uint32)(m_DataLen - m_Pos) >= sizeof(uint8))
		{
			val = m_Data.m_pArray[m_DataLen - sizeof(uint8)];
			m_DataLen -= sizeof(uint8);
		}
		else
		{
			val = 0;
		}
		return val;
	}
};


// CClientShell, the object at CClientMgr+0x150c, as this run reaches it.
// Every offset below is a shipped displacement and every one of them is
// already recorded in src/engine/lithtech/clientshell.cpp under the same
// names; the class is repeated here on a VIEW so this TU does not have to
// pull the whole shell in, exactly as lt2new1.cpp does.  Suffixed Q478 so
// it cannot collide at link time.
class CClientShellQ478
{
public:
	uint8   m_pad_0000[0x34];
	uint32  m_nConnID;                       // 0x034  (clientmgr2.cpp)
	// 0xffff at birth (clientshell.cpp); the server stamps the client's own
	// id into it here, which is what clientmgr.h calls m_nLocalID and what
	// ci_GetLocalClientID hands back.
	uint16  m_Unknown38;                     // 0x038
	uint8   m_pad_003a[0x3c - 0x3a];
	LTBOOL  m_bLocal;                        // 0x03c  (clientshell.cpp)
	uint8   m_pad_0040[0x6c - 0x40];
	uint16  m_Unknown6c;                     // 0x06c  (clientmgr2.cpp)
	uint8   m_pad_006e[0x78 - 0x6e];
	CClientMgrQ478* m_pClientMgr;            // 0x078  (clientmgr2.cpp)
	uint8   m_pad_007c[0x88 - 0x7c];
	// 0x088 -- clientshell.cpp's m_Unknown88: an object whose +0x18c
	// (LTObject::m_Pos) 0x0047a08d copies out.  (name ours)
	class CShellObjQ478* m_pClientObject;    // 0x088

	// 0x00414b10 — `mov 0x78(%ecx),%eax ; ret`.  clientmgr2.cpp already has
	// it: "four bytes: the back pointer, straight out", __thiscall, no
	// arguments.  It matters that it is a real call and not the field: block
	// 8 uses BOTH in one function.
	CClientMgrQ478* GetClientMgr();                             // 0x00414b10

	// 0x00414a50 -- csvis.cpp's CClientShellObjMapQ414::GetObjRecord: the
	// shell's per-id record (found in, or added to, the +0x8c hash).
	void* GetObjRecord(uint16 nID);                             // 0x00414a50

	// 0x00413d30 -- startload.cpp's CClientShell::DoLoadWorld: __thiscall,
	// two dword arguments (`ret $0x8` at 0x00413f1c); 0x004781df loads the
	// shell into %ecx for it.
	LTRESULT DoLoadWorld(CPacketQ478* pPacket, int nFlag);       // 0x00413d30
};


// ======================================================================= //
//  1.  0x004780b0 — message id 12, "Got ID packet"          283 bytes
//
//  THE ROW IS NOT ONE FUNCTION.  The function enumeration gives
//  0x004780b0 a size of 586, which runs to 0x004782fa; the `ret` at
//  0x004781cb is followed by four alignment `nop`s and 0x004781d0 is the
//  message-id-6 handler (block 2).  This block is 0x004780b0..0x004781cb,
//  283 bytes.
//
//  WHAT PINS IT
//   * the dispatch table stores it at 0x004b4900, so id 12, __cdecl, two
//     arguments; `mov 0x10(%esp),%esi` after two pushes is argument TWO
//     (the packet) and `mov 0x10(%esp),%edi` after three is argument ONE
//     (the shell).
//   * two inlined packet.h reads, `lea 0x2(%eax),%edx` then
//     `lea 0x1(%eax),%edx`: uint16 then uint8, in that order.
//   * `mov %dx,0x38(%edi)` is a SIXTEEN-bit store, so the member is uint16;
//     `and $0xff,%edx ; mov %edx,0x3c(%edi)` is a thirty-two bit store of a
//     byte value, so that member is an int (LTBOOL) and the read is uint8.
//   * the console call pushes `0x38(%edi)` RE-READ through
//     `xor %eax,%eax ; mov 0x38(%edi),%ax` at 0x004781a2 rather than the
//     value still sitting in the temporary — the member is named again in
//     the source, not cached in a local (an expression repeated
//     rather than cached is source-reachable).
//   * the store to 0x3c is scheduled between the pushes but its VALUE was
//     computed at 0x0047819e, before them; statement order is read16,
//     store38, read8, store3c, print.
//   * `xor %eax,%eax ; pop ; pop ; pop ; ret` — one exit, LT_OK.
//
//  The function has NO `sub $N,%esp`: the only stack the two inlined
//  ReadType temporaries get is argument two's home slot at 0x14(%esp).
//  That is allocation, not source.
//
//  CONFIDENCE in byte identity: MEDIUM-HIGH.  The shape is small and every
//  instruction is accounted for; the risk is packet.h's inline expanding
//  differently in a client TU than it did in the server one.
// ======================================================================= //

LTRESULT sub_4780b0(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	pShell->m_Unknown38 = pPacket->ReadType((uint16*)0);
	pShell->m_bLocal    = pPacket->ReadType((uint8*)0);

	// 0x004ae4b0
	con_PrintColored(PACKETTRACE_COLOR, 1, "Got ID packet (%d)",
	                 pShell->m_Unknown38);

	return LT_OK;
}


// ---- what the two "start a new message and send it" handlers need ------- //

// The write message.  It is a CPacket (packet.h's layout, member for member:
// 0x34/0x36/0x38/0x40/0x50) that ALSO has a vtable with AddRef at slot 1 and
// Release at slot 2 — src/engine/lithtech/clientshell.cpp reached the same
// object as `ILTMessageView` and pinned m_pData at 0x54, which is
// CPacket::m_Data.m_pData.  The two views are combined here because this run
// needs both halves in one expression: the shipped code does
// `mov (%esi),%edx ; call *0x4(%edx)` (a real virtual call, so the members
// must be declared virtual) and then expands WriteType INLINE off the same
// pointer.  Pure so that VC6 never asks for a vtable to emit — the same
// device clientshell.cpp uses for ILTMessageView.
//
// WriteType below is packet.h's, report helper included.  Its one user is
// sub_4781d0, which with the helper is byte-identical: Insert2 is then
// called as shipped, and this object's BaseNew/_DeleteAndDestroyArray
// COMDATs (0x0040c060/0x00413050, identical from other objects) give way to
// Insert2's own (0x00412f50).
// CMoArray<uint8>::Init as sub_478320's reset reaches it: Term's SetSize2
// (0x0040bfe0), _InitArray (0x00433570) and SetSize2 again are all CALLS
// there (0x004783d9 / 0x004783f3 / 0x00478400), where collection.h's real
// template lets /O2 expand the 21-byte _InitArray (measured: 497 bytes
// against 491).  The same four-call split is in CNetMgrSendView::SendPacket
// (0x00457b45..0x00457b6c) and smgr.cpp's CSFXPacketDataView; this is that
// stand-in, stating VC6's inline budget rather than reproducing it.
// (names ours)
class CMsgDataViewQ478
{
public:
	LTBOOL SetSize2(uint32 newSize, LAlloc* pAlloc);        // 0x0040bfe0
	void   InitArray(uint32 wantedCache);                   // 0x00433570

	LTBOOL Init(uint32 size, uint32 cacheSize)
	{
		SetSize2(0, &g_DefAlloc);
		InitArray(cacheSize);

		return SetSize2(size, &g_DefAlloc);
	}
};

class CMsgQ478
{
public:
	virtual void _p00()=0;
	virtual void AddRef()=0;                 // slot 1 (0x04)
	virtual void Release()=0;                // slot 2 (0x08)

	uint8       m_pad_04[0x10 - 0x04];
	CMsgQ478*   m_pSelf10;                   // 0x10  (0x004783c3)
	uint8       m_pad_14[0x34 - 0x14];
	uint16      m_DataLen;                   // 0x34
	uint16      m_MaxDataLen;                // 0x36
	uint16      m_Pos;                       // 0x38
	uint16      m_Unknown3a;                 // 0x3a  (0x004783b8)
	uint8       m_pad_3c[0x40 - 0x3c];
	uint32      m_Flags;                     // 0x40
	uint8       m_pad_44[0x50 - 0x44];
	CMoArray<uint8> m_Data;                  // 0x50

	void Init(uint16 nMaxDataLen, uint16 nGrowSize)
	{
		m_DataLen    = 1;
		m_MaxDataLen = nMaxDataLen;
		m_Pos        = 1;
		m_Unknown3a  = 1;
		m_Flags      = 0;

		m_pSelf10 = this;

		if (nMaxDataLen > m_Data.GetSize())
		{
			m_Data.Term();
			((CMsgDataViewQ478*)&m_Data)->Init(nMaxDataLen, nGrowSize);
		}
		else
		{
			m_Data.SetCacheSize(nGrowSize);
		}
	}

	// packet.h's always-expanded report, as CPacket has it: its expansion
	// spends the /O2 inline budget that keeps Append's Insert2 a CALL
	// (0x00478268 -> 0x00412f50), as shipped.
	__forceinline void ReportWriteOverrun()
	{
		REPORT_ERROR_INFO(1, "CPacket::WriteType", LT_ERROR, "ERROR: Packet write overrun!");
	}

	template<class T>
	void WriteType(T val)
	{
		if (m_Pos + sizeof(T) < m_MaxDataLen)
		{
			while (m_DataLen + sizeof(T) >= m_Data.GetSize())
				m_Data.Append(0);

			*(T*)&m_Data.m_pArray[m_Pos] = val;
			m_Pos += sizeof(T);
			m_DataLen += sizeof(T);
		}
		else
		{
			ReportWriteOverrun();
			m_Flags |= PACKETFLAG_WRITEOVERFLOW;
		}
	}
};

// clientshell.cpp's CMsgRef, unchanged: the counted handle msg_StartWrite
// hands back by value.  The copy AddRefs and the destructor Releases, which
// is the `if (p) p->AddRef()` / `if (p) p->Release()` pair the shipped bytes
// show either side of the assignment at 0x0047820e..0x0047822a.
class CMsgRefQ478
{
public:
	CMsgRefQ478(const CMsgRefQ478& other)
	{
		m_pMsg = other.m_pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();
	}

	~CMsgRefQ478()
	{
		if (m_pMsg)
			m_pMsg->Release();
	}

	CMsgQ478* m_pMsg;                        // 0x00
};

// 0x0045c080 — __cdecl, returns the holder BY VALUE through a hidden
// pointer argument (clientshell.cpp block 11 derived this; the shipped
// `push $0x1000 ; push $0x1000 ; push %eax ; call ; add $0xc,%esp` is the
// same three-dword site).  (name clientshell.cpp's)
CMsgRefQ478 msg_StartWrite(uint32 nMaxSize, uint32 nSize);

// clientshell.cpp's CMsgHold: the message taken out of the returned holder
// and held on its own (AddRef at 0x00478348, the temporary's Release at
// 0x00478355, this one's on the way out).
class CMsgHoldQ478
{
public:
	CMsgHoldQ478(CMsgQ478* pMsg)
	{
		m_pMsg = pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();
	}

	~CMsgHoldQ478()
	{
		if (m_pMsg)
			m_pMsg->Release();
	}

	CMsgQ478* m_pMsg;
};

// CNetMgr, as far as this run reaches it.  0x00456360 is __thiscall with
// three dword arguments, NOT the four-argument cdecl free function
// src/engine/lithtech/clientmgr2.cpp declares: the shipped site at
// 0x004782dd pushes three dwords, loads %ecx, and does no `add $N,%esp`
// afterwards, and 0x00456360 itself opens `mov 0x8(%esp),%edx ; push %ebp`
// with `this` already in %ecx.
class CNetMgrQ478
{
public:
	// Padded to four bytes ONLY so that the member below it lands at 0x510;
	// the real CNetMgr is much larger and nothing here depends on its size.
	uint8 m_pad_00[0x04];

	void SendTo(CMsgQ478* pMsg, uint32 nConnID, uint32 nFlags);  // 0x00456360
};

// CClientMgr, as far as this run reaches it.  0x50c is clientmgr2.cpp's and
// clientshell.cpp's CNetMgr sub-object; 0x1160 and 0x11a0 are this run's,
// read off OnSkyDef (block 4).
// The world tree at CClientMgr+0x6c (0x004205de `lea 0x6c(%ebx),%ecx`).
// 0x0048cde0 is __thiscall(object, 0): worldtree2.cpp's AddObject.
class CWorldTreeQ478
{
public:
	uint8 m_pad_00[0x04];
	void AddObject(void* pObj, uint32 nListIndex);           // 0x0048cde0
};

// IClientShell, as far as 0x0047a4d8 reaches it: slot 5 (+0x14) is
// iclientshell.h's OnMessage(uint8 messageID, HMESSAGEREAD).
class CClientShellIfQ478
{
public:
	virtual void _v00();  virtual void _v04();  virtual void _v08();
	virtual void _v0c();  virtual void _v10();
	virtual void OnMessage(uint8 messageID, void* hMessage);   // +0x14
};

class CClientMgrQ478
{
public:
	uint8       m_pad_0000[0x6c];
	CWorldTreeQ478 m_WorldTree;              // 0x06c
	uint8       m_pad_0070[0x18c - 0x70];
	// 0x0047ae29: `mov 0x18c(%ebp),%ecx ; mov (%ecx,%edi,4),%edi` with the
	// index bounds-checked against 0x190 first.  A pointer array and its
	// count.  (names ours)
	CWorldNodeQ478** m_pWorldNodes;          // 0x18c
	uint32      m_nWorldNodes;               // 0x190
	uint8       m_pad_0194[0x4c0 - 0x194];
	// 0x4c0 -- clientmgr.h's m_pClientDE, the client interface (0x0047a984).
	ILTClient*  m_pClientDE;                 // 0x4c0
	uint8       m_pad_04c4[0x508 - 0x4c4];
	// 0x508 -- clientmgr.h's m_pUnknown508, the WMMoveState's second
	// pointer (0x0042071a).
	void*       m_pUnknown508;               // 0x508
	CNetMgrQ478 m_NetMgr;                    // 0x50c
	uint8       m_pad_0510[0x61c - 0x510];
	// clientmgr2.cpp's m_pClientShell (lt2new1.cpp's 0x0047ac40 too).
	CClientShellIfQ478* m_pClientShell;      // 0x61c
	uint8       m_pad_0620[0x1160 - 0x620];
	// Thirty sixteen-bit ids.  The count comes from TWO independent places:
	// `mov $0xf,%ecx ; rep stos` fills sixty bytes, and the packet is
	// rejected when its count exceeds 0x1e.  Filled with 0xffff, which is
	// this engine's "no object" id everywhere else.  (name ours)
	uint16      m_SkyObjects[30];            // 0x1160
	// 0x119c — thirty uint16 end here (0x1160 + 0x3c), but OnSkyDef's
	// `mov 0x78(%edi),%eax ; add $0x11a0,%eax` at 0x0047a7e3 puts the next
	// member four bytes further on, so a dword sits in between.  Nothing in
	// this run reads it.  (name ours)
	uint32      m_Unknown119C;               // 0x119c
	// 0x30 bytes read straight off the packet, before anything else in
	// OnSkyDef.  Nothing in this run looks inside it.  (name ours)
	uint8       m_SkyDef[0x30];              // 0x11a0
	uint8       m_pad_11d0[0x1268 - 0x11d0];
	// The client's ConsoleState: 0x0047a7af `add $0x1268,%ecx` is the
	// first argument of con_SetVar.  (name ours)
	uint8       m_ConsoleState[0x04];        // 0x1268
	uint8       m_pad_126c[0x150c - 0x126c];
	// clientmgr.h's m_pConnection; 0x004206f8 only tests it for null.
	void*       m_pConnection;               // 0x150c
	uint8       m_pad_1510[0x1534 - 0x1510];
	// clientmgr3.cpp / clientwin.cpp already reach CClientMgr+0x1534 as the
	// file manager every 0x00404680 lookup is made on.
	CFileMgrQ478* m_pFileMgr;                // 0x1534

	// 0x00411440 -- `mov 0x4bc(%ecx),%eax ; mov 0x4(%esp),%ecx ;
	// mov %eax,0xc(%ecx) ; ret $0x4` (lt2new1.cpp's sub_411440).
	void sub_411440(CPacketQ478* pPacket);                   // 0x00411440

	// __cdecl varargs member (clientrender.cpp / clientshell.cpp): the
	// shipped sites push `this` LAST and clean up themselves, and
	// 0x0041fc60 formats into `this + 0x620` with a 0x12c cap.
	LTRESULT SetupError(LTRESULT dResult, ...);              // 0x0041fc60
};

// 0x00413d30 — __thiscall on CClientShell, two dword arguments (`ret $0x8`
// at 0x00413f1c): startload.cpp's DoLoadWorld, the world/level bring-up the
// id-6 handler runs before acknowledging.  Declared on CClientShellQ478
// above.

// 0x004143b0 — clientmgr2.cpp's cs_TermWorld: the whole world teardown,
// cdecl, one argument.
void cs_TermWorld(CClientShellQ478* pShell);


// ======================================================================= //
//  2.  0x004781d0 — message id 6                            298 bytes
//
//  The SECOND function inside the 586-byte 0x004780b0 row (see block 1).
//  0x004b48e8 in the dispatch table, so id 6, __cdecl, two arguments.
//
//  WHAT PINS IT
//   * `push %ecx` at entry and no `sub $N,%esp`: four bytes of local, which
//     is the LTRESULT the function both branches on and returns.
//   * ONE shared exit, `mov %edi,%eax` at 0x004782f3, reached BOTH from the
//     guard (`test %edi,%edi ; jne` at 0x004781f4) and from the end of the
//     body.  One shared exit with several producers is a
//     nested POSITIVE guard and a single trailing `return`, not an early
//     `return dResult;` — and the value returned on the success path is the
//     same local, so the trailing statement is `return dResult;` too.
//   * the fall-through arm of that test is the body, so the guard is
//     `if (dResult == LT_OK)`.
//   * `movw $0xffff,0x6c(%ebp)` is issued BEFORE the call at 0x004781e7
//     even though the call's arguments were already pushed, so the store is
//     the first statement.
//   * `push $0x0` precedes `push %eax`, and VC6 pushes right to left, so
//     the packet is argument one of 0x00413d30 and the 0 argument two.
//   * `mov 0x54(%esi),%ecx ; movb $0x9,(%ecx)` is the message type stamped
//     into element zero of the payload — clientshell.cpp block 11 pinned
//     this exact idiom (it reads `= 10` there) — so the type is 9.
//   * the write that follows is packet.h's WriteType INLINE with
//     `lea 0x1(...)`-style one-byte arithmetic (`inc %edx`, `inc %ecx`,
//     `incw 0x38`, `incw 0x34`) and a `movb $0x0` store: WriteType<uint8>
//     of the value zero.
//   * the send is three pushes plus %ecx = m_pClientMgr + 0x50c with no
//     caller cleanup: CNetMgr::SendTo(pMsg, m_nConnID, 0x80).
//   * the trailing `if (p) p->Release()` at 0x004782e8 is cMsg leaving
//     scope; the guard's exit skips it, which is why cMsg has to be scoped
//     INSIDE the if and not declared at the top.
//
//  BYTE-IDENTICAL, 298 bytes.  The message is HELD, not kept as the
//  returned CMsgRef: 0x0047820e..0x0047822a takes m_pMsg out of the
//  temporary (AddRef), then releases the temporary, which lives in the
//  dead packet-argument slot -- sub_478320's CMsgHoldQ478 exactly.  Kept as
//  the CMsgRef it was 489 bytes with a 0x14 frame.
// ======================================================================= //

LTRESULT sub_4781d0(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	LTRESULT dResult;

	pShell->m_Unknown6c = 0xffff;

	dResult = pShell->DoLoadWorld(pPacket, 0);

	if (dResult == LT_OK)
	{
		CMsgHoldQ478 cMsg(msg_StartWrite(0x1000, 0x1000).m_pMsg);

		cMsg.m_pMsg->m_Data.m_pArray[0] = 9;
		cMsg.m_pMsg->WriteType((uint8)0);

		pShell->m_pClientMgr->m_NetMgr.SendTo(cMsg.m_pMsg, pShell->m_nConnID,
		                                      0x80);
	}

	return dResult;
}


// ======================================================================= //
//  2b. 0x00478300 — message id 5                             22 bytes
//
//  NOT one of this run's rows (the function enumeration has it as
//  0x00478300,22,gap) but it is the other half of the same statement pair
//  as block 2 and it costs five lines, so it is written here rather than
//  left for a later run to re-derive.
//
//      mov 0x4(%esp),%eax ; push %eax ; movw $0xffff,0x6c(%eax)
//      call 0x004143b0 ; add $0x4,%esp ; xor %eax,%eax ; ret
//
//  cdecl, one argument USED (the shell) — the packet argument is never
//  touched, which is normal for a table entry whose signature is fixed by
//  the table.  The store precedes the call; the `xor` is after it, so the
//  zero is the return value.
//
//  CONFIDENCE in byte identity: HIGH.
// ======================================================================= //

LTRESULT sub_478300(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	pShell->m_Unknown6c = 0xffff;

	cs_TermWorld(pShell);

	return LT_OK;
}


// ======================================================================= //
//  2c. 0x00478320 — OnMessageGroupPacket, message id 14     491 bytes
//
//  The packet is a run of sub-messages, each a length byte and then that
//  many bytes whose first byte is the sub-message's own id.  One write
//  message is taken from msg_StartWrite up front and RE-USED for every
//  sub-message: reset in place (m_MaxDataLen = the length, m_DataLen,
//  m_Pos and +0x3a = 1, m_Flags = 0, +0x10 = itself), its payload array
//  re-initialised only when it is too small (Term, then Init(nLen, 0x1000)
//  expanded: 0x0040bfe0(0) twice, _InitArray 0x00433570, 0x0040bfe0(nLen))
//  and otherwise just given its 0x1000 wanted cache, filled by the bulk read
//  0x0045c2d0, and dispatched through the same table (0x004b48d0) with
//  (pShell, sub-message).  A handler's failure is returned; a zero length
//  ends the group.  A length longer than what is left is
//  SetupError(LT_INVALIDSERVERPACKET) and the "OnMessageGroupPacket" /
//  "invalid packet" report (0x004ae4c4 / 0x004ae4f4).  The write message is
//  released on every way out.
//
//  Written from the shipped bytes to give 0x0047a1f0 (id 10) the callee it
//  calls.  BYTE-IDENTICAL, 491 bytes, once three things were right:
//  the reset is ONE inline (CMsgQ478::Init, servermsg.h's ServerMessage::
//  Init shape) whose array Init goes through the CMsgDataViewQ478 stand-in
//  (all four CMoArray calls are calls); the id goes through a pointer to
//  its uint8 local (clientshell.cpp Sub47bab0's device: the shipped
//  `mov %cl,0x14(%esp) ; mov 0x14(%esp),%edx ; and $0xff,%edx` and the
//  8-byte frame); and it is read as `m_Data[0]` -- `m_Data.m_pArray[0]`
//  lets VC6 hoist the m_pArray load above the two stores before it.
// ======================================================================= //

// clientshell.cpp's dispatch table (0x004b48d0).  The one object is
// cmodelload.cpp's g_Table4b48d0Q474 (cm_InitTable4b48d0Q474 zeroes and
// fills it), an ARRAY of void*, so its spelling is used and the entry cast
// on the way out, as clientshell.cpp does.
class CClientShell;
class ILTMessageView;
typedef LTRESULT (*ClientPacketHandlerFn)(CClientShell* pShell, ILTMessageView* pMsg);
extern void* g_Table4b48d0Q474[0x100];                         // 0x004b48d0

LTRESULT sub_478320(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	CMsgHoldQ478          cMsg(msg_StartWrite(0x1000, 0x1000).m_pMsg);
	CMsgQ478*             pSub = cMsg.m_pMsg;
	uint16                nLen;
	uint8                 nID;
	ClientPacketHandlerFn pHandler;
	LTRESULT              dResult;

	while (pPacket->m_DataLen - pPacket->m_Pos > 0)
	{
		nLen = pPacket->ReadTypeOOL((uint8*)0);

		if (nLen > pPacket->m_DataLen - pPacket->m_Pos)
		{
			pShell->m_pClientMgr->SetupError(LT_INVALIDSERVERPACKET);

			RETURN_ERROR_SYM_DESC(1, "OnMessageGroupPacket",
			                      LT_INVALIDSERVERPACKET, "invalid packet")
		}

		if (nLen == 0)
			return LT_OK;

		pSub->Init(nLen, 0x1000);

		pPacket->ReadData(pSub->m_Data.m_pArray, nLen);
		pSub->m_DataLen = nLen;
		pSub->m_Pos     = 1;

		{
			uint8* pID = &nID;
			*pID = pSub->m_Data[0] & 0x3f;
		}

		pHandler = (ClientPacketHandlerFn)g_Table4b48d0Q474[nID];
		if (pHandler)
		{
			dResult = pHandler((CClientShell*)pShell, (ILTMessageView*)pSub);
			if (dResult != LT_OK)
				return dResult;
		}
	}

	return LT_OK;
}


// ---- what the object-removal handler needs ----------------------------- //

// The client's packet-trace console int at 0x004b32dc.  Three comparisons in
// this run fix it as a plain `int`, not a flag word: `cmp $0x2` and then
// `cmp $0x1` in the change-flag trace at 0x00479160, and `cmpl $0x1` at
// 0x0047a158.  It is the "DebugPackets" binding (lithtech.exe's table entry
// 0x004aa2b0), defined under its server name in src/engine/server/dsi.cpp.
extern int g_CV_DebugPackets;

// One row of the client's object table.  0x00420550 is
//   `nID < pClientMgr->[0x11e0] ? &pClientMgr->[0x11dc][nID] : NULL`
// with `lea (%ecx,%eax,8),%eax`, so the row is EIGHT bytes: a type byte at
// 0 (the only value this run compares against is 1) and a pointer at 4.
class CObjRecQ478
{
public:
	uint8  m_Type;                           // 0x00
	uint8  m_pad_01[0x04 - 0x01];
	void*  m_pObject;                        // 0x04
};

// 0x00420550 — cdecl.  The id parameter is uint16: the callee opens
// `mov 0x8(%esp),%eax ; and $0xffff,%eax`, and the caller at 0x0047a177
// pushes the whole register with a stale upper half, which is only correct
// if the declared parameter is sixteen bits wide.  (name ours)
CObjRecQ478* obj_GetRecord(CClientMgrQ478* pClientMgr, uint16 nID);

// 0x00411bf0 — cdecl, the manager first and the object second (the callee
// takes `mov 0xc(%esp),%esi` for the object before it ever reaches
// `mov 0x18(%esp),%edi` for the manager).  Nothing names it.
void sub_411bf0(CClientMgrQ478* pClientMgr, void* pObject);

// CSoundInstance, as this one call site reaches it.  Everything here is
// already in src/engine/lithtech/clientsoundinst.cpp: m_StateFlags at 0x40
// with bit 0x40, and sub_47d690 a NON-virtual member ("give the client
// manager's table slot back") reached by direct call.
class CSoundInstQ478
{
public:
	uint8    m_pad_00[0x40];
	uint32   m_StateFlags;                   // 0x40
	LTRESULT sub_47d690();                   // 0x0047d690
};

// src/engine/lithtech/clientsoundmgr.cpp:290 already declares this slot.
class CSoundMgrQ478
{
public:
	LTRESULT ReleaseSound(CSoundInstQ478* pSound);           // 0x00480bd0
};

// 0x0040f7f0 — `g_pClientMgr + 0x774`, no arguments (binkvideo.cpp,
// soundinst2.cpp, clientwin.cpp all reach the same body).
CSoundMgrQ478* ci_GetSoundMgr();


// ======================================================================= //
//  3.  0x0047a0d0 — the object-REMOVE list                  274 bytes
//
//  Not a dispatch-table entry: its only caller is 0x00478832, inside the
//  update-packet reader at 0x00478510.  Still __cdecl(shell, packet)
//  returning LTRESULT, like every handler in the run.
//
//  WHAT PINS IT
//   * `mov 0x78(%eax),%ebx` before anything else and %ebx untouched for the
//     whole body: the manager is hoisted into a local once, not re-read.
//   * the loop test is the module's standard one, `(int)(m_DataLen -
//     m_Pos) > 0` with both members zero-extended to 32 bits and a SIGNED
//     `jle`, and it is duplicated at the bottom (0x0047a1b7) with the entry
//     copy at the top — a `while`, not a `do`.
//   * `lea 0x2(%ecx),%edi` inside it: ReadType<uint16>.  The dead-slot rule
//     does not apply — the cursor is the packet's own member and the
//     inline advances it.
//   * `cmpl $0x1,0x004b32dc ; jne` round the con_Print: an `== 1` test, and
//     the printer is the plain white one (0x0041b8c0), not the coloured one
//     block 1 uses.
//   * `push %ebp` hands 0x00420550 the raw sixteen-bit register; the same
//     value goes to con_Print through `and $0xffff,%edx`, which is the
//     varargs promotion.  Two different treatments of one local is what
//     says 0x00420550's parameter is declared uint16.
//   * TWO `je 0x0047a1b7` in a row (record, then record->m_pObject) landing
//     on the loop's continue: one `&&`.
//   * `cmpb $0x1,(%eax) ; jne` — the fall-through arm is the `== 1` body
//     (arm ordering), so the `if` is positive.
//   * `testb $0x40,0x40(%ecx) ; je` — again positive, with the else arm
//     being ci_GetSoundMgr()->ReleaseSound.
//   * `push %ecx ; call 0x0040f7f0 ; mov %eax,%ecx ; call 0x00480bd0` is
//     ONE expression: 0x0040f7f0 takes no arguments, so the push belongs to
//     the `ret $4` thiscall that follows it.
//   * two epilogue copies, but they differ only in whether %ebp is popped
//     (it is pushed inside the entry test), so there is ONE `return LT_OK`.
//
//  CONFIDENCE in byte identity: MEDIUM.  The control flow is fully pinned;
//  what is not is whether `pRec->m_pObject` is written out three times (as
//  here) or hoisted into a local in the original — VC6 keeps it in %ecx
//  either way, so the bytes should not care, but that is an assumption.
// ======================================================================= //

LTRESULT sub_47a0d0(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	CClientMgrQ478* pClientMgr = pShell->m_pClientMgr;

	while (pPacket->m_DataLen - pPacket->m_Pos > 0)
	{
		uint16       nID = pPacket->ReadType((uint16*)0);
		CObjRecQ478* pRec;

		if (g_CV_DebugPackets == 1)
			con_Print("Remove id %d", nID);          // 0x004ae600

		pRec = obj_GetRecord(pClientMgr, nID);

		if (pRec && pRec->m_pObject)
		{
			if (pRec->m_Type == 1)
			{
				sub_411bf0(pClientMgr, pRec->m_pObject);
			}
			else if (((CSoundInstQ478*)pRec->m_pObject)->m_StateFlags & 0x40)
			{
				((CSoundInstQ478*)pRec->m_pObject)->sub_47d690();
			}
			else
			{
				ci_GetSoundMgr()->ReleaseSound(
					(CSoundInstQ478*)pRec->m_pObject);
			}
		}
	}

	return LT_OK;
}


// ======================================================================= //
//  0x0047a480 -- message id 13, 96 bytes
//
//  0x004b4904 in the dispatch table.  The message id is the packet's LAST
//  byte, taken off the end (CPacketQ478::PopByteQ478 above, inlined); the
//  byte is homed in argument two's slot, as the ReadType temporaries are
//  elsewhere in this run.  Then CClientMgr::sub_411440(pPacket) and the game
//  shell's OnMessage(id, read handle at packet+8).  m_pClientMgr is re-read
//  after the call (0x0047a4c4), so it is named twice, not cached.  No null
//  test on the shell.
//
//  NEAR, 96/96 and 35/35: scheduling only.  Shipped loads the message id
//  out of its slot AFTER `push packet+8` and the shell's `this`; ours loads
//  it first.  Unmoved by: packet+8 as a named local, the game shell
//  as a named local, a CClientMgr local after the call, `&m_pad_00[8]`,
//  and the pop written in place instead of PopByteQ478.
// ======================================================================= //

LTRESULT sub_47a480(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	uint8 nMsgID = pPacket->PopByteQ478();

	pShell->m_pClientMgr->sub_411440(pPacket);
	pShell->m_pClientMgr->m_pClientShell->OnMessage(nMsgID,
		(uint8*)pPacket + 8);

	return LT_OK;
}


// 0x0041b0f0 -- clientconsole.cpp: src/engine/server/console.cpp's
// con_SetVar(ConsoleState*, char* pName, char* pVal), compiled into the
// client.
void con_SetVar(void* pState, char* pName, char* pVal);

// ======================================================================= //
//  0x0047a790 -- message id 16, 51 bytes: a console variable set by the
//  server.  Two string reads IN SOURCE ORDER (the first result goes to
//  %edi and is pushed as argument two), so two locals -- argument
//  expressions would evaluate right to left.  clientsoundinst.cpp's banner
//  predicted exactly this body.
// ======================================================================= //

LTRESULT sub_47a790(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	char* pName = pPacket->ReadStringQ478();
	char* pVal  = pPacket->ReadStringQ478();

	con_SetVar(pShell->m_pClientMgr->m_ConsoleState, pName, pVal);

	return LT_OK;
}


// ======================================================================= //
//  4.  0x0047a7d0 — OnSkyDef, message id 17                 403 bytes
//
//  ONE function, not two: 0x0047a7d0..0x0047a963, ending in thirteen
//  alignment `nop`s.  It has two `ret`s and both are this
//  function's — the error return at 0x0047a8a5 and the success return at
//  0x0047a962.  0x004b4914 in the dispatch table gives it id 17.
//  The name is its own check literal (0x004ae610).
//
//  WHAT PINS IT
//   * `mov $0x1,%ebx` at 0x0047a7f7, hoisted above everything, serves all
//     THREE uses of the constant 1 in the function: the inlined ReadType's
//     `dsi_OnReturnError(LT_ERROR)`, both `g_DebugLevel >= 1` compares
//     (`cmp %ebx,%eax`), and the function's own `return LT_ERROR`
//     (`mov %ebx,%eax`).  That is what says this handler returns LT_ERROR
//     and not the LT_INVALIDSERVERPACKET it hands SetupError.
//   * the error report pushes "OnSkyDef" as an immediate (a literal, not a
//     char* variable), reports the code SYMBOLICALLY ("LT_ERROR",
//     0x004a7360) and passes a real description ("invalid packet",
//     0x004ae4f4).  de_error.h has exactly one macro with that combination:
//     RETURN_ERROR_SYM_DESC.
//   * `cmp $0x1e,%dx ; jbe` past the error — the guard is `> 30`, and 30 is
//     independently confirmed by the `mov $0xf,%ecx ; rep stos` that fills
//     sixty bytes.
//   * `or $0xffffffff,%eax ; rep stos %eax` is VC6's inline memset of a
//     constant size with a constant fill.
//   * `test %dx,%dx ; jbe` is a zero-trip check on an unsigned count, and
//     the body counts DOWN (`dec %eax ; jne`) while the address counts UP
//     (`add $0x2,%edi`), which is what a `for (i = 0; i < n; i++)` over a
//     uint16 array becomes when `i` has no other use.
//   * `mov 0x78(%ecx),%edx` is INSIDE the loop and 0x78 is re-read at
//     0x0047a7e0, 0x0047a864, 0x0047a8a6 and 0x0047a94a — four reads, so
//     the manager is NOT hoisted into a local here (contrast block 3, where
//     one `mov 0x78(%eax),%ebx` covers the whole body).  An
//     expression repeated rather than cached is source-reachable, and this
//     one is repeated.
//   * the bulk read is first, before the count: `push $0x30 ; push
//     <mgr+0x11a0> ; call 0x0045c2d0` with no caller cleanup.
//
//  CONFIDENCE in byte identity: MEDIUM.  Control flow, constants and the
//  repeated-vs-cached question are all read off the bytes; the risks are
//  the exact macro (RETURN_ERROR_SYM_DESC vs a hand-written equivalent) and
//  whether the fill is a memset call or an array loop VC6 rewrote.
// ======================================================================= //

LTRESULT sub_47a7d0(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	uint16 nCount;
	uint16 i;

	pPacket->ReadData(pShell->m_pClientMgr->m_SkyDef, 0x30);

	nCount = pPacket->ReadType((uint16*)0);

	if (nCount > 30)
	{
		pShell->m_pClientMgr->SetupError(LT_INVALIDSERVERPACKET);

		RETURN_ERROR_SYM_DESC(1, "OnSkyDef", LT_ERROR, "invalid packet");
	}

	memset(pShell->m_pClientMgr->m_SkyObjects, 0xff,
	       sizeof(pShell->m_pClientMgr->m_SkyObjects));

	for (i = 0; i < nCount; i++)
		pShell->m_pClientMgr->m_SkyObjects[i] = pPacket->ReadType((uint16*)0);

	return LT_OK;
}


// ======================================================================= //
//  0x0047a970 -- message id 26 (dispatch slot 0x004b4938), 707 bytes
//  (0x0047a970..0x0047ac33, single `ret`, then thirteen alignment nops).
//  The global-light packet (name ours): two vectors and a scalar.
//
//  WHAT PINS IT
//   * `mov 0x78(%eax),%ecx ; mov 0x4c0(%ecx),%edi` -- the client manager's
//     interface pointer, cached in %edi for the whole function.
//   * `call *0x13c(%edx)` and `call *0x144(%edx)` through its vtable, each
//     with twelve bytes of vector pushed BY VALUE (`sub $0xc,%esp`, three
//     dword copies): slots 79 and 81, SetGlobalLightDir(LTVector) and
//     SetGlobalLightColor(LTVector) (cltnew.cpp / cltrest.cpp).
//   * `call *0x248(%edi)` with one pointer and caller cleanup is the
//     table's function-pointer MEMBER SetGlobalLightScale(LTVector*)
//     (clientinterface.h 0x248), handed the vector whose three components
//     were set from one read (`fsts` x2, `fstps`).
//   * seven float reads: the first two CALL the out-of-line ReadType<float>
//     (0x0047bc50, `push $0` dummy) and the other five are packet.h's
//     expansion -- /O2's inline budget spent from the end of the function
//     backwards.  Written as ReadType throughout, all seven expand (905
//     bytes), so the first two are spelled ReadTypeOOL like the rest of
//     this file's calling sites.
//   * the scale is ONE read stored x, y, z in that order (`fsts 0x10`,
//     `fsts 0x14`, `fstps 0x18`): `z = y = x = read`.  The other chaining
//     order stores z first.
// ======================================================================= //
LTRESULT sub_47a970(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	ILTClient* pClientDE = pShell->m_pClientMgr->m_pClientDE;
	LTVector   vVec;

	vVec.x = pPacket->ReadTypeOOL((float*)0);
	vVec.y = pPacket->ReadTypeOOL((float*)0);
	vVec.z = pPacket->ReadType((float*)0);
	pClientDE->SetGlobalLightDir(vVec);

	vVec.x = pPacket->ReadType((float*)0);
	vVec.y = pPacket->ReadType((float*)0);
	vVec.z = pPacket->ReadType((float*)0);
	pClientDE->SetGlobalLightColor(vVec);

	vVec.z = vVec.y = vVec.x = pPacket->ReadType((float*)0);
	pClientDE->SetGlobalLightScale(&vVec);

	return LT_OK;
}


// ---- what the two thread-load handlers need ---------------------------- //

// src/engine/lithtech/clientmgr.h's CClientFileRef, repeated here as a view.
// The default constructor is the whole reason it is a class and not a plain
// struct: the two handlers below open with THREE stores of zero -- dword,
// dword, then a sixteen-bit store -- in exactly this order and before
// anything else in the function, which is a constructor running on a local.
class CClientFileRefQ478
{
public:
	CClientFileRefQ478()
	{
		m_nFileType = 0;
		m_pFilename = LTNULL;
		m_nLocalID  = 0;
	}

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// The file-type selector.  src/engine/lithtech/clientwin.cpp's
// reconstruction of 0x00404680 says it in as many words: "an int type at
// +0x00 (2 selects the id path), a char* at +0x04 and a uint16 at +0x08".
// Both handlers below set the type to 2 and fill in the id, which is the id
// path.  The immediate is shared with PACKETFLAG_READOVERFLOW in the shipped
// code (one `mov $0x2,%edi` serves both), so the 2 is certainly a literal.
#define CFILEREF_BYID 2

// The record 0x00404680 hands back.  clientwin.cpp calls it CluFileRec and
// lists six fields, none of them at +0x00; this run only ever reads +0x00,
// and what comes out of it is handed to 0x00477830 which compares it against
// CClientModel::m_pModel (clientmgr.h, +0x1dc).  So +0x00 is the loaded
// model.  (name ours; the field name is a guess, the OFFSET is not)
class CFileRecQ478
{
public:
	CModelQ478* m_pModel;                    // 0x00
};

// The loaded model.  Only one thing is known about it here and it is
// enough: 0x0047b667 is `test %esi,%esi ; mov (%esi),%edx ; push $0x1 ;
// mov %esi,%ecx ; call *(%edx)` -- MSVC's expansion of `delete p` when the
// destructor is virtual and lives in slot 0 (the 1 is the deleting flag).
// Declared and not defined, as clientshell.cpp does for CClientShell's.
class CModelQ478
{
public:
	virtual ~CModelQ478();                   // slot 0
};

// The client's file manager.  Nothing here looks inside it.
class CFileMgrQ478
{
public:
	uint8 m_pad_00[0x04];
};

// 0x00404680 — clientwin.cpp reconstructed it as clu_fm_GetFileRec(pMgr,
// pIdent, nFlag) with the flag a BYTE whose 0xff is a wildcard; both sites
// here pass 1.  cdecl.
CFileRecQ478* fm_GetFileRec(CFileMgrQ478* pMgr, CClientFileRefQ478* pRef,
                            uint8 nFlag);

// 0x00477830 — cdecl(manager, model, record).  It walks the manager's list
// at +0x420 and drops every object whose CClientModel::m_pModel (+0x1dc)
// is this model, so argument two really is the model.  (name by address)
void sub_477830(CClientMgrQ478* pClientMgr, CModelQ478* pModel,
                CFileRecQ478* pRec);

// 0x004770e0 — cdecl, six arguments, 0x0047b4b7's callee.  Its body is
//   *ppRec = fm_GetFileRec(pClientMgr->m_pFileMgr, pRef, 1);
//   if (!*ppRec) ...error 0x16...
//   if ((*ppRec)->m_pModel) { *ppModel = (*ppRec)->m_pModel; return LT_OK; }
//   if (!a5) return 0x54;
//   return sub_477020(pClientMgr, *ppRec, ppModel, a6);
// which is what types arguments three and four.  (name by address)
LTRESULT sub_4770e0(CClientMgrQ478* pClientMgr, CClientFileRefQ478* pRef,
                    CModelQ478** ppModel, CFileRecQ478** ppRec,
                    int bLoad, int nFlags);

// 0x0041ff90 — cdecl, two arguments, and it forwards both plus the address
// of its own second argument slot to 0x0041ff50.  (name by address)
void sub_41ff90(CClientMgrQ478* pClientMgr, CClientFileRefQ478* pRef);

// 0x00462900 — clientmgr2.cpp already names it and warns it is NOT dfree.
void tex_FreeTextureData(void* pTexture);


// ======================================================================= //
//  5.  0x0047b380 — "OnThreadLoadPacket", message id 23     415 bytes
//
//  ONE function, 0x0047b380..0x0047b51f.  Its three `ret`s are
//  its three separate returns (0x0047b4c4, 0x0047b4e3, 0x0047b51e); nothing
//  branches in from outside and the row ends on `ret` + one `nop`.
//  0x004b492c in the dispatch table gives id 23.
//
//  WHAT PINS IT
//   * `movl $0x0 ; movl $0x0 ; movw $0x0` into three adjacent slots before
//     any packet work: CClientFileRef's default constructor on a local, in
//     declaration order.
//   * `mov $0x2,%edi` is issued in BOTH arms of the first inlined ReadType
//     (0x0047b3c4 and 0x0047b3d9) because it is PACKETFLAG_READOVERFLOW
//     there; the SAME register is then stored into the ref's first field at
//     0x0047b41e.  One constant, two uses — which is why the file type is
//     certainly the literal 2 and not something computed.
//   * that store lands AFTER the uint8 read and BEFORE the uint16 read, so
//     the assignment sits between the two reads in the source.
//   * THREE distinct epilogues with three different %eax values:
//     several duplicated exits are separate `return`s, not one
//     shared tail.
//   * `test %al,%al ; jne` then `cmp $0x2,%al ; jne`: the fall-through arm
//     of each is its body, so these are `if` / `if`, branching AROUND the
//     case.  A `switch` would branch INTO it.
//   * the error tail pushes "OnThreadLoadPacket" as an immediate, the code
//     as "44" (0x004ae528, verified in .data) and " " (0x004a7164) as the
//     description — de_error.h's plain RETURN_ERROR, with the name a string
//     literal rather than a char* variable.
//   * two four-byte locals are passed BY ADDRESS to 0x004770e0 and never
//     read afterwards; 0x004770e0 writes both.
//
//  CONFIDENCE in byte identity: MEDIUM-LOW.  Everything above is read off
//  the bytes, but the two write-only locals are pure plumbing — their
//  declaration order and whether the original really discards
//  0x004770e0's result are not decidable from this function alone.
// ======================================================================= //

LTRESULT sub_47b380(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	CClientFileRefQ478 cRef;
	CModelQ478*        pModel;
	CFileRecQ478*      pRec;
	uint8              nType;

	nType = pPacket->ReadType((uint8*)0);

	cRef.m_nFileType = CFILEREF_BYID;
	cRef.m_nLocalID  = pPacket->ReadType((uint16*)0);

	if (nType == 0)
	{
		sub_4770e0(pShell->m_pClientMgr, &cRef, &pModel, &pRec, 1, 0);

		return LT_OK;
	}

	if (nType == 2)
	{
		sub_41ff90(pShell->m_pClientMgr, &cRef);

		return LT_OK;
	}

	RETURN_ERROR(1, "OnThreadLoadPacket", LT_INVALIDSERVERPACKET);
}


// ======================================================================= //
//  6.  0x0047b520 — "OnThreadLoadPacket", message id 24     468 bytes
//
//  ONE function, 0x0047b520..0x0047b6f3, and the twin of block 5: same
//  constructor, same two reads, same 2 into m_nFileType, same three-way
//  selector on the type byte and the same RETURN_ERROR tail with the SAME
//  literal.  Where block 5 asks 0x004770e0 to load, this one goes straight
//  at the record and frees — it is the unload half.  0x004b4930 gives id 24.
//
//  WHAT PINS THE TWO ARMS
//   * arm 0: `test %eax,%eax ; je` and `mov (%eax),%esi ; test %esi,%esi ;
//     je` both jumping to the SAME continue — one `&&`.  Then 0x00477830,
//     and then `test %esi,%esi ; mov (%esi),%edx ; push $0x1 ; mov %esi,%ecx
//     ; call *(%edx)`.  That last group is not an `if` the source wrote: it
//     is `delete pModel;` with a virtual destructor in slot 0 — MSVC always
//     emits the null test, which is exactly why the pointer is tested a
//     second time when it is already known non-null.
//   * arm 2: the same `&&`, then a plain cdecl call to 0x00462900
//     (tex_FreeTextureData), so the same +0x00 field is a texture there.
//     The field is read through the record both times, so the record type
//     is one thing whose +0x00 is reinterpreted by the caller — which is
//     why m_pModel is used for both and the texture path casts.
//   * three separate epilogues again: three `return`s.
//
//  IDENTICAL on lithtech.exe (468/468).
// ======================================================================= //

LTRESULT sub_47b520(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	CClientFileRefQ478 cRef;
	CFileRecQ478*      pRec;
	uint8              nType;

	nType = pPacket->ReadType((uint8*)0);

	cRef.m_nFileType = CFILEREF_BYID;
	cRef.m_nLocalID  = pPacket->ReadType((uint16*)0);

	if (nType == 0)
	{
		pRec = fm_GetFileRec(pShell->m_pClientMgr->m_pFileMgr, &cRef, 1);

		// IDENTICAL.  One `&&` on the record and its +0x00 field, THEN the
		// field copied into a local inside the body.  The copy CSEs with
		// the tested load (shipped keeps the model, not pRec, in %esi and
		// never reloads it after sub_477830), but VC6 does not carry the
		// non-null fact from the memory test to the local, so `delete`
		// keeps its own null test at 0x0047b663 -- the four bytes a
		// nested `if (pModel)` around the delete removed.
		if (pRec && pRec->m_pModel)
		{
			CModelQ478* pModel = pRec->m_pModel;

			sub_477830(pShell->m_pClientMgr, pModel, pRec);

			delete pModel;
		}

		return LT_OK;
	}

	if (nType == 2)
	{
		pRec = fm_GetFileRec(pShell->m_pClientMgr->m_pFileMgr, &cRef, 1);

		if (pRec && pRec->m_pModel)
			tex_FreeTextureData(pRec->m_pModel);

		return LT_OK;
	}

	RETURN_ERROR(1, "OnThreadLoadPacket", LT_INVALIDSERVERPACKET);
}


// ======================================================================= //
//  7.  0x0047b2b0 — OnNetProtocolVersionPacket, id 4        199 bytes
//
//  NOT one of this run's rows by address, but it is the SECOND function
//  inside the 1,110-byte 0x0047af20 row (its extent splits at
//  0x0047b2b0, and 0x004b48e0 in the dispatch table confirms it is an entry
//  in its own right, id 4).  It is small and completely determined, so it
//  is written rather than left.
//
//  WHAT PINS IT
//   * `lea 0x4(%ecx),%esi`: ReadType<uint32>.
//   * 0x0041fc60 is reached with FOUR dwords and the caller cleans:
//     (this, 0x4e, 6, nVersion).  clientrender.cpp/clientshell.cpp already
//     have 0x0041fc60 as CClientMgr::SetupError(LTRESULT, ...) — a __cdecl
//     varargs member — so the two trailing dwords are the format's
//     arguments, "expected 6, got n".
//   * 0x4e is 78 and ltcodes.h has LT_INVALIDNETVERSION = 78; the reported
//     code string is "78" (0x004ae64c, verified in .data), which is
//     de_error.h's plain RETURN_ERROR and not one of the symbol-reporting
//     forms.  The description is " " (0x004a7164).
//   * `cmp $0x6,%ecx ; jne <error>` — the jump is the INVERSE of the source
//     condition, so the condition is `== 6` and its body (the fall-through)
//     is the `return LT_OK`.  Written that way below.  The alternative,
//     `if (nVersion != 6) { ... } return LT_OK;`, would put `je` here.
//     This is the one judgement call in the block.
//
//  CONFIDENCE in byte identity: MEDIUM-HIGH.
// ======================================================================= //

LTRESULT sub_47b2b0(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	uint32 nVersion = pPacket->ReadType((uint32*)0);

	if (nVersion == 6)
		return LT_OK;

	// 6 is a bare immediate at 0x0047b324 and again at 0x0047b332, so the
	// version the client insists on is literally 6.
	pShell->m_pClientMgr->SetupError(LT_INVALIDNETVERSION, 6, nVersion);

	RETURN_ERROR(1, "OnNetProtocolVersionPacket", LT_INVALIDNETVERSION);
}


// ---- what OnPortalFlagsPacket reaches ---------------------------------- //

// One 36-byte record in the array at <world data>+0xb0.  36 comes straight
// off the addressing: `lea (%ebx,%ebx,8),%ecx` is index*9 and the store is
// `mov %dx,0x6(%eax,%ecx,4)`, i.e. base + index*36 + 6.  Only the uint16 at
// +6 is touched here.  (names ours)
class CPortalRecQ478
{
public:
	uint8  m_pad_00[0x06];
	uint16 m_nFlags;                         // 0x06
	uint8  m_pad_08[0x24 - 0x08];
};

// The block hanging off a world node at +0x04.  The record array is at
// 0xb0 and its count at 0xb4 -- the count is compared with an UNSIGNED
// `jae`, so it is a uint32 and the index is being range-checked.
class CWorldDataQ478
{
public:
	uint8           m_pad_00[0xb0];
	CPortalRecQ478* m_pPortalRecs;           // 0xb0
	uint32          m_nPortalRecs;           // 0xb4
};

class CWorldNodeQ478
{
public:
	uint8            m_pad_00[0x04];
	CWorldDataQ478*  m_pData;                // 0x04
};


// ======================================================================= //
//  8.  0x0047ad20 — OnPortalFlagsPacket, message id 19      500 bytes
//
//  ONE function, 0x0047ad20..0x0047af13.  The `ret` at 0x0047aec9 that
//  suggested a start at 0x0047aeca is the
//  SUCCESS return; 0x0047aeca is the cold error block, reached by two `jae`
//  from inside the body, and it ends at 0x0047af13 with twelve alignment
//  `nop`s.  0x004b491c gives id 19.  The name is its own literal
//  (0x004ae61c).
//
//  WHAT PINS IT
//   * the first instruction is `mov 0x4(%esp),%ecx ; call 0x00414b10` —
//     CClientShell::GetClientMgr, a real __thiscall getter
//     (clientmgr2.cpp).  The ERROR path, by contrast, reads the same
//     pointer as a FIELD (`mov 0x78(%ecx),%edx`).  Both spellings are in
//     one function and the bytes distinguish them, so both are written out
//     as they appear; do not "tidy" either into the other.
//   * three inlined reads in order: uint16, uint16, uint8, and the uint8
//     one is read only AFTER both bounds checks have passed, so it is
//     inside the nesting.
//   * BOTH bounds checks are `jae` to the SAME block.
//     One shared exit with several producers is nested POSITIVE guards
//     with a single trailing tail — hence `<` and not `>=` below.
//   * `mov 0x4(%edi),%edx` at 0x0047ae32 and `mov 0x4(%edi),%eax` again at
//     0x0047aeac: the node's +0x04 is read TWICE, so the source names it
//     twice rather than caching it.
//   * `movzbw 0x14(%esp),%dx` then a sixteen-bit store: an eight-bit value
//     assigned into a uint16 member.
//   * the error tail is SetupError(LT_INVALIDSERVERPACKET) followed by
//     de_error.h's plain RETURN_ERROR — the code string is "44"
//     (0x004ae528, verified) and the description " " (0x004a7164).
//
//  IDENTICAL.  The node and record indices are uint32 locals: the
//  shipped code zero-extends each uint16 read into a register the moment
//  it is read (`and $0xffff ; mov %eax,%edi`); uint16 locals spilled both
//  to the stack and gave 508 B.
// ======================================================================= //

LTRESULT sub_47ad20(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	CClientMgrQ478* pClientMgr = pShell->GetClientMgr();
	uint32          nNode;                   // uint32, not uint16 -- see above
	uint32          nRec;

	nNode = pPacket->ReadType((uint16*)0);
	nRec  = pPacket->ReadType((uint16*)0);

	if (nNode < pClientMgr->m_nWorldNodes)
	{
		CWorldNodeQ478* pNode = pClientMgr->m_pWorldNodes[nNode];

		if (nRec < pNode->m_pData->m_nPortalRecs)
		{
			pNode->m_pData->m_pPortalRecs[nRec].m_nFlags =
				pPacket->ReadType((uint8*)0);

			return LT_OK;
		}
	}

	pShell->m_pClientMgr->SetupError(LT_INVALIDSERVERPACKET);

	RETURN_ERROR(1, "OnPortalFlagsPacket", LT_INVALIDSERVERPACKET);
}


// *********************************************************************** //
//
//  PER-ROW STATUS FOR THE 0x00478000..0x0047c000 ENUMERATION
//
//  WRITTEN ABOVE (nine bodies, eight of them from the enumerated rows):
//
//    0x004780b0  283 B  id 12  sub_4780b0        MEDIUM-HIGH
//    0x004781d0  298 B  id  6  sub_4781d0        IDENTICAL
//    0x00478300   22 B  id  5  sub_478300        HIGH        (bonus)
//    0x0047a0d0  274 B         sub_47a0d0        MEDIUM
//    0x0047a7d0  403 B  id 17  OnSkyDef          MEDIUM
//    0x0047ad20  500 B  id 19  OnPortalFlagsPacket   MEDIUM
//    0x0047b2b0  199 B  id  4  OnNetProtocolVersionPacket  MEDIUM-HIGH (bonus)
//    0x0047b380  415 B  id 23  OnThreadLoadPacket    MEDIUM-LOW
//    0x0047b520  468 B  id 24  OnThreadLoadPacket    MEDIUM-LOW
//
//  Every one of them issues the shipped call sequence by construction; the
//  counts were taken from the shipped image and each body was written
//  against them:
//    0x004780b0  5 direct / 0 indirect      0x0047a7d0   8 / 0
//    0x004781d0  6 / 3                      0x0047ad20  10 / 0
//    0x00478300  1 / 0                      0x0047b2b0   5 / 0
//    0x0047a0d0  8 / 0                      0x0047b380   8 / 0
//                                           0x0047b520  10 / 1
//  The counts above are the shipped side only.
//
//  ROWS THAT ARE NOT FUNCTIONS
//
//    0x004780b0 / 586   TWO functions.  0x004780b0..0x004781cb (283 B, id
//                       12) then four alignment `nop`s then
//                       0x004781d0..0x004782f9 (298 B, id 6).  Both are
//                       written above.
//    0x00478320 / 1314  THREE functions at least.  The real extent of the
//                       first is 0x00478320..0x0047850a (491 B, id 14,
//                       "OnMessageGroupPacket"); five alignment `nop`s;
//                       then 0x00478510, whose body runs PAST the end of
//                       the row (its branches reach 0x00478917/0x00478919
//                       and 0x004788ca).
//    0x00478842 / 226   NOT A FUNCTION.  It is an interior block of
//                       0x00478510: 0x004787b4, 0x004787f6 and 0x00478826
//                       all `j..` to 0x00478842 from above it, and
//                       0x00478858 branches OUT to 0x00478578.  The
//                       containing function is 0x00478510..0x00478924
//                       (1,044 B, id 8) and it carries the "OnUpdatePacket"
//                       and "ReadObjectSubPacket" literals.
//    0x0047af20 / 1110  TWO functions.  0x0047af20..0x0047b2ac (id 21) and
//                       0x0047b2b0..0x0047b375 (199 B, id 4,
//                       OnNetProtocolVersionPacket, written above).
//
//  ROWS WHOSE EXTENT IS CONFIRMED CORRECT (the suspect entries are false
//  alarms -- a second epilogue copy, or a cold error block, not a second
//  function):
//    0x00478a50 / 240   one function; 0x00478b2f is the `else` tail
//                       0x00478b1b jumps to.
//    0x0047ad20 / 500   one function; 0x0047aeca is the cold error block
//                       two `jae` inside the body reach.
//    0x0047b380 / 415   one function, three separate returns.
//    0x0047b520 / 468   one function, three separate returns.
//    0x0047a7d0 / 403   one function; the two `ret`s are its error and
//                       success returns.
//    0x004792c0, 0x00479500, 0x004798a0, 0x00479bf0, 0x0047a1f0,
//    0x0047a970, 0x0047b700  -- single `ret`, no branch leaving the range.
//
//  NOT WRITTEN, AND WHY
//
//    0x00478320  WRITTEN since (identical) -- see its banner.  The
//                original note:  OnMessageGroupPacket (id 14), 491 B.
//                Decoded but NOT written: after `msg_StartWrite` it resets the message in
//                place (m_MaxDataLen = nLen, m_DataLen = m_Pos = m_3a = 1,
//                m_Flags = 0, m_10 = this) and then runs FOUR inlined
//                CMoArray calls on m_Data -- 0x0040bfe0 twice with (0,
//                &g_PacketDataBank), then 0x00433570(0x1000), then
//                0x0040bfe0(nLen, &g_PacketDataBank).  src/engine/server/
//                collection.h has those as SetSize2 and SetCacheSize, but
//                nothing in the bytes says whether the two identical
//                SetSize2(0, ...) calls are one Term() plus one explicit
//                reset or something else, and guessing there would put the
//                whole function out.  The rest of it IS pinned: a
//                `while (m_DataLen - m_Pos > 0)` loop, a length byte read
//                through 0x00459f80, `if (nLen > m_DataLen - m_Pos)` ->
//                RETURN_ERROR_SYM_DESC(1, "OnMessageGroupPacket",
//                LT_INVALIDSERVERPACKET, "invalid packet") after
//                SetupError, `if (nLen == 0)` -> LT_OK, a bulk
//                0x0045c2d0 read into the sub-message, and then a dispatch
//                through the SAME table this file documents:
//                `m_Data.m_pData[0] & 0x3f` indexes 0x004b48d0 and the
//                handler is called with (pShell, pSubMsg).
//    0x00478510  id 8, 1,044 B ("OnUpdatePacket" / "ReadObjectSubPacket").
//                The centre of the module and too large to take on
//                here.
//    0x00478842  interior block -- see above.
//    0x00478a50  WRITTEN since (identical) -- see its banner below.
//                240 B, one function, seven arguments, called only from
//                0x00478a13 inside 0x004789d0.  It is the object-update
//                applier (position/rotation flags at +0x18, a 0x80808081
//                magic-number divide by 254 at the end).  Decodable but
//                needs 0x004789d0's argument roles first.
//    0x004792c0  WRITTEN since (identical): the anim-tracker list
//                decoder -- see its banner below.
//    0x00479bf0  WRITTEN since (identical): the sound update arm.
//    0x00479500 / 0x004798a0 / 0x00479d50
//                926 / 836 / 893 B, all single-`ret`, all called from
//                inside 0x00478510.  0x004798a0 and 0x00479d50 WRITTEN
//                since (identical) -- the two sound unpackers, see
//                their banners below; 0x00479500 WRITTEN since
//                (identical): the object create-info unpacker.
//    0x0047a1f0  WRITTEN since (identical): id 10.
//    0x0047a4e0  WRITTEN since (identical): id 15.
//    0x0047a970  WRITTEN since (identical): id 26, the global-light
//                packet -- see its banner below.
//    0x0047af20  id 21, 0x0047af20..0x0047b2ac.  Not reached (its second
//                half, 0x0047b2b0, IS written).
//    0x0047b700  933 B, id 25, single `ret`.  Not reached.
//    0x0047bef0  WRITTEN since (identical): smgr.cpp's
//                CServerThread::IsRequestQueued -- see its banner below.
//    0x0047bef0  175 B and 0x0047bfa0 127 B.  These two are NOT client
//                packet handlers: they sit in the 0x0047bc50..0x0047c62x
//                block that src/engine/lithtech/clientsoundinst.cpp
//                identifies as the SERVER engine compiled into
//                lithtech.exe.  Both are __thiscall `ret $0x4` methods on
//                an object with two collections (+0x04 and +0x2c, both
//                using 0x00444180/0x00444190/0x004442c0) and a server
//                manager at +0x60 whose +0xa2c is servermgr.h's CFileMgr;
//                0x0047bfa0 is slot 3 of the four-slot vtable at
//                0x004a341c, whose slots 1 and 2 (0x004445c0, 0x00444760)
//                are shared with another class.  NEITHER appears in
//                the retail server.dll -- both prologues were searched for
//                byte for byte -- so they are not prebuilt-library code and
//                they do belong to lithtech.exe; they simply belong under
//                src/engine/server/, not in this file.
//
// *********************************************************************** //


// ======================================================================= //
//  0x00478b40 — the object-update packet reader, 1557 bytes.
//
//  THE ROW THE ENUMERATOR SPLIT. The function enumeration carried this as
//  two rows, "0x00478b40, 10" and "0x00478b4a, 1547", and the second is not a
//  function: it is the `push ebp` TEN BYTES into this prologue. The
//  enumerator scanned for 0xC3 at every byte offset rather than at
//  instruction boundaries, and 0x00478b48 is `8b c3` (`mov eax,ebx`), so its
//  operand byte read as a `ret`. A full .text sweep for displacements landing
//  in 0x00478b40..0x00479155 finds 117 hits, 116 internal and exactly one
//  external — `call 0x00478b40` at 0x00478733 — and ZERO references to
//  0x00478b4a from anywhere in the image.
//
//  Five cdecl arguments, confirmed by the caller's `add esp,0x14` at
//  0x00478738. The `push edi` is sunk to 0x00478c03, which the early-return
//  epilogue at 0x00478b9c confirms — it pops only esi/ebp/ebx.
//
//  CLIENT-SIDE ONLY: there is no /O1 twin, so nothing arbitrates a spelling
//  here the way server.dll does elsewhere.
//
//  BYTE-IDENTICAL, 1557 bytes. From the 1484-byte near, four things
//  closed it, each read off the shipped frame (sub esp,0x5c):
//    * every aggregate local at FUNCTION scope, in the order below -- nested
//      scopes let VC6 share slots and the frame came out 0x20;
//    * the two position read-backs are by-value (ReadV24/ReadV28 below):
//      the virtual fills a temporary at [esp+0x34] and the result is COPIED
//      into the read-back vector at [esp+0x10] (0x00478d22..0x00478d73);
//    * the packet's embedded message and the attachment pool are written as
//      expressions at each use, not held in locals: a pWrite local at the
//      top loaded pPacket in the prologue (shipped loads it at 0x00478b67),
//      and a function-level pool pointer took a spill slot. The free-list
//      walk alone holds the pool in a local (ecx, 0x00478e0c);
//    * sub_45be40's fourth argument is the dword read in the attachment loop
//      (VC6 puts it in arg5's dead home, [esp+0x80]), not arg5 -- with arg5
//      passed the read was dead and its load vanished.
//  The pre-U5 notes follow. Three things got the first build of 2220 down to
//  the near, and each is worth more than the bytes it bought:
//
//    * THE READER SPLIT. Spelling every read through packet.h's template put
//      us 663 bytes OVER, because the template is defined in class and so
//      expands at every use. The shipped image calls a reader at fifteen sites
//      and expands at six, and the split is not a budget artefact: every read
//      BEFORE the attachment loop is a call and every read INSIDE it is an
//      expansion. Counting the shipped `call` targets gives 5/2/2/6 for the
//      uint8/uint16/uint32/float instantiations, which matches the pre-loop
//      reads exactly. 2220 -> 1495.
//    * THE GETTER. Hoisting GetClientMgr() into a local at the top cost ~11
//      bytes and a whole prologue block: the shipped code uses the FIELD
//      m_pClientMgr at three sites and the CALL at one, and block 8 of this
//      file already records that the distinction is real.
//    * m_Type IS A BYTE. The shipped tests are `mov al,0xbc(%ebp)` / `cmp
//      $1,%al`, an eight-bit load; declaring it uint32 made every one of the
//      four type tests a dword compare.
// ======================================================================= //

// Two change-flag bits above CF_ATTACHMENTS that servermodel.h does not name,
// because the server never prints them. 0x200 is set here from CF_NEWOBJECT
// (0x00478b64 `or bh,2`) and rides with CF_POSITION at 0x00478cd6
// (`test ebx,0x202`); 0x400 rides with CF_ROTATION at 0x00478da3
// (`test ebx,0x404`) and the two are tested together at 0x00478de7
// (`and ecx,0x600`). Names ours.
// servermodel.h's change-flag bits. Repeated here rather than pulled in,
// because this TU is client-side and includes none of the server headers;
// the values are servermodel.h:34-41 unchanged.
#define CF_NEWOBJECT    0x1
#define CF_POSITION     0x2
#define CF_ROTATION     0x4
#define CF_FLAGS        0x8
#define CF_SCALE        0x10
#define CF_MODELINFO    0x20
#define CF_COLORINFO    0x40
#define CF_ATTACHMENTS  0x100

#define CF_POSITION_FORCE_Q478  0x200
#define CF_ROTATION_FORCE_Q478  0x400

// The model as the tracker reaches it: m_AnimArray (0x164) with its
// array at 0x168 and count at 0x16c; AnimInfo entries are 32 bytes.
class ModelAnimQ478
{
public:
	uint32 GetLength();                                         // 0x00445c30
};

struct AnimInfoQ478
{
	ModelAnimQ478* m_pAnim;                  // +0x00
	uint8          m_pad_04[0x08 - 0x04];
	LTVector       m_vDims;                  // +0x08  (0x0042073b..0x00420759)
	uint8          m_pad_14[0x20 - 0x14];
};

class CAnimModelQ478
{
public:
	uint8          m_pad_000[0x168];
	AnimInfoQ478*  m_pAnims;                 // 0x168  m_AnimArray.m_pArray
	uint32         m_nAnims;                 // 0x16c  m_AnimArray.m_nElements
};

// The animation tracker (LTAnimTracker), as 0x004789d0 / 0x00478a50 /
// 0x00420690 reach it.  (moved up so the object view can embed it)
class CUpdRecQ478
{
public:
	uint8        m_pad_00[0x04];
	CUpdRecQ478* m_pNext;                    // 0x04
	uint8        m_pad_08[0x18 - 0x08];
	// 0x18..0x28 read by 0x00478a50: LTAnimTracker's m_Flags (a WORD --
	// `andw $0xfffd,0x18(%esi)`), m_TimeRef.m_pModel and m_Cur.m_iAnim.
	uint16       m_Flags;                    // 0x18
	uint8        m_pad_1a[0x1c - 0x1a];
	class CAnimModelQ478* m_pModel;          // 0x1c
	uint16       m_iPrevAnim;                // 0x20  m_TimeRef.m_Prev.m_iAnim
	uint8        m_pad_22[0x28 - 0x22];
	uint16       m_iCurAnim;                 // 0x28
	uint8        m_pad_2a[0x34 - 0x2a];
	uint32       m_Unknown34;                // 0x34
};

// The object-change record, arg3. Every offset is a shipped displacement read
// off this function; the gaps are padding nothing here touches.
class CObjChangeInfoQ478
{
public:
	uint8   m_pad_0000[0x88];
	// 0x00478bbf replaces only the LOW WORD, masking with 0xffff0000 — and
	// 0x00478ce8 then tests bit 0x100 of it to pick the position encoding.
	uint32  m_Unknown88;                     // 0x088
	uint32  m_Unknown8C;                     // 0x08c
	uint32  m_Unknown90;                     // 0x090
	uint8   m_Color[4];                      // 0x094  four consecutive bytes
	class CAttachRecQ478* m_pAttachments;    // 0x098
	LTRotation m_Rotation;                   // 0x09c  (0x00420930)
	LTVector m_Scale;                        // 0x0ac  (0x00420580)
	// 0x0b8 -- the object id, a WORD (`mov 0xb8(%eax),%dx` at 0x0047918b).
	uint16  m_ObjectID;                      // 0x0b8
	uint8   m_pad_00ba[0xbc - 0xba];
	// 0x0bc — a BYTE, not a dword: the shipped tests are `mov al,0xbc(%ebp)`
	// / `cmp $1,%al`, so the load is eight bits wide. 1, 3 and 4 all branch.
	uint8   m_Type;                          // 0x0bc
	uint8   m_BlockingPriority;              // 0x0bd  (0x00420726)
	uint8   m_pad_00be[0x180 - 0xbe];
	// 0x180 -- the flag word 0x0042069a tests bit 4 of.
	uint32  m_Flags180;                      // 0x180
	uint8   m_pad_0184[0x1a8 - 0x184];
	float   m_Float1A8;                      // 0x1a8
	uint8   m_pad_01ac[0x1bc - 0x1ac];
	uint32  m_Unknown1BC;                    // 0x1bc
	CUpdRecQ478 m_AnimTracker;               // 0x1c0  (the first of m_pUpdRecs)
	class CUpdRecQ478* m_pUpdRecs;           // 0x1f8  (0x004789dc)
	uint8   m_pad_01fc[0x204 - 0x1fc];
	uint32  m_Unknown204;                    // 0x204  type 1 only

	// predobj.cpp's LTObject::SetPos: __thiscall, the vector BY VALUE
	// (0x00420930 builds the twelve-byte copy with `mov %esp,%ecx`).
	void SetPos(LTVector vPos);              // 0x0045aaa0
};

// The attachment record, walked at 0x00478e16..0x00478e25: the object's
// existing records go back on the manager's free list, read through +0x24 and
// re-linked through +0x00. Two different next pointers, which is what says the
// record is on two lists at once.
class CAttachRecQ478
{
public:
	CAttachRecQ478* m_pNext00;               // 0x00
	uint8           m_pad_04[0x24 - 0x04];
	CAttachRecQ478* m_pNext24;               // 0x24
};

// The attachment pool at CClientMgr+0x1ec. Reached by offset rather than as a
// member so CClientMgrQ478's layout above does not move.
class CAttachListQ478
{
public:
	uint8           m_pad_00[0x64];
	CAttachRecQ478* m_pFree64;               // 0x64
};

// The packet's embedded message, at CPacket+0x08. It cannot be a member of the
// CPacket view because packet.h's own m_pad_00 covers that offset, so it is
// reached by cast. Three of its virtuals are used by slot — +0x24, +0x28 and
// +0x30 — and all three are read positions returning through a hidden buffer.
class LMessageReadViewQ478
{
public:
	virtual void _v00();  virtual void _v04();  virtual void _v08();
	virtual void _v0c();  virtual void _v10();  virtual void _v14();
	virtual void _v18();  virtual void _v1c();  virtual void _v20();
	virtual void _v24(LTVector* pOut);       // 0x00478d1f
	virtual void _v28(LTVector* pOut);       // 0x00478d5c
	virtual void _v2c();
	virtual void _v30(LTRotation* pOut);     // 0x00478dc1

	// By-value spellings of +0x24 / +0x28: the shipped reader passes a
	// temporary and copies it out (0x00478d22), which is this.  (names ours)
	LTVector ReadV24() { LTVector v; _v24(&v); return v; }
	LTVector ReadV28() { LTVector v; _v28(&v); return v; }
};

// The callees, each cited by the address it is called at.
void    cc_TraceChangeFlags(CObjChangeInfoQ478* pInfo, uint32 nFlags);   // 0x00479160
LTRESULT sub_479280(void* pShell, CObjChangeInfoQ478* pInfo, CPacket* pPacket,
                    int bNotNew, int nOne, void* pArg5);                 // 0x00479280
void    sub_420670(CClientMgrQ478* pMgr, CObjChangeInfoQ478* pInfo);     // 0x00420670
void    sub_420580(CClientMgrQ478* pMgr, CObjChangeInfoQ478* pInfo,
                   LTVector* pScale);                                    // 0x00420580
// 0x004359c0 is server/servermsg.cpp's lmsg_ReadCompPos (pMsg, pVec, pRef);
// the third argument is the manager's CPosRef, 0x144/0x150 its min/max.
void    sub_4359c0(LMessageReadViewQ478* pMsg, LTVector* pPos,
                   CClientMgrQ478* pMgr);                                // 0x004359c0
void    sub_460db0(void* pShell, CObjChangeInfoQ478* pInfo, LTVector* pPos,
                   LTVector* pRead, uint32 nNew, uint32 nForce);         // 0x00460db0
void    sub_461120(void* pShell, CObjChangeInfoQ478* pInfo, LTRotation* pRot,
                   uint32 nNew, uint32 nForce);                          // 0x00461120
void    sub_45be40(CAttachListQ478* pList, CObjChangeInfoQ478* pInfo, uint16 nID,
                   void* pArg5, LTVector* pOffset, LTRotation* pRot, int nZero);
                                                                         // 0x0045be40
void    lmsg_ReadCompRotation(LMessageReadViewQ478* pMsg, LTRotation* pRot);
                                                                         // 0x00435d90

// The packet's embedded message (CPacket+0x08) and the client manager's
// attachment pool (CClientMgr+0x1ec), spelled out at each use -- see the
// banner: holding either in a local moves the prologue or the frame.
#define PWRITE_Q478 ((LMessageReadViewQ478*)((uint8*)pPacket + 8))
#define ATTACHLIST_Q478(pShell) ((CAttachListQ478*) \
	((uint8*)((CClientShellQ478*)(pShell))->m_pClientMgr + 0x1ec))

LTRESULT cc_ReadObjectPacketQ478(void* pShell, uint16 nChangeFlags,
                                CObjChangeInfoQ478* pInfo, CPacket* pPacket,
                                void* pArg5)
{
	// Locals, all at function scope; shipped slots after the four pushes:
	//   0x10 vRead   0x1c vScale   0x28 vPos   0x34 ReadV24/28's temporary
	//   0x40 vOffset   0x4c rRot   0x5c rOffset
	// The reconstruction names them by role.
	LTRotation rOffset;
	LTRotation rRot;
	LTVector   vOffset;
	LTVector   vPos;
	LTVector   vScale;
	LTVector   vRead;

	// 0x00478b57 -- the trace runs first, with the flag word narrowed.
	cc_TraceChangeFlags(pInfo, (uint32)(nChangeFlags & 0xffff));

	// 0x00478b5f `test bl,1` / 0x00478b64 `or bh,2` -- a new object always
	// forces the position path below.
	if (nChangeFlags & CF_NEWOBJECT)
		nChangeFlags |= CF_POSITION_FORCE_Q478;

	if (nChangeFlags & CF_MODELINFO)                     // 0x00478b6b, 0x20
	{
		if (pInfo->m_Type == 1)                          // 0x00478b70, +0xbc
		{
			// 0x00478b7a..0x00478b90.  Note the third argument:
			// `mov dl,bl ; not dl ; and edx,1` is the LOGICAL NEGATION of
			// CF_NEWOBJECT, not a mask.
			LTRESULT dResult = sub_479280(pShell, pInfo, pPacket,
				!(nChangeFlags & CF_NEWOBJECT), 1, pArg5);

			// 0x00478b98 `test eax,eax ; je` with its OWN epilogue at
			// 0x00478b9c -- a genuine early return, which is what /O2 does
			// with one.
			if (dResult != LT_OK)
				return dResult;
		}
		else if (pInfo->m_Type == 3)                     // 0x00478ba3
		{
			pInfo->m_Unknown1BC = ((CPacketQ478*)pPacket)->ReadTypeOOL((uint32*)0);  // 0x00478bab
		}
	}

	if (nChangeFlags & CF_FLAGS)                         // 0x00478bb6, 0x08
	{
		// 0x00478bbf -- only the LOW WORD of +0x88 is replaced; the shipped
		// code masks the member with 0xffff0000 and ors the read in.
		pInfo->m_Unknown88 = (pInfo->m_Unknown88 & 0xffff0000) |
			((CPacketQ478*)pPacket)->ReadTypeOOL((uint16*)0);

		pInfo->m_Unknown8C = ((CPacketQ478*)pPacket)->ReadTypeOOL((uint8*)0);    // 0x00478be1
		pInfo->m_Unknown90 = ((CPacketQ478*)pPacket)->ReadTypeOOL((uint32*)0);   // 0x00478bf5
	}

	if (nChangeFlags & CF_COLORINFO)                     // 0x00478c00, 0x40
	{
		// Four bytes into four consecutive members -- 0x94, 0x95, 0x96, 0x97.
		pInfo->m_Color[0] = ((CPacketQ478*)pPacket)->ReadTypeOOL((uint8*)0);     // 0x00478c0a
		pInfo->m_Color[1] = ((CPacketQ478*)pPacket)->ReadTypeOOL((uint8*)0);     // 0x00478c19
		pInfo->m_Color[2] = ((CPacketQ478*)pPacket)->ReadTypeOOL((uint8*)0);     // 0x00478c28
		pInfo->m_Color[3] = ((CPacketQ478*)pPacket)->ReadTypeOOL((uint8*)0);     // 0x00478c37

		if (pInfo->m_Type == 4)                          // 0x00478c42
		{
			// 0x00478c50..0x00478c65 -- a WORD read, zero-extended through a
			// stack slot and `fild`ed.  That integer round trip is what says
			// the source converts a uint16 to float, not a float read.
			pInfo->m_Float1A8 =
				(float)(uint32)((CPacketQ478*)pPacket)->ReadTypeOOL((uint16*)0);

			if (!(nChangeFlags & CF_SCALE))              // 0x00478c62
				sub_420670(((CClientShellQ478*)pShell)->m_pClientMgr, pInfo);
		}
	}

	if (nChangeFlags & CF_SCALE)                         // 0x00478c84, 0x10
	{
		vScale.x = ((CPacketQ478*)pPacket)->ReadTypeOOL((float*)0);         // 0x00478c8d
		vScale.y = ((CPacketQ478*)pPacket)->ReadTypeOOL((float*)0);         // 0x00478c9a

		// 0x00478ca9 -- type 3 gets a hard 1.0f (0x3f800000 stored as an
		// IMMEDIATE at 0x00478cad, which is an integer move of a float
		// constant) instead of a third read.
		if (pInfo->m_Type == 3)
			vScale.z = 1.0f;
		else
			vScale.z = ((CPacketQ478*)pPacket)->ReadTypeOOL((float*)0);     // 0x00478cbb

		sub_420580(((CClientShellQ478*)pShell)->m_pClientMgr, pInfo, &vScale);
	}                                                    // 0x00478cce

	if (nChangeFlags & (CF_POSITION | CF_POSITION_FORCE_Q478))   // 0x00478cd6
	{
		// 0x00478ce8 `test ah,1` on +0x88 -- bit 0x100 of the flag word the
		// CF_FLAGS branch above filled in.
		if (pInfo->m_Unknown88 & 0x100)
		{
			vPos.x = ((CPacketQ478*)pPacket)->ReadTypeOOL((float*)0);       // 0x00478cf1
			vPos.y = ((CPacketQ478*)pPacket)->ReadTypeOOL((float*)0);       // 0x00478cfe
			vPos.z = ((CPacketQ478*)pPacket)->ReadTypeOOL((float*)0);       // 0x00478d0b

			// 0x00478d1f -- LMessageImpl slot +0x24, out through a hidden
			// return buffer at [esp+0x34].
			vRead = PWRITE_Q478->ReadV24();
		}
		else
		{
			// 0x00478d4b -- the compressed form, which needs the client
			// manager; then slot +0x28 for the same read-back.
			sub_4359c0(PWRITE_Q478, &vPos,
				((CClientShellQ478*)pShell)->GetClientMgr());

			vRead = PWRITE_Q478->ReadV28();
		}

		// 0x00478d77..0x00478d9b.  Six cdecl arguments; the last two are
		// MASKS, not booleans -- `and eax,1` and `and ecx,0x200` off the
		// zero-extended flag word.
		sub_460db0(pShell, pInfo, &vPos, &vRead,
			nChangeFlags & CF_NEWOBJECT,
			nChangeFlags & CF_POSITION_FORCE_Q478);
	}

	if (nChangeFlags & (CF_ROTATION | CF_ROTATION_FORCE_Q478))   // 0x00478da3
	{
		if (pInfo->m_Unknown88 & 0x100)                  // 0x00478db1
			PWRITE_Q478->_v30(&rRot);                // 0x00478dc1, slot +0x30
		else
			lmsg_ReadCompRotation(PWRITE_Q478, &rRot);  // 0x00478dcf

		// 0x00478dd7..0x00478df2, five arguments, same mask shape as above
		// but with 0x600 -- BOTH forced bits.
		sub_461120(pShell, pInfo, &rRot,
			nChangeFlags & CF_NEWOBJECT,
			nChangeFlags & (CF_POSITION_FORCE_Q478 | CF_ROTATION_FORCE_Q478));
	}

	// 0x00478dfa `test bh,1` -- CF_ATTACHMENTS.  Everything below is inside
	// it; 0x00478dfd jumps straight to the shared epilogue when it is clear.
	if (nChangeFlags & CF_ATTACHMENTS)
	{
		// 0x00478e0c — the attachment pool at CClientMgr+0x1ec. Reached by
		// offset rather than declared as a member, so CClientMgrQ478's
		// layout above does not move for this one function.

		// 0x00478e16..0x00478e25 -- the object's existing attachment records
		// go back on the manager's free list, walked through +0x24 and
		// re-linked through +0x00.  A do/while: the list head is tested once
		// at 0x00478e12 and the loop closes with `jne` on the NEXT pointer.
		{
			CAttachListQ478* pList = ATTACHLIST_Q478(pShell);
			CAttachRecQ478*  pCur  = pInfo->m_pAttachments;    // +0x98

			while (pCur)
			{
				CAttachRecQ478* pNext = pCur->m_pNext24;

				pCur->m_pNext00 = pList->m_pFree64;
				pList->m_pFree64 = pCur;

				pCur = pNext;
			}
		}

		pInfo->m_pAttachments = LTNULL;                  // 0x00478e2b

		// ---- the attachment loop, 0x00478e35 .. 0x004790b2 -------------- //
		//
		// 0x00478e27 is `mov ebx,[esp+0x70]`, a load of arg1 into the
		// register the loop then uses ONLY as the sixteen-bit id (`mov
		// bx,[edx+eax]` at 0x00478e5a) and pushes WHOLE at 0x004790a1: it
		// is ReadType's uninitialised `T val` being given the register, the
		// same artefact as the +0x204 store at 0x00479145.  The plain loop
		// below reproduces both (byte-identical).
		for (;;)
		{
			uint16 nID;
			uint32 nData;

			nID = pPacket->ReadType((uint16*)0);         // 0x00478e35

			if (nID == 0xffff)                           // 0x00478e9e
				break;

			nData      = pPacket->ReadType((uint32*)0);  // 0x00478ea9
			vOffset.x  = pPacket->ReadType((float*)0);   // 0x00478f19
			vOffset.y  = pPacket->ReadType((float*)0);   // 0x00478f85
			vOffset.z  = pPacket->ReadType((float*)0);   // 0x00478ff9

			lmsg_ReadCompRotation(PWRITE_Q478, &rOffset);   // 0x0047907e

			sub_45be40(ATTACHLIST_Q478(pShell), pInfo, nID, (void*)nData,
				&vOffset, &rOffset, 0);                  // 0x004790aa
		}

		// 0x004790b7 -- only a type-1 object carries the trailing dword.
		if (pInfo->m_Type == 1)
			pInfo->m_Unknown204 = pPacket->ReadType((uint32*)0);   // 0x004790c4
	}

	return LT_OK;                                        // 0x004790ff / 0x0047914e
}


// ======================================================================= //
//  0x004789d0 -- 113 bytes, cdecl, five arguments.  Walks the object's
//  update-record list (+0x1f8, linked through +0x04) in step with the
//  decoded array arg5 (eight-byte entries: a dword then two words; the
//  count at +0x100), handing each pair to the 240-byte applier 0x00478a50.
//  A non-OK result returns at once (0x00478a1d `jne` straight onto the
//  epilogue); otherwise the entry's dword is stored into the record's +0x34
//  and both walks advance.  `jbe` on the count before the loop and `je` on
//  the record at the loop head: the for-condition and a break.
//  The two words are pushed ZERO-EXTENDED (`xor %edx,%edx ; mov 0x2(%edi),
//  %dx`), so the applier's parameters are dwords, not uint16 -- declaring
//  them uint16 was the one near (109 vs 113 bytes).
// ======================================================================= //


struct CUpdEntryQ478
{
	uint32 m_Unknown0;                       // +0
	uint16 m_Word4;                          // +4
	uint16 m_Word6;                          // +6
};

struct CUpdListQ478
{
	CUpdEntryQ478 m_Entries[32];             // 0x000
	uint32        m_nEntries;                // 0x100
};

LTRESULT sub_478a50(void* pShell, CObjChangeInfoQ478* pInfo, CUpdRecQ478* pRec,
                    uint32 nWord4, uint32 nWord6, int bNotNew, int nOne); // 0x00478a50
void     sub_4792c0(void* pShell, CPacket* pPacket, CUpdListQ478* pList); // 0x004792c0

LTRESULT sub_4789d0(void* pShell, CObjChangeInfoQ478* pInfo, int bNotNew,
                    int nOne, CUpdListQ478* pList)
{
	CUpdRecQ478* pRec = pInfo->m_pUpdRecs;
	uint32       i;

	for (i = 0; i < pList->m_nEntries; i++)
	{
		if (!pRec)
			break;

		LTRESULT dResult = sub_478a50(pShell, pInfo, pRec,
			pList->m_Entries[i].m_Word4, pList->m_Entries[i].m_Word6,
			bNotNew, nOne);
		if (dResult != LT_OK)
			return dResult;

		pRec->m_Unknown34 = pList->m_Entries[i].m_Unknown0;
		pRec = pRec->m_pNext;
	}

	return LT_OK;
}


// ======================================================================= //
//  0x00478a50 -- 240 bytes, cdecl, seven arguments; sole caller 0x00478a13.
//  The record is really an LTAnimTracker (m_pNext at +4 is its link, +0x18
//  its flag word, +0x1c the model, +0x28 the current animation index).
//  The word nWord4 carries the animation index in its low 14 bits (`and
//  $0x3f,%ah` == `& ~0xc000`), bit 15 -> tracker flag 2 and bit 14 ->
//  tracker flag 1; nWord6 is a 0..255 fraction of the animation length
//  (the 0x80808081 multiply is `/ 255`).  Calls, by address:
//  0x004011c0 at_SetAnim, 0x00420690 (manager, object), 0x00445c30
//  ModelAnim::GetLength, 0x00401160 at_SetTime, 0x00401280.
// ======================================================================= //

LTBOOL at_SetAnimQ478(CUpdRecQ478* pTracker, uint32 iAnim, LTBOOL bInterp);  // 0x004011c0
void   at_SetTimeQ478(CUpdRecQ478* pTracker, uint32 nTime, LTBOOL bInterp);  // 0x00401160
void   sub_401280(CUpdRecQ478* pTracker, uint32 nTime);                      // 0x00401280
void   sub_420690(CClientMgrQ478* pMgr, CObjChangeInfoQ478* pInfo);          // 0x00420690

LTRESULT sub_478a50(void* pShell, CObjChangeInfoQ478* pInfo, CUpdRecQ478* pRec,
                    uint32 nWord4, uint32 nWord6, int bNotNew, int nOne)
{
	LTBOOL bInterp;

	if ((pInfo->m_Unknown88 & 0x80) && bNotNew)
		bInterp = LTTRUE;
	else
		bInterp = LTFALSE;

	uint32 iAnim = nWord4 & ~0xc000;

	ModelAnimQ478* pAnim;
	if (iAnim >= pRec->m_pModel->m_nAnims)
		pAnim = LTNULL;
	else
		pAnim = pRec->m_pModel->m_pAnims[iAnim].m_pAnim;

	uint32 bPlaying = (nWord4 >> 14) & 1;

	if (nWord4 & 0x8000)
		pRec->m_Flags |= 2;
	else
		pRec->m_Flags &= ~2;

	if (bPlaying)
		pRec->m_Flags |= 1;
	else
		pRec->m_Flags &= ~1;

	if (bPlaying && !nOne && iAnim == pRec->m_iCurAnim)
		return LT_OK;

	at_SetAnimQ478(pRec, iAnim, bInterp);
	sub_420690(((CClientShellQ478*)pShell)->m_pClientMgr, pInfo);

	uint32 nLength = pAnim ? pAnim->GetLength() : 1;
	uint32 nTime = (nLength * nWord6) / 255;

	if (bPlaying)
		at_SetTimeQ478(pRec, nTime, bInterp);
	else
		sub_401280(pRec, nTime);

	return LT_OK;
}


// ======================================================================= //
//  0x00479160 -- 282 bytes, cdecl(object, flags).  The change-flag debug
//  trace: console int 0x004b32dc == 2 prints a blank line, the id line and
//  one line per set CF_ bit; == 1 prints the three lines only for a new
//  object.  The two id formats differ ("type %d" vs "type: %d") -- both are
//  in .data (0x004ae5e0, 0x004ae54c).  The type byte is pushed through
//  `movsbl`, so it is a (signed) char in the printf.
// ======================================================================= //

void cc_TraceChangeFlags(CObjChangeInfoQ478* pInfo, uint32 nFlags)
{
	if (g_CV_DebugPackets == 2)
	{
		con_Print("");
		con_Print("ObjectID: %d, type %d, flags %d",
			pInfo->m_ObjectID, (char)pInfo->m_Type, nFlags);

		if (nFlags & CF_NEWOBJECT)
			con_Print("CF_NEWOBJECT");
		if (nFlags & CF_POSITION)
			con_Print("CF_POSITION");
		if (nFlags & CF_ROTATION)
			con_Print("CF_ROTATION");
		if (nFlags & CF_FLAGS)
			con_Print("CF_FLAGS");
		if (nFlags & CF_SCALE)
			con_Print("CF_SCALE");
		if (nFlags & CF_MODELINFO)
			con_Print("CF_MODELINFO");
		if (nFlags & CF_COLORINFO)
			con_Print("CF_COLORINFO");
		if (nFlags & CF_ATTACHMENTS)
			con_Print("CF_ATTACHMENTS");
	}
	else if (g_CV_DebugPackets == 1)
	{
		if (nFlags & CF_NEWOBJECT)
		{
			con_Print("");
			con_Print("ObjectID: %d, type: %d, flags %d",
				pInfo->m_ObjectID, (char)pInfo->m_Type, nFlags);
			con_Print("CF_NEWOBJECT");
		}
	}
}


// ======================================================================= //
//  0x00479280 -- 50 bytes, cdecl, six arguments, called from 0x00478b90:
//  decode the packet into arg6, then apply it.  clientsoundinst.cpp's
//  banner had this exact body.  The decoder's result is not used (%eax is
//  overwritten by the second call, whose result is returned).
// ======================================================================= //

LTRESULT sub_479280(void* pShell, CObjChangeInfoQ478* pInfo, CPacket* pPacket,
                    int bNotNew, int nOne, void* pArg5)
{
	sub_4792c0(pShell, pPacket, (CUpdListQ478*)pArg5);
	return sub_4789d0(pShell, pInfo, bNotNew, nOne, (CUpdListQ478*)pArg5);
}


// ======================================================================= //
//  0x004792c0 -- 576 bytes, cdecl(shell, packet, list): the anim-tracker
//  list DECODER that 0x00479280 runs before 0x004789d0 applies it.
//  Clears the list's count (+0x100), reads a uint8 count, then per entry a
//  uint16, a uint8 and -- for every entry but the first -- a uint8 (the
//  first gets 0xffffffff), and appends {dword, word, word} while the list
//  has fewer than 32 entries (the count is re-read after the stores).
// ======================================================================= //

void sub_4792c0(void* pShell, CPacket* pPacket, CUpdListQ478* pList)
{
	uint32 nCount;
	uint32 i;

	pList->m_nEntries = 0;

	nCount = pPacket->ReadType((uint8*)0);
	for (i = 0; i < nCount; i++)
	{
		uint16 nWord4 = pPacket->ReadType((uint16*)0);
		uint8  nWord6 = pPacket->ReadType((uint8*)0);
		uint32 nVal;

		if (i == 0)
			nVal = (uint32)-1;
		else
			nVal = pPacket->ReadType((uint8*)0);

		if (pList->m_nEntries < 32)
		{
			CUpdEntryQ478* pEntry = &pList->m_Entries[pList->m_nEntries];

			pEntry->m_Unknown0 = nVal;
			pEntry->m_Word4    = nWord4;
			pEntry->m_Word6    = nWord6;
			pList->m_nEntries++;
		}
	}
}


// ======================================================================= //
//  0x00479500 -- 926 bytes, cdecl(packet, setup, sub-packet): the object
//  CREATE-INFO unpacker 0x00478510 runs for a new object (its one call is
//  0x004786e0, pushing the packet, the 0x28-byte setup it just built at
//  0x24(%esp) with 0x004789a0, and the sub-packet it just allocated).
//  Both results are dwords; the caller ignores the (zero) return.
//
//  WHAT PINS IT
//   * the setup is lt2new4.cpp's CUnknown4789A0 -- three twelve-byte file
//     refs {type, name, uint16 id} and, at +0x24, the ObjectCreateStruct
//     pointer (0x00478668 stores &OCS there).  Every OCS displacement is
//     ltbasedefs.h's: m_ObjectType +0x00 (16-bit store at 0x00479895),
//     m_ContainerCode +0x02, m_CreateFlags +0x04 (bit 0 = OCS_AUTOLOAD,
//     from bit 0x20 of the header), m_Filename +0x7c (strncpy 0x00492b60
//     with MAX_CS_FILENAME_LEN, no terminator store).
//   * the header byte: 0x20 OCS_AUTOLOAD, 0x40 "has user data", 0x80
//     "user data length is 16 bits" (0x0042f950 call) else 8 (inlined),
//     low 5 bits the object type.  The data goes to the sub-packet's
//     m_pData + 1 through 0x0045c2d0 and its m_DataLen grows by one.
//   * OT_MODEL (1) reads three file ids (first + MAX_MODEL_TEXTURES skins,
//     a counted-down loop), OT_WORLDMODEL (2) a name, OT_SPRITE (3) one id,
//     OT_CONTAINER (9) a name and the container code.  The first ref's type
//     is set to 2 once before the chain (0x00479623) and again with each
//     id.  OT_MODEL stores the type BEFORE its read (0x004796f9); OT_SPRITE
//     reads into a local first and stores both after (0x00479881).  The
//     skin loop indexes m_SkinNames[i] directly: through a `pRef` local,
//     VC6 bases the walk at the record (+0x0c) instead of shipped's
//     m_nLocalID (+0x14, `movl $2,-0x8(%edi)`).
// ======================================================================= //

struct CObjSetupRefQ479
{
	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

struct CObjSetupQ479
{
	CObjSetupRefQ479     m_Filename;                         // 0x00
	CObjSetupRefQ479     m_SkinNames[MAX_MODEL_TEXTURES];    // 0x0c, 0x18
	ObjectCreateStruct*  m_pStruct;                          // 0x24
};

LTRESULT sub_479500(CPacketQ478* pPacket, CObjSetupQ479* pSetup,
                    CPacketQ478* pData)
{
	uint8  nType;
	uint32 bHasData;
	uint8  bBigData;
	uint32 i;

	nType = pPacket->ReadTypeOOL((uint8*)0);

	if (nType & 0x20)
		pSetup->m_pStruct->m_CreateFlags |= OCS_AUTOLOAD;
	else
		pSetup->m_pStruct->m_CreateFlags &= ~OCS_AUTOLOAD;

	bHasData = nType & 0x40;
	bBigData = nType & 0x80;
	nType &= 0x1f;

	if (bHasData)
	{
		if (bBigData)
			pData->m_DataLen = pPacket->ReadTypeOOL((uint16*)0);
		else
			pData->m_DataLen = pPacket->ReadType((uint8*)0);
	}
	else
	{
		pData->m_DataLen = 0;
	}

	if (pData->m_DataLen)
	{
		pPacket->ReadData(pData->m_Data.m_pArray + 1, pData->m_DataLen);
		pData->m_DataLen++;
	}

	pSetup->m_Filename.m_nFileType = CFILEREF_BYID;

	if (nType == OT_WORLDMODEL)
	{
		strncpy(pSetup->m_pStruct->m_Filename, pPacket->ReadStringQ478(),
			MAX_CS_FILENAME_LEN);
	}
	else if (nType == OT_CONTAINER)
	{
		strncpy(pSetup->m_pStruct->m_Filename, pPacket->ReadStringQ478(),
			MAX_CS_FILENAME_LEN);
		pSetup->m_pStruct->m_ContainerCode = pPacket->ReadType((uint16*)0);
	}
	else if (nType == OT_MODEL)
	{
		pSetup->m_Filename.m_nFileType = CFILEREF_BYID;
		pSetup->m_Filename.m_nLocalID  = pPacket->ReadType((uint16*)0);
		for (i = 0; i < MAX_MODEL_TEXTURES; i++)
		{
			pSetup->m_SkinNames[i].m_nFileType = CFILEREF_BYID;
			pSetup->m_SkinNames[i].m_nLocalID  = pPacket->ReadType((uint16*)0);
		}
	}
	else if (nType == OT_SPRITE)
	{
		uint16 nID = pPacket->ReadType((uint16*)0);
		pSetup->m_Filename.m_nFileType = CFILEREF_BYID;
		pSetup->m_Filename.m_nLocalID  = nID;
	}

	pSetup->m_pStruct->m_ObjectType = nType;
	return LT_OK;
}


// ======================================================================= //
//  0x00479bf0 -- 347 bytes, cdecl(shell, packet, object id, change flags):
//  the SOUND arm of the update-packet reader (its one caller is 0x00478816
//  inside 0x00478510, which pushes a uint16 id from 0x0042f950 and a uint8
//  widened to uint16 by `movzbw`).
//
//  The frame is exactly float + CClientFileRef + PlaySoundInfo (0x148):
//  fOffset at +0x0c, the file ref at +0x10 (its inline ctor is the three
//  zero stores in the prologue), the PlaySoundInfo at +0x1c, whose
//  m_hSound (+0x110) takes the object id and whose m_vPosition (+0x128) is
//  where the position is read to.  CF_NEWOBJECT: drop whatever the id held
//  (0x00411bf0 for an object, the sound manager's ReleaseSound otherwise),
//  unpack the sound (0x00479d50) and play it last through g_pClientMgr's
//  0x00410960.  Otherwise the record must be a type-2 sound, which the
//  position (CF_POSITION: 0x004359c0 then SetPosition, vtable +0x38) and
//  flag 0x20 (KillLoop, 0x0047e2c0) are applied to.  (name ours)
// ======================================================================= //

class CSoundInstPosQ478
{
public:
	virtual void     _v00();
	virtual void     _v04();
	virtual void     _v08();
	virtual void     _v0c();
	virtual void     _v10();
	virtual void     _v14();
	virtual void     _v18();
	virtual void     _v1c();
	virtual void     _v20();
	virtual void     _v24();
	virtual void     _v28();
	virtual void     _v2c();
	virtual void     _v30();
	virtual void     _v34();
	virtual LTRESULT SetPosition(LTVector* pPos, int nUnknown);      // +0x38

	LTRESULT KillLoop();                                             // 0x0047e2c0
};

class CClientMgrSndQ478
{
public:
	LTRESULT PlaySound(PlaySoundInfo* pInfo, CClientFileRefQ478* pFile,
	                   float fOffset);                               // 0x00410960
};

extern CClientMgrSndQ478* g_pClientMgr;                              // 0x004b17e0

LTRESULT sub_479d50(CClientShellQ478* pShell, CPacketQ478* pPacket,
                    PlaySoundInfo* pInfo, CClientFileRefQ478* pFile,
                    float* pOffset);                                 // 0x00479d50

LTRESULT sub_479bf0(CClientShellQ478* pShell, CPacketQ478* pPacket,
                    uint16 nObjectID, uint16 nChangeFlags)
{
	PlaySoundInfo      psi;
	CClientFileRefQ478 fileRef;
	float              fOffset;
	CSoundInstPosQ478* pSound = LTNULL;
	CObjRecQ478*       pRec;

	if (nChangeFlags & CF_NEWOBJECT)
	{
		if (nObjectID != 0xffff)
		{
			pRec = obj_GetRecord(pShell->m_pClientMgr, nObjectID);
			if (pRec && pRec->m_pObject)
			{
				if (pRec->m_Type == 1)
					sub_411bf0(pShell->m_pClientMgr, pRec->m_pObject);
				else
					ci_GetSoundMgr()->ReleaseSound(
						(CSoundInstQ478*)pRec->m_pObject);
			}
		}

		sub_479d50(pShell, pPacket, &psi, &fileRef, &fOffset);
		psi.m_hSound = (HLTSOUND)(uint32)nObjectID;
	}
	else
	{
		pRec = obj_GetRecord(pShell->m_pClientMgr, nObjectID);
		if (pRec && pRec->m_pObject && pRec->m_Type == 2)
			pSound = (CSoundInstPosQ478*)pRec->m_pObject;
	}

	if (nChangeFlags & CF_POSITION)
	{
		sub_4359c0((LMessageReadViewQ478*)((uint8*)pPacket + 8),
			&psi.m_vPosition, pShell->GetClientMgr());
		if (pSound)
			pSound->SetPosition(&psi.m_vPosition, 0);
	}

	if ((nChangeFlags & 0x20) && pSound)
		pSound->KillLoop();

	if (nChangeFlags & CF_NEWOBJECT)
		g_pClientMgr->PlaySound(&psi, &fileRef, fOffset);

	return LT_OK;
}


// ======================================================================= //
//  0x00479d50 -- 893 bytes, cdecl(shell, packet, info, file ref, &offset):
//  the sound UNPACKER 0x00479bf0 runs for a new sound (its only call is
//  0x00479c83, in 0x00479bf0 above).  Both `ret`s return 0, so it is an
//  LTRESULT (the caller ignores it).  (name ours)
//
//  WHAT PINS IT
//   * `mov $0x4e,%ecx ; rep stos` over arg 3: PLAYSOUNDINFO_INIT, 0x138
//     bytes = sizeof(PlaySoundInfo); every later store lands on a
//     ltbasedefs.h field (m_dwFlags 0x00, m_nPriority 0x114,
//     m_fOuterRadius 0x118, m_fInnerRadius 0x11c, m_nVolume 0x120,
//     m_fPitchShift 0x124, m_vPosition 0x128, m_UserData 0x134).
//   * the file ref gets type 2 and a uint16 id, which is then the key of
//     the shell's per-id record (0x00414a50); a null record falls back to a
//     twelve-byte local.  The record is ccobj.cpp's CSoundSentStateQ464 --
//     the server's delta state for the same sound, mirrored here: change
//     flags 1/2/4 gate re-reading m_Flags, the priority, and the
//     range/inner-ratio pair, and whatever was last sent is used otherwise.
//   * PLAYSOUND_CTRL_VOL (0x40), PLAYSOUND_CTRL_PITCH (0x80) and
//     PLAYSOUND_TIMESYNC (0x400) gate the optional fields; the defaults 100
//     and 1.0f (0x3f800000) are immediates.  The offset is a uint8, 0xff
//     escaping to a uint32, in milliseconds (0.001f at 0x004a14a4); the
//     inner radius is outer * ratio * 1/255 (0x004a1a58).
//   * PLAYSOUND_CLIENTLOCAL (0x800) is taken as `shr $0xb ; and $1` BEFORE
//     the priority is read and parked in the dead file-ref argument slot;
//     at the end it copies the client object's position (or zero) into
//     m_vPosition.
// ======================================================================= //

class CShellObjQ478
{
public:
	uint8    m_pad_000[0x18c];
	LTVector m_Pos;                          // 0x18c  (LTObject::m_Pos)
};

// ccobj.cpp's CSoundSentStateQ464, the record 0x00414a50 hands back.
class CSoundRecQ478
{
public:
	uint8  m_ChangeFlags;                    // +0x00
	uint8  m_pad_01[0x02 - 0x01];
	uint16 m_Flags;                          // +0x02
	uint8  m_nPriority;                      // +0x04
	uint8  m_pad_05[0x06 - 0x05];
	uint16 m_nRange;                         // +0x06
	uint8  m_nInnerRatio;                    // +0x08
};

LTRESULT sub_479d50(CClientShellQ478* pShell, CPacketQ478* pPacket,
                    PlaySoundInfo* pInfo, CClientFileRefQ478* pFile,
                    float* pOffset)
{
	CSoundRecQ478  recLocal;
	CSoundRecQ478* pRec;
	uint32         bClientLocal;
	uint32         nOffset;

	PLAYSOUNDINFO_INIT(*pInfo);

	pFile->m_nFileType = CFILEREF_BYID;
	pFile->m_nLocalID  = pPacket->ReadTypeOOL((uint16*)0);

	pRec = (CSoundRecQ478*)pShell->GetObjRecord(pFile->m_nLocalID);
	if (!pRec)
		pRec = &recLocal;

	pRec->m_ChangeFlags = pPacket->ReadTypeOOL((uint8*)0);

	if (pRec->m_ChangeFlags & 1)
		pRec->m_Flags = pPacket->ReadTypeOOL((uint16*)0);
	pInfo->m_dwFlags = pRec->m_Flags;
	bClientLocal = (pInfo->m_dwFlags >> 11) & 1;

	if (pRec->m_ChangeFlags & 2)
		pRec->m_nPriority = pPacket->ReadTypeOOL((uint8*)0);
	pInfo->m_nPriority = pRec->m_nPriority;

	if (pInfo->m_dwFlags & (PLAYSOUND_AMBIENT | PLAYSOUND_3D))
	{
		if (pRec->m_ChangeFlags & 4)
		{
			pRec->m_nRange      = pPacket->ReadTypeOOL((uint16*)0);
			pRec->m_nInnerRatio = pPacket->ReadTypeOOL((uint8*)0);
		}

		pInfo->m_fOuterRadius = (float)pRec->m_nRange;
		pInfo->m_fInnerRadius = pInfo->m_fOuterRadius * (float)pRec->m_nInnerRatio * (1.0f / 255.0f);
	}

	if (pInfo->m_dwFlags & PLAYSOUND_CTRL_VOL)
		pInfo->m_nVolume = pPacket->ReadType((uint8*)0);
	else
		pInfo->m_nVolume = 100;

	if (pInfo->m_dwFlags & PLAYSOUND_CTRL_PITCH)
		pInfo->m_fPitchShift = pPacket->ReadTypeOOL((float*)0);
	else
		pInfo->m_fPitchShift = 1.0f;

	if (pInfo->m_dwFlags & PLAYSOUND_TIMESYNC)
	{
		nOffset = pPacket->ReadType((uint8*)0);
		if (nOffset == 0xff)
			nOffset = pPacket->ReadType((uint32*)0);

		*pOffset = (float)nOffset * 0.001f;
	}
	else
	{
		*pOffset = 0.0f;
	}

	pInfo->m_UserData = pPacket->ReadType((uint32*)0);

	if (bClientLocal)
	{
		if (pShell->m_pClientObject)
			pInfo->m_vPosition = pShell->m_pClientObject->m_Pos;
		else
			pInfo->m_vPosition.Init();
	}

	return LT_OK;
}


// ======================================================================= //
//  0x004798a0 -- 836 bytes, cdecl(shell, packet), called from 0x004787e6
//  inside 0x00478510 (whose `test %eax,%eax` reads the result): the
//  CLIENT-side twin of 0x00479d50 above.  The same record-driven unpack
//  into a LOCAL PlaySoundInfo and file ref, then played at once through
//  the shell's client manager (0x00410960, offset 0.0f).  (name ours)
//
//  WHAT DIFFERS FROM 0x00479d50, all read off the bytes
//   * m_dwFlags gets PLAYSOUND_CLIENT (0x200, `or $0x2,%ah`) ORed in, and
//     the CLIENTLOCAL bit is taken from the ORed value.
//   * no TIMESYNC offset; the pitch read is expanded, not a call.
//   * the position is only filled for an ambient/3D sound: read from the
//     packet (0x004359c0, as 0x00479bf0 does) unless CLIENTLOCAL, in which
//     case the client object's position, or zero.
//   * the file ref is a local with its inline constructor (the type zero
//     is killed by the type-2 store), and the record key is re-read from it.
//   * the zero position is stored z, y, x (0x00479b24..0x00479b3a): the
//     chained `x = y = z = 0.0f`, where 0x00479d50's Init() stores x first.
// ======================================================================= //
LTRESULT sub_4798a0(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	PlaySoundInfo      psi;
	CClientFileRefQ478 fileRef;
	CSoundRecQ478      recLocal;
	CSoundRecQ478*     pRec;
	uint32             bClientLocal;

	PLAYSOUNDINFO_INIT(psi);

	fileRef.m_nFileType = CFILEREF_BYID;
	fileRef.m_nLocalID  = pPacket->ReadTypeOOL((uint16*)0);

	pRec = (CSoundRecQ478*)pShell->GetObjRecord(fileRef.m_nLocalID);
	if (!pRec)
		pRec = &recLocal;

	pRec->m_ChangeFlags = pPacket->ReadTypeOOL((uint8*)0);

	if (pRec->m_ChangeFlags & 1)
		pRec->m_Flags = pPacket->ReadTypeOOL((uint16*)0);
	psi.m_dwFlags = pRec->m_Flags | PLAYSOUND_CLIENT;
	bClientLocal = (psi.m_dwFlags >> 11) & 1;

	if (pRec->m_ChangeFlags & 2)
		pRec->m_nPriority = pPacket->ReadTypeOOL((uint8*)0);
	psi.m_nPriority = pRec->m_nPriority;

	if (psi.m_dwFlags & (PLAYSOUND_AMBIENT | PLAYSOUND_3D))
	{
		if (pRec->m_ChangeFlags & 4)
		{
			pRec->m_nRange      = pPacket->ReadTypeOOL((uint16*)0);
			pRec->m_nInnerRatio = pPacket->ReadTypeOOL((uint8*)0);
		}

		psi.m_fOuterRadius = (float)pRec->m_nRange;
		psi.m_fInnerRadius = psi.m_fOuterRadius * (float)pRec->m_nInnerRatio * (1.0f / 255.0f);
	}

	if (psi.m_dwFlags & PLAYSOUND_CTRL_VOL)
		psi.m_nVolume = pPacket->ReadType((uint8*)0);
	else
		psi.m_nVolume = 100;

	if (psi.m_dwFlags & PLAYSOUND_CTRL_PITCH)
		psi.m_fPitchShift = pPacket->ReadType((float*)0);
	else
		psi.m_fPitchShift = 1.0f;

	if (psi.m_dwFlags & (PLAYSOUND_AMBIENT | PLAYSOUND_3D))
	{
		if (!bClientLocal)
		{
			sub_4359c0((LMessageReadViewQ478*)((uint8*)pPacket + 8),
				&psi.m_vPosition, pShell->GetClientMgr());
		}
		else if (pShell->m_pClientObject)
		{
			psi.m_vPosition = pShell->m_pClientObject->m_Pos;
		}
		else
		{
			psi.m_vPosition.x = psi.m_vPosition.y = psi.m_vPosition.z = 0.0f;
		}
	}

	psi.m_UserData = pPacket->ReadType((uint32*)0);

	((CClientMgrSndQ478*)pShell->m_pClientMgr)->PlaySound(&psi, &fileRef, 0.0f);

	return LT_OK;
}


// ======================================================================= //
//  0x0047a1f0 -- message id 10 (0x004b48f8), 482 bytes: the UNGUARANTEED
//  update.  First 0x00478320 (id 14's handler) over the same packet, its
//  error returned.  Then while the packet has data: a uint16 word whose low
//  twelve bits are the object id (0x00420520 finds the object) and whose
//  top four bits say what follows -- 0xffff hands the rest to 0x0047a3e0;
//  0x4000 a compressed position (0x004359c0) plus a vector read through
//  the message's slot +0x28 into a block-scoped vRead (+0x20) and COPIED to
//  vVel (+0x14) -- declaring slot +0x28 as returning LTVector by value
//  copies through %eax instead (478 B near) -- applied by 0x00460db0; 0x8000 a full rotation
//  (0x004362c0, on the packet) or else 0x2000 a compressed one (0x00435d90,
//  on the message), applied by 0x00461120; 0x1000 on a model (type byte 1)
//  the anim-tracker list through 0x00479280.  (name ours)
// ======================================================================= //

LTRESULT sub_478320(CClientShellQ478* pShell, CPacketQ478* pPacket);        // 0x00478320
LTRESULT sub_47a3e0(CClientShellQ478* pShell, CPacketQ478* pPacket);        // 0x0047a3e0
CObjChangeInfoQ478* sub_420520(CClientMgrQ478* pMgr, uint16 nID);           // 0x00420520
void     sub_4362c0(CPacketQ478* pPacket, LTRotation* pRot);                // 0x004362c0

LTRESULT sub_47a1f0(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	LTRESULT            dResult;
	uint16              nWord;
	CObjChangeInfoQ478* pInfo;
	LTVector            vVel;
	LTRotation          rRot;
	LTVector            vPos;
	CUpdListQ478        list;

	dResult = sub_478320(pShell, pPacket);
	if (dResult != LT_OK)
		return dResult;

	while (pPacket->m_DataLen - pPacket->m_Pos > 0)
	{
		nWord = pPacket->ReadType((uint16*)0);

		if (nWord == 0xffff)
		{
			dResult = sub_47a3e0(pShell, pPacket);
			if (dResult != LT_OK)
				return dResult;
		}
		else
		{
			pInfo = sub_420520(pShell->m_pClientMgr, nWord & 0xfff);

			if (nWord & 0x4000)
			{
				LMessageReadViewQ478* pMsg =
					(LMessageReadViewQ478*)((uint8*)pPacket + 8);

				LTVector vRead;

				sub_4359c0(pMsg, &vPos, pShell->GetClientMgr());

				pMsg->_v28(&vRead);
				vVel = vRead;

				if (pInfo)
					sub_460db0(pShell, pInfo, &vPos, &vVel, 0, 0);
			}

			if (nWord & 0x8000)
			{
				sub_4362c0(pPacket, &rRot);
				if (pInfo)
					sub_461120(pShell, pInfo, &rRot, 0, 0);
			}
			else if (nWord & 0x2000)
			{
				lmsg_ReadCompRotation(
					(LMessageReadViewQ478*)((uint8*)pPacket + 8), &rRot);
				if (pInfo)
					sub_461120(pShell, pInfo, &rRot, 0, 0);
			}

			if ((nWord & 0x1000) && pInfo && pInfo->m_Type == 1)
				sub_479280(pShell, pInfo, pPacket, 1, 0, &list);
		}
	}

	return LT_OK;
}


// ======================================================================= //
//  0x0047a4e0 -- message id 15 (0x004b490c), 687 bytes: new files for an
//  existing object.  The frame is a CClientCreateRequest (+0x10) and the
//  ObjectCreateStruct it points at (+0x38, to the end of the 0x2b4 frame;
//  every one of Clear()'s stores lands on ltbasedefs.h's layout).  The
//  struct is constructed (its ctor's Clear() call, 0x004147f0, stays a
//  call) and then Cleared again inline.  A uint16 object id, then the
//  object file's id and one per skin, each a type-2 file ref; then the
//  create-hook dispatcher 0x00477d60 on the object 0x00420520 finds, its
//  result returned, or LT_OK when there is no such object.  (name ours)
// ======================================================================= //

class CClientCreateRequestQ478
{
public:
	CClientFileRefQ478  m_ObjectFile;                        // 0x00
	CClientFileRefQ478  m_SkinFiles[MAX_MODEL_TEXTURES];     // 0x0c
	ObjectCreateStruct* m_pStruct;                           // 0x24
};

LTRESULT sub_477d60(CClientMgrQ478* pMgr, CObjChangeInfoQ478* pInfo,
                    CClientCreateRequestQ478* pRequest, LTRESULT dResult);  // 0x00477d60

LTRESULT sub_47a4e0(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	ObjectCreateStruct       ocs;
	CClientCreateRequestQ478 req;
	CObjChangeInfoQ478*      pInfo;
	uint16                   nObjectID;
	uint32                   i;

	req.m_pStruct = LTNULL;
	ocs.Clear();
	req.m_pStruct = &ocs;

	nObjectID = pPacket->ReadType((uint16*)0);

	req.m_ObjectFile.m_nLocalID  = pPacket->ReadType((uint16*)0);
	req.m_ObjectFile.m_nFileType = 2;

	for (i = 0; i < MAX_MODEL_TEXTURES; i++)
	{
		req.m_SkinFiles[i].m_nLocalID  = pPacket->ReadType((uint16*)0);
		req.m_SkinFiles[i].m_nFileType = 2;
	}

	pInfo = sub_420520(pShell->m_pClientMgr, nObjectID);
	if (pInfo)
		return sub_477d60(pShell->m_pClientMgr, pInfo, &req, LT_OK);

	return LT_OK;
}


// ======================================================================= //
//  0x00420580 -- 135 bytes, cdecl(manager, object, scale).  REAL HOME:
//  src/engine/lithtech/clienttexmgr.cpp's TU (0x00420550 is its neighbour);
//  written here because the
//  object view with +0xac/+0xbc lives here.  clientmgr.h declares it.
//
//  Scale changed?  three `fcomps ; test $0x40,%ah ; je` onto one
//  `mov $1,%edi`, the all-equal fall-through `xor %edi,%edi`.  Then the
//  three dword stores, the world tree's AddObject(obj, 0), and for a
//  changed OT_MODEL (type byte == 1) 0x00420690(manager, object).
// ======================================================================= //

void sub_420690(CClientMgrQ478* pMgr, CObjChangeInfoQ478* pInfo);  // 0x00420690

void sub_420580(CClientMgrQ478* pMgr, CObjChangeInfoQ478* pInfo,
                LTVector* pScale)
{
	LTBOOL bChanged;

	if (pScale->x == pInfo->m_Scale.x && pScale->y == pInfo->m_Scale.y &&
		pScale->z == pInfo->m_Scale.z)
		bChanged = LTFALSE;
	else
		bChanged = LTTRUE;

	pInfo->m_Scale = *pScale;

	pMgr->m_WorldTree.AddObject(pInfo, 0);

	if (bChanged && pInfo->m_Type == 1)
		sub_420690(pMgr, pInfo);
}


// ======================================================================= //
//  0x00420690 -- 261 bytes, cdecl(manager, object).  REAL HOME:
//  clienttexmgr.cpp's TU, beside 0x00420580 (its caller there) -- written
//  here for the same reason 0x00420580 is.  Re-derive a model's dims from
//  its current animation: the object must not carry flag 4 at +0x180, its
//  main tracker (+0x1c0) must be valid (LTAnimTracker::IsValid, inlined)
//  and the client must be connected (+0x150c); then the animation's dims
//  (AnimInfo+0x08) are scaled component-wise by the object's scale and
//  handed to wm_SetObjectDims (0x00453f90) through a WMMoveState filled as
//  clientphysics.cpp fills it.  Frame: the 0x68-byte state at the top
//  (E-0x68, its +0x64 cleared on entry), the dims below it (E-0x74).
// ======================================================================= //

struct WMMoveStateQ478
{
	void*  m_pContainer;                     // +0x00
	void*  m_pUnknown04;                     // +0x04
	void*  m_pObj;                           // +0x08
	uint32 m_nUnknown0C;                     // +0x0c
	uint32 m_pPushList;                      // +0x10
	uint32 m_nPushCount;                     // +0x14
	uint8  m_pad_18[0x64 - 0x18];
	uint32 m_nUnknown64;                     // +0x64
};

int wm_SetObjectDims(WMMoveStateQ478* pState, LTVector* pDims, int bFlag,
                     int bUnknown);                                  // 0x00453f90

void sub_420690(CClientMgrQ478* pMgr, CObjChangeInfoQ478* pInfo)
{
	WMMoveStateQ478 state;
	LTVector        vDims;

	state.m_pContainer = LTNULL;
	state.m_pUnknown04 = LTNULL;
	state.m_pObj = LTNULL;
	state.m_nUnknown64 = 0;

	// Four separate refusals, not one guard: the tests are the inlined
	// LTAnimTracker::IsValid's, but with the model and the current index
	// held in locals the body reuses (%edx, %ecx at 0x00420704..0x0042070e)
	// -- an inline IsValid() made VC6 reload both.
	if (pInfo->m_Flags180 & 4)
		return;

	CAnimModelQ478* pModel = pInfo->m_AnimTracker.m_pModel;
	if (!pModel)
		return;

	uint32 iCur = pInfo->m_AnimTracker.m_iCurAnim;
	if (iCur >= pModel->m_nAnims || pInfo->m_AnimTracker.m_iPrevAnim >= pModel->m_nAnims)
		return;

	if (!pMgr->m_pConnection)
		return;

	AnimInfoQ478* pAnimInfo = &pModel->m_pAnims[iCur];

	state.m_pContainer = &pMgr->m_WorldTree;
	state.m_pUnknown04 = pMgr->m_pUnknown508;
	state.m_pObj = pInfo;
	state.m_nUnknown0C = pInfo->m_BlockingPriority;
	state.m_pPushList = 0;
	state.m_nPushCount = 0;

	vDims = pAnimInfo->m_vDims;
	vDims.x *= pInfo->m_Scale.x;
	vDims.y *= pInfo->m_Scale.y;
	vDims.z *= pInfo->m_Scale.z;

	wm_SetObjectDims(&state, &vDims, 0, 1);
}


// ======================================================================= //
//  0x00420930 -- 122 bytes, cdecl(manager, object, pos, rotation), the body
//  of clientinterface.cpp's ci_SetObjectPosAndRotation (clientmgr.h
//  declares it).  Neighbour of 0x00420690 above, so the same TU.  SetPos
//  (0x0045aaa0) with the position by value, the rotation's four dwords
//  stored, then -- only when a manager was passed -- world models and
//  containers (type bytes 2 and 9) are retransformed (0x00450c30) and the
//  object goes back into the world tree (AddObject(obj, 0)).
// ======================================================================= //

void obj_RetransformWorldModel(CObjChangeInfoQ478* pObj);          // 0x00450c30

void sub_420930(CClientMgrQ478* pMgr, CObjChangeInfoQ478* pObj, LTVector* pPos,
                LTRotation* pRotation)
{
	pObj->SetPos(*pPos);
	pObj->m_Rotation = *pRotation;

	if (pMgr)
	{
		if (pObj->m_Type == 2 || pObj->m_Type == 9)
			obj_RetransformWorldModel(pObj);

		pMgr->m_WorldTree.AddObject(pObj, 0);
	}
}

// ======================================================================= //
//  0x0045c280 -- CPacketQ478::ReadStringQ478, 76 bytes: lithtech.exe's
//  copy of servermsg.cpp's CMsgStringReader::ReadStringInPlace (server
//  0x1002609f).  IDENTICAL.  Written through CMoArray's own accessors --
//  `m_Data[i]` (operator[] -> Get) and `m_Data.GetSize()` -- which is what
//  reloads m_pArray/m_nElements inside the loop and sets the SIB order of
//  `cmpb $0,(%edx,%ebx,1)`; the raw `m_pArray[n]` spelling (through a
//  `CMoArray<uint8>*` local or not) gave `(%ebx,%edx,1)`.
// ======================================================================= //

char* CPacketQ478::ReadStringQ478()
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



// ======================================================================= //
//  0x0047bef0 -- CServerThread::IsRequestQueued, lithtech.exe's copy (175
//  bytes; server.dll 0x10035152).  REAL HOME: src/engine/server/smgr.cpp,
//  which already carries this function in a `goto done` spelling that is a
//  near on lithtech (145/175): /O2 keeps the unlock-and-return tails apart,
//  so each hit unlocks and returns TRUE itself and the miss has its own
//  pair.  The same edit to smgr.cpp was measured identical on BOTH images
//  (server 0x10035152 stays identical); written here on views of
//  smgr.cpp's classes.
// ======================================================================= //

class LCritSecQ478
{
public:
	void Enter();                            // 0x00444180
	void Leave();                            // 0x00444190

	uint8 m_Opaque[0x18];
};

class LThreadMessageQ478
{
public:
	LThreadMessageQ478* m_pNext;             // +0x00
	LThreadMessageQ478* m_pPrev;             // +0x04
	uint32              m_Data[6];           // +0x08
};

struct LThreadQueueQ478
{
	LCritSecQ478        m_CS;                // +0x00
	uint8               m_pad_18[0x20 - 0x18];
	LThreadMessageQ478* m_pTail;             // +0x20  m_MessageList.m_pTail
	void*               m_hEvent;            // +0x24
};

class CServerThreadQ478
{
public:
	LTBOOL IsRequestQueued(void* pFile);     // 0x0047bef0

	void*            m_pVTable;              // +0x00
	LThreadQueueQ478 m_IncomingQueue;        // +0x04
	LThreadQueueQ478 m_OutgoingQueue;        // +0x2c
	uint8            m_pad_54[0x60 - 0x54];
	void*            m_pServerMgr;           // +0x60
};

LTBOOL CServerThreadQ478::IsRequestQueued(void* pFile)
{
	LCritSecQ478* pCS2;
	LCritSecQ478* pCS1;

	pCS1 = &m_IncomingQueue.m_CS;

	pCS1->Enter();

	pCS2 = &m_OutgoingQueue.m_CS;

	pCS2->Enter();

	if (m_pServerMgr)
	{
		LThreadMessageQ478* pCur;
		LThreadMessageQ478* pRec;

		pCur = m_IncomingQueue.m_pTail;

		while (pCur)
		{
			pRec = pCur;
			pCur = (pCur->m_pPrev == m_IncomingQueue.m_pTail) ? LTNULL : pCur->m_pPrev;

			if ((void*)pRec->m_Data[3] == pFile)
			{
				pCS2->Leave();
				pCS1->Leave();
				return LTTRUE;
			}
		}

		pCur = m_OutgoingQueue.m_pTail;

		while (pCur)
		{
			pRec = pCur;
			pCur = (pCur->m_pPrev == m_OutgoingQueue.m_pTail) ? LTNULL : pCur->m_pPrev;

			if ((void*)pRec->m_Data[3] == pFile)
			{
				pCS2->Leave();
				pCS1->Leave();
				return LTTRUE;
			}
		}
	}

	pCS2->Leave();
	pCS1->Leave();

	return LTFALSE;
}



// ======================================================================= //
//  0x0047b700 — message id 25, the light-animation update     933 bytes
//
//  The dispatch table stores it at 0x004b4934 (id 25) and the server sends
//  id 0x19 from sm_SendWorldInfo (server/smgr.cpp) with one record per
//  light animation, so this is the reader of those records:
//      uint16 index, uint8 field mask, then the fields the mask names.
//  Each record is merged into the current LAInfo (GetLightAnimInfo, slot 2
//  of m_pClientDE->m_pLightAnimLT, 0x0047b75d) and pushed back through
//  ltnew2.cpp's sub_00404af0 with bForce = 1 (0x0047ba75).
//
//  WHAT PINS IT
//   * `sub; cmp $3; jb` / `jae` at the bottom: an UNSIGNED `>= 3` loop test.
//   * the index and mask CALL the out-of-line readers (0x0042f950,
//     0x00459f80), and so do the two frame words (mask 1) and the
//     percent-between byte (mask 2); everything after is packet.h's
//     expansion -- /O2's inline budget spent from the end, as in 0x0047a970.
//   * the bytes are scaled by 1/255 (0x004a1a58) for the two percents, and
//     NOT scaled for the colour (fild/fstp only).
//   * mask 8 is the compressed position (0x004359c0) with GetClientMgr()
//     as its third argument, as 0x00479bf0 does.
// ======================================================================= //
#include "iltlightanim.h"

class CClientMgr;
LTRESULT sub_00404af0(CClientMgr* pClientMgr, uint32 index,
                      LAInfo* pInfo, int bForce);                        // 0x00404af0

LTRESULT sub_47b700(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	LAInfo info;
	uint32 hLightAnim;
	uint32 nFlags;

	while ((uint32)(pPacket->m_DataLen - pPacket->m_Pos) >= 3)
	{
		hLightAnim = pPacket->ReadTypeOOL((uint16*)0);
		nFlags     = pPacket->ReadTypeOOL((uint8*)0);

		pShell->m_pClientMgr->m_pClientDE->GetLightAnimLT()->GetLightAnimInfo(hLightAnim, info);

		if (nFlags & 1)
		{
			info.m_iFrames[0] = pPacket->ReadTypeOOL((uint16*)0);
			info.m_iFrames[1] = pPacket->ReadTypeOOL((uint16*)0);
		}

		if (nFlags & 2)
			info.m_fPercentBetween = (float)pPacket->ReadTypeOOL((uint8*)0) * (1.0f / 255.0f);

		if (nFlags & 4)
			info.m_fBlendPercent = (float)pPacket->ReadType((uint8*)0) * (1.0f / 255.0f);

		if (nFlags & 8)
			sub_4359c0((LMessageReadViewQ478*)((uint8*)pPacket + 8),
			           &info.m_vLightPos, pShell->GetClientMgr());

		if (nFlags & 0x10)
		{
			info.m_vLightColor.x = (float)pPacket->ReadType((uint8*)0);
			info.m_vLightColor.y = (float)pPacket->ReadType((uint8*)0);
			info.m_vLightColor.z = (float)pPacket->ReadType((uint8*)0);
		}

		if (nFlags & 0x20)
			info.m_fLightRadius = pPacket->ReadType((float*)0);

		sub_00404af0((CClientMgr*)pShell->m_pClientMgr, hLightAnim, &info, 1);
	}

	return LT_OK;
}


// ======================================================================= //
//  0x00478510 — OnUpdatePacket, message id 8               1,044 bytes
//  0x0047af20 — OnPreloadListPacket, message id 21            909 bytes
//
//  Both stored in the dispatch table 0x004b48d0 (0x004b48f0 and
//  0x004b4924); cmodelload.cpp's table fill names them sub_478510Q474 /
//  sub_47af20Q474, aliased onto these in link/aliases.rsp.
//
//  OnUpdatePacket: the message-group prefix (0x00478320) first, then a run
//  of sub-packets.  A leading flag byte (0x80 = a second, high flag byte
//  follows) that is non-zero is an OBJECT record: a uint16 id, and for
//  CF_NEWOBJECT the create-info (0x00479500) and the object built through
//  0x00411a70 (any old object on that id dropped first); otherwise the
//  object is looked up (0x00420520).  Either way the change record is
//  read (0x00478b40), and a new object's special-effect message, if any,
//  goes to IClientShell::SpecialEffectNotify (slot +0x28).  A zero flag
//  byte is followed by a type byte: 0 sound create (0x004798a0), 1 sound
//  update (0x00479bf0), 3 the remove list (0x0047a0d0); anything else is
//  an invalid packet.  The literals are "ReadObjectSubPacket" /
//  "OnUpdatePacket" (0x004ae514 / 0x004ae504).
//
//  OnPreloadListPacket: one type byte, the LithTech PRELOADTYPE_ values
//  0..5 (START, END, MODEL, MODEL_CACHED, SPRITE, SOUND) through a six-entry
//  jump table at 0x0047b294, each list case reading uint16 file ids until
//  the packet is empty.  START clears the models' cached bit (the model list
//  at CClientMgr+0x10f4), the textures' (0x004200c0) and the sounds'
//  (0x00481c60); END frees what is still unmarked (0x00481ca0,
//  FreeUnusedModels 0x004114a0) and acknowledges with message 9, byte 1.
//
//  The views below are local to this block.  (names ours)
// ======================================================================= //

// CClientShell's +0x18: the server game time the update trace prints.
class CClientShellTimeQ478
{
public:
	uint8   m_pad_00[0x18];
	float   m_fGameTime;                     // 0x18
};

// IClientShell past OnMessage: slot 10 (+0x28) is iclientshell.h's
// SpecialEffectNotify(HLOCALOBJ, HMESSAGEREAD).
class CClientShellIfSfxQ478 : public CClientShellIfQ478
{
public:
	virtual void _v18();  virtual void _v1c();  virtual void _v20();
	virtual void _v24();
	virtual void SpecialEffectNotify(void* hObj, void* hMessage);  // +0x28
};

// The write message's holder as 0x00478510 keeps it: an empty pointer at
// birth, assigned through the out-of-line 0x0047bc20 and released through
// the out-of-line 0x004666f0 (server/smgr.cpp's CRefHolder4666F0).
class CMsgHolderQ478
{
public:
	CMsgHolderQ478() { m_pMsg = LTNULL; }
	~CMsgHolderQ478();                                          // 0x004666f0
	CMsgQ478* operator=(const CMsgRefQ478& other);              // 0x0047bc20

	CMsgQ478* m_pMsg;
};

// 0x0047bc20 -- 37 bytes, __thiscall, `ret $0x4`: drop the held message,
// take the other's, AddRef it and hand it back.
CMsgQ478* CMsgHolderQ478::operator=(const CMsgRefQ478& other)
{
	if (m_pMsg)
		m_pMsg->Release();

	m_pMsg = other.m_pMsg;

	if (m_pMsg)
		m_pMsg->AddRef();

	return m_pMsg;
}

// The setup record 0x00479500 fills (lt2new4.cpp's CUnknown4789A0, whose
// constructor 0x004789a0 this reaches under its own name).
struct CUnknown4789A0Entry
{
	CUnknown4789A0Entry()
	{
		m_nA = 0;
		m_nB = 0;
		m_nC = 0;
	}

	uint32 m_nA;                              // 0x00
	uint32 m_nB;                              // 0x04
	uint16 m_nC;                              // 0x08
};

class CUnknown4789A0
{
public:
	CUnknown4789A0();                         // 0x004789a0

	CUnknown4789A0Entry  m_First;             // 0x00
	CUnknown4789A0Entry  m_Rest[2];           // 0x0c, 0x18
	ObjectCreateStruct*  m_pStruct;           // 0x24
};

// ObjectCreateStruct as 0x00478510 reaches it: at this one site VC6
// declined to expand either the in-class constructor or Clear(), so the
// shipped code CALLS both -- the constructor's standalone copy at
// 0x00478930 (whose only call site this is) and predobj.obj's Clear at
// 0x004147f0.  A view with both declared out of line reproduces the two
// calls; the constructor is defined below (its body is the SDK's own
// inline constructor), Clear is aliased onto predobj.obj's.
class CObjectCreateStructQ478 : public ObjectCreateStruct
{
public:
	CObjectCreateStructQ478();                                  // 0x00478930
	void ClearQ478();                                           // 0x004147f0
};

// 0x00478930 -- 111 bytes: ObjectCreateStruct::ObjectCreateStruct()
// (src/LT2/sdk/inc/ltbasedefs.h), Clear() expanded, `this` returned.
CObjectCreateStructQ478::CObjectCreateStructQ478()
{
}

// 0x00411a70 -- clientmgr2.cpp's object create (manager, server id,
// request, out object, bMove, bRotate).
LTRESULT sub_411a70Q478(CClientMgrQ478* pClientMgr, uint16 nServerID,
                        CUnknown4789A0* pRequest,
                        CObjChangeInfoQ478** ppObject,
                        int bMove, int bRotate);               // 0x00411a70

// The object record reader.  Its own error literal says it was a function
// of its own ("ReadObjectSubPacket", 0x004ae514); it is expanded inline
// into its one caller, which is why its constant error result travels
// through the caller's dResult register (0x004788b8).
inline LTRESULT ReadObjectSubPacket(CClientShellQ478* pShell,
                                    CPacketQ478* pPacket, uint16 nFlags)
{
	LTRESULT dResult;
	uint16   nID;

	nID = pPacket->ReadType((uint16*)0);

	CMsgHolderQ478          cMsg;
	CObjectCreateStructQ478 ocs;
	CUnknown4789A0          setup;
	CUpdListQ478            list;
	CObjChangeInfoQ478*     pInfo;
	CObjRecQ478*            pRec;

	cMsg = msg_StartWrite(0x1000, 0x1000);

	setup.m_pStruct = &ocs;
	pInfo = LTNULL;

	if (nFlags & CF_NEWOBJECT)
	{
		if (nID == 0xffff)
		{
			pInfo = LTNULL;
		}
		else
		{
			pRec = obj_GetRecord(pShell->m_pClientMgr, nID);
			if (pRec && pRec->m_pObject)
			{
				if (pRec->m_Type == 1)
					sub_411bf0(pShell->m_pClientMgr, pRec->m_pObject);
				else
					ci_GetSoundMgr()->ReleaseSound((CSoundInstQ478*)pRec->m_pObject);
			}
		}

		ocs.ClearQ478();
		sub_479500(pPacket, (CObjSetupQ479*)&setup, (CPacketQ478*)cMsg.m_pMsg);

		dResult = sub_411a70Q478(pShell->m_pClientMgr, nID, &setup, &pInfo,
		                         !(nFlags & CF_POSITION),
		                         !(nFlags & CF_ROTATION));
		if (dResult != LT_OK)
			return dResult;
	}
	else
	{
		pInfo = sub_420520(pShell->m_pClientMgr, nID);
		if (!pInfo)
		{
			pShell->m_pClientMgr->SetupError(LT_INVALIDSERVERPACKET);
			RETURN_ERROR(1, "ReadObjectSubPacket", LT_INVALIDSERVERPACKET);
		}
	}

	if (pInfo)
	{
		dResult = cc_ReadObjectPacketQ478(pShell, nFlags, pInfo, pPacket, &list);
		if (dResult != LT_OK)
			return dResult;

		if (nFlags & CF_NEWOBJECT)
		{
			if (cMsg.m_pMsg->m_DataLen > 0)
			{
				CClientShellIfSfxQ478* pIf =
					(CClientShellIfSfxQ478*)pShell->m_pClientMgr->m_pClientShell;

				if (pIf)
				{
					cMsg.m_pMsg->m_Pos = 1;
					pShell->m_pClientMgr->sub_411440((CPacketQ478*)cMsg.m_pMsg);
					pIf->SpecialEffectNotify(pInfo, (uint8*)cMsg.m_pMsg + 8);

					if (pInfo->m_Type == 1)
						sub_4789d0(pShell, pInfo, 0, 1, &list);
				}
			}
		}
	}

	return LT_OK;
}

LTRESULT sub_478510(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	LTRESULT dResult;
	uint16   nFlags;
	uint8    nType;

	if (g_CV_DebugPackets > 3)
	{
		con_Print("Update packet with game time %f",
		          ((CClientShellTimeQ478*)pShell)->m_fGameTime);
		con_Print("");
	}

	dResult = sub_478320(pShell, pPacket);
	if (dResult != LT_OK)
		return dResult;

	while (pPacket->m_DataLen - pPacket->m_Pos > 0)
	{
		nFlags = pPacket->ReadTypeOOL((uint8*)0);
		if (nFlags & 0x80)
			nFlags |= (uint16)pPacket->ReadTypeOOL((uint8*)0) << 8;

		if (nFlags)
		{
			dResult = ReadObjectSubPacket(pShell, pPacket, nFlags);
			if (dResult != LT_OK)
				return dResult;
		}
		else
		{
			nType = pPacket->ReadTypeOOL((uint8*)0);

			if (nType == 0)
			{
				dResult = sub_4798a0(pShell, pPacket);
				if (dResult != LT_OK)
					return dResult;
			}
			else if (nType == 1)
			{
				uint16 nChange = pPacket->ReadTypeOOL((uint8*)0);
				uint16 nObject = pPacket->ReadTypeOOL((uint16*)0);

				dResult = sub_479bf0(pShell, pPacket, nObject, nChange);
				if (dResult != LT_OK)
					return dResult;
			}
			else if (nType == 3)
			{
				dResult = sub_47a0d0(pShell, pPacket);
				if (dResult != LT_OK)
					return dResult;
			}
			else
			{
				pShell->m_pClientMgr->SetupError(LT_INVALIDSERVERPACKET);
				RETURN_ERROR(1, "OnUpdatePacket", LT_INVALIDSERVERPACKET);
			}
		}
	}

	return LT_OK;
}


// ---- what OnPreloadListPacket reaches ---------------------------------- //

// The loaded model's flag word; bit 0 is "cached" (preloaded).
class CModelFlagsQ478
{
public:
	uint8   m_pad_00[0x20];
	uint32  m_Flags;                         // 0x20
};

// A shared texture's flag word (0x0041ff90's result); 0x8000 is the
// preload mark (clienttexmgr.cpp's `orb $0x80,0x33`).
class CTextureFlagsQ478
{
public:
	uint8   m_pad_00[0x32];
	uint16  m_Flags;                         // 0x32
};

// CClientMgr with its model list (+0x10f4) and FreeUnusedModels
// (0x004114a0, clientmgr2.cpp / cltrest.cpp).
class CClientMgrPreloadQ478 : public CClientMgrQ478
{
public:
	LTRESULT FreeUnusedModels();                                // 0x004114a0
};

class CClientMgrModelsQ478
{
public:
	uint8   m_pad_0000[0x10f4];
	LTLink  m_Models;                        // 0x10f4
};

// The write message's out-of-line WriteType<uint8> (0x004159f0,
// clientmgr3.cpp).
class CMsgWriteQ478 : public CMsgQ478
{
public:
	void WriteTypeOOL(uint8 val);                               // 0x004159f0
};

// The sound manager's preload marks (clientsoundmgr.cpp) and
// GetSoundData (sndmgr2.cpp).
class CSoundMgrPreloadQ478 : public CSoundMgrQ478
{
public:
	LTRESULT sub_481c60();                                      // 0x00481c60
	LTRESULT sub_481ca0();                                      // 0x00481ca0
	void*    GetSoundData(CFileRecQ478* pFileRec);              // 0x00480a40
};

void     sub_4200c0Q478(CClientMgrQ478* pClientMgr);            // 0x004200c0
CTextureFlagsQ478* sub_41ff90Q478(CClientMgrQ478* pClientMgr,
                                  CClientFileRefQ478* pRef);    // 0x0041ff90
LTRESULT sub_476e80Q478(CClientMgrQ478* pClientMgr, CClientFileRefQ478* pRef,
                        void** ppSprite);                       // 0x00476e80

LTRESULT sub_47af20(CClientShellQ478* pShell, CPacketQ478* pPacket)
{
	CClientFileRefQ478 cRef;
	CModelQ478*        pModel;
	CFileRecQ478*      pRec;
	void*              pSprite;

	cRef.m_nFileType = CFILEREF_BYID;

	switch (pPacket->ReadTypeOOL((uint8*)0))
	{
		case 0:
		{
			LTLink* pHead = &((CClientMgrModelsQ478*)pShell->m_pClientMgr)->m_Models;
			LTLink* pCur;

			for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
				((CModelFlagsQ478*)pCur->m_pData)->m_Flags &= ~1;

			sub_4200c0Q478(pShell->m_pClientMgr);
			((CSoundMgrPreloadQ478*)ci_GetSoundMgr())->sub_481c60();
			return LT_OK;
		}

		case 1:
		{
			((CSoundMgrPreloadQ478*)ci_GetSoundMgr())->sub_481ca0();
			((CClientMgrPreloadQ478*)pShell->m_pClientMgr)->FreeUnusedModels();

			CMsgHoldQ478 cMsg(msg_StartWrite(0x1000, 0x1000).m_pMsg);

			cMsg.m_pMsg->m_Data.m_pArray[0] = 9;
			((CMsgWriteQ478*)cMsg.m_pMsg)->WriteTypeOOL(1);

			pShell->m_pClientMgr->m_NetMgr.SendTo(cMsg.m_pMsg, pShell->m_nConnID,
			                                      0x80);
			return LT_OK;
		}

		case 2:
			while (pPacket->m_DataLen - pPacket->m_Pos > 0)
			{
				cRef.m_nLocalID = pPacket->ReadTypeOOL((uint16*)0);

				if (sub_4770e0(pShell->m_pClientMgr, &cRef, &pModel, &pRec,
				               1, 1) == LT_OK)
				{
					((CModelFlagsQ478*)pModel)->m_Flags |= 1;
				}
			}
			break;

		case 3:
			while (pPacket->m_DataLen - pPacket->m_Pos > 0)
			{
				CTextureFlagsQ478* pTexture;

				cRef.m_nLocalID = pPacket->ReadTypeOOL((uint16*)0);

				pTexture = sub_41ff90Q478(pShell->m_pClientMgr, &cRef);
				if (pTexture)
					pTexture->m_Flags |= 0x8000;
			}
			break;

		case 4:
			while (pPacket->m_DataLen - pPacket->m_Pos > 0)
			{
				cRef.m_nLocalID = pPacket->ReadType((uint16*)0);

				sub_476e80Q478(pShell->m_pClientMgr, &cRef, &pSprite);
			}
			break;

		case 5:
			while (pPacket->m_DataLen - pPacket->m_Pos > 0)
			{
				cRef.m_nLocalID = pPacket->ReadType((uint16*)0);

				pRec = fm_GetFileRec(pShell->m_pClientMgr->m_pFileMgr, &cRef, 4);
				if (pRec)
					((CSoundMgrPreloadQ478*)ci_GetSoundMgr())->GetSoundData(pRec);
			}
			break;
	}

	return LT_OK;
}
