// ----------------------------------------------------------------------- //
//
// MODULE  : p7_gamepad.h
//
// PURPOSE : The modern build's controller (docs/MODERN.md, M-7): the
//           engine's ##gamepad input device.  clientinput.cpp includes
//           this, once, from the blocks it took from nolfsrc's engine
//           (their M-P7-01 and M-P7-09: the file keeps the name they call
//           it by).
//
//           An XInput pad (an Xbox-style controller; what Steam Input
//           presents, and what Wine makes of most pads) as one more
//           DirectInput device: an object behind IDirectInputDevice2A
//           that answers the calls the engine makes of a joystick from
//           XInput's state.  So binding a command to one of its buttons
//           (bind "##gamepad" "A Button" "Jump"), the buffered reads, the
//           config save and Customize Controls' "press a key" capture all
//           work on it unchanged.  It is the only joystick: clientinput.cpp
//           leaves DirectInput's out (under Wine one of them is the same
//           pad), so the profile's joystick bindings are its.
//
//           Six absolute axes and sixteen buttons:
//
//             Left Stick X / Y, Right Stick X / Y   -1000..1000, Y down
//                                                   positive (a joystick's)
//             Left / Right Trigger Axis             0..1000
//             A / B / X / Y Button, Left / Right Bumper, Back, Start,
//             Left / Right Stick Click, D-Pad Up / Down / Left / Right,
//             Left / Right Trigger (pressed past XInput's threshold)
//
//           It reads the pad through m7_padread.h, and does what a
//           foreground DirectInput device does: it is acquired only while
//           the game's window is the foreground one, and losing the
//           foreground loses the acquisition.  Changes are buffered as
//           DIDEVICEOBJECTDATA at the offsets of whatever data format the
//           engine set.  No force feedback.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __P7_GAMEPAD_H__
#define __P7_GAMEPAD_H__

#include <windows.h>
#include <dinput.h>
#include "m7_padread.h"

namespace
{
	#ifndef DIDFT_OPTIONAL
	#define DIDFT_OPTIONAL		0x80000000
	#endif

	// {41565032-4D37-4741-4D45-504144000001}: "AVP2", "M7", "GAMEPAD".
	const GUID c_guidM7Pad =
		{ 0x41565032, 0x4d37, 0x4741, { 0x4d, 0x45, 0x50, 0x41, 0x44, 0x00, 0x00, 0x01 } };

	enum { M7_AXIS, M7_BUTTON };

	struct M7Object
	{
		const char*	m_pName;
		const GUID*	m_pGuid;
		int			m_nKind;
		DWORD		m_nInstance;
		DWORD		m_dwStdOfs;			// its place in c_dfDIJoystick's DIJOYSTATE
		LONG		m_nMin;
		LONG		m_nMax;
	};

