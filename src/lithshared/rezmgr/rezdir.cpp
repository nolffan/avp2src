// *********************************************************************** //
//
//  MODULE:   REZMGR (.CPP) + REZHASH (.CPP)
//            -- 0x0048f380..0x00490101 of lithtech.exe
//
//  WHAT THIS IS -- NOT RE-DERIVED, taken from the two sibling chunks that
//  landed today and re-checked against the shipped bytes where it mattered.
//
//  The run 0x0048df10..0x00490abd is Monolith's prebuilt rezmgr.lib.  Its
//  header is in this repository UNMODIFIED at src/LT2/lithshared/incs/rezmgr.h
//  and every signature below is copied out of it verbatim (it declares
//  ReadEmulationDirectory, StrToType, TypeToStr, IsDirectory, GetCurTime,
//  AllocateRezItm, DeAllocateRezItm, IsGoodChar and the rest by name).  The
//  collection headers are src/lithshared/lith/baselist.h and
//  src/lithshared/lith/basehash.h, also unmodified in the repo.  The library
//  is compiled LITE (REZMGRLITE), so every `#ifndef REZMGRLITE` member is
//  absent from the shipped layouts.
//
//  Two sibling files already carry the class layouts, independently derived
//  and in agreement with each other and with rezmgr.h:
//
//      src/lithshared/rezmgr/rezmgr.cpp   0x0048df30..0x0048e62a  (`Rz48`)
//      src/lithshared/rezmgr/rezmgr2.cpp  0x0048e720..0x0048f37a  (`X720`)
//
//  This file reuses their CRezItm (0x3c) / CRezTyp (0x30) / CRezDir (0x4c) /
//  CRezMgr (0xd4) offsets wholesale and re-derives none of them; every
//  offset this file actually touches is listed against the instruction that
//  reads it in the per-function comments.  Its types carry a THIRD suffix,
//  `X380`, so all three files can be compiled together.
//
//  THIS FILE'S RANGE IS TWO TRANSLATION UNITS, and the seam is visible:
//      0x0048f380..0x00490027   rezmgr.cpp   (14 functions below)
//      0x00490030..0x004901a0   rezhash.cpp  (2 functions below; the rest of
//                                             rezhash.cpp is past 0x00490101
//                                             and is left alone -- see the
//                                             note at the bottom of the file)
//
//  BOTH BINARIES SCORE THESE.  rezmgr.lib is linked into lithtech.exe and
//  server.dll, so each function below has a byte-identical twin in
//  server.dll.  Two of mine were already named against server.dll months ago
//  by src/engine/server/filemgr.cpp without knowing what they were.
//
//  IT NEEDS /GX.  One of the fourteen (ReadEmulationDirectory) opens with
//  VC6's three-dword __CxxFrameHandler registration and writes the unwind
//  state slot around its `new`; build/engine.sh's `lithtech)` recipe has no
//  /GX and would drop that frame.  The other thirteen are frameless.
//
// *********************************************************************** //
//
//  TRUE EXTENTS -- extents checked first.  BOTH misaligned rows
//  are enumeration defects, and they are two DIFFERENT defects:
//
//   * 0x0048f8aa (78 b) is NOT a function.  It is the shared loop tail of
//     0x0048f380: an extent check on 0x0048f380..0x0048f8aa reports seven branch
//     targets leaving the range, six of them landing exactly on 0x0048f8aa
//     (0x0048f4d6, 0x0048f510, 0x0048f619, 0x0048f63f, 0x0048f82d,
//     0x0048f84f -- every `continue` in the loop) and the seventh on
//     0x0048f8d9 inside it.  The row itself then branches BACKWARDS to
//     0x0048f4a2, the top of that loop (0x0048f8be).  The real function is
//     0x0048f380..0x0048f8f5 + 8 nops, 1,400 bytes, one `ret 0x10`.  It is
//     shape (b) of the catalogue -- a shared tail -- not a success arm.
//
//   * 0x0048f9cd (75 b) is NOT a function either.  It is the non-NULL arm of
//     0x0048f9c0 (CRezMgr::StrToType), whose NULL arm falls through to a
//     `xor %eax,%eax ; ret $4` at 0x0048f9c8 five bytes earlier.  That is
//     shape (a): the enumerator kept the arm and lost the entry test.  The
//     real function is 0x0048f9c0..0x0048fa15, 88 bytes.
//
//  The enumeration ALSO missed six whole functions inside the assigned
//  range, which is why nine rows become fourteen bodies here:
//      0x0048f900 Close, 0x0048f9b0 GetRootDir, 0x0048fac0 GetCurTime,
//      0x0048fe80 CRezMgr::GetRezFromDosPath, 0x0048fea0
//      CRezMgr::GetDirFromPath, 0x0048fec0 IsDirectory.
//  and split two of the rows it did have:
//      row 0x0048fb30 (458 b) is ONE function 0x0048fb30..0x0048fcf7 with
//      five `ret 4`s (five early exits, no duplicated epilogue -- each pops
//      a different amount of nothing), and row 0x0048ff00 (257 b) really
//      ends at 0x00490002, 259 bytes: `add $8,%esp` at 0x0048ffff runs one
//      byte past the row and the `ret` is the byte after that.
//      Row 0x00490080 (129 b) is the BACK HALF of 0x00490050 and branches
//      out to 0x0049007a; the function is 0x00490050..0x004900fe, 177 bytes.
//  Four rows were exact: 0x0048fa20 (110), 0x0048fad0 (87), 0x0048fb30 (458)
//  and 0x0048fd00 (370).
//
//
// *********************************************************************** //
//
//  WHAT THIS FILE IDENTIFIES THAT THE SIBLING FILES DID NOT
//
//   * 0x00492b60 is strncpy, NOT memcpy.  src/engine/lithtech/lt2new4.cpp
//     calls it "the CRT memcpy"; it is not.  Its dword scanner carries the
//     0x7efefeff zero-byte magic (0x00492bf1) and, when it finds the source
//     terminator, it FILLS THE REST OF THE COUNT WITH ZEROS (0x00492bba..
//     0x00492bdf, `mov %al,(%edi)` with al already 0) before returning the
//     destination.  That is strncpy and nothing else.  CRezDir::
//     GetRezFromDosPath below depends on it: `strncpy(sName, &sPath[i+1],
//     1023); sName[1023] = 0` only makes sense with strncpy semantics.
//
//   * 0x0049545a is time() -- GetLocalTime + GetSystemTime and a timezone
//     fixup (0x00495467, 0x00495471).
//   * 0x004955ee is _stat  -- the 0x24-byte buffer and the `st_mode`
//     load at +6 pin the VC6 struct _stat exactly.
//   * 0x00493994 / 0x00493a61 / 0x00493b29 are _findfirst / _findnext /
//     _findclose (FindFirstFileA at 0x004939a9).
//   * 0x004a0320 is _strupr (a-z -> A-Z at 0x004a0348, behind a
//     `__mbctype`-style locale check) -- rezmgr.cpp already said so.
//   * 0x00490740 is the constructor of the per-file CBaseRezFile that
//     directory emulation hands to each item: 0x1c bytes, vtable 0x004a3c00,
//     (CRezMgr*, char* sFileName, CRezFileDirectoryEmulation*), stores the
//     emulation object at +0x18, strdup's the name into +0x10 and links
//     itself into the emulation object's list at +0x1c (0x004a05b0).  It is
//     in rezfile.cpp, past this file's range.  rezmgr.h does not name it and neither
//     does anything else in the repo, so the name below is a placeholder.
//     // UNDECODED: the real class name of 0x00490740's type.
//
// *********************************************************************** //
//
//  WHERE THIS FILE DISAGREES WITH A SIBLING
//
//  rezmgr2.cpp says "`new T[n]` ... would have gone through operator new[]'s
//  own thunk and named a different symbol", and concludes every direct call
//  to 0x004290b0 is a raw `::operator new(n)`.  0x004290b0 is a five
//  instruction forwarding thunk (`mov 0x4(%esp),%eax ; push %eax ; call
//  0x00428fd0 ; add $4,%esp ; ret`) -- exactly the body an overridden
//  `operator new[]` and an overridden `operator new` would BOTH compile to,
//  which is precisely the case /OPT:ICF folds.  And the shape at 0x0048ff51
//  is not a raw allocation: allocate, `test %eax,%eax ; je` to a
//  `xor %eax,%eax`, construct forwards in a `dec/test/jl/inc` counted loop,
//  reload the pointer from its spill slot, THEN store it.  That is VC6's
//  array-new expansion verbatim, including the null-yields-null tail, and it
//  is written as `new CRezItm[n]` below.  Nothing in MY range allocates a
//  raw byte buffer, so this does not contradict their three sites; it only
//  means the folded thunk cannot be used as evidence either way.
//
// *********************************************************************** //

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <sys/stat.h>


// ======================================================================= //
//  lithtypes.h / reztypes.h -- only the widths are load-bearing.
// ======================================================================= //

typedef int                 BOOL_X380;
typedef unsigned long       DWORD_X380;
typedef unsigned char       BYTE_X380;

#define TRUE_X380   1
#define FALSE_X380  0
#define NULL_X380   0

typedef char*               REZNAME_X380;
typedef char*               REZDIRNAME_X380;
typedef const char*         REZCDIRNAME_X380;
typedef char*               REZDESC_X380;
typedef DWORD_X380          REZTYPE_X380;
typedef DWORD_X380          REZID_X380;
typedef DWORD_X380          REZSIZE_X380;
typedef DWORD_X380          REZTIME_X380;
typedef DWORD_X380          REZKEYVAL_X380;

#define RezMgrUserTitleSize_X380 60


// ======================================================================= //
//  src/lithshared/lith/baselist.h -- copied, unmodified in substance.
//  CBaseListItem has no virtuals, so m_pNext is at +0x00 of the SUBOBJECT;
//  inside a CBaseHashItem that subobject sits at +0x04 (the vptr takes
//  +0x00), which is what 0x0049008d's `mov 0x4(%edi),%edi` reads.
// ======================================================================= //

class CBaseListX380;

class CBaseListItemX380
{
public:
	CBaseListItemX380* Next() { return m_pNext; }
	CBaseListItemX380* Prev() { return m_pPrev; }

public:
	friend class CBaseListX380;

protected:
	CBaseListItemX380* m_pNext;              // +0x00
	CBaseListItemX380* m_pPrev;              // +0x04
};

class CBaseListX380
{
public:
	CBaseListX380() { m_pFirst = NULL_X380; m_pLast = NULL_X380; }
	~CBaseListX380() { }

