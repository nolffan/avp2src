// *********************************************************************** //
//
//  MODULE:   CLUSTER45D530 (.CPP)  -- scratch file, NOT a drop-in TU
//
//  PURPOSE:  lithtech.exe 0x0045d530 .. 0x0045f080, 27 adjacent functions,
//            reconstructed as ONE translation unit.  26 are written below;
//            0x0045d990 is deliberately absent -- it is already byte-
//            identical from src/engine/lithtech/lt2new3.cpp (section C,
//            `CPaletteSource3::ReadPixel`) and must not be written twice.
//
//  WHAT THIS MODULE IS.  It is the engine's PIXEL FORMAT CONVERTER -- the
//  back end of the `FMConvertMgr` that src/engine/lithtech/lttexmod.cpp
//  already named and sized (0xb94 bytes, constructor 0x0045f390, dispatcher
//  0x0045f3d0).  0x0045f3d0 indexes a table of BLITTERS at 0x004acdb8 by
//  `srcFormat->code * 8 + destFormat->code` and calls
//  `blit(this, pRequest)`; the thirteen blitters in this range are that
//  table's entries.  Five more entries here are the SINGLE-PIXEL API that
//  0x0045f5e0 and 0x0045f620 dispatch through the tables at 0x004aceb8 and
//  0x004aced8.  The rest are the two tiny helper classes every blitter
//  builds on its own stack.
//
//  GROUND TRUTH: the retail lithtech.exe only, cl 8447 /O2.  Every offset,
//  constant and branch below is cited by the address it was read at.
//
//  THE ONE BIG COIN-FLIP, stated once at the top because it decides six
//  functions at a stroke: the destination-writer's four-term OR is emitted
//  GREEN, RED, ALPHA, BLUE in every one of its six instances, while every
//  SOURCE-reader in the same module emits its four terms in plane order
//  ALPHA, RED, GREEN, BLUE.  The readers prove the source order survives
//  when the four operands cost the same; the writers' operands do NOT cost
//  the same (green is free in `%dh`, red needs a spill-and-reload of byte 2,
//  alpha is a `shr $0x18`, blue an `and $0xff`), so the reordering is read
//  here as VC6's own, and the writers are written in plane order like the
//  readers.  If the six writer bodies come back as instruction-identical
//  nears with the OR chain rotated, the fix is to reorder the four terms of
//  FMFormatConvert::Convert8/16/32 to green, red, alpha, blue -- one edit,
//  six functions.
//
// *********************************************************************** //

#include "ltbasedefs.h"         // uint8 / uint16 / uint32 / int32 / LTRESULT
#include "ltpvalue.h"           // PValue, CP_ALPHA..CP_BLUE, NUM_COLORPLANES


// ======================================================================= //
//
//  THE CLASSES THIS FILE DOES NOT OWN
//
// ======================================================================= //

// A pixel format.  lttexmod.cpp already established the first 0x18 bytes
// (vptr, bit-depth code, four masks in CP_ALPHA..CP_BLUE order) from
// PFormat::Init at 0x0045f0e0 and its 565 call at 0x0045f3a8.  The rest --
// the 0x20 bytes of "derived data" lttexmod.cpp could only pad -- is pinned
// by the per-plane helper at 0x0045f120, which for plane i stores
//
//     0x0045f14b   mov %ecx,0x18(%esi,%edi,4)      <- bit COUNT of the plane
//     0x0045f14f   mov %eax,0x28(%esi,%edi,4)      <- low bit INDEX (shift)
//
// having got both out of 0x0045f160, which counts the mask's trailing zeros
// into one out-parameter and trailing-zeros-plus-width into the other.
//
// NAMED FMFormat, not PFormat, purely so this scratch TU cannot collide with
// lttexmod.cpp's declaration of the same class at link time.  The vptr is
// spelled as a plain dword for the same reason: nothing here ever calls the
// virtual, and a declared-but-undefined virtual would make this TU emit a
// vtable referencing 0x0045f0e0.
class FMFormat
{
public:
	// 0x0045f1e0.  __thiscall, one argument, `ret $0x4`; compares +0x04 and
	// the four masks and returns 1/0.  Declared, not defined -- it lives with
	// PFormat::Init past the end of this cluster.  Declaring it as a member
	// is the only spelling that reproduces the `mov ...,%ecx; push arg; call`
	// the seven guarding blitters emit.
	int IsSame(FMFormat* pOther);            // 0x0045f1e0

	// 0x0045f1d0 (defined in section G below): log2 bytes/pixel.
	uint32 GetBPPShift();                    // 0x0045f1d0

public:
	uint32 m_pVTable;                        // 0x00  -> 0x004a1ab4
	uint32 m_nCode;                          // 0x04  1=8bit 2=565 3=32bit ...
	uint32 m_Masks[NUM_COLORPLANES];         // 0x08  A, R, G, B
	uint32 m_nBits[NUM_COLORPLANES];         // 0x18  bits per plane
	uint32 m_nShifts[NUM_COLORPLANES];       // 0x28  low bit index per plane

	// 0x0045f230: bytes per pixel for the format code (defined at the end).
	uint32 GetBytesPerPixel();               // 0x0045f230
};                                           // 0x38


// The converter itself.  Layout read off FMConvertMgr::InitTables,
// 0x0045f4f0, which is the only thing that writes 0x70..0xb94:
//
//   0x0045f4fc..0x0045f561  nine pointers at +0x970 +0x974 ... +0x990, set to
//                           +0x994, +0x995, +0x997, +0x99b, +0x9a3, +0x9b3,
//                           +0x9d3, +0xa13, +0xa93 -- strides 1,2,4,...,128,
//                           so entry n is a 2^n-entry table and the last one
//                           ends at +0xb93.  That IS the class size lttexmod
//                           computed arithmetically (0xb94).
//   0x0045f569              `add $0x70,%eax`, then a 256-byte table per n,
//                           +0x100 each pass (0x0045f5c6) for nine passes
//                           (0x0045f5cc) -- 0x70 + 9*0x100 = 0x970, meeting
//                           the pointer array exactly.
//
// and the two tables are inverses: m_ToTables[n][i] = (i * ((1<<n)-1)) / 255
// (the magic-divide at 0x0045f5a4) turns an eight-bit channel into an n-bit
// one, m_pFromTables[n][i] = (i * 255) / ((1<<n)-1) (0x0045f58d) turns it
// back.  The two PFormats at +0x00 and +0x38 are the constructor's
// (0x0045f397/0x0045f39d install 0x004a1ab4 in both; 0x0045f1b0 gives the
// first 32-bit ARGB and 0x0045f3ba gives the second 565).
//
// m_FormatPValue sitting at offset ZERO is what lets 0x0045d7e2 and its six
// siblings `push %esi` -- the bare manager pointer -- where 0x0045f1e0 wants
// an FMFormat*.
class FMConvertMgr
{
public:
	FMFormat m_FormatPValue;                 // 0x000  32-bit A8R8G8B8
	FMFormat m_Format565;                    // 0x038  R5G6B5
	uint8    m_ToTables[9][256];             // 0x070
	uint8*   m_pFromTables[9];               // 0x970
	uint8    m_TableData[511];               // 0x994
};                                           // 0xb94


// One conversion request.  lttexmod.cpp read +0x00/+0x04/+0x10/+0x14 and the
// three -1 pitches off the constructor at 0x0045f240; the blitters here pin
// the remaining four fields and correct two of lttexmod's labels:
//
//   +0x0c  the SOURCE PALETTE.  0x0045d5f2 loads it and 0x0045d63e indexes
//          it by a source byte with a four-byte stride.  (lttexmod called it
//          m_Unknown0c; the validity check at 0x0045f31e/0x0045f32a demands
//          it be non-null exactly when the source format's code is 0 or 7.)
//   +0x1c  WIDTH   -- the INNER loop count (0x0045d621 -> 0x0045d674).
//   +0x20  HEIGHT  -- the OUTER loop count, and the row the pitches advance
//          (0x0045d726..0x0045d745).  lttexmod guessed +0x24 for this; the
//          blitters say +0x20, and so does the constructor, which seeds
//          +0x1c and +0x20 with -1 and +0x24 with 0.
//   +0x24  FLAGS.  Read as a BYTE and tested for bit 0 at 0x0045d621/
//          0x0045d627: set means "OR into the destination", clear means
//          "overwrite it".
class FMConvertRequest
{
public:
	FMFormat* m_pSrcFormat;                  // 0x00
	uint8*    m_pSrcData;                    // 0x04
	int32     m_nSrcPitch;                   // 0x08
	uint8*    m_pSrcPalette;                 // 0x0c
	FMFormat* m_pDestFormat;                 // 0x10
	uint8*    m_pDestData;                   // 0x14
	int32     m_nDestPitch;                  // 0x18
	uint32    m_nWidth;                      // 0x1c
	uint32    m_nHeight;                     // 0x20
	uint32    m_Flags;                       // 0x24
};

// The bit of m_Flags the blitters test.  Numeric in the shipped code.
#define FMBLIT_OR   1


// ----------------------------------------------------------------------- //
//  Neighbours this file calls but does not own.
// ----------------------------------------------------------------------- //

// 0x0045d1e0.  __cdecl, (FMConvertMgr*, FMConvertRequest*), the straight
// row-copy the three same-format blitters tail into.  Below this cluster.
LTRESULT fm_BlitSame(FMConvertMgr* pMgr,
                                        FMConvertRequest* pRequest);


// ======================================================================= //
//
//  THE TWO HELPER CLASSES
//
//  Both are 0x14-byte stack locals every blitter builds.  Neither has a
//  vtable (no vptr store anywhere) and neither has a constructor -- the
//  blitters call Init explicitly, and Init does not return `this`
//  (0x0045d530 leaves a shifted bit-count in %eax, 0x0045dbb0 leaves the
//  format), which rules a constructor out.
//
// ======================================================================= //

