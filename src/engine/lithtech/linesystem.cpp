// *********************************************************************** //
//
//  MODULE:   LINESYSTEM (.CPP)   (file name ours) -- lithtech.exe only
//
//  The client interface's line-system functions: GetNextLine, GetLineInfo,
//  SetLineInfo, AddLine and RemoveLine (AvP2 1.0.9.6 0x0043ce00 ..
//  0x0043d54f; Primal Hunt 0x00448f40 .. 0x00449689).  Their own
//  translation unit, after the world grid's and before lmessage.cpp's: in
//  Primal Hunt the run ends in its own <string> guard pair (0x00449690,
//  0x004496b0).  Split out of clientinterface.cpp, whose includes these
//  repeat; ci_SetupInterface still installs them.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x449690).
#include <string>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <io.h>
#include <string.h>
#include <math.h>

#include "clientinterface.h"
#include "clientmgr.h"
#include "cluglob.h"                // g_CluClientGlob, 0x004b0ac0
#include "commonmodel.h"
#include "de_error.h"
#include "iclientshell.h"


// A null previous line means start at the head, which is the tail node's own
// next pointer; reaching the tail node itself is the end of the list.
//
// IDENTICAL.  The shipped code writes the "is it the tail" test out twice,
// once per branch.  An early refusal, then an if/else whose arms each test
// and return with their own if/else (reading m_pNext at the test and again
// at the return), keeps the two copies; the ternary form let VC6 cross-jump
// them into one shared tail (56 bytes).
HLTLINE ci_GetNextLine(HLOCALOBJ hObj, HLTLINE hPrev)
{
	CClientLineSystem* pSystem;
	CClientLine* pLine;

	if (!hObj || ((CClientObject*)hObj)->m_ObjectType != OT_LINESYSTEM)
		return LTNULL;

	pSystem = (CClientLineSystem*)hObj;
	if (hPrev)
	{
		pLine = (CClientLine*)hPrev;
		if (pLine->m_pNext == &pSystem->m_LineTail)
			return LTNULL;
		else
			return (HLTLINE)pLine->m_pNext;
	}
	else
	{
		if (pSystem->m_LineTail.m_pNext == &pSystem->m_LineTail)
			return LTNULL;
		else
			return (HLTLINE)pSystem->m_LineTail.m_pNext;
	}
}

void ci_GetLineInfo(HLTLINE hLine, LTLine* pLine)
{
	if (hLine && pLine)
	{
		uint32 i;

		for (i = 0; i < 2; i++)
		{
			pLine->m_Points[i].m_Pos = ((LTLine*)hLine)->m_Points[i].m_Pos;
			pLine->m_Points[i].r = ((LTLine*)hLine)->m_Points[i].r;
			pLine->m_Points[i].g = ((LTLine*)hLine)->m_Points[i].g;
			pLine->m_Points[i].b = ((LTLine*)hLine)->m_Points[i].b;
			pLine->m_Points[i].a = ((LTLine*)hLine)->m_Points[i].a;
		}
	}
}

// The line system as ci_SetLineInfo (0x0043ceb0) and ci_AddLine (0x0043d1c0)
// see it: clientmgr.h's CClientLine / CClientLineSystem with the fields
// between their pads named (views, names ours).  A line is the SDK's LTLine
// (two 0x1c-byte points) followed by its owner and its links; the system
// keeps a changed flag at 0x1b4 and the bounding box, centre and radius the
// two functions maintain from 0x1fc.
class CLineSystemQ43c;

class CLineQ43c : public LTLine
{
public:
	CLineSystemQ43c* m_pSystem;              // 0x038
	CLineQ43c*       m_pPrev;                // 0x03c
	CLineQ43c*       m_pNext;                // 0x040
};

class CLineSystemQ43c
{
public:
	uint8       m_pad_00[0xbc];
	uint8       m_ObjectType;                // 0x0bc
	uint8       m_pad_bd[0x1b0 - 0xbd];
	StructBank* m_pLineBank;                 // 0x1b0
	LTBOOL      m_bChanged;                  // 0x1b4
	CLineQ43c   m_LineHead;                  // 0x1b8
	LTVector    m_MinPos;                    // 0x1fc
	LTVector    m_MaxPos;                    // 0x208
	LTVector    m_SystemCenter;              // 0x214
	float       m_SystemRadius;              // 0x220
};

