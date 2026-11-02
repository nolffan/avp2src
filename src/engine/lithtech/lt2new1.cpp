// *********************************************************************** //
//
//  MODULE:   LT2NEW1 (.CPP)   — staging translation unit (name ours)
//
//  PURPOSE:  Twenty-five enumerated rows out of lithtech.exe, read off
//            the retail lithtech.exe ONLY.  They are not one translation
//            unit; the per-block banners say where each one really belongs.
//            Everything compiles at /O2 (cl 8447), which is what
//            src/engine/lithtech gets.
//
//  WHAT NAMED THE BATCH
//  --------------------
//  Six rows carry a check literal, and two of those anchored a whole
//  interface:
//
//   * "CLTClient::GetSConValueFloat" (0x004a7aac) and
//     "CLTClient::GetSConValueString" (0x004a7acc) sit at slots 74 and 75
//     of the ILTClient vtable 0x004a14e0.  ltnew1.cpp's rule — slot =
//     iltclient index + 71, with slot 71 the destructor — puts them at
//     iltclient.h indices 3 and 4, and iltclient.h declares exactly
//     GetSConValueFloat then GetSConValueString there.  That in turn dated
//     slot 85 = index 14 = StopVideo, which is the third row in block A and
//     had no literal of its own.
//
//   * "CPhysicsLT::UpdateMovement" (0x004a7828) is at 0x004a17bc.  Read the
//     vtable BACKWARDS from it against iltphysics.h and every slot falls
//     into place at base 0x004a1770: the eleven NON-pure ILTPhysics members
//     all hold a shared 0x00434xxx/0x00435xxx body and the eleven PURE ones
//     all hold a client 0x00404xxx/0x00405xxx body, with no exceptions and
//     no leftovers.  That is what identifies 0x00405160 — an anonymous
//     twelve-byte copy-out — as GetGlobalForce (index 15) rather than the
//     SetGlobalForce next door, and 0x00404f30 as UpdateMovement (index 19,
//     the first ILTClientPhysics slot).  Cross-checked: 0x00405190,
//     SetGlobalForce, writes the same 0x4d8 vector this one reads.
//
//   * "CPacket::ReadType" (0x004a864c) is NOT a function of its own in any
//     of its three rows.  packet.h's ReadType is a member template defined
//     in the class body, so it is INLINED into its callers, and the three
//     rows are those callers.  Each one carries the whole check —
//     `or $0x2,m_Flags` (PACKETFLAG_READOVERFLOW), `push $1` into
//     dsi_OnReturnError, level 1, then the printer with "LT_ERROR"
//     (0x004a7360) and "ERROR: Packet read overrun!" (0x004a8660) — which
//     is REPORT_ERROR_INFO(1, ...) exactly as src/engine/server/packet.h
//     already writes it.  Nothing in those three had to be re-derived.
//
//  FOUR ROWS ARE INTERIOR BLOCKS, NOT FUNCTIONS, and one more row is two
//  functions.  Every one of them is written out here as its real containing
//  function, with the row address noted; see the table at the foot.
//
//  A NOTE ON WHAT IS *NOT* HERE.  0x004a7160 is "60" and 0x004a7164 is " "
//  (verified in .data), so every `push $0x3c` site below is the ordinary
//  RETURN_ERROR of de_error.h and not one of its symbol-reporting cousins;
//  where the description argument is a pointer the caller supplies instead,
//  the macro is RETURN_ERROR_INFO and the bytes say so by pushing a
//  register.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "ltlink.h"
#include "iltvideomgr.h"

#include "de_error.h"
#include "packet.h"
#include "collection.h"


// ltobject_internal.h's, repeated here so this TU does not have to pull the
// whole object header in. The shipped test is `test $0x40,%cl`.
#ifndef CHANGEFLAG_MOTION
#define CHANGEFLAG_MOTION 0x40
#endif

// ltserverobj.h's MID_CRUSH. Spelled as a local constant for the same
// reason; the immediate is not a masked relocation, so the VALUE is
// load-bearing and 6 is what the shipped push carries.
#ifndef MID_CRUSH
#define MID_CRUSH 6
#endif


// ======================================================================= //
//  A.  CLTClient — three slots of the ILTClient vtable at 0x004a14e0
//
//  BELONGS IN: the same client TU as ltnew1.cpp's block A.  All three lie
//  inside 0x004058xx..0x00407c2x, the contiguous run of ILTClient slot
//  bodies, and none of them is anything src/engine/lithtech already
//  compiles.
//
//  As in ltnew1.cpp the class is redeclared here on a VIEW and the members
//  are NOT declared virtual: a virtual and a non-virtual member compile to
//  the same bytes, and declaring them virtual would make VC6 demand the
//  other hundred-odd bodies for a vtable we do not emit.
// ======================================================================= //

// The console state CClientMgr keeps for the SERVER's variables.  Its
// address is CClientMgr+0x1268, which clientmgr2.cpp already records as
// "m_Sub1268 — passed by address"; clientdsi.cpp already models the state
// itself as an opaque 0x4c bytes.
class ConsoleStateView
{
public:
	uint8 m_pad_00[0x4c];
};

// One console variable.  servermgr.h's LTCommandVar has m_pStringVal at
// 0x14 and m_FloatVal at 0x18, which is exactly the pair the two readers
// below take (`mov 0x14(%eax),%edx` for the string, `mov 0x18(%eax),%edx`
// for the float).
class LTCommandVarView
{
public:
	uint8  m_pad_00[0x14];
	char*  m_pStringVal;                     // 0x14
	float  m_FloatVal;                       // 0x18
};

// 0x0041b3e0 — strlen(pName), a hash lookup in pState+0x3c, then 0x0042d080
// on the element.  servermgr.h declares the server's twin as
// cc_FindConsoleVar(ConsoleState*, char*); clientdsi.cpp already keeps the
// client's under this address.  cdecl, state first: the shipped call pushes
// pName and then the state.
LTCommandVarView* cc_FindConsoleVar(ConsoleStateView* pState, char* pName);

// CClientMgr, as far as this block reaches it.
class CClientMgrConsoleView
{
public:
	uint8            m_pad_00[0x1268];
	ConsoleStateView m_ServerConsoleState;   // 0x1268
};

class CLTClientConsoleView
{
public:
	uint8 m_pad_00[0x360];
	// 0x360 — the concrete ILTVideoMgr (ltnew1.cpp block A established it
	// off StartVideo, which calls its slot 0 with three arguments).
	ILTVideoMgr* m_pVideoMgr;
	uint8 m_pad_364[0x3c0 - 0x364];
	// 0x3c0 — the client manager.  Twenty-eight distinct ILTClient slot
	// bodies in 0x00405000..0x00407900 open with `mov 0x3c0(%ecx),%e??`,
	// and the two readers below then add 0x1268 to it, which is the offset
	// clientmgr2.cpp already has as a sub-object passed by address.
	CClientMgrConsoleView* m_pClientMgr;
	// 0x3c4 — the one video the client has playing (ltnew1.cpp block A).
	HVIDEO m_hVideo;

	LTRESULT GetSConValueFloat(char* pName, float& val);              // 0x00406ab0
	LTRESULT GetSConValueString(char* pName, char* valBuf, uint32 bufLen); // 0x00406b60
	LTRESULT StopVideo();                                             // 0x004074a0
};


