// *********************************************************************** //
//
//  MODULE:   CLU402F90 (.CPP)  -- lithtech.exe 0x00402f90 .. 0x0040488d
//
//  WHAT THIS RUN IS.  Two adjacent client-only modules that share one
//  contiguous .data literal pool (0x004a74e4 .. 0x004a779e).  The pool is
//  what names them: its literals appear in function-address order and the
//  run reads, top to bottom,
//
//      "%s%s" "-"                                (0x00402ec0, already sourced
//                                                 in src/engine/lithtech/ltnew2.cpp)
//      "Out of memory" "Running for %.1f seconds"
//      "Setting process to high priority" "breakonerror" "windowtitle"
//      "LithTech" "Unknown error initializing engine."
//      "Unable to load ltmsg.dll." "Error" "workingdir" "DebugStructBanks"
//      "LithTech build %s initialized in %.2f seconds.\n"
//      "autoexec.cfg" "config" "noinput" "-rez" "world" "host" "rb"
//      "%s%d.bmp"
//      "Lost focus... Sending LOSTFOCUS event.\n"
//      "Regained focus... Sending GAINEDFOCUS event.\n"
//      "Losing focus.. shutting down renderer.\n"
//      "Regained focus.. initializing renderer.\n"
//      "Unable to load launch.dll." "GetLithTechCommandLine" "launch.dll"
//      "launch" "Could not parse cmdfile." "r" "-cmdfile"
//                                                --- the WINDOWS CLIENT SHELL
//      "c:\\de_cache" "Unable to find server file: %s" "ShowTicks"
//                                                --- the CLIENT FILE MANAGER
//
//  So 0x00402f90..0x00403f8f is the client's WINDOWS ENTRY LAYER: WinMain,
//  the command-line parser and its -cmdfile reader, window class + main
//  message pump, WndProc, and the global ClientGlob at 0x004b0ac0 that all
//  of it writes.  In Monolith's tree this is sys/win/winclientde.cpp -- the
//  name is not in the image, so nothing below is named from it.
//
//  0x00403fd0..0x0040488d is the CLIENT FILE MANAGER, the 0xe88-byte object
//  CClientMgr keeps at +0x1534 (src/engine/lithtech/clientmgr.h already
//  declares that member as `CFileMgr* m_pFileMgr` and notes "the client
//  keeps its file manager behind a pointer; the server has one inline").
//  It is NOT the server's CFileMgr: src/engine/server/servermgr.h's is 0x3c
//  bytes with four fields, this one carries two struct-bank record pools,
//  a 100-bucket and a 200-bucket hash of LTLink chains, a string holder and
//  a "c:\de_cache" tree.  None of its strings exists in the retail server.dll
//  ("c:\\de_cache", "Unable to find server file: %s" and "ShowTicks" are all
//  absent), so it is client-only code and src/engine/server/filemgr.cpp --
//  which reconstructs the archive layer at server 0x1000xxxx -- does not
//  cover a byte of it.
//
//  ALREADY RECONSTRUCTED?  No.  Every address in the run was grepped across
//  src/engine/ and src/lithshared/: the only hits are DECLARATIONS of the
//  neighbours (clientmgr2.cpp declares sub_404480, clientinterface.cpp
//  declares sub_404610, ltnew2.cpp defines sub_00402f70 and sub_004041d0,
//  clientshell.cpp names 0x004040e0).  None of the fourteen bodies below has
//  a definition anywhere in the tree, and the server cross-check is vacuous
//  because none of these strings is in server.dll.
//
//  WHAT 0x00403dcf TURNED OUT TO BE.  Shape (a): an interior
//  arm cut off from its entry test.  The joined range
//  0x00403d60..0x00403e2e has THREE rets and 0x00403da0 looks like a
//  SEPARATE function after alignment padding; taken at
//  0x00403da0 size 143 the two remaining rets (0x00403dce, 0x00403e2d)
//  collapse into one body that ends flush at the stated end.  The function
//  is dsi_ParseCommandFile below: fopen fails, it puts up "Could not parse
//  cmdfile.", returns LTFALSE through the `ret` at 0x00403dce, and the
//  SUCCESS arm -- jumped to by the `75 16` at 0x00403db7 -- begins at
//  0x00403dcf with `push %ebx; push %edi`, which is what the enumerator read
//  as a prologue.  No padding, no shared fail block, no entry test: one
//  function, 0x00403da0..0x00403e2e, 143 bytes.
//
//  NAMING.  Everything file-scope here carries a `Clu` / `clu_` prefix or an
//  address-derived name so that nothing collides with the existing
//  src/engine/lithtech translation units, several of which already declare
//  g_hMainWnd, g_pWindowTitle, dalloc, dfree and sub_429080 at these same
//  addresses.  Relocations are masked by the verifier, so two spellings of
//  one global cost nothing.
//
//  Build /O2, like the rest of src/engine/lithtech.
//
//  ONE THING THAT IS NOT SOURCE-REACHABLE and is called out where it bites:
//  0x00402f90 and 0x004034b0..0x004034b0 aside, only TWO functions in the run
//  carry a `push %ebp; mov %esp,%ebp` frame, and both have a cause that the
//  source controls: 0x00402f90 calls setjmp (VC6 turns FPO off for any
//  function that does, because longjmp restores %ebp), and 0x00403330 has an
//  `__asm` block (src/engine/d3d/d3dbatchd.cpp also records that
//  __asm forces the frame).  Everything else in the run is frameless.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x404300).
#include <string>
#endif
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include "ltbasedefs.h"
#include "ltlink.h"
#include "avp2src_crash.h"     // debug build only: crash backtraces (src/avp2src/)

// windows.h renames engine methods out from under the SDK headers.
#undef PostMessage
#undef GetMessage
#undef PeekMessage
#undef CopyFile
#undef PlaySound
#undef GetClassName


// ======================================================================= //
//  Things this run calls but does not define
// ======================================================================= //

// The two allocator front ends, the pair-per-allocation rule that
// src/engine/lithtech/clientdsi.cpp spells out.  Which pair a line uses is
// read off the bytes: the command-line vector and the -cmdfile buffer go
// through dalloc/dfree, WinMain's argument block and the file manager's tree
// records through the inner pair.
// The outer pair IS the engine's global operator new / operator delete
// (dalloc.cpp; 0x004290b0 / 0x004290c0 are one-line forwards to the inner
// pair), so the clu_ spellings are those two called by name.
#define clu_dalloc(nSize)  operator new(nSize)               // 0x004290b0
#define clu_dfree(pData)   operator delete(pData)            // 0x004290c0
void* clu_sub_428fd0(unsigned long nSize);                   // 0x00428fd0
void  clu_sub_429080(void* pData);                           // 0x00429080

// 0x00402ec0 -- the command-line lookup worker.  ltnew2.cpp pins the
// prototype: __cdecl (pName, argc, argv), argc second, argv third, and it
// answers the NEXT entry after a hit, g_LTEmptyString when the hit is last,
// or NULL when there is no hit.
char* clu_sub_402ec0(char* pName, int argc, char** argv);    // 0x00402ec0

// 0x00402f70 -- the same lookup over the GLOBAL vector; ltnew2.cpp defines
// it as sub_00402f70.
char* clu_sub_402f70(char* pName);                           // 0x00402f70

// 0x0042e6f0 -- the CLIENT's dsi_ServerError, defined in
// src/engine/lithtech/clientdsi.cpp.  Formats into a 500-byte buffer and
// prints to the console at colour 0xffff00.
void clu_dsi_ServerError(char* pFormat, ...);                // 0x0042e6f0

// 0x0041b8c0 -- the console's white printf (colour 0xffffff, selector 0).
void clu_ConPrintf(char* pFormat, ...);                      // 0x0041b8c0

// 0x0042e6a0 -- clientmgr.h declares it; the message reporter.  WM_DESTROY
// passes a null.
// clientdsi.cpp defines it under that name.
void sub_42e6a0(char* pMsg);                                 // 0x0042e6a0
#define clu_sub_42e6a0 sub_42e6a0

// 0x0042e790 -- clientdsi.cpp defines it: MessageBoxA(g_hMainWnd, pText,
// pCaption, 0).
int clu_sub_42e790(char* pText, char* pCaption);             // 0x0042e790

// 0x0042d770 -- lt2new3.cpp pins it: the resource-DLL message formatter,
// __cdecl (pBuf, nBufSize, nMessageCode, va_list).
// servermsg.cpp defines it as dsi_SetupMessage (its answer unread here).
int dsi_SetupMessage(char* pBuf, int nBufSize, int nMessageCode, va_list args);
#define clu_sub_42d770 dsi_SetupMessage

// 0x0042d870 / 0x0042d8e0 -- engine start-up and teardown.  Init answers 0
// on success; the two error texts below say 1 means "ltmsg.dll".
int  clu_sub_42d870();                                       // 0x0042d870
void clu_sub_42d8e0();                                       // 0x0042d8e0

