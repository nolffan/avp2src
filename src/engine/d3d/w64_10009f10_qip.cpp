// *********************************************************************** //
//
//  MODULE:   W64_10009F10_QIP (.CPP) -- stdlith's CMoArray over DrawPolyMgr's
//            saved vertex (SavedVert589c4, 16 bytes), as the /O2 /Qip TU
//            that owns 0x10009c90..0x1000a69f emitted it.
//
//  BUILD:    /O2 /Qip (the _qip convention).
//
//  The array's members are reached from its virtuals (d3dr5b.cpp,
//  /Os /Qip, 0x1005a0b0..0x1005a386): GenAppendList -> NiceSetSize
//  (0x1005a390) -> InternalNiceSetSize, GenCopyList -> SetSize
//  (0x1000a1f0) -> SetSize2 (0x1000a110), GenRemoveAt -> Remove (0x1000a3c0)
//  -> Remove2 (0x1000a220), Append (0x1005a3e0) -> Insert (0x1000a600) ->
//  Insert2 (0x1000a3f0), and Term (0x1000a630).  Those non-virtual bodies
//  are NOT the /Os /Qip shapes: the shipped linker took them from an earlier
//  TU built /O2 /Qip.  Measured with the stock dynarray.h and this element
//  type, every one of 0x10009c90 GetNumAllocatedElements, 0x10009f10
//  InternalNiceSetSize (449 bytes), 0x1000a0e0 _AllocateTArray, 0x1000a110
//  SetSize2, 0x1000a1f0 SetSize, 0x1000a220 Remove2, 0x1000a3c0 Remove,
//  0x1000a3f0 Insert2, 0x1000a600 Insert, 0x1000a630 Term and the four
//  DefaultCache accessors 0x1000a660..0x1000a690 is BYTE-IDENTICAL at
//  /O2 /Qip; at /O2 alone InternalNiceSetSize, SetSize2, Remove2 and Insert2
//  are 272..422 bytes off, at /Os /Qip InternalNiceSetSize is 406 bytes.
//
//  What the rest of that TU was is not known, so only the member nothing
//  else of ours defines as the template is instantiated here, explicitly:
//  InternalNiceSetSize, until now d3dr5d.cpp's hand-transcribed stand-in
//  Arr10009f10::InternalNiceSetSize (its callers' spellings are aliased onto
//  this symbol in link/aliases.rsp).  The others are still stand-ins of
//  their own (w2_1000a110.cpp, d3dr2a.cpp, d3dr2b.cpp ...).
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


// The saved vertex, as d3dr5b.cpp and w27_10056ab0_osqipni.cpp define it.
struct SavedVert589c4                                   // 0x10 bytes
{
	LTVector	m_Pos;                                  // +0x00
	uint32		m_n0c;                                  // +0x0c
};


// 0x10009f10
template BOOL CMoArray<SavedVert589c4>::InternalNiceSetSize(DWORD newSize, BOOL bFast, LAlloc* pAlloc);
