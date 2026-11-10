// *********************************************************************** //
//
//  MODULE:   CLIENTCSBASE (.CPP)   (file name ours) -- lithtech.exe only
//
//  The client side of the shared base, vtable 0x004a1b00: its slot bodies
//  (AvP2 1.0.9.6 from 0x004157f0; Primal Hunt 0x004196a0 .. 0x00419b40).
//  Its own translation unit: in Primal Hunt the run follows the loader
//  thread's <string> guard pair (0x00419670) and ends in its own
//  (0x00419b50, 0x00419b70).  Split out of clientmgr2.cpp, whose views are
//  repeated here.
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
#ifdef AVP2_MODERN
#include "..\..\modern\engine\p1_frametime.h"   // M-P1-23
#endif


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


// ======================================================================= //
//  The client side of the shared base — vtable 0x004a1b00
// ======================================================================= //

// 0x004157f0 — three bytes, `ret $0x8`.
//
// This one body fills FOUR slots of the table at 0x004a1b00: 0 (+0x000),
// 3 (+0x00c), 7 (+0x01c) and 8 (+0x020).  Four separate two-argument
// virtuals the client has nothing to do, compiled to identical code and
// folded into one by the linker — which is why the build needs /Gy, and why
// editing this one body moves four slots at once.  eax is not touched, so
// none of the four has a return value.
//
// Written out as four definitions because that is what the vtable says was
// declared; nothing distinguishes them but the slot they land in.
void CClientCSBase::_p00(void* /*p1*/, void* /*p2*/) {}
void CClientCSBase::_p0c(void* /*p1*/, void* /*p2*/) {}
void CClientCSBase::_p1c(void* /*p1*/, void* /*p2*/) {}
void CClientCSBase::_p20(void* /*p1*/, void* /*p2*/) {}

// 0x00415800 — slot 11 (+0x02c): the side's name.
//
// Eight bytes: load the literal, `ret $0x4`.  The argument is never read and
// neither is `this`, so both are dead and both are in the signature only
// because the shipped `ret $0x4` says a parameter was written.  This is the
// string that identifies the whole table: the server's counterpart at
// CServerMgr+0x6a8 is the other half of the same base.
const char* CClientCSBase::GetSideName(void* /*pUnused*/)
{
	return "CLIENT-OBJECT";
}

// 0x00415750 — slot 1 (+0x004): the address of the manager's 0x504 field.
//
// Nine bytes and no dereference of the result — `add $0x504,%eax` on the
// back pointer and out.  The field is real: CClientMgr::Init zeroes it at
// 0x0040fbd5.  What the caller does with the address is not visible from
// here, so the field keeps its offset for a name.
CollisionInfo** CClientCSBase::GetCollisionInfo()
{
	return &m_pClientMgr->m_pCollisionInfo;
}

// 0x00415760 — slot 2 (+0x008): the engine's touch notification.
//
// The client twin of server objmover.cpp's slot 2 (0x0047c120 /
// 0x10035269): record the collision in the manager's CollisionInfo, then
// tell the game.  Where the server hands MID_TOUCHNOTIFY to the object's
// class, the client calls IClientShell slot 22 (+0x58), OnTouchNotify
// (hMain, pInfo, forceMag) -- the shipped pushes are the float as an
// integer move, then the CollisionInfo pointer, then the first argument.
// The pointer is re-read off the manager at each use, as on the server.
//
// AVP2: AvP2's and Primal Hunt's (PH 0x004196b0, 141 bytes; AvP2
// 0x004175a0, the same bytes) also clear the polygon: unless the toucher is
// null, answers its slot 13 (+0x34) or is of type 2 (OT_WORLDMODEL) or 9
// (OT_CONTAINER) -- the byte at +0xbc -- m_hPoly becomes INVALID_HPOLY
// (0x004196f7..0x0041970e).  Slot 13's meaning is not established here.
class CTouchObjectViewQ526
{
public:
	virtual void   _v00()=0; virtual void _v04()=0; virtual void _v08()=0;
	virtual void   _v0c()=0; virtual void _v10()=0; virtual void _v14()=0;
	virtual void   _v18()=0; virtual void _v1c()=0; virtual void _v20()=0;
	virtual void   _v24()=0; virtual void _v28()=0; virtual void _v2c()=0;
	virtual void   _v30()=0;
	virtual LTBOOL Unknown34()=0;                 // slot 13

	uint8  m_pad_04[0xbc - 0x04];
	uint8  m_ObjectType;                          // 0xbc
};

