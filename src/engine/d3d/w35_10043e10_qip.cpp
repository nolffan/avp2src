// *********************************************************************** //
//
//  MODULE:   BIG_440E3 (.CPP)
//
//  PURPOSE:  One function.  The row calls it "0x100440e3, 3,729
//            bytes"; it is really **0x10043e10 .. 0x10044f64, 4,437
//            bytes**, and 0x100440e3 is an interior label inside it.
//            This is CD3DModelDraw's per-model RENDER-STATE / TEXTURE-
//            STAGE SETUP AND TEARDOWN -- the last thing the model
//            pipeline does before and after the piece renderer runs.
//
//  ===================================================================== //
//  THE EXTENT.  THE STATED START IS WRONG; THE FUNCTION IS 708 BYTES
//  BIGGER AT THE FRONT AND 15 BYTES SHORTER AT THE BACK.
//  ===================================================================== //
//
//  Five independent readings, all of them off the shipped bytes:
//
//    * 0x100440e3 IS NOT 16-BYTE ALIGNED and **nothing calls it**.  A scan
//      of all 551,824 bytes of .text for `e8` displacements resolving to
//      0x100440e3 finds none; the only `call` into this range in the whole
//      image is `call 0x10043e10` at 0x100193d8 (one caller).
//      0x100440e3 is reached only by `jmp`, from 0x100442ef, 0x100443d5,
//      0x10044404 and 0x10044f60, and by fall-through from 0x100440dd.
//
//    * THE PROLOGUE IS AT 0x10043e10 and it is icl's stack-realigning,
//      two-entry one -- byte for byte the prologue big17c14.cpp records
//      for 0x10017c14:
//          53 / 8b dc / 83 ec 08 / 83 e4 f0 / 83 c4 08 / eb 03
//          53 / 8b dc                 <- the second, already-aligned entry
//          55 / 55 / 8b 6b 04 / 89 6c 24 04 / 8b ec / 81 ec 60 01 00 00
//      and the epilogue at 0x1004413e..0x1004414c is its exact partner:
//          mov 0x4(%esp),%esi / mov 0x8(%esp),%edi /
//          mov %ebp,%esp / pop %ebp / **mov %ebx,%esp / pop %ebx** / ret
//      The `mov %ebx,%esp ; pop %ebx` tail can only unwind the `push %ebx ;
//      mov %esp,%ebx` at 0x10043e10/0x10043e1e.  A body starting at
//      0x100440e3 would return through a %ebx that was never set.
//
//    * THERE IS EXACTLY ONE `ret`, at 0x1004414c, and 3,096 bytes of code
//      sit BELOW it (0x1004414d..0x10044f64).  That is /O2 cold-block
//      sinking: every block down there is entered by a forward conditional
//      or unconditional jump from above the `ret` and leaves by a backward
//      `jmp` (0x1004417e -> 0x10044066, 0x10044188/0x1004418f ->
//      0x10043f38, 0x100441ce/0x100441fa -> 0x10043f52, 0x100442ef /
//      0x100443d5 / 0x10044404 / 0x10044f60 -> 0x100440e3).
//
//    * THE STATED RANGE BRANCHES BACKWARDS OUT OF ITSELF.  0x10044153,
//      0x1004417e, 0x100441ba, 0x100441f4 target 0x10044066; 0x10044188,
//      0x1004418f target 0x10043f38; 0x100441ce, 0x100441fa target
//      0x10043f52 -- all below 0x100440e3.  There is no second
//      `push %ebp ; mov %esp,%ebp` anywhere in 0x10043e10..0x10044f64.
//
//    * THE BACK END.  The last instruction is `jmp 0x100440e3` at
//      0x10044f60, ending at 0x10044f65.  0x10044f65..0x10044f6f is
//      icl's multi-byte lea-nop pad (`8d 74 26 00`, `8d bc 27 00 00 00
//      00`) and 0x10044f70 is `mov 0x4(%esp),%eax`, the four-byte stack
//      shim in front of 0x10044f74 -- the NEXT function, which this one
//      calls four times.  d3dbatcha.cpp already records that shim.
//
//    Every jump target in 0x10043e10..0x10044f64 lies inside the range and
//    nothing outside it branches in.  0x10043e10 is also exactly where
//    the enumerated preceding entry ends (0x10043e10 + 723 = 0x100440e3),
//    so the row is one function split in two at the wrong place.
//
//  ===================================================================== //
//  WHAT IT IS, and what proves it
//  ===================================================================== //
//
//  `this` is big17c14.cpp's **CD3DModelDraw**, the global object at
//  0x1008b090.  Six independent tells:
//
//    * THE CALLER.  0x100193d8 is the only `call 0x10043e10` in the image
//      and big17c14.cpp already names it: "0x100193d2..0x10019404 the
//      tail: 0x10043e10, restore g_f1008b908, bump 0x1008cea8 or
//      0x1008cea4", and its class declares `void sub_10043e10();` on
//      CD3DModelDraw.  The class name and the member spelling below are
//      copied from that file so the mangled symbol is the one it declares.
//
//    * THE TWO CALLEES BOTH NAME THIS `this` INDEPENDENTLY.
//      big48180.cpp (the projected-shadow pass) records "the caller
//      (0x10044111) gates it on `this->m_698 & 2`", and big44ff4.cpp (the
//      ModelBoxes debug wireframe) records "0x10044116 is
//      `mov 0x1008cab0,%eax ; test ; je ; mov %esi,%ecx ;
//      call 0x10044ff4`" -- 0x1008cab0 being big21f90.cpp's console
//      variable **ModelBoxes**.  Both call sites are in this function's
//      tail and both are on %esi = %ecx = `this`.
//
//    * ALPHABLENDENABLE IS SAVED AND RESTORED AROUND THE WHOLE BODY.
//      0x10043e74 is `GetRenderState(0x1b, &dwOld)` through vtable +0x54
//      and 0x10044102 is `SetRenderState(0x1b, dwOld)` through +0x50, with
//      the +0x50 slot's own identity fixed by big44ff4.cpp and d3dr5b.cpp.
//      0x1b = 27 = D3DRENDERSTATE_ALPHABLENDENABLE in D3D7's enum, which
//      also makes 0x18 = 24 = D3DRENDERSTATE_ALPHAREF (0x1004400e, handed
//      the texture record's +0x16) and 8 = D3DRENDERSTATE_FILLMODE (the
//      tail of 0x10044f74).
//
//    * THE TEXTURE-RECORD FIND/BIND BLOCK AT 0x10043ee1..0x10044065 IS
//      big2ef34.cpp's, big6d910.cpp's and big61a30.cpp's BLOCK FIELD FOR
//      FIELD -- +0x42 the stage, +0x30 the chain, +0x14 the frame stamp
//      against g_w1008bea8, +0x1c the LTLink moved to front of
//      g_p1008e634, `g_p1008cbcc->+0x48 +=` the record's +0x10, SetTexture
//      at +0x8c, ALPHAREF at +0x50, the inverse-size pair into
//      g_a1008e664[stage], the counter at 0x1008bd80, SetLOD at +0xbc,
//      and the same two `sub_10039bd0(pBlock, nStage, 1/0)` create arms.
//      It is reused here verbatim, not re-derived.
//
//    * 63 of the 69 INDIRECT CALLS ARE ONE IDIOM.  "63
//      indirect calls in 3.7 KB is probably the key to what this function
//      is" resolves completely: they are **seventeen Get/SetTextureStage-
//      State construct pairs plus seventeen SetTextureStageState restores,
//      through vtable slots +0x90 and +0x94 of the SAME device pointer
//      g_pD3DDevice**, in one branch of the function.  No function-pointer
//      table is involved; there is not one indirect call in this function
//      that is not a COM slot on 0x1008d988 or the SetLOD at +0xbc on a
//      surface.  The seventeen restores replay the seventeen saves in
//      EXACT REVERSE ORDER off seventeen consecutive 12-byte frame
//      records at -0x114, -0x108, -0xfc ... -0x4c: that is a
//      block-scope array of scoped save/restore objects whose constructor
//      and destructor are both inlined (there is no `call` for either).
//      d3dbatcha.cpp decoded the render-state twin of that class,
//      C1000adc0, at 0x1000adc0, with the same "Get is handed the
//      PARAMETER, Set is handed the MEMBER" shape; CStageStateSet below
//      is its texture-stage sibling.
//
//    * THE uv GENERATOR AT 0x10003ae0, stored into this->+0x42c at
//      0x10043eb6, reads 0x3d0(%ecx), 0x3d4, 0x3d8 / 0x3e0, 0x3e4, 0x3e8
//      and 0x45c, 0x460, 0x464, 0x468 -- and big17c14.cpp's CD3DModelDraw
//      has **m_Mat3d0 at +0x3d0**, an LTMatrix it fills two instructions
//      before it calls this function.  A projection through that matrix is
//      exactly what an environment-map uv generator is.
//
//    * THE BUMP-ENVIRONMENT ROTATION.  0x1004461a..0x1004466b is
//      `flds g_f1008bd28 ; flds g_f1008bd30 ; fpatan` -- one atan2 -- and
//      then fcos/fsin/fchs/fsin/fcos off a DOUBLE spill at -0x20(%ebp)
//      into four globals, which are then fed as raw dwords to stage 1's
//      states 7, 8, 9 and 10 in the order (cos, -sin, sin, cos).  Those
//      four states are D3DTSS_BUMPENVMAT00/01/10/11 and that is a 2x2
//      rotation matrix.  Stage 1's COLOROP in the same block is 0x16 = 22
//      = D3DTOP_BUMPENVMAP.  The branch is gated on 0x1008edf4.
//
//  ===================================================================== //
//  WHICH FLAG:  /O2.  Three of the /O2 tells, none against.
//  ===================================================================== //
//
//    * OUTGOING ARGUMENTS are `add $-N,%esp` then `mov %reg,K(%esp)` at
//      every one of the 81 call sites -- 0x10043e3c, 0x10043fc1,
//      0x10044079, 0x1004419b, 0x10044252, 0x1004430e, 0x1004440e,
//      0x10044467 ... .  There is not one argument `push` in the 4.4 KB.
//      (The five bare `push %edi` / `push %eax` at 0x10044068,
//      0x100442a2, 0x100442cc, 0x10044fb4-style sites are icl reserving
//      four bytes with a register it does not care about and then storing
//      the real argument with `mov %..,(%esp)`; the pushed value is
//      always overwritten.)
//    * COLD BLOCKS ARE SUNK PAST THE SINGLE EPILOGUE -- see THE EXTENT.
//      /Os cross-jumps instead.
//    * THE EPILOGUE IS NOT DUPLICATED, but there is only one exit, so that
//      signal is silent rather than against.
//    * `&local` is `lea -0x11c(%ebp),%eax`, but every such site is a FRAME
//      local, not a `&p->m`, so the third row of the table does not apply
//      here in either direction.
//
//  ===================================================================== //
//  THE FRAME -- `sub $0x160`, 352 bytes, and it decomposes exactly
//  ===================================================================== //
//
//      esp+0x04 / esp+0x08  = -0x15c / -0x158   %esi and %edi, SAVED WITH
//                       STORES into the bottom of the frame, not pushed
//                       (0x10043e32, 0x10043e36) -- icl's usual.
//      -0x138 -0x130 -0x128 -0x120   four dwords WRITTEN AND NEVER READ,
//                       one per bump-matrix value (0x10044680, 0x100446fd,
//                       0x1004477a, 0x100447f7).  Dead stores; see the
//                       note at the bottom.
//      -0x11c   4 B   dwOldAlphaBlend  (GetRenderState 0x1b out-param)
//      -0x118   4 B   pOldTexture      (GetTexture out-param, both arms)
//      -0x114 .. -0x4d  204 B = 17 x 12   the seventeen CStageStateSet
//                       objects, in construction order from the deepest
//                       slot up, exactly as declaration order requires
//      -0x48    8 B   the two-dword save area 0x10043664 fills and
//                       0x10043784 replays
//      -0x40    4 B   the int temp of `(float)m_bAlpha6a8` (`mov %eax,
//                       -0x40 ; fildl -0x40`)
//      -0x2c    4 B   spill of the stage-state value across a Get
//      -0x28    4 B   bNeedInit, coalesced with nStage (disjoint live
//                       ranges -- see the note at the bottom)
//      -0x24    4 B   the list head, coalesced with pRec (ditto)
//      -0x20    8 B   the double spill of the bump angle (`fstpl`), also
//                       used as a 4-byte pointer spill at 0x1004416c
//      -0x18    4 B   `this` spilled across the g_a1008e664 publish
//
//  ===================================================================== //
//  REGION MAP, with what pins each region
//  ===================================================================== //
//
//  0x10043e10..0x10043e3c  PROLOGUE.  Realigning two-entry form; frame
//      0x160; %esi = %ecx = `this`.
//
//  0x10043e3c..0x10043e77  m_n38 = -1, m_n34 = 0, m_n428 = 1, then
//      GetRenderState(0x1b, &dwOldAlphaBlend) through +0x54.
//
//  0x10043e77..0x10043ec5  THE ENVIRONMENT-MAP GATE.  `g_CV_EnvMapAll ||
//      m_n458` (0x1008cb68 is big21f90.cpp's **EnvMapAll**), then
//      `g_p1008cbcc->m_p104`, then the two texture-coordinate callbacks
//      into m_n434 / m_n42c and `this->sub_10008e10()`.  0x10003a80 and
//      0x10003ae0 are materialised as immediates, so they are code
//      addresses; 0x10003a80 copies a uv pair from %edx's +0x18/+0x1c and
//      0x10003ae0 runs a 3x3-plus-translate projection out of `this`'s
//      +0x3d0 matrix into +0x18/+0x1c -- i.e. they are the "copy uv" and
//      "environment-map uv" generators.
//
//  0x10043ec5..0x10043f52  FIND THIS STAGE'S TEXTURE RECORD.  The global
//      at 0x1008cbcc is RE-READ and its +0x104 RE-TESTED (0x10043ed9),
//      which is the doubled null test big2ef34.cpp records for the same
//      block -- icl 4.0 folds neither, so the source tests twice.  The
//      list test is likewise emitted twice (0x10043eee -> L_MakeRec,
//      0x10043efc -> pRec = NULL), the first having been folded because
//      "the loop never ran, so pRec is still null".  The two `break`
//      trampolines are sunk at 0x10044183 and 0x1004418d.
//
//  0x10043f52..0x10044066  BIND.  g_a1008e684[stage] published, the LRU
//      move-to-front of the +0x1c LTLink through g_p1008e634,
//      `g_p1008cbcc->+0x48 +=`, SetTexture at +0x8c, ALPHAREF at +0x50,
//      the counter at 0x1008bd80, the inverse-size pair into
//      g_a1008e664[stage], SetLOD at +0xbc.
//
//  0x10044066..0x1004409e  END OF THE BIND ARM.  m_n304 = 0,
//      `this->sub_10008da0(0)`, SetRenderState(0x1b, 1), bNeedInit = 0.
//
//  0x1004409e..0x100440e3  `if ((float)m_bAlpha6a8 < 255.0f)
//      SetRenderState(0x1b, 1);` -- 0x100a9cd0 is 0x437f0000 = 255.0f and
//      the compare is `fildl` of the zero-extended byte against it, so the
//      source's right operand is a FLOAT.  Then the second read of
//      g_CV_EnvMapAll forks into the big setup block.
//
//  0x100440e3..0x1004414d  THE TAIL AND THE ONLY EPILOGUE.
//      SetRenderState(0x1b, dwOldAlphaBlend); `if (m_n698 & 2)
//      sub_10048180()`; `if (g_CV_ModelBoxes) sub_10044ff4()`;
//      `if (m_n698 & 0x40) g_f1008b908 = 7.0f` (0x40e00000).
//
//  0x1004414d..0x10044194  SUNK: L_ElseLOD, the "already bound to this
//      stage" arm -- the SetLOD three statements again, which
//      big2ef34.cpp, big6d910.cpp and big61a30.cpp all record as
//      duplicated in their copies too.
//
//  0x10044194..0x100441ff  SUNK: L_MakeRec, the two sub_10039bd0 arms.
//
//  0x100441ff..0x100442f4  SUNK: THE DETAIL-TEXTURE STAGE-1 PATH, gated on
//      `!g_1008edf4 && g_1008ee24 && g_bValid1008d954 &&
//      m_pModel00->m_n14c && g_p1008da74`.  m_n428 = 2, GetTexture(1),
//      SetTexture(1, g_p1008da74), sub_10043664(save) [d3dr5c.cpp: save
//      stage 1's +0x01 and +0x0c and install 7/2/1/3],
//      sub_10044f74(0x10003aa0, 0x10003b50, 1), sub_10043784(save)
//      [0x10043780, below: replay them], SetTexture(1, pOldTexture).
//
//  0x100442f4..0x100443da  SUNK: THE m_n30 PATH.  m_n428 = 2,
//      SetTextureStageState(1,1,8) and (1,4,2), g_1008edf0 = 8,
//      sub_10044f74(0x10003a40, 0x10003ad0, 1), then (1,1,1) and (1,4,1).
//      8 = D3DTOP_ADDSIGNED going in, 1 = D3DTOP_DISABLE coming out.
//
//  0x100443da..0x10044409  SUNK: THE PLAIN PATH.  g_1008edf0 = 1,
//      sub_10044f74(0x10003a10, 0x10003ad0, bNeedInit).
//
//  0x10044409..0x10044462  SUNK: THE BUMP-ENVIRONMENT PATH opens.
//      m_n38 = 0x101, m_n428 = 2, GetTexture(2, &pOldTexture),
//      SetTexture(2, g_p1008da74).
//
//  0x10044462..0x1004461a  Four CStageStateSet on stage 1: states 0x0b=0,
//      0x01=0x16, 0x02=2, 0x03=1.
//
//  0x1004461a..0x1004465a  THE ROTATION.  atan2(g_f1008bd28, g_f1008bd30)
//      once, held as a DOUBLE at -0x20 and reloaded before each of the
//      four fcos/fsin.
//
//  0x1004465a..0x10044bf2  Thirteen more CStageStateSet: stage 1 states
//      7, 8, 9, 0x0a (the four matrix dwords), 0x10, 0x11; stage 2 states
//      0x0b=1, 0x01=7, 0x02=2, 0x03=1, 0x0c=3, 0x10=2, 0x11=2.
//
//  0x10044bf2..0x10044c38  sub_10044f74(0x10003aa0, 0x10003b50, 1) then
//      SetTexture(2, pOldTexture).
//
//  0x10044c38..0x10044f65  THE SEVENTEEN DESTRUCTORS, exact reverse of the
//      construction order, each `SetTextureStageState(m_nStage, m_nState,
//      m_nOld)` off its own 12-byte record.  Then `jmp 0x100440e3`.
//
//  There is NO `// UNDECODED:` region in this file.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// ======================================================================= //
//  THE DEVICE
//
//  IDirect3DDevice7 seen as C COM: the interface pointer is the first
//  outgoing dword AND the base the slot is fetched through
//  (`mov 0x1008d988,%eax ; mov (%eax),%eax ; call *0x50(%eax)`), which
//  d3dr5b.cpp records as "the C COM shape, not a C++ virtual".  Only the
//  six slots this function reaches are named; the identities are the ones
//  d3dr5b.cpp, d3dr5c.cpp, d3dbatcha.cpp and big44ff4.cpp already fixed
//  for +0x50, +0x54, +0x8c, +0x90 and +0x94, and +0x88 is +0x8c's
//  immediate neighbour used as its out-param twin (0x10044424 hands it
//  `lea -0x118(%ebp),%ecx` where 0x1004445c hands the same slot index a
//  surface by value).
// ======================================================================= //