	void Insert(CBaseListItemX380* pItem) { InsertFirst(pItem); }
	void InsertFirst(CBaseListItemX380* pItem);                     // 0x0048de30
	void InsertLast(CBaseListItemX380* pItem);
	void InsertAfter(CBaseListItemX380* pBeforeItem, CBaseListItemX380* pNewItem);
	void Delete(CBaseListItemX380* pItem);                          // 0x0048dee0

	CBaseListItemX380* GetFirst() { return m_pFirst; }
	CBaseListItemX380* GetLast()  { return m_pLast; }

	void FastDeleteAll() { m_pFirst = NULL_X380; m_pLast = NULL_X380; }

protected:
	CBaseListItemX380* m_pFirst;             // +0x00
	CBaseListItemX380* m_pLast;              // +0x04
};


// ======================================================================= //
//  src/lithshared/lith/basehash.h -- copied.  CBaseHash is 8 bytes
//  { m_nNumBins, m_pBinAry } and NextInBin() is the inline one; Next()
//  (0x004a03d0) is the out-of-line cross-bin walk and nothing here uses it.
//  0x00490050's iteration IS NextInBin: `mov 0x4(%edi),%edi ; test ; je ;
//  add $-4,%edi ; test ; jne` -- the -4 and its null guard are the
//  CBaseListItem* -> CBaseHashItem* base adjustment.
// ======================================================================= //

class CBaseHashItemX380;

class CBaseHashX380
{
public:
	CBaseHashX380();
	CBaseHashX380(unsigned int NumBins);                            // 0x004a0430
	~CBaseHashX380();                                               // 0x004a04c0

	void               Insert(CBaseHashItemX380* pItem);            // 0x004a04f0
	void               Delete(CBaseHashItemX380* pItem);            // 0x004a0530
	CBaseHashItemX380* GetFirst();                                  // 0x004a0560
	CBaseHashItemX380* GetLast();

protected:
	CBaseHashItemX380* GetFirstInBin(unsigned int Bin);             // 0x004a0590
	unsigned int       GetNumBins() { return m_nNumBins; }

private:
	friend class CBaseHashItemX380;

	unsigned int       m_nNumBins;           // +0x00  (0x00490126 divides by it)
	void*              m_pBinAry;            // +0x04
};

class CBaseHashItemX380 : public CBaseListItemX380
{
public:
	CBaseHashItemX380* Next();                                      // 0x004a03d0
	CBaseHashItemX380* Prev();
	CBaseHashItemX380* NextInBin() { return (CBaseHashItemX380*)CBaseListItemX380::Next(); }
	CBaseHashItemX380* PrevInBin() { return (CBaseHashItemX380*)CBaseListItemX380::Prev(); }

protected:
	virtual unsigned int HashFunc() = 0;

	CBaseHashX380* GetParentHash() { return m_pParentHash; }

private:
	friend class CBaseHashX380;

	CBaseHashX380* m_pParentHash;            // +0x0c  (0x0049003a reads it)
	unsigned int   m_nCurBin;                // +0x10
};


// ======================================================================= //
//  src/LT2/lithshared/lith/virtlist.h -- only what CRezMgr::Close touches.
//  Delete is 0x004a05e0 and the list's m_pFirst is at +0x04 of the list
//  because of the pure virtual (`mov 0x14(%esi),%edi` at 0x0048f93b reads
//  m_lstRezFiles.m_pFirst with m_lstRezFiles at +0x10).
// ======================================================================= //

class CVirtBaseListX380;

class CVirtBaseListItemX380
{
public:
	CVirtBaseListItemX380* Next() { return m_pNext; }
	CVirtBaseListItemX380* Prev() { return m_pPrev; }

	virtual void VirtualFoo() = 0;

public:
	friend class CVirtBaseListX380;

protected:
	CVirtBaseListItemX380* m_pNext;          // +0x04
	CVirtBaseListItemX380* m_pPrev;          // +0x08
};

class CVirtBaseListX380
{
public:
	CVirtBaseListX380() { m_pFirst = NULL_X380; m_pLast = NULL_X380; }
	~CVirtBaseListX380() { }

	void Insert(CVirtBaseListItemX380* pItem) { InsertFirst(pItem); }
	void InsertFirst(CVirtBaseListItemX380* pItem);                 // 0x004a05b0
	void InsertLast(CVirtBaseListItemX380* pItem);
	void Delete(CVirtBaseListItemX380* pItem);                      // 0x004a05e0

	CVirtBaseListItemX380* GetFirst() { return m_pFirst; }
	CVirtBaseListItemX380* GetLast()  { return m_pLast; }

	virtual void VirtualFoo() = 0;

private:
	CVirtBaseListItemX380* m_pFirst;         // +0x04
	CVirtBaseListItemX380* m_pLast;          // +0x08
};


// ======================================================================= //
//  rezhash.h -- the tables and elements this file implements or reaches.
//
//  The element is 0x18 bytes: CBaseHashItem (0x14) plus one back pointer at
//  +0x14.  0x00490030 reads +0x14 then dereferences it at +0x00 for the
//  name, and 0x0049007a/0x004900a8/0x004900f7 do the same in Find.
//
//  The by-name table's HashFunc(name) is 0x00490180 and the type table's
//  HashFunc(type) is 0x00490120; both are PAST this file's range and are declared
//  here, not defined -- see the closing note.
//
//  0x00490050 serves BOTH by-name Finds: CRezItmHashTableByName::Find and
//  CRezDirHashTable::Find compile to the same bytes (the owner is at +0x14
//  of the element and the owner's name at +0x00 of the owner, for CRezItm
//  and CRezDir alike), so the linker folds them.  Both are written below.
//
//  The second argument selects the compare: nonzero -> 0x00492f30, which
//  folds A-Z on both operands and is _stricmp; zero -> VC6's inlined strcmp.
//  So the flag is bIgnoreCase.  src/lithshared/rezmgr/rezmgr.cpp still calls
//  it bCaseSensitive, which is backwards; rezmgr2.cpp already said so.
// ======================================================================= //

class CRezItmX380;
class CRezTypX380;
class CRezDirX380;
class CRezMgrX380;
class CBaseRezFileX380;
class CRezFileDirectoryEmulationX380;

class CRezItmHashByNameX380 : public CBaseHashItemX380
{
public:
	CRezItmHashByNameX380* Next()      { return (CRezItmHashByNameX380*)CBaseHashItemX380::Next(); }
	CRezItmHashByNameX380* NextInBin() { return (CRezItmHashByNameX380*)CBaseHashItemX380::NextInBin(); }

	CRezItmX380* GetRezItm() { return m_pRezItm; }
	void SetRezItm(CRezItmX380* pRezItm) { m_pRezItm = pRezItm; }

	// 0x1003a536 `mov %ecx,0x30(%eax)` in CRezItm::CRezItm below: the
	// member's own constructor clears m_pRezItm before the owner sets it.
	CRezItmHashByNameX380() { m_pRezItm = NULL_X380; }

protected:
	virtual unsigned int HashFunc();                                // 0x00490030

private:
	friend class CRezItmX380;
	CRezItmX380* m_pRezItm;                  // +0x14
};

class CRezDirHashX380 : public CBaseHashItemX380
{
public:
	CRezDirHashX380* Next()      { return (CRezDirHashX380*)CBaseHashItemX380::Next(); }
	CRezDirHashX380* NextInBin() { return (CRezDirHashX380*)CBaseHashItemX380::NextInBin(); }

	CRezDirX380* GetRezDir() { return m_pRezDir; }
	void SetRezDir(CRezDirX380* pRezDir) { m_pRezDir = pRezDir; }

protected:
	virtual unsigned int HashFunc();                                // 0x00490110

public:
	friend class CRezDirX380;
	CRezDirX380* m_pRezDir;                  // +0x14
};

class CRezTypeHashX380 : public CBaseHashItemX380
{
public:
	CRezTypeHashX380* Next() { return (CRezTypeHashX380*)CBaseHashItemX380::Next(); }
	CRezTypX380* GetRezTyp() { return m_pRezTyp; }

protected:
	virtual unsigned int HashFunc();                                // 0x00490170

private:
	friend class CRezTypX380;
	CRezTypX380* m_pRezTyp;                  // +0x14
};

class CRezItmHashTableByNameX380 : public CBaseHashX380
{
public:
	CRezItmHashTableByNameX380() { }
	CRezItmHashTableByNameX380(unsigned int NumBins) : CBaseHashX380(NumBins) { }

	CRezItmX380* Find(REZNAME_X380 sName, BOOL_X380 bIgnoreCase);   // 0x00490050
	unsigned int HashFunc(REZNAME_X380 sName);                      // 0x00490180
	CRezItmHashByNameX380* GetFirst() { return (CRezItmHashByNameX380*)CBaseHashX380::GetFirst(); }
};

class CRezItmHashTableByIDX380 : public CBaseHashX380
{
public:
	CRezItmHashTableByIDX380() { }
	CRezItmHashTableByIDX380(unsigned int NumBins) : CBaseHashX380(NumBins) { }

	CRezItmX380* Find(REZID_X380 nID);
};

class CRezDirHashTableX380 : public CBaseHashX380
{
public:
	CRezDirHashTableX380() { }
	CRezDirHashTableX380(unsigned int NumBins) : CBaseHashX380(NumBins) { }

	CRezDirX380* Find(REZDIRNAME_X380 sName, BOOL_X380 bIgnoreCase); // folds onto 0x00490050
	unsigned int HashFunc(REZDIRNAME_X380 sName);                    // folds onto 0x00490180
	CRezDirHashX380* GetFirst() { return (CRezDirHashX380*)CBaseHashX380::GetFirst(); }
};

class CRezTypeHashTableX380 : public CBaseHashX380
{
public:
	CRezTypeHashTableX380() { }
	CRezTypeHashTableX380(unsigned int NumBins) : CBaseHashX380(NumBins) { }

	CRezTypX380* Find(REZTYPE_X380 nType);                          // 0x00490130
	unsigned int HashFunc(REZTYPE_X380 nType);                      // 0x00490120
	CRezTypeHashX380* GetFirst() { return (CRezTypeHashX380*)CBaseHashX380::GetFirst(); }
};


