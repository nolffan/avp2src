// *********************************************************************** //
//
//  MODULE:   OPT2D (.CPP)
//
//  PURPOSE:  d3d.ren's Optimized2D family -- RenderStruct slots +0x94..+0xac.
//            This is the renderer's 2D blitting mode: StartOptimized2D puts
//            the D3D7 device into a state suited to drawing screen-space
//            quads, EndOptimized2D puts it back, and the three setters in
//            between choose the blend and the modulation colour.
//
//            Slot map (see src/engine/d3d/d3d.cpp for how the table was
//            established):
//
//              +0x94  0x10034930  StartOptimized2D()          452 bytes
//              +0x98  0x10034b00  EndOptimized2D()            344 bytes
//              +0x9c  0x10034c60  IsInOptimized2D()             6 bytes
//              +0xa0  0x10034c70  SetOptimized2DBlend()       561 bytes
//              +0xa4  0x10034ed0  GetOptimized2DBlend()        18 bytes
//              +0xa8  0x10034eb0  SetOptimized2DColor()        26 bytes
//              +0xac  0x10034ef0  GetOptimized2DColor()        18 bytes
//
//            (The task sheet's sizes for the last four came from
//            the function-extent table; measured against where the `ret`
//            actually is, SetOptimized2DBlend is 561 not ~250, and the two
//            getters are 18 not 20. The table's sizes include inter-function
//            padding.)
//
//            IsInOptimized2D lives in src/engine/d3d/d3dmisc.cpp and
//            SetOptimized2DColor in src/engine/d3d/d3ddraw.cpp; both are
//            already done and are NOT repeated here. The extern
//            declarations at the bottom of this file cover the pieces those
//            two files already own.
//
//  VERIFIABILITY: every function written here except the two getters carries
//            `push %ebp; mov %esp,%ebp`, and every function here except
//            EndOptimized2D carries `xor %eax,%eax; inc %eax` for `return 1`.
//            Per d3ddraw.cpp's header note that means NONE of them can be
//            verified against the shipped bytes until the compiler question
//            is settled. The decodes below are complete -- every instruction
//            in all five bodies was read -- and are compiler-independent.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  THE COM SURFACE THIS FILE TOUCHES
// ---------------------------------------------------------------------- //
//
//  One object, 0x1008d988, which d3ddraw.cpp already pins as the Direct3D 7
//  device (its Start3D/End3D call +0x14/+0x18 on it, BeginScene/EndScene at
//  indices 5 and 6 of IDirect3DDevice7). This file reaches three more slots
//  on the SAME object, and all three are pinned twice over:
//
//    +0x50  index 20  SetRenderState(DWORD dwState, DWORD dwValue)
//    +0x54  index 21  GetRenderState(DWORD dwState, LPDWORD lpdwValue)
//    +0x94  index 37  SetTextureStageState(DWORD dwStage,
//                                          DWORD dwState, DWORD dwValue)
//
//  PIN 1 -- ARITY AND CALLING CONVENTION. The call sites open a fixed
//  outgoing area (`add $0xfffffff4,%esp` = 12 bytes for +0x50/+0x54,
//  `add $0xfffffff0,%esp` = 16 bytes for +0x94), store `this` at (%esp) and
//  the arguments above it, and NEVER pop afterwards -- the frame is dropped
//  wholesale by `mov %ebp,%esp` at the end of the function. Callee cleanup,
//  i.e. __stdcall with `ret 0xc` and `ret 0x10`, which is COM. So +0x50 and
//  +0x54 take two arguments past `this` and +0x94 takes three -- exactly
//  Set/GetRenderState and SetTextureStageState.
//
//  PIN 2 -- THE CONSTANTS DECODE. Without the DX7 headers (the toolchain
//  ships DirectX 5's d3d.h, which stops at IDirect3DDevice2) the slot
//  indices alone would be a guess. They are not a guess, because the
//  numbers pushed through them read as valid D3D7 enumerators AND the
//  resulting behaviour matches the public SDK contract:
//
//    * SetOptimized2DBlend's seven cases push seven (SRCBLEND, DESTBLEND)
//      pairs through +0x50, and each pair is EXACTLY the D3DBLEND pair that
//      implements the LTSurfaceBlend member with the same ordinal, as
//      spelled out in the comments in src/LT2/sdk/inc/ltbasedefs.h. Seven
//      independent agreements; see the table on that function.
//    * StartOptimized2D pushes 0x1c, 0x07, 0x0e, 0x1b, 0x13, 0x14 through
//      +0x50 -- FOGENABLE, ZENABLE, ZWRITEENABLE, ALPHABLENDENABLE,
//      SRCBLEND, DESTBLEND -- and it disables fog and Z and turns alpha
//      blending on, which is what a 2D overlay pass does.
//    * The +0x94 sites push 0x10/0x11/0x12 with values 1,1,2 on entry and
//      2,2,2 on exit -- MAGFILTER/MINFILTER/MIPFILTER, i.e. POINT while the
//      2D batch is open and LINEAR again afterwards, and that whole block is
//      guarded by the "Bilinear" console variable. A texture-filtering
//      decision gated on a console variable named Bilinear is not a
//      coincidence.
//
//  Only these three slots are used by this family. Every other slot in the
//  struct below is padding and is NOT named -- they are not identified and
//  no claim is made.
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x50];
	// +0x50 -- IDirect3DDevice7::SetRenderState (index 20).
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);
	// +0x54 -- IDirect3DDevice7::GetRenderState (index 21).
	long (__stdcall* GetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32* pdwValue);
	uint8 m_pad_58[0x3c];
	// +0x94 -- IDirect3DDevice7::SetTextureStageState (index 37).
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32 dwStage,
	                                       uint32 dwState, uint32 dwValue);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

