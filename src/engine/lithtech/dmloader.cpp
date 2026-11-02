// *********************************************************************** //
//
//  MODULE:   CLIENTMUSICLOADER (.CPP)
//
//  PURPOSE:  The engine's own IDirectMusicLoader, and the two IStream
//            implementations it hands DirectMusic to read with.
//
//  This is the other half of the DirectMusic module whose manager lives in
//  clientmusic.cpp. CLTDirectMusicMgr's Init does not CoCreateInstance a
//  loader: at 0x00440f25 it allocates 0x38 bytes, runs the constructor at
//  0x00443900 on them and stores the result in m_pLoader (0x010) - the same
//  member clientmusic.h already names. So every SetSearchDirectory and
//  GetObject the manager makes lands in this file, and DirectMusic reads its
//  segments and DLS banks through the streams below rather than through the
//  file system.
//
//  WHAT NAMES THE VTABLES
//
//  Three vtables sit in .rdata, all three found by the `movl $addr,(%esi)`
//  in the constructor that installs them:
//
//    0x004a2608  0x00443900 / 0x004439b8   twelve slots
//    0x004a2578  0x00443231 / 0x00443276   fourteen slots, +0x00 of a stream
//    0x004a2568                            four slots,     +0x04 of a stream
//    0x004a25d0  0x00443608 / 0x00443646   fourteen slots, +0x00 of a stream
//    0x004a25c0                            four slots,     +0x04 of a stream
//    0x004a25b0                            four purecalls (0x004928e7)
//
//  The fourteen-slot ones are IStream. Every slot's `ret N` matches the
//  argument bytes ole2 declares, in ole2's order, and the match is exact
//  across all fourteen - QueryInterface 0xc, AddRef/Release 4, Read/Write
//  0x10, Seek 0x14, SetSize 0xc, CopyTo 0x18, Commit 8, Revert 4,
//  LockRegion 0x18, UnlockRegion 0x18, Stat 0xc, Clone 8. The GUIDs the
//  shared QueryInterface (0x004436e0) compares settle it: 0x004a3b50 is
//  {00000000-0000-0000-C000-000000000046}, IID_IUnknown, and 0x004a3b40 is
//  {0000000C-...-46}, IID_IStream.
//
//  The four-slot ones are IDirectMusicGetLoader: three IUnknown slots and
//  one GetLoader whose out-pointer makes it `ret 8`. Its IID is the third
//  GUID that same QueryInterface compares, 0x004a397c =
//  {68A04844-D13D-11D1-AFA6-00AA0024D8B6}. When it matches, the code stores
//  `this + 4` into the out-pointer - the second base's address - which is
//  what puts IStream first and IDirectMusicGetLoader second.
//
//  The twelve-slot one is IDirectMusicLoader, again by `ret N` in
//  declaration order: GetObject 0x10, SetObject 8, SetSearchDirectory 0x10,
//  ScanDirectory 0x10, CacheObject 8, ReleaseObject 8, ClearCache 8,
//  EnableCache 0xc, EnumObject 0x10. Two independent facts confirm the
//  anchoring rather than just fitting it: the loader's own Release
//  (0x00443af0) calls its slot 0x24 with 0x004a39cc =
//  {D2AC2893-B39B-11D1-8704-00600893B1BD}, GUID_DirectMusicAllTypes, and
//  slot 0x24 is ClearCache; and dm_SetSearchDirectory (0x00443ff0, declared
//  in clientmusic.h) reaches a loader's slot 0x14 with a widened path, a
//  class GUID and a flag, which is SetSearchDirectory.
//
//  All of these are __stdcall members, so `this` is the first stack
//  argument and every body reads it from 0x4(%esp) rather than %ecx.
//
// *********************************************************************** //

#include <string.h>

#include "ltbasedefs.h"


// --------------------------------------------------------------------- //
// The pieces of OLE and DirectMusic this file implements. Neither
// ole2.h/objidl.h nor dmusic.h is in this toolchain, so both interfaces are
// declared here with the slots and argument bytes the shipped vtables prove.
// Relocations are masked, so a GUID that is declared and never defined still
// matches: only the fact that an address is pushed has to be right.
// --------------------------------------------------------------------- //

typedef long DMHRESULT;

#define DM_S_OK             0x00000000
#define DM_E_NOTIMPL        0x80004001L
#define DM_E_NOINTERFACE    0x80004002L
#define DM_E_FAIL           0x80004005L

// STREAM_SEEK_SET/CUR/END: Seek's three-way dispatch on dwOrigin tests 1 and
// 2 and treats everything else as the absolute case, which is 0.
#define DM_STREAM_SEEK_SET  0
#define DM_STREAM_SEEK_CUR  1
#define DM_STREAM_SEEK_END  2

struct DMGUID { uint32 m_Data[4]; };

// 24 bytes, the size the loader reserves between 0x0c and 0x24.
struct DMCRITICAL_SECTION { uint8 m_Data[24]; };

struct DMLARGE_INTEGER  { uint32 LowPart; int32 HighPart; };
struct DMULARGE_INTEGER { uint32 LowPart; int32 HighPart; };

extern "C" const DMGUID IID_IUnknown;              // 0x004a3b50 (uuid.lib)
extern "C" const DMGUID IID_IStream;               // 0x004a3b40 (uuid.lib)
extern const DMGUID IID_IDirectMusicGetLoader;     // 0x004a397c
extern const DMGUID IID_IDirectMusicLoader8;       // 0x004a398c
extern const DMGUID GUID_DirectMusicAllTypes;      // 0x004a39cc
extern "C" const DMGUID GUID_NULL;                 // 0x004a3b30 (sixteen zeroes; uuid.lib, as dmextra.cpp)

struct IDirectMusicObject;
struct DMUS_OBJECTDESC;
struct DMSTATSTG;

struct IUnknown
{
	virtual DMHRESULT __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;
};

// dmusici.h's object descriptor. 0x00443b90 clears 0xd4 dwords of one on its
// frame and stores 0x350 as its size, and reads dwValidData at 0x04 and the
// object GUID at 0x08.
struct DMUS_OBJECTDESC
{
	uint32 dwSize;                           // 0x000
	uint32 dwValidData;                      // 0x004
	DMGUID guidObject;                       // 0x008
	DMGUID guidClass;                        // 0x018
	uint32 ftDate[2];                        // 0x028
	uint32 vVersion[2];                      // 0x030
	uint16 wszName[64];                      // 0x038
	uint16 wszCategory[64];                  // 0x0b8
	uint16 wszFileName[260];                 // 0x138
	uint32 llMemLength[2];                   // 0x340
	uint8* pbMemData;                        // 0x348
	void*  pStream;                          // 0x34c
};

#define DM_DMUS_OBJ_OBJECT   0x0001
#define DM_DMUS_OBJ_FILENAME 0x0010
#define DM_DMUS_OBJ_MEMORY   0x0400

// DMUS_E_LOADER_NOFILENAME: GetObject's answer to a descriptor that names
// neither a file nor a block of memory.
#define DM_DMUS_E_LOADER_NOFILENAME 0x88781186

// The one slot past IUnknown the loader calls: GetDescriptor, at 0x0c.
struct IDirectMusicObject : public IUnknown
{
	virtual DMHRESULT __stdcall GetDescriptor(DMUS_OBJECTDESC* pDesc)=0;
};

struct IStream : public IUnknown
{
	virtual DMHRESULT __stdcall Read(void* pv, uint32 cb, uint32* pcbRead)=0;
	virtual DMHRESULT __stdcall Write(const void* pv, uint32 cb, uint32* pcbWritten)=0;
	virtual DMHRESULT __stdcall Seek(DMLARGE_INTEGER dlibMove, uint32 dwOrigin,
	                                 DMULARGE_INTEGER* plibNewPosition)=0;
	virtual DMHRESULT __stdcall SetSize(DMULARGE_INTEGER libNewSize)=0;
	virtual DMHRESULT __stdcall CopyTo(IStream* pstm, DMULARGE_INTEGER cb,
	                                   DMULARGE_INTEGER* pcbRead,
	                                   DMULARGE_INTEGER* pcbWritten)=0;
	virtual DMHRESULT __stdcall Commit(uint32 grfCommitFlags)=0;
	virtual DMHRESULT __stdcall Revert()=0;
	virtual DMHRESULT __stdcall LockRegion(DMULARGE_INTEGER libOffset,
	                                       DMULARGE_INTEGER cb, uint32 dwLockType)=0;
	virtual DMHRESULT __stdcall UnlockRegion(DMULARGE_INTEGER libOffset,
	                                         DMULARGE_INTEGER cb, uint32 dwLockType)=0;
	virtual DMHRESULT __stdcall Stat(DMSTATSTG* pstatstg, uint32 grfStatFlag)=0;
	virtual DMHRESULT __stdcall Clone(IStream** ppstm)=0;
};

class CDMLoader;