// 0x00406ab0, 172 bytes — slot 74 of 0x004a14e0, iltclient.h index 3.
//
// WHAT PINS IT.
//  * the literal at 0x004a7aac, and `cmp $0x2,0x004b0ab0` at both sites, so
//    both reports are at debug level 2.
//  * the FIRST report falls through and the body is jumped to (`test %esi,
//    %esi; jne 0x406af5`), which is the "error falls through" pattern —
//    i.e. `if (!pName) RETURN_ERROR(...)`, a negated guard, not a trailing
//    else.  The SECOND report sits after the success `ret`, which is the
//    ordinary `if (!pVar) RETURN_ERROR(...)` mid-body form.
//  * the second report pushes %esi — the caller's own pName — as the
//    printer's description, where the first pushes 0x004a7164 (" ").  A
//    register in that slot is RETURN_ERROR_INFO; an immediate is
//    RETURN_ERROR.  Both are in de_error.h already.
//  * `movl $0x0,(%edi)` between the two argument pushes: the out parameter
//    is cleared with an INTEGER immediate, so the source assigned the
//    literal 0.0f rather than routing a value through the FPU
//    ("a float clamp: flds a constant means one assignment, movl means
//    three"), and it did so BEFORE the lookup call.
//  * the second parameter is written through with no null test of its own —
//    iltclient.h declares it `float &val`, and a reference is not checked.
//  * `mov 0x18(%eax),%edx; mov %edx,(%edi)` — the float is moved as a dword,
//    which is how VC6 copies float to float when no arithmetic intervenes.
LTRESULT CLTClientConsoleView::GetSConValueFloat(char* pName, float& val)
{
	LTCommandVarView* pVar;

	if (!pName)
		RETURN_ERROR(2, "CLTClient::GetSConValueFloat", LT_INVALIDPARAMS);

	val = 0.0f;

	pVar = cc_FindConsoleVar(&m_pClientMgr->m_ServerConsoleState, pName);

	if (pVar)
	{
		val = pVar->m_FloatVal;

		return LT_OK;
	}

	RETURN_ERROR_INFO(2, "CLTClient::GetSConValueFloat", LT_NOTFOUND, pName);
}


// 0x00406b60, 198 bytes — slot 75 of 0x004a14e0, iltclient.h index 4.
//
// WHAT PINS IT, and it is NOT the same shape as its neighbour.
//  * both parameter checks jump FORWARD to one report block that sits at
//    the very END of the function, past the LT_NOTFOUND block.
//    "A shared exit block at the very end means the failures were
//    AND-merged" — one positive guard wrapping the body, with the refusal
//    as the function's last statement.  Written as two separate
//    `if (!x) RETURN_ERROR` guards the block hosts at the last test instead.
//  * `test %edi,%edi; jbe` on the length, not `je`.  On an
//    unsigned, `> 0` gives `jbe` and `!= 0` gives `je`.  So the source says
//    `bufLen > 0`.
//  * `lea -0x1(%edi),%ecx` for the strncpy count while %edi itself survives
//    for `movb $0x0,-0x1(%esi,%edi,1)`: the `lea` case — the source
//    wrote `bufLen - 1` at each use rather than keeping a length local.
//  * `movb $0x0,(%esi)` before the lookup, then a three-argument cdecl call
//    to 0x00492b60 (CRT region, above 0x004928bf) with (valBuf, string,
//    bufLen-1): strncpy, and the terminator written by hand after it.
//  * the LT_NOTFOUND site pushes %ebx (pName) as the description again.
LTRESULT CLTClientConsoleView::GetSConValueString(char* pName, char* valBuf, uint32 bufLen)
{
	LTCommandVarView* pVar;

	if (pName && bufLen > 0)
	{
		valBuf[0] = 0;

		pVar = cc_FindConsoleVar(&m_pClientMgr->m_ServerConsoleState, pName);

		if (pVar)
		{
			strncpy(valBuf, pVar->m_pStringVal, bufLen - 1);
			valBuf[bufLen - 1] = 0;

			return LT_OK;
		}

		RETURN_ERROR_INFO(2, "CLTClient::GetSConValueString", LT_NOTFOUND, pName);
	}

	RETURN_ERROR(2, "CLTClient::GetSConValueString", LT_INVALIDPARAMS);
}


// 0x004074a0, 33 bytes — slot 85 of 0x004a14e0, iltclient.h index 14, which
// is StopVideo() and nothing else: it is the only no-argument video slot
// (`ret`, no immediate) in that run.
//
// WHAT PINS IT.
//  * `mov 0x360(%esi),%ecx` then `call *0x10(%eax)` with ONE argument —
//    iltvideomgr.h slot 4 (+0x10) is StopVideo(HVIDEO), which ltnew1.cpp's
//    StartVideo already reached at the same offset with the same argument.
//  * the clearing store `movl $0x0,0x3c4(%esi)` comes AFTER the call and
//    does not touch %eax, and the function then returns %eax untouched.  So
//    the interface result is carried in a local across the store; a
//    `return LT_OK;` would have emitted `xor %eax,%eax` and a plain
//    `return m_pVideoMgr->StopVideo(...)` could not have cleared the member
//    afterwards.  Three statements, in this order.
//  * no null test on m_hVideo, unlike StartVideo's — StopVideo is handed
//    whatever is there.
LTRESULT CLTClientConsoleView::StopVideo()
{
	LTRESULT dResult = m_pVideoMgr->StopVideo(m_hVideo);

	m_hVideo = LTNULL;

	return dResult;
}


// ======================================================================= //
//  B.  CPhysicsLT — two slots of the ILTPhysics vtable at 0x004a1770
//
//  MOVE ME: src/engine/lithtech/clientphysics.cpp, which already holds
//  CPhysicsLT::SetVelocity / SetAcceleration / MoveObject / SetObjectDims
//  and already declares `CClientMgr* m_pClientMgr` at 0x0c.  These two are
//  client-only (neither body's opening bytes occur in the retail server.dll),
//  so this is a same-image move, not a shared-file one.
//
//  A scratch TU can invent a residual; if either of
//  these comes back a near, re-measure it inside clientphysics.cpp before
//  believing the residual.
// ======================================================================= //

// iltphysics.h's MoveInfo, transcribed rather than included: iltphysics.h
// also needs `ClientServerType`, which is not in the SDK headers this file
// can reach.  Three fields, 20 bytes; the shipped code reads +0x00 and
// +0x04 and writes +0x08..+0x13, which is exactly this.
class MoveInfoView
{
public:
	HOBJECT  m_hObject;                      // 0x00
	float    m_dt;                           // 0x04
	LTVector m_Offset;                       // 0x08
};

// One object, as far as UpdateMovement reaches it.  Every offset is
// src/engine/server/ltobject_internal.h's already.
class CPhysObjView
{
public:
	uint8    m_pad_00[0x88];
	uint32   m_Flags;                        // 0x088  FLAG_*
	uint8    m_pad_8c[0xc4 - 0x8c];
	LTVector m_Velocity;                     // 0x0c4
	LTVector m_Acceleration;                 // 0x0d0
	uint8    m_pad_dc[0x120 - 0xdc];
	uint32   m_ChangeFlags;                  // 0x120  CHANGEFLAG_*
};

// The client manager's standing move request, at CClientMgr+0x4c4.
// clientmgr2.cpp currently has that range as `m_pad_4c4[0x504 - 0x4c4]`,
// which is 0x40 bytes — exactly the extent the two functions below use
// (the last field written is the offset vector at +0x34..+0x3f).
//
// Fields, all read off these two bodies plus SetGlobalForce (0x00405190),
// which writes m_vGlobalForce and then takes its Mag:
class CMoveAbstractView
{
public:
	CPhysObjView* m_pObject;                 // 0x00  (CClientMgr+0x4c4)
	float         m_fDeltaTime;              // 0x04
	uint32        m_nFlags;                  // 0x08  the object's m_Flags
	LTVector*     m_pVelocity;               // 0x0c  &m_pObject->m_Velocity
	LTVector*     m_pAcceleration;           // 0x10  &m_pObject->m_Acceleration
	LTVector      m_vGlobalForce;            // 0x14  (CClientMgr+0x4d8)
	LTVector      m_vGlobalForceDir;         // 0x20  m_vGlobalForce / its Mag
	float         m_fGlobalForceMag;         // 0x2c
	uint8         m_pad_30[0x34 - 0x30];
	LTVector      m_vOffset;                 // 0x34  the answer, ending the
	                                         //       0x40 bytes clientmgr2.cpp
	                                         //       currently pads over

	// commonphysics.h's GlobalForceControl::Set, expanded at /Ob1.
	inline void SetGlobalForce(LTVector* pVec)
	{
		m_vGlobalForce = *pVec;
		m_fGlobalForceMag = m_vGlobalForce.Mag();

		if (m_fGlobalForceMag > 1e-5f)
		{
			m_vGlobalForceDir = m_vGlobalForce;
			m_vGlobalForceDir /= m_fGlobalForceMag;
		}
		else
		{
			m_vGlobalForceDir.x = 0.0f;
			m_vGlobalForceDir.y = 0.0f;
			m_vGlobalForceDir.z = 0.0f;
		}
	}
};

