// *********************************************************************** //
//
//  MODULE:   REZFILE (.CPP) -- 0x004901b0..0x004907db of lithtech.exe
//
//  WHAT THIS IS
//
//  This is the file-I/O layer of Monolith's RezMgr library (rezmgr.lib).
//  Two sibling chunks of the same library are already in the repository at
//  src/lithshared/rezmgr/rezmgr.cpp (suffix `Rz48`, 0x0048df30..0x0048e62a)
//  and src/lithshared/rezmgr/rezmgr2.cpp (suffix `X720`,
//  0x0048e720..0x0048f37a); rezmgr2.cpp carries the verified CRezItm /
//  CRezTyp / CRezDir / CRezMgr layouts and the on-disk RezFileHeader. The
//  public header is in the repository unmodified at
//  src/LT2/lithshared/incs/rezmgr.h; the header this file's classes are
//  declared in (rezmgr's own rezfile.h) is NOT in the repository, so every
//  class below is reconstructed from the shipped code.
//
//  Everything here carries the suffix `Rf90` -- named for CBaseRezFile's
//  vtable at 0x004a3b90 -- so all three files can be compiled together
//  without colliding. Strip the suffix to get the real name.
//
//  THE SIBLING'S CBaseRezFile VTABLE IS EXACTLY RIGHT.  rezmgr2.cpp
//  reconstructed CBaseRezFile's nine slots from two indirect call sites and
//  guessed the five it could not reach. Every one of its guesses is
//  confirmed here by CRezFile's vtable at 0x004a3bb4, whose nine slots are
//  all concrete functions in this file:
//
//      slot 0  +0x00  0x00490200  VirtualFoo          (from CVirtBaseListItem)
//      slot 1  +0x04  0x00490240  scalar deleting dtor
//      slot 2  +0x08  0x004902c0  Read
//      slot 3  +0x0c  0x00490370  Write
//      slot 4  +0x10  0x00490410  Open
//      slot 5  +0x14  0x004904e0  Close
//      slot 6  +0x18  0x00490540  Flush
//      slot 7  +0x1c  0x00490580  VerifyFileOpen
//      slot 8  +0x20  0x004905d0  GetFileName
//
//  CBaseRezFile's own vtable 0x004a3b90 is {0x00490200, 0x004901d0, and
//  seven _purecall (0x004928e7)}, so slots 2..8 are pure virtual there --
//  again exactly what rezmgr2.cpp said. CRezFile::VerifyFileOpen
//  (0x00490580) calls `*0x10(vptr)` with three arguments and
//  CRezFileDirectoryEmulation::Open (0x004906f0) takes `ret $0xc`, so slot
//  4 is the three-argument Open; slot 8 is proved by 0x004905d0
//  (`mov 0x14(%ecx),%eax; ret`) returning the name pointer.
//
//  WHAT THIS FILE ADDS OVER THE TWO SIBLINGS
//
//    * CRezFile complete: its five data members, its 0x24 size, and eight
//      of its nine virtuals as real bodies rather than declarations.
//    * CRezFileDirectoryEmulation complete at 0x38 bytes -- TWO embedded
//      lists, a file counter, the max-open-files limit and the two open
//      flags -- plus its constructor and destructor.
//    * A fourth class, the per-file object a directory emulation hands out
//      (vtable 0x004a3c00); its constructor at 0x00490740 is mine.
//    * The four CRT entry points this layer is built on, identified from
//      their own bodies rather than assumed:
//          0x00492b41 fopen   (pushes $0x40 = _SH_DENYNO into _fsopen
//                              at 0x00492b10)
//          0x00492a93 fclose  (_lock_str 0x004973e1 / _fclose_lk 0x00492ac4
//                              / _unlock_str 0x00497433)
//          0x00492cf1 ftell   (_ftell_lk 0x00492d13)
//          0x00492e74 fseek   (_fseek_lk 0x00492ea0, which calls
//                              _ftell_lk for SEEK_CUR)
//          0x00493bad fread   (_fread_lk 0x00493bdc, which calls _filbuf
//                              at 0x004977c8)
//          0x00493cc4 fwrite  (_fwrite_lk 0x00493cf3)
//          0x004290b0 operator new, 0x004290c0 operator delete
//          0x004a05b0 CVirtBaseList::InsertFirst,
//          0x004a05e0 CVirtBaseList::Delete
//
//  DISAGREEMENT WITH rezmgr2.cpp: none of substance. Its CBaseRezFile
//  `Read`/`Write` second parameter, marked UNDECODED there because the only
//  call site it could see passes 0, is decoded here: CRezFile::Read adds
//  the first two arguments together and seeks to the sum
//  (`lea (%ecx,%eax,1),%ebp` at 0x004902df), so they are a base position
//  and an offset within it. Its two concrete sizes (0x24 for CRezFile,
//  0x38 for CRezFileDirectoryEmulation) are confirmed member by member.
//
//  ONE CORRECTION TO THE SIBLING'S SEAM MAP. rezmgr2.cpp puts rezfile.cpp
//  at 0x004901b0..0x00490abd. Its start is right; its end is not. The
//  per-file class's vtable at 0x004a3c00 has GetFileName in slot 8 at
//  0x00490b40 (`mov 0x10(%ecx),%eax; ret`), which is that class's own
//  accessor and therefore still this module, and its Close at 0x00490ac0
//  runs to 0x00490b3a. The next function, 0x00490b50, belongs to something
//  else entirely -- it rounds a size up to a multiple of four and clears
//  +0x0c/+0x10 of a structure, with no rez type in sight. So the module is
//  0x004901b0..0x00490b43, and 0x00490abe..0x00490b43 is unclaimed.
//
//  TRUE EXTENTS. One enumerated row is wrong: 0x00490650 is 119 bytes
//  (0x00490650..0x004906c6), not 135. The remaining 16 bytes of that row
//  are nine bytes of lea-nop padding and then a SEPARATE 7-byte function at
//  0x004906d0 (`movl $0x4a3b8c,(%ecx); ret`), which is
//  CVirtBaseList::~CVirtBaseList -- emitted out of line only because the EH
//  unwind funclet for ~CRezFileDirectoryEmulation needs its address.
//  An extent check flags it as "a SEPARATE function ... nothing EARLIER in the
//  range branches to". Every other row measured exactly as stated.
//
//  /GX IS REQUIRED. Three of the bodies here open with VC6's two-push
//  __CxxFrameHandler registration (`push $0xffffffff; push $ehhandler;
//  mov %fs:0x0,%eax; push %eax; mov %esp,%fs:0x0`) and store an EH state
//  into the frame:
//      0x00490260  ~CRezFile                   handler 0x004a0978, state 0
//      0x00490650  ~CRezFileDirectoryEmulation handler 0x004a09ae, state 2
//      0x00490740  the emulated-file ctor      handler 0x004a09c8, state 0
//  0x004a0978 is `mov $0x004a4690,%eax; jmp __CxxFrameHandler` with an
//  unwind funclet at 0x004a0990 that jumps to 0x004901f0
//  (~CBaseRezFile) -- which is what pins ~CRezFile's state 0 to "the base
//  subobject is live". ~CRezFileDirectoryEmulation's state 2 counts the
//  base plus TWO members with destructors, which is the independent proof
//  that the two twelve-byte lists at +0x10 and +0x1c are members and not
//  raw fields.
//
//  BOTH BINARIES SCORE THESE: rezmgr.lib is prebuilt and linked into
//  lithtech.exe and server.dll alike. All nine were located in
//  the retail server.dll by searching it for the longest window out of each
//  that carries no absolute address and no e8/e9 rel32 -- the two things
//  that move between images. Every one matched exactly once, and all nine
//  landed at the same delta, +0x0fbac620:
//
//      0x00490260 -> 0x1003c880      0x004904e0 -> 0x1003cb00
//      0x004902c0 -> 0x1003c8e0      0x00490580 -> 0x1003cba0
//      0x00490370 -> 0x1003c990      0x004905e0 -> 0x1003cc00
//      0x00490410 -> 0x1003ca30      0x00490650 -> 0x1003cc70
//                                    0x00490740 -> 0x1003cd60
//
//  A single consistent delta across nine independently located windows is
//  the whole object being the same object in both images. rezmgr2.cpp's
//  neighbouring nine sit at +0x0fbac620 too (0x0048e720 -> 0x1003ad40).
//
//  THE NINE ROWS (the rest of the file is the surrounding module, included
//  so the translation unit is coherent and so the vtables come out right):
//
//      0x00490260   94  CRezFile::~CRezFile
//      0x004902c0  176  CRezFile::Read
//      0x00490370  151  CRezFile::Write
//      0x00490410  193  CRezFile::Open
//      0x004904e0   86  CRezFile::Close
//      0x00490580   65  CRezFile::VerifyFileOpen
//      0x004905e0   70  CRezFileDirectoryEmulation::CRezFileDirectoryEmulation
//      0x00490650  119  CRezFileDirectoryEmulation::~CRezFileDirectoryEmulation
//      0x00490740  155  CRezFileDirectoryEmulationFile::ctor
//
// *********************************************************************** //

