// *********************************************************************** //
//
//  MODULE:   W11_SINGLETONS (.CPP)  -- scratch file, NOT a drop-in TU
//
//  ROWS:     two "large isolated functions", 0x00461550 (1090) and
//            0x00473bc0 (947).
//
//  RESULT:   ****  NEITHER ROW NEEDS A BODY.  ****  One is already
//            written; the other is not a function at all.  What does is
//            the function the phantom row lives INSIDE, and that is what
//            this file reconstructs.
//
//  ==================== ROW-BY-ROW VERDICT, BEFORE ANY CODE ==============
//
//   0x00473bc0  947   ALREADY WRITTEN, AND LOUDLY SO.
//                     It is `CServerMgr::UpdateObjects`, and the body is at
//                     src/engine/server/smgr.cpp:1326 with a 20-line
//                     near-miss note above it.  servermgr.h:1887 declares it
//                     as `void UpdateObjects();  // 0x10032854`.
//                     WHY IT LOOKED UNWRITTEN: smgr.cpp is a SHARED TU and
//                     is annotated with its server.dll address (0x10032854)
//                     everywhere, so no 0x004xxxxx citation for this
//                     function exists in the tree for an address search to find.
//                     This is EXACTLY the 0x0046c0f0 case's shape.
//                     Evidence chain, all independent:
//                       * smgr.obj's ?UpdateObjects@CServerMgr@@QAEXXZ is
//                         already compiled, 984 bytes against the shipped
//                         947, unplaced, so it is not counted as a near.
//                       * the server.dll twin is a near:
//                         `867 866 250/251 5 STORE ?UpdateObjects@CServerMgr@@QAEXXZ`.
//                       * the /O1 oracle is sub_10032854, 868 bytes, status
//                         `near`.
//                     EXTENT: sound.  0x00473bc0..0x00473f72 inclusive, 947
//                     bytes, `ret` at 0x00473f72 then thirteen `nop` to
//                     0x00473f80.  Its only inbound `e8`/`e9` from outside
//                     the range is the real call at 0x00474491, inside
//                     `CServerMgr::Update` (0x004742a0, 698 bytes) -- which
//                     is where servermgr.h says UpdateObjects is called
//                     from.  Nothing else enters it.
//
//   0x00461550  1090  NOT A FUNCTION.  FALSE CALL TARGET INSIDE A MEMORY
//                     DISPLACEMENT -- the 0x004478fd shape,
//                     exactly.
//                     The row's sole "caller" is 0x0049c7c8, and 0x0049c7c8
//                     is NOT an instruction boundary: the instruction there
//                     is `mov -0x18(%ebp),%esp` at 0x0049c7c6, bytes
//                     `8b 65 e8`, inside a CRT SEH unwind frame.  The `e8`
//                     of the `-0x18` DISPLACEMENT is read as a call opcode
//                     and the three bytes that follow it are the next
//                     instruction's `83 4d fc` (`orl $-1,-0x4(%ebp)`),
//                     giving rel32 = 0xfffc4d83 and a phantom target
//                     0x0049c7cd - 0x3b27d = 0x00461550.
//                     0x00461550 is itself mid-instruction as well: the
//                     instruction at 0x0046154c is
//                     `8b 8f 8c 01 00 00` = `mov 0x18c(%edi),%ecx`, and
//                     0x00461550 is the third byte of the 0x18c
//                     displacement.
//                     THE OWNING FUNCTION IS 0x00461270, it is 1,826 bytes
//                     (0x00461270..0x00461991, `ret` at 0x00461991, fourteen
//                     `nop` to 0x004619a0), and IT IS GENUINELY UNWRITTEN.
//                     src/engine/lithtech/clientshell.cpp:156 declares it
//                     -- `void sub_461270(CClientShell* pShell);` -- and
//                     calls it at line 1287, and NOTHING defines it: the
//                     prototype-declared-never-defined signal.
//                     src/engine/lithtech/predobj.cpp
//                     reconstructs 0x00461120, 0x00460db0 and 0x004619a0 of
//                     the same TU and explicitly does NOT cover 0x00461270.
//                     So the row is a phantom but it is standing over real
//                     work, and that work is written out below.
//
//  ==================== WHAT IS BELOW ====================================
//
//   0x00461270  1826  NEW.  `sub_461270` -- the client's once-per-frame
//                     dead-reckoning tick: it advances every predicted
//                     object's position and every predicted object's
//                     rotation, and draws the three debug lines the
//                     `PredictionLines` console variable turns on.
//                     __cdecl, one argument, void.
//
//  GROUND TRUTH: the retail lithtech.exe ONLY, cl 8447 /O2.  Every offset,
//  constant and branch below is cited by the address it was read at.  There
//  is NO /O1 oracle for this function: it is client-only (CClientShell, the
//  client console variables and CPhysicsLT), and nothing of this shape is in
//  server.dll.
//
//  NAMING: every symbol introduced here carries a Q1S suffix.  The class
//  layouts are src/engine/lithtech/predobj.cpp's, renamed, so that the two
//  files can be merged into one TU by deleting this file's copies.
//
//  HOW THE MODULE WAS IDENTIFIED
//  --------------------------------------------------------------
//  1. OBJECT ATTRIBUTION FIRST.  0x00461270 falls in the
//     hole between fmconvert.obj/lt2new2.obj (ends 0x0045f365) and
//     clientrender.obj (0x004623d0) in the decoded object map, i.e.
//     no object claims it.  So instrument 1 was silent; on to 2.
//  2. THE CONSOLE-VARIABLE TABLE settled it, and predobj.cpp had already
//     read the same three records:
//         0x004aa274  { "Prediction",         0, ->0x004b32bc }
//         0x004aa288  { "PredictionLines",    0, ->0x004b32c0 }
//         0x004aa65c  { "MaxExtrapolateTime", ->0x004a9ef8, 0 }  (0.5f)
//     This function reads 0x004b32bc at 0x0046128e, 0x004b32c0 at
//     0x00461696, and 0x004a9ef8 at EIGHT sites -- 0x0046137a, 0x00461387,
//     0x004613cb, 0x004613e6, 0x004613ec, 0x0046151f, 0x004614d0,
//     0x00461795.  predobj.cpp's header note says "0x004a9ef8
//     (MaxExtrapolateTime) is read at eight sites in
//     0x00461270..0x004619a0"; ALL EIGHT ARE IN THIS FUNCTION, which
//     independently confirms both the TU and the extent.
//  3. The call graph agrees: the only callees outside the CRT are
//     0x004619a0 (predobj.cpp section C, the extrapolator), 0x004207a0 and
//     0x00420880 (predobj.cpp's two setters), 0x00444b20 (quat_Slerp) and
//     0x0043d1c0 (ci_AddLine).  Nothing else.
//
//  EXTENT PROOF FOR 0x00461270 (done both ways)
//  ---------------------------------------------------------------
//    * START.  0x00461270 is preceded by `ret` at 0x0046126f and opens
//      `sub $0x110,%esp ; push %ebx ; push %ebp`.  Instruction boundary,
//      real prologue.
//    * END.  One `ret`, at 0x00461991, behind
//      `pop %edi/%esi/%ebp/%ebx ; add $0x110,%esp`, mirroring the prologue
//      exactly.  Fourteen `nop` follow to 0x004619a0, which is predobj.cpp's
//      section C.
//    * SWEEP.  Every `e8`/`e9` in .text whose target lands strictly inside
//      0x00461270..0x00461991: there is exactly ONE, and it is the phantom
//      at 0x0049c7c8 dissected above.  No real function-entry, no jump in
//      from anywhere else.  It is ONE function.
//    * FRAME CHECKSUM, and it closes to the byte -- see the block above the
//      body.
//
//  CONFIDENCE.  HIGH on structure, call sequence, every offset and every
//  constant: the frame decomposes with zero slack (see below), which is
//  the strongest instrument for a function this call-sparse, and
//  every one of the 8 compiler temporaries the source predicts is present
//  and no others.  MEDIUM-LOW on byte identity: 1,826 bytes of /O2 x87 is
//  not a size band that has ever come out exact here, and three coin-flips
//  are listed at the foot.  Direct-call density is 1,826 / 8 real
//  call sites = 228 B/call with SIX DISTINCT callees, which is
//  in the middle band.
//
// *********************************************************************** //