// What both functions do per point and then once: SetLineInfo reloads the
// line's owner exactly once per group (0x0043cf1f, 0x0043cfec, 0x0043d0bd),
// which is an argument evaluated per call, not a member path per statement.
// Stand-in, name ours.
inline void ls_ExtendBoxQ43c(CLineSystemQ43c* pSystem, const LTVector& vPos)
{
	VEC_MIN(pSystem->m_MinPos, pSystem->m_MinPos, vPos);
	VEC_MAX(pSystem->m_MaxPos, pSystem->m_MaxPos, vPos);
}

// THE CENTRE AND RADIUS -- AN INLINE-DEPTH STAND-IN.  Shipped expands the
// LTVector operator bodies here but CALLS the three-float constructor
// (0x00411d40, both functions) and Mag (0x00436760, both), and ci_AddLine
// calls operator+ whole (0x004316d0) where ci_SetLineInfo expands it.  That
// is VC6's inline depth: the calls appear only this far down.  Measured:
// with 5 forwarders or fewer everything expands; 6 calls the constructors
// only; 7 calls every operator.  Mag and AddLine's operator+ each sit one
// level deeper again.  The original presumably reached the depth through
// its line-system helpers; this chain is only the depth (names ours).
// AddLine's operator+ is a statement of its own one level down: the shipped
// copy into m_SystemCenter reads through the pointer the call returns, which
// a helper returning the sum by value does not give.  Both functions build
// vHalf by ASSIGNMENT from the difference (a constructed temporary copied in),
// not by copy-initialisation.
inline float ls_MagQ43c(const LTVector& v) { return v.Mag(); }

inline void ls_SetCenterQ43c(CLineSystemQ43c* pSystem, const LTVector& vHalf)
{
	pSystem->m_SystemCenter = pSystem->m_MinPos + vHalf;
}

inline void ls_UpdateCenterQ43c(CLineSystemQ43c* pSystem)
{
	LTVector vHalf;

	vHalf = pSystem->m_MaxPos - pSystem->m_MinPos;
	vHalf *= 0.5f;
	pSystem->m_SystemCenter = pSystem->m_MinPos + vHalf;
	pSystem->m_SystemRadius = ls_MagQ43c(vHalf) + 1.0f;
}

// ci_AddLine's: the same, with operator+ one level down.
inline void ls_UpdateCenterAddQ43c(CLineSystemQ43c* pSystem)
{
	LTVector vHalf;

	vHalf = pSystem->m_MaxPos - pSystem->m_MinPos;
	vHalf *= 0.5f;
	ls_SetCenterQ43c(pSystem, vHalf);
	pSystem->m_SystemRadius = ls_MagQ43c(vHalf) + 1.0f;
}

inline void ls_Center1Q43c(CLineSystemQ43c* p) { ls_UpdateCenterQ43c(p); }
inline void ls_Center2Q43c(CLineSystemQ43c* p) { ls_Center1Q43c(p); }
inline void ls_Center3Q43c(CLineSystemQ43c* p) { ls_Center2Q43c(p); }
inline void ls_Center4Q43c(CLineSystemQ43c* p) { ls_Center3Q43c(p); }
inline void ls_Center5Q43c(CLineSystemQ43c* p) { ls_Center4Q43c(p); }
inline void ls_Center6Q43c(CLineSystemQ43c* p) { ls_Center5Q43c(p); }

inline void ls_CenterAdd1Q43c(CLineSystemQ43c* p) { ls_UpdateCenterAddQ43c(p); }
inline void ls_CenterAdd2Q43c(CLineSystemQ43c* p) { ls_CenterAdd1Q43c(p); }
inline void ls_CenterAdd3Q43c(CLineSystemQ43c* p) { ls_CenterAdd2Q43c(p); }
inline void ls_CenterAdd4Q43c(CLineSystemQ43c* p) { ls_CenterAdd3Q43c(p); }
inline void ls_CenterAdd5Q43c(CLineSystemQ43c* p) { ls_CenterAdd4Q43c(p); }
inline void ls_CenterAdd6Q43c(CLineSystemQ43c* p) { ls_CenterAdd5Q43c(p); }

