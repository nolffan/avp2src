// *********************************************************************** //
//
//  MODULE:   REZMGR (.CPP) -- Monolith's RezMgr resource-archive library,
//            the slice 0x0048df30..0x0048e62a of lithtech.exe.
//
//  WHAT THIS IS, AND HOW IT WAS IDENTIFIED
//  ---------------------------------------
//  The 50-function run 0x0048d210..0x00490abd is ONE MODULE and that module
//  is **RezMgr** -- `rezmgr.cpp` out of Monolith's `lithshared\rezmgr`,
//  compiled into the static library `rezmgr.lib` (the one rezmgr.h names in
//  its own `#pragma comment (lib, "\\proj\\libs\\release\\rezmgr.lib")`) and
//  linked into lithtech.exe and server.dll.  Four independent lines of
//  evidence, in the order they landed:
//
//   1. THE SAME BYTES ARE IN BOTH BINARIES, CONTIGUOUSLY.  Masking call
//      rel32s and in-image absolutes, every enumerated start from
//      0x0048df10 to 0x00491060 has exactly one counterpart in
//      the retail server.dll, and the counterparts are in the same order and
//      the same relative positions: 0x0048df10 -> 0x1003a530,
//      0x0048df30 -> 0x1003a550, 0x0048e040 -> 0x1003a660,
//      0x0048e550 -> 0x1003ab70, 0x0048ed50 -> 0x1003b370,
//      0x00490210 -> 0x1003c830, 0x00491060 -> 0x1003d600.  lithtech.exe is
//      built /O2 and server.dll /O1, so identical bytes in both cannot come
//      from a shared .cpp -- it is one PRE-BUILT .LIB linked into each.
//      (NOLF.exe does not contain it; the launcher has its own RezFind.cpp.)
//
//   2. filemgr.cpp ALREADY NAMES IT.  src/engine/server/filemgr.cpp calls
//      the layer it sits on "the archive layer" and cites its class's
//      constructor/destructor at server.dll 0x1003b370 / 0x1003b470 -- which
//      are exactly the images of lithtech 0x0048ed50 / 0x0048ee40 under the
//      map above.  So the run is the thing filemgr.cpp opens .rez files
//      with, and filemgr.cpp is the fs_* layer ON TOP of it, not it.
//
//   3. THE HEADER IS IN THE REPO AND THE LAYOUTS MATCH TO THE BYTE.
//      src/LT2/lithshared/incs/rezmgr.h declares CRezItm, CRezTyp, CRezDir,
//      CRezMgr.  REZMGRFULL is not defined, so REZMGRLITE is, and every
//      offset the disassembly shows falls out of that header exactly:
//        * CRezItm::InitRezItm is declared with ELEVEN parameters and
//          0x0048df30 is `ret 0x2c`; the four that REZMGRLITE compiles out
//          (nID, sDesc, nNumKeys, pKeyAry) are the four the shipped code
//          reads and never stores.
//        * CRezItm: m_sName +0x00, m_pType +0x04, m_nTime +0x08,
//          m_nSize +0x0c, m_pParentDir +0x10, m_nFilePos +0x14,
//          m_nCurPos +0x18, m_heName +0x1c (0x18 bytes), m_pRezFile +0x34,
//          m_pData +0x38 -- total 0x3c, and src/engine/lithtech/ltnew1.cpp
//          independently pinned the stride of this class at 0x3c from the
//          `new T[n]` loop at 0x0048ff6b.
//        * CRezDir: m_sDirName +0x00, m_nDirPos +0x04, m_nDirSize +0x08,
//          m_nItemsPos +0x0c, m_nItemsSize +0x10, m_nLastTimeModified +0x14,
//          m_pRezMgr +0x18, m_pParentDir +0x1c, m_heDir +0x20,
//          m_haDir +0x38, m_haTypes +0x40, m_pMemBlock +0x48.  CRezItm::Read
//          reaches m_pParentDir->+0x48 and ->+0x0c, i.e. m_pMemBlock and
//          m_nItemsPos, which is precisely how a directory loaded as one
//          block is read out of.
//        * CRezDir's constructor is declared with EIGHT parameters and
//          0x0048e300 is `ret 0x20`; CRezTyp has two constructors, one with
//          four parameters and one with three, and 0x0048e120 is `ret 0x10`
//          while 0x0048e1b0 is `ret 0x0c`.
//        * `DWORD Read(BYTE*, DWORD length, DWORD seekPos = 0xffffffff)` --
//          0x0048e040 opens with `cmp eax,0xffffffff` on its third argument
//          and calls Seek when it differs.
//
//   4. THE CALLEES ARE THE `lith` LIBRARY, WHICH IS ALREADY RECONSTRUCTED
//      IN THIS REPO.  0x004a0420/0x004a0430/0x004a04c0/0x004a0530/
//      0x004a0560/0x004a03d0/0x004a0590 are CBaseHash::CBaseHash(),
//      CBaseHash::CBaseHash(unsigned), CBaseHash::~CBaseHash,
//      CBaseHash::Delete, CBaseHash::GetFirst, CBaseHashItem::Next and
//      CBaseHash::GetFirstInBin -- compare src/lithshared/lith/basehash.cpp
//      and .h line for line (the `new CHashBin[NumBins]` in the one-argument
//      constructor is why it carries an SEH frame of its own).  rezmgr.h
//      includes "..\rezmgr\rezhash.h", whose hash elements derive from
//      CBaseHashItem; that is the vptr CRezItm/CRezTyp/CRezDir each store
//      into the MIDDLE of themselves (+0x1c, +0x04, +0x20).
//
//  The last of the run's own evidence that it is an archive reader, rather
//  than anything else: the only three string literals in the whole
//  0x0048d210..0x00490abd run are "r+b", "w+b" and "*.*" -- CRezMgr::Open's
//  two fopen modes and the wildcard its directory-emulation scan uses.
//
//  NOT ALREADY RECONSTRUCTED.  There is no rezmgr source anywhere in
//  src/engine/ or src/lithshared/ (the latter has ButeMgr, CryptMgr,
//  MFCStub, RegMgr, lith, lithfontmgr, ltguimgr, stdlith, zlib, ztools --
//  no rezmgr).  Of this run only the small shared pieces are written:
//  ltnew1.cpp has 0x0048de30/0x0048de60/0x0048de90/0x0048dee0 (CBaseList
//  Insert/Delete), 0x0048df10 (CRezItm::CRezItm) and 0x0048e680..0x0048e700;
//  lt2new2/3/4 and ltnew2 have scattered pieces above 0x0048ec80.  Nothing
//  in 0x0048df30..0x0048e62a is written anywhere.
//
//  BUILD FLAGS -- THIS TU NEEDS /GX.
//  rezmgr.lib was built WITH C++ exceptions.  0x0048e120, 0x0048e1b0,
//  0x0048e220, 0x0048e300 and 0x0048e3e0 all open with
//      push -1 ; push <__ehhandler> ; mov eax,fs:0 ; push eax ; mov fs:0,esp
//  which is VC6's three-dword __CxxFrameHandler registration, and they carry
//  EH state stores (`movl $0,0x10(%esp)`, `movb $1,...`, `movl $-1,...`) and
//  unwind funclets (0x004a07a0, 0x004a07c0, 0x004a07e0/0x004a07eb,
//  0x004a0800 ... each `mov ecx,[ebp-0x10]; add ecx,<member offset>;
//  jmp 0x0048e1a0`, 0x0048e1a0 being a bare `jmp CBaseHash::~CBaseHash`).
//  build/engine.sh's `lithtech)` recipe has no /GX, so compiled under it
//  these five lose their frames entirely.  Compile this file the way
//  build/engine.sh already compiles the OTHER lithshared libraries:
//      /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS
//      /D NO_PRAGMA_LIBS
//  The four EH-free functions (0x0048df30, 0x0048dfc0, 0x0048e040,
//  0x0048e550) are unaffected either way.
//
//  ENUMERATION DEFECT FOUND.  The row `0x0048e120 133` is two things: the
//  four-argument CRezTyp constructor really runs 0x0048e120..0x0048e18f
//  (112 bytes, its one `ret 0x10` at 0x0048e18e), and 0x0048e1a0 is a
//  separate five-byte compiler artefact, `jmp 0x004a04c0`, that the unwind
//  funclets above jump to.  An extent check flags it ("a SEPARATE function" at
//  0x0048e1a0, branch target 0x004a04c0 outside the range).  The other eight
//  rows check out exactly as stated.
//
//  NAMING.  Every type and global below carries the suffix `Rz48` so this
//  file cannot collide with the two sibling chunks of this run or with
//  anything already in src/engine/lithtech.  The real names (CRezItm,
//  CRezDir, CRezTyp, CRezMgr, CBaseHash, CBaseHashItem, CBaseListItem and
//  the member names) are rezmgr.h's and basehash.h's own; only the suffix is
//  added.
//
// *********************************************************************** //

