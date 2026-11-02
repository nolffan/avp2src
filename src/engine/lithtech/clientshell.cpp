// *********************************************************************** //
//
//  MODULE:   CLIENTSHELL (.CPP)   (file name ours)
//
//  PURPOSE:  lithtech.exe, the ten unwritten functions of the run
//            0x00413080 .. 0x00413cc1 -- CClientShell's constructor, its
//            teardown, its local-server startup, its per-frame Update, the
//            CServerMgr it makes and unmakes, and two compiler-generated
//            vector deleting destructors that landed in front of them.
//
//  WHAT NAMES THIS RUN
//  -------------------
//  Three shipped strings and one already-reconstructed neighbour:
//
//    * 0x004134ef pushes "CClientShell::StartupLocal"   -> 0x00413440
//    * 0x00413721 pushes "CClientShell::CreateServerMgr"-> 0x00413590
//    * 0x00413a75 pushes "CClientShell::Update"         -> 0x004139a0
//
//  and src/engine/lithtech/clientmgr2.cpp, which already names CClientShell
//  itself (off "CClientShell::DoLoadWorld" at 0x00414141), reconstructs
//  Init (0x00413310), SetUnknown30 (0x00413430), SendToServer (0x00413cd0),
//  ResetRecords (0x00413cf0) and UnloadWorld (0x004143d0), and names
//  cm_OnShellTerm (0x004115c0) which 0x00413350 calls first.  Every field
//  name below that clientmgr2.cpp already has is its name.
//
//  WHAT THIS FILE CORRECTS IN clientmgr2.cpp
//  -----------------------------------------
//  clientmgr2.cpp reads CClientShell as multiply derived, "two vptrs, at
//  +0x00 (0x004a1d18, five slots) and +0x04 (0x004a1d40, forty-eight)".
//  The constructor at 0x00413210 refutes the second half: it stores
//  0x004a1d18 into (this) and then fills this+0x04 .. this+0x13 with
//  SIXTEEN BYTES computed in a loop (`mov %al,0x4(%esi,%edi,1)`,
//  `cmp $0x10,%edi`).  0x004a1d40 is the second vptr of the 0xb2c-byte
//  object 0x00413590 news up -- CServerMgr -- whose own primary table is
//  0x004a1d54 and whose CBaseMgr base table 0x004a1d2c is the same base
//  CClientShell derives from (the destructor at 0x004132c0 restores it).
//  So CClientShell is SINGLY derived from CBaseMgr, and sizeof is 0x90:
//  its only construction site, 0x0040fdaa, is `push $0x90; call dalloc`.
//
//  THE MODULE
//  ----------
//  src/engine/lithtech/lt2new2.cpp already identified this TU by address --
//  "BELONGS IN: the client-shell TU (the run 0x00413200..0x004156xx)" --
//  when it put CObjectBank<T>::Free for the 0x54-byte element there.  This
//  file is the front of that run.
//
//  GROUND TRUTH: the retail lithtech.exe ONLY, cl 8447 /O2 (no /GX: the
//  vector deleting destructors below expand their loop inline instead of
//  calling __ehvec_dtor, which is what /GX would emit).
//
// *********************************************************************** //

#include "ltbasedefs.h"       // LTVector, StartGameRequest, LTRESULT
#include "ltlink.h"           // LTLink, LTList
#include "ltcodes.h"          // LT_OK, LT_SERVERERROR, LT_CANTCREATESERVER
#include "de_error.h"         // REPORT_ERROR / RETURN_ERROR (src/engine/server)
#include "collection.h"       // CMoArray, g_DefAlloc (src/engine/server)


// ======================================================================= //
//  THE ONE AMBIGUITY THAT RUNS THROUGH THE FILE
//
//  Both deleting destructors at the head of the run free one owned pointer
//  with `mov 0x4c(%esi),%eax; test %eax,%eax; je; push %eax;
//  call 0x004290c0; add $0x4,%esp` -- a null test in front of the engine's
//  one-argument `operator delete`.  That is what `delete m_p;` compiles to
//  IF VC6 emits the guard for a pointer whose pointee has no destructor.
//  It is written as `delete` below because that is what the source says;
//  src/engine/lithtech/cltrest.cpp reads the identical sequence at
//  0x00407c00 the same way.  If the measurement comes back one `test`
//  short, the single-edit flip is
//
//      #define DELETE_OWNED(p)  if (p) ::operator delete(p)
//
//  substituted for the two `delete` statements marked OWNED-FREE below.
//  Decide it once here rather than in each destructor.
// ======================================================================= //




// ----------------------------------------------------------------------- //
//  Free functions this run calls but does not define
// ----------------------------------------------------------------------- //

// src/engine/lithtech/stdlithalloc.cpp names both of these.
void* dalloc(unsigned long nSize);                             // 0x004290b0
void  dfree(void* pMem);                                       // 0x004290c0

// src/engine/server/structbank.cpp's, at the client's addresses.  sb_Free
// is inline in servermgr.h and expands wherever it is used, which is why
// CClientShell::Term carries it open-coded.
class StructBankView
{
public:
	uint32 m_StructSize;                           // +0x00
	uint32 m_Unknown04;                            // +0x04
	uint32 m_Unknown08;                            // +0x08
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                            // +0x18  intrusive list
};

void sb_Init(StructBankView* pBank, uint32 nStructSize, uint32 nCacheSize);
void sb_FreeChunks(StructBankView* pBank, int bAll);           // 0x00490bd0

inline void sb_Free(StructBankView* pBank, void* pStruct)
{
	*(void**)pStruct = pBank->m_pFreeList;
	pBank->m_pFreeList = pStruct;
}

// src/engine/lithtech/clientconsole.cpp names all three (src/engine/server/
// dhash.cpp's, at the client's addresses).
void* dhash_GetFirst(void* hHash);                             // 0x0042d0a0
void* dhash_GetNext(void** phIter);                            // 0x0042d0f0
void* dhash_GetData(void* hElement);                           // 0x0042d080
// The same hash's teardown, one past GetFirst's neighbours: one argument,
// __cdecl, and nothing comes back.  (name ours)
void  dhash_Term(void* hHash);                                 // 0x0042cdc0

// src/engine/lithtech/clientdsi.cpp's reader block: a zero-argument
// __cdecl predicate that four functions in this run gate on.  (name ours)
int   dsi_IsClientActive();                                    // 0x0042e690

// The client console's print entry (src/engine/lithtech/clientconsole.cpp
// and cltnew.cpp: __cdecl, colour first).                      0x0041b880
void  con_PrintF(uint32 nColour, int nLevel, char* pFormat);
// The console variable lookup lt2new1.cpp and clientrender.cpp both name.
class LTCommandVarView
{
public:
	uint8 m_pad_00[0x18];
	float m_FloatVal;                              // 0x18
};
LTCommandVarView* con_FindVar(void* pState, char* pName);      // 0x0041b3e0

// clientmgr2.cpp's console state global -- the OBJECT, not a pointer to
// it: 0x00413a01 pushes its address as an immediate (`push $0x4b2f00`) --
// and the console's message level the two printers below take as their
// first argument.
class ConsoleState;                   // servermgr.h's
extern ConsoleState g_ClientConsoleState;                      // 0x004b2f00
// 0x004b17e4 is THIS TU's: it is .bss, and the dynamic initialiser that
// sets it -- 0x00413200, `movl $0xff00,0x4b17e4; ret`, reached through the
// .CRT$XCU thunk 0x004131f0 (`jmp 0x00413200`) -- sits inside this file's
// address run.  A constant initialiser would have put it in .data instead,
// so the value is computed; 0xff00 is green in the console's 0xRRGGBB.
// (names ours)
inline uint32 cs_ConsoleRGB(uint32 r, uint32 g, uint32 b)
{
	return (r << 16) | (g << 8) | b;
}

uint32 g_ConsoleColour = cs_ConsoleRGB(0, 255, 0);             // 0x004b17e4

// The engine's default allocator; its ADDRESS is what every collection
// destructor pushes (src/engine/lithtech/stdlithalloc.cpp).
// (g_DefAlloc, 0x004b4e88, is collection.h's.)

// clientmgr2.cpp's manager-side half of the shell teardown.
class CClientMgrView;
class CClientShell;
void cm_OnShellTerm(CClientMgrView* pClientMgr, CClientShell* pShell);  // 0x004115c0

// The three free helpers this run reaches that nothing at the call site
// names.  What the bytes say:
//   0x00461270(pShell)  — run once per frame, gated on dsi_IsClientActive.
//   0x00415bb0()        — __thiscall on the CClientMgr, no arguments.
//   0x00421fc0(pNode)   — __cdecl, one argument, inside the node walk of
//                         0x00413b40.
void sub_461270(CClientShell* pShell);                         // 0x00461270
void sub_421fc0(void* pNode);                                  // 0x00421fc0

// The global the shell publishes itself into (CClientShell::Init, written
// in clientmgr2.cpp).  Defined here: it is .bss inside this TU's run, two
// dwords after g_ConsoleColour (0x004b17e4).
CClientShell* g_pClientShell;                                  // 0x004b17ec
// clientmgr.h's, reached by Update to find out whether it is the shell the
// manager is driving.
extern CClientMgrView* g_pClientMgr;                           // 0x004b17e0
// Update's gate: a dword in .data that is 1 in the shipped image and is
// tested before the server is stepped at all.  (name ours)
// It is the "UpdateServer" console command's variable (clientconsole.cpp's
// cc_Sub41de60 atoi's into it), 1 in the shipped .data, which lies in
// src/engine/server/dsi.cpp's initialised run (0x004a9ef8 MaxExtrapolateTime
// .. 0x004a9f00 FarZ; server.dll 0x10051c94, also 1) -- defined there, under
// clientconsole.cpp's spelling of it.
extern int g_nUnknown4a9efc;                                   // 0x004a9efc


// ======================================================================= //
//  1.  THE TWO VECTOR DELETING DESTRUCTORS — 0x00413080, 0x00413110
//
//  Neither is hand-written: each is `??_E<T>`, the compiler-generated
//  VECTOR deleting destructor, and the shape is unmistakable.
//
//    * one byte-wide stack argument used only as a flag pair: `test $0x2`
//      selects the array arm and `test $0x1` the free, and `ret $0x4` plus
//      `mov %esi,%eax` gives the `void* f(unsigned int)` signature;
//    * the array arm reads the element COUNT from `-0x4(%esi)` and walks
//      BACKWARDS from the end (`lea 0x4c(%eax),%esi` then `sub $0x54,%esi`),
//      which is the cookie layout `delete[]` uses and nothing else;
//    * the loop is expanded inline rather than handed to __ehvec_dtor,
//      which is what /GX-off costs it.
//
//  So the SOURCE that produces them is a `delete[]` of each class, and the
//  shipped call sites say so directly — 0x0043fba2 and 0x0043fc52 both read
//  an array pointer out of a record, test it, and call with the flag 3
//  (array + free):
//
//      mov 0x8(%esi),%ecx ; cmp %edi,%ecx ; je ; push $0x3 ; call 0x00413080
//
//  WHAT THE TWO CLASSES ARE, off their own bytes and nothing else:
//
//    * element STRIDE.  0x00413080 forms `esi + 84*n` out of
//      `lea 0x0(,%ecx,8); sub %ecx; lea (%eax,%eax,2)` (8n-n=7n, then 21n,
//      then *4), so sizeof is 0x54.  0x00413110 forms `esi + 80*n` out of
//      `lea (%eax,%eax,4); shl $0x4`, so sizeof is 0x50.
//    * the DESTRUCTOR is one statement, identical in both: an owned pointer
//      at +0x4c released through the engine's one-argument operator delete.
//    * each class has its OWN `operator delete`, and it is a free-list push,
//      not a free: the scalar arm expands `if (p) { p->next = head;
//      head = p; }` against a different head in each (0x004b3f60 and
//      0x004b3f78) while the array arm calls the out-of-line global.  The
//      two heads sit 0x18 apart in the same block of list roots the caller
//      at 0x0043fb80 walks, so these are two element types of one pooled
//      registry.  Nothing in the shipped code names either; they keep their
//      sizes for names.
//
//  CONFIDENCE: MEDIUM-HIGH.  Everything above is read off the bytes and the
//  code is the compiler's, so there is no statement order to get wrong.
//  The one build question is the guard in front of the owned free — see the
//  note at the top of this file.
// ======================================================================= //

