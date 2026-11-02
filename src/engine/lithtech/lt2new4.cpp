// *********************************************************************** //
//
//  MODULE:   LT2NEW4 (.CPP) -- scratch file, NOT a drop-in TU
//
//  PURPOSE:  Fifteen of the twenty-five 42..46-byte rows out of
//            lithtech.exe.  The other ten are reported at the bottom of
//            this file: two are mid-function jump targets, three are
//            mis-sized rows whose real bodies are 384..1622 bytes, two are
//            compiler-generated scalar deleting destructors, and two are
//            already reconstructed inside src/engine/server (they reach
//            lithtech.exe because 44 of those files are compiled a SECOND
//            time into the exe).
//
//  THE FAMILY.  The file is dominated by ONE family and the vtable that
//  names it: CLTClient's ILTClient vtable at 0x004a14e0, anchored through
//  ILTCSBase.  Slot 0 is 0x00407630, slots 1..30 are ILTCSBase's own
//  non-pure message bodies (0x0041ec90..0x0041f5f0), slots 56..70 are its
//  non-pure rotation bodies (0x0041f640..0x0041fb80), slot 71 is
//  ~ILTClient (0x00407c00).  That is 46 free anchors, so slot == the
//  iltcsbase.h virtual index for 0..70 and slot == 71 + the iltclient.h
//  index after that.
//
//  Five of the twenty-five are slots of that table, and three of those five
//  are one sub-family: the CLTClient model calls are all the SAME six
//  instructions -- fetch m_pModelLT (this+0x10), call ILTModel slot 0x28
//  (GetMainTracker) with a tracker local in the dead hObj parameter slot,
//  and on LT_OK forward to one more ILTModel slot.  Eight of them ship,
//  0x004076c0..0x00407850, distinguished ONLY by that last slot number:
//
//      0x004076c0  0x3c GetCurAnim         slot  33 GetModelAnimation
//      0x00407700  0x40 SetCurAnim         slot  32 SetModelAnimation   <-- ours
//      0x00407730  0x44 ResetAnim          slot  36 ResetModelAnimation
//      0x00407760  0x30 GetPlaybackState   slot  37 GetModelPlaybackState
//      0x004077a0  0x48 GetLooping         slot  35 GetModelLooping
//      0x004077e0  0x4c SetLooping         slot  34 SetModelLooping     <-- ours
//      0x00407810  0x50 GetPlaying         slot 104 GetModelPlaying
//      0x00407850  0x54 SetPlaying         slot 105 SetModelPlaying     <-- ours
//
//  A CORRECTION.  ltnew1.cpp's banner says the shipped CLTClient vtable "stops at
//  SetModelLooping" and does not carry GetModelPlaying, SetModelPlaying,
//  StartQuery, UpdateQuery, GetQueryResults, EndQuery.  It carries all six,
//  at slots 104..109 -- proved by the four error-tag literals
//  "StartQuery" (0x00406950), "UpdateQuery" (0x004069b0),
//  "GetQueryResults" (0x00406a00) and by 0x00407810/0x00407850 forwarding
//  to ILTModel's GetPlaying/SetPlaying.  What the shipped header actually
//  drops is LT2 iltclient.h's idx 33..38 -- GetModelAnimation,
//  SetModelAnimation, ResetModelAnimation, GetModelPlaybackState,
//  GetModelLooping, SetModelLooping -- the six that merely duplicate
//  ILTCSBase.  Six missing entries shift LT2's idx 39..44 from slots
//  110..115 down onto 104..109, and the table genuinely ends at 109.
//  Slot 110 (0x004a1698) starts an unrelated EIGHT-slot forwarder table
//  (0x00405dc0..0x00405f10, each `m_pClientMgr(+4)->[0x1614]->slot n`,
//  LT_NOTINITIALIZED when the manager is null), and 0x004a16b8 starts
//  ILTLightAnim's four (see block 6).
//
//  GROUND TRUTH: the retail lithtech.exe, cl 8447 /O2.
//
// *********************************************************************** //

#include <windows.h>          // (was for block 12, now removed; kept so the TU's headers are unchanged)
#include <string.h>           // memcpy, strncpy, strlen

#include "ltbasedefs.h"
#include "ltlink.h"
#include "ltmatrix.h"
#include "ltrotation.h"
#include "ltquatbase.h"
#include "iltmodel.h"
#include "iltlightanim.h"   // HLIGHTANIM


// ======================================================================= //
//  1..5.  CLTClient -- five slots of the vtable at 0x004a14e0
//
//  WHAT PINS THE CLASS.  Every one of the five is `__thiscall` and reads
//  `this` at one of exactly two displacements: 0x10 and 0x3c0.  0x10 is
//  m_pModelLT -- iltcsbase.h's protected block is
//  `ILTMath m_MathLT; ILTCommon *m_pCommonLT; ILTPhysics *m_pPhysicsLT;
//  ILTModel *m_pModelLT; ILTTransform *m_pTransformLT; ...`, so after the
//  vptr those land at 0x04, 0x08, 0x0c, 0x10, 0x14 (this is the same
//  arithmetic ltnew2.cpp used to pin m_pTransformLT at 0x14).  0x3c0 is
//  the client manager: 0x00407880 (slot 38, FreeUnusedModels) is nothing
//  but `mov 0x3c0(%ecx),%ecx; jmp 0x004114a0`.
// ======================================================================= //

class CClientMgr;

// The console-variable record.  0x00407b20 (slot 52, GetVarValueString)
// reads a `char*` out of +0x14 of exactly this record, and so does ours.
struct LTCommandVar
{
	uint8  m_pad_00[0x14];
	char*  m_pStringVal;                     // 0x14
};

// Opaque; only its ADDRESS inside the manager is ever taken.
struct ConsoleState
{
	uint8 m_Opaque[4];
};

// 0x0041b3e0, __cdecl.  Two arguments; the state first.
extern LTCommandVar* cc_FindConsoleVar(ConsoleState* pState, char* pName);

class CClientMgrView
{
public:
	uint8        m_pad_0000[0x1268];
	// The SERVER's console state, mirrored into the client manager -- this
	// is the copy GetServerConVarValue* reads, and it is a different
	// object from the client's own (0x004b2f00, a file-scope global).
	ConsoleState m_ServerConsoleState;        // 0x1268
};

