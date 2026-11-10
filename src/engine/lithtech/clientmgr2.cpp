// *********************************************************************** //
//
//  MODULE:   CLIENTSTARTUP (.CPP)   (file name ours)
//
//  PURPOSE:  lithtech.exe, 0x0040e9d0 .. 0x00418650 — the small functions of
//            the client's startup run: CClientMgr's console/list/teardown
//            helpers, CClientShell (the engine's own client shell, not the
//            game's IClientShell), and the two shared no-op vtable bodies
//            that run fills.
//
//  WHAT NAMES THIS RUN
//  -------------------
//  Three shipped strings anchor everything below:
//
//    * 0x0040fb47 pushes "CClientMgr::Init" — so 0x0040f8d0 is
//      CClientMgr::Init, and the `mov %ecx,%ebx` at its head makes every
//      0x1xxx displacement in the neighbourhood a CClientMgr field.
//    * 0x00414141 pushes "CClientShell::DoLoadWorld" — so the class whose
//      `this` carries a CClientMgr* at +0x78 is CClientShell.  clientmgr.h
//      already models the same object as CClientConnection (it is what
//      CClientMgr+0x150c points at); the shipped image gives it its real
//      name.
//    * 0x00415800 returns "CLIENT-OBJECT", and the only construction site
//      for its vtable (0x00410d70: `movl $0x4a1b00,(%eax)`) stores the
//      8-byte object into CClientMgr+0x508 — the field clientmgr.h calls
//      m_pUnknown508 and notes CServerMgr keeps the same thing at 0x6a8.
//      So the 133-slot table at 0x004a1b00 is the *client* side of a
//      base the shared engine code calls through, and "CLIENT-OBJECT" is
//      the name it answers with.
//
//  VIEWS
//  -----
//  src/engine/lithtech/clientmgr.h already declares CClientMgr,
//  CClientConnection, CSurface, ConsoleState and g_pClientMgr, but it pads
//  over most of the fields this run reaches, and that header is shared with
//  work in flight.  So each class below is a local layout-identical VIEW in
//  the house style: every offset it declares is read off
//  a shipped displacement, the offsets it shares with clientmgr.h agree with
//  it, and when the new fields are folded into that header the views go away
//  and these become plain CClientMgr / CClientConnection methods.
//
//  Build /O2 /Gy.  Four bodies below are folded across several vtable slots
//  by identical-COMDAT folding, which is why /Gy matters; each says how many
//  slots it lands in.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x419b50).
#include <string>
#endif
#include <string.h>
#include <stdio.h>             // sprintf -- cs_ModelKeyCallback

#include "ltbasedefs.h"
#include "ltlink.h"
#include "iclientshell.h"
#include "de_error.h"          // RETURN_ERROR / RETURN_ERROR_SYM_DESC (server/)
#include "packet.h"            // CPacket::WriteType                   (server/)
#include "wonptrq503.h"        // the WON AuthContext pointers below

#ifdef AVP2_MODERN
#include "..\..\modern\engine\p1_frametime.h"   // M-P1-23
#endif


// The game shell's WON AuthContext (clientmgr3.cpp's Init stores it and the
// peer authentication reads it), and a second pointer of the same kind
// right after it that nothing reads or writes: the only references to it
// are its own initialiser set's. Both initialiser sets open this unit
// (Primal Hunt 0x004108b0..0x00410920, AvP2 1.0.9.6 0x0040fa60..0x0040faf0,
// before the command bank's at 0x0040fb20). In Primal Hunt's /GX build the
// smart pointer's constructor, with the base's destructor to unwind, stays
// out of line (0x00410900 jumps to 0x00414090); AvP2's expands it.
CWonSmartPtrQ503<CWonAuthContextQ548> g_WonPtrQ503;        // PH 0x004f0118, AvP2 0x004def68
CWonSmartPtrQ503<CWonAuthContextQ548> g_WonPtrUnusedQ503;  // PH 0x004f011c, AvP2 0x004def6c


// ----------------------------------------------------------------------- //
//  Views
// ----------------------------------------------------------------------- //

// CClientMgr, as far as this run reaches it.  Every offset is a shipped
// displacement; the names of the fields clientmgr.h already has are its
// names, the rest are ours.
// The world tree at CClientMgr+0x6c.  0x0048cde0 is __thiscall with two
// arguments (the site loads %ecx and does no cleanup): worldtree2.cpp's
// AddObject, as cchandler.cpp's CWorldTreeQ478 already declares it.
class CWorldTreeQ2C
{
public:
	void AddObject(void* pObj, uint32 nListIndex);                   // 0x0048cde0
};

// The client's CNetMgr sub-object at CClientMgr+0x50c.  0x00456360 is its
// send entry: __thiscall, three dword arguments (cchandler.cpp's
// CNetMgrQ478::SendTo), and SendToServer tail-returns its eax, so it answers
// an LTRESULT.  +0xb4 is the back pointer CClientShell::Init sets.
class CNetMgrQ2C
{
public:
	LTRESULT SendTo(void* pMsg, uint32 nConnID, uint32 nFlags);     // 0x00456360

	uint8            m_pad_00[0xb4];
	class CClientShell* m_pClientShell;           // 0x0b4
	uint8            m_pad_b8[0x104 - 0xb8];
};

class CClientMgrView
{
public:
	uint8            m_pad_00[0x6c];
	CWorldTreeQ2C    m_World;                     // 0x00006c  (clientmgr.h)
	uint8            m_pad_6d[0x1c8 - 0x6d];
	// The renderer's per-world object.  Made by the render-DLL entry at
	// 0x004b43d4 and given back through 0x004b43d8; null while there is no
	// render DLL, which is the whole of what CreateRenderWorld tests.
	void*            m_pRenderWorld;              // 0x0001c8
	uint8            m_pad_1cc[0x1f0 - 0x1cc];
	// Passed by address to 0x0045bde0 (identical, the shared object
	// manager's unlink-and-free) as sub_411bf0's last act.
	uint8            m_ObjectMgr1ec[0x410 - 0x1f0]; // 0x0001f0
	// Eleven sixteen-byte records.  Nothing here reads inside one — the
	// bound is the shipped `cmp $0xb0,%esi` against a `add $0x10` stride,
	// so the count and the width are proven and the contents are not.
	uint8            m_Records40c[0xb0];          // 0x000410
	// The message object helper clientmgr3.cpp news up for this field
	// (0x00415560 is its constructor); 0x00411440 hands it to a packet.
	class CClientMsgObjectHelper* m_pObjectHelper; // 0x0004c0
	// Reached only as `m_pUnknown4c0->m_Unknown0c` by one vtable slot.
	class CUnknown4C0* m_pUnknown4c0;             // 0x0004c4
	uint8            m_pad_4c8[0x4dc - 0x4c8];
	// Copied out whole by the 0x508 object's slot 13 (PH 0x00419b13).
	LTVector         m_vUnknown4dc;               // 0x0004dc
	uint8            m_pad_4e8[0x508 - 0x4e8];
	// CClientMgr::Init zeroes this and slot 1 of the 0x508 object hands out
	// its ADDRESS; slot 2 (0x00415760) dereferences it and writes +0x18 and
	// +0x10 through it, so it is a POINTER to ltbasedefs.h's CollisionInfo
	// -- the client twin of servermgr.h's CServerMgr::m_pCollisionInfo
	// (0x6ac), whose address the server table's slot 1 returns the same way.
	CollisionInfo*   m_pCollisionInfo;            // 0x000508
	// clientmgr.h: the client's CNetMgr as a sub-object.  Only its send
	// entry (0x00456360) and the back pointer at +0xb4 are used here.
	// 0x000508 -- the client's half of the base whose 133-slot table is at
	// 0x004a1b00; constructed at 0x00410d70 and stored here.  Its absence is
	// what put every field below this point four bytes low, and restoring it
	// took this TU from 13 matched to 19.
	class CClientCSBase* m_pCSBase;               // 0x00050c
	CNetMgrQ2C       m_NetMgr;                    // 0x000510
	uint8            m_pad_614[0x620 - 0x614];
	IClientShell*    m_pClientShell;              // 0x000620  (clientmgr.h)
	uint8            m_pad_624[0x1094 - 0x624];
	// The music DLL's interface struct and the DLL name, both filled by
	// InitMusic (0x00410380) and handed to music_Init (wmrotate.cpp).
	uint8            m_MusicStruct[0x1104 - 0x1094]; // 0x001094
	char             m_szMusicDLL[0x100];         // 0x001104
	// The loaded models: a bare LTLink head, each Model on it by the link
	// at its own +0x08 (FreeUnusedModels, 0x004114a0).
	LTLink           m_Models;                    // 0x001204
	// Two LTLists, side by side.  Both are ltlink.h's LTList exactly: the
	// constructor at 0x00410ab0 calls 0x00410e00 on the first and has the
	// same three stores inlined for the second, and 0x00410e00 IS
	// LTList::LTList() (see the note at the foot of this file).
	LTList           m_List1100;                  // 0x001210
	LTList           m_List1110;                  // 0x001220
	uint8            m_pad_1230[0x1270 - 0x1230];
	// Fifteen dwords set to -1 both by CClientMgr::Term (0x00410f42) and by
	// the connection teardown below, with the same `rep stos` count.
	int32            m_Handles1160[15];           // 0x001270
	uint8            m_pad_12ac[0x12e8 - 0x12ac];
	uint32           m_Unknown11d8;               // 0x0012e8  zeroed by the ctor
	// The server-id -> object table sub_411a70 fills: eight-byte records,
	// a used byte at +0 and the object at +4 (0x00411b10 / 0x00411b27).
	class CObjMapEntryQ5* m_pObjMap11dc;          // 0x0012ec
	uint8            m_pad_12f0[0x1368 - 0x12f0];
	// The frame's elapsed time in seconds; csvis.cpp's walks turn it into
	// milliseconds with `* 1000.0f` (0x004a1afc) and __ftol.
	float            m_fFrameTime;                // 0x001368
	uint8            m_pad_136c[0x1374 - 0x136c];
	// A plain chain, not an LTList: the walk below follows +0x04 to null.
	class CChain1264* m_pChain1264;               // 0x001374
	uint8            m_Sub1268[0x1628 - 0x1378];  // 0x001378  passed by address
	// Set to 1 at the end of CClientMgr::Init and the only gate on the
	// console file being run.
	LTBOOL           m_bConsoleReady;             // 0x001628
	uint8            m_pad_162c[0x22bc - 0x162c];
	// A 16-bit counter that skips 0xffff: see NextFrameCode (0x00411450).
	uint16           m_nFrameCode;                // 0x0022bc

	// 0x0040fd50 / 0x0040fd70
	void   OnEnterWorld(class CClientShell* pShell);
	void   OnExitWorld(class CClientShell* pShell);
	// 0x00410350
	void   Disconnect();
	// 0x00410380 (clientmgr.h)
	LTRESULT InitMusic(char* szMusicDLL);
	// 0x0041fc60 -- formats into +0x620; varargs, so __cdecl.
	LTRESULT SetupError(LTRESULT dResult, ...);
	// 0x00410510 / 0x00410530
	LTBOOL CreateRenderWorld();
	void   DestroyRenderWorld();
	// 0x00410590
	void   FreeList1110Contents();
	// 0x00410540
	void   BindAllTextures();
	// 0x004113e0
	class CClientMsgQ411* StartMessage();
	// 0x004114a0
	LTRESULT FreeUnusedModels();
	// 0x00410620
	void   SetGlobals();
	// 0x00410640
	void   HandleThreadMessage(class CThreadMessageView* pMsg);
	// 0x00411440
	void   SetPacketHelper(class CPacketHelperView* pPacket);
	// 0x00411450
	uint16 NextFrameCode();
	// 0x00410610
	void   Sub410610();
	// 0x00415bb0
	void   RunFrameUpdates();
};

// The two LTList members and the fields past them make the view above
// awkward to keep in one piece, so the fields CClientMgr carries at and past
// 0x150c come off a second view laid over the same object.  Both agree with
// clientmgr.h where they overlap.
// The demo manager at CClientMgr+0x158c -- demodtx.cpp's CDemoMgrQ42C /
// startload.cpp's CDemoMgrQ2B, with only the entry this file reaches.  Stop()
// is a thiscall: the shipped call site at 0x00410353 loads `this + 0x158c`
// into %ecx and pushes nothing.
class CDemoMgrQ2C
{
public:
	void     Stop();                                                     // 0x0042c2a0

	uint8 m_pad_00[0x1720 - 0x169c];
};

class CClientMgrTail
{
public:
	uint8            m_pad_00[0x161c];
	class CClientShell* m_pClientShell2;          // 0x00161c  (clientmgr.h:
	                                              //           m_pConnection)
	uint8            m_pad_1620[0x1644 - 0x1620];
	class CFileMgrView* m_pFileMgr;               // 0x001644  (clientmgr.h)
	uint8            m_pad_1648[0x169c - 0x1648];
	CDemoMgrQ2C      m_Sub158c;                   // 0x00169c  constructed by
	                                              //           0x0042bfa0
	uint32           m_Unknown1610;               // 0x001720
};


// The object at CClientMgr+0x150c.  Two vptrs, at +0x00 (0x004a1d18, five
// slots) and +0x04 (0x004a1d40, forty-eight) — multiple inheritance, so the
// vptrs are spelled out as fields rather than modelled with `virtual`, the
// way clientmgr.h spells out CClientInput's table.  Nothing here calls one
// of its own virtuals, so nothing needs the C++ dispatch.
class CClientShell
{
public:
	void**           m_pVtable00;                 // 0x000  &0x004a1d18
	void**           m_pVtable04;                 // 0x004  &0x004a1d40
	uint8            m_pad_08[0x30 - 0x08];
	uint32           m_Unknown30;                 // 0x030  set, never read here
	// The third argument of every send through the manager's CNetMgr, and
	// the only thing 0x00413cd0 varies.
	uint32           m_nConnID;                   // 0x034
	uint8            m_pad_38[0x6c - 0x38];
	// Forced to 0xffff immediately before the teardown at 0x00414900.
	uint16           m_Unknown6c;                 // 0x06c
	uint8            m_pad_6e[0x78 - 0x6e];
	CClientMgrView*  m_pClientMgr;                // 0x078
	// DoLoadWorld's last act is to set this to 1, and UnloadWorld's first
	// is to test it and clear it, so it is the "a world is up" flag.
	LTBOOL           m_bWorldLoaded;              // 0x07c
	uint8            m_pad_80[0x8c - 0x80];
	void*            m_pUnknown8c;                // 0x08c

	// 0x00413310
	LTBOOL Init(CClientMgrView* pClientMgr);
	// 0x00413430
	LTRESULT SetUnknown30(uint32 nValue);
	// 0x00413cd0
	LTRESULT SendToServer(void* pMsg);
	// 0x00413cf0
	void ResetRecords();
	// 0x004143d0
	void UnloadWorld();
	// 0x00414860 / 0x004148a0
	LTBOOL CreateRenderWorld();
	void   DestroyRenderWorld();
	// 0x00414b10
	CClientMgrView* GetClientMgr();
	// 0x004143f0
	void   sub_4143f0();
	// 0x00477fb0 -- slot 4 of 0x004a1d18
	void   SetDisconnectCode(uint32 nCode, const char* pMsg);
};


// The 8-byte object at CClientMgr+0x508, vtable 0x004a1b00, 133 slots.  Only
// the vptr and the back pointer exist — 0x00410d70 allocates 8 bytes and
// stores exactly those two.  Slot numbers below are (displacement / 4) off
// 0x004a1b00.
class CClientCSBase
{
public:
	// Slots this file supplies.  Declared in slot order so the order is
	// visible; the bodies are further down.
	virtual void        _p00(void* p1, void* p2);                     // 0
	virtual CollisionInfo** GetCollisionInfo();                       // 1
	virtual void        TouchNotify(HLOCALOBJ hObj, HOBJECT hToucher,
	                                LTVector* pStopVel, float fForceMag); // 2
	virtual void        _p0c(void* p1, void* p2);                     // 3
	virtual void        _p10()=0;                                     // 4 -> 0x0048faa0
	// AVP2: slot 5 is the client's attachment walker (0x00419740); NOLF's
	// was the shared empty body 0x0048faa0.
	virtual void        UpdateAttachments(struct WMMoveState* pState); // 5
	virtual LTBOOL      IsMatch(void* pRec, uint32 nKey, void* pTest); // 6
	virtual void        _p1c(void* p1, void* p2);                     // 7
	virtual void        _p20(void* p1, void* p2);                     // 8
	virtual void        _p24()=0;                                     // 9 -> 0x0048fab0
	virtual void        _p28()=0;                                     // 10 -> 0x0048fa90
	virtual const char* GetSideName(void* pUnused);                   // 11
	virtual uint32      GetUnknown4c0_0c();                           // 12
	virtual LTRESULT    GetUnknown4dc(void* p1, LTVector* pVec);      // 13

	CClientMgrView*     m_pClientMgr;                                 // 0x04
};

// What slot 6's third argument is called through: one virtual at 0x30
// (slot 12), asked for a yes/no.  A view of its own, because the shipped
// code only ever reaches that one slot (names ours).
class ISlot6TestView
{
public:
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0; virtual void _p1c()=0; virtual void _p20()=0;
	virtual void _p24()=0; virtual void _p28()=0; virtual void _p2c()=0;
	virtual LTBOOL Test()=0;                                          // 0x30

	// AVP2: compared against the record's +0x0c (PH 0x00419ab9, AvP2
	// 0x004179b9: `movzbl 0xbd(%ecx)`).
	uint8  m_pad_04[0xbd - 0x04];
	uint8  m_Unknown0bd;                                              // 0xbd
};

// The record slot 6 is handed.  Three fields, all read, none written.
class CSlot6Record
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_nKey;                                // 0x08
	// AVP2: the gate, which must exceed the tested object's byte at +0xbd
	// (NOLF's was a flag at +0x10).
	uint32 m_Unknown0c;                           // 0x0c
};


// A surface, as far as the render-DLL teardown needs it.  The offsets past
// 0x0c are clientmgr.h's CSurface; what this file adds is the +0x04 link,
// proven by the wrap-terminated walk in surface_SaveAll, and the +0x08
// system-memory copy 0x0040f150 fills in.
class CSurfaceView
{
public:
	uint8           m_pad_00[0x04];
	CSurfaceView*   m_pNext;                      // 0x04
	void*           m_pSysMem;                    // 0x08
	void*           m_pRenderData;                // 0x0c  (clientmgr.h)
	uint32          m_nWidth;                     // 0x10  (clientmgr.h)
	uint32          m_nHeight;                    // 0x14  (clientmgr.h)
	uint32          m_nPitch;                     // 0x18  (clientmgr.h)
};

// clientmgr.h's m_nServerID: the id the server knows the object by, 0xffff
// for a client-only object (cltnew.cpp tests it the same way).
class CClientObjIDView
{
public:
	uint8   m_pad_00[0xb8];
	uint16  m_nServerID;                          // 0x0b8
	uint8   m_pad_ba[0xbc - 0xba];
	// ltbasedefs.h's OT_ type; movsbl at 0x0045bdea, so signed.  Read only
	// by sub_411bf0 below (OT_MODEL == 1).
	char    m_ObjectType;                         // 0x0bc
	uint8   m_pad_bd[0x188 - 0xbd];
	// Bit 0 gates IClientShell::OnObjectRemove in sub_411bf0 (0x00411c15).
	// clientmgr.h's m_ClientFlags.
	uint32  m_ClientFlags180;                     // 0x188
};

// The elements of CClientMgr's two LTLists, as far as the walks below go.
// The engine's LTList keeps the payload in the link's m_pData, so the only
// thing these walks know about an element is the pointer they free or call.
class CListItemView
{
public:
	virtual void _p00()=0;
	virtual void _p04()=0;
	virtual void Release()=0;                     // 0x08 — slot 2
};


// ----------------------------------------------------------------------- //
//  Globals this run touches
// ----------------------------------------------------------------------- //

extern CClientMgrView*  g_pClientMgr;             // 0x004b17e0  (clientmgr.h)

// The head of the client's surface list — a *circular* singly-linked list
// through CSurfaceView::m_pNext, which is why the walk stops when the next
// pointer comes back round to the head and not when it reaches null.  What
// makes it the surface list rather than any other list: 0x0040f150 reads
// +0x10 and +0x14 off each element as the width and height it multiplies
// into an allocation size, and hands +0x0c to the render DLL — clientmgr.h's
// CSurface, field for field.  (name ours)
//
// It is not a global of its own: 0x004b17a0 is the head field (+0x08) of the
// surface TU's list object at 0x004b1798, which cissurf.cpp defines as
// g_SurfaceListQ40C (a vptr, the count at +0x04, the head at +0x08).  The
// name below is that field, reached through the one definition.
class CGLinkedListQ40C;
extern CGLinkedListQ40C g_SurfaceListQ40C;        // 0x004b1798  (cissurf.cpp)
struct CSurfaceListViewQ2C
{
	void*         m_pVtbl;                        // 0x004b1798
	uint32        m_nElements;                    // 0x004b179c
	CSurfaceView* m_pHead;                        // 0x004b17a0
};
#define g_pSurfaceList (((CSurfaceListViewQ2C*)&g_SurfaceListQ40C)->m_pHead)

