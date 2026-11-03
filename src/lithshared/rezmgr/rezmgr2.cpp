// *********************************************************************** //
//
//  MODULE:   REZMGR (.CPP) -- 0x0048e720..0x0048f37a of lithtech.exe
//
//  WHAT THE MODULE IS
//
//  The 50-function run 0x0048d210..0x00490abd is Monolith's own RezMgr
//  library -- the reader for the .rez archives the game ships its data in.
//  Its header is in this repository already, unmodified, at
//  src/LT2/lithshared/incs/rezmgr.h, and the two collection headers it is built
//  on are src/lithshared/lith/basehash.h and src/LT2/lithshared/lith/virtlist.h.
//  The library is compiled LITE (`REZMGRLITE`): every `#ifndef REZMGRLITE`
//  member of rezmgr.h is absent from the shipped layouts below.
//
//  HOW IT WAS IDENTIFIED -- from the data, and then confirmed field by field
//
//   1. The only string literals the whole run touches are "\\", "*.*", ".",
//      "..", "rb", "r+b" and "w+b" (0x0048f3ec, 0x0048f451, 0x0048f4a2,
//      0x0048f4dc, 0x0049042a..0x0049043c, 0x00490a4d..0x00490a67): a
//      directory walker and three fopen modes. That is CRezMgr's
//      "directory emulation" mode and CRezFile.
//
//   2. Every external caller of the run sits in 0x00420c00..0x00421830,
//      which clienttexmgr.cpp already pins as src/engine/server/filemgr.cpp
//      (fs_OpenTree 0x00420b70, fs_CloseTree 0x00420cd0, ar_FindNext
//      0x004212d0). filemgr.cpp *already declares* CRezItm / CRezDir /
//      CRezTyp against the server.dll copies of this same library, and
//      calls its archive object `CFileArchive`, 0xd4 bytes. 0xd4 is exactly
//      sizeof(CRezMgr) as laid out below.
//
//   3. 0x0048ed50 is reached from inside fs_OpenTree and is a constructor
//      that writes 2000000000 to +0x28 and 3 to +0x2c. rezmgr.h:
//      "m_nNextIDNumToUse ... defaults to 2000000000" and
//      m_nMaxOpenFilesInEmulatedDir. Its vtable (0x004a3b7c) has exactly
//      three slots -- `return 0` / `return` / `return 0` -- which is
//      CRezMgr's Alloc / Free / DiskError, the three virtuals rezmgr.h
//      marks "currently not used or implemented!". Its last store is a
//      single zero byte at +0x94, which is m_sUserTitle[0] with
//      RezMgrUserTitleSize (60) + 1 bytes after it: 0x94 + 61 = 0xd1 -> 0xd4.
//
//   4. 0x0048e300 takes eight arguments in the order (CRezMgr*, CRezDir*,
//      name, nFilePos, nSize, nTime, nDirNumHashBins, nTypNumHashBins) and
//      0x0048df30 takes eleven in the order (pParentDir, sName, nID, pType,
//      sDesc, nSize, nFilePos, nTime, nNumKeys, pKeyAry, pRezFile). Those
//      are CRezDir::CRezDir and CRezItm::InitRezItm from rezmgr.h,
//      parameter for parameter.
//
//   5. filemgr.cpp's six unnamed CRezDir methods 0x1003ac80, 0x1003aca0,
//      0x1003acc0, 0x1003ace0, 0x1003ad00, 0x1003ad20 fall in rezmgr.h's
//      declaration order: GetFirstSubDir, GetNextSubDir, GetFirstType,
//      GetNextType, GetFirstItem, GetNextItem.
//
//  THE RUN IS NOT ONE MODULE, and the seams are visible:
//
//      0x0048d210..0x0048de2f  NOT RezMgr. This is the tail of the unnamed
//                              subsystem lttexmod.cpp already describes,
//                              whose base constructor is 0x0048c6e0 and
//                              whose vtable is 0x004a38d8; every outbound
//                              call from it lands in 0x0048c540..0x0048cd50
//                              or on 0x00411d40. The two import thunks the
//                              linker parks at 0x0048db30
//                              (DirectInputCreateA) and 0x0048de20
//                              (_AIL_shutdown) sit at its end, which is the
//                              usual marker for an object boundary.
//      0x0048de30..0x0048df0f  src/lithshared/lith/baselist.cpp, four
//                              functions and nothing else:
//                              CBaseList::InsertFirst (0x0048de30),
//                              ::InsertLast (0x0048de60),
//                              ::InsertAfter (0x0048de90),
//                              ::Delete (0x0048dee0). Two hundred call
//                              sites across the image reach the first and
//                              the last, which is what a shared collection
//                              primitive looks like.
//      0x0048df10..0x0048ffc3  rezmgr.cpp. It opens with CRezItm::CRezItm
//                              at 0x0048df10, whose `movl $0x4a3b70,
//                              0x1c(%eax)` installs m_heName's vtable at
//                              +0x1c and whose `mov %eax,0x30(%eax)` ties
//                              that element's back pointer to the item --
//                              independent proof of the CRezItm layout
//                              below. It closes with
//                              CRezMgr::DeAllocateRezItm at 0x00490010.
//      0x00490030..0x004901a0  rezhash.cpp: three HashFunc overrides
//                              (0x00490030, 0x00490110, 0x00490170) and the
//                              two Find bodies (0x00490050, 0x00490130).
//      0x004901b0..0x00490abd  rezfile.cpp: CBaseRezFile (vtable 0x004a3b90,
//                              nine slots, seven of them purecall), CRezFile
//                              (0x004a3bb4) and CRezFileDirectoryEmulation
//                              (0x004a3bd8).
//
//  All nine of these functions belong in rezmgr.cpp, and so does the whole
//  of the 0x0048df30..0x0048e62a chunk beside them.
//
//  WHERE THIS BELONGS -- READ BEFORE MERGING.
//
//  The chunk beside this one is
//  **src/lithshared/rezmgr/rezmgr.cpp**, built by build/libs.sh and dropped
//  into BOTH targets, with its types suffixed `Rz48`. That file is the home
//  for everything below; this file is a standalone TU. NONE of these nine functions is in it -- it stops at
//  0x0048e62a (its last body is CRezDir::GetRezFromDosName at 0x0048e550) --
//  so nothing here is duplicated work.
//
//  To merge: rename `X720` -> `Rz48` throughout and delete the copies here of the
//  declarations it already has. Its CRezItm, CRezTyp, CRezDir, CBaseHash,
//  CBaseHashItem and the three hash elements/four hash tables agree with
//  these member for member and offset for offset (independently derived; the
//  agreement is worth something on its own). What it does NOT have and this
//  file supplies:
//
//    * the complete CRezMgr -- it carries only `unsigned char
//      m_pad_00[0x68]` plus m_bLowerCaseUsed/m_bItemByIDUsed. Every one of
//      the 30 members below is pinned by the constructor at 0x0048ed50, the
//      destructor at 0x0048ee40 and Open at 0x0048ef80, and the total comes
//      out at the 0xd4 filemgr.cpp already measured.
//    * CVirtBaseList / CVirtBaseListItem (src/LT2/lithshared/lith/virtlist.h)
//      and CBaseRezFileList, which is why m_lstRezFiles is TWELVE bytes and
//      the whole CRezMgr layout lands where it does.
//    * CBaseRezFile's nine-slot vtable and the two concrete sizes.
//    * RezFileHeader, the 0xa8-byte on-disk header.
//
//  IT NEEDS /GX, like the rest of that library. Six of the nine open with
//  VC6's three-dword __CxxFrameHandler registration (0x0048e720,
//  0x0048e980, 0x0048ec80, 0x0048ed50, 0x0048ee40, 0x0048ef80) and carry EH
//  state stores, because each has a `new` with a constructor that must free
//  the block if the constructor throws. build/engine.sh's `lithtech)`
//  recipe has no /GX and would drop all six frames.
//
//  BOTH BINARIES SCORE THESE. rezmgr.lib is prebuilt and linked into
//  lithtech.exe and server.dll, so every one of the nine has a
//  byte-identical twin in the retail server.dll, located for all nine by
//  searching the retail server.dll for a relocation-free window out of each:
//
//      0x0048e720 -> 0x1003ad40      0x0048ec80 -> 0x1003b2a0
//      0x0048e7f0 -> 0x1003ae10      0x0048ed50 -> 0x1003b370
//      0x0048e870 -> 0x1003ae90      0x0048ee40 -> 0x1003b470
//      0x0048e8c0 -> 0x1003aee0      0x0048ef80 -> 0x1003b5b0
//      0x0048e980 -> 0x1003afa0
//
//  The last three are the clincher, because src/engine/server/filemgr.cpp
//  named those three addresses without knowing what they were:
//  "CFileArchive(); // 0x1003b370", "~CFileArchive(); // 0x1003b470" and
//  "LTBOOL Open(char* pName, int nFlag, int n); // 0x1003b5b0". They are
//  CRezMgr::CRezMgr, CRezMgr::~CRezMgr and CRezMgr::Open -- and filemgr.cpp's
//  note that "the second and third arguments are fixed at 1 and 0 by the only
//  caller, so what they select is not visible from here" is answered here:
//  they are ReadOnly and CreateNew, and the LITE build refuses any open that
//  is not read-only at its first instruction.
//
//  TRUE EXTENTS. One enumerated row is wrong: 0x0048e980 is 759 bytes
//  (0x0048e980..0x0048ec76), not 710 -- an extent check finds six branch
//  targets past 0x0048ec46 and the next function starts at 0x0048ec80.
//  Everything else matched its stated size exactly.
//
//  ALLOCATION. 0x004290b0 is `operator new` and 0x004290c0 is
//  `operator delete` (clientinput.cpp / lt2new1.cpp already
//  name them). Every allocation in this range reaches 0x004290b0 DIRECTLY,
//  including the three that allocate raw byte/dword buffers -- so those are
//  spelled `::operator new(n)` and not `new T[n]`, which would have gone
//  through operator new[]'s own thunk and named a different symbol. The
//  `new CRezDir` / `new CRezTyp` / `new CRezFile` sites are real `new`
//  expressions: each carries its own EH state in the frame's unwind slot,
//  which is the compiler's "free the block if the constructor throws".
//
//  NAMING. Every type and file-scope name in this file carries an `X720`
//  suffix so it cannot collide with the two sibling chunks of the same run
//  (0x0048d210.. and 0x0048df30..) if all three are compiled together.
//  Strip the suffix to get the real name out of rezmgr.h.
//
// *********************************************************************** //

