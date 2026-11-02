// *********************************************************************** //
//
//  MODULE:   LT2NEW2 (.CPP)  -- scratch file, NOT a drop-in TU
//
//  PURPOSE:  Eighteen bodies out of a 25-address list of 33..37-byte
//            functions in lithtech.exe.  The other seven rows are reported
//            at the foot of this file: three are ALREADY WRITTEN in the
//            repo (two of them as nears, with the lever identified), two
//            are mid-function jump targets, and two are real functions
//            whose enumerated size is a mis-measure by a factor of 4 and 37.
//
//  GROUND TRUTH: the retail lithtech.exe, cl 8447 /O2.  Every disassembly
//            quoted below is from that image, never from one of our builds.
//
//  WHAT THE FAMILY TURNED OUT TO BE
//  --------------------------------
//  The batch is NOT one family.  It is a size band, and the one real family
//  inside it is FIVE rows: 0x00405df0 / 0x00405e20 / 0x00405e80 /
//  0x00405eb0 / 0x00405ee0 are slots 1, 2, 4, 5 and 6 of the client's
//  ILTCursor front end -- the eight-slot vtable at 0x004a1698 whose slots 0,
//  3 and 7 are already accounted for (slot 7 is src/engine/lithtech/
//  ltnew1.cpp's CClientCursorProxy::RefreshCursor).  One decode plus a table
//  covers all five, and iltcursor.h's argument counts (1,1,1,2,1,1,1,0)
//  match the shipped `ret $N` slot for slot, which is what anchors it.
//
//  The rest of the band is thirteen unrelated forwarders and splices that
//  the enumeration happened to size the same.  Their real homes, named per
//  banner, are: src/engine/server/netmgr.cpp (1), src/engine/server/
//  commonmodel.cpp (2), the CLTClient interface TU that ltnew1/ltnew2 are
//  building up (2), the client surface/blit TU (2), the client-shell TU (1),
//  the client file-manager TU (1), src/engine/lithtech/dmloader.cpp (1),
//  and two unreconstructed client TUs at 0x0045fxxx and 0x0048dxxx (3).
//
//  SHARED WITH src/engine/server:  FOUR of the eighteen belong to files
//  lithtech.exe compiles a SECOND time, so a change there moves server.dll
//  as well and both images must be re-measured:
//      0x004556b0  ..\server\netmgr.cpp      (CNetMgr::GetSessionList)
//      0x00447180  ..\server\commonmodel.cpp (Model, anim count)
//      0x00447750  ..\server\commonmodel.cpp (Model, node-tree compare)
//      0x0048db00  CObjVisitor::Visit -- declared in ..\server\objtypes.h
//                  at 0x1003a509, so the body is shared even though the TU
//                  that holds it is not reconstructed on either side.
//  All four are written here against VIEW classes so that this file stays
//  self-contained; when they are moved to their real homes they must be
//  respelled against the real Model / CNetMgr / CObjBase declarations.
//
// *********************************************************************** //

#include <new.h>
#include <stdio.h>         // sprintf, block 20
#include <string.h>
#include <windows.h>       // FindResourceA & co., block 20
#undef CopyFile            // block 6 is CLTClient::CopyFile, not the Win32 macro
#include "ltbasedefs.h"
#include "ltcodes.h"
#include "iltcursor.h"
#include "ltlink.h"        // LTLink / CheapLTLink / dl_Insert
#include "iltstream.h"     // block 21


// ======================================================================= //
//  1-5.  THE FAMILY -- the client's ILTCursor front end
//
//  vtable 0x004a1698, eight slots, bodies 0x00405dc0 .. 0x00405f27.
//
//  WHAT PINS THE TABLE.  A naive vtable scan merges 0x004a14e0's run with its
//  neighbours; the seam is at 0x004a1698 because CLTClient's own table ends
//  at slot 109 (0x004a1694) and the next eight pointers are eight
//  consecutive 0x30-byte bodies of ONE shape:
//
//      mov 0x4(%ecx),%eax          ; this->m_pClientMgr
//      test %eax,%eax
//      jne  .L
//      mov  $0x49,%eax             ; 73 == LT_NOTINITIALIZED (ltcodes.h:257)
//      ret  $N
//  .L: mov  0x1614(%eax),%ecx      ; CClientMgr::m_pCursor
//      ...push the arguments...
//      mov  (%ecx),%eax
//      call *0xNN(%eax)
//      ret  $N
//
//  and 0xNN runs 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c across the
//  eight in order.  iltcursor.h declares exactly eight pure virtuals, and
//  the `ret $N` of each body gives the argument count 1,1,1,2,1,1,1,0 --
//  which is SetCursorMode(1), GetCursorMode(1), IsCursorModeAvailable(1),
//  LoadCursorBitmapResource(2), FreeCursor(1), SetCursor(1),
//  IsValidCursor(1), RefreshCursor(0), in that order.  Slot 3 taking two
//  arguments in the middle of six one-argument slots is the anchor that
//  cannot be coincidence; slot 7 taking none and tail-JUMPING (`jmp
//  *0x1c(%eax)`, no stack to clean) is the second.
//
//  WHAT THE OBJECT AT CClientMgr+0x1614 IS.  cltcursor.cpp already reads
//  0x0040fbdb in CClientMgr::Init storing cursor_Create's result there, and
//  already maps that object's vtable 0x004a24c8 onto iltcursor.h slot for
//  slot.  So the eight targets these forward to are cltcursor.cpp's own
//  eight bodies.
//
//  CLASS AND FIELD NAMES: ltnew1.cpp's, kept verbatim so the two files
//  merge.  Only slot 7 is defined there; slots 1, 2, 4, 5, 6 are below.
//  Slots 0 and 3 are left alone.
// ======================================================================= //

// CClientMgr, as far as this reaches it.
class CClientMgrCursorView
{
public:
	uint8      m_pad_00[0x1614];
	ILTCursor* m_pCursor;                    // 0x1614
};

class CClientCursorProxy
{
public:
	uint8                 m_pad_00[0x04];    // 0x000  the vptr
	CClientMgrCursorView* m_pClientMgr;      // 0x004

	LTRESULT SetCursorMode(CursorMode cMode);                          // 0x00405dc0
	LTRESULT GetCursorMode(CursorMode& cMode);                         // 0x00405df0
	LTRESULT IsCursorModeAvailable(CursorMode cMode);                  // 0x00405e20
	LTRESULT LoadCursorBitmapResource(const char* pName,
	                                  HLTCURSOR& hCursor);             // 0x00405e50
	LTRESULT FreeCursor(const HLTCURSOR hCursor);                      // 0x00405e80
	LTRESULT SetCursor(HLTCURSOR hCursor);                             // 0x00405eb0
	LTRESULT IsValidCursor(HLTCURSOR hCursor);                         // 0x00405ee0
	LTRESULT RefreshCursor();                                          // 0x00405f10
};

// WHAT PINS ALL FIVE (one decode, five bodies)
//   * `ret $0x4` and %ecx read as a base on the first instruction -> a
//     __thiscall member with one stack argument.
//   * `mov $0x49,%eax` is 73, LT_NOTINITIALIZED, and it is the FALL-THROUGH
//     of `jne` -- an early `if (!p) return ...;`, not an if/else and not a
//     trailing guard (either of those hosts the constant at the far end).
//   * the forward is `mov (%ecx),%eax; call *0xNN(%eax)` -- a VIRTUAL call
//     through an ILTCursor*, which is what the member's declared type has
//     to be.  A named concrete CLTCursor* would devirtualise to `call
//     rel32` (a virtual call on a named global OBJECT
//     devirtualises; through a pointer it does not).
//   * the argument is materialised into %edx and pushed before the vtable
//     load in every one of them, which is plain right-to-left argument
//     evaluation of a one-argument call and needs no local.
//   * the value comes back in %eax untouched -> `return`, not a bare call.

// 0x00405df0, 34 bytes, `call *0x4(%eax)`.
LTRESULT CClientCursorProxy::GetCursorMode(CursorMode& cMode)
{
	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	return m_pClientMgr->m_pCursor->GetCursorMode(cMode);
}

// 0x00405e20, 34 bytes, `call *0x8(%eax)`.
LTRESULT CClientCursorProxy::IsCursorModeAvailable(CursorMode cMode)
{
	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	return m_pClientMgr->m_pCursor->IsCursorModeAvailable(cMode);
}

// 0x00405e80, 34 bytes, `call *0x10(%eax)`.  The `const` on the parameter is
// iltcursor.h's and costs nothing in the bytes.
LTRESULT CClientCursorProxy::FreeCursor(const HLTCURSOR hCursor)
{
	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	return m_pClientMgr->m_pCursor->FreeCursor(hCursor);
}

// 0x00405eb0, 34 bytes, `call *0x14(%eax)`.
LTRESULT CClientCursorProxy::SetCursor(HLTCURSOR hCursor)
{
	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	return m_pClientMgr->m_pCursor->SetCursor(hCursor);
}

// 0x00405ee0, 34 bytes, `call *0x18(%eax)`.
LTRESULT CClientCursorProxy::IsValidCursor(HLTCURSOR hCursor)
{
	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	return m_pClientMgr->m_pCursor->IsValidCursor(hCursor);
}


// ======================================================================= //
//  6.  CLTClient::CopyFile -- ILTCSBase slot 43
//
//  BELONGS IN: the CLTClient interface TU (the one ltnew1.cpp block A and
//  ltnew2.cpp block 1 are building up).  Client-only.
//
//  WHAT PINS THE SLOT.  Counting 0x004a14e0 against iltcsbase.h in
//  declaration order: slots 1..30 are the thirty message accessors
//  (0x0041ec90..0x0041f5f0), 31 GetAnimIndex, 32..41 the model and container
//  helpers, and slot 42 (0x00406610) contains the literal
//  "CLTClient::OpenFile" in two RETURN_ERROR sites -- iltcsbase.h:148
//  declares OpenFile and :153 declares CopyFile immediately after it, so
//  slot 43 at 0x004a158c -> 0x004066f0 is CopyFile.  The far side agrees:
//  slot 47 is FreeString (0x00407a70), 50 GetStringData, 53/54
//  GetTime/GetFrameTime, all already pinned in ltnew2.cpp.
//
//  WHAT PINS THE MEMBER.  OpenFile at slot 42 reaches the file manager as
//  `mov 0x3c0(%ecx),%ecx; mov 0x1534(%ecx),%edx` -- CLTClient's own back
//  pointer at +0x3c0 (NOT +0x04; that is the cursor proxy's) and
//  CClientMgr::m_pFileMgr at +0x1534, which clientmgr2.cpp's CClientMgrTail
//  already names.  0x004066f0 reads exactly the same pair.
// ======================================================================= //