#include "ltbasedefs.h"        // LTVector, LTRotation, LTBOOL, FLAG_*
#include "ltlink.h"            // LTLink / CheapLTLink::Remove / TieOff
#include "iltclient.h"         // LTLine, LTLinePt, HLTLINE, HLOCALOBJ


// ======================================================================= //
//
//  THE CLASSES THIS FILE DOES NOT OWN
//
//  All of these are src/engine/lithtech/predobj.cpp's, renamed Q1S, except
//  where a comment says the field is new here.
//
// ======================================================================= //

class LTObjectQ1S;


// ----------------------------------------------------------------------- //
//  ILTPhysics, the client's half.  Two slots, and BOTH are already named in
//  the tree by address, so neither is a guess:
//
//    slot 12 (0x30) -> 0x00404e10, which src/engine/lithtech/clientphysics.h
//                      line 32 declares as
//                      `CPhysicsLT::MoveObject(HOBJECT, LTVector*, uint32)`.
//                      Called at 0x0046166b with three stack arguments and
//                      no caller cleanup.
//    slot 15 (0x3c) -> 0x00405160, which src/engine/lithtech/lt2new1.cpp
//                      line 353 declares as
//                      `CPhysicsLT::GetGlobalForce(LTVector&)` and says is
//                      ILTPhysics index 15.  Called at 0x004612df with one
//                      stack argument and no caller cleanup.
//
//  Both slots read straight off the ILTPhysics vtable at 0x004a1770:
//  +0x30 = 0x00404e10, +0x3c = 0x00405160.
// ----------------------------------------------------------------------- //
class ILTPhysicsQ1S
{
public:
	virtual void _v000()=0;
	virtual void _v004()=0;
	virtual void _v008()=0;
	virtual void _v00c()=0;
	virtual void _v010()=0;
	virtual void _v014()=0;
	virtual void _v018()=0;
	virtual void _v01c()=0;
	virtual void _v020()=0;
	virtual void _v024()=0;
	virtual void _v028()=0;
	virtual void _v02c()=0;
	virtual LTRESULT MoveObject(LTObjectQ1S* hObj, LTVector* pPos,
	                            uint32 flags)=0;             // 0x30 -> 0x00404e10
	virtual void _v034()=0;
	virtual void _v038()=0;
	virtual LTRESULT GetGlobalForce(LTVector& vec)=0;        // 0x3c -> 0x00405160
};


// ----------------------------------------------------------------------- //
//  CLTClient, the client's flat interface table
//  (src/engine/lithtech/clientinterface.h).  Its first 0x20 bytes are the
//  sub-interface pointers, and src/engine/lithtech/cltapi.cpp line 765
//  already names +0x0c `ILTPhysics* m_pPhysicsLT`.  0x004612d4
//  (`mov 0x4c0(%ecx),%ecx ; mov 0xc(%ecx),%ecx`) is that chain.
// ----------------------------------------------------------------------- //
class CLTClientQ1S
{
public:
	uint8           m_pad_00[0x0c];
	ILTPhysicsQ1S*  m_pPhysicsLT;                            // 0x0c
};


// ----------------------------------------------------------------------- //
//  IClientShell -- predobj.cpp's, unchanged.  OnObjectMove is slot 15
//  (0x3c, called at 0x00461638) and OnObjectRotate slot 16 (0x40, called at
//  0x00461904); both __thiscall, three stack arguments, callee cleanup.
// ----------------------------------------------------------------------- //
class IClientShellQ1S
{
public:
	virtual void _v000()=0;
	virtual void _v004()=0;
	virtual void _v008()=0;
	virtual void _v00c()=0;
	virtual void _v010()=0;
	virtual void _v014()=0;
	virtual void _v018()=0;
	virtual void _v01c()=0;
	virtual void _v020()=0;
	virtual void _v024()=0;
	virtual void _v028()=0;
	virtual void _v02c()=0;
	virtual void _v030()=0;
	virtual void _v034()=0;
	virtual void _v038()=0;
	virtual LTRESULT OnObjectMove(LTObjectQ1S* hObj, LTBOOL bTeleport,
	                              LTVector* pNewPos)=0;      // 0x3c
	virtual LTRESULT OnObjectRotate(LTObjectQ1S* hObj, LTBOOL bTeleport,
	                                LTRotation* pNewRot)=0;  // 0x40
};


