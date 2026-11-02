// *********************************************************************** //
//
//  W31_10044FF0_QIPNI (.CPP) -- was big44ff4.cpp, now /O2 /Qip
//  /Qip_no_inlining so that it can hold 0x10047150, the register-only line
//  draw its box pass calls twice (0x10047150 identical; the
//  big function itself 10929 -> 10930 bytes differing, i.e. unchanged).
//  The shipped 0x10044ff4 is itself the register body of a /Qip pair whose
//  4-byte stack entry is at 0x10044ff0 (`mov 0x4(%esp),%eax`).
//
//  The sixty byte conversions go through the RoundF helper
//  (w1_10001000.cpp's inlined __asm `fld ; fistp`, the shipped bare
//  `fistpl`), and fW..fVal are FLOAT: any double local gives icl's
//  14-byte realign entry and a `$$1` body, which the shipped function
//  does not have (coin-flip (d) below is settled that way).  11689-byte
//  $$1 body, 10930 differing -> 8531 bytes (shipped 8540), 7108
//  differing, 0x10047150 still identical.  Not paired yet: the frame is
//  0x4a0 against 0x4b0 -- the shipped code keeps two more 8-byte spills
//  (-0xf0/-0xf8), so the opening diverges at byte 4.  Tried without
//  moving it: the SDK's MatVMul_InPlace_H for the two transforms (same
//  frame), double fT/fVal/fW/fX..fZ (each brings the realign stub back),
//  the third edge as a static inline helper (not inlined under
//  /Qip_no_inlining, even __forceinline).
//
//  big44ff4.cpp's banner follows unchanged.
//
// *********************************************************************** //
// *********************************************************************** //
//
//  MODULE:   BIG_44FF4 (.CPP)
//
//  PURPOSE:  One function -- 0x10044ff4, 8540 bytes.  One caller,
//            0x10044121.
//
//  ------------------------------------------------------------------- //
//  THE EXTENT.  CONFIRMED AS STATED, and the "stated end is also a callee"
//  oddity is resolved.
//  ------------------------------------------------------------------- //
//
//  The body runs 0x10044ff4..0x1004714d and 0x1004714e is `8b f6`
//  (`mov %esi,%esi`), icl's two-byte inter-function pad.  0x10047150 then
//  opens `55 / 8b ec / 83 ec 58` -- its own prologue -- so it is the NEXT
//  function and NOT part of this one.  It is also called twice from inside
//  the loop (0x10045234, 0x10045267): the last thing this function's LOOP
//  BODY does is call its neighbour, which is what the tier-Y row was
//  reporting.  There is no overlap and no missing tail.
//
//  There is exactly ONE `ret`, at 0x10046354, and 3579 bytes of code sit
//  BELOW it (0x10046355..0x1004714d).  That is /O2 cold-block sinking, not
//  a second function: those six blocks are the six clip planes' "first
//  endpoint is outside" arms, every one of them entered by a forward
//  conditional jump from above the `ret` and leaving by a backward `jmp`
//  to the next plane's test (0x10046ef5 -> 0x10045941, 0x10046c8d ->
//  0x10045b9b, 0x10046a2d -> 0x10045df5, 0x100467d5 -> 0x1004605f,
//  0x1004657a -> 0x1004629f, 0x10047149 -> 0x100456e6).  Nothing outside
//  0x10044ff4..0x1004714d branches into the range and nothing inside it
//  branches out.
//
//  ------------------------------------------------------------------- //
//  WHAT IT IS: d3d.ren's MODEL BOUNDING-BOX DEBUG DRAW.
//  ------------------------------------------------------------------- //
//
//  Four independent tells:
//
//    * THE CALLER'S GATE.  0x10044116 is
//      `mov 0x1008cab0,%eax ; test %eax,%eax ; je ... ; mov %esi,%ecx ;
//      call 0x10044ff4`, and big21f90.cpp has already named 0x1008cab0:
//      it is the console variable **ModelBoxes**.  The same caller gates
//      0x10048180 (the projected-shadow pass) on `this->m_698 & 2` two
//      instructions earlier, so `this` is the same object big48180.cpp
//      decoded -- this+0x00 a descriptor, this+0x04 the model object.
//
//    * THE EIGHT CORNERS.  0x1004503a..0x10045196 is eight calls to
//      0x1000eec0 (`LTVector::Init(x,y,z)` out of line, d3dr5b.cpp) into
//      eight consecutive 12-byte slots at -0x2a4, -0x298, -0x28c, -0x280,
//      -0x274, -0x268, -0x25c, -0x250.  The arguments are the eight sign
//      combinations of  pos +/- dims  with pos = *m_pObject->sub_100158b0()
//      (&m_vPos18c, big48180.cpp) and dims the three floats at +0x08,
//      +0x0c, +0x10 of the record 0x10043640 returns.  Corners 0..3 all
//      take +dims.y and corners 4..7 all take -dims.y, each face wound
//      (-,-) (+,-) (+,+) (-,+) in x/z -- a box, top face then bottom face.
//
//    * THE TWELVE EDGES.  The loop at 0x100451e0 runs i = 0..3 and issues
//      sub_10047150(&pts[i+4], &pts[(i+1)%4+4]) and
//      sub_10047150(&pts[i], &pts[(i+1)%4]) -- the four bottom-face edges
//      and the four top-face edges -- and then draws pts[i] -> pts[i+4]
//      itself, which is the four uprights.  4*3 = 12 edges of a box.
//      `(i+1)%4` is a SIGNED divide by four (`sar $1 ; shr $0x1e ;
//      lea 1(%ecx,%ebx,1) ; sar $2` at 0x100451f1..0x1004520b), so the
//      source really wrote `% 4` on an `int`, not `& 3`.
//
//    * THE DRAW.  0x100462f2 is `(g_pD3DDevice, 2, 0x1c4, &v[0], 2, 0)`
//      through vtable +0x64 = DrawPrimitive: D3DPT_LINELIST with
//      D3DFVF_XYZRHW|DIFFUSE|SPECULAR|TEX1, two vertices.  The whole loop
//      is bracketed by SetRenderState(ZWRITEENABLE,0)/(ZENABLE,0) at
//      0x100451b4/0x100451d4 and SetRenderState(ZWRITEENABLE,1)/
//      (ZENABLE, g_nZEnable1008d984) at 0x10046321/0x10046343 -- the
//      wireframe is drawn over everything, z-test and z-write off.
//
//  0x10047150 itself (the NEXT function, not decoded here) is the same
//  draw WITHOUT clipping: it copies two LTVectors into two 32-byte
//  vertices, calls 0x100149a0 (MatVMul_InPlace_H) on each against
//  g_Mat1008b9dc, asks 0x10042e80(v, 0x3f) whether the line is trivially
//  accepted, and if so screen-transforms both with 0x10014580 and draws
//  the same LINELIST.  The third edge per turn is the SAME operation with
//  a REAL six-plane clip, expanded inline here -- which is where 8.1 of
//  the 8.5 KB goes.
//
//  ------------------------------------------------------------------- //
//  COMPILER: Intel C/C++ 4.0, **/O2**.  All three of the /O2 tells:
//  ------------------------------------------------------------------- //
//    * outgoing arguments are `add $-N,%esp` then `mov %reg,K(%esp)`
//      everywhere (0x10045008, 0x10045037, 0x1004519a, 0x100462be ...);
//      there is not one argument `push` in the whole 8.5 KB;
//    * `&m_pObject->m_Anim1c0` at 0x1004502a is `add $0x1c0,%ecx` on a
//      register that already holds the object, not `lea 0x1c0(%ecx)`;
//    * the six cold arms are sunk below the single `ret` (see THE EXTENT).
//
//  AND THE SHIPPED TU WAS BUILT WITH /QIfist, exactly as big61a30.cpp,
//  big5a960.cpp and d3dr3c.cpp record.  All sixty float->byte conversions
//  in the clipper are `fstps <float temp> ; flds <float temp> ;
//  fistpl <int temp> ; mov <int temp>,%cl` -- a bare `fistp` in the
//  current rounding mode, no fnstcw/fldcw, no helper call.  Under plain
//  /O2 or /Os a `(uint8)(int32)f` cast emits the control-word dance
//  instead, so those sixty sites will differ in bytes.  They contain no
//  calls, so the acceptance test is blind to them.
//
//  ------------------------------------------------------------------- //
//  REGION MAP, with what pins each region.
//  ------------------------------------------------------------------- //
//
//  0x10044ff4..0x10045037  PROLOGUE AND THE TWO ACCESSORS.  Frame 0x4b0,
//      %ebx/%esi/%edi saved into the TOP of the frame (`mov %edi,0x8(%esp)`
//      ...) rather than pushed -- icl's usual.  `mov %ecx,%ebx` makes this
//      __thiscall.  sub_100232b0(g_n1008d7d0) (big5a960.cpp names both),
//      then m_pObject->sub_100158b0() into %esi and
//      (m_pObject+0x1c0)->sub_10043640() into %ebx.  d3dr4c.cpp decoded
//      0x10043640 as `&m_pOwner->m_pElements[m_nIndex28]` with a 0x20-byte
//      element; only its +0x08/+0x0c/+0x10 floats are read here.
//
//  0x10045037..0x10045197  THE EIGHT CORNERS.  Eight `add $-0xc,%esp` +
//      three `fstps` + `call 0x1000eec0` groups, %ecx walking -0x2a4 by
//      twelve.  Signs read off the fadds/fsubs pairs, corner by corner.
//
//  0x10045197..0x100451d7  Z OFF.  Two `call *0x50(%eax)` on
//      g_pD3DDevice with (14, 0) and (7, 0).
//
//  0x100451d7..0x10045234  THE LOOP HEAD and the FAR-FACE edge call.
//      %ebx is i, -0xfc(%ebp) is i+1, -0x20(%ebp) is (i+1)/4.  %eax and
//      %edx are loaded and 0x10047150 entered with NO stack arguments:
//      the register ABI the tier-Y row flags.  It is decoded
//      as an ordinary two-argument call and declared __cdecl below; it is
//      still one `call` in the same place.  (The dead `mov $-1,%ecx` at
//      0x1004521a and 0x1004525a is icl propagating the callee's own
//      `mov $0xffffffff,%ecx` back into the caller; it is unreachable from
//      C and is not reproduced.)
//
//  0x10045234..0x1004526c  THE NEAR-FACE edge call, same shape.
//
//  0x1004526c..0x100452e3  THE TWO VERTICES.  Both diffuse dwords set to
//      0xffffffff (`mov $-1,%eax` then two stores at -0x214 and -0x234),
//      then v[0].xyz <- pts[i] and v[1].xyz <- pts[i+4] as three integer
//      `mov`s each -- field by field, NOT `rep movsl`, so three float
//      assignments and not a struct copy.  +0x0c (rhw), +0x14 (specular)
//      and +0x18/+0x1c (u,v) are never initialised and are read by the
//      clipper anyway; that is what the shipped code does.
//
//  0x100452e3..0x100453b3  MatVMul_InPlace_H(&g_Mat1008b9dc, &v[0]),
//      INLINE.  The matrix rows are read straight off the operands:
//      0x1008b9dc..0x1008b9e8 is m[0][*], 0x1008b9ec.. m[1][*],
//      0x1008b9fc.. m[2][*], 0x1008ba0c.. m[3][*].  0x10045335 is
//      `fdivrs 0x100a9cdc` and 0x100a9cdc holds 1.0f, so the reciprocal is
//      ltmatrix.h:233's `one_over_w`.  d3dr3b.cpp identified 0x100149a0 as
//      that very header function; here it is expanded, which /Ob1 does to
//      anything the SDK marks `inline`.
//
//  0x100453b3..0x1004549e  the SAME for v[1], with its OWN 1.0f literal at
//      0x100a9ce4 -- two separate constants for two separate expansions.
//
//  0x1004549e..0x10046304  THE SIX-PLANE LINE CLIP.  Six planes, in the
//      order this project has now read off five other functions:
//          z >= g_f1008b908 | -z < x | y < z | x < z | -z < y
//                                                    | z <= g_f1008b910
//      Each plane is two `fcomp`/`sahf` tests and two clip arms, and every
//      arm carries 0x10023380's body inline in its shipped field order
//      (+0x18, +0x1c, then the bytes +0x12, +0x11, +0x10, +0x13, +0x17).
//      Counted, not by eye: 18 `fcomp` (6*2 hot + 6*1 in the sunk arms),
//      12 divides (one per arm), 60 `fistpl` (12 arms * 5 bytes) and 120
//      `fildl` (two per byte) -- exactly twelve complete LERPs and no more.
//      There is NO |fDen| > 0.00001f epsilon guard here, unlike the
//      polygon clippers in big61a30/big5a960/big16324: the only float
//      literals the whole function references are the two 1.0f above.
//      Plane arms, hot then sunk:
//          NEAR    0x100454b0 / 0x10046f00     LEFT    0x100456fd / 0x10046ca0
//          TOP     0x10045952 / 0x10046a40     RIGHT   0x10045bac / 0x100467e0
//          BOTTOM  0x10045e0c / 0x10046580     FAR     0x10046074 / 0x10046355
//      Every sunk arm's own second test jumps to 0x100462f5, the loop
//      increment -- the whole segment is rejected, which is the `continue`
//      written below.
//
//  0x1004629f..0x100462f5  THE DRAW.  sub_10043620(&v[0]) then
//      sub_10043620(&v[1]) -- d3dr2d.cpp: `sub_10014580(p, g_1008b880)`,
//      the screen projection -- then DrawPrimitive.
//
//  0x100462f5..0x10046304  the loop test, `cmp $0x4 ; jl 0x100451e0`.
//
//  0x10046304..0x10046354  Z BACK ON and the epilogue.
//
//  ------------------------------------------------------------------- //
//  UNDECODED: NONE.  Every one of the 8540 bytes is accounted for above.
//  ------------------------------------------------------------------- //
//
//  ------------------------------------------------------------------- //
//  COIN-FLIPS -- stated with both spellings so one build settles each.
//  ------------------------------------------------------------------- //
//
//  (a) THE TWO VERTICES.  Written here as `Vert44ff4 v[2]` because the
//      DrawPrimitive is handed `&v[0]` with a count of 2.  The alternative
//      is two named locals declared adjacently:
//          Vert44ff4 v0;  Vert44ff4 v1;   ... DrawPrimitive(..., &v0, 2, 0)
//      Both put v0 at -0x244 and v1 at -0x224 and fold every subscript,
//      so this is a source-shape flip only.
//
//  (b) THE FOUR "fVal" PLANE ARMS (LEFT, TOP, RIGHT, BOTTOM).  Written in
//      big61a30.cpp's order -- the free component first, then fVal.  The
//      shipped store order for LEFT is z, x, y (0x10045791 `fsts -0x21c`,
//      0x100457a3, 0x100457b3), which is also what
//          fVal   = (b.z - a.z) * fT + a.z;
//          dst->z = fVal;
//          dst->x = -fVal;
//          dst->y = (b.y - a.y) * fT + a.y;
//      would give.  TOP's shipped order (x, z, y) does not fit that
//      rearrangement, so the family order is kept for all four.
//
//  (c) THE LEFT AND BOTTOM NUMERATORS.  The bytes load the z spill first:
//      `fldl z0 ; faddl x0 ; fchs` at 0x1004572c and `fldl z0 ; faddl y0 ;
//      fchs` at 0x10045e3b, i.e. `-(a.z + a.x)` and `-(a.z + a.y)`.
//      big61a30.cpp spells the same two planes `-(a.x + a.z)` and
//      `-(a.y + a.z)`, which is what is written here.  Both operands are
//      compiler spill slots at that point so the load order may be
//      scheduling; if the LEFT/BOTTOM arms come out short, swap them.
//
//  (d) fT, fVal AND THE FOUR TRANSFORM TEMPORARIES ARE DECLARED
//      `double`.  The interpolant is spilled with `fstl` to an EIGHT-byte
//      slot and reloaded with `fmull` (0x10045525 / 0x100455c4), never
//      rounded through a 4-byte slot, and fVal is stored to the vertex
//      with a NON-popping `fsts` and then negated at full width; the
//      transform's spill at 0x10045439 is `fstpl`, also eight bytes.
//      A float local that never leaves an x87 register emits the same
//      instructions, though, so this is not decidable from the bytes --
//      ltmatrix.h spells its own copy `float one_over_w` + `LTVector
//      temp`.  If the transform comes out long, the alternative is
//          float one_over_w = 1.0f / (...);  LTVec44ff4 temp;
//          temp.x = one_over_w * (...); temp.y = ...; temp.z = ...;
//          v[0].x = temp.x; v[0].y = temp.y; v[0].z = temp.z;
//      -- but NOT `*(LTVec44ff4*)&v[0] = temp;`, because a struct
//      assignment is `rep movsl` and there is not one in the shipped
//      bytes.  The twelve DIFFERENT eight-byte slots (-0x68, -0x78,
//      -0x88, -0x90, -0x98, -0xa0, -0xb0, -0xb8, -0xc0, -0xc8, -0xd8)
//      are icl allocating one temp per site -- it does the same for the
//      sixty `fistpl` destinations and the sixty float temps -- and are
//      NOT twelve source variables.
//
//  (e) THE THIRD EDGE IS WRITTEN OUT INLINE, not as an `inline` helper
//      called once per turn.  A marked-inline helper would compile to the
//      same thing under /Ob1, but if icl decided to emit an out-of-line
//      copy as well the call sequence would gain a row the shipped code
//      does not have, so the risk is not worth taking.
//
//  ------------------------------------------------------------------- //
//  WHAT THE CALL-SEQUENCE TEST DOES **NOT** WITNESS HERE.
//  ------------------------------------------------------------------- //
//
//  The call count over the range: 8540 bytes, 20 calls (15 direct, 5 indirect),
//  427 bytes per call, and only SIX distinct direct callees.  That is deep
//  in the weak band (over 300 B/call), so the split matters more than the score:
//
//    * 0x10044ff4..0x10045197   419 B   11 of the 15 direct calls
//    * 0x100451d7..0x1004526c   149 B   the two 0x10047150 calls
//    * 0x10045197..0x100451d7    64 B   the two Z-off SetRenderStates
//    * 0x1004629f..0x10046354   182 B   the draw, the loop test, the tail
//    * 0x1004526c..0x1004629f  4147 B   ZERO calls
//    * 0x10046355..0x1004714d  3577 B   ZERO calls
//
//  So **7724 of 8540 bytes -- 90.4% of the function -- is call-free**:
//  the two inlined MatVMul_InPlace_H copies and all twelve clip arms.  A
//  perfect 20 of 20 says nothing whatever about any of it.
//
//  What pins that 90% instead, all of it read off the bytes:
//    * the transform's operands are sixteen consecutive dwords at
//      0x1008b9dc used in ltmatrix.h:233's exact pattern, with the 1.0f
//      reciprocal at 0x100a9cdc / 0x100a9ce4 (both verified to be
//      0x3f800000) -- and 0x100149a0, which d3dr3b.cpp identified as that
//      very header function, is called from 0x10047150 with 0x1008b9dc as
//      its matrix argument, so the neighbour proves the pairing;
//    * the six plane predicates, in order, are the SAME six this project
//      has now read independently out of 0x100124a4, 0x10016324,
//      0x1005a960, 0x10061a30 and 0x1006d910;
//    * the six numerator/denominator pairs match big61a30.cpp's plane for
//      plane, operand for operand;
//    * the instruction census matches the structure exactly and leaves no
//      room for anything else: 18 `fcomp` (6*2 hot + 6*1 sunk), 12
//      divides, 60 `fistpl` and 120 `fildl` -- twelve arms of five byte
//      LERPs and not one more;
//    * the byte-lerp field order +0x12, +0x11, +0x10, +0x13, +0x17 is
//      0x10023380's, already decoded in d3dbatchd.cpp and expanded the
//      same way in big61a30.cpp and big5a960.cpp.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  0x1000eec0 -- LTVector::Init(x,y,z) OUT OF LINE (d3dr5b.cpp).  It must
//  have its own declaration here: /Ob1 would expand the SDK's inline copy
//  and the eight calls would vanish.
// ---------------------------------------------------------------------- //
struct LTVec44ff4
{
	void	sub_1000eec0(float fX, float fY, float fZ);     // 0x1000eec0

