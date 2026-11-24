// ----------------------------------------------------------------------- //
//
// MODULE  : m35_settings.cpp
//
// PURPOSE : M-35 (docs/MODERN.md): the launcher's settings in a file, not
//           in the registry. Modern build only.
//
//           The launchers (AVP2Launcher.exe, PrimalHuntLauncher.exe) and
//           the direct starts (AVP2.exe, PrimalHunt.exe) keep their
//           options through CRegMgr32 (src/lithshared/RegMgr32/), under
//           HKLM\SOFTWARE\Monolith Productions\Aliens vs. Predator 2\1.0
//           and HKLM\SOFTWARE\Third Law Interactive\<Primal Hunt>\1.0.
//           Under Wine and Proton the registry belongs to a prefix, and a
//           new prefix loses it. This is CRegMgr32 again: the same class
//           (regmgr32.h), linked in place of RegMgr32.lib. It keeps the
//           values in launcher.ini beside the program, as nolfsrc does
//           (its M-P1-28):
//
//             - one section for each registry key, the key without
//               "SOFTWARE\" (for example [Monolith Productions\Aliens vs.
//               Predator 2\1.0]);
//             - strings in quotes, numbers in decimal, binary values as
//               "hex:" and their bytes.
//
//           Read: the file first. When the file does not have the value,
//           the value of the retail registry key, if there is one (read
//           only), so that an existing installation keeps its options.
//           Else the caller's default. Write: the file only. This class
//           never writes to, creates or deletes a registry key or value.
//
// ----------------------------------------------------------------------- //

#include "..\..\launcher\stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regmgr32.h"

namespace
{
	const char	kFileName[]		= "launcher.ini";
	const char	kMissing[]		= "\x01";	// GetPrivateProfileString's default: no such value
	const char	kHexPrefix[]	= "hex:";

	// launcher.ini beside the running program
	const char* SettingsFile()
	{
		static char s_szFile[MAX_PATH] = "";
		if (!s_szFile[0])
		{
			char szDir[MAX_PATH];
			DWORD n = GetModuleFileName(NULL, szDir, sizeof(szDir));
			char* pSlash = (n && n < sizeof(szDir)) ? strrchr(szDir, '\\') : NULL;
			if (pSlash)
			{
				pSlash[1] = '\0';
				_snprintf(s_szFile, sizeof(s_szFile) - 1, "%s%s", szDir, kFileName);
			}
			else
			{
				strcpy(s_szFile, kFileName);
			}
			s_szFile[sizeof(s_szFile) - 1] = '\0';
		}
		return s_szFile;
	}

	// The section of a registry key path: the path without "SOFTWARE\".
	CString SectionOf(const CString& csKey)
	{
		if (csKey.GetLength() > 9 && csKey.Left(9).CompareNoCase("SOFTWARE\\") == 0)
			return csKey.Mid(9);
		return csKey;
	}

	// TRUE when the file has a section of this name.
	BOOL HasSection(const char* pSection)
	{
		char szKeys[16];
		DWORD n = GetPrivateProfileString(pSection, NULL, "", szKeys, sizeof(szKeys), SettingsFile());
		return n > 0;
	}

	// A value as the file holds it; FALSE when there is none.
	BOOL ReadRaw(const char* pSection, const char* pKey, char* pBuf, DWORD nBufLen)
	{
		GetPrivateProfileString(pSection, pKey, kMissing, pBuf, nBufLen, SettingsFile());
		return strcmp(pBuf, kMissing) != 0;
	}

	BOOL WriteString(const char* pSection, const char* pKey, const char* pValue)
	{
		// in quotes, so that spaces at the start and at the end stay
		CString csQuoted = "\"";
		csQuoted += pValue;
		csQuoted += "\"";
		return WritePrivateProfileString(pSection, pKey, csQuoted, SettingsFile());
	}

	BOOL WriteNumber(const char* pSection, const char* pKey, DWORD nValue)
	{
		char szNum[16];
		sprintf(szNum, "%lu", nValue);
		return WritePrivateProfileString(pSection, pKey, szNum, SettingsFile());
	}

