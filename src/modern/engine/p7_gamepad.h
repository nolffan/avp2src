// ----------------------------------------------------------------------- //
//
// MODULE  : p7_gamepad.h
//
// PURPOSE : The engine's ##gamepad, as clientinput.cpp's modern blocks
//           (nolfsrc's M-P7-01 and M-P7-09, imported with the engine) call
//           it.  AvP2's controller support (docs/MODERN.md, M-7) is not
//           implemented yet: until it is, there is no pad -- these say so,
//           and ##gamepad is created as DirectInput would create the
//           device -- so the modern engine builds, without DirectInput's
//           joysticks (which those blocks leave out, M-7's XInput pad
//           taking their place) and with only the keyboard and mouse
//           looked for (M-P7-09).
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __P7_GAMEPAD_H__
#define __P7_GAMEPAD_H__

// Is a controller there to be ##gamepad?  (Not yet: M-7.)
inline BOOL P7Pad_Available(void)
{
	return FALSE;
}

// ##gamepad's device description (only asked for when there is one).
inline void P7Pad_GetInstance(DIDEVICEINSTANCEA* pInstance)
{
	memset(pInstance, 0, sizeof(*pInstance));
	pInstance->dwSize = sizeof(*pInstance);
}

// Is this device ##gamepad?
inline BOOL P7Pad_IsPad(REFGUID guidInstance)
{
	return FALSE;
}

// A device by its GUID: ##gamepad's would be ours; every other is
// DirectInput's.
inline HRESULT P7Pad_CreateDevice(LPDIRECTINPUTA pDI, REFGUID guid, LPDIRECTINPUTDEVICEA* ppDevice)
{
	return pDI->CreateDevice(guid, ppDevice, NULL);
}

#endif // __P7_GAMEPAD_H__
