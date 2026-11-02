// *********************************************************************** //
//
//  MODULE:   CLIENTCONSOLE (.CPP) -- lithtech.exe 0x0041c910..0x0041d033
//
//  PURPOSE:  The CLIENT console object's own methods: the text-line ring it
//            allocates, the Enter / Tab / arrow-key editing operations, the
//            console-variable refresh, and the geometry recompute.
//            (reconstruction; build cl 8447 /O2,
//            /I ..\..\LT2\sdk\inc /I ..\server /I .)
//
//  ------------------------------------------------------------------ //
//  WHAT THIS IS AND HOW IT WAS IDENTIFIED
//  ------------------------------------------------------------------ //
//
//  Not from a command table -- from the OBJECT.  Every one of the ten
//  functions is a thiscall on the same `this`, and the fields it touches are
//  already named by two committed files:
//
//    * src/engine/lithtech/clientconsole.cpp lays out the console's input
//      line at CConsole+0x10 (CConsoleLine: text at +0x08, colour at +0x10c,
//      caret at +0x12c), the ring history at CConsole+0x3cc
//      (CConsoleHistory::Add 0x0041c110, ::SetSize 0x0041c180), the name-list
//      base class and its ten-slot vtable (FindFirst +0x14, FindLast +0x18,
//      FindNext +0x1c, FindPrev +0x20, GetName +0x24), CConsoleLine::Clear
//      0x0041bb90 and ::SetText 0x0041bba0, and the console object itself at
//      0x004b2af8 with its history pointer at +0x3e8 (set by 0x0041cba0).
//    * src/engine/lithtech/lt2new4.cpp already reconstructed 0x0041cd10 --
//      the mode setter that memcpy's the 0x100 bytes at +0x18 (the line's
//      text) into the 0x100 bytes at +0x148 -- and named 0x0041cd40 as the
//      matching clear of +0x0c.
//
//  Those two facts settle the whole cluster: +0x0c is the editing mode
//  (0 = plain typing, 1 = name completion, 2 = history search), +0x148 is
//  the text SAVED when the mode was entered, and every function here is an
//  operation on the console's input line.  0x0041d3a0, the key dispatcher
//  immediately after the range, confirms it -- its 32-entry jump table
//  covers VK 0x09..0x28 and routes RETURN to 0x0041ca10, TAB to
//  0x0041cbc0 / 0x0041cc30 / 0x0041cca0, UP/DOWN to 0x0041cae0, and
//  PRIOR/NEXT/HOME/END to 0x0041cee0.
//
//  The string at 0x004a880c is ">%s*>" and it is the ONLY literal in the
//  range; 0x004a8814 "console.pcx" and 0x004a7638 "rb" belong to the
//  neighbour 0x0041cd50, which is NOT in this cluster.
//
//  ------------------------------------------------------------------ //
//  WHAT WAS REUSED
//  ------------------------------------------------------------------ //
//
//    * clientconsole.cpp -- CConsoleLine's layout, CConsoleHistory's layout
//      and non-virtual entry points, the name-list vtable slot numbering,
//      con_PrintOn's argument order (console, colour, level, format, ...),
//      dalloc/dfree, and the console object's address.
//    * lt2new4.cpp -- 0x0041cd10 SetMode and 0x0041cd40's role.
//    * src/engine/lithtech/clientinterface.h -- the client interface TABLE
//      (a flat struct of cdecl function pointers, not a vtable): +0x160
//      OptimizeSurface(HSURFACE, HLTCOLOR) and +0x1d4
//      SetSurfaceAlpha(HSURFACE, float).  That table hangs off a manager
//      pointer at +0x4c0, which src/engine/lithtech/cltcursor.cpp already
//      names m_pClientDE.
//    * src/LT2/sdk/inc/ltrect.h -- LTRect, whose default constructor
//      `LTRect(int l=0,int t=0,int r=0,int b=0)` is exactly the four
//      ascending zero stores 0x0041cf30 emits for each of its two locals,
//      and whose four-argument form is exactly what 0x0041ca60 builds from
//      the four console variables.
//    * src/LT2/sdk/inc/ltbasedefs.h -- LTMIN/LTMAX, whose double macro expansion
//      is what makes 0x0041cee0 form the same LTMAX twice.
//
//  NOTHING in this cluster was already reconstructed: grepping both
//  src/engine/lithtech and src/engine/server for all ten addresses, and for
//  the 0x10xxxxxx server twins of the fields they touch, finds only the
//  NEIGHBOURS listed above (0x0041cba0, 0x0041cd10, 0x0041cd40, 0x0041cd50,
//  0x0041c110, 0x0041c180, 0x0041bb90, 0x0041bba0) and never a body here.
//
//  NAMING.  Every class and global below carries a `Ccn` prefix so that this
//  file can sit in one build beside clientconsole.cpp and ccclient.cpp,
//  which declare their own incompatible views of the same objects.
//
// *********************************************************************** //

#include <string.h>
#include <stdlib.h>
#include "servermgr.h"          // -> ltbasedefs.h -> lithtech.h -> ltrect.h


// ----------------------------------------------------------------------- //
//  What this run calls but does not define
// ----------------------------------------------------------------------- //

