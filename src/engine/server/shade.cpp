// ======================================================================= //
//  s3_shade.cpp -- w_GetPointShade, the world light-grid trilinear sample.
//
//  ONE SOURCE, TWO IMAGES:
//      server.dll    0x1002cf33   1047 bytes   /O1  (ebp frame)
//      lithtech.exe  0x00405780   1386 bytes   /O2  (esp frame, /Oy)
//
//  Everything below is read off the retail server.dll and
//  lithtech.exe with `objdump -M intel`; every offset,
//  constant and branch carries the address it was read at.
//
//  PLACEMENT: src/engine/server/smgr.cpp, immediately ABOVE
//  si_GetPointShade (smgr.cpp:2698), which already calls it.  The server
//  address 0x1002cf33 falls in a run of smgr.cpp rows -- 0x1002ceac is
//  smgr.cpp:7431, 0x1002cecf is smgr.cpp:5222, and 0x1002d34a, the row
//  immediately after this one, is smgr.cpp:4981 -- so the TU is smgr.cpp,
//  not commonmodel.cpp where the w_SetupPointShade half lives.
//
//  The two types it needs -- CWorldSubFC and CShadeSample -- are ALREADY
//  reconstructed, in src/engine/server/commonmodel.cpp (:3416 and :4368).
//  They are mirrored here with a Q3S suffix so this scratch file compiles
//  standalone; see the REPORT at the bottom for the two changes the real
//  headers need.
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  SHIM -- delete all of this on integration.
// ----------------------------------------------------------------------- //
#include <math.h>

typedef unsigned char  uint8;
typedef unsigned int   uint32;

template<class T>
class _CVectorQ3S
{
public:
	_CVectorQ3S() {}
	_CVectorQ3S(T mx, T my, T mz) { x = mx; y = my; z = mz; }

	// The ONLY operator this function reaches, and it takes its argument
	// BY VALUE -- which is what puts the twelve-byte copy of m_vBase into
	// a frame slot before the subtract (server 0x1002cf4b/4e/4f `movsl`
	// x3, client 0x00405794..0x004057a5 three dword moves).  Spelling in
	// src/LT2/sdk/inc/ltvector.h:57.
	_CVectorQ3S<T> operator - (const _CVectorQ3S<T> v) const
	{
		return _CVectorQ3S<T>(x - v.x, y - v.y, z - v.z);
	}

	T x;
	T y;
	T z;
};

typedef _CVectorQ3S<float> LTVectorQ3S;
#define LTVector LTVectorQ3S

// src/engine/server/commonmodel.cpp:4368 verbatim (name theirs, already
// in the tree).  Four bytes per cell: the shipped code multiplies the
// linear index by 4 (`lea eax,[eax+ecx*4]`, server 0x1002d04b, client
// 0x004058e2) and reads +0, +1, +2 off it.
class CShadeSampleQ3S
{
public:
	uint8 m_b;                               // 0x00
	uint8 m_g;                               // 0x01
	uint8 m_r;                               // 0x02
	uint8 m_a;                               // 0x03  never assigned
};

// src/engine/server/commonmodel.cpp:3416's CWorldSubFC, with the four
// fields this function reaches given the type the shipped bytes require.
// The named offsets are the ones w_GetPointShade actually touches:
//
//   +0x00  m_pData      server 0x1002d037 `mov eax,[ebx]`
//   +0x08  m_nYStride   server 0x1002d014, client 0x004058a0
//   +0x14  m_nMaxX      server 0x1002cfb5, client 0x00405836
//   +0x18  m_nMaxY      server 0x1002cfc8, client 0x0040584d
//   +0x1c  m_nMaxZ      server 0x1002cfdd, client 0x00405860
//   +0x20  m_nZStride   server 0x1002d017, client 0x004058a3
//   +0x30  m_vScale     server 0x1002cf71/7d/86, client 0x004057e4/f7/802
//   +0x3c  m_vBase      server 0x1002cf42, client 0x00405791
//
// The three clamp bounds are SIGNED: `jge`/`jle` at 0x1002cfaf/0x1002cfba
// (and 0x00405830/0x0040583b).  Typed uint32 they would compare `jae`/
// `jbe`.  commonmodel.cpp currently spells them uint32 -- see the REPORT.
class CWorldSubFCQ3S
{
public:
	CShadeSampleQ3S* m_pData;                // 0x00
	uint32   m_n04;                          // 0x04
	int      m_nYStride;                     // 0x08
	uint32   m_n0c;                          // 0x0c
	uint32   m_n10;                          // 0x10
	int      m_nMaxX;                        // 0x14
	int      m_nMaxY;                        // 0x18
	int      m_nMaxZ;                        // 0x1c
	int      m_nZStride;                     // 0x20
	volatile float m_v24x, m_v24y, m_v24z;   // 0x24  the cell size
	LTVector m_vScale;                       // 0x30  its reciprocal
	LTVector m_vBase;                        // 0x3c  the grid origin
};
// ----------------------------------------------------------------------- //
//  END SHIM
// ----------------------------------------------------------------------- //


