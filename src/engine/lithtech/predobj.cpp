// *********************************************************************** //
//
//  MODULE:   CLUSTER460000 (.CPP)  -- scratch file, NOT a drop-in TU
//
//  PURPOSE:  lithtech.exe 0x00460db0, 0x00461120 and 0x004619a0 -- three
//            functions of the client's OBJECT-PREDICTION module -- plus
//            0x00460160, the 32-bit twin of the DXT decompressor
//            src/engine/lithtech/partfm.cpp already reconstructs.  Four of
//            the five rows; the fifth is already written elsewhere.
//
//  THESE ARE TWO DIFFERENT TRANSLATION UNITS and this file is a scratch
//  file, not a drop-in TU.  Sections A/B/C (0x00461120, 0x00460db0,
//  0x004619a0) belong together in the client-prediction TU that owns
//  0x00460d30..0x00461ccf; section D (0x00460160) belongs beside
//  partfm.cpp's section C, i.e. in src/engine/lithtech/fmconvert.cpp.
//
//  ==================== ROW-BY-ROW VERDICT, BEFORE ANY CODE ==============
//
//   ADDR        SIZE   VERDICT
//   0x00460160  2704   ONE FUNCTION (extent sound), and it is the 32-BIT
//                      TWIN of 0x0045f6a0, which src/engine/lithtech/
//                      partfm.cpp already reconstructs and already names
//                      as this row's sibling.  Written below as section D.
//                      Belongs in the SAME TU as 0x0045f6a0.
//   0x00460db0   871   NEW.  Client object-prediction, position half.
//                      Written below as section B.
//   0x00461120   336   NEW.  The rotation half of the same pair.
//                      Written below as section A.
//   0x004619a0   727   EXTENT WRONG -- the real function is 815 bytes,
//                      0x004619a0..0x00461ccf.  The row is cut at
//                      0x00461c77, which is a FALSE call target: the bytes
//                      `e8 40 4b 00` inside `movl $0x0,0x4b40e8` at
//                      0x0045d132 decode as `call 0x461c77` to a naive
//                      sweep, and 0x00461c77 is mid-instruction
//                      (`fmuls 0x1c(%eax)`, 0x00461c75..0x00461c77).  The
//                      88-byte `call` row the enumeration puts at
//                      0x00461c77 is not a function.  Written below as
//                      section C.  It RETURNS AN LTVector BY VALUE: seven
//                      pushed dwords, six real arguments plus the hidden
//                      buffer.
//   0x00462fd0   286   ALREADY WRITTEN.  It is sm_Unknown26755 and
//                      src/engine/server/serverlightanim.cpp line 404 has
//                      the body; servermgr.h declares it and smgr.cpp calls
//                      it.  NOT rewritten here -- see the note at the foot
//                      of this block for the one thing the lithtech.exe
//                      copy settles that the server.dll copy could not.
//
//   0x00462fd0 IN DETAIL.  lithtech.exe's copy is 286 bytes against
//   server.dll's 261, and the whole difference is one inline expansion:
//   0x00463055..0x0046306e is
//       movzwl 0x36(%esi),%eax ; movzwl 0x38(%esi),%ecx
//       sub %ecx,%eax ; sub $0x7,%eax ; js .. ; cmp $0x16,%eax ; jg ..
//   i.e. `GetSpaceLeft()` expanded as `(m_Unknown36 - m_Unknown38) - 7`,
//   where server.dll CALLS it.  That is a free field-level fact for
//   servermsg.h: +0x36 is the buffer limit, +0x38 the write cursor, and the
//   accessor is their difference less seven.  The rest of the body matches
//   serverlightanim.cpp statement for statement, including the
//   `m_Unknown38 = 1; m_Unknown34 = 1;` reset pair in that order
//   (0x00463089/0x0046308d) and the `m_Unknown34 - 1 > 0` tail test
//   (0x004630ae `dec %edx ; test %edx,%edx ; jle`).
//
//  ==================== WHAT THE CANDIDATE STRINGS TURNED OUT TO BE =======
//
//  Nothing in this cluster.  "DetailTex" / "BitDepth %d" / "CardDesc %s"
//  (0x004acf1c / 0x004acf28 / 0x004acf64) are referenced ONLY from
//  0x00461f51, 0x004626f9 and 0x00462672 -- r_LoadSystemTexture and
//  r_InitRender, i.e. src/engine/lithtech/clientrender.cpp, which is
//  already written.  "CPacket::WriteType" (0x004a7b08) has sixty-two
//  referencing sites across the image and none of them is in this cluster.
//
//  WHAT THE MODULE IS, and how it was identified
//  ---------------------------------------------
//  Not by those strings.  "DetailTex" / "BitDepth %d" /
//  "CardDesc %s" sit at 0x004acf1c..0x004acf64 and are referenced ONLY from
//  0x00461f51, 0x004626f9 and 0x00462672 -- r_LoadSystemTexture and
//  r_InitRender, i.e. src/engine/lithtech/clientrender.cpp, which is already
//  written.  None of the five rows touches any of them.
//
//  The CONSOLE-VARIABLE TABLE is what typed this cluster.  .data carries a
//  20-byte record per variable, { char* m_pName; float* m_pFloatVal;
//  int32* m_pIntVal; ... }, and three of its rows name the three globals
//  these functions read:
//
//      0x004aa274  { "Prediction",         0, ->0x004b32bc }
//      0x004aa288  { "PredictionLines",    0, ->0x004b32c0 }
//      0x004aa65c  { "MaxExtrapolateTime", ->0x004a9ef8, 0 }   (0.5f)
//
//  So 0x004b32bc is the console variable `Prediction`, 0x004b32c0 is
//  `PredictionLines`, and this run is the client's dead-reckoning of remote
//  objects.  0x004a9ef8 (MaxExtrapolateTime) is read at eight sites in
//  0x00461270..0x004619a0, which puts the whole run 0x00460d30..0x00461c cf
//  in one translation unit.
//
//  The call graph agrees: 0x00460db0 and 0x00461120 are each called from
//  exactly two sites, and the two sites are ADJACENT in both callers
//  (0x00478d9b/0x00478df2 and 0x0047a326/0x0047a376) -- the position half
//  and the rotation half of one object update decoded out of a packet.
//
//  THE TWO ALREADY-RECONSTRUCTED NEIGHBOURS that fix the class shapes
//  ------------------------------------------------------------------
//    * src/engine/lithtech/clientshell.cpp gives CClientShell: LTLink
//      m_Link44 at 0x044, LTLink m_Link50 at 0x050, CClientMgr* at 0x078.
//      Argument one of both functions is a CClientShell* -- 0x00461120's
//      `mov 0x78(%ebx),%eax ; mov 0x61c(%eax),%ecx` is
//      pShell->m_pClientMgr->m_pClientShell, and clientmgr.h line 157
//      already names CClientMgr+0x61c `IClientShell* m_pClientShell`.
//    * src/engine/lithtech/lt2new3.cpp section D reconstructs 0x00460d80,
//      which is this same class's per-instance reset: it writes a float to
//      +0x28, a dword copied from m_pClientMgr+0x125c to +0x2c, and TIES
//      OFF +0x44 and +0x50.  Those two links are exactly the two list heads
//      the two functions below insert into -- +0x44 for position, +0x50 for
//      rotation -- which is what proves argument one is the same object.
//
//  A COIN-FLIP THIS CLUSTER SETTLES.  lt2new3.cpp's foot lists as its first
//  unsettled question whether VC6 copies a float member to a float member
//  with an integer `mov`.  It does.  Chain:
//      * 0x00460d8b `flds 0x8(%esp) ; fstps 0x28(%eax)` proves CClientShell
//        +0x28 is a FLOAT;
//      * 0x00460ebd `mov 0x28(%ebp),%ecx ; mov %ecx,0x128(%esi)` copies that
//        same +0x28 into the object with an INTEGER pair;
//      * and LTRotation::operator= -- four float-to-float member copies and
//        nothing else, inline in src/LT2/sdk/inc/ltrotation.h -- ships at
//        0x00461199 and 0x00461202 as four integer `mov` pairs.
//  So the `uint32` spelling lt2new3.cpp shipped for +0x2c is safe, and a
//  `float` spelling would have been equally safe.
//
//  GROUND TRUTH: the retail lithtech.exe only, cl 8447 /O2.  Every offset,
//  constant and branch below is cited by the address it was read at.
//
//  NAMING: every symbol introduced here carries a Q460 suffix so this file
//  cannot collide at link time with clientshell.cpp, clientmgr.h or
//  lt2new3.cpp, each of which declares its own view of these objects.
//
// *********************************************************************** //