// PValue -> destination format.
class FMFormatConvert
{
public:
	void   Init(FMConvertMgr* pMgr, FMFormat* pFormat);     // 0x0045d530
	uint8  Convert8 (PValue* pValue);                       // 0x0045d580
	uint16 Convert16(PValue* pValue);                       // 0x0045d760
	PValue Convert32(PValue* pValue);                       // 0x0045d9c0

	// Expanded bodies of the four above; see "INLINE HERE, CALL THERE" in F.
	void   InitInline(FMConvertMgr* pMgr, FMFormat* pFormat);
	uint8  Convert8Inline (PValue* pValue);
	uint16 Convert16Inline(PValue* pValue);
	PValue Convert32Inline(PValue* pValue);

public:
	uint8*    m_pTables[NUM_COLORPLANES];    // 0x00
	FMFormat* m_pFormat;                     // 0x10
};

// Source format -> PValue.
class FMFormatRead
{
public:
	void   Init(FMConvertMgr* pMgr, FMFormat* pFormat);     // 0x0045dbb0
	PValue Read8 (uint8*  pSrc);                            // 0x0045dc00
	PValue Read16(uint16* pSrc);                            // 0x0045e0d0
	PValue Read32(uint32* pSrc);                            // 0x0045e490

	// Expanded bodies of the four above; see "INLINE HERE, CALL THERE" in F.
	void   InitInline(FMConvertMgr* pMgr, FMFormat* pFormat);
	PValue Read8Inline (uint8*  pSrc);
	PValue Read16Inline(uint16* pSrc);
	PValue Read32Inline(uint32* pSrc);

public:
	FMFormat* m_pFormat;                     // 0x00
	uint8*    m_pTables[NUM_COLORPLANES];    // 0x04
};

// One palette entry.  lt2new3.cpp read +1/+2/+3 as r/g/b off 0x0045d990;
// 0x0045eb60 reads +0 as well and packs all four the same way, which fixes
// +0 as the alpha byte and the entry as a straight A,R,G,B quad.
struct FMPaletteEntry
{
	uint8 m_a;                               // 0
	uint8 m_r;                               // 1
	uint8 m_g;                               // 2
	uint8 m_b;                               // 3
};

// Palette -> PValue.  One member.  ReadRGB IS 0x0045d990, which is already
// byte-identical in src/engine/lithtech/lt2new3.cpp; it is repeated in this
// file only because two blitters need VC6 to be able to inline it.  See the
// note over its body.
class FMPaletteRead
{
public:
	PValue ReadRGB (uint8* pSrc);                           // 0x0045d990
	PValue ReadARGB(uint8* pSrc);                           // 0x0045eb60

	// Expanded bodies of the two above (see ReadRGB's note in section F).
	PValue ReadRGBInline (uint8* pSrc);
	PValue ReadARGBInline(uint8* pSrc);

public:
	FMPaletteEntry* m_pPalette;              // 0x00
};


// ======================================================================= //
//
//  A.  THE TWO Init BODIES
//
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x0045d530 (67 bytes)  FMFormatConvert::Init
//
//  WHAT PINS IT
//    * `ret $0x8` with %ecx read and never written -> __thiscall, two stack
//      arguments.  %eax holds a shifted bit count at the `ret`, NOT `this`,
//      so this is a void member and not a constructor.
//    * 0x0045d539 `mov %eax,0x10(%ecx)` stores argument two whole -> the
//      format pointer is a member at +0x10, and it is stored FIRST.
//    * 0x0045d53c/0x0045d548/0x0045d555/0x0045d562 read +0x18, +0x1c, +0x20,
//      +0x24 of argument two -- FMFormat::m_nBits[0..3], in plane order --
//      and each is `shl $0x8` then `lea 0x70(%esi,%edx,1)` off argument one.
//      A 0x100 stride from a base of 0x70 is `pMgr->m_ToTables[n]` and
//      nothing else; 0x0045f569 proves the array starts at 0x70.
//    * four separate stores to +0x00, +0x04, +0x08, +0x0c and no loop: VC6
//      at /O2 does not unroll, so the source is four statements.
// ----------------------------------------------------------------------- //
inline void FMFormatConvert::InitInline(FMConvertMgr* pMgr, FMFormat* pFormat)
{
	m_pFormat = pFormat;

	m_pTables[CP_ALPHA] = pMgr->m_ToTables[pFormat->m_nBits[CP_ALPHA]];
	m_pTables[CP_RED]   = pMgr->m_ToTables[pFormat->m_nBits[CP_RED]];
	m_pTables[CP_GREEN] = pMgr->m_ToTables[pFormat->m_nBits[CP_GREEN]];
	m_pTables[CP_BLUE]  = pMgr->m_ToTables[pFormat->m_nBits[CP_BLUE]];
}

void FMFormatConvert::Init(FMConvertMgr* pMgr, FMFormat* pFormat)
{
	InitInline(pMgr, pFormat);
}


// ----------------------------------------------------------------------- //
//  0x0045dbb0 (67 bytes)  FMFormatRead::Init
//
//  The mirror image of the above and the same shape, with two differences
//  read straight off the bytes:
//    * the table it picks is the POINTER array at +0x970, not the flat one
//      at +0x70 -- `mov 0x970(%edx,%esi,4),%esi` (0x0045dbbc), a four-byte
//      stride through nine pointers, which is m_pFromTables.
//    * the format store, 0x0045dbea `mov %eax,(%ecx)`, is at offset ZERO and
//      comes LAST, after all four table stores (+0x04..+0x10).  Written last
//      here for that reason; it is the one statement-order call in this pair
//      that could be the scheduler's rather than the source's.
// ----------------------------------------------------------------------- //
inline void FMFormatRead::InitInline(FMConvertMgr* pMgr, FMFormat* pFormat)
{
	m_pTables[CP_ALPHA] = pMgr->m_pFromTables[pFormat->m_nBits[CP_ALPHA]];
	m_pTables[CP_RED]   = pMgr->m_pFromTables[pFormat->m_nBits[CP_RED]];
	m_pTables[CP_GREEN] = pMgr->m_pFromTables[pFormat->m_nBits[CP_GREEN]];
	m_pTables[CP_BLUE]  = pMgr->m_pFromTables[pFormat->m_nBits[CP_BLUE]];

	m_pFormat = pFormat;
}

void FMFormatRead::Init(FMConvertMgr* pMgr, FMFormat* pFormat)
{
	InitInline(pMgr, pFormat);
}


// ======================================================================= //
//
//  B.  PValue -> DESTINATION FORMAT
//
//  One expression, three return types.  VC6 narrows the whole OR chain to
//  the width of the result, and the three bodies show it doing exactly that:
//  0x0045d580 is all eight-bit (`shl %cl,%al`, `or %bl,%al`), 0x0045d760
//  zero-extends sixteen bits (`movzbw`) and 0x0045d9c0 is thirty-two
//  (`xor %ebx,%ebx` before each byte load).  So the only source difference
//  between the three is the return type.
//
//  WHAT PINS THE TERMS (all three agree instruction for instruction)
//    * the shift comes from the FORMAT, at +0x28 + 4*plane -- 0x30 goes with
//      the `%dh` byte (green), 0x2c with byte two (red), 0x28 with
//      `shr $0x18` (alpha) and 0x34 with `and $0xff` (blue).  That is
//      ltpvalue.h's PValue_GetG/R/A/B exactly, and it fixes plane 0 = alpha.
//    * the index comes from the CONVERTER's own table for that plane, at
//      +0x00 + 4*plane; the table entry is one byte (`mov (%ecx,%eax,1),%al`
//      -- a stride of one, so uint8, not a wider table).
//    * the parameter is a POINTER (0x0045d580 loads `(%eax)` off it) and
//      the loaded PValue is spilled to the dead argument slot so that byte
//      two can be reloaded as a byte -- one local, not four.
//
//  See the header note for the one thing here that is NOT settled: the
//  emitted OR order is green, red, alpha, blue and these are written in
//  plane order.
//
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x0045d580 (94 bytes)
// ----------------------------------------------------------------------- //
inline uint8 FMFormatConvert::Convert8Inline(PValue* pValue)
{
	PValue value = *pValue;

	return (uint8)((m_pTables[CP_ALPHA][PValue_GetA(value)] << m_pFormat->m_nShifts[CP_ALPHA]) |
	               (m_pTables[CP_RED][PValue_GetR(value)]   << m_pFormat->m_nShifts[CP_RED])   |
	               (m_pTables[CP_GREEN][PValue_GetG(value)] << m_pFormat->m_nShifts[CP_GREEN]) |
	               (m_pTables[CP_BLUE][PValue_GetB(value)]  << m_pFormat->m_nShifts[CP_BLUE]));
}

// The result is NAMED in the two narrow wrappers: `return Convert8Inline()`
// compiles the whole chain at 32 bits (108 bytes, a relocation-masked copy of
// 0x0045d9c0, which is how it matched before); with the uint8 /
// uint16 local VC6 narrows the terms as 0x0045d580 (`shl %cl,%al`) and
// 0x0045d760 (`movzbw`) do.
uint8 FMFormatConvert::Convert8(PValue* pValue)
{
	uint8 nColor = Convert8Inline(pValue);

	return nColor;
}


// ----------------------------------------------------------------------- //
//  0x0045d760 (102 bytes)
// ----------------------------------------------------------------------- //
inline uint16 FMFormatConvert::Convert16Inline(PValue* pValue)
{
	PValue value = *pValue;

	return (uint16)((m_pTables[CP_ALPHA][PValue_GetA(value)] << m_pFormat->m_nShifts[CP_ALPHA]) |
	                (m_pTables[CP_RED][PValue_GetR(value)]   << m_pFormat->m_nShifts[CP_RED])   |
	                (m_pTables[CP_GREEN][PValue_GetG(value)] << m_pFormat->m_nShifts[CP_GREEN]) |
	                (m_pTables[CP_BLUE][PValue_GetB(value)]  << m_pFormat->m_nShifts[CP_BLUE]));
}

