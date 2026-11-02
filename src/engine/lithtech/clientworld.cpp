// *********************************************************************** //
//
//  MODULE:   LIGHT (.CPP)   — staging translation unit (name ours)
//
//  PURPOSE:  Small functions read out of lithtech.exe's 0x423c00..0x42da50
//            run.  Everything here was read off the retail lithtech.exe;
//            nothing is carried over from server.dll except the DECLARATIONS
//            each comment names.
//
//  ORGANISATION WARNING — read before compiling this into anything:
//            the run is not one translation unit.  It is at least six, and
//            three of them are shared engine sources the client build ALREADY
//            compiles (build/engine.sh's lithtech line pulls ..\server\dsi.cpp,
//            ..\server\dhash.cpp, ..\server\console.cpp, ..\server\netmgr.cpp
//            and the rest).  Bodies whose home is one of those files are NOT
//            defined here — defining them would duplicate a symbol already in
//            the link.  They are listed in the ATTRIBUTION block below with
//            the proof, so the run can be credited without being rewritten.
//
//  ONE FINDING THAT AFFECTS EVERY VTABLE IN THIS IMAGE:
//            /OPT:ICF IS ON in lithtech.exe.  CBaseDriver's vtable at
//            0x004a223c names 0x0042d190 in EIGHT different slots, 0x0042d180
//            in two, 0x0042d1b0 in two, 0x00432030 in two and 0x00432040 in
//            two.  Eight distinct virtuals of one class cannot share an
//            address unless the linker folded them, and server.dll's notes
//            record ICF as OFF there — so the two images differ in this, and
//            an address in lithtech.exe does NOT identify which source file
//            the surviving COMDAT came from.
//
// *********************************************************************** //

#include <stdlib.h>
#include <windows.h>
#include "ltbasedefs.h"
#include "ltlink.h"
#include "cluglob.h"                // g_CluClientGlob, 0x004b0ac0

// windows.h renames engine methods out from under us otherwise; lthread.h
// does the same thing for the same reason.
#undef PostMessage
#undef GetMessage
#undef PeekMessage
#undef CopyFile
#undef PlaySound
#undef GetClassName


// ---- ATTRIBUTION ONLY: bodies already in the client's link -------------- //
//
// Each of these is in the run and enumerated, and each is the client's
// compile of a shared source the client build already lists.  They need a
// name, not a new body.
//
//  0x00428ef0  mem_Init            — dsi.cpp.  Byte for byte the shape
//              dsi.cpp already carries: `if (g_nMemRefs == 0)` (0x004b2fe4),
//              g_pMemBlocks = 0 (0x004b2fd8), _set_new_handler (0x00493ec0)
//              into 0x004b2fd0, then the four dwords 0x004b2fcc, 0x004b2fe0,
//              0x004b2fd4, 0x004b2fdc cleared IN THAT ORDER — the asymmetric
//              order dsi.cpp's comment already records — then the increment.
//              Not enumerated; listed because it is what identifies its
//              neighbours.
//  0x00428f40  mem_NewHandler      — dsi.cpp.  `dsi_OnOutOfMemory(); return
//              0;`, where 0x0042e560 is dsi_OnOutOfMemory.  The proof is that
//              0x00428efc pushes THIS address into _set_new_handler.
//              (enumerated, 8 bytes, src=cb)
//  0x00429170  memtrack_Init       — dsi.cpp.  sb_Init(0x004b3030, 0x20,
//              0x20) under a zero test on 0x004b304c, then the increment.
//              0x00490b50 is sb_Init: it writes m_StructSize and
//              m_Unknown04, floors the stride at 4, rounds it up to a
//              multiple of 4 and clears +0x0c/+0x10/+0x14/+0x18, which is
//              servermgr.h's StructBank exactly.  (enumerated, 33 bytes)
//  0x004291a0  memtrack_Term       — dsi.cpp.  Pre-decrement, then sb_Term
//              (0x00490c10, itself sb_FreeChunks(pBank, 0)).  (enumerated, 25)
//  0x0042cc70  dhash_HashFilename  — dhash.cpp, with dhash_FoldFileChar
//              inlined.  Reached only as data: 0x0042cbb0 stores this address
//              into the hash-function table at 0x004b31e0, between
//              0x0042cc00 / 0x0042cc40 and 0x0042ccb0 / 0x0042ccd0 — the four
//              hashes and the compares dhash.cpp's create rewrites every
//              time.  The client's inlining is why it reads `add $0xe0` (an
//              in-place `c -= 'a'-'A'`) and `mov $0x5c` where the server has
//              `sub $0x20` and `add $0x2d`: inside `if (c == '/')` the
//              client's optimizer knows c, so `c += '\\' - '/'` folds to a
//              store of '\\'.  Same source, different -O.
//              (enumerated, 57 bytes, src=gap)
//  0x0042d8c0  dsi_InitMessageModule — dsi.cpp.  LoadLibraryA("ltmsg.dll")
//              into 0x004b3204, answered as `!= LTNULL` through the xor/setne
//              pair.  (enumerated, 26 bytes)
//  0x0042d920  dsi_FreeMessageModule — dsi.cpp.  FreeLibrary and clear, same
//              global.  (enumerated, 27 bytes)
//  0x0042ee70  con_ICompare        — console.cpp.  Not enumerated, but
//              sub_00423c90 below is written against it: character by
//              character, each side folded into a SECOND char (`mov %cl,%dl;
//              sub $0x20,%dl`), answering 1 on a shared NUL.  That is
//              servermgr.h's con_ICompare.
//
// Two more in the run belong to shared sources that have NO server-side
// reconstruction, so they are still open — and still not this file's to
// write:
//
//  0x00428fa0  dsi.cpp (enumerated, 42 bytes).  malloc(0xc) through 0x00492938,
//              arg1 into +0x00, arg2 into +0x04, then prepended to
//              g_pMemBlocks (0x004b2fd8) through +0x08 — dsi.cpp's MemBlock,
//              whose m_pNext that header already puts at +0x08.  A
//              `mem_AddBlock(a, b)` (name ours) dsi.cpp is missing.
//  0x00428ed0  the same TU: GetProcessHeap, then HeapCompact(hHeap, 0) under
//              a null test.  Not enumerated.


