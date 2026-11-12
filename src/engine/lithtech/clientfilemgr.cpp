// *********************************************************************** //
//
//  MODULE:   CLIENTFILEMGR (.CPP)   (file name ours) -- lithtech.exe only
//
//  The client file manager (AvP2 1.0.9.6 0x00403fd0 .. 0x0040488d; Primal
//  Hunt 0x00404330 .. 0x00404eeb), split out of clientwin.cpp: its own
//  translation unit, after the WinMain run's <string> guard pair (Primal
//  Hunt 0x00404300) and ending in its own (0x00405040).  The declarations
//  ahead of section 2 are clientwin.cpp's, repeated so this TU stands
//  alone; the record banks' members and the queue descriptor's empty slots
//  (0x00404460, 0x00404470) are this run's and moved with it.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x405040).
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

// The two record pools.  Over bankbase.h's CBankBase: base vtable
// 0x004a14c8 (slots 1..3 are _purecall), derived 0x004a14b8 and 0x004a14a8.
// ltnew2.cpp already reads the same three tables out of the image.
#include "bankbase.h"

class CluFileRecBank : public CBankBase                      // vtable 0x004a14b8
{
public:
	CluFileRecBank() { clu_sb_Init(&m_Bank, 0x20, 0x20); }
	virtual ~CluFileRecBank() { clu_sb_FreeChunks(&m_Bank, 1); }
	virtual void* AllocVoid();               // 0x004049d0
	virtual void  FreeVoid(void* pObj);      // 0x004049c0
	virtual void  Term();                    // 0x00444820 (ICF)

	CluStructBank m_Bank;                    // +0x04
	uint32        m_Unknown20;               // +0x20
};

class CluNetFileRecBank : public CBankBase                   // vtable 0x004a14a8
{
public:
	CluNetFileRecBank() { clu_sb_Init(&m_Bank, 0x28, 0x20); }
	virtual ~CluNetFileRecBank() { clu_sb_FreeChunks(&m_Bank, 1); }
	virtual void* AllocVoid();               // 0x004049d0 (ICF)
	virtual void  FreeVoid(void* pObj);      // 0x004049c0 (ICF)
	virtual void  Term();                    // 0x00404a00

	CluStructBank m_Bank;                    // +0x04
	uint32        m_Unknown20;               // +0x20
};

// The banks' three own slots, read off the two vtables: slot 1 at 0x004049d0
// (the struct-bank pop, sb_Allocate), slot 2 at 0x004049c0 (push back on the
// free list at +0x1c), slot 3 the reset -- free every chunk, then sb_Init
// again with the record size (0x00404a00 with 0x28; 0x00444820, the
// 0x20-byte one, is objbank.cpp's CObjectBank<CBankElem20>::Init, which
// /OPT:ICF kept in place of this body).  Slots 1 and 2 are the same bytes for
// both classes and for every objbank.cpp instantiation; ICF keeps the first,
// in this run.
void* CluFileRecBank::AllocVoid()
{
	return clu_sb_Allocate(&m_Bank);
}

// AVP2: sb_Free's null tests, on the object and on the bank (&m_Bank, so
// `lea 0x4(%ecx)` then `test`): lithtech.exe 0x00416070, which both banks'
// vtables (0x004d44f4, 0x004d4504) share.
void CluFileRecBank::FreeVoid(void* pObj)
{
	clu_sb_FreeQ501(&m_Bank, pObj);
}

void CluFileRecBank::Term()
{
	clu_sb_FreeChunks(&m_Bank, 1);
	clu_sb_Init(&m_Bank, 0x20, 0x20);
}

void* CluNetFileRecBank::AllocVoid()
{
	return clu_sb_Allocate(&m_Bank);
}

// AVP2: sb_Free's null tests, on the object and on the bank (&m_Bank, so
// `lea 0x4(%ecx)` then `test`): lithtech.exe 0x00416070, which both banks'
// vtables (0x004d44f4, 0x004d4504) share.
void CluNetFileRecBank::FreeVoid(void* pObj)
{
	clu_sb_FreeQ501(&m_Bank, pObj);
}

void CluNetFileRecBank::Term()
{
	clu_sb_FreeChunks(&m_Bank, 1);
	clu_sb_Init(&m_Bank, 0x28, 0x20);
}

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

// 0x00404460 / 0x00404470 -- the queue descriptor's other four slots:
// `xor %eax,%eax; ret` and a bare `ret`.  The last three slots are THREE
// separate empty functions that the linker's /OPT:ICF folded onto
// 0x00404470 (the same address also answers ci_DeleteColor, cc_BlastServer,
// rs_Empty and cmg_Handler404470).  0x0040424a..0x0040425a store the address
// as three separate immediates; one function named three times is CSE'd
// into %eax by /O2 instead.
//
// They are this run's own (0x00404460/0x00404470 sit between 0x00404410 and
// 0x00404480), so they are defined here, and their shipped bodies are what
// is written: `xor %eax,%eax; ret` and `ret`.
int  clu_sub_404460() { return 0; }                          // 0x00404460
void clu_sub_404470() {}                                     // 0x00404470
void clu_fm_Empty0c() {}                                     // 0x00404470 (ICF)
void clu_fm_Empty10() {}                                     // 0x00404470 (ICF)