// The engine's own client shell, published by its Init.  (name ours)
extern CClientShell*    g_pClientShell;           // 0x004b17ec  (clientshell.cpp)

// clientmgr.h: the flag the render DLL sets once it is up -- g_RenderStruct's
// +0x44, in the view below.

// Two more of the render-DLL function table clientmgr.h has the neighbours
// of (0x004b43dc onwards).  These two make and unmake the renderer's
// per-world object; the maker is handed the address of a one-field request
// struct, which is the only reason CClientShell::CreateRenderWorld has a
// local at all.  (names ours)
//
// Neither is a global of its own: 0x004b4358..0x004b44a7 is ONE object,
// clientrender.cpp's RenderStruct g_RenderStruct, and every render-DLL entry
// and render flag this file reaches is a field of it.  They are reached
// through that one definition, cast to the view below (the absolute
// address is the same `sym+disp` either way, so the code is unchanged).
struct RenderWorldRequest { CClientMgrView* m_pClientMgr; };
struct SceneDescQ5;
class RenderStruct;
extern RenderStruct g_RenderStruct;               // 0x004b4358  (clientrender.cpp)
struct RenderStructViewQ2C
{
	uint8   m_pad_000[0x44];
	int     m_bInited;                                       // 0x044  0x004b439c
	uint8   m_pad_048[0x7c - 0x48];
	void*   (*CreateWorld)(RenderWorldRequest* pRequest);    // 0x07c  0x004b43d4
	void    (*DestroyWorld)(void* pRenderWorld);             // 0x080  0x004b43d8
	uint8   m_pad_084[0xb0 - 0x84];
	LTBOOL  (*RenderScene)(SceneDescQ5* pScene);             // 0x0b0  0x004b4408
	uint8   m_pad_0b4[0xf8 - 0xb4];
	void    (*ReadConsoleVars)();                            // 0x0f8  0x004b4450
	uint8   m_pad_0fc[0x14c - 0xfc];
	uint32  m_dwDirtyFlags;                                  // 0x14c  0x004b44a4
};
#define g_RenderStructQ2C (*(RenderStructViewQ2C*)&g_RenderStruct)
#define g_pRenderCreateWorld          (g_RenderStructQ2C.CreateWorld)
#define g_pRenderDestroyWorld         (g_RenderStructQ2C.DestroyWorld)
#define g_pfnRenderSceneQ5            (g_RenderStructQ2C.RenderScene)
#define g_pRenderReadConsoleVarsQ411  (g_RenderStructQ2C.ReadConsoleVars)
#define g_dwDirtyFlags4b44a4          (g_RenderStructQ2C.m_dwDirtyFlags)
#define g_bRenderInited               (g_RenderStructQ2C.m_bInited)

// clientmgr.h: the client's own console state.  The dword one past the last
// field that header models is written by two functions here, both times with
// the CClientMgr the console is to work through — the client's counterpart of
// ConsoleState::m_pServerMgr at +0x38.  It is declared on its own rather
// than added to ConsoleState, because that class lives in the shared
// src/engine/server headers.  (name ours)
extern CClientMgrView*  g_pConsoleClientMgr;      // 0x004b2f44
// The object is src/engine/server's ConsoleState; this file only ever
// passes its address, so it is reached as the one definition and the
// address handed on under the view type the console calls here take.
class ConsoleStateRef;
class ConsoleState;
extern ConsoleState     g_ClientConsoleState;     // 0x004b2f00
#define g_ClientConsoleStateRef (*(ConsoleStateRef*)&g_ClientConsoleState)


// ----------------------------------------------------------------------- //
//  Helpers this run calls but does not define
// ----------------------------------------------------------------------- //

// The console entry points, in the shared console TU.  0x0041b420 takes the
// stage the command is allowed to run in plus a literal 1 that never varies;
// 0x0041b4c0 is the console's file runner (clientmgr.h names con_RunFile /
// con_SaveFile as the config-file pair, and 0x004116a0 hands it
// "autoexec.cfg").  (names ours where marked)
void con_ParseCommands(ConsoleStateRef* pState, char* pCommands, int nStage,
                       int nAlwaysOne);                             // 0x0041b420
void con_RunFile(ConsoleStateRef* pState, char* pFilename);         // 0x0041b4c0
void con_Sub41b820(int nWhich);                                     // 0x0041b820

// 0x00462900 is NOT dfree -- the shipped call below resolves here, while our `dfree` links to dalloc.cpp's real one somewhere else
// entirely.  This is the texture-data release: it hands the texture to the
// render DLL's own entry, then frees m_pEngineData unless it carries
// TDFLAG_TEXMOD.  See src/engine/lithtech/clienttexmgr.cpp, which decodes it.
void  tex_FreeTextureData(void* pTexture);                          // 0x00462900
void  sub_483c60(void* pItem);                                      // 0x00483c60
void  sub_429080(void* pItem);                                      // 0x00429080
// 0x00490200 is the image's one bare `ret`: /OPT:ICF folded every empty
// function onto it.  clienttexmgr.cpp defines it with one fixed argument,
// the manager, and both sites here are reached through that.  It has to be
// the fixed-argument spelling: with startload.cpp's varargs one VC6 cleans
// up after each call instead of sharing BindAllTextures' one `add $0xc`.
class CClientMgrTexBankView;
void  sub_490200(CClientMgrTexBankView* pClientMgr);             // 0x00490200
void  sub_404480(class CFileMgrView* pFileMgr);                     // 0x00404480
void  sub_41b200(void* pSub1268);                                   // 0x0041b200
void* sub_42cb20(int n1, int n2);                                   // 0x0042cb20
// The client message-dispatch table fill: zeroes 0x100 dwords at 0x004b48d0
// and stores nineteen handlers.  cdecl, no arguments -- reconstructed in
// src/engine/lithtech/cmodelload.cpp under this name.
void  cm_InitTable4b48d0Q474();                                     // 0x00477fe0
void  sub_411c80(CClientMgrView* pClientMgr, void* pRecord, int nFlag); // 0x00411c80
LTBOOL surface_CopyOut(CSurfaceView* pSurface);                     // 0x0040f150
void  sub_414240(CClientMgrView* pClientMgr, class CWtWorldView* pWorld); // 0x00414240


// ======================================================================= //
//  CClientMgr — the game DLL's two world notifications
// ======================================================================= //

// 0x0040fd50 / 0x0040fd70 — tell the game DLL a world came up / went down.
//
// What the bytes prove, and it is worth spelling out because it is what
// numbers IClientShell's whole vtable for this run:
//
//   * each is `ret $0x4`, so each takes one argument;
//   * neither pushes anything before the indirect call, so the virtual it
//     reaches takes NONE;
//   * the displacements are 0x20 and 0x24, which are slots 8 and 9.
//
// src/LT2/sdk/inc/iclientshell.h's declaration order puts ~IClientShell at 0,
// so slot 8 is OnEnterWorld() and slot 9 is OnExitWorld() — the only
// adjacent pair in the header that both take no arguments.  It agrees with
// the repo's existing anchor (clientinterface.cpp calls OnPlaySound
// "IClientShell's twenty-fourth slot", which is index 23 by the same count)
// and with the call sites: 0x0040fd50 is the last thing
// CClientShell::DoLoadWorld does before setting m_bWorldLoaded, and
// 0x0040fd70 is the first thing UnloadWorld does before clearing it.
//
// The argument is the shell that is entering or leaving.  It is dead — the
// game DLL's hooks take nothing — and it is still in the signature because
// the shipped `ret $0x4` says the parameter was written.
// 0x0040fd50
void CClientMgrView::OnEnterWorld(CClientShell* /*pShell*/)
{
	if (m_pClientShell)
		m_pClientShell->OnEnterWorld();
}

// 0x0040fd70
void CClientMgrView::OnExitWorld(CClientShell* /*pShell*/)
{
	if (m_pClientShell)
		m_pClientShell->OnExitWorld();
}


// ======================================================================= //
//  CClientMgr::StartGame (0x004114a0)
//
//  Primal Hunt's client manager unit has it here, between OnExitWorld
//  (0x00411480) and Disconnect (0x00411a30), which it calls first.  It was
//  startload.cpp's; startload.cpp's views of the manager, the shell, the
//  drivers and the message holder come with it, restated (the same class
//  names, so the calls name the same functions).
// ======================================================================= //

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
// CClientFileRef and hand back a stream, or LTNULL").  iltstream.h's
// ILTStream, with the slot DoLoadWorld calls: SeekTo, slot 5 (PH
// 0x00417e14 `call *0x14(%edx)` with one zero argument, after the virtual
// destructor, Release, Read, ReadString and ErrorStatus).
class ILTStreamQ2B
{
public:
	virtual void     _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void     _p0c()=0; virtual void _p10()=0;
	virtual LTRESULT SeekTo(uint32 nOffset)=0;                           // 0x14
};
// AVP2: iltcommon.h's ILTCommon, as far as GetCRC, slot 15 (0x3c): the
// virtual destructor, then SetObjectFilenames .. CreateMessage.  PH
// 0x00417cd0 `call *0x3c(%edx)` with (stream, &result).
class ILTCommonQ556
{
public:
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0; virtual void _p1c()=0; virtual void _p20()=0;
	virtual void _p24()=0; virtual void _p28()=0; virtual void _p2c()=0;
	virtual void _p30()=0; virtual void _p34()=0; virtual void _p38()=0;
	virtual LTRESULT GetCRC(ILTStreamQ2B* pStream, uint32& dwResult)=0; // 0x3c
};
// The client interface CClientMgr+0x4c4 points to: iltcsbase.h's
// ILTCSBase puts the embedded ILTMath (one vptr) at +0x04 and m_pCommonLT
// after it, at +0x08 (PH 0x00417ccb `mov 0x8(%ecx),%ecx`).
class ILTClientQ556
{
public:
	void*          m_pVtbl;                                              // 0x00
	void*          m_MathLT;                                             // 0x04
	ILTCommonQ556* m_pCommonLT;                                          // 0x08
};
// lmessage.cpp's view of the ServerMessage string read: the string in place,
// the cursor stepped past its terminator (lithtech.exe 0x0046f380, server.dll
// 0x10031041).  DoLoadWorld calls it on its CPacket (PH 0x0041796f, `mov
// %edi,%ecx`).  The same class name and signature as lmessage.cpp's, so the
// two declarations name one function.
class CServerMessageStrQ528
{
public:
	const char* ReadString();                // 0x10031041 / 0x0046f380
};
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
// ~IClientShell at slot 0.  (NOLF's header, which this view was first read
// against at NOLF 0x00413fd5, had PreLoadWorld at 0x1c and a three-argument
// SetDisconnectCode at 0x60.)
class IClientShellQ2B
{
public:
	// AVP2: the AvP2 SDK's iclientshell.h order, not NOLF's: GetAuthContext
	// at slot 1 and OnPeerToPeerAuthPacket at slot 6 push PreLoadWorld to
	// slot 9 (0x24, PH 0x00417c2c) and SetDisconnectCode to slot 26 (0x68,
	// PH 0x00417c7b), where it has two arguments and no default -- the
	// shipped sites push two dwords.
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0; virtual void _p1c()=0; virtual void _p20()=0;
	virtual void PreLoadWorld(char* pWorldName)=0;                       // 0x24
	virtual void OnEnterWorld()=0;                                       // 0x28
	virtual void OnExitWorld()=0;                                        // 0x2c
	virtual void _p30()=0; virtual void _p34()=0; virtual void _p38()=0;
	virtual void _p3c()=0; virtual void _p40()=0; virtual void _p44()=0;
	virtual void _p48()=0; virtual void _p4c()=0; virtual void _p50()=0;
	virtual void _p54()=0; virtual void _p58()=0; virtual void _p5c()=0;
	virtual void _p60()=0; virtual void _p64()=0;
	virtual void SetDisconnectCode(uint32 nCode, const char* pMsg)=0;   // 0x68
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

// AVP2: Primal Hunt's StartGame (0x004114a0) declares its holder EMPTY at
// the top -- `mov %edi,0x1c(%esp)` and EH state 0 before Disconnect
// (0x004114c9) -- and assigns the returned temporary to it at the send
// (0x004118c4..0x004118ef): the pointer copied and AddRef'd, nothing
// released first, then the temporary's Release under EH state 2.  The
// release at the end tests the copy kept in %edi (0x004119f8).  (name ours)
class CMsgSlotQ556
{
public:
	CMsgSlotQ556() { m_pMsg = LTNULL; }

	~CMsgSlotQ556()
	{
		if (m_pMsg)
			((IPacketRefQ2B*)m_pMsg)->Release();
	}

	void operator=(const CMsgRefQ2B& other)
	{
		m_pMsg = other.m_pMsg;

		if (m_pMsg)
			((IPacketRefQ2B*)m_pMsg)->AddRef();
	}