#include <stdio.h>
#include <string.h>


// ======================================================================= //
//  lithtypes.h / reztypes.h -- only the widths are load-bearing.
// ======================================================================= //

typedef int                 BOOL_Rf90;
typedef unsigned long       DWORD_Rf90;
typedef unsigned char       BYTE_Rf90;

#define TRUE_Rf90   1
#define FALSE_Rf90  0
#define NULL_Rf90   0


// ======================================================================= //
//  src/LT2/lithshared/lith/virtlist.h -- reproduced from rezmgr2.cpp, which
//  derived it independently. The item's vptr is at +0x00, m_pNext at +0x04
//  and m_pPrev at +0x08; the list's vptr is at +0x00, m_pFirst at +0x04 and
//  m_pLast at +0x08, which is why a list member is twelve bytes.
//
//  CVirtBaseList's own vtable is the one-slot purecall table at 0x004a3b8c:
//  ~CRezFileDirectoryEmulation parks both of its list members' vptrs on it
//  at 0x004906a5 as it unwinds them, and the out-of-line copy of that
//  destructor is the 7-byte function at 0x004906d0.
// ======================================================================= //

class CVirtBaseListRf90;

class CVirtBaseListItemRf90
{
public:
	CVirtBaseListItemRf90* Next() { return m_pNext; }
	CVirtBaseListItemRf90* Prev() { return m_pPrev; }

	virtual void VirtualFoo() = 0;                   // vtable slot 0

	friend class CVirtBaseListRf90;

protected:
	CVirtBaseListItemRf90* m_pNext;                  // +0x04
	CVirtBaseListItemRf90* m_pPrev;                  // +0x08
};

class CVirtBaseListRf90
{
public:
	CVirtBaseListRf90() { m_pFirst = NULL_Rf90; m_pLast = NULL_Rf90; }
	~CVirtBaseListRf90() { }                         // 0x004906d0

	void Insert(CVirtBaseListItemRf90* pItem) { InsertFirst(pItem); }
	void InsertFirst(CVirtBaseListItemRf90* pItem); // 0x004a05b0
	void InsertLast(CVirtBaseListItemRf90* pItem);
	void Delete(CVirtBaseListItemRf90* pItem);      // 0x004a05e0

	CVirtBaseListItemRf90* GetFirst() { return m_pFirst; }
	CVirtBaseListItemRf90* GetLast()  { return m_pLast; }

	virtual void VirtualFoo() = 0;

private:
	CVirtBaseListItemRf90* m_pFirst;                 // +0x04
	CVirtBaseListItemRf90* m_pLast;                  // +0x08
};


// ======================================================================= //
//  rezmgr.h, LITE build -- only the three virtuals this file reaches.
//
//  CBaseRezFile keeps a CRezMgr* at +0x0c (0x004901bc) and every error path
//  in this module calls `*0x8(vptr)` on it -- the third and last slot of
//  CRezMgr's three-slot vtable at 0x004a3b7c, whose bodies are 0x0048fa90
//  (`return 0`), 0x0048faa0 (`return`) and 0x0048fab0 (`return 0`):
//  Alloc, Free and DiskError, the three rezmgr.h marks "currently not used
//  or implemented!". So `*0x8(vptr)` is DiskError, and the shape of every
//  retry loop below -- "do it again if the user says try again" -- is
//  exactly what rezmgr.h documents DiskError for.
// ======================================================================= //

class CRezMgrRf90
{
public:
	virtual void* Alloc(DWORD_Rf90 NumBytes);        // 0x0048fa90
	virtual void  Free(void* Ptr);                   // 0x0048faa0
	virtual BOOL_Rf90 DiskError();                   // 0x0048fab0
};


