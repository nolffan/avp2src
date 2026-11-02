// ======================================================================= //
//
//  w11_46c000.cpp -- scratch reconstruction, cluster
//  "0x0046eb50 / 0x0046f320 / 0x0046fd00 + 0x00410035 / 0x00411560 /
//   0x00411600 / 0x00412820 / 0x00413f1f".
//
//  Build /O2 (= /Og /Oi /Ot /Oy /Ob1), the lithtech.exe leg.
//  Ground truth read throughout is the retail lithtech.exe.
//
//  ---------------------------------------------------------------------
//  FIVE OF THE EIGHT ROWS ARE NOT NEW FUNCTIONS.  In one line each:
//
//    0x0046eb50  ALREADY WRITTEN -- CLTServer::SetObjectFilenames,
//                src/engine/server/servermessage.cpp:283.  server.dll
//                0x1002ef29 is IDENTICAL,
//                so this is an o2gap row: /O2 EXPANDS
//                ObjectCreateStruct::Clear() twice where /Os CALLS it
//                twice (server 0x1002ef3b + 0x1002ef46 -> 0x10001bd7).
//    0x0046f320  ALREADY WRITTEN -- CLTServer::EndMessage2,
//                servermessage.cpp:988.  server.dll 0x1002f43a is a
//                302/302 COUNT-EXACT near, a
//                cross-jump host lottery the source note already records.
//    0x0046fd00  ALREADY WRITTEN -- sm_GatherContainers,
//                src/engine/server/smgr.cpp:8934.  server.dll 0x1002faaa
//                is IDENTICAL: o2gap.
//    0x00410035  NOT A FUNCTION -- the last displacement byte of
//                `mov al,BYTE PTR [edi+0x130]` at 0x00410030, itself an
//                interior block of CClientMgr::StartGame (0x0040fd90,
//                real extent 0x0040fd90..0x00410350).
//    0x00413f1f  NOT A FUNCTION -- the tail half of
//                CClientShell::DoLoadWorld, whose real extent is
//                0x00413d30..0x004141f3 (495 + 724 = 1219 exactly).
//
//  THE THREE REAL ROWS ARE BELOW.  All three are client-side; none has
//  a server.dll counterpart, so there is no /O1 oracle for any of them.
//  Every one belongs in src/engine/lithtech/.  Symbols introduced here carry the Q46C tag.
//
// ======================================================================= //

#include <string.h>                  // memset  (intrinsic under /Oi)
#include <stdio.h>                   // sprintf
#include <new.h>                     // placement new

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef long           int32;
typedef unsigned long  LTBOOL;

#define LTNULL 0


// ======================================================================= //
//  Shared declarations
// ======================================================================= //

// 0x004b0ab8 / 0x004b13e8 -- the command line clu_BuildCommandLine built.
// clientwin.cpp already declares both at these addresses (g_pCluArgList /
// g_nCluArgCount) and ltnew2.cpp the same pair under g_pLTArgList /
// g_nLTArgCount.  Same objects; the names below are clientwin.cpp's.
extern char** g_pCluArgList;                                  // 0x004b0ab8
extern int    g_nCluArgCount;                                 // 0x004b13e8

// 0x004b2f00 -- the client's global ConsoleState, clientmgr2.cpp's opaque
// ConsoleStateRef.  NOT the same object as CClientMgr+0x1268 below: that
// one is a second ConsoleState the manager owns, and this one is the
// file-scope one whose +0x44 holds the CClientMgr (set at 0x0040fb72).
// Declared as the one definition's real type (src/engine/server's
// ConsoleState); only its address is taken, under the view type below.
class ConsoleStateRef;
class ConsoleState;
extern ConsoleState g_ClientConsoleState;                     // 0x004b2f00

// src/engine/server/console.cpp's own entry points, at the client's
// addresses.  The signatures are servermgr.h's, unchanged.
void con_RunString3(ConsoleStateRef* pState, char* pString, int nSource);
                                                              // 0x0041ac00

// 0x00428fd0 / 0x00429080 -- lt2new1.cpp names these dalloc / dfree and
// clientwin.cpp declares the same pair as clu_sub_428fd0 / clu_sub_429080.
// The ConsoleState fields they land in are function pointers, so they are
// declared here with the signatures those fields carry
// (src/engine/server/servermgr.h, ConsoleState +0x28 and +0x2c).
void* dalloc(uint32 nSize);                                   // 0x00428fd0
void  dfree(void* pMem);                                      // 0x00429080

