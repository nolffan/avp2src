// *********************************************************************** //
//
//  CLUGLOB.H -- lithtech.exe's client glob, 0x004b0ac0 .. 0x004b13e3
//
//  ONE object, 0x924 bytes: clientwin.cpp's 0x00402f90 zeroes the whole
//  range with one inlined memset (`mov $0x4b0ac0,%edi; mov $0x249,%ecx;
//  rep stos`), so nothing inside it can be a separate definition.  It is
//  DEFINED in src/engine/lithtech/clientwin.cpp (the Windows entry layer
//  that owns it); every other TU reaches its fields through
//  g_CluClientGlob.  Offsets are the shipped ones; names ours.
//
//  Self-contained on purpose (no windows.h, no ltbasedefs.h): the handle
//  fields are spelled `HWND__*` / `HINSTANCE__*`, which is what HWND and
//  HINSTANCE are under VC6's default STRICT.
//
// *********************************************************************** //

#ifndef __CLUGLOB_H__
#define __CLUGLOB_H__

#include <setjmp.h>

struct HWND__;
struct HINSTANCE__;
class  CluClientMgrView;

struct CluClientGlob
{
	int               m_bWindowUp;           // 0x000  0x004b0ac0  1 while the pump runs
	jmp_buf           m_MemoryJmp;           // 0x004  0x004b0ac4  the out-of-memory landing
	HWND__*           m_hMainWnd;            // 0x044  0x004b0b04
	HINSTANCE__*      m_hInstance;           // 0x048  0x004b0b08
	char*             m_pClassName;          // 0x04c  0x004b0b0c
	char*             m_pWindowTitle;        // 0x050  0x004b0b10
	// The client manager; other TUs read the same dword as the engine hook
	// manager / render callback manager (0x004b0b14).
	CluClientMgrView* m_pClientMgr;          // 0x054  0x004b0b14
	int               m_bInInitRender;       // 0x058  0x004b0b18
	int               m_bBreakOnError;       // 0x05c  0x004b0b1c  the debug-break flag
	int               m_nUnknown60;          // 0x060  1 at start-up, toggled by
	                                         //        the focus handler
	int               m_bAppLostFocus;       // 0x064  0x004b0b24
	int               m_bMinimizing;         // 0x068  set by SC_CLOSE
	int               m_nUnknown6c;          // 0x06c
	int               m_bRenderShutdown;     // 0x070  0x004b0b30
	int               m_bHost;               // 0x074
	unsigned char     m_pad_078[0x7c - 0x78];
	char*             m_pWorldName;          // 0x07c
	unsigned char     m_pad_080[0x274 - 0x80];
	// The three 100-entry key queues WndProc fills and clientdsi.cpp's
	// 0x0042e5b0..0x0042e5f0 read/clear, and their two uint16 counts.
	int               m_KeyDownVK[100];      // 0x274  0x004b0d34
	int               m_KeyUpVK[100];        // 0x404  0x004b0ec4
	int               m_KeyDownRep[100];     // 0x594  0x004b1054
	unsigned short    m_nKeyDowns;           // 0x724  0x004b11e4
	unsigned short    m_nKeyUps;             // 0x726  0x004b11e6
	unsigned long     m_nUnknown728;         // 0x728  0x004b11e8  (0x0042e670 / 0x0042e680)
	int               m_bInputEnabled;       // 0x72c  0x004b11ec
	char              m_ErrorString[0x924 - 0x730];   // 0x730  0x004b11f0 .. end
};

extern CluClientGlob g_CluClientGlob;        // 0x004b0ac0

#endif  // __CLUGLOB_H__