// ======================================================================= //
//  server.dll 0x1002cf33 (1047 bytes) / lithtech.exe 0x00405780 (1386).
//
//  Declared servermgr.h:2414 and called from si_GetPointShade
//  (smgr.cpp:2698); on the client it is reached from
//  ILTCommon::GetPointShade (lithtech.exe 0x004056b0, whose call is at
//  0x004056e9) and from 0x0046bb9a.  Those are the ONLY two entries into
//  the client body and 0x1002cefd is the only entry into the server body
//  -- every other displacement landing inside either range is a
//  mid-instruction false positive, so both extents are one whole function.
//
//  WHAT IT DOES.  Turn a world point into a grid coordinate, clamp the
//  integer cell, take the eight neighbouring samples and blend them along
//  y, then x, then z.  The grid stores b,g,r and the answer is written
//  back the same way round -- pColor[2] is red (server 0x1002d333).
//
//  ---- THE FRAME IS THE DECLARATION ORDER, AND BOTH IMAGES AGREE --------
//
//  Thirteen LTVectors and nothing else: server `sub esp,0x9c` = 156 =
//  13 x 12 exactly (0x1002cf36), client `sub esp,0xac` = 172 = 4 (the
//  int->float temp at [esp+0x10]) + 156 + 12 (0x00405780).
//
//  server (ebp frame, listed high to low)   client (esp frame, low to high)
//     -0x0c  vRight                            [esp+0x14]  v
//     -0x18  vLeft                             [esp+0x20]  vLeft
//     -0x24  v                                 [esp+0x2c]  vRight
//     -0x30  vFar                              [esp+0x38]  vNear
//     -0x3c  vNear                             [esp+0x44]  vFar
//     -0x48  vBR1                              [esp+0x50]  vTL0
//     -0x54  vBL1                              [esp+0x5c]  vTR0
//     -0x60  vTR1                              [esp+0x68]  vBL0
//     -0x6c  vTL1                              [esp+0x74]  vBR0
//     -0x78  vBR0                              [esp+0x80]  vTL1
//     -0x84  vBL0                              [esp+0x8c]  vTR1
//     -0x90  vTR0                              [esp+0x98]  vBL1
//     -0x9c  vTL0                              [esp+0xa4]  vBR1
//
//  VC6 allocates DECLARATION STATEMENTS in reverse order and the
//  declarators WITHIN one statement in forward order.  Proven on two
//  byte-identical server functions before it was applied here:
//    * CCommonMath::AlignRotation (0x10016186, MATCH) declares
//      `LTVector vRight, vUp, vForward; LTMatrix mat;` and gets
//      -0x24/-0x18/-0x0c with the 64-byte mat sunk to -0x64;
//    * w_ReadWorldLights (0x10009466, MATCH) has twenty-three locals and
//      every one lands where the reverse rule puts it -- nLen, declared
//      twentieth, takes the FIRST slot at -0x2 (a uint16, so the 2-byte
//      hole is visible), then j -0x8, fRadius -0xc, i -0x10, nNumObjects
//      -0x14, bFast -0x18; and the aggregates come out vColor -0x38,
//      vPos -0x44, vDir -0x50, vOuterColor -0x5c, vForward -0x68,
//      value[0x80] -0xe8, vRight -0xf4, vUp -0x100, sName[0x100] -0x200.
//
//  Applied here the two images CROSS-CHECK the grouping, because /Oy
//  walks the statement list the other way (first statement at the TOP of
//  the esp frame) while the declarators inside a statement still ascend:
//    * the eight corners ascend vTL0..vBR1 in BOTH frames, so they are
//      ONE statement (split 4+4 the client would put vTL0 at 0x80);
//    * vNear below vFar in BOTH, so `LTVector vNear, vFar;` is one
//      statement (as two, the client would give vFar 0x38 and vNear 0x44);
//    * v below vLeft below vRight in BOTH, so `LTVector v, vLeft, vRight;`
//      is one statement (as three, the client would give v 0x2c).
//  Three independent agreements; that is why the declarations below are
//  grouped exactly as they are.
//
//  ix/iy/iz take no slot of their own -- they are COLOURED into dead
//  vector slots (server iy at -0x8 and iz at -0x4, i.e. vRight.y/.z;
//  client iy at [esp+0x24], i.e. vLeft.y), and the int->float conversion
//  temp lives in the dead `pShadeState` parameter home at [ebp+0x8] on
//  the server.
//
//  ---- WHY THE BLEND IS 3+3 AND NOT THREE FUSED STATEMENTS -------------
//
//  Each blend step emits three `w * a` products and only then three
//  `+= t * b` ones.  That is source order, not scheduling: the second
//  group ADDS BACK OUT OF THE DESTINATION'S OWN SLOT --
//  `fadd DWORD PTR [ebp-0x14]` at 0x1002d1c4 reads vLeft.y, the very
//  slot 0x1002d1a4 had just written.  A fused `a*w + b*t` would never
//  round-trip through the variable.  In every group the FIRST component's
//  store is elided because it stays in st (0x1002d19d keeps it, 0x1002d1b3
//  adds into it), which is the only asymmetry.
//
//  ---- OPERAND ORDER: THE WEIGHT IS ON THE LEFT ------------------------
//
//  VC6 `fld`s the RIGHT operand of a float product and folds the left
//  into the `fmul`; proven in-tree by LTMatrix::SetupRot (ltmatrix.h:631
//  `tx = t * vVec.x` -> `fld [ebp+0x8]; fmul st,st(1)` at 0x10012054,
//  byte-identical at 0x10012028).  Here 0x1002d19f is
//  `fld [ebp-0x80]` (the sample) `fmul st,st(3)` (the weight), so the
//  source is `weight * sample`.  The client says the same at 0x00405aca
//  (`fld [esp+0x54]; fmul [esp+0x18]`).  BOTH IMAGES AGREE on every
//  product in the function.
// ======================================================================= //