// 0x0041b190 -- con_Init, src/engine/server/console.cpp:289.  Declared on
// the client's opaque state for the same reason clientmgr2.cpp declares
// con_Term (0x0041b200) as `void sub_41b200(void* pSub1268)`: the real
// ConsoleState lives in a shared server header.
void con_InitQ46C(void* pState);                              // 0x0041b190

// 0x004041f0 -- clientwin.cpp's clu_fm_Init, reconstructed there as
// `void clu_fm_Init(CluFileMgr* pMgr, int nUnknown)`.  Declared through
// void* here because CluFileMgr is that file's class.
void clu_fm_InitQ46C(void* pFileMgr, uint32 nConnID);         // 0x004041f0

// 0x00490c20 -- sb_Grow, the struct bank's block adder.  Same function
// src/engine/server/objbank.cpp and structbank.cpp declare on the server
// side (0x1003d240).
struct StructBankQ46C;
void sb_GrowQ46C(StructBankQ46C* pBank, uint32 nCount);       // 0x00490c20


// ======================================================================= //
//  THE STRUCT BANK, as ltnew2.cpp reads it (SObjectBank, 0x1c bytes).
//  0x00490b50 writes +0x00 = size, +0x04 = size rounded up to 4, +0x08 =
//  items per block and zeroes +0x0c..+0x18; 0x00490bd0 walks the block
//  list at +0x14 and zeroes +0x0c,+0x10,+0x14,+0x18.
// ======================================================================= //

struct StructBankQ46C
{
	uint32 m_nItemSize;        // 0x00
	uint32 m_nAlignedSize;     // 0x04
	uint32 m_nItemsPerBlock;   // 0x08   read at 0x00412832 as sb_Grow's count
	uint32 m_Unknown0c;        // 0x0c
	uint32 m_Unknown10;        // 0x10
	void*  m_pBlocks;          // 0x14
	void*  m_pFreeList;        // 0x18   read at 0x00412822 / 0x00412839
};


// ======================================================================= //
//  The client's second ConsoleState, at CClientMgr+0x1268.
//
//  The layout is src/engine/server/servermgr.h's ConsoleState exactly; only
//  the two fields 0x00411560 writes are named here, the rest is the pad
//  that header spells out (m_pSavers .. m_hStringHash).  What pins the SIZE
//  is the clear itself: 0x0041157c..0x0041158b is `lea edx,[esi+0x1268] ;
//  mov ecx,0x11 ; xor eax,eax ; mov edi,edx ; rep stos` -- 17 dwords, 0x44
//  bytes, which is the same 0x44 src/engine/server/console.cpp:847 clears
//  in cc_InitState and one past m_hStringHash at +0x40.  The eight bytes
//  servermgr.h pads past that "belong to whoever embedded it" (that file's
//  own words) and are NOT cleared here either.
// ======================================================================= //

class ConsoleStateQ46C
{
public:
	uint8  m_pad_00[0x28];                   // 0x00  m_pSavers .. m_CommandHead, m_pPrint
	void* (*m_pAlloc)(uint32 nSize);         // 0x28  <- 0x0041158e, dalloc
	void  (*m_pFree)(void* pMem);            // 0x2c  <- 0x00411598, dfree
	uint8  m_pad_30[0x44 - 0x30];            // 0x30  m_pOnCreate .. m_hStringHash
};


// ======================================================================= //
//  CClientMgr, the fields 0x00411560 reaches.
//
//  Every offset below is already in src/engine/lithtech/clientmgr2.cpp:
//  m_pClientShell2 at 0x150c and m_pFileMgr at 0x1534 are its CClientMgrTail,
//  and 0x1268 is the start of its `m_Sub1268`.  The only field this run adds
//  is 0x14f0, which falls INSIDE that pad.
//
//  CClientShell+0x34 is clientmgr2.cpp's m_nConnID (its line 161), the third
//  argument of every send through the manager's CNetMgr.
// ======================================================================= //