// The two free-list heads are not globals of their own: 0x004b3f60 and
// 0x004b3f78 are the m_pFree (+0x10) of the DirectMusic record banks at
// 0x004b3f50 and 0x004b3f68, which cltcursor.cpp defines as CDMBanks::
// s_SegmentBank and CDMBanks::s_Unknown58Bank (dmextra.cpp spells the same
// push through them).  Declared here only as far as the field; the bank
// template's members are cltcursor.cpp's.
struct CDMSegmentRecord;
struct CDMUnknown58Record;

template<class T>
class CDMRecordBank
{
public:
	uint8  m_pad_00[0x10];
	void*  m_pFree;                                // +0x10
	uint32 m_Unknown14;                            // +0x14
};

class CDMBanks
{
public:
	static CDMRecordBank<CDMSegmentRecord>   s_SegmentBank;   // 0x004b3f50
	static CDMRecordBank<CDMUnknown58Record> s_Unknown58Bank; // 0x004b3f68
};

// 0x54 bytes, one owned pointer at 0x4c.
class CPooled54
{
public:
	~CPooled54()
	{
		if (m_pOwned4c)                            // OWNED-FREE
			::operator delete(m_pOwned4c);
	}

	void operator delete(void* p)
	{
		if (p)
		{
			*(void**)p = CDMBanks::s_SegmentBank.m_pFree;
			CDMBanks::s_SegmentBank.m_pFree = p;
		}
	}

	uint8  m_pad_00[0x4c];
	uint8* m_pOwned4c;                             // 0x4c
	uint8  m_pad_50[0x54 - 0x50];
};

// 0x50 bytes, the same owned pointer at the same offset.
class CPooled50
{
public:
	~CPooled50()
	{
		if (m_pOwned4c)                            // OWNED-FREE
			::operator delete(m_pOwned4c);
	}

	void operator delete(void* p)
	{
		if (p)
		{
			*(void**)p = CDMBanks::s_Unknown58Bank.m_pFree;
			CDMBanks::s_Unknown58Bank.m_pFree = p;
		}
	}

	uint8  m_pad_00[0x4c];
	uint8* m_pOwned4c;                             // 0x4c
};

// The two `delete[]` sites that make VC6 emit the destructors above.  The
// shipped callers are CLTDirectMusicMgr::Term's bank teardown (0x0043fba4,
// 0x0043fc54, src/engine/lithtech/dmextra.cpp); these stand in for them so
// the COMDATs land in this object.  STAND-INS, names ours.
//
// MEASURED: VC6 expands a `??_E` at every `delete[]` site -- at /O2, at /Od,
// under a spent inline budget -- and the only way found to get the shipped
// out-of-line call is the inline DEPTH: with the `delete[]` eight inline
// levels down the site becomes `push 3; call ??_E`, and the COMDAT is then
// compiled on its own with the destructor and the class `operator delete`
// expanded inside it, byte for byte the shipped 0x00413080 / 0x00413110.
// Seven levels leaves `??_E` expanded and the destructor called; six and
// fewer expand everything.  (#pragma inline_depth(0) also forces the call
// but compiles the COMDAT itself at depth 0, destructor as a call -- wrong.)
// The shipped source presumably reached this depth through its bank
// templates; this chain is only the depth.
//
// The destructors' owned free is guarded -- the DELETE_OWNED flip the note
// at the top of this file anticipated: shipped has `test %eax,%eax; je`
// before the call to 0x004290c0 in both arms, and `delete` of a uint8*
// emits no test.
template<class T> inline void pool_Del1(T* p) { delete[] p; }
template<class T> inline void pool_Del2(T* p) { pool_Del1(p); }
template<class T> inline void pool_Del3(T* p) { pool_Del2(p); }
template<class T> inline void pool_Del4(T* p) { pool_Del3(p); }
template<class T> inline void pool_Del5(T* p) { pool_Del4(p); }
template<class T> inline void pool_Del6(T* p) { pool_Del5(p); }
template<class T> inline void pool_Del7(T* p) { pool_Del6(p); }
template<class T> inline void pool_Del8(T* p) { pool_Del7(p); }

void pool_FreeArray54(CPooled54* pArray)
{
	pool_Del8(pArray);
}

void pool_FreeArray50(CPooled50* pArray)
{
	pool_Del8(pArray);
}


// ======================================================================= //
//  2.  THE VIEWS
// ======================================================================= //

// The client's CNetMgr, 0x104 bytes (0x50c .. 0x60f inside CClientMgr).
// Only the five entries this run reaches are declared; all five are
// __thiscall on the sub-object, which is why every call site loads its
// ADDRESS into %ecx and pushes nothing for `this`.
class CNetMgrView
{
public:
	CNetMgrView();                                              // 0x00455310
	~CNetMgrView();                                             // 0x00455410

	// ("Client: ", pClientMgr->m_Unknown125c, 1) from both StartupLocal and
	// Update — the per-frame service of the client's own connection.
	void  Update(char* pPrefix, uint32 nUnknown, int nFlag);    // 0x00455790
	// ("local") -> the connection object the shell keeps at +0x30.
	class CConnObj* Connect(char* pDriverName);                 // 0x00456010
	// (m_nConnID, 0, 0) from Term.
	void  Disconnect(uint32 nConnID, uint32 n2, uint32 n3);     // 0x00456320
	// (&pClientMgr->m_NetMgr.m_Unknownf0) from CreateServerMgr — the server
	// manager's net manager is handed something out of the client's.
	void  Sub459ac0(void* p);                                   // 0x00459ac0
	// servermgr.h's CNetMgr packet pump, at the client's addresses.
	void   StartGettingPackets();                               // 0x004563e0
	void   EndGettingPackets();                                 // 0x004563f0
	LTBOOL GetPacket(class ILTMessageView* pMsg);               // 0x00457840
	// (msg, connection id, 0x80) from CClientShell::Sub47bbb0.
	LTBOOL SendToConnection(class ILTMessageView* pMsg, uint32 nConnID,
	                        uint32 nFlags);                     // 0x00456360

	uint8  m_pad_00[0x30];
	// 0x53c absolute in CClientMgr -- Term runs one extra teardown step
	// only while this is above zero (`cmp %edi,0x53c(%ecx); jbe`, so the
	// test is unsigned and the source is `> 0`).
	uint32 m_Unknown30;                                         // +0x30
	uint8  m_pad_34[0x3c - 0x34];
	// servermgr.h's CNetMgr::m_Drivers: CMoArray<CBaseDriver*>.
	CMoArray<class CDriverView*> m_Drivers;                     // +0x3c
	uint8  m_pad_50[0xb4 - 0x50];

	void   ForgetDriver(class CDriverView* pDriver)
	{
		uint32 index = m_Drivers.FindElement(pDriver);

		if (index != BAD_INDEX)
			m_Drivers.Remove(index);
	}

	// 0x5c0 absolute: the back pointer clientmgr2.cpp's CClientShell::Init
	// fills in and Term clears.
	CClientShell* m_pShell;                                     // +0xb4
	uint8  m_pad_b8[0xf0 - 0xb8];
	uint8  m_Unknownf0[0x100 - 0xf0];                           // +0xf0
	// servermgr.h's m_pMainDriver.
	class CDriverView* m_pMainDriver;                           // +0x100
};

// What CClientShell+0x30 is: one virtual, slot 4, taking one argument.
class CConnObj
{
public:
	virtual void _p00()=0; virtual void _p04()=0;
	virtual void _p08()=0; virtual void _p0c()=0;
	virtual void OnConnect(void* p)=0;                          // 0x10
};

// The game DLL's shell, as far as Update reaches it.  Three adjacent
// no-argument virtuals at 0x30, 0x34 and 0x38 — slots 12, 13 and 14 by the
// same count clientmgr2.cpp establishes off src/LT2/sdk/inc/iclientshell.h
// (~IClientShell at slot 0).
class IClientShellView
{
public:
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0; virtual void _p1c()=0; virtual void _p20()=0;
	virtual void _p24()=0; virtual void _p28()=0; virtual void _p2c()=0;
	virtual void PreUpdate()=0;                                 // 0x30
	virtual void Update()=0;                                    // 0x34
	virtual void PostUpdate()=0;                                // 0x38
	virtual void _p3c()=0; virtual void _p40()=0; virtual void _p44()=0;
	virtual void _p48()=0;
	// (1, 0) from CClientShell's disconnect notification.
	virtual void OnEvent(uint32 nEvent, uint32 nParam)=0;       // 0x4c
};

// The record 0x00413b40 walks.  Every field is a shipped displacement.
class CNodeOwner;

class CRecF4
{
public:
	CNodeOwner* m_pOwner;                          // 0x00
	class CPosRec* m_pRec;                         // 0x04
	uint32      m_Unknown08;                       // 0x08
	class CRecCallbacks* m_pCallbacks;             // 0x0c
	CRecF4*     m_pNext;                           // 0x10
};

// The three-vector payload the callback is handed, and the record it comes
// out of.  Nine dwords copied out, the callback run, the same nine copied
// back: the vectors are the only thing that can change.
class CPosRec
{
public:
	uint32   m_Unknown00;                          // 0x00
	LTVector m_Vec04;                              // 0x04
	LTVector m_Vec10;                              // 0x10
	LTVector m_Vec1c;                              // 0x1c
	uint8    m_pad_28[0x40 - 0x28];
	uint16   m_nNextNode;                          // 0x40  0xffff terminates
};

class CNodeOwner
{
public:
	uint8     m_pad_00[0xa0];
	CPosRec** m_pNodes;                            // 0xa0
};

// The argument block, laid out exactly as the shipped stores build it:
// three vectors and then the three pointers, 0x30 bytes in all.
class CPosRequest
{
public:
	LTVector        m_Vec00;                       // 0x00
	LTVector        m_Vec0c;                       // 0x0c
	LTVector        m_Vec18;                       // 0x18
	CClientMgrView* m_pClientMgr;                  // 0x24
	CNodeOwner*     m_pOwner;                      // 0x28
	CPosRec*        m_pRec;                        // 0x2c
};

// A table of __cdecl entries, not a vtable: the call is `mov 0xc(%esi),%eax;
// ... call *0x4(%eax)` with `add $0x8,%esp` after it, so nothing is passed
// as `this` and the caller cleans up.
class CRecCallbacks
{
public:
	void* m_pEntry00;                                                  // 0x00
	int   (*m_pGetPos)(CPosRequest* pRequest, uint32 nUnknown);        // 0x04
};

