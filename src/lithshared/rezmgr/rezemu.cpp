// *********************************************************************** //
//
//  MODULE:   REZFILE (.CPP) -- 0x00490740..0x00490b43 of lithtech.exe
//
//  This is the tail of rezfile.cpp, the disk-I/O layer of Monolith's
//  rezmgr.lib.  Two sibling chunks of the same library are already in the
//  repository (src/lithshared/rezmgr/rezmgr.cpp, suffix `Rz48`, and
//  src/lithshared/rezmgr/rezmgr2.cpp, suffix `X720`); everything here uses a
//  third suffix, `Rf80`, so all three can be compiled together.  Strip the
//  suffix to get the real name out of rezmgr.h / rezfile.h.
//
//  WHAT THIS CHUNK IS -- A FOURTH CBaseRezFile, WHICH NEITHER SIBLING HAS.
//
//  rezmgr2.cpp lists three vtables for rezfile.cpp: CBaseRezFile 0x004a3b90
//  (nine slots, seven purecall), CRezFile 0x004a3bb4 and
//  CRezFileDirectoryEmulation 0x004a3bd8.  There is a FOURTH, at 0x004a3c00,
//  and every function in this chunk belongs to it:
//
//      slot 0  0x00490200  VirtualFoo            (empty body, folded)
//      slot 1  0x004907e0  `scalar deleting destructor' -> 0x00490800
//      slot 2  0x00490870  Read
//      slot 3  0x00490910  Write
//      slot 4  0x004909b0  Open        { return FALSE; }
//      slot 5  0x0048fab0  Close       { return FALSE; }   (folded)
//      slot 6  0x004909c0  Flush
//      slot 7  0x0048fab0  VerifyFileOpen { return FALSE; } (folded)
//      slot 8  0x00490b40  GetFileName { return m_sFileName; } (folded)
//
//  It is the per-file object CRezFileDirectoryEmulation hands out when the
//  archive is emulated by a real directory tree: one instance per resource,
//  each wrapping one real FILE*, with the emulation object keeping an
//  LRU list of the ones currently open so the process never exceeds
//  CRezMgr::m_nMaxOpenFilesInEmulatedDir handles.
//
//  PROOF, not inference:
//
//   1. Its constructor is 0x00490740 and its only caller is 0x0048f893,
//      inside CRezMgr::ReadEmulationDirectory.  That call site does
//      `push $0x1c ; call 0x004290b0` at 0x0048f855 -- sizeof is 0x1c -- and
//      stores the result with `mov %eax,0x34(%esi)` at 0x0048f8a7, where esi
//      came out of CRezDir::CreateRezInternal (0x0048e7f0).  CRezItm +0x34 is
//      m_pRezFile in rezmgr2.cpp's layout.  So the object IS a CBaseRezFile
//      and it IS what an emulated CRezItm reads through.
//
//   2. 0x00490740 opens with `push <pRezMgr> ; call 0x004901b0`, which is
//      CBaseRezFile::CBaseRezFile (it writes vtable 0x004a3b90 to +0x00 and
//      the manager to +0x0c), and the destructor 0x00490800 closes with
//      `call 0x004901f0`, which is ~CBaseRezFile.  Single inheritance,
//      nine-slot vtable, base at offset zero.
//
//   3. CRezFileDirectoryEmulation::Close (0x00490710) walks the list at
//      +0x14 of the emulation object and calls 0x00490ac0 on each element
//      with a DIRECT call -- so 0x00490ac0 is a non-virtual member of the
//      element type, and the element type is this class.
//
//  THE EMULATION OBJECT'S LAYOUT, which rezmgr2.cpp carries only as
//  `BYTE m_pad_0c[0x38-0x0c]`, is fully pinned by its constructor
//  (0x004905e0) plus the four functions here that reach into it:
//
//      +0x00  vptr (0x004a3bd8)
//      +0x04  m_pNext          (CVirtBaseListItem)
//      +0x08  m_pPrev
//      +0x0c  m_pRezMgr        (CBaseRezFile)
//      +0x10  m_lstOpenFiles   vptr 0x004a3bfc @+0x10, first @+0x14, last @+0x18
//      +0x1c  m_lstClosedFiles vptr 0x004a3bfc @+0x1c, first @+0x20, last @+0x24
//      +0x28  m_nNumOpenFiles  int   (0x004905e0 zeroes it; 0x00490ab3 `incl`,
//                                     0x00490b10 `dec`)
//      +0x2c  m_nMaxOpenFiles  int   (the constructor's second argument;
//                                     compared SIGNED at 0x00490a27 `jle`)
//      +0x30  m_bReadOnly      BOOL  (constructor sets it to 1)
//      +0x34  m_bCreateNew     BOOL  (constructor sets it to 0)
//      0x38 total -- the size 0x0048f021 asks operator new for.
//
//  Both lists carry vtable 0x004a3bfc, whose single slot is 0x00490200, the
//  empty VirtualFoo -- so they are ONE list type, not two, and it derives
//  from CVirtBaseList (whose own one-slot purecall vtable 0x004a3b8c is what
//  ~CRezFileDirectoryEmulation parks in both at 0x004906ac/0x004906af).
//
//  TWO CORRECTIONS TO THE SIBLING FILES.  Neither is a defect in the code
//  they emitted; both are in the commentary:
//
//   a. rezmgr2.cpp declares slot 2 as
//        `Read(DWORD Pos, DWORD nUnknown, DWORD Size, void* pData)` with
//      "the second argument is 0 at the only site this file reaches, so what
//      it selects is not visible from here".  It is visible from here.
//      CRezFile::Read (0x004902c0) computes `lea (%ecx,%eax,1),%ebp` from
//      arg0 and arg1 at 0x004902df and seeks to the SUM, then stores
//      `arg0+arg1+nBytesRead` into its +0x20 cache at 0x00490353.  The second
//      argument is the offset WITHIN the resource; the first is the
//      resource's position in the archive.  The signature is
//        Read(DWORD nPos, DWORD nOffset, DWORD nSize, void* pData).
//      This class ignores nPos entirely (0x00490873 reads only +0x18 and
//      0x0049088d only +0x14), because one emulated file IS one resource.
//
//   b. rezmgr2.cpp's CRezFileDirectoryEmulation is described as 0x38 bytes
//      with m_pad_0c -- correct -- but the same comment block guesses slots
//      3/5/6/7/8 are "never reached ... their names are placeholders".
//      They are all pinned now, by CRezFile's bodies:
//        0x004904e0 Close  -- fclose (0x00492a93), frees m_sFileName, nulls
//                             +0x10 and +0x14, resets the +0x20 cache
//        0x00490540 Flush  -- fflush (0x00494135), resets +0x20 only
//        0x00490580 VerifyFileOpen -- ftell (0x00492cf1), and on -1 re-issues
//                             slot 4 as Open(m_sFileName, m_bReadOnly, FALSE)
//                             at 0x004905b6
//        0x004905d0 GetFileName -- `mov 0x14(%ecx),%eax ; ret`
//      so the order Read/Write/Open/Close/Flush/VerifyFileOpen/GetFileName
//      that rezmgr2.cpp guessed is exactly right.
//      0x004905b6 also settles the argument order of slot 4 for good:
//      CRezFile::Open stores its SECOND argument at +0x18 (0x00490472) and
//      VerifyFileOpen passes +0x18 back as the second argument, and the
//      second argument is the one that selects "rb".  So it really is
//        Open(const char* sFileName, BOOL bReadOnly, BOOL bCreateNew).
//
//  TRUE EXTENTS.  An extent check agrees with all five stated rows
//  (0x00490800/106, 0x00490870/159, 0x00490910/159, 0x004909c0/73,
//  0x00490a10/173): one ret each for the first, duplicated /O2 epilogues for
//  the rest, no branch target outside any range, no trailing padding.  What
//  the enumeration MISSED is that the 670 bytes it names are not the whole
//  chunk.  Four more functions of the same class live in the gaps and the
//  margins, and all four are here:
//
//      0x00490740  0x9b b  constructor          (before the stated start)
//      0x004907e0  0x1e b  scalar deleting dtor (the "backwards" window --
//                          there IS something at 0x004907e0..0x004907fb, and
//                          it is compiler-generated, not source)
//      0x004909b0  5 b     Open  { return FALSE; }  (the gap between the
//                          0x00490910 row and the 0x004909c0 row)
//      0x00490ac0  0x7c b  CloseFile            (after the stated end, and
//                          before the 0x00490b3f library boundary)
//
//  BOTH BINARIES SCORE THESE.  Every function below has a byte-identical
//  twin in the retail server.dll; I located each by searching for a window of
//  its body containing neither a relocation nor a rel32 displacement.  The
//  delta is a constant 0x0fb8c620 across all of them, which is itself the
//  check that the run is contiguous in both images:
//
//      0x00490740 -> 0x1003cd60      0x004909b0 -> 0x1003cfd0
//      0x004907e0 -> 0x1003ce00      0x004909c0 -> 0x1003cfe0
//      0x00490800 -> 0x1003ce20      0x00490a10 -> 0x1003d030
//      0x00490870 -> 0x1003ce90      0x00490ac0 -> 0x1003d0e0
//      0x00490910 -> 0x1003cf30
//
//  IT NEEDS /GX.  0x00490740 and 0x00490800 open with VC6's three-dword
//  __CxxFrameHandler registration (handlers 0x004a09c8 and 0x004a09e8) and
//  carry EH state stores, because the base subobject has to be unwound if
//  `operator new` or `operator delete` throws.  Build is /MT /W3 /GX /O2.
//
//  WHAT IS INLINE AND WHAT IS NOT -- this is load-bearing, because the
//  build has no /Gy, so ONLY inline (in-class) functions become COMDATs and
//  only COMDATs fold.  Close and VerifyFileOpen both land on 0x0048fab0,
//  which is also CRezMgr::DiskError, and GetFileName lands on 0x00490b40,
//  outside the library entirely -- so all three MUST be defined in the class
//  body.  Open (0x004909b0) folded with nothing and sits in the .text run at
//  exactly its declaration position between Write and Flush, so it is
//  defined out of line.  The same test puts ~CBaseRezFile out of line: it is
//  reached by a CALL from all four destructors (0x004902a9, 0x004906b2,
//  0x00490855) rather than inlined.
//
//  NAMING.  rezfile.h is not in this repository, so the class's real name is
//  not recoverable; `CRezFileDirectoryEmulationFile` is descriptive, not
//  attested.  The same goes for the two non-virtual members OpenFile
//  (0x00490a10) and CloseFile (0x00490ac0) -- what is attested is that they
//  are non-virtual (0x0049071a, 0x00490832, 0x00490888, 0x00490928 and
//  0x00490a34 all reach them with a direct call, from three different
//  classes' code) and that they are members of THIS class (every one of
//  those sites sets ecx to an element of the emulation object's lists).
//
// *********************************************************************** //

