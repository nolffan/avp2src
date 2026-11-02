// *********************************************************************** //
//
//  MODULE:   BIG4B720 (.CPP)
//
//  PURPOSE:  0x1004b720 .. 0x1004e2e4 -- 11,204 bytes, 135 direct call
//            sites and 29 indirect ones, ONE caller (0x100481ec).
//            Reconstructed instruction by instruction from the retail d3d.ren
//            under the "structurally right, not byte-exact"
//            acceptance bar: same control flow, same callees in the same
//            order, same field offsets, same constants.
//
//  WHAT IT IS.  The renderer's SHADOW PASS.  Two strings settle that and
//  nothing else is needed:
//
//      0x100aa2f0  "Invalid texture size for shadows -- must be power
//                   of 2..."                              (0x1004ba92)
//      0x100aa330  "Shadow -> Find: %d, Setup: %d, Texture : %d,
//                   Draw: %d, Total: %d ticks"            (0x1004e238)
//
//  and the body agrees: it picks the N nearest shadow casters, renders each
//  one into a square power-of-two render target through an orthographic
//  light basis, then hands the resulting projector to a per-polygon
//  clipper.
//
//  WHICH FLAG.  /O2, on four independent readings:
//    * outgoing arguments are `add $-N,%esp` then `mov` (0x1004b7e3,
//      0x1004bb2f, 0x1004c296 ...), which is the /O2
//      pattern; /Os would push.
//    * `&p->m` comes out as `mov $0x4,%edi ; add %esp,%edi` (0x1004b7ec,
//      0x1004bebc, 0x1004ca9a), the /O2 spelling of `lea 0x4(%esp),%edi`.
//    * the distance-search loop at 0x1004b877 is UNROLLED FOUR TIMES into
//      the tail at 0x1004e279 .. 0x1004e2c8.  /Os never unrolls.
//    * the cold arm of the clamp at 0x1004bade is moved out of line to
//      0x1004e2c9, past the epilogue -- /O2 cold-block placement.
//
//  THE FRAME.  0x1004b720 is icl's ALIGNED entry pair: `push %ebx ;
//  mov %esp,%ebx ; sub $8 ; and $-16 ; add $8 ; jmp` with the unaligned
//  twin at 0x1004b72e, then `mov $0x2988,%eax ; call 0x100805a0` (_chkstk)
//  for 10,632 bytes of locals.  Arguments are read off %ebx, so
//  `0x8(%ebx)` is the single dword argument and `ret $0x4` confirms one.
//  %ecx is never reloaded: __thiscall.
//
//  WHAT IS NOT DECODED.  0x1004d29e .. 0x1004de44 -- about 3.0 KB of
//  straight-line x87 with ZERO calls in it, an inlined chain of 4x4 matrix
//  builds and multiplies whose product is copied to the frustum's +0x60.
//  It is left as a documented gap; see the banner at the call site.  It
//  cannot move the call sequence because it contains no call.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>

// The shipped code CALLS the CRT for both of these (0x1007fc98 sqrt,
// 0x1007fc90 fabs); `#pragma function` keeps icl from turning either into
// an inline `fsqrt`/`fabs`, which is the opposite of what d3dbatchd.cpp
// needed for 0x10023330.
#pragma function(sqrt, fabs)


// ======================================================================= //
//  THE VECTOR.  ltvector.h's _CVector<float>.
//
//  Every member below is DECLARED ONLY.  Under /Ob1 icl inlines what the
//  source marks inline, and ltvector.h marks all of these inline -- but the
//  shipped function CALLS them at these addresses, so defining them here
//  would delete 60-odd calls from the sequence.  Where the shipped code
//  really does inline the same header line (the four plane builds at
//  0x1004ce42 .. 0x1004d29e, and the two DistTo's at 0x1004df26) the
//  arithmetic is written out by hand instead, for the same reason in
//  reverse.
//
//      0x1004b6f0  operator=(const _CVector&)  -- 3 int moves, returns this
//      0x10015480  _CVector(T,T,T)             -- ret $0xc, returns this
//      0x100155f0  operator+(_CVector)         -- ret $0x10
//      0x10015640  operator-(_CVector)         -- ret $0x10
//      0x10015690  operator*(T)                -- ret $0x08
//      0x10023350  operator-()                 -- ret $0x04
//      0x10015590  Cross(_CVector)             -- ret $0x10
//      0x10027f50  Dot(_CVector)               -- ret $0x0c, float in st0
//      0x10023330  Mag()                       -- ret, float in st0
//      0x10015530  Norm(T)                     -- ret $0x04
//      0x1004e2f0  operator[](uint32)          -- ret $0x04, `lea (%ecx,%eax,4)`
// ======================================================================= //
struct Vec4b720
{
	float   x;                                          // +0x00
	float   y;                                          // +0x04
	float   z;                                          // +0x08

	Vec4b720&  sub_1004b6f0(const Vec4b720& v);
	Vec4b720&  sub_10015480(float mx, float my, float mz);
	Vec4b720   sub_100155f0(Vec4b720 v) const;
	Vec4b720   sub_10015640(Vec4b720 v) const;
	Vec4b720   sub_10015690(float v) const;
	Vec4b720   sub_10023350() const;
	Vec4b720   sub_10015590(Vec4b720 v) const;
	float   sub_10027f50(Vec4b720 v) const;
	float   sub_10023330() const;
	void    sub_10015530(float nVal);
	float*  sub_1004e2f0(uint32 i);

	// 0x1000eec0 is byte-for-byte 0x10015480 plus one dead
	// `mov %edi,0x8(%esp)` -- the SAME header line emitted a second time
	// out of another translation unit.  The shipped function reaches the
	// 0x10015480 copy for its LTVector locals (0x1004be56, 0x1004beae,
	// 0x1004c033) and the 0x1000eec0 copy for the four D3D vertex
	// positions (0x1004c22c .. 0x1004c291), so both are named here.
	// (d3dr5b.cpp already names 0x1000eec0 by address.)
	Vec4b720&  sub_1000eec0(float mx, float my, float mz);
};


// ltplane.h's LTPlane, laid out by the shipped offsets: the normal at +0x00
// and the distance at +0x0c, which is what makes the frustum's plane array
// stride 0x10 (0x1004cd32 writes -0x50c and 0x1004cdb4 writes -0x500).
struct Plane4b720
{
	Vec4b720   m_Normal;                                   // +0x00
	float   m_Dist;                                     // +0x0c
};


// ltmatrix.h's LTMatrix.  MakeInverse (0x10015740) is already reconstructed
// in d3dr4b.cpp; here it is declared only, and the 64-byte `rep movsl` at
// 0x1004c31d is the ordinary struct assignment of its by-value return.
struct Mat4b720
{
	float       m[4][4];

	Mat4b720   sub_10015740();                         // MakeInverse
};

// ltmatrix.h:233 MatVMul_InPlace, out of line at 0x1004e300 (d3dr2c.cpp
// already pins it).  __cdecl: the two pushes at 0x1004b7d6/0x1004b7dd are
// still live when 0x1004b7e3 opens the next outgoing area.
extern void sub_1004e300(Mat4b720* pMat, Vec4b720* pSrc);


// ======================================================================= //
//  CMoArray<T,C> -- src/LT2/lithshared/stdlith/dynarray.h, the family d3dr4b.cpp
//  identified from the genlist.h vtables.  Two members are reached here and
//  both are one instruction:
//
//      0x1004fb70   push %esi ; mov 0x4(%ecx),%eax ; add $4,%esp ; ret
//      0x1007ee60   push %esi ; mov 0x8(%ecx),%eax ; add $4,%esp ; ret
//
//  so +0x04 is m_pArray (GetArray) and +0x08 is m_nElements (GetSize),
//  behind the GenList vptr at +0x00.  The `add $0x4,%esp` with nothing
//  pushed is icl's dead slot, exactly as d3dbatchb.cpp measured.
// ======================================================================= //
struct Arr4b720
{
	void*   m_pVTable;                                  // +0x00
	void*   m_pArray;                                   // +0x04
	uint32  m_nElements;                                // +0x08