struct D3DDev440e3;
struct DDSurface440e3;

struct DDSurface440e3Vtbl
{
	uint8	m_pad_00[0xbc];
	long (__stdcall* SetLOD)(DDSurface440e3*, uint32 dwLOD);     // +0xbc
};

struct DDSurface440e3
{
	DDSurface440e3Vtbl*	lpVtbl;
};

struct D3DDev440e3Vtbl
{
	uint8	m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDev440e3*, uint32 dwState,
	                                 uint32 dwValue);            // +0x50
	long (__stdcall* GetRenderState)(D3DDev440e3*, uint32 dwState,
	                                 uint32* lpdwValue);         // +0x54
	uint8	m_pad_58[0x88 - 0x58];
	long (__stdcall* GetTexture)(D3DDev440e3*, uint32 dwStage,
	                             DDSurface440e3** lplpTex);      // +0x88
	long (__stdcall* SetTexture)(D3DDev440e3*, uint32 dwStage,
	                             DDSurface440e3* lpTex);         // +0x8c
	long (__stdcall* GetTextureStageState)(D3DDev440e3*, uint32 dwStage,
	                                       uint32 dwState,
	                                       uint32* lpdwValue);   // +0x90
	long (__stdcall* SetTextureStageState)(D3DDev440e3*, uint32 dwStage,
	                                       uint32 dwState,
	                                       uint32 dwValue);      // +0x94
};