// 0x0042f740 / 0x0042f780 / 0x0042f7c0 / 0x0042f7d0 -- the message-record
// queue clientdsi.cpp's survey found at 0x0042f660..0x00430da7.  Create
// takes a 28-byte descriptor and copies it to the object's +0x08; the user
// pointer lives at +0x24 and 0x0042f7c0/0x0042f7d0 are its get/set.
struct CluQueueDesc
{
	int   (*m_pfnOnServerFile)(void* pQueue, char* pName, uint32 nUnused, uint32 nID);
	int   (*m_pfn04)();
	void  (*m_pfn08)();
	void  (*m_pfn0c)();
	void  (*m_pfn10)();
	void*  m_pNetMgr;
	int    m_nUnknown18;
};

void* clu_sub_42f740(CluQueueDesc* pDesc);                   // 0x0042f740
void  clu_sub_42f780(void* pQueue);                          // 0x0042f780
void* clu_sub_42f7c0(void* pQueue);                          // 0x0042f7c0
void  clu_sub_42f7d0(void* pQueue, void* pUser);             // 0x0042f7d0

// The archive/tree layer this run reaches.  0x00420b70 opens a tree by name
// and hands it back through its second argument (it clears *ppTree first);
// 0x00420cd0 closes one; 0x00420d30 answers the type word a tree opens with
// (1 for a plain directory) or 1 when there is no tree; 0x00420d50 fills a
// 0x110-byte find record for one name inside one tree.
struct CluFileInfo
{
	uint8 m_Data[0x110];
#ifdef PRIMAL_HUNT
	// PH: 0x104 more (MAX_PATH), as src/engine/server/filemgr.cpp's
	// CFileFindState: the frame below is 0x214 there (0x00404810).  AvP2
	// 1.0.9.6 keeps the 0x110 record (unguarded, its copy stops matching).
	uint8 m_DataPH[0x104];
#endif
};

int  clu_fs_OpenTree(char* pName, void** ppTree);            // 0x00420b70
void clu_fs_CloseTree(void* pTree);                          // 0x00420cd0
int  clu_fs_GetTreeType(void* pTree);                        // 0x00420d30
int  clu_fs_GetFileInfo(void* pTree, char* pName,
                        CluFileInfo* pInfo);                 // 0x00420d50

// 0x00436520 -- turn a set of open trees into the FileEntry chain
// ILTClient::GetFileList hands out (the client's own; the server's
// fs_BuildFileList is a different address entirely).
void* clu_fs_BuildFileList(void** pTrees, int nTrees, char* pDirName);

// stdlith's struct bank.  sb_Grow is the only piece of it that is a call
// here; sb_Allocate is inline in the header the client used, which is why
// the two record allocations below expand rather than call.
class CluStructBank
{
public:
	uint32 m_StructSize;                     // +0x00
	uint32 m_Unknown04;                      // +0x04
	uint32 m_GrowCount;                      // +0x08
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                      // +0x18
};

void clu_sb_Init(CluStructBank* pBank, uint32 nStructSize, uint32 nCache);
void clu_sb_Init2(CluStructBank* pBank, uint32 nStructSize, uint32 nCache,
                  uint32 nGrow);                             // 0x00490ba0
void clu_sb_FreeChunks(CluStructBank* pBank, int bAll);      // 0x00490bd0
void clu_sb_Grow(CluStructBank* pBank, uint32 nCount);       // 0x00490c20

inline void* clu_sb_Allocate(CluStructBank* pBank)
{
	void* pStruct = pBank->m_pFreeList;

	if (!pStruct)
	{
		clu_sb_Grow(pBank, pBank->m_GrowCount);

		pStruct = pBank->m_pFreeList;
		if (!pStruct)
			return LTNULL;
	}

	pBank->m_pFreeList = *(void**)pStruct;
	return pStruct;
}

// stdlith's sb_Free as servermgr.h expands it: AVP2: the record and then
// the bank are tested before the two stores.
inline void clu_sb_FreeQ501(CluStructBank* pBank, void* pStruct)
{
	if (pStruct && pBank)
	{
		*(void**)pStruct = pBank->m_pFreeList;
		pBank->m_pFreeList = pStruct;
	}
}

// The 0x18-byte string holder the file manager keeps at +0x0c: a vptr
// (0x004a3c24) over four dwords and a uint16 chunk size at +0x14.
// It is stdlith's CStringHolder (StdLith.lib), so this is that class under
// its own name, cut to the four methods used.
class CStringHolder
{
public:
	CStringHolder();                         // 0x00490f60
	~CStringHolder();                        // 0x00490fe0
	void  SetAllocSize(uint16 nSize);        // 0x00491060
	// stdlith's CStringHolder::AddString(const char*, LTBOOL) -- the
	// parameter is const there, so it is here.
	char* AddString(const char* pString, int bUnique);   // 0x00491070

	void*  m_pVTable;                        // 0x00
	void*  m_pUnknown04;                     // 0x04
	void*  m_pUnknown08;                     // 0x08
	void*  m_pUnknown0c;                     // 0x0c
	void*  m_pUnknown10;                     // 0x10
	uint16 m_nChunkSize;                     // 0x14
	uint16 m_nUnknown16;                     // 0x16
};

// The two record pools.  Same vptr-over-StructBank shape servermgr.h calls
// CBankBase: base vtable 0x004a14c8 (slots 1..3 are _purecall), derived
// 0x004a14b8 and 0x004a14a8.  ltnew2.cpp already reads the same three
// tables out of the image.
class CluBankBase
{
public:
	virtual ~CluBankBase() {}
	virtual void* Allocate() = 0;
	virtual void  Free(void* pObj) = 0;
	virtual void  Init() = 0;
};

class CluFileRecBank : public CluBankBase                    // vtable 0x004a14b8
{
public:
	CluFileRecBank() { clu_sb_Init(&m_Bank, 0x20, 0x20); }
	virtual ~CluFileRecBank() { clu_sb_FreeChunks(&m_Bank, 1); }
	virtual void* Allocate();                // 0x004049d0
	virtual void  Free(void* pObj);          // 0x004049c0
	virtual void  Init();                    // 0x00444820 (ICF)

	CluStructBank m_Bank;                    // +0x04
	uint32        m_Unknown20;               // +0x20
};

class CluNetFileRecBank : public CluBankBase                 // vtable 0x004a14a8
{
public:
	CluNetFileRecBank() { clu_sb_Init(&m_Bank, 0x28, 0x20); }
	virtual ~CluNetFileRecBank() { clu_sb_FreeChunks(&m_Bank, 1); }
	virtual void* Allocate();                // 0x004049d0 (ICF)
	virtual void  Free(void* pObj);          // 0x004049c0 (ICF)
	virtual void  Init();                    // 0x00404a00

	CluStructBank m_Bank;                    // +0x04
	uint32        m_Unknown20;               // +0x20
};

// The record banks' members: clientfilemgr.cpp (the file manager's run).

// The game DLL's interface, reached ONLY through its vtable and only from
// two slots.  Both call sites are `mov <shell>,%ecx; mov (%ecx),%edx; push
// ...; call *0xNN(%edx)` with NO caller cleanup, so these are __thiscall
// virtuals and the arguments below are exactly what each site pushes.
// The slots in between are declared only to put those two at the right
// index; nothing here claims to know what they are.
//
// AVP2: both slots sit eight bytes later than NOLF's 0x4c / 0x60, and the
// second takes two arguments, not three -- OnEvent at 0x54 (0x00403924
// `push $0; push $7; call *0x54(%edx)`) and OnMessage at 0x68 (0x00404741
// `push %esi; push $1; call *0x68(%edx)`), the same in AvP2 1.0.9.6's image
// (0x004035b4, 0x00404235 file offsets).
class CluClientShell
{
public:
	virtual void Slot00() = 0;  virtual void Slot01() = 0;
	virtual void Slot02() = 0;  virtual void Slot03() = 0;
	virtual void Slot04() = 0;  virtual void Slot05() = 0;
	virtual void Slot06() = 0;  virtual void Slot07() = 0;
	virtual void Slot08() = 0;  virtual void Slot09() = 0;
	virtual void Slot10() = 0;  virtual void Slot11() = 0;
	virtual void Slot12() = 0;  virtual void Slot13() = 0;
	virtual void Slot14() = 0;  virtual void Slot15() = 0;
	virtual void Slot16() = 0;  virtual void Slot17() = 0;
	virtual void Slot18() = 0;  virtual void Slot19() = 0;
	virtual void Slot20() = 0;
	virtual void OnEvent(int nEventCode, int nParam) = 0;    // +0x54
	virtual void Slot22() = 0;  virtual void Slot23() = 0;
	virtual void Slot24() = 0;  virtual void Slot25() = 0;
	virtual void OnMessage(int nCode, char* pName) = 0;      // +0x68
};

