// *********************************************************************** //
//
//  MODULE:   rezmgr.lib -- the twelve functions of it nobody owns.
//            0x00490110..0x004901a8 (the tail of rezhash.cpp) and
//            0x0048e630 / 0x0048e660..0x0048e716 (seven CRezDir accessors
//            from rezmgr.cpp).  Two original translation units in one file,
//            the way src/lithshared/rezmgr/rezdir.cpp already is; /O2
//            implies /Gy, so every body is its own COMDAT and is
//            matched one at a time regardless.
//
//  READ THIS FIRST -- THE ENUMERATION IS NOT WHAT IT LOOKS LIKE.
//
//  All twelve enumerated rows are ALREADY WRITTEN.  0x0048df10..0x00490b43 of
//  lithtech.exe is Monolith's prebuilt rezmgr.lib, which this
//  repository already reconstructs in src/lithshared/rezmgr/{rezmgr,rezmgr2,
//  rezfile,rezemu,rezdir}.cpp.  Ten of the twelve rows are functions with
//  bodies in those files; the other two (0x0048ec46, 0x0048f8aa) are the two
//  enumeration defects those files already document.  Every one of the ten
//  is an UNMATCHED NEAR, which is exactly why the screen let them through:
//  rows inside near-but-not-identical functions are not screened out.
//
//      src/lithshared/rezmgr/Release/*.obj against the retail lithtech.exe:
//      matched 55, unmatched 12, too-small-to-match 27
//
//  and the twelve unmatched are the ten enumerated rows plus CRezFile::Close
//  and CRezFile::Flush.  So there is no reconstruction to do on the rows
//  themselves; the appendix below is a residual diagnosis
//  for each, and the CODE below is the one piece of genuinely UNCLAIMED
//  rezmgr.lib left in the neighbourhood.
//
//  WHAT THIS FILE IS.  src/lithshared/rezmgr/rezdir.cpp ends with a note
//  headed "NOT WRITTEN HERE, and why" that deliberately leaves five
//  functions undefined because they sat past that file's range:
//
//      0x00490110  15 bytes  `ret`        an element HashFunc override
//      0x00490120  13 bytes  `ret $4`     a table HashFunc(REZTYPE)
//      0x00490130  62 bytes  `ret $4`     CRezTypeHashTable::Find
//      0x00490170  15 bytes  `ret`        the other element HashFunc
//      0x00490180  41 bytes  `ret $4`     a table HashFunc(REZNAME)
//
//  Nothing in the repository defines them (grep confirms: they appear only
//  as declarations-with-address-comments in rezdir.cpp and rezmgr2.cpp).
//  All five clear the eight-unmasked-byte floor, and all five have
//  server.dll twins (0x1003c760, 0x1003c770, 0x1003c780, 0x1003c7c0 and
//  0x1003c670 respectively -- verified by disassembling both images), so
//  they score in BOTH images.
//
//  AND SEVEN MORE, from inside the enumerated range itself.  0x0048e630 and
//  0x0048e660..0x0048e716 are seven CRezDir accessors that rezmgr.h declares
//  by name.  Two of them (0x0048e660, 0x0048e6a0) are in no source file at
//  all; the other five are written up in src/engine/lithtech/ as
//  unidentified CLIENT code, which means they can never score in server.dll
//  even when they match in lithtech.exe.  All seven have server.dll twins.
//  They are written below with their real names and real classes -- see the
//  block headed "SEVEN MORE".  One of them, 0x0048e630, is also a controlled
//  experiment; read its comment before measuring.
//
//  AND rezdir.cpp HAS TWO OF THEM THE WRONG WAY ROUND.  Its class views
//  declare 0x00490110 as CRezDirHash::HashFunc (line 374) and 0x00490170 as
//  CRezTypeHash::HashFunc (line 388).  The bytes say the opposite and say it
//  twice over:
//    * 0x00490110 tail-calls 0x00490120, which is `value % m_nNumBins` with
//      a bare `divl (%ecx)` and NO null guard -- a NUMERIC key.  The only
//      numeric key in the library is CRezTyp::m_nType, which rezmgr.h puts
//      at CRezTyp +0x00, and `mov (%eax),%eax` reads exactly that.
//    * 0x00490170 tail-calls 0x00490180, which is a byte-at-a-time length
//      loop behind `if (s == NULL) return 0;` -- a STRING key, i.e.
//      CRezDir::m_sDirName at CRezDir +0x00.
//    * corroboration from the other end: CRezTypeHashTable::Find
//      (0x00490130) calls 0x00490120, so 0x00490120 is the TYPE table's
//      HashFunc and 0x00490110 must be the TYPE element's override; and the
//      already-matched CRezItmHashByName::HashFunc (0x00490030) calls
//      0x00490180, so 0x00490180 is the BY-NAME table's HashFunc and
//      0x00490170 is the DIRECTORY element's override (CRezDirHashTable::
//      Find hashes its name through the same 0x00490180 at 0x0049005f).
//  Fix those two comments in rezdir.cpp when this lands.
//
//  BUILD.  This is rezmgr.lib code, so it wants build/libs.sh's
//  `/GX /O2` (LIB_CFLAGS) and CL_REV=8447, and its object must be dropped
//  into BOTH src/engine/lithtech/Release and src/engine/server/Release the
//  way the other five rezmgr objects are.  Easiest integration: rename to
//  src/lithshared/rezmgr/rezhash.cpp and add `rezhash.cpp` to
//  build_rezmgr()'s source list in build/libs.sh.  Every symbol here carries
//  a `Q48C` suffix so it cannot collide with the five existing files.
//
//  NO CRT IS USED.  There is no strlen here and that is deliberate: the
//  library IS compiled /Oi (0x00490050 inlines strcmp and 0x0048e7c7 emits
//  `repnz scasb` for strlen), so a source-level `strlen()` at 0x00490180
//  would have come out as `repnz scasb`.  It did not -- the shipped code
//  walks bytes with `mov 0x1(%edx),%bl ; inc %eax ; inc %edx` -- so the
//  source counts the characters itself.
//
// *********************************************************************** //


// ======================================================================= //
//  lithtypes.h / reztypes.h -- only the widths are load-bearing.
// ======================================================================= //

typedef int                 BOOL_Q48C;
typedef unsigned long       DWORD_Q48C;

#define TRUE_Q48C   1
#define FALSE_Q48C  0
#define NULL_Q48C   0

typedef char*               REZNAME_Q48C;
typedef char*               REZDIRNAME_Q48C;
typedef DWORD_Q48C          REZTYPE_Q48C;


// ======================================================================= //
//  src/lithshared/lith/baselist.h and basehash.h -- copied verbatim in
//  substance from src/LT2/lithshared/lith/basehash.h, which is in the repo
//  unmodified, and identical to the views src/lithshared/rezmgr/rezdir.cpp
//  already uses (renamed).
//
//  CBaseHashItem adds the first virtual, so the vptr takes +0x00 and the
//  CBaseListItem subobject is pushed to +0x04.  That is what makes
//  m_pParentHash land at +0x0c (read by `mov 0xc(%ecx),%ecx` at 0x00490113
//  and 0x00490173) and the derived back-pointer land at +0x14 (read by
//  `mov 0x14(%ecx),%eax` at 0x00490110 and 0x00490170).
//
//  It is also what makes NextInBin() cost a base adjustment: 0x00490151
//  reads the list link at +0x04 and 0x00490158 `add $0xfffffffc,%eax`
//  converts CBaseListItem* back to CBaseHashItem*, with its own null guard.
// ======================================================================= //

