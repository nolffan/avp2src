// *********************************************************************** //
//
//  MODULE:   CONPARSE (.CPP) -- Primal Hunt's d3d.ren, TU13
//            (0x10013c30..0x10013f96, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The console-string parser (ConParse, ltbasedefs.h) as the
//            renderer links its own copy, with two string helpers in
//            front of it.  The older LithTech form: parenthesised and
//            quoted phrases are read by their own recursive functions.
//
// *********************************************************************** //

#include <ctype.h>
#include <string.h>
#include "ltbasedefs.h"


#define GNT_NOTOKEN         0
#define GNT_GOTTOKEN        1
#define GNT_GOTSEMICOLON    2

// Add one character to the token, truncating it at PARSE_MAXARGLEN.
#define CP_ADDCHAR(c) \
	{ \
		**ppTokenPos = (c); \
		if ((*ppTokenPos - pToken) >= PARSE_MAXARGLEN) \
		{ \
			**ppTokenPos = 0; \
			--(*ppTokenPos); \
		} \
		++(*ppTokenPos); \
	}


int cp_Parse(char* pCommand, char** pNewCommandPos, char* argBuffer, char** argPointers, int* nArgs);
static int cp_GetNewToken(char** ppCurPos, char** ppTokenPos);
static int cp_ParseParenPhrase(char** ppCurPos, char** ppTokenPos, char* pToken);
static int cp_ParseQuotePhrase(char** ppCurPos, char** ppTokenPos, char* pToken);


// 0x10013c30 -- upper-case a string in place.
void MyStrUpr(char* pStr)
{
	uint32 i;

	for (i = 0; i < strlen(pStr); i++)
		pStr[i] = (char)toupper(pStr[i]);
}

// 0x10013c60 -- the number of bits set.
uint32 CountBits(uint32 val)
{
	uint32 i, count;

	count = 0;
	for (i = 0; i < 32; i++)
	{
		if (val & 1)
			count++;

		val >>= 1;
	}

	return count;
}


// 0x10013c75
LTBOOL ConParse::Parse()
{
	if (!m_pCommandPos)
		return LTFALSE;

	if (!cp_Parse(m_pCommandPos, &m_pCommandPos, m_ArgBuffer, m_Args, &m_nArgs))
	{
		m_pCommandPos = LTNULL;
	}

	return LTTRUE;
}

// 0x10013cac
LTBOOL ConParse::ParseFind(char* pLookFor, LTBOOL bCaseSensitive, uint32 minTokens)
{
	LTBOOL equal;

	// Must have at least one token, otherwise it can't find anything.
	if (minTokens == 0)
		minTokens = 1;

	while (Parse())
	{
		if (m_nArgs >= (int)minTokens)
		{
			if (bCaseSensitive)
				equal = (strcmp(m_Args[0], pLookFor) == 0);
			else
				equal = (stricmp(m_Args[0], pLookFor) == 0);

			if (equal)
			{
				return LTTRUE;
			}
		}
	}

	return LTFALSE;
}


// 0x10013cfe
int cp_Parse(char* pCommand, char** pNewCommandPos, char* argBuffer, char** argPointers, int* nArgs)
{
	int status;
	char *pCurArgBufferPos, *pToken;
	char* pCurPos;

	pCurPos = pCommand;
	pCurArgBufferPos = argBuffer;
	*nArgs = 0;

	while (1)
	{
		pToken = pCurArgBufferPos;
		status = cp_GetNewToken(&pCurPos, &pCurArgBufferPos);

		if (status == GNT_NOTOKEN)
		{
			// All done..
			return 0;
		}
		else if (status == GNT_GOTTOKEN)
		{
			argPointers[*nArgs] = pToken;

			++(*nArgs);
			if (*nArgs >= PARSE_MAXARGS)
				break;
		}
		else if (status == GNT_GOTSEMICOLON)
		{
			// Got a semicolon.. finish up and tell them that there are more.
			*pNewCommandPos = pCurPos;
			return 1;
		}
	}

	return 0;
}


// 0x10013d5a
static int cp_GetNewToken(char** ppCurPos, char** ppTokenPos)
{
	char* pToken;
	char curChar;

	// Skip spaces.
	while (**ppCurPos == ' ')
		(*ppCurPos)++;

	// Is there even a string?
	if (**ppCurPos == 0)
		return GNT_NOTOKEN;

	pToken = *ppTokenPos;
	while (1)
	{
		curChar = **ppCurPos;

		if (curChar == 0)
		{
			break;
		}
		else if (curChar == '%')
		{
			// Just add the next character to the string.
			(*ppCurPos)++;
			if (**ppCurPos == 0)
				break;

			**ppTokenPos = **ppCurPos;
			goto ADDED_CHAR;
		}
		else if (curChar == ';' || iscntrl(curChar))
		{
			**ppTokenPos = 0;
			++(*ppTokenPos);
			if (pToken[0] == 0)
			{
				++(*ppCurPos);
				return GNT_GOTSEMICOLON;
			}
			else
			{
				return GNT_GOTTOKEN;
			}
		}
		else if (curChar == '(')
		{
			if (*ppTokenPos == pToken)
			{
				(*ppCurPos)++;
				cp_ParseParenPhrase(ppCurPos, ppTokenPos, pToken);
			}
			break;
		}
		else if (curChar == '\"')
		{
			if (*ppTokenPos == pToken)
			{
				(*ppCurPos)++;
				cp_ParseQuotePhrase(ppCurPos, ppTokenPos, pToken);
			}
			break;
		}
		else if (curChar == ' ')
		{
			break;
		}

		**ppTokenPos = curChar;

ADDED_CHAR:
		if ((*ppTokenPos - pToken) >= PARSE_MAXARGLEN)
		{
			**ppTokenPos = 0;
			--(*ppTokenPos);
		}

		++(*ppTokenPos);
		(*ppCurPos)++;
	}

	**ppTokenPos = 0;
	++(*ppTokenPos);
	return pToken[0] != 0;
}


// 0x10013e37
static int cp_ParseParenPhrase(char** ppCurPos, char** ppTokenPos, char* pToken)
{
	char curChar;

	while (**ppCurPos != 0 && **ppCurPos != ')')
	{
		curChar = **ppCurPos;
		if (curChar == '%')
		{
			(*ppCurPos)++;
			CP_ADDCHAR(curChar);

			curChar = **ppCurPos;
			if (curChar == 0)
				continue;
		}
		else if (curChar == '(')
		{
			(*ppCurPos)++;
			CP_ADDCHAR(curChar);
			cp_ParseParenPhrase(ppCurPos, ppTokenPos, pToken);
			CP_ADDCHAR(')');
			continue;
		}

		(*ppCurPos)++;
		CP_ADDCHAR(curChar);
	}

	if (**ppCurPos == ')')
	{
		(*ppCurPos)++;
		if (*ppTokenPos == pToken)
			CP_ADDCHAR(' ');

		return 1;
	}

	return 0;
}


// 0x10013f00
static int cp_ParseQuotePhrase(char** ppCurPos, char** ppTokenPos, char* pToken)
{
	while (**ppCurPos != 0 && **ppCurPos != '\"')
	{
		if (**ppCurPos == '%')
		{
			(*ppCurPos)++;
			CP_ADDCHAR('%');

			if (**ppCurPos == 0)
				continue;
		}

		CP_ADDCHAR(*(*ppCurPos)++);
	}

	if (**ppCurPos == '\"')
	{
		(*ppCurPos)++;
		if (*ppTokenPos == pToken)
			CP_ADDCHAR(' ');

		return 1;
	}

	return 0;
}