#include <stdlib.h>
#include <string.h>


// ======================================================================= //
//  reztypes.h's typedefs (the header itself is in ..\rezmgr\, not in this
//  repo; these are the ones rezmgr.h uses and their widths are forced by
//  the code -- every one of them moves as a dword).
// ======================================================================= //

typedef int             BOOLRz48;
typedef unsigned char   BYTERz48;
typedef unsigned long   DWORDRz48;

typedef char*           REZNAMERz48;
typedef char*           REZDIRNAMERz48;
typedef char*           REZDESCRz48;
typedef unsigned long   REZTYPERz48;
typedef unsigned long   REZSIZERz48;
typedef unsigned long   REZTIMERz48;
typedef unsigned long   REZIDRz48;
typedef unsigned long   REZKEYVALRz48;

#ifndef NULL
#define NULL 0
#endif
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

class CRezItmRz48;
class CRezTypRz48;
class CRezDirRz48;
class CRezMgrRz48;


// ======================================================================= //
//  lith -- baselist.h / basehash.h.  These are src/lithshared/lith's own
//  classes, reproduced here verbatim in shape so the offsets and the
//  out-of-line calls come out right.  The out-of-line members are DECLARED
//  and not defined on purpose: they live in lith.lib at 0x004a03d0 ..
//  0x004a05e0 and every call to them is a masked rel32.
// ======================================================================= //

class CBaseListItemRz48
{
public:
	CBaseListItemRz48*  Next()      { return m_pNext; }
	CBaseListItemRz48*  Prev()      { return m_pPrev; }

protected:
	CBaseListItemRz48*  m_pNext;                     // +0x00 (+0x04 under a vptr)
	CBaseListItemRz48*  m_pPrev;                     // +0x04 (+0x08 under a vptr)
};


class CBaseHashItemRz48;

// 8 bytes: m_nNumBins then m_pBinAry.  CRezTyp's two tables sit at +0x1c and
// +0x24 and CRezDir's at +0x38 and +0x40, eight bytes apart in both.
class CBaseHashRz48
{
public:
	CBaseHashRz48();                                 // 0x004a0420
	CBaseHashRz48(unsigned int NumBins);             // 0x004a0430 (SEH: new[])
	~CBaseHashRz48();                                // 0x004a04c0

	void                Insert(CBaseHashItemRz48* pItem);
	void                Delete(CBaseHashItemRz48* pItem);   // 0x004a0530
	CBaseHashItemRz48*  GetFirst();                         // 0x004a0560
	CBaseHashItemRz48*  GetLast();

private:
	friend class CBaseHashItemRz48;

	unsigned int        m_nNumBins;
	void*               m_pBinAry;
};


// vptr at +0x00 (HashFunc is the only virtual and CBaseListItem carries
// none, so MSVC puts the pointer first and the base after it), m_pNext
// +0x04, m_pPrev +0x08, m_pParentHash +0x0c, m_nCurBin +0x10.  Size 0x14 --
// which is why every one of the three hash elements below is 0x18 bytes and
// why its owner pointer lands at +0x14 of the element.
class CBaseHashItemRz48 : public CBaseListItemRz48
{
public:
	CBaseHashItemRz48*  Next();                      // 0x004a03d0
	CBaseHashItemRz48*  Prev();

protected:
	virtual unsigned int HashFunc() = 0;

	CBaseHashRz48*      GetParentHash() { return m_pParentHash; }

private:
	friend class CBaseHashRz48;

	CBaseHashRz48*      m_pParentHash;
	unsigned int        m_nCurBin;
};