#include <string.h>


// ======================================================================= //
//  lithtypes.h
// ======================================================================= //

typedef int                 BOOL_X720;
typedef unsigned long       DWORD_X720;
typedef unsigned char       BYTE_X720;

#define TRUE_X720   1
#define FALSE_X720  0
#define NULL_X720   0


// ======================================================================= //
//  reztypes.h -- the typedef set rezmgr.h's prototypes are written in.
//  Only the widths are load-bearing and every one of them is a dword or a
//  char pointer in the shipped code.
// ======================================================================= //

typedef char*               REZNAME_X720;
typedef char*               REZDIRNAME_X720;
typedef char*               REZDESC_X720;
typedef DWORD_X720          REZTYPE_X720;
typedef DWORD_X720          REZID_X720;
typedef DWORD_X720          REZSIZE_X720;
typedef DWORD_X720          REZTIME_X720;
typedef DWORD_X720          REZKEYVAL_X720;

#define RezMgrUserTitleSize_X720 60


// ======================================================================= //
//  src/LT2/lithshared/lith/virtlist.h
//
//  m_pFirst at +0x04 and m_pLast at +0x08 of the list, m_pNext at +0x04 and
//  m_pPrev at +0x08 of the item: both carry a vptr at +0x00 because of the
//  pure `VirtualFoo`. 0x004a05e0 (Delete) and 0x004a05b0 (InsertFirst) read
//  exactly those offsets, and ~CRezMgr's last store parks the list's vptr on
//  the one-slot purecall vtable at 0x004a3b8c, which is CVirtBaseList's.
// ======================================================================= //

class CVirtBaseListItemX720
{
public:
	CVirtBaseListItemX720* Next() { return m_pNext; }
	CVirtBaseListItemX720* Prev() { return m_pPrev; }

	virtual void VirtualFoo() = 0;

public:
	friend class CVirtBaseListX720;

protected:
	CVirtBaseListItemX720* m_pNext;          // +0x04
	CVirtBaseListItemX720* m_pPrev;          // +0x08
};

class CVirtBaseListX720
{
public:
	CVirtBaseListX720() { m_pFirst = NULL_X720; m_pLast = NULL_X720; }
	~CVirtBaseListX720() { }

	void Insert(CVirtBaseListItemX720* pItem) { InsertFirst(pItem); }
	void InsertFirst(CVirtBaseListItemX720* pItem);                 // 0x004a05b0
	void InsertLast(CVirtBaseListItemX720* pItem);
	void Delete(CVirtBaseListItemX720* pItem);                      // 0x004a05e0

	CVirtBaseListItemX720* GetFirst() { return m_pFirst; }
	CVirtBaseListItemX720* GetLast()  { return m_pLast; }

	virtual void VirtualFoo() = 0;

private:
	CVirtBaseListItemX720* m_pFirst;         // +0x04
	CVirtBaseListItemX720* m_pLast;          // +0x08
};


// ======================================================================= //
//  src/lithshared/lith/baselist.h and basehash.h
//
//  CBaseHash is { m_nNumBins, m_pBinAry } and nothing else -- 8 bytes -- and
//  that is what makes sizeof(CRezDir) come out at the 0x4c the shipped
//  allocations ask for. The bin stride is 0x10 and m_lstItems sits at +0x08
//  of a bin (0x004a04f0, 0x004a0530, 0x004a0560, 0x004a0590 all agree).
//  CBaseHashItem's vptr is at +0x00 because HashFunc is its first virtual,
//  so the list links are at +0x04/+0x08 and the item is 0x14 bytes; every
//  rez hash element adds one back pointer at +0x14 and is 0x18.
// ======================================================================= //

class CBaseListItemX720
{
public:
	CBaseListItemX720* Next() { return m_pNext; }
	CBaseListItemX720* Prev() { return m_pPrev; }

public:
	friend class CBaseListX720;

protected:
	CBaseListItemX720* m_pNext;              // +0x00
	CBaseListItemX720* m_pPrev;              // +0x04
};

class CBaseListX720
{
public:
	CBaseListX720() { m_pFirst = NULL_X720; m_pLast = NULL_X720; }
	~CBaseListX720() { }

	void Insert(CBaseListItemX720* pItem) { InsertFirst(pItem); }
	void InsertFirst(CBaseListItemX720* pItem);                     // 0x0048de30
	void InsertLast(CBaseListItemX720* pItem);
	void Delete(CBaseListItemX720* pItem);                          // 0x0048dee0

	CBaseListItemX720* GetFirst() { return m_pFirst; }
	CBaseListItemX720* GetLast()  { return m_pLast; }

protected:
	CBaseListItemX720* m_pFirst;             // +0x00
	CBaseListItemX720* m_pLast;              // +0x04
};

class CBaseHashItemX720;

class CBaseHashX720
{
public:
	CBaseHashX720();
	CBaseHashX720(unsigned int NumBins);                            // 0x004a0430
	~CBaseHashX720();                                               // 0x004a04c0

	void               Insert(CBaseHashItemX720* pItem);            // 0x004a04f0
	void               Delete(CBaseHashItemX720* pItem);            // 0x004a0530
	CBaseHashItemX720* GetFirst();                                  // 0x004a0560
	CBaseHashItemX720* GetLast();

protected:
	CBaseHashItemX720* GetFirstInBin(unsigned int Bin);             // 0x004a0590
	unsigned int       GetNumBins() { return m_nNumBins; }

private:
	friend class CBaseHashItemX720;

	unsigned int       m_nNumBins;           // +0x00
	void*              m_pBinAry;            // +0x04
};

class CBaseHashItemX720 : public CBaseListItemX720
{
public:
	CBaseHashItemX720* Next();                                      // 0x004a03d0
	CBaseHashItemX720* Prev();

protected:
	virtual unsigned int HashFunc() = 0;

	CBaseHashX720* GetParentHash() { return m_pParentHash; }

private:
	friend class CBaseHashX720;

	CBaseHashX720* m_pParentHash;            // +0x0c
	unsigned int   m_nCurBin;                // +0x10
};


// ======================================================================= //
//  rezhash.h -- the four tables and three elements rezmgr.cpp reaches.
//  Every Find below is out of line in rezhash.cpp. 0x00490050 serves BOTH
//  by-name tables: CRezDirHashTable::Find and CRezItmHashTableByName::Find
//  compile to the same bytes (hash the string at 0x00490180, walk the bin
//  from 0x004a0590, compare, answer the element's owner at +0x14 -- and the
//  owner's name is at +0x00 of every one of those owners), so the linker
//  folded them. 0x00490130 is the type table's, keyed on a dword.
//
//  The second argument to the by-name Find selects the comparison: nonzero
//  calls 0x00492f30, zero runs VC6's inlined strcmp. 0x00492f30 folds A-Z
//  on both operands (`sub $0x41; cmp $0x1a; sbb; and $0x20; add`) and is
//  therefore _stricmp, so the flag means IGNORE CASE -- not "case
//  sensitive", which is what src/lithshared/rezmgr/rezmgr.cpp currently
//  calls it. CRezDir::CreateDir passing `!m_pRezMgr->m_bLowerCaseUsed` then
//  reads correctly: an archive not known to hold lower case has to be
//  matched case-blind. CRezDir::ReadDirBlock passes a literal 1 for items.
// ======================================================================= //

class CRezItmX720;
class CRezTypX720;
class CRezDirX720;
class CRezMgrX720;
class CBaseRezFileX720;

