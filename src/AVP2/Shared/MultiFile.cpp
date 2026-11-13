// ----------------------------------------------------------------------- //
//
// MODULE  : MultiFile.cpp
//
// PURPOSE : CMultiFile - Primal Hunt's file interface (MultiFile.h).
//
//           Reconstructed from PHObject.lto (0x10154960..0x101552d0): the
//           base class's pointer overloads, CMultiFile_FILE (stdio, what
//           the bute managers' WriteCache writes) and CMultiFile_LITH (the
//           engine's ILTStream, what ReadCache reads). The linker folded
//           the functions of the same code: CMultiFile_LITH's WriteDWord,
//           WriteInt, WriteFloat and WriteBool are one function, as are its
//           ReadDWord, ReadInt and ReadBool, and the two classes' WriteVector
//           and IsOpen.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"
#include "MultiFileImpl.h"

#ifdef PRIMAL_HUNT

#ifdef _CLIENTBUILD
	#define MULTIFILE_INTERFACE		g_pLTClient
#else
	#define MULTIFILE_INTERFACE		g_pLTServer
#endif

// ----------------------------------------------------------------------- //
//
//	CMultiFile
//
// ----------------------------------------------------------------------- //

CMultiFile::CMultiFile()
{
}

CMultiFile::~CMultiFile()
{
}

LTBOOL CMultiFile::ReadDWord(uint32* pVal)
{
	if(!IsOpen()) return LTFALSE;

	*pVal = ReadDWord();
	return LTTRUE;
}

LTBOOL CMultiFile::ReadInt(int* pVal)
{
	if(!IsOpen()) return LTFALSE;

	*pVal = ReadInt();
	return LTTRUE;
}

LTBOOL CMultiFile::ReadFloat(LTFLOAT* pVal)
{
	if(!IsOpen()) return LTFALSE;

	*pVal = ReadFloat();
	return LTTRUE;
}

LTBOOL CMultiFile::ReadByte(uint8* pVal)
{
	if(!IsOpen()) return LTFALSE;

	*pVal = ReadByte();
	return LTTRUE;
}

LTBOOL CMultiFile::ReadBool(LTBOOL* pVal)
{
	if(!IsOpen()) return LTFALSE;

	*pVal = ReadBool();
	return LTTRUE;
}

LTVector CMultiFile::ReadVector()
{
	if(!IsOpen()) return LTVector(0.0f, 0.0f, 0.0f);

	LTFLOAT x = ReadFloat();
	LTFLOAT y = ReadFloat();
	LTFLOAT z = ReadFloat();

	return LTVector(x, y, z);
}