// 0x004b44a0.  `fildll` of the value with an explicitly-zeroed high dword
// is VC6's UNSIGNED int -> float sequence; a signed int is a plain
// `fildl`, and a uint8/uint16 field would be a `movzx` first.  It is a
// field of clientrender.cpp's g_RenderStruct (+0x148, m_Unknown148 there),
// not a global of its own.
class RenderStruct
{
public:
	uint8  m_pad_000[0x148];
	uint32 m_Unknown148;                      // 0x148  0x004b44a0
};
extern RenderStruct g_RenderStruct;           // 0x004b4358

class CLTClient
{
public:
	// slot 32 / iltcsbase.h idx 32
	void      SetModelAnimation(HOBJECT hObj, HMODELANIM hAnim);      // 0x00407700
	// slot 34 / iltcsbase.h idx 34
	void      SetModelLooping(HOBJECT hObj, LTBOOL bLoop);            // 0x004077e0
	// slot 105 / iltclient.h idx 40 (see the correction above)
	void      SetModelPlaying(HLOCALOBJ hObj, LTBOOL bPlaying);       // 0x00407850
	// slot 77 / iltclient.h idx 6
	char*     GetServerConVarValueString(char* pName);                // 0x00406c60
	// slot 82 / iltclient.h idx 11
	LTRESULT  GetAmbientLight(float& light);                          // 0x00407320

public:
	uint8          m_pad_0000[0x10];
	ILTModel*      m_pModelLT;                // 0x010
	uint8          m_pad_0014[0x3c0 - 0x14];
	CClientMgrView* m_pClientMgr;             // 0x3c0
};


// --- 1.  0x00407700 -------------------------------------------------- //
// WHAT PINS IT
//   * `ret $0x8` and `mov 0x10(%ecx),%esi` -> thiscall, two stack args.
//   * `lea 0x8(%esp),%ecx` with NO `sub %esp` in the prologue: the tracker
//     out-parameter lives in the DEAD hObj parameter slot, which is what a
//     plain local gives once hObj has been read into %edx
//     ("A parameter's stack slot can hold a local whose address is taken").
//     A `sub $4,%esp` would mean the source kept hObj alive.
//   * `call *0x28(%eax)` is iltmodel.h index 10, GetMainTracker; the second
//     call `*0x40` is index 16, SetCurAnim.  Both go through the SAME
//     %esi, so `m_pModelLT` is read once -- but the vtable IS reloaded
//     (`mov (%esi),%eax` twice), which is what a second call through the
//     same pointer gives; a cached `void** vtbl` local would load once.
//   * `test %eax,%eax; jne <epilogue>` with the SetCurAnim call falling
//     through: the success arm is the fall-through, so the source guards
//     positively (`if (... == LT_OK) ...`), not `if (dr != LT_OK) return;`
//     -- the latter puts the epilogue between the two calls.
//   * no `xor %eax,%eax` anywhere -> the return type is `void` (iltcsbase.h
//     declares SetModelAnimation void), not LTRESULT.
void CLTClient::SetModelAnimation(HOBJECT hObj, HMODELANIM hAnim)
{
	LTAnimTracker* pTracker;
	ILTModel*      pModelLT = m_pModelLT;

	if (pModelLT->GetMainTracker(hObj, pTracker) == LT_OK)
		pModelLT->SetCurAnim(pTracker, hAnim);
}


// --- 2.  0x004077e0 -------------------------------------------------- //
// WHAT PINS IT
//   Byte-for-byte block 1 with `*0x4c` in place of `*0x40`: iltmodel.h
//   index 19, SetLooping(LTAnimTracker*, LTBOOL).  Same `ret $8`, same
//   dead-parameter tracker slot, same fall-through success arm.  The
//   GETTER beside it (0x004077a0, slot 35) is the same six instructions
//   plus `cmp $0x56` -- LT_YES -- which is how the pair is told apart.
void CLTClient::SetModelLooping(HOBJECT hObj, LTBOOL bLoop)
{
	LTAnimTracker* pTracker;
	ILTModel*      pModelLT = m_pModelLT;

	if (pModelLT->GetMainTracker(hObj, pTracker) == LT_OK)
		pModelLT->SetLooping(pTracker, bLoop);
}


// --- 3.  0x00407850 -------------------------------------------------- //
// WHAT PINS IT
//   Same again with `*0x54` -- iltmodel.h index 21, SetPlaying.  Its
//   partner 0x00407810 (`*0x50` GetPlaying, `cmp $0x56`) sits at slot 104
//   and this at 105, which is what proves the shipped ILTClient really
//   does declare GetModelPlaying/SetModelPlaying (see the header banner).
//   iltclient.h declares SetModelPlaying void, and again there is no
//   `xor %eax,%eax`.
void CLTClient::SetModelPlaying(HLOCALOBJ hObj, LTBOOL bPlaying)
{
	LTAnimTracker* pTracker;
	ILTModel*      pModelLT = m_pModelLT;

	if (pModelLT->GetMainTracker(hObj, pTracker) == LT_OK)
		pModelLT->SetPlaying(pTracker, bPlaying);
}


// --- 4.  0x00406c60 -------------------------------------------------- //
// WHAT PINS IT
//   * slot 77 = iltclient.h idx 6 = `char* GetServerConVarValueString`.
//     The neighbour at slot 76 (0x00406c30) is the float twin, and slot 52
//     (0x00407b20, GetVarValueString, already written up in ltnew2.cpp)
//     reads the same `+0x14` out of the same record.
//   * `add $0x1268,%eax` on the loaded manager pointer -- an ADD, not an
//     `lea` -- is `&m_pClientMgr->m_ServerConsoleState`; the two pushes
//     are `pName` then the state, so cdecl argument order is
//     (pState, pName).
//   * THE SHAPE.  Both `je` land on ONE `xor %eax,%eax; ret $4` block that
//     sits at the very END, after `mov 0x14(%eax),%eax; ret $4`.
//     That is the AND-merged form: the success path is wrapped
//     in positive guards and the failure return is the last statement.
//     Written flat as two `if (!x) return LTNULL;` statements VC6 hosts
//     the merged block at the SECOND test -- i.e. before the
//     `mov 0x14(%eax)` -- which is not what shipped.
char* CLTClient::GetServerConVarValueString(char* pName)
{
	if (pName)
	{
		LTCommandVar* pVar = cc_FindConsoleVar(&m_pClientMgr->m_ServerConsoleState, pName);

		if (pVar)
			return pVar->m_pStringVal;
	}

	return LTNULL;
}


