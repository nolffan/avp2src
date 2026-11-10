// server.dll — LMessageImpl's raw pair and housekeeping (reconstruction).
// Build /O1.
//
// A translation unit of its own, after lmessage.cpp's typed readers and
// writers, in both images: Release, ReadRawFL, WriteRaw, ResetPos,
// GetStatus and ReadString lie together (lithtech.exe 0x00449fa0..
// 0x0044a08f, server.dll 0x1001a22e..0x1001a2dc) between two <string>
// initializers of their own (lithtech.exe 0x00449f70 before them, as /O2
// lays a unit out; server.dll 0x1001a2dd after them, as /O1 does), where
// lmessage.cpp's code has the one before (0x00449690) and after
// (0x1001a20d) its own.
#if defined(PRIMAL_HUNT)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit has
// <string>'s guarded initializer of std::ctype<unsigned short>::id
// (lithtech.exe guard 0x004f8014, atexit of the empty 0x00401790;
// server.dll guard byte 0x1008ec84, atexit of 0x1002eaeb).
#include <string>
#endif
#include <string.h>
#include "servermsg.h"
#include "iltmessage.h"
#include "de_error.h"

LTRESULT LMessageImpl::Release()
{
	m_pOwner->Release();

	return LT_OK;
}

// 0x10014cf8 (server.dll, out of line) / inlined into lithtech.exe's
// ReadRawFL at 0x0043ddd5. It lives HERE, `inline`, not in servermsg.cpp:
// server.dll places it between ReadRawFL (0x10014cd8) and WriteRaw
// (0x10014d1a) — an inline /O1 declined, emitted after its first use — and
// lithtech.exe's /O2 ReadRawFL (0x0043ddc0, 64 bytes) has it expanded.
// Both images byte-identical this way.
//
// What the reader does after every read: if the cursor has
// caught up with the payload length and flag bit 0 is set, wind it back to
// one. The subtraction is real — the shipped code subtracts and tests the
// result rather than comparing the two words (name ours).
//
// (Historical, now identical:) NEAR: 34 of 34 bytes, eleven of eleven instructions, four bytes different
// and all of them the same fact — the shipped build loads m_Unknown38 first
// (into ecx) and m_Unknown34 second (into edx), so its `sub`/`test` name the
// opposite pair from ours. Our VC6 always puts the minuend in the
// first-loaded register, and nine source shapes fail to move it: the two
// members inline, each through a uint16 local, each through an int local,
// `(int)` casts on both, `-(m38 - m34) <= 0`, `0 >= m34 - m38`,
// `nLeft = m34; nLeft -= m38;`, the pOwner local removed so both reads go
// through pMsg->m_pOwner, and an inlined two-argument helper (which would
// evaluate right-to-left but comes out nine bytes longer because the uint16
// parameters get promoted twice). cl 8168, 8966 and 8804 all emit what 8447
// emits. Load-order lottery; documented and left.
inline void lmsg_AfterRead(LMessageImpl* pMsg)
{
	ServerMessage* pOwner = pMsg->m_pOwner;

	// The store goes back through pMsg->m_pOwner, not the local, and that is
	// what orders the loads: with the store through pOwner the compiler
	// loads +0x34 first and subtracts into it (`sub %edx,%ecx`), with this
	// store it loads +0x38 first and subtracts into the +0x34 register
	// (0x10014cff..0x10014d07 `movzwl 0x38 ; movzwl 0x34 ; sub %ecx,%edx`).
	// lithtech.exe's inlined copy (0x0043dde9) loads +0x38 first too. The
	// same load order falls out of sm_ReadClientCommands in smgr.cpp.
	// Writing the store to any other field also flips the order, so it is
	// the pOwner store aliasing the +0x38 read that forces +0x34 first.
	if (pOwner->m_Unknown34 - pOwner->m_Unknown38 <= 0 && (pOwner->m_MsgFlags & 1))
		pMsg->m_pOwner->m_Unknown38 = 1;
}


// The raw pair is where the queue entry finally sees the bytes.
LTRESULT LMessageImpl::ReadRawFL(void* pData, uint32 len)
{
	m_pOwner->ReadData(pData, len);

	lmsg_AfterRead(this);

	return LT_OK;
}

LTRESULT LMessageImpl::WriteRaw(void* pData, uint32 len)
{
	m_pOwner->WriteData(pData, len);

	return LT_OK;
}

// Rewinding leaves the cursor at one, not zero: byte zero is the message id
// and was never part of the payload.
LTRESULT LMessageImpl::ResetPos()
{
	m_pOwner->m_Unknown38 = 1;

	return LT_OK;
}

// The queue entry's two overflow bits, reported in the interface's own
// vocabulary.
LTRESULT LMessageImpl::GetStatus(uint32& flags)
{
	flags = 0;

	if (m_pOwner->m_MsgFlags & 2)
		flags = LMSTAT_READOVERFLOW;

	if (m_pOwner->m_MsgFlags & 4)
		flags |= LMSTAT_WRITEOVERFLOW;

	return LT_OK;
}

// AVP2: iltmessage.h's ReadString(), slot 1 of the reader (servermsg.h's
// IMessageRead +0x04): the owner hands back a pointer to the string in
// place and steps its read cursor past the terminator, then the same
// end-of-data test as every read. server.dll 0x1001a2c3 (ServerMessage's
// half at 0x10031041), lithtech.exe 0x0044a050 (0x0046f380), each right
// after GetStatus. servermsg.h declares neither half, so both are views
// here (names ours).
class CServerMessageStrQ528
{
public:
	const char* ReadString();                // 0x10031041 / 0x0046f380
};

class CLMessageStrQ528 : public LMessageImpl
{
public:
	const char* ReadString();                // 0x1001a2c3 / 0x0044a050
};

const char* CLMessageStrQ528::ReadString()
{
	const char* pRet = ((CServerMessageStrQ528*)m_pOwner)->ReadString();

	lmsg_AfterRead(this);

	return pRet;
}
