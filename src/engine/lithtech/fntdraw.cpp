// *********************************************************************** //
//
//  MODULE:   W8_488000 (.CPP)   -- scratch reconstruction, lithtech.exe only
//
//  THE GDI FONT module, 0x00488980..0x0048950c (AvP2 1.0.9.6; Primal Hunt
//  0x004a58b0..). This file held two more translation units until they
//  were split out: the Smack/DirectDraw video module (videomgr.cpp) and
//  the visibility walk (visquery.cpp). VC6's literal pooling and block
//  merging are per-TU ("A scratch translation unit can invent a
//  residual"). The PART ONE notes below are this module's.
//
// ======================================================================= //
//
//  PART ONE -- WHAT THE MODULE IS, AND HOW IT WAS IDENTIFIED
//
//  0x00488980 stores seven function addresses into an object it is handed:
//
//      +0x13c <- 0x00488a60      +0x18c <- 0x00488c20
//      +0x140 <- 0x00488b10      +0x190 <- 0x00488cb0
//      +0x144 <- 0x00488b80      +0x170 <- 0x00489440
//      +0x148 <- 0x00488bd0
//
//  src/engine/lithtech/clientinterface.h already types that object: it is
//  CLTClient, the flat interface table the engine hands the game DLL, and
//  its field at 0x020 + 4*i is the i'th function-pointer member of
//  src/LT2/sdk/inc/iltclient.h.  The seven displacements above are, in the
//  header's own order,
//
//      0x13c CreateFont              0x18c GetStringDimensions
//      0x140 DeleteFont              0x190 DrawStringToSurface
//      0x144 SetFontExtraSpace       0x170 CreateSurfaceFromString
//      0x148 GetFontExtraSpace
//
//  and those are EXACTLY the seven slots clientinterface.h records as "not
//  set by the shipped engine".  They are set -- just not by the big setter.
//  0x00488980 is called from one site only, 0x0040c996, at the tail of the
//  surface module's own init (which fills +0x1d0/+0x1d4 of the same object
//  twenty instructions earlier), so the font API is installed as a second
//  pass.  That is a correction to clientinterface.h's header comment and the
//  most useful thing this cluster establishes.
//
//  Confirmation from the imports, in address order: CreateCompatibleDC,
//  GetStockObject, SelectObject, SetTextColor, SetBkMode, SetBkColor,
//  DeleteObject, SetTextCharacterExtra, GetTextCharacterExtra,
//  GetTextExtentPoint32A, CreateDIBSection, Rectangle, TextOutA, CreateFontA.
//  Every one is GDI32 and every one is reached from this 0xb8c-byte run and
//  nowhere else in the image.
//
//  THE MODULE'S STATE, read off the .data addresses it touches:
//
//      0x004b4cf8  CGLinkedList<CFont*> g_FontList: vptr, then the count
//                  (0x004b4cfc) and the circular list's head (0x004b4d00)
//      0x004b4d04  HDC      g_hFontDC        CreateCompatibleDC(NULL)
//      0x004b4d08  HBITMAP  g_hDIB           CreateDIBSection's answer
//      0x004b4d0c  HGDIOBJ  g_hOldDIB        SelectObject's answer
//      0x004b4d10  HGDIOBJ  g_hOldBrush      SelectObject(BLACK_BRUSH)
//      0x004b4d14  int      g_nDIBWidth      pSize->cx, rounded up to 32
//      0x004b4d18  int      g_nDIBHeight     pSize->cy
//      0x004b4d1c  int      g_nDIBPitch      g_nDIBWidth * 2  (16bpp)
//      0x004b4d20  void*    g_pDIBBits       CreateDIBSection's out-param
//
//  THE FONT RECORD is twelve bytes -- `push $0xc; call 0x00428fd0` (dalloc,
//  not operator new) -- and the delete path
//  splices it out of a circular doubly-linked list, so
//
//      +0x00 CFont* m_pPrev      +0x04 CFont* m_pNext      +0x08 HFONT
//
//  with +0x00 the PREV link: 0x00488b2f reads (%esi) and 0x4(%esi) and
//  writes `(%esi)->[4] = 0x4(%esi)` then `0x4(%esi)->[0] = (%esi)`, i.e.
//  prev->next = next; next->prev = prev. That is goodlinklist.h's CGLLNode
//  (m_pGPrev, m_pGNext), and the list is a CGLinkedList: CreateFont's splice
//  is AddHead's InsertBefore and FreeFont's is RemoveAt, both expanded.
//
//  THE HSTRING ACCESSORS, 0x00484a30 and 0x00484a50, are just outside this
//  cluster and are read off their own four instructions:
//
//      0x00484a30  uint32 GetLen(HSTRING h)
//                    { if (!h) return 0; return (uint16)h->+0x10; }
//      0x00484a50  char*  GetData(HSTRING h, uint32* pLen)
//                    { if (!h) { if (pLen) *pLen = 0; return NULL; }
//                      if (pLen) *pLen = (uint16)h->+0x12;
//                      return (char*)h + 0x14; }
//
//  Both are declared, not defined, below: they are not in the enumeration and
//  defining them here would collide with the file that defines 0x00484a00.
//
//  EXTENTS.  All eight PART ONE rows are clean extents -- no branch
//  target leaves any of them and every last instruction ends exactly on the
//  stated boundary.  The four `setup`-sourced rows flagged as less
//  reliable (0x00488a60/170, 0x00488c20/144, 0x00488cb0/151, 0x00489440/204)
//  are all genuine single functions: each is a whole slot body, each is
//  reached by a `movl $addr` in 0x00488980, and the multiple `ret`s
//  they have are duplicated /O2 epilogues (0x00488a60's three rets
//  are its CreateFontA-failed early return plus the two arms of the list
//  splice; 0x00488c20's two are its body and its zero-the-outputs tail).
//
// ======================================================================= //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x4a64e0).
#include <string>
#endif
#include <windows.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "ltrect.h"
#include "../../LT2/lithshared/stdlith/goodlinklist.h"


// ----------------------------------------------------------------------- //
//  The three foreign types this file needs, cut down to the offsets the
//  bytes actually prove.  Same shape clientblit.cpp and diblit.cpp use.
// ----------------------------------------------------------------------- //

