// *********************************************************************** //
//
//  MODULE:   BIG_4E3E4 (.CPP)
//
//  PURPOSE:  One function -- 0x1004e3e0 (stack entry) / 0x1004e3e4
//            (register entry), 5,891 bytes.  ONE caller, 0x1004dfb3,
//            inside CShadowRenderer::sub_1004b720 (src/engine/d3d/
//            big4b720.cpp), which had already declared it as
//
//                void sub_1004e3e4(ShadowFrustum* pFrustum,
//                                  ShadowItem* pItem, float fRadius);
//
//            This decode confirms that signature off the bytes and fills
//            in the body: it is d3d.ren's DRAW-ONE-SHADOW-POLYGON pass.
//
//  ------------------------------------------------------------------- //
//  THE EXTENT.  Corrected.  The enumerated start is a /Qip register
//  entry, and that is real.
//  ------------------------------------------------------------------- //
//
//      the extent 0x1004e3e4, 5887, walked from the bytes:
//        ret 1 (0x1004e433) / push %ebp frames 1 (0x1004e3e4)
//        /Qip  !! THIS ROW STARTS AT A REGISTER ENTRY
//        branch targets 0 outside the range or mid-instruction
//        last instruction ends exactly at the stated end
//
//  CONFIRMED, byte by byte:
//
//    * 0x1004e3d0..0x1004e3d7 is the previous function (`push %esi ;
//      mov 0x3c(%ecx),%eax ; add $0x4,%esp ; ret`), 0x1004e3d8..0x1004e3df
//      is eight 0x90 pads, and 0x1004e3e0 -- SIXTEEN-BYTE ALIGNED -- is
//      `mov 0x4(%esp),%eax` (4 bytes) falling straight into `push %ebp`
//      at 0x1004e3e4.  That is exactly the /Qip pair: a stack
//      entry that materialises the first argument in a register and falls
//      into the register entry.  The function enumeration has
//      the four bytes as a separate `0x1004e3e0,4,gap` row; they are not a
//      gap, they are this function's first entry.
//    * The register copy is DEAD in the body: %eax is overwritten by
//      `mov $0x28d8,%eax` at 0x1004e3e7 and the first argument is re-read
//      from the stack at 0x1004e4d3 (`mov 0x8(%ebp),%ebx`).  The sole
//      caller enters at 0x1004e3e4 with %eax already holding &sFrustum
//      (`lea -0x5ac(%ebp),%eax` at 0x1004df96) and %ecx holding `this`,
//      so both entries see the same three stack arguments and both
//      `ret $0xc`.
//    * SO THE CORRECTED EXTENT IS 0x1004e3e0..0x1004fae3, 5,891 bytes.
//      Walked again from there, the start is the STACK entry, the
//      register entry is at 0x1004e3e4, and nothing else is flagged.
//
//  ONE FUNCTION, not several:
//
//    * ONE `ret` in the whole 5,891 bytes, at 0x1004e433 -- only 0x53
//      into the row, with 5,808 bytes after it.  Every one of those bytes
//      is /O2 cold-block sinking: the six clip-epsilon arms at
//      0x1004fa09 / 0x1004fa2c / 0x1004fa51 / 0x1004fa76 / 0x1004fa9b /
//      0x1004fac0 each jump BACK to 0x1004e8ac / 0x1004eb8f / 0x1004ee74 /
//      0x1004f14d / 0x1004f42e / 0x1004f70a, and the two reject
//      trampolines at 0x1004f9ff and 0x1004fa04 are `jmp 0x1004e42d` --
//      into this function's own epilogue.
//    * ONE `push %ebp ; mov %esp,%ebp`, at 0x1004e3e4.  No second
//      prologue and no lea-nop padding anywhere inside the range.
//    * Every branch target in the range is inside the range (a hand pass
//      over all 78 distinct targets).
//    * 0x1004fae3..0x1004faef is thirteen 0x90 pads and 0x1004faf0 is
//      `push %ebp ; mov %esp,%ebp ; sub $0x10,%esp` -- the next function,
//      a console-variable registrar.
//
//  ------------------------------------------------------------------- //
//  HOW STRONG THE ACCEPTANCE TEST IS HERE -- READ THIS FIRST
//  ------------------------------------------------------------------- //
//
//      the call count over 0x1004e3e0..0x1004fae3:
//      5,891 bytes, 22 calls (21 direct, 1 indirect), 268 bytes per call
//
//  268 B/call is the WEAK band, and it is worse
//  than that number suggests, because the calls are not spread out:
//
//      0x1004e3e0 .. 0x1004e782    20 of the 22 calls, in 931 bytes
//      0x1004e783 .. 0x1004f8fb    ZERO calls, 4,473 bytes  (76% of the
//                                  function)
//      0x1004f8fc .. 0x1004f9fc    the last 2 calls
//
//  So the call sequence witnesses roughly 24% of the bytes and is blind
//  to the entire six-plane clipper.  The three instruments wanted
//  in that band are all present and all three are quantitative:
//
//    1. AN EXACT SIBLING.  The 4,472 call-free bytes are the SAME SOURCE
//       as src/engine/d3d/big6d910.cpp's CLIP6D910_HEAD/LERP/TAIL macro
//       expansion -- the same six planes, in the same order, on the same
//       two globals, with the same bit numbers:
//
//         bit 0x01  z >= g_f1008b908          0x1004e7ea  (fcomps/jae)
//         bit 0x04  -z < x                    0x1004eabc  (fchs/jb)
//         bit 0x08  y < z                     0x1004eda0  (jb)
//         bit 0x10  x < z                     0x1004f07c  (jb)
//         bit 0x20  -z < y                    0x1004f358  (fchs/jb)
//         bit 0x02  z <= g_f1008b910          0x1004f63c  (fcomps/jbe)
//
//       and src/engine/d3d/d3dr3a.cpp carries two members of the same
//       family (0x10002d40 "x < z", 0x100030a0 "-z < y") as standalone
//       functions, expression for expression.  The ONE structural
//       difference here is the vertex: 0x24 bytes with THREE interpolated
//       floats (+0x18/+0x1c/+0x20) where big6d910's is 0x28 with four and
//       d3dr3a's is 0x28 with four.  Every plane block below was diffed
//       against big6d910's macro instruction by instruction, not skimmed.
//
//    2. AN EXACT FRAME DECOMPOSITION.  `mov $0x28d8,%eax ; call
//       0x100805a0` at 0x1004e3e7 -- 10,456 bytes -- and every slot in it
//       is DUMPED, not assumed:
//
//         -0x28b8 .. -0x27c9   30 floats, 8-byte stride, = 6 planes x
//                              {fRed,fGreen,fBlue,fAlpha,fSpec}.  Plane
//                              1 uses -0x28b8/-0x28b0/-0x28a8/-0x28a0/
//                              -0x2898 (0x1004e928..0x1004ea18), plane 6
//                              uses -0x27f0/-0x27e8/-0x27e0/-0x27d8/
//                              -0x27d0 (0x1004f786..0x1004f876).  -0x28b8
//                              + 29*8 = -0x27d0 exactly.
//         -0x27cc              void* pData      (out-param, 0x1004e3fb)
//         -0x27c8              int32 nVerts     (out-param, 0x1004e401)
//         -0x27c4 .. -0x15c5   Vert4e3e4 aVerts[128] = 0x1200 bytes.
//                              THE 128 IS READ OFF THE FRAME, and it is
//                              the same 128 that 0x1000a740 enforces.
//         -0x15c4              Vert4e3e4* pVerts    (0x1004e4d6)
//         -0x15c0/-0x15b4/     three 12-byte vector temps (icl's hidden
//         -0x15a8              return / by-value slots, 0x1004e47c,
//                              0x1004e598, 0x1004e5b2)
//         -0x159c .. -0x1075   SIX 0xdc plane blocks.  Flag arrays at
//                              -0x159c, -0x14c0, -0x13e4, -0x1308,
//                              -0x122c, -0x1150 (0x1004e7d8, 0x1004eaaa,
//                              0x1004ed8e, 0x1004f06a, 0x1004f346,
//                              0x1004f62a) -- 0xdc apart, and 0xdc =
//                              0xc8 (int32[50]) + 5 int32 fistpl temps
//                              (-0x14d4..-0x14c4 for plane 1).  FIFTY,
//                              the same fifty d3dr3a.cpp and big6d910.cpp
//                              both read off their own frames.
//         -0x1074 .. -0x75     D3DVert4e3e4 aOut[128] = 0x1000 bytes,
//                              32-byte FVF 0x1c4 vertices.
//         -0x70 .. -0x0c       the scalars, listed in the body.
//
//       -0x28b8 + 0x28b8 = 0, so the 0x28d8 frame is the 0x28b8 above
//       plus 0x20 of bottom alignment.  A seventh plane, a fourth
//       interpolated float, a 49th or 51st flag, or a 64- or 256-vertex
//       array would each move at least one of those numbers.
//
//    3. THE LITERAL POOL, read in order.  Bounded below by 0x100aa254
//       (unreferenced anywhere) and above by 0x100aa2d0 (unreferenced
//       anywhere), our run is 0x100aa258..0x100aa2cc and it reads as a
//       transcript:
//
//         0x100aa258  1.0f    `flds ; fdivs 0x10(%ebp)`  1.0f/fRadius
//         0x100aa25c  1.0f    `fsubrs`                   1.0f - t
//         0x100aa260  0.0f    `flds ; fcomp`             the < 0 test
//         0x100aa264  0.0f    `flds`                     the clamped value
//         0x100aa268  1.0f    DEAD -- never loaded
//         0x100aa26c  1.0f    DEAD -- never loaded
//         0x100aa270  -1e-5   plane 1 inline epsilon
//         0x100aa274  1.0f    DEAD
//         0x100aa278  -1e-5   plane 2      0x100aa27c  1.0f  DEAD
//         0x100aa280  -1e-5   plane 3      0x100aa284  1.0f  DEAD
//         0x100aa288  -1e-5   plane 4      0x100aa28c  1.0f  DEAD
//         0x100aa290  -1e-5   plane 5      0x100aa294  1.0f  DEAD
//         0x100aa298  -1e-5   plane 6      0x100aa29c  1.0f  DEAD
//         0x100aa2a0  +1e-5   plane 6 sunk arm   0x100aa2a4  0.0f
//         0x100aa2a8  +1e-5   plane 5           0x100aa2ac  0.0f
//         0x100aa2b0  +1e-5   plane 4           0x100aa2b4  0.0f
//         0x100aa2b8  +1e-5   plane 3           0x100aa2bc  0.0f
//         0x100aa2c0  +1e-5   plane 2           0x100aa2c4  0.0f
//         0x100aa2c8  +1e-5   plane 1           0x100aa2cc  0.0f
//
//       TWO THINGS COME OUT OF THAT.  First, it settles the epsilon
//       spelling: the six inline `-1e-5` are adjacent to each other and
//       the six `+1e-5` are in a separate run, so the second comparison
//       is NOT adjacent to the first in source order -- it is emitted
//       with the sunk block, which is what a short-circuit `||` whose
//       second arm got sunk looks like.  (The `+1e-5`/`0.0f` pairs
//       descend, plane 6 lowest, because the six sunk blocks' literals
//       were allocated in reverse of their emission order.)  Second, the
//       eight DEAD `1.0f` are a property of THIS FAMILY'S SOURCE and not
//       of this decode: the standalone sibling 0x10002d40 has exactly the
//       same dead literal, 0x100a4094 = 1.0f sitting between its
//       0x100a4090 = -1e-5 and its 0x100a4098 = +1e-5, and d3dr3a.cpp's
//       decode does not account for it either.  It is listed as an open
//       item at the bottom of this file rather than guessed at.
//
//  ------------------------------------------------------------------- //
//  WHAT IT IS
//  ------------------------------------------------------------------- //
//
//  Draw one projected-shadow polygon.  Its caller (big4b720.cpp REGION H)
//  has already built a ShadowFrustum for the light and has decided, from
//  the two plane distances at 0x1004df26, that this polygon faces the
//  light; this function turns the polygon into a shadow-textured triangle
//  fan and draws it.
//
//   A  0x1004e3e4  frame, then the geometry: 0x1000a700 hands back a
//                  pointer/count pair and 0x1000a740 rejects anything over
//                  128 vertices.
//   B  0x1004e436  copy each source position into aVerts and push it off
//                  the polygon plane by g_f1008ee84 along the plane
//                  normal (0x1004b630 -> operator* -> operator+=).
//   C  0x1004e4cd  clip the offset polygon to the frustum's own six
//                  planes with 0x10047cf0, and copy the result back into
//                  aVerts if the clipper moved it.
//   D  0x1004e559  per vertex: the shadow alpha from the distance to the
//                  frustum origin, and the three projective texture
//                  coordinates from rows 0, 1 and 3 of m_mProject.
//   E  0x1004e748  transform to camera space (0x1000d9b0) and read the
//                  clip mask back out of g_n1008b874.
//   F  0x1004e79c  THE SIX-PLANE CLIPPER, big6d910.cpp's, inlined.
//   G  0x1004f8e3  project to screen (0x1004b5c0), build the 32-byte FVF
//                  0x1c4 vertices, DrawPrimitive a triangle fan.
//
//  /Os /Oi- (build/engine.sh), RoundF on the thirty clip
//  conversions, the alpha's distance vector a named copy (vDist -- icl's
//  by-value temp at -0x15b4 copied to -0x15a8 before Mag()) and the body
//  inside `if (sub_1000a740(nVerts)) { ... }`: the __chkstk frame 0x28d8
//  is exact and the row pairs as a near (5995 bytes against 5887).
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  TYPES.  Every offset below is a load or a store in the shipped bytes
//  and is cited.
// ======================================================================= //