// ---- CBaseDriver's default virtuals ------------------------------------ //
//
// The class is servermgr.h's CBaseDriver and the vtable is 0x004a223c.  What
// pins it, off the shipped bytes and nothing else:
//
//   * BOTH driver constructors inline the base constructor, and both store
//     the same three things before their own vptr.  0x0043e3eb (CLocalDriver,
//     whose Disconnect at 0x0043e7e0 is already named) and 0x00484c1f
//     (CTCPDriver, next door to TCPDriver::StartQuery at 0x00485050) each do
//         movl $0x4a223c,(this)         ; the base vptr
//         movl $0x42f00000,0x4c(this)   ; 120.0f -> m_Timeout      @0x4c
//         movl $0,0x44(this)            ;        -> m_DriverFlags  @0x44
//     and only then store 0x004a244c / 0x004a37a4.  servermgr.h puts
//     m_DriverFlags at 0x44 and m_Timeout at 0x4c, so 0x004a223c is
//     CBaseDriver's own vtable and 120 seconds is the shipped idle timeout.
//   * that vtable is exactly 27 slots long: it runs 0x004a223c..0x004a22a8,
//     and 0x004a22a8 is separately referenced — at 0x00484c78, as the vptr
//     CTCPDriver stores into its sub-object at +0xa8.  servermgr.h's
//     CBaseDriver declares exactly 27 virtuals.
//   * slots 1, 2, 3 and 23..26 hold 0x004928e7, which is __purecall
//     (`push $0x19; call 0x00494dc9`), so Init, Term, Update, Disconnect,
//     _p60, ReceivePacket and GetRecvSize really are pure.  Every OTHER slot
//     holds a real default body — which is what says servermgr.h's `=0` on
//     those eighteen is not what the shipped header had.
//   * and the stack-cleanup byte of each default body agrees with the
//     argument count servermgr.h gives that slot, all the way down:
//        slot  5 GetServiceList     ret $4   0x0042d190  return 1
//        slot  6 SelectService      ret $4   0x0042d190  return 1
//        slot  7 _p1c               ret $8   0x0042d180  return 1
//        slot  8 _p20               ret $4   0x0042d190  return 1
//        slot  9 _p24               ret      0x00432030  return 1
//        slot 10 _p28               ret $4   0x0042d190  return 1
//        slot 11 _p2c               ret      0x00432030  return 1
//        slot 12 HostSession        ret $4   0x0042d190  return 1
//        slot 13 _p34               ret $4   0x0042d190  return 1
//        slot 14 UpdateSessionName  ret $4   0x0042d190  return 1
//        slot 15 GetSessionName     ret $8   0x0042d180  return 1
//        slot 16 IsLobbyLaunched    ret      0x00432040  return 0
//        slot 17 GetLobbyLaunchInfo ret $4   0x0042d1b0  return 0
//        slot 18 _p48               ret $4   0x0042d190  return 1
//        slot 19 _p4c               ret      0x00432040  return 0
//        slot 20 GetTcpIpAddress    ret $c   0x0042d1a0  return 0
//        slot 21 _p54               ret $4   0x0042d1b0  return 0
//        slot 22 SendTo             ret $10  0x0042d1c0  return 1
//     GetServiceList and SelectService answering 1 rather than nothing is why
//     CLTClient::GetServiceList and ::SelectService (0x004081e0 / 0x00408240,
//     already named) read a nonzero answer as a failure.  0x00432030,
//     0x00432040 and slot 4's 0x00459f70 (a bare `ret $4` — an EMPTY
//     one-argument body) are outside this run.
//
// Landing these for real means turning those `=0` in src/engine/server/
// servermgr.h into plain declarations and giving CBaseDriver the bodies —
// a change to a header server.dll is built from.
// So they are written below against a layout-identical view, house style (see
// IObjInsideTestView in src/engine/server/servercommon.cpp): the view exists
// only to give the slots somewhere to live and is never instantiated, so it
// never acquires a vtable.  Because ICF is on, the linker folds the eight
// one-argument `return 1;` bodies into one address whichever file declares
// them.
class CBaseDriverDefaultsView
{
public:
	virtual ~CBaseDriverDefaultsView()=0;
	virtual LTBOOL Init()=0;
	virtual void   Term()=0;
	virtual void   Update()=0;
	virtual void   Connect(void* pDriver);                     // 0x10

	virtual LTBOOL GetServiceList(void** ppList);              // 0x14
	virtual LTBOOL SelectService(void* hService);              // 0x18
	virtual LTBOOL _p1c(void* pA, void* pB);                   // 0x1c
	virtual LTBOOL _p20(void* pA);                             // 0x20
	virtual LTBOOL _p24();                                     // 0x24
	virtual LTBOOL _p28(void* pA);                             // 0x28
	virtual LTBOOL _p2c();                                     // 0x2c
	virtual LTBOOL HostSession(void* pHostInfo);               // 0x30
	virtual LTBOOL _p34(void* pA);                             // 0x34
	virtual LTBOOL UpdateSessionName(const char* sName);       // 0x38
	virtual LTBOOL GetSessionName(char* sName, uint32 dwLen);  // 0x3c
	virtual LTBOOL IsLobbyLaunched();                          // 0x40
	virtual LTBOOL GetLobbyLaunchInfo(void** ppData);          // 0x44
	virtual LTBOOL _p48(void* pA);                             // 0x48
	virtual LTBOOL _p4c();                                     // 0x4c
	virtual LTBOOL GetTcpIpAddress(char* sAddr, uint32 dwLen,
	                               unsigned short& hostPort);  // 0x50
	virtual LTBOOL _p54(void* pA);                             // 0x54
	virtual LTBOOL SendTo(const void* pData, uint32 len,
	                      const char* sAddr, uint32 port);     // 0x58

	virtual void   Disconnect(void* pConn, uint32 nReason, uint32 flags)=0;
	virtual void   _p60()=0;
	virtual LTBOOL ReceivePacket(class ServerMessage* pMsg)=0;
	virtual uint32 GetRecvSize()=0;
};

// 0x0042d190 — `mov $0x1,%eax; ret $0x4`.  Eight slots, one folded body.
LTBOOL CBaseDriverDefaultsView::GetServiceList(void** ppList)        { return 1; }
LTBOOL CBaseDriverDefaultsView::SelectService(void* hService)        { return 1; }
LTBOOL CBaseDriverDefaultsView::_p20(void* pA)                       { return 1; }
LTBOOL CBaseDriverDefaultsView::_p28(void* pA)                       { return 1; }
LTBOOL CBaseDriverDefaultsView::HostSession(void* pHostInfo)         { return 1; }
LTBOOL CBaseDriverDefaultsView::_p34(void* pA)                       { return 1; }
LTBOOL CBaseDriverDefaultsView::UpdateSessionName(const char* sName) { return 1; }
LTBOOL CBaseDriverDefaultsView::_p48(void* pA)                       { return 1; }

