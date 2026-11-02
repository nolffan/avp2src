// ======================================================================= //
//  w35_1007dc54 -- d3d.ren: 0x1007dc54, the constructor of the
//  object whose destructor is 0x1007dcc4 (d3dbatchb_os.cpp): its vptr
//  0x10089a20, a CMoArray at +0x04 (vptr 0x10089a60), then nine dwords.
//
//  Moved out of d3dr4a.cpp, where it was written as hand-unrolled stores
//  (the redundant zero groups spelled out); icl folded those to one group
//  and framed it with `push %esi` (72 bytes, 61 differing).  The shipped
//  shape -- `push %ebp` frame with a 0xc reserve, the three zero groups
//  each re-materialising the zero -- is dynarray.h's CMoArray constructor
//  inlined: Clear() then Init(), the same Construct helper w27_10015e80
//  uses for CD3DModelDraw's three arrays.  It needs dynarray.h, which
//  d3dr4a.cpp cannot include (its own GenList/IAllocator spellings), so it
//  has its own file.  /Os (the `_os` suffix), as d3dr4a was: IDENTICAL.
//  (Shipped it is a /Qip pair -- the 0x1007dc50 shim -- entered at the
//  register entry by 0x1007e0a4; the body at +4 does not need the pair.)
// ======================================================================= //
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"

extern void* g_vtbl_10089a20[];                         // 0x10089a20  the object's own
extern void* g_vtbl_10089a60[];                         // 0x10089a60  CMoArray (+0x04)

struct Elem1007dc54 { uint32 m_n00; };

struct Array1007dc54
{
	void* const*	m_pVtbl;
	void*			m_pArray;
	uint32			m_nElements;
	uint32			m_nCacheSize;
	uint32			m_nWantedCache;
};

template<class T>
inline void Construct1007dc54(CMoArray<T>* pArray, void* const* pVtbl)
{
	Array1007dc54* p = (Array1007dc54*)pArray;

	p->m_pVtbl        = pVtbl;
	p->m_pArray       = 0;
	p->m_nElements    = 0;
	p->m_nCacheSize   = 0;
	p->m_nWantedCache = 0;
	pArray->Init();
}

struct C1007dc54
{
	void*		m_pVTable;			// +0x00
	CMoArray<Elem1007dc54>	m_Array;	// +0x04
	uint32		m_n18;				// +0x18
	uint32		m_n1c;				// +0x1c
	uint32		m_n20;				// +0x20
	uint32		m_n24;				// +0x24
	uint32		m_n28;				// +0x28
	uint32		m_n2c;				// +0x2c
	uint32		m_n30;				// +0x30
	uint32		m_n34;				// +0x34
	uint32		m_n38;				// +0x38

	C1007dc54*	sub_1007dc54();
};

C1007dc54* C1007dc54::sub_1007dc54()
{
	m_pVTable = g_vtbl_10089a20;
	Construct1007dc54(&m_Array, g_vtbl_10089a60);

	m_n18 = 0;
	m_n1c = 0;
	m_n28 = 0;
	m_n2c = 0;
	m_n30 = 0;
	m_n34 = 0;
	m_n38 = 0;

	m_n20 = 0x300;
	m_n24 = 1;

	return this;
}