class CClientShellQ46C
{
public:
	uint8  m_pad_00[0x34];                   // 0x00
	uint32 m_nConnID;                        // 0x34  <- 0x00411572
};

class CClientMgrQ46C
{
public:
	uint8            m_pad_0000[0x1268];
	ConsoleStateQ46C m_ConsoleState;         // 0x1268
	uint8            m_pad_12ac[0x14f0 - 0x12ac];
	// 0x0041155a's last store.  Nothing else in this run reads it; the
	// name is ours, from the address.
	uint32           m_Unknown14f0;          // 0x14f0
	uint8            m_pad_14f4[0x150c - 0x14f4];
	CClientShellQ46C* m_pClientShell2;       // 0x150c  (clientmgr2.cpp)
	uint8            m_pad_1510[0x1534 - 0x1510];
	void*            m_pFileMgr;             // 0x1534  (clientmgr2.cpp)
};


// ----------------------------------------------------------------------- //
//  0x00411560, 87 bytes -- the manager's half of the shell STARTUP, and the
//  exact mirror of cm_OnShellTerm (0x004115c0), which clientmgr2.cpp:571
//  already carries.  Its one caller is CClientMgr::StartGame at 0x0041005c,
//  immediately after `mov %esi,0x150c(%ebx)` installs the new shell -- which
//  is why the shell is read back out of the manager here instead of being
//  passed in.  cdecl, one argument (`push %ebx ; call ; add $0x4,%esp`).
//
//  WHAT IS READ OFF THE BYTES, not guessed:
//
//    * the file manager and the shell's connection id are both loaded before
//      either is pushed (0x00411566 shell, 0x0041156c file manager,
//      0x00411572 the id), and the push order `push %ecx ; push %edx` makes
//      the id argument two -- clu_fm_Init(pFileMgr, nConnID), matching
//      clientwin.cpp's two-argument reconstruction of 0x004041f0.
//    * `lea 0x1268(%esi),%edx` is computed ONCE and used for both the clear
//      and con_Init's argument, so it is one lvalue in the source and not
//      two expressions -- the same reading ltnew2.cpp makes of
//      CClass4A14A8::Reset.
//    * the clear is an inline `rep stos`, i.e. the INTRINSIC memset, which
//      /O2 gives for free (/Oi); server.dll's cc_InitState needs the same
//      call spelled the same way and console.cpp already writes it as
//      `memset(pState, 0, 0x44)`.
//    * the allocator pair is stored AFTER the clear (0x0041158e / 0x00411598
//      against the `rep stos` at 0x0041158b), so they are statements below
//      it and not an initialiser the clear would have wiped.
//    * `add $0xc,%esp` at 0x004115a7 cleans BOTH calls at once -- five
//      dwords' worth minus clu_fm_Init's two already-standing pushes -- so
//      nothing between them forces a fixed %esp.
// ----------------------------------------------------------------------- //
void cm_OnShellInitQ46C(CClientMgrQ46C* pClientMgr)
{
	clu_fm_InitQ46C(pClientMgr->m_pFileMgr,
	                pClientMgr->m_pClientShell2->m_nConnID);

	memset(&pClientMgr->m_ConsoleState, 0, sizeof(ConsoleStateQ46C));

	pClientMgr->m_ConsoleState.m_pAlloc = dalloc;
	pClientMgr->m_ConsoleState.m_pFree  = dfree;

	con_InitQ46C(&pClientMgr->m_ConsoleState);

	pClientMgr->m_Unknown14f0 = 0;
}


