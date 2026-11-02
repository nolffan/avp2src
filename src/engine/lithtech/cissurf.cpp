// *********************************************************************** //
//
//  MODULE:   CIS (.CPP)  -- the client interface's SURFACE half.
//            (file name ours; the module's own prefix is `cis_`)
//
//  WHAT NAMES THIS MODULE.  Two `static char*` error-name variables inside
//  the run dereference to FUNCTION NAMES, which is the de_error.h
//  self-naming case that outranks every other instrument:
//
//      0x004a8080 -> 0x004a80c8  "cis_MaskedDraw"
//      0x004a8084 -> 0x004a80ac  "cis_DoDrawSurfaceToSurface"
//
//  0x004a8080 is pushed by 0x0040d8a0 and 0x004a8084 by 0x0040d290, so
//  those two addresses ARE those two functions.  src/engine/lithtech/
//  fntdraw.cpp had already read 0x0040c130 as `cis_CreateSurface` and
//  src/engine/lithtech/diblit.cpp reconstructs `cis_LockSurface`
//  (0x0043acb0) and `cis_DrawWarp*` under the same prefix, so the whole
//  0x0040c000..0x0040f2ff run is one `cis_` translation unit -- the bodies
//  behind clientinterface.cpp's ILTClient surface entry points.
//
//  Build /O2 (/Og /Oi /Ot /Oy /Ob1).  Every offset, constant and branch
//  below is cited by the shipped address it was read at, in
//  the retail lithtech.exe.  There is no /O1 twin: none of this is
//  compiled into server.dll, so the oracle does not apply to any row here.
//
// *********************************************************************** //

// ----------------------------------------------------------------------- //
//  ROW-BY-ROW VERDICT  (11 rows, 2,158 bytes as enumerated)
// ----------------------------------------------------------------------- //
//
//  addr        size  what it is                       verdict / confidence
//  ----------  ----  -------------------------------  --------------------
//  0x0040c130   301  cis_CreateSurface                WRITTEN  identical
//  0x0040c260    58  cis_CreateSurfaceFromDIB         WRITTEN  high
//  0x0040c2a0   516  cis_DrawDIBToSurface             WRITTEN  low-medium
//  0x0040cc80    93  cis_IsColumnColor                WRITTEN  high
//  0x0040cce0    87  cis_IsRowColor                   WRITTEN  high
//  0x0040cf30    96  cis_LoadDIB                      WRITTEN  high
//  0x0040d010   260  ci_DrawBitmapToSurface           WRITTEN  identical
//  0x0040d120   157  ci_DrawSurfaceSolidColor         WRITTEN  identical
//  0x0040d1c0   202  DrawSurfaceSolidColor callback   WRITTEN  identical
//  0x0040d290   963  cis_DoDrawSurfaceToSurface       WRITTEN  identical
//  0x0040d660   162  DrawSurfaceToSurface callback    WRITTEN  identical
//  0x0040d710   191  ...Transparent callback          WRITTEN  identical
//  0x0040d810   140  ci_DrawSurfaceMasked             WRITTEN  identical
//  0x0040d8a0   254  cis_MaskedDraw, FIRST HALF  ---+ MERGED, 679 bytes
//  0x0040d99e   425  cis_MaskedDraw, SECOND HALF ---+ WRITTEN  near 671/679
//  0x0040ed50   190  ci_GetEngineHook                 WRITTEN  identical
//  0x0040ee10    34  cis_Term  (the THUNK is 0x0040bfd0, not this)  high
//  0x0040ee40   100  cis_FreeAllSurfaces              WRITTEN  medium-high
//  0x0040ef30   293  surface_CopyIn                   WRITTEN  medium
//  0x00489510   107  fnt_Term (home: fntdraw.cpp)     WRITTEN  identical
//  0x00489580    47  fnt_FreeAllFonts (home: fntdraw) WRITTEN  identical
//
//  All eleven were checked against the
//  src/engine tree and the decoded symbol map: every one is
//  `todo` and none has a body anywhere.  No screen defect found.
//
// ----------------------------------------------------------------------- //

#include <windows.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltrect.h"
#include "cluglob.h"                // g_CluClientGlob, 0x004b0ac0


// ----------------------------------------------------------------------- //
//  Views.  Offsets that overlap src/engine/lithtech/clientmgr.h,
//  clientblit.cpp, clientmgr2.cpp and diblit.cpp agree with them.
// ----------------------------------------------------------------------- //

// clientmgr.h's CSurface plus the three fields this run proves:
//   +0x00 is the struct-bank free-list link (0x0040ee79 stores the bank's
//         head through the surface pointer itself),
//   +0x04 is the circular list link clientmgr2.cpp's surface_SaveAll walks
//         (0x0040ee57 `mov 0x4(%edi),%edi`),
//   +0x08 is the system-memory copy surface_CopyOut fills (0x0040f16c) and
//         surface_CopyIn releases (0x0040ef21 -> 0x0040c5d0).
class CSurfaceQ40C
{
public:
	CSurfaceQ40C*  m_pPrev;                  // 0x000  0x0040c215; also the
	                                         //        bank's free link, 0x0040ee79
	CSurfaceQ40C*  m_pNext;                  // 0x004  0x0040ee57
	void*          m_pSysMem;                // 0x008  0x0040efc0 / 0x0040c5d5
	void*          m_pRenderData;            // 0x00c  0x0040c5a5
	uint32         m_nWidth;                 // 0x010  0x0040d8b2
	uint32         m_nHeight;                // 0x014  0x0040d8c1
	uint32         m_nPitch;                 // 0x018  0x0040c3c2
	uint32         m_nFlags;                 // 0x01c  0x0040c3db
	HLTCOLOR       m_hTransColor;            // 0x020  0x0040f031
	void*          m_pUserData;              // 0x024
	float          m_fAlpha;                 // 0x028
};

// The render table, as this file reaches it -- all through the g_pRender
// pointer at 0x004b17a4, never virtuals.  Slot names are clientblit.cpp's;
// the two this run adds are +0xc8 and +0xd0, read off cis_CreateSurface
// (0x0040c171 / 0x0040c199) and surface_CopyIn (0x0040ef56 / 0x0040ef83).
class RenderStructQ40C
{
public:
	uint8   m_pad_00[0xc8];
	void*  (*CreateSurface)(uint32 nWidth, uint32 nHeight);         // 0x0c8
	void   (*DestroySurface)(void* pRenderData);                    // 0x0cc
	void   (*GetSurfaceInfo)(void* pRenderData, uint32* pWidth,
	                         uint32* pHeight, uint32* pPitch);      // 0x0d0
	void*  (*LockSurface)(void* pRenderData);                       // 0x0d4
	void   (*UnlockSurface)(void* pRenderData);                     // 0x0d8
	LTBOOL (*OptimizeSurface)(void* pRenderData, HLTCOLOR hTrans);  // 0x0dc
	void   (*UnoptimizeSurface)(void* pRenderData);                 // 0x0e0
	LTBOOL (*LockScreen)(int nLeft, int nTop, int nRight, int nBottom,
	                     void** ppPixels, uint32* pPitch);          // 0x0e4
	void   (*UnlockScreen)();                                       // 0x0e8
};

// 0x004b1758 -- the screen PFormat (fntdraw.cpp / lttexmod.cpp already type
// it).  0x38 bytes: 0x004b1758..0x004b178f, which is exactly the stride
// between the two PFormats BlitRequest embeds (0x0045f248/0x0045f24e put
// the same vptr 0x004a1ab4 at this+0x28 and this+0x60).  Only +0x04 is
// reached from here.
class PFormatQ40C
{
public:
	// The vptr (0x004a1ab4) is a real one: 0x0040c0f0 / 0x0040c110 are the
	// dynamic initialisers that store it into g_ScreenFormat and
	// g_SavedScreenFormatQ40C.  Its one slot is lttexmod.cpp's PFormat::Init
	// (0x0045f0e0); declared, not defined -- the link resolves it there.
	virtual void Init(uint32 nBitDepth, uint32 aMask, uint32 rMask,
	                  uint32 gMask, uint32 bMask);          // 0x0045f0e0

	uint32 m_nBitDepth;                      // 0x04  -> 0x004b175c;
	                                         //       2 == 16-bit, 3 == 32-bit
	uint8  m_pad_08[0x38 - 0x08];
};

// The blitter's request object, 0x98 bytes.  Read straight off its
// constructor at 0x0045f240 and the two fillers here:
//
//      0x0045f254  [this+0x00] = this+0x28     m_pSrcFormat  (embedded)
//      0x0045f261  [this+0x04] = 0             m_pSrc
//      0x0045f264  [this+0x08] = -1            m_nSrcPitch
//      0x0045f25e  [this+0x0c] = 0             m_pSrcPalette
//      0x0045f256  [this+0x10] = this+0x60     m_pDestFormat (embedded)
//      0x0045f267  [this+0x14] = 0             m_pDest
//      0x0045f26a  [this+0x18] = -1            m_nDestPitch
//      0x0045f26d  [this+0x1c] = -1            m_nWidth
//      0x0045f270  [this+0x20] = -1            m_nHeight
//      0x0045f273  [this+0x24] = 0             m_pDestPalette
//      0x0045f248  [this+0x28] = 0x004a1ab4    m_SrcFormat  (a PFormat)
//      0x0045f24e  [this+0x60] = 0x004a1ab4    m_DestFormat (a PFormat)
//
//  0x0040ef30 reserves 0xa0 for this plus two dwords, so 0x98 is the size.
//  The blit itself is 0x0045f3d0: it maps both formats through 0x004476b0
//  and indexes an 8x8 table of blitters at 0x004acdb8, so the two PFormat*
//  fields are what pick the converter.  NOT DEFINED HERE -- the class
//  belongs to the blitter's own TU; this is the caller-side view.
class BlitRequestQ40C
{
public:
	BlitRequestQ40C();                       // 0x0045f240

	PFormatQ40C* m_pSrcFormat;               // 0x00
	void*        m_pSrc;                     // 0x04
	uint32       m_nSrcPitch;                // 0x08
	void*        m_pSrcPalette;              // 0x0c
	PFormatQ40C* m_pDestFormat;              // 0x10
	void*        m_pDest;                    // 0x14
	uint32       m_nDestPitch;               // 0x18
	int32        m_nWidth;                   // 0x1c
	int32        m_nHeight;                  // 0x20
	void*        m_pDestPalette;             // 0x24
	uint8        m_SrcFormat[0x38];          // 0x28
	uint8        m_DestFormat[0x38];         // 0x60
};

// The blitter object itself: `*(char**)0x004b17e0 + 0x1618`, i.e. a
// sub-object of CClientMgr at +0x1618 (0x0040c120, two instructions).
// clientinterface.cpp already calls this `sub_40c120()`.
class CBlitterQ40C
{
public:
	LTRESULT Blt(BlitRequestQ40C* pRequest);                   // 0x0045f3d0
	void     ConvertColor(PFormatQ40C* pFormat, HLTCOLOR hColor,
	                      uint32* pOut);                       // 0x0045f5e0
};

// The 0x45c-byte DIB the bitmap loader builds on the stack.  cltcursor.cpp
// found the two vptrs (0x004a1ab4 at +0x000, 0x004a1ab8 at +0x444) and the
// three dwords at +0x438..+0x440; this run adds what they are.  The FIRST
// vptr is the PFormat one, and 0x0040c437 hands `pDib` itself to
// BlitRequest::m_pSrcFormat -- so the DIB IS-A PFormat.  0x0040c451 hands
// pDib+0x38 to m_pSrcPalette, and 0x0040c42d adds the source rect's LEFT to
// the bit pointer with NO bytes-per-pixel multiply, so the DIB is 8-bit
// paletted and the palette is 256 entries filling 0x38..0x437.
// The DIB's pixel array: collection.h's CMoArray<uint8> shape (vptr,
// buffer, count, cache size, wanted cache -- 0x14 bytes, 0x444..0x457,
// which is the 0x458 frame 0x0040d010 reserves).  Its teardown is INLINE
// at every stack DIB's exit (0x0040d032..0x0040d09a): re-store the vptr
// 0x004a1ab8, then `if (m_pArray)` BaseDelete(&g_DefAlloc, m_pArray,
// m_nElements + m_nCache) through the out-of-line 0x004152b0.  A view in
// clientmgr3.cpp's CCmgArray04 manner: the vptr is a plain field and the
// destructor re-stores it by hand (a real virtual would drag a vtable and
// its slots into this TU).
class LAlloc;
void BaseDelete(LAlloc* pAlloc, uint8* pArray, uint32 nElements);        // 0x004152b0
extern uint8 g_DefAllocQ40C[];                                           // 0x004b4e88
extern void* g_CMoByteArrayVtblQ40C[];                                   // 0x004a1ab8

