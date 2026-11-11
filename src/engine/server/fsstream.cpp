// server.dll — the file-system's ILTStream implementations (reconstruction).
// Build /O1.
//
// Two of the engine's four streams live here, and both read out of a FILE*
// that they do not own alone: a resource tree is one file with many members
// in it, so a stream over a member is an offset, a length, and a share of the
// handle. The tree pointer at +0x10 is what decides who closes it: a stream
// over a file in a plain directory (tree kind 1) opened the handle itself and
// closes it, and a stream over a .rez member never does.
//
// Neither can be written to: Write is a three-byte "no", which is what makes
// them read-only in practice while still satisfying the interface.
//
// Both vtables (0x1004d280 and 0x1004d2ac) land on iltstream.h's eleven
// virtuals in declaration order, and both share ReadString, WriteStream and
// WriteString with every other stream in the image — the same three generic
// implementations, which is what confirms the anchoring.
//
// Class and file names are ours; the method names are the SDK's.
#include <stdio.h>
#include <string.h>
#include <new.h>
#include "servermgr.h"
#include "de_error.h"
#include "basestream.h"
#include "lthread.h"

// 0x100584a8 — how much the stream layer says about itself: 2 prints a line
// per close, 4 a line per read (name ours). It is the "ShowFileAccess"
// console variable, so dsi.cpp defines it beside the binding table.
extern int g_StreamDebugLevel;

// 0x1003d1f0 — hand a struct bank's chunks back; the second argument is
// accepted and ignored (declared in filemgr.cpp's terms; name ours).
void sb_FreeChunks(StructBank* pBank, int bAll);


// 0x100582e8 and 0x10058320 — the banks the two stream kinds come out of.
// Release hands the object back rather than deleting it (names ours).
//
// The layout comes off the two Allocates (0x100075f1 for the tree streams,
// 0x100077d7 for the memory ones): both take the lock at this+0x20, pass
// this+4 to sb_Allocate, and store their own vtable into the record on the
// way out. The first word is the vptr — the bank is one of the same
// vptr-over-StructBank objects as CMessageBank and CNetBank.
//
// The banks are polymorphic, and the vtable at 0x1004d260 is the base's: its
// three slots past the destructor are all _purecall (0x1003ea24 is
// `push $0x19; call _amsg_exit`, and 25 is _RT_PUREVIRT), so the base declares
// a virtual destructor and three pure virtuals and nothing else: bankbase.h's
// CBankBase, every bank's base. The
// destructor has to be *defined*, empty, or the derived destructor calls it
// instead of inlining the vtable-pointer store that ends both bank destructors
// (name ours).
//
// The three pure virtuals are an allocate, a free and Term, in that slot
// order, and they are not decoration either. The shipped /O2 client's two
// bank vtables (0x004a1ee8 and 0x004a1ef8) read
//
//     [ ??_G, AllocVoid, FreeVoid, Term ]
//     0x421a20 0x4218c0 0x4219c0 0x4219f0
//     0x421a60 0x421940 0x4219c0 0x4219f0
//
// and NOTHING in that image's .text calls any of those six addresses: every
// call site has the body expanded (Allocate into fs_OpenStreamInTree, Free
// into the two Releases). The standalone bodies exist only because the vtables
// take their addresses.
//
// The /O1 server says what the two slots really are. Its vtables (0x1004d250
// and 0x1004d270) read [ ??_G, 0x100075b5, 0x100075ba, Term ] and
// [ ??_G, 0x100075c6, 0x100075ba, Term ], and those three are FORWARDERS:
// `jmp 0x100075f1`, `push 4(esp) ; call 0x10007781 ; ret 4`, and
// `jmp 0x100077d7` -- while fs_OpenStreamInTree and the Releases call the
// workers 0x100075f1 / 0x100077d7 / 0x10007781 directly. So the virtuals are
// one-line wrappers over non-virtual inline workers, exactly stdlith's
// ObjectBank shape (object_bank.h: `virtual void* AllocVoid() {return
// Allocate();}` and `virtual void FreeVoid(void *ptr) {Free((T*)ptr);}`),
// whose names these take. /Os declines to expand the workers into the
// wrappers; /Ot expands them, which is the /O2 vtables' expanded slots.
class CStreamBank : public CBankBase
{
public:
	// 0x10006d88 — a bank comes up with room for eight streams and grows
	// eight at a time. 0x24 is sizeof(CTreeStream), which is declared
	// further down this file; both stream kinds are that size, which is why
	// the two banks differ only in the vtable pointer they stamp.
	//
	// `__forceinline` is not decoration: the shipped initializer has the
	// whole constructor expanded into it, with the global's address folded
	// into every store and no `this` register at all, and that is the only
	// spelling found that reproduces it. Plain `inline` (in the class or out
	// of it) leaves VC6 calling an out-of-line copy, and /Ob2 does not
	// expand it either — it merges the two initializer thunks into one
	// 22-byte function, which the shipped build does not have.
	// AVP2: the counts are the constructor's arguments, as ObjectBank's
	// (cache size, pre-allocate) constructor has them: PH's out-of-line
	// copies (lithtech 0x0042a3a0, server.dll 0x1000a237) push their two
	// arguments to sb_Init2, `ret $8`, and the initializers pass 8, 8
	// (lithtech 0x00429310). AvP2 1.0.9.6, without /GX, expands it as
	// before.
	__forceinline CStreamBank(uint32 nCacheSize, uint32 nPreAllocate)
		{ sb_Init2(&m_Bank, 0x24, nCacheSize, nPreAllocate); }
	~CStreamBank();                          // 0x100075cb
	// 0x100075b5 and 0x100075ba — the vtable's two forwarders.
	virtual void* AllocVoid()               { return Allocate(); }
	virtual void  FreeVoid(void* pStream)   { Free(pStream); }
	void Free(void* pStream);                // 0x10007781
	// 0x100075f1 / 0x100077d7 — one per bank, differing only in which
	// vtable they stamp on the record.
	ILTStream* Allocate();
	// 0x100078e9 — the third of the base's three pure virtuals, shared by
	// both banks the way Free is: the two streams are the same size, so the
	// two bodies are one function in the image.
	void Term();

