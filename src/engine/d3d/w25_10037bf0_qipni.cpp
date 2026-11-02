// *********************************************************************** //
//
//  w25_10037bf0_qipni.cpp -- the texture-format table: 0x10037bf0 (the
//  channel-shift helper), 0x10037cb0 (the EnumTextureFormats callback
//  that records every format the device offers) and 0x10037f30 (the
//  picker that chooses the seven formats the renderer uses).
//
//  FLAGS: /O2 /Qip /Qip_no_inlining.  All three end in multi-byte nop
//  padding to 16 (/O2).
//  0x10037bf0 has no stack entry: 0x10037cb0 calls it ten times with the
//  first three arguments in %eax/%edx/%ecx and only the fourth written to
//  the outgoing area (`mov %edx,0xc(%esp)` at 0x10037d83), and the callee
//  reads it back at 0x20(%esp) behind four pushes -- icl's /Qip register
//  convention for a static, so caller and callee are one /Qip TU.
//
//  0x1003ad30 (d3dqip2.cpp) calls 0x10037f30 with no arguments (a plain
//  `call`, nothing in registers) and passes 0x10037cb0 by address to
//  EnumTextureFormats (__stdcall, `ret $0x8`), so neither needs to share
//  d3dqip2.cpp's TU.
//
//  WHY _qipni (measured): under plain /O2 /Qip icl expands 0x10037bf0 into
//  all ten call sites (0x10037cb0 grew to 2107 bytes).  /Qip_no_inlining
//  keeps it out of line with the register call, as shipped; the explicit
//  `inline` helpers below are still expanded.  (Plain /Qip with the helper
//  defined AFTER its caller also keeps it out of line and gives the same
//  bytes, but emits it after 0x10037cb0.)
//
//  RESULTS: 0x10037cb0 identical (the DDPIXELFORMAT copy is a memcpy: a
//  struct assignment forwards the flags word instead of re-reading it from
//  the record).  0x10037f30 near, 1579 = 1579 bytes, 8 differ
//  (was 67 with the two single specs filled by an inline Set member).
//  The single specs are ONE-ELEMENT ARRAYS with aggregate initialisers:
//  icl copies a multi-element array from its .data template but fills a
//  one-element one with six immediate stores, in member order -- exactly
//  the shipped 8888 and bump stores (a plain struct initialiser copies
//  from a template instead, 1307 off).  What is left is the first lookup
//  (8888, nSpecs 1): shipped loads the spec's flags before the record's,
//  ours after (`test` operands swapped, 4 instructions).  Measured: the
//  `&` operand order (either way, no change), a flags local, `!= 0`, a
//  pointer loop (much worse); with the bump spec filled by Set again the
//  first lookup comes out as shipped but the bump stores pair by register
//  (720 Set bodies, best 4 off; Set parameter order changes nothing; 720
//  orders of plain stores, best 12).
//  IDENTICAL with DDPIXELFORMAT's dwFlags read as a SIGNED
//  int (the int32/uint32 lever): the first lookup's `test` then loads the
//  spec's flags first, as shipped, and nothing else moves (0x10037cb0
//  stays identical).  ddraw.h spells the field DWORD; this TU's copy of
//  the struct is the only place the signedness is stated, so it is
//  recorded here as the measured choice, not as a claim about ddraw.h.
//  0x10037bf0 IDENTICAL, see MaskStart; before,
//  near, 199 vs 176: same loops and
//  shuffles, but icl keeps the stack-passed fourth argument in %edi and
//  spills a count (an 8-byte frame); shipped leaves it in memory.  Loop
//  spellings, a helper for the mask end, branch orders, separate bit
//  variables and int counters all measured, none changes the allocation.
//
// *********************************************************************** //

#include "ltbasedefs.h"

#include <string.h>

struct DDPixelFormat
{
	uint32 dwSize;                                      // +0x00
	int32  dwFlags;                                     // +0x04  signed: see 0x10037f30
	uint32 dwFourCC;                                    // +0x08
	uint32 dwRGBBitCount;                               // +0x0c
	uint32 dwRBitMask;                                  // +0x10
	uint32 dwGBitMask;                                  // +0x14
	uint32 dwBBitMask;                                  // +0x18
	uint32 dwRGBAlphaBitMask;                           // +0x1c
};                                                      // 0x20

#define DDPF_ALPHAPIXELS                0x00000001
#define DDPF_FOURCC                     0x00000004
#define DDPF_PALETTEINDEXED8            0x00000020
#define DDPF_RGB                        0x00000040
#define DDPF_LUMINANCE                  0x00020000
#define DDPF_BUMPDUDV                   0x00080000