// ----------------------------------------------------------------------- //
//  0x00411600, 109 bytes -- run every `+command` the command line carries.
//
//  Two call sites, both in CClientMgr::Init: 0x0040fb80 with stage 2 and
//  0x0040fbc5 with stage 1, each preceded by the same stage handed to
//  0x00411670.  cdecl, two arguments, and ARGUMENT ONE IS NEVER READ -- the
//  body's only frame reference above the locals is `mov 0x210(%esp),%edi`
//  at 0x0041161a, which is argument two.  (cm_Sub410610 in clientmgr2.cpp
//  is the same shape: a parameter the body ignores costs nothing and is
//  left in place.)
//
//  WHAT IS READ OFF THE BYTES:
//
//    * the buffer is 0x200 bytes (`sub $0x200,%esp`, `lea 0x10(%esp),%edx`
//      at 0x00411630 landing on its base).
//    * the loop is a COUNTING for, not a countdown: it closes with
//      `inc %esi ; cmp %esi,%edx ; jl` at 0x0041165e and tests
//      `dec %eax ; test %eax,%eax ; jle` once above it.  Both compares are
//      SIGNED, so the index and the count are `int`.
//    * the bound is re-read from 0x004b13e8 on EVERY trip (0x00411658) and
//      the vector from 0x004b0ab8 is re-read after the two calls
//      (0x0041164f), so neither global is cached in a local.
//    * `cmp $0x2b,(%eax)` is a BYTE compare against '+', and the pointer is
//      only then incremented (0x0041162d) -- so the test reads element 0 and
//      the argument passed on is element 1 forward.
//    * the format at 0x004a84e0 is "(%s) (%s)" and the call at 0x004929c4 is
//      the CRT sprintf (`_output` with INT_MAX, 0x004929ed).
//    * the console entry is 0x0041ac00 = con_RunString3 on the FILE-SCOPE
//      state 0x004b2f00, not on the manager's.
// ----------------------------------------------------------------------- //
void cm_RunPlusCommandsQ46C(CClientMgrQ46C* /*pClientMgr*/, int nSource)
{
	char buf[512];
	int  i;

	for (i = 0; i < g_nCluArgCount - 1; i++)
	{
		if (g_pCluArgList[i][0] == '+')
		{
			sprintf(buf, "(%s) (%s)", g_pCluArgList[i] + 1,
			        g_pCluArgList[i + 1]);

			con_RunString3((ConsoleStateRef*)&g_ClientConsoleState, buf, nSource);
		}
	}
}


// ======================================================================= //
//  0x00412820, 104 bytes -- the SHARED-TEXTURE bank hands out one record.
//
//  WHAT PINS THE BANK.  0x00412820 is slot 1 of the four-slot vftable
//  0x004a1cf4, and 0x004a1cf4 is written as
//  a vptr immediate at exactly three sites: 0x004110d5 (`movl $0x4a1cf4,
//  0x113c(%ebx)`, then `push $1 ; lea 0x1140(%ebx),%ecx ; call 0x00490bd0`,
//  then `movl $0x4a14c8,0x113c(%ebx)`), 0x00412bbc and 0x00412eda.  So the
//  object is at CClientMgr+0x113c with its StructBank at +0x1140, and
//  0x004a14c8 is the abstract base table ltnew2.cpp and netmgr.cpp already
//  identify (three _purecall slots, 0x004928e7).  clientmgr3.cpp:487 models
//  the object as CCmgBank113C; this is its Allocate.
//
//  WHAT PINS THE ELEMENT, and it is not a guess:
//
//    * the bank's free list is bank+0x18 = CClientMgr+0x1158, and
//      src/engine/lithtech/clienttexmgr.cpp:83 already reads
//      `mov 0x1158(%edi),%eax ; mov %eax,(%esi) ; mov %esi,0x1158(%edi)`
//      at 0x0041ffc0 as "a free-list push and nothing else" for a
//      SharedTexture.  Same list, same bank.
//    * slot 3 (0x00412bd0) re-inits the bank with `0x00490b50(&m_Bank,
//      0x34,0x20)` -- element size 0x34, which is clienttexmgr.cpp's
//      "the whole object is 0x34 bytes ... and the bank's element size".
//    * the twelve stores this function expands at 0x00412852..0x00412881
//      are INSTRUCTION FOR INSTRUCTION the ones 0x00401b90 expands at
//      0x00401ba0..0x00401bd9 (Bink, sub-object at +0x28) -- same calls to
//      0x0045a350 and 0x0045a370, same +0x08/+0x0c/+0x18/+0x1c/+0x20/+0x24
//      run, same +0x28, same 0x32-then-0x30 word pair, and the same three
//      untouched words at +0x10, +0x14 and +0x2c.
//
//  So the element is SharedTexture and the constructor below is NOT new
//  work: it is src/engine/lithtech/binkvideo.cpp:241's SharedTextureK(),
//  copied here verbatim under a tagged name.  binkvideo.cpp derived the
//  0x32-before-0x30 order from the same bytes at 0x00401b90, which is what
//  retires the one coin-flip this function had.
//
//  ONE TEMPLATE, TWELVE INSTANTIATIONS.  0x00411d80, 0x00411dc0, 0x00411e00,
//  0x00411e40, 0x00411e80, 0x00411ec0, 0x00411f00, 0x00411f40, 0x00411f80,
//  0x00411fc0 and 0x004128d0 are the same 58 bytes differing only in the
//  constructor address they end on (clientmgr2.cpp:1134 lists them).  This
//  one is that code with the element's constructor EXPANDED rather than
//  called, which is the only reason it needs a zero register: `cmp %ebx,%esi`
//  here against their `test %esi,%esi` is allocation, not source.
//
//  THE NULL TEST at 0x0041284b is object_bank.h's `if (pRet)` around the
//  placement new (ObjectBank::Allocate).  The +0x1c/+0x20 pair is an inline
//  memset and +0x32 a whole-word store of both bitfields, as binkvideo.cpp
//  found at 0x00401b90.
// ======================================================================= //