#include <stdio.h>
#include <string.h>


// ======================================================================= //
//  lithtypes.h
// ======================================================================= //

typedef int                 BOOL_Rf80;
typedef unsigned long       DWORD_Rf80;
typedef unsigned char       BYTE_Rf80;

#define TRUE_Rf80   1
#define FALSE_Rf80  0
#define NULL_Rf80   0


// ======================================================================= //
//  src/LT2/lithshared/lith/virtlist.h
//
//  Identical to rezmgr2.cpp's CVirtBaseList / CVirtBaseListItem; repeated
//  here only so this file is one translation unit.  Delete on the merge.
//  0x004a05b0 is InsertFirst and 0x004a05e0 is Delete; both are thiscall
//  with one pushed argument and both are reached from this chunk.
// ======================================================================= //

class CVirtBaseListItemRf80
{
public:
	CVirtBaseListItemRf80* Next() { return m_pNext; }
	CVirtBaseListItemRf80* Prev() { return m_pPrev; }

	virtual void VirtualFoo() = 0;                                  // slot 0

public:
	friend class CVirtBaseListRf80;

protected:
	CVirtBaseListItemRf80* m_pNext;          // +0x04
	CVirtBaseListItemRf80* m_pPrev;          // +0x08
};

class CVirtBaseListRf80
{
public:
	CVirtBaseListRf80() { m_pFirst = NULL_Rf80; m_pLast = NULL_Rf80; }
	~CVirtBaseListRf80() { }

