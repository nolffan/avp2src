// *********************************************************************** //
//  MODULE:   COMMONPHYSICS (.H)
//  PURPOSE:  The shared half of ILTPhysics — the eleven methods the SDK header
//            declares non-pure, so the engine implements them once for both
//            client and server. In server.dll they sit contiguously at
//            0x1001260b-0x100129d9.
//
//            The shipped parameter-check messages name this class three
//            different ways (ILTPhysics::, ILTCommon::, CommonLT::) and get
//            two of the method names wrong outright — SetObjectMass reports
//            "GetObjectMass", GetVelocity reports "CommonLT::GetVelocity".
//            Those literals are reproduced exactly as shipped: they are what
//            the bytes contain, and correcting them would break the match.
// *********************************************************************** //
#ifndef __COMMONPHYSICS_H__
#define __COMMONPHYSICS_H__

#include "ltobject_internal.h"
#include "iltphysics.h"

// The global-force state embedded in the world at +0x668. Set (0x1002c8aa)
// updates it from a vector; the current value sits at +0x14 (world +0x67c).
// The world's global force, kept three ways: the force itself, the direction
// it points in, and how strong it is — so the simulation never has to
// normalize per object. A force too small to have a direction leaves the
// direction at nothing rather than dividing by it (names ours).
//
// The first 0x14 bytes are not padding: they are the per-object request the
// frame-move worker (0x1001e088) is driven by, and the vector at +0x34 is
// what it answers with. Every field is read off two functions — the caller
// sm_GetFrameMove (0x1002a6de) filling it and the worker consuming it:
//
//   +0x00  the object being moved. The caller stores it (`mov %eax,(%ebx)`)
//          and the worker asks it what it is standing on (`0xe8(%esi)`).
//   +0x04  the frame time in seconds. The caller copies its third argument
//          into it; the worker's first act is `flds 0x4(%ebx)`, squaring it
//          and halving it for the `at²/2` term.
//   +0x08  the object's flag word: the caller stores `0x88(%eax)` (through a
//          local on one of its two paths) and the worker tests
//          `testb $0x2,0xa(%ebx)` — bit 17 of the dword, FLAG_GRAVITY.
//   +0x0c  the velocity, and +0x10 the acceleration, as POINTERS, not
//          copies: the caller passes `&pObj->m_Velocity` / `&m_Acceleration`
//          on its fast path and the addresses of two locals on the path that
//          has to adjust them first, and the worker calls MagSqr on each
//          through `ecx` and writes zeros back through them.
//   +0x34  the offset the frame produced. The worker clears it up front and
//          the caller copies it out to its fourth argument with three
//          `movsl`.
//
// So the record is 0x40 bytes, and the four force fields in the middle are
// the only part of it that outlives one call.
class GlobalForceControl
{
public:
	GlobalForceControl();                    // 0x1002c87b

	void Set(LTVector* pVec);                // 0x1002c8aa

	class LTObject* m_pObj;                  // +0x00
	float           m_fFrameTime;            // +0x04
	uint32          m_nObjFlags;             // +0x08
	LTVector*       m_pVelocity;             // +0x0c
	LTVector*       m_pAcceleration;         // +0x10
	LTVector m_Force;                        // +0x14
	LTVector m_Dir;                          // +0x20
	float    m_fMag;                         // +0x2c
	float    m_Unknown30;                    // +0x30
	LTVector m_vFrameMove;                   // +0x34
};

// 0x1001e088 — the worker the record above is filled for: it integrates the
// velocity and acceleration it was pointed at over m_fFrameTime and leaves
// the answer in m_vFrameMove. 1615 bytes and not reconstructed; the two CSV
// rows at 0x1001e127 and 0x1001e649 are false starts inside it (name ours).
//
// Extent re-confirmed from the bytes: `push ebp / mov esp,ebp / sub $0x58`
// at 0x1001e088 and the matching `leave / ret` at 0x1001e6d5, so the body is
// 0x1001e088..0x1001e6d6 and 0x1001e6d7 is the next function (a 25-byte
// three-dword copy). 159 + 1314 + 142 = 1615, which is the three enumeration
// rows tiling it exactly. A full .text sweep of every e8/e9/0f8x
// displacement finds the head reached from OUTSIDE twice, both from
// sm_GetFrameMove — 0x1002a7a1 and 0x1002a7de — and finds nothing at all
// reaching the other two rows from outside. So the head row is the real
// function with a truncated size, not a fragment: it cannot be excluded,
// only written.
//
// 629 instructions, 23 calls to 8 targets, and almost entirely x87: the
// scheduling is the whole difficulty, and it spills floats into the
// parameter slot at 0x8(%ebp) once the argument is dead, which is
// not source-reachable. The realistic ceiling is a near (only
// identical bytes count), which is why it has stayed unwritten.
int phys_ComputeFrameMove(GlobalForceControl* pControl);