class CClientMgrPhysicsView
{
public:
	uint8             m_pad_00[0x4c4];
	CMoveAbstractView m_MoveAbstract;        // 0x4c4
};

// 0x004500d0 (name ours) — the integrator.  cdecl, one argument, and it
// reads exactly the fields filled in above: +0x04 (the time, immediately
// squared and halved against the 0.5f at 0x004a1a74), +0x0c and +0x10 (the
// two vector pointers) and +0x00.
void pm_UpdateMovement(CMoveAbstractView* pMove);

class CPhysicsLTView
{
public:
	uint8 m_pad_00[0x0c];
	// 0x0c — clientphysics.h already has this as CClientMgr* m_pClientMgr.
	CClientMgrPhysicsView* m_pClientMgr;

	LTRESULT GetGlobalForce(LTVector& vec);                           // 0x00405160
	LTRESULT SetGlobalForce(LTVector& vec);                           // 0x00405190
	LTRESULT UpdateMovement(MoveInfoView* pInfo);                     // 0x00404f30
};


// 0x00405160, 33 bytes — vtable slot at 0x004a17ac, ILTPhysics index 15.
//
// WHAT PINS IT.
//  * index 15 is GetGlobalForce and it is PURE in iltphysics.h, which is
//    why a client body sits here at all; the direction settles which of the
//    Get/Set pair it is — this one copies OUT of the manager INTO the
//    caller's vector.
//  * `add $0x4d8,%eax` and then three loads off the adjusted base is
//    a whole-vector assignment (`*pAdd = pCam->m_LightAdd`), not
//    three component assignments, which would have kept the object in the
//    base register and indexed it.
//  * `xor %eax,%eax; ret $0x4` — LTRESULT, one argument, so the parameter
//    is the reference iltphysics.h declares.
LTRESULT CPhysicsLTView::GetGlobalForce(LTVector& vec)
{
	vec = m_pClientMgr->m_MoveAbstract.m_vGlobalForce;

	return LT_OK;
}


// 0x00405190, 155 bytes — vtable slot at 0x004a17b0, ILTPhysics index 16,
// SetGlobalForce: the client twin of src/engine/server/commonphysics.cpp's
// SPhysicsLT::SetGlobalForce (lithtech 0x0046c0f0), byte for byte the same
// body with the record at CClientMgr+0x4c4 instead of CServerMgr+0x668.
//  * `add $0x4c4,%ecx` first, then the whole-vector copy, the Mag, `fsts
//    0x2c(%ecx)` and a compare against 1e-5f (0x004a1a04 holds 0x3727c5ac).
//  * the divisor is RELOADED (`flds 0x2c(%ecx)`) after the branch and held
//    across the three `fdiv %st(1)`: the divide reads the member, not a
//    local left on the FPU stack.
//  * the else arm is three zero stores off one `xor %eax,%eax`.
LTRESULT CPhysicsLTView::SetGlobalForce(LTVector& vec)
{
	m_pClientMgr->m_MoveAbstract.SetGlobalForce(&vec);

	return LT_OK;
}


// 0x00404f30, 193 bytes — vtable slot at 0x004a17bc, the first
// ILTClientPhysics slot, and the check literal names it outright.
//
// WHAT PINS IT.
//  * one report block, reached from two tests, sitting AFTER both returns —
//    but reached by `je` from tests that fall through into the body.  Both
//    tests are against the SAME materialised zero (`xor %edx,%edx` then
//    `cmp %edx,%edi` / `cmp %edx,%eax`), which is the "a zero
//    already in a register is reused as a constant": the same %edx serves
//    the two null tests and the three zero stores at the end.
//  * the first store goes in at the folded displacement
//    (`mov %eax,0x4c4(%esi)`) and only THEN is the base adjusted
//    (`add $0x4c4,%esi`).  That IS a named
//    `CMoveAbstractView* pMove = &m_pClientMgr->m_MoveAbstract;` local
//    (measured: identical).  Writing the member chain out at every use, as
//    this body formerly did, reloads m_pClientMgr after each store (266
//    bytes); a `pMgr = m_pClientMgr` local gets 227.  `mov (%esi),%eax`
//    RELOADS the object pointer just stored instead of reusing
//    pInfo->m_hObject because the source reads pMove->m_pObject.
//  * `mov 0x120(%eax),%cl; test $0x40,%cl` — ltobject_internal.h's
//    CHANGEFLAG_MOTION is 0x40, and the test is byte-wide because the mask
//    fits a byte (movzx elision).
//  * TWO separate `xor %eax,%eax` / `ret $0x4` epilogues, one of them with
//    the xor scheduled into the middle of the copy — "several
//    return 0 epilogues mean several separate if (...) return X;
//    statements", so the no-motion case returns early rather than being an
//    else arm.
//  * the copy at the end adjusts BOTH bases (`add $0x34,%esi`,
//    `add $0x8,%edi`) before three load/store pairs: a whole-vector
//    assignment on each side, not six component assignments.
LTRESULT CPhysicsLTView::UpdateMovement(MoveInfoView* pInfo)
{
	if (!pInfo || !pInfo->m_hObject)
		RETURN_ERROR(2, "CPhysicsLT::UpdateMovement", LT_INVALIDPARAMS);

	CMoveAbstractView* pMove = &m_pClientMgr->m_MoveAbstract;

	pMove->m_pObject = (CPhysObjView*)pInfo->m_hObject;
	pMove->m_fDeltaTime = pInfo->m_dt;
	pMove->m_pVelocity = &pMove->m_pObject->m_Velocity;
	pMove->m_pAcceleration = &pMove->m_pObject->m_Acceleration;
	pMove->m_nFlags = pMove->m_pObject->m_Flags;

	if (!(pMove->m_pObject->m_ChangeFlags & CHANGEFLAG_MOTION))
	{
		pMove->m_vOffset.Init();

		return LT_OK;
	}

	pm_UpdateMovement(pMove);

	pInfo->m_Offset = pMove->m_vOffset;

	return LT_OK;
}


// ======================================================================= //
//  C.  Two LTRotation members the engine kept OUT of line
//
//  MOVE ME: wherever the engine's own ltmath/ltquatbase TU is.  Both bodies
//  also appear verbatim in the retail server.dll for the second one, so at
//  least operator= is in a file lithtech.exe and server.dll both compile.
//
//  WHY THEY ARE FORCED RATHER THAN DEFINED.  Our SDK copy of ltrotation.h
//  defines both of these `inline`, and VC6 at /Ob1 WILL expand them — they
//  are four assignments each.  The engine's own copy of that header plainly
//  carried declarations only, exactly as for
//  ltvector.h/ltmatrix.h, because the shipped image CALLS them (eight call
//  sites for Init, twelve for operator=).  Redefining them out of line here
//  would collide with the SDK header, so instead their addresses are taken,
//  which obliges VC6 to emit the out-of-line COMDAT of each.  The bytes are
//  the header's own body either way; what this block asserts is only that
//  the body is the one below.
// ======================================================================= //

// 0x00402ea0, 30 bytes — LTRotation::Init(float,float,float,float).
//
// WHAT PINS IT.
//  * `ret $0x10` and four dword stores at 0x00,0x04,0x08,0x0c off %ecx:
//    a thiscall member taking four four-byte arguments and writing them,
//    in order, to a sixteen-byte object.  LTRotation::m_Quat[4] is the only
//    such member in the SDK, and Init is the only four-argument writer.
//  * the eight call sites clinch it: 0x00402c9a pushes 0x3f800000 then
//    three zeros — Init's own default arguments (0,0,0,1) — and the object
//    it is called on is immediately followed in the frame by the fields an
//    ObjectCreateStruct has after its rotation.
//  * the stores are INTEGER movs, which is how VC6 copies float to float.
//  * the loads and stores interleave (load a, load b, store a, load c,
//    store b, load d, store c, store d), i.e. four separate assignments,
//    not a struct copy.
//
//    void LTRotation::Init(float x, float y, float z, float w)
//    {
//        m_Quat[QX] = x;
//        m_Quat[QY] = y;
//        m_Quat[QZ] = z;
//        m_Quat[QW] = w;
//    }