// +0x0c, +0x10, +0x14, +0x18, +0x1c are all read in 0x00488f90; the widths
// and the meaning of bit 2 come from clientblit.cpp/diblit.cpp, which prove
// them against the renderer's own lock/unlock slots.
class CSurfaceQ488
{
public:
	uint8  m_pad_00[0x0c];
	void*  m_pRenderData;                        // 0x00c
	int    m_nWidth;                             // 0x010
	int    m_nHeight;                            // 0x014
	uint32 m_nPitch;                             // 0x018
	uint32 m_nFlags;                             // 0x01c
};

// The four render slots this file uses, with clientblit.cpp's prototypes.
class RenderStructQ488
{
public:
	uint8   m_pad_00[0xd4];
	// AVP2: as Primal Hunt (PH addresses) -- the renderer half of the RenderStruct is two entries longer below
	// here (cissurf.cpp, clientblit.cpp).
	uint8 m_padExtra[8];
	void*  (*LockSurface)(void* pRenderData);                       // 0x0d4
	void   (*UnlockSurface)(void* pRenderData);                     // 0x0d8
	uint8   m_pad_dc[0xe4 - 0xdc];
	LTBOOL (*LockScreen)(int nLeft, int nTop, int nRight, int nBottom,
	                     void** ppPixels, uint32* pPitch);          // 0x0e4
	void   (*UnlockScreen)();                                       // 0x0e8
};

class CFontQ488;

// CLTClient, cut to the seven slots 0x00488980 writes.  Displacements and
// signatures both from src/engine/lithtech/clientinterface.h.
class CLTClientQ488
{
public:
	uint8 m_pad_000[0x13c];
	// AVP2: as Primal Hunt (PH addresses) -- one more entry below here (0x004a58b9 stores CreateFont at +0x140).
	uint8 m_padExtra[4];
	CFontQ488* (*CreateFont)(char* pFontName, int width, int height,
	                         LTBOOL bItalic, LTBOOL bUnderline,
	                         LTBOOL bBold);                            // 0x13c
	void       (*DeleteFont)(CFontQ488* hFont);                        // 0x140
	LTRESULT   (*SetFontExtraSpace)(CFontQ488* hFont, int pixels);     // 0x144
	LTRESULT   (*GetFontExtraSpace)(CFontQ488* hFont, int& pixels);    // 0x148
	uint8 m_pad_14c[0x170 - 0x14c];
	CSurfaceQ488* (*CreateSurfaceFromString)(CFontQ488* hFont,
	                                         HSTRING hString,
	                                         HLTCOLOR hForeColor,
	                                         HLTCOLOR hBackColor,
	                                         int extraPixelsX,
	                                         int extraPixelsY);        // 0x170
	uint8 m_pad_174[0x18c - 0x174];
	void (*GetStringDimensions)(CFontQ488* hFont, HSTRING hString,
	                            int* sizeX, int* sizeY);               // 0x18c
	void (*DrawStringToSurface)(CSurfaceQ488* hDest, CFontQ488* hFont,
	                            HSTRING hString, LTRect* pRect,
	                            HLTCOLOR hForeColor,
	                            HLTCOLOR hBackColor);                  // 0x190
};


// ----------------------------------------------------------------------- //
//  Externals.  Every address is a masked relocation, so only the SHAPE has
//  to be right; the comment carries the address the shipped code uses.
// ----------------------------------------------------------------------- //

void* dallocQ488(unsigned long size);              // 0x00428fd0
void  dfreeQ488(void* ptr);                        // 0x00429080

uint32 str_GetLenQ488(HSTRING hString);                    // 0x00484a30
char*  str_GetDataQ488(HSTRING hString, uint32* pLen);     // 0x00484a50

// 0x0043a2a0 -- the twelve-argument blit clipper.  It is handed the source
// surface's dimensions, the source rect, the destination's dimensions and
// the destination origin, and it fills the two out-rects.  Non-zero means
// something survived the clip.  Argument order read straight off the twelve
// pushes at 0x00488fc8..0x00489004.  Defined (and placed) in diblit.cpp
// under this name.
LTBOOL cis_ClipBlitQ438(int nSrcWidth, int nSrcHeight,
                         int nSrcLeft, int nSrcTop,
                         int nSrcRight, int nSrcBottom,
                         int nDestWidth, int nDestHeight,
                         int nDestX, int nDestY,
                         LTRect* pOutSrc, LTRect* pOutDest);

// 0x0040c130 -- cis_CreateSurface(width, height).  Two dwords, cdecl,
// answer null-tested (0x004894cf).
CSurfaceQ488* cis_CreateSurfaceQ488(int nWidth, int nHeight);

// The colour converter.  0x0040c120 is a two-instruction accessor
// (`*(char**)0x004b17e0 + 0x1618`) returning the object, and 0x0045f5e0 is
// a thiscall on it with `ret $0xc`, dispatching through the table at
// 0x004aceb8 on the PFormat's +0x04.  diblit.cpp and fmconvert.cpp both
// name 0x0045f5e0 as the routine clientinterface.cpp uses to turn an
// HLTCOLOR into a pixel of the screen's format.
class PFormatQ488;

class CColorOpsQ488
{
public:
	LTRESULT ConvertColor(PFormatQ488* pFormat, HLTCOLOR hColor,
	                      uint32* pOut);
};

CColorOpsQ488* GetColorOpsQ488();                  // 0x0040c120

// 0x004b1758 -- the screen PFormat.  lttexmod.cpp already types this class
// and reads the bit-depth constants off 0x0045f390: 2 is 565 and 3 is 32.
// Only the first two dwords are touched here, so only they are declared;
// the vptr is a plain dword because declaring the virtual would demand a
// definition of Init that belongs to lttexmod.cpp.
class PFormatQ488
{
public:
	void*  m_pVTable;                              // 0x00
	uint32 m_nBitDepth;                            // 0x04  -> 0x004b175c
};

extern PFormatQ488 g_ScreenFormatQ488;             // 0x004b1758

#define FMT_565_Q488   2
#define FMT_32_Q488    3

extern RenderStructQ488* g_pRenderQ488;            // 0x004b17a4
extern CSurfaceQ488      g_ScreenSurfaceQ488;      // 0x004b1620
// The screen size (0x004b1630 / 0x004b1634) is the screen surface's own
// m_nWidth / m_nHeight (+0x10 / +0x14), not a global of its own.
extern uint32            g_nBytesPerPixel;         // 0x004b1794