#include "cluglob.h"

// ======================================================================= //
//
//  2. THE CLIENT FILE MANAGER   0x00403fd0 .. 0x0040488d
//
//  THE OBJECT IS 0xe88 BYTES and every field below is fixed by an offset
//  the code touches:
//
//    0x000  LTLink            the open-tree list         (walked at 0x004040f5,
//                                                         0x004043c0, 0x00404620)
//    0x00c  string holder     ctor 0x00490f60, chunk size 0x00491060,
//                             AddString 0x00491070, dtor 0x00490fe0
//    0x024  record bank       vptr 0x004a14b8, sb_Init(0x20, 0x20) --
//                             0x20-byte records, free list at 0x040
//    0x048  record bank       vptr 0x004a14a8, sb_Init(0x28, 0x20) --
//                             0x28-byte records, free list at 0x064
//    0x06c  void*             the "c:\de_cache" tree
//    0x070  LTLink[100]       hashed by server file id   (0x070 + 100*12 = 0x520)
//    0x520  LTLink[200]       hashed by path             (0x520 + 200*12 = 0xe80)
//    0xe80  void*             the message queue 0x0042f740 makes
//    0xe84  CClientMgr*       the owner
//
//  0x00404098 and 0x004040ba both walk their array with `mov %eax,0x4(%eax);
//  mov %eax,(%eax); add $0xc,%eax` -- LTLink::TieOff writes m_pNext then
//  m_pPrev because ltlink.h spells it `m_pPrev = m_pNext = (LTLink*)this`.
//  The 12-byte stride and the 0x64 / 0xc8 counts are what fix the bounds.
//
//  0x00401000 is the compiler's own `vector constructor iterator`:
//  (pArray, nStride, nCount, pfnCtor), with LTLink's do-nothing constructor
//  at 0x004040e0 (`mov %ecx,%eax; ret`).  It appears because ltlink.h gives
//  LTLink a user-declared constructor, so an ARRAY member of it has to be
//  constructed element by element -- which is the strongest single piece of
//  evidence that these really are LTLink arrays and not raw dword triples.
//
// ======================================================================= //

// One open tree.  16 bytes from the 0x00428fd0 pair, its own link, and
// m_pData pointing back at itself.
struct CluTreeNode
{
	LTLink m_Link;                           // 0x00
	void*  m_pTree;                          // 0x0c
};

// The 0x20-byte record, hashed by path into m_ByName.
struct CluFileRec
{
	uint32 m_nUnknown00;                     // 0x00  cleared, never written
	LTLink m_Link;                           // 0x04  m_pData points at the record
	void*  m_pTree;                          // 0x10
	uint16 m_nUnknown14;                     // 0x14  set to 0xffff at birth
	uint16 m_nNameLen;                       // 0x16
	uint8  m_nFlag;                          // 0x18  0xff matches anything
	uint8  m_pad_19[0x1c - 0x19];
	char*  m_pName;                          // 0x1c  interned by the string holder
};

// The 0x28-byte record, hashed by server file id into m_ByID.  0x004045d0
// (not on this list) answers ILTClient's file name out of +0x1c, which is
// what says +0x18/+0x1c/+0x24 all hold the same interned path.
struct CluNetFileRec
{
	LTLink      m_Link;                      // 0x00  m_pData points at the record
	void*       m_pTree;                     // 0x0c
	uint16      m_nID;                       // 0x10
	uint16      m_pad_12;
	CluFileRec* m_pFileRec;                  // 0x14  filled in elsewhere
	char*       m_pName18;                   // 0x18
	char*       m_pName;                     // 0x1c
	uint16      m_nNameLen;                  // 0x20
	uint16      m_pad_22;
	char*       m_pName24;                   // 0x24
};

// What ILTClient hands the manager to name a file: either a path, or a type
// 2 that names a server file by id and carries the path as a fallback.
struct CluFileIdent
{
	int    m_nType;                          // 0x00  2 == a server file id
	char*  m_pName;                          // 0x04
	uint16 m_nID;                            // 0x08
};