	void*   sub_1004fb70() const;                       // GetArray
	uint32  sub_1007ee60() const;                       // GetSize
};


// ======================================================================= //
//  THE D3D7 DEVICE AT 0x1008d988 -- the same object d3ddraw.cpp,
//  d3dopt2d.cpp and d3dbatcha.cpp model, extended with the three slots this
//  function needs that they did not.  Index = offset/4 against
//  IDirect3DDevice7, which the project already anchors with +0x50
//  SetRenderState (20) and +0x8c SetTexture (35):
//
//      +0x50 (20) SetRenderState          +0x88 (34) GetTexture
//      +0x54 (21) GetRenderState          +0x8c (35) SetTexture
//      +0x64 (25) DrawPrimitive           +0x90 (36) GetTextureStageState
//                                         +0x94 (37) SetTextureStageState
// ======================================================================= //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	void*   m_pad_00[20];                               // +0x00 .. +0x4c
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32, uint32);        // +0x50
	long (__stdcall* GetRenderState)(D3DDevice7*, uint32, uint32*);       // +0x54
	void*   m_pad_58[3];                                // +0x58 .. +0x60
	long (__stdcall* DrawPrimitive)(D3DDevice7*, uint32 nType, uint32 nFVF,
	                                void* pVerts, uint32 nCount,
	                                uint32 nFlags);     // +0x64
	void*   m_pad_68[8];                                // +0x68 .. +0x84
	long (__stdcall* GetTexture)(D3DDevice7*, uint32, void**);            // +0x88
	long (__stdcall* SetTexture)(D3DDevice7*, uint32, void*);             // +0x8c
	long (__stdcall* GetTextureStageState)(D3DDevice7*, uint32, uint32,
	                                       uint32*);    // +0x90
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32, uint32,
	                                       uint32);     // +0x94
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

#define D3DPT_TRIANGLELIST                    4
#define D3DPT_TRIANGLEFAN                     6

#define D3DFVF_SHADOWVERTEX               0x1c4         // XYZRHW|DIFFUSE|SPECULAR|TEX1

#define D3DTSS_COLOROP                        1
#define D3DTSS_COLORARG1                      2
#define D3DTSS_COLORARG2                      3
#define D3DTSS_ALPHAOP                        4
#define D3DTSS_ALPHAARG1                      5
#define D3DTSS_ALPHAARG2                      6
#define D3DTSS_ADDRESS                       12

#define D3DRENDERSTATE_ZENABLE                7
#define D3DRENDERSTATE_ZWRITEENABLE        0x0e
#define D3DRENDERSTATE_SRCBLEND            0x13
#define D3DRENDERSTATE_DESTBLEND           0x14
#define D3DRENDERSTATE_ALPHABLENDENABLE    0x1b
#define D3DRENDERSTATE_FOGENABLE           0x1c


// The engine's RenderStruct at 0x1008cbcc, as d3dbatchc.cpp and d3dmid.cpp
// model it.  Only +0x14 is used here, twice, and both sites are __cdecl
// with the caller restoring the outgoing area (`add $0x8,%esp` at
// 0x1004baa1, `add $0x18,%esp` at 0x1004e269) -- a varargs console printer.
struct RenderStructCallbacks
{
	uint8   m_Reserved00[0x14];                         // +0x00 .. +0x13
	void  (*ConsolePrint)(const char* pszFormat, ...);  // +0x14
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// ======================================================================= //
//  THE OFF-SCREEN TARGET PAIR, 0x1006a710 and 0x1006a6a0.
//
//  d3dr3a.cpp already reads both: they are ONE-ENTRY CACHES over the globals
//  at 0x100911a0.. and 0x100911ac.., __cdecl, two dwords in, a pointer out.
//  Every use of the answer here is a C++ virtual __thiscall (`mov (%ecx),%eax
//  ; mov 0xNN(%eax),%eax ; call *%eax`), which is why this is spelled with
//  `virtual` and the COM device above is not.  Four slots are reached:
//
//      +0x0c  0x1004de46   no arguments
//      +0x10  0x1004c9a5   (uint32, uint32, uint32, bool)  -- `movb $0x0`
//      +0x14  0x1004c949   (uint32, uint32)
//      +0x18  0x1004c154, 0x1004c92a   (uint32, uint32)
// ======================================================================= //
class RT4b720
{
public:
	virtual void    Slot00();                           // vtbl +0x00
	virtual void    Slot04();                           // vtbl +0x04
	virtual void    Slot08();                           // vtbl +0x08
	virtual void    Slot0c();                           // vtbl +0x0c
	virtual void    Slot10(uint32 a, uint32 b, uint32 c, bool d);         // +0x10
	virtual void    Slot14(uint32 a, uint32 b);         // vtbl +0x14
	virtual void    Slot18(uint32 a, uint32 b);         // vtbl +0x18
};

extern RT4b720* sub_1006a710(uint32 nWidth, uint32 nHeight);
extern RT4b720* sub_1006a6a0(uint32 nWidth, uint32 nHeight);


// ======================================================================= //
//  THE WORLD-QUERY OBJECT AT 0x1008beac.
//
//      mov 0x1008beac,%eax
//      mov 0x6c(%eax),%edx        the vptr of the SUB-OBJECT at +0x6c
//      lea 0x6c(%eax),%eax        `this` is the sub-object, not the whole
//      mov 0xc(%edx),%eax         slot 3
//      call *%eax                 four pushed arguments, %ecx = this
//
//  The second argument is the immediate 0x10073700 -- a code address, so a
//  callback -- and the third is the address of the 32-byte parameter block
//  built at -0x75c.  This is the "collect the polygons around a point"
//  traversal.
// ======================================================================= //
struct ShadowItem;

struct ShadowList                                       // 0x404 bytes
{
	ShadowItem* m_apItems[256];                         // +0x000 .. +0x3ff
	uint32      m_nCount;                               // +0x400
};

struct ShadowQuery                                      // 0x20 bytes
{
	ShadowList* m_pList;                                // +0x00
	Vec4b720       m_vPos;                                 // +0x04
	Vec4b720       m_vEnd;                                 // +0x10
	float       m_fSize;                                // +0x1c
};

class Query4b720
{
public:
	virtual void    Slot00();
	virtual void    Slot04();
	virtual void    Slot08();
	virtual void    Slot0c(Vec4b720* pPos, void* pfnCallback, ShadowQuery* pQuery,
	                       uint32 nFlags);              // vtbl +0x0c
};

struct QueryOwner4b720
{
	uint8       m_pad_00[0x6c];                         // +0x00 .. +0x6b
	Query4b720 m_Query6c;                              // +0x6c
};

extern QueryOwner4b720* g_p1008beac;                        // 0x1008beac

// The traversal callback, by address only: it is reached through an
// immediate, never called from here.
extern uint32 sub_10073700(void* pA, void* pB);


// ======================================================================= //
//  THE WORLD SIDE.
//
//      0x1004e3d0   push %esi ; mov 0x3c(%ecx),%eax ; add $4 ; ret
//                   -- the LOD-table count at +0x3c.
//      0x1004e340   mov 0x38(%ecx),%ecx ; mov 0x8(%esp),%eax
//                   mov (%ecx,%eax,4),%eax ; ret $0x4
//                   -- the LOD table itself, a dword array based at +0x38.
//      0x1004e360   index 0 answers `this`; every other index is
//                   decremented, bounds-checked UNSIGNED against +0x3c and
//                   turned into base+0x38 plus index*36.  (Both already
//                   decoded in d3dbatcha.cpp / d3dr4a.cpp.)
// ======================================================================= //
struct ShadowGeometry;

struct CLodTable
{
	uint8           m_pad_00[0x38];                     // +0x00 .. +0x37
	uint32*         m_pElements;                        // +0x38
	uint32          m_nElements;                        // +0x3c

