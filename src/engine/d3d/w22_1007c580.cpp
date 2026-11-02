// *********************************************************************** //
//
//  w22_1007c580.cpp -- d3d.ren functions reached only through vtables,
//  written for the whole-DLL link.  /O2 (the w<N>_*.cpp default).
//
//  0x1007c580  GenList<void*>::GenCopyList, slot +0x20 of the
//              CMoArray<void*> vtable 0x100899f0.
//
//  Why /O2: its slot +0x24 twin 0x1007c480 (GenAppendList, the same
//  instantiation) is byte-identical at /O2 in d3dr5c_o2.cpp and not at
//  /Os, and 0x1007c580 has the same /O2 marks -- the three callee-saves
//  stored with `mov` into the frame, the epilogue duplicated on every
//  exit, and the loop head aligned with `nop`.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// 0x10091df0 -- stdlith's g_DefAlloc, declared as d3dr5c_o2.cpp declares it.
class LAllocR5C
{
public:
	virtual				~LAllocR5C();                   // vtable +0x00
	virtual void*		Alloc(uint32 nSize);            // vtable +0x04
	virtual void		Free(void* pMem);               // vtable +0x08
};

extern LAllocR5C g_10091df0;


// genlist.h's GenListPos, spelled as d3dr5c_o2.cpp spells it: the
// `pos = theList.GenBegin()` assignment is two `mov`s (-0x14/-0x10 into
// -0xc/-0x8), not a `rep movsl`.
struct BaseGenListPosR5C
{
	uint32	m_Index;
};

struct GenListPosR5C : public BaseGenListPosR5C
{
	GenListPosR5C()					{}
	GenListPosR5C(uint32 nIndex)	{ m_Index = nIndex; }

	BaseGenListPosR5C	m_SubIndex;
};


typedef void* T1007c480;

struct GenList1007c480
{
	virtual GenListPosR5C	GenBegin() const = 0;                       // +0x00
	virtual uint32			GenIsValid(const GenListPosR5C& pos) const = 0;
	virtual T1007c480		GenGetNext(GenListPosR5C& pos) const = 0;   // +0x08
	virtual T1007c480		GenGetAt(GenListPosR5C& pos) const = 0;
	virtual uint32			GenAppend(T1007c480& toAppend) = 0;
	virtual void			GenRemoveAt(GenListPosR5C pos) = 0;
	virtual void			GenRemoveAll() = 0;
	virtual uint32			GenGetSize() const = 0;                     // +0x1c
	virtual uint32			GenCopyList(const GenList1007c480& other) = 0;
	virtual uint32			GenAppendList(const GenList1007c480& other) = 0;
	virtual uint32			GenFindElement(const T1007c480& toFind,
										   GenListPosR5C& thePos) const = 0;
	virtual void			GenSetCacheSize(uint32 nSize) = 0;
};


// ---------------------------------------------------------------------- //
//  0x1007c580 -- 233 bytes, `ret $0x4`.  In .data at 0x10089a10: slot
//  +0x20 of the 0x100899f0 group, GenCopyList.
//
//  dynarray.h's GenCopyList: theList.GenGetSize() (+0x1c) FIRST, handed
//  to SetSize (0x1007c840, `SetSize2(n, &g_DefAlloc)`); the counter
//  starts at 0 (`xor %esi,%esi` in the GenBegin block); the loop's
//  `iCurOut >= GetSize()` (0x1005f780) arm plants &g_DefAlloc for Term
//  (0x1007ca30) and returns 0; the element store is `mov %edi,(%eax)`
//  through operator[] (0x1007c870) after GenGetNext returns the pointer
//  in %eax.  The out-of-line arm at 0x1007c644 (reached only by the
//  `jae` at 0x1007c5fa) is part of this body.
//
//  Arr1007c480 is d3dr5c_o2.cpp's spelling of this instantiation, so the
//  GetSize / operator[] / Term references resolve through the aliases
//  already in link/aliases.rsp.
// ---------------------------------------------------------------------- //
struct Arr1007c480
{
	uint8		m_pad_00[0x14];

	uint32		sub_1005f780() const;                   // GetSize
	uint32		sub_1007c840(uint32 nNewSize);          // SetSize
	void		sub_1007ca30(LAllocR5C* pAlloc = &g_10091df0);  // Term
	T1007c480&	sub_1007c870(uint32 nIndex) const;      // operator[]

	uint32		sub_1007c580(const GenList1007c480& theList);
};

uint32 Arr1007c480::sub_1007c580(const GenList1007c480& theList)
{
	GenListPosR5C	pos;
	uint32			iCurOut;

	if(!sub_1007c840(theList.GenGetSize()))
		return LTFALSE;

	iCurOut = 0;
	for(pos=theList.GenBegin(); theList.GenIsValid(pos); )
	{
		// BAD bug in theList!
		if(iCurOut >= sub_1005f780())
		{
			sub_1007ca30();
			return LTFALSE;
		}

		sub_1007c870(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}