// ======================================================================= //
//  rezhash.h -- the three hash elements and the two typed hash tables.
//  Shapes forced by the code:
//    * the element is 0x18 bytes with its owner back-pointer at +0x14
//      (CRezItm +0x30, CRezTyp +0x18, CRezDir +0x34 -- all written twice in
//      the constructors, NULL from the element's own constructor and then
//      `this` from the owner's body).
//    * CRezTypeHashTable::Find(REZTYPE) is 0x00490130, `ret 4`.
//    * CRezItmHashTableByName::Find(REZNAME, BOOL) is 0x00490050, `ret 8`.
//  Both tables are plain CBaseHash derivatives: eight bytes, no vptr of
//  their own (the constructors 0x004a0420 / 0x004a0430 are called on them
//  directly and store nothing but the two base words).
// ======================================================================= //

class CRezItmHashByNameRz48 : public CBaseHashItemRz48
{
public:
	CRezItmHashByNameRz48() { m_pRezItm = NULL; }

	CRezItmRz48*    GetRezItm()                  { return m_pRezItm; }
	void            SetRezItm(CRezItmRz48* pItm) { m_pRezItm = pItm; }

protected:
	virtual unsigned int HashFunc();

private:
	CRezItmRz48*    m_pRezItm;                       // element +0x14
};


class CRezTypeHashRz48 : public CBaseHashItemRz48
{
public:
	CRezTypeHashRz48() { m_pRezTyp = NULL; }

	CRezTypRz48*    GetRezTyp()                  { return m_pRezTyp; }
	void            SetRezTyp(CRezTypRz48* pTyp) { m_pRezTyp = pTyp; }

protected:
	virtual unsigned int HashFunc();

private:
	CRezTypRz48*    m_pRezTyp;                       // element +0x14
};


class CRezDirHashRz48 : public CBaseHashItemRz48
{
public:
	CRezDirHashRz48() { m_pRezDir = NULL; }

	CRezDirRz48*    GetRezDir()                  { return m_pRezDir; }
	void            SetRezDir(CRezDirRz48* pDir) { m_pRezDir = pDir; }

protected:
	virtual unsigned int HashFunc();

private:
	CRezDirRz48*    m_pRezDir;                       // element +0x14
};


class CRezItmHashTableByIDRz48 : public CBaseHashRz48
{
public:
	CRezItmHashTableByIDRz48() : CBaseHashRz48() {}
	CRezItmHashTableByIDRz48(unsigned int nBins) : CBaseHashRz48(nBins) {}
};


class CRezItmHashTableByNameRz48 : public CBaseHashRz48
{
public:
	CRezItmHashTableByNameRz48() : CBaseHashRz48() {}
	CRezItmHashTableByNameRz48(unsigned int nBins) : CBaseHashRz48(nBins) {}

	CRezItmRz48* Find(REZNAMERz48 sName, BOOLRz48 bCaseSensitive);   // 0x00490050
};


class CRezTypeHashTableRz48 : public CBaseHashRz48
{
public:
	CRezTypeHashTableRz48() : CBaseHashRz48() {}
	CRezTypeHashTableRz48(unsigned int nBins) : CBaseHashRz48(nBins) {}

	CRezTypRz48* Find(REZTYPERz48 nType);                            // 0x00490130
};


class CRezDirHashTableRz48 : public CBaseHashRz48
{
public:
	CRezDirHashTableRz48() : CBaseHashRz48() {}
	CRezDirHashTableRz48(unsigned int nBins) : CBaseHashRz48(nBins) {}
};


// ======================================================================= //
//  rezfile.h -- CBaseRezFile, reached only through slot 2 of its vtable.
//
//  0x0048e0f2 is `call *0x8(%edx)` with four arguments pushed and NO caller
//  clean-up, so slot 2 is a __thiscall member taking four dwords, and the
//  order the pushes give is (m_nFilePos, m_nCurPos, length, pBytes).  The
//  two slots before it are not reached from this slice; they are declared
//  only so Read lands at slot 2.
// ======================================================================= //

class CBaseRezFileRz48
{
public:
	virtual BOOLRz48   Open(const char* sFileName, BOOLRz48 bReadOnly,
	                        BOOLRz48 bCreateNew) = 0;                 // slot 0
	virtual BOOLRz48   Close() = 0;                                   // slot 1
	virtual DWORDRz48  Read(DWORDRz48 nFilePos, DWORDRz48 nPos,
	                        DWORDRz48 nSize, void* pData) = 0;        // slot 2

	const char* GetFileName();
};


// ======================================================================= //
//  CRezMgr -- only the three things this slice reaches.
//
//  0x0048e510 reads m_pRezMgr->+0x68 and turns `== 0` into the second
//  argument of the by-name lookup, i.e. the lookup is case-SENSITIVE when
//  lower case is NOT used: that is rezmgr.h's m_bLowerCaseUsed.  0x0048e220
//  gates its whole by-ID sweep on m_pRezMgr->+0x6c, the flag right after it:
//  m_bItemByIDUsed.  The 0x68 bytes in front of them are not decoded here
//  and are left as opaque padding rather than guessed at from the header's
//  declaration order.
// ======================================================================= //

class CRezMgrRz48
{
public:
	void         DeAllocateRezItm(CRezItmRz48* pItem);   // 0x00490010
	REZTYPERz48  StrToType(char* pStr);                  // 0x0048f9c0

public:
	// UNDECODED: +0x00..+0x67.  rezmgr.h's declaration order does not land
	// m_bLowerCaseUsed on 0x68 by itself, so no field names are invented
	// for this span -- only the two flags the code actually reads are named.
	unsigned char m_pad_00[0x68];
	BOOLRz48      m_bLowerCaseUsed;                      // +0x68
	BOOLRz48      m_bItemByIDUsed;                       // +0x6c
};


// ======================================================================= //
//  CRezItm -- 0x3c bytes.
//
//  Every offset below is read off the code, not off the header alone:
//    +0x00 m_sName      0x0048df30 strdup target, 0x0048dfc0 frees it
//    +0x04 m_pType      0x0048e020 `mov eax,[ecx+4]; mov eax,[eax]`
//    +0x08 m_nTime      0x0048df30 stores the eighth argument there
//    +0x0c m_nSize      0x0048e040 clamps the read against it
//    +0x10 m_pParentDir 0x0048e040 reaches its +0x48 and +0x0c
//    +0x14 m_nFilePos   0x0048e040 hands it to CBaseRezFile::Read
//    +0x18 m_nCurPos    0x0048e030 writes it, 0x0048e040 advances it
//    +0x1c m_heName     vptr store at 0x0048df14 (the default constructor)
//    +0x34 m_pRezFile   0x0048df30 stores the eleventh argument there
//    +0x38 m_pData      0x0048e040's in-memory fast path
// ======================================================================= //