// 0x0043ceb0.  Copy the new points in, grow the owner's box to take them,
// and recompute its centre and radius.
void ci_SetLineInfo(HLTLINE hLine, LTLine* pLine)
{
	if (hLine && pLine)
	{
		CLineQ43c* pDst = (CLineQ43c*)hLine;
		uint32 i;

		for (i = 0; i < 2; i++)
		{
			pDst->m_Points[i].m_Pos = pLine->m_Points[i].m_Pos;
			pDst->m_Points[i].r = pLine->m_Points[i].r;
			pDst->m_Points[i].g = pLine->m_Points[i].g;
			pDst->m_Points[i].b = pLine->m_Points[i].b;
			pDst->m_Points[i].a = pLine->m_Points[i].a;
		}

		ls_ExtendBoxQ43c(pDst->m_pSystem, pDst->m_Points[0].m_Pos);
		ls_ExtendBoxQ43c(pDst->m_pSystem, pDst->m_Points[1].m_Pos);
		ls_Center6Q43c(pDst->m_pSystem);

		pDst->m_pSystem->m_bChanged = LTTRUE;
	}
}

// 0x0043d1c0.  A new line from the system's bank, linked in before the head
// node (so at the end of the walk), filled, and folded into the box.  The
// allocation is not tested (0x0043d20b skips only the pop).
HLTLINE ci_AddLine(HLOCALOBJ hObj, LTLine* pLine)
{
	CLineSystemQ43c* pSystem;
	CLineQ43c* pNew;
	uint32 i;

	if (!hObj || !pLine || ((CLineSystemQ43c*)hObj)->m_ObjectType != OT_LINESYSTEM)
		return LTNULL;

	pSystem = (CLineSystemQ43c*)hObj;
	pNew = (CLineQ43c*)sb_Allocate(pSystem->m_pLineBank);

	// Both back-link stores go through the new line's own links: 0x0043d227
	// stores the head's back link through the just-written m_pNext (%eax),
	// not as a system field.
	pNew->m_pNext = &pSystem->m_LineHead;
	pNew->m_pPrev = pSystem->m_LineHead.m_pPrev;
	pNew->m_pNext->m_pPrev = pNew;
	pNew->m_pPrev->m_pNext = pNew;
	pNew->m_pSystem = pSystem;

	for (i = 0; i < 2; i++)
	{
		pNew->m_Points[i].m_Pos = pLine->m_Points[i].m_Pos;
		pNew->m_Points[i].r = pLine->m_Points[i].r;
		pNew->m_Points[i].g = pLine->m_Points[i].g;
		pNew->m_Points[i].b = pLine->m_Points[i].b;
		pNew->m_Points[i].a = pLine->m_Points[i].a;
	}

	ls_ExtendBoxQ43c(pSystem, pNew->m_Points[0].m_Pos);
	ls_ExtendBoxQ43c(pSystem, pNew->m_Points[1].m_Pos);
	ls_CenterAdd6Q43c(pSystem);

	pSystem->m_bChanged = LTTRUE;

	return (HLTLINE)pNew;
}

// Unlinking a line does not free it: it goes back on the system's free list,
// through the same word at offset zero the allocator hands out.
void ci_RemoveLine(HLOCALOBJ hObj, HLTLINE hLine)
{
	if (hObj && hLine && ((CClientObject*)hObj)->m_ObjectType == OT_LINESYSTEM)
	{
		CClientLine* pLine = (CClientLine*)hLine;

		pLine->m_pNext->m_pPrev = pLine->m_pPrev;
		pLine->m_pPrev->m_pNext = pLine->m_pNext;

		if (((CClientLineSystem*)hObj)->m_pLineBank)
			sb_Free(((CClientLineSystem*)hObj)->m_pLineBank, pLine);
	}
}