// src/engine/lithtech/stdlithalloc.cpp names both of these.
void* dalloc(unsigned long nSize);                          // 0x004290b0
void  dfree(void* pMem);                                    // 0x004290c0


// The flat client-interface table, as src/engine/lithtech/clientinterface.h
// lays it out.  Only the two entries this run calls are declared; both are
// cdecl (the caller does the `add $8,%esp`) and neither is virtual.
class CcnClientTable
{
public:
	uint8    m_pad_000[0x160];
	LTRESULT (*OptimizeSurface)(void* hSurface, uint32 hTransparentColor);   // 0x160
	uint8    m_pad_164[0x1d4 - 0x164];
	LTRESULT (*SetSurfaceAlpha)(void* hSurface, float fAlpha);               // 0x1d4
};

// The manager the console reaches that table through.  The global is
// 0x004b0b14 -- NOT clientmgr.h's g_pClientMgr at 0x004b17e0, and NOT
// clientconsole.cpp's 0x004b2f44.  It is read at twenty-three sites in
// lithtech.exe (the ci_* surface calls around 0x00403500 and 0x0040ed80, the
// console here, and 0x00462000) and WRITTEN at none of them; its address is
// never even taken, so what fills it could not be read out of this image.
// (name ours)
class CcnClientMgr
{
public:
	uint8           m_pad_000[0x4c0];
	CcnClientTable* m_pClientDE;             // 0x4c0  (cltcursor.cpp's name)
};

// 0x004b0b14 is cluglob.h's g_CluClientGlob.m_pClientMgr (+0x054): a FIELD of
// the one client glob, so it is read through that object, as this view.
#include "cluglob.h"
#define g_pCcnClientMgr ((CcnClientMgr*)g_CluClientGlob.m_pClientMgr)  // 0x004b0b14


// ----------------------------------------------------------------------- //
//  The console variables this run reads
// ----------------------------------------------------------------------- //
//
// Seven consecutive dwords at 0x004a9f94..0x004a9fac, and their SHIPPED
// INITIALISERS are what name them: 20, 500, 1.0f, -1, -1, -1, -2.  The
// 1.0f is the only float in the group and it is the value handed to
// SetSurfaceAlpha; the last four are the four fields of an LTRect and -1
// is this engine's "use the default" marker.  They are the console
// bindings "ConsoleHistoryLen" .. "ConsoleBottom" (lithtech.exe's table at
// 0x004aa670..0x004aa6e8), defined under their server names in
// src/engine/server/dsi.cpp.

extern int   g_CV_ConsoleHistoryLen;                          // 0x004a9f94  20
extern int   g_CV_ConsoleBufferLen;                           // 0x004a9f98  500
extern float g_CV_ConsoleAlpha;                               // 0x004a9f9c  1.0f
extern int   g_CV_ConsoleLeft;                                // 0x004a9fa0  -1
extern int   g_CV_ConsoleTop;                                 // 0x004a9fa4  -1
extern int   g_CV_ConsoleRight;                               // 0x004a9fa8  -1
extern int   g_CV_ConsoleBottom;                              // 0x004a9fac  -2


// ----------------------------------------------------------------------- //
//  The objects the console is built out of
// ----------------------------------------------------------------------- //

// One line of scrollback.  0x0041c910 allocates exactly 0x10c bytes and
// zeroes 0x43 dwords, and threads the record through two pointers at +0x00
// and +0x04; the remaining 0x104 bytes are the colour and the 0x100-byte
// text that 0x0041d1c0 fills, which is outside this run.
//
// WHICH POINTER IS WHICH is proven by the REMOVE at 0x0041c9ad, not by the
// insert: it emits `p->f0->f4 = p->f4` and then `p->f4->f0 = p->f0`, the
// canonical unlink, and then advances the list head to the removed node's
// f4.  A head that moves FORWARD off a removed element makes f4 the next
// pointer and f0 the previous one.  (names ours)
class CcnTextLine
{
public:
	CcnTextLine* m_pPrev;                    // 0x000
	CcnTextLine* m_pNext;                    // 0x004
	uint8        m_Rest[0x10c - 0x008];      // 0x008  colour + 0x100 of text
};


// clientconsole.cpp's CConsoleNameList, under a non-colliding name.  Only
// the five slots this run dispatches through are declared, and they are
// declared NON-pure because CcnHistory is a member of CcnConsole below and
// an abstract member is not a member.
class CcnNameList
{
public:
	virtual LTBOOL First();                            // +0x00
	virtual LTBOOL Last();                             // +0x04
	virtual LTBOOL Next();                             // +0x08
	virtual LTBOOL Prev();                             // +0x0c
	virtual ~CcnNameList();                            // +0x10
	virtual LTBOOL FindFirst(char* pPrefix);           // +0x14
	virtual LTBOOL FindLast(char* pPrefix);            // +0x18
	virtual LTBOOL FindNext(char* pPrefix);            // +0x1c
	virtual LTBOOL FindPrev(char* pPrefix);            // +0x20
	virtual char*  GetName();                          // +0x24
};