class CRezItmRz48
{
public:
	REZNAMERz48  GetName() { return m_sName; }
	REZTYPERz48  GetType();                                          // 0x0048e020
	REZSIZERz48  GetSize() { return m_nSize; }
	CRezDirRz48* GetParentDir() { return m_pParentDir; }
	REZTIMERz48  GetTime() { return m_nTime; }

	DWORDRz48    GetSeekPos() { return m_nCurPos; }
	BOOLRz48     Seek(DWORDRz48 offset);                             // 0x0048e030
	DWORDRz48    Read(BYTERz48* pBytes, DWORDRz48 length,
	                  DWORDRz48 seekPos = 0xffffffff);               // 0x0048e040

	void         SetTime(REZTIMERz48 Time) { m_nTime = Time; }

	// public here only so this file's own CRezTyp destructor can reach them
	// the way the shipped code does; in rezmgr.h they are private with
	// CRezTyp, CRezDir and CRezMgr as friends.
	CRezItmRz48();                                                   // 0x0048df10
	void InitRezItm(CRezDirRz48* pParentDir, REZNAMERz48 sName,
	                REZIDRz48 nID, CRezTypRz48* pType, REZDESCRz48 sDesc,
	                REZSIZERz48 nSize, DWORDRz48 nFilePos, REZTIMERz48 nTime,
	                DWORDRz48 nNumKeys, REZKEYVALRz48* pKeyAry,
	                CBaseRezFileRz48* pRezFile);                     // 0x0048df30
	void TermRezItm();                                               // 0x0048dfc0

public:
	REZNAMERz48             m_sName;                 // +0x00
	CRezTypRz48*            m_pType;                 // +0x04
	REZTIMERz48             m_nTime;                 // +0x08
	REZSIZERz48             m_nSize;                 // +0x0c
	CRezDirRz48*            m_pParentDir;            // +0x10
	DWORDRz48               m_nFilePos;              // +0x14
	DWORDRz48               m_nCurPos;               // +0x18
	CRezItmHashByNameRz48   m_heName;                // +0x1c .. +0x33
	CBaseRezFileRz48*       m_pRezFile;              // +0x34
	BYTERz48*               m_pData;                 // +0x38
};


// ======================================================================= //
//  CRezTyp -- 0x30 bytes.
//
//    +0x00 m_nType       stored from argument one in both constructors and
//                        zeroed by the destructor; CRezItm::GetType loads it
//                        through m_pType with no offset.
//    +0x04 m_heType      vptr store `movl $0x004a3b74,0x4(%esi)`
//    +0x1c m_haID        `lea 0x1c(%esi),%ecx` -> CBaseHash ctor
//    +0x24 m_haName      `lea 0x24(%esi),%ecx` -> CBaseHash ctor, and
//                        0x0048e53b's `lea 0x24(%eax),%ecx` for the lookup
//    +0x2c m_pParentDir  stored from argument two, read by the destructor
// ======================================================================= //

class CRezTypRz48
{
public:
	REZTYPERz48 GetType() { return m_nType; }

	CRezTypRz48(REZTYPERz48 nType, CRezDirRz48* pParentDir,
	            unsigned int nByIDNumHashBins,
	            unsigned int nByNameNumHashBins);                    // 0x0048e120
	CRezTypRz48(REZTYPERz48 nType, CRezDirRz48* pParentDir,
	            unsigned int nByNameNumHashBins);                    // 0x0048e1b0
	~CRezTypRz48();                                                  // 0x0048e220

public:
	REZTYPERz48                 m_nType;             // +0x00
	CRezTypeHashRz48            m_heType;            // +0x04 .. +0x1b
	CRezItmHashTableByIDRz48    m_haID;              // +0x1c .. +0x23
	CRezItmHashTableByNameRz48  m_haName;            // +0x24 .. +0x2b
	CRezDirRz48*                m_pParentDir;        // +0x2c
};


// ======================================================================= //
//  CRezDir -- 0x4c bytes.
//
//    +0x00 m_sDirName           strdup'd from argument three
//    +0x04 m_nDirPos            argument four
//    +0x08 m_nDirSize           argument five
//    +0x0c m_nItemsPos          zeroed by the constructor; CRezItm::Read
//                               subtracts it from m_nFilePos
//    +0x10 m_nItemsSize         zeroed by the constructor
//    +0x14 m_nLastTimeModified  argument six
//    +0x18 m_pRezMgr            argument one
//    +0x1c m_pParentDir         argument two
//    +0x20 m_heDir              vptr store `movl $0x004a3b78,0x20(%ebx)`
//    +0x38 m_haDir              `lea 0x38(%ebx),%ecx` -> CBaseHash(argument 7)
//    +0x40 m_haTypes            `lea 0x40(%ebx),%ecx` -> CBaseHash(argument 8)
//    +0x48 m_pMemBlock          zeroed by the constructor; CRezItm::Read's
//                               first fast path and TermRezItm's guard
// ======================================================================= //

class CRezDirRz48
{
public:
	REZDIRNAMERz48 GetDirName() { return m_sDirName; }
	CRezDirRz48*   GetParentDir() { return m_pParentDir; }
	CRezMgrRz48*   GetParentMgr() { return m_pRezMgr; }

	CRezItmRz48*   GetRez(REZNAMERz48 sRes, REZTYPERz48 Type);       // 0x0048e510
	CRezItmRz48*   GetRezFromDosName(char* sDosname);                // 0x0048e550

	BOOLRz48       IsLoaded() { return (m_pMemBlock != NULL); }
	REZTIMERz48    GetTime() { return m_nLastTimeModified; }