// The interface DirectMusic asks a stream for when it wants to chase a
// reference out of the file it is reading: hand back the loader that opened
// the stream. Its vtable is 0x004a25b0 when nothing has overridden it - four
// purecalls at 0x004928e7 - and both stream constructors store that address
// at 0x04 before storing their own, which is how an abstract second base
// shows up in the bytes.
struct IDirectMusicGetLoader : public IUnknown
{
	virtual DMHRESULT __stdcall GetLoader(CDMLoader** ppLoader)=0;
};

struct IDirectMusicLoader : public IUnknown
{
	virtual DMHRESULT __stdcall GetObject(DMUS_OBJECTDESC* pDesc, const DMGUID& riid,
	                                      void** ppv)=0;
	virtual DMHRESULT __stdcall SetObject(DMUS_OBJECTDESC* pDesc)=0;
	virtual DMHRESULT __stdcall SetSearchDirectory(const DMGUID& rguidClass,
	                                               uint16* pwzPath, int fClear)=0;
	virtual DMHRESULT __stdcall ScanDirectory(const DMGUID& rguidClass,
	                                          uint16* pwzFileExtension,
	                                          uint16* pwzScanFileName)=0;
	virtual DMHRESULT __stdcall CacheObject(IDirectMusicObject* pObject)=0;
	virtual DMHRESULT __stdcall ReleaseObject(IDirectMusicObject* pObject)=0;
	virtual DMHRESULT __stdcall ClearCache(const DMGUID& rguidClass)=0;
	virtual DMHRESULT __stdcall EnableCache(const DMGUID& rguidClass, int fEnable)=0;
	virtual DMHRESULT __stdcall EnumObject(const DMGUID& rguidClass, uint32 dwIndex,
	                                       DMUS_OBJECTDESC* pDesc)=0;
};


// --------------------------------------------------------------------- //
// Views on what this file reaches outside itself.
// --------------------------------------------------------------------- //

// The engine's cached-open-file record, from the manager at 0x004b3fe8. Its
// home is the locked list in src/engine/lithtech/clientworld.cpp ("the locked
// list at 0x42d210..0x42d760", CLockedItemView / CLockedOwnerView there, and
// demodtx.cpp's CLockedItemQ42C for the corrected two-list owner). The two
// bodies this file's streams depend on, 0x0042d330 and 0x0042d610, are
// written below with a MOVE-ME to that section; the layout is the one
// demodtx.cpp reads off Load/Discard: links at 0x00/0x04, the open stream at
// 0x08, the owner at 0x0c, a use count at 0x10 and a file ref at 0x14 whose
// name pointer is 0x18. 0x0042d330 raises the use count and opens the file
// when it was not open, handing back its ILTStream; 0x0042d3b0 drops it
// again; 0x0042d2d0 refuses while the count is above zero and otherwise
// closes (names ours).
class ILTStreamView;
class CDMFileMgr;
class CDMFileRecord
{
public:
	LTBOOL SetFilename(const char* pFilename);                      // 0x0042d210
	ILTStreamView* Open();                                          // 0x0042d330
	void  Close();                                                  // 0x0042d3b0
	// 0x0042d2d0 is NOT this class's own: src/engine/lithtech/clientworld.cpp
	// defines it as CLockedItemView::TryDiscard and reproduces it, reading a
	// reference count under a critical section with three release sites.  So
	// a DirectMusic file record IS a locked-list item, and the answer is an
	// LTBOOL, not an int.  The two files were written independently and both annotated this address.
	LTBOOL TryDiscard();                                            // 0x0042d2d0
	// 0x0042d3f0, demodtx.cpp's CLockedItemQ42C::Load: nonzero is FAILURE.
	LTBOOL Load();                                                  // 0x0042d3f0

public:
	CDMFileRecord* m_pNext;                  // 0x000
	CDMFileRecord* m_pPrev;                  // 0x004
	ILTStreamView* m_pStream;                // 0x008
	CDMFileMgr*    m_pOwner;                 // 0x00c
	uint32         m_nRefs;                  // 0x010
	uint32         m_nFileType;              // 0x014
	char*          m_pFilename;              // 0x018
};

// src/LT2/sdk/inc/iltstream.h's declaration order is the vtable order, and the
// Read at slot 0x08 is the one 0x00443478 calls with (pv, cb) - two
// arguments, an LTRESULT back, nonzero meaning failure. Only that slot is
// needed here, so the three before it are placeholders that keep the offset.
class ILTStreamView
{
public:
	virtual void _p00()=0;                                          // ~ILTStream
	virtual void Release()=0;                                       // 0x04
	virtual LTRESULT Read(void* pData, unsigned long size)=0;       // 0x08
	// 0x004434c0 (Seek) reaches the next five: SeekTo with one argument,
	// GetPos / GetLen with an out-pointer each.
	virtual LTRESULT ReadString(char* pStr, unsigned long maxBytes)=0; // 0x0c
	virtual LTRESULT ErrorStatus()=0;                               // 0x10
	virtual LTRESULT SeekTo(unsigned long offset)=0;                // 0x14
	virtual LTRESULT GetPos(unsigned long* pOffset)=0;              // 0x18
	virtual LTRESULT GetLen(unsigned long* pLen)=0;                 // 0x1c
};

// One of the manager's two record lists: {head, tail, count}. Insert/Delete
// are the shared list helpers at 0x0048de30 / 0x0048dee0; Add/Remove are
// in-class so /Ob1 expands the count update beside the call, as in
// demodtx.cpp's CLockedListQ42C.
class CDMFileRecordList
{
public:
	void Insert(CDMFileRecord* pRecord);                            // 0x0048de30
	void Delete(CDMFileRecord* pRecord);                            // 0x0048dee0

	void Add(CDMFileRecord* pRecord)    { m_nCount++; Insert(pRecord); }
	void Remove(CDMFileRecord* pRecord) { m_nCount--; Delete(pRecord); }

public:
	CDMFileRecord* m_pHead;                  // 0x000
	CDMFileRecord* m_pTail;                  // 0x004
	uint32         m_nCount;                 // 0x008
};

// The manager the records come from, a single global: a critical section at
// its front (0x0042d330 hands the owner pointer itself to
// EnterCriticalSection), the loaded list at 0x18 and the unloaded one at
// 0x24. 0x0042d610 hands a record back (name ours).
class CDMFileMgr
{
public:
	void FreeRecord(CDMFileRecord* pRecord);                        // 0x0042d610
	// 0x0042d520, clientworld.cpp's CLockedOwnerView::Init: the lock and the
	// open-file limit.
	void Init(uint32 nLimit);                                       // 0x0042d520
	// 0x0042d540: the teardown ~CDMLoader calls last before its own lock
	// goes (name ours).
	void Term();

public:
	DMCRITICAL_SECTION m_CriticalSection;    // 0x000
	CDMFileRecordList  m_Loaded;             // 0x018
	CDMFileRecordList  m_Unloaded;           // 0x024
	uint32             m_nLimit;             // 0x030
};

extern CDMFileMgr g_DMFileMgr;                     // 0x004b3fe8

// The doubly-linked list helpers the whole DirectMusic module shares, already
// named in clientmusic.h. Every node type puts its links at 0x00 and 0x04, so
// one pair serves all of them.
extern "C" {
__declspec(dllimport) void  __stdcall InitializeCriticalSection(DMCRITICAL_SECTION*);
__declspec(dllimport) void  __stdcall DeleteCriticalSection(DMCRITICAL_SECTION*);
__declspec(dllimport) void  __stdcall EnterCriticalSection(DMCRITICAL_SECTION*);
__declspec(dllimport) void  __stdcall LeaveCriticalSection(DMCRITICAL_SECTION*);
__declspec(dllimport) long  __stdcall InterlockedIncrement(long* pAddend);
__declspec(dllimport) long  __stdcall InterlockedDecrement(long* pAddend);
__declspec(dllimport) int   __stdcall GetVersionExA(void* pVersionInfo);
__declspec(dllimport) int   __stdcall IsBadReadPtr(const void* lp, uint32 ucb);
__declspec(dllimport) int   __stdcall MultiByteToWideChar(uint32 CodePage, uint32 dwFlags,
                                                          const char* lpMultiByteStr,
                                                          int cbMultiByte,
                                                          uint16* lpWideCharStr,
                                                          int cchWideChar);
}

#define DM_CP_ACP           0
#define DM_MB_PRECOMPOSED   1
#define DM_MAX_PATH         260

// KERNEL32's OSVERSIONINFOA, sized by the 0x94 the frame reserves and the
// 0x94 it writes into the first word. Only the platform id is read, and it is
// at 0x10 of the structure - 0x10(%esp) once the size has been stored at
// 0x0(%esp).
struct DMOSVERSIONINFOA
{
	uint32 dwOSVersionInfoSize;              // 0x000
	uint32 dwMajorVersion;                   // 0x004
	uint32 dwMinorVersion;                   // 0x008
	uint32 dwBuildNumber;                    // 0x00c
	uint32 dwPlatformId;                     // 0x010
	char   szCSDVersion[0x94 - 0x14];        // 0x014
};

#define DM_VER_PLATFORM_WIN32_WINDOWS 1

// 0x004431e0, defined at the bottom of this file (name ours).
void dm_CheckPlatform();