	StructBank       m_Bank;                 // 0x04
	LCriticalSection m_Lock;                 // 0x20
};

// The memory-stream bank is the same record with the other vtable, and its
// destructor (0x100077b1) is the same 38 bytes with the other two vtable
// pointers in it. The image holds a single Free for both banks (0x10007781,
// reached from both Releases with different `this`), which looks at first like
// a shared member in a common base -- but no shape with the members there
// reproduces the order the bytes show: a base's member destructs
// *after* the base destructor has already stamped its own vptr, and here the
// lock goes down first. So the members are duplicated, and Free and Term are
// written out twice; the two bodies are identical to the byte and the linker
// folds them, which is exactly what both shipped images show -- one Free
// (0x10007781 / 0x004219c0) and one Term (0x100078e9 / 0x004219f0) reached
// from both banks' vtables (names ours).
class CMemStreamBank : public CBankBase
{
public:
	// 0x10006df7 — the same, with sizeof(CMemStream) (also 0x24).
	// (lithtech 0x0042a540, server.dll 0x1000a52a; initializer 0x00429350)
	__forceinline CMemStreamBank(uint32 nCacheSize, uint32 nPreAllocate)
		{ sb_Init2(&m_Bank, 0x24, nCacheSize, nPreAllocate); }
	~CMemStreamBank();                       // 0x100077b1
	// 0x100075c6, and 0x100075ba folded with the tree bank's.
	virtual void* AllocVoid()               { return Allocate(); }
	virtual void  FreeVoid(void* pStream)   { Free(pStream); }
	void Free(void* pStream);                // 0x10007781, folded
	ILTStream* Allocate();                   // 0x100077d7
	void Term();                             // 0x100078e9, folded

	StructBank       m_Bank;                 // 0x04
	LCriticalSection m_Lock;                 // 0x20
};

// A stream on its way back to its bank is destructed but not freed, and the
// call that does it goes through slot 0 with a zero flag. ILTStream declares
// its destructor `protected`, so the call cannot be written through the SDK
// type; this local declaration has the shape the call site needs and nothing
// else — a vptr at +0 whose slot 0 is a virtual destructor (name ours).
class CStreamForFree
{
public:
	virtual ~CStreamForFree();
};

// The two banks are file-scope objects, not pointers; they are DEFINED below,
// after their constructors, because the shipped initializers have the whole
// constructor expanded into them (no `this` register, the global's address
// folded into every store) and VC6 can only do that where it has already seen
// the body.
extern CStreamBank    g_TreeStreamBank;      // 0x100582e8
extern CMemStreamBank g_MemStreamBank;       // 0x10058320


