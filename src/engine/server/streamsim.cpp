// server.dll — the CRT-file ILTStream (reconstruction). Build /O1.
//
// The simplest of the engine's four ILTStream implementations: a FILE* and a
// sticky error flag, twelve bytes in all. The interface is the SDK's — the
// vtable at 0x1004e920 lands on iltstream.h's eleven virtuals in declaration
// order, and three of its slots (ReadString, WriteStream, WriteString) point
// at the same generic implementations every other stream in the image uses,
// which is what confirms the anchoring.
//
// The error flag is only ever set, never cleared: once a read runs short the
// stream stays failed, which is what makes the SDK's advice work — read
// everything without checking, then ask ErrorStatus once at the end. Reads
// that fail also zero the caller's buffer, exactly as the header promises,
// so a partly-read structure is all-zeroes rather than half-garbage.
//
// The class name and the file name are ours; every method name is the SDK's.
//
// The whole run 0x10035a8d-0x10035c27 is one translation unit's worth of
// contiguous code, which is what places this in a file of its own.
#include <stdio.h>
#include <string.h>
#include "servermgr.h"
#include "basestream.h"
#include "servermsg.h"


class CFileStream : public CBaseStream
{
public:
	CFileStream() { m_nError = 0; m_pFile = LTNULL; }

	// Inline, and the shipped image says so: there is no separate
	// destructor body, only the deleting form with the close folded in.
	~CFileStream() { if (m_pFile) fclose(m_pFile); }   // 0x10035bf9

	void     Release();                      // 0x100241cc
	LTRESULT Read(void* pData, unsigned long size);          // 0x10035acc
	LTRESULT ErrorStatus();                  // 0x10035b51
	LTRESULT SeekTo(unsigned long offset);   // 0x10035b5a
	LTRESULT GetPos(unsigned long* offset);  // 0x10035b7e
	LTRESULT GetLen(unsigned long* len);     // 0x10035bab
	LTRESULT Write(const void* pData, unsigned long size);   // 0x10035b15

public:
	FILE* m_pFile;                           // 0x04
	int   m_nError;                          // 0x08  sticky; set, never cleared
};


// 0x10035a8d — open a file by name and wrap the handle. A file that will not
// open produces no stream at all, so a caller that gets one always has a
// handle; an allocation that fails produces nothing either, and the handle is
// simply leaked rather than closed (name ours).
ILTStream* streamsim_Open(char* pFilename, char* pAccess)
{
	CFileStream* pStream;
	FILE*        pFile;

	pFile = fopen(pFilename, pAccess);

	if (!pFile)
		return LTNULL;

	pStream = new CFileStream;

	if (pStream)
		pStream->m_pFile = pFile;
#ifdef AVP2SRC_MODERN
	else
		fclose(pFile);	// fixed: the file is not leaked (RETAIL_BUGS.md)
#endif

	return pStream;
}

// A stream with no handle fails everything. A zero-length read is not a
// failure and does not touch the buffer.
//
// NEAR (73 of 73 bytes): every instruction and register agrees. The two
// success exits are shared in both builds; the shipped one puts the shared
// block at its first reference, right after the length test, and ours puts
// it at its second, after the read. Four spellings of the three exits move
// it around without landing on the shipped order.
//
// Ten spellings now, and the shape is worth writing down because it is the
// one thing that separates these two from sub_100070b6 in filemgr.cpp, which
// closed on exactly this question. There, two ADJACENT guarded `return 0`
// statements put the merged block between the two tests and inverted the
// second test's polarity; here the two identical returns have the read
// between them, and VC6 hosts the merged block at the SECOND site no matter
// how the source is written. Probed and rejected, on both Read and Write:
// the second test inverted so the failure is the `if` body; `else if` on the
// second test with either polarity; the pair wrapped in a positive
// `if (m_pFile) { ... }` with one trailing `return LT_ERROR`; an empty first
// arm with the failure in the `else if` (which sinks the LT_OK block to the
// end instead); and `if (size == 0 || fread(...) == size) return LT_OK;`,
// which sinks it the same way -- the documented `||` sinking. The shipped
// order needs the merged block hosted at the FIRST site and nothing reaches
// it.
LTRESULT CFileStream::Read(void* pData, unsigned long size)
{
	if (!m_pFile)
		return LT_ERROR;
	else if (size == 0)
		return LT_OK;
	else if (fread(pData, 1, size, m_pFile) == size)
		return LT_OK;
	else
	{
		memset(pData, 0, size);

		m_nError = 1;

		return LT_ERROR;
	}
}

// The same shape as the read, without the buffer clear: there is no caller
// buffer to leave in a known state — and NEAR (60 of 60) for the same
// reason.
LTRESULT CFileStream::Write(const void* pData, unsigned long size)
{
	if (!m_pFile)
		return LT_ERROR;
	else if (size == 0)
		return LT_OK;
	else if (fwrite(pData, 1, size, m_pFile) == size)
		return LT_OK;
	else
	{
		m_nError = 1;

		return LT_ERROR;
	}
}