class CluFileMgr
{
public:
	LTLink            m_TreeList;                    // 0x000
	CStringHolder     m_StringHolder;                // 0x00c
	CluFileRecBank    m_FileRecBank;                 // 0x024
	CluNetFileRecBank m_NetFileRecBank;              // 0x048
	void*             m_pCacheTree;                  // 0x06c
	LTLink            m_ByID[100];                   // 0x070
	LTLink            m_ByName[200];                 // 0x520
	void*             m_pQueue;                      // 0xe80
	CluClientMgrView* m_pClientMgr;                  // 0xe84
};

// 0x00404480 -- drop every id record back on its bank and close the queue
// (defined below, after clu_sub_404410).
void clu_sub_404480(CluFileMgr* pFileMgr);                   // 0x00404480

CluTreeNode* clu_fm_FindTreeWithFile(LTLink* pTreeList, char* pName);   // 0x004043b0
int clu_fm_OnServerFile(void* pQueue, char* pName, uint32 nUnused, uint32 nID);


// ----------------------------------------------------------------------- //
//  0x00403f90 -- 60 bytes.  The path normaliser declared above: strncpy
//  (0x00492b60) into pDest, terminate at nBytes - 1, _strupr (0x004a0320),
//  then every '/' becomes '\\'.
// ----------------------------------------------------------------------- //

void clu_sub_403f90(char* pSrc, char* pDest, int nBytes)
{
	strncpy(pDest, pSrc, nBytes);
	pDest[nBytes - 1] = 0;
	_strupr(pDest);

	// the cursor is pDest itself (`inc %esi`, no copy)
	while (*pDest)
	{
		if (*pDest == '/')
			*pDest = '\\';

		pDest++;
	}
}


// ----------------------------------------------------------------------- //
//  0x00403fd0 -- 261 bytes, 11 calls.  Make one.
//
//  WHAT PINS IT.  `push $0xe88; call dalloc; test; je` with the whole
//  member-construction run inside the `if` and a `xor %esi,%esi` in the
//  else is VC6's `new` -- the guard is the compiler's, which is why the
//  code AFTER it dereferences the pointer without checking it.  The
//  constructor itself is inlined: string holder, then the two banks (each
//  writing its own vptr then sb_Init), then two `vector constructor
//  iterator` calls for the LTLink arrays, in declaration order.  m_TreeList
//  is an LTLink too but its constructor is empty and generates nothing.
//
//  The two banks are then RE-initialised with sb_Init2 after their chunks
//  are handed back -- a deliberate resize past what the constructors set,
//  0x40/0x400 and 0x80/0x400 against the constructors' 0x20.
//
//  BOTH `vector constructor iterator` calls (0x0040401a and 0x0040402c) are
//  there only because /O2's per-function inline budget runs out before the
//  arrays' empty LTLink constructors are reached; with budget left VC6
//  inlines the empty constructor loop to nothing.  Plain dl_TieOff at all
//  three sites leaves one iterator (m_ByName's) and loses m_ByID's; the one
//  extra inline expansion of AsLTLink() on the list head is what makes both
//  come out (a second TieOff loop does the same, so it is budget, not shape).
// ----------------------------------------------------------------------- //
CluFileMgr* clu_fm_Create(CluClientMgrView* pClientMgr)
{
	CluFileMgr* pMgr;
	int         i;

	pMgr = new CluFileMgr;

	pMgr->m_pCacheTree = LTNULL;
	pMgr->m_pQueue = LTNULL;

	pMgr->m_StringHolder.SetAllocSize(0x1000);

	clu_sb_FreeChunks(&pMgr->m_FileRecBank.m_Bank, 1);
	clu_sb_Init2(&pMgr->m_FileRecBank.m_Bank, 0x20, 0x40, 0x400);

	clu_sb_FreeChunks(&pMgr->m_NetFileRecBank.m_Bank, 1);
	clu_sb_Init2(&pMgr->m_NetFileRecBank.m_Bank, 0x28, 0x80, 0x400);

	for (i = 0; i < 100; i++)
		dl_TieOff(&pMgr->m_ByID[i]);

	pMgr->m_pClientMgr = pClientMgr;
	dl_TieOff(pMgr->m_TreeList.AsLTLink());

	for (i = 0; i < 200; i++)
		dl_TieOff(&pMgr->m_ByName[i]);

	return pMgr;
}


