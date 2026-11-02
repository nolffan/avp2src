// *********************************************************************** //
//
//  MODULE:   CLIENTMGRCORE (.CPP)   (file name ours)
//
//  PURPOSE:  lithtech.exe, the eight unwritten functions of the run
//            0x00410670 .. 0x00411399 -- the heart of CClientMgr: the thread
//            drain, the per-frame Update, the counter/stat report it sends
//            to the server, the two sound entry points, the FACTORY that
//            news the manager up and runs its inlined constructor, the
//            DESTRUCTOR that takes it all down, and the string-holder list
//            release the destructor calls.
//
//  WHAT NAMES THE MODULE
//  ---------------------
//  There is no .data anchor on this run (none of the eight is referenced
//  from .data).  It is named from three directions and they all agree:
//
//    1. THE NEIGHBOURS.  src/engine/lithtech/clientmgr2.cpp already covers
//       0x00410350/0x00410510/0x00410530/0x00410540/0x00410590/0x00410610/
//       0x00410620/0x004113a0/0x004115c0/0x00411670/0x004116a0/0x00411a50/
//       0x00411cd0 and names them all CClientMgr methods off the shipped
//       string "CClientMgr::Init" at 0x0040fb47.  Six of those are called
//       from inside this run, all with `this` in %ecx off the same pointer.
//       That file ALSO cites, by address, sites that fall INSIDE two of the
//       functions below -- "the constructor at 0x00410ab0", "0x00410d70
//       allocates 8 bytes", "CClientMgr::Term (0x00410f42)" -- so the two
//       big bodies here are the ones it was already reading over the
//       shoulder of.
//
//    2. THE FIELD OFFSETS.  Every displacement in this run is one
//       clientmgr.h or clientmgr2.cpp already has on CClientMgr: 0x150c (the
//       connection / CClientShell), 0x61c (IClientShell*), 0x1100 and 0x1110
//       (the two LTLists), 0x1160 (the fifteen -1s), 0x1264 (the chain),
//       0x1518 (m_bConsoleReady), 0x508 (the "CLIENT-OBJECT" base), 0x40c
//       (the eleven sixteen-byte records).  The allocation at the head of
//       0x00410a70 is `push $0x21b4`, and 0x21b0 -- four bytes short of that
//       -- is the last field the body writes, so 0x21b4 is sizeof(CClientMgr)
//       and the run really is that whole object.
//
//    3. THE SHARED PREFIX WITH CServerMgr.  src/engine/lithtech/
//       clientshell.cpp reconstructs CServerMgr's constructor/destructor and
//       lists its members.  Subtract 0x1a4 from those offsets and they land
//       on this object's, member for member and CONSTRUCTOR ADDRESS FOR
//       CONSTRUCTOR ADDRESS:
//
//          CServerMgr 0x1a4 CServerWorld      <->  CClientMgr 0x000
//          CServerMgr 0x1a8 CArray1a8         <->  CClientMgr 0x004
//          CServerMgr 0x1bc CArray1bc         <->  CClientMgr 0x018
//          CServerMgr 0x1d0 CArray1d0         <->  CClientMgr 0x02c
//          CServerMgr 0x1e4 CArray1e4         <->  CClientMgr 0x040 (Term 0x40bfe0)
//          CServerMgr 0x1f8 CArray1f8         <->  CClientMgr 0x054 (Term 0x415050)
//          CServerMgr 0x21c CSub21c           <->  CClientMgr 0x078 (dtor 0x48c790)
//          CServerMgr 0x2a0 CSub2a0           <->  CClientMgr 0x0fc (dtor 0x43c6e0)
//          CServerMgr 0x32c CArray32c         <->  CClientMgr 0x188 (Term 0x414fa0)
//          CServerMgr 0x340 CArray340         <->  CClientMgr 0x19c (Term 0x414fa0)
//          CServerMgr 0x390 CSub390           <->  CClientMgr 0x1ec (ctor 0x45a470)
//          CServerMgr 0x3f8 CSub3f8           <->  CClientMgr 0x254 (dtor 0x4128b0)
//          CServerMgr 0x41c..0x560 (ten)      <->  CClientMgr 0x278..0x3bc
//          CServerMgr 0x668 GlobalForce       <->  CClientMgr 0x4c4 (ctor 0x410e20
//                                                  plus the same -0.7071f at +0x30)
//
//       Thirteen members, one constant offset, and the two out-of-line
//       constructors (0x0045a470, 0x00410e20) and three out-of-line
//       destructors (0x004128b0, 0x0043c6e0, 0x0048c790) are literally the
//       same addresses in both.  So CClientMgr and CServerMgr share a common
//       prefix class 0x1ec+ bytes long that CServerMgr carries after its own
//       0x1a4-byte head and CClientMgr carries FIRST.  That is not modelled
//       as a base below -- the shipped destructor restores no vptr at +0x00,
//       so whatever it is it is not polymorphic, and folding it into a base
//       would change nothing this file is scored on.
//
//  WHAT IS *NOT* HERE.  This is genuinely new ground: `grep -rn` for all
//  eight addresses across src/engine/ and src/lithshared/ returns nothing,
//  and none of the eight is a server.dll body under another name (the whole
//  run reaches CClientShell, IClientShell, the client sound manager and the
//  client console, and src/engine/server/*.cpp is annotated with 0x10xxxxxx
//  addresses only).
//
//  TRUE EXTENTS.  All eight rows have clean extents.  Three
//  carry two `ret`s (0x004106c0, 0x00410960, 0x00410a70) and each is the
//  ordinary /O2 duplicated epilogue -- the early `return` out of the middle
//  of the body, sharing no code with the tail.  Nothing is misaligned and
//  nothing overlaps.
//
//  GROUND TRUTH: the retail lithtech.exe ONLY, cl 8447 /O2 (frameless,
//  %esp-relative).  Every offset, constant and branch below is read off that
//  image and the note beside it says from where.
//
//  NAMING.  Every class and global here carries a `Cmg`/`cmg_` tag so that
//  nothing collides with src/engine/lithtech/clientmgr.h, clientmgr2.cpp or
//  clientshell.cpp, all three of which declare overlapping views of the same
//  objects under their own names.  When these fields are folded into
//  clientmgr.h the views go away.
//
// *********************************************************************** //

#include <stdlib.h>            // malloc -- 0x00428fa2
#include <stdio.h>             // sprintf -- 0x004929c4 in Init
#include <string.h>            // strcpy -- expanded inline by /O2 at 0x00410bd6

#include "ltbasedefs.h"        // LTBOOL, LTRESULT, uint8/16/32, LTNULL
#include "collection.h"       // CMoArray, LAlloc g_DefAlloc (0x004b4e88)
#include "ltrect.h"            // LTRect -- CCmgSub11F0::m_rcArea
#include "ltlink.h"            // LTLink, LTList  (0x00410e00 IS LTList::LTList)
#include "ltcodes.h"           // LT_OK
#include "de_error.h"          // RETURN_ERROR (CClientMgr::Init)
#include "iltstream.h"         // ILTStream -- the demo file, slots 0x08/0x24
#include "iclientshell.h"      // the game DLL's shell -- slots 0x30/0x34/0x38/
                               // 0x48/0x5c are reached below


// ======================================================================= //
//  1.  THINGS THIS RUN CALLS BUT DOES NOT DEFINE
// ======================================================================= //

class CCmgClientMgr;
class CCmgClientShell;

// src/engine/lithtech/stdlithalloc.cpp names both.
// 0x004290b0 / 0x004290c0 are dalloc.cpp's operator new / operator delete
// (each a one-call forwarder: 0x004290b0 -> 0x00428fd0, 0x004290c0 ->
// 0x00429080), and 0x00429080 is its dfree.
void  dfree(void* pMem);                                       // 0x00429080

// The engine's default allocator; its ADDRESS is what every collection
// teardown pushes (src/engine/lithtech/clientshell.cpp).
// (declared by collection.h as the LAlloc it is)

// ----------------------------------------------------------------------- //
//  stdlith's struct banks (src/engine/lithtech/lt2new3.cpp names sb_Init2 and
//  clientshell.cpp names sb_FreeChunks).
// ----------------------------------------------------------------------- //
class CCmgStructBank
{
public:
	uint8 m_pad_00[0x1c];
};

void sb_Init2(CCmgStructBank* pBank, uint32 nStructSize,
              uint32 nCacheSize, uint32 nGrowSize);            // 0x00490ba0
void sb_FreeChunks(CCmgStructBank* pBank, int bAll);           // 0x00490bd0
// 0x00490c10 is itself sb_FreeChunks(pBank, 0) (src/engine/lithtech/
// clientworld.cpp).
void sb_Term(CCmgStructBank* pBank);                           // 0x00490c10

// ----------------------------------------------------------------------- //
//  The console (src/engine/lithtech/clientconsole.cpp / ccclient.cpp name
//  all four).
// ----------------------------------------------------------------------- //
void con_ConsolePrint(char* pFormat, ...);                     // 0x0042e6f0
void con_PrintColored(uint32 nColour, int nLevel, char* pFormat, ...); // 0x0041b8a0
void con_Print(char* pFormat, ...);                            // 0x0041b8c0
// clientconsole.cpp's spelling: an int -- what it is handed is one of the
// dwords the 0x0042e5b0 table holds (clientdsi.cpp's sub_42e5b0).
void con_Sub41b8e0(int p1);                                    // 0x0041b8e0
void con_Sub41b820(int nStage);                                // 0x0041b820
// clientmgr2.cpp's "run a console file, if the console is up".
void cm_RunConsoleFile(CCmgClientMgr* pClientMgr, char* pFilename); // 0x004116a0

// binkvideo.cpp names 0x004b0ab0 g_DebugLevel; ltnew1.cpp records the same
// `cmp $0x2` gate in front of a printer.
extern int g_CmgDebugLevel;                                    // 0x004b0ab0

// ----------------------------------------------------------------------- //
//  src/engine/lithtech/clientdsi.cpp's block.  0x0042e690 is its
//  zero-argument "is the client window active" predicate; 0x0042e590 /
//  0x0042e5b0 / 0x0042e5e0 / 0x0042e5f0 are the uint16-counted command queue
//  that file maps at 0x004b11e4 / 0x004b11e6 (names ours where marked).
// ----------------------------------------------------------------------- //
int    dsi_IsClientActive();                                   // 0x0042e690
uint16 dsi_GetNumCommands();                                   // 0x0042e590
// The same queue read as clientdsi.cpp's three table readers and two count
// clears, under that file's names (it defines them): key-down (key, repeat)
// pairs counted by 0x0042e590 and key-ups counted by 0x0042e5a0.  The Update
// drain reads the key-down keys the same way (0x0042e5b0).
int    sub_42e5b0(uint32 nIndex);                              // 0x0042e5b0  key-down key
int    sub_42e5c0(uint32 nIndex);                              // 0x0042e5c0  key-down repeat
int    sub_42e5d0(uint32 nIndex);                              // 0x0042e5d0  key-up key
void   sub_42e5e0();                                           // 0x0042e5e0  clear the key-downs
void   sub_42e5f0();                                           // 0x0042e5f0  clear the key-ups
uint16 dsi_GetNumKeyUps();                                     // 0x0042e5a0
// 0x00428fd0 -- clientdsi.cpp's sub_428fd0, the one-argument allocator.
void*  dsi_Alloc(unsigned long nSize);                         // 0x00428fd0
void   dsi_Sub42e570(void* p);                                 // 0x0042e570
// 0x0042f5d0 -- clientdsi.cpp: "a singly linked list hanging off the client".
void   dsi_Sub42f5d0(CCmgClientMgr* pClientMgr);               // 0x0042f5d0
// 0x0042f710 -- clientdsi.cpp's errorlog_Term: "flush, close, forget".
void   errorlog_Term();                                        // 0x0042f710
// 0x00428fa0 -- clientworld.cpp puts it in dsi.cpp: mallocs 12 bytes and
// files the (callback, context) pair it is handed.  (name ours)
void   dsi_AddHandler(void (*pfn)(void*), CCmgClientMgr* pClientMgr); // 0x00428fa0
// The handler 0x00410a70 registers.  Outside this run.
void   cmg_Handler404470(void*);                               // 0x00404470

// ----------------------------------------------------------------------- //
//  CClientMgr's own neighbours, already reconstructed in
//  src/engine/lithtech/clientmgr2.cpp.  Declared, not redefined.
// ----------------------------------------------------------------------- //
void cm_ClearChain1264(CCmgClientMgr* pClientMgr);              // 0x00411cd0
void cm_ClearList1100(LTList* pList);                           // 0x004113a0
void cm_Sub411c80(CCmgClientMgr* pClientMgr, void* pRecord, int nFlag); // 0x00411c80

// ----------------------------------------------------------------------- //
//  The rest, by address.  Each note says exactly what the shipped call site
//  proves about it and nothing more.
// ----------------------------------------------------------------------- //
// __thiscall on the manager, no arguments.  0x00410620 is clientmgr2.cpp's
// "publish the manager" (g_pClientMgr = g_pConsoleClientMgr = this).
// 0x00410350 is its Disconnect.  0x00410640 takes one message.
// 0x00410500 is reached the same way with none (`mov %ebx,%ecx`): a member,
// written below beside the destructor.
// ----------------------------------------------------------------------- //
//  What 0x0040fc80 (TermClientShell) reaches.  The console's command list:
//  servermgr.h's ConsoleState keeps its LTLink head at +0x18 and each
//  ConsoleCommand's link at +0x08 (m_pData back to the command), with the
//  flag dword at +0x14 whose bit 0 marks a command the game shell added.
//  (view names ours; src/engine/server/console.cpp has the real ones)
// ----------------------------------------------------------------------- //
class CCmgConsoleCommand
{
public:
	char*  m_pName;                                             // 0x00
	void*  m_pFn;                                               // 0x04
	LTLink m_Link;                                              // 0x08
	uint32 m_Flags;                                             // 0x14
};

class CCmgConsoleState
{
public:
	uint8  m_pad_00[0x18];
	LTLink m_CommandHead;                                       // 0x18
};

// 0x004b2f00 -- console.cpp's ConsoleState, the client's instance.  Declared
// under its own name and read through the view (the address is the same
// constant either way, so the bytes do not change).
class ConsoleState;
extern ConsoleState g_ClientConsoleState;                       // 0x004b2f00
#define g_CmgClientConsoleState (*(CCmgConsoleState*)&g_ClientConsoleState)
void con_RemoveCommand(CCmgConsoleState* pState,
                       CCmgConsoleCommand* pCmd);               // 0x0041b340
// counter.cpp's sub_10034f9b / cb_FreeSoundTable at their client addresses:
// hand the module record's two entry points out, and release the record.
void cmg_GetShellFunctions(void* pRec, void** ppCreate,
                           void** ppDelete);                    // 0x00477f10
void cmg_FreeShellRecord(void* pRec);                           // 0x00477ef0
// 0x004016f0 -- FreeLibrary when the handle's +0x04 says it was loaded,
// then the 0x00429080 free.  (name ours)
void cmg_UnloadModule(void* pModule);                           // 0x004016f0

// 0x0041fc90 -- __cdecl (pClientMgr, nFlags) and it ANSWERS: every exit of
// it that the disassembly reaches sets %eax (`xor %eax,%eax` at 0x0041fce0).
// CClientMgr::Update returns its answer straight out, which is the only
// reason the type below is LTRESULT and not void (ccclient.cpp declares the
// same address void, having only needed the call).
LTRESULT cmg_Sub41fc90(CCmgClientMgr* pClientMgr, uint32 nFlags);   // 0x0041fc90
// 0x0041fdd0 -- __thiscall on the manager, two arguments, the pair the
// counter gather at 0x0042c310 wrote into its third and second out-slots.
// 0x00415810 -- __thiscall on the manager, four: the CNetMgr sub-object's
// ADDRESS, the connection id, the gathered buffer and the first out-slot.
// 0x00415bb0 -- __thiscall, no arguments (clientmgr2.cpp lists it).
// 0x0040bfd0, 0x0041e920, 0x00420020, 0x00484740 -- __cdecl teardown entries.
void cmg_Sub420020(CCmgClientMgr* p);                           // 0x00420020
void cmg_Sub41e920();                                           // 0x0041e920
void cmg_Sub40bfd0();                                           // 0x0040bfd0
void cmg_Sub484740(void (*pfn)(char*), int nFlag);              // 0x00484740
// 0x00411320 -- the callback that goes with it: `con_Print("Unfreed string:
// %s", p)` and nothing else.  Not one of this run's eight (it is 19 bytes of
// padding-terminated code between 0x00411318 and 0x00411340) but it is only
// ever named here, so it is written out below for the record.
void cmg_ReportUnfreedString(char* pString);                    // 0x00411320
// 0x004627d0 is clientrender.cpp's r_TermRender (ccclient.cpp names it).
void r_TermRender(CCmgClientMgr* pClientMgr, LTBOOL bReportFailure); // 0x004627d0
// src/engine/lithtech/clientworld.cpp: the render/texture cache flush.
void cmg_Sub45bb30(void* pSub1ec, int nFlag);                   // 0x0045bb30
void cmg_Sub45bc70(void* pSub1ec);                              // 0x0045bc70
// 0x00429080 is clientrender.cpp's dfree wrapper.
// (it is dalloc.cpp's dfree -- see the destructor)
// 0x00439fb0 -- clientinput.cpp: "hand the caller the interface table and
// report success".  One argument, the address of the slot it fills.
// Takes the ADDRESS of the table pointer: the call site is
// input_GetTable(&pClientMgr->m_pInput) and m_pInput is itself void**,
// so the parameter is void***. The one-star form was a slip in the
// declaration, not in the member, which the field's own uses confirm
// (m_pInput[1] is called with m_pInput as its argument).
void input_GetTable(void*** ppTable);                           // 0x00439fb0
// The client file manager: made by 0x00403fd0, destroyed by 0x004040f0,
// asked for a file by 0x00404680 and for a name by 0x004045d0.
class CCmgFileMgr;
class CCmgClientMgr;
CCmgFileMgr* fm_Create(CCmgClientMgr* pClientMgr);              // 0x00403fd0
void         fm_Destroy(CCmgFileMgr* pMgr);                     // 0x004040f0
void*        fm_FindFile(CCmgFileMgr* pMgr, char* pName, int nType); // 0x00404680
char*        fm_GetName(CCmgFileMgr* pMgr, char* pName);        // 0x004045d0

// The 0x2c4-byte object the factory news up with two allocator arguments and
// keeps at +0x160c.  Its only reached fields are a refcount at +0x208 and a
// virtual slot 0 that takes the deleting-destructor flag.
class CCmgRefCounted
{
public:
	CCmgRefCounted(void* pAlloc1, void* pAlloc2);               // 0x00446360
	virtual ~CCmgRefCounted();                                  // slot 0

	uint8  m_pad_04[0x208 - 0x04];
	int32  m_nRefCount;                                         // 0x208
	uint8  m_pad_20c[0x2c4 - 0x20c];
};

// The 8-byte object the factory news up at +0x4bc: a constructor that takes
// the manager and nothing else is reached.
class CCmgSub4BC
{
public:
	CCmgSub4BC(CCmgClientMgr* pClientMgr);                      // 0x00415560
	uint8 m_pad_00[8];
};

