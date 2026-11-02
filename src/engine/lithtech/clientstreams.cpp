// *********************************************************************** //
//
//  lithtech.exe — 0x00483430..0x004869a0, the CLIENT-ONLY functions.
//
//  The range is five subsystems, not three, and three of the five already
//  have source in the repository. What is left, and what is written here:
//
//    * 0x00484170..0x004846d0 — TWO memory-backed ILTStream implementations
//      that server.dll does not have. Both derive from basestream.h's
//      CBaseStream (their vtables' ReadString/WriteStream/WriteString slots
//      are its three shared bodies at 0x004337f0/0x00433920/0x004338e0, and
//      the growable one's constructor stores CBaseStream's own vtable
//      0x004a374c before storing its own). One wraps a caller's fixed
//      buffer; the other owns a growable one, and owns it as a MEMBER of type
//      collection.h's CMoArray<uint8> — which is the single fact that
//      unlocks the whole cluster (see CGrowStream below).
//
//    * 0x00484780 — the string-resource formatter behind HSTRING. Named by
//      its two error strings and pinned by LoadStringA + FormatMessageA.
//
//    * 0x00484a30 — the string bank's character-count getter, the one
//      stringbank.cpp is missing.
//
//    * 0x00484a80 — tcp_BuildSockaddrFromString. Its own error string names
//      it; src/engine/server/tcpdriver.cpp DECLARES it (as tcp_ParseAddr,
//      0x10035efb) and does not define it.
//
//  THE FILE IS A SCRATCH DELIVERABLE, NOT A DROP-IN TU. Two of its four
//  free functions have a home that already exists and must MOVE there
//  rather than be compiled alongside it:
//
//      sb_GetStringLen                 -> src/engine/server/stringbank.cpp
//      tcp_BuildSockaddrFromString     -> src/engine/server/tcpdriver.cpp
//                                         (rename its tcp_ParseAddr decl)
//
//  The two stream classes and str_GetMessage are client-only and belong in
//  a new src/engine/lithtech/ TU.
//
//  Every name marked "(name ours)" is invented; everything else is the
//  SDK's or is read off a shipped string literal.
//
// *********************************************************************** //
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ltbasedefs.h"
#include "basestream.h"
#include "collection.h"


// ----------------------------------------------------------------------- //
//  What this run calls but does not define
// ----------------------------------------------------------------------- //

// src/engine/lithtech/stdlithalloc.cpp and clientconsole.cpp name these
// three at exactly these addresses.
void  con_ConsolePrint(char* pFormat, ...);                 // 0x0042e6f0
void  ci_DebugOut(char* pMsg, ...);                         // 0x004297d0

// 0x004b32d8 — the console flag that gates the two string-resource
// complaints below: the "DebugStrings" binding, which servermessage.cpp's
// str_FormatMessage reads at the same address and dsi.cpp defines.
extern int g_CV_DebugStrings;

// src/engine/server/stringbank.cpp's record, repeated here for the one
// getter that file is missing. The layout is that file's, unchanged: the
// two 16-bit counts at +0x10 and +0x12 are the CHARACTER count and the BYTE
// count, and sb_GetStringDataAndLen (0x00484a50) hands back the one at
// +0x12. The getter below hands back the other.
class CStringRec
{
public:
	CStringRec* m_pNext;                     // 0x00
	CStringRec* m_pPrev;                     // 0x04
	CStringRec* m_pSelf;                     // 0x08
	uint32      m_RefCount;                  // 0x0c
	uint16      m_nChars;                    // 0x10
	uint16      m_Len;                       // 0x12
	char        m_Data[1];                   // 0x14
};


// ======================================================================= //
//  0x00484a30 — the string bank's character count
// ======================================================================= //
//
//  HOME: src/engine/server/stringbank.cpp, next to sb_GetStringData
//  (0x00484a20) and sb_GetStringDataAndLen (0x00484a50), which it sits
//  between. Callers at 0x00488c4c, 0x00488cec, 0x00488dd7 and 0x00489478.
//
//  Eighteen bytes with NO relocations at all, so the oracle can settle it
//  outright.
//
//  Two things off the bytes. The null answer is a bare `ret` with no `xor`:
//  the loaded pointer IS the zero, which is what `return 0` compiles to when
//  the tested value is already in eax. And the count is loaded `mov
//  0x10(%eax),%cx` into a pre-zeroed ecx, i.e. a uint16 member widened to
//  the uint32 return — not a uint32 member (name ours).
uint32 sb_GetStringLen(HSTRING hString)
{
	if (!hString)
		return 0;

	return ((CStringRec*)hString)->m_nChars;
}