	void Insert(CVirtBaseListItemRf80* pItem) { InsertFirst(pItem); }
	void InsertFirst(CVirtBaseListItemRf80* pItem);                 // 0x004a05b0
	void InsertLast(CVirtBaseListItemRf80* pItem);
	void Delete(CVirtBaseListItemRf80* pItem);                      // 0x004a05e0

	CVirtBaseListItemRf80* GetFirst() { return m_pFirst; }
	CVirtBaseListItemRf80* GetLast()  { return m_pLast; }

	virtual void VirtualFoo() = 0;

private:
	CVirtBaseListItemRf80* m_pFirst;         // +0x04
	CVirtBaseListItemRf80* m_pLast;          // +0x08
};


// ======================================================================= //
//  rezmgr.h -- only the three virtuals.  Vtable 0x004a3b7c holds exactly
//  three slots: 0x0048fa90 (`xor %eax,%eax ; ret $0x4`), 0x0048faa0
//  (`ret $0x4`) and 0x0048fab0 (`xor %eax,%eax ; ret`).  rezmgr.h calls
//  them Alloc / Free / DiskError and marks all three "currently not used or
//  implemented!".  Slot 2, at vtable offset 0x8, is the one this chunk
//  reaches -- five times, always as `call *0x8(%edx)` with no pushed
//  argument and ecx holding the manager.
// ======================================================================= //

class CRezMgrRf80
{
public:
	virtual void* Alloc(DWORD_Rf80 nSize);                          // 0x0048fa90
	virtual void  Free(void* pData);                                // 0x0048faa0
	virtual BOOL_Rf80 DiskError();                                  // 0x0048fab0
};


// ======================================================================= //
//  rezfile.h
//
//  CBaseRezFile is CVirtBaseListItem plus one pointer: 0x004901b0 writes the
//  vtable to +0x00 and the manager to +0x0c, 0x004901f0 writes the vtable
//  back and NULLs +0x0c.  Sizeof is 0x10, and the two derived sizes the
//  shipped `new`s ask for are 0x24 (CRezFile, 0x0048f021 is 0x38 for the
//  emulation and 0x0048f855 is 0x1c for the element below).
//
//  VirtualFoo is declared last but overrides CVirtBaseListItem's, so it
//  keeps slot 0 and the nine slots come out in the order the vtable dump
//  shows.  Seven of the nine are purecall in 0x004a3b90, so they are pure
//  here.
// ======================================================================= //

class CBaseRezFileRf80 : public CVirtBaseListItemRf80
{
public:
	CBaseRezFileRf80(CRezMgrRf80* pRezMgr);                         // 0x004901b0
	virtual ~CBaseRezFileRf80();                                    // 0x004901f0  slot 1

	virtual DWORD_Rf80 Read(DWORD_Rf80 nPos, DWORD_Rf80 nOffset,
	                        DWORD_Rf80 nSize, void* pData) = 0;     // slot 2
	virtual DWORD_Rf80 Write(DWORD_Rf80 nPos, DWORD_Rf80 nOffset,
	                         DWORD_Rf80 nSize, void* pData) = 0;    // slot 3
	virtual BOOL_Rf80 Open(const char* sFileName, BOOL_Rf80 bReadOnly,
	                       BOOL_Rf80 bCreateNew) = 0;               // slot 4
	virtual BOOL_Rf80 Close() = 0;                                  // slot 5
	virtual BOOL_Rf80 Flush() = 0;                                  // slot 6
	virtual BOOL_Rf80 VerifyFileOpen() = 0;                         // slot 7
	virtual const char* GetFileName() = 0;                          // slot 8

	virtual void VirtualFoo() { }                                   // slot 0, 0x00490200

	CRezMgrRf80* GetRezMgr() { return m_pRezMgr; }

protected:
	CRezMgrRf80* m_pRezMgr;                  // +0x0c
};


class CRezFileDirectoryEmulationRf80;
class CRezFileDirectoryEmulationFileRf80;


// ======================================================================= //
//  The list both of the emulation object's lists are.  One type, not two:
//  the constructor at 0x004905e0 writes the SAME vtable pointer,
//  0x004a3bfc, into +0x10 and +0x1c (`mov $0x4a3bfc,%ecx` once, stored
//  twice, 0x004905ef..0x004905fd).  Its one slot is 0x00490200, the empty
//  VirtualFoo body, so the override has an empty body and is inline.
// ======================================================================= //

class CRezFileDirectoryEmulationFileListRf80 : public CVirtBaseListRf80
{
public:
	CRezFileDirectoryEmulationFileRf80* GetFirst()
		{ return (CRezFileDirectoryEmulationFileRf80*)CVirtBaseListRf80::GetFirst(); }
	CRezFileDirectoryEmulationFileRf80* GetLast()
		{ return (CRezFileDirectoryEmulationFileRf80*)CVirtBaseListRf80::GetLast(); }

	virtual void VirtualFoo() { }                                   // 0x00490200
};


// ======================================================================= //
//  CRezFileDirectoryEmulation, 0x38 bytes, vtable 0x004a3bd8.  Declared in
//  full here because four of the functions below index into it; only its
//  data members are load-bearing for this file, so its own bodies
//  (0x004905e0, 0x00490630, 0x004906e0, 0x004906f0, 0x00490710, 0x00490730)
//  are left to whoever takes 0x004901b0..0x0049073f.
//
//  m_bReadOnly and m_bCreateNew are in THAT order, +0x30 then +0x34, and
//  their order is fixed by CRezFileDirectoryEmulation::Open (0x004906f0):
//      mov 0x8(%esp),%eax ; mov 0xc(%esp),%edx
//      mov %eax,0x30(%ecx) ; mov %edx,0x34(%ecx) ; mov $0x1,%eax ; ret $0xc
//  -- second argument to +0x30, third to +0x34, and slot 4's second
//  argument is bReadOnly (see the header note above).
// ======================================================================= //

