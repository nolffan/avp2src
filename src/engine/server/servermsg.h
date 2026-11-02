// *********************************************************************** //
//  MODULE:   SERVERMSG (.H)
//  PURPOSE:  server.dll's ILTCSBase message-writer methods (CLTServer), the
//            message payload movers, the packet allocator and the three
//            compressed codecs (vector, position, rotation) the message
//            layer shares with the client side of the engine.
//            IMessageWrite is the engine-internal write-message object behind
//            the HMESSAGEWRITE handle; its vtable slots are recovered from the
//            forwarding offsets in the binary (Byte=0x40 .. FormattedHStr=0x78).
//            server.dll is built /O1 (minimize size) — see SERVER-NOTES.md.
// *********************************************************************** //
#ifndef __SERVERMSG_H__
#define __SERVERMSG_H__

#include <stdarg.h>
#include "ltbasedefs.h"
#include "ltlink.h"
#include "ltengineobjects.h"
#include "iltserver.h"

class IMessageWrite
{
public:
	virtual void     EndWrite()=0;                        // 0x00
	virtual LTRESULT _p01()=0;
	// 0x08 — reads a word into an out parameter, on what is otherwise a write
	// interface: 0x10034fcb calls it to pull an object id back off the message
	// before resolving it, so the same vftable serves both directions.
	virtual LTRESULT ReadWord(uint16* pVal)=0;            // 0x08
	virtual LTRESULT _p03()=0; virtual LTRESULT _p04()=0; virtual LTRESULT _p05()=0;
	virtual LTRESULT _p06()=0; virtual LTRESULT _p07()=0; virtual LTRESULT _p08()=0;
	virtual LTRESULT _p09()=0; virtual LTRESULT _p10()=0; virtual LTRESULT _p11()=0;
	virtual LTRESULT _p12()=0; virtual LTRESULT _p13()=0; virtual LTRESULT _p14()=0;
	virtual LTRESULT _p15()=0;
	virtual LTRESULT WriteByte(uint8 val)=0;              // 0x40
	virtual LTRESULT WriteWord(uint16 val)=0;             // 0x44
	virtual LTRESULT WriteDWord(uint32 val)=0;            // 0x48
	virtual LTRESULT WriteFloat(float val)=0;             // 0x4c
	virtual LTRESULT WriteString(char* pStr)=0;           // 0x50
	virtual LTRESULT WriteVector(LTVector* pVal)=0;       // 0x54
	virtual LTRESULT WriteCompVector(LTVector* pVal)=0;   // 0x58
	virtual LTRESULT WriteCompPos(LTVector* pVal)=0;      // 0x5c
	virtual LTRESULT WriteRotation(LTRotation* pVal)=0;   // 0x60
	virtual LTRESULT _p25()=0;                            // 0x64
	// 0x68 — iltmessage.h's WriteRaw; every fixed-width writer is this one
	// with a constant length and the argument's own address.
	virtual LTRESULT WriteRaw(void* pData, uint32 len)=0; // 0x68
	virtual LTRESULT WriteMessageHandle(void* h)=0;       // 0x6c
	virtual LTRESULT WriteHString(HSTRING h)=0;           // 0x70
	virtual LTRESULT _p29()=0;                            // 0x74
	// Takes the format string and a pointer to the caller's va_list.
	virtual LTRESULT WriteFormattedHString(char* pMsg, va_list* pMarker)=0;  // 0x78
	virtual LTRESULT _p31()=0;                            // 0x7c
	virtual LTRESULT WriteObject(HOBJECT hObj)=0;         // 0x80
	virtual LTRESULT _p33()=0;                            // 0x84
	virtual LTRESULT _p34()=0;                            // 0x88
	// 0x8c — nonzero when the handle is not writable; every sender and the
	// EndMessage pair validate through it (name ours).
	virtual LTRESULT Verify()=0;                          // 0x8c
};

typedef IMessageWrite* HMESSAGEWRITE_I;

// Engine-internal read-message object (behind the HMESSAGEREAD handle).
// Vtable slots recovered from the forwarding offsets in the binary.
class IMessageRead
{
public:
	virtual void     EndRead()=0;                         // 0x00
	virtual LTRESULT ReadByte(uint8* pVal)=0;             // 0x04  ReadByteFL
	virtual LTRESULT ReadWord(uint16* pVal)=0;            // 0x08
	virtual LTRESULT ReadDWord(uint32* pVal)=0;           // 0x0c
	virtual LTRESULT ReadFloat(float* pVal)=0;            // 0x10
	virtual LTRESULT _p05()=0;                            // 0x14
	virtual LTRESULT ReadHString(HSTRING* pVal)=0;        // 0x18
	virtual LTRESULT _p07()=0;                            // 0x1c
	// 0x20 — iltmessage.h's ReadRawFL: every fixed-width reader above is
	// this one with a constant length.
	virtual LTRESULT ReadRawFL(void* pData, uint32 len)=0;
	virtual LTRESULT ReadVector(LTVector* pVal)=0;        // 0x24
	virtual LTRESULT ReadCompVector(LTVector* pVal)=0;    // 0x28
	virtual LTRESULT ReadCompPos(LTVector* pVal)=0;       // 0x2c
	virtual LTRESULT ReadRotation(LTRotation* pVal)=0;    // 0x30
	virtual LTRESULT _p13()=0;                            // 0x34
	virtual LTRESULT ReadMessage(void** pVal)=0;          // 0x38
	virtual LTRESULT ReadObject(HOBJECT* pVal)=0;         // 0x3c
	virtual LTRESULT _p16()=0; virtual LTRESULT _p17()=0; // 0x40 0x44
	virtual LTRESULT _p18()=0; virtual LTRESULT _p19()=0; // 0x48 0x4c
	virtual LTRESULT _p20()=0; virtual LTRESULT _p21()=0; // 0x50 0x54
	virtual LTRESULT _p22()=0; virtual LTRESULT _p23()=0; // 0x58 0x5c
	virtual LTRESULT _p24()=0; virtual LTRESULT _p25()=0; // 0x60 0x64
	virtual LTRESULT _p26()=0; virtual LTRESULT _p27()=0; // 0x68 0x6c
	virtual LTRESULT _p28()=0; virtual LTRESULT _p29()=0; // 0x70 0x74
	virtual LTRESULT _p30()=0; virtual LTRESULT _p31()=0; // 0x78 0x7c
	virtual LTRESULT _p32()=0;                            // 0x80
	virtual void     ResetRead()=0;                       // 0x84
};

typedef IMessageRead* HMESSAGEREAD_I;


// The write-message object a caller gets behind an HMESSAGEWRITE. The routing
// fields live here, on the sub-object, not on the queue entry that contains
// it — which is why every field store in the starters is relative to the
// handle rather than to what the manager returned.
class LMessageImpl
{
public:
	// The vptr, and a real one: ServerMessage's constructor stores this
	// class's table (0x1004de10) into its own +0x08 from member-construction
	// position, ahead of the packet data's constructor call, which only a
	// member with a constructor of its own can reach. That table is
	// iltmessage.h in declaration order — slot 0 is Release (0x10014ccd),
	// then ReadByteFL, ReadWordFL, ReadDWordFL, ReadFloatFL, ReadStringFL,
	// ReadHStringFL, ReadHStringAsStringFL, ReadRawFL, ReadVectorFL,
	// ReadCompVectorFL, ReadCompPosFL, ReadRotationFL, ReadCompRotationFL,
	// ReadMessageFL, ReadObjectFL — but the methods stay non-virtual below,
	// because the reconstruction reaches them through the IMessageRead /
	// IMessageWrite views instead and declaring them virtual here would turn
	// the direct calls the shipped bytes have into indirect ones. So one
	// placeholder slot carries the vptr and nothing else.
	virtual void _lm00();
	// +0x04  CServerMgr::StartMessage seeds it from mgr+0x804. Two slots of
	// it are reached: the object reader and the object writer, both taking
	// the message as their first argument (names ours).
	class CMsgObjectIO* m_pObjectIO;
	class ServerMessage* m_pOwner;           // +0x08  the queue entry containing this
	uint32   m_Type;                         // 0x0c  MT_*
	HCLIENT  m_hClient;                      // 0x10
	HOBJECT  m_hSender;                      // 0x14
	HOBJECT  m_hSendTo;                      // 0x18
	uint32   m_MessageID;                    // 0x1c
	LTVector m_vPos;                         // 0x20