// ======================================================================= //
//  0x00484780 — one formatted string out of the resource module
// ======================================================================= //
//
//  Pinned by the imports and by both literals: USER32!LoadStringA with a
//  5000-byte buffer, then KERNEL32!FormatMessageA with dwFlags 0x500 =
//  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING, and
//  "Couldn't get string %d" / "FormatMessage error on string %d" on the two
//  failures. The caller at 0x004079b4 shows the whole idiom — this, then
//  sb_CreateStringRaw (0x00484850), then str_FreeMessage (0x00484840),
//  which is stringbank.cpp's LocalFree wrapper and is exactly what
//  ALLOCATE_BUFFER obliges (names ours; str_FreeMessage's is the repo's).
//
//  Read off the bytes:
//
//    * the first parameter is DEREFERENCED (`mov (%ecx),%edx`) to get the
//      HINSTANCE, so it is a pointer to a record whose first dword is the
//      module handle, not the handle itself. The caller loads it from
//      g_pClientMgr+0x614 and, at its second site, +0x610 — two resource
//      modules tried in turn.
//    * the third parameter goes straight into FormatMessage's `Arguments`,
//      so it is a va_list*. The caller builds it as one stack slot holding
//      the address of another, which is what a one-argument %1 insert needs.
//    * `*pLen = 0` is stored BEFORE LoadString, not on the way out.
//    * the returned length is `nChars * 3 + 1` (`lea 0x1(%eax,%eax,2)`) —
//      three bytes per character plus a terminator, the lead-byte-set
//      allowance sb_CreateStringRaw's character/byte split exists for.
//    * frame 0x138c, laid out FIRST-DECLARED-HIGHEST: the 5000-byte buffer
//      occupies the top of the frame and the four-byte pointer the bottom,
//      so the buffer is declared first (the unsettled-frame case,
//      not the /O1 scalars-first rule).
//    * the two `return LTNULL`s are separate statements. The second one's
//      quiet path cross-jumps back into the first one's epilogue
//      (0x0048480f -> 0x004847d6), which is folding, not one shared return.
class CResModule
{
public:
	HINSTANCE m_hInst;                       // 0x00
};

char* str_GetMessage(CResModule* pModule, uint32 nStringID, void* pArgs,
                     uint32* pLen)
{
	char   szFormat[5000];
	char*  pMessage;
	uint32 nChars;

	*pLen = 0;

	if (!::LoadString(pModule->m_hInst, nStringID, szFormat, sizeof(szFormat)))
	{
		if (g_CV_DebugStrings)
			con_ConsolePrint("Couldn't get string %d", nStringID);

		return LTNULL;
	}

	nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
	                         FORMAT_MESSAGE_FROM_STRING,
	                         szFormat, 0, 0, (char*)&pMessage, 1,
	                         (va_list*)pArgs);

	if (!nChars)
	{
		if (g_CV_DebugStrings)
			con_ConsolePrint("FormatMessage error on string %d", nStringID);

		return LTNULL;
	}

	*pLen = nChars * 3 + 1;

	return pMessage;
}


// ======================================================================= //
//  0x00484a80 — "a.b.c.d[:port]" or "host[:port]" to a sockaddr_in
// ======================================================================= //
//
//  MOVED to src/engine/server/tcpdriver.cpp, which already DECLARED it
//  (under the placeholder name tcp_ParseAddr, now renamed) and calls it
//  from StartQuery.  It had to move: this file is in lithtech.exe's source
//  list ONLY, so server.dll's 0x10035efb (263 bytes) was never compiled for
//  that image at all while the body lived here.  tcpdriver.cpp is built
//  into BOTH, so lithtech.exe keeps 0x00484a80.


// ======================================================================= //
//  CClientMemStream — the fixed-buffer ILTStream (vtable 0x004a3778)
// ======================================================================= //
//
//  Twenty bytes, and the factory at 0x00484580 proves every one of them:
//  `operator new(0x14)`, then the vptr, then the first argument to +0x04,
//  the second to +0x0c, and zeros to +0x08 and +0x10. The two zeros come
//  last because they reuse the register `new`'s null test had already
//  zeroed — the documented store-order effect — so declaration
//  order is m_pData, m_nPos, m_nLen, m_nError and the ASSIGNMENT order in
//  the constructor is data, length, then the two zeros.
//
//  The vtable is eleven slots at 0x004a3778, iltstream.h's declaration
//  order, with slots 3/8/10 pointing at CBaseStream's shared
//  ReadString/WriteStream/WriteString (0x004337f0/0x00433920/0x004338e0)
//  and slot 0 at 0x00421aa0 — the same address CBaseStream's own vtable
//  (0x004a374c) carries, i.e. this class's generated destructor folded with
//  the base's under /OPT:ICF. Slot 6 (GetPos) is 0x00405490, outside this
//  range and likewise folded; it is written below because the class needs
//  it.
//
//  Every method name is the SDK's; the class name and m_pData/m_nPos/
//  m_nLen/m_nError are ours. (Not "CMemStream": that is fsstream.cpp's
//  server stream class, a different shipped class also linked into
//  lithtech.exe, 0x004211d0 and friends.)
class CClientMemStream : public CBaseStream
{
public:
	CClientMemStream(void* pData, uint32 nLen)
	{
		m_pData  = (unsigned char*)pData;
		m_nLen   = nLen;
		m_nPos   = 0;
		m_nError = 0;
	}