class CRezFileDirectoryEmulationRf80 : public CBaseRezFileRf80
{
public:
	CRezFileDirectoryEmulationRf80(CRezMgrRf80* pRezMgr, int nMaxOpenFiles);  // 0x004905e0
	virtual ~CRezFileDirectoryEmulationRf80();                                // 0x00490650

	virtual DWORD_Rf80 Read(DWORD_Rf80 nPos, DWORD_Rf80 nOffset,
	                        DWORD_Rf80 nSize, void* pData) { return 0; }      // 0x004906e0
	virtual DWORD_Rf80 Write(DWORD_Rf80 nPos, DWORD_Rf80 nOffset,
	                         DWORD_Rf80 nSize, void* pData) { return 0; }     // 0x004906e0 (folded)
	virtual BOOL_Rf80 Open(const char* sFileName, BOOL_Rf80 bReadOnly,
	                       BOOL_Rf80 bCreateNew);                             // 0x004906f0
	virtual BOOL_Rf80 Close();                                                // 0x00490710
	virtual BOOL_Rf80 Flush() { return TRUE_Rf80; }                           // 0x00490730
	virtual BOOL_Rf80 VerifyFileOpen() { return TRUE_Rf80; }                  // 0x00490730 (folded)
	virtual const char* GetFileName() { return NULL_Rf80; }                   // 0x0048fab0 (folded)

public:
	friend class CRezFileDirectoryEmulationFileRf80;

	CRezFileDirectoryEmulationFileListRf80 m_lstOpenFiles;    // +0x10
	CRezFileDirectoryEmulationFileListRf80 m_lstClosedFiles;  // +0x1c
	int          m_nNumOpenFiles;            // +0x28
	int          m_nMaxOpenFiles;            // +0x2c
	BOOL_Rf80    m_bReadOnly;                // +0x30
	BOOL_Rf80    m_bCreateNew;               // +0x34
};


// ======================================================================= //
//  The class this file is about.  0x1c bytes (`push $0x1c ; call 0x4290b0`
//  at 0x0048f855), vtable 0x004a3c00.
//
//  +0x10 m_sFileName is proved twice: the constructor strcpy's the second
//  argument into it (0x0049079e) and GetFileName is
//  `mov 0x10(%ecx),%eax ; ret` (0x00490b40).
//  +0x14 m_pFile is the fopen result (0x00490a75) and the fseek/fread/
//  fwrite/fflush/fclose operand everywhere else.
//  +0x18 m_pDirEmulation is the constructor's third argument (0x00490773);
//  every reach into the emulation object below goes through it, INCLUDING
//  the manager: the five DiskError sites are
//  `mov 0x18(%esi),%X ; mov 0xc(%X),%ecx ; mov (%ecx),%edx ; call *0x8(%edx)`
//  -- the parent's m_pRezMgr, not this object's own, even though the
//  constructor hands the same pointer to the base.  That is a source fact,
//  not a codegen one, so it is spelled that way below.
// ======================================================================= //

class CRezFileDirectoryEmulationFileRf80 : public CBaseRezFileRf80
{
public:
	CRezFileDirectoryEmulationFileRf80(CRezMgrRf80* pRezMgr, char* sFileName,
	                                   CRezFileDirectoryEmulationRf80* pDirEmulation); // 0x00490740
	virtual ~CRezFileDirectoryEmulationFileRf80();                        // 0x00490800

	virtual DWORD_Rf80 Read(DWORD_Rf80 nPos, DWORD_Rf80 nOffset,
	                        DWORD_Rf80 nSize, void* pData);               // 0x00490870
	virtual DWORD_Rf80 Write(DWORD_Rf80 nPos, DWORD_Rf80 nOffset,
	                         DWORD_Rf80 nSize, void* pData);              // 0x00490910
	virtual BOOL_Rf80 Open(const char* sFileName, BOOL_Rf80 bReadOnly,
	                       BOOL_Rf80 bCreateNew);                         // 0x004909b0
	virtual BOOL_Rf80 Close() { return FALSE_Rf80; }                      // 0x0048fab0 (folded)
	virtual BOOL_Rf80 Flush();                                            // 0x004909c0
	virtual BOOL_Rf80 VerifyFileOpen() { return FALSE_Rf80; }             // 0x0048fab0 (folded)
	virtual const char* GetFileName() { return m_sFileName; }             // 0x00490b40 (folded)

	// Not virtual.  0x0049071a (CRezFileDirectoryEmulation::Close),
	// 0x00490832 (~this), 0x00490888 / 0x00490928 (Read / Write) and
	// 0x00490a34 (OpenFile) all reach these two with a direct call.
	BOOL_Rf80 OpenFile();                                                 // 0x00490a10
	BOOL_Rf80 CloseFile();                                                // 0x00490ac0

private:
	char*                           m_sFileName;       // +0x10
	FILE*                           m_pFile;           // +0x14
	CRezFileDirectoryEmulationRf80* m_pDirEmulation;   // +0x18
};


// ======================================================================= //
//  CBaseRezFile::CBaseRezFile  --  0x004901b0, 15 bytes, ret $0x4
//
//      mov %ecx,%eax ; mov 0x4(%esp),%ecx
//      movl $0x4a3b90,(%eax) ; mov %ecx,0xc(%eax) ; ret $0x4
//
//  0x004901b0 belongs at the head of rezfile.cpp.
//  It is here because the two constructors below CALL it and the call is
//  only generated if the definition is out of line.  Drop it on the merge if
//  that chunk lands first.
//
//  Calls, in order (0).
// ======================================================================= //