// --------------------------------------------------------------------- //
// The two lists the loader keeps, each behind its own critical section.
//
// Both are the same shape - head at 0x00, tail at 0x04 and a CRITICAL_SECTION
// at 0x08 - which is what makes the object the 0x20 bytes the constructor
// allocates for each of them at 0x00443957 and 0x00443976. They are separate
// template instantiations rather than one class: the record the search-
// directory list holds carries the links itself, and the stream list wraps
// each stream in a three-word node, so no method is shared between them.
// (Names ours.)
// --------------------------------------------------------------------- //

// One remembered search directory: its own links, the class GUID it is for,
// and the path. 0x00442fe0 allocates 0x220 bytes for one and starts the GUID
// off as GUID_NULL before assigning the caller's, and SetSearchDirectory
// copies at most 0x103 wide characters into 0x18 after clearing 0x82 words -
// 0x208 bytes - which puts MAX_PATH wide characters at 0x18 and nothing
// after them (name ours).
class CDMSearchDir
{
public:
	CDMSearchDir* m_pNext;                   // 0x000
	CDMSearchDir* m_pPrev;                   // 0x004
	DMGUID        m_guidClass;               // 0x008
	uint16        m_wzPath[0x104];           // 0x018
};

class CDMSearchDirList
{
public:
	CDMSearchDir*      m_pHead;              // 0x000
	CDMSearchDir*      m_pTail;              // 0x004
	DMCRITICAL_SECTION m_CriticalSection;    // 0x008

	// In-class so /Ob1 expands it into the loader's constructor, where
	// 0x00443967 clears the two links and initialises the lock inside the
	// `new` null test.
	CDMSearchDirList()
	{
		m_pHead = LTNULL;
		m_pTail = LTNULL;
		InitializeCriticalSection(&m_CriticalSection);
	}

	// In-class too: ~CDMLoader (0x004439f5) expands it inside the delete's
	// null test - empty the list, then the lock.
	~CDMSearchDirList()
	{
		RemoveAll();
		DeleteCriticalSection(&m_CriticalSection);
	}

	// 0x00442f20 / 0x00442fe0 (names ours). Add answers nothing: both exits
	// leave LeaveCriticalSection's eax in place.
	CDMSearchDir* Find(const DMGUID& rguidClass, int bCreate);      // 0x00442f20
	void Add(const DMGUID& rguidClass, uint16* pwzPath);            // 0x00442fe0

	void Remove(CDMSearchDir* pDir);                                // 0x00443090
	void RemoveAll();                                               // 0x004430d0

	void ListAddHead(CDMSearchDir* pDir);                           // 0x0048de30
	void ListRemove(CDMSearchDir* pDir);                            // 0x0048dee0
};

// The wrapper the stream list puts round each stream: links and the stream.
// 0x00443100 allocates twelve bytes for one and writes the stream into 0x08
// before linking it, so those three words are the whole node (name ours).
class CDMFileStream;
class CDMStreamNode
{
public:
	CDMStreamNode* m_pNext;                  // 0x000
	CDMStreamNode* m_pPrev;                  // 0x004
	CDMFileStream* m_pStream;                // 0x008
};

class CDMStreamList
{
public:
	CDMStreamNode*     m_pHead;              // 0x000
	CDMStreamNode*     m_pTail;              // 0x004
	DMCRITICAL_SECTION m_CriticalSection;    // 0x008

	// The same shape as CDMSearchDirList's, expanded at 0x00443989.
	CDMStreamList()
	{
		m_pHead = LTNULL;
		m_pTail = LTNULL;
		InitializeCriticalSection(&m_CriticalSection);
	}

	// Expanded at 0x00443a19, the same shape as ~CDMSearchDirList.
	~CDMStreamList()
	{
		DeleteAll();
		DeleteCriticalSection(&m_CriticalSection);
	}

	void Add(CDMFileStream* pStream);                               // 0x00443100
	// 0x00443140: walk the list for the node whose 0x08 is this stream,
	// unlink it and free the node. The stream destructor calls it (name ours).
	void Remove(CDMFileStream* pStream);                            // 0x00443140
	void DeleteAll();                                               // 0x004431a0

	void ListAddHead(CDMStreamNode* pNode);                         // 0x0048de30
	void ListRemove(CDMStreamNode* pNode);                          // 0x0048dee0
};


// --------------------------------------------------------------------- //
// CDMLoader - the engine's IDirectMusicLoader (name ours).
//
// 0x38 bytes, which is the `push $0x38` at 0x00440f25 and also exactly where
// the constructor's last member ends. Two reference counts: the COM one at
// 0x04, which AddRef raises and Release lowers, and a second at 0x08 that the
// streams hold while they are alive. Release only deletes when both are zero
// and so does the streams' own release, so whichever drops last does the
// delete.
// --------------------------------------------------------------------- //

// One entry of the loader's object cache: 0x18 bytes (`push $0x18` at
// 0x00443c7c), the next link, the object's GUID and the object. The in-class
// constructor clears the link and the object - the two stores the new at
// 0x00443c8a makes before the GUID is copied in (name ours).
class CDMCachedObject
{
public:
	CDMCachedObject() { m_pNext = LTNULL; m_pObject = LTNULL; }

	CDMCachedObject*    m_pNext;             // 0x000
	DMGUID              m_guidObject;        // 0x004
	IDirectMusicObject* m_pObject;           // 0x014
};

class CDMLoader : public IDirectMusicLoader
{
public:
	CDMLoader();                                                    // 0x00443900
	~CDMLoader();                                                   // 0x004439b0

	// 0x00443a50 (name ours): a plain member - it takes nothing, cleans
	// nothing, and answers S_OK - called on the loader at 0x00440f4e as soon
	// as the constructor has returned.
	DMHRESULT Init();

	virtual DMHRESULT __stdcall QueryInterface(const DMGUID& riid, void** ppv); // 0x00443a60
	virtual uint32 __stdcall AddRef();                              // 0x00443ac0
	virtual uint32 __stdcall Release();                             // 0x00443af0

	virtual DMHRESULT __stdcall GetObject(DMUS_OBJECTDESC* pDesc, const DMGUID& riid,
	                                      void** ppv);              // 0x00443b90
	virtual DMHRESULT __stdcall SetObject(DMUS_OBJECTDESC* pDesc);  // 0x00443f60
	virtual DMHRESULT __stdcall SetSearchDirectory(const DMGUID& rguidClass,
	                                               uint16* pwzPath, int fClear);
	                                                                // 0x00443f80
	virtual DMHRESULT __stdcall ScanDirectory(const DMGUID& rguidClass,
	                                          uint16* pwzFileExtension,
	                                          uint16* pwzScanFileName);
	virtual DMHRESULT __stdcall CacheObject(IDirectMusicObject* pObject);
	virtual DMHRESULT __stdcall ReleaseObject(IDirectMusicObject* pObject);
	virtual DMHRESULT __stdcall ClearCache(const DMGUID& rguidClass);
	virtual DMHRESULT __stdcall EnableCache(const DMGUID& rguidClass, int fEnable);
	virtual DMHRESULT __stdcall EnumObject(const DMGUID& rguidClass, uint32 dwIndex,
	                                       DMUS_OBJECTDESC* pDesc);

	// The count the streams hold. Both stream constructors call the first on
	// the loader they were given, and both destructors the second; the
	// argument-free thiscall shape (`add $0x8,%ecx` straight into
	// InterlockedIncrement) is what says these are members and not the COM
	// slots (names ours).
	void   InternalAddRef();                                        // 0x00443ae0

	// The two loads GetObject dispatches to, by the descriptor's
	// DMUS_OBJ_FILENAME / DMUS_OBJ_MEMORY bits: thiscall, `ret 8`, the
	// descriptor and the out-pointer for the object (names ours).
	// Both written in dmextra.cpp, under these names.
	DMHRESULT GetObjectFromFileQ440(DMUS_OBJECTDESC* pDesc, IDirectMusicObject** ppObject);   // 0x00443d20
	DMHRESULT GetObjectFromMemoryQ440(DMUS_OBJECTDESC* pDesc, IDirectMusicObject** ppObject); // 0x00443e90
	uint32 InternalRelease();                                       // 0x00443b50

public:
	// 0x00 is the vtable.
	long   m_nRefCount;                      // 0x004  starts at 1
	long   m_nStreamRefCount;                // 0x008  starts at 0
	DMCRITICAL_SECTION m_CriticalSection;    // 0x00c
	// The objects the loader has cached: a hand-rolled singly-linked list of
	// 0x20-byte records, each with an IUnknown at 0x14 that the destructor
	// releases. Nothing here adds to it, so the record stays unnamed.
	CDMCachedObject* m_pCachedObjects;       // 0x024
	CDMSearchDirList* m_pSearchDirs;         // 0x028
	CDMStreamList*    m_pStreams;            // 0x02c
	// L"/" and L"\", each two wide characters, built by two
	// MultiByteToWideChar calls at 0x00443940 and 0x00443955 out of the one-
	// character strings at 0x004abf70 and 0x004ab728. Both calls pass MAX_PATH
	// as the destination size even though the members are four bytes apart,
	// which is why the second writes into the first's second character
	// onwards; only the first character of each is ever meaningful
	// (names ours).
	uint16 m_wzFwdSlash[2];                  // 0x030
	uint16 m_wzBackSlash[2];                 // 0x034
};