void CClientCSBase::TouchNotify(HLOCALOBJ hObj, HOBJECT hToucher,
                                LTVector* pStopVel, float fForceMag)
{
	if (m_pClientMgr->m_pCollisionInfo)
	{
		m_pClientMgr->m_pCollisionInfo->m_vStopVel = *pStopVel;
		m_pClientMgr->m_pCollisionInfo->m_hObject  = hToucher;

		if (hToucher)
		{
			CTouchObjectViewQ526* pToucher = (CTouchObjectViewQ526*)hToucher;

			if (!pToucher->Unknown34() &&
			    pToucher->m_ObjectType != OT_WORLDMODEL &&
			    pToucher->m_ObjectType != OT_CONTAINER)
			{
				m_pClientMgr->m_pCollisionInfo->m_hPoly = INVALID_HPOLY;
			}
		}

		if (m_pClientMgr->m_pClientShell)
			m_pClientMgr->m_pClientShell->OnTouchNotify(
			    hObj, m_pClientMgr->m_pCollisionInfo, fForceMag);
	}
}

// AVP2: 0x00419740, slot 5 (+0x014) -- the client's twin of objmover.cpp's
// CObjectMoverQ47C::UpdateAttachments (server.dll 0x10045177): move every
// attached child to where its attachment puts it relative to the parent.
// The client differs from the server in three ways, each read off the
// bytes: a child with none of FLAG_SOLID | FLAG_TOUCH_NOTIFY |
// FLAG_CONTAINER (0x20012000) is skipped outright (0x004197a1); the place
// is computed in line -- quat_ConvertToMatrix (0x00451180), the
// one-argument Apply3x3 (0x00419a40 and its copy-back), the parent's
// position added -- where the server calls 0x1002a7ed; and every move goes
// with flags 5 (0x00419822).  The rotation is the parent's times the
// attachment's (LTRotation::operator*, quat_Mul 0x004199b0), compared with
// LTRotation::Equals at 0.00001f (the double at 0x004d4eb8); a world model
// or container is turned through wm_RotateWorldModel (0x00466a40) with the
// child's own priority, anything else takes the rotation.  The child comes
// from clienttexmgr.cpp's tbl_GetObject (0x00428dc0).  The frame is the
// matrix, then the move request, then the scalars (names ours).
//
// IDENTICAL (611 bytes).  Two spellings are load-bearing: the child tests
// nest the body (a `continue` puts the `this` spill four bytes high), and
// the world-model test repeats the child's null test, which VC6 folds but
// which is what lays the rotate arm out first (0x00419920..0x00419926: `je`
// then `jne` to the assignment); `==`/`||`, `!=`/`&&`, a switch and a
// `continue` out of the rotate arm all put the assignment first.
struct WMMoveState
{
	// servermgr.h's inline constructor (one zero to +0x00, +0x04, +0x08,
	// +0x64 and +0x68: 0x00419756..0x00419769).
	WMMoveState()
	{
		m_pContainer = LTNULL;
		m_pUnknown04 = LTNULL;
		m_pObj       = LTNULL;
		m_nUnknown64 = 0;
		m_fUnknown68 = 0.0f;
	}

	void*                    m_pContainer;  // +0x00
	void*                    m_pUnknown04;  // +0x04
	class CAttachObjectQ558* m_pObj;        // +0x08
	uint32                   m_nUnknown0C;  // +0x0c
	uint32                   m_pPushList;   // +0x10
	uint32                   m_nPushCount;  // +0x14
	uint8                    m_pad_18[0x64 - 0x18];
	uint32                   m_nUnknown64;  // +0x64
	float                    m_fUnknown68;  // +0x68
};

// ltobject_internal.h's Attachment.
class CAttachmentQ558
{
public:
	LTransform       m_Offset;              // 0x00  position + rotation offset
	uint16           m_nParentID;           // 0x1c
	uint16           m_nChildID;            // 0x1e
	int              m_iNode;               // 0x20
	CAttachmentQ558* m_pNext;               // 0x24
};

// ltobject_internal.h's LTObject, at the fields the walk reaches.
class CAttachObjectQ558
{
public:
	uint8            m_pad_000[0x88];
	uint32           m_Flags;               // 0x088
	uint8            m_pad_08c[0x98 - 0x8c];
	CAttachmentQ558* m_pAttachments;        // 0x098
	LTRotation       m_Rotation;            // 0x09c
	uint8            m_pad_0ac[0xbc - 0xac];
	char             m_ObjectType;          // 0x0bc
	uint8            m_BlockingPriority;    // 0x0bd
	uint8            m_pad_0be[0x194 - 0xbe];
	LTVector         m_Pos;                 // 0x194
};

void* tbl_GetObject(CClientMgrTexBankView* pClientMgr, uint16 nID);  // 0x00428dc0
void  quat_ConvertToMatrix(LTRotation* pRot, LTMatrix* pMat);        // 0x00451180
void  wm_MoveObject(WMMoveState* pState, LTVector vPos, uint32 flags);   // 0x00462610
void  wm_RotateWorldModel(WMMoveState* pState, LTRotation* pRotation, int bUnknown); // 0x00466a40