// 0x100075cb — the tree-stream bank goes away: the chunks go back to the
// allocator and the lock is torn down. The teardown of the lock lands *before*
// the base class's vtable-pointer store, which is what says the critical
// section is this class's own member and not the base's — a base's member
// would destruct after the base destructor had already stamped its vptr.
//
// `inline`, by the same two-image proof the constructor carries: the shipped
// /O2 client has no standalone body for either of these at all -- both are
// expanded into their scalar deleting destructors (0x00421a20 and 0x00421a60)
// AND into the atexit thunks the two file-scope banks register (0x00420a40 and
// 0x00420ac0) -- while the shipped /O1 server keeps the standalone bodies at
// 0x100075cb / 0x100077b1 and CALLS them from both. /Os declines at 38 bytes;
// /Ot accepts.
inline CStreamBank::~CStreamBank()
{
	sb_FreeChunks(&m_Bank, 1);
}

// 0x100077b1 — and the memory-stream bank, identically.
inline CMemStreamBank::~CMemStreamBank()
{
	sb_FreeChunks(&m_Bank, 1);
}


// 0x10007781 — a stream back to its bank, under the bank's own lock. The
// object is destructed in place (`push $0` through the deleting destructor's
// flag, so it destructs without freeing) and the storage then goes straight
// onto the struct bank's intrusive free list. That is why Release is a
// one-liner at /O1: nothing here ever reaches the allocator.
//
// `inline`, and defined above both Releases because VC6's inliner only looks
// backwards: the shipped /O2 client has no call to 0x004219c0 anywhere -- this
// whole body, sb_Free included, is expanded into CTreeStream::Release
// (0x00420b00) and CMemStream::Release (0x00420b30), leaving 0x004219c0 alive
// only as the two vtables' Free slot. The shipped /O1 server calls it from
// both (0x10006e45, 0x10006e51), so /Os declines it at 48 bytes.
inline void CStreamBank::Free(void* pStream)
{
	m_Lock.Enter();

	((CStreamForFree*)pStream)->~CStreamForFree();

	sb_Free(&m_Bank, pStream);

	m_Lock.Leave();
}

// The memory bank's own copy, identical to the byte and folded onto the same
// address in both shipped images.
inline void CMemStreamBank::Free(void* pStream)
{
	m_Lock.Enter();

	((CStreamForFree*)pStream)->~CStreamForFree();

	sb_Free(&m_Bank, pStream);

	m_Lock.Leave();
}


// The record fs_AddResources hands out for one mounted resource tree, as far
// as the streams need it. servermgr.h models the same thing as CFileTree with
// only its +0x04 archive named, and filemgr.cpp calls it COpenTree; the two
// fields the open needs are the kind at +0x00 -- 1 is a plain directory on
// disk, anything else is a .rez archive -- and, for a directory, the path at
// +0x20, which sits just past the critical section fs_CloseTree tears down
// (names ours).
class COpenFileTree
{
public:
	int        m_nKind;                      // +0x00
	CFileList* m_pList;                      // +0x04  the archive
	CRITICAL_SECTION m_CS;                   // +0x08  serializes reads of the
	                                         //        one shared handle
	char       m_szDir[1];                   // +0x20  inline, not a pointer
};

// 0x1003c4a0 — the archive's own name lookup: RezMgr.lib's
// CRezMgr::GetRezFromDosPath (CRezMgrX380 in the library's reconstruction;
// link/aliases.rsp pairs the spellings by address). servermgr.h has no slot
// for the method and this file must not add one, so it is declared on a view
// whose layout starts at the same offset (name ours).
class CFileListView
{
public:
	class CStreamSource* Find(char* pFilename);   // 0x1003c4a0
};


// The class the two file streams actually are. Its existence is not inferred:
// the two Allocates each `call 0x10007628` and then store a DIFFERENT vtable
// pointer into the record, which is a derived class's inlined constructor over
// an out-of-line base constructor, and 0x10007628 stamps a third vtable
// (0x1004d2ac) of its own. That third vtable is this class's: it carries
// ErrorStatus, GetLen and Write filled in, ReadString/WriteStream/WriteString
// from CBaseStream, and _purecall in the four slots the two streams override
// (Release, Read, SeekTo, GetPos). Slot 0 of all three vtables is the SAME
// deleting destructor (0x10007951), which calls 0x10007668 directly — so
// neither stream declares a destructor and both destruct as this class.
//
// The eight members the constructor zeroes are shared, and the two streams
// read some of them differently: +0x08 is where a tree member starts inside
// the archive file for one and the read position for the other, and +0x18 is
// unused by the tree stream and the archive item for the memory one. That is
// the class's own doing and not a union: one constructor clears all eight
// (names ours).
class CStreamFields : public CBaseStream
{
public:
	CStreamFields();                         // 0x10007628
	~CStreamFields();                        // 0x10007668