// The 8-byte object at +0x508 -- clientmgr2.cpp's CClientCSBase, the client
// half of the 133-slot base whose table is at 0x004a1b00 and whose
// GetSideName answers "CLIENT-OBJECT".  Its constructor is INLINE here (the
// factory stores the vptr and the back pointer itself at 0x00410d70), so it
// is written in-class.  Only one virtual is declared: the vtable address is
// a relocation and byte matching cannot see it, and a longer list would only add
// bodies this file does not own.
class CCmgCSBase
{
public:
	CCmgCSBase(CCmgClientMgr* pClientMgr) { m_pClientMgr = pClientMgr; }
	virtual void _v00();                                        // slot 0

	CCmgClientMgr* m_pClientMgr;                                // 0x04
};

// The object at +0x4c0, reached once and only through its table: `mov
// (%ecx),%edx; push $1; call *0x11c(%edx)`.  So the vptr is at +0x00 and the
// entry is slot 0x11c/4 = 71.  Seventy-one slots of nothing would be
// seventy-one bodies this file does not own, so the one entry is reached
// through the table by hand instead.  (name ours)
class CCmgSub4C0
{
public:
	virtual void _v000()=0;  virtual void _v004()=0;  virtual void _v008()=0;
	virtual void _v00c()=0;  virtual void _v010()=0;  virtual void _v014()=0;
	virtual void _v018()=0;  virtual void _v01c()=0;  virtual void _v020()=0;
	virtual void _v024()=0;  virtual void _v028()=0;  virtual void _v02c()=0;
	virtual void _v030()=0;  virtual void _v034()=0;  virtual void _v038()=0;
	virtual void _v03c()=0;  virtual void _v040()=0;  virtual void _v044()=0;
	virtual void _v048()=0;  virtual void _v04c()=0;  virtual void _v050()=0;
	virtual void _v054()=0;  virtual void _v058()=0;  virtual void _v05c()=0;
	virtual void _v060()=0;  virtual void _v064()=0;  virtual void _v068()=0;
	virtual void _v06c()=0;  virtual void _v070()=0;  virtual void _v074()=0;
	virtual void _v078()=0;  virtual void _v07c()=0;  virtual void _v080()=0;
	virtual void _v084()=0;  virtual void _v088()=0;  virtual void _v08c()=0;
	virtual void _v090()=0;  virtual void _v094()=0;  virtual void _v098()=0;
	virtual void _v09c()=0;  virtual void _v0a0()=0;  virtual void _v0a4()=0;
	virtual void _v0a8()=0;  virtual void _v0ac()=0;  virtual void _v0b0()=0;
	virtual void _v0b4()=0;  virtual void _v0b8()=0;  virtual void _v0bc()=0;
	virtual void _v0c0()=0;  virtual void _v0c4()=0;  virtual void _v0c8()=0;
	virtual void _v0cc()=0;  virtual void _v0d0()=0;  virtual void _v0d4()=0;
	virtual void _v0d8()=0;  virtual void _v0dc()=0;  virtual void _v0e0()=0;
	virtual void _v0e4()=0;  virtual void _v0e8()=0;  virtual void _v0ec()=0;
	virtual void _v0f0()=0;  virtual void _v0f4()=0;  virtual void _v0f8()=0;
	virtual void _v0fc()=0;  virtual void _v100()=0;  virtual void _v104()=0;
	virtual void _v108()=0;  virtual void _v10c()=0;  virtual void _v110()=0;
	virtual void _v114()=0;  virtual void _v118()=0;
	virtual void Shutdown(int nFlag)=0;                         // 0x11c
};


// ======================================================================= //
//  2.  THE MEMBER VIEWS
//
//  One class per member the constructor or the destructor reaches, because
//  the SEQUENCE of calls this file is scored on is exactly "the members in
//  declaration order, then reversed".  Sizes are the gaps between adjacent
//  reached offsets; where a gap is bigger than the object needs, the slack
//  is padding inside the view and says so.
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  The collection members.  All five instantiations have a vptr at +0x00,
//  the array at +0x04 and two counts at +0x08/+0x0c, which is src/engine/
//  server/collection.h's CMoArray<T> (cltnew.cpp pinned the same shape at
//  CPacket+0x50).  Three have an out-of-line Term and two are expanded
//  inline; both expansions are written out because their bodies DIFFER, and
//  the difference is in the shipped bytes.
//
//  Each is polymorphic (one virtual standing for the GenList table), so
//  each destructor restores its vptr on the way out (`movl $0x4a1b34,
//  0x4(%ebx)` and friends).  That costs the factory nothing because none of
//  them is a direct member of CClientMgr: they live in the world object
//  (CCmgWorld below), whose out-of-line constructor 0x004222f0 stores them.
// ----------------------------------------------------------------------- //

// 0x004152b0 -- clientstreams.cpp reads the same three-argument shape:
// (&g_DefAlloc, p, n).  It is the deallocator the inline Term at 0x004112e5
// ends in.
void cmg_FreeArray(void* pAlloc, void* pArray, uint32 nElements);   // 0x004152b0

// The array at +0x04, inside the world object.  Its Term is INLINE: the
// shipped tail at 0x004112e5..0x00411313 is `if (m_pArray) { free; m_pArray
// = 0; m_nCache = 0; } m_nElements = 0;` -- the null arm jumps straight to
// the LAST store, which is what puts m_nElements outside the if.
class CCmgArray04
{
public:
	~CCmgArray04()
	{
		if (m_pArray)
		{
			cmg_FreeArray(&g_DefAlloc, m_pArray, m_nElements + m_nCache);
			m_pArray = LTNULL;
			m_nCache = 0;
		}

		m_nElements = 0;
	}

	virtual void _g00();                                        // 0x00 vptr
	void*  m_pArray;                                            // 0x04
	uint32 m_nElements;                                         // 0x08
	uint32 m_nCache;                                            // 0x0c
	// FIFTH DWORD, and it was missing. This is collection.h's
	// CMoArray<T,C> shape exactly -- vptr, buffer, count, cache size,
	// WANTED cache -- which is 0x14, not 0x10. Without it every field of
	// CCmgClientMgr after m_World sat four bytes low, which is what the
	// offsetof audit caught at the very first composite.
	uint32 m_nWantedCache;                                      // 0x10
};

// The array at +0x18.  Also inline, and NOT the same body: the deallocation
// is a __thiscall member (0x00413050, which cltnew.cpp already reads as
// "thiscall, (pAlloc, nElements)"), and only m_nElements is cleared
// afterwards -- so the pointer is cleared by the callee.
class CCmgArray18
{
public:
	void Free(void* pAlloc, uint32 nElements);                  // 0x00413050

	~CCmgArray18()
	{
		if (m_pArray)
			Free(&g_DefAlloc, m_nElements + m_nCache);

		m_nElements = 0;
	}

	virtual void _g00();                                        // 0x00 vptr
	void*  m_pArray;                                            // 0x04
	uint32 m_nElements;                                         // 0x08
	uint32 m_nCache;                                            // 0x0c
	// FIFTH DWORD -- the same omission as CCmgArray04. The
	// DECLARE_CMG_ARRAY macro below already uses 0x14; only the two
	// hand-written classes were short.
	uint32 m_nWantedCache;                                      // 0x10
};

// The five with an out-of-line resize.  They ARE clientshell.cpp's
// DECLARE_MOARRAY classes -- the same five collections of the same world
// object, which CServerMgr carries at +0x1a4 and CClientMgr at +0x00 -- so
// they are declared here with that file's names and bodies, token for token,
// and the linker folds the two TUs' copies.  Each teardown is the vptr
// restore (0x004a1b94 / 0x004a1ab8 / 0x004a1bc4 / 0x004a1bf4 / 0x004a1c24),
// then `SetSize2(0, &g_DefAlloc)` -- `push $0x4b4e88; push $0x0; call` with
// `this` in %ecx.  The one virtual stands for the collection's GenList table
// and is what makes VC6 restore the vptr in the destructor.
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

DECLARE_MOARRAY(CArray1d0);                                    // Term 0x00414fa0
DECLARE_MOARRAY(CArray1e4);                                    // Term 0x0040bfe0
DECLARE_MOARRAY(CArray1f8);                                    // Term 0x00415050
DECLARE_MOARRAY(CArray32c);                                    // Term 0x00414fa0
DECLARE_MOARRAY(CArray340);                                    // Term 0x00414fa0

typedef CArray1d0 CCmgArray2C;                                  // +0x2c
typedef CArray1e4 CCmgArray40;                                  // +0x40
typedef CArray1f8 CCmgArray54;                                  // +0x54
typedef CArray32c CCmgArray188;                                 // +0x188
typedef CArray340 CCmgArray19C;                                 // +0x19c

// The members with a plain out-of-line destructor and nothing else reached.
#define DECLARE_CMG_DTOR(cls, size)                           \
	class cls                                                 \
	{                                                         \
	public:                                                   \
		~cls();                                               \
		uint8 m_pad_00[size];                                 \
	}

DECLARE_CMG_DTOR(CCmgSub78,  0xfc - 0x78);                      // dtor 0x0048c790
DECLARE_CMG_DTOR(CCmgSubFC,  0x188 - 0xfc);                     // dtor 0x0043c6e0
DECLARE_CMG_DTOR(CCmgSub254, 0x24);                             // dtor 0x004128b0

// The members with only a constructor.
#define DECLARE_CMG_CTOR(cls, size)                           \
	class cls                                                 \
	{                                                         \
	public:                                                   \
		cls();                                                \
		uint8 m_pad_00[size];                                 \
	}

DECLARE_CMG_CTOR(CCmgSkyDef,  0x11d0 - 0x11a0);                 // ctor 0x004040e0
DECLARE_CMG_CTOR(CCmgRateTracker, 0x11f0 - 0x11e4);             // ctor 0x00461d10
DECLARE_CMG_CTOR(CCmgTexMod,  0x14);                            // ctor 0x0045f390

// The ten banks at 0x278..0x3bc, stride 0x24.  Each is torn down INLINE --
// `movl $vtbl,(%p); push $1; push %p+4; call 0x00490bd0; movl $0x4a14c8,(%p)`
// -- which is a derived-class destructor over servermgr.h's CBankBase shape
// (vptr, StructBank at +0x04, one dword).  They are members of the object
// at +0x1ec (CCmgSub1EC below), whose out-of-line constructor 0x0045a470
// stores their vptrs.  They are clientshell.cpp's CObjBankView<T> -- CServerMgr's CSub390 banks
// at +0x41c..+0x560 are these same ten objects -- and are declared here with
// that file's names and bodies, token for token, so the two TUs' copies fold:
// each teardown is the bank's own vptr, sb_FreeChunks(&m_Bank, 1), then the
// base's 0x004a14c8.
class StructBankView
{
public:
	uint32 m_StructSize;                           // +0x00
	uint32 m_Unknown04;                            // +0x04
	uint32 m_Unknown08;                            // +0x08
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                            // +0x18  intrusive list
};

void sb_FreeChunks(StructBankView* pBank, int bAll);           // 0x00490bd0

class CBankBaseView
{
public:
	virtual ~CBankBaseView() {}
	virtual void* Allocate() = 0;
	virtual void  Free(void* pObj) = 0;
	virtual void  Init() = 0;
};

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

typedef CObjBankView<CObjElem41c> CCmgBank278;
typedef CObjBankView<CObjElem440> CCmgBank29C;
typedef CObjBankView<CObjElem464> CCmgBank2C0;
typedef CObjBankView<CObjElem488> CCmgBank2E4;
typedef CObjBankView<CObjElem4ac> CCmgBank308;
typedef CObjBankView<CObjElem4d0> CCmgBank32C;
typedef CObjBankView<CObjElem4f4> CCmgBank350;
typedef CObjBankView<CObjElem518> CCmgBank374;
typedef CObjBankView<CObjElem53c> CCmgBank398;
typedef CObjBankView<CObjElem560> CCmgBank3BC;

// The bank at +0x113c is the same shape with a constructor of its own
// (0x00412bb0, which clientmgr2.cpp files with 0x00412bd0/0x00412bf0/
// 0x004151f0 as "the same family").
//
// Its table is modelled (it used to be an extern array stored by hand): the
// four slots are 0x00412ed0 (the scalar deleting destructor) and
// cmgrinit.cpp's Allocate / Free / Term, so the vptr is the compiler's and
// the table the linker builds has the shipped slots.  The constructor's
// bytes are the same either way -- one `movl $vtbl,(%ecx)` before sb_Init.
//
// The base is servermgr.h's CBankBase (vftable 0x004a14c8, three _purecall
// slots): its vptr is what 0x00412ed0 and the CClientMgr destructor
// (0x004110e4) store back after the chunks are freed.
class CCmgBankBase
{
public:
	virtual ~CCmgBankBase() {}
	virtual void _v04()=0;                                      // slot 1 Allocate
	virtual void _v08()=0;                                      // slot 2 Free
	virtual void Term()=0;                                      // slot 3
};

// Over the same base class as the ten banks above (CBankBaseView, not
// CCmgBankBase): the destructor loads 0x004a14c8 into %edi once, at this
// bank's restore (0x004110e4), and stores it from there into all ten -- one
// base table, so one symbol.
class CCmgBank113C : public CBankBaseView
{
public:
	CCmgBank113C();                                             // 0x00412bb0
	virtual ~CCmgBank113C() { sb_FreeChunks(&m_Bank, 1); }
	virtual void* Allocate();                                   // slot 1 0x00412820
	virtual void  Free(void* pObj);                             // slot 2 0x00412890
	virtual void  Init();                                       // slot 3 0x00412bd0 (Term)

	CCmgStructBank m_Bank;                                      // 0x04
	uint32         m_Unknown20;                                 // 0x20
};

// 0x00412bb0 -- object_bank.h's default ObjectBank<SharedTexture>() (the
// element is cmgrinit.cpp's SharedTextureQ46C): the derived vptr 0x004a1cf4
// ({0x00412ed0 dtor, 0x00412820 Allocate, 0x00412890 Free, 0x00412bd0 Term})
// and sb_Init(&m_Bank, 0x34, 32).  No base vptr store: nothing runs between
// the two.  Out of class, so it stays the call CClientMgr's constructor makes
// at 0x00410afc.
void sb_Init(CCmgStructBank* pBank, uint32 nStructSize, uint32 nCacheSize); // 0x00490b50

CCmgBank113C::CCmgBank113C()
{
	sb_Init(&m_Bank, 0x34, 32);
}

// The twelve-byte vector of the force control's local (LTVector's shape).
// Its constructor is INLINE, as LTVector's is, and /O2 still CALLS
// 0x00411d40 for it. With a body here, the refused copy is emitted as a
// COMDAT, and that COMDAT pairs with 0x00411d40. The factory is identical
// with a declaration only as well.
class CCmgVec3
{
public:
	CCmgVec3(float x, float y, float z) { m_x = x; m_y = y; m_z = z; } // 0x00411d40

	float m_x, m_y, m_z;
};

// 0x4c4 -- commonphysics.h's GlobalForceControl (CServerMgr has it at 0x668,
// offset by the same 0x1a4).  Its constructor is the server's one: a NAMED
// local vector (0x00410ab7 re-takes its address with `lea 0xc(%esp)` rather
// than using the constructor's %eax), handed by address to Set, which /O2
// leaves out of line here at 0x00410e20 (`this` = +0x4c4, m_Force at +0x14,
// the magnitude at +0x2c) -- then the slope store the caller makes directly
// into +0x30.
class CCmgForceControl
{
public:
	CCmgForceControl()
	{
		CCmgVec3 vForce(0.0f, -2000.0f, 0.0f);

		Set(&vForce);

		// 0xbf350481 EXACTLY, which is -0.7071f and not 1/sqrt(2)
		// (clientshell.cpp settles the same literal for CServerMgr).
		m_Unknown30 = -0.7071f;
	}

	void Set(CCmgVec3* pVec);                                   // 0x00410e20

	uint8       m_pad_00[0x30];                                 // 0x00
	float       m_Unknown30;                                    // 0x30
	// Slack to 0x508, where the CSBase pointer starts.  CServerMgr's
	// GlobalForceControl ends at its own +0x40 (servermgr.h: "the record ends
	// at 0x6a8"), which is 0x4c4 + 0x40 = 0x504 here; the last four bytes are
	// unaccounted for here; they are CClientMgr's own m_pCollisionInfo
	// (0x504), which CClientMgr::Init zeroes (0x0040fbd5).
	uint8       m_pad_34[0x504 - 0x4f8];
};

// The client's CNetMgr sub-object (clientmgr.h has it as m_NetMgr at 0x50c,
// 0x104 bytes).  Four entries are reached: the constructor, the destructor,
// a Term and the name it is given.
class CCmgNetMgr
{
public:
	CCmgNetMgr();                                               // 0x00455310
	~CCmgNetMgr();                                              // 0x00455410
	void SetName(char* pName);                                  // 0x004554d0
	void Term();                                                // 0x00455540
	// 0x00456360, __thiscall, three arguments (cchandler.cpp's SendTo).
	void SendTo(void* pMsg, uint32 nConnID, uint32 nFlags);

	uint8  m_pad_00[0xf0];
	// The application GUID CClientMgr::Init hands the game shell's
	// OnEngineInitialized (`lea 0x5fc(%ebx)`, 0x0040fbfc).  (name ours)
	LTGUID m_guidApp;                                           // 0xf0
	uint8  m_pad_100[0x104 - 0x100];
};

// ---- the write messages 0x00415810 builds -------------------------------
//
// The sound manager's own message, CSoundMgr+0x780 (= CClientMgr+0xef4):
// nettrace.cpp's ServerMessage shape.  A base with a virtual destructor
// (CAbstractIO, vftable 0x004a2b44, refcount at +0x04) and, on the derived
// class, NO user destructor and a CMoArray<uint8> at +0x50 -- which is
// exactly the teardown ~CSoundMgr expands at 0x0047f8ff..0x0047f929: the
// array's vptr 0x004a1ab8 and its SetSize2(0, &g_DefAlloc) (0x0040bfe0),
// then the base's vptr and nothing for the derived class.  (names ours)
class CCmgAbstractIO
{
public:
	CCmgAbstractIO() { m_nRefCount = 0; }
	virtual ~CCmgAbstractIO() {}

	uint32 m_nRefCount;                                         // 0x04
};

// +0x08 .. +0x34 -- the embedded LMessageImpl (vftable 0x004a2b68; its
// implicit constructor is the one store at 0x0047f6ea).  nettrace.cpp's
// CMsgWriteQ18.
class CCmgMsgWrite
{
public:
	virtual void _v00();

	uint8 m_pad_04[0x2c - 0x04];
};

class CCmgServerMessage : public CCmgAbstractIO
{
public:
	// servermsg.cpp's ServerMessage::ServerMessage (server 0x1002417f).
	// Inside ~CSoundMgr's constructor (0x0047f702..0x0047f711) the body
	// is expanded but Init (0x00459cc0) and the CMoArray constructor
	// (0x004833c0) are both CALLED.
	CCmgServerMessage()
	{
		m_Unknown3a = 1;
		m_MsgFlags  = 0;

		Init(0x1000, 0x1000);
	}