class CClientMgrFileView
{
public:
	uint8  m_pad_00[0x1534];
	class CFileMgrView* m_pFileMgr;          // 0x1534
};

// 0x00404950 -- walk the file manager's tree list for the source name and
// copy it out to the destination.  cdecl, three arguments, answers an
// LTRESULT (`mov $0x1,%eax` for a null manager, `mov $0x3d,%eax` == 61 when
// no tree has the file).  Not reconstructed here; only its shape matters.
extern LTRESULT sub_00404950(class CFileMgrView* pFileMgr,
                             const char* pszSourceFile,
                             const char* pszDestFile);

class CLTClientFiles
{
public:
	uint8               m_pad_00[0x3c0];
	CClientMgrFileView* m_pClientMgr;        // 0x3c0

	LTRESULT CopyFile(const char* pszSourceFile, const char* pszDestFile);
};

// WHAT PINS IT
//   * `ret $0x8` -> __thiscall member, two stack arguments.
//   * the three pushes are, in emission order, arg2, arg1, m_pFileMgr, and
//     `add $0xc,%esp` after the call -> a CDECL callee of three arguments
//     taking the manager FIRST (right-to-left evaluation puts the last push
//     in the first parameter slot).  A member of the file manager would
//     have loaded %ecx instead.
//   * %eax is not touched between the `add` and the `ret $8` -> `return`.
//   * there is no null test on anything: the guard lives in the callee (its
//     first act is `test; jne; mov $0x1,%eax; ret`), which is why this body
//     is 34 bytes and OpenFile beside it is 224.
LTRESULT CLTClientFiles::CopyFile(const char* pszSourceFile,
                                  const char* pszDestFile)
{
	return sub_00404950(m_pClientMgr->m_pFileMgr, pszSourceFile, pszDestFile);
}


// ======================================================================= //
//  7.  CNetMgr::GetSessionList     ** SHARED: ..\server\netmgr.cpp **
//
//  src/engine/server/servermgr.h:1203 ALREADY DECLARES THIS FUNCTION AT
//  THIS ADDRESS -- `LTRESULT GetSessionList(NetSession*& pListHead, char*
//  pInfo);  // 0x004556b0` -- with a comment saying the session half of the
//  interface is shared code that only lithtech.exe calls, which is why the
//  address given is lithtech.exe's.  It has never been defined.  netmgr.cpp
//  is in lithtech.exe's compile list and server.dll's; /Gy + /OPT:REF is
//  why server.dll does not carry the body.
//
//  The neighbours confirm the row: our netmgr.obj matches FreeServiceList at
//  0x00455610, SelectService at 0x00455640, GetSessionName at 0x00455710
//  and UpdateSessionName at 0x00455740, and 0x004556b0 /
//  0x004556e0 are the two holes between them -- exactly GetSessionList and
//  FreeSessionList as servermgr.h has them.
// ======================================================================= //

class NetSession;

// servermgr.h's CBaseDriver, as far as this reaches it: slot 0x1c.
// tcpdriver.cpp:283 already names that slot
// `GetSessionList(NetSession** ppList, char* pAddresses)`.
class CBaseDriverSessionView
{
public:
	// Slot 0 is the scalar-deleting destructor; spelled as a plain pure
	// virtual so this scratch file emits nothing destructor-shaped.
	virtual void   _p00() = 0;                                        // 0x00
	virtual LTBOOL Init() = 0;                                        // 0x04
	virtual void   Term() = 0;                                        // 0x08
	virtual void   Update() = 0;                                      // 0x0c
	virtual void   _p10() = 0;                                        // 0x10
	virtual void   GetServiceList(NetSession** ppList) = 0;           // 0x14
	virtual int    SelectService(void* hService) = 0;                 // 0x18
	virtual LTRESULT GetSessionList(NetSession** ppList,
	                                char* pInfo) = 0;                 // 0x1c
};

class CNetMgrSessionView
{
public:
	uint8 m_pad_00[0x100];
	CBaseDriverSessionView* m_pMainDriver;   // 0x100

	LTRESULT GetSessionList(NetSession*& pListHead, char* pInfo);
};

// WHAT PINS IT
//   * `mov 0x100(%ecx),%ecx` -- servermgr.h already puts m_pMainDriver at
//     +0x100, and the value is loaded straight into %ecx because it is the
//     `this` of the forwarded call.
//   * `mov $0x49,%eax; ret $0x8` on the null arm -> LT_NOTINITIALIZED and
//     two stack arguments; the arm is the fall-through of `jne`, so it is
//     an early return.
//   * the two pushes are `mov 0x8(%esp),%edx; push %edx` then `mov
//     0x8(%esp),%edx; push %edx` -- the SECOND read is through the already
//     -pushed stack, i.e. it reads the FIRST parameter slot.  So the order
//     is (pListHead, pInfo) and the reference parameter is passed as the
//     raw address of the caller's slot, which is what `NetSession*&` gives
//     against the driver's `NetSession**`.
//   * `call *0x1c(%eax)` with no stack cleanup afterwards -> a __thiscall
//     virtual, callee-cleaned, and the value is returned untouched.
LTRESULT CNetMgrSessionView::GetSessionList(NetSession*& pListHead,
                                            char* pInfo)
{
	if (!m_pMainDriver)
		return LT_NOTINITIALIZED;

	return m_pMainDriver->GetSessionList(&pListHead, pInfo);
}


// ======================================================================= //
//  8-9.  Two Model members   ** SHARED: ..\server\commonmodel.cpp **
//
//  Both are pinned by our commonmodel.obj's matches in
//  lithtech.exe: Model::FindAnimInModel at 0x00446eb0,
//  Model::GetTotalAnimCount at 0x00447130, Model::sub_10017add at
//  0x00447670, Model::SetFilename at 0x004476c0, Model::FreeFilename at
//  0x00447720 and Model::FindSocket at 0x00447860.  0x00447180 and
//  0x00447750 are two of the holes in that run, and both read fields
//  commonmodel.h already names.
// ======================================================================= //

class CModelAnimView
{
public:
	uint8 m_pad_00[0x20];
	class CModelAnimsView* m_pModel;         // 0x20  ModelAnim::m_pModel
};

class CAnimInfoView
{
public:
	CModelAnimView* m_pAnim;                 // 0x00
	uint8           m_pad_04[0x20 - 0x04];   // AnimInfo is 0x20 bytes
};

class CModelAnimsView
{
public:
	uint8           m_pad_000[0x168];        // m_AnimArray's vptr is at 0x164
	CAnimInfoView*  m_pArray;                // 0x168  m_AnimArray.m_pArray
	uint32          m_nElements;             // 0x16c  m_AnimArray.m_nElements

	uint32 CountOwnAnims();                  // 0x00447180 (name ours)
};

// 0x00447180, 37 bytes.
//
// WHAT PINS IT
//   * no arguments and a bare `ret`, with %ecx used as a base -> thiscall,
//     nothing on the stack, and the answer is in %eax.
//   * `mov 0x16c(%ecx),%esi` then `xor %eax,%eax` then `test %esi,%esi;
//     jbe` -- the count is hoisted into a register ONCE before the guard
//     and the guard is UNSIGNED (`jbe`, not `jl`), which is `i <
//     m_AnimArray.m_nElements` with an unsigned `i`.  A signed loop gives
//     `jle`.
//   * inside the loop, `mov (%edx),%edi` then `cmp %ecx,0x20(%edi)` --
//     AnimInfo's first field is ModelAnim* and ModelAnim+0x20 is
//     Model::m_pModel (commonmodel.h:176), compared against %ecx, which is
//     still `this`.  So the test is `== this`, not `== pSomeArgument`;
//     there is no argument to compare against.
//   * `add $0x20,%edx; dec %esi; jne` -- strength reduction of the
//     subscript to a pointer, stride 0x20 == sizeof(AnimInfo).  The counter
//     `i` never appears, so the body must not use it (Model::FindAnimInModel
//     right above does use it, and keeps it in a register).
//   * `inc %eax` guarded by `jne` over ONE instruction: an `if` with no
//     else and no branchless form -- VC6 will not `sete`/`add` a counter.
//
// WHAT IT IS.  Model::GetTotalAnimCount (0x00447130, matched) walks the
// CHILD models and adds each one's whole table; this walks THIS model's
// table and counts only the entries whose animation belongs to this model
// -- i.e. the animations the model owns as opposed to the ones a child
// contributed.  Model::sub_100170f2 frees exactly that subset.  Name ours.
uint32 CModelAnimsView::CountOwnAnims()
{
	uint32 nCount = 0;
	uint32 i;

	for (i = 0; i < m_nElements; i++)
	{
		if (m_pArray[i].m_pAnim->m_pModel == this)
			nCount++;
	}

	return nCount;
}


// commonmodel.h:978 already declares this, as a friend of
// Model::CModelSub20C, at server.dll's 0x10016304; commonmodel.cpp:1526
// defines it.  In lithtech.exe it is 0x004451c0, and 0x00447750 is its only
// caller.
class CModelSub20CView;
extern LTBOOL mod_SameNodeTree(CModelSub20CView* pA, CModelSub20CView* pB,
                               CModelSub20CView** ppMismatch);

class CModelNodeTreeView
{
public:
	uint8              m_pad_000[0x2c0];
	CModelSub20CView*  m_pSub20c;            // 0x2c0

	LTBOOL SameNodeTree(CModelNodeTreeView* pOther,
	                    CModelSub20CView** ppMismatch);   // 0x00447750
};

// 0x00447750, 34 bytes.
//
// WHAT PINS IT
//   * `ret $0x8` with %ecx used as a base -> thiscall, two stack arguments.
//   * BOTH `this` and argument one are dereferenced at +0x2c0, which
//     commonmodel.h:1012 names Model::m_pSub20c -- so argument one is
//     another Model, not a node.
//   * three pushes then `add $0xc,%esp` -> a cdecl callee of three
//     arguments, and 0x004451c0 is mod_SameNodeTree (its body is
//     commonmodel.cpp:1526 instruction for instruction: compare
//     m_Array18.m_nElements at +0x20, store *ppMismatch = pA and return 0
//     on disagreement, else recurse pairwise over m_Array18.m_pArray at
//     +0x1c).
//   * push order is arg2 (%edx), then pOther's +0x2c0 (%eax), then this's
//     +0x2c0 (%ecx) -- last pushed is the first parameter, so the call is
//     mod_SameNodeTree(this->m_pSub20c, pOther->m_pSub20c, ppMismatch) and
//     THIS model's node is the one that goes out through ppMismatch.
//   * %eax untouched between the `add` and the `ret` -> `return`.
LTBOOL CModelNodeTreeView::SameNodeTree(CModelNodeTreeView* pOther,
                                        CModelSub20CView** ppMismatch)
{
	return mod_SameNodeTree(m_pSub20c, pOther->m_pSub20c, ppMismatch);
}