	float	x;                                              // +0x00
	float	y;                                              // +0x04
	float	z;                                              // +0x08
};


// ---------------------------------------------------------------------- //
//  The 32-byte draw vertex.  FVF 0x1c4 = XYZRHW|DIFFUSE|SPECULAR|TEX1,
//  read straight off the DrawPrimitive at 0x100462f2, and every offset
//  below is touched by the clipper.
// ---------------------------------------------------------------------- //
union VColor44ff4
{
	uint32	m_n;
	uint8	m_b[4];
};

struct Vert44ff4                                            // 0x20 bytes
{
	float		x;                                          // +0x00
	float		y;                                          // +0x04
	float		z;                                          // +0x08
	float		m_fRHW0c;                                   // +0x0c
	VColor44ff4	m_Diffuse10;                                // +0x10
	VColor44ff4	m_Specular14;                               // +0x14
	float		tu;                                         // +0x18
	float		tv;                                         // +0x1c
};


// ---------------------------------------------------------------------- //
//  ltmatrix.h's LTMatrix, but declared locally: only the sixteen floats
//  matter and the SDK class would drag its inline members in.
// ---------------------------------------------------------------------- //
struct LTMat44ff4
{
	float	m[4][4];
};


// ---------------------------------------------------------------------- //
//  IDirect3DDevice7 -- two slots: +0x50 SetRenderState (index 20) and
//  +0x64 DrawPrimitive (index 25).
// ---------------------------------------------------------------------- //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8	m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);       // +0x50
	uint8	m_pad_54[0x64 - 0x54];
	long (__stdcall* DrawPrimitive)(D3DDevice7*, uint32 dwType, uint32 dwFVF,
	                                void* lpVerts, uint32 dwCount,
	                                uint32 dwFlags);        // +0x64
};

