// *********************************************************************** //
//
//  MODULE:   CLIENTMGR (.H)
//
//  PURPOSE:  CClientMgr — the client's counterpart to CServerMgr, and the
//            one global every ci_* interface function starts from.
//
//  Almost every function in the client interface table opens the same way:
//
//      mov  0x004b17e0,%ecx        ; g_pClientMgr
//      mov  0x150c(%ecx),%edx      ; its shell object
//      test %edx,%edx
//
//  so the global and the field at 0x150c are the first two things the
//  reconstruction needs. Both are named from what the functions do with
//  them, not from any header: the 0x150c object is what IsConnected() tests
//  for null, which makes it the connection to a server.
//
// *********************************************************************** //

#ifndef __CLIENTMGR_H__
#define __CLIENTMGR_H__

#include "ltbasedefs.h"
#include "iltclient.h"
#include "iclientshell.h"
#include "iltsoundmgr.h"

#include "servermgr.h"     // CNetMgr, ConsoleState — the engine core is shared


// The command table's row length. Proven by the stride ci_IsCommandOn walks
// with - the row index is multiplied by 255 - and by the bound it rejects at.
#define MAX_COMMANDS 255

// The field of view either angle is clamped to: a hundredth of pi, and a
// full turn less that much.
#define MIN_FOV (MATH_PI / 100.0f)
#define MAX_FOV ((MATH_PI * 2.0f) - MIN_FOV)

// The object at CClientMgr+0x150c. Only the fields the interface functions
// have needed so far are known.
class CClientConnection
{
public:
	// Slot zero is the disconnect, called with 1 and followed by the manager
	// dropping its pointer.
	virtual void Disconnect(int nReason)=0;  // vtable 0x00

	uint8  m_pad_04[0x18 - 0x04];
	float  m_fGameTime;                      // 0x018  GetGameTime
	float  m_fFrameTime;                     // 0x01c  GetGameFrameTime
	uint8  m_pad_20[0x38 - 0x20];
	uint16 m_nLocalID;                       // 0x038  GetLocalClientID
	uint8  m_pad_3a[0x84 - 0x3a];
	int    m_nGameMode;                      // 0x084  GetGameMode
};


// The input object at CClientMgr+0x1508 is a *table of plain function
// pointers*, not a class with a vtable: every call goes through
// `call *0x34(%eax)` with no vtable load, and the caller cleans the stack, so
// the entries are cdecl. They are not uniform about the receiver either —
// EnableDevice and StartDeviceTrack are handed the table itself as their
// first argument and the rest are not, which is why each field carries the
// signature its call site implies rather than a shared one.
class CClientInput
{
public:
	uint8 m_pad_00[0x10];
	void     (*PlayJoystickEffect)(CClientInput* pInput, char* pEffectName,
	                               float x, float y);                      // 0x10
	uint8 m_pad_14[0x24 - 0x14];
	LTBOOL   (*EnableDevice)(CClientInput* pInput, char* strDeviceName);   // 0x24
	uint8 m_pad_28[0x34 - 0x28];
	DeviceBinding* (*GetDeviceBindings)(uint32 nDevice);                   // 0x34
	void     (*FreeDeviceBindings)(DeviceBinding* pBindings);              // 0x38
	LTBOOL   (*StartDeviceTrack)(CClientInput* pInput, uint32 nDevices,
	                             uint32 nBufferSize);                      // 0x3c
	LTBOOL   (*TrackDevice)(DeviceInput* pInputArray, uint32* pnInOut);     // 0x40
	LTBOOL   (*EndDeviceTrack)();                                          // 0x44
	DeviceObject* (*GetDeviceObjects)(uint32 nDeviceFlags);                // 0x48
	void     (*FreeDeviceObjects)(DeviceObject* pList);                    // 0x4c
	LTBOOL   (*GetDeviceName)(uint32 nDeviceType, char* pStrBuffer,
	                          uint32 nBufferSize);                         // 0x50
	LTBOOL   (*IsDeviceEnabled)(char* strDeviceName);                      // 0x54
};


