// *********************************************************************** //
//
//  w17_1001c760_os.cpp -- two vtable-table targets.
//  /Os (see build/engine.sh): both are padded with 0x90s, the /Os pad.
//
//    * 0x1001c760 -- CMoArray's GenIsValid (genlist.h: `virtual BOOL
//      GenIsValid(const GenListPos &pos) const`), the slot +0x04 entry of
//      the six array vtables at 0x100892e0, 0x10089310, 0x10089340,
//      0x100897a0, 0x10089a60 and 0x10089a90 (see d3dr4b.cpp's banner):
//      the position is valid while its index is below GetSize(), which is
//      the out-of-line 0x1007ee60 (d3dbatchb.cpp's Obj1007ee60).
//    * 0x100591e0 -- the second of the four per-stage texture-coordinate
//      functions at 0x100896f8 (w7_10056c60_os.cpp's one-stage vertex fns,
//      beside d3dr4c.cpp's 0x100591a0, d3dr5b.cpp's 0x10059210 and
//      d3dr4a.cpp's 0x10059270): the vertex's own u, v copied through.
//      `this` and the stage index are not read; the two dead register
//      saves in its 0xc frame are icl's (d3dneutral.cpp, point 4).
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  0x1001c760
// ======================================================================= //
struct GenListPos1001c760
{
	uint32	m_Index;                                    // +0x00
};

// d3dbatchb.cpp's spelling of the array whose GetSize is 0x1007ee60.
struct Obj1007ee60
{
	uint32  m_n00;                                      // +0x00
	uint32  m_n04;                                      // +0x04
	uint32  m_n08;                                      // +0x08

	uint32 sub_1007ee60() const;
};

class CGenArray1001c760 : public Obj1007ee60
{
public:
	LTBOOL	sub_1001c760(const GenListPos1001c760& pos) const;
};

LTBOOL CGenArray1001c760::sub_1001c760(const GenListPos1001c760& pos) const
{
	return pos.m_Index < sub_1007ee60();
}


// ======================================================================= //
//  0x100591e0
// ======================================================================= //
struct Src100591e0
{
	float	m_f00;                                      // +0x00
	float	m_f04;                                      // +0x04
	float	m_f08;                                      // +0x08
	float	m_fU0c;                                     // +0x0c
	float	m_fV10;                                     // +0x10
};

struct Dest100591e0
{
	float	m_fU00;                                     // +0x00
	float	m_fV04;                                     // +0x04
};

class C100591e0
{
public:
	void	sub_100591e0(const Src100591e0* pSrc, Dest100591e0* pDest,
	                     uint32 nIndex);
};

void C100591e0::sub_100591e0(const Src100591e0* pSrc, Dest100591e0* pDest,
                             uint32 nIndex)
{
	pDest->m_fU00 = pSrc->m_fU0c;
	pDest->m_fV04 = pSrc->m_fV10;
}