	// servermsg.h's CMessageRef::Init: drop the old reference, take one on
	// the other's.
	CPacket* Init(const CMsgRefQ2B& other)
	{
		if (m_pMsg)
			((IPacketRefQ2B*)m_pMsg)->Release();

		m_pMsg = other.m_pMsg;

		if (m_pMsg)
			((IPacketRefQ2B*)m_pMsg)->AddRef();

		return m_pMsg;
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
	// AVP2: the offsets below are AvP2's and Primal Hunt's, not NOLF's (the
	// NOLF displacements this view was first read at are kept in the
	// comments): four bytes longer from m_World's end to the client shell and
	// 0x110 longer from the music manager on, as clientmgr.h has it.  Every
	// one is read off Primal Hunt's DoLoadWorld (0x00417790) and StartGame
	// (0x004114a0); AvP2 1.0.9.6's (0x004157b0) reads the same ones.
	uint8            m_pad_0000[0x184];
	// 0x00414085 stores the world stream here and 0x0041408b reads it back
	// one instruction later, through the SAME base register -- so it is a
	// member and not a local.  It falls inside clientmgr.h's m_World
	// (0x06c..0x50c).  (name ours)  PH 0x00417e5d.
	ILTStreamQ2B*    m_pWorldStream184;                        // 0x000184
	uint8            m_pad_0188[0x410 - 0x188];
	// clientmgr2.cpp's m_Records40c (NOLF +0x40c): eleven sixteen-byte
	// records.  Both CClientShell::ResetRecords and DoLoadWorld's tail loop
	// (PH 0x00417f2e, `mov edi,0x410 .. cmp edi,0x4c0`) walk them with an
	// `add 0x10` stride and a 0xb0 span.  They are LTLists: 0x00418130
	// takes each one's address as an LTList*.
	uint8            m_Lists40c[0xb0];                         // 0x000410
	uint8            m_pad_04c0[0x4c4 - 0x4c0];
	// clientmgr.h's m_pClientDE (ccline.cpp / cchandler.cpp: +0x4c4 in both
	// retail exes).  DoLoadWorld reaches its CommonLT for the world CRC
	// (PH 0x00417cbf).
	ILTClientQ556*   m_pClientDE;                              // 0x0004c4
	uint8            m_pad_04c8[0x510 - 0x4c8];
	CNetMgrQ2B       m_NetMgr;                                 // 0x000510 (NOLF 0x50c)
	uint8            m_pad_0614[0x620 - 0x614];
	IClientShellQ2B* m_pClientShell;                           // 0x000620 (NOLF 0x61c)
	uint8            m_pad_0624[0x12e8 - 0x624];
	// clientmgr2.cpp: "zeroed by the ctor", read by cm_GetUnknown11d8.
	// DoLoadWorld copies it into the world load request (PH 0x00417e35).
	uint32           m_Unknown11d8;                            // 0x0012e8 (NOLF 0x11d8)
	uint8            m_pad_12ec[0x136c - 0x12ec];
	// A FLOAT.  lt2new3.cpp's note settles it from the other side --
	// CLTClient::GetTime is a `flds` of it -- and the decoded mangled name
	// for DoStartWorld (`...@@QAEKPADKM@Z`) makes StartGame's third
	// DoStartWorld argument a float too, which agrees.  PH 0x0041783d.
	float            m_fTime125c;                              // 0x00136c (NOLF 0x125c)
	uint8            m_pad_1370[0x161c - 0x1370];
	CClientShellQ2B* m_pClientShell2;                          // 0x00161c (NOLF 0x150c)
	uint8            m_pad_1620[0x1644 - 0x1620];
	CFileMgrQ2B*     m_pFileMgr;                               // 0x001644 (NOLF 0x1534)
	uint8            m_pad_1648[0x169c - 0x1648];
	CDemoMgrQ2B      m_DemoMgr;                                // 0x00169c (NOLF 0x158c)

	// ---- the two entry points this file defines -------------------------
	LTRESULT StartGame(StartGameRequest* pRequest);            // 0x0040fd90
	// PH 0x00410e40 -- __thiscall, no arguments; StartGame's last act for
	// every game type but the two hosts and NORMAL.  It starts from the
	// shell's GetAuthContext (slot 1 of m_pClientShell).  (name ours)
	void     Sub410e40Q556();                                  // PH 0x00410e40
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
	// PH 0x004133c0 -- __thiscall, (string, uint32*): the CRC of the misc
	// files the server names in the world-load packet, checked against the
	// server's own value (LT_DISCON_MISCCRC / LT_CRCMISCCHECKFAILED when they
	// differ).  The answer is never read.  (name ours)
	LTRESULT GetMiscCRCQ556(const char* pFiles, uint32& nCRC); // PH 0x004133c0
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

void cm_OnShellInitQ2B(CClientMgrQ2B* pClientMgr);             // 0x00411560
void sub_42e680(uint32 nValue);                                // 0x0042e680

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

// WHAT CLOSED StartGame IN NOLF'S BUILD (measured, NOLF lithtech.exe 0x0040fd90):
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
	// NEAR (Primal Hunt 0x004114a0: 1419 shipped, 1704 ours).  Everything
	// down to the message is shipped's block for block -- the holder
	// declared empty at the top with EH state 0, the early returns leaving
	// without its Release, the empty-name error, the start flags, the
	// trailing 0x00410e40 call.  What is left is the message's WriteType:
	// shipped CALLS the byte array's Insert2 (0x004118e9 -> 0x004162f0)
	// where ours expands it.  Measured: the NOLF-shaped body this replaced
	// (holder built in the block, no 0x00410e40 call) calls it, and adding
	// EITHER the top-level holder or the trailing call alone makes VC6
	// expand it; the empty-name test, the flags local, an inline
	// shell-disconnect helper at the four sites, an operator=(CPacket*)
	// holder and an explicit Insert2 instantiation ahead of the function
	// change nothing.
	//
	// AVP2: Primal Hunt 0x004114a0 (1419 bytes).  `sub esp,0x210`: E-0x210
	// the `this` spill that pDriver (and the message temporary) reuse,
	// E-0x208 dResult, E-0x204 cMsg, E-0x200 errorString, E-0x100
	// worldName.
	CMsgSlotQ556     cMsg;
	LTRESULT         dResult;
	char             errorString[256];
	char             worldName[256];
	CClientShellQ2B* pShell;
	CBaseDriverQ2B*  pDriver;
	char*            pWorldName;
#ifdef PRIMAL_HUNT
	uint32           nFlags;
#endif

	Disconnect();                                                   // 0x004114d4

	pShell = new CClientShellQ2B;                                   // 0x004114de
	pShell->m_nGameMode = pRequest->m_Type;                         // 0x00411514
	pShell->Init(this);                                             // 0x0041151a

	sub_42e680(0);                                                  // 0x00411521

	switch (pRequest->m_Type)                                       // 0x0041152e
	{
	case STARTGAME_CLIENT:                                          // 0x00411531
	{
		pDriver = m_NetMgr.m_pMainDriver;                           // 0x00411803

		if (!pDriver)                                               // 0x00411809
			RETURN_ERROR(1, "StartGame", LT_NOTINITIALIZED)

		if (pRequest->m_flags & SG_LOBBY)                           // 0x0041180d
		{
			if (!pDriver->JoinLobbySession())                       // 0x0041181a
				RETURN_ERROR(1, "StartGame", LT_ERROR)
		}
		else
		{
			if (!pRequest->m_pNetSession)                           // 0x0041182c
				RETURN_ERROR_SYM_DESC(1, "StartGame", LT_INVALIDPARAMS,
				                      "Missing m_pNetSession")

			dResult = pDriver->JoinSession(pRequest->m_pNetSession);  // 0x00411871

			if (dResult != LT_OK)                                   // 0x00411874
				return dResult;
		}

		dResult = pShell->SetUnknown30(pDriver);                    // 0x0041187f

		if (dResult != LT_OK)                                       // 0x00411884
			return dResult;
		break;
	}
	case STARTGAME_CLIENTTCP:                                       // 0x00411538
	{
		pDriver = m_NetMgr.FindDriver("internet");                  // 0x00411790

		if (!pDriver)                                               // 0x00411797
			RETURN_ERROR(1, "StartGame", LT_NOTINITIALIZED)

		dResult = pDriver->JoinTCPSession(pRequest->m_TCPAddress);  // 0x004117e3

		if (dResult != LT_OK)                                       // 0x004117e6
			return dResult;

		dResult = pShell->SetUnknown30(pDriver);                    // 0x004117f1

		if (dResult != LT_OK)                                       // 0x004117f6
			return dResult;
		break;
	}
	default:
	{
		pDriver = LTNULL;                                           // 0x00411540

		if (pRequest->m_Type == STARTGAME_HOST)                     // 0x0041153e
		{
			pDriver = m_NetMgr.m_pMainDriver;                       // 0x0041154e

			if (!pDriver)                                           // 0x00411554
				RETURN_ERROR(1, "StartGame", LT_NOTINITIALIZED)

			if (pRequest->m_flags & SG_LOBBY)                       // 0x004115a0
			{
				if (pDriver->HostLobbySession(&pRequest->m_HostInfo) != LT_OK)  // 0x004115ab
					RETURN_ERROR(1, "StartGame", LT_ERROR)
			}
			else
			{
				dResult = pDriver->HostSession(&pRequest->m_HostInfo);  // 0x004115ee

				if (dResult != LT_OK)                               // 0x004115f1
					return dResult;
			}
		}

		pWorldName = pRequest->m_WorldName;                         // 0x00411607

#ifdef PRIMAL_HUNT
		if (pRequest->m_RecordFilename[0])                          // 0x00411605
		{
			dResult = m_DemoMgr.StartRecording(pWorldName,
			                                   pRequest->m_RecordFilename);  // 0x00411614

			if (dResult != LT_OK)                                   // 0x00411619
			{
				if (pShell)                                         // 0x00411621
					pShell->Disconnect(1);                          // 0x0041162f

				return dResult;
			}

			pWorldName[0] = 0;                                      // 0x00411684
		}
		else if (pRequest->m_PlaybackFilename[0])                   // 0x00411646
		{
			dResult = m_DemoMgr.StartPlayback(pRequest->m_PlaybackFilename,
			                                  worldName, 0x100);    // 0x0041165e

			if (dResult != LT_OK)                                   // 0x00411663
			{
				if (pShell)                                         // 0x0041166b
					pShell->Disconnect(1);                          // 0x00411679

				return dResult;
			}

			pWorldName[0] = 0;                                      // 0x00411684
		}

		dResult = pShell->StartupLocal(pRequest,
		                               pRequest->m_Type == STARTGAME_HOST,
		                               (uint32)pDriver);            // 0x0041169b

		if (dResult != LT_OK)                                       // 0x004116a0
		{
			if (pShell)                                             // 0x004116a8
				pShell->Disconnect(1);                              // 0x004116b6

			return dResult;
		}
#else
		// AVP2 1.0.9.6: one failure tail for both demo arms (0x00410680),
		// and the shell is disconnected without a null test at every site
		// (0x00410688, 0x004106c4, 0x0041071c).
		if (pRequest->m_RecordFilename[0] || pRequest->m_PlaybackFilename[0])
		{
			if (pRequest->m_RecordFilename[0])                      // 0x00410641
				dResult = m_DemoMgr.StartRecording(pWorldName,
				                                   pRequest->m_RecordFilename);  // 0x00410650
			else
				dResult = m_DemoMgr.StartPlayback(pRequest->m_PlaybackFilename,
				                                  worldName, 0x100);  // 0x0041067b

			if (dResult != LT_OK)                                   // 0x00410680
			{
				pShell->Disconnect(1);                              // 0x0041068e
				return dResult;
			}

			pWorldName[0] = 0;                                      // 0x004106a1
		}

		dResult = pShell->StartupLocal(pRequest,
		                               pRequest->m_Type == STARTGAME_HOST,
		                               (uint32)pDriver);            // 0x004106b7

		if (dResult != LT_OK)                                       // 0x004106bc
		{
			pShell->Disconnect(1);                                  // 0x004106ca
			return dResult;
		}
#endif

		// AVP2: an empty world name is now an error rather than a skip
		// (0x004116c9 jumps straight to the report), and the start flags
		// are 0 for STARTGAME_NORMAL, 3 otherwise (0x004116ce..0x004116d8).
#ifdef PRIMAL_HUNT
		nFlags = 3;                                                 // 0x004116ce
		if (pRequest->m_Type == STARTGAME_NORMAL)                   // 0x004116d3
			nFlags = 0;

		if (!pWorldName[0] ||                                       // 0x004116c7
		    pShell->m_pServerMgr->DoStartWorld(pWorldName, nFlags,
		                                       m_fTime125c) != LT_OK)  // 0x004116e6
		{
			pShell->m_pServerMgr->GetErrorString(errorString, 0x100);  // 0x004116fc
			SetupError(LT_SERVERERROR, errorString);                // 0x00411709

			if (pShell)                                             // 0x00411711
				pShell->Disconnect(1);                              // 0x0041171b
#else
		// AVP2 1.0.9.6 starts every world with flags 3 (0x004106f1).
		if (!pWorldName[0] ||                                       // 0x004106e5
		    pShell->m_pServerMgr->DoStartWorld(pWorldName, 3,
		                                       m_fTime125c) != LT_OK)  // 0x004106f4
		{
			pShell->m_pServerMgr->GetErrorString(errorString, 0x100);  // 0x0041070a
			SetupError(LT_SERVERERROR, errorString);                // 0x00410717

			pShell->Disconnect(1);                                  // 0x00410725
#endif

			RETURN_ERROR_SYM_DESC(1, "CClientMgr::StartShell", LT_SERVERERROR,
			                      "error loading world")
		}

		if (pRequest->m_PlaybackFilename[0])                        // 0x00411758
		{
			LTStrCpy(pWorldName, worldName, MAX_SGR_STRINGLEN);     // 0x00411774
		}
		break;
	}
	}

	m_pClientShell2 = pShell;                                       // 0x00411891
	cm_OnShellInitQ2B(this);                                        // 0x00411897

	if (m_pClientShell2)                                            // 0x004118a5
	{
		// AVP2: 0x44c (Primal Hunt 0x004118ad).
#ifdef PRIMAL_HUNT
		cMsg = msg_StartWriteQ2B(0x44c, 0x44c);                     // 0x004118bc
#else
		// AVP2 1.0.9.6 keeps the holder in a register (%ebp, zeroed at
		// 0x00410510) and takes the reference through Init (0x0041084d).
		cMsg.Init(msg_StartWriteQ2B(0x44c, 0x44c));                 // 0x00410848
#endif

		cMsg.m_pMsg->WriteType((uint16)pRequest->m_ClientDataLen);  // 0x004118f2
		((IPacketRefQ2B*)cMsg.m_pMsg)->WriteData(pRequest->m_pClientData,
			(uint16)pRequest->m_ClientDataLen);                     // 0x004119ba
		cMsg.m_pMsg->m_Data.m_pArray[0] = 5;                         // 0x004119c7
		SendToServerQ2B(cMsg.m_pMsg);                               // 0x004119db
	}

	// AVP2: every game type but the two hosts and NORMAL (0x004119e0).
	if (pRequest->m_Type != STARTGAME_HOST &&
	    pRequest->m_Type != STARTGAME_HOSTTCP &&
	    pRequest->m_Type != STARTGAME_NORMAL)
		Sub410e40Q556();                                            // 0x004119f3

	return LT_OK;
}


// ======================================================================= //
//  CClientMgr — teardown and lists
// ======================================================================= //

// Slot 0 of CClientShell's first table (0x004a1d18), called as a thiscall:
// 0x0041036c is `mov (%ecx),%eax; push $1; call *(%eax)` with the shell in
// %ecx and no caller cleanup.  (view; name from clientmgr.h's Disconnect(int))
class IClientShellSlot0View
{
public:
	virtual void Disconnect(int nFlag)=0;                            // 0x00
};

// 0x00410350 — drop the connection.
//
// Two independent things in one function, in this order: the sub-object at
// +0x158c is wound down first (a thiscall on `this + 0x158c`, so it is a
// member and not a pointer), and only then is the shell told to disconnect
// and forgotten.  The `1` the shell's slot 0 is called with is the reason
// clientmgr.h reads that slot as Disconnect(int).
//
// Called from CClientMgr::StartGame (0x0040fd90, first thing it does) and
// from CClientMgr::Term (0x00410f22).
void CClientMgrView::Disconnect()
{
	((CClientMgrTail*)this)->m_Sub158c.Stop();

	CClientShell* pShell = ((CClientMgrTail*)this)->m_pClientShell2;

	if (pShell)
	{
		((IClientShellSlot0View*)pShell)->Disconnect(1);
		((CClientMgrTail*)this)->m_pClientShell2 = LTNULL;
	}
}

// 0x00410380 — CClientMgr::InitMusic, which names itself AppInitMusic.
//
// Refuse with LT_ERROR unless CD audio is enabled (0x004a9f7c) and the
// dword at 0x004b3300 is clear.  Otherwise copy the DLL name (the caller's,
// or "cdaudio.dll") into m_szMusicDLL, terminate it, and load it through
// music_Init (0x00454fb0, wmrotate.cpp); its 1/2/3 failures become
// LT_MISSINGMUSICDLL / LT_INVALIDMUSICDLL / LT_UNABLETOINITMUSICDLL, each
// through SetupError and the engine's RETURN_ERROR (the code strings are
// "36"/"37"/"38", so the macro stringizes the expanded value).
// dsi.cpp's console-bound "MusicEnable" and "ForceSoundDisable".
extern int    g_CV_MusicEnable;                                    // 0x004a9f7c
extern int    g_CV_ForceSoundDisable;                              // 0x004b3300
extern char*  g_pReturnErrorFormatQ14;                             // 0x004a7110
extern int    g_DebugLevelQ14;                                     // 0x004b0ab0
void dsi_OnReturnErrorQ14(int nCode);                              // 0x0042d760
void con_ConsolePrintQ14(char* pFormat, ...);                      // 0x0042e6f0
int  music_InitQ14(char* pDLLName, void* pMusicStruct);           // 0x00454fb0

#define RETURN_ERROR_Q14(level, fnName, code) \
	{ \
		dsi_OnReturnErrorQ14(code); \
		if (g_DebugLevelQ14 >= level) \
			con_ConsolePrintQ14(g_pReturnErrorFormatQ14, fnName, STRINGIZE_Q14(code), " "); \
		return code; \
	}
#define STRINGIZE_Q14(x)  STRINGIZE2_Q14(x)
#define STRINGIZE2_Q14(x) #x

LTRESULT CClientMgrView::InitMusic(char* szMusicDLL)
{
	if (!g_CV_MusicEnable || g_CV_ForceSoundDisable)
		RETURN_ERROR_Q14(1, "CClientMgr::AppInitMusic", LT_ERROR);

	if (szMusicDLL)
		strncpy(m_szMusicDLL, szMusicDLL, sizeof(m_szMusicDLL) - 1);
	else
		strncpy(m_szMusicDLL, "cdaudio.dll", sizeof(m_szMusicDLL) - 1);
	m_szMusicDLL[sizeof(m_szMusicDLL) - 1] = 0;

	switch (music_InitQ14(m_szMusicDLL, m_MusicStruct))
	{
		case 1:
			SetupError(LT_MISSINGMUSICDLL, m_szMusicDLL);
			RETURN_ERROR_Q14(1, "CClientMgr::AppInitMusic", LT_MISSINGMUSICDLL);

		case 2:
			SetupError(LT_INVALIDMUSICDLL, m_szMusicDLL);
			RETURN_ERROR_Q14(1, "CClientMgr::AppInitMusic", LT_INVALIDMUSICDLL);

		case 3:
			SetupError(LT_UNABLETOINITMUSICDLL, m_szMusicDLL);
			RETURN_ERROR_Q14(1, "CClientMgr::AppInitMusic", LT_UNABLETOINITMUSICDLL);
	}

	return LT_OK;
}

// 0x00410510 — hand the renderer its world, through the shell.
//
// The shell's answer is *discarded*: the shipped tail is an unconditional
// `mov $0x1,%eax`, reached from both the taken and the untaken branch, so
// this reports success whether or not the shell managed it.  Written any
// other way — `return pShell->CreateRenderWorld();` with a null guard — the
// return would come out of the call.
LTBOOL CClientMgrView::CreateRenderWorld()
{
	CClientShell* pShell = ((CClientMgrTail*)this)->m_pClientShell2;

	if (pShell)
		pShell->CreateRenderWorld();

	return LTTRUE;
}

// 0x00410530 — and take it back.  A `jmp`, not a call: nothing follows, and
// the callee is a thiscall with no arguments, so the tail call is exact.
void CClientMgrView::DestroyRenderWorld()
{
	CClientShell* pShell = ((CClientMgrTail*)this)->m_pClientShell2;

	if (pShell)
		pShell->DestroyRenderWorld();
}

// 0x00410540 -- give every live texture back to the renderer.
//
// First the render DLL's no-argument entry at RenderStruct+0xf8
// (0x004b4450; clientworldinfo.cpp's table has its one caller here), only
// while the renderer is up and the entry is set; then the same LTList walk
// as FreeList1110Contents below, each payload handed to rif_BindTexture
// (0x004628e0) with 0 and then to the folded empty 0x00490200 with the
// manager.  The two cdecl calls share one `add $0xc`.  (name ours)
void  rif_BindTextureQ411(void* pTexture, LTBOOL bChanged);        // 0x004628e0
// g_pRenderReadConsoleVarsQ411 is RenderStruct+0xf8 (0x004b4450), above.

void CClientMgrView::BindAllTextures()
{
	LTLink* pCur;

	if (g_bRenderInited && g_pRenderReadConsoleVarsQ411)
		g_pRenderReadConsoleVarsQ411();

	LTLink* pHead = &m_List1110.m_Head;
	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		rif_BindTextureQ411(pCur->m_pData, LTFALSE);
		sub_490200((CClientMgrTexBankView*)this);
	}
}

// 0x00410590 — free everything in the 0x1110 list.
//
// The walk is an LTList walk: it starts at m_Head.m_pNext, stops when the
// cursor reaches &m_Head — the shipped `lea 0x1114(%ecx),%edi` is that
// address, taken once before the loop — and each element's payload is the
// link's m_pData at +0x08.  The list is left with its links intact; only the
// payloads go, which is what says this is a free-the-contents pass and not a
// Clear.
void CClientMgrView::FreeList1110Contents()
{
	LTLink* pCur = m_List1110.m_Head.m_pNext;

	while (pCur != &m_List1110.m_Head)
	{
		// The payloads are SharedTextures -- +0x1110 is the live-texture
		// list -- so this releases texture data, it does not free memory.
		tex_FreeTextureData(pCur->m_pData);
		pCur = pCur->m_pNext;
	}
}

// 0x00410620 — publish the manager.
//
// Called twice: once from the constructor at 0x00410ab0 and once from the
// head of CClientMgr::Init.  Both stores are of `this`; the second is to the
// dword one past the last field ConsoleState models, which is how the
// console reaches the client (the server's equivalent is
// ConsoleState::m_pServerMgr at +0x38).
void CClientMgrView::SetGlobals()
{
	((CClientMgrTail*)this)->m_Unknown1610 = 0;
	g_pClientMgr        = this;
	g_pConsoleClientMgr = this;
}

// 0x00410640 — one message the worker thread posted back.
//
// The caller is clientmgr3.cpp's ProcessThreadMessages (0x00410670), which
// drains LThread::m_OutgoingQueue into a 32-byte LThreadMessage on its stack
// and hands each one here (`ret $0x4`, thiscall).  The message is
// src/engine/server/lthread.h's LThreadMessage: two list links, then six
// payload dwords from +0x08.  Only one kind is acted on -- payload[0] (+0x08)
// zero -- and only when payload[2] (+0x10) is zero too, as two `test/jne`
// exits to the one `ret`.  Then payload[4] (+0x18) and payload[3] (+0x14)
// go to 0x004776f0, cmodelload.cpp's model registration
// (pMgr, pModel, pFileRec, bRebind), with bRebind the literal 1.  So this is
// the main thread taking delivery of a model the loader thread finished.
// (message field names ours)
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

// cmodelload.cpp: cm_RegisterModelQ474.
LTRESULT cm_RegisterModel(CClientMgrView* pMgr, void* pModel, void* pFileRec,
                          LTBOOL bRebind);                          // 0x004776f0

void CClientMgrView::HandleThreadMessage(CThreadMessageView* pMsg)
{
	if (pMsg->m_nID == 0 && pMsg->m_dResult == 0)
		cm_RegisterModel(this, pMsg->m_pModel, pMsg->m_pFileRec, LTTRUE);
}

// 0x004156a0 -- the loader thread's ProcessMessage (Primal Hunt 0x004195c0):
// loaderthread.cpp, the loader thread's own translation unit.

// 0x00477fb0 -- CClientShell's override of slot 4 of 0x004a1d18 (the base's
// default is the folded three-argument no-op at 0x004132e0, below).  It
// forwards its three arguments, in order, to the game DLL's shell at
// CClientMgr+0x61c, IClientShell slot 24 (`call *0x60(%edx)`) --
// iclientshell.h's SetDisconnectCode(nCode, pMsg, nSubReason), the slot
// after OnPlaySound (23).  REAL HOME: whichever TU carries the rest of
// 0x004a1d18 (0x00477f40..0x00477fb0); written here with the view the
// field chain needs.  Declared non-virtual on the view, which spells its
// vptrs out as fields.
//
// AVP2: two arguments, not three (`ret $0x8`, PH 0x004939cc; AvP2 1.0.9.6's
// 0x0048a842 too) -- iclientshell.h's SetDisconnectCode takes no sub-reason,
// and neither does this override.
void CClientShell::SetDisconnectCode(uint32 nCode, const char* pMsg)
{
	if (m_pClientMgr->m_pClientShell)
		m_pClientMgr->m_pClientShell->SetDisconnectCode(nCode, pMsg);
}

// 0x004113a0 — clear the 0x1100 list.
//
// The same LTList walk as above with two differences that are both in the
// bytes: the payload goes to 0x00483c60 rather than to dfree, and the head
// is re-tied and the count zeroed afterwards.  Those last three stores are
// exactly LTLink::TieOff() plus `m_nElements = 0` — i.e. ltlink.h's
// dl_InitList — inlined, and they are why this one is a Clear.
//
// cdecl, one argument: the `mov 0x8(%esp),%ebx` is past the `push %ebx`, so
// it is argument one.
void cm_ClearList1100(LTList* pList)
{
	LTLink* pCur = pList->m_Head.m_pNext;

	while (pCur != &pList->m_Head)
	{
		LTLink* pNext = pCur->m_pNext;
		sub_483c60(pCur->m_pData);
		pCur = pNext;
	}

	pList->m_Head.TieOff();
	pList->m_nElements = 0;
}

// 0x004113e0 -- a fresh write message for the client to fill.  __thiscall,
// no arguments; cltrest.cpp's StartHMessageWrite and lt2new3.cpp's
// StartMessage reach it through CLTClient+0x3c0.
//
// clientshell.cpp's SendMessage0A shape: the by-value holder 0x0045c080
// returns (hidden pointer pushed last), copied into a counted local
// (`if (p) p->AddRef()`), the temporary released.  Then ONE unconditional
// AddRef (`mov (%esi),%edx; call *0x4(%edx)` with no test) so the message
// outlives the local, the manager's object helper (+0x4bc) into +0x0c, bit
// 0 into +0x40, and the local's Release on the way out -- %esi returned.
// The helper goes in through an inline setter on the embedded message
// (+0x08, servermsg.h's LMessageImpl, whose +0x04 is the object-IO hook):
// written as a plain field store, VC6 hoists the +0x40 load above it
// (3 blocks); through the setter the store stays first, as shipped.
class CClientMsgImplQ411
{
public:
	void*                   m_pVtable;                              // 0x00
	CClientMsgObjectHelper* m_pObjectHelper;                        // 0x04
	uint8                   m_pad_08[0x20 - 0x08];

	void SetObjectHelper(CClientMsgObjectHelper* p) { m_pObjectHelper = p; }
};

class CClientMsgQ411
{
public:
	virtual void    _p00();
	virtual uint32  AddRef();                                       // slot 1
	virtual uint32  Release();                                      // slot 2

	uint8                   m_pad_04[0x08 - 0x04];
	CClientMsgImplQ411      m_Message;                              // 0x08
	uint8                   m_pad_28[0x40 - 0x28];
	uint32                  m_Flags;                                // 0x40
};

class CMsgRefQ411
{
public:
	CMsgRefQ411() { m_pMsg = LTNULL; }

	CMsgRefQ411& operator=(const CMsgRefQ411& other)
	{
		if (m_pMsg)
			m_pMsg->Release();

		m_pMsg = other.m_pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();

		return *this;
	}