struct D3DDevice7
{
	D3DDevice7Vtbl*	lpVtbl;
};

#define D3DPT_LINELIST                          2
#define D3DFVF_CLIPVERTEX                   0x1c4  // XYZRHW|DIFFUSE|SPEC|TEX1
#define D3DRENDERSTATE_ZENABLE                  7
#define D3DRENDERSTATE_ZWRITEENABLE            14


// ---------------------------------------------------------------------- //
//  0x10043640 -- d3dr4c.cpp's `&m_pOwner->m_pElements[m_nIndex28]`, an
//  0x20-byte element.  The class and element TAG NAMES are copied from
//  that file verbatim so the mangled symbol is the same one it defines;
//  only the element's field spelling differs (this function reads three
//  floats where d3dr4c.cpp had nothing to go on and wrote uint32[8]).
// ---------------------------------------------------------------------- //
struct Elem10043640                                         // 0x20 bytes
{
	uint32	m_n00;                                          // +0x00
	uint32	m_n04;                                          // +0x04
	float	m_fDims08;                                      // +0x08
	float	m_fDims0c;                                      // +0x0c
	float	m_fDims10;                                      // +0x10
	uint32	m_n14;                                          // +0x14
	uint32	m_n18;                                          // +0x18
	uint32	m_n1c;                                          // +0x1c
};