struct D3DDev440e3
{
	D3DDev440e3Vtbl*	lpVtbl;
};

extern D3DDev440e3*		g_pD3DDevice;                   // 0x1008d988

// D3D7's D3DRENDERSTATETYPE, only the three that appear.
#define D3DRS440_FILLMODE				8
#define D3DRS440_ALPHAREF				0x18
#define D3DRS440_ALPHABLENDENABLE		0x1b

// The texture-stage state indices are left as RAW NUMBERS at every use.
// d3dr5c.cpp reads 0x0c as D3DTSS_ALPHAOP where the published D3D7 header
// has 4 there and 0x0c at D3DTSS_ADDRESS; nothing in these bytes settles
// which numbering the shipped header used, so no name is asserted.

// The DX SDK's own float-to-dword macro.  Every bump-matrix value below is
// loaded out of a float global with an INTEGER `mov` and handed straight to
// SetTextureStageState (0x100446e2, 0x1004475f, 0x100447dc, 0x1004465f),
// which is what F2DW compiles to.
#define F2DW440(f)		(*(uint32*)&(f))


// ======================================================================= //
//  THE SCOPED TEXTURE-STAGE-STATE SAVE
//
//  Seventeen 12-byte records at -0x114, -0x108, -0xfc, -0xf0, -0xe4,
//  -0xd8, -0xcc, -0xc0, -0xb4, -0xa8, -0x9c, -0x90, -0x84, -0x78, -0x6c,
//  -0x60, -0x54.  Field order is read off the stores: +0x00 is the stage
//  (`movl $0x1,-0xd8(%ebp)` before the Get), +0x04 the state, +0x08 the
//  Get's out-param (`lea -0xd0(%ebp),%ecx` into 0xc(%esp)).
//
//  It has to be a class with an inlined constructor and destructor and not
//  a run of open-coded statements, because the SEVENTEEN restores at
//  0x10044c38..0x10044f60 replay the seventeen constructions in EXACT
//  REVERSE ORDER -- which is destructor order for block-scope objects and
//  nothing else.  There is no `call` for either half, so both bodies are
//  defined in-class (implicitly inline, which is what /Ob1 needs).
//
//  The argument-vs-member split is d3dbatcha.cpp's, decoded there on the
//  render-state twin C1000adc0 at 0x1000adc0: the Get is handed the
//  PARAMETERS and the Set is handed the MEMBERS reloaded from the object.
//  The shipped bytes here say the same thing -- the Get's stage and state
//  are the live immediate/register (`movl $0x1,0x4(%esp)` /
//  `mov %eax,0x8(%esp)`) and the Set's are `mov -0xd8(%ebp),%eax` and
//  `mov -0xd4(%ebp),%eax`.
// ======================================================================= //

