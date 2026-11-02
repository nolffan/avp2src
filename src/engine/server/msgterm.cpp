// =====================================================================
//  MODULE: msgterm.cpp
//
//  0x00455bc0 -- ServerMessage::~ServerMessage(), a /O2-only row.
//
//  Its own translation unit rather than an addition to servermsg.cpp,
//  for one reason: the stand-in needs collection.h, and servermsg.cpp
//  does not include it. Adding that include there would put 45 already-
//  matching COMDATs at risk of a pooling or ordering shift for no gain,
//  and the inline-depth argument this row turns on is a property of the
//  class and its member, not of the translation unit.
// =====================================================================

#include "servermsg.h"
#include "collection.h"

// =====================================================================
//  0x00455bc0 (lithtech.exe, /O2 only) -- ServerMessage::~ServerMessage
//
//  A /O2-only row: at /O1 the same source gives a body server.dll has no
//  standalone copy of, so it can neither gain nor lose anything there.
//  Purely ADDITIVE -- the existing CTermPacketData/CTermMessage drivers
//  above keep matching server.dll 0x100242c1 and 0x100241f4 untouched.
//
//  THE FINDING: the +0x50 member is literally CMoArray<uint8,
//  DefaultCache>, and g_PacketDataBank IS g_DefAlloc (0x10058c7c in
//  server.dll, 0x004b4e88 in lithtech.exe -- collection.h and packet.h
//  both name that address). So CMsgPacketData::SetSize(n,
//  &g_PacketDataBank) and CMoArray<uint8,DefaultCache>::SetSize2(n,
//  &g_DefAlloc) are the same call, and Term()'s default argument
//  already supplies it. Three proofs off shipped bytes: the field
//  layout (0x50 + 0x14 = 0x64, the size 'new ServerMessage' asks for),
//  the vftable 0x004a1ab8 already identified in clientstreams.cpp:468,
//  and the body, which is SetSize2(0, pAlloc) with
//  _DeleteAndDestroyArray folded in, instruction for instruction.
//
//  WHY BaseDelete IS A CALL HERE. The same template chain appears at
//  four lithtech addresses, stopping one inline level earlier each
//  time: 0x0040bfe0 (BaseDelete expanded) -> 0x004912d0 (expanded,
//  +1 level, ALREADY byte-identical from collection.cpp) -> 0x00455bc0
//  (this row, BaseDelete CALLED) -> 0x00459dc0 (SetSize2 itself
//  called). So only the NESTING DEPTH had to be got right, and a real
//  member of that type gets it structurally.
// =====================================================================

// collection.h is already in scope through servermgr.h -> lthread.h, so
// CMoArray, DefaultCache, LAlloc and g_DefAlloc need no extra include.

// 0x004a2b44 -- CAbstractIO's vftable.  Stored as a plain address rather
// than through virtuals of our own, for the reason servermsg.cpp gives:
// a vtable here would drag in a deleting destructor per view that the
// shipped image has no counterpart for.  Relocations are masked, so the
// store is the same bytes either way.
extern void* g_AbstractIOVtblQ454;                   // 0x004a2b44

class CTermAbstractIOQ454
{
public:
	~CTermAbstractIOQ454() { m_pVtbl = &g_AbstractIOVtblQ454; }

	void*  m_pVtbl;                                  // +0x00
	uint32 m_nRefCount;                              // +0x04
};

// The ServerMessage layout, reduced to the two things its destructor
// touches.  Declaration order is the destruction order reversed, and the
// shipped bytes destroy +0x50 first and +0x00 last.
class CTermMessageQ454
{
public:
	~CTermMessageQ454();                             // 0x00455bc0

	CTermAbstractIOQ454           m_Base;            // +0x00
	uint8                         m_pad_08[0x50 - 0x08];
	// +0x50 .. +0x64.  servermsg.h calls this CMsgPacketData and keeps
	// the four fields on ServerMessage under the names six other TUs
	// already use; they are m_pArray / m_nElements / m_CacheSize /
	// m_WantedCache of this template.
	CMoArray<uint8, DefaultCache> m_PacketData;      // +0x50
};

// DEFINED OUT OF LINE ON PURPOSE.  In-class (or with a caller in this TU
// that expands it) VC6 emits no COMDAT for it at all and the row cannot
// be measured.  The body is empty: everything in the shipped 61 bytes is
// the two member destructors the compiler generates after it.
CTermMessageQ454::~CTermMessageQ454()
{
}