// --- 5.  0x00407320 -------------------------------------------------- //
// WHAT PINS IT
//   * slot 82 = iltclient.h idx 11 = `GetAmbientLight(float &light)`.
//     `ret $4` and `fstps (%ecx)` through the one argument confirm the
//     reference out-parameter; slot 83 (0x00407350) is the setter.
//   * `this` is NEVER read.  The value comes from the file-scope
//     0x004b44a0, so the ambient level is a global, not a member.
//   * `sub $8,%esp` / `movl $0,0x4(%esp)` / `mov %eax,0x0(%esp)` /
//     `fildll 0x0(%esp)` is VC6's uint32 -> float widening in full; the
//     eight bytes of frame are that temporary and nothing else.
//   * `fmuls 0x4a1a58` where the four bytes read 0.0039215689 -- exactly
//     1.0f/255.0f.  A `/ 255.0f` would be `fdivs`.
//   * `xor %eax,%eax` scheduled into the middle of the FPU sequence is
//     `return LT_OK;`.
LTRESULT CLTClient::GetAmbientLight(float& light)
{
	light = g_RenderStruct.m_Unknown148 * (1.0f / 255.0f);
	return LT_OK;
}


// ======================================================================= //
//  6.  0x004064e0 -- the client's ILTLightAnim::FindLightAnim
//
//  WHAT PINS THE CLASS: ltnew2.cpp already anchored this table.  CLTClient's
//  constructor writes vtable 0x004a16b8 to this+0x3a8 and the client
//  manager to this+0x3ac (0x00404c21/0x00404c2b), and iltlightanim.h
//  declares exactly four virtuals.  The four slots are 0x004064e0,
//  0x00406510 (GetNumFrames: `*pOut = 0`, range-check against [0x0c],
//  96-byte stride, read +0x28), 0x00406580 (GetLightAnimInfo) and
//  0x004065f0 (SetLightAnimInfo, already reconstructed in ltnew2.cpp).
//  Ours is slot 0 -- FindLightAnim -- and it is the only one of the four
//  that does not index the array itself.
//
//  The server's twin is src/engine/server/serverlightanim.cpp's
//  ServerLightAnimLT::FindLightAnim (server.dll 0x1002def5) and it is the
//  same five statements; the differences are all build-level:
//    - /O1 spells the failure store `orl $0xffffffff,(%esi)` (4 bytes),
//      /O2 spells it `movl $0xffffffff,(%esi)` (6 bytes);
//    - the server reaches the polymorphic holder as `&m_pServerMgr->m_World`
//      (`lea 0x1a4(%eax),%ecx`), the client as the stored pointer itself
//      (`mov 0x4(%ecx),%ecx`) -- so on the client side the subobject's
//      +0x04 already points at the object whose vptr slot 0 is the lookup.
// ======================================================================= //

// The polymorphic holder the light-anim table hangs off.  Slot 0 is
// nonzero when the level has an animation of that name, with the handle
// written through the second argument.  Names ours.
class CWorldLightAnims
{
public:
	virtual LTBOOL FindLightAnim(const char* pName, HLIGHTANIM* phLightAnim) = 0;
};

class CLTLightAnimClient
{
public:
	LTRESULT FindLightAnim(const char* pName, HLIGHTANIM& hLightAnim);   // 0x004064e0

public:
	void*             m_pVTable;             // 0x00
	CWorldLightAnims* m_pClientMgr;          // 0x04  (written by the CLTClient ctor)
};

// WHAT PINS IT
//   * `mov 0x4(%ecx),%ecx` then `mov (%ecx),%eax` then `call *(%eax)` with
//     %ecx still the object -- a virtual call on slot 0 of the member
//     itself, not on something it points to.
//   * two pushes and `ret $8` on both paths -> (pName, &hLightAnim), and
//     the reference is kept in %esi ACROSS the call because it is used
//     again afterwards: one reference, used twice.
//   * `test %eax,%eax; je fail` with LT_OK falling through and the failure
//     block sunk past the epilogue -> `if (found) return LT_OK;` and the
//     failure as the trailing statements, exactly as the server's does.
//   * 0x3d = 61 = LT_NOTFOUND, and there is NO error report at all -- no
//     dsi_OnReturnError call, no g_DebugLevel test, no "LT_NOTFOUND"
//     string -- unlike every one of its three neighbours.
LTRESULT CLTLightAnimClient::FindLightAnim(const char* pName, HLIGHTANIM& hLightAnim)
{
	if (m_pClientMgr->FindLightAnim(pName, &hLightAnim))
		return LT_OK;

	hLightAnim = (HLIGHTANIM)-1;

	return LT_NOTFOUND;
}


// ======================================================================= //
//  7.  0x00489750 -- ILTTransform::FromMatrix
//
//  WHAT PINS THE CLASS: ltnew2.cpp anchored the nine-slot table at
//  0x004a16c8 as ilttransform.h's, on CLTClient's constructor writing it
//  to this+0x3a4 and on GetPos landing at slot 2 (0x004896b0).  Slot 5 at
//  0x004a16dc is 0x00489750, and ilttransform.h's slot 5 is FromMatrix.
//  Every neighbour agrees: slot 4 (ToMatrix, 0x00489700) is the twin.
// ======================================================================= //

// DEFINED IN src/engine/lithtech/transform.cpp, CLTTransform's own TU (all
// nine slots; its copy is byte-identical too).  It used to be written here
// as well, which made the whole-exe link see two definitions (LNK2005); the
// shipped exe has the one at 0x00489750, so this copy is gone.


// ======================================================================= //
//  8.  0x00491070 -- the two-argument overload of a string-table insert
//
//  HOME: the client's own string/hash table file (0x00491070 and its
//  callee 0x004910a0 are adjacent and the call between them is a direct
//  `call`, so they are one TU).  Called twice, from 0x0040434f and
//  0x00404867, on `this = <object>+0x0c` with the result stored to two
//  fields of the caller's record.
// ======================================================================= //

class CUnknown491070Table
{
public:
	void* Add(const char* pStr, LTBOOL bFlag);                     // 0x00491070
	void* Add(const char* pStr, LTBOOL bFlag, uint32 nLen);        // 0x004910a0
};