// --------------------------------------------------------------------- //
// CDMFileStream - the IStream DirectMusic reads a file through (name ours).
//
// Constructed at 0x00443220 from a loader, and the only thing it holds beyond
// the COM machinery is one of the engine's cached-open-file records. Read
// opens the record, forwards to the ILTStream that comes back and closes it
// again, so the file is only held open across a single Read.
//
// The two bases are IStream at 0x00 and IDirectMusicGetLoader at 0x04, in
// that order: the constructor stores 0x004a2578 at 0x00 and 0x004a2568 at
// 0x04, and QueryInterface hands out `this + 4` for the loader interface.
// --------------------------------------------------------------------- //

class CDMFileStream : public IStream, public IDirectMusicGetLoader
{
public:
	CDMFileStream(CDMLoader* pLoader);                              // 0x00443220
	~CDMFileStream();                                               // 0x00443270

	// 0x00443380 (name ours): let go of the file record. Answers S_OK
	// unconditionally - `xor %eax,%eax` on both paths - so it is a HRESULT
	// and not void.
	DMHRESULT Close();

	virtual DMHRESULT __stdcall QueryInterface(const DMGUID& riid, void** ppv); // 0x004436e0
	virtual uint32 __stdcall AddRef();                              // 0x004433e0
	virtual uint32 __stdcall Release();                             // 0x00443400

	virtual DMHRESULT __stdcall Read(void* pv, uint32 cb, uint32* pcbRead); // 0x00443440
	virtual DMHRESULT __stdcall Write(const void* pv, uint32 cb, uint32* pcbWritten);
	virtual DMHRESULT __stdcall Seek(DMLARGE_INTEGER dlibMove, uint32 dwOrigin,
	                                 DMULARGE_INTEGER* plibNewPosition); // 0x004434c0
	virtual DMHRESULT __stdcall SetSize(DMULARGE_INTEGER libNewSize);
	virtual DMHRESULT __stdcall CopyTo(IStream* pstm, DMULARGE_INTEGER cb,
	                                   DMULARGE_INTEGER* pcbRead,
	                                   DMULARGE_INTEGER* pcbWritten);
	virtual DMHRESULT __stdcall Commit(uint32 grfCommitFlags);
	virtual DMHRESULT __stdcall Revert();
	virtual DMHRESULT __stdcall LockRegion(DMULARGE_INTEGER libOffset,
	                                       DMULARGE_INTEGER cb, uint32 dwLockType);
	virtual DMHRESULT __stdcall UnlockRegion(DMULARGE_INTEGER libOffset,
	                                         DMULARGE_INTEGER cb, uint32 dwLockType);
	virtual DMHRESULT __stdcall Stat(DMSTATSTG* pstatstg, uint32 grfStatFlag);
	virtual DMHRESULT __stdcall Clone(IStream** ppstm);

	virtual DMHRESULT __stdcall GetLoader(CDMLoader** ppLoader);    // 0x004433b0

public:
	// 0x00 and 0x04 are the two vtables.
	long   m_nRefCount;                      // 0x008  starts at 1
	CDMFileRecord* m_pFile;                  // 0x00c  starts null
	CDMLoader*     m_pLoader;                // 0x010
};


// --------------------------------------------------------------------- //
// CDMMemStream - the IStream over a block of memory (name ours).
//
// Constructed at 0x004435f0 from a loader and pointed at a buffer afterwards
// by 0x00443670, which stores the pointer and a 64-bit length, refuses a null
// buffer or a zero length with 0x88781182 and IsBadReadPtr's the two. The
// buffer is borrowed, not owned: nothing here frees it.
//
// Its 64-bit size and position are what name the fields. Read adds the
// request to the position with `add`/`adc` and compares the result against
// 0x10/0x14 the same way, and Seek's three origins read 0x18/0x1c for
// current and 0x10/0x14 for end.
// --------------------------------------------------------------------- //

class CDMMemStream : public IStream, public IDirectMusicGetLoader
{
public:
	CDMMemStream(CDMLoader* pLoader);                               // 0x004435f0
	~CDMMemStream();                                                // 0x00443640

	// 0x004436d0 (name ours): forget the buffer. Zeroes the length's low
	// word, the pointer and the length's high word, in that order, and leaves
	// the position alone. 0x00443670 calls it before taking a new buffer, and
	// the destructor after letting the loader go.
	void Close();

	// 0x00443670 (name ours). The length arrives as one 64-bit value: the
	// zero test is `mov ecx,ebx; or edx,ebx`, VC6's __int64 != 0.
	DMHRESULT SetBuffer(void* pBuffer, unsigned __int64 cbSize);

	virtual DMHRESULT __stdcall QueryInterface(const DMGUID& riid, void** ppv); // 0x004436e0
	virtual uint32 __stdcall AddRef();                              // 0x004433e0
	virtual uint32 __stdcall Release();                             // 0x004437b0

	virtual DMHRESULT __stdcall Read(void* pv, uint32 cb, uint32* pcbRead); // 0x004437f0
	virtual DMHRESULT __stdcall Write(const void* pv, uint32 cb, uint32* pcbWritten);
	virtual DMHRESULT __stdcall Seek(DMLARGE_INTEGER dlibMove, uint32 dwOrigin,
	                                 DMULARGE_INTEGER* plibNewPosition); // 0x00443880
	virtual DMHRESULT __stdcall SetSize(DMULARGE_INTEGER libNewSize);
	virtual DMHRESULT __stdcall CopyTo(IStream* pstm, DMULARGE_INTEGER cb,
	                                   DMULARGE_INTEGER* pcbRead,
	                                   DMULARGE_INTEGER* pcbWritten);
	virtual DMHRESULT __stdcall Commit(uint32 grfCommitFlags);
	virtual DMHRESULT __stdcall Revert();
	virtual DMHRESULT __stdcall LockRegion(DMULARGE_INTEGER libOffset,
	                                       DMULARGE_INTEGER cb, uint32 dwLockType);
	virtual DMHRESULT __stdcall UnlockRegion(DMULARGE_INTEGER libOffset,
	                                         DMULARGE_INTEGER cb, uint32 dwLockType);
	virtual DMHRESULT __stdcall Stat(DMSTATSTG* pstatstg, uint32 grfStatFlag);
	virtual DMHRESULT __stdcall Clone(IStream** ppstm);

	virtual DMHRESULT __stdcall GetLoader(CDMLoader** ppLoader);    // 0x00443780

public:
	// 0x00 and 0x04 are the two vtables.
	long   m_nRefCount;                      // 0x008  starts at 1
	uint8* m_pBuffer;                        // 0x00c
	uint32 m_nSizeLow;                       // 0x010
	int32  m_nSizeHigh;                      // 0x014
	uint32 m_nPosLow;                        // 0x018
	int32  m_nPosHigh;                       // 0x01c
	CDMLoader* m_pLoader;                    // 0x020
};


// *********************************************************************** //
//  CDMFileRecord / CDMFileMgr
//
//  MOVE-ME: src/engine/lithtech/clientworld.cpp, section "the locked list at
//  0x42d210..0x42d760" - these two bodies sit inside that block in the image.
// *********************************************************************** //

// 0x0042d210: point the record at a (new) file and load it. Refused (1,
// the same nonzero-is-failure answer Load gives) while anyone holds a use;
// an open file is let go first, the name is replaced with a copy of the
// argument when there is one, and the answer is Load's.
LTBOOL CDMFileRecord::SetFilename(const char* pFilename)
{
	LTBOOL bResult;

	EnterCriticalSection(&m_pOwner->m_CriticalSection);

	if (m_nRefs > 0)
	{
		LeaveCriticalSection(&m_pOwner->m_CriticalSection);
		return LTTRUE;
	}

	if (m_pStream)
		TryDiscard();

	if (pFilename)
	{
		if (m_pFilename)
			delete[] m_pFilename;

		m_pFilename = new char[strlen(pFilename) + 1];
		if (!m_pFilename)
		{
			LeaveCriticalSection(&m_pOwner->m_CriticalSection);
			return LTTRUE;
		}

		strcpy(m_pFilename, pFilename);
	}

	bResult = Load();

	LeaveCriticalSection(&m_pOwner->m_CriticalSection);

	return bResult;
}

// 0x0042d330: take a use of the record and hand back its open stream. An
// unopened record is opened (Load answers nonzero on failure, and the use is
// given back); an open one is moved to the front of the loaded list unless it
// is already there. Answers the stream, null on failure.
ILTStreamView* CDMFileRecord::Open()
{
	EnterCriticalSection(&m_pOwner->m_CriticalSection);

	m_nRefs++;

	if (!m_pStream)
	{
		if (Load())
		{
			m_nRefs--;
			LeaveCriticalSection(&m_pOwner->m_CriticalSection);
			return LTNULL;
		}
	}
	else if (m_pOwner->m_Loaded.m_pHead != this)
	{
		m_pOwner->m_Loaded.Remove(this);
		m_pOwner->m_Loaded.Add(this);
	}

	LeaveCriticalSection(&m_pOwner->m_CriticalSection);

	return m_pStream;
}

