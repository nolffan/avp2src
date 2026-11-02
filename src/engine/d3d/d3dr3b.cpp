// *********************************************************************** //
//
//  MODULE:   D3DBATCHB2 (.CPP)  -- d3d.ren, batch B
//
//  Ten functions from the retail d3d.ren.  Every offset, constant and branch below
//  was read off the shipped bytes; nothing
//  here is a guess about what the code "probably" does.
//
//  TWO NEW FACTS THIS BATCH ESTABLISHES, both worth more than a function:
//
//  1. IDirect3DDevice7 +0x2c is SetTransform (index 11) and +0x98 is
//     ValidateDevice (index 38).  Both fall straight out of the vtable order
//     the existing pins already fix: 5=+0x14 BeginScene, 6=+0x18 EndScene,
//     20=+0x50 SetRenderState, 21=+0x54 GetRenderState, 35=+0x8c SetTexture,
//     37=+0x94 SetTextureStageState.  Counting from those, 11 lands on 0x2c
//     and 38 on 0x98.  Independently confirmed by USE: 0x10026670 calls
//     +0x2c three times with the constants 1, 2, 3 and, each time, the
//     address of a distinct 0x40-byte local -- D3DTRANSFORMSTATE_WORLD,
//     _VIEW and _PROJECTION with a D3DMATRIX each; and 0x10032610 calls
//     +0x98 twice with the address of one DWORD local and tests the answer
//     against zero, which is ValidateDevice(LPDWORD lpdwNumPasses) exactly.
//
//  2. 0x100149a0 is ltmatrix.h's `MatVMul_InPlace_H` and 0x10014a50 is its
//     `MatVMul_3x3` -- both verbatim, both CALLED rather than inlined.  That
//     in turn identifies 0x10027eb0 as `LTMatrix::TransformPlane`, whose
//     four statements it reproduces in order.  See that function's comment.
//
//  SKIPPED, and why (all four were decoded before being dropped):
//
//    * 0x10026670 and 0x10025820.  Both CALL PAST A SHIM -- 0x10026670 calls
//      0x100265a8 when the stack-ABI entry is 0x100265a0, and 0x10025820
//      calls 0x1002803c when the entry is 0x10028030.  d3dmid.cpp already
//      records this for 0x10025680: written as C the call goes to the shim,
//      so the displacement is eight bytes out and the function cannot match.
//      They are complete otherwise and are described below for whoever gets
//      the shimmed callees into the same translation unit.
//    * 0x1006d79d and 0x10011879.  Not function starts.  0x1006d79d begins
//      `flds -0x10(%ebp)` in the middle of a live frame and 0x10011879 is
//      inside the function whose head is above it; both are enumeration
//      artefacts.
//    * 0x1005a550, 0x10051204, and the 800-byte clipper family
//      (0x10001000 / 0x10001330 / 0x10003410).  See the closing note.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  THE DIRECT3D 7 DEVICE AT 0x1008d988
//
//  Modelled as the function-pointer struct the rest of this reconstruction
//  uses, because `mov (%eax),%eax ; call *0x50(%eax)` is that model and NOT
//  icl's C++ virtual call (which is two bytes longer).
//  Only the four slots this file needs are named.
// ======================================================================= //

struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x2c];
	// +0x2c -- IDirect3DDevice7::SetTransform (index 11).
	long (__stdcall* SetTransform)(D3DDevice7*, uint32 dwState,
	                               void* lpMatrix);
	uint8 m_pad_30[0x50 - 0x30];
	// +0x50 -- IDirect3DDevice7::SetRenderState (index 20).
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);
	uint8 m_pad_54[0x94 - 0x54];
	// +0x94 -- IDirect3DDevice7::SetTextureStageState (index 37).
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32 dwStage,
	                                       uint32 dwState, uint32 dwValue);
	// +0x98 -- IDirect3DDevice7::ValidateDevice (index 38).
	long (__stdcall* ValidateDevice)(D3DDevice7*, uint32* lpdwNumPasses);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988


// D3DRENDERSTATETYPE, spelled out: the toolchain ships DirectX 5's d3d.h and
// the 0x88..0x8a block is DX7-only.
#define D3DRENDERSTATE_ANTIALIAS             2          // 0x02
#define D3DRENDERSTATE_TEXTUREPERSPECTIVE    4          // 0x04
#define D3DRENDERSTATE_ZENABLE               7          // 0x07
#define D3DRENDERSTATE_FILLMODE              8          // 0x08
#define D3DRENDERSTATE_SHADEMODE             9          // 0x09
#define D3DRENDERSTATE_ZWRITEENABLE          14         // 0x0e
#define D3DRENDERSTATE_CULLMODE              22         // 0x16
#define D3DRENDERSTATE_ZFUNC                 23         // 0x17
#define D3DRENDERSTATE_DITHERENABLE          26         // 0x1a
#define D3DRENDERSTATE_FOGENABLE             28         // 0x1c
#define D3DRENDERSTATE_SPECULARENABLE        29         // 0x1d
#define D3DRENDERSTATE_FOGTABLEMODE          35         // 0x23
#define D3DRENDERSTATE_COLORKEYENABLE        41         // 0x29
#define D3DRENDERSTATE_CLIPPING              136        // 0x88
#define D3DRENDERSTATE_LIGHTING              137        // 0x89
#define D3DRENDERSTATE_EXTENTS               138        // 0x8a

#define D3DFILL_SOLID                        3
#define D3DSHADE_GOURAUD                     2
#define D3DCULL_NONE                         1
#define D3DCMP_LESSEQUAL                     4
#define D3DFOG_LINEAR                        3

