/****************************************************************************
;
;	MODULE:		REDMGR (.CPP)
;
;	PURPOSE:	Red Book (CD audio) playback through MCI
;
;	NOTICE:		Reconstructed for this project from the release's
;				prebuilt Redmgr.lib (libs/release/, cl 12.00.8966),
;				against its header (src/LT2/lithshared/redmgr/redmgr.h).
;				The library ships no source; the class, its members
;				and its error strings are the header's and the
;				object's own.
;
****************************************************************************/


// Includes...

#include "stdafx.h"
#include "redmgr.h"


// OutMCIErr: the release build's is empty (REDMGR.OBJ: a bare `ret 8`).
// Every failure in the class reports through it with the failing call's
// name, which only a debug build would have shown.

void CRedMgr::OutMCIErr(MCIERROR nErr, char* sOut)
{
}


// Init: finds the auxiliary device that is the CD's own output (for the
// volume), and opens the CD audio device, shared if it will not open
// alone, in tracks/minutes/seconds/frames.

BOOL CRedMgr::Init(HWND hWnd)
{
	AUXCAPS auxCaps;

	m_bOutputDevIDFound = FALSE;

	UINT nNumDevs = auxGetNumDevs();
	for (UINT i = 0; i < nNumDevs; i++)
	{
		if (auxGetDevCaps(i, &auxCaps, sizeof(auxCaps)) == MMSYSERR_NOERROR && auxCaps.wTechnology == AUXCAPS_CDAUDIO)
		{
			m_iOutputDevID = i;
			m_bOutputDevIDFound = TRUE;
			break;
		}
	}

	if (m_bOutputDevIDFound)
	{
		if (auxGetVolume(m_iOutputDevID, &m_dwOriginalVolume) != MMSYSERR_NOERROR) return FALSE;
	}

	m_hWnd = hWnd;

	MCIOpen.lpstrDeviceType  = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
	MCIOpen.lpstrElementName = NULL;
	MCIOpen.lpstrAlias       = NULL;
	MCIOpen.dwCallback       = 0;

	if (mciSendCommand(NULL, MCI_OPEN, MCI_WAIT | MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD)&MCIOpen))
	{
		MCIERROR nErr = mciSendCommand(NULL, MCI_OPEN, MCI_WAIT | MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE, (DWORD)&MCIOpen);
		if (nErr)
		{
			OutMCIErr(nErr, "MCI_OPEN");
			return FALSE;
		}
	}

	MCISet.dwTimeFormat = MCI_FORMAT_TMSF;
	MCISet.dwCallback   = 0;
	MCISet.dwAudio      = 0;

	MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_SET, MCI_WAIT | MCI_SET_TIME_FORMAT, (DWORD)&MCISet);
	if (nErr)
	{
		OutMCIErr(nErr, "CRedMgr::Init MCI_SET");
		mciSendCommand(MCIOpen.wDeviceID, MCI_CLOSE, 0, NULL);
		return FALSE;
	}

	m_posPause = 0;
	m_bIsInited = TRUE;
	return TRUE;
}


// PlayTrack: the whole of one track.

BOOL CRedMgr::PlayTrack(int nTrack)
{
	if (!MediaPresent()) return FALSE;

	int nNumTracks = GetNumTracks();
	if (nTrack > nNumTracks) return FALSE;

	UINT32 nLen = GetTrackLen(nTrack);
	return Play(nTrack, 0, 0, 0, nTrack, MCI_MSF_MINUTE(nLen), MCI_MSF_SECOND(nLen), MCI_MSF_FRAME(nLen));
}


// Play: from one TMSF position to another (to the start of the next track
// when no end track is given); MCI notifies m_hWnd when it is done.

BOOL CRedMgr::Play(int nFromTrack, int nFromMin, int nFromSec, int nFromFrame,
				   int nToTrack, int nToMin, int nToSec, int nToFrame)
{
	if (!m_bIsInited) return FALSE;

	if (nToTrack == 0) nToTrack = nFromTrack + 1;

	MCIPlay.dwFrom     = MCI_MAKE_TMSF(nFromTrack, nFromMin, nFromSec, nFromFrame);
	MCIPlay.dwTo       = MCI_MAKE_TMSF(nToTrack, nToMin, nToSec, nToFrame);
	MCIPlay.dwCallback = (DWORD)m_hWnd;

	MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)&MCIPlay);
	if (nErr == 0) return TRUE;

	OutMCIErr(nErr, "CRedMgr::Play MCI_PLAY");
	return FALSE;
}


// PlayToEnd: from a TMSF position to the end of the disc.

BOOL CRedMgr::PlayToEnd(int nFromTrack, int nFromMin, int nFromSec, int nFromFrame)
{
	if (!m_bIsInited) return FALSE;

	MCIPlay.dwFrom     = MCI_MAKE_TMSF(nFromTrack, nFromMin, nFromSec, nFromFrame);
	MCIPlay.dwCallback = (DWORD)m_hWnd;

	MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM, (DWORD)&MCIPlay);
	if (nErr == 0) return TRUE;

	OutMCIErr(nErr, "CRedMgr::Play MCI_PLAY");
	return FALSE;
}


BOOL CRedMgr::Stop()
{
	if (!m_bIsInited) return FALSE;

	MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_STOP, MCI_WAIT, NULL);
	if (nErr == 0) return TRUE;

	OutMCIErr(nErr, "CRedMgr::Stop MCI_STOP");
	return FALSE;
}


// Term: puts the CD's volume back as Init found it, and closes the device.