// 0x0042d610: take a record off whichever list holds it, close it, and free
// its name and itself. An open record that TryDiscard keeps (still in use) is
// on the loaded list; anything else is on the unloaded one.
void CDMFileMgr::FreeRecord(CDMFileRecord* pRecord)
{
	if (pRecord)
	{
		EnterCriticalSection(&m_CriticalSection);

		if (pRecord->m_pStream && pRecord->TryDiscard())
			m_Loaded.Remove(pRecord);
		else
			m_Unloaded.Remove(pRecord);

		LeaveCriticalSection(&m_CriticalSection);

		if (pRecord->m_pStream)
			pRecord->TryDiscard();

		if (pRecord->m_pFilename)
			delete[] pRecord->m_pFilename;

		delete pRecord;
	}
}


// *********************************************************************** //
//  CDMSearchDirList / CDMStreamList
// *********************************************************************** //

// 0x00442f20: the directory remembered for a class. A miss on any class but
// GUID_DirectMusicAllTypes falls back, when asked to, to the all-types entry
// by calling itself - the lock is still held, which the recursive section
// allows. A miss on all-types itself is simply null.
CDMSearchDir* CDMSearchDirList::Find(const DMGUID& rguidClass, int bCreate)
{
	CDMSearchDir* pDir;
	DMCRITICAL_SECTION* pCS = &m_CriticalSection;

	EnterCriticalSection(pCS);

	for (pDir = m_pHead; pDir; pDir = pDir->m_pNext)
	{
		if (!memcmp(&pDir->m_guidClass, &rguidClass, sizeof(DMGUID)))
		{
			LeaveCriticalSection(pCS);
			return pDir;
		}
	}

	if (!memcmp(&rguidClass, &GUID_DirectMusicAllTypes, sizeof(DMGUID)))
	{
		LeaveCriticalSection(pCS);
		return LTNULL;
	}

	if (bCreate)
	{
		pDir = Find(GUID_DirectMusicAllTypes, 1);
		LeaveCriticalSection(pCS);
		return pDir;
	}

	LeaveCriticalSection(pCS);
	return LTNULL;
}

// 0x00442fe0: remember a directory for a class, at the head of the list. The
// record's GUID starts as GUID_NULL and is then assigned; the path is cleared
// in full and at most 0x103 characters copied, so it always ends in a zero.
void CDMSearchDirList::Add(const DMGUID& rguidClass, uint16* pwzPath)
{
	CDMSearchDir* pDir;

	EnterCriticalSection(&m_CriticalSection);

	pDir = new CDMSearchDir;

	if (pDir)
	{
		pDir->m_guidClass = GUID_NULL;
		pDir->m_guidClass = rguidClass;
		memset(pDir->m_wzPath, 0, sizeof(pDir->m_wzPath));
		wcsncpy(pDir->m_wzPath, pwzPath, 0x103);
		ListAddHead(pDir);
		LeaveCriticalSection(&m_CriticalSection);
		return;
	}

	LeaveCriticalSection(&m_CriticalSection);
}

// 0x00443090: unlink one search directory under the lock and free it. The
// list Remove at 0x0048dee0 does not free, so the delete is this function's.
void CDMSearchDirList::Remove(CDMSearchDir* pDir)
{
	EnterCriticalSection(&m_CriticalSection);

	ListRemove(pDir);
	delete pDir;

	LeaveCriticalSection(&m_CriticalSection);
}

// 0x004430d0: empty the list. One lock round the whole loop and the inner
// Remove taking it again - the shipped code really does re-enter, which is
// what a recursive Win32 critical section allows and what makes this a plain
// call to Remove rather than an inlined body. Reads the head afresh each time
// because Remove is what moved it.
void CDMSearchDirList::RemoveAll()
{
	EnterCriticalSection(&m_CriticalSection);

	while (m_pHead)
		Remove(m_pHead);

	LeaveCriticalSection(&m_CriticalSection);
}

// 0x00443100: wrap a stream in a node and put it at the head, under the lock.
// A failed allocation is simply not linked: the store of the stream and the
// AddHead are both inside the null test and there is no other path.
void CDMStreamList::Add(CDMFileStream* pStream)
{
	CDMStreamNode* pNode;

	EnterCriticalSection(&m_CriticalSection);

	pNode = new CDMStreamNode;

	if (pNode)
	{
		pNode->m_pStream = pStream;
		ListAddHead(pNode);
	}

	LeaveCriticalSection(&m_CriticalSection);
}

// 0x00443140: find the stream's node and unlink and free it. The search is a
// loop that breaks on a match and the found-path tests the node again - the
// second `test %esi,%esi` at 0x00443171 - so the unlink sits after the loop
// under its own null test rather than inside it.
void CDMStreamList::Remove(CDMFileStream* pStream)
{
	CDMStreamNode* pNode;

	EnterCriticalSection(&m_CriticalSection);

	for (pNode = m_pHead; pNode; pNode = pNode->m_pNext)
	{
		if (pNode->m_pStream == pStream)
			break;
	}

	if (pNode)
	{
		ListRemove(pNode);
		delete pNode;
	}

	LeaveCriticalSection(&m_CriticalSection);
}

// 0x004431a0: destroy every stream still on the list. Nothing here unlinks
// anything - the loop only ever deletes the stream at the head node's 0x08 and
// then reads the head again - because ~CDMFileStream is what removes its own
// node (through CDMStreamList::Remove). A stream whose node has a null 0x08
// would spin, and nothing ever stores one.
void CDMStreamList::DeleteAll()
{
	EnterCriticalSection(&m_CriticalSection);

	while (m_pHead)
		delete m_pHead->m_pStream;

	LeaveCriticalSection(&m_CriticalSection);
}


// *********************************************************************** //
//  CDMLoader
// *********************************************************************** //

// 0x00443900: one COM count, no stream count and no cache; the two one-
// character wide strings; the two lists; and the engine's file manager set to
// keep five files open (0x0042d520 with 5).
CDMLoader::CDMLoader()
{
	InitializeCriticalSection(&m_CriticalSection);

	m_nRefCount      = 1;
	m_nStreamRefCount = 0;
	m_pCachedObjects = LTNULL;

	MultiByteToWideChar(DM_CP_ACP, DM_MB_PRECOMPOSED, "/", -1, m_wzFwdSlash, DM_MAX_PATH);
	MultiByteToWideChar(DM_CP_ACP, DM_MB_PRECOMPOSED, "\\", -1, m_wzBackSlash, DM_MAX_PATH);

	m_pSearchDirs = new CDMSearchDirList;
	m_pStreams    = new CDMStreamList;

	g_DMFileMgr.Init(5);
}

// 0x004439b0: release and free every cached object, delete the two lists
// (each destructor expanded inside the delete's null test), take the file
// manager down and then the lock.
CDMLoader::~CDMLoader()
{
	CDMCachedObject* pObject = m_pCachedObjects;

	while (pObject)
	{
		m_pCachedObjects = pObject->m_pNext;

		if (pObject->m_pObject)
			pObject->m_pObject->Release();

		delete pObject;

		pObject = m_pCachedObjects;
	}

	if (m_pSearchDirs)
	{
		delete m_pSearchDirs;
		m_pSearchDirs = LTNULL;
	}

	if (m_pStreams)
	{
		delete m_pStreams;
		m_pStreams = LTNULL;
	}

	g_DMFileMgr.Term();

	DeleteCriticalSection(&m_CriticalSection);
}

// 0x00443a50 (name ours): all it does is settle which platform this is, and
// answer S_OK. Called once, on the loader Init has just constructed.
DMHRESULT CDMLoader::Init()
{
	dm_CheckPlatform();

	return DM_S_OK;
}

// 0x00443a60: IUnknown or IDirectMusicLoader8, compared with an inlined
// memcmp each (`repz cmpsl`, four dwords). The loader is its own interface,
// so both hand out `this` unadjusted and AddRef through the vtable.
DMHRESULT CDMLoader::QueryInterface(const DMGUID& riid, void** ppv)
{
	if (!memcmp(&riid, &IID_IUnknown, sizeof(DMGUID)) ||
	    !memcmp(&riid, &IID_IDirectMusicLoader8, sizeof(DMGUID)))
	{
		*ppv = this;
		AddRef();
		return DM_S_OK;
	}

	*ppv = LTNULL;
	return DM_E_NOINTERFACE;
}

// 0x00443ac0: the COM count, at 0x04.
uint32 CDMLoader::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

// 0x00443ae0: the count the streams hold, at 0x08. A thiscall member with no
// arguments and no return - the shipped body is three instructions, `add
// $0x8,%ecx`, the push and the call, and it does not even keep the result.
void CDMLoader::InternalAddRef()
{
	InterlockedIncrement(&m_nStreamRefCount);
}