// NEAR (server 1047/1047, 365/365, 11 blocks; lithtech 0 blocks). Every
// residual is a frame slot. Shipped packs {operator- result, vRight, iy, iz}
// at -0xc and {m_vBase copy, v, vInv.z} at -0x24. Ours packs {v, vLeft} at
// -0xc, {copy, vNear} and {result, vFar} lower down, gives vInv.z its own slot
// below the corner array, and homes iy/iz in the dead parameter slots 0x8/0xc.
// Declarations do not move any of this. Measured with a slot-by-slot map, and
// each gave the same map: `LTVector v, vLeft, vRight;` with an assigned v;
// v or vInv declared first; vRight in a statement of its own, declared last;
// ix/iy/iz as `long`, as three statements, or in reverse order. So the
// stack packer is working from something other than declaration order.
void w_GetPointShadeQ3S(uint8* pShadeState, LTVector* pPoint, uint8* pColor)
{
	CWorldSubFCQ3S* pShade = (CWorldSubFCQ3S*)pShadeState;

	// THE EIGHT CORNERS ARE ONE ARRAY, and that is what sets the frame.
	//
	// Written as eight separate locals, VC6 stack-packs vNear onto vTL0's
	// slot and vFar onto vBL0's, because with eight independent objects
	// neither live range straddles the corners'. That gives a 0x84 frame
	// where the shipped build has 0x9c -- exactly two twelve-byte vectors
	// smaller. The depth is the whole gap: slots past -0x80 need a 6-byte
	// disp32 ModRM, the shipped body has ten such FP operands against our
	// two, and 8 x 3 = 24 against a 23-byte difference.
	//
	// One array cannot be packed that way: its live range is one range and
	// it straddles vNear's birth. It also reproduces the shipped layout,
	// element 0 at -0x9c ascending to element 7 at -0x48 -- ascending, which
	// is what an array does and what frame-growth direction cannot explain,
	// and the client frame at [esp+0x50] ascends the same way.
	//
	// The names are kept as macros so the 48 assignment and read sites below
	// read exactly as they did.
	//
	// MEASURED, AND ONLY HALF RIGHT. 1024 bytes -> 1060 against a shipped
	// 1047, so the size error halves from 23 under to 13 over -- but the
	// mnemonic diff stays at 21 blocks and the instruction count does not
	// move (362 against 365). The frame-slot SIGN residual the older note
	// recorded is still every one of those blocks: ours are `X(%ebp)` where
	// the shipped build has `-X(%ebp)`. So the array is kept as the better
	// of the two spellings and the depth argument above stands, but the
	// claim that it explains the WHOLE gap is not supported: something else
	// is putting our locals on the other side of the frame pointer.
	LTVector vSample[8];

	#define vTL0 vSample[0]
	#define vTR0 vSample[1]
	#define vBL0 vSample[2]
	#define vBR0 vSample[3]
	#define vTL1 vSample[4]
	#define vTR1 vSample[5]
	#define vBL1 vSample[6]
	#define vBR1 vSample[7]

	LTVector vNear, vFar;
	LTVector vLeft, vRight;
	LTVector vInv;

	CShadeSampleQ3S* pCell;
	int ix, iy, iz;

	// 0x1002cf42..0x1002cf89 / 0x00405791..0x00405805.  The assignment
	// (not an initialisation) is what puts the operator- result in a
	// second twelve-byte temp and copies it into v -- exactly the shape
	// CCommonMath::SetupRotationAroundPoint shows for `vBack = -*pPoint`
	// (servermath.cpp:319, return slot -0x18, then three `movsl` into
	// vBack at -0x0c).  The scale is applied component by component
	// against m_vScale read straight out of the object; `v *= m_vScale`
	// would copy m_vScale into a by-value temp first, and no such copy
	// is in either image.
	// Declared at its initialisation, not in the `v, vLeft, vRight`
	// statement: codegen-neutral on the frame but flips one SIB pair,
	// `movzbl (%edx,%eax,1)` at 0x1002d093 / 0x1002d12d, to shipped order.
	LTVector v = *pPoint - pShade->m_vBase;
	v.x *= pShade->m_vScale.x;
	v.y *= pShade->m_vScale.y;
	v.z *= pShade->m_vScale.z;

	// 0x1002cf8c/98/a3 -> _ftol 0x1003e0f0; client 0x0040580d/18/27 ->
	// 0x004928c0.  v.x is still in st from the `fst` above, hence the
	// `fld st(0)` rather than a reload on the server.
	ix = (int)v.x;
	iy = (int)v.y;
	iz = (int)v.z;

	// 0x1002cfad..0x1002cfe8 / 0x0040582c..0x00405869.  if/else-if: each
	// low arm ends in a `jmp` PAST the high test (0x1002cfb3, 0x1002cfc6,
	// 0x1002cfdb).  The bound is loaded once and used twice, so VC6 CSEs
	// it -- consistent with a plain, non-volatile int field.
	if (ix < 0)
		ix = 0;
	else if (ix > pShade->m_nMaxX)
		ix = pShade->m_nMaxX;

	if (iy < 0)
		iy = 0;
	else if (iy > pShade->m_nMaxY)
		iy = pShade->m_nMaxY;

	if (iz < 0)
		iz = 0;
	else if (iz > pShade->m_nMaxZ)
		iz = pShade->m_nMaxZ;

	// 0x1002cfe8..0x1002d021 / 0x00405869..0x004058aa.  Eight bytes of
	// stack (`push ecx; push ecx` on the server, `sub esp,0x8` on the
	// client), the float widened into it, floor, then `fsubr` back
	// against the slot.  The store of v.z is dead -- its value stays in
	// st and -0x1c is reused for (1.0f - v.z) at 0x1002d041 -- which is
	// why only two of the three have an `fstp` of their own.
	v.x -= (float)floor(v.x);
	v.y -= (float)floor(v.y);
	v.z -= (float)floor(v.z);

	// THE WEIGHTS ARE A NAMED VECTOR, computed once, here (0x1002d027,
	// 0x1002d02e, 0x1002d03d / client 0x004058b5..).  Written inline as
	// (1.0f - v.y) at each use, VC6 builds them lazily at first use:
	// 1060 bytes, 362 insns, 21 blocks.  As three named floats they hoist
	// here (365 insns, lithtech mnemonic-identical) but the spilled z gets
	// a slot of its own (frame 0x94).  As a VECTOR, x and y stay in st and
	// only vInv.z is stored -- packed onto v's dead slot at v.z's offset,
	// -0x1c, exactly as shipped -- which gives frame 0x9c and size-exact
	// on both images.  A constructor `LTVector vInv(...)` is worse on the
	// client (1390/380).
	vInv.x = 1.0f - v.x;
	vInv.y = 1.0f - v.y;
	vInv.z = 1.0f - v.z;

	// 0x1002d014..0x1002d04b / 0x004058a0..0x004058e2.  Cells are four
	// bytes, so the whole index folds into `lea eax,[eax+ecx*4]`.
	pCell = pShade->m_pData +
	        (ix + pShade->m_nYStride * iy + pShade->m_nZStride * iz);

	// 0x1002d055..0x1002d0f9 / 0x004058e7..0x0040599a.  Twenty-four
	// uint8 -> float conversions in exactly this order: the y+1 row
	// first, then the y row, left cell then right cell, and r,g,b within
	// each.  The two values whose stores are elided are vTL0.x and
	// vBL0.x, which is what leaves the -0x84 hole in the server frame
	// and the [esp+0x68] hole in the client's.
	vTL0.x = pCell[pShade->m_nYStride].m_r;      // [esi+eax+2]  0x1002d055
	vTL0.y = pCell[pShade->m_nYStride].m_g;      // [ecx+1]      0x1002d063
	vTL0.z = pCell[pShade->m_nYStride].m_b;      // [ecx]        0x1002d06d

	vTR0.x = pCell[pShade->m_nYStride + 1].m_r;  // [ecx+6]      0x1002d07c
	vTR0.y = pCell[pShade->m_nYStride + 1].m_g;  // [ecx+5]      0x1002d086
	vTR0.z = pCell[pShade->m_nYStride + 1].m_b;  // [edx+eax]    0x1002d093

	vBL0.x = pCell[0].m_r;                       // [eax+2]      0x1002d0a3
	vBL0.y = pCell[0].m_g;                       // [eax+1]      0x1002d0b3
	vBL0.z = pCell[0].m_b;                       // [eax]        0x1002d0c6

	vBR0.x = pCell[1].m_r;                       // [eax+6]      0x1002d0d2
	vBR0.y = pCell[1].m_g;                       // [eax+5]      0x1002d0df
	vBR0.z = pCell[1].m_b;                       // [eax+4]      0x1002d0ec

	// 0x1002d0f3 `lea eax,[eax+edi*4]` / client 0x0040599d.
	pCell += pShade->m_nZStride;

	vTL1.x = pCell[pShade->m_nYStride].m_r;      // 0x1002d0ff
	vTL1.y = pCell[pShade->m_nYStride].m_g;      // 0x1002d10d
	vTL1.z = pCell[pShade->m_nYStride].m_b;      // 0x1002d11a

	vTR1.x = pCell[pShade->m_nYStride + 1].m_r;  // 0x1002d126
	vTR1.y = pCell[pShade->m_nYStride + 1].m_g;  // 0x1002d12d
	vTR1.z = pCell[pShade->m_nYStride + 1].m_b;  // 0x1002d13a

	vBL1.x = pCell[0].m_r;                       // 0x1002d147
	vBL1.y = pCell[0].m_g;                       // 0x1002d154
	vBL1.z = pCell[0].m_b;                       // 0x1002d161

	vBR1.x = pCell[1].m_r;                       // 0x1002d16d
	vBR1.y = pCell[1].m_g;                       // 0x1002d17a
	vBR1.z = pCell[1].m_b;                       // 0x1002d184

	// ---- y, the near plane.  0x1002d19d..0x1002d212 ------------------
	// vInv.y and v.y stay in st from 0x1002d02b..0x1002d030; of the six
	// (v.x/y/z, vInv.x/y/z) only vInv.z is spilled.
	vLeft.x  = vInv.y * vBL0.x;   // 0x1002d19d  fmul st,st(2)
	vLeft.y  = vInv.y * vBL0.y;   // 0x1002d19f  -> [ebp-0x14]
	vLeft.z  = vInv.y * vBL0.z;   // 0x1002d1a7  -> [ebp-0x10]
	vLeft.x += v.y * vTL0.x;            // 0x1002d1af  -> [ebp-0x18]
	vLeft.y += v.y * vTL0.y;            // 0x1002d1bc  fadd [ebp-0x14]
	vLeft.z += v.y * vTL0.z;            // 0x1002d1c7  fadd [ebp-0x10]

	vRight.x  = vInv.y * vBR0.x;  // 0x1002d1d5
	vRight.y  = vInv.y * vBR0.y;  // 0x1002d1da  -> [ebp-0x08]
	vRight.z  = vInv.y * vBR0.z;  // 0x1002d1e2  -> [ebp-0x04]
	vRight.x += v.y * vTR0.x;           // 0x1002d1ea  -> [ebp-0x0c]
	vRight.y += v.y * vTR0.y;           // 0x1002d1f9
	vRight.z += v.y * vTR0.z;           // 0x1002d207

	// ---- x, the near plane.  0x1002d215..0x1002d24a ------------------
	vNear.x  = vInv.x * vLeft.x;  // 0x1002d215
	vNear.y  = vInv.x * vLeft.y;  // 0x1002d21a  -> [ebp-0x38]
	vNear.z  = vInv.x * vLeft.z;  // 0x1002d221  -> [ebp-0x34]
	vNear.x += v.x * vRight.x;          // 0x1002d229  -> [ebp-0x3c]
	vNear.y += v.x * vRight.y;          // 0x1002d237
	vNear.z += v.x * vRight.z;          // 0x1002d242

	// ---- y, the far plane.  0x1002d24d..0x1002d2b4 -------------------
	// vLeft and vRight are REUSED; that reuse is why the frame holds
	// thirteen vectors and not seventeen.
	vLeft.x  = vInv.y * vBL1.x;   // 0x1002d24d
	vLeft.y  = vInv.y * vBL1.y;   // 0x1002d252
	vLeft.z  = vInv.y * vBL1.z;   // 0x1002d25a
	vLeft.x += v.y * vTL1.x;            // 0x1002d262
	vLeft.y += v.y * vTL1.y;            // 0x1002d26e
	vLeft.z += v.y * vTL1.z;            // 0x1002d279

	vRight.x  = vInv.y * vBR1.x;  // 0x1002d284
	vRight.y  = vInv.y * vBR1.y;  // 0x1002d28c
	vRight.z  = vInv.y * vBR1.z;  // 0x1002d294  fmul [ebp-0x40]
	vRight.x += v.y * vTR1.x;           // 0x1002d297
	vRight.y += v.y * vTR1.y;           // 0x1002d2a2
	vRight.z += v.y * vTR1.z;           // 0x1002d2ad

	// ---- x, the far plane.  0x1002d2bb..0x1002d2eb -------------------
	vFar.x  = vInv.x * vLeft.x;   // 0x1002d2bb  -> [ebp-0x30]
	vFar.y  = vInv.x * vLeft.y;   // 0x1002d2c3  -> [ebp-0x2c]
	vFar.z  = vInv.x * vLeft.z;   // 0x1002d2cb  fmul [ebp-0x10]
	vFar.x += v.x * vRight.x;           // 0x1002d2ce
	vFar.y += v.x * vRight.y;           // 0x1002d2d9
	vFar.z += v.x * vRight.z;           // 0x1002d2e4  stays in st

	// ---- z.  0x1002d2ef..0x1002d323.  The answer goes back into
	// vRight, whose slots -0xc/-0x8/-0x4 the three `_ftol` calls read.
	vRight.x  = vInv.z * vNear.x; // 0x1002d2ef  fmul [ebp-0x1c]
	vRight.y  = vInv.z * vNear.y; // 0x1002d2f5  -> [ebp-0x08]
	vRight.z  = vInv.z * vNear.z; // 0x1002d2fe  -> [ebp-0x04]
	vRight.x += v.z * vFar.x;           // 0x1002d307  -> [ebp-0x0c]
	vRight.y += v.z * vFar.y;           // 0x1002d313
	vRight.z += v.z * vFar.z;           // 0x1002d31e  stays in st

	// 0x1002d325..0x1002d343 / 0x00405cb9..0x00405cdd.  Blue last, and
	// the third conversion needs no `fld` because the value never left
	// st.  si_GetPointShade (smgr.cpp:2698) unwinds the same order.
	pColor[2] = (uint8)vRight.x;
	pColor[1] = (uint8)vRight.y;
	pColor[0] = (uint8)vRight.z;
}