uint16 FMFormatConvert::Convert16(PValue* pValue)
{
	uint16 nColor = Convert16Inline(pValue);

	return nColor;
}


// ----------------------------------------------------------------------- //
//  0x0045d9c0 (108 bytes)
// ----------------------------------------------------------------------- //
inline PValue FMFormatConvert::Convert32Inline(PValue* pValue)
{
	PValue value = *pValue;

	return (m_pTables[CP_ALPHA][PValue_GetA(value)] << m_pFormat->m_nShifts[CP_ALPHA]) |
	       (m_pTables[CP_RED][PValue_GetR(value)]   << m_pFormat->m_nShifts[CP_RED])   |
	       (m_pTables[CP_GREEN][PValue_GetG(value)] << m_pFormat->m_nShifts[CP_GREEN]) |
	       (m_pTables[CP_BLUE][PValue_GetB(value)]  << m_pFormat->m_nShifts[CP_BLUE]);
}

PValue FMFormatConvert::Convert32(PValue* pValue)
{
	return Convert32Inline(pValue);
}


// ======================================================================= //
//
//  C.  SOURCE FORMAT -> PValue
//
//  WHAT PINS THE TERMS
//    * the packing is a `shl $0x8` CHAIN -- 0x0045dc38, 0x0045dc71,
//      0x0045dc77 -- i.e. ((((t0<<8)|t1)<<8|t2)<<8)|t3.  That is VC6's
//      reassociation of `(a<<24)|(r<<16)|(g<<8)|b`, the same one lt2new3.cpp
//      already proved on 0x0045d990, and the four terms come out in PLANE
//      ORDER here, which is the evidence the writers' green-first order is
//      the compiler's and not the source's.
//    * `and %edi,%eax` with the MASK in %eax and the pixel in %edi
//      (0x0045dc20) -- the mask was loaded into the accumulator and the
//      pixel and-ed into it, so the mask is the left operand.
//    * `shr %cl` after the and, with %cl from +0x28 + 4*plane.
//    * the table index is the result, and the table is m_pFromTables', one
//      byte per entry.
//
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x0045dc00 (130 bytes)  eight-bit source
//  `xor %eax,%eax; mov (%ecx),%al` (0x0045dc0c) -- a zero-extended BYTE load
//  into a 32-bit value that then survives in %edi across all four planes.
//  The `sub $0x10` frame is a `uint32 planes[4]` local (one term spilled to
//  it at 0x0045dc44); as a single OR expression it is 107 bytes with no frame.
// ----------------------------------------------------------------------- //
inline PValue FMFormatRead::Read8Inline(uint8* pSrc)
{
	uint32 nData = *pSrc;
	uint32 planes[NUM_COLORPLANES];

	planes[CP_ALPHA] = m_pTables[CP_ALPHA][(m_pFormat->m_Masks[CP_ALPHA] & nData) >> m_pFormat->m_nShifts[CP_ALPHA]];
	planes[CP_RED]   = m_pTables[CP_RED][(m_pFormat->m_Masks[CP_RED] & nData) >> m_pFormat->m_nShifts[CP_RED]];
	planes[CP_GREEN] = m_pTables[CP_GREEN][(m_pFormat->m_Masks[CP_GREEN] & nData) >> m_pFormat->m_nShifts[CP_GREEN]];
	planes[CP_BLUE]  = m_pTables[CP_BLUE][(m_pFormat->m_Masks[CP_BLUE] & nData) >> m_pFormat->m_nShifts[CP_BLUE]];

	return (planes[CP_ALPHA] << 24) | (planes[CP_RED] << 16) | (planes[CP_GREEN] << 8) | planes[CP_BLUE];
}

PValue FMFormatRead::Read8(uint8* pSrc)
{
	return Read8Inline(pSrc);
}


// ----------------------------------------------------------------------- //
//  0x0045e0d0 (129 bytes)  sixteen-bit source
//  Instruction for instruction the same function; the ONE difference is
//  0x0045e0e3 `mov (%eax),%si` against 0x0045dc11 `mov (%ecx),%al`.
// ----------------------------------------------------------------------- //
inline PValue FMFormatRead::Read16Inline(uint16* pSrc)
{
	uint32 nData = *pSrc;
	uint32 planes[NUM_COLORPLANES];

	planes[CP_ALPHA] = m_pTables[CP_ALPHA][(m_pFormat->m_Masks[CP_ALPHA] & nData) >> m_pFormat->m_nShifts[CP_ALPHA]];
	planes[CP_RED]   = m_pTables[CP_RED][(m_pFormat->m_Masks[CP_RED] & nData) >> m_pFormat->m_nShifts[CP_RED]];
	planes[CP_GREEN] = m_pTables[CP_GREEN][(m_pFormat->m_Masks[CP_GREEN] & nData) >> m_pFormat->m_nShifts[CP_GREEN]];
	planes[CP_BLUE]  = m_pTables[CP_BLUE][(m_pFormat->m_Masks[CP_BLUE] & nData) >> m_pFormat->m_nShifts[CP_BLUE]];

	return (planes[CP_ALPHA] << 24) | (planes[CP_RED] << 16) | (planes[CP_GREEN] << 8) | planes[CP_BLUE];
}

PValue FMFormatRead::Read16(uint16* pSrc)
{
	return Read16Inline(pSrc);
}


// ----------------------------------------------------------------------- //
//  0x0045e490 (81 bytes)  thirty-two-bit source
//
//  NOT the same body: there is no mask and no table here at all.  The four
//  terms are `(nData >> shift) & 0xff` -- 0x0045e4a0 `shr %cl,%eax` then
//  0x0045e4ae `and $0xff,%eax`, in that order, four times -- which is what a
//  format with eight bits in every plane degenerates to (mask-then-shift and
//  shift-then-mask agree, and m_pFromTables[8] is the identity).  The class
//  is still FMFormatRead: 0x0045e496 reads `this`+0x00 for the format.
//  The four terms go through a `planes[4]` array like Read8/16's; written
//  as one OR expression VC6 drops the alpha term's `and` (73 bytes, not 81).
// ----------------------------------------------------------------------- //
inline PValue FMFormatRead::Read32Inline(uint32* pSrc)
{
	uint32 nData = *pSrc;
	uint32 planes[NUM_COLORPLANES];

	planes[CP_ALPHA] = (nData >> m_pFormat->m_nShifts[CP_ALPHA]) & 0xff;
	planes[CP_RED]   = (nData >> m_pFormat->m_nShifts[CP_RED])   & 0xff;
	planes[CP_GREEN] = (nData >> m_pFormat->m_nShifts[CP_GREEN]) & 0xff;
	planes[CP_BLUE]  = (nData >> m_pFormat->m_nShifts[CP_BLUE])  & 0xff;

	return (planes[CP_ALPHA] << 24) | (planes[CP_RED] << 16) | (planes[CP_GREEN] << 8) | planes[CP_BLUE];
}

PValue FMFormatRead::Read32(uint32* pSrc)
{
	return Read32Inline(pSrc);
}


// ======================================================================= //
//
//  D.  0x0045eb60 (47 bytes)  FMPaletteRead::ReadARGB
//
//  lt2new3.cpp's 0x0045d990 with the alpha byte put back.  Same class, same
//  one member, same `lea (%ecx,%eax,4)` four-byte stride, same `%dh`/`%dl`
//  byte-pair idiom -- the only differences are that the first load is at
//  +0 rather than +1 and there is a second `shl $0x8`/`or` step.
//
// ======================================================================= //
inline PValue FMPaletteRead::ReadARGBInline(uint8* pSrc)
{
	return (m_pPalette[*pSrc].m_a << 24) | (m_pPalette[*pSrc].m_r << 16) | (m_pPalette[*pSrc].m_g << 8) | m_pPalette[*pSrc].m_b;
}

PValue FMPaletteRead::ReadARGB(uint8* pSrc)
{
	FMPaletteEntry* pEntry = &m_pPalette[*pSrc];

	return (pEntry->m_a << 24) | (pEntry->m_r << 16) | (pEntry->m_g << 8) | pEntry->m_b;
}


// ======================================================================= //
//
//  E.  THE FIVE SINGLE-PIXEL ENTRY POINTS  (0x0045ed50 .. 0x0045efd0)
//
//  WHAT THEY ARE.  0x0045f5e0 and 0x0045f620, just past this cluster, are
//  FMConvertMgr methods with `ret $0xc`.  Each reads
//  `pFormat->m_nCode`, indexes a table of function pointers by it
//  (0x004aceb8 for the first, 0x004aced8 for the second), returns 1 if the
//  slot is null, and otherwise pushes -- in this order --
//
//      push <its own third argument>
//      lea  <its own second argument's slot>; push
//      push <the FMFormat*>
//      push <the manager>
//      call *%eax ; add $0x10,%esp
//
//  so every slot in both tables is
//  `__cdecl void (FMConvertMgr*, FMFormat*, void*, void*)`.  Reading the
//  five bodies back says which is which: three of them load a PValue out of
//  argument three and store one, two or four bytes through argument four
//  (writers), and two load one or two bytes through argument three and store
//  a PValue through argument four (readers).
//
//  WHAT PINS THE SHAPE.  Each is the corresponding helper class built on the
//  stack and used once, with BOTH calls inlined (the *Inline twins).  The evidence is exact:
//  0x0045ed50 reserves `sub $0x14` -- sizeof(FMFormatConvert) -- computes
//  the four `0x70 + (bits << 8)` table pointers into registers, spills only
//  m_pTables[3] and only to that local's own +0x0c slot (0x0045ed8b), never
//  stores m_pFormat at all (it stays in %esi), and then runs the byte pack
//  instruction for instruction as Convert8.  0x0045edf0 is the same source
//  with no frame at all, VC6 having kept the whole struct in registers.
//  0x0045ef20 and 0x0045efd0 do it with FMFormatRead, spilling
//  m_pTables[3] to +0x10 of the same 0x14-byte local (0x0045ef5e).
//
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x0045ed50 (145 bytes)   `mov %dl,(%ecx)` -- one byte out
// ----------------------------------------------------------------------- //
void fm_WritePixel8(FMConvertMgr* pMgr, FMFormat* pFormat, void* pValue, void* pDest)
{
	FMFormatConvert convert;

	convert.InitInline(pMgr, pFormat);

	*((uint8*)pDest) = convert.Convert8Inline((PValue*)pValue);
}