class CBaseListQ48C;

class CBaseListItemQ48C
{
public:
	CBaseListItemQ48C* Next() { return m_pNext; }
	CBaseListItemQ48C* Prev() { return m_pPrev; }

public:
	friend class CBaseListQ48C;

protected:
	CBaseListItemQ48C* m_pNext;              // +0x00 of the subobject
	CBaseListItemQ48C* m_pPrev;              // +0x04 of the subobject
};

class CBaseHashItemQ48C;

class CBaseHashQ48C
{
public:
	CBaseHashQ48C();
	CBaseHashQ48C(unsigned int NumBins);                            // 0x004a0430
	~CBaseHashQ48C();                                               // 0x004a04c0

	void               Insert(CBaseHashItemQ48C* pItem);            // 0x004a04f0
	void               Delete(CBaseHashItemQ48C* pItem);            // 0x004a0530
	CBaseHashItemQ48C* GetFirst();                                  // 0x004a0560
	CBaseHashItemQ48C* GetLast();

protected:
	CBaseHashItemQ48C* GetFirstInBin(unsigned int Bin);             // 0x004a0590
	unsigned int       GetNumBins() { return m_nNumBins; }

private:
	friend class CBaseHashItemQ48C;

	unsigned int       m_nNumBins;           // +0x00  (0x00490126 / 0x004901a2
	                                         //         divide by it in place)
	void*              m_pBinAry;            // +0x04
};

class CBaseHashItemQ48C : public CBaseListItemQ48C
{
public:
	CBaseHashItemQ48C* Next();                                      // 0x004a03d0
	CBaseHashItemQ48C* Prev();
	CBaseHashItemQ48C* NextInBin() { return (CBaseHashItemQ48C*)CBaseListItemQ48C::Next(); }
	CBaseHashItemQ48C* PrevInBin() { return (CBaseHashItemQ48C*)CBaseListItemQ48C::Prev(); }

protected:
	virtual unsigned int HashFunc() = 0;

	CBaseHashQ48C* GetParentHash() { return m_pParentHash; }

private:
	friend class CBaseHashQ48C;

	CBaseHashQ48C* m_pParentHash;            // +0x0c
	unsigned int   m_nCurBin;                // +0x10
};


// ======================================================================= //
//  rezmgr.h and rezhash.h -- the owners, their hash elements and their hash
//  tables.  Every offset below is either read off an instruction in this
//  file's range (cited against the member) or is forced by rezmgr.h's
//  declaration order once the ones that are read are fixed.  The three
//  sibling files in src/lithshared/rezmgr derived CRezItm = 0x3c,
//  CRezTyp = 0x30, CRezDir = 0x4c and CRezMgr = 0xd4 independently and
//  agree; the six accessors at 0x0048e660..0x0048e716 below confirm every
//  one of those totals a third time, because their `lea` displacements are
//  the member offsets and they run right to the end of each object.
//
//  The elements are 0x18 bytes: CBaseHashItem (vptr +0x00, list links +0x04
//  and +0x08, m_pParentHash +0x0c, m_nCurBin +0x10) plus one back pointer
//  at +0x14.
// ======================================================================= //

class CRezItmQ48C;
class CRezTypQ48C;
class CRezDirQ48C;
class CRezMgrQ48C;

class CRezItmHashTableByNameQ48C;
class CRezDirHashTableQ48C;
class CRezTypeHashTableQ48C;


// -- the three hash ELEMENTS ------------------------------------------- //

class CRezItmHashByNameQ48C : public CBaseHashItemQ48C
{
public:
	CRezItmHashByNameQ48C* Next()      { return (CRezItmHashByNameQ48C*)CBaseHashItemQ48C::Next(); }
	CRezItmHashByNameQ48C* NextInBin() { return (CRezItmHashByNameQ48C*)CBaseHashItemQ48C::NextInBin(); }

	CRezItmQ48C* GetRezItm() { return m_pRezItm; }
	void SetRezItm(CRezItmQ48C* pRezItm) { m_pRezItm = pRezItm; }

protected:
	// 0x00490030, and src/lithshared/rezmgr/rezdir.cpp already defines and
	// matches it.  Declared here and deliberately NOT defined, so the two
	// objects cannot both emit it.
	virtual unsigned int HashFunc();                                // 0x00490030

private:
	friend class CRezItmQ48C;
	CRezItmQ48C* m_pRezItm;                  // +0x14
};

class CRezDirHashQ48C : public CBaseHashItemQ48C
{
public:
	CRezDirHashQ48C* Next()      { return (CRezDirHashQ48C*)CBaseHashItemQ48C::Next(); }
	CRezDirHashQ48C* NextInBin() { return (CRezDirHashQ48C*)CBaseHashItemQ48C::NextInBin(); }

	CRezDirQ48C* GetRezDir() { return m_pRezDir; }

protected:
	virtual unsigned int HashFunc();                                // 0x00490170

private:
	friend class CRezDirQ48C;
	CRezDirQ48C* m_pRezDir;                  // +0x14
};

class CRezTypeHashQ48C : public CBaseHashItemQ48C
{
public:
	CRezTypeHashQ48C* Next()      { return (CRezTypeHashQ48C*)CBaseHashItemQ48C::Next(); }
	CRezTypeHashQ48C* NextInBin() { return (CRezTypeHashQ48C*)CBaseHashItemQ48C::NextInBin(); }

	CRezTypQ48C* GetRezTyp() { return m_pRezTyp; }

protected:
	virtual unsigned int HashFunc();                                // 0x00490110

private:
	friend class CRezTypQ48C;
	CRezTypQ48C* m_pRezTyp;                  // +0x14
};


// -- the four hash TABLES (each is CBaseHash, 8 bytes) ------------------ //

class CRezItmHashTableByNameQ48C : public CBaseHashQ48C
{
public:
	CRezItmQ48C* Find(REZNAME_Q48C sName, BOOL_Q48C bIgnoreCase);   // 0x00490050
	unsigned int HashFunc(REZNAME_Q48C sName);                      // 0x00490180
	CRezItmHashByNameQ48C* GetFirst() { return (CRezItmHashByNameQ48C*)CBaseHashQ48C::GetFirst(); }
};

class CRezItmHashTableByIDQ48C : public CBaseHashQ48C
{
public:
	CRezItmQ48C* Find(DWORD_Q48C nID);
};

class CRezDirHashTableQ48C : public CBaseHashQ48C
{
public:
	CRezDirQ48C* Find(REZDIRNAME_Q48C sName, BOOL_Q48C bIgnoreCase); // folds onto 0x00490050
	unsigned int HashFunc(REZDIRNAME_Q48C sName);                    // folds onto 0x00490180
	CRezDirHashQ48C* GetFirst() { return (CRezDirHashQ48C*)CBaseHashQ48C::GetFirst(); }
};