// The world simulation the server physics forwards to. Two virtual slots are
// reached (ILTServer's pointer-taking force accessors go through them), and
// two data fields (the reference-taking ILTPhysics ones read them directly).
class IWorldSim
{
public:
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0; virtual void _p1c()=0; virtual void _p20()=0;
	virtual void _p24()=0; virtual void _p28()=0; virtual void _p2c()=0;
	virtual void _p30()=0; virtual void _p34()=0; virtual void _p38()=0;
	virtual LTRESULT GetGlobalForce(LTVector* pVec)=0;   // 0x3c
	virtual LTRESULT SetGlobalForce(LTVector* pVec)=0;   // 0x40

	uint8             m_pad_004[0x668 - 0x004];
	GlobalForceControl m_ForceControl;       // 0x668
};

// 0x100355fe — move an object through the world simulation.
void sm_MoveObject(IWorldSim* pWorld, HOBJECT hObj, LTVector* pPos, uint32 flags);

// 0x1001eb2d — the simulation's own move, which takes the position BY VALUE
// (twelve bytes onto the stack ahead of the flags). Name ours.
//
// void, not the `int` this was first declared as: eax is never set on any
// path. The flags word is a bit set, and every bit is read off a call site
// or a test in the body:
//
//   1 = drop whatever the object was standing on before moving it
//   2 = two notifications through the 0x6a8 object's slot 0 (2 on the way
//       in, then 2 and 0x200 on the way out)
//   4 = carry everything standing on this object with it
//   8 = move to the position the caller passed rather than the object's own
//
// The 7 that sm_MoveObject and wm_SetObjectDims pass is therefore the first
// three, and the recursion passes `(flags & 8) | 7`.
void wm_MoveObject(struct WMMoveState* pState, LTVector vPos, uint32 flags);

// One entry of the carry list wm_MoveObject builds: an object that was
// standing on the one being moved, and where it was standing relative to it.
// Sixteen bytes, and the walk caps the array at 0x20 of them (names ours).
struct CWMStandRec
{
	class LTObject* m_pObj;                  // 0x00
	LTVector        m_vOffset;               // 0x04
};

#define WM_MAX_STANDING 0x20

// The 0x24-byte record wm_MoveObject parks in the 0x6a8 object's
// current-request slot for the duration of the move, having saved whatever
// was there. Only its size (a memset of 0x24) is recoverable (name ours).
struct CWMRequestRec
{
	uint8 m_pad_00[0x24];
};

// 0x1001e85d — put an object onto another one, or onto nothing. ltobject.cpp
// reconstructs it (byte-identical) beside the two unlinkers servermgr.h
// declares as obj_UnlinkTrackersA (0x1001e7ef) and obj_UnlinkTrackersB
// (0x1001e828), which are what wm_MoveObject calls; it had no declaration of
// its own until this one, and servermgr.h is not ours to add it to (name
// ours, from ltobject.cpp).
void obj_SetStandingOn(class LTObject* pObj, class LTObject* pOn,
                       WorldPoly* pPoly);

// The two box tests wm_MoveObject shares with the resize walk (names ours);
// see commonphysics.cpp for what each reads off the shipped bytes.
int wm_BoxesOverlap(LTVector* pMin1, LTVector* pMax1,
                    LTVector* pMin2, LTVector* pMax2, float fTolerance);
int wm_ObjectsIntersect(class LTObject* pObj1, class LTObject* pObj2,
                        LTVector* pMin1, LTVector* pMax1,
                        LTVector* pMin2, LTVector* pMax2,
                        float fTolerance, int* pbUsedGeometry);
void wm_PushIntersectingObjects(struct WMMoveState* pState, LTVector* pOrigPos,
                                LTVector* pNewPos);

// How much of a move is small enough to be no move at all. The squared
// length is compared against this as a FLOAT (`fcomps`, so the literal in
// the source carries an `f`); the per-component clamp inside the body
// compares a `fabs` result against the DOUBLE 0.001 (`fcompl`). Both
// constants exist only in the shipped bytes.
#define WM_MOVE_EPSILON_SQR 0.0001f