CBaseRezFileRf80::CBaseRezFileRf80(CRezMgrRf80* pRezMgr)
{
	m_pRezMgr = pRezMgr;
}


// ======================================================================= //
//  CBaseRezFile::~CBaseRezFile  --  0x004901f0, 14 bytes
//
//      movl $0x4a3b90,(%ecx) ; movl $0x0,0xc(%ecx) ; ret
//
//  Belongs in rezfile.cpp, same caveat.  Out of line, which is what makes 0x00490855 a
//  call instead of two inline stores.
//
//  Calls, in order (0).
// ======================================================================= //

CBaseRezFileRf80::~CBaseRezFileRf80()
{
	m_pRezMgr = NULL_Rf80;
}


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::CRezFileDirectoryEmulationFile
//  0x00490740..0x004907da, 0x9b bytes (155), ret $0xc
//  the retail server.dll 0x1003cd60
//
//  BEFORE the stated chunk; included because the class cannot be
//  instantiated without it and because nothing else enumerated covers
//  0x00490740.
//
//  Argument order comes off the only call site, 0x0048f893 in
//  CRezMgr::ReadEmulationDirectory:
//      push %ecx   (= 0x1794(%esp), the CRezFileDirectoryEmulation)
//      push %edx   (= lea 0xd68(%esp), the assembled path)
//      push %ecx   (= 0x14(%esp),  the CRezMgr)
//  reversed: (pRezMgr, sFileName, pDirEmulation).
//
//  /GX frame: `push $0xffffffff ; push $0x4a09c8` at 0x00490740, EH state 0
//  stored at 0x00490789 just before operator new, because the base
//  subobject has to be unwound if the allocation throws.
//
//  The two `repnz scasb` runs at 0x00490784 and 0x004907a1 are ONE
//  `strlen(sFileName) + 1` and VC6's intrinsic strcpy, which recomputes the
//  length rather than reusing it -- there is no cached length in the source.
//  There is no NULL test on the allocation (compare CRezFile::Open at
//  0x00490498, which does have one), so the copy is unconditional.
//
//  Calls, in order (3):
//    0x004901b0  CBaseRezFile::CBaseRezFile
//    0x004290b0  operator new
//    0x004a05b0  CVirtBaseList::InsertFirst
// ======================================================================= //

CRezFileDirectoryEmulationFileRf80::CRezFileDirectoryEmulationFileRf80(
	CRezMgrRf80* pRezMgr, char* sFileName,
	CRezFileDirectoryEmulationRf80* pDirEmulation)
	: CBaseRezFileRf80(pRezMgr)
{
	m_pDirEmulation = pDirEmulation;                         // 0x00490773
	m_pFile = NULL_Rf80;                                     // 0x00490781

	m_sFileName = (char*)::operator new(strlen(sFileName) + 1);  // 0x0049078d
	strcpy(m_sFileName, sFileName);                          // 0x004907a1..0x004907b9

	m_pDirEmulation->m_lstClosedFiles.InsertFirst(this);     // 0x004907c0
}


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::`scalar deleting destructor'
//  0x004907e0..0x004907fd, 0x1e bytes (30), ret $0x4
//  the retail server.dll 0x1003ce00
//
//  THIS IS THE ANSWER TO "is anything real at 0x004907e0..0x004907ff".
//  Something is there, and it is NOT source: it is the thunk VC6 emits
//  automatically for slot 1 of any class with a virtual destructor that is
//  destroyed through a base pointer.
//
//      push %esi ; mov %ecx,%esi ; call 0x00490800
//      testb $0x1,0x8(%esp) ; je out ; push %esi ; call 0x004290c0 ; add $4,%esp
//      out: mov %esi,%eax ; pop %esi ; ret $0x4
//
//  Byte for byte the same shape as 0x004901d0 (CBaseRezFile's), 0x00490240
//  (CRezFile's) and 0x00490630 (CRezFileDirectoryEmulation's).  There is
//  nothing to write for it; defining ~CRezFileDirectoryEmulationFile and
//  having the emulation object's destructor delete through the base pointer
//  is what produces it, and 0x00490686 / 0x0049069b (`push $0x1 ;
//  call *0x4(%eax)`) are exactly that delete.
//
//  Calls, in order (2):
//    0x00490800  CRezFileDirectoryEmulationFile::~CRezFileDirectoryEmulationFile
//    0x004290c0  operator delete
// ======================================================================= //


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::~CRezFileDirectoryEmulationFile
//  0x00490800..0x00490869, 106 bytes
//  the retail server.dll 0x1003ce20
//
//  /GX frame, handler 0x004a09e8, EH state 0 stored at 0x00490826.
//
//  The `if (m_pFile != NULL)` at 0x00490823 is redundant -- CloseFile opens
//  with the same test at 0x00490ac4 -- but it is in the shipped bytes, so it
//  is in the source.  By the time the body runs the object is always in
//  m_lstClosedFiles: it was either never opened, or CloseFile just moved it
//  there (0x00490b23).
//
//  Calls, in order (4):
//    0x00490ac0  CRezFileDirectoryEmulationFile::CloseFile
//    0x004290c0  operator delete
//    0x004a05e0  CVirtBaseList::Delete
//    0x004901f0  CBaseRezFile::~CBaseRezFile
// ======================================================================= //