class CRezTypeHashTableQ48C : public CBaseHashQ48C
{
public:
	CRezTypQ48C* Find(REZTYPE_Q48C nType);                          // 0x00490130
	unsigned int HashFunc(REZTYPE_Q48C nType);                      // 0x00490120
	CRezTypeHashQ48C* GetFirst() { return (CRezTypeHashQ48C*)CBaseHashQ48C::GetFirst(); }
};


// -- the owners -------------------------------------------------------- //

// CRezItm, 0x3c bytes.  m_heName at +0x1c is read by 0x0048e704
// (`lea 0x1c(%eax),%ecx`); 0x3c is the stride of CRezMgr::AllocateRezItm's
// `new CRezItm[n]` loop (`add $0x3c,%edi` at 0x0048ff70).
class CRezItmQ48C
{
public:
	REZNAME_Q48C GetName() { return m_sName; }

public:
	REZNAME_Q48C          m_sName;           // +0x00
	CRezTypQ48C*          m_pType;           // +0x04
	DWORD_Q48C            m_nTime;           // +0x08
	DWORD_Q48C            m_nSize;           // +0x0c
	CRezDirQ48C*          m_pParentDir;      // +0x10
	DWORD_Q48C            m_nFilePos;        // +0x14
	DWORD_Q48C            m_nCurPos;         // +0x18
	CRezItmHashByNameQ48C m_heName;          // +0x1c  (0x0048e704)
	void*                 m_pRezFile;        // +0x34
	unsigned char*        m_pData;           // +0x38
};

// CRezTyp, 0x30 bytes -- and the 0x30 is not inferred, it is the
// `push $0x30` CRezDir::GetOrMakeTyp hands operator new at 0x0048ecb3.
// m_heType at +0x04 is 0x0048e6c4, m_haName at +0x24 is 0x0048e6e4.
class CRezTypQ48C
{
public:
	REZTYPE_Q48C GetType() { return m_nType; }

public:
	REZTYPE_Q48C               m_nType;      // +0x00  (0x00490116, 0x0049014d)
	CRezTypeHashQ48C           m_heType;     // +0x04  (0x0048e6c4)
	CRezItmHashTableByIDQ48C   m_haID;       // +0x1c
	CRezItmHashTableByNameQ48C m_haName;     // +0x24  (0x0048e6e4)
	CRezDirQ48C*               m_pParentDir; // +0x2c
};

// CRezDir, 0x4c bytes.  m_pRezMgr +0x18 (0x0048e63f), m_heDir +0x20
// (0x0048e684), m_haDir +0x38 (0x0048e660, 0x0048e64e), m_haTypes +0x40
// (0x0048e6a0, 0x0048e518).
class CRezDirQ48C
{
public:
	REZDIRNAME_Q48C GetDirName() { return m_sDirName; }

	CRezDirQ48C* GetDir(REZDIRNAME_Q48C sDir);                      // 0x0048e630
	CRezDirQ48C* GetFirstSubDir();                                  // 0x0048e660
	CRezDirQ48C* GetNextSubDir(CRezDirQ48C* pRezDir);               // 0x0048e680
	CRezTypQ48C* GetFirstType();                                    // 0x0048e6a0
	CRezTypQ48C* GetNextType(CRezTypQ48C* pRezType);                // 0x0048e6c0
	CRezItmQ48C* GetFirstItem(CRezTypQ48C* pRezType);               // 0x0048e6e0
	CRezItmQ48C* GetNextItem(CRezItmQ48C* pRezItm);                 // 0x0048e700

public:
	REZDIRNAME_Q48C        m_sDirName;           // +0x00
	DWORD_Q48C             m_nDirPos;            // +0x04
	DWORD_Q48C             m_nDirSize;           // +0x08
	DWORD_Q48C             m_nItemsPos;          // +0x0c
	DWORD_Q48C             m_nItemsSize;         // +0x10
	DWORD_Q48C             m_nLastTimeModified;  // +0x14
	CRezMgrQ48C*           m_pRezMgr;            // +0x18
	CRezDirQ48C*           m_pParentDir;         // +0x1c
	CRezDirHashQ48C        m_heDir;              // +0x20
	CRezDirHashTableQ48C   m_haDir;              // +0x38
	CRezTypeHashTableQ48C  m_haTypes;            // +0x40
	unsigned char*         m_pMemBlock;          // +0x48
};

// CRezMgr -- only m_bLowerCaseUsed at +0x68 is reached from this file
// (0x0048e644).  rezmgr.h declares the accessor in-class at line 363 and
// that spelling is deliberate here; see the note on GetDir below.
class CRezMgrQ48C
{
public:
	BOOL_Q48C GetLowerCaseUsed() { return m_bLowerCaseUsed; }
	void      SetLowerCaseUsed(BOOL_Q48C b) { m_bLowerCaseUsed = b; }

public:
	unsigned char m_pad_0000[0x68];          // +0x00
	BOOL_Q48C     m_bLowerCaseUsed;          // +0x68
	BOOL_Q48C     m_bItemByIDUsed;           // +0x6c
};


// ======================================================================= //
//  0x00490110  CRezTypeHash::HashFunc          15 bytes + 1 pad, `ret`
//
//  Shipped bytes (0x00490110..0x0049011e):
//      8b 41 14     mov 0x14(%ecx),%eax      m_pRezTyp
//      8b 49 0c     mov 0xc(%ecx),%ecx       GetParentHash()
//      8b 00        mov (%eax),%eax          m_pRezTyp->m_nType  (+0x00)
//      50           push %eax
//      e8 02 ..     call 0x00490120          CRezTypeHashTable::HashFunc
//      c3           ret
//
//  WHAT PINS IT
//    * bare `ret`, `this` in %ecx, no arguments -> a thiscall no-argument
//      member, and the only such member the base declares is the pure
//      virtual HashFunc().  (It is an override, so it is also what the
//      vtable slot needs.)
//    * NO NULL GUARD, unlike the already-matching CRezItmHashByName::
//      HashFunc at 0x00490030, which opens `mov 0x14(%ecx),%eax ; test ;
//      jne ; ret`.  The CRezItm back pointer can be null (the free list
//      holds elements whose owner is not set yet -- CRezMgr::AllocateRezItm
//      tests exactly that at 0x0048ff1e); the type element's cannot, and the
//      source does not test it.
//    * `mov (%eax),%eax` feeding 0x00490120, which is an unsigned modulo on
//      its argument, makes the key CRezTyp::m_nType and not a string.
//    * the callee's %eax is returned untouched -> `return ...`.
// ======================================================================= //

unsigned int CRezTypeHashQ48C::HashFunc()
{
	return ((CRezTypeHashTableQ48C*)GetParentHash())->HashFunc(m_pRezTyp->GetType());
}


