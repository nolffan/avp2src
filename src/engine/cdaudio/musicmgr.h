// musicmgr.h -- the engine's music-driver interface, as cdaudio.dll
// (2002-05-07) fills it in.
//
// The shape is the LithTech music manager's: NOLF2's engine release has
// it as runtime/shared/src/musicmgr.h (by way of nolf2src), and this is
// that header brought back to the engine cdaudio.dll was built for. The
// function table is the same 22 entries plus the console print the engine
// supplies (MusicDLLSetup at 0x100010f0 fills 0x00..0x54; the engine's
// loader sets 0x58, lithtech.exe 0x00454fb0); the fields after it are
// 32-bit, not NOLF2's bools: Init (0x10001230) clears m_bValid at 0x5c,
// sets m_bEnabled at 0x60 and m_ulCaps at 0x6c with whole-dword stores,
// and reads m_hWnd and m_hInstance at 0x64 and 0x68.

#ifndef __MUSICMGR_H__
#define __MUSICMGR_H__

#ifndef __LTBASETYPES_H__
#include "ltbasetypes.h"
#endif


// Caps bits for music.
enum _music_caps
{
	MUSIC_VOLUMECONTROL = 0x0001,   // Volume control allowed.
	MUSIC_INSTRUMENTSET = 0x0002,   // Instrument set supported.
	MUSIC_MOTIFSAVAIL   = 0x0004    // Motifs available.
};


// Function interface for playing music through the music DLL.
struct SMusicMgr
{
	// Initialize the music engine...
	LTBOOL  (*Init)(SMusicMgr *pMusicMgr);

	// Terminate the music engine...
	void    (*Term)();

	// Set the data directory...
	LTBOOL  (*SetDataDirectory)(const char *szDataDir);

	// Set instrument set for music engine, must have caps for this...
	LTBOOL  (*InitInstruments)(const char *szDLSFile, const char *szStyleFile);

// Song access stuff...

	// Preload the music data for song...
	void *  (*CreateSong)(const char *pFileName);

	// Get a song pointer...
	void *  (*GetSong)(const char *pFileName);

	// Destroy the music data for a song...
	void    (*DestroySong)(void *pSong);

// Song play stuff...

	// Play preloaded music...
	LTBOOL  (*PlayBreak)(void *pSong, uint32 dwPlayBoundaryFlags);

	// Create a playlist...
	void *  (*CreatePlayList)(const char *szPlayListName);

	// Get a playlist...
	void *  (*GetPlayList)(const char *szPlayListName);

	// Add song to playlist...
	LTBOOL  (*AddSongToList)(void *pPlayList, void *pSong);

	// Get music data for a song in a playlist...
	void *  (*GetSongFromList)(void *pPlayList, unsigned long dwIndex);

	// Remove a playlist...
	void    (*RemoveList)(void *pPlayList);

	// Play the playlist.  bLoop=1 will play playlist forever.
	LTBOOL  (*PlayList)(void *pPlayList, void *pTransition, LTBOOL bLoop, uint32 dwPlayBoundaryFlags);

	// Play a motif on top of current song.  Must have caps for this...
	LTBOOL  (*PlayMotif)(const char *szMotifName, LTBOOL bLoop);

	// Stop a motif.  Must have caps for this...
	LTBOOL  (*StopMotif)(const char *szMotifName);

	// Destroy the music data for all songs...
	void    (*DestroyAllSongs)();

	// Stop the current song...
	void    (*Stop)(uint32 dwPlayBoundaryFlags);

	// Pause the current song...
	LTBOOL  (*Pause)(uint32 dwPlayBoundaryFlags);

	// Resume the current song...
	LTBOOL  (*Resume)();

	// Get the volume.  Must have caps for this.  100-full volume, 0-no volume...
	uint16  (*GetVolume)();

	// Set the volume.  Must have caps for this.  100-full volume, 0-no volume...
	void    (*SetVolume)(unsigned short vol);

	// App must provide a console print function, so music dll can send messages to console.
	void    (*ConsolePrint)(char *pMsg, ...);

	// Music dll setup correctly.  Set by music dll.  0-not valid, !0-valid.
	LTBOOL  m_bValid;

	// Music enabled.  Set by dll to 1.  App can set to 0 or 1.
	LTBOOL  m_bEnabled;

	// Handle to window playing music.  Set by app.
	void *  m_hWnd;

	// Handle to Instance of app.  Set by app.
	void *  m_hInstance;

	// Capability bits.  Set by music dll.
	uint32  m_ulCaps;
};
typedef SMusicMgr *LPSMusicMgr;


// A music DLL exports "MusicDLLSetup", which fills in every function
// pointer of the structure.
typedef void (*MusicDLLSetupFn)(SMusicMgr *pMusicMgr);


#endif // __MUSICMGR_H__