LTBOOL CMultiFile::ReadVector(LTVector* pVal)
{
	if(!IsOpen()) return LTFALSE;

	pVal->x = ReadFloat();
	pVal->y = ReadFloat();
	pVal->z = ReadFloat();
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	CMultiFile_FILE
//
// ----------------------------------------------------------------------- //

CMultiFile_FILE::CMultiFile_FILE()
{
	m_pFile = LTNULL;
}

CMultiFile_FILE::~CMultiFile_FILE()
{
	Close();
}

LTBOOL CMultiFile_FILE::Open(const char* szFile, int nMode)
{
	Close();

	if(nMode == MULTIFILE_READ)
		m_pFile = fopen(szFile, "rb");
	else if(nMode == MULTIFILE_WRITE)
		m_pFile = fopen(szFile, "wb");
	else
		return LTFALSE;

	return (m_pFile != LTNULL);
}

LTBOOL CMultiFile_FILE::Close()
{
	if(!m_pFile) return LTFALSE;

	if(fclose(m_pFile)) return LTFALSE;

	m_pFile = LTNULL;
	return LTTRUE;
}

void CMultiFile_FILE::WriteDWord(uint32 dwVal)
{
	if(m_pFile) fwrite(&dwVal, sizeof(dwVal), 1, m_pFile);
}

void CMultiFile_FILE::WriteInt(int nVal)
{
	if(m_pFile) fwrite(&nVal, sizeof(nVal), 1, m_pFile);
}

void CMultiFile_FILE::WriteString(const char* szVal)
{
	if(m_pFile)
	{
		uint16 nLen = strlen(szVal);
		fwrite(&nLen, sizeof(nLen), 1, m_pFile);
		fwrite(szVal, nLen, 1, m_pFile);
	}
}

void CMultiFile_FILE::WriteFloat(LTFLOAT fVal)
{
	if(m_pFile) fwrite(&fVal, sizeof(fVal), 1, m_pFile);
}

void CMultiFile_FILE::WriteByte(uint8 nVal)
{
	if(m_pFile) fwrite(&nVal, sizeof(nVal), 1, m_pFile);
}

void CMultiFile_FILE::WriteBool(LTBOOL bVal)
{
	if(m_pFile) fwrite(&bVal, sizeof(bVal), 1, m_pFile);
}

void CMultiFile_FILE::WriteRaw(const void* pData, uint32 nSize)
{
	if(m_pFile) fwrite(pData, nSize, 1, m_pFile);
}

void CMultiFile_FILE::WriteVector(const LTVector & vVal)
{
	if(m_pFile)
	{
		WriteFloat(vVal.x);
		WriteFloat(vVal.y);
		WriteFloat(vVal.z);
	}
}

uint32 CMultiFile_FILE::ReadDWord()
{
	if(!m_pFile) return 0;

	uint32 dwVal;
	fread(&dwVal, sizeof(dwVal), 1, m_pFile);
	return dwVal;
}

int CMultiFile_FILE::ReadInt()
{
	if(!m_pFile) return 0;

	int nVal;
	fread(&nVal, sizeof(nVal), 1, m_pFile);
	return nVal;
}

// RETAIL BUG: a string longer than the buffer is cut short, but the rest of
// it is not read past, so everything read after it is read out of place
// (here and in CMultiFile_LITH::ReadString)
void CMultiFile_FILE::ReadString(char* szVal, uint16 nMaxLen)
{
	if(!m_pFile || !nMaxLen) return;

	uint16 nLen;
	fread(&nLen, sizeof(nLen), 1, m_pFile);
#ifdef AVP2SRC_MODERN
	uint16 nFullLen = nLen;	// M-B79 (RETAIL_BUGS.md)
#endif

	if(nLen > nMaxLen - 1)
		nLen = nMaxLen - 1;

	fread(szVal, nLen, 1, m_pFile);
	szVal[nLen] = 0;
#ifdef AVP2SRC_MODERN
	// M-B79: and the rest of a string too long for the buffer passed over
	if(nFullLen > nLen)
		fseek(m_pFile, nFullLen - nLen, SEEK_CUR);
#endif
}

LTFLOAT CMultiFile_FILE::ReadFloat()
{
	if(!m_pFile) return 0.0f;

	LTFLOAT fVal;
	fread(&fVal, sizeof(fVal), 1, m_pFile);
	return fVal;
}

uint8 CMultiFile_FILE::ReadByte()
{
	if(!m_pFile) return 0;

	uint8 nVal;
	fread(&nVal, sizeof(nVal), 1, m_pFile);
	return nVal;
}

LTBOOL CMultiFile_FILE::ReadBool()
{
	if(!m_pFile) return LTFALSE;

	LTBOOL bVal;
	fread(&bVal, sizeof(bVal), 1, m_pFile);
	return bVal;
}

LTBOOL CMultiFile_FILE::ReadRaw(void* pData, uint32 nSize)
{
	if(!m_pFile) return LTFALSE;

	return (fread(pData, nSize, 1, m_pFile) == 1);
}

LTBOOL CMultiFile_FILE::IsEOF()
{
	if(!m_pFile) return LTFALSE;

	return feof(m_pFile) ? LTTRUE : LTFALSE;
}

void CMultiFile_FILE::Seek(uint32 nOffset, int nOrigin)
{
	if(!m_pFile) return;

	if(nOrigin == MULTIFILE_SEEK_CUR)
		fseek(m_pFile, nOffset, SEEK_CUR);
	else if(nOrigin == MULTIFILE_SEEK_SET)
		fseek(m_pFile, nOffset, SEEK_SET);
	else if(nOrigin == MULTIFILE_SEEK_END)
		fseek(m_pFile, nOffset, SEEK_END);
}

uint32 CMultiFile_FILE::GetPos()
{
	if(!m_pFile) return 0;

	return ftell(m_pFile);
}

uint32 CMultiFile_FILE::GetLen()
{
	if(!m_pFile) return 0;

	uint32 nPos = GetPos();
	Seek(0, MULTIFILE_SEEK_END);
	uint32 nLen = GetPos();
	Seek(nPos, MULTIFILE_SEEK_SET);

	return nLen;
}

LTBOOL CMultiFile_FILE::IsOpen()
{
	return (m_pFile != LTNULL);
}

// ----------------------------------------------------------------------- //
//
//	CMultiFile_LITH
//
// ----------------------------------------------------------------------- //

CMultiFile_LITH::CMultiFile_LITH()
{
	m_pStream = LTNULL;
}

LTBOOL CMultiFile_LITH::Close()
{
	if(m_pStream)
		m_pStream->Release();

	m_pStream = LTNULL;
	return LTTRUE;
}

CMultiFile_LITH::~CMultiFile_LITH()
{
	Close();
}

LTBOOL CMultiFile_LITH::Open(const char* szFile, int nMode)
{
	Close();

	if((nMode != MULTIFILE_READ) && (nMode != MULTIFILE_WRITE))
		return LTFALSE;

	if(MULTIFILE_INTERFACE->OpenFile((char*)szFile, &m_pStream) != LT_OK)
		return LTFALSE;

	return (m_pStream != LTNULL);
}


void CMultiFile_LITH::WriteDWord(uint32 dwVal)
{
	if(m_pStream) m_pStream->Write(&dwVal, sizeof(dwVal));
}

void CMultiFile_LITH::WriteInt(int nVal)
{
	if(m_pStream) m_pStream->Write(&nVal, sizeof(nVal));
}

void CMultiFile_LITH::WriteString(const char* szVal)
{
	if(m_pStream)
	{
		uint16 nLen = strlen(szVal);
		m_pStream->Write(&nLen, sizeof(nLen));
		m_pStream->Write(szVal, nLen);
	}
}

void CMultiFile_LITH::WriteFloat(LTFLOAT fVal)
{
	if(m_pStream) m_pStream->Write(&fVal, sizeof(fVal));
}

void CMultiFile_LITH::WriteByte(uint8 nVal)
{
	if(m_pStream) m_pStream->Write(&nVal, sizeof(nVal));
}

void CMultiFile_LITH::WriteBool(LTBOOL bVal)
{
	if(m_pStream) m_pStream->Write(&bVal, sizeof(bVal));
}

void CMultiFile_LITH::WriteRaw(const void* pData, uint32 nSize)
{
	if(m_pStream) m_pStream->Write(pData, nSize);
}

void CMultiFile_LITH::WriteVector(const LTVector & vVal)
{
	if(m_pStream)
	{
		WriteFloat(vVal.x);
		WriteFloat(vVal.y);
		WriteFloat(vVal.z);
	}
}

uint32 CMultiFile_LITH::ReadDWord()
{
	if(!m_pStream) return 0;

	uint32 dwVal;
	m_pStream->Read(&dwVal, sizeof(dwVal));
	return dwVal;
}

int CMultiFile_LITH::ReadInt()
{
	if(!m_pStream) return 0;

	int nVal;
	m_pStream->Read(&nVal, sizeof(nVal));
	return nVal;
}

void CMultiFile_LITH::ReadString(char* szVal, uint16 nMaxLen)
{
	if(!m_pStream || !nMaxLen) return;

	uint16 nLen;
	m_pStream->Read(&nLen, sizeof(nLen));
#ifdef AVP2SRC_MODERN
	uint16 nFullLen = nLen;	// M-B79 (RETAIL_BUGS.md)
#endif

	if(nLen > nMaxLen - 1)
		nLen = nMaxLen - 1;

	m_pStream->Read(szVal, nLen);
	szVal[nLen] = 0;
#ifdef AVP2SRC_MODERN
	// M-B79: and the rest of a string too long for the buffer passed over
	if(nFullLen > nLen)
		Seek(nFullLen - nLen, MULTIFILE_SEEK_CUR);
#endif
}

LTFLOAT CMultiFile_LITH::ReadFloat()
{
	if(!m_pStream) return 0.0f;

	LTFLOAT fVal;
	m_pStream->Read(&fVal, sizeof(fVal));
	return fVal;
}

uint8 CMultiFile_LITH::ReadByte()
{
	if(!m_pStream) return 0;

	uint8 nVal;
	m_pStream->Read(&nVal, sizeof(nVal));
	return nVal;
}

LTBOOL CMultiFile_LITH::ReadBool()
{
	if(!m_pStream) return LTFALSE;

	LTBOOL bVal;
	m_pStream->Read(&bVal, sizeof(bVal));
	return bVal;
}

LTBOOL CMultiFile_LITH::ReadRaw(void* pData, uint32 nSize)
{
	if(!m_pStream) return LTFALSE;

	return (m_pStream->Read(pData, nSize) == LT_OK);
}

LTBOOL CMultiFile_LITH::IsEOF()
{
	if(!m_pStream) return LTFALSE;

	unsigned long nPos, nLen;
	m_pStream->GetPos(&nPos);
	m_pStream->GetLen(&nLen);

	return (nPos >= nLen);
}

void CMultiFile_LITH::Seek(uint32 nOffset, int nOrigin)
{
	if(!m_pStream) return;

	if(nOrigin == MULTIFILE_SEEK_CUR)
	{
		unsigned long nPos;
		m_pStream->GetPos(&nPos);
		m_pStream->SeekTo(nPos + nOffset);
	}
	else if(nOrigin == MULTIFILE_SEEK_SET)
	{
		m_pStream->SeekTo(nOffset);
	}
	else if(nOrigin == MULTIFILE_SEEK_END)
	{
		unsigned long nLen;
		m_pStream->GetLen(&nLen);
		m_pStream->SeekTo(nLen + nOffset);
	}
}

uint32 CMultiFile_LITH::GetPos()
{
	if(!m_pStream) return 0;

	unsigned long nPos;
	m_pStream->GetPos(&nPos);
	return nPos;
}

uint32 CMultiFile_LITH::GetLen()
{
	if(!m_pStream) return 0;

	unsigned long nLen;
	m_pStream->GetLen(&nLen);
	return nLen;
}

LTBOOL CMultiFile_LITH::IsOpen()
{
	return (m_pStream != LTNULL);
}

#endif // PRIMAL_HUNT
