// *********************************************************************** //
//
//  w19_10050450.cpp -- 0x10050450 (236 bytes), GenList<T>::GenAppendList
//  for stdlith's CMoArray<T> with a twelve-byte T.  /O2 (the loop top is
//  aligned with `mov %esi,%esi; lea 0x0(%edi,%eiz,1),%edi`, /O2's filler).
//
//  Reached only through the g_100895b0 view region (+0x104 and +0x134):
//  slot +0x24 of the two GenList vtable groups whose +0x20 is 0x10050560
//  (d3dr5c_o2.cpp's Arr10050560::sub_10050560, GenCopyList).  This is that
//  function's sibling, exactly as 0x1007c480 is for the four-byte T:
//
//    * GetSize (0x1005f780) comes FIRST and is added to theList's
//      GenGetSize (+0x1c); the resize is 0x100507b0 -- d3dr2b_o2.cpp's
//      Arr100507b0::sub_100507b0, the NiceSetSize wrapper -- and the
//      counter starts at the old size (`mov %ebx,%edi`, no `xor`);
//    * the copy loop is GenCopyList's: GenBegin (+0x00), GenIsValid
//      (+0x04), GenGetNext (+0x08) returned as three `mov`s into
//      operator[] (0x100507e0), and the "BAD bug in theList" arm calls
//      Term (0x1002ec30) with &g_DefAlloc and answers FALSE.
//
//  The declarations are d3dr5c_o2.cpp's, so every callee resolves to the
//  symbol that file already links against; NiceSetSize is reached through
//  d3dr2b_o2.cpp's own spelling.
//
// *********************************************************************** //

typedef unsigned long uint32;

#define LTTRUE  1
#define LTFALSE 0

class LAllocR5C
{
public:
	virtual				~LAllocR5C();                   // vtable +0x00
	virtual void*		Alloc(uint32 nSize);            // vtable +0x04
	virtual void		Free(void* pMem);               // vtable +0x08
};

extern LAllocR5C g_10091df0;                            // 0x10091df0 g_DefAlloc

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

struct T10050560
{
	uint32	m_v[3];
};

struct GenList10050560
{
	virtual GenListPosR5C	GenBegin() const = 0;                       // +0x00
	virtual uint32			GenIsValid(const GenListPosR5C& pos) const = 0;
	virtual T10050560		GenGetNext(GenListPosR5C& pos) const = 0;   // +0x08
	virtual T10050560		GenGetAt(GenListPosR5C& pos) const = 0;
	virtual uint32			GenAppend(T10050560& toAppend) = 0;
	virtual void			GenRemoveAt(GenListPosR5C pos) = 0;
	virtual void			GenRemoveAll() = 0;
	virtual uint32			GenGetSize() const = 0;                     // +0x1c
	virtual uint32			GenCopyList(const GenList10050560& other) = 0;
	virtual uint32			GenAppendList(const GenList10050560& other) = 0;
	virtual uint32			GenFindElement(const T10050560& toFind,
										   GenListPosR5C& thePos) const = 0;
	virtual void			GenSetCacheSize(uint32 nSize) = 0;
};

// 0x100507b0 -- NiceSetSize, d3dr2b_o2.cpp's spelling.
struct IAllocator;
struct Arr100507b0
{
	uint32 sub_1002e4c0(uint32 nNewSize, uint32 bFast, IAllocator* pAlloc);
	uint32 sub_100507b0(uint32 nNewSize);
};

struct Arr10050560
{
	// Declared only (d3dr5c_o2.cpp's declarations).
	uint32		sub_1005f780() const;                   // GetSize
	uint32		sub_1002ec30(LAllocR5C* pAlloc = &g_10091df0);  // Term
	T10050560&	sub_100507e0(uint32 nIndex) const;      // operator[]

	uint32		sub_100507b0(uint32 nNewSize)           // NiceSetSize
	{
		return ((Arr100507b0*)this)->sub_100507b0(nNewSize);
	}

	uint32		sub_10050450(const GenList10050560& theList);
};

uint32 Arr10050560::sub_10050450(const GenList10050560& theList)
{
	GenListPosR5C	pos;
	uint32			iCurOut, prevSize;

	prevSize = sub_1005f780();

	if(!sub_100507b0(prevSize + theList.GenGetSize()))
		return LTFALSE;

	iCurOut = prevSize;
	for(pos=theList.GenBegin(); theList.GenIsValid(pos); )
	{
		// BAD bug in theList!
		if(iCurOut >= sub_1005f780())
		{
			sub_1002ec30();
			return LTFALSE;
		}

		sub_100507e0(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}