	// 0x00459cc0 -- ServerMessage::Init.
	void Init(uint16 nMaxDataLen, uint16 nGrowSize);

	// Cut back to the one id byte (0x0041597e: +0x38 then +0x34 off one
	// pointer, which is what makes it a member of this object).
	void Reset() { m_DataLen = m_Pos = 1; }

	CCmgMsgWrite    m_Write;                                    // 0x08
	uint16          m_DataLen;                                  // 0x34
	uint16          m_MaxDataLen;                               // 0x36
	uint16          m_Pos;                                      // 0x38
	uint16          m_Unknown3a;                                // 0x3a
	uint8           m_pad_3c[0x40 - 0x3c];
	uint32          m_MsgFlags;                                 // 0x40
	uint8           m_pad_44[0x50 - 0x44];
	CMoArray<uint8> m_PacketData;                               // 0x50 .. 0x64
};

// servermgr.h's CBankBase (vftable 0x004a14c8) and four banks CSoundMgr
// carries at +0x284/+0x2b8/+0x2dc/+0x300 (vftables 0x004a3610/0x004a3600/
// 0x004a35f0/0x004a35e0): each destructor is its own vptr, the chunks
// handed back, then the base's vptr.  (names ours)
//
// Each constructor is its vptr and sb_Init (0x00490b50) with the element
// size and a cache of 32 (0x0047f63a..0x0047f6d0); slot 3 is ObjectBank's
// Term, which CSoundMgr's constructor calls on all four (0x0047f7a0..).
//
// The base's three slots are _purecall in 0x004a14c8, so they are pure here;
// each bank overrides them, because each shipped table holds its own
// Allocate and Term (clientstreams.cpp's ObjectBankL10<T>, 0x00483430..
// 0x00483590) and they share only Free (0x00483470).
// (CCmgBankBase is declared above, with CCmgBank113C, which derives from it too.)

void sb_Init(CCmgStructBank* pBank, uint32 nStructSize, uint32 nCacheSize); // 0x00490b50

#define DECLARE_CMG_SNDBANK(cls, size)                        \
	class cls : public CCmgBankBase                           \
	{                                                         \
	public:                                                   \
		cls() { sb_Init(&m_Bank, size, 32); }                 \
		virtual ~cls() { sb_FreeChunks(&m_Bank, 1); }         \
		virtual void _v04();                                  \
		virtual void _v08();                                  \
		virtual void Term();                                  \
		CCmgStructBank m_Bank;                                \
		uint32         m_Unknown20;                           \
	}

DECLARE_CMG_SNDBANK(CCmgSndBank284, 0xa4);
DECLARE_CMG_SNDBANK(CCmgSndBank2B8, 0xb8);
DECLARE_CMG_SNDBANK(CCmgSndBank2DC, 0xb8);
DECLARE_CMG_SNDBANK(CCmgSndBank300, 0xd0);

// The refcounted write message msg_StartWrite hands out: cltnew.cpp's
// CPacketView (a CPacket whose storage is a real CMoArray<uint8>, so the
// /O2 grow is collection.h's Insert2) with the AddRef/Release vtable
// cchandler.cpp's CMsgQ478 has.  0x004159f0 is WriteType<uint8>, CALLED
// here, not expanded (three `call 0x4159f0`).
class CCmgMsg
{
public:
	virtual void _p00()=0;
	virtual void AddRef()=0;                                    // slot 1
	virtual void Release()=0;                                   // slot 2

	// 0x0045c230 -- smgrmisc.cpp's ServerMessage::AppendBody.
	LTBOOL AppendBody(CCmgServerMessage* pMsg);

	uint8           m_pad_04[0x34 - 0x04];
	uint16          m_DataLen;                                  // 0x34
	uint16          m_MaxDataLen;                               // 0x36
	uint16          m_Pos;                                      // 0x38
	uint8           m_pad_3a[0x40 - 0x3a];
	uint32          m_Flags;                                    // 0x40
	uint8           m_pad_44[0x50 - 0x44];
	CMoArray<uint8> m_Data;                                     // 0x50 .. 0x63

	// Declared, not defined: CPacket::WriteType<uint8> (0x004159f0) through
	// link/aliases.rsp. Out of the class, so 0x00415810's three writes stay
	// calls.
	void WriteType(uint8 val);

	// 0x0045c390 -- ServerMessage::WriteData (server 0x1002616a).  The
	// count is a uint16: the callee masks it (0x0045c3af) and this caller
	// pushes an unmasked `dec %edx` (0x0045c261).
	void WriteData(void* pData, uint16 nBytes);

	// servermessage.cpp's GetSpaceLeft, expanded at 0x0045c232.
	int GetSpaceLeft()
	{
		int nLeft = (int)m_MaxDataLen - (int)m_Pos - 7;

		if (nLeft < 0)
			nLeft = 0;

		return nLeft;
	}
};

// 0x0045c230 -- ServerMessage::AppendBody: this message gets the other's
// payload minus its id byte, if it fits.  REAL HOME: server/servermsg.cpp
// (the 0x0045c230.. servermsg run); smgrmisc.cpp carries a scratch
// ServerMessageQ468 copy.  Written here beside its one caller (0x00415942).
LTBOOL CCmgMsg::AppendBody(CCmgServerMessage* pMsg)
{
	if (GetSpaceLeft() >= pMsg->m_DataLen - 1)
	{
		WriteData(pMsg->m_PacketData.m_pArray + 1, pMsg->m_DataLen - 1);

		return LTTRUE;
	}

	return LTFALSE;
}

// 0x004159f0 -- CPacket::WriteType<uint8>: the real one (the same offsets
// from `this`), bound by link/aliases.rsp. The transcription that stood here
// was a register-allocation near (355 of 348 bytes) of a function another
// translation unit already matches, so it was a second body for one address.

// clientshell.cpp's CMsgRef.
class CCmgMsgRef
{
public:
	CCmgMsgRef(CCmgMsg* pMsg)
	{
		m_pMsg = pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();
	}

	CCmgMsgRef(const CCmgMsgRef& other)
	{
		m_pMsg = other.m_pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();
	}

	~CCmgMsgRef()
	{
		if (m_pMsg)
			m_pMsg->Release();
	}

	CCmgMsg* m_pMsg;
};

// 0x0045c080 -- a fresh write message, returned by value.
CCmgMsgRef cmg_StartWrite(uint32 nMaxSize, uint32 nSize);

// 0x004a9ed8 -- the "UpdateRate" console variable (10 in the shipped image;
// server/dsi.cpp's name).
extern int g_CV_UpdateRate;

// src/engine/lithtech/clientsoundmgr.cpp's CSoundMgr, embedded at +0x774.
// Its vtable is 0x004a356c and that file numbers the two slots this run
// reaches off iltsoundmgr.h's declaration order: +0x5c PauseSounds,
// +0x60 ResumeSounds.
//
// SIZE.  Only 0x124 bytes are claimed here, up to the first field past it
// that this run reads (0x898).  Everything from 0x898 to 0x10f0 is almost
// certainly INSIDE the same object -- the CD-audio module name at 0xff4 and
// the two "sounds playing / heard" counters at 0xe98 / 0xef0 belong with it
// -- but every one of those is reached as a displacement off the MANAGER, so
// the shipped bytes cannot tell the two readings apart and the codegen is
// identical either way.  They are declared on CClientMgr below.
// iltsoundmgr.h's ILTClientSoundMgr as the constructor sees it: one vptr
// store, 0x004a3620, before the members (0x0047f62e).  No destructor.
class CCmgSoundMgrBase
{
public:
	virtual LTRESULT PlaySound(PlaySoundInfo* pInfo, HLTSOUND& hResult)=0;
	virtual void _v04()=0;  virtual void _v08()=0;
	virtual void _v0c()=0;  virtual void _v10()=0;  virtual void _v14()=0;
	virtual void _v18()=0;  virtual void _v1c()=0;  virtual void _v20()=0;
	virtual void _v24()=0;  virtual void _v28()=0;  virtual void _v2c()=0;
	virtual void _v30()=0;  virtual void _v34()=0;  virtual void _v38()=0;
	virtual void _v3c()=0;  virtual void _v40()=0;  virtual void _v44()=0;
	virtual void _v48()=0;  virtual void _v4c()=0;  virtual void _v50()=0;
	virtual void _v54()=0;  virtual void _v58()=0;
	virtual void PauseSounds()=0;
	virtual void ResumeSounds()=0;
};

class CCmgSoundMgr : public CCmgSoundMgrBase
{
public:
	CCmgSoundMgr();                                             // 0x0047f620
	~CCmgSoundMgr();                                            // 0x0047f8e0

	void Term(LTBOOL bFull);                                    // 0x00480090
	void Update();                                              // 0x00480d40
	// 0x004816c0 -- three arguments, the last an integer count of
	// milliseconds the caller rounds out of a float.
	LTRESULT Play(void* pInfo, void* pFile, int nMilliseconds);

	// Declared and not defined: this is a MEMBER, so the class cannot be
	// abstract, and the two slots that matter are the last two.  The bodies
	// live in src/engine/lithtech/clientsoundmgr.cpp's CSoundMgr.
	// 0x004822f0, slot 0 -- ILTSoundMgr::PlaySound, written below.
	virtual LTRESULT PlaySound(PlaySoundInfo* pInfo, HLTSOUND& hResult);
	virtual void _v04();  virtual void _v08();
	virtual void _v0c();  virtual void _v10();  virtual void _v14();
	virtual void _v18();  virtual void _v1c();  virtual void _v20();
	virtual void _v24();  virtual void _v28();  virtual void _v2c();
	virtual void _v30();  virtual void _v34();  virtual void _v38();
	virtual void _v3c();  virtual void _v40();  virtual void _v44();
	virtual void _v48();  virtual void _v4c();  virtual void _v50();
	virtual void _v54();  virtual void _v58();
	virtual void PauseSounds();                                 // 0x5c
	virtual void ResumeSounds();                                // 0x60

	// 0x00480220 -- clientsoundmgr.cpp's FreeProviderList.
	void FreeProviderList(void* pList);

	// The vptr the twenty-five declarations above produce IS the object's
	// +0x00, so the fields start at +0x04.
	uint32            m_Unknown04;                              // 0x004
	uint32            m_Unknown08[0x47];                        // 0x008 .. 0x124
	// The two gates every sound entry tests, in that order.
	void*             m_pSoundDriver;                           // 0x124 (0x898)
	uint32            m_bSoundEnabled;                          // 0x128 (0x89c)
	void*             m_pProviderList;                          // 0x12c
	void*             m_hDigDriver;                             // 0x130
	uint32            m_bHandleReleased;                        // 0x134
	uint32            m_bReacquireFailed;                       // 0x138
	uint8             m_3DProvider[0x118];                      // 0x13c
	uint8             m_nNum3DVoices;                           // 0x254
	uint8             m_nNum3DVoicesWanted;                     // 0x255
	uint8             m_pad_256[0x258 - 0x256];
	void*             m_p3DVoices;                              // 0x258
	LTList            m_3DVoiceList;                            // 0x25c
	uint8             m_nNumSWVoices;                           // 0x26c
	uint8             m_nNumSWVoicesWanted;                     // 0x26d
	uint8             m_pad_26e[0x270 - 0x26e];
	void*             m_pSWVoices;                              // 0x270
	LTList            m_SWVoiceList;                            // 0x274
	CCmgSndBank284    m_Bank284;                                // 0x284
	LTList            m_SoundDataList;                          // 0x2a8
	CCmgSndBank2B8    m_Bank2b8;                                // 0x2b8
	CCmgSndBank2DC    m_Bank2dc;                                // 0x2dc
	CCmgSndBank300    m_Bank300;                                // 0x300
	// Every live sound, densely packed (clientsoundmgr.cpp's
	// m_pSoundInstances), and the count after it.
	void*             m_pSoundInstances[256];                   // 0x324
	uint32            m_nSoundsPlaying;                         // 0x724 (0xe98)
	void*             m_h3DListener;                            // 0x728
	LTVector          m_vListenerPos;                           // 0x72c
	LTVector          m_vListenerVel;                           // 0x738
	LTVector          m_vListenerUp;                            // 0x744
	LTVector          m_vListenerForward;                       // 0x750  (0,0,1)
	float             m_f75c;                                   // 0x75c  1.0f
	uint32            m_Unknown760;                             // 0x760
	LTVector          m_vUnknown764;                            // 0x764  (0,0,1)
	uint32            m_Unknown770;                             // 0x770
	uint32            m_Unknown774;                             // 0x774  1
	float             m_f778;                                   // 0x778  1.0f
	uint8             m_nSoundsHeard;                           // 0x77c (0xef0)  zero-extended
	uint8             m_pad_77d[0x780 - 0x77d];
	// 0x00415810 appends its body to the counter report, sends it on its
	// own (id 8) when it does not fit, and cuts it back to the id byte.
	CCmgServerMessage m_Packet;                                 // 0x780 (0xef4)
	uint32            m_Unknown7e4;                             // 0x7e4
	uint32            m_Unknown7e8;                             // 0x7e8
	uint32            m_Unknown7ec;                             // 0x7ec
	uint32            m_Unknown7f0;                             // 0x7f0
	uint32            m_Unknown7f4;                             // 0x7f4
	uint32            m_Unknown7f8;                             // 0x7f8
	float             m_f7fc;                                   // 0x7fc  0.1f
	float             m_f800;                                   // 0x800  1.0f
	uint32            m_Unknown804;                             // 0x804
	uint32            m_Unknown808;                             // 0x808
	uint32            m_Unknown80c;                             // 0x80c
};                                                              // 0x810

// src/engine/lithtech/clientworld.cpp's CDemoMgrView at +0x158c.  Its
// 0x0042bfa0 is that class's constructor, and this one is linked onto it -- it
// moves `this` into %eax first and leaves it there, which is exactly how
// VC6 answers an out-of-line constructor.  The back pointer at +0x0c is the
// 0x1598 store the factory makes.
class CCmgDemoMgr
{
public:
	CCmgDemoMgr();                                              // 0x0042bfa0
	~CCmgDemoMgr();                                             // 0x0042bfc0

	LTBOOL   IsRunning();                                       // 0x0042bfd0
	// 0x0042c5f0 -- the per-frame clock: it ANSWERS (0x0042c6bf `xor
	// %eax,%eax`, and the 0x0042c6d0 failure goes straight out in %eax).
	LTRESULT Sub42c5f0();                                       // 0x0042c5f0
	// 0x0042c6d0 -- the playback step (demodtx.cpp's reader; thiscall, one
	// argument).
	LTRESULT Sub42c6d0(int nFlag);                              // 0x0042c6d0
	// 0x0042c310 -- five arguments, three out-slots and a 1KB buffer, plus
	// the "the demo/counter machinery is live" flag the caller computes.
	void   GatherCounters(uint32* pOut1, uint32* pOut2, char* pBuffer,
	                      uint32* pOut3, LTBOOL bActive);

	// demodtx.cpp's field list.
	uint32         m_nFrames;                                   // 0x00
	float          m_fElapsed;                                  // 0x04
	float          m_fStartTime;                                // 0x08  -1.0f until playback starts
	CCmgClientMgr* m_pClientMgr;                                // 0x0c
	ILTStream*     m_pStream;                                   // 0x10  the demo file
	ILTStream*     m_pMemStream;                                // 0x14
	uint32         m_nMode;                                     // 0x18  1 record, 2 play
};

// 0x00489600 -- the client clock, seconds since start (ltnew2.cpp).
float cmg_GetClock();                                           // 0x00489600

// collelem.cpp's seventeen-record counter table at 0x004b3098.
// collelem.cpp defines it (g_CounterTableQ428); declared here under that
// file's names with the same four fields.
class CCounterEntryQ428
{
public:
	uint32* m_pCounter;
	char*   m_pName;
	uint32  m_nMin;
	uint32  m_nMax;
};
typedef CCounterEntryQ428 CCmgCounterRec;
extern CCounterEntryQ428 g_CounterTableQ428[17];               // 0x004b3098
#define g_CmgCounters g_CounterTableQ428

// The client's worker thread at +0x15a8.  src/engine/server/lthread.h's
// LThread laid out field for field: the OUTGOING queue at +0x2c is exactly
// the 0x15d4 the drain below walks, and the destructor 0x004445a0 is the one
// clientshell.cpp already pins as LThread's (CServerThread carries the same
// base at CServerMgr+0xac4).  The constructor is the client's own.
class CCmgThreadMessage
{
public:
	CCmgThreadMessage();                                        // 0x004441a0
	uint8 m_pad_00[0x20];       // 32 bytes -- the `sub $0x20,%esp` frame
};

class CCmgThreadQueue
{
public:
	// 0x00444400 -- LThreadQueue::GetMessage(pMsg, bWait).  LT_OK when one
	// came off, which is why the drain runs WHILE the answer is zero.
	LTRESULT GetMessage(CCmgThreadMessage* pMsg, LTBOOL bWait);

	uint8 m_pad_00[0x28];
};

class CCmgThread
{
public:
	CCmgThread();                                               // 0x00415660
	~CCmgThread();                                              // 0x004445a0

	LTBOOL   Init(CCmgClientMgr* pClientMgr);                   // 0x0047bee0
	LTRESULT Start(int nPriority);                              // 0x004445e0
	void     Term(LTBOOL bWaitForThread);                       // 0x00444710

	void*           m_pVtable;                                  // 0x00
	CCmgThreadQueue m_IncomingQueue;                            // 0x04
	CCmgThreadQueue m_OutgoingQueue;                            // 0x2c
	uint8           m_pad_54[0x160c - 0x15fc];
};

// The object at +0x1610, reached once through slot 2 of its own table.
class CCmgSub1610
{
public:
	virtual void _v00()=0;
	virtual void _v04()=0;
	virtual void Update()=0;                                    // 0x08
	// The first thing Init does with the manager it makes (0x0040fc2e).
	virtual void _v0c()=0;                                      // 0x0c
};

// The object at +0x11f0: constructed, wound down by the destructor's body
// and then destroyed.
//
// The two arrays are collection.h's CMoArray<T, DefaultCache> (collelem.cpp
// pins both: vtable 0x004a2190 holds a 4-byte T, 0x004a2160 the 0x1c-byte
// element below, whose releaser BaseDelete<T1C> is 0x0042bb60).  The rest of
// the layout is cmgsurf.cpp's CSub11F0Q428.  REAL HOME of the destructor:
// the unnamed client TU at 0x00429810..0x0042a04f (cmgsurf.cpp PART C).
class CCmgElem08
{
public:
	CCmgElem08() { m_f00 = 1.0f; m_n04 = 0; }                  // 0x0042bb4a

	float  m_f00;                                               // +0x00
	uint32 m_n04;                                               // +0x04
};

class CCmgElem1C
{
public:
	// ONE constructor with defaulted arguments. It is expanded in BaseNew
	// (0x0042bbd0) and CALLED out of line as 0x0042b580 (push 0; push 0) by
	// InternalNiceSetSize's placement new.  m_n00/m_n04 are INITIALISERS:
	// 0x0042bc19 and 0x0042b592 store them before the member array's vptr.
	CCmgElem1C(uint32 n00 = 0, uint32 n04 = 0)
		: m_n00(n00), m_n04(n04) { m_Array08.Init(0, 0x400); }  // 0x0042b580

