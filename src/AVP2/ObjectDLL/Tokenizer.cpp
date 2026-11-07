// ----------------------------------------------------------------------- //
//
// MODULE  : Tokenizer.cpp
//
// PURPOSE : CTokenizer - implementation
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x101586d0..0x10158a20)
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "Tokenizer.h"

static inline void CopyChars(char* szDest, const char* szSrc)
{
	if (szSrc && szSrc[0])
	{
		strncpy(szDest, szSrc, TOKENIZER_MAX_CHARS - 1);
		szDest[TOKENIZER_MAX_CHARS - 1] = '\0';
	}
	else
	{
		szDest[0] = '\0';
	}
}

// PH: 0x101586d0
CTokenizer::CTokenizer()
{
	m_szWhiteSpace[0] = '\0';
	m_szQuotes[0] = '\0';
	m_szSpecial[0] = '\0';
	m_nNumTokens = 0;

	SetDefaults();
}

// PH: 0x10158700
void CTokenizer::SetDefaults()
{
	CopyChars(m_szWhiteSpace, " \t\n");
	CopyChars(m_szQuotes, "\"");
	CopyChars(m_szSpecial, "=");
}

// PH: 0x101587a0
LTBOOL CTokenizer::IsWhiteSpace(char c)
{
	for (const char* p = m_szWhiteSpace; *p; p++)
	{
		if (c == *p) return LTTRUE;
	}

	return LTFALSE;
}

// PH: 0x101587d0
LTBOOL CTokenizer::IsQuote(char c)
{
	for (const char* p = m_szQuotes; *p; p++)
	{
		if (c == *p) return LTTRUE;
	}

	return LTFALSE;
}

// PH: 0x10158800
LTBOOL CTokenizer::IsSpecial(char c)
{
	for (const char* p = m_szSpecial; *p; p++)
	{
		if (c == *p) return LTTRUE;
	}

	return LTFALSE;
}

// PH: 0x10158830
void CTokenizer::AddToken(const char* & pStart, const char* pEnd)
{
	uint32 nLen = pEnd - pStart;
	if (nLen > TOKENIZER_MAX_TOKEN_LEN - 1)
	{
		m_nStatus = TOKENIZER_TRUNCATED;
		nLen = TOKENIZER_MAX_TOKEN_LEN - 1;
	}

	strncpy(m_szTokens[m_nNumTokens], pStart, nLen);
	m_szTokens[m_nNumTokens][nLen] = '\0';
	m_nNumTokens++;

	pStart = LTNULL;
}

// PH: 0x10158890
uint32 CTokenizer::Parse(const char* szString, const char* szSpecial)
{
	m_nStatus = TOKENIZER_OK;

	if (!szSpecial)
	{
		CopyChars(m_szSpecial, "");
	}
	else
	{
		CopyChars(m_szSpecial, szSpecial);
	}

	LTBOOL bSpecial = strlen(m_szSpecial) > 0;
	m_nNumTokens = 0;

	const char* pStart = LTNULL;
	LTBOOL bInQuotes = LTFALSE;

	const char* p;
	for (p = szString; *p; p++)
	{
		if (m_nNumTokens >= TOKENIZER_MAX_TOKENS) break;

		char c = *p;

		if (!bInQuotes)
		{
			if (IsWhiteSpace(c) == LTTRUE)
			{
				if (pStart)
				{
					AddToken(pStart, p);
				}
				continue;
			}

			if (bSpecial == LTTRUE && IsSpecial(c) == LTTRUE)
			{
				if (pStart)
				{
					AddToken(pStart, p);
				}

				m_szTokens[m_nNumTokens][0] = c;
				m_szTokens[m_nNumTokens][1] = '\0';
				m_nNumTokens++;
				continue;
			}
		}

		if (IsQuote(c) == LTTRUE)
		{
			if (bInQuotes == LTTRUE)
			{
				AddToken(pStart, p);
				bInQuotes = LTFALSE;
			}
			else
			{
				if (pStart)
				{
					AddToken(pStart, p);
				}

				pStart = p + 1;
				bInQuotes = LTTRUE;
			}
		}
		else if (!pStart)
		{
			pStart = p;
		}
	}

	if (pStart)
	{
		AddToken(pStart, p);
	}

	return m_nStatus;
}

#endif // PRIMAL_HUNT