	BOOL WriteBinary(const char* pSection, const char* pKey, const void* pValue, int nLen)
	{
		CString csHex = kHexPrefix;
		for (int i = 0; i < nLen; i++)
		{
			char szByte[4];
			sprintf(szByte, "%02x", ((const unsigned char*)pValue)[i]);
			csHex += szByte;
		}
		return WritePrivateProfileString(pSection, pKey, csHex, SettingsFile());
	}

	// The three kinds of value, from the file. Each answers 1 (the value,
	// of this kind), 0 (the file has the name, but not a value of this
	// kind: the same as a registry value of another type) or -1 (the file
	// does not have the name).
	int FileString(const char* pSection, const char* pKey, char* pValue, DWORD& dwSize)
	{
		char szValue[4096];
		if (!ReadRaw(pSection, pKey, szValue, sizeof(szValue))) return -1;
		if (strncmp(szValue, kHexPrefix, strlen(kHexPrefix)) == 0) return 0;
		DWORD nNeed = strlen(szValue) + 1;
		if (nNeed > dwSize) return 0;	// (the registry: ERROR_MORE_DATA)
		strcpy(pValue, szValue);
		dwSize = nNeed;
		return 1;
	}

	int FileBinary(const char* pSection, const char* pKey, void* pValue, DWORD& dwSize)
	{
		char szValue[4096];
		if (!ReadRaw(pSection, pKey, szValue, sizeof(szValue))) return -1;
		int nPrefix = strlen(kHexPrefix);
		if (strncmp(szValue, kHexPrefix, nPrefix) != 0) return 0;
		DWORD nLen = (DWORD)(strlen(szValue) - nPrefix) / 2;
		if (nLen > dwSize) return 0;
		for (DWORD i = 0; i < nLen; i++)
		{
			char szByte[3] = { szValue[nPrefix + 2 * i], szValue[nPrefix + 2 * i + 1], 0 };
			((unsigned char*)pValue)[i] = (unsigned char)strtoul(szByte, NULL, 16);
		}
		dwSize = nLen;
		return 1;
	}

	int FileNumber(const char* pSection, const char* pKey, DWORD* pdwValue)
	{
		char szValue[64];
		if (!ReadRaw(pSection, pKey, szValue, sizeof(szValue))) return -1;
		if (!szValue[0]) return 0;
		char* pEnd = NULL;
		DWORD nValue = strtoul(szValue, &pEnd, 10);
		if (!pEnd || *pEnd) return 0;
		*pdwValue = nValue;
		return 1;
	}