// WHAT PINS IT
//   * `ret $8` -> thiscall, two arguments; `mov %ecx,%edx` at the top and
//     `mov %edx,%ecx` before the call is `this` being parked out of the
//     way of `repnz scasb`, which is the /Oi strlen intrinsic and nothing
//     else (`or $0xffffffff,%ecx; xor %eax,%eax; repnz scas; not %ecx;
//     dec %ecx`).
//   * `and $0xffff,%ecx` on the length before it is pushed, while the
//     callee reads that argument as a full dword (`mov 0x24(%esp),%ebp`,
//     `lea 0x1(%esi,%ebp,1)`), so the truncation is a CAST IN THE SOURCE,
//     not a uint16 parameter -- a uint16 parameter would make the callee
//     `movzwl` it.
//   * three pushes, `ret $0xc` at the callee -> the three-argument
//     overload, arguments (pStr, bFlag, len).
//   * the callee's %eax is returned untouched -> `return Add(...)`.
//   * strlen is evaluated BEFORE bFlag is loaded, which is VC6's
//     right-to-left argument evaluation with the cast written inline; a
//     `uint16 nLen = (uint16)strlen(pStr);` statement before the call
//     would load bFlag first.
void* CUnknown491070Table::Add(const char* pStr, LTBOOL bFlag)
{
	return Add(pStr, bFlag, (uint16)strlen(pStr));
}


// ======================================================================= //
//  9.  0x0041cd10 -- a mode setter that snapshots a 256-byte table
//
//  HOME: the client's input/command file.  Three call sites, 0x0041cb2b,
//  0x0041cbe3 and 0x0041cc53, all passing small literals (the first
//  passes 2), and the function immediately after it (0x0041cd40) clears
//  the same +0x0c field.
// ======================================================================= //

class CUnknown41CD10
{
public:
	void SetMode(int nMode);                  // 0x0041cd10
	void ClearMode();                         // 0x0041cd40

public:
	uint8 m_pad_0000[0x0c];
	int   m_nMode;                            // 0x00c
	uint8 m_pad_0010[0x18 - 0x10];
	char  m_Live[0x100];                      // 0x018
	uint8 m_pad_0118[0x148 - 0x118];
	char  m_Saved[0x100];                     // 0x148
};

// WHAT PINS IT
//   * `ret $4` -> thiscall, one argument, kept in %edi across the call
//     because it is used again after it (the store to +0x0c).
//   * `cmp %edi,0xc(%esi)` is Intel `cmp [this+0xc], arg`: operand order
//     survives, so the member is written on the LEFT.
//     `je` skips the whole body -> the source tests `!=`.
//   * three pushes and one `add $0xc,%esp` -> one __cdecl three-argument
//     call.  0x00492b60 is the CRT **strncpy**, NOT memcpy: it carries the
//     0x7efefeff zero-byte magic and, after the source terminator, ZERO-FILLS
//     the remaining count (0x00492bba..0x00492bdf -- an alignment loop storing
//     %al then a dword loop, which memcpy has no reason to emit).  Corrected
//     when rezmgr's GetRezFromDosPath turned out to depend on the difference.
//     Formerly described here as the CRT memcpy (dword loop behind a
//     `test $3` alignment check), and a constant size still emits a CALL
//     in this build -- `push $0x138 ... call memcpy` is
//     exactly that.
//   * the source address (+0x18) is computed FIRST and the destination
//     (+0x148) second, with the pushes in the opposite order: cdecl
//     right-to-left over strncpy(dst, src, n).  (The body said memcpy
//     after this comment was corrected; VC6 inlines a constant-size memcpy
//     as `rep movsl`, which is what kept this function 37 bytes to 45.)
//   * the `mov %edi,0xc(%esi)` sits AFTER the call -> the assignment is
//     the second statement inside the guard, not before the copy.
void CUnknown41CD10::SetMode(int nMode)
{
	if (m_nMode != nMode)
	{
		strncpy(m_Saved, m_Live, sizeof(m_Saved));
		m_nMode = nMode;
	}
}

// 0x0041cd40 -- leave the mode (ccline.cpp's CcnConsole::ClearMode, called
// by the key handler's default arm and the completion/history operations).
// `mov 0xc(%ecx),%eax; test; je` then the zero store: the store is guarded,
// so the source tests before clearing rather than storing unconditionally.
void CUnknown41CD10::ClearMode()
{
	if (m_nMode)
		m_nMode = 0;
}


// ======================================================================= //
//  10.  0x00445250 -- free every node on a singly-linked list
//
//  HOME: the object/world file around 0x00445000.  One caller, 0x00446911.
// ======================================================================= //

class CUnknown445250Node;

// The owner the nodes go back to.  Slot 2 (0x8) takes one pointer.
class CUnknown445250Owner
{
public:
	virtual void Slot0() = 0;
	virtual void Slot1() = 0;
	virtual void Free(CUnknown445250Node* pNode) = 0;             // 0x08
};

class CUnknown445250Node
{
public:
	void*                m_pUnknown00;        // 0x00
	CUnknown445250Node*  m_pNext;             // 0x04
};

class CUnknown445250List
{
public:
	void FreeAll();                           // 0x00445250

public:
	CUnknown445250Node*  m_pHead;             // 0x00
	CUnknown445250Owner* m_pOwner;            // 0x04
};

// WHAT PINS IT
//   * bare `ret`, `this` in %edi -> thiscall, no arguments, void.
//   * the head test at the top and the `jne` back edge at the bottom are
//     a rotated `while (pCur)`; the next pointer is +0x04 and it is read
//     BEFORE the call, so the walk saves the successor first.
//   * `test %eax,%eax; je` INSIDE the loop is provably never taken (the
//     back edge has just proved %esi non-zero and copied it into %eax).
//     VC6 keeps unsatisfiable guards rather than deleting them,
//     so this redundant test is a real `if` in the source --
//     it is not something the optimiser invented.
//   * `mov 0x4(%edi),%ecx` is reloaded every iteration: the owner is
//     addressed through the member each time, not hoisted into a local.
//   * TWO stores of null to the head, one at each exit
//     (`mov %esi,(%edi)` where %esi is the zero that ended the loop, and a
//     separate `movl $0,(%edi)` for the never-entered case).  That is the
//     tail duplication a single trailing `m_pHead = LTNULL;` gets, not two
//     statements.
//
// COIN-FLIP (recorded, not settled): the inner guard could equally be an
// inline helper -- `void Free(T* p) { if (p) m_pOwner->Free(p); }` called
// unconditionally -- which lowers to the identical bytes.  The spelling
// below is the shorter of the two.
void CUnknown445250List::FreeAll()
{
	CUnknown445250Node* pCur = m_pHead;

	while (pCur)
	{
		CUnknown445250Node* pNext = pCur->m_pNext;

		if (pCur)
			m_pOwner->Free(pCur);

		pCur = pNext;
	}

	m_pHead = LTNULL;
}