// ----------------------------------------------------------------------- //
//  CClientMgr.  predobj.cpp's +0x4c0 and +0x61c, plus one field this
//  function adds: +0x125c, which src/engine/lithtech/cltrest.cpp line 384
//  already names `float m_Time` (ILTCSBase slot 53, GetTime).  Read here at
//  0x0046129b as `flds 0x125c(%ecx)`.
// ----------------------------------------------------------------------- //
class CClientMgrQ1S
{
public:
	uint8             m_pad_000[0x4c0];
	CLTClientQ1S*     m_pClientDE;                           // 0x4c0
	uint8             m_pad_4c4[0x61c - 0x4c4];
	IClientShellQ1S*  m_pClientShell;                        // 0x61c
	uint8             m_pad_620[0x125c - 0x620];
	float             m_Time;                                // 0x125c
};


// ----------------------------------------------------------------------- //
//  CClientShell.  predobj.cpp's layout; this function adds +0x2c.
//
//  +0x28 is the prediction clock predobj.cpp already found 0x00460d80
//  writing, and 0x004612c3/0x004612ce accumulate into it here.
//  +0x2c is the LAST value of CClientMgr::m_Time -- 0x004612a3 subtracts it
//  from the freshly-read m_Time and 0x004612d1 writes m_Time back into it.
//  src/engine/lithtech/lt2new3.cpp had already observed a dword copied from
//  m_pClientMgr+0x125c into +0x2c and spelled it `uint32`; the `fsubs
//  0x2c(%ebp)` here proves it is a FLOAT, and lt2new3.cpp's own footnote
//  says either spelling is safe.
// ----------------------------------------------------------------------- //
class CClientShellQ1S
{
public:
	uint8            m_pad_000[0x24];
	float            m_fPredictScale24;                      // 0x024 (unused here)
	float            m_fTime28;                              // 0x028
	float            m_fLastTime2c;                          // 0x02c
	uint8            m_pad_030[0x44 - 0x30];
	LTLink           m_PosPredictList;                       // 0x044
	LTLink           m_RotPredictList;                       // 0x050
	uint8            m_pad_05c[0x78 - 0x5c];
	CClientMgrQ1S*   m_pClientMgr;                           // 0x078
};


// ----------------------------------------------------------------------- //
//  LTObject.  predobj.cpp's prediction record verbatim, plus m_Rotation at
//  +0x09c, which src/engine/server/ltobject_internal.h line 269 already
//  names (`LTRotation m_Rotation; // 0x09c  quaternion`) and which
//  0x004618d8 / 0x00461939 read here.
// ----------------------------------------------------------------------- //
class LTObjectQ1S
{
public:
	uint8      m_pad_000[0x88];
	uint32     m_Flags;                                      // 0x088
	uint8      m_pad_08c[0x9c - 0x8c];
	LTRotation m_Rotation;                                   // 0x09c
	uint8      m_pad_0ac[0xc4 - 0xac];
	LTVector   m_Velocity;                                   // 0x0c4
	LTVector   m_Acceleration;                               // 0x0d0
	float      m_FrictionCoefficient;                        // 0x0dc
	uint8      m_pad_0e0[0x120 - 0xe0];
	uint32     m_ChangeFlags;                                // 0x120
	float      m_fPosPredictTime;                            // 0x124
	float      m_fPredictStamp;                              // 0x128
	LTVector   m_vPredictPos;                                // 0x12c
	LTVector   m_vPredictVel;                                // 0x138
	LTVector   m_vPredictBase;                               // 0x144
	LTLink     m_PosPredictLink;                             // 0x150
	float      m_fRotPredictTime;                            // 0x15c
	LTRotation m_PredictRot;                                 // 0x160
	LTLink     m_RotPredictLink;                             // 0x170
	HLOCALOBJ  m_hPredictLines;                              // 0x17c
	uint8      m_pad_180[0x18c - 0x180];
	LTVector   m_Pos;                                        // 0x18c
};


// predobj.cpp's SPredictRec, the argument type of 0x004619a0: it IS
// LTObject+0x124.  0x00461407 and 0x00461511 both push `pObj + 0x124`.
class SPredictRecQ1S
{
public:
	float    m_fPredictTime;                                 // 0x00 (+0x124)
	float    m_fPredictStamp;                                // 0x04 (+0x128)
	LTVector m_vPos;                                         // 0x08 (+0x12c)
	LTVector m_vVel;                                         // 0x14 (+0x138)
	uint8    m_pad_20[0x58 - 0x20];
	// 0x0046168b / 0x00461733 / 0x00461837 read +0x17c through the record
	// pointer, so the record runs at least to the debug-line handle.
	HLOCALOBJ m_hPredictLines;                               // 0x58 (+0x17c)
};


// ----------------------------------------------------------------------- //
//  The console variables and the module tunable.  0x004b32bc and 0x004b32c0
//  are predobj.cpp's already; 0x004a9ef8 is MaxExtrapolateTime's storage,
//  read at the eight sites listed in the header block and NOWHERE ELSE in
//  the image, so this function owns the symbol.
// ----------------------------------------------------------------------- //
//  All three are the console bindings src/engine/server/dsi.cpp defines for
//  lithtech.exe ("Prediction", "PredictionLines", "MaxExtrapolateTime" --
//  the client's table lays them out as server.dll's), so they are declared
//  under dsi.cpp's names and types.
extern int   g_CV_Prediction;                                // 0x004b32bc
extern int   g_CV_PredictionLines;                           // 0x004b32c0
extern float g_CV_MaxExtrapolateTime;                        // 0x004a9ef8  (0.5f)
#define g_CV_PredictionQ1S       g_CV_Prediction
#define g_CV_PredictionLinesQ1S  g_CV_PredictionLines
#define g_fMaxExtrapolateTimeQ1S g_CV_MaxExtrapolateTime


// ----------------------------------------------------------------------- //
//  The five callees, all already named elsewhere in the tree by address.
// ----------------------------------------------------------------------- //

// predobj.cpp section C: extrapolate a predicted object forward, returning
// the offset-from-base position BY VALUE.  Seven pushed dwords at both call
// sites (0x00461407, 0x00461511), `add $0x1c,%esp`, result read back
// through %eax.
//  Defined there as sub_4619a0Q460 over its own view classes.
class LTObjectQ460;
class SPredictRecQ460;
LTVector sub_4619a0Q460(LTObjectQ460* pObj, SPredictRecQ460* pRec,
                        LTVector* pForce, float fTime, float fMaxTime,
                        int bUsePhysics);                    // 0x004619a0
