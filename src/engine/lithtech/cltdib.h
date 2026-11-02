// *********************************************************************** //
//
//  CLTDIB.H -- the engine's 8-bit bitmap, the object 0x0043de60 constructs
//
//  Shared by the two TUs that build one on their stack: cltcursor.cpp (the
//  constructor, the PCX loader 0x0043df40, ci_CreateSurfaceFromBitmap and
//  the console backdrop 0x0041cd50) and clientconsole.cpp (the console font,
//  0x0041c5a0).  Each caller expands the implicit destructor (~CMoArray ->
//  Term) itself, so the whole class has to be visible to both.
//
// *********************************************************************** //

#ifndef __CLTDIB_H__
#define __CLTDIB_H__

#include "collection.h"

// The 0x4a1ab4 vtable is PFormat's (lttexmod.cpp: one virtual, 0x38
// bytes, no declared constructor) and cissurf.cpp shows a PFormat at +0 with a 256-entry palette at +0x38.  The second vptr at +0x444 is
// collection.h's CMoArray<uint8>: +0x448 m_pArray, +0x44c m_nElements,
// +0x450 m_CacheSize, +0x454 m_WantedCache -- the BaseDelete count at
// 0x0043de98 is m_nElements + m_CacheSize, and 0x0043df40 reads a PCX
// header into it (byte 3 == 8 is the PCX bits-per-pixel), so this is the
// engine's 8-bit bitmap loader's image.  Class names ours.
//
// The format member IS lttexmod.cpp's PFormat, so it carries that name and
// its calls (Init 0x0045f0e0, InitPValue 0x0045f1b0, GetBytesPerPixel
// 0x0045f230) link to lttexmod.cpp's definitions.
class PFormat
{
public:
	virtual void Init(uint32 nBitDepth, uint32 aMask, uint32 rMask,
	                  uint32 gMask, uint32 bMask);          // +0x00 0x0045f0e0

public:
	uint32 m_nBitDepth;                      // 0x04
	uint32 m_Masks[4];                       // 0x08
	uint32 m_nBits[4];                       // 0x18
	uint32 m_nShifts[4];                     // 0x28

	// Both called out of line by dib_LoadQ41C.
	void   InitPValue();                                    // 0x0045f1b0
	uint32 GetBytesPerPixel();                              // 0x0045f230
};

// One palette entry: a pad byte, then red, green, blue (0x0043e114..).
struct CDibColorQ41C
{
	uint8 m_Pad;                             // 0x00
	uint8 m_R;                               // 0x01
	uint8 m_G;                               // 0x02
	uint8 m_B;                               // 0x03
};

class CObj4a1ab4View
{
public:
	CObj4a1ab4View();                        // 0x0043de60

	void   ResetCounters();                  // 0x0043df20 (name ours)

public:
	// A MEMBER, not a base: a base with virtuals would give this class a
	// vtable of its own and a second vptr store after the members'.  The
	// constructor stores 0x004a1ab4 exactly once.
	PFormat     m_Format;                    // +0x000
	CDibColorQ41C m_Palette[256];            // +0x038
	uint32 m_Unknown438;                     // +0x438
	uint32 m_Unknown43c;                     // +0x43c
	uint32 m_Unknown440;                     // +0x440
	CMoArray<uint8> m_Data;                  // +0x444
};

class ILTStream;

// 0x0043df40 -- read a PCX image from a stream into the bitmap
// (cltcursor.cpp).
LTBOOL dib_LoadQ41C(ILTStream* pStream, CObj4a1ab4View* pDib);

#endif  // __CLTDIB_H__
