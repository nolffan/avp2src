// server.dll — the generic half of ILTStream (reconstruction). Build /O1.
//
// See basestream.h for why these three are shared.
#include <string.h>
#include "servermgr.h"
#include "basestream.h"

// NEAR (183 bytes against 186, and the same seventy-nine instructions): the
// only difference is where the shared `xor %eax,%eax; jmp` that returns LT_OK
// is hosted. Two source statements return LT_OK — the empty-string arm and
// the maxBytes == 0 drain — and three edges reach the block. The shipped
// build puts it at the *first* of them, right after `pStr[0] = 0`, and jumps
// backward into it from the other two; ours puts it after the drain loop and
// jumps forward. That costs the shipped build three bytes, because from there
// the jump to the epilogue is 142 bytes away and needs the five-byte `e9`
// encoding where ours reaches it with a two-byte `eb`. The documented
// shared-exit-host class.
//
// Probed to a negative: the two LT_OK returns written as early returns and as
// an if/else-if/else chain with one return at the end, and the file rebuilt
// at cl 8168, 8966 and 8804. All four give the same 183 bytes.
//
// 0x10011314 — the read half of the same wire format: a word of length, then
// that many bytes, and a terminator the caller's buffer gets but the wire
// never carries.
//
// The interesting part is what happens when the string does not fit. The
// buffer is filled to maxBytes-1 and terminated, and the rest of the string
// is then read a byte at a time into a one-byte local and thrown away — the
// stream has to end up positioned after the string either way, or everything
// read afterwards is garbage. The same byte-at-a-time drain is what a caller
// asking for maxBytes == 0 gets for the whole string.
//
// The length is a `uint16` local and the shipped code reads it back as a word
// everywhere, but its two updates are 32-bit operations on the four-byte
// slot: `sub` without the 0x66 prefix is a byte shorter, and the high half is
// never read. The decrement is the same trick again — 0xffff materialised
// once into a register and added twice (11 bytes) rather than an imm32 add at
// each site (14).
//
// `maxBytes - 1` has to be a named local, not written out at its three uses.
// Written inline it comes out 188 bytes: VC6 keeps maxBytes itself in the
// register and folds the -1 into each use separately — a `-0x1(...)` in the
// terminator's addressing mode, and `1 - maxBytes` computed with a
// push/pop/sub for the remaining-length subtraction. One local gets the
// shipped single `lea -0x1(%eax),%edi` and the register reused three times.
//
// Nothing here reports a short read: the result of the length read is
// discarded, and only the result of the body read is returned. A truncated
// or absent string therefore comes back LT_OK with an empty buffer, and the
// caller is expected to ask ErrorStatus at the end instead.
LTRESULT CBaseStream::ReadString(char* pStr, unsigned long maxBytes)
{
	uint16   nLen;
	char     ch;
	uint32   i;
	unsigned long nMaxChars;
	LTRESULT dResult;

#ifdef AVP2_MODERN
	// fixed: CBaseStream::ReadString never reports a short read
	// (RETAIL_BUGS.md). A length that cannot be read is an error.
	dResult = Read(&nLen, 2);

	if (dResult != LT_OK)
	{
		if (maxBytes)
			pStr[0] = 0;

		return dResult;
	}
#else
	Read(&nLen, 2);
#endif

	// Nested, not a run of guards: both early arms answer LT_OK and the
	// shipped code hosts that answer at the FIRST of them, with the drain loop
	// jumping back up to it. A run of guarded returns hosts a shared tail at
	// the last arm instead.
	if (nLen == 0)
	{
		pStr[0] = 0;

		return LT_OK;
	}
	else
	{
		if (maxBytes == 0)
		{
			for (i = 0; i < nLen; i++)
				Read(&ch, 1);

			return LT_OK;
		}
		else
		{
			nMaxChars = maxBytes - 1;

			if (nLen > nMaxChars)
			{
				dResult = Read(pStr, nMaxChars);

				pStr[nMaxChars] = 0;

				nLen -= nMaxChars;

				while (nLen--)
					Read(&ch, 1);
			}
			else
			{
				dResult = Read(pStr, nLen);

				pStr[nLen] = 0;
			}

			return dResult;
		}
	}
}

// A string on the wire is a word of length followed by that many bytes, with
// no terminator: the length is written from a full dword local but only its
// low half reaches the stream, and the same half is what sizes the bytes.
LTRESULT CBaseStream::WriteString(char* pStr)
{
	uint32 nLen = strlen(pStr);

	Write(&nLen, 2);

	return Write(pStr, (uint16)nLen);
}

// 0x10011402 — pump one stream into another through a 512-byte stack buffer.
// The frame is exactly 0x204 on server.dll: the buffer plus the four bytes
// VC6 spills `this` into; /O1 homes both the GetLen target and the running
// remainder in the dead dwMin parameter slot. lithtech.exe (0x00433920) keeps
// them apart — the length in a 4-byte stack local, the remainder in edi —
// which is what says they are two variables, the length one scoped to the
// default-end block (at function scope /O1 put it in dwMax's slot instead).
// The clamp is a ternary: an if-clamp makes /O1 hoist the 0x200 into a
// register and /O2 test the other way round. Byte-identical on both images.
//
// Both defaults are WRITESTREAM_DEFAULT (0xffffffff): an unspecified start is
// zero and an unspecified end is the source's whole length.
//
// SHIPPED BUG: dwMin never reaches the source stream. It is subtracted from
// the end to size the transfer and nothing else — there is no SeekTo call in
// the function at all (the only slots it indexes are 0x1c GetLen, 0x08 Read,
// 0x24 Write and 0x10 ErrorStatus). So asking for a range starting anywhere
// but the current position copies the right *number* of bytes from the wrong
// place, and every caller in the image passes the default.
LTRESULT CBaseStream::WriteStream(ILTStream& dsSource, unsigned long dwMin,
                                  unsigned long dwMax)
{
	char          buf[512];
	unsigned long nLeft;
	unsigned long nChunk;

#ifdef AVP2_MODERN
	// fixed: CBaseStream::WriteStream ignores the source offset
	// (RETAIL_BUGS.md). A start the caller gives is sought to; the default
	// still copies from wherever the source is, as every caller expects.
	if (dwMin == WRITESTREAM_DEFAULT)
		dwMin = 0;
	else if (dsSource.SeekTo(dwMin) != LT_OK)
		return LT_ERROR;
#else
	if (dwMin == WRITESTREAM_DEFAULT)
		dwMin = 0;
#endif

	if (dwMax == WRITESTREAM_DEFAULT)
	{
		unsigned long nLen;

		dsSource.GetLen(&nLen);

		dwMax = nLen;
	}

	if (dwMax >= dwMin)
	{
		nLeft = dwMax - dwMin;

		while (nLeft)
		{
			nChunk = (nLeft >= sizeof(buf)) ? sizeof(buf) : nLeft;

			if (dsSource.Read(buf, nChunk) != LT_OK)
				return LT_ERROR;

			if (Write(buf, nChunk) != LT_OK)
				return LT_ERROR;

			nLeft -= nChunk;
		}

		if (dsSource.ErrorStatus() == LT_OK && ErrorStatus() == LT_OK)
			return LT_OK;
	}

	return LT_ERROR;
}