// 0x0042d180 — `mov $0x1,%eax; ret $0x8`.  Two slots.  Not enumerated;
// written here because it is the same body with a different cleanup and the
// pair is what makes the argument-count argument above check out.
LTBOOL CBaseDriverDefaultsView::_p1c(void* pA, void* pB)             { return 1; }
LTBOOL CBaseDriverDefaultsView::GetSessionName(char* s, uint32 n)    { return 1; }

// 0x00432030 — `mov $0x1,%eax; ret`, and 0x00432040 — `xor %eax,%eax;
// ret`: the two argument-less defaults, which the shipped vtable points
// OUTSIDE this run (their bodies fold with every other such function in the
// image). 0x00459f70 — a bare `ret $0x4` — is slot 4, Connect, which does
// nothing. Written so that src/engine/server/localdriver.cpp's client build
// can link its CLocalDriver vtable onto these (link/aliases.rsp) rather than
// compiling /Od copies of its own.
LTBOOL CBaseDriverDefaultsView::_p24()                               { return 1; }
LTBOOL CBaseDriverDefaultsView::_p2c()                               { return 1; }
LTBOOL CBaseDriverDefaultsView::IsLobbyLaunched()                    { return 0; }
LTBOOL CBaseDriverDefaultsView::_p4c()                               { return 0; }
void   CBaseDriverDefaultsView::Connect(void* pDriver)               {}

// 0x0042d1a0 — `xor %eax,%eax; ret $0xc`.  One slot; the unsigned short goes
// by reference, which is the third dword.
LTBOOL CBaseDriverDefaultsView::GetTcpIpAddress(char* sAddr, uint32 dwLen,
                                                unsigned short& hostPort)
{
	return 0;
}

// 0x0042d1b0 — `xor %eax,%eax; ret $0x4`.  Two slots.
LTBOOL CBaseDriverDefaultsView::GetLobbyLaunchInfo(void** ppData)    { return 0; }
LTBOOL CBaseDriverDefaultsView::_p54(void* pA)                       { return 0; }

// 0x0042d1c0 — `mov $0x1,%eax; ret $0x10`.  One slot, four arguments.
LTBOOL CBaseDriverDefaultsView::SendTo(const void* pData, uint32 len,
                                       const char* sAddr, uint32 port)
{
	return 1;
}

// Recorded here:
//   0x0042d1d0 (32 bytes, not enumerated) is CBaseDriver's own scalar
//   deleting destructor — it stores 0x004a223c back into (this), tests bit 1
//   of its flags argument and calls the free forwarder at 0x004290c0, the
//   standard VC6 `??_G` shape.  0x0042d1f0 is the same shape for the
//   0x004a22a8 sub-object CTCPDriver carries at +0xa8.
//   0x0042d120 (enumerated, 27 bytes) is slot 3 of the FIVE-slot vtable at
//   0x004a2228 — a CBankBase-shaped class (servermgr.h) with a global
//   instance at 0x004b31b0 that a static initializer builds with
//   sb_Init2(&m_Bank, 0x18, 0x20, 0x40) (0x0042cac0, registered by the
//   init/atexit pair at 0x0042cab0) and 0x0042caf0 tears down.  Its body is
//       sb_FreeChunks(&m_Bank, 1);  sb_Init(&m_Bank, 0x18, 0x20);
//   i.e. CBankBase::Term for a bank of 0x18-byte records.  NOT written here:
//   the global sits between its bank at 0x004b31b4 and dhash's function
//   tables at 0x004b31d8, so its home is almost certainly the client's
//   compile of dhash.cpp, which the link already has.


// ---- the 0x18-byte record and its two sentinels ------------------------ //

// The record type is not named in the image; it is named here by its size.
// 0x00423c00's `lea (%eax,%eax,2); lea (%ecx,%eax,8)` is index*3 then *8,
// which is what a 0x18-byte element type gives for free (names ours).
class CRec18View
{
public:
	uint8 m_pad_00[0x18];
};

extern CRec18View* g_pRec18Sentinel1;        // 0x004a9878, reached by index -1
extern CRec18View* g_pRec18Sentinel2;        // 0x004a987c, reached by index -2

// 0x00423c00, with its 0x00423c23 tail — ONE function of 46 bytes; the
// enumeration splits them because the enumerator found the tail past a `ret`.
// Pick one of those records by index, with two sentinel indices.
//
// Read off the bytes:
//
//   * cdecl, three arguments, and the bound is compared SIGNED (`jl`), so the
//     index parameter is signed and the count is widened to it.  That is what
//     lets -1 and -2 be indices at all.
//   * the two sentinels are separate `if`s each with its own `ret`, not a
//     switch: `cmp; jne` over a two-instruction body, twice.
//   * the bounds test's FAILING arm is the fall-through and the element
//     arithmetic sits after it, past that `ret`.  That is an early
//     `return LTNULL`, not `if (i < n) return &a[i];` — the latter puts the
//     element first.  CObj4a2078View::_v0c below has the other spelling and
//     the opposite layout, in the same image, which is what says the two are
//     written differently in source rather than folded by the compiler.
//
// 0x00425eda and 0x00425f35 are two of the four callers; both read a 4-byte
// index off an ILTStream (slot 8, Read) and hand the answer straight into a
// record at +0x64 or into a two-element local array, so the sentinels are
// what the world file stores when it means "the default one".
CRec18View* sub_00423c00(CRec18View* pArray, uint32 nCount, int32 iIndex)
{
	if (iIndex == -1)
		return g_pRec18Sentinel1;

	if (iIndex == -2)
		return g_pRec18Sentinel2;

	if (iIndex >= (int32)nCount)
		return LTNULL;

	return &pArray[iIndex];
}


// ---- find-by-name in a singly-linked list ------------------------------ //

// Only the two fields sub_00423c90 touches are named.  The name is stored
// INLINE at +0x10, not as a pointer: the shipped code takes its address with
// `lea 0x10(%esi),%eax` (names ours).
class CNamedNodeView
{
public:
	uint8           m_pad_00[0x0c];
	CNamedNodeView* m_pNext;                 // +0x0c
	char            m_Name[1];               // +0x10
};