	LTRESULT ErrorStatus();                  // 0x1000764b
	LTRESULT GetLen(unsigned long* len);     // 0x10007654
	LTRESULT Write(const void* pData, unsigned long size);   // 0x10007662

public:
	uint32  m_nLen;                          // 0x04  the member's length
	uint32  m_nOffset;                       // 0x08  where it starts in the
	                                         //       file / the read position
	FILE*   m_pFile;                         // 0x0c  shared with its siblings
	COpenFileTree* m_pTree;                  // 0x10  the tree it came out of
	int     m_nError;                        // 0x14  sticky
	CStreamSource* m_pSource;                // 0x18  the archive item, if any
	uint32  m_nReads;                        // 0x1c  statistics, printed at close
	uint32  m_nBytesRead;                    // 0x20
};

// A stream over one member of an open resource tree: a window onto a shared
// handle, with the read statistics the debug line prints.
class CTreeStream : public CStreamFields
{
public:
	void     Release();                      // 0x10006e3f
	LTRESULT Read(void* pData, unsigned long size);          // 0x1000770a
	LTRESULT SeekTo(unsigned long offset);   // 0x100076e4
	LTRESULT GetPos(unsigned long* offset);  // 0x100076c9
};


// 0x10007628 — every field cleared, in offset order.
//
// `inline` is not decoration and it is not a guess: the shipped /O2 client has
// this body expanded into both Allocates (0x004218c0 and 0x00421940, where the
// eight zero stores come out of one `xor %edi,%edi` held across the whole
// function), and the shipped /O1 server CALLS it at both of those same sites
// (0x10007611). One source, two outcomes -- which is only possible if the
// function was an inline CANDIDATE and /Os declined it at 35 bytes. So the
// keyword is what the original had, and adding it cannot move server.dll.
inline CStreamFields::CStreamFields()
{
	m_nLen = 0;
	m_nOffset = 0;
	m_pFile = LTNULL;
	m_pTree = LTNULL;
	m_nError = 0;
	m_pSource = LTNULL;
	m_nReads = 0;
	m_nBytesRead = 0;
}

// The last holder of the handle closes it. Everything else about the stream
// is the bank's to reuse.
//
// `inline` on the same evidence: the shipped /O2 client has this body expanded
// into the one deleting destructor all three stream vtables share (0x00421ac0)
// and no standalone copy anywhere, and the shipped /O1 server keeps the
// standalone at 0x10007668 and calls it from 0x10007951.
inline CStreamFields::~CStreamFields()
{
	if (g_StreamDebugLevel >= 2)
	{
		dsi_ServerError("close stream %p num reads = %u bytes read = %u",
		                this, m_nReads, m_nBytesRead);
	}

	if (m_pTree->m_nKind == 1 && m_pFile)
		fclose(m_pFile);
}

void CTreeStream::Release()
{
	g_TreeStreamBank.Free(this);
}

// A stream that has already failed keeps failing, and still clears the
// caller's buffer every time — so a caller that reads a whole structure
// without checking gets zeroes rather than stale bytes.
LTRESULT CTreeStream::Read(void* pData, unsigned long size)
{
	unsigned long nRead;

	if (g_StreamDebugLevel >= 4)
		dsi_ServerError("read %u bytes from stream %p", size, this);

	if (m_nError == 1)
	{
		memset(pData, 0, size);

		return LT_ERROR;
	}

	if (size)
	{
		nRead = fread(pData, 1, size, m_pFile);

		if (nRead == size)
		{
			m_nReads++;
			m_nBytesRead += nRead;
		}
		else
		{
			memset(pData, 0, size);

			m_nError = 1;

			return LT_ERROR;
		}
	}

	return LT_OK;
}

LTRESULT CStreamFields::ErrorStatus()
{
	return m_nError != 0;
}

// Positions are relative to the member, so the base is added going in and
// taken off coming out; nothing above this layer knows the file is shared.
LTRESULT CTreeStream::SeekTo(unsigned long offset)
{
	if (fseek(m_pFile, m_nOffset + offset, SEEK_SET) == 0)
		return LT_OK;

	m_nError = 1;

	return LT_ERROR;
}