// ======================================================================= //
//  rezfile.h -- CBaseRezFile.  Vtable 0x004a3b90, nine slots, seven of them
//  _purecall (0x004928e7). Sixteen bytes: the CVirtBaseListItem base plus
//  m_pRezMgr.
// ======================================================================= //

class CBaseRezFileRf90 : public CVirtBaseListItemRf90
{
public:
	CBaseRezFileRf90(CRezMgrRf90* pRezMgr);                        // 0x004901b0
	virtual ~CBaseRezFileRf90();                                   // slot 1

	// Pos and Offset are summed and seeked to; see CRezFile::Read.
	virtual DWORD_Rf90 Read(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
	                        DWORD_Rf90 Size, void* pData) = 0;     // slot 2
	virtual DWORD_Rf90 Write(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
	                         DWORD_Rf90 Size, void* pData) = 0;    // slot 3
	virtual BOOL_Rf90  Open(const char* sFileName, BOOL_Rf90 bReadOnly,
	                        BOOL_Rf90 bCreateNew) = 0;             // slot 4
	virtual BOOL_Rf90  Close() = 0;                                // slot 5
	virtual BOOL_Rf90  Flush() = 0;                                // slot 6
	virtual BOOL_Rf90  VerifyFileOpen() = 0;                       // slot 7
	virtual const char* GetFileName() = 0;                         // slot 8

	virtual void VirtualFoo();                                     // slot 0

protected:
	CRezMgrRf90* m_pRezMgr;                          // +0x0c
};


// ======================================================================= //
//  rezfile.h -- CRezFile.  Vtable 0x004a3bb4, 0x24 bytes.
//
//  +0x10  FILE*  m_pFile          fopen's result (0x0049044a), fclose's
//                                 argument (0x004904ed), fflush's
//                                 (0x00490554), ftell's (0x00490593)
//  +0x14  char*  m_sFileName      the strcpy'd copy Open makes
//                                 (0x00490498), and what GetFileName
//                                 returns (0x004905d0)
//  +0x18  BOOL   m_bReadOnly      Open's second argument (0x00490472),
//                                 fed straight back to Open by
//                                 VerifyFileOpen (0x004905a8)
//  +0x1c  ??     UNDECODED        no instruction anywhere in
//                                 0x004901b0..0x00490abd reads or writes
//                                 it. Declared only to put m_nLastSeekPos
//                                 at +0x20 and sizeof at the 0x24 that
//                                 CRezMgr::Open's `new` asks for.
//  +0x20  DWORD  m_nLastSeekPos   the cached file position: set to
//                                 0xffffffff by the constructor
//                                 (0x0049022b) and by every operation that
//                                 invalidates it, compared against the
//                                 wanted position at 0x004902e5 to skip a
//                                 redundant fseek, and advanced past the
//                                 bytes read at 0x00490353.
// ======================================================================= //

class CRezFileRf90 : public CBaseRezFileRf90
{
public:
	CRezFileRf90(CRezMgrRf90* pRezMgr);                            // 0x00490210
	virtual ~CRezFileRf90();                                       // 0x00490260

	virtual DWORD_Rf90 Read(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
	                        DWORD_Rf90 Size, void* pData);         // 0x004902c0
	virtual DWORD_Rf90 Write(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
	                         DWORD_Rf90 Size, void* pData);        // 0x00490370
	virtual BOOL_Rf90  Open(const char* sFileName, BOOL_Rf90 bReadOnly,
	                        BOOL_Rf90 bCreateNew);                 // 0x00490410
	virtual BOOL_Rf90  Close();                                    // 0x004904e0
	virtual BOOL_Rf90  Flush();                                    // 0x00490540
	virtual BOOL_Rf90  VerifyFileOpen();                           // 0x00490580
	virtual const char* GetFileName();                             // 0x004905d0

private:
	FILE*       m_pFile;                             // +0x10
	char*       m_sFileName;                         // +0x14
	BOOL_Rf90   m_bReadOnly;                         // +0x18
	DWORD_Rf90  m_nUnused_1c;                        // +0x1c  UNDECODED
	DWORD_Rf90  m_nLastSeekPos;                      // +0x20
};


// ======================================================================= //
//  rezfile.h -- the directory-emulation classes.
//
//  CRezFileDirectoryEmulation is what CRezMgr::Open builds when the name it
//  is handed is a real directory rather than a .rez file (rezmgr2.cpp,
//  0x0048ef80); it presents that directory as if it were an archive. Its
//  vtable is 0x004a3bd8 and it is 0x38 bytes:
//
//  +0x10  a twelve-byte list  vptr 0x004a3bfc, first +0x14, last +0x18
//  +0x1c  a twelve-byte list  vptr 0x004a3bfc, first +0x20, last +0x24
//  +0x28  DWORD               decremented by the per-file Close at
//                             0x00490b10, so it is the open-file count
//  +0x2c  int                 the constructor's second argument, which
//                             CRezMgr passes m_nMaxOpenFilesInEmulatedDir
//  +0x30  BOOL                Open's second argument (0x004906f8);
//                             the constructor defaults it to TRUE
//  +0x34  BOOL                Open's third argument (0x004906fb);
//                             the constructor defaults it to FALSE
//
//  BOTH LISTS ARE THE SAME CLASS. Their vptrs are the same address,
//  0x004a3bfc, and a vtable is a per-class COMDAT that the VC6 linker's
//  /OPT:ICF does not fold (it folds .text only -- which is why three
//  different vtable slots in this module share the body at 0x0048fab0 while
//  the identical one-slot vtables 0x004a3b88 (CBaseRezFileList, from
//  rezmgr2.cpp) and 0x004a3bfc stay distinct). So these are two members of
//  one list type, and that type is NOT CBaseRezFileList.
//
//  WHICH LIST IS WHICH. The per-file object's constructor (0x00490740,
//  mine) inserts itself into the +0x1c list; the per-file Close
//  (0x00490ac0) decrements +0x28 and removes itself from the +0x10 list.
//  An intrusive list item has one m_pNext/m_pPrev pair, so an object cannot
//  sit in both at once: the file is moved from one to the other as it is
//  opened and closed, and +0x10 is therefore the list of the files
//  currently open (the one +0x28 counts and +0x2c caps) while +0x1c holds
//  the rest. The destructor empties +0x10 first and +0x1c second.
//
//  UNDECODED: the real names of this list class and of the per-file class.
//  Everything about their shape is pinned; only the identifiers are guesses.
// ======================================================================= //

class CRezFileDirectoryEmulationRf90;