class CDibDataQ40C
{
public:
	~CDibDataQ40C()
	{
		m_pVtable = g_CMoByteArrayVtblQ40C;
		if (m_pArray)
			BaseDelete((LAlloc*)g_DefAllocQ40C, m_pArray, m_nElements + m_nCache);
	}

	void**  m_pVtable;                       // 0x00
	uint8*  m_pArray;                        // 0x04
	uint32  m_nElements;                     // 0x08
	uint32  m_nCache;                        // 0x0c
	uint32  m_nWantedCache;                  // 0x10
};

class CDIBQ40C
{
public:
	CDIBQ40C();                              // 0x0043de60

	uint8   m_PFormat[0x38];                 // 0x000  vptr 0x004a1ab4
	uint32  m_Palette[256];                  // 0x038  0x0040c43f/0x0040c451
	uint32  m_nWidth;                        // 0x438  0x0040c266
	uint32  m_nHeight;                       // 0x43c  0x0040c266
	uint32  m_nPitch;                        // 0x440  0x0040c407
	CDibDataQ40C m_Data;                     // 0x444  vptr 0x004a1ab8; the
	                                         //        bits at 0x448 (0x0040c417)
};

// stdlith's struct bank, as clientwin.cpp models it.  0x004b1600 is the
// surface bank: +0x08 is the grow count (0x0040c1ab) and +0x18 the free
// list (0x0040c1ca / 0x0040ee70).
class CStructBankQ40C
{
public:
	uint32 m_StructSize;                     // 0x00
	uint32 m_Unknown04;                      // 0x04
	uint32 m_GrowCount;                      // 0x08
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                      // 0x18
};

// clientmgr.h's, as far as this file reaches it.
class CClientMgrQ40C
{
public:
	uint8            m_pad_00[0x1534];
	class CFileMgrQ40C* m_pFileMgr;          // 0x1534  0x0040cf50
	uint8            m_pad_1538[0x1618 - 0x1538];
	CBlitterQ40C     m_Blitter;              // 0x1618  0x0040c125
};


// ----------------------------------------------------------------------- //
//  Globals.  Every one of these is in .bss (not in any initialised
//  section).
//
//  0x004b15c0..0x004b17a7 is THIS TU's .bss block, so the ones in it are
//  DEFINED here (every other TU's spelling of these addresses is an extern
//  of, or a link alias onto, these definitions).  The block's order in the
//  image is 15c0 saved format, 15f8 mask surface, 1600 surface bank, 161c
//  transparent colour, 1620 screen surface, 164c mask table, 1750 fill
//  colour, 1758 screen format, 1790/1794 bytes per pixel, 1798 surface
//  list, 17a4 g_pRender.
// ----------------------------------------------------------------------- //

RenderStructQ40C*        g_pRender;                    // 0x004b17a4
CSurfaceQ40C             g_ScreenSurface;              // 0x004b1620
// The screen size (0x004b1630 / 0x004b1634) is not a global of its own: it
// is g_ScreenSurface's m_nWidth / m_nHeight (+0x10 / +0x14).
uint32                   g_nBytesPerPixel;             // 0x004b1794
extern PFormatQ40C       g_ScreenFormat;               // 0x004b1758 (defined below)
extern CClientMgrQ40C*   g_pClientMgr;                 // 0x004b17e0

// The surface pool and its list.
CStructBankQ40C          g_SurfaceBankQ40C;            // 0x004b1600

// THE SURFACE LIST IS A FILE-SCOPE OBJECT, not two loose globals.
// 0x004b1798 holds the vptr 0x004a1a84, 0x004b179c the count and
// 0x004b17a0 the head (clientmgr2.cpp's g_pSurfaceList).  Its dynamic
// initialiser is this TU's first code: 0x0040c080 (call ctor; jmp reg),
// 0x0040c090 the constructor (vptr, count = 0, head = 0), 0x0040c0b0 the
// atexit registration of 0x0040c0c0, the destructor (vptr, head = 0,
// count = 0 -- RemoveAll's order).  0x004a1a84's twelve slots are
// genlist.h's GenList<T> in declaration order (slot 8 0x0040f640 is
// GenCopyList: RemoveAll, then GenAppendList), and the node links at
// +0x00/+0x04 of every surface are CGLLNode's m_pGPrev/m_pGNext: this is
// stdlith goodlinklist.h's CGLinkedList<T>, restated because stdlith is
// not on this target's include path.  The virtuals are declared, not
// defined: their bodies are shared template COMDATs other TUs own.
class CGLinkedListQ40C
{
public:
	CGLinkedListQ40C()
	{
		m_nElements = 0;                                  // 0x0040c09c
		m_pHead = LTNULL;                                 // 0x0040c0a1
	}

	~CGLinkedListQ40C()
	{
		RemoveAll();                                      // 0x0040c0cc / 0x0040c0d1
	}

	void RemoveAll()
	{
		m_pHead = LTNULL;
		m_nElements = 0;
	}

	virtual void   GenBegin(void* pPosOut) const;         // 0x00439fc0
	virtual BOOL   GenIsValid(void* pPos) const;          // 0x00459f30
	virtual void*  GenGetNext(void* pPos) const;          // 0x0041d520
	virtual void*  GenGetAt(void* pPos) const;            // 0x00459f40
	virtual BOOL   GenAppend(void* pToAppend);            // 0x0041d550
	virtual void   GenRemoveAt(void* pPos);               // 0x0041d5a0
	virtual void   GenRemoveAll();                        // 0x00459f50
	virtual uint32 GenGetSize() const;                    // 0x00459f60
	virtual BOOL   GenCopyList(void* pOther);             // 0x0040f640
	virtual BOOL   GenAppendList(void* pOther);           // 0x00439fe0
	virtual BOOL   GenFindElement(void* pElem, void* pPos) const;  // 0x004895b0
	virtual void   GenSetCacheSize(uint32 size);          // 0x00459f70

	uint32         m_nElements;                           // 0x004b179c
	CSurfaceQ40C*  m_pHead;                               // 0x004b17a0
};

CGLinkedListQ40C g_SurfaceListQ40C;                       // 0x004b1798

// The two PFormats' dynamic initialisers follow the list's in the image
// (0x0040c0e0/0x0040c0f0 store the vptr at 0x004b1758, 0x0040c100/0x0040c110
// at 0x004b15c0), so they are defined after it and in that order.
PFormatQ40C      g_ScreenFormat;                          // 0x004b1758

// The two colours clientinterface.cpp fills through 0x0045f5e0 (its lines
// 1726 / 2018-2019) and diblit.cpp already calls g_TransColorQ438 /
// g_FillColorQ438.
uint32                   g_TransColorQ40C;             // 0x004b161c
uint32                   g_FillColorQ40C;              // 0x004b1750

// The shadow copy of the screen format 0x0040f060 takes just before the
// render DLL goes away: 0x004b175c..0x004b178c -> 0x004b15c4..0x004b15f4,
// a constant 0x198 apart, so 0x004b15c0 is a PFormat and the vptr is the
// one field the copy skips.  0x004b1790 is the matching shadow of
// g_nBytesPerPixel.  surface_CopyIn reads BOTH (0x0040efc7, 0x0040efd9).
PFormatQ40C              g_SavedScreenFormatQ40C;      // 0x004b15c0
uint32                   g_nSavedBytesPerPixelQ40C;    // 0x004b1790

// The mask surface ci_DrawSurfaceMasked parks for its blitter callback:
// 0x0040d863 stores the wrapper's third parameter here and 0x0040d8a0
// reads it back.  A file-scope global, not a parameter, which is why
// cis_MaskedDraw's own signature has no mask in it.
CSurfaceQ40C*            g_pMaskSurfaceQ40C;           // 0x004b15f8

// 257 bytes at 0x004b164c, indexed by a surface dimension that has already
// been proved <= 0x100 and used as an AND mask (0x0040da0e, 0x0040da37).
// Zero for a dimension that is not a usable power of two -- that is what
// the two `test`/`je` guards at 0x0040d8e8 / 0x0040d8f4 reject.  It sits
// immediately after g_ScreenSurface (0x004b1620 + 0x2c = 0x004b164c).
uint8                    g_DimMaskTableQ40C[257];      // 0x004b164c


// ----------------------------------------------------------------------- //
//  Helpers this file calls but does not define.
// ----------------------------------------------------------------------- //

// 0x0040c130 -- fntdraw.cpp's cis_CreateSurface(width, height).
CSurfaceQ40C* cis_CreateSurfaceQ40C(uint32 nWidth, uint32 nHeight);

// 0x0040c120, 11 bytes -- g_pClientMgr's blitter sub-object at +0x1618:
// `mov 0x4b17e0,%eax; add $0x1618,%eax; ret` (name ours; clientinterface.cpp
// calls the same address sub_40c120).
CBlitterQ40C* cis_GetBlitterQ40C()
{
	return &g_pClientMgr->m_Blitter;
}

// 0x0040c5a0 / 0x0040c5d0 -- release a surface's renderer data / its
// system-memory copy.  Both are `if (p->field) { free it; field = 0; }`.
void cis_FreeRenderDataQ40C(CSurfaceQ40C* pSurface);        // 0x0040c5a0
void cis_FreeSysMemQ40C(CSurfaceQ40C* pSurface);            // 0x0040c5d0

// 0x0043a2a0 -- the clipper.  Twelve cdecl dwords; the two trailing LTRect*
// are OUT parameters and come back as the source and destination rectangles
// the blit is to use.  Read off the two call sites in cis_DoDrawSurfaceToSurface
// (0x0040d2f1 / 0x0040d325) and the two here (0x0040c2df / 0x0040c32d).
LTBOOL cis_ClipBlitQ40C(uint32 nSrcW, uint32 nSrcH,
                        int nSrcLeft, int nSrcTop, int nSrcRight, int nSrcBottom,
                        uint32 nDestW, uint32 nDestH, int nDestX, int nDestY,
                        LTRect* pOutSrc, LTRect* pOutDest);  // 0x0043a2a0

// 0x0043df40 -- fill a DIB from a stream; answers non-zero on success.
LTBOOL dib_LoadQ40C(class ILTStreamQ40C* pStream, CDIBQ40C* pDib);