// D3DTEXTURESTAGESTATETYPE
#define D3DTSS_COLOROP                       1
#define D3DTSS_COLORARG1                     2
#define D3DTSS_COLORARG2                     3
#define D3DTSS_TEXCOORDINDEX                 11         // 0x0b

#define D3DTOP_MODULATE                      4
#define D3DTA_CURRENT                        1
#define D3DTA_TEXTURE                        2

#define D3D_OK                               0


// The renderer's two-argument warning printer, as d3dblit.cpp names it.
extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage (was d3d_Warning*; icl treats a callee named *Warn*/*Error* as cold)

// 0x10020f30 -- no arguments, already reconstructed in d3dr2d.cpp.
extern void sub_10020f30();


// ======================================================================= //
//  0x1001eb20   100 bytes (0x1001eb20..0x1001eb83), 12 bytes of nop pad.
//               2 callers, both inside 0x100298xx.
//
//  WHAT IT IS: tear down a list of nodes.  Each node owns a second list
//  which is spliced whole onto one free list, then the node itself is
//  pushed onto another free list.  A counter is reset and then incremented
//  once per node, so somebody upstream reads it as "how many were there".
//
//  WHAT PINS IT:
//    * bare `push %ebx` / `pop %ebx` / `ret` with nothing read off the
//      stack: no arguments, __cdecl, void.
//    * `incl 0x1008bf10` -- a read-modify-write straight in memory, i.e.
//      `++g_n1008bf10` and not a register accumulator.
//    * the outer walk is `mov 0x4(%ebx),%edx` (node +0x04 is the link) and
//      the recycle is `mov %eax,(%ebx)` over the SAME field the loop just
//      dereferenced as an object pointer.  +0x00 is both the owner pointer
//      and the free-list link -- a node pool reusing the first word.
//    * `mov (%ebx),%eax` appears TWICE, at 0x1001eb3b and again at
//      0x1001eb59 after the inner loop.  icl 4.0 does not fold loads through
//      memory, so the source mentions the member twice; written longhand.
//    * both loops are the rotated `while` shape -- test, `je` out, body,
//      `jne` back to the top -- not do/while.
// ======================================================================= //

struct Item1001eb20
{
	Item1001eb20*  m_pNext;                             // +0x00
};

struct Owner1001eb20
{
	uint8          m_Unknown00[0x2c];                   // +0x00..+0x2b
	Item1001eb20*  m_pItems;                            // +0x2c
};

struct Node1001eb20
{
	Owner1001eb20* m_pOwner;                            // +0x00, reused as
	                                                    //        the free link
	Node1001eb20*  m_pNext;                             // +0x04
};

extern Node1001eb20* g_pList1008bf14;                   // 0x1008bf14
extern uint32        g_nCount1008bf10;                  // 0x1008bf10
extern Item1001eb20* g_pItemFree1008bf0c;               // 0x1008bf0c
extern Node1001eb20* g_pNodeFree1008bef0;               // 0x1008bef0

void sub_1001eb20()
{
	Node1001eb20* pCur;
	Node1001eb20* pNext;
	Item1001eb20* pItem;
	Item1001eb20* pItemNext;

	g_nCount1008bf10 = 0;

	pCur = g_pList1008bf14;
	while (pCur)
	{
		++g_nCount1008bf10;

		pItem = pCur->m_pOwner->m_pItems;
		while (pItem)
		{
			pItemNext = pItem->m_pNext;
			pItem->m_pNext = g_pItemFree1008bf0c;
			g_pItemFree1008bf0c = pItem;
			pItem = pItemNext;
		}

		pCur->m_pOwner->m_pItems = LTNULL;

		pNext = pCur->m_pNext;
		pCur->m_pOwner = (Owner1001eb20*)g_pNodeFree1008bef0;
		g_pNodeFree1008bef0 = pCur;
		pCur = pNext;
	}

	g_pList1008bf14 = LTNULL;
}


// ======================================================================= //
//  0x1006a710   107 bytes (0x1006a710..0x1006a77a), 5 bytes of pad.
//               1 caller, 0x1004bdac.
//
//  WHAT IT IS: a one-entry memo cache.  If the cached object is missing or
//  either key has changed, throw the old one away and build a new one from
//  a factory object, remembering the keys.  Then answer the cached object.
//
//  WHAT PINS IT:
//    * `mov 0x8(%ebp),%ebx` / `mov 0xc(%ebp),%esi` and a bare `ret`: two
//      arguments, __cdecl.  The caller pushes %edi twice, so both arguments
//      are the same value there and carry no type information; they are
//      dwords.
//    * the guard is `test/je`, `cmp/jne`, `cmp/je` in that order -- one
//      three-term `||`, with the LAST test jumping FORWARD to the common
//      answer and the first two jumping into the work.
//    * both calls are `mov (%ecx),%reg ; mov N(%reg),%eax ; call *%eax`,
//      which is icl's REAL C++ virtual call (the
//      two-byte-longer form), so this is a class with virtual members and
//      not a function-pointer struct.  Slot +0x08 takes one argument and
//      slot +0x04 takes two; neither leaves the stack unbalanced, so both
//      are __thiscall.
//    * 0x100911b8 is loaded SEPARATELY for each of the two calls -- two
//      mentions of the global in the source, not one.
//    * the answer is RELOADED from 0x100911ac at 0x1006a770 rather than
//      kept in %eax, so the last statement really is `return g_pCached;`.
//    * the neighbour at 0x1006a780 is `mov 0x100911b8,%eax ; ret`, the
//      accessor for the same factory global.
// ======================================================================= //

class C1006a710Factory
{
public:
	virtual void  Slot00() = 0;                         // +0x00, unused here
	virtual void* Slot04(uint32 nA, uint32 nB) = 0;     // +0x04
	virtual void  Slot08(void* pOld) = 0;               // +0x08
};

extern void*             g_pCached100911ac;            // 0x100911ac
extern uint32            g_nKeyA100911b0;               // 0x100911b0
extern uint32            g_nKeyB100911b4;               // 0x100911b4
extern C1006a710Factory* g_pFactory100911b8;            // 0x100911b8

void* sub_1006a710(uint32 nA, uint32 nB)
{
	if (!g_pCached100911ac || nA != g_nKeyA100911b0 ||
	    nB != g_nKeyB100911b4)
	{
		g_pFactory100911b8->Slot08(g_pCached100911ac);
		g_pCached100911ac = g_pFactory100911b8->Slot04(nA, nB);
		g_nKeyA100911b0 = nA;
		g_nKeyB100911b4 = nB;
	}

	return g_pCached100911ac;
}


// ======================================================================= //
//  0x10038560   127 bytes (0x10038560..0x100385de), one nop.
//               2 callers, 0x100387bc and 0x100389b7.
//
//  WHAT IT IS: clamp a width/height pair, then write it back out through
//  two pointers.  One capability bit forces the pair square at the LARGER
//  of the two; otherwise a console-set maximum aspect ratio raises the
//  SMALLER of the two until the ratio fits.
//
//  WHAT PINS IT:
//    * both callers pass (w, h, &w_out, &h_out) -- 0x100387bc passes the
//      values and the addresses of the very same two locals, which is what
//      makes the by-value-in / by-pointer-out shape certain.
//    * `test $0x20,%al` on 0x1008d938 is the same capability bit d3dblit.cpp
//      already reads at that address, so the name is reused.
//    * `cmp %eax,%edx ; jbe +2 ; mov %edx,%eax` is MAX, not min: %eax starts
//      as nHeight and is replaced by nWidth only when nWidth is bigger.
//      Both output slots are then written from it unconditionally, which is
//      a chained assignment `a = b = max`, and the stores are 0x14 before
//      0x10 -- right to left, i.e. nHeight before nWidth.
//    * the aspect arm selects two POINTERS with two separate ternaries off
//      one compare (cmp, jbe, lea, jmp, lea, then a second jbe with no
//      second cmp -- `lea` leaves the flags alone).  A single if/else
//      assigning both would be one branch and two leas per arm.
//    * the ratio is computed with `xor %edx,%edx ; div` (UNSIGNED, so the
//      dimensions are uint32) and then compared with `jle` (SIGNED), so it
//      lands in an int32 variable first.  Both facts are needed.
// ======================================================================= //

extern uint32 g_nCaps_1008d938;                         // 0x1008d938
extern int32  g_nMaxAspect1008cbc4;                     // 0x1008cbc4

void sub_10038560(uint32 nWidth, uint32 nHeight, uint32* pWidthOut,
                  uint32* pHeightOut)
{
	uint32* pSmall;
	uint32* pLarge;
	int32   nRatio;

	if (g_nCaps_1008d938 & 0x20)
	{
		nWidth = nHeight = (nWidth > nHeight) ? nWidth : nHeight;
	}
	else if (g_nMaxAspect1008cbc4 > 0)
	{
		pSmall = (nWidth > nHeight) ? &nHeight : &nWidth;
		pLarge = (nWidth > nHeight) ? &nWidth : &nHeight;

		nRatio = *pLarge / *pSmall;
		if (nRatio > g_nMaxAspect1008cbc4)
		{
			*pSmall = *pLarge / g_nMaxAspect1008cbc4;
		}
	}

	*pWidthOut  = nWidth;
	*pHeightOut = nHeight;
}


// ======================================================================= //
//  0x10069e40   141 bytes (0x10069e40..0x10069ecc), lea-nop padded.
//               2 callers, 0x100212e4 and 0x1002169b -- both inside the
//               exported mode enumeration.
//
//  WHAT IT IS: enumerate DirectDraw devices.  Prefer DirectDrawEnumerateExA,
//  resolved by hand out of the already-loaded ddraw.dll so the DX7-only
//  entry point is optional; fall back to DirectDrawEnumerateA on any of the
//  three ways that can fail, printing which one it was.
//
//  WHAT PINS IT:
//    * the four strings are in the image at 0x100ab0b0, 0x100ab0c0,
//      0x100ab0e0, 0x100ab120 and 0x100ab170 and they name every branch.
//    * `call *0x1008801c` / `call *0x10088020` are indirect through the IAT,
//      which is what __declspec(dllimport) gives -- so those two come from
//      windows.h.  `call 0x1007fa8c` is a jump thunk to DDRAW.dll!
//      DirectDrawEnumerateA, i.e. an ordinary (non-dllimport) declaration,
//      which is what ddraw.h gives.  The two spellings are not a choice.
//    * the Ex call pushes 7, then 0, then 0x10069e10: __stdcall
//      (lpCallback, lpContext, dwFlags) with dwFlags = DDENUM_ATTACHED |
//      _DETACHED | _NONDISPLAY.  No `add` follows, confirming callee
//      cleanup.
//    * the success answer is `xor %eax,%eax ; inc %eax` (icl's LTTRUE) and
//      the fallback answer is `sete` on the HRESULT against zero.
//    * the three warnings all jump to the SAME fallback tail, and the
//      success return sits in the middle -- the shape of one positive early
//      return inside nested `if`s with `else` arms, not a guard chain.
// ======================================================================= //

extern "C"
{
	__declspec(dllimport) void* __stdcall GetModuleHandleA(const char* lpModuleName);
	__declspec(dllimport) void* __stdcall GetProcAddress(void* hModule, const char* lpProcName);

	long __stdcall DirectDrawEnumerateA(void* lpCallback, void* lpContext);
}

// The two enumeration callbacks, by address (w8_10069ca0_osoiqip.cpp):
// LPDDENUMCALLBACKEXA, `ret $0x14`, and LPDDENUMCALLBACKA, `ret $0x10`.
extern "C" int32 __stdcall sub_10069e10(void* a, void* b, void* c, void* d, void* e);
extern "C" int32 __stdcall sub_10069ca0(void* a, void* b, void* c, void* d);

typedef long (__stdcall* PFnDDEnumerateExA)(void* lpCallback, void* lpContext,
                                            uint32 dwFlags);

// DDENUM_ATTACHEDSECONDARYDEVICES | _DETACHEDSECONDARYDEVICES |
// _NONDISPLAYDEVICES
#define DDENUM_ALL_10069e40                  7

LTBOOL sub_10069e40()
{
	void*             hDDraw;
	PFnDDEnumerateExA pfnEnumEx;

	sub_10020f30();

	hDDraw = GetModuleHandleA("DDRAW.DLL");
	if (hDDraw)
	{
		pfnEnumEx = (PFnDDEnumerateExA)GetProcAddress(hDDraw,
		                                              "DirectDrawEnumerateExA");
		if (pfnEnumEx)
		{
			if (pfnEnumEx((void*)sub_10069e10, LTNULL,
			              DDENUM_ALL_10069e40) == D3D_OK)
			{
				return LTTRUE;
			}

			sub_10021d60(1, "DirectDrawEnumerateExA failed, using DirectDrawEnumerate");
		}
		else
		{
			sub_10021d60(1, "Unable to get DirectDrawEnumerateExA proc address, using DirectDrawEnumerate");
		}
	}
	else
	{
		sub_10021d60(1, "Unable to get ddraw.dll module handle");
	}

	sub_10020f30();
	return (DirectDrawEnumerateA((void*)sub_10069ca0, LTNULL) == D3D_OK);
}


// ======================================================================= //
//  0x10027eb0   147 bytes (0x10027eb0..0x10027f42), lea-nop padded.
//               1 caller.
//
//  WHAT IT IS: `LTMatrix::TransformPlane`, src/LT2/sdk/inc/ltmatrix.h:704 --
//  VERBATIM, all four statements in the header's order.  This is the header
//  seam, and it comes with two free callee
//  identifications:
//
//      0x10014a50  MatVMul_3x3(pDest, pMat, pSrc)        ltmatrix.h:267
//      0x100149a0  MatVMul_InPlace_H(pMat, pSrc)         ltmatrix.h:233
//
//  0x10014a50 is three rows of `m[i][0]*x + m[i][1]*y + m[i][2]*z` written
//  to +0/+4/+8 of its first argument and nothing else -- MatVMul_3x3 and no
//  other member of that family.  0x100149a0 opens with the homogeneous
//  divide `1.0f / (m[3][0]*x + m[3][1]*y + m[3][2]*z + m[3][3])` (the 1.0f
//  is at 0x100a48f4), writes the three scaled rows and leaves the reciprocal
//  in %st(0): MatVMul_H's arithmetic with only two arguments, which is
//  MatVMul_InPlace_H.
//
//  WHAT PINS THIS FUNCTION:
//    * `ret $0x8` with %ecx never loaded off the stack: __thiscall, ONE
//      four-byte argument, plus a hidden return-slot pointer.  A by-value
//      LTPlane would be `ret $0x14`, so the plane arrives BY REFERENCE --
//      which is exactly the header's `TransformPlane(LTPlane &thePlane)`.
//    * the first call is `operator*(T)` (0x10015690, already reconstructed
//      in d3dbatchd.cpp) with %ecx = &thePlane and the float argument read
//      from thePlane+0x0c, i.e. `thePlane.m_Normal * thePlane.m_Dist`.  Its
//      16-byte return slot at -0x1c is then copied member by member into the
//      named local at -0x10: `startPt = ...`, temporary not elided.
//    * MatVMul_3x3's destination is 0x8(%ebp), the RETURN SLOT itself, so
//      icl applied NRVO to `ret` -- and `mov %ebx,%eax` at the end returns
//      that same pointer.
//    * MatVMul_InPlace_H's float answer is dropped with `fstp %st(0)`: the
//      header ignores it too.
//    * the final Dot is INLINED (`ret.m_Normal.z * startPt.z` first, then y,
//      then x, then one store to +0x0c), which is what an in-class member
//      does under /Ob1.
//    * &thePlane is spilled to -0x4(%ebp) and reloaded for the MatVMul_3x3
//      call rather than kept in %ebx -- no dead-store elimination through
//      memory, as measured elsewhere in this module.
//
//  The classes are declared locally, with the two helpers OUT OF LINE, for
//  this reason: /Ob1 inlines what the source marks
//  inline, and the shipped code calls these two.
// ======================================================================= //

class Vec10027eb0
{
public:
	                Vec10027eb0() {}
	                Vec10027eb0(float mx, float my, float mz) {x=mx; y=my; z=mz;}

	float           Dot(Vec10027eb0 v) const { return (x * v.x) + (y * v.y) + (z * v.z); }

	// 0x10015690 -- _CVector<T>::operator*(T), reconstructed in d3dbatchd.cpp.
	Vec10027eb0     sub_10015690(float v) const;

	float           x;
	float           y;
	float           z;
};

class Plane10027eb0
{
public:
	Vec10027eb0     m_Normal;                           // +0x00
	float           m_Dist;                             // +0x0c
};

class Mat10027eb0
{
public:
	Plane10027eb0   sub_10027eb0(Plane10027eb0 &thePlane);

	float           m[4][4];
};

extern void  sub_10014a50(Vec10027eb0* pDest, Mat10027eb0* pMat,
                          Vec10027eb0* pSrc);           // MatVMul_3x3
extern float sub_100149a0(Mat10027eb0* pMat, Vec10027eb0* pSrc);
                                                        // MatVMul_InPlace_H



// ======================================================================= //
//  0x10020c00   223 bytes (0x10020c00..0x10020cde), one nop.
//               1 caller, 0x1001ee1e.
//
//  WHAT IT IS: build a transform from a rotation and a position and hand it,
//  with nine unchanged arguments, to 0x10020ce0.  It always answers LTTRUE.
//
//  WHAT PINS IT:
//    * arguments are read at 0x88(%esp) through 0xb0(%esp) after
//      `sub $0x84,%esp` -- that is +0x04..+0x2c from entry, ELEVEN __cdecl
//      arguments, and the caller at 0x1001eda0..0x1001ee23 fills exactly
//      those eleven slots and cleans them itself.
//    * the last two are not forwarded.  0x1001ee16 passes `lea 0xcc(%ebx)`
//      as the eleventh and 0x1001ee12 `lea 0xc0(%ebx)` as the tenth, and
//      this function feeds the eleventh to 0x10073400 (`sub_10073400(pRot,
//      pMat)`, already declared in d3dr2a.cpp) and reads +0x00/+0x04/+0x08
//      of the tenth.  So they are a rotation and a position.
//    * the destination of 0x10073400 is `lea 0x28(%esp)` and the three
//      position words then land at 0x34, 0x44 and 0x54(%esp) -- offsets
//      0x0c, 0x1c and 0x2c INSIDE that same local.  That is m[0][3],
//      m[1][3], m[2][3] of a 4x4: ltmatrix.h's SetTranslation, inlined.
//      It is the (x, y, z) overload called with pPos->x, pPos->y, pPos->z:
//      the shipped code loads all three words before the first store, and
//      three direct member stores interleave load/store instead (measured;
//      that was the whole 43-byte residual).
//    * the same `lea 0x28(%esp)` is stored to 0x24(%esp) as the TENTH and
//      last outgoing argument of 0x10020ce0.
//    * `xor %eax,%eax ; inc %eax` before the epilogue: LTTRUE, and the
//      caller does test it.
//    * the fourth argument is the caller's literal 0x40e00000 (7.0f) and
//      the sixth through ninth are `fildl`/`fstps` results, so those five
//      are floats; the rest are copied with integer moves in both functions
//      and carry no type.
// ======================================================================= //

struct Vec10020c00
{
	float          x;                                   // +0x00
	float          y;                                   // +0x04
	float          z;                                   // +0x08
};

struct Rot10020c00;

struct Mat10020c00
{
	// ltmatrix.h's LTMatrix::SetTranslation(float, float, float), verbatim.
	// Its three parameters are what make icl load all of x, y, z before the
	// first store (they are locals of the inlined body); three direct
	// member stores instead interleave each load with its store.
	void SetTranslation(float x, float y, float z)
	{
		m[0][3] = x;
		m[1][3] = y;
		m[2][3] = z;
	}

	float          m[4][4];
};

// 0x10073400 -- already declared in d3dr2a.cpp as (pRot, pMat).
extern void sub_10073400(Rot10020c00* pRot, Mat10020c00* pMat);

extern void sub_10020ce0(void* pArg0, uint32 nArg1, uint32 nArg2, float fArg3,
                         uint32 nArg4, float fArg5, float fArg6, float fArg7,
                         float fArg8, Mat10020c00* pMat);

LTBOOL sub_10020c00(void* pArg0, uint32 nArg1, uint32 nArg2, float fArg3,
                    uint32 nArg4, float fArg5, float fArg6, float fArg7,
                    float fArg8, Vec10020c00* pPos, Rot10020c00* pRot)
{
	Mat10020c00 mMat;

	sub_10073400(pRot, &mMat);

	mMat.SetTranslation(pPos->x, pPos->y, pPos->z);

	sub_10020ce0(pArg0, nArg1, nArg2, fArg3, nArg4, fArg5, fArg6, fArg7,
	             fArg8, &mMat);

	return LTTRUE;
}


// ======================================================================= //
//  0x1001e660   251 bytes (0x1001e660..0x1001e75a), lea-nop padded.
//               1 caller, 0x10020beb.
//
//  WHAT IT IS: find where a point sits inside a box as a 0..1 fraction per
//  axis, then use that fraction to interpolate between two colours (or two
//  vectors) held in a second object, storing the answer in a global.  When
//  the second object is not in mode 1 the fraction is forced to the middle.
//
//  WHAT PINS IT:
//    * no arguments read, bare `ret`: void(void), __cdecl.
//    * `DE F9` is Intel FDIVP st(1),st -- st(1)/st(0) -- so the quotient is
//      (point - lo) / (hi - lo) and not its reciprocal.  objdump prints it
//      "fdivrp", the same inversion as for `DE E9`.
//    * `fsubrs 0x1008bd34` after `flds 0x144(%eax)` is memory-minus-stack:
//      g_v1008bd34.x - lo.x.
//    * 0x1008beac is loaded THREE times, into three registers, and each
//      load then serves all three axes.  That is ltvector.h's by-value
//      operators, inlined: `(pt - box->min) / (box->max - box->min)` binds
//      box->min, box->max and box->min once each.  Measured: the operator
//      form reproduces the first half instruction for instruction, where
//      the longhand per-axis form folds the three loads into one.
//    * 0x1008beb0 is loaded TWICE: once for the mode test and once, into
//      %ebx, for all six ramp reads -- one pointer, so the lerp is written
//      through a local.  The shipped code hoists that second load above
//      the branch and moves the 0.5 arm past the `ret`; as an if/else (==
//      or !=, either order, an inline member lerp) icl keeps the load after
//      the join and the 0.5 arm inline, and a local set before the test is
//      folded into the test's load.  CLOSED, byte-identical, by two levers:
//        - the test is `switch (mode) { case 1: ...; default: 0.5 }`, and
//          icl sinks the default arm past the epilogue;
//        - the test's pointer is read through `*(T* volatile*)&g` and the
//          ramp pointer plainly AFTER it: the volatile read cannot absorb
//          the second load, so both are hoisted, in the shipped order.
//          (Volatile on the ramp read instead swaps the two loads, 3 bytes;
//          on both turns `cmpl $1,(%eax)` into a load and a compare.)
//      The chained 0.5 store order z, y, x comes from `vT.z = vT.y = vT.x`.
//    * both results are built in a 12-byte temporary and then `rep movsl`ed
//      three dwords -- the unelided `LTVector(a,b,c)` temporary this module
//      produces everywhere.
//    * the `else` arm instead stores 0x3f000000 straight into the local at
//      0x8, 0x4, 0x0(%esp) -- z, y then x -- and NOT a constructed temporary.
//    * the lerp is `(hi - lo) * t + lo` with the subtraction first, three
//      times, so it is written that way and not as `lo*(1-t) + hi*t`.
// ======================================================================= //

class Vec1001e660
{
public:
	               Vec1001e660() {}
	               Vec1001e660(float mx, float my, float mz) {x=mx; y=my; z=mz;}
	Vec1001e660 operator + (const Vec1001e660 v) const { return Vec1001e660(x + v.x, y + v.y, z + v.z); }
	Vec1001e660 operator - (const Vec1001e660 v) const { return Vec1001e660(x - v.x, y - v.y, z - v.z); }
	Vec1001e660 operator * (const Vec1001e660 v) const { return Vec1001e660(x * v.x, y * v.y, z * v.z); }
	Vec1001e660 operator / (const Vec1001e660 v) const { return Vec1001e660(x / v.x, y / v.y, z / v.z); }

	float          x;                                   // +0x00
	float          y;                                   // +0x04
	float          z;                                   // +0x08
};

struct Box1001e660                                      // *0x1008beac
{
	uint8          m_Unknown00[0x144];                  // +0x000..+0x143
	Vec1001e660    m_vMin;                              // +0x144
	Vec1001e660    m_vMax;                              // +0x150
};

struct Ramp1001e660                                     // *0x1008beb0
{
	int32          m_nMode;                             // +0x00
	uint8          m_Unknown04[0x84 - 0x04];            // +0x04..+0x83
	Vec1001e660    m_vLo;                               // +0x84
	Vec1001e660    m_vHi;                               // +0x90
};

extern Box1001e660*  g_pBox1008beac;                    // 0x1008beac
extern Ramp1001e660* g_pRamp1008beb0;                   // 0x1008beb0
extern Vec1001e660   g_vPoint1008bd34;                  // 0x1008bd34
extern Vec1001e660   g_vResult1008bd40;                 // 0x1008bd40

void sub_1001e660()
{
	Ramp1001e660 *pTest, *pRamp;
	Vec1001e660 vT;

	pTest = *(Ramp1001e660* volatile*)&g_pRamp1008beb0;
	pRamp = g_pRamp1008beb0;

	switch (pTest->m_nMode)
	{
		case 1:
			vT = (g_vPoint1008bd34 - g_pBox1008beac->m_vMin) /
				(g_pBox1008beac->m_vMax - g_pBox1008beac->m_vMin);
			break;
		default:
			vT.z = vT.y = vT.x = 0.5f;
	}

	g_vResult1008bd40 = (pRamp->m_vHi - pRamp->m_vLo) * vT + pRamp->m_vLo;
}


// ======================================================================= //
//  0x1006c510   352 bytes (0x1006c510..0x1006c66f).
//               1 caller, 0x1002d2e5, which passes the float global
//               0x1008cb94 and afterwards copies it to 0x1008cb90 and sets
//               a "done" flag -- a one-shot table build.
//
//  WHAT IT IS: fill a 64-entry table with `i/63 * scale`, clamped at 1.0 and
//  scaled to 16.16 fixed point.  A gamma or fade ramp.
//
//  WHAT PINS IT:
//    * the four constants are duplicated pairs -- 0x100ab4d4 and 0x100ab4e4
//      are both 0.015873016 (1/63), 0x100ab4d8/0x100ab4e8 and
//      0x100ab4dc/0x100ab4ec are all 1.0, 0x100ab4e0/0x100ab4f0 both
//      65536.0.  Two identical halves with private constant-pool copies is
//      an UNROLLED loop, not two different bodies.
//    * `movzwl %ax,%eax` on the counter every time it is used, with a 32-bit
//      `inc` behind it and a SIGNED `cmp $0x40 ; jge` on the zero-extended
//      value: the induction variable is a 16-bit unsigned that promotes to
//      int for the comparison.
//    * `fcomp %st(1)` against 1.0 with `jae` past the replacement -- the
//      clamp is `if (v > 1.0f) v = 1.0f`, taking the value from a SECOND
//      1.0 in the pool.
//    * the multiply order is `fild i ; fmuls (1/63) ; fmul %st(2)` where
//      %st(2) is the argument, i.e. `((float)i * (1/63)) * fScale` and not
//      `i * (fScale/63)`.
//    * `fstpl` then the exponent/shld/shr sequence over the two halves of
//      the double is icl's inline double-to-int32 conversion, so the source
//      is a plain `(int32)` cast; the argument itself is the only thing left
//      on the x87 stack, which is why both epilogues open `fstp %st(0)`.
// ======================================================================= //

extern int32 g_nRamp10091240[64];                       // 0x10091240



// ======================================================================= //
//  0x1000aa90   374 bytes (0x1000aa90..0x1000ac05), nop padded.
//               1 caller, 0x1000a9f1.
//
//  WHAT IT IS: the frustum clip driver.  Six clip planes, each guarded by
//  its own bit, each handed the same three by-reference state variables:
//  the vertex pointer, the vertex count and the scratch buffer the clippers
//  ping-pong into.  Any plane that answers zero aborts the whole thing.
//
//  WHAT PINS IT:
//    * the caller passes `g_1008b874`, %esi and %edi, and immediately after
//      the call reads `(%edi)` as a count and `(%esi)` as a vertex pointer,
//      which fixes all three parameter types.
//    * the six callees take the same four arguments in the same order.
//      0x10003410 (itself a listed row) reads 0xc(%ebp) as the vertex
//      pointer, 0x10(%ebp) as the count and 0x14(%ebp) as the scratch slot,
//      and never touches 0x8(%ebp) at all -- a dead first parameter, which
//      is normal and which `ret`'s argument count
//      here proves exists.
//    * the bit order is 0x01, 0x04, 0x08, 0x10, 0x20, 0x02: FIVE ascending
//      bits with bit 1 moved to the END, so the source really does test
//      them in that order and it is not a switch or a loop.
//    * `and $0x1,%ebx` DESTROYS the flags word, and every later `test` is
//      against the masked value.  That only happens if the source assigns
//      `nFlags &= 1` rather than testing `nFlags & 1`, and it happens only
//      on the arm where 0x1008d7a0 is set.
//    * the six failures tail-merge onto one `xor %eax,%eax`, and the success
//      path jumps BACKWARDS to the LTTRUE that the early guard already
//      hosts -- the shared-exit shape of one early positive return.
// ======================================================================= //

extern uint32 g_bFlag1008d7a0;                          // 0x1008d7a0
extern void*  g_pScratch1008b87c;                       // 0x1008b87c

extern int32 sub_10002310(uint32* pUnused, void** ppVerts, uint32* pnVerts,
                          void** ppScratch);
extern int32 sub_10002670(uint32* pUnused, void** ppVerts, uint32* pnVerts,
                          void** ppScratch);
extern int32 sub_100029e0(uint32* pUnused, void** ppVerts, uint32* pnVerts,
                          void** ppScratch);
extern int32 sub_10002d40(uint32* pUnused, void** ppVerts, uint32* pnVerts,
                          void** ppScratch);
extern int32 sub_100030a0(uint32* pUnused, void** ppVerts, uint32* pnVerts,
                          void** ppScratch);
extern int32 sub_10003410(uint32* pUnused, void** ppVerts, uint32* pnVerts,
                          void** ppScratch);

LTBOOL sub_1000aa90(uint32 nFlags, void** ppVerts, uint32* pnVerts)
{
	void*  pScratch;
	void*  pVerts;
	uint32 nVerts;
	uint32 nOut0;
	uint32 nOut1;
	uint32 nOut2;
	uint32 nOut3;
	uint32 nOut4;
	uint32 nOut5;

	if (g_bFlag1008d7a0)
	{
		nFlags &= 0x01;
		if (!nFlags)
			return LTTRUE;
	}

	pScratch = g_pScratch1008b87c;
	pVerts   = *ppVerts;
	nVerts   = *pnVerts;

	if (nFlags & 0x01)
	{
		if (!sub_10002310(&nOut0, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x04)
	{
		if (!sub_10002670(&nOut1, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x08)
	{
		if (!sub_100029e0(&nOut2, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x10)
	{
		if (!sub_10002d40(&nOut3, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x20)
	{
		if (!sub_100030a0(&nOut4, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x02)
	{
		if (!sub_10003410(&nOut5, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;
	return LTTRUE;
}


// ======================================================================= //
//  0x10032610   889 bytes (0x10032610..0x10032988), lea-nop padded.
//               1 caller.
//
//  WHAT IT IS: put the device into the renderer's default state -- nineteen
//  SetRenderState / SetTextureStageState calls in a fixed order -- and then
//  ValidateDevice twice, once around a saved/restored stage setup and once
//  after 0x10011560, caching each answer in a global.
//
//  WHAT PINS IT, and this is the function that pins +0x98:
//    * every state number decodes as a real D3DRENDERSTATETYPE with a legal
//      value for it, in a sequence that reads like a device reset:
//      FOGTABLEMODE=LINEAR, FOGENABLE=0, ANTIALIAS=0, CULLMODE=NONE,
//      COLORKEYENABLE=0, ZENABLE=<global>, ZWRITEENABLE=1, ZFUNC=LESSEQUAL,
//      SPECULARENABLE=0, SHADEMODE=GOURAUD, TEXTUREPERSPECTIVE=1,
//      DITHERENABLE=1, FILLMODE=SOLID, CLIPPING=<flag>, LIGHTING=0,
//      EXTENTS=0.  Nineteen independent agreements.
//    * the +0x94 calls are (0, TEXCOORDINDEX, 0), (1, TEXCOORDINDEX, 1),
//      (0, COLOROP, MODULATE), (0, COLORARG1, TEXTURE), (0, COLORARG2,
//      CURRENT) -- again all legal, which is what confirms the four-argument
//      shape.
//    * +0x98 is called with (device, &local) and its answer is compared with
//      zero: two arguments, __stdcall, HRESULT.  Index 38 of
//      IDirect3DDevice7 is ValidateDevice(LPDWORD), and 38*4 = 0x98.
//    * CLIPPING's value is built with `mov $1 / jmp / xor %edx,%edx`, the
//      branchy shape of a `? 1 : 0` ternary, NOT the `setne` a bare `!= 0`
//      would give.
//    * each ValidateDevice answer is `xor ; test ; sete`, which icl gives
//      for `!hr` and NOT for `hr == D3D_OK` (that one comes out as two
//      branchy stores to the global).  Measured.
//    * /O2, not the /Os of the rest of d3dr3b.cpp: `mov $1,%eax` for the
//      second TEXCOORDINDEX where /Os emits `xor ; inc`.  Hence
//      d3dr3b_o2.cpp.
//    * 0x1008d950 is written TWICE, once from the HRESULT test and then
//      again with zero when the uint16 at 0x1008d8e6 is below two.  icl kept
//      both stores; write it longhand.
//    * `movzwl` then a SIGNED `cmp $2 ; jge`: a uint16 promoted to int.
//    * bare `ret`, nothing read off the stack: void(void).
// ======================================================================= //

extern uint32 g_nZEnable1008d984;                       // 0x1008d984
extern int32  g_bValid1008d954;                         // 0x1008d954
extern int32  g_bValid1008d950;                         // 0x1008d950
extern uint16 g_nStages1008d8e6;                        // 0x1008d8e6

struct State10043660
{
	uint32 m_n00;                                       // +0x00
	uint32 m_n04;                                       // +0x04
};

extern void sub_10023c40();
extern void sub_10043660(State10043660* pState);
extern void sub_10043780(State10043660* pState);
extern void sub_10011560();
extern void sub_100115b0();



// ======================================================================= //
//  DECODED BUT NOT WRITTEN
// ======================================================================= //
//
//  0x10026670  (163 bytes, 1 caller).  Set all three fixed-function
//  transforms.  Three identical blocks:
//
//      sub_100265a8(&g_1008ba9c, &mWorld);
//      pDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
//      sub_100265a8(&g_1008b95c, &mView);
//      pDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &mView);
//      sub_100265a8(&g_1008b99c, &mProj);
//      pDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &mProj);
//
//  The three locals are 0x40 bytes apart (-0xc4, -0x84, -0x44) and the three
//  sources are matrix-sized globals.  This is where +0x2c = SetTransform
//  was earned.  NOT WRITTEN because 0x100265a8 is twelve bytes past the
//  stack-ABI entry at 0x100265a0: the converter gets its source in %eax and
//  its destination in %edx, so a C call would name 0x100265a0 and the
//  displacement would be eight bytes out.  If 0x100265a0's body is ever
//  reconstructed into the SAME translation unit, this becomes writable
//  unchanged.
//
//  0x10025820  (98 bytes, 1 caller).  Same shape as 0x1001eb20 above -- walk
//  a list at 0x1008d4bc through +0x14, publish each node's +0x10 into
//  0x1008b874, call `sub_10028030(node->m_p00, &a, &b, &g_1008ba9c)`, then
//  push the node onto the free list at 0x1008d4fc through its +0x00, and
//  finally null the head.  NOT WRITTEN for the same reason: the call is to
//  0x1002803c, twelve bytes past the shim at 0x10028030, which d3dmid.cpp
//  already documents for 0x10025680.
//
//  0x1005a550  (173 bytes, 1 caller).  A loop over an array of 40-byte
//  vertices doing the homogeneous transform in place: 1.0f (0x100aa874)
//  over the w row, then three scaled rows, stride 0x28, count counted down
//  with `dec %ecx`.  Rows 0, 1 and 2 are byte-for-byte the same shape as
//  0x100149a0 (MatVMul_InPlace_H) -- but the w row is NOT.  0x100149a0 adds
//  (m32*z + m31*y) + m30*x; 0x1005a550 adds (m30*x + m32*z) + m31*y.  That
//  is a different association of the same four terms, so the source is not
//  simply MatVMul_InPlace_H in a loop, and what it is instead
//  would be a guess.  Both associations remain to be tried.
//
//  0x10001000 / 0x10001330 / 0x10003410  (816 / 832 / 849 bytes).  The six
//  clip planes 0x1000aa90 drives, three of them.  They are one family:
//  normalised against their own addresses, 0x10001000 and 0x10001330 differ
//  in 82 lines out of 275 and 0x10003410 in 194 -- the same skeleton with a
//  different plane test and a different interpolated axis.  Each opens by
//  bumping the counter at 0x1008be44, classifies every vertex into a
//  0x114-byte bitmap on the stack, sums the classifications, short-circuits
//  when the sum is zero (all out, answer 0) or equal to the count (all in,
//  answer 1), and otherwise walks the edges emitting into the scratch
//  buffer and finally divides the byte count by 0x28 to get the new vertex
//  count.  All three are tractable and they are worth a batch of their own;
//  they are too big to risk alongside ten other bodies here.
//
//  0x10011879 and 0x1006d79d are not function starts (see the file header).
// ======================================================================= //