// ======================================================================= //
//  10.  CClientMgr -- the frame-rate tracker tick
//
//  BELONGS IN: the client TU that holds 0x0041fe30 (between csbase.cpp's
//  ILTCSBase bodies, which end at 0x0041fb80, and clienttexmgr.cpp, which
//  starts at 0x0041ffc0).  Client-only.
//
//  WHAT PINS THE CLASS.  The one caller, 0x0042c6ba, loads `mov 0xc(%esi),
//  %ecx` and the three instructions before it compute
//  `p->0x1258 = p->0x125c - p->0x1254` off the same pointer.  ltnew2.cpp
//  already pins CClientMgr+0x125c as GetTime's field and +0x1258 as
//  GetFrameTime's (ILTCSBase slots 53 and 54, `flds 0x125c` / `flds
//  0x1258`).  So `this` is CClientMgr and +0x1258 is m_FrameTime.
//
//  WHAT PINS THE MEMBER AT +0x11e4.  The call target 0x00461d30 is
//  CRateTracker::Update -- our netmgr.obj matches
//  ??0CRateTracker@@QAE@XZ at 0x00461d10 and ?Update@CRateTracker@@QAEXM@Z
//  at 0x00461d30 -- and servermgr.h:745 gives CRateTracker three floats,
//  m_Value at +0x00, m_Time at +0x04, m_Unknown08 at +0x08.  The `lea
//  0x11e4(%ecx),%eax` that becomes the callee's %ecx is therefore the
//  tracker's address, and the `flds 0x11e4(%ecx)` before it is m_Value.
// ======================================================================= //

class CRateTrackerView
{
public:
	void Update(float fDelta);               // 0x00461d30

	float m_Value;                           // 0x00
	float m_Time;                            // 0x04
	float m_Unknown08;                       // 0x08
};

class CClientMgrRateView
{
public:
	uint8            m_pad_0000[0x11e4];
	CRateTrackerView m_FrameRate;            // 0x11e4
	uint8            m_pad_11f0[0x1258 - 0x11f0];
	float            m_FrameTime;            // 0x1258

	void CountFrame();                       // 0x0041fe30 (name ours)
};

// 0x0041fe30, 35 bytes.
//
// WHAT PINS IT
//   * no arguments, bare `ret`, %ecx a base -> thiscall, nothing on the
//     stack.
//   * `flds 0x11e4(%ecx); fadds 0x4a1a08; ... fstps (%eax)` and the dword at
//     0x004a1a08 is 0x3f800000 -- 1.0f exactly.  So the
//     statement is `+= 1.0f` and not `++` on an integer.
//   * the store is through `lea 0x11e4(%ecx),%eax` rather than a direct
//     displacement, and %eax then becomes %ecx for the call: VC6
//     materialised the sub-object's address ONCE because the next statement
//     needs it as `this`.  Writing the two statements against two different
//     objects would give `fstps 0x11e4(%ecx)` and a second `lea`.
//   * `mov 0x1258(%ecx),%ecx; push %ecx` -- a float passed BY VALUE, moved
//     as a dword rather than fld/fstp'd, which is what VC6 does for a
//     32-bit float argument already in memory.
//   * a plain `call`, not a virtual one: CRateTracker has no vtable
//     (its constructor at 0x00461d10 writes only the three floats).
//   * the `fstps (%eax)` lands BEFORE the `mov 0x1258(%ecx),%ecx` load.
//     Written against the member directly (`m_FrameRate.m_Value += 1.0f;
//     m_FrameRate.Update(m_FrameTime);`) the load is scheduled first and the
//     store sinks two instructions -- size-exact, one block.  Naming the
//     sub-object once in a pointer local puts the store where shipped has it.
void CClientMgrRateView::CountFrame()
{
	CRateTrackerView *pRate = &m_FrameRate;
	pRate->m_Value += 1.0f;
	pRate->Update(m_FrameTime);
}


// ======================================================================= //
//  11.  A surface's renderer-side data, released
//
//  BELONGS IN: the client surface/blit TU (the run 0x0040c060..0x0040ca10;
//  its three callers are 0x0040c579, 0x0040ee6b and 0x0040f1d5, all inside
//  the surface code clientblit.cpp / clientinterface.cpp cover).
//  Client-only.
//
//  WHAT PINS THE GLOBAL AND THE FIELD.  0x004b17a4 is g_pRender: three
//  separate repo files already name it (clientmgr.h:252, clientblit.cpp:224,
//  d3d/d3d.cpp:123), and clientmgr.h models RenderStruct with function
//  POINTERS at +0xdc/+0xe0, so +0xcc is another entry in the same flat
//  table -- `call *0xcc(%eax)` on a pointer loaded from a global is a
//  function-pointer call, not a virtual one (a virtual would have loaded
//  the vptr first).  The argument is the surface's +0x0c, which
//  clientmgr.h:454 already names CSurface::m_pRenderData.
// ======================================================================= //

class CRenderStructFreeView
{
public:
	uint8 m_pad_00[0xcc];
	void  (*FreeSurfaceData)(void* pRenderData);   // 0x0cc
};

class CSurfaceRenderView
{
public:
	uint8 m_pad_00[0x0c];
	void* m_pRenderData;                     // 0x00c
};

extern CRenderStructFreeView* g_pRender;     // 0x004b17a4

// 0x0040c5a0, 36 bytes.
//
// WHAT PINS IT
//   * bare `ret` with the argument read as `mov 0x8(%esp),%esi` after one
//     push -> CDECL, one argument, caller-cleaned.  A member would use %ecx.
//   * the guard is `test %eax,%eax; je` over BOTH the call and the
//     clearing store, and the fall-through is the body -- an `if (p) { ...
//     }` with no else, not an early return (an early return would jump
//     forward to the epilogue from the other polarity).
//   * `mov 0xc(%esi),%eax` once, `push %eax` -- the field is read into a
//     register and reused for the test AND the argument, so the source
//     tests the member itself rather than a copy in a local (a local would
//     be identical here; see COIN-FLIPS).
//   * `add $0x4,%esp` after the indirect call -> the callee is cdecl with
//     one argument.
//   * `movl $0x0,0xc(%esi)` AFTER the call, through the object pointer, not
//     through a cached address.
void surf_FreeRenderData(CSurfaceRenderView* pSurface)
{
	if (pSurface->m_pRenderData)
	{
		g_pRender->FreeSurfaceData(pSurface->m_pRenderData);
		pSurface->m_pRenderData = LTNULL;
	}
}


// ======================================================================= //
//  12.  CObjectBank<T>::Free for the client's 0x54-byte element
//
//  BELONGS IN: the client-shell TU (the run 0x00413200..0x004156xx).  The
//  bank's four vtable slots are contiguous at 0x004a1dac..0x004a1db8 and
//  they are the four objbank.cpp declares -- deleting destructor
//  (0x00415270), Allocate (0x00414db0), Free (0x00414df0), Init
//  (0x00415210) -- which is what identifies the class:
//
//      Init  : `lea 0x4(%ecx),%esi; push $1; push %esi; call 0x00490bd0`
//              (sb_FreeChunks) then `push $0x20; push $0x54; push %esi;
//              call 0x00490b50` (sb_Init).  0x54 IS THE ELEMENT SIZE and
//              0x20 the per-block count -- objbank.cpp's CObjectBank::Init
//              word for word.
//      Alloc : sb_Allocate inlined off the same +0x04 bank, then a
//              placement-new that zeroes +0x2c, +0x34 and +0x38.
//      Free  : this function.
//
//  So the bank is `CObjectBank<T>` with sizeof(T) == 0x54, and its
//  StructBank at +0x04 puts sb_Free's list at +0x04+0x18 == +0x1c, which is
//  exactly the field this pushes onto.  The trivial instantiations
//  0x004049c0 / 0x004049d0 (matched: CObjectBank<CBankElem0c>::Free and
//  CObjectBank<CBankElem14>::Free) are the SAME function without the
//  destructor call, which is the control that proves the extra three
//  instructions here are T's destructor and nothing else.
//
//  WHAT T IS.  Its destructor releases a pointer at +0x2c through slot 2 of
//  that pointer's own vtable, and slot 2 is Release on every ILTMessage /
//  CAbstractIO in this engine (servermsg.h:314 puts ServerMessage::Release
//  at slot 2 of the same shape).  The element is therefore a client record
//  that owns a reference-counted message.  Names ours.
// ======================================================================= //

// Whatever the element holds at +0x2c: only slot 2 is ever reached.
class IReleasableView
{
public:
	virtual void _v00() = 0;                                          // 0x00
	virtual void AddRef() = 0;                                        // 0x04
	virtual void Release() = 0;                                       // 0x08
};

class CBankElem54View
{
public:
	// Both in-class, so both are expanded where the shipped code has them:
	// the constructor into Allocate (0x00414db0), the destructor into Free.
	//
	// The destructor does NOT null the field afterwards -- there is no
	// `movl $0x0` in Free, which is the whole difference from the surface
	// release at 0x0040c5a0 above.
	//
	// The constructor's two trailing zeros are an ARRAY cleared in a loop:
	// VC6 unrolls it and keeps the zero in a register (`xor %edx,%edx; mov
	// %edx,0x34(%eax); mov %edx,0x38(%eax)`), where two named fields -- as
	// separate stores, chained, member-initialised, floats, a struct pair or
	// one __int64 -- all give two `movl $0x0` immediates.  The +0x2c zero is
	// an immediate in the shipped bytes too, i.e. a plain assignment.
	CBankElem54View()
	{
		m_pMessage = LTNULL;

		for (int i = 0; i < 2; i++)
			m_n34[i] = 0;
	}

	~CBankElem54View()
	{
		if (m_pMessage)
			m_pMessage->Release();
	}

