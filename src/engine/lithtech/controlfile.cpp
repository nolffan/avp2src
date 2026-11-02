// *********************************************************************** //
//
//  MODULE:   CONTROLFILEMGR (.CPP)  -- scratch file, NOT a drop-in TU
//
//  WHAT THIS IS: five bodies out of the fourteen enumerated rows for
//            0x00490000..0x004928bf in the retail lithtech.exe.
//
//            NINE of the fourteen rows are NOT new code and carry no code
//            here; each is accounted for in the
//            "ROWS DECLINED" block at the foot of this file. Two are CRT /
//            compiler scaffolding, four are already byte-identical in
//            src/lithshared, three are already-written NEARS in
//            src/lithshared/rezmgr (with a diagnosis for two of them).
//
//  MODULE ID: NOT from the string pool. The identifying instrument was the
//            CALL GRAPH plus a surviving period header:
//
//              * every caller of 0x00491b20 / 0x00491be0 / 0x00491e70 /
//                0x00491ea0 / 0x00491ee0 / 0x00491f20 / 0x00491f60 lies in
//                0x0043fcd0..0x00442700, which is CLTDirectMusicMgr
//                (0x0043fcf2 pushes "CLTDirectMusicMgr::InitLevel ...").
//              * 0x00491be0 (Init) writes six literal characters into the
//                object -- '\n' 0x0a (0x00491cc8), '[' 0x5b, ']' 0x5d,
//                ';' 0x3b, '"' 0x22, '#' 0x23 (0x00491cdc) -- and defaults
//                the delimiter string to " =,\t" (0x004af7b4).
//              * src/LT2/lithshared/incs/controlfilemgr.h, which IS IN THIS
//                REPOSITORY, declares exactly that class: six special
//                characters in that order, a FILE* the file-access
//                virtuals use, two CLithSimpAry<char> strings, a section
//                list, a define list and a BOOL m_bInitialized.
//
//            src/engine/lithtech/clientmusic.h already models the class as
//            `CControlFileMgr` and names 0x00491b20/0x00491b50/0x00491be0/
//            0x00491d10/0x00491e70/0x00491ea0/0x00491f60 and the four
//            virtuals; everything below is consistent with it and with the
//            shipped header's member NAMES, which clientmusic.h did not
//            have.
//
//  NOT SHARED: CControlFileMgr exists in NO other shipped image. The byte
//            pattern of Init's six character stores (`0a c6 4? 11 5b`)
//            occurs once in the retail lithtech.exe (0x00491ccb) and ZERO
//            times in the retail NOLF.exe, the retail server.dll, NolfServ.exe,
//            ltmsg.dll or d3d.ren. THERE IS NO /O1 ORACLE for this file.
//
//  GROUND TRUTH: the retail lithtech.exe, cl 8447, /O2 (/Og /Oi /Ot /Oy
//            /Ob1). Every offset, constant and branch below is cited by the
//            address it was read at.
//
//  Locally-introduced symbols carry the Q490 suffix. Nothing the shipped
//  header names is renamed.
//
// *********************************************************************** //

#include <string.h>          // memcpy, strcpy, strchr, stricmp
#include <stdio.h>           // FILE

#ifndef ASSERT
#define ASSERT(x)
#endif

typedef int BOOL_Q490;
#define TRUE_Q490   1
#define FALSE_Q490  0
#define NULL_Q490   0


// ======================================================================= //
//  lith.h / lithtmpl.h stand-ins
//
//  These three templates are NOT guesses either: src/lithshared/lith holds
//  reconstructed copies already (lithsimpary.h, lithbaselist.h) and
//  src/LT2/lithshared/lith/baselist.h holds the period non-template original.
//  They are repeated here only so this scratch file compiles on its own;
//  when this lands, include the real headers and delete this block.
//
//  WHAT THE SHIPPED BYTES CONFIRM:
//
//   * CLithSimpAry<char> is ONE POINTER. 0x00491e30 -- the folded
//     destructor body Term() calls on every node -- is exactly
//     `if (m_pData) { operator delete(m_pData); m_pData = 0; }` on
//     `this+0x08`, and 0x00492850 is the same body on `this+0x00`, reached
//     only from the EH unwind funclets at 0x004a0a66/0x004a0a71 that
//     destroy CControlFileMgr's two strings at +0x08 and +0x0c. No size
//     member survives the release build, which is why Alloc() has to free
//     unconditionally and Copy() has to be told the count.
//   * Alloc() is Free() then `new Type[n]`: 0x004920d9..0x004920ff reads
//     m_pData, frees it if non-null, NULLs it, allocates, and stores.
//   * CLithBaseList is TWO POINTERS and InsertLast is inlined verbatim at
//     four sites (0x00492102, 0x00492515, 0x004925fe, 0x0049274a), each
//     time as: store NULL to m_pNext, load m_pLast into m_pPrev, RE-LOAD
//     m_pLast and branch on it, then store m_pLast.
// ======================================================================= //

template<class Type>
class CLithSimpAry
{
public:
	CLithSimpAry()                                 { m_pData = NULL_Q490; }
	~CLithSimpAry()                                { Free(); }

	void Copy(const Type* pSource, const unsigned int nSize)
	{
		memcpy(m_pData, pSource, nSize);
	}

	void Alloc(const unsigned int nSize)
	{
		Free();
		m_pData = new Type [nSize];
	}

	void Free()
	{
		if (m_pData != NULL_Q490)
		{
			delete [] m_pData;
			m_pData = NULL_Q490;
		}
	}

	operator Type* ()                              { return m_pData; }

private:
	Type* m_pData;
};


template<class ItemType> class CLithBaseList;

template<class Type>
class CLithBaseListItem
{
public:
	Type*   Next()      { return (Type*)m_pNext; }
	Type*   Prev()      { return (Type*)m_pPrev; }

public:
	friend class CLithBaseList<Type>;

protected:
	Type*   m_pNext;
	Type*   m_pPrev;
};

template<class ItemType>
class CLithBaseList
{
public:
	CLithBaseList()                                { m_pFirst = NULL_Q490; m_pLast = NULL_Q490; }
	~CLithBaseList()                               {}

	inline void Insert(ItemType* pItem)            { InsertFirst(pItem); }
	inline void InsertFirst(ItemType* pItem);
	inline void InsertLast(ItemType* pItem);
	inline void Delete(ItemType* pItem);

	ItemType*   GetFirst()                         { return (ItemType*)m_pFirst; }
	ItemType*   GetLast()                          { return (ItemType*)m_pLast; }

private:
	ItemType*  m_pFirst;
	ItemType*  m_pLast;
};

template<class ItemType>
void CLithBaseList<ItemType>::InsertFirst(ItemType* pItem)
{
	ASSERT(pItem != NULL_Q490);
	pItem->m_pNext = m_pFirst;
	pItem->m_pPrev = NULL_Q490;
	if (m_pFirst != NULL_Q490) m_pFirst->m_pPrev = pItem;
	else m_pLast = pItem;
	m_pFirst = pItem;
}

template<class ItemType>
void CLithBaseList<ItemType>::InsertLast(ItemType* pItem)
{
	ASSERT(pItem != NULL_Q490);
	pItem->m_pNext = NULL_Q490;
	pItem->m_pPrev = m_pLast;
	if (m_pLast != NULL_Q490) m_pLast->m_pNext = pItem;
	else m_pFirst = pItem;
	m_pLast = pItem;
}

template<class ItemType>
void CLithBaseList<ItemType>::Delete(ItemType* pItem)
{
	ASSERT(pItem != NULL_Q490);
	if (pItem->m_pPrev != NULL_Q490) pItem->m_pPrev->m_pNext = pItem->m_pNext;
	else m_pFirst = pItem->m_pNext;
	if (pItem->m_pNext != NULL_Q490) pItem->m_pNext->m_pPrev = pItem->m_pPrev;
	else m_pLast = pItem->m_pPrev;
}