	const M7Object c_M7Objects[] =
	{
		{ "Left Stick X",		&GUID_XAxis,	M7_AXIS,	0,	DIJOFS_X,			-1000, 1000 },
		{ "Left Stick Y",		&GUID_YAxis,	M7_AXIS,	1,	DIJOFS_Y,			-1000, 1000 },
		{ "Left Trigger Axis",	&GUID_ZAxis,	M7_AXIS,	2,	DIJOFS_Z,			0, 1000 },
		{ "Right Stick X",		&GUID_RxAxis,	M7_AXIS,	3,	DIJOFS_RX,			-1000, 1000 },
		{ "Right Stick Y",		&GUID_RyAxis,	M7_AXIS,	4,	DIJOFS_RY,			-1000, 1000 },
		{ "Right Trigger Axis",	&GUID_RzAxis,	M7_AXIS,	5,	DIJOFS_RZ,			0, 1000 },
		{ "A Button",			&GUID_Button,	M7_BUTTON,	0,	DIJOFS_BUTTON(0),	0, 0 },
		{ "B Button",			&GUID_Button,	M7_BUTTON,	1,	DIJOFS_BUTTON(1),	0, 0 },
		{ "X Button",			&GUID_Button,	M7_BUTTON,	2,	DIJOFS_BUTTON(2),	0, 0 },
		{ "Y Button",			&GUID_Button,	M7_BUTTON,	3,	DIJOFS_BUTTON(3),	0, 0 },
		{ "Left Bumper",		&GUID_Button,	M7_BUTTON,	4,	DIJOFS_BUTTON(4),	0, 0 },
		{ "Right Bumper",		&GUID_Button,	M7_BUTTON,	5,	DIJOFS_BUTTON(5),	0, 0 },
		{ "Back",				&GUID_Button,	M7_BUTTON,	6,	DIJOFS_BUTTON(6),	0, 0 },
		{ "Start",				&GUID_Button,	M7_BUTTON,	7,	DIJOFS_BUTTON(7),	0, 0 },
		{ "Left Stick Click",	&GUID_Button,	M7_BUTTON,	8,	DIJOFS_BUTTON(8),	0, 0 },
		{ "Right Stick Click",	&GUID_Button,	M7_BUTTON,	9,	DIJOFS_BUTTON(9),	0, 0 },
		{ "D-Pad Up",			&GUID_Button,	M7_BUTTON,	10,	DIJOFS_BUTTON(10),	0, 0 },
		{ "D-Pad Down",			&GUID_Button,	M7_BUTTON,	11,	DIJOFS_BUTTON(11),	0, 0 },
		{ "D-Pad Left",			&GUID_Button,	M7_BUTTON,	12,	DIJOFS_BUTTON(12),	0, 0 },
		{ "D-Pad Right",		&GUID_Button,	M7_BUTTON,	13,	DIJOFS_BUTTON(13),	0, 0 },
		{ "Left Trigger",		&GUID_Button,	M7_BUTTON,	14,	DIJOFS_BUTTON(14),	0, 0 },
		{ "Right Trigger",		&GUID_Button,	M7_BUTTON,	15,	DIJOFS_BUTTON(15),	0, 0 },
	};

	enum
	{
		M7_OBJECTS	= sizeof(c_M7Objects) / sizeof(c_M7Objects[0]),
		M7_AXES		= 6,
		M7_BUTTONS	= 16,
		M7_BUFFER	= 256				// the most events a buffer holds
	};

	// The XInput bit of each button but the two triggers, in the table's
	// order from the first button.
	const WORD c_M7ButtonBits[M7_BUTTONS - 2] =
	{
		M7PAD_A, M7PAD_B, M7PAD_X, M7PAD_Y, M7PAD_LEFT_SHOULDER, M7PAD_RIGHT_SHOULDER,
		M7PAD_BACK, M7PAD_START, M7PAD_LEFT_THUMB, M7PAD_RIGHT_THUMB,
		M7PAD_DPAD_UP, M7PAD_DPAD_DOWN, M7PAD_DPAD_LEFT, M7PAD_DPAD_RIGHT
	};

	DWORD M7ObjectType(int i)
	{
		return (c_M7Objects[i].m_nKind == M7_AXIS ? DIDFT_ABSAXIS : DIDFT_PSHBUTTON) |
		       DIDFT_MAKEINSTANCE(c_M7Objects[i].m_nInstance);
	}

	LONG M7Thumb(SHORT n, BOOL bFlip)
	{
		LONG v = ((LONG)n * 1000) / 32767;

		if (bFlip)
			v = -v;

		return v < -1000 ? -1000 : v > 1000 ? 1000 : v;
	}

	// Every object's value, in the table's order, from the pad's state.
	void M7PadValues(const M7PadState& pad, LONG* pValues)
	{
		int i;

		pValues[0] = M7Thumb(pad.sThumbLX, FALSE);
		pValues[1] = M7Thumb(pad.sThumbLY, TRUE);
		pValues[2] = ((LONG)pad.bLeftTrigger * 1000) / 255;
		pValues[3] = M7Thumb(pad.sThumbRX, FALSE);
		pValues[4] = M7Thumb(pad.sThumbRY, TRUE);
		pValues[5] = ((LONG)pad.bRightTrigger * 1000) / 255;

		for (i = 0; i < M7_BUTTONS - 2; i++)
			pValues[M7_AXES + i] = (pad.wButtons & c_M7ButtonBits[i]) ? 0x80 : 0;

		pValues[M7_AXES + 14] = pad.bLeftTrigger  > M7PAD_TRIGGER_THRESHOLD ? 0x80 : 0;
		pValues[M7_AXES + 15] = pad.bRightTrigger > M7PAD_TRIGGER_THRESHOLD ? 0x80 : 0;
	}

