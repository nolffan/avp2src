// *********************************************************************** //
//
//  MODULE:   W12_BIGTWO (.CPP)   (scratch file; name ours)
//
//  PURPOSE:  The two large client-startup functions whose enumeration sizes
//            are wrong, reconstructed from the retail lithtech.exe:
//
//              0x0040fd90 .. 0x00410350   CClientMgr::StartGame     1472 B
//              0x00413d30 .. 0x004141f3   CClientShell::DoLoadWorld 1219 B
//
//            plus the twelve-byte file-static forwarder at 0x00413d20 whose
//            ADDRESS DoLoadWorld stores into the world-load request, and
//            which therefore cannot be left out of the same object.
//
//  EXTENTS -- RE-VERIFIED HERE, NOT TAKEN ON TRUST
//  -----------------------------------------------
//  A raw byte sweep of the whole of .text for every `e8`, `e9`, `0f 8x`,
//  `eb` and `7x` displacement landing inside each range (plus a scan of
//  .rdata/.data for address-shaped dwords) gives:
//
//    StartGame  0x0040fd90..0x00410350
//      inbound from OUTSIDE: three `call`s, all to the ENTRY POINT --
//        0x00408b6b, 0x0041da23, 0x0041dae2.
//      17 interior branches, highest target 0x00410341 (the shared
//        epilogue).  The one .data "reference" found, 0x004afc7e ->
//        0x00410000, is UNALIGNED and reads back out of a table of
//        {0x21,0x0d},{0x35,0x02},{0x41,0x0d},... key records: the bytes are
//        `00 00 41 00` straddling two entries.  Not a reference.
//      The function ENDS at 0x0041034b (`ret 4`) + two `nop`; 0x00410350 is
//        a new function (`push esi ; mov esi,ecx ; lea ecx,[esi+0x158c]`) --
//        and it is CClientMgr::Disconnect, the very function StartGame calls
//        at 0x0040fda0.  clientmgr2.cpp:414 already reconstructs it.
//      => 1472 bytes CONFIRMED.
//
//    DoLoadWorld 0x00413d30..0x004141f3
//      inbound from OUTSIDE: ONE `call`, to the entry point, at 0x004781e7.
//      5 interior branches.  Three apparent short jumps from outside are
//        false decodes of the displacement byte 0x78/0x7c inside
//        `mov eax,[edi+0x78]` (0x413cf8), `mov ecx,[ecx+0x78]` (0x413cd9)
//        and `mov eax,[ebp+0x7c]` (0x41424d) -- each verified by
//        disassembling the containing instruction.
//      The function ENDS at 0x004141f0 (`ret 8`) followed by thirteen
//        `nop`; 0x00414200 is a new function.
//      => 1219 bytes CONFIRMED.  (src/engine/lithtech/cmgrinit.cpp:33
//        already records "0x00413d30..0x004141f3 (495 + 724 = 1219
//        exactly)"; this is an independent confirmation of it.)
//
//  WHAT NAMES THEM -- BOTH NAME THEMSELVES
//  ---------------------------------------
//    * DoLoadWorld pushes 0x004a8600 "CClientShell::DoLoadWorld" at
//      0x00413ffc, 0x004140cb, 0x00414146 and 0x004141d6, and 0x004a8630
//      "ClientShell::DoLoadWorld" (NO leading C -- a second, distinct
//      literal in .data, so it is a second string in the SOURCE) at
//      0x00413f02 and 0x00413f57.  Both are the fnName argument of
//      de_error.h's RETURN_ERROR family.
//    * StartGame pushes 0x004a844c "StartGame" at 0x0040fe34, 0x0040fe94,
//      0x00410164, 0x004101e7, 0x00410247 and 0x00410295 -- six sites, all
//      the fnName argument.  One further site, 0x00410010, pushes
//      0x004a8410 "CClientMgr::StartShell"; that literal is used NOWHERE
//      else in the image, so it is a copy-paste in the original source and
//      is reproduced verbatim below rather than "corrected".
//
//  THE /O1 ORACLE: THERE IS NONE.  The retail server.dll's strings have no
//  "CClientShell", no "DoLoadWorld" and no "StartGame".  Both functions are
//  client-only; neither has a server.dll twin, so every field offset below
//  is read off lithtech.exe and nothing is cross-checked at /O1.
//
//  GROUND TRUTH: the retail lithtech.exe only.  cl 8168 /O2
//  (/Og /Oi /Ot /Oy /Ob1), which is what this image was built with.  Every symbol this file introduces carries the Q2B suffix.
//
//  WHERE THE PIECES COME FROM.  Nothing here invents a class that another
//  file already owns; the views below are layout-identical restatements in
//  the house style, and each field cites the file that already had it:
//
//    clientmgr.h          CClientMgr, CClientConnection (== CClientShell),
//                         CClientFileRef and its inline constructor
//    clientmgr2.cpp       CClientShell/CClientMgr views, cs_TermWorld,
//                         cs_ReleaseListToWorld, cs_ReleaseArrayItems,
//                         CClientMgr::OnEnterWorld, CClientShell::Init,
//                         CClientShell::SetUnknown30, m_Records40c
//    clientshell.cpp      CClientShell's field map (0x14/0x18/0x1c, 0x3c,
//                         0x40, 0x70, 0x78, 0x7c), sizeof 0x90,
//                         CClientShell::StartupLocal, CMsgRef/ILTMessage,
//                         msg_StartWrite, CNetMgrView, IClientShellView
//    cmgrinit.cpp         cm_OnShellInit (0x00411560), the 1219-byte extent
//    csvis.cpp            CClientShell::CreateVisContainerObjects
//    clientrender.cpp     CClientMgr::SetupError (0x0041fc60), g_pErrorFormat
//    demodtx.cpp          CDemoMgr::StartRecording / StartPlayback / Stop
//    servermgr.h          CBaseDriver's vtable, CNetMgr::FindDriver
//    packet.h             CPacket::ReadType / WriteType (BOTH inlined here)
//    de_error.h           RETURN_ERROR / RETURN_ERROR_SYM_DESC
//    lt2new3.cpp          0x00460d80 (and the correction below)
//
//  CORRECTION TO lt2new3.cpp:413.  That file's comment on 0x00460d80 reads
//  "Sole caller is 0x00413ec0, inside the 495-byte __thiscall at
//  0x00413d30, which passes its own `this` and `this->+0x18`."  The call
//  site, the two arguments and the `this->+0x18` are all CORRECT and are
//  reproduced verbatim below (0x00413eb5..0x00413ec0).  Two things in that
//  sentence need changing:
//    (a) "495-byte" is wrong -- the function is 1219 bytes; and
//    (b) the function is not anonymous: it is CClientShell::DoLoadWorld,
//        named by its own RETURN_ERROR literal at 0x004141d6.
//  The same 495 appears in src/engine/lithtech/cchandler.cpp:343-346
//  ("495 bytes ... Nothing at this call site names it") and should get the
//  same two corrections; cchandler.cpp's SIGNATURE for it is right --
//  __thiscall, `ret $0x8`, (CPacket*, 0).
//
// *********************************************************************** //

#include "ltbasedefs.h"       // LTVector, StartGameRequest, NetHost, NetSession
#include "ltcodes.h"          // LT_OK, LT_ERROR, LT_MISSINGWORLDFILE, ...
#include "ltlink.h"           // LTList
#include "de_error.h"         // RETURN_ERROR / RETURN_ERROR_SYM_DESC (server/)
#include "packet.h"           // CPacket::ReadType / WriteType   (server/)

#include <string.h>           // strncpy -- NOT on VC6's intrinsic list
                              // (ccobj.cpp:355 measured exactly that split),
                              // so 0x00410045's `call 0x00492b60` survives


// ----------------------------------------------------------------------- //
//  Forward declarations
// ----------------------------------------------------------------------- //

class CClientMgrQ2B;
class CClientShellQ2B;


// ======================================================================= //
//  1.  THE VIEWS
// ======================================================================= //

// clientmgr.h's CClientFileRef, restated so this file does not have to pull
// that whole header in.  Twelve bytes and the constructor zeroes three
// fields -- dword, dword, WORD -- which is exactly the shape of DoLoadWorld's
// first three prologue stores (0x00413d3c dword, 0x00413d40 dword,
// 0x00413d44 `mov WORD PTR [esp+0x24],bx`).
class CClientFileRefQ2B
{
public:
	CClientFileRefQ2B() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                            // 0x00
	char*  m_pFilename;                            // 0x04
	uint16 m_nLocalID;                             // 0x08
};

// The opaque file-manager record 0x00404680 hands back.  DoLoadWorld only
// ever compares it with CClientShell+0x70 and stores it there, so nothing
// about its contents is known.  clientwin.cpp calls the type CluFileRec.
class CluFileRecQ2B;
// The stream 0x004048c0 hands back (clientworldinfo.cpp: "open one
// CClientFileRef and hand back a stream, or LTNULL").
class ILTStreamQ2B;
// The file manager at CClientMgr+0x1534 (clientmgr2.cpp / clientwin.cpp).
class CFileMgrQ2B;
// The server's world object.  The global at 0x004b17e8 is written at
// 0x0046440f as `lea ecx,[ebx+0x1a4] ; mov ds:0x4b17e8,ecx` -- CServerMgr's
// m_World (servermgr.h puts it at 0x1a4), published only when bit 1 of
// CClient+0x3c0 is set.  DoLoadWorld is its only reader.  (name ours)
class CServerWorldQ2B;
// CServerMgr, as far as StartGame reaches it: two methods and nothing else.
// clientshell.cpp already declares both on its own CServerMgrView.
class CServerMgrQ2B
{
public:
	// The decoded symbol is
	// ?DoStartWorld@CServerMgr@@QAEKPADKM@Z, so LTRESULT (char*, uint32,
	// float).  The float is what makes CClientMgr+0x125c a float here.
	LTRESULT DoStartWorld(char* pWorldName, uint32 nFlags, float fTime); // 0x004750f0
	void     GetErrorString(char* pBuf, int nMaxLen);                    // 0x00474580
};