	// Defined in lmessage.cpp — a TU compiled /Os (no /Og), unlike the rest
	// of the server (CLocalDriver::Disconnect proved the flag).
	LTRESULT ReadStringFL(char* pStr, uint32 maxLen);          // 0x100145fc
	LTRESULT ReadHStringAsStringFL(char* pStr, uint32 maxLen); // 0x10014720
	LTRESULT WriteMessage(class IMessageWrite* hMsg);          // 0x10014b75

	// The rest of iltmessage.h, anchored by those three: the vtable at
	// 0x1004de10 (the one ServerMessage's constructor stores into +0x08)
	// lands on the header's thirty-five virtuals in declaration order, with
	// ReadStringFL and WriteMessage where the header puts them. Every name
	// below is the SDK's.
	LTRESULT Release();                                        // 0x10014ccd
	LTRESULT ReadByteFL(uint8& val);                           // 0x100145d2
	LTRESULT ReadWordFL(uint16& val);                          // 0x100145e0
	LTRESULT ReadDWordFL(uint32& val);                         // 0x100145ee
	LTRESULT ReadFloatFL(float& val);                          // 0x100145ee (folded)
	LTRESULT ReadRawFL(void* pData, uint32 len);               // 0x10014cd8
	LTRESULT ReadVectorFL(LTVector& vec);                      // 0x100147eb
	LTRESULT ReadCompPosFL(LTVector& vec);                     // 0x10014911
	LTRESULT ReadRotationFL(LTRotation& rot);                  // 0x1001493a
	LTRESULT ReadCompRotationFL(LTRotation& rot);              // 0x10014990
	LTRESULT ReadObjectFL(HOBJECT& hObj);                      // 0x100147da
	LTRESULT WriteByte(uint8 val);                             // 0x10014a44
	LTRESULT WriteWord(uint16 val);                            // 0x10014a53
	LTRESULT WriteDWord(uint32 val);                           // 0x10014a62
	LTRESULT WriteFloat(float val);                            // 0x10014a62 (folded)
	LTRESULT WriteString(char* pData);                         // 0x10014a71
	LTRESULT WriteVector(LTVector& vec);                       // 0x10014ab0
	LTRESULT ReadHStringFL(HSTRING& hString);                  // 0x10014683
	LTRESULT ReadMessageFL(class LMessageImpl*& pMsg);         // 0x100149a1
	LTRESULT ReadCompVectorFL(LTVector& vec);                  // 0x10014830
	LTRESULT WriteRotation(LTRotation& rot);                   // 0x10014b20
	LTRESULT WriteHString(HSTRING hString);                    // 0x10014c13
	LTRESULT WriteStringAsHString(char* pStr);                 // 0x10014c8c
	LTRESULT WriteCompVector(LTVector& vec);                   // 0x10014ae6
	LTRESULT WriteCompPos(LTVector& vec);                      // 0x10014af7
	LTRESULT WriteHStringFormatted(int messageCode, ...);      // 0x10014c5b
	LTRESULT WriteHStringArgList(int messageCode, va_list* pList);  // 0x10014c76
	LTRESULT WriteCompRotation(LTRotation& rot);               // 0x10014b64
	LTRESULT WriteRaw(void* pData, uint32 len);                // 0x10014d1a
	LTRESULT WriteObject(HOBJECT hObj);                        // 0x10014a9e
	LTRESULT ResetPos();                                       // 0x10014d32
	LTRESULT GetStatus(uint32& flags);                         // 0x10014d3e
};

// 0x100260d9 — pull bytes out of a queue entry at its read cursor, and
// 0x10014cf8 — whatever the reader does afterwards. The three compressed
// codecs (0x10012d4e, 0x100132da, 0x100132fc) are shared with the client
// side of the engine (names ours).
void lmsg_AfterRead(class LMessageImpl* pMsg);
// 0x10035ed3 — the string's bytes and its length together, which the
// message writer needs and the plain getter does not give.
char* sb_GetStringDataAndLen(HSTRING hString, uint32* pLen);

// 0x10035d4b — a handle from a plain string, counting characters.
HSTRING sb_CreateStringRaw(char* pString);

// 0x10056ff4 — the empty string, which a zero-length handle becomes.
extern char g_EmptyString[];

void lmsg_WriteCompVector(class LMessageImpl* pMsg, LTVector* pVec);
void lmsg_WriteCompRotation(class LMessageImpl* pMsg, LTRotation* pRot);
void lmsg_ReadCompRotation(class LMessageImpl* pMsg, LTRotation* pRot);

// The compressed-position codec, which needs the world scale slot 3 hands
// back (0x10012f79 writes, 0x10012f9e reads; names ours).
// The compressed-position pair. A position goes out as three words, each the
// fraction of the way across the reference box on that axis; with no box
// there is nothing to be a fraction of, so a null reference reads back as the
// zero vector rather than as an error (0x10012f9e reads, and its partner
// writes).
//
// The box itself is two vectors inside whatever the reference points at, the
// low corner at +0x144 and the high at +0x150 (names ours).
class CPosRef
{
public:
	uint8    m_pad_00[0x144];
	LTVector m_vMin;                         // +0x144
	LTVector m_vMax;                         // +0x150
	// Per-axis scale, and the arithmetic says what it holds: the writer
	// sends `(pos - m_vMin) * m_vScale * 65536.0f` as a 16.8 fixed-point
	// triple, so this is the reciprocal of the box's extent (name ours).
	LTVector m_vScale;                       // +0x15c
};

void lmsg_WriteCompPos(class LMessageImpl* pMsg, LTVector* pVec, CPosRef* pRef);
void lmsg_ReadCompPos(class LMessageImpl* pMsg, LTVector* pVec, CPosRef* pRef);

// How an object handle crosses the wire. The message does not know: it hands
// itself to this and lets it decide (names ours).
class CMsgObjectIO
{
public:
	virtual LTRESULT ReadObject(class LMessageImpl* pMsg, HOBJECT* phObj) = 0;
	virtual LTRESULT WriteObject(class LMessageImpl* pMsg, HOBJECT hObj) = 0;
	virtual LTRESULT WriteFormattedHString(class LMessageImpl* pMsg,
	                                       int messageCode, va_list* pList) = 0;
	// Slot 3 hands back what the compressed-position codec measures against:
	// not a scale but a pointer, tested for null by both codecs and carrying
	// the box a position is expressed as a fraction of. It belongs to the
	// side that owns the world, not to the message (name ours).
	virtual void     GetPosRef(class CPosRef** ppRef) = 0;
};

