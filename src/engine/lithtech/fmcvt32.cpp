// Cluster 0x00460000 section D: the 32-bit DXT decompressor, twin of
// partfm.cpp 0x0045f6a0.  Split out of the cluster's file because
// VC6 literal pooling and block merging are per-TU.
#include "ltbasedefs.h"
//  D.  0x00460160 .. 0x00460bef, 2,704 bytes  --  THE DXT DECOMPRESSOR,
//      32-BIT DESTINATION
//
//  EXTENT: SOUND, and it is ONE function.  A sweep of every `e8`/`e9`
//  displacement in .text finds three landing inside 0x00460160..0x00460bef:
//  the entry (`call 0x460160` at 0x0045ed3e) and two of its own jumps
//  (0x004602dc -> 0x004601b3, 0x00460a32 -> 0x00460b97).  One `ret`
//  (0x00460bef), no branch target outside the range, last instruction
//  flush with the end.
//
//  IT IS THE TWIN OF 0x0045f6a0, WHICH IS ALREADY WRITTEN.
//  src/engine/lithtech/partfm.cpp reconstructs 0x0045f6a0 as
//  fm_BlitDXTTo16Q45C and says so in as many words: "0x00460160 (2,704
//  bytes, the next row) is the same function with a 32-bit destination --
//  0x0045ed30 is its forwarder and is entry 3 of the same table group at
//  0x004ace44."  The two forwarders are byte-identical bar the target
//  (0x0045ed10 -> 0x0045f6a0, 0x0045ed30 -> 0x00460160), each pushing the
//  caller's two arguments plus two literal zeros.  This body is that file's
//  source with the destination width changed, and EVERY structural claim in
//  its write-up was re-verified against these bytes rather than assumed.
//
//  WHAT CHANGES WITH THE WIDTH, all read off 0x00460160
//    * 0x00460590 / 0x004605a8 / 0x004605c0 / 0x004605d8 call 0x0045d9c0 --
//      FMFormatConvert::Convert32 -- where the 16-bit twin calls 0x0045d760.
//      So aColors and aAlpha are PValue (uint32), and 0x00460601
//      `mov 0x34(%esp,%esi,4)` / 0x0046084a `mov 0x44(%esp,%edx,4)` confirm
//      the four-byte stride in both arrays.
//    * the pixel stores are full dwords (`mov %esi,(%eax)` /
//      `mov %esi,0x4(%eax)` / +0x8 / +0xc at 0x00460605..0x00460631), and
//      the per-block destination advance is 0x10 (0x00460ba7 `add $0x10`),
//      i.e. four pixels of four bytes -- `xb * 16`, against the twin's
//      `xb * 8`.
//    * the six interpolation divides are UNSIGNED here: 0x0046074d uses
//      0x24924925 with `mul` + `sub` + `shr $1` + `add` + `shr $2` (the
//      unsigned /7 form) and 0x004607ec uses 0xcccccccd + `shr $2` (/5),
//      where the twin has the signed 0x92492493 / 0x66666667 pair.  The
//      reason is visible at 0x00460734: the two DXT5 endpoints are
//      zero-extended into full REGISTERS (`xor %edx,%edx ; mov 0x1(%eax),
//      %dl`) and never homed into the alpha array first, so they are
//      uint32 here where the twin's were uint16 promoted to int.
//
//  FRAME CHECKSUM, 0xfc.  Every slot is accounted for:
//     0x010 the destination x cursor (VC6's strength reduction of xb*16)
//     0x014 pABlock            0x018 bHasAlphaBlock   0x01c xb
//     0x020 nRowBase           0x024 nAlphaShift      0x028 nBlockShift
//     0x02c yb                 0x030 nAlpha8
//     0x034 aColors[4]         0x044 aAlpha[16]
//     0x084 c0   0x088 c1   0x08c c2   0x090 c3
//     0x094 nRowBase<<shift    0x098 nBlocksX         0x09c nColor1
//     0x0a0 nColorOffset       0x0a4 nAlphaOr         0x0a8 nColor0
//     0x0ac bInterpAlpha
//     0x0b0 nA0 (dead) 0x0b4 nR0 0x0b8 nG0 0x0bc nB0
//     0x0c0 nA1 (dead) 0x0c4 nR1 0x0c8 nG1 0x0cc nB1
//     0x0d0 pAlphaToTable      0x0d4 convert (0x14)   0x0e8 nBlocksY
//     0x0ec xb<<shift          0x0f0 nAlphaMaskInv    0x0f4 nAlpha8<<8
//     0x0f8 read (0x14)
//  0x10 .. 0x10c is 0xfc exactly.  The two dead alpha out-parameters at
//  0x0b0 and 0x0c0 are what pin ltpvalue.h's PValue_Get rather than three
//  hand-written shifts.
//
//  WHAT I RE-VERIFIED ON THESE BYTES (not carried over on trust)
//    * 0x00460168/0x00460193: argument ONE is the manager and argument TWO
//      the request, the same order as the twin.
//    * 0x0046019a `cmp $0x5` sets only bHasAlphaBlock; 0x0046029d `cmp $0x6`
//      sets bInterpAlpha as well and clears nAlphaOr/nAlpha8 -- so code 5 is
//      the EXPLICIT four-bit alpha (DXT3) and code 6 the interpolated one
//      (DXT5), and anything else is the eight-byte block (DXT1).
//      0x00460267/0x004602e1 turn that into `nBlockShift = 4, nColorOffset
//      = 8` versus `3, 0`.
//    * 0x004601b3..0x00460257 is the two Inits expanded, the reader on
//      `&pMgr->m_Format565` (`lea 0x38(%eax)`) and the writer on
//      `pRequest->m_pDestFormat`, with `~m_Masks[CP_ALPHA]` taken between
//      them (0x004601c3 `mov 0x8(%edx),%edi`, 0x004601d0 `not %edi`).
//    * 0x0046042d `cmp 0x9c(%esp),%ax` with %ax = nColor0 and `jbe` to the
//      three-colour arm: the guard is `nColor0 > nColor1`, on the RAW 565
//      words and not on the decoded PValues.
//    * 0x004606f5 skips the whole alpha half when there is no alpha block,
//      and 0x0046072a splits interpolated from explicit; 0x004606fb..
//      0x00460726 rebuilds BOTH pointers from scratch instead of rewinding.
//    * 0x004609ab: DXT5 index ten straddles the dword -- `(nLo >> 30) |
//      ((nHi & 1) << 2)`.
//    * 0x00460be6 `xor %eax,%eax` -> LTRESULT LT_OK.
//
//  HONEST CONFIDENCE.  Lower than A/B/C.  The control flow, every field
//  offset, every constant and the two SDK helpers are read off these bytes
//  and cited, but three things are free choices that the bytes do not
//  decide: (a) whether the thirty-two pixel statements come from a macro,
//  (b) the operand order inside each `lea`-folded sum, (c) in the /5 arm,
//  whether `aAlpha[6]`/`aAlpha[7]` are written before or after the four
//  interpolants -- shipped SCHEDULES them between [2] and [3], and they are
//  written first here to stay in step with partfm.cpp's twin.  Expect a
//  near rather than a first-shot match; if the twin at 0x0045f6a0 comes
//  back exact, every difference this one still has is a width question and
//  nothing else.
//
//  WHERE IT BELONGS.  The same TU as 0x0045f6a0, i.e. spliced into
//  src/engine/lithtech/fmconvert.cpp beside partfm.cpp's section C.  The
//  two Init bodies below are DEFINED so that a stand-alone build of this
//  scratch file reproduces the inline expansion the shipped code has; on
//  splicing they are dropped and fmconvert.cpp's own definitions serve.
//  Convert32 and Read16 are deliberately DECLARED ONLY -- the shipped code
//  CALLS both (0x0045d9c0 four times plus the sixteen-entry ramp loop,
//  0x0045e0d0 twice), so defining them here would inline them and wreck it.
//
// ======================================================================= //