// servermgr.h's CBaseDriver, restated with the five slots StartGame reaches.
// Slot 0x30 (HostSession) and the two field-free lobby entries are that
// header's numbering; the three unnamed ones get the names their call sites
// imply and keep their displacements in the comment.
class CBaseDriverQ2B
{
public:
	virtual ~CBaseDriverQ2B()=0;                             // 0x00
	virtual LTBOOL Init()=0;                                 // 0x04
	virtual void _p08()=0;                                   // 0x08
	virtual void _p0c()=0;                                   // 0x0c
	virtual void _p10()=0;                                   // 0x10
	virtual void _p14()=0;                                   // 0x14
	virtual void _p18()=0;                                   // 0x18
	virtual void _p1c()=0;                                   // 0x1c
	virtual void _p20()=0;                                   // 0x20
	virtual void _p24()=0;                                   // 0x24
	virtual void _p28()=0;                                   // 0x28
	virtual void _p2c()=0;                                   // 0x2c
	// servermgr.h: HostSession.  0x0040feb4 -- LTRESULT, and StartGame
	// RETURNS what it answers.
	virtual LTRESULT HostSession(NetHost* pHostInfo)=0;      // 0x30
	// 0x004102bb -- one argument, the request's m_pNetSession, LTRESULT.
	// (name ours)
	virtual LTRESULT JoinSession(NetSession* pSession)=0;    // 0x34
	virtual void _p38()=0;                                   // 0x38
	virtual void _p3c()=0;                                   // 0x3c
	virtual void _p40()=0;                                   // 0x40
	virtual void _p44()=0;                                   // 0x44
	// 0x0040fe69 -- the SG_LOBBY arm of the host path, same argument as
	// HostSession.  LTRESULT: StartGame tests it against zero and reports
	// LT_ERROR rather than passing it on.  (name ours)
	virtual LTRESULT HostLobbySession(NetHost* pHostInfo)=0; // 0x48
	// 0x00410215 -- the SG_LOBBY arm of the join path, NO arguments, and
	// the sense is INVERTED against the two above: `test eax,eax ; jne
	// <continue>`, so nonzero is success.  An LTBOOL, not an LTRESULT.
	// (name ours)
	virtual LTBOOL JoinLobbySession()=0;                     // 0x4c
	virtual void _p50()=0;                                   // 0x50
	// 0x0041018d -- one argument, the request's m_TCPAddress, LTRESULT.
	// (name ours)
	virtual LTRESULT JoinTCPSession(char* sAddress)=0;       // 0x54

	// servermgr.h's CBaseDriver: the name is INLINE at +0x04 (its
	// m_pad_04 run, which netmgr.cpp's AddDriver fills in place).
	// FindDriver (0x00459b1d) hands `lea 0x4(%esi)` to stricmp.
	char m_Name[0x44 - 0x04];                                // 0x04
};


// servermgr.h's CDriverList as far as FindDriver reaches it (name ours).
class CDriverListQ11
{
public:
	uint32 GetSize() const { return m_nElements; }

	void*            m_pVtbl;                                // +0x00
	CBaseDriverQ2B** m_pArray;                               // +0x04
	uint32           m_nElements;                            // +0x08
};


// The client's CNetMgr sub-object (CClientMgr+0x50c, 0x104 bytes).  Both
// entries are __thiscall on the sub-object, which is why every call site
// loads its ADDRESS into ecx and pushes nothing for `this`.  m_pMainDriver
// is servermgr.h's name for CNetMgr+0x100 -- 0x60c absolute, which is the
// field StartGame reads in both its STARTGAME_HOST and STARTGAME_CLIENT
// arms.
class CNetMgrQ2B
{
public:
	CBaseDriverQ2B* FindDriver(char* pDriverName);           // 0x00459b00
	// 0x00456360.  __thiscall with THREE arguments: CClientShell::
	// SendToServer (0x00413cd0) is `add ecx,0x50c ; push 0x80 ; push
	// <connid> ; push <msg> ; call` with no cleanup after, so the callee
	// cleans and `this` is the sub-object.
	void SendPacket(CPacket* pMsg, uint32 nConnID, uint32 nFlags);

	uint8           m_pad_000[0x3c];
	// servermgr.h's m_Drivers (CDriverList, a CMoArray<CBaseDriver*>) at
	// +0x3c: vptr, then m_pArray at +0x40 and m_nElements at +0x44 --
	// FindDriver's two reads (0x00459b13, 0x00459b05).  Spelled as the
	// three words rather than the template so this TU does not
	// instantiate a second CMoArray.
	CDriverListQ11  m_Drivers;                               // +0x03c
	uint8           m_pad_048[0x100 - 0x48];
	CBaseDriverQ2B* m_pMainDriver;                           // +0x100 (0x60c)
};


// The demo manager at CClientMgr+0x158c.  demodtx.cpp's CDemoMgrQ42C, with
// only the three entries this file reaches; the trailing pad is what
// clientmgr2.cpp's `m_Sub158c[0x1610 - 0x158c]` covers.
class CDemoMgrQ2B
{
public:
	LTRESULT StartRecording(char* pShellName, char* pFilename);          // 0x0042c050
	LTRESULT StartPlayback(char* pFilename, char* pShellName,
	                       uint32 nMaxBytes);                            // 0x0042c130
	void     Stop();                                                     // 0x0042c2a0

	uint8 m_pad_00[0x1610 - 0x158c];
};


// The game DLL's shell.  src/LT2/sdk/inc/iclientshell.h's declaration order with
// ~IClientShell at slot 0 -- the count clientmgr2.cpp establishes off
// OnEnterWorld/OnExitWorld at 0x20/0x24 and clientinterface.cpp off
// OnPlaySound at 0x5c.  This file needs slot 7 (0x1c) and slot 24 (0x60),
// and slot 24 is the one that CONFIRMS the whole count: SetDisconnectCode
// is the only slot in the header with a DEFAULT ARGUMENT, and the shipped
// call at 0x00413fd5 pushes exactly three dwords, the last of them the zero
// register -- the default being materialised by the caller.
class IClientShellQ2B
{
public:
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0;
	virtual void PreLoadWorld(char* pWorldName)=0;                       // 0x1c
	virtual void OnEnterWorld()=0;                                       // 0x20
	virtual void OnExitWorld()=0;                                        // 0x24
	virtual void _p28()=0; virtual void _p2c()=0; virtual void _p30()=0;
	virtual void _p34()=0; virtual void _p38()=0; virtual void _p3c()=0;
	virtual void _p40()=0; virtual void _p44()=0; virtual void _p48()=0;
	virtual void _p4c()=0; virtual void _p50()=0; virtual void _p54()=0;
	virtual void _p58()=0; virtual void _p5c()=0;
	virtual void SetDisconnectCode(uint32 nCode, const char* pMsg,
	                               uint32 nSubReason = 0)=0;             // 0x60
};


// The refcounting half of the write message.  The object StartGame drives is
// a CPacket -- packet.h's, field for field: WriteType's expansion at
// 0x004100a3 reads m_Pos at +0x38, m_MaxDataLen at +0x36, m_DataLen at +0x34,
// m_Data.m_AllocSize at +0x58, m_Data.m_pData at +0x54 and m_Flags at +0x40,
// and its overrun arm pushes "CPacket::WriteType".  It ALSO has a vtable at
// +0x00 whose slots 1 and 2 are AddRef and Release (0x00410093 / 0x004100a0 /
// 0x0041033c), which is the first dword of packet.h's `m_pad_00[0x34]`.
//
// The two are spelled as two classes over one address rather than as a
// derivation, because deriving would put the vptr BEFORE CPacket's members
// and move every displacement four bytes.  clientshell.cpp's ILTMessageView
// takes the other route and re-spells the layout; this file keeps packet.h's
// templates instead, which is what makes the two inlined WriteTypes
// reproducible.
class IPacketRefQ2B
{
public:
	virtual void _p00()=0;
	virtual void AddRef()=0;                                             // 0x04
	virtual void Release()=0;                                            // 0x08

	// 0x0045c390, __thiscall, callee-cleaned.  The decoded symbol map
	// carries it as ?WriteData@ServerMessage@@QAEXPAXK@Z, whose K would be a
	// uint32 -- but that symbol is a bridged-callsite inference, and
	// StartGame's site loads the length with `mov ax,WORD PTR [edi+0x11f4]`
	// and pushes eax with the top half dirty, which only a uint16 parameter
	// explains ("uint16 parameters load as mov ax").  The callee
	// masks with `and eax,0xffff` at 0x0045c3af, which is the other half of
	// the same fact.
	void WriteData(void* pData, uint16 nLen);
};

// clientshell.cpp's CMsgRef: the four-byte by-value holder 0x0045c080 hands
// back through a hidden pointer argument (pushed LAST, which is why the
// shipped `push eax` at 0x00410080 sits AFTER the two 0x1000s).  The copy
// AddRefs and the destructor Releases; StartGame's
//   call 0x45c080 ; esi = [eax] ; if (esi) esi->AddRef()
//   ecx = [slot]  ; if (ecx) ecx->Release()
// is the copy into the named holder followed by the temporary going away.
class CMsgRefQ2B
{
public:
	CMsgRefQ2B(const CMsgRefQ2B& other)
	{
		m_pMsg = other.m_pMsg;

		if (m_pMsg)
			((IPacketRefQ2B*)m_pMsg)->AddRef();
	}