// clientconsole.cpp's CConsoleHistory: 0x18 bytes, vtable 0x004a1e60.  Only
// the two NON-virtual entries this run calls are declared -- both are
// reached with plain `call <abs>` in the shipped code, which is what says
// they are not virtual.
class CcnHistory : public CcnNameList
{
public:
	void   Add(char* pText);                           // 0x0041c110
	LTBOOL SetSize(int nSize);                         // 0x0041c180

	uint8 m_pad_04[0x18 - 0x04];
};

// clientconsole.cpp's CConsoleLine, 0x130 bytes, at CConsole+0x10.  The one
// field this run WRITES that that file left unnamed is the four-dword block
// at +0x118: 0x0041cf30 stores a left/top/right/bottom set there, so it is
// an LTRect and not four scalars.
class CcnLine
{
public:
	void Clear();                                      // 0x0041bb90
	void SetText(char* pText);                         // 0x0041bba0

	uint32 m_Unknown00;                      // 0x000
	int    m_nLen;                           // 0x004
	char   m_szText[0x100];                  // 0x008
	uint32 m_Unknown108;                     // 0x108
	uint32 m_ColorText;                      // 0x10c
	uint32 m_ColorBack;                      // 0x110
	uint32 m_ColorCaret;                     // 0x114
	LTRect m_rcText;                         // 0x118
	int    m_nUnknown128;                    // 0x128
	int    m_nCursor;                        // 0x12c
};


// ----------------------------------------------------------------------- //
//  The console
// ----------------------------------------------------------------------- //
//
// Every offset below is a shipped displacement.  The three editing modes are
// read off the two mode tests in this range: 0x0041cbc0 and 0x0041cc30 test
// for 1 and set 1 (name completion), 0x0041cae0 tests for 2 and sets 2
// (history search), and 0x0041cd40 clears it.
//
// The object is the file-scope console at 0x004b2af8.

#define CCN_MODE_NONE       0
#define CCN_MODE_COMPLETE   1
#define CCN_MODE_HISTORY    2

class CcnConsole
{
public:
	// --- defined in this file -------------------------------------- //
	LTBOOL SetNumLines(uint32 nNumLines);              // 0x0041c910
	void   OnEnter();                                  // 0x0041ca10
	void   RefreshFromVars();                          // 0x0041ca60
	void   SearchHistory(int nCount);                  // 0x0041cae0
	void   CompleteNext();                             // 0x0041cbc0
	void   CompletePrev();                             // 0x0041cc30
	void   ListMatches();                              // 0x0041cca0
	void   SetAlpha(float fAlpha);                     // 0x0041ce70
	void   Scroll(int nDelta);                         // 0x0041cee0
	void   SetRect(LTRect* pRect);                     // 0x0041cf30

	// --- elsewhere -------------------------------------------------- //
	void   AddTextLine(uint32 nColor, int nLevel, char* pText);  // 0x0041d1c0
	void   GetTextRect(LTRect* pRect);                 // 0x0041d040
	void   SetMode(int nMode);                         // 0x0041cd10 (lt2new4)
	void   ClearMode();                                // 0x0041cd40

	uint8        m_pad_000[0x00c];
	int          m_nMode;                    // 0x00c
	CcnLine      m_Line;                     // 0x010 .. 0x140
	uint8        m_pad_140[0x148 - 0x140];
	char         m_szSaved[0x100];           // 0x148  SetMode's snapshot
	uint8        m_pad_248[0x26c - 0x248];
	LTRect       m_rcConsole;                // 0x26c
	uint8        m_pad_27c[0x2a0 - 0x27c];
	uint16       m_nFontHeight;              // 0x2a0
	uint8        m_pad_2a2[0x3a4 - 0x2a2];
	void*        m_hSurface;                 // 0x3a4
	float        m_fAlpha;                   // 0x3a8
	LTRESULT     m_nOptimized;               // 0x3ac
	uint8        m_pad_3b0[0x3bc - 0x3b0];
	int          m_nNumLines;                // 0x3bc
	CcnTextLine* m_pLines;                   // 0x3c0
	int          m_nNumVisible;              // 0x3c4
	int          m_nScrollPos;               // 0x3c8
	CcnHistory   m_History;                  // 0x3cc .. 0x3e4
	void        (*m_pfnCommand)(char* pText);// 0x3e4
	CcnNameList* m_pNameList;                // 0x3e8
};

// 0x0041d380 -- clientconsole.cpp's con_PrintOn, on the console passed in
// rather than the global.  Variadic and cdecl; the five pushes and the
// `add $0x14,%esp` at 0x0041cccc are what count its arguments.
void ccn_PrintOn(CcnConsole* pConsole, uint32 nColor, int nLevel,
                 char* pFormat, ...);                       // 0x0041d380