#define sub_4619a0Q1S(pObj, pRec, pForce, fTime, fMaxTime, bUsePhysics) \
	sub_4619a0Q460((LTObjectQ460*)(pObj), (SPredictRecQ460*)(pRec),     \
	               (pForce), (fTime), (fMaxTime), (bUsePhysics))

// predobj.cpp / clientmgr.h line 237 and 543.
//  Defined in clienttexmgr.cpp as cm_SetObjectPos / cm_SetObjectRotation
//  over its own view classes.
class CClientMgrObjView;
class CObjSetView;
void cm_SetObjectPos(CClientMgrObjView* pClientMgr, CObjSetView* hObj,
                     LTVector* pPos, int nUnknown);          // 0x004207a0
void cm_SetObjectRotation(CClientMgrObjView* pClientMgr, CObjSetView* hObj,
                          LTRotation* pRotation);            // 0x00420880
#define sub_4207a0Q1S(pClientMgr, hObj, pPos, nUnknown) \
	cm_SetObjectPos((CClientMgrObjView*)(pClientMgr), (CObjSetView*)(hObj), \
	                (pPos), (nUnknown))
#define sub_420880Q1S(pClientMgr, hObj, pRotation) \
	cm_SetObjectRotation((CClientMgrObjView*)(pClientMgr), \
	                     (CObjSetView*)(hObj), (pRotation))

// src/engine/lithtech/transform.cpp already pins 0x00444b20 as
// `quat_Slerp(float*, const float*, const float*, float)` -- four __cdecl
// arguments, `add $0x10,%esp`, the float pushed through the `push %ecx ;
// fstps (%esp)` idiom, which is exactly 0x004618d1/0x004618e5 here.
void quat_Slerp(float* pDest, const float* pQ1, const float* pQ2,
                float t);                                    // 0x00444b20  (commonmodel.cpp)
#define quat_SlerpQ1S quat_Slerp

// src/engine/lithtech/clientinterface.h line 432, verbatim.  Called three
// times (0x00461717, 0x00461790, 0x00461864) with the cdecl cleanup for all
// three DEFERRED into one `add $0x18,%esp` at 0x00461869 -- the
// deferred-cleanup shape, and the arithmetic (3 x 8 = 0x18) is what proves
// there are exactly three calls and two arguments each.
HLTLINE ci_AddLine(HLOCALOBJ hObj, LTLine* pLine);           // 0x0043d1c0  (clientinterface.cpp)
#define ci_AddLineQ1S ci_AddLine