	~CMsgRefQ2B()
	{
		if (m_pMsg)
			((IPacketRefQ2B*)m_pMsg)->Release();
	}

	CPacket* operator->() const { return m_pMsg; }
	operator CPacket*() const   { return m_pMsg; }

	CPacket* m_pMsg;                                                     // 0x00
};

// clientshell.cpp's CMsgHold: the NAMED holder is built from the returned
// temporary's pointer (`CMsgHold cMsg(msg_StartWrite(...).m_pMsg)`), which
// is what gives the shipped AddRef-then-Release-the-temporary pair at
// 0x00410093 / 0x004100a0.  Initialising a CMsgRef from the call directly
// elides the copy and loses both.
class CMsgHoldQ2B
{
public:
	CMsgHoldQ2B(CPacket* pMsg)
	{
		m_pMsg = pMsg;

		if (m_pMsg)
			((IPacketRefQ2B*)m_pMsg)->AddRef();
	}

	~CMsgHoldQ2B()
	{
		if (m_pMsg)
			((IPacketRefQ2B*)m_pMsg)->Release();
	}

	CPacket* m_pMsg;                                                     // 0x00
};

// 0x0045c080 -- a fresh write message of the given size.  __cdecl, returns
// the holder by value (clientshell.cpp names it).
CMsgRefQ2B msg_StartWriteQ2B(uint32 nMaxSize, uint32 nSize);


// The sixteen-byte block CClientMgr's world loader is handed.  Every field
// is a shipped store: +0x00 the stream just opened (0x00414048), +0x04 the
// manager's dword at 0x11d8 (0x00414065), +0x08 the address of the twelve-
// byte forwarder at 0x00413d20 (0x0041406c) and +0x0c the manager itself
// (0x00414074), which is that forwarder's own argument -- so the pair is a
// callback and its user pointer.  The constructor zeroes all four: the
// shipped code stores zero to each of them before storing the real value
// (0x00414044 / 0x0041404c / 0x00414053 / 0x00414057).  (names ours)
class CWorldLoadRequestQ2B
{
public:
	CWorldLoadRequestQ2B()
	{
		m_pStream      = LTNULL;
		m_nUnknown04   = 0;
		m_pfnCallback  = LTNULL;
		m_pCallbackUser = LTNULL;
	}

	ILTStreamQ2B* m_pStream;                                   // 0x00
	uint32        m_nUnknown04;                                // 0x04
	void        (*m_pfnCallback)(CClientMgrQ2B* pClientMgr);   // 0x08
	CClientMgrQ2B* m_pCallbackUser;                            // 0x0c
};


// CClientMgr, as far as these two functions reach it.  Every offset is a
// shipped displacement and every name that another file already has is that
// file's name.
class CClientMgrQ2B
{
public:
	uint8            m_pad_0000[0x184];
	// 0x00414085 stores the world stream here and 0x0041408b reads it back
	// one instruction later, through the SAME base register -- so it is a
	// member and not a local.  It falls inside clientmgr.h's m_World
	// (0x06c..0x508).  (name ours)
	ILTStreamQ2B*    m_pWorldStream184;                        // 0x000184
	uint8            m_pad_0188[0x40c - 0x188];
	// clientmgr2.cpp's m_Records40c: eleven sixteen-byte records.  Both
	// CClientShell::ResetRecords (0x00413cf0) and DoLoadWorld's tail loop
	// (0x00414163) walk them with a `add 0x10` stride and a
	// `cmp 0xb0` bound, so the count and the width are proven twice.  They
	// are LTLists: 0x00414370 takes each one's address as an LTList*.
	uint8            m_Lists40c[0xb0];                         // 0x00040c
	uint8            m_pad_04bc[0x50c - 0x4bc];
	CNetMgrQ2B       m_NetMgr;                                 // 0x00050c
	uint8            m_pad_0610[0x61c - 0x610];
	IClientShellQ2B* m_pClientShell;                           // 0x00061c
	uint8            m_pad_0620[0x11d8 - 0x620];
	// clientmgr2.cpp: "zeroed by the ctor", read by 0x004746aa and by
	// cm_GetUnknown11d8 (0x00414920).  DoLoadWorld copies it into the world
	// load request.
	uint32           m_Unknown11d8;                            // 0x0011d8
	uint8            m_pad_11dc[0x125c - 0x11dc];
	// A FLOAT.  lt2new3.cpp's note settles it from the other side --
	// CLTClient::GetTime (0x00407b40) is `mov 0x4b17e0,%eax ; flds
	// 0x125c(%eax)` -- and the decoded mangled
	// name for 0x004750f0 (`...@@QAEKPADKM@Z`) makes StartGame's third
	// DoStartWorld argument a float too, which agrees.
	float            m_fTime125c;                              // 0x00125c
	uint8            m_pad_1260[0x150c - 0x1260];
	CClientShellQ2B* m_pClientShell2;                          // 0x00150c
	uint8            m_pad_1510[0x1534 - 0x1510];
	CFileMgrQ2B*     m_pFileMgr;                               // 0x001534
	uint8            m_pad_1538[0x158c - 0x1538];
	CDemoMgrQ2B      m_DemoMgr;                                // 0x00158c

	// ---- the two entry points this file defines -------------------------
	LTRESULT StartGame(StartGameRequest* pRequest);            // 0x0040fd90
	inline void SendToServerQ2B(CPacket* pMsg);


	// ---- reached, not defined -------------------------------------------
	// clientmgr2.cpp:414 (0x00410350): winds the demo manager down and
	// tells the shell to disconnect.  StartGame's very first act.
	void     Disconnect();                                     // 0x00410350
	// clientmgr2.cpp:377 -- tell the game DLL a world came up.  __thiscall,
	// one argument, the shell.
	void     OnEnterWorld(CClientShellQ2B* pShell);            // 0x0040fd50
	// 0x004248d0 -- __thiscall, one argument, LTBOOL.  Both `this + 0x6c`
	// and `arg + 0x6c` are handed to 0x0048d930, so the argument is a world
	// object of the same shape as the manager's own m_World; it also sets
	// this+0x1e8 to 1 first.  DoLoadWorld reports "Inherit failed" when it
	// answers false, which is where the name comes from.  (name ours)
	LTBOOL   InheritWorld(CServerWorldQ2B* pWorld);            // 0x004248d0
	// 0x004250e0 -- __thiscall, one argument, the stream just stored at
	// +0x184; it clears bit 2 of this+0x180 and calls slot 5 (0x14) on the
	// argument.  The decoded symbol map calls the address
	// CWorldData::LoadObjects on `bridged-callsite` evidence, which
	// may be wrong -- and `this` here is the CLIENT MANAGER,
	// so the row's class is at least not the one at this call site.  Kept
	// as an address.
	void     Sub4250e0Q2B(ILTStreamQ2B* pStream);              // 0x004250e0
	// 0x004224d0 -- __thiscall, one argument, LTRESULT; it answers 3
	// immediately if the request's first dword is null.  Same
	// `bridged-callsite` caveat: the map calls it CServerWorldFull::Load.
	LTRESULT LoadWorldQ2B(CWorldLoadRequestQ2B* pRequest);     // 0x004224d0
	// clientrender.cpp / cchandler.cpp: a __cdecl VARARGS member, which is
	// why the shipped sites push `this` LAST.  It formats the resource
	// string for the code into this+0x620 with a 0x12c cap.
	LTRESULT SetupError(LTRESULT dResult, ...);                // 0x0041fc60
};

extern CClientMgrQ2B* g_pClientMgrQ2B;                         // 0x004b17e0


// The engine's own client shell (clientmgr2.cpp names the class off
// DoLoadWorld's own literal; clientshell.cpp fixes sizeof at 0x90 and
// refutes the second vptr).  ONE virtual is declared so that the vptr lands
// at +0x00 and StartGame's `mov eax,[esi] ; push 1 ; mov ecx,esi ;
// call [eax]` comes out as a __thiscall through slot 0.
class CClientShellQ2B
{
public:
	CClientShellQ2B();                                         // 0x00413210

	// Slot 0 of 0x004a1d18.  clientmgr.h reads it as Disconnect(int) from
	// the `1` CClientMgr::Disconnect passes; the identical bytes would come
	// out of `delete pShell` on a class with a virtual destructor, and
	// nothing in either function separates the two.  Declared, not defined,
	// so no vtable is emitted in this object.
	virtual void Disconnect(int nReason);                      // vtable 0x00