// ======================================================================= //
//  controlfilemgr.h -- transcribed from src/LT2/lithshared/incs/controlfilemgr.h
//
//  LAYOUTS, every offset read off the shipped bytes:
//
//    CControlFileWord     16 bytes  -- `push $0x10` at 0x004926e0
//        +0x00 m_pNext        (0x0049274a stores NULL)
//        +0x04 m_pPrev        (0x00492753)
//        +0x08 m_sVal         (0x00492737)
//        +0x0c m_nStrLen      (0x00492777; read at 0x004919e8, 0x00491a50)
//
//    CControlFileKey      20 bytes  -- `push $0x14` at 0x00492598
//        +0x00/+0x04 links,  +0x08 m_sKeyName (0x004925dd),
//        +0x0c/+0x10 m_lstWords (0x00492750/0x00492774; 0x00491a90 returns
//                                +0x0c bare, i.e. GetFirstWord)
//
//    CControlFileSection  20 bytes  -- `push $0x14` at 0x004920bc/0x004924b9
//        +0x00/+0x04 links,  +0x08 m_sSectionName (0x004920f9),
//        +0x0c/+0x10 m_lstKeys (0x00491ad8 reads +0x0c, 0x00491adf makes
//                               +0x0c the `this` of the list search)
//
//    CControlFileDefine   12 bytes  (+0x08 m_sVal, read at 0x00492056)
//
//    CControlFileMgr      44 bytes
//        +0x00 vptr            (0x00491b36 / 0x00491b6d store 0x004a3c6c)
//        +0x04 m_pFile         (0x004927f0 FileOpen stores fopen's result)
//        +0x08 m_sFileName     (0x00491c16)
//        +0x0c m_sWhiteSpace   (0x00491c5c; strchr's string at 0x00492145)
//        +0x10 m_cEndOfLine    '\n'  (0x00491cc8)
//        +0x11 m_cSectionStart '['   (0x00491ccc)
//        +0x12 m_cSectionEnd   ']'   (0x00491cd0)
//        +0x13 m_cComment      ';'   (0x00491cd4)
//        +0x14 m_cStringMarker '"'   (0x00491cd8)   -- never read by
//                                                      ProcessFile
//        +0x15 m_cSpecialCommand '#' (0x00491cdc)
//        +0x18 m_lstSections   (0x00492115/0x00492118)
//        +0x20 m_lstDefines    (0x00491ff4/0x0049200a)
//        +0x28 m_bInitialized  (0x00491ce7 sets 1, 0x00491e1f clears it)
//
//  The four file-access virtuals take vtable slots 0..3 in the header's
//  order: 0x004920a9 calls *(%eax) with one argument (FileOpen),
//  0x004927d5 calls *0x4(%edx) with none (FileClose), 0x00492130 calls
//  *0x8(%eax) and keeps %al (FileGetChar), 0x00492120 calls *0xc(%edx)
//  and tests the result (FileEOF).
// ======================================================================= //

class CControlFileMgr;

class CControlFileWord : public CLithBaseListItem<CControlFileWord>
{
public:
	BOOL_Q490 GetVal(char* sVal, int nMaxStrLen);        // 0x004919e0  WRITTEN
	int   GetStrLen();
	BOOL_Q490 GetVal(int& nVal);                         // 0x00491a50  written
	BOOL_Q490 GetVal(float& nVal);                       // 0x00491a70  written
	BOOL_Q490 GetVal(double& nVal);
	char* GetVal()                                       { return m_sVal; }
private:
	friend class CControlFileMgr;
	CLithSimpAry<char> m_sVal;
	int m_nStrLen;
};

class CControlFileWordList : public CLithBaseList<CControlFileWord> {};

class CControlFileKey : public CLithBaseListItem<CControlFileKey>
{
public:
	CControlFileWord* GetFirstWord();                    // 0x00491a90
	CControlFileKey*  NextWithSameName();
	char* GetKeyName()                                   { return m_sKeyName; }
private:
	friend class CControlFileKeyList;
	friend class CControlFileMgr;
	CLithSimpAry<char> m_sKeyName;
	CControlFileWordList m_lstWords;
};

class CControlFileKeyList : public CLithBaseList<CControlFileKey>
{
public:
	CControlFileKey* Find(const char* sKeyName);         // 0x00491af0
};

class CControlFileSection : public CLithBaseListItem<CControlFileSection>
{
public:
	CControlFileKey* GetKey(const char* sKeyName = NULL_Q490);  // 0x00491ad0
	char* GetSectionName()                               { return m_sSectionName; }
private:
	friend class CControlFileSectionList;
	friend class CControlFileMgr;
	CLithSimpAry<char> m_sSectionName;
	CControlFileKeyList m_lstKeys;
};

class CControlFileSectionList : public CLithBaseList<CControlFileSection>
{
public:
	CControlFileSection* Find(const char* sSectionName); // 0x00491af0 (folded)
};

class CControlFileDefine : public CLithBaseListItem<CControlFileDefine>
{
public:
	char* GetVal()                                       { return m_sVal; }
private:
	friend class CControlFileMgr;
	CLithSimpAry<char> m_sVal;
};

class CControlFileDefineList : public CLithBaseList<CControlFileDefine> {};

class CControlFileMgr
{
public:
	CControlFileMgr();                                                    // 0x00491b20
	CControlFileMgr(const char* sFileName, const char* sWhiteSpace = NULL_Q490);
	~CControlFileMgr();                                                   // 0x00491b50

	BOOL_Q490 Init(const char* sFileName, const char* sWhiteSpace = NULL_Q490); // 0x00491be0
	void  Term();                                                         // 0x00491d10

	CControlFileSection* GetSection(const char* sSectionName = NULL_Q490); // 0x00491e50
	CControlFileKey*     GetKey(const char* sSectionName, const char* sKeyName); // 0x00491e70

	BOOL_Q490 GetKeyVal(const char* sSectionName, const char* sKeyName, int& nVal);    // 0x00491ea0
	BOOL_Q490 GetKeyVal(const char* sSectionName, const char* sKeyName, float& nVal);  // 0x00491ee0
	BOOL_Q490 GetKeyVal(const char* sSectionName, const char* sKeyName, double& nVal);
	BOOL_Q490 GetKeyVal(const char* sSectionName, const char* sKeyName,
	                    char* sVal, int nMaxStrLen);                      // 0x00491f20  WRITTEN
	int   GetKeyValStrLen(const char* sSectionName, const char* sKeyName);

	BOOL_Q490 AddDefine(const char* sDefine);                             // 0x00491f60
	void  ClearDefines();                                                 // 0x00491ff0  WRITTEN

	virtual BOOL_Q490 FileOpen(const char* sName);                        // 0x004927f0
	virtual void      FileClose();                                        // 0x00492820
	virtual int       FileGetChar();                                      // 0x00492830
	virtual BOOL_Q490 FileEOF();                                          // 0x00492840

protected:
	FILE* m_pFile;

private:
	BOOL_Q490 IsDefined(const char* sDefine);                             // 0x00492040  WRITTEN
	BOOL_Q490 ProcessFile(const char* sFileName);                         // 0x00492080  WRITTEN

	CLithSimpAry<char> m_sFileName;
	CLithSimpAry<char> m_sWhiteSpace;

	char m_cEndOfLine;
	char m_cSectionStart;
	char m_cSectionEnd;
	char m_cComment;
	char m_cStringMarker;
	char m_cSpecialCommand;

	CControlFileSectionList m_lstSections;
	CControlFileDefineList  m_lstDefines;

	BOOL_Q490 m_bInitialized;
};