	void     Release();                                      // 0x00455c20
	LTRESULT Read(void* pData, unsigned long size);          // 0x004845b0
	LTRESULT ErrorStatus();                                  // 0x00484690
	LTRESULT SeekTo(unsigned long offset);                   // 0x004846a0
	LTRESULT GetPos(unsigned long* offset);                  // 0x00405490
	LTRESULT GetLen(unsigned long* len);                     // 0x004846c0
	LTRESULT Write(const void* pData, unsigned long size);   // 0x00484630

public:
	unsigned char* m_pData;                  // 0x04
	uint32         m_nPos;                   // 0x08
	uint32         m_nLen;                   // 0x0c
	uint32         m_nError;                 // 0x10
};


// 0x00484580 — wrap a caller's buffer. `new`'s null test and the zero
// return are VC6's, not the source's: the only statement is the new.
// The caller at 0x0047cdb0 takes the result as an ILTStream* (name ours).
ILTStream* memstream_Open(void* pData, uint32 nLen)
{
	return new CClientMemStream(pData, nLen);
}

// A read that would run past the end is not clipped: the position is parked
// at the end, the sticky flag is set and the caller's buffer is zeroed —
// exactly the contract iltstream.h documents and streamsim.cpp's CFileStream
// honours the same way.
//
// The two LT_OK returns do NOT share a block here (each epilogue pops a
// different register set), so they are written as they read.
LTRESULT CClientMemStream::Read(void* pData, unsigned long size)
{
	if (size == 0)
		return LT_OK;

	if (m_nPos + size > m_nLen)
	{
		m_nPos   = m_nLen;
		m_nError = 1;

		memset(pData, 0, size);

		return LT_ERROR;
	}

	memcpy(pData, &m_pData[m_nPos], size);

	m_nPos += size;

	return LT_OK;
}

// The write half, and the asymmetry is in the bytes: an overrun neither
// sets the sticky flag nor moves the position, it just refuses.
LTRESULT CClientMemStream::Write(const void* pData, unsigned long size)
{
	if (size == 0)
		return LT_OK;

	if (m_nPos + size > m_nLen)
		return LT_ERROR;

	memcpy(&m_pData[m_nPos], pData, size);

	m_nPos += size;

	return LT_OK;
}

// 0x00484690 — the eleven-byte `setne` shape streamsim.cpp's CFileStream
// gives for the same source at 0x00484050.
LTRESULT CClientMemStream::ErrorStatus()
{
	return m_nError != 0;
}

// 0x004846a0 — seeking exactly to the end is legal; past it is not, and it
// does not move the position.
LTRESULT CClientMemStream::SeekTo(unsigned long offset)
{
	if (offset > m_nLen)
		return LT_ERROR;

	m_nPos = offset;

	return LT_OK;
}

LTRESULT CClientMemStream::GetPos(unsigned long* offset)
{
	*offset = m_nPos;

	return LT_OK;
}

// 0x004846c0 — fourteen bytes, and the SAME fourteen bytes as
// CGrowStream::GetLen below, because both read a length at +0x0c. The
// linker folds them; both are written out in full, because aiming the
// second vtable slot at the first function is not the same source
// (/OPT:ICF).
LTRESULT CClientMemStream::GetLen(unsigned long* len)
{
	*len = m_nLen;

	return LT_OK;
}

void CClientMemStream::Release()
{
	delete this;
}