// 0x0047c460, 31 bytes — LTRotation& LTRotation::operator=(const LTRotation&).
//
// WHAT PINS IT.
//  * `mov %ecx,%eax` FIRST — `this` is moved into the return register
//    before anything else, which frees %ecx for the source operand and is
//    what `return *this;` on a thiscall costs.  A void assignment helper
//    would not do it.
//  * `ret $0x4` and four dword copies between two sixteen-byte objects.
//  * the twelve call sites are all quaternion work: 0x00489feb copies the
//    sixteen-byte result of 0x00411a30 (which negates three components —
//    a conjugate) straight into a local.
//  * present verbatim in the retail server.dll, so it is shared-header code.
//
//    LTRotation& LTRotation::operator=(const LTRotation &other)
//    {
//        m_Quat[0] = other.m_Quat[0];
//        m_Quat[1] = other.m_Quat[1];
//        m_Quat[2] = other.m_Quat[2];
//        m_Quat[3] = other.m_Quat[3];
//        return *this;
//    }

// 0x004316b0, 31 bytes — float _CVector<float>::MagSqr() const.
//
// WHAT PINS IT.
//  * `flds 0x8(%ecx); flds 0x4(%ecx); flds (%ecx)` — z, y, x loaded in
//    reverse order, then x*x, + y*y, + z*z, with the three source loads
//    popped off at the end.  That is ltvector.h's
//    `return (x * x) + (y * y) + (z * z);` verbatim, and the rule "an FP
//    sum emits its terms backwards" says the reversal is the compiler's.
//  * thiscall, no arguments, result left in st(0) — Mag would have a
//    `fsqrt` or a double-argument call, Dot would have a by-value operand.
//  * VC6 declines to expand _CVector's
//    in-class members, and 0x004500d0 (the integrator this file's
//    UpdateMovement calls) has this exact instruction sequence written out
//    inline at 0x004500f8, which is what the caller looks like when the
//    expansion DOES happen — so the out-of-line copy is a real COMDAT and
//    not an artefact.
//  * present verbatim in the retail server.dll.
//
// Forced the same way as the two above, for the same reason.

void  (LTRotation::*g_pfnLTRotationInit)(float, float, float, float) = &LTRotation::Init;
LTRotation& (LTRotation::*g_pfnLTRotationAssign)(const LTRotation&) = &LTRotation::operator=;
float (LTVector::*g_pfnVectorMagSqr)() const = &LTVector::MagSqr;


// ======================================================================= //
//  D.  Five BaseNew<T> instantiations
//
//  MOVE ME: this is src/engine/server/collection.h's template, and the
//  instantiation list belongs beside whichever client TU owns the element
//  types (the constructors are at 0x004453f0..0x00446030, i.e. the client's
//  own 0x0044xxxx block, so it is NOT collection.cpp's server list).
//  lithtech.exe DOES compile collection.cpp a second time
//  (src/engine/lithtech/Release/collection.obj exists), so the natural home
//  is a client-only instantiation block inside that second compile — but
//  the element types themselves are not server types, so adding them to
//  collection.cpp unconditionally would put five spurious COMDATs into
//  server.dll. Guard them, or give the client its own list.
//
//  FIVE ROWS, ONE TEMPLATE.  The enumerated rows 0x0044c7ef, 0x0044c98e,
//  0x0044caff, 0x0044cb3f and 0x0044cc8e are all INTERIOR blocks: each is
//  the `jne` target of a `test %ebx,%ebx` three instructions earlier, i.e.
//  the construction loop of the function that starts 0x1f bytes above it.
//  The real functions are
//
//      0x0044c7d0  ->  0x0044c7ef   stride 0x14, ctor 0x00445560
//      0x0044c970  ->  0x0044c98e   stride 0x20, ctor 0x00445410
//      0x0044cae0  ->  0x0044caff   stride 0x24, ctor 0x00446030
//      0x0044cb20  ->  0x0044cb3f   stride 0x14, ctor 0x004453f0
//      0x0044cc70  ->  0x0044cc8e   stride 0x20, ctor 0x00445c50
//
//  and all five are byte-for-byte the same shape:
//
//      mov  0x4(%esp),%ecx        ; the LAlloc
//      mov  0x14(%esp),%edi       ; nElements   (0x10 = the (T*)0 carrier,
//      mov  (%ecx),%eax           ;              pushed and never read)
//      <stride arithmetic>
//      push %edx
//      call *0x4(%eax)            ; LAlloc slot 1 = Alloc
//      mov  %eax,%ebx
//      test %ebx,%ebx
//      jne  <the row address>
//      pop  %edi; pop %ebx; ret   ; if (!ptr) return 0;
//    row:
//      push %esi
//      mov  %ebx,%esi             ; pTemp = ptr
//      test %edi,%edi
//      je   done
//    loop:
//      test %esi,%esi             ; the placement new's own null test
//      je   skip
//      mov  %esi,%ecx
//      call <ctor>
//    skip:
//      add  $stride,%esi
//      dec  %edi
//      jne  loop
//    done:
//      pop %esi; mov %ebx,%eax; pop %edi; pop %ebx; ret
//
//  WHAT PINS IT.
//   * three stack arguments of which the middle is never read is the type
//     carrier collection.h already documents: LNew_Array passes `(T*)0`.
//   * `call *0x4(%eax)` on the first argument is LAlloc slot 1, Alloc.
//   * `test %ebx,%ebx; jne` over a bare epilogue is the "vanishing
//     null test is an early return": `if (!ptr) return 0;` survives here,
//     rather than folding into the final `return ptr`, precisely because
//     the construction loop stands between the two returns.
//   * `test %edi,%edi; je` at the top with `dec %edi; jne` at the bottom is
//     `while (nElements) { --nElements; ... }`, not a for.
//   * the inner `test %esi,%esi` is loop-invariant and VC6 keeps it: that
//     is placement new's guard, which is what says the element type has a
//     constructor at all.  collection.h's BaseNew is already written this
//     way and matched nine times in server.dll.
//
//  The element types are named by size and by which constructor they run;
//  nothing else in the image distinguishes them, and the constructor call
//  is a masked relocation, so only sizeof(T) and the *existence* of a
//  declared constructor are load-bearing here.
// ======================================================================= //

class CLtElem14A { public: CLtElem14A(); uint8 m_pad_00[0x14]; };  // ctor 0x00445560
class CLtElem14B { public: CLtElem14B(); uint8 m_pad_00[0x14]; };  // ctor 0x004453f0
class CLtElem20A { public: CLtElem20A(); uint8 m_pad_00[0x20]; };  // ctor 0x00445410
class CLtElem20B { public: CLtElem20B(); uint8 m_pad_00[0x20]; };  // ctor 0x00445c50
class CLtElem24A { public: CLtElem24A(); uint8 m_pad_00[0x24]; };  // ctor 0x00446030

template CLtElem14A* BaseNew(LAlloc*, CLtElem14A*, uint32);   // 0x0044c7d0
template CLtElem20A* BaseNew(LAlloc*, CLtElem20A*, uint32);   // 0x0044c970
template CLtElem24A* BaseNew(LAlloc*, CLtElem24A*, uint32);   // 0x0044cae0
template CLtElem14B* BaseNew(LAlloc*, CLtElem14B*, uint32);   // 0x0044cb20
template CLtElem20B* BaseNew(LAlloc*, CLtElem20B*, uint32);   // 0x0044cc70

// 0x0044c560 and 0x0044c5f0 — CMoArray<CLtElem20B, NoCache>::SetSize2 and
// ::InternalNiceSetSize, collection.h:826 and :858 with every cache term
// erased by NoCache. They were left unwritten by an earlier pass on the
// grounds that closing them "forces a template instantiation" and the note
// above records the open question of where the client's instantiation list
// belongs. That question does not actually block these two: the note's own
// answer is "give the client its own list", and THIS FILE IS ALREADY THAT
// list — it compiles only into lithtech.exe, so nothing here can put a
// spurious COMDAT into server.dll, which was the whole worry.
//
// Instantiated member by member rather than with `template class`, which
// would emit every member of the template and not just the two the image
// has.
template class CMoArray<CLtElem20B, NoCache>;