// names ours: resize a world object through the simulation (0x10020611;
// nonzero = the dims were applied) and the "DebugMaxDims" console gate.
int wm_SetObjectDims(struct WMMoveState* pState, LTVector* pDims, int bFlag, int bUnknown);
extern float g_DebugMaxDims;                 // 0x10051d28

// 0x1002082b — try to grow the object along one axis (0 = x, 1 = y, 2 = z),
// given the address of the wanted component; it writes the component back with
// whatever it managed. Name ours.
void wm_ResizeAxis(struct WMMoveState* pState, int nAxis, float* pfWanted);

// 0x10020a2d — the six-field fill wm_ResizeAxis uses to prime its own request
// record: the four arguments into +0x00/+0x04/+0x08/+0x0c and zeros into
// +0x10/+0x14. It is a real out-of-line thiscall (`ret $0x10`), even though
// sm_MoveObject and SPhysicsLT::SetObjectDims write the same six fields
// inline — so the shipped source had both a helper and two places that did
// not use it. WMMoveState lives in servermgr.h, which this reconstruction
// does not own, so the record is cast to this to reach the method (names
// ours).
class CWMStateInit
{
public:
	void Init(void* pContainer, void* pUnknown04, class LTObject* pObj,
	          uint32 nPriority);            // 0x10020a2d

	// 0x1001efe8 — the other filler, for the recursive move: the container,
	// the source object and the blocking priority come straight off another
	// record, the object is the one being carried, and the two supplied-list
	// fields are cleared. `ret 8` with `this` in ecx, so it is a thiscall
	// member like the four-argument one above (name ours).
	void InitFrom(struct WMMoveState* pSrc, class LTObject* pObj);
};

// 0x1001eb0c — retransform the object if it is a world model and the flag is
// set. The type test is against 2 and 9, i.e. OT_WORLDMODEL and OT_CONTAINER
// (name ours).
void obj_MaybeRetransformWorldModel(class LTObject* pObj, int bFlag);

// Two thiscall entry points whose natural homes are headers this file does not
// own, declared as one-method casts instead. Neither name is attested.
//
// The 0x6a8 object as Setup and wm_MoveObject reach it: eleven virtual slots
// so slot 10 (+0x28) exists, dispatched through the abstract base so VC6
// cannot devirtualize. servermgr.h's CUnknown6A8Obj declares only slot 0 and
// is not a file this reconstruction owns, hence the stand-in (name ours).
// The argument counts are the ones wm_MoveObject's call sites push; none of
// the names is attested.
class IWMSetupSource
{
public:
	// 0x00 — a notification about one object, with a reason word: 2 on the
	// way into a move and again on the way out, then 0x200 when the move
	// went to the position the caller supplied.
	virtual void  _v00(class LTObject* pObj, uint32 nReason) = 0;
	// 0x04 — the slot holding the request record currently being served.
	// wm_MoveObject saves what is there, parks its own, and restores it.
	virtual void** _v04() = 0;
	// 0x08 — tell one object it is touching another, with the contact point
	// and the force behind it. 0x0c — tell one object it is inside a
	// container. Both are reached only from the contact notifier
	// (0x1001f51e), which sends each pair both ways round.
	virtual void  _v08(class LTObject* pObj, class LTObject* pOther,
	                   LTVector* pPoint, float fForce) = 0;
	virtual void  _v0c(class LTObject* pObj, class LTObject* pContainer) = 0;
	virtual void  _v10(class LTObject* pObj) = 0;
	virtual void  _v14(struct WMMoveState* pState) = 0;
	// Slot +0x18 takes THREE arguments and returns int, not void: both
	// 0x00451e10 and 0x004526f0 gate their whole body on its result, and
	// their esp traces only balance if it is `ret $0xc`. Corrected from
	// `void _v18()` when those two were reconstructed.
	virtual int   _v18(struct WMMoveState*, class LTObject*, class LTObject*) = 0;
	// 0x1c — one object could not be pushed clear of a moving world model and
	// is still inside it. wm_RotatePushObject is the only reconstructed
	// caller, and it reports the pair after the move has already been tried
	// (the argument names are ours).
	virtual void  _v1c(class LTObject* pObj, class LTObject* pMover) = 0;
	virtual void  _v20(struct WMMoveState* pState, LTVector* pPos) = 0;
	virtual void* _v24() = 0;                // slot 9
	// 0x28 — answers whether this object is one the move may simply place
	// rather than simulate.
	virtual int   _v28(class LTObject* pObj) = 0;
};