LTRESULT CTreeStream::GetPos(unsigned long* offset)
{
	*offset = ftell(m_pFile) - m_nOffset;

	return LT_OK;
}

LTRESULT CStreamFields::GetLen(unsigned long* len)
{
	*len = m_nLen;

	return LT_OK;
}

LTRESULT CStreamFields::Write(const void* pData, unsigned long size)
{
	return LT_ERROR;
}


// Whatever the stream is reading out of. Only one thing is asked of it from
// here: whether a given offset is reachable (names ours).
class CStreamSource
{
public:
	LTBOOL CanSeek(unsigned long offset);    // 0x1003a650
	// 0x1003a660 — read from the member. The third argument is the offset to
	// read from, or all-ones to carry on from wherever the last read left
	// off, which is what the two globals below are for.
	unsigned long Read(void* pData, unsigned long size, unsigned long offset);

	uint8  m_pad_00[0x0c];
	uint32 m_nLen;                           // +0x0c  the member's length
};

// The second kind: the position is its own field rather than the file's, so
// the read comes from somewhere that is not a FILE* at all (names ours).
class CMemStream : public CStreamFields
{
public:
	void     Release();                      // 0x10006e4b
	LTRESULT Read(void* pData, unsigned long size);          // 0x10007843
	LTRESULT SeekTo(unsigned long offset);   // 0x1000781c
	LTRESULT GetPos(unsigned long* offset);  // 0x1000780e
};


void CMemStream::Release()
{
	g_MemStreamBank.Free(this);
}

LTRESULT CMemStream::GetPos(unsigned long* offset)
{
	*offset = m_nOffset;

	return LT_OK;
}

// 0x1005835c is one dword past these two; 0x10058358 and 0x100582e0 are the
// archive layer's own globals, zeroed by fs_InitModule and by every open and
// close (filemgr.cpp names them). What they hold is the last read this file
// did: the item it came out of and the position it ended at. A read that
// carries on from exactly there tells the archive so with an all-ones offset
// instead of a position, which is what lets the archive skip a seek.
extern void* g_pUnknown58358;
extern void* g_pUnknown582e0;

// 0x10007843 — read from an archive member.
//
// The whole read is taken under the TREE's critical section, not the stream's:
// every stream over one .rez file shares one handle, so two streams reading at
// once would fight over the file position. The bookkeeping after it is done
// outside the lock.
//
// SHIPPED BUG: the debug line's format asks for two values and is given one.
// `size` is never pushed, so the "%u" prints whatever follows `this` on the
// stack. The sibling read in CTreeStream::Read passes both.
//
// NEAR on Primal Hunt's lithtech.exe (0x00429c80, 204 of 195): shipped loads
// m_nOffset once before the first test's branch and keeps a single call.
// `if (A && B) Read(-1) else Read(nOffset)` with nOffset read first comes to
// 196 (only the global's register differs) but loses server.dll's match
// (0x1000a641), which this nested form has.
LTRESULT CMemStream::Read(void* pData, unsigned long size)
{
	unsigned long nRead;

	if (g_StreamDebugLevel >= 4)
#ifdef AVP2_MODERN
		dsi_ServerError("read from stream %p size %u", this, size);	// fixed (RETAIL_BUGS.md)
#else
		dsi_ServerError("read from stream %p size %u", this);
#endif

	if (size == 0)
		return LT_OK;

	EnterCriticalSection(&m_pTree->m_CS);

	if (g_pUnknown58358 == m_pSource)
	{
		if ((uint32)g_pUnknown582e0 == m_nOffset)
			nRead = m_pSource->Read(pData, size, 0xffffffff);
		else
			nRead = m_pSource->Read(pData, size, m_nOffset);
	}
	else
	{
		nRead = m_pSource->Read(pData, size, m_nOffset);
	}

	LeaveCriticalSection(&m_pTree->m_CS);

	m_nOffset += nRead;

	g_pUnknown58358 = m_pSource;
	g_pUnknown582e0 = (void*)m_nOffset;

	if (nRead != size)
	{
		memset(pData, 0, size);

		m_nError = 1;

		return LT_ERROR;
	}

	m_nReads++;
	m_nBytesRead += nRead;

	return LT_OK;
}

// The seek is only recorded once the source agrees it can be reached; a
// refusal is what makes the stream fail rather than silently reading from
// the wrong place.
LTRESULT CMemStream::SeekTo(unsigned long offset)
{
	if (m_pSource->CanSeek(offset))
	{
		m_nOffset = offset;

		return LT_OK;
	}

	m_nError = 1;

	return LT_ERROR;
}