// ======================================================================= //
//  E.  Small client bodies
// ======================================================================= //

// ---- 0x0040c5d0, 30 bytes -------------------------------------------- //
//
// WHAT PINS IT.
//  * cdecl, one argument; the call is to 0x00429080, which is
//    `dfree` (NOT 0x004290c0, `operator delete`), so
//    the source says dfree and not delete.
//  * `push %eax` reuses the value already loaded for the test rather than
//    reloading the member, and the clearing store is a separate
//    `movl $0x0,0x8(%esi)` after the cdecl cleanup: two statements inside
//    one guard, in that order.
//  * the guard's arm falls through to the shared `pop %esi; ret`, so there
//    is no early return — a single positive `if`.
// Its three callers (0x0040c573, 0x0040ee65, 0x0040f022) all unlink the
// record first and free it afterwards, which is what says +0x08 is a buffer
// the record owns rather than a link.
class CBuffer08View
{
public:
	uint8  m_pad_00[0x08];
	void*  m_pData;                          // 0x08
};

void dfree(void* pData);                     // 0x00429080

void sub_40c5d0(CBuffer08View* pRec)
{
	if (pRec->m_pData)
	{
		dfree(pRec->m_pData);

		pRec->m_pData = LTNULL;
	}
}


// ---- 0x00447110, 30 bytes -------------------------------------------- //
//
// WHAT PINS IT.
//  * thiscall, no arguments, returns a sum in %eax.
//  * +0x38 is a pointer array and +0x3c its count.  Those two offsets four
//    apart, with the count tested `jbe` (unsigned) and the array walked
//    four bytes at a time, are a CMoArray embedded at +0x34: collection.h
//    puts m_pArray at +0x04 and m_nElements at +0x08 of the array object,
//    so an array object at 0x34 has them at 0x38 and 0x3c.  The caller
//    (0x00446b51) reaches a SECOND such array in the same object at
//    0x0168/0x016c with a 0x20 stride, so the pattern is the class's.
//  * `test %edx,%edx; jbe` is an unsigned `> 0`, i.e. a `for` from
//    zero against an unsigned count, and the body is the strength-reduced
//    pointer walk VC6 makes of an INDEXED source loop.
//  * `mov (%ecx),%esi; mov 0x8(%esi),%edi; add %edi,%eax` — the element is
//    a POINTER and the summed field is at its +0x08.
//  * %esi/%edi are pushed inside the guard, not in the prologue: the empty
//    case never saves them, which is shrink-wrapping and needs the loop to
//    be the only user.
class CSizedThingView
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_nSize;                          // 0x08
};

class CThingArrayOwnerView
{
public:
	uint8              m_pad_00[0x38];
	CSizedThingView**  m_pThings;            // 0x38  (the array object is at 0x34)
	uint32             m_nThings;            // 0x3c

	uint32 GetTotalSize();                                            // 0x00447110
};

uint32 CThingArrayOwnerView::GetTotalSize()
{
	uint32 nTotal = 0;

	for (uint32 i = 0; i < m_nThings; i++)
		nTotal += m_pThings[i]->m_nSize;

	return nTotal;
}


// 0x00454f00 (31) and 0x00466b90 (52) used to be written here as
// sub_454f00 and sub_466b90. They are server/commonphysics.cpp's
// wm_RotatePushFilter and server/console.cpp's cc_DisableWMPhysics, which
// are byte-identical on lithtech.exe from there, so these copies were
// removed as duplicates.


// ---- 0x0041c0b0, 44 bytes  (the row 0x0041c0bd is its second block) --- //
//
// WHAT PINS IT.
//  * the row address is the target of `jne 0x41c0bd` at 0x0041c0b8, three
//    instructions into the function: an INTERIOR block, and %eax and %edx
//    are live into it from the compare above.  The function starts at
//    0x0041c0b0.
//  * it is slot 2 of the vtable at 0x004a1e60.  That table is the DERIVED
//    half of a pair: 0x0041c060 stores 0x004a1e60, calls a base destructor,
//    then stores 0x004a1e38, and the two tables share their slots 5..8.
//  * the early `return LTFALSE` is INLINE at the first test and the body is
//    jumped to, which is the negated-guard form.
//  * `inc %eax` before `mov %eax,0x8(%ecx)` with the compare between them:
//    the increment is stored unconditionally and the wrap is a separate
//    `if`.  `jl` is SIGNED, so the two members are ints.
//  * the answer reloads +0x08 from memory after the merge and ends in
//    `xor %eax,%eax; cmp; setne %al` — an LTBOOL-valued `!=`, not a
//    branch.
class CRingCursorView
{
public:
	uint8 m_pad_00[0x08];
	int   m_nCur;                            // 0x08
	int   m_nSize;                           // 0x0c
	uint8 m_pad_10[0x14 - 0x10];
	int   m_nEnd;                            // 0x14

	LTBOOL sub_41c0b0();
};

LTBOOL CRingCursorView::sub_41c0b0()
{
	if (m_nCur == m_nEnd)
		return LTFALSE;

	m_nCur++;

	if (m_nCur >= m_nSize)
		m_nCur = 0;

	return m_nCur != m_nEnd;
}


// ---- 0x0045a450 and 0x0045a460, 5 + 16 bytes -------------------------- //
//
// The 32-byte row covers TWO functions: a five-byte `jmp` at 0x0045a450
// followed by eleven alignment `nop`s, and the sixteen-byte body at
// 0x0045a460.
//
// WHAT PINS IT.
//  * `mov $0x4b40c8,%eax; mov %eax,0x4b40cc; mov %eax,0x4b40c8` — one
//    address materialised once and stored to +0x04 and then +0x00 of the
//    same object.  ltlink.h's TieOff is `m_pPrev = m_pNext = (LTLink*)this;`
//    and a chained assignment stores the RIGHTMOST target first, so m_pNext
//    (+0x04) really does precede m_pPrev (+0x00).  Nothing else in ltlink.h
//    writes those two and only those two.
//  * 0x004b40c8 is a genuine list head: 0x0045bc47 stores its address into
//    a fresh link and 0x0045bcb2/0x0045bcc1 compare walk pointers against
//    it.
//  * the `jmp` is a tail call, which is what VC6 emits for a void function
//    whose whole body is a call to another void function with no arguments.
//    0x0045a450 is entry 10 of the function-pointer table at 0x004a7090,
//    so it is a module hook that delegates to the reset below.
//  * it is netmgr.cpp's g_Un390List (objtypes.cpp reads the same address
//    under that name), so this is an extern of that definition.
extern LTLink g_Un390List;                   // 0x004b40c8

void sub_45a460();

// Defined in the shipped order (0x0045a450 before 0x0045a460), which is the
// source definition order for COFF layout.
void sub_45a450()
{
	sub_45a460();
}

void sub_45a460()
{
	g_Un390List.TieOff();
}


// ---- 0x0048a620, 32 bytes -------------------------------------------- //
//
// WHAT PINS IT.
//  * thiscall, no arguments, no return.
//  * `mov (%esi),%eax; test %eax,%eax; je` — an owner pointer at +0x00,
//    guarded.
//  * inside the guard: `lea 0x4(%esi),%ecx; push %ecx` (the argument is
//    &this->+0x04) and `lea 0x28(%eax),%ecx` (the receiver is
//    &owner->+0x28), then a thiscall to 0x0045a400.  0x0045a400 is a
//    singly-linked-list unlink: it walks `next` at +0x08 from the head it
//    is given and relinks past the node whose address it is passed.  So
//    +0x04 here is this object's node in the owner's list at +0x28.
//  * after the guard, unconditionally, `push $0x0; mov %esi,%ecx;
//    call 0x0048a680` — a one-argument thiscall on THIS object with a null
//    argument, and it is a `call` and not a tail `jmp` because %esi is
//    still to be popped.
//  * 0x0048a640 immediately below is the same prologue and the same first
//    six instructions, i.e. a sibling that shares the unlink and differs
//    afterwards; that is why the unlink is written out here rather than
//    being a helper.
// 0x0045a400 — thiscall, ONE argument (`ret $0x4`): the receiver is the
// list head and the argument the node. It walks `next` at +0x08 from the
// head and relinks past the node it is given.
class CSimpleListView
{
public:
	void  Remove(void* pNode);               // 0x0045a400