// ======================================================================= //
//  CGrowStream — the growable-buffer ILTStream (vtable 0x004a3720)
// ======================================================================= //
//
//  THIS IS THE KEY FACT IN THE WHOLE RANGE. The class is thirty-two bytes
//  and its middle twenty are collection.h's CMoArray<uint8> — as a
//  MEMBER at +0x04, not (as this file first had it) a second base. The
//  ??_G at 0x004843e0 settles that: it addresses the array's fields straight
//  off `this` with no `neg/sbb/and` null test, which cl 8447 emits for
//  every derived-to-base conversion at a non-zero offset; as a base, ours
//  was 102 bytes against 90. As a member both 0x004843e0 and the
//  constructor inside growstream_Open (0x00484170) became identical.
//  The evidence below for the array itself still stands:
//
//    * the constructor at 0x00484170 stores 0x004a374c (CBaseStream's
//      vtable) at +0x00 and 0x004a1ab8 at +0x04, then 0x004a3720 at +0x00.
//      Two vptrs, base-then-derived order (a polymorphic member is
//      constructed after the base and before the derived vptr store).
//    * 0x004a1ab8 is already known to be a collection array's vtable —
//      src/engine/lithtech/cltcursor.cpp records it as the second vptr of
//      the 0x004a1ab4 class, "a three-or-four-dword array header ... whose
//      teardown calls 0x004152b0 ... i.e. one of collection.cpp's arrays".
//    * the four dwords after that vptr are written, in the constructor, in
//      the order m_pArray, m_nElements, SetWantedCache, SetCacheSize —
//      which is collection.h's _InitArray, member for member.
//    * the allocation is `0x0040c060(&g_DefAlloc, 0, n)` and the release
//      `0x004152b0(&g_DefAlloc, p, n)`: three arguments, the middle one a
//      literal zero that is never read. That is collection.h's
//      BaseNew<T>(LAlloc*, T* dummy, uint32) / BaseDelete<T>, whose dummy
//      parameter exists precisely to carry the type. g_DefAlloc is at
//      0x004b4e88 in this image.
//    * the resize inlined into Write is InternalNiceSetSize with bFast set,
//      arm for arm: shrink-into-cache, grow-within-cache, the equal case,
//      then allocate / LTMIN copy / memcpy / _DeleteAndDestroyArray /
//      reassign. The third arm's re-test of equality is the tail-merge of
//      the capacity-check failure, which is what makes the three tests
//      GUARDS rather than an if/else-if chain.
//
//  So m_nElements is the stream's LENGTH and m_pArray its buffer, and the
//  stream adds only a position and a sticky error word:
//
//      +0x00  vptr                     CBaseStream
//      +0x04  vptr                     CMoArray<uint8> (0x004a1ab8)
//      +0x08  m_pArray
//      +0x0c  m_nElements              == the stream length
//      +0x10  m_Cache.m_CacheSize
//      +0x14  m_Cache.m_WantedCache    == the grow-by chunk
//      +0x18  m_nPos                   (ours)
//      +0x1c  m_nError                 (ours)
//
//  Vtable 0x004a3720, eleven slots, iltstream.h order:
//      0  0x004843e0  ??_GCGrowStream
//      1  0x00455c20  Release          (the shared `delete this`)
//      2  0x00484200  Read
//      3  0x004337f0  ReadString       (CBaseStream's)
//      4  0x004843a0  ErrorStatus
//      5  0x004843b0  SeekTo
//      6  0x004843d0  GetPos
//      7  0x004846c0  GetLen           (folded with CClientMemStream's)
//      8  0x00433920  WriteStream      (CBaseStream's)
//      9  0x00484280  Write
//     10  0x004338e0  WriteString      (CBaseStream's)
//
//  Class name and the two added members are ours; everything else is the
//  SDK's or collection.h's.
class CGrowStream : public CBaseStream
{
public:
	// 0x00484170 and 0x00484440 both store the chunk to +0x14 AFTER the
	// derived vptr store, i.e. as a statement of this constructor rather
	// than through the CMoArray(cacheSize) constructor — which is also why
	// the base's own _InitArray writes a ZERO there first.
	CGrowStream(uint32 nWantedCache)
	{
		m_Array.m_Cache.SetWantedCache(nWantedCache);

		m_nPos   = 0;
		m_nError = 0;
	}

	void     Release();                                      // 0x00455c20
	LTRESULT Read(void* pData, unsigned long size);          // 0x00484200
	LTRESULT ErrorStatus();                                  // 0x004843a0
	LTRESULT SeekTo(unsigned long offset);                   // 0x004843b0
	LTRESULT GetPos(unsigned long* offset);                  // 0x004843d0
	LTRESULT GetLen(unsigned long* len);                     // 0x004846c0
	LTRESULT Write(const void* pData, unsigned long size);   // 0x00484280

public:
	CMoArray<uint8> m_Array;                 // 0x04
	uint32 m_nPos;                           // 0x18
	uint32 m_nError;                         // 0x1c
};


// 0x00484170 — build an empty growable stream with the given chunk size.
// Callers at 0x0041c69d and 0x0042c252 (name ours).
//
// IDENTICAL (129 bytes) since m_Array became a member. The body is one
// `new`, but 129 of its bytes are the
// CMoArray constructor's `Clear(); Init();` expanded with Init's own Term()
// left as the out-of-line 0x0040bfe0 and its trailing SetSize(0) inlined and
// constant-folded. Whether VC6 splits it the same way here is a build
// question, not a source one.
ILTStream* growstream_Open(uint32 nWantedCache)
{
	return new CGrowStream(nWantedCache);
}

// The same read as CClientMemStream's against a length at +0x0c instead of +0x0c
// of a smaller object — identical shape, different offsets, and therefore a
// separate 114-byte body rather than a fold.
LTRESULT CGrowStream::Read(void* pData, unsigned long size)
{
	if (size == 0)
		return LT_OK;

	if (m_nPos + size > m_Array.m_nElements)
	{
		m_nPos   = m_Array.m_nElements;
		m_nError = 1;

		memset(pData, 0, size);

		return LT_ERROR;
	}

	memcpy(pData, &m_Array.m_pArray[m_nPos], size);

	m_nPos += size;

	return LT_OK;
}