#include "ltbasedefs.h"        // LTVector, LTRotation, ObjectCreateStruct,
                               // LTDIFF, OT_LINESYSTEM, FLAG_VISIBLE
#include "ltlink.h"            // LTLink / CheapLTLink::Remove / AddAfter


// ======================================================================= //
//
//  THE CLASSES THIS FILE DOES NOT OWN
//
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  ILTClient, as far as this run reaches it.
//
//  clientmgr2.cpp pads CClientMgr to +0x4c0 and ccline.cpp/cltcursor.cpp
//  name the pointer there m_pClientDE.  It is BOTH a C++ object with a vptr
//  at +0x00 (clientmgr3.cpp: `mov (%ecx),%edx ; call *0x11c(%edx)`) and the
//  flat table of cdecl function pointers src/engine/lithtech/
//  clientinterface.h lays out from +0x020 to +0x35c.  0x00460db0 uses one of
//  each:
//
//      0x00461018  mov 0x4c0(%eax),%edx ; push &ocs ; call *0x254(%edx)
//                  ; add $0x4,%esp
//          -> the FLAT slot, and clientinterface.h line 187 names 0x254
//             `HLOCALOBJ (*CreateObject)(ObjectCreateStruct*)` exactly.
//      0x00461040  mov 0x4c0(%ecx),%ecx ; mov (%ecx),%edx ; push %eax
//                  ; call *0xdc(%edx)
//          -> a VIRTUAL, slot 0xdc/4 = 55, __thiscall, one argument, no
//             caller cleanup.  Its name is not decidable from this image, so
//             it keeps its slot for a name; the call site says it destroys
//             the line-system object the other arm created.
// ----------------------------------------------------------------------- //
class LTObjectQ460;


