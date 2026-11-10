// *********************************************************************** //
//
//  MODULE:   MODELLOADSTATE (.CPP)   (file name ours)
//
//  The model loader's load state: CModelLoadState's constructor and its
//  size estimate (sub_1001cec1), and the counts record's Clear and Read
//  (AvP2 1.0.9.6 server.dll 0x1001cdd5 ..).  Their own translation unit in
//  both of Primal Hunt's images, between two <string> guard pairs:
//  lithtech.exe 0x0045e060 .. 0x0045e2c3 (guards 0x0045e030, 0x0045e2d0),
//  server.dll 0x10024320 .. 0x10024545 (guards 0x100242ff, 0x10024546).
//  Split out of commonmodel.cpp, whose callers (Model's loaders) declare
//  them through commonmodel.h.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit has
// <string>'s guarded initializer of std::ctype<unsigned short>::id
// (lithtech.exe guard 0x004f8014, atexit of the empty 0x00401790;
// server.dll guard byte 0x1008ec84, atexit of 0x1002eaeb).
#include <string>
#endif
#include <string.h>
#include "commonmodel.h"


// 0x1001cdd5 — nothing but the sub-object's constructor and the `this` the
// caller gets back: `push %esi; mov %ecx,%esi; call; mov %esi,%eax`.
// Defined AHEAD of Clear, which is the shipped order: 0x1001cdd5 + 12 is
// 0x1001cde1 and the call's displacement is 4. The
// counts record's own constructor has to be out of line for the call to
// survive — /O1 implies /Ob1, so a body in the class declaration would be
// expanded here instead.
CModelLoadState::CModelLoadState()
{
	m_Counts.Clear();
}

// 0x1001cde1 — the counts record cleared. Thirteen stores off one `xor
// eax,eax`, which is what says the source assigned each field rather than
// memsetting the record (a memset would be `rep stos`) — and, with an array
// standing in for thirteen fields whose names are not recoverable, thirteen
// subscripts is the only spelling that produces it: /O1 does not unroll a
// loop.
//
// A constructor is what this looks like and is not what it is: written as
// `CModelFileCounts::CModelFileCounts()` our build comes out 43 bytes, two
// over, because VC6 makes a constructor return `this` (`mov %ecx,%eax`) and
// the shipped function leaves the zero register in eax instead. So the
// shipped source has an ordinary void member here and calls it from
// CModelLoadState's constructor.
void CModelFileCounts::Clear()
{
	m_n[0]  = 0;
	m_n[1]  = 0;
	m_n[2]  = 0;
	m_n[3]  = 0;
	m_n[4]  = 0;
	m_n[5]  = 0;
	m_n[6]  = 0;
	m_n[7]  = 0;
	m_n[8]  = 0;
	m_n[9]  = 0;
	m_n[10] = 0;
	m_n[11] = 0;
	m_n[12] = 0;
}

// 0x1001ce0a — and the counts read back off the wire, thirteen four-byte
// reads and then one question to the stream about whether any of them
// failed. Nothing checks the individual reads, which is exactly what
// ILTStream::Read's contract says to do: it zero-fills on failure so that a
// caller can read everything and test ErrorStatus() once at the end.
//
// The four is hoisted into ebx (`push $0x4; pop %ebx`) and pushed thirteen
// times from there, so every read uses the same size constant. The answer is
// `neg; sbb; inc` — an `== 0` compare, i.e. against LT_OK, not a `!= 0`.
LTBOOL CModelFileCounts::Read(ILTStream* pStream)
{
	pStream->Read(&m_n[0],  sizeof(m_n[0]));
	pStream->Read(&m_n[1],  sizeof(m_n[1]));
	pStream->Read(&m_n[2],  sizeof(m_n[2]));
	pStream->Read(&m_n[3],  sizeof(m_n[3]));
	pStream->Read(&m_n[4],  sizeof(m_n[4]));
	pStream->Read(&m_n[5],  sizeof(m_n[5]));
	pStream->Read(&m_n[6],  sizeof(m_n[6]));
	pStream->Read(&m_n[7],  sizeof(m_n[7]));
	pStream->Read(&m_n[8],  sizeof(m_n[8]));
	pStream->Read(&m_n[9],  sizeof(m_n[9]));
	pStream->Read(&m_n[10], sizeof(m_n[10]));
	pStream->Read(&m_n[11], sizeof(m_n[11]));
	pStream->Read(&m_n[12], sizeof(m_n[12]));

	return pStream->ErrorStatus() == LT_OK;
}