// console.cpp's case-insensitive compare, 0x0042ee70 in this image.
LTBOOL con_ICompare(char* p1, char* p2);

// 0x00423c90 — find the node whose inline name matches, case-insensitively.
// cdecl, two arguments.
//
// Read off the bytes: the null test is hoisted above the loop and the loop
// tail re-tests it (`test %esi,%esi; jne` back to the compare), which is VC6's
// rotation of a `while (p)` and not a `do`.  con_ICompare's FIRST argument is
// the node's own name — its `lea` is pushed last.  Its only caller is
// 0x004233c9, below this run.
CNamedNodeView* sub_00423c90(CNamedNodeView* pList, char* pName)
{
	while (pList)
	{
		if (con_ICompare(pList->m_Name, pName))
			return pList;

		pList = pList->m_pNext;
	}

	return LTNULL;
}


// ---- release the object at +0x184 -------------------------------------- //

// Slot 1 is the only slot reached, and it is reached as a thiscall with no
// arguments — the release/destroy convention every reference-counted
// interface in this image uses (names ours).
class CReleasableView
{
public:
	virtual void _v00()=0;
	virtual void _v04()=0;
};

class CHost184View
{
public:
	void ReleaseSub184();                    // 0x00424db0 (name ours)

public:
	uint8            m_pad_000[0x184];
	CReleasableView* m_p184;                 // +0x184
};

// 0x00424db0 — let go of whatever is at +0x184.  thiscall, no arguments.
// The store of LTNULL is INSIDE the guard: its `movl $0x0` sits between the
// call and the epilogue and the `je` jumps past it.  Callers 0x00424ce2 and
// 0x004754eb.
void CHost184View::ReleaseSub184()
{
	if (m_p184)
	{
		m_p184->_v04();

		m_p184 = LTNULL;
	}
}


// ---- hand a list's contents to the sub-object at +0x6c ----------------- //

// The sub-object's one reached method: a thiscall with two arguments, the
// second always the literal 1 (name ours).
class CSub6cView
{
public:
	void Discard(void* pData, int nFlag);    // 0x0048cde0
};

class CListOwner6cView
{
public:
	void DiscardList(LTLink* pHead);         // 0x00424ef0 (name ours)

public:
	uint8      m_pad_000[0x6c];
	CSub6cView m_Sub06c;                     // +0x6c
};

// 0x00424ef0 — hand every element of an LTList to the sub-object at +0x6c.
// thiscall, one argument, `ret $4`.  Callers 0x004244b2 and 0x00424b77.
//
// Read off the bytes: the walk starts at +0x04 of the argument and takes each
// node's +0x08, stopping when it comes back to the argument ITSELF.  ltlink.h
// puts m_pPrev at +0x00, m_pNext at +0x04 and m_pData at +0x08 (CheapLTLink
// first, LTLink adds m_pData), so the argument is the head LINK.  It is not an
// LTList: that class puts m_nElements at +0x00 and m_Head at +0x04, so an
// LTList* would have read the first next from +0x08 and compared against
// pList+4.  Neither is in the shipped bytes.
void CListOwner6cView::DiscardList(LTLink* pHead)
{
	LTLink* pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		m_Sub06c.Discard(pCur->m_pData, 1);

		pCur = pCur->m_pNext;
	}
}


// ---- flag every record an array of keys resolves to -------------------- //

// The array the loop walks: dwords with their count beside them (names ours,
// from the two offsets the loop reads every time round).
class CKeyArray2cView
{
public:
	uint8  m_pad_00[0x2c];
	void** m_pKeys;                          // +0x2c
	uint32 m_nKeys;                          // +0x30
};

// What the lookup answers: a record whose BYTE at +0x15 carries flags.
class CFlagged15View
{
public:
	uint8 m_pad_00[0x15];
	uint8 m_Flags15;                         // +0x15
};

class CResolver2cView
{
public:
	virtual void _v00()=0;
	// slot 1: one argument, the ADDRESS of a key, answering the record or
	// LTNULL.  Reached only virtually, which is what says it is a slot and not
	// a member (name ours).
	virtual CFlagged15View* Resolve(void** ppKey)=0;

	void FlagResolved(CKeyArray2cView* pArray);   // 0x00425580 (name ours)
};

// 0x00425580 — set bit 7 of +0x15 on every record the keys resolve to.
// thiscall, one argument, `ret $4`.  Its only caller is 0x00404b7b.
//
// Read off the bytes:
//
//   * the count is re-read from +0x30 at the BOTTOM of every iteration
//     (`mov 0x30(%ebx),%eax` after the store), so the loop condition reads the
//     member each time rather than a cached copy — the resolver is allowed to
//     grow the array.
//   * the flag is `orb $0x80,0x15(%eax)` on the answer, under its own null
//     test, and the operand is a byte — not a dword masked down.
void CResolver2cView::FlagResolved(CKeyArray2cView* pArray)
{
	uint32 i;

	for (i = 0; i < pArray->m_nKeys; i++)
	{
		CFlagged15View* pRec = Resolve(&pArray->m_pKeys[i]);

		if (pRec)
			pRec->m_Flags15 |= 0x80;
	}
}


// ---- the 0xc0-byte world-object record --------------------------------- //
//
// The class is not named anywhere in the image; it is named here by its
// constructor's address.  What establishes the layout:
//
//   * 0x00425c00 is the constructor (it answers `this` in eax) and 0x00425cf0
//     the destructor (it re-stores the vptrs).  The constructor stores
//     0x004a2000 at +0x00, and two array sub-objects get their own vptrs:
//     0x004a1c24 at +0x50 and 0x004a2024 at +0x68.  Both are twelve-slot
//     tables, and lthread.h records that a twelve-slot vtable in this engine
//     is collection.h's GenList<>.
//   * each array is five dwords — vptr, pointer, three counts — so the first
//     runs 0x50..0x64 and the second 0x68..0x7c.  The destructor's
//     `pool_free(0x004b4e88, m_p06c, m_n070 + m_n074)` is what says +0x6c is
//     the pointer and +0x70/+0x74 the counts.
//   * the constructor seeds +0x64 from the SAME global 0x004a987c that
//     sub_00423c00 hands back for index -2, so +0x64 holds one of those
//     0x18-byte records and starts out as the default one.
//   * 0x00428e30 walks an array of these with a stride of 0xc0, which is
//     where the 0xc0 below comes from and where every offset fits.
//   * 0x004a2000 is nine slots (0x004a2000..0x004a2024, and 0x004a2024 is
//     separately referenced), and 0x004a2054 is a nine-slot table whose every
//     slot is __purecall — the same interface, abstract.  So those nine slots
//     are an interface this record implements.  Only four of its bodies are in
//     this run and none of them names it, so the slots are numbered.
class CObj7cView;