#define FOURCC_DXT1                     0x31545844
#define FOURCC_DXT3                     0x33545844
#define FOURCC_DXT5                     0x35545844

// stdlith's LTLink (d3dr5d.cpp's D3DLink): the format records are on a
// circular list whose head is the two dwords at 0x1008e6a4.
struct D3DLink
{
	D3DLink* m_pPrev;                                   // +0x00
	D3DLink* m_pNext;                                   // +0x04
	void*    m_pData;                                   // +0x08
};

// One format record, 0xa4 bytes (the d3d_Alloc size at 0x10037cb5).  The
// other TUs that read g_1008e6b0 spell only the prefix they use.
struct D3DFormatRec
{
	D3DLink       m_Link;                               // +0x00
	DDPixelFormat m_ddpf;                               // +0x0c..+0x2b
	uint32        m_nPixelShift;                        // +0x2c
	uint32        m_nPixelSize;                         // +0x30
	uint32        m_nRBits;                             // +0x34
	uint32        m_nGBits;                             // +0x38
	uint32        m_nBBits;                             // +0x3c
	uint32        m_nABits;                             // +0x40
	uint32        m_nShift8888[4][2];                   // +0x44 (right, left) R,G,B,A
	uint32        m_nShift565[3][2];                    // +0x64 R,G,B
	uint32        m_nShift555[3][2];                    // +0x7c R,G,B
	uint16        m_wMask[4];                           // +0x94 R,G,B,A
	uint32        m_b1555;                              // +0x9c
	uint32        m_b565;                               // +0xa0
};                                                      // 0xa4

// One entry of the picker's wish lists, 0x18 bytes.  The alpha count is
// carried but never compared.
struct FormatSpec
{
	uint32 m_nRBits;                                    // +0x00
	uint32 m_nGBits;                                    // +0x04
	uint32 m_nBBits;                                    // +0x08
	uint32 m_nABits;                                    // +0x0c
	uint32 m_dwFlagsOn;                                 // +0x10
	uint32 m_dwFlagsOff;                                // +0x14
};

extern D3DLink       g_List1008e6a4;                    // 0x1008e6a4
extern D3DFormatRec* g_1008e6b0[7];                     // 0x1008e6b0
extern uint32        g_1008d978;                        // 0x1008d978  lightmapping enabled
extern uint32        g_1008da60;                        // 0x1008da60  DXT5 supported
extern uint32        g_1008da64;                        // 0x1008da64  DXT3 supported
extern uint32        g_1008da68;                        // 0x1008da68  DXT1 supported

extern void*  sub_10021c20(uint32 nBytes);              // 0x10021c20 d3d_Alloc
// 0x10021f10 counts the set bits (d3dbatcha.cpp).  The callers here
// zero-extend its result (`movzwl %ax,%eax` after each of the four calls
// at 0x10037d44..0x10037d77), so this TU's prototype answers a WORD.
extern uint16 sub_10021f10(uint32 nBits);               // 0x10021f10
extern void   sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60


// ======================================================================= //
//  0x10037bf0 -- 176 bytes.  The shift between two channel masks: find
//  the bit just past each mask's run of ones and hand the difference to
//  whichever out-pointer points the right way (the other gets 0).
// ======================================================================= //
// The run of zeros below the mask is its own inline helper
// stepping the probe bit THROUGH A POINTER (the pointer-to-inline-helper
// lever); with it icl hands the two loops' counters to different
// registers as shipped, and 0x10037bf0 is byte-identical (was 187 off).
// Both loops as such helpers, or both inline with separate counters, do
// not reproduce it (283 / 187 off).
static inline uint32 MaskStart(uint32 nMask, uint32* pBit)
{
	uint32 i, n = 0;

	for (i = 0; i < 32; i++)
	{
		if (nMask & *pBit)
			break;
		*pBit <<= 1;
		n++;
	}

	return n;
}

static inline uint32 MaskEnd(uint32 nMask)
{
	uint32 nBit, nEnd, i;

	nBit = 1;
	nEnd = MaskStart(nMask, &nBit);
	for (i = 0; i < 32; i++)
	{
		if (!(nMask & nBit))
			break;
		nBit <<= 1;
		nEnd++;
	}

	return nEnd;
}

static void sub_10037bf0(uint32 nMaskA, uint32 nMaskB, uint32* pRight,
                         uint32* pLeft)
{
	uint32 nEndA, nEndB;

	nEndA = MaskEnd(nMaskA);
	nEndB = MaskEnd(nMaskB);

	if (nEndA > nEndB)
	{
		*pRight = nEndA - nEndB;
		*pLeft = 0;
	}
	else
	{
		*pRight = 0;
		*pLeft = nEndB - nEndA;
	}
}


