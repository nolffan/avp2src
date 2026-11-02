// *********************************************************************** //
//
//  MODULE:   CLIENTINTERFACE (.H)
//
//  PURPOSE:  CLTClient — the client's flat interface table, and the function
//            that fills it.
//
//  The engine hands each game DLL one struct of function pointers: the
//  server's is CLTServer (servermsg.h), filled by si_SetupInterface, and the
//  client's is this one. Reconstructing the *setter* byte for byte turns the
//  struct into a map, because every store's displacement names a field and
//  its operand names the implementation — one 1606-byte function hands over
//  166 addresses at once, and the one it tail-calls another 33.
//
//  The layout is not guesswork and neither are the names. The pointer region
//  runs 0x020..0x35c, which is 208 slots, and src/LT2/sdk/inc/iltclient.h
//  declares exactly 208 function-pointer members. The order matches: slot
//  0x02c is `return m_pClientShell != NULL` and the header's fourth member is
//  IsConnected(); 0x034 calls 0x038 with a literal string and the header has
//  Shutdown() followed by ShutdownWithMessage(char*, ...). So the field name
//  at 0x020 + 4*i is the i'th function-pointer member of the SDK header, and
//  the ILTClient era mismatch that bit the server's vtables does not bite
//  here.
//
//  Nine slots the header declares are never set by the shipped engine, and
//  they are not scattered — they are the two Optimized2D *getters* and all
//  seven of the font calls (CreateFont, DeleteFont, SetFontExtraSpace,
//  GetFontExtraSpace, CreateSurfaceFromString, GetStringDimensions,
//  DrawStringToSurface). NOLF1's engine has no font API, which is why the
//  game ships LithFontMgr and does its own bitmap-font rendering.
//
// *********************************************************************** //

#ifndef __CLIENTINTERFACE_H__
#define __CLIENTINTERFACE_H__

#include "iltclient.h"

class CLTClient
{
public:
	// The first 0x20 bytes are not function pointers; nothing in the setter
	// touches them.
	uint8 m_pad_00[0x20];