	CMsgRefQ411(const CMsgRefQ411& other)
	{
		m_pMsg = other.m_pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();
	}

	~CMsgRefQ411()
	{
		if (m_pMsg)
			m_pMsg->Release();
	}

	CClientMsgQ411* m_pMsg;                                         // 0x00
};

CMsgRefQ411 msg_StartWriteQ411(uint32 nMaxSize, uint32 nSize);      // 0x0045c080

CClientMsgQ411* CClientMgrView::StartMessage()
{
	// AVP2: the holder is declared empty and then assigned the 0x44c-byte
	// message (Primal Hunt 0x004131e0: the null store at 0x004131f0 ahead of
	// the call, EH states 1 then 0 round the temporary), clientmgr3.cpp's
	// Sub415810 shape.
	//
	// NEAR (PH 170/170, one register): the local's closing Release loads
	// the vtable into %edx, ours into %eax.  The same with a separate holder
	// class or a named return local.
	CMsgRefQ411 cMsg;

	cMsg = msg_StartWriteQ411(0x44c, 0x44c);

	cMsg.m_pMsg->AddRef();
	cMsg.m_pMsg->m_Message.SetObjectHelper(m_pObjectHelper);
	cMsg.m_pMsg->m_Flags |= 1;

	return cMsg.m_pMsg;
}


// ----------------------------------------------------------------------- //
// AVP2: 0x004133c0 (755 bytes; AvP2 1.0.9.6 has its own copy, the
// "; \n\r\t" literal at 0x004d0584 read from 0x00411e24) -- the client's
// twin of smgr.cpp's CServerMgrQ502::CreateStringCRC: sum the CRCs of every
// file a list names.  A thiscall on the manager, `ret $8`, with the list and
// where the sum goes; its one caller is DoLoadWorld (0x00417d5f).  The list
// is split on "; \n\r\t" (0x004e173c); a name without a '*' (0x004e1738)
// is opened as it is, a name with one is a prefix and a suffix, the prefix
// cut back to its directory -- at a '\\' or, unlike the server's, a '/'
// (0x004134ca/0x004134ce) -- whose listing (sub_404610, 0x00404a90) is
// matched with _strnicmp (0x004ce090) on both ends.  Each file goes through
// a type-1 CClientFileRef (two of them, one per arm: the 0x5b0 frame) to the
// file manager's open (0x00404d30) and through ILTCommon::GetCRC (slot 15)
// of the interface at +0x4c4.  No file at all is a sum of 1; no list is 0
// (PH only, below).  The file count is kept and never read.  Always LT_OK
// (names ours).  IDENTICAL in both images (PH 755 bytes, AvP2 655).
// clientmgr.h's CClientFileRef, which clientwin.cpp's open reads as its
// CluFileIdent.
struct CluFileIdent
{
	CluFileIdent() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

class CFileMgr;
class CluFileMgr;
class ILTStream;
FileEntry* sub_404610(CFileMgr* pFileMgr, char* pDirName);         // 0x00404a90 (clientinterface.cpp)
ILTStream* clu_fm_OpenFile(CluFileMgr* pMgr, CluFileIdent* pIdent); // 0x00404d30 (clientwin.cpp)
void       fs_FreeFileList(FileEntry* pList);                       // 0x00442850

class ILTStreamCRCQ558
{
public:
	virtual void     _v00();
	virtual void     Release();              // 0x04
};

// ILTCommon (iltcommon.h) to GetCRC, slot 15 (`call *0x3c(%edx)`, 0x004135c1).
class ILTCommonCRCQ558
{
public:
	virtual void _v00(); virtual void _v04(); virtual void _v08();
	virtual void _v0c(); virtual void _v10(); virtual void _v14();
	virtual void _v18(); virtual void _v1c(); virtual void _v20();
	virtual void _v24(); virtual void _v28(); virtual void _v2c();
	virtual void _v30(); virtual void _v34(); virtual void _v38();
	virtual LTRESULT GetCRC(ILTStreamCRCQ558* pStream, uint32& dwResult);   // 0x3c
};

// ILTCSBase's members (iltcsbase.h): vptr, ILTMath, then the common.
class ILTCSBaseCRCQ558
{
public:
	ILTCommonCRCQ558* Common() { return m_pCommonLT; }

	uint8             m_pad_00[0x08];
	ILTCommonCRCQ558* m_pCommonLT;           // 0x08
};

class CClientMgrCRCQ558
{
public:
	LTRESULT CreateStringCRC(char* pFiles, uint32* pCRC);

	uint8             m_pad_000[0x4c4];
	ILTCSBaseCRCQ558* m_pClientLT;           // 0x4c4
	uint8             m_pad_4c8[0x1644 - 0x4c8];
	CFileMgr*         m_pFileMgr;            // 0x1644  (clientmgr.h)
};

LTRESULT CClientMgrCRCQ558::CreateStringCRC(char* pFiles, uint32* pCRC)
{
	char              szPrefix[0x80];
	char              szDir[0x80];
	char              szSuffix[0x80];
	char              szFiles[0x400];
	uint32            nCRC;
	uint32            nFiles;
	uint32            dwCRC;
	char*             pTok;
	ILTStreamCRCQ558* pStream;

	if (pFiles && pFiles[0])
	{
		strcpy(szFiles, pFiles);

		nFiles = 0;
		nCRC   = 0;

		pTok = strtok(szFiles, "; \n\r\t");
		while (pTok)
		{
			char* pStar = strstr(pTok, "*");

			if (pStar)
			{
				strncpy(szPrefix, pTok, pStar - pTok);
				strcpy(szSuffix, pStar + 1);
				szPrefix[pStar - pTok] = 0;

				strcpy(szDir, szPrefix);
				for (int i = strlen(szDir); i > 0 && szDir[i] != '\\' && szDir[i] != '/'; i--)
					szDir[i] = 0;

				FileEntry* pList  = sub_404610(m_pFileMgr, szDir);
				FileEntry* pEntry = pList;
				uint32 nPrefixLen = strlen(szPrefix);
				uint32 nSuffixLen = strlen(szSuffix);

				for (; pEntry; pEntry = pEntry->m_pNext)
				{
					if (pEntry->m_Type == TYPE_FILE &&
					    !_strnicmp(pEntry->m_pFullFilename, szPrefix, nPrefixLen) &&
					    !_strnicmp(pEntry->m_pFullFilename +
					               strlen(pEntry->m_pFullFilename) - nSuffixLen,
					               szSuffix, nSuffixLen))
					{
						CluFileIdent ref;

						ref.m_nFileType = 1;
						ref.m_pFilename = pEntry->m_pFullFilename;

						pStream = (ILTStreamCRCQ558*)clu_fm_OpenFile((CluFileMgr*)m_pFileMgr, &ref);
						if (pStream)
						{
							m_pClientLT->Common()->GetCRC(pStream, dwCRC);
							nCRC += dwCRC;
							nFiles++;
							pStream->Release();
						}
					}
				}

				fs_FreeFileList(pList);
			}
			else
			{
				CluFileIdent ref;

				ref.m_nFileType = 1;
				ref.m_pFilename = pTok;

				pStream = (ILTStreamCRCQ558*)clu_fm_OpenFile((CluFileMgr*)m_pFileMgr, &ref);
				if (pStream)
				{
					m_pClientLT->Common()->GetCRC(pStream, dwCRC);
					nCRC += dwCRC;
					nFiles++;
					pStream->Release();
				}
			}

			pTok = strtok(LTNULL, "; \n\r\t");
		}

		if (!nCRC)
			nCRC = 1;

		*pCRC = nCRC;
	}
#ifdef PRIMAL_HUNT
	else
	{
		// PH: no list is a sum of 0 (0x0041369c); AvP2 1.0.9.6 leaves the
		// caller's value alone (its empty path jumps straight to the
		// shared epilogue, 0x00411df6 -> 0x00412063).
		*pCRC = 0;
	}
#endif

	return LT_OK;
}

// 0x00411440 — give a packet the manager's message object helper.
//
// `mov 0x4bc(%ecx),%eax; mov 0x4(%esp),%ecx; mov %eax,0xc(%ecx); ret $4`:
// a thiscall storing CClientMgr+0x4bc (the helper) into +0x0c of its one
// argument.  Called from cchandler.cpp (on the packet it is about to hand
// the game) and lt2new1.cpp (on a message).  (Name and view ours.)
class CPacketHelperView
{
public:
	uint8                   m_pad_00[0x0c];
	CClientMsgObjectHelper* m_pHelper;            // 0x0c
};

void CClientMgrView::SetPacketHelper(CPacketHelperView* pPacket)
{
	pPacket->m_pHelper = m_pObjectHelper;
}

// 0x00411450 — step the manager's 16-bit code at +0x21ac and return it.
//
// 0xffff is never handed out: at the wrap the code goes back to 1 and the
// word at +0x30 of every payload in the 0x1110 list is cleared, so nothing
// still holds a code from the previous lap.  clientrender.cpp's render
// interface slot +0x30 (0x004623a0) is a thiscall of this on g_pClientMgr.
// (Name and the payload view ours.)
//
// The head is a named local taken before the `= 1` store: shipped has the
// `lea 0x1114(%ecx),%edx` first and reads the first link through %edx
// after the store.  Walking `m_List1110.m_Head` directly loads +0x1118 off
// %ecx ahead of the store instead (3 blocks).
class CFrameCodeHolderView
{
public:
	uint8   m_pad_00[0x30];
	uint16  m_nFrameCode;                         // 0x030
};

uint16 CClientMgrView::NextFrameCode()
{
	if (m_nFrameCode == 0xffff)
	{
		LTLink* pHead = &m_List1110.m_Head;

		m_nFrameCode = 1;

		for (LTLink* pCur = pHead->m_pNext; pCur != pHead;
		     pCur = pCur->m_pNext)
		{
			((CFrameCodeHolderView*)pCur->m_pData)->m_nFrameCode = 0;
		}
	}
	else
	{
		m_nFrameCode++;
	}

	return m_nFrameCode;
}


// 0x004114a0 -- CClientMgr::FreeUnusedModels (cltrest.cpp's slot 38 is a
// tail jump here).  Three walks, then LT_OK:
//
//   1. every model on m_Models: use count (+0x208) up by one, bit 0 of
//      the flags (+0x20) cleared (`and $0xfffffffe`);
//   2. every object in record 1 of the eleven (&m_Head at +0x420, the
//      model list) whose server id (+0xb8) is 0xffff -- a client-only
//      object -- sets bit 0 on its model (+0x1dc);
//   3. every model on m_Models, the next link taken first: the count comes
//      back down, and a model that reaches zero without bit 0 is unlinked
//      (ltlink.h's Remove inline: prev/next re-tied, then TieOff) and
//      deleted through its virtual destructor (`push 1; call *(%eax)`).
//
// So a model survives if anything held it before the pass or a client-only
// object is using it.  (views ours)
class CModelQ411
{
public:
	virtual ~CModelQ411();

	uint8   m_pad_04[0x08 - 0x04];
	LTLink  m_Link;                                 // 0x008
	uint8   m_pad_14[0x20 - 0x14];
	uint32  m_Flags;                                // 0x020
	uint8   m_pad_24[0x20c - 0x24];
	// AVP2: 0x20c, and every release is guarded against going below zero
	// (`test; jbe` round the decrement, 0x00413371) -- clientmgr3.cpp's
	// CCmgRefCounted::Release on the same object.
	uint32  m_RefCount;                             // 0x20c

	void    Release() { if (m_RefCount > 0) m_RefCount--; }
};

class CModelObjQ411
{
public:
	uint8       m_pad_000[0xb8];
	uint16      m_nServerID;                        // 0x0b8
	// AVP2: 0x1fc (NOLF's was 0x1dc). PH: the model's extra texture at
	// +0x1d0 puts it 4 further on than AvP2 1.0.9.6's 0x1f8 (0x00411d6e).
#ifdef PRIMAL_HUNT
	uint8       m_pad_0ba[0x1fc - 0xba];
#else
	uint8       m_pad_0ba[0x1f8 - 0xba];
#endif
	CModelQ411* m_pModel;                           // 0x1fc (AvP2 0x1f8)
};

LTRESULT CClientMgrView::FreeUnusedModels()
{
	LTLink* pCur;
	LTLink* pNext;
	LTLink* pHead;
	LTLink* pModels = &m_Models;
	CModelQ411* pModel;
	CModelObjQ411* pObj;

	for (pCur = pModels->m_pNext; pCur != pModels; pCur = pCur->m_pNext)
	{
		pModel = (CModelQ411*)pCur->m_pData;
		pModel->m_RefCount++;
		pModel->m_Flags &= ~1;
	}

	pHead = &((LTList*)m_Records40c)[1].m_Head;
	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		pObj = (CModelObjQ411*)pCur->m_pData;
		if (pObj->m_nServerID == 0xffff)
			pObj->m_pModel->m_Flags |= 1;
	}

	for (pCur = pModels->m_pNext; pCur != pModels; pCur = pNext)
	{
		pModel = (CModelQ411*)pCur->m_pData;
		pNext = pCur->m_pNext;

		pModel->Release();
		if (pModel->m_RefCount == 0 && !(pModel->m_Flags & 1))
		{
			pModel->m_Link.Remove();
			delete pModel;
		}
	}

	return LT_OK;
}

// 0x00411670 — run one stage of the console commands.
//
// CClientMgr::Init calls this twice with the same string and stages 2 then
// 1, with the render DLL brought up in between; the fourth argument of the
// console call is a literal 1 at both sites, never a variable.  The store of
// the manager into the console's back pointer is repeated here even though
// Init has just done it, which is what makes this a self-contained entry
// point rather than a piece of Init.
void cm_ParseConsoleCommands(CClientMgrView* pClientMgr, char* pCommands,
                             int nStage)
{
	g_pConsoleClientMgr = pClientMgr;
	con_ParseCommands(&g_ClientConsoleStateRef, pCommands, nStage, 1);
}

// 0x004116a0 — run a console file, if the console is up.
//
// The only caller passes "autoexec.cfg" (0x00410f27), from CClientMgr::Term's
// neighbour.  The gate is the flag CClientMgr::Init sets to 1 at 0x0040fbcb,
// so a file is only run once the console has been configured.
void cm_RunConsoleFile(CClientMgrView* pClientMgr, char* pFilename)
{
	if (pClientMgr->m_bConsoleReady)
		con_RunFile(&g_ClientConsoleStateRef, pFilename);
}

// g_dwDirtyFlags4b44a4 is RenderStruct+0x14c (0x004b44a4), above.

// 0x004116d0 — bring the renderer up in the configured mode.  cdecl, one
// argument, answering r_InitRender's LTRESULT.  The frame is exactly one
// ltbasedefs.h RMode (0x214 bytes -- 0x0040f810 clears it with a 0x85-dword
// `rep stos` before filling in the "renderdll" name and the three .data
// dimensions), both calls' arguments are cleaned by one `add $0x220`.  Its
// callers are the RestartRender / ResizeScreen console commands in
// ccclient.cpp, which OR 0x04000000 into a failure.  (Names ours.)
void     cm_GetRenderModeQ5(RMode* pMode);                            // 0x0040f810
LTRESULT r_InitRenderQ5(CClientMgrView* pClientMgr, RMode* pMode);    // 0x004623d0

LTRESULT sub_4116d0(CClientMgrView* pClientMgr)
{
	RMode mode;

	cm_GetRenderModeQ5(&mode);
	return r_InitRenderQ5(pClientMgr, &mode);
}

// 0x00411700 — render one camera: fill a SceneDesc on the stack and hand it
// to the render DLL's RenderScene (RenderStruct slot +0xb0, 0x004b4408).
// cdecl (pClientMgr, pCamera, nDrawMode, p4, p5), LTBOOL.  The field map is
// clientblit.cpp's section B; the d3d side reads it in big2d1c0.cpp.
//
// The call to 0x00411a30 at entry is NOT LTRotation's constructor: it is
// SceneDesc::m_Rect's default construction, ltrect.h's
// `LTRect(int l=0, int t=0, int r=0, int b=0)`, emitted out of line and
// folded by the linker with LTRotation(float,float,float,float) -- both are
// four dword stores and `ret $0x10`.  The frame is exactly SceneDesc (0xec)
// plus the thirty-entry object list (0x78), the larger aggregate lower.
// (Names ours.)
struct SceneDescQ5
{
	int          m_DrawMode;                       // 0x00
	int*         m_pTicks[6];                      // 0x04  .data counters
	LTVector     m_ModelAdd;                       // 0x1c  g_ModelAdd
	LTVector     m_ModelDirAdd;                    // 0x28  g_ModelDirAdd
	void*        m_pRenderWorld;                   // 0x34
	LTVector     m_GlobalLightDir;                 // 0x38  mgr+0x760
	LTVector     m_GlobalLightColor;               // 0x44  mgr+0x754
	LTVector     m_GlobalLightScale;               // 0x50  mgr+0x76c
#ifndef PRIMAL_HUNT
	// AVP2: two more vectors, the manager's at +0x778 and ccclient.cpp's
	// "WMAmbient" triple (cc_WMAmbient, 0x00422480), which push everything
	// after them 0x18 bytes on (0x00412461, 0x004124e1; names ours).
	LTVector     m_GlobalLightScale2;              // 0x5c  mgr+0x778
	LTVector     m_WMAmbient;                      // 0x68  g_fWMAmbientR/G/B
#endif
	LTVector     m_GlobalLightAdd;                 // 0x5c  camera+0x1c4
	float        m_FrameTime;                      // 0x68
	SkyDef       m_SkyDef;                         // 0x6c
	HLOCALOBJ*   m_pObjectList;                    // 0x9c
	int          m_ObjectListSize;                 // 0xa0
	LTRect       m_Rect;                           // 0xa4
	float        m_xFov, m_yFov;                   // 0xb4
	float        m_FarZ;                           // 0xbc
	LTVector     m_Pos;                            // 0xc0
	LTRotation   m_Rotation;                       // 0xcc
	void*        m_pUser4;                         // 0xdc
	void*        m_pUser5;                         // 0xe0
	ModelHookFn  m_ModelHookFn;                    // 0xe4
	void*        m_ModelHookUser;                  // 0xe8
};

class CCameraQ5
{
public:
	uint8        m_pad_00[0x9c];
	LTRotation   m_Rotation;                       // 0x09c
	uint8        m_pad_ac[0x194 - 0xac];
	LTVector     m_Pos;                            // 0x194
	uint8        m_pad_1a0[0x1b0 - 0x1a0];
	LTRect       m_Rect;                           // 0x1b0
	float        m_xFov, m_yFov;                   // 0x1c0
	LTBOOL       m_bFullScreen;                    // 0x1c8
	LTVector     m_LightAdd;                       // 0x1cc
};

// The sub-object at CClientMgr+0x158c (CClientMgrTail's m_Sub158c), whose
// first dword RenderCamera bumps.  Written as a member bump the load of it
// is hoisted above the `m_bRendering = FALSE` store; through an inline
// method on the sub-object it stays below, as shipped (0x004119f9).
class CFrameCounterQ5
{
public:
	void Inc() { m_nCount++; }
	uint32 m_nCount;
};

class CRenderMgrQ5
{
public:
	uint8        m_pad_00[0x754];
	LTVector     m_GlobalLightColor;               // 0x754
	LTVector     m_GlobalLightDir;                 // 0x760
	LTVector     m_GlobalLightScale;               // 0x76c
#ifndef PRIMAL_HUNT
	LTVector     m_GlobalLightScale2;              // 0x778  (name ours)
	uint8        m_pad_784[0x1270 - 0x784];
#else
	uint8        m_pad_778[0x1270 - 0x778];
#endif
	uint16       m_ObjectIDs[30];                  // 0x1270
	uint8        m_pad_12ac[0x12b0 - 0x12ac];
	SkyDef       m_SkyDef;                         // 0x12b0
	uint8        m_pad_12e0[0x1368 - 0x12e0];
	float        m_FrameTime;                      // 0x1368
	uint8        m_pad_136c[0x161c - 0x136c];
	CClientShell* m_pClientShell;                  // 0x161c
	uint8        m_pad_1620[0x1634 - 0x1620];
	ModelHookFn  m_ModelHookFn;                    // 0x1634
	void*        m_ModelHookUser;                  // 0x1638
	LTBOOL       m_bRendering;                     // 0x163c
	uint8        m_pad_1640[0x169c - 0x1640];
	CFrameCounterQ5 m_RenderFrame;                 // 0x169c
};

// clientrender.cpp's RenderStruct at 0x004b4358, as much of it as this
// function reads: the mode's size at +0x3c/+0x40 (0x004b4394/0x004b4398).
class RenderStruct
{
public:
	uint8  m_pad_00[0x3c];
	int    m_Width;                            // 0x03c
	int    m_Height;                           // 0x040
};

extern RenderStruct g_RenderStruct;                // 0x004b4358
extern LTVector g_ModelAdd;                        // 0x004b3228
extern LTVector g_ModelDirAdd;                     // 0x004b3218
// dsi.cpp's console-bound "FarZ" and "RenderEnable"; the scene entry is
// RenderStruct+0xb0 (0x004b4408), above.
extern float    g_CV_FarZ;                         // 0x004a9f00
extern int      g_CV_RenderEnable;                 // 0x004a9eb8
// Six .bss counters the renderer writes back through the scene's pointers.
// No other code in the image reaches 0x004b1590..0x004b15a7, so they are
// defined here, beside the one function that hands them out (name ours).
int             g_TicksQ5[6];                      // 0x004b1590..0x004b15a4
void* tbl_GetObject(CClientMgrView* pClientMgr, uint16 nID);  // 0x00420520

// Stand-in (name ours): an inline LTVector copy.  Shipped spends the /O2
// inline budget on the later sites of this function and leaves the first
// one -- SceneDesc::m_Rect's LTRect() -- as a call to 0x00411a30.
inline void VecCopyQ5(LTVector& vDest, const LTVector& vSrc)
{
	vDest = vSrc;
}

#ifndef PRIMAL_HUNT
// AVP2: counter.cpp's timer and clientmgr3.cpp's adder, as AvP2 1.0.9.6's
// RenderCamera uses them, and the two totals it adds to (0x004defa8,
// 0x004ded10; names ours).
class CBaseCounter
{
public:
	CBaseCounter(int nMode);