	uint8            m_pad_00[0x2c];
	IReleasableView* m_pMessage;             // 0x2c
	uint32           m_n30;                  // 0x30
	uint32           m_n34[2];               // 0x34
	uint8            m_pad_3c[0x54 - 0x3c];
};

// servermgr.h's StructBank, the fields Allocate/Free reach.
class CStructBankView
{
public:
	uint32 m_StructSize;                     // +0x00
	uint32 m_Unknown04;                      // +0x04
	uint32 m_Unknown08;                      // +0x08  sb_Grow's count
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                      // +0x18
};

// 0x00490c20 -- sb_Grow (servermgr.h: 0x1003e1e0 in server.dll).
void sb_GrowView(CStructBankView* pBank, uint32 nCount);

// servermgr.h's inline sb_Allocate, verbatim, against the view.
inline void* sb_AllocateView(CStructBankView* pBank)
{
	void* pStruct = pBank->m_pFreeList;

	if (!pStruct)
	{
		sb_GrowView(pBank, pBank->m_Unknown08);

		pStruct = pBank->m_pFreeList;
		if (!pStruct)
			return LTNULL;
	}

	pBank->m_pFreeList = *(void**)pStruct;
	return pStruct;
}

class CBankElem54BankView
{
public:
	// NOT declared virtual here even though the shipped slot is: a virtual
	// and a non-virtual member compile to the same body, and leaving the
	// class non-polymorphic keeps this scratch file from emitting a vftable
	// that references three bodies it does not define.
	void* Allocate();                        // 0x00414db0
	void  Free(void* pObj);                  // 0x00414df0

	uint8           m_pad_00[0x04];          // 0x00  the vptr
	CStructBankView m_Bank;                  // 0x04 .. 0x20
	uint8           m_pad_20[0x24 - 0x20];
};

// 0x00414df0, 33 bytes.
//
// WHAT PINS IT
//   * `ret $0x4` and %ecx copied to %esi on entry -> thiscall, one stack
//     argument, and `this` is live across the call so it needs a
//     callee-saved register -- which is why the shipped body pushes two.
//   * `mov 0x2c(%edi),%ecx; test %ecx,%ecx; je` over `mov (%ecx),%eax;
//     call *0x8(%eax)`: a guarded VIRTUAL call, slot 2, no arguments and no
//     stack cleanup.  This is the element destructor expanded inline; a
//     `delete` would have called an operator delete afterwards and a
//     non-inline destructor would be a `call rel32`.
//   * the three instructions after the join are sb_Free verbatim
//     (servermgr.h:266): read the list head, store it through the freed
//     object's first dword, make the object the new head.
void CBankElem54BankView::Free(void* pObj)
{
	((CBankElem54View*)pObj)->~CBankElem54View();

	// sb_Free(&m_Bank, pObj), expanded: servermgr.h declares it `inline`.
	*(void**)pObj      = m_Bank.m_pFreeList;
	m_Bank.m_pFreeList = pObj;
}

// 0x00414db0, 59 bytes -- slot 1 of the same four-slot vtable.
//
// WHAT PINS IT
//   * `lea 0x4(%ecx),%esi` then every bank field off %esi: sb_Allocate
//     expanded over &m_Bank, the free-list test first, sb_Grow (0x00490c20,
//     cdecl, (bank, bank+0x08)) on an empty list and a second test after.
//   * the `test %eax,%eax; je` on the popped pointer AFTER the pop is
//     objbank.cpp's `if (pRet) new(pRet) T;`, and the three stores behind it
//     are T's in-class constructor.
void* CBankElem54BankView::Allocate()
{
	CBankElem54View* pRet = (CBankElem54View*)sb_AllocateView(&m_Bank);

	if (pRet)
		new(pRet) CBankElem54View;

	return pRet;
}


// ======================================================================= //
//  13.  The client file manager -- open a file a record already names
//
//  BELONGS IN: the client file-manager TU (the run 0x00404400..0x004049b0,
//  immediately below CLTClient's constructor at 0x00404ba0).  Client-only.
//
//  WHAT THE NEIGHBOURS SAY.  0x004048c0 is CLTClient::OpenFile's worker
//  (called from slot 42 with the manager and a request struct) and
//  0x00404950 is CopyFile's (block 6 above).  All three end in the same
//  cdecl call to 0x00420ec0, whose body sprintfs "%s\%s" out of its first
//  argument's +0x20 and its second, then opens the result -- so 0x00420ec0
//  is `open <tree>, <name>, <flags>` and both of this function's field
//  reads are one of each.
//
//  WHAT THE RECORD IS.  Its constructor is the tail at 0x00404806: 0x20
//  bytes zeroed with `rep stos` (ecx=8), an LTLink at +0x04 spliced into a
//  circular list with its m_pData at +0x0c pointing back at the record
//  itself, +0x10 taken from the caller, +0x14 forced to 0xffff, +0x16 and
//  +0x18 set, and +0x1c filled from 0x00491070.  So +0x10 is the tree the
//  record was found in and +0x1c the name it owns.  Names ours.
// ======================================================================= //

class ILTStream;

class CFileMgrView;                                       // opaque here

class CFileRecView
{
public:
	uint8 m_pad_00[0x10];
	void* m_pTree;                           // 0x10
	uint8 m_pad_14[0x1c - 0x14];
	char* m_pName;                           // 0x1c
};

extern ILTStream* sub_00420ec0(void* pTree, char* pName, uint32 nFlags);

// 0x00404890, 34 bytes.
//
// WHAT PINS IT
//   * bare `ret` with both arguments read off the stack and %ecx never
//     touched -> CDECL, two arguments.
//   * `mov 0x4(%esp),%eax; test %eax,%eax; jne .L; xor %eax,%eax; ret` --
//     the FIRST argument is null-tested and then never used again, and the
//     null arm is the fall-through, so the source opens with a guard on it.
//     That dead parameter is not an artifact: 0x004048c0 beside it guards
//     the same argument and then does use it.
//   * `xor %eax,%eax` for the failure value, not `mov $0x0` -> the return
//     type is a pointer or an int, NOT an LTRESULT (every LTRESULT failure
//     in this image is `mov $imm,%eax`).  The callee returns a stream
//     pointer and this returns it untouched.
//   * push order is $0, then +0x1c, then +0x10 -> the callee takes
//     (tree, name, flags) and the flags are a literal zero.
ILTStream* sub_00404890(CFileMgrView* pFileMgr, CFileRecView* pRec)
{
	if (!pFileMgr)
		return LTNULL;

	return sub_00420ec0(pRec->m_pTree, pRec->m_pName, 0);
}


// ======================================================================= //
//  14.  CDMBank's block list -- AddHead
//
//  BELONGS IN: src/engine/lithtech/dmloader.cpp (the DirectMusic loader
//  TU; 0x00442cf0, thirty bytes further on, is CDMLoader::FileOpen, which
//  clientmusic.h:517 already names).  Client-only.
//
//  WHAT PINS THE OBJECT.  The one caller, 0x004415c8, is CDMBank::Grow for
//  the bank clientmusic.h:282 calls g_Unknown60Bank at 0x004b3f80: it
//  allocates a 12-byte header, allocates 0x4b3f84 records of 12 bytes each,
//  threads them onto the free list at 0x004b3f90 (which clientmusic.h
//  already names as that bank's m_pFree at +0x10) and then calls this with
//  `mov $0x4b3f88,%ecx` -- the bank's own +0x08.  So CDMBank carries a
//  two-pointer list of its allocated BLOCKS at +0x08/+0x0c, head first.
// ======================================================================= //

class CDMBlockView
{
public:
	CDMBlockView* m_pNext;                   // 0x00
	CDMBlockView* m_pPrev;                   // 0x04
	void*         m_pRecords;                // 0x08
};

class CDMBlockListView
{
public:
	CDMBlockView* m_pHead;                   // 0x00
	CDMBlockView* m_pTail;                   // 0x04

	void AddHead(CDMBlockView* pNode);       // 0x00442cc0
};

// 0x00442cc0, 37 bytes.
//
// WHAT PINS IT
//   * `ret $0x4` and %ecx a base -> thiscall, one stack argument.
//   * next is +0x00 and prev is +0x04 on the NODE, which the caller
//     confirms from the other side: it threads the free list through
//     `mov %edx,(%eax)` at stride 0x0c.  (Note this is NOT an SDK LTLink:
//     ltlink.h puts m_pPrev first.  A CDM block is its own three-dword
//     record.)
//   * `movl $0x0,0x4(%eax)` -- the new node's prev is set to a LITERAL
//     null, so the list is null-terminated rather than circular, and that
//     store happens BEFORE the head test.
//   * `mov (%ecx),%edx` is issued TWICE, once before the null store and
//     once after: VC6 could not prove `pNode->m_pPrev` does not alias
//     `m_pHead`, so the source reads m_pHead again rather than caching it
//     in a local.  A cached local gives one load.
//   * the head test's two arms are `mov %eax,0x4(%edx)` and
//     `mov %eax,0x4(%ecx)` -- prev-of-old-head on one side, m_pTail on the
//     other, which is the standard "empty list also sets the tail" shape.
void CDMBlockListView::AddHead(CDMBlockView* pNode)
{
	pNode->m_pNext = m_pHead;
	pNode->m_pPrev = LTNULL;

	if (m_pHead)
		m_pHead->m_pPrev = pNode;
	else
		m_pTail = pNode;

	m_pHead = pNode;
}


// ======================================================================= //
//  15.  CObjVisitor::Visit   ** SHARED (declared in ..\server\objtypes.h) **
//
//  src/engine/server/objtypes.h:159 ALREADY DECLARES THIS FUNCTION --
//  `class CObjVisitor { void Visit(CObjBase* pObj); // 0x1003a509 };` --
//  and objtypes.cpp:171 already has its only caller, CObjBase::_v04, which
//  our code matches at 0x0045ac80 in lithtech.exe.  That caller is the
//  proof: `push %ecx` (the object) then `mov 0x8(%esp),%ecx` (the visitor)
//  then `call 0x0048db00`, i.e. pVisitor->Visit(this).  The body has never
//  been written on either side; the TU that holds it (0x0048dxxx in the
//  client) is not reconstructed.
// ======================================================================= //

// ltlink.h's own types, deliberately: CheapLTLink puts m_pPrev FIRST and
// m_pNext second, and getting that order wrong inverts every store below.
class CObjVisitLinkView
{
public:
	LTLink m_Link;                           // 0x00 .. 0x0b
	uint32 m_Flag;                           // 0x0c
};

