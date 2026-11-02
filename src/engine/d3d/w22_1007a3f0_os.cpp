// *********************************************************************** //
//
//  w22_1007a3f0_os.cpp -- d3d.ren functions reached only through vtables,
//  written for the whole-DLL link.  /Os group (w<N>_*_os.cpp, see
//  build/engine.sh).
//
//  0x1007a3f0  PFormat's slot 0 (vtable 0x10089390): the five-argument
//              Init(type, alpha, red, green, blue) big3a080.cpp calls
//              `Slot00`, which fills in each channel's mask, shift and
//              width.
//
//  Measured: byte-identical at /Os; at /O2 the loops are aligned with
//  lea-nops and the constant 1 is `mov $1` (144 bytes against 123).
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  0x1007a3f0 -- 123 bytes, `ret $0x14`, `this` in %ecx.
//
//  THE LAYOUT (0x38 bytes, as big3a080.cpp sizes PFormat): the five
//  arguments land at +0x04..+0x14 in order -- the type, then the alpha,
//  red, green and blue masks (the order all three Slot00 call sites
//  agree on).  The loop walks the four masks (`mov 0x8(%esi),%ebx` with
//  %esi stepping by 4 up to this+0x10) and writes each channel's shift to
//  +0x28+4i and its width to +0x18+4i -- the same +0x08/+0x18/+0x28
//  mask/ramp/shift triple d3dhigh.cpp's 0x10079c70 reads back.
//
//  THE TWO SCANS: the first counts clear bits from bit 0 (%ebp, with the
//  probe bit doubling in %eax, `add %eax,%eax`), the second carries on
//  from there counting set bits (%ecx, seeded with the shift), each
//  capped at 32 steps.  icl drops the first `j < 32` test of the first
//  scan (j is known to be 0) and unrolls it once; the width is the
//  difference of the two counts (`sub %ebp,%ecx`).  The probe doubling is
//  written before the count increment in the first scan and after it in
//  the second: that is the instruction order shipped
//  (`inc %edx; inc %ebp; add %eax,%eax` / `add %eax,%eax; inc %edx;
//  inc %ecx`), and the other order swaps the pairs.
//
//  No return value is set (the last thing in %eax is the probe bit), so
//  the slot is void.
// ---------------------------------------------------------------------- //
class PFormat1007a3f0
{
public:
	void	sub_1007a3f0(uint32 nType, uint32 nAlphaMask, uint32 nRedMask,
	                     uint32 nGreenMask, uint32 nBlueMask);

	void*	m_pVtbl;                                    // +0x00  0x10089390
	uint32	m_nType;                                    // +0x04
	uint32	m_nMask[4];                                 // +0x08  a, r, g, b
	uint32	m_nWidth[4];                                // +0x18
	uint32	m_nShift[4];                                // +0x28
};                                                      // 0x38

void PFormat1007a3f0::sub_1007a3f0(uint32 nType, uint32 nAlphaMask,
                                   uint32 nRedMask, uint32 nGreenMask,
                                   uint32 nBlueMask)
{
	uint32 i, j, nMask, nBit, nShift, nEnd;

	m_nType = nType;
	m_nMask[0] = nAlphaMask;
	m_nMask[1] = nRedMask;
	m_nMask[2] = nGreenMask;
	m_nMask[3] = nBlueMask;

	for (i = 0; i < 4; i++)
	{
		nMask = m_nMask[i];
		nShift = 0;
		nBit = 1;

		for (j = 0; j < 32; j++)
		{
			if (nMask & nBit)
				break;

			nBit += nBit;
			nShift++;
		}

		nEnd = nShift;
		for (j = 0; j < 32; j++)
		{
			if (!(nMask & nBit))
				break;

			nBit += nBit;
			nEnd++;
		}

		m_nShift[i] = nShift;
		m_nWidth[i] = nEnd - nShift;
	}
}