struct Owner10043640
{
	uint8			m_Reserved000[0x168];                   // +0x000
	Elem10043640*	m_pElements;                            // +0x168
};

class C10043640
{
public:
	Elem10043640*	sub_10043640();                         // 0x10043640

	uint8			m_Reserved00[0x1c];                     // +0x00
	Owner10043640*	m_pOwner;                               // +0x1c
	uint8			m_Reserved20[0x28 - 0x20];              // +0x20
	uint16			m_nIndex28;                             // +0x28
};


// ---------------------------------------------------------------------- //
//  The model object, this+0x04.  big48180.cpp reads the same +0x18c
//  through the same accessor; +0x1c0 is the sub-object 0x10043640 is
//  entered on (its +0x1c and +0x28 are the object's +0x1dc and +0x1e8,
//  and 0x100158c0 -- 0x100158b0's immediate neighbour -- is
//  `mov 0x1dc(%ecx),%eax`, so +0x1dc really is a pointer member).
// ---------------------------------------------------------------------- //
class Obj44ff4
{
public:
	LTVec44ff4*	sub_100158b0();                             // 0x100158b0

	uint8		m_Reserved000[0x18c];                       // +0x000
	LTVec44ff4	m_vPos18c;                                  // +0x18c
	uint8		m_Reserved198[0x1c0 - 0x198];               // +0x198
	C10043640	m_Anim1c0;                                  // +0x1c0
};