// The abstract stream the packet writer sits on, and ServerMessage's base:
// slot 1 is the AddRef the allocator calls and slot 2 the Release every
// caller of msg_AllocPacket makes.
//
// That it is a base and not just a view is what ServerMessage's constructor
// says. Two vtables go into the object's +0x00 — 0x1004dea0 first and
// 0x1004de00 last — and the two tables share slots 1 and 2 (0x100241bd,
// 0x100241c1) while differing in slots 0 and 3 (0x100241d7/0x100241f4 and
// 0x100241cc/0x1002608f). A base-then-derived pair with two of four slots
// overridden, in other words; and the refcount clear that rides with the
// first store puts m_nRefCount on this class rather than on ServerMessage.
// Exactly two vtable stores also means exactly two vptr-carrying levels, so
// there is nothing between them.
class CAbstractIO
{
public:
	CAbstractIO() { m_nRefCount = 0; }

	virtual void _v00() = 0;                 // +0x00  0x100241d7
	virtual void AddRef();                   // +0x04  0x100241bd (streamsim.cpp)
	virtual void Release();                  // +0x08  0x100241c1 (streamsim.cpp)
	virtual void _v0c() = 0;                 // +0x0c  0x100241cc

	uint32 m_nRefCount;                      // +0x04  AddRef/Release
};


// 0x50 of a ServerMessage — packet.h's CPacketData, the growable byte
// collection the payload lives in. Only three of its functions are reached
// from this side and none is defined here: the constructor (0x1000af9a,
// which is the four-field-plus-Init shape every one of these arrays has),
// the resize (0x1000be59) and the reset-and-resize (0x1000be24). The four
// fields it owns are left on ServerMessage below rather than moved in here,
// because six other translation units already read m_pData and m_DataSize by
// those names — so this member is the vptr alone and the fields follow it
// (name ours).
class CMsgPacketData
{
public:
	CMsgPacketData();                                    // 0x1000af9a

	LTBOOL SetSize(uint32 nItems, uint32* pBank);        // 0x1000be59
	LTBOOL Init(uint32 nItems, uint32 nGrowBy);          // 0x1000be24

	uint8 m_vptr[4];
};

// 0x10058c7c — the bank the packet buffer grows out of. packet.h owns the
// declaration; it is repeated here so ServerMessage::Init's body can live in
// this header (see below) without dragging packet.h in behind it.
extern uint32 g_PacketDataBank;


// One entry in the server's outgoing queue. Polymorphic and refcounted:
// CServerMgr::StartMessage AddRefs through slot 1 and the scoped reference's
// destructor Releases through slot 2.
//
// The object is 0x64 bytes, which is what msg_AllocPacket's `new` asks for
// (0x10026020 pushes 0x64), and its constructor at 0x1002417f says how it is
// put together: a vptr at +0x00, the refcount at +0x04, a SECOND vptr at
// +0x08 (the embedded LMessageImpl's), the packet data's constructor on
// +0x50, and then Init(0x1000, 0x1000). Init's `mov %ecx,0x10(%ecx)` is
// m_Write.m_pOwner = this, which is what ties the two halves together.
//
// Everything from +0x34 down is a CPacket (see packet.h): m_Unknown34 sits
// where CPacket puts m_DataLen, m_MaxDataLen at 0x36, m_Unknown38 at m_Pos,
// m_MsgFlags at m_Flags (so MSGFLAG_READERROR and PACKETFLAG_READOVERFLOW
// are the same bit), and m_pData/m_DataSize are the CPacketData at 0x50's
// own m_pData/m_AllocSize. The two field names here are kept as they were
// because six other translation units already use them.
//
// The virtuals are declared non-pure only so `new ServerMessage` compiles in
// msg_AllocPacket; nothing here defines them.
class ServerMessage : public CAbstractIO
{
public:
	ServerMessage();                         // 0x1002417f
	virtual void _v00();                     // 0x100241f4
	// Slots 1 and 2 are the base's own AddRef and Release (0x100241bd,
	// 0x100241c1, streamsim.cpp): both vtables point at them, so they are
	// not redeclared here.
	virtual void _v0c();                     // 0x1002608f

	// 0x10010da9 — set the packet's limits and make room for them. Both
	// arguments are uint16, which the bytes say twice over: the first loads
	// as `mov 0x4(%esp),%ax` and it is the *truncated* value that is then
	// zero-extended and compared against the allocation, and the second is
	// zero-extended out of its slot at each of its two uses rather than
	// shared in a register (name ours).
	void Init(uint16 nMaxDataLen, uint16 nGrowSize);

	// Payload writers (names ours): 0x1000ec3d appends a word, 0x1001393e a
	// byte, 0x1002616a a run of bytes.
	void WriteWord(uint16 nValue);
	void WriteByte(uint8 nValue);
	void WriteData(void* pData, uint32 nBytes);

	// 0x100260d9 — the other direction: bytes out of the payload at the
	// read cursor (name ours).
	void ReadData(void* pData, uint32 nBytes);

	// 0x100138d6 is really CPacket::ReadType<uint8> (packet.cpp matches it
	// there byte for byte), so this declaration is a stand-in for the
	// inherited template: the one call site pushes a literal zero, which the
	// template's dummy-pointer argument encodes identically. Kept because
	// tcpdriver.cpp already reads a byte through it (name ours).
	uint8 ReadByte(uint8 nDefault);

	// 0x1002a1da — take the LAST unread byte and shorten the packet by it,
	// answering zero when there is none. Same template stand-in shape as
	// ReadByte above: the argument is the dummy the template's signature
	// needs and this body never reads it (name ours).
	uint8 ReadByteFromEnd(uint8 nDummy);

	LMessageImpl m_Write;                    // 0x08  what HMESSAGEWRITE points at
	uint16       m_Unknown34;                // 0x34  set to 1 before a direct send
	                                         //       (CPacket::m_DataLen)
	uint16       m_MaxDataLen;               // 0x36  Init's first argument
	uint16       m_Unknown38;                // 0x38  set to 1 before a direct send
	                                         //       (CPacket::m_Pos)
	// 0x3a — a word Init and the constructor both set to one and nothing
	// reconstructed reads. CPacket's view has it inside the padding before
	// m_Flags (name ours).
	uint16       m_Unknown3a;                // 0x3a
	class NetConnection* m_pConnection;      // 0x3c  who sent it (null for local)
	uint32       m_MsgFlags;                 // 0x40  StartMessage sets bit 0;
	                                         //       bit 1 = a read ran off the end
	// 0x44 — the link the free list at 0x10058790 holds the entry on while
	// it is spare; msg_AllocPacket unlinks it through 0x10026234.
	LTLink       m_BankLink;                 // 0x44
	// 0x50 — the CPacketData (packet.h) the two payload movers reach as one
	// object. Only its vptr is a member here; the four fields it owns follow
	// it under the names six other translation units already use, so
	// m_pData/m_DataSize are its m_pData/m_AllocSize and the last two words
	// are the spare and the grow step Init writes when no resize is needed.
	CMsgPacketData m_PacketData;             // 0x50
	uint8*       m_pData;                    // 0x54  raw packet bytes; [0] & 0x3f is the id
	uint32       m_DataSize;                 // 0x58  how much m_pData holds
	uint32       m_DataSpare;                // 0x5c  (CPacketData::m_nSpare)
	uint32       m_DataGrowBy;               // 0x60  (CPacketData::m_nGrowBy)
};

#define MSGFLAG_READERROR 2

