// *********************************************************************** //
//
//  w12_7fae0.cpp -- d3d.ren's default allocator object, g_DefAlloc
//  (0x10091df0), and its three virtuals.
//
//  0x1007faa0..0x1007fb1d is ONE translation unit of Monolith's StdLith
//  library, l_allocator.cpp (src/LT2/lithshared/stdlith/l_allocator.h):
//
//      class LAlloc {
//          virtual       ~LAlloc() {}
//          virtual void* Alloc(DWORD size) { return DefStdlithAlloc(size); }
//          virtual void  Free(void *ptr)   { DefStdlithFree(ptr); }
//      };
//      LAlloc g_DefAlloc;
//
//    0x1007faa0  the static-init entry: call 0x1007fab0; jmp 0x1007fac0
//    0x1007fab0  g_DefAlloc's constructor, inlined: store the vtable
//                0x100881b8 into 0x10091df0
//    0x1007fac0  atexit(0x1007fad0)
//    0x1007fad0  its destructor, inlined: store the vtable again
//    0x1007fae0  LAlloc::Alloc    `push size; call 0x10021ce0; ret $4`
//    0x1007faf0  LAlloc::Free     `push ptr;  call 0x10021cb0; ret $4`
//    0x1007fb00  LAlloc's scalar deleting destructor (vtable slot 0)
//    vtable 0x100881b8 = { 0x1007fb00, 0x1007fae0, 0x1007faf0 }
//
//  It is VC6 code, not icl's: built here exactly as StdLith is (cl 8447,
//  build/libs.sh's flags; build/engine.sh's D3D_CL_TUS), this file is all
//  seven shipped functions, byte for byte -- the three virtuals, and the
//  four initialiser thunks _$E4 (0x1007faa0), _$E1 (0x1007fab0), _$E3
//  (0x1007fac0) and _$E2 (0x1007fad0).  (Compiled by icl, as it was until
//  now under the name CD3DAlloc, the three functions it emitted matched
//  nothing and a ___sti__ initialiser stood where VC6's thunks are.)
//
//  It is NOT linked from src/lithshared/out/StdLith.lib, though: that
//  library's l_allocator.obj also DEFINES DefStdlithAlloc/DefStdlithFree
//  (malloc/free, as the LT2-era source release has them), while in d3d.ren
//  those two are the renderer's own functions -- LAlloc::Alloc calls
//  0x10021ce0 and LAlloc::Free calls 0x10021cb0 (d3d_Free), both routing
//  through the engine's allocator interface at 0x1008cbcc.  Pulling the
//  library member would hand every StdLith allocation to the CRT heap while
//  the renderer frees through the engine.  So only the class and the
//  object are compiled here, and the two calls resolve, like
//  struct_bank.obj's, through link/aliases.rsp onto 0x10021ce0/0x10021cb0.
//
// *********************************************************************** //

#include "l_allocator.h"


// 0x10091df0
LAlloc g_DefAlloc;