// ======================================================================= //
//  rezfile.h -- only as much as this file needs.
//
//  Nine vtable slots.  Slot 0 is CVirtBaseListItem::VirtualFoo, slot 1 the
//  virtual destructor and slot 5 Close(): CRezMgr::Close issues
//  `call *0x14(%eax)` at 0x0048f90c and 0x0048f946 (slot 5) and
//  `push $1 ; call *0x4(%edx)` at 0x0048f935 and 0x0048f962 (slot 1, the
//  scalar deleting destructor).  Slots 2, 3, 4, 6, 7 and 8 are never reached
//  from this file and exist only to put 1 and 5 at the right offsets; their
//  signatures come from rezmgr2.cpp.
// ======================================================================= //

class CBaseRezFileX380 : public CVirtBaseListItemX380
{
public:
	virtual ~CBaseRezFileX380();                                          // slot 1

	virtual DWORD_X380 Read(DWORD_X380 Pos, DWORD_X380 nUnknown,
	                        DWORD_X380 Size, void* pData);                // slot 2
	virtual DWORD_X380 Write(DWORD_X380 Pos, DWORD_X380 nUnknown,
	                         DWORD_X380 Size, void* pData);               // slot 3
	virtual BOOL_X380  Open(const char* sFileName, BOOL_X380 bReadOnly,
	                        BOOL_X380 bCreateNew);                        // slot 4
	virtual BOOL_X380  Close();                                           // slot 5
	virtual BOOL_X380  Flush();                                           // slot 6
	virtual BOOL_X380  VerifyFileOpen();                                  // slot 7
	virtual const char* GetFileName();                                    // slot 8

	virtual void VirtualFoo();                                            // slot 0
};

class CRezFileX380 : public CBaseRezFileX380
{
public:
	CRezFileX380(CRezMgrX380* pRezMgr);                             // 0x00490210
private:
	BYTE_X380 m_pad_0c[0x24 - 0x0c];
};

class CRezFileDirectoryEmulationX380 : public CBaseRezFileX380
{
public:
	CRezFileDirectoryEmulationX380(CRezMgrX380* pRezMgr, int nMaxOpenFiles);  // 0x004905e0
private:
	BYTE_X380 m_pad_0c[0x38 - 0x0c];
};

// 0x00490740, 0x1c bytes, vtable 0x004a3c00.  One of these per file found
// inside an emulated directory; it keeps the emulation object at +0x18, a
// strdup of the full path at +0x10, and links itself into the emulation
// object's CVirtBaseList at +0x1c.  ReadEmulationDirectory is its only
// constructor site in the image.
// UNDECODED: rezmgr.h does not name this class and nothing else in the repo
// does either.  The name below is mine.
class CRezFileDirectoryEmulationFileX380 : public CBaseRezFileX380
{
public:
	CRezFileDirectoryEmulationFileX380(CRezMgrX380* pRezMgr, char* sFileName,
	                                   CRezFileDirectoryEmulationX380* pEmulation); // 0x00490740
private:
	BYTE_X380 m_pad_0c[0x1c - 0x0c];
};

class CBaseRezFileListX380 : public CVirtBaseListX380
{
public:
	CBaseRezFileX380* GetFirst() { return (CBaseRezFileX380*)CVirtBaseListX380::GetFirst(); }
	void Delete(CBaseRezFileX380* pItem) { CVirtBaseListX380::Delete(pItem); }

	virtual void VirtualFoo() { }                                   // 0x00490200
};


// ======================================================================= //
//  rezmgr.h, LITE build.  Layouts taken whole from the two sibling files;
//  the offsets MY code actually touches, with the instruction that proves
//  each one:
//
//    CRezItm  +0x00 m_sName        0x0049007e `mov (%eax),%eax`
//             +0x08 m_nTime        0x0048f857 `mov %ecx,0x8(%esi)`
//             +0x0c m_nSize        0x0048f85e `mov %edx,0xc(%esi)`
//             +0x1c m_heName       0x0048ffee `lea 0x1c(%esi),%ecx`
//             +0x30 m_heName.m_pRezItm  0x0048ffb0 `mov %eax,0x30(%eax)`
//             +0x34 m_pRezFile     0x0048f8a7 `mov %eax,0x34(%esi)`
//             0x3c stride          0x0048ff70 `add $0x3c,%edi`
//    CRezDir  +0x18 m_pRezMgr      0x0048fb5a `mov 0x18(%esi),%ecx`
//    CRezMgr  +0x04 m_sDirSeparators 0x0048fad5 / 0x0048fb5f
//             +0x0c m_bFileOpened  0x0048f99d
//             +0x10 m_lstRezFiles  0x0048f912 `lea 0x10(%esi),%ebx`
//             +0x14   .m_pFirst    0x0048f93b
//             +0x1c m_nNumRezFiles 0x0048f921
//             +0x20 m_pPrimaryRezFile 0x0048f907
//             +0x28 m_nNextIDNumToUse 0x0048f786
//             +0x44 m_pRootDir     0x0048f9b0 `mov 0x44(%ecx),%eax`
//             +0x64 m_sFileName    0x0048f986
//             +0x68 m_bLowerCaseUsed 0x0048f54c `mov 0x68(%ebp),%eax`
//             +0x80 m_hashRezItmFreeList 0x0048ff09 `lea 0x80(%ebp),%edi`
//             +0x88 m_lstRezItmChunks    0x0048ffcf
//             +0x90 m_nRezItmChunkSize   0x0048ff41
//             0xd4  total (filemgr.cpp's CFileArchive)
// ======================================================================= //

class CRezItmX380
{
public:
	REZNAME_X380  GetName() { return m_sName; }
	REZSIZE_X380  GetSize() { return m_nSize; }
	REZTIME_X380  GetTime() { return m_nTime; }
	CRezDirX380*  GetParentDir() { return m_pParentDir; }

private:
	CRezItmX380();                                                        // 0x0048df10

	void InitRezItm(CRezDirX380* pParentDir, REZNAME_X380 sName, REZID_X380 nID,
	                CRezTypX380* pType, REZDESC_X380 sDesc, REZSIZE_X380 nSize,
	                DWORD_X380 nFilePos, REZTIME_X380 nTime, DWORD_X380 nNumKeys,
	                REZKEYVAL_X380* pKeyAry, CBaseRezFileX380* pRezFile);  // 0x0048df30
	void TermRezItm();                                                    // 0x0048dfc0

	friend class CRezTypX380;
	friend class CRezDirX380;
	friend class CRezMgrX380;

	REZNAME_X380          m_sName;           // +0x00
	CRezTypX380*          m_pType;           // +0x04
	REZTIME_X380          m_nTime;           // +0x08
	REZSIZE_X380          m_nSize;           // +0x0c
	CRezDirX380*          m_pParentDir;      // +0x10
	DWORD_X380            m_nFilePos;        // +0x14
	DWORD_X380            m_nCurPos;         // +0x18
	CRezItmHashByNameX380 m_heName;          // +0x1c
	CBaseRezFileX380*     m_pRezFile;        // +0x34
	BYTE_X380*            m_pData;           // +0x38
};

class CRezTypX380
{
public:
	REZTYPE_X380 GetType() { return m_nType; }

private:
	CRezTypX380(REZTYPE_X380 nType, CRezDirX380* pParentDir,
	            unsigned int nByIDNumHashBins, unsigned int nByNameNumHashBins);
	CRezTypX380(REZTYPE_X380 nType, CRezDirX380* pParentDir, unsigned int nByNameNumHashBins);
	~CRezTypX380();

	friend class CRezItmX380;
	friend class CRezDirX380;
	friend class CRezMgrX380;

	REZTYPE_X380               m_nType;      // +0x00
	CRezTypeHashX380           m_heType;     // +0x04
	CRezItmHashTableByIDX380   m_haID;       // +0x1c
	CRezItmHashTableByNameX380 m_haName;     // +0x24
	CRezDirX380*               m_pParentDir; // +0x2c
};

class CRezDirX380
{
public:
	REZDIRNAME_X380 GetDirName() { return m_sDirName; }
	CRezDirX380*    GetParentDir() { return m_pParentDir; }
	CRezMgrX380*    GetParentMgr() { return m_pRezMgr; }

	CRezItmX380* GetRez(REZNAME_X380 sRes, REZTYPE_X380 Type);            // 0x0048e510
	CRezItmX380* GetRezFromDosName(char* sDosname);                       // 0x0048e550
	CRezItmX380* GetRezFromPath(const char* sPath, REZTYPE_X380 type);
	CRezItmX380* GetRezFromDosPath(const char* sPath);                    // 0x0048fd00

	BOOL_X380 Load(BOOL_X380 LoadAllSubDirs = FALSE_X380);
	BOOL_X380 UnLoad(BOOL_X380 UnLoadAllSubDirs = FALSE_X380);
	BOOL_X380 IsLoaded() { return (m_pMemBlock != NULL_X380); }

	CRezDirX380* GetDir(REZCDIRNAME_X380 sDir);                           // 0x0048e630
	CRezDirX380* GetDirFromPath(const char* pPath);                       // 0x0048fb30
	CRezDirX380* GetFirstSubDir();
	CRezDirX380* GetNextSubDir(CRezDirX380* pRezDir);

	CRezTypX380* GetRezTyp(REZTYPE_X380 Type);
	CRezTypX380* GetFirstType();
	CRezTypX380* GetNextType(CRezTypX380* pRezType);

	CRezItmX380* GetFirstItem(CRezTypX380* pRezType);
	CRezItmX380* GetNextItem(CRezItmX380* pRezItm);

	CRezDirX380* CreateDir(REZDIRNAME_X380 sDir);                         // 0x0048e720
	CRezItmX380* CreateRez(REZID_X380 ID, REZNAME_X380 Name, REZTYPE_X380 Type);
	REZTIME_X380 GetTime() { return m_nLastTimeModified; }

private:
	CRezDirX380(CRezMgrX380* pRezMgr, CRezDirX380* pParentDir, REZDIRNAME_X380 szDirName,
	            DWORD_X380 nFilePos, DWORD_X380 nSize, REZTIME_X380 nTime,
	            unsigned int nDirNumHashBins, unsigned int nTypNumHashBins);
	~CRezDirX380();                                                       // 0x0048e3e0

	friend class CRezItmX380;
	friend class CRezTypX380;
	friend class CRezMgrX380;

	BOOL_X380 ReadAllDirs(CBaseRezFileX380* pRezFile, DWORD_X380 Pos,
	                      DWORD_X380 Size, BOOL_X380 bOverwriteItems);
	BOOL_X380 ReadDirBlock(CBaseRezFileX380* pRezFile, DWORD_X380 Pos,
	                       DWORD_X380 Size, BOOL_X380 bOverwriteItems);
	CRezTypX380* GetOrMakeTyp(REZTYPE_X380 nType);                        // 0x0048ec80