// ======================================================================= //
//  0x10037cb0 -- 640 bytes.  EnumTextureFormats callback: record the
//  format with its pixel size, channel bit counts, the shifts from 8888,
//  565 and 555 into it, its masks and whether it is exactly 1555 or 565,
//  and put it at the head of the list.  Always continues (DDENUMRET_OK).
// ======================================================================= //
long __stdcall sub_10037cb0(void* pFormat, void* pContext)
{
	DDPixelFormat* pPF = (DDPixelFormat*)pFormat;
	D3DFormatRec*  pRec;

	pRec = (D3DFormatRec*)sub_10021c20(sizeof(D3DFormatRec));
	if (pRec)
	{
		memcpy(&pRec->m_ddpf, pPF, sizeof(DDPixelFormat));

		if (pRec->m_ddpf.dwFlags & DDPF_PALETTEINDEXED8)
			pRec->m_nPixelShift = 0;
		else if (pRec->m_ddpf.dwRGBBitCount == 16)
			pRec->m_nPixelShift = 1;
		else if (pRec->m_ddpf.dwRGBBitCount == 32)
			pRec->m_nPixelShift = 2;

		pRec->m_nPixelSize = 1 << pRec->m_nPixelShift;

		pRec->m_nRBits = sub_10021f10(pPF->dwRBitMask);
		pRec->m_nGBits = sub_10021f10(pPF->dwGBitMask);
		pRec->m_nBBits = sub_10021f10(pPF->dwBBitMask);
		pRec->m_nABits = sub_10021f10(pPF->dwRGBAlphaBitMask);

		sub_10037bf0(0xff, pPF->dwRBitMask, &pRec->m_nShift8888[0][0], &pRec->m_nShift8888[0][1]);
		sub_10037bf0(0xff, pPF->dwGBitMask, &pRec->m_nShift8888[1][0], &pRec->m_nShift8888[1][1]);
		sub_10037bf0(0xff, pPF->dwBBitMask, &pRec->m_nShift8888[2][0], &pRec->m_nShift8888[2][1]);
		sub_10037bf0(0xff, pPF->dwRGBAlphaBitMask, &pRec->m_nShift8888[3][0], &pRec->m_nShift8888[3][1]);

		sub_10037bf0(0xf800, pPF->dwRBitMask, &pRec->m_nShift565[0][0], &pRec->m_nShift565[0][1]);
		sub_10037bf0(0x07e0, pPF->dwGBitMask, &pRec->m_nShift565[1][0], &pRec->m_nShift565[1][1]);
		sub_10037bf0(0x001f, pPF->dwBBitMask, &pRec->m_nShift565[2][0], &pRec->m_nShift565[2][1]);

		sub_10037bf0(0x7c00, pPF->dwRBitMask, &pRec->m_nShift555[0][0], &pRec->m_nShift555[0][1]);
		sub_10037bf0(0x03e0, pPF->dwGBitMask, &pRec->m_nShift555[1][0], &pRec->m_nShift555[1][1]);
		sub_10037bf0(0x001f, pPF->dwBBitMask, &pRec->m_nShift555[2][0], &pRec->m_nShift555[2][1]);

		pRec->m_wMask[0] = (uint16)pPF->dwRBitMask;
		pRec->m_wMask[1] = (uint16)pPF->dwGBitMask;
		pRec->m_wMask[2] = (uint16)pPF->dwBBitMask;
		pRec->m_wMask[3] = (uint16)pPF->dwRGBAlphaBitMask;

		pRec->m_b1555 = pPF->dwRBitMask == 0x7c00 && pPF->dwGBitMask == 0x3e0 &&
		                pPF->dwBBitMask == 0x1f && pPF->dwRGBAlphaBitMask == 0x8000;
		pRec->m_b565 = pPF->dwRBitMask == 0xf800 && pPF->dwGBitMask == 0x7e0 &&
		               pPF->dwBBitMask == 0x1f && pPF->dwRGBAlphaBitMask == 0;

		pRec->m_Link.m_pData = pRec;
		pRec->m_Link.m_pPrev = &g_List1008e6a4;
		pRec->m_Link.m_pNext = g_List1008e6a4.m_pNext;
		pRec->m_Link.m_pNext->m_pPrev = &pRec->m_Link;
		pRec->m_Link.m_pPrev->m_pNext = &pRec->m_Link;
	}

	return 1;
}