	uint32 m_n00;                                               // +0x00
	uint32 m_n04;                                               // +0x04
	CMoArray<CCmgElem08, DefaultCache> m_Array08;               // +0x08
};

class CCmgSub11F0
{
public:
	CCmgSub11F0();                                              // 0x00429a80
	~CCmgSub11F0();                                             // 0x00429b40

	LTRESULT Flush();                                           // 0x00429ca0
	// 0x00429bc0 -- cmgsurf.cpp's CSub11F0Q428::Init (the client interface
	// and the screen rectangle); the last call CClientMgr::Init makes.
	LTRESULT Init(void* pClientDE, LTRect* pRect);

	void*                               m_pClientDE;            // 0x00
	CMoArray<uint32, DefaultCache>      m_Array04;              // 0x04
	CMoArray<CCmgElem1C, DefaultCache>  m_Array18;              // 0x18
	uint32                              m_Unknown2c;            // 0x2c
	uint32                              m_Unknown30;            // 0x30
	uint32                              m_Unknown34;            // 0x34
	LTRect                              m_rcArea;               // 0x38
	uint32                              m_nCellWidth;           // 0x48
	uint32                              m_nCellHeight;          // 0x4c
	uint32                              m_nSpacing;             // 0x50
	uint32                              m_nCols;                // 0x54
	uint32                              m_nRows;                // 0x58
	uint32                              m_nExtraX;              // 0x5c
	uint32                              m_nExtraY;              // 0x60
};

// clientmgr2.cpp's CClientShell, as far as this run reaches it.  The engine's
// own client shell, at CClientMgr+0x150c.
class CCmgClientShell
{
public:
	LTRESULT Update();                                          // 0x004139a0

	uint8    m_pad_00[0x34];
	uint32   m_nConnID;                                         // 0x34
	uint8    m_pad_38[0x7c - 0x38];
	LTBOOL   m_bWorldLoaded;                                    // 0x7c
};

// The DirectMusic manager the factory news up last (cltcursor.cpp names
// 0x0043f890 CLTDirectMusicMgr::CLTDirectMusicMgr, 139 bytes, and the
// allocation here is 0xfc bytes).
//
// 0x0043f890 is written below (section 12d).  REAL HOME: src/engine/
// lithtech/clientmusic.cpp -- it sits between the notification thread (0x0043f520)
// and Init (0x0043f920) -- but the class there is clientmusic.h's and that
// file is not this one's; the layout here is clientmusic.h's, field for
// field.  ILTDirectMusicMgr is the interface base: a pure-virtual class
// with an implicit constructor, so only the derived vptr (0x004a2500)
// survives.
class CCmgMusicMgrBase
{
public:
	virtual LTRESULT Init() = 0;                                // slot 0
};

// clientmusic.h's two-pointer lists, each constructed empty.
class CCmgMusicList
{
public:
	CCmgMusicList() { m_pHead = LTNULL; m_pTail = LTNULL; }
	void* m_pHead;
	void* m_pTail;
};

class CCmgMusicMgr : public CCmgMusicMgrBase
{
public:
	CCmgMusicMgr();                                             // 0x0043f890
	virtual LTRESULT Init();                                    // 0x0043f920

	LTBOOL        m_bInitialized;                               // 0x004
	LTBOOL        m_bLevelInitialized;                          // 0x008
	void*         m_pDirectMusic;                               // 0x00c
	void*         m_pLoader;                                    // 0x010
	void*         m_pPerformance;                               // 0x014
	void*         m_pPort;                                      // 0x018
	uint8         m_PortParams[0x40 - 0x1c];                    // 0x01c
	CCmgMusicList m_SecondaryDefs;                              // 0x040
	CCmgMusicList m_MotifDefs;                                  // 0x048
	CCmgMusicList m_BandList;                                   // 0x050
	CCmgMusicList m_Unknown58List;                              // 0x058
	CCmgMusicList m_Unknown60List;                              // 0x060
	CCmgMusicList m_PendingList;                                // 0x068
	void*         m_pQueueAfter;                                // 0x070
	CCmgMusicList m_SecondaryList;                              // 0x074
	CCmgMusicList m_MotifList;                                  // 0x07c
	uint8         m_CriticalSection[24];                        // 0x084
	int           m_nCurIntensity;                              // 0x09c
	int           m_nNumIntensities;                            // 0x0a0
	int           m_nInitialIntensity;                          // 0x0a4
	int           m_nInitialVolume;                             // 0x0a8
	int           m_nNumPChannels;                              // 0x0ac
	int           m_nNumVoices;                                 // 0x0b0
	int           m_nSynthSampleRate;                           // 0x0b4
	void*         m_aIntensities;                               // 0x0b8
	int           m_nNumTransitions;                            // 0x0bc
	void*         m_aTransitions;                               // 0x0c0
	char*         m_pUnknownC4;                                 // 0x0c4
	char*         m_pUnknownC8;                                 // 0x0c8
	uint8         m_pad_cc[0xfc - 0xcc];
};

// The manager singleton the constructor publishes (0x0043f915).
// clientmusic.cpp's name and type for it; the constructor below stores
// `this` through a cast.
class CLTDirectMusicMgr;
extern CLTDirectMusicMgr* g_pDirectMusicMgr;                    // 0x004b3fe0


// ======================================================================= //
//  3.  CClientMgr
//
//  Every offset below is a shipped displacement from this run.  The names of
//  the fields clientmgr.h / clientmgr2.cpp already have are their names; the
//  rest are ours.  Declaration order IS construction order and its reverse
//  IS the destruction order, and both are read straight off the two big
//  bodies at 0x00410a70 and 0x00410ec0.
// ======================================================================= //

// The world tree at +0x6c (clientmgr2.cpp's CWorldTreeQ2C, worldtree2.cpp's
// CWorldTree): only its code-source setter is reached here.
class CCmgWorldTree6C
{
public:
	void SetCodeSource(void* pSource);                          // 0x0048cd30

	uint8 m_pad_00[0x78 - 0x6c];
};

// The world object at +0x00 (clientshell.cpp's CWorld1a4, commonmodel.cpp's
// CServerWorldFull -- the same object CServerMgr carries at +0x1a4, 0x1ec
// bytes).  Its constructor is out of line (0x004222f0, which builds every
// member below: the collections' vptr stores are there, not in the factory
// at 0x00410a70) and its destructor is the compiler's, expanded into
// ~CClientMgr as the collections' teardown -- which is the only reason the
// members are spelled out.
class CCmgWorld
{
public:
	CCmgWorld();                                                // 0x004222f0

	uint32           m_Unknown00;                               // 0x000
	CCmgArray04      m_Array04;                                 // 0x004
	CCmgArray18      m_Array18;                                 // 0x018
	CCmgArray2C      m_Array2c;                                 // 0x02c
	CCmgArray40      m_Array40;                                 // 0x040
	CCmgArray54      m_Array54;                                 // 0x054
	uint8            m_pad_68[0x6c - 0x68];
	// clientmgr2.cpp's world tree at +0x6c; Init sets its code source.
	CCmgWorldTree6C  m_WorldTree;                               // 0x06c
	CCmgSub78        m_Sub78;                                   // 0x078
	CCmgSubFC        m_SubFc;                                   // 0x0fc
	CCmgArray188     m_Array188;                                // 0x188
	CCmgArray19C     m_Array19c;                                // 0x19c
	uint8            m_pad_1b0[0x1cc - 0x1b0];
	// Tested by 0x0042c603 before the demo clock starts.  (name ours)
	void*            m_pUnknown1cc;                             // 0x1cc
	uint8            m_pad_1d0[0x1ec - 0x1d0];
};

// The object at +0x1ec (clientshell.cpp's CSub390, CServerMgr+0x390): one
// out-of-line constructor (0x0045a470, which stores its vptr and builds the
// eleven object banks) and a compiler-written destructor, which ~CClientMgr
// expands -- the first bank's destructor called (0x004128b0), the other ten
// inline.
class CCmgSub1EC
{
public:
	CCmgSub1EC();                                               // 0x0045a470

	uint8            m_pad_00[0x68];
	CCmgSub254       m_Sub254;                                  // 0x254
	CCmgBank278      m_Bank278;                                 // 0x278
	CCmgBank29C      m_Bank29c;                                 // 0x29c
	CCmgBank2C0      m_Bank2c0;                                 // 0x2c0
	CCmgBank2E4      m_Bank2e4;                                 // 0x2e4
	CCmgBank308      m_Bank308;                                 // 0x308
	CCmgBank32C      m_Bank32c;                                 // 0x32c
	CCmgBank350      m_Bank350;                                 // 0x350
	CCmgBank374      m_Bank374;                                 // 0x374
	CCmgBank398      m_Bank398;                                 // 0x398
	CCmgBank3BC      m_Bank3bc;                                 // 0x3bc
};

class CCmgClientMgr
{
public:
	CCmgClientMgr();

	~CCmgClientMgr();                                           // 0x00410ec0

	// 0x0040f8d0
	LTRESULT Init(char** pResTrees, uint32 nResTrees, char* pCommands);
	// 0x0040fc80
	void     TermClientShell();
	// 0x0041fd30
	void     ProcessKeyEvents();
	// 0x00410670
	void     ProcessThreadMessages();
	// 0x004106c0
	LTRESULT Update();
	// 0x00410860
	void     UpdateCounters(LTBOOL bForce);
	// 0x00410960
	LTRESULT PlaySound(void* pInfo, char* pFilename, float fRadius);
	// 0x00410a10 -- the argument is never read; the one call site pushes
	// +0x1258 with `mov`/`push`, which is how VC6 passes a float that is
	// only copied.  +0x1258 IS a float (0x00415829 `flds`, 0x0042c6a2
	// `fstps`), and both callers stay identical with the float spelling.
	void     UpdateSounds(float fFrameTime);

	// Reached from this run but defined elsewhere.
	void  SetGlobals();                                         // 0x00410620
	void  Disconnect();                                         // 0x00410350
	void  HandleThreadMessage(CCmgThreadMessage* pMsg);         // 0x00410640
	void  Sub41fdd0(uint32* pCommands, int nCommands);          // 0x0041fdd0
	LTRESULT SetupError(LTRESULT dResult, ...);                 // 0x0041fc60
	void  Sub415810(CCmgNetMgr* pNetMgr, uint32 nConnID,
	                uint32* pChanges, int nChanges);            // 0x00415810
	void  Sub415bb0();                                          // 0x00415bb0
	void  Sub410500();                                          // 0x00410500
	// 0x0041fe30 -- lt2new2.cpp's CountFrame (the rate tracker tick).
	void  CountFrame();                                         // 0x0041fe30

public:
	// ---- the prefix CServerMgr carries at 0x1a4 (see the head of file) ----
	CCmgWorld        m_World;                                   // 0x0000
	CCmgSub1EC       m_Sub1ec;                                  // 0x01ec
	uint8            m_pad_3e0[0x40c - 0x3e0];
	// clientmgr2.cpp's eleven sixteen-byte records; the destructor's
	// `mov $0xb,%edi` / `add $0x10,%esi` loop is what proves both numbers.
	uint8            m_Records40c[0xb0];                        // 0x040c
	CCmgSub4BC*      m_pSub4bc;                                 // 0x04bc
	CCmgSub4C0*      m_pSub4c0;                                 // 0x04c0
	CCmgForceControl m_ForceControl;                            // 0x04c4
	// clientmgr2.cpp's m_pCollisionInfo; Init zeroes it (0x0040fbd5).
	void*            m_pCollisionInfo;                          // 0x0504
	// ---- end of the shared prefix ----

	// clientmgr2.cpp's m_pCSBase: the 8-byte "CLIENT-OBJECT" base whose
	// 133-slot table is at 0x004a1b00.  It is newed here and dfree'd by the
	// destructor -- raw, not deleted, which is what says its class has no
	// destructor.
	CCmgCSBase*      m_pCSBase;                                 // 0x0508
	CCmgNetMgr       m_NetMgr;                                  // 0x050c
	void*            m_Unknown610;                              // 0x0610  two loaded modules
	void*            m_Unknown614;                              // 0x0614
	void*            m_Unknown618;                              // 0x0618  the shell's module record
	IClientShell*    m_pClientShell;                            // 0x061c  (clientmgr.h)
	uint8            m_Flag620;                                 // 0x0620  a BYTE store
	uint8            m_pad_621[0x750 - 0x621];
	uint32           m_Unknown750[6];                           // 0x0750  six dwords, zeroed
	float            m_Scale768;                                // 0x0768  the three
	float            m_Scale76c;                                // 0x076c  1.0f
	float            m_Scale770;                                // 0x0770  stores
	CCmgSoundMgr     m_SoundMgr;                                // 0x0774 .. 0x0f84
	uint8            m_pad_f84[0xfcc - 0xf84];
	// The CD-audio module's two entries.  The pause one takes a flag, the
	// resume one takes nothing; that is what the two call sites say.
	void           (*m_pfnCDPause)(int);                        // 0x0fcc
	void           (*m_pfnCDResume)();                          // 0x0fd0
	uint8            m_pad_fd4[0xfe0 - 0xfd4];
	uint32           m_bCDAudioReady;                           // 0x0fe0
	uint32           m_nCDAudioEnabled;                         // 0x0fe4  last seen
	// CClientMgr::Init fills both from dsi (0x0040fb92 / 0x0040fb9d).
	void*            m_hMainWnd;                                // 0x0fe8
	void*            m_hInstance;                               // 0x0fec
	uint8            m_pad_ff0[0xff4 - 0xff0];
	char             m_szCDAudioDLL[0x10f4 - 0xff4];            // 0x0ff4
	// A bare LTLink, tied off by hand and drained by 0x00411340: the walk
	// there follows +0x04 round to the head and keeps NO count.
	LTLink           m_Link10f4;                                // 0x10f4
	LTList           m_List1100;                                // 0x1100
	LTList           m_List1110;                                // 0x1110
	CCmgStructBank   m_Bank1120;                                // 0x1120
	CCmgBank113C     m_Bank113c;                                // 0x113c
	// Fifteen dwords set to -1 by the destructor (`rep stos` with ecx=0xf)
	// and by cm_OnShellTerm, which is what proves the count twice.
	int32            m_Handles1160[15];                         // 0x1160
	uint16           m_Unknown119c;                             // 0x119c  a WORD store
	uint8            m_pad_119e[0x11a0 - 0x119e];
	CCmgSkyDef       m_SkyDef;                                  // 0x11a0
	int32            m_Unknown11d0;                             // 0x11d0  -1 in the init list
	int32            m_Unknown11d4;                             // 0x11d4  -1 in the init list
	struct CCmgSurfaceEffect* m_pSurfaceEffects;                // 0x11d8  0x0041fc46
	void*            m_pUnknown11dc;                            // 0x11dc  freed by 0x00429080
	uint32           m_Unknown11e0;                             // 0x11e0
	CCmgRateTracker  m_RateTracker;                             // 0x11e4
	CCmgSub11F0      m_Sub11f0;                                 // 0x11f0
	float            m_fPrevTime;                               // 0x1254  (demodtx.cpp)
	// A FLOAT: 0x00415829 `flds 0x1258` / `fadds 0x14f4`.  (name ours)
	float            m_fFrameTime;                              // 0x1258  handed to UpdateSounds
	float            m_fCurTime;                                // 0x125c  GetTime's field
	uint32           m_Unknown1260;                             // 0x1260
	void*            m_pChain1264;                              // 0x1264  (clientmgr2.cpp)
	uint8            m_pad_1268[0x12ac - 0x1268];
	// Two banks of 255 on/off bytes, one per command, indexed first by
	// m_nCommandBank (0x0041fdff: `shl $8; sub` is the *255 stride).
	uint8            m_CommandStates[2][255];                   // 0x12ac
	uint8            m_pad_14aa[0x14ac - 0x14aa];
	uint32           m_nCommandBank;                            // 0x14ac
	// What 0x00415810 last sent: the first 0x40 bytes of the change list,
	// how many, the time since, and the rate byte.  (names ours)
	uint8            m_LastChanges[0x40];                       // 0x14b0
	uint32           m_Unknown14f0;                             // 0x14f0  the count
	float            m_Unknown14f4;                             // 0x14f4  time since the send
	uint8            m_nLastRate;                               // 0x14f8
	uint8            m_pad_14f9[0x14fc - 0x14f9];
	// Three dwords CClientMgr::Init zeroes.  (names ours)
	uint32           m_Unknown14fc;                             // 0x14fc
	uint32           m_Unknown1500;                             // 0x1500
	uint32           m_Unknown1504;                             // 0x1504
	// clientmgr.h: a table of plain function pointers, not an object; the
	// destructor calls its second entry with the table itself.
	void**           m_pInput;                                  // 0x1508
	CCmgClientShell* m_pShell;                                  // 0x150c  (clientmgr.h)
	// MISSING FOUR BYTES: m_pShell ends at 0x1510 and the next documented
	// field is 0x1514. Caught by the offsetof audit.
	uint8            m_pad_1510[0x1514 - 0x1510];
	uint32           m_Unknown1514;                             // 0x1514
	LTBOOL           m_bConsoleReady;                           // 0x1518  (clientmgr2.cpp)
	uint32           m_Unknown151c;                             // 0x151c
	uint32           m_Unknown1520;                             // 0x1520
	uint32           m_Unknown1524;                             // 0x1524
	uint32           m_Unknown1528;                             // 0x1528
	uint32           m_Unknown152c;                             // 0x152c
	uint32           m_Unknown1530;                             // 0x1530  1 in the ctor, 0 in the dtor
	CCmgFileMgr*     m_pFileMgr;                                // 0x1534
	// The resource trees Init was given: all twenty pointers are copied
	// (`mov $0x14,%ecx; rep movsl`, 0x0040f98b) and the count kept.
	char*            m_ResTrees[20];                            // 0x1538
	uint32           m_Unknown1588;                             // 0x1588  m_nResTrees
	CCmgDemoMgr      m_DemoMgr;                                 // 0x158c
	CCmgThread       m_Thread;                                  // 0x15a8
	CCmgRefCounted*  m_pRefCounted160c;                         // 0x160c
	CCmgSub1610*     m_pSub1610;                                // 0x1610
	// cursor_Create's CLTCursor (0x0040fbe0, cltcursor.cpp).
	void*            m_pCursor;                                 // 0x1614
	CCmgTexMod       m_TexMod;                                  // 0x1618
	uint8            m_pad_162c[0x21ac - 0x162c];
	// clientmgr2.cpp's m_nFrameCode; Init zeroes it with a WORD store.
	uint16           m_nFrameCode;                              // 0x21ac
	uint8            m_pad_21ae[0x21b0 - 0x21ae];
	CCmgMusicMgr*    m_pMusicMgr;                               // 0x21b0
	// sizeof == 0x21b4: the `push $0x21b4` at 0x00410a76.
};



extern CCmgClientMgr* g_pCmgClientMgr;                          // 0x004b17e0