class CObj425C00View
{
public:
	~CObj425C00View();                       // 0x00425cf0

	virtual void   _v00()=0;
	virtual void   _v04()=0;
	virtual void   _v08()=0;
	virtual void   _v0c()=0;
	virtual void   _v10(LTVector vValue);    // 0x10  0x00426190
	virtual void*  _v14();                   // 0x14  0x004261c0
	virtual uint32 _v18();                   // 0x18  0x004261d0
	virtual void   _v1c()=0;                 // 0x1c  0x0048fab0, outside the run
	virtual float  _v20();                   // 0x20  0x004261e0

	void RecalcBounds();                     // 0x00426090 (name ours)

public:
	uint8       m_pad_004[0x50 - 0x04];
	// the GenList<> at +0x50: vptr, then the pointer and count RecalcBounds
	// reads as an array of points.
	uint8       m_Sub050_vptr[4];            // +0x50  0x004a1c24
	void*       m_p054;                      // +0x54
	uint32      m_n058;                      // +0x58
	uint8       m_pad_05c[0x64 - 0x5c];
	CRec18View* m_p064;                      // +0x64
	// the GenList<> at +0x68: vptr, then the 0x18-byte records
	uint8       m_Sub068_vptr[4];            // +0x68  0x004a2024
	void*       m_p06c;                      // +0x6c
	uint32      m_n070;                      // +0x70
	uint32      m_n074;                      // +0x74
	uint32      m_n078;                      // +0x78
	CObj7cView* m_p07c;                      // +0x7c
	uint8       m_Sub080[0xb0 - 0x80];       // +0x80, twelve dwords the ctor clears
	LTVector    m_v0b0;                      // +0xb0
	float       m_f0bc;                      // +0xbc
};

// What +0x7c points at; only the dword slot 0x004261d0 reads is named.
class CObj7cView
{
public:
	uint8  m_pad_00[0x48];
	uint32 m_n048;                           // +0x48
};

// The +0x80 sub-object's one reached method: a thiscall taking an LTVector BY
// VALUE, which is why it cleans 0xc bytes of its own.  server/worldbsp.cpp
// has it as CWorldSub108::GetPointStatus.
class CSubRecord;
class CWorldSub108
{
public:
	CSubRecord* GetPointStatus(LTVector vValue);          // 0x00425b60
};

// The point-array reducer.  cdecl, three arguments, answering a float in
// st(0).  0x00426b38 calls it too, from the sibling class below.
// server/worldbsp.cpp's wb_MaxDistSqr.
class CWorldPart;
float wb_MaxDistSqr(CWorldPart** pPoints, uint32 nPoints, LTVector* pOut); // 0x004260c0

// 0x00426090 — recompute +0xb0/+0xbc from the +0x50 point array.  thiscall,
// no arguments.  Its only caller is 0x0042607a.
//
// Read off the bytes: the three pushes go right to left — &m_v0b0, then the
// count, then the pointer — so the out-parameter is the LAST argument, and the
// answer is stored with `fstps 0xbc(%esi)`, so +0xbc is a float and not a
// double.
void CObj425C00View::RecalcBounds()
{
	m_f0bc = wb_MaxDistSqr((CWorldPart**)m_p054, m_n058, &m_v0b0);
}

// 0x00426190 — vtable slot 4.  Forward the by-value LTVector to the +0x80
// sub-object.
//
// Read off the bytes: `sub $0xc,%esp`, three dword stores through the new esp,
// and NO cleanup after the call — the callee pops those twelve bytes, and the
// `ret $0xc` is this function's own parameter.  A by-value struct passed
// straight on is COPIED, not aliased, which is what the three stores are.
void CObj425C00View::_v10(LTVector vValue)
{
	((CWorldSub108*)m_Sub080)->GetPointStatus(vValue);
}

// 0x004261c0 — vtable slot 5.  `mov 0x6c(%ecx),%eax; ret`, four bytes.
void* CObj425C00View::_v14()
{
	return m_p06c;
}

// 0x004261d0 — vtable slot 6.  Two loads and no null test:
// `mov 0x7c(%ecx),%eax; mov 0x48(%eax),%eax`.
uint32 CObj425C00View::_v18()
{
	return m_p07c->m_n048;
}

// 0x004261e0 — vtable slot 8.  `flds 0xbc(%ecx)`.  What settles float over
// double is that RecalcBounds STORES the same offset with `fstps`.
float CObj425C00View::_v20()
{
	return m_f0bc;
}

// The manager the array teardown frees through.  Only slot 2 is reached
// (name ours).
class CObjAllocView
{
public:
	virtual void _v00()=0;
	virtual void _v04()=0;
	virtual void Free(void* pMem)=0;         // 0x08
};

// 0x00428e30 — destroy an array of the records and give the storage back.
// cdecl, three arguments.  Nine callers, 0x00423a5f through 0x00428166.
//
// Read off the bytes:
//
//   * the array pointer is tested FIRST and its `je` skips everything,
//     including the free, so the guard is around the whole body.
//   * the count is a countdown (`dec %edi; jne`), not an index against a
//     limit, and it is tested once before the loop — a `while`, not a `do`.
//   * the destructor is 0x00425cf0 called directly (no flags argument), i.e.
//     the destructor PROPER and not the deleting form, and the stride is 0xc0.
//   * the free is virtual, through slot 2 of the first argument, and is handed
//     the ORIGINAL pointer: the walk uses its own register.
void sub_00428e30(CObjAllocView* pAlloc, CObj425C00View* pArray, uint32 nCount)
{
	CObj425C00View* pCur;

	if (pArray)
	{
		pCur = pArray;

		while (nCount)
		{
			pCur->~CObj425C00View();

			pCur++;
			nCount--;
		}

		pAlloc->Free(pArray);
	}
}