class CStageStateSet440e3
{
public:
	inline CStageStateSet440e3(uint32 nStage, uint32 nState, uint32 nValue)
	{
		m_nStage00 = nStage;
		m_nState04 = nState;

		g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice,
			nStage, nState, &m_nOld08);

		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
			m_nStage00, m_nState04, nValue);
	}

	inline ~CStageStateSet440e3()
	{
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
			m_nStage00, m_nState04, m_nOld08);
	}

	uint32	m_nStage00;                                 // +0x00
	uint32	m_nState04;                                 // +0x04
	uint32	m_nOld08;                                   // +0x08
};


// ======================================================================= //
//  THE TEXTURE-RECORD SUBSYSTEM
//
//  Copied from big2ef34.cpp, which decoded the identical block; every
//  offset below is also a load or a store in THESE bytes, listed against
//  its site.  big6d910.cpp and big61a30.cpp carry the same structures.
// ======================================================================= //

// ltlink.h's link, prev at +0x00 and next at +0x04 (0x10043f7a stores the
// next through the prev, 0x10043f83 the prev through the next).
struct Link440e3
{
	Link440e3*	m_pPrev;                                // +0x00
	Link440e3*	m_pNext;                                // +0x04
};

struct Tex440e3
{
	uint32			m_n00;                              // +0x00
	float			m_fInvW04;                          // +0x04  0x10044022
	float			m_fInvH08;                          // +0x08  0x10044025
	DDSurface440e3*	m_pSurface0c;                       // +0x0c  0x10043fd7
	uint32			m_nBytes10;                         // +0x10  0x10043fac
	uint16			m_nStamp14;                         // +0x14  0x10043f69
	uint16			m_nAlphaRef16;                      // +0x16  0x10043fe9
	uint8			m_pad_18[0x1c - 0x18];
	Link440e3		m_Link1c;                           // +0x1c  0x10043f71
	uint8			m_pad_24[0x30 - 0x24];
	Tex440e3*		m_pNext30;                          // +0x30  0x10043f1c
	uint8			m_pad_34[0x42 - 0x34];
	uint16			m_nStage42;                         // +0x42  0x10043f10
	uint16			m_nDirty44;                         // +0x44  0x1004403c
};

// The block: a list head at +0x0c (0x10043ee8) and a sixteen-bit stamp at
// +0x30 (`mov %ax,0x30(%ecx)` at 0x10043ef0).
struct TexBlock440e3
{
	uint8			m_pad_00[0x0c];
	Tex440e3*		m_pHead0c;                          // +0x0c
	uint8			m_pad_10[0x30 - 0x10];
	uint16			m_nStamp30;                         // +0x30
};

// The pair published per stage, `mov %ecx,0x1008e668(,%eax,8)` and
// `mov %esi,0x1008e664(,%eax,8)` at 0x1004402e / 0x10044035.
struct TexScale440e3
{
	float		m_fInvW00;                              // +0x00
	float		m_fInvH04;                              // +0x04
};

// 0x1008cbcc.  big17c14.cpp's D3DGlobalLight seen from here: +0x104 is the
// texture block this function binds through (0x10043e9c, 0x10043ed0) and
// +0x48 the byte counter the LRU bump adds to (0x10043faf).
struct Stats440e3
{
	uint8			m_pad_00[0x48];
	uint32			m_nBytes48;                         // +0x48
	uint8			m_pad_4c[0x104 - 0x4c];
	TexBlock440e3*	m_pBlock104;                        // +0x104
};


// ======================================================================= //
//  GLOBALS.  Every one is an address these bytes touch.
// ======================================================================= //

extern uint16			g_w1008b870;                    // 0x1008b870
extern float			g_f1008b908;                    // 0x1008b908  near
extern float			g_f1008bd28;                    // 0x1008bd28
extern float			g_f1008bd30;                    // 0x1008bd30
extern uint32			g_n1008bd80;                    // 0x1008bd80
extern uint16			g_w1008bea8;                    // 0x1008bea8
extern int32			g_CV_ModelBoxes;                // 0x1008cab0
extern int32			g_CV_EnvMapAll;                 // 0x1008cb68
extern Stats440e3*		g_p1008cbcc;                    // 0x1008cbcc
extern uint32			g_n1008d7d0;                    // 0x1008d7d0  stage
extern int32			g_bValid1008d954;               // 0x1008d954
extern DDSurface440e3*	g_p1008da74;                    // 0x1008da74
extern uint32			g_n1008edf0;                    // 0x1008edf0
extern int32			g_n1008edf4;                    // 0x1008edf4
extern int32			g_n1008ee24;                    // 0x1008ee24
extern Link440e3*		g_p1008e634;                    // 0x1008e634
extern TexScale440e3	g_a1008e664[];                  // 0x1008e664
extern Tex440e3*		g_a1008e684[];                  // 0x1008e684

// The four bump-matrix cells.  0x10089550/0x10089554 are adjacent and
// 0x1008edf8/0x1008edfc are adjacent, but the four are used as m00, m01,
// m10, m11 in the order 0x10089554, 0x1008edf8, 0x1008edfc, 0x10089550,
// so no array shape is asserted.
extern float			g_f10089550;                    // 0x10089550
extern float			g_f10089554;                    // 0x10089554
extern float			g_f1008edf8;                    // 0x1008edf8
extern float			g_f1008edfc;                    // 0x1008edfc


// ======================================================================= //
//  THE FREE FUNCTIONS THIS ONE CALLS.  Declared and never defined, so no
//  byte here is claimed at any of their addresses.
// ======================================================================= //