	void* m_pHead;                           // 0x00
};

class CListNodeOwnerView
{
public:
	uint8           m_pad_00[0x28];
	CSimpleListView m_List28;                // 0x28
};

class CListMemberView
{
public:
	CListNodeOwnerView* m_pOwner;            // 0x00
	uint8               m_Node04[0x0c];      // 0x04  next at +0x08 of the node
	CSimpleListView*    m_pOld10;            // 0x10  (0x0048a680)
	uint8               m_Node14[0x0c];      // 0x14  (0x0048a680)

	void sub_48a620();
	void sub_48a640(CListNodeOwnerView* pOwner);
	void sub_48a680(CSimpleListView* pList);
};

void CListMemberView::sub_48a620()
{
	if (m_pOwner)
		m_pOwner->m_List28.Remove(m_Node04);

	sub_48a680(LTNULL);
}


// ---- 0x0048a640, 51 bytes -------------------------------------------- //
//
// binkvideo.cpp's / smackvideo.cpp's CVideoBinding::SetPoly (HOME: the
// shared video-manager TU, 0x0048a5f0..; written here beside its sibling
// 0x0048a620 on the same view).  `ret $0x4`, %eax never set: void, one
// argument.
//  * the first six instructions are 0x0048a620's unlink, verbatim.
//  * then the argument is stored at +0x00 and, if non-null, this object's
//    node goes on the NEW owner's +0x28 list through 0x0043c580 -- cdecl,
//    two arguments, the list (`add $0x28,%eax`) pushed last so it is the
//    first.  Both bases are adjusted in place (`add $0x4,%esi`), no lea.
void lst_AddNodeQ48A(CSimpleListView* pList, void* pNode);   // 0x0043c580

void CListMemberView::sub_48a640(CListNodeOwnerView* pOwner)
{
	if (m_pOwner)
		m_pOwner->m_List28.Remove(m_Node04);

	m_pOwner = pOwner;

	if (pOwner)
		lst_AddNodeQ48A(&pOwner->m_List28, m_Node04);
}


// ---- 0x0048a680, 47 bytes -------------------------------------------- //
//
// binkvideo.cpp's / smackvideo.cpp's CVideoBinding::SetOldTexture (HOME:
// the shared video-manager TU, like its two siblings above).  0x0048a640's
// shape on the SECOND hooked pointer: +0x10 and its node at +0x14, and the
// list is the texture's own +0x00 (`mov 0x10(%esi),%ecx` straight into
// 0x0045a400; `push %eax` with no offset into 0x0043c580).  `ret $0x4`,
// void.  0x0048a620 above calls it with LTNULL.
void CListMemberView::sub_48a680(CSimpleListView* pList)
{
	if (m_pOld10)
		m_pOld10->Remove(m_Node14);

	m_pOld10 = pList;

	if (pList)
		lst_AddNodeQ48A(pList, m_Node14);
}


// ---- 0x00490710, 29 bytes -------------------------------------------- //
//
// WHAT PINS IT.
//  * thiscall, no arguments, returns 1.  It is slot 11 of the vtable at
//    0x004a3bc0 (the entry at 0x004a3bec).
//  * `mov 0x14(%esi),%ecx; test %ecx,%ecx; je done` then a body that calls
//    0x00490ac0 with that same pointer in %ecx and RELOADS +0x14 before
//    testing again: a rotated `while (m_p14) m_p14->Release();` in which
//    the callee is what empties the slot.  The reload is not optional — the
//    call may change the member — so this cannot be a `do`/`while` or an
//    `if`.
//  * `mov $0x1,%eax` (five bytes, not the three-byte push/pop form) then
//    the epilogue: one unconditional `return LTTRUE;` after the loop.
class CChain14View
{
public:
	uint8 m_pad_00[0x14];
	class CChain14Item* m_p14;               // 0x14

	LTBOOL sub_490710();
};

class CChain14Item
{
public:
	void sub_490ac0();
};

LTBOOL CChain14View::sub_490710()
{
	while (m_p14)
		m_p14->sub_490ac0();

	return LTTRUE;
}


// ---- 0x0047c180, 30 bytes -------------------------------------------- //
//
// WHAT PINS IT.
//  * it is a vtable entry (0x004a33f0) with `ret $0x8` that never touches
//    %ecx: a two-argument virtual whose body does not use `this`.
//  * `mov 0x188(%eax),%ecx` is ltobject_internal.h's m_pSData, and
//    `mov 0x20(%ecx),%ecx` is SObjData::m_pBaseClass — the game object.
//  * `mov (%ecx),%edx; call *0x4(%edx)` is slot 1 of BaseClass, which
//    ltengineobjects.h declares as EngineMessageFn(uint32, void*, LTFLOAT)
//    behind the destructor at slot 0.
//  * the pushes are $0x0, then the second parameter, then $0x6 — so the
//    arguments in source order are (6, pData, 0.0f), and ltserverobj.h has
//    MID_CRUSH == 6.  The trailing 0.0f is pushed as an integer immediate,
//    which is how VC6 passes a float literal zero.
//  * the vtable call's result falls straight into `ret $0x8`, so the
//    message result is returned.
class CBaseClassView
{
public:
	virtual ~CBaseClassView();
	virtual uint32 EngineMessageFn(uint32 messageID, void* pData, float fData);
};

class CSObjDataView
{
public:
	uint8           m_pad_00[0x20];
	CBaseClassView* m_pBaseClass;            // 0x20
};

class CCrushObjView
{
public:
	uint8          m_pad_00[0x188];
	CSObjDataView* m_pSData;                 // 0x188
};

class CCrushHostView
{
public:
	LTRESULT sub_47c180(CCrushObjView* pObj, void* pData);
};

LTRESULT CCrushHostView::sub_47c180(CCrushObjView* pObj, void* pData)
{
	return pObj->m_pSData->m_pBaseClass->EngineMessageFn(MID_CRUSH, pData, 0.0f);
}


// ---- 0x0047ac40, 51 bytes -------------------------------------------- //
//
// The 224-byte row covers TWO functions: this one ends at 0x0047ac72 with
// thirteen alignment `nop`s after it, and 0x0047ac80 (block F below) is the
// next.
//
// WHAT PINS IT.
//  * cdecl, two arguments.  +0x78 of the first is a CClientMgr — its +0x61c
//    is the field clientmgr2.cpp already names m_pClientShell.
//  * `mov 0x78(%eax),%ecx` is still live in %ecx at the call to 0x00411440,
//    which is a thiscall on the manager (that function is
//    `mov 0x4bc(%ecx),%eax; mov 0x4(%esp),%ecx; mov %eax,0xc(%ecx);
//    ret $0x4`), so the manager is reached once and used twice.
//  * `mov (%esi),%edx; push %edi; push $0x0; mov %esi,%ecx;
//    call *0x28(%edx)` — slot 10 of IClientShell, which iclientshell.h
//    declares as SpecialEffectNotify(HLOCALOBJ, HMESSAGEREAD): a null
//    object and the message.  The message argument is the second parameter
//    PLUS EIGHT (`add $0x8,%edi`), so the parameter is the container and
//    the read handle sits eight bytes into it.
//  * two epilogues that cannot be merged (the body path also pops %edi), so
//    the positive-guard form with the shared answer last is what the layout
//    wants: `if (pShell) { ... } return LT_OK;`
class CClientShellView
{
public:
	virtual ~CClientShellView();
	virtual void OnCommandOn(int command);
	virtual void OnCommandOff(int command);
	virtual void OnKeyDown(int key, int rep);
	virtual void OnKeyUp(int key);
	virtual void OnMessage(uint8 messageID, void* hMessage);
	virtual void OnModelKey(HLOCALOBJ hObj, void* pArgs);
	virtual void PreLoadWorld(char* pWorldName);
	virtual void OnEnterWorld();
	virtual void OnExitWorld();
	virtual void SpecialEffectNotify(HLOCALOBJ hObj, void* hMessage);
};