// ---- the sibling record class (vtable 0x004a2078) ---------------------- //
//
// Ten slots, 0x004a2078..0x004a20a0 (both separately referenced).  It is a
// sibling of the class above and not a relative: 0x00426b38 and 0x00426c1f
// call the SAME two helpers 0x004260c0 and 0x00425b60 that RecalcBounds and
// _v10 call, so the two classes carry the same point array and the same
// +0x80-style sub-object at different offsets.  Nothing here names the
// interface, so the slots are numbered, and only the four offsets these three
// bodies touch are mapped (names ours).
class CObj4a2078View
{
public:
	virtual void   _v00()=0;                 // 0x0048fab0
	virtual void   _v04()=0;                 // 0x004428e0
	virtual void   _v08()=0;                 // 0x00426b90
	virtual void*  _v0c(uint32 hHandle);     // 0x0c  0x00426bd0
	virtual void   _v10()=0;                 // 0x00426c00
	virtual void*  _v14();                   // 0x14  0x00426c30
	virtual void   _v18()=0; virtual void _v1c()=0;
	virtual void   _v20()=0; virtual void _v24()=0;

	void Forward34(void* pA, void* pB);      // 0x00426c90 (name ours)

public:
	// The vptr occupies 0x00, so the pad that reaches +0x34 is four bytes
	// shorter than the offset: with the full 0x34 every field below came out
	// four bytes high.
	uint8  m_pad_004[0x34 - 0x04];
	void*  m_p034;                           // +0x34
	uint32 m_n038;                           // +0x38
	uint8  m_pad_03c[0x70 - 0x3c];
	void*  m_p070;                           // +0x70
	uint8  m_pad_074[0xa0 - 0x74];
	void** m_pTable0a0;                      // +0xa0
	uint32 m_nTable0a4;                      // +0xa4
};

// 0x00426bd0 — vtable slot 3.  A bounds-checked table lookup keyed by the low
// sixteen bits of a handle.
//
// Read off the bytes:
//
//   * `and $0xffff,%eax` on the parameter before anything else, so the index
//     is a uint16 cut out of a wider handle: the narrowing cast is in source,
//     the caller's dword is not narrowed for it.
//   * the compare is UNSIGNED (`jae`) against +0xa4, and the failing arm sits
//     AFTER the element return, past its `ret`.  That is the positive
//     spelling, and the opposite layout to sub_00423c00 above.
void* CObj4a2078View::_v0c(uint32 hHandle)
{
	uint16 nIndex = (uint16)hHandle;

	if (nIndex < m_nTable0a4)
		return m_pTable0a0[nIndex];

	return LTNULL;
}

// 0x00426c30 — vtable slot 5.  `mov 0x70(%ecx),%eax; ret`, four bytes.
void* CObj4a2078View::_v14()
{
	return m_p070;
}

// The helper Forward34 hands off to: cdecl, four arguments (name ours).
void sub_0043cd10(uint32 nTop3Bits, void* p034, void* pA, void* pB);

// 0x00426c90 — hand over +0x34 and the TOP THREE BITS of +0x38 along with the
// two incoming arguments.  thiscall, `ret $8`.
//
// Read off the bytes: `shr $0x1d,%ecx` is LOGICAL, so +0x38 is unsigned, and
// its result is pushed LAST, i.e. it is the first argument.  The two incoming
// arguments keep their order.  This body is not in vtable 0x004a2078; the
// enumeration's other entry in this neighbourhood, 0x00427ca0, is slot 8 of the
// twelve-slot table at 0x004a20a0 and is over 64 bytes — skipped.
void CObj4a2078View::Forward34(void* pA, void* pB)
{
	sub_0043cd10(m_n038 >> 29, m_p034, pA, pB);
}


// ---- the 0x429xxx handle wrapper --------------------------------------- //
//
// A two-dword object: a pointer to a table of __cdecl function pointers at
// +0x00 and a handle drawn from that table at +0x04.  What says the table is
// a C struct of function pointers and not a C++ interface is 0x00429841:
// `call *0x178(%ecx)` with the handle pushed and `add $0x4,%esp` AFTER the
// call, so the callee does not pop and no `this` goes with it.  Every C++
// virtual in this image is thiscall and pops its own (names ours).
class CFnTable178View
{
public:
	uint8 m_pad_000[0x178];
	void  (*m_pfnFree0178)(void* hHandle);   // +0x178
};

class CHandleWrapperView
{
public:
	LTRESULT IsReady();                      // 0x00429820 (name ours)
	LTRESULT Free();                         // 0x00429830 (name ours)

public:
	CFnTable178View* m_pTable;               // +0x00
	void*            m_hHandle;              // +0x04
};

// 0x00429820 — is this wrapper usable?  thiscall, no arguments, twelve bytes.
//
// Read off the bytes: `neg`/`sbb`/`and $0xb7`/`add $0x49` is the
// branchless `?:`, evaluated as `B + (mask & (A - B))` with B = 0x49 and
// A = 0 — 0xb7 is (0 - 0x49) & 0xff.  ltcodes.h makes 73 LT_NOTINITIALIZED
// and LT_OK is 0, so the answer is LT_OK when the table is there and
// LT_NOTINITIALIZED when it is not, written as a conditional expression and
// not as an if.  0x00429ca0 is one caller and it guards on `!= LT_OK`.
LTRESULT CHandleWrapperView::IsReady()
{
	return m_pTable ? LT_OK : LT_NOTINITIALIZED;
}

// 0x00429830 — give the handle back and forget both halves.  thiscall, no
// arguments.  Found as a gap entry; it is a whole function (aligned start, its
// own prologue and `ret`).
//
// Read off the bytes:
//
//   * the two guards are NESTED, and the two stores are what both of them
//     skip to — so neither guard can skip the stores, which is why they are
//     unconditional below.
//   * the stores run +0x04 then +0x00, i.e. handle first.
//   * the answer is a bare `xor %eax,%eax`, and nothing above it can fail.
LTRESULT CHandleWrapperView::Free()
{
	if (m_pTable)
	{
		if (m_hHandle)
			m_pTable->m_pfnFree0178(m_hHandle);
	}

	m_hHandle = LTNULL;
	m_pTable  = LTNULL;

	return LT_OK;
}


// ---- a four-dword clear ------------------------------------------------ //