// ======================================================================= //
//  R E P O R T
// ======================================================================= //
//
//  ---- VERDICTS --------------------------------------------------------
//
//  server.dll 0x1002cf33, 1047 bytes, ONE function.
//    signature: void w_GetPointShade(uint8* pShadeState, LTVector* pPoint,
//                                    uint8* pColor)   [servermgr.h:2414]
//    verified:  sweep of every e8/e9/0f8x/eb/7x displacement and every
//               aligned dword in all sections for targets inside
//               0x1002cf33..0x1002d349 finds exactly ONE entry from
//               outside -- `call 0x1002cf33` at 0x1002cefd -- and six
//               internal branch targets, all of them the clamp joins
//               (0x1002cfb5/be/c8/d3/dd/e8).  No data reference, no jump
//               into the interior, `pop edi/esi/ebx; leave; ret` at
//               0x1002d345..0x1002d349, next row 0x1002d34a.  Frame
//               0x9c = exactly 13 x sizeof(LTVector).
//    confidence of byte-identity: MODERATE-TO-GOOD.  Every statement
//    below is pinned by a shipped instruction; the two open items are
//    listed under COIN-FLIPS and neither changes the instruction
//    sequence, only which frame displacement appears.
//
//  lithtech.exe 0x00405780, 1386 bytes, ONE function, SAME SOURCE.
//    verified:  same sweep over 0x00405780..0x00405ce9 -- two entries,
//               both to the start (0x004056e9 inside
//               ILTCommon::GetPointShade 0x004056b0, and 0x0046bb9a), no
//               interior target from outside, eleven-byte `nop` run at
//               0x00405775..0x0040577f ahead of it, `add esp,0xac; ret`
//               at 0x00405ce3.  Frame 0xac = 4 + 13 x 12 + 12.
//    confidence: MODERATE.  The body is the same function instruction
//    for instruction; the size gap is /Ot taking `xor edx,edx; mov dl,m`
//    where /Os takes `movzx`, and `fld [0x004a1a08]` where /Os takes
//    `fld1`.  ONE residual is known and unexplained -- see below.
//
//  ---- DO THE TWO IMAGES AGREE? ----------------------------------------
//
//  On everything that is source-reachable, YES, and the agreement is the
//  main evidence in this file:
//
//    * the twenty-four sample loads are in the same order in both, down
//      to the two whose stores are elided (vTL0.x and vBL0.x -- the
//      -0x84 hole in the server frame, the [esp+0x68] hole in the
//      client's);
//    * the blend is 3-then-3 in both, with the second group adding out
//      of the destination's own slot;
//    * every product has the WEIGHT as the left operand in both;
//    * the plane advance is a single pointer bump in both
//      (0x1002d0f3 / 0x0040599d);
//    * the three declaration GROUPS cross-check, because /Oy walks the
//      statement list the same way but grows the frame the other way:
//      corners ascend vTL0..vBR1 in both, vNear is below vFar in both,
//      and v is below vLeft below vRight in both.  Split differently,
//      the client's frame would not come out.
//
//  THEY DISAGREE IN EXACTLY ONE PLACE, and it is an allocation, not an
//  ordering.  The final z-blend's destination is:
//      server  -0x0c/-0x08/-0x04   = vRight
//      client  [esp+0x14/18/1c]    = v
//  In both it is the FIRST slot the frame allocator hands out (the
//  server grows down from ebp, the client up from esp), so one of the
//  two compilers has moved the triple off the variable the source
//  named.  vRight is written below because it is the one that needs NO
//  relocation on the server, which is the primary target; if the client
//  then misses on three displacements, changing those six statements to
//  write `v` is the one-line alternative, and a fourteenth LTVector
//  declared after vRight is the other.  Nothing else in the function
//  moves either way.
//
//  ---- WHAT BELONGS IN THE REPO (no file was edited) --------------------
//
//  1. src/engine/server/commonmodel.cpp:3416, class CWorldSubFC.  Two
//     changes, both forced by the shipped bytes of THIS function:
//
//       a. m_n14 / m_n18 / m_n1c must be SIGNED (`int`).  The clamps
//          compare `jge`/`jle` (0x1002cfaf, 0x1002cfba and the four
//          after them; client 0x00405830/0x0040583b).  As uint32 the
//          same source emits `jae`/`jbe`.  m_n08 and m_n20 are also
//          used as signed strides here, though `imul` cannot prove it.
//
//       b. the triples at +0x30 and +0x3c must be LTVector, not three
//          `volatile float`s.  `*pPoint - pShade->m_vBase` needs a
//          by-value LTVector at +0x3c -- that is the twelve-byte copy
//          at 0x1002cf42..0x1002cf4f -- and a volatile object cannot be
//          passed to `_CVector::operator-(const _CVector)`.
//
//          The `volatile` is there for CWorldSubFC::Clear (0x1001424e),
//          whose comment says a volatile store is the barrier that keeps
//          the eight dword stores ahead of the nine `fldz`/`fstps`
//          pairs.  UNTESTED SUGGESTION: only the FIRST float store needs
//          to be that barrier -- once the dwords are all emitted there
//          is nothing left for the scheduler to interleave -- so keeping
//          `volatile float m_v24x, m_v24y, m_v24z;` at +0x24 and making
//          +0x30 / +0x3c plain `LTVector m_vScale, m_vBase;` may hold
//          Clear while unblocking this function.  If it does not, this
//          function can cast instead:
//          `*pPoint - *(LTVector*)&pShade->m_v3cx`.
//
//  2. CWorldSubFC and CShadeSample live inside commonmodel.cpp, but
//     0x1002cf33 is a smgr.cpp address (its neighbours 0x1002ceac,
//     0x1002cecf and 0x1002d34a are all smgr.cpp).  Both classes move to a
//     shared header -- servermgr.h is where w_GetPointShade is already
//     declared -- or be re-declared in smgr.cpp.
//
//  3. servermgr.h:2414 can keep its `uint8*` first parameter: the local
//     cast at the top of the body costs nothing (`mov ebx,[ebp+0x8]`),
//     and keeping it means smgr.cpp:2702's call site is untouched.  The
//     dead parameter home at [ebp+0x8] is then free, which is exactly
//     where the shipped build parks the int->float conversion temp
//     (0x1002d05d and every `fild [ebp+0x8]` after it).
//
//  ---- COIN-FLIPS NOT SETTLED ------------------------------------------
//
//  (1) `vInv.y` written INLINE at each use (what is below) versus
//      three named locals computed once:
//          float fOneMinusX = 1.0f - v.x;  (etc.)
//      The shipped build creates all three at 0x1002d027/0x1002d02e/
//      0x1002d03d -- immediately after v.x/v.y/v.z are defined and long
//      before first use -- and spills only vInv.z, into -0x1c,
//      which is v.z's own now-dead slot.  Inline says that placement is
//      CSE hoisting to the definition point; named locals say it is
//      source order.  Inline is chosen because the server frame is
//      exactly thirteen vectors with no scalar slot at all, and a
//      spilled NAMED float would have to be colour-merged the same way
//      iy/iz are.  WHAT WOULD SETTLE IT: build both; if the named form
//      gives `sub esp,0xa0` the inline form is right, if both give 0x9c
//      the question is moot.
//
//  (2) The final destination, vRight versus v versus a fourteenth
//      vector -- see "THEY DISAGREE" above.  WHAT WOULD SETTLE IT: one
//      build of each; the server is decisive on its own (it writes
//      -0x0c/-0x08/-0x04 and nothing else fits without relocation), so
//      really only the client's three displacements are in play.
//
//  (3) `pShade->m_nYStride * iy` versus `iy * pShade->m_nYStride`.  Both
//      images put the FIELD in the register and the index in the r/m
//      operand (`mov eax,edx; imul eax,[ebp-0x8]` at 0x1002d01b, and
//      `mov ecx,eax; imul ecx,[esp+0x2c]` at 0x004058aa) -- but both
//      already hold the field in a register for other reasons, so the
//      operand order is not proven.  Cost if wrong: nothing, or one
//      `mov`.  WHAT WOULD SETTLE IT: a build.
//
//  (4) The eight corners as eight names in one declaration versus
//      `LTVector vCorner[8];`.  Indistinguishable -- an array is one
//      declarator and lands in the same 96 bytes with the same ascending
//      element order in both frames.  Eight names are used because every
//      access is a compile-time-constant index.
//
//  (5) `v.x -= (float)floor(v.x)` versus `v.x -= floor(v.x)`.  The cast
//      is codegen-neutral for VC6 without /Op (no store/reload appears
//      at 0x1002cff2), so the cast is kept only to silence C4244 under
//      /W3.  If the shipped build warned, the cast was not there; that
//      cannot be read off bytes.
//
//  ---- THINGS VERIFIED THAT ARE NOT IN THE CODE ------------------------
//
//    * `floor` is 0x1004007e (server) / 0x004930bf (client); `_ftol` is
//      0x1003e0f0 / 0x004928c0.  Both take/return through the x87 stack
//      in the shipped code, which is the CRT's own convention.
//    * There is NO null check and NO early return in either body: one
//      `ret` each, so the /O1-tail-merge-versus-/O2 question does not
//      arise here.
//    * g++ -fsyntax-only -Wall on this file: clean.  Call sequence:
//      VACUOUS / NOT RUN -- no object was produced.
//
//  ---- THE FRAME RULE IS NOT UNIVERSAL, AND THAT MATTERS -------
//
//  The rule "The frame is the declaration order" says the bump allocator
//  walks DOWN in declaration order, first-declared nearest ebp.  Five
//  functions read out of src/engine/server/Release/*.obj (one build, one
//  compiler, cl 8447 /O1) split two ways and the rule holds for
//  only two of them:
//
//    DOWN in declaration order (the rule):
//      w_IsectObjectGeometry  0x1000f74e / objbank.obj -- fDist -0x4,
//        hPoly -0x8, vStartPt -0x14, vPoint -0x20, vLocalTo -0x2c,
//        vLocalFrom -0x38, with the 16-byte LTPlane sunk to -0x48.  (Still
//        a near: 298/298 with ten slot mismatches.)
//      CCommonMath::SetupRotationAroundPoint 0x100160d7 / servermath.obj
//        -- vBack -0xc, a 12-byte temp -0x18, then mOut -0x58, mBack
//        -0x98, mRot -0xd8, mResult -0x198, with only mA/mB transposed.
//
//    UP in declaration order (the opposite):
//      w_ReadWorldLights 0x10009466 / commonmodel.obj (byte MATCH) --
//        bFast -0x18, nNumObjects -0x14, i -0x10 in declaration order
//        ascending, nRecordSize first at -0x2; and the aggregates
//        vUp -0x100, vRight -0xf4, value[0x80] -0xe8, vForward -0x68,
//        vOuterColor -0x5c, vDir -0x50, vPos -0x44, vColor -0x38, with
//        sName[0x100] at -0x200.  The DOWN rule mispredicts eight of
//        these; this one is byte-identical, so the bytes are not in doubt.
//      CCommonMath::AlignRotation 0x10016186 / servermath.obj (MATCH) --
//        `LTVector vRight, vUp, vForward;` gives -0x24/-0x18/-0x0c.
//      wm_UpdateDimsFromGeometry / commonphysics.obj -- vSize -0x18,
//        vTemp -0xc.
//
//  The discriminator is not known.  It is not the TU (servermath.obj
//  supplies one of each), not the compiler revision (all five objects are
//  from one 2026-09-20 13:30 run at the default CL_REV=8447), and not
//  whether the address is taken (both groups have examples).  So the rule
//  is stated here as an OPEN QUESTION, not a correction -- but it is worth
//  attention, because the rule's two usual specimens
//  are exactly the two that are still nears, and both of their residuals
//  are nothing but frame displacements.
//
//  WHAT THIS FUNCTION'S DECLARATION ORDER RESTS ON IS NOT THAT RULE.  It
//  rests on the two shipped images of this function agreeing.  One
//  assignment -- three statements, `{8 corners}`, `{vNear, vFar}`,
//  `{v, vLeft, vRight}`, the statements taken in reverse and the
//  declarators inside each one ascending -- reproduces all thirteen slots
//  of the server's ebp frame AND all thirteen of the client's esp frame,
//  which grows the other way.  The alternatives were each checked and each
//  breaks one image or the other:
//    * corners split 4+4  -> the client would put vTL0 at [esp+0x80].
//    * vNear and vFar as separate statements -> the client would give
//      vFar [esp+0x38] and vNear [esp+0x44]; it is the other way round.
//    * v, vLeft, vRight as separate statements -> the client would give
//      v [esp+0x2c]; it has v at [esp+0x14].
//    * the DOWN rule applied to any grouping -> the server would
//      put the eight corners at the TOP of the frame; they are at the
//      bottom, -0x9c..-0x40.
//  That is three independent agreements across two differently-shaped
//  frames, which is why the ordering here is reported as evidence rather
//  than as a guess.
// ======================================================================= //