// ======================================================================= //
//  0x00490120  CRezTypeHashTable::HashFunc(REZTYPE)   13 bytes + 3 pad,
//                                                     `ret $4`
//
//  Shipped bytes (0x00490120..0x0049012c):
//      8b 44 24 04  mov 0x4(%esp),%eax       nType
//      33 d2        xor %edx,%edx
//      f7 31        divl (%ecx)              m_nNumBins, read in place
//      8b c2        mov %edx,%eax            the REMAINDER
//      c2 04 00     ret $0x4
//
//  WHAT PINS IT
//    * `ret $4` with `this` in %ecx -> thiscall, one dword argument.
//    * `div` and not `idiv` -> both operands unsigned; rezmgr.h has
//      REZTYPE as `unsigned long` and GetNumBins() as `unsigned int`.
//    * the divisor is `(%ecx)`, i.e. `this + 0x00`, which is CBaseHash's
//      m_nNumBins -- GetNumBins() is in-class in basehash.h and expands.
//    * `mov %edx,%eax` takes the remainder, so the operator is `%`.
//    * there is NO null guard: a type is a number.
// ======================================================================= //

unsigned int CRezTypeHashTableQ48C::HashFunc(REZTYPE_Q48C nType)
{
	return (nType % GetNumBins());
}


// ======================================================================= //
//  0x00490130  CRezTypeHashTable::Find(REZTYPE)   62 bytes + 2 pad,
//                                                 `ret $4`
//
//  VERIFIED CALL SEQUENCE (2, in address order):
//    0x00490139  0x00490120  CRezTypeHashTable::HashFunc(REZTYPE)
//    0x00490141  0x004a0590  CBaseHash::GetFirstInBin
//
//  Shipped, annotated:
//      490130  push %esi ; push %edi
//      490132  mov 0xc(%esp),%edi           nType parked in %edi
//      490136  mov %ecx,%esi                this
//      490138  push %edi ; call 0x490120    HashFunc(nType)
//      49013e  push %eax ; mov %esi,%ecx
//      490141  call 0x4a0590                GetFirstInBin(...)
//      490146  test %eax,%eax ; je 0x49015f       -> the `return NULL` tail
//      49014a  mov 0x14(%eax),%ecx          pElem->m_pRezTyp
//      49014d  cmp %edi,(%ecx)              pElem->GetRezTyp()->GetType() == nType
//      49014f  je 0x490166
//      490151  mov 0x4(%eax),%eax           NextInBin(): the list link
//      490154  test %eax,%eax ; je 0x49015f
//      490158  add $0xfffffffc,%eax         CBaseListItem* -> CBaseHashItem*
//      49015b  test %eax,%eax ; jne 0x49014a
//      49015f  pop %edi ; xor %eax,%eax ; pop %esi ; ret $4
//      490166  mov 0x14(%eax),%eax ; pop %edi ; pop %esi ; ret $4
//
//  WHAT PINS IT
//    * this is CRezItmHashTableByName::Find (0x00490050, already matching in
//      rezdir.cpp) with the string compare replaced by an integer one and
//      with the bIgnoreCase fork and the null-name guard gone.  The walk is
//      instruction-for-instruction the same, including the -4 base
//      adjustment and its null test, so it is written the same way.
//    * ONE `xor %eax,%eax ; ret` shared by the entry test and the bottom
//      test -> the rotated `while` with a single trailing `return NULL;`.
//      (0x00490050 has TWO copies of that block because its source has two
//      loops, one per arm of the bIgnoreCase fork.)
//    * `cmp %edi,(%ecx)` is `cmp DWORD PTR [ecx],edi` in Intel order, so the
//      ELEMENT's type is the left operand and nType the right -- the same
//      operand order 0x00490050's two compares use.
//    * the found arm reloads +0x14 rather than reusing %ecx, which is
//      `return pElem->GetRezTyp();` written out again rather than cached.
// ======================================================================= //

CRezTypQ48C* CRezTypeHashTableQ48C::Find(REZTYPE_Q48C nType)
{
	CRezTypeHashQ48C* pElem;

	pElem = (CRezTypeHashQ48C*)GetFirstInBin(HashFunc(nType));

	while (pElem != NULL_Q48C)
	{
		if (pElem->GetRezTyp()->GetType() == nType)
			return pElem->GetRezTyp();

		pElem = pElem->NextInBin();
	}

	return NULL_Q48C;
}


// ======================================================================= //
//  0x00490170  CRezDirHash::HashFunc           15 bytes + 1 pad, `ret`
//
//  Shipped bytes (0x00490170..0x0049017e) -- byte for byte the same shape as
//  0x00490110 above, with the callee changed:
//      8b 41 14     mov 0x14(%ecx),%eax      m_pRezDir
//      8b 49 0c     mov 0xc(%ecx),%ecx       GetParentHash()
//      8b 00        mov (%eax),%eax          m_pRezDir->m_sDirName  (+0x00)
//      50           push %eax
//      e8 02 ..     call 0x00490180          CRezDirHashTable::HashFunc
//      c3           ret
//
//  It calls the STRING hash, which is what makes this the DIRECTORY element
//  and 0x00490110 the type element -- see the header note.  No null guard
//  here either: a CRezDirHash is only ever in the table while its CRezDir
//  owns it (CRezDir::CRezDir stores the back pointer before inserting).
// ======================================================================= //

unsigned int CRezDirHashQ48C::HashFunc()
{
	return ((CRezDirHashTableQ48C*)GetParentHash())->HashFunc(m_pRezDir->GetDirName());
}


// ======================================================================= //
//  0x00490180  CRezItmHashTableByName::HashFunc(REZNAME)   41 bytes + 7 pad,
//                                                          `ret $4`
//              CRezDirHashTable::HashFunc(REZDIRNAME)      (folds onto it)
//
//  Shipped, annotated:
//      490180  mov 0x4(%esp),%edx           sName
//      490184  test %edx,%edx ; jne 0x49018d
//      490188  xor %eax,%eax ; ret $4       <- `if (s == NULL) return 0;`
//      49018d  push %ebx
//      49018e  mov (%edx),%bl               the peeled first character
//      490190  xor %eax,%eax                nLen = 0
//      490192  test %bl,%bl ; je 0x49019f
//      490196  mov 0x1(%edx),%bl            the NEXT character
//      490199  inc %eax                     nLen++
//      49019a  inc %edx                     pCur++
//      49019b  test %bl,%bl ; jne 0x490196
//      49019f  xor %edx,%edx
//      4901a1  pop %ebx
//      4901a2  divl (%ecx)                  m_nNumBins, read in place
//      4901a4  mov %edx,%eax                the REMAINDER
//      4901a6  ret $0x4
//
//  WHAT PINS IT
//    * THIS IS NOT strlen().  The library is compiled /Oi -- 0x00490050
//      inlines strcmp and CRezDir::CreateDir emits `repnz scasb` for strlen
//      at 0x0048e7c7 -- so a source `strlen(sName)` here would have been a
//      `repnz scasb` too.  A byte-at-a-time loop that increments a counter
//      and a cursor separately is a hand-written length loop and nothing
//      else.
//    * the load of the SECOND character at the top of the loop body with the
//      first peeled out above it is VC6's rotation of `while (*pCur)`, and
//      `inc %eax` before `inc %edx` keeps the two statements in source
//      order.
//    * the guard's `xor %eax,%eax ; ret $4` is its own block that the `jne`
//      jumps over -> a leading `if (sName == NULL) return 0;` (the same
//      shape CRezItmHashByName::HashFunc at 0x00490030 uses, which matches).
//    * `divl` again -> unsigned, and again `this + 0x00` for the divisor.
//
//  COIN-FLIP, RECORDED NOT SETTLED: whether the cursor is a separate local
//  (`char* pCur = sName;`) or the parameter itself (`while (*sName) { nLen++;
//  sName++; }`).  Both put the pointer in %edx and both give these bytes on
//  every VC6 build considered; the separate local is written because
//  Monolith's other loops in this library do not scribble on parameters.
// ======================================================================= //