// ----------------------------------------------------------------------- //
//  0x004040f0 -- 179 bytes, 7 calls.  Take one down.
//
//  WHAT PINS IT
//    * the null test wraps EVERYTHING, and `push %ebx; push %ebp; push
//      %edi` sit inside it -- the three extra registers are saved only on
//      the taken path, which is what /O2 does for a body under one guard.
//    * the tree walk reads m_pNext BEFORE the node is freed and keeps it in
//      %ebx, the textbook safe-erase shape.
//    * the record sweep does not tie the buckets off afterwards; it only
//      threads each record onto the bank's free list, which is sb_Free's
//      two stores inlined (servermgr.h carries it as an inline for exactly
//      this reason).
//    * the tail -- derived vptr, member dtor, base vptr, next member, then
//      dfree -- is the compiler's own inline destructor followed by
//      `operator delete`, i.e. one `delete pMgr;` in the source.
//    * IDENTICAL.  The tree walk reads m_pData TWICE (0x0040410f and
//      again after the CloseTree call at 0x0040411e), so there is no node
//      local -- a named one CSE'd it into %edi.  The record sweep reads
//      m_pData ONCE into a local, after m_pNext (0x00404142/0x00404145);
//      writing pCur->m_pData at both stores reloaded it and cost %ebp.
// ----------------------------------------------------------------------- //
void clu_fm_Destroy(CluFileMgr* pMgr)
{
	LTLink*      pCur;
	LTLink*      pNext;
	void*        pRec;
	int          i;

	if (!pMgr)
		return;

	clu_sub_404480(pMgr);

	pCur = pMgr->m_TreeList.m_pNext;
	while (pCur != &pMgr->m_TreeList)
	{
		pNext = pCur->m_pNext;

		clu_fs_CloseTree(((CluTreeNode*)pCur->m_pData)->m_pTree);
		clu_sub_429080(pCur->m_pData);

		pCur = pNext;
	}

	for (i = 0; i < 200; i++)
	{
		pCur = pMgr->m_ByName[i].m_pNext;

		while (pCur != &pMgr->m_ByName[i])
		{
			pNext = pCur->m_pNext;

			// AVP2: stdlith's sb_Free with its two null tests, the record
			// and then the bank (0x00404568 / 0x0040456c).
			pRec = pCur->m_pData;
			clu_sb_FreeQ501(&pMgr->m_FileRecBank.m_Bank, pRec);

			pCur = pNext;
		}
	}

	delete pMgr;
}


// ----------------------------------------------------------------------- //
//  0x004041f0 -- 140 bytes, 3 calls.  Start the manager serving.
//
//  WHAT PINS IT.  The 28 bytes written at 0x0040423a..0x00404236 are one
//  descriptor, and 0x0042f740 copies exactly seven dwords out of it into
//  its object's +0x08, so the struct is five function pointers, the
//  CNetMgr sub-object (CClientMgr + 0x50c, which clientmgr.h already
//  names) and the caller's second argument.  Three of the five slots are
//  the SAME function, 0x00404470 (a bare `ret`), and 0x00404460 is `xor
//  %eax,%eax; ret` -- /OPT:ICF is off in this image, so those really are
//  three references to one stub and not three folded ones.
//
//  The 100-bucket table is tied off AGAIN here, after the constructor
//  already did it.  That is in the bytes; it is not a mistake in the
//  reconstruction.
// ----------------------------------------------------------------------- //
void clu_fm_Init(CluFileMgr* pMgr, int nUnknown)
{
	CluQueueDesc desc;
	int          i;

	if (!pMgr)
		return;

	for (i = 0; i < 100; i++)
		dl_TieOff(&pMgr->m_ByID[i]);

	clu_fs_OpenTree("c:\\de_cache", &pMgr->m_pCacheTree);

	desc.m_pNetMgr = pMgr->m_pClientMgr->m_NetMgr;
	desc.m_nUnknown18 = nUnknown;
	desc.m_pfnOnServerFile = clu_fm_OnServerFile;
	desc.m_pfn04 = clu_sub_404460;
	desc.m_pfn08 = clu_sub_404470;
	desc.m_pfn0c = clu_fm_Empty0c;
	desc.m_pfn10 = clu_fm_Empty10;

	pMgr->m_pQueue = clu_sub_42f740(&desc);
	clu_sub_42f7d0(pMgr->m_pQueue, pMgr);
}


// ----------------------------------------------------------------------- //
//  0x004043b0 -- 89 bytes, 1 call.  Which open tree holds this name?
//
//  WHAT PINS IT.  `sub $0x110,%esp` and nothing else on the frame: the
//  whole frame is the 0x110-byte find record 0x00420d50 fills, the same
//  record src/engine/server/filemgr.cpp calls "the 0x110-byte record".  The
//  answer is the NODE (%edi, the link's m_pData), not the tree, and NULL
//  when the walk runs out -- `xor %eax,%eax` sits on the exhausted path
//  only.
// ----------------------------------------------------------------------- //
CluTreeNode* clu_fm_FindTreeWithFile(LTLink* pTreeList, char* pName)
{
	CluFileInfo  info;
	LTLink*      pCur;
	CluTreeNode* pNode;

	for (pCur = pTreeList->m_pNext; pCur != pTreeList; pCur = pCur->m_pNext)
	{
		pNode = (CluTreeNode*)pCur->m_pData;

		if (clu_fs_GetFileInfo(pNode->m_pTree, pName, &info))
			return pNode;
	}

	return LTNULL;
}