// The eight-byte sub-object at the texture's +0x00: constructor 0x0045a350
// (`push $0 ; call 0x0045a370 ; return this`), Init 0x0045a370
// (`{ m_p00 = 0; m_p04 = arg; return 1; }`), Release 0x0045a360 (a bare
// `jmp 0x0045a390`).  binkvideo.cpp:199 and lt2new3.cpp:772 already declare
// it at these addresses; the name and the members are binkvideo.cpp's.
// Both members stay OUT of class: /Ob1 never expands a non-template member
// declared out-of-class, and the shipped code CALLS both.
class CBankLinkQ46C
{
public:
	CBankLinkQ46C();                         // 0x0045a350
	LTBOOL Init(void* pOwner);               // 0x0045a370
	void   Release();                        // 0x0045a360

public:
	void* m_pNextFree;                       // 0x00
	void* m_pOwner;                          // 0x04
};

class TextureDataQ46C;

class LTLinkQ46C
{
public:
	LTLinkQ46C* m_pPrev;                     // 0x00
	LTLinkQ46C* m_pNext;                     // 0x04
	void*       m_pData;                     // 0x08
};

// src/engine/lithtech/binkvideo.cpp:218's SharedTextureK, field for field;
// clienttexmgr.cpp's SharedTextureView is the same class from the other end.
class SharedTextureQ46C
{
public:
	SharedTextureQ46C();

public:
	CBankLinkQ46C     m_Bank;                // 0x00
	TextureDataQ46C*  m_pEngineData;         // 0x08
	void*             m_pRenderData;         // 0x0c
	LTLinkQ46C        m_Link;                // 0x10  +0x10/+0x14 untouched
	uint32            m_Unknown1c[2];        // 0x1c, 0x20
	void*             m_pFileRef;            // 0x24
	uint32            m_Unknown28;           // 0x28
	uint32            m_Unknown2c;           // 0x2c  untouched
	uint16            m_nFrameCode;          // 0x30
	uint16            m_nRefCount : 15;      // 0x32
	uint16            m_bMarked   : 1;       // 0x32 bit 15
};

// VERBATIM from binkvideo.cpp:241.  Do not diverge from that copy: it is
// what 0x00401b90 and 0x0048aba0 expand, and it is what 0x00412852 expands.
inline SharedTextureQ46C::SharedTextureQ46C()
{
	m_pEngineData  = LTNULL;                             // 0x0041285e
	m_pRenderData  = LTNULL;                             // 0x00412861
	m_Link.m_pData = this;                               // 0x00412864
	memset(&m_Unknown1c, 0, sizeof(m_Unknown1c));        // 0x00412867
	m_pFileRef     = LTNULL;                             // 0x0041286f
	m_Bank.Init(this);                                   // 0x00412872
	m_Unknown28    = 0;                                  // 0x00412877
	m_nRefCount    = 0;                                  // 0x0041287a
	m_bMarked      = 0;                                  //   whole-word store
	m_nFrameCode   = 0;                                  // 0x0041287e
}