// ======================================================================= //
//  0x004919e0  CControlFileWord::GetVal(char*, int)        WRITTEN, 100 bytes
//
//  Extent: 0x004919e0..0x00491a44 (100 bytes to 0x00491a44, then one 0x90
//  pad to the 0x00491a50 COMDAT boundary). Two `ret $0x8` and no branch
//  leaves the range.
//
//  Calls (0): both string moves are /Oi intrinsics.
//
//  WHAT PINS IT
//   * `ret $0x8` with `this` out of %ecx -> __thiscall, two stack arguments.
//     0x004919e0 reads 0x8(%esp) BEFORE any push, i.e. ARGUMENT TWO, into
//     the register the compare uses; 0x004919f0 reads 0x0c(%esp) after two
//     pushes, i.e. ARGUMENT ONE, into %edi as the move DESTINATION. So the
//     buffer is first and the length second, exactly as the period header
//     declares it.
//   * `cmp %edx,0xc(%eax); jle` -- SIGNED, so m_nStrLen is `int`, and the
//     member is on the LEFT of the comparison.
//   * The 0x00491a18 arm is VC6's inline `strcpy`: `or $-1,%ecx; xor
//     %eax,%eax; repnz scasb; not %ecx` -- and NO `dec %ecx`, so the count
//     is len+1 and the NUL is copied. `sub %ecx,%edi` then rewinds to the
//     start. The 0x004919ed arm has no scan at all, so it is `memcpy` with
//     an explicit count.
//   * THE TRUNCATION WRITES THROUGH THE MEMBER, NOT THE CALLER'S BUFFER.
//     0x00491a06 RELOADS `mov 0x8(%eax),%eax` with %eax still holding
//     `this` (set at 0x004919e4 and untouched -- `rep movs` writes only
//     %ecx/%esi/%edi), and 0x00491a0b stores the NUL at
//     -0x1(%edx,%eax,1) = m_sVal[nMaxStrLen-1]. This is a bug in the
//     shipped source; it is reproduced because the bytes require it.
//     Spelling it `sVal[nMaxStrLen-1]` would reload 0x0c(%esp), not
//     0x8(%eax).
//   * `mov $0x1,%eax` is five bytes wide in both arms -> int-sized return.
//     See COIN-FLIP 1 for the single-trailing-return reading.
// ======================================================================= //

BOOL_Q490 CControlFileWord::GetVal(char* sVal, int nMaxStrLen)
{
	if (m_nStrLen > nMaxStrLen)
	{
		memcpy(sVal, m_sVal, nMaxStrLen);
		m_sVal[nMaxStrLen - 1] = '\0';
	}
	else
	{
		strcpy(sVal, m_sVal);
	}

	return TRUE_Q490;
}


// ======================================================================= //
//  0x00491a90  CControlFileKey::GetFirstWord()               4 bytes
//
//  `mov 0xc(%ecx),%eax; ret`: the head of m_lstWords (+0x0c).
// ======================================================================= //

CControlFileWord* CControlFileKey::GetFirstWord()
{
	return m_lstWords.GetFirst();
}


// ======================================================================= //
//  0x00491af0  CControlFileKeyList::Find(const char*)       42 bytes
//              (and CControlFileSectionList::Find -- identical bodies,
//              folded by the linker to this one address)
//
//  `ret $0x4`, thiscall; the head is the list's m_pFirst (+0x00), the walk
//  is the rotated `while (p)` (pre-test 0x00491af3, back edge 0x00491b0f),
//  and a zero stricmp leaves the loop with p in %eax.  stricmp(name, arg):
//  `push %edi` (the argument) then `push %eax` (the node's +0x08).
// ======================================================================= //

CControlFileKey* CControlFileKeyList::Find(const char* sKeyName)
{
	CControlFileKey* pKey;

	pKey = GetFirst();
	while (pKey != NULL_Q490)
	{
		if (stricmp(pKey->m_sKeyName, sKeyName) == 0)
			break;

		pKey = pKey->Next();
	}

	return pKey;
}

CControlFileSection* CControlFileSectionList::Find(const char* sSectionName)
{
	CControlFileSection* pSection;

	pSection = GetFirst();
	while (pSection != NULL_Q490)
	{
		if (stricmp(pSection->m_sSectionName, sSectionName) == 0)
			break;

		pSection = pSection->Next();
	}

	return pSection;
}


// ======================================================================= //
//  0x00491b20  CControlFileMgr::CControlFileMgr()           32 bytes
//
//  Six zero stores (the four inlined CLithSimpAry / CLithBaseList member
//  constructors at +0x08, +0x0c, +0x18..+0x24), the vptr (0x00491b36) and
//  m_bInitialized = FALSE (0x00491b3c). m_pFile is NOT initialised.
// ======================================================================= //

CControlFileMgr::CControlFileMgr()
{
	m_bInitialized = FALSE_Q490;
}


// ======================================================================= //
//  0x00491b50  CControlFileMgr::~CControlFileMgr()          130 bytes
//
//  Calls Term (0x00491b7b) then ClearDefines (0x00491b82); the rest is
//  the EH-framed member destruction of m_sWhiteSpace then m_sFileName
//  (CLithSimpAry::Free inlined, state 3 -> 0 -> -1).
// ======================================================================= //

CControlFileMgr::~CControlFileMgr()
{
	Term();
	ClearDefines();
}


// ======================================================================= //
//  0x00491be0  CControlFileMgr::Init(const char*, const char*)   300 bytes
//
//  `ret $0x8`.  m_sFileName gets strlen+1 (0x00491bf8 dec, 0x00491bfb inc)
//  and Copy's memcpy of strlen+1 (a second scan, no `dec`).  With no delimiter string, m_sWhiteSpace gets 6
//  bytes and the literal " =,\t\r" (0x004af7b4, dword + word copy); with
//  one, it gets strlen WITHOUT the +1 (0x00491c86 `dec`, no `inc`) and then
//  a Copy of strlen+1 -- a one-byte overrun in the shipped source, reproduced.
//  Six special characters (0x00491cc8..0x00491cdc), ProcessFile
//  (0x00491ce0), m_bInitialized = TRUE stored AFTER the call but before its
//  result is tested (0x00491ce7), and Term (0x00491cfe) on failure.
// ======================================================================= //

BOOL_Q490 CControlFileMgr::Init(const char* sFileName, const char* sWhiteSpace)
{
	BOOL_Q490 bRet;

	m_sFileName.Alloc(strlen(sFileName) + 1);
	m_sFileName.Copy(sFileName, strlen(sFileName) + 1);

	if (sWhiteSpace == NULL_Q490)
	{
		m_sWhiteSpace.Alloc(6);
		m_sWhiteSpace.Copy(" =,\t\r", 6);
	}
	else
	{
		m_sWhiteSpace.Alloc(strlen(sWhiteSpace));
		m_sWhiteSpace.Copy(sWhiteSpace, strlen(sWhiteSpace) + 1);
	}

	m_cEndOfLine      = '\n';
	m_cSectionStart   = '[';
	m_cSectionEnd     = ']';
	m_cComment        = ';';
	m_cStringMarker   = '"';
	m_cSpecialCommand = '#';

	bRet = ProcessFile(sFileName);
	m_bInitialized = TRUE_Q490;

	if (bRet)
		return TRUE_Q490;

	Term();
	return FALSE_Q490;
}


// ======================================================================= //
//  0x00491d10  CControlFileMgr::Term()                          282 bytes
//
//  Guarded on m_bInitialized (0x00491d18); both strings freed (inline
//  CLithSimpAry::Free); then three nested `while ((p = GetFirst()))`
//  teardowns -- words of each key, keys of each section, sections --
//  each an inline CLithBaseList::Delete, the folded item destructor
//  0x00491e30 and operator delete; m_bInitialized = FALSE (0x00491e1f).
// ======================================================================= //

void CControlFileMgr::Term()
{
	CControlFileSection* pSection;
	CControlFileKey*     pKey;
	CControlFileWord*    pWord;

	if (m_bInitialized)
	{
		m_sFileName.Free();
		m_sWhiteSpace.Free();

		while ((pSection = m_lstSections.GetFirst()) != NULL_Q490)
		{
			while ((pKey = pSection->m_lstKeys.GetFirst()) != NULL_Q490)
			{
				while ((pWord = pKey->m_lstWords.GetFirst()) != NULL_Q490)
				{
					pKey->m_lstWords.Delete(pWord);
					delete pWord;
				}

				pSection->m_lstKeys.Delete(pKey);
				delete pKey;
			}

			m_lstSections.Delete(pSection);
			delete pSection;
		}

		m_bInitialized = FALSE_Q490;
	}
}


// ======================================================================= //
//  0x00491e70  CControlFileMgr::GetKey(sec, key)             34 bytes
//
//  GetSection (0x00491e50) then CControlFileSection::GetKey (0x00491ad0),
//  one shared `xor %eax,%eax; ret $0x8` after the tail call.
// ======================================================================= //

CControlFileKey* CControlFileMgr::GetKey(const char* sSectionName, const char* sKeyName)
{
	CControlFileSection* pSection;

	pSection = GetSection(sSectionName);
	if (pSection != NULL_Q490)
		return pSection->GetKey(sKeyName);

	return NULL_Q490;
}