// ---------------------------------------------------------------------- //
//  The renderer object -- big48180.cpp's C10048180, same two pointers.
// ---------------------------------------------------------------------- //
class C10044ff4
{
public:
	void		sub_10044ff4();                             // 0x10044ff4

	void*		m_pDesc;                                    // +0x00
	Obj44ff4*	m_pObject;                                  // +0x04
};


// ---------------------------------------------------------------------- //
//  Globals.  Every one is an address this function touches.
// ---------------------------------------------------------------------- //
extern D3DDevice7*	g_pD3DDevice;                           // 0x1008d988
extern uint32		g_nZEnable1008d984;                     // 0x1008d984
extern uint32		g_n1008d7d0;                            // 0x1008d7d0
extern float		g_f1008b908;                            // 0x1008b908  near
extern float		g_f1008b910;                            // 0x1008b910  far
extern LTMat44ff4	g_Mat1008b9dc;                          // 0x1008b9dc

extern "C" void sub_100232b0(uint32 nStage);                // 0x100232b0
extern "C" void sub_10043620(void* pPoint);                 // 0x10043620

// 0x10047150 -- the UNCLIPPED two-point line draw.  It is entered with
// %eax = the first point and %edx = the second and takes no stack
// arguments: a /Qip REGISTER-ONLY entry, i.e. a static of the TU of its
// only caller, this function.  Moved here from w7_10040cc0.cpp
// and byte-identical here, with this file built /O2 /Qip
// /Qip_no_inlining (plain /Qip inlines it into both call sites).  The two
// colour stores go v[1] first, as the shipped -0x10 then -0x30.
// It takes the colour as a THIRD register argument -- both
// shipped call sites load `mov $0xffffffff,%ecx` before the call (with a
// 16-byte outgoing area, `add $-0x10`), and the body, which /Qip compiles
// with the one constant both callers pass, materialises the same %ecx
// itself; still byte-identical.  Each call site also spells `(i + 1) % 4`
// out (no nNext variable): the shipped code keeps the quotient of the
// first and rebuilds the second index from it.  0x10044ff4: 7108 -> 6822
// bytes differing, frame 0x4a0 -> 0x4a8 (shipped 0x4b0).
struct W7TLVert47150
{
	float		m_x, m_y, m_z, m_rhw;                   // +0x00
	uint32		m_nColor;                               // +0x10
	uint32		m_nSpecular;                            // +0x14
	float		m_tu, m_tv;                             // +0x18
};