// 0x00443af0: the COM release. At zero the loader first props its count back
// up to one and clears its whole cache through its own ClearCache slot (0x24,
// GUID_DirectMusicAllTypes) - the objects it drops may call back into it -
// and only a second drop to zero with no stream still holding the loader
// deletes it. Every other path answers the count as it now stands.
uint32 CDMLoader::Release()
{
	if (InterlockedDecrement(&m_nRefCount) == 0)
	{
		InterlockedIncrement(&m_nRefCount);
		ClearCache(GUID_DirectMusicAllTypes);

		if (InterlockedDecrement(&m_nRefCount) == 0 && m_nStreamRefCount == 0)
		{
			delete this;
			return 0;
		}
	}

	return m_nRefCount;
}

// 0x00443b50: drop the streams' count, and delete the loader when that and
// the COM count are both zero. The COM count is read plainly, not
// interlocked, which is what the second `mov 0x4(%esi),%eax` shows.
uint32 CDMLoader::InternalRelease()
{
	if (InterlockedDecrement(&m_nStreamRefCount) == 0 && m_nRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_nStreamRefCount;
}

// 0x00443b90: a cached object with the descriptor's GUID answers the
// request straight away. Otherwise the object is loaded from the file or the
// memory the descriptor names, cached if its own descriptor carries a GUID,
// and asked for the interface; the loader's own reference is then dropped
// (the cache holds one of its own). One Leave on the way out.
DMHRESULT CDMLoader::GetObject(DMUS_OBJECTDESC* pDesc, const DMGUID& riid, void** ppv)
{
	DMUS_OBJECTDESC     desc;
	DMCRITICAL_SECTION* pCS = &m_CriticalSection;
	IDirectMusicObject* pObject;
	CDMCachedObject*    pCached;
	DMHRESULT           hr;

	EnterCriticalSection(pCS);

	if (pDesc->dwValidData & DM_DMUS_OBJ_OBJECT)
	{
		for (pCached = m_pCachedObjects; pCached; pCached = pCached->m_pNext)
		{
			if (!memcmp(&pDesc->guidObject, &pCached->m_guidObject, sizeof(DMGUID)))
				break;
		}

		if (pCached)
		{
			pObject = pCached->m_pObject;
			hr = DM_E_FAIL;
			if (pCached->m_pObject)
				hr = pCached->m_pObject->QueryInterface(riid, ppv);

			goto Done;
		}
	}

	if (pDesc->dwValidData & DM_DMUS_OBJ_FILENAME)
		hr = GetObjectFromFileQ440(pDesc, &pObject);
	else if (pDesc->dwValidData & DM_DMUS_OBJ_MEMORY)
		hr = GetObjectFromMemoryQ440(pDesc, &pObject);
	else
		hr = DM_DMUS_E_LOADER_NOFILENAME;

	if (hr >= 0)
	{
		memset(&desc, 0, sizeof(desc));
		desc.dwSize = sizeof(desc);
		pObject->GetDescriptor(&desc);

		if (desc.dwValidData & DM_DMUS_OBJ_OBJECT)
		{
			pCached = new CDMCachedObject;
			if (pCached)
			{
				pCached->m_guidObject = desc.guidObject;
				pCached->m_pNext = m_pCachedObjects;
				m_pCachedObjects = pCached;
				pCached->m_pObject = pObject;
				pObject->AddRef();
			}
		}

		hr = pObject->QueryInterface(riid, ppv);
		pObject->Release();
	}

Done:
	LeaveCriticalSection(pCS);

	return hr;
}

// 0x00443f60: takes the lock, drops it again and refuses. The lock is taken
// round nothing at all, which is what a scoped guard object declared at the
// top of a function whose body is a single `return` compiles to.
DMHRESULT CDMLoader::SetObject(DMUS_OBJECTDESC* pDesc)
{
	EnterCriticalSection(&m_CriticalSection);
	LeaveCriticalSection(&m_CriticalSection);

	return DM_E_NOTIMPL;
}

// 0x00443f80: remember (or forget) the directory for a class. A class the
// list already has gets its path cleared in full and at most 0x103 wide
// characters copied over it (`rep stos` of 0x82 words, then wcsncpy), or is
// dropped when the path is null; a new class goes to Add. fClear is never
// read. Every exit answers S_OK - three separate `xor %eax,%eax; ret $0x10`.
DMHRESULT CDMLoader::SetSearchDirectory(const DMGUID& rguidClass, uint16* pwzPath,
                                        int fClear)
{
	CDMSearchDir* pDir;

	pDir = m_pSearchDirs->Find(rguidClass, 0);

	if (pDir)
	{
		if (!pwzPath)
		{
			m_pSearchDirs->Remove(pDir);
			return DM_S_OK;
		}

		memset(pDir->m_wzPath, 0, sizeof(pDir->m_wzPath));
		wcsncpy(pDir->m_wzPath, pwzPath, 0x103);
		return DM_S_OK;
	}

	m_pSearchDirs->Add(rguidClass, pwzPath);
	return DM_S_OK;
}

// The five slots this loader never implements. Every one of them is a two-
// instruction `mov $E_NOTIMPL,%eax; ret N`, and the linker folds each shape
// into one body shared with the streams' stubs below - so ScanDirectory and
// EnumObject are both 0x00443870, CacheObject, ReleaseObject and ClearCache
// are all 0x004435d0, and EnableCache is 0x004435b0.
DMHRESULT CDMLoader::ScanDirectory(const DMGUID& rguidClass, uint16* pwzFileExtension,
                                   uint16* pwzScanFileName)                 // 0x00443870
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMLoader::CacheObject(IDirectMusicObject* pObject)               // 0x004435d0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMLoader::ReleaseObject(IDirectMusicObject* pObject)             // 0x004435d0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMLoader::ClearCache(const DMGUID& rguidClass)                   // 0x004435d0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMLoader::EnableCache(const DMGUID& rguidClass, int fEnable)     // 0x004435b0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMLoader::EnumObject(const DMGUID& rguidClass, uint32 dwIndex,
                                DMUS_OBJECTDESC* pDesc)                     // 0x00443870
{
	return DM_E_NOTIMPL;
}

// clientmusic.h's view of the loader it hands dm_SetSearchDirectory: only
// ever a pointer, so deriving it from the interface this file declares is
// enough to keep the declaration's (struct) mangling and reach slot 0x14.
struct IDirectMusicLoader8 : public IDirectMusicLoader
{
};

// 0x00443ff0: widen a path into a MAX_PATH frame buffer (`sub $0x208`),
// refusing one of MAX_PATH characters or more with E_FAIL before anything is
// converted, and hand it to the loader's SetSearchDirectory, slot 0x14,
// answering whatever that answers. __stdcall, `ret $0x10` (name ours; the
// declaration is clientmusic.h's).
long __stdcall dm_SetSearchDirectory(IDirectMusicLoader8* pLoader, const DMGUID& rguidClass,
                                     const char* sPath, int bClear)
{
	uint16 wzPath[DM_MAX_PATH];

	if (strlen(sPath) >= DM_MAX_PATH)
		return DM_E_FAIL;

	MultiByteToWideChar(DM_CP_ACP, DM_MB_PRECOMPOSED, sPath, -1, wzPath, DM_MAX_PATH);

	return pLoader->SetSearchDirectory(rguidClass, wzPath, bClear);
}


// *********************************************************************** //
//  CDMFileStream
// *********************************************************************** //

// 0x00443220: a count of one, no file yet, and the loader's stream count and
// stream list both joined when there is a loader. The store of 0x004a25b0 at
// 0x04 ahead of the real vptrs is IDirectMusicGetLoader's own constructor.
CDMFileStream::CDMFileStream(CDMLoader* pLoader)
{
	m_nRefCount = 1;
	m_pFile     = LTNULL;
	m_pLoader   = pLoader;

	if (pLoader)
	{
		pLoader->InternalAddRef();
		pLoader->m_pStreams->Add(this);
	}
}

// 0x00443270: take the stream off the loader's list, let go of the loader's
// stream count and close the file. Both of the loader's two jobs are inside
// one null test on m_pLoader, and the close is outside it. The two `movl` of
// the vtable addresses at the top are the destructor reinstating its own
// vptrs, which VC6 emits for every class with virtuals.
CDMFileStream::~CDMFileStream()
{
	if (m_pLoader)
	{
		m_pLoader->m_pStreams->Remove(this);
		m_pLoader->InternalRelease();
	}

	Close();
}

// 0x00443380: shut the file record and hand it back to the manager. Answers
// S_OK either way; `xor %eax,%eax` sits after the test, on the shared exit.
DMHRESULT CDMFileStream::Close()
{
	if (m_pFile)
	{
		m_pFile->TryDiscard();
		g_DMFileMgr.FreeRecord(m_pFile);
		m_pFile = LTNULL;
	}

	return DM_S_OK;
}