class ICltQ460
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
	virtual void _v03c()=0;
	virtual void _v040()=0;
	virtual void _v044()=0;
	virtual void _v048()=0;
	virtual void _v04c()=0;
	virtual void _v050()=0;
	virtual void _v054()=0;
	virtual void _v058()=0;
	virtual void _v05c()=0;
	virtual void _v060()=0;
	virtual void _v064()=0;
	virtual void _v068()=0;
	virtual void _v06c()=0;
	virtual void _v070()=0;
	virtual void _v074()=0;
	virtual void _v078()=0;
	virtual void _v07c()=0;
	virtual void _v080()=0;
	virtual void _v084()=0;
	virtual void _v088()=0;
	virtual void _v08c()=0;
	virtual void _v090()=0;
	virtual void _v094()=0;
	virtual void _v098()=0;
	virtual void _v09c()=0;
	virtual void _v0a0()=0;
	virtual void _v0a4()=0;
	virtual void _v0a8()=0;
	virtual void _v0ac()=0;
	virtual void _v0b0()=0;
	virtual void _v0b4()=0;
	virtual void _v0b8()=0;
	virtual void _v0bc()=0;
	virtual void _v0c0()=0;
	virtual void _v0c4()=0;
	virtual void _v0c8()=0;
	virtual void _v0cc()=0;
	virtual void _v0d0()=0;
	virtual void _v0d4()=0;
	virtual void _v0d8()=0;
	virtual void _v0dc(HLOCALOBJ hObj)=0;                   // slot 55

public:
	// The flat table.  Only the one entry this file calls is declared; it is
	// cdecl (the caller does the `add $0x4,%esp`) and it is not virtual.
	uint8     m_pad_004[0x254 - 0x004];
	HLOCALOBJ (*CreateObject)(ObjectCreateStruct* pStruct);  // 0x254
};


// ----------------------------------------------------------------------- //
//  IClientShell -- the game DLL's callback interface, src/LT2/sdk/inc/
//  iclientshell.h.  Counting that header's virtuals from the destructor
//  gives OnObjectMove at slot 15 (0x3c) and OnObjectRotate at slot 16
//  (0x40), which is exactly what 0x004610f9 and 0x0046125b call.  Both are
//  __thiscall with three stack arguments and no caller cleanup.
// ----------------------------------------------------------------------- //
class IClientShellQ460
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
	virtual LTRESULT OnObjectMove(LTObjectQ460* hObj, LTBOOL bTeleport,
	                              LTVector* pNewPos)=0;      // 0x3c
	virtual LTRESULT OnObjectRotate(LTObjectQ460* hObj, LTBOOL bTeleport,
	                                LTRotation* pNewRot)=0;  // 0x40
};


// ----------------------------------------------------------------------- //
//  CClientMgr, as far as this run reaches it: the interface table at +0x4c0
//  (clientmgr2.cpp / ccline.cpp) and the game's shell at +0x61c
//  (clientmgr.h line 157).
// ----------------------------------------------------------------------- //
class CClientMgrQ460
{
public:
	uint8             m_pad_000[0x4c0];
	ICltQ460*         m_pClientDE;                           // 0x4c0
	uint8             m_pad_4c4[0x61c - 0x4c4];
	IClientShellQ460* m_pClientShell;                        // 0x61c
};


// ----------------------------------------------------------------------- //
//  CClientShell, the layout src/engine/lithtech/clientshell.cpp already
//  established, with the four fields this run adds.  +0x24 and +0x28 fall in
//  that file's `m_pad_0020[0x30-0x20]`; +0x28 is the float 0x00460d80 writes
//  from its second parameter and +0x24 the one the two functions here scale
//  the prediction window by.  Both are only ever READ here.
// ----------------------------------------------------------------------- //
class CClientShellQ460
{
public:
	uint8            m_pad_000[0x24];
	float            m_fPredictScale24;                      // 0x024
	float            m_fTime28;                              // 0x028
	uint8            m_pad_02c[0x44 - 0x2c];
	LTLink           m_PosPredictList;                       // 0x044
	LTLink           m_RotPredictList;                       // 0x050
	uint8            m_pad_05c[0x78 - 0x5c];
	CClientMgrQ460*  m_pClientMgr;                           // 0x078
};


// ----------------------------------------------------------------------- //
//  LTObject, as the client's predictor uses it.
//
//  src/engine/server/ltobject_internal.h reads 0x124..0x188 as one opaque
//  pad, because the SERVER never touches it.  The CLIENT keeps its whole
//  prediction record there, and the three functions below fill every field
//  of it.  Everything at or below 0x120 and the 0x18c position are that
//  header's, unchanged.
//
//  Read off the bytes:
//    0x124  float      the position window; `1.1f * pShell->+0x24` at
//                      0x00461088, zeroed at 0x004610d0.
//    0x128  float      pShell->+0x28 at 0x00460ebd (see the header note on
//                      float-to-float copies).
//    0x12c  LTVector   last position told to us -- the LTDIFF at 0x00460dd4
//                      compares the incoming position against it before
//                      0x00460ec8 overwrites it.
//    0x138  LTVector   last velocity told to us (0x00460ee2).
//    0x144  LTVector   the position we were AT when we were told
//                      (`= m_Pos`, 0x00460efc).
//    0x150  LTLink     place in CClientShell::m_PosPredictList.
//    0x15c  float      the rotation window (0x004611c0 / 0x004611fc).
//    0x160  LTRotation the rotation told to us (0x00461199 / 0x00461202).
//    0x170  LTLink     place in CClientShell::m_RotPredictList.
//    0x17c  HLOCALOBJ  the debug OT_LINESYSTEM object `PredictionLines`
//                      turns on (0x0046102c / 0x0046104e).
// ----------------------------------------------------------------------- //
class LTObjectQ460
{
public:
	uint8      m_pad_000[0x88];
	uint32     m_Flags;                                      // 0x088
	uint8      m_pad_08c[0xc4 - 0x8c];
	LTVector   m_Velocity;                                   // 0x0c4
	LTVector   m_Acceleration;                               // 0x0d0
	float      m_FrictionCoefficient;                        // 0x0dc
	uint8      m_pad_0e0[0xf0 - 0xe0];
	LTVector   m_BBoxMin;                                    // 0x0f0
	LTVector   m_BBoxMax;                                    // 0x0fc
	uint8      m_pad_108[0x120 - 0x108];
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
	LTVector   m_Dims;                                       // 0x198
	float      m_Radius;                                     // 0x1a4

public:
	// 0x0045aaa0, ltobject.obj:
	// ?SetPos@LTObject@@QAEXV?$_CVector@M@@@Z -- __thiscall, the vector BY
	// VALUE (0x00461a2a builds a twelve-byte copy on the stack with
	// `mov %esp,%ecx` and three dword pairs before the call).  Declared, not
	// defined: src/engine/server/ltobject.cpp's LTObject::SetPos is the body
	// (link/aliases.rsp), matching both images.
	void SetPos(LTVector vPos);                              // 0x0045aaa0
};


