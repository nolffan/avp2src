// *********************************************************************** //
//
//  MODULE:   W18_10035A70_OSOIQIP (.CPP)  -- /Os /Oi- /Qip
//
//  PURPOSE:  Two slots of the DirectDraw surface wrapper's vtable 0x10089480
//            (d3dr2c.cpp's C10035bb4 constructs it: +0x04 width, +0x08
//            height, +0x0c the IDirectDrawSurface7*), written for the
//            d3d-link:
//
//    slot +0x1c  0x10035bd0  Term: release the surface and zero the size.
//    slot +0x00  0x10035a70  the scalar deleting destructor, which calls it.
//
//  WHY /Qip, AND WHY ONE FILE.  0x10035bd0 is icl's external two-entry
//  pair for a no-argument __thiscall member: a vestigial four-byte stack
//  entry `mov 0x4(%esp),%eax` falling into the register entry at
//  0x10035bd4 (a plain `ret`), and the destructor calls 0x10035bd4 -- the
//  register entry -- which only a same-TU caller does.  Measured: a
//  no-argument member under /Qip emits exactly that four-byte entry
//  (0x10035bb0, the constructor, has the same one), and /Qip does not
//  inline it into the destructor.
//
//  0x10035bd4 was written on its own in d3dr2b.cpp (/Os, no /Qip, also
//  byte-identical there).  The pair here IS that function plus the stack
//  entry the vtable needs, so d3dr2b.cpp's copy was removed (one body per
//  shipped function); nothing referenced it by name.
//
//  FLAGS: /Os padding after both (`90`s, `8b f6`); nothing needs an
//  intrinsic, so /Os /Oi- /Qip is /Os /Qip here.
//
// *********************************************************************** //

#include "ltbasedefs.h"

// The IDirectDrawSurface7 slot reached here (+0x08, Release), spelled
// the way d3dr2b.cpp spells it.
struct DDSurface;

struct DDSurfaceVtbl
{
	void*  _QueryInterface;                             // +0x00
	void*  _AddRef;                                     // +0x04
	uint32 (__stdcall* Release)(DDSurface*);            // +0x08
};

struct DDSurface
{
	DDSurfaceVtbl* lpVtbl;
};

extern void* g_vtbl_10089480[];                         // 0x10089480
extern void* const g_vtbl_10089450[];                   // 0x10089450

void sub_10080479(void* p);                             // operator delete

struct C10035a70
{
	void*      m_pVTable;                               // +0x00
	uint32     m_nWidth;                                // +0x04
	uint32     m_nHeight;                               // +0x08
	DDSurface* m_pSurface;                              // +0x0c

	void   sub_10035bd0();                              // slot +0x1c
	void   sub_10035a70(uint32 nFlags);                 // slot +0x00
};

// ---------------------------------------------------------------------- //
//  0x10035bd0 -- 4-byte stack entry + 42-byte register body at 0x10035bd4.
//  Release the surface if there is one and forget it, then zero the size
//  (+0x08 is stored before +0x04).
// ---------------------------------------------------------------------- //
void C10035a70::sub_10035bd0()
{
	DDSurface* p = m_pSurface;

	if (p)
	{
		DDSurfaceVtbl* pVtbl = p->lpVtbl;

		pVtbl->Release(p);
		m_pSurface = LTNULL;
	}

	m_nWidth  = 0;
	m_nHeight = 0;
}

// ---------------------------------------------------------------------- //
//  0x10035a70 -- 55 bytes.  The scalar deleting destructor: re-store this
//  class's vtable, Term (the register entry, `this` in %ecx), then the
//  base's vtable 0x10089450, then operator delete on bit 0.  The two vptr
//  stores are member stores, as d3dr2c.cpp does for the constructor, so
//  that no vtable of ours is emitted.
// ---------------------------------------------------------------------- //
void C10035a70::sub_10035a70(uint32 nFlags)
{
	m_pVTable = g_vtbl_10089480;
	sub_10035bd0();
	m_pVTable = (void*)g_vtbl_10089450;

	if ((nFlags & 1) == 0)
		return;

	sub_10080479(this);
}