class Mat10027eb0;
class Vec10027eb0;
extern float sub_100149a0(Mat10027eb0* pMat, Vec10027eb0* pVec);    // 0x100149a0
extern "C" int  sub_10042e80(void* pVerts, uint32 nMask);           // 0x10042e80
extern "C" void sub_10014580(void* pVert, void* pLight);            // 0x10014580
extern unsigned char g_1008b880[28];                                // 0x1008b880

static void sub_10047150(LTVec44ff4* p1, LTVec44ff4* p2, uint32 nColor)
{
	W7TLVert47150 v[2];

	v[1].m_nColor = nColor;
	v[0].m_nColor = nColor;
	v[0].m_x = p1->x; v[0].m_y = p1->y; v[0].m_z = p1->z;
	v[1].m_x = p2->x; v[1].m_y = p2->y; v[1].m_z = p2->z;

	sub_100149a0((Mat10027eb0*)&g_Mat1008b9dc, (Vec10027eb0*)&v[0]);
	sub_100149a0((Mat10027eb0*)&g_Mat1008b9dc, (Vec10027eb0*)&v[1]);

	if (sub_10042e80(v, 0x3f))
	{
		sub_10014580(&v[0], g_1008b880);
		sub_10014580(&v[1], g_1008b880);
		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 2, 0x1c4, v, 2, 0);
	}
}


// ---------------------------------------------------------------------- //
//  0x10023380's body, expanded inline TWELVE times in the shipped clipper
//  (there is no `call 0x10023380` anywhere in the 8.5 KB).  d3dbatchd.cpp
//  describes it and big61a30.cpp / big5a960.cpp use the same spelling:
//  two floats at +0x18/+0x1c, then five bytes in the order +0x12, +0x11,
//  +0x10, +0x13, +0x17, each `fstps temp ; flds temp ; fistpl n ;
//  mov n,%al`.  Written as a macro rather than a function so that no
//  out-of-line copy can be emitted.
// ---------------------------------------------------------------------- //
// Round to nearest in the current FPU mode (w1_10001000.cpp's helper;
// 0x10015190 out of line): inlined, `fstps t ; flds t ; fistpl n`.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

#define LERP44FF4(a, b, dst, t)                                                                                                          \
	(dst).tu = ((b).tu - (a).tu) * (t) + (a).tu;                                                                                         \
	(dst).tv = ((b).tv - (a).tv) * (t) + (a).tv;                                                                                         \
	(dst).m_Diffuse10.m_b[2]  = (uint8)RoundF(((b).m_Diffuse10.m_b[2]  - (a).m_Diffuse10.m_b[2])  * (t) + (a).m_Diffuse10.m_b[2]);      \
	(dst).m_Diffuse10.m_b[1]  = (uint8)RoundF(((b).m_Diffuse10.m_b[1]  - (a).m_Diffuse10.m_b[1])  * (t) + (a).m_Diffuse10.m_b[1]);      \
	(dst).m_Diffuse10.m_b[0]  = (uint8)RoundF(((b).m_Diffuse10.m_b[0]  - (a).m_Diffuse10.m_b[0])  * (t) + (a).m_Diffuse10.m_b[0]);      \
	(dst).m_Diffuse10.m_b[3]  = (uint8)RoundF(((b).m_Diffuse10.m_b[3]  - (a).m_Diffuse10.m_b[3])  * (t) + (a).m_Diffuse10.m_b[3]);      \
	(dst).m_Specular14.m_b[3] = (uint8)RoundF(((b).m_Specular14.m_b[3] - (a).m_Specular14.m_b[3]) * (t) + (a).m_Specular14.m_b[3])