LTRESULT CFileStream::ErrorStatus()
{
	return m_nError != 0;
}

LTRESULT CFileStream::SeekTo(unsigned long offset)
{
	if (!m_pFile)
		return LT_ERROR;

	return fseek(m_pFile, offset, SEEK_SET) != 0;
}

// A stream with no handle answers zero rather than leaving the caller's
// variable alone, so the value is always defined.
LTRESULT CFileStream::GetPos(unsigned long* offset)
{
	if (!m_pFile)
	{
		*offset = 0;
		return LT_ERROR;
	}

	*offset = ftell(m_pFile);

	return *offset == 0xffffffff;
}

// The position is saved and restored around the seek to the end, so asking
// for the length does not move the stream.
LTRESULT CFileStream::GetLen(unsigned long* len)
{
	unsigned long nPos;

	if (!m_pFile)
	{
		*len = 0;
		return LT_ERROR;
	}

	nPos = ftell(m_pFile);

	fseek(m_pFile, 0, SEEK_END);
	*len = ftell(m_pFile);
	fseek(m_pFile, nPos, SEEK_SET);

	return LT_OK;
}

// 0x100241cc — `delete this`, and nothing else: eleven bytes of null test,
// vptr load, `push $1` and an indirect call through slot 0. That is VC6's
// expansion of `delete p` when the destructor is virtual — slot 0 of every
// ILTStream vtable holds the *deleting* destructor `??_G`, whose hidden flag
// argument is the 1 being pushed.
//
// The address is nowhere near the rest of this file (0x10035a8d-0x10035c27).
// It does not have to be: the body reaches the destructor through the
// object's own vptr, so it compiles to the same eleven bytes for every class
// with a virtual destructor, and the linker folds all of those COMDATs into
// one. Which of the identical copies the image kept is not information about
// which file this one came from.
void CFileStream::Release()
{
	delete this;
}


// ---- CAbstractIO's refcount pair (0x100241bd, 0x100241c1) ---------------- //
//
// These are not this module's: they belong to servermsg.h's CAbstractIO, and
// their home is servermsg.cpp. They are defined here rather than there
// — the address they match at does not depend on the
// translation unit, and their neighbour above (0x100241cc) shows why the
// address proximity is no evidence of a home either.
//
// Both are read straight off the bytes. AddRef is `incl 0x4(%ecx)` and
// nothing else, on the same word ServerMessage's constructor clears through
// `andl $0x0,0x4(%esi)` — which is what puts the count on this class rather
// than on the derived one. Release decrements and tests in one instruction
// (`decl` then `jne`), so the source is `--m_nRefCount == 0` in the
// condition, and the destroy is a TAIL JUMP through vtable slot 3
// (`jmp *0xc(%eax)`) rather than a call, because nothing follows it.
//
// Slot 3 is the destroy: for the base's own table (0x1004dea0) it is
// 0x100241cc, the eleven-byte `delete this` above.
//
// The shipped vtables point at these two addresses from both tables
// (0x1004dea0 and ServerMessage's 0x1004de00), so ServerMessage inherits
// them rather than re-declaring them (servermsg.h).
void CAbstractIO::AddRef()
{
	m_nRefCount++;
}

void CAbstractIO::Release()
{
	if (--m_nRefCount == 0)
		_v0c();
}


// ---- the module either side ---------------------------------------------- //

// 0x10058bf8 and 0x10058c04 — the list every stream this module hands out is
// threaded onto, and how many callers have asked for the module (names ours).
// The run above was read as ending at 0x10035c27; these two carry it to
// 0x10035c80, so they are the same translation unit's tail.
// stringbank.cpp threads every string record onto this same head (its
// g_StringList, 0x10058bf8): one object, defined here beside its reference
// count, so the tie-off below is the one sb_EnumStrings later walks.
LTLink g_StreamList;
static uint32 g_nStreamRefs;

// 0x10035c2a and 0x10035c49 — reference counted, and the first caller in ties
// the list off rather than trusting the .bss zeroes: a zeroed head is not an
// empty circular list, it is a broken one.
//
// TieOff is the SDK's `m_pPrev = m_pNext = this`, which assigns right to left,
// so the next pointer is stored before the prev one -- exactly the order the
// shipped code writes 0x10058bfc then 0x10058bf8 in, at both call sites.
void stream_Init()
{
	if (g_nStreamRefs == 0)
		g_StreamList.TieOff();

	g_nStreamRefs++;
}

// The drain walks the list through m_pPrev, not m_pNext -- backwards from the
// head, so the most recently added stream is released first. Each node's own
// link is read before its data is freed, because the free takes the whole
// record the link sits in.
void stream_Term()
{
	if (--g_nStreamRefs == 0)
	{
		LTLink* pHead = &g_StreamList;
		LTLink* pLink = pHead->m_pPrev;

		while (pLink != pHead)
		{
			LTLink* pPrev = pLink->m_pPrev;

			dfree(pLink->m_pData);

			pLink = pPrev;
		}

		pHead->TieOff();
	}
}