class CClientMgrShellView
{
public:
	uint8             m_pad_00[0x61c];
	CClientShellView* m_pClientShell;        // 0x61c

	void sub_411440(class CMessageView* pMsg);
};

class CMsgCtxView
{
public:
	uint8                 m_pad_00[0x78];
	CClientMgrShellView*  m_pClientMgr;      // 0x78
};

// The container the handler is handed: the read handle is eight bytes in.
class CMessageView
{
public:
	uint8 m_pad_00[0x08];
	uint8 m_ReadHandle[0x04];                // 0x08
};

LTRESULT sub_47ac40(CMsgCtxView* pCtx, CMessageView* pMsg)
{
	CClientShellView* pShell = pCtx->m_pClientMgr->m_pClientShell;

	if (pShell)
	{
		pCtx->m_pClientMgr->sub_411440(pMsg);

		pShell->SpecialEffectNotify(LTNULL, pMsg->m_ReadHandle);
	}

	return LT_OK;
}


// ======================================================================= //
//  F.  The three CPacket::ReadType callers
//
//  All three inline src/engine/server/packet.h's ReadType member template,
//  which is why "CPacket::ReadType" is the literal the enumeration hung on
//  them.  The template is not reconstructed here — it is already written,
//  and every byte of the inlined copy agrees with it: the two bounds tests
//  (`end < m_Data.m_AllocSize` with `jae`, `end <= m_DataLen` with `ja`),
//  the `or $0x2,0x40(%eax)` READOVERFLOW set BEFORE the report, level 1,
//  and the LT_ERROR / overrun-text pair.
//
//  ONE THING TO KNOW BEFORE READING THE BYTES.  ReadType's `val` is left
//  UNINITIALISED on the overflow path, and it has no frame slot: none of
//  the three functions has a `sub $N,%esp`.  VC6 homes the temporary in a
//  PARAMETER slot instead.  In 0x004694b0 that slot belongs to the packet
//  pointer, which is genuinely dead (cached in %esi at entry and never
//  reloaded); in 0x0047ac80 it is shared with a parameter that IS still
//  live, which is only sound because the value read there is undefined
//  anyway.  Do not read either as a member or as an extra argument.
// ======================================================================= //

// ---- 0x0047a3e0, 160 bytes ------------------------------------------- //
//
// WHAT PINS IT.
//  * cdecl, two arguments, and its one caller (0x0047a2a0) pushes %esi then
//    %ebx, so argument one is the context and argument two the packet.
//    That is the same context object 0x0047ac40 takes: the caller reads its
//    +0x78 as a CClientMgr four instructions later.
//  * `lea 0x4(%ecx),%esi` — the read is four bytes wide, and `flds` loads
//    it, so T is float.
//  * `flds <temp>; fcomps 0x18(%ecx)` then `test $0x41,%ah; jne` past the
//    store: the store happens when st(0) is strictly GREATER than the
//    member (C0 and C3 both clear), so the source is `if (f > m_f18)`.
//    The store itself is `mov %edx,0x18(%ecx)` — an integer move of the
//    dword the temporary holds, which is a float-to-float assignment.
//  * `fildl 0x004a9ed8` then `fdivrs 0x004a1a08` (which is 1.0f):
//    st(0) = 1.0f / (float)<the int at 0x004a9ed8, currently 10>.  The
//    integer goes through `fild` rather than `fidiv`, which
//    means a float TEMPORARY stood between the cast and the divide.
//  * `xor %eax,%eax` scheduled into the middle of the FPU tail: one
//    `return LT_OK;` and no other exit.
class CPacketCtxView
{
public:
	uint8 m_pad_00[0x18];
	float m_fPeak18;                         // 0x18
	uint8 m_pad_1c[0x24 - 0x1c];
	float m_fRate24;                         // 0x24
};

// 0x004a9ed8 — an int, 10 in the shipped image; 0x0041585d reads only its
// low byte, which is what says it is a small settable count and not a
// pointer.  It is the "UpdateRate" console variable (the int* of that
// binding in lithtech.exe's table); server/dsi.cpp's name.
extern int g_CV_UpdateRate;

LTRESULT sub_47a3e0(CPacketCtxView* pCtx, CPacket* pPacket)
{
	float fVal;

	fVal = pPacket->ReadType((float*)0);

	if (fVal > pCtx->m_fPeak18)
		pCtx->m_fPeak18 = fVal;

	{
		float fCount = (float)g_CV_UpdateRate;

		pCtx->m_fRate24 = 1.0f / fCount;
	}

	return LT_OK;
}


// ---- 0x0047ac80, 160 bytes (the second half of the 0x0047ac40 row) ---- //
//
// WHAT PINS IT.
//  * cdecl, two arguments: the destination is argument one (read back as
//    `mov 0x8(%esp),%eax` after `pop %edi`) and the packet is argument two
//    (`mov 0x8(%esp),%eax` at entry, before any push).
//  * `lea 0x2(%ecx),%esi` and `mov (%esi,%ecx,1),%cx`: a two-byte read, so
//    T is uint16.
//  * `mov %cx,0x6c(%eax)` on the success path and `mov %ax,0x6c(%edx)` on
//    both report paths — one sixteen-bit store, reached from every exit,
//    which is the single assignment statement this function is.
//  * the two report tails differ only in the `add $0x10,%esp` the printing
//    path owes, so they are the same statement seen twice, not two.
class CField6cView
{
public:
	uint8  m_pad_00[0x6c];
	uint16 m_n6c;                            // 0x6c
};

LTRESULT sub_47ac80(CField6cView* pDest, CPacket* pPacket)
{
	pDest->m_n6c = pPacket->ReadType((uint16*)0);

	return LT_OK;
}


// ---- 0x004694b0, 189 bytes ------------------------------------------- //
//
// WHAT PINS IT.
//  * cdecl, THREE arguments.  The packet is argument two (`mov 0xc(%esp),
//    %esi` after one push) and the destination argument three
//    (`mov 0x14(%esp),%edi` after two).  Argument one is never read; the
//    handler is installed as a function pointer by the big initialiser at
//    0x00468f6x, so the shape is the family's and not this member's.
//  * `lea 0x2(%eax),%edx` and `mov (%edx,%eax,1),%ax`: T is uint16.
//  * `and $0xffff,%eax; mov %eax,0x114(%edi); je` — the count is stored
//    into a 32-BIT member (a full dword store) and the SAME value's flags
//    drive the guard, so the test is on the member just assigned.
//  * `push %eax; call 0x004290b0` is `operator new` (the
//    allocator wrapper), not dalloc (0x00428fd0) and not dfree; the size
//    pushed is the count itself, so the element is one byte wide.
//  * `mov 0x114(%edi),%dx` RELOADS the member as sixteen bits for the next
//    call, which is what a `uint16` parameter looks like at
//    a call site.  0x0045c2d0 is therefore `ReadData(void*, uint16)`,
//    thiscall on the packet.
//  * `mov 0x3c0(%edi),%eax; or $0x1,%ah; mov %eax,0x3c0(%edi)` — a 32-bit
//    read-modify-write with a byte-narrowed immediate, i.e. `|= 0x100` on
//    a dword field.
//
// COIN FLIP: `new uint8[nLen]` versus `(uint8*)::operator new(nLen)`.  VC6
// routes the array form through operator new[] which forwards to operator
// new, and the call operand is a masked relocation, so the two are
// indistinguishable in the bytes; the array form is written because the
// size pushed is the raw count with no multiply.
// packet.h's CPacket does not declare the payload reader; 0x0045c2d0 is a
// thiscall on the packet taking (void*, uint16) and is reached only from
// here, so it is added on a derived view rather than by editing packet.h.
class CPacketRead : public CPacket
{
public:
	void ReadData(void* pBuf, uint16 nLen);  // 0x0045c2d0
};