	ShadowGeometry* sub_1004e360(uint32 nIndex) const;  // 0x1004e360
};

struct CWorldShadow
{
	uint8       m_pad_00[0x38];                         // +0x00 .. +0x37
	uint32*     m_pTable38;                             // +0x38
	uint32      m_nTable3c;                             // +0x3c
	uint8       m_pad_40[0x30];                         // +0x40 .. +0x6f
	Arr4b720 m_Array70;                             // +0x70
	uint8       m_pad_7c[0x10];                         // +0x7c .. +0x8b
	Arr4b720 m_Array8c;                             // +0x8c

	uint32      sub_1004e3d0() const;                   // 0x1004e3d0 -- m_nTable3c
	CLodTable*  sub_1004e340(uint32 nIndex) const;      // 0x1004e340 -- m_pTable38[i]
};

// The scene/light side, at this+0x04.
//   0x100158b0  lea 0x18c(%ecx),%eax    -- &m_v18c
//   0x1004b710  lea 0x198(%ecx),%eax    -- &m_v198
//   0x1004e3a0  (m_n204 & (1 << nBit)) != 0, a 32-bit LTBOOL
struct CSceneShadow
{
	uint8       m_pad_00[0x18c];                        // +0x000 .. +0x18b
	Vec4b720       m_v18c;                                 // +0x18c
	Vec4b720       m_v198;                                 // +0x198
	uint8       m_pad_1a4[0x60];                        // +0x1a4 .. +0x203
	uint32      m_nFlags204;                            // +0x204

	Vec4b720*      sub_100158b0();                         // 0x100158b0
	Vec4b720*      sub_1004b710();                         // 0x1004b710
	LTBOOL      sub_1004e3a0(uint32 nBit) const;        // 0x1004e3a0
};


// The per-LOD geometry block reached through 0x1004e360.  Its own vertex
// array is the CMoArray at +0x00 and its triangle array the one at +0x10:
// 0x1004c409 calls GetSize with %ecx = the block itself and 0x1004c6f8
// calls GetArray with `lea 0x10(%edi),%ecx`.
struct ShadowTri                                        // 0x20 bytes
{
	uint16      m_nIndex[3];                            // +0x00
	uint8       m_pad_06[0x1a];                         // +0x06 .. +0x1f
};

struct ShadowGeometry
{
	Arr4b720 m_Verts00;                             // +0x00
	uint8        m_pad_0c[0x04];                        // +0x0c
	Arr4b720 m_Tris10;                              // +0x10
};


// The polygon record the traversal collects, and the plane hanging off it:
// 0x1004ded0 `mov 0x28(%eax),%eax` then 0x1004ded3 `mov 0x34(%eax),%esi`,
// and the dist is read at +0x0c of what that points at -- an LTPlane.
struct ShadowPoly
{
	uint8       m_pad_00[0x34];                         // +0x00 .. +0x33
	Plane4b720*   m_pPlane34;                             // +0x34
};

struct ShadowItem
{
	uint8       m_pad_00[0x28];                         // +0x00 .. +0x27
	ShadowPoly* m_pPoly28;                              // +0x28
};


// ======================================================================= //
//  THE D3D VERTEX.  DrawPrimitive is called twice with FVF 0x1c4 =
//  D3DFVF_XYZRHW|DIFFUSE|SPECULAR|TEX1, which is 32 bytes -- and 32 is
//  exactly the stride both vertex walks use (`add $0x20` at 0x1004c5e6 and
//  0x1004c8c0, `shl $0x5` at 0x1004c746).  The init loop at 0x1004c1dd
//  writes +0x0c, +0x10, +0x18 and +0x1c and skips +0x14, which is what puts
//  rhw at +0x0c and the two texture coordinates at the end.
// ======================================================================= //
struct ShadowVertex                                     // 0x20 bytes
{
	Vec4b720       m_vPos;                                 // +0x00
	float       m_fRHW;                                 // +0x0c
	uint32      m_nDiffuse;                             // +0x10
	uint32      m_nSpecular;                            // +0x14
	float       m_fU;                                   // +0x18
	float       m_fV;                                   // +0x1c
};


// ======================================================================= //
//  THE FRUSTUM.  The 0x100-byte block at -0x5ac whose address is handed to
//  0x1004e3e4 at 0x1004dfaa.  Every offset below is a store or a load in
//  the shipped bytes; the two plane arrays are stride 0x10 and the matrix
//  is the 64-byte `rep movsl` at 0x1004de23.
// ======================================================================= //
struct ShadowFrustum
{
	Vec4b720       m_vPos;                                 // +0x00
	Vec4b720       m_vRight;                               // +0x0c
	Vec4b720       m_vUp;                                  // +0x18
	Vec4b720       m_vDir;                                 // +0x24
	Plane4b720    m_Plane30;                              // +0x30
	Vec4b720       m_vLightPos;                            // +0x40
	Vec4b720       m_vCorner4c;                            // +0x4c
	float       m_fSize58;                              // +0x58
	float       m_fSize5c;                              // +0x5c
	float       m_mProject[4][4];                       // +0x60
	Plane4b720    m_Planes[6];                            // +0xa0 .. +0xff
};


// ======================================================================= //
//  `this`.
//
//  The caster array is pinned by the two induction pointers of the first
//  loop: one starts at `this` (0x1004b7a5) and is dereferenced at +0x54,
//  the other starts at `this+0x3c` (0x1004b7a2) and is dereferenced at +0,
//  and BOTH step 0x2c.  So the array base is +0x3c, the stride 0x2c, the
//  position at element+0x00 and the float at element+0x18.  The count sits
//  at +0x2fc, and 0x3c + 16*0x2c is 0x2fc exactly -- sixteen elements, the
//  count immediately after them.
// ======================================================================= //
struct ShadowCaster                                     // 0x2c bytes
{
	Vec4b720       m_vPos;                                 // +0x00
	uint8       m_pad_0c[0x0c];                         // +0x0c .. +0x17
	float       m_fRadiusSqr;                           // +0x18
	uint8       m_pad_1c[0x10];                         // +0x1c .. +0x2b
};

class CShadowRenderer
{
public:
	void            sub_1004b720(uint32 nMaxShadows);   // 0x1004b720
	void            sub_1004e3e4(ShadowFrustum* pFrustum, ShadowItem* pItem,
	                             float fRadius);        // 0x1004e3e4
	void            sub_10008fd0(ShadowGeometry* pGeom, ShadowGeometry* pGeom2,
	                             void* pVerts, uint32 n42c, void* pData);
	                                                    // 0x10008fd0