class CRezItmHashByNameX720 : public CBaseHashItemX720
{
public:
	CRezItmHashByNameX720* Next() { return (CRezItmHashByNameX720*)CBaseHashItemX720::Next(); }
	CRezItmX720* GetRezItm() { return m_pRezItm; }

protected:
	virtual unsigned int HashFunc();                                // 0x00490030

private:
	friend class CRezItmX720;
	CRezItmX720* m_pRezItm;                  // +0x14
};

class CRezDirHashX720 : public CBaseHashItemX720
{
public:
	CRezDirHashX720* Next() { return (CRezDirHashX720*)CBaseHashItemX720::Next(); }
	CRezDirX720* GetRezDir() { return m_pRezDir; }

protected:
	virtual unsigned int HashFunc();                                // 0x00490110

public:
	friend class CRezDirX720;
	CRezDirX720* m_pRezDir;                  // +0x14
};

class CRezTypeHashX720 : public CBaseHashItemX720
{
public:
	CRezTypeHashX720* Next() { return (CRezTypeHashX720*)CBaseHashItemX720::Next(); }
	CRezTypX720* GetRezTyp() { return m_pRezTyp; }

protected:
	virtual unsigned int HashFunc();                                // 0x00490170

private:
	friend class CRezTypX720;
	CRezTypX720* m_pRezTyp;                  // +0x14
};

class CRezItmHashTableByNameX720 : public CBaseHashX720
{
public:
	CRezItmHashTableByNameX720() { }
	CRezItmHashTableByNameX720(unsigned int NumBins) : CBaseHashX720(NumBins) { }

	CRezItmX720* Find(REZNAME_X720 sName, BOOL_X720 bIgnoreCase);    // 0x00490050
	CRezItmHashByNameX720* GetFirst() { return (CRezItmHashByNameX720*)CBaseHashX720::GetFirst(); }
};

class CRezItmHashTableByIDX720 : public CBaseHashX720
{
public:
	CRezItmHashTableByIDX720() { }
	CRezItmHashTableByIDX720(unsigned int NumBins) : CBaseHashX720(NumBins) { }

	CRezItmX720* Find(REZID_X720 nID);
};

class CRezDirHashTableX720 : public CBaseHashX720
{
public:
	CRezDirHashTableX720() { }
	CRezDirHashTableX720(unsigned int NumBins) : CBaseHashX720(NumBins) { }

	CRezDirX720* Find(REZDIRNAME_X720 sName, BOOL_X720 bIgnoreCase); // 0x00490050
	CRezDirHashX720* GetFirst() { return (CRezDirHashX720*)CBaseHashX720::GetFirst(); }
};

class CRezTypeHashTableX720 : public CBaseHashX720
{
public:
	CRezTypeHashTableX720() { }
	CRezTypeHashTableX720(unsigned int NumBins) : CBaseHashX720(NumBins) { }

	CRezTypX720* Find(REZTYPE_X720 nType);                          // 0x00490130
	CRezTypeHashX720* GetFirst() { return (CRezTypeHashX720*)CBaseHashX720::GetFirst(); }
};


// ======================================================================= //
//  rezfile.h -- only as much as rezmgr.cpp needs.
//
//  Nine vtable slots. Slot 0 is CVirtBaseListItem::VirtualFoo, slot 1 is the
//  virtual destructor (`push $1; call *0x4(%eax)` in ~CRezMgr), slot 2 is the
//  block read CRezDir::ReadDirBlock issues as `call *0x8(%eax)` with four
//  arguments, and slot 4 is the open CRezMgr::Open issues as
//  `call *0x10(%eax)` with three. Slots 3, 5, 6, 7 and 8 are never reached
//  from this file; they are declared only to put 2 and 4 at the right
//  offsets and their names are placeholders.
//
//  The two concrete sizes are what `new` asks for in CRezMgr::Open:
//  0x24 for CRezFile and 0x38 for CRezFileDirectoryEmulation.
// ======================================================================= //

class CBaseRezFileX720 : public CVirtBaseListItemX720
{
public:
	virtual ~CBaseRezFileX720();                                          // slot 1

	// UNDECODED: the second argument is 0 at the only site this file
	// reaches, so what it selects is not visible from here.
	virtual DWORD_X720 Read(DWORD_X720 Pos, DWORD_X720 nUnknown,
	                        DWORD_X720 Size, void* pData);                // slot 2
	virtual DWORD_X720 Write(DWORD_X720 Pos, DWORD_X720 nUnknown,
	                         DWORD_X720 Size, void* pData);               // slot 3
	virtual BOOL_X720  Open(const char* sFileName, BOOL_X720 bReadOnly,
	                        BOOL_X720 bCreateNew);                        // slot 4
	virtual BOOL_X720  Close();                                           // slot 5
	virtual BOOL_X720  Flush();                                           // slot 6
	virtual BOOL_X720  VerifyFileOpen();                                  // slot 7
	virtual const char* GetFileName();                                    // slot 8

	virtual void VirtualFoo();
};

class CRezFileX720 : public CBaseRezFileX720
{
public:
	CRezFileX720(CRezMgrX720* pRezMgr);                             // 0x00490210

private:
	BYTE_X720 m_pad_0c[0x24 - 0x0c];
};

class CRezFileDirectoryEmulationX720 : public CBaseRezFileX720
{
public:
	CRezFileDirectoryEmulationX720(CRezMgrX720* pRezMgr, int nMaxOpenFiles);  // 0x004905e0

private:
	BYTE_X720 m_pad_0c[0x38 - 0x0c];
};

class CBaseRezFileListX720 : public CVirtBaseListX720
{
public:
	CBaseRezFileX720* GetFirst() { return (CBaseRezFileX720*)CVirtBaseListX720::GetFirst(); }

	virtual void VirtualFoo() { }                                   // 0x00490200
};


// ======================================================================= //
//  rezmgr.h, LITE build. Every offset below is read off the shipped code;
//  the comment on each member names the instruction that proves it where
//  this file reaches it at all.
//
//  CRezItm  0x3c    (m_heName at +0x1c: `lea 0x1c(%ebx),%edx` at 0x0048e838)
//  CRezTyp  0x30    (m_heType at +0x04: `lea 0x4(%edi),%eax` at 0x0048ed20,
//                    m_haName at +0x24: `lea 0x24(%ebp),%ecx` at 0x0048eb29)
//  CRezDir  0x4c    (the size `new` asks for at 0x0048e760 / 0x0048ea58 /
//                    0x0048f0a7 / 0x0048f1a6 / 0x0048f2f6)
//  CRezMgr  0xd4    (filemgr.cpp's CFileArchive; m_sUserTitle[0] is the
//                    byte store at +0x94 that ends the constructor)
// ======================================================================= //

class CRezItmX720
{
public:
	REZNAME_X720  GetName() { return m_sName; }
	REZSIZE_X720  GetSize() { return m_nSize; }
	REZTIME_X720  GetTime() { return m_nTime; }
	CRezDirX720*  GetParentDir() { return m_pParentDir; }

private:
	CRezItmX720();

	void InitRezItm(CRezDirX720* pParentDir, REZNAME_X720 sName, REZID_X720 nID,
	                CRezTypX720* pType, REZDESC_X720 sDesc, REZSIZE_X720 nSize,
	                DWORD_X720 nFilePos, REZTIME_X720 nTime, DWORD_X720 nNumKeys,
	                REZKEYVAL_X720* pKeyAry, CBaseRezFileX720* pRezFile);  // 0x0048df30
	void TermRezItm();                                                    // 0x0048dfc0

	friend class CRezTypX720;
	friend class CRezDirX720;
	friend class CRezMgrX720;

	void MarkCurTime();

	REZNAME_X720          m_sName;           // +0x00
	CRezTypX720*          m_pType;           // +0x04
	REZTIME_X720          m_nTime;           // +0x08
	REZSIZE_X720          m_nSize;           // +0x0c
	CRezDirX720*          m_pParentDir;      // +0x10
	DWORD_X720            m_nFilePos;        // +0x14
	DWORD_X720            m_nCurPos;         // +0x18
	CRezItmHashByNameX720 m_heName;          // +0x1c
	CBaseRezFileX720*     m_pRezFile;        // +0x34
	BYTE_X720*            m_pData;           // +0x38
};

class CRezTypX720
{
public:
	REZTYPE_X720 GetType() { return m_nType; }

private:
	CRezTypX720(REZTYPE_X720 nType, CRezDirX720* pParentDir,
	            unsigned int nByIDNumHashBins, unsigned int nByNameNumHashBins);  // 0x0048e120
	CRezTypX720(REZTYPE_X720 nType, CRezDirX720* pParentDir,
	            unsigned int nByNameNumHashBins);                                 // 0x0048e1b0
	~CRezTypX720();

	friend class CRezItmX720;
	friend class CRezDirX720;
	friend class CRezMgrX720;

	REZTYPE_X720               m_nType;      // +0x00
	CRezTypeHashX720           m_heType;     // +0x04
	CRezItmHashTableByIDX720   m_haID;       // +0x1c
	CRezItmHashTableByNameX720 m_haName;     // +0x24
	CRezDirX720*               m_pParentDir; // +0x2c
};