// The order the two constructors above show is the order MSVC builds a class
// in: the base's vtable pointer, then the members that have constructors (the
// lock at +0x20 -- the struct bank at +0x04 has none, which is why the body
// has to initialise it), then this class's own vtable pointer, then the body.
// The initializer chain around them is the compiler's: 0x10006d7e calls the
// expanded constructor and tails into 0x10006dd7, which registers 0x10006de3
// (`this` then a jump to the destructor) with atexit.
// Both are `inline`: the shipped initializers reach the global by its own
// address rather than through a `this` register and end without returning
// one, so the constructor bodies are expanded into them and no out-of-line
// copy exists.
CStreamBank    g_TreeStreamBank(8, 8);       // 0x100582e8
CMemStreamBank g_MemStreamBank(8, 8);        // 0x10058320


// 0x100078e9 — start the bank over: whatever it was holding goes back and the
// bank is set up again for records of one stream each, 0x20 at a time. Both
// cdecl calls share one `add esp,0x14`, which is what consecutive
// fixed-prototype calls get.
void CStreamBank::Term()
{
	m_Lock.Enter();

	sb_FreeChunks(&m_Bank, 1);
	sb_Init(&m_Bank, sizeof(CTreeStream), 0x20);

	m_Lock.Leave();
}

// The memory bank's copy: same size of record, same body, folded onto the same
// address. Nothing calls either -- both exist for the vtables' third slot.
void CMemStreamBank::Term()
{
	m_Lock.Enter();

	sb_FreeChunks(&m_Bank, 1);
	sb_Init(&m_Bank, sizeof(CMemStream), 0x20);

	m_Lock.Leave();
}

// 0x100075f1 and 0x100077d7 — one stream out of one bank, under that bank's
// own lock. The bank hands back raw storage, so the stream is constructed in
// place; the base constructor is the call and the derived class's own is the
// vtable-pointer store right after it. A bank that cannot grow returns null
// and nothing is constructed.
//
// `inline`, and above fs_OpenStreamInTree for the inliner's benefit: the
// shipped /O2 client expands both of these into that function (which is why it
// is 501 bytes there against 288 at /O1) and calls neither -- 0x004218c0 and
// 0x00421940 survive only as the vtables' Allocate slot. The shipped /O1
// server calls them at 0x10007181 and 0x100071ca, so /Os declines at 55 bytes.
inline ILTStream* CStreamBank::Allocate()
{
	CTreeStream* pStream;

	m_Lock.Enter();

	pStream = (CTreeStream*)sb_Allocate(&m_Bank);

	if (pStream)
		new(pStream) CTreeStream;

	m_Lock.Leave();

	return pStream;
}

inline ILTStream* CMemStreamBank::Allocate()
{
	CMemStream* pStream;

	m_Lock.Enter();

	pStream = (CMemStream*)sb_Allocate(&m_Bank);

	if (pStream)
		new(pStream) CMemStream;

	m_Lock.Leave();

	return pStream;
}


// 0x1005835c — the running total the timed block below adds to; it sits one
// dword past the archive layer's own two globals (name ours). Zeroed .bss.
uint32 g_Unknown5835c;