// 0x1001efd8 reads one thing out of the request record's 0x6a8 object through
// its vtable (slot 9) and caches it at record +0x18; WMMoveState in servermgr.h
// maps that offset as padding, so the record is cast to this.
class CWMMoveSetup
{
public:
	void Setup();                            // 0x1001efd8

	uint8           m_pad_00[0x04];
	IWMSetupSource* m_pUnknown04;            // 0x04  (WMMoveState::m_pUnknown04)
	uint8           m_pad_08[0x18 - 0x08];
	void*           m_Unknown18;             // 0x18
};

// 0x10024ae3 writes an object's m_Dims from a by-value vector and then calls
// Relocate (0x10024a4c) — the dims counterpart of ltobject_internal.h's
// SetPos, which that header does not carry. UpdateBounds is that same
// 0x10024a4c, declared here so the call can be written; ltobject_internal.h
// defines it on LTObject.
class CObjDimsSetter
{
public:
	void SetDims(LTVector vDims);            // 0x10024ae3
	void UpdateBounds();                     // 0x10024a4c

	uint8    m_pad_000[0x198];
	LTVector m_Dims;                         // 0x198
};

// 0x1001e6d7 — LTVector's three-float constructor: it stores the three
// arguments at this+0/4/8 and returns `this` in eax, which is the constructor
// convention, and the shipped build CALLS it. So the engine's own ltvector.h
// declared it in the class and defined it out of line, where the SDK copy we
// compile against defines it in the class body and /Ob1 expands it away. This
// same-layout stand-in is how the call is kept (name ours). Note that the
// vector *operators* still inline, exactly as they do in the shipped code —
// only the direct construction is a call.
class CVectorInit
{
public:
	CVectorInit(float x, float y, float z);  // 0x1001e6d7

	float x, y, z;
};


// Derived from the SDK interface so the vtable layout is iltphysics.h's
// declaration order — which it must be: CLTServer's old-API wrappers call
// through these slots by number (SetVelocity is slot 5, MoveObject slot 12,
// GetGlobalForce slot 15), and ILTPhysics's protected m_ClientServerType
// behind the vptr is what puts SPhysicsLT's own fields at +0x08.
class CommonPhysicsLT : public ILTPhysics
{
public:
	LTRESULT GetFrictionCoefficient(HOBJECT hObj, float& coeff);
	LTRESULT SetFrictionCoefficient(HOBJECT hObj, float coeff);

	LTRESULT GetForceIgnoreLimit(HOBJECT hObj, float& limit);
	LTRESULT SetForceIgnoreLimit(HOBJECT hObj, float limit);

	LTRESULT GetVelocity(HOBJECT hObj, LTVector* pVel);
	LTRESULT GetAcceleration(HOBJECT hObj, LTVector* pAccel);

	LTRESULT GetObjectMass(HOBJECT hObj, float& mass);
	LTRESULT SetObjectMass(HOBJECT hObj, float mass);

	LTRESULT GetStandingOn(HOBJECT hObj, CollisionInfo* pInfo);
	LTRESULT GetObjectDims(HOBJECT hObj, LTVector* pDims);
	LTRESULT IsWorldObject(HOBJECT hObj);
};


// The server's half of ILTPhysics (0x1002d472-0x1002d5a8, 0x1002f0e1-0x1002f172).
class SPhysicsLT : public CommonPhysicsLT
{
public:
	// Defined `inline` in commonphysics.cpp, the one TU that constructs one:
	// CLTServer's constructor expands it (0x1002d3d7..0x1002d3e7).
	SPhysicsLT(class CServerMgr* pServerMgr);

	LTRESULT SetVelocity(HOBJECT hObj, LTVector* pVel);
	LTRESULT SetAcceleration(HOBJECT hObj, LTVector* pAccel);
	LTRESULT SetObjectDims(HOBJECT hObj, LTVector* pNewDims, uint32 flags);  // 0x1002d5a9, not yet reconstructed
	LTRESULT MoveObject(HOBJECT hObj, LTVector* pPos, uint32 flags);

	// iltphysics.h's own tail, in its declaration order — named from the
	// class's vtable at 0x1004e560+. References,
	// so no null checks are possible, and none are compiled.
	LTRESULT GetGlobalForce(LTVector& vec);
	LTRESULT SetGlobalForce(LTVector& vec);
	LTRESULT GetStairHeight(float& fHeight);
	LTRESULT SetStairHeight(float fHeight);

public:
	float      m_StairHeight;                // 0x08 (m_ClientServerType fills 0x04)
	IWorldSim* m_pWorld;                     // 0x0c
};

#endif