class CRezDirX720
{
public:
	REZDIRNAME_X720 GetDirName() { return m_sDirName; }
	CRezDirX720*    GetParentDir() { return m_pParentDir; }
	CRezMgrX720*    GetParentMgr() { return m_pRezMgr; }

	CRezItmX720* GetRez(REZNAME_X720 sRes, REZTYPE_X720 Type);
	CRezItmX720* GetRezFromDosName(char* sDosname);
	CRezItmX720* GetRezFromPath(const char* sPath, REZTYPE_X720 type);
	CRezItmX720* GetRezFromDosPath(const char* sPath);

	BOOL_X720 Load(BOOL_X720 LoadAllSubDirs = FALSE_X720);
	BOOL_X720 UnLoad(BOOL_X720 UnLoadAllSubDirs = FALSE_X720);
	BOOL_X720 IsLoaded() { return (m_pMemBlock != NULL_X720); }

	CRezDirX720* GetDir(const char* sDir);
	CRezDirX720* GetDirFromPath(const char* pPath);
	CRezDirX720* GetFirstSubDir();
	CRezDirX720* GetNextSubDir(CRezDirX720* pRezDir);

	CRezTypX720* GetRezTyp(REZTYPE_X720 Type);
	CRezTypX720* GetFirstType();
	CRezTypX720* GetNextType(CRezTypX720* pRezType);

	CRezItmX720* GetFirstItem(CRezTypX720* pRezType);
	CRezItmX720* GetNextItem(CRezItmX720* pRezItm);

	CRezDirX720* CreateDir(REZDIRNAME_X720 sDir);                    // 0x0048e720
	CRezItmX720* CreateRez(REZID_X720 ID, REZNAME_X720 Name, REZTYPE_X720 Type);
	REZTIME_X720 GetTime() { return m_nLastTimeModified; }

private:
	CRezDirX720(CRezMgrX720* pRezMgr, CRezDirX720* pParentDir, REZDIRNAME_X720 szDirName,
	            DWORD_X720 nFilePos, DWORD_X720 nSize, REZTIME_X720 nTime,
	            unsigned int nDirNumHashBins, unsigned int nTypNumHashBins);   // 0x0048e300
	~CRezDirX720();                                                           // 0x0048e3e0

	friend class CRezItmX720;
	friend class CRezTypX720;
	friend class CRezMgrX720;

	BOOL_X720 ReadAllDirs(CBaseRezFileX720* pRezFile, DWORD_X720 Pos,
	                      DWORD_X720 Size, BOOL_X720 bOverwriteItems);        // 0x0048e8c0
	BOOL_X720 ReadDirBlock(CBaseRezFileX720* pRezFile, DWORD_X720 Pos,
	                       DWORD_X720 Size, BOOL_X720 bOverwriteItems);       // 0x0048e980
	CRezTypX720* GetOrMakeTyp(REZTYPE_X720 nType);                            // 0x0048ec80
	BOOL_X720    IsGoodChar(char c);
	CRezItmX720* CreateRezInternal(REZID_X720 nID, REZNAME_X720 sName,
	                               CRezTypX720* pTyp, CBaseRezFileX720* pRezFile); // 0x0048e7f0
	BOOL_X720    RemoveRezInternal(CRezTypX720* pTyp, CRezItmX720* pItm);         // 0x0048e870

	REZDIRNAME_X720      m_sDirName;            // +0x00
	DWORD_X720           m_nDirPos;             // +0x04
	DWORD_X720           m_nDirSize;            // +0x08
	DWORD_X720           m_nItemsPos;           // +0x0c
	DWORD_X720           m_nItemsSize;          // +0x10
	REZTIME_X720         m_nLastTimeModified;   // +0x14
	CRezMgrX720*         m_pRezMgr;             // +0x18
	CRezDirX720*         m_pParentDir;          // +0x1c
	CRezDirHashX720      m_heDir;               // +0x20
	CRezDirHashTableX720 m_haDir;               // +0x38
	CRezTypeHashTableX720 m_haTypes;            // +0x40
	BYTE_X720*           m_pMemBlock;           // +0x48
};

class CRezMgrX720
{
public:
	CRezMgrX720();                                                            // 0x0048ed50
	CRezMgrX720(const char* FileName, BOOL_X720 ReadOnly = TRUE_X720,
	            BOOL_X720 CreateNew = FALSE_X720);
	~CRezMgrX720();                                                           // 0x0048ee40

	BOOL_X720 Open(const char* FileName, BOOL_X720 ReadOnly = TRUE_X720,
	               BOOL_X720 CreateNew = FALSE_X720);                         // 0x0048ef80
	BOOL_X720 OpenAdditional(const char* FileName, BOOL_X720 bOverwriteItems = FALSE_X720);
	BOOL_X720 Close(BOOL_X720 bCompact = FALSE_X720);                         // 0x0048f900
	CRezDirX720* GetRootDir();
	BOOL_X720 IsOpen() { return m_bFileOpened; }
	BOOL_X720 VerifyFileOpen();
	REZTIME_X720 GetTime() { return m_nLastTimeModified; }
	BOOL_X720 Reset();
	BOOL_X720 IsSorted() { return m_bIsSorted; }

	CRezItmX720* GetRezFromPath(const char* sPath, REZTYPE_X720 type);
	CRezItmX720* GetRezFromDosPath(const char* sPath);
	CRezDirX720* GetDirFromPath(const char* pPath);

	void SetDirSeparators(const char* sDirSeparators);

	virtual void* Alloc(DWORD_X720 NumBytes);                                 // 0x0048fa90
	virtual void  Free(void* Ptr);                                            // 0x0048faa0
	virtual BOOL_X720 DiskError();                                            // 0x0048fab0

	REZTYPE_X720 StrToType(char* pStr);
	void         TypeToStr(REZTYPE_X720 nType, char* sType);

	void SetRenumberIDCollisions(BOOL_X720 bFlag);
	void SetNextIDNumber(DWORD_X720 nID);

	BOOL_X720 GetLowerCaseUsed() { return m_bLowerCaseUsed; }
	void SetLowerCaseUsed(BOOL_X720 bLowerCaseUsed) { m_bLowerCaseUsed = bLowerCaseUsed; }

	BOOL_X720 GetItemByIDUsed() { return m_bItemByIDUsed; }
	void SetItemByIDUsed(BOOL_X720 bItemByIDUsed) { m_bItemByIDUsed = bItemByIDUsed; }

	void SetHashTableBins(unsigned int nByNameNumHashBins, unsigned int nByIDNumHashBins,
	                      unsigned int nDirNumHashBins, unsigned int nTypNumHashBins);

	void ForceIsSortedFlag(BOOL_X720 bFlag) { m_bIsSorted = bFlag; }
	void SetMaxOpenFilesInEmulatedDir(int nNumFiles) { m_nMaxOpenFilesInEmulatedDir = nNumFiles; }

	void  SetUserTitle(const char* sUserTitle);
	char* GetUserTitle() { return m_sUserTitle; }

private:
	friend class CRezDirX720;
	friend class CRezTypX720;
	friend class CRezItmX720;

	// class CRezItmChunk / CRezItmChunkList. The chunk's array pointer is at
	// +0x08 of the chunk, which is what ~CRezMgr frees before unlinking it,
	// so the CBaseListItem links are at +0x00/+0x04 exactly as baselist.h
	// declares them.
	class CRezItmChunkX720 : public CBaseListItemX720
	{
	public:
		CRezItmChunkX720* Next() { return (CRezItmChunkX720*)CBaseListItemX720::Next(); }
		CRezItmX720* m_pRezItmAry;           // +0x08
	};

	class CRezItmChunkListX720 : public CBaseListX720
	{
	public:
		CRezItmChunkX720* GetFirst() { return (CRezItmChunkX720*)CBaseListX720::GetFirst(); }
	};

	CRezItmX720* AllocateRezItm();                                  // 0x0048ff00
	void         DeAllocateRezItm(CRezItmX720* pItem);              // 0x00490010

	REZTIME_X720 GetCurTime();                                      // 0x0048fac0
	BOOL_X720    IsDirectory(const char* sFileName);                // 0x0048fec0
	BOOL_X720    ReadEmulationDirectory(CRezFileDirectoryEmulationX720* pRezFileEmulation,
	                                    CRezDirX720* pDir, char* sParamPath,
	                                    BOOL_X720 bOverwriteItems); // 0x0048f380