// 0x004433b0: ask the loader for its IDirectMusicLoader8 face. The offsets
// prove where this one lives: it reads its `this` from 0x4(%esp) and then
// 0x0c(%esp), which is 0x10 of the whole object - m_pLoader - because
// GetLoader is only ever in the second base's vtable and VC6 compiles such a
// function with `this` already pointing at that base, folding the four-byte
// adjustment into every member offset.
DMHRESULT CDMFileStream::GetLoader(CDMLoader** ppLoader)
{
	// The positive condition wraps the body, which is what hosts the failure
	// answer at the END of the function rather than inline after a guard.
	if (m_pLoader)
		return m_pLoader->QueryInterface(IID_IDirectMusicLoader8,
		                                 (void**)ppLoader);

	*ppLoader = LTNULL;
	return DM_E_NOINTERFACE;
}

// 0x004436e0, folded with CDMMemStream::QueryInterface (the address is the
// memory stream's; both stream vtables' slot 0x00 point there). IUnknown and
// IStream hand out `this`, IDirectMusicGetLoader the second base at +4 - the
// `test`/`lea 0x4` pair is VC6's null-guarded derived-to-base conversion -
// and every hit AddRefs through the first vtable.
DMHRESULT CDMFileStream::QueryInterface(const DMGUID& riid, void** ppv)
{
	if (!memcmp(&riid, &IID_IUnknown, sizeof(DMGUID)) ||
	    !memcmp(&riid, &IID_IStream, sizeof(DMGUID)))
	{
		*ppv = static_cast<IStream*>(this);
		AddRef();
		return DM_S_OK;
	}
	else if (!memcmp(&riid, &IID_IDirectMusicGetLoader, sizeof(DMGUID)))
	{
		*ppv = static_cast<IDirectMusicGetLoader*>(this);
		AddRef();
		return DM_S_OK;
	}

	*ppv = LTNULL;
	return DM_E_NOINTERFACE;
}

// 0x004433e0: the count at 0x08. Identical to CDMMemStream's, and the linker
// folds the two into this one address - both stream vtables' slot 0x04 point
// here.
uint32 CDMFileStream::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

// 0x00443400: `delete this` at zero, and the surviving count otherwise. The
// null test round the delete is the compiler's, not the source's.
uint32 CDMFileStream::Release()
{
	if (InterlockedDecrement(&m_nRefCount) == 0)
	{
		delete this;
		return 0;
	}

	return m_nRefCount;
}

// 0x00443440: open the record for the length of one read. No record, or a
// record that will not open, is E_FAIL; so is a failed ILTStream::Read, which
// lets the record go on its own exit. The byte count reported is always the
// full request.
DMHRESULT CDMFileStream::Read(void* pv, uint32 cb, uint32* pcbRead)
{
	ILTStreamView* pStream;

	if (!m_pFile)
		return DM_E_FAIL;

	pStream = m_pFile->Open();

	if (!pStream)
		return DM_E_FAIL;

	if (pStream->Read(pv, cb))
	{
		m_pFile->Close();
		return DM_E_FAIL;
	}

	if (pcbRead)
		*pcbRead = cb;

	m_pFile->Close();

	return DM_S_OK;
}

// 0x004434b0: writing is refused with E_FAIL rather than E_NOTIMPL - the only
// stub in either stream that does, and the reason this body is its own eight
// bytes instead of folding into 0x00443870.
DMHRESULT CDMFileStream::Write(const void* pv, uint32 cb, uint32* pcbWritten)
{
	return DM_E_FAIL;
}

// 0x004434c0: the record is opened for the length of the seek, like Read.
// Only 32-bit positions exist underneath: the move's low word is added to
// the current position or the length, and the high word of every position
// handed back is zero. A zero move from the current position answers it
// without seeking. An unknown origin, a failed GetPos or a failed SeekTo is
// E_FAIL; GetLen's answer is not checked.
DMHRESULT CDMFileStream::Seek(DMLARGE_INTEGER dlibMove, uint32 dwOrigin,
                              DMULARGE_INTEGER* plibNewPosition)
{
	ILTStreamView*   pStream;
	DMULARGE_INTEGER pos;
	uint32           nOffset;

	if (!m_pFile)
		return DM_E_FAIL;

	pStream = m_pFile->Open();

	if (!pStream)
		return DM_E_FAIL;

	if (dwOrigin == DM_STREAM_SEEK_SET)
	{
		nOffset = dlibMove.LowPart;
	}
	else if (dwOrigin == DM_STREAM_SEEK_CUR)
	{
		if (pStream->GetPos((unsigned long*)&pos.LowPart))
		{
			m_pFile->Close();
			return DM_E_FAIL;
		}

		pos.HighPart = 0;
		nOffset = pos.LowPart + dlibMove.LowPart;

		if (*(__int64*)&dlibMove == 0)
		{
			if (plibNewPosition)
				*plibNewPosition = pos;

			m_pFile->Close();
			return DM_S_OK;
		}
	}
	else if (dwOrigin == DM_STREAM_SEEK_END)
	{
		pStream->GetLen((unsigned long*)&pos.LowPart);
		nOffset = pos.LowPart + dlibMove.LowPart;
	}
	else
	{
		m_pFile->Close();
		return DM_E_FAIL;
	}

	if (pStream->SeekTo(nOffset))
	{
		m_pFile->Close();
		return DM_E_FAIL;
	}

	if (plibNewPosition)
	{
		pStream->GetPos((unsigned long*)&plibNewPosition->LowPart);
		plibNewPosition->HighPart = 0;
	}

	m_pFile->Close();

	return DM_S_OK;
}

// The eight slots neither stream implements. Each shape folds to one body:
//   0x004435b0  ret 0xc   SetSize, Stat
//   0x004435c0  ret 0x18  CopyTo, LockRegion, UnlockRegion
//   0x004435d0  ret 8     Commit, Clone
//   0x004435e0  ret 4     Revert
// Eight slots in each of the two stream vtables, sixteen in all, land on
// those four addresses.
DMHRESULT CDMFileStream::SetSize(DMULARGE_INTEGER libNewSize)               // 0x004435b0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMFileStream::CopyTo(IStream* pstm, DMULARGE_INTEGER cb,
                                DMULARGE_INTEGER* pcbRead,
                                DMULARGE_INTEGER* pcbWritten)               // 0x004435c0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMFileStream::Commit(uint32 grfCommitFlags)                      // 0x004435d0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMFileStream::Revert()                                           // 0x004435e0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMFileStream::LockRegion(DMULARGE_INTEGER libOffset,
                                    DMULARGE_INTEGER cb, uint32 dwLockType) // 0x004435c0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMFileStream::UnlockRegion(DMULARGE_INTEGER libOffset,
                                      DMULARGE_INTEGER cb, uint32 dwLockType) // 0x004435c0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMFileStream::Stat(DMSTATSTG* pstatstg, uint32 grfStatFlag)      // 0x004435b0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMFileStream::Clone(IStream** ppstm)                             // 0x004435d0
{
	return DM_E_NOTIMPL;
}


// *********************************************************************** //
//  CDMMemStream
// *********************************************************************** //

// 0x004435f0: a count of one, an empty buffer at position zero, and the
// loader's stream count joined. Not put on the stream list - only file
// streams are. The size and position are each cleared as ONE 64-bit store:
// that is what hoists the two low words (0x10, 0x18) above the vptr stores as
// shipped; four separate 32-bit stores keep them in field order after.
CDMMemStream::CDMMemStream(CDMLoader* pLoader)
{
	m_nRefCount = 1;
	m_pBuffer   = LTNULL;
	*(unsigned __int64*)&m_nSizeLow = 0;
	*(unsigned __int64*)&m_nPosLow  = 0;
	m_pLoader   = pLoader;

	if (pLoader)
		pLoader->InternalAddRef();
}

// 0x00443640: let the loader go and forget the buffer. The same two vtable
// stores at the top, and the same single null test round the loader.
CDMMemStream::~CDMMemStream()
{
	if (m_pLoader)
		m_pLoader->InternalRelease();

	Close();
}

// 0x004436d0: the length's low word, the pointer, then the length's high
// word. The position is deliberately left where it was - only three stores.
void CDMMemStream::Close()
{
	m_nSizeLow  = 0;
	m_pBuffer   = LTNULL;
	m_nSizeHigh = 0;
}

// 0x00443670 (name ours): point the stream at a caller's buffer, rewound.
// A null buffer, an empty one or one IsBadReadPtr refuses is 0x88781182
// (DMUS_E_...); the refused-pointer path forgets the buffer again with the
// same three stores Close makes, in the same order. Size and position are
// again stored as 64-bit values (the store order at 0x00443687 needs it), and
// the refusal is the `if` arm with success the `else`, which is what puts the
// S_OK exit last.
DMHRESULT CDMMemStream::SetBuffer(void* pBuffer, unsigned __int64 cbSize)
{
	Close();

	m_pBuffer   = (uint8*)pBuffer;
	*(unsigned __int64*)&m_nSizeLow = cbSize;
	*(unsigned __int64*)&m_nPosLow  = 0;

	if (pBuffer && cbSize != 0)
	{
		if (IsBadReadPtr(pBuffer, (uint32)cbSize))
		{
			m_nSizeLow  = 0;
			m_pBuffer   = LTNULL;
			m_nSizeHigh = 0;
		}
		else
			return DM_S_OK;
	}

	return 0x88781182;
}