// The object CClientMgr keeps at +0x1614: one virtual is reached, slot
// 0x1c (index 7), with no arguments.
class CluUnknown1614
{
public:
	virtual void Slot00() = 0;  virtual void Slot01() = 0;
	virtual void Slot02() = 0;  virtual void Slot03() = 0;
	virtual void Slot04() = 0;  virtual void Slot05() = 0;
	virtual void Slot06() = 0;
	virtual void OnFocusGained() = 0;                        // +0x1c
};

// The client sound manager, reached as a SUB-OBJECT at CClientMgr+0x774 and
// as the whole of ci_GetSoundMgr's answer.  All three entries are
// __thiscall with no arguments (`mov ...,%ecx; call`, nothing pushed), so
// they are methods and not the cdecl helpers a first reading suggests.
// They are clientsoundmgr.cpp's CSoundMgrView methods under that file's
// names (each returns an LTRESULT nobody here reads), so this is that class,
// cut to the three.
class CSoundMgrView
{
public:
	uint32 sub_481e70();                     // 0x00481e70  pause
	uint32 sub_481ef0();                     // 0x00481ef0  resume
	uint32 StopAllSounds();                  // 0x00480ca0
};

// CClientMgr+0x11d0: `sprintf(buf, "%lu.%lu", this[1], this[0])` bounded by
// the caller's size.  __thiscall, two arguments.
class CluVersionView
{
public:
	void GetString(char* pBuf, int nSize);   // 0x0048a4b0
};

// The music manager ci_GetMusicMgr answers with.  +0x44 is called with ONE
// argument and cleaned by the caller (`push $0; call *0x44(%eax); add
// $0x4,%esp`) -- a cdecl function POINTER field, not a virtual: a virtual
// would have gone through `mov (%eax),%ecx` first.
struct CluMusicMgrView
{
	uint8 m_pad_00[0x44];
	void (*m_pfnUnknown44)(int);             // 0x44
	uint8 m_pad_48[0x5c - 0x48];
	void* m_pUnknown5c;                      // 0x5c
};

// CClientMgr+0x1508's function table (clientmgr.h's CClientInput), cut to
// the one entry ClearInput calls: `call *0x1c(%eax)` with nothing pushed.
struct CluInputView
{
	uint8 m_pad_00[0x1c];
	void (*m_pfnClearInput)();               // 0x1c
};

// The client manager, opaque except for the six offsets this run reads.
class CluClientMgrView
{
public:
	// AVP2: a fourth argument, the command line itself -- clu_dsi_InitGame
	// pushes pArgs before the config file (0x004037c0 `push %esi`), and
	// AvP2 1.0.9.6's image does the same.
	LTRESULT Init(char** pRezFiles, uint32 nRezFiles, char* pConfigFile,
	              struct CluCommandLineArgs* pArgs);
	LTRESULT Update();                                       // 0x004106c0
	void     Term();                                         // 0x00410ec0
	LTRESULT ClearInput();                                   // 0x00410900
	// clientmgr3.cpp's CCmgClientMgr::UpdateCounters.  __thiscall.
	void     UpdateCounters(LTBOOL bForce);                  // 0x00410860

	uint8            m_pad_000[0x50c];
	// AVP2: as Primal Hunt (PH addresses) -- four more bytes before the net manager (0x0040469b adds 0x510).
	uint8            m_padExtra50c[4];
	uint8            m_NetMgr[0x61c - 0x50c];        // 0x50c
	CluClientShell*  m_pClientShell;                 // 0x61c
	uint8            m_pad_620[0x774 - 0x620];
	uint8            m_SoundMgr[0x898 - 0x774];      // 0x774
	// AVP2: as Primal Hunt (PH addresses) -- the sound manager is 0x10c longer (the music manager, 0xf84 here,
	// is at 0x1094 there): 0xc of it before this field (0x0040398a reads
	// +0x8a8), the rest after.
	uint8            m_padExtraSoundMgr[0xc];
	void*            m_pUnknown898;                  // 0x898
	uint8            m_pad_89c[0x11d0 - 0x89c];
	// AVP2: as Primal Hunt (PH addresses) -- so everything from here sits 0x110 further in (0x004121c3 reads
	// the input table at 0x1618).
	uint8            m_padExtra89c[0x100];
	uint8            m_Version[0x1508 - 0x11d0];     // 0x11d0
	// clientmgr.h's m_pInput: a table of plain cdecl function pointers.
	CluInputView*    m_pInput;                       // 0x1508
	uint8            m_pad_150c[0x15a4 - 0x150c];
	// Inside clientmgr3.cpp's CCmgDemoMgr (0x158c) at its +0x18.
	uint32           m_Unknown15a4;                  // 0x15a4
	uint8            m_pad_15a8[0x1614 - 0x15a8];
	CluUnknown1614*  m_pUnknown1614;                 // 0x1614
};

// clientdsi.cpp's names for the two queue clears.
void sub_42e5e0();                                           // 0x0042e5e0
void sub_42e5f0();                                           // 0x0042e5f0
void dsi_Sub42e600();                                        // 0x0042e600

// ======================================================================= //
//  0x00410900 -- CClientMgr::ClearInput, 48 bytes.  REAL HOME: the
//  CClientMgr TU clientmgr3.cpp (between UpdateCounters 0x00410860 and
//  PlaySound 0x00410960); written here because this file owns its callers.
//  Off the bytes: a positive guard on +0x15a4 (`jne` straight to the single
//  `xor %eax,%eax` at 0x0041092c), then UpdateCounters(TRUE) with ecx still
//  `this`, the three dsi queue flushes, and the input table's +0x1c entry
//  called with nothing pushed.
// ======================================================================= //
LTRESULT CluClientMgrView::ClearInput()
{
	if (!m_Unknown15a4)
	{
		UpdateCounters(LTTRUE);             // 0x0041090f
		sub_42e5e0();                       // 0x00410914
		sub_42e5f0();                       // 0x00410919
		dsi_Sub42e600();                    // 0x0041091e
		m_pInput->m_pfnClearInput();        // 0x00410929
	}

	return LT_OK;
}

CluClientMgrView* clu_sub_410a70();                          // 0x00410a70
void clu_r_TermRender(CluClientMgrView* pMgr, int nReason);  // 0x004627d0
// clientrender.cpp's r_InitRender, clientinterface.cpp's two getters: their
// own declarations, the real classes forward-declared (the views above are
// cast to).
class  CClientMgrView;
struct RMode;
class  ILTClientSoundMgr;
class  CClientMusicMgr;
uint32             r_InitRender(CClientMgrView* pMgr, RMode* pMode);   // 0x004623d0
ILTClientSoundMgr* ci_GetSoundMgr();                         // 0x0040f7f0
CClientMusicMgr*   ci_GetMusicMgr();                         // 0x0040f800

// 0x00403f90 -- the path normaliser this run's two lookups share: copy
// nBytes, force the last byte to 0, upper-case in place, then turn every
// '/' into '\\'.  Defined below, ahead of 0x00403fd0.
void clu_sub_403f90(char* pSrc, char* pDest, int nBytes);    // 0x00403f90

// 0x004a02ed -- strlen / malloc / strcpy: the CRT's _strdup (string.h).
#define clu_strdup _strdup                                   // 0x004a02ed

// 0x00404410 -- the by-id probe over the 100-bucket table (defined
// below, ahead of 0x00404520).  Its parameter is a
// uint16: the callee does the `and $0xffff` itself (then the SIGNED divide
// of int promotion) and compares with `cmp %si,0x10(%eax)`, and both
// callers push without widening -- 0x0040469a loads the ident's word with
// `mov 0x8(%esi),%ax` straight into the push, and 0x004042e2 pushes
// 0x00404280's uint32 id unmasked.
struct CluNetFileRec;
class  CluFileMgr;
CluNetFileRec* clu_sub_404410(CluFileMgr* pFileMgr, uint16 nID);

// 0x00404460 / 0x00404470 -- the queue descriptor's empty slots: clientfilemgr.cpp.

// ======================================================================= //
//
//  THE CLIENT GLOB -- 0x004b0ac0, 0x924 bytes
//
//  WHAT PINS THE SIZE AND THE FIRST TWO FIELDS.  0x00402f90 opens with
//  `mov $0x4b0ac0,%edi; mov $0x249,%ecx; xor %eax,%eax; rep stos` -- an
//  inlined memset of 0x249*4 = 0x924 bytes -- and then hands 0x004b0ac4 to
//  0x00492a18, which stores %ebp/%ebx/%edi/%esi/%esp, the return address and
//  the literal 'VC20' and reads %fs:0.  That is _setjmp3(env, 0), so +0x04
//  is a jmp_buf; VC6's is sixteen dwords, which ends it at +0x44 -- exactly
//  where the HWND starts.  0x004b0ac0 + 0x924 = 0x004b13e4, and 0x004b13e4
//  is the separate start-time dword WinMain writes, so the struct ends
//  flush.
//
//  THE OFFSETS THAT ALREADY HAVE NAMES ELSEWHERE.  clientmgr.h calls
//  0x004b0b04 g_hMainWnd and clientdsi.cpp calls 0x004b0b10 g_pWindowTitle;
//  both are inside this struct (+0x44 and +0x50).  0x004b0b14 is the
//  CClientMgr* the WndProc reads through -- the same dword 0x00402f90 fills
//  from 0x00410a70 at +0x54.
//
// ======================================================================= //

