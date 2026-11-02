// *********************************************************************** //
//
//  MODULE:   W26_10031E10 (.CPP)  --  the LISTDEVICECAPS console command
//
//  0x10031e10..0x100322b7 of the retail d3d.ren (1192 bytes), written for the
//  d3d-link: RenderCommand (0x10032d00, w32_10032d00_qip.cpp) runs it for
//  "LISTDEVICECAPS".  /O2: lea-nop padding (0x100322b8), the /O2 frame
//  (`mov %edi,0x8(%esp) / %esi,0x4(%esp) / %ebx,(%esp)`) and the device
//  identifier cleared inline (`rep stos`, 0x10c dwords).  No register
//  calls.
//
//  WHAT PINS IT:
//    * the DirectDraw device identifier: IDirectDraw7 +0x6c
//      GetDeviceIdentifier(&id, 0) into a 0x430-byte local
//      (DDDEVICEIDENTIFIER2: two 512-byte strings, the 8-byte driver
//      version -- which 8-aligns the struct to 0x430 -- vendor, device,
//      subsystem, revision at +0x408..+0x414).  Only when that succeeds:
//      a rule, "Driver", "Description", the version split into words
//      (product = HIWORD(high), version = LOWORD(high), subversion =
//      HIWORD(low), build = LOWORD(low)), the PCI IDs, the mode (width
//      0x1008cbb4, height 0x1008cbb0, 16 bits when the back buffer's
//      PFormat type at 0x1008bf2c is 2, else 32), a rule.
//    * then one line per capability, each "%s" a "Yes"/"No" on the
//      capability globals the device setup filled (their names and
//      meanings are the other TUs'; the format strings name them too).
//    * everything goes out through the RenderStruct's ConsolePrintf
//      (+0x14).  "Portals" is an unsigned `> 0` (the `jbe`) on 0x1008d7e4.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <string.h>


struct RenderStruct
{
	uint8   m_Reserved00[0x14];                         // +0x00..+0x13
	void    (*ConsolePrintf)(const char* pszFmt, ...);  // +0x14
};
extern RenderStruct* g_pStruct;                         // 0x1008cbcc

struct DDDeviceIdentifier26
{
	char    szDriver[512];                              // +0x000
	char    szDescription[512];                         // +0x200
	union
	{
		struct
		{
			uint32  dwLow;                              // +0x400
			uint32  dwHigh;                             // +0x404
		} parts;
		double  fAlign;                                 // (8-byte alignment)
	} liDriverVersion;
	uint32  dwVendorId;                                 // +0x408
	uint32  dwDeviceId;                                 // +0x40c
	uint32  dwSubSysId;                                 // +0x410
	uint32  dwRevision;                                 // +0x414
	uint8   guidDeviceIdentifier[16];                   // +0x418
	uint32  dwWHQLLevel;                                // +0x428
};                                                      // 0x430

struct DDraw
{
	virtual long __stdcall Slot00() = 0;  virtual long __stdcall Slot04() = 0;
	virtual long __stdcall Slot08() = 0;  virtual long __stdcall Slot0c() = 0;
	virtual long __stdcall Slot10() = 0;  virtual long __stdcall Slot14() = 0;
	virtual long __stdcall Slot18() = 0;  virtual long __stdcall Slot1c() = 0;
	virtual long __stdcall Slot20() = 0;  virtual long __stdcall Slot24() = 0;
	virtual long __stdcall Slot28() = 0;  virtual long __stdcall Slot2c() = 0;
	virtual long __stdcall Slot30() = 0;  virtual long __stdcall Slot34() = 0;
	virtual long __stdcall Slot38() = 0;  virtual long __stdcall Slot3c() = 0;
	virtual long __stdcall Slot40() = 0;  virtual long __stdcall Slot44() = 0;
	virtual long __stdcall Slot48() = 0;  virtual long __stdcall Slot4c() = 0;
	virtual long __stdcall Slot50() = 0;  virtual long __stdcall Slot54() = 0;
	virtual long __stdcall Slot58() = 0;  virtual long __stdcall Slot5c() = 0;
	virtual long __stdcall Slot60() = 0;  virtual long __stdcall Slot64() = 0;
	virtual long __stdcall Slot68() = 0;
	virtual long __stdcall GetDeviceIdentifier(DDDeviceIdentifier26* pId,
	                                           uint32 dwFlags) = 0;            // +0x6c
};