	uint8            m_pad_0004[0x14 - 0x04];
	// clientshell.cpp's m_Unknown14 / m_Unknown18 / m_Unknown1c, and
	// clientmgr.h's CClientConnection names the middle two: +0x18 is what
	// GetGameTime hands out and +0x1c what GetGameFrameTime hands out.
	float            m_fPrevGameTime;                          // 0x014
	float            m_fGameTime;                              // 0x018
	float            m_fFrameTime;                             // 0x01c
	// Copied from CClientMgr+0x125c with an INTEGER move
	// (0x00413ddd/0x00413dea), which is why it is spelled uint32 here --
	// see lt2new3.cpp's coin-flip note on exactly this pair.  (name ours)
	uint32           m_Unknown20;                              // 0x020
	// 0x3d088889 stored as an immediate, which is 1.0f/30.0f correctly
	// rounded -- a float assigned a constant, so no FPU.
	float            m_fUnknown24;                             // 0x024
	uint8            m_pad_0028[0x34 - 0x28];
	// clientmgr2.cpp: the third argument of every send through the
	// manager's CNetMgr.  StartGame reads it at 0x00410324.
	uint32           m_nConnID;                                // 0x034
	uint8            m_pad_0038[0x3c - 0x38];
	// clientshell.cpp: "1 in StartupLocal".  DoLoadWorld branches on it to
	// choose inheriting the server's world over loading the file.
	LTBOOL           m_bLocal;                                 // 0x03c
	CServerMgrQ2B*   m_pServerMgr;                             // 0x040
	uint8            m_pad_0044[0x70 - 0x44];
	// The world file record currently loaded.  DoLoadWorld compares the
	// lookup against it to decide whether the world CHANGED, and stores the
	// new one as its last act but one.  (name ours)
	CluFileRecQ2B*   m_pWorldFileRec;                          // 0x070
	uint8            m_pad_0074[0x78 - 0x74];
	CClientMgrQ2B*   m_pClientMgr;                             // 0x078
	LTBOOL           m_bWorldLoaded;                           // 0x07c
	uint8            m_pad_0080[0x84 - 0x80];
	// clientmgr.h's CClientConnection::m_nGameMode, which GetGameMode hands
	// out; StartGame seeds it from StartGameRequest::m_Type.
	int              m_nGameMode;                              // 0x084
	uint8            m_pad_0088[0x90 - 0x88];

	// ---- the entry point this file defines ------------------------------
	LTRESULT DoLoadWorld(CPacket* pMsg, int nUnused);          // 0x00413d30

	// ---- reached, not defined -------------------------------------------
	LTBOOL   Init(CClientMgrQ2B* pClientMgr);                  // 0x00413310 (clientmgr2.cpp)
	// clientmgr2.cpp types the argument uint32; StartGame hands it a
	// CBaseDriver*, so it is spelled void* here to avoid a cast that would
	// be free anyway.
	LTRESULT SetUnknown30(void* pValue);                       // 0x00413430 (clientmgr2.cpp)
	LTRESULT StartupLocal(StartGameRequest* pRequest, LTBOOL bHost,
	                      uint32 nGameType);                   // 0x00413440 (clientshell.cpp)
	void     Sub4143f0Q2B();                                   // 0x004143f0 (clientmgr2.cpp)
	LTBOOL   CreateRenderWorld();                              // 0x00414860 (clientmgr2.cpp)
	LTBOOL   CreateVisContainerObjects();                      // 0x004144e0 (csvis.cpp)
};


// ----------------------------------------------------------------------- //
//  Globals and free functions these two reach but do not define
// ----------------------------------------------------------------------- //

// 0x004b17e8 -- see CServerWorldQ2B above.  Two references in the whole
// image: the store at 0x0046440f and DoLoadWorld's read at 0x00413eb8.
// Zeroed .bss; defined here beside its one reader (smgr.cpp externs it).
CServerWorldQ2B* g_pServerWorldQ2B;                            // 0x004b17e8

// clientmgr2.cpp's cs_TermWorld: UnloadWorld, ResetRecords, Sub4143f0.
// __cdecl, one argument.
void cs_TermWorldQ2B(CClientShellQ2B* pShell);                 // 0x004143b0
// clientmgr2.cpp's cs_ReleaseListToWorld: hands every element of an LTList
// to the manager's world.  __cdecl, two arguments.
void cs_ReleaseListToWorldQ2B(CClientShellQ2B* pShell, LTList* pList);  // 0x00414370
// clientmgr2.cpp's cs_ReleaseArrayItems.  __cdecl, two arguments; the
// SECOND supplies the array at +0x18c and the count at +0x190 and the FIRST
// is only passed on.  clientmgr2.cpp types the first CClientShell* from its
// other call site; at DoLoadWorld's site BOTH arguments are the client
// manager (0x004140fd `push edi` = the cached m_pClientMgr, 0x004140fe
// `push edx` = a fresh load of the same member), so the first parameter
// cannot be a CClientShell* there.  Declared void* to keep both callers
// honest.
void cs_ReleaseArrayItemsQ2B(void* pOwner, CClientMgrQ2B* pArray);      // 0x00414200

// cmgrinit.cpp's cm_OnShellInit -- the manager's half of the shell startup
// and the exact mirror of clientmgr2.cpp's cm_OnShellTerm (0x004115c0).
// __cdecl, one argument; its only caller is StartGame at 0x0041005c.
void cm_OnShellInitQ2B(CClientMgrQ2B* pClientMgr);             // 0x00411560

// The file manager's three entry points, at the client's addresses.
// 0x004045d0 answers the empty buffer at 0x004b0ab4 when the manager is
// null, so it is a name lookup; clientmgr3.cpp:232 says the same ("asked
// for a file by 0x00404680 and for a name by 0x004045d0").
char* fm_GetFilenameQ2B(CFileMgrQ2B* pFileMgr, CClientFileRefQ2B* pRef);  // 0x004045d0
// clientwin.cpp's clu_fm_GetFileRec, three arguments, the third a uint8.
CluFileRecQ2B* fm_GetFileRecQ2B(CFileMgrQ2B* pFileMgr, CClientFileRefQ2B* pRef,
                                uint8 nFlag);                             // 0x00404680
// clientworldinfo.cpp: open one CClientFileRef and hand back a stream.
ILTStreamQ2B* fm_OpenFileQ2B(CFileMgrQ2B* pFileMgr, CClientFileRefQ2B* pRef); // 0x004048c0

// lt2new3.cpp reconstructs this one (its section D): __cdecl, two
// arguments, void; it stores the float into the object's +0x28, copies the
// owner's +0x125c into +0x2c and tie-offs the two LTLinks at +0x44/+0x50.
void sub_460d80Q2B(CClientShellQ2B* pShell, float fTime);      // 0x00460d80

// clienttexmgr.cpp names both: 0x004202c0 runs the whole texture collection
// twice, 0x004202f0 hands every texture with no render-side copy back to
// the render DLL.  __cdecl, one argument each.
void tex_Sub4202c0Q2B(CClientMgrQ2B* pClientMgr);              // 0x004202c0
void tex_Sub4202f0Q2B(CClientMgrQ2B* pClientMgr);              // 0x004202f0

// clientdsi.cpp: the setter for the dword 0x0042e670 reads.  __cdecl, one
// argument.  Declared with clientdsi.cpp's own name and signature so the
// call links to that definition.
void sub_42e680(uint32 nValue);                                // 0x0042e680

// ccclient.cpp / cchandler.cpp: the console's coloured printf.  Colour
// first, then the level, then the format.  __cdecl varargs.
void con_PrintColoredQ2B(uint32 nColour, int nLevel, char* pFormat, ...); // 0x0041b8a0

// cchandler.cpp's PACKETTRACE_COLOR: the same 0x006464fa immediate, pushed
// here at 0x00414035.  A LITERAL, not the g_ConsoleColour global that
// CClientShell::StartupLocal reads.
#define WORLDLOAD_COLOR_Q2B 0x006464fa



// ======================================================================= //
//  2.  0x00413d20, 12 bytes -- the world-load progress forwarder
//
//  A file-static one-liner that exists only so that DoLoadWorld can take its
//  ADDRESS: 0x0041406c stores 0x00413d20 into the load request's +0x08 slot,
//  beside the manager that the request's +0x0c slot carries as the user
//  pointer -- and this function's single argument is exactly that pointer.
//  It is the COMDAT immediately in front of DoLoadWorld, which is where a
//  file-static helper of the same TU belongs.
//
//    413d20: mov eax,DWORD PTR [esp+0x4]
//    413d24: push eax
//    413d25: call 0x490200
//    413d2a: pop ecx
//    413d2b: ret
//
//  THE `pop ecx` IS WHY 0x00490200 IS DECLARED VARARGS HERE.
//  After a cdecl call, a `pop ecx` per call means the callee was declared
//  VARARGS at that site; a single batched `add esp,N` means fixed.  With
//  the fixed prototype src/engine/lithtech/clientmgr2.cpp:333 carries, the
//  cleanup is `83 c4 04` and the function is FOURTEEN bytes, not twelve.
//  If the measurement disagrees, the one-line flip is to drop the `...`
//  from the declaration above and accept the two bytes.
// ======================================================================= //

// 0x00490200 -- clientmgr2.cpp declares it `void sub_490200(CClientMgrView*)`.
// Varargs here; see the note above.
void sub_490200VaQ2B(CClientMgrQ2B* pClientMgr, ...);          // 0x00490200

// 0x00490200 is a bare `ret` (warpblit.cpp: the linker folds every empty
// function there), so the shipped callee IS an empty function.  Defined as
// one; /OPT:ICF folds it with the rest.
void sub_490200VaQ2B(CClientMgrQ2B* pClientMgr, ...)
{
}

static void cs_WorldLoadNotifyQ2B(CClientMgrQ2B* pClientMgr)
{
	sub_490200VaQ2B(pClientMgr);
}


