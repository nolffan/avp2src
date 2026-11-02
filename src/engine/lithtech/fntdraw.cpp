// *********************************************************************** //
//
//  MODULE:   W8_488000 (.CPP)   -- scratch reconstruction, lithtech.exe only
//
//  TWO TRANSLATION UNITS LIVE IN THIS FILE.  They are kept in address order
//  and separated by a banner; whoever lands this should SPLIT THEM, because
//  VC6's literal pooling and block merging are per-TU ("A scratch
//  translation unit can invent a residual").
//
//    PART ONE  0x00488980..0x0048950c   the GDI FONT module   (8 enumerated
//              rows + 4 free neighbours)
//    PART TWO  0x0048a770..0x0048c19e   the Smack/DirectDraw video module
//              (6 enumerated rows)
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
//      0x004b4cfc  uint32   g_nFontCount     inc'd on add, dec'd on delete
//      0x004b4d00  CFont*   g_pFontList      circular list HEAD POINTER
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
//  prev->next = next; next->prev = prev.
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

#include <windows.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "ltrect.h"


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

class CFontQ488
{
public:
	CFontQ488* m_pPrev;                            // 0x00
	CFontQ488* m_pNext;                            // 0x04
	HFONT      m_hFont;                            // 0x08
};

uint32     g_nFontCountQ488;                       // 0x004b4cfc
CFontQ488* g_pFontListQ488;                        // 0x004b4d00
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
//  The list splice reloads pHead->m_pPrev at 0x00488ac7 after storing
//  through it at 0x00488ac4, which is what says the source writes
//  `pHead->m_pPrev` twice rather than caching it.  The head itself IS a
//  local: shipped keeps it in %ecx across all four stores (0x00488ab5..
//  0x00488ace), where reading the global each time reloads it after every
//  store (188 bytes, near).  The final
//  `if (pFont->m_pNext == g_pFontList)` is provably always true and VC6
//  keeps it, so it is a statement in the source.
// ----------------------------------------------------------------------- //