// The message this run builds, as far as it reaches it.  cltnew.cpp already
// pinned m_pData at +0x54 ("SendToServer stamps the message type into
// element zero of it").
class ILTMessageView
{
public:
	virtual void _p00()=0;
	virtual void AddRef()=0;                                    // 0x04
	virtual void Release()=0;                                   // 0x08

	uint8  m_pad_04[0x54 - 0x04];
	uint8* m_pData;                                             // 0x54

	// __thiscall, one dword, callee-cleaned.                    0x0045c350
	void   WriteDWord(uint32 n);
};

// The by-value holder 0x0045c080 hands back.  Four bytes, and both halves
// of the temporary dance the shipped bytes show are its: the copy that
// AddRefs and the destructor that Releases.  MSVC returns it through a
// hidden pointer argument (pushed LAST, which is why the shipped `push %eax`
// sits after the two 0x1000s) because it is not trivially copyable.
class CMsgRef
{
public:
	CMsgRef(const CMsgRef& other)
	{
		m_pMsg = other.m_pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();
	}

	~CMsgRef()
	{
		if (m_pMsg)
			m_pMsg->Release();
	}

	ILTMessageView* m_pMsg;                                     // 0x00
};

// 0x0045c080 — a fresh write message of the given size.  __cdecl and it
// returns the holder by value.  (name ours)
CMsgRef msg_StartWrite(uint32 nMaxSize, uint32 nSize);

// CClientMgr, as far as this run reaches it.  Every offset is a shipped
// displacement; clientmgr.h / clientmgr2.cpp / clientrender.cpp already
// carry 0x050c, 0x061c, 0x0620, 0x150c and 0x1534, and this run adds the
// rest.
class CClientMgrView
{
public:
	uint8            m_pad_0000[0x0f4];
	// The chain 0x00413b40 walks; it follows +0x10 to null.
	CRecF4*          m_pChainF4;                  // 0x0000f4
	uint8            m_pad_00f8[0x50c - 0x0f8];
	CNetMgrView      m_NetMgr;                    // 0x00050c  (clientmgr2.cpp)
	uint8            m_pad_0610[0x61c - 0x610];
	IClientShellView* m_pClientShell;             // 0x00061c  (clientmgr.h)
	// clientrender.cpp: the 0x12c-byte string SetupError formats into.
	char             m_ErrorString[0x12c];        // 0x000620
	uint8            m_pad_074c[0x1120 - 0x74c];
	// Reached only by sb_Free's inline expansion in CClientShell::Term:
	// `add $0x1120,%ecx` and then the bank's +0x18 free list.
	StructBankView   m_Bank1120;                  // 0x001120
	uint8            m_pad_113c[0x125c - 0x113c];
	// The second argument of both CNetMgr updates in this run.
	uint32           m_Unknown125c;               // 0x00125c
	uint8            m_pad_1260[0x150c - 0x1260];
	CClientShell*    m_pClientShell2;             // 0x00150c  (clientmgr.h:
	                                              //            m_pConnection)
	uint8            m_pad_1510[0x1534 - 0x1510];
	// The file manager every unhandled server packet goes to (cchandler.cpp,
	// clientdsi.cpp and clientwin.cpp all reach it here).
	void*            m_pFileMgr;                  // 0x001534
	// Handed to the server manager by address, with the count that follows
	// it: `add $0x1538,%eax` and `mov 0x1588(%eax),%ecx` in StartupLocal.
	uint8            m_Resources1538[0x1588 - 0x1538];   // 0x001538
	uint32           m_nResources1588;            // 0x001588

	// A varargs member is __cdecl, which is why the shipped call pushes
	// `this` LAST and cleans up itself (clientrender.cpp).      0x0041fc60
	LTRESULT SetupError(LTRESULT dResult, ...);
	// __thiscall, no arguments; clientmgr2.cpp lists it as "0x00415bb0
	// (six cdecl calls in a row on one pointer)" and nothing at this call
	// site names it.
	void     Sub415bb0();                                       // 0x00415bb0
};

// The engine's own client shell.  sizeof is 0x90: its only construction
// site, 0x0040fda5, is `push $0x90; call dalloc` straight in front of the
// only call to the constructor below.
class CServerMgrView;

// servermgr.h's CBaseMgr, the base CClientShell and CServerMgr share (its
// table is 0x004a1d2c).  Defined here, ahead of CClientShell, because the
// shell derives from it: the shell's destructor (0x004132c0) restores
// 0x004a1d2c after Term, which is this base's inlined empty destructor.
class CBaseMgrView
{
public:
	// SLOT 0 IS THE DESTRUCTOR, not an unnamed virtual. `.rdata`
	// 0x004a1d2c[0] is 0x004132f0, the base's own scalar deleting
	// destructor, and ??_GCServerMgrView ends `mov [esi],0x4a1d2c` with NO
	// call -- an inlined EMPTY base destructor, which is not the same as
	// having none. One byte, one instruction in CClientShell::Term.
	virtual ~CBaseMgrView() {}
};

class CClientShell : public CBaseMgrView
{
public:
	CClientShell();                                             // 0x00413210

	// The five slots of 0x004a1d18, in table
	// order.  They are declared and not
	// defined: none of them is in this run, and what the declaration buys
	// is the ONE vptr store the constructor below makes (`movl $0x4a1d18,
	// (%esi)`).  CClientShell is singly derived -- see the header note --
	// so that store is the only one, which is why no base is modelled.
	virtual ~CClientShell();                                    // 0x004132c0
	virtual LTBOOL _v04(uint32 nConnID, LTBOOL bLocal);         // 0x00477f40
	virtual void _v08(void* pConn);                             // 0x00477f60
	// Slot 3 is CBaseMgr's ProcessPacket (servermgr.h); the client's
	// override is empty -- 0x00477fa0 is `ret $0xc` and nothing else.
	virtual void ProcessPacket(class CPacket* pPacket, uint8* pSenderAddr,
	                           uint16 nSenderPort);             // 0x00477fa0
	virtual void _v10();                                        // 0x00477fb0

	void     Term();                                            // 0x00413350
	LTRESULT StartupLocal(StartGameRequest* pRequest, LTBOOL bHost,
	                      uint32 nGameType);                     // 0x00413440
	LTRESULT CreateServerMgr();                                 // 0x00413590
	LTRESULT Update();                                          // 0x004139a0
	void     SendMessage0A(uint32 nData);                       // 0x00413c60

	// clientmgr2.cpp's and clientmgr.h's, all outside this run.
	LTBOOL   Init(CClientMgrView* pClientMgr);                  // 0x00413310
	LTRESULT SetUnknown30(uint32 nValue);                       // 0x00413430
	LTRESULT SendToServer(ILTMessageView* pMsg);                // 0x00413cd0
	void     ResetRecords();                                    // 0x00413cf0
	void     UnloadWorld();                                     // 0x004143d0
	void     Sub4143f0();                                       // 0x004143f0
	// The three more this run reaches on itself; nothing at the call site
	// names any of them.
	void     Sub47bbb0();                                       // 0x0047bbb0
	LTRESULT Sub47bab0();                                       // 0x0047bab0
	uint32   Sub47bcd0();                                       // 0x0047bcd0

	// 0x000 is the vptr (&0x004a1d18), which the virtuals above supply.
	// Sixteen bytes the constructor fills with a ramp; nothing else in the
	// run reads them.  (name ours)
	uint8            m_Ramp[0x10];                // 0x004
	float            m_Unknown14;                 // 0x014  previous 0x018
	float            m_Unknown18;                 // 0x018
	float            m_Unknown1c;                 // 0x01c  0x018 - 0x014
	uint8            m_pad_0020[0x30 - 0x20];
	// clientmgr2.cpp's SetUnknown30 writes this; StartupLocal fills it with
	// whatever the manager's CNetMgr answers and then calls slot 4 on it.
	CConnObj*        m_pConnection;               // 0x030
	uint32           m_nConnID;                   // 0x034  (clientmgr2.cpp)
	uint16           m_Unknown38;                 // 0x038  0xffff at birth
	uint8            m_pad_003a[0x3c - 0x3a];
	LTBOOL           m_bLocal;                    // 0x03c  1 in StartupLocal
	CServerMgrView*  m_pServerMgr;                // 0x040
	LTLink           m_Link44;                    // 0x044
	LTLink           m_Link50;                    // 0x050
	LTBOOL           m_bOnServer;                 // 0x05c  1 on connect (_v04)
	uint8            m_pad_0060[0x6c - 0x60];
	uint16           m_Unknown6c;                 // 0x06c  (clientmgr2.cpp)
	uint8            m_pad_006e[0x70 - 0x6e];
	uint32           m_Unknown70;                 // 0x070
	uint32           m_Unknown74;                 // 0x074
	CClientMgrView*  m_pClientMgr;                // 0x078  (clientmgr2.cpp)
	LTBOOL           m_bWorldLoaded;              // 0x07c  (clientmgr2.cpp)
	uint32           m_Unknown80;                 // 0x080
	uint8            m_pad_0084[0x88 - 0x84];
	uint32           m_Unknown88;                 // 0x088
	void*            m_pUnknown8c;                // 0x08c  (clientmgr2.cpp)
};


// ======================================================================= //
//  3.  CServerMgr, AS THIS RUN BUILDS AND UNBUILDS IT
//
//  The 0xb2c-byte object 0x00413590 news up IS src/engine/server/
//  servermgr.h's CServerMgr, and three independent things say so:
//
//    * SIZE.  `push $0xb2c; call 0x004290b0`.  servermgr.h's last member,
//      m_pPlaceholderModel, is at 0xb28 — so that header's class is 0xb2c
//      bytes to the byte.
//    * BASES.  The object takes vptrs at +0x00 and +0x04, and the first of
//      them is 0x004a1d2c, which is the SAME table CClientShell's
//      destructor (0x004132c0) restores.  servermgr.h says
//      `class CServerMgr : public CBaseMgr, public CServerSoundMgr`, and
//      the decoded symbol map already places CBaseMgr::_v10 at
//      0x004132e0, slot 4 of that very table.
//    * MEMBERS.  Every constructed offset lands on a member servermgr.h
//      names: 0x044/0x054 (m_List44/m_List54, tied off inline exactly as
//      LTList's inline constructor does it), 0x1a4 (m_World), 0x668
//      (m_ForceControl, and the LTVector it is handed is (0, -2000, 0) —
//      gravity), 0x6e0 (m_NetMgr), 0x7e4 (m_ClientList), 0x80c
//      (m_ClassMgr), 0x8dc/0x8ec (m_List8dc/m_List8ec), 0x8fc
//      (m_Unknown8fc), 0x92c (m_SkyDef), 0x9f0 (m_MessageBank), 0xa14
//      (m_UnknownA14), 0xa2c (m_FileMgr — and the constructor at 0x00413980
//      is `sb_Init(&bank, 0x14, 0x20)` on a bank at its +0x18, which is
//      CMessageBank's shape one level down), 0xac4 (m_Thread).  The body
//      after the vptrs is m_ClientServerType = 1 and the two flush counters
//      at 0x7fc/0x800 zeroed — the three things servermgr.h's comments say
//      the constructor does.
//
//  WHY IT IS A LOCAL VIEW AND NOT THAT HEADER.  servermgr.h declares
//  `CServerMgr();` and `virtual ~CServerMgr();` OUT OF LINE.  The shipped
//  client has both of them EXPANDED — the constructor inline inside
//  CreateServerMgr, the destructor inline inside the `??_G` at 0x00413740 —
//  so in the original they were defined in the class body.  Including that
//  header would emit two calls where the image has forty-nine.  The view
//  below therefore declares the same object with the same member offsets
//  and the ctor/dtor written in-class; when servermgr.h's pair moves into
//  its class body this view goes away.
//
//  ONE DISAGREEMENT WITH servermgr.h, recorded rather than resolved.  That
//  header reads 0x1ac/0x1b0 as `LightAnim* m_pLightAnims; uint32
//  m_nLightAnims;`.  The destructor here destroys FIVE 0x14-byte collection
//  members at 0x1a8, 0x1bc, 0x1d0, 0x1e4 and 0x1f8, each with the same
//  `movl $vtbl,(%ecx); push $0x4b4e88; push $0x0; call <Term>` shape the
//  client's CMoArray<T> destructor has.  0x1ac and 0x1b0 are then the first
//  collection's m_pArray and m_nElements, which is the same two values that
//  header is reading, one level in.  Nothing here settles it; the view
//  models what this function's bytes show.
//
//  THE VPTR STORES (what closed ??_GCServerMgrView, 0x00413740).  The
//  shipped constructor stores no member vptrs because the members that have
//  them live inside two aggregates with OUT-OF-LINE constructors: CWorld1a4
//  (0x004222f0, which stores the collection vptrs itself) and CSub390
//  (0x0045a470, the eleven object banks).  Both destructors are the
//  compiler's.  The collections are CMoArray-shaped with a NON-virtual
//  `~T() { Term(); }` over `Term() { SetSize2(0, &g_DefAlloc); }` (virtual
//  destructors cost the inline budget three expansions); the banks are
//  objbank.cpp's CObjectBank<T> over a base whose empty virtual destructor
//  is the 0x004a14c8 restore.  VC6 expands only the last three of CSub390's
//  eleven banks (the budget is per inlined body, spent from the end), which
//  is exactly the shipped 3-inline / 8-call split -- and it does so only
//  while CSub390's destructor is IMPLICIT (an explicit `{}` loses 0x518).
// ======================================================================= //