unsigned int CRezItmHashTableByNameQ48C::HashFunc(REZNAME_Q48C sName)
{
	unsigned int nLen;
	char*        pCur;

	if (sName == NULL_Q48C) return 0;

	nLen = 0;
	pCur = sName;

	while (*pCur != '\0')
	{
		nLen++;
		pCur++;
	}

	return (nLen % GetNumBins());
}


// The directory table's HashFunc is the same source over REZDIRNAME, and
// REZDIRNAME is the same `char*`, so the two compile to identical bytes and
// the linker folds them onto 0x00490180 -- exactly as CRezDirHashTable::Find
// folds onto CRezItmHashTableByName::Find at 0x00490050 (rezdir.cpp already
// writes that pair out for the same reason).  Written because it is real
// source; it will not produce a second function in the image, and it
// will match at 0x00490180 as a second claimant.

unsigned int CRezDirHashTableQ48C::HashFunc(REZDIRNAME_Q48C sName)
{
	unsigned int nLen;
	char*        pCur;

	if (sName == NULL_Q48C) return 0;

	nLen = 0;
	pCur = sName;

	while (*pCur != '\0')
	{
		nLen++;
		pCur++;
	}

	return (nLen % GetNumBins());
}


// *********************************************************************** //
//
//  SEVEN MORE, AND THEY ARE NOT IN THE ENUMERATION BECAUSE THEY ARE
//  FILED UNDER THE WRONG MODULE.
//
//  0x0048e630 and 0x0048e660..0x0048e716 sit in the MIDDLE of the enumerated
//  range, between two rows that are rezmgr.lib, and they are rezmgr.lib too
//  -- all seven have byte-identical twins in server.dll (0x1003ac50,
//  0x1003ac80, 0x1003aca0, 0x1003acc0, 0x1003ace0, 0x1003ad00, 0x1003ad20,
//  disassembled and compared instruction by instruction).  Today:
//
//    * src/engine/lithtech/lt2new4.cpp carries 0x0048e630 as
//      "CUnknown48E630::FindByName" and is a NEAR (ours 41, shipped 42).
//    * src/engine/lithtech/ltnew1.cpp carries four of them as
//      "CSub48e680Owner::GetValue20/04/24/1c" and they MATCH -- but only in
//      lithtech.exe, because that file is a client TU and never reaches
//      server.dll.
//    * 0x0048e660 and 0x0048e6a0 are in no source file at all.
//
//  Writing them here costs lithtech.exe nothing (a byte match reports the first
//  hit; the address is already counted) and is worth up to seven functions
//  in server.dll plus two in lithtech.exe.
//
//  WHAT THEY ARE.  rezmgr.h declares, in this order,
//      GetFirstSubDir, GetNextSubDir, GetRezTyp, GetFirstType, GetNextType,
//      GetFirstItem, GetNextItem
//  and the six bodies come out in exactly that address order with GetRezTyp
//  missing.  Each one's `lea` displacement names the member it walks, and
//  each displacement is a member of a DIFFERENT class, which is what fixes
//  the argument types:
//
//    0x0048e660  add $0x38,%ecx   this->m_haDir     GetFirst -> CRezDir*
//    0x0048e680  lea 0x20(%eax)   CRezDir::m_heDir   Next    -> CRezDir*
//    0x0048e6a0  add $0x40,%ecx   this->m_haTypes    GetFirst -> CRezTyp*
//    0x0048e6c0  lea 0x04(%eax)   CRezTyp::m_heType  Next    -> CRezTyp*
//    0x0048e6e0  lea 0x24(%eax)   CRezTyp::m_haName  GetFirst -> CRezItm*
//    0x0048e700  lea 0x1c(%eax)   CRezItm::m_heName  Next    -> CRezItm*
//
//  (0x004a0560 is CBaseHash::GetFirst and 0x004a03d0 is CBaseHashItem::Next
//  -- the two the sibling files already name.)  Every one then does
//  `test %eax,%eax ; jne` over a bare `ret`, which is the vanishing null
//  test: %eax already holds the zero the compare just proved, so the source
//  `return NULL;` costs no `xor`.  ltnew1.cpp's four matched with exactly
//  this spelling over its invented classes, so the shape is measured, not
//  guessed; all that changes here is that the classes are the real ones.
//
// *********************************************************************** //

CRezDirQ48C* CRezDirQ48C::GetFirstSubDir()
{
	CRezDirHashQ48C* pElem = m_haDir.GetFirst();

	if (pElem == NULL_Q48C)
		return NULL_Q48C;

	return pElem->GetRezDir();
}


CRezDirQ48C* CRezDirQ48C::GetNextSubDir(CRezDirQ48C* pRezDir)
{
	CRezDirHashQ48C* pElem = pRezDir->m_heDir.Next();

	if (pElem == NULL_Q48C)
		return NULL_Q48C;

	return pElem->GetRezDir();
}


CRezTypQ48C* CRezDirQ48C::GetFirstType()
{
	CRezTypeHashQ48C* pElem = m_haTypes.GetFirst();

	if (pElem == NULL_Q48C)
		return NULL_Q48C;

	return pElem->GetRezTyp();
}


CRezTypQ48C* CRezDirQ48C::GetNextType(CRezTypQ48C* pRezType)
{
	CRezTypeHashQ48C* pElem = pRezType->m_heType.Next();

	if (pElem == NULL_Q48C)
		return NULL_Q48C;

	return pElem->GetRezTyp();
}


CRezItmQ48C* CRezDirQ48C::GetFirstItem(CRezTypQ48C* pRezType)
{
	CRezItmHashByNameQ48C* pElem = pRezType->m_haName.GetFirst();

	if (pElem == NULL_Q48C)
		return NULL_Q48C;

	return pElem->GetRezItm();
}


CRezItmQ48C* CRezDirQ48C::GetNextItem(CRezItmQ48C* pRezItm)
{
	CRezItmHashByNameQ48C* pElem = pRezItm->m_heName.Next();

	if (pElem == NULL_Q48C)
		return NULL_Q48C;

	return pElem->GetRezItm();
}


