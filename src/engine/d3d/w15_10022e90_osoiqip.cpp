// *********************************************************************** //
//
//  MODULE:   W15_10022E90_OSOIQIP (.CPP)  -- /Os /Oi- /Qip
//
//  PURPOSE:  0x10022e90 / 0x10022e9c, the console-line tokeniser: split
//            the command at pCommand into at most 30 arguments, each
//            copied NUL-terminated into the argument buffer and pointed
//            to from ppArgs, stopping at the end of one command (';' or
//            a control character) with the resume point stored through
//            ppNewCommand.  ConParse's (C10023130's) Parse at 0x100230d4
//            and ParseFind at 0x10023130 are its two callers.
//
//  Quoting: "..." is one argument; (...) is one argument with its outer
//  parentheses dropped and inner ones kept; % escapes the next character;
//  an argument longer than 255 characters keeps its first 255.
//
//  With it, ltbasedefs.h's ConParse (C10023130 here, same layout: m_Args
//  at +0x00, m_nArgs +0x100, m_pCommandPos +0x104, m_ArgBuffer +0x108):
//
//    0x100230d0/d4  Parse     -- moved here from w4_10023c40_os.cpp;
//    0x10023130     ParseFind -- moved here from d3dqip3.cpp.
//
//  All three enter each other through /Qip REGISTER entries in the shipped
//  image, so they must share one /Qip translation unit.  0x10022e90 and
//  0x100230d0 are EXTERNAL-linkage two-entry pairs (a stack entry that
//  loads %eax/%edx/%ecx, falling into the register entry the callers use).
//  ParseFind inlines Parse into its loop (the out-of-line call is only the
//  first one), which is plain /Qip inlining; the tokeniser is big enough
//  that /Qip leaves it out of line in both callers.
//
//  Flags, measured: /Os (single-nop padding, `xor %eax,%eax ; inc %eax`
//  for 1) /Qip, and /Oi- -- ParseFind CALLS strcmp (0x10080910), which
//  /Oi expands inline.  0x10022e90, 0x100230d0, 0x100230d4 and 0x10023130
//  are byte-identical.  ParseFind wants the header's signature: uint32
//  minTokens, compared SIGNED against m_nArgs (`jg`), and the `== 0 -> 1`
//  fix-up written against the unsigned parameter.
//
//  The helper now reads as server.dll's twin in the three
//  places that put the inner loop in memory: `if (!**ppPos) break; c =
//  **ppPos;` at the head, `c = **ppPos` re-read after a `%`, and the
//  stored character is `c`, with the end-of-command test `if (!*pStart)
//  { advance; return NEWCOMMAND; } return TOKEN;`.  The cursor and output
//  pointer then live in frame slots across the iscntrl call as shipped,
//  and the body is 556 bytes against the shipped 553 (was 428), the
//  tokeniser still out of line and ParseFind still identical (the full
//  twin, with parse_Command's shape too, gets the tokeniser inlined into
//  both callers again).  Still attempted by the pairing: the frame is
//  0x2c against 0x34 (the shipped code keeps pArgs and c in two more
//  slots), so even the first 16 bytes differ.
//
//  The command loop is now server.dll's parse_Command
//  shape too (if/else-if on the token type, `argBuffer = pOut` at the
//  bottom, the resume point stored after the loop), with the helper as
//  above: the tokeniser stays out of line (the three callers' rows still
//  identical), its opening now loads and stores the arguments in the
//  shipped order (`mov 0x24(%esp),%edi ; sub ; mov %edi,.. ; mov
//  0x4c(%esp),%ebx ; movl $0,(%edi) ; mov %ebx,%ebp ; mov %edx,%ebx`), and
//  the instruction match goes 0.42 -> 0.51 (589 bytes against 568).  The
//  frame is still 0x2c against 0x34: the shipped loop keeps the character
//  in TWO frame slots (0x14 and 0x10(%esp)) and re-reads the 0x14 copy for
//  the '(' ')' '"' ' ' tests after iscntrl.  `int c` instead of `char c`
//  makes /Qip inline the tokeniser into Parse (the register entries are
//  lost); `iscntrl((int)c)` changes nothing.
//
//  EARLIER NEAR NOTE: the tokeniser body 0x10022e9c (425 of 553 bytes).  Same blocks,
//  same order of tests, same switch on the helper's 0/1/2 result; what
//  differs is allocation -- the shipped inner loop keeps its cursor and
//  output pointer in frame slots (0x1c/0x18(%esp)), copied from and back
//  to %ebp/%esi at the helper's entry and exits, and two copies of the
//  character; icl here keeps them in registers.  Local copies, reference
//  parameters, pointer parameters, a separate end-of-command inline and a
//  non-`inline` helper were all built; none moves the loop into memory.
//  Server.dll's twin (src/engine/server/console.cpp's
//  parse_Token / parse_Command, the same LithTech source) written here
//  in its shape -- if/else-if on the token type and `argBuffer = pOut`
//  at the bottom of the loop -- comes out 185 lines against the shipped
//  177 (the 425-byte body here is 137), but /Qip then inlines the
//  tokeniser into both callers and 0x10022e90, 0x100230d4 and 0x10023130
//  stop matching.  Not kept; it wants /Qip_no_inlining on this file, which
//  ParseFind's inlined Parse would have to survive.
//
// *********************************************************************** //