// 0x004436e0: the same body as CDMFileStream::QueryInterface; the linker
// keeps this copy's address for both.
DMHRESULT CDMMemStream::QueryInterface(const DMGUID& riid, void** ppv)
{
	if (!memcmp(&riid, &IID_IUnknown, sizeof(DMGUID)) ||
	    !memcmp(&riid, &IID_IStream, sizeof(DMGUID)))
	{
		*ppv = static_cast<IStream*>(this);
		AddRef();
		return DM_S_OK;
	}
	else if (!memcmp(&riid, &IID_IDirectMusicGetLoader, sizeof(DMGUID)))
	{
		*ppv = static_cast<IDirectMusicGetLoader*>(this);
		AddRef();
		return DM_S_OK;
	}

	*ppv = LTNULL;
	return DM_E_NOINTERFACE;
}

// 0x00443780: the same body as CDMFileStream's, against m_pLoader at 0x20 -
// 0x1c from the second base, which is where the shipped `mov 0x1c(%eax),%eax`
// reads it.
DMHRESULT CDMMemStream::GetLoader(CDMLoader** ppLoader)
{
	// The positive condition wraps the body, which is what hosts the failure
	// answer at the END of the function rather than inline after a guard.
	if (m_pLoader)
		return m_pLoader->QueryInterface(IID_IDirectMusicLoader8,
		                                 (void**)ppLoader);

	*ppLoader = LTNULL;
	return DM_E_NOINTERFACE;
}

// 0x004433e0, folded with CDMFileStream::AddRef.
uint32 CDMMemStream::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

// 0x004437b0: the same shape as CDMFileStream::Release, and a separate body
// because the destructor it calls is a different one.
uint32 CDMMemStream::Release()
{
	if (InterlockedDecrement(&m_nRefCount) == 0)
	{
		delete this;
		return 0;
	}

	return m_nRefCount;
}

// 0x004437f0: copy out of the buffer and advance. A request that would run
// past the end is E_FAIL with nothing copied - the end test is the signed
// 64-bit `jg`/`jl`/`ja` against the size - and the byte count reported is the
// full request.
DMHRESULT CDMMemStream::Read(void* pv, uint32 cb, uint32* pcbRead)
{
	if (cb + *(__int64*)&m_nPosLow <= *(__int64*)&m_nSizeLow)
	{
		memcpy(pv, &m_pBuffer[m_nPosLow], cb);
		*(__int64*)&m_nPosLow += cb;

		if (pcbRead)
			*pcbRead = cb;

		return DM_S_OK;
	}

	return DM_E_FAIL;
}

// 0x00443870: this one refuses with E_NOTIMPL, not E_FAIL, and so folds with
// the loader's ScanDirectory and EnumObject rather than with
// CDMFileStream::Write.
DMHRESULT CDMMemStream::Write(const void* pv, uint32 cb, uint32* pcbWritten)
{
	return DM_E_NOTIMPL;
}

// 0x00443880: the new position is the move relative to the current position,
// the end or the start, and anything before the start or past the end is
// E_FAIL. The comparisons are signed 64-bit (`jl`/`jg` on the high words).
DMHRESULT CDMMemStream::Seek(DMLARGE_INTEGER dlibMove, uint32 dwOrigin,
                             DMULARGE_INTEGER* plibNewPosition)
{
	__int64 nPos = *(__int64*)&dlibMove;

	if (dwOrigin == DM_STREAM_SEEK_CUR)
		nPos += *(__int64*)&m_nPosLow;
	else if (dwOrigin == DM_STREAM_SEEK_END)
		nPos += *(__int64*)&m_nSizeLow;

	if (nPos < 0 || nPos > *(__int64*)&m_nSizeLow)
		return DM_E_FAIL;

	*(__int64*)&m_nPosLow = nPos;

	if (plibNewPosition)
		*(__int64*)plibNewPosition = nPos;

	return DM_S_OK;
}

// The other eight, folding onto the same four bodies as CDMFileStream's.
DMHRESULT CDMMemStream::SetSize(DMULARGE_INTEGER libNewSize)                // 0x004435b0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMMemStream::CopyTo(IStream* pstm, DMULARGE_INTEGER cb,
                               DMULARGE_INTEGER* pcbRead,
                               DMULARGE_INTEGER* pcbWritten)                // 0x004435c0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMMemStream::Commit(uint32 grfCommitFlags)                       // 0x004435d0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMMemStream::Revert()                                            // 0x004435e0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMMemStream::LockRegion(DMULARGE_INTEGER libOffset,
                                   DMULARGE_INTEGER cb, uint32 dwLockType)  // 0x004435c0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMMemStream::UnlockRegion(DMULARGE_INTEGER libOffset,
                                     DMULARGE_INTEGER cb, uint32 dwLockType) // 0x004435c0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMMemStream::Stat(DMSTATSTG* pstatstg, uint32 grfStatFlag)       // 0x004435b0
{
	return DM_E_NOTIMPL;
}

DMHRESULT CDMMemStream::Clone(IStream** ppstm)                              // 0x004435d0
{
	return DM_E_NOTIMPL;
}


// *********************************************************************** //
//  Module odds and ends
// *********************************************************************** //

// 0x004431e0 (name ours): ask Windows which platform this is and remember
// whether it is not Win9x. VER_PLATFORM_WIN32_WINDOWS is 1, and the flag is
// `setne` on that comparison, so a set flag means NT. Nothing in the image
// reads the global back - the only reference to 0x004b4020 anywhere in .text
// is this store - so whatever used to depend on it is gone and only the query
// is left. GetVersionExA's result is not tested either.
LTBOOL g_bDMNotWin9x;                        // 0x004b4020 (name ours)

void dm_CheckPlatform()
{
	DMOSVERSIONINFOA ver;

	ver.dwOSVersionInfoSize = sizeof(DMOSVERSIONINFOA);
	GetVersionExA(&ver);

	g_bDMNotWin9x = (ver.dwPlatformId != DM_VER_PLATFORM_WIN32_WINDOWS);
}


// --------------------------------------------------------------------- //
// The three CLTDirectMusicMgr slots that only read a number out of the
// manager. iltdirectmusic.h's declaration order is the vtable order, and the
// vtable is 0x004a2500 - the address CLTDirectMusicMgr's constructor stores
// at 0x0043f8cc. Counting the header's twenty pure virtuals from there puts
// slot 17 at 0x004a2544, slot 18 at 0x004a2548 and slot 19 at 0x004a254c,
// which hold 0x004428f0, 0x00442900 and 0x00442910 - and each of those reads
// exactly the member clientmusic.h already named for it, m_nNumIntensities at
// 0xa0, m_nInitialIntensity at 0xa4 and m_nInitialVolume at 0xa8. Two
// independent facts agreeing, so the naming is not a guess.
//
// (The shipped vtable runs two slots past the header's twenty, to 0x004a2554;
// those two are the two-argument PlayMotif and StopMotif clientmusic.h
// already documents as header drift. They do not move these three.)
//
// The manager itself is clientmusic.h's, not this file's, so it is reached
// here through a layout-identical view whose only members are the three
// numbers at the right offsets. A non-virtual member reading a field compiles
// to the same two instructions a virtual one does, which is all these bodies
// are: `mov 0xNN(%ecx),%eax; ret`.
// --------------------------------------------------------------------- //

class CDMMgrIntensityView
{
public:
	uint8 m_pad_00[0xa0];
	int   m_nNumIntensities;                 // 0x0a0
	int   m_nInitialIntensity;               // 0x0a4
	long  m_nInitialVolume;                  // 0x0a8

	int  GetNumIntensities();                                       // 0x004428f0
	int  GetInitialIntensity();                                     // 0x00442900
	long GetInitialVolume();                                        // 0x00442910
};

int CDMMgrIntensityView::GetNumIntensities()
{
	return m_nNumIntensities;
}

int CDMMgrIntensityView::GetInitialIntensity()
{
	return m_nInitialIntensity;
}

long CDMMgrIntensityView::GetInitialVolume()
{
	return m_nInitialVolume;
}


// --------------------------------------------------------------------- //
// Not written out, because the compiler writes them: the four adjustor thunks
// at 0x00444060, 0x00444070, 0x00444080 and 0x00444090, each ten bytes of
// `subl $0x4,0x4(%esp); jmp <body>`. They are the IDirectMusicGetLoader
// vtables' first three slots - the three IUnknown methods, which live in both
// of a stream's vtables and so need the four-byte `this` correction in the
// second - and they fall out of the multiple inheritance above:
//
//   0x00444060 -> 0x004436e0  QueryInterface, both streams (0x004a2568+0x00,
//                                                           0x004a25c0+0x00)
//   0x00444070 -> 0x004433e0  AddRef, both streams
//   0x00444080 -> 0x00443400  Release, CDMFileStream
//   0x00444090 -> 0x004437b0  Release, CDMMemStream
//
// GetLoader needs no thunk in either: it appears only in the second base's
// vtable, so its body is compiled against that base's `this` directly.
// --------------------------------------------------------------------- //