void C10044ff4::sub_10044ff4()
{
	LTVec44ff4		pts[8];
	Vert44ff4		v[2];

	LTVec44ff4*		pPos;
	Elem10043640*	pDims;
	int32			i;
	float			fW;
	float			fX;
	float			fY;
	float			fZ;
	float			fT;
	float			fVal;

	// -- 0x1004500b: sub_100232b0(g_n1008d7d0) ------------------------- //
	sub_100232b0(g_n1008d7d0);

	// -- 0x1004501a / 0x10045025: the two accessors -------------------- //
	pPos  = m_pObject->sub_100158b0();
	pDims = m_pObject->m_Anim1c0.sub_10043640();

	// -- 0x10045037..0x10045196: the eight corners of the box ---------- //
	//    Read off the fadds/fsubs pair in front of each of the eight
	//    `call 0x1000eec0`s; the destination walks -0x2a4 by twelve.
	pts[0].sub_1000eec0(pPos->x - pDims->m_fDims08,
	                    pPos->y + pDims->m_fDims0c,
	                    pPos->z - pDims->m_fDims10);
	pts[1].sub_1000eec0(pPos->x + pDims->m_fDims08,
	                    pPos->y + pDims->m_fDims0c,
	                    pPos->z - pDims->m_fDims10);
	pts[2].sub_1000eec0(pPos->x + pDims->m_fDims08,
	                    pPos->y + pDims->m_fDims0c,
	                    pPos->z + pDims->m_fDims10);
	pts[3].sub_1000eec0(pPos->x - pDims->m_fDims08,
	                    pPos->y + pDims->m_fDims0c,
	                    pPos->z + pDims->m_fDims10);
	pts[4].sub_1000eec0(pPos->x - pDims->m_fDims08,
	                    pPos->y - pDims->m_fDims0c,
	                    pPos->z - pDims->m_fDims10);
	pts[5].sub_1000eec0(pPos->x + pDims->m_fDims08,
	                    pPos->y - pDims->m_fDims0c,
	                    pPos->z - pDims->m_fDims10);
	pts[6].sub_1000eec0(pPos->x + pDims->m_fDims08,
	                    pPos->y - pDims->m_fDims0c,
	                    pPos->z + pDims->m_fDims10);
	pts[7].sub_1000eec0(pPos->x - pDims->m_fDims08,
	                    pPos->y - pDims->m_fDims0c,
	                    pPos->z + pDims->m_fDims10);

	// -- 0x10045197..0x100451d6: z-write and z-test off ---------------- //
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_ZWRITEENABLE, 0);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_ZENABLE, 0);

	// -- 0x100451d7: `xor %ebx,%ebx`, then 0x100462fb `cmp $0x4 ; jl` -- //
	for (i = 0; i < 4; i++)
	{
		// -- 0x10045234 and 0x10045267, both register-ABI ------------- //
		sub_10047150(&pts[i + 4], &pts[(i + 1) % 4 + 4], 0xffffffff);
		sub_10047150(&pts[i],     &pts[(i + 1) % 4], 0xffffffff);

		// ---------------------------------------------------------- //
		//  0x1004526c: the upright edge, drawn WITH a full six-plane
		//  clip.  Everything from here to the DrawPrimitive is one
		//  expansion of the same body 0x10047150 carries without the
		//  clip.
		// ---------------------------------------------------------- //
		v[1].m_Diffuse10.m_n = 0xffffffff;
		v[0].m_Diffuse10.m_n = 0xffffffff;

		v[0].x = pts[i].x;
		v[0].y = pts[i].y;
		v[0].z = pts[i].z;

		v[1].x = pts[i + 4].x;
		v[1].y = pts[i + 4].y;
		v[1].z = pts[i + 4].z;

		// The frame's missing 8 bytes are ONE x87 spill:
		// the shipped v[1] transform overflows the FPU stack and parks a
		// row sum in -0x10 (`fstpl` ... `fldl`); every slot below is ours
		// shifted by 8.  The shipped transforms also store in a different
		// order (v[0]: x, y, z; v[1]: z, x, y -- ours z, y, x and x, z, y),
		// i.e. icl scheduled the rows differently, not a spelling we have.
		// -- 0x100452e3: MatVMul_InPlace_H(&g_Mat1008b9dc, &v[0]) ---- //
		fW = 1.0f / (g_Mat1008b9dc.m[3][0] * v[0].x
		           + g_Mat1008b9dc.m[3][1] * v[0].y
		           + g_Mat1008b9dc.m[3][2] * v[0].z
		           + g_Mat1008b9dc.m[3][3]);

		fX = fW * (g_Mat1008b9dc.m[0][0] * v[0].x
		         + g_Mat1008b9dc.m[0][1] * v[0].y
		         + g_Mat1008b9dc.m[0][2] * v[0].z
		         + g_Mat1008b9dc.m[0][3]);
		fY = fW * (g_Mat1008b9dc.m[1][0] * v[0].x
		         + g_Mat1008b9dc.m[1][1] * v[0].y
		         + g_Mat1008b9dc.m[1][2] * v[0].z
		         + g_Mat1008b9dc.m[1][3]);
		fZ = fW * (g_Mat1008b9dc.m[2][0] * v[0].x
		         + g_Mat1008b9dc.m[2][1] * v[0].y
		         + g_Mat1008b9dc.m[2][2] * v[0].z
		         + g_Mat1008b9dc.m[2][3]);

		v[0].x = (float)fX;
		v[0].y = (float)fY;
		v[0].z = (float)fZ;

		// -- 0x100453b3: the same for v[1], its own 1.0f at 0x100a9ce4 //
		fW = 1.0f / (g_Mat1008b9dc.m[3][0] * v[1].x
		           + g_Mat1008b9dc.m[3][1] * v[1].y
		           + g_Mat1008b9dc.m[3][2] * v[1].z
		           + g_Mat1008b9dc.m[3][3]);

		fX = fW * (g_Mat1008b9dc.m[0][0] * v[1].x
		         + g_Mat1008b9dc.m[0][1] * v[1].y
		         + g_Mat1008b9dc.m[0][2] * v[1].z
		         + g_Mat1008b9dc.m[0][3]);
		fY = fW * (g_Mat1008b9dc.m[1][0] * v[1].x
		         + g_Mat1008b9dc.m[1][1] * v[1].y
		         + g_Mat1008b9dc.m[1][2] * v[1].z
		         + g_Mat1008b9dc.m[1][3]);
		fZ = fW * (g_Mat1008b9dc.m[2][0] * v[1].x
		         + g_Mat1008b9dc.m[2][1] * v[1].y
		         + g_Mat1008b9dc.m[2][2] * v[1].z
		         + g_Mat1008b9dc.m[2][3]);

		v[1].x = (float)fX;
		v[1].y = (float)fY;
		v[1].z = (float)fZ;

		// =========================================================== //
		//  THE SIX-PLANE LINE CLIP.  Plane order and predicates are
		//  the ones five other d3d.ren functions already carry:
		//      z >= near | -z < x | y < z | x < z | -z < y | z <= far
		//  Each plane: if the first endpoint is inside, clip the second
		//  when it is outside; if the first endpoint is outside, throw
		//  the whole segment away when the second is outside too, else
		//  clip the first.  The `else` arms are the six blocks /O2 sank
		//  below the epilogue.
		// =========================================================== //

		// ---- 0x100454b0 / 0x10046f00: NEAR, z >= g_f1008b908 ------- //
		if (v[0].z >= g_f1008b908)
		{
			if (v[1].z < g_f1008b908)
			{
				fT = (g_f1008b908 - v[0].z) / (v[1].z - v[0].z);

				v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
				v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
				v[1].z = g_f1008b908;

				LERP44FF4(v[0], v[1], v[1], fT);
			}
		}
		else
		{
			if (v[1].z < g_f1008b908)
				continue;

			fT = (g_f1008b908 - v[0].z) / (v[1].z - v[0].z);

			v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			v[0].z = g_f1008b908;

			LERP44FF4(v[0], v[1], v[0], fT);
		}

		// ---- 0x100456fd / 0x10046ca0: LEFT, -z < x ----------------- //
		if (-v[0].z < v[0].x)
		{
			if (-v[1].z >= v[1].x)
			{
				fT = -(v[0].x + v[0].z)
				   / ((v[1].x - v[0].x) + v[1].z - v[0].z);

				v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
				fVal   = (v[1].z - v[0].z) * fT + v[0].z;
				v[1].z = (float)fVal;
				v[1].x = (float)-fVal;

				LERP44FF4(v[0], v[1], v[1], fT);
			}
		}
		else
		{
			if (-v[1].z >= v[1].x)
				continue;

			fT = -(v[0].x + v[0].z)
			   / ((v[1].x - v[0].x) + v[1].z - v[0].z);

			v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			fVal   = (v[1].z - v[0].z) * fT + v[0].z;
			v[0].z = (float)fVal;
			v[0].x = (float)-fVal;

			LERP44FF4(v[0], v[1], v[0], fT);
		}

		// ---- 0x10045952 / 0x10046a40: TOP, y < z ------------------- //
		if (v[0].y < v[0].z)
		{
			if (v[1].y >= v[1].z)
			{
				fT = -(v[0].y - v[0].z)
				   / ((v[1].y - v[0].y) - v[1].z + v[0].z);

				v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
				fVal   = (v[1].z - v[0].z) * fT + v[0].z;
				v[1].y = (float)fVal;
				v[1].z = (float)fVal;

				LERP44FF4(v[0], v[1], v[1], fT);
			}
		}
		else
		{
			if (v[1].y >= v[1].z)
				continue;

			fT = -(v[0].y - v[0].z)
			   / ((v[1].y - v[0].y) - v[1].z + v[0].z);

			v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			fVal   = (v[1].z - v[0].z) * fT + v[0].z;
			v[0].y = (float)fVal;
			v[0].z = (float)fVal;

			LERP44FF4(v[0], v[1], v[0], fT);
		}

		// ---- 0x10045bac / 0x100467e0: RIGHT, x < z ----------------- //
		if (v[0].x < v[0].z)
		{
			if (v[1].x >= v[1].z)
			{
				fT = -(v[0].x - v[0].z)
				   / ((v[1].x - v[0].x) - v[1].z + v[0].z);

				v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
				fVal   = (v[1].z - v[0].z) * fT + v[0].z;
				v[1].x = (float)fVal;
				v[1].z = (float)fVal;

				LERP44FF4(v[0], v[1], v[1], fT);
			}
		}
		else
		{
			if (v[1].x >= v[1].z)
				continue;

			fT = -(v[0].x - v[0].z)
			   / ((v[1].x - v[0].x) - v[1].z + v[0].z);

			v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			fVal   = (v[1].z - v[0].z) * fT + v[0].z;
			v[0].x = (float)fVal;
			v[0].z = (float)fVal;

			LERP44FF4(v[0], v[1], v[0], fT);
		}

		// ---- 0x10045e0c / 0x10046580: BOTTOM, -z < y --------------- //
		if (-v[0].z < v[0].y)
		{
			if (-v[1].z >= v[1].y)
			{
				fT = -(v[0].y + v[0].z)
				   / ((v[1].y - v[0].y) + v[1].z - v[0].z);

				v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
				fVal   = (v[1].z - v[0].z) * fT + v[0].z;
				v[1].z = (float)fVal;
				v[1].y = (float)-fVal;

				LERP44FF4(v[0], v[1], v[1], fT);
			}
		}
		else
		{
			if (-v[1].z >= v[1].y)
				continue;

			fT = -(v[0].y + v[0].z)
			   / ((v[1].y - v[0].y) + v[1].z - v[0].z);

			v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			fVal   = (v[1].z - v[0].z) * fT + v[0].z;
			v[0].z = (float)fVal;
			v[0].y = (float)-fVal;

			LERP44FF4(v[0], v[1], v[0], fT);
		}

		// ---- 0x10046074 / 0x10046355: FAR, z <= g_f1008b910 -------- //
		if (v[0].z <= g_f1008b910)
		{
			if (v[1].z > g_f1008b910)
			{
				fT = (g_f1008b910 - v[0].z) / (v[1].z - v[0].z);

				v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
				v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
				v[1].z = g_f1008b910;

				LERP44FF4(v[0], v[1], v[1], fT);
			}
		}
		else
		{
			if (v[1].z > g_f1008b910)
				continue;

			fT = (g_f1008b910 - v[0].z) / (v[1].z - v[0].z);

			v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			v[0].z = g_f1008b910;

			LERP44FF4(v[0], v[1], v[0], fT);
		}

		// -- 0x1004629f: project both endpoints and draw the line --- //
		sub_10043620(&v[0]);
		sub_10043620(&v[1]);

		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, D3DPT_LINELIST,
		                                    D3DFVF_CLIPVERTEX, v, 2, 0);
	}

	// -- 0x10046304..0x10046345: z-write and z-test back on ------------ //
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_ZWRITEENABLE, 1);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_ZENABLE,
	                                     g_nZEnable1008d984);
}