// ======================================================================= //
//
//  0x00461270 (1826 bytes) -- the client's per-frame dead-reckoning tick
//
//  __cdecl, one argument, void.  src/engine/lithtech/clientshell.cpp:1287
//  calls it as `sub_461270(this)` from CClientShell::Update, gated on
//  dsi_IsClientActive, and line 156 of that file declares it and never
//  defines it.  The prologue confirms the shape: `sub $0x110,%esp` then
//  four pushes, and 0x00461278 reads the argument at `0x11c(%esp)` while
//  two pushes have happened -- S+4, one dword, cdecl.
//
//  ------------------------------------------------------------------- //
//  FRAME CHECKSUM.  It closes to the byte with NO SLACK, which for a
//  function this call-sparse is the load-bearing evidence.
//
//  `sub $0x110,%esp` comes FIRST and the four register pushes after it, so
//  the saved registers sit BELOW the locals: with E = %esp after the
//  prologue, E+0x00..0x0f are edi/esi/ebp/ebx and the 0x110 local block is
//  E+0x10..E+0x11f, with the argument at E+0x124.  (Get this backwards and
//  every offset below moves by 0x10 and the frame stops closing -- that is
//  how the layout was found.)
//
//   E+0x010  float     fDT   (position loop) / fStep (rotation loop)
//   E+0x014  LTVector  vNewPos
//   E+0x020  LTLink*   pCur  (both loops)
//   E+0x024  float     fFrameTime
//   E+0x028  LTLine    line                 56 bytes, 0x028..0x05f
//   E+0x060  LTBOOL    bStopped
//   E+0x064  IClientShellQ1S* pShellIface
//   E+0x068  LTVector  vForce
//   E+0x074  temp   operator*  result, arm 2's average        (0x074/078/07c)
//   E+0x080  temp   operator+  result, the blue line          (0x080/084/088)
//   E+0x08c  temp   operator*  result, arm 1's average        (0x08c/090/094)
//   E+0x098  temp   operator+'s BY-VALUE argument, arm 2      (0x098/09c/0a0)
//   E+0x0a4  temp   operator+'s BY-VALUE argument, arm 1      (0x0a4/0a8/0ac)
//   E+0x0b0  LTVector  vOldPos     (.x lives in %ebp; .y 0x0b4, .z 0x0b8)
//   E+0x0bc  LTVector  vZeroForce
//   E+0x0c8  temp   operator*  result, m_vPredictVel * fMax   (.z elided)
//   E+0x0d4  temp   operator+  result, arm 1's sum            (.x elided)
//   E+0x0e0  LTVector  vGlobalForce
//   E+0x0ec  temp   operator+  result, arm 2's sum            (.x elided)
//   E+0x0f8  LTRotation rNew                                  16 bytes
//   E+0x108  LTVector  <hidden return buffer, call site 1>    0x004613f9
//   E+0x114  LTVector  <hidden return buffer, call site 2>    0x004614e7
//
//  0x114 + 12 = 0x120.  Nothing is left over and nothing overlaps.
//
//  AND THE TEMPORARY COUNT IS EXACT.  The source below contains three
//  `_CVector::operator+` and three `_CVector::operator*` evaluations.
//  ltvector.h's `operator+(const _CVector<T> v)` takes its argument BY
//  VALUE, so each `+` whose right operand is a memory object needs a copy
//  temp as well as a result temp; each `+` whose right operand is ALREADY a
//  temp needs only the result.  That predicts 3 result temps for `*`, 3
//  result temps for `+`, and 2 argument copies (the two `+ pObj->m_Pos`) --
//  EIGHT, and there are exactly eight, at the eight slots above.  A ninth
//  expression, or one written the other way round, would move this row.
//  ------------------------------------------------------------------- //
//
//  WHAT PINS THE SHAPE
//
//   * ONE `ret`, at 0x00461991.  The top guard's `je 0x00461987`
//     (0x00461295), the position list's empty test `je 0x00461884`
//     (0x004612ea) and the rotation list's `je 0x00461987` (0x0046188c) all
//     converge on it.  One shared exit with several
//     producers means POSITIVE GUARDS THAT NEST, which is why the whole
//     body is inside `if (g_CV_Prediction)` rather than an early return.
//
//   * BOTH LOOPS ARE ROTATED `while`s over an LTLink ring, not `for`s.  The
//     head is tested once before entry (0x004612e8, 0x0046188a) and again at
//     the bottom (0x0046187c, 0x00461981), and the `jmp` at 0x004612f0 /
//     0x00461892 skips the latch block on the first iteration.  The body
//     reads `pCur->m_pData` and then advances `pCur = pCur->m_pNext` BEFORE
//     doing anything else (0x004612fd/0x00461300, 0x00461898/0x0046189b),
//     which is what lets the body unlink the object it is holding.
//
//   * THE x87 CONDITIONS, all read with `-M intel` and cross-checked:
//     `test $0x01,%ah` is C0 alone (`<`), `test $0x41,%ah` is C0|C3 (`<=`),
//     and MSVC branches to the FALSE arm.  So
//       0x004612b5 `test $1 ; je`   -> `if (fFrameTime < 0.0f)`
//       0x0046136a `test $1 ; je`   -> `if (fDT < 0.0f)`
//       0x00461385 `test $41 ; jne` -> `else if (fDT > fMax)`
//       0x004613c5 `test $41 ; jne` -> `if (vel.Mag() > 0.0f`
//       0x004613dc `test $41 ; jne` -> `    && fMax > 0.0f)`
//       0x0046153c `test $41 ; je`  -> `if (fDT <= fMax)`
//       0x004618b1 `test $1 ; je`   -> `if (m_fRotPredictTime < fFrameTime)`
//       0x00461937 `test $41 ; je`  -> `if (m_fRotPredictTime <= 0.0f)`
//
//   * A SHIPPED BUG, REPRODUCED AS SHIPPED.  fDT is clamped to at most
//     g_fMaxExtrapolateTime at 0x00461387, and 0x0046151f then asks whether
//     fDT is greater than g_fMaxExtrapolateTime.  It never is, so the
//     `else` at 0x004615de -- the one that unlinks the object and raises
//     bStopped -- is DEAD CODE in the shipped binary.  Both compares read
//     the same slot (E+0x010) and the same global (0x004a9ef8), and nothing
//     writes either between them, so this is not a misread: it is a
//     defensive test the clamp above it defeats.  It is written out below
//     because the bytes are there and removing it would lose 44 of them.
//
//   * `pObj->m_fRotPredictTime` is RE-READ at 0x004618b3 after being
//     compared at 0x004618a2, and again at 0x004618cb and 0x00461919.  Four
//     loads of one member: that is four written-out
//     uses, not one cached local.
//
//   * SIX link stores in each stopping arm (0x004615f8..0x0046160a and
//     0x0046195f..0x00461976) -- `Remove()`, which already ties off, and
//     then a SECOND explicit `TieOff()`.  predobj.cpp's section A found the
//     same six-store signature in the same TU.
//
//   * THE THREE DEBUG LINES.  0x0043d1c0 is ci_AddLine and iltclient.h's
//     `LTLine` is two 28-byte `LTLinePt` -- 56 bytes, which is exactly the
//     E+0x028..0x05f block.  The colours are immediates, so there is no
//     ambiguity at all: line one is (1,0,0,1) RED, line two (0,1,0,1)
//     GREEN, line three (0,0,1,1) BLUE.
//
//   * The `1.1f` at 0x004acef8 that predobj.cpp's other two sections use is
//     NOT read here; the only float literals this function loads are the
//     0.0f at 0x004a14d8 (comparisons) and the 0.5f at 0x004a1a74 (the two
//     averages).  A multiply by 2.0f would have been `fadd %st,%st(0)` with
//     no literal at all, so 0x004a1a74's 0.5f is a real halving.
//
// ======================================================================= //
// A by-value float min (name ours; stand-in for whatever LTMIN-like helper
// the original used). Through a by-value inline the taken arm copies the
// member through the FPU (`fstps` at 0x004618b9) as shipped, where the
// written-out if/else and a plain ternary both copy with an integer `mov`;
// the edx/eax swaps later in the rotation loop also go away with it
// (12 -> 5 mnemonic blocks). Still open: shipped pops the compared value
// (`fcomps`) and reloads the member (0x004618b3), ours keeps it (`fcoms`).
// Reference parameters (either or both) bring back the 12-block `mov` form.
inline float pt_MinQ1S(float a, float b)
{
	return (a < b) ? a : b;
}