// ----------------------------------------------------------------------- //
//  0x00404280 -- 302 bytes, 8 calls.  The queue's first callback: the
//  server has named a file, find it and remember it under its id.
//
//  WHAT PINS THE SIGNATURE.  It is stored as slot 0 of the descriptor above
//  and is __cdecl with four arguments, of which the THIRD is never read --
//  nothing in the body touches that slot.  The first goes to 0x0042f7c0,
//  which is the queue's user-pointer getter, so it is the queue; the SECOND
//  (0x21c(%esp) at 0x0040428e) is the path -- it reaches the 0x200-byte
//  normaliser and the "Unable to find server file: %s" line.  The fourth is
//  a full uint32: 0x004042e2 passes it to 0x00404410 with no `and $0xffff`,
//  the bucket is an unsigned `xor %edx,%edx; div`, and only the store into
//  the record (`mov %bp,0x10(%esi)`) narrows it.  The caller, ltnew2.cpp's
//  sub_0042f7e0, calls it as (record, name, unknown, id).
//
//  Every exit is `xor %eax,%eax` before the pops, so the answer is a
//  constant zero and not a fall-out of the last call.
//
//  The record's three name fields are written 0x24, 0x1c, 0x18 -- the order
//  a right-to-left chained assignment produces.
// ----------------------------------------------------------------------- //
int clu_fm_OnServerFile(void* pQueue, char* pName, uint32 nUnused, uint32 nID)
{
	CluFileMgr*    pMgr;
	CluTreeNode*   pNode;
	CluNetFileRec* pRec;
	char           szPath[0x200];

	pMgr = (CluFileMgr*)clu_sub_42f7c0(pQueue);

	pNode = clu_fm_FindTreeWithFile(&pMgr->m_TreeList, pName);
	if (!pNode)
	{
		clu_ConPrintf("Unable to find server file: %s", pName);

		if (pMgr->m_pClientMgr->m_pClientShell)
			pMgr->m_pClientMgr->m_pClientShell->OnMessage(1, pName);

		return 0;
	}

	clu_sub_403f90(pName, szPath, 0x200);

	if (clu_sub_404410(pMgr, nID))
		return 0;

	// PH: the bank's own Allocate, called (0x00404788 `lea 0x48(%ebx),%ecx ;
	// call 0x00404fb0`); AvP2 1.0.9.6 expands the struct-bank pop in place.
#ifdef PRIMAL_HUNT
	pRec = (CluNetFileRec*)pMgr->m_NetFileRecBank.CluNetFileRecBank::AllocVoid();
#else
	pRec = (CluNetFileRec*)clu_sb_Allocate(&pMgr->m_NetFileRecBank.m_Bank);
#endif
	memset(pRec, 0, sizeof(CluNetFileRec));

	pRec->m_pName18 = pRec->m_pName = pRec->m_pName24 =
		pMgr->m_StringHolder.AddString(szPath, 1);

	pRec->m_nID = (uint16)nID;
	pRec->m_pTree = pNode->m_pTree;
	pRec->m_nNameLen = (uint16)strlen(szPath);
	pRec->m_Link.m_pData = pRec;

	dl_Insert(&pMgr->m_ByID[nID % 100], &pRec->m_Link);

	return 0;
}


// ----------------------------------------------------------------------- //
//  0x00404410 -- 83 bytes.  The by-id probe declared above: walk the
//  m_ByID bucket for nID % 100 (signed idiv of the promoted word, then the
//  bucket index masked back to a word) and answer the record whose +0x10
//  id matches, or NULL.  The record is answered in %eax straight out of
//  the loop (0x00404444 jumps to the `pop %esi`).
// ----------------------------------------------------------------------- //

CluNetFileRec* clu_sub_404410(CluFileMgr* pFileMgr, uint16 nID)
{
	LTLink*        pHead;
	LTLink*        pCur;
	CluNetFileRec* pRec;
	uint16         iBucket;

	iBucket = nID % 100;
	pHead = &pFileMgr->m_ByID[iBucket];

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		pRec = (CluNetFileRec*)pCur->m_pData;
		if (pRec->m_nID == nID)
			return pRec;
	}

	return NULL;
}


// ----------------------------------------------------------------------- //
//  0x00404480 -- the per-shell reset; IDENTICAL in AvP2 1.0.9.6 (0x004043e0,
//  196 bytes) and Primal Hunt (0x004048e0, 192 bytes): it is the reset
//  (called from clu_fm_Destroy above and cm_OnShellTerm): close the cache
//  tree, then for each of the 100 id buckets unlink every net-file record,
//  give its file record (if any) back to the 0x20-byte bank (+0x28) and the
//  record itself to the 0x28-byte bank (+0x4c), and finally close the
//  message queue (0x0043a700).
//
//  WHAT PINS IT
//    * both gives-back are clu_sb_FreeQ501, the record and then the bank
//      tested (PH 0x00404927/0x0040492e, 0x00404943/0x00404959).
//    * the record (+0x08) is read before the next link (+0x04), and the
//      unlink is ltlink.h's Remove, TieOff included (0x0040493e..).
//    * the bucket walk is a pointer kept in a spilled local and a
//      separate down-counter from 100 (0x0040490f, 0x00404972).
// ----------------------------------------------------------------------- //