// ======================================================================= //
//  0x0041c910  254 bytes  --  grow or shrink the scrollback ring
// ======================================================================= //
//
// WHAT PINS IT
//   * `ret $4` with `mov %ecx,%esi` -> thiscall, one argument; `mov $1,%eax`
//     at BOTH exits -> LTBOOL, always true.
//   * THE COMPARISONS ARE UNSIGNED.  0x0041c92f is `jae` and 0x0041c9ab is
//     `jbe`, both against the same field 0x3bc that 0x0041cee0 later feeds to
//     a SIGNED `sub`/`inc`/`jge`.  One field cannot be both, so the mixed
//     signedness is in the PARAMETER: an `int` member compared against a
//     `uint32` argument promotes the whole comparison to unsigned, and
//     0x0041cee0's all-member arithmetic stays signed.  Hence
//     `uint32 nNumLines` over `int m_nNumLines`.
//   * two `while` loops, both rotated the normal /O2 way: a pre-test that
//     jumps PAST the loop (0x0041c92f, 0x0041c9ab) and a bottom test that
//     jumps BACK to its head (0x0041c9a3, 0x0041ca01).
//   * `push $0x10c; call 0x4290b0` then `mov $0x43,%ecx; rep stos` -- 0x43
//     dwords is 0x10c bytes exactly, so the memset size is the same
//     sizeof(), and there is NO null check on the allocation.
//   * THE STORE ORDER IS THE SOURCE ORDER and it is not the obvious one:
//     the old tail's forward pointer is fixed FIRST (0x0041c957), before
//     the new node is filled in.  It has to be read that way round because
//     every store goes through a CcnTextLine*, which may alias the member
//     `m_pLines` -- which is why the shipped code RELOADS
//     `m_pLines->m_pPrev` at 0x0041c95a and `m_pLines` at 0x0041c969.  The
//     head itself, though, stays in %eax across the four link stores
//     (0x0041c955..0x0041c961), so it is read once into a local `pHead`.
//   * the shrink loop holds the node TWICE: %eax for the unlinking and the
//     head compare, and a copy in %edx (0x0041c9b3) for dfree.  Two locals,
//     `pLine` and `pHead`, both loaded from m_pLines, give that copy.
//   * RESIDUAL (near, 250 vs 254): in the grow loop's linked arm shipped
//     keeps the count in %edi (0x0041c963) and so re-tests the MEMBER at the
//     loop bottom (0x0041c99d, `cmp %ebx,0x3bc(%esi)`); ours keeps it in
//     %eax and tests the register.  Pre/post/`+= 1` increments, moving the
//     increment, braces, swapping the arms, block-scoped locals, a `for`
//     loop and local declaration order all leave it -- allocation.
//   * `cmp %eax,%ecx` at 0x0041c979 has the LEFT operand `pLine->m_pNext`
//     (a fresh load, so the source names the field, not the local) and at
//     0x0041c9ca has the left operand `pLine`.  Operand order survives.
//   * the head fixup in the shrink loop reads `m_pLines->m_pNext`, not
//     `pLine->m_pNext`: 0x0041c9ce indexes %ecx, which is the RELOADED
//     `m_pLines`, not %eax.
LTBOOL CcnConsole::SetNumLines(uint32 nNumLines)
{
	CcnTextLine* pLine;
	CcnTextLine* pHead;

	if (m_nNumLines == nNumLines)
		return LTTRUE;

	while (m_nNumLines < nNumLines)
	{
		pLine = (CcnTextLine*)dalloc(sizeof(CcnTextLine));
		memset(pLine, 0, sizeof(CcnTextLine));

		pHead = m_pLines;

		if (pHead)
		{
			pHead->m_pPrev->m_pNext = pLine;
			pLine->m_pPrev = pHead->m_pPrev;
			pLine->m_pNext = pHead;
			pHead->m_pPrev = pLine;

			m_nNumLines++;

			if (pLine->m_pNext == m_pLines)
				m_pLines = pLine;
		}
		else
		{
			pLine->m_pNext = pLine;
			pLine->m_pPrev = pLine;

			m_pLines = pLine;
			m_nNumLines++;
		}
	}

	while (m_nNumLines > nNumLines)
	{
		pLine = m_pLines;
		pHead = m_pLines;

		pHead->m_pPrev->m_pNext = pHead->m_pNext;
		pHead->m_pNext->m_pPrev = pHead->m_pPrev;

		if (pHead == m_pLines)
			m_pLines = m_pLines->m_pNext;

		m_nNumLines--;

		if (m_nNumLines == 0)
			m_pLines = LTNULL;

		dfree(pLine);
	}

	return LTTRUE;
}


// ======================================================================= //
//  0x0041ca10  75 bytes  --  the RETURN key
// ======================================================================= //
//
// Reached from the key dispatcher's jump table at 0x0041d3c3 (VK_RETURN).
//
// WHAT PINS IT
//   * plain `ret` -> thiscall, no arguments, void.
//   * the FIRST call is 0x0041cd40 with %ecx untouched since the entry, so
//     it takes no argument: the mode is dropped before the line is read.
//   * `mov 0x18(%esi),%al; test %al,%al` is the line's first CHARACTER, not
//     its length field at +0x14 -- the load is a byte.
//   * `lea 0x18(%esi),%edi` once, then %edi pushed at three call sites: the
//     same expression `m_Line.m_szText` written at each.
//   * 0x0041d1c0 takes (colour, level, text) -- three pushes, no stack
//     cleanup, `ret $0xc`; clientconsole.cpp already fixed that order from
//     0x0041d320's use of its second argument as a level.
//   * `mov 0x3e4(%esi),%eax; test; call *%eax; add $4,%esp` -- a CDECL
//     function pointer with ONE argument, null-guarded.
void CcnConsole::OnEnter()
{
	ClearMode();

	if (m_Line.m_szText[0])
	{
		AddTextLine(m_Line.m_ColorText, 0, m_Line.m_szText);
		m_History.Add(m_Line.m_szText);

		if (m_pfnCommand)
			m_pfnCommand(m_Line.m_szText);

		m_Line.Clear();
	}
}