void CRedMgr::Term(BOOL bStopIfPlaying)
{
	if (!m_bIsInited) return;

	if (m_bOutputDevIDFound) auxSetVolume(m_iOutputDevID, m_dwOriginalVolume);

	if (bStopIfPlaying && IsPlaying()) Stop();

	MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_CLOSE, 0, NULL);
	if (nErr) OutMCIErr(nErr, "CRedMgr::Term MCI_CLOSE");

	m_bOutputDevIDFound = FALSE;
	m_bIsInited = FALSE;
	m_hWnd = NULL;
}


UINT32 CRedMgr::GetPosition()
{
	if (!m_bIsInited) return 0;

	MCI_STATUS_PARMS mciStatus;
	mciStatus.dwItem = MCI_STATUS_POSITION;

	MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&mciStatus);
	if (nErr == 0) return mciStatus.dwReturn;

	OutMCIErr(nErr, "CRedMgr::GetPosition MCI_STATUS");
	return (UINT32)-1;
}


UINT32 CRedMgr::GetNumTracks()
{
	if (!m_bIsInited) return 0;

	MCI_STATUS_PARMS mciStatus;
	mciStatus.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;

	MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&mciStatus);
	if (nErr == 0) return mciStatus.dwReturn;

	OutMCIErr(nErr, "CRedMgr::GetNumTracks MCI_STATUS");
	return (UINT32)-1;
}


UINT32 CRedMgr::GetTrackLen(int nTrack)
{
	if (!m_bIsInited) return 0;

	MCI_STATUS_PARMS mciStatus;
	mciStatus.dwItem  = MCI_STATUS_LENGTH;
	mciStatus.dwTrack = nTrack;

	MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&mciStatus);
	if (nErr == 0) return mciStatus.dwReturn;

	OutMCIErr(nErr, "CRedMgr::GetTrackLen MCI_STATUS");
	return (UINT32)-1;
}


UINT32 CRedMgr::GetTrackLenSeconds(int nTrack)
{
	UINT32 nLen = GetTrackLen(nTrack);
	if (nLen == (UINT32)-1) return 0;

	return MCI_MSF_MINUTE(nLen) * 60 + MCI_MSF_SECOND(nLen);
}


BOOL CRedMgr::MediaPresent()
{
	if (m_bIsInited)
	{
		MCI_STATUS_PARMS mciStatus;
		mciStatus.dwItem = MCI_STATUS_MEDIA_PRESENT;

		MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&mciStatus);
		if (nErr == 0) return mciStatus.dwReturn;

		OutMCIErr(nErr, "CRedMgr::MediaPresent MCI_STATUS");
	}

	return FALSE;
}


UINT32 CRedMgr::GetStatus()
{
	if (m_bIsInited)
	{
		MCI_STATUS_PARMS mciStatus;
		mciStatus.dwItem = MCI_STATUS_MODE;

		MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&mciStatus);
		if (nErr == 0) return mciStatus.dwReturn;

		OutMCIErr(nErr, "CRedMgr::GetStatus MCI_STATUS_MODE");
	}

	return 0;
}


// Pause: MCI's CD audio has no pause of its own, so the position is kept
// and the play stopped; Resume plays on from it.

BOOL CRedMgr::Pause()
{
	if (!m_bIsInited) return FALSE;
	if (!IsPlaying()) return FALSE;

	m_posPause = GetPosition();
	if (!Stop()) m_posPause = 0;

	return TRUE;
}


BOOL CRedMgr::Resume(int nTrack)
{
	if (!m_bIsInited || !IsPaused()) return FALSE;

	if (IsPlaying())
	{
		ClearPause();
		return FALSE;
	}

	if (nTrack)
	{
		UINT32 nLen = GetTrackLen(nTrack);

		MCIPlay.dwFrom     = m_posPause;
		MCIPlay.dwTo       = MCI_MAKE_TMSF(nTrack, MCI_MSF_MINUTE(nLen), MCI_MSF_SECOND(nLen), MCI_MSF_FRAME(nLen));
		MCIPlay.dwCallback = (DWORD)m_hWnd;
		ClearPause();

		MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)&MCIPlay);
		if (nErr == 0) return TRUE;

		OutMCIErr(nErr, "CRedMgr::Play MCI_PLAY");
		return FALSE;
	}
	else
	{
		MCIPlay.dwFrom     = m_posPause;
		MCIPlay.dwCallback = (DWORD)m_hWnd;
		ClearPause();

		MCIERROR nErr = mciSendCommand(MCIOpen.wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM, (DWORD)&MCIPlay);
		if (nErr == 0) return TRUE;

		OutMCIErr(nErr, "CRedMgr::Play MCI_PLAY");
		return FALSE;
	}
}


// The volume, 0 to 100, of the CD's auxiliary output (its left channel;
// SetVolume sets both). Full volume when Init found no such device.

UINT32 CRedMgr::GetVolume()
{
	if (!m_bOutputDevIDFound) return 100;

	DWORD dwVolume;
	auxGetVolume(m_iOutputDevID, &dwVolume);

	return ((dwVolume & 0xFFFF) * 100) / 0xFFFF;
}


void CRedMgr::SetVolume(UINT32 iVolume)
{
	if (!m_bOutputDevIDFound) return;

	if (iVolume > 100) iVolume = 100;

	DWORD dwVolume = (iVolume * 0xFFFF) / 100;
	auxSetVolume(m_iOutputDevID, (dwVolume << 16) + dwVolume);
}