// 0x1001cec1 — how many bytes the file's data will need, from the thirteen
// counts the header chunk carried. Twenty-four terms, each one a count (or a
// product of two) times a record size, accumulated through the caller's
// pointer: every `+=` stores, so the shipped code has a `mov %reg,(%eax)`
// after each and re-reads each count out of `this` afterwards, because a
// store through `uint32*` may have hit the counts. It forwards its own store
// into the accumulator register, which is why the *running total* is never
// re-read.
//
// A file that declares none of whatever the ninth count counts is refused
// outright, and the compare says how it was written: `cmpl $0x1,0x20(%ecx)`
// with `jae` past the failure is `< 1` on an unsigned, not `== 0` (which
// would compare against zero).
//
// The record sizes are read straight off the multipliers — 12, 28, 4, 48,
// 20, 36, 108, 40, 52, 20, 64, 92, 46, 32 — and 46 is not a mistake: it
// really is `imul $0x2e`, folded together with a neighbouring term so that
// the common `* 4` is factored out. Which count means what is not readable
// from here (only the arithmetic is), so they stay an array.
//
// The `(m_n[2] - 1)` at the top is loaded before the first store and reused
// three times over, and `(m_n[2] - 1) * 46` twice; the counts VC6 does NOT
// hold across a store it reloads every time. Naming the first as a local is
// what puts the load above the store.
//
// NEAR (309 of 309 bytes, 118 of 118 instructions, zero mnemonic blocks): two
// bytes differ, and they are the two memory operands of ONE commutative
// product swapping roles. For `m_n[2] * m_n[10] * 4` the shipped build emits
// `mov 0x8(%ecx),%esi; imul 0x28(%ecx),%esi` and ours the other way round.
// Every other product in the function agrees, and they all follow the same
// rule — move the higher offset, multiply by the lower (0x08*0x00,
// 0x10*0x08, 0x20*0x0c) — so it is this one term in the shipped code that
// breaks the pattern, not ours.
//
// Probed to a firm negative: both operand orders, `(a*b)*4`, `a*(b*4)`,
// `(a*4)*b`, `4*a*b`, `a*4*b`, an `(int)` cast on either side, and a
// block-scoped local holding m_n[2] before the multiply — VC6 canonicalises
// all nine to the same two instructions (so do `(a*b) << 2`, `b*sizeof(uint32)*a`
// and `(a+0)*b*4`; `(b<<2)*a` adds a shl; `(nOther + 1) * m_n[10] * 4`
// gives 311 bytes and 12 blocks; dropping nOther at every use, or at any
// one of its three uses, gives 309/115, 311/119, 311/119 or 309/116; all
// counts read through a `uint32* pn = m_Counts.m_n` local gives 10 differing
// instructions; `*(&m_n[2]) * *(&m_n[10])` is unchanged) — and cl 8168, 8804 and 8966 emit
// what 8447 does. Same class as the SIB base/index roles recorded in
// 09-codegen-patterns for SpriteControl::GetNumFrames and
// CServerMgr::EndWorld: which operand of a symmetric pair becomes the
// accumulator is not reachable from source.
//
// Primal Hunt lithtech.exe 0x0045e160 (355 bytes) is the same class of near
// once AvP2's +1000 is in: size- and mnemonic-exact, the one swapped pair
// being `m_n[4] * m_n[2] * 28` (shipped `mov 0x10; imul 0x8` at
// 0x0045e1ef), and both spellings of it canonicalise to ours.
LTBOOL CModelLoadState::sub_1001cec1(uint32* pnBytes)
{
	if (m_Counts.m_n[8] < 1)
		return LTFALSE;

	*pnBytes = 0;

	uint32 nOther = m_Counts.m_n[2] - 1;

	*pnBytes += m_Counts.m_n[11] * 12;
	// AVP2: a thousand bytes of slack on top (Primal Hunt lithtech.exe
	// 0x0045e196).
	*pnBytes += (((m_Counts.m_n[11] + 1) * 3 + m_Counts.m_n[12]) & 0xfffffffc) + 1000;
	*pnBytes += m_Counts.m_n[0] * m_Counts.m_n[2] * 28;
	*pnBytes += nOther * m_Counts.m_n[1] * 4;
	*pnBytes += nOther * m_Counts.m_n[1] * 48;
	*pnBytes += m_Counts.m_n[0] * 20;
	*pnBytes += (m_Counts.m_n[2] * m_Counts.m_n[1] + nOther * 46) * 4;
	*pnBytes += m_Counts.m_n[4] * m_Counts.m_n[2] * 28;
	*pnBytes += m_Counts.m_n[5] * 32;
	*pnBytes += m_Counts.m_n[6] * 32;
	*pnBytes += (m_Counts.m_n[8] - 1) * m_Counts.m_n[3] * 36;
	*pnBytes += m_Counts.m_n[3] * 4;
	*pnBytes += m_Counts.m_n[3] * 108;
	*pnBytes += m_Counts.m_n[2] * m_Counts.m_n[10] * 4;
	*pnBytes += m_Counts.m_n[10] * 4;
	*pnBytes += m_Counts.m_n[10] * 40;
	*pnBytes += m_Counts.m_n[9] * 4;
	*pnBytes += m_Counts.m_n[9] * 52;
	*pnBytes += (m_Counts.m_n[8] - 1) * 4;
	*pnBytes += m_Counts.m_n[7] * 20;
	*pnBytes += m_Counts.m_n[2] * 64;
	*pnBytes += m_Counts.m_n[2] * 4;
	*pnBytes += (m_Counts.m_n[4] - 1) * 48;
	*pnBytes += m_Counts.m_n[1] * 92;

	return LTTRUE;
}
