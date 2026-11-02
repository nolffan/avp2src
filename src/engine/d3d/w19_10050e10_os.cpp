// *********************************************************************** //
//
//  w19_10050e10_os.cpp -- two table-only functions, /Os (both pad to 4
//  with a lea/mov filler and then 0x90, the /Os shape):
//
//    0x10050e10 (36)  slot +0x140 of the g_100895b0 view region, the
//                     sibling of d3dhigh.cpp's 0x10050e40 (+0x144);
//    0x1005a350 (56)  slot +0x08 of the vtable at 0x100897a0,
//                     GenList<T>::GenGetNext over d3dr4a.cpp's 16-byte
//                     CMoArray instantiation (Arr1005a).
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  0x10050e10 -- cdecl, three stack arguments, plain `ret`.  d3dhigh.cpp's
//  0x10050e40 with the dword read from p->m_p28->m_n30 instead of p->m_n44:
//  the raw dword goes back through the THIRD pointer, bits 2..6 of it
//  through the SECOND, stored in that order.
// ---------------------------------------------------------------------- //
struct Sub10050e10Inner
{
	uint8   m_Pad00[0x30];                              // +0x00
	uint32  m_n30;                                      // +0x30
};

struct Sub10050e10Object
{
	uint8               m_Pad00[0x28];                  // +0x00
	Sub10050e10Inner*   m_p28;                          // +0x28
};

void sub_10050e10(Sub10050e10Object* pObject, uint32* pOutField,
                  uint32* pOutRaw)
{
	uint32 nValue = pObject->m_p28->m_n30;

	*pOutRaw   = nValue;
	*pOutField = (nValue >> 2) & 0x1f;
}


// ---------------------------------------------------------------------- //
//  0x1005a350 -- GenList<T>::GenGetNext.  d3dr4a.cpp's 0x1005a310 (GenGetAt,
//  `return Get(pos.m_Index)`) plus the `incl (%eax)` on the position AFTER
//  the call; `ret $0x8`, the first dword the hidden result pointer.
//  moarray.h's `return Get(pos.m_Index++);` increments before the call
//  (`xor %ebx,%ebx; inc %ebx; add %edx,%ebx; mov %ebx,(%eax)` ahead of it,
//  measured); taking the reference first and incrementing after is the
//  shipped order, byte-identical.
//  Declarations as d3dr4a.cpp spells them, so Get resolves the same way.
// ---------------------------------------------------------------------- //
struct BaseGenListPos
{
	uint32	m_Index;
};

struct GenListPos : public BaseGenListPos
{
	GenListPos()				{}
	GenListPos(uint32 nIndex)	{ m_Index = nIndex; }

	BaseGenListPos	m_SubIndex;
};

struct T1005a
{
	uint8	m_Data[16];
};

struct Arr1005a
{
	void*		m_pVTable;			// +0x00
	T1005a*		m_pArray;			// +0x04
	uint32		m_nElements;		// +0x08

	T1005a&		sub_1007ee40(uint32 nIndex) const;        // Get (declared only)

	T1005a		sub_1005a350(GenListPos& pos) const;      // GenGetNext
};

T1005a Arr1005a::sub_1005a350(GenListPos& pos) const
{
	T1005a& ret = sub_1007ee40(pos.m_Index);

	pos.m_Index++;
	return ret;
}
