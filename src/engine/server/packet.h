// *********************************************************************** //
//
//  MODULE:   PACKET (.H)
//
//  PURPOSE:  CPacket — the network packet the message layer reads and writes.
//            The class name and both method names are attested by check
//            literals ("CPacket::ReadType", "CPacket::WriteType").
//
//            ReadType and WriteType are member templates defined in the class
//            body, and the image proves it three ways: the same two names
//            cover element sizes 1, 2 and 4; the code is identical across
//            them except for the size; and WriteType<uint32> appears TWICE at
//            different addresses — an out-of-line function cannot do that,
//            only an inline COMDAT compiled into more than one translation
//            unit whose copies the linker did not fold.
//
//            ReadType takes a T* it never reads. That is the VC6-era idiom
//            for choosing a template's type at the call site — VC6 could not
//            parse p->ReadType<uint16>(), so the type rides in on a dummy
//            argument instead: pPacket->ReadType((uint16*)0). The shipped
//            functions pop 4 bytes of unread argument, which is this.
//
// *********************************************************************** //

#ifndef __PACKET_H__
#define __PACKET_H__

#include "ltbasedefs.h"
#include "de_error.h"
#include "collection.h"

// m_Flags bits. WRITEOVERFLOW is 4 = sizeof(uint32), and the compiler knows:
// WriteType<uint32>'s error path sets it with `or ebx` from the register
// already holding sizeof(T).
#define PACKETFLAG_READOVERFLOW  2
#define PACKETFLAG_WRITEOVERFLOW 4

// The payload lives in a real stdlith byte array, CMoArray<uint8,
// DefaultCache> (collection.h), at CPacket+0x50: vptr, m_pArray at 0x54,
// m_nElements at 0x58, the spare count at 0x5c and the grow step at 0x60.
// Its grow is Insert2 (server.dll 0x1000c537, with _AllocateTArray
// 0x1000c690 and _DeleteAndDestroyArray 0x1000d020), and all three now come
// out of collection.h byte-identical; they used to be written by hand in
// packet.cpp as a `CPacketData::SetAtGrow` on a flat view of the array.
// The allocator is collection.h's g_DefAlloc -- the same 0x10058c7c that
// servermsg.h still declares as `uint32 g_PacketDataBank`.


class CPacket
{
public:
	uint8       m_pad_00[0x34];
	uint16      m_DataLen;                   // 0x34
	uint16      m_MaxDataLen;                // 0x36
	uint16      m_Pos;                       // 0x38
	uint8       m_pad_3a[0x40 - 0x3a];
	uint32      m_Flags;                     // 0x40
	uint8       m_pad_44[0x50 - 0x44];
	CMoArray<uint8> m_Data;                  // 0x50 .. 0x63

	// The dummy parameter carries T; it is never read.
	//
	// m_Pos is read in place at every use, not copied into a uint16 local:
	// at /O2 the copy stops VC6 sharing the zero-extended m_Pos with a
	// caller's `m_DataLen - m_Pos` loop test (lithtech.exe 0x00430080 and
	// 0x0047a0d0 re-masked it; both are identical this way).  server.dll
	// compiles the same either way.
	template<class T>
	T ReadType(T *dummy)
	{
		T val;

		if (m_Pos + sizeof(T) < m_Data.m_nElements && m_Pos + sizeof(T) <= m_DataLen)
		{
			val = *(T*)&m_Data.m_pArray[m_Pos];
			m_Pos += sizeof(T);
		}
		else
		{
			m_Flags |= PACKETFLAG_READOVERFLOW;
			REPORT_ERROR_INFO(1, "CPacket::ReadType", LT_ERROR, "ERROR: Packet read overrun!");
		}

		return val;
	}

	// WriteType's error report, in a helper that is always expanded. This is
	// not decoration: it is what sets how much VC6 inlines around the grow.
	//
	// lithtech.exe's /O2 build (/Ob1) has a per-function budget for inline
	// growth, and the expanded report is counted against it. With the macro
	// written straight into WriteType the budget is never reached: the
	// standalone WriteType<uint32> expands Insert2, _AllocateTArray AND
	// BaseNew/BaseDelete (359 of 356 bytes). With the report expanded through
	// this helper, BaseNew (0x0040c060) and BaseDelete (0x004152b0) stay calls,
	// which is what shipped has -- 0x004303c0, 0x0042fa50, 0x004159f0 and
	// 0x00466700 all become identical. At call sites that expand WriteType
	// itself, Insert2 becomes a call (0x00412f50) instead of being expanded,
	// which is what keeps sm_UpdateClientState2 (0x004630f0) identical.
	//
	// __forceinline because server.dll's /O1 does NOT expand a plain inline
	// helper here: it calls it, and WriteType drops to 107 bytes against 146.
	// With __forceinline both images expand it and the server bodies are
	// unchanged (0x1001393e, 0x1000ec3d, 0x1000ecd6 still identical).
	__forceinline void ReportWriteOverrun()
	{
		REPORT_ERROR_INFO(1, "CPacket::WriteType", LT_ERROR, "ERROR: Packet write overrun!");
	}

	template<class T>
	void WriteType(T val)
	{
		if (m_Pos + sizeof(T) < m_MaxDataLen)
		{
			// Append reaches Insert2 through Insert, with &g_DefAlloc; the
			// literal zero is a temporary, so no named dummy is needed.
			// A named `uint8 dummy` handed to Insert2 or Insert directly
			// costs server.dll two bytes (155 against 153).
			while (m_DataLen + sizeof(T) >= m_Data.GetSize())
				m_Data.Append(0);

			*(T*)&m_Data.m_pArray[m_Pos] = val;
			m_Pos += sizeof(T);
			m_DataLen += sizeof(T);
		}
		else
		{
			ReportWriteOverrun();
			m_Flags |= PACKETFLAG_WRITEOVERFLOW;
		}
	}
};

#endif  // __PACKET_H__
