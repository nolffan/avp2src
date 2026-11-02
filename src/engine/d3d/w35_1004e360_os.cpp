// ======================================================================= //
//  w35_1004e360 -- d3d.ren: 0x1004e360 and 0x1004e3d0, moved
//  out of d3dbatcha.cpp.  They are written over the real dynarray.h, which
//  d3dbatcha.cpp cannot include without changing three of its own
//  functions (0x1001cf00, 0x10021a80, 0x1007ebe0 -- measured: 19 -> 17
//  identical there), so they have their own file, /Os as before.
// ======================================================================= //
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


// ======================================================================= //
//  0x1004e360   49 bytes + `8d 76 00`        3 callers
//  0x1004e3d0    7 bytes                     4 callers
//
//  DECODED, and they are two members of one object: 0x1004e3d0 answers the
//  dword at +0x3c and 0x1004e360 bounds-checks against that same +0x3c.
//
//      mov 0x4(%esp),%eax                <- the argument, read BEFORE the
//      push %esi                            dead slot is opened
//      test %eax,%eax / jne  ->  ... / mov %ecx,%eax / ret $0x4
//      dec %eax
//      mov 0x3c(%ecx),%edx / cmp %edx,%eax / jb ->  ... / xor %eax,%eax
//      add %eax,%eax / add %eax,%eax / lea (%eax,%eax,8),%eax
//      add 0x38(%ecx),%eax
//
//  WHAT PINS IT: index 0 answers `this` (`mov %ecx,%eax`), every other index
//  is decremented and checked UNSIGNED (`jb`) against +0x3c, and the element
//  address is base+0x38 plus index*36 -- 36 spelled as ((i*2)*2)*9 in three
//  lea/add, which is how icl multiplies by 36.  Three separate returns, each
//  with its own `add $0x4,%esp` epilogue: a run of guarded returns, not a
//  shared exit.
//
//  IDENTICAL.  +0x34 is a CMoArray<36-byte element> -- vptr
//  +0x34, m_pArray +0x38, m_nElements +0x3c -- and the function is the
//  LOD-style accessor over it: index 0 is the object itself, index i the
//  array's element i-1, bounds-checked through GetSize().  Written over
//  the real dynarray.h (`&m_Array[i]`, CMoArray::operator[] -> Get()) the
//  last block's `add $0x4,%esp` lands after the whole *36 computation as
//  shipped, with a plain `if` for the index-0 test (the `switch` form
//  compiles the same); the hand-modelled pointer and count left that
//  epilogue instruction one slot early.  0x1004e3d0 is its GetSize().
// ======================================================================= //

struct S1004e360Element                                 // 36 bytes
{
	uint8 m_Bytes[36];
};

class C1004e360
{
public:
	void*  sub_1004e360(uint32 nIndex);
	uint32 sub_1004e3d0();

	uint8                       m_Unknown00[0x34];      // +0x00
	CMoArray<S1004e360Element>  m_Array34;              // +0x34 (+0x38 m_pArray, +0x3c m_nElements)
};

void* C1004e360::sub_1004e360(uint32 nIndex)
{
	if (nIndex == 0)
		return this;

	nIndex--;
	if (nIndex >= m_Array34.GetSize())
		return LTNULL;

	return &m_Array34[nIndex];
}

uint32 C1004e360::sub_1004e3d0()
{
	return m_Array34.GetSize();
}