// ----------------------------------------------------------------------- //
//  The console variables, off the .data table quoted in the header note.
//  Both are plain int32 in .bss: no site in .text WRITES either, and their
//  addresses are never taken -- only con_ machinery outside this image's
//  code reaches them, through the record.
// ----------------------------------------------------------------------- //
// dsi.cpp defines both, under the server's names, as plain int.
extern int g_CV_Prediction;                                  // 0x004b32bc
extern int g_CV_PredictionLines;                             // 0x004b32c0

// 1.1f, a file-scope float in .data read at 0x0046108a and 0x004611c2 and
// nowhere else in the image.  It is NOT in the console-variable table, so it
// is this module's own tunable and not a variable; defined here, with the
// shipped word 0x3f8ccccd. (name ours)
float g_fPredictSlackQ460 = 1.1f;                            // 0x004acef8


// ----------------------------------------------------------------------- //
//  The two setters this run tails into.  src/engine/lithtech/clientmgr.h
//  lines 237 and 543 already declare both with these prototypes; restated
//  here so this TU stands alone.
// ----------------------------------------------------------------------- //
void sub_4207a0Q460(CClientMgrQ460* pClientMgr, LTObjectQ460* hObj, LTVector* pPos,
                    int nUnknown);                           // 0x004207a0
void sub_420880Q460(CClientMgrQ460* pClientMgr, LTObjectQ460* hObj,
                    LTRotation* pRotation);                  // 0x00420880


// ======================================================================= //
//
//  A.  0x00461120 (336 bytes) -- tell an object its new ROTATION
//
//  Five __cdecl arguments, void.  Both call sites (0x00478df2, 0x0047a376)
//  push six dwords and clean 0x14, and the callee reads 0x10/0x14/0x18/
//  0x1c/0x20 off %esp after its three pushes.
//
//  WHAT PINS THE SHAPE
//    * 0x0046112a `mov 0x78(%ebx),%eax ; mov 0x61c(%eax),%ecx` is loaded
//      BEFORE the first branch, and %ecx is not touched again until the
//      virtual call at the very bottom -- so the interface pointer is a
//      statement of its own at the top of the function, not an expression at
//      the call.  m_pClientMgr itself is re-read at 0x0046125e for the
//      0x00420880 call: two written-out member reads, not one local.
//    * the guard chain is three FORWARD jumps to two blocks that both sit
//      past the interpolating body's own `ret` (0x004611d1), so every guard
//      is positive and they nest: `if (Prediction) { if (!a && !b && scale)
//      { ...; return; } ... }`.
//    * `test $0x40,%ah` after `fcomps` at 0x00461158 is the EQUAL bit alone
//      -- `!= 0.0f`, not `> 0.0f`.
//    * the four dword pairs at 0x00461199 and again at 0x00461202 are
//      ltrotation.h's inline `operator=`, written out in BOTH arms in the
//      other order relative to the float store: the interpolating arm
//      assigns the rotation and then the window, the stopping arm the window
//      and then the rotation.  Two separate source sites.
//    * the six link stores in the stopping arm (0x004611f0..0x004611f8) are
//      Remove() -- which already tie-offs -- FOLLOWED BY a second explicit
//      TieOff().  The interpolating arm has only four, i.e. Remove() alone.
//    * the `(a || b)` at 0x00461225 and 0x00461239 is ONE source expression
//      that VC6 tail-duplicated: the two copies differ only in where the
//      zero came from, and both fall into the same `mov (%ecx),%edx`.
//
// ======================================================================= //
void sub_461120Q460(CClientShellQ460* pShell, LTObjectQ460* pObj,
                    LTRotation* pRot, LTBOOL bTeleport, LTBOOL bStopped)
{
	IClientShellQ460* pShellIface = pShell->m_pClientMgr->m_pClientShell;

	if (g_CV_Prediction)
	{
		if (!bTeleport && !bStopped && pShell->m_fPredictScale24 != 0.0f)
		{
			pObj->m_RotPredictLink.Remove();
			pShell->m_RotPredictList.AddAfter(&pObj->m_RotPredictLink);

			pObj->m_PredictRot = *pRot;
			pObj->m_fRotPredictTime = g_fPredictSlackQ460 *
			                          pShell->m_fPredictScale24;
			return;
		}

		pObj->m_RotPredictLink.Remove();
		pObj->m_RotPredictLink.TieOff();

		pObj->m_fRotPredictTime = 0.0f;
		pObj->m_PredictRot = *pRot;
	}

	pShellIface->OnObjectRotate(pObj, bTeleport || bStopped, pRot);
	sub_420880Q460(pShell->m_pClientMgr, pObj, pRot);
}


