// *********************************************************************** //
//
//  MODULE:   W15_10021E50_OS (.CPP)
//
//  PURPOSE:  0x10021e50 -- push every renderer console variable's live
//            value out to its record and to the record's destinations
//            (big21f90.cpp's chain at 0x1008bf90), then 0x100316e0 (the
//            device fog/dither/filter states), and force LightMap off when
//            0x1008cba8 is set.  Called at the top of 0x1001eb90's frame
//            setup and from 0x100215bb; also installed at 0x10021554.
//
//  BYTE-IDENTICAL at /Os (the plain 0x90 padding).  The int value is
//  RoundF of the float -- the inlined __asm `fld ; fistp` helper whose
//  out-of-line copy is 0x10015190 -- which is what gives the shipped
//  `fstps tmp ; flds tmp ; fistpl`, the %ebp frame and the unused saved
//  %edi; a C cast (with or without /Qrcd) gives fistpll.
//
// *********************************************************************** //

#include "ltbasedefs.h"

// The console-variable record (big21f90.cpp's CD3DConVar), spelled with
// the type name the chain head 0x1008bf90 already links under.
struct Var10021dc0
{
	int32         m_nValue;                             // +0x00 value as int
	float         m_fValue;                             // +0x04 value as float
	float         m_fDefault;                           // +0x08
	int32*        m_pnValue;                            // +0x0c int32 destination
	float*        m_pfValue;                            // +0x10 float destination
	const char*   m_pszName;                            // +0x14
	void*         m_pVar;                               // +0x18 console handle
	Var10021dc0*  m_pNext;                              // +0x1c
};

struct RenderStructCallbacks
{
	uint8    m_Reserved00[0x1c];                        // +0x00..+0x1b
	float   (*GetVarValueFloat)(void* pVar);            // +0x1c
};

extern Var10021dc0*           g_1008bf90;               // 0x1008bf90
extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc
extern uint32                 g_1008cba8;               // 0x1008cba8
extern long                   g_CV_LightMap;            // 0x10089400   LightMap

extern void sub_100316e0();

// Round to nearest in the current FPU mode (0x10015190 out of line; see
// d3dbatchd_o2.cpp and w1_10001000.cpp).  icl inlines it, and the inlined
// copy is the shipped `fstps tmp ; flds tmp ; fistpl n` with the %ebp frame.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}                             // 0x100316e0


// ======================================================================= //
//  0x10021e50 -- 115 bytes.
// ======================================================================= //
void sub_10021e50()
{
	Var10021dc0* pVar;

	for (pVar = g_1008bf90; pVar; pVar = pVar->m_pNext)
	{
		pVar->m_fValue = g_pStruct->GetVarValueFloat(pVar->m_pVar);
		pVar->m_nValue = RoundF(pVar->m_fValue);

		if (pVar->m_pfValue)
			*pVar->m_pfValue = pVar->m_fValue;
		if (pVar->m_pnValue)
			*pVar->m_pnValue = pVar->m_nValue;
	}

	sub_100316e0();

	if (g_1008cba8)
		g_CV_LightMap = 0;
}
