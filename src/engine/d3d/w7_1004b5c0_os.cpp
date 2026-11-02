// ======================================================================= //
//  w7_1004b5c0_os.cpp -- d3d.ren functions written for the whole-DLL link
//.  /Os: each function here is followed by 0x90 padding.
// ======================================================================= //
#include "ltbasedefs.h"


// ======================================================================= //
//  0x1004b5c0 -- 103 bytes, then nine 0x90.  The screen projection of one
//  clip vertex through the transform object (big4e3e4.cpp's and
//  big49bb0.cpp's callee; 0x1000d940 is its twin):
//
//      rhw = 1/z      (`flds 1.0f ; fdivs 0x8(%eax)`, the dead 1.0f of the
//                      division pair sits before it in the pool)
//      x = (x*m31c + z*m320) * rhw
//      y = (y*m324 + z*m328) * rhw
//      z = (z*m32c + m330)   * rhw
//
//  rhw is held in a register for x and RE-READ from +0x0c for y and z
//  (`fmuls 0xc(%eax)`), which is the store to v->x possibly aliasing
//  v->rhw: the source multiplies by the member each time.  Plain `ret`,
//  (vertex, transform) on the stack: __cdecl.
// ======================================================================= //
struct W7Vert4b5c0
{
	float	x, y, z;                                    // +0x00
	float	rhw;                                        // +0x0c
};

struct W7XForm4b5c0
{
	uint8	m_pad_000[0x31c];                           // +0x000
	float	m_fXX31c;                                   // +0x31c
	float	m_fXZ320;                                   // +0x320
	float	m_fYY324;                                   // +0x324
	float	m_fYZ328;                                   // +0x328
	float	m_fZZ32c;                                   // +0x32c
	float	m_fZ330;                                    // +0x330
};

extern "C" void sub_1004b5c0(W7Vert4b5c0* pVert, W7XForm4b5c0* pXForm)
{
	pVert->rhw = 1.0f / pVert->z;
	pVert->x = (pVert->x * pXForm->m_fXX31c + pVert->z * pXForm->m_fXZ320) * pVert->rhw;
	pVert->y = (pVert->y * pXForm->m_fYY324 + pVert->z * pXForm->m_fYZ328) * pVert->rhw;
	pVert->z = (pVert->z * pXForm->m_fZZ32c + pXForm->m_fZ330) * pVert->rhw;
}


// ======================================================================= //
//  0x10050d00 -- 36 bytes, then twelve 0x90.  The node initialiser
//  0x10050d30 (d3dr2c_os.cpp) runs on a node fresh from its pool: a
//  self-linked pair at +0x00 and two {prev, next, owner} links at +0x10 and
//  +0x1c whose owner is the node (0x10050d60 later unlinks those two with
//  0x1000a6e0).  `mov %ecx,%eax` before the `ret` returns `this`: it is a
//  constructor-shaped initialiser, so it is written returning `this`; the
//  caller's void spelling is aliased onto it in link/aliases.rsp.
// ======================================================================= //
struct W7Link50d00
{
	W7Link50d00*	m_pPrev;                            // +0x00
	W7Link50d00*	m_pNext;                            // +0x04
	void*			m_pData;                            // +0x08

	void	Init(void* pData)
	{
		m_pNext = this;
		m_pPrev = this;
		m_pData = pData;
	}
};

struct W7Node50d00
{
	W7Node50d00*	m_pPrev00;                          // +0x00
	W7Node50d00*	m_pNext04;                          // +0x04
	uint8			m_pad_08[0x10 - 0x08];              // +0x08
	W7Link50d00		m_Link10;                           // +0x10
	W7Link50d00		m_Link1c;                           // +0x1c

	W7Node50d00*	sub_10050d00();
};

W7Node50d00* W7Node50d00::sub_10050d00()
{
	m_Link10.Init(this);
	m_Link1c.Init(this);
	m_pPrev00 = this;
	m_pNext04 = this;
	return this;
}
