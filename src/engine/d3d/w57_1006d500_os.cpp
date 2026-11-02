// *********************************************************************** //
//  w57_1006d500_os -- d3d.ren: the static initialiser
//  0x1006d500 (17 bytes, init-table entry .data 0x100891e0).
//
//      push %ebp ; mov %esp,%ebp
//      mov $0x10091950,%ecx ; call 0x1006d3a4 ; mov %ebp,%esp ; pop %ebp ; ret
//
//  The construction of the object at 0x10091950 by d3dr2d_os.cpp's
//  constructor sub_1006d3a4 (identical there).  The call reaches the /Qip
//  REGISTER entry (0x1006d3a0 is the `mov 0x4(%esp),%eax` stack shim in
//  front of it) and %eax is never set -- the shim's load, which a
//  zero-argument member never reads: d3dr4b.cpp's "blocked, %eax passed as
//  garbage" note is that, not a missing argument.  The register entry's
//  body itself uses the absolute addresses 0x10091954 / 0x10091968 (the
//  object's two arrays), which is why d3dr2d_os.cpp writes it over the
//  globals; the call is a relocation, so the initialiser is the same bytes
//  whichever entry it names.  The object is defined here with that
//  constructor's own struct tag (only the dword at +0x00 is the object's
//  own: d3ddata.cpp gives 0x10091954.. their own views, as before).
//
//  /Os pads it exactly as shipped (`lea 0x0(%esi),%esi` then 0x90s).
//  IDENTICAL.  New global (defined here): 0x10091950.
// *********************************************************************** //

#include "ltbasedefs.h"

struct sub_1006d3a4
{
	sub_1006d3a4();                                     // 0x1006d3a0

	uint32      m_n00;                                  // +0x00
};

sub_1006d3a4 g_10091950;                                // 0x10091950