// The two console variables the per-frame sound/CD-audio blocks watch, both
// re-read from .data after the store that latches them (an
// expression used at several sites may be REPEATED in the source rather than
// cached, and here it demonstrably is).  All of these are console
// variables: the address each is read from is the int* of a binding in
// lithtech.exe's table, whose console name is given; the C names are
// server/dsi.cpp's, which defines lithtech's copies.
extern int g_CV_MusicEnable;                                    // 0x004a9f7c  "MusicEnable"
extern int g_CV_SoundEnable;                                    // 0x004a9f80  "SoundEnable"
extern int g_CV_FullLightScale;                                 // 0x004b3288  "FullLightScale"
extern int g_CV_ClientSleepMS;                                  // 0x004b32cc  "ClientSleepMS"
extern int g_CV_DebugNumSounds;                                 // 0x004b32e4  "DebugNumSounds"


// ======================================================================= //
//  3a. 0x0040f8d0 — CClientMgr::Init, 933 bytes.  __thiscall, three
//      arguments (`ret $0xc`): the resource trees, how many, and the console
//      command string both console stages run.  Named by the string
//      "CClientMgr::Init" its RETURN_ERRORs push (0x004a83e0).
//
//  Off the bytes, in order: the render struct, the mover counter and the
//  globals; four field stores; the world tree's code source; the client
//  interface (0x00404b90); the command banks; then the trees (clamped to
//  twenty, refused when there are none, all twenty pointers copied), the
//  file manager mounts them and a mount count of zero is refused; the
//  game-shell interface (0x0042e230) whose failure is passed straight out;
//  the console and its "version" variable; input, when dsi says so; the two
//  console stages with the window handles and the NullRender warning
//  between them; the cursor, the render mode and the game shell's
//  OnEngineInitialized, whose answer is the function's; the Bink manager;
//  and the 0x11f0 object over the full screen rectangle.
//
//  The three RETURN_ERROR code strings are "57" / "27" / "28", so the code
//  is expanded before it is stringized (de_error.h's RETURN_ERROR).
// ======================================================================= //

// What Init reaches outside this run.  (names ours where not cited)
void r_InitRenderStruct(int bFullClear);                        // 0x00462110
void phys_ResetMoveCounter();                                   // 0x004451a0
CCmgSub4C0* cmg_CreateClientDE(CCmgClientMgr* pClientMgr);      // 0x00404b90
void fm_AddTrees(CCmgFileMgr* pMgr, char** pNames, int nNames,
                 int* pTypes, int* pnAdded);                    // 0x00404520
LTRESULT cmg_InitClientShellDE(CCmgClientMgr* pClientMgr);      // 0x0042e230
void cmg_InitConsole();                                         // 0x0041e890
// 0x0041b830 -- clientconsole.cpp: the console's InitLines, forwarded.
void con_Sub41b830();                                           // 0x0041b830
int  dsi_IsInputEnabled();                                      // 0x0042e580
void dsi_OpenLog();                                             // 0x0042f6c0
// clientinput.cpp's spelling: HWND and HINSTANCE, declared bare.
struct HWND__;
struct HINSTANCE__;
HWND__*      dsi_GetMainWindow();                               // 0x0042e740
HINSTANCE__* dsi_GetInstanceHandle();                           // 0x0042e730
void* cursor_Create(CCmgClientMgr* pClientMgr);                 // 0x0043eba0
void r_GetRequestedMode(RMode* pMode);                          // 0x0040f810
CCmgSub1610* cmg_CreateVideoMgr(CCmgClientMgr* pClientMgr,
                                char* pName);                   // 0x0048a770
void con_RunString(CCmgConsoleState* pState, char* pString);    // 0x0041abe0
void cm_ParseConsoleCommands(CCmgClientMgr* pClientMgr, char* pCommands,
                             int nStage);                       // 0x00411670
void cm_RunPlusCommands(CCmgClientMgr* pClientMgr, int nStage); // 0x00411600
// clientmgr2.cpp / clientconsole.cpp's name and type for 0x004b2f44.
class CClientMgrView;
extern CClientMgrView* g_pConsoleClientMgr;                     // 0x004b2f44
extern int g_CV_NullRender;                                     // 0x004b32a4  "NullRender"

// The engine version pair at +0x11d0 (clientwin.cpp's CluVersionView):
// dsi fills it (0x0042e7b0) and 0x0048a4b0 prints it "%lu.%lu".  The
// manager's constructor initialises the two dwords as plain fields.
class CCmgVersion
{
public:
	void GetString(char* pBuf, int nSize);                      // 0x0048a4b0

	uint32 m_n00;
	uint32 m_n04;
};
// clientdsi.cpp's definition, under its own names.
struct CEngineVersionQ42E;
LTRESULT dsi_GetEngineVersion(CEngineVersionQ42E* pVersion);    // 0x0042e7b0

LTRESULT CCmgClientMgr::Init(char** pResTrees, uint32 nResTrees, char* pCommands)
{
	LTRect   rcScreen;
	int      nTreesAdded;
	char     szVersion[32];
	char     szCommand[64];
	int      treeTypes[20];
	RMode    mode;
	LTRESULT dResult;

	r_InitRenderStruct(1);
	phys_ResetMoveCounter();
	SetGlobals();

	m_nLastRate    = 10;
	m_Unknown151c  = 1;
	m_Unknown1520  = 0;
	m_nFrameCode   = 0;

	m_World.m_WorldTree.SetCodeSource(&m_Sub1ec);

	m_pSub4c0 = cmg_CreateClientDE(this);

	m_Unknown1504 = 0;
	m_Unknown1500 = 0;
	m_Unknown14fc = 0;

	memset(m_CommandStates, 0, sizeof(m_CommandStates));
	m_nCommandBank = 0;

	if (nResTrees > 20)
		nResTrees = 20;

	if (nResTrees == 0)
	{
		SetupError(LT_NOGAMERESOURCES);
		cmg_Sub41fc90(this, LT_NOGAMERESOURCES | ERROR_SHUTDOWN);
		RETURN_ERROR(1, "CClientMgr::Init", LT_NOGAMERESOURCES);
	}

	memcpy(m_ResTrees, pResTrees, sizeof(m_ResTrees));
	m_Unknown1588 = nResTrees;

	fm_AddTrees(m_pFileMgr, pResTrees, nResTrees, treeTypes, &nTreesAdded);
	if (nTreesAdded == 0)
	{
		SetupError(LT_CANTLOADGAMERESOURCES, pResTrees[0]);
		cmg_Sub41fc90(this, LT_CANTLOADGAMERESOURCES);
		RETURN_ERROR(1, "CClientMgr::Init", LT_CANTLOADGAMERESOURCES);
	}

	dResult = cmg_InitClientShellDE(this);
	if (dResult != LT_OK)
	{
		cmg_Sub41fc90(this, dResult | ERROR_SHUTDOWN);
		return dResult;
	}

	cmg_InitConsole();
	con_Sub41b830();

	dsi_GetEngineVersion((CEngineVersionQ42E*)&m_Unknown11d0);
	((CCmgVersion*)&m_Unknown11d0)->GetString(szVersion, sizeof(szVersion));
	sprintf(szCommand, "version %s", szVersion);
	con_RunString(&g_CmgClientConsoleState, szCommand);

	if (dsi_IsInputEnabled())
	{
		if (!((LTBOOL (*)(void**, CCmgConsoleState*))m_pInput[0])(m_pInput, &g_CmgClientConsoleState))
		{
			cmg_Sub41fc90(this, LT_CANTINITIALIZEINPUT | ERROR_SHUTDOWN);
			RETURN_ERROR(1, "CClientMgr::Init", LT_CANTINITIALIZEINPUT);
		}
	}

	g_pConsoleClientMgr = (CClientMgrView*)this;
	cm_ParseConsoleCommands(this, pCommands, 2);
	cm_RunPlusCommands(this, 2);

	dsi_OpenLog();

	m_hMainWnd  = dsi_GetMainWindow();
	m_hInstance = dsi_GetInstanceHandle();

	if (g_CV_NullRender)
		con_ConsolePrint("Warning: NullRender is ON.");

	cm_ParseConsoleCommands(this, pCommands, 1);
	cm_RunPlusCommands(this, 1);

	m_bConsoleReady   = 1;
	m_pCollisionInfo  = LTNULL;

	m_pCursor = cursor_Create(this);

	r_GetRequestedMode(&mode);
	dResult = m_pClientShell->OnEngineInitialized(&mode, &m_NetMgr.m_guidApp);

	m_pSub1610 = cmg_CreateVideoMgr(this, "BINK");
	if (m_pSub1610)
		m_pSub1610->_v0c();

	rcScreen.Init(0, 0, mode.m_Width, mode.m_Height);
	m_Sub11f0.Init(m_pSub4c0, &rcScreen);

	return dResult;
}


// ======================================================================= //
//  3b. 0x0040fc80 — CClientMgr::TermClientShell, 196 bytes.  __thiscall,
//      no arguments; called once, from the destructor (0x00410efe).
//      REAL HOME: the CClientMgr TU that holds Init (0x0040f8d0) and
//      StartShell (0x0040fd90) -- this sits between them.
//
//  Off the bytes: clear +0x1524; walk the client console's command list
//  (head LTLink at 0x004b2f18 = g_ClientConsoleState+0x18; the rotated
//  `pCur && pCur != &head` loop that console.cpp's con_Term also has) and
//  drop every command whose +0x14 flags have bit 0; then, if the game shell
//  is up, fetch its module's delete entry through 0x00477f10 and call it on
//  the shell; then release the shell's module record (+0x618) and the two
//  modules at +0x610 / +0x614, zeroing each.
// ======================================================================= //

void CCmgClientMgr::TermClientShell()
{
	LTLink*             pCur;
	LTLink*             pNext;
	CCmgConsoleCommand* pCmd;
	void*               pCreate;
	void              (*pDelete)(IClientShell*);

	m_Unknown1524 = 0;

	pCur = g_CmgClientConsoleState.m_CommandHead.m_pNext;
	while (pCur && pCur != &g_CmgClientConsoleState.m_CommandHead)
	{
		pNext = pCur->m_pNext;

		pCmd = (CCmgConsoleCommand*)pCur->m_pData;
		if (pCmd->m_Flags & 1)
			con_RemoveCommand(&g_CmgClientConsoleState, pCmd);

		pCur = pNext;
	}

	if (m_pClientShell)
	{
		cmg_GetShellFunctions(m_Unknown618, &pCreate, (void**)&pDelete);
		pDelete(m_pClientShell);
		m_pClientShell = LTNULL;
	}

	if (m_Unknown618)
	{
		cmg_FreeShellRecord(m_Unknown618);
		m_Unknown618 = LTNULL;
	}

	if (m_Unknown610)
	{
		cmg_UnloadModule(m_Unknown610);
		m_Unknown610 = LTNULL;
	}

	if (m_Unknown614)
	{
		cmg_UnloadModule(m_Unknown614);
		m_Unknown614 = LTNULL;
	}
}


// ======================================================================= //
//  4.  0x00410670 — drain the worker thread's outgoing queue
//      76 bytes, 4 calls.
//
//  WHAT THE BYTES SAY.  `sub $0x20,%esp` is exactly sizeof(LThreadMessage) --
//  src/engine/server/lthread.h's "fixed 32-byte records" -- and the first
//  thing done with that local is 0x004441a0, which is that header's
//  LThreadMessage constructor at the client's address (`mov %ecx,%eax`, two
//  stores at +0x08 and +0x0c, `ret`: "only the first two payload dwords are
//  cleared", word for word).  The object it then asks is `this + 0x15d4`,
//  and 0x15d4 - 0x15a8 is 0x2c, which is LThread::m_OutgoingQueue.  So this
//  is the manager taking everything the thread has posted back.
//
//  0x00444400 is LThreadQueue::GetMessage(pMsg, bWait): the second argument
//  is turned into 0 or -1 with `neg`/`sbb` and handed to
//  WaitForSingleObject, and the answer is 0 or 0x3d -- LT_OK when a message
//  came off.  The literal 0 at both call sites is bWait=LTFALSE, a poll.
//
//  THE LOOP SHAPE is /O2's rotated `while`: the test is done once up front
//  (`jne` out at 0x00410696), the body is the fall-through, and the copy of
//  the test at the bottom jumps BACK (`je 0x00410698`).  One `while`, not a
//  `do`, and the two `call 0x00444400` sites are the one source call.
// ======================================================================= //

void CCmgClientMgr::ProcessThreadMessages()
{
	CCmgThreadMessage msg;

	while (m_Thread.m_OutgoingQueue.GetMessage(&msg, LTFALSE) == LT_OK)
		HandleThreadMessage(&msg);
}


// ======================================================================= //
//  5.  0x004106c0 — CClientMgr::Update
//      412 bytes, 27 calls.  Two `ret`s: the shell-error arm returns out of
//      the middle and shares nothing with the tail.
//
//  WHY THIS IS Update.  It calls 0x004139a0 -- the address clientshell.cpp
//  pins as CClientShell::Update off the shipped string "CClientShell::Update"
//  at 0x00413a75 -- through CClientMgr+0x150c, and when there is no such
//  shell it drives the GAME's shell instead through three ADJACENT
//  IClientShell slots, 0x30 / 0x34 / 0x38.  Numbered off
//  src/LT2/sdk/inc/iclientshell.h's declaration order (slot 0 is
//  ~IClientShell), those are PreUpdate, Update and PostUpdate -- the only
//  adjacent triple of no-argument voids in that header, and the numbering is
//  the same one clientmgr2.cpp uses for OnEnterWorld/OnExitWorld at 0x20/0x24
//  and clientinterface.cpp for OnPlaySound at 0x5c.
//
//  THE RETURN.  The tail answers `xor %eax,%eax` = LT_OK.  The early exit
//  does NOT set %eax: it is `push %eax; push %esi; call 0x0041fc90; add $8;
//  pop; pop; ret`, so the function answers whatever 0x0041fc90 answered.
//  That call is therefore a `return`, and 0x0041fc90 really does set %eax on
//  the paths this reconstruction can see (`xor %eax,%eax` at 0x0041fce0).
//
//  TWO GLOBALS ARE READ TWICE EACH, once for the comparison and once for the
//  branch, with a store in between (0x00410725/0x0041073a and
//  0x00410760/0x00410775).  An expression used at several sites
//  may be REPEATED in the source rather than cached in a local.  It is
//  written repeated below because that is what the loads say.
//
//  THE COMMAND DRAIN at the tail re-calls 0x0042e590 every iteration, so the
//  bound is in the condition and not hoisted; the counter is masked with
//  `and $0xffff` on the way into 0x0042e5b0 and compared with `cmp %ax,%si`,
//  so it is a uint16 against clientdsi.cpp's uint16 count at 0x004b11e4.
// ======================================================================= //

LTRESULT CCmgClientMgr::Update()
{
	LTRESULT dResult;
	uint16   i;

	ProcessThreadMessages();

	if (m_pSub1610)
		m_pSub1610->Update();

	if (g_CV_FullLightScale)
	{
		m_Scale768 = 1.0f;
		m_Scale76c = 1.0f;
		m_Scale770 = 1.0f;
	}

	if (dsi_IsClientActive())
		m_DemoMgr.Sub42c5f0();

	dsi_Sub42e570((void*)g_CV_ClientSleepMS);

	// The CD-audio module, latched against its console variable.
	if (m_bCDAudioReady)
	{
		if (g_CV_MusicEnable != m_nCDAudioEnabled)
		{
			m_nCDAudioEnabled = g_CV_MusicEnable;

			// Written against the NEGATIVE because that is which arm the
			// shipped code makes the fall-through: 0x0041073f is
			// `test %eax,%eax; jne 0x410750`, so the pause is the block
			// that falls through and the resume is hosted after.
			if (!g_CV_MusicEnable)
				m_pfnCDPause(0);
			else
				m_pfnCDResume();
		}
	}

	// The sound manager, the same way.  Slots 0x5c and 0x60 of CSoundMgr are
	// clientsoundmgr.cpp's PauseSounds and ResumeSounds.
	if (m_SoundMgr.m_pSoundDriver)
	{
		if (g_CV_SoundEnable != m_SoundMgr.m_bSoundEnabled)
		{
			m_SoundMgr.m_bSoundEnabled = g_CV_SoundEnable;

			// The same polarity, and the same reason: 0x0041077c is
			// `test %eax,%eax; ... jne 0x41078b`.
			if (!g_CV_SoundEnable)
				m_SoundMgr.PauseSounds();
			else
				m_SoundMgr.ResumeSounds();
		}
	}

	if (!m_DemoMgr.IsRunning())
		ProcessKeyEvents();

	if (dsi_IsClientActive())
		UpdateCounters(LTFALSE);

	if (m_pShell)
	{
		dResult = m_pShell->Update();

		if (dResult != LT_OK)
			return cmg_Sub41fc90(this, dResult);
	}
	else if (m_pClientShell)
	{
		m_pClientShell->PreUpdate();
		m_pClientShell->Update();
		Sub415bb0();
		m_pClientShell->PostUpdate();
	}

	UpdateSounds(m_fFrameTime);

	if (m_DemoMgr.IsRunning())
	{
		for (i = 0; i < dsi_GetNumCommands(); i++)
			con_Sub41b8e0(sub_42e5b0(i));

		sub_42e5e0();
		sub_42e5f0();
	}

	return LT_OK;
}


// ======================================================================= //
//  6.  0x00410860 — gather the frame's counters and report them
//      147 bytes, 4 calls.  `ret $0x4`, one argument.
//
//  WHAT IT DOES, off the bytes: it hands the object at +0x158c four local
//  slots and a kilobyte of stack (0x0042c310, five arguments, `this` in
//  %ecx), then feeds two of the answers back into the manager (0x0041fdd0),
//  then -- only when a shell is up and its +0x7c world flag is set -- sends
//  the filled buffer to the server through the CNetMgr sub-object at +0x50c
//  with the shell's +0x34 connection id (0x00415810).  +0x7c and +0x34 are
//  the fields clientmgr2.cpp already names m_bWorldLoaded and m_nConnID on
//  CClientShell, which is what makes the send a send.
//
//  0x0042c310 sits inside the run src/engine/lithtech/clientworld.cpp names
//  from "DemoMgr: ..." and from the counter names "Malloc" / "Free" /
//  "Music" / "Sound" / "Input" / "ClientShell" / "Server" / "Render" /
//  "TagVisibleLeaves" at 0x0042bd36..0x0042bf5a, and its first act is to
//  zero two of the out-slots and check its own +0x18 mode against 2.  So the
//  kilobyte is the counter report and this is what publishes it.
//
//  THE FLAG.  `if (arg) eax = 1; else { eax = IsRunning(); if (eax) eax = 1; }`
//  -- the normalising jump to a shared `mov $0x1,%eax` is `||`, and the
//  argument that is pushed FIRST is the LAST one, so it is written inline as
//  the fifth argument rather than as a local.
//
//  THE FRAME is 0x800: two dwords at +0x4 / +0x8, then the slot at +0xc
//  that is handed to BOTH 0x0042c310 and 0x0041fdd0 -- and 0x0041fdd0 reads
//  it as an ARRAY of command ids (pCommands[i], counted by the +0x8 dword).
//  0x0042c310 refuses more than 0xfe changes ("DemoMgr: Invalid number of
//  changes", 0x0042c384), so the array is uint32[255] = 0x3fc bytes, ending
//  exactly at +0x408 where the report buffer starts; the buffer takes the
//  remaining 0x3fc to the top of the frame.
// ======================================================================= //