	CRezDirRz48(CRezMgrRz48* pRezMgr, CRezDirRz48* pParentDir,
	            REZDIRNAMERz48 szDirName, DWORDRz48 nFilePos, DWORDRz48 nSize,
	            REZTIMERz48 nTime, unsigned int nDirNumHashBins,
	            unsigned int nTypNumHashBins);                       // 0x0048e300
	~CRezDirRz48();                                                  // 0x0048e3e0

public:
	REZDIRNAMERz48          m_sDirName;              // +0x00
	DWORDRz48               m_nDirPos;               // +0x04
	DWORDRz48               m_nDirSize;              // +0x08
	DWORDRz48               m_nItemsPos;             // +0x0c
	DWORDRz48               m_nItemsSize;            // +0x10
	REZTIMERz48             m_nLastTimeModified;     // +0x14
	CRezMgrRz48*            m_pRezMgr;               // +0x18
	CRezDirRz48*            m_pParentDir;            // +0x1c
	CRezDirHashRz48         m_heDir;                 // +0x20 .. +0x37
	CRezDirHashTableRz48    m_haDir;                 // +0x38 .. +0x3f
	CRezTypeHashTableRz48   m_haTypes;               // +0x40 .. +0x47
	BYTERz48*               m_pMemBlock;             // +0x48
};


// ======================================================================= //
//  0x0048df30, 141 bytes -- void CRezItm::InitRezItm(11 args), `ret 0x2c`
//
//  ELEVEN dwords of argument, and only SEVEN of them are stored.  That is
//  not a puzzle, it is the proof: rezmgr.h declares InitRezItm with eleven
//  parameters unconditionally but wraps m_nID, m_sDescription, m_nNumKeys
//  and m_pKeyAry in `#ifndef REZMGRLITE`, and the four arguments the shipped
//  code never touches are exactly nID (+0x0c), sDesc (+0x14), nNumKeys
//  (+0x24) and pKeyAry (+0x28).  So this build is REZMGRLITE, the parameter
//  list is the full one, and the assignments are the LITE subset.
//
//  THE NAME COPY.  `repnz scasb` twice with the allocation between them is
//  the explicit `strlen(sName) + 1` for the `new`, and then STRCPY's OWN
//  inline expansion, which scans the source again.  The tell that it is
//  strcpy and not memcpy is 0x0048df76 `sub %ecx,%edi` followed by
//  `mov %edi,%esi`: the intrinsic recovers the source pointer by subtracting
//  the length it just measured from the scan cursor, where memcpy would
//  simply reuse the pointer it was handed.  The same three instructions are
//  at 0x0048e37a in CRezDir::CRezDir and 0x0048e5d7 in GetRezFromDosName --
//  and GetRezFromDosName's copy is an unambiguous strcpy.
//
//  THE NULL ARM.  0x0048df4b stores %esi, which the `test %esi,%esi` two
//  instructions earlier has just proved zero; that is the vanishing
//  null test, and `m_sName = NULL` is its source.  The fall-through being
//  the null arm is what `if (sName == NULL) ... else ...` gives.
//
//  STORE ORDER.  m_pRezFile and m_pParentDir are written BEFORE the call to
//  operator new and m_pType/m_nSize/m_nFilePos/m_nTime after it, so their
//  side of the call is fixed; within each side the scheduler is free and the
//  order below is the emitted one.
// ======================================================================= //

void CRezItmRz48::InitRezItm(CRezDirRz48* pParentDir, REZNAMERz48 sName,
                             REZIDRz48 nID, CRezTypRz48* pType, REZDESCRz48 sDesc,
                             REZSIZERz48 nSize, DWORDRz48 nFilePos, REZTIMERz48 nTime,
                             DWORDRz48 nNumKeys, REZKEYVALRz48* pKeyAry,
                             CBaseRezFileRz48* pRezFile)
{
	m_pRezFile = pRezFile;
	m_pParentDir = pParentDir;

	if (sName == NULL)
	{
		m_sName = NULL;
	}
	else
	{
		m_sName = new char[strlen(sName) + 1];
		if (m_sName != NULL) strcpy(m_sName, sName);
	}

	m_pType = pType;
	m_nSize = nSize;
	m_nFilePos = nFilePos;
	m_nTime = nTime;

	m_pData = NULL;
	m_nCurPos = 0;
	m_heName.SetRezItm(this);
}


// ======================================================================= //
//  0x0048dfc0, 87 bytes -- void CRezItm::TermRezItm(), `ret` (no arguments)
//
//  THE OWNERSHIP RULE THIS ENCODES.  m_pData is only ours to free when the
//  directory did NOT load itself as one block: if m_pParentDir->m_pMemBlock
//  is non-null the item's bytes are a slice of that block and freeing them
//  would be a free of an interior pointer.  0x0048dfdc tests exactly that.
//
//  THE TEST IS WRITTEN TWICE, NOT ONCE.  0x0048dfe1 and 0x0048dfea are two
//  copies of `if (m_pData != NULL)`, and only the free itself is shared --
//  0x0048dfe8 jumps forward INTO the second copy's `push`/`call`.  A single
//  `||` gives one copy of the body and no such jump; the rule that
//  of two identical sites VC6 keeps the SECOND inline and jumps the first
//  into it says these are two separate source statements, so the guard is
//  an if/else on m_pParentDir with the same free written in both arms.
//
//  The nine trailing zero stores all come out of one `xor %edi,%edi`, which
//  is also the register every comparison above uses; that is what writing
//  NULL/0 as constants gives at /O2.
// ======================================================================= //

void CRezItmRz48::TermRezItm()
{
	if (m_sName != NULL) delete[] m_sName;

	if (m_pParentDir != NULL)
	{
		if (m_pParentDir->m_pMemBlock == NULL)
		{
			if (m_pData != NULL) delete[] m_pData;
		}
	}
	else
	{
		if (m_pData != NULL) delete[] m_pData;
	}

	m_sName = NULL;
	m_pType = NULL;
	m_nTime = 0;
	m_nSize = 0;
	m_pData = NULL;
	m_pParentDir = NULL;
	m_nFilePos = 0;
	m_nCurPos = 0;
	m_heName.SetRezItm(NULL);
}


// ======================================================================= //
//  0x0048e020, 6 bytes -- REZTYPE CRezItm::GetType(), `ret` (thiscall)
//
//  `mov 0x4(%ecx),%eax ; mov (%eax),%eax`: m_pType, then CRezTyp::m_nType at
//  its +0x00, with the header's inline CRezTyp::GetType() folded in.  No
//  null check, which is why it cannot be the header's inline GetName form.
//  (In the gap between two of the enumerated rows, but inside this
//  file's range, and Read below needs it to be here.)
// ======================================================================= //