class CObjBaseVisitView
{
public:
	uint8  m_pad_00[0x44];
	CObjVisitLinkView m_Visit44;             // 0x44 .. 0x53
};

class CObjVisitorView
{
public:
	uint8  m_pad_00[0x74];
	LTLink m_Head74;                         // 0x74  a tie-off head

	void Visit(CObjBaseVisitView* pObj);     // 0x0048db00
};

// 0x0048db00, 35 bytes.
//
// WHAT PINS IT
//   * `ret $0x4`, %ecx a base -> thiscall, one stack argument.
//   * `add $0x74,%ecx` and `add $0x44,%eax` -- both addresses are formed
//     ONCE and every later store is a displacement off them, which is what
//     taking the address of two sub-objects gives.
//   * the four splice stores are, in order: (pLink) = pAfter,
//     0x4(pLink) = 0x4(pAfter), (pAfter's old next)->0x00 = pLink,
//     (pLink's prev)->0x04 = pLink.  With ltlink.h's prev/next order that
//     is CheapLTLink::AddAfter EXACTLY, including the last two being the
//     chained `pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink`
//     evaluated right to left -- which is why the second of the pair
//     RELOADS `(%eax)` instead of reusing a register.
//   * there is NO m_pData store and NO counter increment, so it is
//     dl_Insert (the two-argument helper), not dl_AddHead / dl_AddAfter
//     (the LTList four-argument ones, which do both).
//   * `movl $0x0,0xc(%eax)` is written through the LINK's base register at
//     +0x0c, i.e. pObj+0x50 -- the dword immediately past the link.  That
//     is the source: the link and the dword are ONE 16-byte sub-object at
//     +0x44, taken once as a pointer.  With a bare LTLink member and a
//     separate `pObj->m_Unknown50 = 0` (with or without an LTLink* local)
//     VC6 keeps pObj in %edx, forms the link with `lea 0x44(%edx),%eax` and
//     stores through `0x50(%edx)` -- size-exact, three blocks.
void CObjVisitorView::Visit(CObjBaseVisitView* pObj)
{
	CObjVisitLinkView *pLink = &pObj->m_Visit44;
	dl_Insert(&m_Head74, &pLink->m_Link);
	pLink->m_Flag = 0;
}


// ======================================================================= //
//  16.  CClass4A38D8::Slot10
//
//  BELONGS IN: the unreconstructed client TU at 0x0048cxxx..0x0048dxxx.
//  Same class ltnew2.cpp block 8 already opened: the fifteen-slot vtable at
//  0x004a38d8, whose slot 11 (0x0048d1f0) ltnew2.cpp writes.  0x0048d070 is
//  slot 10 of the same table (0x004a3900).  Class name and slot names are
//  ltnew2.cpp's, kept so the two merge.
// ======================================================================= //

class CClass4A38D8Slot10
{
public:
	// Non-virtual for the same reason as block 12: the body is identical
	// and the class stays non-polymorphic.
	void   Slot10(void* pQuery);             // 0x0048d070

	uint8  m_pad_00[0x04];                   // 0x00  the vptr
	class CSlot0ProviderView* m_p04;         // 0x04
	void*  m_Unknown08;                      // 0x08
	uint8  m_pad_0c[0x0c];                   // 0x0c  what sub_0048d0a0 walks
};

class CSlot0ProviderView
{
public:
	virtual void* _v00() = 0;                                         // 0x00
};

// 0x0048d0a0 -- the box walk, written as block 19 below.
class CWTNode2View;
class CWTBoxQueryView;
void wt_FindInBoxQ48D(CWTNode2View* pNode, CWTBoxQueryView* pQuery);

// 0x0048d070, 36 bytes.
//
// WHAT PINS IT
//   * `ret $0x4`, `mov %ecx,%esi` on entry -> thiscall, one stack argument,
//     `this` live across two calls.
//   * `mov 0x4(%esi),%ecx; mov (%ecx),%eax; call *(%eax)` -- a VIRTUAL call
//     on the member at +0x04, slot 0, NO arguments, and the result is
//     stored straight into +0x08.
//   * `mov %esi,(%eax)` writes `this` through the ARGUMENT: the query
//     object's first dword is a back pointer to this object, and it is set
//     before the walk, not after.
//   * two pushes then `add $0x8,%esp` -> a cdecl callee of two arguments;
//     the last push is `this+0x0c`, so the sub-object at +0x0c is the
//     first parameter and the query is the second.
//   * %eax is untouched between the `add` and the `ret $4`, and the callee
//     (block 19) is void -- it leaves fnstsw/call results in %eax -- so this
//     is a void call and Slot10 returns nothing.
void CClass4A38D8Slot10::Slot10(void* pQuery)
{
	m_Unknown08 = m_p04->_v00();

	*(void**)pQuery = this;

	wt_FindInBoxQ48D((CWTNode2View*)m_pad_0c, (CWTBoxQueryView*)pQuery);
}


// ======================================================================= //
//  17.  CLTTransform::GetRot -- ILTTransform slot 3
//
//  BELONGS IN: the same TU as ltnew2.cpp block 2.  Client-only.
//
//  WHAT PINS THE SLOT.  ltnew2.cpp already anchors 0x004a16c8 as
//  ILTTransform's vtable (CLTClient's constructor writes it to this+0x3a4
//  and files &this[0x3a4] in m_pTransformLT at +0x14) and writes slot 2,
//  GetPos, at 0x004896b0.  ilttransform.h's order is Get, Set, GetPos,
//  GetRot, ToMatrix, ...  -- so slot 3 at 0x004a16d4 -> 0x004896d0 is
//  GetRot.  ltnew2.cpp's own banner PREDICTED this row: "slot 3 (GetRot) is
//  0x30 -- one dword MORE, because a LTRotation is four floats where a
//  LTVector is three."  Four dword pairs is exactly what is here.
// ======================================================================= //

class CLTTransformRot
{
public:
	LTRESULT GetRot(LTransform& transform, LTRotation& rot);   // 0x004896d0
};

// 0x004896d0, 36 bytes.
//
// WHAT PINS IT
//   * `ret $0x8` and %ecx clobbered on the second instruction -> member,
//     two reference arguments, `this` unused.
//   * the source offsets are 0x0c, 0x10, 0x14, 0x18 -- LTransform is
//     ltbasetypes.h:108, m_Pos (three floats) then m_Rot, so m_Rot starts
//     at +0x0c and is four dwords.  The destination offsets are 0x00..0x0c.
//   * four `mov`/`mov` dword pairs, no x87 at all -> the compiler-generated
//     bitwise operator=, i.e. one whole-object assignment.  Field-by-field
//     float copies would be fld/fstp.
//   * `xor %eax,%eax` immediately before the `ret` -> `return LT_OK;`.
LTRESULT CLTTransformRot::GetRot(LTransform& transform, LTRotation& rot)
{
	rot = transform.m_Rot;

	return LT_OK;
}


// ======================================================================= //
//  18.  The pixel-format holder's constructor
//
//  BELONGS IN: the unreconstructed TU at 0x0045fxxx that also holds
//  0x0045f0e0 (PFormat::Init) and 0x0045f5e0.  Client-only.
//
//  WHAT PINS THE EMBEDDED OBJECT.  0x004a1ab4 is PFormat's one-entry
//  vtable: src/engine/lithtech/lttexmod.cpp:104 already reads its only slot
//  (0x0045f0e0, Init(bitdepth, aMask, rMask, gMask, bMask)) and fixes the
//  class at 0x38 bytes with the vptr its only constructor store.  This
//  function writes that vptr at this+0x20 and files &this[0x20] at this+0,
//  so the object is a 0x58-byte holder whose tail IS a PFormat.
//
//  THE SIZE IS CONFIRMED FROM THE CALLER.  0x0040ea30 (clientinterface.h:363
//  names it ci_FillRect) reserves `sub $0x7c` plus two pushes and builds
//  this at `lea 0x2c(%esp)`; 0x2c + 0x58 == 0x84 == 0x7c + 8, i.e. the
//  object ends exactly at the top of the frame.
// ======================================================================= //

class PFormat
{
public:
	// Declared, never defined -- lttexmod.cpp takes the same approach and
	// says why: PFormat has no user constructor, so the vptr store VC6
	// emits inline is the whole of its construction, and declaring a
	// constructor would add a call that is not in the bytes.
	virtual void Init(uint32 nBitDepth, uint32 aMask, uint32 rMask,
	                  uint32 gMask, uint32 bMask);          // 0x0045f0e0

	uint32 m_nBitDepth;                      // 0x04
	uint32 m_Masks[4];                       // 0x08
	uint8  m_pad_18[0x38 - 0x18];            // 0x18
};

class CPFormatQuadView
{
public:
	CPFormatQuadView() { m_p0 = LTNULL; m_p4 = LTNULL; m_p8 = LTNULL; m_pc = LTNULL; }

	void* m_p0;
	void* m_p4;
	void* m_p8;
	void* m_pc;
};

class CPFormatHolderView
{
public:
	CPFormatHolderView();                    // 0x0045f340

	PFormat*     m_pFormat;                  // 0x00  -> &m_Format
	void*        m_p04;                      // 0x04
	void*        m_p08;                      // 0x08
	CPFormatQuadView m_Quad0c;               // 0x0c .. 0x1b
	uint32       m_n1c;                      // 0x1c
	PFormat      m_Format;                   // 0x20 .. 0x58
};

// 0x0045f340, 37 bytes.
//
// WHAT PINS IT
//   * `mov %ecx,%eax` first and a bare `ret` last, with %eax never
//     reassigned -> a constructor: thiscall, no arguments, `this` returned.
//   * `xor %ecx,%ecx` once and seven stores of that register: seven fields
//     initialised to zero/null.  Only ONE field is not zero, and it is
//     `mov %edx,(%eax)` where %edx is `lea 0x20(%eax)` -- the holder points
//     at its own embedded format.
//   * `movl $0x4a1ab4,(%edx)` is a store of the vtable ADDRESS into the
//     embedded object's first dword: the member's implicit constructor,
//     expanded.  No call, so PFormat's constructor is compiler-generated.
//   * the emitted store order is 0x0c, 0x10, 0x14, 0x18, [vptr], 0x00,
//     0x04, 0x08, 0x1c -- CONSTRUCTION order: the four +0x0c..+0x18 nulls
//     are an earlier MEMBER's inline constructor (declared before
//     m_Format), then m_Format's implicit vptr store, then the body.  With
//     the four as plain fields zeroed in the body, the vptr store comes out
//     FIRST (size-exact, two blocks); with the member, identical.
CPFormatHolderView::CPFormatHolderView()
{
	m_pFormat = &m_Format;

	m_p04 = LTNULL;
	m_p08 = LTNULL;
	m_n1c = 0;
}