// 0x00484280 — 276 bytes, IDENTICAL. (The csv had split it at an interior
// byte, 0x00484341; the real extent runs to the `ret $8` at 0x00484391.)
// About 200 of the bytes are Fast_NiceSetSize inlined. What closed it:
//
//   * BaseNew (0x0040c060) and BaseDelete (0x004152b0) are CALLS in the
//     shipped expansion. VC6 /Ob1 spends its per-function inline budget on
//     the LAST sites in source order first, so the budget has to be used up
//     by something AFTER the grow: `&m_Array[m_nPos]` (operator[] -> Get)
//     for the memcpy target does it, and generates the same code as a bare
//     m_pArray. With the bare member both helpers were expanded (and a
//     probe helper placed after the memcpy did the same thing as
//     operator[]; one placed before it did nothing).
//   * the `!` of the inlined LTBOOL is MATERIALISED — `xor %ecx,%ecx; test
//     %eax,%eax; sete %cl; mov %ecx,%eax; test %eax,%eax; jne` — and the
//     failure exit returns that register: a result local holding the `!`,
//     returned as itself. `if (!f()) return LT_ERROR;` folds the test away
//     (237 bytes).
//   * the new length as a named local: shipped `mov 0x18,%ebp; add
//     %edx,%ebp`, where the sum written out twice gives `lea`.
LTRESULT CGrowStream::Write(const void* pData, unsigned long size)
{
	if (size == 0)
		return LT_OK;

	uint32 nNewLen = m_nPos + size;

	if (m_Array.m_nElements < nNewLen)
	{
		LTRESULT dResult = !m_Array.Fast_NiceSetSize(nNewLen);
		if (dResult)
			return dResult;
	}

	memcpy(&m_Array[m_nPos], pData, size);

	m_nPos += size;

	return LT_OK;
}

// 0x004843a0
LTRESULT CGrowStream::ErrorStatus()
{
	return m_nError != 0;
}

// 0x004843b0
LTRESULT CGrowStream::SeekTo(unsigned long offset)
{
	if (offset > m_Array.m_nElements)
		return LT_ERROR;

	m_nPos = offset;

	return LT_OK;
}

// 0x004843d0
LTRESULT CGrowStream::GetPos(unsigned long* offset)
{
	*offset = m_nPos;

	return LT_OK;
}

// 0x004846c0 — folded with CClientMemStream::GetLen; see the note there.
LTRESULT CGrowStream::GetLen(unsigned long* len)
{
	*len = m_Array.m_nElements;

	return LT_OK;
}

void CGrowStream::Release()
{
	delete this;
}

// The destructor is NOT written, and that is deliberate: 0x004843e0 is the
// compiler-generated ??_G, and its whole body is the implicit chain —
// IDENTICAL (90 bytes) since m_Array became a member. It
// stores 0x004a1ab8 at +0x04 (that is ~CMoArray restoring its vptr — the
// dtor is not virtual, but the class has GenList's virtuals, so the store
// still happens), then expands Term() == SetSize2(0, &g_DefAlloc), then
// stores 0x004a1f60 at +0x00. That last address is ILTStream's OWN vtable,
// not CBaseStream's, because the two intermediate vptr stores (CGrowStream's
// and CBaseStream's) are dead in the same basic block and VC6 removes them —
// which is why no 0x004a3720 store appears in the destructor at all.
//
// The same three-store-collapse is visible at 0x00484cf0 (~TCPDriver), where
// the array is at +0x50 and the ONE guard around SetSize2's inner
// `if (m_pArray)` is folded, putting SetCacheSize(0) inside it. In
// CGrowStream::Write the same helper is reached WITHOUT an outer guard, so
// SetCacheSize(0) lands outside. That pair is the proof the two expansions
// are one source function.


// 0x00484440 — read a whole file into a growable stream. fopen "rb"
// (0x004a7638), seek/tell/seek for the length, one stream with a 0x100
// chunk, SetSize to the length, one fread, fclose. Caller 0x0042c141
// (name ours).
//
// SHIPPED BUG, and it is why the SetSize is not guarded on the `new`: at
// 0x004844c6 the length comparison reads 0xc(%esi) with %esi still zero on
// the allocation-failure path. The source does not test `new`'s result.
//
// IDENTICAL (310 bytes). Read off the bytes:
//   * `xor %esi,%esi` before fopen and `mov %esi,%eax` on the fopen-failure
//     exit: pStream is initialised to null and returned once.
//   * each failure path deletes, nulls pStream and falls into a shared
//     fclose / return pStream. The SetSize-failure copy is the one that
//     shares its tail with success, which VC6 does for the arm LAST in
//     source order — so the SetSize test is positive, with the failure as
//     its `else`.
//   * the ctor's Init is a CALL (0x004283b0) and SetSize's BaseNew/
//     BaseDelete are calls: the /Ob1 budget is spent last-site-first, and
//     fread's `GetArray()` (after the SetSize) is what exhausts it. With a
//     bare m_pArray the whole SetSize chain and Init's callees came out
//     differently (340 bytes).
ILTStream* growstream_OpenFile(char* pFilename)
{
	CGrowStream* pStream;
	FILE*        pFile;
	uint32       nLen;

	pStream = LTNULL;

	pFile = fopen(pFilename, "rb");

	if (pFile)
	{
		fseek(pFile, 0, SEEK_END);
		nLen = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		pStream = new CGrowStream(0x100);

		if (pStream->m_Array.SetSize(nLen))
		{
			if (fread(pStream->m_Array.GetArray(), 1, nLen, pFile) != nLen)
			{
				delete pStream;
				pStream = LTNULL;
			}
		}
		else
		{
			delete pStream;
			pStream = LTNULL;
		}

		fclose(pFile);
	}

	return pStream;
}