CFontQ488* fnt_CreateFontQ488(char* pFontName, int width, int height,
                              LTBOOL bItalic, LTBOOL bUnderline, LTBOOL bBold)
{
	CFontQ488* pFont;
	CFontQ488* pHead;
	HFONT      hFont;

	hFont = ::CreateFont(height, width, 0, 0,
	                     bBold ? FW_BOLD : FW_NORMAL,
	                     bItalic, bUnderline, 0,
	                     DEFAULT_CHARSET, 0, 0, 0, 0, pFontName);

	if (!hFont)
		return LTNULL;

	pFont = (CFontQ488*)dallocQ488(sizeof(CFontQ488));

	pFont->m_hFont = hFont;

	pHead = g_pFontListQ488;

	if (pHead)
	{
		pHead->m_pPrev->m_pNext = pFont;
		pFont->m_pPrev = pHead->m_pPrev;
		pFont->m_pNext = pHead;
		pHead->m_pPrev = pFont;

		g_nFontCountQ488++;

		if (pFont->m_pNext == g_pFontListQ488)
			g_pFontListQ488 = pFont;
	}
	else
	{
		pFont->m_pNext = pFont;
		pFont->m_pPrev = pFont;

		g_pFontListQ488 = pFont;
		g_nFontCountQ488++;
	}

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
//  `dec %eax; mov %eax,g_nFontCount; jne` is a decrement whose RESULT is
//  tested, so the source is `g_nFontCount--;` followed by a test of the
//  decremented value -- written here as the pre-decrement in the condition
//  would read, but as two statements, which is what the separate store
//  before the branch says.
// ----------------------------------------------------------------------- //

void fnt_FreeFontQ488(CFontQ488* pFont)
{
	::DeleteObject(pFont->m_hFont);

	pFont->m_pPrev->m_pNext = pFont->m_pNext;
	pFont->m_pNext->m_pPrev = pFont->m_pPrev;

	if (pFont == g_pFontListQ488)
		g_pFontListQ488 = g_pFontListQ488->m_pNext;

	g_nFontCountQ488--;

	if (g_nFontCountQ488 == 0)
		g_pFontListQ488 = LTNULL;

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


// ======================================================================= //
//  END OF PART ONE
// ======================================================================= //


// ======================================================================= //
//
//  PART TWO -- 0x0048a770 .. 0x0048c19e
//
//  A DIFFERENT TRANSLATION UNIT.  Split this off before landing it.
//
//  WHAT THE SIX REMAINING ENUMERATED ROWS ACTUALLY ARE.  The region was
//  described as "DirectDraw surfaces and video playback", with
//  four literals.  Three of the four are real but belong to rows that are
//  NOT in this enumeration -- "BACKBUFFER" (0x004a73b4),
//  "GetHook(BACKBUFFER) failed" (0x004a7440) and
//  "IDirectDraw::CreateSurface failed" (0x004a736c) are pushed only at
//  0x00401e2c/0x00401f83/0x004022eb (binkvideo.cpp's territory) and at
//  0x0048aeb5/0x0048af57/0x0048b44c/0x0048b47e, which are
//  SmackVideoInst::InitScreen (0x0048ada0) and ::UpdateOnScreen
//  (0x0048b440); "%d videos, %d active" (0x004af4e8) is pushed at
//  0x0048a6f8, inside SmackVideoMgr::UpdateVideo (0x0048a6b0).  None of
//  those five addresses is a row here.  The six rows split three ways:
//
//    0x0048a770  the video-manager FACTORY -- picks Bink or Smack by name
//    0x0048aa50  SmackVideoMgr::StartVideo -- the Smack twin of
//                binkvideo.cpp's BinkVideoMgr::StartVideo (0x00401a50)
//    0x0048b8e0  \
//    0x0048bca0   |  a VISIBILITY TRAVERSAL that has nothing to do with
//    0x0048bd70   |  video at all (see the note at the bottom)
//    0x0048be40  /
//
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  The error report, exactly binkvideo.cpp's K_RETURN_ERROR.  The
//  stringifier double-expands, which is why 0x004a730c holds "67" and not
//  "LT_OUTOFMEMORY", and 0x004a7164 is the lone space.
// ----------------------------------------------------------------------- //

extern "C" void dsi_OnReturnErrorQ488(int code);           // 0x0042d760
extern int      g_DebugLevelQ488;                          // 0x004b0ab0
extern char*    g_pErrorFormatQ488;                        // 0x004a7110
void            dsi_ServerErrorQ488(char* pFormat, ...);   // 0x0042e6f0

#define Q_ERRSTR_(x) #x
#define Q_ERRSTR(x)  Q_ERRSTR_(x)
#define Q_ERRDESC    " "

#define Q_RETURN_ERROR(level, fnName, code) \
	{ \
		dsi_OnReturnErrorQ488(code); \
		if (g_DebugLevelQ488 >= level) \
			dsi_ServerErrorQ488(g_pErrorFormatQ488, fnName, \
			                    Q_ERRSTR(code), Q_ERRDESC); \
		return code; \
	}

// 0x00492f30 -- the CRT's case-insensitive compare.  lt2new3.cpp,
// clientinput.cpp and dmextra.cpp all name this address as stricmp: it is
// <string.h>'s _stricmp, called by its CRT name below.

// binkvideo.cpp's LTLink/LTList, renamed.
struct LTLinkQ488
{
	LTLinkQ488* m_pPrev;                     // 0x00
	LTLinkQ488* m_pNext;                     // 0x04
	void*       m_pData;                     // 0x08
};

struct LTListQ488
{
	// binkvideo.cpp's LTListK::Init, head first then count (0x0048a8ce,
	// 0x0048a8d5).
	void Init() { m_pHead = LTNULL; m_nElements = 0; }

	uint32      m_nElements;                 // 0x00
	LTLinkQ488* m_pHead;                     // 0x04
};

// The same inline helper binkvideo.cpp proves at 0x00401b0b; 0x0048ab11 is
// the identical expansion, reload of `pAfter->m_pNext` included.
inline void q_AddToListQ488(LTListQ488* pList, LTLinkQ488* pLink, void* pObj)
{
	if (pList->m_pHead)
	{
		LTLinkQ488* pAfter = pList->m_pHead->m_pPrev;

		pAfter->m_pNext->m_pPrev = pLink;
		pLink->m_pNext = pAfter->m_pNext;
		pLink->m_pPrev = pAfter;
		pAfter->m_pNext = pLink;
		pLink->m_pData = pObj;
	}
	else
	{
		pLink->m_pPrev = pLink->m_pNext = pLink;
		pLink->m_pData = pObj;
		pList->m_pHead = pLink;
	}

	++pList->m_nElements;
}


// ----------------------------------------------------------------------- //
//  The two video managers and the Smack instance.
//
//  Sizes come from the `push $N; call 0x004290b0` at each allocation site:
//  0x54 for the Bink manager (binkvideo.cpp's own figure), 0x34 for the
//  Smack manager (its constructor 0x0048a870 writes through +0x30 and
//  stops) and 0xa4 for the Smack instance.
//
//  The five virtuals are iltvideomgr.h's, in the order lvideomgr.cpp pins
//  them; only the fact that there ARE virtuals matters here, because
//  0x0048a770 reaches none of them.
// ----------------------------------------------------------------------- //

// NOT pure: the two derived classes are allocated with `new` right here, so
// an abstract base would not compile.  The five bodies live in binkvideo.cpp
// and in the unwritten Smack file, and this object leaves them as undefined
// externals -- which is all the object-level verifier ever sees.
class IVideoMgrQ488
{
public:
	virtual LTRESULT StartOnScreenVideo(char* pFilename, uint32 nFlags,
	                                    void*& hVideo);
	virtual LTRESULT StartTextureVideo(char* pFilename, uint32 nFlags,
	                                   void*& hVideo);
	virtual LTRESULT UpdateVideo(void* hVideo);
	virtual LTRESULT GetVideoStatus(void* hVideo);
	virtual LTRESULT StopVideo(void* hVideo);
};

// binkvideo.cpp's BinkVideoMgrK, cut to what 0x0048a770 needs.  The
// destructor is NOT virtual: 0x0048a7c2 and 0x0048a819 both call
// 0x00401820 directly and then operator delete, which lt2new4.cpp already
// reads as `delete p` on a class whose destructor is non-virtual.
class BinkVideoMgrQ488 : public IVideoMgrQ488
{
public:
	BinkVideoMgrQ488(void* pClientMgr);                     // 0x004017e0
	~BinkVideoMgrQ488();                                    // 0x00401820

	LTRESULT Init();                                        // 0x00401850

public:
	uint8 m_pad_04[0x54 - 0x04];
};

class SmackVideoInstQ488;

// binkvideo.cpp's LTListInitK: the list as a member with an inline
// constructor, count then head -- the two stores 0x0048a870 makes ahead of
// its vptr store.
struct LTListInitQ488 : public LTListQ488
{
	LTListInitQ488() { m_nElements = 0; m_pHead = LTNULL; }
};

class SmackVideoMgrQ488 : public IVideoMgrQ488
{
public:
	SmackVideoMgrQ488(void* pClientMgr);                    // 0x0048a870
	~SmackVideoMgrQ488();                                   // 0x0048a8b0

	LTRESULT Init();                                        // 0x0048a8e0
	LTRESULT StartVideo(char* pFilename, uint32 nFlags,
	                    void*& hVideo, uint32 bTexture);    // 0x0048aa50

public:
	LTListInitQ488 m_Videos;                                // 0x04 / 0x08 (0x0048a870)
	void*      m_pClientMgr;                                // 0x0c
	uint32     m_Unknown10;                                 // 0x10
	// The seven smackw32.dll entry points Init (0x0048a8e0) looks up, in
	// lookup order.  All __stdcall (the "@N" decoration); only the sound
	// one is ever called here, so the rest stay untyped.
	void       (__stdcall *m_pSmackSoundUseMSS)(void* hDig); // 0x14
	void*      (__stdcall *m_pSmackOpen)(char* pName, uint32 nFlags,
	                                     uint32 nExtraBuf);  // 0x18
	void       (__stdcall *m_pSmackClose)(void* pSmack);     // 0x1c
	void*      m_pSmackWait;                                // 0x20
	void*      m_pSmackToBuffer;                            // 0x24
	void*      m_pSmackDoFrame;                             // 0x28
	void*      m_pSmackNextFrame;                           // 0x2c
	HINSTANCE  m_hSmackDLL;                                 // 0x30, smackw32.dll
};

class SmackVideoInstQ488
{
public:
	SmackVideoInstQ488(SmackVideoMgrQ488* pMgr);            // 0x0048aba0

	virtual ~SmackVideoInstQ488();                          // slot 0
	virtual void Release();                                 // slot 1, 0x0048b390
	virtual void Restore();                                 // slot 2, 0x0048b120
	virtual void FreeSurfaces();                            // slot 3, 0x0048b160

	void     Term();                                        // 0x0048ac70
	LTRESULT Init(char* pFilename, uint32 nFlags, uint32 nMode); // 0x0048ad10
	LTRESULT InitScreen();                                  // 0x0048ada0
	LTRESULT InitTexture();                                 // 0x0048b000

public:
	LTLinkQ488         m_Link;                              // 0x04
	LTListQ488         m_Bindings;                          // 0x10 count, 0x14 head
	void*              m_pSmack;                            // 0x18
	SmackVideoMgrQ488* m_pMgr;                              // 0x1c
	uint8              m_pad_20[0x64 - 0x20];
	void*              m_pTextureData;                      // 0x64
	uint8              m_pad_68[0x9c - 0x68];               // 0x68 SharedTexture
	uint32             m_nFlags;                            // 0x9c
	uint32             m_nMode;                             // 0xa0
};


// ----------------------------------------------------------------------- //
//  0x0048a770, 248 bytes -- make a video manager for a named back end.
//
//  Its one caller, 0x0040fc18, pushes the client manager and then the
//  literal "BINK" (0x004a83a8), so the arguments are (pClientMgr, pName)
//  and the answer is stored into pClientMgr->+0x1610 -- which lvideomgr.cpp
//  already names as m_pClientMgr->m_pVideoMgr.
//
//  THE SURPRISE, and it is in the bytes twice over: the "SMACKER" arm tries
//  a BINK manager FIRST (the same 0x54-byte allocation, the same
//  constructor 0x004017e0, the same Init 0x00401850 and the same
//  destructor 0x00401820 as the "BINK" arm) and only falls through to the
//  0x34-byte Smack manager when that fails.  Bink's runtime plays Smacker
//  files, so this is deliberate, not a copy-paste.
//
//  THE TWO NULL TESTS after each allocation are `new`'s own guard merged
//  with the source's `if (p)`: `new` with a
//  constructor splits the allocation from the constructor.  The Init test
//  is `jne` to the teardown, so LT_OK (zero) is the success value and the
//  success return falls through.
//
//  THREE separate `xor %eax,%eax; ret` epilogues -- 0x0048a7d0 for the BINK
//  arm's post-delete return and 0x0048a863 shared by everything else --
//  which is why the BINK arm's failure `return LTNULL` is written out and
//  the SMACKER arm's two are falls-through.
// ----------------------------------------------------------------------- //

IVideoMgrQ488* CreateVideoMgrQ488(void* pClientMgr, char* pName)
{
	BinkVideoMgrQ488*  pBink;
	SmackVideoMgrQ488* pSmack;

	if (_stricmp(pName, "BINK") == 0)
	{
		pBink = new BinkVideoMgrQ488(pClientMgr);

		if (pBink)
		{
			if (!pBink->Init())
				return pBink;

			delete pBink;

			return LTNULL;
		}
	}
	else if (_stricmp(pName, "SMACKER") == 0)
	{
		pBink = new BinkVideoMgrQ488(pClientMgr);

		if (pBink)
		{
			if (!pBink->Init())
				return pBink;

			delete pBink;
		}

		pSmack = new SmackVideoMgrQ488(pClientMgr);

		if (pSmack)
		{
			if (!pSmack->Init())
				return pSmack;

			delete pSmack;
		}
	}

	return LTNULL;
}


// ----------------------------------------------------------------------- //
//  0x0048a870, 50 bytes -- SmackVideoMgr::SmackVideoMgr.
//
//  HOME: smackvideo.cpp, like the destructor below.  binkvideo.cpp's
//  0x004017e0 with the Smack layout: the list (+0x04/+0x08) and the client
//  manager (+0x0c) BEFORE the vptr 0x004a3890 (the initialiser list), then
//  the seven smackw32 entry points +0x14..+0x2c and the handle +0x30
//  cleared in the body.  +0x10 is left alone, as in Bink.
// ----------------------------------------------------------------------- //

SmackVideoMgrQ488::SmackVideoMgrQ488(void* pClientMgr)
	: m_pClientMgr(pClientMgr)
{
	m_pSmackSoundUseMSS = LTNULL;
	m_pSmackOpen = LTNULL;
	m_pSmackClose = LTNULL;
	m_pSmackWait = LTNULL;
	m_pSmackToBuffer = LTNULL;
	m_pSmackDoFrame = LTNULL;
	m_pSmackNextFrame = LTNULL;
	m_hSmackDLL = LTNULL;
}


// ----------------------------------------------------------------------- //
//  0x0048a8b0, 46 bytes -- SmackVideoMgr::~SmackVideoMgr.
//
//  HOME: smackvideo.cpp (it sits between the constructor 0x0048a870 and
//  Init 0x0048a8e0); written here because this file declares the class
//  and its one caller is the factory above (0x0048a855).  Byte for byte
//  binkvideo.cpp's ~BinkVideoMgr (0x00401820) with the handle at +0x30:
//  own vptr only (non-virtual, no base destructor), the handle cleared
//  inside the `if`, then the list reset as the inline Init().
// ----------------------------------------------------------------------- //

SmackVideoMgrQ488::~SmackVideoMgrQ488()
{
	if (m_hSmackDLL)
	{
		FreeLibrary(m_hSmackDLL);
		m_hSmackDLL = LTNULL;
	}

	m_Videos.Init();
}


// ----------------------------------------------------------------------- //
//  0x0048a8e0 .. 0x0048aa4e, 367 bytes -- SmackVideoMgr::Init.
//
//  The csv carried it as 0x0048a8e0 (87) + a "gap" 0x0048a937 (280); the
//  second row is the `push %edi` opening the success arm of the `jne` at
//  0x0048a8f3 -- the same enumeration defect binkvideo.cpp documents for
//  BinkVideoMgr::Init (0x00401850 / 0x004018a7), and this body is that
//  one's twin: LoadLibrary("smackw32.dll") stored to +0x30, seven
//  GetProcAddress lookups through the CSE'd import in %edi, the validity
//  tests in lookup order all `je` one block after both success returns,
//  ci_GetSoundMgr() called twice around the sound-system test.  Unlike
//  Bink, BOTH arms call the one pointer at +0x14 (_SmackSoundUseMSS@4),
//  with the driver or with 0 -- and the source makes ONE call on a local
//  set by an if/else: VC6 tail-duplicates the call and the return into
//  each arm, the else arm as `xor %eax,%eax; push %eax` (0x0048a9ee).
//  Two written-out calls give `push $0` instead.
// ----------------------------------------------------------------------- //

class ILTClientSoundMgrQ488
{
public:
	uint8 m_pad_00[0x124];
	void* m_pSoundSys;                       // 0x124
	uint8 m_pad_128[0x130 - 0x128];
	void* m_hDigDriver;                      // 0x130  (name ours)
};

ILTClientSoundMgrQ488* ci_GetSoundMgrQ488();               // 0x0040f7f0

LTRESULT SmackVideoMgrQ488::Init()
{
	m_hSmackDLL = LoadLibrary("smackw32.dll");
	if (!m_hSmackDLL)
	{
		dsi_ServerErrorQ488("Unable to find smackw32.dll, video playback disabled.");
		Q_RETURN_ERROR(1, "SmackVideoMgr::Init", LT_MISSINGFILE);
	}

	m_pSmackSoundUseMSS = (void (__stdcall *)(void*))GetProcAddress(m_hSmackDLL, "_SmackSoundUseMSS@4");
	m_pSmackOpen        = (void* (__stdcall *)(char*, uint32, uint32))GetProcAddress(m_hSmackDLL, "_SmackOpen@12");
	m_pSmackClose       = (void (__stdcall *)(void*))GetProcAddress(m_hSmackDLL, "_SmackClose@4");
	m_pSmackWait        = (void*)GetProcAddress(m_hSmackDLL, "_SmackWait@4");
	m_pSmackToBuffer    = (void*)GetProcAddress(m_hSmackDLL, "_SmackToBuffer@28");
	m_pSmackDoFrame     = (void*)GetProcAddress(m_hSmackDLL, "_SmackDoFrame@4");
	m_pSmackNextFrame   = (void*)GetProcAddress(m_hSmackDLL, "_SmackNextFrame@4");

	if (m_pSmackSoundUseMSS && m_pSmackOpen && m_pSmackClose && m_pSmackWait &&
	    m_pSmackToBuffer && m_pSmackDoFrame && m_pSmackNextFrame)
	{
		void* hDig;

		if (ci_GetSoundMgrQ488()->m_pSoundSys)
			hDig = ci_GetSoundMgrQ488()->m_hDigDriver;
		else
			hDig = LTNULL;

		m_pSmackSoundUseMSS(hDig);

		return LT_OK;
	}

	dsi_ServerErrorQ488("Invalid smackw32.dll, video playback disabled.");

	FreeLibrary(m_hSmackDLL);
	m_hSmackDLL = LTNULL;

	Q_RETURN_ERROR(1, "SmackVideoMgr::Init", LT_NOTINITIALIZED);
}


// ----------------------------------------------------------------------- //
//  0x0048ac70, 152 bytes -- SmackVideoInst::Term.
//
//  HOME: smackvideo.cpp, like the rest of the instance; written here
//  because this file declares the class.  The Bink twin is binkvideo.cpp's
//  BinkVideoInst::Term (0x00401c70) and the two list walks are its bytes
//  exactly (branchless-select advance, `delete pBind` through 0x0048a620).
//  The Smack order differs: FreeSurfaces (slot 3) comes FIRST, then a
//  single _SmackClose@4 (manager +0x1c) with no wait loop, then the
//  texture data at +0x64.  lt2new3.cpp's CClass4A38A4 and cmgsurf.cpp's
//  SmackVideoInstQ428 name the same function; neither defines it.
// ----------------------------------------------------------------------- //

void tex_FreeTextureDataQ488(void* pData);                 // 0x0042ee30

struct SharedTextureQ488;

struct CVideoPolyQ488
{
	uint8               m_pad_00[0x30];
	SharedTextureQ488*  m_pTexture;          // 0x30
};

// binkvideo.cpp's CVideoBindingK, cut to the two fields read here.
class CVideoBindingQ488
{
public:
	~CVideoBindingQ488();                    // 0x0048a620

public:
	CVideoPolyQ488*     m_pPoly;             // 0x00
	uint8               m_pad_04[0x10 - 0x04];
	SharedTextureQ488*  m_pOldTexture;       // 0x10
	uint8               m_pad_14[0x2c - 0x14];
};

void SmackVideoInstQ488::Term()
{
	LTLinkQ488*        pLink;
	CVideoBindingQ488* pBind;

	FreeSurfaces();

	if (m_pSmack)
	{
		m_pMgr->m_pSmackClose(m_pSmack);
		m_pSmack = LTNULL;
	}

	if (m_pTextureData)
	{
		tex_FreeTextureDataQ488(m_pTextureData);
		m_pTextureData = LTNULL;
	}

	pLink = m_Bindings.m_pHead;
	while (pLink)
	{
		pBind = (CVideoBindingQ488*)pLink->m_pData;
		pLink = (pLink->m_pNext == m_Bindings.m_pHead) ? LTNULL : pLink->m_pNext;

		if (pBind->m_pPoly)
			pBind->m_pPoly->m_pTexture = pBind->m_pOldTexture;
	}

	pLink = m_Bindings.m_pHead;
	while (pLink)
	{
		pBind = (CVideoBindingQ488*)pLink->m_pData;
		pLink = (pLink->m_pNext == m_Bindings.m_pHead) ? LTNULL : pLink->m_pNext;

		delete pBind;
	}

	m_Bindings.Init();
}


// ----------------------------------------------------------------------- //
//  0x0048ad10, 140 bytes -- SmackVideoInst::Init.
//
//  Twin of binkvideo.cpp's BinkVideoInst::Init (0x00401d30): `ret $0xc`,
//  (pFilename, nFlags, nMode), the RETURN_ERROR_INFO report with the code
//  spelled symbolically (0x004a7324 "LT_MISSINGFILE") and "SmackOpen
//  failed", and the two trailing stores +0xa0 then +0x9c.  The open flags
//  are `neg; sbb; and $0xfff02000; add $0xfe000` -- a ternary over two
//  constants, 0xfe000 (RAD's SMACKTRACKS) for an on-screen video and 0 for
//  a texture one -- then 0x100000 or'd in for flag 2.  _SmackOpen@12's
//  third argument is -1.
// ----------------------------------------------------------------------- //

#define Q_RETURN_ERROR_INFO(level, fnName, code, info) \
	{ \
		dsi_OnReturnErrorQ488(code); \
		if (g_DebugLevelQ488 >= level) \
			dsi_ServerErrorQ488(g_pErrorFormatQ488, fnName, #code, info); \
		return code; \
	}

LTRESULT SmackVideoInstQ488::Init(char* pFilename, uint32 nFlags, uint32 nMode)
{
	uint32 nOpenFlags = nMode ? 0 : 0xfe000;

	if (nFlags & 2)
		nOpenFlags |= 0x100000;

	m_pSmack = m_pMgr->m_pSmackOpen(pFilename, nOpenFlags, 0xFFFFFFFF);
	if (!m_pSmack)
		Q_RETURN_ERROR_INFO(1, "SmackVideoInst::Init", LT_MISSINGFILE,
		                    "SmackOpen failed");

	m_nMode = nMode;
	m_nFlags = nFlags;

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0048aa50, 259 bytes -- SmackVideoMgr::StartVideo.
//
//  Line for line the twin of binkvideo.cpp's BinkVideoMgr::StartVideo
//  (0x00401a50), and everything that file pins applies here unchanged:
//  `ret $0x10` with `this` in %ecx, `hVideo = LTNULL` as the first
//  statement, the LT_OUTOFMEMORY report on the allocation failure, slot 1
//  (Release) rather than slot 0 for both cleanups, `test %ebp,%ebp; jne` so
//  InitScreen is the FALL-THROUGH and the source tests `!bTexture`, the
//  join assigning both arms into one local, and the list insert reloading
//  `pAfter->m_pNext` at 0x0048ab22 after storing through it at 0x0048ab20.
//
//  The only differences from the Bink body are the instance size (0xa4
//  against 0x68), the constructor (0x0048aba0), the three entry points
//  (0x0048ad10 / 0x0048ada0 / 0x0048b000) and the error tag, which
//  0x004af610 spells "SmackVideoMgr".
// ----------------------------------------------------------------------- //

LTRESULT SmackVideoMgrQ488::StartVideo(char* pFilename, uint32 nFlags,
                                       void*& hVideo, uint32 bTexture)
{
	SmackVideoInstQ488* pInst;
	LTRESULT            dResult;

	hVideo = LTNULL;

	pInst = new SmackVideoInstQ488(this);
	if (!pInst)
		Q_RETURN_ERROR(1, "SmackVideoMgr", LT_OUTOFMEMORY);

	dResult = pInst->Init(pFilename, nFlags, bTexture);
	if (dResult != LT_OK)
	{
		pInst->Release();
		return dResult;
	}

	if (!bTexture)
		dResult = pInst->InitScreen();
	else
		dResult = pInst->InitTexture();

	if (dResult != LT_OK)
	{
		pInst->Release();
		return dResult;
	}

	q_AddToListQ488(&m_Videos, &pInst->m_Link, pInst);

	hVideo = pInst;

	return LT_OK;
}


// ======================================================================= //
//  THE VISIBILITY TRAVERSAL -- 0x0048b8e0, 0x0048bca0, 0x0048bd70,
//  0x0048be40.  NOT video code.
//
//  0x0048b8e0 opens `mov $0x4e30,%eax ; call 0x00492c60` (__chkstk) and
//  then publishes the TOP of that 20,016-byte frame as a global:
//
//      lea 0x4e34(%esp),%eax ; mov %eax,0x004b4d70
//
//  so 0x004b4d70 is the scratch buffer's limit and everything below
//  bounds-checks against it.  Its argument is stored to 0x004b4d74 and is a
//  query object with a world at +0x04, three callbacks at +0x0c, +0x10 and
//  +0x14 (the +0x10 one DEFAULTED to 0x0048bc50 when the caller left it
//  null), and user data at +0x18.  0x004b4d78/0x004b4d7c are a bit index
//  and a bit array -- `mov $1,%al ; and $7,%cl ; shl %cl,%al ;
//  and (%ecx,%edi,1),%al ; inc <index>` is a sequential PVS bit read, which
//  is what makes this the client's potentially-visible-set walk.
//
//  Its two callers are 0x00426b55 and 0x0045afaf, neither in this region.
//
//  0x0048be40 is the per-node recursion (it calls itself at 0x0048c077 and
//  is called from 0x0048bb0c), and the two filters below are its object-list
//  splitters.  Both are written below (see vis_TraversalQ488 and
//  vis_WalkNodeQ488).
// ======================================================================= //

#include "ltplane.h"

// The walked object.  +0x18c and +0x1a4 are src/engine/server's LTObject
// m_Pos and m_Radius, which ltobject_internal.h and clientmgr.h both carry;
// +0x58 is ltobject_internal.h's m_Unknown58.
class CVisObjectQ488
{
public:
	uint8    m_pad_00[0x58];
	uint32   m_Unknown58;                    // 0x058
	uint8    m_pad_5c[0x18c - 0x5c];
	LTVector m_Pos;                          // 0x18c
	uint8    m_pad_198[0x1a4 - 0x198];
	float    m_Radius;                       // 0x1a4
};

// ---- the traversal's world, node and tree (0x0048b8e0 / 0x0048be40) ---- //
// Layouts as src/engine/server/worldtree.cpp's sections C and D read them
// off server.dll's twins 0x10038e77 / 0x10038ff1; every offset below is also
// a load in 0x0048b8e0 or 0x0048be40.

// An object-list head: the 8-byte half of a link (the SDK's CheapLTLink);
// the links on it are LTLinkQ488s, and vis_CollectListQ488 takes the head
// as one.
struct VisListHeadQ488
{
	LTLinkQ488* m_pPrev;                     // 0x00
	LTLinkQ488* m_pNext;                     // 0x04
};

// A leaf record, 0x30 bytes (`lea (%ecx,%ecx,2); shl $4`).
struct VisRecordQ488
{
	uint8 m_pad[0x30];
};

// A chunk, 0x24 bytes (`lea (%eax,%eax,8); lea (%ecx,%edx,4)`).
struct VisChunkQ488
{
	uint8 m_pad_00[0x06];
	uint8 m_Flags;                           // 0x06  bit 0
	uint8 m_pad_07[0x24 - 0x07];
};

struct VisSurfaceQ488
{
	uint8    m_pad_00[0x34];
	LTPlane* m_pPlane;                       // 0x34
};

struct VisPolyQ488
{
	uint8           m_pad_00[0x28];
	VisSurfaceQ488* m_pSurface;              // 0x28
};

struct VisNodeQ488
{
	VisPolyQ488* m_pPoly;                    // 0x00
	VisNodeQ488* m_Sides[2];                 // 0x04
	VisListHeadQ488 m_Objects;               // 0x0c  the node's object list
	uint16       m_nLeaf;                    // 0x14  0xffff: none
	uint8        m_Flags;                    // 0x16  bit 3: descend

	LTPlane* GetPlane() { return m_pPoly->m_pSurface->m_pPlane; }
};

class CVisRecordArrayQ488
{
public:
	uint32         m_Unknown00;              // 0x00
	VisRecordQ488* m_pArray;                 // 0x04
	uint32         m_nElements;              // 0x08

	inline VisRecordQ488* GetRecord(uint32 i)
	{
		return i < m_nElements ? &m_pArray[i] : LTNULL;
	}
};

// A world leaf, 0x18 bytes, at world +0x70/+0x74 (0x0048bbaf, `add $0x18`).
struct VisLeafEntryQ488
{
	uint8           m_pad_00[0x0c];
	VisListHeadQ488 m_Objects;               // 0x0c
	uint16     m_iRecord;                    // 0x14  0xffff = none
	uint8      m_pad_16[0x18 - 0x16];
};

class CVisWorldQ488
{
public:
	uint8               m_pad_00[0x70];
	VisLeafEntryQ488*   m_pLeaves;           // 0x70
	uint32              m_nLeaves;           // 0x74
	uint8               m_pad_78[0x84 - 0x78];
	CVisRecordArrayQ488 m_Records;           // 0x84
	uint8               m_pad_90[0x9c - 0x90];
	VisNodeQ488*        m_pRootNode;         // 0x9c
	uint8               m_pad_a0[0xb0 - 0xa0];
	VisChunkQ488*       m_pChunks;           // 0xb0

	VisChunkQ488* GetChunk(uint32 i) { return &m_pChunks[i]; }

	// The walk's lookup (0x0048be9c..0x0048bec3): straight off the world,
	// the array base loaded into its own register before the add.
	VisRecordQ488* GetRecord(uint32 i)
	{
		if (i < m_Records.m_nElements)
			return &m_Records.m_pArray[i];
		return LTNULL;
	}
};

struct VisTreeEntryQ488
{
	uint16 m_iChunk;                         // 0x00
	uint16 m_pad_02;
	uint8* m_pBits;                          // 0x04  the entry's PVS bits
};

struct VisTreeQ488
{
	uint8             m_pad_00[0x10];
	VisTreeEntryQ488* m_pEntries;            // 0x10
	uint8             m_pad_14[0x2e - 0x14];
	uint16            m_nEntries;            // 0x2e

	VisTreeEntryQ488* GetEntry(uint32 i) { return &m_pEntries[i]; }
};

typedef void (*VisObjFnQ488)(CVisObjectQ488* pObj, void* pUser);
typedef void (*VisRecordFnQ488)(VisRecordQ488* pRecord, void* pUser);
typedef void (*VisCollectFnQ488)(LTLinkQ488* pHead, CVisObjectQ488*** ppArray,
                                 uint32* pnCount);
typedef int  (*VisFilterFnQ488)(VisChunkQ488* pChunk);

class CVisQueryQ488
{
public:
	VisTreeQ488*     m_pTree;                // 0x00
	CVisWorldQ488*   m_pWorld;               // 0x04
	VisObjFnQ488     m_ObjFn;                // 0x08
	VisRecordFnQ488  m_RecordFn;             // 0x0c
	VisCollectFnQ488 m_CollectFn;            // 0x10
	VisFilterFnQ488  m_FilterFn;             // 0x14
	void*            m_pUser;                // 0x18
	uint32           m_Unknown1c;            // 0x01c  the visit mark

	// STAND-IN (name and existence ours): the leaves path's record report
	// written as an inline helper.  It is here for its INLINE COST: without
	// it 0x0048b8e0's /O2 budget still covers vis_FilterNegQ488 (1053 bytes,
	// the filter expanded and the third bit read called); with it both
	// filters are called and every bit read expanded, as shipped (880).
	// Which inline the original spent that budget on is unknown.
	void ReportRecord(VisRecordQ488* pRecord)
	{
		if (pRecord)
			m_RecordFn(pRecord, m_pUser);
	}
};

// The traversal's state, defined here: nothing else in the image touches
// these addresses (all .bss, so no initialisers).
CVisObjectQ488** g_pVisLimitQ488;     // 0x004b4d70
CVisQueryQ488*   g_pVisQueryQ488;     // 0x004b4d74
uint32           g_nVisBitQ488;       // 0x004b4d78
uint8*           g_pVisBitsQ488;      // 0x004b4d7c
VisRecordQ488*   g_pVisRecordQ488;    // 0x004b5220
CVisObjectQ488** g_pVisCursorQ488;    // 0x004b5224
uint32           g_nVisFilteredQ488;  // 0x004b5228
CVisObjectQ488** g_pVisFilteredQ488;  // 0x004b522c

#define VIS_BUFFER_SIZE_Q488  5000           // 0x4e30 - 0x10 = 5000 * 4

void vis_CollectListQ488(LTLinkQ488* pHead, CVisObjectQ488*** ppArray,
                         uint32* pnCount);
CVisObjectQ488** vis_FilterNegQ488(LTPlane* pPlane, CVisObjectQ488** pIn,
                                   uint32 nIn, uint32* pnOut, LTBOOL bAppend);
CVisObjectQ488** vis_FilterPosQ488(LTPlane* pPlane, CVisObjectQ488** pIn,
                                   uint32 nIn, uint32* pnOut);
void vis_WalkNodeQ488(VisNodeQ488* pNode, CVisObjectQ488** pArray,
                      uint32 nCount);

// The next bit of the PVS stream -- server.dll's 0x10039151, expanded at
// every use here (0x0048b9bc, 0x0048baaa, 0x0048bb18, 0x0048be70, ...).
inline uint8 vis_ReadBitQ488()
{
	uint8 bBit = (uint8)((1 << (g_nVisBitQ488 & 7)) & g_pVisBitsQ488[g_nVisBitQ488 >> 3]);
	g_nVisBitQ488++;
	return bBit;
}


// ----------------------------------------------------------------------- //
//  0x0048b8e0, 880 bytes -- the entry point (server.dll 0x10038e77,
//  src/engine/server/worldtree.cpp's wt_VisibilityTraversal).  The one
//  difference from the server is that the per-node walk 0x0048be40 is
//  EXPANDED here for the root call (0x0048b996..0x0048bb7c: the walk's
//  loop, with its two plane filters called out of line at 0x0048baef /
//  0x0048bb58 and its recursion calling 0x0048be40 at 0x0048bb0c).
//  Callers 0x00426b55 and 0x0045afaf (netobj.cpp's
//  wt_VisibilityTraversalQ458 is this function).
// ----------------------------------------------------------------------- //
void vis_TraversalQ488(CVisQueryQ488* pQuery)
{
	CVisObjectQ488*  buffer[VIS_BUFFER_SIZE_Q488];
	CVisObjectQ488** pObjects;
	uint32           i, j, nObjects;

	g_pVisLimitQ488 = &buffer[VIS_BUFFER_SIZE_Q488];

	if (!pQuery->m_CollectFn)
		pQuery->m_CollectFn = vis_CollectListQ488;

	g_pVisQueryQ488 = pQuery;

	if (pQuery->m_pTree && pQuery->m_pTree->m_nEntries > 0)
	{
		for (i = 0; i < pQuery->m_pTree->m_nEntries; i++)
		{
			VisTreeEntryQ488* pEntry = pQuery->m_pTree->GetEntry(i);

			if (i != 0)
			{
				VisChunkQ488* pChunk = pQuery->m_pWorld->GetChunk(pEntry->m_iChunk);

				if (!(pChunk->m_Flags & 1))
					continue;

				if (!pQuery->m_FilterFn(pChunk))
					continue;
			}

			g_pVisBitsQ488 = pEntry->m_pBits;
			g_nVisBitQ488 = 0;
			vis_WalkNodeQ488(pQuery->m_pWorld->m_pRootNode, buffer, 0);
		}
	}
	else
	{
		for (i = 0; i < pQuery->m_pWorld->m_nLeaves; i++)
		{
			VisLeafEntryQ488* pLeaf = &pQuery->m_pWorld->m_pLeaves[i];

			pObjects = buffer;
			nObjects = 0;

			if (pLeaf->m_iRecord != 0xffff)
			{
				pQuery->ReportRecord(pQuery->m_pWorld->GetRecord(pLeaf->m_iRecord));
			}

			pQuery->m_CollectFn((LTLinkQ488*)&pLeaf->m_Objects, &pObjects, &nObjects);

			for (j = 0; (int)j < (int)nObjects; j++)
				pQuery->m_ObjFn(pObjects[j], pQuery->m_pUser);
		}
	}
}


// ----------------------------------------------------------------------- //
//  0x0048bc50, 76 bytes -- the default +0x10 callback.  Not in the enumeration;
//  free neighbour, and the thing 0x0048b8e0 installs at 0x0048b908 when the
//  caller left the slot null, so its three arguments are exactly the three
//  0x0048bc0a pushes: a list head, the ADDRESS of the output cursor, and the
//  address of the count.
//
//  The array base is re-read from *ppArray on every iteration (0x0048bc66)
//  and the limit and query globals with it, which is what says the source
//  writes the double dereference rather than caching it.
//
//  The walk follows +0x00, the PREV link, and stops on the head -- the same
//  circular list binkvideo.cpp's LTLink describes.
// ----------------------------------------------------------------------- //

void vis_CollectListQ488(LTLinkQ488* pHead, CVisObjectQ488*** ppArray,
                         uint32* pnCount)
{
	LTLinkQ488* pCur;

	pCur = pHead->m_pPrev;

	while (pCur != pHead)
	{
		CVisObjectQ488* pObj = (CVisObjectQ488*)pCur->m_pData;

		if (*ppArray < g_pVisLimitQ488 &&
		    pObj->m_Unknown58 != g_pVisQueryQ488->m_Unknown1c)
		{
			(*ppArray)[*pnCount] = pObj;
			(*pnCount)++;
		}

		pCur = pCur->m_pPrev;
	}
}


// ----------------------------------------------------------------------- //
//  0x0048bca0, 203 bytes -- keep the objects on ONE side of a plane.
//
//  Five arguments; its only call site, 0x0048baef, pushes them as
//  (plane, array, count, &g_Count, flag) with `add $0x14`.  The fifth
//  decides whether the survivors are written OVER the input or just PAST it
//  (`mov %edi,%esi ; je ; lea (%edi,%eax,4),%esi`).
//
//  `cmp %ebx,%ecx ; jae` on `pOut + nIn` against 0x004b4d70 is an UNSIGNED
//  pointer compare -- the scratch-buffer overflow guard -- and it returns
//  pOut with the count left at zero.
//
//  The distance is LTPlane::DistTo taken BY VALUE: 0x0048bcfb copies the
//  object's three position floats into consecutive frame slots and only
//  then loads them, which is the by-value LTVector parameter
//  shape.  The multiply order is z, y, x -- a flat three-term sum
//  emits its terms backwards -- so the source is m_Normal.Dot(vec), i.e.
//  ltplane.h's DistTo verbatim.
//
//  `fcompp` compares the SECOND-loaded value with the first, so
//  `test $0x41,%ah ; jne <skip>` skips when (radius + 0.1f) <= dist: the
//  kept case is `dist < radius + 0.1f`.  0x004a1e24 is 0x3dcccccd, 0.1f.
//
//  `mov (%edx),%eax` appears TWICE around the store (0x0048bd49 and
//  0x0048bd4e) because the store through pOut may alias the counter, which
//  is what says *pnOut is written directly rather than through a local.
//
//  The loop is `while (nIn--)` over a walking pIn: `mov %eax,%ecx ; dec
//  %eax ; test %ecx,%ecx ; je` at 0x0048bcd6 is the post-decrement test,
//  the count lives in a stack slot decremented at the bottom, and pIn is
//  bumped by `add $4` AFTER the body -- so `pIn++` is the loop's last
//  statement, not `*pIn++` in the load (that puts the add at the top).
// ----------------------------------------------------------------------- //

inline CVisObjectQ488** vis_FilterNegQ488(LTPlane* pPlane, CVisObjectQ488** pIn,
                                   uint32 nIn, uint32* pnOut, LTBOOL bAppend)
{
	CVisObjectQ488** pOut;

	pOut = pIn;

	if (bAppend)
		pOut = pIn + nIn;

	*pnOut = 0;

	if (pOut + nIn >= g_pVisLimitQ488)
		return pOut;

	while (nIn--)
	{
		CVisObjectQ488* pObj = *pIn;

		if (pObj->m_Unknown58 != g_pVisQueryQ488->m_Unknown1c)
		{
			if (pPlane->DistTo(pObj->m_Pos) < pObj->m_Radius + 0.1f)
			{
				pOut[*pnOut] = pObj;
				(*pnOut)++;
			}
		}

		pIn++;
	}

	return pOut;
}


// ----------------------------------------------------------------------- //
//  0x0048bd70, 199 bytes -- the twin, for the other side of the plane.
//
//  FOUR arguments (0x0048bb58 pushes four and cleans 0x10); the survivors
//  always go over the input, so there is no selector and no `lea`.  The
//  overflow guard's early return has its OWN epilogue here (0x0048bd97)
//  because %ebp is not yet pushed, which is why it is written as a guarded
//  return rather than a jump to the end.
//
//  The only other difference is `fchs` at 0x0048be0a and `test $0x1,%ah ;
//  je` in place of `test $0x41,%ah ; jne` -- the negated bound and the
//  strictly-greater sense, i.e. `dist > -(radius + 0.1f)`.  `fchs` AFTER
//  the add says the source negates the whole sum, not the radius.  Same
//  `while (nIn--)` loop with `pIn++` last (0x0048bda0, 0x0048be23).
// ----------------------------------------------------------------------- //

inline CVisObjectQ488** vis_FilterPosQ488(LTPlane* pPlane, CVisObjectQ488** pIn,
                                   uint32 nIn, uint32* pnOut)
{
	CVisObjectQ488** pOut;

	pOut = pIn;

	*pnOut = 0;

	if (pOut + nIn >= g_pVisLimitQ488)
		return pOut;

	while (nIn--)
	{
		CVisObjectQ488* pObj = *pIn;

		if (pObj->m_Unknown58 != g_pVisQueryQ488->m_Unknown1c)
		{
			if (pPlane->DistTo(pObj->m_Pos) > -(pObj->m_Radius + 0.1f))
			{
				pOut[*pnOut] = pObj;
				(*pnOut)++;
			}
		}

		pIn++;
	}

	return pOut;
}


// ----------------------------------------------------------------------- //
//  0x0048be40, 862 bytes -- one node of the walk (server.dll 0x10038ff1,
//  worldtree.cpp's vis_WalkNode_D).  Recursive (0x0048c077) and tail-
//  looping (0x0048c179 / 0x0048c191 rewrite the argument slots and `jmp
//  0x0048be47`).  Both plane filters are EXPANDED here (0x0048bfde..,
//  0x0048c0f2.., `fchs` included), and so is the bit read.
// ----------------------------------------------------------------------- //
inline void vis_WalkNodeQ488(VisNodeQ488* pNode, CVisObjectQ488** pArray,
                             uint32 nCount)
{
	uint32 bBack;

	for (;;)
	{
		g_pVisQueryQ488->m_CollectFn((LTLinkQ488*)&pNode->m_Objects, &pArray, &nCount);

		if (pNode->m_nLeaf != 0xffff && vis_ReadBitQ488())
		{
			g_pVisRecordQ488 = g_pVisQueryQ488->m_pWorld->GetRecord(pNode->m_nLeaf);
			if (g_pVisRecordQ488)
				g_pVisQueryQ488->m_RecordFn(g_pVisRecordQ488, g_pVisQueryQ488->m_pUser);

			g_pVisCursorQ488 = pArray;
			while (nCount--)
			{
				if ((*g_pVisCursorQ488)->m_Unknown58 != g_pVisQueryQ488->m_Unknown1c)
				{
					(*g_pVisCursorQ488)->m_Unknown58 = g_pVisQueryQ488->m_Unknown1c;
					g_pVisQueryQ488->m_ObjFn(*g_pVisCursorQ488, g_pVisQueryQ488->m_pUser);
				}

				g_pVisCursorQ488++;
			}

			nCount = 0;
		}

		bBack = pNode->m_Sides[1]->m_Flags & 8;

		if ((pNode->m_Sides[0]->m_Flags & 8) && vis_ReadBitQ488())
		{
			g_pVisFilteredQ488 = vis_FilterNegQ488(pNode->GetPlane(), pArray, nCount,
			                                       &g_nVisFilteredQ488, bBack);
			if (bBack)
			{
				vis_WalkNodeQ488(pNode->m_Sides[0], g_pVisFilteredQ488, g_nVisFilteredQ488);
			}
			else
			{
				pArray = g_pVisFilteredQ488;
				nCount = g_nVisFilteredQ488;
				pNode  = pNode->m_Sides[0];
				continue;
			}
		}

		if (bBack && vis_ReadBitQ488())
		{
			pArray = vis_FilterPosQ488(pNode->GetPlane(), pArray, nCount, &nCount);
			pNode  = pNode->m_Sides[1];
			continue;
		}

		return;
	}
}


// ======================================================================= //
//  END OF PART TWO
// ======================================================================= //


// ======================================================================= //
//  0x0048b8e0 (880) and 0x0048be40 (862) -- NOW WRITTEN above
//  (vis_TraversalQ488 / vis_WalkNodeQ488), both size-exact nears whose only
//  residual is the record lookup's registers (shipped scales the index in
//  place and adds the base, ours the reverse; ternary, if/else, result local,
//  byte arithmetic, uint16/int index and a CMoArray at +0x84 all measured).
//  The notes below are the evidence as first gathered.
//
//  BOTH ARE SINGLE FUNCTIONS.  Both extents are clean: no branch
//  target leaves either range and the last instruction ends exactly on the
//  boundary.  0x0048b8e0's two `ret`s are the two exits of its two
//  alternative top-level loops (0x0048bb9c and 0x0048bc4f); 0x0048be40's
//  one `ret` is at 0x0048c19d.  0x0048be40 is recursive (0x0048c077 calls
//  itself) AND self-tail-looping: 0x0048c179 and 0x0048c191 rewrite its own
//  argument slots and `jmp 0x0048be47`, three bytes past the entry, which is
//  VC6 turning the tail call into a loop.
//
//  WHY THEY ARE NOT DONE HERE.  The two plane filters above are CALLED out
//  of line by 0x0048b8e0 (0x0048baef, 0x0048bb58) and EXPANDED INLINE by
//  0x0048be40 (0x0048bfde..0x0048c059 is vis_FilterNeg, 0x0048c0f2..
//  0x0048c166 is vis_FilterPos, `fchs` included).  The rule --
//  "the inline decision is CALLER-CONTEXT-DEPENDENT, not a property of the
//  callee" -- says no keyword in one header separates those two sites, so
//  whoever writes 0x0048be40 has to write the filter bodies out by hand
//  inside it and leave the standalone pair alone.  That is a source-shape
//  decision this file deliberately does not pre-empt: the two functions
//  above are written as ordinary out-of-line helpers, which is what
//  0x0048b8e0's call sites want and what the presence of the standalone
//  bodies at 0x0048bca0/0x0048bd70 proves the original had.
//
//  THE STATE, all of it read off the bytes:
//
//    0x004b4d70  Obj**    the scratch buffer's limit -- `mov $0x4e30,%eax ;
//                         call 0x00492c60` (__chkstk) then
//                         `lea 0x4e34(%esp),%eax ; mov %eax,0x004b4d70`
//    0x004b4d74  Query*   0x0048b8e0's own argument, published at 0x0048b90f
//    0x004b4d78  uint32   a bit INDEX, incremented once per test
//    0x004b4d7c  uint8*   the bit array those indices walk:
//                           mov $1,%al ; and $7,%cl ; shl %cl,%al ;
//                           and (%ecx,%edi,1),%al ; inc <index>
//                         -- a sequential PVS read, which is what makes this
//                         the client's potentially-visible-set traversal
//    0x004b5220  void*    the current leaf/poly record
//    0x004b5224  Obj**    the current object cursor
//    0x004b5228  uint32   the filtered count (the fourth argument
//                         0x0048baef pushes is literally $0x4b5228)
//    0x004b522c  Obj**    the filtered array
//
//  THE QUERY CLASS, by displacement:
//    +0x00  a tree/root object; null takes the whole function down its
//           SECOND path at 0x0048bb9d
//    +0x04  the world, with +0x70/+0x74 an array of 0x18-byte records and
//           its count, +0x88/+0x8c an array of 0x30-byte records and its
//           count (`lea (%ecx,%ecx,2),%ecx ; shl $0x4,%ecx`), +0x9c a node
//           pointer and +0xb0 an array of 0x24-byte records
//           (`lea (%eax,%eax,8),%edx ; lea (%ecx,%edx,4)`)
//    +0x08  void (*)(Obj*, void* pUser)   -- "this object is visible"
//    +0x0c  void (*)(void* pRecord, void* pUser)
//    +0x10  void (*)(void* pNode, Obj*** ppArray, uint32* pnCount)
//           DEFAULTED to 0x0048bc50 at 0x0048b908 when the caller left it
//           null -- which is how that function's signature is known
//    +0x14  int  (*)(void* pRecord)       -- one argument, cdecl
//    +0x18  void* the user pointer handed to +0x08 and +0x0c
//    +0x1c  uint32 the visit MARK, compared against and written to an
//           object's +0x58 (`if (pObj->m_58 != mark) { pObj->m_58 = mark;
//           query->m_Fn08(pObj, query->m_pUser); }`)
//
//  THE NODE, by displacement: +0x04 and +0x08 the two children, +0x0c the
//  argument handed to the +0x10 callback, +0x14 a uint16 index with 0xffff
//  meaning "none", and each child's +0x16 carrying a flag byte whose bit 3
//  gates that side of the descent.
//
//  Its two callers are 0x00426b55 and 0x0045afaf, neither in this region.
// ======================================================================= //