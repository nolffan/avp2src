// *********************************************************************** //
//
//  w3_10016234_os.cpp -- written for the d3d.ren link.
//
//  0x10016234: the teardown of the three member arrays at +0x614, +0x628
//  and +0x63c of the global model drawer at 0x1008b090
//  (w27_10015e80_osoiqip.cpp's C1008b090_15e84); d3dr4a.cpp's 0x1001c1b0
//  calls it.
//
//  IDENTICAL: each array is dynarray.h's own CMoArray<T> and
//  is torn down by the template's own Term() -- SetSize2(0, &g_DefAlloc),
//  _DeleteAndDestroyArray, BaseDelete, all inlined -- after its vptr is
//  put back, as the constructor file builds them with the template's
//  Init().  The hand-expanded macro this replaced (three nested tests,
//  one allocator pointer) was 96 bytes off: it loaded g_DefAlloc's vptr
//  by absolute address where the inlined BaseDelete, holding the
//  allocator in %ecx as its argument from the SetSize2 level down, reads
//  it through %ecx.  (icl also emits the templates' out-of-line
//  SetSize2 / _DeleteAndDestroyArray / _AllocateTArray as unreferenced
//  COMDATs, which the link drops.)
//
//  WHAT THE SHIPPED FUNCTION IS (measured with a probe): the drawer's
//  real C++ destructor.  icl gives a destructor a hidden int argument,
//  so at /Qip its external copy is a two-entry pair -- the `mov
//  0x4(%esp),%eax` shim at 0x10016230, the body at 0x10016234 ignoring
//  %eax -- and the atexit thunk 0x1001c1b0 (`mov $obj,%ecx ; call`) is
//  compiler-generated, entering at the register entry.  A probe with a
//  global of a class with an out-of-line destructor reproduces
//  0x1001c1b0 byte for byte.  So 0x10016230 needs this class's
//  destructor in one /Qip TU with the global's definition (the
//  constructor's TU, w27_10015e80_osoiqip.cpp); not done here -- the
//  shim is unwritten and 0x1001c1b0 stays d3dr4a.cpp's explicit call.
//
//  /Os: the body's `push %ebx` after the frame.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


// The three arrays' vtables (GenList slots: 0x1001d240 GenBegin,
// 0x1001c760 GenIsValid, ... -- a CMoArray<T> per table), and their
// element types, spelled as w27_10015e80_osoiqip.cpp spells them.
extern void* const g_vtbl_100892e0[];                   // 0x100892e0  +0x614's vtable
extern void* const g_vtbl_10089310[];                   // 0x10089310  +0x628's vtable
extern void* const g_vtbl_10089340[];                   // 0x10089340  +0x63c's vtable

struct Elem614_15e84 { uint32 m_n00; };
struct Elem628_15e84 { uint32 m_n00; };
struct Elem63c_15e84 { uint32 m_n00; };

// ~CMoArray(): the vptr back, then Term().
template<class T>
inline void Destruct16234(CMoArray<T>* pArray, void* const* pVtbl)
{
	*(void* const**)pArray = pVtbl;
	pArray->Term();
}

class C1008b090
{
public:
	void sub_10016234();                                // 0x10016234

	uint8                       m_pad_000[0x614];
	CMoArray<Elem614_15e84>     m_Array614;             // +0x614
	CMoArray<Elem628_15e84>     m_Array628;             // +0x628
	CMoArray<Elem63c_15e84>     m_Array63c;             // +0x63c
};


// ----------------------------------------------------------------------- //
//  0x10016234 -- the members are destroyed in reverse order of
//  declaration: +0x63c, +0x628, +0x614.
// ----------------------------------------------------------------------- //
void C1008b090::sub_10016234()
{
	Destruct16234(&m_Array63c, g_vtbl_10089340);
	Destruct16234(&m_Array628, g_vtbl_10089310);
	Destruct16234(&m_Array614, g_vtbl_100892e0);
}