void clu_sub_404480(CluFileMgr* pFileMgr)
{
	LTLink*        pHead;
	LTLink*        pCur;
	LTLink*        pNext;
	CluNetFileRec* pRec;
	uint32         nBuckets;

	if (!pFileMgr)
		return;

	if (pFileMgr->m_pCacheTree)
	{
		clu_fs_CloseTree(pFileMgr->m_pCacheTree);
		pFileMgr->m_pCacheTree = LTNULL;
	}

	pHead = pFileMgr->m_ByID;
	nBuckets = 100;
	do
	{
		pCur = pHead->m_pNext;
		while (pCur != pHead)
		{
			pRec = (CluNetFileRec*)pCur->m_pData;
			pNext = pCur->m_pNext;

			if (pRec->m_pFileRec)
				clu_sb_FreeQ501(&pFileMgr->m_FileRecBank.m_Bank, pRec->m_pFileRec);

			pCur->Remove();
			clu_sb_FreeQ501(&pFileMgr->m_NetFileRecBank.m_Bank, pRec);

			pCur = pNext;
		}
		++pHead;
	}
	while (--nBuckets);

	if (pFileMgr->m_pQueue)
	{
		clu_sub_42f780(pFileMgr->m_pQueue);
		pFileMgr->m_pQueue = LTNULL;
	}
}

// ----------------------------------------------------------------------- //
//  0x00404520 -- 172 bytes, 3 calls.  Mount a list of resource names.
//
//  WHAT PINS IT
//    * five arguments; the FIRST is null-tested and everything hangs off
//      it, the third is the count (`jle` -- a signed int), the fourth is
//      written only through a moving pointer that advances in lockstep
//      with the added count, and the fifth takes that count at the end.
//    * fs_OpenTree's out-parameter lands in a stack slot that MSVC placed
//      on top of the first parameter; %esi already holds the manager, so
//      the reuse is free.  Nothing in the source says "reuse a parameter".
//    * the node's m_pData is stored FIRST and the tree SECOND, then the
//      six stores of ltlink.h's AddAfter follow in its exact order.  The
//      source assigns the tree first all the same: that is what makes /O2
//      load the tree (0x0040456e) BEFORE the m_pData store; written in
//      store order the load lands after it.
//    * the type probe runs only when the caller asked for types, but the
//      count and the write pointer advance for every tree that opened.
// ----------------------------------------------------------------------- //
void clu_fm_AddTrees(CluFileMgr* pMgr, char** pNames, int nNames,
                     int* pTypes, int* pnAdded)
{
	CluTreeNode* pNode;
	void*        pTree;
	int          nAdded;
	int          i;

	if (!pMgr)
		return;

	nAdded = 0;

	for (i = 0; i < nNames; i++)
	{
		clu_fs_OpenTree(pNames[i], &pTree);

		if (pTree)
		{
			pNode = (CluTreeNode*)clu_sub_428fd0(sizeof(CluTreeNode));

			pNode->m_pTree = pTree;
			pNode->m_Link.m_pData = pNode;

			dl_Insert(&pMgr->m_TreeList, &pNode->m_Link);

			if (pTypes)
				pTypes[nAdded] = clu_fs_GetTreeType(pTree);

			nAdded++;
		}
	}

	if (pnAdded)
		*pnAdded = nAdded;
}


// ----------------------------------------------------------------------- //
//  0x004045d0 -- 49 bytes.  ILTClient's file-name answer (clientmgr3.cpp's
//  fm_GetName, startload.cpp's fm_GetFilenameQ2B): no manager -> the empty
//  string at 0x004b0ab4 (its ADDRESS, `mov $imm,%eax`); a type-2 identifier
//  -> the id record's +0x1c interned name, or the empty string when the id
//  is unknown; anything else -> the identifier's own +0x04 name.
// ----------------------------------------------------------------------- //

extern char g_CluEmptyString[];                              // 0x004b0ab4

char* clu_fm_GetFilename(CluFileMgr* pFileMgr, CluFileIdent* pIdent)
{
	CluNetFileRec* pRec;

	if (!pFileMgr)
		return g_CluEmptyString;

	if (pIdent->m_nType == 2)
	{
		pRec = clu_sub_404410(pFileMgr, pIdent->m_nID);
		if (pRec)
			return pRec->m_pName;

		return g_CluEmptyString;
	}

	return pIdent->m_pName;
}