CRezFileDirectoryEmulationFileRf80::~CRezFileDirectoryEmulationFileRf80()
{
	if (m_pFile != NULL_Rf80) CloseFile();                       // 0x00490823

	if (m_sFileName != NULL_Rf80) ::operator delete(m_sFileName);// 0x00490837

	m_pDirEmulation->m_lstClosedFiles.Delete(this);              // 0x0049084e
}


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::Read
//  0x00490870..0x0049090e, 159 bytes, ret $0x10
//  the retail server.dll 0x1003ce90
//
//  Frame: push ebx/esi/edi, no ebp, arguments at 0x10/0x14/0x18/0x1c(%esp)
//  after the three pushes.  nPos (0x10) is NEVER READ -- one emulated file
//  is one resource, so the resource's position in the archive means nothing
//  and only the offset within it is used.  edi = nSize (0x00490873),
//  ebx = nOffset (0x0049088d), ebx reloaded = pData (0x004908c9).
//
//  `test %edi,%edi ; jbe 0x00490907` at 0x00490877: UNSIGNED, so nSize is a
//  DWORD, and the fall-through is the body -- the source tests for the case
//  that continues, `if (nSize > 0)`, with the `return 0` at the bottom.
//  (CRezFile::Read at 0x004902ca is written the other way round: `ja` to the
//  body, with the `return 0` inline at the top.  Two shapes, two sources.)
//
//  Both loops are rotated while-loops, and the shape is unmistakable: the
//  condition is evaluated once before the body (0x00490891 / 0x004908c6) and
//  once at the bottom (0x004908b3 / 0x004908ed), with the body in between.
//  m_pFile is re-read at every one of the five sites because DiskError sits
//  between them -- it is repeated in the source, not cached.
//
//  fseek's third argument is the literal 0 pushed at 0x00490894 (SEEK_SET);
//  fread's are (pData, 1, nSize, m_pFile) read off the pushes at
//  0x004908cd..0x004908d1 in reverse.
//
//  Calls, in order (7, two of them indirect):
//    0x00490a10  CRezFileDirectoryEmulationFile::OpenFile
//    0x00492e74  fseek
//    [indirect]  *0x8(vtbl) CRezMgr::DiskError
//    0x00492e74  fseek
//    0x00493bad  fread
//    [indirect]  *0x8(vtbl) CRezMgr::DiskError
//    0x00493bad  fread
// ======================================================================= //

DWORD_Rf80 CRezFileDirectoryEmulationFileRf80::Read(DWORD_Rf80 nPos, DWORD_Rf80 nOffset,
                                                    DWORD_Rf80 nSize, void* pData)
{
	if (nSize > 0)
	{
		if (m_pFile == NULL_Rf80) OpenFile();                        // 0x00490881

		while (fseek(m_pFile, nOffset, SEEK_SET) != 0)               // 0x00490891
		{
			if (!m_pDirEmulation->GetRezMgr()->DiskError()) return 0;// 0x004908a4
		}

		DWORD_Rf80 nBytesRead = fread(pData, 1, nSize, m_pFile);     // 0x004908d2
		while (nBytesRead != nSize)
		{
			if (!m_pDirEmulation->GetRezMgr()->DiskError()) return 0;// 0x004908de
			nBytesRead = fread(pData, 1, nSize, m_pFile);            // 0x004908f5
		}

		return nBytesRead;
	}

	return 0;                                                        // 0x00490907
}


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::Write
//  0x00490910..0x004909ae, 159 bytes, ret $0x10
//  the retail server.dll 0x1003cf30
//
//  Instruction for instruction the same function as Read with fwrite in
//  place of fread: the two bodies differ only at 0x00490972 and 0x00490995,
//  where 0x00493bad becomes 0x00493cc4.  Every offset below is Read's plus
//  0xa0.  The nSize test is `jbe` again (0x0049091b), nPos is again never
//  read, and fwrite's arguments are (pData, 1, nSize, m_pFile).
//
//  Note what this does NOT do: CRezFile::Write (0x00490370) invalidates its
//  seek-position cache with `movl $0xffffffff,0x20(%esi)` at 0x0049037b and
//  never calls VerifyFileOpen.  This class has no cache and DOES open the
//  file first (0x00490928).
//
//  Calls, in order (7, two of them indirect):
//    0x00490a10  CRezFileDirectoryEmulationFile::OpenFile
//    0x00492e74  fseek
//    [indirect]  *0x8(vtbl) CRezMgr::DiskError
//    0x00492e74  fseek
//    0x00493cc4  fwrite
//    [indirect]  *0x8(vtbl) CRezMgr::DiskError
//    0x00493cc4  fwrite
// ======================================================================= //

DWORD_Rf80 CRezFileDirectoryEmulationFileRf80::Write(DWORD_Rf80 nPos, DWORD_Rf80 nOffset,
                                                     DWORD_Rf80 nSize, void* pData)
{
	if (nSize > 0)
	{
		if (m_pFile == NULL_Rf80) OpenFile();                        // 0x00490921

		while (fseek(m_pFile, nOffset, SEEK_SET) != 0)               // 0x00490931
		{
			if (!m_pDirEmulation->GetRezMgr()->DiskError()) return 0;// 0x00490944
		}

		DWORD_Rf80 nBytesWritten = fwrite(pData, 1, nSize, m_pFile); // 0x00490972
		while (nBytesWritten != nSize)
		{
			if (!m_pDirEmulation->GetRezMgr()->DiskError()) return 0;// 0x0049097e
			nBytesWritten = fwrite(pData, 1, nSize, m_pFile);        // 0x00490995
		}

		return nBytesWritten;
	}

	return 0;                                                        // 0x004909a7
}


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::Open
//  0x004909b0..0x004909b4, 5 bytes, ret $0xc
//  the retail server.dll 0x1003cfd0
//
//      xor %eax,%eax ; ret $0xc
//
//  IN THE GAP the enumeration left between the 0x00490910 row and the
//  0x004909c0 row, and it is a real function: slot 4 of vtable 0x004a3c00.
//  An emulated file is named by the directory walk that created it, so it
//  cannot be opened by name from outside; the override refuses.
//
//  It did NOT fold with any of the image's other `xor %eax,%eax ; ret $0xc`
//  bodies and it sits at exactly its declaration position between Write and
//  Flush -- with no /Gy in the build, that means it is defined OUT OF LINE,
//  unlike Close / VerifyFileOpen / GetFileName above.
//
//  Calls, in order (0).  Call-free, so it can only ever score BYTE-IDENTICAL.
// ======================================================================= //