// ======================================================================= //
//  0x00491ea0  CControlFileMgr::GetKeyVal(sec, key, int&)     50 bytes
//  0x00491ee0  CControlFileMgr::GetKeyVal(sec, key, float&)   50 bytes
//
//  The char* sibling at 0x00491f20 with the third call swapped for
//  0x00491a50 (GetVal(int&)) / 0x00491a70 (GetVal(float&)), `ret $0xc`.
// ======================================================================= //

BOOL_Q490 CControlFileMgr::GetKeyVal(const char* sSectionName, const char* sKeyName, int& nVal)
{
	CControlFileKey*  pKey;
	CControlFileWord* pWord;

	pKey = GetKey(sSectionName, sKeyName);
	if (pKey != NULL_Q490)
	{
		pWord = pKey->GetFirstWord();
		if (pWord != NULL_Q490)
			return pWord->GetVal(nVal);
	}

	return FALSE_Q490;
}

BOOL_Q490 CControlFileMgr::GetKeyVal(const char* sSectionName, const char* sKeyName, float& nVal)
{
	CControlFileKey*  pKey;
	CControlFileWord* pWord;

	pKey = GetKey(sSectionName, sKeyName);
	if (pKey != NULL_Q490)
	{
		pWord = pKey->GetFirstWord();
		if (pWord != NULL_Q490)
			return pWord->GetVal(nVal);
	}

	return FALSE_Q490;
}


// ======================================================================= //
//  0x00491f20  CControlFileMgr::GetKeyVal(sec, key, char*, int)
//                                                          WRITTEN, 55 bytes
//
//  Extent: 0x00491f20..0x00491f57 (55 bytes, then one 0x90 pad).
//
//  Calls, in order (3):
//      0x00491f2a  0x00491e70  CControlFileMgr::GetKey
//      0x00491f35  0x00491a90  CControlFileKey::GetFirstWord
//      0x00491f4a  0x004919e0  CControlFileWord::GetVal(char*, int)
//
//  WHAT PINS IT
//   * `ret $0x10` -> __stdcall-cleaned, four dwords of arguments; the
//     arguments are read at 0x4(%esp), 0x8(%esp), 0xc(%esp) and 0x10(%esp)
//     with no pushes outstanding, so they are (sec, key, sVal, nMaxStrLen).
//   * It is BYTE-FOR-BYTE the 0x00491ea0 (int&) and 0x00491ee0 (float&)
//     siblings with the third call swapped and one more argument, so the
//     source is the same three-line shape.
//   * Both null tests jump FORWARD to one shared `xor %eax,%eax;
//     ret $0x10` at 0x00491f52 that sits past the tail call -- two guarded
//     early returns tail-merged, not an if/else.
//   * The final call is `call` + `ret $0x10`, not a tail `jmp`: VC6 will
//     not tail-jump when the callee has stack arguments to clean.
// ======================================================================= //

BOOL_Q490 CControlFileMgr::GetKeyVal(const char* sSectionName, const char* sKeyName,
                                     char* sVal, int nMaxStrLen)
{
	CControlFileKey*  pKey;
	CControlFileWord* pWord;

	// NESTED, not two early returns. Written as `if (!pKey) return FALSE;`
	// twice, VC6 emits `jne` + `ret` at each site; the shipped bytes branch
	// forward with `je` from BOTH tests onto one shared `xor %eax,%eax ;
	// ret $0x10` tail. Same merge as OnFoundInBox in 11 -- two returns of
	// the same constant are one block in the shipped code.
	pKey = GetKey(sSectionName, sKeyName);
	if (pKey != NULL_Q490)
	{
		pWord = pKey->GetFirstWord();
		if (pWord != NULL_Q490)
			return pWord->GetVal(sVal, nMaxStrLen);
	}

	return FALSE_Q490;
}


// ======================================================================= //
//  0x00491f60  CControlFileMgr::AddDefine(const char*)     134 bytes
//
//  `ret $0x4`, thiscall -- and `this` is never read: %ecx is clobbered by
//  the strlen scan at 0x00491f75 and never reloaded.  The new define
//  (`push $0xc; call operator new`, 0x00491f8e) gets its string --
//  CLithSimpAry::Alloc inlined, Free's dead `if` included (0x00491fa5), then
//  Copy's memcpy of nLen+1 -- and is NOT linked into m_lstDefines.  The
//  shipped function leaks it; reproduced because the bytes require it.
//  Three `return FALSE` arms (null, empty, new failed) share 0x00491f86.
// ======================================================================= //

BOOL_Q490 CControlFileMgr::AddDefine(const char* sDefine)
{
	CControlFileDefine* pDefine;
	int nLen;

	if (sDefine == NULL_Q490)
		return FALSE_Q490;

	nLen = strlen(sDefine);
	if (nLen < 1)
		return FALSE_Q490;

	pDefine = new CControlFileDefine;
	if (pDefine == NULL_Q490)
		return FALSE_Q490;

	pDefine->m_sVal.Alloc(nLen + 1);
	pDefine->m_sVal.Copy(sDefine, nLen + 1);

	return TRUE_Q490;
}


// ======================================================================= //
//  0x00491ff0  CControlFileMgr::ClearDefines()             WRITTEN, 70 bytes
//
//  Extent: 0x00491ff0..0x00492036 (70 bytes, then one 0x90 pad to
//  0x00492040). One `ret`; the only backward branch is its own
//  `jmp 0x00491ff4`.
//
//  Calls, in order (2):
//      0x00492023  0x00491e30  ~CControlFileDefine (the folded
//                              CLithSimpAry<char>::Free body)
//      0x00492029  0x004290c0  operator delete
//
//  WHAT PINS IT
//   * Bare `ret`, `this` from %ecx -> __thiscall, no arguments, void.
//   * 0x00491ffb..0x0049201e is CLithBaseList::Delete transcribed: prev's
//     m_pNext or m_pFirst, then next's m_pPrev or m_pLast, on the list at
//     +0x20/+0x24. It is INLINE because the list is a template.
//   * THE LIST HEAD IS LOADED EXACTLY ONCE, at the loop TOP (0x00491ff4),
//     and the back edge at 0x00492031 is an unconditional `jmp` to it.
//     VC6 rotates a `while (p)` whose condition is a plain load by peeling
//     the load and duplicating the test at the bottom; it cannot do that
//     when the condition CONTAINS THE ASSIGNMENT, because the assignment
//     would have to be duplicated too. So the source is
//     `while ((p = GetFirst()) != NULL)`, not `p = GetFirst(); while (p)
//     { ...; p = GetFirst(); }`.
//   * `call 0x00491e30` then `push; call operator delete` with NO null test
//     -> `delete pDefine` on a pointer the loop condition already proved
//     non-null.
// ======================================================================= //

void CControlFileMgr::ClearDefines()
{
	CControlFileDefine* pDefine;

	while ((pDefine = m_lstDefines.GetFirst()) != NULL_Q490)
	{
		m_lstDefines.Delete(pDefine);
		delete pDefine;
	}
}


// ======================================================================= //
//  0x00492040  CControlFileMgr::IsDefined(const char*)     WRITTEN, 62 bytes
//
//  Extent: 0x00492040..0x0049207e (62 bytes, then two 0x90 pads to
//  0x00492080). Two `ret $0x4`, no branch leaves the range.
//
//  Calls (1):
//      0x0049205b  0x00492f30  stricmp
//
//  WHY stricmp AND NOT strcmp: 0x00492f30 folds each byte with
//  `sub $0x41,%al; cmp $0x1a,%al; sbb %cl,%cl; and $0x20,%cl; add %cl,%al;
//  add $0x41,%al` (0x00492f5a and 0x00492f69) -- the classic MSVC
//  `_stricmp` case fold. The same callee serves 0x00491af0
//  (CControlFileKeyList::Find) and 0x00491aa0.
//
//  WHAT PINS IT
//   * `ret $0x4` -> __thiscall, one argument; `mov $0x1,%eax` five bytes
//     wide -> int-sized return.
//   * The two argument guards (0x00492046 `test %edi,%edi`, 0x0049204a
//     `cmpb $0x0,(%edi)`) and the empty-list pre-test (0x00492054) all
//     jump to ONE `xor %eax,%eax; ret $0x4` at 0x0049206d which sits AFTER
//     the loop -- the shared exit is hosted LAST, which is the `||`
//     signature (a run of separate guarded returns hosts it first).
//   * The walk is the rotated `while (p) { ...; p = p->Next(); }`: a
//     pre-test at 0x00492054 and the back edge's own test at 0x00492069.
//   * `push %edi; push %eax` -> stricmp(pDefine->GetVal(), sDefine), the
//     member FIRST.
// ======================================================================= //