	char*                      m_sDirSeparators;           // +0x04
	BOOL_X720                  m_bIsSorted;                // +0x08
	BOOL_X720                  m_bFileOpened;              // +0x0c
	CBaseRezFileListX720       m_lstRezFiles;              // +0x10 (vptr,first,last)
	DWORD_X720                 m_nNumRezFiles;             // +0x1c
	CBaseRezFileX720*          m_pPrimaryRezFile;          // +0x20
	BOOL_X720                  m_bRenumberIDCollisions;    // +0x24
	DWORD_X720                 m_nNextIDNumToUse;          // +0x28
	int                        m_nMaxOpenFilesInEmulatedDir; // +0x2c
	DWORD_X720                 m_nRootDirPos;              // +0x30
	DWORD_X720                 m_nRootDirSize;             // +0x34
	REZTIME_X720               m_nRootDirTime;             // +0x38
	DWORD_X720                 m_nNextWritePos;            // +0x3c
	BOOL_X720                  m_bReadOnly;                // +0x40
	CRezDirX720*               m_pRootDir;                 // +0x44
	REZTIME_X720               m_nLastTimeModified;        // +0x48
	BOOL_X720                  m_bMustReWriteDirs;         // +0x4c
	DWORD_X720                 m_nFileFormatVersion;       // +0x50
	DWORD_X720                 m_nLargestKeyAry;           // +0x54
	DWORD_X720                 m_nLargestDirNameSize;      // +0x58
	DWORD_X720                 m_nLargestRezNameSize;      // +0x5c
	DWORD_X720                 m_nLargestCommentSize;      // +0x60
	char*                      m_sFileName;                // +0x64
	BOOL_X720                  m_bLowerCaseUsed;           // +0x68
	BOOL_X720                  m_bItemByIDUsed;            // +0x6c
	unsigned int               m_nByNameNumHashBins;       // +0x70
	unsigned int               m_nByIDNumHashBins;         // +0x74
	unsigned int               m_nDirNumHashBins;          // +0x78
	unsigned int               m_nTypNumHashBins;          // +0x7c
	CRezItmHashTableByNameX720 m_hashRezItmFreeList;       // +0x80
	CRezItmChunkListX720       m_lstRezItmChunks;          // +0x88
	unsigned int               m_nRezItmChunkSize;         // +0x90
	char                       m_sUserTitle[RezMgrUserTitleSize_X720 + 1];  // +0x94 (0xd1 -> 0xd4)
};


// ======================================================================= //
//  The on-disk header CRezMgr::Open reads.
//
//  0xa8 bytes, and the field offsets are the literal displacements of the
//  eleven loads at 0x0048f219..0x0048f28f plus the three byte compares at
//  0x0048f2c2, 0x0048f2d0 and 0x0048f2de. None of the dwords is 4-aligned,
//  so the structure is packed; the three 62/60-byte character runs and the
//  CR/LF/EOF sentinels are the usual "this is not a text file" banner.
// ======================================================================= //

#pragma pack(push, 1)
struct RezFileHeaderX720
{
	char       cCR1;                          // +0x00  0x0d
	char       sMessage[62];                  // +0x01
	char       cLF1;                          // +0x3f  0x0a
	char       sUserTitle[RezMgrUserTitleSize_X720];  // +0x40
	char       cCR2;                          // +0x7c
	char       cLF2;                          // +0x7d
	char       cEOF;                          // +0x7e  0x1a
	DWORD_X720 nFileFormatVersion;            // +0x7f
	DWORD_X720 nRootDirPos;                   // +0x83
	DWORD_X720 nRootDirSize;                  // +0x87
	DWORD_X720 nRootDirTime;                  // +0x8b
	DWORD_X720 nNextWritePos;                 // +0x8f
	DWORD_X720 nTime;                         // +0x93
	DWORD_X720 nLargestKeyAry;                // +0x97
	DWORD_X720 nLargestDirNameSize;           // +0x9b
	DWORD_X720 nLargestRezNameSize;           // +0x9f
	DWORD_X720 nLargestCommentSize;           // +0xa3
	BYTE_X720  bIsSorted;                     // +0xa7
};
#pragma pack(pop)


// ======================================================================= //
//  0x0048e720  CRezDir::CreateDir                                206 bytes
//
//  Calls, in order (5): 0x00490050 CRezDirHashTable::Find,
//  0x004290b0 operator new, 0x0048fac0 CRezMgr::GetCurTime,
//  0x0048e300 CRezDir::CRezDir, 0x004a04f0 CBaseHash::Insert.
//
//  Two shapes the bytes decide. The duplicate-name test jumps to the create
//  block and falls through to `return NULL`, so the source tests for the
//  case that CONTINUES; and the two hash-bin counts are pushed before
//  GetCurTime is called, which is just right-to-left argument evaluation of
//  the constructor call.
//
//  The name-length update reads `cmp %eax,%ecx; ja` with the manager's field
//  in eax, so the comparison is written with m_nLargestDirNameSize on the
//  left; and the length is held in a LOCAL.  0x0048e7c7 is the only
//  `repnz scasb` in the function and 0x0048e7d3 is `inc %ecx` on the same
//  register the compare just read -- VC6 does not CSE two written-out
//  `strlen(sDir)` calls (measured: it emits the scan twice), so one scan
//  plus an in-place increment is a named length the `+ 1` is applied to.
//  CreateRezInternal below has the identical shape for sName.
//
//  server.dll 0x1003ad40.  The lower-case flag goes through the in-class
//  GetLowerCaseUsed() accessor: with the bare member VC6 loads it into %ebx
//  (`mov 0x68(%eax),%ebx`), with the accessor into %eax as shipped.
//  The source was always right. The 38-byte residual (the unwind epilogue
//  copied into both `return NULL` sites where shipped jumps, `xor %eax,%eax ;
//  jmp`, to one shared exit) came from CreateDir being the FIRST function
//  compiled in this TU. cl 8447 /O2 /GX tail-duplicates the EH epilogue of
//  the first function of a translation unit whose last return is not a
//  constant. Any function compiled before it stops this. That was measured
//  on a toy TU: the same body gets three epilogues when it is first and one
//  when a one-line `int f(int a) { return a + 1; }` comes before it; the
//  order of two identical bodies decides which one duplicates. In shipped,
//  CreateDir is not first. It follows the seven CRezDir accessors at
//  0x0048e630..0x0048e716, whose scored copies are in rezhash.cpp, and
//  before those the whole of rezmgr.cpp: one original rezmgr.cpp split
//  across files here. So the accessor right before it, GetNextItem at
//  0x0048e700, is compiled here too, in its original position, as the
//  function that precedes CreateDir. Its COMDAT is unreferenced and /OPT:REF
//  drops it from the image. Merging this file into rezmgr.cpp would make
//  this copy redundant.
// ======================================================================= //

//  0x0048e700  CRezDir::GetNextItem  (scored copy: rezhash.cpp, CRezDirQ48C)
CRezItmX720* CRezDirX720::GetNextItem(CRezItmX720* pRezItm)
{
	CRezItmHashByNameX720* pElem = pRezItm->m_heName.Next();

	if (pElem == NULL_X720)
		return NULL_X720;

	return pElem->GetRezItm();
}

CRezDirX720* CRezDirX720::CreateDir(REZDIRNAME_X720 sDir)
{
	// a directory of this name already here is a failure, not a find
	if (m_haDir.Find(sDir, !m_pRezMgr->GetLowerCaseUsed()) != NULL_X720)
		return NULL_X720;

	CRezDirX720* pDir = new CRezDirX720(m_pRezMgr, this, sDir, 0, 0,
	                                    m_pRezMgr->GetCurTime(),
	                                    m_pRezMgr->m_nDirNumHashBins,
	                                    m_pRezMgr->m_nTypNumHashBins);
	if (pDir == NULL_X720)
		return NULL_X720;

	m_haDir.Insert(&pDir->m_heDir);

	DWORD_X720 nLen = strlen(sDir);

	if (m_pRezMgr->m_nLargestDirNameSize <= nLen)
		m_pRezMgr->m_nLargestDirNameSize = nLen + 1;

	return pDir;
}


// ======================================================================= //
//  0x0048e7f0  CRezDir::CreateRezInternal                        117 bytes
//
//  Calls, in order (4): 0x0048ff00 CRezMgr::AllocateRezItm,
//  0x0048fac0 CRezMgr::GetCurTime, 0x0048df30 CRezItm::InitRezItm,
//  0x004a04f0 CBaseHash::Insert.
//
//  InitRezItm's eleven arguments are pushed right to left as
//  (this, sName, nID, pTyp, NULL, 0, 0, GetCurTime(), 0, NULL, pRezFile) --
//  rezmgr.h's parameter list exactly. Unlike CreateDir the early exit
//  carries no `xor %eax,%eax`: `mov %eax,%ebx` at 0x0048e7fc has already
//  told VC6 that eax holds the null it is about to return.
// ======================================================================= //

CRezItmX720* CRezDirX720::CreateRezInternal(REZID_X720 nID, REZNAME_X720 sName,
                                            CRezTypX720* pTyp, CBaseRezFileX720* pRezFile)
{
	CRezItmX720* pItm = m_pRezMgr->AllocateRezItm();
	if (pItm == NULL_X720)
		return NULL_X720;

	pItm->InitRezItm(this, sName, nID, pTyp, NULL_X720, 0, 0,
	                 m_pRezMgr->GetCurTime(), 0, NULL_X720, pRezFile);

	pTyp->m_haName.Insert(&pItm->m_heName);

	DWORD_X720 nLen = strlen(sName);

	if (m_pRezMgr->m_nLargestRezNameSize <= nLen)
		m_pRezMgr->m_nLargestRezNameSize = nLen + 1;

	return pItm;
}