// ----------------------------------------------------------------------- //
//  0x00404680 -- 526 bytes, 5 calls.  The manager's main lookup: turn a
//  file identifier into a cached record, creating one if the name resolves
//  in some open tree.
//
//  WHAT PINS IT
//    * three arguments; the third is read with `mov 0x120(%esp),%dl` and
//      compared against 0xff -- a BYTE, and 0xff is its wildcard.
//    * the identifier is the same three-field block 0x004045d0 reads: an
//      int type at +0x00 (2 selects the id path), a char* at +0x04 and a
//      uint16 at +0x08.
//    * the path hash multiplies each character by its INDEX (`movsbl` then
//      `imul %ecx`), sums into a uint32 and takes it modulo 200 with an
//      unsigned `div`.  The 0x7d00 bound is a separate guard on the index,
//      not on the string.
//    * the in-bucket compare is a THREE-part match: the cached length
//      first (a uint16 compare, cheap), then an inline strcmp, then the
//      flag byte with 0xff on either side matching anything.
//    * the new record's stores come out in the order m_pTree, m_nNameLen,
//      m_nUnknown14, m_Link.m_pData, m_nFlag, m_pName -- which is the
//      order they are written below, because VC6 emits one store per
//      assignment in source order.
// ----------------------------------------------------------------------- //
CluFileRec* clu_fm_GetFileRec(CluFileMgr* pMgr, CluFileIdent* pIdent, uint8 nFlag)
{
	char           szName[256];
	CluNetFileRec* pNetRec;
	CluFileRec*    pRec;
	CluTreeNode*   pNode;
	LTLink*        pBucket;
	LTLink*        pCur;
	char*          pName;
	char*          pChar;
	uint32         nHash;
	uint32         nLen;
	uint32         i;

	if (!pMgr)
		return LTNULL;

	if (pIdent->m_nType == 2)
	{
		pNetRec = clu_sub_404410(pMgr, pIdent->m_nID);
		if (!pNetRec)
			return LTNULL;

		if (pNetRec->m_pTree == pMgr->m_pCacheTree)
		{
			pRec = pNetRec->m_pFileRec;
			if (pRec)
			{
				if (nFlag == 0xff)
					return pRec;

				if (pRec->m_nFlag == 0xff)
					return pRec;

				if (pRec->m_nFlag == nFlag)
					return pRec;
			}

			pName = pIdent->m_pName;
		}
		else
		{
			pName = pNetRec->m_pName;
		}
	}
	else
	{
		pName = pIdent->m_pName;
	}

	if (!pName)
		return LTNULL;

	clu_sub_403f90(pName, szName, 256);

	nHash = 0;
	nLen = 0;

	pChar = szName;
	for (i = 0; i < 32000; i++)
	{
		if (*pChar == 0)
			break;

		nHash += *pChar * i;
		pChar++;
		nLen++;
	}

	pBucket = &pMgr->m_ByName[nHash % 200];

	for (pCur = pBucket->m_pNext; pCur != pBucket; pCur = pCur->m_pNext)
	{
		pRec = (CluFileRec*)pCur->m_pData;

		if (pRec->m_nNameLen == (uint16)nLen && strcmp(pRec->m_pName, szName) == 0)
		{
			if (pRec->m_nFlag == nFlag)
				return pRec;

			if (nFlag == 0xff)
				return pRec;

			if (pRec->m_nFlag == 0xff)
				return pRec;
		}
	}

	pNode = clu_fm_FindTreeWithFile(&pMgr->m_TreeList, szName);
	if (!pNode)
		return LTNULL;

	// PH: the bank's own Allocate, called, as in clu_fm_OnServerFile; AvP2
	// 1.0.9.6 expands the struct-bank pop in place.
#ifdef PRIMAL_HUNT
	pRec = (CluFileRec*)pMgr->m_FileRecBank.CluFileRecBank::AllocVoid();
#else
	pRec = (CluFileRec*)clu_sb_Allocate(&pMgr->m_FileRecBank.m_Bank);
#endif
	memset(pRec, 0, sizeof(CluFileRec));

	pRec->m_pTree = pNode->m_pTree;
	pRec->m_nNameLen = (uint16)nLen;
	pRec->m_nUnknown14 = 0xffff;
	pRec->m_Link.m_pData = pRec;
	pRec->m_nFlag = nFlag;
	pRec->m_pName = pMgr->m_StringHolder.AddString(szName, 1);

	dl_Insert(pBucket, &pRec->m_Link);

	return pRec;
}


