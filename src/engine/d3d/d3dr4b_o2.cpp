// Split out of d3dr4b.cpp: these two need /O2 where the rest of that file
// needs /Os (the same split as d3dr4a_o2.cpp / d3dr5a_o2.cpp).  Measured by
// compiling the whole of d3dr4b.cpp both ways: /Os 30 identical, /O2 22,
// and exactly these two are identical at /O2 and not at /Os.  Files not
// named in build/engine.sh's per-file groups build at /O2, so this file
// needs no build change.
//
//   0x10037470  the destructor-with-delete-flag: /O2 DUPLICATES the
//               `add $0xc,%esp; ret $4` epilogue on the delete arm; /Os
//               jumps back into one (that is its /Os twin 0x10035ae0,
//               which stays in d3dr4b.cpp).
//   0x100506f0  CMoArray<T12>::GenGetAt, forwarding to Get (0x10050790).

#include "ltbasedefs.h"


// genlist.h's GenListPos, as d3dr4b.cpp declares it (two dwords, POD).
struct GenListPos
{
	uint32 m_Index;                                     // +0x00
	uint32 m_SubIndex;                                  // +0x04
};


// ---- 0x100506f0 -- T is 12 bytes; Get is 0x10050790 (stride 12).
struct T100506f0 { uint32 m_v[3]; };

struct Arr100506f0
{
	T100506f0& sub_10050790(uint32 nIndex) const;
	T100506f0  sub_100506f0(GenListPos& pos) const;
};

T100506f0 Arr100506f0::sub_100506f0(GenListPos& pos) const
{
	return sub_10050790(pos.m_Index);
}


// ---- 0x10037470 -- vptr to 0x100894c0, then operator delete if bit 0.
extern void __cdecl sub_10080479(void* pMem);

extern void* const g_vtbl_100894c0[];                   // 0x100894c0

struct C10037470
{
	void* m_pVTable;                                    // +0x00

	void  sub_10037470(uint32 nFlags);
};

void C10037470::sub_10037470(uint32 nFlags)
{
	m_pVTable = (void*)g_vtbl_100894c0;

	if ((nFlags & 1) == 0)
		return;

	sub_10080479(this);
}