// The music manager at CClientMgr+0xf84 is a function-pointer table too —
// `jmp *0x40(%eax)` off the object with no vtable load. The field at 0x5c is
// the loaded music DLL: every entry point in the interface refuses when it is
// null, which is what named it. The slot names come from the interface
// function that reaches each one, since NOLF1's music interface does not
// share the NOLF2-era iltdirectmusic.h vtable order.
class CClientMusicMgr
{
public:
	uint8 m_pad_00[0x8];
	LTBOOL (*SetDirectory)(char* szDirectory);                             // 0x08
	LTBOOL (*InitInstruments)(char* szDLSFile, char* szStyleFile);         // 0x0c
	// Both of these take a song name and both get tried in turn by LoadSong,
	// by PlayBreak and by the play-list entries; which one is the loader and
	// which the lookup is not decidable from the call sites, so they keep
	// their offsets for names.
	// Both take a song name and both return the song, not a flag: PlayBreak
	// keeps what comes back and hands it to the entry at 0x1c. Which of the
	// two is the loader and which the lookup is not decidable from the call
	// sites, so they keep their offsets for names.
	void*  (*Fn_10)(char* szSong);                                         // 0x10
	void*  (*Fn_14)(char* szSong);                                         // 0x14
	uint8 m_pad_18[0x1c - 0x18];
	LTBOOL (*Fn_1c)(void* pSong, uint32 dwBoundaryFlags);                  // 0x1c
	// The play lists have the same pair-of-lookups shape the songs do, at
	// 0x20 and 0x24, and every caller tries 0x24 first.
	void*  (*Fn_20)(char* szPlayList);                                     // 0x20
	void*  (*Fn_24)(char* szPlayList);                                     // 0x24
	LTBOOL (*Fn_28)(void* pPlayList, void* pSong);                         // 0x28
	uint8 m_pad_2c[0x30 - 0x2c];
	void   (*Fn_30)(void* pPlayList);                                      // 0x30
	LTBOOL (*Fn_34)(void* pPlayList, void* pTransition, LTBOOL bLoop,
	                uint32 dwBoundaryFlags);                               // 0x34
	LTBOOL (*PlayMotif)(char* szMotif, LTBOOL bLoop);                      // 0x38
	void   (*StopMotif)(char* szMotif);                                    // 0x3c
	void   (*DestroyAllSongs)();                                           // 0x40
	void   (*Stop)(uint32 dwBoundaryFlags);                                // 0x44
	LTBOOL (*Pause)(uint32 dwBoundaryFlags);                               // 0x48
	LTBOOL (*UnPause)();                                                   // 0x4c
	// Bit 1 of the flags gates the volume pair.
	short  (*GetVolume)();                                                 // 0x50
	void   (*SetVolume)(short wVolume);                                    // 0x54
	uint8 m_pad_58[0x5c - 0x58];
	void*  m_pMusicDLL;                                                    // 0x5c
	uint8 m_pad_60[0x6c - 0x60];
	// Bit 2 gates the instrument calls and bit 4 the motif calls.
	uint32 m_nFlags;                                                       // 0x6c
};