	uint32 m_Data[2];
};
void   cnt_StartCounter(CBaseCounter* pCounter);
uint32 cnt_EndCounter(CBaseCounter* pCounter);

class CRenderCountAdderQ5
{
public:
	CRenderCountAdderQ5(uint32* pTotal) : m_Counter(0)
	{
		m_pTotal = pTotal;
		cnt_StartCounter(&m_Counter);
	}

	~CRenderCountAdderQ5()
	{
		*m_pTotal += cnt_EndCounter(&m_Counter);
	}

	CBaseCounter m_Counter;                        // 0x00
	uint32*      m_pTotal;                         // 0x08
};

// ccclient.cpp's "WMAmbient" (0x004e36a8..0x004e36b0).
extern float g_fWMAmbientR;
extern float g_fWMAmbientG;
extern float g_fWMAmbientB;

uint32 g_nRenderTicksQ5;                           // 0x004defa8
uint32 g_nSceneTicksQ5;                            // 0x004ded10
#endif

LTBOOL cm_RenderCamera(CRenderMgrQ5* pClientMgr, CCameraQ5* pCamera,
                       int nDrawMode, void* pUser4, void* pUser5)
{
#ifndef PRIMAL_HUNT
	// AVP2: a timer built first (0x00412272, before m_Rect's constructor)
	// and never started or read; it sits above sceneDesc in the frame.
	CBaseCounter cntUnused(0);
#endif
	SceneDescQ5 sceneDesc;
	HLOCALOBJ   objectList[30];
	int         i;
	HLOCALOBJ   hObj;
	int         nScreenWidth, nScreenHeight;

	if (g_bRenderInited)
	{
		if (!pClientMgr->m_bRendering)
		{
			sceneDesc.m_DrawMode = nDrawMode;
			nScreenWidth = g_RenderStruct.m_Width;
			nScreenHeight = g_RenderStruct.m_Height;

			if (pClientMgr->m_pClientShell && pClientMgr->m_pClientShell->GetClientMgr())
				sceneDesc.m_pRenderWorld = pClientMgr->m_pClientShell->GetClientMgr()->m_pRenderWorld;
			else
				sceneDesc.m_pRenderWorld = LTNULL;

			VecCopyQ5(sceneDesc.m_Pos, pCamera->m_Pos);
			sceneDesc.m_Rotation = pCamera->m_Rotation;
			sceneDesc.m_xFov = pCamera->m_xFov;
			sceneDesc.m_yFov = pCamera->m_yFov;

			if (pCamera->m_bFullScreen)
			{
				sceneDesc.m_Rect.left = sceneDesc.m_Rect.top = 0;
				sceneDesc.m_Rect.right = nScreenWidth;
				sceneDesc.m_Rect.bottom = nScreenHeight;
			}
			else
			{
				sceneDesc.m_Rect.left = pCamera->m_Rect.left;
				sceneDesc.m_Rect.top = pCamera->m_Rect.top;
				sceneDesc.m_Rect.right = pCamera->m_Rect.right;
				sceneDesc.m_Rect.bottom = pCamera->m_Rect.bottom;
			}

			sceneDesc.m_FarZ = g_CV_FarZ;

			VecCopyQ5(sceneDesc.m_GlobalLightDir, pClientMgr->m_GlobalLightDir);
			sceneDesc.m_pTicks[0] = &g_TicksQ5[5];
			sceneDesc.m_pTicks[1] = &g_TicksQ5[4];
			sceneDesc.m_pTicks[2] = &g_TicksQ5[2];
			sceneDesc.m_pTicks[3] = &g_TicksQ5[3];
			sceneDesc.m_pTicks[4] = &g_TicksQ5[1];
			sceneDesc.m_pTicks[5] = &g_TicksQ5[0];
			VecCopyQ5(sceneDesc.m_GlobalLightColor, pClientMgr->m_GlobalLightColor);
			VecCopyQ5(sceneDesc.m_GlobalLightScale, pClientMgr->m_GlobalLightScale);
#ifndef PRIMAL_HUNT
			VecCopyQ5(sceneDesc.m_GlobalLightScale2, pClientMgr->m_GlobalLightScale2);
#endif
			VecCopyQ5(sceneDesc.m_GlobalLightAdd, pCamera->m_LightAdd);
			VecCopyQ5(sceneDesc.m_ModelAdd, g_ModelAdd);
			VecCopyQ5(sceneDesc.m_ModelDirAdd, g_ModelDirAdd);
#ifndef PRIMAL_HUNT
			sceneDesc.m_WMAmbient.x = g_fWMAmbientR;
			sceneDesc.m_WMAmbient.y = g_fWMAmbientG;
			sceneDesc.m_WMAmbient.z = g_fWMAmbientB;
#endif
			sceneDesc.m_FrameTime = pClientMgr->m_FrameTime;
			sceneDesc.m_pUser4 = pUser4;
			sceneDesc.m_pUser5 = pUser5;
			sceneDesc.m_SkyDef = pClientMgr->m_SkyDef;

			sceneDesc.m_pObjectList = objectList;
			sceneDesc.m_ObjectListSize = 0;
			for (i = 0; i < 30; i++)
			{
				hObj = (HLOCALOBJ)tbl_GetObject((CClientMgrView*)pClientMgr,
				                                pClientMgr->m_ObjectIDs[i]);
				objectList[sceneDesc.m_ObjectListSize] = hObj;
				if (hObj)
					sceneDesc.m_ObjectListSize++;
			}

			sceneDesc.m_ModelHookFn = pClientMgr->m_ModelHookFn;
			sceneDesc.m_ModelHookUser = pClientMgr->m_ModelHookUser;

			pClientMgr->m_bRendering = LTTRUE;
#ifndef PRIMAL_HUNT
			{
				// AVP2: the render is timed into two totals
				// (0x004125ad..0x00412628).
				CRenderCountAdderQ5 cntRender(&g_nRenderTicksQ5);
				CRenderCountAdderQ5 cntScene(&g_nSceneTicksQ5);

				if (g_CV_RenderEnable)
					g_pfnRenderSceneQ5(&sceneDesc);
			}
#else
			if (g_CV_RenderEnable)
				g_pfnRenderSceneQ5(&sceneDesc);
#endif
			pClientMgr->m_bRendering = LTFALSE;

			pClientMgr->m_RenderFrame.Inc();
			g_dwDirtyFlags4b44a4 = 0;
			return LTTRUE;
		}
	}

	return LTFALSE;
}

// 0x00411a50 — free the 0x1110 list's contents and then whatever 0x00410540
// holds.  cdecl, one argument; both callees are reached with `this` in %ecx
// 0x00411a50 — free the 0x1110 list's contents and then bind them again
// (0x00410540).  cdecl, one argument; both callees are reached with `this` in %ecx
// off the same pointer, which is what says the argument is the manager.
void cm_FreeList1110AndMore(CClientMgrView* pClientMgr)
{
	pClientMgr->FreeList1110Contents();
	pClientMgr->BindAllTextures();
}

// 0x00411c80 — hand every object in one of the eleven records to 0x00411bf0.
//
// cdecl (pClientMgr, pRecord, nFlag).  Each sixteen-byte record at +0x40c is
// an ltlink.h LTList: `lea 0x4(%eax)` is &m_Head and +0x08 its m_pNext.  The
// next link is taken before the call (0x00411c9a), so the callee may unlink
// the element -- cltnew.cpp's RemoveObject returns what 0x00411bf0 says.
// With nFlag set, objects whose server id (+0xb8) is 0xffff -- client-only
// objects -- are left alone: ResetRecords (0x00413cf0) passes 1, CClientMgr's
// teardown in clientmgr3.cpp passes 0.  (Name ours.)
LTRESULT sub_411bf0(CClientMgrView* pClientMgr, CClientObjIDView* pObj); // 0x00411bf0

void sub_411c80(CClientMgrView* pClientMgr, void* pRecord, int nFlag)
{
	// The head as a named local is what puts it in %edi and the flag in
	// %ebx as shipped; the for-loop over pList->m_Head swaps the two.
	LTLink* pHead = &((LTList*)pRecord)->m_Head;
	LTLink* pCur  = pHead->m_pNext;

	while (pCur != pHead)
	{
		LTLink* pNext = pCur->m_pNext;
		CClientObjIDView* pObj = (CClientObjIDView*)pCur->m_pData;

		if (!nFlag || pObj->m_nServerID != 0xffff)
			sub_411bf0(pClientMgr, pObj);

		pCur = pNext;
	}
}

// 0x00411a70 — make one client object from a create request: cdecl
// (pClientMgr, nServerID, pRequest, phObject, bMove, bRotate), LT_OK or
// the first failure.  The object manager at +0x1ec allocates it
// (0x0045bcf0), the type-indexed create hook initialises it (0x00477d60,
// cmodelload.cpp's counterpart of sub_411bf0's 0x00477d90) and on failure
// the manager frees it again.  A real server id is claimed in the table at
// +0x11dc.  Then the two flags choose which of the position/rotation
// setters run, a non-unit scale is applied, world models and containers
// get 0x00450fc0, and bit 0 of 0x004b44a4 is set -- the mirror image of
// sub_411bf0's bit 1.  Callers: csvis.cpp (1,0 and 0,0) and
// clientinterface.cpp's CreateObject (1,1).  (Names ours.)
class CObjMapEntryQ5
{
public:
	uint8             m_bUsed;                     // 0x00
	uint8             m_pad_01[3];
	CClientObjIDView* m_pObj;                      // 0x04
};

class CClientCreateRequestQ5
{
public:
	// The file refs: the object and its four skins, 0x3c bytes (AvP2
	// 1.0.9.6 reads the create struct at `mov 0x3c(%reg)`, 0x004126c0);
	// PH: and the model's extra texture, 0x48 (cltapi.cpp's
	// CClientCreateRequest).
#ifdef PRIMAL_HUNT
	uint8               m_pad_00[0x48];
#else
	uint8               m_pad_00[0x3c];
#endif
	ObjectCreateStruct* m_pStruct;                 // 0x48 (AvP2 0x3c)
};

LTRESULT om_AllocObjectQ5(void* pObjectMgr, ObjectCreateStruct* pStruct,
                          CClientObjIDView** ppObj);                 // 0x0045bcf0
LTRESULT obj_CreateHookQ5(CClientMgrView* pClientMgr, CClientObjIDView* pObj,
                          CClientCreateRequestQ5* pRequest, int nUnused); // 0x00477d60
void     tbl_ClaimSlotQ5(CClientMgrView* pClientMgr, uint16 nID);  // 0x00420460
void     cm_MoveAndRotateQ5(CClientMgrView* pClientMgr, CClientObjIDView* pObj,
                            LTVector* pPos, LTRotation* pRot);       // 0x00420930
void     cm_MoveObjectQ5(CClientMgrView* pClientMgr, CClientObjIDView* pObj,
                         LTVector* pPos, int bForce);                // 0x004207a0
void     cm_RotateObjectQ5(CClientMgrView* pClientMgr, CClientObjIDView* pObj,
                           LTRotation* pRot);                        // 0x00420880
void     cm_ScaleObjectQ5(CClientMgrView* pClientMgr, CClientObjIDView* pObj,
                          LTVector* pScale);                         // 0x00420580
void     obj_Sub450fc0Q5(CClientObjIDView* pObj);                    // 0x00450fc0
void     om_RemoveObjectQ5(void* pObjectMgr, CClientObjIDView* pObj); // 0x0045bde0

LTRESULT sub_411a70(CClientMgrView* pClientMgr, uint16 nServerID,
                    CClientCreateRequestQ5* pRequest, CClientObjIDView** phObject,
                    int bMove, int bRotate)
{
	CClientObjIDView*   pObj;
	ObjectCreateStruct* pStruct;
	LTRESULT            dResult;

	*phObject = LTNULL;

	dResult = om_AllocObjectQ5(pClientMgr->m_ObjectMgr1ec, pRequest->m_pStruct,
	                           &pObj);                                  // 0x00411a96
	if (dResult != LT_OK)
		return dResult;

	dResult = obj_CreateHookQ5(pClientMgr, pObj, pRequest, 0);         // 0x00411aae
	if (dResult != LT_OK)
	{
		om_RemoveObjectQ5(pClientMgr->m_ObjectMgr1ec, pObj);           // 0x00411ac2
		return dResult;
	}

	pObj->m_nServerID = nServerID;                                     // 0x00411ada

	if (pObj->m_nServerID != 0xffff)                                   // 0x00411aec
	{
		tbl_ClaimSlotQ5(pClientMgr, pObj->m_nServerID);                // 0x00411af5
		pClientMgr->m_pObjMap11dc[pObj->m_nServerID].m_bUsed = 1;      // 0x00411b10
		pClientMgr->m_pObjMap11dc[pObj->m_nServerID].m_pObj  = pObj;   // 0x00411b27
	}

	if (bMove)                                                         // 0x00411b33
	{
		if (bRotate)
			cm_MoveAndRotateQ5(pClientMgr, pObj, &pRequest->m_pStruct->m_Pos,
			                   &pRequest->m_pStruct->m_Rotation);      // 0x00411b4c
		else
			cm_MoveObjectQ5(pClientMgr, pObj, &pRequest->m_pStruct->m_Pos, 1);  // 0x00411b61
	}
	else if (bRotate)
	{
		cm_RotateObjectQ5(pClientMgr, pObj, &pRequest->m_pStruct->m_Rotation);  // 0x00411b78
	}

	pStruct = pRequest->m_pStruct;                                     // 0x00411b84

	if (pStruct->m_Scale.x != 1.0f || pStruct->m_Scale.y != 1.0f ||
	    pStruct->m_Scale.z != 1.0f)                                    // 0x00411b92
		cm_ScaleObjectQ5(pClientMgr, pObj, &pStruct->m_Scale);         // 0x00411ba3

	if (pObj->m_ObjectType == OT_WORLDMODEL || pObj->m_ObjectType == OT_CONTAINER)
		obj_Sub450fc0Q5(pObj);                                         // 0x00411bc0

	*phObject = pObj;                                                  // 0x00411bd0
	g_dwDirtyFlags4b44a4 |= 1;                                         // 0x00411bd9
	return LT_OK;
}

// 0x00411bf0 — take one object out of the client world.  cdecl
// (pClientMgr, pObj), always LT_OK.  Every callee is cited by address:
// 0x00450b50 / 0x00450b90 / 0x00429740 (obj_FreeData) take the object
// alone and their three pushes are cleaned by one `add $0xc`; the game
// shell's OnObjectRemove (IClientShell slot 11, +0x2c) runs only for
// objects with bit 0 of +0x180 set; 0x00477d90 is the type-indexed
// remove hook (cmodelload.cpp); a server id other than 0xffff goes back
// through 0x004204f0 (uint16 parameter -- the caller pushes all of %eax);
// OT_MODEL objects have their textures released by 0x00420330; and the
// object manager at +0x1ec frees it.  Last, bit 1 of 0x004b44a4 is set.
// (Names ours.)
void     obj_Sub450b50Q5(CClientObjIDView* pObj);                    // 0x00450b50
void     obj_Sub450b90Q5(CClientObjIDView* pObj);                    // 0x00450b90
void     obj_FreeDataQ5(CClientObjIDView* pObj);                     // 0x00429740
// cmodelload.cpp's cm_RunRemoveHookQ474, under its own name and types; the
// answer is ignored here.
class CClientMgrQ474;
class CClientHookObjQ474;
LTRESULT cm_RunRemoveHookQ474(CClientMgrQ474* pMgr, CClientHookObjQ474* pObject); // 0x00477d90
void     tbl_ReleaseSlotQ5(CClientMgrView* pClientMgr, uint16 nID);  // 0x004204f0
// cltapi.cpp's cm_ReleaseObjectTexturesQ404, under its own name and types
// (AvP2 1.0.9.6 0x004263f0, Primal Hunt 0x00420330).
class CClientMgr;
class CClientObject;
void     cm_ReleaseObjectTexturesQ404(CClientMgr* pClientMgr, CClientObject* hObj);
void     om_RemoveObjectQ5(void* pObjectMgr, CClientObjIDView* pObj); // 0x0045bde0

LTRESULT sub_411bf0(CClientMgrView* pClientMgr, CClientObjIDView* pObj)
{
	obj_Sub450b50Q5(pObj);
	obj_Sub450b90Q5(pObj);
	obj_FreeDataQ5(pObj);

	if (pObj->m_ClientFlags180 & 1)
	{
		if (pClientMgr->m_pClientShell)
			pClientMgr->m_pClientShell->OnObjectRemove((HLOCALOBJ)pObj);
	}

	cm_RunRemoveHookQ474((CClientMgrQ474*)pClientMgr, (CClientHookObjQ474*)pObj);

	if (pObj->m_nServerID != 0xffff)
		tbl_ReleaseSlotQ5(pClientMgr, pObj->m_nServerID);

	if (pObj->m_ObjectType == 1)
		cm_ReleaseObjectTexturesQ404((CClientMgr*)pClientMgr, (CClientObject*)pObj);

	om_RemoveObjectQ5(pClientMgr->m_ObjectMgr1ec, pObj);
	g_dwDirtyFlags4b44a4 |= 2;
	return LT_OK;
}

// 0x00411cd0 — empty the 0x1264 chain.
//
// Not an LTList: the chain is followed through +0x04 and ends at null, and
// the next pointer is read *before* the element is freed.  The tail is the
// giveaway for how it was written — the loop leaves %esi holding the null it
// stopped on and stores that, while the not-taken path stores a literal
// zero from a separate basic block.  That is one `m_pChain1264 = LTNULL`
// after the loop in source, with VC6 reusing the register on the path where
// it can prove the value.
void cm_ClearChain1264(CClientMgrView* pClientMgr)
{
	CChain1264* pCur = pClientMgr->m_pChain1264;

	while (pCur)
	{
		CChain1264* pNext = *(CChain1264**)((uint8*)pCur + 4);
		sub_429080(pCur);
		pCur = pNext;
	}

	pClientMgr->m_pChain1264 = LTNULL;
}

// 0x004115c0 — the manager's half of the shell teardown.
//
// Guarded by identity: it does nothing unless the shell handed in is the one
// the manager still points at, which is what makes it safe to call from the
// shell's own destructor path (0x0041335a, inside CClientShell::Term).
// The fifteen -1s are the same `rep stos` CClientMgr::Term runs at
// 0x00410f42, with the same count and the same address, so the array's
// length is proven twice.
void cm_OnShellTerm(CClientMgrView* pClientMgr, CClientShell* pShell)
{
	if (pShell != ((CClientMgrTail*)pClientMgr)->m_pClientShell2)
		return;

	// memset, NOT a counted loop. The argument registers settle it:
	// `ecx, eax, edi` is memset's right-to-left order, and shipped
	// CClientMgr::Term at 0x00410f42 has the same order, while our own
	// for-loop copy in clientmgr3.cpp's ~CCmgClientMgr emits dest-first.
	memset(pClientMgr->m_Handles1160, 0xff,
	       sizeof(pClientMgr->m_Handles1160));

	sub_404480(((CClientMgrTail*)pClientMgr)->m_pFileMgr);
	sub_41b200(pClientMgr->m_Sub1268);
}

// AVP2: Primal Hunt lithtech.exe 0x00413760 (210 bytes; AvP2 1.0.9.6's is
// 0x00412110) -- cmgrinit.cpp's cm_RunPlusCommandsQ46C grown a third
// argument: a command line of its own to run the `+command`s of, the
// process's (0x004f7c60 / 0x004f7c64, clientwin.cpp's pair) only when it is
// null. Called twice from CClientMgr::Init (0x00410d18, 0x00410d5e); the
// first argument is still never read. The list is two words, the vector
// and then its count (`mov 0x4(%edi),%eax` is the bound, `mov (%edi),%eax`
// the vector); names ours.
//
// RETAIL BUG: the list form runs `i` up to the count itself, where the
// process form stops one short (`dec %eax` at 0x004137d4), so a `+name`
// as its LAST word reads the vector one past its end for the value
// (0x00413792).
class CCmdLineArgsQ546
{
public:
	char** m_ppArgs;                         // +0x00
	int    m_nArgs;                          // +0x04
};

extern char** g_pCluArgList;                 // PH 0x004f7c64
extern int    g_nCluArgCount;                // PH 0x004f7c60
void con_RunString3(ConsoleStateRef* pState, char* pString, int nSource);  // PH 0x00421c00

void cm_RunPlusCommandsQ546(CClientMgrView* /*pClientMgr*/, int nSource,
                            CCmdLineArgsQ546* pArgs)
{
	char buf[512];
	int  i;

	if (pArgs)
	{
#ifdef AVP2_MODERN
		for (i = 0; i < pArgs->m_nArgs - 1; i++)
#else
		for (i = 0; i < pArgs->m_nArgs; i++)
#endif
		{
			if (pArgs->m_ppArgs[i][0] == '+')
			{
				sprintf(buf, "(%s) (%s)", pArgs->m_ppArgs[i] + 1,
				        pArgs->m_ppArgs[i + 1]);

				con_RunString3(&g_ClientConsoleStateRef, buf, nSource);
			}
		}
	}
	else
	{
		for (i = 0; i < g_nCluArgCount - 1; i++)
		{
			if (g_pCluArgList[i][0] == '+')
			{
				sprintf(buf, "(%s) (%s)", g_pCluArgList[i] + 1,
				        g_pCluArgList[i + 1]);

				con_RunString3(&g_ClientConsoleStateRef, buf, nSource);
			}
		}
	}
}

// 0x00410610 — the console side of something, with the stage fixed at 0.
//
// It is a MEMBER that ignores `this`, not a free function: the call site at
// 0x00462854 (clientrender.cpp's r_TermRender) does `mov %esi,%ecx`
// immediately before it.  The bytes are the same either way -- an unread
// `this` costs nothing -- so it is the member here, the name that caller
// links against.  The literal is the whole of it.  Its counterpart is
// called with 1 from CClientMgr::Term (0x00410f92), which is what says the
// argument is a stage and not a flag.
void CClientMgrView::Sub410610()
{
	con_Sub41b820(0);
}

// 0x00414920 — a dword out of the manager, reached through the global.
// The field is zeroed by the constructor at 0x00410ce5 and read by exactly
// one caller (0x004746aa); nothing in this run writes it, so it keeps its
// offset for a name.
uint32 cm_GetUnknown11d8()
{
	return g_pClientMgr->m_Unknown11d8;
}


// ======================================================================= //
//  CClientShell
// ======================================================================= //

// 0x00413310 — take up a manager.
//
// Three pairings in five stores, and each names its target: the manager goes
// into the shell, the message-dispatch table is filled (0x0041331a calls
// 0x00477fe0, and m_pClientMgr is reloaded after it -- so the store precedes
// the call in source), the shell goes into the manager's CNetMgr at +0xb4 (0x5c0
// absolute, inside the 0x104-byte sub-object at 0x50c), and the shell
// publishes itself into the global the three free functions at the foot of
// this file reach it through.  Returns LTTRUE unconditionally — the shipped
// `mov $0x1,%eax` is not on a branch.
LTBOOL CClientShell::Init(CClientMgrView* pClientMgr)
{
	m_pClientMgr = pClientMgr;

	cm_InitTable4b48d0Q474();

	m_pClientMgr->m_NetMgr.m_pClientShell = this;
	g_pClientShell = this;

	m_pUnknown8c = sub_42cb20(0x64, 0);

	return LTTRUE;
}

// 0x00413430 — store and report LT_OK.  The zero comes out of an `xor`
// *after* the store, so it is the return value and not the value stored.
LTRESULT CClientShell::SetUnknown30(uint32 nValue)
{
	m_Unknown30 = nValue;

	return LT_OK;
}

// 0x00413cd0 — send a message to the server.
//
// A thiscall on `m_pClientMgr + 0x50c`, which is the manager's CNetMgr
// sub-object, with the shell's own connection id and a fixed 0x80.  The
// function has no return of its own: it falls straight out of the call, so
// whatever the net manager answers is the answer.
LTRESULT CClientShell::SendToServer(void* pMsg)
{
	return m_pClientMgr->m_NetMgr.SendTo(pMsg, m_nConnID, 0x80);
}

// 0x00413cf0 — walk the manager's eleven records and reset each.
//
// The records are sixteen-byte LTLists (0x10 and 0x30 are walked as lists
// in DoLoadWorld), and the loop is an index over them: VC6 strength-reduces
// `i < 11` into the byte offset it shows (`add $0x10,%esi` against
// `cmp $0xb0,%esi`, signed `jl`).  Indexing an LTList array is also what gives
// `lea 0x40c(%esi,%eax,1)`; a byte offset added to m_Records40c, in any of
// three spellings, swapped base and index.  The same loop is open-coded in
// CClientMgr::Term (0x00410f03) and DoLoadWorld (0x00414165).
void CClientShell::ResetRecords()
{
	int i;

	for (i = 0; i < 11; i++)
		sub_411c80(m_pClientMgr, &((LTList*)m_pClientMgr->m_Records40c)[i], 1);
}

// 0x004143d0 — bring the world down.
//
// The manager is loaded into %ecx and the shell pushed as the argument, so
// the notification is a CClientMgr method taking the shell — the mirror of
// DoLoadWorld's last two instructions, which call the enter-world half and
// then set this same flag.
void CClientShell::UnloadWorld()
{
	if (m_bWorldLoaded)
		m_pClientMgr->OnExitWorld(this);

	m_bWorldLoaded = LTFALSE;
}

// 0x004143b0 — the whole world teardown, in the order the bytes give it:
// tell the game DLL, reset the records, then whatever 0x004143f0 owns.
// cdecl, one argument.
void cs_TermWorld(CClientShell* pShell)
{
	pShell->UnloadWorld();
	pShell->ResetRecords();
	pShell->sub_4143f0();
}

// 0x00414860 — make sure the renderer has a world object.
//
// Three exits and only one of them is false, which is the shape worth
// getting right: no render DLL is *success* (there is nothing to make), an
// object already there is success, and only a maker that comes back null
// fails.  The request struct is a single field on the stack — the shipped
// code reserves four bytes with `push %ecx`, stores the manager into them
// and passes their address — so the render entry takes a pointer to a
// struct, not the manager itself.
LTBOOL CClientShell::CreateRenderWorld()
{
	// The manager goes in a LOCAL. Writing `m_pClientMgr->m_pRenderWorld` at
	// each mention parks `this` in %esi and reloads the member three times;
	// the shipped code parks the MEMBER (`mov 0x78(%ecx),%esi`), which only a
	// local expresses.
	RenderWorldRequest request;

	// The two guards are NESTED, not joined with `&&`: the shipped code tests
	// g_bRenderInited and only then loads the manager (`mov 0x78(%ecx),%esi`),
	// where one combined condition hoists the load above the test. The local
	// is block-scoped for the same reason.
	//
	// One positive condition wrapping the body, so the LTTRUE answer is
	// hosted once at the END and all three arms reach it.
	if (g_bRenderInited)
	{
		CClientMgrView* pClientMgr = m_pClientMgr;

		if (!pClientMgr->m_pRenderWorld)
		{
			request.m_pClientMgr = pClientMgr;
			pClientMgr->m_pRenderWorld = g_pRenderCreateWorld(&request);

			if (!pClientMgr->m_pRenderWorld)
				return LTFALSE;
		}
	}

	return LTTRUE;
}

// 0x004148a0 — and give it back.  No render-DLL check here, only the null
// check: the field is the whole state.
void CClientShell::DestroyRenderWorld()
{
	// Same lever as above: the member is read into a local once.
	CClientMgrView* pClientMgr = m_pClientMgr;

	if (pClientMgr->m_pRenderWorld)
	{
		g_pRenderDestroyWorld(pClientMgr->m_pRenderWorld);
		pClientMgr->m_pRenderWorld = LTNULL;
	}
}

// 0x00414b10 — four bytes: the back pointer, straight out.  Six call sites
// across the image, which is what earns it a function of its own.
CClientMgrView* CClientShell::GetClientMgr()
{
	return m_pClientMgr;
}

// 0x00414370 — hand every element of a list to the world.
//
// Both arguments are read off the stack with no push in front, so it is
// cdecl with two, and the first is only used to reach
// `m_pClientMgr + 0x6c` — clientmgr.h's m_World, taken as an address once
// before the loop.  The second argument of the world call is a literal 0 at
// this site; the call itself is a thiscall (AddObject, 0x0048cde0).
//
// NEAR (size-exact, 2 blocks): shipped loads pList into %ecx and pShell
// into %eax; we get them the other way round.  Pure allocation -- eleven
// spellings (local order, named head, for/while/do, a pMgr local, a pObj or
// pNext temporary, the m_World member vs a cast) either leave these two
// blocks or make it worse.
void cs_ReleaseListToWorld(CClientShell* pShell, LTList* pList)
{
	LTLink*        pCur   = pList->m_Head.m_pNext;
	CWorldTreeQ2C* pWorld = &pShell->m_pClientMgr->m_World;

	while (pCur != &pList->m_Head)
	{
		pWorld->AddObject(pCur->m_pData, 0);
		pCur = pCur->m_pNext;
	}
}

// 0x004144b0 — the same walk, releasing each element through its own vtable
// slot 2 instead.  The shell argument is loaded and never used, which the
// `mov 0x8(%esp),%eax` on entry makes plain: the function takes two and
// touches one, so the pair below is one signature used twice.  The head is
// named as a local and the walk is a for: with `while` over
// `&pList->m_Head` VC6 took the vtable into %eax instead of %edx.
void cs_ReleaseListItems(CClientShell* /*pShell*/, LTList* pList)
{
	LTLink* pHead = &pList->m_Head;
	LTLink* pCur;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
		((CListItemView*)pCur->m_pData)->Release();
}

// 0x004143f0 -- the rest of cs_TermWorld (0x004143b0): release every
// record's items, run each record's objects through the two 0x00450b50 /
// 0x00450b90 unlinks, flush records 2 and 9, drop the render world, empty
// the 0x1264 chain, stop the sounds and term the world.  The views and
// helper names below are stand-ins (ours): 0x00480ca0 is clientsoundmgr.cpp's
// StopAllSounds on the sound manager at CClientMgr+0x774 (thiscall), and
// 0x00424bc0 is the world Term smgr.cpp shares, reached with %ecx = the
// manager itself.
class CSoundMgrQ4143f0
{
public:
	LTRESULT StopAllSounds();                                        // 0x00480ca0
};

class CWorldQ4143f0
{
public:
	void Term();                                                     // 0x00424bc0
};

void sub_450b50(void* pObj);                                         // 0x00450b50
void sub_450b90(void* pObj);                                         // 0x00450b90

void CClientShell::sub_4143f0()
{
	int     i;
	LTLink* pHead;
	LTLink* pCur;
	void*   pObj;

	for (i = 0; i < 11; i++)
		cs_ReleaseListItems(this, &((LTList*)m_pClientMgr->m_Records40c)[i]);

	for (i = 0; i < 11; i++)
	{
		pHead = &((LTList*)m_pClientMgr->m_Records40c)[i].m_Head;

		for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
		{
			pObj = pCur->m_pData;
			sub_450b50(pObj);
			sub_450b90(pObj);
		}
	}

	sub_411c80(m_pClientMgr, &((LTList*)m_pClientMgr->m_Records40c)[2], 0);
	sub_411c80(m_pClientMgr, &((LTList*)m_pClientMgr->m_Records40c)[9], 0);

	DestroyRenderWorld();
	cm_ClearChain1264(m_pClientMgr);
	((CSoundMgrQ4143f0*)((uint8*)m_pClientMgr + 0x784))->StopAllSounds();
	((CWorldQ4143f0*)m_pClientMgr)->Term();
}

// 0x00414200 — the same idea over an array rather than a list.
//
// The count at +0x190 is re-read every iteration (the shipped loop reloads
// it after the call), so the callee is allowed to change it; the elements
// are pointers out of the array at +0x18c and what is passed on is each
// element's +0x04.  cdecl, two arguments, and the comparison is `jbe`/`jb`
// — unsigned, so the count is a uint32.
class CArray18C
{
public:
	uint8   m_pad_00[0x18c];
	void**  m_ppItems;                            // 0x18c
	uint32  m_nItems;                             // 0x190
};

//
// The first argument is the client manager, not the shell: it is only passed
// on, and 0x00414240 below reads the manager's +0x1264 chain through it
// (startload.cpp notes DoLoadWorld's call site pushes the manager for both).
void cs_ReleaseArrayItems(CClientMgrView* pClientMgr, CArray18C* pArray)
{
	uint32 i;

	for (i = 0; i < pArray->m_nItems; i++)
		sub_414240(pClientMgr, *(CWtWorldView**)((uint8*)pArray->m_ppItems[i] + 4));
}

// 0x00414240 -- bind every surface of one world to its texture.  (Name ours.)
//
// cdecl, (manager, world).  For each 0x40-byte surface of the world
// (+0x78 array, +0x7c count, re-read every pass): name its texture out of
// the world's name table (+0xbc, indexed by the surface's uint16 at +0x36)
// through one CClientFileRef built before the loop.  A surface flagged
// 0x4000 is animated: load the sprite (0x00476e80), and on LT_OK chain a
// 0x20-byte record onto the manager's +0x1264 list (0x00429050, no null
// test) holding the surface, the sprite and a sprite tracker started on it
// (0x00483cc0); the tracker's current frame supplies the texture.  Any other
// surface resolves the name (0x0041ff90), complains at debug level >= 1 when
// that fails, and falls back to "textures\default_texture.dtx".
class CWtFileRef                              // clientmgr.h's CClientFileRef
{
public:
	CWtFileRef() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// AVP2: the surface is 0x40 bytes, 8 fewer than NOLF's, and its fields sit
// 4 lower (texture 0x2c, flags 0x30, texture index 0x36: `add $0x40,%edi`
// at 0x004180ba, AvP2 1.0.9.6's 0x00416036/0x00416045 agree).
class CWtSurfaceView
{
public:
	uint8  m_pad_00[0x2c];
	void*  m_pTexture;                       // 0x2c
	uint32 m_Flags;                          // 0x30
	uint8  m_pad_34[0x36 - 0x34];
	uint16 m_iTexture;                       // 0x36
	uint8  m_pad_38[0x40 - 0x38];
};

class CWtWorldView
{
public:
	uint8           m_pad_00[0x78];
	CWtSurfaceView* m_Surfaces;              // 0x78
	uint32          m_nSurfaces;             // 0x7c
	uint8           m_pad_80[0xbc - 0x80];
	char**          m_TextureNames;          // 0xbc
};

struct CWtSpriteTracker                       // csvis.cpp's CSpriteTrackerQ414
{
	void*  m_pSprite;                        // 0x00
	void*  m_pAnim;                          // 0x04
	void** m_pCurFrame;                      // 0x08  -> the frame's texture
	uint32 m_MSCurTime;                      // 0x0c
	uint32 m_Flags;                          // 0x10
};

// The records cm_ClearChain1264 frees.
class CChain1264
{
public:
	CWtSurfaceView*  m_pSurface;             // 0x00
	CChain1264*      m_pNext;                // 0x04
	void*            m_pSprite;              // 0x08
	CWtSpriteTracker m_Tracker;              // 0x0c
};

extern int g_DebugLevelQ14;                                         // 0x004b0ab0
void     con_ConsolePrintQ14(char* pFormat, ...);                   // 0x0042e6f0
void*    sub_429050(unsigned long nSize);                           // 0x00429050
// The three callees under their definitions' names and types
// (cmodelload.cpp, csvis.cpp, lttexmod.cpp); the arguments are cast.
class CClientMgrQ474;
class CFileIdentQ474;
class CSpriteQ474;
class CSpriteTrackerQ414;
class CSpriteQ414;
class SharedTextureView;
class CClientMgrTexBankView;
class CClientFileRefView;
LTRESULT cm_LoadSpriteQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pRef,
                           CSpriteQ474** ppSprite);                 // 0x00476e80
void     spr_InitTrackerQ414(CSpriteTrackerQ414* pTracker,
                             CSpriteQ414* pSprite);                 // 0x00483cc0
SharedTextureView* cm_AddSharedTexture(CClientMgrTexBankView* pMgr,
                                       CClientFileRefView* pRef);   // 0x0041ff90
#define sub_476e80(pMgr, pRef, ppSprite) \
	cm_LoadSpriteQ474((CClientMgrQ474*)(pMgr), (CFileIdentQ474*)(pRef), (CSpriteQ474**)(ppSprite))
#define sub_483cc0(pTracker, pSprite) \
	spr_InitTrackerQ414((CSpriteTrackerQ414*)(pTracker), (CSpriteQ414*)(pSprite))
#define sub_41ff90(pMgr, pRef) \
	((void*)cm_AddSharedTexture((CClientMgrTexBankView*)(pMgr), (CClientFileRefView*)(pRef)))

// NEAR (0x00418000, size-exact): the layout is now AvP2's; what is left
// is volatile-register choice at the loop head (shipped loads m_Surfaces
// and m_TextureNames both through %ecx with the `xor %eax` between them).
// Assigning m_nFileType after the name, or indexing through pSurface (303
// bytes), does not move it.
void sub_414240(CClientMgrView* pClientMgr, CWtWorldView* pWorld)
{
	CWtFileRef ref;
	uint32 i;

	for (i = 0; i < pWorld->m_nSurfaces; i++)
	{
		CWtSurfaceView* pSurface = &pWorld->m_Surfaces[i];

		// The name's index is read through pWorld->m_Surfaces[i], not
		// pSurface: shipped re-derives that element's address from its own
		// i*0x48 slot (frame 0x14, `mov 0x78(%ebp),%ecx; add %ecx,%edi` at
		// 0x00414273), where spelling it through pSurface packed the offset
		// into pWorld's argument slot (frame 0x10, 303 bytes, unpaired).
		ref.m_nFileType = 0;
		ref.m_pFilename = pWorld->m_TextureNames[pWorld->m_Surfaces[i].m_iTexture];

		if (pSurface->m_Flags & 0x4000)
		{
			void* pSprite;
			if (sub_476e80(pClientMgr, &ref, &pSprite) == LT_OK)
			{
				CChain1264* pRec = (CChain1264*)sub_429050(sizeof(CChain1264));
				pRec->m_pNext = pClientMgr->m_pChain1264;
				pClientMgr->m_pChain1264 = pRec;
				pRec->m_pSurface = pSurface;
				pRec->m_pSprite = pSprite;
				sub_483cc0(&pRec->m_Tracker, pSprite);

				if (pRec->m_Tracker.m_pCurFrame)
					pSurface->m_pTexture = *pRec->m_Tracker.m_pCurFrame;
			}
		}
		else
		{
			pSurface->m_pTexture = sub_41ff90(pClientMgr, &ref);
			if (!pSurface->m_pTexture && g_DebugLevelQ14 >= 1)
				con_ConsolePrintQ14("Unable to find world texture %s", ref.m_pFilename);

			if (!pSurface->m_pTexture)
			{
				ref.m_nFileType = 1;
				ref.m_pFilename = "textures\\default_texture.dtx";
				pSurface->m_pTexture = sub_41ff90(pClientMgr, &ref);
			}
		}
	}
}

// 0x004148d0 — tell the game DLL a world is about to load.
//
// A free function, not a method: nothing reads %ecx.  Two null checks, the
// displacement is 0x1c — slot 7, PreLoadWorld(char*) — and the argument goes
// straight through, so it is the world name.  The manager is named as a
// local inside the outer test: written as one `&&` chain through the global,
// VC6 took the argument into %eax and the vtable into %edx, the reverse of
// shipped.
void cs_PreLoadWorld(char* pWorldName)
{
	if (g_pClientShell)
	{
		CClientMgrView* pMgr = g_pClientShell->m_pClientMgr;

		if (pMgr->m_pClientShell)
			pMgr->m_pClientShell->PreLoadWorld(pWorldName);
	}
}

// 0x00414900 — invalidate and tear down, through the global.
// The 0xffff is a *word* store (`movw`), which is what sizes the field.
void cs_InvalidateAndTerm()
{
	if (g_pClientShell)
	{
		g_pClientShell->m_Unknown6c = 0xffff;
		cs_TermWorld(g_pClientShell);
	}
}

// 0x004149e0 — reach the manager through the global and hand it on.  cdecl;
// both call sites (smgr.cpp's load hook and CacheFile) push one dword it
// never reads. AvP2 1.0.9.6's cl 9044 turns the call into a jump and reuses
// that argument's slot (0x0041680c `mov %eax,0x4(%esp)`), so it is declared.
void cs_Sub4149e0(int /*nUnused*/)
{
	if (g_pClientShell)
		sub_490200((CClientMgrTexBankView*)g_pClientShell->m_pClientMgr);
}


// ======================================================================= //
//  The client side of the shared base (vtable 0x004a1b00; 0x004157f0 ..;
//  Primal Hunt 0x004196a0 .. 0x00419b40): clientcsbase.cpp, its own
//  translation unit.
// ======================================================================= //


// ======================================================================= //
//  The client's message object helper — 0x00415580 / 0x004155c0
// ======================================================================= //

// The 8-byte object with the four-slot table at 0x004a1dcc: vptr, then the
// manager at +0x04 (0x00415560 is its constructor -- stores the table and
// its one argument, and clientmgr3.cpp news it up for CClientMgr+0x4bc).
// It is the client twin of server.dll's CMessageObjectHelper
// (src/engine/server/servermgr.h, bodies in servermessage.cpp): LMessageImpl
// hands every ReadObjectFL / WriteObject to one of these, and the slots line
// up -- 0 ReadObjectID (0x00415580) and 1 WriteObjectID, both below, then
// 2 a three-argument slot (0x004155f0) and 3 a one-argument getter
// (0x0043ec90), which are not written here and so not declared.  (class name ours)
// CClientMsgObjectHelper (0x00415560 .. 0x004155e0; Primal Hunt 0x00419320
// ..): clientmsghelper.cpp, its own translation unit with the slot-2
// WriteFormattedString.
class CClientMsgObjectHelper;


// ======================================================================= //
//  The per-frame object updates — 0x00415bb0
// ======================================================================= //

// Home: csvis.cpp, which writes the callees around it (0x00415b50 and
// 0x00415ec0; the other four are unwritten).  Written here rather than in
// csvis.cpp.  A thiscall with no arguments, called by
// clientmgr3.cpp between the client shell's Update and PostUpdate: zero the
// dword at 0x004b1800 (written nowhere else in the image), then hand the
// manager to six cdecl walks in turn.  The shipped `push %esi` of the first
// argument is scheduled above the store; the six pushes are popped by one
// `add $0x18`.  (Names ours.)
// Defined here, beside its one writer (written nowhere else in the image;
// it follows clientshell.cpp's .bss run 0x004b17e4..0x004b17fc).
uint32 g_Unknown4b1800;                                            // 0x004b1800

// A duplicate: clientupdate.cpp's CClientMgrQ414::RunFrameUpdates is the
// one both images place (Primal Hunt 0x00419fa0, AvP2 0x00417de0, the
// unit retail has on its own); this view's callers reach it through
// link/aliases.rsp.
#if 0
void cs_UpdateSprites(CClientMgrView* pMgr);                       // 0x00415b50
void cs_UpdateAnimations(CClientMgrView* pMgr);                    // 0x00415d00
void cs_Sub415bf0(CClientMgrView* pMgr);                           // 0x00415bf0
void cs_Sub415e40(CClientMgrView* pMgr);                           // 0x00415e40
void cs_ApplyPendingMoves(CClientMgrView* pMgr);                   // 0x00415ec0
void cs_Sub415f10(CClientMgrView* pMgr);                           // 0x00415f10

void CClientMgrView::RunFrameUpdates()
{
	g_Unknown4b1800 = 0;

	cs_UpdateSprites(this);
	cs_UpdateAnimations(this);
	cs_Sub415bf0(this);
	cs_Sub415e40(this);
	cs_ApplyPendingMoves(this);
	cs_Sub415f10(this);
}
#endif

// 0x00415d00 -- the second walk: advance every animation.  (Names ours.)
//
// Home: csvis.cpp with the other five walks; written here for the same
// reason as RunFrameUpdates above.  The frame time in milliseconds, signed
// (`jle`).  The two lists are CClientMgr's per-type object lists -- the
// eleven 16-byte LTLists at +0x40c are indexed by object type, so +0x41c
// is OT_MODEL's and +0x43c is OT_SPRITE's.  Every model's tracker chain
// (+0x220, PH +0x224; linked through +0x04) gets the key callback 0x00415da0 at +0x0c
// and is advanced by 0x004010e0; every sprite's tracker at +0x1b0 goes to
// 0x00483d00.  Shipped re-derives the model list's end marker from the
// manager each pass (`mov 0x14(%esp),%ecx; lea 0x420(%ecx)`), so that walk
// has no named head; the sprite walk keeps its head in %ebx.
struct CAnimKeyFrameView
{
	uint32 m_Time;                           // 0x00
	char*  m_pString;                        // 0x04
};

class CAnimTrackerView;
// AVP2: ltanimtracker.h's StringKeyCallback, with the append string and
// the force-replace flag.
typedef void (*AnimKeyCallbackFn)(CAnimTrackerView* pTracker, CAnimKeyFrameView* pKey,
                                  char* szAppend, LTBOOL bForceReplace);

class CAnimTrackerView
{
public:
	uint8             m_pad_00[0x04];
	CAnimTrackerView* m_pNext;               // 0x04
	class CKeyObjectView* m_pObject;         // 0x08
	AnimKeyCallbackFn m_KeyCallback;         // 0x0c
	// AVP2: LTAnimTracker::m_Index, the tracker's number, after the 0x24-byte
	// m_TimeRef at +0x28 in AvP2's SDK (ltanimtracker.h)
	uint8             m_pad_10[0x4c - 0x10];
	int               m_Index;               // 0x4c
};

class CKeyObjectView
{
public:
	// AVP2: the flags are 8 higher than NOLF's (`testb $2,0x188(%esi)`,
	// 0x0041a21a; AvP2 1.0.9.6's 0x0041802a too).
	uint8  m_pad_000[0x188];
	uint8  m_Flags;                          // 0x188  bit 1: FLAG_MODELKEYS
};

// AVP2: the model's tracker chain is at +0x220 (AvP2 1.0.9.6's 0x00417f97);
// PH: +0x224, behind the model's extra texture (0x0041a187).  The sprite's
// tracker is +0x1b0 in both (0x0041a1cd, AvP2 0x00417fdd).
class CAnimModelView
{
public:
#ifdef PRIMAL_HUNT
	uint8             m_pad_000[0x224];
#else
	uint8             m_pad_000[0x220];
#endif
	CAnimTrackerView* m_pTrackers;           // PH 0x224, AvP2 0x220
};

class CAnimSpriteView
{
public:
	uint8  m_pad_000[0x1b0];
	uint8  m_SpriteTracker[0x14];            // 0x1b0
};

void trk_Update(CAnimTrackerView* pTracker, int msDelta);           // 0x004010e0
void spr_UpdateTrackerQ14(void* pTracker, int msDelta);             // 0x00483d00
void cs_ModelKeyCallback(CAnimTrackerView* pTracker,
                         CAnimKeyFrameView* pKey, char* szAppend,
                         LTBOOL bForceReplace);                     // 0x00415da0

void cs_UpdateAnimations(CClientMgrView* pMgr)
{
	LTLink* pCur;
	LTLink* pListHead;
	CAnimTrackerView* pTracker;
	int msDelta;

#ifdef AVP2_MODERN
	// M-P1-23: the fraction of a millisecond carried to the next frame
	static float s_fP1Carry;
	msDelta = (int)P1_FrameMs(pMgr->m_fFrameTime, &s_fP1Carry);
#else
	msDelta = (int)(pMgr->m_fFrameTime * 1000.0f);
#endif

	if (msDelta > 0)
	{
		for (pCur = ((LTList*)&pMgr->m_Records40c[0x10])->m_Head.m_pNext;
		     pCur != &((LTList*)&pMgr->m_Records40c[0x10])->m_Head;
		     pCur = pCur->m_pNext)
		{
			for (pTracker = ((CAnimModelView*)pCur->m_pData)->m_pTrackers;
			     pTracker; pTracker = pTracker->m_pNext)
			{
				pTracker->m_KeyCallback = cs_ModelKeyCallback;
				trk_Update(pTracker, msDelta);
			}
		}

		pListHead = &((LTList*)&pMgr->m_Records40c[0x30])->m_Head;
		for (pCur = pListHead->m_pNext; pCur != pListHead; pCur = pCur->m_pNext)
			spr_UpdateTrackerQ14(((CAnimSpriteView*)pCur->m_pData)->m_SpriteTracker, msDelta);
	}
}

// 0x00415da0 -- the model key callback cs_UpdateAnimations installs.
// (Name ours.)  Split off 0x00415d00's csv row, which ran over it.
//
// A 0x1510-byte frame (__chkstk): an ArgList and the engine's
// CommandParser (servermgr.h, 0x1508 bytes).  If the tracker's object asks
// for model keys and the game has a client shell, tokenise the key's string
// and hand each non-empty command to IClientShell::OnModelKey (slot 6).
// Source order: the cursor, then argv (the other order swaps the
// registers of the pKey load and the parser lea at 0x00415dde).
class CKeyParserView                          // servermgr.h's CommandParser
{
public:
	int Next();                              // 0x0041b590