// The struct itself is in cluglob.h, so the other TUs that read its fields
// share the one type.  DEFINED here: this run owns it.
#include "cluglob.h"

CluClientGlob g_CluClientGlob;               // 0x004b0ac0

// The command-line vector.  AVP2: it is the CRT's own __argv / __argc
// (0x004f7c64 / 0x004f7c60 in Primal Hunt's image) -- nothing in the engine
// parses the command line any more (see clu_dsi_BuildCommandLine), and
// every reader in the image (0x00403300, cmgrinit.cpp's 0x004137cd) loads
// the CRT's pair.  cmgrinit.cpp and ltnew2.cpp still read them under these
// two names, so until they say __argv / __argc themselves these are the
// CRT's values, taken once at start-up (the CRT fills its pair before it
// runs the C++ initialisers).
char** g_pCluArgList = __argv;
int    g_nCluArgCount = __argc;

// 0x004b13e4 -- timeGetTime() at the moment WinMain finished parsing.  The
// "Running for %.1f seconds" line subtracts it.
uint32 g_nCluStartTime;                      // 0x004b13e4

// 0x004b3238 / 0x004b13ec -- how many times to run the client, and which run
// this is.  The limit is compared against 1 with `jle` (signed int) and
// against the counter with `jae`/`jb` (so the counter is unsigned).
// The limit is dsi.cpp's "PlayDemoReps" console variable; the counter is
// this run's own (demodtx.cpp's g_nCluCurRunQC5 is it).
extern int    g_CV_PlayDemoReps;             // 0x004b3238  (dsi.cpp)
#define g_nCluNumRuns g_CV_PlayDemoReps
uint32 g_nCluCurRun;                         // 0x004b13ec

// Console variables the pump reads every frame.  Each is compared against a
// zeroed register, never loaded for its value, so all that is known is that
// they are dwords.
// They are dsi.cpp's console bindings (lithtech's copy of the table lays
// them out as server.dll's does): "HighPriority", "ShowRunningTime",
// "CursorCenter" and "NullRender".
extern int g_CV_HighPriority;                // 0x004b32a8
extern int g_CV_ShowRunningTime;             // 0x004b32a0
extern int g_CV_CursorCenter;                // 0x004a9ec4
extern int g_CV_NullRender;                  // 0x004b32a4
#define g_bCluHighPriority  g_CV_HighPriority
#define g_bCluShowTicks     g_CV_ShowRunningTime
#define g_bCluCenterCursor  g_CV_CursorCenter
#define g_bCluFocusSimple   g_CV_NullRender

// 0x004b4d80 -- set to 1 when "DebugStructBanks" is on the command line.
// It is stdlith's own flag (struct_bank.cpp, in StdLith.lib).
extern int g_bDebugStructBanks;              // 0x004b4d80
#define g_bCluDebugStructBanks g_bDebugStructBanks

// 0x004b439c -- the render-up flag, g_RenderStruct's +0x44 (m_bInited in
// clientrender.cpp); reached through the view below, g_bCluRenderInited.

// 0x004b4140 -- the RMode the focus handler re-initialises the renderer
// with.  Its address is pushed, so it is an object and not a pointer.
// clientrender's g_CurRenderMode (an RMode, forward-declared above).
extern RMode g_CurRenderMode;                // 0x004b4140

// The three 100-entry key queues and their two uint16 counts, already
// declared at these addresses in src/engine/lithtech/clientdsi.cpp -- this
// is the code that FILLS them.
// They are FIELDS of the glob (0x00402f90's memset covers them), so these
// names are its members.
#define g_CluKeyDownVK   g_CluClientGlob.m_KeyDownVK      // 0x004b0d34
#define g_CluKeyUpVK     g_CluClientGlob.m_KeyUpVK        // 0x004b0ec4
#define g_CluKeyDownRep  g_CluClientGlob.m_KeyDownRep     // 0x004b1054
#define g_nCluKeyDowns   g_CluClientGlob.m_nKeyDowns      // 0x004b11e4
#define g_nCluKeyUps     g_CluClientGlob.m_nKeyUps        // 0x004b11e6

// The screen-shot pair.  0x004b4358 is tested by its ADDRESS (`mov
// $0x4b4358,%edx; test %edx,%edx`), which is what an `if (array)` on a
// file-scope array compiles to -- VC6 keeps the test even though it can
// never fail.  0x004a9f18 holds the mutable base name "Screenshot" and
// 0x004b444c is the renderer's own bitmap writer, reached through a data
// slot rather than a direct call.
//
// 0x004b4358 is clientrender.cpp's g_RenderStruct and 0x004b444c its +0xf4
// entry (clientworldinfo.cpp's table: referenced once, from 0x00403973), so
// the "directory" test is the address of the render struct and the writer
// is its member; 0x004a9f18 is clientconsole.cpp's g_szScreenshotName.
class RenderStruct;
extern RenderStruct g_RenderStruct;          // 0x004b4358
struct CluRenderStructView
{
	uint8 m_pad_00[0x44];
	int   m_bInited;                                         // +0x44  0x004b439c
	uint8 m_pad_48[0xf4 - 0x48];
	void (*m_pfnMakeScreenShot)(char* pFilename);            // +0xf4  0x004b444c
};
extern char  g_szScreenshotName[];           // 0x004a9f18
#define g_CluScreenShotDir    ((CluRenderStructView*)&g_RenderStruct)
#define g_CluScreenShotName   g_szScreenshotName
#define g_pCluMakeScreenShot  (g_CluScreenShotDir->m_pfnMakeScreenShot)
#define g_bCluRenderInited    (g_CluScreenShotDir->m_bInited)


// The argument block WinMain builds on its own stack and hands to the two
// workers.  0x004039f0 writes all three fields (+0x00 the vector, +0x04 the
// count, +0x08 the one block every string lives in), and WinMain frees +0x00
// and +0x08 on every exit.
struct CluCommandLineArgs
{
	char** m_pArgs;                          // 0x00
	int    m_nArgs;                          // 0x04
	char*  m_pArgBuffer;                     // 0x08
};

// launch.dll's export, called with six arguments and cleaned by the caller.
typedef int (*CluGetLithTechCommandLineFn)(int argc, char** argv, int* pnArgs,
                                           char** pArgs, int nMaxArgs,
                                           int nMaxArgLen);


// ======================================================================= //
//
//  1. THE WINDOWS ENTRY LAYER   0x00402f90 .. 0x00403f8f
//
// ======================================================================= //

LTBOOL clu_dsi_InitGame(CluClientGlob* pGlob, CluCommandLineArgs* pArgs);   // 0x00403330
LRESULT CALLBACK clu_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LTBOOL clu_dsi_BuildCommandLine(CluCommandLineArgs* pArgs);                 // 0x00403d80 (PH)
int WINAPI clu_WinMainQ501(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                           LPSTR lpCmdLine, int nCmdShow);                  // 0x00404180 (PH)

// stdlith's helper class (StdLith.lib, helpers.cpp), cut to the one member
// the command line uses: a case-insensitive compare answering TRUE when the
// two strings are equal (0x004be5d0 in Primal Hunt's image; sharedcommon.cpp
// declares the same member).
class CHelpers
{
public:
	static int UpperStrcmp(const char* pStr1, const char* pStr2);
};

// The crash reporter WinMain's exception filter calls (0x00439b80 in Primal
// Hunt's image, 0x00436270 in AvP2 1.0.9.6's): it is handed the exception
// record and the thread's name, writes crashlog.txt, and answers the
// filter's verdict (clientdsi.cpp).
int __cdecl RecordExceptionInfo(PEXCEPTION_POINTERS data, const char* Message);