void CCmgClientMgr::UpdateCounters(LTBOOL bForce)
{
	uint32 nValue1, nValue2;
	uint32 aCommands[255];
	char   szReport[0x3fc];

	m_DemoMgr.GatherCounters(aCommands, &nValue2, szReport, &nValue1,
	                         bForce || m_DemoMgr.IsRunning());

	Sub41fdd0(aCommands, nValue2);

	if (m_pShell && m_pShell->m_bWorldLoaded)
		Sub415810(&m_NetMgr, m_pShell->m_nConnID, (uint32*)szReport, nValue1);
}


// ======================================================================= //
//  6a. 0x00415810 -- send the frame's changes to the server (466 bytes)
//
//  REAL HOME: the client TU at 0x004155c0.. (csvis.cpp writes its
//  neighbours); written here beside its one caller.  __thiscall on the
//  manager, `ret $0x10`.  It builds a fresh write message: the rate byte
//  (0x004a9ed8 clamped to 2..60, unsigned byte compares), the change count
//  and one byte per change, all through WriteType<uint8> (0x004159f0, a
//  CALL).  It sends only when something differs from what went last time
//  (the rate, more than 0x40 changes, the count, a pending body in the
//  embedded packet at +0xef4, or the first bytes of the list): then the
//  message becomes id 7, the embedded packet's body is appended (or, when it
//  does not fit, sent alone as id 8), the embedded packet is cut back to its
//  id byte, the message goes out with flag 0x80 and the list is remembered.
//  The byte copy of the list into a local array is never read; it is there
//  in the shipped bytes (0x004158d2).
//
//  THE HOLDER.  Shipped copies the returned holder into a register-held one
//  (`mov (%eax),%ebx ; AddRef`) and then releases the temporary
//  (0x00415840..0x0041585a).  `CCmgMsgRef cMsg = cmg_StartWrite(...)` is
//  constructed in place by VC6 (no AddRef/Release at all, 445 bytes); going
//  through the pointer constructor is what keeps both.  The embedded
//  message's cut-back is an inline member (Reset) -- written as two stores
//  off m_SoundMgr.m_Packet it swapped esi/edi and addressed off ebp.
// ======================================================================= //

void CCmgClientMgr::Sub415810(CCmgNetMgr* pNetMgr, uint32 nConnID,
                              uint32* pChanges, int nChanges)
{
	uint8  nRate;
	LTBOOL bRateChanged;
	uint8  aChanges[256];
	int    i, nCopy;

	m_Unknown14f4 += m_fFrameTime;

	CCmgMsgRef cMsg = cmg_StartWrite(0x1000, 0x1000).m_pMsg;

	nRate = LTCLAMP((uint8)g_CV_UpdateRate, 2, 60);
	cMsg.m_pMsg->WriteType(nRate);

	bRateChanged = (nRate != m_nLastRate);
	m_nLastRate = nRate;

	cMsg.m_pMsg->WriteType((uint8)nChanges);

	for (i = 0; i < nChanges; i++)
	{
		cMsg.m_pMsg->WriteType((uint8)pChanges[i]);
		aChanges[i] = (uint8)pChanges[i];
	}

	nCopy = LTMIN(nChanges, 0x40);

	if (bRateChanged || nChanges > 0x40 || m_Unknown14f0 != (uint32)nCopy ||
	    m_SoundMgr.m_Packet.m_DataLen - 1 > 0 || memcmp(m_LastChanges, pChanges, nCopy) != 0)
	{
		cMsg.m_pMsg->m_Data.m_pArray[0] = 7;

		if (!cMsg.m_pMsg->AppendBody(&m_SoundMgr.m_Packet))
		{
			m_SoundMgr.m_Packet.m_PacketData.m_pArray[0] = 8;
			pNetMgr->SendTo(&m_SoundMgr.m_Packet, nConnID, 0);
		}

		if (m_SoundMgr.m_Packet.m_DataLen - 1 > 0)
			m_SoundMgr.m_Packet.Reset();

		pNetMgr->SendTo(cMsg.m_pMsg, nConnID, 0x80);

		memcpy(m_LastChanges, pChanges, nCopy);
		m_Unknown14f0 = nCopy;
		m_Unknown14f4 = 0;
	}
}


// ======================================================================= //
//  6b. 0x0047f8e0 -- CSoundMgr's destructor (206 bytes)
//
//  REAL HOME: src/engine/lithtech/clientsoundmgr.cpp (the 0x0047f620..
//  CSoundMgr run); written here beside the member layout it tears down.
//  Its own vptr (0x004a356c), Term(LTTRUE) (0x00480090), the provider list
//  freed (0x00480220) and cleared, then the members in reverse: the
//  message at +0x780 (its CMoArray at +0x7d0 first, then the base vptr)
//  and the four banks +0x300/+0x2dc/+0x2b8/+0x284.  Not virtual: slot 0 of
//  0x004a356c is 0x004822f0, a two-argument method.
// ======================================================================= //

// ======================================================================= //
//  6c. 0x0042c5f0 -- the demo manager's per-frame clock (212 bytes)
//
//  REAL HOME: demodtx.cpp's CDemoMgr run (0x0042bfa0..); written here
//  beside its one caller (Update, 0x004106c0).  Playing back, it starts the
//  demo clock on the first frame the world is up (and zeroes every counter
//  of collelem.cpp's table), runs the playback step and reads the frame's
//  time off the file; otherwise the time is the clock plus the elapsed
//  offset, and recording writes a zero byte and the time.  Then the frame
//  time and the rate tracker.
// ======================================================================= //

LTRESULT CCmgDemoMgr::Sub42c5f0()
{
	LTRESULT        dResult;
	CCmgCounterRec* pRec;

	if (m_nMode == 2)
	{
		if (m_fStartTime == -1.0f && m_pClientMgr->m_World.m_pUnknown1cc)
		{
			m_fStartTime = cmg_GetClock();

			for (pRec = g_CmgCounters; pRec < &g_CmgCounters[17]; pRec++)
				*pRec->m_pCounter = 0;
		}

		dResult = Sub42c6d0(0);
		if (dResult != LT_OK)
			return dResult;

		m_pStream->Read(&m_pClientMgr->m_fCurTime, 4);
	}
	else
	{
		m_pClientMgr->m_fCurTime = cmg_GetClock() + m_fElapsed;

		if (m_nMode == 1)
		{
			m_pStream->WriteVal((uint8)0);
			m_pStream->Write(&m_pClientMgr->m_fCurTime, 4);
		}
	}

	m_pClientMgr->m_fFrameTime = m_pClientMgr->m_fCurTime - m_pClientMgr->m_fPrevTime;
	m_pClientMgr->m_fPrevTime  = m_pClientMgr->m_fCurTime;
	m_pClientMgr->CountFrame();

	return LT_OK;
}


// ======================================================================= //
//  6a. 0x0047f620 -- CSoundMgr's constructor (695 bytes)
//
//  REAL HOME: src/engine/lithtech/clientsoundmgr.cpp (the 0x0047f620..
//  CSoundMgr run).  Called from the client manager's factory (0x00410adb).
//  The interface vptr (0x004a3620), the members in declaration order (three
//  lists, four banks, the message at +0x780), CSoundMgr's own vptr
//  (0x004a356c), then the body: every field cleared, the lists re-tied, the
//  four banks Term'd, and the listener/volume defaults.
//
//  NEAR, 711 bytes against 695.  The whole residual is inline budget:
//  shipped CALLS the message's CMoArray<uint8> constructor (0x004833c0);
//  here the constructor expands one level (its vptr, then Clear and Init
//  called), and three stores schedule one slot apart as a result.  Padding
//  the body with four to six more dl_InitList-sized inlines turns it into
//  the shipped call, so the original body spends more inline budget than
//  this one (VC6 spends it from the end backwards); what it spent it on
//  is not visible in the bytes.  LTVector/Init for the listener block
//  (0x72c..0x76c) was chosen over plain stores because it spends some.
// ======================================================================= //

CCmgSoundMgr::CCmgSoundMgr()
{
	m_Unknown04 = 0;
	m_hDigDriver    = LTNULL;
	memset(m_Unknown08, 0, sizeof(m_Unknown08));
	m_pProviderList = LTNULL;
	memset(m_3DProvider, 0, sizeof(m_3DProvider));

	m_nNum3DVoices       = 0;
	m_nNum3DVoicesWanted = 0;
	m_p3DVoices          = LTNULL;
	dl_InitList(&m_3DVoiceList);

	m_nNumSWVoices       = 0;
	m_nNumSWVoicesWanted = 0;
	m_pSWVoices          = LTNULL;
	dl_InitList(&m_SWVoiceList);

	dl_InitList(&m_SoundDataList);

	m_Bank284.Term();
	m_Bank2b8.Term();
	m_Bank2dc.Term();
	m_Bank300.Term();

	m_h3DListener = LTNULL;
	m_vListenerPos.Init();
	m_vListenerVel.Init();
	m_vListenerUp.Init();
	m_vListenerForward.Init(0.0f, 0.0f, 1.0f);
	m_f75c = 1.0f;
	m_Unknown760 = 0;
	m_vUnknown764.Init(0.0f, 0.0f, 1.0f);
	m_Unknown770 = 0;
	m_Unknown774 = 1;
	m_nSoundsHeard = 0;

	memset(m_pSoundInstances, 0, sizeof(m_pSoundInstances));
	m_nSoundsPlaying = 0;

	m_Unknown7e4 = 0;
	m_f778       = 1.0f;
	m_Unknown7e8 = 0;
	m_Unknown7ec = 0;
	m_Unknown7f8 = 0;
	m_Unknown7f0 = 0;
	m_Unknown7f4 = 0;
	m_f7fc       = 0.1f;
	m_f800       = 1.0f;

	m_bHandleReleased  = 0;
	m_bReacquireFailed = 0;
	m_Unknown804 = 0;
	m_Unknown808 = 0;
	m_Unknown80c = 0;

	m_pSoundDriver  = LTNULL;
	m_bSoundEnabled = 0;
}


CCmgSoundMgr::~CCmgSoundMgr()
{
	Term(LTTRUE);

	FreeProviderList(m_pProviderList);
	m_pProviderList = LTNULL;
}


// ======================================================================= //
//  6d. 0x004822f0 -- CSoundMgr::PlaySound, vtable 0x004a356c slot 0
//      (250 bytes, `ret $0x8`)
//
//  REAL HOME: src/engine/lithtech/clientsoundmgr.cpp (the 0x00480000..
//  CSoundMgr run, whose header lists this slot as "not written here").
//  The request becomes a client sound (flags &= ~0x428, |= 0x200), its
//  name a FILE_CLIENTFILE reference looked up as type 4 (0x00404680); a
//  miss is only reported at debug level 2 and is NOT an error.  Play
//  (0x004816c0) with no duration; the new sound is the last one in the
//  instance table.
// ======================================================================= //

class CCmgFileRef
{
public:
	CCmgFileRef() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                                         // 0x00
	char*  m_pFilename;                                         // 0x04
	uint16 m_nLocalID;                                          // 0x08
};

LTRESULT CCmgSoundMgr::PlaySound(PlaySoundInfo* pInfo, HLTSOUND& hResult)
{
	CCmgFileRef fileRef;
	void*       pFile;
	LTRESULT    dResult;

	hResult = LTNULL;

	if (!pInfo)
		return LT_INVALIDPARAMS;

	if (!m_pSoundDriver || !m_bSoundEnabled || !g_pCmgClientMgr)
		return LT_ERROR;

	pInfo->m_dwFlags &= ~(PLAYSOUND_ATTACHED | PLAYSOUND_TIME | PLAYSOUND_TIMESYNC);
	pInfo->m_dwFlags |= PLAYSOUND_CLIENT;

	fileRef.m_pFilename = pInfo->m_szSoundName;
	fileRef.m_nFileType = 1;

	pFile = fm_FindFile(g_pCmgClientMgr->m_pFileMgr, (char*)&fileRef, 4);
	if (!pFile)
	{
		if (g_CmgDebugLevel >= 2)
			con_ConsolePrint("Missing sound file %s",
			                 fm_GetName(g_pCmgClientMgr->m_pFileMgr, (char*)&fileRef));

		return LT_ERROR;
	}

	dResult = Play(pInfo, pFile, 0);
	if (dResult == LT_OK)
		hResult = (HLTSOUND)m_pSoundInstances[m_nSoundsPlaying - 1];

	return dResult;
}


// ======================================================================= //
//  7.  0x00410960 — start one sound
//      175 bytes, 6 calls.  `ret $0xc`, three arguments; two `ret`s, the
//      second being the LT_OK tail after the game DLL has been told.
//
//  THE THREE ARGUMENTS, read off the frame: the first (%ebp, from 0xc(%esp)
//  behind two pushes) is null-tested and then handed BOTH to the sound
//  manager and to IClientShell's slot 0x5c; iclientshell.h's slot 0x5c is
//  `OnPlaySound(PlaySoundInfo*)`, so the first argument is the request.  The
//  second (%ebx) is a name: it goes to the file manager's lookup with the
//  literal type 4 and, when that fails, to its name formatter for the error.
//  The third is a FLOAT -- `flds 0x1c(%esp)` -- multiplied by 0x4a1afc and
//  offset by 0x4a1a74 before __ftol.  Those two .rdata dwords are
//  0x447a0000 and 0x3f000000, i.e. 1000.0f and 0.5f, so the third argument
//  is a time in seconds turned into rounded milliseconds.
//
//  THE ERROR ARM calls the name formatter BEFORE it tests the debug level
//  (0x004109a6 then 0x004109ab), so the formatter's answer is a local the
//  source computes unconditionally; a call cannot be hoisted over a load, so
//  this is source order and not scheduling.  0x004b0ab0 is the level
//  binkvideo.cpp names g_DebugLevel, tested `>= 2` exactly as ltnew1.cpp
//  records at 0x0042d760.
// ======================================================================= //

LTRESULT CCmgClientMgr::PlaySound(void* pInfo, char* pFilename, float fRadius)
{
	void*    pFile;
	char*    pName;
	LTRESULT dResult;

	if (!pInfo || !m_SoundMgr.m_pSoundDriver || !m_SoundMgr.m_bSoundEnabled)
		return LT_ERROR;

	pFile = fm_FindFile(m_pFileMgr, pFilename, 4);

	// Written against the MISS, not the hit: 0x0041099c is
	// `test %edi,%edi; jne 0x4109d3`, so the error block is the
	// fall-through and the whole sound path is hosted after it -- which is
	// also the order the four calls come out in.
	if (!pFile)
	{
		pName = fm_GetName(m_pFileMgr, pFilename);

		if (g_CmgDebugLevel >= 2)
			con_ConsolePrint("Missing sound file %s", pName);

		return LT_ERROR;
	}

	dResult = m_SoundMgr.Play(pInfo, pFile, (int)(fRadius * 1000.0f + 0.5f));

	if (dResult != LT_OK)
		return dResult;

	m_pClientShell->OnPlaySound((PlaySoundInfo*)pInfo);

	return LT_OK;
}


// ======================================================================= //
//  8.  0x00410a10 — step the sound manager, and say what it is doing
//      83 bytes, 2 calls.  `ret $0x4`.
//
//  THE ARGUMENT IS NEVER READ.  Nothing in the 83 bytes touches 0x8(%esp);
//  the `ret $0x4` is the only evidence it exists.  Its one caller is Update,
//  which passes CClientMgr+0x1258 -- the slot every LithTech client manager
//  keeps the frame time in -- so it is written as a frame time and marked
//  unused.
//
//  The report is con_PrintColored (ccclient.cpp's 0x0041b8a0: colour first,
//  then level, then the format), colour 0xff0000 and level 0.  The second
//  value is loaded with `xor %eax,%eax; mov 0xef0(%esi),%al`, so that field
//  is a uint8 and the first is a full dword.
// ======================================================================= //

void CCmgClientMgr::UpdateSounds(float /*fFrameTime*/)
{
	if (m_SoundMgr.m_pSoundDriver && m_SoundMgr.m_bSoundEnabled)
	{
		m_SoundMgr.Update();

		if (g_CV_DebugNumSounds)
		{
			con_PrintColored(0xff0000, 0, "Sounds: playing:%d, heard:%d",
			                 m_SoundMgr.m_nSoundsPlaying, m_SoundMgr.m_nSoundsHeard);
		}
	}
}


// ======================================================================= //
//  9.  0x00411340 — release a bare LTLink list of ref-counted objects
//      90 bytes, 1 call (the indirect deleting destructor).  __cdecl, one
//      argument (`mov 0x8(%esp),%esi` is past the `push %esi`).
//
//  It is an LTLink head and NOT an LTList: the walk starts at +0x04
//  (m_pNext), goes round until it comes back to the head, and the tail-off
//  at the end writes only the two link pointers -- there is no count, which
//  every LTList walk in clientmgr2.cpp has.
//
//  TWO PASSES over the same list, and they are not the same pass: the first
//  INCREMENTS each payload's +0x208, the second DECREMENTS it and deletes
//  the payload when it reaches zero (`dec` / `jne` / null test / `call
//  *(%eax)` with the flag 1 -- the scalar deleting destructor).  Taking a
//  reference on everything first is what makes it safe for a payload's
//  destructor to unlink its neighbours.
//
//  The second walk reads m_pNext BEFORE the delete (0x00411370), which is
//  the usual `pNext = pCur->m_pNext;` ahead of the free.  So does the
//  FIRST walk (0x0041134d loads +0x04 before +0x08): both passes are the
//  same while/pNext shape.  A `for (...; pCur = pCur->m_pNext)` first pass
//  loaded the link after the increment and missed by 5 bytes.
// ======================================================================= //

void cmg_ReleaseRefList(LTLink* pHead)
{
	LTLink*         pCur;
	LTLink*         pNext;
	CCmgRefCounted* pObj;

	pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		pNext = pCur->m_pNext;
		pObj  = (CCmgRefCounted*)pCur->m_pData;
		pObj->m_nRefCount++;
		pCur = pNext;
	}

	pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		pObj  = (CCmgRefCounted*)pCur->m_pData;
		pNext = pCur->m_pNext;

		if (--pObj->m_nRefCount == 0)
			delete pObj;

		pCur = pNext;
	}

	pHead->TieOff();
}


// ======================================================================= //
//  9b. 0x00411320 — the callback 0x00411320 hands 0x00484740.
//      19 bytes; NOT one of the eight rows, but nothing else names it.
// ======================================================================= //

void cmg_ReportUnfreedString(char* pString)
{
	con_Print("Unfreed string: %s", pString);
}