// 0x10010da9. The packet's limits, and room for them. Both arguments are
// uint16 (see the declaration); the first is what the payload is sized to
// and the second is the array's grow step.
//
// IN THIS HEADER, AND `inline`, because lithtech.exe's /O2 build EXPANDS it.
// CNetMgrSendView::SendPacket (0x00457ab0) carries the whole body at
// 0x00457afc-0x00457b7a with no call in it:
//
//     mov $0x1,%edx            ; the three ones
//     mov 0x34(%ebp),%cx       ; m_DataLen, the caller's argument
//     mov %dx,0x34(%eax)       ; m_Unknown34 = 1
//     add $0x32,%cx            ; ... + 0x32
//     mov %dx,0x38(%eax)       ; m_Unknown38 = 1
//     mov %cx,0x36(%eax)       ; m_MaxDataLen = nMaxDataLen
//     mov %dx,0x3a(%eax)       ; m_Unknown3a  = 1
//     movl $0x0,0x40(%eax)     ; m_MsgFlags   = 0
//     mov %eax,0x10(%eax)      ; m_Write.m_pOwner = this
//     mov 0x58(%eax),%ecx      ; m_DataSize
//     and $0xffff,%esi / cmp %ecx,%esi / jbe ...
//     lea 0x50(%eax),%edi      ; &m_PacketData, the SetSize/Init arm
//     ... / jmp
//     movl $0x1000,0x60(%eax)  ; else m_DataGrowBy = nGrowSize
//
// server.dll's /O1 build still calls it out of line at 0x10010da9 — /Os
// declines a ninety-byte body with two calls, exactly as it declines
// CMessageRef::Init's thirty-seven below — so servermsg.obj keeps emitting
// the standalone COMDAT and nothing that matches today is put at risk.
// VC6's inliner is forward-only, so the body has to be above its callers:
// here, which is above every translation unit that includes this header.
//
// The interesting half is the last two lines: an allocation that is already
// at least as big as the new limit is not touched at all, and only the grow
// step is updated. Anything larger throws the storage away — SetSize(0) —
// and starts again at the new size rather than reserving over the old
// contents.
//
// `m_Write.m_pOwner = this` is the store that ties the two halves of the
// object together (`mov %ecx,0x10(%ecx)` — LMessageImpl at +0x08, its owner
// slot at +0x08 of that).
//
// BYTE-IDENTICAL at /O1 (0x10010da9, 90 bytes — the function enumeration's
// two 45-byte rows are one function; 0x10010dd6 is mid-instruction). The
// shipped build puts `cmp 0x58(%ecx),%esi` immediately before the `jbe`,
// after all six field stores; a plain read of m_DataSize lets VC6 hoist the
// load up beside the `movzwl` that makes %esi. Reading it as a volatile
// (the one-site cast, as elsewhere in this tree) pins the load after the
// stores — which is also where lithtech.exe's /O2 expansion has it
// (`mov 0x58(%eax),%ecx` after `mov %eax,0x10(%eax)`, above).
//
// Ruled out (all emit the hoisted schedule): `m_DataSize < nMaxDataLen`
// (swaps the cmp operands too), `>=` with the arms swapped, a uint32 local
// for either operand, the owner store through a `pThis` local, reading the
// bound through a pointer to the packet data, and all four held compiler
// revisions (8168, 8447, 8804, 8966).
inline void ServerMessage::Init(uint16 nMaxDataLen, uint16 nGrowSize)
{
	m_MsgFlags   = 0;
	m_Unknown34  = 1;
	m_MaxDataLen = nMaxDataLen;
	m_Unknown38  = 1;
	m_Unknown3a  = 1;

	m_Write.m_pOwner = this;

	if (nMaxDataLen > *(volatile uint32*)&m_DataSize)
	{
		m_PacketData.SetSize(0, &g_PacketDataBank);
		m_PacketData.Init(nMaxDataLen, nGrowSize);
	}
	else
	{
		m_DataGrowBy = nGrowSize;
	}
}

// EndMessage2 and EndMessage dispatch the send through CLTServer's own
// vtable — the big anchored ILTServer table. Only slots 88-94
// are reached; this view names them so the vtable-less reconstruction can
// still emit the calls (`((ILTServerSendView*)this)->...`).
class ILTServerSendView
{
public:
	virtual void _s00()=0; virtual void _s01()=0; virtual void _s02()=0; virtual void _s03()=0; virtual void _s04()=0; virtual void _s05()=0; virtual void _s06()=0; virtual void _s07()=0;
	virtual void _s08()=0; virtual void _s09()=0; virtual void _s10()=0; virtual void _s11()=0; virtual void _s12()=0; virtual void _s13()=0; virtual void _s14()=0; virtual void _s15()=0;
	virtual void _s16()=0; virtual void _s17()=0; virtual void _s18()=0; virtual void _s19()=0; virtual void _s20()=0; virtual void _s21()=0; virtual void _s22()=0; virtual void _s23()=0;
	virtual void _s24()=0; virtual void _s25()=0; virtual void _s26()=0; virtual void _s27()=0; virtual void _s28()=0; virtual void _s29()=0; virtual void _s30()=0; virtual void _s31()=0;
	virtual void _s32()=0; virtual void _s33()=0; virtual void _s34()=0; virtual void _s35()=0; virtual void _s36()=0; virtual void _s37()=0; virtual void _s38()=0; virtual void _s39()=0;
	virtual void _s40()=0; virtual void _s41()=0; virtual void _s42()=0; virtual void _s43()=0; virtual void _s44()=0; virtual void _s45()=0; virtual void _s46()=0; virtual void _s47()=0;
	virtual void _s48()=0; virtual void _s49()=0; virtual void _s50()=0; virtual void _s51()=0; virtual void _s52()=0; virtual void _s53()=0; virtual void _s54()=0; virtual void _s55()=0;
	virtual void _s56()=0; virtual void _s57()=0; virtual void _s58()=0; virtual void _s59()=0; virtual void _s60()=0; virtual void _s61()=0; virtual void _s62()=0; virtual void _s63()=0;
	virtual void _s64()=0; virtual void _s65()=0; virtual void _s66()=0; virtual void _s67()=0; virtual void _s68()=0; virtual void _s69()=0; virtual void _s70()=0; virtual void _s71()=0;
	virtual void _s72()=0; virtual void _s73()=0; virtual void _s74()=0; virtual void _s75()=0; virtual void _s76()=0; virtual void _s77()=0; virtual void _s78()=0; virtual void _s79()=0;
	virtual void _s80()=0; virtual void _s81()=0; virtual void _s82()=0; virtual void _s83()=0; virtual void _s84()=0; virtual void _s85()=0; virtual void _s86()=0; virtual void _s87()=0;
	virtual LTRESULT EndMessage2(IMessageWrite &msg, uint32 flags)=0;                    // 0x160
	virtual LTRESULT EndMessage(IMessageWrite &msg)=0;                                   // 0x164
	virtual LTRESULT SetObjectSFXMessage(HOBJECT hObject, IMessageWrite &msg)=0;         // 0x168
	virtual LTRESULT SendToObject(IMessageWrite &msg, uint32 msgID, HOBJECT hSender,
	                              HOBJECT hSendTo, uint32 flags)=0;                      // 0x16c
	virtual LTRESULT SendToServer(IMessageWrite &msg, uint32 msgID, HOBJECT hSender,
	                              uint32 flags)=0;                                       // 0x170
	virtual LTRESULT SendToClient(IMessageWrite &msg, uint8 msgID, HCLIENT hSendTo,
	                              uint32 flags)=0;                                       // 0x174
	virtual LTRESULT SendSFXMessage(IMessageWrite &msg, LTVector &pos, uint32 flags)=0;  // 0x178
};

// A scoped reference to a server message; four bytes, and the destructor's
// release is what balances StartMessage's AddRef on the pointer it returns.
class CMessageRef
{
public:
	CMessageRef() { m_pMsg = 0; }
	~CMessageRef() { if (m_pMsg) m_pMsg->Release(); }

	ServerMessage* Init(void* pPacket);      // 0x1000ebab — returns m_pMsg

	ServerMessage* m_pMsg;
};