REZTYPERz48 CRezItmRz48::GetType()
{
	return m_pType->GetType();
}


// ======================================================================= //
//  0x0048e030, 15 bytes -- BOOL CRezItm::Seek(DWORD offset), `ret 4`
// ======================================================================= //

BOOLRz48 CRezItmRz48::Seek(DWORDRz48 offset)
{
	m_nCurPos = offset;
	return TRUE;
}


// ======================================================================= //
//  0x0048e040, 211 bytes -- DWORD CRezItm::Read(BYTE*, DWORD, DWORD),
//  `ret 0xc`.  Four `ret`s, and the extent check's warning is a false alarm: this
//  is one function with the /O2 epilogue duplicated into each of its three
//  successful exits plus the shared `xor %eax,%eax` failure exit at
//  0x0048e10a, every one of them inside the range and reached only from
//  inside it.
//
//  AND THAT SPLIT IS THE SHAPE OF THE SOURCE.  Several copies of
//  an epilogue mean several separate `return` statements, one shared copy
//  means the conditions nest around a single one.  Here the THREE
//  `m_nCurPos += length; return length;` tails are each written out and the
//  ONE `return 0` is the last statement of the function, with every failure
//  branching to it -- so the three guards are nested positively rather than
//  written as early `if (...) return 0;` returns.  Written the other way
//  round the compiler inverts the split exactly: one shared success tail and
//  three copies of the failure epilogue.
//
//  THE DEFAULT ARGUMENT IS VISIBLE.  rezmgr.h declares
//  `DWORD Read(BYTE* pBytes, DWORD length, DWORD seekPos = 0xffffffff)` and
//  0x0048e047 is `cmp $0xffffffff,%eax` on the third argument, calling Seek
//  only when the caller supplied one.
//
//  THREE WAYS TO GET THE BYTES, in the order the code tries them:
//    1. the whole DIRECTORY is in memory (m_pParentDir->m_pMemBlock): the
//       item's bytes are at m_nFilePos - m_pParentDir->m_nItemsPos into it,
//       m_nFilePos being file-relative and m_nItemsPos the block's own base.
//    2. this ITEM is in memory (m_pData): straight offset by m_nCurPos.
//    3. neither: go to the rez file, and only advance m_nCurPos if it
//       returned everything that was asked for.
//
//  `test %ebp,%ebp ; jbe` rather than `je` is what `length <= 0` on an
//  unsigned gives; `je` is what `length == 0` gives, so the source says
//  `<= 0`.
// ======================================================================= //

DWORDRz48 CRezItmRz48::Read(BYTERz48* pBytes, DWORDRz48 length, DWORDRz48 seekPos)
{
	if (seekPos != 0xffffffff) Seek(seekPos);

	if (m_nCurPos <= m_nSize)
	{
		if ((m_nCurPos + length) > m_nSize) length = m_nSize - m_nCurPos;

		if (length > 0)
		{
			if (m_pParentDir->m_pMemBlock != NULL)
			{
				memcpy(pBytes,
				       m_pParentDir->m_pMemBlock + (m_nFilePos - m_pParentDir->m_nItemsPos) + m_nCurPos,
				       length);
				m_nCurPos += length;
				return length;
			}

			if (m_pData != NULL)
			{
				memcpy(pBytes, m_pData + m_nCurPos, length);
				m_nCurPos += length;
				return length;
			}

			if (m_pRezFile->Read(m_nFilePos, m_nCurPos, length, pBytes) == length)
			{
				m_nCurPos += length;
				return length;
			}
		}
	}

	return 0;
}


// ======================================================================= //
//  0x0048e120, 112 bytes (NOT 133 -- see the extent note in the header) --
//  CRezTyp::CRezTyp(REZTYPE, CRezDir*, unsigned, unsigned), `ret 0x10`
//
//  Four arguments, which is rezmgr.h's first of the two constructors, the
//  one that gives the type BOTH hash tables.  The member-initialiser order
//  is forced: m_heType's implicit constructor (the vptr store at +0x04 and
//  its owner pointer at +0x18 set to NULL) runs first, then m_haID at +0x1c
//  with argument three, then m_haName at +0x24 with argument four; the body
//  then writes m_nType, m_heType's owner pointer again -- this time `this`
//  -- and m_pParentDir, in that order.
//
//  THE EH FRAME IS THE POINT.  0x0048e120's three-push registration and its
//  `movl $0,0x10(%esp)` state store exist because m_haName's constructor can
//  throw and m_haID would then have to be destroyed; the funclet that does
//  it is 0x004a07a0 -> 0x0048e1a0 -> CBaseHash::~CBaseHash.  Compile this
//  file WITHOUT /GX and none of that is emitted.
// ======================================================================= //

CRezTypRz48::CRezTypRz48(REZTYPERz48 nType, CRezDirRz48* pParentDir,
                         unsigned int nByIDNumHashBins,
                         unsigned int nByNameNumHashBins)
	: m_haID(nByIDNumHashBins), m_haName(nByNameNumHashBins)
{
	m_nType = nType;
	m_heType.SetRezTyp(this);
	m_pParentDir = pParentDir;
}


// ======================================================================= //
//  0x0048e1b0, 108 bytes -- CRezTyp::CRezTyp(REZTYPE, CRezDir*, unsigned),
//  `ret 0x0c`.  rezmgr.h's second constructor, commented there as "non ID
//  has table version": the only difference from the one above is that
//  m_haID gets CBaseHash's DEFAULT constructor (0x004a0420, which just
//  zeroes the two words) instead of the sized one, and that is exactly what
//  0x0048e1de calls.
// ======================================================================= //

CRezTypRz48::CRezTypRz48(REZTYPERz48 nType, CRezDirRz48* pParentDir,
                         unsigned int nByNameNumHashBins)
	: m_haName(nByNameNumHashBins)
{
	m_nType = nType;
	m_heType.SetRezTyp(this);
	m_pParentDir = pParentDir;
}