// ======================================================================= //
//  0x0048e630  CRezDir::GetDir(REZCDIRNAME)    42 bytes + 2 pad, `ret $4`
//
//  Shipped, annotated:
//      48e630  push %esi
//      48e631  mov 0x8(%esp),%esi          sDir, parked in a SAVED register
//      48e635  test %esi,%esi ; jne 0x48e63f
//      48e639  xor %eax,%eax ; pop %esi ; ret $4      <- `return NULL;`
//      48e63f  mov 0x18(%ecx),%eax         m_pRezMgr
//      48e642  xor %edx,%edx
//      48e644  mov 0x68(%eax),%eax         m_bLowerCaseUsed  (dest == base)
//      48e647  test %eax,%eax
//      48e649  sete %dl                    the ! , materialised
//      48e64c  push %edx ; push %esi
//      48e64e  add $0x38,%ecx              &m_haDir
//      48e651  call 0x00490050             CRezDirHashTable::Find
//      48e656  pop %esi ; ret $4
//
//  THIS IS A DELIBERATE EXPERIMENT, and it is the cheapest test of the
//  hypothesis in appendix item 8.  lt2new4.cpp already has this function,
//  written with the raw member access, and it comes out 41 bytes against 42
//  with the flag in a callee-saved register and the argument in %eax (so its
//  null arm needs no `xor %eax,%eax` and shipped's does).  The version below
//  is the same source with ONE token changed -- `GetLowerCaseUsed()`, the
//  in-class accessor rezmgr.h declares at line 363, instead of the raw
//  `m_bLowerCaseUsed`.  If this body comes out byte-identical and
//  lt2new4.cpp's does not, the same edit is worth making at the other four
//  sites (0x0048e510, 0x0048e747, 0x0048ea38 -- see appendix item 8).  If it
//  comes out 41 bytes too, the accessor is a red herring and the whole
//  family is allocation-side, which is equally worth knowing and costs one
//  build to learn.
//
//  Either way this body belongs in rezmgr.lib and lt2new4.cpp's copy should
//  go once this one lands -- a client TU cannot score it in server.dll.
// ======================================================================= //

CRezDirQ48C* CRezDirQ48C::GetDir(REZDIRNAME_Q48C sDir)
{
	if (sDir == NULL_Q48C)
		return NULL_Q48C;

	return m_haDir.Find(sDir, !m_pRezMgr->GetLowerCaseUsed());
}