// ----------------------------------------------------------------------- //
//  The font record and the module's state.
// ----------------------------------------------------------------------- //

class CFontQ488 : public CGLLNode
{
public:
	HFONT      m_hFont;                            // 0x08
};

// The fonts. Its initialiser set opens this unit: Primal Hunt 0x004a5850
// (call the constructor body 0x004a5860: the vptr 0x004d6b24, then
// m_nElements and m_pHead zeroed; jmp to 0x004a5880, atexit of 0x004a5890,
// the destructor: the vptr, then RemoveAll's m_pHead before m_nElements).
// The vtable is referenced by nothing else.
static CGLinkedList<CFontQ488*> g_FontList;        // 0x004b4cf8, PH 0x004f73e0
HDC        g_hFontDCQ488;                          // 0x004b4d04
HBITMAP    g_hDIBQ488;                             // 0x004b4d08
HGDIOBJ    g_hOldDIBQ488;                          // 0x004b4d0c
HGDIOBJ    g_hOldBrushQ488;                        // 0x004b4d10
int        g_nDIBWidthQ488;                        // 0x004b4d14
int        g_nDIBHeightQ488;                       // 0x004b4d18
int        g_nDIBPitchQ488;                        // 0x004b4d1c
void*      g_pDIBBitsQ488;                         // 0x004b4d20


// ----------------------------------------------------------------------- //
//  Forward declarations.  The definition order below is the shipped COFF
//  order, which is source definition order -- and the
//  inliner being forward-only is why 0x00488d50 CALLS 0x00488ea0 and
//  0x00488f90 rather than expanding them.
// ----------------------------------------------------------------------- //

CFontQ488* fnt_CreateFontQ488(char* pFontName, int width, int height,
                              LTBOOL bItalic, LTBOOL bUnderline, LTBOOL bBold);
void       fnt_DeleteFontQ488(CFontQ488* hFont);
void       fnt_FreeFontQ488(CFontQ488* pFont);
LTRESULT   fnt_SetFontExtraSpaceQ488(CFontQ488* hFont, int pixels);
LTRESULT   fnt_GetFontExtraSpaceQ488(CFontQ488* hFont, int& pixels);
void       fnt_GetStringDimensionsQ488(CFontQ488* hFont, HSTRING hString,
                                       int* sizeX, int* sizeY);
void       fnt_DrawStringToSurfaceQ488(CSurfaceQ488* hDest, CFontQ488* hFont,
                                       HSTRING hString, LTRect* pRect,
                                       HLTCOLOR hForeColor,
                                       HLTCOLOR hBackColor);
void       fnt_DrawStringQ488(CSurfaceQ488* hDest, CFontQ488* hFont,
                              HSTRING hString, LTRect* pRect,
                              HLTCOLOR hForeColor, HLTCOLOR hBackColor,
                              SIZE* pSize);
void       fnt_SizeDIBQ488(SIZE* pSize);
void       fnt_BlitTextQ488(CSurfaceQ488* hDest, LTRect* pSrcRect,
                            LTRect* pDestRect, HLTCOLOR hForeColor,
                            HLTCOLOR hBackColor);
CSurfaceQ488* fnt_CreateSurfaceFromStringQ488(CFontQ488* hFont,
                                              HSTRING hString,
                                              HLTCOLOR hForeColor,
                                              HLTCOLOR hBackColor,
                                              int extraPixelsX,
                                              int extraPixelsY);


// ----------------------------------------------------------------------- //
//  0x00488980, 211 bytes -- install the seven font slots and build the
//  memory DC the whole module draws through.
//
//  The seven stores are in the shipped order (0x13c,0x140,0x144,0x148,
//  then 0x18c,0x190, then 0x170 -- NOT ascending, so it is statement order
//  and not a loop).  `push %esi` at 0x00488988 is CreateCompatibleDC's
//  NULL argument hoisted above them; the same zero register then serves
//  SetBkColor and all five trailing clears.
//
//  GetStockObject(4) is BLACK_BRUSH and SetBkMode(...,2) is OPAQUE.
//  SetTextColor is called TWICE with the same colour -- the import is
//  cached in %edi and used at 0x00488a06 and 0x00488a22 -- so the source
//  really does say it twice (repeated calls are repeated calls).
//
//  The trailing clears are in the shipped store order, which puts
//  g_nDIBHeight (0x004b4d18) BEFORE g_nDIBWidth (0x004b4d14).
// ----------------------------------------------------------------------- //

void fnt_InitQ488(CLTClientQ488* pClientDE)
{
	pClientDE->CreateFont              = fnt_CreateFontQ488;
	pClientDE->DeleteFont              = fnt_DeleteFontQ488;
	pClientDE->SetFontExtraSpace       = fnt_SetFontExtraSpaceQ488;
	pClientDE->GetFontExtraSpace       = fnt_GetFontExtraSpaceQ488;
	pClientDE->GetStringDimensions     = fnt_GetStringDimensionsQ488;
	pClientDE->DrawStringToSurface     = fnt_DrawStringToSurfaceQ488;
	pClientDE->CreateSurfaceFromString = fnt_CreateSurfaceFromStringQ488;

	g_hFontDCQ488 = ::CreateCompatibleDC(LTNULL);

	g_hOldBrushQ488 = ::SelectObject(g_hFontDCQ488,
	                                 ::GetStockObject(BLACK_BRUSH));

	::SetTextColor(g_hFontDCQ488, RGB(255, 255, 255));
	::SetBkMode(g_hFontDCQ488, OPAQUE);
	::SetTextColor(g_hFontDCQ488, RGB(255, 255, 255));
	::SetBkColor(g_hFontDCQ488, RGB(0, 0, 0));

	g_hDIBQ488      = LTNULL;
	g_hOldDIBQ488   = LTNULL;
	g_nDIBHeightQ488 = 0;
	g_nDIBWidthQ488  = 0;
	g_pDIBBitsQ488  = LTNULL;
}