// The 12-byte vector the gravity temporary is.  It IS ltvector.h's
// LTVector -- src/engine/lithtech/clientmgr2.cpp identifies 0x00411d40 as
// "a three-float constructor returning `this`, `ret $0xc` -- LTVector's,
// from ltvector.h", and records the very same (0.0f, -2000.0f, 0.0f) being
// built at 0x00410ab2 in CClientMgr's own constructor.  It is declared
// locally rather than used from the header for ONE reason: the shipped
// constructor CALLS it, and LTVector's is an inline whose expansion is at
// VC6's discretion, so spelling it as an inline risks losing the call this
// function is being scored on.  For a byte attempt, put LTVector back.
class CVec3
{
public:
	CVec3(float x, float y, float z);                          // 0x00411d40

	float m_x, m_y, m_z;
};

// The collection members, one class per instantiation because each has its
// own out-of-line Term.  All are 0x14 bytes: vptr, m_pArray, m_nElements,
// m_CacheSize, m_WantedCache (src/engine/server/collection.h's CMoArray<T>,
// which cltnew.cpp already pinned field for field at CPacket+0x50).
#define DECLARE_MOARRAY(cls)                                 \
	class cls                                                \
	{                                                        \
	public:                                                  \
		~cls() { Term(); }                                   \
		void  Term() { SetSize2(0, &g_DefAlloc); }           \
		int   SetSize2(uint32 nNewSize, LAlloc* pAlloc);     \
		virtual void _g00();                                 \
		uint8 m_pad_04[0x14 - 0x04];                         \
	}

DECLARE_MOARRAY(CArray1a8);                                    // Term 0x00414f00
DECLARE_MOARRAY(CArray1bc);                                    // Term 0x00414fa0
DECLARE_MOARRAY(CArray1d0);                                    // Term 0x00414fa0
DECLARE_MOARRAY(CArray1e4);                                    // Term 0x0040bfe0
DECLARE_MOARRAY(CArray1f8);                                    // Term 0x00415050
DECLARE_MOARRAY(CArray32c);                                    // Term 0x00414fa0
DECLARE_MOARRAY(CArray340);                                    // Term 0x00414fa0

// servermgr.h's CServerWorld.  Its constructor is out of line (0x004222f0)
// and it has no destructor of its own; what the shipped teardown runs is
// the collection it carries at +0x04.

// A bank with the shape servermgr.h's CMessageBank has: a vptr, a
// StructBank at +0x04 and one dword, destroyed by handing the bank's
// chunks back.  Three of them sit in a row at 0x518/0x53c/0x560 with no
// constructor reached from here, and a fourth at 0x9f0 with one.
// The banks' common base: its table (0x004a14c8) is what every bank's
// destructor restores on the way out, and nothing else runs.
class CBankBaseView
{
public:
	virtual ~CBankBaseView() {}
	virtual void* Allocate() = 0;
	virtual void  Free(void* pObj) = 0;
	virtual void  Init() = 0;
};

#define DECLARE_BANK(cls)                                    \
	class cls : public CBankBaseView                         \
	{                                                        \
	public:                                                  \
		virtual ~cls() { sb_FreeChunks(&m_Bank, 1); }        \
		virtual void* Allocate();                            \
		virtual void  Free(void* pObj);                      \
		virtual void  Init();                                \
		StructBankView m_Bank;                               \
		uint32         m_Unknown20;                          \
	}


// objbank.cpp's CObjectBank<T>: the eleven object banks of CSub390 are one
// template.  Only the element type differs, so each instantiation is its own
// class with its own table, as shipped.
template<class T>
class CObjBankView : public CBankBaseView
{
public:
	virtual ~CObjBankView() { sb_FreeChunks(&m_Bank, 1); }
	virtual void* Allocate();
	virtual void  Free(void* pObj);
	virtual void  Init();

	StructBankView m_Bank;
	uint32         m_Unknown20;
};

#define DECLARE_ELEM(cls) class cls { public: uint8 m_pad[4]; }
DECLARE_ELEM(CObjElem3f8);
DECLARE_ELEM(CObjElem41c);
DECLARE_ELEM(CObjElem440);
DECLARE_ELEM(CObjElem464);
DECLARE_ELEM(CObjElem488);
DECLARE_ELEM(CObjElem4ac);
DECLARE_ELEM(CObjElem4d0);
DECLARE_ELEM(CObjElem4f4);
DECLARE_ELEM(CObjElem518);
DECLARE_ELEM(CObjElem53c);
DECLARE_ELEM(CObjElem560);
DECLARE_BANK(CBankA44);

// The same bank with the constructor the shipped code calls (0x004151f0 —
// servermgr.h's CMessageBank, whose body is sb_Init(&m_Bank, 0x54, 0x20)).
class CMessageBankView : public CBankBaseView
{
public:
	CMessageBankView();                                        // 0x004151f0
	virtual ~CMessageBankView() { sb_FreeChunks(&m_Bank, 1); }
	virtual void* Allocate();
	virtual void  Free(void* pObj);
	virtual void  Init();

	StructBankView m_Bank;
	uint32         m_Unknown20;
};

// The members with plain out-of-line destructors and nothing else reached.
#define DECLARE_OPAQUE_DTOR(cls, size)                       \
	class cls                                                \
	{                                                        \
	public:                                                  \
		~cls();                                              \
		uint8 m_pad_00[size];                                \
	}

DECLARE_OPAQUE_DTOR(CSub21c, 0x2a0 - 0x21c);                   // dtor 0x0048c790
DECLARE_OPAQUE_DTOR(CSub2a0, 0x32c - 0x2a0);                   // dtor 0x0043c6e0

// 0x1a4: servermgr.h's CServerWorld.  Its constructor is out of line
// (0x004222f0, which builds every member below -- the collections' vptr
// stores are there, not in CreateServerMgr) and its destructor is the
// compiler's, expanded into ??_GCServerMgrView.
class CWorld1a4
{
public:
	CWorld1a4();                                               // 0x004222f0
	~CWorld1a4() {}

	uint32     m_Unknown00;                                    // 0x1a4
	CArray1a8  m_Array1a8;                                     // 0x1a8
	CArray1bc  m_Array1bc;                                     // 0x1bc
	CArray1d0  m_Array1d0;                                     // 0x1d0
	CArray1e4  m_Array1e4;                                     // 0x1e4
	CArray1f8  m_Array1f8;                                     // 0x1f8
	uint8      m_pad_20c[0x21c - 0x20c];
	CSub21c    m_Sub21c;                                       // 0x21c
	CSub2a0    m_Sub2a0;                                       // 0x2a0
	CArray32c  m_Array32c;                                     // 0x32c
	CArray340  m_Array340;                                     // 0x340
	uint8      m_pad_354[0x390 - 0x354];
};

// 0x390: one object with its own out-of-line constructor (0x0045a470, which
// builds the 0x24-byte members and the three banks) and a compiler-written
// destructor.  Its vptr at +0 is stored only by that constructor.
class CSub390
{
public:
	CSub390();                                                 // 0x0045a470
	virtual void _p00();

	uint8     m_pad_04[0x3f8 - 0x394];
	CObjBankView<CObjElem3f8> m_Sub3f8;                                        // 0x3f8
	CObjBankView<CObjElem41c> m_Sub41c;                                        // 0x41c
	CObjBankView<CObjElem440> m_Sub440;                                        // 0x440
	CObjBankView<CObjElem464> m_Sub464;                                        // 0x464
	CObjBankView<CObjElem488> m_Sub488;                                        // 0x488
	CObjBankView<CObjElem4ac> m_Sub4ac;                                        // 0x4ac
	CObjBankView<CObjElem4d0> m_Sub4d0;                                        // 0x4d0
	CObjBankView<CObjElem4f4> m_Sub4f4;                                        // 0x4f4
	CObjBankView<CObjElem518> m_Bank518;                                       // 0x518
	CObjBankView<CObjElem53c> m_Bank53c;                                       // 0x53c
	CObjBankView<CObjElem560> m_Bank560;                                       // 0x560
};

// The members with only a constructor.
#define DECLARE_OPAQUE_CTOR(cls, size)                       \
	class cls                                                \
	{                                                        \
	public:                                                  \
		cls();                                               \
		uint8 m_pad_00[size];                                \
	}

DECLARE_OPAQUE_CTOR(CSub92c, 0x9f0 - 0x92c);                   // ctor 0x004040e0  (SkyDef)
// The two lists at 0x8dc and 0x8ec.  They ARE ltlink.h's LTList — the same
// three stores — but the shipped constructor CALLS 0x00410e00 for each
// instead of expanding it, exactly as clientmgr2.cpp records CClientMgr's
// own constructor doing for the first of its pair and not the second.  A
// class with a declared constructor is how that is said here; if the
// measurement shows VC6 expanding them, the fix is `LTList` in their place.
DECLARE_OPAQUE_CTOR(CListCall, 0x10);                          // ctor 0x00410e00

// Constructor and destructor both out of line.
#define DECLARE_OPAQUE_BOTH(cls, size)                       \
	class cls                                                \
	{                                                        \
	public:                                                  \
		cls();                                               \
		~cls();                                              \
		uint8 m_pad_00[size];                                \
	}

DECLARE_OPAQUE_BOTH(CClassMgrView, 0x8dc - 0x80c);             // 0x00402810 / 0x00402830
DECLARE_OPAQUE_BOTH(CSubA14, 0xa2c - 0xa14);                   // 0x00490f60 / 0x00490fe0
DECLARE_OPAQUE_BOTH(CThreadView, 0xb2c - 0xac4);               // 0x0047bec0 / 0x004445a0

