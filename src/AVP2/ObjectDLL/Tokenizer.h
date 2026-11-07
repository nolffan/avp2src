// ----------------------------------------------------------------------- //
//
// MODULE  : Tokenizer.h
//
// PURPOSE : CTokenizer - splits a string into up to 32 tokens: separated
//           by white space, grouped by quotes, and with each "special"
//           character a token of its own (names ours; no RTTI)
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x101586d0..0x10158a20). Third Law's AI parses
//           its commands with it (PHObject.lto 0x101454b5).
//
// ----------------------------------------------------------------------- //

#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#ifdef PRIMAL_HUNT

#define TOKENIZER_MAX_TOKENS		32
#define TOKENIZER_MAX_TOKEN_LEN		32
#define TOKENIZER_MAX_CHARS			16

// Parse's result
#define TOKENIZER_OK				0
#define TOKENIZER_TRUNCATED			2

class CTokenizer
{
	public :

		CTokenizer();

		void	SetDefaults();
		uint32	Parse(const char* szString, const char* szSpecial);

		int			GetNumTokens() const		{ return m_nNumTokens; }
		const char*	GetToken(int nToken) const	{ return m_szTokens[nToken]; }

	protected :

		LTBOOL	IsWhiteSpace(char c);
		LTBOOL	IsQuote(char c);
		LTBOOL	IsSpecial(char c);
		void	AddToken(const char* & pStart, const char* pEnd);

	public :

		int		m_nNumTokens;										// 0x0
		char	m_szTokens[TOKENIZER_MAX_TOKENS][TOKENIZER_MAX_TOKEN_LEN];	// 0x4
		int		m_nCurToken;										// 0x404
		uint32	m_nStatus;											// 0x408
		char	m_szWhiteSpace[TOKENIZER_MAX_CHARS];				// 0x40c
		char	m_szQuotes[TOKENIZER_MAX_CHARS];					// 0x41c
		char	m_szSpecial[TOKENIZER_MAX_CHARS];					// 0x42c
};

#endif // PRIMAL_HUNT

#endif // __TOKENIZER_H__