// ----------------------------------------------------------------------- //
//  0x00488a60, 170 bytes -- CLTClient::CreateFont, slot 0x13c.
//
//  The fourteen pushes at 0x00488a69..0x00488a9c decode CreateFontA's
//  parameter list exactly: lpszFace = pFontName, nHeight = height,
//  nWidth = width, fdwItalic = bItalic, fdwUnderline = bUnderline,
//  fdwCharSet = 1 (DEFAULT_CHARSET) and everything else zero.  fnWeight is
//  `neg/sbb/and $0x12c/add $0x190` -- "sub/neg/sbb/and is a ?:"
//  -- so it is the EXPRESSION bBold ? 700 : 400, i.e. FW_BOLD : FW_NORMAL.
//
//  The failure arm has NO `xor %eax,%eax`: CreateFontA's zero answer is
//  still live in %eax and VC6 reuses it for the null return.
//
//  The list splice is CGLinkedList::AddHead's, InsertBefore expanded: it
//  reloads pos->m_pGPrev at 0x00488ac7 after storing through it at
//  0x00488ac4, keeps the head (InsertBefore's `pos`) in %ecx across all
//  four stores (0x00488ab5..0x00488ace), and makes InsertBefore's
//  `if (el->m_pGNext == m_pHead)`, provably true here, which VC6 keeps.
// ----------------------------------------------------------------------- //

CFontQ488* fnt_CreateFontQ488(char* pFontName, int width, int height,
                              LTBOOL bItalic, LTBOOL bUnderline, LTBOOL bBold)
{
	CFontQ488* pFont;
	HFONT      hFont;

	hFont = ::CreateFont(height, width, 0, 0,
	                     bBold ? FW_BOLD : FW_NORMAL,
	                     bItalic, bUnderline, 0,
	                     DEFAULT_CHARSET, 0, 0, 0, 0, pFontName);

	if (!hFont)
		return LTNULL;

	pFont = (CFontQ488*)dallocQ488(sizeof(CFontQ488));

	pFont->m_hFont = hFont;

	g_FontList.AddHead(pFont);

	return pFont;
}


// ----------------------------------------------------------------------- //
//  0x00488b10, 16 bytes -- CLTClient::DeleteFont, slot 0x140.  Not in the
//  enumeration; free neighbour.  `pop %ecx` rather than `add $4` because the
//  call is the last statement -- diblit.obj's cis_UnlockSurface emits the
//  same shape and matches, so this is not a varargs tell.
// ----------------------------------------------------------------------- //

void fnt_DeleteFontQ488(CFontQ488* hFont)
{
	if (hFont)
		fnt_FreeFontQ488(hFont);
}


// ----------------------------------------------------------------------- //
//  0x00488b20, 82 bytes -- the real delete.  Not in the enumeration.
//
//  The splice is CGLinkedList::RemoveAt's: `dec %eax; mov %eax,
//  m_nElements; jne` is its `--m_nElements; if (m_nElements == 0)`.
// ----------------------------------------------------------------------- //

void fnt_FreeFontQ488(CFontQ488* pFont)
{
	::DeleteObject(pFont->m_hFont);

	g_FontList.RemoveAt(pFont);

	dfreeQ488(pFont);
}


// ----------------------------------------------------------------------- //
//  0x00488b80, 69 bytes -- CLTClient::SetFontExtraSpace, slot 0x144.  Not
//  in the enumeration.  No g_hFontDC guard: the DC is used unchecked.
// ----------------------------------------------------------------------- //