// ======================================================================= //
//
//  B.  0x00460db0 (871 bytes) -- tell an object its new POSITION
//
//  Six __cdecl arguments, void.  The position half of the same update
//  0x00461120 is the rotation half of; the two callers push the pair back to
//  back.  Argument four is the object's VELOCITY as the server sent it, and
//  argument six is an in/out "it has stopped" flag the body may raise.
//
//  WHAT PINS THE SHAPE
//    * FRAME CHECKSUM.  `sub $0x288,%esp` plus four pushes; the arguments
//      start at %esp+0x29c.  The locals are one dword at +0x10 (the spilled
//      interface pointer) and 0x284 bytes at +0x14, and 4 + 0x284 = 0x288
//      exactly.  0x284 IS sizeof(ObjectCreateStruct) for this SDK:
//      MAX_OCS_CLASSNAME_LEN 64 puts m_Filename at +0x7c,
//      MAX_CS_FILENAME_LEN+1 = 128 puts m_SkinNames at +0xfc, and
//      MAX_MODEL_TEXTURES 2 is the literal trip count of the loop at
//      0x00460fe3.  So there is exactly one local besides the struct.
//    * the three-way |dx| test is LTDIFF (ltbasedefs.h line 1244) verbatim:
//      `flds a ; fcomps b` then the swapped subtraction on the C0 arm.  The
//      threshold test is `test $0x41,%ah` -- C0 OR C3 -- so `<= 0.1f`, while
//      the magnitude test at 0x00460ea1 is `test $0x01,%ah` alone, `< 0.01f`.
//    * the magnitude IS LTVector::Mag: the three components are loaded once
//      (0x00460e7b..0x00460e81), squared and summed, and `fsqrt` appears
//      inline because /O2 implies /Oi and sqrt is an intrinsic.
//    * ObjectCreateStruct: the CONSTRUCTOR at 0x004147f0 is CALLED
//      (0x00460f73) and Clear() is then expanded INLINE (0x00460f78..
//      0x00461009) in ltbasedefs.h's exact statement order, down to the
//      `for (i = 0; i < MAX_MODEL_TEXTURES; i++)` loop.  That is the
//      `ObjectCreateStruct ocs; INIT_OBJECTCREATESTRUCT(ocs);` idiom, and
//      the redundancy is the source's, not the compiler's.
//      NOTE the one thing that differs from 0x004147f0's own expansion:
//      there m_Rotation.Init() is inlined as four stores, here it is a CALL
//      to 0x00402ea0 with (0,0,0,1.0f) pushed.  Same header, same inline
//      body -- VC6 simply stopped inlining once this caller got big.  If our
//      build inlines it, this function comes back ~12 bytes short and the
//      fix is to hand-write Clear()'s statements with the four stores.
//    * the line-system pair is `if (!h && lines) {...} else if (h && !lines)
//      {...}` and not the nested spelling: 0x00461034 RE-TESTS the handle
//      that 0x00460f5f just tested, which only the flat `else if` produces.
//    * the `(bTeleport || bStopped)` at the bottom is one expression that
//      VC6 tail-duplicated into both predecessors, exactly as in 0x00461120.
//
//  THE TAIL, MEASURED.  Shipped keeps bTeleport in %ecx and bStopped in
//  %eax across the member copies (0x00460ea8..0x00460f39) and gives the
//  prediction-off path its OWN `bTeleport || bStopped` test on those
//  registers (0x004610e3); one shared tail after the `if` reloads both from
//  their slots at every test (18 blocks, 845 bytes).  One copy of the tail
//  per arm reproduces the registers and the shipped layout (9 blocks) -- but
//  then /O2 expands m_Rotation.Init() inside the inlined Clear(), where
//  shipped CALLS 0x00402ea0.  Writing the tail as this inline helper spends
//  the extra inline budget that turns Rotation.Init back into the call
//  (budget spent from the end backwards) -- 7 blocks, 865 bytes.
//  The helper is budget evidence, not a known original name.  Left: the x
//  LTDIFF's operand order (ours `flds pred ; fcomps pos` with the load
//  hoisted out of both arms, shipped `flds pos ; fcomps pred` unhoisted;
//  swapped operands and a split `if` both measured no change) and %eax/%edx
//  swapped around the CreateObject call.
// ======================================================================= //
inline void pred_MoveTailQ460(IClientShellQ460* pShellIface, CClientShellQ460* pShell,
                              LTObjectQ460* pObj, LTVector* pPos,
                              LTBOOL bTeleport, LTBOOL bStopped)
{
	pShellIface->OnObjectMove(pObj, bTeleport || bStopped, pPos);
	sub_4207a0Q460(pShell->m_pClientMgr, pObj, pPos, 1);
}