// struct_bank.h's inline sb_Allocate (the release build), over this file's
// StructBank.  Shipped holds &m_Bank in %edi (`lea 0x4(%ecx),%edi`,
// 0x00412826) and gives the grow-failure `return 0` its own epilogue
// (0x00412843) -- the shape of an expanded inline helper that returns, not
// of a body written into Allocate.  object_bank.h's ObjectBank::Allocate is
// the caller's shape: `if (pRet) new(pRet) T;`.
inline void* sb_AllocateQ46C(StructBankQ46C* pBank)
{
	void* pRet;

	pRet = pBank->m_pFreeList;
	if (!pRet)
	{
		sb_GrowQ46C(pBank, pBank->m_nItemsPerBlock);
		pRet = pBank->m_pFreeList;
		if (!pRet)
			return 0;
	}

	pBank->m_pFreeList = *(void**)pRet;
	return pRet;
}

class CCmgBank113CQ46C
{
public:
	SharedTextureQ46C* Allocate();           // 0x00412820, vftable slot 1
	void               Free(void* pRec);     // 0x00412890, vftable slot 2
	void               Term();               // 0x00412bd0, vftable slot 3

	void*             m_pVtable;             // 0x00
	StructBankQ46C    m_Bank;                // 0x04
	uint32            m_Unknown20;           // 0x20
};

// Out of class, so /Ob1 cannot expand it anywhere.  The shipped body is a
// standalone COMDAT reached only through the vftable: a sweep of every
// e8/e9 displacement in .text finds no `call` to 0x00412820 at all.
SharedTextureQ46C* CCmgBank113CQ46C::Allocate()
{
	SharedTextureQ46C* pRet;

	pRet = (SharedTextureQ46C*)sb_AllocateQ46C(&m_Bank); // 0x00412822..0x0041284d
	if (pRet)                                            // 0x0041284b
	{
		::new(pRet) SharedTextureQ46C;                   // 0x00412852 onwards
	}

	return pRet;
}


// 0x00490bd0 / 0x00490b50 -- struct_bank.h's sb_Term2 and sb_Init, the two
// calls every Term in this bank family makes (clientmgr2.cpp's 0x00412bf0
// reading).
void sb_Term2Q46C(StructBankQ46C* pBank, int bCheckObjects);  // 0x00490bd0
void sb_InitQ46C(StructBankQ46C* pBank, uint32 nStructSize, uint32 nCacheSize); // 0x00490b50

// 0x00412890, 28 bytes -- vftable 0x004a1cf4 slot 2, object_bank.h's
// Free(T*): the element's teardown, then struct_bank.h's inline sb_Free
// (`mov 0x1c(%esi),%eax ; mov %eax,(%edi) ; mov %edi,0x1c(%esi)` -- the
// free head at this+0x1c == m_Bank+0x18).  The teardown is the texture's
// only non-trivial member, the +0x00 bank link, whose 0x0045a360 is CALLED
// with the record itself in %ecx.  `ret $4`: one argument.
void CCmgBank113CQ46C::Free(void* pRec)
{
	((SharedTextureQ46C*)pRec)->m_Bank.Release();

	*(void**)pRec     = m_Bank.m_pFreeList;
	m_Bank.m_pFreeList = pRec;
}

// 0x00412bd0, 27 bytes -- vftable 0x004a1cf4 slot 3, object_bank.h's Term:
// sb_Term2(&m_Bank, 1), then sb_Init(&m_Bank, sizeof(T), 32) with
// sizeof(SharedTexture) == 0x34 (the immediate at 0x00412bde).  The same
// body as the identical 0x00412bf0 (0x0c) and the 0x00412910.. run.
void CCmgBank113CQ46C::Term()
{
	sb_Term2Q46C(&m_Bank, 1);
	sb_InitQ46C(&m_Bank, sizeof(SharedTextureQ46C), 32);
}

// ======================================================================= //
//  Compile-time proof of the two layouts this file invents nothing about.
//  Both are read straight off the shipped bytes (0x44 from the `rep stos`
//  count at 0x00411582, 0x34 from sb_Init's size argument at 0x00412bde),
//  so if either ever drifts the build stops here rather than producing a
//  near.
// ======================================================================= //

typedef int SizeCheckConsoleStateQ46C[(sizeof(ConsoleStateQ46C)  == 0x44) ? 1 : -1];
typedef int SizeCheckSharedTextureQ46C[(sizeof(SharedTextureQ46C) == 0x34) ? 1 : -1];