	LTRESULT (*StartGame)(StartGameRequest *pRequest);                        // 0x020  -> 0x00408b60
	LTRESULT (*GetGameMode)(int *mode);                                       // 0x024  -> 0x00408b80
	LTRESULT (*GetLocalClientID)(uint32 *pID);                                // 0x028  -> 0x00408bb0
	LTBOOL (*IsConnected)();                                                  // 0x02c  -> 0x00408bf0
	void (*Disconnect)();                                                     // 0x030  -> 0x00408c10
	void (*Shutdown)();                                                       // 0x034  -> 0x00408c80
	void (*ShutdownWithMessage)(char *pMsg, ...);                             // 0x038  -> 0x00408c40
	LTRESULT (*FlipScreen)(uint32 flags);                                     // 0x03c  -> 0x00408c90
	LTRESULT (*ClearScreen)(LTRect *pClearRect, uint32 flags);                // 0x040  -> 0x00408cf0
	LTRESULT (*Start3D)();                                                    // 0x044  -> 0x00408e10
	LTRESULT (*RenderCamera)(HLOCALOBJ hCamera);                              // 0x048  -> 0x00408ea0
	LTRESULT (*RenderObjects)(HLOCALOBJ hCamera, HLOCALOBJ *pObjects, int nObjects); // 0x04c  -> 0x00408f90
	LTRESULT (*StartOptimized2D)();                                           // 0x050  -> 0x004090c0
	LTRESULT (*EndOptimized2D)();                                             // 0x054  -> 0x00409150
	LTRESULT (*SetOptimized2DBlend)(LTSurfaceBlend blend);                    // 0x058  -> 0x004091e0
	LTRESULT (*GetOptimized2DBlend)(LTSurfaceBlend &blend);                   // 0x05c  not set by the shipped engine
	LTRESULT (*SetOptimized2DColor)(HLTCOLOR hColor);                         // 0x060  -> 0x00409280
	LTRESULT (*GetOptimized2DColor)(HLTCOLOR &hColor);                        // 0x064  not set by the shipped engine
	LTRESULT (*End3D)();                                                      // 0x068  -> 0x00409320
	RMode*(*GetRenderModes)();                                                // 0x06c  -> 0x0042de00
	void (*RelinquishRenderModes)(RMode *pModes);                             // 0x070  -> 0x0042df20
	LTRESULT (*GetRenderMode)(RMode *pMode);                                  // 0x074  -> 0x0042df40
	LTRESULT (*SetRenderMode)(RMode *pMode);                                  // 0x078  -> 0x0042df60
	LTRESULT (*ShutdownRender)(uint32 flags);                                 // 0x07c  -> 0x0042e0c0
	FileEntry*(*GetFileList)(char *pDirName);                                 // 0x080  -> 0x004093b0
	void (*FreeFileList)(FileEntry *pHead);                                   // 0x084  -> 0x00436710
	LTRESULT (*GetWorldInfoString)(char *pFilename, char *pInfoString, uint32 maxLen, uint32 *pActualLen); // 0x088  -> 0x004093d0
	LTRESULT (*ReadConfigFile)(char *pFilename);                              // 0x08c  -> 0x004094b0
	LTRESULT (*WriteConfigFile)(char *pFilename);                             // 0x090  -> 0x004094d0
	void (*GetAxisOffsets)(LTFLOAT *offsets);                                 // 0x094  -> 0x004094f0
	void (*PlayJoystickEffect)(char *pEffectName, float x, float y);          // 0x098  -> 0x00409520
	LTBOOL (*InitMusic)(char *szMusicDLL);                                    // 0x09c  -> 0x00409550
	LTBOOL (*SetMusicDirectory)(char *szMusicDirectory);                      // 0x0a0  -> 0x00409570
	LTBOOL (*InitInstruments)(char *szDLSFile, char *szStyleFile);            // 0x0a4  -> 0x004095a0
	LTBOOL (*LoadSong)(char *szSong);                                         // 0x0a8  -> 0x004095e0
	void (*DestroyAllSongs)();                                                // 0x0ac  -> 0x00409620
	LTBOOL (*PlayList)(char *szPlayList, char *szTransition, LTBOOL bLoop, uint32 dwBoundaryFlags); // 0x0b0  -> 0x004097c0
	LTBOOL (*PlayBreak)(char *szSong, uint32 dwBoundaryFlags);                // 0x0b4  -> 0x00409640
	void (*StopMusic)(uint32 dwBoundaryFlags);                                // 0x0b8  -> 0x004096a0
	LTBOOL (*PauseMusic)();                                                   // 0x0bc  -> 0x004096c0
	LTBOOL (*ResumeMusic)();                                                  // 0x0c0  -> 0x004096e0
	LTBOOL (*AddSongToPlayList)(char *szPlayList, char *szSong);              // 0x0c4  -> 0x00409700
	void (*DeletePlayList)(char *szPlayList);                                 // 0x0c8  -> 0x00409780
	LTBOOL (*PlayMotif)(char *szMotif, LTBOOL bLoop);                         // 0x0cc  -> 0x00409850
	void (*StopMotif)(char *szMotif);                                         // 0x0d0  -> 0x00409890
	short (*GetMusicVolume)();                                                // 0x0d4  -> 0x004098c0
	void (*SetMusicVolume)(short wVolume);                                    // 0x0d8  -> 0x004098f0
	LTRESULT (*GetSound3DProviderLists)(Sound3DProvider *&pSound3DProviderList, LTBOOL bVerify); // 0x0dc  -> 0x00409920
	void (*ReleaseSound3DProviderList)(Sound3DProvider *pSound3DProviderList); // 0x0e0  -> 0x00409940
	LTRESULT (*InitSound)(InitSoundInfo *pSoundInfo);                         // 0x0e4  -> 0x00409960
	unsigned short (*GetSoundVolume)();                                       // 0x0e8  -> 0x00409980
	void (*SetSoundVolume)(unsigned short nVolume);                           // 0x0ec  -> 0x004099a0
	LTRESULT (*SetReverbProperties)(ReverbProperties *pReverbProperties);     // 0x0f0  -> 0x004099c0
	LTRESULT (*GetReverbProperties)(ReverbProperties *pReverbProperties);     // 0x0f4  -> 0x004099e0
	LTRESULT (*PlaySound)(PlaySoundInfo *pPlaySoundInfo);                     // 0x0f8  -> 0x00409a00
	LTRESULT (*SetSoundPosition)(HLTSOUND hSound, LTVector *pPos);            // 0x0fc  -> 0x00409a40
	LTRESULT (*GetSoundPosition)(HLTSOUND hSound, LTVector *pPos);            // 0x100  -> 0x00409a60
	void (*PauseSounds)();                                                    // 0x104  -> 0x00409a80
	void (*ResumeSounds)();                                                   // 0x108  -> 0x00409a90
	LTRESULT (*GetSoundDuration)(HLTSOUND hSound, LTFLOAT *fDuration);        // 0x10c  -> 0x00409aa0
	LTRESULT (*GetSoundTimer)(HLTSOUND hSound, LTFLOAT *fTimer);              // 0x110  -> 0x00409ac0
	LTRESULT (*GetSoundData)(HLTSOUND hSound, int16 * & pSixteenBitData, int8 * & pEightBitData, uint32 * dwSamplesPerSecond, uint32 * dwChannels); // 0x114  -> 0x00409bd0
	LTRESULT (*GetSoundOffset)(HLTSOUND hSound, uint32 * dwOffset, uint32 * dwSize = NULL); // 0x118  -> 0x00409d60
	LTBOOL (*IsDone)(HLTSOUND pSoundHandle);                                  // 0x11c  -> 0x00409ec0
	void (*KillSound)(HLTSOUND pSoundHandle);                                 // 0x120  -> 0x00409ee0
	void (*KillSoundLoop)(HLTSOUND hSoundHandle);                             // 0x124  -> 0x00409f00
	void (*SetListener)(LTBOOL bListenerInClient, LTVector *pPos, LTRotation *pRot); // 0x128  -> 0x00409f20
	void (*GetListener)(LTBOOL *bListenerInClient, LTVector *pPos, LTRotation *pRot); // 0x12c  -> 0x0040a000
	LTBOOL (*IntersectSegment)(ClientIntersectQuery *pQuery, ClientIntersectInfo *pInfo); // 0x130  -> 0x00404aa0
	LTBOOL (*CastRay)(ClientIntersectQuery *pQuery, ClientIntersectInfo *pInfo); // 0x134  -> 0x0040a150
	LTRESULT (*FindObjectsInSphere)(LTVector *pCenter, float radius, HLOCALOBJ *inObjects, uint32 nInObjects, uint32 *nOutObjects, uint32 *nFound); // 0x138  -> 0x0040a230
	HLTFONT (*CreateFont)(char *pFontName, int width, int height, LTBOOL bItalic, LTBOOL bUnderline, LTBOOL bBold); // 0x13c  not set by the shipped engine
	void (*DeleteFont)(HLTFONT hFont);                                        // 0x140  not set by the shipped engine
	LTRESULT (*SetFontExtraSpace)(HLTFONT hFont, int pixels);                 // 0x144  not set by the shipped engine
	LTRESULT (*GetFontExtraSpace)(HLTFONT hFont, int &pixels);                // 0x148  not set by the shipped engine
	HLTCOLOR (*CreateColor)(float r, float g, float b, LTBOOL bTransparent);  // 0x14c  -> 0x0040c9b0
	void (*DeleteColor)(HLTCOLOR hColor);                                     // 0x150  -> 0x00404470
	HLTCOLOR (*SetupColor1)(float r, float g, float b, LTBOOL bTransparent);  // 0x154  -> 0x0040ca10
	HLTCOLOR (*SetupColor2)(float r, float g, float b, LTBOOL bTransparent);  // 0x158  -> 0x0040ca30
	LTRESULT (*GetBorderSize)(HSURFACE hSurface, HLTCOLOR hColor, LTRect *pRect); // 0x15c  -> 0x0040ca50
	LTRESULT (*OptimizeSurface)(HSURFACE hSurface, HLTCOLOR hTransparentColor); // 0x160  -> 0x0040cd40
	LTRESULT (*UnoptimizeSurface)(HSURFACE hSurface);                         // 0x164  -> 0x0040cdf0
	HSURFACE (*GetScreenSurface)();                                           // 0x168  -> 0x0040ce60
	HSURFACE (*CreateSurfaceFromBitmap)(char *pBitmapName);                   // 0x16c  -> 0x0040ce70
	HSURFACE (*CreateSurfaceFromString)(HLTFONT hFont, HSTRING hString, HLTCOLOR hForeColor, HLTCOLOR hBackColor, int extraPixelsX, int extraPixelsY); // 0x170  not set by the shipped engine
	HSURFACE (*CreateSurface)(uint32 width, uint32 height);                   // 0x174  -> 0x0040cf90
	LTRESULT (*DeleteSurface)(HSURFACE hSurface);                             // 0x178  -> 0x0040c4b0
	void*(*GetSurfaceUserData)(HSURFACE hSurface);                            // 0x17c  -> 0x0040cfb0
	void (*SetSurfaceUserData)(HSURFACE hSurface, void *pUserData);           // 0x180  -> 0x0040cfc0
	LTRESULT (*GetPixel)(HSURFACE hSurface, uint32 x, uint32 y, HLTCOLOR *color); // 0x184  -> 0x0040c5f0
	LTRESULT (*SetPixel)(HSURFACE hSurface, uint32 x, uint32 y, HLTCOLOR color); // 0x188  -> 0x0040c6f0
	void (*GetStringDimensions)(HLTFONT hFont, HSTRING hString, int *sizeX, int *sizeY); // 0x18c  not set by the shipped engine
	void (*DrawStringToSurface)(HSURFACE hDest, HLTFONT hFont, HSTRING hString, LTRect *pRect, HLTCOLOR hForeColor, HLTCOLOR hBackColor); // 0x190  not set by the shipped engine
	void (*GetSurfaceDims)(HSURFACE hSurf, uint32 *pWidth, uint32 *pHeight);  // 0x194  -> 0x0040cfd0
	LTBOOL (*DrawBitmapToSurface)(HSURFACE hDest, char *pSourceBitmapName, LTRect *pSrcRect, int destX, int destY); // 0x198  -> 0x0040d010
	LTRESULT (*DrawSurfaceMasked)(HSURFACE hDest, HSURFACE hSrc, HSURFACE hMask, LTRect *pSrcRect, int destX, int destY, HLTCOLOR hColor); // 0x19c  -> 0x0040d810
	LTRESULT (*DrawSurfaceSolidColor)(HSURFACE hDest, HSURFACE hSrc, LTRect *pSrcRect, int destX, int destY, HLTCOLOR hTransColor, HLTCOLOR hFillColor); // 0x1a0  -> 0x0040d120
	LTRESULT (*DrawSurfaceToSurface)(HSURFACE hDest, HSURFACE hSrc, LTRect *pSrcRect, int destX, int destY); // 0x1a4  -> 0x0040db50
	LTRESULT (*DrawSurfaceToSurfaceTransparent)(HSURFACE hDest, HSURFACE hSrc, LTRect *pSrcRect, int destX, int destY, HLTCOLOR hColor); // 0x1a8  -> 0x0040dbc0
	LTRESULT (*ScaleSurfaceToSurface)(HSURFACE hDest, HSURFACE hSrc, LTRect *pDestRect, LTRect *pSrcRect); // 0x1ac  -> 0x0040dc50
	LTRESULT (*ScaleSurfaceToSurfaceTransparent)(HSURFACE hDest, HSURFACE hSrc, LTRect *pDestRect, LTRect *pSrcRect, HLTCOLOR hColor); // 0x1b0  -> 0x0040dfc0
	LTRESULT (*ScaleSurfaceToSurfaceSolidColor)(HSURFACE hDest, HSURFACE hSrc, LTRect *pDestRect, LTRect *pSrcRect, HLTCOLOR hTransColor, HLTCOLOR hFillColor); // 0x1b4  -> 0x0040e040
	LTRESULT (*WarpSurfaceToSurface)(HSURFACE hDest, HSURFACE hSrc, LTWarpPt *pCoords, int nCoords); // 0x1b8  -> 0x0040e0b0
	LTRESULT (*WarpSurfaceToSurfaceTransparent)(HSURFACE hDest, HSURFACE hSrc, LTWarpPt *pCoords, int nCoords, HLTCOLOR hColor); // 0x1bc  -> 0x0040e330
	LTRESULT (*WarpSurfaceToSurfaceSolidColor)(HSURFACE hDest, HSURFACE hSrc, LTWarpPt *pCoords, int nCoords, HLTCOLOR hTransColor, HLTCOLOR hFillColor); // 0x1c0  -> 0x0040e3b0
	LTRESULT (*TransformSurfaceToSurface)(HSURFACE hDest, HSURFACE hSrc, LTFloatPt *pOrigin, int destX, int destY, float angle, float scaleX, float scaleY); // 0x1c4  -> 0x0040e450
	LTRESULT (*TransformSurfaceToSurfaceTransparent)(HSURFACE hDest, HSURFACE hSrc, LTFloatPt *pOrigin, int destX, int destY, float angle, float scaleX, float scaleY, HLTCOLOR hColor); // 0x1c8  -> 0x0040e990
	LTRESULT (*FillRect)(HSURFACE hDest, LTRect *pRect, HLTCOLOR hColor);     // 0x1cc  -> 0x0040ea30
	LTRESULT (*GetSurfaceAlpha)(HSURFACE hSurface, float &alpha);             // 0x1d0  -> 0x0040ec70
	LTRESULT (*SetSurfaceAlpha)(HSURFACE hSurface, float alpha);              // 0x1d4  -> 0x0040ecc0
	LTRESULT (*RegisterConsoleProgram)(char *pName, ConsoleProgramFn fn);     // 0x1d8  -> 0x0040a400
	LTRESULT (*UnregisterConsoleProgram)(char *pName);                        // 0x1dc  -> 0x0040a470
	HCONSOLEVAR (*GetConsoleVar)(char *pName);                                // 0x1e0  -> 0x00462310
	void (*StartCounter)(LTCounter *pCounter);                                // 0x1e4  -> 0x00435720
	uint32 (*EndCounter)(LTCounter *pCounter);                                // 0x1e8  -> 0x00435730
	LTRESULT (*SetGlobalPanTexture)(uint32 index, char *pFilename);           // 0x1ec  -> 0x0040a4f0
	LTRESULT (*SetGlobalPanInfo)(uint32 index, float xOffset, float zOffset, float xScale, float zScale); // 0x1f0  -> 0x0040a5d0
	LTRESULT (*AddSurfaceEffect)(SurfaceEffectDesc *pDesc);                   // 0x1f4  -> 0x0040a640
	void (*SetInputState)(LTBOOL bOn);                                        // 0x1f8  -> 0x0040a660
	LTRESULT (*ClearInput)();                                                 // 0x1fc  -> 0x0040a680
	DeviceBinding*(*GetDeviceBindings)(uint32 nDevice);                       // 0x200  -> 0x0040a690
	void (*FreeDeviceBindings)(DeviceBinding* pBindings);                     // 0x204  -> 0x0040a6b0
	LTRESULT (*StartDeviceTrack)(uint32 nDevices, uint32 nBufferSize);        // 0x208  -> 0x0040a6d0
	LTRESULT (*TrackDevice)(DeviceInput* pInputArray, uint32* pnInOut);       // 0x20c  -> 0x0040a720
	LTRESULT (*EndDeviceTrack)();                                             // 0x210  -> 0x0040a760
	DeviceObject*(*GetDeviceObjects)(uint32 nDeviceFlags);                    // 0x214  -> 0x0040a7a0
	void (*FreeDeviceObjects)(DeviceObject* pList);                           // 0x218  -> 0x0040a7c0
	LTRESULT (*GetDeviceName)(uint32 nDeviceType, char* pStrBuffer, uint32 nBufferSize); // 0x21c  -> 0x0040a7e0
	LTRESULT (*IsDeviceEnabled)(char* strDeviceName, LTBOOL* pIsEnabled);     // 0x220  -> 0x0040a820
	LTRESULT (*EnableDevice)(char* strDeviceName);                            // 0x224  -> 0x0040a850
	float (*GetGameTime)();                                                   // 0x228  -> 0x0040a880
	float (*GetGameFrameTime)();                                              // 0x22c  -> 0x0040a8a0
	void (*DebugOut)(char *pMsg, ...);                                        // 0x230  -> 0x004297d0
	LTRESULT (*GetSkyDef)(SkyDef *pDef);                                      // 0x234  -> 0x0040a8c0
	LTBOOL (*IsCommandOn)(int commandNum);                                    // 0x238  -> 0x0040a8e0
	void (*RunConsoleString)(char *pString);                                  // 0x23c  -> 0x0040a920
	HLOCALOBJ (*GetClientObject)();                                           // 0x240  -> 0x0040a930
	void (*GetGlobalLightScale)(LTVector *pScale);                            // 0x244  -> 0x0040a950
	void (*SetGlobalLightScale)(LTVector *pScale);                            // 0x248  -> 0x0040a970
	void (*OffsetGlobalLightScale)(LTVector *pOffset);                        // 0x24c  -> 0x0040aa70
	int (*Parse)(char *pCommand, char **pNewCommandPos, char *argBuffer, char **argPointers, int *nArgs); // 0x250  -> 0x0040bfa0
	HLOCALOBJ (*CreateObject)(ObjectCreateStruct *pStruct);                   // 0x254  -> 0x0040abc0
	void (*GetObjectPos)(HLOCALOBJ hObj, LTVector *pPos);                     // 0x258  -> 0x0040afa0
	void (*GetObjectRotation)(HLOCALOBJ hObj, LTRotation *pRotation);         // 0x25c  -> 0x0040afd0
	void (*SetObjectRotation)(HLOCALOBJ hObj, LTRotation *pRotation);         // 0x260  -> 0x0040b010
	void (*SetObjectPosAndRotation)(HLOCALOBJ hObj, LTVector *pPos, LTRotation *pRotation); // 0x264  -> 0x0040ac50
	LTRESULT (*GetObjectScale)(HLOCALOBJ hObj, LTVector *pScale);             // 0x268  -> 0x0040ac80
	LTRESULT (*SetObjectScale)(HLOCALOBJ hObj, LTVector *pScale);             // 0x26c  -> 0x0040ace0
	void (*GetObjectColor)(HLOCALOBJ hObject, float *r, float *g, float *b, float *a); // 0x270  -> 0x0040ad40
	void (*SetObjectColor)(HLOCALOBJ hObject, float r, float g, float b, float a); // 0x274  -> 0x0040add0
	LTRESULT (*GetObjectUserFlags)(HLOCALOBJ hObj, uint32 *pFlags);           // 0x278  -> 0x0040ae30
	LTRESULT (*SetObjectUserFlags)(HLOCALOBJ hObj, uint32 flags);             // 0x27c  -> 0x0040ae90
	uint32 (*GetObjectClientFlags)(HLOCALOBJ hObj);                           // 0x280  -> 0x0040af20
	void (*SetObjectClientFlags)(HLOCALOBJ hObj, uint32 flags);               // 0x284  -> 0x0040af40
	void*(*GetObjectUserData)(HLOCALOBJ hObj);                                // 0x288  -> 0x0040af60
	void (*SetObjectUserData)(HLOCALOBJ hObj, void *pData);                   // 0x28c  -> 0x0040af80
	LTRESULT (*Get3DCameraPt)(HLOCALOBJ hCamera, int sx, int sy, LTVector *pOut); // 0x290  -> 0x0040b040
	void (*GetCameraFOV)(HLOCALOBJ hObj, float *pX, float *pY);               // 0x294  -> 0x0040b2d0
	void (*SetCameraFOV)(HLOCALOBJ hObj, float fovX, float fovY);             // 0x298  -> 0x0040b310
	void (*GetCameraRect)(HLOCALOBJ hObj, LTBOOL *bFullscreen, int *left, int *top, int *right, int *bottom); // 0x29c  -> 0x0040b3b0
	void (*SetCameraRect)(HLOCALOBJ hObj, LTBOOL bFullscreen, int left, int top, int right, int bottom); // 0x2a0  -> 0x0040b420
	LTBOOL (*GetCameraLightAdd)(HLOCALOBJ hCamera, LTVector *pAdd);           // 0x2a4  -> 0x0040b470
	LTBOOL (*SetCameraLightAdd)(HLOCALOBJ hCamera, LTVector *pAdd);           // 0x2a8  -> 0x0040b4b0
	LTRESULT (*SetupParticleSystem)(HLOCALOBJ hObj, char *pTextureName, float gravityAccel, uint32 flags, float particleRadius); // 0x2ac  -> 0x0040b4f0
	LTRESULT (*SetSoftwarePSColor)(HLOCALOBJ hObj, float r, float g, float b); // 0x2b0  -> 0x0040b5b0
	void (*AddParticles)(HLOCALOBJ hObj, uint32 nParticles, LTVector *pMinOffset, LTVector *pMaxOffset, LTVector *pMinVelocity, LTVector *pMaxVelocity, LTVector *pMinColor, LTVector *pMaxColor, float minLifetime, float maxLifetime); // 0x2b4  -> 0x0040b640
	LTBOOL (*GetParticles)(HLOCALOBJ hObj, LTParticle **pHead, LTParticle **pTail); // 0x2b8  -> 0x0040b690
	void (*GetParticlePos)(HLOCALOBJ hSystem, LTParticle *pParticle, LTVector *pPos); // 0x2bc  -> 0x0040b6d0
	void (*SetParticlePos)(HLOCALOBJ hSystem, LTParticle *pParticle, LTVector *pPos); // 0x2c0  -> 0x0040b7a0
	LTRESULT (*GetParticleLifetime)(HLOCALOBJ hSystem, LTParticle *pParticle, LTFLOAT & fLifetime); // 0x2c4  -> 0x0040b700
	LTRESULT (*GetParticleTotalLifetime)(HLOCALOBJ hSystem, LTParticle *pParticle, LTFLOAT & fTotalLifetime); // 0x2c8  -> 0x0040b750
	void (*RemoveParticle)(HLOCALOBJ hSystem, LTParticle *pParticle);         // 0x2cc  -> 0x0040b880
	LTRESULT (*OptimizeParticles)(HLOCALOBJ hSystem);                         // 0x2d0  -> 0x0040b8d0
	HLTLINE (*GetNextLine)(HLOCALOBJ hObj, HLTLINE hPrev);                    // 0x2d4  -> 0x0043ce00
	void (*GetLineInfo)(HLTLINE hLine, LTLine *pLine);                        // 0x2d8  -> 0x0043ce50
	void (*SetLineInfo)(HLTLINE hLine, LTLine *pLine);                        // 0x2dc  -> 0x0043ceb0
	HLTLINE (*AddLine)(HLOCALOBJ hObj, LTLine *pLine);                        // 0x2e0  -> 0x0043d1c0
	void (*RemoveLine)(HLOCALOBJ hObj, HLTLINE hLine);                        // 0x2e4  -> 0x0043d510
	LTBOOL (*SetupPolyGrid)(HLOCALOBJ hObj, uint32 width, uint32 height, LTBOOL bHalfTrianges); // 0x2e8  -> 0x0040b940
	LTRESULT (*SetPolyGridTexture)(HLOCALOBJ hObj, char *pFilename);          // 0x2ec  -> 0x0040b970
	LTRESULT (*SetPolyGridEnvMap)(HLOCALOBJ hObj, char *pFilename);           // 0x2f0  -> 0x0040ba30
	LTRESULT (*GetPolyGridTextureInfo)(HLOCALOBJ hObj, float *xPan, float *yPan, float *xScale, float *yScale); // 0x2f4  -> 0x0040bb20
	LTRESULT (*SetPolyGridTextureInfo)(HLOCALOBJ hObj, float xPan, float yPan, float xScale, float yScale); // 0x2f8  -> 0x0040bc00
	LTRESULT (*GetPolyGridInfo)(HLOCALOBJ hObj, char **pBytes, uint32 *pWidth, uint32 *pHeight, PGColor **pColorTable); // 0x2fc  -> 0x0040bc80
	LTRESULT (*FitPolyGrid)(HLOCALOBJ hObj, LTVector *pMin, LTVector *pMax, LTVector *pPos, LTVector *pScale); // 0x300  -> 0x0040bd20
	void (*GetLightColor)(HLOCALOBJ hObj, float *r, float *g, float *b);      // 0x304  -> 0x0040bed0
	void (*SetLightColor)(HLOCALOBJ hObj, float r, float g, float b);         // 0x308  -> 0x0040bef0
	float (*GetLightRadius)(HLOCALOBJ hObj);                                  // 0x30c  -> 0x00472af0
	void (*SetLightRadius)(HLOCALOBJ hObj, float radius);                     // 0x310  -> 0x0040bf30
	LTRESULT (*ClipSprite)(HLOCALOBJ hObj, HPOLY hPoly);                      // 0x314  -> 0x0040bf70
	LTRESULT (*GetNextModelNode)(HLOCALOBJ hObject, HMODELNODE hNode, HMODELNODE *pNext); // 0x318  -> 0x00436370
	LTRESULT (*GetModelNodeName)(HLOCALOBJ hObject, HMODELNODE hNode, char *pName, uint32 maxLen); // 0x31c  -> 0x004363f0
	LTBOOL (*GetSurfaceBounds)(SurfaceData *pSurface, LTVector *pMin, LTVector *pMax); // 0x320  -> 0x00407e30
	LTRESULT (*SetPolyHideStatus)(HPOLY hPoly, LTBOOL bHide);                 // 0x324  -> 0x00407f90
	LTRESULT (*GetPolyTextureFlags)(HPOLY hPoly, uint32 *pFlags);             // 0x328  -> 0x00408070
	LTRESULT (*SetModelHook)(ModelHookFn fn, void *pUser);                    // 0x32c  -> 0x00408150
	LTRESULT (*GetEngineHook)(char *pName, void **pData);                     // 0x330  -> 0x0040ed50
	LTRESULT (*InitNetworking)(char *pDriver, uint32 dwFlags);                // 0x334  -> 0x00408180
	LTRESULT (*GetServiceList)(NetService* &pListHead);                       // 0x338  -> 0x004081c0
	LTRESULT (*FreeServiceList)(NetService *pListHead);                       // 0x33c  -> 0x004081e0
	LTRESULT (*SelectService)(HNETSERVICE hNetService);                       // 0x340  -> 0x00408240
	LTRESULT (*GetSessionList)(NetSession* &pListHead, char *pInfo);          // 0x344  -> 0x004082a0
	LTRESULT (*FreeSessionList)(NetSession *pListHead);                       // 0x348  -> 0x004082c0
	LTRESULT (*AddInternetDriver)();                                          // 0x34c  -> 0x004082e0
	LTRESULT (*RemoveInternetDriver)();                                       // 0x350  -> 0x00408320
	LTRESULT (*IsLobbyLaunched)(char* sDriver);                               // 0x354  -> 0x004083c0
	LTRESULT (*GetLobbyLaunchInfo)(char* sDriver, void** ppLobbyLaunchData);  // 0x358  -> 0x00408440
	LTRESULT (*GetTcpIpAddress)(char* sAddress, uint32 dwBufferSize);         // 0x35c  -> 0x00408360
};