// 0x004283a0 — clear +0x04..+0x10.  thiscall, no arguments, fifteen bytes.
//
// Read off the bytes: eax is left holding zero and `this` is NOT answered, so
// this is an ordinary void member and not a constructor — 0x00425c00, a
// constructor in the same run, ends `mov %esi,%eax; ret`.  The stores run
// ASCENDING (4, 8, 0xc, 0x10), which is four separate assignments; ONE chained
// assignment stores right to left, the way commonmodel.cpp's VEC_INIT note
// records.  The single `xor` is VC6 commoning the literal across all four.
class CClear04View
{
public:
	void Clear();                            // 0x004283a0 (name ours)

public:
	uint8  m_pad_00[0x04];
	uint32 m_n004;                           // +0x04
	uint32 m_n008;                           // +0x08
	uint32 m_n00c;                           // +0x0c
	uint32 m_n010;                           // +0x10
};

void CClear04View::Clear()
{
	m_n004 = 0;
	m_n008 = 0;
	m_n00c = 0;
	m_n010 = 0;
}


// ---- the demo manager (0x42bd00..0x42ca50) ----------------------------- //
//
// The TU is named by its strings: "DemoMgr: Invalid number of changes"
// (0x0042c388), "DemoMgr: Invalid change", "DemoMgr: Sync byte wrong (wanted
// %d, got %d)", "Demo played in %.3f seconds", "PlayDemoReps info ---------",
// and the counter names "Malloc" / "Free" / "Music" / "Sound" / "Input" /
// "ClientShell" / "  Server" / "    Render" / "      TagVisibleLeaves" at
// 0x0042bd36..0x0042bf5a.  The class's own name is not in the image; only the
// offsets these four bodies touch are mapped, and whether 0x0042bfa0 is the
// same class as the other three is NOT proven — the offsets are merely
// consistent (names ours).
class CClientHost61cView
{
public:
	virtual void _v00()=0; virtual void _v04()=0; virtual void _v08()=0;
	virtual void _v0c()=0; virtual void _v10()=0; virtual void _v14()=0;
	virtual void _v18()=0; virtual void _v1c()=0; virtual void _v20()=0;
	virtual void _v24()=0; virtual void _v28()=0; virtual void _v2c()=0;
	virtual void _v30()=0; virtual void _v34()=0; virtual void _v38()=0;
	virtual void _v3c()=0; virtual void _v40()=0; virtual void _v44()=0;
	virtual void _v48()=0; virtual void _v4c()=0; virtual void _v50()=0;
	virtual void _v54()=0;                                    // 0x54
	virtual void _v58(void* pA, void* pB)=0;                  // 0x58
};

class CClientMgr61cView
{
public:
	uint8               m_pad_000[0x61c];
	CClientHost61cView* m_p61c;              // +0x61c
};

// 0x0042e670 -- clientdsi.cpp's getter for the dword at 0x004b11e8; a
// plain cdecl function, which IsRunning tail-jumps to with ecx untouched.
uint32 sub_42e670();

class CDemoMgrView
{
public:
	CDemoMgrView();                          // 0x0042bfa0
	LTBOOL IsRunning();                      // 0x0042bfd0 (name ours)
	void   SeedAndStart();                   // 0x0042bff0 (name ours)
	void   Forward58(void* pA, void* pB);    // 0x0042c020 (name ours)

public:
	uint8              m_pad_000[0x04];
	uint32             m_n004;               // +0x04
	uint8              m_pad_008[0x0c - 0x08];
	CClientMgr61cView* m_pClientMgr;         // +0x0c
	uint32             m_n010;               // +0x10
	uint32             m_n014;               // +0x14
	uint32             m_nMode018;           // +0x18
};

// 0x0042bfa0 — the CONSTRUCTOR (clientmgr3.cpp's CCmgDemoMgr, which the
// client manager's constructor builds at +0x158c, is linked onto it).
// thiscall, no arguments, twenty bytes.
//
// Read off the bytes: the stores are +0x0c, +0x10, +0x18, +0x04, +0x14 — NOT
// ascending, and not the reverse of ascending either, so they are five
// separate assignments in that source order, and the order is the shipped
// code's rather than ours.  `this` is moved into eax FIRST and left there:
// that is how VC6 answers an out-of-line constructor, and a void member
// written with the same five stores commons the zero into eax instead and
// comes out two bytes short (`xor %eax,%eax`, stores through ecx).  The
// earlier reading of this as a void "Clear" left 0x0042bfa0 to be paired,
// by size, with an unrelated CMoArray constructor.
CDemoMgrView::CDemoMgrView()
{
	m_pClientMgr = LTNULL;
	m_n010       = 0;
	m_nMode018   = 0;
	m_n004       = 0;
	m_n014       = 0;
}

// 0x0042bfd0 — thiscall, no arguments, twenty-one bytes.
//
// Read off the bytes: two `cmp`/`je` against 1 and 2, both landing on
// `xor %eax,%eax; ret`, then a TAIL JUMP (`jmp 0x0042e670`) with ecx still
// holding `this`.  So the fall-through is `return SomeOtherMethod();` on the
// same object and the two early exits answer zero; two compares jumping to one
// label is what a short-circuit `||` gives.
LTBOOL CDemoMgrView::IsRunning()
{
	if (m_nMode018 == 1 || m_nMode018 == 2)
		return 0;

	return sub_42e670();
}

// 0x0042bff0 — thiscall, no arguments, thirty-nine bytes.
//
// Read off the bytes:
//
//   * `push $0x7b; call 0x00493eff; add $4` — 0x00493eff is srand: it reaches
//     the per-thread block through 0x00499e9d and stores its argument at +0x14,
//     and its neighbour at 0x0049390c reads the same slot and multiplies by
//     0x343fd, which is rand.  So playback seeds the generator with 123 to
//     make itself deterministic, and the seed is a literal.
//   * the two guards are nested `if`s reaching one exit, and the call is
//     TAIL-JUMPED (`jmp *0x54(%edx)`), which is why that path has no epilogue.
void CDemoMgrView::SeedAndStart()
{
	srand(0x7b);

	if (m_pClientMgr)
	{
		if (m_pClientMgr->m_p61c)
			m_pClientMgr->m_p61c->_v54();
	}
}

// 0x0042c020 — the same two guards, forwarding both arguments to slot 0x58.
// thiscall, two arguments, `ret $8`, thirty-seven bytes.
//
// Read off the bytes: the arguments are pushed second-then-first, so they
// reach slot 0x58 in their original order; and unlike SeedAndStart this one is
// a plain `call`, because the return types do not let it tail-jump past the
// `ret $8`.
void CDemoMgrView::Forward58(void* pA, void* pB)
{
	if (m_pClientMgr)
	{
		if (m_pClientMgr->m_p61c)
			m_pClientMgr->m_p61c->_v58(pA, pB);
	}
}