// ======================================================================= //
//  0x0041ca60  123 bytes  --  re-read every console variable
// ======================================================================= //
//
// One caller, 0x0041c4a5, inside the console's own setup.
//
// WHAT PINS IT
//   * plain `ret` with `add $0x10,%esp` -> thiscall, no arguments, void, and
//     16 bytes of locals: one LTRect.
//   * `mov 0x4a9f9c,%eax ... push %eax` passes 0x004a9f9c by INTEGER move.
//     That global ships holding 0x3f800000 and its consumer at 0x0041ce70
//     hands it straight to SetSurfaceAlpha(HSURFACE, float), so it is a
//     float; VC6 copies a float lvalue to a float parameter with a plain
//     dword push, and the load is hoisted to the very first instruction of
//     the function, which an `fld`/`fstp` pair could not be.
//   * `cmp $0xa,%eax; jg; mov $0xa,%eax` twice over, against 10 and 5, is
//     LTMAX's `((a) > (b) ? (a) : (b))` with the variable on the LEFT.
//   * `lea 0x3cc(%esi),%ecx; call 0x41c180` -- a DIRECT call, so
//     CcnHistory::SetSize is reached by object syntax on the member.
//   * the four stores into the local block are field stores written in the
//     order top, left, bottom, right: 0x0041caa3..0x0041cacd load 0x4a9fa4,
//     0x4a9fa0, 0x4a9fac, 0x4a9fa8 in that order.  LTRect's four-argument
//     constructor and Init() both load left/top/right/bottom instead
//     (measured: both near, stores at the wrong offsets).
void CcnConsole::RefreshFromVars()
{
	SetAlpha(g_CV_ConsoleAlpha);

	SetNumLines(LTMAX(g_CV_ConsoleBufferLen, 10));

	m_History.SetSize(LTMAX(g_CV_ConsoleHistoryLen, 5));

	LTRect rcNew;

	rcNew.top = g_CV_ConsoleTop;
	rcNew.left = g_CV_ConsoleLeft;
	rcNew.bottom = g_CV_ConsoleBottom;
	rcNew.right = g_CV_ConsoleRight;

	SetRect(&rcNew);
}


// ======================================================================= //
//  0x0041cae0  190 bytes  --  UP / DOWN: walk the history for a prefix
// ======================================================================= //
//
// Reached from the dispatcher at 0x0041d3ee (VK_UP, argument -1) and
// 0x0041d417 (VK_DOWN, argument +1).
//
// WHAT PINS IT
//   * `ret $4` -> thiscall, one argument; no value returned.
//   * `cltd; xor %edx,%edi; sub %edx,%edi` is the /Oi `abs()` intrinsic, and
//     `setg %bl` off the same `test` is `nCount > 0`.  The direction flag is
//     computed once and tested three times, so it is a local.
//   * THE TWO PREFIXES ARE DIFFERENT and that is the whole point of the
//     function: the FIRST search (0x0041cb13) uses `m_Line.m_szText` and
//     every later one (0x0041cb4a, 0x0041cb64) uses `m_szSaved`.  0x0041cd10
//     snapshots +0x18 into +0x148 as it sets the mode, so once the mode is
//     on, the typed prefix survives in m_szSaved while SetText overwrites
//     the line.
//   * the dispatch is INDIRECT (`mov 0x3cc(%esi),%eax; call *0x14(%eax)`)
//     even though the object at +0x3cc is a member: a virtual invoked on a
//     named object devirtualises in VC6, so the source must go
//     through a base-class POINTER.  %esi, %edi and %ebx are all already
//     taken, so VC6 has no callee-saved register to park that pointer in and
//     rematerialises `lea 0x3cc(%esi)` at each of the three sites -- which
//     is what the shipped code does.
//   * the loop is `while (n && bOk)`: the entry test at 0x0041cb31 is on the
//     counter, the loop's own head at 0x0041cb3a is the flag, and the bottom
//     `dec %edi; jne 0x41cb3a` folds the counter test into the decrement.
//     `mov $1,%eax` sits BEFORE the entry branch, so the flag is initialised
//     above the loop and is live on the skip path.
//   * after the loop, `test %eax,%eax; jne` picks between two tails: found
//     -> SetText(GetName()), not found -> SetText(m_szSaved) and drop the
//     mode.  The NOT-found tail is laid out first (0x0041cb64, before the
//     found tail at 0x0041cb80), so the source tests `!bOk` first; with
//     `if (bOk)` first the two tails come out swapped.  The zero-count path at 0x0041cb38 jumps straight into the
//     found tail, which is only correct because the flag is already true.
void CcnConsole::SearchHistory(int nCount)
{
	CcnNameList* pList;
	LTBOOL       bForward;
	int          nLeft;
	LTBOOL       bOk;

	if (nCount == 0)
		return;

	bForward = (nCount > 0);
	nLeft = abs(nCount);

	if (m_nMode != CCN_MODE_HISTORY)
	{
		pList = &m_History;

		if (bForward)
			bOk = pList->FindFirst(m_Line.m_szText);
		else
			bOk = pList->FindLast(m_Line.m_szText);

		if (!bOk)
			return;

		SetMode(CCN_MODE_HISTORY);

		nLeft--;
	}

	bOk = LTTRUE;

	while (nLeft != 0 && bOk)
	{
		pList = &m_History;

		if (bForward)
			bOk = pList->FindNext(m_szSaved);
		else
			bOk = pList->FindPrev(m_szSaved);

		nLeft--;
	}

	if (!bOk)
	{
		m_Line.SetText(m_szSaved);

		ClearMode();
	}
	else
	{
		pList = &m_History;

		m_Line.SetText(pList->GetName());
	}
}