// 0x1000ebab — the scoped reference's Init, IN THIS HEADER and `inline`,
// which is where the original had it: lithtech.exe's /O2 build expands it
// into every caller in every translation unit that takes one.
// BroadcastFileMessage, xfer_AddFile, xfer_SendPendingFiles,
// xfer_FinishTransfer, xfer_SendNextChunk, sm_ClientEnterWorld2,
// sm_UpdateClientState2 and sm_WriteClientUpdate all open with
// `call msg_AllocPacket; mov (%eax),%reg; test %reg,%reg; call *0x4(...)` —
// the body with the freshly-constructed null propagated through the first
// guard — and CNetMgr::Update (0x00455790) is the same story.
//
// /Os declines a thirty-seven-byte body with two conditional virtual calls,
// which is why server.dll's /O1 image still calls 0x1000ebab out of line and
// still gets the standalone COMDAT emitted from every object that calls it.
// That is measured, not assumed: servermessage.cpp and smgr.cpp have carried
// this body `inline` for some time and both objects still define
// ?Init@CMessageRef@@ and still match 0x1000ebab.
//
// VC6's inliner is forward-only, so the definition has to precede its
// callers — here, above every including translation unit.
//
// Takes the packet the message rides on and reads the message out of it;
// takes a reference on what arrives and drops the one it was holding, in
// that order, so assigning a reference to itself is safe.
inline ServerMessage* CMessageRef::Init(void* pPacket)
{
	if (m_pMsg)
		m_pMsg->Release();

	m_pMsg = *(ServerMessage**)pPacket;

	if (m_pMsg)
		m_pMsg->AddRef();

	return m_pMsg;
}

// A write position into a message's packet: the same counted reference the
// scoped one is, but taking the message directly rather than the packet it
// rides on (Init 0x10023e86; names ours).
class CMsgCursor
{
public:
	CMsgCursor() { m_pMsg = 0; }
	~CMsgCursor() { if (m_pMsg) m_pMsg->Release(); }

	ServerMessage* Init(ServerMessage* pMsg);   // 0x10023e86

	ServerMessage* m_pMsg;
};

// Everything sm_WriteClientUpdate's writers pass around (names ours; the
// constructor is 0x1002860b and the destructor 0x1002861f releases both
// message refs).
class CUpdateContext
{
public:
	CUpdateContext();
	~CUpdateContext();

	class CServerMgr* m_pMgr;                // +0x00
	class Client*     m_pClient;             // +0x04
	CMessageRef m_Msg1;                      // +0x08  the guaranteed stream
	CMessageRef m_Msg2;                      // +0x0c  the unguaranteed stream
	// One array, not two members: the construction and the teardown walk it
	// as a run of two, which is what the shipped stos and the counted
	// destructor loop show. Index 0 writes into m_Msg2 and index 1 into
	// m_Msg1.
	CMsgCursor  m_Cursors[2];                // +0x10
	uint32      m_Unknown18;                 // +0x18
	uint32      m_bLocal;                    // +0x1c  ClientFlags bit 9
};

// lithtech.exe has no out-of-line constructor or destructor for the context
// (neither has a /O2 address): sm_WriteClientUpdate (0x00465610), their only
// user, opens with the four references zeroed in place (0x0046562b..
// 0x00465639) and closes with the cursor array walked backwards and the two
// message refs released (0x00465dbc..0x00465df1). So in the /O2 build they
// are inline bodies; server.dll keeps the /O1 copies at 0x1002860b /
// 0x1002861f (servermessage.cpp).
#ifndef LT_SERVER_BUILD
inline CUpdateContext::CUpdateContext()
{
}

inline CUpdateContext::~CUpdateContext()
{
}
#endif

// 0x10026020 — take a spare ServerMessage off the free list at 0x10058790 or
// make a new one, Init it, hand it back through the out-parameter and AddRef
// it. The return value is the ADDRESS of that out-parameter, not the object:
// CMessageRef::Init (0x1000ebab) dereferences its argument, which is what
// makes `msg.Init(msg_AllocPacket(&pIO, ...))` work. The out-parameter is
// typed CAbstractIO** because that is the only thing every caller does with
// it (checks it and Releases it); the object behind it is a ServerMessage.
void* msg_AllocPacket(class CAbstractIO** ppIO, uint32 blockSize, uint32 growSize);

#define MT_TOOBJECT   0
#define MT_TOCLIENT   1
#define MT_SFX        2
#define MT_INSTANTSFX 3
#define MT_TOSERVER   4

class CServerMgr;
class SPhysicsLT;
class SharedCommonLT;
class ClassPropInfo;


class CLTServer
{
public:
	LTRESULT WriteToMessageByte(HMESSAGEWRITE_I hMessage, uint8 val);
	LTRESULT WriteToMessageWord(HMESSAGEWRITE_I hMessage, uint16 val);
	LTRESULT WriteToMessageDWord(HMESSAGEWRITE_I hMessage, uint32 val);
	LTRESULT WriteToMessageFloat(HMESSAGEWRITE_I hMessage, float val);
	LTRESULT WriteToMessageString(HMESSAGEWRITE_I hMessage, char* pStr);
	LTRESULT WriteToMessageHString(HMESSAGEWRITE_I hMessage, HSTRING hStr);
	LTRESULT WriteToMessageHMessageWrite(HMESSAGEWRITE_I hMessage, void* hData);
	LTRESULT WriteToMessageHMessageRead(HMESSAGEWRITE_I hMessage, void* hData);
	LTRESULT WriteToMessageVector(HMESSAGEWRITE_I hMessage, LTVector* pVal);
	LTRESULT WriteToMessageCompVector(HMESSAGEWRITE_I hMessage, LTVector* pVal);
	LTRESULT WriteToMessageCompPosition(HMESSAGEWRITE_I hMessage, LTVector* pVal);
	LTRESULT WriteToMessageRotation(HMESSAGEWRITE_I hMessage, LTRotation* pVal);
	LTRESULT WriteToMessageFormattedHString(HMESSAGEWRITE_I hMessage, char* pMsg, ...);
	LTRESULT WriteToMessageObject(HMESSAGEWRITE_I hMessage, HOBJECT hObj);

	float    ReadFromMessageFloat(HMESSAGEREAD_I hMessage);
	uint8    ReadFromMessageByte(HMESSAGEREAD_I hMessage);
	uint16   ReadFromMessageWord(HMESSAGEREAD_I hMessage);
	uint32   ReadFromMessageDWord(HMESSAGEREAD_I hMessage);
	HOBJECT  ReadFromMessageObject(HMESSAGEREAD_I hMessage);
	HSTRING  ReadFromMessageHString(HMESSAGEREAD_I hMessage);
	void*    ReadFromMessageHMessageRead(HMESSAGEREAD_I hMessage);
	LTRESULT ReadFromLoadSaveMessageObject(HMESSAGEREAD_I hMessage, HOBJECT* pObj);
	void     ReadFromMessageVector(HMESSAGEREAD_I hMessage, LTVector* pVal);
	void     ReadFromMessageCompVector(HMESSAGEREAD_I hMessage, LTVector* pVal);
	void     ReadFromMessageCompPosition(HMESSAGEREAD_I hMessage, LTVector* pVal);
	void     ReadFromMessageRotation(HMESSAGEREAD_I hMessage, LTRotation* pVal);
	void     EndHMessageRead(HMESSAGEREAD_I hMessage);
	void     EndHMessageWrite(HMESSAGEWRITE_I hMessage);
	void     ResetRead(HMESSAGEREAD_I hMessage);