extern DDraw*   g_pDirectDraw;                          // 0x1008bf74
extern uint32   g_nWindowWidth;                         // 0x1008cbb4
extern uint32   g_nWindowHeight;                        // 0x1008cbb0

// NEW spelling: the back buffer's PFormat (0x1008bf28) type word, +0x04.
extern uint32   g_nScreenFormatType1008bf2c;            // 0x1008bf2c

extern uint32   g_1008d7e4;                             // 0x1008d7e4 dwStencilBitDepth
extern uint32   g_1008d964;                             // 0x1008d964
extern uint32   g_1008d970;                             // 0x1008d970
extern uint32   g_1008d978;                             // 0x1008d978 lightmapping enabled
extern uint32   g_1008d968;                             // 0x1008d968
extern uint32   g_1008d808;                             // 0x1008d808 dwDevCaps & 0x400
extern uint32   g_1008d804;                             // 0x1008d804 dpcTriCaps.dwRasterCaps & DITHER
extern uint32   g_Caps1008d7f4;                         // 0x1008d7f4
extern uint32   g_1008d7f8;                             // 0x1008d7f8
extern uint32   g_1008d7fc;                             // 0x1008d7fc
extern uint32   g_1008d800;                             // 0x1008d800
extern uint32   g_1008d93c;                             // 0x1008d93c
extern uint32   g_nCaps_1008d938;                       // 0x1008d938
extern uint32   g_1008d80c;                             // 0x1008d80c
extern uint32   g_1008d810;                             // 0x1008d810
extern uint32   g_1008d814;                             // 0x1008d814
extern uint32   g_1008d820;                             // 0x1008d820 texture memory total
extern uint32   g_1008d824;                             // 0x1008d824 video memory total
extern int32    g_1008d818;                             // 0x1008d818 z-buffer bit depth
extern int32    g_1008d81c;                             // 0x1008d81c render bit depth
extern uint32   g_1008d920;                             // 0x1008d920 raster caps
extern uint32   g_n1008d828;                            // 0x1008d828

// NEW (no TU declared them before): the device's maximum texture width
// and height.
extern uint32   g_1008d8b0;                             // 0x1008d8b0
extern uint32   g_1008d8b4;                             // 0x1008d8b4

#define YESNO26(x)      ((x) ? "Yes" : "No")