// ======================================================================= //
//  3.  CClientShell::DoLoadWorld -- 0x00413d30, 1219 bytes, 38 calls
//
//  WHAT NAMES IT: its own four RETURN_ERROR literals, 0x004a8600
//  "CClientShell::DoLoadWorld".
//
//  WHAT PINS THE SIGNATURE.  `ret $0x8` with ecx read into esi, so
//  __thiscall with two dword arguments.  The sole caller, 0x004781e7, is
//  `push 0x0 ; push eax ; mov ecx,ebp ; call` -- argument two is the
//  literal zero and DoLoadWorld never reads it (the frame slot it would
//  live in, esp+0x3c after the prologue, is only ever written).  Argument
//  one is a CPacket: 0x00413d65 onwards is packet.h's ReadType expanded
//  verbatim on it -- m_Pos at +0x38, m_Data.m_AllocSize at +0x58,
//  m_DataLen at +0x34, m_Data.m_pData at +0x54, m_Flags at +0x40 -- with
//  the overrun arm pushing "CPacket::ReadType" and "ERROR: Packet read
//  overrun!".  src/engine/lithtech/cchandler.cpp:346 already gives the same
//  signature from the call site.
//
//  WHAT PINS THE BODY
//
//    * TWO INLINED ReadTypes, float then uint16.  Both are packet.h's
//      member template, both put their uninitialised `val` in the DEAD
//      PARAMETER SLOT (esp+0x38 == the home of argument one, which is
//      cached in edi across the whole function) -- "an inlined
//      callee's temporary lives in a dead parameter slot", here twice over.
//      The float read's failure arm is `fld DWORD PTR [esp+0x38]` at
//      0x00413dcd, which is that temporary being returned unset.
//
//    * The five member stores at 0x00413dd4..0x00413dea are one read and
//      five assignments: `fst +0x14` keeps the value on the stack and
//      `fstp +0x18` pops it, so ONE float feeds both; `mov +0x1c,ebx` is
//      `= 0.0f` through the function's zero register (three
//      separate assignments store float constants as immediates and never
//      touch the FPU); `mov DWORD PTR +0x24,0x3d088889` is 1.0f/30.0f
//      correctly rounded; and `mov edx,[ecx+0x125c] ; mov +0x20,edx` is an
//      INTEGER copy, which is why +0x20 is spelled uint32 and the source
//      casts (see lt2new3.cpp's note on the identical pair at 0x00460d80).
//
//    * `cmp eax,ebx ; mov [esp+0x14],eax ; mov DWORD PTR [esp+0x38],1 ;
//      jne ; mov [esp+0x38],ebx` is a two-arm select into MEMORY, not a
//      `setne`: an LTBOOL local assigned LTTRUE and then conditionally
//      LTFALSE.  A `(a != b)` expression gives `xor/cmp/setne`.
//
//    * m_bLocal (+0x3c) chooses between INHERITING the server's world --
//      which is what the local-server case can do, and what makes the
//      global at 0x004b17e8 (CServerMgr::m_World, published at 0x0046440f)
//      readable at all -- and loading the world file.  The inherit arm
//      falls through and the file arm is jumped to, so the source tests it
//      POSITIVE.
//
//    * SetDisconnectCode is the vtable slot that CONFIRMS IClientShell's
//      whole numbering: 0x60 is slot 24 and it is the only slot in
//      iclientshell.h with a default argument, and the shipped site pushes
//      three dwords with the zero register LAST -- the caller materialising
//      `nSubReason = 0`.  Its first argument is 1, which is
//      LT_DISCON_MISSINGFILE, in the block that returns
//      LT_MISSINGWORLDFILE.  Two independent facts agreeing.
//
//    * THE FOUR EPILOGUES ARE FOUR REAL EARLY RETURNS, not one shared exit.
//      This image is /O2, which PRESERVES duplicated epilogues where /O1
//      tail-merges them, so the direction to read is: shipped
//      DUPLICATES where we would share => write the early return.  Each of
//      0x00413f15, 0x0041400a, 0x004140d9 and 0x004141e4 is a distinct
//      RETURN_ERROR with a distinct code; 0x00413f6a is a fifth.  What IS
//      shared is narrower and falls out on its own: the three
//      `g_DebugLevel < 1` arms of the three LT_ERROR sites all jump to
//      0x00414154, the third site's own epilogue.
//
//    * The load failure is the one cold block VC6 SINKS past the success
//      return (0x004141a7, after the `return LT_OK` at 0x0041419b) while
//      the other two error blocks stay inline at their guards.
//      The arm that sinks is the one written as the `else` of a POSITIVE
//      test, so that one site is `if (dResult == LT_OK) {...} else {...}`
//      and the other two are `if (!ok) { err }`.
//
//    * The tail loop closes with `add edi,0x10 ; cmp edi,0xb0 ; jl`, a
//      COUNTING for over a byte offset -- not a countdown --
//      and it is the same idiom, same stride and same bound as
//      CClientShell::ResetRecords in clientmgr2.cpp:662.
//
//  SETTLED (measured): the four dwords of `request` are zeroed TWICE, once
//  by the constructor in the prologue (0x00413d49..0x00413d55, off the ebx
//  zero register) and again at the point of use (0x00414044..0x00414057,
//  off a FRESHLY materialised `xor edx,edx`).  The second run is the
//  intrinsic memset -- it materialises its own zero; four member
//  assignments reuse ebx.  Likewise `mov 0x4b17e8,%ebp` BEFORE the call at
//  0x00413ec0 is a local read of the global ahead of the call, and edi is a
//  pClientMgr local loaded after it (InheritWorld, PreLoadWorld, LoadWorld,
//  +0x184, and cs_ReleaseArrayItems' second argument use it; every other
//  site reloads this+0x78).
// ======================================================================= //

LTRESULT CClientShellQ2B::DoLoadWorld(CPacket* pMsg, int /*nUnused*/)
{
	// Slot order, read off the frame: E-0x24 pWorldName, E-0x20 pFileRec,
	// E-0x1c fileRef (12 bytes), E-0x10 request (16 bytes), and the frame
	// is `sub esp,0x24` = 0x24 exactly.  Note that the SCALARS sit at the
	// LOW end and the aggregates above them, which is the mirror image of
	// the ebp-frame rule -- this image is /Oy and esp-relative.
	char*                pWorldName;
	CluFileRecQ2B*       pFileRec;
	CClientFileRefQ2B    fileRef;
	CWorldLoadRequestQ2B request;
	// Register-resident: bWorldChanged takes the dead argument-two home
	// slot, pStream takes ebp, dResult and fTime never touch memory.
	LTBOOL               bWorldChanged;
	ILTStreamQ2B*        pStream;
	LTRESULT             dResult;
	float                fTime;
	int                  nOffset;
	CServerWorldQ2B*     pServerWorld;
	CClientMgrQ2B*       pClientMgr;

	cs_TermWorldQ2B(this);                                          // 0x00413d59

	fTime           = pMsg->ReadType((float*)0);                    // 0x00413d65
	m_fPrevGameTime = fTime;                                        // 0x00413dd7
	m_fGameTime     = fTime;                                        // 0x00413dda
	m_fFrameTime    = 0.0f;                                         // 0x00413dd4
	m_Unknown20     = *(uint32*)&m_pClientMgr->m_fTime125c;         // 0x00413dea
	m_fUnknown24    = 1.0f / 30.0f;                                 // 0x00413de3

	fileRef.m_nFileType = 2;                                        // 0x00413dfb
	fileRef.m_nLocalID  = pMsg->ReadType((uint16*)0);               // 0x00413ded

	pWorldName = fm_GetFilenameQ2B(m_pClientMgr->m_pFileMgr, &fileRef);    // 0x00413e7d
	pFileRec   = fm_GetFileRecQ2B(m_pClientMgr->m_pFileMgr, &fileRef, 0);  // 0x00413e96

	bWorldChanged = LTTRUE;                                         // 0x00413ea7
	if (pFileRec == m_pWorldFileRec)                                // 0x00413ea1
		bWorldChanged = LTFALSE;                                    // 0x00413eb1

	pServerWorld = g_pServerWorldQ2B;                               // 0x00413eb8
	sub_460d80Q2B(this, m_fGameTime);                               // 0x00413ec0
	pClientMgr   = m_pClientMgr;                                    // 0x00413ec8

	if (m_bLocal)                                                   // 0x00413ece
	{
		// The server is in this process, so the world it already has is
		// taken over rather than read off disk again.
		if (!pServerWorld)                                          // 0x00413ed6
			RETURN_ERROR(1, "ClientShell::DoLoadWorld", LT_ERROR)

		if (!pClientMgr->InheritWorld(pServerWorld))                // 0x00413f22
			RETURN_ERROR_SYM_DESC(1, "ClientShell::DoLoadWorld", LT_ERROR,
			                      "Inherit failed")
	}
	else
	{
		if (pClientMgr->m_pClientShell)                             // 0x00413f74
			pClientMgr->m_pClientShell->PreLoadWorld(pWorldName);   // 0x00413f85

		pStream = fm_OpenFileQ2B(m_pClientMgr->m_pFileMgr, &fileRef);  // 0x00413f97

		if (!pStream)                                               // 0x00413fa1
		{
			// The vtable is loaded BEFORE the filename call and `this`
			// reloaded after it, so the shell member is written out twice
			// in the source and VC6 commons only the pair the call does
			// not separate.  nSubReason is the header's default.
			if (m_pClientMgr->m_pClientShell)                       // 0x00413fae
				m_pClientMgr->m_pClientShell->SetDisconnectCode(
					LT_DISCON_MISSINGFILE,
					fm_GetFilenameQ2B(m_pClientMgr->m_pFileMgr, &fileRef));  // 0x00413fd5

			RETURN_ERROR(1, "CClientShell::DoLoadWorld", LT_MISSINGWORLDFILE)
		}

		con_PrintColoredQ2B(WORLDLOAD_COLOR_Q2B, 1, "Entering world %s",
			fm_GetFilenameQ2B(m_pClientMgr->m_pFileMgr, &fileRef)); // 0x0041403a

		// Re-initialised here; see the SETTLED note above.
		memset(&request, 0, sizeof(request));                       // 0x00414042

		request.m_pStream       = pStream;                          // 0x00414048
		request.m_nUnknown04    = m_pClientMgr->m_Unknown11d8;      // 0x00414065
		request.m_pfnCallback   = cs_WorldLoadNotifyQ2B;            // 0x0041406c
		request.m_pCallbackUser = m_pClientMgr;                     // 0x00414074

		dResult = pClientMgr->LoadWorldQ2B(&request);               // 0x00414078

		if (dResult == LT_OK)                                       // 0x0041407d
		{
			pClientMgr->m_pWorldStream184 = pStream;                // 0x00414085
		}
		else
		{
			// The cold arm VC6 sinks past the success return.
			m_pClientMgr->SetupError(LT_INVALIDWORLDFILE, pWorldName);  // 0x004141b2

			RETURN_ERROR_SYM_DESC(1, "CClientShell::DoLoadWorld",
			                      LT_INVALIDWORLDFILE, pWorldName)
		}
	}

	// 0x0041408b -- where both arms join.
	if (pClientMgr->m_pWorldStream184)                              // 0x00414091
		pClientMgr->Sub4250e0Q2B(pClientMgr->m_pWorldStream184);    // 0x00414098

	if (!CreateRenderWorld())                                       // 0x0041409f
		RETURN_ERROR(1, "CClientShell::DoLoadWorld", LT_ERRORBINDINGWORLD)

	if (bWorldChanged)                                              // 0x004140e8
		tex_Sub4202c0Q2B(m_pClientMgr);                             // 0x004140f2

	cs_ReleaseArrayItemsQ2B(m_pClientMgr, pClientMgr);              // 0x004140ff
	tex_Sub4202f0Q2B(m_pClientMgr);                                 // 0x00414108

	if (!CreateVisContainerObjects())                               // 0x00414112
	{
		Sub4143f0Q2B();                                             // 0x0041411d

		RETURN_ERROR_SYM_DESC(1, "CClientShell::DoLoadWorld", LT_ERROR,
		                      "CreateVisContainerObjects failed")
	}

	for (nOffset = 0; nOffset < 0xb0; nOffset += 0x10)              // 0x00414163
		cs_ReleaseListToWorldQ2B(this,
			(LTList*)(m_pClientMgr->m_Lists40c + nOffset));         // 0x00414171

	m_pWorldFileRec = pFileRec;                                     // 0x00414189
	m_pClientMgr->OnEnterWorld(this);                               // 0x0041418f
	m_bWorldLoaded  = LTTRUE;                                       // 0x00414194

	return LT_OK;
}