class CRezFileEmuListRf90 : public CVirtBaseListRf90
{
public:
	CBaseRezFileRf90* GetFirst()
		{ return (CBaseRezFileRf90*)CVirtBaseListRf90::GetFirst(); }

	virtual void VirtualFoo() { }                    // folded onto 0x00490200
};

class CRezFileDirectoryEmulationRf90 : public CBaseRezFileRf90
{
public:
	CRezFileDirectoryEmulationRf90(CRezMgrRf90* pRezMgr,
	                               int nMaxOpenFiles);             // 0x004905e0
	virtual ~CRezFileDirectoryEmulationRf90();                     // 0x00490650

	virtual DWORD_Rf90 Read(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
	                        DWORD_Rf90 Size, void* pData);         // 0x004906e0
	virtual DWORD_Rf90 Write(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
	                         DWORD_Rf90 Size, void* pData);        // 0x004906e0
	virtual BOOL_Rf90  Open(const char* sFileName, BOOL_Rf90 bReadOnly,
	                        BOOL_Rf90 bCreateNew);                 // 0x004906f0
	virtual BOOL_Rf90  Close();                                    // 0x00490710
	virtual BOOL_Rf90  Flush();                                    // 0x00490730
	virtual BOOL_Rf90  VerifyFileOpen();                           // 0x00490730
	virtual const char* GetFileName();                             // 0x0048fab0

	friend class CRezFileDirectoryEmulationFileRf90;

private:
	CRezFileEmuListRf90 m_lstOpenFiles;              // +0x10
	CRezFileEmuListRf90 m_lstRezFiles;               // +0x1c
	DWORD_Rf90          m_nNumOpenFiles;             // +0x28
	int                 m_nMaxOpenFiles;             // +0x2c
	BOOL_Rf90           m_bReadOnly;                 // +0x30
	BOOL_Rf90           m_bCreateNew;                // +0x34
};

//  One real file inside an emulated directory. Vtable 0x004a3c00:
//      slot 1 0x004907e0 deleting dtor (body 0x00490800)
//      slot 2 0x00490870 Read      slot 3 0x00490910 Write
//      slot 4 0x004909b0 Open      slot 5 0x0048fab0 Close (`return 0`)
//      slot 6 0x004909c0 Flush     slot 7 0x0048fab0 VerifyFileOpen
//      slot 8 0x00490b40 GetFileName -- `mov 0x10(%ecx),%eax; ret`, which
//                                    is what pins m_sFileName to +0x10.
//  +0x14 is its FILE* (0x00490ac4 hands it to fclose) and +0x18 its owner
//  (0x00490ae5 walks +0x18 -> +0x0c -> DiskError).  Only its constructor
//  is in this range.

class CRezFileDirectoryEmulationFileRf90 : public CBaseRezFileRf90
{
public:
	CRezFileDirectoryEmulationFileRf90(CRezMgrRf90* pRezMgr,
	                                   const char* sFileName,
	                                   CRezFileDirectoryEmulationRf90* pDirEmulation);
	                                                               // 0x00490740
	virtual ~CRezFileDirectoryEmulationFileRf90();                 // 0x00490800

	virtual DWORD_Rf90 Read(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
	                        DWORD_Rf90 Size, void* pData);         // 0x00490870
	virtual DWORD_Rf90 Write(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
	                         DWORD_Rf90 Size, void* pData);        // 0x00490910
	virtual BOOL_Rf90  Open(const char* sFileName, BOOL_Rf90 bReadOnly,
	                        BOOL_Rf90 bCreateNew);                 // 0x004909b0
	virtual BOOL_Rf90  Close();                                    // 0x0048fab0
	virtual BOOL_Rf90  Flush();                                    // 0x004909c0
	virtual BOOL_Rf90  VerifyFileOpen();                           // 0x0048fab0
	virtual const char* GetFileName();                             // 0x00490b40

	// NOT virtual: CRezFileDirectoryEmulation::Close reaches 0x00490ac0 with
	// a direct `call`, and slot 5 of this class's vtable 0x004a3c00 is the
	// shared `xor %eax,%eax ; ret` at 0x0048fab0, not 0x00490ac0.
	BOOL_Rf90 CloseFile();                                         // 0x00490ac0

private:
	char*                           m_sFileName;     // +0x10
	FILE*                           m_pFile;         // +0x14
	CRezFileDirectoryEmulationRf90* m_pDirEmulation; // +0x18
};


// ======================================================================= //
//  0x004901b0  CBaseRezFile::CBaseRezFile                          18 bytes
//
//  No calls. `movl $0x4a3b90,(%eax)` then `mov %ecx,0xc(%eax)`: vptr, then
//  the manager pointer. The CVirtBaseListItem base contributes nothing but
//  the vptr, so m_pNext and m_pPrev are left alone -- CVirtBaseList::Insert
//  writes them.
//
//  NOT A ROW (before 0x00490260); here so the class is whole.
// ======================================================================= //

CBaseRezFileRf90::CBaseRezFileRf90(CRezMgrRf90* pRezMgr)
{
	m_pRezMgr = pRezMgr;
}


// ======================================================================= //
//  0x004901f0  CBaseRezFile::~CBaseRezFile                         14 bytes
//
//  No calls. Restores the vptr and nulls m_pRezMgr. The deleting form at
//  0x004901d0 is compiler-generated.
//
//  NOT A ROW.
// ======================================================================= //

CBaseRezFileRf90::~CBaseRezFileRf90()
{
	m_pRezMgr = NULL_Rf90;
}


// ======================================================================= //
//  0x00490200  CBaseRezFile::VirtualFoo                             1 byte
//
//  A bare `ret`. /OPT:ICF folds every empty VirtualFoo in the library onto
//  it, which is why it appears as slot 0 of 0x004a3b90, 0x004a3bb4,
//  0x004a3bd8 and 0x004a3c00 and as the only slot of 0x004a3b88 and
//  0x004a3bfc.
//
//  NOT A ROW.
// ======================================================================= //

void CBaseRezFileRf90::VirtualFoo()
{
}


// ======================================================================= //
//  0x00490210  CRezFile::CRezFile                                  38 bytes
//
//  Calls (1): 0x004901b0 CBaseRezFile::CBaseRezFile.
//
//  One `xor %eax,%eax` feeds both zero stores, so m_pFile and m_sFileName
//  are cleared with the same constant; m_nLastSeekPos gets its own
//  `movl $0xffffffff`. +0x18 and +0x1c are left uninitialised.
//
//  NOT A ROW.
// ======================================================================= //