LTRESULT fnt_SetFontExtraSpaceQ488(CFontQ488* hFont, int pixels)
{
	HGDIOBJ hOld;

	if (!hFont)
		return LT_ERROR;

	hOld = ::SelectObject(g_hFontDCQ488, hFont->m_hFont);

	::SetTextCharacterExtra(g_hFontDCQ488, pixels);

	::SelectObject(g_hFontDCQ488, hOld);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x00488bd0, 71 bytes -- CLTClient::GetFontExtraSpace, slot 0x148.  Not
//  in the enumeration.  The out-parameter is a REFERENCE in iltclient.h and
//  the single `mov %eax,(%ecx)` agrees.
// ----------------------------------------------------------------------- //

LTRESULT fnt_GetFontExtraSpaceQ488(CFontQ488* hFont, int& pixels)
{
	HGDIOBJ hOld;

	if (!hFont)
		return LT_ERROR;

	hOld = ::SelectObject(g_hFontDCQ488, hFont->m_hFont);

	pixels = ::GetTextCharacterExtra(g_hFontDCQ488);

	::SelectObject(g_hFontDCQ488, hOld);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x00488c20, 144 bytes -- CLTClient::GetStringDimensions, slot 0x18c.
//
//  Both guards jump FORWARD to one block that sits past the body's own
//  return, which is an OR-merged failure sunk to the end --
//  hence the single `if (!hFont || !hString)`.  The two epilogues really
//  are separate (the body pops edi/ebx/esi, the tail pops only esi).
//
//  In the tail the stores are *sizeY first, then *sizeX -- two stores
//  through different pointers, which VC6 cannot reorder, so that IS the
//  source order.  The body writes *sizeX first for the same reason.
// ----------------------------------------------------------------------- //

void fnt_GetStringDimensionsQ488(CFontQ488* hFont, HSTRING hString,
                                 int* sizeX, int* sizeY)
{
	uint32 nLen;
	SIZE    size;
	HGDIOBJ hOld;

	if (!hFont || !hString)
	{
		*sizeY = 0;
		*sizeX = 0;
		return;
	}

	hOld = ::SelectObject(g_hFontDCQ488, hFont->m_hFont);

	// HOISTED, and the spelling is already proven in this TU:
	// fnt_DrawStringQ488 does exactly this for TextOut and matches there.
	// Written inline, VC6 evaluates the length after the data pointer and
	// the register roles rotate through the call.
	nLen = str_GetLenQ488(hString);

	::GetTextExtentPoint32(g_hFontDCQ488, str_GetDataQ488(hString, LTNULL),
	                       nLen, &size);

	::SelectObject(g_hFontDCQ488, hOld);

	*sizeX = size.cx;
	*sizeY = size.cy;
}


// ----------------------------------------------------------------------- //
//  0x00488cb0, 151 bytes -- CLTClient::DrawStringToSurface, slot 0x190.
//
//  Four tests, all jumping to the one epilogue at the very end and there is
//  only ONE `ret`: an AND-merged shared exit, so the whole body is
//  wrapped in a single positive guard.  The guard order is the shipped
//  order -- hFont, hString, hDest, g_hFontDC -- not the parameter order.
// ----------------------------------------------------------------------- //

void fnt_DrawStringToSurfaceQ488(CSurfaceQ488* hDest, CFontQ488* hFont,
                                 HSTRING hString, LTRect* pRect,
                                 HLTCOLOR hForeColor, HLTCOLOR hBackColor)
{
	uint32 nLen;
	SIZE    size;
	HGDIOBJ hOld;

	if (hFont && hString && hDest && g_hFontDCQ488)
	{
		hOld = ::SelectObject(g_hFontDCQ488, hFont->m_hFont);

		// HOISTED -- see fnt_GetStringDimensionsQ488 above.
		nLen = str_GetLenQ488(hString);

		::GetTextExtentPoint32(g_hFontDCQ488,
		                       str_GetDataQ488(hString, LTNULL),
		                       nLen, &size);

		::SelectObject(g_hFontDCQ488, hOld);

		fnt_DrawStringQ488(hDest, hFont, hString, pRect,
		                   hForeColor, hBackColor, &size);
	}
}


// ----------------------------------------------------------------------- //
//  The surface lock/unlock pair, as this module writes it.
//
//  diblit.cpp reconstructs the OUT-OF-LINE copies at 0x0043acb0 and
//  0x0043afc0 and its header records the body line by line; these two are
//  the same source EXPANDED, which is why they carry `inline` here.  The
//  expansion is not a guess: 0x00488f90 has the whole of it open-coded --
//  the g_pRender guard at 0x00489015, `*pPitch = 0` at 0x00489028, the
//  screen compare at 0x00489022, and at 0x0048906f..0x0048907f the
//  `if (bWrite && pBits && pSurface)` guard with bWrite folded away,
//  leaving the tautological-looking `cmp %edi,%esi` that is
//  the signature of a guarded inline.
//
//  They are `inline` and nothing else in this file calls them, so no
//  out-of-line COMDAT is emitted and there is nothing here to collide with
//  diblit.cpp's pair.
// ----------------------------------------------------------------------- //

inline void* fnt_LockSurfaceQ488(CSurfaceQ488* pSurface, uint32* pPitch,
                                 LTBOOL bWrite)
{
	void* pBits;

	if (!g_pRenderQ488)
		return LTNULL;

	*pPitch = 0;

	if (pSurface == &g_ScreenSurfaceQ488)
	{
		// Success first: the expansion in 0x00488f90 falls through from the
		// `jne` at 0x00489053 into its own epilogue on failure.  Written
		// `if (!LockScreen(...)) return LTNULL;` the failure jumps to the
		// shared exit instead.
		if (g_pRenderQ488->LockScreen(0, 0, g_ScreenSurfaceQ488.m_nWidth,
		                              g_ScreenSurfaceQ488.m_nHeight, &pBits, pPitch))
		{
			return pBits;
		}

		return LTNULL;
	}

	*pPitch = pSurface->m_nPitch;

	pBits = g_pRenderQ488->LockSurface(pSurface->m_pRenderData);

	if (bWrite && pBits && pSurface)
		pSurface->m_nFlags |= 4;

	return pBits;
}


inline void fnt_UnlockSurfaceQ488(CSurfaceQ488* pSurface)
{
	if (pSurface == &g_ScreenSurfaceQ488)
		g_pRenderQ488->UnlockScreen();
	else
		g_pRenderQ488->UnlockSurface(pSurface->m_pRenderData);
}


// ----------------------------------------------------------------------- //
//  0x00488d50, 336 bytes -- the seven-argument internal draw.  Both
//  0x00488cb0 (slot 0x190) and 0x00489440 (slot 0x170) call it with the
//  measured string size as the seventh argument, so the caller owns the
//  GetTextExtentPoint32 and this owns the TextOut.
//
//  Two LTRects are constructed at the very top -- eight stores of one zero
//  register before the first guard -- which is ltrect.h's defaulted
//  constructor running at the declaration.  rcSrc is constructed first
//  (0x00488d5f writes its four slots before rcDest's).
//
//  str_GetLen is called BEFORE Rectangle and its answer parked in %ebp
//  across it, which no scheduler can do to a call: the length really is a
//  statement of its own.
//
//  The pRect copy stores left, top, BOTTOM, right --
//  a four-argument preload whose last two stores transpose -- so it is
//  LTRect::Init and not four assignments.  The two ZERO arms are not:
//  shipped stores top BEFORE left (0x00488e50/0x00488e54, and again for
//  rcSrc at 0x00488e88), which is the chained `left = top = 0`.  Init(0,0,..)
//  stores left first (near, 336/336); `= LTRect(...)` is 345 bytes.
// ----------------------------------------------------------------------- //

void fnt_DrawStringQ488(CSurfaceQ488* hDest, CFontQ488* hFont,
                        HSTRING hString, LTRect* pRect,
                        HLTCOLOR hForeColor, HLTCOLOR hBackColor,
                        SIZE* pSize)
{
	LTRect  rcSrc, rcDest;
	HGDIOBJ hOld;
	uint32  nLen;

	if (hString && hFont && hDest)
	{
		fnt_SizeDIBQ488(pSize);

		if (g_hDIBQ488 && g_pDIBBitsQ488)
		{
			hOld = ::SelectObject(g_hFontDCQ488, hFont->m_hFont);

			nLen = str_GetLenQ488(hString);

			::Rectangle(g_hFontDCQ488, 0, 0, pSize->cx, pSize->cy);

			::TextOut(g_hFontDCQ488, 0, 0,
			          str_GetDataQ488(hString, LTNULL), nLen);

			::SelectObject(g_hFontDCQ488, hOld);

			if (pRect)
			{
				rcDest.Init(pRect->left, pRect->top,
				            pRect->right, pRect->bottom);
			}
			else
			{
				rcDest.left = rcDest.top = 0;
				rcDest.right  = hDest->m_nWidth;
				rcDest.bottom = hDest->m_nHeight;
			}

			rcSrc.left = rcSrc.top = 0;
			rcSrc.right  = pSize->cx;
			rcSrc.bottom = pSize->cy;

			fnt_BlitTextQ488(hDest, &rcSrc, &rcDest,
			                 hForeColor, hBackColor);
		}
	}
}


// ----------------------------------------------------------------------- //
//  0x00488ea0, 236 bytes -- grow the scratch DIB to hold pSize.
//
//  The guard is `cx > width || cy > height`, read straight off the two
//  compares: `cmp %ecx,%eax / jg` on the width (AT&T, so Intel
//  `cmp cx, g_nDIBWidth`) and `cmp %eax,%ecx / jle` on the height, the
//  second inverted because it falls through to the return.
//
//  `and $0x8000001f / jns / dec / or $0xffffffe0 / inc` is the signed
//  `% 32` idiom, computed ONCE and used twice -- so the source writes the
//  modulo twice and VC6 commons it.
//
//  The frame is exactly 0x28 bytes, which is sizeof(BITMAPINFOHEADER), and
//  the `rep stos` of ten dwords is the memset over it.  `biCompression = 0`
//  survives the memset as a real store (0x00488f28, from the zero register)
//  and is therefore a statement.  biHeight is NEGATED -- a top-down DIB --
//  while g_nDIBHeight keeps the positive value, re-read from pSize at
//  0x00488f73 because CreateDIBSection may have aliased it.
// ----------------------------------------------------------------------- //

void fnt_SizeDIBQ488(SIZE* pSize)
{
	BITMAPINFOHEADER bmi;
	int              nWidth;

	if (pSize->cx > g_nDIBWidthQ488 || pSize->cy > g_nDIBHeightQ488)
	{
		if (g_hOldDIBQ488)
			::SelectObject(g_hFontDCQ488, g_hOldDIBQ488);

		if (g_hDIBQ488)
			::DeleteObject(g_hDIBQ488);

		nWidth = pSize->cx;

		if (nWidth % 32)
			nWidth += 32 - (nWidth % 32);

		memset(&bmi, 0, sizeof(bmi));

		bmi.biSize        = sizeof(BITMAPINFOHEADER);
		bmi.biWidth       = nWidth;
		bmi.biHeight      = -pSize->cy;
		// STATEMENT ORDER, and it is worth two bytes: shipped writes
		// biBitCount BEFORE biPlanes.
		bmi.biBitCount    = 16;
		bmi.biPlanes      = 1;
		bmi.biCompression = BI_RGB;

		g_hDIBQ488 = ::CreateDIBSection(g_hFontDCQ488, (BITMAPINFO*)&bmi,
		                                DIB_RGB_COLORS, &g_pDIBBitsQ488,
		                                LTNULL, 0);

		g_hOldDIBQ488 = ::SelectObject(g_hFontDCQ488, g_hDIBQ488);

		g_nDIBWidthQ488  = nWidth;
		g_nDIBHeightQ488 = pSize->cy;
		g_nDIBPitchQ488  = nWidth * 2;
	}
}


// ----------------------------------------------------------------------- //
//  0x00488f90, 1199 bytes -- blit the rendered text out of the 16bpp DIB
//  onto the destination surface.  The largest row in the cluster and a
//  single function: no branch target leaves it, and its
//  three `ret`s are the screen-lock failure's own epilogue (0x0048905a),
//  the unlocked-screen tail (0x00489425) and the shared exit (0x0048943e).
//
//  THE CLIPPER, 0x0043a2a0.  Twelve arguments, pushed at 0x00488fc8..
//  0x00489004 in this order (last push first): the DIB's width and height,
//  the source rect's four edges, the destination surface's +0x10/+0x14,
//  the destination rect's left and top, and finally the two out-rects --
//  &rcSrc at argument 11 and &rcDest at argument 12, which is what the
//  bodies below prove: 0x004890a1 multiplies rcSrc.top by the DIB pitch and
//  0x00489097 multiplies rcDest.left by the screen's bytes-per-pixel.
//
//  THE COLOURS.  0x0040c120 is a two-instruction accessor and 0x0045f5e0 a
//  `ret $0xc` thiscall on its answer, so the pair is ONE C++ expression:
//  `GetColorOps()->ConvertColor(&g_ScreenFormat, hColor, &nPixel)` with the
//  three arguments pushed before the accessor runs, which is exactly the
//  order C++ evaluates `f()->g(a,b,c)` in.
//
//  THE TRANSPARENCY BIT.  Bit 31 of an HLTCOLOR is the "transparent" flag
//  CreateColor's fourth argument sets.  0x004890f0 computes
//  `hForeColor & 0x80000000` into %eax, tests it, and TESTS THE SAME %eax
//  AGAIN at 0x00489105 -- one CSE'd sub-expression serving two conditions,
//  which is what an `||` guard followed by an if/else-if chain on the same
//  terms gives.  Both-transparent draws nothing but still unlocks, so the
//  guard wraps the chain rather than returning.
//
//  SIX LOOP BODIES.  Two formats (0x004b175c == 2 is 565, == 3 is 32-bit;
//  lttexmod.cpp reads both constants off PFormat::Init's call sites) times
//  three transparency cases.  Each recomputes `bottom - top` and
//  `right - left` for itself, so the extents are local to each arm and not
//  hoisted -- the source repeats them.  `test %eax,%eax ; jbe` rather than
//  `jle` makes the trip counts UNSIGNED.
// ----------------------------------------------------------------------- //

// The six row loops, one per (transparency case, pixel format), as inline
// helpers taking the colours, the destination pitch and the source rect's
// edges BY VALUE (names ours).  What says they were helpers and not six
// loops written out in the body:
//
//   * nDestPitch, nFore and nBack all have their address taken (the lock's
//     pitch out-parameter, the two ConvertColor out-parameters), and
//     written out in the body VC6 treats every store through the row
//     pointer as a possible write to them: it reloads the colour inside the
//     inner loop and the pitch and rcSrc.left on every row (1092 bytes).
//     Shipped does neither: each arm copies the pitch into a fresh frame
//     slot (0x00489131, 0x004891aa, ...), keeps the colour in %ebp and
//     2*left in a slot of its own -- exactly what by-value parameters of an
//     expanded inline look like.
//   * the inner loops open `test ; je` (0x0048914f, 0x004891ca), not the
//     `test ; jbe` of the unsigned outer `y < nHeight`: a count-down loop.
//     The pointer walk is what makes the SOURCE pointer the induction
//     variable in the transparent arms (`cmpw $0,(%eax)`, 0x00489159).
//
// Residual: register assignment inside the arms (the pitch copy's load
// order, `lea` against `add` for 2*left) and the opaque 565 arm's index
// form; 1210 bytes against 1199.
inline void fnt_PaintBack16Q488(uint8* pSrc, uint8* pDest,
                                int left, int top, int right, int bottom,
                                uint32 nDestPitch, uint32 nColor)
{
	uint32 nWidth, nHeight, x, y;

	nHeight = bottom - top;
	nWidth  = right - left;

	for (y = 0; y < nHeight; y++)
	{
		uint16* pSrcRow  = (uint16*)pSrc + left;
		uint16* pDestRow = (uint16*)pDest;

		for (x = nWidth; x; x--)
		{
			if (*pSrcRow == 0)
				*pDestRow = (uint16)nColor;

			pSrcRow++;
			pDestRow++;
		}

		pSrc  += g_nDIBPitchQ488;
		pDest += nDestPitch;
	}
}

inline void fnt_PaintBack32Q488(uint8* pSrc, uint8* pDest,
                                int left, int top, int right, int bottom,
                                uint32 nDestPitch, uint32 nColor)
{
	uint32 nWidth, nHeight, x, y;

	nHeight = bottom - top;
	nWidth  = right - left;

	for (y = 0; y < nHeight; y++)
	{
		uint16* pSrcRow  = (uint16*)pSrc + left;
		uint32* pDestRow = (uint32*)pDest;

		for (x = nWidth; x; x--)
		{
			if (*pSrcRow == 0)
				*pDestRow = nColor;

			pSrcRow++;
			pDestRow++;
		}

		pSrc  += g_nDIBPitchQ488;
		pDest += nDestPitch;
	}
}

inline void fnt_PaintFore16Q488(uint8* pSrc, uint8* pDest,
                                int left, int top, int right, int bottom,
                                uint32 nDestPitch, uint32 nColor)
{
	uint32 nWidth, nHeight, x, y;

	nHeight = bottom - top;
	nWidth  = right - left;

	for (y = 0; y < nHeight; y++)
	{
		uint16* pSrcRow  = (uint16*)pSrc + left;
		uint16* pDestRow = (uint16*)pDest;

		for (x = nWidth; x; x--)
		{
			if (*pSrcRow != 0)
				*pDestRow = (uint16)nColor;

			pSrcRow++;
			pDestRow++;
		}

		pSrc  += g_nDIBPitchQ488;
		pDest += nDestPitch;
	}
}

inline void fnt_PaintFore32Q488(uint8* pSrc, uint8* pDest,
                                int left, int top, int right, int bottom,
                                uint32 nDestPitch, uint32 nColor)
{
	uint32 nWidth, nHeight, x, y;

	nHeight = bottom - top;
	nWidth  = right - left;

	for (y = 0; y < nHeight; y++)
	{
		uint16* pSrcRow  = (uint16*)pSrc + left;
		uint32* pDestRow = (uint32*)pDest;

		for (x = nWidth; x; x--)
		{
			if (*pSrcRow != 0)
				*pDestRow = nColor;

			pSrcRow++;
			pDestRow++;
		}

		pSrc  += g_nDIBPitchQ488;
		pDest += nDestPitch;
	}
}

inline void fnt_PaintBoth16Q488(uint8* pSrc, uint8* pDest,
                                int left, int top, int right, int bottom,
                                uint32 nDestPitch, uint32 nFore, uint32 nBack)
{
	uint32 nWidth, nHeight, x, y;

	nHeight = bottom - top;
	nWidth  = right - left;

	for (y = 0; y < nHeight; y++)
	{
		uint16* pSrcRow  = (uint16*)pSrc + left;
		uint16* pDestRow = (uint16*)pDest;

		for (x = nWidth; x; x--)
			*pDestRow++ = (uint16)(*pSrcRow++ != 0 ? nFore : nBack);

		pSrc  += g_nDIBPitchQ488;
		pDest += nDestPitch;
	}
}

inline void fnt_PaintBoth32Q488(uint8* pSrc, uint8* pDest,
                                int left, int top, int right, int bottom,
                                uint32 nDestPitch, uint32 nFore, uint32 nBack)
{
	uint32 nWidth, nHeight, x, y;

	nHeight = bottom - top;
	nWidth  = right - left;

	for (y = 0; y < nHeight; y++)
	{
		uint16* pSrcRow  = (uint16*)pSrc + left;
		uint32* pDestRow = (uint32*)pDest;

		for (x = nWidth; x; x--)
			*pDestRow++ = *pSrcRow++ != 0 ? nFore : nBack;

		pSrc  += g_nDIBPitchQ488;
		pDest += nDestPitch;
	}
}

void fnt_BlitTextQ488(CSurfaceQ488* hDest, LTRect* pSrcRect,
                      LTRect* pDestRect, HLTCOLOR hForeColor,
                      HLTCOLOR hBackColor)
{
	LTRect rcSrc, rcDest;
	uint8* pDestBits;
	uint8* pSrc;
	uint8* pDest;
	uint32 nDestPitch;
	uint32 nFore, nBack;

	if (!g_pDIBBitsQ488)
		return;

	if (!cis_ClipBlitQ438(g_nDIBWidthQ488, g_nDIBHeightQ488,
	                       pSrcRect->left, pSrcRect->top,
	                       pSrcRect->right, pSrcRect->bottom,
	                       hDest->m_nWidth, hDest->m_nHeight,
	                       pDestRect->left, pDestRect->top,
	                       &rcSrc, &rcDest))
	{
		return;
	}

	pDestBits = (uint8*)fnt_LockSurfaceQ488(hDest, &nDestPitch, LTTRUE);

	if (!pDestBits)
		return;

	pSrc = (uint8*)g_pDIBBitsQ488 + rcSrc.top * g_nDIBPitchQ488;

	pDest = pDestBits + rcDest.left * g_nBytesPerPixel +
	        rcDest.top * nDestPitch;

	GetColorOpsQ488()->ConvertColor(&g_ScreenFormatQ488, hForeColor, &nFore);
	GetColorOpsQ488()->ConvertColor(&g_ScreenFormatQ488, hBackColor, &nBack);

	if (!((hForeColor & 0x80000000) && (hBackColor & 0x80000000)))
	{
		if (hForeColor & 0x80000000)
		{
			// The text is transparent: paint only the background.
			if (g_ScreenFormatQ488.m_nBitDepth == FMT_565_Q488)
				fnt_PaintBack16Q488(pSrc, pDest, rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom, nDestPitch, nBack);
			else if (g_ScreenFormatQ488.m_nBitDepth == FMT_32_Q488)
				fnt_PaintBack32Q488(pSrc, pDest, rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom, nDestPitch, nBack);
		}
		else if (hBackColor & 0x80000000)
		{
			// The background is transparent: paint only the text.
			if (g_ScreenFormatQ488.m_nBitDepth == FMT_565_Q488)
				fnt_PaintFore16Q488(pSrc, pDest, rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom, nDestPitch, nFore);
			else if (g_ScreenFormatQ488.m_nBitDepth == FMT_32_Q488)
				fnt_PaintFore32Q488(pSrc, pDest, rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom, nDestPitch, nFore);
		}
		else
		{
			// Both opaque: every pixel is written.
			if (g_ScreenFormatQ488.m_nBitDepth == FMT_565_Q488)
				fnt_PaintBoth16Q488(pSrc, pDest, rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom, nDestPitch, nFore, nBack);
			else if (g_ScreenFormatQ488.m_nBitDepth == FMT_32_Q488)
				fnt_PaintBoth32Q488(pSrc, pDest, rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom, nDestPitch, nFore, nBack);
		}
	}

	fnt_UnlockSurfaceQ488(hDest);
}


// ----------------------------------------------------------------------- //
//  0x00489440, 204 bytes -- CLTClient::CreateSurfaceFromString, slot 0x170.
//
//  The two entry guards (hFont, g_hFontDC) share the `xor %eax,%eax` return
//  at 0x00489503, and so does the allocation failure at 0x004894dc, and that
//  block sits PAST the success return at 0x00489502.  The rule "test
//  for the case that CONTINUES" says the shared answer is a trailing
//  statement and every guard above it is positive, so the body is wrapped
//  rather than written as three `if (!x) return NULL;` statements.
//
//  The string may be NULL: that arm sets both extents to ONE
//  (`mov $0x1,%ecx ; mov %ecx,%eax` at 0x004894b2) rather than measuring --
//  cy (%ecx) first, so the chained `cx = cy = 1`; two statements load %eax
//  first.  str_GetLen is called BEFORE the &size push (0x00489478 vs
//  0x00489480), so the length is its own statement, as in fnt_DrawString.
//
//  The size slots are re-used: 0x004894c7/0x004894cb store the padded
//  extents back over the SIZE the measurement filled, which is live-range
//  reuse of an eight-byte frame, and 0x004894e6 hands their address to the
//  draw as its seventh argument.  So the padded extents ARE the SIZE, and
//  the draw is given a NULL rect.
// ----------------------------------------------------------------------- //

CSurfaceQ488* fnt_CreateSurfaceFromStringQ488(CFontQ488* hFont,
                                              HSTRING hString,
                                              HLTCOLOR hForeColor,
                                              HLTCOLOR hBackColor,
                                              int extraPixelsX,
                                              int extraPixelsY)
{
	SIZE          size;
	HGDIOBJ       hOld;
	CSurfaceQ488* hSurface;
	uint32        nLen;

	if (hFont && g_hFontDCQ488)
	{
		if (hString)
		{
			hOld = ::SelectObject(g_hFontDCQ488, hFont->m_hFont);

			nLen = str_GetLenQ488(hString);

			::GetTextExtentPoint32(g_hFontDCQ488,
			                       str_GetDataQ488(hString, LTNULL),
			                       nLen, &size);

			::SelectObject(g_hFontDCQ488, hOld);
		}
		else
		{
			size.cx = size.cy = 1;
		}

		size.cx += extraPixelsX;
		size.cy += extraPixelsY;

		hSurface = cis_CreateSurfaceQ488(size.cx, size.cy);

		if (hSurface)
		{
			fnt_DrawStringQ488(hSurface, hFont, hString, LTNULL,
			                   hForeColor, hBackColor, &size);

			return hSurface;
		}
	}

	return LTNULL;
}


// ----------------------------------------------------------------------- //
//  0x00489510, 107 bytes -- the module's GDI teardown; its one caller is
//  cissurf.cpp's cis_Term (0x0040ee22). The fonts go first, before the DC
//  they were selected into. The two SelectObject()s restore the brush
//  first (0x0048952b pushes 0x004b4d10), then the DIB (0x0048953a), under
//  ONE test of the DC (0x0048951a); DeleteObject is under its own test
//  (0x00489551). The stores are old-DIB, DIB, DC (0x0048955c..70); the
//  old brush is NOT cleared.
// ----------------------------------------------------------------------- //

void fnt_FreeAllFontsQ488();

void fnt_TermQ488()
{
	fnt_FreeAllFontsQ488();

	if (g_hFontDCQ488)
	{
		::SelectObject(g_hFontDCQ488, g_hOldBrushQ488);
		::SelectObject(g_hFontDCQ488, g_hOldDIBQ488);
		::DeleteDC(g_hFontDCQ488);
	}

	if (g_hDIBQ488)
		::DeleteObject(g_hDIBQ488);

	g_hOldDIBQ488 = LTNULL;
	g_hDIBQ488    = LTNULL;
	g_hFontDCQ488 = LTNULL;
}


// ----------------------------------------------------------------------- //
//  0x00489580, 47 bytes -- free every font: the list's GetNext walk, the
//  head re-read each pass (0x0048958b, as GetNext reads m_pHead) and its
//  wrap test the 0-or-next select at 0x00489599, before the free.
// ----------------------------------------------------------------------- //

void fnt_FreeAllFontsQ488()
{
	GPOS pos = g_FontList.GetHeadPosition();

	while (pos)
		fnt_FreeFontQ488(g_FontList.GetNext(pos));
}


// ======================================================================= //
//  END OF PART ONE
// ======================================================================= //