// ======================================================================= //
//  4.  CClientMgr::StartGame -- 0x0040fd90, 1472 bytes, 47 calls
//
//  WHAT NAMES IT: six RETURN_ERROR sites push 0x004a844c "StartGame".
//  src/engine/lithtech/clientmgr.h:197 already declares
//  `LTRESULT StartGame(StartGameRequest*)` at this address off the
//  ILTClient interface slot, and the two agree.
//
//  WHAT PINS THE SIGNATURE, and it is the single best thing in the
//  function.  `ret $0x4`, `mov ecx,ebx`, one argument -- and that argument
//  is ltbasedefs.h's StartGameRequest EXACTLY, byte for byte, with no
//  invention at all:
//
//      m_Type              0x0000   read at 0x0040fdcd / 0x0040fde1
//      m_WorldName[100]    0x0004   `lea ebp,[edi+0x4]`      0x0040fecd
//      m_TCPAddress[100]   0x0068   `lea eax,[edi+0x68]`     0x00410187
//      m_RecordFilename    0x00cc   `lea eax,[edi+0xcc]`     0x0040fec5
//      m_PlaybackFilename  0x0130   `lea eax,[edi+0x130]`    0x0040ff12
//      m_pNetSession       0x0194   read at 0x00410267
//      m_HostInfo (NetHost) 0x0198  `lea ecx,[edi+0x198]`    0x0040fe5a
//      m_flags             0x11e4   `test BYTE PTR [edi+0x11e4],0x1`
//      m_pGameInfo         0x11e8   (not read here)
//      m_GameInfoLen       0x11ec   (not read here)
//      m_pClientData       0x11f0   read at 0x00410304
//      m_ClientDataLen     0x11f4   read at 0x004100a3 and 0x004102fd
//
//  MAX_SGR_STRINGLEN is 100 and sizeof(NetHost) is 4+4+4+4096+64 = 0x104c,
//  and 0x198 + 0x104c is 0x11e4 to the byte.  The `test BYTE ...,0x1` is
//  SG_LOBBY, and `strncpy(..., 0x63)` at 0x00410045 is
//  MAX_SGR_STRINGLEN - 1.  Nothing in this layout is inferred.
//
//  WHAT PINS THE BODY
//
//    * `push 0x90 ; call 0x004290b0 ; test ; je ; mov ecx,eax ; call
//      0x00413210` is `new CClientShell` verbatim, and 0x90 is
//      the sizeof that clientshell.cpp derives independently.  The
//      m_nGameMode store at 0x0040fdcf is OUTSIDE the null guard and the
//      constructor call is inside it, which is exactly what `new` gives and
//      what a written-out allocate-and-test does not.
//
//    * `sub ecx,2 ; je ... ; dec ecx ; je ... ; test eax,eax ; jne` off ONE
//      load of m_Type: a three-way compare chain, tested in the order
//      CLIENT, CLIENTTCP, HOST, not a jump table.
//
//    * THE THIRD ARGUMENT OF StartupLocal IS THE NET DRIVER, NOT m_Type.
//      0x0040ff66 is `push ebx` and 0x0040ff67 is `mov ebx,[edi]` -- the
//      push happens FIRST, so what reaches the callee is ebx's PREVIOUS
//      value: the zero written at 0x0040fdf8 on the plain path, or the
//      driver loaded at 0x0040fe06 on the STARTGAME_HOST path.  ebx is
//      callee-saved and nothing else writes it between.  This corrects
//      src/engine/lithtech/clientshell.cpp:1043, which reads argument three
//      as "arg1->field0 itself".
//
//    * The lobby split in the HOST arm is an if/else whose arms share their
//      argument setup, which is why `lea ecx,[edi+0x198] ; push ecx ;
//      mov ecx,ebx` is hoisted ABOVE the `test al,1` branch
//      ("an if/else that starts with the same store hoists it above the
//      branch").  The two slots differ in SENSE as well as number: 0x48
//      answers LTRESULT and a non-LT_OK is reported as LT_ERROR, 0x30
//      answers LTRESULT and a non-LT_OK is RETURNED, and in the CLIENT arm
//      0x4c answers an LTBOOL whose ZERO is the failure.
//
//    * `m_RecordFilename[0]` / `m_PlaybackFilename[0]` are an if / else-if:
//      the playback test is only reached when the record test fails, and
//      both arms end in `pRequest->m_WorldName[0] = 0` -- one block at
//      0x0040ff62 that the first arm jumps forward into, which is
//      "VC6 keeps the SECOND copy inline".
//
//    * FOUR separate `if (pShell) pShell->Disconnect(1)` sites
//      (0x0040feeb, 0x0040ff41, 0x0040ff7f, 0x0040ffe0), each followed by
//      its own return.  At /O2 duplicated epilogues are PRESERVED
//      so these are four real early returns in the source, not
//      one shared cleanup.  Two of them (0x0040feed, 0x0040ff43) let VC6
//      cross-jump the `return dResult` into the final epilogue because eax
//      already holds the value on that path; the other two reload it.
//
//    * The message block is a POSITIVE guard with the LT_OK return
//      trailing: `mov eax,[ebx+0x150c] ; test ; je 0x0041033f` and
//      0x0041033f is `xor eax,eax` immediately before the last epilogue.
//
//    * WriteType<uint16> is packet.h's member template expanded in place --
//      the grow loop, the `SetAtGrow(m_AllocSize, &dummy, &g_PacketDataBank)`
//      with 0x004b4e88 as the bank, the `add WORD PTR [esi+0x38],ax` /
//      `add WORD PTR [esi+0x34],ax` pair off a materialised 2, and the
//      overrun arm pushing "CPacket::WriteType" and setting
//      PACKETFLAG_WRITEOVERFLOW with `or al,0x4` AFTER the report.  The
//      by-value `T val` parameter lands in the frame slot that held `this`
//      (0x004100bb), which is dead by then.
//
//    * m_ClientDataLen is read TWICE, once for each call, both times as a
//      WORD -- so the cast is written out at each use and not cached
//      (repeated member loads are repeated source expressions).
//
//  THE ONE LITERAL THAT IS NOT "StartGame".  0x00410010 pushes 0x004a8410
//  "CClientMgr::StartShell" as the fnName of the world-load failure.  That
//  string occurs nowhere else in the image and no function of that name
//  exists; it is a copy-paste in the original source and is reproduced.
// ======================================================================= //