// ======================================================================= //
//  19.  The world tree's box walk
//
//  BELONGS IN: src/engine/lithtech/worldtree.cpp (the 0x0048cxxx..
//  0x0048dxxx world-tree TU; its CWTNodeN48D is the node here, its
//  CWTObjectN48D the object, and 0x0048d070 above -- that file's slot 2 --
//  is the one caller outside the recursion).  Written here, outside that
//  file; the view names carry Q48D so the two merge.
//
//  THE QUERY.  0x0048d070 stores the tree in its +0x00; the walk reads a
//  link index at +0x04, a box min at +0x08 and max at +0x14, a callback at
//  +0x2c and its user word at +0x30.
// ======================================================================= //

class CWTVec2View
{
public:
	float x, y, z;
};

class CWTObject2View
{
public:
	virtual void _v00() = 0;
	virtual void _v04() = 0;
	virtual void _v08() = 0;
	virtual void GetBBox(CWTVec2View* pMin, CWTVec2View* pMax) = 0;   // 0x0c

	uint8  m_pad_04[0x58 - 0x04];
	uint32 m_nTouchCode;                     // 0x58
};

class CWTTree2View
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_nCurCode;                       // 0x08  0x0048d070 sets it
};

typedef void (*WTBoxCallbackQ48D)(CWTObject2View* pObj, void* pUser);

class CWTBoxQueryView
{
public:
	CWTTree2View*     m_pTree;               // 0x00
	uint32            m_nLinkIndex;          // 0x04
	CWTVec2View       m_vMin;                // 0x08
	CWTVec2View       m_vMax;                // 0x14
	uint8             m_pad_20[0x2c - 0x20];
	WTBoxCallbackQ48D m_pfnCallback;         // 0x2c
	void*             m_pUserData;           // 0x30
};

class CWTNode2View
{
public:
	CheapLTLink   m_Objects[3];              // 0x00
	CWTVec2View   m_vBoxMin;                 // 0x18
	CWTVec2View   m_vBoxMax;                 // 0x24
	CWTVec2View   m_vCenter;                 // 0x30
	uint8         m_pad_3c[0x44 - 0x3c];
	uint32        m_nObjectsBelow;           // 0x44  0 -> nothing to visit
	uint8         m_pad_48[0x4c - 0x48];
	CWTNode2View* m_Children[4];             // 0x4c
};

// 0x0048d0a0, 334 bytes.  cdecl (node, query), recursive.
//
// WHAT PINS IT
//   * `mov 0x44(%ebp); je <epilogue>` first: nothing is done at all for a
//     node whose +0x44 is zero, children included.
//   * the object loop is worldtree.cpp's 0x0048d290 loop (head =
//     &m_Objects[index], data then next, touch-code stamp) with the
//     callback replaced by a GetBBox (slot 3, two pointers to the 0x18-byte
//     frame) and six overlap tests: min against the query max with
//     `test $0x41; je` (skip when greater), max against the query min with
//     `test $0x1; jne` (skip when less).
//   * then `m_Children[0]` null test and four recursions, split on the
//     centre's x (+0x30) and z (+0x38): min < centre and max > centre.
void wt_FindInBoxQ48D(CWTNode2View* pNode, CWTBoxQueryView* pQuery)
{
	CheapLTLink*    pHead;
	LTLink*         pCur;
	CWTObject2View* pObj;
	CWTVec2View     vMin, vMax;

	if (!pNode->m_nObjectsBelow)
		return;

	pHead = &pNode->m_Objects[pQuery->m_nLinkIndex];

	pCur = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj = (CWTObject2View*)pCur->m_pData;
		pCur = pCur->m_pNext;

		if (pObj->m_nTouchCode != pQuery->m_pTree->m_nCurCode)
		{
			pObj->m_nTouchCode = pQuery->m_pTree->m_nCurCode;

			pObj->GetBBox(&vMin, &vMax);

			if (vMin.x <= pQuery->m_vMax.x && vMin.y <= pQuery->m_vMax.y &&
			    vMin.z <= pQuery->m_vMax.z && vMax.x >= pQuery->m_vMin.x &&
			    vMax.y >= pQuery->m_vMin.y && vMax.z >= pQuery->m_vMin.z)
			{
				pQuery->m_pfnCallback(pObj, pQuery->m_pUserData);
			}
		}
	}

	if (!pNode->m_Children[0])
		return;

	if (pQuery->m_vMin.x < pNode->m_vCenter.x)
	{
		if (pQuery->m_vMin.z < pNode->m_vCenter.z)
			wt_FindInBoxQ48D(pNode->m_Children[0], pQuery);

		if (pQuery->m_vMax.z > pNode->m_vCenter.z)
			wt_FindInBoxQ48D(pNode->m_Children[1], pQuery);
	}

	if (pQuery->m_vMax.x > pNode->m_vCenter.x)
	{
		if (pQuery->m_vMin.z < pNode->m_vCenter.z)
			wt_FindInBoxQ48D(pNode->m_Children[2], pQuery);

		if (pQuery->m_vMax.z > pNode->m_vCenter.z)
			wt_FindInBoxQ48D(pNode->m_Children[3], pQuery);
	}
}


// ======================================================================= //
//  20.  The engine version out of the module's own resources
//
//  BELONGS IN: the "version pair" module transform.cpp's banner carves out
//  (0x0048a4b0 and this).  clientdsi.cpp's dsi_GetEngineVersion
//  (0x0042e7b0) already declares it under this name and calls it with the
//  instance handle; defining it here with the same signature resolves that
//  declaration.
//
//  EXTENT: 0x0048a510..0x0048a588, 121 bytes, not the CSV's 36.
// ======================================================================= //

struct CEngineVersionQ42E
{
	void GetString(char* pStr, uint32 nStrLen);   // 0x0048a4b0

	uint32 m_nMajor;                         // 0x00
	uint32 m_nMinor;                         // 0x04
};

// 0x0048a4b0, 85 bytes -- the pair as "major.minor" (LithTech's
// LTVersionInfo::GetString; clientwin.cpp calls it on CClientMgr+0x11d0 as
// CluVersionView::GetString). __thiscall, ret $8: sprintf "%lu.%lu"
// (0x004af4cc) into a 0x200-byte local with m_nMajor/m_nMinor, then, for a
// non-zero size, strncpy (0x00492b60) size-1 bytes and terminate.
void CEngineVersionQ42E::GetString(char* pStr, uint32 nStrLen)
{
	char str[512];

	sprintf(str, "%lu.%lu", m_nMajor, m_nMinor);
	if (nStrLen)
	{
		strncpy(pStr, str, nStrLen - 1);
		pStr[nStrLen - 1] = 0;
	}
}

// 0x0048a510.
//
// WHAT PINS IT
//   * cdecl, (hModule, pVersion): hModule in %ebx across every call.
//   * FindResourceA(hModule, 0x303a, 10 == RT_RCDATA), LoadResource,
//     SizeofResource == 8, LockResource, each failure `mov $1,%eax`
//     (LT_ERROR) with its own copy of the epilogue.
//   * two dword copies out of the locked data and `xor %eax,%eax`.
LTRESULT ver_GetModuleVersionQ42E(void* hModule, CEngineVersionQ42E* pVersion)
{
	HRSRC   hRes;
	HGLOBAL hData;
	uint32* pData;

	hRes = FindResourceA((HMODULE)hModule, MAKEINTRESOURCE(0x303a), RT_RCDATA);
	if (!hRes)
		return LT_ERROR;

	hData = LoadResource((HMODULE)hModule, hRes);
	if (!hData)
		return LT_ERROR;

	if (SizeofResource((HMODULE)hModule, hRes) != sizeof(CEngineVersionQ42E))
		return LT_ERROR;

	pData = (uint32*)LockResource(hData);
	if (!pData)
		return LT_ERROR;

	pVersion->m_nMajor = pData[0];
	pVersion->m_nMinor = pData[1];
	return LT_OK;
}


// ======================================================================= //
//  21.  CDMControlFileMgr::FileOpen   (clientmusic.h:517)
//
//  BELONGS IN: the DirectMusic TU (clientmusic.h declares the class; its
//  vtable 0x004a2558 holds 0x00442cf0 / 0x00442e50 / 0x00442e60 /
//  0x00442e90).  Written here against a view with the header's layout:
//  m_pData +0x2c, m_pCurPos +0x30, m_pEnd +0x34.
// ======================================================================= //

// clientmgr.h's CClientFileRef, its in-class constructor included.
class CClientFileRefQ442
{
public:
	CClientFileRefQ442() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

class CClientMgrQ442
{
public:
	uint8 m_pad_0000[0x1534];
	void* m_pFileMgr;                        // 0x1534
};

extern CClientMgrQ442* g_pClientMgrQ442;     // 0x004b17e0

// 0x004048c0 -- clientworldinfo.cpp's sub_4048c0.
ILTStream* sub_4048c0Q442(void* pFileMgr, CClientFileRefQ442* pRef);

void* dalloc(unsigned long size);            // 0x004290b0
void  dfree(void* ptr);                      // 0x004290c0

class CDMControlFileMgr
{
public:
	// The four slots of vtable 0x004a2558, in slot order (FileGetChar calls
	// FileEOF through +0x0c).  No constructor here, so no vtable is emitted.
	virtual int  FileOpen(const char* sName);        // 0x00442cf0
	virtual void FileClose();                        // 0x00442e50
	virtual int  FileGetChar();                      // 0x00442e60
	virtual int  FileEOF();                          // 0x00442e90