	// The object a property header names, or -1.
	int M7FindObject(LPCDIPROPHEADER pHeader, const int* pFormatOfs)
	{
		int i;

		for (i = 0; i < M7_OBJECTS; i++)
		{
			if (pHeader->dwHow == DIPH_BYID && pHeader->dwObj == M7ObjectType(i))
				return i;

			if (pHeader->dwHow == DIPH_BYOFFSET && pFormatOfs[i] >= 0 &&
			    pHeader->dwObj == (DWORD)pFormatOfs[i])
				return i;
		}

		return -1;
	}

	// Is the window, or one it belongs to, the foreground one?
	BOOL M7IsForeground(HWND hWnd)
	{
		HWND h;

		if (!hWnd)
			return TRUE;

		for (h = GetForegroundWindow(); h; h = GetParent(h))
		{
			if (h == hWnd)
				return TRUE;
		}

		return FALSE;
	}

	void M7GetInstance(DIDEVICEINSTANCEA* pInstance)
	{
		memset(pInstance, 0, sizeof(DIDEVICEINSTANCEA));
		pInstance->dwSize       = sizeof(DIDEVICEINSTANCEA);
		pInstance->guidInstance = c_guidM7Pad;
		pInstance->guidProduct  = c_guidM7Pad;
		pInstance->dwDevType    = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_GAMEPAD << 8) | DIDEVTYPE_HID;
		strcpy(pInstance->tszInstanceName, "##gamepad");
		strcpy(pInstance->tszProductName, "XInput Gamepad");
	}

	class CM7PadDevice : public IDirectInputDevice2A
	{
	public:
		CM7PadDevice()
		{
			int i;

			m_nRef       = 1;
			m_hWnd       = NULL;
			m_dwCoop     = 0;
			m_bAcquired  = FALSE;
			m_bLost      = FALSE;
			m_bFormat    = FALSE;
			m_dwBufSize  = 0;
			m_nBuf       = 0;
			m_bOverflow  = FALSE;
			m_dwSequence = 0;

			for (i = 0; i < M7_OBJECTS; i++)
			{
				m_FormatOfs[i] = -1;
				m_Value[i]     = 0;
				m_Range[i][0]  = c_M7Objects[i].m_nMin;
				m_Range[i][1]  = c_M7Objects[i].m_nMax;
			}
		}

		// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv)
		{
			if (IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IDirectInputDeviceA) ||
			    IsEqualGUID(riid, IID_IDirectInputDevice2A))
			{
				*ppv = (IDirectInputDevice2A*)this;
				AddRef();
				return S_OK;
			}

			*ppv = NULL;
			return E_NOINTERFACE;
		}

		STDMETHODIMP_(ULONG) AddRef()
		{
			return ++m_nRef;
		}

		STDMETHODIMP_(ULONG) Release()
		{
			ULONG n = --m_nRef;

			if (n == 0)
				delete this;

			return n;
		}

		// IDirectInputDeviceA
		STDMETHODIMP GetCapabilities(LPDIDEVCAPS pCaps)
		{
			M7PadState        pad;
			DIDEVICEINSTANCEA inst;

			if (!pCaps || pCaps->dwSize < sizeof(DIDEVCAPS))
				return DIERR_INVALIDPARAM;

			M7GetInstance(&inst);
			memset(pCaps, 0, sizeof(DIDEVCAPS));
			pCaps->dwSize    = sizeof(DIDEVCAPS);
			pCaps->dwFlags   = (M7PadRead_State(&pad) ? DIDC_ATTACHED : 0) | DIDC_POLLEDDATAFORMAT;
			pCaps->dwDevType = inst.dwDevType;
			pCaps->dwAxes    = M7_AXES;
			pCaps->dwButtons = M7_BUTTONS;
			pCaps->dwPOVs    = 0;
			return DI_OK;
		}

		STDMETHODIMP EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA pCallback, LPVOID pvRef, DWORD dwFlags)
		{
			DIDEVICEOBJECTINSTANCEA doi;
			int i;

			if (!pCallback)
				return DIERR_INVALIDPARAM;

			for (i = 0; i < M7_OBJECTS; i++)
			{
				if (dwFlags != DIDFT_ALL && !(DIDFT_GETTYPE(dwFlags) & DIDFT_GETTYPE(M7ObjectType(i))))
					continue;

				GetObject(i, &doi);

				if (pCallback(&doi, pvRef) == DIENUM_STOP)
					break;
			}

			return DI_OK;
		}

		STDMETHODIMP GetProperty(REFGUID rguidProp, LPDIPROPHEADER pHeader)
		{
			int i;

			if (!pHeader)
				return DIERR_INVALIDPARAM;

			// (DIPROP_* are small integers cast to GUID pointers)
			if (&rguidProp == &DIPROP_BUFFERSIZE)
			{
				((LPDIPROPDWORD)pHeader)->dwData = m_dwBufSize;
				return DI_OK;
			}

			if (&rguidProp == &DIPROP_RANGE)
			{
				i = M7FindObject(pHeader, m_FormatOfs);
				if (i < 0 || c_M7Objects[i].m_nKind != M7_AXIS)
					return DIERR_OBJECTNOTFOUND;

				((LPDIPROPRANGE)pHeader)->lMin = m_Range[i][0];
				((LPDIPROPRANGE)pHeader)->lMax = m_Range[i][1];
				return DI_OK;
			}

			return DIERR_UNSUPPORTED;
		}

		STDMETHODIMP SetProperty(REFGUID rguidProp, LPCDIPROPHEADER pHeader)
		{
			int i;

			if (!pHeader)
				return DIERR_INVALIDPARAM;

			if (&rguidProp == &DIPROP_BUFFERSIZE)
			{
				if (m_bAcquired)
					return DIERR_ACQUIRED;

				m_dwBufSize = ((LPCDIPROPDWORD)pHeader)->dwData;
				if (m_dwBufSize > M7_BUFFER)
					m_dwBufSize = M7_BUFFER;

				m_nBuf = 0;
				return DI_OK;
			}

			if (&rguidProp == &DIPROP_RANGE)
			{
				LONG nMin = ((LPCDIPROPRANGE)pHeader)->lMin;
				LONG nMax = ((LPCDIPROPRANGE)pHeader)->lMax;

				for (i = 0; i < M7_OBJECTS; i++)
				{
					if (c_M7Objects[i].m_nKind != M7_AXIS)
						continue;

					if (pHeader->dwHow == DIPH_DEVICE || M7FindObject(pHeader, m_FormatOfs) == i)
					{
						m_Range[i][0] = nMin;
						m_Range[i][1] = nMax;
					}
				}

				return DI_OK;
			}

			// Dead zone, saturation, ...: the game applies its own.
			return DI_PROPNOEFFECT;
		}

		STDMETHODIMP Acquire()
		{
			M7PadState pad;

			if (!m_bFormat)
				return DIERR_INVALIDPARAM;

			if ((m_dwCoop & DISCL_FOREGROUND) && !M7IsForeground(m_hWnd))
				return DIERR_OTHERAPPHASPRIO;

			if (!m_bAcquired)
			{
				// What is held now is the state the buffered changes start
				// from, as with DirectInput.
				M7PadRead_State(&pad);
				M7PadValues(pad, m_Value);

				m_bAcquired = TRUE;
				m_bLost     = FALSE;
				m_nBuf      = 0;
				m_bOverflow = FALSE;
			}

			return DI_OK;
		}

		STDMETHODIMP Unacquire()
		{
			if (!m_bAcquired)
				return DI_NOEFFECT;

			m_bAcquired = FALSE;
			m_nBuf      = 0;
			m_bOverflow = FALSE;
			return DI_OK;
		}

		STDMETHODIMP GetDeviceState(DWORD cbData, LPVOID pvData)
		{
			int i;

			if (!pvData)
				return DIERR_INVALIDPARAM;

			if (!Update())
				return m_bLost ? DIERR_INPUTLOST : DIERR_NOTACQUIRED;

			memset(pvData, 0, cbData);

			for (i = 0; i < M7_OBJECTS; i++)
			{
				DWORD dwOfs = (DWORD)m_FormatOfs[i];

				if (m_FormatOfs[i] < 0)
					continue;

				if (c_M7Objects[i].m_nKind == M7_AXIS)
				{
					if (dwOfs + sizeof(LONG) <= cbData)
						*(LONG*)((BYTE*)pvData + dwOfs) = Scaled(i, m_Value[i]);
				}
				else if (dwOfs < cbData)
				{
					*((BYTE*)pvData + dwOfs) = (BYTE)m_Value[i];
				}
			}

			return DI_OK;
		}

		STDMETHODIMP GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut,
		                           DWORD dwFlags)
		{
			DWORD   n;
			DWORD   i;
			HRESULT hr;

			if (!pdwInOut)
				return DIERR_INVALIDPARAM;

			if (!Update())
				return m_bLost ? DIERR_INPUTLOST : DIERR_NOTACQUIRED;

			hr = m_bOverflow ? DI_BUFFEROVERFLOW : DI_OK;

			// A null array and INFINITE: empty the buffer.
			if (!rgdod && *pdwInOut == INFINITE)
			{
				*pdwInOut   = m_nBuf;
				m_nBuf      = 0;
				m_bOverflow = FALSE;
				return hr;
			}

			n = *pdwInOut < m_nBuf ? *pdwInOut : m_nBuf;

			if (rgdod)
			{
				for (i = 0; i < n; i++)
					memcpy((BYTE*)rgdod + i * cbObjectData, &m_Buf[i],
					       cbObjectData < sizeof(DIDEVICEOBJECTDATA) ? cbObjectData : sizeof(DIDEVICEOBJECTDATA));
			}

			if (!(dwFlags & DIGDD_PEEK))
			{
				memmove(m_Buf, m_Buf + n, (m_nBuf - n) * sizeof(DIDEVICEOBJECTDATA));
				m_nBuf     -= n;
				m_bOverflow = FALSE;
			}

			*pdwInOut = n;
			return hr;
		}

		STDMETHODIMP SetDataFormat(LPCDIDATAFORMAT pFormat)
		{
			BOOL  bUsed[M7_OBJECTS];
			DWORD i;
			int   j;

			if (!pFormat)
				return DIERR_INVALIDPARAM;

			if (m_bAcquired)
				return DIERR_ACQUIRED;

			for (j = 0; j < M7_OBJECTS; j++)
			{
				m_FormatOfs[j] = -1;
				bUsed[j]       = FALSE;
			}

			// Each entry takes the first object not yet taken that it
			// describes: by type, instance (or any) and GUID (or any).
			for (i = 0; i < pFormat->dwNumObjs; i++)
			{
				const DIOBJECTDATAFORMAT* pEntry =
					(const DIOBJECTDATAFORMAT*)((const BYTE*)pFormat->rgodf + i * pFormat->dwObjSize);
				DWORD dwType = pEntry->dwType;

				for (j = 0; j < M7_OBJECTS; j++)
				{
					if (bUsed[j])
						continue;

					if (DIDFT_GETTYPE(dwType) && !(DIDFT_GETTYPE(dwType) & DIDFT_GETTYPE(M7ObjectType(j))))
						continue;

					if ((dwType & DIDFT_INSTANCEMASK) != DIDFT_ANYINSTANCE &&
					    DIDFT_GETINSTANCE(dwType) != c_M7Objects[j].m_nInstance)
						continue;

					if (pEntry->pguid && !IsEqualGUID(*pEntry->pguid, *c_M7Objects[j].m_pGuid))
						continue;

					bUsed[j]       = TRUE;
					m_FormatOfs[j] = (int)pEntry->dwOfs;
					break;
				}
			}

			m_bFormat = TRUE;
			m_nBuf    = 0;
			return DI_OK;
		}

		STDMETHODIMP SetEventNotification(HANDLE)
		{
			return DI_OK;
		}

		STDMETHODIMP SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
		{
			m_hWnd   = hWnd;
			m_dwCoop = dwFlags;
			return DI_OK;
		}

		STDMETHODIMP GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA pdoi, DWORD dwObj, DWORD dwHow)
		{
			DIPROPHEADER header;
			int i;

			if (!pdoi)
				return DIERR_INVALIDPARAM;

			header.dwObj = dwObj;
			header.dwHow = dwHow;
			i = M7FindObject(&header, m_FormatOfs);

			if (i < 0)
				return DIERR_OBJECTNOTFOUND;

			GetObject(i, pdoi);
			return DI_OK;
		}

		STDMETHODIMP GetDeviceInfo(LPDIDEVICEINSTANCEA pdidi)
		{
			if (!pdidi)
				return DIERR_INVALIDPARAM;

			M7GetInstance(pdidi);
			return DI_OK;
		}

		STDMETHODIMP RunControlPanel(HWND, DWORD)
		{
			return DI_OK;
		}

		STDMETHODIMP Initialize(HINSTANCE, DWORD, REFGUID)
		{
			return DI_OK;
		}

		// IDirectInputDevice2A: no force feedback.
		STDMETHODIMP CreateEffect(REFGUID, LPCDIEFFECT, LPDIRECTINPUTEFFECT* ppEffect, LPUNKNOWN)
		{
			if (ppEffect)
				*ppEffect = NULL;

			return DIERR_UNSUPPORTED;
		}

		STDMETHODIMP EnumEffects(LPDIENUMEFFECTSCALLBACKA, LPVOID, DWORD)
		{
			return DI_OK;
		}

		STDMETHODIMP GetEffectInfo(LPDIEFFECTINFOA, REFGUID)
		{
			return DIERR_UNSUPPORTED;
		}

		STDMETHODIMP GetForceFeedbackState(LPDWORD)
		{
			return DIERR_UNSUPPORTED;
		}

		STDMETHODIMP SendForceFeedbackCommand(DWORD)
		{
			return DIERR_UNSUPPORTED;
		}

		STDMETHODIMP EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK, LPVOID, DWORD)
		{
			return DI_OK;
		}

		STDMETHODIMP Escape(LPDIEFFESCAPE)
		{
			return DIERR_UNSUPPORTED;
		}

		STDMETHODIMP Poll()
		{
			if (!Update())
				return m_bLost ? DIERR_INPUTLOST : DIERR_NOTACQUIRED;

			return DI_OK;
		}

		STDMETHODIMP SendDeviceData(DWORD, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD)
		{
			return DIERR_UNSUPPORTED;
		}

	private:
		~CM7PadDevice()
		{
		}

		void GetObject(int i, LPDIDEVICEOBJECTINSTANCEA pdoi)
		{
			memset(pdoi, 0, sizeof(DIDEVICEOBJECTINSTANCEA));
			pdoi->dwSize   = sizeof(DIDEVICEOBJECTINSTANCEA);
			pdoi->guidType = *c_M7Objects[i].m_pGuid;
			pdoi->dwOfs    = m_FormatOfs[i] >= 0 ? (DWORD)m_FormatOfs[i] : c_M7Objects[i].m_dwStdOfs;
			pdoi->dwType   = M7ObjectType(i);
			pdoi->dwFlags  = c_M7Objects[i].m_nKind == M7_AXIS ? DIDOI_ASPECTPOSITION : 0;
			strncpy(pdoi->tszName, c_M7Objects[i].m_pName, sizeof(pdoi->tszName) - 1);
		}

		// An axis's value in the range the application set (the native one
		// unless it set another).
		LONG Scaled(int i, LONG nValue)
		{
			LONG nMin = c_M7Objects[i].m_nMin, nMax = c_M7Objects[i].m_nMax;

			if (m_Range[i][0] == nMin && m_Range[i][1] == nMax)
				return nValue;

			return m_Range[i][0] +
			       (LONG)(((double)(nValue - nMin) * (m_Range[i][1] - m_Range[i][0])) / (nMax - nMin));
		}

		// Read the pad and buffer what changed; FALSE when not acquired.
		BOOL Update()
		{
			M7PadState pad;
			LONG       values[M7_OBJECTS];
			DWORD      dwNow;
			int        i;

			if (!m_bAcquired)
				return FALSE;

			if ((m_dwCoop & DISCL_FOREGROUND) && !M7IsForeground(m_hWnd))
			{
				m_bAcquired = FALSE;
				m_bLost     = TRUE;
				m_nBuf      = 0;
				return FALSE;
			}

			M7PadRead_State(&pad);
			M7PadValues(pad, values);
			dwNow = GetTickCount();

			for (i = 0; i < M7_OBJECTS; i++)
			{
				if (values[i] == m_Value[i])
					continue;

				m_Value[i] = values[i];

				if (m_FormatOfs[i] < 0 || !m_dwBufSize)
					continue;

				if (m_nBuf >= m_dwBufSize)
				{
					m_bOverflow = TRUE;
					continue;
				}

				m_Buf[m_nBuf].dwOfs       = (DWORD)m_FormatOfs[i];
				m_Buf[m_nBuf].dwData      = c_M7Objects[i].m_nKind == M7_AXIS ? (DWORD)Scaled(i, values[i])
				                                                              : (DWORD)values[i];
				m_Buf[m_nBuf].dwTimeStamp = dwNow;
				m_Buf[m_nBuf].dwSequence  = ++m_dwSequence;
				m_nBuf++;
			}

			return TRUE;
		}

		ULONG				m_nRef;
		HWND				m_hWnd;
		DWORD				m_dwCoop;
		BOOL				m_bAcquired;
		BOOL				m_bLost;
		BOOL				m_bFormat;
		int					m_FormatOfs[M7_OBJECTS];	// -1: not in the data format
		LONG				m_Value[M7_OBJECTS];		// as last buffered, native range
		LONG				m_Range[M7_OBJECTS][2];
		DWORD				m_dwBufSize;
		DIDEVICEOBJECTDATA	m_Buf[M7_BUFFER];
		DWORD				m_nBuf;
		BOOL				m_bOverflow;
		DWORD				m_dwSequence;
	};
}