	// The retail registry key, opened to read only (NULL: none).
	HKEY OpenRetailKey(HKEY hRootKey, const char* pKey)
	{
		HKEY hKey = NULL;
		if (RegOpenKeyEx(hRootKey, pKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
			return NULL;
		return hKey;
	}

	BOOL RegString(HKEY hKey, const char* pField, char* pValue, DWORD& dwSize)
	{
		DWORD dwType;
		if (!hKey || dwSize <= 0) return FALSE;
		return RegQueryValueEx(hKey, pField, 0, &dwType, (unsigned char*)pValue, &dwSize) == ERROR_SUCCESS && dwType == REG_SZ;
	}

	BOOL RegBinary(HKEY hKey, const char* pField, void* pValue, DWORD& dwSize)
	{
		DWORD dwType;
		if (!hKey || dwSize <= 0) return FALSE;
		return RegQueryValueEx(hKey, pField, 0, &dwType, (unsigned char*)pValue, &dwSize) == ERROR_SUCCESS && dwType == REG_BINARY;
	}

	BOOL RegNumber(HKEY hKey, const char* pField, DWORD* pdwValue)
	{
		DWORD dwType, dwVal, dwSize = sizeof(dwVal);
		if (!hKey) return FALSE;
		if (RegQueryValueEx(hKey, pField, 0, &dwType, (unsigned char*)&dwVal, &dwSize) != ERROR_SUCCESS || dwType != REG_DWORD)
			return FALSE;
		*pdwValue = dwVal;
		return TRUE;
	}
}


// ----------------------------------------------------------------------- //
//  The key in use: m_csKey (the full path, as retail), m_hKey (the retail
//  registry key, read only, or NULL)
// ----------------------------------------------------------------------- //

BOOL CRegMgr32::Init()
{
	if (m_bIsInited)
		return FALSE;

	m_hKey = NULL;
	m_bIsInited = TRUE;
	return TRUE;
}

void CRegMgr32::Term()
{
	if (m_bIsInited)
	{
		CloseCurrentKey();
		m_bIsInited = FALSE;
	}
}

BOOL CRegMgr32::CloseCurrentKey()
{
	if (!m_bIsInited)
		return FALSE;

	if (m_hKey)
	{
		RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
	m_csKey.Empty();
	return TRUE;
}

// The key is there when the file has its section, or the registry has it.
BOOL CRegMgr32::OpenKey(HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
					   char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!m_bIsInited)
		return FALSE;

	if (!hRootKey || !szKey1)
		return FALSE;

	CloseCurrentKey();

	m_csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);
	m_hKey = OpenRetailKey(hRootKey, m_csKey);

	return m_hKey != NULL || HasSection(SectionOf(m_csKey));
}

// As retail: FALSE when the key is there already. Only the file's section
// is made (the first value written makes it).
BOOL CRegMgr32::CreateKey(HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
					   char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!OpenKey(hRootKey, szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9))
		return !m_csKey.IsEmpty();

	CloseCurrentKey();
	return FALSE;
}

// Removes the file's section; the registry stays as it is.
BOOL CRegMgr32::DeleteKey(HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
					   char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!m_bIsInited || !hRootKey || !szKey1)
		return FALSE;

	CloseCurrentKey();

	CString csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);
	return WritePrivateProfileString(SectionOf(csKey), NULL, NULL, SettingsFile());
}


// ----------------------------------------------------------------------- //
//  The values of the key in use
// ----------------------------------------------------------------------- //

BOOL CRegMgr32::SetField(char *sField, char *sValue)
{
	if (!m_bIsInited || m_csKey.IsEmpty() || !sField || !sValue)
		return FALSE;
	return WriteString(SectionOf(m_csKey), sField, sValue);
}

BOOL CRegMgr32::SetField(char *sField, void *pValue, int nLen)
{
	if (!m_bIsInited || m_csKey.IsEmpty() || !sField || !pValue || nLen < 0)
		return FALSE;
	return WriteBinary(SectionOf(m_csKey), sField, pValue, nLen);
}

BOOL CRegMgr32::SetField(char *sField, DWORD dwValue)
{
	if (!m_bIsInited || m_csKey.IsEmpty() || !sField)
		return FALSE;
	return WriteNumber(SectionOf(m_csKey), sField, dwValue);
}

BOOL CRegMgr32::GetField(char *sField, char *sValue, DWORD &dwSize)
{
	if (!m_bIsInited || m_csKey.IsEmpty() || !sField || dwSize <= 0)
		return FALSE;

	int nFile = FileString(SectionOf(m_csKey), sField, sValue, dwSize);
	if (nFile >= 0)
		return nFile;
	return RegString(m_hKey, sField, sValue, dwSize);
}

BOOL CRegMgr32::GetField(char *sField, void *pValue, DWORD &dwSize)
{
	if (!m_bIsInited || m_csKey.IsEmpty() || !sField || dwSize <= 0)
		return FALSE;

	int nFile = FileBinary(SectionOf(m_csKey), sField, pValue, dwSize);
	if (nFile >= 0)
		return nFile;
	return RegBinary(m_hKey, sField, pValue, dwSize);
}

BOOL CRegMgr32::GetField(char *sField, DWORD *pdwValue)
{
	if (!m_bIsInited || m_csKey.IsEmpty() || !sField || !pdwValue)
		return FALSE;

	int nFile = FileNumber(SectionOf(m_csKey), sField, pdwValue);
	if (nFile >= 0)
		return nFile;
	return RegNumber(m_hKey, sField, pdwValue);
}