// ======================================================================= //
//  11.  0x0048e630 -- a null-guarded lookup that forwards to a sub-table
//
//  *** THIS ONE IS SHARED WITH server.dll. ***  The same forty-two bytes
//  ship at server.dll 0x1003ac50 (byte-identical apart from the two call
//  relocations), so its home is one of the src/engine/server files that
//  lithtech.exe compiles a second time, NOT src/engine/lithtech.  Neither
//  copy is reconstructed yet -- src/engine/server/NAMEMAP.txt line 2038
//  carries 0x1003ac50 with an empty name -- so this is new code on BOTH
//  images and both need re-measuring.  Two callers in the
//  exe (0x0048f5ff, 0x0048fc50).
// ======================================================================= //

// DEFINED IN src/lithshared/rezmgr/rezdir.cpp (RezMgr.lib, which both
// images link): it is CRezDir::GetDir, its sub-table the by-name hash
// table's Find at 0x00490050, and the library's copy is the one placed at
// 0x0048e630 (and server.dll 0x1003ac50).  The copy that stood here
// duplicated it and is gone.


// ======================================================================= //
//  12.  0x00401820 -- REMOVED: it is ~BinkVideoMgrK in binkvideo.cpp
//  (identical there); the copy that stood here duplicated it.
// ======================================================================= //


// ======================================================================= //
//  13.  0x004789a0 -- a constructor over a three-element array
//
//  HOME: the packet file: the single caller is 0x0047862c, inside the
//  function whose error literal is "CPacket::ReadType", constructing a
//  0x28-byte object in the caller's frame at `lea 0x24(%esp),%ecx`.
// ======================================================================= //

struct CUnknown4789A0Entry
{
	CUnknown4789A0Entry()
	{
		m_nA = 0;
		m_nB = 0;
		m_nC = 0;
	}

	uint32 m_nA;                              // 0x00
	uint32 m_nB;                              // 0x04
	uint16 m_nC;                              // 0x08  (pads the record to 12)
};

class CUnknown4789A0
{
public:
	CUnknown4789A0();                         // 0x004789a0

public:
	CUnknown4789A0Entry m_First;              // 0x00
	CUnknown4789A0Entry m_Rest[2];            // 0x0c, 0x18
	uint32              m_nCount;             // 0x24
};

// WHAT PINS IT
//   * `mov %ecx,%eax` first and `mov %esi,%eax`-less epilogue returning
//     %eax = `this`, with a bare `ret`: a default constructor.
//   * the per-element stores are dword, dword, WORD at +0, +4, +8 and the
//     stride is `add $0xc,%edx` -- a twelve-byte record whose last field
//     is sixteen bits wide, i.e. {uint32, uint32, uint16} with two bytes
//     of tail padding.  A uint32 third field would make the third store
//     `mov %ecx,0x8(%edx)`.
//   * `mov $2,%esi` with the FIRST element done inline off %eax and the
//     loop walking a fresh %edx from `this+0xc`: one single member
//     constructed inline, then a two-element array's vector-construction
//     loop (NOT a rotated three-element array -- see SETTLED below).
//   * `mov %ecx,0x24(%eax)` LAST, after the loop -> the scalar is a
//     member of the enclosing class assigned in the constructor BODY,
//     which always follows the member constructors.
//
// SETTLED: a first element plus a two-element array.  With a single
// `m_Entries[3]` VC6 does NOT peel -- it runs one three-trip loop off
// `lea 0x8(%eax)` in %edi (35 bytes, near); `m_First; m_Rest[2]` gives the
// shipped inline first construction and the literal two-trip loop, byte-
// identical.
CUnknown4789A0::CUnknown4789A0()
{
	m_nCount = 0;
}


// ======================================================================= //
//  14.  0x0048a5f0 -- a constructor over two dispatch sinks
//
//  HOME: next to 0x0048a640 (which walks the same +0x00/+0x04 pair).  Two
//  callers; 0x00402215 is `operator new(0x2c)` -> `mov %eax,%ecx` ->
//  `call`, so the object is 0x2c bytes.
// ======================================================================= //

// 0x004af4d8 and 0x004af4e0 are eight-byte .data records, {function,
// pointer}: {0x0048a590, 0x004ab398} and {0x0048a5c0, 0x004ab398}.  The
// two functions are the same nine instructions apart from the vtable slot
// they end on (0x20 vs 0x24), which is what makes the pair a dispatch
// table rather than a vtable (a vtable would live in .rdata).
struct CUnknown48A5F0Dispatch
{
	void* m_pFn;
	void* m_pInfo;
};

// 0x004af4d8 / 0x004af4e0 are binkvideo.cpp's k_PolyHookDescK /
// k_TextureHookDescK (the video hooks' static descriptors), so these are
// those two, reached through this file's view of them.
struct CVideoHookDescK;
extern CVideoHookDescK k_PolyHookDescK;      // 0x004af4d8
extern CVideoHookDescK k_TextureHookDescK;   // 0x004af4e0
#define g_Dispatch_004af4d8 (*(CUnknown48A5F0Dispatch*)&k_PolyHookDescK)
#define g_Dispatch_004af4e0 (*(CUnknown48A5F0Dispatch*)&k_TextureHookDescK)

class CUnknown48A5F0Owner;

struct CUnknown48A5F0Sink
{
	CUnknown48A5F0Sink(CUnknown48A5F0Dispatch* pDispatch, CUnknown48A5F0Owner* pOwner)
	{
		m_pNext     = LTNULL;                 // 0x00
		m_pDispatch = pDispatch;              // 0x04
		m_pOwner    = pOwner;                 // 0x08
		m_pUser     = LTNULL;                 // 0x0c
	}

	CUnknown48A5F0Sink*     m_pNext;
	CUnknown48A5F0Dispatch* m_pDispatch;
	CUnknown48A5F0Owner*    m_pOwner;
	void*                   m_pUser;
};