// ======================================================================= //
//  0x10037f30 -- 1584 bytes.  Pick the renderer's formats from the list
//  0x10037cb0 built: each slot takes the first record matching the first
//  spec of its wish list that anything matches.  Slots 1-4 are required
//  (FALSE with a message); slot 5, the lightmap format, only switches
//  lightmapping off.  Then note which DXT formats the device has.
// ======================================================================= //
static inline D3DFormatRec* FindFormat(FormatSpec* pSpecs, uint32 nSpecs)
{
	FormatSpec*   pSpec;
	D3DLink*      pCur;
	D3DFormatRec* pRec;
	uint32        i;

	for (i = 0; i < nSpecs; i++)
	{
		pSpec = &pSpecs[i];

		for (pCur = g_List1008e6a4.m_pNext; pCur != &g_List1008e6a4;
		     pCur = pCur->m_pNext)
		{
			pRec = (D3DFormatRec*)pCur->m_pData;

			if (pRec->m_nRBits == pSpec->m_nRBits &&
			    pRec->m_nGBits == pSpec->m_nGBits &&
			    pRec->m_nBBits == pSpec->m_nBBits &&
			    (pSpec->m_dwFlagsOn & pRec->m_ddpf.dwFlags) &&
			    !(pSpec->m_dwFlagsOff & pRec->m_ddpf.dwFlags))
			{
				return pRec;
			}
		}
	}

	return LTNULL;
}

LTBOOL sub_10037f30()
{
	FormatSpec specs8888[1] =
	{
		{ 8, 8, 8, 8, DDPF_ALPHAPIXELS, DDPF_LUMINANCE },
	};

	FormatSpec specsFullbrite[2] =
	{
		{ 5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE },
		{ 4, 4, 4, 4, DDPF_ALPHAPIXELS, DDPF_LUMINANCE },
	};
	FormatSpec specs4444[2] =
	{
		{ 4, 4, 4, 4, DDPF_ALPHAPIXELS, DDPF_LUMINANCE },
		{ 5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE },
	};
	FormatSpec specsNormal[3] =
	{
		{ 5, 6, 5, 0, DDPF_RGB,         DDPF_LUMINANCE },
		{ 5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE },
		{ 4, 4, 4, 4, DDPF_ALPHAPIXELS, DDPF_LUMINANCE },
	};
	FormatSpec specsInterface[2] =
	{
		{ 5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE | DDPF_PALETTEINDEXED8 },
		{ 4, 4, 4, 4, DDPF_ALPHAPIXELS, DDPF_LUMINANCE | DDPF_PALETTEINDEXED8 },
	};
	FormatSpec specsLightmap[2] =
	{
		{ 5, 5, 5, 0, DDPF_RGB,         DDPF_LUMINANCE },
		{ 5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE },
	};
	FormatSpec specsBump[1] =
	{
		{ 8, 8, 0, 0, DDPF_BUMPDUDV,    DDPF_LUMINANCE },
	};
	D3DLink*      pCur;
	D3DFormatRec* pRec;


	g_1008e6b0[0] = FindFormat(specs8888, 1);

	g_1008e6b0[1] = FindFormat(specsFullbrite, 2);
	if (!g_1008e6b0[1])
	{
		sub_10021d60(0, "FORMAT_FULLBRITE texture format missing.");
		return LTFALSE;
	}

	g_1008e6b0[2] = FindFormat(specs4444, 2);
	if (!g_1008e6b0[2])
	{
		sub_10021d60(0, "FORMAT_4444 texture format missing.");
		return LTFALSE;
	}

	g_1008e6b0[3] = FindFormat(specsNormal, 3);
	if (!g_1008e6b0[3])
	{
		sub_10021d60(0, "FORMAT_NORMAL texture format missing.");
		return LTFALSE;
	}

	g_1008e6b0[4] = FindFormat(specsInterface, 2);
	if (!g_1008e6b0[4])
	{
		sub_10021d60(0, "FORMAT_INTERFACE texture format missing.");
		return LTFALSE;
	}

	g_1008e6b0[5] = FindFormat(specsLightmap, 2);
	if (!g_1008e6b0[5])
	{
		sub_10021d60(0, "Warning: device not lightmap capable.");
		g_1008d978 = 0;
	}

	g_1008e6b0[6] = FindFormat(specsBump, 1);

	g_1008da60 = g_1008da64 = g_1008da68 = 0;

	for (pCur = g_List1008e6a4.m_pNext; pCur != &g_List1008e6a4;
	     pCur = pCur->m_pNext)
	{
		pRec = (D3DFormatRec*)pCur->m_pData;

		if (pRec->m_ddpf.dwFlags & DDPF_FOURCC)
		{
			if (pRec->m_ddpf.dwFourCC == FOURCC_DXT1)
				g_1008da68 = 1;
			else if (pRec->m_ddpf.dwFourCC == FOURCC_DXT3)
				g_1008da64 = 1;
			else if (pRec->m_ddpf.dwFourCC == FOURCC_DXT5)
				g_1008da60 = 1;
		}
	}

	return LTTRUE;
}