// 0x1008d988 -- the device. Same global d3ddraw.cpp's Start3D/End3D use.
extern D3DDevice7* g_pD3DDevice;


// ---------------------------------------------------------------------- //
//  The D3D7 enumerators this file needs, spelled out because the toolchain's
//  d3d.h is the DirectX 5 one and does not declare them. Values are the
//  public DirectX 7 SDK values; the shipped code pushes the raw numbers.
// ---------------------------------------------------------------------- //

// D3DRENDERSTATETYPE
#define D3DRENDERSTATE_ZENABLE                7     // 0x07
#define D3DRENDERSTATE_ZWRITEENABLE          14     // 0x0e
#define D3DRENDERSTATE_SRCBLEND              19     // 0x13
#define D3DRENDERSTATE_DESTBLEND             20     // 0x14
#define D3DRENDERSTATE_ALPHABLENDENABLE      27     // 0x1b
#define D3DRENDERSTATE_FOGENABLE             28     // 0x1c

// D3DBLEND
#define D3DBLEND_ZERO                         1
#define D3DBLEND_ONE                          2
#define D3DBLEND_SRCCOLOR                     3
#define D3DBLEND_INVSRCCOLOR                  4
#define D3DBLEND_SRCALPHA                     5
#define D3DBLEND_INVSRCALPHA                  6
#define D3DBLEND_DESTCOLOR                    9

// D3DTEXTURESTAGESTATETYPE
#define D3DTSS_MAGFILTER                     16     // 0x10
#define D3DTSS_MINFILTER                     17     // 0x11
#define D3DTSS_MIPFILTER                     18     // 0x12

// D3DTEXTUREMAGFILTER / D3DTEXTUREMINFILTER / D3DTEXTUREMIPFILTER
#define D3DTFG_POINT                          1
#define D3DTFG_LINEAR                         2
#define D3DTFN_POINT                          1
#define D3DTFN_LINEAR                         2
#define D3DTFP_POINT                          2


// ---------------------------------------------------------------------- //
//  GLOBALS
// ---------------------------------------------------------------------- //

// 0x1008d7c4 -- non-zero while an Optimized2D batch is open. Declared in
// d3ddraw.cpp; IsInOptimized2D (d3dmisc.cpp, 0x10034c60) returns it, End3D
// tests it at 0x10032ebe to close a batch the caller left open, and the two
// functions below are the only writers (0x10034a4b sets it, 0x10034bb5
// clears it). Ten references across the image in total; the other three are
// 0x1002d21c (RenderScene) and 0x10036d24/0x10036d56/0x10036dc6 (the blit
// entry points), all reads.
extern LTBOOL g_bInOptimized2D;

// 0x1008d7c8 -- non-zero between Start3D and End3D. Declared in d3ddraw.cpp.
extern LTBOOL g_bIn3D;