// 0x10039bd0 -- d3dmid.cpp's stack-ABI entry (body at 0x10039bdc); the
// same three arguments big2ef34.cpp and big6d910.cpp hand it.
extern "C" Tex440e3* sub_10039bd0(TexBlock440e3* pBlock, uint32 nStage,
                                  uint32 bLink);

// 0x10043780 / 0x10043784 is defined below, beside 0x10043660 (moved in
// from d3dr5b.cpp).

// ----------------------------------------------------------------------- //
//  0x10043660 / 0x10043664 -- MOVED here from d3dr5c.cpp.  Save
//  stage 1's colour and alpha ops (states 0x01, 0x0c) into a two-dword
//  record and install 7/2/1/3, recording mode 7 in g_1008edf0.  It is a
//  /Qip two-entry pair and its one register-entry caller is 0x10043e10
//  below (`call 0x10043664` at 0x1004429d, the argument in %eax), so it
//  lives in this TU, which is therefore /O2 /Qip (the _qip suffix; it was
//  big43e10.cpp at /O2).  Measured: the shim and 0x10043664 IDENTICAL
//  (was 214 bytes off as a stack-only /Os function in d3dr5c.cpp; the
//  shipped body is /O2-shaped -- frame-slot %ebx save, stored arguments);
//  0x10043e10's shim stays identical and its body goes 3587 -> 3625 bytes
//  off at /Qip (same with /Qip_no_inlining).  The IDirect3DDevice7 slots
//  are d3dr5c.cpp's spelling.
// ----------------------------------------------------------------------- //
struct D3DDevTSSR5C;
struct D3DDevTSSR5CVtbl
{
	void*	m_Slot00[36];                                       // +0x00..+0x8c
	long	(__stdcall* GetTextureStageState)(D3DDevTSSR5C*,
					uint32 dwStage, uint32 dwState, uint32* pdwValue);  // +0x90
	long	(__stdcall* SetTextureStageState)(D3DDevTSSR5C*,
					uint32 dwStage, uint32 dwState, uint32 dwValue);    // +0x94
};
struct D3DDevTSSR5C
{
	D3DDevTSSR5CVtbl*	lpVtbl;
};

extern D3DDevTSSR5C*	g_1008d988;                     // 0x1008d988
extern uint32			g_1008edf0;                     // 0x1008edf0

void sub_10043660(uint32* pSave)
{
	g_1008d988->lpVtbl->GetTextureStageState(g_1008d988, 1, 1, pSave);
	g_1008d988->lpVtbl->GetTextureStageState(g_1008d988, 1, 0xc, pSave + 1);

	g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, 1, 1, 7);
	g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, 1, 2, 2);
	g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, 1, 3, 1);
	g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, 1, 0xc, 3);

	g_1008edf0 = 7;
}

// ----------------------------------------------------------------------- //
//  0x10043780 / 0x10043784 -- MOVED here from d3dr5b.cpp.
//  Replay the two stage-1 values 0x10043660 saved: (1, 0x01) and
//  (1, 0x0c), the device re-read for the second call.  The /Qip pair's
//  register-entry caller is 0x10043e10 below (`call 0x10043784` at
//  0x100442c7, in its cold tail; 0x100440e3 is an interior label of
//  0x10043e10, not a function), its stack-entry caller 0x10032935.  Its
//  body is the d3dr5b.cpp one with the record as the uint32[2] the
//  caller passes.  Measured: the shim and 0x10043784 IDENTICAL (81 bytes
//  off as a stack-only /Os function in d3dr5b.cpp); 0x10043e1e 3625 ->
//  3597 bytes off.
// ----------------------------------------------------------------------- //
void sub_10043780(uint32* pSave)
{
	g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, 1, 1, pSave[0]);
	g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, 1, 0xc, pSave[1]);
}

// The texture-coordinate generators stored into m_n434 / m_n42c.  All four
// are materialised as immediates (`movl $0x10003a80,0x434(%esi)` and the
// four `movl $0x10003xxx,K(%esp)` argument stores), so they are code
// addresses.  Their bodies take `this` in %ecx, a second object in %edx
// and one stack argument, and write a uv pair to +0x18/+0x1c (and
// +0x20/+0x24 for 0x10003aa0); no C declaration reproduces that register
// pair, and none is needed -- only their ADDRESSES are used here.  They
// are declared __fastcall with their 12 argument bytes (%ecx, %edx, one
// stack word -- `ret $4` in each) so the names agree with the definitions
// in w2_10003a10.cpp (@sub_10003a10@12 ...).
extern "C" void __fastcall sub_10003a10(void*, void*, void*); // 0x10003a10
extern "C" void __fastcall sub_10003a40(void*, void*, void*); // 0x10003a40
extern "C" void __fastcall sub_10003a80(void*, void*, void*); // 0x10003a80
extern "C" void __fastcall sub_10003aa0(void*, void*, void*); // 0x10003aa0
extern "C" void __fastcall sub_10003ad0(void*, void*, void*); // 0x10003ad0
extern "C" void __fastcall sub_10003ae0(void*, void*, void*); // 0x10003ae0
extern "C" void __fastcall sub_10003b50(void*, void*, void*); // 0x10003b50


// ======================================================================= //
//  `this` -- big17c14.cpp's CD3DModelDraw, the global object at
//  0x1008b090.  The class TAG NAME and every field name that file already
//  fixed are copied VERBATIM so the mangled symbol of the definition below
//  is the one it declares; the six offsets it left inside padding
//  (+0x34, +0x38, +0x304, +0x428, +0x42c, +0x434, +0x698) are spelled out
//  here, each against the instruction that reaches it.
// ======================================================================= //

// this->m_pModel00, only its +0x14c (0x10044228).
struct D3DModel440e3
{
	uint8		m_pad_000[0x14c];
	uint32		m_n14c;                                 // +0x14c
};

class CD3DModelDraw
{
public:
	void			sub_10043e10();                     // 0x10043e10  THIS

	// Members of the same class defined elsewhere in the module.  Declared
	// and not defined so that /Ob1 has nothing to fold and this file
	// claims no bytes at their addresses.
	void			sub_10008da0(uint32 nValue);        // 0x10008da0
	void			sub_10008e10();                     // 0x10008e10
	void			sub_10044f74(void* pfnA, void* pfnB,
			                     uint32 bInit);         // 0x10044f74
	void			sub_10044ff4();                     // 0x10044ff4
	void			sub_10048180();                     // 0x10048180

	D3DModel440e3*	m_pModel00;                         // +0x000
	uint8			m_pad_004[0x030 - 0x004];           // +0x004
	uint32			m_n030;                             // +0x030  0x100442f4
	uint32			m_n034;                             // +0x034  0x10043e4c
	uint32			m_n038;                             // +0x038  0x10043e3f
	uint8			m_pad_03c[0x304 - 0x03c];           // +0x03c
	uint32			m_n304;                             // +0x304  0x1004406e
	uint8			m_pad_308[0x428 - 0x308];           // +0x308
	uint32			m_n428;                             // +0x428  0x10043e53
	void*			m_pfn42c;                           // +0x42c  0x10043eb6
	uint32			m_n430;                             // +0x430
	void*			m_pfn434;                           // +0x434  0x10043eaa
	uint8			m_pad_438[0x458 - 0x438];           // +0x438
	uint32			m_n458;                             // +0x458  0x10043e89
	uint8			m_pad_45c[0x698 - 0x45c];           // +0x45c
	uint32			m_n698;                             // +0x698  0x10044105
	uint8			m_pad_69c[0x6a8 - 0x69c];           // +0x69c
	uint8			m_bAlpha6a8;                        // +0x6a8  0x1004409e
};