CRezFileRf90::CRezFileRf90(CRezMgrRf90* pRezMgr) : CBaseRezFileRf90(pRezMgr)
{
	m_pFile = NULL_Rf90;
	m_sFileName = NULL_Rf90;
	m_nLastSeekPos = 0xffffffff;
}


// ======================================================================= //
//  0x00490260  CRezFile::~CRezFile                          ROW, 94 bytes
//
//  Calls, in order (3):
//      0x004904e0  CRezFile::Close        (direct -- devirtualised inside
//                                          the destructor)
//      0x004290c0  operator delete
//      0x004901f0  CBaseRezFile::~CBaseRezFile
//
//  /GX frame: `push $0xffffffff; push $0x004a0978` and the fs:0 chain, with
//  the object pointer parked at 0x4(%esp) (0x00490279) and EH state 0
//  written to 0x10(%esp) (0x00490286). The funclet at 0x004a0990 jumps to
//  0x004901f0, i.e. state 0 means "the base subobject still has to be
//  destroyed". Nothing in the source asks for that frame -- it is what
//  having a base with a destructor and a body that can throw costs.
//
//  Two shapes the bytes decide. `test %eax,%eax; je` on m_pFile and again
//  on m_sFileName: both tests fall through into the work, so both are
//  written for the case that CONTINUES. And the explicit null test before
//  operator delete has to be in the source -- MSVC emits no null check of
//  its own for `delete` of a non-class pointer.
// ======================================================================= //

CRezFileRf90::~CRezFileRf90()
{
	if (m_pFile != NULL_Rf90) Close();
	if (m_sFileName != NULL_Rf90) delete m_sFileName;
}


// ======================================================================= //
//  0x004902c0  CRezFile::Read                              ROW, 176 bytes
//
//  Calls, in order (6):
//      0x004902f0  0x00492e74  fseek
//      0x00490301  *0x8(m_pRezMgr's vptr)  CRezMgr::DiskError
//      0x0049030f  0x00492e74  fseek
//      0x00490327  0x00493bad  fread
//      0x00490338  *0x8(m_pRezMgr's vptr)  CRezMgr::DiskError
//      0x00490347  0x00493bad  fread
//
//  The two arguments the sibling could not separate are summed here:
//  `mov 0x18(%esp),%eax; mov 0x14(%esp),%ecx; lea (%ecx,%eax,1),%ebp` at
//  0x004902d7 makes the seek target out of argument 1 plus argument 2.
//  (Which of the two is the lea's base and which its index is not
//  reachable from source, so the sum is written in declaration order.)
//
//  Four shapes the bytes decide.
//
//   1. `test %edi,%edi; ja` -- ja, not jne, so the guard is an UNSIGNED
//      relational test on Size and not a comparison with zero; and the
//      `return 0` it jumps over is laid out immediately after it, which is
//      what an early return looks like.
//   2. `cmp %ebp,%eax` with m_nLastSeekPos in %eax, so the member is
//      written on the LEFT of the seek-skip test.
//   3. Both loops evaluate their call TWICE -- once before the loop and
//      once at the bottom (0x004902f0/0x0049030f and 0x00490327/0x00490347)
//      -- while CRezFile::Close's identical-looking loop over fclose emits
//      only one copy. The compiler did not duplicate anything in Close, so
//      it did not duplicate anything here either: the source holds each
//      result in a variable and assigns it in two places. For fread that is
//      forced anyway, because the count is needed after the loop.
//   4. Both failure exits jump to ONE block at 0x00490360 that stores
//      0xffffffff and returns 0; that is tail merging of two identical
//      source statements, not one shared label.
// ======================================================================= //

DWORD_Rf90 CRezFileRf90::Read(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
                              DWORD_Rf90 Size, void* pData)
{
	DWORD_Rf90 nSeekPos;
	DWORD_Rf90 nBytesRead;
	int        nResult;

	if (Size <= 0) return 0;

	nSeekPos = Pos + Offset;

	// only seek if we are not already sitting where we want to be
	if (m_nLastSeekPos != nSeekPos)
	{
		nResult = fseek(m_pFile, nSeekPos, SEEK_SET);
		while (nResult != 0)
		{
			if (!m_pRezMgr->DiskError())
			{
				m_nLastSeekPos = 0xffffffff;
				return 0;
			}
			nResult = fseek(m_pFile, nSeekPos, SEEK_SET);
		}
	}

	nBytesRead = fread(pData, 1, Size, m_pFile);
	while (nBytesRead != Size)
	{
		if (!m_pRezMgr->DiskError())
		{
			m_nLastSeekPos = 0xffffffff;
			return 0;
		}
		nBytesRead = fread(pData, 1, Size, m_pFile);
	}

	m_nLastSeekPos = nSeekPos + nBytesRead;
	return nBytesRead;
}


// ======================================================================= //
//  0x00490370  CRezFile::Write                              ROW, 151 bytes
//
//  Calls, in order (6):
//      0x00490396  0x00492e74  fseek
//      0x004903a7  *0x8(m_pRezMgr's vptr)  CRezMgr::DiskError
//      0x004903b5  0x00492e74  fseek
//      0x004903cd  0x00493cc4  fwrite
//      0x004903de  *0x8(m_pRezMgr's vptr)  CRezMgr::DiskError
//      0x004903ed  0x00493cc4  fwrite
//
//  Read's twin, with three differences the bytes insist on:
//
//   1. `movl $0xffffffff,0x20(%esi)` at 0x0049037b is UNCONDITIONAL and
//      sits before the size guard's branch, so the statement is the first
//      one in the function -- the zero-size return at 0x004903ff does not
//      re-store it.
//   2. There is no `if (m_nLastSeekPos != ...)` guard: a write always
//      seeks, which follows from having just invalidated the cache.
//   3. Nothing is stored back to m_nLastSeekPos at the end, and the success
//      path returns the fwrite count straight out of %eax.
//
//  Here the guard is spelled `jbe` and falls INTO the body, where Read's is
//  spelled `ja` and falls into the return; that is block placement, not a
//  different test -- both are unsigned, and Write's zero return got tail
//  merged with the two DiskError failures at 0x004903ff.
// ======================================================================= //