// 0x1008d9a0 -- where StartOptimized2D parks the device's FOGENABLE state so
// EndOptimized2D can put it back. NOT a console variable and not written
// anywhere else: the whole image contains exactly two references to it,
// 0x10034969 (the GetRenderState out-pointer) and 0x10034b7c (the value
// fed back to SetRenderState). That two-reference pattern is what makes it
// a save slot rather than a setting.
uint32 g_nSavedFogEnable;

// 0x1008d9a4 -- the current Optimized2D blend mode, as an LTSurfaceBlend.
// Also exactly two references image-wide: 0x10034c7a (the store in
// SetOptimized2DBlend) and 0x10034ed6 (the load in GetOptimized2DBlend).
// Nothing else reads it, because the setter programs SRCBLEND/DESTBLEND into
// the device immediately -- the global is bookkeeping for a getter that
// nothing calls.
uint32 g_Optimized2DBlend;

// Two console variables, both registered by the renderer's variable-table
// builder around 0x10022130. The registration helper is 0x10021f44, called
// __fastcall-style with the variable record in %ecx and
// (name, float default, int* value, int* value2) on the stack -- the same
// call that registers NoLMPages into 0x1008ca34, which d3ddraw.cpp already
// names, so the shape is confirmed rather than assumed.
//
//   0x1008cb88 <- "FilterOptimized" (0x100a57b0), default 0.0
//   0x1008ca98 <- "Bilinear"        (0x100a5770), default 1.0
//
// Both are tested as raw dwords against zero.
extern int32 g_CV_FilterOptimized;                      // 0x1008cb88
extern int32 g_CV_Bilinear;                             // 0x1008ca98

// 0x10023c40 -- point texture stage 0 at "modulate the texture by the
// diffuse colour", for both colour and alpha. Two callers: 0x100327e7 and
// StartOptimized2D at 0x100349e6. It branches on the "UseDX6Commands"
// console variable (0x100893b4, registered at 0x10022620, default 1.0):
// when set it issues six SetTextureStageState calls -- COLOROP/ALPHAOP =
// D3DTOP_MODULATE(4), COLORARG1/ALPHAARG1 = D3DTA_TEXTURE(2),
// COLORARG2/ALPHAARG2 = D3DTA_DIFFUSE(0) -- and when clear it falls back to
// the DirectX 5 way, one SetRenderState of TEXTUREMAPBLEND(0x15) to
// D3DTBLEND_MODULATEALPHA(4). That is the same operation expressed in the
// old API and the new one, which is itself a third pin on the +0x50 and
// +0x94 slots.
extern void d3d_SetTextureModulate();