void sub_460db0Q460(CClientShellQ460* pShell, LTObjectQ460* pObj,
                    LTVector* pPos, LTVector* pVel,
                    LTBOOL bTeleport, LTBOOL bStopped)
{
	IClientShellQ460* pShellIface = pShell->m_pClientMgr->m_pClientShell;

	if (LTDIFF(pPos->x, pObj->m_vPredictPos.x) <= 0.1f &&
	    LTDIFF(pPos->y, pObj->m_vPredictPos.y) <= 0.1f &&
	    LTDIFF(pPos->z, pObj->m_vPredictPos.z) <= 0.1f &&
	    pVel->Mag() < 0.01f)
	{
		bStopped = 1;
	}

	pObj->m_fPredictStamp = pShell->m_fTime28;
	pObj->m_vPredictPos   = *pPos;
	pObj->m_vPredictVel   = *pVel;
	pObj->m_vPredictBase  = pObj->m_Pos;

	if (g_CV_Prediction)
	{
		if (!bTeleport && !bStopped && pShell->m_fPredictScale24 != 0.0f)
		{
			if (!pObj->m_hPredictLines && g_CV_PredictionLines)
			{
				ObjectCreateStruct ocs;

				INIT_OBJECTCREATESTRUCT(ocs);
				ocs.m_ObjectType = OT_LINESYSTEM;
				ocs.m_Flags      = FLAG_VISIBLE;

				pObj->m_hPredictLines =
					pShell->m_pClientMgr->m_pClientDE->CreateObject(&ocs);
			}
			else if (pObj->m_hPredictLines && !g_CV_PredictionLines)
			{
				pShell->m_pClientMgr->m_pClientDE->_v0dc(
					pObj->m_hPredictLines);

				pObj->m_hPredictLines = NULL;
			}

			pObj->m_PosPredictLink.Remove();
			pShell->m_PosPredictList.AddAfter(&pObj->m_PosPredictLink);

			pObj->m_fPosPredictTime = g_fPredictSlackQ460 *
			                          pShell->m_fPredictScale24;
			return;
		}

		pObj->m_PosPredictLink.Remove();
		pObj->m_PosPredictLink.TieOff();

		pObj->m_fPosPredictTime = 0.0f;

		pred_MoveTailQ460(pShellIface, pShell, pObj, pPos, bTeleport, bStopped);
	}
	else
	{
		pred_MoveTailQ460(pShellIface, pShell, pObj, pPos, bTeleport, bStopped);
	}
}


// ======================================================================= //
//
//  C.  0x004619a0 (815 bytes, NOT the 727 the row says) -- extrapolate an
//      object's position forward
//
//  EXTENT DEFECT, FIXED HERE.  The function enumeration splits this
//  row at 0x00461c77 and calls the remainder an 88-byte `call` row.  There
//  is no function there: 0x00461c77 is a FALSE call target, the four bytes
//  `e8 40 4b 00` inside `movl $0x0,0x4b40e8` at 0x0045d132 (an initialiser
//  for the sixteen-entry DXT3 alpha ramp at 0x004b40e8).  The real function
//  runs 0x004619a0..0x00461ccf: 0x00461c21 is reached only by the entry
//  guard's own `je`, it falls THROUGH 0x00461c77 mid-instruction
//  (`fmuls 0x1c(%eax)` spans 0x00461c75..0x00461c77), and 0x00461ccf is a
//  single `nop` before the next function at 0x00461cd0.
//
//  IT RETURNS AN LTVector BY VALUE.  Both call sites (0x00461407,
//  0x00461511) push SEVEN dwords and clean 0x1c, and then read the result
//  through %eax (`mov (%eax),%ecx ; mov 0x4(%eax),%edx`).  That is the
//  hidden-buffer convention: six real arguments plus the buffer pointer
//  pushed last, and the callee hands the buffer back in %eax -- which is
//  exactly what falls out of 0x00461be1 (`mov %eax,%ecx` with %eax still
//  holding the first stack argument) and 0x00461c99.
//
//  FRAME CHECKSUM.  `sub $0x78,%esp` and the locals are
//      +0x10  uint32   the saved change flags
//      +0x14  float    the saved friction
//      +0x18  LTVector the saved position
//      +0x24  LTVector the saved velocity
//      +0x30  LTVector the saved acceleration
//      +0x3c  LTVector the gravity constant handed to the constructor
//      +0x48  0x40     the MoveAbstract
//  4 + 4 + 12 + 12 + 12 + 12 + 0x40 = 0x78 to the byte, so nothing else is
//  declared.  The 0x40-byte record is lt2new1.cpp's CMoveAbstractView (the
//  one CClientMgr keeps at +0x4c4) with the three fields that file could
//  only pad now named: +0x20 the normalised force, +0x2c its magnitude,
//  +0x30 the friction coefficient -- because 0x00410e20, which
//  clientmgr3.cpp already calls on CClientMgr+0x4c4, writes +0x14, +0x2c
//  and +0x20 and nothing else, and 0x004619a0 duplicates that body inline
//  against a different vector.
//
//  WHAT THE BODY DOES: it borrows the object.  The five physics fields are
//  copied out, the object is moved to the predicted state, the engine's own
//  integrator (0x004500d0) is run over it for one step, and every field is
//  put back -- so the answer is computed with the real physics code and the
//  object is left untouched.
//
//  WHAT PINS THE SHAPE
//    * the entry guard's `je` goes forward to 0x00461c21, which sits past
//      the body's own `ret` (0x00461c20), and 0x00461c21 has its OWN
//      epilogue (it pops the four registers early, at 0x00461c36).  Two
//      separate exits, so: positive guard, `return` inside it, and the
//      closed-form extrapolation as the function's trailing statement.
//    * `m_Acceleration` is zeroed with three integer stores through the
//      %edi pointer while 0x00461b48 CALLS 0x0041ab50 for m_Dir.Init():
//      BOTH are _CVector::Init().  The first is at depth one and expands;
//      the second is inside the expanded Set() and is declined.  Writing
//      the first as three component stores (the old reading of this
//      bullet) addressed them off %esi and cost the %ebx/%edi assignment.
//
//  IDENTICAL (815 of 815) once the record is GlobalForceControl with its
//  inline constructor and an explicit inline Set(pForce) -- 834 bytes and
//  20 blocks as a hand-written copy of Set's body behind an out-of-line
//  constructor, 821 with the class but the three stores.
//    * `fcomps 0x004a1a04 ; test $0x41,%ah ; jne` is `> 0.00001f`: the
//      normalising arm falls through and the Init arm is jumped to.
//    * the normalised copy is loaded from the PARAMETER (`mov (%ecx)`,
//      %ecx = pForce) and not from the member that was just written, which
//      is why `m_vGlobalForceDir = *pForce;` and not `= m_vGlobalForce;`.
//      The three `fdivs 0x74(%esp)` that follow re-read the magnitude each
//      time -- ltvector.h's `operator/=(T)` verbatim.
//    * `push %ecx ; call 0x004500d0` is followed by `sub $0x8,%esp` and a
//      twelve-byte argument build with no `add $0x4` in between: cl 8447
//      DEFERRED the cdecl cleanup into the next call's frame.
//      The integrator is fixed-prototype cdecl, not stdcall.
//    * the return is one `operator+`: the second operand is copied
//      twelve bytes into a slot (0x00461bc7..0x00461bdd) because
//      _CVector::operator+ takes its argument BY VALUE, and the sum is
//      built in a second temp and copied to the hidden buffer.
//
// ======================================================================= //