class CBlobDestView
{
public:
	uint8  m_pad_00[0x110];
	uint8* m_pData;                          // 0x110
	uint32 m_nDataLen;                       // 0x114
	uint8  m_pad_118[0x3c0 - 0x118];
	uint32 m_Flags3c0;                       // 0x3c0
};

LTRESULT sub_4694b0(void* pUnused, CPacketRead* pPacket, CBlobDestView* pDest)
{
	pDest->m_nDataLen = pPacket->ReadType((uint16*)0);

	if (pDest->m_nDataLen)
	{
		pDest->m_pData = new uint8[pDest->m_nDataLen];

		pPacket->ReadData(pDest->m_pData, (uint16)pDest->m_nDataLen);
	}

	pDest->m_Flags3c0 |= 0x100;

	return LT_OK;
}


// ======================================================================= //
//  F.  0x00402c90, 528 bytes -- counter.cpp's sub_10001b54 at /O2: written
//  there, on the SDK's ObjectCreateStruct and servermodel.h's inline
//  allocate, which matches both images (the /O2 twin that stood here is
//  gone).
// ======================================================================= //


// ======================================================================= //
//  THE PER-ADDRESS REPORT
//
//  addr        size  verdict
//  ---------- -----  ------------------------------------------------------
//  0x0047a3e0   160  DONE   sub_47a3e0. Inlines CPacket::ReadType<float>;
//                           the row's "CPacket::ReadType" tag is the
//                           template's own check literal, not a callee.
//  0x00406ab0   172  DONE   CLTClient::GetSConValueFloat (vtable slot 74).
//  0x004694b0   189  DONE   sub_4694b0. Inlines ReadType<uint16>; installed
//                           as a function pointer by 0x00468f6x.
//  0x00404f30   193  DONE   CPhysicsLT::UpdateMovement. MOVE ME to
//                           src/engine/lithtech/clientphysics.cpp (client
//                           only; not present in the retail server.dll).
//  0x00406b60   198  DONE   CLTClient::GetSConValueString (slot 75).
//  0x0047ac40   224  TWO FUNCTIONS, both DONE. The row spans 0x0047ac40
//                           (51 bytes, ends at 0x0047ac72 with thirteen
//                           alignment nops) and 0x0047ac80 (160 bytes,
//                           another ReadType<uint16> caller). The stated
//                           size is the distance to the next enumerated
//                           address, not one body.
//  0x00490710    29  DONE   sub_490710, slot 11 of the vtable 0x004a3bc0.
//  0x00402ea0    30  DONE   LTRotation::Init(float,float,float,float).
//                           MOVE ME: it belongs in whatever TU held the
//                           engine's out-of-line LTRotation members. Forced
//                           here by taking its address, because our SDK
//                           ltrotation.h defines it `inline` and VC6 at
//                           /Ob1 would expand it; the engine's own copy of
//                           that header plainly carried the declaration
//                           only, which is why the image has eight calls.
//  0x0040c5d0    30  DONE   sub_40c5d0 (dfree a buffer at +0x08, clear it).
//  0x00447110    30  DONE   CThingArrayOwnerView::GetTotalSize.
//  0x0044c7ef    30  NOT A FUNCTION - the construction loop INSIDE
//                           BaseNew<T> at 0x0044c7d0 (stride 0x14, ctor
//                           0x00445560). The whole function is written.
//                           MOVE ME: src/engine/server/collection.h's
//                           template, instantiated for a CLIENT element
//                           type, so the instantiation belongs beside
//                           whichever client TU owns it - not in
//                           collection.cpp's server list.
//  0x0044c98e    30  NOT A FUNCTION - same, inside 0x0044c970 (stride 0x20,
//                           ctor 0x00445410). Written.
//  0x0044caff    30  NOT A FUNCTION - same, inside 0x0044cae0 (stride 0x24,
//                           ctor 0x00446030). Written.
//  0x0044cb3f    30  NOT A FUNCTION - same, inside 0x0044cb20 (stride 0x14,
//                           ctor 0x004453f0). Written.
//  0x0044cc8e    30  NOT A FUNCTION - same, inside 0x0044cc70 (stride 0x20,
//                           ctor 0x00445c50). Written.
//  0x0047c180    30  DONE   sub_47c180, vtable entry 0x004a33f0: sends
//                           MID_CRUSH to the object's BaseClass.
//  0x0041c0bd    31  NOT A FUNCTION - the second basic block of the
//                           function at 0x0041c0b0 (slot 2 of the vtable
//                           0x004a1e60), reached by `jne` from 0x0041c0b8
//                           with %eax and %edx live. The whole function is
//                           written.
//  0x004316b0    31  DONE   _CVector<float>::MagSqr. MOVE ME: it is an SDK
//                           template COMDAT, present verbatim in
//                           the retail server.dll too. Forced by address.
//  0x00454f00    31  DONE   wm_RotatePushFilter, in commonphysics.cpp.
//  0x00466ba5    31  NOT A FUNCTION - the loop body of the function at
//                           0x00466b90, reached by falling through the
//                           entry test at 0x00466ba3. Written whole.
//  0x0047c460    31  DONE   LTRotation::operator=(const LTRotation&).
//                           MOVE ME with LTRotation::Init; also present
//                           verbatim in the retail server.dll, so it is a
//                           shared-header body: expect one exact and
//                           possibly one near.
//                           Forced by address, same reason as Init.
//  0x0045a450    32  TWO FUNCTIONS, both DONE: a five-byte tail `jmp` at
//                           0x0045a450 (plus eleven alignment nops) onto
//                           the sixteen-byte LTLink::TieOff on the global
//                           at 0x004b40c8, which is 0x0045a460.
//  0x0048a620    32  DONE   CListMemberView::sub_48a620.
//  0x00405160    33  DONE   CPhysicsLT::GetGlobalForce. MOVE ME with
//                           UpdateMovement.
//  0x004074a0    33  DONE   CLTClient::StopVideo (vtable slot 85).
//
//
//  COIN FLIPS I COULD NOT SETTLE
//
//  1. (0x00466b90's flag: moot here now; the function is console.cpp's
//     cc_DisableWMPhysics.)
//
//  2. 0x004694b0's allocation. `push %eax; call 0x004290b0` is
//     `operator new`, and the size pushed is the raw byte count with no
//     multiply. `new uint8[n]` and `(uint8*)::operator new(n)` differ only
//     in which of operator new / operator new[] the relocation names, and
//     the relocation is masked. Written as the array form. Alternative:
//         pDest->m_pData = (uint8*)::operator new(pDest->m_nDataLen);
//
//  3. 0x0047a3e0's reciprocal. `fildl <int>; fdivrs 1.0f` and not
//     `fld 1.0f; fidivl <int>`. The `fild` form needs a
//     float TEMPORARY between the cast and the divide, so the block-scoped
//     `float fCount` is written. Alternative, if that turns out to be one
//     instruction off:
//         pCtx->m_fRate24 = 1.0f / (float)g_CV_UpdateRate;
//     which gives `fidiv` and is therefore expected to
//     be WRONG - recorded as the disproved arm.
//
//  4. UpdateMovement's move-state addressing. Written with the member
//     chain spelled out at every store, because the FIRST store uses the
//     folded displacement `mov %eax,0x4c4(%esi)` and only the SECOND
//     triggers `add $0x4c4,%esi` - and because the object pointer is
//     RELOADED (`mov (%esi),%eax`) after being stored. A named local
//         CMoveAbstractView* pMove = &m_pClientMgr->m_MoveAbstract;
//     would put the lea/add ahead of the first store and reuse
//     pInfo->m_hObject instead of reloading, so it is the alternative to
//     try if this comes back a near, and it is expected to be worse.
//
//  5. 0x0047ac40's manager. `mov 0x78(%eax),%ecx` stays live in %ecx across
//     the shell test, so the manager is read once. Written as a local
//     `CClientShellView* pShell` plus a repeated `pCtx->m_pClientMgr`
//     chain. The alternative is a second local for the manager; both give
//     one load, and nothing in the bytes chooses.
// ======================================================================= //