	// The class-reflection block that opens the TU (0x1002e074-0x1002e34b).
	// (ClassPropInfo is iltserver.h's; the TU includes it.)
	// HCLASS is a CClassInfo*; the ClassDef/ClassPropInfo layouts are the
	// SDK's own (ltserverobj.h / iltserver.h).
	LTRESULT GetNumClassProps(const HCLASS hClass, uint32 &count);
	LTRESULT GetClassProp(const HCLASS hClass, const uint32 iProp, ClassPropInfo &info);
	LTRESULT GetClassName(const HCLASS hClass, char *pName, uint32 maxNameBytes);
	LTRESULT GetClientPing(HCLIENT hClient, float &ping);
	LTRESULT GetNetFlags(HOBJECT hObj, uint32 &flags);
	LTRESULT SetNetFlags(HOBJECT hObj, uint32 flags);

	// The object finders (0x1002e349-0x1002e678).
	LTRESULT GetHPolyObject(const HPOLY hPoly, HOBJECT &hObject);
	LTRESULT FindNamedObjects(char *pName, BaseObjArray<HOBJECT> &objArray, uint32 *nTotalFound);
	LTRESULT FindWorldModelObjectIntersections(HOBJECT hWorldModel, LTVector vNewPos,
	                                           LTRotation rNewRot, BaseObjArray<HOBJECT> &objArray);

	// The world box and the threaded file loader (0x1002e67b-0x1002e8a0).
	LTRESULT GetWorldBox(LTVector &min, LTVector &max);
	LTRESULT ThreadLoadFile(char *pFilename, uint32 type);
	LTRESULT UnloadFile(char *pFilename, uint32 type);
	LTRESULT OpenFile(char *pFilename, ILTStream **pStream);

	// The per-type halves ThreadLoadFile/UnloadFile dispatch to — plain
	// members (thiscall, callee-clean, which is what the dispatch bytes say).
	LTRESULT ThreadLoadModel(char *pFilename);    // 0x1002ec9a
	LTRESULT ThreadLoadTexture(char *pFilename);  // 0x1002ed9c
	LTRESULT UnloadModel(char *pFilename);        // 0x1002ee11
	LTRESULT UnloadTexture(char *pFilename);      // 0x1002ee9d

	LTRESULT GetClientData(HCLIENT hClient, void *&pData, uint32 &nLength);
	LTRESULT SetObjectFilenames(HOBJECT hObj, char *pFilename, char *pSkinName);
	LTRESULT GetObjectName(HOBJECT hObject, char *pName, uint32 nameBufSize);
	LTRESULT GetPolyTextureFlags(HPOLY hPoly, uint32 *pFlags);
	LTRESULT GetModelAnimUserDims(HOBJECT hObject, LTVector *pDims, HMODELANIM hAnim);

	// The senders (0x1002f43a-0x1002f8b0): EndMessage2 dispatches on the
	// message type through the vtable (ILTServerSendView); the
	// implementations are these plain members.
	LTRESULT EndMessage2(HMESSAGEWRITE_I hMessage, uint32 flags);
	LTRESULT EndMessage(HMESSAGEWRITE_I hMessage);
	LTRESULT SetObjectSFXMessage(HOBJECT hObject, IMessageWrite &msg);
	LTRESULT SendToObject(IMessageWrite &msg, uint32 msgID, HOBJECT hSender,
	                      HOBJECT hSendTo, uint32 flags);
	LTRESULT SendToServer(IMessageWrite &msg, uint32 msgID, HOBJECT hSender, uint32 flags);
	LTRESULT SendToClient(IMessageWrite &msg, uint8 msgID, HCLIENT hSendTo, uint32 flags);
	LTRESULT SendSFXMessage(IMessageWrite &msg, LTVector &pos, uint32 flags);

	// 0x1002eceb — write (type byte, id word) into a fresh packet and send it
	// to every client (skipping flag-2 clients unless bToAll) as message
	// nMsgID; the loader halves broadcast through it (name ours).
	void BroadcastFileMessage(uint32 nFileType, uint32 nMsgID, uint16 nFileID, int bToAll);

	// The old flat model-state block and friends (0x1002e8a0-0x1002eb36).
	LTRESULT CopyFile(const char *pszSourceFile, const char *pszDestFile);
	void SetModelAnimation(HOBJECT hObj, HMODELANIM hAnim);
	HMODELANIM GetModelAnimation(HOBJECT hObj);
	void SetModelLooping(HOBJECT hObj, LTBOOL bLoop);
	LTBOOL GetModelLooping(HOBJECT hObj);
	LTRESULT ResetModelAnimation(HOBJECT hObj);
	uint32 GetModelPlaybackState(HOBJECT hObj);
	HMODELANIM GetAnimIndex(HOBJECT hObj, char *pAnimName);
	void CPrint(char *pMsg, ...);
	uint32 GetPointContainers(LTVector *pPoint, HOBJECT *pList, uint32 maxListSize);
	LTBOOL GetContainerCode(HOBJECT hObj, uint16 *pCode);
	HSTRING FormatString(int messageCode, ...);

	// The message starters (0x1002f25f-0x1002f415).
	HMESSAGEWRITE StartSpecialEffectMessage(LPBASECLASS pObject);
	HMESSAGEWRITE StartInstantSpecialEffectMessage(LTVector* pPos);
	HMESSAGEWRITE StartMessageToObject(LPBASECLASS pSender, HOBJECT hSendTo, uint32 messageID);
	LTRESULT      StartMessageToServer(LPBASECLASS pSender, uint32 messageID, HMESSAGEWRITE* hWrite);
	HMESSAGEWRITE StartMessage(HCLIENT hSendTo, uint8 messageID);

	// iltserver.h's force accessors take LTVector* — hence the null checks —
	// unlike iltphysics.h's reference-taking pair (0x1002f0e1, 0x1002f12a).
	LTRESULT GetGlobalForce(LTVector* pVec);
	LTRESULT SetGlobalForce(LTVector* pVec);

	// The old-API wrappers (0x1002ef95-0x1002f24a): ILTServer's flat methods,
	// each forwarding to the interface that superseded it. The vtable slots
	// they call through m_pPhysicsLT are SPhysicsLT's in declaration order,
	// which is what pinned these two fields.
	LTRESULT SetFrictionCoefficient(HOBJECT hObj, float coeff);
	float    GetForceIgnoreLimit(HOBJECT hObj, float &limit);
	void     SetForceIgnoreLimit(HOBJECT hObj, float limit);
	LTRESULT GetVelocity(HOBJECT hObj, LTVector *pVel);
	LTRESULT SetVelocity(HOBJECT hObj, LTVector *pVel);
	LTRESULT GetAcceleration(HOBJECT hObj, LTVector *pAccel);
	LTRESULT SetAcceleration(HOBJECT hObj, LTVector *pAccel);
	float    GetObjectMass(HOBJECT hObj);
	void     SetObjectMass(HOBJECT hObj, float mass);
	void     GetObjectDims(HOBJECT hObj, LTVector *pNewDims);
	LTRESULT SetObjectDims(HOBJECT hObj, LTVector *pNewDims);
	LTRESULT SetObjectDims2(HOBJECT hObj, LTVector *pNewDims);
	LTRESULT MoveObject(HOBJECT hObj, LTVector *pNewPos);
	LTRESULT GetStandingOn(HOBJECT hObj, CollisionInfo *pInfo);
	uint32   GetObjectFlags(HOBJECT hObj);
	void     SetObjectFlags(HOBJECT hObj, uint32 flags);
	char*    GetObjectName(HOBJECT hObject);