	// NOT out of line anywhere in the image -- every one of its five call
	// sites below is an inline expansion, so the definition it was compiled
	// from must have carried `inline`.  VC6's /O2 is /Ob1, which will not
	// inline it otherwise.
	BOOL_X380    IsGoodChar(char c);

	CRezItmX380* CreateRezInternal(REZID_X380 nID, REZNAME_X380 sName,
	                               CRezTypX380* pTyp, CBaseRezFileX380* pRezFile); // 0x0048e7f0
	BOOL_X380    RemoveRezInternal(CRezTypX380* pTyp, CRezItmX380* pItm);          // 0x0048e870

	REZDIRNAME_X380       m_sDirName;            // +0x00
	DWORD_X380            m_nDirPos;             // +0x04
	DWORD_X380            m_nDirSize;            // +0x08
	DWORD_X380            m_nItemsPos;           // +0x0c
	DWORD_X380            m_nItemsSize;          // +0x10
	REZTIME_X380          m_nLastTimeModified;   // +0x14
	CRezMgrX380*          m_pRezMgr;             // +0x18
	CRezDirX380*          m_pParentDir;          // +0x1c
	CRezDirHashX380       m_heDir;               // +0x20
	CRezDirHashTableX380  m_haDir;               // +0x38
	CRezTypeHashTableX380 m_haTypes;             // +0x40
	BYTE_X380*            m_pMemBlock;           // +0x48
};

class CRezMgrX380
{
public:
	CRezMgrX380();                                                        // 0x0048ed50
	CRezMgrX380(const char* FileName, BOOL_X380 ReadOnly = TRUE_X380,
	            BOOL_X380 CreateNew = FALSE_X380);
	~CRezMgrX380();                                                       // 0x0048ee40

	BOOL_X380 Open(const char* FileName, BOOL_X380 ReadOnly = TRUE_X380,
	               BOOL_X380 CreateNew = FALSE_X380);                     // 0x0048ef80
	BOOL_X380 OpenAdditional(const char* FileName, BOOL_X380 bOverwriteItems = FALSE_X380);
	BOOL_X380 Close(BOOL_X380 bCompact = FALSE_X380);                     // 0x0048f900
	CRezDirX380* GetRootDir();                                            // 0x0048f9b0
	BOOL_X380 IsOpen() { return m_bFileOpened; }
	BOOL_X380 VerifyFileOpen();
	REZTIME_X380 GetTime() { return m_nLastTimeModified; }
	BOOL_X380 Reset();
	BOOL_X380 IsSorted() { return m_bIsSorted; }

	CRezItmX380* GetRezFromPath(const char* sPath, REZTYPE_X380 type);
	CRezItmX380* GetRezFromDosPath(const char* sPath);                    // 0x0048fe80
	CRezDirX380* GetDirFromPath(const char* pPath);                       // 0x0048fea0

	void SetDirSeparators(const char* sDirSeparators);                    // 0x0048fad0

	virtual void*     Alloc(DWORD_X380 NumBytes);                         // 0x0048fa90
	virtual void      Free(void* Ptr);                                    // 0x0048faa0
	virtual BOOL_X380 DiskError();                                        // 0x0048fab0

	REZTYPE_X380 StrToType(char* pStr);                                   // 0x0048f9c0
	void         TypeToStr(REZTYPE_X380 nType, char* sType);              // 0x0048fa20

	void SetRenumberIDCollisions(BOOL_X380 bFlag);
	void SetNextIDNumber(DWORD_X380 nID);

	BOOL_X380 GetLowerCaseUsed() { return m_bLowerCaseUsed; }
	void SetLowerCaseUsed(BOOL_X380 bLowerCaseUsed) { m_bLowerCaseUsed = bLowerCaseUsed; }

	BOOL_X380 GetItemByIDUsed() { return m_bItemByIDUsed; }
	void SetItemByIDUsed(BOOL_X380 bItemByIDUsed) { m_bItemByIDUsed = bItemByIDUsed; }

	void SetHashTableBins(unsigned int nByNameNumHashBins, unsigned int nByIDNumHashBins,
	                      unsigned int nDirNumHashBins, unsigned int nTypNumHashBins);

	void ForceIsSortedFlag(BOOL_X380 bFlag) { m_bIsSorted = bFlag; }
	void SetMaxOpenFilesInEmulatedDir(int nNumFiles) { m_nMaxOpenFilesInEmulatedDir = nNumFiles; }

	void  SetUserTitle(const char* sUserTitle);
	char* GetUserTitle() { return m_sUserTitle; }

private:
	friend class CRezDirX380;
	friend class CRezTypX380;
	friend class CRezItmX380;

	class CRezItmChunkX380 : public CBaseListItemX380
	{
	public:
		CRezItmChunkX380* Next() { return (CRezItmChunkX380*)CBaseListItemX380::Next(); }
		CRezItmX380* m_pRezItmAry;           // +0x08
	};

	class CRezItmChunkListX380 : public CBaseListX380
	{
	public:
		CRezItmChunkX380* GetFirst() { return (CRezItmChunkX380*)CBaseListX380::GetFirst(); }
	};

	CRezItmX380* AllocateRezItm();                                        // 0x0048ff00
	void         DeAllocateRezItm(CRezItmX380* pItem);                    // 0x00490010

	REZTIME_X380 GetCurTime();                                            // 0x0048fac0
	BOOL_X380    IsDirectory(const char* sFileName);                      // 0x0048fec0
	BOOL_X380    ReadEmulationDirectory(CRezFileDirectoryEmulationX380* pRezFileEmulation,
	                                    CRezDirX380* pDir, char* sParamPath,
	                                    BOOL_X380 bOverwriteItems);       // 0x0048f380

	char*                      m_sDirSeparators;           // +0x04
	BOOL_X380                  m_bIsSorted;                // +0x08
	BOOL_X380                  m_bFileOpened;              // +0x0c
	CBaseRezFileListX380       m_lstRezFiles;              // +0x10 (vptr,first,last)
	DWORD_X380                 m_nNumRezFiles;             // +0x1c
	CBaseRezFileX380*          m_pPrimaryRezFile;          // +0x20
	BOOL_X380                  m_bRenumberIDCollisions;    // +0x24
	DWORD_X380                 m_nNextIDNumToUse;          // +0x28
	int                        m_nMaxOpenFilesInEmulatedDir; // +0x2c
	DWORD_X380                 m_nRootDirPos;              // +0x30
	DWORD_X380                 m_nRootDirSize;             // +0x34
	REZTIME_X380               m_nRootDirTime;             // +0x38
	DWORD_X380                 m_nNextWritePos;            // +0x3c
	BOOL_X380                  m_bReadOnly;                // +0x40
	CRezDirX380*               m_pRootDir;                 // +0x44
	REZTIME_X380               m_nLastTimeModified;        // +0x48
	BOOL_X380                  m_bMustReWriteDirs;         // +0x4c
	DWORD_X380                 m_nFileFormatVersion;       // +0x50
	DWORD_X380                 m_nLargestKeyAry;           // +0x54
	DWORD_X380                 m_nLargestDirNameSize;      // +0x58
	DWORD_X380                 m_nLargestRezNameSize;      // +0x5c
	DWORD_X380                 m_nLargestCommentSize;      // +0x60
	char*                      m_sFileName;                // +0x64
	BOOL_X380                  m_bLowerCaseUsed;           // +0x68
	BOOL_X380                  m_bItemByIDUsed;            // +0x6c
	unsigned int               m_nByNameNumHashBins;       // +0x70
	unsigned int               m_nByIDNumHashBins;         // +0x74
	unsigned int               m_nDirNumHashBins;          // +0x78
	unsigned int               m_nTypNumHashBins;          // +0x7c
	CRezItmHashTableByNameX380 m_hashRezItmFreeList;       // +0x80
	CRezItmChunkListX380       m_lstRezItmChunks;          // +0x88
	unsigned int               m_nRezItmChunkSize;         // +0x90
	char                       m_sUserTitle[RezMgrUserTitleSize_X380 + 1];  // +0x94 -> 0xd4
};


// ======================================================================= //
//  0x0048fb30 / 0x0048fd00 / 0x0048f380 all inline this.  Its two halves:
//  a strchr against m_pRezMgr->m_sDirSeparators when that is set, and
//  otherwise four inclusive ranges tested in the order ' '..'.', '0'..'9',
//  'A'..'Z', 'a'..'z' (0x0048fbbf, 0x0048fbe1, 0x0048fcc1, 0x0048fd84,
//  0x0048fda6 -- five copies, all identical).  The compares are `jl`/`jle`,
//  so `c` is a plain signed char, and the strchr answer is inverted by
//  `neg %eax ; sbb %eax,%eax ; inc %eax`, i.e. `== NULL`.
//
//  The ranges are four `if (...) return TRUE;` lines, not one `||` return:
//  the `||` form flips the last branch of the copy inlined into the
//  leading-separator test (`jg bad; jmp good` for shipped's `jle good; jmp
//  bad`, 0x0048fda2) and cost GetRezFromDosPath and GetDirFromPath.
// ======================================================================= //

inline BOOL_X380 CRezDirX380::IsGoodChar(char c)
{
	if (m_pRezMgr->m_sDirSeparators != NULL_X380)
		return (strchr(m_pRezMgr->m_sDirSeparators, c) == NULL_X380);

	if ((c >= ' ') && (c <= '.')) return TRUE_X380;
	if ((c >= '0') && (c <= '9')) return TRUE_X380;
	if ((c >= 'A') && (c <= 'Z')) return TRUE_X380;
	if ((c >= 'a') && (c <= 'z')) return TRUE_X380;
	return FALSE_X380;
}