// ======================================================================= //
//  0x0041cbc0  98 bytes  --  TAB: next completion
//  0x0041cc30  98 bytes  --  shift-TAB: previous completion
// ======================================================================= //
//
// The dispatcher at 0x0041d445 picks between them on a modifier key, and
// together with 0x0041cca0 below they are ONE FAMILY of three: same
// prologue (`push %esi; mov %ecx,%esi; push %edi`), same 98 bytes, same
// `m_pNameList` and `m_szSaved` pair, differing only in which of the
// name-list's four prefix-search slots they dispatch through.  /OPT:ICF did
// not fold them, which is the proof they are three bodies and not one.
//
// WHAT PINS THEM
//   * plain `ret` -> thiscall, no arguments, void.
//   * `mov 0x3e8(%esi),%ecx` ONCE, above the mode test, then `mov (%ecx)`
//     inside each arm: the member pointer is written in both arms of the
//     source and VC6 hoists the load because no call separates them.  It is
//     RELOADED at 0x0041cbe8 because a call does sit in front of that use.
//   * the mode test is `cmp $1,%eax; je <the else>`, so the source tests
//     `!=` and the completion-already-running case is the ELSE.  The shared
//     tail sits BETWEEN the two arms -- the `!=` arm falls into it and the
//     `==` arm jumps back to it -- which is the layout VC6 gives when the
//     then-arm's fall-through is the tail.
//   * the prefix handed to FindFirst/FindLast is the LINE (+0x18) and the
//     prefix handed to FindNext/FindPrev is the SNAPSHOT (+0x148), for the
//     same reason as 0x0041cae0: SetMode took the snapshot.
//   * `push $1; call 0x41cd10` -- mode 1 is name completion, against
//     0x0041cae0's mode 2.
void CcnConsole::CompleteNext()
{
	if (m_nMode != CCN_MODE_COMPLETE)
	{
		if (!m_pNameList->FindFirst(m_Line.m_szText))
			return;

		SetMode(CCN_MODE_COMPLETE);
	}
	else
	{
		if (!m_pNameList->FindNext(m_szSaved))
		{
			m_Line.SetText(m_szSaved);

			ClearMode();

			return;
		}
	}

	m_Line.SetText(m_pNameList->GetName());
}

// 0x0041cc30 -- the same walking backwards: slots +0x18 (FindLast) and
// +0x20 (FindPrev) against +0x14 and +0x1c.
void CcnConsole::CompletePrev()
{
	if (m_nMode != CCN_MODE_COMPLETE)
	{
		if (!m_pNameList->FindLast(m_Line.m_szText))
			return;

		SetMode(CCN_MODE_COMPLETE);
	}
	else
	{
		if (!m_pNameList->FindPrev(m_szSaved))
		{
			m_Line.SetText(m_szSaved);

			ClearMode();

			return;
		}
	}

	m_Line.SetText(m_pNameList->GetName());
}


// ======================================================================= //
//  0x0041cca0  98 bytes  --  ctrl-TAB: print every completion
// ======================================================================= //
//
// The third of the 98-byte family.  It touches no mode at all: it prints
// rather than edits.
//
// WHAT PINS IT
//   * `push $0x4a880c` is ">%s*>", the range's only string literal, and the
//     five pushes plus `add $0x14,%esp` make the call 0x0041d380's
//     five-argument variadic printer with the console itself as the first
//     argument.  It is emitted ONCE, above the loop, not inside it: its
//     block at 0x0041ccb7 is not a branch target.
//   * the loop head is the GetName() at 0x0041cccf, reached by fall-through
//     the first time and by the `jne` at 0x0041ccfd afterwards: the loop is
//     NOT rotated.  A do/while on FindNext whose body is an if/else with the
//     `break` in the else gives that; a `while (GetName())`, a `for (;;)`
//     with two breaks, a goto loop, and a do/while with `if (!p) break;`
//     all rotate it (GetName duplicated at the bottom, 113 bytes).
//   * the printed lines go through 0x0041d1c0 (three pushes, `ret $0xc`),
//     not through the variadic printer: only the banner is formatted.
//   * `m_Line.m_ColorText` is loaded TWICE, once for the banner and once
//     inside the loop, with calls between: the expression is written at both
//     sites rather than cached.
//   * `lea 0x18(%esi),%edi` once and pushed at three sites -- the same
//     `m_Line.m_szText` at all three.
void CcnConsole::ListMatches()
{
	char* pName;

	if (!m_pNameList->FindFirst(m_Line.m_szText))
		return;

	ccn_PrintOn(this, m_Line.m_ColorText, 0, ">%s*>", m_Line.m_szText);

	do
	{
		pName = m_pNameList->GetName();

		if (pName)
			AddTextLine(m_Line.m_ColorText, 0, pName);
		else
			break;
	}
	while (m_pNameList->FindNext(m_Line.m_szText));
}