void sub_461270Q1S(CClientShellQ1S* pShell)
{
	// 0x00461281 / 0x00461284 / 0x0046128a -- loaded before the first
	// branch and spilled to E+0x064, then re-read at 0x00461620 and
	// 0x004618f0.  One local, two uses.
	IClientShellQ1S* pShellIface = pShell->m_pClientMgr->m_pClientShell;

	// 0x0046128e `mov 0x4b32bc,%eax ; test %eax,%eax ; je 0x00461987`.
	if (g_CV_PredictionQ1S)
	{
		LTLink*   pCur;
		LTObjectQ1S* pObj;
		// Declared here, not in the rotation loop: block-scoped, rNew shared
		// its slot with the second call's hidden return buffer and the frame
		// came out 0xc short (0x104 vs the shipped 0x110).
		LTRotation rNew;
		float      fStep;
		SPredictRecQ1S* pRec;
		LTVector  vGlobalForce;

		// 0x0046129b `flds 0x125c(%ecx)` is loaded ONCE and kept in %st
		// across the clamp below (0x004612a1 `fld %st(0)`), then stored to
		// +0x2c at 0x004612d1 -- so it is a named local read twice, not two
		// member reads.
		float fTime      = pShell->m_pClientMgr->m_Time;
		float fFrameTime = fTime - pShell->m_fLastTime2c;   // 0x004612a3

		if (fFrameTime < 0.0f)                              // 0x004612aa
			fFrameTime = 0.0f;                              // 0x004612b7

		pShell->m_fTime28    += fFrameTime;                 // 0x004612c3/0x004612ce
		pShell->m_fLastTime2c = fTime;                      // 0x004612d1

		// 0x004612d4..0x004612df.  Virtual, one argument, callee cleanup.
		pShell->m_pClientMgr->m_pClientDE->m_pPhysicsLT->
			GetGlobalForce(vGlobalForce);

		// --------------------------------------------------------------- //
		//  THE POSITION LIST -- 0x004612e2 .. 0x00461882
		// --------------------------------------------------------------- //
		pCur = pShell->m_PosPredictList.m_pNext;            // 0x004612e2
		while (pCur != &pShell->m_PosPredictList)           // 0x004612e5/0x0046187a
		{
			LTVector vForce, vNewPos, vOldPos;
			LTBOOL   bStopped;
			float    fDT;

			pObj = (LTObjectQ1S*)pCur->m_pData;             // 0x004612fd
			pCur = pCur->m_pNext;                           // 0x00461300

			// 0x0046130d `lea 0x124(%edi),%esi`: the record pointer is a
			// named local, read through at 0x00461358 and 0x00461391 and
			// pushed at both call sites.
			pRec = (SPredictRecQ1S*)&pObj->m_fPosPredictTime;

			// 0x00461307 `test $0x20000,%eax` -- FLAG_GRAVITY is 1<<17,
			// and src/engine/server/commonphysics.h already reads bit 17 of
			// this same dword as FLAG_GRAVITY.
			if (pObj->m_Flags & FLAG_GRAVITY)
			{
				// 0x0046131a..0x00461337: three integer dword pairs, i.e.
				// the whole-vector assignment, not three float stores.
				vForce = vGlobalForce;
			}
			else
			{
				// 0x0046133d..0x00461354: three `movl $0x0` and no call.
				// predobj.cpp records that VC6 declines to inline
				// `_CVector::Init` (it calls 0x0041ab50), so these cannot
				// be an Init() -- they are component assignments.
				vForce.x = 0.0f;
				vForce.y = 0.0f;
				vForce.z = 0.0f;
			}

			// 0x00461355 / 0x00461358 / 0x0046135b.
			fDT = pShell->m_fTime28 - pRec->m_fPredictStamp;
			if (fDT < 0.0f)                                 // 0x0046135f
				fDT = 0.0f;                                 // 0x0046136c
			else if (fDT > g_fMaxExtrapolateTimeQ1S)        // 0x0046137a
				fDT = g_fMaxExtrapolateTimeQ1S;             // 0x00461387

			bStopped = LTFALSE;                             // 0x004613a2

			// 0x00461391..0x004613b2 is Mag() expanded in place --
			// three `flds` off +0x138/+0x13c/+0x140, x*x + y*y + z*z,
			// `fsqrt`, no call.  0x004613ba compares it with 0.0f and
			// 0x004613d1 compares the global with 0.0f; both `jne` to the
			// same else at 0x004614d0, which is the `&&`.
			if (pRec->m_vVel.Mag() > 0.0f &&
			    g_fMaxExtrapolateTimeQ1S > 0.0f)
			{
				// 0x004613e2..0x00461407.  Seven pushes: the hidden buffer
				// (E+0x108) last-pushed and therefore leftmost, then pObj,
				// then pObj+0x124, then &vForce, then fDT/fMax, then fMax,
				// then 1.  `add $0x1c,%esp` = 7 dwords.
				vNewPos = sub_4619a0Q1S(
					pObj, pRec,
					&vForce, fDT / g_fMaxExtrapolateTimeQ1S,
					g_fMaxExtrapolateTimeQ1S, 1);

				// 0x00461415..0x004614cb.  m_Pos is copied twelve bytes to
				// E+0x0a4 (operator+'s by-value parameter), the sum lands in
				// E+0x0d4 and the halved result in E+0x08c before being
				// assigned back.  /O2 canonicalises the fadd operand order,
				// so the load order is not evidence of which term is first.
				vNewPos = (vNewPos + pObj->m_Pos) * 0.5f;
			}
			else
			{
				// 0x004614f0..0x0046150d.  A SECOND zero vector, at
				// E+0x0bc, distinct from vForce at E+0x068 -- so it is its
				// own local and not a reuse.
				LTVector vZeroForce;

				vZeroForce.x = 0.0f;
				vZeroForce.y = 0.0f;
				vZeroForce.z = 0.0f;

				// 0x004614d0..0x00461511.  Same seven pushes, hidden buffer
				// at E+0x114 this time, and the fourth argument is the
				// immediate 0x3f800000 = 1.0f (0x004614e0).
				vNewPos = sub_4619a0Q1S(
					pObj, pRec,
					&vZeroForce, 1.0f, g_fMaxExtrapolateTimeQ1S, 1);

				// 0x0046151b .. 0x0046153c.  See the DEAD CODE note above:
				// the clamp at 0x00461387 makes this always true.
				if (fDT <= g_fMaxExtrapolateTimeQ1S)
				{
					// 0x00461542..0x004615dc.  The same expression as the
					// other arm, written out a second time rather than
					// shared -- VC6 would have folded one site
					// into the other if it were one source statement.
					vNewPos = (vNewPos + pObj->m_Pos) * 0.5f;
				}
				else
				{
					// 0x004615de..0x0046160a.
					bStopped = LTTRUE;                      // 0x004615f0
					pObj->m_PosPredictLink.Remove();
					pObj->m_PosPredictLink.TieOff();
				}
			}

			// 0x0046160c..0x00461629.  Saved BEFORE the move, because the
			// two calls below both write m_Pos and the red debug line wants
			// the before/after pair.  Three integer dword moves = one
			// whole-vector assignment.
			vOldPos = pObj->m_Pos;

			// 0x00461630..0x00461638.  Second argument is the literal 0,
			// where predobj.cpp's 0x00460db0 passes `bTeleport || bStopped`.
			pShellIface->OnObjectMove(pObj, LTFALSE, &vNewPos);

			// 0x0046163b `test $0x20,%ah` on the high half = 0x2000 =
			// FLAG_SOLID (1<<13), then 0x00461646 tests bStopped.  Both
			// `je`/`jne` go to the same else at 0x00461670, so it is one
			// `&&`.
			if ((pObj->m_Flags & FLAG_SOLID) && !bStopped)
			{
				// 0x0046164e..0x0046166b.  Virtual slot 0x30, three stack
				// arguments, callee cleanup.
				pShell->m_pClientMgr->m_pClientDE->m_pPhysicsLT->
					MoveObject(pObj, &vNewPos, 0);
			}
			else
			{
				// 0x00461670..0x00461688, `add $0x10,%esp`.
				sub_4207a0Q1S(pShell->m_pClientMgr, pObj, &vNewPos, 1);
			}

			// 0x0046168b / 0x00461696.  The console variable is compared
			// against the literal 1 (`cmpl $0x1,0x4b32c0`), not merely
			// tested, so it is `== 1` and not a truth test.
			if (pRec->m_hPredictLines && g_CV_PredictionLinesQ1S == 1)
			{
				LTLine line;

				// RED: where the object was, to where it now is.
				// 0x004616a3..0x00461717.
				line.m_Points[0].m_Pos = vOldPos;
				line.m_Points[0].r = 1.0f;
				line.m_Points[0].g = 0.0f;
				line.m_Points[0].b = 0.0f;
				line.m_Points[0].a = 1.0f;
				line.m_Points[1].m_Pos = pObj->m_Pos;
				line.m_Points[1].r = 1.0f;
				line.m_Points[1].g = 0.0f;
				line.m_Points[1].b = 0.0f;
				line.m_Points[1].a = 1.0f;
				ci_AddLineQ1S(pRec->m_hPredictLines, &line);

				// GREEN: where the object was, to where prediction asked it
				// to go.  0x0046171c..0x00461790.
				line.m_Points[0].m_Pos = vOldPos;
				line.m_Points[0].r = 0.0f;
				line.m_Points[0].g = 1.0f;
				line.m_Points[0].b = 0.0f;
				line.m_Points[0].a = 1.0f;
				line.m_Points[1].m_Pos = vNewPos;
				line.m_Points[1].r = 0.0f;
				line.m_Points[1].g = 1.0f;
				line.m_Points[1].b = 0.0f;
				line.m_Points[1].a = 1.0f;
				ci_AddLineQ1S(pRec->m_hPredictLines, &line);

				// BLUE: the raw server state -- the last position we were
				// told, and that position plus a full extrapolation window
				// of the last velocity we were told.
				// 0x00461795..0x00461864.  The multiply temp is at E+0x0c8
				// and the sum reads m_vPredictPos IN PLACE (`fadds (%eax)`),
				// with no by-value copy, which is what makes m_vPredictPos
				// the left operand and the product the right one.
				line.m_Points[0].m_Pos = pRec->m_vPos;
				line.m_Points[0].r = 0.0f;
				line.m_Points[0].g = 0.0f;
				line.m_Points[0].b = 1.0f;
				line.m_Points[0].a = 1.0f;
				line.m_Points[1].m_Pos = pRec->m_vPos +
				                         pRec->m_vVel *
				                         g_fMaxExtrapolateTimeQ1S;
				line.m_Points[1].r = 0.0f;
				line.m_Points[1].g = 0.0f;
				line.m_Points[1].b = 1.0f;
				line.m_Points[1].a = 1.0f;
				ci_AddLineQ1S(pRec->m_hPredictLines, &line);
			}
		}

		// --------------------------------------------------------------- //
		//  THE ROTATION LIST -- 0x00461884 .. 0x00461986
		// --------------------------------------------------------------- //
		pCur = pShell->m_RotPredictList.m_pNext;            // 0x00461884
		while (pCur != &pShell->m_RotPredictList)           // 0x00461887/0x0046197f
		{
			pObj = (LTObjectQ1S*)pCur->m_pData;             // 0x00461898
			pCur = pCur->m_pNext;                           // 0x0046189b

			// 0x004618a2..0x004618c3.  The member is RELOADED in the taken
			// arm (0x004618b3); see pt_MinQ1S above.
			fStep = pt_MinQ1S(pObj->m_fRotPredictTime, fFrameTime);

			// 0x004618c7..0x004618eb.  `push %ecx ; fstps (%esp)` is the
			// float argument; three pointer pushes; `add $0x10,%esp`.
			quat_SlerpQ1S(rNew.m_Quat, pObj->m_Rotation.m_Quat,
			              pObj->m_PredictRot.m_Quat,
			              fStep / pObj->m_fRotPredictTime);

			// 0x004618f7..0x00461904, virtual slot 0x40.
			pShellIface->OnObjectRotate(pObj, LTFALSE, &rNew);

			// 0x00461907..0x00461914, `add $0xc,%esp`.
			sub_420880Q1S(pShell->m_pClientMgr, pObj, &rNew);

			// 0x00461919..0x00461926 (`fsts`, so the value stays in %st for
			// the compare that follows).
			pObj->m_fRotPredictTime -= fStep;

			if (pObj->m_fRotPredictTime <= 0.0f)            // 0x0046192c
			{
				// 0x00461939..0x00461952: four dword pairs = ltrotation.h's
				// inline operator=.  The direction is +0x9c -> +0x160.
				pObj->m_PredictRot = pObj->m_Rotation;
				pObj->m_fRotPredictTime = 0.0f;             // 0x00461955
				pObj->m_RotPredictLink.Remove();            // 0x0046195f
				pObj->m_RotPredictLink.TieOff();            // 0x00461973
			}
		}
	}
}