	char* m_ArgPointers[64];                 // 0x0000
	int   m_nArgs;                           // 0x0100
	char* m_pPos;                            // 0x0104
	char  m_ArgBuffer[0x1400];               // 0x0108
};

extern CClientMgrView* g_pClientMgrQ14;                             // 0x004b17e0

//
// AVP2 (PH 0x0041a1f0, 0x11b bytes; AvP2 1.0.9.6's 0x00418000): the SDK
// callback's two extra arguments.  With an append string the parser runs
// over a 128-byte copy -- "%s; %s" of the key's string and the append when
// the key has one and replacing is not forced, else "%s" of the append
// alone (0x0041a23b..0x0041a2ab).
void cs_ModelKeyCallback(CAnimTrackerView* pTracker, CAnimKeyFrameView* pKey,
                         char* szAppend, LTBOOL bForceReplace)
{
	ArgList        args;
	char           szBuffer[128];
	CKeyParserView parser;
	CKeyObjectView* pObj;
	CClientMgrView* pMgr;

	parser.m_pPos = LTNULL;

	pObj = pTracker->m_pObject;
	if (!pObj || !(pObj->m_Flags & 2))
		return;

	pMgr = g_pClientMgrQ14;
	if (!pMgr->m_pClientShell)
		return;

	if (szAppend)
	{
		if (pKey->m_pString[0] && !bForceReplace)
			sprintf(szBuffer, "%s; %s", pKey->m_pString, szAppend);
		else
			sprintf(szBuffer, "%s", szAppend);

		parser.m_pPos = szBuffer;
	}
	else
	{
		parser.m_pPos = pKey->m_pString;
	}

	args.argv = parser.m_ArgPointers;

	while (parser.Next())
	{
		if (parser.m_nArgs > 0)
		{
			args.argc = parser.m_nArgs;
			// AVP2: iclientshell.h's OnModelKey takes the tracker's number
			// too (LTAnimTracker::m_Index); provisional, until the retail
			// code shows what is passed
			pMgr->m_pClientShell->OnModelKey((HLOCALOBJ)pObj, &args, pTracker->m_Index);
		}
	}
}


// ======================================================================= //
//  The shared three-argument no-op — 0x004132e0
// ======================================================================= //

// Three bytes, `ret $0xc`, and it fills THREE vtable slots in two different
// class hierarchies:
//
//   * 0x004a1d3c — slot 4 of the five-slot table at 0x004a1d2c, the base
//     CClientShell derives from.  Its other three slots are 0x004928e7,
//     the pure-virtual trap, so this is the one virtual in that base with a
//     body, and CClientShell overrides it (0x004a1d18 slot 4 is 0x00477fb0).
//   * 0x004a1d64 — slot 9 of the forty-eight-slot table at 0x004a1d40,
//     CClientShell's second base.
//   * 0x004a2778 — slot 56 of a third table this run does not otherwise
//     reach.
//
// Three unrelated do-nothing three-argument virtuals, folded to one body.
// Only the two CClientShell bases are named here; the third slot's class is
// outside this run.
class CShellBase00
{
public:
	// Slot 0 is the destructor (0x004132f0 in the base's own table,
	// 0x004132a0 in CClientShell's); it is left out here rather than
	// declared, because a view cannot supply one without also supplying a
	// body, and this file only owns slot 4.
	virtual void _p00()=0;                                     // slot 0
	virtual void _p04()=0;                                     // slot 1  0x004928e7
	virtual void _p08()=0;                                     // slot 2  0x004928e7
	virtual void _p0c()=0;                                     // slot 3  0x004928e7
	virtual void OnUnknown10(void* p1, void* p2, void* p3);    // slot 4
};

void CShellBase00::OnUnknown10(void* /*p1*/, void* /*p2*/, void* /*p3*/) {}


// ======================================================================= //
//  The render DLL going away — 0x0040f110
// ======================================================================= //

// Every surface has to come out of the render DLL's memory before the DLL
// does, and this is the pass that does it.  Its only caller is 0x0040f060,
// which copies thirteen dwords of the render struct into a shadow copy, runs
// this, and then sets g_pRender to null — so the order is: shadow, save the
// surfaces, drop the renderer.
//
// The walk is the piece worth reading carefully.  The list is circular, so
// the next pointer is compared against the head to spot the wrap:
//
//     mov  0x4b17a0,%edx        ; the head, re-read every iteration
//     mov  0x4(%esi),%esi       ; pCur->m_pNext
//     sub  %edx,%ecx / neg / sbb / and
//
// which is `pNext = (pNext == g_pSurfaceList) ? NULL : pNext` — a ternary
// compiled without a branch.
// The head is re-read inside the loop rather than cached, so the copy-out is
// allowed to change it.  The next pointer is taken *before* the element is
// worked on, and the loop bails on the first failure with LTFALSE.
// Spelled as an `if (pNext == head) pNext = NULL;` it compiles to a
// cmp/jne/xor branch (5 blocks); only the ternary gives the neg/sbb/and.
LTBOOL surface_SaveAll()
{
	CSurfaceView* pCur = g_pSurfaceList;

	while (pCur)
	{
		CSurfaceView* pSurface = pCur;
		pCur = (pCur->m_pNext == g_pSurfaceList) ? LTNULL : pCur->m_pNext;

		if (!surface_CopyOut(pSurface))
			return LTFALSE;
	}

	return LTTRUE;
}

// 0x0040eef0 -- the other direction: put every surface back into the new
// render DLL.  REAL HOME: src/engine/lithtech/cissurf.cpp, beside its callee
// surface_CopyInQ40C (0x0040ef30), which is why it is not in this file's
// address range.  It is written here because it has the SAME 60 bytes as
// surface_SaveAll apart from the callee (masked as a relocation), and
// while it had no source surface_SaveAll was credited to
// 0x0040eef0 -- the first hit -- and left 0x0040f110 uncovered.  With both
// written, each claims its own address.  Move it to cissurf.cpp (calling
// surface_CopyInQ40C) when that file is next open.
class CSurfaceQ40C;
LTBOOL surface_CopyInQ40C(CSurfaceQ40C* pSurface);                  // 0x0040ef30

LTBOOL surface_RestoreAll()
{
	CSurfaceView* pCur = g_pSurfaceList;

	while (pCur)
	{
		CSurfaceView* pSurface = pCur;
		pCur = (pCur->m_pNext == g_pSurfaceList) ? LTNULL : pCur->m_pNext;

		if (!surface_CopyInQ40C((CSurfaceQ40C*)pSurface))
			return LTFALSE;
	}

	return LTTRUE;
}


// ======================================================================= //
//  surface_CopyOut -- 0x0040f150
// ======================================================================= //

// One surface out of the render DLL into system memory.  The exact mirror
// of cissurf.cpp's surface_CopyInQ40C (0x0040ef30), and like surface_SaveAll
// its real home is that TU; it is written here next to its one caller.
//
//   0x0040f155  width * g_nBytesPerPixel * height -> 0x00428fd0 (dalloc),
//               the answer stored in m_pSysMem with NO null test;
//   0x0040f176  g_pRender+0xdc LockSurface(m_pRenderData), refusal on null;
//   0x0040f191  one row per iteration, the row length recomputed each time
//               (`rep movsl` then `rep movsb` is VC6's inline memcpy), the
//               source stepping by the pitch at +0x18 and the destination by
//               the packed row;
//   0x0040f1ce  g_pRender+0xe0 UnlockSurface, then 0x0040c5a0 releases the
//               render data (cissurf.cpp's cis_FreeRenderDataQ40C).  The two
//               cdecl pushes share one `add $0x8`, as the dalloc and lock
//               pushes do above.
class CSurfaceRenderView
{
public:
	// AvP2's RenderStruct has two more dwords ahead of these than NOLF's.
	uint8   m_pad_00[0xdc];
	void*  (*LockSurface)(void* pRenderData);                       // 0x0dc
	void   (*UnlockSurface)(void* pRenderData);                     // 0x0e0
};

// The pointer itself is cissurf.cpp's, under that file's type; this file
// reaches its two slots through the view above.
class RenderStructQ40C;
extern RenderStructQ40C*   g_pRender;             // 0x004b17a4  (cissurf.cpp)
#define g_pRenderQ2C ((CSurfaceRenderView*)g_pRender)
extern uint32              g_nBytesPerPixel;      // 0x004b1794  (cissurf.cpp)

void* dalloc(uint32 nSize);                                         // 0x00428fd0
void  cis_FreeRenderData(CSurfaceView* pSurface);                   // 0x0040c5a0

LTBOOL surface_CopyOut(CSurfaceView* pSurface)
{
	uint8* pBits;
	uint32 y;

	pSurface->m_pSysMem = dalloc(pSurface->m_nWidth * g_nBytesPerPixel *
	                             pSurface->m_nHeight);

	pBits = (uint8*)g_pRenderQ2C->LockSurface(pSurface->m_pRenderData);
	if (!pBits)
		return LTFALSE;

	for (y = 0; y < pSurface->m_nHeight; y++)
	{
		memcpy((uint8*)pSurface->m_pSysMem + pSurface->m_nWidth * g_nBytesPerPixel * y,
		       pBits + pSurface->m_nPitch * y,
		       pSurface->m_nWidth * g_nBytesPerPixel);
	}

	g_pRenderQ2C->UnlockSurface(pSurface->m_pRenderData);
	cis_FreeRenderData(pSurface);

	return LTTRUE;
}


// ======================================================================= //
//  The render DLL going away, the caller -- 0x0040f060
// ======================================================================= //

// 0x0040f060 -- shadow the screen format, copy every surface out, drop the
// renderer.  clientrender.cpp's r_TermRender is the one caller and it tests
// the answer, and the answer is surface_SaveAll's: the shipped tail is
// `call 0x0040f110; movl $0x0,0x4b17a4; ret`, the store leaving %eax alone.
//
// The thirteen dword moves 0x004b175c..0x004b178c -> 0x004b15c4..0x004b15f4
// skip the dword at +0x00 of each 0x38-byte block: that is the implicit
// operator= of a PFormat (lttexmod.cpp's layout), which copies every member
// and never the vptr.  0x004b1794 -> 0x004b1790 is the matching shadow of
// g_nBytesPerPixel (cissurf.cpp's g_nSavedBytesPerPixelQ40C).  (names ours)
class PFormatQ40F
{
public:
	virtual void Init(uint32 nBitDepth, uint32 aMask, uint32 rMask,
	                  uint32 gMask, uint32 bMask);          // 0x0045f0e0