class CClientMgr
{
public:
	uint8              m_pad_00[0x6c];
	uint8              m_World[0x508 - 0x6c];  // 0x06c  the client's world,
	                                           //        passed whole to the
	                                           //        intersector and to the
	                                           //        physics mover
	// What the physics mover wants beside the world; the server manager keeps
	// the same thing at 0x6a8.
	class CUnknown6A8Obj* m_pUnknown508;      // 0x508
	// The same CNetMgr the server has, as a sub-object rather than behind a
	// pointer: the service-list calls are a thiscall on g_pClientMgr+0x50c.
	CNetMgr            m_NetMgr;              // 0x50c
	uint8              m_pad_610[0x61c - (0x50c + 0x104)];
	// The game DLL's own interface, called back into from the middle of the
	// engine's work: ci_PlaySound reports every sound it starts.
	IClientShell*      m_pClientShell;        // 0x61c
	uint8              m_pad_620[0x768 - 0x620];
	LTVector           m_vGlobalLightScale;   // 0x768
	// The client's ILTSoundMgr lives inside the manager, not behind a
	// pointer: ci_GetSoundMgr is `g_pClientMgr + 0x774`, and the two calls
	// through it land on vtable slots 0x5c and 0x60, which are ILTSoundMgr's
	// PauseSounds and ResumeSounds — counted across iltsoundmgr.h, whose
	// ILTClientSoundMgr continues ILTSoundMgr's vtable. Its own layout is not
	// known yet, so only the vptr is modelled.
	uint8              m_SoundMgr[4];         // 0x774
	uint8              m_pad_778[0xf84 - 0x778];
	// The music manager is another sub-object. NOLF1's ILTDirectMusicMgr does
	// not share the NOLF2-era iltdirectmusic.h's vtable order, so only the
	// vptr is modelled here too.
	uint8              m_MusicMgr[4];         // 0xf84  see CClientMusicMgr
	uint8              m_pad_f88[0x11a0 - 0xf88];
	SkyDef             m_SkyDef;              // 0x11a0  forty-eight bytes, copied
	                                          //         out whole by GetSkyDef
	uint8              m_pad_11d0[0x12ac - (0x11a0 + 48)];
	// Two rows of command state - the index that follows leaves room for
	// exactly two - with the live row selected by that index.
	char               m_CommandOn[2][MAX_COMMANDS];  // 0x12ac
	uint8              m_pad_14aa[0x14ac - (0x12ac + 2 * MAX_COMMANDS)];
	int                m_nCurCommandSet;      // 0x14ac
	uint8              m_pad_14b0[0x14fc - 0x14b0];
	LTFLOAT            m_fAxisOffsets[3];     // 0x14fc
	CClientInput*      m_pInput;              // 0x1508
	CClientConnection* m_pConnection;         // 0x150c
	uint8              m_pad_1510[0x151c - 0x1510];
	LTBOOL             m_bInputEnabled;       // 0x151c  SetInputState
	LTBOOL             m_bTrackingDevices;    // 0x1520  set by StartDeviceTrack,
	                                          //         cleared by EndDeviceTrack,
	                                          //         and required by TrackDevice
	ModelHookFn        m_ModelHookFn;         // 0x1524
	void*              m_pModelHookUser;      // 0x1528
	uint8              m_pad_152c[0x1534 - 0x152c];
	// A pointer, where the server holds its file manager inline.
	CFileMgr*          m_pFileMgr;            // 0x1534

	LTRESULT ClearInput();                    // 0x00410900
	LTRESULT StartGame(StartGameRequest* pRequest);   // 0x0040fd90
	LTRESULT InitMusic(char* szMusicDLL);             // 0x00410380
	void     FlipScreen(uint32 flags);                // 0x00410930
};


extern CClientMgr* g_pClientMgr;             // 0x004b17e0

// The shutdown reason the parameterless Shutdown() hands on: a mutable
// .data buffer, not a literal, so something else fills it in.
// The screen size and render-up flag (0x004b4394 / 0x004b4398 / 0x004b439c)
// are not globals of their own: they are g_RenderStruct's m_Width / m_Height
// / m_bInited (+0x3c / +0x40 / +0x44) -- see the RenderStruct view below.
// The renderer entry points the interface calls (0x004b43dc ClearScreen ..
// 0x004b4400 SetOptimized2DColor, 0x004b4444 BlitToScreen) and the pan info
// (0x004b4460) are not globals of their own: they are members of
// clientrender.cpp's g_RenderStruct (0x004b4358), +0x84.. -- see the
// RenderStruct view below, which carries them at those offsets.

// Set by a console variable; forces the visual clear on every ClearScreen.
extern int g_CV_ForceClear;                                     // 0x004b328c  "ForceClear" (server/dsi.cpp)

extern char g_ShutdownMessage[];             // 0x004b0ab4

void sub_42e6a0(char* pMsg);                 // 0x0042e6a0
HLOCALOBJ sub_47bcd0();                      // 0x0047bcd0
void sub_40ad40(HLOCALOBJ hObj, float* r, float* g, float* b, int nUnused);  // 0x0040ad40
LTBOOL sub_4305f0(ClientIntersectQuery* pQuery, ClientIntersectInfo* pInfo,
                  void* pWorld, int nUnused);                                // 0x004305f0
void sub_420930(CClientMgr* pMgr, HLOCALOBJ hObj, LTVector* pPos,
                LTRotation* pRotation);                                      // 0x00420930
void sub_420880(CClientMgr* pMgr, HLOCALOBJ hObj, LTRotation* pRotation);    // 0x00420880
LTBOOL sub_460bf0(HLOCALOBJ hObj, uint32 width, uint32 height,
                  LTBOOL bHalfTriangles);                                    // 0x00460bf0

// The current render mode, handed out whole by GetRenderMode.
extern RMode g_CurRenderMode;                // 0x004b4140

// The client's own console state. The server has one too; the config-file
// calls are con_RunFile / con_SaveFile against this one.
extern ConsoleState g_ClientConsoleState;    // 0x004b2f00

