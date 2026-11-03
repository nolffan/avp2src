//------------------------------------------------------------------
//
//	FILE	  : AbstractIO.cpp
//
//	PURPOSE	  : Implements the CAbstractFileIO class.
//
//	CREATED	  : November 12 1996
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

// Includes....
#include "abstractio.h"


CAbstractIO::CAbstractIO()
{
	m_bExceptionsEnabled = TRUE;
}


CAbstractIO::~CAbstractIO()
{
}


void CAbstractIO::EnableExceptions( BOOL bEnable )
{
	m_bExceptionsEnabled = bEnable;
}


BOOL CAbstractIO::IsExceptionsEnabled()
{
	return m_bExceptionsEnabled;
}


BOOL CAbstractIO::WriteString( char *pStr )
{
	WORD	len = (WORD)strlen(pStr);
	
	if( !Write(&len, sizeof(len)) )
		return FALSE;

	return Write( pStr, len );
}


BOOL CAbstractIO::ReadString( char *pStr, DWORD maxLen )
{
	WORD		len;

	if( !Read(&len, sizeof(len)) )
		return FALSE;

	if( (WORD)(len+1) > maxLen )
		return FALSE;

	if( !Read(pStr, len) )
		return FALSE;
	
	pStr[len] = 0;
	return TRUE;
}

BOOL CAbstractIO::ReadTextString(char *pStr, DWORD maxLen)
{
	if (maxLen > 4096) maxLen = 4096;

	memset(pStr, 0, maxLen);

	DWORD i = 0;

	while (i < maxLen)
	{		
		Read(&pStr[i], 1);

		if (pStr[i] == 0x0A)
		{
			if (pStr[i - 1] == 0x0D)
			{
				pStr[i - 1] = 0;
				
				return TRUE;
			}
		}
			
		i ++;
	}

	return TRUE;
}


void CAbstractIO::MaybeThrowIOException( LithIOExceptionType code )
{
#ifndef NO_EXCEPTIONS
	if( m_bExceptionsEnabled )
		throw CLithIOException( code );
#endif
}