	uint32 m_nBitDepth;                      // 0x04
	uint32 m_Masks[4];                       // 0x08
	uint32 m_nBits[4];                       // 0x18
	uint32 m_nShifts[4];                     // 0x28
};

// All three are cissurf.cpp's, reached under that file's names and types.
class PFormatQ40C;
extern PFormatQ40C g_ScreenFormat;                  // 0x004b1758
extern PFormatQ40C g_SavedScreenFormatQ40C;         // 0x004b15c0
extern uint32      g_nSavedBytesPerPixelQ40C;       // 0x004b1790
#define g_ScreenFormatQ40F      (*(PFormatQ40F*)&g_ScreenFormat)
#define g_SavedScreenFormatQ40F (*(PFormatQ40F*)&g_SavedScreenFormatQ40C)

LTBOOL surface_SaveAll();                           // 0x0040f110

LTBOOL r_UnpublishRenderStruct()
{
	LTBOOL bRet;

	g_SavedScreenFormatQ40F   = g_ScreenFormatQ40F;
	g_nSavedBytesPerPixelQ40C = g_nBytesPerPixel;

	bRet = surface_SaveAll();
	g_pRender = LTNULL;
	return bRet;
}


// ======================================================================= //
//  The render mode the console asks for -- 0x0040f810
// ======================================================================= //

// 0x0040f810 -- fill an RMode (ltbasedefs.h, 0x214 bytes: the `rep stos`
// count 0x85) from the requested screen size and two console variables.
// __cdecl, one argument.  ccclient.cpp's restart path hands the result to
// r_InitRender through 0x004116d0.
//
//   * 0x004a9f08 / 0x004a9f0c / 0x004a9f04 go to m_Width / m_Height /
//     m_BitDepth (+0x204/+0x208/+0x20c);
//   * "renderdll" (0x004a839c) through con_FindVar (0x0041b3e0) on the
//     client console 0x004b2f00: strncpy (0x00492b60) of its string (+0x14)
//     into m_RenderDLL with 0xff -- NO null test of the string -- else an
//     inline strcpy of "d3d.ren";
//   * "carddesc" (0x004a8388): strncpy into m_InternalName (+0x104) with
//     0x7f, only when the variable AND its string are there.
class LTCommandVarQ40F
{
public:
	uint8  m_pad_00[0x14];
	char*  m_pStringVal;                     // 0x14
};

LTCommandVarQ40F* con_FindVarQ40F(ConsoleStateRef* pState,
                                  const char* pName);       // 0x0041b3e0

// dsi.cpp's console-bound "ScreenWidth", "ScreenHeight" and "BitDepth".
extern int g_CV_ScreenWidth;                         // 0x004a9f08
extern int g_CV_ScreenHeight;                        // 0x004a9f0c
extern int g_CV_BitDepth;                            // 0x004a9f04

// The client interface's sound and music managers, CClientMgr's members
// (clientmgr.h: m_SoundMgr at 0x784, m_MusicMgr at 0x1094). AvP2 0x0040fba0
// and 0x0040fbb0 (Primal Hunt 0x00410980, 0x00410990): this translation
// unit's, not clientinterface.cpp's (whose callers, compiled by cl 9044,
// would otherwise know which registers they leave alone).
class ILTClientSoundMgr;
class CClientMusicMgr;

ILTClientSoundMgr* ci_GetSoundMgr()
{
	return (ILTClientSoundMgr*)((uint8*)g_pClientMgr + 0x784);
}

CClientMusicMgr* ci_GetMusicMgr()
{
	return (CClientMusicMgr*)((uint8*)g_pClientMgr + 0x1094);
}

void r_GetRequestedMode(RMode* pMode)
{
	LTCommandVarQ40F* pVar;

	memset(pMode, 0, sizeof(RMode));

	pMode->m_Width    = g_CV_ScreenWidth;
	pMode->m_Height   = g_CV_ScreenHeight;
	pMode->m_BitDepth = g_CV_BitDepth;

	pVar = con_FindVarQ40F(&g_ClientConsoleStateRef, "renderdll");
	if (pVar)
		strncpy(pMode->m_RenderDLL, pVar->m_pStringVal, sizeof(pMode->m_RenderDLL) - 1);
	else
		strcpy(pMode->m_RenderDLL, "d3d.ren");
#ifdef AVP2_MODERN
	// The modern build ships d3d9.ren as its default and our d3d.ren as
	// d3d7.ren: a config naming d3d.ren gets the default.
	if (!_stricmp(pMode->m_RenderDLL, "d3d.ren"))
		strcpy(pMode->m_RenderDLL, "d3d9.ren");
#endif

	pVar = con_FindVarQ40F(&g_ClientConsoleStateRef, "carddesc");
	if (pVar && pVar->m_pStringVal)
		strncpy(pMode->m_InternalName, pVar->m_pStringVal, sizeof(pMode->m_InternalName) - 1);
}


// ======================================================================= //
//  The file-scope 12-byte object bank at 0x004b17a8 -- 0x0040f780
// ======================================================================= //

// 0x0040f780 is the CONSTRUCT half of a dynamic initializer, with the
// constructor expanded into it (the wgrid.cpp 0x0043c3f0 pattern): the
// .CRT$XCU head 0x0040f770 is `call 0x0040f780 ; jmp 0x0040f7b0`, and
// 0x0040f7b0 hands the destructor thunk 0x0040f7c0 to atexit.  The body:
//
//   push 0x400 ; push 0x40 ; push 0xc ; push 0x004b17ac
//   movl $0x004a1ae8, 0x004b17a8          one vptr store, the derived one
//   call 0x00490ba0                       sb_Init2
//
// so this is stdlith object_bank.h's `ObjectBank<T>(cacheSize,
// preAllocate)` for a 12-byte T, 64 per chunk and 1024 up front, with no
// base vptr store because nothing runs between the two.  Its table
// 0x004a1ae8 is {0x00412f10 deleting dtor, 0x004049d0 AllocVoid, 0x004049c0
// FreeVoid, 0x00412bf0 Term} over the base's 0x004a14c8 {0x004041b0,
// _purecall x3}: BaseObjectBank / ObjectBank exactly, and 0x00412bf0 is
// Term's `sb_Term2(&m_Bank, 1); sb_Init(&m_Bank, 0xc, 0x20)`.  The dtor
// thunk 0x0040f7c0 is ~ObjectBank's `sb_Term2(&m_Bank, 1)` (0x00490bd0)
// between the derived and base vptr stores.
//
// object_bank.h is not on this target's include path, so the pair is
// restated.  The three non-destructor slots are declared, not defined:
// their bodies are shared template COMDATs other TUs already supply.  The
// element type and the object's name are not in the image; it sits right
// after g_pSurfaceList (0x004b17a0) and g_pRender (0x004b17a4), which puts
// it in the surface TU's data.  (names ours)
class StructBankView0c
{
public:
	uint32 m_StructSize;                          // 0x00
	uint32 m_Unknown04;                           // 0x04
	uint32 m_CacheSize;                           // 0x08
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                           // 0x18
};

// stdlith's struct bank entry points (StdLith.lib), under their own types.
struct StructBank_t;
int  sb_Init2(StructBank_t* pBank, uint32 structSize, uint32 cacheSize,
              uint32 preAllocate);                                  // 0x00490ba0
void sb_Term2(StructBank_t* pBank, int bCheckObjects);              // 0x00490bd0

class CBankElem0c
{
public:
	uint32 m_Data[3];
};

class BaseObjectBankView
{
public:
	virtual         ~BaseObjectBankView() {}
	virtual void*   AllocVoid()=0;
	virtual void    FreeVoid(void* ptr)=0;
	virtual void    Term()=0;
};

class CObjectBank0c : public BaseObjectBankView
{
public:
	CObjectBank0c(uint32 cacheSize, uint32 preAllocate)
	{
		sb_Init2((StructBank_t*)&m_Bank, sizeof(CBankElem0c), cacheSize, preAllocate);
	}