class CUnknown48A5F0
{
public:
	CUnknown48A5F0(CUnknown48A5F0Owner* pOwner);                  // 0x0048a5f0

public:
	CUnknown48A5F0Sink m_SinkA;               // 0x00
	CUnknown48A5F0Sink m_SinkB;               // 0x10
	uint8              m_pad_0020[0x2c - 0x20];
};

// WHAT PINS IT
//   * `ret $4` returning `this` in %eax -> a one-argument constructor.
//   * TWO identical four-field groups, sixteen bytes apart, whose only
//     difference is the constant at +0x04 (0x4af4d8 vs 0x4af4e0): two
//     sub-objects of one type, initialised with different table pointers
//     and the SAME argument (`mov 0x4(%esp),%ecx` is loaded once and
//     stored twice).
//   * `c7 40 04 <addr>` is the ADDRESS of the record being stored, not a
//     load from it (`a1`/`8b` would be a load) -> `&g_Dispatch_...`.
//   * the constructor writes NOTHING to +0x20..+0x2b, so the remaining
//     twelve bytes of the 0x2c-byte allocation are left alone.
//   * the two zero stores to +0x00 and +0x10 are emitted LAST, grouped,
//     out of field order.  That is the zero-store grouping class
//     -- size-neutral and unaffected by source order -- so the
//     declaration order below is the honest one.
CUnknown48A5F0::CUnknown48A5F0(CUnknown48A5F0Owner* pOwner)
	: m_SinkA(&g_Dispatch_004af4d8, pOwner),
	  m_SinkB(&g_Dispatch_004af4e0, pOwner)
{
}


// ======================================================================= //
//  15/16.  0x0048cce0 and 0x0048cd50 -- a constructor and its Term
//
//  HOME: one TU with 0x0048c780/0x0048c930 (both reached by direct calls)
//  and with 0x0048ccd0, which is slot 0 of the seven-slot vtable at
//  0x004a38f8 that the constructor installs.  The constructor's caller is
//  0x00422393.
//
//  WHY THE LAYOUT IS WHAT IT IS.  The vptr store at +0x00 comes AFTER the
//  thiscall on this+0x0c in the constructor.  VC6 emits base constructors,
//  THEN the vptr, THEN member constructors, THEN the body -- so the
//  sub-object at +0x0c can only be a BASE, and since the class introduces
//  its own virtuals the vptr takes offset 0 and pushes the bases to +0x04
//  and +0x0c.  A member at +0x0c would put the vptr store FIRST, which is
//  not what shipped.
// ======================================================================= //

// The first base: eight bytes, no constructor of its own (nothing is
// called for it), one pointer the derived constructor clears.
class CUnknown48CCE0BaseA
{
public:
	void* m_pUnknown04;                       // 0x04 in the derived object
	void* m_pUnknown08;                       // 0x08
};

// The second base: 0x5c bytes, a default constructor at 0x0048c780 (it
// returns `this`, so it is a constructor and not an Init) and a Term at
// 0x0048c930.
class CUnknown48CCE0BaseB
{
public:
	CUnknown48CCE0BaseB();                    // 0x0048c780
	void Term();                              // 0x0048c930

	uint8 m_Storage[0x68 - 0x0c];
};

class CUnknown48CCE0 : public CUnknown48CCE0BaseA, public CUnknown48CCE0BaseB
{
public:
	CUnknown48CCE0();                         // 0x0048cce0
	void Term();                              // 0x0048cd50

	virtual void Slot0() = 0;                 // 0x004a38f8 has seven slots

public:
	void*       m_pUnknown68;                 // 0x68
	void*       m_pUnknown6c;                 // 0x6c
	void*       m_pUnknown70;                 // 0x70
	CheapLTLink m_Link;                       // 0x74
};

// WHAT PINS THE CONSTRUCTOR (0x0048cce0)
//   * `mov %esi,%eax` in the epilogue with a bare `ret`: a default
//     constructor returning `this`.
//   * `lea 0xc(%esi),%ecx; call 0x0048c780` BEFORE `movl $0x4a38f8,(%esi)`
//     -- the base/vptr ordering argued above.
//   * one `xor %eax,%eax` feeding four stores (+0x04, +0x68, +0x6c,
//     +0x70) in that order: four assignments in the constructor body,
//     after the vptr, in declaration order.
//   * `lea 0x74(%esi),%eax; mov %eax,0x4(%eax); mov %eax,(%eax)` is
//     ltlink.h's CheapLTLink::TieOff inlined -- m_pNext (+0x04) written
//     before m_pPrev (+0x00) because the header spells it
//     `m_pPrev = m_pNext = this`, a chained assignment.  clientrender.cpp
//     and clienttexmgr.cpp both record that exact order.
CUnknown48CCE0::CUnknown48CCE0()
{
	m_pUnknown04 = LTNULL;
	m_pUnknown68 = LTNULL;
	m_pUnknown6c = LTNULL;
	m_pUnknown70 = LTNULL;

	m_Link.TieOff();
}

// WHAT PINS THE TERM (0x0048cd50)
//   * NO vptr store anywhere, and the call on this+0x0c comes FIRST.  A
//     destructor would do the opposite on both counts (vptr first, base
//     destructors last), so this is an ordinary void member function whose
//     first statement is the base's own Term.
//   * `xor %eax,%eax` feeding exactly two stores, +0x68 and +0x6c -- and
//     NOT +0x70, which the constructor does clear.  A missing store is a
//     source fact, so Term really does leave +0x70 alone.
//   * the eleven-instruction tail is ltlink.h's CheapLTLink::Remove in
//     full: `m_pPrev->m_pNext = m_pNext`, `m_pNext->m_pPrev = m_pPrev`,
//     then TieOff -- Remove() already ties off, so there is no second
//     call.  m_pPrev is at +0x74 and m_pNext at +0x78, which is
//     ltlink.h's declaration order (`LTLink *m_pPrev, *m_pNext;`).
void CUnknown48CCE0::Term()
{
	CUnknown48CCE0BaseB::Term();

	m_pUnknown68 = LTNULL;
	m_pUnknown6c = LTNULL;

	m_Link.Remove();
}