// ======================================================================= //
//  0x0041ce70  99 bytes  --  set the backdrop's alpha
// ======================================================================= //
//
// Two callers: 0x0041ca6c (the console variable, above) and 0x0041ce15
// (0x0041cd50, which re-loads console.pcx and then re-applies the alpha it
// already holds at +0x3a8).
//
// WHAT PINS IT
//   * `ret $4` -> thiscall, one argument, void.
//   * the whole body is inside `if (m_hSurface)`: 0x0041ce7b jumps over
//     everything, and +0x3a4 is the surface 0x0041cd50 stores there.
//   * the argument is copied to +0x3a8 by integer `mov` and later pushed as
//     the second argument of clientinterface.h's
//     SetSurfaceAlpha(HSURFACE, float) at table offset +0x1d4 -- so both the
//     parameter and the member are floats, and VC6's float copies are
//     integer moves.
//   * +0x160 is OptimizeSurface(HSURFACE, HLTCOLOR); both calls are cdecl
//     (`add $8,%esp`) and neither is a vtable dispatch -- the table is a
//     flat struct of function pointers, reached as
//     g_pCcnClientMgr->m_pClientDE.
//   * the manager global and `m_hSurface` are RELOADED for the second call
//     (0x0041ceaf, 0x0041cebf) and the parameter is reloaded from its home
//     slot (0x0041ceb4): every one of them is written twice in the source,
//     not cached, because a call sits between the two uses.
//   * RESIDUAL (near, 100 vs 99): the second call's operands land in
//     different registers (shipped: manager %eax -- the 5-byte `a1` load --
//     argument %edx, table %ecx, surface %eax; ours %edx/%ecx/%eax/%edx).
//     Same instruction stream.  A table local, an if-block body instead of
//     the early return, a named OptimizeSurface result and an explicit
//     float reinterpretation all leave it -- allocation.
void CcnConsole::SetAlpha(float fAlpha)
{
	if (!m_hSurface)
		return;

	m_fAlpha = fAlpha;

	if (m_nOptimized)
		m_nOptimized = g_pCcnClientMgr->m_pClientDE->OptimizeSurface(m_hSurface, 0);

	g_pCcnClientMgr->m_pClientDE->SetSurfaceAlpha(m_hSurface, fAlpha);
}


// ======================================================================= //
//  0x0041cee0  66 bytes  --  scroll the backlog
// ======================================================================= //
//
// NO CALLS AT ALL.  Six callers, all in the key dispatcher: 0x0041d3e0 and
// 0x0041d409 (ctrl-UP / ctrl-DOWN, +-1), 0x0041d471 (PRIOR with the whole
// line count), 0x0041d484 (PRIOR by a page, +m_nNumVisible), 0x0041d4a6
// (NEXT, -m_nScrollPos, i.e. all the way back) and 0x0041d4bb (NEXT by a
// page).
//
// WHAT PINS IT
//   * `ret $4` -> thiscall, one argument, void.  No frame: %esi and %edi are
//     pushed and popped INSIDE the body, which is VC6 shrink-wrapping.
//   * `xor %eax,%eax; test %edx,%edx; setle %al; dec %eax; and %edx,%eax` is
//     a branchless `x > 0 ? x : 0`, i.e. LTMAX(x, 0) -- and it is emitted
//     TWICE, at 0x0041ceec and again at 0x0041cf0f.  That double expansion
//     is the same LTMIN(LTMAX(...), ...) macro nesting clientconsole.cpp
//     already recorded for CConsoleLine::MoveCursor: the inner maximum is
//     formed once for the comparison and formed AGAIN for the value.
//   * the comparison is `cmp %eax,%esi; jge` -- SIGNED -- over
//     `m_nNumLines - m_nNumVisible + 1`, which is what makes +0x3bc an `int`
//     member and 0x0041c910's unsigned tests a property of THAT function's
//     parameter rather than of the field.
//   * the limit is the SECOND operand of the LTMIN (it is the value left in
//     %eax when the branch is taken), so the clamped sum is written first.
//   * RESIDUAL (near, 75 vs 66): shipped forms the sum in the PARAMETER's
//     register (`add %eax,%edx`) so the mask and the result both end in
//     %eax and the two arms share one store; ours sums into %eax and
//     duplicates the store.  Operand order of the sum, of LTMIN and of
//     LTMAX, `(a - b) + 1` / `a + 1 - b`, and named sum/limit/result locals
//     (which CSE the double LTMAX away) all measured -- allocation.
void CcnConsole::Scroll(int nDelta)
{
	m_nScrollPos = LTMIN(LTMAX(m_nScrollPos + nDelta, 0),
	                     m_nNumLines - m_nNumVisible + 1);
}