// ======================================================================= //
//  0x0048f380  CRezMgr::ReadEmulationDirectory        1,400 bytes, `ret $10`
//  (the enumeration's 1,322 + the 78 of the bogus row 0x0048f8aa)
//
//  This is the /GX one: `push $-1 ; push $0x4a095e ; push %fs:0 ;
//  mov $0x1774,%eax ; call 0x00492c60` is VC6's __CxxFrameHandler
//  registration plus _chkstk for a 6,004 byte frame, and the state slot the
//  registration pushed at -1 is driven to 0 at 0x0048f86f and back to -1 at
//  0x0048f89c around the one `new`.
//
//  VERIFIED CALL SEQUENCE (20 + _chkstk), in address order.  Every one read
//  off the bytes, none inferred:
//    0x0048f39a  0x00492c60  _chkstk (compiler, not source)
//    0x0048f486  0x00493994  _findfirst
//    0x0048f560  0x004a0320  _strupr          (subdirectory name)
//    0x0048f5ff  0x0048e630  CRezDir::GetDir
//    0x0048f612  0x0048e720  CRezDir::CreateDir
//    0x0048f63a  0x0048f380  itself, recursive
//    0x0048f6c4  0x00495313  _splitpath
//    0x0048f6fc  0x004a0320  _strupr          (file name)
//    0x0048f737  0x0049371c  atol
//    0x0048f76c  0x004951f0  strncat
//    0x0048f7cf  0x004a0320  _strupr          (type string)
//    0x0048f7e0  0x0048f9c0  CRezMgr::StrToType
//    0x0048f7f5  0x0048fa20  CRezMgr::TypeToStr
//    0x0048f7fd  0x0048ec80  CRezDir::GetOrMakeTyp
//    0x0048f80f  0x0048e510  CRezDir::GetRez
//    0x0048f833  0x0048e870  CRezDir::RemoveRezInternal
//    0x0048f846  0x0048e7f0  CRezDir::CreateRezInternal
//    0x0048f861  0x004290b0  operator new
//    0x0048f893  0x00490740  the emulation file constructor
//    0x0048f8b4  0x00493a61  _findnext
//    0x0048f8c9  0x00493b29  _findclose
//  Everything else that looks like a string call is VC6's intrinsic
//  expansion: eleven `repnz scasb`+`rep movs` pairs (strcpy/strcat/strlen)
//  and two open-coded strcmp byte loops ("." at 0x0048f4ab, ".." at
//  0x0048f4e5).  strncat and _strupr are NOT intrinsics and stayed calls.
//
//  SHAPES THE BYTES DECIDE:
//    * the `.`/`..` tests and the `!bOverwriteItems` test all branch to
//      0x0048f8aa, the _findnext at the bottom, so they are `continue`s in a
//      do/while -- the loop back edge is 0x0048f8be -> 0x0048f4a2.
//    * the `hFind < 0` exit is `test %eax,%eax ; jl` to a `mov $1,%eax`
//      placed at the very END (0x0048f8d9), after the loop's own `return
//      TRUE` -- a positive guard `if (hFind >= 0) { loop; _findclose;
//      return TRUE; } return TRUE;`.  An early `if (hFind < 0) return TRUE;`
//      puts that return inline (`jge ; mov $1 ; jmp`).
//    * the all-digits name test: the break path re-tests at 0x0048f77e
//      (`cmp ; jge atol`) and the m_nNextIDNumToUse arm sits out of line
//      after the `>5` arm -- `if (i < nLen) { next id } else { atol }`,
//      the counter first.  `if (i >= nLen)` puts the counter arm inline.
//    * _strupr is conditional on m_bLowerCaseUsed for the SUBDIRECTORY name
//      (0x0048f552) and UNCONDITIONAL for the file name (0x0048f6fc).  That
//      asymmetry is in the shipped code, not invented here.
//    * the GetRez hit and the GetRez miss reach CreateRezInternal through
//      TWO SEPARATE ARGUMENT SETUPS that share only the tail
//      `push %ebp ; mov %ebx,%ecx ; call` at 0x0048f843.  The miss pushes
//      the NULL it already has in %eax (0x0048f818) and the hit pushes a
//      literal $0 (0x0048f838); a single source call site cannot produce
//      two different prologues for one push list, so this is TWO call sites
//      cross-jumped onto their common tail -- one inside each arm of
//      `if (pItm == NULL) ... else ...`.  The arm order is in the branch:
//      `jne 0x0048f824` enters the OVERWRITE arm and lets the NULL arm fall
//      through, so the guard tests `pItm == NULL` and the overwrite work is
//      the else.  (An earlier reading of this function called it one call;
//      the two prologues say otherwise, and a shared tail is what
//      cross-jumping always leaves.)
//    * `nType = 0` is materialised ONCE, at 0x0048f7e9, after the
//      StrToType arm (which jumps over it); the `>5` arm (0x0048f77c) and
//      the empty-extension test (0x0048f7a2) both branch into it.  That is
//      `nType = 0;` in the `>5` arm AND in a final `else`, cross-jumped
//      onto one block.  One `nType = 0;` before the chain instead leaves a
//      `xor %esi,%esi` ahead of the first strlen (measured: rezdir.obj
//      loses this function).
//    * `sName[256] = 0` (0x0048f774) writes into the _splitpath NAME buffer
//      at +0xb54, while the strncat just above it appends to the WORKING
//      COPY at +0x23c.  Two different buffers.  src/lithshared/rezmgr/
//      rezmgr.cpp's CRezDir::GetRezFromDosName has the same pair of
//      statements operating on ONE buffer, so this looks like that code
//      copy-pasted after the working copy was introduced -- the truncation
//      lands on the wrong array.  Written as the bytes have it.
//    * TypeToStr's output buffer (+0x18) is never read afterwards.  Dead,
//      and kept.
//
//  BUFFER SIZES -- the frame is 0x1774 and every displacement below is
//  pinned, so the slot BOUNDARIES are certain and the declared sizes are
//  only certain to within VC6's 4-byte rounding:
//    +0x1774 end          +0x146c sSearchPath 0x308   +0x1368 sDir    0x104
//    +0x1060 sSubPath 0x308   +0x0d58 sFullName 0x308  +0x0b54 sName  0x204
//    +0x084c sSubDirName 0x308 +0x0544 sPath  0x308    +0x023c sFileName 0x308
//    +0x0138 sExt 0x104   +0x0020 fileinfo 0x118 (sizeof _finddata_t)
//    +0x0018 sTypeStr 8   +0x0014 sDrive 4   +0x0010 compiler temp
//    +0x0008 sType 8      +0x0004 hFind   +0x0000 spilled `this`
//  0x204 and 0x104 match rezmgr.cpp's GetRezFromDosName slot sizes exactly,
//  which is some independent support for 516/260 being what the source said.
//  The 0x308s have no name: written as 776.
//  COIN FLIP, SAME ONE rezmgr.cpp CALLED: the declaration order below is
//  first-declared-highest.  Reverse the block if VC6 goes the other way.
// ======================================================================= //

BOOL_X380 CRezMgrX380::ReadEmulationDirectory(CRezFileDirectoryEmulationX380* pRezFileEmulation,
                                              CRezDirX380* pDir, char* sParamPath,
                                              BOOL_X380 bOverwriteItems)
{
	char sSearchPath[776];
	char sDir[260];
	char sSubPath[776];
	char sFullName[776];
	char sName[516];
	char sSubDirName[776];
	char sPath[776];
	char sFileName[776];
	char sExt[260];
	struct _finddata_t fileinfo;
	char sTypeStr[5];
	char sDrive[4];
	char sType[5];
	long hFind;

	strcpy(sPath, sParamPath);
	if (sPath[strlen(sPath) - 1] != '\\')
		strcat(sPath, "\\");

	strcpy(sSearchPath, sPath);
	strcat(sSearchPath, "*.*");

	hFind = _findfirst(sSearchPath, &fileinfo);
	if (hFind >= 0)
	{
	do
	{
		if (strcmp(fileinfo.name, ".") == 0) continue;
		if (strcmp(fileinfo.name, "..") == 0) continue;

		if ((fileinfo.attrib & _A_SUBDIR) == _A_SUBDIR)
		{
			CRezDirX380* pSubDir;

			strcpy(sSubDirName, fileinfo.name);
			if (!m_bLowerCaseUsed) _strupr(sSubDirName);

			strcpy(sSubPath, sPath);
			strcat(sSubPath, sSubDirName);
			strcat(sSubPath, "\\");

			pSubDir = pDir->GetDir(sSubDirName);
			if (pSubDir == NULL_X380)
			{
				pSubDir = pDir->CreateDir(sSubDirName);
				if (pSubDir == NULL_X380) continue;
			}

			ReadEmulationDirectory(pRezFileEmulation, pSubDir, sSubPath, bOverwriteItems);
		}
		else
		{
			REZID_X380   nID;
			REZTYPE_X380 nType;
			CRezTypX380* pTyp;
			CRezItmX380* pItm;
			int          i;
			int          nLen;

			strcpy(sFullName, sPath);
			strcat(sFullName, fileinfo.name);

			_splitpath(sFullName, sDrive, sDir, sName, sExt);

			strcpy(sFileName, sName);
			_strupr(sFileName);

			nLen = strlen(sFileName);
			for (i = 0; i < nLen; i++)
			{
				if ((sFileName[i] < '0') || (sFileName[i] > '9')) break;
			}

			if (i < nLen)
			{
				nID = m_nNextIDNumToUse;
				m_nNextIDNumToUse++;
			}
			else
			{
				nID = atol(sFileName);
			}

			if (strlen(sExt) > 5)
			{
				strncat(sFileName, sExt, 513);
				sName[256] = '\0';
				nType = 0;
			}
			else if (strlen(sExt) != 0)
			{
				strcpy(sType, &sExt[1]);
				_strupr(sType);
				nType = StrToType(sType);
			}
			else
			{
				nType = 0;
			}

			TypeToStr(nType, sTypeStr);

			pTyp = pDir->GetOrMakeTyp(nType);

			pItm = pDir->GetRez(sFileName, nType);
			if (pItm == NULL_X380)
			{
				pItm = pDir->CreateRezInternal(nID, sFileName, pTyp, NULL_X380);
			}
			else
			{
				if (!bOverwriteItems) continue;
				pDir->RemoveRezInternal(pTyp, pItm);
				pItm = pDir->CreateRezInternal(nID, sFileName, pTyp, NULL_X380);
			}

			if (pItm == NULL_X380) continue;

			pItm->m_nTime = fileinfo.time_write;
			pItm->m_nSize = fileinfo.size;
			pItm->m_pRezFile = new CRezFileDirectoryEmulationFileX380(this, sFullName,
			                                                          pRezFileEmulation);
		}
	}
	while (_findnext(hFind, &fileinfo) == 0);

	_findclose(hFind);

	return TRUE_X380;
	}

	return TRUE_X380;
}