// ---- the locked list at 0x42d210..0x42d760 ----------------------------- //
//
// Two classes.  The owner IS a CRITICAL_SECTION at offset zero — 0x0042d6d4
// hands `this` straight to EnterCriticalSection with no displacement — plus a
// list head at +0x18 and a limit at +0x30.  Each item carries a back-pointer
// to the owner at +0x0c, which is what the item bodies hand to
// EnterCriticalSection, a reference count at +0x10, a payload at +0x08 and the
// next link at +0x00.  Neither class is named in the image, and the only
// callers are 0x0044338a, 0x00443482, 0x004439a2 and 0x00443a3b — in the
// DirectMusic block (CLTDirectMusicMgr::Init is 0x0043f920 and
// "CLTDMFileStream::Open FileName=%s" is at 0x00443358) — which says what uses
// it but not what it is.  Names ours.
class CLockedOwnerView;

class CLockedItemView
{
public:
	// 0x0042d2d0 with its 0x0042d30a tail — ONE function; the enumeration splits
	// them at the third `ret`.
	LTBOOL TryDiscard();
	void   Release();                        // 0x0042d3b0
	LTBOOL Discard();                        // 0x0042d4b0, outside the list

public:
	CLockedItemView*  m_pNext;               // +0x00
	uint8             m_pad_004[0x08 - 0x04];
	void*             m_pData;               // +0x08
	CLockedOwnerView* m_pOwner;              // +0x0c
	uint32            m_nRefs;               // +0x10
};

class CLockedOwnerView
{
public:
	// 0x0042d520 and 0x0042d540.  Neither answers `this` and neither is
	// reached through a vtable, so they are named Init/Term rather than
	// constructor/destructor: the shipped bytes cannot tell those apart here.
	void   Init(uint32 nLimit);
	void   Term();
	LTBOOL PurgeOne();                       // 0x0042d6d0
	void   Flush();                          // 0x0042d680, outside the list
	void   Touch();                          // 0x0042d710, outside the list

public:
	CRITICAL_SECTION m_CS;                   // +0x00  (0x18 bytes)
	CLockedItemView* m_pItems;               // +0x18
	uint8            m_pad_01c[0x30 - 0x1c];
	uint32           m_nLimit;               // +0x30
};

// 0x0042d2d0 (+0x0042d30a) — discard this item if nobody is holding it.
//
// Read off the bytes:
//
//   * the lock is taken through the OWNER (`mov 0xc(%esi),%eax; push %eax`)
//     and released on all three exits — three separate LeaveCriticalSection
//     sites, which is what three `return`s give.
//   * `test %eax,%eax; jbe` on the reference count is VC6's spelling of an
//     UNSIGNED `> 0`, not of `!= 0`.
//   * the last exit answers whatever Discard answered: the value survives the
//     Leave in edi, which is why edi is pushed only on that path.
LTBOOL CLockedItemView::TryDiscard()
{
	LTBOOL bResult;

	EnterCriticalSection(&m_pOwner->m_CS);

	if (m_nRefs > 0)
	{
		LeaveCriticalSection(&m_pOwner->m_CS);

		return LTTRUE;
	}

	if (!m_pData)
	{
		LeaveCriticalSection(&m_pOwner->m_CS);

		return LTFALSE;
	}

	bResult = Discard();

	LeaveCriticalSection(&m_pOwner->m_CS);

	return bResult;
}

// 0x0042d3b0 — drop one reference.  Five callers, all in the DirectMusic
// block.
//
// Read off the bytes: two Leave sites again, so the zero case is an early
// `return`.  The decrement reuses the value the test already loaded
// (`dec %eax; mov %eax,0x10(%esi)`), and the owner notification is a thiscall
// on the owner with no arguments, taken from the same +0x0c the lock came
// from.
void CLockedItemView::Release()
{
	EnterCriticalSection(&m_pOwner->m_CS);

	if (m_nRefs == 0)
	{
		LeaveCriticalSection(&m_pOwner->m_CS);

		return;
	}

	m_nRefs--;

	m_pOwner->Touch();

	LeaveCriticalSection(&m_pOwner->m_CS);
}

// 0x0042d520 — thiscall, one argument, `ret $4`.  The critical section is the
// object's whole front, which is why InitializeCriticalSection is handed
// `this` with no displacement.
void CLockedOwnerView::Init(uint32 nLimit)
{
	InitializeCriticalSection(&m_CS);

	m_nLimit = nLimit;
}

// 0x0042d540 — thiscall, no arguments.
void CLockedOwnerView::Term()
{
	Flush();

	DeleteCriticalSection(&m_CS);
}

// 0x0042d6d0 — discard the first item that will let go.  thiscall, no
// arguments.  Its only caller is 0x0042d68c, inside Flush.
//
// Read off the bytes: the walk is `for (p = m_pItems; p; p = p->m_pNext)` with
// the null test hoisted and re-tested at the bottom, and the hit leaves
// through its OWN Leave with 1 while the fall-through Leaves with 0 — two
// Leave sites, two returns.
LTBOOL CLockedOwnerView::PurgeOne()
{
	CLockedItemView* pCur;

	EnterCriticalSection(&m_CS);

	for (pCur = m_pItems; pCur; pCur = pCur->m_pNext)
	{
		if (pCur->TryDiscard())
		{
			LeaveCriticalSection(&m_CS);

			return LTTRUE;
		}
	}

	LeaveCriticalSection(&m_CS);

	return LTFALSE;
}


// ---- the debug break --------------------------------------------------- //

// 0x004b0b1c — whether the break is armed: g_CluClientGlob.m_bBreakOnError.

// 0x0042d760 — break into the debugger if it was asked for.  cdecl, sixteen
// bytes, and it takes ONE argument that it never reads: the body reveals
// nothing (a cdecl `ret` says nothing about arity), but all five call sites
// push an error code and clean it themselves.  The parameter is in the
// signature so a caller written against this declaration emits the push.
//
// Read off the bytes: the call is a TAIL JUMP through the import
// (`jmp *0x4a1060`), so DebugBreak is the last statement and there is nothing
// after the `if`.
void sub_0042d760(uint32 /*nErrorCode*/)
{
	if (g_CluClientGlob.m_bBreakOnError)
		DebugBreak();
}