// *********************************************************************** //
//
//  FOOT OF FILE -- extents, hand-offs, coin-flips
//
// *********************************************************************** //
//
//  ==================== EXTENT FINDINGS ==================================
//
//   0x00473bc0   947   ROW IS CORRECT.  One function,
//                      0x00473bc0..0x00473f72 with `ret` at 0x00473f72 and
//                      thirteen `nop` to the next COMDAT at 0x00473f80.
//                      Zero `e8`/`e9` anywhere in .text lands strictly
//                      inside it; the one inbound call, 0x00474491, hits
//                      the entry.  ALREADY WRITTEN -- see the verdict.
//
//   0x00461550  1090   ROW IS NOT A FUNCTION.  Operand byte inside
//                      `mov 0x18c(%edi),%ecx` at 0x0046154c, manufactured by
//                      a `call` opcode read out of the `-0x18` displacement
//                      of `mov -0x18(%ebp),%esp` at 0x0049c7c6.
//
//   0x00461270   736   ROW SIZE IS WRONG.  The true function is
//                      0x00461270..0x00461991, **1826 bytes**, not 736.  The
//                      736 comes from the enumeration cutting it at the
//                      phantom 0x00461550.  The arithmetic closes exactly:
//                      736 + 1090 = 1826, so the two rows TOGETHER are the
//                      one function and neither boundary loses a byte -- the
//                      cut between them is purely spurious.  `ret` at
//                      0x00461991, then fourteen `nop` to predobj.cpp's
//                      section C at 0x004619a0.
//
//  ==================== WHAT BELONGS IN ANOTHER FILE =====================
//
//   1. THIS BODY BELONGS IN THE SAME TU AS predobj.cpp's SECTIONS A/B/C,
//      i.e. the client-prediction TU that owns 0x00460d30..0x00461ccf.  It
//      is the CONSUMER of all three: it calls 0x004619a0 twice and it is the
//      only reader of the eight MaxExtrapolateTime sites predobj.cpp
//      counted.  Merging is a matter of deleting this file's duplicate class
//      block; the field sets are identical bar the three additions named
//      below.
//
//   2. src/engine/lithtech/clientshell.cpp:156 currently declares
//      `void sub_461270(CClientShell* pShell);` and nothing defines it.
//      Once this lands, that declaration should point at the definition.
//
//   3. CClientShell GAINS A FIELD NAME AND A TYPE CORRECTION.  +0x2c is
//      `float m_fLastTime2c`, the previous frame's CClientMgr::m_Time.
//      src/engine/lithtech/lt2new3.cpp spells it `uint32` and its own foot
//      says either spelling is safe; `fsubs 0x2c(%ebp)` at 0x004612a3 and
//      `fstps 0x2c(%ebp)` at 0x004612d1 settle that it is a FLOAT.  That is
//      a free correction for clientshell.cpp / lt2new3.cpp.
//
//   4. CClientMgr +0x125c is read here as a float (`flds 0x125c(%ecx)`),
//      agreeing with src/engine/lithtech/cltrest.cpp:384 (`float m_Time`)
//      and DISAGREEING with src/engine/lithtech/clientshell.cpp:465 and
//      clientmgr3.cpp:834, which both spell it `uint32 m_Unknown125c`.  The
//      three files should agree; cltrest.cpp is right.
//
//   5. LTObject +0x09c `LTRotation m_Rotation` is inside predobj.cpp's
//      `m_pad_08c[0xc4 - 0x8c]`.  It is already named in
//      src/engine/server/ltobject_internal.h:269, so predobj.cpp's view can
//      simply adopt it.
//
//   6. A HEADER SHAPE WORTH TAKING.  predobj.cpp declares SPredictRec as a
//      free class and casts LTObject+0x124 to it.  Since LTObject's
//      +0x124..+0x143 fields are exactly that record, the cleaner spelling
//      -- and the one the shipped source almost certainly had -- is an
//      embedded `SPredictRec m_PredictRec;` member at +0x124, which removes
//      the cast from both call sites here and from predobj.cpp.  It cannot
//      change codegen: the address handed over is the same.
//
//  ==================== COIN-FLIPS NOT SETTLED ===========================
//
//   A. THE ORDER OF `bStopped = LTTRUE` AGAINST THE UNLINK, 0x004615de.
//        spelling 1 (WRITTEN ABOVE):
//            bStopped = LTTRUE;
//            pObj->m_PosPredictLink.Remove();
//            pObj->m_PosPredictLink.TieOff();
//        spelling 2:
//            pObj->m_PosPredictLink.Remove();
//            pObj->m_PosPredictLink.TieOff();
//            bStopped = LTTRUE;
//      The `movl $0x1,0x60(%esp)` is emitted at 0x004615f0, AFTER the two
//      link loads (0x004615de, 0x004615e4) but BEFORE every link store
//      (0x004615f8 onward).  "VC6 keeps independent stores where
//      it found them" favours spelling 1; ordinary load-shadow scheduling
//      would explain spelling 2 just as well.  SETTLED BY: one build --
//      spelling 2 would put the `movl $1` after 0x0046160a.
//
//   B. THE THREE ZERO STORES, 0x0046133d and 0x004614f0.
//        spelling 1 (WRITTEN ABOVE):  v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
//        spelling 2:                  v.Init();
//        spelling 3:                  v.Init(0.0f, 0.0f, 0.0f);
//      Three `movl $0x0` and no call.  predobj.cpp observed at 0x00461b48
//      that VC6 emits a CALL to 0x0041ab50 for `Init(0,0,0)` in this very
//      module, which is why Init is not written -- but that observation and
//      these three stores cannot both be Init unless VC6 changed its mind
//      between two functions of one TU.  One of the two readings is wrong
//      and a single build tells you which, since spelling 2/3 would either
//      produce a call or produce exactly these three stores.
//
//   C. WHICH quat_Slerp OVERLOAD, 0x004618eb.
//        spelling 1 (WRITTEN ABOVE, transform.cpp's):
//            quat_Slerp(rNew.m_Quat, pObj->m_Rotation.m_Quat,
//                       pObj->m_PredictRot.m_Quat, t);
//        spelling 2 (servermath.h's):
//            quat_Slerp(&rNew, &pObj->m_Rotation, &pObj->m_PredictRot, t);
//      `m_Quat` is at offset 0 of LTRotation, so both emit the identical
//      three `lea`s.  UNDECIDABLE FROM THE BYTES AND FREE EITHER WAY; it
//      only matters for which header this TU has to include.
//
//   D. SCOPE OF `LTLine line`, 0x00461728.  All three fills use the one
//      slot E+0x028, so either one declaration reused three times (written
//      above) or three block-scoped declarations MSVC overlapped.  No byte
//      distinguishes them.
//
//   E. WITHIN EACH LTLine FILL, whether the source writes
//      `m_Points[0]` completely and then `m_Points[1]`, or writes both
//      positions and then both colours.  The two whole-vector assignments
//      are scheduled into the colour runs in all three blocks, and the
//      colour immediates themselves come out in strict r,g,b,a order for
//      point 0 then point 1 -- which is why it is written point-major.  A
//      position-major source would most likely have put both `m_Pos` stores
//      adjacent at least once in three tries, and it never does.  Weak
//      evidence rather than none, so it is listed here.
//
//  ==================== WHAT WAS NOT RUN =================================
//
//  The call-sequence comparison -- NOT RUN.  It needs a compiled object.
//  For the record, the shipped side has 8
//  direct calls (0x00461407, 0x00461511, 0x00461683, 0x00461717,
//  0x00461790, 0x00461864, 0x004618eb, 0x00461914) to SIX distinct callees
//  plus 4 indirect (0x004612df, 0x00461638, 0x0046166b, 0x00461904), and
//  the source above issues every one of the twelve in that order.  No
//  region of this function is undecoded.
//
// *********************************************************************** //