BOOL_Q490 CControlFileMgr::IsDefined(const char* sDefine)
{
	CControlFileDefine* pDefine;

	if (sDefine == NULL_Q490 || sDefine[0] == '\0')
		return FALSE_Q490;

	pDefine = m_lstDefines.GetFirst();
	while (pDefine != NULL_Q490)
	{
		if (stricmp(pDefine->GetVal(), sDefine) == 0)
			return TRUE_Q490;

		pDefine = pDefine->Next();
	}

	return FALSE_Q490;
}


// ======================================================================= //
//  0x00492080  CControlFileMgr::ProcessFile(const char*)
//                                                        WRITTEN, 1,898 bytes
//
//  ***  THE ENUMERATION'S "0x004920ba / 1840 / gap" ROW IS NOT A FUNCTION.  ***
//
//  0x004920ba is the TRUE ARM of the entry guard of the function that
//  starts at 0x00492080: `test %eax,%eax; jne 0x004920ba` at 0x004920ab.
//  The two enumerated rows 0x00492080 (58, "call") and 0x004920ba (1840,
//  "gap") are the head and body of ONE function.
//
//  Extent proof:
//    1. 0x00492080 opens `mov $0x3818,%eax; call 0x00492c60` -- the _chkstk
//       probe for a 14,360-byte frame -- and 0x004927e1 closes with
//       `add $0x3818,%esp`. The allocation and the release are the same
//       number and they are the only two in the range.
//    2. 0x00492080/58 has a branch target outside the range:
//       0x004920ad -> 0x004920ba, so the extent is wrong.
//    3. A sweep of EVERY 0xe8/0xe9 displacement in .text and every aligned and
//       unaligned dword in .text/.rdata/.data/.rsrc/.reloc for a value in
//       0x00492081..0x004927ea. THERE ARE NONE. Nothing outside calls,
//       jumps to, or stores a pointer into the body, so it is one function
//       with one entry, not several.
//    4. The frame accounts for itself EXACTLY: five 0x800-byte char buffers
//       at +0x18, +0x818, +0x1018, +0x1818, +0x2018, one 0x400-entry int
//       array at +0x2818, and 0x18 bytes of scalars below them --
//       0x18 + 5*0x800 + 0x1000 = 0x3818. A row that were really several
//       functions could not share one frame like that.
//    5. One `ret $0x4`, at 0x004927e7, and the body ends there (0x004927ea
//       onward is 0x90 padding to the 0x004927f0 COMDAT).
//
//  FRAME (S = %esp after the _chkstk call; body %esp = S-0x10):
//      S+0x03   char  c                  spilled at 0x00492770, "0x13(%esp)"
//      S+0x04   int   nSkip       = 0    0x0049209d,           "0x14(%esp)"
//      S+0x08   int   nLen               0x0049267a,           "0x18(%esp)"
//      S+0x0c   int   nIfIdx      = -1   0x004920a1,           "0x1c(%esp)"
//      S+0x10   CControlFileKey*     pKey      0x004925c0,   "0x20(%esp)"
//      S+0x14   CControlFileSection* pSection  0x004920dc,   "0x24(%esp)"
//      S+0x18   char  sCommand[2048]     "0x28(%esp)"
//      S+0x818  char  sSectionName[2048] "0x828(%esp)"
//      S+0x1018 char  sKeyName[2048]     "0x1028(%esp)"
//      S+0x1818 char  sWord[2048]        "0x1828(%esp)"
//      S+0x2018 char  sArg[2048]         "0x2028(%esp)"
//      S+0x2818 int   anIfStack[1024]    "0x2828(%esp,%esi,4)"
//  See COIN-FLIP 2 for the declaration order that produces that layout.
//
//  CALLEES
//      0x004290b0  operator new       0x004290c0  operator delete
//      0x00492f30  stricmp            0x00493660  strchr
//      0x00491af0  CControlFileSectionList::Find
//      0x00492040  CControlFileMgr::IsDefined
//      vtable +0x00 FileOpen  +0x04 FileClose  +0x08 FileGetChar
//      vtable +0x0c FileEOF
//
//  THE #-COMMAND STATE CONSTANTS. anIfStack holds 1, 2, 3 for a live
//  #ifdef / #ifndef / #else and 0x3e9, 0x3ea, 0x3eb (1001, 1002, 1003) for
//  a dead one; "is this block dead" is `> 1000`, read as
//  `cmpl $0x3e8,...; jle` at 0x0049236b and 0x004923b8. The values are
//  written as bare integers because nothing in the image names them.
//
//  SHAPES THE BYTES DECIDE
//   * The entry guard's failure block is the FALL-THROUGH (0x004920af,
//     jumped over by `jne`), so it is an early `return FALSE`, not an
//     inverted positive guard: the function's other return is 1,800 bytes
//     further on and the guard does not jump past it.
//   * The outer loop is a ROTATED `while (!FileEOF())`: pre-test at
//     0x0049211b, back edge at 0x004927ca onto 0x0049212b (the
//     FileGetChar), which is why FileEOF is emitted twice.
//   * Every "skip to the end of the line" is the same two-condition while:
//     `if (FileEOF()) goto out` / `L: if (c == EOL) goto out` /
//     `c = FileGetChar()` / `if (!FileEOF()) goto L`, i.e.
//     `while (!FileEOF() && c != m_cEndOfLine) c = FileGetChar();`
//     (0x0049216b, 0x004923ce, 0x0049240c, 0x00492791).
//   * THE LAST ONE OF THOSE COMPARES AGAINST A LITERAL '\n', NOT THE
//     MEMBER: 0x004927a3 is `cmp $0xa,%bl` where all three siblings are
//     `cmp 0x10(%ebp),%bl`. The source really does write '\n' there.
//   * The four #-command tests are four INDEPENDENT `if`s, not an
//     else-if chain: the matched arm of "ifdef" ends `jmp 0x004922fa`
//     (0x004922e4), straight into the "ifndef" stricmp.
//   * THE COMMAND-ARGUMENT SCAN DROPS ITS FIRST CHARACTER. The
//     skip-whitespace loop at 0x00492208 exits holding the first
//     non-whitespace character, and the word loop at 0x0049223e starts by
//     calling FileGetChar again (0x0049224c) instead of storing it. That
//     is a bug in the shipped source; the byte sequence forces it.
//   * The key-name (0x0049254b) and word (0x0049268b) loops STORE FIRST
//     and read second, so they do keep their first character; the command
//     (0x004921b6) and argument (0x0049224c) loops read first.
//   * `new CControlFileSection` / `new CControlFileKey` push 0x14 and
//     zero +0x08, +0x0c, +0x10 -- CLithSimpAry's and CLithBaseList's
//     constructors inlined, nothing else, which is why the two link
//     pointers are left uninitialised. `new CControlFileWord` pushes 0x10
//     and zeroes only +0x08, which is why m_nStrLen has to be assigned
//     explicitly at 0x00492777.
//   * m_nStrLen is stored AFTER the InsertLast expansion (0x00492777
//     follows 0x00492774), so it is written after the insert in source.
// ======================================================================= //