// ======================================================================= //
//  0x0048f900  CRezMgr::Close                  168 bytes + 8 pad, `ret $4`
//  (MISSED BY THE ENUMERATION.  server.dll's twin is what
//  src/engine/server/filemgr.cpp has been calling CFileArchive::Close.)
//
//  VERIFIED CALL SEQUENCE (9, in address order):
//    0x0048f90c  *(vtbl+0x14) = CBaseRezFile::Close, slot 5
//    0x0048f91c  0x004a05e0 CVirtBaseList::Delete
//    0x0048f935  *(vtbl+0x04) = the scalar deleting destructor, slot 1
//    0x0048f946  *(vtbl+0x14) again (the loop's Close)
//    0x0048f94c  0x004a05e0 again
//    0x0048f962  *(vtbl+0x04) again
//    0x0048f975  0x0048e3e0 CRezDir::~CRezDir
//    0x0048f97b  0x004290c0 operator delete
//    0x0048f98e  0x004290c0 operator delete
//
//  bCompact is never read: the LITE build cannot compact.
//
//  The two `delete`s of a CBaseRezFile carry NO source-level null test --
//  `cmp %ebp,%ecx ; je` at 0x0048f92d and 0x0048f955 is the test VC6 emits
//  inside `delete` itself, and the `m_pPrimaryRezFile = NULL` at 0x0048f938
//  is UNCONDITIONAL (it is the branch target).  The two below it are the
//  other way round: `m_pRootDir = NULL` (0x0048f983) and `m_sFileName = NULL`
//  (0x0048f996) sit INSIDE their tests, so those two `if`s are in the
//  source and VC6 folded away delete's own duplicate test.
//
//  m_pPrimaryRezFile is loaded three separate times (0x0048f907, 0x0048f90f,
//  0x0048f92a) with a call between each pair, so it is written three times
//  in the source rather than cached in a local.
// ======================================================================= //

BOOL_X380 CRezMgrX380::Close(BOOL_X380 /*bCompact*/)
{
	BOOL_X380 bRetVal;
	CBaseRezFileX380* pRezFile;

	bRetVal = m_pPrimaryRezFile->Close();

	m_lstRezFiles.Delete(m_pPrimaryRezFile);
	m_nNumRezFiles--;
	delete m_pPrimaryRezFile;
	m_pPrimaryRezFile = NULL_X380;

	pRezFile = m_lstRezFiles.GetFirst();
	while (pRezFile != NULL_X380)
	{
		pRezFile->Close();
		m_lstRezFiles.Delete(pRezFile);
		m_nNumRezFiles--;
		delete pRezFile;
		pRezFile = m_lstRezFiles.GetFirst();
	}

	if (m_pRootDir != NULL_X380)
	{
		delete m_pRootDir;
		m_pRootDir = NULL_X380;
	}

	if (m_sFileName != NULL_X380)
	{
		delete m_sFileName;
		m_sFileName = NULL_X380;
	}

	m_bFileOpened = FALSE_X380;

	return bRetVal;
}


// ======================================================================= //
//  0x0048f9b0  CRezMgr::GetRootDir                       4 bytes, `ret`
//  (MISSED BY THE ENUMERATION.)  `mov 0x44(%ecx),%eax ; ret`.  Declared out
//  of line in rezmgr.h, so it is a real function and both 0x0048fe80 and
//  0x0048fea0 call it.  Call-free -- it scores only if byte-identical.
// ======================================================================= //

CRezDirX380* CRezMgrX380::GetRootDir()
{
	return m_pRootDir;
}


// ======================================================================= //
//  0x0048f9c0  CRezMgr::StrToType                       88 bytes, `ret $4`
//  (the enumeration's bogus 0x0048f9cd row is this function's non-NULL arm)
//
//  Call-free; the only non-trivial thing in it is a `repnz scasb` strlen.
//
//  The dword being built lives in the INCOMING ARGUMENT SLOT (`movl $0,
//  0x8(%esp)` at 0x0048f9da, with %edi pushed, is [orig_esp+4]); the four
//  conditional byte stores then index off it as `%esp+%ecx+3`, `+2`, `+1`,
//  `+0`, i.e. pType[nLen-1], pType[nLen-2], pType[nLen-3], pType[nLen-4].
//  That is a local whose live range VC6 happened to park on the argument --
//  not reachable from source and not meant to be.
//
//  The four guards are `test/jle`, `cmp $1/jle`, `cmp $2/jle`, `cmp $3/jle`:
//  SIGNED, so nLen is an int, not the size_t strlen returns.
// ======================================================================= //

REZTYPE_X380 CRezMgrX380::StrToType(char* sType)
{
	REZTYPE_X380 nType;
	char* pType;
	int   nLen;

	if (sType == NULL_X380) return 0;

	nType = 0;
	nLen  = strlen(sType);
	pType = (char*)&nType;

	if (nLen > 0) pType[nLen - 1] = sType[0];
	if (nLen > 1) pType[nLen - 2] = sType[1];
	if (nLen > 2) pType[nLen - 3] = sType[2];
	if (nLen > 3) pType[nLen - 4] = sType[3];

	return nType;
}


// ======================================================================= //
//  0x0048fa20  CRezMgr::TypeToStr              110 bytes + 2 pad, `ret $8`
//  (enumerated size exact: the `ret $8` at 0x0048fa8b ends at 0x0048fa8e)
//
//  Call-free.  The exact inverse of StrToType, and the argument slot is
//  again the byte array: `mov 0x6(%esp),%cx` loads bytes 2 and 3 of nType as
//  a word (0x0048fa28) and `mov 0x4(%esp),%ecx` (0x0048fa45) reloads it for
//  bytes 0 and 1, so the four `!= 0` tests below are one load each in the
//  source and VC6 merged the pairs.
//
//  There is no `test %eax,%eax` before the first store: VC6 propagated the
//  constants out of the if/else-if chain and sent the nLen==0 arm straight
//  to the terminator at 0x0048fa87.  So `if (nLen > 0)` IS in the source --
//  it is just that the compiler already knew the answer on every path.
// ======================================================================= //

void CRezMgrX380::TypeToStr(REZTYPE_X380 nType, char* sType)
{
	char* pType;
	int   nLen;

	if (sType == NULL_X380) return;

	pType = (char*)&nType;
	nLen  = 0;

	if (pType[3] != '\0')      nLen = 4;
	else if (pType[2] != '\0') nLen = 3;
	else if (pType[1] != '\0') nLen = 2;
	else if (pType[0] != '\0') nLen = 1;

	if (nLen > 0) sType[0] = pType[nLen - 1];
	if (nLen > 1) sType[1] = pType[nLen - 2];
	if (nLen > 2) sType[2] = pType[nLen - 3];
	if (nLen > 3) sType[3] = pType[nLen - 4];

	sType[nLen] = '\0';
}


// ======================================================================= //
//  0x0048fac0  CRezMgr::GetCurTime                      15 bytes, `ret`
//  (MISSED BY THE ENUMERATION.  src/lithshared/rezmgr/rezmgr2.cpp's
//  CRezDir::CreateDir and CreateRezInternal both call it.)
//
//  VERIFIED CALL SEQUENCE (1): 0x0048fac6 time (0x0049545a -- GetLocalTime
//  + GetSystemTime + a timezone fixup).
//
//  `push %ecx` makes the four byte slot, `lea 0x0(%esp),%eax` takes its
//  address, and the single `add $8,%esp` pops the argument AND the slot.
//  The result is returned straight out of %eax and the slot is never read
//  back, so the source returns time()'s value, not the variable.
// ======================================================================= //

REZTIME_X380 CRezMgrX380::GetCurTime()
{
	time_t nTime;

	return (REZTIME_X380)time(&nTime);
}


// ======================================================================= //
//  0x0048fad0  CRezMgr::SetDirSeparators           87 bytes, `ret $4`
//  (enumerated size exact)
//
//  VERIFIED CALL SEQUENCE (2, in address order):
//    0x0048fadd  0x004290c0 operator delete
//    0x0048faf5  0x004290b0 operator new
//  Both strlens and the copy are intrinsic (`repnz scasb` / `rep movs`).
//
//  The `test %eax,%eax ; je 0x0048fae5` at 0x0048fad8 IS A SOURCE `if`, not
//  `delete`'s own null test.  VC6 emits no guard of its own for `delete p`
//  on a type with no destructor -- a bare `delete m_sDirSeparators;` here
//  compiles to `mov ; push ; call` with nothing in front, seven bytes short
//  of the shipped body.  The guard is also what splits the cleanup: shipped
//  pops each call separately (`add $0x4,%esp` at 0x0048fae2 inside the arm
//  and again at 0x0048fb03), where two unguarded cdecl calls batch into one
//  `add $0x8,%esp`.  The same `if (p != NULL) delete p;` spelling is in
//  CRezFile::Close (rezfile.cpp, 0x00490514).
//
//  The allocation size is the `not %ecx` result -- strlen + 1 -- and strlen
//  is computed TWICE (0x0048faf0 and 0x0048fb09) with operator new between
//  them, so the source names it twice.
// ======================================================================= //

void CRezMgrX380::SetDirSeparators(const char* sDirSeparators)
{
	if (m_sDirSeparators != NULL_X380) delete m_sDirSeparators;

	m_sDirSeparators = (char*)::operator new(strlen(sDirSeparators) + 1);
	strcpy(m_sDirSeparators, sDirSeparators);
}


