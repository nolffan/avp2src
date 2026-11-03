//*********************************************************************************
//*********************************************************************************
// Project:		Lith Tech Font Manager
// Purpose:		A general font method for bitmap and system fonts
//*********************************************************************************
// File:		LithFont.cpp
//*********************************************************************************
//*********************************************************************************

#include "lithfont.h"

//*********************************************************************************

LithFont::LithFont()
{
	m_pClient = LTNULL;
	memset(m_szRefName, 0, LTF_REF_NAME_SIZE);
	m_nHeight = 0;
}

//*********************************************************************************

LithFont::~LithFont()
{
	Free();
}

//*********************************************************************************

LTBOOL LithFont::Init(ILTClient *pClient, LITHFONTCREATESTRUCT *lfCS)
{
	if (!pClient || !lfCS)
		return LTFALSE;

	// Setup the reference name for this font
	if (lfCS->szRefName)
		strcpy(m_szRefName, lfCS->szRefName);
	else if (lfCS->szFontBitmap)
		GetFilename(lfCS->szFontBitmap, m_szRefName);
	else
		strcpy(m_szRefName, lfCS->szFontName);

	strupr(m_szRefName);

	m_pClient = pClient;

	return LTTRUE;
}

//*********************************************************************************

void LithFont::Free()
{
	m_pClient = LTNULL;
}

//*********************************************************************************

void LithFont::Draw(HSURFACE pDest, HSTRING szString, LITHFONTDRAWDATA *lfDD, int nX, int nY, LITHFONTSAVEDATA *lfSD)
{
	if (!szString)
		return;

	Draw(pDest, m_pClient->GetStringData(szString), lfDD, nX, nY, lfSD);
}

//*********************************************************************************

void LithFont::GetTextExtents(HSTRING szString, LITHFONTDRAWDATA *lfDD, uint32 &x, uint32 &y, int index)
{
	if (!szString)
		return;

	GetTextExtents(m_pClient->GetStringData(szString), lfDD, x, y, index);
}

//*********************************************************************************

void LithFont::GetPositionFromIndex(HSTRING szString, LITHFONTDRAWDATA *lfDD, int index, uint32 &x, uint32 &y)
{
	if (!szString)
		return;

	GetPositionFromIndex(m_pClient->GetStringData(szString), lfDD, index, x, y);
}

//*********************************************************************************

void LithFont::GetFilename(char *szFullPath, char *szName)
{
	// Find the start of the filename
	char *pStart = szFullPath;
	char *pTemp = strstr(szFullPath, "\\");

	while (pTemp)
	{
		pTemp++;
		pStart = pTemp;
		pTemp = strstr(pTemp, "\\");
	}

	// Copy the filename up to the extension
	int i = 0;
	while (pStart[i] != 0)
	{
		if (pStart[i] == '.')
			break;

		szName[i] = pStart[i];
		i++;
	}
}