// ----------------------------------------------------------------------- //
//  0x0045edf0 (141 bytes)   `mov %bp,(%eax)` -- two bytes out
// ----------------------------------------------------------------------- //
void fm_WritePixel16(FMConvertMgr* pMgr, FMFormat* pFormat, void* pValue, void* pDest)
{
	FMFormatConvert convert;

	convert.InitInline(pMgr, pFormat);

	*((uint16*)pDest) = convert.Convert16Inline((PValue*)pValue);
}


// ----------------------------------------------------------------------- //
//  0x0045ee80 (158 bytes)   `mov %ebp,(%eax)` -- four bytes out
// ----------------------------------------------------------------------- //
void fm_WritePixel32(FMConvertMgr* pMgr, FMFormat* pFormat, void* pValue, void* pDest)
{
	FMFormatConvert convert;

	convert.InitInline(pMgr, pFormat);

	*((PValue*)pDest) = convert.Convert32Inline((PValue*)pValue);
}


// ----------------------------------------------------------------------- //
//  0x0045ef20 (168 bytes)   `mov (%esi),%cl` in, `mov %edx,(%eax)` out
// ----------------------------------------------------------------------- //
void fm_ReadPixel8(FMConvertMgr* pMgr, FMFormat* pFormat, void* pSrc, void* pValue)
{
	FMFormatRead read;

	read.InitInline(pMgr, pFormat);

	*((PValue*)pValue) = read.Read8Inline((uint8*)pSrc);
}


// ----------------------------------------------------------------------- //
//  0x0045efd0 (167 bytes)   `mov (%ecx),%si` in, `mov %edx,(%eax)` out
// ----------------------------------------------------------------------- //
void fm_ReadPixel16(FMConvertMgr* pMgr, FMFormat* pFormat, void* pSrc, void* pValue)
{
	FMFormatRead read;

	read.InitInline(pMgr, pFormat);

	*((PValue*)pValue) = read.Read16Inline((uint16*)pSrc);
}


// ======================================================================= //
//
//  F.  THE THIRTEEN BLITTERS
//
//  All thirteen are `__cdecl LTRESULT (FMConvertMgr*, FMConvertRequest*)`.
//  The dispatcher proves it: 0x0045f3f1 and 0x0045f3fa turn each format into
//  a small code, 0x0045f3ff indexes 0x004acdb8 by `dest*8 + src` (`lea
//  (%ebx,%eax,8)`), and 0x0045f418 does `push %esi; push %edi; call *%eax;
//  add $0x8,%esp` with %edi the manager and %esi the request, returning
//  whatever the blitter left in %eax.
//
//  THE SKELETON, read off 0x0045dc90 and identical in all thirteen:
//
//      <build the helper(s) on the stack>
//      pSrcRow  = pRequest->m_pSrcData;         0x0045dcbc
//      pDestRow = pRequest->m_pDestData;        0x0045dcbf
//      for (y = pRequest->m_nHeight; y; y--)       0x0045dcb9/0x0045dcc6
//      {
//          pSrc  = pSrcRow;  pDest = pDestRow;  0x0045dce0, hoisted above
//                                               the flags branch
//          nWidth = pRequest->m_nWidth;         0x0045dcda
//          ... for (x = nWidth; x; x--) ...
//          if (pRequest->m_Flags & 1)  { ... *pDest |= ...; }   0x0045dcd7
//          else                        { ... *pDest  = ...; }   0x0045dce2
//          pSrcRow  += pRequest->m_nSrcPitch;   0x0045ddae
//          pDestRow += pRequest->m_nDestPitch;  0x0045ddb5
//      }
//      return LT_OK;                            0x0045ddd6 / 0x0045dde0
//
//  Five things in that skeleton are read, not assumed:
//    * BOTH loops count DOWN: the counters are `dec ; jne` (0x0045ddc0,
//      0x0045dd17) and nothing reads them.  `for (x = 0; x < n; x++)` gives
//      `inc ; cmp ; jb` instead, 46 bytes longer over 0x0045ddf0.  The width
//      is a named row local: shipped loads it into %eax and COPIES it into
//      the counter (0x0045de49 `mov %eax,%ebx`); `x = pRequest->m_nWidth`
//      loads the counter directly and is 10 bytes short.
//    * BOTH loop guards are `test %eax,%eax; je` and not `jle`, so the width
//      and height are UNSIGNED -- a signed `x < nWidth` would give `jle`.
//    * m_Flags and m_nWidth are re-read from the request on EVERY row
//      (0x0045dcd7, 0x0045dcda are inside the outer loop), which is what
//      stores through pDest force.
//    * the two arms are the same statement with `|=` against `=`; the "or"
//      arm is the fall-through of `test $0x1`.
//    * the two epilogues (0x0045ddd3 and 0x0045dddd) differ only by a
//      `pop %edi`: one `return LT_OK` shrink-wrapped, not two returns.
//
//  INLINE HERE, CALL THERE.  The two-arm blitters CALL the helpers from
//  the `|=` arm and EXPAND them in the `=` arm (0x0045dd07 calls 0x0045d760;
//  0x0045dd52..0x0045dd94 is that function expanded), while the four-arm
//  ones (0x0045ddf0, 0x0045e2f0, 0x0045e4f0, 0x0045e680) call everywhere.
//  This TU is /O2 = /Ob1, so only `inline` bodies expand -- and they expand
//  at every site (measured: marking Convert16 inline grows 0x0045e680 to
//  785 bytes; /Ob2 inlines everything, 0x0045ddf0 749 bytes).  So each
//  helper has an `inline` *Inline twin and the real out-of-line member is a
//  one-line wrapper around it (the wrappers are byte-identical to 0x0045d530
//  etc.); an expanded site names the twin.  The expanded `=` arms also want
//  PValue `value` at ROW scope and the converted result named
//  (`nColor = ...Inline(&value); *pDest = nColor;`) -- without those the
//  arms are register-swapped nears (0x0045da30 slot swap, 0x0045dc90 /
//  0x0045e160 / 0x0045e820 / 0x0045d5e0 table-pointer register).
//
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  FMPaletteRead::ReadRGB -- 0x0045d990, ALREADY BYTE-IDENTICAL from
//  src/engine/lithtech/lt2new3.cpp (as CPaletteSource3::ReadPixel).
//
//  It is repeated here, under a different class name so the mangled symbols
//  cannot collide, for one reason only: 0x0045d5e0 and 0x0045d7d0 EXPAND IT
//  INLINE at some sites (0x0045d63e..0x0045d650) and call it at others.
//  The expanded twin indexes m_pPalette[*pSrc] afresh per field: through a
//  named pEntry the expanded byte loads come out in the wrong order (a
//  2-instruction near on 0x0045e9f0/0x0045eb90 with ReadARGB).  The
//  out-of-line body keeps pEntry, which is what 0x0045d990/0x0045eb60 need.
// ----------------------------------------------------------------------- //
inline PValue FMPaletteRead::ReadRGBInline(uint8* pSrc)
{
	return (m_pPalette[*pSrc].m_r << 16) | (m_pPalette[*pSrc].m_g << 8) | m_pPalette[*pSrc].m_b;
}

PValue FMPaletteRead::ReadRGB(uint8* pSrc)
{
	FMPaletteEntry* pEntry = &m_pPalette[*pSrc];

	return (pEntry->m_r << 16) | (pEntry->m_g << 8) | pEntry->m_b;
}


// ----------------------------------------------------------------------- //
//  0x0045d1e0 (461 bytes)  fm_BlitSame -- source and destination share a
//  format, so rows are copied (or OR-ed) raw.
//
//    * the row width in bytes is m_nWidth << GetBPPShift() (0x0045d1f1).
//    * formats 4/5/6 (the DXT codes fm_GetImageDataSize special-cases) are
//      "compressed": a mismatch is LT_ERROR (0x0045d242), and a compressed
//      pair is one memcpy of fm_GetImageDataSize bytes (0x0045d278), with
//      the OR flag refused (0x0045d26c).
//    * the OR arm splits the row into dwords, words or bytes by the
//      alignment of the two data pointers (0x0045d2b7..0x0045d2e8) and ORs
//      the DEST into the SOURCE (0x0045d30e `or %esi,(%eax)`, %eax walking
//      m_pSrcData) -- as shipped.
//    * the plain arm is one memcpy per row (0x0045d37d).
// ----------------------------------------------------------------------- //
inline LTBOOL fm_IsCompressed(uint32 nCode)
{
	return nCode == 4 || nCode == 5 || nCode == 6;
}

uint32 fm_GetImageDataSize(uint32 nFormatCode, uint32 nWidth, uint32 nHeight);