DWORD_Rf90 CRezFileRf90::Write(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
                               DWORD_Rf90 Size, void* pData)
{
	DWORD_Rf90 nSeekPos;
	DWORD_Rf90 nBytesWritten;
	int        nResult;

	m_nLastSeekPos = 0xffffffff;

	if (Size <= 0) return 0;

	nSeekPos = Pos + Offset;

	nResult = fseek(m_pFile, nSeekPos, SEEK_SET);
	while (nResult != 0)
	{
		if (!m_pRezMgr->DiskError()) return 0;
		nResult = fseek(m_pFile, nSeekPos, SEEK_SET);
	}

	nBytesWritten = fwrite(pData, 1, Size, m_pFile);
	while (nBytesWritten != Size)
	{
		if (!m_pRezMgr->DiskError()) return 0;
		nBytesWritten = fwrite(pData, 1, Size, m_pFile);
	}

	return nBytesWritten;
}


// ======================================================================= //
//  0x00490410  CRezFile::Open                              ROW, 193 bytes
//
//  Calls, in order (4):
//      0x00490442  0x00492b41  fopen
//      0x00490456  *0x8(m_pRezMgr's vptr)  CRezMgr::DiskError
//      0x0049047a  0x004290c0  operator delete
//      0x0049048e  0x004290b0  operator new
//  plus two inline `repnz scasb` string walks and one `rep movsl/movsb`,
//  which are /Oi's strlen and strcpy and name no symbol.
//
//  This is the function that owns the three fopen modes the whole library
//  is recognised by: "w+b" 0x004af7b0 (0x0049042a), "rb" 0x004a7638
//  (0x00490435) and "r+b" 0x004af7ac (0x0049043c). CRezMgr::Open
//  (rezmgr2.cpp, 0x0048ef80) picks the flags; this picks the mode.
//
//  Five shapes the bytes decide.
//
//   1. The retry `jmp 0x00490422` at 0x00490464 lands ABOVE the mode
//      selection, so the whole open -- flag tests and all -- is inside the
//      loop. The loop is entered without a leading test, and its bottom
//      test at 0x0049045d re-reads m_pFile, so it is a do/while on
//      `m_pFile == NULL`.
//   2. The three modes are pushed as immediates in three separate arms that
//      converge on one `push %esi; call fopen` at 0x00490441. A `const
//      char* sMode` variable would have been materialised into a register
//      and pushed once; three separate fopen calls cross-jumped is what
//      leaves the immediates in the arms.
//   3. `test %ebp,%ebp; je` on bCreateNew and `test %edi,%edi; jne` on
//      bReadOnly: the read-only-and-create-new combination is the one the
//      LITE build refuses, and it is the branch that jumps AWAY.
//   4. `mov %edi,0x18(%ebx)` at 0x00490472 is scheduled between the load of
//      m_sFileName and its null test, so m_bReadOnly is assigned before the
//      name is replaced.
//   5. `repnz scasb; not %ecx` with no following `dec` gives strlen+1
//      exactly, so the allocation size is written `strlen(sFileName) + 1`;
//      a plain strlen would have carried the dec.
// ======================================================================= //

BOOL_Rf90 CRezFileRf90::Open(const char* sFileName, BOOL_Rf90 bReadOnly,
                             BOOL_Rf90 bCreateNew)
{
	do
	{
		if (bCreateNew)
		{
			// a new file can not be opened read only
			if (bReadOnly) return FALSE_Rf90;
			m_pFile = fopen(sFileName, "w+b");
		}
		else if (bReadOnly)
		{
			m_pFile = fopen(sFileName, "rb");
		}
		else
		{
			m_pFile = fopen(sFileName, "r+b");
		}

		if (m_pFile == NULL_Rf90)
		{
			if (!m_pRezMgr->DiskError()) return FALSE_Rf90;
		}
	}
	while (m_pFile == NULL_Rf90);

	m_bReadOnly = bReadOnly;

	// keep our own copy of the name so we can re-open the file later
	if (m_sFileName != NULL_Rf90) delete m_sFileName;
	m_sFileName = (char*)::operator new(strlen(sFileName) + 1);
	if (m_sFileName != NULL_Rf90) strcpy(m_sFileName, sFileName);

	m_nLastSeekPos = 0xffffffff;
	return TRUE_Rf90;
}


// ======================================================================= //
//  0x004904e0  CRezFile::Close                              ROW, 86 bytes
//
//  Calls, in order (3):
//      0x004904ee  0x00492a93  fclose
//      0x004904ff  *0x8(m_pRezMgr's vptr)  CRezMgr::DiskError
//      0x00490519  0x004290c0  operator delete
//
//  server.dll 0x1003cb00 (rezmgr.lib is prebuilt: same bytes in both images).
//
//  ONE fclose, at 0x004904ea, and the loop top RE-READS m_pFile (`mov
//  0x10(%esi),%eax`) even on the first pass, straight after the null test
//  loaded it; DiskError's result is tested and `jne` goes straight back to
//  the fclose, with no second copy of anything.  That is a pre-tested loop
//  on a retry FLAG:
//
//      BOOL bRetry = TRUE;
//      while (bRetry) { if (fclose(..) == 0) {..; return TRUE;}
//                       bRetry = DiskError(); }
//
//  VC6 rotates it and folds the constant first test away, which leaves the
//  loop head a join (hence the reload) and the bottom test just DiskError's
//  result.  Every form that tests the fclose result or DiskError directly in
//  a loop condition -- `do {..} while (DiskError())`, `while (fclose(..))`,
//  `for (;;)` with break/return, a goto -- is rotated the other way instead:
//  DiskError heads the loop and a second fclose is peeled above it, 99 bytes
//  against 86 (all measured).  `while (m_pFile != NULL)` gets the head right
//  but re-tests m_pFile at the bottom (93 bytes).
//
//  `mov 0x14(%esi),%eax` then `movl $0x0,0x10(%esi)` then the null test:
//  m_sFileName is read BEFORE m_pFile is cleared, i.e. into a local.  Written
//  with `if (m_sFileName) delete m_sFileName;` in either order around the
//  store, VC6 emits the store first.
// ======================================================================= //

BOOL_Rf90 CRezFileRf90::Close()
{
	if (m_pFile == NULL_Rf90) return FALSE_Rf90;

	BOOL_Rf90 bRetry = TRUE_Rf90;
	while (bRetry)
	{
		if (fclose(m_pFile) == 0)
		{
			char* sFileName = m_sFileName;
			m_pFile = NULL_Rf90;
			if (sFileName != NULL_Rf90) delete sFileName;
			m_sFileName = NULL_Rf90;
			m_nLastSeekPos = 0xffffffff;
			return TRUE_Rf90;
		}
		bRetry = m_pRezMgr->DiskError();
	}

	return FALSE_Rf90;
}


