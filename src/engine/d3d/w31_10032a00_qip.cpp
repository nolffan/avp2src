// *********************************************************************** //
//
//  MODULE:   W31_10032A00_QIP (.CPP)  --  three d3d.ren device-setup
//            functions: 0x10032a00 (is a console variable set to a value),
//            0x10032a50 (chipset workarounds) and 0x10032c30 (release the
//            Direct3D objects).   /O2 /Qip.
//
//  Was w5_10032a50.cpp (/O2) plus d3dbatchc.cpp's 0x10032a00 (/Os).  In the
//  shipped image 0x10032a00 is a /Qip two-entry pair -- the 8-byte stack
//  entry `mov 0x4(%esp),%eax ; mov 0x8(%esp),%edx` falling into the register
//  entry 0x10032a08 -- and all three of its callers are 0x10032a50's, which
//  enter at 0x10032a08 with the name in %eax and the value in %edx.  icl only
//  calls a register entry from inside the callee's TU, so the pair and its
//  caller share one here.  Its body is /O2 (`mov $1,%eax`, argument stores,
//  not pushes).  Measured: 0x10032a00 and 0x10032a08
//  identical (were one 64-byte /Os near, 52 bytes differing), 0x10032c30
//  still identical, 0x10032a50 328 -> 316 bytes differing.  /Qip and
//  /Qip_no_inlining measure the same; plain /Qip is used.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <stdlib.h>
#include <string.h>