// 0x8fc: constructed out of line, torn down inline like a collection.
class CSub8fc
{
public:
	CSub8fc();                                                 // 0x00414b20
	~CSub8fc() { Term(); }                                     // 0x00415050

	void  Term() { SetSize2(0, &g_DefAlloc); }
	int   SetSize2(uint32 nNewSize, LAlloc* pAlloc);
	virtual void _g00();
	uint8 m_pad_04[0x30 - 0x04];
};

// 0xa2c: servermgr.h's CFileMgr.  Its constructor is out of line and its
// destructor is the compiler's — the only thing it runs is the bank the
// constructor sb_Init's at its own +0x18.
class CFileMgrView
{
public:
	CFileMgrView();                                            // 0x00413980

	uint8    m_pad_00[0x18];
	CBankA44 m_Bank18;                                         // 0x18
	uint8    m_pad_3c[0x98 - 0x3c];
};

// 0x668: servermgr.h's GlobalForceControl.  The constructor is written
// in-class because the shipped sequence is a CALL on its first member
// followed by a store the caller makes directly — a member initialiser and
// a one-line body, not an out-of-line constructor.
class CGravityView
{
public:
	// GlobalForceControl::Set (src/engine/lithtech/predobj.cpp section F):
	// an ordinary member function, NOT a constructor.
	void Set(CVec3* pVec);                                     // 0x00410e20

	uint8 m_pad_00[0x30];
};

class CForceControlView
{
public:
	// src/engine/server/commonphysics.cpp's GlobalForceControl ctor: a NAMED
	// local passed by address.  That is why 0x004135fc is followed by a
	// fresh `lea 0x10(%esp),%eax` instead of reusing the ctor's returned
	// `this` (a temporary bound to a reference reuses eax), and why the
	// `lea 0x668(%esi),%edi` is scheduled BEFORE the vector is built.
	CForceControlView()
	{
		CVec3 vForce(0.0f, -2000.0f, 0.0f);

		m_Gravity.Set(&vForce);

		// 0xbf350481 is EXACTLY -0.7071f and is not 1/sqrt(2): the nearest
		// float to -0.70710678 is 0xbf3504f4, 115 ulps away.  (clientmgr2.cpp
		// reads the same constant as -0.70710678f in its note on 0x00410ab2;
		// the literal is -0.7071f.)
		m_Unknown30 = -0.7071f;
	}

	CGravityView m_Gravity;                                    // 0x00
	float        m_Unknown30;                                  // 0x30
	uint8        m_pad_34[0x78 - 0x34];
};

// The two bases.  Neither has a destructor: the shipped `??_G` restores
// their vptrs and makes no call on the way out, so any destructor declared
// on them would be one call too many.
// CBaseMgrView is defined above CClientShell (which derives from it).

class CServerSoundMgrView
{
public:
	// Every slot of 0x004a1d68 is _purecall (0x004928e7): abstract.
	virtual void _s00() = 0;                                   // vptr 0x004a1d68
};

class CServerMgrView : public CBaseMgrView, public CServerSoundMgrView
{
public:
	CServerMgrView()
	{
		m_ClientServerType = 1;
		m_Unknown7fc = 0;
		m_Unknown800 = 0;
	}

	virtual ~CServerMgrView()
	{
		Term();
	}

	// Slot 0 of the CServerSoundMgr table 0x004a1d40: smgr.cpp's
	// CServerMgr::PlaySound (0x004759f0).  Declared so the view's second
	// table has the override the shipped one has; the base's own table
	// (0x004a1d68) is all _purecall.
	virtual void _s00();                                       // 0x004759f0

	void   Term();                                             // 0x00473550
	LTBOOL Init();                                             // 0x00473160
	// The four StartupLocal reaches, and the one Update does.  None is
	// named by anything at its call site; what the bytes say is in the
	// comment on each call.
	void   SetGameInfo(void* pGameInfo, uint32 nLen);          // 0x00473b00
	LTBOOL AddResources(void* pResources, uint32 nResources);  // 0x00473a50
	LTBOOL LoadBinaries();                                     // 0x00473ae0
	LTBOOL StartSession(char* pDriver, char* pSession);        // 0x004738a0
	void   SetupGlobals();                                     // 0x00474560
	LTRESULT SetupError(int nError, ...);                      // 0x00476370
	void   GetErrorString(char* pBuf, int nMaxLen);            // 0x00474580
	LTBOOL Sub4738f0(class CDriverView* pDriver);              // 0x004738f0
	class CDriverView* Sub475970();                            // 0x00475970
	LTBOOL Update(LTBOOL bPaused, uint32 nUnknown);            // 0x004742a0

	uint32            m_ClientServerType;                      // 0x008
	uint8             m_pad_00c[0x44 - 0x0c];
	LTList            m_List44;                                // 0x044
	LTList            m_List54;                                // 0x054
	uint8             m_pad_064[0x1a4 - 0x64];
	CWorld1a4         m_World;                                 // 0x1a4
	CSub390           m_Sub390;                                // 0x390
	uint8             m_pad_584[0x668 - 0x584];
	CForceControlView m_ForceControl;                          // 0x668
	CNetMgrView       m_NetMgr;                                // 0x6e0
	LTList            m_ClientList;                            // 0x7e4
	uint32            m_Unknown7f4;                            // 0x7f4
	uint32            m_Unknown7f8;                            // 0x7f8
	uint32            m_Unknown7fc;                            // 0x7fc
	uint32            m_Unknown800;                            // 0x800
	uint8             m_pad_804[0x80c - 0x804];
	CClassMgrView     m_ClassMgr;                              // 0x80c
	CListCall         m_List8dc;                               // 0x8dc
	CListCall         m_List8ec;                               // 0x8ec
	CSub8fc           m_Sub8fc;                                // 0x8fc
	CSub92c           m_SkyDef;                                // 0x92c
	CMessageBankView  m_MessageBank;                           // 0x9f0
	CSubA14           m_SubA14;                                // 0xa14
	CFileMgrView      m_FileMgr;                               // 0xa2c
	CThreadView       m_Thread;                                // 0xac4
};


// ======================================================================= //
//  4.  CClientShell::CClientShell — 0x00413210, 134 bytes, 1 call
//
//  WHAT PINS IT
//    * `movl $0x004a1d18,(%esi)` and no other vptr store: one vtable, so
//      one base chain.  Then `mov %al,0x4(%esi,%edi,1)` with
//      `cmp $0x10,%edi; jb` -- SIXTEEN BYTES at +0x04, which is where
//      clientmgr2.cpp read a second vptr.
//    * the ramp itself: `fildll` of a qword whose high dword is the zero
//      register is the UNSIGNED int-to-float conversion, so the counter is
//      uint32; `fmuls 0x004a1a5c` is *255.0f and `fmuls 0x004a1d14` is
//      *0.06666667f, which is 1.0f/15.0f correctly rounded -- VC6 folds the
//      division, but it will not fold the two multiplies together, so the
//      source really does multiply twice.  `call 0x004928c0` is __ftol and
//      only %al is stored, so the cast is to a byte.
//    * the two 0xffff stores are 16-bit (`mov %ax,...`), everything else is
//      32-bit off a zero register held in %ebx.
//    * the two `lea`+`mov %eax,0x4(%eax)`+`mov %eax,(%eax)` pairs are
//      ltlink.h's `CheapLTLink::TieOff` exactly -- `m_pPrev = m_pNext =
//      this` assigns m_pNext (+0x04) first -- on bare LTLinks at 0x44 and
//      0x50.  Neither has an element count zeroed next to it, which is what
//      says LTLink and not LTList; the +0x40 zero belongs to m_pServerMgr,
//      which StartupLocal reads as a pointer.
//    * the STORE ORDER is the source's and is transcribed verbatim below.
//      The vptr store and the two `lea`s float within it because VC6
//      schedules them; the zero stores do not move.
//
//  CONFIDENCE: MEDIUM-HIGH.  The only judgement is the shape of the ramp
//  expression, and both multiplies are in the bytes.
// ======================================================================= //

CClientShell::CClientShell()
{
	uint32 i;

	m_Unknown38 = 0xffff;
	m_Unknown6c = 0xffff;

	m_Unknown70 = 0;
	m_bWorldLoaded = LTFALSE;
	m_Unknown1c = 0.0f;
	m_Unknown14 = 0.0f;
	m_Unknown18 = 0.0f;
	m_Unknown74 = 0;
	m_nConnID = 0;
	m_pConnection = LTNULL;
	m_Unknown80 = 0;
	m_pServerMgr = LTNULL;

	m_Link44.TieOff();
	m_Link50.TieOff();

	m_Unknown88 = 0;

	// Named intermediate: written as one expression (with `* (1.0f/15.0f)`
	// or `/ 15.0f`) VC6 folds 255/15 into one `fmuls` of 17.0f.
	for (i = 0; i < 16; i++)
	{
		float f = i * 255.0f;
		m_Ramp[i] = (uint8)(f / 15.0f);
	}
}


// 0x004132c0 -- the destructor the `??_G` at 0x004132a0 calls: Term, then
// the base's inlined empty destructor restores CBaseMgr's table
// (0x004a1d2c).  The constructor above shows only the derived vptr store
// because /O2 drops the base's dead one; nothing between the two stores
// here lets it drop the second.
CClientShell::~CClientShell()
{
	Term();
}


// ======================================================================= //
//  5.  CClientShell::Term — 0x00413350, 219 bytes, 11 calls
//
//  WHAT PINS IT
//    * the first call is `push %esi; push 0x78(%esi); call 0x004115c0` --
//      clientmgr2.cpp's cm_OnShellTerm(pClientMgr, pShell), __cdecl, in
//      that order.  It is the function whose own comment says it is safe to
//      call "from the shell's own destructor path (0x0041335a)", which is
//      this instruction.
//    * `cmp %edi,0x53c(%ecx); jbe` is an UNSIGNED `> 0` on a field inside
//      the manager's CNetMgr (0x53c - 0x50c = +0x30), not a `!= 0`.
//    * `mov (%ecx),%edx; push $0x1; call *(%edx)` on m_pServerMgr is
//      `delete m_pServerMgr` through a virtual destructor -- slot 0 with
//      the deleting flag -- and it is followed by the null store, so the
//      source nulls the pointer itself.
//    * the loop is src/engine/lithtech/clientconsole.cpp's dhash walk, with
//      one difference that matters: there the walk is `if (!hIter) return;
//      do { } while (hIter);` because a return follows, and here work
//      follows, so it is a plain `while`.  VC6 rotates it either way.
//    * `add $0x1120,%ecx; mov 0x18(%ecx),%edx; mov %edx,(%eax);
//      mov %eax,0x18(%ecx)` is servermgr.h's INLINE sb_Free against a
//      StructBank at CClientMgr+0x1120: the freed block's first dword takes
//      the old head.  The separate `add` rather than one displacement is
//      what says the bank's address was taken.
//
//  CONFIDENCE: MEDIUM-HIGH.
// ======================================================================= //