// objmover.cpp's six-field move-request fill (om_InitMoveState).
static inline void cm_InitMoveStateQ558(WMMoveState* pState, void* pContainer,
                                        void* pUnknown04, CAttachObjectQ558* pObj,
                                        uint32 nPriority)
{
	pState->m_pContainer = pContainer;
	pState->m_pUnknown04 = pUnknown04;
	pState->m_pObj       = pObj;
	pState->m_nUnknown0C = nPriority;
	pState->m_pPushList  = 0;
	pState->m_nPushCount = 0;
}

void CClientCSBase::UpdateAttachments(WMMoveState* pState)
{
	LTMatrix           mRot;
	WMMoveState        state;
	CAttachmentQ558*   pAttach;
	CAttachObjectQ558* pChild;
	LTVector           vOffset;

	for (pAttach = pState->m_pObj->m_pAttachments; pAttach; pAttach = pAttach->m_pNext)
	{
		pChild = (CAttachObjectQ558*)tbl_GetObject(
		    (CClientMgrTexBankView*)m_pClientMgr, pAttach->m_nChildID);

		if (pChild && (pChild->m_Flags & 0x20012000))
		{
			vOffset = pAttach->m_Offset.m_Pos;

			quat_ConvertToMatrix(&pState->m_pObj->m_Rotation, &mRot);
			mRot.Apply3x3(vOffset);

			LTVector vNew = pState->m_pObj->m_Pos + vOffset;

			cm_InitMoveStateQ558(&state, pState->m_pContainer, pState->m_pUnknown04,
			                     pChild, pState->m_nUnknown0C);

			wm_MoveObject(&state, vNew, 5);

			LTRotation rRot = pState->m_pObj->m_Rotation * pAttach->m_Offset.m_Rot;

			if (!rRot.Equals(pChild->m_Rotation, 0.00001f))
			{
				if (pChild && (pChild->m_ObjectType == OT_WORLDMODEL ||
				               pChild->m_ObjectType == OT_CONTAINER))
				{
					cm_InitMoveStateQ558(&state, pState->m_pContainer,
					                     pState->m_pUnknown04, pChild,
					                     pChild->m_BlockingPriority);

					wm_RotateWorldModel(&state, &rRot, 1);
				}
				else
				{
					pChild->m_Rotation = rRot;
				}
			}
		}
	}
}

// 0x00410eb0 — slot 12 (+0x030): two dereferences off the back pointer.
// Thirteen bytes, no null checks, no work: the manager's 0x4c0 object and
// its dword at +0x0c.  Both keep their offsets — nothing in this run
// establishes what either is.
uint32 CClientCSBase::GetUnknown4c0_0c()
{
	return *(uint32*)((uint8*)m_pClientMgr->m_pUnknown4c0 + 0x0c);
}

// 0x00419b00 — slot 13 (+0x034), PH (AvP2 0x00417a00, the same bytes):
// copy the manager's vector at +0x4dc out through the second argument when
// it is there, and say LT_OK either way.  The two copies are two arms on the
// first argument's null test (0x00419b0a), each with its own copy; only the
// null arm returns from inside (0x00419b2b).  What the first argument is is
// not visible here.
LTRESULT CClientCSBase::GetUnknown4dc(void* p1, LTVector* pVec)
{
	if (!p1)
	{
		if (pVec)
		{
			*pVec = m_pClientMgr->m_vUnknown4dc;
			return LT_OK;
		}
	}
	else
	{
		if (pVec)
			*pVec = m_pClientMgr->m_vUnknown4dc;
	}

	return LT_OK;
}

// 0x004157c0 — slot 6 (+0x018): does this record match?
//
// `this` is never read, which is what makes the body worth stating exactly.
// Three NESTED positive guards with one `return LTFALSE` after them: all
// three failures jump forward to the one shared `xor %eax,%eax; ret $0xc`
// at the end (0x004157eb), and the success tail `mov $0x1,%eax; ret $0xc`
// sits inside.  Three early refusals instead gave each its own inline
// epilogue and a `neg/sbb/neg` for the last test.  The last test is a
// virtual on the third argument, slot 12 (+0x30), asked for a truth value.
LTBOOL CClientCSBase::IsMatch(void* pRec, uint32 nKey, void* pTest)
{
	CSlot6Record*   pRecord = (CSlot6Record*)pRec;
	ISlot6TestView* pObj    = (ISlot6TestView*)pTest;

	// AVP2: PH 0x00419aa0 / AvP2 0x004179a0 (58 bytes): the key first, then
	// the record's +0x0c against the object's +0xbd, then the virtual.
	if (pRecord->m_nKey == nKey)
	{
		if (pRecord->m_Unknown0c > pObj->m_Unknown0bd)
		{
			if (pObj->Test())
				return LTTRUE;
		}
	}

	return LTFALSE;
}