// ======================================================================= //
//  0x00490540  CRezFile::Flush                                     56 bytes
//  server.dll 0x1003cb60.
//
//  Calls, in order (2):
//      0x00490555  0x00494135  fflush
//      0x00490566  *0x8(m_pRezMgr's vptr)  CRezMgr::DiskError
//
//  Close's retry-flag loop exactly, one statement shorter: one fflush with
//  the top re-reading m_pFile, `jne` from DiskError's result straight back
//  to it.  `movl $0xffffffff,0x20(%esi)` sits before the guard's branch, so
//  the invalidation is the first statement.
//
//  NOT ONE OF THE NINE (the enumeration skips from 0x004904e0 to 0x00490580);
//  included because it is CRezFile's vtable slot 6 and the class would be
//  abstract without it.
// ======================================================================= //

BOOL_Rf90 CRezFileRf90::Flush()
{
	m_nLastSeekPos = 0xffffffff;

	if (m_pFile == NULL_Rf90) return FALSE_Rf90;

	BOOL_Rf90 bRetry = TRUE_Rf90;
	while (bRetry)
	{
		if (fflush(m_pFile) == 0) return TRUE_Rf90;
		bRetry = m_pRezMgr->DiskError();
	}

	return FALSE_Rf90;
}


// ======================================================================= //
//  0x00490580  CRezFile::VerifyFileOpen                    ROW, 65 bytes
//
//  Calls, in order (2):
//      0x00490594  0x00492cf1  ftell
//      0x004905b6  *0x10(this's own vptr)  CBaseRezFile::Open
//
//  rezmgr.h: "Checks if the files is actually open and try's to open it
//  again if it is not." ftell on a dead handle returns -1, and that is the
//  only thing this asks.
//
//  Three shapes the bytes decide.
//
//   1. The m_pFile==NULL exit at 0x00490591 returns whatever is in %eax --
//      which is the m_pFile it just loaded, i.e. zero. Written as a plain
//      `return FALSE`; the compiler reused the register it already knew.
//   2. `cmp $0xffffffff,%eax` with the ftell result on the left.
//   3. `neg; sbb; neg` on the re-open result normalises it to 0 or 1, which
//      a bare `return Open(...)` would not have needed: the source compares
//      it. The re-open goes through the vtable (`call *0x10(%eax)`) even
//      though this is CRezFile, so it is spelled as an unqualified virtual
//      call, and its arguments come straight back out of the members the
//      first Open stored: m_sFileName (+0x14) and m_bReadOnly (+0x18).
// ======================================================================= //

BOOL_Rf90 CRezFileRf90::VerifyFileOpen()
{
	m_nLastSeekPos = 0xffffffff;

	if (m_pFile == NULL_Rf90) return FALSE_Rf90;

	// if we can still ask the file where it is, it is still there
	if (ftell(m_pFile) != -1) return TRUE_Rf90;

	return (Open(m_sFileName, m_bReadOnly, FALSE_Rf90) != FALSE_Rf90);
}


// ======================================================================= //
//  0x004905d0  CRezFile::GetFileName                                4 bytes
//
//  `mov 0x14(%ecx),%eax; ret`. Out of line, so it is written in the .cpp
//  and not in the class body.
//
//  NOT A ROW.
// ======================================================================= //

const char* CRezFileRf90::GetFileName()
{
	return m_sFileName;
}


// ======================================================================= //
//  0x004905e0  CRezFileDirectoryEmulation::CRezFileDirectoryEmulation
//                                                          ROW, 70 bytes
//
//  Calls (1): 0x004905e8 0x004901b0 CBaseRezFile::CBaseRezFile.
//
//  No EH frame: both list members construct to three inline stores that
//  cannot throw, so nothing needs unwinding.
//
//  The eleven stores come out in the order +0x14, +0x18, +0x10, +0x1c,
//  +0x20, +0x24, +0x28, +0x34, +0x00, +0x2c, +0x30 -- which is the
//  scheduler grouping by the register that holds the value (%eax holds the
//  zero for +0x14/+0x18/+0x20/+0x24/+0x28/+0x34, %ecx holds the list vtable
//  for +0x10/+0x1c and then the argument for +0x2c), not a source order.
//  What IS source order is that the two list members come before the four
//  scalars, and the destructor's unwind order confirms the two lists are
//  declared +0x10 first.
//
//  `movl $0x1,0x30(%esi)` and the zero at +0x34 are the same defaults
//  rezmgr.h gives CRezMgr::Open -- ReadOnly = TRUE, CreateNew = FALSE.
// ======================================================================= //

CRezFileDirectoryEmulationRf90::CRezFileDirectoryEmulationRf90(
		CRezMgrRf90* pRezMgr, int nMaxOpenFiles)
	: CBaseRezFileRf90(pRezMgr)
{
	m_nNumOpenFiles = 0;
	m_nMaxOpenFiles = nMaxOpenFiles;
	m_bReadOnly = TRUE_Rf90;
	m_bCreateNew = FALSE_Rf90;
}


// ======================================================================= //
//  0x00490650  CRezFileDirectoryEmulation::~CRezFileDirectoryEmulation
//                                   ROW, 119 bytes (enumeration said 135)
//
//  TRUE EXTENT: 0x00490650..0x004906c6. The `ret` at 0x004906c6 is followed
//  by nine lea-nops and then a separate 7-byte function at 0x004906d0 that
//  nothing in this range branches to.
//
//  Calls, in order (3):
//      0x00490686  *0x4(the item's vptr), argument 1 -- the scalar deleting
//                  destructor, i.e. `delete` on a CBaseRezFile*
//      0x0049069b  *0x4(the item's vptr), argument 1 -- the same, on the
//                  other list
//      0x004906b2  0x004901f0  CBaseRezFile::~CBaseRezFile
//  and two inlined ~CVirtBaseList (the 0x004a3b8c stores at 0x004906ac and
//  0x004906af), whose out-of-line body is the 0x004906d0 function.
//
//  /GX frame: handler 0x004a09ae, EH state 2 at 0x00490676 -- base plus two
//  destructible members, which is what makes the two lists members.
//
//  Both loops emit ONE copy of their body with the back edge returning to
//  it (0x0049068e -> 0x00490682, 0x004906a3 -> 0x00490697): plain while
//  loops, the same shape as CRezFile::Close and not Read's duplicated form.
//  The +0x10 list is emptied first and the +0x1c list second; the member
//  vptr restores at the end run +0x1c then +0x10, which is the reverse
//  destruction order that fixes the declaration order.
// ======================================================================= //