// *********************************************************************** //
//
//  ATTRIBUTION — what in 0x00483430..0x004869a0 is NOT written here
//
//  ALREADY SOURCED. Every one of these has its home in the repository and
//  must not be written a second time; the addresses are the CLIENT's.
//
//    src/engine/server/objbank.cpp — four CObjectBank<T> instantiations,
//    proved by their four-slot vtables at 0x004a3610 / 0x004a3600 /
//    0x004a35f0 / 0x004a35e0, whose slots are exactly
//    {~CObjectBank, Allocate, Free, Init}:
//      0x00483430  0x00483490  0x004834d0  0x00483510   Allocate
//      0x00483550  0x00483570  0x00483590                Init (0xa4/0xb8/0xd0)
//      0x004835b0  0x004835f0  0x00483650  0x00483690    ??_G
//      0x00483470                                        Free (already MATCHED)
//    CORRECTION: these four are NOT objbank.cpp's CObjectBank — they are
//    stdlith object_bank.h's ObjectBank<T>::AllocVoid over the sound
//    manager's banks, and the "Init" row is object_bank.h's Term. All of
//    0x00483430..0x00483590 are now written at the end of this file.
//
//    src/engine/server/spritecontrol.cpp
//      0x004836f0  SoundInstance::Init          (nearpos places it here)
//      0x00483900  SoundInstance::UpdateTimer   (its "SetSoundTrackChangeFlags"
//                                                error string is the pin)
//      0x00483e60  SpriteControl::SetCurPos     (by vtable neighbourhood)
//
//    src/engine/server/streamsim.cpp
//      0x0048406f  NOT A FUNCTION. It is the interior of
//                  CFileStream::SeekTo, whose real extent is
//                  0x00484060..0x00484088 (40 bytes). The enumeration split it.
//
//    src/engine/server/stringbank.cpp
//      0x00484869  NOT A FUNCTION — interior of sb_CreateStringRaw
//                  (0x00484850, 134 bytes).
//      0x004848f9  NOT A FUNCTION — interior of sb_CreateString
//                  (0x004848e0, 146 bytes).
//      0x004849c0 / 0x004849f0 are sb_CompareStrings and
//      sb_CompareStringsUpper. Both are 41 bytes in the client because the
//      client links strcmp/stricmp as CALLS; our lithtech build inlines
//      strcmp and comes out 94. That is a compile difference, not a source
//      one — do not edit the source to chase it.
//
//    src/engine/server/tcpdriver.cpp — the whole driver. Slot numbers are
//    from that file's CTCPDriverBase; the client vtable is 0x004a37a4:
//      0x00484c10  TCPDriver::TCPDriver
//      0x00484cd0  ??_GTCPDriver          (already MATCHED, ambiguously)
//      0x00484cf0  TCPDriver::~TCPDriver
//      0x00484de0  TCPDriver::CloseSockets   (between Init and Term, as on
//                                             the server)
//      0x00485050  TCPDriver::StartQuery        slot 0x20
//      0x00485c40  TCPDriver::EndQuery          slot 0x2c
//      0x00485d70  TCPDriver::UpdateSessionName slot 0x38
//      0x00485e00  TCPDriver::Disconnect        slot 0x5c
//      0x00486040  TCPDriver::HandleConnPacket (1446 bytes)
//      0x00486459  NOT A FUNCTION — interior of the above.
//      0x004869a0  TCPDriver::JoinSession(sockaddr_in*) (1867 bytes, one
//                  function: the "interior ret" at 0x0048708c is only its
//                  timeout exit, placed last)
//      0x00486640  TCPDriver::ReceivePacket     slot 0x64
//      0x00486780  TCPDriver::GetTcpIpAddress   slot 0x50
//
//  NEW, IDENTIFIED, AND DELIBERATELY LEFT — each one's home is
//  src/engine/server/tcpdriver.cpp, and writing it here would mean carrying
//  a second copy of TCPDriver/TCPQueryResponse into a scratch file, so they
//  are named and left:
//
//    0x00485590  1196 bytes.  TCPDriver::UpdateQuery, vtable slot 0x24.
//                tcpdriver.cpp does not override this slot at all.
//    0x00485a80   401 bytes.  TCPDriver::GetQueryResults, slot 0x28. Same.
//    0x004853c0   118 bytes.  TCPQueryResponse::operator=. It frees the
//                destination's m_pData (+0x98), copies 0x29 dwords — which
//                is exactly the 0xa4 bytes tcpdriver.cpp gives that class —
//                then strlen/dalloc/strcpy's the source's m_pData. A deep
//                copy, so user-written, and it is what
//                CMoArray<TCPQueryResponse>::Insert2 calls.
//    0x00484cc0     6 bytes.  TCPDriver::GetRecvSize, slot 0x68:
//                `mov $0x1c,%eax; ret`. Too small for the oracle to anchor;
//                adding the override to tcpdriver.cpp costs nothing and
//                would be scored `unverifiable`.
//
//  NEW, IDENTIFIED, AND OUT OF BUDGET — the client's sprite loader. All
//  client-only, so there is no duplication risk; they just need the sprite
//  record laid out first:
//
//    0x00483a60   507 bytes.  Loads a .spr through an ILTStream: allocates
//                a 0x14-byte header, then m_nAnims 0x3c-byte animation
//                records, each with a frame-pointer array at +0x20, a name
//                defaulted to "Untitled" with LTStrCpy(.., 0x1f), and
//                +0x28 = (1000 / msPerFrame) * nFrames. On failure it calls
//                0x00483c60 (name below) and returns 0.
//    0x00483c60    81 bytes.  Frees one of those records. ALREADY DECLARED
//                in src/engine/lithtech/clientmgr2.cpp as sub_483c60 and
//                called from cm_ClearList1100, so that file is its home.
//    0x00483cc0    58 bytes.  ALREADY DECLARED in
//                src/engine/lithtech/clientmgr.h as sub_483cc0.
//    0x00483d00   113 bytes.  Advances a sprite instance's frame from a
//                frame time: the two `div`s against +0x2c and +0x24 of the
//                animation record are the ms-per-frame and frame-count
//                fields the loader wrote.
//    0x00483630    18 bytes.  A constructor — `call 0x0047d310; movl
//                $0x004a3698,(%esi); return this` — for the client object
//                class whose vtable is 0x004a3698 (the same one
//                0x00483490's CObjectBank Allocate placement-news).
//                SKIPPED ON PURPOSE: 8 of its 18 bytes are relocations, and
//                a known example is a body of exactly this
//                shape matching an address it had nothing to do with. It
//                needs its call targets checked before anyone trusts a match.
//
//  CONFIDENCE SUMMARY for what IS written above:
//    HIGH        sb_GetStringLen, str_GetMessage, CClientMemStream (all seven),
//                CGrowStream::Read/ErrorStatus/SeekTo/GetPos/GetLen
//    MED-HIGH    tcp_BuildSockaddrFromString (now in tcpdriver.cpp)
//    IDENTICAL   growstream_Open, ??_GCGrowStream, the three bank AllocVoids,
//                CGrowStream::Write, growstream_OpenFile
//
// *********************************************************************** //


