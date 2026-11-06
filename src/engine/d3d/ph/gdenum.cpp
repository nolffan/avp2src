// *********************************************************************** //
//  ph/gdenum.cpp -- Primal Hunt's d3d.ren, TU35, second object
//  (0x10032af0..0x10032c6c), /O1.
//
//  The DirectDraw device enumeration: DirectDrawEnumerateExA, looked up by
//  hand in ddraw.dll so the DX7 entry point stays optional, falling back
//  to DirectDrawEnumerateA; each device found is added to common_init's
//  device list.  No string or path names this object; "gdenum" is a guess,
//  spelled to sort between fullintersectline and geomroutines, the objects
//  either side of it in retail's link.
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <string.h>

#include "ltbasedefs.h"


// One DirectDraw device (common_init.cpp's D3DDeviceInfo).
struct D3DDeviceInfo
{
	LTLink		m_Link;                                 // +0x00
	LTBOOL		m_bHardware;                            // +0x0c
	GUID		m_DriverGUID;                           // +0x10
	GUID		*m_pDriverGUID;                         // +0x20
	char		m_Name[100];                            // +0x24
	char		m_Desc[100];                            // +0x88
};

extern LTLink	g_Devices;                              // 0x1005d298 (common_init.cpp)

void	d3d_FreeDeviceList();                           // 0x100116fe
void	AddDebugMessage(int32 debugLevel, const char* pMsg, ...);   // 0x10013b13
void*	dalloc(uint32 size);                            // 0x10013a66

static BOOL WINAPI d3d_EnumDDrawCallback(GUID *pGUID, LPSTR pDesc, LPSTR pName, LPVOID pContext);
static BOOL WINAPI d3d_EnumDDrawCallbackEx(GUID *pGUID, LPSTR pDesc, LPSTR pName, LPVOID pContext, HMONITOR hMonitor);

typedef HRESULT (WINAPI *DDEnumerateExFn)(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);


// 0x10032af0
LTBOOL d3d_EnumDevices()
{
	HMODULE hDDraw;
	DDEnumerateExFn fn;

	d3d_FreeDeviceList();

	hDDraw = GetModuleHandle("DDRAW.DLL");
	if (hDDraw)
	{
		fn = (DDEnumerateExFn)GetProcAddress(hDDraw, "DirectDrawEnumerateExA");
		if (fn)
		{
			if (fn(d3d_EnumDDrawCallbackEx, LTNULL,
				DDENUM_ATTACHEDSECONDARYDEVICES | DDENUM_DETACHEDSECONDARYDEVICES | DDENUM_NONDISPLAYDEVICES) == DD_OK)
			{
				return LTTRUE;
			}

			AddDebugMessage(1, "DirectDrawEnumerateExA failed, using DirectDrawEnumerate");
		}
		else
		{
			AddDebugMessage(1, "Unable to get DirectDrawEnumerateExA proc address, using DirectDrawEnumerate");
		}
	}
	else
	{
		AddDebugMessage(1, "Unable to get ddraw.dll module handle");
	}

	d3d_FreeDeviceList();
	return DirectDrawEnumerate(d3d_EnumDDrawCallback, LTNULL) == DD_OK;
}


// 0x10032b5a
static BOOL WINAPI d3d_EnumDDrawCallback(GUID *pGUID, LPSTR pDesc, LPSTR pName, LPVOID pContext)
{
	LPDIRECTDRAW pDD;
	DDCAPS hwCaps, helCaps;
	D3DDeviceInfo *pDevice;

	if (DirectDrawCreate(pGUID, &pDD, LTNULL) != DD_OK)
		return TRUE;

	memset(&hwCaps, 0, sizeof(hwCaps));
	hwCaps.dwSize = sizeof(hwCaps);
	memset(&helCaps, 0, sizeof(helCaps));
	helCaps.dwSize = sizeof(helCaps);

	if (pDD->GetCaps(&hwCaps, &helCaps) != DD_OK)
	{
		pDD->Release();
		return TRUE;
	}

	pDevice = (D3DDeviceInfo*)dalloc(sizeof(D3DDeviceInfo));
	if (pDevice)
	{
		pDevice->m_bHardware = hwCaps.dwCaps & DDCAPS_3D;
		pDevice->m_Link.m_pData = pDevice;

		if (pGUID)
		{
			pDevice->m_DriverGUID = *pGUID;
			pDevice->m_pDriverGUID = &pDevice->m_DriverGUID;
		}
		else
		{
			pDevice->m_pDriverGUID = LTNULL;
		}

		strncpy(pDevice->m_Name, pName, sizeof(pDevice->m_Name));
		strncpy(pDevice->m_Desc, pDesc, sizeof(pDevice->m_Desc));

		dl_Insert(&g_Devices, &pDevice->m_Link);
	}

	pDD->Release();
	return TRUE;
}


// 0x10032c54
static BOOL WINAPI d3d_EnumDDrawCallbackEx(GUID *pGUID, LPSTR pDesc, LPSTR pName, LPVOID pContext, HMONITOR hMonitor)
{
	return d3d_EnumDDrawCallback(pGUID, pDesc, pName, pContext);
}
