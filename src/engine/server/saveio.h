// *********************************************************************** //
//  MODULE:   SAVEIO (.H)
//  PURPOSE:  The guard the save stream puts around every object's data.
//            Each read and each write is followed by a four-byte sentinel,
//            0xABCDEFAF, so that an object which wrote more or less than it
//            reads back is caught at the boundary rather than corrupting the
//            next one — which is what the "Restore OVER wrote %d bytes!" and
//            "Restore UNDER wrote %d bytes!" reports nearby are for.
//
//            Class and field names are ours.
// *********************************************************************** //
#ifndef __SAVEIO_H__
#define __SAVEIO_H__

#include "ltbasedefs.h"
#include "iltstream.h"

// The word that follows every guarded read and write.
#define SAVE_SENTINEL 0xabcdefaf

// WRONG ADDRESS, and worth knowing before the next reader trusts it:
// 0x1000f08d is NOT this class's constructor. It is a 22-byte body that
// zeroes six dwords at +0, +4, +8, +0x14, +0xc, +0x10 of an 0x18-byte record,
// and its only caller is the ray-versus-box test at 0x1000f00c
// (`lea -0x18(%ebp),%ecx; call 0x1000f08d`), which is the world intersect
// family's query record, not a save object. Our
// ??0CSaveIO@@QAE@XZ matches there because the two constructors happen
// to zero the same six offsets in the same order, so a byte match counts
// this address twice and neither count is this file's.
//
// What the real class is, is settled by the vtable at 0x1004d660: slot 8 is
// 0x10010090, ReadChecked below. That vtable belongs to a TWELVE-byte object
// -- a vptr, a class-manager pointer at +4 and the stream at +8 -- which
// sm_RestoreNextObject and sav_SaveObject build on the stack, and whose
// other eleven slots are compiled into the message module
// (0x100145d2-0x10014990). So ReadChecked, WriteChecked and GetNothing are
// virtuals of that class; saveio.cpp models it separately, as
// CSaveLoadMessage, because those eleven slots are not ours to declare and
// making these three virtual here would need all of them.
//
// The three bodies below are byte-identical either way -- they are thiscall
// members whose codegen does not depend on being virtual -- so this is a
// naming and layout question, not a verification one.
class CSaveIO
{
public:
	CSaveIO();                               // NOT 0x1000f08d; see above

	// 0x10010090 and 0x100100cd — the guarded pair. A zero length is not
	// guarded: there is nothing to have got wrong.
	LTRESULT ReadChecked(void* pData, uint32 nLen);
	LTRESULT WriteChecked(void* pData, uint32 nLen);

	// 0x10010105 — answers nothing, successfully. `this` is untouched, so
	// what it would have answered from is not reached (name ours).
	LTRESULT GetNothing(uint32* pOut);

public:
	uint32     m_n00;                        // 0x00
	uint32     m_n04;                        // 0x04
	ILTStream* m_pStream;                    // 0x08
	uint32     m_n0c;                        // 0x0c
	uint32     m_n10;                        // 0x10
	uint32     m_n14;                        // 0x14
};

#endif  // __SAVEIO_H__