// ======================================================================= //
//  0x00483490 / 0x004834d0 / 0x00483510 — the sound manager's three
//  instance banks' AllocVoid
// ======================================================================= //
//
//  HOME: these are stdlith's object_bank.h ObjectBank<T>::AllocVoid, slot 1
//  of the four-slot vtables at 0x004a3600 / 0x004a35f0 / 0x004a35e0
//  ({??_G, AllocVoid, FreeVoid, Term} — BaseObjectBank's order). Their T is
//  the element of clientsoundmgr.cpp's m_SoundBank0/1/2 (+0x2b8/+0x2dc/
//  +0x300): the three concrete sound-instance classes, told apart by the
//  constructor each placement-news and by the size each Term hands sb_Init:
//
//      vtable      AllocVoid   ctor                      Term (size)
//      0x004a3600  0x00483490  0x0047d310 + vptr 0x4a3698 0x00483570 (0xb8)
//                              (the in-head class: its constructor is
//                              inline — out of line at 0x00483630)
//      0x004a35f0  0x004834d0  0x0047e970 (ambient)      0x00483570 (0xb8)
//      0x004a35e0  0x00483510  0x0047eda0 (3D)           0x00483590 (0xd0)
//
//  (0x004a3610 / 0x00483430 is the same template over CSoundData, 0xa4,
//  ctor 0x0047c630 — not written here.)
//
//  object_bank.h is off this target's include path, so the template and
//  struct_bank.h's inline sb_Allocate are restated below verbatim, with the
//  same view names sndmgr2.cpp uses and an L10 suffix. The body is
//  AllocVoid() { return Allocate(); } with Allocate expanded: the free-list
//  head, the grow through 0x00490c20 and the RE-READ, the unlink, then
//  placement-new's redundant null test and the constructor. The element
//  classes carry only what the three bodies reach (names ours).
struct StructBankL10
{
	uint32 m_StructSize;                     // +0x00
	uint32 m_AlignedStructSize;              // +0x04
	uint32 m_CacheSize;                      // +0x08
	uint32 m_nPages;                         // +0x0c
	uint32 m_nTotalObjects;                  // +0x10
	void*  m_pPageHead;                      // +0x14
	void*  m_FreeListHead;                   // +0x18
};                                           // 0x1c