// ======================================================================= //
//  0x0048e870  CRezDir::RemoveRezInternal                         71 bytes
//
//  Calls, in order (3): 0x004a0530 CBaseHash::Delete,
//  0x0048dfc0 CRezItm::TermRezItm, 0x00490010 CRezMgr::DeAllocateRezItm.
//
//  m_pRezMgr is loaded twice (0x0048e89a and 0x0048e8a3) with the
//  DeAllocateRezItm call between them, so the two uses are written as two
//  separate member accesses rather than cached in a local.
// ======================================================================= //

BOOL_X720 CRezDirX720::RemoveRezInternal(CRezTypX720* pTyp, CRezItmX720* pItm)
{
	m_nItemsSize -= pItm->m_nSize;

	pTyp->m_haName.Delete(&pItm->m_heName);

	pItm->TermRezItm();
	m_pRezMgr->DeAllocateRezItm(pItm);

	m_pRezMgr->m_bIsSorted = FALSE_X720;

	return TRUE_X720;
}


// ======================================================================= //
//  0x0048e8c0  CRezDir::ReadAllDirs                              178 bytes
//
//  Calls, in order (6): 0x004a0560 CBaseHash::GetFirst,
//  0x004a03d0 CBaseHashItem::Next, 0x0048e980 CRezDir::ReadDirBlock,
//  0x004a0560 CBaseHash::GetFirst, 0x0048e8c0 (itself, recursive),
//  0x004a03d0 CBaseHashItem::Next.
//
//  `cmp %ebp,%ebx; jbe` on a zeroed ebp is `Size > 0`, not `Size != 0`.
//  Both walks are the ordinary pretest-plus-do-while the two `GetFirst` /
//  `Next` pairs make; the recursion is entered through the hash element's
//  back pointer at +0x14, which is why `this` never has to be reloaded.
//
//  Two shapes are read off the control flow.  `Size <= 0` is an EARLY
//  RETURN: the zero-size exit jumps past `pop %edi` (0x0048e8d7 ->
//  0x0048e96b), so %edi is only saved inside the work, which a wrapping
//  `if (Size > 0)` block does not give.  And the failed ReadDirBlock is the
//  ELSE of a positive test: shipped stores the FALSE to bRetVal's slot at
//  the very end (0x0048e95e) and falls into the loop's `mov 0x10(%esp),
//  %eax` exit; written `if (!ReadDirBlock) bRetVal = FALSE; else ...` VC6
//  returns the call's zero straight from %eax instead (166 bytes).
// ======================================================================= //

BOOL_X720 CRezDirX720::ReadAllDirs(CBaseRezFileX720* pRezFile, DWORD_X720 Pos,
                                   DWORD_X720 Size, BOOL_X720 bOverwriteItems)
{
	BOOL_X720 bRetVal = TRUE_X720;
	CRezDirHashX720* pHash;

	if (Size <= 0)
		return bRetVal;

	// every sub directory starts out unseen
	for (pHash = m_haDir.GetFirst(); pHash != NULL_X720; pHash = pHash->Next())
		pHash->m_pRezDir->m_nDirPos = 0;

	if (ReadDirBlock(pRezFile, Pos, Size, bOverwriteItems))
	{
		// and now the ones this block mentioned have a position again
		for (pHash = m_haDir.GetFirst(); pHash != NULL_X720; pHash = pHash->Next())
		{
			CRezDirX720* pDir = pHash->m_pRezDir;

			if (pDir->m_nDirPos != 0)
			{
				if (!pDir->ReadAllDirs(pRezFile, pDir->m_nDirPos,
				                       pDir->m_nDirSize, bOverwriteItems))
					bRetVal = FALSE_X720;
			}
		}
	}
	else
	{
		bRetVal = FALSE_X720;
	}

	return bRetVal;
}


// ======================================================================= //
//  0x0048e980  CRezDir::ReadDirBlock         759 bytes, NOT the 710 listed
//
//  True extent 0x0048e980..0x0048ec76: an extent check flags six branch targets
//  past 0x0048ec46 (0x0048ec62, 0x0048ec54, 0x0048ec46) and the next
//  function starts at 0x0048ec80.
//
//  Calls, in order (16, one of them indirect): 0x004290b0 operator new,
//  [indirect] pRezFile->Read (vtable slot 2, `call *0x8(%eax)`),
//  0x004290c0 operator delete, 0x00490050 CRezDirHashTable::Find,
//  0x004290b0 operator new, 0x0048e300 CRezDir::CRezDir,
//  0x004a04f0 CBaseHash::Insert, 0x0048ec80 CRezDir::GetOrMakeTyp,
//  0x00490050 CRezItmHashTableByName::Find, 0x0048e870 RemoveRezInternal,
//  0x004290b0 operator new, 0x0048ff00 CRezMgr::AllocateRezItm,
//  0x0048df30 CRezItm::InitRezItm, 0x004a04f0 CBaseHash::Insert,
//  0x004290c0 operator delete, 0x004290c0 operator delete.
//
//  THE BLOCK FORMAT, read straight off the walk. Every entry starts with a
//  dword tag; 1 is a directory and anything else is a resource.  The tag is
//  compared in memory (`cmpl $0x1,(%esi)` at 0x0048e9d1, with the following
//  loads biased by four), so the cursor has NOT been advanced when the test
//  runs: the `pCurPos += sizeof(DWORD)` that consumes the tag is the first
//  statement of each arm, not a statement between the read and the `if`.
//  Put it between them and the tag has to be materialised in a register
//  (`mov (%esi),%eax; add $4,%esi; cmp $1,%eax`) because the pointer it came
//  from has changed underneath it.
//
//      directory:  1, nDirPos, nDirSize, nDirTime, name\0
//      resource:   0, nFilePos, nSize, nTime, nID, nType, nNumKeys,
//                  name\0, description\0, nNumKeys dwords of key values
//
//  An empty description string becomes a null pointer (0x0048eb67 reads the
//  first byte and 0x0048eb78 stores the null), which is what InitRezItm's
//  REZDESC parameter wants.
//
//  nLastPos (the stack slot at entry-0x2c, zeroed at 0x0048e9af, maximised
//  at 0x0048ec31) is written and never read. It is transcribed because it is
//  there; the non-LITE build is presumably where it gets used.
//
//  The `new CRezDir` here has no null check before `m_haDir.Insert(&pDir->
//  m_heDir)` -- 0x0048ea9a is a bare `add $0x20,%eax` on the possibly-null
//  pointer. That is VC6's `new`: the null test guards only the constructor
//  call, and taking a member's address afterwards is just an add.
// ======================================================================= //