#include <ctype.h>
#include <string.h>
#include "ltbasedefs.h"

#define CP_END                  0                       // nothing more
#define CP_TOKEN                1                       // one argument read
#define CP_NEWCOMMAND           2                       // end of the command

// Read one argument at *ppPos into the buffer at *ppOut.
static inline uint8 cp_GetToken(char** ppPos, char** ppOut)
{
	char*   pToken;
	int32   nDepth;
	char    c;

	nDepth = 0;

	while (**ppPos == ' ')
		(*ppPos)++;

	if (**ppPos == 0)
		return CP_END;

	pToken = *ppOut;

	for (;;)
	{
		if (**ppPos == 0)
			break;

		c = **ppPos;

		if (c == '%')
		{
			(*ppPos)++;
			c = **ppPos;
			if (c == 0)
				break;
		}
		else if (c == ';' || iscntrl(c))     // the end of one command
		{
			**ppOut = 0;
			(*ppOut)++;

			if (pToken[0] == 0)
			{
				(*ppPos)++;
				return CP_NEWCOMMAND;
			}

			return CP_TOKEN;
		}
		else if (c == '(')
		{
			nDepth++;
			if (nDepth == 1)
			{
				(*ppPos)++;
				continue;
			}
		}
		else if (c == ')')
		{
			nDepth--;
			if (nDepth == 0)
			{
				(*ppPos)++;
				break;
			}
		}
		else if (c == '"')
		{
			if (nDepth)
			{
				(*ppPos)++;
				break;
			}

			nDepth = 1;
			(*ppPos)++;
			continue;
		}
		else if (c == ' ')
		{
			if (!nDepth)
				break;
		}

		**ppOut = c;
		if (*ppOut - pToken >= PARSE_MAXARGLEN)
		{
			**ppOut = 0;
			(*ppOut)--;
		}

		(*ppOut)++;
		(*ppPos)++;
	}

	**ppOut = 0;
	(*ppOut)++;

	return pToken[0] ? CP_TOKEN : CP_END;
}


// ======================================================================= //
//  0x10022e90 (stack entry, 12 bytes) / 0x10022e9c (register entry).
// ======================================================================= //
LTBOOL sub_10022e90(char* pCommand, char** ppNewCommand, void** pArgBuffer,
                    void* pArgs, int32* pnArgs)
{
	char**  ppArgs = (char**)pArgs;
	char*   argBuffer = (char*)pArgBuffer;
	char*   pCur = pCommand;
	char*   pOut = argBuffer;
	int     nType;

	*pnArgs = 0;

	for (;;)
	{
		nType = cp_GetToken(&pCur, &pOut);

		if (nType == CP_END)
			return LTFALSE;

		if (nType == CP_TOKEN)
		{
			ppArgs[*pnArgs] = argBuffer;
			(*pnArgs)++;

			if (*pnArgs >= PARSE_MAXARGS)
				return LTFALSE;
		}
		else if (nType == CP_NEWCOMMAND)
		{
			break;
		}

		argBuffer = pOut;
	}

	*ppNewCommand = pCur;

	return LTTRUE;
}


// ======================================================================= //
//  0x100230d0 (stack entry, 4 bytes) / 0x100230d4 -- ConParse's advance:
//  if there is text left at +0x104, tokenise the next command of it into
//  the argument array (+0x00) and buffer (+0x108), clearing the cursor
//  when the text runs out.  __thiscall, no arguments; FALSE only when
//  there was nothing left to begin with.
// ======================================================================= //
class C10023130
{
public:
	LTBOOL  sub_100230d0();                             // 0x100230d4
	LTBOOL  sub_10023130(const char* pName, uint32 bCaseSensitive,
	                     uint32 nIndex);                 // 0x10023130

	char*   m_pText;                                    // +0x00
	uint8   m_pad_04[0xfc];                             // +0x04
	int32   m_n100;                                     // +0x100
	char*   m_p104;                                     // +0x104
	void*   m_p108;                                     // +0x108
};

LTBOOL C10023130::sub_100230d0()
{
	if (!m_p104)
		return LTFALSE;

	if (sub_10022e90(m_p104, &m_p104, &m_p108, this, &m_n100))
		return LTTRUE;

	m_p104 = LTNULL;
	return LTTRUE;
}


LTBOOL C10023130::sub_10023130(const char* pName, uint32 bCaseSensitive,
                               uint32 nIndex)
{
	LTBOOL  bMatch;

	if (nIndex == 0)
		nIndex = 1;

	while (sub_100230d0())
	{
		if (m_n100 >= (int32)nIndex)
		{
			if (bCaseSensitive)
				bMatch = (strcmp(m_pText, pName) == 0);
			else
				bMatch = (stricmp(m_pText, pName) == 0);

			if (bMatch)
				return LTTRUE;
		}
	}

	return LTFALSE;
}