// ======================================================================= //
//  17.  0x0041d040, 384 bytes -- the console's text rectangle
//
//  HOME: the console TU (src/engine/lithtech/ccline.cpp ends at 0x0041d033
//  and declares this as CcnConsole::GetTextRect; written here because that
//  file is not this run's).  The enumeration split it in two: a 45-byte
//  row at 0x0041d040 (the fast path) and a 328-byte gap row at 0x0041d06d,
//  which is only the target of the `jne` at 0x0041d048.
//
//  WHAT PINS IT
//   * thiscall, `ret $4` at both exits, one LTRect* out-parameter.
//   * `if (!m_pScreen)` copies the four dwords at +0x26c through `add
//     $0x26c,%ecx` -- a struct assignment of the requested rectangle.
//   * each requested edge: -1 means the default (0 for left/top, the screen
//     extent for right/bottom), a negative value is a fraction of the
//     screen (`idiv; neg` for left/top, `idiv; add` for right/bottom), and
//     anything else is taken as is.  Order: top, left, right, bottom.
//   * then six clamps, stored in the order right, bottom, right, bottom,
//     left, top: right/bottom to the screen, right to at least 160, bottom
//     to at least four lines of the uint16 at +0x2a2, left and top to leave
//     that much room.
// ======================================================================= //

struct CUnknown41D040Screen
{
	uint8 m_pad_00[0x3c];
	int   m_nWidth;                           // 0x3c
	int   m_nHeight;                          // 0x40

	// In-class accessors: the two screen clamps read the extent through
	// them.  Spelled as bare member reads, VC6 loads the rect edge into the
	// result register and flips the compare (`cmp %edx,%eax; jl`) -- a
	// 2-block near; every ternary/if spelling of the min was tried.
	int GetWidth()  { return m_nWidth; }
	int GetHeight() { return m_nHeight; }
};

class CUnknown41D040
{
public:
	void GetTextRect(LTRect* pRect);          // 0x0041d040

	uint8                 m_pad_000[0x268];
	CUnknown41D040Screen* m_pScreen;          // 0x268
	LTRect                m_rcConsole;        // 0x26c
	uint8                 m_pad_27c[0x2a2 - 0x27c];
	uint16                m_nLineHeight;      // 0x2a2
};

void CUnknown41D040::GetTextRect(LTRect* pRect)
{
	if (!m_pScreen)
	{
		*pRect = m_rcConsole;
		return;
	}

	if (m_rcConsole.top == -1)
		pRect->top = 0;
	else if (m_rcConsole.top < 0)
		pRect->top = -(m_pScreen->m_nHeight / m_rcConsole.top);
	else
		pRect->top = m_rcConsole.top;

	if (m_rcConsole.left == -1)
		pRect->left = 0;
	else if (m_rcConsole.left < 0)
		pRect->left = -(m_pScreen->m_nWidth / m_rcConsole.left);
	else
		pRect->left = m_rcConsole.left;

	if (m_rcConsole.right == -1)
		pRect->right = m_pScreen->m_nWidth;
	else if (m_rcConsole.right < 0)
		pRect->right = m_pScreen->m_nWidth + m_pScreen->m_nWidth / m_rcConsole.right;
	else
		pRect->right = m_rcConsole.right;

	if (m_rcConsole.bottom == -1)
		pRect->bottom = m_pScreen->m_nHeight;
	else if (m_rcConsole.bottom < 0)
		pRect->bottom = m_pScreen->m_nHeight + m_pScreen->m_nHeight / m_rcConsole.bottom;
	else
		pRect->bottom = m_rcConsole.bottom;

	pRect->right  = LTMIN(pRect->right, m_pScreen->GetWidth());
	pRect->bottom = LTMIN(pRect->bottom, m_pScreen->GetHeight());
	pRect->right  = LTMAX(pRect->right, 160);
	pRect->bottom = LTMAX(pRect->bottom, m_nLineHeight * 4);
	pRect->left   = LTMIN(pRect->left, pRect->right - 160);
	pRect->top    = LTMIN(pRect->top, pRect->bottom - m_nLineHeight * 4);
}