// WHAT CLOSED StartGame (measured, lithtech.exe 0x0040fd90, now identical):
//   * the type test is a SWITCH: `sub ecx,2 ; je ; dec ecx ; je` at
//     0x0040fde8 is VC6's small-switch chain; the if/else-if chain gave
//     `cmp $2 ; jne` and 83 bytes more.
//   * the message holder is CMsgHoldQ2B built from `.m_pMsg` (AddRef /
//     Release-the-temporary at 0x00410093 / 0x004100a0).
//   * the grow inside WriteType is a CALL to Insert2 (0x004100ee ->
//     0x00412f50): /O2's per-function inline budget.  Two expansions spend
//     it -- LTStrCpy (ltbasedefs.h) for the world-name copy at 0x00410045,
//     and this send wrapper (name ours; a stand-in for whatever inline the
//     original sent through -- the bytes are just the SendPacket call).
//     Either one alone leaves Insert2 expanded (measured).
//   * pWorldName is a local: `lea 0x4(%edi),%ebp` at 0x0040fecd, live to
//     0x0041004d, is what pushes dResult into memory in the two demo arms
//     and lets VC6 merge their `m_WorldName[0] = 0` tails at 0x0040ff62.
inline void CClientMgrQ2B::SendToServerQ2B(CPacket* pMsg)
{
	m_NetMgr.SendPacket(pMsg, m_pClientShell2->m_nConnID, 0x80);
}

LTRESULT CClientMgrQ2B::StartGame(StartGameRequest* pRequest)
{
	// Frame arithmetic: `sub esp,0x20c` = 524.  0x20c = 4 (the compiler's
	// own spill of `this`, 0x0040fd9c) + 4 unused + 4 (dResult, E-0x204)
	// + 256 (errorString, E-0x200) + 256 (worldName, E-0x100).  Slot order
	// again runs scalars-low / arrays-high, as in DoLoadWorld.
	LTRESULT         dResult;
	char             errorString[256];
	char             worldName[256];
	CClientShellQ2B* pShell;
	CBaseDriverQ2B*  pDriver;
	char*            pWorldName;

	Disconnect();                                                   // 0x0040fda0

	pShell = new CClientShellQ2B;                                   // 0x0040fda5
	pShell->m_nGameMode = pRequest->m_Type;                         // 0x0040fdcf
	pShell->Init(this);                                             // 0x0040fdd5

	sub_42e680(0);                                                  // 0x0040fddc

	switch (pRequest->m_Type)                                       // 0x0040fde6
	{
	case STARTGAME_CLIENT:                                          // 0x0040fdeb
	{
		pDriver = m_NetMgr.m_pMainDriver;                           // 0x004101b5

		if (!pDriver)                                               // 0x004101bb
			RETURN_ERROR(1, "StartGame", LT_NOTINITIALIZED)

		if (pRequest->m_flags & SG_LOBBY)                           // 0x00410207
		{
			if (!pDriver->JoinLobbySession())                       // 0x00410215
				RETURN_ERROR(1, "StartGame", LT_ERROR)
		}
		else
		{
			if (!pRequest->m_pNetSession)                           // 0x00410267
				RETURN_ERROR_SYM_DESC(1, "StartGame", LT_INVALIDPARAMS,
				                      "Missing m_pNetSession")

			dResult = pDriver->JoinSession(pRequest->m_pNetSession);  // 0x004102bb

			if (dResult != LT_OK)                                   // 0x00410190
				return dResult;
		}

		dResult = pShell->SetUnknown30(pDriver);                    // 0x0041019b

		if (dResult != LT_OK)                                       // 0x004101a0
			return dResult;
		break;
	}
	case STARTGAME_CLIENTTCP:                                       // 0x0040fdf2
	{
		pDriver = m_NetMgr.FindDriver("internet");                  // 0x00410132

		if (!pDriver)                                               // 0x00410139
			RETURN_ERROR(1, "StartGame", LT_NOTINITIALIZED)

		dResult = pDriver->JoinTCPSession(pRequest->m_TCPAddress);  // 0x0041018d

		if (dResult != LT_OK)                                       // 0x00410190
			return dResult;

		dResult = pShell->SetUnknown30(pDriver);                    // 0x0041019b

		if (dResult != LT_OK)                                       // 0x004101a0
			return dResult;
		break;
	}
	default:
	{
		pDriver = LTNULL;                                           // 0x0040fdf8

		if (pRequest->m_Type == STARTGAME_HOST)                     // 0x0040fdfa
		{
			pDriver = m_NetMgr.m_pMainDriver;                       // 0x0040fe06

			if (!pDriver)                                           // 0x0040fe0c
				RETURN_ERROR(1, "StartGame", LT_NOTINITIALIZED)

			if (pRequest->m_flags & SG_LOBBY)                       // 0x0040fe60
			{
				if (pDriver->HostLobbySession(&pRequest->m_HostInfo) != LT_OK)  // 0x0040fe69
					RETURN_ERROR(1, "StartGame", LT_ERROR)
			}
			else
			{
				dResult = pDriver->HostSession(&pRequest->m_HostInfo);  // 0x0040feb4

				if (dResult != LT_OK)                               // 0x0040feb7
					return dResult;
			}
		}

		pWorldName = pRequest->m_WorldName;                         // 0x0040fecd

		if (pRequest->m_RecordFilename[0])                          // 0x0040fecb
		{
			dResult = m_DemoMgr.StartRecording(pWorldName,
			                                   pRequest->m_RecordFilename);  // 0x0040fede

			if (dResult != LT_OK)                                   // 0x0040fee3
			{
				if (pShell)                                         // 0x0040feeb
					pShell->Disconnect(1);                          // 0x0040fef9

				return dResult;
			}

			pWorldName[0] = 0;                                      // 0x0040ff62
		}
		else if (pRequest->m_PlaybackFilename[0])                   // 0x0040ff18
		{
			// The demo file names the world it was recorded in; that name
			// comes back in worldName and is copied over m_WorldName once
			// the local server is up.
			dResult = m_DemoMgr.StartPlayback(pRequest->m_PlaybackFilename,
			                                  worldName, 0x100);    // 0x0040ff34

			if (dResult != LT_OK)                                   // 0x0040ff39
			{
				if (pShell)                                         // 0x0040ff41
					pShell->Disconnect(1);                          // 0x0040ff4f

				return dResult;
			}

			pWorldName[0] = 0;                                      // 0x0040ff62
		}

		dResult = pShell->StartupLocal(pRequest,
		                               pRequest->m_Type == STARTGAME_HOST,
		                               (uint32)pDriver);            // 0x0040ff74

		if (dResult != LT_OK)                                       // 0x0040ff7b
		{
			if (pShell)                                             // 0x0040ff7f
				pShell->Disconnect(1);                              // 0x0040ff89

			return dResult;
		}

		if (pWorldName[0])                                          // 0x0040ff9a
		{
			if (pShell->m_pServerMgr->DoStartWorld(pWorldName, 3,
			                                       m_fTime125c) != LT_OK)  // 0x0040ffb5
			{
				pShell->m_pServerMgr->GetErrorString(errorString, 0x100);  // 0x0040ffcb
				SetupError(LT_SERVERERROR, errorString);            // 0x0040ffd8

				if (pShell)                                         // 0x0040ffe0
					pShell->Disconnect(1);                          // 0x0040ffea

				RETURN_ERROR_SYM_DESC(1, "CClientMgr::StartShell", LT_SERVERERROR,
				                      "error loading world")
			}
		}

		if (pRequest->m_PlaybackFilename[0])                        // 0x00410030
		{
			LTStrCpy(pWorldName, worldName, MAX_SGR_STRINGLEN);     // 0x00410045
		}
		break;
	}
	}

	m_pClientShell2 = pShell;                                       // 0x00410056
	cm_OnShellInitQ2B(this);                                        // 0x0041005c

	if (m_pClientShell2)                                            // 0x0041006a
	{
		CMsgHoldQ2B cMsg(msg_StartWriteQ2B(0x1000, 0x1000).m_pMsg); // 0x00410081

		cMsg.m_pMsg->WriteType((uint16)pRequest->m_ClientDataLen);  // 0x004100a3
		((IPacketRefQ2B*)cMsg.m_pMsg)->WriteData(pRequest->m_pClientData,
			(uint16)pRequest->m_ClientDataLen);                     // 0x0041030e
		cMsg.m_pMsg->m_Data.m_pArray[0] = 5;                         // 0x0041031b
		SendToServerQ2B(cMsg.m_pMsg);                               // 0x0041032f
	}

	return LT_OK;
}


// ======================================================================= //
//
//  PER-ADDRESS STATUS
//
//    0x00413d20    12 B   1 call   cs_WorldLoadNotify           HIGH
//    0x00413d30  1219 B  38 calls  CClientShell::DoLoadWorld    IDENTICAL
//    0x0040fd90  1472 B  47 calls  CClientMgr::StartGame        IDENTICAL
//
//  CALL CENSUS -- counted on the shipped
//  side and BY HAND on this source, because there is no object to
//  compare against.  Both functions are call-DENSE, 31 and 32 bytes per call, with 21
//  and 20 DISTINCT direct callees, so the sequence discriminates well
//  (distinct callees discriminate, not the count).
//
//    DoLoadWorld   shipped 36 direct + 2 indirect = 38
//                  this source 36 direct + 2 indirect = 38.  EXACT.
//                  (36 = 1 TermWorld + 2x2 inside the two inlined ReadTypes
//                   + 3 fm_GetFilename + 1 fm_GetFileRec + 1 0x00460d80
//                   + 1 InheritWorld + 6x2 RETURN_ERROR + 1 fm_OpenFile
//                   + 1 con_PrintColored + 1 LoadWorld + 1 SetupError
//                   + 1 Sub4250e0 + 1 CreateRenderWorld + 1 0x004202c0
//                   + 1 ReleaseArrayItems + 1 0x004202f0
//                   + 1 CreateVisContainerObjects + 1 Sub4143f0
//                   + 1 ReleaseListToWorld + 1 OnEnterWorld;
//                   2 indirect = PreLoadWorld, SetDisconnectCode.)
//
//    StartGame     shipped 35 direct + 12 indirect = 47
//                  this source 36 direct + 12 indirect = 48.
//                  The INDIRECT count is exact -- 4 Disconnect(1), 3
//                  AddRef/Release on the message holder and 5 driver slots.
//                  The one direct EXCESS is CClientShell::SetUnknown30,
//                  which this source calls once in each of the CLIENT and
//                  CLIENTTCP arms where the shipped code cross-jumps the
//                  two into one copy at 0x0041019b.  That is a known
//                  coin-flip, and the census localises it
//                  to that one statement and nothing else.
//
//  "MEDIUM-HIGH" and "MEDIUM" are about BYTE identity.  Neither function
//  has an undecoded region: every instruction in both ranges is accounted
//  for above, and every offset, constant, code and branch is cited by the
//  address it was read at.  What is NOT certain is the source SHAPE at the
//  places the two reports list as coin-flips.
//
// ======================================================================= //