// 0x00490c20 — struct_bank.h's page grower, __cdecl.
int sb_AllocateNewStructPageL10(StructBankL10* pBank, unsigned long nAllocations);
// 0x00490b50 / 0x00490bd0 — struct_bank.h's sb_Init and sb_Term2, __cdecl.
void sb_InitL10(StructBankL10* pBank, unsigned long structSize, unsigned long cacheSize);
void sb_Term2L10(StructBankL10* pBank, int bCheckObjects);

inline void* sb_AllocateL10(StructBankL10* pBank)
{
	void* pRet;

	pRet = pBank->m_FreeListHead;
	if (!pRet)
	{
		sb_AllocateNewStructPageL10(pBank, pBank->m_CacheSize);
		pRet = pBank->m_FreeListHead;
		if (!pRet)
			return 0;
	}

	pBank->m_FreeListHead = *(void**)pRet;
	return pRet;
}

class DummyThingForConstructorL10
{
public:
};

inline void* operator new(size_t size, DummyThingForConstructorL10* pThing,
                          void* ptr)
{
	return ptr;
}

inline void operator delete(void* pDataPtr, DummyThingForConstructorL10* pThing,
                            void* ptr)
{
}

class NullCSL10
{
public:
	void Enter() {}
	void Leave() {}
};

class BaseObjectBankL10
{
public:
	virtual       ~BaseObjectBankL10() {}

	virtual void* AllocVoid() = 0;
	virtual void  FreeVoid(void* ptr) = 0;
	virtual void  Term() = 0;
};

template<class T, class CS = NullCSL10>
class ObjectBankL10 : public BaseObjectBankL10
{
public:
	// object_bank.h has `virtual void* AllocVoid() {return Allocate();}`
	// with Allocate() an inline member; Allocate's body is written straight
	// into AllocVoid here ONLY so that the explicit instantiations below do
	// not also emit three out-of-line Allocate COMDATs. The shipped image has
	// no such copies, and those phantom symbols, being byte-identical to the
	// whole 58-byte bank family, would match unrelated addresses
	// (one took CSoundData's 0x00483430). The expansion is the
	// same bytes either way.
	virtual void* AllocVoid()
	{
		T* pRet;

		m_CS.Enter();
			pRet = (T*)sb_AllocateL10(&m_Bank);
			if (pRet)
			{
				::new((DummyThingForConstructorL10*)0, pRet) T;
			}
		m_CS.Leave();

		return pRet;
	}

	// 0x00483550 (0xa4) / 0x00483570 (0xb8) / 0x00483590 (0xd0) — slot 3,
	// object_bank.h's Term verbatim (DEFAULT_OBJECT_CACHE_SIZE is 32). The
	// in-head and ambient banks share 0x00483570: both elements are 0xb8,
	// so the two bodies are identical and the linker folded them.
	virtual void Term()
	{
		m_CS.Enter();
			sb_Term2L10(&m_Bank, 1);
			sb_InitL10(&m_Bank, sizeof(T), 32);
		m_CS.Leave();
	}

public:
	StructBankL10 m_Bank;                    // +0x04
	CS            m_CS;                      // +0x20
};

// The sound manager's sound-data bank element (0x004a3610's T), 0xa4
// bytes, constructor out of line. Its bank's AllocVoid is 0x00483430.
class CSoundDataL10
{
public:
	CSoundDataL10();                         // 0x0047c630

	uint8 m_pad_00[0xa4];
};

// The base sound instance, vtable 0x004a3448 (clientsoundinst.cpp's
// CSoundInstanceView); only its out-of-line constructor matters here.
class CSoundInstanceL10
{
public:
	CSoundInstanceL10();                     // 0x0047d310
	virtual ~CSoundInstanceL10();

	uint8 m_pad_04[0xb8 - 0x04];
};

// m_Type == 0, the in-head sound (vtable 0x004a3698). Its constructor is
// the implicit one — base call, then the vptr store — which is why
// 0x00483490 carries a `movl $0x4a3698,(%esi)` the other two do not. No
// destructor of its own either: slot 0 of all four vtables is the one
// shared ??_G at 0x004836d0, which calls the base's 0x0047d3e0 directly.
// (Its three overridden slots, soundinst2.cpp's, are not needed here.)
class CSoundInstanceLocalL10 : public CSoundInstanceL10
{
};

// m_Type == 1 (ambient) and 2 (3D), constructors out of line.
class CSoundInstanceAmbientL10 : public CSoundInstanceL10
{
public:
	CSoundInstanceAmbientL10();              // 0x0047e970
};

class CSoundInstance3DL10 : public CSoundInstanceL10
{
public:
	CSoundInstance3DL10();                   // 0x0047eda0

	uint8 m_pad_b8[0xd0 - 0xb8];
};

template class ObjectBankL10<CSoundDataL10>;              // 0x00483430
template class ObjectBankL10<CSoundInstanceLocalL10>;     // 0x00483490
template class ObjectBankL10<CSoundInstanceAmbientL10>;   // 0x004834d0
template class ObjectBankL10<CSoundInstance3DL10>;        // 0x00483510
