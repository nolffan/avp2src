// *********************************************************************** //
//
//  w8_10069ca0_osoiqip.cpp -- d3d.ren's two DirectDraw device-enumeration
//  callbacks, 0x10069ca0 and 0x10069e10.  /Os /Oi- /Qip (see build/engine.sh).
//
//  WHY /Qip, AND WHY ONE FILE.  0x10069ca0 is the two-entry shape icl's
//  register calling convention under /Qip gives for an EXTERNAL function under /Qip: a 12-byte stack entry
//  (`mov 0x4/0x8/0xc(%esp)` into %eax/%edx/%ecx) falling into the register
//  entry at 0x10069cac.  0x10069e10 calls that register entry directly
//  (`call 0x10069cac` with %eax/%edx/%ecx set and the fourth argument in
//  0xc(%esp)), which only a caller in the same /Qip translation unit does.
//
//  THE SIGNATURES are DirectDraw's.  0x10069ca0 ends `ret $0x10`: four
//  stdcall dwords, LPDDENUMCALLBACKA (GUID*, description, driver name,
//  context).  0x10069e10 ends `ret $0x14`: five, LPDDENUMCALLBACKEXA -- the
//  same four plus the HMONITOR, which it drops.  d3dr3b.cpp's
//  sub_10069e40 hands 0x10069e10 to DirectDrawEnumerateExA and 0x10069ca0
//  to DirectDrawEnumerateA; its declarations were one argument short each
//  (@16/@12) and are corrected to @20/@16 with this file.
//
//  0x1007fa80 is the jump thunk to DDRAW.dll!DirectDrawCreate (guid,
//  &pDD, NULL); 0x100809a0 is memset, 0x10080a90 strncpy, 0x10021c20 the
//  renderer's allocator.  The two 0x17c-byte locals are DDCAPS (DirectX 7
//  size, dwSize stamped after each memset) passed to IDirectDraw vtable
//  +0x2c, GetCaps(hal, hel); +0x08 is Release.  The toolchain's ddraw.h is
//  the DirectX 5 one (smaller DDCAPS), so the pieces used are declared here.
//
//  THE RECORD (0xec bytes) is an LTLink at +0x00 (m_pData = the record
//  itself, +0x08), `hal.dwCaps & DDCAPS_3D` as a flag at +0x0c, a copy of
//  the GUID at +0x10, a pointer to that copy (or NULL for the primary
//  display driver) at +0x20, and the two names strncpy'd to 100 bytes at
//  +0x24 and +0x88.  It is linked onto the device list 0x1008bf78 with
//  ltlink.h's AddAfter, store for store (+0 = head, +4 = head->next,
//  next->prev, prev->next); d3dr2a_os.cpp's 0x10020f80 walks that list and
//  prints +0x24 as "Device: %s".
// *********************************************************************** //

#include "ltbasedefs.h"

extern "C"
{
	void*	__cdecl memset(void* pDst, int c, unsigned int nBytes);
	char*	__cdecl strncpy(char* pDst, const char* pSrc, unsigned int nMax);
}

// memset is CALLED (0x100809a0), not expanded.
#pragma function(memset)

struct GUID10069ca0
{
	uint32	m_Data[4];
};

// DDCAPS, DirectX 7 layout: only dwSize and dwCaps are touched.
struct DDCaps10069ca0
{
	uint32	m_dwSize;                                   // +0x00
	uint32	m_dwCaps;                                   // +0x04
	uint8	m_pad_08[0x17c - 0x08];
};

#define DDCAPS_3D_10069ca0		0x00000001

class DDraw10069ca0
{
public:
	virtual long __stdcall QueryInterface(void* riid, void** ppv) = 0;  // +0x00
	virtual uint32 __stdcall AddRef() = 0;                               // +0x04
	virtual uint32 __stdcall Release() = 0;                              // +0x08
	virtual long __stdcall _v0c() = 0;
	virtual long __stdcall _v10() = 0;
	virtual long __stdcall _v14() = 0;
	virtual long __stdcall _v18() = 0;
	virtual long __stdcall _v1c() = 0;
	virtual long __stdcall _v20() = 0;
	virtual long __stdcall _v24() = 0;
	virtual long __stdcall _v28() = 0;
	virtual long __stdcall GetCaps(DDCaps10069ca0* pHAL,
	                               DDCaps10069ca0* pHEL) = 0;            // +0x2c
};

extern "C" long __stdcall DirectDrawCreate(GUID10069ca0* pGuid,
                                           DDraw10069ca0** ppDD,
                                           void* pUnkOuter);

extern "C" void* sub_10021c20(uint32 nBytes);                  // 0x10021c20

// The device list head.  d3ddata.cpp defines it under this tag; only the
// tag reaches the mangled name, so it is spelled here as the LTLink it is.
struct Link10020f80 : public LTLink
{
};

extern Link10020f80 g_1008bf78;                                // 0x1008bf78

struct DDDevice10069ca0
{
	LTLink			m_Link;                             // +0x00
	LTBOOL			m_bHW3D;                            // +0x0c
	GUID10069ca0	m_Guid;                             // +0x10
	GUID10069ca0*	m_pGuid;                            // +0x20
	char			m_szName[100];                      // +0x24
	char			m_szDesc[100];                      // +0x88
};


// ======================================================================= //
//  0x10069ca0  360 bytes (12-byte stack entry + 348-byte register entry
//  at 0x10069cac), `ret $0x10`.
// ======================================================================= //
extern "C" LTBOOL __stdcall sub_10069ca0(GUID10069ca0* pGuid, char* pDesc,
                                          char* pName, void* pContext)
{
	DDraw10069ca0*		pDD;
	DDCaps10069ca0		halCaps;
	DDCaps10069ca0		helCaps;
	DDDevice10069ca0*	pDev;

	if (DirectDrawCreate(pGuid, &pDD, LTNULL) != 0)
		return LTTRUE;

	memset(&halCaps, 0, sizeof(halCaps));
	halCaps.m_dwSize = sizeof(halCaps);
	memset(&helCaps, 0, sizeof(helCaps));
	helCaps.m_dwSize = sizeof(helCaps);

	if (pDD->GetCaps(&halCaps, &helCaps) != 0)
	{
		pDD->Release();
		return LTTRUE;
	}

	pDev = (DDDevice10069ca0*)sub_10021c20(sizeof(DDDevice10069ca0));
	if (pDev)
	{
		pDev->m_bHW3D = (halCaps.m_dwCaps & DDCAPS_3D_10069ca0) != 0;
		pDev->m_Link.m_pData = pDev;

		if (pGuid)
		{
			pDev->m_Guid = *pGuid;
			pDev->m_pGuid = &pDev->m_Guid;
		}
		else
		{
			pDev->m_pGuid = LTNULL;
		}

		strncpy(pDev->m_szName, pName, 100);
		strncpy(pDev->m_szDesc, pDesc, 100);

		g_1008bf78.AddAfter(&pDev->m_Link);
	}

	pDD->Release();
	return LTTRUE;
}


// ======================================================================= //
//  0x10069e10  44 bytes, `ret $0x14`.  The Ex callback: the same four
//  arguments through 0x10069ca0's register entry, the HMONITOR dropped,
//  and TRUE regardless of what that answered.
// ======================================================================= //
extern "C" LTBOOL __stdcall sub_10069e10(GUID10069ca0* pGuid, char* pDesc,
                                          char* pName, void* pContext,
                                          void* hMonitor)
{
	sub_10069ca0(pGuid, pDesc, pName, pContext);
	return LTTRUE;
}