// 0x00408510 fills 166 slots and tail-calls 0x0040c800, which fills the
// other 33. The second one is not a pure setter — it interleaves its stores
// with allocation calls — so only its targets are recorded above.
void ci_SetupInterface(CLTClient* pClient);
void ci_SetupInterface2(CLTClient* pClient);

// The 199 implementations the setters install, with the signature the
// header's slot gives them and the shipped address they live at.
LTRESULT ci_StartGame(StartGameRequest *pRequest);         // 0x00408b60
LTRESULT ci_GetGameMode(int *mode);                        // 0x00408b80
LTRESULT ci_GetLocalClientID(uint32 *pID);                 // 0x00408bb0
LTBOOL ci_IsConnected();                                   // 0x00408bf0
void ci_Disconnect();                                      // 0x00408c10
void ci_Shutdown();                                        // 0x00408c80
void ci_ShutdownWithMessage(char *pMsg, ...);              // 0x00408c40
LTRESULT ci_FlipScreen(uint32 flags);                      // 0x00408c90
LTRESULT ci_ClearScreen(LTRect *pClearRect, uint32 flags); // 0x00408cf0
LTRESULT ci_Start3D();                                     // 0x00408e10
LTRESULT ci_RenderCamera(HLOCALOBJ hCamera);               // 0x00408ea0
LTRESULT ci_RenderObjects(HLOCALOBJ hCamera, HLOCALOBJ *pObjects, int nObjects); // 0x00408f90
LTRESULT ci_StartOptimized2D();                            // 0x004090c0
LTRESULT ci_EndOptimized2D();                              // 0x00409150
LTRESULT ci_SetOptimized2DBlend(LTSurfaceBlend blend);     // 0x004091e0
LTRESULT ci_SetOptimized2DColor(HLTCOLOR hColor);          // 0x00409280
LTRESULT ci_End3D();                                       // 0x00409320
RMode*ci_GetRenderModes();                                 // 0x0042de00
void ci_RelinquishRenderModes(RMode *pModes);              // 0x0042df20
LTRESULT ci_GetRenderMode(RMode *pMode);                   // 0x0042df40
LTRESULT ci_SetRenderMode(RMode *pMode);                   // 0x0042df60
LTRESULT ci_ShutdownRender(uint32 flags);                  // 0x0042e0c0
FileEntry*ci_GetFileList(char *pDirName);                  // 0x004093b0
void ci_FreeFileList(FileEntry *pHead);                    // 0x00436710
LTRESULT ci_GetWorldInfoString(char *pFilename, char *pInfoString, uint32 maxLen, uint32 *pActualLen); // 0x004093d0
LTRESULT ci_ReadConfigFile(char *pFilename);               // 0x004094b0
LTRESULT ci_WriteConfigFile(char *pFilename);              // 0x004094d0
void ci_GetAxisOffsets(LTFLOAT *offsets);                  // 0x004094f0
void ci_PlayJoystickEffect(char *pEffectName, float x, float y); // 0x00409520
LTBOOL ci_InitMusic(char *szMusicDLL);                     // 0x00409550
LTBOOL ci_SetMusicDirectory(char *szMusicDirectory);       // 0x00409570
LTBOOL ci_InitInstruments(char *szDLSFile, char *szStyleFile); // 0x004095a0
LTBOOL ci_LoadSong(char *szSong);                          // 0x004095e0
void ci_DestroyAllSongs();                                 // 0x00409620
LTBOOL ci_PlayList(char *szPlayList, char *szTransition, LTBOOL bLoop, uint32 dwBoundaryFlags); // 0x004097c0
LTBOOL ci_PlayBreak(char *szSong, uint32 dwBoundaryFlags); // 0x00409640
void ci_StopMusic(uint32 dwBoundaryFlags);                 // 0x004096a0
LTBOOL ci_PauseMusic();                                    // 0x004096c0
LTBOOL ci_ResumeMusic();                                   // 0x004096e0
LTBOOL ci_AddSongToPlayList(char *szPlayList, char *szSong); // 0x00409700
void ci_DeletePlayList(char *szPlayList);                  // 0x00409780
LTBOOL ci_PlayMotif(char *szMotif, LTBOOL bLoop);          // 0x00409850
void ci_StopMotif(char *szMotif);                          // 0x00409890
short ci_GetMusicVolume();                                 // 0x004098c0
void ci_SetMusicVolume(short wVolume);                     // 0x004098f0
LTRESULT ci_GetSound3DProviderLists(Sound3DProvider *&pSound3DProviderList, LTBOOL bVerify); // 0x00409920
void ci_ReleaseSound3DProviderList(Sound3DProvider *pSound3DProviderList); // 0x00409940
LTRESULT ci_InitSound(InitSoundInfo *pSoundInfo);          // 0x00409960
unsigned short ci_GetSoundVolume();                        // 0x00409980
void ci_SetSoundVolume(unsigned short nVolume);            // 0x004099a0
LTRESULT ci_SetReverbProperties(ReverbProperties *pReverbProperties); // 0x004099c0
LTRESULT ci_GetReverbProperties(ReverbProperties *pReverbProperties); // 0x004099e0
LTRESULT ci_PlaySound(PlaySoundInfo *pPlaySoundInfo);      // 0x00409a00
LTRESULT ci_SetSoundPosition(HLTSOUND hSound, LTVector *pPos); // 0x00409a40
LTRESULT ci_GetSoundPosition(HLTSOUND hSound, LTVector *pPos); // 0x00409a60
void ci_PauseSounds();                                     // 0x00409a80
void ci_ResumeSounds();                                    // 0x00409a90
LTRESULT ci_GetSoundDuration(HLTSOUND hSound, LTFLOAT *fDuration); // 0x00409aa0
LTRESULT ci_GetSoundTimer(HLTSOUND hSound, LTFLOAT *fTimer); // 0x00409ac0
LTRESULT ci_GetSoundData(HLTSOUND hSound, int16 * & pSixteenBitData, int8 * & pEightBitData, uint32 * dwSamplesPerSecond, uint32 * dwChannels); // 0x00409bd0
LTRESULT ci_GetSoundOffset(HLTSOUND hSound, uint32 * dwOffset, uint32 * dwSize = NULL); // 0x00409d60
LTBOOL ci_IsDone(HLTSOUND pSoundHandle);                   // 0x00409ec0
void ci_KillSound(HLTSOUND pSoundHandle);                  // 0x00409ee0
void ci_KillSoundLoop(HLTSOUND hSoundHandle);              // 0x00409f00
void ci_SetListener(LTBOOL bListenerInClient, LTVector *pPos, LTRotation *pRot); // 0x00409f20
void ci_GetListener(LTBOOL *bListenerInClient, LTVector *pPos, LTRotation *pRot); // 0x0040a000
LTBOOL ci_IntersectSegment(ClientIntersectQuery *pQuery, ClientIntersectInfo *pInfo); // 0x00404aa0
LTBOOL ci_CastRay(ClientIntersectQuery *pQuery, ClientIntersectInfo *pInfo); // 0x0040a150
LTRESULT ci_FindObjectsInSphere(LTVector *pCenter, float radius, HLOCALOBJ *inObjects, uint32 nInObjects, uint32 *nOutObjects, uint32 *nFound); // 0x0040a230
HLTCOLOR ci_CreateColor(float r, float g, float b, LTBOOL bTransparent); // 0x0040c9b0
void ci_DeleteColor(HLTCOLOR hColor);                      // 0x00404470
HLTCOLOR ci_SetupColor1(float r, float g, float b, LTBOOL bTransparent); // 0x0040ca10
HLTCOLOR ci_SetupColor2(float r, float g, float b, LTBOOL bTransparent); // 0x0040ca30
LTRESULT ci_GetBorderSize(HSURFACE hSurface, HLTCOLOR hColor, LTRect *pRect); // 0x0040ca50
LTRESULT ci_OptimizeSurface(HSURFACE hSurface, HLTCOLOR hTransparentColor); // 0x0040cd40
LTRESULT ci_UnoptimizeSurface(HSURFACE hSurface);          // 0x0040cdf0
HSURFACE ci_GetScreenSurface();                            // 0x0040ce60
HSURFACE ci_CreateSurfaceFromBitmap(char *pBitmapName);    // 0x0040ce70
HSURFACE ci_CreateSurface(uint32 width, uint32 height);    // 0x0040cf90
LTRESULT ci_DeleteSurface(HSURFACE hSurface);              // 0x0040c4b0
void*ci_GetSurfaceUserData(HSURFACE hSurface);             // 0x0040cfb0
void ci_SetSurfaceUserData(HSURFACE hSurface, void *pUserData); // 0x0040cfc0
LTRESULT ci_GetPixel(HSURFACE hSurface, uint32 x, uint32 y, HLTCOLOR *color); // 0x0040c5f0
LTRESULT ci_SetPixel(HSURFACE hSurface, uint32 x, uint32 y, HLTCOLOR color); // 0x0040c6f0
void ci_GetSurfaceDims(HSURFACE hSurf, uint32 *pWidth, uint32 *pHeight); // 0x0040cfd0
LTBOOL ci_DrawBitmapToSurface(HSURFACE hDest, char *pSourceBitmapName, LTRect *pSrcRect, int destX, int destY); // 0x0040d010
LTRESULT ci_DrawSurfaceMasked(HSURFACE hDest, HSURFACE hSrc, HSURFACE hMask, LTRect *pSrcRect, int destX, int destY, HLTCOLOR hColor); // 0x0040d810
LTRESULT ci_DrawSurfaceSolidColor(HSURFACE hDest, HSURFACE hSrc, LTRect *pSrcRect, int destX, int destY, HLTCOLOR hTransColor, HLTCOLOR hFillColor); // 0x0040d120
LTRESULT ci_DrawSurfaceToSurface(HSURFACE hDest, HSURFACE hSrc, LTRect *pSrcRect, int destX, int destY); // 0x0040db50
LTRESULT ci_DrawSurfaceToSurfaceTransparent(HSURFACE hDest, HSURFACE hSrc, LTRect *pSrcRect, int destX, int destY, HLTCOLOR hColor); // 0x0040dbc0
LTRESULT ci_ScaleSurfaceToSurface(HSURFACE hDest, HSURFACE hSrc, LTRect *pDestRect, LTRect *pSrcRect); // 0x0040dc50
LTRESULT ci_ScaleSurfaceToSurfaceTransparent(HSURFACE hDest, HSURFACE hSrc, LTRect *pDestRect, LTRect *pSrcRect, HLTCOLOR hColor); // 0x0040dfc0
LTRESULT ci_ScaleSurfaceToSurfaceSolidColor(HSURFACE hDest, HSURFACE hSrc, LTRect *pDestRect, LTRect *pSrcRect, HLTCOLOR hTransColor, HLTCOLOR hFillColor); // 0x0040e040
LTRESULT ci_WarpSurfaceToSurface(HSURFACE hDest, HSURFACE hSrc, LTWarpPt *pCoords, int nCoords); // 0x0040e0b0
LTRESULT ci_WarpSurfaceToSurfaceTransparent(HSURFACE hDest, HSURFACE hSrc, LTWarpPt *pCoords, int nCoords, HLTCOLOR hColor); // 0x0040e330
LTRESULT ci_WarpSurfaceToSurfaceSolidColor(HSURFACE hDest, HSURFACE hSrc, LTWarpPt *pCoords, int nCoords, HLTCOLOR hTransColor, HLTCOLOR hFillColor); // 0x0040e3b0
LTRESULT ci_TransformSurfaceToSurface(HSURFACE hDest, HSURFACE hSrc, LTFloatPt *pOrigin, int destX, int destY, float angle, float scaleX, float scaleY); // 0x0040e450
LTRESULT ci_TransformSurfaceToSurfaceTransparent(HSURFACE hDest, HSURFACE hSrc, LTFloatPt *pOrigin, int destX, int destY, float angle, float scaleX, float scaleY, HLTCOLOR hColor); // 0x0040e990
LTRESULT ci_FillRect(HSURFACE hDest, LTRect *pRect, HLTCOLOR hColor); // 0x0040ea30
LTRESULT ci_GetSurfaceAlpha(HSURFACE hSurface, float &alpha); // 0x0040ec70
LTRESULT ci_SetSurfaceAlpha(HSURFACE hSurface, float alpha); // 0x0040ecc0
LTRESULT ci_RegisterConsoleProgram(char *pName, ConsoleProgramFn fn); // 0x0040a400
LTRESULT ci_UnregisterConsoleProgram(char *pName);         // 0x0040a470
HCONSOLEVAR ci_GetConsoleVar(char *pName);                 // 0x00462310
void ci_StartCounter(LTCounter *pCounter);                 // 0x00435720
uint32 ci_EndCounter(LTCounter *pCounter);                 // 0x00435730
LTRESULT ci_SetGlobalPanTexture(uint32 index, char *pFilename); // 0x0040a4f0
LTRESULT ci_SetGlobalPanInfo(uint32 index, float xOffset, float zOffset, float xScale, float zScale); // 0x0040a5d0
LTRESULT ci_AddSurfaceEffect(SurfaceEffectDesc *pDesc);    // 0x0040a640
void ci_SetInputState(LTBOOL bOn);                         // 0x0040a660
LTRESULT ci_ClearInput();                                  // 0x0040a680
DeviceBinding*ci_GetDeviceBindings(uint32 nDevice);        // 0x0040a690
void ci_FreeDeviceBindings(DeviceBinding* pBindings);      // 0x0040a6b0
LTRESULT ci_StartDeviceTrack(uint32 nDevices, uint32 nBufferSize); // 0x0040a6d0
LTRESULT ci_TrackDevice(DeviceInput* pInputArray, uint32* pnInOut); // 0x0040a720
LTRESULT ci_EndDeviceTrack();                              // 0x0040a760
DeviceObject*ci_GetDeviceObjects(uint32 nDeviceFlags);     // 0x0040a7a0
void ci_FreeDeviceObjects(DeviceObject* pList);            // 0x0040a7c0
LTRESULT ci_GetDeviceName(uint32 nDeviceType, char* pStrBuffer, uint32 nBufferSize); // 0x0040a7e0
LTRESULT ci_IsDeviceEnabled(char* strDeviceName, LTBOOL* pIsEnabled); // 0x0040a820
LTRESULT ci_EnableDevice(char* strDeviceName);             // 0x0040a850
float ci_GetGameTime();                                    // 0x0040a880
float ci_GetGameFrameTime();                               // 0x0040a8a0
void ci_DebugOut(char *pMsg, ...);                         // 0x004297d0
LTRESULT ci_GetSkyDef(SkyDef *pDef);                       // 0x0040a8c0
LTBOOL ci_IsCommandOn(int commandNum);                     // 0x0040a8e0
void ci_RunConsoleString(char *pString);                   // 0x0040a920
HLOCALOBJ ci_GetClientObject();                            // 0x0040a930
void ci_GetGlobalLightScale(LTVector *pScale);             // 0x0040a950
void ci_SetGlobalLightScale(LTVector *pScale);             // 0x0040a970
void ci_OffsetGlobalLightScale(LTVector *pOffset);         // 0x0040aa70
int ci_Parse(char *pCommand, char **pNewCommandPos, char *argBuffer, char **argPointers, int *nArgs); // 0x0040bfa0
HLOCALOBJ ci_CreateObject(ObjectCreateStruct *pStruct);    // 0x0040abc0
void ci_GetObjectPos(HLOCALOBJ hObj, LTVector *pPos);      // 0x0040afa0
void ci_GetObjectRotation(HLOCALOBJ hObj, LTRotation *pRotation); // 0x0040afd0
void ci_SetObjectRotation(HLOCALOBJ hObj, LTRotation *pRotation); // 0x0040b010
void ci_SetObjectPosAndRotation(HLOCALOBJ hObj, LTVector *pPos, LTRotation *pRotation); // 0x0040ac50
LTRESULT ci_GetObjectScale(HLOCALOBJ hObj, LTVector *pScale); // 0x0040ac80
LTRESULT ci_SetObjectScale(HLOCALOBJ hObj, LTVector *pScale); // 0x0040ace0
void ci_GetObjectColor(HLOCALOBJ hObject, float *r, float *g, float *b, float *a); // 0x0040ad40
void ci_SetObjectColor(HLOCALOBJ hObject, float r, float g, float b, float a); // 0x0040add0
LTRESULT ci_GetObjectUserFlags(HLOCALOBJ hObj, uint32 *pFlags); // 0x0040ae30
LTRESULT ci_SetObjectUserFlags(HLOCALOBJ hObj, uint32 flags); // 0x0040ae90
uint32 ci_GetObjectClientFlags(HLOCALOBJ hObj);            // 0x0040af20
void ci_SetObjectClientFlags(HLOCALOBJ hObj, uint32 flags); // 0x0040af40
void*ci_GetObjectUserData(HLOCALOBJ hObj);                 // 0x0040af60
void ci_SetObjectUserData(HLOCALOBJ hObj, void *pData);    // 0x0040af80
LTRESULT ci_Get3DCameraPt(HLOCALOBJ hCamera, int sx, int sy, LTVector *pOut); // 0x0040b040
void ci_GetCameraFOV(HLOCALOBJ hObj, float *pX, float *pY); // 0x0040b2d0
void ci_SetCameraFOV(HLOCALOBJ hObj, float fovX, float fovY); // 0x0040b310
void ci_GetCameraRect(HLOCALOBJ hObj, LTBOOL *bFullscreen, int *left, int *top, int *right, int *bottom); // 0x0040b3b0
void ci_SetCameraRect(HLOCALOBJ hObj, LTBOOL bFullscreen, int left, int top, int right, int bottom); // 0x0040b420
LTBOOL ci_GetCameraLightAdd(HLOCALOBJ hCamera, LTVector *pAdd); // 0x0040b470
LTBOOL ci_SetCameraLightAdd(HLOCALOBJ hCamera, LTVector *pAdd); // 0x0040b4b0
LTRESULT ci_SetupParticleSystem(HLOCALOBJ hObj, char *pTextureName, float gravityAccel, uint32 flags, float particleRadius); // 0x0040b4f0
LTRESULT ci_SetSoftwarePSColor(HLOCALOBJ hObj, float r, float g, float b); // 0x0040b5b0
void ci_AddParticles(HLOCALOBJ hObj, uint32 nParticles, LTVector *pMinOffset, LTVector *pMaxOffset, LTVector *pMinVelocity, LTVector *pMaxVelocity, LTVector *pMinColor, LTVector *pMaxColor, float minLifetime, float maxLifetime); // 0x0040b640
LTBOOL ci_GetParticles(HLOCALOBJ hObj, LTParticle **pHead, LTParticle **pTail); // 0x0040b690
void ci_GetParticlePos(HLOCALOBJ hSystem, LTParticle *pParticle, LTVector *pPos); // 0x0040b6d0
void ci_SetParticlePos(HLOCALOBJ hSystem, LTParticle *pParticle, LTVector *pPos); // 0x0040b7a0
LTRESULT ci_GetParticleLifetime(HLOCALOBJ hSystem, LTParticle *pParticle, LTFLOAT & fLifetime); // 0x0040b700
LTRESULT ci_GetParticleTotalLifetime(HLOCALOBJ hSystem, LTParticle *pParticle, LTFLOAT & fTotalLifetime); // 0x0040b750
void ci_RemoveParticle(HLOCALOBJ hSystem, LTParticle *pParticle); // 0x0040b880
LTRESULT ci_OptimizeParticles(HLOCALOBJ hSystem);          // 0x0040b8d0
HLTLINE ci_GetNextLine(HLOCALOBJ hObj, HLTLINE hPrev);     // 0x0043ce00
void ci_GetLineInfo(HLTLINE hLine, LTLine *pLine);         // 0x0043ce50
void ci_SetLineInfo(HLTLINE hLine, LTLine *pLine);         // 0x0043ceb0
HLTLINE ci_AddLine(HLOCALOBJ hObj, LTLine *pLine);         // 0x0043d1c0
void ci_RemoveLine(HLOCALOBJ hObj, HLTLINE hLine);         // 0x0043d510
LTBOOL ci_SetupPolyGrid(HLOCALOBJ hObj, uint32 width, uint32 height, LTBOOL bHalfTrianges); // 0x0040b940
LTRESULT ci_SetPolyGridTexture(HLOCALOBJ hObj, char *pFilename); // 0x0040b970
LTRESULT ci_SetPolyGridEnvMap(HLOCALOBJ hObj, char *pFilename); // 0x0040ba30
LTRESULT ci_GetPolyGridTextureInfo(HLOCALOBJ hObj, float *xPan, float *yPan, float *xScale, float *yScale); // 0x0040bb20
LTRESULT ci_SetPolyGridTextureInfo(HLOCALOBJ hObj, float xPan, float yPan, float xScale, float yScale); // 0x0040bc00
LTRESULT ci_GetPolyGridInfo(HLOCALOBJ hObj, char **pBytes, uint32 *pWidth, uint32 *pHeight, PGColor **pColorTable); // 0x0040bc80
LTRESULT ci_FitPolyGrid(HLOCALOBJ hObj, LTVector *pMin, LTVector *pMax, LTVector *pPos, LTVector *pScale); // 0x0040bd20
void ci_GetLightColor(HLOCALOBJ hObj, float *r, float *g, float *b); // 0x0040bed0
void ci_SetLightColor(HLOCALOBJ hObj, float r, float g, float b); // 0x0040bef0
float ci_GetLightRadius(HLOCALOBJ hObj);                   // 0x00472af0
void ci_SetLightRadius(HLOCALOBJ hObj, float radius);      // 0x0040bf30
LTRESULT ci_ClipSprite(HLOCALOBJ hObj, HPOLY hPoly);       // 0x0040bf70
LTRESULT ci_GetNextModelNode(HLOCALOBJ hObject, HMODELNODE hNode, HMODELNODE *pNext); // 0x00436370
LTRESULT ci_GetModelNodeName(HLOCALOBJ hObject, HMODELNODE hNode, char *pName, uint32 maxLen); // 0x004363f0
LTBOOL ci_GetSurfaceBounds(SurfaceData *pSurface, LTVector *pMin, LTVector *pMax); // 0x00407e30
LTRESULT ci_SetPolyHideStatus(HPOLY hPoly, LTBOOL bHide);  // 0x00407f90
LTRESULT ci_GetPolyTextureFlags(HPOLY hPoly, uint32 *pFlags); // 0x00408070
LTRESULT ci_SetModelHook(ModelHookFn fn, void *pUser);     // 0x00408150
LTRESULT ci_GetEngineHook(char *pName, void **pData);      // 0x0040ed50
LTRESULT ci_InitNetworking(char *pDriver, uint32 dwFlags); // 0x00408180
LTRESULT ci_GetServiceList(NetService* &pListHead);        // 0x004081c0
LTRESULT ci_FreeServiceList(NetService *pListHead);        // 0x004081e0
LTRESULT ci_SelectService(HNETSERVICE hNetService);        // 0x00408240
LTRESULT ci_GetSessionList(NetSession* &pListHead, char *pInfo); // 0x004082a0
LTRESULT ci_FreeSessionList(NetSession *pListHead);        // 0x004082c0
LTRESULT ci_AddInternetDriver();                           // 0x004082e0
LTRESULT ci_RemoveInternetDriver();                        // 0x00408320
LTRESULT ci_IsLobbyLaunched(char* sDriver);                // 0x004083c0
LTRESULT ci_GetLobbyLaunchInfo(char* sDriver, void** ppLobbyLaunchData); // 0x00408440
LTRESULT ci_GetTcpIpAddress(char* sAddress, uint32 dwBufferSize); // 0x00408360

#endif  // __CLIENTINTERFACE_H__