// ======================================================================= //
//  0x0048e220, 215 bytes -- CRezTyp::~CRezTyp(), `ret` (no arguments)
//
//  TWO SWEEPS, and the first is conditional.  0x0048e24b walks
//  m_pParentDir->m_pRezMgr and tests its +0x6c before touching m_haID at
//  all: rezmgr.h's m_bItemByIDUsed, whose whole documented job is that "if
//  false then [items] can not be [accessed by ID]" -- so when it is false
//  the by-ID table was never populated and there is nothing to unhook.
//
//  Both sweeps take the successor BEFORE unhooking the node, because
//  CBaseHashItem::Next() walks the bin array through m_pParentHash and
//  CBaseHash::Delete clears it -- and the way they are written is visible.
//  0x0048e267 copies the node into edi BEFORE the Next() call and
//  0x0048e271 drops Next()'s result straight into esi, the register the
//  loop test and the loop head both use; there is no copy at the bottom of
//  the loop at all.  That is `pCur = pItem; pItem = pItem->Next();` -- the
//  successor is assigned to the loop variable itself and the node being
//  unhooked is the saved copy.  The other spelling, `pNext = pItem->Next();
//  ... pItem = pNext;`, puts the copy at the BOTTOM instead and costs a
//  `mov` there; both walks here, and both in ~CRezDir, are the first form.
//
//  THE DOUBLE LOAD AT 0x0048e29f / 0x0048e2aa is not a spill: the item
//  pointer is loaded out of the hash element, used, and loaded AGAIN after
//  TermRezItm has run, because TermRezItm could have changed it.  This is
//  the repeated expression, and the source has to spell it twice.
//
//  The two tables are destroyed in reverse declaration order, m_haName then
//  m_haID, which is the compiler's doing and not the body's.
// ======================================================================= //

CRezTypRz48::~CRezTypRz48()
{
	CBaseHashItemRz48* pItem;
	CBaseHashItemRz48* pCur;

	if (m_pParentDir->m_pRezMgr->m_bItemByIDUsed)
	{
		pItem = m_haID.GetFirst();
		while (pItem != NULL)
		{
			pCur = pItem;
			pItem = pItem->Next();
			m_haID.Delete(pCur);
		}
	}

	pItem = m_haName.GetFirst();
	while (pItem != NULL)
	{
		pCur = pItem;
		pItem = pItem->Next();
		m_haName.Delete(pCur);
		((CRezItmHashByNameRz48*)pCur)->GetRezItm()->TermRezItm();
		m_pParentDir->m_pRezMgr->DeAllocateRezItm(
			((CRezItmHashByNameRz48*)pCur)->GetRezItm());
	}

	m_nType = 0;
	m_heType.SetRezTyp(NULL);
}


// ======================================================================= //
//  0x0048e300, 212 bytes -- CRezDir::CRezDir(8 args), `ret 0x20`
//
//  Eight arguments, matching rezmgr.h's one constructor exactly, and each
//  one lands where the header says: pRezMgr -> +0x18, pParentDir -> +0x1c,
//  szDirName -> the strdup at +0x00, nFilePos -> +0x04, nSize -> +0x08,
//  nTime -> +0x14, nDirNumHashBins -> the CBaseHash at +0x38,
//  nTypNumHashBins -> the CBaseHash at +0x40.  m_nItemsPos, m_nItemsSize and
//  m_pMemBlock start at zero: nothing has been read into this directory yet.
//
//  UNLIKE InitRezItm THERE IS NO NULL TEST ON THE NAME.  0x0048e34e goes
//  straight into `repnz scasb` on argument three; the only test is on what
//  operator new returned.  So the source here really is the two-line form
//  and not the three-arm one, and the copy is strcpy (0x0048e37a's
//  `sub %ecx,%edi` / `mov %edi,%esi` recovering the source pointer from the
//  scan cursor is the intrinsic's signature, not memcpy's).
//
//  THE FIRST FOUR ASSIGNMENTS ARE IN THE ARGUMENT PRELOADS, not in the store
//  order.  VC6 preloads the first three values into ecx/edx/eax and then
//  alternates store-then-reload: 0x0048e38e..0x0048e396 fetch nTime, nSize
//  and nFilePos, so those are source statements one, two and three, and
//  pRezMgr -- reloaded into ecx after nTime's store frees it -- is the
//  fourth even though its store at 0x0048e3a8 comes out BEFORE nFilePos's at
//  0x0048e3b0.  Read the preloads, not the stores.
// ======================================================================= //

CRezDirRz48::CRezDirRz48(CRezMgrRz48* pRezMgr, CRezDirRz48* pParentDir,
                         REZDIRNAMERz48 szDirName, DWORDRz48 nFilePos,
                         DWORDRz48 nSize, REZTIMERz48 nTime,
                         unsigned int nDirNumHashBins,
                         unsigned int nTypNumHashBins)
	: m_haDir(nDirNumHashBins), m_haTypes(nTypNumHashBins)
{
	m_sDirName = new char[strlen(szDirName) + 1];
	if (m_sDirName != NULL) strcpy(m_sDirName, szDirName);

	m_nLastTimeModified = nTime;
	m_nDirSize = nSize;
	m_nDirPos = nFilePos;
	m_pRezMgr = pRezMgr;
	m_nItemsSize = 0;
	m_nItemsPos = 0;
	m_pMemBlock = NULL;
	m_pParentDir = pParentDir;
	m_heDir.SetRezDir(this);
}


// ======================================================================= //
//  0x0048e3e0, 289 bytes -- CRezDir::~CRezDir(), `ret` (no arguments)
//
//  The whole tree comes down from here: every CRezTyp in m_haTypes is
//  unhooked and deleted (0x0048e43c calls ~CRezTyp at 0x0048e220 and then
//  operator delete directly, which is how VC6 spells `delete p` for a class
//  with a NON-virtual destructor -- rezmgr.h's ~CRezTyp is private and
//  non-virtual), and then every CRezDir in m_haDir the same way, 0x0048e480
//  being this function calling ITSELF.  The header's comment on ReadAllDirs,
//  "Recursivly read all directories in this dir", has its mirror here.
//
//  Then the two owned buffers: m_sDirName always, m_pMemBlock if this
//  directory was loaded as one block.  Then the ten zero stores, all off one
//  `xor %ebx,%ebx`, in the emitted order.
//
//  WHERE THE NULL TESTS COME FROM.  The two `delete p` of class pointers
//  need no `if` written: VC6 has to test before it can call a destructor, so
//  0x0048e438 and 0x0048e47c are the compiler's.  The two `delete []` of
//  char/BYTE arrays have no destructor to guard, so 0x0048e498 and
//  0x0048e4aa are the source's own tests and are written out.
// ======================================================================= //