void CClientShell::Term()
{
	void* hIter;
	void* hElement;

	cm_OnShellTerm(m_pClientMgr, this);

	if (m_pClientMgr->m_NetMgr.m_Unknown30 > 0)
		Sub47bbb0();

	m_pClientMgr->m_NetMgr.Disconnect(m_nConnID, 0, 0);

	m_nConnID = 0;
	m_pClientMgr->m_NetMgr.m_pShell = LTNULL;

	if (m_pServerMgr)
	{
		delete m_pServerMgr;
		m_pServerMgr = LTNULL;
	}

	ResetRecords();
	UnloadWorld();
	Sub4143f0();

	m_pConnection = LTNULL;
	m_Unknown6c = 0xffff;
	m_Unknown88 = 0;

	hIter = dhash_GetFirst(m_pUnknown8c);

	while (hIter)
	{
		hElement = dhash_GetNext(&hIter);

		if (hElement)
			sb_Free(&m_pClientMgr->m_Bank1120, dhash_GetData(hElement));
	}

	dhash_Term(m_pUnknown8c);

	g_pClientShell = LTNULL;
}


// ======================================================================= //
//  6.  CClientShell::StartupLocal — 0x00413440, 332 bytes, 14 calls
//
//  WHAT NAMES IT: 0x004134ef pushes "CClientShell::StartupLocal".
//
//  WHAT PINS THE SIGNATURE.  `ret $0xc`, so three dwords, and the call site
//  at 0x0040ff66 pushes them: `push %ebx` (= `mov (%edi),%ebx`, the first
//  dword of the first argument), `push %ecx` (= `test %ebx,%ebx; sete %cl`)
//  and `push %edi`.  So argument one is the object whose first dword the
//  caller read, argument two is `arg1->field0 == 0`, argument three is
//  `arg1->field0` itself.
//
//  Argument one IS ltbasedefs.h's StartGameRequest, and the offsets prove
//  it rather than the call site: the body reads `0x11e8(%eax)` and
//  `0x11ec(%eax)` as a pointer/length pair.  Laying StartGameRequest out --
//  m_Type 0x000, four MAX_SGR_STRINGLEN strings to 0x194, NetSession* at
//  0x194, NetHost (4+4+4+4096+64 = 0x104c) at 0x198, m_flags at 0x11e4 --
//  puts m_pGameInfo at 0x11e8 and m_GameInfoLen at 0x11ec exactly.  Which
//  also names argument three: `m_Type == STARTGAME_HOST` is argument two
//  and m_Type is argument three.  Argument two is never read here.
//
//  WHAT PINS THE BODY
//    * the three server-manager checks are THREE `if (!x) { error }`
//      statements with the error block written out each time: the first two
//      `je` into the LAST copy and the third `jne` OVER it, which is VC6
//      cross-jumping identical tails (as r_InitRender's three arms do).  A
//      single `||` puts the error block after the success return instead.
//    * `test %eax,%eax; jbe` on m_GameInfoLen is `> 0` on an unsigned, not
//      `!= 0`.
//    * the failure return is the RETURN_ERROR family with the SYM form --
//      the pushed code string is "LT_SERVERERROR", not "49" -- and with a
//      description, which is the 0x100-byte buffer the manager just filled.
//    * `mov 0x30(%esi),%ecx` RELOADS the connection between the store and
//      the virtual call, and the vptr is loaded only after Sub475970 returns:
//      the driver is named in a local first.  Nested as the argument, VC6
//      loads the vptr into %edi before the call.
// ======================================================================= //

LTRESULT CClientShell::StartupLocal(StartGameRequest* pRequest, LTBOOL /*bHost*/,
                                    uint32 nGameType)
{
	char     errorString[256];
	LTRESULT dResult;
	CDriverView* pDriver;

	dResult = CreateServerMgr();

	if (dResult != LT_OK)
		return dResult;

	if (pRequest->m_pGameInfo && pRequest->m_GameInfoLen > 0)
		m_pServerMgr->SetGameInfo(pRequest->m_pGameInfo, pRequest->m_GameInfoLen);

	if (!m_pServerMgr->AddResources(m_pClientMgr->m_Resources1538,
	                                m_pClientMgr->m_nResources1588))
	{
		m_pServerMgr->GetErrorString(errorString, 256);

		RETURN_ERROR_SYM_DESC(1, "CClientShell::StartupLocal", LT_SERVERERROR,
		                      errorString)
	}

	if (!m_pServerMgr->LoadBinaries())
	{
		m_pServerMgr->GetErrorString(errorString, 256);

		RETURN_ERROR_SYM_DESC(1, "CClientShell::StartupLocal", LT_SERVERERROR,
		                      errorString)
	}

	if (!m_pServerMgr->StartSession("local", "Metaltek Session"))
	{
		m_pServerMgr->GetErrorString(errorString, 256);

		RETURN_ERROR_SYM_DESC(1, "CClientShell::StartupLocal", LT_SERVERERROR,
		                      errorString)
	}

	m_bLocal = LTTRUE;

	if (nGameType)
		m_pServerMgr->Sub4738f0((CDriverView*)nGameType);

	con_PrintF(g_ConsoleColour, 1, "Connecting locally");

	m_pConnection = m_pClientMgr->m_NetMgr.Connect("local");
	pDriver = m_pServerMgr->Sub475970();
	m_pConnection->OnConnect(pDriver);

	m_pClientMgr->m_NetMgr.Update("Client: ", m_pClientMgr->m_Unknown125c, 1);

	return LT_OK;
}


// ======================================================================= //
//  7.  CClientShell::CreateServerMgr — 0x00413590, 427 bytes, 20 calls
//
//  WHAT NAMES IT: 0x00413721 pushes "CClientShell::CreateServerMgr".
//
//  Nineteen of its twenty calls are the CServerMgr constructor expanded in
//  place, which is the whole of section 3 above; the shape around them is
//  four lines:
//
//    * `push $0xb2c; call 0x004290b0; add $0x4,%esp` then `test; je` around
//      the constructor and the `xor %ecx,%ecx` on the other arm is VC6's
//      `new T` verbatim -- the null arm materialises the pointer in the
//      same register the constructor arm leaves it in, and only then is it
//      stored.  Written as `if ((p = dalloc(...)) != NULL) new(p) T;` it
//      comes out differently.
//    * the two failures BOTH fall into a block sitting after the success
//      return, which is the "nested success path" -- so it is one
//      `if (p) { if (Init()) { ...; return LT_OK; } }` with the error as
//      the function's last statement, not two guarded early returns.
//    * the error block is the plain RETURN_ERROR: the code string pushed is
//      "50" (at 0x004a8564), not "LT_CANTCREATESERVER", so
//      it is the LT_ERRSTR form, and the description is the lone space at
//      0x004a7164 -- LT_ERRDESC.  50 IS LT_CANTCREATESERVER.
//
//  IDENTICAL.  The last residual was the gravity vector: 0x00410e20 is
//  GlobalForceControl::Set(LTVector*), not a constructor, called from the
//  force control's constructor BODY on a named local (as
//  src/engine/server/commonphysics.cpp writes it) -- see CForceControlView.
// ======================================================================= //

LTRESULT CClientShell::CreateServerMgr()
{
	m_pServerMgr = new CServerMgrView;

	if (m_pServerMgr)
	{
		if (m_pServerMgr->Init())
		{
			if (m_pClientMgr)
				m_pServerMgr->m_NetMgr.Sub459ac0(m_pClientMgr->m_NetMgr.m_Unknownf0);

			return LT_OK;
		}
	}

	delete m_pServerMgr;
	m_pServerMgr = LTNULL;

	RETURN_ERROR(1, "CClientShell::CreateServerMgr", LT_CANTCREATESERVER)
}


// ======================================================================= //
//  8.  CServerMgr's scalar deleting destructor — 0x00413740, 562 bytes,
//      29 calls
//
//  Not written: it is `??_GCServerMgr`, and section 3's class emits it the
//  moment either `delete m_pServerMgr` above is compiled.  What says it is
//  the DELETING stub and not the destructor proper:
//
//    * `mov 0xc(%esp),%al; test $0x1,%al; je; push %esi; call 0x004290c0`
//      at the tail -- the flag argument and the operator delete that only
//      ??_G emits -- with `mov %esi,%eax` and `ret $0x4` for the signature;
//    * the destructor BODY is inlined into it: the two vptr stores on entry
//      (0x004a1d54 into (this), 0x004a1d40 into this+0x04), one call to
//      Term (0x00473550) and then twenty-seven member teardowns in strict
//      REVERSE of the order CreateServerMgr constructs them in, ending with
//      the base vptr restore (0x004a1d2c).
//    * only ONE flag is tested, so this is ??_G and not the ??_E form the
//      two functions at the head of this file are.
//
//  Its 0x004a1d54 slot 0 is this address,
//  which is the last thing tying the object to its table.
// ======================================================================= //


// The per-frame node pass Update runs, 0x00413b40; defined below, declared
// here because Update reaches it.
void cs_UpdateAttachPositions(CClientShell* pShell);                // 0x00413b40


// ======================================================================= //
//  9.  CClientShell::Update — 0x004139a0, 410 bytes, 18 calls
//
//  WHAT NAMES IT: 0x00413a75 pushes "CClientShell::Update".
//
//  `ret` with no immediate and `mov %ecx,%esi` at the head: __thiscall,
//  no arguments.  Three distinct returns, all LTRESULTs.
//
//  WHAT PINS THE BODY
//    * `mov 0x61c(%eax),%edi` is HOISTED above the CNetMgr update, so the
//      game shell is read into a local before that call and not after it --
//      VC6 would not move the load across a call otherwise.  All three
//      virtuals it later reaches (0x30, 0x34, 0x38) go through that local.
//    * `mov 0x4a9efc,%eax; test; je` gates the entire server step on a
//      .data flag, ANDed with m_pServerMgr being present: both jump to the
//      same continuation, which is the `&&`.
//    * the pause decision: `call dsi_IsClientActive; test; jne` SKIPS the
//      console lookup, so the source tests `!active`.  Then
//      `test %eax,%eax; je` on the variable and `cmpl $0x3f800000,0x18(%eax)
//      ; je` on its value -- a shared `bPaused = 1` block that both misses
//      fall into, i.e. one `||`.
//    * `test %eax,%eax; jne <continue>` after the server update makes the
//      ERROR the fall-through, so the source guards the failure
//      (`if (!Update(...))`) rather than the success.
//    * the failure runs the manager's __cdecl varargs SetupError with the
//      string the manager just filled and then RETURN_ERROR_SYM_DESC with
//      (LT_SERVERERROR|ERROR_DISCONNECT) -- 0x02000031, and the shipped
//      code string is the whole expression, "LT_SERVERERROR|ERROR_DISCONNECT".
//    * `or $0x2000000,%eax` on the way out of the second failure is
//      `return dResult | ERROR_DISCONNECT;` on the value just returned.
//    * `flds 0x18; fsubs 0x14; mov 0x18 -> 0x14; fstps 0x1c` is two
//      statements in the order written: the difference first, the carry
//      forward second, with the store scheduled late.
//    * `cmp 0x150c(%eax),%esi` and `cmp 0x61c(%eax),%esi` are CMP r32,r/m32
//      -- the register is the left operand -- so `this ==` comes first in
//      both, and the two share one body, which is the `||`.  The miss
//      `jne` goes straight to `return LT_OK`: PostUpdate and the
//      Sub47bcd0 step are INSIDE that block.
//    * `push $0x4b2f00` is the console state's ADDRESS as an immediate:
//      con_FindVar takes &g_ClientConsoleState, not a loaded pointer.
//
//  ONE THING NOT SETTLED.  `cmpl $0x3f800000,0x18(%eax)` is an INTEGER
//  compare against the bit pattern of 1.0f, where the field is the console
//  variable's float value (servermgr.h's LTCommandVar::m_FloatVal at
//  +0x18).  Comparing a float for equality against a non-zero, non-NaN
//  constant is exactly representable as a dword compare and VC6 is entitled
//  to do it; the source below says what the source said.  If the build
//  comes back with `fld`/`fcomp` there, the single-edit flip is
//
//      if (!pVar || *(uint32*)&pVar->m_FloatVal != 0x3f800000)
//
//  CONFIDENCE: MEDIUM -- everything but that one compare is off the bytes.
// ======================================================================= //