// 0x1004e2f0 -- d3dbatcha.cpp, `lea (%ecx,%eax,4)` then `ret $0x4`,
// 35 callers.  DECLARED AND NOT DEFINED on purpose: written as a field
// read it emits no call at all, and nine of this function's twenty-two
// calls are this accessor (a tiny accessor spelled as a field
// read makes no call).
struct Vec4e3e4
{
	// 0x10015640 -- d3dbatchc.cpp, operator-, 26 callers.  Hidden return
	// pointer at (%esp) and the 12-byte operand at 0x4(%esp): the shipped
	// site is `add $0xfffffff0,%esp ; ... rep movsl %ecx=3 ; call`.
	Vec4e3e4	sub_10015640(const Vec4e3e4 v) const;   // 0x10015640
	// 0x10015690 -- big48180.cpp, operator*(float).  `add $0xfffffff8,
	// %esp` = hidden return pointer plus one float.
	Vec4e3e4	sub_10015690(float f) const;            // 0x10015690
	// 0x1004b640 -- d3dbatchc.cpp, operator+=, 6 callers.  12 bytes of
	// operand, no hidden return.
	void		sub_1004b640(const Vec4e3e4 v);         // 0x1004b640
	// 0x10023330 -- d3dbatchd.cpp, Mag(), ltvector.h:39.
	float		sub_10023330() const;                   // 0x10023330
	// 0x1004e2f0 -- operator[].
	float&		sub_1004e2f0(uint32 nIndex);            // 0x1004e2f0

	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

union VColor4e3e4
{
	uint32	m_n;
	uint8	m_b[4];
};

// ---------------------------------------------------------------------- //
//  THE WORKING VERTEX, 0x24 bytes.  The stride is `add $0x24` at
//  0x1004e4b6, 0x1004e737, 0x1004e862, 0x1004ea49 and eleven more places,
//  and `mov $0x9 ; rep movsl` at 0x1004e871 copies it whole.  It is
//  big6d910.cpp's Vert6d910 without the second texture pair: this one
//  carries THREE interpolated floats, not four, which is why every clip
//  block below has three `flds/fsubs/fmull/fadds/fstps` groups where
//  big6d910's macro has four.
//
//  +0x0c is READ at 0x1004f949 and never written by this function; the
//  0x1004b5c0 loop at 0x1004f8fa is what fills it (`fsts 0xc(%eax)` at
//  0x1004b5e1, the 1.0f/z from 0x100a9f5c).
// ---------------------------------------------------------------------- //
struct Vert4e3e4                                        // 0x24 bytes
{
	Vec4e3e4	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	VColor4e3e4	m_Diffuse10;                            // +0x10
	VColor4e3e4	m_Specular14;                           // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
	float		m_fQ;                                   // +0x20
};

// ---------------------------------------------------------------------- //
//  THE D3D VERTEX, 0x20 bytes.  FVF 0x1c4 = D3DFVF_XYZRHW | DIFFUSE |
//  SPECULAR | TEX1 (the literal at 0x1004f9e6), and 0x20 is the stride
//  `lea 0x20(%edx),%eax` at 0x1004f99a.  Field for field, the assembly
//  loop at 0x1004f924 writes +0x00..+0x0b by `rep movsl %ecx=3`, then
//  +0x0c, +0x10, +0x14 as integer moves and +0x18, +0x1c as the two
//  perspective-divided texture coordinates.
// ---------------------------------------------------------------------- //
struct D3DVert4e3e4                                     // 0x20 bytes
{
	Vec4e3e4	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	VColor4e3e4	m_Diffuse;                              // +0x10
	VColor4e3e4	m_Specular;                             // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
};

// ---------------------------------------------------------------------- //
//  THE SOURCE VERTEX.  Stride 0x18 (`add $0x18,%ebx` at 0x1004e4bd) and
//  +0x00 is a POINTER: 0x1004e464 does `mov (%edx,%ebx,1),%esi` and then
//  `rep movsl %ecx=3` out of it.  Nothing else in the record is touched
//  here, so +0x04..+0x17 is named by its size only.  big6d910.cpp's
//  SrcVert6d910 is the same record with its four texture floats named.
// ---------------------------------------------------------------------- //
struct SrcVert4e3e4                                     // 0x18 bytes
{
	Vec4e3e4*	m_pPos00;                               // +0x00
	uint8		m_pad_04[0x14];                         // +0x04 .. +0x17
};

// ---------------------------------------------------------------------- //
//  0x1004b630 -- d3dbatchc.cpp, `mov 0x28(%ecx),%ecx ; mov 0x34(%ecx),
//  %eax`.  d3dbatchc.cpp types it `uint32`; big4b720.cpp names the two
//  hops ShadowItem::m_pPoly28->m_pPlane34 and reads the plane's normal at
//  +0x00 and its distance at +0x0c.  The result is used HERE as the
//  `this` of operator* (0x1004e48f `mov %eax,%ecx`), so what it returns
//  is the plane, i.e. the address of its normal.
// ---------------------------------------------------------------------- //
struct Item4e3e4
{
	Vec4e3e4*	sub_1004b630();                         // 0x1004b630