CRezDirRz48::~CRezDirRz48()
{
	CBaseHashItemRz48* pItem;
	CBaseHashItemRz48* pCur;
	CRezTypRz48*       pTyp;
	CRezDirRz48*       pDir;

	pItem = m_haTypes.GetFirst();
	while (pItem != NULL)
	{
		pCur = pItem;
		pItem = pItem->Next();
		m_haTypes.Delete(pCur);
		pTyp = ((CRezTypeHashRz48*)pCur)->GetRezTyp();
		delete pTyp;
	}

	pItem = m_haDir.GetFirst();
	while (pItem != NULL)
	{
		pCur = pItem;
		pItem = pItem->Next();
		m_haDir.Delete(pCur);
		pDir = ((CRezDirHashRz48*)pCur)->GetRezDir();
		delete pDir;
	}

	if (m_sDirName != NULL) delete[] m_sDirName;
	if (m_pMemBlock != NULL) delete[] m_pMemBlock;

	m_sDirName = NULL;
	m_nLastTimeModified = 0;
	m_nDirSize = 0;
	m_nDirPos = 0;
	m_nItemsSize = 0;
	m_nItemsPos = 0;
	m_pMemBlock = NULL;
	m_pRezMgr = NULL;
	m_pParentDir = NULL;
	m_heDir.SetRezDir(NULL);
}


// ======================================================================= //
//  0x0048e510, 55 bytes -- CRezItm* CRezDir::GetRez(REZNAME, REZTYPE),
//  `ret 8`.  (Between two enumerated rows, inside this file's range, and
//  GetRezFromDosName below tail-calls it.)
//
//  The type table first, then that type's by-name table.  The second
//  argument of the by-name lookup is `m_pRezMgr->m_bLowerCaseUsed == 0`
//  (`test %ecx,%ecx ; sete %dl`), i.e. the compare is case-SENSITIVE exactly
//  when the manager has been told lower case is not used -- which is what
//  rezmgr.h's "DEFAULT IS FALSE" flag is for.
// ======================================================================= //

CRezItmRz48* CRezDirRz48::GetRez(REZNAMERz48 sRes, REZTYPERz48 Type)
{
	CRezTypRz48* pTyp;

	pTyp = m_haTypes.Find(Type);
	if (pTyp == NULL) return NULL;

	return pTyp->m_haName.Find(sRes, !m_pRezMgr->m_bLowerCaseUsed);
}


// ======================================================================= //
//  0x0048e550, 218 bytes -- CRezItm* CRezDir::GetRezFromDosName(char*),
//  `ret 4`.  rezmgr.h: "gets a resource from an old style dos file name
//  which includes the type in the extension".
//
//  _splitpath (0x00495313) pulls the name apart; the extension IS the type.
//    * longer than five characters (".abcd" is five) it cannot be a type, so
//      it is glued back onto the name and the type is left 0;
//    * absent entirely, the type is 0;
//    * otherwise the characters after the dot are copied out, upper-cased
//      (0x004a0320 is _strupr) and handed to CRezMgr::StrToType
//      (0x0048f9c0), which rezmgr.h declares as "Convert an ascii string to
//      a rez type".
//  Either way the answer comes from GetRez above.
//
//  The `strcpy` at 0x0048e5cc is VC6's expansion -- strlen then rep movs --
//  not a call; only _splitpath, strncat (0x004951f0), _strupr, StrToType and
//  GetRez are calls.
//
//  UNPINNED, AND SAID SO RATHER THAN DRESSED UP: the buffer SIZES.  The
//  frame is `sub $0x418,%esp` and the five live slots start at +0x00 (the
//  type string, <= 8 bytes), +0x08 (the drive, 4), +0x0c (the extension,
//  0x104), +0x110 (the name, 0x204) and +0x314 (the directory, 0x104), which
//  sums to 0x418 exactly -- so those are the slot sizes, but which _MAX_*
//  constants the source spelled them with is not recoverable, and neither is
//  the 0x201 strncat bound nor the 256 the name is truncated at.  They are
//  written as the literals the bytes give.
//  ARM ORDER.  The second test is `strlen(sExt) != 0` with the TYPE arm as
//  its body and `nType = 0` as the trailing `else`, not the other way
//  round.  Two things say so and one source change fixes both: 0x0048e60d's
//  `xor %eax,%eax` sits AFTER the type arm's body, which is where an `else`
//  clause lands (the body has to `jmp` over it at 0x0048e60b), and the type
//  arm's strcpy has NO `xor %eax,%eax` of its own -- reached by fall-through
//  from the strlen block it inherits the zero already in eax, which a block
//  reached only by a branch does not.
//
//  COIN FLIP, UNSETTLED: the declaration order below is the one that puts
//  each array where the frame has it if VC6 lays locals out first-declared-
//  highest.  If it lays them out the other way round, reverse these five
//  lines; nothing else changes.
// ======================================================================= //

CRezItmRz48* CRezDirRz48::GetRezFromDosName(char* sDosName)
{
	char sDir[260];
	char sName[516];
	char sExt[260];
	char sDrive[4];
	char sType[5];
	REZTYPERz48 nType;

	_splitpath(sDosName, sDrive, sDir, sName, sExt);

	if (strlen(sExt) > 5)
	{
		strncat(sName, sExt, 513);
		sName[256] = '\0';
		nType = 0;
	}
	else if (strlen(sExt) != 0)
	{
		strcpy(sType, &sExt[1]);
		_strupr(sType);
		nType = m_pRezMgr->StrToType(sType);
	}
	else
	{
		nType = 0;
	}

	return GetRez(sName, nType);
}


// ======================================================================= //
//  NOT WRITTEN HERE, and why
//
//  0x0048df10 (26 bytes, CRezItm::CRezItm) is below this file's range and
//  is already reconstructed as CSub48df10View in
//  src/engine/lithtech/ltnew1.cpp -- with the vptr at +0x1c, the owner
//  pointer at +0x30 written twice, and the 0x3c stride, all of which this
//  file's CRezItm confirms.  It is declared above and deliberately left
//  undefined so the two cannot collide.
//
//  0x0048e1a0 (5 bytes, `jmp CBaseHash::~CBaseHash`) is not source: it is
//  the shared tail of this translation unit's unwind funclets, emitted by
//  the compiler when /GX is on.
// ======================================================================= //