// *********************************************************************** //
//
//  PER-ADDRESS REPORT
//  ==================
//
//  DONE (source above)
//  -------------------
//  0x00407700  46  CLTClient::SetModelAnimation      vtable 0x004a14e0 slot 32
//  0x004077e0  46  CLTClient::SetModelLooping        slot 34
//  0x00407850  46  CLTClient::SetModelPlaying        slot 105
//  0x00406c60  44  CLTClient::GetServerConVarValueString  slot 77
//  0x00407320  44  CLTClient::GetAmbientLight        slot 82
//  0x004064e0  43  CLTLightAnimClient::FindLightAnim vtable 0x004a16b8 slot 0
//  0x00489750  44  CLTTransform::FromMatrix          vtable 0x004a16c8 slot 5
//  0x00491070  45  two-arg string-table insert overload
//  0x0041cd10  45  mode setter + 256-byte memcpy snapshot
//  0x00445250  45  list free-all walk
//  0x0048e630  42  null-guarded sub-table lookup    *** SHARED WITH server.dll ***
//  0x00401820  46  REMOVED -- ~BinkVideoMgrK, binkvideo.cpp (identical)
//  0x004789a0  42  first + two-element array constructor (identical)
//  0x0041d040 384  GetTextRect (block 17, identical; row 0x0041d06d is its interior)
//  0x0048a5f0  42  two-sink constructor
//  0x0048cce0  43  constructor (vtable 0x004a38f8)
//  0x0048cd50  45  the matching Term
//
//  ALREADY ACCOUNTED FOR -- reconstructed in src/engine/server, which
//  lithtech.exe compiles a SECOND time.  `grep 0xADDR src/engine/` misses
//  them because those files are annotated with server.dll addresses.
//  ---------------------------------------------------------------------
//  0x0046f110  44  CLTServer::StartMessage(HCLIENT, uint8)
//                  = src/engine/server/servermessage.cpp:763 (server.dll
//                  0x1002f2xx).  CLTServer's vtable is 0x004a2ef4 (190
//                  slots; slot 0 = 0x0046f290, slots 1..30 and 56..70 are
//                  the same ILTCSBase bodies as the client's), and
//                  0x004a3050 is slot 87 = iltserver.h idx 16 =
//                  StartMessage.  The bytes match the existing source
//                  statement for statement: `add $8,%eax` is
//                  `&StartMessage()->m_Write`, `movl $1,0xc` is
//                  m_Type = MT_TOCLIENT, `mov %ecx,0x10` is m_hClient,
//                  `mov %edx,0x1c` is m_MessageID with the uint8
//                  zero-extended.
//  0x004722f0  42  si_GetObjectState
//                  = src/engine/server/smgr.cpp:3967.  No caller at all;
//                  it is reached as a function-pointer store,
//                  `movl $0x004722f0,0x1b0(%eax)` at 0x004704c7 inside
//                  si_SetupInterface, and smgr.cpp:3220 puts
//                  GetObjectState at flat-interface offset 0x1b0.  Worth
//                  a note: server.dll's /O1 copy is
//                  the known 18-of-19 `movzbl`-elision near, but the
//                  /O2 copy here spells the byte narrowing `and $0xff` --
//                  which is what our `(uint8)nFlags` already emits -- so
//                  the exe's twin may go exact where the DLL's does not.
//
//  NOT-A-FUNCTION -- mid-function jump targets
//  -------------------------------------------
//  0x00401679  46  the tail of the function at 0x00401650 (41+46 = 87
//                  bytes, 0x00401650..0x004016b0).  It opens `push %edi`
//                  and closes `pop %esi`, and %esi is loaded at 0x00401650;
//                  it is the loop that scans 20-byte records for a zero
//                  byte and returns the record address.
//  0x0046d743  45  the tail of the function at 0x0046d700 (67+45 = 112
//                  bytes, 0x0046d700..0x0046d770).  It uses %eax before
//                  writing it.  The enclosing function is CLTServer's
//                  vtable slot 74 (0x004a301c) = iltserver.h idx 3 =
//                  GetClassName(HCLASS, char*, uint32): the
//                  LT_INVALIDPARAMS report is the head, and this is
//                  `if (nBufSize) { memcpy(pBuf, ..., nBufSize-1);
//                  pBuf[nBufSize-1] = 0; } return LT_OK;`.  It is
//                  therefore SHARED server code (server.dll 0x1002e1e8 is
//                  the same tail).
//
//  MIS-SIZED -- the row's start is right, the size is the distance to a
//  branch target INSIDE the body.  All three are real functions an order
//  of magnitude larger, out of scope for a file of 42..46-byte functions.
//  ---------------------------------------------------------------------
//  0x00458e50  44 stated, 1622 actual (0x00458e50..0x004594b0).  It opens
//                 `sub $0x50,%esp` + four pushes and its first `je` goes
//                 to 0x00458e7c, which the enumeration then lists as a
//                 separate 1578-byte row named CPacket::ReadType.  One
//                 function, split in two.
//  0x00464840  45 stated, 448 actual (0x00464840..0x00464a00).  Same
//                 pattern: the `je` at 0x0046484c targets 0x0046486d,
//                 enumerated as a 403-byte row named CPacket::WriteType.
//  0x0041d040  45 stated, 384 actual (0x0041d040..0x0041d1c0).  The `jne`
//                 at 0x0041d048 targets 0x0041d06d, enumerated as a
//                 328-byte gap row.  The 45 bytes are only the
//                 `if (!m_Something268) { *pOut = the four dwords at
//                 +0x26c; return; }` fast path.
//
//  COMPILER-GENERATED -- scalar deleting destructors.  Both are the
//  MSVC `X::'scalar deleting destructor'(unsigned int flags)` thunk:
//  run the destructor, `if (flags & 1) operator delete(this)`, return
//  `this`.  There is no source text that produces one on its own; it
//  falls out of a class declaration plus a `delete` on a virtual
//  destructor, so neither belongs in a scratch TU.
//  ---------------------------------------------------------------------
//  0x0046f970  42  CLTServer's.  It is slot 71 of the CLTServer vtable
//                  (0x004a3010) and it restores TWO vptrs: 0x004a31ec at
//                  +0x00 and 0x004a1a0c at +0x234.  Both of those are
//                  ABSTRACT-BASE vftables, not implementation ones --
//                  0x004a31ec's slot 0 is the shared `_purecall` stub
//                  0x004928e7 while its slots 1..30 are the real
//                  ILTCSBase bodies, i.e. it is `ILTServer`'s own table,
//                  written by the inlined `~ILTServer(){}`; 0x004a1a0c is
//                  slot 0 = 0x0046f9a0 (the second base's own deleting
//                  destructor) followed by purecall stubs.  That settles
//                  the fourth ILTCSBase-derived table:
//                  0x004a31ec is ILTServer's abstract vftable, and by the
//                  same argument 0x004a1810 (slot 0 = 0x004928e7, slot 1
//                  = 0x0041ec90) is ILTClient's.  Only two of the four are
//                  implementations: 0x004a14e0 and 0x004a2ef4.
//                  It also says CLTServer has a SECOND base at +0x234 in
//                  the exe that server.dll's copy (0x1003d310, one vptr)
//                  does not -- worth a look when the exe's CLTServer is
//                  taken up properly.
//  0x00488230  44  an unnamed class's.  No vptr restore at all, so the
//                  inlined destructor body is just
//                  `if (m_p98) operator delete(m_p98);`, then the
//                  `testb $1,0x8(%esp)` / `operator delete(this)` /
//                  `return this` boilerplate.  Six direct call sites
//                  (0x00485c78, 0x004873f1, 0x004875b4, 0x00487705,
//                  0x00487af7, 0x00487f3f).
//
//  OPEN COIN-FLIPS, with the exact alternative spelling
//  ---------------------------------------------------
//  * 0x00445250: `if (pCur) m_pOwner->Free(pCur);` versus an inline
//    helper `void Free(T* p) { if (p) m_pOwner->Free(p); }` called
//    unconditionally.  Identical bytes; the first is written.
//  * 0x004789a0: SETTLED -- `m_First; m_Rest[2]` is identical, `m_Entries[3]`
//    is not.
//  * 0x0048cce0 / 0x0048cd50: the +0x0c sub-object is modelled as a BASE
//    because the vptr store follows its constructor call.  If VC6 turns
//    out to schedule that store across the call, the alternative is
//    `CUnknown48CCE0BaseB m_Sub0c;` as a plain member with the two bases
//    dropped and the padding moved into m_pad_0004[8].
//  * 0x0048a5f0: the two sinks are given a two-argument constructor; the
//    alternative is a default-constructed pair plus four body
//    assignments, which would emit the zero stores in field order rather
//    than grouped at the end.  Size-neutral either way.
//
// *********************************************************************** //