// The prediction record 0x00460db0 fills, addressed as a unit.  It IS
// LTObject+0x124: +0x08 is m_vPredictPos and +0x14 is m_vPredictVel, and
// 0x00461407 hands the address of exactly that field. (name ours)
class SPredictRecQ460
{
public:
	float    m_fPredictTime;                                 // 0x00 (+0x124)
	float    m_fPredictStamp;                                // 0x04 (+0x128)
	LTVector m_vPos;                                         // 0x08 (+0x12c)
	LTVector m_vVel;                                         // 0x14 (+0x138)
};

// lt2new1.cpp's CMoveAbstractView is this same 0x40-byte record: it IS
// GlobalForceControl (commonphysics.h), section F's class.  0x004619a0
// constructs one on the stack -- the constructor expanded, the Set() inside
// it CALLED (0x00461a8d -> 0x00410e20, the one-level constructor rule) and
// its m_Unknown30 store dead under the 1.0f written after -- and then
// EXPANDS an explicit Set(pForce), whose else arm's m_Dir.Init() is the
// declined call at 0x00461b52.
class GlobalForceControlQ460
{
public:
	GlobalForceControlQ460()
	{
		LTVector vForce(0.0f, -2000.0f, 0.0f);

		Set(&vForce);

		m_Unknown30 = -0.70709997f;
	}

	void Set(LTVector* pVec);                                // 0x00410e20

	LTObjectQ460* m_pObj;                                    // +0x00
	float         m_fFrameTime;                              // +0x04
	uint32        m_nObjFlags;                               // +0x08
	LTVector*     m_pVelocity;                               // +0x0c
	LTVector*     m_pAcceleration;                           // +0x10
	LTVector      m_Force;                                   // +0x14
	LTVector      m_Dir;                                     // +0x20
	float         m_fMag;                                    // +0x2c
	float         m_Unknown30;                               // +0x30
	LTVector      m_vFrameMove;                              // +0x34
};

// Section F's body; see there.
inline void GlobalForceControlQ460::Set(LTVector* pVec)
{
	m_Force = *pVec;

	m_fMag = m_Force.Mag();

	if (m_fMag > 1e-5f)
	{
		m_Dir = m_Force;
		m_Dir /= m_fMag;
	}
	else
	{
		m_Dir.Init();
	}
}

// lt2new1.cpp's name for 0x004500d0, restated so this TU stands alone.
void pm_UpdateMovementQ460(GlobalForceControlQ460* pMove);   // 0x004500d0


LTVector sub_4619a0Q460(LTObjectQ460* pObj, SPredictRecQ460* pRec,
                        LTVector* pForce, float fTime, float fMaxTime,
                        int bUsePhysics)
{
	if (bUsePhysics)
	{
		LTVector vSavePos         = pObj->m_Pos;
		LTVector vSaveVel         = pObj->m_Velocity;
		LTVector vSaveAccel       = pObj->m_Acceleration;
		uint32   nSaveChangeFlags = pObj->m_ChangeFlags;
		float    fSaveFriction    = pObj->m_FrictionCoefficient;

		pObj->SetPos(pRec->m_vPos);

		pObj->m_Velocity = pRec->m_vVel;

		pObj->m_Acceleration.Init();

		pObj->m_FrictionCoefficient = 0.0f;

		GlobalForceControlQ460 move;

		move.m_pObj          = pObj;
		move.m_fFrameTime    = fTime * fMaxTime;
		move.m_nObjFlags     = pObj->m_Flags;
		move.m_pVelocity     = &pObj->m_Velocity;
		move.m_pAcceleration = &pObj->m_Acceleration;
		move.m_Unknown30     = 1.0f;

		move.Set(pForce);

		pm_UpdateMovementQ460(&move);

		pObj->SetPos(vSavePos);

		pObj->m_Velocity            = vSaveVel;
		pObj->m_Acceleration        = vSaveAccel;
		pObj->m_ChangeFlags         = nSaveChangeFlags;
		pObj->m_FrictionCoefficient = fSaveFriction;

		return move.m_vFrameMove + pRec->m_vPos;
	}

	// 0x00461c21.  `flds arg5 ; fmuls arg6` happens ONCE and the product
	// lives in %st for the whole block (`fld %st(0)` three times) -- a
	// register-allocated local whose address is never taken.  It is declared
	// HERE and not at the top of the function: the physics arm computes the
	// same product for itself at 0x00461a9f.
	{
		float fDelta = fTime * fMaxTime;

		return pRec->m_vPos + pRec->m_vVel * fDelta +
		       *pForce * (fDelta * fDelta * 0.5f);
	}
}


// ======================================================================= //
//