	// The ILTCSBase string / console / time block (0x1002eb37-0x1002ebea).
	HSTRING  CopyString(HSTRING hString);
	HSTRING  CreateString(char *pString);
	void     FreeString(HSTRING hString);
	LTBOOL   CompareStrings(HSTRING hString1, HSTRING hString2);
	LTBOOL   CompareStringsUpper(HSTRING hString1, HSTRING hString2);
	char*    GetStringData(HSTRING hString);
	float    GetVarValueFloat(HCONSOLEVAR hVar);
	char*    GetVarValueString(HCONSOLEVAR hVar);
	float    GetTime();
	float    GetFrameTime();
	LTRESULT RemoveObject(HOBJECT hObj);
	LTRESULT FreeUnusedModels();
	LTRESULT SendTo(const void *pData, uint32 len, const char *sAddr, uint32 port);

public:
	// ILTCSBase's own six interface pointers open the object, and every
	// ILTServer method that is NOT virtual is a function-pointer member
	// after them — which is what makes the table below a layout, not a
	// vtable. si_SetupInterface (smgr.cpp, 0x1002fd9b) fills all of it in
	// one straight run of stores; the declaration order here is iltserver.h's
	// and pins each offset, so the shipped store offsets are the check on it.
	uint8           m_pad_000[0x008];
	SharedCommonLT* m_pCommonLT;             // 0x008
	SPhysicsLT*     m_pPhysicsLT;            // 0x00c
	class ILTModel*     m_pModelLT;          // 0x010
	class ILTTransform* m_pTransformLT;      // 0x014
	class ILTLightAnim* m_pLightAnimLT;      // 0x018
	class ILTSoundMgr*  m_pSoundMgr;         // 0x01c