// ----------------------------------------------------------------------- //
//  0x10043e10 .. 0x10044f64 -- 4,437 bytes.  __thiscall, no arguments,
//  plain `c3` (no `ret $N`), void.  ONE caller, 0x100193d8, which
//  big17c14.cpp decodes as the tail of CD3DModelDraw::sub_10017c14.
//
//  The realigning two-entry prologue at 0x10043e10/0x10043e1e is icl's own
//  and is not spellable in C; big17c14.cpp records the identical shape on
//  the caller.  It is what the `mov %ebx,%esp ; pop %ebx` in the epilogue
//  unwinds, and the `fstpl -0x20(%ebp)` in the bump block is the eight-byte
//  local that asks for it.
// ----------------------------------------------------------------------- //
void CD3DModelDraw::sub_10043e10()
{
	// Declaration order is frame order, deepest slot first: -0x11c then
	// -0x118 (0x10043e46 and 0x10044255 / 0x10044424).
	uint32			dwOldAlphaBlend;                    // -0x11c
	DDSurface440e3*	pOldTexture;                        // -0x118

	// The rest are register-resident or coalesced temps; their slots are
	// listed in the frame map in the banner.
	uint32			nStage;
	uint32			bNeedInit;
	TexBlock440e3*	pBlock;
	Tex440e3*		pHead;
	Tex440e3*		pRec;
	Tex440e3*		pCur;

	// -- 0x10043e3f: three stores, HIGHEST OFFSET LAST is NOT what the
	//    bytes show -- they go +0x38, +0x34, +0x428, which is neither
	//    ascending nor descending, so it is source order.
	m_n038 = 0xffffffff;
	m_n034 = 0;
	m_n428 = 1;

	// -- 0x10043e74: vtable +0x54.
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice,
		D3DRS440_ALPHABLENDENABLE, &dwOldAlphaBlend);

	// -- 0x10043e77: `mov $1,%eax ; mov %eax,-0x28(%ebp)`, planted BEFORE
	//    the EnvMapAll test and only ever read again at 0x100443ef, where
	//    it is sub_10044f74's third argument.  On the arm below it is
	//    cleared to 0; on the two early-out arms it survives as 1.
	bNeedInit = 1;

	// ------------------------------------------------------------------ //
	//  0x10043e7c -- THE GATE.  0x1008cb68 is big21f90.cpp's console
	//  variable EnvMapAll; `jne` over the m_n458 test is the `||`.  The
	//  block pointer is read and tested here and READ AND TESTED AGAIN at
	//  0x10043ec5/0x10043ed9 -- icl 4.0 folds neither, so the source
	//  really does name g_p1008cbcc->m_pBlock104 twice, and the two tests
	//  have DIFFERENT targets (0x1004409e and 0x10044066), which is what
	//  makes them two source tests and not one.
	// ------------------------------------------------------------------ //
	if ((g_CV_EnvMapAll || m_n458) && g_p1008cbcc->m_pBlock104)
	{
		// -- 0x10043eaa: both are `movl $imm,K(%esi)`, so they are code
		//    addresses stored into the two uv-generator slots.  +0x434 is
		//    written first.
		m_pfn434 = (void*)sub_10003a80;
		m_pfn42c = (void*)sub_10003ae0;

		// -- 0x10043ec0: `mov %esi,%ecx ; call 0x10008e10`.
		sub_10008e10();

		// -------------------------------------------------------------- //
		//  0x10043ec5 -- FIND THIS STAGE'S TEXTURE RECORD.  This is
		//  big2ef34.cpp's block, reused rather than re-derived; the only
		//  differences from that copy are that the block pointer comes
		//  from g_p1008cbcc->+0x104 instead of the object, and that there
		//  is no ShowSkySplits unbind arm here.
		// -------------------------------------------------------------- //
		nStage = g_n1008d7d0;
		pBlock = g_p1008cbcc->m_pBlock104;

		if (pBlock)
		{
			pBlock->m_nStamp30 = g_w1008b870;

			// 0x10043ee8 -- loaded ONCE into -0x24 and reused at
			// L_MakeRec (0x10044194, 0x100441c0), where big2ef34.cpp's
			// copy re-reads the field instead; see the coin-flips.
			pHead = pBlock->m_pHead0c;

			pRec = LTNULL;

			// The list test is emitted TWICE, 0x10043eee and 0x10043efc.
			// The first goes straight to L_MakeRec, icl having folded
			// "the loop never ran, so pRec is still null"; big2ef34.cpp
			// and big61a30.cpp record the same pair.
			if (pHead)
			{
				pCur = pHead;

				while (pCur)
				{
					// `cmp %edx,%edi` at 0x10043f14 with the RECORD's
					// field in %edi -- the source's left operand.  The
					// two `break` trampolines are sunk at 0x10044183 and
					// 0x1004418d.
					if (pCur->m_nStage42 == (uint8)nStage)
					{
						pRec = pCur;
						break;
					}

					pCur = pCur->m_pNext30;
				}
			}

			if (!pRec)
				goto L_MakeRec;                 // 0x10043f3c

			if (pRec == g_a1008e684[nStage])
				goto L_ElseLOD;                 // 0x10043f4c

L_Bind:
			// ---------------------------------------------------------- //
			//  0x10043f52 -- BIND.
			// ---------------------------------------------------------- //
			g_a1008e684[pRec->m_nStage42] = pRec;

			if (pRec->m_nStamp14 != g_w1008bea8)
			{
				// ltlink.h:100..145 dl_Remove / dl_TieOff / dl_Insert,
				// 0x10043f71..0x10043fa6, prev at +0x00 and next at +0x04.
				pRec->m_Link1c.m_pPrev->m_pNext = pRec->m_Link1c.m_pNext;
				pRec->m_Link1c.m_pNext->m_pPrev = pRec->m_Link1c.m_pPrev;
				pRec->m_Link1c.m_pPrev = &pRec->m_Link1c;
				pRec->m_Link1c.m_pNext = &pRec->m_Link1c;

				pRec->m_Link1c.m_pPrev = g_p1008e634;
				pRec->m_Link1c.m_pNext = g_p1008e634->m_pNext;
				pRec->m_Link1c.m_pNext->m_pPrev = &pRec->m_Link1c;
				pRec->m_Link1c.m_pPrev->m_pNext = &pRec->m_Link1c;

				g_p1008cbcc->m_nBytes48 += pRec->m_nBytes10;

				pRec->m_nStamp14 = g_w1008bea8;
			}

			// 0x10043fe0 -- vtable +0x8c, the stage TRUNCATED TO A BYTE
			// (`movzbl %al,%eax` at 0x10043fd0).
			g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice,
				(uint8)pRec->m_nStage42, pRec->m_pSurface0c);

			if (pRec->m_nAlphaRef16)
			{
				// 0x1004400e -- vtable +0x50, state 0x18.
				g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
					D3DRS440_ALPHAREF, pRec->m_nAlphaRef16);
			}

			// 0x10044015 -- NOTE THE ORDER.  This copy loads the counter
			// into a register, increments it and STORES IT BACK BEFORE the
			// pair goes out (`mov 0x1008bd80,%edx ; inc %edx ;
			// mov %edx,0x1008bd80` at 0x10044015/0x1004401e/0x10044028,
			// then the two stores at 0x1004402e/0x10044035).
			// big2ef34.cpp's and big6d910.cpp's copies emit `incl
			// 0x1008bd80` in memory AFTER the pair (0x10030777 and
			// 0x1006f2ba), which is what the opposite source order gives.
			// So this function's statement order is counter first.
			g_n1008bd80++;

			g_a1008e664[pRec->m_nStage42].m_fInvW00 = pRec->m_fInvW04;
			g_a1008e664[pRec->m_nStage42].m_fInvH04 = pRec->m_fInvH08;

			if (pRec->m_nDirty44)
			{
				// 0x1004405a -- the surface's own vtable +0xbc.
				pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
				pRec->m_nDirty44 = 0;
			}

			goto L_Done;                        // 0x10044045 / fallthrough