BOOL_Rf80 CRezFileDirectoryEmulationFileRf80::Open(const char* sFileName,
                                                   BOOL_Rf80 bReadOnly, BOOL_Rf80 bCreateNew)
{
	return FALSE_Rf80;
}


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::Flush
//  0x004909c0..0x00490a08, 73 bytes
//  the retail server.dll 0x1003cfe0
//
//  `neg %eax ; sbb %eax,%eax ; inc %eax` at 0x004909d3 and 0x004909f5 is
//  VC6's materialisation of `(x == 0)` into a value -- not a branch on it.
//  The value is then branched on AND returned (0x004909d8 jumps to the
//  shared `pop %esi ; ret` at 0x00490a07 with it still in eax, and
//  0x004909fc falls into the other one), so the source holds it in a
//  variable and returns the variable rather than a literal TRUE.
//  CRezFile::Flush (0x00490540) does NOT do this -- it branches straight off
//  fflush's result and returns a literal -- which is the check that the
//  idiom here is real and not a peephole.
//
//  The NULL case returns TRUE, where CRezFile::Flush returns FALSE
//  (0x0049056d): a file this class has not opened yet has nothing to flush.
//
//  AND THAT `return TRUE` IS THE LAST BLOCK OF THE FUNCTION (0x00490a02,
//  after both of the other exits), with the null test JUMPING AWAY to it
//  (`je 0x00490a02`) and the body falling through.  CloseFile below is the
//  twin that does the opposite -- `jne 0x00490ad3` with the `return TRUE`
//  inline right after the test -- so the two are written differently in the
//  source and the layout is not the allocator's choice: this one wraps the
//  work in a positive guard and leaves the TRUE as the function's last
//  statement, CloseFile spells it as an early return.  Same 73 bytes either
//  way, which is why only the byte compare shows it.
//
//  Calls, in order (3, one of them indirect):
//    0x00494135  fflush
//    [indirect]  *0x8(vtbl) CRezMgr::DiskError
//    0x00494135  fflush
// ======================================================================= //

BOOL_Rf80 CRezFileDirectoryEmulationFileRf80::Flush()
{
	if (m_pFile != NULL_Rf80)                                        // 0x004909c3
	{
		BOOL_Rf80 bRetValue = (fflush(m_pFile) == 0);                // 0x004909cb
		while (!bRetValue)
		{
			if (!m_pDirEmulation->GetRezMgr()->DiskError()) return FALSE_Rf80;  // 0x004909da
			bRetValue = (fflush(m_pFile) == 0);                      // 0x004909ed
		}

		return bRetValue;                                            // 0x004909fc
	}

	return TRUE_Rf80;                                                // 0x00490a02
}


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::OpenFile
//  0x00490a10..0x00490abc, 173 bytes
//  the retail server.dll 0x1003d030
//
//  This is the whole point of the class: the lazy open, with the handle
//  budget enforced on the way in.
//
//  THE LRU EVICTION, 0x00490a1e..0x00490a38, runs ONCE and is outside the
//  retry loop -- the loop-back edge at 0x00490a92 targets 0x00490a39, not
//  0x00490a1e.
//      mov 0x18(%esi),%eax ; mov 0x28(%eax),%ecx ; mov 0x2c(%eax),%edx
//      cmp %edx,%ecx ; jle 0x00490a39
//  cmp with the +0x28 operand loaded first and `jle` (SIGNED) gives
//  `m_nNumOpenFiles > m_nMaxOpenFiles` in that operand order, and makes both
//  members `int` -- which agrees with rezmgr.h's
//  `int m_nMaxOpenFilesInEmulatedDir`.  The victim is
//  `mov 0x18(%eax),%eax`, +0x18 of the emulation object = m_pLast of the
//  list at +0x10, i.e. m_lstOpenFiles.GetLast(): new opens go in at the
//  FRONT (0x00490aab), so the last element is the least recently opened.
//  The eviction is a direct `call 0x00490ac0`, which is what makes CloseFile
//  non-virtual.
//
//  THE MODE TABLE, 0x00490a39..0x00490a6c.  +0x34 is tested first and +0x30
//  is loaded before the branch:
//      bCreateNew && bReadOnly  -> return FALSE   (0x00490a48)
//      bCreateNew && !bReadOnly -> "w+b"  0x004af7b0
//      !bCreateNew && bReadOnly -> "rb"   0x004a7638
//      !bCreateNew && !bReadOnly-> "r+b"  0x004af7ac
//  identical to CRezFile::Open's at 0x00490422..0x0049043c, which is where
//  the same three literals were first seen.  Each arm loads m_sFileName for
//  itself (into %edx, %eax and %ecx respectively) and pushes it before
//  jumping to the shared `call fopen` at 0x00490a6d -- three fopen calls in
//  the source, tail-merged by the compiler, not one call with a mode
//  variable.  (CRezFile::Open merges one push further because there the name
//  is already in a register from the parameter.)
//
//  THE RETRY, 0x00490a78..0x00490a92, is a do/while whose condition is
//  `m_pFile == NULL` and whose body contains the same test again -- which is
//  why m_pFile is re-read at 0x00490a8b after DiskError returns and why the
//  compiler emitted the exit branch twice (0x00490a7a and 0x00490a90) into
//  the same target.
//
//  The outer `if (m_pFile == NULL)` at 0x00490a13 doubles as the early
//  `return TRUE`: its `jne` goes to 0x00490ab6, the shared
//  `mov $0x1,%eax ; pop %esi ; ret` that the success path also falls into.
//
//  Calls, in order (5, one of them indirect):
//    0x00490ac0  CRezFileDirectoryEmulationFile::CloseFile
//    0x00492b41  fopen
//    [indirect]  *0x8(vtbl) CRezMgr::DiskError
//    0x004a05e0  CVirtBaseList::Delete
//    0x004a05b0  CVirtBaseList::InsertFirst
// ======================================================================= //