CRezFileDirectoryEmulationRf90::~CRezFileDirectoryEmulationRf90()
{
	while (m_lstOpenFiles.GetFirst() != NULL_Rf90)
		delete m_lstOpenFiles.GetFirst();

	while (m_lstRezFiles.GetFirst() != NULL_Rf90)
		delete m_lstRezFiles.GetFirst();
}


// ======================================================================= //
//  0x004906e0  CRezFileDirectoryEmulation::Read  and  ::Write      6 bytes
//
//  `xor %eax,%eax; ret $0x10`. Slots 2 and 3 of 0x004a3bd8 hold the same
//  address, so /OPT:ICF folded two identical bodies; the `ret $0x10`
//  independently confirms the four-argument Read/Write signature. A
//  directory emulation has no block of its own to read -- the per-file
//  objects do that.
//
//  NOT A ROW.
// ======================================================================= //

DWORD_Rf90 CRezFileDirectoryEmulationRf90::Read(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
                                                DWORD_Rf90 Size, void* pData)
{
	return 0;
}

DWORD_Rf90 CRezFileDirectoryEmulationRf90::Write(DWORD_Rf90 Pos, DWORD_Rf90 Offset,
                                                 DWORD_Rf90 Size, void* pData)
{
	return 0;
}


// ======================================================================= //
//  0x004906f0  CRezFileDirectoryEmulation::Open                    20 bytes
//
//  No calls. Remembers the two flags and says yes; the file name argument
//  is never touched, which is why CRezMgr keeps its own copy.
//
//  NOT A ROW.
// ======================================================================= //

BOOL_Rf90 CRezFileDirectoryEmulationRf90::Open(const char* sFileName,
                                               BOOL_Rf90 bReadOnly,
                                               BOOL_Rf90 bCreateNew)
{
	m_bReadOnly = bReadOnly;
	m_bCreateNew = bCreateNew;
	return TRUE_Rf90;
}


// ======================================================================= //
//  0x00490710  CRezFileDirectoryEmulation::Close                   29 bytes
//
//  Calls (1): 0x0049071a 0x00490ac0, the per-file CloseFile.
//
//  THE CALL IS DIRECT (`e8 rel32`), not `mov (%ecx),%eax ; call *0x14(%eax)`
//  -- both are five bytes, so the size never showed it.  0x00490ac0 is NOT
//  slot 5 of the per-file vtable 0x004a3c00: that slot is the shared
//  `xor %eax,%eax ; ret` at 0x0048fab0.  So what the emulation drains its
//  open-file list with is the class's own non-virtual CloseFile, which is
//  also what the LRU eviction inside OpenFile calls (0x00490a34) and what
//  the destructor calls (0x00490832).
//
//  NOT A ROW.
// ======================================================================= //

BOOL_Rf90 CRezFileDirectoryEmulationRf90::Close()
{
	while (m_lstOpenFiles.GetFirst() != NULL_Rf90)
		((CRezFileDirectoryEmulationFileRf90*)m_lstOpenFiles.GetFirst())->CloseFile();

	return TRUE_Rf90;
}


// ======================================================================= //
//  0x00490730  CRezFileDirectoryEmulation::Flush and ::VerifyFileOpen
//                                                                  6 bytes
//
//  `mov $0x1,%eax; ret`, folded into one body for slots 6 and 7. Nothing to
//  flush and nothing to verify.
//
//  NOT A ROW.
// ======================================================================= //

BOOL_Rf90 CRezFileDirectoryEmulationRf90::Flush()
{
	return TRUE_Rf90;
}

BOOL_Rf90 CRezFileDirectoryEmulationRf90::VerifyFileOpen()
{
	return TRUE_Rf90;
}


// ======================================================================= //
//  0x0048fab0 (folded)  CRezFileDirectoryEmulation::GetFileName
//
//  Slot 8 of 0x004a3bd8 points at 0x0048fab0, which is also CRezMgr's
//  DiskError and slots 5 and 7 of 0x004a3c00: `xor %eax,%eax; ret`. It has
//  no address of its own in this module.
//
//  NOT A ROW.
// ======================================================================= //

const char* CRezFileDirectoryEmulationRf90::GetFileName()
{
	return NULL_Rf90;
}


// ======================================================================= //
//  0x00490740  CRezFileDirectoryEmulationFile::CRezFileDirectoryEmulationFile
//                                                         ROW, 155 bytes
//
//  Calls, in order (3):
//      0x00490764  0x004901b0  CBaseRezFile::CBaseRezFile
//      0x0049078d  0x004290b0  operator new
//      0x004907c0  0x004a05b0  CVirtBaseList::InsertFirst
//  plus two inline `repnz scasb` and one `rep movsl/movsb` -- /Oi's strlen
//  and strcpy again.
//
//  /GX frame: handler 0x004a09c8, EH state 0 written at 0x00490789 (from
//  the %eax the scasb setup had already zeroed), so the state is "base
//  constructed" and the funclet frees the base if operator new throws.
//
//  Three shapes the bytes decide.
//
//   1. The stores run +0x18, vptr, +0x14, then +0x10 after the allocation
//      -- so m_pDirEmulation is assigned before m_pFile, and neither is an
//      initialiser-list entry (those would follow declaration order, which
//      the vtable's GetFileName at 0x00490b40 fixes as m_sFileName first).
//   2. There is NO null test between operator new and the copy here, where
//      CRezFile::Open has one. The two sites are written differently.
//   3. `mov 0x18(%ebx),%ecx; add $0x1c,%ecx` re-reads the member rather
//      than reusing the argument, and 0x1c is the offset of the emulation's
//      second list -- the one that holds files that are not currently open.
//      A newly built file object has no handle yet, so that is where it
//      belongs.
// ======================================================================= //

CRezFileDirectoryEmulationFileRf90::CRezFileDirectoryEmulationFileRf90(
		CRezMgrRf90* pRezMgr, const char* sFileName,
		CRezFileDirectoryEmulationRf90* pDirEmulation)
	: CBaseRezFileRf90(pRezMgr)
{
	m_pDirEmulation = pDirEmulation;
	m_pFile = NULL_Rf90;

	m_sFileName = (char*)::operator new(strlen(sFileName) + 1);
	strcpy(m_sFileName, sFileName);

	m_pDirEmulation->m_lstRezFiles.InsertFirst(this);
}