LTRESULT CClientShell::Update()
{
	char               errorString[256];
	IClientShellView*  pClientShell;
	LTBOOL             bPaused;
	LTCommandVarView*  pVar;
	LTRESULT           dResult;
	uint32             hObject;

	pClientShell = m_pClientMgr->m_pClientShell;

	m_pClientMgr->m_NetMgr.Update("Client: ", m_pClientMgr->m_Unknown125c, 1);

	if (m_pServerMgr && g_nUnknown4a9efc)
	{
		bPaused = LTFALSE;

		if (!dsi_IsClientActive())
		{
			pVar = con_FindVar(&g_ClientConsoleState, "alwaysfocused");

			if (!pVar || pVar->m_FloatVal != 1.0f)
				bPaused = LTTRUE;
		}

		if (!m_pServerMgr->Update(bPaused, m_pClientMgr->m_Unknown125c))
		{
			m_pServerMgr->GetErrorString(errorString, 256);
			m_pClientMgr->SetupError(LT_SERVERERROR, errorString);

			RETURN_ERROR_SYM_DESC(1, "CClientShell::Update",
			                      LT_SERVERERROR|ERROR_DISCONNECT, errorString)
		}
	}

	m_Unknown1c = 0.0f;

	dResult = Sub47bab0();

	if (dResult)
		return dResult | ERROR_DISCONNECT;

	m_Unknown1c = m_Unknown18 - m_Unknown14;
	m_Unknown14 = m_Unknown18;

	if (dsi_IsClientActive())
		sub_461270(this);

	pClientShell->PreUpdate();
	pClientShell->Update();

	if (this == g_pClientMgr->m_pClientShell2 ||
	    this == (CClientShell*)g_pClientMgr->m_pClientShell)
	{
		if (dsi_IsClientActive())
		{
			m_pClientMgr->Sub415bb0();
			cs_UpdateAttachPositions(this);
		}

		pClientShell->PostUpdate();

		hObject = Sub47bcd0();

		if (hObject && m_Unknown88 != hObject)
			m_Unknown88 = hObject;
	}

	return LT_OK;
}


// ======================================================================= //
//  10.  cs_UpdateAttachPositions — 0x00413b40, 283 bytes, 2 calls
//
//  __cdecl with one argument (`mov 0x4(%esp),%eax` and a bare `ret`), and
//  its only caller is Update, which pushes `this`.
//
//  WHAT PINS IT
//    * the 0x30 bytes of stack are ONE object, not six locals: the nine
//      dwords copied in run 0x0c(%esp) upward in three groups of three, and
//      the three pointers follow them at 0x30/0x34/0x38 -- contiguous
//      ascending slots, which is the fingerprint of a
//      structure and not of separate named locals.  Its address is what the
//      callback is handed.
//    * the callback is NOT a virtual: `mov 0xc(%esi),%eax; ...
//      call *0x4(%eax)` with `add $0x8,%esp` after it -- nothing is passed
//      as `this` and the caller cleans up, so +0x0c points at a table of
//      __cdecl entries and the call is its second.
//    * `cmp $0x1,%eax; jne` -- the result is tested against 1, not against
//      zero.
//    * the copy BACK reloads `0x4(%esi)` before each of the three vectors
//      while the copy OUT does not: the call in between is what makes VC6
//      rematerialise it, so one local pointer serves both in the source.
//    * the node walk reads a uint16 at +0x40, terminates on 0xffff, and
//      indexes `m_pOwner->m_pNodes[]` -- with m_pOwner re-read from
//      `(%esi)` every iteration, so the source writes it out each time.
//
//  CONFIDENCE: MEDIUM.
// ======================================================================= //

void cs_UpdateAttachPositions(CClientShell* pShell)
{
	CClientMgrView* pClientMgr;
	CRecF4*         pCur;
	CPosRec*        pRec;
	CNodeOwner*     pOwner;
	CPosRec*        pNode;
	CPosRequest     request;
	uint16          nNode;

	pClientMgr = pShell->m_pClientMgr;
	pCur       = pClientMgr->m_pChainF4;

	while (pCur)
	{
		pRec   = pCur->m_pRec;
		pOwner = pCur->m_pOwner;

		request.m_Vec00      = pRec->m_Vec04;
		request.m_Vec0c      = pRec->m_Vec10;
		request.m_Vec18      = pRec->m_Vec1c;
		request.m_pClientMgr = pClientMgr;
		request.m_pOwner     = pOwner;
		request.m_pRec       = pRec;

		if (pCur->m_pCallbacks->m_pGetPos(&request, pCur->m_Unknown08) == 1)
		{
			pCur->m_pRec->m_Vec04 = request.m_Vec00;
			pCur->m_pRec->m_Vec10 = request.m_Vec0c;
			pCur->m_pRec->m_Vec1c = request.m_Vec18;

			nNode = pCur->m_pRec->m_nNextNode;

			while (nNode != 0xffff)
			{
				pNode = pCur->m_pOwner->m_pNodes[nNode];

				sub_421fc0(pNode);

				nNode = pNode->m_nNextNode;
			}
		}

		pCur = pCur->m_pNext;
	}
}


// ======================================================================= //
//  11.  CClientShell::SendMessage0A — 0x00413c60, 97 bytes, 6 calls
//
//  `ret $0x4` and `mov %ecx,%edi`: __thiscall, one dword, and the value it
//  leaves in %eax is whatever the last Release returned, so it returns
//  NOTHING.
//
//  WHAT PINS IT
//    * `push $0x1000; push $0x1000; push %eax(=&local); call 0x0045c080;
//      add $0xc,%esp` -- three dwords and the caller cleans up, but the
//      LAST pushed is the address of a four-byte local that the function
//      then dereferences (`mov (%eax),%esi`).  That is MSVC's hidden
//      return-buffer argument, so 0x0045c080 returns a small class BY VALUE
//      and the class is not trivially copyable.
//    * what follows is that class's copy and destruction, not anything the
//      source spells: `if (p) p->AddRef()` on the copy (slot 1) and
//      `if (p) p->Release()` on the temporary (slot 2), off the SAME
//      pointer, reloaded from the temporary's slot rather than reused.
//      One counted handle initialised from a returned temporary.
//    * `mov 0x54(%esi),%ecx; movb $0xa,(%ecx)` is the message TYPE stamped
//      into element zero of the payload -- cltnew.cpp pinned +0x54 as
//      "m_pData ... SendToServer stamps the message type into element zero
//      of it" -- so the type is 10.
//    * the send is clientmgr2.cpp's CClientShell::SendToServer
//      (0x00413cd0), reached `mov %edi,%ecx`, and the final
//      `if (p) p->Release()` is the handle going out of scope.
//
//  CONFIDENCE: MEDIUM-HIGH.  The only judgement is the holder's shape, and
//  the AddRef/Release pair either side of the assignment is exactly it.
// ======================================================================= //

class CMsgHold
{
public:
	CMsgHold(ILTMessageView* pMsg)
	{
		m_pMsg = pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();
	}

	~CMsgHold()
	{
		if (m_pMsg)
			m_pMsg->Release();
	}

	ILTMessageView* m_pMsg;                                     // 0x00
};

void CClientShell::SendMessage0A(uint32 nData)
{
	CMsgHold cMsg(msg_StartWrite(0x1000, 0x1000).m_pMsg);

	cMsg.m_pMsg->m_pData[0] = 10;
	cMsg.m_pMsg->WriteDWord(nData);

	SendToServer(cMsg.m_pMsg);
}



// ======================================================================= //
//  5.  FIVE LITHTECH-ONLY BODIES THIS FILE NAMES BUT WHOSE HOMES ARE ELSEWHERE
//
//  Each is written here because this file already declares its class (or
//  its call site) as a view; the comment on each gives the TU the shipped
//  address order places it in.
// ======================================================================= //

// The varargs console printer 0x00477f60 reaches (clientconsole.cpp's
// con_PrintColored: __cdecl, colour first, caller cleans 0xc).
void con_PrintColored(uint32 nColour, int nLevel, char* pFormat, ...); // 0x0041b8a0
// clientdsi.cpp's (0x0042e6f0): __cdecl varargs.
void dsi_ServerError(char* pFormat, ...);                      // 0x0042e6f0

// What a CNetMgr driver is as far as FindLocalDriver reaches it:
// servermgr.h's CBaseDriver, whose name follows the vptr.
class CDriverView
{
public:
	virtual void _p00();
	char         m_Name[0x44 - 0x04];                           // +0x04
	uint32       m_DriverFlags;                                 // +0x44
	CNetMgrView* m_pNetMgr;                                     // +0x48
};

// 0x004738a0 -- 71 bytes.  HOME: src/engine/server/smgr.cpp (client build
// only; it sits between sm_ErrorHook 0x00473880 and AddResources
// 0x00473a50, and server.dll has no twin -- the string "Listening on
// driver: %s" exists only in lithtech.exe).  The second argument is never
// read (`ret $0x8`).  LT_ERRORINITTINGNETDRIVER is 56 (`push $0x38`).
LTBOOL CServerMgrView::StartSession(char* pDriver, char* pSession)
{
	SetupGlobals();

	if (!m_NetMgr.Connect(pDriver))
	{
		SetupError(LT_ERRORINITTINGNETDRIVER, pDriver);
		return LTFALSE;
	}

	dsi_ServerError("Listening on driver: %s", pDriver);
	return LTTRUE;
}

// 0x00475970 -- 113 bytes.  HOME: src/engine/server/smgr.cpp (client
// build), between FreeUnusedModels 0x00475560 and PlaySound 0x004759f0.
// The net manager's driver named "local" (the inline strcmp against
// 0x004a8528), or null.
CDriverView* CServerMgrView::Sub475970()
{
	uint32 i;

	for (i = 0; i < m_NetMgr.m_Drivers.m_nElements; i++)
	{
		if (strcmp(m_NetMgr.m_Drivers.m_pArray[i]->m_Name, "local") == 0)
			return m_NetMgr.m_Drivers.m_pArray[i];
	}

	return LTNULL;
}

// 0x004738f0 -- 339 bytes.  HOME: src/engine/server/smgr.cpp (client
// build), after StartSession 0x004738a0.  StartupLocal hands it the client
// net manager's driver: the driver leaves the list of the manager that owns
// it (the find loop is the one CNetMgr::RemoveDriver writes out; the removal
// is Remove -> Remove2 0x0042b630) and joins this manager's, which is
// CMoArray::Insert with Insert2 expanded, and becomes its main driver.
//
// NEAR, 347 of 339 bytes, 4 blocks (was 554, 27). The whole shape is the
// /O2 inline budget: shipped CALLS Remove2 (`push g_DefAlloc; push idx;
// call 0x0042b630`) and expands Insert2. Written `Append(pDriver)` or
// `Insert(m_nElements, ...)`, ours expanded Remove2 as well; the insert at
// `pList->GetSize()` is what makes VC6 decline Remove2 (measured: every
// removal spelling tried -- ForgetDriver, FindElement in place, an open
// loop, explicit Remove2 -- moves with the insert spelling, not its own),
// and the list named once is what lets the `index > m_nElements` guard
// fold away as it does under Append. The residual: ours also expands
// BaseDelete inside _DeleteAndDestroyArray (`call *0x8` on g_DefAlloc)
// where shipped calls 0x004152b0 -- the budget is one step too generous.
//
// stdlith dynarray.h's FindElement, which collection.h declares and does
// not define.
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