// ======================================================================= //
//
//  D2. 0x00460d30 (65 bytes) -- free a client poly grid's two buffers
//
//  __cdecl, one argument, void.  Its only caller is the grid's (re)build
//  0x00460bf0 (clientmgr.h's sub_460bf0), whose first act is this call
//  (0x00460bfb) and which then dalloc()s +0x1a8 (the height bytes,
//  0x00460c3d) and +0x1ac (the index buffer, 0x00460c6f).  Each member is
//  tested, dfree()d (0x00429080) and nulled.  Its real home is the
//  poly-grid TU that owns 0x00460bf0; written here because this is the file
//  that names the address. (name ours)
// ======================================================================= //
class CPolyGridQ460
{
public:
	uint8   m_pad_000[0x1a8];
	char*   m_pData;                                         // 0x1a8
	uint16* m_pIndices;                                      // 0x1ac
	uint8   m_pad_1b0[0x1dc - 0x1b0];
	uint32  m_nTris;                                         // 0x1dc
	uint32  m_nIndices;                                      // 0x1e0
	uint8   m_pad_1e4[0x1f0 - 0x1e4];
	uint32  m_Width;                                         // 0x1f0
	uint32  m_Height;                                        // 0x1f4
};

void  dfree(void* pData);                                    // 0x00429080
void* dalloc(uint32 nSize);                                  // 0x00428fd0
void* dalloc_z(uint32 nSize);                                // 0x00429050
void  pg_FreeDataQ460(CPolyGridQ460* pGrid);                 // 0x00460d30


// ======================================================================= //
//  D1. 0x00460bf0 (320 bytes) -- clientmgr.h's sub_460bf0, the poly grid's
//  (re)build, called by ci_SetupPolyGrid.  Frees the old buffers
//  (0x00460bfb), refuses a dimension under 2 or more than 65000 samples
//  (0x00460c22 `cmp $0xfde8`), allocates the zeroed height bytes
//  (0x00460c2f dalloc_z) and the index buffer (0x00460c63), and fills it
//  with one or two triangles per quad (`neg/sbb/add $2` at 0x00460c39 is
//  `bHalfTriangles ? 1 : 2`; the second triangle at 0x00460cca is skipped
//  when bHalfTriangles).  Width/height are stored last (0x00460d0e).
// ======================================================================= //
LTBOOL pg_SetupGridQ460(CPolyGridQ460* pGrid, uint32 width, uint32 height,
                        LTBOOL bHalfTriangles)
{
	uint32  x, y, nTrisPerQuad, index, nXQuads, nYQuads;
	uint16* pIndex;

	pg_FreeDataQ460(pGrid);

	if (width < 2 || height < 2 || width * height > 65000)
		return LTFALSE;

	pGrid->m_pData = (char*)dalloc_z(width * height);

	nXQuads = width - 1;
	nYQuads = height - 1;

	nTrisPerQuad      = bHalfTriangles ? 1 : 2;
	pGrid->m_nTris    = nTrisPerQuad * nYQuads * nXQuads;
	pGrid->m_nIndices = pGrid->m_nTris * 3;
	pGrid->m_pIndices = (uint16*)dalloc(pGrid->m_nIndices * sizeof(uint16));

	for (y = 0; y < nYQuads; y++)
	{
		pIndex = &pGrid->m_pIndices[y * nTrisPerQuad * nXQuads * 3];

		for (x = 0; x < nXQuads; x++)
		{
			index = y * width + x;

			*pIndex++ = (uint16)(index + width + 1);
			*pIndex++ = (uint16)(index + 1);
			*pIndex++ = (uint16)index;

			if (!bHalfTriangles)
			{
				*pIndex++ = (uint16)(index + width);
				*pIndex++ = (uint16)(index + width + 1);
				*pIndex++ = (uint16)index;
			}
		}
	}

	pGrid->m_Width  = width;
	pGrid->m_Height = height;
	return LTTRUE;
}

void pg_FreeDataQ460(CPolyGridQ460* pGrid)
{
	if (pGrid->m_pData)
	{
		dfree(pGrid->m_pData);
		pGrid->m_pData = LTNULL;
	}

	if (pGrid->m_pIndices)
	{
		dfree(pGrid->m_pIndices);
		pGrid->m_pIndices = LTNULL;
	}
}


// ======================================================================= //
//
//  E.  0x0045aaa0 (255 bytes) -- LTObject::SetPos: written in its home,
//  src/engine/server/ltobject.cpp, where UpdateBounds is now `inline` and
//  defined above it -- lithtech.exe's /O2 SetPos expands it, server.dll's
//  /Os calls 0x10024a4c -- so one body matches both images.
// ======================================================================= //


// ======================================================================= //
//
//  F.  0x00410e20 (144 bytes) -- GlobalForceControl::Set, out of line
//
//  Home: src/engine/server/commonphysics.cpp / commonphysics.h, which
//  define it `inline` -- so our lithtech build expands it everywhere and
//  emits no body, while shipped lithtech.exe calls this out-of-line copy
//  from the CClientMgr constructor (0x00410abe, this = +0x4c4), the
//  CServerMgr constructor (0x00413608) and 0x004619a0 (0x00461a8d).  Its
//  position among CClientMgr's functions is the COMDAT the first caller's
//  TU emitted.  __thiscall, one pointer argument (`ret $0x4`).
//
//  The body differs from the server's in one read: the divisor is RE-LOADED
//  from m_fMag (0x00410e72 `flds 0x2c(%ecx)`), so the magnitude goes to the
//  member and the compare and the divides both read it from there.
// ======================================================================= //
// The class and its Set are defined above section C: 0x004619a0 constructs
// one and expands Set, so both have to be visible there.