// Is there a ##gamepad: XInput loaded (or the test pad is on).  Decided
// once, at input_Init.
inline BOOL P7Pad_Available(void)
{
	return M7PadRead_Available() ? TRUE : FALSE;
}

// ##gamepad's DIDEVICEINSTANCE, as EnumDevices would give it: a joystick
// of the gamepad subtype.
inline void P7Pad_GetInstance(DIDEVICEINSTANCEA* pInstance)
{
	M7GetInstance(pInstance);
}

// Is this ##gamepad's instance GUID?
inline BOOL P7Pad_IsPad(REFGUID guidInstance)
{
	return IsEqualGUID(guidInstance, c_guidM7Pad) ? TRUE : FALSE;
}

// CreateDevice for any device: ours for ##gamepad's GUID, DirectInput's
// for every other.
inline HRESULT P7Pad_CreateDevice(LPDIRECTINPUTA pDI, REFGUID guid, LPDIRECTINPUTDEVICEA* ppDevice)
{
	if (P7Pad_IsPad(guid))
	{
		*ppDevice = (LPDIRECTINPUTDEVICEA)(IDirectInputDevice2A*)new CM7PadDevice;
		return *ppDevice ? DI_OK : DIERR_OUTOFMEMORY;
	}

	return pDI->CreateDevice(guid, ppDevice, NULL);
}

#endif // __P7_GAMEPAD_H__