#include "ltpvalue.h"          // PValue, CP_ALPHA..CP_BLUE, PValue_Get/Set
#include "ltcodes.h"           // LT_OK

class FMFormatQ460
{
public:
	uint32 m_pVTable;                        // 0x00
	uint32 m_nCode;                          // 0x04
	uint32 m_Masks[NUM_COLORPLANES];         // 0x08
	uint32 m_nBits[NUM_COLORPLANES];         // 0x18
	uint32 m_nShifts[NUM_COLORPLANES];       // 0x28
};                                           // 0x38

class FMConvertMgrQ460
{
public:
	FMFormatQ460 m_FormatPValue;             // 0x000
	FMFormatQ460 m_Format565;                // 0x038
	uint8        m_ToTables[9][256];         // 0x070
	uint8*       m_pFromTables[9];           // 0x970
	uint8        m_TableData[511];           // 0x994
};                                           // 0xb94

class FMConvertRequestQ460
{
public:
	FMFormatQ460* m_pSrcFormat;              // 0x00
	uint8*        m_pSrcData;                // 0x04
	int32         m_nSrcPitch;               // 0x08
	uint8*        m_pSrcPalette;             // 0x0c
	FMFormatQ460* m_pDestFormat;             // 0x10
	uint8*        m_pDestData;               // 0x14
	int32         m_nDestPitch;              // 0x18
	uint32        m_nWidth;                  // 0x1c
	uint32        m_nHeight;                 // 0x20
	uint32        m_Flags;                   // 0x24
};