// *********************************************************************** //
//
//  APPENDIX -- THE TWELVE ENUMERATED ROWS, WHICH ARE ALL EXISTING NEARS.
//
//  Nothing below is compiled.  Each entry says where the function lives,
//  what the residual is, and (where there is one) the exact source edit to
//  make.  Sizes are ours-vs-shipped, measured
//  against the retail lithtech.exe.
//
//  Every one of these scores TWICE when it lands -- rezmgr.lib is linked
//  into lithtech.exe and server.dll both.
//
// ----------------------------------------------------------------------- //
//  1.  0x0048e8c0  CRezDir::ReadAllDirs   ours 166 / shipped 177
//      src/lithshared/rezmgr/rezmgr2.cpp, the body after the comment block
//      headed "0x0048e8c0  CRezDir::ReadAllDirs".
//
//      THE ONE TO MAKE FIRST.  The whole 11-byte residual is one
//      inverted guard, and it is the trailing-statement rule verbatim.
//
//      Shipped:  0x0048e914  test %eax,%eax
//                0x0048e916  je   0x0048e95e          <- FORWARD, past every
//                                                        other return
//                0x0048e918  ..the second hash walk..
//                0x0048e95e  movl $0x0,0x10(%esp)     <- the else arm, parked
//                0x0048e966  mov  0x10(%esp),%eax     <- the shared return
//                0x0048e96a  pop %edi ; pop %esi ; pop %ebp ; pop %ebx ;
//                            pop %ecx ; ret $0x10
//
//      So `ReadDirBlock(...)` is tested POSITIVELY, the loop is the
//      fall-through, and `bRetVal = FALSE;` is an else block hoisted to the
//      tail.  Ours tests it negatively and VC6 turned the `bRetVal = FALSE;`
//      arm into an immediate `return 0` (it already had the zero in %eax
//      from the `test`), which is the 11 bytes.
//
//      REPLACE
//            if (!ReadDirBlock(pRezFile, Pos, Size, bOverwriteItems))
//            {
//                    bRetVal = FALSE_X720;
//            }
//            else
//            {
//                    // ...the second walk...
//            }
//      WITH
//            if (ReadDirBlock(pRezFile, Pos, Size, bOverwriteItems))
//            {
//                    // ...the second walk, UNCHANGED...
//            }
//            else
//            {
//                    bRetVal = FALSE_X720;
//            }
//
//      There is a second, dependent difference that should fall out with it
//      and is worth checking rather than chasing: shipped issues `push %edi`
//      INSIDE the `if (Size > 0)` block (0x0048e8dd) and lets the Size<=0
//      exit enter the epilogue five bytes late, at 0x0048e96b, skipping both
//      `mov 0x10(%esp),%eax` and `pop %edi` -- it knows %eax is still the 1
//      it stored.  That second entry point only exists because the else arm
//      above it created a separate producer for 0x0048e966.  Ours pushes
//      %edi in the prologue and has one epilogue.  Expect both to move
//      together; if the push does not move, the row is +0 and the conclusion is
//      that the guard was necessary but not sufficient.
//
// ----------------------------------------------------------------------- //
//  2.  0x0048fd00  CRezDir::GetRezFromDosPath   ours 370 / shipped 370
//      src/lithshared/rezmgr/rezdir.cpp.
//
//      TWO BYTES, and they are both in the inlined IsGoodChar():
//        shipped 0x0048fda2   7e bc   jle 0x0048fd60   ; "good" -> skip
//                0x0048fda4   eb b8   jmp 0x0048fd5e   ; "bad"  -> sPath++
//        ours    +0xa2        7f ba   jg  +0x5e        ; "bad"
//                +0xa4        eb ba   jmp +0x60        ; "good"
//      Same three instructions, same ten bytes, opposite successor on the
//      conditional.  Everything else in the function is byte-identical,
//      including the block ADDRESSES, so nothing local distinguishes the two
//      builds.
//
//      Five copies of IsGoodChar are inlined across the library and the
//      shipped build does not pick one form:
//        0x0048fbdd  jle good / jmp bad   (both targets backward)  WE DIFFER
//        0x0048fbff  jg  bad  / jmp good  (both forward)           we agree
//        0x0048fcdf  jle good / jmp bad   (good is +2)             we agree
//        0x0048fda2  jle good / jmp bad   (both backward)          WE DIFFER
//        0x0048fdc4  jg  bad  / jmp good  (both forward)           we agree
//      The two we get wrong are exactly the two where BOTH successors are
//      backward branches, i.e. the free tie-break, and they are the two
//      `if ((nLen > 1) && (!IsGoodChar(p[0])))` leading-separator tests.
//
//      rezdir.cpp's own note says "do not chase it by rewriting IsGoodChar
//      -- that helper is inlined into functions of this file that already
//      match".  THAT PREMISE IS WRONG TODAY: all three functions that inline
//      IsGoodChar (0x0048fb30, 0x0048fd00, 0x0048f380 -- and 0x0048f380
//      actually inlines none, there is no `cmp $0x7a` in it) are UNMATCHED,
//      so an experiment on IsGoodChar costs nothing and 0x0048fd00 is two
//      bytes from matching in both images.  Three spellings worth one build
//      each, in the order to try them:
//        (a) nest the conjunction at the call site --
//              if (nLen > 1) { if (!IsGoodChar(pPath[0])) { pPath++; nLen--; } }
//            -- which changes the block the `nLen > 1` test targets and is
//            the only edit that touches ONLY the two failing sites;
//        (b) write IsGoodChar as four `if (...) return TRUE;` statements and
//            a trailing `return FALSE;` instead of one `||` chain.  That
//            gives the last range an explicit "true" successor and a
//            fall-through "false", which is the shipped shape at 0x0048fda2
//            -- but it touches all five copies, so measure it whole;
//        (c) `(IsGoodChar(pPath[0]) == FALSE_X380)` instead of `!`.
//      Measure per edit and against BOTH images.
//
// ----------------------------------------------------------------------- //
//  3.  0x0048fb30  CRezDir::GetDirFromPath   ours 445 / shipped 458
//      src/lithshared/rezmgr/rezdir.cpp.
//
//      13 bytes = one whole duplicated epilogue (15) less the `jmp` (2) we
//      emit instead.  Shipped ends the trailing skip loop with TWO identical
//      fifteen-byte `return pDir;` epilogues, at 0x0048fc76 and 0x0048fcb2,
//      and FALLS INTO each: `test %al,%al ; jne 0x0048fc85` at both
//      0x0048fc74 and 0x0048fcb0, with the return as the fall-through.  Ours
//      emits `je <shared exit>` at both and one epilogue at the end.
//
//      The current source already spells two `return pDir;` statements (a
//      `if (pPath[i] == '\0') return pDir;` guard and a `return pDir;` after
//      a do/while), and rezdir.cpp's note says the plain `while` form was
//      considered and rejected -- but the do/while form it chose produces
//      the plain-`while` codegen anyway, which is what our object shows.  So
//      the two spellings that have been reasoned about both lose; VC6 is
//      canonicalising `if (x) return; do {..} while (x); return;` into one
//      rotated loop with one exit.
//
//      No third spelling has been found, so this is honestly open.
//      The one thing the bytes add over rezdir.cpp's note: the block ORDER
//      differs too -- shipped lays out [entry test][return pDir][loop top]
//      [bottom test][return pDir][IsGoodChar ranges][recurse], ours lays out
//      [entry test][loop top][bottom test][ranges][recurse][return pDir],
//      i.e. we treat `return pDir` as cold and sink it.  Anything that makes
//      that block hot -- for instance giving the two exits DIFFERENT
//      statements so they cannot be merged at all -- is the direction.
//      The 2-byte IsGoodChar difference above is also present here and will
//      not close this function on its own.
//
// ----------------------------------------------------------------------- //
//  4.  0x0048e720  CRezDir::CreateDir   ours 244 / shipped 206
//      src/lithshared/rezmgr/rezmgr2.cpp.
//
//      38 bytes, and the MECHANISM is now pinned even though the lever is
//      not.  Shipped has ONE exit (0x0048e7d9, the 19-byte
//      `mov 0x10(%esp),%ecx ; pop ; pop ; pop ; mov %ecx,%fs:0x0 ; pop ;
//      add $0xc,%esp ; ret $4` unwind epilogue) with three producers:
//      `xor %eax,%eax ; jmp` at 0x0048e75c, the same at 0x0048e7b0, and
//      `mov %ebx,%eax` at 0x0048e7d7 falling in.  All three of OUR returns
//      carry their own copy of that epilogue.
//
//      Why: VC6 scheduled our `mov %ebx,%eax` INTO THE MIDDLE of the
//      epilogue (ours reads `mov 0x10(%esp),%ecx ; pop %edi ; pop %esi ;
//      mov %ebx,%eax ; pop %ebp ; ...`), which makes that copy textually
//      unlike the other two and defeats the cross-jump that would have
//      merged them.  Shipped keeps `mov %ebx,%eax` strictly ABOVE the
//      epilogue, so all three copies are identical and merge.  Everything
//      else in the function is instruction-for-instruction identical.
//
//      That is allocation-side (not source-reachable), and
//      it is the same signature already retired elsewhere as "nine
//      residuals".  The mechanism is recorded here rather than an edit
//      proposed: the useful next probe is anything that stops %ebx being the
//      home of pDir at the return -- see item 8 below, which is the only
//      source-level candidate found for the register choice in this file.
//
// ----------------------------------------------------------------------- //
//  5.  0x0048ec80  CRezDir::GetOrMakeTyp   ours 198 / shipped 194
//      src/lithshared/rezmgr/rezmgr2.cpp.
//
//      Allocation, three linked symptoms, one cause:
//        * shipped SPLITS nothing: `pTyp` lives in %edi from the Find result
//          (0x0048ecaa `mov %eax,%edi`) all the way to `mov %edi,%eax` at
//          0x0048ed2b.  Ours tests %eax directly and only later parks the
//          merged result in %esi, i.e. two live ranges for one variable.
//        * because shipped's %edi is provably zero after the `jne`, it
//          spends it on the EH state store -- `mov %edi,0x18(%esp)` at
//          0x0048ecca where ours needs `movl $0x0,0x14(%esp)`.  That is the
//          3 bytes.
//        * shipped therefore needs FOUR callee-saved registers and pushes
//          %ebp; ours needs three.
//      The source is already the single-variable, single-trailing-`return
//      pTyp;` form the shape asks for.  Nothing source-reachable found.
//
// ----------------------------------------------------------------------- //
//  6.  0x0048ef80  CRezMgr::Open   ours 1022 / shipped 1018
//      src/lithshared/rezmgr/rezmgr2.cpp.
//
//      FOUR bytes, two `mov %edi,%eax` we emit and shipped does not.
//      Shipped materialises the return constant twice: `mov $0x1,%eax` at
//      0x0048f19a for `m_bFileOpened = TRUE;` (reused three instructions
//      later for `m_bMustReWriteDirs = TRUE;` at 0x0048f1af) and a SECOND
//      `mov $0x1,%eax` at 0x0048f35a in the shared exit for `return TRUE;`.
//      Ours keeps the 1 in %edi across the whole function and copies it to
//      %eax at each producer.  Constant rematerialisation; allocation-side.
//      Three `return TRUE;` statements and one shared exit are already what
//      the source says, and the other 1014 bytes are identical.
//
// ----------------------------------------------------------------------- //
//  7.  0x0048f380  CRezMgr::ReadEmulationDirectory   ours 1399 / shipped 1400
//      src/lithshared/rezmgr/rezdir.cpp.  (True extent 1400 bytes,
//      0x0048f380..0x0048f8f5; the enumeration's 1,322 + the bogus 78-byte row
//      0x0048f8aa, which is this function's shared loop tail -- rezdir.cpp
//      already documents both.)
//
//      ONE byte over 468 instructions, and the whole difference is block
//      ORDER around the directory-walk loop: shipped puts the `cmp %ecx,%eax
//      ; jge` copy of the bound test at the TOP and the increment block
//      below it, ours rotates the other way and needs one extra `jmp`/`mov
//      $X,%eax` pair at the entry.  Loop rotation is not
//      source-reachable.  No statement-order change has been found
//      that flips it; nothing else in the function differs.
//
// ----------------------------------------------------------------------- //
//  8.  0x0048e510  CRezDir::GetRez   ours 55 / shipped 55
//      src/lithshared/rezmgr/rezmgr.cpp.
//
//      THREE INSTRUCTIONS, and this is the one place with a concrete
//      source candidate rather than a shrug.
//
//        shipped 0x0048e52d  mov 0x68(%ecx),%ecx   ; dest == base
//                0x0048e530  test %ecx,%ecx
//                0x0048e532  mov 0x8(%esp),%ecx    ; reuse after the test
//        ours    mov 0x68(%ecx),%esi
//                mov 0x8(%esp),%ecx
//                test %esi,%esi
//
//      The SAME idiom differs the same way at every site in the library that
//      reads CRezMgr::m_bLowerCaseUsed:
//        0x0048e52d  GetRez        ship %ecx->%ecx   ours %ecx->%esi
//        0x0048e644  GetDir        ship %eax->%eax   ours %edx->%esi
//        0x0048e747  CreateDir     ship %eax->%eax   ours %eax->%ebx
//        0x0048ea38  ReadDirBlock  ship %eax->%eax   ours %eax->%edx
//      In every shipped case the loaded flag OVERWRITES the pointer it was
//      loaded through and dies immediately; in every one of ours it goes to
//      a fresh register -- and at two of the four ours then needs a
//      callee-saved register it has to push and pop (%ebx in CreateDir and
//      in lt2new4.cpp's copy of GetDir) where shipped spends a volatile
//      %edx.  FOUR sites, one signature, one expression: this is one source
//      difference, not four allocation coincidences.  (The fourth is
//      lt2new4.cpp's ?FindByName@CUnknown48E630, i.e. CRezDir::GetDir --
//      which is why the version of GetDir written above uses the accessor:
//      it is the controlled test.)
//
//      THE CANDIDATE: src/LT2/lithshared/incs/rezmgr.h line 363 declares
//        BOOL GetLowerCaseUsed() { return m_bLowerCaseUsed; };
//      in-class, and all three sites spell the access as the raw member
//      `m_pRezMgr->m_bLowerCaseUsed` (legal -- CRezDir is a friend).  Try
//        m_haDir.Find(sDir, !m_pRezMgr->GetLowerCaseUsed())
//      at all three sites.  An expanded in-class accessor returns through
//      the function's own return temp, which is exactly the "dies in the
//      register it was produced in" shape shipped has.  This is a coin-flip
//      not settled without a build; it is cheap, it is one token, and
//      if it lands it moves GetRez outright and may unstick the register
//      choices in CreateDir (item 4) and ReadDirBlock (item 9).
//
// ----------------------------------------------------------------------- //
//  9.  0x0048e980  CRezDir::ReadDirBlock   ours 759 / shipped 759
//      src/lithshared/rezmgr/rezmgr2.cpp.  (True extent 759, not the
//      enumeration's 710 -- rezmgr2.cpp already corrected it.)
//
//      FIVE instructions out of 261, all in one straight-line block at
//      0x0048ea2c..0x0048ea46: shipped orders [mov %esi,%ebx][lea 0x38(%ebp),
//      %edi][mov 0x68(%eax),%eax][mov %edx,0x10(%esp)], ours orders
//      [mov %edx,0x10(%esp)][mov %esi,%ebx][mov 0x68(%eax),%edx][lea
//      0x38(%ebp),%edi].  Sinking our local store frees %edx early and the
//      allocator then spends it on the flag, which is item 8's signature
//      again.  Try item 8 here too.  Nothing else in 753 bytes differs.
//
// ----------------------------------------------------------------------- //
// 10.  0x0048ff00  CRezMgr::AllocateRezItm   ours 329 / shipped 259
//      src/lithshared/rezmgr/rezdir.cpp.  (True extent 259, not the
//      enumeration's 257, which cut `add $0x8,%esp` in half -- rezdir.cpp
//      already corrected it.)
//
//      The 70 bytes are /GX machinery around `new CRezItm[n]`: a three-dword
//      __CxxFrameHandler registration we emit and shipped does not, two
//      `movl $state,0x20(%esp)` stores bracketing the constructor loop, and
//      three `mov ; mov %ecx,%fs:0x0` unwind pops in the epilogues.  Two
//      bytes more are the SIB base/index roles in the seed loop.  rezdir.cpp
//      diagnosed all of this.
//
//      ONE MORE THING, and it is a negative worth recording so nobody
//      re-runs it: the "wrong compiler revision" escape does not apply.  The
//      __CxxFrameHandler FuncInfo magic is 0x19930520 in all 27 tables of
//      lithtech.exe AND in all five of our rezmgr objects, so the EH ABI is
//      the same; the Rich header carries no cl C++ entry other than builds
//      8168 and 8447 (the 7291/7299 rows are the CRT's masm and alias
//      objects -- the decoded Rich header says so and refutes the
//      VC5 reading); and 8168 has already been measured against 8447 across
//      all five files with identical per-file counts.  So the remaining
//      axes for this one function are the array-new SPELLING and nothing
//      else.  0x0048ff41..0x0048ff7e IS VC6's array-new expansion verbatim
//      (allocate, null-yields-null, `dec/test/jl/inc` counted ctor loop,
//      reload the spilled pointer, store), so the expansion is right and
//      only the wrapper is extra -- which leaves no visible source lever.
//      Leave it.
//
// ----------------------------------------------------------------------- //
// 11.  0x0048ec46  49 bytes  `init`   NOT A FUNCTION.
//      It is inside CRezDir::ReadDirBlock (0x0048e980..0x0048ec76), which is
//      759 bytes and not the 710 the enumeration carries.  0x0048ec46 is one of
//      that function's own late branch targets (0x0048ec46, 0x0048ec54 and
//      0x0048ec62 are all branched to from inside it) and the next real
//      function starts at 0x0048ec80.  rezmgr2.cpp documents this.
//
// 12.  0x0048f8aa  78 bytes  `init`   NOT A FUNCTION.
//      It is the shared loop tail of CRezMgr::ReadEmulationDirectory
//      (0x0048f380..0x0048f8f5, 1400 bytes) -- the `_findnext` at the bottom
//      of the directory scan, which six `continue`s inside the function
//      branch to.  rezdir.cpp documents this.
//
// ----------------------------------------------------------------------- //
//  THE SEVEN MIS-HOMED FUNCTIONS ARE WRITTEN ABOVE, NOT DESCRIBED HERE.
//  0x0048e630 and 0x0048e660..0x0048e716 are rezmgr.lib and are currently
//  filed in src/engine/lithtech/lt2new4.cpp ("CUnknown48E630::FindByName")
//  and src/engine/lithtech/ltnew1.cpp ("CSub48e680Owner::GetValue*").  See
//  the block headed "SEVEN MORE" in the body of this file; when they land
//  here, remove lt2new4.cpp's block 11 and ltnew1.cpp's CSub48e680Owner
//  section so one address is not claimed twice (0x0048e630 is not
//  ?FindByName@CUnknown48E630@@QAEPAXPBD@Z).
//
// *********************************************************************** //