// Removes the value from the file. A value in the registry stays, and the
// next read finds it there.
BOOL CRegMgr32::DeleteField(char *sField)
{
	if (!m_bIsInited || m_csKey.IsEmpty() || !sField)
		return FALSE;
	return WritePrivateProfileString(SectionOf(m_csKey), sField, NULL, SettingsFile());
}


// ----------------------------------------------------------------------- //
//  The values of a key named in the call
// ----------------------------------------------------------------------- //

BOOL CRegMgr32::SetField(char *sField, char *sValue, HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
				char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!sField || !sValue || !szKey1) return FALSE;
	CString csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);
	return WriteString(SectionOf(csKey), sField, sValue);
}

BOOL CRegMgr32::SetField(char *sField, void *pValue, int nLen, HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
				char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!sField || !pValue || nLen < 0 || !szKey1) return FALSE;
	CString csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);
	return WriteBinary(SectionOf(csKey), sField, pValue, nLen);
}

BOOL CRegMgr32::SetField(char *sField, DWORD dwValue, HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
				char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!sField || !szKey1) return FALSE;
	CString csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);
	return WriteNumber(SectionOf(csKey), sField, dwValue);
}

BOOL CRegMgr32::GetField(char *sField, char *sValue, DWORD &dwSize, HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
				char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!sField || !sValue || dwSize <= 0 || !szKey1) return FALSE;
	CString csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);

	int nFile = FileString(SectionOf(csKey), sField, sValue, dwSize);
	if (nFile >= 0) return nFile;

	HKEY hKey = OpenRetailKey(hRootKey, csKey);
	BOOL bOK = RegString(hKey, sField, sValue, dwSize);
	if (hKey) RegCloseKey(hKey);
	return bOK;
}

BOOL CRegMgr32::GetField(char *sField, void *pValue, DWORD &dwSize, HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
				char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!sField || !pValue || dwSize <= 0 || !szKey1) return FALSE;
	CString csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);

	int nFile = FileBinary(SectionOf(csKey), sField, pValue, dwSize);
	if (nFile >= 0) return nFile;

	HKEY hKey = OpenRetailKey(hRootKey, csKey);
	BOOL bOK = RegBinary(hKey, sField, pValue, dwSize);
	if (hKey) RegCloseKey(hKey);
	return bOK;
}

BOOL CRegMgr32::GetField(char *sField, DWORD *pdwValue, HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
				char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!sField || !pdwValue || !szKey1) return FALSE;
	CString csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);

	int nFile = FileNumber(SectionOf(csKey), sField, pdwValue);
	if (nFile >= 0) return nFile;

	HKEY hKey = OpenRetailKey(hRootKey, csKey);
	BOOL bOK = RegNumber(hKey, sField, pdwValue);
	if (hKey) RegCloseKey(hKey);
	return bOK;
}

BOOL CRegMgr32::DeleteField(char *sField, HKEY hRootKey, char *szKey1, char *szKey2, char *szKey3, char *szKey4,
				char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	if (!sField || !szKey1) return FALSE;
	CString csKey = CombineKeys(szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9);
	return WritePrivateProfileString(SectionOf(csKey), sField, NULL, SettingsFile());
}


// ----------------------------------------------------------------------- //
//  As retail: the key strings joined with '\'
// ----------------------------------------------------------------------- //

CString CRegMgr32::CombineKeys(char *szKey1, char *szKey2, char *szKey3, char *szKey4,
					   char *szKey5, char *szKey6, char *szKey7, char *szKey8, char *szKey9)
{
	char* apKeys[9] = { szKey1, szKey2, szKey3, szKey4, szKey5, szKey6, szKey7, szKey8, szKey9 };
	CString buf;
	for (int i = 0; i < 9 && apKeys[i]; i++)
	{
		if (i) buf += '\\';
		buf += apKeys[i];
	}
	return buf;
}