BOOL_Rf80 CRezFileDirectoryEmulationFileRf80::OpenFile()
{
	if (m_pFile == NULL_Rf80)                                        // 0x00490a13
	{
		if (m_pDirEmulation->m_nNumOpenFiles > m_pDirEmulation->m_nMaxOpenFiles)  // 0x00490a27
		{
			CRezFileDirectoryEmulationFileRf80* pRezFile = m_pDirEmulation->m_lstOpenFiles.GetLast();
			if (pRezFile != NULL_Rf80) pRezFile->CloseFile();        // 0x00490a34
		}

		do
		{
			if (m_pDirEmulation->m_bCreateNew)                       // 0x00490a3f
			{
				if (m_pDirEmulation->m_bReadOnly) return FALSE_Rf80; // 0x00490a46
				m_pFile = fopen(m_sFileName, "w+b");                 // 0x00490a4a
			}
			else if (m_pDirEmulation->m_bReadOnly)                   // 0x00490a55
			{
				m_pFile = fopen(m_sFileName, "rb");                  // 0x00490a59
			}
			else
			{
				m_pFile = fopen(m_sFileName, "r+b");                 // 0x00490a64
			}

			if (m_pFile == NULL_Rf80)                                // 0x00490a78
			{
				if (!m_pDirEmulation->GetRezMgr()->DiskError()) return FALSE_Rf80;  // 0x00490a7c
			}
		}
		while (m_pFile == NULL_Rf80);                                // 0x00490a8b

		m_pDirEmulation->m_lstClosedFiles.Delete(this);              // 0x00490a9f
		m_pDirEmulation->m_lstOpenFiles.InsertFirst(this);           // 0x00490aab
		m_pDirEmulation->m_nNumOpenFiles++;                          // 0x00490ab3
	}

	return TRUE_Rf80;                                                // 0x00490ab6
}


// ======================================================================= //
//  CRezFileDirectoryEmulationFile::CloseFile
//  0x00490ac0..0x00490b3b, 0x7c bytes
//  the retail server.dll 0x1003d0e0
//
//  AFTER the stated chunk, and the last function of rezmgr.lib: 0x00490b40
//  is already StdLith (and is where this class's inline GetFileName folded
//  to).  Included because it is what the whole chunk calls -- 0x00490832,
//  0x00490a34 and 0x0049071a all land here -- and because leaving it out
//  would leave the library's tail uncovered.
//
//  Same `neg/sbb/inc` value-materialisation as Flush, same rotated while,
//  and the same variable returned at the end (`mov %esi,%eax`, 0x00490b32).
//
//  The bookkeeping at 0x00490b09 is the exact inverse of OpenFile's, and its
//  order is in the bytes: decrement first (0x00490b0d..0x00490b13), then
//  Delete from the open list, then InsertFirst into the closed list, then
//  `movl $0x0,0x14(%edi)` -- m_pFile is nulled LAST, after both list
//  operations.  m_pDirEmulation is re-read before each of the three because
//  of the calls between them.
//
//  Calls, in order (5, one of them indirect):
//    0x00492a93  fclose
//    [indirect]  *0x8(vtbl) CRezMgr::DiskError
//    0x00492a93  fclose
//    0x004a05e0  CVirtBaseList::Delete
//    0x004a05b0  CVirtBaseList::InsertFirst
// ======================================================================= //

BOOL_Rf80 CRezFileDirectoryEmulationFileRf80::CloseFile()
{
	if (m_pFile == NULL_Rf80) return TRUE_Rf80;                      // 0x00490ac4

	BOOL_Rf80 bRetValue = (fclose(m_pFile) == 0);                    // 0x00490ad4
	while (!bRetValue)
	{
		if (!m_pDirEmulation->GetRezMgr()->DiskError()) return FALSE_Rf80;  // 0x00490ae5
		bRetValue = (fclose(m_pFile) == 0);                          // 0x00490af8
	}

	m_pDirEmulation->m_nNumOpenFiles--;                              // 0x00490b0d
	m_pDirEmulation->m_lstOpenFiles.Delete(this);                    // 0x00490b1a
	m_pDirEmulation->m_lstClosedFiles.InsertFirst(this);             // 0x00490b26
	m_pFile = NULL_Rf80;                                             // 0x00490b2b

	return bRetValue;                                                // 0x00490b32
}


// ======================================================================= //
//  WHAT IS NOT HERE, and why
//
//  0x00490200  VirtualFoo            -- inline `{ }` in CBaseRezFile above.
//  0x0048fab0  Close / VerifyFileOpen-- inline `{ return FALSE; }`.
//  0x00490b40  GetFileName           -- inline `{ return m_sFileName; }`.
//  0x004907e0  scalar deleting dtor  -- compiler-generated, see its block.
//
//  0x004901b0..0x0049073f (CBaseRezFile's two bodies, all of CRezFile, all
//  of CRezFileDirectoryEmulation) is rezfile.cpp's; the two
//  CBaseRezFile bodies are duplicated here only because this file has to be
//  a standalone TU and the calls to them would otherwise not be generated.
//
//  UNRESOLVED
//
//  // UNDECODED: the class's real name.  rezfile.h is not in this
//  // repository (rezmgr.h includes it as "..\rezmgr\rezfile.h" at line 40
//  // and it is the one header of the set that is missing), so
//  // CRezFileDirectoryEmulationFile, OpenFile and CloseFile are descriptive
//  // names, not attested ones.  Nothing about them affects codegen.
//
//  // UNDECODED: CRezMgr::Alloc's and ::Free's signatures.  Only slot 2,
//  // DiskError, is reached from this chunk; 0x0048fa90 and 0x0048faa0 are
//  // `ret $0x4` shapes, so each takes one dword, but which dword is not
//  // visible from here and rezmgr.h says only "currently not used or
//  // implemented!".
//
//  // UNDECODED: whether m_bReadOnly/m_bCreateNew are the names rezfile.h
//  // uses for +0x30/+0x34 of the emulation object.  What IS pinned is which
//  // is which: +0x34 selects the "w+b" arm and +0x30 selects "rb", exactly
//  // as CRezFile::Open's third and second arguments do.
// ======================================================================= //