BOOL_Q490 CControlFileMgr::ProcessFile(const char* sFileName)
{
	int   anIfStack[1024];
	char  sArg[2048];
	char  sWord[2048];
	char  sKeyName[2048];
	char  sSectionName[2048];
	char  sCommand[2048];
	CControlFileSection* pSection;
	CControlFileKey*     pKey;
	CControlFileWord*    pWord;
	int   nIfIdx;
	int   nLen;
	int   nSkip;
	char  c;

	// Statements, nSkip first: 0x0049209d stores the 0 before 0x004920a1
	// stores the -1 (initialisers in declaration order swapped them).
	nSkip = 0;
	nIfIdx = -1;

	// 0x004920a9 -- the only use of vtable slot 0, and the only place a
	// stack argument is pushed for a virtual in this function.
	if (!FileOpen(sFileName))
		return FALSE_Q490;

	// 0x004920ba -- the nameless section every key before the first
	// '[' header goes into. Alloc(1) then a bare NUL store (0x004920ff),
	// not a Copy: see COIN-FLIP 3.
	pSection = new CControlFileSection;
	pSection->m_sSectionName.Alloc(1);
	pSection->m_sSectionName[0] = '\0';
	m_lstSections.InsertLast(pSection);

	// 0x0049211b / 0x004927c0
	while (!FileEOF())
	{
		c = FileGetChar();                                   // 0x0049212b
		if (FileEOF())                                       // 0x00492133
			break;

		if (strchr(m_sWhiteSpace, c))                        // 0x0049214d
			continue;

		if (c == m_cEndOfLine)                               // 0x0049215d
			continue;

		// ---- a whole-line comment ------------------------------------ //
		if (c == m_cComment)                                 // 0x00492166
		{
			while (!FileEOF() && c != m_cEndOfLine)          // 0x0049216b
				c = FileGetChar();

			continue;
		}

		// ---- a special command: #ifdef / #ifndef / #else / #endif ---- //
		if (c == m_cSpecialCommand)                          // 0x0049219f
		{
			// the command word                              // 0x004921a8
			nLen = 0;
			while (!FileEOF())
			{
				c = FileGetChar();                           // 0x004921bb
				if (strchr(m_sWhiteSpace, c))                // 0x004921c8
					break;
				if (c == m_cEndOfLine)                       // 0x004921d4
					break;
				if (FileEOF())                               // 0x004921de
					break;
				if (nLen < 2047)                             // 0x004921e5
				{
					sCommand[nLen] = c;
					nLen++;
				}
			}
			sCommand[nLen] = '\0';                           // 0x00492203

			// step over the whitespace after it             // 0x00492208
			while (!FileEOF())
			{
				c = FileGetChar();                           // 0x0049220f
				if (!strchr(m_sWhiteSpace, c))               // 0x00492221
					break;
				if (c == m_cEndOfLine)                       // 0x0049222d
					break;
			}

			// the command's argument                        // 0x0049223e
			nLen = 0;
			while (!FileEOF())
			{
				c = FileGetChar();                           // 0x00492251
				if (strchr(m_sWhiteSpace, c))                // 0x0049225e
					break;
				if (c == m_cEndOfLine)                       // 0x0049226a
					break;
				if (FileEOF())                               // 0x00492274
					break;
				if (nLen < 2047)                             // 0x0049227b
				{
					sArg[nLen] = c;
					nLen++;
				}
			}
			sArg[nLen] = '\0';                               // 0x004922a1

			if (stricmp(sCommand, "ifdef") == 0)             // 0x004922a9
			{
				if (nIfIdx < 1023)                           // 0x004922b9
				{
					nIfIdx++;
					if (IsDefined(sArg))                     // 0x004922d0
					{
						anIfStack[nIfIdx] = 1;               // 0x004922d9
					}
					else
					{
						anIfStack[nIfIdx] = 1001;            // 0x004922ea
						nSkip++;
					}
				}
			}

			if (stricmp(sCommand, "ifndef") == 0)            // 0x00492304
			{
				if (nIfIdx < 1023)                           // 0x00492310
				{
					nIfIdx++;
					// Negated, unlike "ifdef": 0x0049232e `jne` skips the
					// 2-store fall-through to the 1002 arm.
					if (!IsDefined(sArg))                    // 0x00492327
					{
						anIfStack[nIfIdx] = 2;               // 0x00492330
					}
					else
					{
						anIfStack[nIfIdx] = 1002;            // 0x00492341
						nSkip++;
					}
				}
			}

			if (stricmp(sCommand, "else") == 0)              // 0x0049235b
			{
				if (nIfIdx >= 0)                             // 0x00492367
				{
					if (anIfStack[nIfIdx] > 1000)            // 0x0049236b
					{
						anIfStack[nIfIdx] = 3;               // 0x0049237c
						nSkip--;
					}
					else
					{
						anIfStack[nIfIdx] = 1003;            // 0x0049238e
						nSkip++;
					}
				}
			}

			if (stricmp(sCommand, "endif") == 0)             // 0x004923a8
			{
				if (nIfIdx >= 0)                             // 0x004923b4
				{
					if (anIfStack[nIfIdx] > 1000)            // 0x004923b8
						nSkip--;                             // 0x004923c5

					nIfIdx--;                                // 0x004923c9
				}
			}

			while (!FileEOF() && c != m_cEndOfLine)          // 0x004923ce
				c = FileGetChar();

			continue;
		}

		// ---- inside a dead #ifdef block ------------------------------ //
		if (nSkip > 0)                                       // 0x00492402
		{
			while (!FileEOF() && c != m_cEndOfLine)          // 0x0049240c
				c = FileGetChar();

			continue;
		}

		// ---- a section header ---------------------------------------- //
		if (c == m_cSectionStart)                            // 0x00492440
		{
			nLen = 0;                                        // 0x00492449
			while (!FileEOF())
			{
				c = FileGetChar();                           // 0x0049245c
				if (c == m_cSectionEnd)                      // 0x00492464
					break;
				if (c == m_cEndOfLine)                       // 0x00492468
					break;
				if (FileEOF())                               // 0x00492472
					break;
				if (nLen < 2047)                             // 0x00492479
				{
					sSectionName[nLen] = c;
					nLen++;
				}
			}
			sSectionName[nLen] = '\0';                       // 0x0049249c

			pSection = m_lstSections.Find(sSectionName);     // 0x004924a8
			if (pSection == NULL_Q490)                       // 0x004924ad
			{
				pSection = new CControlFileSection;          // 0x004924b9
				pSection->m_sSectionName.Alloc(nLen + 1);    // 0x004924f0
				pSection->m_sSectionName.Copy(sSectionName, nLen + 1);
				m_lstSections.InsertLast(pSection);          // 0x00492515
			}

			continue;
		}

		// ---- a key, and the words on its line ------------------------ //
		nLen = 0;                                            // 0x0049253d
		while (!FileEOF())
		{
			if (nLen < 2047)                                 // 0x0049254b
			{
				sKeyName[nLen] = c;
				nLen++;
			}
			if (FileEOF())                                   // 0x0049255b
				break;
			c = FileGetChar();                               // 0x00492567
			if (c == m_cEndOfLine)                           // 0x00492574
				break;
			if (strchr(m_sWhiteSpace, c))                    // 0x00492580
				break;
		}
		sKeyName[nLen] = '\0';                               // 0x0049259a

		pKey = new CControlFileKey;                          // 0x00492598
		pKey->m_sKeyName.Alloc(nLen + 1);                    // 0x004925d5
		pKey->m_sKeyName.Copy(sKeyName, nLen + 1);
		pSection->m_lstKeys.InsertLast(pKey);                // 0x004925fe

		while (!FileEOF())                                   // 0x0049261b
		{
			c = FileGetChar();                               // 0x00492630
			if (FileEOF())                                   // 0x0049263e
				break;

			if (strchr(m_sWhiteSpace, c))                    // 0x00492651
				continue;

			if (c == m_cEndOfLine)                           // 0x00492661
				break;

			if (c == m_cComment)                             // 0x0049266a
			{
				// 0x004927a3 tests a LITERAL '\n' here, not m_cEndOfLine
				while (!FileEOF() && c != '\n')              // 0x00492791
					c = FileGetChar();

				break;
			}

			nLen = 0;                                        // 0x00492673
			while (!FileEOF())
			{
				if (nLen < 2047)                             // 0x0049268b
				{
					sWord[nLen] = c;
					nLen++;
				}
				if (FileEOF())                               // 0x0049269b
					break;
				c = FileGetChar();                           // 0x004926ac
				if (c == m_cEndOfLine)                       // 0x004926b6
					break;
				if (strchr(m_sWhiteSpace, c))                // 0x004926c4
					break;
			}
			sWord[nLen] = '\0';                              // 0x004926e2

			pWord = new CControlFileWord;                    // 0x004926e0
			pWord->m_sVal.Alloc(nLen + 1);                   // 0x00492722
			pWord->m_sVal.Copy(sWord, nLen + 1);
			pKey->m_lstWords.InsertLast(pWord);              // 0x0049274a
			pWord->m_nStrLen = nLen;                         // 0x00492777

			if (c == m_cEndOfLine)                           // 0x0049277a
				break;
		}
	}

	FileClose();                                             // 0x004927d5

	return TRUE_Q490;                                        // 0x004927db
}