L_ElseLOD:
			// -- 0x1004414d (SUNK): already bound to this stage, so only
			//    the LOD reset survives.  The same three statements as the
			//    tail of L_Bind; big2ef34.cpp, big6d910.cpp and
			//    big61a30.cpp all record the same duplication.
			if (pRec->m_nDirty44)
			{
				pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
				pRec->m_nDirty44 = 0;
			}

			goto L_Done;                        // 0x1004417e

L_MakeRec:
			// -- 0x10044194 (SUNK): a THIRD test of the same list head,
			//    and the third argument is 1 when the list is non-empty
			//    and 0 when it is empty.
			if (pHead)
			{
				pRec = sub_10039bd0(pBlock, nStage, 1);

				if (!pRec)
					goto L_Done;                // 0x100441ba

				pRec->m_pNext30 = pHead->m_pNext30;
				pHead->m_pNext30 = pRec;

				goto L_Bind;                    // 0x100441ce
			}

			// -- 0x100441d3: the empty-list arm.
			pRec = sub_10039bd0(pBlock, nStage, 0);

			if (!pRec)
				goto L_Done;                    // 0x100441f4

			goto L_Bind;                        // 0x100441fa
		}

L_Done:
		// -- 0x10044066 ------------------------------------------------ //
		m_n304 = 0;

		sub_10008da0(0);

		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS440_ALPHABLENDENABLE, 1);

		bNeedInit = 0;
	}

	// ------------------------------------------------------------------ //
	//  0x1004409e -- `movzbl 0x6a8(%esi),%eax ; mov %eax,-0x40(%ebp) ;
	//  fildl -0x40(%ebp) ; fcomps 0x100a9cd0 ; fnstsw ; sahf ; jae`.
	//  0x100a9cd0 holds 0x437f0000 = 255.0f.  The `fildl` is the tell: the
	//  right operand of the comparison is a FLOAT, so the byte is promoted
	//  rather than compared as an integer.
	// ------------------------------------------------------------------ //
	if ((float)m_bAlpha6a8 < 255.0f)
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRS440_ALPHABLENDENABLE, 1);
	}

	// -- 0x100440d6: EnvMapAll read a SECOND time.
	if (!g_CV_EnvMapAll)
	{
		if (g_n1008edf4)
		{
			// ---------------------------------------------------------- //
			//  0x10044409 (SUNK) -- THE BUMP-ENVIRONMENT PATH.
			// ---------------------------------------------------------- //
			double	fAngle;                             // -0x20 (fstpl)

			m_n038 = 0x101;
			m_n428 = 2;

			// 0x10044437 / 0x1004445c -- vtable +0x88 then +0x8c, stage 2.
			g_pD3DDevice->lpVtbl->GetTexture(g_pD3DDevice, 2,
				&pOldTexture);

			g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 2,
				g_p1008da74);

			// The seventeen scoped saves.  Every (stage, state, value)
			// triple below is read off its own construction site; the
			// frame slot each object lands in is given so the declaration
			// order can be checked against the destructor order at
			// 0x10044c38..0x10044f60.
			CStageStateSet440e3	ss00(1, 0x0b, 0);       // -0x114
			CStageStateSet440e3	ss01(1, 0x01, 0x16);    // -0x108
			CStageStateSet440e3	ss02(1, 0x02, 2);       // -0xfc
			CStageStateSet440e3	ss03(1, 0x03, 1);       // -0xf0

			// -- 0x1004461a: `flds 0x1008bd28 ; flds 0x1008bd30 ; fpatan`
			//    is atan2(ST(1), ST(0)) = atan2(g_f1008bd28,
			//    g_f1008bd30), computed ONCE and spilled as a DOUBLE at
			//    -0x20, then reloaded before each of the four
			//    fcos/fsin/fchs.
			fAngle = atan2(g_f1008bd28, g_f1008bd30);

			g_f10089554 = (float)cos(fAngle);
			g_f1008edf8 = (float)-sin(fAngle);
			g_f1008edfc = (float)sin(fAngle);
			g_f10089550 = (float)cos(fAngle);

			// States 7, 8, 9, 0x0a fed the four dwords in the order
			// (cos, -sin, sin, cos) -- a 2x2 rotation.  Each value is
			// re-read out of its global with an INTEGER `mov`
			// (0x1004465f, 0x100446e2, 0x1004475f, 0x100447dc).
			CStageStateSet440e3	ss04(1, 0x07,
			                         F2DW440(g_f10089554));  // -0xe4
			CStageStateSet440e3	ss05(1, 0x08,
			                         F2DW440(g_f1008edf8));  // -0xd8
			CStageStateSet440e3	ss06(1, 0x09,
			                         F2DW440(g_f1008edfc));  // -0xcc
			CStageStateSet440e3	ss07(1, 0x0a,
			                         F2DW440(g_f10089550));  // -0xc0
			CStageStateSet440e3	ss08(1, 0x10, 2);       // -0xb4
			CStageStateSet440e3	ss09(1, 0x11, 2);       // -0xa8
			CStageStateSet440e3	ss10(2, 0x0b, 1);       // -0x9c
			CStageStateSet440e3	ss11(2, 0x01, 7);       // -0x90
			CStageStateSet440e3	ss12(2, 0x02, 2);       // -0x84
			CStageStateSet440e3	ss13(2, 0x03, 1);       // -0x78
			CStageStateSet440e3	ss14(2, 0x0c, 3);       // -0x6c
			CStageStateSet440e3	ss15(2, 0x10, 2);       // -0x60
			CStageStateSet440e3	ss16(2, 0x11, 2);       // -0x54

			// 0x10044c0e then 0x10044c32; the seventeen destructors follow
			// at 0x10044c38 and run in exact reverse.
			sub_10044f74((void*)sub_10003aa0, (void*)sub_10003b50, 1);

			g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 2,
				pOldTexture);
		}
		else if (g_n1008ee24 && g_bValid1008d954 &&
		         m_pModel00->m_n14c && g_p1008da74)
		{
			// ---------------------------------------------------------- //
			//  0x10044243 (SUNK) -- THE DETAIL-TEXTURE STAGE-1 PATH.
			// ---------------------------------------------------------- //
			uint32	aStageSave[2];                      // -0x48

			m_n428 = 2;

			g_pD3DDevice->lpVtbl->GetTexture(g_pD3DDevice, 1,
				&pOldTexture);

			g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 1,
				g_p1008da74);

			// 0x1004429d / 0x100442c7 -- the pair d3dr5c.cpp and
			// d3dr5b.cpp decoded; the SAME eight-byte record goes to both.
			sub_10043660(aStageSave);

			sub_10044f74((void*)sub_10003aa0, (void*)sub_10003b50, 1);

			sub_10043780(aStageSave);

			g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 1,
				pOldTexture);
		}
		else if (m_n030)
		{
			// ---------------------------------------------------------- //
			//  0x100442ff (SUNK) -- 8 in, 1 out on stage 1's states 1 and
			//  4.  Under D3D7's published numbering that is COLOROP
			//  D3DTOP_ADDSIGNED and ALPHAOP D3DTOP_SELECTARG1 going in,
			//  D3DTOP_DISABLE on both coming out.
			// ---------------------------------------------------------- //
			m_n428 = 2;

			g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
				1, 1, 8);
			g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
				1, 4, 2);

			g_n1008edf0 = 8;

			sub_10044f74((void*)sub_10003a40, (void*)sub_10003ad0, 1);

			g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
				1, 1, 1);
			g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
				1, 4, 1);
		}
		else
		{
			// ---------------------------------------------------------- //
			//  0x100443da (SUNK) -- THE PLAIN PATH.  This is the ONLY
			//  reader of bNeedInit.
			// ---------------------------------------------------------- //
			g_n1008edf0 = 1;

			sub_10044f74((void*)sub_10003a10, (void*)sub_10003ad0,
				bNeedInit);
		}
	}

	// ------------------------------------------------------------------ //
	//  0x100440e3 -- THE TAIL.  Four `jmp`s and one fall-through reach it;
	//  it is the label the function listing mistook for a function entry.
	// ------------------------------------------------------------------ //
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRS440_ALPHABLENDENABLE, dwOldAlphaBlend);

	// 0x10044105 -- `test $0x2,%al` on the low byte of m_n698.
	if (m_n698 & 2)
		sub_10048180();

	// 0x10044116 -- big21f90.cpp's console variable ModelBoxes.
	if (g_CV_ModelBoxes)
		sub_10044ff4();

	// 0x10044126 -- the SAME field re-read into %esi and tested with a
	// full-width `test $0x40,%esi`, so the source names it twice.
	// 0x40e00000 is 7.0f.
	if (m_n698 & 0x40)
		g_f1008b908 = 7.0f;
}