// 0x100070e3 — open a stream on one named file inside one mounted tree
// (serverengine.h's name).
//
// The tree's kind decides everything. A plain directory (kind 1) means the
// file really is on disk: the path is built from the tree's own directory,
// opened with fopen, and its length taken by seeking to the end and back. A
// .rez tree means the file is a member of an already-open archive, so the
// archive's name lookup answers with the member record and the length comes
// straight off it -- no file is opened at all, which is why the two kinds
// come out of two different banks.
//
// The fopen alone is wrapped in a timed scope; nothing else in the function
// is measured.
//
// The `push` of the filename is hoisted above the branch by the compiler
// because it is the last argument of both calls -- sprintf's fourth and the
// lookup's only one -- so there is nothing in the source to match it.
//
// Field stores are in lithtech.exe's order (0x00420fb7: +0xc, +0x10, +0x4,
// +0x8 -- the offset zeroed last); /O1 emits the same bytes either way.
//
// NEAR on both AvP2 lithtech.exes (PH 0x00429870, 590 of 639; 1.0.9.6
// 0x00427020, 584 of 544): in the directory arm only, both CALL ILTStream's
// implicit constructor out of line (PH 0x004299a9 -> 0x00429b10, 1.0.9.6
// 0x00427139 -> 0x00427290) where ours expands it, and in PH that call is
// what brings in the new-expression's exception frame (state 0 around it,
// 0x004299a2). The .rez arm expands the same constructor in both images.
// Not yet reproduced.
ILTStream* fs_OpenStreamInTree(COpenFileTree* pTree, char* pFilename, int nFlag)
{
	char   szPath[500];
	FILE*  pFile;
	uint32 nLen;

	if (!pTree)
		return LTNULL;

	if (pTree->m_nKind == 1)
	{
		CTreeStream* pStream;

		// AVP2: joined with a forward slash when the name has one (PH
		// 0x004298b9 strstr, 0x004298ce "%s/%s"), as fs_GetRawFileInfo.
		if (strstr(pFilename, "/"))
			sprintf(szPath, "%s/%s", pTree->m_szDir, pFilename);
		else
			sprintf(szPath, "%s\\%s", pTree->m_szDir, pFilename);

		{
			// AVP2: the client times the fopen as well (PH 0x004298f1
			// CBaseCounter(0), 0x00429903 cnt_StartCounter, 0x0042991e
			// cnt_EndCounter added to the total, AvP2 0x0042708e..0x004270c9).
			CountAdder counter(&g_Unknown5835c);

			pFile = fopen(szPath, "rb");
		}

		if (!pFile)
			return LTNULL;

		fseek(pFile, 0, SEEK_END);
		nLen = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		pStream = (CTreeStream*)g_TreeStreamBank.Allocate();

		pStream->m_pFile = pFile;
		pStream->m_pTree = pTree;
		pStream->m_nLen = nLen;
		pStream->m_nOffset = 0;

		if (g_StreamDebugLevel >= 1)
		{
			dsi_ServerError("stream %p open file %s size = %u",
			                pStream, pFilename, nLen);
		}

		return pStream;
	}
	else
	{
		CStreamSource* pItem;
		CMemStream*    pStream;

		pItem = ((CFileListView*)pTree->m_pList)->Find(pFilename);

		if (!pItem)
			return LTNULL;

		nLen = pItem->m_nLen;

		pStream = (CMemStream*)g_MemStreamBank.Allocate();

		pStream->m_pSource = pItem;
		pStream->m_pTree = pTree;
		pStream->m_nLen = nLen;
		pStream->m_nOffset = 0;

		if (g_StreamDebugLevel >= 1)
		{
			dsi_ServerError("stream %p open rez %s size = %u",
			                pStream, pFilename, nLen);
		}

		return pStream;
	}
}


// 0x100074df — drain a stream into a named file on disk (name ours;
// serverengine.h declares it for fs_CopyFile, its only reconstructed caller).
//
// A stream that has already failed is not copied at all, and neither is a
// null one. The copy runs in 0x4000-byte blocks through one temporary
// allocation, and the block size is a *variable* rather than a literal: the
// shipped code puts 0x4000 in ebx once and uses the register both for the
// allocation and for the per-block clamp, where a literal would push and
// compare the immediate twice.
//
// The buffer comes from ::operator new (0x1000d25d, a one-argument forwarder
// in front of dalloc at 0x1000d2ef) and goes back through ::operator delete
// (0x10028ace): those are the two addresses the shipped calls reach. Calling
// dalloc directly compiles to the same bytes -- the displacement is masked --
// and names the wrong callee.
//
// Success is not the loop finishing but the offset having reached the length,
// which is what makes a stream that failed mid-copy report failure even
// though the file is left behind on disk with whatever arrived. The file and
// the buffer are released either way.
//
// The guards are NESTED POSITIVE tests with one `return 0` after them, and
// the allocation test is `if (!pBuf) fclose; else { copy }`. The lithtech
// twin (0x00421670) says so: all four failures land on ONE `pop; xor; ret`
// block there, where four early `return 0`s gave each its own copy, and the
// server build put the epilogue on the success path instead of the zero exit
// (whose old comment here blamed the compiler; it was the source shape).
// Byte-identical on both images.
int fs_CopyStreamToFile(ILTStream* pStream, const char* pszDestFile)
{
	FILE*         pFile;
	unsigned long nLen;
	unsigned long nPos;
	unsigned long nBlock;
	unsigned long nCopy;
	char*         pBuf;

	if (pStream && pStream->ErrorStatus() == LT_OK)
	{
		pFile = fopen(pszDestFile, "wb");

		if (pFile)
		{
			nBlock = 0x4000;

			pBuf = (char*)::operator new(nBlock);

			if (!pBuf)
			{
				fclose(pFile);
			}
			else
			{
				pStream->SeekTo(0);

				nPos = 0;

				pStream->GetLen(&nLen);

				while (nPos < nLen)
				{
					nCopy = nLen - nPos;

					if (nCopy > nBlock)
						nCopy = nBlock;

					pStream->Read(pBuf, nCopy);

					if (pStream->ErrorStatus() != LT_OK)
						break;

					if (fwrite(pBuf, nCopy, 1, pFile) != 1)
						break;

					nPos += nCopy;
				}

				::operator delete(pBuf);
				fclose(pFile);

				return (nPos == nLen);
			}
		}
	}

	return 0;
}