// ======================================================================= //
//  10. THE CONSTRUCTOR — the member-initialiser list, inlined at 0x00410a8f
//
//  Nothing but member construction, and the order is READ OFF the image:
//
//      0x00410a91  0x004222f0   m_World        (+0x0000)
//      0x00410a9c  0x0045a470   m_Sub1ec       (+0x01ec)
//      0x00410ab2  0x00411d40   m_ForceControl's local vForce (inline ctor, CALLED)
//      0x00410abe  0x00410e20   m_ForceControl.Set(&vForce), then the -0.7071f
//      0x00410ad0  0x00455310   m_NetMgr       (+0x050c)
//      0x00410adb  0x0047f620   m_SoundMgr     (+0x0774)
//      0x00410ae6  0x00410e00   m_List1100     (+0x1100)
//      0x00410af1  0x00410e00   m_List1110     (+0x1110)
//      0x00410afc  0x00412bb0   m_Bank113c     (+0x113c)
//      0x00410b07  0x004040e0   m_SkyDef       (+0x11a0)
//      0x00410b15  store -1     m_Unknown11d0  (+0x11d0)
//      0x00410b1b  store -1     m_Unknown11d4  (+0x11d4)
//      0x00410b21  0x00461d10   m_RateTracker  (+0x11e4)
//      0x00410b2c  0x00429a80   m_Sub11f0      (+0x11f0)
//      0x00410b37  0x0042bfa0   m_DemoMgr      (+0x158c)
//      0x00410b42  0x00415660   m_Thread       (+0x15a8)
//      0x00410b4d  0x0045f390   m_TexMod       (+0x1618)
//
//  That is strictly ascending, which is what a member-initialiser list gives
//  (VC6 runs it in DECLARATION order, not written order).  The two -1 stores
//  falling BETWEEN two member constructions is the proof that they are
//  initialisers and not body statements: a body store would land after every
//  constructor call, not in the middle of them.
//
//  0x00410e00 is ltlink.h's LTList::LTList -- `pHead = this + 4;
//  pHead->m_pNext = pHead; pHead->m_pPrev = pHead; m_nElements = 0` -- with
//  m_nElements at +0x00 and m_Head at +0x04, exactly as that header declares
//  it.  clientmgr2.cpp already identifies it.
//
//  Written in-class (see the class above) because the shipped factory
//  EXPANDS it rather than calling a `??0CClientMgr@@QAE@XZ`.
// ======================================================================= //

inline CCmgClientMgr::CCmgClientMgr()
	: m_Unknown11d0(-1),
	  m_Unknown11d4(-1)
{
	// The whole initialisation is the body; the factory below only news
	// the object and checks what the body made.  The body's size is what
	// makes /O2's inline budget refuse both LTList constructors
	// (0x00410ae6/0x00410af1 CALL 0x00410e00) while still expanding the
	// constructor itself into the factory: with the world object and the
	// object-bank set as the two aggregates they are (see CCmgWorld and
	// CCmgSub1EC), any shorter body expands one or both of those calls.

	// 0x00410b54 — clientmgr2.cpp's "publish the manager": g_pClientMgr and
	// the console's back pointer both get `this`.
	SetGlobals();

	m_DemoMgr.m_pClientMgr = this;

	m_pUnknown11dc = LTNULL;
	m_Unknown11e0  = 0;

	m_pMusicMgr = new CCmgMusicMgr;
	m_Flag620   = 0;

	// 0x00410b9a — file the (callback, context) pair with dsi.
	dsi_AddHandler(cmg_Handler404470, this);

	m_Unknown1530 = 1;
	m_pShell        = LTNULL;
	m_pClientShell  = LTNULL;
	m_Unknown618    = 0;
	m_Unknown610    = 0;
	m_Unknown614    = 0;

	strcpy(m_szCDAudioDLL, "cdaudio.dll");

	m_bCDAudioReady = 0;
	m_Unknown14f4   = 0;
	m_bConsoleReady = 0;

	dl_InitList(&m_List1100);
	m_Link10f4.TieOff();

	m_pChain1264 = LTNULL;

	m_Scale768 = 1.0f;
	m_Scale76c = 1.0f;
	m_Scale770 = 1.0f;

	m_Unknown750[0] = 0;
	m_Unknown750[1] = 0;
	m_Unknown750[2] = 0;
	m_Unknown750[3] = 0;
	m_Unknown750[4] = 0;
	m_Unknown750[5] = 0;

	m_pFileMgr = fm_Create(this);

	cmg_Sub45bb30(&m_Sub1ec, 1);

	sb_Init2(&m_Bank1120, 10, 0x20, 0x20);

	// The bank at +0x113c is re-initialised on top of the one its own
	// constructor made: chunks back first, then a fresh 0x34-byte pool.
	sb_FreeChunks(&m_Bank113c.m_Bank, 1);
	sb_Init2(&m_Bank113c.m_Bank, 0x34, 0x40, 0x40);

	dl_InitList(&m_List1110);

	input_GetTable(&m_pInput);

	m_NetMgr.SetName("Player");

	m_Unknown152c = 0;
	m_Unknown119c = 0;
	m_pSurfaceEffects = LTNULL;
	m_Unknown1260 = 0;
	m_fPrevTime = 0;
	m_fCurTime = 0;
	m_nCommandBank = 0;
	m_Unknown14f0 = 0;
	m_Unknown1514 = 0;
	m_Unknown151c = 0;
	m_Unknown1520 = 0;
	m_Unknown1524 = 0;
	m_Unknown1528 = 0;
	m_Unknown1588 = 0;

	m_pRefCounted160c = new CCmgRefCounted(&g_DefAlloc, &g_DefAlloc);
	m_pRefCounted160c->m_nRefCount++;

	m_pCSBase = new CCmgCSBase(this);
	m_pSub4bc = new CCmgSub4BC(this);
}


// ======================================================================= //
//  11. 0x00410a70 — make a CClientMgr
//      904 bytes, 36 calls.  __cdecl, NO arguments (%ecx is never read), and
//      it answers the manager or null.
//
//  WHY A FACTORY AND NOT A CONSTRUCTOR: the first thing it does is
//  `push $0x21b4; call 0x004290b0`, and the last thing the success arm does
//  is `mov %ebp,%eax`.  A constructor takes `this` in %ecx; this one makes
//  it.  0x21b4 is sizeof(CClientMgr) and 0x21b0 -- the last field the body
//  writes -- is the last four bytes of it.
//
//  THE FAILURE FUNNEL at 0x00410ddf is `call 0x00410ec0; push %ebp; call
//  0x004290c0` -- the destructor below, then the engine's one-argument
//  operator delete.  That is `delete pClientMgr`, and it is what makes
//  0x00410ec0 `~CClientMgr` rather than a Term (a Term would not be followed
//  by a raw free, and a *deleting* destructor would `ret $0x4`; this one is
//  a plain `ret`).
//
//  THE SINGLE NULL TEST at 0x00410a89 covers both `new`'s own guard and the
//  source's `if (!pClientMgr) return LTNULL;` -- /O2 merges them, which is
//  why the whole inlined constructor sits inside one `je` whose target is
//  the bare `return 0` epilogue and not the failure funnel.
//
//  THE FIVE `new`s all have the usual shape: `push <size>; call
//  dalloc; add $4; cmp %ebx,%eax; je L; <constructor>; jmp M; L: xor %eax,%eax;
//  M: store`.  %ebx holds zero for the whole function, which is why the tests
//  read `cmp %ebx,...` and not `test`.  Two of the five are out-of-line
//  constructors (0x0043f890 CLTDirectMusicMgr, 0x00446360 the 0x2c4-byte
//  ref-counted object, 0x00415560 the 8-byte one at +0x4bc); the fourth,
//  at 0x00410d70, is INLINE -- `movl $0x4a1b00,(%eax); mov %ebp,0x4(%eax)` --
//  and is clientmgr2.cpp's CClientCSBase.
//
//  THE STRING COPY at 0x00410bc1..0x00410bf3 is `repnz scas` + `rep movsl` +
//  `rep movsb`: VC6's INLINE strcpy (/O2 implies /Oi), not a call.  The
//  source is a plain strcpy of the literal "cdaudio.dll" (0x004a8480) into
//  the field at +0xff4.
//
//  WHAT IS IN THE CONSTRUCTOR.  Everything from 0x00410b52 to 0x00410d9f --
//  SetGlobals, the demo manager's back pointer and every store and `new`
//  after them -- is the constructor's body; the factory itself is the `new`,
//  the five-way failure test and the funnel.  The cut is the inline budget's:
//  the constructor is expanded here but both LTList constructors inside it
//  are CALLED, and with the members modelled as the aggregates they are
//  (CCmgWorld, CCmgSub1EC) no shorter body refuses both (measured: the body
//  ending after the 0x11dc/0x11e0 stores or anywhere up to the last plain
//  store leaves 904 bytes but schedules the pushes wrong, and SetGlobals
//  alone expands both LTLists, 916 bytes).
// ======================================================================= //

CCmgClientMgr* cmg_CreateClientMgr()
{
	CCmgClientMgr* pClientMgr;

	pClientMgr = new CCmgClientMgr;

	if (!pClientMgr)
		return LTNULL;

	if (!pClientMgr->m_pSub4bc ||
	    !pClientMgr->m_pRefCounted160c ||
	    !pClientMgr->m_pCSBase ||
	    !pClientMgr->m_Thread.Init(pClientMgr) ||
	    pClientMgr->m_Thread.Start(3) != LT_OK)
	{
		delete pClientMgr;
		return LTNULL;
	}

	return pClientMgr;
}


// ======================================================================= //
//  11b. Three tail-call entries the destructor makes, each a one-statement
//       body (all three are below the scorer's size floor).
//
//  0x00410500 -- 5 bytes, `jmp 0x00455060`: a CClientMgr member (the call
//  site loads `this` into %ecx) whose whole body is wmrotate.cpp's
//  music_Term.  REAL HOME: the CClientMgr TU around 0x00410350.
//  0x0041e920 -- 12 bytes, `push $0x4b2f00; call 0x0041b200; pop %ecx`:
//  console.cpp's con_Term on the client's console state.
//  0x0040bfd0 -- 5 bytes, `jmp 0x0040ee10`: cissurf.cpp's cis_Term.
// ======================================================================= //

void music_TermQ4545();                                         // 0x00455060
void con_Term(ConsoleState* pState);                            // 0x0041b200
void cis_TermQ40C();                                            // 0x0040ee10

void CCmgClientMgr::Sub410500()
{
	music_TermQ4545();
}

void cmg_Sub41e920()
{
	con_Term(&g_ClientConsoleState);
}

void cmg_Sub40bfd0()
{
	cis_TermQ40C();
}


// ======================================================================= //
//  12. 0x00410ec0 — ~CClientMgr
//      1,112 bytes, 57 calls.  __thiscall, no arguments, plain `ret`.
//
//  It is the destructor and not a Term: its only caller is the factory's
//  failure funnel (0x00410de1), which follows it with the raw one-argument
//  `operator delete`, and it ends by running twenty-one member destructors
//  in EXACT REVERSE DECLARATION ORDER --
//
//      0x004110b5  0x004445a0   m_Thread       (+0x15a8)
//      0x004110c0  0x0042bfc0   m_DemoMgr      (+0x158c)
//      0x004110c7  0x00429b40   m_Sub11f0      (+0x11f0)
//      0x004110df  0x00490bd0   m_Bank113c     (+0x113c)   inline
//      0x004110f4  0x0047f8e0   m_SoundMgr     (+0x0774)
//      0x004110ff  0x00455410   m_NetMgr       (+0x050c)
//      0x00411117  0x00490bd0   m_Bank3bc      (+0x03bc)   inline
//      0x00411135  0x00490bd0   m_Bank398      (+0x0398)   inline
//      0x00411153  0x00490bd0   m_Bank374      (+0x0374)   inline
//      0x00411171  0x00490bd0   m_Bank350      (+0x0350)   inline
//      0x0041118f  0x00490bd0   m_Bank32c      (+0x032c)   inline
//      0x004111ad  0x00490bd0   m_Bank308      (+0x0308)   inline
//      0x004111cb  0x00490bd0   m_Bank2e4      (+0x02e4)   inline
//      0x004111e9  0x00490bd0   m_Bank2c0      (+0x02c0)   inline
//      0x0041120a  0x00490bd0   m_Bank29c      (+0x029c)   inline
//      0x00411228  0x00490bd0   m_Bank278      (+0x0278)   inline
//      0x00411239  0x004128b0   m_Sub254       (+0x0254)
//      0x00411252  0x00414fa0   m_Array19c     (+0x019c)
//      0x00411269  0x00414fa0   m_Array188     (+0x0188)
//      0x00411274  0x0043c6e0   m_SubFc        (+0x00fc)
//      0x0041127c  0x0048c790   m_Sub78        (+0x0078)
//      0x00411290  0x00415050   m_Array54      (+0x0054)
//      0x004112a4  0x0040bfe0   m_Array40      (+0x0040)
//      0x004112b8  0x00414fa0   m_Array2c      (+0x002c)
//      0x004112d7  0x00413050   m_Array18      (+0x0018)   inline
//      0x004112dd  0x004152b0   m_World        (+0x0000)   inline, via m_Array04
//
//  (m_Sub254 through m_Bank3bc are CCmgSub1EC's, m_Array04 through
//  m_Array19c CCmgWorld's: two implicit destructors, expanded.)
//
//  -- which is a compiler-generated tail and nothing else.  Everything ABOVE
//  0x004110af is the destructor's own body, and it is written out below in
//  the order the calls appear.
//
//  THINGS WORTH POINTING AT IN THE BODY:
//
//    * 0x00410ef8 is IClientShell slot 0x48 = index 18 = OnEngineTerm, by
//      the same iclientshell.h count Update uses.
//    * 0x00410f0e..0x00410f1e is `mov $0xb,%edi` over a `add $0x10,%esi`
//      stride from +0x40c: eleven sixteen-byte records, which is exactly the
//      array clientmgr2.cpp declares and the same 0xb/0x10 pair it reads.
//    * 0x00410f39 is `mov 0x1508(%ebx),%eax; push %eax; call *0x4(%eax)` --
//      a table of PLAIN function pointers called with the table itself, not
//      a vtable dispatch (there is no object; the pushed argument is the
//      pointer that was loaded).  clientmgr.h already says +0x1508 is "a
//      *table of plain function pointers*".
//    * 0x00410f53 is `rep stos` with %ecx = 0xf and %eax = -1 from +0x1160:
//      the same fifteen handles cm_OnShellTerm clears, same count, same
//      address.
//    * 0x00410fda / 0x00411057 / 0x00411074 / 0x004110a5 each null the field
//      AFTER releasing it, and each release is guarded by the same null, so
//      each is one `if (p) { release; p = NULL; }`.
//    * 0x0041108e decrements the ref count and then deletes REGARDLESS of
//      the answer -- there is no `jne` on the `dec`, unlike the list release
//      at 0x00411379 which has one.  Two different idioms, both written as
//      they are.
//
//  IDENTICAL.  The tail is modelled whole -- every collection's and every
//  bank's vptr restore, and the bank base's 0x004a14c8 after each
//  sb_FreeChunks -- because the members that have vptrs sit inside the two
//  aggregates with out-of-line constructors (CCmgWorld 0x004222f0,
//  CCmgSub1EC 0x0045a470), exactly as clientshell.cpp models the same two
//  objects inside CServerMgr, so the factory's inlined constructor stores
//  none of them.  The 0x004a14c8 held in %edi across the eleven banks
//  (0x004110e4) is one base class for all of them, CBankBaseView; with
//  +0x113c over a base of its own the %edi value split in two and the
//  +0x774 / +0x11f0 member pointers swapped registers.
// ======================================================================= //

CCmgClientMgr::~CCmgClientMgr()
{
	int i;

	m_Thread.Term(LTTRUE);

	ProcessThreadMessages();

	m_Unknown1530 = 0;
	cm_ClearChain1264(this);

	if (m_pClientShell)
		m_pClientShell->OnEngineTerm();

	TermClientShell();

	for (i = 0; i < 11; i++)
		cm_Sub411c80(this, &m_Records40c[i * 0x10], 0);

	Disconnect();

	cm_RunConsoleFile(this, "autoexec.cfg");

	// The input table's second entry, handed the table.
	((void (*)(void**))m_pInput[1])(m_pInput);

	dsi_Sub42f5d0(this);

	// memset, not a counted loop -- same lever as cm_OnShellTerm; the
	// ecx/eax/edi argument order is memset's right-to-left.
	memset(m_Handles1160, 0xff, sizeof(m_Handles1160));

	m_SoundMgr.Term(LTTRUE);

	Sub410500();

	m_Sub11f0.Flush();

	cmg_ReleaseRefList(&m_Link10f4);
	cm_ClearList1100(&m_List1100);

	con_Sub41b820(1);

	m_NetMgr.Term();

	r_TermRender(this, 2);

	cmg_Sub420020(this);
	cmg_Sub41e920();

	cmg_Sub45bc70(&m_Sub1ec);

	if (m_pUnknown11dc)
	{
		dfree(m_pUnknown11dc);
		m_pUnknown11dc = LTNULL;
	}

	m_Unknown11e0 = 0;
	sb_Term(&m_Bank1120);

	cmg_Sub40bfd0();
	cmg_Sub484740(cmg_ReportUnfreedString, 0);

	if (m_pFileMgr)
		fm_Destroy(m_pFileMgr);

	errorlog_Term();

	if (m_pSub4c0)
	{
		m_pSub4c0->Shutdown(1);
		m_pSub4c0 = LTNULL;
	}

	if (m_pCSBase)
	{
		::operator delete(m_pCSBase);
		m_pCSBase = LTNULL;
	}

	if (m_pSub4bc)
	{
		::operator delete(m_pSub4bc);
		m_pSub4bc = LTNULL;
	}

	if (m_pRefCounted160c)
	{
		m_pRefCounted160c->m_nRefCount--;
		delete m_pRefCounted160c;
		m_pRefCounted160c = LTNULL;
	}
}


// ======================================================================= //
//  12b. 0x0041fdd0 — hand the shell a batch of command transitions
//       86 bytes, `ret $0x8`.  REAL HOME: the client TU at 0x0041fbe0..
//       0x0041ff90 (next to 0x0041fc90 / 0x0041fd30); written here because
//       this file is the only one that declares it.
//
//  Off the bytes: both guards jump to the same `pop %esi` (positive &&);
//  `jle` is a SIGNED count.  The `dec %ebx` down-count and the pointer
//  walking by 4 are VC6's own strength reduction of a plain indexed `for`:
//  a hand-written `do { ... p++; } while (--n)` loses the `mov %eax,%ebx`
//  copy and pushes %ebx before the guards (measured, 84 bytes vs 86).  Each command's state byte is
//  m_CommandStates[m_nCommandBank][cmd]; set -> slot 1 (OnCommandOn),
//  clear -> slot 2 (OnCommandOff).
// ======================================================================= //

void CCmgClientMgr::Sub41fdd0(uint32* pCommands, int nCommands)
{
	if (m_pClientShell && nCommands > 0)
	{
		for (int i = 0; i < nCommands; i++)
		{
			if (m_CommandStates[m_nCommandBank][pCommands[i]])
				m_pClientShell->OnCommandOn(pCommands[i]);
			else
				m_pClientShell->OnCommandOff(pCommands[i]);
		}
	}
}