class FMFormatConvert
{
public:
	// fmconvert.cpp's 0x0045d530, repeated so the stand-alone build gets the
	// inline expansion at 0x0046020b..0x00460257.  DROP ON SPLICE.
	void Init(FMConvertMgrQ460* pMgr, FMFormatQ460* pFormat)
	{
		m_pFormat = pFormat;

		m_pTables[CP_ALPHA] = pMgr->m_ToTables[pFormat->m_nBits[CP_ALPHA]];
		m_pTables[CP_RED]   = pMgr->m_ToTables[pFormat->m_nBits[CP_RED]];
		m_pTables[CP_GREEN] = pMgr->m_ToTables[pFormat->m_nBits[CP_GREEN]];
		m_pTables[CP_BLUE]  = pMgr->m_ToTables[pFormat->m_nBits[CP_BLUE]];
	}

	PValue Convert32(PValue* pValue);                            // 0x0045d9c0

public:
	uint8*        m_pTables[NUM_COLORPLANES];   // 0x00
	FMFormatQ460* m_pFormat;                    // 0x10
};

class FMFormatRead
{
public:
	// fmconvert.cpp's 0x0045dbb0, same reason.  DROP ON SPLICE.
	void Init(FMConvertMgrQ460* pMgr, FMFormatQ460* pFormat)
	{
		m_pTables[CP_ALPHA] = pMgr->m_pFromTables[pFormat->m_nBits[CP_ALPHA]];
		m_pTables[CP_RED]   = pMgr->m_pFromTables[pFormat->m_nBits[CP_RED]];
		m_pTables[CP_GREEN] = pMgr->m_pFromTables[pFormat->m_nBits[CP_GREEN]];
		m_pTables[CP_BLUE]  = pMgr->m_pFromTables[pFormat->m_nBits[CP_BLUE]];

		m_pFormat = pFormat;
	}

	PValue Read16(uint16* pSrc);                                 // 0x0045e0d0

public:
	FMFormatQ460* m_pFormat;                    // 0x00
	uint8*        m_pTables[NUM_COLORPLANES];   // 0x04
};

