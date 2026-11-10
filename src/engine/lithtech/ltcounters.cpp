// lithtech.exe — a global counter block (reconstruction). Build /O2.
//
// A translation unit of its own in both images: nothing but this global's
// initializer (AvP2 1.0.9.6 0x004049f0/0x00404a00, after
// clientinterface.cpp's ci_IntersectSegment; Primal Hunt
// 0x004050d0/0x004050e0, between that unit's <string> guard 0x004050a0 and
// its own, 0x00405100). The file name is ours.
#if defined(PRIMAL_HUNT)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): this unit ends in <string>'s
// guarded initializer of std::ctype<unsigned short>::id (0x00405100: guard
// 0x004f8014, atexit of the empty 0x00401790).
#include <string>
#endif
#include "counterblock.h"

// AvP2 1.0.9.6 0x004decb4, Primal Hunt 0x004efe40 (name ours).
CCounterBlockQ423 g_CounterBlockQ423;