LTBOOL CServerMgrView::Sub4738f0(CDriverView* pDriver)
{
	pDriver->m_pNetMgr->ForgetDriver(pDriver);

	CMoArray<CDriverView*>* pList = &m_NetMgr.m_Drivers;
	pList->Insert(pList->GetSize(), pDriver);
	m_NetMgr.m_pMainDriver = pDriver;
	pDriver->m_pNetMgr = &m_NetMgr;

	return LTTRUE;
}

// 0x00477f40 -- 30 bytes.  CClientShell's vtable (0x004a1d18) slot 1,
// CBaseMgr::_v04 -- the net manager's new-connection notification.
// thiscall, `ret $0x8`: two arguments, the second (is-local) unread.  Only
// one connection is taken: with +0x34 already set the answer is FALSE;
// otherwise the connection is kept, +0x5c set, and the answer is TRUE (the
// one `mov $1,%eax` serves both the store and the return).  The row used to
// be cut at 0x00477f4c, the `je` target after the interior `ret $0x8`.
LTBOOL CClientShell::_v04(uint32 nConnID, LTBOOL bLocal)
{
	if (m_nConnID)
		return LTFALSE;

	m_nConnID   = nConnID;
	m_bOnServer = LTTRUE;
	return LTTRUE;
}

// 0x00477f60 -- 56 bytes.  CClientShell's vtable (0x004a1d18) slot 2,
// CBaseMgr::_v08 -- the net manager's dead-connection notification.  HOME:
// the TU at 0x00477f40..0x00477fd3 that holds all four of CClientShell's
// CBaseMgr overrides (slots 1-4).
void CClientShell::_v08(void* pConn)
{
	IClientShellView* pShell;

	con_PrintColored(0x6464fa, 1, "Disconnected from server");
	m_nConnID = 0;

	pShell = m_pClientMgr->m_pClientShell;
	if (pShell)
		pShell->OnEvent(1, 0);
}

// 0x00477fa0 -- 3 bytes, `ret $0xc`.  CClientShell's vtable (0x004a1d18)
// slot 3, CBaseMgr::ProcessPacket: the client shell takes no raw packets
// through its base, so the override is empty.
void CClientShell::ProcessPacket(CPacket* pPacket, uint8* pSenderAddr,
                                 uint16 nSenderPort)
{
}

// 0x0047bbb0 -- 106 bytes.  Called once, from CClientShell::Term.  HOME:
// the TU at 0x0047bab0.. (Sub47bab0 / Sub47bcd0 are its neighbours).
// Message 6 to the server, only while connected.
// The holder is built from the message pointer the temporary carries, not
// copied from the temporary: the shipped bytes AddRef the local before the
// temporary's Release, which an elided copy (SendMessage0A's shape) lacks.
// 0x0047bab0 -- 242 bytes.  Called once per frame from Update: drains the
// net manager's incoming packets through the server-to-client dispatch
// table (cchandler.cpp: 0x004b48d0, indexed by `m_pData[0] & 0x3f`, each
// entry __cdecl (pShell, pPacket) returning an LTRESULT).  A packet with no
// handler goes to the file manager (ltnew2.cpp's sub_004041d0).  The first
// failing handler's result is returned.  The server's twin is smgr.cpp's
// sm_ProcessPackets.  HOME: the TU at 0x0047bab0 (this is its first
// function; Sub47bbb0 follows).
// IDENTICAL.  The packet id goes through a DWORD stack slot in the
// shipped bytes (`and $0x3f,%dl; mov %dl,0x14(%esp); mov 0x14(%esp),%eax;
// and $0xff,%eax` at 0x0047bb10): the uint8 local is ADDRESS-TAKEN, so VC6
// keeps it in memory.  Stored through a pointer to it, it matches; a plain
// store, `*&nPacketID =`, an inline getter, an out-pointer inline and
// uint8[1] all enregister it.  `volatile` gives a byte reload.
typedef LTRESULT (*ClientPacketHandlerFn)(CClientShell* pShell, ILTMessageView* pMsg);
// The table is cmodelload.cpp's (its cm_InitTable4b48d0Q474, 0x00477fe0,
// zeroes and fills it); one object, so its spelling is used here and the
// entry cast on the way out.
extern void* g_Table4b48d0Q474[0x100];                         // 0x004b48d0
void sub_004041d0(void* pFileMgr, ILTMessageView* pMsg);       // 0x004041d0

LTRESULT CClientShell::Sub47bab0()
{
	CMsgHold              cMsg(msg_StartWrite(0x1000, 0x1000).m_pMsg);
	LTRESULT              dResult;
	uint8                 nPacketID;
	ClientPacketHandlerFn pHandler;

	m_pClientMgr->m_NetMgr.StartGettingPackets();

	while (m_pClientMgr->m_NetMgr.GetPacket(cMsg.m_pMsg))
	{
		{
			uint8* pID = &nPacketID;
			*pID = cMsg.m_pMsg->m_pData[0] & 0x3f;
		}
		pHandler  = (ClientPacketHandlerFn)g_Table4b48d0Q474[nPacketID];

		if (pHandler)
		{
			dResult = pHandler(this, cMsg.m_pMsg);

			if (dResult)
			{
				m_pClientMgr->m_NetMgr.EndGettingPackets();
				return dResult;
			}
		}
		else
		{
			sub_004041d0(m_pClientMgr->m_pFileMgr, cMsg.m_pMsg);
		}
	}

	m_pClientMgr->m_NetMgr.EndGettingPackets();
	return LT_OK;
}

void CClientShell::Sub47bbb0()
{
	CMsgHold cMsg(msg_StartWrite(0x1000, 0x1000).m_pMsg);

	if (m_nConnID)
	{
		cMsg.m_pMsg->m_pData[0] = 6;
		m_pClientMgr->m_NetMgr.SendToConnection(cMsg.m_pMsg, m_nConnID, 0x80);
	}
}

// 0x0047bcd0 -- 27 bytes.  The client's own object: the id at +0x6c
// (16 bits, 0xffff for none) looked up in the manager's object table
// (0x00420520, clientmgr2.cpp's tbl_GetObject).  thiscall, no arguments;
// Update (above) and clientinterface.cpp's ci_GetClientObject call it.
// HOME: the TU at 0x0047bab0, after Sub47bbb0 and 0x0047bc20 /
// CPacket::ReadType (0x0047bc50).
void* tbl_GetObjectQ47b(CClientMgrView* pClientMgr, uint16 nID);  // 0x00420520

uint32 CClientShell::Sub47bcd0()
{
	if (m_Unknown6c == 0xffff)
		return 0;

	return (uint32)tbl_GetObjectQ47b(m_pClientMgr, m_Unknown6c);
}

// 0x00421fc0 -- 222 bytes.  HOME: the world-geometry TU between
// worldmodels.obj's wd_FindWorldModel (0x00421cc0) and commonmodel.obj's
// w_FindPortalInWorld (0x004220a0).  Called from cs_UpdateAttachPositions
// and from the world loader (0x00423b63) once per polygon: each vertex's
// texture coordinates from the surface's O/P/Q vectors.  Every `_CVector`
// operand is taken by value, which is the three 12-byte stack copies.
class CUVSurface
{
public:
	uint8    m_pad_00[0x04];
	LTVector m_O;                                               // 0x04
	LTVector m_P;                                               // 0x10
	LTVector m_Q;                                               // 0x1c
};

class CUVVertex
{
public:
	LTVector* m_pVec;                                           // 0x00
	float     m_U;                                              // 0x04
	float     m_V;                                              // 0x08
	uint8     m_pad_0c[0x18 - 0x0c];
};

class CUVPoly
{
public:
	uint32 GetNumVertices()
	{
		if (m_pVertices != m_Vertices)
			return m_nVertices + m_nExtraVertices;
		return m_nVertices;
	}

	uint8       m_pad_00[0x28];
	CUVSurface* m_pSurface;                                     // 0x28
	uint8       m_pad_2c[0x4c - 0x2c];
	CUVVertex*  m_pVertices;                                    // 0x4c
	uint16      m_nVertices;                                    // 0x50
	uint16      m_nExtraVertices;                               // 0x52
	CUVVertex   m_Vertices[1];                                  // 0x54
};

void sub_421fc0(void* pNode)
{
	CUVPoly*   pPoly = (CUVPoly*)pNode;
	CUVVertex* pCur;
	CUVVertex* pEnd;
	LTVector   vDiff;

	pCur = pPoly->m_Vertices;
	pEnd = pCur + pPoly->GetNumVertices();
	for (; pCur != pEnd; pCur++)
	{
		vDiff = *pCur->m_pVec - pPoly->m_pSurface->m_O;
		pCur->m_U = vDiff.Dot(pPoly->m_pSurface->m_P);
		pCur->m_V = vDiff.Dot(pPoly->m_pSurface->m_Q);
	}
}

// ======================================================================= //
//
//  PER-ADDRESS STATUS
//
//    0x00413080  136 B   3 calls  ??_ECPooled54            MEDIUM-HIGH
//    0x00413110  128 B   3 calls  ??_ECPooled50            MEDIUM-HIGH
//    0x00413210  134 B   1 call   CClientShell::CClientShell   MEDIUM-HIGH
//    0x00413350  219 B  11 calls  CClientShell::Term       MEDIUM-HIGH
//    0x00413440  332 B  14 calls  CClientShell::StartupLocal   MEDIUM
//    0x00413590  427 B  20 calls  CClientShell::CreateServerMgr  LOW (bytes)
//    0x00413740  562 B  29 calls  ??_GCServerMgr           LOW (bytes)
//    0x004139a0  410 B  18 calls  CClientShell::Update     MEDIUM
//    0x00413b40  283 B   2 calls  cs_UpdateAttachPositions MEDIUM
//    0x00413c60   97 B   6 calls  CClientShell::SendMessage0A  MEDIUM-HIGH
//
//  Every one of the ten issues the shipped call sequence exactly, by
//  construction: the counts and the direct/indirect pattern above were
//  taken off the image and each body was
//  written against them.  The two LOW rows are LOW for BYTES only -- what
//  they cannot reproduce is the vptr store each collection member of
//  CServerMgr makes on the way out (section 3 says why), not the calls.
//
//  NOT IN THIS FILE, though they sit inside the same address range: the
//  functions at 0x00413190, 0x004131d0, 0x004131f0, 0x00413200, 0x004132a0,
//  0x004132c0, 0x004132e0, 0x004132f0, 0x00413310, 0x00413430 and
//  0x00413980.  0x004132e0 / 0x00413310 are already in
//  the decoded symbol map, 0x004132a0 / 0x004132f0 / 0x004131f0
//  are described in src/engine/lithtech/clientmgr2.cpp, and the rest are
//  outside the ten written here.
//
// ======================================================================= //


// 0x004151f0 -- servermgr.h's CMessageBank constructor: the message bank's
// StructBank of 0x54-byte records, 0x20 to a block.  The vptr store sits
// between the argument pushes (0x004a1dac, the table whose Init is
// 0x00415210 and whose deleting destructor is 0x00415270).
CMessageBankView::CMessageBankView()
{
	sb_Init(&m_Bank, 0x54, 0x20);
}