	virtual ~CObjectBank0c()
	{
		sb_Term2((StructBank_t*)&m_Bank, 1);
	}

	virtual void*   AllocVoid();                                    // 0x004049d0
	virtual void    FreeVoid(void* ptr);                            // 0x004049c0
	virtual void    Term();                                         // 0x00412bf0

	StructBankView0c m_Bank;                      // 0x04
};

CObjectBank0c g_Bank0c(0x40, 0x400);              // 0x004b17a8


// ======================================================================= //
//  The eight-slot collection body — 0x0040f640
// ======================================================================= //

// Twenty-one bytes filling EIGHT vtable slots, which makes it the highest-
// leverage body in the run.  All eight sit inside an identical twelve-entry
// block that recurs at 0x004a1a84+0x20, 0x004a1e88+0x20, 0x004a23e4+0x20,
// 0x004a2648+0x20, 0x004a2a40+0x20, +0x50, +0x80 and 0x004a3860+0x20 — the
// same twelve addresses every time, once standing alone as a whole twelve-
// slot table (0x004a23e4) and otherwise embedded.  Eight vtables sharing
// twelve function bodies is a template with eight instantiations whose code
// is identical for a pointer-sized element and folded by the linker.
//
// What the twelve do, read off their bodies, is a collection interface:
//
//    0  0x00439fc0  hand back a cursor by value (the `sub $0x8` local and
//                   the two stores into the hidden return slot)
//    1  0x00459f30  is the cursor valid            (`*p != 0`)
//    2  0x0041d520  advance, wrapping to null at the head
//    3  0x00459f40  the element at the cursor
//    4  0x0041d550  link one in and bump the count
//    5  0x0041d5a0  unlink one and drop the count
//    6  0x00459f50  head = 0, count = 0
//    7  0x00459f60  the count
//    8  THIS
//    9  0x00439fe0  walk another collection through slots 0/1/2 and add
//                   each element to this one
//   10  0x004895b0  a lookup that writes what it found through a pointer
//   11  0x00459f70  nothing, one argument
//
// so slot 8 is "replace my contents with another collection's": clear, then
// do slot 9's work.  The zeroing is written out rather than delegated to
// slot 6, and the reload of the vptr after the two stores is ordinary — the
// stores are at +0x04 and +0x08 and VC6 cannot prove they miss the vptr at
// +0x00.  `ret $0x4`, and the return value falls straight out of the call to
// slot 9.
//
// The definition belongs in the template's header, not in this TU: the
// folding is over eight instantiations of one inline body.  It is written
// out here so the shape is on record.
class CCollectionBase
{
public:
	virtual void        _p00()=0;                              // slot 0
	virtual LTBOOL      IsValid(void* pCursor)=0;              // slot 1
	virtual void        Advance(void* pCursor)=0;              // slot 2
	virtual void*       GetAt(void* pCursor)=0;                // slot 3
	virtual LTBOOL      Add(void* pElement)=0;                 // slot 4
	virtual void        Remove(void* pElement, int nFlag)=0;   // slot 5
	virtual void        RemoveAll()=0;                         // slot 6
	virtual uint32      GetSize()=0;                           // slot 7
	virtual LTBOOL      Copy(CCollectionBase* pSource);        // slot 8
	virtual LTBOOL      Append(CCollectionBase* pSource)=0;    // slot 9
	virtual LTBOOL      Find(void* pKey, void** ppFound)=0;    // slot 10
	virtual void        _p2c(void* p1)=0;                      // slot 11

	// The two words slots 6 and 7 touch, at the offsets they touch them.
	void*  m_pHead;                                            // 0x04
	uint32 m_nElements;                                        // 0x08
};

// 0x0040f640
LTBOOL CCollectionBase::Copy(CCollectionBase* pSource)
{
	m_nElements = 0;
	m_pHead     = LTNULL;

	return Append(pSource);
}
// The shipped stores are +0x08 then +0x04 — the count before the head, which
// is the order above; both come out of one zeroed register.


// ======================================================================= //
//  The file manager's per-shell reset -- 0x00404480
// ======================================================================= //

// Home: clientwin.cpp, which models this file manager in full as CluFileMgr
// (0x00403fd0's run) and declares this address as clu_sub_404480.  Written
// here rather than in clientwin.cpp; the view below restates only
// the fields this function touches, at CluFileMgr's offsets.  Called from
// cm_OnShellTerm above.  Close the cache tree, then for each of the 100
// id buckets unlink every net-file record, give its file record (if any)
// back to the 0x20-byte bank and the record itself to the 0x28-byte bank,
// and finally close the message queue.  The shipped `dec` of a counter kept
// in the dead argument slot is a separate down-counter.  (Names ours.)
struct CFmStructBankView
{
	uint8  m_pad_00[0x18];
	void*  m_pFreeList;                      // +0x18
};

inline void fm_sb_Free(CFmStructBankView* pBank, void* pStruct)
{
	*(void**)pStruct = pBank->m_pFreeList;
	pBank->m_pFreeList = pStruct;
}

struct CFmNetFileRecView
{
	LTLink m_Link;                           // 0x00
	uint8  m_pad_0c[0x14 - 0x0c];
	void*  m_pFileRec;                       // 0x14
};

class CFileMgrView
{
public:
	uint8             m_pad_000[0x28];
	CFmStructBankView m_FileRecBank;         // 0x028  (CluFileRecBank::m_Bank)
	uint8             m_pad_044[0x4c - 0x44];
	CFmStructBankView m_NetFileRecBank;      // 0x04c  (CluNetFileRecBank::m_Bank)
	uint8             m_pad_068[0x6c - 0x68];
	void*             m_pCacheTree;          // 0x06c
	LTLink            m_ByID[100];           // 0x070
	LTLink            m_ByName[200];         // 0x520
	void*             m_pQueue;              // 0xe80
};

void fs_CloseTree(void* pTree);                                     // 0x00420cd0
void sub_42f780(void* pQueue);                                      // 0x0042f780

// A duplicate: clientfilemgr.cpp's clu_sub_404480 is the one both images
// place (Primal Hunt 0x004048e0, AvP2 0x004043e0); cm_OnShellTerm reaches
// it through link/aliases.rsp.
#if 0
void sub_404480(CFileMgrView* pFileMgr)
{
	if (!pFileMgr)
		return;

	if (pFileMgr->m_pCacheTree)
	{
		fs_CloseTree(pFileMgr->m_pCacheTree);
		pFileMgr->m_pCacheTree = LTNULL;
	}

	LTLink* pHead = pFileMgr->m_ByID;
	uint32 nBuckets = 100;
	do
	{
		LTLink* pCur = pHead->m_pNext;
		while (pCur != pHead)
		{
			CFmNetFileRecView* pRec = (CFmNetFileRecView*)pCur->m_pData;
			LTLink* pNext = pCur->m_pNext;

			if (pRec->m_pFileRec)
				fm_sb_Free(&pFileMgr->m_FileRecBank, pRec->m_pFileRec);

			pCur->Remove();
			fm_sb_Free(&pFileMgr->m_NetFileRecBank, pRec);

			pCur = pNext;
		}
		++pHead;
	}
	while (--nBuckets);

	if (pFileMgr->m_pQueue)
	{
		sub_42f780(pFileMgr->m_pQueue);
		pFileMgr->m_pQueue = LTNULL;
	}
}
#endif


// ======================================================================= //
//  Notes on the rest of this run
// ======================================================================= //
//
//  FOUR ADDRESSES THE SDK AND clientmgr.h NAME FOR FREE.  Each is an inline
//  from a header this reconstruction already has, emitted out of line.  None
//  is redefined here — referencing them from a caller is what makes VC6 emit
//  the header's own COMDAT, the way servercommon.cpp gets MatVMul_H — but
//  each is recorded because the match is a free identification:
//
//    * 0x00410e00 (17 bytes) is `LTList::LTList()` from
//      src/LT2/sdk/inc/ltlink.h, exactly.  The header's body is
//      `m_Head.TieOff(); m_nElements = 0;` and TieOff() is
//      `m_pPrev = m_pNext = (LTLink*)this` — a chained assignment, so
//      m_pNext is written first.  The shipped bytes store +0x04 then +0x00
//      then zero +0x00 of the list, in that order, and return `this`.  The
//      constructor at 0x00410ab0 calls it on CClientMgr+0x1100 and has the
//      same three stores inlined for CClientMgr+0x1110, which is what makes
//      both fields LTLists rather than look-alikes.
//    * 0x00411d40 (25 bytes) is a three-float constructor returning `this`,
//      `ret $0xc` — LTVector's, from ltvector.h.  Its call site at
//      0x00410ab2 builds LTVector(0.0f, -2000.0f, 0.0f): the middle push is
//      0xc4fa0000, which is -2000.0f, and the neighbouring store is
//      0xbf350481, -0.70710678f.  Gravity and a diagonal.
//    * 0x00411a30 (32 bytes) is the same shape with four floats,
//      `ret $0x10` — LTRotation's, from ltrotation.h.
//    * 0x004147e0 (14 bytes) is `CClientFileRef::CClientFileRef()`, already
//      declared inline in src/engine/lithtech/clientmgr.h: two dwords and a
//      *word* zeroed, which is m_nFileType / m_pFilename / uint16
//      m_nLocalID, field for field and width for width.
//
//  COMPILER-GENERATED, NOT SOURCE.  The 53-pointer run at 0x004a7004 is the
//  CRT's dynamic-initializer table (`.CRT$XC*`, merged into .data), so its
//  entries are the `??__E` thunks VC6 writes for file-scope objects and no
//  source line corresponds to them:
//
//    * 0x0040f770 — `call 0x0040f780; jmp 0x0040f7b0`, table entry 4.  The
//      constructor it calls sets the vptr of the object at 0x004b17a8 to
//      0x004a1ae8 and initialises the pool at 0x004b17ac with (0x0c, 0x40,
//      0x400); the jump registers the destructor thunk 0x0040f7c0 with
//      atexit (0x0049307e).  0x0040f7c0 in turn tears the pool down and
//      resets the vptr to the base's, 0x004a14c8, which is the ordinary
//      inlined base destructor.  So there is a file-scope object here whose
//      element size is 12 bytes.  It is now written (g_Bank0c above), and
//      0x0040f780 and 0x0040f7c0 compile from it.
//    * 0x004131f0, 0x00415fd0, 0x00415ff0, 0x00416010, 0x00416030 — table
//      entries 5..9, each a `jmp` into an eleven-nop gap.  Four of them
//      reach a bare `ret`: file-scope objects whose constructors inline to
//      nothing.  The fifth sets 0x004b17e4 to 0xff00.
//
//  DELIBERATELY SKIPPED.
//
//    * 0x00410018, 0x00410020, 0x0041002e, 0x00410030, 0x004131af,
//      0x00411473, 0x00417845 — not function starts.  Each
//      disassembles mid-instruction and each is referenced from a switch
//      jump table (the run at 0x004a872c, for instance, holds 0x00416380,
//      0x004163e0, 0x00416440, 0x004164a0, 0x00416560 and 0x004162e0) or is
//      the fall-through half of a function the scan split.  0x00417840 is
//      the five-byte prologue of the function 0x00417845 continues.
//    * 0x00411d80, 0x00411dc0, 0x00411e00, 0x00411e40, 0x00411e80,
//      0x00411ec0, 0x00411f00, 0x00411f40, 0x00411f80, 0x00411fc0,
//      0x004128d0 — eleven 58-byte bodies differing in exactly one dword,
//      the address of the constructor they end on (0x0045b180, 0x0045aca0,
//      0x0045b470, 0x0045b530, 0x0045b590, 0x0045b610, 0x0045b7f0,
//      0x0045b940, 0x0045ba50, 0x0045bad0, 0x0045a670).  All eleven are one
//      template: take the free head at +0x1c, grow the pool at +0x04
//      through 0x00490c20 if it is empty, unlink, and placement-new the
//      element — the redundant `test %esi,%esi` before the constructor call
//      is `new`'s null check, which VC6 emits even where the value is known
//      non-null.  Worth doing as a template in one pass with the eleven
//      element types identified; not worth doing eleven times by hand, and
//      the element types are not decidable from these bytes.
//    * 0x00412bb0, 0x00412bd0, 0x00412bf0, 0x004151f0 — the same family's
//      constructors and reset methods (`0x00490b50(&m_Bank, size, count)`
//      with sizes 0x34/0x34/0x0c/0x54 and count 0x20, and
//      `0x00490bd0(&m_Bank, 1)` then a re-init for the resets).  Same
//      reason.
//    * 0x004152b0 — `if (pArg2) pArg1->Slot2(pArg2)`, cdecl, two arguments.
//      Six call sites, all in the surface code around 0x0040cec5, but
//      nothing in this run establishes the class behind that vtable, so the
//      slot cannot be named and the argument types would be invented.
//    * 0x004115c0's neighbour 0x004116d0 -- needs a class layout that
//      could not be established from the shipped bytes.  (0x004156a0,
//      0x00410640, 0x00411440, 0x00411450, 0x00411c80, 0x00415580,
//      0x004155c0, 0x00415760, 0x00415bb0, 0x00404480, 0x00410380,
//      0x00414240 (near), 0x00415d00 and 0x00415da0, once on this list or next to
//      it, are written above.)
//
//  FOR THE NEXT PASS.
//
//    * g_pClientMgr (0x004b17e0) and g_pClientShell (0x004b17ec) are the
//      two globals the client's startup hangs off, and they are set in that
//      order: CClientMgr::SetGlobals from the manager's constructor, then
//      CClientShell::Init from CClientMgr::StartGame (0x0040fdd5).
//    * The dword at 0x004b2f44 is ConsoleState+0x44 — one past the last
//      field src/engine/server/servermgr.h models — and it holds the
//      CClientMgr the console works through.  Its server-side counterpart
//      is m_pServerMgr at +0x38.  It cannot be added to that header without
//      putting server.dll's 1580 identical functions at risk, so it is
//      declared on its own above.
//    * 0x004b43d4 and 0x004b43d8 extend the render-DLL function table
//      clientmgr.h already carries from 0x004b43dc; they make and unmake
//      the renderer's per-world object, which lives in CClientMgr+0x1c8.
//    * The 133-slot table at 0x004a1b00 is the client half of a base the
//      shared engine code calls through, one 8-byte object per manager, and
//      "CLIENT-OBJECT" is its name for itself.  Its server twin hangs off
//      CServerMgr+0x6a8; matching the two tables slot for slot would name a
//      lot of these functions at once.
//    * CClientShell has two vtables (0x004a1d18 with 5 slots and
//      0x004a1d40 with 48), so it is multiply derived.  The base at
//      0x004a1d2c has three pure slots and one default, and the default is
//      the folded `ret $0xc` above.
