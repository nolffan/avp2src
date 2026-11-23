// mrplatform.h -- what the renderer core needs from the system under it.
//
// The display modes and the engine's window sized for a mode:
// mr_window.cpp does them with user32 and ChangeDisplaySettings. The device
// on that window is the backend's (rdev.h). Everything else in the core is
// the engine's interface.

#ifndef MRPLATFORM_H
#define MRPLATFORM_H

#ifdef _WIN32
#include <windows.h>
#else
#include <strings.h>
#define _stricmp strcasecmp
#endif
#include <stdint.h>

// Every display mode at 16 and 32 bits, 640x480 and up, once each.
typedef void (*PlatModeFn)(uint32_t nWidth, uint32_t nHeight, uint32_t nBitDepth, void* pUser);
void plat_EnumModes(PlatModeFn fn, void* pUser);

// Size the engine's window (RenderStructInit::m_hWnd) for the mode: the
// whole display at that mode for full screen, else a centred window whose
// client area is the mode. *pbWindowed comes back true when the display
// could not be switched. plat_CloseWindow undoes it.
void plat_OpenWindow(void* hWnd, uint32_t nWidth, uint32_t nHeight, bool* pbWindowed);
void plat_CloseWindow(void);

#endif