// ----------------------------------------------------------------------- //
//  0x004048c0 / 0x00404950 -- the run continues past 0x0040488d: after
//  0x00404890 (lt2new2.cpp's sub_00404890) come the two workers ILTClient's
//  OpenFile (slot 42, 0x00406610) and CopyFile (slot 43, 0x004066f0) call
//  with m_pFileMgr first.  Both walk m_TreeList exactly as
//  clu_fm_FindTreeWithFile does and open through 0x00420ec0
//  (fs_OpenStreamInTree: tree, name, flags -- the flags a literal 0).
// ----------------------------------------------------------------------- //
#include "iltstream.h"

// fsstream.cpp's fs_OpenStreamInTree, the tree type forward-declared.
class COpenFileTree;
ILTStream* fs_OpenStreamInTree(COpenFileTree* pTree, char* pFilename,
                               int nFlag);                   // 0x00420ec0
#define clu_fs_OpenStreamInTree(pTree, pName, nFlags) \
	fs_OpenStreamInTree((COpenFileTree*)(pTree), (pName), (nFlags))
int clu_fs_CopyStreamToFile(ILTStream* pStream,
                            const char* pszDestFile);        // 0x00421670

// 0x004048c0 -- 132 bytes.  Open the file an ident names.  Type 2 goes by
// server id through 0x00404410 (the uint16 at +0x08 pushed unwidened) and
// opens the id record's own tree/name (+0x0c/+0x1c); anything else tries
// every tree in order, and type 0 finally tries the "c:\de_cache" tree at
// +0x6c.  Every miss shares the one `xor %eax,%eax` at 0x0040493e.
ILTStream* clu_fm_OpenFile(CluFileMgr* pMgr, CluFileIdent* pIdent)
{
	LTLink*        pCur;
	CluTreeNode*   pNode;
	CluNetFileRec* pRec;
	ILTStream*     pStream;

	if (!pMgr)
		return LTNULL;

	if (pIdent->m_nType == 2)
	{
		pRec = clu_sub_404410(pMgr, pIdent->m_nID);
		if (!pRec)
			return LTNULL;

		return clu_fs_OpenStreamInTree(pRec->m_pTree, pRec->m_pName, 0);
	}

	for (pCur = pMgr->m_TreeList.m_pNext; pCur != &pMgr->m_TreeList; pCur = pCur->m_pNext)
	{
		pNode = (CluTreeNode*)pCur->m_pData;
		pStream = clu_fs_OpenStreamInTree(pNode->m_pTree, pIdent->m_pName, 0);
		if (pStream)
			return pStream;
	}

	if (pIdent->m_nType == 0)
	{
		pStream = clu_fs_OpenStreamInTree(pMgr->m_pCacheTree, pIdent->m_pName, 0);
		if (pStream)
			return pStream;
	}

	return LTNULL;
}

// 0x00404950 -- 109 bytes.  Copy the first tree's copy of a file out to
// disk.  LT_ERROR (1) for no manager, LT_NOTFOUND (0x3d) when no tree has
// it; otherwise the stream is released BEFORE the copy's answer is tested
// (`mov %eax,%edi; call *0x4(%edx)`) and that answer becomes `sete`.
LTRESULT clu_fm_CopyFile(CluFileMgr* pMgr, char* pszSourceFile, char* pszDestFile)
{
	LTLink*      pCur;
	CluTreeNode* pNode;
	ILTStream*   pStream;
	int          bCopied;

	if (!pMgr)
		return LT_ERROR;

	for (pCur = pMgr->m_TreeList.m_pNext; pCur != &pMgr->m_TreeList; pCur = pCur->m_pNext)
	{
		pNode = (CluTreeNode*)pCur->m_pData;
		pStream = clu_fs_OpenStreamInTree(pNode->m_pTree, pszSourceFile, 0);
		if (pStream)
		{
			bCopied = clu_fs_CopyStreamToFile(pStream, pszDestFile);
			pStream->Release();
			return bCopied ? LT_OK : LT_ERROR;
		}
	}

	return LT_NOTFOUND;
}

#ifdef PRIMAL_HUNT
// PH: 0x00404ed0 -- 28 bytes.  Does any tree have the file?  The client half
// of ILTCSBase's new slot 45: CLTClient::DoesFileExist (0x00407320) hands it
// the client manager's file manager (+0x1644) and the name.  One call to the
// tree search (0x00404810), then `neg; sbb; and $0xc3; add $0x3d` -> LT_OK
// or LT_NOTFOUND.  (Name ours; lt2new2.cpp calls it.)
// 0x00404e30 between CopyFile and this is the slot-44 GetFileInfo, which
// smgrmisc.cpp's fs_GetFileInfoQ468 already reproduces byte for byte.
LTRESULT clu_fm_FileExistsQ511(CluFileMgr* pMgr, char* pszFilename)
{
	return clu_fm_FindTreeWithFile(&pMgr->m_TreeList, pszFilename) ? LT_OK : LT_NOTFOUND;
}
#endif