#ifndef LT_SERVER_BUILD
// 0x00421750 (lithtech.exe only; server.dll has no twin, so the whole body
// is inside the guard) -- where a named file's bytes really live: the file
// to open, the offset into it and the length (sndinst2.cpp's caller reads
// the offset; names ours). A directory tree answers with the joined path
// itself (opened once with fopen only to measure it, offset 0); a .rez tree
// answers with the archive's own file name and the member's position and
// size inside it (rezmgr.h's CRezItm::DirectRead_GetFullRezName, called
// TWICE -- the `+0x34` null test and the slot-8 call are expanded at both
// 0x00421832 and 0x0042185e -- DirectRead_GetFileOffset at +0x14 and
// GetSize at +0x0c). Either way the name is copied bounded and terminated,
// and the answer is whether it fitted. The rez arm's fit test is written
// `nOutNameLen <= strlen(..)`: shipped compares with the length on the left
// (`cmp %ecx,%ebp ; ja`), which `strlen(..) >= nOutNameLen` reverses.
class CRawRezFileView
{
public:
	virtual void        _v00();
	virtual void        _v04();
	virtual void        _v08();
	virtual void        _v0c();
	virtual void        _v10();
	virtual void        _v14();
	virtual void        _v18();
	virtual void        _v1c();
	virtual const char* GetFileName();       // +0x20
};

class CRawRezItmView
{
public:
	uint32      GetSize() { return m_nSize; }
	const char* DirectRead_GetFullRezName()
	{
		if (m_pRezFile == LTNULL)
			return LTNULL;
		else
			return m_pRezFile->GetFileName();
	}
	uint32      DirectRead_GetFileOffset() { return m_nFilePos; }

	uint8            m_pad_00[0x0c];
	uint32           m_nSize;                // +0x0c
	uint8            m_pad_10[0x14 - 0x10];
	uint32           m_nFilePos;             // +0x14
	uint8            m_pad_18[0x34 - 0x18];
	CRawRezFileView* m_pRezFile;             // +0x34
};

int fs_GetRawFileInfo(COpenFileTree* pTree, char* pFilename, char* pOutName,
                      uint32 nOutNameLen, uint32* pPos, uint32* pSize)
{
	char            szPath[500];
	FILE*           pFile;
	CRawRezItmView* pItem;

	if (!pTree)
		return 0;

	if (pTree->m_nKind == 1)
	{
		// AVP2: a name that already uses forward slashes is joined with one
		// (PH 0x0042a234 strstr(pFilename, "/"), "%s/%s" at 0x0042a249).
		if (strstr(pFilename, "/"))
			sprintf(szPath, "%s/%s", pTree->m_szDir, pFilename);
		else
			sprintf(szPath, "%s\\%s", pTree->m_szDir, pFilename);

		pFile = fopen(szPath, "rb");
		if (!pFile)
			return 0;

		fseek(pFile, 0, SEEK_END);
		*pSize = ftell(pFile);
		*pPos = 0;
		fclose(pFile);

		strncpy(pOutName, szPath, nOutNameLen);
		pOutName[nOutNameLen - 1] = 0;

		return strlen(szPath) < nOutNameLen;
	}
	else
	{
		pItem = (CRawRezItmView*)((CFileListView*)pTree->m_pList)->Find(pFilename);
		if (!pItem)
			return 0;

		strncpy(pOutName, pItem->DirectRead_GetFullRezName(), nOutNameLen);
		pOutName[nOutNameLen - 1] = 0;

		if (nOutNameLen <= strlen(pItem->DirectRead_GetFullRezName()))
			return 0;

		*pPos = pItem->DirectRead_GetFileOffset();
		*pSize = pItem->GetSize();

		return 1;
	}
}
#endif