// *********************************************************************** //
//  COIN-FLIPS, both spellings stated so one build settles each.
// *********************************************************************** //
//
//  1. THE LIST HEAD AT L_MakeRec.  Written above as a local `pHead` loaded
//     once at 0x10043ee8 and reused at 0x10044194 / 0x100441c0, which is
//     what the shipped `mov -0x24(%ebp),%eax` reloads do.  big2ef34.cpp's
//     copy of the same block re-reads the FIELD in all three places and
//     its shipped bytes reload it from the block each time.  The
//     alternative, if the local form misses:
//         if (pBlock->m_pHead0c) { ... }
//         ...
//         if (pBlock->m_pHead0c)
//         {
//             pRec = sub_10039bd0(pBlock, nStage, 1);
//             ...
//             pRec->m_pNext30 = pBlock->m_pHead0c->m_pNext30;
//             pBlock->m_pHead0c->m_pNext30 = pRec;
//         }
//
//  2. THE BUMP ANGLE.  Written above as an explicit `double fAngle`
//     assigned once.  The shipped `fstpl -0x20(%ebp)` plus three `fldl`
//     is equally the temp icl makes when it CSEs a PURE INTRINSIC out of
//     four statements, in which case the source has no variable at all:
//         g_f10089554 = (float)cos(atan2(g_f1008bd28, g_f1008bd30));
//         g_f1008edf8 = (float)-sin(atan2(g_f1008bd28, g_f1008bd30));
//         g_f1008edfc = (float)sin(atan2(g_f1008bd28, g_f1008bd30));
//         g_f10089550 = (float)cos(atan2(g_f1008bd28, g_f1008bd30));
//     Nothing in the bytes separates the two; both are call-free, so the
//     acceptance test is blind to the difference either way.
//
//  3. THE PAIR ORDER AT 0x1004402e.  The two inverse-size stores come out
//     +0x08-source first (`mov %ecx,0x1008e668(,%eax,8)` before
//     `mov %esi,0x1008e664(,%eax,8)`) while the two LOADS are +0x04 then
//     +0x08 (0x10044022, 0x10044025).  Written above in load order,
//     matching big2ef34.cpp and big6d910.cpp.  The alternative:
//         g_a1008e664[pRec->m_nStage42].m_fInvH04 = pRec->m_fInvH08;
//         g_a1008e664[pRec->m_nStage42].m_fInvW00 = pRec->m_fInvW04;
//
//  4. WHERE `bNeedInit = 0` SITS.  The store at 0x10044098 is scheduled
//     into the argument setup of the SetRenderState three instructions
//     later; it is written above AFTER that call.  The alternative is
//     before it, i.e. between `sub_10008da0(0);` and the SetRenderState.
//
//  5. THE FOUR DEAD STORES at -0x138, -0x130, -0x128 and -0x120
//     (0x10044680, 0x100446fd, 0x1004477a, 0x100447f7) are written and
//     NEVER read, one per bump-matrix value, on an EIGHT-byte stride with
//     a FOUR-byte `mov`.  They are consistent with icl keeping a second
//     copy of the inlined constructor's third parameter (the module does
//     no dead-store elimination through memory), and
//     nothing above depends on them; but if a build reproduces every
//     other store and not these four, the source spelled the four values
//     into four named locals first, e.g.
//         uint32 dwM00 = F2DW440(g_f10089554);
//         CStageStateSet440e3 ss04(1, 0x07, dwM00);
//     and the same for 0x08 / 0x09 / 0x0a.
//
//  ---------------------------------------------------------------------- //
//  TWO THINGS THAT ARE NOT COIN-FLIPS AND ARE NOT SPELLABLE
//  ---------------------------------------------------------------------- //
//
//  * THE TWO-ENTRY REALIGNING PROLOGUE at 0x10043e10/0x10043e1e and the
//    `mov %ebx,%esp ; pop %ebx` it pairs with.  big17c14.cpp records the
//    same shape on this function's caller and also declines to spell it.
//
//  * (Superseded: both pairs, 0x10043660 and 0x10043780, are now defined
//    in this file and called at their register entries, identical.)
//    sub_10043664 and sub_10043784 are the REGISTER entries of two-entry
//    functions; past the four-byte shim the argument lives in %eax and
//    nothing is pushed.  d3dr5c.cpp records the same verdict for the pair
//    and for 0x10011450/0x10011454.  Declaring them __cdecl costs one
//    `mov %reg,(%esp)` per site and does not move the call sequence.
//
//  ---------------------------------------------------------------------- //
//  TWO FRAME SLOTS ARE SHARED BY TWO SOURCE VARIABLES EACH, and both
//  pairs have provably disjoint live ranges, so this is icl coalescing
//  and not a misreading:
//
//    * -0x28 holds bNeedInit (set to 1 at 0x10043e77) and nStage (set at
//      0x10043ed6).  Every path that stores nStage passes through
//      0x10044098, which reassigns the slot to 0 before its only reader
//      at 0x100443ef; every path that skips the bind block never stores
//      nStage.
//    * -0x24 holds pHead (0x10043eeb) and pRec (0x10043f5f).  pRec is
//      stored only on the L_Bind path, which is downstream of pHead's
//      last reader at 0x100441c0.
//  ---------------------------------------------------------------------- //