// ======================================================================= //
//  0x0041cf30  259 bytes  --  move/resize the console and re-lay the line
// ======================================================================= //
//
// One caller, 0x0041cad1, immediately above.
//
// WHAT PINS IT
//   * `ret $4` with `add $0x20,%esp` -> thiscall, one argument, void, and
//     32 bytes of locals: TWO LTRects, at %esp+0x14 and %esp+0x04.  They do
//     not share a slot, so they are two named locals.
//   * the four-way guard compares MEMBER on the left every time (`3b ca`
//     with the member in %ecx, `3b d1` with the member in %edx), and its
//     last arm is a `je` to the return while the first three are `jne` to
//     the body -- the shape of `if (all four equal) return;`.
//   * four zero stores in ascending field order before EACH of the two calls
//     to 0x0041d040: ltrect.h's default constructor
//     `LTRect(int l=0,int t=0,int r=0,int b=0)`, inlined, twice.
//   * `xor %ecx,%ecx; mov 0x2a0(%esi),%cx` then `cltd; idiv %ecx` -- a
//     uint16 member, zero-extended and then integer-promoted, so the divide
//     is signed.
//   * THE LINE COUNT IS A LOCAL, NOT THE MEMBER.  0x0041cfe4 stores it to
//     +0x3c4 and then 0x0041cffe and 0x0041d002 use %edi across the second
//     call without reloading, while 0x0041cff5 DOES reload m_nFontHeight
//     across that same call.  A member read after a call is reloaded; a
//     local is not.
//   * the second GetTextRect answers differently from the first only because
//     +0x3c4 was written between them, which is why the call is made twice
//     on the same argument shape.
//   * the four results go to +0x128..+0x134, i.e. CcnLine+0x118..+0x124,
//     in left/top/right/bottom order -- an LTRect inside the input line.
//   * the LTRect locals are declared AT THEIR CALLS: shipped runs each
//     constructor's zero stores right before its GetTextRect, not in the
//     prologue (declared at the top, both ran at entry and the function
//     did not even pair).  `m_rcConsole = *pRect` as a struct copy: the
//     field-by-field form re-orders the guard's loads (16 blocks).
//   * `inc %edi` before the bottom's multiply is an explicit `nVisible++`
//     between the top and the bottom; `(nVisible + 1)` gives a `lea`.
//   * RESIDUAL (measured, allocation): 79/79 instructions, 268 vs 259 bytes.
//     Shipped keeps all four results in registers (right in %edi) and then
//     rebases the dead `this` with `add $0x128,%esi` for 2-byte-displacement
//     stores; ours stores the top early and uses disp32.  Also `lea
//     -2(%eax),%edi` vs `mov/sub`.  Not moved by: Init vs four stores vs
//     LTRect temporary vs named rcLine copy, an LTRect* local (with Init or
//     four stores), a CcnLine inline setter (values or rect+line+height),
//     named nLeft/nRight, `nVisible -= 2` / a named height, nVisible
//     declared at use, rcText declared before the divide (worse).
void CcnConsole::SetRect(LTRect* pRect)
{
	int nVisible;

	if (m_rcConsole.left == pRect->left &&
	    m_rcConsole.top == pRect->top &&
	    m_rcConsole.right == pRect->right &&
	    m_rcConsole.bottom == pRect->bottom)
		return;

	m_rcConsole = *pRect;

	LTRect rcFull;
	GetTextRect(&rcFull);

	nVisible = (rcFull.bottom - rcFull.top) / m_nFontHeight - 2;

	m_nNumVisible = nVisible;

	LTRect rcText;
	GetTextRect(&rcText);

	int nTop = rcText.top + m_nFontHeight * nVisible + 5;
	nVisible++;
	int nBottom = rcText.top + m_nFontHeight * nVisible + 1;

	m_Line.m_rcText.Init(rcText.left + 8, nTop, rcText.right - 1, nBottom);
}


// ======================================================================= //
//  WHAT COULD NOT BE PINNED
// ======================================================================= //
//
//  * 0x004b0b14's identity.  It is a pointer to something whose +0x4c0 is
//    the client-interface table, which makes it a CClientMgr by every field
//    the rest of the image reads off it (+0x610 is the string-resource
//    module cltnew.cpp names) -- but it is NOT clientmgr.h's g_pClientMgr at
//    0x004b17e0, and lithtech.exe never writes it or takes its address at
//    any of its twenty-three reference sites.  Declared with a view class
//    and a name of ours.
//
//  * The two spare dwords at CcnConsole+0x140..+0x148, between the end of
//    the input line and the 0x100-byte snapshot.  Nothing in this range
//    touches them.
//
//  * Whether the console variables are `int`/`float` file-scope objects or
//    the storage behind registered console variables; nothing in this range
//    registers them, and their addresses are consecutive with the shipped
//    initialisers 20 / 500 / 1.0f / -1 / -1 / -1 / -2.
//
//  * The name-list pointer at +0x3e8 and the ring history at +0x3cc are two
//    DIFFERENT lists (0x0041cba0 sets the first, 0x0041c010 builds the
//    second in place), and nothing here says whether the source declares a
//    third accessor for the embedded one; `pList = &m_History` is the form
//    that reproduces the rematerialised `lea 0x3cc(%esi)`.