	uint8 m_pad_04[0x2c - 0x04];
	char* m_pData;                           // 0x2c
	char* m_pCurPos;                         // 0x30
	char* m_pEnd;                            // 0x34
};

// 0x00442cf0, 349 bytes.
//
// WHAT PINS IT
//   * `ret $4`, %ecx kept in %ebx: thiscall, one argument, the name.
//   * CClientFileRef's three zero stores, then dalloc(strlen+1) and an
//     inline strcpy of the name into it: the reference owns a copy.
//   * sub_4048c0(g_pClientMgr->m_pFileMgr, &ref); each failure dfrees the
//     name copy (and Releases the stream, slot 1) and returns 0.
//   * GetLen (slot 7) is called three times into ONE local, before each
//     use: dalloc(len) into +0x2c, Read (slot 2) of len bytes into it, and
//     the end pointer +0x34 = data + len - 1.  +0x30 = +0x2c.
int CDMControlFileMgr::FileOpen(const char* sName)
{
	CClientFileRefQ442 ref;
	ILTStream*         pStream;
	uint32             nLen;

	ref.m_nFileType = 0;
	ref.m_pFilename = (char*)dalloc(strlen(sName) + 1);
	strcpy(ref.m_pFilename, sName);

	pStream = sub_4048c0Q442(g_pClientMgrQ442->m_pFileMgr, &ref);
	if (!pStream)
	{
		dfree(ref.m_pFilename);
		return 0;
	}

	pStream->GetLen(&nLen);
	m_pData = (char*)dalloc(nLen);
	if (!m_pData)
	{
		dfree(ref.m_pFilename);
		pStream->Release();
		return 0;
	}

	pStream->GetLen(&nLen);
	if (pStream->Read(m_pData, nLen) != LT_OK)
	{
		dfree(ref.m_pFilename);
		pStream->Release();
		dfree(m_pData);
		return 0;
	}

	m_pCurPos = m_pData;
	pStream->GetLen(&nLen);
	m_pEnd = m_pData + nLen - 1;

	pStream->Release();
	dfree(ref.m_pFilename);
	return 1;
}

// 0x00442e50, 15 bytes (block 21 cont.).  The buffer goes back; the three
// pointers are left as they are.
void CDMControlFileMgr::FileClose()
{
	if (m_pData)
		dfree(m_pData);
}

// 0x00442e60, 41 bytes (block 21 cont.).  No buffer, or at the end (FileEOF
// through the vtable, `call *0xc(%eax)`), gives 0; otherwise the next byte,
// SIGN-extended (`movsbl`), so the cursor is a plain char*.
int CDMControlFileMgr::FileGetChar()
{
	if (!m_pData)
		return 0;

	if (FileEOF())
		return 0;

	return *m_pCurPos++;
}

// 0x00442e90, 23 bytes (block 21 cont.).  `cmp %eax,%edx ; sbb ; neg`: the
// cursor is past the end pointer (unsigned), which FileOpen set to the LAST
// byte, not one past it.
int CDMControlFileMgr::FileEOF()
{
	if (!m_pData)
		return 0;

	return m_pCurPos > m_pEnd;
}


// ======================================================================= //
//  22.  ci_GetBorderSize   (ILTClient +0x15c, clientinterface.h:338)
//
//  BELONGS IN: src/engine/lithtech/cissurf.cpp, the cis_ surface TU
//  (0x0040c000..0x0040f2ff): it sits between ci_SetupColor2 (0x0040ca30)
//  and cis_IsColumnColor (0x0040cc80), the helper it calls twice, and its
//  error name is "GetBorderSize" (0x004a8148).  Written here against views.
// ======================================================================= //

class CSurfaceQ40CA
{
public:
	uint8  m_pad_00[0x0c];
	void*  m_pRenderData;                    // 0x00c
	int    m_nWidth;                         // 0x010  compared SIGNED
	int    m_nHeight;                        // 0x014  (jge/jl) with LTRect's
	int    m_nPitch;                         // 0x018  ints
};

class RenderStructQ40CA
{
public:
	uint8  m_pad_00[0xd4];
	void*  (*LockSurface)(void* pRenderData);                       // 0x0d4
	void   (*UnlockSurface)(void* pRenderData);                     // 0x0d8
};

class CBlitterQ40CA
{
public:
	void ConvertColor(void* pFormat, HLTCOLOR hColor, uint32* pOut); // 0x0045f5e0
};

extern RenderStructQ40CA* g_pRenderQ40CA;    // 0x004b17a4 (g_pRender)
extern uint8              g_ScreenFormatQ40CA[];   // 0x004b1758
CBlitterQ40CA* cis_GetBlitterQ40CA();        // 0x0040c120

LTBOOL cis_IsColumnColorQ40CA(uint8* pPixels, uint32 nColor, uint32 nCount,
                              uint32 nStride);                  // 0x0040cc80
LTBOOL cis_IsRowColorQ40CA(uint8* pPixels, uint32 nColor, uint32 nCount);  // 0x0040cce0

// de_error.h's RETURN_ERROR, as cissurf.cpp writes it out.  The codes are
// stringised AFTER expansion: "60", "73", "1" (0x004a7160/68/94).
extern "C" void dsi_OnReturnErrorQ40CA(int code);            // 0x0042d760
extern int      g_DebugLevelQ40CA;                           // 0x004b0ab0
extern char*    g_pErrorFormatQ40CA;                         // 0x004a7110
void            con_ConsolePrintQ40CA(char* pFormat, ...);   // 0x0042e6f0

#define QA_ERRSTR_(x) #x
#define QA_ERRSTR(x)  QA_ERRSTR_(x)

#define QA_RETURN_ERROR(level, fnName, code) \
	{ \
		dsi_OnReturnErrorQ40CA(code); \
		if (g_DebugLevelQ40CA >= level) \
			con_ConsolePrintQ40CA(g_pErrorFormatQ40CA, fnName, \
			                      QA_ERRSTR(code), " "); \
		return code; \
	}

// 0x0040ca50, 546 bytes.
//
// WHAT PINS IT
//   * three refusals, in this order: null surface / null rect / surface
//     with no render data -> LT_INVALIDPARAMS (block at 0x0040cc37); no
//     g_pRender -> LT_NOTINITIALIZED (0x0040ca80); LockSurface (+0xd4)
//     null -> LT_ERROR (0x0040caca).
//   * the rect zeroed bottom, right, top, left (0x0040cb14..29): one
//     chained assignment.
//   * the transparent colour converted to the screen format into the dead
//     hSurface slot (0x0040cb09), then four scans that each walk one edge
//     inward while the row/column is all that colour: LEFT and RIGHT with
//     the column test (stride = pitch, count = height), TOP and BOTTOM with
//     the row test (count = width).  The left/right offset is NOT scaled
//     by the pixel size (0x0040cb52 `add %ebx,%edx`), as shipped.
//   * UnlockSurface (+0xd8) and LT_OK.
LTRESULT ci_GetBorderSizeQ40CA(HSURFACE hSurface, HLTCOLOR hColor, LTRect* pRect)
{
	CSurfaceQ40CA* pSurface = (CSurfaceQ40CA*)hSurface;
	uint8*         pData;
	uint32         nColor;

	if (!pSurface || !pRect || !pSurface->m_pRenderData)
		QA_RETURN_ERROR(1, "GetBorderSize", LT_INVALIDPARAMS)

	if (!g_pRenderQ40CA)
		QA_RETURN_ERROR(1, "GetBorderSize", LT_NOTINITIALIZED)

	pData = (uint8*)g_pRenderQ40CA->LockSurface(pSurface->m_pRenderData);
	if (!pData)
		QA_RETURN_ERROR(1, "GetBorderSize", LT_ERROR)

	pRect->left = pRect->top = pRect->right = pRect->bottom = 0;
	cis_GetBlitterQ40CA()->ConvertColor(g_ScreenFormatQ40CA, hColor, &nColor);

	while (pRect->left < pSurface->m_nWidth)
	{
		if (!cis_IsColumnColorQ40CA(pData + pRect->left, nColor,
		                            pSurface->m_nHeight, pSurface->m_nPitch))
			break;
		pRect->left++;
	}

	while (pRect->right < pSurface->m_nWidth)
	{
		if (!cis_IsColumnColorQ40CA(pData + (pSurface->m_nWidth - pRect->right - 1),
		                            nColor, pSurface->m_nHeight, pSurface->m_nPitch))
			break;
		pRect->right++;
	}

	while (pRect->top < pSurface->m_nHeight)
	{
		if (!cis_IsRowColorQ40CA(pData + pRect->top * pSurface->m_nPitch,
		                         nColor, pSurface->m_nWidth))
			break;
		pRect->top++;
	}

	while (pRect->bottom < pSurface->m_nHeight)
	{
		if (!cis_IsRowColorQ40CA(pData + (pSurface->m_nHeight - pRect->bottom - 1) * pSurface->m_nPitch,
		                         nColor, pSurface->m_nWidth))
			break;
		pRect->bottom++;
	}

	g_pRenderQ40CA->UnlockSurface(pSurface->m_pRenderData);
	return LT_OK;
}


// *********************************************************************** //
//
//  PER-ADDRESS REPORT
//  ------------------
//  DONE (written above), 18 rows:
//    0x00405df0  CClientCursorProxy::GetCursorMode          (family, slot 1)
//    0x00405e20  CClientCursorProxy::IsCursorModeAvailable  (family, slot 2)
//    0x00405e80  CClientCursorProxy::FreeCursor             (family, slot 4)
//    0x00405eb0  CClientCursorProxy::SetCursor              (family, slot 5)
//    0x00405ee0  CClientCursorProxy::IsValidCursor          (family, slot 6)
//    0x004066f0  CLTClient::CopyFile           (ILTCSBase slot 43)
//    0x004556b0  CNetMgr::GetSessionList       ** ..\server\netmgr.cpp **
//    0x00447180  Model::CountOwnAnims          ** ..\server\commonmodel.cpp **
//    0x00447750  Model::SameNodeTree           ** ..\server\commonmodel.cpp **
//    0x0041fe30  CClientMgr::CountFrame        (CRateTracker tick)
//    0x0040c5a0  surf_FreeRenderData           (g_pRender +0xcc)
//    0x00414df0  CObjectBank<0x54-elem>::Free  (client-shell TU)
//    0x00404890  sub_00404890                  (client file manager)
//    0x00442cc0  CDMBank block list AddHead    (dmloader.cpp)
//    0x0048db00  CObjVisitor::Visit            ** declared ..\server\objtypes.h **
//    0x0048d070  CClass4A38D8::Slot10
//    0x004896d0  CLTTransform::GetRot          (ILTTransform slot 3)
//    0x0045f340  CPFormatHolderView ctor       (0x0045fxxx TU)
//
//  SECOND PASS, 5 more rows:
//    0x00414db0  CObjectBank<0x54-elem>::Allocate  (block 12)   identical
//    0x0048d0a0  wt_FindInBoxQ48D -- worldtree.cpp's box walk (19) identical
//    0x0048a510  ver_GetModuleVersionQ42E, 121 bytes (block 20) identical
//    0x00442cf0  CDMControlFileMgr::FileOpen   (block 21)       identical
//    0x00401250  at_Reset -- still ..\server\modelanim.cpp's, see below.
//
//  THIRD PASS, 4 more rows, all identical:
//    0x00442e50  CDMControlFileMgr::FileClose  (block 21)
//    0x00442e60  CDMControlFileMgr::FileGetChar (block 21)
//    0x00442e90  CDMControlFileMgr::FileEOF    (block 21)
//    0x0040ca50  ci_GetBorderSize -- cissurf.cpp's (block 22)
//  Not here: 0x00446f20 is ..\server\commonmodel.cpp's Model::sub_10017517
//  (near: /O2 expands Mat_InverseTransformation where shipped calls
//  0x00446fc0 -- a known dead end), and 0x00470c70 is
//  ..\server\smgr.cpp's si_CreateObjectProps.
//
//  ALREADY WRITTEN IN THE REPO -- do NOT write these again, 3 rows:
//
//    0x00401250  at_Reset, src/engine/server/modelanim.cpp:348.
//                modelanim.cpp is in lithtech.exe's compile list, so this
//                row is the /O2 twin of a function server.dll already
//                matches at /O1 (0x100011c2, 37 bytes, byte-identical --
//                a byte diff confirms).  The /O2 twin is a NEAR: ours 35 bytes /
//                11 instructions, shipped 35 / 11, and the ONE difference
//                is where `m_TimeRef.m_Percent = 0.0f;` lands.
//                    ours    : xor %ecx,%ecx / mov 0x28,%dx / mov %ecx,0x30
//                              / mov %dx,0x20 / ...
//                    shipped : mov 0x28,%cx / mov %cx,0x20 / xor %ecx,%ecx
//                              / mov %ecx,0x14 / ... / mov %ecx,0x30
//                The shipped /O2 build puts the Percent store LAST; our
//                source has it FIRST, which is where server.dll's matching
//                /O1 build needs it (there it is `fldz`/`fstps 0x30` two
//                instructions in).  EXACT ALTERNATIVE: move
//                `pTracker->m_TimeRef.m_Percent = 0.0f;` from the first
//                line of at_Reset to the last.  That is a one-line change
//                that MUST be measured against BOTH images -- if /O1 does
//                not hoist the fldz back to the front, it trades a client
//                near for a server regression, and then the answer is that
//                the two builds cannot come from one spelling and the
//                client copy stays a near.
//                MEASURED: moved last, server.dll still identical
//                (modelanim 13/14) and lithtech.exe 0x00401250 identical
//                (modelanim 5 -> 6).  Applied in modelanim.cpp.
//
//    0x00470f20  si_GetServerFlags, src/engine/server/smgr.cpp:2677.
//                Also a NEAR, also already correct in shape: ours 37 bytes
//                / 13 instructions, shipped 37 / 13, every operand,
//                constant and branch identical.  The whole residual is a
//                two-register transposition --
//                    ours    : mov g_pServerMgr,%ecx / mov 0x1a0(%ecx),%edx
//                    shipped : mov g_pServerMgr,%edx / mov 0x1a0(%edx),%ecx
//                and %dl/%cl follow it into the `test $0x10`.  The source
//                names the MANAGER in a local (`CServerMgr* pMgr =
//                g_pServerMgr;`) so the manager gets the first register;
//                the shipped build gives the first register to the STATUS
//                WORD instead, which is what naming that in a local and
//                letting the global be a CSE temp does.  EXACT ALTERNATIVE:
//                    uint32 si_GetServerFlags()
//                    {
//                        uint32 nStatus;
//                        uint32 nFlags;
//                        nStatus = g_pServerMgr->m_Unknown1a0;
//                        nFlags  = ((nStatus >> 1) & SS_DEMOPLAYBACK) |
//                                  g_pServerMgr->m_ServerFlags;
//                        if (nStatus & 0x10)
//                            nFlags |= SS_CACHING;
//                        return nFlags;
//                    }
//                smgr.cpp is shared and server.dll's copy (0x100308cf, 33
//                bytes) MATCHES today, so this too must be re-measured on
//                both sides.
//
//    0x0045a210  CMessageQueueBase::InsertBefore,
//                src/engine/server/lthread.h -- an in-class inline, so our
//                build expands it at every call site and emits no COMDAT.
//                The shipped image has an out-of-line copy because ONE
//                caller declined the expansion: 0x004583f9 (netmgr.cpp's
//                SendPacketData, 0x004582f0) has Prepend's body written out
//                with `call 0x0045a210` for the splice and `call
//                0x0045a1c0` for Append.  Our compiler expands the splice
//                there, so src/engine/lithtech/nettrace.cpp takes the
//                function's address to emit it.  IDENTICAL once it returns
//                pMsg (the eax it ends with).  Do not move InsertBefore out
//                of the class -- server.dll's Prepend (0x10015ad0) expands
//                it inline and depends on it staying inline.
//
//  NOT A FUNCTION, 2 rows:
//
//    0x00428e8f  The second half of the function at 0x00428e70, reached by
//                its own `jne 0x428e8f` five instructions in.  It opens
//                `push %esi` and closes `pop %esi; mov %ebx,%eax; pop %edi;
//                pop %ebx` -- two pops with no matching pushes, which is
//                the tell.  The real function is 0x00428e70..0x00428eaf,
//                64 bytes: collection.cpp's
//                `BaseNew(LAlloc*, CCollElemC0*, uint32)` (allocate n *
//                0xc0 bytes through the allocator's slot 1, then run the
//                element constructor at 0x00425c00 over each).  Our
//                BaseNew already matches at 0x00428e70, so this row is
//                covered; the CSV's 31/33 split is a mis-measure at the
//                branch target.
//
//    0x00476e5a  The loop entry INSIDE CServerMgr::SetGlobalLightObject at
//                0x00476e40, reached by falling through its `je 0x476e78`.
//                Its first instruction `push %edi` is matched by the `pop
//                %edi` at 0x00476e77 and it uses %eax/%edx/%ecx set up
//                before 0x00476e5a.  The real function is
//                0x00476e40..0x00476e7b, 59 bytes -- src/engine/server/
//                smgr.cpp already has it and it already MATCHES at
//                0x00476e40.  Nothing to do.
//
//  MIS-SIZED -- real functions, but not 36/37 bytes, 2 rows:
//
//    0x0048a510  A genuine call target, but the row's 36 is the distance to
//                0x0048a534, which is its own `jne` target nine
//                instructions in (the CSV then charges the remainder to two
//                "gap" rows of 23 and 62 bytes).  The real extent is
//                0x0048a510..0x0048a588, 121 bytes (padding to 0x0048a58f):
//                FindResourceA / LoadResource / SizeofResource /
//                LockResource with `mov $0x1,%eax` (LT_ERROR) on each
//                failure.  WRITTEN since as block 20.
//
//    0x0047f020  Same fault, far worse: the row's 37 is the distance to
//                0x0047f045, the `jne` target four comparisons in, and the
//                CSV charges the next 1347 bytes to a "gap".  The real
//                function is ~1384 bytes; it opens `sub $0x24,%esp` (a
//                frame this "37 bytes" never uses), tests three floats at
//                +0x44/+0x48/+0x4c of a CSoundInstance-shaped object
//                against zero and returns 1 when all three are zero.
//                clientsoundinst.cpp's neighbourhood.
//
// *********************************************************************** //
//  COIN-FLIPS I COULD NOT SETTLE
// *********************************************************************** //
//
//   1. 0x00442cc0 -- whether `m_pHead = pNode;` is written once after the
//      if/else (as above) or once inside EACH arm.  The shipped tail is
//      duplicated: `mov %eax,0x4(%edx); mov %eax,(%ecx); ret $4` and
//      `mov %eax,0x4(%ecx); mov %eax,(%ecx); ret $4`, two five-byte copies
//      where a `jmp` would be two bytes.  Written once, VC6 may cross-jump
//      them into one -- and src/engine/server/netmgr.cpp:527 records
//      exactly that happening to CMessageQueueBase::Append ("ours
//      cross-jumps the two arms' shared `m_pTail = pMsg` into one copy and
//      the shipped build leaves both"), which is why this is a live risk
//      rather than a theoretical one.  If this comes back with one copy and
//      a forward jump, the alternative spelling is:
//          if (m_pHead) { m_pHead->m_pPrev = pNode; m_pHead = pNode; }
//          else         { m_pTail = pNode;          m_pHead = pNode; }
//      That is the only lever; the branch polarity and arm order are
//      already the shipped ones.
//
//   2. SETTLED (identical): the +0x0c..+0x18 nulls are a member's inline
//      constructor, see the function.  Original note kept for the record:
//      0x0045f340 -- where the embedded PFormat's vptr store lands.  C++
//      runs member constructors BEFORE the constructor body, so a strict
//      reading wants `movl $0x4a1ab4,0x20(%eax)` first; the shipped build
//      has it fifth, between the +0x18 store and the +0x00 store, with the
//      `lea 0x20(%eax)` hoisted to instruction three.  Written as above the
//      compiler is free to schedule it there and probably will (the four
//      stores it moves past are to provably distinct addresses).  If it
//      comes out size-exact with the vptr store at the top, the source did
//      NOT have a PFormat member: the alternative is a holder that carries
//      0x38 bytes of padding and an explicit `((void**)&m_Format)[0]`-style
//      construction, which cannot be spelled without knowing the
//      real class -- so the fallback is to reorder the body, putting
//      the four +0x0c..+0x18 stores after the `m_pFormat = &m_Format;`.
//
//   3. 0x0048d070 -- SETTLED by writing its callee (block 19): 0x0048d0a0
//      is void, so Slot10 is a void call.  DISPROVED alternative: the store `*(void**)pQuery = this` is NOT
//      inside the callee -- it is `mov %esi,(%eax)` in this body, two
//      instructions before the push, and %esi is still `this`.
//
//   4. 0x0040c5a0 -- whether the member read is cached in a local:
//          void* pData = pSurface->m_pRenderData;
//          if (pData) { g_pRender->FreeSurfaceData(pData);
//                       pSurface->m_pRenderData = LTNULL; }
//      is byte-identical to the spelling above, because VC6 keeps the one
//      load in %eax either way.  No lever, no risk -- recorded only so the
//      next reader does not spend the probe.
//
//   5. 0x00414df0 -- `((T*)pObj)->~T()` versus the destructor body written
//      out inline in Free.  Both give the same three instructions.  What is
//      NOT a flip: the field is not nulled afterwards, and there is no
//      operator delete, so it is a destructor call and not a `delete`.
//
// *********************************************************************** //