	uint8		m_pad_00[0x28];                         // +0x00 .. +0x27
	void*		m_pPoly28;                              // +0x28
};

// The plane, 0x10 bytes -- d3dr3a.cpp's SPlaneA.  Only the array base and
// the stride matter here: 0x1004e4e5 passes pFrustum+0xa0 and 0x10047cf0
// walks it with `add $0x10,%ebp`.
struct Plane4e3e4
{
	float	m_00, m_04, m_08, m_0c;
};

// ---------------------------------------------------------------------- //
//  THE FRUSTUM.  big4b720.cpp built this 0x100-byte block and handed its
//  address over; the four members this function touches are exactly the
//  four it reads:
//
//    +0x00  m_vPos      0x1004e59e `mov %ebx,%esi` then rep movsl 3
//    +0x60  m_mProject  rows 0, 1 and 3 at 0x1004e65c..0x1004e717
//    +0xa0  m_Planes    0x1004e4e5 `lea 0xa0(%ebx),%edx`, with the
//                       literal 6 at 0x1004e4eb
//
//  The rest is big4b720.cpp's naming, carried over unchanged so the two
//  files agree.
// ---------------------------------------------------------------------- //
struct Frustum4e3e4
{
	Vec4e3e4	m_vPos;                                 // +0x00
	Vec4e3e4	m_vRight;                               // +0x0c
	Vec4e3e4	m_vUp;                                  // +0x18
	Vec4e3e4	m_vDir;                                 // +0x24
	Plane4e3e4	m_Plane30;                              // +0x30
	Vec4e3e4	m_vLightPos;                            // +0x40
	Vec4e3e4	m_vCorner4c;                            // +0x4c
	float		m_fSize58;                              // +0x58
	float		m_fSize5c;                              // +0x5c
	float		m_mProject[4][4];                       // +0x60
	Plane4e3e4	m_Planes[6];                            // +0xa0 .. +0xff
};

// ---------------------------------------------------------------------- //
//  The device, as d3dr3a.cpp types it (0x1008d988 -- the device).  Slot
//  0x64 of the vtable is the only one called: `mov (%edx),%edx ;
//  call *0x64(%edx)` at 0x1004f9fa with six outgoing dwords, which is
//  IDirect3DDevice3::DrawPrimitive(this, type, fvf, verts, count, flags).
//  A COM method, so __stdcall: it pops its own six arguments, and the
//  shipped call (0x1004f9fc) is followed by no stack adjustment. Declared
//  without it, the caller popped them again: its saved registers came back
//  from the wrong slots, which crashed the shadow pass.
// ---------------------------------------------------------------------- //
struct D3DDevice4e3e4;

struct D3DDeviceVtbl4e3e4
{
	uint8	m_pad_00[0x64];                             // +0x00 .. +0x63
	int32	(__stdcall *DrawPrimitive)(D3DDevice4e3e4* pThis, uint32 nType,
	                         uint32 nFVF, void* pVerts, uint32 nCount,
	                         uint32 nFlags);            // +0x64
};

struct D3DDevice4e3e4
{
	D3DDeviceVtbl4e3e4*	m_pVtbl;                        // +0x00
};

// `this`.  NOT ONE FIELD OF IT IS TOUCHED: the caller loads %ecx at
// 0x1004dfad and the register entry never reads it (no spill of %ecx in
// the prologue, and %ecx is written before every read).  It is declared as
// big4b720.cpp declared it, and it stays a member so the call keeps its
// __thiscall shape.
class CShadowRenderer
{
public:
	void	sub_1004e3e4(Frustum4e3e4* pFrustum, Item4e3e4* pItem,
	                     float fRadius);                // 0x1004e3e4
};


// ======================================================================= //
//  GLOBALS, by address.  None of them has an initialiser in the image
//  (they all live in the uninitialised .data1/_DATA sections), so no value
//  is quoted for any of them.
// ======================================================================= //

// 0x1008b874 -- the clip mask.  Written 0x3f at 0x1004e75b and read back
// at 0x1004e77e/0x1004e786; 0x1000a770 (reached through 0x1000d9b0) is
// what clears bits in it.  Same global big6d910.cpp and big124a4.cpp use.
extern uint32			g_n1008b874;                    // 0x1008b874

// 0x1008b87c -- the clip scratch cursor.  d3dr3a.cpp declares it
// `SClipVertA* g_1008b87c`; 0x10047cf0 reads it into its own local at
// 0x10047d07 and this function reads it at 0x1004e79c.
extern Vert4e3e4*		g_p1008b87c;                    // 0x1008b87c

// 0x1008b880 -- the transform object handed to both transform helpers.
extern uint8			g_1008b880[];                   // 0x1008b880

// The near and far clip constants.  Compared at 0x1004e7ed / 0x1004f63f
// and stored, AS INTEGER MOVES, into the crossing vertex at 0x1004e8d7 /
// 0x1004f735.
extern float			g_f1008b908;                    // 0x1008b908
extern float			g_f1008b910;                    // 0x1008b910

// 0x1008be44 -- the clipped-polygon counter, `incl` once per plane pass at
// 0x1004e7c9, 0x1004ea9b, 0x1004ed7f, 0x1004f05b, 0x1004f337, 0x1004f61b.
extern uint32			g_n1008be44;                    // 0x1008be44

// 0x1008d7a0 -- UseD3DClip.  Tested at 0x1004e78c; big6d910.cpp types it
// `void*` and reads it in exactly the same place for exactly the same
// `nClip &= 1` narrowing.
extern void*			g_p1008d7a0;                    // 0x1008d7a0

// 0x1008d988 -- the device.
extern D3DDevice4e3e4*	g_p1008d988;                    // 0x1008d988

// 0x1008ee84 -- the distance the shadow polygon is lifted off its own
// plane.  Loaded as an integer at 0x1004e482 and pushed as the float
// argument of operator*.
extern float			g_f1008ee84;                    // 0x1008ee84

// 0x1008eea4 -- the shadow alpha scale, `fmuls` at 0x1004e5fe.
extern float			g_f1008eea4;                    // 0x1008eea4


// ======================================================================= //
//  NEIGHBOURS.  Each prototype is read off the call site in these bytes.
// ======================================================================= //

// 0x1000a700 -- d3dbatchd.cpp.  (geometry, &data, &count); the three
// outgoing slots at 0x1004e407..0x1004e40e are pItem, &pData, &nVerts.
// big572b4.cpp declares it identically.
extern "C" void  sub_1000a700(void* pGeom, SrcVert4e3e4** ppOutData,
                              int32* pnOutCount);

// 0x1000a740 -- d3dbatchd.cpp, the 128-vertex guard, `cmpl $0x80,arg ;
// jg`.  One dword pushed at 0x1004e420.
extern "C" int32 sub_1000a740(int32 nVerts);

// 0x10047cf0 -- d3dr3a.cpp, THE GENERAL-PLANE DRIVER.  Its own body reads
// arg1 into %edi and dereferences it as the vertex pointer, arg2 into
// %esi as the count, arg3 into %ebp as a 0x10-stride plane array and arg4
// into %ebx as the countdown -- so the four outgoing slots at
// 0x1004e4eb..0x1004e501 are (&pVerts, &nVerts, m_Planes, 6).
extern "C" int32 sub_10047cf0(Vert4e3e4** ppVerts, int32* pnVerts,
                              Plane4e3e4* pPlanes, int32 nPlanes);

// 0x1000d9b0 -- d3dlow.cpp, __cdecl (vertex, transform object).  Forwards
// to 0x1000a770 with `pXForm + 0x15c`.  big6d910.cpp declares it the same
// way and calls it in the same place for the same reason.
extern "C" void  sub_1000d9b0(Vert4e3e4* pVert, uint8* pXForm);

// 0x1004b5c0 -- d3dr2*.cpp's screen projection, __cdecl (vertex,
// transform object).  `flds 0x100a9f5c (1.0f) ; fdivs 0x8(%eax) ; ... ;
// fsts 0xc(%eax)` writes the vertex's +0x0c and then scales x, y and z
// through the object's +0x31c..+0x330.  This is 0x1000d940's twin --
// big6d910.cpp calls THAT one here and this function calls this one.
extern "C" void  sub_1004b5c0(Vert4e3e4* pVert, uint8* pXForm);

// 0x100805d0 -- the CRT memcpy.  d3dblit.cpp declares it this way.
extern void* memcpy(void* pDst, const void* pSrc, uint32 nBytes);


// The frame says fifty, exactly as it does in big6d910.cpp and in
// d3dr3a.cpp: each pass's flag array is 0xc8 = 200 bytes and the six
// arrays are 0xdc apart (-0x159c, -0x14c0, -0x13e4, -0x1308, -0x122c,
// -0x1150), the remaining 0x14 being the five `fistpl` destinations.
#define CLIP4E3E4_MAX	50


// ---------------------------------------------------------------------- //
//  THE PER-CROSSING INTERPOLATION, as a macro so that no call can be
//  emitted for it -- there is no `call` anywhere between 0x1004e783 and
//  0x1004f8fb.  THREE floats and FIVE bytes, in this order, read off
//  plane 1 at 0x1004eba0..0x1004ea3a and diffed against the other five:
//
//    +0x18  0x1004e8e0   +0x1c  0x1004e8ef   +0x20  0x1004e8fe
//    +0x12  0x1004e90d   +0x11  0x1004e949   +0x10  0x1004e985
//    +0x13  0x1004e9c1   +0x17  0x1004e9fd
//
//  The byte differences are taken in the INTEGER unit (`movzbl ; movzbl ;
//  sub` at 0x1004e90d) and only then converted, and the result goes out
//  through a named float slot and a bare `fistpl` (`fstps -0x28b8(%ebp) ;
//  flds -0x28b8(%ebp) ; fistpl -0x14d4(%ebp)` at 0x1004e928), which is
//  /QIfist -- no fnstcw/fldcw dance at any of the thirty sites.  The one
//  place in this function that DOES do the control-word dance is region D
//  (0x1004e609), and that one is written as a plain `(uint8)` cast below.
//  This is the same observation big6d910.cpp records for its own thirty
//  sites; under plain /O2 these thirty will differ in bytes and they
//  contain no calls.
//
//  CENSUS THAT PINS THE COUNTS: `movzbl` 60 = 6 planes x 5 bytes x 2
//  (current and previous) with 12 at each of +0x10, +0x11, +0x12, +0x13
//  and +0x17; `fildl` 60, the same 60; `fistpl` 30; `fmull` 61 = 6 x 10 +
//  1 (region D's 1.0f/fRadius); `fstps` 67 = 62 in the six passes + 3 in
//  region D + 2 in region G.  A fourth interpolated float or a sixth byte
//  would move at least four of those rows.
// ---------------------------------------------------------------------- //
// Round to nearest in the current FPU mode (w1_10001000.cpp's helper;
// 0x10015190 out of line): icl inlines a function holding __asm as
// `fstps f ; flds f ; fistpl n` -- the thirty bare `fistpl` sites.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

#define CLIP4E3E4_LERP                                                       \
	pOut->m_fU = (pWalk->m_fU - pPrev->m_fU) * fT + pPrev->m_fU;             \
	pOut->m_fV = (pWalk->m_fV - pPrev->m_fV) * fT + pPrev->m_fV;             \
	pOut->m_fQ = (pWalk->m_fQ - pPrev->m_fQ) * fT + pPrev->m_fQ;             \
                                                                             \
	fRed   = (pWalk->m_Diffuse10.m_b[2] - pPrev->m_Diffuse10.m_b[2]) * fT    \
	       + pPrev->m_Diffuse10.m_b[2];                                      \
	pOut->m_Diffuse10.m_b[2] = (uint8)RoundF(fRed);                                  \
	fGreen = (pWalk->m_Diffuse10.m_b[1] - pPrev->m_Diffuse10.m_b[1]) * fT    \
	       + pPrev->m_Diffuse10.m_b[1];                                      \
	pOut->m_Diffuse10.m_b[1] = (uint8)RoundF(fGreen);                                \
	fBlue  = (pWalk->m_Diffuse10.m_b[0] - pPrev->m_Diffuse10.m_b[0]) * fT    \
	       + pPrev->m_Diffuse10.m_b[0];                                      \
	pOut->m_Diffuse10.m_b[0] = (uint8)RoundF(fBlue);                                 \
	fAlpha = (pWalk->m_Diffuse10.m_b[3] - pPrev->m_Diffuse10.m_b[3]) * fT    \
	       + pPrev->m_Diffuse10.m_b[3];                                      \
	pOut->m_Diffuse10.m_b[3] = (uint8)RoundF(fAlpha);                                \
	fSpec  = (pWalk->m_Specular14.m_b[3] - pPrev->m_Specular14.m_b[3]) * fT  \
	       + pPrev->m_Specular14.m_b[3];                                     \
	pOut->m_Specular14.m_b[3] = (uint8)RoundF(fSpec);


// ---------------------------------------------------------------------- //
//  THE SIX CLIP PASSES.  Character for character big6d910.cpp's
//  CLIP6D910_HEAD/TAIL, because it is character for character the same
//  source; the only edits are the identifier prefix and the reject, which
//  is a plain `return` here because this function is void and its single
//  epilogue at 0x1004e42d has no `xor %eax,%eax`.
//
//  Each `if (nClip & BIT)` is a `test $BIT` on the SAME slot -0xc:
//    0x1004e7b9 test $0x1,%cl        0x1004ea8b test $0x4,%al
//    0x1004ed6f test $0x8,%al        0x1004f04b test $0x10,%al
//    0x1004f327 test $0x20,%al       0x1004f60b test $0x2,%al
//
//  `if (nInside == 0)` rejects the whole polygon: the shipped target is
//  0x1004f9ff, a trampoline whose whole body is `jmp 0x1004e42d`, the
//  epilogue.  The `cmp %eax,%edi ; je 0x1004f9ff` guard at the TOP of each
//  classify loop (0x1004e7e0) is the same reject -- icl rotated the loop
//  and folded "loop never ran therefore nInside is still zero", exactly as
//  d3dr3a.cpp records for 0x10002d40.
//
//  The classify loop is emitted pointer-walking (`add $0x24,%edx ;
//  cmp %edi,%edx` at 0x1004e7fa/0x1004e80e) and the edge loop
//  index-walking on the flag array and pointer-walking on the vertices
//  (`mov -0x159c(%ebp,%eax,4),%eax` at 0x1004e851, `add $0x24,%ebx` at
//  0x1004ea49); both are written that way below.
//
//  The tail's `pOut = pOut + nIn` DOUBLE-ADVANCES the cursor -- pOut is
//  already past what was just written and *nIn more* vertices are added
//  to it (`idiv $0x24` then `lea (%edx,%edx,8)` and `add %ebx,%edx` at
//  0x1004ea76..0x1004ea85).  d3dr3a.cpp records the same double advance in
//  the standalone members.  It is what the bytes say.
// ---------------------------------------------------------------------- //
#define CLIP4E3E4_HEAD(BIT, INSIDE)                                          \
	if (nClip & (BIT))                                                       \
	{                                                                        \
		int32       abInside[CLIP4E3E4_MAX];                                 \
		float       fRed, fGreen, fBlue, fAlpha, fSpec;                      \
		Vert4e3e4  *pEnd, *pWalk, *pPrev;                                    \
		int32      *pFlag;                                                   \
		int32       nInside, nFlag, i, j;                                    \
		float       fDen, fT, fVal;                                          \
                                                                             \
		g_n1008be44++;                                                       \
                                                                             \
		nInside = 0;                                                         \
		pFlag   = abInside;                                                  \
		pEnd    = pIn + nIn;                                                 \
                                                                             \
		for (pWalk = pIn; pWalk != pEnd; pWalk++)                            \
		{                                                                    \
			nFlag    = (INSIDE) ? 1 : 0;                                     \
			nInside += nFlag;                                                \
			*pFlag   = nFlag;                                                \
			pFlag++;                                                         \
		}                                                                    \
                                                                             \
		if (nInside == 0)                                                    \
			return;                                                          \
                                                                             \
		if (nInside != nIn)                                                  \
		{                                                                    \
			pOutStart = pOut;                                                \
                                                                             \
			j     = nIn - 1;                                                 \
			pPrev = pIn + (nIn - 1);                                         \
                                                                             \
			for (i = 0; i < nIn; i++)                                        \
			{                                                                \
				pWalk = &pIn[i];                                             \
                                                                             \
				if (abInside[j])                                             \
				{                                                            \
					*pOut = *pPrev;                                          \
					pOut++;                                                  \
				}                                                            \
                                                                             \
				if (abInside[j] != abInside[i])                              \
				{

#define CLIP4E3E4_TAIL                                                       \
					CLIP4E3E4_LERP                                           \
					pOut++;                                                  \
				}                                                            \
                                                                             \
				pPrev = pWalk;                                               \
				j     = i;                                                   \
			}                                                                \
                                                                             \
			pIn  = pOutStart;                                                \
			nIn  = (int32)(pOut - pOutStart);                                \
			pOut = pOut + nIn;                                               \
		}                                                                    \
	}


// ======================================================================= //
//  0x1004e3e0 (stack entry) / 0x1004e3e4 (register entry) .. 0x1004fae3
//  5,891 bytes.  One caller, 0x1004dfb3.
//
//  __thiscall with three stack arguments (`ret $0xc`, arguments read at
//  0x8(%ebp), 0xc(%ebp) and 0x10(%ebp)).  `this` is NEVER READ: the caller
//  loads %ecx at 0x1004dfad, the prologue does not spill it, and every
//  read of %ecx in the body follows a write.  It stays a member function
//  because that is what the call site's %ecx says it is, and because
//  big4b720.cpp already declared it one.
// ======================================================================= //
void CShadowRenderer::sub_1004e3e4(Frustum4e3e4* pFrustum, Item4e3e4* pItem,
                                   float fRadius)
{
	// -- The frame, in the shipped frame's order, lowest slot first.
	//    Everything here is dumped in the extent banner at the top.
	SrcVert4e3e4*	pData;              // -0x27cc
	int32			nVerts;             // -0x27c8
	Vert4e3e4		aVerts[128];        // -0x27c4 .. -0x15c5  (0x1200)
	Vert4e3e4*		pVerts;             // -0x15c4

	Vert4e3e4*		pIn;                // -0x10
	Vert4e3e4*		pOut;               // -0x58
	Vert4e3e4*		pOutStart;          // -0x18, and -0xc in the last pass
	int32			nIn;                // -0x48 (which -0x48 shares with
	                                    //  the reciprocal below -- the two
	                                    //  live ranges do not overlap)
	uint32			nFlags;             // -0xc
	uint32			nClip;              // -0xc
	int32			i;

	D3DVert4e3e4	aOut[128];          // -0x1074 .. -0x75   (0x1000)

	// The thirty 8-byte-stride float slots at -0x28b8..-0x27d0 are NOT a
	// source array: they are where icl put the six passes'
	// {fRed,fGreen,fBlue,fAlpha,fSpec}, which the passes below declare by
	// name inside their own blocks.  Nothing is declared for them here.

	// ================================================================== //
	//  REGION A -- 0x1004e3e4 .. 0x1004e433.  The geometry, and the only
	//  `ret` in the function.
	//
	//  `mov $0x28d8,%eax ; call 0x100805a0` is the stack probe on the
	//  10,456-byte frame; it is a benign shipped-only row
	//  and is not to be chased with /Ge.
	// ================================================================== //
	sub_1000a700(pItem, &pData, &nVerts);

	if (sub_1000a740(nVerts))
	{

		// ================================================================== //
		//  REGION B -- 0x1004e436 .. 0x1004e4cb.  Copy the polygon's own
		//  positions into aVerts and lift each one off the polygon plane.
		//
		//  0x1004e474 `mov -0x5c(%ebp),%ecx ; call 0x1004b630` -- the two-hop
		//  accessor -- then 0x1004e491 operator*(g_f1008ee84) into the hidden
		//  return slot -0x15c0, then 0x1004e4ab operator+= with the twelve
		//  bytes copied to the outgoing area by `rep movsl %ecx=3`.  The
		//  accessor is called INSIDE the loop, once per vertex, not hoisted.
		//
		//  The induction variables are the loop counter at -0x58, a 0x24-stride
		//  byte offset at -0x54 into aVerts and a 0x18-stride byte offset in
		//  %ebx into pData -- i.e. two indexed walks, written indexed.
		// ================================================================== //
		for (i = 0; i < nVerts; i++)
		{
			aVerts[i].m_vPos = *pData[i].m_pPos00;
			aVerts[i].m_vPos.sub_1004b640(
				pItem->sub_1004b630()->sub_10015690(g_f1008ee84));
		}

		// ================================================================== //
		//  REGION C -- 0x1004e4cd .. 0x1004e556.  Clip the lifted polygon to
		//  the frustum's own six planes.
		//
		//  0x1004e4eb sets the fourth outgoing slot to the literal 6 and
		//  0x1004e4e5 passes `pFrustum + 0xa0`; on failure (`test %eax,%eax ;
		//  je 0x1004fa04`) 0x1004fa04 is a trampoline into the epilogue.
		//
		//  0x1004e514: the clipper may have left the result in ITS OWN
		//  scratch, so the pointer is compared against aVerts and the result
		//  copied back when it differs.  `add %eax,%eax ; add %eax,%eax ;
		//  lea (%eax,%eax,8),%eax` at 0x1004e53a is nVerts * 36.
		// ================================================================== //
		pVerts = aVerts;

		if (!sub_10047cf0(&pVerts, &nVerts, pFrustum->m_Planes, 6))
			return;

		if (pVerts != aVerts)
		{
			memcpy(aVerts, pVerts, nVerts * sizeof(Vert4e3e4));
			pVerts = aVerts;
		}

		// ================================================================== //
		//  REGION D -- 0x1004e559 .. 0x1004e745.  Per vertex: the shadow
		//  alpha, and the three projective texture coordinates.
		//
		//  0x1004e56c hoists `1.0f / fRadius` out of the loop into the 8-byte
		//  slot at -0x48 -- an x87 spill of the loop invariant, so the source
		//  names it once outside the loop.  It is spelled `float` here; the
		//  slot is 8 bytes because that is how icl spills the x87 stack, and
		//  the same thing happens to fT in all six clip passes below (13
		//  `fstpl` in the function: 6 inline fT, 6 sunk fT, and this one).
		//
		//  The alpha: 0x1004e5ad `operator-`, 0x1004e5cb `Mag()` on a copy of
		//  its result (icl's by-value temp at -0x15a8), then
		//  `1.0f - fDist * fOORadius`, clamped at zero (`flds 0x100aa260 ;
		//  fcomp ; jbe` -- the replacement is the FALL-THROUGH arm and the
		//  keep path jumps over it, so the source guard is `< 0.0f`), scaled
		//  by g_f1008eea4 and truncated by the ONE control-word dance in this
		//  function (0x1004e609..0x1004e624, `or $0xc00` = round-to-zero).
		//
		//  The three texture coordinates come from rows 0, 1 and 3 of
		//  m_mProject -- +0x60..+0x6c, +0x70..+0x7c and +0x90..+0x9c.  ROW 2
		//  IS NEVER READ.  The position components arrive through NINE calls
		//  to operator[] (0x1004e2f0), three per row, each row asking for 0,
		//  then 1, then 2; that is nine of the function's twenty-two calls and
		//  writing `m_vPos.x` instead would emit none of them.
		//
		//  The x87 schedule multiplies out in the order z, y, x and only then
		//  adds the translate; icl reorders float sums freely (the same thing
		//  happens in this function's own caller at 0x1004df26, where a source
		//  x + y + z comes out x, z, y), so the source order below is the
		//  operator[] call order, which icl may NOT reorder.
		// ================================================================== //
		{
			float		fOORadius;
			float		fAlpha;
			Vert4e3e4*	pVert;

			fOORadius = 1.0f / fRadius;

			for (i = 0; i < nVerts; i++)
			{
				pVert = &pVerts[i];

				Vec4e3e4 vDist = pVert->m_vPos.sub_10015640(pFrustum->m_vPos);
				fAlpha = 1.0f - vDist.sub_10023330() * fOORadius;

				if (fAlpha < 0.0f)
					fAlpha = 0.0f;

				pVert->m_Diffuse10.m_n   = 0xffffffff;
				pVert->m_Diffuse10.m_b[3] = (uint8)(fAlpha * g_f1008eea4);

				pVert->m_fU = pFrustum->m_mProject[0][0]
				                  * pVert->m_vPos.sub_1004e2f0(0)
				            + pFrustum->m_mProject[0][1]
				                  * pVert->m_vPos.sub_1004e2f0(1)
				            + pFrustum->m_mProject[0][2]
				                  * pVert->m_vPos.sub_1004e2f0(2)
				            + pFrustum->m_mProject[0][3];

				pVert->m_fV = pFrustum->m_mProject[1][0]
				                  * pVert->m_vPos.sub_1004e2f0(0)
				            + pFrustum->m_mProject[1][1]
				                  * pVert->m_vPos.sub_1004e2f0(1)
				            + pFrustum->m_mProject[1][2]
				                  * pVert->m_vPos.sub_1004e2f0(2)
				            + pFrustum->m_mProject[1][3];

				pVert->m_fQ = pFrustum->m_mProject[3][0]
				                  * pVert->m_vPos.sub_1004e2f0(0)
				            + pFrustum->m_mProject[3][1]
				                  * pVert->m_vPos.sub_1004e2f0(1)
				            + pFrustum->m_mProject[3][2]
				                  * pVert->m_vPos.sub_1004e2f0(2)
				            + pFrustum->m_mProject[3][3];

				// 0x1004e72d, AFTER the +0x20 store.  A dword zero.
				pVert->m_Specular14.m_n = 0;
			}
		}

		// ================================================================== //
		//  REGION E -- 0x1004e748 .. 0x1004e799.  Camera space, and the clip
		//  mask.
		//
		//  0x1004e75b writes the literal 0x3f into g_n1008b874 and 0x1004e77e
		//  reads it back after the loop; icl kept the 0x3f in %eax so the
		//  zero-trip path skips the reload.  The loop is the rotated
		//  post-decrement shape (`test %edx,%edx ; je` then `dec %esi ; jne`),
		//  which big6d910.cpp writes as `while (nLeft--)` in exactly this
		//  place for exactly this call.
		//
		//  0x1004e786: UseD3DClip.  When it is set only the near plane is
		//  done, and if that bit is clear the whole clipper AND the
		//  `pVerts = pIn ; nVerts = nIn` epilogue at 0x1004f8d7 are skipped by
		//  the jump to 0x1004f8e3.
		// ================================================================== //
		{
			Vert4e3e4*	pWalk;
			int32		nLeft;

			g_n1008b874 = 0x3f;

			pWalk = pVerts;
			nLeft = nVerts;

			while (nLeft--)
			{
				sub_1000d9b0(pWalk, g_1008b880);
				pWalk++;
			}

			nFlags = g_n1008b874;
		}

		nClip = nFlags;

		if (g_p1008d7a0)
		{
			nClip &= 1;

			if (!nClip)
				goto L_Project;
		}

		// ================================================================== //
		//  REGION F -- 0x1004e79c .. 0x1004f8e0.  THE SIX-PLANE CLIPPER.
		//  4,423 call-free bytes; see the banner at the top of this file for
		//  what pins them.
		//
		//  0x1004e79c/0x1004e7aa/0x1004e7b6/0x1004e7bc: all four assignments
		//  are in the shipped bytes, pOut and pOutStart both from
		//  g_p1008b87c.
		// ================================================================== //
		pOut      = g_p1008b87c;
		pOutStart = g_p1008b87c;
		pIn       = pVerts;
		nIn       = nVerts;

		// ------------------------------------------------------------------ //
		//  0x1004e7c5 -- bit 0x01, NEAR.  z >= g_f1008b908.
		//  `flds 0x8(%edx) ; fcomps 0x1008b908` / jae at 0x1004e7ea.
		//  Epsilons 0x100aa270 (-1e-5) and 0x100aa2c8 / 0x100aa2cc
		//  (+1e-5 / 0.0f, in the sunk arm at 0x1004fa09) -- all three dumped.
		// ------------------------------------------------------------------ //
		CLIP4E3E4_HEAD(0x01, pWalk->m_vPos.z >= g_f1008b908)
			fDen = pWalk->m_vPos.z - pPrev->m_vPos.z;

			if (fDen < -0.00001f || fDen > 0.00001f)
				fT = (g_f1008b908 - pPrev->m_vPos.z) / fDen;
			else
				fT = 0.0f;

			pOut->m_vPos.x = (pWalk->m_vPos.x - pPrev->m_vPos.x) * fT
			               + pPrev->m_vPos.x;
			pOut->m_vPos.y = (pWalk->m_vPos.y - pPrev->m_vPos.y) * fT
			               + pPrev->m_vPos.y;
			// 0x1004e8d7 -- `mov 0x1008b908,%edx ; mov %edx,0x8(%ecx)`, an
			// INTEGER move, so it is a plain float lvalue copy of the global.
			pOut->m_vPos.z = g_f1008b908;
			(void)fVal;
		CLIP4E3E4_TAIL

		// ------------------------------------------------------------------ //
		//  0x1004ea93 -- bit 0x04, LEFT.  -z < x.
		//  `flds 0x8(%edx) ; fchs ; fcomps (%edx)` / jb at 0x1004eabc.
		//  Epsilons 0x100aa278 and 0x100aa2c0 / 0x100aa2c4 (sunk arm
		//  0x1004fa2c).
		// ------------------------------------------------------------------ //
		CLIP4E3E4_HEAD(0x04, -pWalk->m_vPos.z < pWalk->m_vPos.x)
			fDen = (pWalk->m_vPos.x - pPrev->m_vPos.x)
			     + pWalk->m_vPos.z - pPrev->m_vPos.z;

			if (fDen < -0.00001f || fDen > 0.00001f)
				fT = -(pPrev->m_vPos.x + pPrev->m_vPos.z) / fDen;
			else
				fT = 0.0f;

			pOut->m_vPos.y = (pWalk->m_vPos.y - pPrev->m_vPos.y) * fT
			               + pPrev->m_vPos.y;
			fVal = (pWalk->m_vPos.z - pPrev->m_vPos.z) * fT + pPrev->m_vPos.z;
			// 0x1004ebb9 `fld %st(0) ; fchs ; fxch %st(1) ; fstps 0x8(%ecx) ;
			// fstps (%ecx)` -- z first, then x, and the duplicate-and-negate
			// is only emitted for two DIFFERENT values, so the source names
			// the interpolated z and stores it twice (d3dr3a.cpp records the
			// same five instructions for 0x100030a0).
			pOut->m_vPos.z = fVal;
			pOut->m_vPos.x = -fVal;
		CLIP4E3E4_TAIL

		// ------------------------------------------------------------------ //
		//  0x1004ed77 -- bit 0x08, TOP.  y < z.
		//  `flds 0x4(%edx) ; fcomps 0x8(%edx)` / jb at 0x1004eda0.  Epsilons
		//  0x100aa280 and 0x100aa2b8 / 0x100aa2bc (sunk arm 0x1004fa51).
		// ------------------------------------------------------------------ //
		CLIP4E3E4_HEAD(0x08, pWalk->m_vPos.y < pWalk->m_vPos.z)
			fDen = (pWalk->m_vPos.y - pPrev->m_vPos.y)
			     - pWalk->m_vPos.z + pPrev->m_vPos.z;

			if (fDen < -0.00001f || fDen > 0.00001f)
				fT = -(pPrev->m_vPos.y - pPrev->m_vPos.z) / fDen;
			else
				fT = 0.0f;

			pOut->m_vPos.x = (pWalk->m_vPos.x - pPrev->m_vPos.x) * fT
			               + pPrev->m_vPos.x;
			fVal = (pWalk->m_vPos.z - pPrev->m_vPos.z) * fT + pPrev->m_vPos.z;
			// 0x1004ee9a: `fsts 0x4(%ecx)` then `fstps 0x8(%ecx)` -- y then z.
			pOut->m_vPos.y = fVal;
			pOut->m_vPos.z = fVal;
		CLIP4E3E4_TAIL

		// ------------------------------------------------------------------ //
		//  0x1004f053 -- bit 0x10, RIGHT.  x < z.
		//  `flds (%edx) ; fcomps 0x8(%edx)` / jb at 0x1004f07c.  Epsilons
		//  0x100aa288 and 0x100aa2b0 / 0x100aa2b4 (sunk arm 0x1004fa76).
		// ------------------------------------------------------------------ //
		CLIP4E3E4_HEAD(0x10, pWalk->m_vPos.x < pWalk->m_vPos.z)
			fDen = (pWalk->m_vPos.x - pPrev->m_vPos.x)
			     - pWalk->m_vPos.z + pPrev->m_vPos.z;

			if (fDen < -0.00001f || fDen > 0.00001f)
				fT = -(pPrev->m_vPos.x - pPrev->m_vPos.z) / fDen;
			else
				fT = 0.0f;

			pOut->m_vPos.y = (pWalk->m_vPos.y - pPrev->m_vPos.y) * fT
			               + pPrev->m_vPos.y;
			fVal = (pWalk->m_vPos.z - pPrev->m_vPos.z) * fT + pPrev->m_vPos.z;
			// 0x1004f177: `fsts (%ecx)` then `fstps 0x8(%ecx)` -- x then z.
			pOut->m_vPos.x = fVal;
			pOut->m_vPos.z = fVal;
		CLIP4E3E4_TAIL

		// ------------------------------------------------------------------ //
		//  0x1004f32f -- bit 0x20, BOTTOM.  -z < y.
		//  `flds 0x8(%edx) ; fchs ; fcomps 0x4(%edx)` / jb at 0x1004f358.
		//  Epsilons 0x100aa290 and 0x100aa2a8 / 0x100aa2ac (sunk arm
		//  0x1004fa9b).
		// ------------------------------------------------------------------ //
		CLIP4E3E4_HEAD(0x20, -pWalk->m_vPos.z < pWalk->m_vPos.y)
			fDen = (pWalk->m_vPos.y - pPrev->m_vPos.y)
			     + pWalk->m_vPos.z - pPrev->m_vPos.z;

			if (fDen < -0.00001f || fDen > 0.00001f)
				fT = -(pPrev->m_vPos.y + pPrev->m_vPos.z) / fDen;
			else
				fT = 0.0f;

			pOut->m_vPos.x = (pWalk->m_vPos.x - pPrev->m_vPos.x) * fT
			               + pPrev->m_vPos.x;
			fVal = (pWalk->m_vPos.z - pPrev->m_vPos.z) * fT + pPrev->m_vPos.z;
			// 0x1004f454 `fld %st(0) ; fchs ; fxch %st(1)` -- z first, then y.
			pOut->m_vPos.z = fVal;
			pOut->m_vPos.y = -fVal;
		CLIP4E3E4_TAIL

		// ------------------------------------------------------------------ //
		//  0x1004f613 -- bit 0x02, FAR.  z <= g_f1008b910.  LAST, as it is in
		//  big6d910.cpp, big70d10.cpp and d3dr3a.cpp's family.
		//  `flds 0x8(%edx) ; fcomps 0x1008b910` / jbe at 0x1004f63c.  Epsilons
		//  0x100aa298 and 0x100aa2a0 / 0x100aa2a4 (sunk arm 0x1004fac0).
		// ------------------------------------------------------------------ //
		CLIP4E3E4_HEAD(0x02, pWalk->m_vPos.z <= g_f1008b910)
			fDen = pWalk->m_vPos.z - pPrev->m_vPos.z;

			if (fDen < -0.00001f || fDen > 0.00001f)
				fT = (g_f1008b910 - pPrev->m_vPos.z) / fDen;
			else
				fT = 0.0f;

			pOut->m_vPos.x = (pWalk->m_vPos.x - pPrev->m_vPos.x) * fT
			               + pPrev->m_vPos.x;
			pOut->m_vPos.y = (pWalk->m_vPos.y - pPrev->m_vPos.y) * fT
			               + pPrev->m_vPos.y;
			// 0x1004f735 -- integer move again.
			pOut->m_vPos.z = g_f1008b910;
			(void)fVal;
		CLIP4E3E4_TAIL

		// -- 0x1004f8bb: the clipper's own epilogue, skipped by the UseD3DClip
		//    early exit above.  `sub %edx,%eax ; cltd ; idiv $0x24` is the
		//    sixth of the six divisions by the 36-byte vertex.
		pVerts = pIn;
		nVerts = nIn;

	L_Project:
		// ================================================================== //
		//  REGION G -- 0x1004f8e3 .. 0x1004fa08.  Screen space, the D3D
		//  vertices, and the draw.
		//
		//  The 0x1004b5c0 loop is the same rotated post-decrement as region
		//  E's; it writes each vertex's +0x0c (the RHW, 1.0f/z through the
		//  1.0f at 0x100a9f5c) and scales x, y and z, which is the only reason
		//  +0x0c is defined by the time it is copied out below.
		//
		//  The assembly loop at 0x1004f924 walks aOut with a 0x20 byte offset
		//  at -0x20 and pVerts with a 0x24 byte offset in %ebx: twelve bytes
		//  of position by `rep movsl %ecx=3`, then three integer moves out of
		//  +0x0c, +0x10 and +0x14, then the two texture coordinates divided by
		//  +0x20 (`flds 0x18(%eax,%ebx,1) ; fdivs 0x20(%eax,%ebx,1)`).  +0x20
		//  is loaded twice, once per divide -- two `fdivs`, not one reciprocal.
		//
		//  0x1004f9fa `mov (%edx),%edx ; call *0x64(%edx)` with six outgoing
		//  dwords: the device, 6 (D3DPT_TRIANGLEFAN), 0x1c4 (XYZRHW | DIFFUSE
		//  | SPECULAR | TEX1), aOut, the count, and 0.  This is the function's
		//  only indirect call and the last of its twenty-two.
		// ================================================================== //
		{
			Vert4e3e4*	pWalk;
			int32		nLeft;

			pWalk = pVerts;
			nLeft = nVerts;

			while (nLeft--)
			{
				sub_1004b5c0(pWalk, g_1008b880);
				pWalk++;
			}
		}

		for (i = 0; i < nVerts; i++)
		{
			aOut[i].m_vPos     = pVerts[i].m_vPos;
			aOut[i].m_fRHW     = pVerts[i].m_fRHW;
			aOut[i].m_Diffuse  = pVerts[i].m_Diffuse10;
			aOut[i].m_Specular = pVerts[i].m_Specular14;
			aOut[i].m_fU       = pVerts[i].m_fU / pVerts[i].m_fQ;
			aOut[i].m_fV       = pVerts[i].m_fV / pVerts[i].m_fQ;
		}

		g_p1008d988->m_pVtbl->DrawPrimitive(g_p1008d988, 6, 0x1c4, aOut,
		                                    nVerts, 0);
	}
}


// *********************************************************************** //
//
//  WHAT THIS DECODE CLAIMS, AND WHAT IT DOES NOT
//
// *********************************************************************** //
//
//  ------------------------------------------------------------------- //
//  THE CALL SEQUENCE.  Predicted before any build.
//  ------------------------------------------------------------------- //
//
//  Over 0x1004e3e0..0x1004fae3, shipped against sub_1004e3e4 in our
//  big4e3e4.obj.
//
//  The shipped sequence, in order and with the address of each site:
//
//      0x1004e3ec  0x100805a0  _chkstk         <- shipped only, expected
//      0x1004e412  0x1000a700
//      0x1004e421  0x1000a740
//      0x1004e474  0x1004b630   \
//      0x1004e491  0x10015690    >  region B, one loop body
//      0x1004e4ab  0x1004b640   /
//      0x1004e504  0x10047cf0
//      0x1004e545  0x100805d0  memcpy
//      0x1004e5ad  0x10015640   \
//      0x1004e5cb  0x10023330    |
//      0x1004e633  0x1004e2f0    |
//      0x1004e643  0x1004e2f0    |
//      0x1004e655  0x1004e2f0    |
//      0x1004e683  0x1004e2f0    >  region D, one loop body
//      0x1004e694  0x1004e2f0    |
//      0x1004e6a7  0x1004e2f0    |
//      0x1004e6d5  0x1004e2f0    |
//      0x1004e6e6  0x1004e2f0    |
//      0x1004e6f6  0x1004e2f0   /
//      0x1004e770  0x1000d9b0
//      0x1004f8fc  0x1004b5c0
//      0x1004f9fc  *0x64(%edx)  indirect
//
//  The source above issues those twenty-one direct calls in exactly that
//  order and one indirect in that position, so the expected figure is
//
//      shipped calls 22   ours 21
//      in agreement  21
//      shipped-only   1   ours-only 0
//
//  and the single shipped-only row is `_chkstk` at 0x1004e3ec, the
//  known benign residual.  DO NOT add /Ge to close it.
//
//  TWO THINGS THAT COULD MAKE THE FIGURE WORSE, both recognisable:
//
//    * memcpy.  If icl expands 0x1004e545's `memcpy(aVerts, pVerts,
//      nVerts * 36)` inline (`rep movsb`) instead of calling, that is one
//      more shipped-only row at 0x1004e545 and it is the same class of gap
//      as the CRT intrinsics (sqrt, fabs, tan, atan, memcpy).  The size is a variable, so a
//      call is the likely outcome.
//    * the nine operator[] calls.  They are the reason `sub_1004e2f0` is
//      DECLARED AND NOT DEFINED in this file.  If it ever acquires a body
//      here, nine of the twenty-one calls vanish at a stroke and the
//      figure collapses to 12 of 22 -- exactly the failure seen
//      for 0x1005dca0.  The same is true of `sub_1004b630`,
//      `sub_10015640`, `sub_10015690`, `sub_1004b640` and `sub_10023330`.
//
//  ------------------------------------------------------------------- //
//  HOW MUCH OF THE FUNCTION THE CALL CHECK ACTUALLY WITNESSES
//  ------------------------------------------------------------------- //
//
//  About a quarter of it, and the quarter is at the two ends:
//
//      0x1004e3e0 .. 0x1004e782    931 bytes, 20 of the 22 calls
//      0x1004e783 .. 0x1004f8fb  4,473 bytes, NOT ONE CALL  (76%)
//      0x1004f8fc .. 0x1004fae2    487 bytes, the last 2 calls
//
//  Everything the call check sees is regions A, B, C, D, E and G.  All of
//  region F -- the six-plane clipper, which is the bulk of the function --
//  is invisible to it, and so are the six sunk epsilon arms.  What carries
//  region F instead, in decreasing order of strength:
//
//    1. THE INSTRUCTION CENSUS, WITH NO RESIDUE.  Every mnemonic in the
//       function's 1,750 instructions is accounted for by the structure
//       written above; these are the load-bearing rows and each one was
//       decomposed by operand, not quoted as a total:
//
//         movzbl  60 = 6 planes x 5 bytes x 2, and BY OFFSET it is
//                      12 x +0x10, 12 x +0x11, 12 x +0x12, 12 x +0x13,
//                      12 x +0x17 -- five bytes, not four and not six
//         fildl   60 = the same sixty
//         fistpl  30 = 6 x 5;  fistpll 1 = region D's one (int) cast
//         fmull   61 = 6 x (2 coordinate lerps + 3 float lerps + 5 byte
//                      lerps) + 1 (region D's hoisted 1.0f/fRadius)
//         fstps   67 = 62 in the six passes (10,11,10,10,11,10 -- the two
//                      elevens are planes 2 and 5, whose `-fVal` needs a
//                      third store) + 3 (region D) + 2 (region G)
//         flds   114 = 88 inline in the six passes (6 classify + 12
//                      denominator + 6 epsilon + 4 extra numerator loads
//                      + 12 coordinate + 18 float lerps + 30 float-temp
//                      reloads) + 12 in the six sunk arms + 12 in regions
//                      C/D (3 constants + 3 rows x 3) + 2 in region G
//         fadds   37 = 12 coordinate + 18 float lerps + 4 denominator
//                      + 3 (the three m_mProject translates)
//         fsubs   34 = 30 lerps + 4 denominator
//         faddp   38 = 30 byte lerps + 2 numerators (planes 2 and 5)
//                      + 6 (two per m_mProject row)
//         fmuls   10 = 9 (three per m_mProject row) + 1 (g_f1008eea4)
//         fcomp   13 = 6 inline epsilons + 6 sunk epsilons + 1 (the
//                      `fAlpha < 0.0f` clamp)
//         fcomps   6 = the six classify tests
//         fnstsw/sahf 19 each = 13 + 6, the same nineteen comparisons
//         fstp    17 = 16 stack cleanups in the six sunk arms (2,3,3,3,
//                      3,2) + 1 (the discarded fAlpha)
//         fstpl   13 = 6 inline fT + 6 sunk fT + 1 (1.0f/fRadius)
//         fchs     8 = 2 classify negations (planes 2 and 5) + 4
//                      numerator negations (planes 2, 3, 4, 5) + 2
//                      duplicate-and-negate stores (planes 2 and 5)
//         fld      2 = the two duplicate-and-negate stores
//         fsts     2 = planes 3 and 4, which store the SAME value twice
//         fsubrs   3 = planes 1 and 6 numerators + `1.0f - t`
//         fsubrp   2 = planes 3 and 4 numerators
//         fdivp    6 = the six fT divisions
//         fdivs    3 = 1.0f/fRadius + region G's u/q and v/q
//         fnstcw   1, fldcw 2 = the ONE control-word dance, region D
//         idiv     6, cltd 6 = the six `(pOut - pOutStart) / 0x24`
//         incl     6 = g_n1008be44, once per pass
//         rep movsl 11 = 1 (region B position copy) + 1 (region B
//                      operator+= argument) + 2 (region D's operator-
//                      argument and its Mag() copy) + 6 (the six passes'
//                      whole-vertex copies) + 1 (region G)
//         lea     58, and this is the row that looks like noise and is
//                      not.  By displacement: 12 `lea (%eax,%eax,8)` and
//                      11 `lea (%edx,%edx,8)` (the x36 and x9 halves of
//                      the vertex-stride multiplies), 5 `lea -0x4(,%eax,
//                      4)` (five of the six `(nIn-1)*36`; pass 1 spells
//                      it `mov $-1 ; add` instead at 0x1004e828), 5
//                      aVerts, 6 flag-array bases -- ONE PER PASS, which
//                      is how the six 0xdc blocks are counted -- 2 aOut,
//                      5 aOut field addresses, 2 each for the three
//                      12-byte vector temps and for &nVerts, and 4
//                      singletons (0xa0(%ebx) = m_Planes, 0x20(%edx) =
//                      the aOut stride, &pData, &pVerts).  12+11+5+5+6+2
//                      +5+2+2+2+2+4 = 58, with nothing left over
//
//       A seventh plane, a fifth interpolated float, a sixth interpolated
//       byte, a missing `-fVal` store or a different flag-array bound
//       would each move at least three of those rows.
//
//    2. THE FRAME, DECOMPOSED EXACTLY.  See the banner at the top: 0x28d8
//       = 0x20 alignment + 240 (30 float temps) + 8 (pData, nVerts) +
//       0x1200 (128 x 36) + 4 (pVerts) + 36 (three vector temps) + 6 x
//       0xdc (the passes) + 0x1000 (128 x 32) + 0x74 of scalars.  BOTH
//       array bounds are read off the frame: 128 from 0x1200/36 and from
//       0x1000/32, 50 from 0xdc - 0x14.
//
//    3. THE SIBLING DIFF.  big6d910.cpp's CLIP6D910_* macro, expression by
//       expression, with the epsilon addresses, the plane constants and
//       the store orders dumped on both sides; and d3dr3a.cpp's two
//       standalone family members for the two planes it carries.
//
//    4. THE LITERAL POOL, 0x100aa258..0x100aa2cc, read in order.  See the
//       banner.
//
//  ------------------------------------------------------------------- //
//  UNDECODED
//  ------------------------------------------------------------------- //
//
//  NONE.  Every byte of 0x1004e3e0..0x1004fae2 is covered by a region
//  above: A 0x1004e3e4-0x1004e435, B 0x1004e436-0x1004e4cc,
//  C 0x1004e4cd-0x1004e558, D 0x1004e559-0x1004e747,
//  E 0x1004e748-0x1004e79b, F 0x1004e79c-0x1004f8e2,
//  G 0x1004f8e3-0x1004f9fe, the two reject trampolines
//  0x1004f9ff-0x1004fa08 and the six sunk epsilon arms
//  0x1004fa09-0x1004fae2, plus the /Qip stack entry 0x1004e3e0-0x1004e3e3.
//
//  ------------------------------------------------------------------- //
//  WHAT COULD NOT BE PINNED
//  ------------------------------------------------------------------- //
//
//  1. EIGHT DEAD 1.0f LITERALS.  0x100aa268 and 0x100aa26c (region D's),
//     and 0x100aa274 / 0x27c / 0x284 / 0x28c / 0x294 / 0x29c (one per clip
//     pass).  Nothing in the image loads any of them -- a grep of the
//     whole 0x10001000..0x10085000 disassembly finds zero references --
//     and no statement written above accounts for them.  SIX OF THE EIGHT
//     ARE NOT THIS FUNCTION'S PROBLEM: the standalone family member
//     0x10002d40 has exactly the same dead literal in exactly the same
//     place (0x100a4084/0x100a4094/0x100a40a4/0x100a40b4, each sitting
//     between its pass's -1e-5 and its +1e-5), and d3dr3a.cpp's decode of
//     it does not account for them either.  So the clip passes' six are a
//     property of the family's original source that three independent
//     decodes have now failed to recover.  The two at 0x100aa268/0x26c
//     belong to region D, between the `0.0f` clamp and the first pass's
//     epsilon, and are equally unexplained.  Guessing a statement to
//     produce them would be worse than saying so.
//
//  2. `this`.  The caller sets %ecx (0x1004dfad) and the body never reads
//     it.  Nothing says which class member this belongs to beyond
//     big4b720.cpp's naming, and nothing in these bytes could.
//
//  3. THE /Qip STACK ENTRY'S DEAD REGISTER COPY.  0x1004e3e0's
//     `mov 0x4(%esp),%eax` puts the first argument in %eax and the
//     register entry immediately clobbers %eax and re-reads the argument
//     from 0x8(%ebp).  Both entries take the same three stack arguments
//     and both `ret $0xc`; what the register copy was FOR cannot be read
//     off these four bytes.  It is decoded as the /Qip pair because that
//     is the documented /Qip shape, it is 16-byte aligned behind eight
//     pads, and the only caller is same-TU and enters at +4.
//
//  4. THE STORAGE CLASS OF fT AND fOORadius.  Both are spilled to 8-byte
//     slots (`fstpl` / `fmull`), which is how icl spills the x87 stack;
//     the sibling 0x10002d40 keeps the same value in a register and
//     d3dr3a.cpp calls it `float`.  `float` is written here for that
//     reason.  `double` is the alternative and it costs no calls either
//     way, so one build settles it and neither spelling can be wrong
//     about the structure.
//
//  5. THE VALUES OF THE GLOBALS.  g_f1008b908, g_f1008b910, g_f1008ee84
//     and g_f1008eea4 all live in the image's uninitialised data, so no
//     constant can be dumped for any of them; they are named by address
//     only.  Likewise the contents of g_1008b880 beyond "the object both
//     transform helpers index" (+0x15c for 0x1000d9b0's 3x4, +0x31c..
//     +0x330 for 0x1004b5c0's projection).
//
//  ------------------------------------------------------------------- //
//  GENUINE COIN-FLIPS, both spellings written out so one build settles
//  each.  None of them moves the call sequence.
//  ------------------------------------------------------------------- //
//
//  A. THE EPSILON TEST.  Written, following d3dr3a.cpp and big6d910.cpp,
//     as the short-circuit
//
//         if (fDen < -0.00001f || fDen > 0.00001f)  fT = <divide>;
//         else                                      fT = 0.0f;
//
//     The alternative is the negated form
//
//         if (fDen > -0.00001f && fDen < 0.00001f)  fT = 0.0f;
//         else                                      fT = <divide>;
//
//     which emits the SAME instructions in the same places.  THE POOL
//     PREFERS THE FIRST: the six inline `-1e-5` are contiguous at
//     0x100aa270..0x100aa298 and the six `+1e-5` are in a separate run at
//     0x100aa2a0..0x100aa2c8, i.e. the second comparison's literal was
//     emitted with the sunk block and not next to the first -- which is
//     what a `||` whose second arm is sunk looks like, and is how
//     d3dr3a.cpp read the same shape ("the second's true arm jumps
//     BACKWARDS into the first's division block.  A short-circuit ||, not
//     an if/else if").
//
//  B. THE ZERO CLAMP IN REGION D.  Written as
//
//         if (fAlpha < 0.0f) fAlpha = 0.0f;
//
//     because the replacement is the FALL-THROUGH arm at 0x1004e5e6 and
//     the keep path is the `jbe 0x1004e5ee` that jumps over it.  Written
//     the other way round (`if (fAlpha >= 0.0f) ; else fAlpha = 0.0f;`)
//     the two blocks swap.  Two 0.0f in the pool (0x100aa260 for the
//     compare, 0x100aa264 for the value) say there are two literals, not
//     one reused, which the form above gives.
//
//  C. `pVert->m_Specular14.m_n = 0;` IS WRITTEN LAST in region D because
//     0x1004e72d emits it after the +0x20 store.  icl is free to sink a
//     store to a distinct address, so it could be first in the source.
//     No call and no literal distinguishes the two.
//
//  D. THE DIFFUSE INITIALISATION.  `m_Diffuse10.m_n = 0xffffffff` then
//     `m_Diffuse10.m_b[3] = ...` reproduces 0x1004e5fa's dword store of -1
//     followed by 0x1004e62d's byte store at +0x13.  Four separate byte
//     stores would not (the dword store is one instruction), but
//     `m_n = 0xffffffff` followed by a byte store through a different
//     union member is not the only spelling of that pair.
//
//  E. REGION E's AND REGION G's COUNTDOWN LOOPS.  `while (nLeft--)`
//     reproduces the shipped `test ; je` guard plus `dec ; jne` close.
//     `for (i = 0; i < n; i++)` over an explicit index is the other
//     spelling; big6d910.cpp chose `while (nLeft--)` for the identical
//     0x1000d9b0 loop, and this file follows it.
//
// *********************************************************************** //