	CWorldShadow*   m_pWorld;                           // +0x000
	CSceneShadow*   m_pScene;                           // +0x004
	uint8           m_pad_008[0x34];                    // +0x008 .. +0x03b
	ShadowCaster    m_Casters[16];                      // +0x03c .. +0x2fb
	uint32          m_nNumCasters;                      // +0x2fc
	uint8           m_pad_300[0x10];                    // +0x300 .. +0x30f
	Mat4b720       m_mView310;                         // +0x310 .. +0x34f
	uint8           m_pad_350[0xdc];                    // +0x350 .. +0x42b
	uint32          m_n42c;                             // +0x42c
	uint8           m_pad_430[0x1c];                    // +0x430 .. +0x44b
	uint32          m_n44c;                             // +0x44c
	uint32          m_n450;                             // +0x450
	uint8           m_pad_454[0x1d4];                   // +0x454 .. +0x627
	Arr4b720    m_Verts628;                         // +0x628
};


// ---------------------------------------------------------------------- //
//  Globals, by address.
// ---------------------------------------------------------------------- //
extern Mat4b720 g_m1008b9dc;                           // 0x1008b9dc
extern uint32    g_n1008ca38;                           // 0x1008ca38 -- debug timing on
extern uint32    g_n1008eec0;                           // 0x1008eec0
extern uint32    g_n1008eee0;                           // 0x1008eee0
extern int32     g_n1008ef00;                           // 0x1008ef00 -- shadow texture size
extern float     g_f1008ef04;                           // 0x1008ef04 -- ...as a float


// ======================================================================= //
//  0x1004b720 -- 11,204 bytes (0x1004b720 .. 0x1004e2e4).  __thiscall, one
//  dword argument (`ret $0x4`, argument read as `0x8(%ebx)` off the aligned
//  entry's saved stack pointer).  One caller, 0x100481ec, which passes 8.
//
//  REGION MAP, each region's evidence with it:
//
//   A  0x1004b748  three counters zeroed, the light position and colour
//                  copied out of m_pScene through 0x100158b0 (+0x18c) and
//                  0x1004b710 (+0x198) with operator= (0x1004b6f0).
//   B  0x1004b7b7  the caster loop.  Transform, distance, keep the nearest
//                  nMaxShadows by an insertion search whose 0x1004b877 body
//                  icl unrolled by four into 0x1004e279.
//   C  0x1004ba50  early-out on nothing found; clamp the console texture
//                  size to [8,256]; reject a non-power-of-two with the
//                  0x100aa2f0 string; clamp the LOD index against
//                  m_Array8c.GetSize()-1 (the cold arm is out of line at
//                  0x1004e2c9, which is why its GetSize is the LAST call in
//                  the shipped sequence).
//   D  0x1004baea  save one texture, seven texture-stage states and six
//                  render states; turn fog off; acquire the render target.
//   E  0x1004bddc  per-shadow loop.  Basis, world query, off-screen clear,
//                  two passes over the LOD geometry (one to project the
//                  vertices, one to draw the triangles), then the frustum.
//   F  0x1004ce42  four planes built inline: cross, inline Norm, inline Dot.
//   G  0x1004d29e  UNDECODED -- see the banner in the body.
//   H  0x1004de46  resolve, restore Z, clip each collected polygon.
//   I  0x1004dffa  restore all fourteen saved states and the texture, then
//                  the 0x100aa330 timing line behind g_n1008ca38.
// ======================================================================= //
void CShadowRenderer::sub_1004b720(uint32 nMaxShadows)
{
	// -- the fixed-size scratch, in the shipped frame's order ----------- //
	Vec4b720           vLightPos;                          // -0x2920
	Vec4b720           vLightColor;                        // -0x2914
	Vec4b720           vCasterPos;                         // -0x2908
	Vec4b720           vDelta;                             // -0x28f0
	Vec4b720           vNormal;                            // -0x28e4
	float           afDist[8];                          // -0x28d8
	Vec4b720           avPos[8];                           // -0x28b8
	Vec4b720           avDir[8];                           // -0x2858
	float           afRadius[8];                        // -0x27f8

	void*           pOldTexture;                        // -0x27d8
	uint32          anStageState[7];                    // -0x27d4 .. -0x27bc
	uint32          anRenderState[6];                   // -0x27b8 .. -0x27a4

	Vec4b720           vPos;                               // -0x27a0
	Vec4b720           vDir;                               // -0x2794
	ShadowList      aLists[8];                          // -0x277c
	ShadowQuery     sQuery;                             // -0x75c
	Vec4b720           vQueryPos;                          // -0x73c
	Vec4b720           vQueryDir;                          // -0x730
	Vec4b720           vUp;                                // -0x708
	Vec4b720           vRight;                             // -0x6f0
	Vec4b720           vBasisUp;                           // -0x6d8
	ShadowVertex    aVerts[4];                          // -0x6cc
	float           fMaxX;                              // -0x64c
	float           fMaxY;                              // -0x648
	Mat4b720       mWorld;                             // -0x604
	Vec4b720           vLightDelta;                        // -0x5b8
	ShadowFrustum   sFrustum;                           // -0x5ac
	Vec4b720           vCorner0;                           // -0x488
	Vec4b720           vCorner1;                           // -0x458
	Vec4b720           vCorner2;                           // -0x428
	Vec4b720           vCorner3;                           // -0x3f8
	Vec4b720           vPlaneNormal;                       // -0x1f8

	// The doubles.  Every one of these is written with `fstpl` and read
	// with `fldl` in the shipped bytes, so they are `double` in the source
	// and not `float`: 0x1004b838, 0x1004bb06, 0x1004bb0c, 0x1004c1d7,
	// 0x1004c434, 0x1004c67c, 0x1004c682, 0x1004c726.
	double          fDist;                              // -0x1b8 (loop B)
	double          fLightRadius;                       // -0x160
	double          fLightSize;                         // -0x148
	double          fTexSizeF;                          // -0x188
	double          fScaleX;                            // -0x198
	double          fScaleY;                            // -0x1a0

	uint32          nFindTicks;                         // -0x2924
	uint32          nSetupTicks;                        // -0x2928
	uint32          nTextureTicks;                      // -0x292c

	uint32          i, j, k, n, m;
	uint32          nNumShadows;                        // -0x1d0
	uint32          nLodIndex;                          // -0x1cc (after loop B)
	int32           nTexSize;                           // -0x158
	RT4b720*  pTarget;                            // -0x150
	RT4b720*  pResolve;                           // -0x168

	// ================================================================== //
	//  REGION A -- 0x1004b748 .. 0x1004b79c
	//
	//  `xor %eax,%eax` then three stores at -0x2924, -0x2928, -0x292c in
	//  that order; they are never touched again until the timing line at
	//  0x1004e232 reads them back in the same order.  The shipped build
	//  has the increments compiled out, so the three zeroes are all that
	//  is left -- and they survive because icl does no dead-store
	//  elimination through memory.
	// ================================================================== //
	nFindTicks    = 0;
	nSetupTicks   = 0;
	nTextureTicks = 0;

	vLightPos.sub_1004b6f0(*m_pScene->sub_100158b0());
	vLightColor.sub_1004b6f0(*m_pScene->sub_1004b710());

	nNumShadows = 0;

	// ================================================================== //
	//  REGION B -- 0x1004b7b7 .. 0x1004ba4a.  The caster loop.
	//
	//  The loop is entered through `test %edx,%edx ; jbe` on +0x2fc and
	//  closed by `cmp 0x2fc(%edx),%eax ; jb` -- an unsigned `for`.
	//
	//  The insertion search at 0x1004b877 breaks on `afDist[j] > fDist`
	//  (`flds afDist[j] ; fcomp %st(1) ; jbe -> continue`) and the
	//  `if (j == nMaxShadows) continue;` after it is the `je 0x1004ba13`
	//  at 0x1004b894.  Its four-times-unrolled tail is 0x1004e279.
	//
	//  The six element writes go through operator[] TWICE each -- source
	//  subscript first, then destination -- which is the twelve calls to
	//  0x1004e2f0 at 0x1004b8a8 .. 0x1004b9cd.
	// ================================================================== //
	for (i = 0; i < m_nNumCasters; i++)
	{
		vCasterPos = m_Casters[i].m_vPos;
		sub_1004e300(&m_mView310, &vCasterPos);

		vDelta  = vLightPos.sub_10015640(vCasterPos);
		fDist   = vDelta.sub_10023330();
		vNormal = vDelta;
		vNormal.sub_10015530(1.0f);

		j = nNumShadows;
		if (j == nMaxShadows)
		{
			for (j = 0; j < nMaxShadows; j++)
			{
				if (afDist[j] > fDist)
					break;
			}

			if (j == nMaxShadows)
				continue;
		}

		*avPos[j].sub_1004e2f0(0) = *vCasterPos.sub_1004e2f0(0);
		*avPos[j].sub_1004e2f0(1) = *vCasterPos.sub_1004e2f0(1);
		*avPos[j].sub_1004e2f0(2) = *vCasterPos.sub_1004e2f0(2);

		*avDir[j].sub_1004e2f0(0) = *vNormal.sub_1004e2f0(0);
		*avDir[j].sub_1004e2f0(1) = *vNormal.sub_1004e2f0(1);
		*avDir[j].sub_1004e2f0(2) = *vNormal.sub_1004e2f0(2);

		afDist[j]   = (float)fDist;
		afRadius[j] = (float)sqrt(m_Casters[i].m_fRadiusSqr);

		if (nNumShadows < nMaxShadows)
			nNumShadows++;
	}

	// ================================================================== //
	//  REGION C -- 0x1004ba50 .. 0x1004bae4.
	//
	//  `test %eax,%eax ; je 0x1004e277` where 0x1004e277 is the two-byte
	//  `jmp` trampoline into the epilogue.
	// ================================================================== //
	if (nNumShadows == 0)
		return;

	// 0x1004ba60: signed compares (`jge`, `jle`), one result register,
	// one store -- an if/else-if/else through a single variable.
	if (g_n1008ef00 < 8)
		nTexSize = 8;
	else if (g_n1008ef00 > 256)
		nTexSize = 256;
	else
		nTexSize = g_n1008ef00;

	// 0x1004ba83: `lea -0x1(%eax),%edx ; test %edx,%eax ; je`.
	if (((nTexSize - 1) & nTexSize) != 0)
	{
		g_pStruct->ConsolePrint("Invalid texture size for shadows -- must be power of 2...");
		g_n1008ef00 = 128;
		nTexSize    = 128;
		g_f1008ef04 = 128.0f;
	}

	// 0x1004babe: the min.  GetSize() is called TWICE -- once inside the
	// comparison at 0x1004bac6 and once inside the taken arm at
	// 0x1004e2d1 -- which is what a `<` / ternary spelled out over the
	// same call gives, and it is why the cold arm's call is the last call
	// in the whole function.
	if ((g_n1008eee0 + m_n44c) < (m_pWorld->m_Array8c.sub_1007ee60() - 1))
		nLodIndex = g_n1008eee0 + m_n44c;
	else
		nLodIndex = m_pWorld->m_Array8c.sub_1007ee60() - 1;

	// ================================================================== //
	//  REGION D -- 0x1004baea .. 0x1004bdca.  Save the device state.
	//
	//  0x1004baf5 multiplies the colour's magnitude by the 2.0f at
	//  0x100aa0b4; both products are spilled as doubles, the doubled one
	//  first.
	// ================================================================== //
	fLightRadius = vLightColor.sub_10023330();
	fLightSize   = fLightRadius * 2.0f;

	g_pD3DDevice->lpVtbl->GetTexture(g_pD3DDevice, 0, &pOldTexture);
	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 0, LTNULL);
	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 1, LTNULL);

	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, D3DTSS_ADDRESS,   &anStageState[0]);
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLORARG1, &anStageState[1]);
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLORARG2, &anStageState[2]);
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLOROP,   &anStageState[3]);
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAARG1, &anStageState[4]);
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAARG2, &anStageState[5]);
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAOP,   &anStageState[6]);

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, D3DRENDERSTATE_ZENABLE,          &anRenderState[0]);
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE,     &anRenderState[1]);
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, &anRenderState[2]);
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, D3DRENDERSTATE_FOGENABLE,        &anRenderState[3]);
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, D3DRENDERSTATE_SRCBLEND,         &anRenderState[4]);
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, D3DRENDERSTATE_DESTBLEND,        &anRenderState[5]);

	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_FOGENABLE, 0);

	pTarget = sub_1006a710(nTexSize, nTexSize);

	// ================================================================== //
	//  REGION E -- 0x1004bddc .. 0x1004dff4.  One pass per shadow.
	// ================================================================== //
	for (k = 0; k < nNumShadows; k++)
	{
		ShadowGeometry* pGeom;
		ShadowGeometry* pGeom2;
		CLodTable*      pTable;
		ShadowVertex*   pVert;
		ShadowVertex*   pVertBase;
		ShadowTri*      pTri;
		ShadowItem*     pItem;
		Plane4b720*     pPlane;
		uint32          nVerts, nTris;
		double          fInv, fHalf;
		float           fShadowRadius;
		double          fDistPos, fDistLight;

		fShadowRadius = afRadius[k];

		// 0x1004be0b: the three subscripts feed the three-float
		// constructor 0x10015480 left to right.
		vPos.sub_10015480(*avPos[k].sub_1004e2f0(0),
		                  *avPos[k].sub_1004e2f0(1),
		                  *avPos[k].sub_1004e2f0(2));

		vDir.sub_10015480(*avDir[k].sub_1004e2f0(0),
		                  *avDir[k].sub_1004e2f0(1),
		                  *avDir[k].sub_1004e2f0(2));

		// 0x1004bed9.  The result at -0x2788 is written and never read
		// again anywhere in the function; it is an initialisation icl
		// gave NRVO and whose variable is dead.  It is kept because the
		// call is in the shipped sequence.
		{
			Vec4b720 vUnused = vLightPos.sub_10015640(vPos);
		}

		sQuery.m_pList = &aLists[k];
		aLists[k].m_nCount = 0;

		vQueryPos = vPos;
		vQueryDir = vDir;

		sQuery.m_vPos  = vQueryPos;
		sQuery.m_vEnd  = sQuery.m_vPos.sub_100155f0(vQueryDir.sub_10015690(fShadowRadius));
		sQuery.m_fSize = (float)fLightSize;

		g_p1008beac->m_Query6c.Slot0c(&vLightPos, (void*)sub_10073700, &sQuery, 0);

		if (aLists[k].m_nCount == 0)
			continue;

		// -- build an orthonormal basis around the light direction ---- //
		//
		//  0x1004c019 constructs (0.0f, 1.0f, 0.0f); 0x1004c060 compares
		//  |dot| against the 0.9999f at 0x100aa0c4 and 0x1004c083 nudges
		//  z by the 0.01f at 0x100aa0c8.
		vUp.sub_10015480(0.0f, 1.0f, 0.0f);

		if (fabs(vUp.sub_10027f50(vDir)) > 0.9999f)
		{
			// ONE call to operator[] at 0x1004c07c, whose %eax is both
			// the `flds` source and the `fstps` destination -- a compound
			// assignment, which evaluates the subscript once.
			*vDir.sub_1004e2f0(2) += 0.01f;
			vDir.sub_10015530(1.0f);
		}

		vRight = vDir.sub_10015590(vUp);
		vRight.sub_10015530(1.0f);

		vBasisUp = vRight.sub_10015590(vDir);
		vBasisUp.sub_10015530(1.0f);

		// -- clear the off-screen target ------------------------------ //
		pTarget->Slot18(0, 0);

		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_ZENABLE,          0);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE,     0);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, 0);

		fTexSizeF = nTexSize;                           // `fildl` at 0x1004c1cc

		// 0x1004c1dd: four iterations, stride 0x20, writing +0x0c, +0x10,
		// +0x18 and +0x1c of each vertex and skipping +0x14.
		for (n = 0; n < 4; n++)
		{
			aVerts[n].m_fRHW     = 0.0f;
			aVerts[n].m_nDiffuse = 0xFFFFFFFF;
			aVerts[n].m_fU       = 0.0f;
			aVerts[n].m_fV       = 0.0f;
		}

		aVerts[0].m_vPos.sub_1000eec0(0.0f,               0.0f,               0.0f);
		aVerts[1].m_vPos.sub_1000eec0(0.0f,               (float)fTexSizeF,   0.0f);
		aVerts[2].m_vPos.sub_1000eec0((float)fTexSizeF,   (float)fTexSizeF,   0.0f);
		aVerts[3].m_vPos.sub_1000eec0((float)fTexSizeF,   0.0f,               0.0f);

		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, D3DPT_TRIANGLEFAN,
		                                    D3DFVF_SHADOWVERTEX, aVerts, 4, 0);

		// 0x1004c2da: a second, separate loop over the same four vertices.
		for (n = 0; n < 4; n++)
			aVerts[n].m_nDiffuse = 0;

		fMaxX = 0.0f;
		fMaxY = 0.0f;

		pVertBase = (ShadowVertex*)m_Verts628.sub_1004fb70();

		Mat4b720 mInverse = g_m1008b9dc.sub_10015740();
		mWorld = mInverse;

		// -- pass one: project every LOD vertex into light space ------ //
		for (n = 0; n < m_pWorld->sub_1004e3d0(); n++)
		{
			pTable = m_pWorld->sub_1004e340(n);
			pGeom  = pTable->sub_1004e360(nLodIndex);

			if (!pGeom)
				continue;

			pGeom2 = pGeom;
			if (m_n450)
				pGeom2 = pTable->sub_1004e360(nLodIndex + 1);

			if (m_pScene->sub_1004e3a0(n))
				continue;

			sub_10008fd0(pGeom, pGeom2, pVertBase, m_n42c,
			             m_pWorld->m_Array70.sub_1004fb70());

			pVert  = pVertBase;
			nVerts = pGeom->m_Verts00.sub_1007ee60();

			if (nVerts != 0)
			{
				// 0x1004c41c: the 0.5f at 0x100aa248 over the light radius.
				fInv = 0.5f / fLightRadius;

				do
				{
					sub_1004e300(&mWorld, &pVert->m_vPos);

					vLightDelta = pVert->m_vPos.sub_10015640(vLightPos);

					*pVert->m_vPos.sub_1004e2f0(0) =
						(float)(vLightDelta.sub_10027f50(vRight) * fInv * fTexSizeF);
					*pVert->m_vPos.sub_1004e2f0(1) =
						(float)(vLightDelta.sub_10027f50(vBasisUp) * fInv * fTexSizeF);
					*pVert->m_vPos.sub_1004e2f0(2) = 0.0f;

					if (fabs(*pVert->m_vPos.sub_1004e2f0(0)) > fMaxX)
						fMaxX = (float)fabs(*pVert->m_vPos.sub_1004e2f0(0));

					if (fabs(*pVert->m_vPos.sub_1004e2f0(1)) > fMaxY)
						fMaxY = (float)fabs(*pVert->m_vPos.sub_1004e2f0(1));

					pVert++;
				}
				while (--nVerts);
			}

			pVertBase += pGeom->m_Verts00.sub_1007ee60();
		}

		// 0x1004c648: (texsize - 2.0f) over twice the extent, both spilled
		// as doubles.  The three 2.0f come from 0x100aa114, 0x100aa10c and
		// 0x100aa110 -- three separate literals in the source.
		fScaleX = (fTexSizeF - 2.0f) / (2.0f * fMaxX);
		fScaleY = (fTexSizeF - 2.0f) / (fMaxY * 2.0f);

		pVertBase = (ShadowVertex*)m_Verts628.sub_1004fb70();

		// -- pass two: draw the projected triangles ------------------- //
		for (n = 0; n < m_pWorld->sub_1004e3d0(); n++)
		{
			pTable = m_pWorld->sub_1004e340(n);
			pGeom  = pTable->sub_1004e360(nLodIndex);

			if (!pGeom)
				continue;

			if (m_pScene->sub_1004e3a0(n))
				continue;

			pTri  = (ShadowTri*)pGeom->m_Tris10.sub_1004fb70();
			nTris = pGeom->m_Tris10.sub_1007ee60();

			if (nTris != 0)
			{
				// 0x1004c70e: the 0.5f at 0x100aa240.
				fHalf = 0.5f * fTexSizeF;

				do
				{
					aVerts[0].m_vPos = pVertBase[pTri->m_nIndex[0]].m_vPos;
					aVerts[1].m_vPos = pVertBase[pTri->m_nIndex[1]].m_vPos;
					aVerts[2].m_vPos = pVertBase[pTri->m_nIndex[2]].m_vPos;

					aVerts[0].m_vPos.x = (float)(fScaleX * aVerts[0].m_vPos.x);
					aVerts[1].m_vPos.x = (float)(fScaleX * aVerts[1].m_vPos.x);
					aVerts[2].m_vPos.x = (float)(fScaleX * aVerts[2].m_vPos.x);

					aVerts[0].m_vPos.y = (float)(aVerts[0].m_vPos.y * fScaleY);
					aVerts[1].m_vPos.y = (float)(aVerts[1].m_vPos.y * fScaleY);
					aVerts[2].m_vPos.y = (float)(aVerts[2].m_vPos.y * fScaleY);

					aVerts[0].m_vPos.x = (float)(aVerts[0].m_vPos.x + fHalf);
					aVerts[1].m_vPos.x = (float)(aVerts[1].m_vPos.x + fHalf);
					aVerts[2].m_vPos.x = (float)(aVerts[2].m_vPos.x + fHalf);

					aVerts[0].m_vPos.y = (float)(fHalf + aVerts[0].m_vPos.y);
					aVerts[1].m_vPos.y = (float)(fHalf + aVerts[1].m_vPos.y);
					aVerts[2].m_vPos.y = (float)(fHalf + aVerts[2].m_vPos.y);

					g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice,
					                                    D3DPT_TRIANGLELIST,
					                                    D3DFVF_SHADOWVERTEX,
					                                    aVerts, 3, 0);

					pTri++;
				}
				while (--nTris);
			}

			pVertBase += pGeom->m_Verts00.sub_1007ee60();
		}

		// -- resolve the target -------------------------------------- //
		pResolve = sub_1006a6a0(nTexSize, nTexSize);
		pResolve->Slot18(0, 0);
		pTarget->Slot14(0, 0);

		if (g_n1008eec0 != 0)
		{
			// 0x1004c966: (nNumShadows - k - 1) * nTexSize, and the
			// `k + 1` is common with the loop's own increment.
			pResolve->Slot10(nTexSize, (nNumShadows - k - 1) * nTexSize,
			                 0xFFFFFFFF, false);
		}

		// ============================================================== //
		//  0x1004c9aa -- the projector.
		// ============================================================== //
		sFrustum.m_vRight  = vRight;
		sFrustum.m_vUp     = vBasisUp;
		sFrustum.m_vDir    = vDir;
		sFrustum.m_vPos    = vPos;
		sFrustum.m_fSize58 = (float)fLightSize;
		sFrustum.m_fSize5c = (float)fLightSize;

		sFrustum.m_Plane30.m_Normal = vDir;
		sFrustum.m_vLightPos        = vLightPos;
		sFrustum.m_Plane30.m_Dist   =
			sFrustum.m_Plane30.m_Normal.sub_10027f50(sFrustum.m_vLightPos);

		// The four corners of the light quad.  Each is three calls --
		// operator*(T), operator*(T), then operator+ or operator- -- and
		// the 0.5f is the 0x3f000000 immediate at 0x1004ca84 and friends.
		vCorner0 = sFrustum.m_vLightPos.sub_10015640(
		               sFrustum.m_vRight.sub_10015690((float)fLightSize).sub_10015690(0.5f));
		vCorner1 = sFrustum.m_vLightPos.sub_100155f0(
		               sFrustum.m_vRight.sub_10015690((float)fLightSize).sub_10015690(0.5f));
		vCorner2 = sFrustum.m_vLightPos.sub_100155f0(
		               sFrustum.m_vUp.sub_10015690((float)fLightSize).sub_10015690(0.5f));
		vCorner3 = sFrustum.m_vLightPos.sub_10015640(
		               sFrustum.m_vUp.sub_10015690((float)fLightSize).sub_10015690(0.5f));

		sFrustum.m_vCorner4c =
			sFrustum.m_vLightPos
				.sub_10015640(sFrustum.m_vRight.sub_10015690((float)fLightSize).sub_10015690(0.5f))
				.sub_10015640(sFrustum.m_vUp.sub_10015690((float)fLightSize).sub_10015690(0.5f));

		// The near and far caps.  0x1004cd32 copies m_vDir into plane 0 and
		// 0x1004cdc1 negates it into plane 1.
		sFrustum.m_Planes[0].m_Normal = sFrustum.m_vDir;
		sFrustum.m_Planes[0].m_Dist   = sFrustum.m_Planes[0].m_Normal.sub_10027f50(
			vLightPos.sub_10015640(vDir.sub_10015690((float)fLightRadius)));

		sFrustum.m_Planes[1].m_Normal = sFrustum.m_vDir.sub_10023350();
		sFrustum.m_Planes[1].m_Dist   = sFrustum.m_Planes[1].m_Normal.sub_10027f50(
			vLightPos.sub_100155f0(sFrustum.m_vDir.sub_10015690(fShadowRadius)));

		// ============================================================== //
		//  REGION F -- 0x1004ce42 .. 0x1004d29e.  Four side planes.
		//
		//  These four are the ONLY places in the function where the header
		//  bodies come out inline, so they are written out by hand rather
		//  than through the sub_ methods -- calling operator- / Cross /
		//  Norm / Dot here would add sixteen calls the shipped code does
		//  not make.  What is inlined is exactly ltvector.h:
		//
		//    * Cross(v) = (v.y*z - v.z*y, v.z*x - v.x*z, v.x*y - v.y*x);
		//      the sign alternates between the pairs, which is the two
		//      operand orders below.
		//    * Norm(1.0f): `mag = Mag(); if (mag == 0.0f) return;` -- the
		//      compare is `flds <0.0f> ; fcomp %st(1) ; je`, and the four
		//      zeroes are four separate literals (0x100aa138, 13c, 140,
		//      144), as are the four 1.0f (0x100aa23c, 234, 22c, 224).
		//      The sqrt is a CALL to 0x1007fc98 in all four.
		//    * Dot(vec): `(x*v.x) + (y*v.y) + (z*v.z)`, accumulated
		//      last-term-backwards, z first.
		// ============================================================== //
		{
			Vec4b720 vA;
			Vec4b720 vN;
			float fMag;
			float fInvMag;

			// plane 2 -- (corner0 - pos) x up
			vA.x = vCorner0.x - sFrustum.m_vPos.x;
			vA.y = vCorner0.y - sFrustum.m_vPos.y;
			vA.z = vCorner0.z - sFrustum.m_vPos.z;
			vN.x = (sFrustum.m_vUp.y * vA.z) - (sFrustum.m_vUp.z * vA.y);
			vN.y = (sFrustum.m_vUp.z * vA.x) - (sFrustum.m_vUp.x * vA.z);
			vN.z = (sFrustum.m_vUp.x * vA.y) - (sFrustum.m_vUp.y * vA.x);
			sFrustum.m_Planes[2].m_Normal = vN;
			fMag = (float)sqrt((sFrustum.m_Planes[2].m_Normal.x * sFrustum.m_Planes[2].m_Normal.x) +
			                   (sFrustum.m_Planes[2].m_Normal.y * sFrustum.m_Planes[2].m_Normal.y) +
			                   (sFrustum.m_Planes[2].m_Normal.z * sFrustum.m_Planes[2].m_Normal.z));
			if (fMag != 0.0f)
			{
				fInvMag = 1.0f / fMag;
				sFrustum.m_Planes[2].m_Normal.x = sFrustum.m_Planes[2].m_Normal.x * fInvMag;
				sFrustum.m_Planes[2].m_Normal.y = sFrustum.m_Planes[2].m_Normal.y * fInvMag;
				sFrustum.m_Planes[2].m_Normal.z = sFrustum.m_Planes[2].m_Normal.z * fInvMag;
			}
			sFrustum.m_Planes[2].m_Dist =
				(sFrustum.m_Planes[2].m_Normal.x * sFrustum.m_vPos.x) +
				(sFrustum.m_Planes[2].m_Normal.y * sFrustum.m_vPos.y) +
				(sFrustum.m_Planes[2].m_Normal.z * sFrustum.m_vPos.z);

			// plane 3 -- up x (corner1 - pos)
			vA.x = vCorner1.x - sFrustum.m_vPos.x;
			vA.y = vCorner1.y - sFrustum.m_vPos.y;
			vA.z = vCorner1.z - sFrustum.m_vPos.z;
			vN.x = (vA.y * sFrustum.m_vUp.z) - (vA.z * sFrustum.m_vUp.y);
			vN.y = (vA.z * sFrustum.m_vUp.x) - (vA.x * sFrustum.m_vUp.z);
			vN.z = (vA.x * sFrustum.m_vUp.y) - (vA.y * sFrustum.m_vUp.x);
			sFrustum.m_Planes[3].m_Normal = vN;
			fMag = (float)sqrt((sFrustum.m_Planes[3].m_Normal.x * sFrustum.m_Planes[3].m_Normal.x) +
			                   (sFrustum.m_Planes[3].m_Normal.y * sFrustum.m_Planes[3].m_Normal.y) +
			                   (sFrustum.m_Planes[3].m_Normal.z * sFrustum.m_Planes[3].m_Normal.z));
			if (fMag != 0.0f)
			{
				fInvMag = 1.0f / fMag;
				sFrustum.m_Planes[3].m_Normal.x = sFrustum.m_Planes[3].m_Normal.x * fInvMag;
				sFrustum.m_Planes[3].m_Normal.y = sFrustum.m_Planes[3].m_Normal.y * fInvMag;
				sFrustum.m_Planes[3].m_Normal.z = sFrustum.m_Planes[3].m_Normal.z * fInvMag;
			}
			sFrustum.m_Planes[3].m_Dist =
				(sFrustum.m_Planes[3].m_Normal.x * sFrustum.m_vPos.x) +
				(sFrustum.m_Planes[3].m_Normal.y * sFrustum.m_vPos.y) +
				(sFrustum.m_Planes[3].m_Normal.z * sFrustum.m_vPos.z);

			// plane 4 -- (corner2 - pos) x right
			vA.x = vCorner2.x - sFrustum.m_vPos.x;
			vA.y = vCorner2.y - sFrustum.m_vPos.y;
			vA.z = vCorner2.z - sFrustum.m_vPos.z;
			vN.x = (sFrustum.m_vRight.y * vA.z) - (sFrustum.m_vRight.z * vA.y);
			vN.y = (sFrustum.m_vRight.z * vA.x) - (sFrustum.m_vRight.x * vA.z);
			vN.z = (sFrustum.m_vRight.x * vA.y) - (sFrustum.m_vRight.y * vA.x);
			sFrustum.m_Planes[4].m_Normal = vN;
			fMag = (float)sqrt((sFrustum.m_Planes[4].m_Normal.x * sFrustum.m_Planes[4].m_Normal.x) +
			                   (sFrustum.m_Planes[4].m_Normal.y * sFrustum.m_Planes[4].m_Normal.y) +
			                   (sFrustum.m_Planes[4].m_Normal.z * sFrustum.m_Planes[4].m_Normal.z));
			if (fMag != 0.0f)
			{
				fInvMag = 1.0f / fMag;
				sFrustum.m_Planes[4].m_Normal.x = sFrustum.m_Planes[4].m_Normal.x * fInvMag;
				sFrustum.m_Planes[4].m_Normal.y = sFrustum.m_Planes[4].m_Normal.y * fInvMag;
				sFrustum.m_Planes[4].m_Normal.z = sFrustum.m_Planes[4].m_Normal.z * fInvMag;
			}
			sFrustum.m_Planes[4].m_Dist =
				(sFrustum.m_Planes[4].m_Normal.x * sFrustum.m_vPos.x) +
				(sFrustum.m_Planes[4].m_Normal.y * sFrustum.m_vPos.y) +
				(sFrustum.m_Planes[4].m_Normal.z * sFrustum.m_vPos.z);

			// plane 5 -- right x (corner3 - pos)
			vA.x = vCorner3.x - sFrustum.m_vPos.x;
			vA.y = vCorner3.y - sFrustum.m_vPos.y;
			vA.z = vCorner3.z - sFrustum.m_vPos.z;
			vN.x = (vA.y * sFrustum.m_vRight.z) - (vA.z * sFrustum.m_vRight.y);
			vN.y = (vA.z * sFrustum.m_vRight.x) - (vA.x * sFrustum.m_vRight.z);
			vN.z = (vA.x * sFrustum.m_vRight.y) - (vA.y * sFrustum.m_vRight.x);
			sFrustum.m_Planes[5].m_Normal = vN;
			fMag = (float)sqrt((sFrustum.m_Planes[5].m_Normal.x * sFrustum.m_Planes[5].m_Normal.x) +
			                   (sFrustum.m_Planes[5].m_Normal.y * sFrustum.m_Planes[5].m_Normal.y) +
			                   (sFrustum.m_Planes[5].m_Normal.z * sFrustum.m_Planes[5].m_Normal.z));
			if (fMag != 0.0f)
			{
				fInvMag = 1.0f / fMag;
				sFrustum.m_Planes[5].m_Normal.x = sFrustum.m_Planes[5].m_Normal.x * fInvMag;
				sFrustum.m_Planes[5].m_Normal.y = sFrustum.m_Planes[5].m_Normal.y * fInvMag;
				sFrustum.m_Planes[5].m_Normal.z = sFrustum.m_Planes[5].m_Normal.z * fInvMag;
			}
			sFrustum.m_Planes[5].m_Dist =
				(sFrustum.m_Planes[5].m_Normal.x * sFrustum.m_vPos.x) +
				(sFrustum.m_Planes[5].m_Normal.y * sFrustum.m_vPos.y) +
				(sFrustum.m_Planes[5].m_Normal.z * sFrustum.m_vPos.z);
		}

		// ============================================================== //
		//  UNDECODED: 0x1004d29e .. 0x1004de44
		//
		//  About 3.0 KB of straight-line x87 with NO calls in it -- the
		//  reason it can be left open without moving the acceptance test.
		//
		//  What can be read off it, and is offered only as a description
		//  of the gap, not as reconstructed source:
		//
		//    * 0x1004d2a0 .. 0x1004d324 is m_Plane30.DistTo(m_vPos)
		//      spelled out: `m_Normal.Dot(vec) - m_Dist` with BOTH the
		//      normal and the position copied into compiler temporaries at
		//      -0x2934 .. -0x2950 with integer `mov`s -- icl's by-value
		//      argument copies of a float lvalue.
		//    * 0x1004d370 .. 0x1004d4e2 fills a 0x40-byte block through
		//      the pointer parked at -0x2960 (`lea -0x338(%ebp),%edx`),
		//      writing +0x04 .. +0x3c from negated products of those same
		//      temporaries -- a skew/cross matrix.
		//    * 0x1004d4e5 onward divides by the frustum's +0x54/+0x58/+0x5c
		//      floats and by fScaleX/fScaleY (-0x198 / -0x1a0), zero-fills
		//      a second 4x4 at -0x2b4 .. -0x260 with 0x3f800000 on the
		//      diagonal, and then runs three 4x4 multiplies whose many
		//      `fadds` of the zero constants at 0x100aa1b0 .. 0x100aa208
		//      are the untouched entries of those identity matrices.
		//    * 0x1004de23 copies the 64-byte product from -0x238 into the
		//      frustum's +0x60.
		//
		//  Reconstructing it means recovering four matrix expressions'
		//  exact association order out of a single interleaved x87
		//  schedule; that is left for a measured build.
		// ============================================================== //

		// ============================================================== //
		//  REGION H -- 0x1004de46 .. 0x1004dff4.
		// ============================================================== //
		pResolve->Slot0c();

		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_ZENABLE,      1);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE, 0);

		for (m = 0; m < aLists[k].m_nCount; m++)
		{
			pItem  = aLists[k].m_apItems[m];
			pPlane = pItem->m_pPoly28->m_pPlane34;

			vPlaneNormal = pPlane->m_Normal;

			// 0x1004df26: both distances are the inlined
			// `m_Normal.Dot(v) - m_Dist`, with the four 0.0f at
			// 0x100aa210, 0x100aa214, 0x100aa218 and 0x100aa21c --
			// four separate literals, i.e. four comparisons in source.
			fDistPos = ((vPos.x * vPlaneNormal.x) +
			            (vPos.y * vPlaneNormal.y) +
			            (vPos.z * vPlaneNormal.z)) - pPlane->m_Dist;

			fDistLight = ((vPlaneNormal.x * vLightPos.x) +
			              (vPlaneNormal.y * vLightPos.y) +
			              (vPlaneNormal.z * vLightPos.z)) - pPlane->m_Dist;

			if (((fDistPos < 0.0f) && (fDistLight < 0.0f)) ||
			    ((fDistPos > 0.0f) && (fDistLight > 0.0f)))
			{
				sub_1004e3e4(&sFrustum, pItem, fShadowRadius);
			}
		}
	}

	// ================================================================== //
	//  REGION I -- 0x1004dffa .. 0x1004e274.  Put everything back.
	//
	//  The seven texture-stage restores replay the SAVE order (12, 2, 3,
	//  1, 5, 6, 4) and the six render-state restores replay theirs
	//  (7, 0x0e, 0x1b, 0x1c, 0x13, 0x14), then the texture goes back.
	// ================================================================== //
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_ADDRESS,   anStageState[0]);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLORARG1, anStageState[1]);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLORARG2, anStageState[2]);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLOROP,   anStageState[3]);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAARG1, anStageState[4]);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAARG2, anStageState[5]);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAOP,   anStageState[6]);

	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_ZENABLE,          anRenderState[0]);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE,     anRenderState[1]);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, anRenderState[2]);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_FOGENABLE,        anRenderState[3]);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_SRCBLEND,         anRenderState[4]);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_DESTBLEND,        anRenderState[5]);

	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 0, pOldTexture);

	if (g_n1008ca38)
	{
		g_pStruct->ConsolePrint(
			"Shadow -> Find: %d, Setup: %d, Texture : %d,  Draw: %d, Total: %d ticks",
			nFindTicks, nSetupTicks, nTextureTicks, 0, 0);
	}
}