// ======================================================================= //
//  0x004927f0  CControlFileMgr::FileOpen(const char*)        37 bytes
//
//  fopen(sName, "rt") (0x00492b41, mode string 0x004a87d4), the result
//  stored to m_pFile (0x0049280c) and `setne` on the register copy.
// ======================================================================= //

BOOL_Q490 CControlFileMgr::FileOpen(const char* sName)
{
	m_pFile = fopen(sName, "rt");
	return (m_pFile != NULL_Q490);
}


// ======================================================================= //
//  0x00492820  CControlFileMgr::FileClose()                  11 bytes
//
//  vtable slot +0x04: `push m_pFile; call fclose (0x00492a93); pop %ecx`.
// ======================================================================= //

void CControlFileMgr::FileClose()
{
	fclose(m_pFile);
}


// ======================================================================= //
//  0x00492830  CControlFileMgr::FileGetChar()                13 bytes
//
//  vtable slot +0x08. `push m_pFile; call fgetc` (0x00495919: _lock_str,
//  the getc body, _unlock_str) with `add $4,%esp` because the result is
//  returned in %eax.
// ======================================================================= //

int CControlFileMgr::FileGetChar()
{
	return fgetc(m_pFile);
}


// ======================================================================= //
//  0x00492840  CControlFileMgr::FileEOF()                    10 bytes
//
//  vtable slot +0x0c: stdio.h's feof macro on m_pFile, `mov 0x4(%ecx),%eax;
//  mov 0xc(%eax),%eax; and $0x10,%eax` (FILE::_flag & _IOEOF).
// ======================================================================= //

BOOL_Q490 CControlFileMgr::FileEOF()
{
	return feof(m_pFile);
}


// ======================================================================= //
//  ROWS DECLINED, AND WHY  (nine of the fourteen)
// ======================================================================= //
//
//  A. CRT / COMPILER SCAFFOLDING -- do not reconstruct.
//
//   0x00492888  56  "call"   ***  _cfltcvt_init, MSVC CRT  ***
//       Six stores into 0x004af880..0x004af894 of five .text addresses,
//       with the SAME address (0x00495d73) stored to slot 0 (0x00492897)
//       and slot 5 (0x004928ba). That is the MSVC6 `_cfltcvt_init`
//       filling `_cfltcvt_tab[6]` with _cfltcvt, _cropzeros, _fassign,
//       _forcdecpt, _positive and _cfltcvt again -- the printf
//       floating-point hook table. It is library code, it is reached from
//       the CRT's own startup, and the next row (0x004928c0) and
//       0x004928e7 (_purecall) are CRT too. DECLINED.
//
//   0x00490c90  53  "thunk"  *** a dynamic initialiser plus an
//                                already-written constructor ***
//       The row covers TWO things and neither is source to write, and
//       BOTH already exist in src/lithshared/stdlith/helpers.cpp.
//       The chain is the exact `??__E` shape src/engine/lithtech/
//       ltnew2.cpp catalogues:
//           0x004a70d0 (a `.CRT$XC` table slot, table
//                       0x004a7000..0x004a70d8)
//             -> 0x00490c80  `jmp 0x00490c90` + eleven 0x90
//             -> 0x00490c90  `mov $0x004b4d84,%ecx; jmp 0x00490ca0`
//                            + six 0x90              <-- THE ROW
//             -> 0x00490ca0  CHelpers::CHelpers(), 37 bytes, which
//                            matches BYTE-IDENTICAL from
//                            src/lithshared/stdlith/Release/helpers.obj
//                            AND from the period
//                            src/LT2/lithshared/libs/Release/StdLith.lib. It
//                            fills the 256-entry table at 0x004b4d88 from
//                            0x00493ffa(i).
//       0x004b4d84 is the file-scope object, and helpers.cpp:24 already
//       says `static CHelpers g_Helpers;` with `static char
//       g_UpperTable[256];` above it. So the source for this row is
//       WRITTEN; only the object's COMDATs are not being spliced into
//       lithtech.exe. DECLINED -- there is no function to reconstruct.
//
//  B. ALREADY WRITTEN AND ALREADY BYTE-IDENTICAL in src/lithshared.
//     The .obj files under src/lithshared match
//     all four at exactly the enumerated size:
//
//   0x00490dd0  115  ?ExtractFileNameAndExtension@CHelpers@@SAHPBDPAD1@Z
//                    (src/lithshared/stdlith/helpers.cpp)
//   0x004911a0  197  ?FindString@CStringHolder@@QAEPADPBD@Z
//   0x00491270   89  ?ClearStrings@CStringHolder@@QAEXXZ
//                    (src/lithshared/stdlith/stringholder.cpp; I diffed
//                     FindString's 197 bytes instruction by instruction
//                     against the object and they are identical)
//
//     The 0x00491270 row's stated size, 160, is the distance to the NEXT
//     COMDAT: ClearStrings ends at 0x004912c8 and 0x004912c9..0x004912cf
//     is 0x90 padding. 0x004912d0 is
//     `??1?$CMoArray@VSBank@@VDefaultCache@@@@QAE@XZ`, which is already
//     matched.
//
//     WHAT THESE NEED: nothing in source. The
//     gap is that these COMDATs are not being spliced into lithtech.exe.
//     The whole run 0x00490010..0x00491e30 is lithshared: 84 addresses
//     in it come from src/lithshared alone (CRezMgr/CRezFile/
//     CRezFileDirectoryEmulation, sb_*/StructBank, CHelpers,
//     CStringHolder, CMoArray<> instantiations, LAllocSimpleBlock,
//     MoArray_FindElementMemcmp, GLinkedList_FindElementMemcmp).
//
//  C. ALREADY WRITTEN AS NEARS in src/lithshared/rezmgr -- not untouched
//     rows. Both are named, sized and analysed there already:
//
//   0x004904e0  86  CRezFileRf90::Close     (rezfile.cpp:790; ours is 99)
//   0x00490540  56  CRezFileRf90::Flush     (rezfile.cpp:840; ours is 69)
//
//     A DIAGNOSIS FOR THE LAST TWO, since it is cheap and they are 13
//     bytes out each. I disassembled our own
//     src/lithshared/rezmgr/Release/rezfile.obj: the `do { if (fclose(..)
//     == 0) {...} } while (m_pRezMgr->DiskError());` we ship is ROTATED by
//     VC6 -- it peels the first fclose above the loop (reusing the %eax
//     the null guard already loaded) and heads the loop with DiskError,
//     giving 99 bytes. The shipped 86 bytes emit ONE fclose, at the loop
//     top (0x004904ea), and that top RELOADS `mov 0x10(%esi),%eax` -- a
//     join point, not a peeled copy. An entry-untested loop cannot be
//     rotated, so the source shape that produces it is the one the
//     file's own comment describes but does NOT use:
//
//         if (m_pFile == NULL) return FALSE;
//         for (;;)
//         {
//             if (fclose(m_pFile) == 0) break;
//             if (!m_pRezMgr->DiskError()) return FALSE;
//         }
//         m_pFile = NULL;
//         if (m_sFileName != NULL) delete m_sFileName;
//         m_sFileName = NULL;
//         m_nLastSeekPos = 0xffffffff;
//         return TRUE;
//
//     and, identically, for Flush (0x00490551 is the loop top and it
//     reloads the same way):
//
//         m_nLastSeekPos = 0xffffffff;
//         if (m_pFile == NULL) return FALSE;
//         for (;;)
//         {
//             if (fflush(m_pFile) == 0) return TRUE;
//             if (!m_pRezMgr->DiskError()) break;
//         }
//         return FALSE;
//
//     The `return FALSE` sites then tail-merge the way they ship
//     (0x00490506 before 0x0049050a; 0x0049056d before 0x00490571).
//     UNTESTED.
//
//
//  ALSO WORTH REPORTING: FIVE FUNCTIONS OF THIS TU ARE ALREADY WRITTEN
//  UNDER INVENTED NAMES in src/engine/lithtech/ltnew2.cpp, as "CNode",
//  "CNodeList" and "COwnedBlock". The bodies there are right; only the
//  names and the layout comment are wrong, and the layout comment is
//  wrong in a way that matters (it makes CNode 0x24 bytes when the
//  allocation at 0x004920bc and 0x00492598 is 0x14):
//
//      0x00491a50  CNode::GetValue(int32*)   -> CControlFileWord::GetVal(int&)
//      0x00491a70  CNode::GetValue(float*)   -> CControlFileWord::GetVal(float&)
//      0x00491ad0  CNode::GetChild           -> CControlFileSection::GetKey
//      0x00491e50  CNode::GetSection         -> CControlFileMgr::GetSection
//      0x00492850  COwnedBlock::~COwnedBlock -> CLithSimpAry<char>::~CLithSimpAry
//                                               (the out-of-line copy the EH
//                                                funclets at 0x004a0a66 and
//                                                0x004a0a71 jump to)
//      0x00491a90  (named GetFirstChild)     -> CControlFileKey::GetFirstWord
//      0x00491af0  (named CNodeList::Find)   -> CControlFileKeyList::Find AND
//                                               CControlFileSectionList::Find,
//                                               one address because the two
//                                               bodies are identical and the
//                                               linker folded them
//      0x00491e30  (byte-matched as
//                   CLTGUIFont::Term)        -> the folded destructor of
//                                               CControlFileWord /
//                                               CControlFileKey /
//                                               CControlFileSection --
//                                               all three destroy one
//                                               CLithSimpAry<char> at +0x08
//                                               and nothing else, so all
//                                               three fold together. The
//                                               CLTGUIFont name is a
//                                               same-bytes false positive.
//
//  They belong in this file, not in ltnew2.cpp, and the whole run
//  0x004919e0..0x00492850 should be moved into one controlfilemgr.cpp.
//
//
// ======================================================================= //
//  COIN-FLIPS I COULD NOT SETTLE
// ======================================================================= //
//
//   1. 0x004919e0 -- ONE trailing `return TRUE;` (written above) versus a
//      `return TRUE;` at the end of each arm. /O2 preserves duplicated
//      epilogues, so both spellings can produce the two `mov $0x1,%eax;
//      pop; pop; ret $0x8` blocks at 0x00491a10 and 0x00491a26. The weak
//      evidence for the single trailing return is that the `else` arm
//      SCHEDULES its `mov $0x1,%eax` into the middle of the strcpy
//      expansion (0x00491a26, between the `sub %ecx,%edi` and the
//      `rep movsl`), which is a sink of a value produced once. Nothing
//      decides it; try the per-arm spelling second.
//
//   2. 0x00492080 -- the DECLARATION ORDER of ProcessFile's locals. The
//      frame layout is fixed and read off the bytes (see the banner), but
//      which end of it the first declaration lands on is not. Written here is
//      the order that assigns the FIRST-declared local the HIGHEST offset
//      (anIfStack, sArg, sWord, sKeyName, sSectionName, sCommand, then the
//      scalars), which is the usual MSVC assignment. The alternative is
//      the exact reverse (sCommand first, anIfStack last). Getting it
//      wrong costs no BYTES OF SIZE -- every array displacement is a
//      dword and every scalar displacement is a byte either way -- so the
//      function will come out size-exact and differ only in
//      displacements, and one bindiff of the two builds settles it.
//      The scalars' own order (c, nSkip, nLen, nIfIdx, pKey, pSection
//      upward) is a second, independent permutation of the same kind.
//
//   3. 0x004920f0..0x004920ff -- `pSection->m_sSectionName[0] = '\0';`
//      versus `strcpy(pSection->m_sSectionName, "");`. VC6's /Oi strcpy
//      with a one-character constant source folds to the same
//      `movb $0x0,(%edx)`, and `Copy(pSection->m_sSectionName, "", 1)`
//      does NOT (it would load the literal's byte). So it is one of the
//      first two; I took the assignment because it needs no literal in
//      .rdata and the image has no empty string pooled next to
//      "ifdef"/"ifndef"/"else"/"endif" at 0x004af7bc..0x004af7d9.
//
//   4. 0x00492145 etc. -- `strchr(m_sWhiteSpace, c)` relies on
//      CLithSimpAry<char>::operator char*(). The shipped push is
//      `movsbl %bl,%ecx` (0x00492148), i.e. a SIGNED char widened to int,
//      so `c` is a plain `char` and not `unsigned char` or `int`. That
//      part is settled; what is not is whether the source wrote
//      `strchr(m_sWhiteSpace, c)` or `strchr((char*)m_sWhiteSpace, c)`.
//      Both compile to the same call.
//
//   5. The literals 2047 and 1023 (`cmp $0x7ff` at 0x004921e5 and
//      `cmp $0x3ff` at 0x004922b9). Written as bare integers. If the
//      original used `sizeof(sWord)-1` or a macro the bytes are the same;
//      no way to tell.