BOOL_X720 CRezDirX720::ReadDirBlock(CBaseRezFileX720* pRezFile, DWORD_X720 Pos,
                                    DWORD_X720 Size, BOOL_X720 bOverwriteItems)
{
	m_nItemsSize = 0;
	m_nItemsPos  = 0xffffffff;

	DWORD_X720 nLastPos = 0;

	BYTE_X720* pBlock = (BYTE_X720*)::operator new(Size);
	if (pBlock == NULL_X720)
		return FALSE_X720;

	if (pRezFile->Read(Pos, 0, Size, pBlock) != Size)
	{
		delete pBlock;
		return FALSE_X720;
	}

	BYTE_X720* pEndPos = pBlock + Size;
	BYTE_X720* pCurPos = pBlock;

	while (pCurPos < pEndPos)
	{
		DWORD_X720 nEntryType = *((DWORD_X720*)pCurPos);

		if (nEntryType == 1)
		{
			pCurPos += sizeof(DWORD_X720);

			DWORD_X720   nDirPos  = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);
			DWORD_X720   nDirSize = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);
			REZTIME_X720 nDirTime = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);

			REZDIRNAME_X720 sDirName = (REZDIRNAME_X720)pCurPos;
			pCurPos += strlen(sDirName) + 1;

			CRezDirX720* pDir = m_haDir.Find(sDirName, !m_pRezMgr->m_bLowerCaseUsed);

			if (pDir == NULL_X720)
			{
				pDir = new CRezDirX720(m_pRezMgr, this, sDirName, nDirPos, nDirSize,
				                       nDirTime, m_pRezMgr->m_nDirNumHashBins,
				                       m_pRezMgr->m_nTypNumHashBins);
				m_haDir.Insert(&pDir->m_heDir);
			}
			else
			{
				pDir->m_nDirPos           = nDirPos;
				pDir->m_nDirSize          = nDirSize;
				pDir->m_nLastTimeModified = nDirTime;
			}
		}
		else
		{
			pCurPos += sizeof(DWORD_X720);

			DWORD_X720   nFilePos = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);
			REZSIZE_X720 nSize    = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);
			REZTIME_X720 nTime    = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);
			REZID_X720   nID      = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);
			REZTYPE_X720 nType    = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);
			DWORD_X720   nNumKeys = *((DWORD_X720*)pCurPos);
			pCurPos += sizeof(DWORD_X720);

			REZNAME_X720 sName = (REZNAME_X720)pCurPos;
			pCurPos += strlen(sName) + 1;

			CRezTypX720* pTyp = GetOrMakeTyp(nType);

			BOOL_X720 bSkip = FALSE_X720;

			CRezItmX720* pOldItm = pTyp->m_haName.Find(sName, TRUE_X720);
			if (pOldItm != NULL_X720)
			{
				if (bOverwriteItems)
					RemoveRezInternal(pTyp, pOldItm);
				else
					bSkip = TRUE_X720;
			}

			REZDESC_X720 sDesc = (REZDESC_X720)pCurPos;
			pCurPos += strlen(sDesc) + 1;
			if (*sDesc == '\0')
				sDesc = NULL_X720;

			REZKEYVAL_X720* pKeyAry;

			if (nNumKeys > 0)
			{
				pKeyAry = (REZKEYVAL_X720*)::operator new(nNumKeys * sizeof(REZKEYVAL_X720));

				for (DWORD_X720 nKey = 0; nKey < nNumKeys; nKey++)
				{
					pKeyAry[nKey] = *((DWORD_X720*)pCurPos);
					pCurPos += sizeof(DWORD_X720);
				}
			}
			else
			{
				pKeyAry = NULL_X720;
			}

			if (!bSkip)
			{
				CRezItmX720* pItm = m_pRezMgr->AllocateRezItm();

				pItm->InitRezItm(this, sName, nID, pTyp, sDesc, nSize, nFilePos,
				                 nTime, nNumKeys, pKeyAry, pRezFile);

				pTyp->m_haName.Insert(&pItm->m_heName);

				m_nItemsSize += pItm->m_nSize;

				if (pItm->m_nFilePos < m_nItemsPos)
					m_nItemsPos = pItm->m_nFilePos;

				if (pItm->m_nFilePos > nLastPos)
					nLastPos = pItm->m_nFilePos;
			}

			if (pKeyAry != NULL_X720)
				delete pKeyAry;
		}
	}

	delete pBlock;

	return TRUE_X720;
}


// ======================================================================= //
//  0x0048ec80  CRezDir::GetOrMakeTyp                             194 bytes
//
//  Calls, in order (6): 0x00490130 CRezTypeHashTable::Find,
//  0x004290b0 operator new, 0x0048e120 CRezTyp::CRezTyp (four argument),
//  0x004290b0 operator new, 0x0048e1b0 CRezTyp::CRezTyp (three argument),
//  0x004a04f0 CBaseHash::Insert.
//
//  This is what proves the two CRezTyp constructors rezmgr.h declares are
//  both real and which is which: the arm taken when m_bItemByIDUsed is set
//  pushes both bin counts (nByIDNumHashBins from +0x74, nByNameNumHashBins
//  from +0x70) and the other pushes only +0x70. `push $0x30` is hoisted
//  above the test because both arms allocate the same size.
// ======================================================================= //

CRezTypX720* CRezDirX720::GetOrMakeTyp(REZTYPE_X720 nType)
{
	CRezTypX720* pTyp = m_haTypes.Find(nType);

	if (pTyp == NULL_X720)
	{
		if (m_pRezMgr->m_bItemByIDUsed)
			pTyp = new CRezTypX720(nType, this, m_pRezMgr->m_nByIDNumHashBins,
			                       m_pRezMgr->m_nByNameNumHashBins);
		else
			pTyp = new CRezTypX720(nType, this, m_pRezMgr->m_nByNameNumHashBins);

		// an early refusal, not a guard round the Insert: that keeps pTyp in
		// one register (%edi) for both paths, and the state store at
		// 0x0048ecca stores that known-zero %edi
		if (pTyp == NULL_X720) return NULL_X720;
		m_haTypes.Insert(&pTyp->m_heType);
	}

	return pTyp;
}


// ======================================================================= //
//  0x0048ed50  CRezMgr::CRezMgr                                  226 bytes
//
//  Calls (1): 0x004a0430 CBaseHash::CBaseHash(unsigned int), for
//  m_hashRezItmFreeList with one bin. m_lstRezFiles and m_lstRezItmChunks
//  get their inline list constructors (the zeroed pairs at +0x14/+0x18 and
//  +0x88/+0x8c) and the vptr for the list goes to 0x004a3b88.
//
//  This constructor is the strongest single identification in the file:
//  2000000000 at +0x28 is rezmgr.h's documented default for
//  m_nNextIDNumToUse, and the zero byte at +0x94 is m_sUserTitle[0].
//
//  ONE THING NOT PINNED: the shipped code emits the +0x70/+0x74 pair
//  (19, 19) before the vptr store at +0x00, i.e. before anything else in the
//  body. They are written below in declaration order and VC6's block
//  scheduler has to hoist them -- they share the one `mov $0x13,%eax` and
//  everything else in the block runs off the zero in ebx. If the assembly
//  comes out with them in place rather than at the top, the source order is
//  wrong and they belong first.
// ======================================================================= //

CRezMgrX720::CRezMgrX720()
	: m_hashRezItmFreeList(1)
{
	m_bFileOpened                = FALSE_X720;
	m_bRenumberIDCollisions      = TRUE_X720;
	m_nNextIDNumToUse            = 2000000000;
	m_pPrimaryRezFile            = NULL_X720;
	m_nNumRezFiles               = 0;
	m_nRootDirPos                = 0;
	m_nNextWritePos              = 0;
	m_bReadOnly                  = TRUE_X720;
	m_pRootDir                   = NULL_X720;
	m_nLastTimeModified          = 0;
	m_bMustReWriteDirs           = FALSE_X720;
	m_nFileFormatVersion         = 0;
	m_nLargestKeyAry             = 0;
	m_nLargestDirNameSize        = 0;
	m_nLargestRezNameSize        = 0;
	m_nLargestCommentSize        = 0;
	m_bIsSorted                  = TRUE_X720;
	m_sFileName                  = NULL_X720;
	m_nMaxOpenFilesInEmulatedDir = 3;
	m_sDirSeparators             = NULL_X720;
	m_bLowerCaseUsed             = FALSE_X720;
	m_bItemByIDUsed              = FALSE_X720;
	m_nByNameNumHashBins         = 19;
	m_nByIDNumHashBins           = 19;
	m_nDirNumHashBins            = 5;
	m_nTypNumHashBins            = 9;
	m_nRezItmChunkSize           = 100;
	m_sUserTitle[0]              = '\0';
}


// ======================================================================= //
//  0x0048ee40  CRezMgr::~CRezMgr                                 319 bytes
//
//  Calls, in order (11, one indirect): 0x0048f900 CRezMgr::Close,
//  0x004a05e0 CVirtBaseList::Delete, [indirect] the rez file's virtual
//  destructor (`push $1; call *0x4(%eax)`), 0x0048e3e0 CRezDir::~CRezDir,
//  0x004290c0 operator delete, 0x004290c0, 0x004290c0, 0x004290c0,
//  0x0048dee0 CBaseList::Delete, 0x004290c0, 0x004a04c0 CBaseHash::~CBaseHash.
//
//  The member resets in the middle repeat work the three `if (p) { delete p;
//  p = NULL; }` blocks above them have already done (m_pRootDir and
//  m_sFileName are cleared twice) and set m_nFileFormatVersion to 1 where
//  the constructor set it to 0. That is CRezMgr::Reset's body, inlined; it
//  is written out in place here because Reset itself is not in this range.
//
//  `delete pChunk->m_pRezItmAry` and `delete pBlock` get no null test and
//  the three pointers above them do: MSVC omits the test when the type has
//  no destructor to run and the programmer did not write one, and the ones
//  that have a test have an explicit `if` in the source.
// ======================================================================= //

CRezMgrX720::~CRezMgrX720()
{
	if (m_bFileOpened)
		Close(FALSE_X720);

	// throw away every low level rez file
	CBaseRezFileX720* pRezFile = m_lstRezFiles.GetFirst();
	while (pRezFile != NULL_X720)
	{
		m_lstRezFiles.Delete(pRezFile);
		m_nNumRezFiles--;

		delete pRezFile;

		pRezFile = m_lstRezFiles.GetFirst();
	}

	if (m_pRootDir != NULL_X720)
	{
		delete m_pRootDir;
		m_pRootDir = NULL_X720;
	}

	if (m_sFileName != NULL_X720)
	{
		delete m_sFileName;
		m_sFileName = NULL_X720;
	}

	if (m_sDirSeparators != NULL_X720)
	{
		delete m_sDirSeparators;
		m_sDirSeparators = NULL_X720;
	}

	// Reset(), inlined
	m_bFileOpened         = FALSE_X720;
	m_pPrimaryRezFile     = NULL_X720;
	m_nRootDirPos         = 0;
	m_nRootDirSize        = 0;
	m_nRootDirTime        = 0;
	m_nNextWritePos       = 0;
	m_bReadOnly           = TRUE_X720;
	m_pRootDir            = NULL_X720;
	m_nLastTimeModified   = 0;
	m_bMustReWriteDirs    = FALSE_X720;
	m_nFileFormatVersion  = 1;
	m_nLargestKeyAry      = 0;
	m_nLargestDirNameSize = 0;
	m_nLargestRezNameSize = 0;
	m_nLargestCommentSize = 0;
	m_bIsSorted           = TRUE_X720;
	m_sFileName           = NULL_X720;

	// and the rez item chunks
	CRezItmChunkX720* pChunk = m_lstRezItmChunks.GetFirst();
	while (pChunk != NULL_X720)
	{
		delete pChunk->m_pRezItmAry;

		m_lstRezItmChunks.Delete(pChunk);

		delete pChunk;

		pChunk = m_lstRezItmChunks.GetFirst();
	}
}