// ---------------------------------------------------------------------- //
//  The engine's RenderStruct at 0x1008cbcc, the slots used here.  +0x14 is
//  the console print: one pushed string, the CALLER pops it (`add $0x8,
//  %esp` after an 8-byte outgoing area), so it is __cdecl.
// ---------------------------------------------------------------------- //
struct RenderStructCallbacks
{
	uint8    m_Reserved00[0x14];                        // +0x00..+0x13
	void    (*ConsolePrint)(const char* pszMsg, ...);   // +0x14
	void*   (*GetConsoleVariable)(const char* pszName); // +0x18
	double  (*GetVarValueFloat)(void* pVar);            // +0x1c
	char*   (*GetVarValueString)(void* pVar);           // +0x20
	uint8    m_Reserved24[0x10];                        // +0x24..+0x33
	void*   (*dalloc)(uint32 nSize);                    // +0x34
	void    (*dfree)(void* pMem);                       // +0x38
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// DDDEVICEIDENTIFIER2: two 512-byte strings, a LARGE_INTEGER (so the struct
// is 8-aligned and 0x430 bytes -- the `rep stos` clears 0x10c dwords), then
// the vendor and device ids at +0x408/+0x40c.
struct DDDevId10032a50
{
	char   szDriver[512];                               // +0x000
	char   szDescription[512];                          // +0x200
	double liDriverVersion;                             // +0x400
	uint32 dwVendorId;                                  // +0x408
	uint32 dwDeviceId;                                  // +0x40c
	uint32 dwSubSysId;                                  // +0x410
	uint32 dwRevision;                                  // +0x414
	uint8  guidDeviceIdentifier[16];                    // +0x418
	uint32 dwWHQLLevel;                                 // +0x428
};

struct DDraw7;
struct DDraw7Vtbl
{
	void* m_pad_00[27];                                 // +0x00..+0x68
	long (__stdcall* GetDeviceIdentifier)(DDraw7*, DDDevId10032a50* pId,
	                                      uint32 dwFlags);           // +0x6c
};
struct DDraw7 { DDraw7Vtbl* lpVtbl; };

extern DDraw7* g_pDirectDraw;                           // 0x1008bf74

// 0x10032a00 -- "is console variable <name> set to <value>", case-
// insensitive (decoded in d3dbatchc.cpp, where it was first written).  The
// two false answers share the fall-through and the true answer is out of
// line: a guarded `return LTFALSE`, a `return LTTRUE` inside the second
// test, then a trailing `return LTFALSE`.
LTBOOL sub_10032a00(const char* pszVarName, const char* pszValue)
{
	void* pVar;

	pVar = g_pStruct->GetConsoleVariable(pszVarName);
	if (pVar == LTNULL)
		return LTFALSE;

	if (_stricmp(g_pStruct->GetVarValueString(pVar), pszValue) == 0)
		return LTTRUE;

	return LTFALSE;
}

extern uint32 g_1008d950;               // 0x1008d950  cleared for a Voodoo3
extern uint32 g_1008d95c;               // 0x1008d95c  PowerVR
extern uint32 g_1008d960;               // 0x1008d960  shadows/fullbrites enabled
extern uint32 g_1008d974;               // 0x1008d974  Rage128
extern uint32 g_1008d978;               // 0x1008d978  lightmapping enabled


// ======================================================================= //
//  0x10032a50 -- 432 bytes, one caller (big331e0.cpp's bring-up, 0x10033a54).
//  CHIPSET WORKAROUNDS: a "ForceMode" console variable naming PowerVR,
//  Permedia2 or Rage128 overrides detection; otherwise the DirectDraw
//  device identifier's PCI vendor/device ids pick the chip.
//
//  WHAT PINS THE SHAPE:
//   * both g_pDirectDraw and g_pStruct are null-tested first, each to the
//     same bare return;
//   * 0x1008d960 = 1, 0x1008d95c = 0 BEFORE the identifier is fetched, and
//     a failed GetDeviceIdentifier (+0x6c, flags 0) returns with them set;
//   * three bool flags: the PowerVR one is never materialised (a match
//     jumps straight to the PowerVR block, 0x10032aeb), the Permedia one
//     lives in %bh and the Rage one in a frame byte (-0x8).  That is jump
//     threading through `if (!bPowerVR && !bPermedia && !bRage)` into the
//     final if/else chain, whose three blocks are 0x10032b5b (PowerVR:
//     message, 0x1008d95c = 1, 0x1008d960 = 0), 0x10032bbf (Permedia:
//     message, 0x1008d960 = 0x1008d978 = 0) and 0x10032bb3 (Rage128:
//     0x1008d974 = 1);
//   * bPowerVR is an LTBOOL and the other two bools: icl
//     threads the int flag's constant stores straight into the final
//     chain (no flag materialised, the `jne 0x10032b5b` from the first
//     ForceMode test) and keeps the two byte flags (%bh, -0x8), as
//     shipped; all three bool keeps all three (512 bytes), all three
//     LTBOOL threads all three (416).  480 bytes now, a near: what is
//     left is the detection arms -- the shipped vendor tests jump straight
//     to the Permedia / PowerVR / Rage blocks, ours set the byte flags.
//   * detection: vendor 0x104c (TI -> Permedia 2), vendor 0x1033 with
//     device 0x46 or 0x2a (NEC -> PowerVR), vendor 0x1002 device 0x5246
//     (ATI Rage 128), and vendor 0x121a device 5 (3dfx Voodoo3), which only
//     clears 0x1008d950.
// ======================================================================= //
void sub_10032a50()
{
	const char*      pszForce = "ForceMode";
	DDDevId10032a50  devId;
	LTBOOL           bPowerVR;
	bool             bPermedia, bRage128;

	if (!g_pDirectDraw || !g_pStruct)
		return;

	g_1008d960 = 1;
	g_1008d95c = 0;

	memset(&devId, 0, sizeof(devId));
	if (g_pDirectDraw->lpVtbl->GetDeviceIdentifier(g_pDirectDraw, &devId, 0) != 0)
		return;

	bPowerVR  = false;
	bPermedia = false;
	bRage128  = false;

	if (sub_10032a00(pszForce, "PowerVR"))
		bPowerVR = true;
	else if (sub_10032a00(pszForce, "Permedia2"))
		bPermedia = true;
	else if (sub_10032a00(pszForce, "Rage128"))
		bRage128 = true;

	if (!bPowerVR && !bPermedia && !bRage128)
	{
		if (devId.dwVendorId == 0x104c)
			bPermedia = true;
		else if (devId.dwVendorId == 0x1033 &&
		         (devId.dwDeviceId == 0x46 || devId.dwDeviceId == 0x2a))
			bPowerVR = true;
		else if (devId.dwVendorId == 0x1002 && devId.dwDeviceId == 0x5246)
			bRage128 = true;
		else if (devId.dwVendorId == 0x121a && devId.dwDeviceId == 5)
			g_1008d950 = 0;
	}

	if (bPowerVR)
	{
		g_pStruct->ConsolePrint("POWERVR DETECTED: disabling shadows and fullbrites");
		g_1008d95c = 1;
		g_1008d960 = 0;
	}
	else if (bPermedia)
	{
		g_pStruct->ConsolePrint("PERMEDIA 2 DETECTED: disabling lightmapping and shadows");
		g_1008d960 = 0;
		g_1008d978 = 0;
	}
	else if (bRage128)
	{
		g_1008d974 = 1;
	}
}


// ======================================================================= //
//  0x10032c30 -- 144 bytes, one caller (d3dbatcha.cpp's DirectDraw teardown
//  0x100210d0, at 0x100210de).  Releases the Direct3D-side objects after
//  0x100395f0, each as `if (p) { p->Release(); p = NULL; }` with the global
//  re-read for the call (`mov g,%eax; test; je; mov g,%eax; mov (%eax),
//  %edx; push %eax; call *8(%edx)`) -- d3dbatcha.cpp's volatile-read idiom
//  for the same reload.  Order: the clipper (0x1008d97c), the device
//  (0x1008d988), the z-buffer surface (0x1008bf64), IDirect3D7 (0x1008d980).
// ======================================================================= //

// Every one of the four is released through its vtable's +0x08 (IUnknown::
// Release), which is all that is needed of them here; the tags are the ones
// the rest of the module spells these globals with.
struct UnkVtbl10032c30
{
	long   (__stdcall* QueryInterface)(void*, const void*, void**);  // +0x00
	uint32 (__stdcall* AddRef)(void*);                                // +0x04
	uint32 (__stdcall* Release)(void*);                               // +0x08
};
struct DDClipper  { UnkVtbl10032c30* lpVtbl; };
struct D3DDevice7 { UnkVtbl10032c30* lpVtbl; };
struct DDSurface  { UnkVtbl10032c30* lpVtbl; };
struct D3D7       { UnkVtbl10032c30* lpVtbl; };

extern DDClipper*  g_pClipper;          // 0x1008d97c
extern D3DDevice7* g_pD3DDevice;        // 0x1008d988
extern DDSurface*  g_pZBuffer;          // 0x1008bf64
extern D3D7*       g_pD3D;              // 0x1008d980

// 0x100395f0 -- d3dr5d.cpp, no arguments.
extern void sub_100395f0();

// Read the global once, through a volatile, for both the vtable and the
// argument: the shipped code tests it and then loads it again.
#define SAFE_RELEASE_10032C30(T, g)                                     \
	if (g)                                                              \
	{                                                                   \
		{ T* p = *(T* volatile*)&(g); p->lpVtbl->Release(p); }          \
		g = LTNULL;                                                     \
	}

void sub_10032c30()
{
	sub_100395f0();

	SAFE_RELEASE_10032C30(DDClipper,  g_pClipper);
	SAFE_RELEASE_10032C30(D3DDevice7, g_pD3DDevice);
	SAFE_RELEASE_10032C30(DDSurface,  g_pZBuffer);
	SAFE_RELEASE_10032C30(D3D7,       g_pD3D);
}