// The screen surface and the flag that says whether it is valid. The getter
// hands back one or null with `neg`/`sbb`/`and`, which is a ternary and not
// a branch.
class RenderStruct;
// cissurf.cpp owns the pointer and spells it with its own class name; the
// same name here, derived from this header's view below, so the references
// link to that definition and still read the view's members.
class RenderStructQ40C;
extern RenderStructQ40C* g_pRender;          // 0x004b17a4
// NOT an int, despite the type here: every use is `cmp $0x4b1620,%reg` on a
// POINTER, so 0x004b1620 is the screen's CSurface OBJECT, and that one
// comparison is what selects the accelerated path in all three blit workers.
// CSurface+0x18 is the pitch, which this header's padding currently covers.
// Declared with cissurf.cpp's type (that file owns the definition); only
// its address is taken here.
class CSurfaceQ40C;
extern CSurfaceQ40C g_ScreenSurface;         // 0x004b1620


// A client-side object, as far as the flag accessor needs it. The client's
// object layout is not the server's LTObject — the flags are a uint16 here.
class CClientObject
{
public:
	uint8    m_pad_00[0x90];
	uint32   m_UserFlags;                    // 0x090
	uint8    m_ColorR;                       // 0x094  the four colour bytes,
	uint8    m_ColorG;                       // 0x095  each a 0-255 scaling of
	uint8    m_ColorB;                       // 0x096  the float the interface
	uint8    m_ColorA;                       // 0x097  takes
	uint8    m_pad_98[0x9c - 0x98];
	LTRotation m_Rotation;                   // 0x09c  four words, copied out whole
	LTVector m_Scale;                        // 0x0ac
	// The server object this one mirrors, or 0xffff for a client-side object.
	// Only a client-side one may have its user flags set.
	uint16   m_nServerID;                    // 0x0b8
	uint8    m_pad_ba[0xbc - 0xba];
	char     m_ObjectType;                   // 0x0bc  OT_*, as on the server
	uint8    m_pad_bd[0xc0 - 0xbd];
	void*    m_pUserData;                    // 0x0c0  the game's, untouched
	LTVector m_Velocity;                     // 0x0c4
	LTVector m_Acceleration;                 // 0x0d0
	uint8    m_pad_dc[0x120 - 0xdc];
	uint32   m_ChangeFlags;                  // 0x120  same bits the server uses
	uint8    m_pad_124[0x180 - 0x124];
	uint16   m_ClientFlags;                  // 0x180
	uint8    m_pad_182[0x18c - 0x182];
	LTVector m_Pos;                          // 0x18c
	uint8    m_pad_198[0x1bc - 0x198];
	HPOLY    m_hClipPoly;                    // 0x1bc  ClipSprite's only effect
};

// The camera's own fields start where the sprite's clip poly sits, so they
// only make sense once the object type byte has been checked.
class CClientCamera
{
public:
	uint8    m_pad_00[0x1a8];
	LTRect   m_Rect;                         // 0x1a8
	LTFLOAT  m_fFovX;                        // 0x1b8
	LTFLOAT  m_fFovY;                        // 0x1bc
	LTBOOL   m_bFullScreen;                  // 0x1c0
	LTVector m_LightAdd;                     // 0x1c4
};


// The engine's particle is the SDK's LTParticle with its position on the end:
// GetParticlePos reads +0x24, which is one dword past m_pNext.
// LTParticle ends at 0x24 with the m_pNext the game walks; the engine keeps
// the matching back pointer, the position and the two lifetimes past it.
class CClientParticle : public LTParticle
{
public:
	LTVector         m_Pos;                  // 0x024
	LTFLOAT          m_Lifetime;             // 0x030
	LTFLOAT          m_TotalLifetime;        // 0x034
	CClientParticle* m_pPrev;                // 0x038
};

// Each object type lays its own fields out from 0x1a8; a light keeps one
// float there, a camera a rect, a particle system its list.
class CClientLight
{
public:
	uint8   m_pad_00[0x1a8];
	LTFLOAT m_fLightRadius;                  // 0x1a8
};

// A line in a line system, linked into the live list through the pair at 0x3c.
// A removed one goes back to the system's StructBank, which chains free
// records through their first word.
class CClientLine
{
public:
	uint8        m_pad_00[0x3c];
	CClientLine* m_pPrev;                    // 0x03c
	CClientLine* m_pNext;                    // 0x040
};