LTRESULT fm_BlitSame(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	uint32 nRowBytes, nDwords, nWords, nBytes, x, y, nWidth, nSrcCode;
	uint8* pDestRow;
	uint8* pSrcRow;

	nWidth    = pRequest->m_nWidth;
	nRowBytes = nWidth << pRequest->m_pSrcFormat->GetBPPShift();
	pSrcRow   = pRequest->m_pSrcData;
	pDestRow  = pRequest->m_pDestData;
	nSrcCode  = pRequest->m_pSrcFormat->m_nCode;

	if (fm_IsCompressed(nSrcCode) != fm_IsCompressed(pRequest->m_pDestFormat->m_nCode))
		return LT_ERROR;

	if (fm_IsCompressed(nSrcCode))
	{
		if (pRequest->m_Flags & FMBLIT_OR)
			return LT_ERROR;

		memcpy(pRequest->m_pDestData, pRequest->m_pSrcData,
		       fm_GetImageDataSize(nSrcCode, nWidth, pRequest->m_nHeight));
		return LT_OK;
	}

	if (pRequest->m_Flags & FMBLIT_OR)
	{
		nDwords = 0;
		nWords  = 0;
		nBytes  = nRowBytes;

		if (!((uint32)pSrcRow & 3) && !((uint32)pDestRow & 3))
		{
			nDwords = nBytes >> 2;
			nBytes -= nDwords * 4;
		}
		else if (!((uint32)pSrcRow & 1) && !((uint32)pDestRow & 1))
		{
			nWords  = nBytes >> 1;
			nBytes -= nWords * 2;
		}

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint8* pIn  = pDestRow;
			uint8* pOut = pSrcRow;

			for (x = nDwords; x; x--)
			{
				*(uint32*)pOut |= *(uint32*)pIn;
				pIn  += 4;
				pOut += 4;
			}

			for (x = nWords; x; x--)
			{
				*(uint16*)pOut |= *(uint16*)pIn;
				pIn  += 2;
				pOut += 2;
			}

			for (x = nBytes; x; x--)
			{
				*pOut |= *pIn;
				pIn++;
				pOut++;
			}

			pDestRow += pRequest->m_nDestPitch;
			pSrcRow  += pRequest->m_nSrcPitch;
		}
	}
	else
	{
		for (y = pRequest->m_nHeight; y; y--)
		{
			memcpy(pDestRow, pSrcRow, nRowBytes);

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045d3b0 (378 bytes)  8-bit palettised source -> 8-bit destination
//
//  fm_BlitPal8To16 with a uint8 destination and Convert8 (entry 1 of the
//  blitter table 0x004acdb8).  The OR arm calls Convert8 (0x0045d437) and
//  the assign arm expands it (0x0045d465..0x0045d4d5).
// ----------------------------------------------------------------------- //
LTRESULT fm_BlitPal8To8(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMPaletteRead   palette;
	FMFormatConvert convert;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	palette.m_pPalette = (FMPaletteEntry*)pRequest->m_pSrcPalette;
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint8*  pSrc  = pSrcRow;
		uint8*  pDest = pDestRow;
		PValue  value;
		uint8   nColor;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = palette.ReadRGBInline(pSrc);
				*pDest |= convert.Convert8(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = palette.ReadRGBInline(pSrc);
				nColor = convert.Convert8Inline(&value);
				*pDest = nColor;

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045d5e0 (373 bytes)  8-bit palettised source -> 16-bit destination
//
//  No source reader and no format guard: the source is a palette, so only
//  the destination converter is built (0x0045d5ff).  The palette pointer is
//  filed into its one-word object at 0x0045d5fb, before that call.
//  0x0045d664 `or %ax,(%esi)` / 0x0045d70a `mov %bx,-0x2(%esi)` are the two
//  arms; 0x0045d66c/0x0045d697 `add $0x2,%esi` makes the destination cursor
//  a uint16*.
// ----------------------------------------------------------------------- //
LTRESULT fm_BlitPal8To16(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMPaletteRead   palette;
	FMFormatConvert convert;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	palette.m_pPalette = (FMPaletteEntry*)pRequest->m_pSrcPalette;
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint8*  pSrc  = pSrcRow;
		uint16* pDest = (uint16*)pDestRow;
		PValue  value;
		uint16  nColor;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = palette.ReadRGBInline(pSrc);
				*pDest |= convert.Convert16(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = palette.ReadRGBInline(pSrc);
				nColor = convert.Convert16Inline(&value);
				*pDest = nColor;

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045dc90 (342 bytes)  8-bit source -> 16-bit destination
//
//  The reference skeleton.  Reader built at 0x0045dca6, writer at
//  0x0045dcb4; 0x0045dd0c `or %ax,(%esi)` and 0x0045dd9a
//  `mov %bx,-0x2(%esi)`.
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit8To16(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMFormatRead    read;
	FMFormatConvert convert;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	read.Init(pMgr, pRequest->m_pSrcFormat);
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint8*  pSrc  = pSrcRow;
		uint16* pDest = (uint16*)pDestRow;
		PValue  value;
		uint16  nColor;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = read.Read8(pSrc);
				*pDest |= convert.Convert16(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = read.Read8(pSrc);
				nColor = convert.Convert16Inline(&value);
				*pDest = nColor;

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045df80 (328 bytes)  16-bit source -> 8-bit destination
//
//  The same function with the two cursors' strides exchanged: 0x0045dffe
//  `add $0x2,%ebp` on the source and 0x0045e009 `inc %edi` on the
//  destination, and 0x0045dffc..0x0045e007 is the read-modify-write byte OR
//  (`mov (%edi),%cl; or %al,%cl; mov %cl,(%edi)`) that a uint8 `|=` needs
//  and a uint16 one does not.
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit16To8(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMFormatRead    read;
	FMFormatConvert convert;
	PValue          value;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	read.Init(pMgr, pRequest->m_pSrcFormat);
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint16* pSrc  = (uint16*)pSrcRow;
		uint8*  pDest = pDestRow;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = read.Read16(pSrc);
				*pDest |= convert.Convert8(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = read.Read16(pSrc);
				*pDest = convert.Convert8Inline(&value);

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045e9f0 (358 bytes)  32-bit palettised source -> 16-bit destination
//
//  Same as 0x0045d5e0 but reading the palette entry's ALPHA byte too:
//  0x0045ea4b calls 0x0045eb60 in the first arm and 0x0045ea9d
//  (`mov (%eax),%ch`, the +0 load) is that function expanded in the second.
//  The frame is 0x28 rather than 0x38 because there is no reader object.
// ----------------------------------------------------------------------- //
LTRESULT fm_BlitPal32To16(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMPaletteRead   palette;
	FMFormatConvert convert;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	palette.m_pPalette = (FMPaletteEntry*)pRequest->m_pSrcPalette;
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint8*  pSrc  = pSrcRow;
		uint16* pDest = (uint16*)pDestRow;
		PValue  value;
		uint16  nColor;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = palette.ReadARGB(pSrc);
				*pDest |= convert.Convert16(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = palette.ReadARGBInline(pSrc);
				nColor = convert.Convert16Inline(&value);
				*pDest = nColor;

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045eb90 (375 bytes)  32-bit palettised source -> 32-bit destination
//
//  0x0045e9f0 with Convert32 in place of Convert16: 0x0045ec01 calls
//  0x0045d9c0 in the first arm and 0x0045ec64..0x0045ecae is that function
//  expanded in the second.  0x0045ec08 `add $0x4,%edi` makes the destination
//  a PValue*; 0x0045ec0d `inc %esi` keeps the source one byte wide.
// ----------------------------------------------------------------------- //
LTRESULT fm_BlitPal32To32(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMPaletteRead   palette;
	FMFormatConvert convert;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	palette.m_pPalette = (FMPaletteEntry*)pRequest->m_pSrcPalette;
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint8*  pSrc  = pSrcRow;
		PValue* pDest = (PValue*)pDestRow;
		PValue  value;
		PValue  nColor;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = palette.ReadARGB(pSrc);
				*pDest |= convert.Convert32(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = palette.ReadARGBInline(pSrc);
				nColor = convert.Convert32Inline(&value);
				*pDest = nColor;

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ======================================================================= //
//
//  F2.  THE GUARDED BLITTERS
//
//  Eight of the thirteen open with a call to 0x0045f1e0, and the two
//  operands say which of three guards it is.  Read off the `%ecx` (the
//  `this`) and the pushed argument at each site:
//
//    0x0045da30  src vs dest   -- `mov (%ebx),%ecx`   / `push` +0x10
//    0x0045e160  src vs dest
//    0x0045e820  dest vs mgr   -- `mov 0x10(%edi),%ecx` / `push %esi`
//    0x0045d7d0  dest vs mgr
//    0x0045ddf0  dest vs mgr
//    0x0045e2f0  dest vs mgr
//    0x0045e4f0  src  vs mgr   -- `mov (%esi),%ecx`   / `push %edi`
//    0x0045e680  src  vs mgr
//
//  `push %esi` (the bare manager) where an FMFormat* is wanted is
//  `&pMgr->m_FormatPValue` -- the manager's first member is a PFormat and
//  the offset is zero, which is exactly why 0x0045f397 can install a vptr at
//  the manager's +0x00.  So "dest vs mgr" asks "is the destination already
//  PValue?", in which case the reader alone does the work and no converter
//  is built; "src vs mgr" asks the mirror, and the converter alone does it.
//  "src vs dest" hands the whole blit to 0x0045d1e0.
//
//  DECLARATION SCOPE IS LOAD-BEARING HERE and is written the way the slots
//  fall out.  0x0045da30 and 0x0045e160 return from the guard, so their two
//  helpers are function-scope and land read-high/convert-low, the same way
//  0x0045dc90's do.  0x0045ddf0's two arms each build their OWN reader (two
//  separate calls to 0x0045dbb0, 0x0045de18 and 0x0045deb5, sharing one slot
//  at -0x28), so the helpers are declared per arm.
//
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  0x0045da30 (369 bytes)  8-bit source -> 8-bit destination
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit8To8(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMFormatRead    read;
	FMFormatConvert convert;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	if (pRequest->m_pSrcFormat->IsSame(pRequest->m_pDestFormat))
		return fm_BlitSame(pMgr, pRequest);

	read.Init(pMgr, pRequest->m_pSrcFormat);
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint8* pSrc  = pSrcRow;
		uint8* pDest = pDestRow;
		PValue value;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = read.Read8(pSrc);
				*pDest |= convert.Convert8(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = read.Read8(pSrc);
				*pDest = convert.Convert8Inline(&value);

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045e160 (386 bytes)  16-bit source -> 16-bit destination
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit16To16(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMFormatRead    read;
	FMFormatConvert convert;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	if (pRequest->m_pSrcFormat->IsSame(pRequest->m_pDestFormat))
		return fm_BlitSame(pMgr, pRequest);

	read.Init(pMgr, pRequest->m_pSrcFormat);
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint16* pSrc  = (uint16*)pSrcRow;
		uint16* pDest = (uint16*)pDestRow;
		PValue  value;
		uint16  nColor;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = read.Read16(pSrc);
				*pDest |= convert.Convert16(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = read.Read16(pSrc);
				nColor = convert.Convert16Inline(&value);
				*pDest = nColor;

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045e820 (460 bytes)  32-bit source -> 32-bit destination
//
//  The odd one of the three same-width blitters: its guard is dest-vs-mgr
//  (0x0045e82e `mov 0x10(%edi),%ecx`, 0x0045e82d `push %esi`), not
//  src-vs-dest, and it still tails into 0x0045d1e0.
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit32To32(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	FMFormatRead    read;
	FMFormatConvert convert;
	uint8*          pSrcRow;
	uint8*          pDestRow;
	uint32          x, y;

	if (pRequest->m_pDestFormat->IsSame(&pMgr->m_FormatPValue))
		return fm_BlitSame(pMgr, pRequest);

	read.Init(pMgr, pRequest->m_pSrcFormat);
	convert.Init(pMgr, pRequest->m_pDestFormat);

	pSrcRow  = pRequest->m_pSrcData;
	pDestRow = pRequest->m_pDestData;

	for (y = pRequest->m_nHeight; y; y--)
	{
		uint32* pSrc  = (uint32*)pSrcRow;
		PValue* pDest = (PValue*)pDestRow;
		PValue  nColor;
		PValue  value;

		uint32  nWidth = pRequest->m_nWidth;

		if (pRequest->m_Flags & FMBLIT_OR)
		{
			for (x = nWidth; x; x--)
			{
				value   = read.Read32(pSrc);
				*pDest |= convert.Convert32(&value);

				pSrc++;
				pDest++;
			}
		}
		else
		{
			for (x = nWidth; x; x--)
			{
				value  = read.Read32Inline(pSrc);
				nColor = convert.Convert32Inline(&value);
				*pDest = nColor;

				pSrc++;
				pDest++;
			}
		}

		pSrcRow  += pRequest->m_nSrcPitch;
		pDestRow += pRequest->m_nDestPitch;
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045d7d0 (442 bytes)  8-bit palettised source -> 32-bit destination
//
//  0x0045d7ea loads +0x0c AFTER the guard's call and each arm stores it
//  to the same slot (0x0045d7f9 and 0x0045d8af, the dead first-parameter
//  slot): each arm declares and fills its OWN palette object and VC6
//  hoists the common load.  One palette object filed ahead of the guard
//  stores it before the call instead (the previous spelling, 4 bytes short).
//
//  The fast arm builds NO converter -- the destination already IS a PValue,
//  so 0x0045d833 ORs the palette read straight in and 0x0045d873 stores it
//  -- while the slow arm builds one at 0x0045d8b9 and runs it at
//  0x0045d903/0x0045d949.  0x0045d82e and 0x0045d8f1 CALL 0x0045d990 (the
//  `|=` arms); 0x0045d851..0x0045d873 and 0x0045d91e..0x0045d940 expand it.
// ----------------------------------------------------------------------- //
LTRESULT fm_BlitPal8To32(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	if (pRequest->m_pDestFormat->IsSame(&pMgr->m_FormatPValue))
	{
		FMPaletteRead palette;

		palette.m_pPalette = (FMPaletteEntry*)pRequest->m_pSrcPalette;

		uint8*  pSrcRow;
		uint8*  pDestRow;
		uint32  x, y;

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint8*  pSrc  = pSrcRow;
			PValue* pDest = (PValue*)pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					*pDest |= palette.ReadRGB(pSrc);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					*pDest = palette.ReadRGBInline(pSrc);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}
	else
	{
		FMPaletteRead   palette;
		FMFormatConvert convert;
		PValue          value;
		uint8*          pSrcRow;
		uint8*          pDestRow;
		uint32          x, y;

		palette.m_pPalette = (FMPaletteEntry*)pRequest->m_pSrcPalette;
		convert.Init(pMgr, pRequest->m_pDestFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint8*  pSrc  = pSrcRow;
			PValue* pDest = (PValue*)pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					value   = palette.ReadRGB(pSrc);
					*pDest |= convert.Convert32(&value);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					value  = palette.ReadRGBInline(pSrc);
					*pDest = convert.Convert32(&value);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045ddf0 (400 bytes)  8-bit source -> 32-bit destination
//
//  The four-way family's first member, and the one decoded in full.  Both
//  arms build their own reader (0x0045de18 and 0x0045deb5 -- two calls to
//  0x0045dbb0, one slot); only the slow arm builds a converter
//  (0x0045dec3).  The fast arm's inner statement has no temporary at all
//  (0x0045de55 `mov (%edi),%ecx; or %eax,%ecx`), the slow arm's spills the
//  read into 0x48(%esp) and takes its address (0x0045df00/0x0045df04).
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit8To32(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	if (pRequest->m_pDestFormat->IsSame(&pMgr->m_FormatPValue))
	{
		FMFormatRead read;
		uint8*       pSrcRow;
		uint8*       pDestRow;
		uint32       x, y;

		read.Init(pMgr, pRequest->m_pSrcFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint8*  pSrc  = pSrcRow;
			PValue* pDest = (PValue*)pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					*pDest |= read.Read8(pSrc);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					*pDest = read.Read8(pSrc);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}
	else
	{
		FMFormatRead    read;
		FMFormatConvert convert;
		PValue          value;
		uint8*          pSrcRow;
		uint8*          pDestRow;
		uint32          x, y;

		read.Init(pMgr, pRequest->m_pSrcFormat);
		convert.Init(pMgr, pRequest->m_pDestFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint8*  pSrc  = pSrcRow;
			PValue* pDest = (PValue*)pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					value   = read.Read8(pSrc);
					*pDest |= convert.Convert32(&value);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					value  = read.Read8(pSrc);
					*pDest = convert.Convert32(&value);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045e2f0 (408 bytes)  16-bit source -> 32-bit destination
//
//  0x0045ddf0 with Read16 for Read8 and a two-byte source stride.  Call
//  sequence identical target for target (0x0045f1e0, 0x0045dbb0, 0x0045e0d0
//  twice, then 0x0045dbb0, 0x0045d530, 0x0045e0d0/0x0045d9c0 twice), and the
//  guard is the same dest-vs-manager test at 0x0045e2ff.
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit16To32(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	if (pRequest->m_pDestFormat->IsSame(&pMgr->m_FormatPValue))
	{
		FMFormatRead read;
		uint8*       pSrcRow;
		uint8*       pDestRow;
		uint32       x, y;

		read.Init(pMgr, pRequest->m_pSrcFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint16* pSrc  = (uint16*)pSrcRow;
			PValue* pDest = (PValue*)pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					*pDest |= read.Read16(pSrc);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					*pDest = read.Read16(pSrc);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}
	else
	{
		FMFormatRead    read;
		FMFormatConvert convert;
		PValue          value;
		uint8*          pSrcRow;
		uint8*          pDestRow;
		uint32          x, y;

		read.Init(pMgr, pRequest->m_pSrcFormat);
		convert.Init(pMgr, pRequest->m_pDestFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint16* pSrc  = (uint16*)pSrcRow;
			PValue* pDest = (PValue*)pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					value   = read.Read16(pSrc);
					*pDest |= convert.Convert32(&value);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					value  = read.Read16(pSrc);
					*pDest = convert.Convert32(&value);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045e4f0 (398 bytes)  32-bit source -> 8-bit destination
//
//  The mirror of 0x0045ddf0: the guard is SOURCE-vs-manager (0x0045e4ff
//  `mov (%esi),%ecx`, 0x0045e501 `push %edi`), so the fast arm skips the
//  READER and hands the source pointer to the converter unchanged --
//  0x0045e54b `push %ebp` is the raw source cursor going straight into
//  0x0045d580, and 0x0045e557 `add $0x4,%ebp` makes it a PValue*.
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit32To8(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	if (pRequest->m_pSrcFormat->IsSame(&pMgr->m_FormatPValue))
	{
		FMFormatConvert convert;
		uint8*          pSrcRow;
		uint8*          pDestRow;
		uint32          x, y;

		convert.Init(pMgr, pRequest->m_pDestFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			PValue* pSrc  = (PValue*)pSrcRow;
			uint8*  pDest = pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					*pDest |= convert.Convert8(pSrc);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					*pDest = convert.Convert8(pSrc);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}
	else
	{
		FMFormatRead    read;
		FMFormatConvert convert;
		PValue          value;
		uint8*          pSrcRow;
		uint8*          pDestRow;
		uint32          x, y;

		read.Init(pMgr, pRequest->m_pSrcFormat);
		convert.Init(pMgr, pRequest->m_pDestFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint32* pSrc  = (uint32*)pSrcRow;
			uint8*  pDest = pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					value   = read.Read32(pSrc);
					*pDest |= convert.Convert8(&value);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					value  = read.Read32(pSrc);
					*pDest = convert.Convert8(&value);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0045e680 (404 bytes)  32-bit source -> 16-bit destination
//
//  0x0045e4f0 with Convert16 for Convert8 and a two-byte destination
//  stride; the guard at 0x0045e68f is the same source-vs-manager test.
// ----------------------------------------------------------------------- //
LTRESULT fm_Blit32To16(FMConvertMgr* pMgr, FMConvertRequest* pRequest)
{
	if (pRequest->m_pSrcFormat->IsSame(&pMgr->m_FormatPValue))
	{
		FMFormatConvert convert;
		uint8*          pSrcRow;
		uint8*          pDestRow;
		uint32          x, y;

		convert.Init(pMgr, pRequest->m_pDestFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			PValue* pSrc  = (PValue*)pSrcRow;
			uint16* pDest = (uint16*)pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					*pDest |= convert.Convert16(pSrc);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					*pDest = convert.Convert16(pSrc);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}
	else
	{
		FMFormatRead    read;
		FMFormatConvert convert;
		PValue          value;
		uint8*          pSrcRow;
		uint8*          pDestRow;
		uint32          x, y;

		read.Init(pMgr, pRequest->m_pSrcFormat);
		convert.Init(pMgr, pRequest->m_pDestFormat);

		pSrcRow  = pRequest->m_pSrcData;
		pDestRow = pRequest->m_pDestData;

		for (y = pRequest->m_nHeight; y; y--)
		{
			uint32* pSrc  = (uint32*)pSrcRow;
			uint16* pDest = (uint16*)pDestRow;

			uint32  nWidth = pRequest->m_nWidth;

			if (pRequest->m_Flags & FMBLIT_OR)
			{
				for (x = nWidth; x; x--)
				{
					value   = read.Read32(pSrc);
					*pDest |= convert.Convert16(&value);

					pSrc++;
					pDest++;
				}
			}
			else
			{
				for (x = nWidth; x; x--)
				{
					value  = read.Read32(pSrc);
					*pDest = convert.Convert16(&value);

					pSrc++;
					pDest++;
				}
			}

			pSrcRow  += pRequest->m_nSrcPitch;
			pDestRow += pRequest->m_nDestPitch;
		}
	}

	return LT_OK;
}


// ======================================================================= //
//
//  G.  THE ADDRESS MAP, THE CALL GRAPH, AND WHAT IS NOT PINNED
//
//  ADDRESS -> DEFINITION (all 27; 0x0045d990 is lt2new3.cpp's and is
//  repeated above only so VC6 can inline it):
//
//    0x0045d530   FMFormatConvert::Init          0x0045e2f0   fm_Blit16To32
//    0x0045d580   FMFormatConvert::Convert8      0x0045e490   FMFormatRead::Read32
//    0x0045d5e0   fm_BlitPal8To16                0x0045e4f0   fm_Blit32To8
//    0x0045d760   FMFormatConvert::Convert16     0x0045e680   fm_Blit32To16
//    0x0045d7d0   fm_BlitPal8To32                0x0045e820   fm_Blit32To32
//   [0x0045d990   FMPaletteRead::ReadRGB]        0x0045e9f0   fm_BlitPal32To16
//    0x0045d9c0   FMFormatConvert::Convert32     0x0045eb60   FMPaletteRead::ReadARGB
//    0x0045da30   fm_Blit8To8                    0x0045eb90   fm_BlitPal32To32
//    0x0045dbb0   FMFormatRead::Init             0x0045ed50   fm_WritePixel8
//    0x0045dc00   FMFormatRead::Read8            0x0045edf0   fm_WritePixel16
//    0x0045dc90   fm_Blit8To16                   0x0045ee80   fm_WritePixel32
//    0x0045ddf0   fm_Blit8To32                   0x0045ef20   fm_ReadPixel8
//    0x0045df80   fm_Blit16To8                   0x0045efd0   fm_ReadPixel16
//    0x0045e0d0   FMFormatRead::Read16
//    0x0045e160   fm_Blit16To16
//
//  THE INTERNAL CALL GRAPH.  Fourteen of the twenty-seven are leaves; the
//  thirteen blitters are the only callers, and between them they reach every
//  leaf.  Callee, with its callers:
//
//    0x0045d530 Convert::Init  <- d5e0 d7d0 da30 dc90 ddf0 df80 e160 e2f0
//                                 e4f0(x2) e680(x2) e820 e9f0 eb90   [13 sites]
//    0x0045dbb0 Read::Init     <- da30 dc90 ddf0(x2) df80 e160 e2f0(x2)
//                                 e4f0 e680 e820                     [11 sites]
//    0x0045d580 Convert8       <- da30 df80 e4f0(x4)                 [6]
//    0x0045d760 Convert16      <- d5e0 dc90 e160 e680(x4) e9f0       [8]
//    0x0045d9c0 Convert32      <- d7d0(x2) ddf0(x2) e2f0(x2) e820 eb90 [8]
//    0x0045dc00 Read8          <- da30(x2) dc90(x2) ddf0(x4)         [8]
//    0x0045e0d0 Read16         <- df80(x2) e160(x2) e2f0(x4)         [8]
//    0x0045e490 Read32         <- e4f0(x2) e680(x2) e820             [5]
//    0x0045d990 ReadRGB        <- d7d0(x2)  (+ inlined in d5e0)
//    0x0045eb60 ReadARGB       <- e9f0 eb90 (+ inlined in each again)
//    0x0045f1e0 (outside)      <- d7d0 da30 ddf0 e160 e2f0 e4f0 e680 e820
//    0x0045d1e0 (outside)      <- da30 e160 e820
//
//  and NOTHING in the cluster calls the five single-pixel entries: they are
//  reached only through the tables at 0x004aceb8 / 0x004aced8.
//
//  WHAT IS NOT PINNED, in descending order of how many functions it costs:
//
//   1. THE WRITER'S OR ORDER.  See the file header.  Six functions
//      (Convert8/16/32 and the three fm_WritePixel*), plus the inlined copy
//      inside every blitter that writes.  Written in plane order; the
//      shipped emission is green, red, alpha, blue.
//
//   2. THE INLINE LOTTERY.  In all thirteen blitters VC6 CALLS the
//      destination writer in the "or" arm and EXPANDS it in the "assign"
//      arm -- e.g. 0x0045dd07 against 0x0045dd52.  Both arms are one
//      statement apart in the source and nothing in it can steer /Ob2.
//      This is the single largest risk to the thirteen and it is not
//      source-reachable.
//
//   3. THE SLOT ORDER OF THE TWO HELPERS.  0x0045dc90, 0x0045da30 and
//      0x0045e160 put the reader at the TOP of the frame and the converter
//      below it; 0x0045ddf0 and its three siblings put the converter on
//      top.  Written as nested scopes where the shipped code duplicates the
//      Init call per arm and as function scope where it does not, which is
//      the only lever that exists here.
//
//   4. `m_pFormat` FIRST OR LAST.  0x0045d530 stores it before the four
//      tables and 0x0045dbb0 after them; both are written that way, but
//      either could be the scheduler rather than the source.
//
//   5. `mask & data` OR `data & mask` in FMFormatRead::Read8/Read16 and the
//      two fm_ReadPixel entries.  0x0045dc20 `mov mask,%eax; and %edi,%eax`
//      puts the MASK in the accumulator, which by the `add` rule
//      makes it the left operand; written that way.  Four functions.
//
//   6. `pSrc`/`pDest` as inner cursors.  0x0045dce0 `mov %ebp,%edi` sits
//      ABOVE the flags branch, so the two cursor assignments are written
//      above the `if` and not duplicated into its arms.  Where the shipped
//      code has no such copy at all (0x0045d5e0, 0x0045e9f0) the row
//      pointer's own register served, which the same source produces.
//
//   7. 0x0045e820's guard.  It is dest-vs-manager, not src-vs-dest as in
//      the other two same-width blitters, and it still tails into
//      0x0045d1e0.  Written as read; it is not what the semantics would
//      suggest, which is why it is called out.
//
// ======================================================================= //


// 0x0045f1d0, 11 bytes — the pixel format's bytes-per-pixel SHIFT.
//
//      8b 41 04                  mov 0x4(%ecx),%eax
//      8b 04 85 78 cd 4a 00      mov 0x004acd78(,%eax,4),%eax
//      c3                        ret
//
// __thiscall, no arguments. The member at +0x04 indexes a .data table of
// dwords at 0x004acd78 whose first ten entries are 0,0,1,2,0,0,0,0,1,1 —
// shift counts, not sizes. Its one caller, 0x0045d1f1, uses the answer as
// one: `mov %eax,%ecx ; shl %cl,%edx` at 0x0045d1f8/0x0045d200. So the
// member is the PFormat type code and the table turns it into
// log2(bytes per pixel).
//
// The two eight-entry dword tables at 0x004acd78 and 0x004acd98 open this
// TU's .data run (shipped words; the pixel-function tables and g_FMBlitters
// follow at 0x004acdb8..0x004acef7).  The eighth bytes-per-pixel entry is 1
// in the shipped image, as written.
uint32 g_PFormatBPPShift[8]      = { 0, 0, 1, 2, 0, 0, 0, 0 };   // 0x004acd78
uint32 g_PFormatBytesPerPixel[8] = { 1, 1, 2, 4, 0, 0, 0, 1 };   // 0x004acd98

// FMFormat's own member: fm_BlitSame's call at 0x0045d1f1 lands here.
uint32 FMFormat::GetBPPShift()
{
	return g_PFormatBPPShift[m_nCode];
}


// ======================================================================= //
//
//  G.  THE REST OF THE SINGLE-PIXEL API, AND THE PFormat MASK HELPER
//
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x0045f080 (87 bytes)   `mov (%eax),%edx` in, `mov %eax,(%ecx)` out
//
//  The 32-bit reader, sixth slot of the 0x004aced8 table.  No reader struct
//  and no tables: only argument two (the format, in %ebx) is read, at
//  +0x28..+0x34 -- m_nShifts -- and each term is `shr %cl` then
//  `and $0xff`, packed through the same `shl $0x8` chain as Read32.
//
//  Shipped keeps the `and $0xff` on the ALPHA term (0x0045f0a0): the four
//  terms go through a `uint32 planes[4]` array (see Read32), which is what
//  stops VC6 folding `(x & 0xff) << 24` to `x << 24`.  This is Read32 built
//  on the stack and expanded, the same shape as fm_ReadPixel8/16.
// ----------------------------------------------------------------------- //
void fm_ReadPixel32(FMConvertMgr* pMgr, FMFormat* pFormat, void* pSrc, void* pValue)
{
	FMFormatRead read;

	read.InitInline(pMgr, pFormat);

	*((PValue*)pValue) = read.Read32Inline((uint32*)pSrc);
}


// ----------------------------------------------------------------------- //
//  0x0045f160 (70 bytes)   PFormat's mask scanner, called only from the
//  per-plane helper 0x0045f120 (0x0045f139).
//
//  __cdecl (mask, pEnd, pFirst).  First loop counts the mask's trailing
//  zeros into *pFirst (0x0045f16a zeroes it, 0x0045f184 stores it every
//  pass -- a pointer target, not a register local); 0x0045f18e copies it to
//  *pEnd and the second loop, continuing with the SAME walking bit in %eax,
//  counts the set bits on top.  Each loop has its own 0..32 counter
//  (%edx, %ecx).  0x0045f146 turns the pair into m_nBits = end - first.
// ----------------------------------------------------------------------- //
void fm_GetMaskInfo(uint32 nMask, uint32* pEnd, uint32* pFirst)
{
	uint32 i, nBit;

	*pFirst = 0;
	nBit = 1;
	for (i = 0; i < 32; i++)
	{
		if (nMask & nBit)
			break;

		nBit <<= 1;
		(*pFirst)++;
	}

	*pEnd = *pFirst;
	for (i = 0; i < 32; i++)
	{
		if (!(nMask & nBit))
			break;

		nBit <<= 1;
		(*pEnd)++;
	}
}


// ----------------------------------------------------------------------- //
//  0x0045f620 (50 bytes)   FMConvertMgr::FormatColorToPValue
//
//  The twin of 0x0045f5e0 over the READER table 0x004aced8: index by the
//  format's +0x04 code, 1 if the slot is empty, otherwise call
//  slot(this, pFormat, &nColor, pOut) -- 0x0045f638 takes the address of
//  its own argument slot, so the colour is passed BY VALUE -- and return 0.
// ----------------------------------------------------------------------- //
typedef void (*FMPixelFn)(FMConvertMgr* pMgr, FMFormat* pFormat, void* pIn, void* pOut);

// The writer and reader tables, eight slots each, indexed by the format
// code (shipped words at 0x004aceb8 and 0x004aced8: codes 1..3 are the 8-,
// 16- and 32-bit functions of section F/G, the rest empty).
FMPixelFn g_FMWritePixelFns[8] =             // 0x004aceb8
{
	LTNULL, fm_WritePixel8, fm_WritePixel16, fm_WritePixel32,
	LTNULL, LTNULL, LTNULL, LTNULL
};

FMPixelFn g_FMReadPixelFns[8] =              // 0x004aced8
{
	LTNULL, fm_ReadPixel8, fm_ReadPixel16, fm_ReadPixel32,
	LTNULL, LTNULL, LTNULL, LTNULL
};

class FMConvertMgrAPI : public FMConvertMgr
{
public:
	LTRESULT PValueToFormatColor(FMFormat* pFormat, PValue nValue, uint32* pOut);  // 0x0045f5e0
	LTRESULT FormatColorToPValue(FMFormat* pFormat, uint32 nColor, PValue* pOut);  // 0x0045f620
};

// 0x0045f5e0 (50 bytes).  Not on this file's list but written here because
// it is the relocation-masked twin of 0x0045f620 -- without it the matcher
// can pair 0x0045f620's body with this address.  Writer table 0x004aceb8.
LTRESULT FMConvertMgrAPI::PValueToFormatColor(FMFormat* pFormat, PValue nValue, uint32* pOut)
{
	FMPixelFn fn = g_FMWritePixelFns[pFormat->m_nCode];

	if (fn)
	{
		fn(this, pFormat, &nValue, pOut);
		return LT_OK;
	}

	return LT_ERROR;
}

LTRESULT FMConvertMgrAPI::FormatColorToPValue(FMFormat* pFormat, uint32 nColor, PValue* pOut)
{
	FMPixelFn fn = g_FMReadPixelFns[pFormat->m_nCode];

	if (fn)
	{
		fn(this, pFormat, &nColor, pOut);
		return LT_OK;
	}

	return LT_ERROR;
}


// ----------------------------------------------------------------------- //
//  0x0045f660 (59 bytes)   the byte size of a w x h image of one format.
//
//  ALSO WRITTEN (as fm_GetImageDataSizeQ468) in src/engine/server/
//  smgrmisc.cpp, which names this TU as its home; that copy is a plain
//  4-first if-chain and comes out a size-exact near (it emits `jne` over
//  an inline 4 arm).  Shipped tests 4, 5, 6 in that order
//  (0x0045f664..0x0045f671) but lays the 5/6 body out FIRST (0x0045f673)
//  and the 4 body after it (0x0045f67d).  Measured spellings:
//    * switch (with or without default, any case order) -- VC6 builds a
//      `je 4 / jbe / ja 6` tree, not this chain;
//    * 4-first if-chain, nested `if (!= 4)`, `4||5||6` then inner test,
//      ternary, `if 4` then a 5/6 switch -- all 11-12 differing insns;
//    * the guarded 5/6 test below -- byte-identical.  The redundant
//      `!= 4` is what makes VC6 test 4 first and jump forward past 5/6.
//  The smgrmisc.cpp copy should be deleted in favour of this one.
//  The default indexes 0x004acd98, the table 0x0045f230 also reads.
// ----------------------------------------------------------------------- //
// g_PFormatBytesPerPixel (0x004acd98) is defined with the shift table above.

uint32 fm_GetImageDataSize(uint32 nFormatCode, uint32 nWidth, uint32 nHeight)
{
	if (nFormatCode != 4 && (nFormatCode == 5 || nFormatCode == 6))
		return nWidth * nHeight;
	if (nFormatCode == 4)
		return (nWidth * nHeight) / 2;
	return g_PFormatBytesPerPixel[nFormatCode] * nWidth * nHeight;
}


// ----------------------------------------------------------------------- //
//  0x0045f230 -- 11 bytes, thiscall, no arguments.  PFormat's bytes per
//  pixel, straight out of the code-indexed table:
//      mov 0x4(%ecx),%eax ; mov 0x4acd98(,%eax,4),%eax ; ret
//  Too small to be placed on its own (seven unmasked bytes); its callers
//  are clientrender.cpp's r_PublishRenderStruct (0x0040eeb0) and
//  smgrmisc.cpp's fill (0x0045f47d).
// ----------------------------------------------------------------------- //
uint32 FMFormat::GetBytesPerPixel()
{
	return g_PFormatBytesPerPixel[m_nCode];
}

//  0x004acdb8 (0x100 bytes)   the 8x8 BLITTER TABLE lttexmod.cpp's
//  FMConvertMgr::Convert (0x0045f3d0) indexes by `src code * 8 + dest
//  code`.  Defined here with the blitters it holds, between this file's
//  bytes-per-pixel table (0x004acd98) and its two single-pixel tables
//  (0x004aceb8 / 0x004aced8).  The 64 words are the shipped .data, in order;
//  the two DXT entries are the forwarders partfm.cpp (0x0045ed10) and
//  fmcvt32.cpp (0x0045ed30) write over their own view classes, so they go in
//  through a cast.
// ----------------------------------------------------------------------- //
typedef LTRESULT (*FMBlitFn)(FMConvertMgr* pMgr, FMConvertRequest* pRequest);

class FMConvertMgrQ45C;
class FMConvertRequestQ45C;
class FMConvertMgrQ460;
class FMConvertRequestQ460;
LTRESULT fm_BlitDXTTo16EntryQ45C(FMConvertMgrQ45C* pMgr, FMConvertRequestQ45C* pRequest);  // 0x0045ed10
LTRESULT fm_BlitDXTTo32EntryQ460(FMConvertMgrQ460* pMgr, FMConvertRequestQ460* pRequest);  // 0x0045ed30

#define FM_DXT16 ((FMBlitFn)fm_BlitDXTTo16EntryQ45C)
#define FM_DXT32 ((FMBlitFn)fm_BlitDXTTo32EntryQ460)

FMBlitFn g_FMBlitters[8 * 8] =
{
	fm_BlitSame, fm_BlitPal8To8,  fm_BlitPal8To16, fm_BlitPal8To32,  0, 0, 0, 0,   // 0x004acdb8
	0,           fm_Blit8To8,     fm_Blit8To16,    fm_Blit8To32,     0, 0, 0, 0,   // 0x004acdd8
	0,           fm_Blit16To8,    fm_Blit16To16,   fm_Blit16To32,    0, 0, 0, 0,   // 0x004acdf8
	0,           fm_Blit32To8,    fm_Blit32To16,   fm_Blit32To32,    0, 0, 0, 0,   // 0x004ace18
	0,           0,               FM_DXT16,        FM_DXT32,         fm_BlitSame, 0, 0, 0,   // 0x004ace38
	0,           0,               FM_DXT16,        FM_DXT32,         0, fm_BlitSame, 0, 0,   // 0x004ace58
	0,           0,               FM_DXT16,        FM_DXT32,         0, 0, fm_BlitSame, 0,   // 0x004ace78
	0,           0,               fm_BlitPal32To16, fm_BlitPal32To32, 0, 0, 0, fm_BlitSame,  // 0x004ace98
};