// ======================================================================= //
//  0x00490130  CRezTypeHashTable::Find(REZTYPE)            WRITTEN, 62 bytes
//
//  *** THIS BODY BELONGS IN src/lithshared/rezmgr/rezdir.cpp, NOT HERE. ***
//  It is fenced out of this translation unit because it needs that file's
//  CBaseHash / CRezTypeHash / CRezTyp declarations, every one of which
//  rezdir.cpp already has (lines 381-391, 425-435, 573-589). rezdir.cpp's
//  closing note says this function is past that file's range, declared
//  and left undefined; here it is. Drop it in beside
//  CRezDirHashTableX380::Find at rezdir.cpp:1691 and delete the fence.
//
//  Extent: 0x00490130..0x0049016e (62 bytes; 0x0049016f is one 0x90 pad
//  before the 0x00490170 COMDAT). Two `ret $0x4`, nothing branches out.
//
//  Calls, in order (2):
//      0x00490139  0x00490120  CRezTypeHashTable::HashFunc(REZTYPE)
//                              (`xor %edx,%edx; divl (%ecx)` -- nType %
//                              m_nNumBins, with m_nNumBins at +0x00)
//      0x00490141  0x004a0590  CBaseHash::GetFirstInBin
//
//  WHAT PINS IT
//   * `ret $0x4` -> __thiscall, ONE argument. There is NO null guard on it,
//     unlike the two by-name Finds at 0x00490050, because REZTYPE is a
//     DWORD and not a pointer. That absence is the only structural
//     difference between this and its siblings.
//   * `mov 0x14(%eax),%ecx` then `cmp %edi,(%ecx)` -- the element's back
//     pointer is at +0x14 (rezdir.cpp already reads +0x14 that way at
//     0x00490030) and CRezTyp::m_nType is at +0x00, so the compare is
//     `pElem->GetRezTyp()->GetType() == nType`.
//   * THE MEMBER IS ON THE LEFT: `cmp %edi,(%ecx)` with %edi = the
//     argument, i.e. `cmp [member], arg`.
//   * `mov 0x4(%eax),%eax; test; je; add $0xfffffffc,%eax; test; jne` at
//     0x00490151 is NextInBin() -- CBaseListItem::Next() off +0x04 plus
//     the guarded -4 base adjustment for the CBaseListItem* ->
//     CBaseHashItem* cast, exactly as rezdir.cpp documents for 0x00490050.
//   * A pre-test at 0x00490148 and the back edge's own test at 0x0049015b
//     -- the standard rotated `while (pElem != NULL)`.
//   * The hit arm RE-READS `mov 0x14(%eax),%eax` (0x00490166) rather than
//     keeping the %ecx it just compared through, so GetRezTyp() is written
//     out twice in the source, once in the test and once in the return.
//
//  CONFIDENCE: high. It is the 0x00490050 body with one guard removed and
//  an integer compare in place of the string compare, and 0x00490050 is
//  already byte-identical in our tree.
// ======================================================================= //

#ifdef W9_REZDIR_FRAGMENT_Q490

CRezTypX380* CRezTypeHashTableX380::Find(REZTYPE_X380 nType)
{
	CRezTypeHashX380* pElem;

	pElem = (CRezTypeHashX380*)GetFirstInBin(HashFunc(nType));

	while (pElem != NULL_X380)
	{
		if (pElem->GetRezTyp()->GetType() == nType)
			return pElem->GetRezTyp();

		pElem = pElem->NextInBin();
	}

	return NULL_X380;
}

#endif // W9_REZDIR_FRAGMENT_Q490