// ======================================================================= //
//  0x10031e10  1192 bytes.
// ======================================================================= //
//  NEAR: 12 bytes.  What is left is three `movl fmt,(%esp)`
//  that the shipped code stores BEFORE loading the first integer argument
//  ("Texture Memory", "ZBuffer Depth", "Max texture size" -- the only
//  printfs that reuse the previous call's argument block); ours loads
//  first.  The "Width/Height" and "VendorID" printfs load first in both.
//  Measured without effect: the four globals as one struct, as long /
//  unsigned long / char* / volatile, read through a pointer local or an
//  inline accessor, ConsolePrintf returning int or taking char*, the
//  externs moved, the file renamed (any length), this function appended
//  to w15_100316e0.cpp (its shipped neighbour), and /Ox /O3 /G6 /Ob0 /Ob2
//  /Qip /Qrcd (/Os /O1 /Oi- /Op /G5 /GB are far worse).
void sub_10031e10()
{
	DDDeviceIdentifier26 id;

	if (g_pDirectDraw)
	{
		memset(&id, 0, sizeof(id));
		if (g_pDirectDraw->GetDeviceIdentifier(&id, 0) == 0)
		{
			g_pStruct->ConsolePrintf("---------------------------------------------------------------");
			g_pStruct->ConsolePrintf("Driver: %s", id.szDriver);
			g_pStruct->ConsolePrintf("Description: %s", id.szDescription);
			g_pStruct->ConsolePrintf("Product: 0x%x, Version: 0x%x, SubVersion: 0x%x, Build: 0x%x",
				id.liDriverVersion.parts.dwHigh >> 16, id.liDriverVersion.parts.dwHigh & 0xffff,
				id.liDriverVersion.parts.dwLow >> 16, id.liDriverVersion.parts.dwLow & 0xffff);
			g_pStruct->ConsolePrintf("VendorID: 0x%x, DeviceID: 0x%x, SubSysID: 0x%x, Revision: 0x%x",
				id.dwVendorId, id.dwDeviceId, id.dwSubSysId, id.dwRevision);
			// The 16-bit arm is out of line at the very end of the function
			// (0x100322ae: `mov $0x10,%eax ; jmp` back), the 32 falls
			// through.  `x == 2 ? 16 : 32` lays 16 inline after a `jne`;
			// `x != 2 ? 32 : 16`, `nBPP = 32; if (x == 2) nBPP = 16;` and a
			// switch on the type with `case 2` all keep it in line; the
			// switch-default sink (the default arm goes to the end) on the
			// comparison does it (522 -> 12 bytes off).
			uint32 nBPP;
			switch (g_nScreenFormatType1008bf2c == 2)
			{
				case 0:  nBPP = 32; break;
				default: nBPP = 16; break;
			}
			g_pStruct->ConsolePrintf("Width: %d, Height: %d, BitDepth: %d",
				g_nWindowWidth, g_nWindowHeight, nBPP);
			g_pStruct->ConsolePrintf("---------------------------------------------------------------");
		}
	}

	g_pStruct->ConsolePrintf("Portals: %s", YESNO26(g_1008d7e4 > 0));
	g_pStruct->ConsolePrintf("Model fullbrites: %s, Gouraud fullbrites: %s",
		YESNO26(g_1008d964), YESNO26(g_1008d970));
	g_pStruct->ConsolePrintf("Lightmap capable: %s, Light add poly: %s",
		YESNO26(g_1008d978), YESNO26(g_1008d968));
	g_pStruct->ConsolePrintf("DrawPrim: %s, Dither: %s",
		YESNO26(g_1008d808), YESNO26(g_1008d804));
	g_pStruct->ConsolePrintf("Src blend SRCCOLOR: %s, Dest blend SRCALPHA: %s",
		YESNO26(g_Caps1008d7f4), YESNO26(g_1008d7f8));
	g_pStruct->ConsolePrintf("TBlend Add: %s, TBlend Modulate (gouraud): %s",
		YESNO26(g_1008d7fc), YESNO26(g_1008d800));
	g_pStruct->ConsolePrintf("LINEARMIPNEAREST supported: %s",
		YESNO26(g_1008d93c & 0x10));
	g_pStruct->ConsolePrintf("Square textures only: %s, Vid mem textures: %s",
		YESNO26(g_nCaps_1008d938 & 0x20), YESNO26(g_1008d80c));
	g_pStruct->ConsolePrintf("System mem textures: %s, AGP mem textures: %s",
		YESNO26(g_1008d810), YESNO26(g_1008d814));
	g_pStruct->ConsolePrintf("Texture Memory: %d, Video Memory: %d",
		g_1008d820, g_1008d824);
	g_pStruct->ConsolePrintf("ZBuffer Depth: %d, Device Depth: %d",
		g_1008d818, g_1008d81c);
	g_pStruct->ConsolePrintf("Z-test: %s, Table fog: %s, Palette alpha: %s",
		YESNO26(g_1008d920 & 0x10), YESNO26(g_1008d920 & 0x100),
		YESNO26(g_nCaps_1008d938 & 0x80));
	g_pStruct->ConsolePrintf("Max texture size: (%d x %d)",
		g_1008d8b0, g_1008d8b4);
	g_pStruct->ConsolePrintf("Surfaces larger than screen: %s",
		YESNO26(g_n1008d828));
}