class CClientLineSystem
{
public:
	uint8        m_pad_00[0x1a8];
	StructBank*  m_pLineBank;                // 0x1a8
	uint8        m_pad_1ac[0x1b0 - 0x1ac];
	// The end marker is a node inside the system, so a walk stops when the
	// next pointer reaches this address rather than when it reaches null -
	// and the list's head is this node's own m_pNext, at 0x1f0.
	CClientLine  m_LineTail;                 // 0x1b0
};

// The client's model object keeps the loaded Model behind a pointer at 0x1dc,
// where the server's ModelInstance is the LTObject itself.
class CClientModel
{
public:
	uint8  m_pad_00[0x1dc];
	Model* m_pModel;                         // 0x1dc
};

// One file the object creator has to resolve: a type, the name, and a word
// the creator fills in. Only type 1 is ever passed from the interface.
class CClientFileRef
{
public:
	CClientFileRef() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// What CreateObject builds on the stack: the object's own file, one reference
// per skin, and the create struct itself. The object's file is a member of its
// own rather than element zero of one array - the shipped code constructs it
// separately and then loops over exactly the skins.
class CClientCreateRequest
{
public:
	CClientFileRef      m_ObjectFile;                        // 0x00
	CClientFileRef      m_SkinFiles[MAX_MODEL_TEXTURES];     // 0x0c
	ObjectCreateStruct* m_pStruct;                           // 0x24
};

class CClientPolyGrid
{
public:
	uint8   m_pad_00[0x1a8];
	char*   m_pBytes;                        // 0x1a8
	uint8   m_pad_1ac[0x1b0 - 0x1ac];
	void*   m_pTexture;                      // 0x1b0
	uint32  m_Unknown1b4;                    // 0x1b4  address taken, contents
	                                         //        not reached from here
	uint8   m_pad_1b8[0x1cc - 0x1b8];
	LTFLOAT m_fXPan;                         // 0x1cc
	LTFLOAT m_fYPan;                         // 0x1d0
	LTFLOAT m_fXScale;                       // 0x1d4
	LTFLOAT m_fYScale;                       // 0x1d8
	uint8   m_pad_1dc[0x1f0 - 0x1dc];
	uint32  m_nWidth;                        // 0x1f0
	uint32  m_nHeight;                       // 0x1f4
	// Embedded in the grid, not allocated: GetPolyGridInfo hands out this
	// address. How many entries there are is not visible from here.
	PGColor m_ColorTable[1];                 // 0x1f8
};

class CClientParticleSystem
{
public:
	uint8       m_pad_00[0x1a8];
	// A node embedded in the system itself, handed out as the list's end
	// marker rather than as a particle of its own. Its own m_pNext, at
	// 0x1c8, is the head of the list - and the bank pointer that follows it
	// lands at 0x1e4 exactly, which is what fixes CClientParticle at 0x3c
	// bytes.
	CClientParticle m_ParticleTail;             // 0x1a8
	StructBank* m_pParticleBank;                // 0x1e4
	uint8       m_pad_1e8[0x1ec - 0x1e8];
	// The colour the software renderer draws the system with, three bytes
	// where the interface takes three floats.
	uint8       m_SoftwareR;                    // 0x1ec
	uint8       m_SoftwareG;                    // 0x1ed
	uint8       m_SoftwareB;                    // 0x1ee
	uint8       m_pad_1ef[0x228 - 0x1ef];
	uint32      m_nParticles;                   // 0x228
	uint8       m_pad_22c[0x248 - 0x22c];
	LTFLOAT     m_fGravityAccel;                // 0x248
	LTFLOAT     m_fParticleRadius;              // 0x24c
	uint8       m_pad_250[0x258 - 0x250];
	uint32      m_nPSFlags;                     // 0x258
};


// A surface, as far as the two user-data accessors need it. The engine's own
// surface class is not in the SDK; all these two functions know is that the
// game's pointer sits at +0x24 and that a null handle is tolerated.
// Two of these, indexed by the interface's own index; the first word is not
// touched by SetGlobalPanInfo.  RenderStruct+0x108 (0x004b4460).
class PanInfo
{
public:
	uint32  m_nUnknown00;                    // 0x00
	LTFLOAT m_xOffset;                       // 0x04
	LTFLOAT m_zOffset;                       // 0x08
	LTFLOAT m_xScale;                        // 0x0c
	LTFLOAT m_zScale;                        // 0x10
};

class CIBlitRequest;

// The renderer's own interface, null until the render DLL is up - which is
// what every LT_NOTINITIALIZED check in the interface actually tests.  The
// same struct as clientrender.cpp's g_RenderStruct (0x004b4358), which is
// where the function members are called through by name; the offsets are
// clientrender.cpp's table (d3d.ren fills +0x6c..+0xfc).
class RenderStruct
{
public:
	uint8  m_pad_00[0x3c];
	int    m_Width;                          // 0x03c  0x004b4394
	int    m_Height;                         // 0x040  0x004b4398
	union
	{
	uint32 m_nCanOptimizeSurfaces;           // 0x044
	int    m_bInited;                        // 0x044  0x004b439c, clientrender.cpp's name
	};
	uint8  m_pad_48[0x84 - 0x48];
	void   (*ClearScreen)(LTRect* pRect, uint32 dwFlags);                // 0x084  0x004b43dc
	void   (*Start3D)();                                                 // 0x088  0x004b43e0
	void   (*End3D)();                                                   // 0x08c  0x004b43e4
	LTBOOL (*IsIn3D)();                                                  // 0x090  0x004b43e8
	void   (*StartOptimized2D)();                                        // 0x094  0x004b43ec
	void   (*EndOptimized2D)();                                          // 0x098  0x004b43f0
	uint8  m_pad_9c[0xa0 - 0x9c];
	LTBOOL (*SetOptimized2DBlend)(LTSurfaceBlend blend);                 // 0x0a0  0x004b43f8
	uint8  m_pad_a4[0xa8 - 0xa4];
	LTBOOL (*SetOptimized2DColor)(HLTCOLOR hColor);                      // 0x0a8  0x004b4400
	uint8  m_pad_ac[0xdc - 0xac];
	LTBOOL (*OptimizeSurface)(void* pRenderData, HLTCOLOR hTransColor);  // 0x0dc
	void   (*UnoptimizeSurface)(void* pRenderData);                      // 0x0e0
	uint8  m_pad_e4[0xec - 0xe4];
	void   (*BlitToScreen)(CIBlitRequest* pRequest);                     // 0x0ec  0x004b4444
	LTBOOL (*WarpToScreen)(CIBlitRequest* pRequest);                     // 0x0f0  0x004b4448
	uint8  m_pad_f4[0x108 - 0xf4];
	PanInfo m_GlobalPanInfo[2];                                          // 0x108  0x004b4460
};

class RenderStructQ40C : public RenderStruct
{
};

class CSurface
{
public:
	uint8  m_pad_00[0xc];
	void*  m_pRenderData;                    // 0x00c  the renderer's, opaque
	uint32 m_nWidth;                         // 0x010
	uint32 m_nHeight;                        // 0x014
	uint8  m_pad_18[0x1c - 0x18];
	uint32 m_nFlags;                         // 0x01c  bit 1 optimized,
	                                         //        bit 2 cleared once the
	                                         //        renderer has taken it
	// The colour the renderer treats as see-through, or 0xffffffff for none.
	HLTCOLOR m_hTransColor;                  // 0x020
	void*  m_pUserData;                      // 0x024  the game's, untouched
	float  m_fAlpha;                         // 0x028
};

ILTClientSoundMgr* ci_GetSoundMgr();         // 0x0040f7f0
CClientMusicMgr*   ci_GetMusicMgr();         // 0x0040f800


// The window the engine drew into (0x004b0b04) and the flag ShutdownRender
// leaves set (0x004b0b30) are fields of clientwin.cpp's g_CluClientGlob:
// cluglob.h's m_hMainWnd and m_bRenderShutdown.


// The console's command lookup, in the shared console TU (name ours); only
// the client's address is known, because the server never calls it.
ConsoleCommand* con_FindCommand(ConsoleState* pState, char* pName);          // 0x0041b370

// The transparent blits all open the same way: an object with a vtable at
// 0x004a1ab4 and a type code at +4, constructed by 0x0040c0f0, is handed to a
// method of the client manager's sub-object at 0x1618 along with the colour
// and a second global. That method dispatches on the type code through a
// four-entry table of handlers nothing else references, which reads as a
// per-pixel-format conversion of the transparent colour - but the module it
// belongs to has not been reconstructed, so all three keep their addresses
// for names.
//
// Since placed: the sub-object is fmconvert.cpp's FMConvertMgr, the method
// is FMConvertMgrAPI::PValueToFormatColor (0x0045f5e0; its reading twin
// FormatColorToPValue is 0x0045f620), the getter is cissurf.cpp's
// cis_GetBlitterQ40C, and the three globals are cissurf.cpp's screen format
// (0x004b1758) and transparent / fill colours (0x004b161c / 0x004b1750).
// Declared with those files' names and types so the calls link to them.
class FMFormat;
class FMConvertMgrAPI
{
public:
	LTRESULT PValueToFormatColor(FMFormat* pFormat, uint32 nValue, uint32* pOut); // 0x0045f5e0
	LTRESULT FormatColorToPValue(FMFormat* pFormat, uint32 nColor, uint32* pOut); // 0x0045f620
};

class CBlitterQ40C;
CBlitterQ40C* cis_GetBlitterQ40C();                                          // 0x0040c120
class PFormatQ40C;
extern PFormatQ40C g_ScreenFormat;                                           // 0x004b1758
extern uint32      g_TransColorQ40C;                                         // 0x004b161c
extern uint32      g_FillColorQ40C;                                          // 0x004b1750

// The three surface workers every blit entry forwards to. The scale worker
// takes a mode - 0 keys on a transparent colour, 1 copies, 2 fills with a
// solid one - and the two colours that go with it; the other two take the
// per-pixel routine to use as an argument.
LTRESULT sub_40d290(HSURFACE hDest, HSURFACE hSrc, LTRect* pSrcRect, int destX,
                    int destY, void* pBlitFn);                               // 0x0040d290
LTRESULT sub_40dcc0(HSURFACE hDest, HSURFACE hSrc, LTRect* pDestRect,
                    LTRect* pSrcRect, int nMode, HLTCOLOR hTransColor,
                    HLTCOLOR hFillColor);                                    // 0x0040dcc0
LTRESULT sub_40e120(HSURFACE hDest, HSURFACE hSrc, LTWarpPt* pCoords,
                    int nCoords, void* pWarpFn);                             // 0x0040e120
LTRESULT sub_40e4d0(HSURFACE hDest, HSURFACE hSrc, LTFloatPt* pOrigin, int destX,
                    int destY, float angle, float scaleX, float scaleY,
                    int nMode, HLTCOLOR hTransColor);                        // 0x0040e4d0
void     sub_40d660();                                                       // 0x0040d660
void     sub_40d710();                                                       // 0x0040d710
void     sub_43aa20();                                                       // 0x0043aa20
void     sub_43aff0();                                                       // 0x0043aff0
void     sub_43ad40();                                                       // 0x0043ad40

LTRESULT sub_411a70(CClientMgr* pClientMgr, uint32 nServerID,
                    CClientCreateRequest* pRequest, HLOCALOBJ* phObject,
                    int nUnknown1, int nUnknown2);                           // 0x00411a70
LTBOOL   sub_411700(CClientMgr* pClientMgr, HLOCALOBJ hCamera, int nMode,
                    HLOCALOBJ* pObjects, int nObjects);                      // 0x00411700
void     sub_42de60(char* pFilename, RMode** ppModeList);                    // 0x0042de60
LTRESULT sub_476e80(CClientMgr* pClientMgr, CClientFileRef* pRef, void** ppTexture); // 0x00476e80
void     sub_483cc0(void* pUnknown, void* pTexture);                         // 0x00483cc0
LTRESULT sub_45c680(HLOCALOBJ hObj, CClientMgr* pClientMgr, char* pTextureName); // 0x0045c680
void     sub_45cfb0(HLOCALOBJ hSystem);                                      // 0x0045cfb0
void     sub_4207a0(CClientMgr* pClientMgr, HLOCALOBJ hObj, LTVector* pPos,
                    int nUnknown);                                           // 0x004207a0
void     sub_420580(CClientMgr* pClientMgr, HLOCALOBJ hObj, LTVector* pScale); // 0x00420580
void     sub_4627d0(CClientMgr* pClientMgr, int nUnknown);                   // 0x004627d0
void     sub_420670(CClientMgr* pClientMgr, CClientLight* pLight);           // 0x00420670
void     sub_45c780(HLOCALOBJ hObj, uint32 nParticles, LTVector* pMinOffset,
                    LTVector* pMaxOffset, LTVector* pMinVelocity,
                    LTVector* pMaxVelocity, LTVector* pMinColor,
                    LTVector* pMaxColor, float minLifetime, float maxLifetime); // 0x0045c780

#endif  // __CLIENTMGR_H__