// ----------------------------------------------------------------------- //
//  0x00402f90 -- 918 bytes, 42 calls.  Register the window class, put the
//  window up, run the message pump until CClientMgr::Update says stop, then
//  tear the whole thing down.
//
//  WHAT PINS IT
//    * the ONLY function in the run with a frame pointer that is not an
//      __asm function.  `push %ebp; mov %esp,%ebp; sub $0x78,%esp` and
//      `mov %ebp,%esp; pop %ebp` at both exits: VC6 disables FPO for any
//      function that calls setjmp, because longjmp restores %ebp.  The
//      setjmp is at 0x00402fde.
//    * two parameters, read at 0x8(%ebp) and 0xc(%ebp); the first is stored
//      into the glob's +0x48 and then handed to RegisterClass as
//      WNDCLASS::hInstance, so it is the HINSTANCE.
//    * `neg %eax; sbb %eax,%eax; neg %eax` at 0x004030a3 is `!!x`, so
//      m_bBreakOnError is an int holding a 0/1 and not the pointer itself.
//    * the PeekMessageA import slot is loaded ONCE, at 0x004031a1, before
//      the pump -- a local function pointer, the same idiom clientdsi.cpp's
//      sub_42e600 uses.  A loop-invariant hoist would have re-loaded it.
//    * the desktop RECT is taken once at start-up and the CURSOR is centred
//      from it, while the window's own rect (a second RECT) is filled every
//      frame and never read.  That is in the shipped bytes -- 0x00403219
//      passes -0x34(%ebp) to GetWindowRect and 0x00403224 reads -0x18(%ebp),
//      which is the OTHER rect -- and it is preserved here rather than
//      tidied, because tidying it would delete a call.
//    * the two start-up MessageBoxA calls share `push $0` / `push "Error"`
//      before the branch and `push $0` / `call` after it: VC6 tail-merged
//      two separate calls that differ in one argument.
//    * `cmp %esi,%eax` (not `test`) after the DebugStructBanks lookup at
//      0x00402fc6: the result goes through the pStr local that the
//      workingdir lookup reuses.  "Running for" multiplies with `fmuls` --
//      the product is float, promoted only for the varargs push.  The
//      shutdown keeps m_pClientMgr in %edi across Term and dfree, so it
//      is read once into a local.
// ----------------------------------------------------------------------- //
int clu_dsi_RunClientApp(HINSTANCE hInstance, CluCommandLineArgs* pArgs)
{
	CluClientGlob* pGlob;
	WNDCLASS       wndClass;
	MSG            msg;
	RECT           rcWindow;
	RECT           rcDesktop;
	int            nRet;
	int            bOutOfMemory;
	int            nErr;
	char*          pStr;
	CluClientMgrView* pClientMgr;
	BOOL (WINAPI *pfnPeekMessage)(LPMSG, HWND, UINT, UINT, UINT);

	pGlob = &g_CluClientGlob;
	memset(pGlob, 0, sizeof(CluClientGlob));

	nRet = 0;

	pStr = clu_sub_402ec0("DebugStructBanks", pArgs->m_nArgs, pArgs->m_pArgs);
	if (pStr)
		g_bCluDebugStructBanks = 1;

	bOutOfMemory = 0;
	if (setjmp(g_CluClientGlob.m_MemoryJmp))
	{
		bOutOfMemory = 1;
		goto shutdown;
	}

	pStr = clu_sub_402ec0("workingdir", pArgs->m_nArgs, pArgs->m_pArgs);
	if (pStr)
		SetCurrentDirectoryA(pStr);

	pGlob->m_bInputEnabled = 1;

	nErr = clu_sub_42d870();
	if (nErr)
	{
		if (nErr == 1)
			MessageBoxA(NULL, "Unable to load ltmsg.dll.", "Error", 0);
		else
			MessageBoxA(NULL, "Unknown error initializing engine.", "Error", 0);

		clu_sub_42d8e0();
		return -1;
	}

	pGlob->m_hInstance = hInstance;
	pGlob->m_pClassName = "LithTech";
	pGlob->m_pClientMgr = clu_sub_410a70();
	pGlob->m_nUnknown60 = 1;

	pGlob->m_pWindowTitle = clu_sub_402ec0("windowtitle", pArgs->m_nArgs, pArgs->m_pArgs);
	if (!pGlob->m_pWindowTitle)
		pGlob->m_pWindowTitle = "LithTech";

	pGlob->m_bBreakOnError =
		(clu_sub_402ec0("breakonerror", pArgs->m_nArgs, pArgs->m_pArgs) != NULL);

	wndClass.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;   // 0xb
	wndClass.lpfnWndProc   = clu_WndProc;
	wndClass.cbClsExtra    = 0;
	wndClass.cbWndExtra    = 0;
	wndClass.hInstance     = pGlob->m_hInstance;
	wndClass.hIcon         = LoadIconA(NULL, IDI_APPLICATION);
	wndClass.hCursor       = LoadCursorA(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName  = NULL;
	wndClass.lpszClassName = pGlob->m_pClassName;
	RegisterClassA(&wndClass);

	GetWindowRect(GetDesktopWindow(), &rcDesktop);

	pGlob->m_hMainWnd = CreateWindowExA(0,
		pGlob->m_pClassName,
		pGlob->m_pWindowTitle,
		0x10810000,
		((rcDesktop.right - rcDesktop.left) - 320) / 2,
		((rcDesktop.bottom - rcDesktop.top) - 200) / 2,
		320,
		200,
		NULL,
		NULL,
		pGlob->m_hInstance,
		NULL);

	if (!clu_dsi_InitGame(pGlob, pArgs))
		goto shutdown;

	pGlob->m_bWindowUp = 1;
	ShowWindow(pGlob->m_hMainWnd, SW_SHOWNORMAL);
	UpdateWindow(pGlob->m_hMainWnd);

	pfnPeekMessage = PeekMessageA;

	while (pGlob->m_pClientMgr->Update() == 0)
	{
		if (g_bCluHighPriority)
		{
			clu_dsi_ServerError("Setting process to high priority");
			SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		}

		if (g_bCluShowTicks)
		{
			clu_dsi_ServerError("Running for %.1f seconds",
			                    (timeGetTime() - g_nCluStartTime) * 0.001f);
		}

		if (g_bCluCenterCursor && !g_CluClientGlob.m_bAppLostFocus)
		{
			GetWindowRect(pGlob->m_hMainWnd, &rcWindow);
			SetCursorPos((rcDesktop.right - rcDesktop.left) / 2,
			             (rcDesktop.bottom - rcDesktop.top) / 2);
		}

		while (pfnPeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				nRet = msg.wParam;
				goto shutdown;
			}

			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

shutdown:
	if (pGlob->m_ErrorString[0])
	{
		clu_r_TermRender(pGlob->m_pClientMgr, 2);
		clu_sub_42e790(pGlob->m_ErrorString, pGlob->m_pWindowTitle);
	}

	pGlob->m_bWindowUp = 0;

	pClientMgr = pGlob->m_pClientMgr;
	if (pClientMgr)
	{
		pClientMgr->Term();
		clu_dfree(pClientMgr);
	}

	pGlob->m_pClientMgr = NULL;
	DestroyWindow(pGlob->m_hMainWnd);
	clu_sub_42d8e0();

	if (bOutOfMemory)
	{
		ShowWindow(pGlob->m_hMainWnd, SW_HIDE);
		MessageBoxA(GetDesktopWindow(), "Out of memory", pGlob->m_pWindowTitle, 0);
	}

	return nRet;
}


// ----------------------------------------------------------------------- //
//  0x00403330 -- 372 bytes, 12 calls.  Read the game options off the
//  command line and bring CClientMgr up.
//
//  WHAT PINS IT
//    * the frame pointer and the 16-bit local at -0x2(%ebp) are the __asm
//      block: `fstcw`, `andw $0xfcff` and `fldcw` on the same two bytes.
//      0xfcff clears the x87 control word's PC field, i.e. 24-bit
//      (single) precision.  No C construct emits that trio, and __asm is
//      what forces `push %ebp` under /O2.
//    * two parameters: 0x8(%ebp) is written at +0x74/+0x7c/+0x72c and read
//      at +0x54, so it is the glob; 0xc(%ebp) is dereferenced at +0 and +4
//      for the vector and the count, so it is the argument block.
//    * `neg/sbb/neg` again at 0x0040335d: m_bHost is an int 0/1.
//    * the -rez scan bounds itself with `cmp $0x14,%ebx; jae`, an UNSIGNED
//      compare, so the counter is a uint32; the outer bound reloads
//      pArgs->m_nArgs every iteration and compares `jl`, so the index and
//      the count are signed ints.
//    * 0x0040345b's `mov 0x54(%esi),%ecx; add $0x11d0,%ecx` with two stack
//      arguments and no caller cleanup is a __thiscall on a sub-object of
//      CClientMgr at +0x11d0 -- the dword after clientmgr.h's SkyDef.
//    * `movl $0x0,-0x8(%ebp)` before `fildll -0xc(%ebp)` is VC6's
//      unsigned-int-to-double: zero the high dword and load the quadword;
//      the multiply by 0.001f is `fmuls`, so the product is float.
//    * the -rez collector is indexed (`pRezFiles[nRezFiles]`): VC6's own
//      strength-reduced pointer lives in the dead pArgs slot and the count
//      in %ebx, the reverse of what an explicit write pointer gives.
// ----------------------------------------------------------------------- //
LTBOOL clu_dsi_InitGame(CluClientGlob* pGlob, CluCommandLineArgs* pArgs)
{
	char*  pRezFiles[20];
	char   szVersion[32];
	char*  pStr;
	char*  pConfigFile;
	uint32 nRezFiles;
	int    i;
	uint32 nStartTime;
	short  nControlWord;

	__asm
	{
		fstcw nControlWord
		and   nControlWord, 0fcffh
		fldcw nControlWord
	}

	pGlob->m_bHost =
		(clu_sub_402ec0("host", pArgs->m_nArgs, pArgs->m_pArgs) != NULL);

	pGlob->m_pWorldName =
		clu_sub_402ec0("world", pArgs->m_nArgs, pArgs->m_pArgs);

	nRezFiles = 0;

	for (i = 0; i < pArgs->m_nArgs - 1; i++)
	{
		if (stricmp(pArgs->m_pArgs[i], "-rez") == 0)
		{
			pRezFiles[nRezFiles] = pArgs->m_pArgs[i + 1];
			nRezFiles++;
			if (nRezFiles >= 20)
				break;
		}
	}

	if (clu_sub_402ec0("noinput", pArgs->m_nArgs, pArgs->m_pArgs))
		pGlob->m_bInputEnabled = 0;

	pStr = clu_sub_402ec0("workingdir", pArgs->m_nArgs, pArgs->m_pArgs);
	if (pStr)
		SetCurrentDirectoryA(pStr);

	pConfigFile = clu_sub_402ec0("config", pArgs->m_nArgs, pArgs->m_pArgs);
	if (!pConfigFile)
		pConfigFile = "autoexec.cfg";

	nStartTime = timeGetTime();

	if (pGlob->m_pClientMgr->Init(pRezFiles, nRezFiles, pConfigFile, pArgs))
		return LTFALSE;

	((CluVersionView*)pGlob->m_pClientMgr->m_Version)->GetString(szVersion, 32);

	printf("LithTech build %s initialized in %.2f seconds.\n",
	       szVersion,
	       (timeGetTime() - nStartTime) * 0.001f);

	return LTTRUE;
}


// ----------------------------------------------------------------------- //
//  0x004034b0 -- 1335 bytes, 35 calls.  The window procedure.
//
//  WHAT PINS IT
//    * `ret $0x10` and four stack reads: the standard WndProc signature.
//    * the switch is VC6's two-level form -- `cmp $0x100` splits it, the
//      low group goes through a 27-entry BYTE table at 0x004039cc into a
//      5-entry DWORD table at 0x004039b8, and the high group is an
//      if-chain (`sub $0x101`, `dec`, `sub $0x10`).  The byte table's
//      non-default entries are at indices 0, 13, 18 and 26 of `uMsg - 2`,
//      i.e. 0x02 WM_DESTROY, 0x0f WM_PAINT, 0x14 WM_ERASEBKGND and 0x1c
//      WM_ACTIVATEAPP; the high arm is 0x100/0x101/0x102/0x112.
//    * WM_CHAR's arm is a `je` into the middle of WM_DESTROY's tail: two
//      `return 0`s tail-merged, which is what says WM_CHAR's body is empty.
//    * every focus exit answers `sete` on `uMsg == 0x86`.  0x86 is
//      WM_NCACTIVATE, which this switch never routes here, so the four
//      copies are one source-level `return (uMsg == WM_NCACTIVATE);` that
//      VC6 duplicated into each tail.  It is written once, below.
//    * ci_GetSoundMgr/ci_GetMusicMgr are CALLED AGAIN for every use rather
//      than cached -- three calls to 0x0040f800 around two tests and one
//      indirect call.  binkvideo.cpp records the same idiom; the repeat is
//      in the source, not a rematerialisation.
//    * the key queues index with the uint16 count zero-extended
//      (`and $0xffff`) and bump it with `incw`, so the counts are uint16
//      and the compare `cmpw $0x64 ... jae` is unsigned against 100.
//    * the not-ready test GUARDS the switch rather than returning early:
//      both its `je`s land on the one DefWindowProcA call at 0x004038e2,
//      which is why hWnd (%ebp) and wParam (%edi) are loaded above the
//      tests.  An early `return DefWindowProcA(...)` is a second call and
//      loads them after.
//    * the focus handler reaches its one `return (uMsg == WM_NCACTIVATE)`
//      from every path through an if/else nest; with the return written at
//      each exit uMsg took a register of its own (four pushes, %ebx) and
//      the test layout moved.  Case bodies are laid out in source order:
//      WM_DESTROY's call sits after the focus handler (0x004037bd).
//
//  NEAR (1289 code bytes against 1285, 354 instructions each): the one
//  residual is WM_DESTROY's `return 0` -- shipped jumps WM_CHAR into it
//  (0x0040383b -> 0x004037c7, a short `je`), so its epilogue is `pop; pop;
//  xor; pop`; ours sends WM_CHAR to WM_KEYUP's final epilogue (a long
//  `je`, the four bytes) and schedules WM_DESTROY's as `xor; pop; pop; pop`.  Not moved by: WM_DESTROY falling through into
//  WM_CHAR's `return 0`, WM_CHAR first in the switch, a `return 0` after
//  the switch, or sixteen orders of the case bodies.
// ----------------------------------------------------------------------- //
LRESULT CALLBACK clu_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	char        szFile[200];
	char        szMsg[256];
	FILE*       pFile;
	int         i;

	if (g_CluClientGlob.m_bWindowUp && g_CluClientGlob.m_pClientMgr)
	switch (uMsg)
	{
		case WM_PAINT:
		{
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_ERASEBKGND:
		{
			if (g_bCluRenderInited)
				return 0;

			break;
		}

		case WM_ACTIVATEAPP:
		{
			if (!g_bCluFocusSimple)
			{
				if (!g_CluClientGlob.m_nUnknown6c)
				{
					if (wParam)
					{
						if (g_CluClientGlob.m_bAppLostFocus)
						{
							g_CluClientGlob.m_pClientMgr->m_pClientShell->OnEvent(7, 0);

							if (!g_CluClientGlob.m_bRenderShutdown)
							{
								OutputDebugStringA("Regained focus.. initializing renderer.\n");

								if (r_InitRender((CClientMgrView*)g_CluClientGlob.m_pClientMgr,
								                 &g_CurRenderMode))
								{
									clu_sub_42d770(szMsg, 255, 0x30, NULL);
									clu_sub_42e6a0(szMsg);
								}
							}

							if (g_CluClientGlob.m_pClientMgr->m_pUnknown898)
							{
								((CSoundMgrView*)
								 g_CluClientGlob.m_pClientMgr->m_SoundMgr)->sub_481ef0();
							}
						}

						g_CluClientGlob.m_pClientMgr->ClearInput();

						g_CluClientGlob.m_bAppLostFocus = 0;
						g_CluClientGlob.m_nUnknown60 = 1;

						if (g_CluClientGlob.m_pClientMgr->m_pUnknown1614)
							g_CluClientGlob.m_pClientMgr->m_pUnknown1614->OnFocusGained();
					}
					else
					{
						g_CluClientGlob.m_pClientMgr->m_pClientShell->OnEvent(6, 0);

						OutputDebugStringA("Losing focus.. shutting down renderer.\n");

						clu_r_TermRender(g_CluClientGlob.m_pClientMgr, 1);
						g_CluClientGlob.m_pClientMgr->ClearInput();

						g_CluClientGlob.m_bAppLostFocus = 1;
						g_CluClientGlob.m_nUnknown60 = 0;

						if (!g_CluClientGlob.m_bMinimizing)
						{
							if (g_CluClientGlob.m_pClientMgr->m_pUnknown898)
							{
								((CSoundMgrView*)
								 g_CluClientGlob.m_pClientMgr->m_SoundMgr)->sub_481e70();
							}
						}
					}
				}
			}
			else if (wParam)
			{
				if (g_CluClientGlob.m_bAppLostFocus)
				{
					OutputDebugStringA("Regained focus... Sending GAINEDFOCUS event.\n");

					g_CluClientGlob.m_pClientMgr->m_pClientShell->OnEvent(7, 0);
				}

				g_CluClientGlob.m_pClientMgr->ClearInput();
				g_CluClientGlob.m_bAppLostFocus = 0;
			}
			else if (!g_CluClientGlob.m_bMinimizing)
			{
				OutputDebugStringA("Lost focus... Sending LOSTFOCUS event.\n");

				g_CluClientGlob.m_pClientMgr->m_pClientShell->OnEvent(6, 0);

				ShowWindow(g_CluClientGlob.m_hMainWnd, SW_MINIMIZE);

				g_CluClientGlob.m_bAppLostFocus = 1;
			}

			return (uMsg == WM_NCACTIVATE);
		}

		case WM_DESTROY:
		{
			clu_sub_42e6a0(NULL);
			return 0;
		}

		case WM_KEYDOWN:
		{
			if (g_nCluKeyDowns < 100)
			{
				g_CluKeyDownVK[g_nCluKeyDowns] = wParam;
				g_CluKeyDownRep[g_nCluKeyDowns] = lParam & 0x7fff;
				g_nCluKeyDowns++;
			}

			return 0;
		}

		case WM_KEYUP:
		{
			if (wParam == VK_F8)
			{
				if (g_CluScreenShotDir)
				{
					for (i = 0; i < 3000; i++)
					{
						sprintf(szFile, "%s%d.bmp", g_CluScreenShotName, i);

						pFile = fopen(szFile, "rb");
						if (pFile)
						{
							fclose(pFile);
						}
						else
						{
							g_pCluMakeScreenShot(szFile);
							return 0;
						}
					}
				}

				return 0;
			}

			if (g_nCluKeyUps < 100)
			{
				g_CluKeyUpVK[g_nCluKeyUps] = wParam;
				g_nCluKeyUps++;
			}

			return 0;
		}

		case WM_CHAR:
		{
			return 0;
		}

		case WM_SYSCOMMAND:
		{
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 1;

			// NOT an else-if chain: when the window IS iconic the
			// screen-saver arm's `jne 0x00403893` lands on the SC_CLOSE
			// test rather than past it, so the three tests are sequential
			// statements in the source.
			if ((wParam & 0xfff0) == SC_SCREENSAVE ||
			    (wParam & 0xfff0) == SC_MONITORPOWER)
			{
				if (!IsIconic(hWnd))
					return 1;
			}

			if ((wParam & 0xfff0) == SC_CLOSE)
			{
				g_CluClientGlob.m_bMinimizing = 1;

				// ci_GetSoundMgr and ci_GetMusicMgr are CALLED AGAIN for
				// every use -- five calls across two tests and one
				// dispatch -- which is what the shipped bytes do and not a
				// rematerialisation: the source repeats the expression.
				if (*(void**)((char*)ci_GetSoundMgr() + 0x124))
					((CSoundMgrView*)ci_GetSoundMgr())->StopAllSounds();

				if (ci_GetMusicMgr())
				{
					if (((CluMusicMgrView*)ci_GetMusicMgr())->m_pUnknown5c)
						((CluMusicMgrView*)ci_GetMusicMgr())->m_pfnUnknown44(0);
				}
			}

			break;
		}
	}

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}


// ----------------------------------------------------------------------- //
//  0x00403d80 -- 1022 bytes (Primal Hunt).  Turn the CRT's argument vector
//  into the compact block WinMain hands around, giving launch.dll first
//  refusal on the whole thing and expanding any -cmdfile on the way.
//
//  AVP2: this replaces NOLF's pair of passes.  There is no command-line
//  parser of the engine's own any more: the arguments are the CRT's __argc
//  / __argv (0x004f7c60 / 0x004f7c64, written by the startup code at
//  0x004cabea / 0x004cabf2), and -cmdfile is handled HERE, by splicing the
//  first line of the named file into the vector in place of the switch and
//  its operand.
//
//  WHAT PINS THE FRAME.  `mov $0x785c,%eax; call __chkstk`: the count at
//  +0x14, the quote flag (a byte) at +0x13, the outer index at +0x18, a
//  200-byte scratch token at +0x1c, then the same 150 pointers (+0xe4) and
//  150 by 200 bytes of text (+0x33c) NOLF's had.  0x33c + 0x7530 = 0x786c,
//  the frame plus the four saved registers.
//
//  WHAT PINS THE REST
//    * launch.dll's export is called exactly as before, six arguments, and
//      either failure falls into the one message box (0x00403e29).
//    * the switch test is stdlith's CHelpers::UpperStrcmp (0x004be5d0,
//      TRUE when equal), and a NON-match is the copy path (0x00404051).
//    * the file is sized with fseek(SEEK_END) / ftell / rewind and read with
//      ONE fgets into a buffer of the length plus one; a failed read closes
//      the file and refuses, leaving the buffer allocated.
//    * the line is cut with strtok on " ".  A token opening with '"' is
//      copied without it into the scratch token; if it also closes there the
//      closing quote is cut and the argument stored straight away (and the
//      next token fetched, skipping the bound below), otherwise the flag is
//      raised and the SAME token goes round again: strtok on '"' fetches the
//      rest of the quoted text, which is appended after a space.
//    * every stored argument is cut at [199] and counted, and the count is
//      checked AFTER the increment with `cmp $0x96; jg` -- so 150 is allowed
//      through and the next store lands one past the array (RETAIL BUG
//      below).
//    * the tail is NOLF's: sum the lengths, allocate both blocks through the
//      0x00433440 pair, refuse unless both came back, copy.
//
//  RETAIL BUG: the bound is `> 150`, not `>= 150`, here and in the copy
//  path; the 151st argument is copied into szArgs[150], past the end of the
//  frame's array.
// ----------------------------------------------------------------------- //
LTBOOL clu_dsi_BuildCommandLine(CluCommandLineArgs* pArgs)
{
	int    nArgs;
	bool   bInQuote;
	int    i;
	char   szToken[200];
	char*  pArgPtrs[150];
	char   szArgs[150][200];
	HINSTANCE hLaunch;
	CluGetLithTechCommandLineFn pfnGetCommandLine;
	int    bGotArgs;
	FILE*  pFile;
	int    nLen;
	char*  pBuffer;
	char*  pToken;
	uint32 nTotal;
	char*  pDest;
	int    k;

	nArgs = 0;
	bInQuote = false;

	for (k = 0; k < 150; k++)
	{
		pArgPtrs[k] = szArgs[k];
		szArgs[k][0] = 0;
	}

	if (clu_sub_402f70("launch"))
	{
		bGotArgs = 0;

		hLaunch = LoadLibraryA("launch.dll");
		if (hLaunch)
		{
			pfnGetCommandLine = (CluGetLithTechCommandLineFn)
				GetProcAddress(hLaunch, "GetLithTechCommandLine");

			if (pfnGetCommandLine)
			{
				bGotArgs = pfnGetCommandLine(__argc, __argv,
				                             &nArgs, pArgPtrs, 150, 200);
			}

			FreeLibrary(hLaunch);
		}

		if (!bGotArgs)
		{
			MessageBoxA(NULL, "Unable to load launch.dll.", "LithTech", 0);
			return LTFALSE;
		}
	}
	else
	{
		for (i = 0; i < __argc; i++)
		{
			if (CHelpers::UpperStrcmp(__argv[i], "-cmdfile"))
			{
				i++;

				if (__argv[i])
				{
					pFile = fopen(__argv[i], "r");
					if (pFile)
					{
						fseek(pFile, 0, SEEK_END);
						nLen = ftell(pFile);
						rewind(pFile);
						nLen++;

						pBuffer = (char*)clu_sub_428fd0(nLen);

						if (!fgets(pBuffer, nLen, pFile))
						{
							fclose(pFile);
							return LTFALSE;
						}

						fclose(pFile);

						pToken = strtok(pBuffer, " ");
						while (pToken)
						{
							if (!bInQuote)
							{
								if (*pToken == '"')
								{
#ifdef PRIMAL_HUNT
									nLen = strlen(pToken);
									strncpy(szToken, pToken + 1, nLen - 1);
									szToken[nLen - 1] = 0;

									if (nLen - 2 >= 0 && szToken[nLen - 2] == '"')
									{
										szToken[nLen - 2] = 0;
										strncpy(szArgs[nArgs], szToken, 200);
										pToken = strtok(NULL, " ");
										nArgs++;
									}
									else
									{
										bInQuote = true;
									}
#else
									// AVP2: the token is copied without its
									// quote by hand, and it always opens a
									// quoted run (0x00403b79..0x00403baa).
									// RETAIL BUG: the terminator lands two
									// bytes past the copy, leaving
									// szToken[nLen - 1] and szToken[nLen]
									// as they were.
									nLen = strlen(pToken);
									for (k = 1; k < nLen; k++)
										szToken[k - 1] = pToken[k];
									szToken[k + 1] = 0;

									bInQuote = true;
#endif

									continue;
								}

								strncpy(szArgs[nArgs], pToken, 200);
							}
							else
							{
								pToken = strtok(NULL, "\"");
								strncat(szToken, " ", 1);
								strncat(szToken, pToken, strlen(pToken));
								bInQuote = false;
								strncpy(szArgs[nArgs], szToken, 200);
							}

							szArgs[nArgs][199] = 0;
							nArgs++;
							if (nArgs > 150)
								break;

							pToken = strtok(NULL, " ");
						}

						clu_sub_429080(pBuffer);
					}
				}
			}
			else
			{
				strncpy(szArgs[nArgs], __argv[i], 200);
				szArgs[nArgs][199] = 0;
				nArgs++;
				if (nArgs > 150)
					break;
			}
		}
	}

	nTotal = 0;
	for (k = 0; k < nArgs; k++)
		nTotal += strlen(pArgPtrs[k]) + 1;

	pArgs->m_pArgBuffer = (char*)clu_sub_428fd0(nTotal);
	pArgs->m_pArgs = (char**)clu_sub_428fd0(nArgs * sizeof(char*));

	if (pArgs->m_pArgBuffer && pArgs->m_pArgs)
	{
		pDest = pArgs->m_pArgBuffer;

		for (k = 0; k < nArgs; k++)
		{
			strcpy(pDest, pArgPtrs[k]);
			pArgs->m_pArgs[k] = pDest;
			pDest += strlen(pArgPtrs[k]) + 1;
		}

		pArgs->m_nArgs = nArgs;
		return LTTRUE;
	}

	return LTFALSE;
}


// ----------------------------------------------------------------------- //
//  0x00404180 -- 232 bytes (Primal Hunt).  The body of WinMain, called from
//  the guarded entry below with the same four arguments (`ret $0x10`).
//
//  AVP2: NOLF's parse of lpCmdLine, its -cmdfile scan and the
//  FreeCommandLine at the end are gone -- BuildCommandLine reads the CRT's
//  vector itself -- so lpCmdLine is not read at all.  What is left is
//  NOLF's: the three-dword argument block zeroed field by field, the start
//  time, the build, the run, the repeat-run loop (the guard's increment and
//  the body's, `jle` signed against the limit and `jb` unsigned against the
//  counter), and the two frees through 0x00433520 on both exits.
// ----------------------------------------------------------------------- //
int WINAPI clu_WinMainQ501(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                           LPSTR lpCmdLine, int nCmdShow)
{
	CluCommandLineArgs args;
	int                nRet;

	AVP2SRC_INSTALL_CRASH_HANDLER();   // debug build only

	args.m_pArgs = NULL;
	args.m_nArgs = 0;
	args.m_pArgBuffer = NULL;

	g_nCluStartTime = timeGetTime();

	if (!clu_dsi_BuildCommandLine(&args))
	{
		if (args.m_pArgs)
			clu_sub_429080(args.m_pArgs);

		if (args.m_pArgBuffer)
			clu_sub_429080(args.m_pArgBuffer);

		return -1;
	}

	nRet = clu_dsi_RunClientApp(hInstance, &args);

	if (g_nCluNumRuns > 1)
	{
		g_nCluCurRun++;

		while (g_nCluCurRun < (uint32)g_nCluNumRuns)
		{
			g_nCluCurRun++;
			nRet = clu_dsi_RunClientApp(hInstance, &args);
		}
	}

	if (args.m_pArgs)
		clu_sub_429080(args.m_pArgs);

	if (args.m_pArgBuffer)
		clu_sub_429080(args.m_pArgBuffer);

	return nRet;
}


// ----------------------------------------------------------------------- //
//  0x00404270 -- WinMain (Primal Hunt; 0x004042c0 and 0x004042d2 are its
//  filter expression and handler, enumerated as rows of their own).
//
//  AVP2: the whole client runs inside a structured-exception frame: the
//  `push $-1; push $0x4d44e8; push $0x4c08c8` prologue is VC6's __try with
//  _except_handler3, the answer starts at -1 (`or $-1,%esi`), and the
//  filter hands GetExceptionInformation() and the thread's name ("main
//  thread", 0x004e092c) to the crash reporter at 0x00439b80.  The handler
//  does nothing: the answer stays -1.
// ----------------------------------------------------------------------- //
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	int nRet = -1;

	__try
	{
		nRet = clu_WinMainQ501(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except (RecordExceptionInfo(GetExceptionInformation(), "main thread"))
	{
	}

	return nRet;
}



// ======================================================================= //
//  2. THE CLIENT FILE MANAGER (0x00403fd0 .. 0x0040488d): clientfilemgr.cpp,
//  its own translation unit.
// ======================================================================= //

// ======================================================================= //
//
//  WHAT IS IN THE RUN AND WHAT IS NOT
//
//    0x00402f90  918 b  42 c  clu_dsi_RunClientApp
//    0x00403330  372 b  12 c  clu_dsi_InitGame
//    0x004034b0 1335 b  35 c  clu_WndProc
//    0x004039f0  454 b  10 c  clu_dsi_BuildCommandLine
//    0x00403bc0  414 b   3 c  (NOLF's clu_dsi_ParseCommandLine)
//    0x00403d60   56 b   2 c  (NOLF's clu_dsi_FreeCommandLine)
//    0x00403da0  142 b  10 c  (NOLF's clu_dsi_ParseCommandFile)
//    0x00403e30  338 b  13 c  WinMain
//
//  AVP2: the three NOLF command-line helpers are gone from both AvP2 images
//  -- the CRT's own vector is used and -cmdfile is expanded inside
//  BuildCommandLine -- and WinMain is a guarded entry around its old body.
//  Primal Hunt's addresses: BuildCommandLine 0x00403d80 (1022 b),
//  clu_WinMainQ501 0x00404180, WinMain 0x00404270; AvP2 1.0.9.6's
//  0x00403a00, 0x00403d90, 0x00403e80.
//    0x00403fd0  261 b  11 c  clu_fm_Create
//    0x004040f0  179 b   7 c  clu_fm_Destroy
//    0x004041f0  140 b   3 c  clu_fm_Init
//    0x00404280  302 b   8 c  clu_fm_OnServerFile
//    0x004043b0   89 b   1 c  clu_fm_FindTreeWithFile
//    0x00404520  172 b   3 c  clu_fm_AddTrees
//    0x00404680  526 b   5 c  clu_fm_GetFileRec
//
//  Direct/indirect call counts were taken from the shipped image and
//  every body above issues exactly that many:
//
//    0x00402f90  18 direct / 24 indirect      0x00403fd0  11 / 0
//    0x00403330   9 /  3                      0x004040f0   7 / 0
//    0x004034b0  19 / 16                      0x004041f0   3 / 0
//    0x004039f0   5 /  5                      0x00404280   7 / 1
//    0x00403bc0   3 /  0                      0x004043b0   1 / 0
//    0x00403da0   9 /  1                      0x00404520   3 / 0
//    0x00403e30  12 /  1                      0x00404680   5 / 0
//
//  Four of those counts depend on the compiler doing something rather than
//  on what is written, and they are the four places to look first if a
//  count comes back wrong:
//
//    * 0x00402f90's start-up error box.  The shipped image has ONE
//      MessageBoxA call site for two source-level calls -- VC6 hoisted the
//      two shared pushes above the branch and tail-merged the rest.  If the
//      merge does not happen the indirect count reads 25, not 24.
//    * 0x00403e30's two cleanup blocks.  The -cmdfile failure and the
//      BuildCommandLine failure are written out separately here and the
//      shipped image tail-merges them into one pair of frees.  Without the
//      merge the direct count reads 14.
//    * strlen / strcpy / memcpy / memset.  Every one of them in this file is
//      expected to expand to the `repnz scasb` / `rep movs` / `rep stos`
//      intrinsic and cost no call; 0x004039f0's 200-byte copy is a real
//      call because it is strncpy (0x00492b60), which has no intrinsic.  /Oi is implied by /O2, so this
//      should hold, but if a call appears where a `rep` was expected that is
//      what happened.
//    * new / delete in clu_fm_Create and clu_fm_Destroy.  The shipped
//      allocation goes straight to 0x004290b0 and the release to
//      0x004290c0, with no `operator new` forwarder in between, so whatever
//      the build links `operator new` to has to be that pair for the call to
//      land on the right address.  The displacement is masked either way, so
//      this cannot cost byte-identity -- only the call's resolution.
//
//  WHAT COULD NOT BE PINNED
//
//    * ClientGlob +0x058, +0x060, +0x068, +0x06c, +0x070 and +0x078.  Each
//      is only ever compared against zero or stored 0/1, so the names above
//      describe the CONTROL FLOW that reads them, not what they mean.
//      +0x060 in particular is set to 1 at start-up and then toggled by the
//      focus handler in the opposite sense to m_bAppLostFocus, which is
//      suggestive and no more.
//    * 0x004b32a4, 0x004b32a8, 0x004b32a0 and 0x004a9ec4 are console
//      variables by their use (read every frame, never written here) but
//      nothing in this run names them.  "ShowTicks" at 0x004a7794 is the
//      only literal near them and it is not pushed by any function in the
//      run -- it is almost certainly the registration of 0x004b32a0, done
//      elsewhere, which is why g_bCluShowTicks carries that spelling.
//    * 0x004b4358, tested by its address in the WM_KEYUP screen-shot arm.
//      It is an array of something; what, the bytes do not say.
//    * CluNetFileRec's +0x18 and +0x24.  Both take the same interned path as
//      +0x1c and nothing in this run reads either of them.  They could be
//      two more names, or a union, or fields a later revision stopped using.
//    * the second argument of clu_fm_Init and the third of
//      clu_fm_OnServerFile.  The first is copied into the queue descriptor's
//      last dword and never read here; the second is not read at all.
//    * whether the two halves of this file were one translation unit in
//      2000.  The literal pool is contiguous and in address order across
//      both, which is consistent with one TU and also with two adjacent
//      ones; the linker concatenates either way.  Verification is per
//      function, so it does not matter for scoring -- but it is the reason
//      this file has two clearly separated sections rather than one.
//
// ======================================================================= //