// 0x004048c0 -- clientworldinfo.cpp's: open one CClientFileRef, or LTNULL.
class CClientFileRefQ40C
{
public:
	CClientFileRefQ40C() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// Only the two slots this file needs, both pure so no vtable is emitted
// here.  Slot 0 is iltstream.h's virtual destructor, slot 1 Release --
// clientworldinfo.cpp reads the same `call *0x4(%edx)` at its own site.
class ILTStreamQ40C
{
public:
	virtual void _dtor()=0;                  // slot 0
	virtual void Release()=0;                // slot 1  (0x0040cf85)
};

ILTStreamQ40C* cis_OpenFileQ40C(CFileMgrQ40C* pFileMgr,
                                CClientFileRefQ40C* pRef);   // 0x004048c0

// 0x00490c10 -- sb_FreeChunks(pBank, 0); clientmgr3.cpp names it sb_Term.
void sb_TermQ40C(CStructBankQ40C* pBank);                    // 0x00490c10

// 0x00489510 -- the font module's GDI teardown; defined at the end of this
// file (its real home is fntdraw.cpp's PART ONE).
void fnt_TermQ40C();                                         // 0x00489510

// de_error.h, as diblit.cpp and binkvideo.cpp already write it out.  The
// stringiser double-expands, which is why 0x004a7494 is "1" and not
// "LT_ERROR"; the SYM form applies `#` to its own parameter and gets
// "LT_INVALIDPARAMS" (0x004a77c8) instead.
extern "C" void dsi_OnReturnErrorQ40C(int code);             // 0x0042d760
extern int      g_DebugLevelQ40C;                            // 0x004b0ab0
extern char*    g_pErrorFormatQ40C;                          // 0x004a7110
void            con_ConsolePrintQ40C(char* pFormat, ...);    // 0x0042e6f0

#define Q_ERRSTR_(x) #x
#define Q_ERRSTR(x)  Q_ERRSTR_(x)
#define Q_ERRDESC    " "

#define Q_RETURN_ERROR(level, fnName, code) \
	{ \
		dsi_OnReturnErrorQ40C(code); \
		if (g_DebugLevelQ40C >= level) \
			con_ConsolePrintQ40C(g_pErrorFormatQ40C, fnName, \
			                     Q_ERRSTR(code), Q_ERRDESC); \
		return code; \
	}

#define Q_RETURN_ERROR_SYM(level, fnName, code) \
	{ \
		dsi_OnReturnErrorQ40C(code); \
		if (g_DebugLevelQ40C >= level) \
			con_ConsolePrintQ40C(g_pErrorFormatQ40C, fnName, \
			                     #code, Q_ERRDESC); \
		return code; \
	}


// ----------------------------------------------------------------------- //
//  0x0043acb0 / 0x0043afc0 -- the lock/unlock pair, INLINE.
//
//  src/engine/lithtech/diblit.cpp reconstructs both out of line (and
//  measured that marking them `inline` THERE costs two matches).  Here they
//  must expand, because both of this file's rows that lock a surface have
//  them expanded in the shipped bytes:
//
//    * 0x0040c374..0x0040c3dd is cis_LockSurface with bWrite folded to
//      LTTRUE -- the `cmp %ebx,%eax; cmp %ebx,%edi; orl $0x4,0x1c(%edi)`
//      at 0x0040c3d3..0x0040c3db is the `&&`'s two surviving conjuncts;
//    * 0x0040d8fc..0x0040d95d is the same body with bWrite folded to
//      LTFALSE -- no `|= 4` and none of the three tests;
//    * 0x0040c469..0x0040c497 and 0x0040da8d..0x0040dac9 are two
//      expansions of cis_UnlockSurface.
//
//  Both are declared ABOVE every caller because VC6's inliner is
//  forward-only.  IF THE BUILD SHOWS A CALL rather than
//  an expansion, hand-inline the bodies at the two sites: the expansion is
//  what the shipped bytes are, and the source spelling is only a means to
//  it.
// ----------------------------------------------------------------------- //

inline void* cis_LockSurfaceQ40C(CSurfaceQ40C* pSurface, uint32* pPitch,
                                 LTBOOL bWrite)
{
	void* pBits;

	if (!g_pRender)                                       // 0x0040c37c
		return LTNULL;

	*pPitch = 0;                                          // 0x0040c38a

	if (pSurface == &g_ScreenSurface)                     // 0x0040c384
	{
		// Positive test, success return first: 0x0040d93b is
		// `jne <return pBits>; jmp <fail>`.  The `if (!Lock) return NULL`
		// form inverted that pair in cis_MaskedDraw; 0x0040c3aa is unchanged.
		if (g_pRender->LockScreen(0, 0, g_ScreenSurface.m_nWidth, g_ScreenSurface.m_nHeight,
		                           &pBits, pPitch))       // 0x0040c3aa
		{
			return pBits;                                 // 0x0040c3b7 neg/sbb/and
		}

		return LTNULL;
	}

	*pPitch = pSurface->m_nPitch;                         // 0x0040c3c6

	pBits = g_pRender->LockSurface(pSurface->m_pRenderData);   // 0x0040c3ca

	if (bWrite && pBits && pSurface)                      // 0x0040c3d3..0x0040c3d9
		pSurface->m_nFlags |= 4;                          // 0x0040c3db

	return pBits;
}

inline void cis_UnlockSurfaceQ40C(CSurfaceQ40C* pSurface)
{
	if (pSurface == &g_ScreenSurface)                     // 0x0040c469
		g_pRender->UnlockScreen();                        // 0x0040c478
	else
		g_pRender->UnlockSurface(pSurface->m_pRenderData); // 0x0040c491
}


// ----------------------------------------------------------------------- //
//  THE BLITTER CALLBACK SIGNATURE
//
//  cis_DoDrawSurfaceToSurface (0x0040d290) takes one of these as its sixth
//  parameter and calls it at 0x0040d4f9 and 0x0040d599.  Seven cdecl dwords
//  (`add $0x1c,%esp` at 0x0040d4fb / 0x0040d59b), read off the two call
//  sites and confirmed against all three callees:
//
//      arg0  bOverlap   1 at 0x0040d4f7 (hDest == hSrc), 0 at 0x0040d597
//      arg1  pSrcBits   0x0040d660 reads it at [E+0x08] and adds
//                       srcRect.top*arg3 + srcRect.left*bpp
//      arg2  pDestBits  read at [E+0x0c], offset by destRect
//      arg3  nSrcPitch  [E+0x10]
//      arg4  nDestPitch [E+0x14]
//      arg5  pSrcRect   [E+0x18]   the clipper's FIRST out rect
//      arg6  pDestRect  [E+0x1c]   the clipper's SECOND out rect
//
//  The answer is an LTRESULT and ZERO MEANS SUCCESS: 0x0040d59e treats a
//  non-zero result as "no failure to record", and 0x0040d5fc turns a
//  never-set flag into RETURN_ERROR.  All three callbacks end
//  `xor %eax,%eax; ret`.
// ----------------------------------------------------------------------- //

typedef LTRESULT (*BlitFnQ40C)(LTBOOL bOverlap, void* pSrcBits, void* pDestBits,
                               uint32 nSrcPitch, uint32 nDestPitch,
                               LTRect* pSrcRect, LTRect* pDestRect);

LTRESULT cis_DoDrawSurfaceToSurfaceQ40C(CSurfaceQ40C* hDest, CSurfaceQ40C* hSrc,
                                        LTRect* pSrcRect, int destX, int destY,
                                        BlitFnQ40C pfnBlit);        // 0x0040d290


// ======================================================================= //
//  0x0040cc80 -- 93 bytes.  Is this COLUMN all one colour?
// ======================================================================= //
//
//  WHAT PINS IT.  Its two callers are inside ci_GetBorderSize (0x0040cb56
//  and 0x0040cb8f, the function that reports itself as "GetBorderSize",
//  0x004a8148).  Four cdecl dwords (`add $0x10,%esp`); the caller passes
//  the surface's PITCH as the stride and its HEIGHT as the count, which is
//  what makes this the vertical scan and 0x0040cce0 the horizontal one.
//
//  SHAPE.  `mov 0x4b175c,%eax; cmp $0x2; jne` then `cmp $0x3; jne` with
//  BOTH misses falling on the shared `mov $0x1,%eax` -- one if/else-if and
//  a trailing `return LTTRUE`, so an unknown bit depth answers TRUE.  The
//  two `jne 0x40ccd9` both reach one `xor %eax,%eax` tail: two written-out
//  `return LTFALSE` statements, tail-merged.
//
//  The counters are UNSIGNED: `test %edx,%edx; je` and `dec %edx; jne`,
//  never a signed `jle`.  The pointer advances by nStride BYTES
//  (`add %ecx,%eax`, 0x0040cca4), so it is a uint8*.
// ======================================================================= //

LTBOOL cis_IsColumnColorQ40C(uint8* pPixels, uint32 nColor, uint32 nCount,
                             uint32 nStride)
{
	if (g_ScreenFormat.m_nBitDepth == 2)                  // 0x0040cc86
	{
		// A plain `while` with the count decremented in the body; VC6
		// rotates it into the guard + `dec ; jne` shipped has.  An explicit
		// `if (n) do {} while (--n)` made VC6 peel the first compare (99
		// bytes, near).
		while (nCount)                                    // 0x0040cc8f
		{
			if (*(uint16*)pPixels != (uint16)nColor)      // 0x0040cc9f
				return LTFALSE;                           // 0x0040ccd9

			pPixels += nStride;                           // 0x0040cca4
			--nCount;                                     // 0x0040cca6
		}
	}
	else if (g_ScreenFormat.m_nBitDepth == 3)             // 0x0040ccb0
	{
		while (nCount)                                    // 0x0040ccbb
		{
			if (*(uint32*)pPixels != nColor)              // 0x0040ccc9
				return LTFALSE;

			pPixels += nStride;                           // 0x0040cccd
			--nCount;                                     // 0x0040cccf
		}
	}

	return LTTRUE;                                        // 0x0040cca9
}


// ======================================================================= //
//  0x0040cce0 -- 87 bytes.  Is this ROW all one colour?
// ======================================================================= //
//
//  The same function with the stride folded to the pixel size: THREE cdecl
//  dwords (`add $0xc,%esp` at its two callers, 0x0040cbcd and 0x0040cc07),
//  `add $0x2,%ecx` / `add $0x4,%ecx` in place of `add %ecx,%eax`.  The
//  callers pass the surface's WIDTH as the count.
//
//  The one shape difference from its sibling is the loop test: `dec %eax;
//  add $0x2,%ecx; test %eax,%eax; jne` rather than `dec; jne` -- here the
//  source decrements BEFORE stepping the pointer, the column scan after.
// ======================================================================= //

LTBOOL cis_IsRowColorQ40C(uint8* pPixels, uint32 nColor, uint32 nCount)
{
	if (g_ScreenFormat.m_nBitDepth == 2)                  // 0x0040cce5
	{
		// A COUNTDOWN `while`, not a counting for and not `if (n) do {}
		// while (--n)` (that one peels the first compare: 89 bytes, near).
		// The decrement comes BEFORE the pointer step: shipped `dec %eax ;
		// add $0x2,%ecx ; test %eax,%eax ; jne` (0x0040ccff).
		while (nCount)                                    // 0x0040ccee
		{
			if (*(uint16*)pPixels != (uint16)nColor)      // 0x0040ccfa
				return LTFALSE;                           // 0x0040cd34

			--nCount;                                     // 0x0040ccff
			pPixels += 2;                                 // 0x0040cd00
		}
	}
	else if (g_ScreenFormat.m_nBitDepth == 3)             // 0x0040cd0d
	{
		while (nCount)                                    // 0x0040cd18
		{
			if (*(uint32*)pPixels != nColor)              // 0x0040cd22
				return LTFALSE;

			--nCount;                                     // 0x0040cd26
			pPixels += 4;                                 // 0x0040cd27
		}
	}

	return LTTRUE;                                        // 0x0040cd07
}


// ======================================================================= //
//  0x0040cf30 -- 96 bytes.  Load a named bitmap into a DIB.
// ======================================================================= //
//
//  WHAT PINS IT.  Its two callers are ci_CreateSurfaceFromBitmap
//  (0x0040ce8c) and ci_DrawBitmapToSurface (0x0040d061), and both hand it
//  their own `char* pBitmapName` parameter plus the address of the 0x458-
//  byte DIB they have just constructed with 0x0043de60.
//
//  The twelve-byte frame is a CClientFileRef, field for field with
//  clientmgr.h: `movw $0,0x8(%esp)` then `movl $1,(%esp)` then
//  `mov %eax,0x4(%esp)`.  That ORDER is the fingerprint -- the constructor
//  zeroes all three in declaration order, dead-store elimination kills the
//  two the body immediately overwrites, and what survives is m_nLocalID's
//  zero followed by the two assignments in source order.  This site sets
//  m_nFileType to 1 where ci_GetWorldInfoString leaves it 0.
//
//  Release() is the stream's vtable slot +0x04 (0x0040cf85), exactly as
//  clientworldinfo.cpp reads it, and it runs BEFORE the answer is returned
//  -- the result lives in %edi across the call.
// ======================================================================= //

LTBOOL cis_LoadDIBQ40C(char* pBitmapName, CDIBQ40C* pDib)
{
	CClientFileRefQ40C ref;
	ILTStreamQ40C*     pStream;
	LTBOOL             bOk;

	ref.m_nFileType = 1;                                  // 0x0040cf44
	ref.m_pFilename = pBitmapName;                        // 0x0040cf4c

	pStream = cis_OpenFileQ40C(g_pClientMgr->m_pFileMgr, &ref);   // 0x0040cf5d
	if (!pStream)                                         // 0x0040cf67
		return LTFALSE;

	bOk = dib_LoadQ40C(pStream, pDib);                    // 0x0040cf77

	pStream->Release();                                   // 0x0040cf85

	return bOk;                                           // 0x0040cf88
}


// 0x0040cf90, 19 bytes -- ILTClient::CreateSurface (clientinterface.h slot
// 0x174): a straight forward to cis_CreateSurface.
HSURFACE ci_CreateSurface(uint32 width, uint32 height)
{
	return (HSURFACE)cis_CreateSurfaceQ40C(width, height);
}


// ======================================================================= //
//  0x0040cfb0 -- 15 bytes.  ILTClient::GetSurfaceUserData.
// ======================================================================= //
//
//  clientinterface.h slot 0x17c names it.  A null surface answers NULL
//  (`test; je` to `xor %eax,%eax`), otherwise the +0x24 user pointer.
// ======================================================================= //

void* ci_GetSurfaceUserData(HSURFACE hSurface)
{
	if (hSurface)
		return ((CSurfaceQ40C*)hSurface)->m_pUserData;

	return LTNULL;
}


// ======================================================================= //
//  0x0040d010 -- 260 bytes.  ILTClient::DrawBitmapToSurface.
// ======================================================================= //
//
//  clientinterface.h slot 0x198 names it and gives the signature.  The
//  body is the three helpers either side of it: a 0x458-byte stack DIB
//  (constructed by 0x0043de60 at 0x0040d01b), cis_LoadDIB (0x0040d061),
//  cis_DrawDIBToSurface (0x0040d0ca).  The NULL test at 0x0040d027 is on
//  the FIRST argument (0x460(%esp) past the 0x458 frame and %esi), the
//  surface -- not the file name.  Three exits, each with its own copy of
//  the DIB's inline teardown; the two failures share `xor %eax,%eax`.
// ======================================================================= //

void cis_DrawDIBToSurfaceQ40C(CSurfaceQ40C* hDest, CDIBQ40C* pDib,
                              LTRect* pSrcRect, int destX, int destY);  // 0x0040c2a0

LTBOOL ci_DrawBitmapToSurface(HSURFACE hDest, char* pSourceBitmapName,
                              LTRect* pSrcRect, int destX, int destY)
{
	CDIBQ40C dib;                                         // 0x0040d01b

	if (!hDest)                                           // 0x0040d027
		return LTFALSE;

	if (!cis_LoadDIBQ40C(pSourceBitmapName, &dib))        // 0x0040d061
		return LTFALSE;

	cis_DrawDIBToSurfaceQ40C((CSurfaceQ40C*)hDest, &dib, pSrcRect,
	                         destX, destY);               // 0x0040d0ca

	return LTTRUE;                                        // 0x0040d107
}


// ======================================================================= //
//  0x0040c2a0 -- 516 bytes.  Blit a loaded DIB into a surface.
// ======================================================================= //
//
//  WHAT PINS THE SIGNATURE.  ci_DrawBitmapToSurface (0x0040d010, the SDK
//  entry at clientinterface.h slot 0x198) calls it at 0x0040d0ca with
//  `push destY; push destX; push pSrcRect; push &dib; push hDest` and
//  `add $0x14,%esp` -- five cdecl dwords, in clientinterface.h's own
//  parameter order with the file name replaced by the DIB it loaded.
//  0x0040c260 calls it with (surface, dib, LTNULL, 0, 0).
//
//  IT RETURNS NOTHING.  0x0040c47e leaves whatever UnlockScreen answered in
//  %eax and 0x0040c49a leaves the zero the null test branched on; both
//  callers discard it (0x0040c292 reloads %esi, 0x0040d0cf reloads a local).
//
//  THE TWO RECTANGLES are LTRects: eight `mov %ebx,<slot>` at 0x0040c2af..
//  0x0040c2cb, four per object in left/top/right/bottom order, which is
//  ltrect.h's defaulted constructor inlined twice.  The one at the LOWER
//  address is constructed first and is the clipper's FIRST out parameter,
//  so it is the SOURCE rectangle -- the same order cis_DoDrawSurfaceToSurface
//  uses at 0x0040d2af.
//
//  THE CLIP CALL's twelve arguments are identical to
//  cis_DoDrawSurfaceToSurface's, and the two arms share their last push and
//  the call itself (0x0040c32b `jmp 0x40c36e`) because both leave the DIB's
//  width in %ecx.  ITS ANSWER IS DISCARDED here (0x0040c374 overwrites %eax
//  with g_pRender) where cis_DoDrawSurfaceToSurface tests it.
//
//  THE SOURCE POINTER has NO bytes-per-pixel multiply on the left edge --
//  0x0040c42d is a plain `add %edx,%ebp` -- so the DIB is one byte per
//  pixel, which is what makes the 0x400 bytes at pDib+0x38 a 256-entry
//  palette and pDib itself the source PFormat.
//
//  BYTE-IDENTICAL.  The nine stores into the request come out of the
//  shipped scheduler in the order m_pDest, m_nSrcPitch, m_nWidth,
//  m_pSrcFormat, m_pDestFormat, m_pSrc, m_pSrcPalette, m_nHeight,
//  m_pDestPalette; the source order below (dest first, then the two
//  formats) is the one measured to reproduce it.
// ======================================================================= //

void cis_DrawDIBToSurfaceQ40C(CSurfaceQ40C* hDest, CDIBQ40C* pDib,
                              LTRect* pSrcRect, int destX, int destY)
{
	LTRect          srcRect, destRect;                    // 0x0040c2af..0x0040c2cb
	BlitRequestQ40C request;                              // 0x0040c2cf
	uint8*          pDestRow;

	if (pSrcRect)                                         // 0x0040c2dd
	{
		cis_ClipBlitQ40C(pDib->m_nWidth, pDib->m_nHeight,
		                 pSrcRect->left, pSrcRect->top,
		                 pSrcRect->right, pSrcRect->bottom,
		                 hDest->m_nWidth, hDest->m_nHeight, destX, destY,
		                 &srcRect, &destRect);            // 0x0040c2ee..0x0040c36f
	}
	else
	{
		cis_ClipBlitQ40C(pDib->m_nWidth, pDib->m_nHeight,
		                 0, 0, pDib->m_nWidth, pDib->m_nHeight,
		                 hDest->m_nWidth, hDest->m_nHeight, destX, destY,
		                 &srcRect, &destRect);            // 0x0040c32d..0x0040c36f
	}

	request.m_pDest = cis_LockSurfaceQ40C(hDest, &request.m_nDestPitch, LTTRUE);

	if (request.m_pDest)                                  // 0x0040c3df
	{
		// The destination row is named first, then offset by the column:
		// written as one sum, VC6 adds the column term first and the two
		// multiplies swap registers (0x0040c3eb..0x0040c405).
		pDestRow               = (uint8*)request.m_pDest
		                       + destRect.top * request.m_nDestPitch;       // 0x0040c3eb / 0x0040c3ff
		request.m_pDest        = pDestRow
		                       + destRect.left * g_nBytesPerPixel;          // 0x0040c3ef / 0x0040c405
		request.m_pSrcFormat   = (PFormatQ40C*)pDib;      // 0x0040c437
		request.m_pDestFormat  = &g_ScreenFormat;         // 0x0040c445
		request.m_pSrc         = pDib->m_Data.m_pArray
		                       + srcRect.top * pDib->m_nPitch
		                       + srcRect.left;            // 0x0040c407..0x0040c44d
		request.m_nSrcPitch    = pDib->m_nPitch;          // 0x0040c423
		request.m_pSrcPalette  = pDib->m_Palette;         // 0x0040c43f / 0x0040c451
		request.m_nWidth       = srcRect.right - srcRect.left;      // 0x0040c433
		request.m_nHeight      = srcRect.bottom - srcRect.top;      // 0x0040c455
		request.m_pDestPalette = LTNULL;                  // 0x0040c459

		cis_GetBlitterQ40C()->Blt(&request);              // 0x0040c45d / 0x0040c464

		cis_UnlockSurfaceQ40C(hDest);                     // 0x0040c469
	}
}


// ======================================================================= //
//  0x0040c130 -- 301 bytes.  Create a surface (fntdraw.cpp's name).
// ======================================================================= //
//
//  Two cdecl dwords, (width, height): both must be non-zero and <= 5000
//  (0x1388, `ja`, unsigned) and the renderer present, else LTNULL
//  (0x0040c258, one shared `xor %eax,%eax`).  The render DLL makes the
//  surface (+0xc8, 0x0040c171) and reports its real size back THROUGH THE
//  PARAMETERS (+0xd0 is handed &nWidth, &nHeight and the one local, the
//  pitch -- 0x0040c184..0x0040c199).  The CSurface comes out of the bank
//  at 0x004b1600 through struct_bank.h's inline sb_Allocate (grow via
//  0x00490c20 and RE-READ, no null test after: the stores at 0x0040c1d0
//  go through whatever came back).  Then fntdraw.cpp's fnt_CreateFont
//  list splice, with ONE difference that is load-bearing: the head is a
//  LOCAL for the four splice stores (0x0040c20a..0x0040c21a keep it in
//  %ecx and reload only head->m_pPrev), while the always-true
//  `m_pNext == head` test after the count re-reads the GLOBAL (0x0040c222).
//  Spelled through g_pSurfaceList at every mention, VC6 reloads the global
//  after each store: 319 bytes, near.  (fntdraw.cpp's fnt_CreateFont,
//  0x00488a50, is near for exactly this reason -- same fix.)
// ======================================================================= //

int sb_GrowQ40C(CStructBankQ40C* pBank, uint32 nCount);    // 0x00490c20

inline void* sb_AllocateQ40C(CStructBankQ40C* pBank)
{
	void* pRet;

	pRet = pBank->m_pFreeList;
	if (!pRet)
	{
		sb_GrowQ40C(pBank, pBank->m_GrowCount);
		pRet = pBank->m_pFreeList;
		if (!pRet)
			return 0;
	}

	pBank->m_pFreeList = *(void**)pRet;
	return pRet;
}

CSurfaceQ40C* cis_CreateSurfaceQ40C(uint32 nWidth, uint32 nHeight)
{
	void*         pRenderData;
	CSurfaceQ40C* pSurface;
	uint32        nPitch;

	if (!nWidth || !nHeight || nWidth > 5000 || nHeight > 5000)
		return LTNULL;

	if (!g_pRender)                                       // 0x0040c167
		return LTNULL;

	pRenderData = g_pRender->CreateSurface(nWidth, nHeight);   // 0x0040c171
	if (!pRenderData)
		return LTNULL;

	g_pRender->GetSurfaceInfo(pRenderData, &nWidth, &nHeight, &nPitch);  // 0x0040c199

	pSurface = (CSurfaceQ40C*)sb_AllocateQ40C(&g_SurfaceBankQ40C);

	pSurface->m_pRenderData = pRenderData;                // 0x0040c1d0
	pSurface->m_pSysMem     = LTNULL;
	pSurface->m_nWidth      = nWidth;
	pSurface->m_nHeight     = nHeight;
	pSurface->m_nPitch      = nPitch;
	pSurface->m_nFlags      = 0;
	pSurface->m_pUserData   = LTNULL;
	pSurface->m_fAlpha      = 1.0f;                       // 0x0040c1fd

	if (g_SurfaceListQ40C.m_pHead)                                   // 0x0040c20a
	{
		CSurfaceQ40C* pHead = g_SurfaceListQ40C.m_pHead;

		pHead->m_pPrev->m_pNext = pSurface;
		pSurface->m_pPrev = pHead->m_pPrev;
		pSurface->m_pNext = pHead;
		pHead->m_pPrev = pSurface;

		g_SurfaceListQ40C.m_nElements++;

		if (pSurface->m_pNext == g_SurfaceListQ40C.m_pHead)          // 0x0040c232
			g_SurfaceListQ40C.m_pHead = pSurface;
	}
	else
	{
		pSurface->m_pNext = pSurface;
		pSurface->m_pPrev = pSurface;

		g_SurfaceListQ40C.m_pHead = pSurface;
		g_SurfaceListQ40C.m_nElements++;
	}

	return pSurface;
}


// ======================================================================= //
//  0x0040c260 -- 58 bytes.  Make a surface the size of a DIB and fill it.
// ======================================================================= //
//
//  WHAT PINS IT.  Two callers: ci_CreateSurfaceFromBitmap (0x0040cedc,
//  clientinterface.h slot 0x16c) and 0x0041cde3, which stores the answer at
//  CClientMgr+0x3a4 a few instructions before pushing "ConsoleFont".  Both
//  hand it the address of a 0x458-byte DIB they built on the stack, so it
//  takes ONE argument and answers an HSURFACE.
//
//  cis_CreateSurface's arguments are (width, height): 0x0040c272 pushes
//  pDib+0x43c then pDib+0x438, and cis_CreateSurface stores the FIRST at
//  CSurface+0x10 (0x0040c1de).
//
//  BOTH RETURNS ARE `return pSurface` -- neither epilogue materialises a
//  zero (0x0040c282 falls out with %eax still holding cis_CreateSurface's
//  answer), so the guard is `if (pSurface) { ... }` with one trailing
//  return, not an early `return LTNULL`.  The second copy needs
//  `mov %esi,%eax` only because the inner call clobbered %eax.
// ======================================================================= //

CSurfaceQ40C* cis_CreateSurfaceFromDIBQ40C(CDIBQ40C* pDib)
{
	CSurfaceQ40C* pSurface;

	pSurface = cis_CreateSurfaceQ40C(pDib->m_nWidth, pDib->m_nHeight);  // 0x0040c274

	// AN EARLY RETURN, not a guarded call. This is the merge-the-exits rule
	// running BACKWARDS, and the two-image note explains why: /O1 tail-merges
	// duplicated epilogues, /O2 PRESERVES them. Written as `if (p) call;`
	// VC6 emits one `je` onto a shared exit; the shipped /O2 bytes duplicate
	// the epilogue (`jne ; pop %edi ; pop %esi ; ret`), which is what a real
	// early return gives. So in lithtech.exe the question is not "do the
	// exits merge" but "does the SOURCE return early", and here it does.
	if (!pSurface)                                        // 0x0040c27e
		return LTNULL;

	cis_DrawDIBToSurfaceQ40C(pSurface, pDib, LTNULL, 0, 0);  // 0x0040c28d

	return pSurface;
}


// ======================================================================= //
//  0x0040d120 -- 157 bytes.  ILTClient::DrawSurfaceSolidColor.
// ======================================================================= //
//
//  clientinterface.h slot 0x1a0; reports itself as "DrawSurfaceSolidColor"
//  (0x004a8168) with code string "73" (0x004a7168, RETURN_ERROR form).
//  Converts hTransColor into g_TransColor (0x0040d16f) and hFillColor into
//  g_FillColor (0x0040d18a), then hands cis_DoDrawSurfaceToSurface the
//  callback 0x0040d1c0 (0x0040d1a2).  ci_DrawSurfaceMasked's shape.
// ======================================================================= //

LTRESULT cis_SolidColorDrawQ40C(LTBOOL bOverlap, void* pSrcBits, void* pDestBits,
                                uint32 nSrcPitch, uint32 nDestPitch,
                                LTRect* pSrcRect, LTRect* pDestRect);  // 0x0040d1c0

LTRESULT ci_DrawSurfaceSolidColorQ40C(CSurfaceQ40C* hDest, CSurfaceQ40C* hSrc,
                                      LTRect* pSrcRect, int destX, int destY,
                                      HLTCOLOR hTransColor, HLTCOLOR hFillColor)
{
	if (!g_pRender)                                       // 0x0040d120
		Q_RETURN_ERROR(1, "DrawSurfaceSolidColor", LT_NOTINITIALIZED)

	cis_GetBlitterQ40C()->ConvertColor(&g_ScreenFormat, hTransColor,
	                                   &g_TransColorQ40C);  // 0x0040d176
	cis_GetBlitterQ40C()->ConvertColor(&g_ScreenFormat, hFillColor,
	                                   &g_FillColorQ40C);   // 0x0040d191

	return cis_DoDrawSurfaceToSurfaceQ40C(hDest, hSrc, pSrcRect, destX, destY,
	                                      cis_SolidColorDrawQ40C);  // 0x0040d1b4
}


// ======================================================================= //
//  0x0040d1c0 -- 202 bytes.  ci_DrawSurfaceSolidColor's blitter callback.
// ======================================================================= //
//
//  WHAT PINS IT.  0x0040d1a2 is `push $0x40d1c0` -- the sixth argument of
//  cis_DoDrawSurfaceToSurface -- inside the function that reports itself as
//  "DrawSurfaceSolidColor" (0x004a8168) and that first converts BOTH of its
//  colour parameters through 0x0045f5e0, hTransColor into 0x004b161c and
//  hFillColor into 0x004b1750.  So this callback is the one that reads
//  those two globals, and it does: 0x0040d224 / 0x0040d230.
//
//  The two arms are the 16-bit and 32-bit screen formats and there is NO
//  third: a bit depth that is neither 2 nor 3 falls straight to the row
//  advance, leaving the surface untouched, and the function still answers
//  LT_OK.  That is why 0x0040d20e LOADS 0x004b175c into %eax (two compares)
//  where cis_MaskedDraw, which has one arm, compares memory directly.
//
//  The bit-depth test is INSIDE the row loop (0x0040d20e is the loop head,
//  reached from 0x0040d281) -- the global is re-read for every row.
//
//  ARGUMENT 0 IS UNUSED: there is no reference to [%esp+0x14] anywhere in
//  the 202 bytes.  0x0040d660 is the sibling that does use it.
//
//  The destination pointer is carried as `src + delta` (0x0040d222
//  `sub %ebx,%esi`, then `mov %cx,(%esi,%eax,1)`): VC6's strength reduction
//  of two pointers walked in lockstep, not something the source spells.
//
//  IDENTICAL.  Two levers closed it (it had been called allocation):
//   * The row cursors ARE the pointer parameters pSrcBits / pDestBits,
//     as in cis_CopyDraw (0x0040d660).  Separate uint8* locals left bpp
//     cached in %ebx before the pushes and 14 blocks; with the params the
//     prologue (pSrcRect in %edx, bpp in %esi after the pushes) matches.
//     nCols is computed before nRows, as in cis_TransparentDraw.
//   * The fill colour is loaded as a whole DWORD and stored through its
//     low half (0x0040d230 `mov 0x4b1750,%ecx` / `mov %cx,...`), and both
//     arms keep trans+fill in ONE register (%ecx).  Every direct spelling
//     narrows the load (`mov ...,%bp`, one byte longer) and gives the 32-bit
//     arm its own allocation.  The select with two equal arms is diblit.cpp's
//     STAND-IN (0x0043b167): VC6 folds it late, keeping the load whole.
//     Needed in BOTH arms -- 16-bit only leaves 3 blocks in the 32-bit arm,
//     32-bit only leaves 203 bytes.  The original spelling is unknown.
// ======================================================================= //

LTRESULT cis_SolidColorDrawQ40C(LTBOOL /*bOverlap*/, void* pSrcBits, void* pDestBits,
                                uint32 nSrcPitch, uint32 nDestPitch,
                                LTRect* pSrcRect, LTRect* pDestRect)
{
	uint32 nRows, nCols, y;

	nCols = pSrcRect->right - pSrcRect->left;             // 0x0040d1e0
	nRows = pSrcRect->bottom - pSrcRect->top;             // 0x0040d1d9

	pSrcBits = (uint8*)pSrcBits
	      + pSrcRect->top * nSrcPitch                     // 0x0040d1db
	      + pSrcRect->left * g_nBytesPerPixel;            // 0x0040d1e6 / 0x0040d1eb

	pDestBits = (uint8*)pDestBits
	      + pDestRect->top * nDestPitch                   // 0x0040d1f7
	      + pDestRect->left * g_nBytesPerPixel;           // 0x0040d1fc / 0x0040d205

	for (y = 0; y < nRows; y++)                           // 0x0040d203 / 0x0040d27c
	{
		if (g_ScreenFormat.m_nBitDepth == 2)              // 0x0040d213
		{
			uint16* pS = (uint16*)pSrcBits;
			uint16* pD = (uint16*)pDestBits;
			uint32  x;

			x = nCols;
			while (x)                                     // 0x0040d218 / 0x0040d23d
			{
				if (*pS != (uint16)g_TransColorQ40C)      // 0x0040d224 / 0x0040d22b
					*pD = (uint16)(*pS ? g_FillColorQ40C          // 0x0040d230 / 0x0040d236
					                   : g_FillColorQ40C);  // STAND-IN, see above

				pS++;                                     // 0x0040d23a
				pD++;
				x--;
			}
		}
		else if (g_ScreenFormat.m_nBitDepth == 3)         // 0x0040d242
		{
			uint32* pS = (uint32*)pSrcBits;
			uint32* pD = (uint32*)pDestBits;
			uint32  x;

			x = nCols;
			while (x)                                     // 0x0040d247 / 0x0040d269
			{
				if (*pS != g_TransColorQ40C)              // 0x0040d253 / 0x0040d259
					*pD = *pS ? g_FillColorQ40C                   // 0x0040d25d / 0x0040d263
					          : g_FillColorQ40C;            // STAND-IN, see above

				pS++;                                     // 0x0040d266
				pD++;
				x--;
			}
		}

		pSrcBits  = (uint8*)pSrcBits + nSrcPitch;         // 0x0040d278
		pDestBits = (uint8*)pDestBits + nDestPitch;       // 0x0040d27a
	}

	return LT_OK;                                         // 0x0040d286
}


// ======================================================================= //
//  0x0040d290 -- 963 bytes.  cis_DoDrawSurfaceToSurface.
// ======================================================================= //
//
//  IT NAMES ITSELF: both error sites push the dword at 0x004a8084, which
//  dereferences to "cis_DoDrawSurfaceToSurface" (see the file head).  The
//  worker behind ci_DrawSurfaceToSurface, ...Transparent (clientinterface.
//  cpp), ci_DrawSurfaceSolidColor (0x0040d1b4) and the masked draw: clip,
//  offer a plain or keyed copy that touches the screen to the renderer's
//  accelerated slots, otherwise lock both surfaces and hand the rows to
//  the blitter callback.
//
//  * The two rects and the request are constructed at the TOP
//    (0x0040d29f..0x0040d2cf), before the argument test: locals with
//    inline constructors (LTRect's zeroing default arguments;
//    CisBlitRequestQ40C's five fields and the 1.0 alpha).
//  * RenderStruct+0xec (0x004b4444) blits a surface ONTO the screen,
//    +0xfc (0x004b4454) the screen onto a surface; clientblit.cpp
//    measured the request.  Only cis_CopyDraw (0x0040d660) and
//    cis_TransparentDraw (0x0040d710) are offered to them, and
//    m_bTransparent is `pfnBlit == cis_TransparentDraw` (sete, 0x0040d3c5).
//  * The software route locks the source read-only and the destination
//    for writing through the inline cis_LockSurface (0x0040d441 /
//    0x0040d513); a surface drawn onto itself is locked once and passed
//    as both, with bOverlap 1 (0x0040d4f7).
//  * bOk lives in hSrc's dead argument slot (0x0040d446): set only when a
//    callback answers LT_OK (0x0040d506 / 0x0040d5a2), and a clear flag
//    at the end is the same LT_ERROR exit a failed source lock takes
//    (0x0040d602 -> 0x0040d481).
// ======================================================================= //

// The request +0xec / +0xfc take -- clientinterface.cpp's CIBlitRequest
// (its 0x0040dcc0 builds the same one), restated here.
class CisBlitRequestQ40C
{
public:
	CisBlitRequestQ40C()
	{
		m_pSurfaceData = LTNULL;
		m_bTransparent = 0;
		m_pSrcRect     = LTNULL;
		m_pDestRect    = LTNULL;
		m_fAlpha       = 1.0f;
	}

	void*   m_pSurfaceData;                  // 0x00
	uint32  m_bTransparent;                  // 0x04
	uint32  m_TransColor;                    // 0x08  g_TransColor
	LTRect* m_pSrcRect;                      // 0x0c
	LTRect* m_pDestRect;                     // 0x10
	float   m_fAlpha;                        // 0x14
	void*   m_pPoints;                       // 0x18  0x0040e4d0's warp only;
	uint32  m_nPoints;                       // 0x1c  still in the 0x40 frame
};

// Both are SLOTS of the renderer's function table, clientrender.cpp's
// g_RenderStruct at 0x004b4358 (+0xec = 0x004b4444, +0xfc = 0x004b4454),
// not globals of their own: reached through that object so the link binds
// them to its one definition.
class RenderStruct;
extern RenderStruct g_RenderStruct;                                        // 0x004b4358

class CisRenderSlotsQ40C
{
public:
	uint8  m_pad_00[0xec];
	void (*m_pfnBlitToScreen)(CisBlitRequestQ40C* pRequest);               // 0x0ec
	uint8  m_pad_f0[0xfc - 0xf0];
	void (*m_pfnBlitFromScreen)(CisBlitRequestQ40C* pRequest);             // 0x0fc
};

#define g_pRenderBlitToScreenQ40C   (((CisRenderSlotsQ40C*)&g_RenderStruct)->m_pfnBlitToScreen)    // 0x004b4444
#define g_pRenderBlitFromScreenQ40C (((CisRenderSlotsQ40C*)&g_RenderStruct)->m_pfnBlitFromScreen)  // 0x004b4454

void cis_OptimizeIfDirtyQ40C(CSurfaceQ40C* pSurface);        // 0x0040d7d0 (clientblit.cpp's sub_40d7d0)

LTRESULT cis_CopyDrawQ40C(LTBOOL bOverlap, void* pSrcBits, void* pDestBits,
                          uint32 nSrcPitch, uint32 nDestPitch,
                          LTRect* pSrcRect, LTRect* pDestRect);        // 0x0040d660
LTRESULT cis_TransparentDrawQ40C(LTBOOL bOverlap, void* pSrcBits, void* pDestBits,
                                 uint32 nSrcPitch, uint32 nDestPitch,
                                 LTRect* pSrcRect, LTRect* pDestRect); // 0x0040d710

LTRESULT cis_DoDrawSurfaceToSurfaceQ40C(CSurfaceQ40C* hDest, CSurfaceQ40C* hSrc,
                                        LTRect* pSrcRect, int destX, int destY,
                                        BlitFnQ40C pfnBlit)
{
	static char* pErrFnName = "cis_DoDrawSurfaceToSurface";   // 0x004a8084

	LTRect             srcRect, destRect;
	CisBlitRequestQ40C request;
	void*              pSrcBits;
	void*              pDestBits;
	uint32             nSrcPitch, nDestPitch;
	LTBOOL             bVisible, bOk;

	if (hSrc && hDest)                                    // 0x0040d29d / 0x0040d2e1
	{
		if (pSrcRect)                                     // 0x0040d2ed
		{
			bVisible = cis_ClipBlitQ40C(hSrc->m_nWidth, hSrc->m_nHeight,
			                            pSrcRect->left, pSrcRect->top,
			                            pSrcRect->right, pSrcRect->bottom,
			                            hDest->m_nWidth, hDest->m_nHeight,
			                            destX, destY, &srcRect, &destRect);
		}
		else
		{
			bVisible = cis_ClipBlitQ40C(hSrc->m_nWidth, hSrc->m_nHeight,
			                            0, 0, hSrc->m_nWidth, hSrc->m_nHeight,
			                            hDest->m_nWidth, hDest->m_nHeight,
			                            destX, destY, &srcRect, &destRect);
		}

		if (!bVisible)                                    // 0x0040d355
			return LT_OK;

		if (pfnBlit == cis_CopyDrawQ40C || pfnBlit == cis_TransparentDrawQ40C)  // 0x0040d367 / 0x0040d36f
		{
			if (hDest == &g_ScreenSurface)                // 0x0040d37b
			{
				if (g_pRenderBlitToScreenQ40C)            // 0x0040d383
				{
					cis_OptimizeIfDirtyQ40C(hSrc);        // 0x0040d390

					request.m_pSurfaceData = hSrc->m_pRenderData;
					request.m_TransColor   = g_TransColorQ40C;
					request.m_pSrcRect     = &srcRect;
					request.m_pDestRect    = &destRect;
					request.m_bTransparent = (pfnBlit == cis_TransparentDrawQ40C);
					request.m_fAlpha       = hSrc->m_fAlpha;
					g_pRenderBlitToScreenQ40C(&request);  // 0x0040d3d1

					hDest->m_nFlags |= 4;                 // 0x0040d3dd
					return LT_OK;
				}
			}
			else if (hSrc == &g_ScreenSurface)            // 0x0040d3ec
			{
				if (g_pRenderBlitFromScreenQ40C)          // 0x0040d3f4
				{
					request.m_pSurfaceData = hDest->m_pRenderData;
					request.m_TransColor   = g_TransColorQ40C;
					request.m_pSrcRect     = &srcRect;
					request.m_pDestRect    = &destRect;
					request.m_bTransparent = (pfnBlit == cis_TransparentDrawQ40C);
					g_pRenderBlitFromScreenQ40C(&request);  // 0x0040d432
					return LT_OK;
				}
			}
		}

		bOk = LTFALSE;                                    // 0x0040d446

		pSrcBits = cis_LockSurfaceQ40C(hSrc, &nSrcPitch, LTFALSE);

		if (pSrcBits)                                     // 0x0040d4dd
		{
			if (hSrc == hDest)                            // 0x0040d4e1
			{
				if (pfnBlit(LTTRUE, pSrcBits, pSrcBits, nSrcPitch, nSrcPitch,
				            &srcRect, &destRect) == LT_OK)  // 0x0040d4f9
				{
					bOk = LTTRUE;
				}
			}
			else
			{
				pDestBits = cis_LockSurfaceQ40C(hDest, &nDestPitch, LTTRUE);

				if (pDestBits)                            // 0x0040d57d
				{
					if (pfnBlit(LTFALSE, pSrcBits, pDestBits, nSrcPitch, nDestPitch,
					            &srcRect, &destRect) == LT_OK)  // 0x0040d599
					{
						bOk = LTTRUE;
					}

					cis_UnlockSurfaceQ40C(hDest);         // 0x0040d5aa
				}
			}

			cis_UnlockSurfaceQ40C(hSrc);                  // 0x0040d5d3

			if (bOk)                                      // 0x0040d600
				return LT_OK;
		}

		Q_RETURN_ERROR(1, pErrFnName, LT_ERROR)           // 0x0040d481
	}

	Q_RETURN_ERROR_SYM(2, pErrFnName, LT_INVALIDPARAMS)   // 0x0040d612
}


// ======================================================================= //
//  0x0040d660 -- 162 bytes.  ci_DrawSurfaceToSurface's blitter callback.
// ======================================================================= //
//
//  clientinterface.cpp's ci_DrawSurfaceToSurface passes it (its sub_40d660).
//  A plain row copy: bOverlap (arg0, 0x0040d6b5) picks memmove (0x004931e0,
//  called) over an inlined memcpy (`rep movsl; rep movsb`, 0x0040d6d3).
//  The row count is spilled into pSrcRect's slot and decremented
//  (0x0040d6f0).  The two row cursors ARE the pointer parameters: with
//  separate uint8* locals VC6 spilled the source cursor instead of the
//  destination one and grouped the source sum differently (163 bytes,
//  3 blocks); walking pSrcBits / pDestBits themselves is byte-identical.
// ======================================================================= //

LTRESULT cis_CopyDrawQ40C(LTBOOL bOverlap, void* pSrcBits, void* pDestBits,
                          uint32 nSrcPitch, uint32 nDestPitch,
                          LTRect* pSrcRect, LTRect* pDestRect)
{
	uint32 nRows, nBytes, y;

	nRows  = pSrcRect->bottom - pSrcRect->top;            // 0x0040d67d
	nBytes = (pSrcRect->right - pSrcRect->left) * g_nBytesPerPixel;  // 0x0040d684

	pSrcBits = (uint8*)pSrcBits
	      + pSrcRect->top * nSrcPitch
	      + pSrcRect->left * g_nBytesPerPixel;            // 0x0040d68e

	pDestBits = (uint8*)pDestBits
	      + pDestRect->top * nDestPitch
	      + pDestRect->left * g_nBytesPerPixel;           // 0x0040d6a8

	for (y = 0; y < nRows; y++)
	{
		if (bOverlap)
			memmove(pDestBits, pSrcBits, nBytes);         // 0x0040d6c0
		else
			memcpy(pDestBits, pSrcBits, nBytes);          // 0x0040d6d3

		pSrcBits  = (uint8*)pSrcBits + nSrcPitch;         // 0x0040d6ec
		pDestBits = (uint8*)pDestBits + nDestPitch;       // 0x0040d6ee
	}

	return LT_OK;
}


// ======================================================================= //
//  0x0040d710 -- 191 bytes.  ci_DrawSurfaceToSurfaceTransparent's callback.
// ======================================================================= //
//
//  clientinterface.cpp's ci_DrawSurfaceToSurfaceTransparent passes it (its
//  sub_40d710) after converting the key colour into g_TransColor.  The
//  sibling of cis_SolidColorDraw (0x0040d1c0): same two bit-depth arms
//  re-tested per row, but it copies the SOURCE pixel where that one writes
//  g_FillColor.  bOverlap is unused.
// ======================================================================= //

LTRESULT cis_TransparentDrawQ40C(LTBOOL /*bOverlap*/, void* pSrcBits, void* pDestBits,
                                 uint32 nSrcPitch, uint32 nDestPitch,
                                 LTRect* pSrcRect, LTRect* pDestRect)
{
	uint8* pSrc;
	uint8* pDest;
	uint32 nRows, nCols, y;

	nCols = pSrcRect->right - pSrcRect->left;             // 0x0040d728
	nRows = pSrcRect->bottom - pSrcRect->top;             // 0x0040d72d

	pSrc  = (uint8*)pSrcBits
	      + pSrcRect->top * nSrcPitch
	      + pSrcRect->left * g_nBytesPerPixel;            // 0x0040d73f

	pDest = (uint8*)pDestBits
	      + pDestRect->top * nDestPitch
	      + pDestRect->left * g_nBytesPerPixel;           // 0x0040d754

	for (y = 0; y < nRows; y++)
	{
		if (g_ScreenFormat.m_nBitDepth == 2)              // 0x0040d764
		{
			uint16* pS = (uint16*)pSrc;
			uint16* pD = (uint16*)pDest;
			uint32  x;

			x = nCols;
			while (x)
			{
				if (*pS != (uint16)g_TransColorQ40C)      // 0x0040d778
					*pD = *pS;                            // 0x0040d781

				pS++;
				pD++;
				x--;
			}
		}
		else if (g_ScreenFormat.m_nBitDepth == 3)         // 0x0040d78d
		{
			uint32* pS = (uint32*)pSrc;
			uint32* pD = (uint32*)pDest;
			uint32  x;

			x = nCols;
			while (x)
			{
				if (*pS != g_TransColorQ40C)              // 0x0040d7a0
					*pD = *pS;                            // 0x0040d7a8

				pS++;
				pD++;
				x--;
			}
		}

		pSrc  += nSrcPitch;                               // 0x0040d7bd
		pDest += nDestPitch;                              // 0x0040d7bf
	}

	return LT_OK;
}


// ======================================================================= //
//  0x0040d810 -- 140 bytes.  ILTClient::DrawSurfaceMasked.
// ======================================================================= //
//
//  clientinterface.h's slot 0x19c points here, and the error site names it:
//  0x0040d83c pushes "DrawSurfaceMasked" (0x004a8180) with the code string
//  "73" (0x004a7168) -- LT_NOTINITIALIZED, double-expanded, so the
//  RETURN_ERROR form.  It lives in THIS TU (between 0x0040d660 and
//  cis_MaskedDraw), not in clientinterface.cpp beside its siblings.
//
//  The body is clientinterface.cpp's ci_ScaleSurfaceToSurfaceTransparent
//  shape: convert the key colour into g_TransColor (0x0040d869 ->
//  0x0045f5e0), then hand cis_DoDrawSurfaceToSurface (0x0040d893) the
//  callback 0x0040d8a0.  The mask cannot travel through the callback, so it
//  is parked in g_pMaskSurface first (0x0040d863).
// ======================================================================= //

LTRESULT cis_MaskedDraw(LTBOOL bOverlap, void* pSrcBits, void* pDestBits,
                        uint32 nSrcPitch, uint32 nDestPitch,
                        LTRect* pSrcRect, LTRect* pDestRect);    // 0x0040d8a0

LTRESULT ci_DrawSurfaceMaskedQ40C(CSurfaceQ40C* hDest, CSurfaceQ40C* hSrc,
                                  CSurfaceQ40C* hMask, LTRect* pSrcRect,
                                  int destX, int destY, HLTCOLOR hColor)
{
	if (!g_pRender)                                       // 0x0040d815
		Q_RETURN_ERROR(1, "DrawSurfaceMasked", LT_NOTINITIALIZED)  // 0x0040d819

	g_pMaskSurfaceQ40C = hMask;                           // 0x0040d863
	cis_GetBlitterQ40C()->ConvertColor(&g_ScreenFormat, hColor,
	                                   &g_TransColorQ40C);  // 0x0040d870

	return cis_DoDrawSurfaceToSurfaceQ40C(hDest, hSrc, pSrcRect, destX, destY,
	                                      cis_MaskedDraw);  // 0x0040d893
}


// ======================================================================= //
//  0x0040d8a0 -- 679 bytes.  ci_DrawSurfaceMasked's blitter callback.
//
//  *** EXTENT DEFECT: this row and the 425-byte "gap" row at 0x0040d99e
//  *** are ONE function.  0x0040d8a0 + 254 = 0x0040d99e exactly, and
//  *** the 254-byte row has six branch targets
//  *** leaving it -- three `je/ja 0x40db08`, two `je 0x40daca` and
//  *** `jne 0x40d99e` itself, all of which land inside the "gap".  Nothing
//  *** in .text or .data references 0x0040d99e (a data-reference search and a
//  *** full e8/e9 sweep both come back empty), and the
//  *** merged 0x0040d8a0..0x0040db47 has ZERO targets outside and an
//  *** exact end.  679 bytes, one function.
// ======================================================================= //
//
//  IT NAMES ITSELF.  0x0040d973 and 0x0040dade/0x0040db1c push the dword at
//  0x004a8080, which dereferences to "cis_MaskedDraw" --
//  the `static char*` form of de_error.h's name argument, shared by all
//  three check sites.  That is the one string-pool reading that
//  can outrank a vtable, and it settles the whole module prefix.
//
//  WHAT IT IS.  0x0040d881 is `push $0x40d8a0` inside ci_DrawSurfaceMasked
//  (0x0040d810, reporting "DrawSurfaceMasked", 0x004a8180), whose third
//  parameter -- clientinterface.h's `HSURFACE hMask` -- is parked in the
//  global at 0x004b15f8 (0x0040d863) rather than passed, because
//  cis_DoDrawSurfaceToSurface's callback signature has no room for it.
//
//  THE MASK IS TILED, which is what the dimension table is for.  Both mask
//  dimensions must be <= 0x100 (0x0040d8b5 / 0x0040d8c4, `ja`, so
//  unsigned), and each indexes the byte table at 0x004b164c whose value is
//  then used as an AND mask on the row (0x0040da0e) and the column
//  (0x0040da37).  A zero entry is rejected, so the table is (n-1) for a
//  power of two and 0 otherwise.
//
//  ONLY THE 16-BIT FORMAT IS IMPLEMENTED.  0x0040d9fa is
//  `cmpl $0x2,0x4b175c; jne` straight to the row advance -- no second
//  compare against 3, and a direct memory compare rather than the
//  load-then-two-compares its sibling at 0x0040d1c0 emits.
//
//  THE TWO `LT_INVALIDPARAMS` BLOCKS are two statements, not two arms of
//  one: three guards share 0x0040db08 and two share 0x0040daca, which is
//  exactly one `||` chain of three followed by one of two.  The `jl` inside
//  the second block jumps into the FIRST block's tail (0x0040db3c) -- an
//  automatic tail merge, not a third statement.
//
//  THE THREE `LT_ERROR` EXITS are one block (0x0040d95f), reached by
//  `je` from the g_pRender test, `jmp` from the LockScreen failure and
//  fall-through from the `if (!pMaskBits)` -- i.e. three written-out early
//  returns that VC6 hosted once because the expansion is 25 instructions.
//  Its code string is "1" (0x004a7494), the double-expanded form, where
//  the two LT_INVALIDPARAMS sites push "LT_INVALIDPARAMS" (0x004a77c8) --
//  so this site is RETURN_ERROR and those two are RETURN_ERROR_SYM, both
//  through the one shared name variable.
//
//  THE MASK SURFACE IS RE-READ FROM THE GLOBAL at the unlock (0x0040da8d)
//  rather than kept in a local, which means the source
//  writes the global out at every mention; the guard block's single load
//  is VC6 CSEing three uses that no call separates.
//
//  STILL NEAR: 671 vs 679.  The row cursors are the pointer PARAMETERS
//  (shipped advances them in their own arg slots, 0x0040da72 / 0x0040da7f,
//  and its frame is 0x14 -- five locals; separate uint8* cursors gave 0x18,
//  678 bytes, and a different loop body).  What is left is allocation:
//  shipped gives nMaskH %ebp from 0x0040d8e6 right through the lock call to
//  the row loop (so %ebp is pushed on entry and popped on all five exits),
//  where ours spills it and gives %ebp to nRows.  No effect: local
//  declaration order (3 orders), nCols before nRows, a named mask-row
//  local, a uint8* pMaskBits, for(i=nCols;i;i--), `y != nRows`.
// ======================================================================= //

LTRESULT cis_MaskedDraw(LTBOOL /*bOverlap*/, void* pSrcBits, void* pDestBits,
                        uint32 nSrcPitch, uint32 nDestPitch,
                        LTRect* pSrcRect, LTRect* pDestRect)
{
	static char* pErrFnName = "cis_MaskedDraw";           // 0x004a8080

	uint32 nMaskW, nMaskH, nMaskPitch;
	void*  pMaskBits;
	uint32 nRows, nCols, y;

	if (!g_pMaskSurfaceQ40C                                       // 0x0040d8a8
	    || g_pMaskSurfaceQ40C->m_nWidth  > 0x100                  // 0x0040d8b5
	    || g_pMaskSurfaceQ40C->m_nHeight > 0x100)                 // 0x0040d8c4
	{
		Q_RETURN_ERROR_SYM(2, pErrFnName, LT_INVALIDPARAMS)       // 0x0040db08
	}

	nMaskW = g_DimMaskTableQ40C[g_pMaskSurfaceQ40C->m_nWidth];    // 0x0040d8d2
	nMaskH = g_DimMaskTableQ40C[g_pMaskSurfaceQ40C->m_nHeight];   // 0x0040d8dc

	if (!nMaskW || !nMaskH)                                       // 0x0040d8e8 / 0x0040d8f4
	{
		Q_RETURN_ERROR_SYM(2, pErrFnName, LT_INVALIDPARAMS)       // 0x0040daca
	}

	pMaskBits = cis_LockSurfaceQ40C(g_pMaskSurfaceQ40C, &nMaskPitch, LTFALSE);

	if (!pMaskBits)                                               // 0x0040d95b
	{
		Q_RETURN_ERROR(1, pErrFnName, LT_ERROR)                   // 0x0040d95f
	}

	nRows = pSrcRect->bottom - pSrcRect->top;             // 0x0040d9b5
	nCols = pSrcRect->right - pSrcRect->left;             // 0x0040d9bc

	pSrcBits = (uint8*)pSrcBits
	      + pSrcRect->top * nSrcPitch                     // 0x0040d9b7
	      + pSrcRect->left * g_nBytesPerPixel;            // 0x0040d9be / 0x0040d9c9

	pDestBits = (uint8*)pDestBits
	      + pDestRect->top * nDestPitch                   // 0x0040d9d8
	      + pDestRect->left * g_nBytesPerPixel;           // 0x0040d9dd / 0x0040d9e8

	for (y = 0; y < nRows; y++)                           // 0x0040d9ee / 0x0040da7c
	{
		if (g_ScreenFormat.m_nBitDepth == 2)              // 0x0040d9fa
		{
			uint16* pMaskRow;
			uint16* pS;
			uint16* pD;
			uint32  x;
			uint32  i;

			pMaskRow = (uint16*)((uint8*)pMaskBits
			         + ((pSrcRect->top + y) & nMaskH) * nMaskPitch);  // 0x0040da03..0x0040da15
			x  = pSrcRect->left;                          // 0x0040da06
			pS = (uint16*)pSrcBits;                       // 0x0040da0a
			pD = (uint16*)pDestBits;                      // 0x0040da1d

			i = nCols;
			while (i)                                     // 0x0040da19 / 0x0040da4b
			{
				if (*pS != (uint16)g_TransColorQ40C)      // 0x0040da29 / 0x0040da2c
					*pD = pMaskRow[x & nMaskW];           // 0x0040da37 / 0x0040da3b

				x++;                                      // 0x0040da47
				pS++;                                     // 0x0040da48
				pD++;
				i--;
			}
		}

		pSrcBits  = (uint8*)pSrcBits + nSrcPitch;         // 0x0040da6c
		pDestBits = (uint8*)pDestBits + nDestPitch;       // 0x0040da7a
	}

	cis_UnlockSurfaceQ40C(g_pMaskSurfaceQ40C);            // 0x0040da8d

	return LT_OK;                                         // 0x0040daa8 / 0x0040dac2
}


// ======================================================================= //
//  0x0040ee40 -- 100 bytes.  Destroy every surface and empty the pool.
// ======================================================================= //
//
//  WHAT PINS IT.  Its only caller is cis_Term (0x0040ee10) and it takes no
//  arguments (no push before `call 0x40ee40`, no cleanup after).  The walk
//  is clientmgr2.cpp's surface_SaveAll walk exactly -- circular list
//  through m_pNext at +0x04, head re-read inside the loop, wrap detected by
//  `sub/neg/sbb/and` against it -- which is what identifies the list and
//  the link offset without needing the allocator.
//
//  The tail after the loop is `g_pSurfaceList = 0; g_nSurfaceCount = 0`,
//  emitted TWICE (0x0040ee86 and 0x0040ee95): one copy for the loop's exit
//  and one for the empty-list case, which is /O2 rotating the `while` and
//  duplicating the trailing statements rather than a second statement in
//  the source.
//
//  The two frees go in the order sysmem (0x0040c5d0, `dfree` on +0x08)
//  then renderer data (0x0040c5a0, render slot +0xcc on +0x0c); VC6 merges
//  their two pushes into one `add $0x8,%esp` at 0x0040ee76.
//
//  SHAPE.  `mov %edi,%esi ; mov 0x4(%edi),%edi` then `sub/neg/sbb/and` on
//  %edi: ONE cursor stepped in place (the surface freed is a copy taken
//  first) and the wrap test a ternary -- VC6's branchless select.  The
//  surface_SaveAll spelling (separate pNext, `if (pNext == head) pNext =
//  0`) branches here instead: 94 bytes, near; a ternary on a separate
//  pNext was 99, near.
//
//  The last two instructions of the body are stdlith's sb_Free expanded:
//  the surface's own first dword becomes the bank's free-list link.
// ======================================================================= //

void cis_FreeAllSurfacesQ40C()
{
	CSurfaceQ40C* pCur = g_SurfaceListQ40C.m_pHead;                  // 0x0040ee42

	while (pCur)                                          // 0x0040ee4a / 0x0040ee7b
	{
		CSurfaceQ40C* pSurface = pCur;                    // 0x0040ee55
		pCur = pCur->m_pNext;                             // 0x0040ee57
		pCur = (pCur == g_SurfaceListQ40C.m_pHead) ? LTNULL : pCur;  // 0x0040ee5b..63

		cis_FreeSysMemQ40C(pSurface);                     // 0x0040ee65
		cis_FreeRenderDataQ40C(pSurface);                 // 0x0040ee6b

		pSurface->m_pPrev = (CSurfaceQ40C*)g_SurfaceBankQ40C.m_pFreeList;  // 0x0040ee79
		g_SurfaceBankQ40C.m_pFreeList = pSurface;         // 0x0040ee7d
	}

	g_SurfaceListQ40C.m_pHead = LTNULL;                              // 0x0040ee86
	g_SurfaceListQ40C.m_nElements = 0;                              // 0x0040ee8c
}


// ======================================================================= //
//  0x0040ed50 -- 190 bytes.  ILTClient::GetEngineHook.
// ======================================================================= //
//
//  clientinterface.h slot 0x330.  It sits between ci_SetSurfaceAlpha
//  (0x0040ecc0) and cis_Term (0x0040ee10), so it is written in this TU.
//  An if / else-if chain of _stricmp (0x00492f30) tests:
//    "hwnd"             -> *pData = g_hMainWnd (0x004b0b04), LT_OK
//    "cres_hinstance"   -> bm_GetInstanceHandle(mgr+0x610, pData)
//    "cresl_hinstance"  -> bm_GetInstanceHandle(mgr+0x614, pData)
//    "cshell_hinstance" -> bm_GetInstanceHandle(
//                              clm_GetRecordModule(mgr+0x618), pData)
//  with the manager the pointer at 0x004b0b14 (ccline.cpp's
//  g_pCcnClientMgr), and LT_ERROR (1) for any other name.  The three
//  forwards return bm_GetInstanceHandle's (0x00401770) answer.
// ======================================================================= //

// 0x004b0b04 (the window) and 0x004b0b14 (the manager, read here through
// the view below) are fields of clientwin.cpp's g_CluClientGlob.

class CEngineHookMgrQ40C
{
public:
	uint8  m_pad_000[0x610];
	void*  m_pClientResModule;               // 0x610  0x0040ed93
	void*  m_pClientResLModule;              // 0x614  0x0040edc0
	void*  m_pShellRecord;                   // 0x618  0x0040eded
};

LTRESULT bm_GetInstanceHandleQ40C(void* pModule, void** phInst); // 0x00401770
void*    clm_GetRecordModuleQ40C(void* pRecord);                 // 0x00477f30

LTRESULT ci_GetEngineHookQ40C(char* pName, void** pData)
{
	if (_stricmp(pName, "hwnd") == 0)                    // 0x0040ed5b
	{
		*pData = g_CluClientGlob.m_hMainWnd;                          // 0x0040ed72
		return LT_OK;
	}
	else if (_stricmp(pName, "cres_hinstance") == 0)      // 0x0040ed7d
	{
		return bm_GetInstanceHandleQ40C(
			((CEngineHookMgrQ40C*)g_CluClientGlob.m_pClientMgr)->m_pClientResModule, pData);   // 0x0040ed9a
	}
	else if (_stricmp(pName, "cresl_hinstance") == 0)     // 0x0040edaa
	{
		return bm_GetInstanceHandleQ40C(
			((CEngineHookMgrQ40C*)g_CluClientGlob.m_pClientMgr)->m_pClientResLModule, pData);  // 0x0040edc7
	}
	else if (_stricmp(pName, "cshell_hinstance") == 0)    // 0x0040edd7
	{
		return bm_GetInstanceHandleQ40C(
			clm_GetRecordModuleQ40C(((CEngineHookMgrQ40C*)g_CluClientGlob.m_pClientMgr)->m_pShellRecord),
			pData);                                       // 0x0040edfd
	}

	return 1;                                             // 0x0040ee07 LT_ERROR
}


// ======================================================================= //
//  0x0040ee10 -- 34 bytes.  Shut the surface system down.
// ======================================================================= //
//
//  PROVENANCE NOTE.  The enumeration labels this row `thunk`.  The thunk is
//  NOT this function: 0x0040bfd0 is a bare five-byte `jmp 0x40ee10` sitting
//  alone between two padded bodies, and that is what the enumerator found
//  the address through.  0x0040ee10 itself is four statements and a `ret`,
//  so it is kept, and 0x0040bfd0 -- which is not enumerated -- is the
//  scaffolding.
//
//  0x00490c10 is sb_FreeChunks(pBank, 0), which clientmgr3.cpp already
//  names sb_Term; 0x004b1600 is the surface bank cis_CreateSurface
//  allocates out of (0x0040c1b2 grows it through 0x00490c20).  0x00489510
//  is the font module's GDI teardown (fnt_TermQ40C, end of this file).  The last act is to drop
//  g_pRender, which is what makes this the surface half of the render-DLL
//  shutdown.
// ======================================================================= //

void cis_TermQ40C()
{
	cis_FreeAllSurfacesQ40C();                            // 0x0040ee10
	sb_TermQ40C(&g_SurfaceBankQ40C);                      // 0x0040ee1a
	fnt_TermQ40C();                                       // 0x0040ee22
	g_pRender = LTNULL;                                   // 0x0040ee27
}


// ======================================================================= //
//  0x0040ef30 -- 293 bytes.  Put one surface back into the render DLL.
// ======================================================================= //
//
//  WHAT PINS IT.  It is the exact mirror of surface_CopyOut (0x0040f150),
//  which src/engine/lithtech/clientmgr2.cpp already declares by that name
//  and never defines; its only caller is the walker at 0x0040eef0, which is
//  to surface_SaveAll (0x0040f110) what this is to surface_CopyOut -- same
//  circular walk, same LTBOOL answer, same bail on the first failure.  The
//  pair runs across a render-mode change: 0x0040f060 shadows the screen
//  PFormat into 0x004b15c0 and copies every surface OUT to system memory
//  before dropping g_pRender; this reads that shadow back as the SOURCE
//  format and the live one as the DESTINATION.
//
//  So the direction is unambiguous and so is the asymmetry: the save side
//  is a plain `rep movs` row copy (0x0040f1b2) because nothing can have
//  changed yet, and the restore side has to go through the blitter because
//  the new mode's format may differ from the old one's.
//
//  Render slots used, all through g_pRender: +0xc8 CreateSurface(w, h)
//  whose answer becomes m_pRenderData (0x0040ef5f); +0xd0
//  GetSurfaceInfo(data, &w, &h, &pitch) with the pitch written straight
//  into CSurface+0x18 (`lea 0x18(%esi),%edi`, 0x0040ef6f) and the two
//  dimensions into throwaway locals; +0xd4/+0xd8 the lock pair; +0xdc
//  OptimizeSurface, run only when m_nFlags bit 1 survived the round trip.
//
//  THREE SEPARATE `return LTFALSE` EPILOGUES (0x0040ef66, 0x0040efac,
//  0x0040f016) -- /O2 duplicating rather than sharing, so these are three
//  early returns and not nested guards.  The first two need no
//  `xor %eax,%eax` because the value they tested is already the zero.
//
//  BYTE-IDENTICAL.  The nine request stores are scheduled into the gaps
//  between three calls (0x0040ef93 lands between GetSurfaceInfo and
//  LockSurface); the order below was found by measurement.
// ======================================================================= //

LTBOOL surface_CopyInQ40C(CSurfaceQ40C* pSurface)
{
	BlitRequestQ40C request;                              // 0x0040ef3c
	uint32          nWidth, nHeight;
	LTRESULT        dResult;

	pSurface->m_pRenderData = g_pRender->CreateSurface(pSurface->m_nWidth,
	                                                   pSurface->m_nHeight);  // 0x0040ef56
	if (!pSurface->m_pRenderData)                         // 0x0040ef62
		return LTFALSE;

	g_pRender->GetSurfaceInfo(pSurface->m_pRenderData, &nWidth, &nHeight,
	                          &pSurface->m_nPitch);       // 0x0040ef83

	// The lock's answer goes STRAIGHT into the request (0x0040efa4 stores
	// it before the 0x0040efa8 test) -- no pBits local -- and the dest
	// format is set before the lock (0x0040ef93, scheduled into the call's
	// argument setup).  The rest is in the order that reproduces shipped's
	// schedule; each other order measured moves a store across the push.
	request.m_pDestFormat  = &g_ScreenFormat;             // 0x0040ef93
	request.m_pDest        = g_pRender->LockSurface(pSurface->m_pRenderData);   // 0x0040ef9b / 0x0040efa4
	if (!request.m_pDest)                                 // 0x0040efa8
		return LTFALSE;

	request.m_pSrcFormat   = &g_SavedScreenFormatQ40C;    // 0x0040efd9
	request.m_nDestPitch   = pSurface->m_nPitch;          // 0x0040efba
	request.m_pSrc         = pSurface->m_pSysMem;         // 0x0040efc3
	request.m_nSrcPitch    = pSurface->m_nWidth * g_nSavedBytesPerPixelQ40C;  // 0x0040efc7
	request.m_nWidth       = pSurface->m_nWidth;          // 0x0040efce
	request.m_nHeight      = pSurface->m_nHeight;         // 0x0040efe6
	request.m_pDestPalette = LTNULL;                      // 0x0040efea

	dResult = cis_GetBlitterQ40C()->Blt(&request);        // 0x0040eff2 / 0x0040eff9

	g_pRender->UnlockSurface(pSurface->m_pRenderData);    // 0x0040f009

	if (dResult)                                          // 0x0040f012
		return LTFALSE;

	cis_FreeSysMemQ40C(pSurface);                         // 0x0040f022

	if (pSurface->m_nFlags & 2)                           // 0x0040f02d
	{
		g_pRender->OptimizeSurface(pSurface->m_pRenderData,
		                           pSurface->m_hTransColor);   // 0x0040f03e
	}

	return LTTRUE;                                        // 0x0040f048
}


// ======================================================================= //
//  0x00489510 -- 107 bytes.  The GDI font module's teardown.
// ======================================================================= //
//
//  REAL HOME: src/engine/lithtech/fntdraw.cpp, PART ONE (the GDI font
//  module, 0x00488980..0x004895ae).  Written here, where its one caller
//  cis_Term (0x0040ee22) is; move it (and drop the externs below) when
//  fntdraw.cpp is next open.  Its first act is 0x00489580, fnt_FreeAllFonts, which today lives
//  in src/engine/lithtech/cmgsurf.cpp PART B as fnt_FreeAllFontsQ428.
//
//  All four globals are fntdraw.cpp's, at the addresses it defines them:
//  g_hFontDC 0x004b4d04, g_hDIB 0x004b4d08, g_hOldDIB 0x004b4d0c,
//  g_hOldBrush 0x004b4d10.  The two SelectObject()s restore the brush
//  first (0x0048952b pushes 0x004b4d10), then the DIB (0x0048953a), under
//  ONE test of the DC (0x0048951a); DeleteObject is under its own test
//  (0x00489551).  The stores are old-DIB, DIB, DC (0x0048955c..70); the
//  old brush is NOT cleared.
// ======================================================================= //

void fnt_FreeAllFontsQ40C();                                 // 0x00489580

extern HDC     g_hFontDCQ488;                                // 0x004b4d04
extern HBITMAP g_hDIBQ488;                                   // 0x004b4d08
extern HGDIOBJ g_hOldDIBQ488;                                // 0x004b4d0c
extern HGDIOBJ g_hOldBrushQ488;                              // 0x004b4d10

void fnt_TermQ40C()
{
	fnt_FreeAllFontsQ40C();                               // 0x00489510

	if (g_hFontDCQ488)                                    // 0x0048951a
	{
		SelectObject(g_hFontDCQ488, g_hOldBrushQ488);     // 0x0048952d
		SelectObject(g_hFontDCQ488, g_hOldDIBQ488);       // 0x0048953c
		DeleteDC(g_hFontDCQ488);                          // 0x00489545
	}

	if (g_hDIBQ488)                                       // 0x00489551
		DeleteObject(g_hDIBQ488);                         // 0x00489556

	g_hOldDIBQ488 = LTNULL;                               // 0x0048955c
	g_hDIBQ488    = LTNULL;                               // 0x00489566
	g_hFontDCQ488 = LTNULL;                               // 0x00489570
}


// ======================================================================= //
//  0x00489580 -- 47 bytes.  fnt_FreeAllFonts: free every GDI font.
// ======================================================================= //
//
//  REAL HOME: fntdraw.cpp PART ONE, straight after fnt_Term above (its
//  only caller, 0x00489510).  src/engine/lithtech/cmgsurf.cpp PART B has
//  an earlier 42-byte attempt, fnt_FreeAllFontsQ428, that never placed;
//  this one supersedes it and that copy should be deleted when the file
//  is next open.  The shape is cis_FreeAllSurfaces' exactly: one cursor
//  stepped in place after a copy is taken (0x00489591 `mov %esi,%eax`,
//  0x00489593 `mov 0x4(%esi),%esi`), the head RE-READ each pass
//  (0x0048958b), and the wrap test a ternary -- the sub/neg/sbb/and at
//  0x00489599 -- computed before the free (0x004895a1).
// ======================================================================= //

class CFontQ40C
{
public:
	CFontQ40C* m_pPrev;                            // 0x00
	CFontQ40C* m_pNext;                            // 0x04
	HFONT      m_hFont;                            // 0x08
};

extern CFontQ40C* g_pFontListQ40C;                 // 0x004b4d00 (fntdraw.cpp's g_pFontList)

void fnt_FreeFontQ40C(CFontQ40C* pFont);           // 0x00488b20

void fnt_FreeAllFontsQ40C()
{
	CFontQ40C* pCur = g_pFontListQ40C;             // 0x00489581

	while (pCur)                                   // 0x00489587 / 0x004895a9
	{
		CFontQ40C* pFont = pCur;                   // 0x00489591
		pCur = pCur->m_pNext;                      // 0x00489593
		pCur = (pCur == g_pFontListQ40C) ? LTNULL : pCur;  // 0x00489597..9f

		fnt_FreeFontQ40C(pFont);                   // 0x004895a1
	}
}