// ======================================================================= //
//  0x0048fb30  CRezDir::GetDirFromPath        458 bytes + 6 pad, `ret $4`
//  (enumerated size exact -- the last `ret $4` at 0x0048fcf7 ends at 0x0048fcfa.
//  Its five `ret $4`s
//  are five early exits, not several functions: an extent check finds no
//  branch target outside the range and no second prologue.)
//
//  VERIFIED CALL SEQUENCE (5, in address order):
//    0x0048fb6b  0x00493660 strchr      (inlined IsGoodChar, leading char)
//    0x0048fbb0  0x00493660 strchr      (inlined IsGoodChar, copy loop)
//    0x0048fc50  0x0048e630 CRezDir::GetDir
//    0x0048fc98  0x00493660 strchr      (inlined IsGoodChar, skip loop)
//    0x0048fce8  0x0048fb30 itself      (recursive, tail of the function)
//  The one strlen is intrinsic.
//
//  Frame 0x404: `this` is spilled to +0x00 and sDir[1024] is +0x04..+0x403.
//  The copy loop is strength-reduced -- %ebx holds (sDir - pPath) and the
//  store is `mov %cl,(%ebx,%esi,1)` -- but the two tests after it index off
//  the ORIGINAL pPath in the argument slot (`0x418(%esp)`), which is only
//  written once, by the leading-separator skip at 0x0048fb85.  So the
//  source indexes pPath[i]; %esi is the compiler's induction variable.
//
//  The leading-separator test used to end `jg <bad> ; jmp <good>` where
//  shipped has `jle <good> ; jmp <bad>` (here and in GetRezFromDosPath).
//  That was IsGoodChar's own spelling: its range test written as one
//  `return a || b || c || d;` instead of four `if (...) return TRUE;` lines.
//
//  The bound is checked AFTER the increment (`cmp $0x3ff,%edi ; jl`) with no
//  entry guard, which is loop rotation of `i < 1023` when i starts at 0.
//
//  THE ODD ONE, TRANSCRIBED AND NOT TIDIED: 0x0048fc17..0x0048fc44 tests
//  `nLen >= 3 && pPath[i+1] == '.' && pPath[i+2] == '.'` and returns `this`
//  (the +0x00 spill, 0x0048fc36).  The two '.'s are at i+1 and i+2, one past
//  the separator that ended the copy.  The code reads plainly but the intent
//  does not add up, so it is written exactly as the displacements have it.
//
//  The character loaded for IsGoodChar (0x0048fb9f) and the character stored
//  into sDir (0x0048fc07) are two separate loads of pPath[i] with the strchr
//  call between them -- two source reads, not one cached.
//
//  THE TRAILING SKIP LOOP HAS TWO `return pDir` EPILOGUES, NOT ONE.  Both
//  are the full fifteen-byte `pop/pop/mov %ebx,%eax/pop/pop/add/ret $4`, at
//  0x0048fc76 and 0x0048fcb2, each FALLEN INTO.  A do/while with the
//  recursion inside it and one `return pDir;` after it (or a while/for with
//  the entry test folded in) merges the two into one epilogue, thirteen
//  bytes short.  The shipped form is `while (!IsGoodChar(..)) { i++; if
//  (end) return pDir; }` with the recursion as the last statement.
// ======================================================================= //

CRezDirX380* CRezDirX380::GetDirFromPath(const char* pPath)
{
	char sDir[1024];
	CRezDirX380* pDir;
	int i;
	int nLen;

	nLen = strlen(pPath);

	if ((nLen > 1) && (!IsGoodChar(pPath[0])))
	{
		pPath++;
		nLen--;
	}

	i = 0;
	while ((i < 1023) && (IsGoodChar(pPath[i])))
	{
		sDir[i] = pPath[i];
		i++;
		nLen--;
	}
	sDir[i] = '\0';

	if ((nLen >= 3) && (pPath[i + 1] == '.') && (pPath[i + 2] == '.'))
		return this;

	pDir = GetDir(sDir);
	if (pDir == NULL_X380) return NULL_X380;

	if (pPath[i] == '\0') return pDir;

	// skip separators; each exit gets its own epilogue (0x0048fc76, 0x0048fcb2)
	while (!IsGoodChar(pPath[i]))
	{
		i++;
		if (pPath[i] == '\0') return pDir;
	}

	return pDir->GetDirFromPath(&pPath[i]);
}


// ======================================================================= //
//  0x0048fd00  CRezDir::GetRezFromDosPath       370 bytes + 14 pad, `ret $4`
//  (enumerated size exact: the last `ret $4` at 0x0048fe6f ends at 0x0048fe72)
//
//  VERIFIED CALL SEQUENCE (7, in address order):
//    0x0048fd4d  0x00493660 strchr             (inlined IsGoodChar)
//    0x0048fd75  0x00493660 strchr             (inlined IsGoodChar)
//    0x0048fdf1  0x00492b60 strncpy
//    0x0048fe0d  0x0048e550 CRezDir::GetRezFromDosName
//    0x0048fe29  0x00492b60 strncpy
//    0x0048fe43  0x0048fb30 CRezDir::GetDirFromPath
//    0x0048fe60  0x0048e550 CRezDir::GetRezFromDosName
//  The strlen at 0x0048fd1a is intrinsic.  0x00492b60 is strncpy and not
//  memcpy -- see the note at the head of this file.
//
//  Frame 0x800: two 1024-byte buffers, the name at +0x000 and the directory
//  at +0x400, no spill slots at all.
//
//  The backward scan starts at nLen-1 (`lea -0x1(%edi),%esi`) and ends on
//  `dec %esi ; jns`, so it is a signed countdown to 0.  IT HAS NO ENTRY
//  GUARD: 0x0048fd60 `lea -0x1(%edi),%esi` is followed straight by the body
//  at 0x0048fd63, so the loop runs once even when nLen is 0 and sPath[-1] is
//  read.  `for (i = nLen - 1; i >= 0; i--)` cannot produce that -- nLen is
//  strlen's result and VC6 cannot prove it positive, so it emits
//  `test %esi,%esi ; jl` in front, four bytes the shipped body does not
//  have.  Written as the do/while it actually is.  The guard that
//  follows it, `cmp %edi,%esi ; jne` -- i == nLen -- CANNOT FIRE: i is at
//  most nLen-1 on the break path and -1 on the fall-out.  It is in the
//  shipped code and it is written below rather than inventing a
//  different comparison to make it reachable.
//
//  `if (nLen >= 1023) return NULL` is the fall-through arm of `jl` -- VC6's
//  usual inversion of `if (cond) return`, not a rearranged source test.
// ======================================================================= //

CRezItmX380* CRezDirX380::GetRezFromDosPath(const char* sPath)
{
	char sDir[1024];
	char sName[1024];
	CRezDirX380* pDir;
	int i;
	int nLen;

	nLen = strlen(sPath);
	if (nLen >= 1023) return NULL_X380;

	if ((nLen > 1) && (!IsGoodChar(sPath[0])))
	{
		sPath++;
		nLen--;
	}

	i = nLen - 1;
	do
	{
		if (!IsGoodChar(sPath[i])) break;
		i--;
	}
	while (i >= 0);

	if (i == nLen) return NULL_X380;

	strncpy(sName, &sPath[i + 1], 1023);
	sName[1023] = '\0';

	if (i <= 1) return GetRezFromDosName(sName);

	strncpy(sDir, sPath, i);
	sDir[i] = '\0';

	pDir = GetDirFromPath(sDir);
	if (pDir == NULL_X380) return NULL_X380;

	return pDir->GetRezFromDosName(sName);
}


// ======================================================================= //
//  0x0048fe80  CRezMgr::GetRezFromDosPath        20 bytes + 12 pad, `ret $4`
//  0x0048fea0  CRezMgr::GetDirFromPath           20 bytes + 12 pad, `ret $4`
//  (BOTH MISSED BY THE ENUMERATION.)
//
//  VERIFIED CALL SEQUENCES (2 each, in address order):
//    0x0048fe85 / 0x0048fea5  0x0048f9b0 CRezMgr::GetRootDir
//    0x0048fe8c                0x0048fd00 CRezDir::GetRezFromDosPath
//    0x0048feac                0x0048fb30 CRezDir::GetDirFromPath
//
//  The `push %eax` at 0x0048fe84 / 0x0048fea4 happens BEFORE the GetRootDir
//  call even though it is the argument of the SECOND call: GetRootDir takes
//  none and pops none, so VC6 hoisted the push.  One statement each.
// ======================================================================= //

CRezItmX380* CRezMgrX380::GetRezFromDosPath(const char* sPath)
{
	return GetRootDir()->GetRezFromDosPath(sPath);
}

CRezDirX380* CRezMgrX380::GetDirFromPath(const char* pPath)
{
	return GetRootDir()->GetDirFromPath(pPath);
}


// ======================================================================= //
//  0x0048fec0  CRezMgr::IsDirectory               60 bytes + 4 pad, `ret $4`
//  (MISSED BY THE ENUMERATION.)
//
//  VERIFIED CALL SEQUENCE (1): 0x0048fecd 0x004955ee _stat.
//
//  `sub $0x24,%esp` is sizeof(struct _stat) on VC6 exactly, and the
//  0x0048fee1 load is a DWORD at +6 masked with 0x4000 -- st_mode is the
//  unsigned short at +6 and 0x4000 is _S_IFDIR; VC6 widened the load and
//  the mask throws the neighbouring st_nlink away.  `sete` off
//  `cmp $0x4000` is the `== _S_IFDIR` written out rather than a plain `!= 0`.
// ======================================================================= //

BOOL_X380 CRezMgrX380::IsDirectory(const char* sFileName)
{
	struct _stat statbuf;

	if (_stat(sFileName, &statbuf) != 0) return FALSE_X380;

	return ((statbuf.st_mode & _S_IFDIR) == _S_IFDIR);
}


// ======================================================================= //
//  0x0048df10  CRezItm::CRezItm                      26 bytes (server.dll
//  0x1003a530, byte-identical).  Lives here, not with the Rz48 half of
//  rezmgr.cpp, because AllocateRezItm below needs its body in the same TU
//  (see there).  Store order +0x34, +0x10, +0x00 is the source order.
//  (lithtech.exe's copy is also carried as ??0CSub48df10View in
//  src/engine/lithtech/ltnew1.cpp, written before the owner was known.)
// ======================================================================= //

CRezItmX380::CRezItmX380()
{
	m_pRezFile   = NULL_X380;
	m_pParentDir = NULL_X380;
	m_sName      = NULL_X380;
	m_heName.SetRezItm(this);
}


