/*******************************************************************************
;
;	MODULE:			PLAYSOUND (.H)
;
;	PURPOSE:		Helper function to play a WAVE resource
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/

#ifndef _PLAYSOUND_H_
#define _PLAYSOUND_H_

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	PlaySound
//
//	PURPOSE:	Helper function to play a sound (AVP2.exe 0x404b70: one copy,
//				the linker folds the others into it)
//
// ----------------------------------------------------------------------- //

static void PlaySound(LPCTSTR lpszSound)
{
	HRSRC hRes; // resource handle to wave file
	HGLOBAL hData;

	if ((hRes = ::FindResource(AfxGetResourceHandle(), lpszSound,
	  _T("WAVE"))) != NULL &&
	  (hData = ::LoadResource(AfxGetResourceHandle(), hRes)) != NULL)
	{
		// found the resource, play it
		sndPlaySound((LPCTSTR)::LockResource(hData),
			SND_MEMORY|SND_ASYNC|SND_NODEFAULT);
	}
}

inline static void PlaySound(UINT nIDS)
	{ PlaySound(MAKEINTRESOURCE(nIDS)); }

#endif