// The sixteen-step alpha ramp 0x0045d130 lays down at 0x004b40e8.
// partfm.cpp DEFINES it (g_AlphaPValuesQ45C); declared here, under that
// definition's name, so this file emits no second copy.
extern PValue g_AlphaPValuesQ45C[16];                            // 0x004b40e8

// The two source format codes that carry an alpha block, numeric in the
// shipped code (0x0046019a, 0x0046029d).
#define FMCODE_DXT3Q460   5
#define FMCODE_DXT5Q460   6


LTRESULT fm_BlitDXTTo32Q460(FMConvertMgrQ460* pMgr, FMConvertRequestQ460* pRequest,
                            uint32 nUnused1, uint32 nUnused2)
{
	FMFormatRead        read;
	FMFormatConvert     convert;
	PValue              aColors[4];
	PValue              aAlpha[16];
	// Arrays, all three: the endpoint PValues sit contiguous at 0x84..0x90
	// and each component group keeps its dead alpha slot (0xb0 / 0xc0).
	// Scalars give a 0xf0 frame; these give the shipped 0xfc.
	PValue              aEndpoints[4];
	uint16              nColor0, nColor1;
	uint32              aC0[4];
	uint32              aC1[4];
	uint32              nBlocksX, nBlocksY, xb, yb;
	uint32              nBlockShift, nColorOffset;
	uint32              nAlphaOr, nAlpha8, nAlphaShift;
	uint32              bHasAlphaBlock, bInterpAlpha;
	uint32              nAlphaMaskInv;
	uint8*              pAlphaToTable;
	uint32              nCode;

	// 0x00460176..0x0046019d.  The defaults first, then the two format codes
	// overwrite them; 0x0046029d's compare is on the fall-through of
	// 0x0046019a's, which is an `else if`.
	nAlphaOr       = PVALUE_ALPHAMASK;
	nAlpha8        = 0xff;
	bInterpAlpha   = 0;
	bHasAlphaBlock = 0;

	// 0x00460190 loads the code ONCE into %ecx and both compares read that
	// register, across a basic-block boundary -- a local, not two field
	// reads.
	nCode = pRequest->m_pSrcFormat->m_nCode;

	if (nCode == FMCODE_DXT3Q460)
	{
		bHasAlphaBlock = 1;
	}
	else if (nCode == FMCODE_DXT5Q460)
	{
		bInterpAlpha   = 1;
		bHasAlphaBlock = 1;
		nAlphaOr       = 0;
		nAlpha8        = 0;

		nAlphaShift   = pRequest->m_pDestFormat->m_nShifts[CP_ALPHA];
		pAlphaToTable = pMgr->m_ToTables[pRequest->m_pDestFormat->m_nBits[CP_ALPHA]];
	}

	// 0x004601b3..0x00460257 -- both Inits expanded, the reader on the
	// manager's own 565 format (DXT stores its endpoints as R5G6B5) and the
	// writer on the request's destination format.  The mask first: it is
	// loaded and inverted inside the reader's Init (0x004601c3/0x004601d0).
	nAlphaMaskInv = ~pRequest->m_pDestFormat->m_Masks[CP_ALPHA];
	read.Init(pMgr, &pMgr->m_Format565);
	convert.Init(pMgr, pRequest->m_pDestFormat);

	if (bHasAlphaBlock)
	{
		uint32 i;

		nBlockShift  = 4;
		nColorOffset = 8;

		for (i = 0; i < 16; i++)
			aAlpha[i] = convert.Convert32(&g_AlphaPValuesQ45C[i]);
	}
	else
	{
		nBlockShift  = 3;
		nColorOffset = 0;
	}

	nBlocksX = pRequest->m_nWidth  >> 2;
	nBlocksY = pRequest->m_nHeight >> 2;

	for (yb = 0; yb < nBlocksY; yb++)
	{
		for (xb = 0; xb < nBlocksX; xb++)
		{
			uint8*  pBlock;
			PValue* pDest;
			uint32  nBits;

			// 0x00460355 shifts xb and 0x00460330 shifts nRowBase, and the
			// two shifted values are added SEPARATELY (0x0046035e /
			// 0x00460369) -- two shifts in the source, not one shift of a
			// sum.  The xb term is kept at 0xec(%esp) for the alpha half.
			// The row base is `yb * nBlocksX`, which VC6 strength-reduces
			// into the accumulator at 0x20(%esp), zeroed AFTER the height
			// test (0x00460314); an explicit `nRowBase` zeroes it before.
			pBlock = pRequest->m_pSrcData + (xb << nBlockShift)
			         + ((yb * nBlocksX) << nBlockShift) + nColorOffset;

			nColor0 = ((uint16*)pBlock)[0];
			nColor1 = ((uint16*)pBlock)[1];

			aEndpoints[0] = read.Read16(&nColor0);
			aEndpoints[1] = read.Read16(&nColor1);

			PValue_Get(aEndpoints[0], aC0[0], aC0[1], aC0[2], aC0[3]);
			PValue_Get(aEndpoints[1], aC1[0], aC1[1], aC1[2], aC1[3]);

			aEndpoints[0] |= nAlphaOr;
			aEndpoints[1] |= nAlphaOr;

			if (nColor0 > nColor1)
			{
				aEndpoints[2] = PValue_Set(nAlpha8, (aC0[1] * 2 + aC1[1]) / 3,
				                         (aC0[2] * 2 + aC1[2]) / 3,
				                         (aC0[3] * 2 + aC1[3]) / 3);
				aEndpoints[3] = PValue_Set(nAlpha8, (aC0[1] + aC1[1] * 2) / 3,
				                         (aC0[2] + aC1[2] * 2) / 3,
				                         (aC0[3] + aC1[3] * 2) / 3);
			}
			else
			{
				// 0x00460543 stores the zero first.
				aEndpoints[3] = 0;
				aEndpoints[2] = PValue_Set(nAlpha8, (aC0[1] + aC1[1]) / 2,
				                         (aC0[2] + aC1[2]) / 2,
				                         (aC0[3] + aC1[3]) / 2);
			}

			aColors[0] = convert.Convert32(&aEndpoints[0]);
			aColors[1] = convert.Convert32(&aEndpoints[1]);
			aColors[2] = convert.Convert32(&aEndpoints[2]);
			aColors[3] = convert.Convert32(&aEndpoints[3]);

			// 0x004605e8 reads the index dword and 0x004605e5..0x004605fa
			// only then builds the destination pointer, so both come AFTER
			// the four Convert32 calls in the source.
			nBits = *(uint32*)(pBlock + 4);

			// `yb << 2` keeps the shift-then-imul (0x004605eb/0x004605ee)
			// and leaves xb * 16 to be strength-reduced into the cursor at
			// 0x10(%esp); `yb * 4` lets VC6 factor both into one `lea`.
			pDest = (PValue*)(pRequest->m_pDestData
			                  + (yb << 2) * pRequest->m_nDestPitch + xb * 16);

			pDest[0] = aColors[ nBits        & 3];
			pDest[1] = aColors[(nBits >>  2) & 3];
			pDest[2] = aColors[(nBits >>  4) & 3];
			pDest[3] = aColors[(nBits >>  6) & 3];
			pDest = (PValue*)((uint8*)pDest + pRequest->m_nDestPitch);

			pDest[0] = aColors[(nBits >>  8) & 3];
			pDest[1] = aColors[(nBits >> 10) & 3];
			pDest[2] = aColors[(nBits >> 12) & 3];
			pDest[3] = aColors[(nBits >> 14) & 3];
			pDest = (PValue*)((uint8*)pDest + pRequest->m_nDestPitch);

			pDest[0] = aColors[(nBits >> 16) & 3];
			pDest[1] = aColors[(nBits >> 18) & 3];
			pDest[2] = aColors[(nBits >> 20) & 3];
			pDest[3] = aColors[(nBits >> 22) & 3];
			pDest = (PValue*)((uint8*)pDest + pRequest->m_nDestPitch);

			pDest[0] = aColors[(nBits >> 24) & 3];
			pDest[1] = aColors[(nBits >> 26) & 3];
			pDest[2] = aColors[(nBits >> 28) & 3];
			pDest[3] = aColors[(nBits >> 30) & 3];

			// 0x004606fb..0x00460726 -- the alpha half re-derives BOTH
			// pointers from scratch rather than rewinding the ones above.
			if (bHasAlphaBlock)
			{
				uint8*  pABlock;
				PValue* pADest;

				pADest = (PValue*)(pRequest->m_pDestData
				                   + (yb << 2) * pRequest->m_nDestPitch + xb * 16);

				pABlock = pRequest->m_pSrcData + (xb << nBlockShift)
				          + ((yb * nBlocksX) << nBlockShift);

				if (bInterpAlpha)
				{
					// 0x00460734 -- both endpoints are zero-extended BYTES
					// into full registers (`xor %edx,%edx ; mov 0x1(%eax),
					// %dl`), and nothing homes them into aAlpha first.  That
					// is why every divide below is UNSIGNED here where the
					// 16-bit twin's are signed.
					uint32 nA = pABlock[0];
					uint32 nB = pABlock[1];
					uint32 nLo, nHi;

					if (nA > nB)
					{
						aAlpha[2] = (nA * 6 + nB    ) / 7;
						aAlpha[3] = (nA * 5 + nB * 2) / 7;
						aAlpha[4] = (nA * 4 + nB * 3) / 7;
						aAlpha[5] = (nA * 3 + nB * 4) / 7;
						aAlpha[6] = (nA * 2 + nB * 5) / 7;
						aAlpha[7] = (nA     + nB * 6) / 7;
					}
					else
					{
						aAlpha[6] = 0;
						aAlpha[7] = 255;

						aAlpha[2] = (nA * 4 + nB    ) / 5;
						aAlpha[3] = (nA * 3 + nB * 2) / 5;
						aAlpha[4] = (nA * 2 + nB * 3) / 5;
						aAlpha[5] = (nA     + nB * 4) / 5;
					}

					// 0x00460843..0x004608e0 -- each of the eight in place;
					// [0] and [1] index the table with the two endpoints
					// still in registers, [2]..[7] read their own slots back.
					aAlpha[0] = pAlphaToTable[nA]        << nAlphaShift;
					aAlpha[1] = pAlphaToTable[nB]        << nAlphaShift;
					aAlpha[2] = pAlphaToTable[aAlpha[2]] << nAlphaShift;
					aAlpha[3] = pAlphaToTable[aAlpha[3]] << nAlphaShift;
					aAlpha[4] = pAlphaToTable[aAlpha[4]] << nAlphaShift;
					aAlpha[5] = pAlphaToTable[aAlpha[5]] << nAlphaShift;
					aAlpha[6] = pAlphaToTable[aAlpha[6]] << nAlphaShift;
					aAlpha[7] = pAlphaToTable[aAlpha[7]] << nAlphaShift;

					nHi = *(uint16*)(pABlock + 6);
					nLo = *(uint32*)(pABlock + 2);

					pADest[0] |= aAlpha[ nLo        & 7];
					pADest[1] |= aAlpha[(nLo >>  3) & 7];
					pADest[2] |= aAlpha[(nLo >>  6) & 7];
					pADest[3] |= aAlpha[(nLo >>  9) & 7];
					pADest = (PValue*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] |= aAlpha[(nLo >> 12) & 7];
					pADest[1] |= aAlpha[(nLo >> 15) & 7];
					pADest[2] |= aAlpha[(nLo >> 18) & 7];
					pADest[3] |= aAlpha[(nLo >> 21) & 7];
					pADest = (PValue*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] |= aAlpha[(nLo >> 24) & 7];
					pADest[1] |= aAlpha[(nLo >> 27) & 7];
					// 0x004609ab -- index ten straddles the dword.
					pADest[2] |= aAlpha[(nLo >> 30) | ((nHi & 1) << 2)];
					pADest[3] |= aAlpha[(nHi >>  1) & 7];
					pADest = (PValue*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] |= aAlpha[(nHi >>  4) & 7];
					pADest[1] |= aAlpha[(nHi >>  7) & 7];
					pADest[2] |= aAlpha[(nHi >> 10) & 7];
					pADest[3] |= aAlpha[(nHi >> 13) & 7];
				}
				else
				{
					// 0x00460a37 -- DXT3.  The colour pass already put an
					// opaque alpha in, so each pixel is masked before the
					// nibble goes in and stored with `mov`, not `or`.
					uint32 nLo = *(uint32*)pABlock;
					uint32 nHi;

					pADest[0] = (pADest[0] & nAlphaMaskInv) | aAlpha[ nLo        & 0xf];
					pADest[1] = (pADest[1] & nAlphaMaskInv) | aAlpha[(nLo >>  4) & 0xf];
					pADest[2] = (pADest[2] & nAlphaMaskInv) | aAlpha[(nLo >>  8) & 0xf];
					pADest[3] = (pADest[3] & nAlphaMaskInv) | aAlpha[(nLo >> 12) & 0xf];
					pADest = (PValue*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] = (pADest[0] & nAlphaMaskInv) | aAlpha[(nLo >> 16) & 0xf];
					pADest[1] = (pADest[1] & nAlphaMaskInv) | aAlpha[(nLo >> 20) & 0xf];
					pADest[2] = (pADest[2] & nAlphaMaskInv) | aAlpha[(nLo >> 24) & 0xf];
					pADest[3] = (pADest[3] & nAlphaMaskInv) | aAlpha[(nLo >> 28) & 0xf];
					pADest = (PValue*)((uint8*)pADest + pRequest->m_nDestPitch);

					nHi = *(uint32*)(pABlock + 4);

					pADest[0] = (pADest[0] & nAlphaMaskInv) | aAlpha[ nHi        & 0xf];
					pADest[1] = (pADest[1] & nAlphaMaskInv) | aAlpha[(nHi >>  4) & 0xf];
					pADest[2] = (pADest[2] & nAlphaMaskInv) | aAlpha[(nHi >>  8) & 0xf];
					pADest[3] = (pADest[3] & nAlphaMaskInv) | aAlpha[(nHi >> 12) & 0xf];
					pADest = (PValue*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] = (pADest[0] & nAlphaMaskInv) | aAlpha[(nHi >> 16) & 0xf];
					pADest[1] = (pADest[1] & nAlphaMaskInv) | aAlpha[(nHi >> 20) & 0xf];
					pADest[2] = (pADest[2] & nAlphaMaskInv) | aAlpha[(nHi >> 24) & 0xf];
					pADest[3] = (pADest[3] & nAlphaMaskInv) | aAlpha[(nHi >> 28) & 0xf];
				}
			}
		}

		// 0x00460bd1 `add %eax,%esi` -- the row's block base is ACCUMULATED
		// (the strength-reduced `yb * nBlocksX`).
	}

	return LT_OK;
}


// 0x0045ed30, 23 bytes -- the blitter-table entry for the 32-bit DXT blit
// (entry 3 of the group at 0x004ace44; name ours).  The body above takes
// four arguments; the table's blitter signature is (pMgr, pRequest), so
// this forwards the two it is given and two literal zeros, `add $0x10`
// after the call (0x0045ed38..0x0045ed43).  Byte-for-byte the 16-bit
// forwarder at 0x0045ed10 bar the call target.
LTRESULT fm_BlitDXTTo32EntryQ460(FMConvertMgrQ460* pMgr, FMConvertRequestQ460* pRequest)
{
	return fm_BlitDXTTo32Q460(pMgr, pRequest, 0, 0);
}


// ======================================================================= //
//  END OF FILE
// ======================================================================= //