// ======================================================================= //
//  0x0048ff00  CRezMgr::AllocateRezItm         259 bytes + 13 pad, `ret`
//  (the enumeration said 257 and cut the last instruction in half: `add $8,%esp`
//  at 0x0048ffff runs to 0x00490002 and the `ret` is that byte, so the body
//  is 0x0048ff00..0x00490002 inclusive)
//
//  VERIFIED CALL SEQUENCE (8, in address order):
//    0x0048ff15  0x004a0560 CBaseHash::GetFirst
//    0x0048ff2b  0x004290b0 operator new            (the chunk, 0xc bytes)
//    0x0048ff51  0x004290b0 operator new            (the array, n * 0x3c)
//    0x0048ff6b  0x0048df10 CRezItm::CRezItm        (in the ctor loop)
//    0x0048ff86  0x004290c0 operator delete         (the chunk, on failure)
//    0x0048ffbb  0x004a04f0 CBaseHash::Insert       (in the seed loop)
//    0x0048ffd5  0x0048de30 CBaseList::InsertFirst
//    0x0048ffde  0x004a0560 CBaseHash::GetFirst     (second time)
//    0x0048fff4  0x004a0530 CBaseHash::Delete
//
//  &m_hashRezItmFreeList is computed once (`lea 0x80(%ebp),%edi`) and parked
//  in a frame slot; that is CSE, not a source variable.
//
//  n * 0x3c is built as `lea (%esi,%esi,2) ; lea (%eax,%eax,4) ; shl $2` --
//  3 * 5 * 4 -- which is VC6 doing sizeof(CRezItm) == 60 by shifts, and the
//  0x3c stride in the ctor loop is the same number.  See the note at the
//  head of this file for why this is written `new CRezItm[n]`.
//
//  The two GetFirst results are treated differently on purpose: the first is
//  null-tested (0x0048ff1c) and the second is NOT (0x0048ffe3 dereferences
//  +0x14 straight away).
//
//  The seed loop reloads pChunk->m_pRezItmAry twice per iteration
//  (0x0048ffa6 and 0x0048ffb3) because the store between them
//  (`mov %eax,0x30(%eax)`) may alias it -- two source references.
//
//  NO EH WRAPPER AROUND THE ARRAY-NEW.  Shipped has no __CxxFrameHandler
//  frame and no state stores around the constructor loop, which is VC6
//  proving CRezItm::CRezItm cannot throw -- and it can only prove that when
//  the constructor's BODY is in the same translation unit, above this
//  function (the original rezmgr.cpp opens with it at 0x0048df10 and ends
//  with AllocateRezItm/DeAllocateRezItm).  Measured: with only the
//  declaration visible VC6 wraps the loop (329 bytes); with the definition
//  above, 259 bytes, size-exact.  `throw()` or __declspec(nothrow) on the
//  declaration does the same, but the definition is what the original had.
//
//  The first GetRezItm IS pRezItm: the non-null path jumps straight to the
//  Delete with it in %esi (0x0048ff23 -> 0x0048ffee), so it is assigned in
//  the condition.  The refill is `GetFirst()->GetRezItm()` with no
//  pHashItm re-assignment (naming it swaps esi/edi throughout), and the
//  null test stays a guard round the Delete sharing the one `return
//  pRezItm` (an early `return NULL` there duplicates the epilogue).  The
//  seed-loop SIB order came right with the allocation.
// ======================================================================= //

CRezItmX380* CRezMgrX380::AllocateRezItm()
{
	CRezItmHashByNameX380* pHashItm;
	CRezItmX380* pRezItm;

	pHashItm = m_hashRezItmFreeList.GetFirst();
	if ((pHashItm == NULL_X380) || ((pRezItm = pHashItm->GetRezItm()) == NULL_X380))
	{
		CRezItmChunkX380* pChunk;
		unsigned int i;

		pChunk = new CRezItmChunkX380;
		if (pChunk == NULL_X380) return NULL_X380;

		pChunk->m_pRezItmAry = new CRezItmX380[m_nRezItmChunkSize];
		if (pChunk->m_pRezItmAry == NULL_X380)
		{
			delete pChunk;
			return NULL_X380;
		}

		for (i = 0; i < m_nRezItmChunkSize; i++)
		{
			pChunk->m_pRezItmAry[i].m_heName.SetRezItm(&pChunk->m_pRezItmAry[i]);
			m_hashRezItmFreeList.Insert(&pChunk->m_pRezItmAry[i].m_heName);
		}

		m_lstRezItmChunks.Insert(pChunk);

		pRezItm = m_hashRezItmFreeList.GetFirst()->GetRezItm();
	}

	if (pRezItm != NULL_X380)
		m_hashRezItmFreeList.Delete(&pRezItm->m_heName);

	return pRezItm;
}


// ======================================================================= //
//  0x00490010  CRezMgr::DeAllocateRezItm        26 bytes + 6 pad, `ret $4`
//  (MISSED BY THE ENUMERATION.  It is the last body of rezmgr.cpp -- the
//  next object starts at 0x00490030.)
//
//  VERIFIED CALL SEQUENCE (1): 0x00490022 0x004a04f0 CBaseHash::Insert.
//
//  `add $0x1c,%eax` is &pItem->m_heName and `add $0x80,%ecx` is
//  &m_hashRezItmFreeList; both are folded into the argument setup, which is
//  why the `this` adjustment happens after the null test.
// ======================================================================= //

void CRezMgrX380::DeAllocateRezItm(CRezItmX380* pItem)
{
	if (pItem != NULL_X380)
		m_hashRezItmFreeList.Insert(&pItem->m_heName);
}


// *********************************************************************** //
//
//  rezhash.cpp starts here.  The object boundary is at 0x00490030: rezmgr.cpp
//  ends with DeAllocateRezItm at 0x00490027 and everything from 0x00490030 to
//  0x004901a0 is hash-table code.  The two bodies below are the part of it
//  inside this file's range; the rest (0x00490110..0x004901a6) is past 0x00490101 and
//  is declared above, not defined -- see the closing note.
//
// *********************************************************************** //


// ======================================================================= //
//  0x00490030  CRezItmHashByName::HashFunc      20 bytes + 12 pad, `ret`
//
//  VERIFIED CALL SEQUENCE (1): 0x0049003e 0x00490180
//  CRezItmHashTableByName::HashFunc(REZNAME).
//
//  `mov 0x14(%ecx),%eax ; test ; jne` over a one byte `ret` is the null
//  guard returning 0 -- %eax already holds the zero it is testing, which is
//  why there is no `xor`.  `mov 0xc(%ecx),%ecx` is GetParentHash(); the
//  table it lands on is used as the by-name table, so the source casts.
// ======================================================================= //

unsigned int CRezItmHashByNameX380::HashFunc()
{
	if (m_pRezItm == NULL_X380) return 0;

	return ((CRezItmHashTableByNameX380*)GetParentHash())->HashFunc(m_pRezItm->GetName());
}


// ======================================================================= //
//  0x00490050  CRezItmHashTableByName::Find    177 bytes + 15 pad, `ret $8`
//              CRezDirHashTable::Find          (folded onto the same bytes)
//  (the enumeration's 0x00490080 row is the back half of this function -- its
//  0x00490099 branches back to 0x0049007a, inside the front half.)
//
//  VERIFIED CALL SEQUENCE (3, in address order):
//    0x0049005f  0x00490180 HashFunc(REZNAME)
//    0x00490067  0x004a0590 CBaseHash::GetFirstInBin
//    0x00490081  0x00492f30 stricmp
//  The second walk's compare is VC6's inlined strcmp (0x004900af), not a
//  call, which is the whole reason the function is written as two loops.
//
//  bIgnoreCase is tested ONCE, before either loop (0x00490072), so the
//  source hoists it rather than testing per element.
//
//  `mov 0x4(%edi),%edi ; test ; je ; add $-4,%edi ; test ; jne` is
//  NextInBin(): CBaseListItem::Next() reads the list link at +0x04 of the
//  element (the vptr owns +0x00), and the -4 with its null guard is the
//  CBaseListItem* -> CBaseHashItem* base adjustment VC6 emits for the cast.
//
//  Both compares put the ELEMENT's name first and sName second (`push %ebp ;
//  push %eax` at 0x0049007d and `cmp %bl,%dl` with %eax = the element's name
//  at 0x004900b5), so that is the argument order in the source.
// ======================================================================= //

CRezItmX380* CRezItmHashTableByNameX380::Find(REZNAME_X380 sName, BOOL_X380 bIgnoreCase)
{
	CRezItmHashByNameX380* pElem;

	if (sName == NULL_X380) return NULL_X380;

	pElem = (CRezItmHashByNameX380*)GetFirstInBin(HashFunc(sName));

	if (bIgnoreCase)
	{
		while (pElem != NULL_X380)
		{
			if (stricmp(pElem->GetRezItm()->GetName(), sName) == 0)
				return pElem->GetRezItm();

			pElem = pElem->NextInBin();
		}
	}
	else
	{
		while (pElem != NULL_X380)
		{
			if (strcmp(pElem->GetRezItm()->GetName(), sName) == 0)
				return pElem->GetRezItm();

			pElem = pElem->NextInBin();
		}
	}

	return NULL_X380;
}


// The directory table's Find is the same source shape over CRezDirHash /
// CRezDir, and CRezDir's name is at +0x00 exactly as CRezItm's is, so the two
// compile to identical bytes and the linker folds them onto 0x00490050.
// Written out because it is real source; it will not produce a second
// function in the image.

CRezDirX380* CRezDirHashTableX380::Find(REZDIRNAME_X380 sName, BOOL_X380 bIgnoreCase)
{
	CRezDirHashX380* pElem;

	if (sName == NULL_X380) return NULL_X380;

	pElem = (CRezDirHashX380*)GetFirstInBin(HashFunc(sName));

	if (bIgnoreCase)
	{
		while (pElem != NULL_X380)
		{
			if (stricmp(pElem->GetRezDir()->GetDirName(), sName) == 0)
				return pElem->GetRezDir();

			pElem = pElem->NextInBin();
		}
	}
	else
	{
		while (pElem != NULL_X380)
		{
			if (strcmp(pElem->GetRezDir()->GetDirName(), sName) == 0)
				return pElem->GetRezDir();

			pElem = pElem->NextInBin();
		}
	}

	return NULL_X380;
}


// *********************************************************************** //
//
//  NOT WRITTEN HERE, and why
//
//  0x0048fa90 / 0x0048faa0 / 0x0048fab0 -- CRezMgr::Alloc, ::Free and
//  ::DiskError, the three "currently not used or implemented!" virtuals --
//  sit inside this file's address range but src/lithshared/rezmgr/rezmgr2.cpp already
//  defines all three (its lines 1479, 1484, 1488).  Declared above,
//  deliberately left undefined, so the two files cannot collide.
//
//  0x00490110 CRezDirHash::HashFunc, 0x00490120
//  CRezTypeHashTable::HashFunc(REZTYPE), 0x00490130
//  CRezTypeHashTable::Find, 0x00490170 CRezTypeHash::HashFunc and 0x00490180
//  CRezItmHashTableByName::HashFunc(REZNAME) are the rest of rezhash.cpp.
//  They are PAST this file's range (which ends at 0x00490101) and two of
//  them are called from the code above, so they are declared and left
//  undefined rather than guessed at.
//  For the record, what the bytes there say: 0x00490120 is
//  `nType % m_nNumBins` with a `div`, 0x00490180 is `strlen(s) % m_nNumBins`
//  with the same `div` behind a null guard, and 0x00490110 / 0x00490170 are
//  the two element overrides that forward to them -- the same three-line
//  shape as 0x00490030 above.
//
//  0x00490740 is in rezfile.cpp (0x004901b0..0x00490abd), which is nobody's
//  range yet.  Declared, not defined.
//
// *********************************************************************** //