// ======================================================================= //
//
//  2.  TWO CNetMgr-SIDE BODIES StartGame's neighbourhood needs
//
//  REAL HOME: src/engine/server/netmgr.cpp (lithtech.exe's netmgr.obj run,
//  0x00455xxx..0x0045a1xx).  Written HERE, in a lithtech-only TU, because
//  netmgr.cpp is shared with server.dll and neither body exists in that
//  image as the code below: server.dll has no FindDriver at all, and its
//  CNetBank slot 1 is a bare 5-byte `jmp` onto Allocate (0x10023fd6 ->
//  0x10024162, see servermgr.h's CNetBank note), which servermgr.h leaves
//  undeclared on purpose.  Moving either into netmgr.cpp would change what
//  server.dll's netmgr.obj emits for nothing.
//
// ======================================================================= //

// 0x00459b00, 72 bytes -- CNetMgr::FindDriver (servermgr.h:1337 declares
// it, "(client)").  The driver whose inline name matches, case-blind, or
// null.  __thiscall, `ret $4`; the bound is re-read from +0x44 every trip
// (0x00459b2e), the null driver is skipped (0x00459b1b), and 0x00492f30 is
// stricmp.  Sole caller: StartGame's "internet" lookup (0x00410132).
CBaseDriverQ2B* CNetMgrQ2B::FindDriver(char* pDriverName)
{
	uint32          i;
	CBaseDriverQ2B* pDriver;

	for (i = 0; i < m_Drivers.GetSize(); i++)
	{
		pDriver = m_Drivers.m_pArray[i];

		if (pDriver && stricmp(pDriver->m_Name, pDriverName) == 0)
			return pDriver;
	}

	return LTNULL;
}


// 0x00459c30, 135 bytes -- slot 1 of CNetBank's vftable 0x004a2b30
// ({0x0045a120 ??_G, 0x00459c30, 0x00459dc0, 0x0045a000}).  At /O1 the slot
// is `jmp` onto CNetBank::Allocate (0x10024162, netmgr.cpp's
// CNetBankAllocView::Allocate); at /O2 Allocate AND ServerMessage's
// constructor are expanded into it, and only ServerMessage::Init
// (0x00459cc0, `push $0x1000 ; push $0x1000`) is left as a call.
//
// The record is netmgr.cpp's CNetSentPacketView: three link words and a
// whole ServerMessage at +0x0c (0x70 bytes, the bank's element size).  The
// constructor's stores, in shipped order:
//   +0x00 = 0x004a2b44   CAbstractIO's vptr     (0x00459c6a)
//   +0x04 = 0            CAbstractIO::m_nRefCount
//   +0x08 = 0x004a2b68   LMessageImpl's vptr (m_Write)
//   +0x50 = 0x004a1ab8, then +0x54..+0x60 = 0, then Init(0, 0) through
//                        0x004283b0 -- CMoArray<uint8>'s in-class ctor
//   +0x00 = 0x004a2b58   ServerMessage's vptr   (0x00459c9d)
//   +0x3a = 1, +0x40 = 0, Init(0x1000, 0x1000)
// servermsg.cpp's ServerMessage() is `inline` but lives in servermsg.cpp,
// and CMsgPacketData's constructor is out of line, so neither the real
// ServerMessage nor netmgr.cpp can produce this expansion; the views below
// restate the three levels with in-class constructors.  (names ours)

class CAbstractIOQ11
{
public:
	CAbstractIOQ11() { m_nRefCount = 0; }

	virtual void _v00() = 0;
	virtual void AddRef() = 0;
	virtual void Release() = 0;
	virtual void _v0c() = 0;

	uint32 m_nRefCount;                                      // +0x04
};

// The two member vtables the constructor stores, the real classes' own:
// 0x004a2b68 is LMessageImpl's (servermsg.obj's ??_7LMessageImpl@@6B@) and
// 0x004a1ab8 is CMoArray<uint8>'s.  The views below hold the vptr as a plain
// field and store these by hand (clientmgr3.cpp's CCmgArray04 manner), so
// this TU emits no one-slot stand-in vtable of its own that a message would
// then be dispatched through.
extern void* g_LMessageImplVtblQ11[];                         // 0x004a2b68
extern void* g_CMoByteArrayVtblQ11[];                         // 0x004a1ab8

// servermsg.h's LMessageImpl as far as the constructor sees it: a vptr and
// nothing it initialises (Init sets m_pOwner at +0x10 later).
class LMessageImplQ11
{
public:
	LMessageImplQ11() { m_pVtbl = g_LMessageImplVtblQ11; }

	void** m_pVtbl;                                          // +0x00
	uint8  m_pad_04[0x2c - 0x04];
};

// packet.h's CPacketData (CMoArray<uint8>) as its constructor is shipped
// here: the vptr, four zeroed words (collection.h's Clear()), then Init(0, 0)
// CALLED (0x00459c8c -> 0x004283b0).  collection.h's own CMoArray::Init is
// in view and /O2 expands it (214 bytes against 135, measured), so the
// array is restated with Init out of line.
class CPacketDataQ11
{
public:
	CPacketDataQ11()
	{
		m_pVtbl      = g_CMoByteArrayVtblQ11;
		m_pArray     = LTNULL;
		m_nElements  = 0;
		m_nAllocated = 0;
		m_nCache     = 0;
		Init(0, 0);
	}

	LTBOOL Init(uint32 nSize, uint32 nCacheSize);            // 0x004283b0

	void** m_pVtbl;                                          // +0x00
	uint8* m_pArray;                                         // +0x04
	uint32 m_nElements;                                      // +0x08
	uint32 m_nAllocated;                                     // +0x0c
	uint32 m_nCache;                                         // +0x10
};

class ServerMessageQ11 : public CAbstractIOQ11
{
public:
	ServerMessageQ11()
	{
		// Shipped order (0x00459ca3, 0x00459ca9); the other order is
		// size-exact with these two stores swapped, measured.
		m_Unknown3a = 1;
		m_MsgFlags  = 0;

		Init(0x1000, 0x1000);
	}

	virtual void _v00();
	virtual void AddRef();
	virtual void Release();
	virtual void _v0c();

	void Init(uint16 nMaxDataLen, uint16 nGrowSize);         // 0x00459cc0

	LMessageImplQ11               m_Write;                   // +0x08
	uint16                        m_Unknown34;               // +0x34
	uint16                        m_MaxDataLen;              // +0x36
	uint16                        m_Unknown38;               // +0x38
	uint16                        m_Unknown3a;               // +0x3a
	void*                         m_pConnection;             // +0x3c
	uint32                        m_MsgFlags;                // +0x40
	uint8                         m_pad_44[0x50 - 0x44];     // +0x44 LTLink
	CPacketDataQ11                m_PacketData;              // +0x50
};

class CNetSentPacketQ11
{
public:
	uint32           m_Unknown00;                            // +0x00
	uint32           m_Unknown04;                            // +0x04
	uint32           m_Unknown08;                            // +0x08
	ServerMessageQ11 m_Packet;                               // +0x0c
};

// stdlith's StructBank, the fields sb_Allocate touches.
class CStructBankQ11
{
public:
	uint32 m_StructSize;                                     // +0x00
	uint32 m_Unknown04;                                      // +0x04
	uint32 m_nGrowCount;                                     // +0x08  sb_Grow's count
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                                      // +0x18
};

// 0x00490c20 -- sb_Grow (servermgr.h: 0x1003e1e0 in server.dll).
void sb_GrowQ11(CStructBankQ11* pBank, uint32 nCount);

// servermgr.h's inline sb_Allocate, against the view.
inline void* sb_AllocateQ11(CStructBankQ11* pBank)
{
	void* pStruct = pBank->m_pFreeList;

	if (!pStruct)
	{
		sb_GrowQ11(pBank, pBank->m_nGrowCount);

		pStruct = pBank->m_pFreeList;
		if (!pStruct)
			return LTNULL;
	}

	pBank->m_pFreeList = *(void**)pStruct;
	return pStruct;
}

// CNetBank (servermgr.h): a vptr and the struct bank at +0x04.  Slot 1 is
// declared non-virtual so this TU emits no vftable of its own; the body is
// the same either way.
class CNetBankQ11
{
public:
	CNetSentPacketQ11* Allocate();                           // 0x00459c30

	void*          m_pVtbl;                                  // +0x00
	CStructBankQ11 m_Bank;                                   // +0x04
};

CNetSentPacketQ11* CNetBankQ11::Allocate()
{
	CNetSentPacketQ11* pRec = (CNetSentPacketQ11*)sb_AllocateQ11(&m_Bank);

	if (pRec)
		pRec->m_Packet.ServerMessageQ11::ServerMessageQ11();

	return pRec;
}