	LTRESULT (*GetGameInfo)(void **ppData, uint32 *pLen);                                    // 0x020
	HCLASS (*GetClass)(char *pName);                                                         // 0x024
	LTRESULT (*GetStaticObject)(HCLASS hClass, HOBJECT *obj);                                // 0x028
	HCLASS (*GetObjectClass)(HOBJECT hObject);                                               // 0x02c
	LTBOOL (*IsKindOf)(HCLASS hClass, HCLASS hTest);                                         // 0x030
	LPBASECLASS (*CreateObject)(HCLASS hClass, ObjectCreateStruct *pStruct);                 // 0x034
	LPBASECLASS (*CreateObjectProps)(HCLASS hClass, ObjectCreateStruct *pStruct, char *pszProps ); // 0x038
	uint32 (*GetServerFlags)();                                                              // 0x03c
	uint32 (*SetServerFlags)(uint32 flags);                                                  // 0x040
	LTRESULT (*CacheFile)(uint32 fileType, char *pFilename);                                 // 0x044
	LTRESULT (*GetNextPoly)(HPOLY *hPoly);                                                   // 0x048
	void (*StartCounter)(LTCounter *pCounter);                                               // 0x04c
	uint32 (*EndCounter)(LTCounter *pCounter);                                               // 0x050
	LTFLOAT (*Random)(LTFLOAT min, LTFLOAT max);                                             // 0x054
	int (*IntRandom)(int min, int max);                                                      // 0x058
	LTFLOAT (*RandomScale)(LTFLOAT scale);                                                   // 0x05c
	void (*BPrint)(char *pMsg, ...);                                                         // 0x060
	void (*DebugOut)( char *pMsg, ... );                                                     // 0x064
	LTRESULT (*GetSkyDef)(SkyDef *pDef);                                                     // 0x068
	LTRESULT (*SetSkyDef)(SkyDef *pDef);                                                     // 0x06c
	LTRESULT (*AddObjectToSky)(HOBJECT hObj, uint32 index);                                  // 0x070
	LTRESULT (*RemoveObjectFromSky)(HOBJECT hObj);                                           // 0x074
	LTRESULT (*GetGlobalLightObject)(HOBJECT *hObj);                                         // 0x078
	LTRESULT (*SetGlobalLightObject)(HOBJECT hObj);                                          // 0x07c
	LTRESULT (*GetPortalFlags)(char *pPortalName, uint32 *pFlags);                           // 0x080
	LTRESULT (*SetPortalFlags)(char *pPortalName, uint32 flags);                             // 0x084
	LTBOOL (*IntersectSegment)(IntersectQuery *pQuery, IntersectInfo *pInfo);                // 0x088
	LTBOOL (*CastRay)(IntersectQuery *pQuery, IntersectInfo *pInfo);                         // 0x08c
	ObjectList* (*GetPointAreas)(LTVector *pPoint);                                          // 0x090
	ObjectList* (*GetBoxIntersecters)(LTVector *pMin, LTVector *pMax);                       // 0x094
	LTBOOL (*GetPointShade)(LTVector *pPoint, LTVector *pColor);                             // 0x098
	ObjectList* (*FindObjectsTouchingSphere)(LTVector *pPosition, float radius);             // 0x09c
	void (*RelinquishList)(ObjectList *pList);                                               // 0x0a0
	ObjectList* (*CreateObjectList)();                                                       // 0x0a4
	ObjectLink* (*AddObjectToList)(ObjectList *pList, HOBJECT hObj);                         // 0x0a8
	void (*RemoveObjectFromList)(ObjectList *pList, HOBJECT hObj);                           // 0x0ac
	LTRESULT (*AttachClient)(HCLIENT hParent, HCLIENT hChild);                               // 0x0b0
	LTRESULT (*DetachClient)(HCLIENT hClient);                                               // 0x0b4
	HCLIENT (*GetNextClient)(HCLIENT hPrev);                                                 // 0x0b8
	HCLIENTREF (*GetNextClientRef)(HCLIENTREF hRef);                                         // 0x0bc
	uint32 (*GetClientRefInfoFlags)(HCLIENTREF hClient);                                     // 0x0c0
	LTBOOL (*GetClientRefName)(HCLIENTREF hClient, char *pName, int maxLen);                 // 0x0c4
	HOBJECT (*GetClientRefObject)(HCLIENTREF hClient);                                       // 0x0c8
	uint32 (*GetClientID)(HCLIENT hClient);                                                  // 0x0cc
	LTBOOL (*GetClientName)(HCLIENT hClient, char *pName, int maxLen);                       // 0x0d0
	void (*SetClientInfoFlags)(HCLIENT hClient, uint32 dwClientFlags );                      // 0x0d4
	uint32 (*GetClientInfoFlags)(HCLIENT hClient);                                           // 0x0d8
	void (*SetClientUserData)(HCLIENT hClient, void *pData);                                 // 0x0dc
	void* (*GetClientUserData)(HCLIENT hClient);                                             // 0x0e0
	void (*KickClient)(HCLIENT hClient, uint32 nReasonCode = 0);                             // 0x0e4
	LTRESULT (*SetClientViewPos)(HCLIENT hClient, LTVector *pPos);                           // 0x0e8
	void (*RunGameConString)(char *pString);                                                 // 0x0ec
	void (*SetGameConVar)(char *pName, char *pVal);                                          // 0x0f0
	HCONVAR (*GetGameConVar)(char *pName);                                                   // 0x0f4
	LTRESULT (*GetPropString)(char *pPropName, char *pRet, int maxLen);                      // 0x0f8
	LTRESULT (*GetPropVector)(char *pPropName, LTVector *pRet);                              // 0x0fc
	LTRESULT (*GetPropColor)(char *pPropName, LTVector *pRet);                               // 0x100
	LTRESULT (*GetPropReal)(char *pPropName, float *pRet);                                   // 0x104
	LTRESULT (*GetPropFlags)(char *pPropName, uint32 *pRet);                                 // 0x108
	LTRESULT (*GetPropBool)(char *pPropName, LTBOOL *pRet);                                  // 0x10c
	LTRESULT (*GetPropLongInt)(char *pPropName, long *pRet);                                 // 0x110
	LTRESULT (*GetPropRotation)(char *pPropName, LTRotation *pRet);                          // 0x114
	LTRESULT (*GetPropRotationEuler)(char *pPropName, LTVector *pAngles);                    // 0x118
	LTRESULT (*GetPropGeneric)(char *pPropName, GenericProp *pProp);                         // 0x11c
	LTRESULT (*DoesPropExist)(char *pPropName, int *pPropType);                              // 0x120
	HOBJECT (*ObjectToHandle)(LPBASECLASS pObject);                                          // 0x124
	LPBASECLASS (*HandleToObject)(HOBJECT hObject);                                          // 0x128
	LTBOOL (*IsCommandOn)(HCLIENT hClient, int command);                                     // 0x12c
	LTBOOL (*UpperStrcmp)(char *pStr1, char *pStr2);                                         // 0x130
	int (*Parse)(char *pCommand, char **pNewCommandPos, char *argBuffer, char **argPointers, int *nArgs); // 0x134
	LTRESULT (*PlaySound)( PlaySoundInfo *pPlaySoundInfo );                                  // 0x138
	LTRESULT (*GetSoundDuration)( HLTSOUND hSound, LTFLOAT *fDuration );                     // 0x13c
	LTRESULT (*IsSoundDone)( HLTSOUND hSound, LTBOOL *bDone );                               // 0x140
	LTRESULT (*KillSound)( HLTSOUND hSound );                                                // 0x144
	LTRESULT (*KillSoundLoop)( HLTSOUND hSound );                                            // 0x148
	LTRESULT (*CreateInterObjectLink)(HOBJECT hOwner, HOBJECT hLinked);                      // 0x14c
	void (*BreakInterObjectLink)(HOBJECT hOwner, HOBJECT hLinked);                           // 0x150
	LTRESULT (*CreateAttachment)(HOBJECT hParent, HOBJECT hChild, char *pSocketName, LTVector *pOffset, LTRotation *pRotationOffset, HATTACHMENT *pAttachment); // 0x154
	LTRESULT (*RemoveAttachment)(HATTACHMENT hAttachment);                                   // 0x158
	LTRESULT (*FindAttachment)(HOBJECT hParent, HOBJECT hChild, HATTACHMENT *hAttachment);   // 0x15c
	LTBOOL (*GetObjectColor)(HOBJECT hObject, float *r, float *g, float *b, float *a);       // 0x160
	LTBOOL (*SetObjectColor)(HOBJECT hObject, float r, float g, float b, float a);           // 0x164
	uint32 (*GetObjectUserFlags)(HOBJECT hObj);                                              // 0x168
	LTRESULT (*SetObjectUserFlags)(HOBJECT hObj, uint32 flags);                              // 0x16c
	HOBJECT (*GetNextObject)(HOBJECT hObj);                                                  // 0x170
	HOBJECT (*GetNextInactiveObject)(HOBJECT hObj);                                          // 0x174
	void (*SetNextUpdate)(HOBJECT hObj, LTFLOAT nextUpdate);                                 // 0x178
	void (*SetDeactivationTime)(HOBJECT hObj, LTFLOAT fDeactivationTime);                    // 0x17c
	void (*PingObjects)(HOBJECT hObj);                                                       // 0x180
	void (*GetObjectPos)(HOBJECT hObj, LTVector *pos);                                       // 0x184
	void (*SetObjectPos)(HOBJECT hObj, LTVector *pos);                                       // 0x188
	void (*ScaleObject)(HOBJECT hObj, LTVector *pNewScale);                                  // 0x18c
	LTRESULT (*GetObjectScale)(HOBJECT hObj, LTVector *pScale);                              // 0x190
	LTRESULT (*TeleportObject)(HOBJECT hObj, LTVector *pNewPos);                             // 0x194
	LTRESULT (*GetLastCollision)(CollisionInfo *pInfo);                                      // 0x198
	LTRESULT (*GetObjectRotation)(HOBJECT hObj, LTRotation *pRotation);                      // 0x19c
	LTRESULT (*SetObjectRotation)(HOBJECT hObj, LTRotation *pRotation);                      // 0x1a0
	LTRESULT (*RotateObject)(HOBJECT hObj, LTRotation *pRotation);                           // 0x1a4
	void (*TiltToPlane)(HOBJECT hObj, LTVector *pNormal);                                    // 0x1a8
	void (*SetObjectState)(HOBJECT hObj, int state);                                         // 0x1ac
	int (*GetObjectState)(HOBJECT hObj);                                                     // 0x1b0
	void (*SetBlockingPriority)(HOBJECT hObj, uint8 pri);                                    // 0x1b4
	uint8 (*GetBlockingPriority)(HOBJECT hObj);                                              // 0x1b8
	LTRESULT (*ClipSprite)(HOBJECT hObj, HPOLY hPoly);                                       // 0x1bc
	void (*GetLightColor)(HOBJECT hObj, float *r, float *g, float *b);                       // 0x1c0
	void (*SetLightColor)(HOBJECT hObj, float r, float g, float b);                          // 0x1c4
	float (*GetLightRadius)(HOBJECT hObj);                                                   // 0x1c8
	void (*SetLightRadius)(HOBJECT hObj, float radius);                                      // 0x1cc
	LTRESULT (*GetNextModelNode)(HOBJECT hObject, HMODELNODE hNode, HMODELNODE *pNext);      // 0x1d0
	LTRESULT (*GetModelNodeName)(HOBJECT hObject, HMODELNODE hNode, char *pName, uint32 maxLen); // 0x1d4
	LTRESULT (*GetModelCommandString)(HOBJECT hObj, char *pStr, uint32 maxLen);              // 0x1d8
	LTRESULT (*SetModelNodeHideStatus)(HOBJECT hObj, char *pNodeName, LTBOOL bHidden);       // 0x1dc
	LTRESULT (*GetModelNodeHideStatus)(HOBJECT hObj, char *pNodeName, /* out */LTBOOL *bHidden); // 0x1e0
	const char *(*GetAnimName)(HOBJECT hObject, HMODELANIM hAnim);                           // 0x1e4
	void (*SetModelPlaying)(HOBJECT hObj, LTBOOL bPlaying);                                  // 0x1e8
	LTBOOL (*GetModelPlaying)(HOBJECT hObj);                                                 // 0x1ec
	LTBOOL (*GetModelFilenames)(HOBJECT hObj, char *pFilename, int fileBufLen, char *pSkinName, int skinBufLen); // 0x1f0
	uint32 (*GetObjectContainers)(HOBJECT hObj, HOBJECT *pContainerList, uint32 *pFlagList, uint32 maxListSize); // 0x1f4
	uint32 (*GetContainedObjects)(HOBJECT hContainer, HOBJECT *pObjectList, uint32 *pFlagList, uint32 maxListSize); // 0x1f8
	LTRESULT (*SaveObjects)(char *pszSaveFileName, ObjectList *pList, uint32 dwParam, uint32 flags); // 0x1fc
	LTRESULT (*RestoreObjects)( char *pszRestoreFileName, uint32 dwParam, uint32 flags );    // 0x200
	LTRESULT (*LoadWorld)(char *pszWorldFileName, uint32 flags);                             // 0x204
	LTRESULT (*RunWorld)();                                                                  // 0x208
	LTRESULT (*UpdateSessionName)(const char* sName);                                        // 0x20c
	LTRESULT (*GetSessionName)(char* sName, uint32 dwBufferSize);                            // 0x210
	LTRESULT (*SendToServerApp)( char *pMsg, uint32 nLen );                                  // 0x214
	LTRESULT (*GetTcpIpAddress)(char* sAddress, uint32 dwBufferSize, uint16 &hostPort);      // 0x218
	FileEntry* (*GetFileList)(char *pDirName);                                               // 0x21c
	void (*FreeFileList)(FileEntry *pHead);                                                  // 0x220
	CServerMgr*     m_pServerMgr;            // 0x224
};

#endif