// ---------------------------------------------------------------------- //
//  0x10034930 -- RenderStruct +0x94, `g_pRenderStartOptimized2D`,
//  452 bytes. DECODED: every instruction read.
//
//  WHAT PINS IT: the slot comes from d3d.cpp's table; the body is pinned to
//  the 2D mode by the three globals it gates on (device, in-3D, in-2D) and
//  by the render states it sets. The exact refusal ORDER is read off
//  0x10034936 / 0x10034943 / 0x10034951.
//
//  Note the third test is NOT a refusal: if a batch is already open the
//  function falls straight into the shared `xor %eax,%eax; inc %eax`
//  epilogue at 0x1003495b and answers LTTRUE, having done nothing. Only the
//  first two conditions reach 0x10034a5a, the `xor %eax,%eax` that answers
//  LTFALSE. So this is idempotent-on-success, not fail-if-open -- which is
//  why it is written below as an `if` around the body rather than an early
//  return.
//
//  The state it leaves behind, in shipped order:
//    save FOGENABLE, then fog off, Z test off, Z write off;
//    optionally drop texture filtering to point;
//    modulate texture by diffuse;
//    alpha blending on, SRCALPHA / INVSRCALPHA.
//
//  Two codegen details worth recording, because they are what a candidate
//  compiler has to reproduce. First, the device pointer loaded at 0x10034936
//  for the null test is still live at 0x1003496d and is reused as `this` for
//  the GetRenderState call; every later call reloads 0x1008d988 from memory.
//  Second, the whole thing is one basic block chain with the two arms of the
//  filter test laid out AFTER the return paths (0x10034a61 onwards) and
//  jumping back.
// ---------------------------------------------------------------------- //
LTBOOL d3d_StartOptimized2D()
{
	// Two tests, and already-in-2D answers TRUE up front: nested under
	// `if (!g_bInOptimized2D)`, or with the null tests as separate returns,
	// the exits are hosted differently (337 bytes differ).
	if (!g_pD3DDevice || !g_bIn3D)
		return LTFALSE;

	if (g_bInOptimized2D)
		return LTTRUE;

	// Remember the fog setting so EndOptimized2D can restore it, then
	// take fog and the whole Z pipeline out of the way. 2D quads carry
	// no depth and must not be fogged.
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_FOGENABLE, &g_nSavedFogEnable);

	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_FOGENABLE, 0);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_ZENABLE, 0);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_ZWRITEENABLE, 0);

	// Screen-space blits are 1:1, so bilinear filtering only blurs them.
	// The block runs when bilinear is on globally AND the user has not
	// asked for it in 2D specifically -- so FilterOptimized is an opt-IN
	// that SUPPRESSES this demotion. Read the test carefully: it is
	// `je` on FilterOptimized (0x100349e4) falling to the Bilinear test
	// at 0x10034a61, and `je` on Bilinear jumping past the block, so the
	// condition is (!FilterOptimized && Bilinear).
	if (!g_CV_FilterOptimized && g_CV_Bilinear)
	{
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
			0, D3DTSS_MINFILTER, D3DTFN_POINT);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
			0, D3DTSS_MAGFILTER, D3DTFG_POINT);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
			0, D3DTSS_MIPFILTER, D3DTFP_POINT);
	}

	// Texture * diffuse, colour and alpha both. The diffuse half is
	// where SetOptimized2DColor's value enters the pipeline.
	d3d_SetTextureModulate();

	// Standard source-alpha blend. SetOptimized2DBlend can replace the
	// SRCBLEND/DESTBLEND pair afterwards; ALPHABLENDENABLE stays on for
	// the whole batch and is only cleared by EndOptimized2D.
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_ALPHABLENDENABLE, 1);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);

	g_bInOptimized2D = LTTRUE;

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x10034b00 -- RenderStruct +0x98, `g_pRenderEndOptimized2D`, 344 bytes.
//  DECODED: every instruction read.
//
//  WHAT PINS IT: it is the exact inverse of the function above, on the same
//  three globals, and it is the function d3ddraw.cpp's End3D already calls
//  through this slot at 0x10032ebe.
//
//  RETURN TYPE. This one is `void`, and that is read off the bytes, not
//  assumed: all four exit paths reach `mov %ebp,%esp; pop %ebp; ret` at
//  0x10034b2b without any of them writing %eax, and the last thing %eax
//  holds on the main path is the device pointer. Its sibling
//  StartOptimized2D sets %eax on every path. So the slot is
//  `void (*)()` -- which is what d3ddraw.cpp already declares it as.
//
//  It does NOT undo everything StartOptimized2D did, and the asymmetry is
//  real, not a decode gap:
//    * Z test and Z write are set to 1, not to a saved value -- the entry
//      path never saved them.
//    * FOGENABLE alone is restored from 0x1008d9a0.
//    * The filters go to LINEAR, again not to a saved value.
//    * MIPFILTER is written 2 on the way IN and 2 on the way OUT, so that
//      one call is a no-op restore. Kept because it is in the bytes
//      (0x10034c2b..0x10034c53).
//    * The modulate stage setup and the SRCBLEND/DESTBLEND pair are left
//      exactly as they were; only ALPHABLENDENABLE is cleared.
// ---------------------------------------------------------------------- //
void d3d_EndOptimized2D()
{
	if (!g_pD3DDevice || !g_bIn3D)
		return;

	if (!g_bInOptimized2D)
		return;

	// Z back on, and fog back to whatever it was before the batch.
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_ZENABLE, 1);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_ZWRITEENABLE, 1);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_FOGENABLE, g_nSavedFogEnable);

	// Same guard as the entry path, so filtering is only put back if it was
	// taken away.
	if (!g_CV_FilterOptimized && g_CV_Bilinear)
	{
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
			0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
			0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
			0, D3DTSS_MIPFILTER, D3DTFP_POINT);
	}

	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_ALPHABLENDENABLE, 0);

	g_bInOptimized2D = LTFALSE;
}