// ======================================================================= //
//  12b'. 0x0041fbe0 — register a surface effect, 117 bytes, __cdecl
//        (pClientMgr, pDesc).  Reached from clientdsi.cpp's built-in table
//        loop (0x0042f5ad) and ci_AddSurfaceEffect (0x0040a64c).  REAL
//        HOME: the client TU at 0x0041fbe0..0x0041ff90, like 12b.
//
//  One allocation of the record plus the name (strlen + sizeof: the `not;
//  add $0x13` is strlen+1 plus 0x13), the three entry points copied, the
//  name copied in behind them, and the record pushed on the front of the
//  manager's list at +0x11d8.  Both exits answer zero.
// ======================================================================= //

struct CCmgSurfaceEffectDesc
{
	char* m_pName;                                              // 0x00
	void* m_pInitEffect;                                        // 0x04
	void* m_pUpdateEffect;                                      // 0x08
	void* m_pTermEffect;                                        // 0x0c
};

struct CCmgSurfaceEffect
{
	void*              m_pInitEffect;                           // 0x00
	void*              m_pUpdateEffect;                         // 0x04
	void*              m_pTermEffect;                           // 0x08
	CCmgSurfaceEffect* m_pNext;                                 // 0x0c
	char               m_Name[1];                               // 0x10
};

LTRESULT cmg_AddSurfaceEffect(CCmgClientMgr* pClientMgr, CCmgSurfaceEffectDesc* pDesc)
{
	CCmgSurfaceEffect* pEffect;

	pEffect = (CCmgSurfaceEffect*)dsi_Alloc(sizeof(CCmgSurfaceEffect) + strlen(pDesc->m_pName));
	if (pEffect)
	{
		pEffect->m_pInitEffect   = pDesc->m_pInitEffect;
		pEffect->m_pUpdateEffect = pDesc->m_pUpdateEffect;
		pEffect->m_pTermEffect   = pDesc->m_pTermEffect;
		strcpy(pEffect->m_Name, pDesc->m_pName);

		pEffect->m_pNext = pClientMgr->m_pSurfaceEffects;
		pClientMgr->m_pSurfaceEffects = pEffect;
	}

	return LT_OK;
}


// ======================================================================= //
//  12b'a. 0x0041fc60 — CClientMgr::SetupError, 35 bytes, __cdecl varargs
//         member (`this` first, caller cleans).  Formats the resource
//         message into the 0x12c-byte error string at +0x620 through
//         dsi_SetupMessage (0x0042d770) and answers ITS result: nothing sets
//         %eax after the call (server twin: smgr.cpp's SetupError).
//
//  12b'b. 0x0041fc90 — the disconnect/shutdown notifier, 148 bytes,
//         __cdecl (pClientMgr, nFlags).  0x2000000: tell the game shell
//         (IClientShell slot 0x4c, OnEvent(1, flags without the two bits)),
//         delete the engine shell at +0x150c (`push 1; call *(%eax)`, the
//         scalar deleting destructor) and report the error string through
//         0x0042e750; answers 0.  0x4000000: the same event, then
//         r_TermRender(this, 2) and 0x0042e6a0; answers 1.  Otherwise 0.
// ======================================================================= //

#include <stdarg.h>

int dsi_SetupMessage(char* pMsg, int maxLen, int messageCode,
                     va_list marker);                           // 0x0042d770
LTRESULT cmg_ReportError(char* pMsg);                           // 0x0042e750
void     cmg_ShowError(char* pMsg);                             // 0x0042e6a0

// Stand-in for the engine shell's virtual destructor (slot 0); the layout
// class above has no vtable modelled.
class CCmgDeletableShell
{
public:
	virtual ~CCmgDeletableShell();
};

LTRESULT CCmgClientMgr::SetupError(LTRESULT dResult, ...)
{
	va_list marker;

	va_start(marker, dResult);
	return dsi_SetupMessage((char*)&m_Flag620, 300, dResult, marker);
}

LTRESULT cmg_Sub41fc90(CCmgClientMgr* pClientMgr, uint32 nFlags)
{
	if (nFlags & 0x2000000)
	{
		if (pClientMgr->m_pClientShell)
			pClientMgr->m_pClientShell->OnEvent(1, nFlags & ~0x6000000);

		if (pClientMgr->m_pShell)
		{
			delete (CCmgDeletableShell*)pClientMgr->m_pShell;
			pClientMgr->m_pShell = LTNULL;
		}

		cmg_ReportError((char*)&pClientMgr->m_Flag620);
	}
	else if (nFlags & 0x4000000)
	{
		if (pClientMgr->m_pClientShell)
			pClientMgr->m_pClientShell->OnEvent(1, nFlags & ~0x6000000);

		r_TermRender(pClientMgr, 2);
		cmg_ShowError((char*)&pClientMgr->m_Flag620);
		return LT_ERROR;
	}

	return LT_OK;
}


// ======================================================================= //
//  12b''. 0x0041fd30 — CClientMgr::ProcessKeyEvents, 151 bytes, __thiscall,
//         no arguments.  Called from Update (0x004107a3) when no demo is
//         playing.  REAL HOME: the client TU at 0x0041fbe0..0x0041ff90.
//
//  Only with a game shell: every queued key-down goes to IClientShell
//  slot 0x0c (OnKeyDown(key, rep)), every key-up to slot 0x10
//  (OnKeyUp(key)), then both queues are cleared.  The index is an int
//  against the uint16 counts (`and $0xffff; cmp; jl`).
// ======================================================================= //

void CCmgClientMgr::ProcessKeyEvents()
{
	int i;

	if (m_pClientShell)
	{
		for (i = 0; i < dsi_GetNumCommands(); i++)
			m_pClientShell->OnKeyDown(sub_42e5b0(i), sub_42e5c0(i));

		for (i = 0; i < dsi_GetNumKeyUps(); i++)
			m_pClientShell->OnKeyUp(sub_42e5d0(i));

		sub_42e5e0();
		sub_42e5f0();
	}
}


// ======================================================================= //
//  12c. 0x00428fa0 — dsi_AddHandler, 42 bytes.  REAL HOME: src/engine/
//       server/dsi.cpp (clientworld.cpp's attribution block: the list head
//       0x004b2fd8 is dsi.cpp's g_pMemBlocks, whose m_pNext is at +0x08).
//       Written here because dsi.cpp is shared with server.dll and has no
//       server twin of this body to pair against.
//
//  malloc(12) through 0x00492938, both arguments stored, then prepended.
// ======================================================================= //

struct CCmgHandlerBlock
{
	void            (*m_pfn)(void*);                           // +0x00
	CCmgClientMgr*    m_pContext;                              // +0x04
	CCmgHandlerBlock* m_pNext;                                 // +0x08
};

extern CCmgHandlerBlock* g_pCmgHandlerBlocks;                   // 0x004b2fd8

void dsi_AddHandler(void (*pfn)(void*), CCmgClientMgr* pClientMgr)
{
	CCmgHandlerBlock* pBlock = (CCmgHandlerBlock*)malloc(sizeof(CCmgHandlerBlock));

	if (pBlock)
	{
		pBlock->m_pfn      = pfn;
		pBlock->m_pContext = pClientMgr;
		pBlock->m_pNext    = g_pCmgHandlerBlocks;
		g_pCmgHandlerBlocks = pBlock;
	}
}


// ======================================================================= //
//  12d. 0x0043f890 — CLTDirectMusicMgr::CLTDirectMusicMgr, 139 bytes.
//       REAL HOME clientmusic.cpp (see the class note above).
//
//  The seven lists are zeroed first (the member constructors), then the
//  vptr, then the body in shipped store order: the six head pointers, the
//  arrays and directory strings, the 64/64/44100 port defaults, the queue
//  pointer, and the singleton.
// ======================================================================= //

CCmgMusicMgr::CCmgMusicMgr()
{
	m_bInitialized      = LTFALSE;
	m_bLevelInitialized = LTFALSE;
	m_pDirectMusic      = LTNULL;
	m_pLoader           = LTNULL;
	m_pPerformance      = LTNULL;
	m_pPort             = LTNULL;
	m_aTransitions      = LTNULL;
	m_aIntensities      = LTNULL;
	m_pUnknownC4        = LTNULL;
	m_pUnknownC8        = LTNULL;
	m_nNumPChannels     = 64;
	m_nNumVoices        = 64;
	m_nSynthSampleRate  = 44100;
	m_pQueueAfter       = LTNULL;

	g_pDirectMusicMgr = (CLTDirectMusicMgr*)this;
}


// ======================================================================= //
//  12e. 0x00429b40 — ~CCmgSub11F0, 115 bytes.  Flush (0x00429ca0), then the
//       two CMoArray members' inline destructors in reverse order: +0x18
//       (vptr 0x004a2160, BaseDelete<T1C> called) and +0x04 (vptr
//       0x004a2190, BaseDelete<uint32> expanded -- its dead element count is
//       the `push %ecx` slot).
// ======================================================================= //

CCmgSub11F0::~CCmgSub11F0()
{
	Flush();
}


// ======================================================================= //
//  12f. 0x00429a80 — CCmgSub11F0::CCmgSub11F0, 191 bytes.  REAL HOME: the
//       client TU at 0x00429810..0x0042a04f (cmgsurf.cpp PART C); written
//       here beside its destructor because this file owns the view.
//
//  The two CMoArray default constructors expand inline (Clear, then Init's
//  Term / _InitArray / SetSize -- the second array's SetSize inlined, the
//  first's called), then the rect at +0x38, the client table, the 0x9e x
//  0x4e cell with spacing 2, and the four derived counts.
//
//  The rect is an LTRect MEMBER (ltrect.h), not four body stores: its
//  inline default constructor is what puts the rect stores ahead of
//  m_pClientDE (0x00429b0a), and at /O2 it spends the inline budget that
//  leaves shipped CALLING 0x00414fa0 (SetSize2<uint32>) for the first array
//  and 0x0042b9b0 (_DeleteAndDestroyArray<T1C>) in the second.  With four
//  uint32 stores in the body this was near, 220 against 191, expanded one
//  level deeper at both sites.
// ======================================================================= //

CCmgSub11F0::CCmgSub11F0()
{
	m_pClientDE  = LTNULL;
	m_nCellWidth  = 0x9e;
	m_nCellHeight = 0x4e;
	m_nSpacing    = 2;
	m_nCols      = 0;
	m_nRows      = 0;
	m_nExtraX    = 0;
	m_nExtraY    = 0;
}


// ======================================================================= //
//  12g. 0x0040bd20 -- ci_FitPolyGrid (CLTClient::FitPolyGrid), 424 bytes.
//       REAL HOME: src/engine/lithtech/clientinterface.cpp (the ci_ run,
//       between SetPolyGridTextureInfo 0x0040bc00 and 0x0040bed0); that file
//       only takes its address for the interface table, and the prototype
//       is clientinterface.h's, so defining it here links as-is.
//
//  The box's half extent and centre; the centre is the position, and the
//  scale fits the grid's width (x) and height (z) half-cells to the half
//  extent, with y measured in 1/127 (0x004a1a80) -- the byte range a
//  polygrid's heights span.
//
//  Spelling, each measured: the centre built IN PLACE with `-=`-style
//  compound operators (the `=` of the difference copies its temp into the
//  local at 0x0040bd8c, and `+=` takes *pMin by value, the second copy at
//  0x0040bdb8); a one-expression or `vHalf + *pMin` form lost both copies.
//  In the scale block the half height and the two differences are locals,
//  z before x (0x0040be26 loads the height first, 0x0040be54 the z
//  difference before the x one); every other arrangement tried was 4-9
//  mnemonic blocks off.
// ======================================================================= //

// clientmgr.h's CClientPolyGrid, cut to the fields read here.
class CCmgPolyGridView
{
public:
	uint8  m_pad_00[0xbc];
	uint8  m_ObjectType;                                        // 0xbc
	uint8  m_pad_bd[0x1f0 - 0xbd];
	uint32 m_nWidth;                                            // 0x1f0
	uint32 m_nHeight;                                           // 0x1f4
};

LTRESULT ci_FitPolyGrid(HLOCALOBJ hObj, LTVector* pMin, LTVector* pMax,
                        LTVector* pPos, LTVector* pScale)
{
	CCmgPolyGridView* pGrid = (CCmgPolyGridView*)hObj;
	LTVector          vCenter;

	if (!pGrid || pGrid->m_ObjectType != OT_POLYGRID || !pMin || !pMax)
		RETURN_ERROR(1, "CLTClient::FitPolyGrid", LT_INVALIDPARAMS);

	vCenter = *pMax - *pMin;
	vCenter *= 0.5f;
	vCenter += *pMin;

	if (pPos)
		*pPos = vCenter;

	if (pScale)
	{
		float fHalfHeight = (float)pGrid->m_nHeight * 0.5f;
		float fDZ = pMax->z - vCenter.z;
		float fDX = pMax->x - vCenter.x;

		pScale->x = fDX / ((float)pGrid->m_nWidth * 0.5f);
		pScale->z = fDZ / fHalfHeight;
		pScale->y = (pMax->y - vCenter.y) / 127.0f;
	}

	return LT_OK;
}


// ======================================================================= //
//  13. WHAT THIS RUN LEAVES OPEN
//
//    * // UNDECODED: the element TYPE of every collection and bank above.
//      Only their Term addresses and their 0x14 / 0x24 strides are proven.
//    * // UNDECODED: what 0x004b3288, 0x004b32cc and 0x004b32e4 are called.
//      Each is one dword in .data with exactly one reader in this run.
//    * // UNDECODED: whether the 0x1ec-byte-plus prefix CClientMgr shares
//      with CServerMgr is a base class, an embedded member, or a copy of the
//      same declarations.  The shipped bytes cannot tell those apart, and
//      the destructor's lack of a vptr restore at +0x00 only rules out a
//      polymorphic one.
//    * The three views this file duplicates -- CClientMgr, CClientShell and
//      CSoundMgr -- already exist under other names in
//      src/engine/lithtech/clientmgr.h, clientmgr2.cpp, clientshell.cpp and
//      clientsoundmgr.cpp.  When the fields above are folded into
//      clientmgr.h every `CCmg*` name here goes away.
// ======================================================================= //


// ======================================================================= //
//  14. THE PREDICTED CALL SEQUENCES
//
//  Read off the shipped image in ADDRESS order, with what each source line
//  above is expected to emit.  `IND` is an indirect call; those compare by
//  count and position only.
//
//  0x00410670 — 4 calls
//      0x004441a0  CCmgThreadMessage msg;
//      0x00444400  the while condition, first copy
//      0x00410640  HandleThreadMessage(&msg)
//      0x00444400  the while condition, bottom copy
//
//  0x004106c0 — 27 calls (19 direct, 8 indirect)
//      0x00410670  ProcessThreadMessages()
//      IND         m_pSub1610->Update()
//      0x0042e690  dsi_IsClientActive()
//      0x0042c5f0  m_DemoMgr.Sub42c5f0()
//      0x0042e570  dsi_Sub42e570(g_pCmgUnknown32cc)
//      IND         m_pfnCDPause(0)            (the fall-through arm)
//      IND         m_pfnCDResume()
//      IND         m_SoundMgr.PauseSounds()   (the fall-through arm)
//      IND         m_SoundMgr.ResumeSounds()
//      0x0042bfd0  m_DemoMgr.IsRunning()
//      0x0041fd30  cmg_Sub41fd30(this)
//      0x0042e690  dsi_IsClientActive()
//      0x00410860  UpdateCounters(LTFALSE)
//      0x004139a0  m_pShell->Update()
//      0x0041fc90  return cmg_Sub41fc90(this, dResult)
//      IND         m_pClientShell->PreUpdate()
//      IND         m_pClientShell->Update()
//      0x00415bb0  Sub415bb0()
//      IND         m_pClientShell->PostUpdate()
//      0x00410a10  UpdateSounds(m_Unknown1258)
//      0x0042bfd0  m_DemoMgr.IsRunning()
//      0x0042e590  the for condition, first copy
//      0x0042e5b0  dsi_GetCommand(i)
//      0x0041b8e0  con_Sub41b8e0(...)
//      0x0042e590  the for condition, bottom copy
//      0x0042e5e0  dsi_ClearCommands()
//      0x0042e5f0  dsi_ClearCommands2()
//
//  0x00410860 — 4 calls
//      0x0042bfd0  the `||`'s right operand
//      0x0042c310  m_DemoMgr.GatherCounters(...)
//      0x0041fdd0  Sub41fdd0(aCommands, nValue2)
//      0x00415810  Sub415810(&m_NetMgr, ..., szReport, nValue1)
//
//  0x00410960 — 6 calls (5 direct, 1 indirect)
//      0x00404680  fm_FindFile(m_pFileMgr, pFilename, 4)
//      0x004045d0  fm_GetName(m_pFileMgr, pFilename)
//      0x0042e6f0  con_ConsolePrint("Missing sound file %s", pName)
//      0x004928c0  __ftol, from the (int) cast
//      0x004816c0  m_SoundMgr.Play(...)
//      IND         m_pClientShell->OnPlaySound(...)
//
//  0x00410a10 — 2 calls
//      0x00480d40  m_SoundMgr.Update()
//      0x0041b8a0  con_PrintColored(...)
//
//  0x00410a70 — 36 calls
//      0x004290b0  new CCmgClientMgr           (dalloc 0x21b4)
//      0x004222f0 0x0045a470 0x00411d40 0x00410e20 0x00455310 0x0047f620
//      0x00410e00 0x00410e00 0x00412bb0 0x004040e0 0x00461d10 0x00429a80
//      0x0042bfa0 0x00415660 0x0045f390                the member list
//      0x00410620  SetGlobals()
//      0x004290b0 0x0043f890  new CCmgMusicMgr
//      0x00428fa0  dsi_AddHandler(cmg_Handler404470, pClientMgr)
//      0x00403fd0  fm_Create()
//      0x0045bb30  cmg_Sub45bb30(&m_Sub1ec, 1)
//      0x00490ba0  sb_Init2(&m_Bank1120, 10, 0x20, 0x20)
//      0x00490bd0  sb_FreeChunks(&m_Bank113c.m_Bank, 1)
//      0x00490ba0  sb_Init2(&m_Bank113c.m_Bank, 0x34, 0x40, 0x40)
//      0x00439fb0  input_GetTable(&m_pInput)
//      0x004554d0  m_NetMgr.SetName("Player")
//      0x004290b0 0x00446360  new CCmgRefCounted
//      0x004290b0             new CCmgCSBase      (its ctor is INLINE)
//      0x004290b0 0x00415560  new CCmgSub4BC
//      0x0047bee0  m_Thread.Init(pClientMgr)
//      0x004445e0  m_Thread.Start(3)
//      0x00410ec0 0x004290c0  delete pClientMgr
//
//  0x00410ec0 — 57 calls (54 direct, 3 indirect); the order is the body in
//      source order followed by the twenty-six member destructions listed in
//      the note above.
//
//  0x00411340 — 1 call
//      IND         delete pObj  (the scalar deleting destructor, flag 1)
// ======================================================================= //
