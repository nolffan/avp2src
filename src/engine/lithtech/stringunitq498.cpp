// lithtech.exe -- a translation unit with nothing in it but the STL's
// <string> (reconstruction; the file name is ours).
//
// Primal Hunt's engine has two units whose whole contribution to .text is
// <string>'s guarded initializer of std::ctype<unsigned short>::id (guard
// 0x004f8014, atexit of the empty 0x00401790): 0x00498020, straight after
// the guard that closes the unit before it (0x00497ff0) and before
// CClientShell's code, and 0x004a73f0, straight after transformmaker.cpp's
// (0x004a73c0). Whatever else such a unit held was inline or template code
// placed with another unit, or data. This is one of them; AvP2 1.0.9.6's
// engine (STLport, which has no such initializer) has nothing of it.
#if defined(PRIMAL_HUNT)
#include <string>
#endif