// ---------------------------------------------------------------------- //
//  0x10034c70 -- RenderStruct +0xa0, `g_pRenderSetOptimized2DBlend`,
//  561 bytes. DECODED: every instruction read.
//
//  WHAT PINS IT, AND IT PINS ITSELF SEVEN TIMES OVER. The argument is
//  dispatched by a linear compare chain against 0, 1, 2, 3, 4, 5, 6 -- and
//  nothing else -- and each arm issues one SRCBLEND and one DESTBLEND. Line
//  the arms up against LTSurfaceBlend and its comments in
//  src/LT2/sdk/inc/ltbasedefs.h:
//
//    n  LTSurfaceBlend member  SRC / DEST (shipped)        formula
//    -  ---------------------  --------------------------  -------------------
//    0  LTSURFACEBLEND_ALPHA   SRCALPHA(5) INVSRCALPHA(6)  lerp(src,dest,a)
//    1  LTSURFACEBLEND_SOLID   ONE(2)      ZERO(1)         src
//    2  LTSURFACEBLEND_ADD     ONE(2)      ONE(2)          src + dest
//    3  LTSURFACEBLEND_MULT.   DESTCOLOR(9) ZERO(1)        src * dest
//    4  LTSURFACEBLEND_MULT.2  DESTCOLOR(9) SRCCOLOR(3)    2 * src * dest
//    5  LTSURFACEBLEND_MASK    ZERO(1)     INVSRCCOLOR(4)  (1-src) * dest
//    6  LTSURFACEBLEND_MASKADD ONE(2)      INVSRCCOLOR(4)  src + (1-src)*dest
//
//  Every row is the textbook D3DBLEND pair for the formula the SDK header
//  writes next to that member, including the MULTIPLY2 trick of getting the
//  factor of two out of dest*src + src*dest. Seven independent agreements
//  settle both the argument type and the meaning of slots +0x50/0x13/0x14 at
//  once. Anything outside 0..6 answers LTFALSE without touching the device.
//
//  ORDER OF OPERATIONS, read off 0x10034c76..0x10034c80: the global at
//  0x1008d9a4 is written BEFORE the range is checked, so a rejected blend
//  mode is still recorded and GetOptimized2DBlend will hand it back. That is
//  a real behaviour of the shipped code and the statement below is placed
//  first deliberately.
//
//  The shipped dispatch is a compare chain, not a jump table, with the case
//  bodies laid out in descending order of case number (case 6 lowest in
//  memory at 0x10034cba, case 0 highest at 0x10034e5b) and every one of them
//  jumping back to the shared `xor %eax,%eax; inc %eax` at 0x10034cfb. That
//  is a codegen choice, not a source one -- written as a switch here.
// ---------------------------------------------------------------------- //
LTBOOL d3d_SetOptimized2DBlend(LTSurfaceBlend nBlend)
{
	// Recorded first, and unconditionally -- see the note above.
	g_Optimized2DBlend = (uint32)nBlend;

	switch (nBlend)
	{
		case LTSURFACEBLEND_ALPHA:
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;

		case LTSURFACEBLEND_SOLID:
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
			break;

		case LTSURFACEBLEND_ADD:
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
			break;

		case LTSURFACEBLEND_MULTIPLY:
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
			break;

		case LTSURFACEBLEND_MULTIPLY2:
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;

		case LTSURFACEBLEND_MASK:
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCCOLOR);
			break;

		case LTSURFACEBLEND_MASKADD:
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCCOLOR);
			break;

		default:
			return LTFALSE;
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x10034ed0 / 0x10034ef0 -- GetOptimized2DBlend / GetOptimized2DColor
//  (RenderStruct +0xa4 / +0xac) are in w81_10034ed0_osg5.cpp: their shipped
//  scheduling is /G5's (see that file).
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  ALREADY DONE ELSEWHERE -- declared, not defined, so this file reads as a
//  complete account of the family without duplicating anything.
//
//    0x10034c60  d3d_IsInOptimized2D()          src/engine/d3d/d3dmisc.cpp
//    0x10034eb0  d3d_SetOptimized2DColor(...)   src/engine/d3d/d3ddraw.cpp
// ---------------------------------------------------------------------- //
extern LTBOOL d3d_IsInOptimized2D();
extern LTBOOL d3d_SetOptimized2DColor(uint32 hColor);