// ======================================================================= //
//  0x0048ef80  CRezMgr::Open                                   1,018 bytes
//
//  Calls, in order (24, three indirect): 0x004290c0 operator delete,
//  0x004290b0 operator new, 0x0048fec0 CRezMgr::IsDirectory,
//  0x004290b0, 0x004905e0 CRezFileDirectoryEmulation::CRezFileDirectoryEmulation,
//  0x004290c0, 0x004a05b0 CVirtBaseList::InsertFirst,
//  [indirect] pRezFile->Open (slot 4, `call *0x10(%eax)`),
//  0x004290b0, 0x0048fac0 CRezMgr::GetCurTime, 0x0048e300 CRezDir::CRezDir,
//  0x0048f380 CRezMgr::ReadEmulationDirectory,
//  0x004290b0, 0x00490210 CRezFile::CRezFile, 0x004290c0,
//  0x004a05b0 CVirtBaseList::InsertFirst,
//  [indirect] pRezFile->Open (slot 4),
//  0x004290b0, 0x0048fac0 CRezMgr::GetCurTime, 0x0048e300 CRezDir::CRezDir,
//  [indirect] pRezFile->Read (slot 2, `call *0x8(%edx)`),
//  0x004290b0, 0x0048e300 CRezDir::CRezDir,
//  0x0048e8c0 CRezDir::ReadAllDirs.
//
//  The two `!ReadOnly` tests are both real and the second is dead: the
//  function refuses a writable open at the top and the emulation branch asks
//  again from the member. That is what a `#ifdef REZMGRLITE` pair looks like
//  after the preprocessor has been through it.
//
//  The five `new` expressions carry EH states 0,1,2,3,4 in that order, which
//  is what fixes the emulation branch as the FIRST arm of the `if`.
//
//  The trailing-space strip on the user title is an integer index loop, not
//  a pointer walk: 0x0048f2bb recomputes `p - m_sUserTitle` and tests it
//  signed, which is VC6's strength-reduced `i >= 0`.
//
//  ONE THING NOT PINNED: the sorted flag at header offset +0xa7 is
//  loaded as a dword and masked with 0xff (0x0048f27f, 0x0048f290). A
//  `BYTE` field assigned to a `BOOL` is what that should be -- VC6 avoids
//  8-bit loads into 32-bit destinations to dodge partial-register stalls --
//  but a `DWORD` field with an explicit `& 0xFF` in the source would produce
//  the same two instructions. The byte reading is the one that keeps
//  sizeof(RezFileHeader) at the 0xa8 the read asks for.
//
//  CreateNew and the header read are an if/else sharing the final `return
//  TRUE`: the CreateNew arm jumps to the `mov $1,%eax` at 0x0048f35a.  An
//  early `return TRUE` in that arm keeps the 1 in %edi across the new
//  instead of %eax (four bytes long).  The header copy's statement order
//  (next-write position first, format version after the time) is read off
//  the /O2 store schedule at 0x0048f219..0x0048f2a0; header order does not
//  schedule the same.
// ======================================================================= //

BOOL_X720 CRezMgrX720::Open(const char* FileName, BOOL_X720 ReadOnly, BOOL_X720 CreateNew)
{
	m_bReadOnly = ReadOnly;

	if (!ReadOnly)
		return FALSE_X720;

	if (m_sFileName != NULL_X720)
		delete m_sFileName;

	m_sFileName = (char*)::operator new(strlen(FileName) + 1);
	strcpy(m_sFileName, FileName);

	if (IsDirectory(FileName))
	{
		if (!m_bReadOnly)
			return FALSE_X720;

		CRezFileDirectoryEmulationX720* pRezFile =
			new CRezFileDirectoryEmulationX720(this, m_nMaxOpenFilesInEmulatedDir);

		if (pRezFile == NULL_X720)
		{
			delete m_sFileName;
			m_sFileName = NULL_X720;
			return FALSE_X720;
		}

		m_pPrimaryRezFile = pRezFile;
		m_lstRezFiles.Insert(pRezFile);
		m_nNumRezFiles++;

		if (!pRezFile->Open(FileName, ReadOnly, CreateNew))
			return FALSE_X720;

		m_bFileOpened = TRUE_X720;

		m_pRootDir = new CRezDirX720(this, NULL_X720, "", 0, 0, GetCurTime(),
		                             m_nDirNumHashBins, m_nTypNumHashBins);

		ReadEmulationDirectory(pRezFile, m_pRootDir, m_sFileName, FALSE_X720);

		return TRUE_X720;
	}

	CRezFileX720* pRezFile = new CRezFileX720(this);

	if (pRezFile == NULL_X720)
	{
		delete m_sFileName;
		m_sFileName = NULL_X720;
		return FALSE_X720;
	}

	m_pPrimaryRezFile = pRezFile;
	m_lstRezFiles.Insert(pRezFile);
	m_nNumRezFiles++;

	if (!pRezFile->Open(FileName, ReadOnly, CreateNew))
		return FALSE_X720;

	m_bFileOpened = TRUE_X720;

	if (CreateNew)
	{
		m_nNextWritePos    = sizeof(RezFileHeaderX720);
		m_bMustReWriteDirs = TRUE_X720;

		m_pRootDir = new CRezDirX720(this, NULL_X720, "", 0, 0, GetCurTime(),
		                             m_nDirNumHashBins, m_nTypNumHashBins);
	}
	else
	{
		RezFileHeaderX720 Header;

		pRezFile->Read(0, 0, sizeof(RezFileHeaderX720), &Header);

		m_nNextWritePos       = Header.nNextWritePos;
		m_nRootDirPos         = Header.nRootDirPos;
		m_nRootDirSize        = Header.nRootDirSize;
		m_nRootDirTime        = Header.nRootDirTime;
		m_nLastTimeModified   = Header.nTime;
		m_nFileFormatVersion  = Header.nFileFormatVersion;
		m_nLargestKeyAry      = Header.nLargestKeyAry;
		m_nLargestDirNameSize = Header.nLargestDirNameSize;
		m_nLargestRezNameSize = Header.nLargestRezNameSize;
		m_nLargestCommentSize = Header.nLargestCommentSize;
		m_bIsSorted           = Header.bIsSorted;

		memcpy(m_sUserTitle, Header.sUserTitle, RezMgrUserTitleSize_X720);
		m_sUserTitle[RezMgrUserTitleSize_X720] = '\0';

		// the title is blank padded on disk
		for (int nChar = RezMgrUserTitleSize_X720 - 1; nChar >= 0; nChar--)
		{
			if (m_sUserTitle[nChar] != ' ')
				break;

			m_sUserTitle[nChar] = '\0';
		}

		if (Header.cCR1 != 0x0d)
			return FALSE_X720;

		if (Header.cLF1 != 0x0a)
			return FALSE_X720;

		if (Header.cEOF != 0x1a)
			return FALSE_X720;

		if (m_nFileFormatVersion != 1)
			return FALSE_X720;

		m_pRootDir = new CRezDirX720(this, NULL_X720, "", m_nRootDirPos, m_nRootDirSize,
		                             m_nRootDirTime, m_nDirNumHashBins, m_nTypNumHashBins);

		m_pRootDir->ReadAllDirs(pRezFile, m_nRootDirPos, m_nRootDirSize, FALSE_X720);
	}

	return TRUE_X720;
}


// ======================================================================= //
//  The three virtuals of CRezMgr, needed here only so that the vtable
//  ~CRezMgr installs at 0x0048ee60 has something to point at. Their shipped
//  bodies are 0x0048fa90 (`xor %eax,%eax; ret $4`), 0x0048faa0 (`ret $4`)
//  and 0x0048fab0 (`xor %eax,%eax; ret`) -- outside this range, and
//  rezmgr.h's own comment on them is "currently not used or implemented!".
// ======================================================================= //

void* CRezMgrX720::Alloc(DWORD_X720 /*NumBytes*/)
{
	return NULL_X720;
}

void CRezMgrX720::Free(void* /*Ptr*/)
{
}

BOOL_X720 CRezMgrX720::DiskError()
{
	return FALSE_X720;
}
