// *********************************************************************** //
//  ph/ph_decls.h -- Primal Hunt's d3d.ren: the cross-file declarations
//  proven against their definitions.
//
//  Each translation unit declares what it uses of the others locally, and
//  a local declaration that differs from the definition (a parameter or
//  return type, a struct/class key, a global's type) leaves the reference
//  unresolved in the link, under the definition's name. Every declaration
//  here names its definition's exact symbol, read back from the link's map
//  (Release/d3d.map). A file that includes this after its own local
//  declarations gets a compile error where one of them disagrees in a
//  return type or a global's type (C2556, C2371); a parameter type that
//  differs would only declare an overload, so declare parameters from here.
//
//  Only the types' names: each file keeps its own view of their layouts.
//  Include it after "ltbasedefs.h".
// *********************************************************************** //

#ifndef __PH_DECLS_H__
#define __PH_DECLS_H__

struct IDirect3DDevice7;
struct IDirectDraw7;
struct IDirectDrawSurface7;
struct RenderStruct;
struct D3DRenderData;
struct RenderBlitRequest;
struct SharedTexture;
struct SceneDesc;
struct SceneWorld;
struct VisibleSet;
class PFormat;
class CRenderStateList;


// common_init.cpp (TU11)
extern IDirectDraw7*        g_pDirectDraw;                  // 0x1005d2a8
extern IDirectDrawSurface7* g_pFrontBuffer;                 // 0x1005d2ac
extern IDirectDrawSurface7* g_pPrimarySurface;              // 0x1005d2b0
extern IDirectDrawSurface7* g_pBackBuffer;                  // 0x1005d2b4
extern PFormat              g_ScreenFormat;                 // 0x1005d260

// common_draw.cpp (TU10)
extern SceneWorld*          g_pSceneWorld;                  // 0x1005c208

// common_stuff.cpp (TU12)
extern RenderStruct*        g_pStruct;                      // 0x1005df08
extern int32                g_CV_MaxTextureSize;            // 0x1005d998
extern int32                g_CV_Saturate;                  // 0x1005d384
void    AddDebugMessage(int32 debugLevel, const char* pMsg, ...);   // 0x10013b13

// conparse.cpp (TU13)
uint32  CountBits(uint32 val);                              // 0x10013c60

// d3d_draw.cpp (TU15)
void    d3d_TermTranslucentStates();                        // 0x10014550
void    d3d_Clear(LTRect* pRect, uint32 flags, LTVector* pClearColor);  // 0x10014d70

// d3d_init.cpp (TU17)
extern IDirect3DDevice7*    g_pD3DDevice;                   // 0x100638c8
extern LTBOOL               g_bIn3D;                        // 0x100638d8
void    d3d_RebindWorld(void* pWorld);                      // 0x1001c3a0
void    d3d_TermSubsystems();                               // 0x1001c450
LTBOOL  d3d_InitSubsystems();                               // 0x1001c480

// d3d_optimizedsurface.cpp (TU18)
LTBOOL  d3d_OptimizeSurface(D3DRenderData* pData, uint32 transparentColor);  // 0x1001cff4

// d3d_surface.cpp (TU19)
D3DRenderData* d3d_CreateSurface(int width, int height);   // 0x1001e66f
void    d3d_DeleteSurface(D3DRenderData* pData);            // 0x1001e71d
void    d3d_GetSurfaceInfo(D3DRenderData* pData, uint32* pWidth, uint32* pHeight, long* pPitch);  // 0x1001e73e
void*   d3d_LockSurface(D3DRenderData* pData);              // 0x1001e76e
void    d3d_UnlockSurface(D3DRenderData* pData);            // 0x1001e7a4
void    d3d_BlitFromScreen(RenderBlitRequest* pRequest);    // 0x1001e871
void    d3d_BlitToScreen(RenderBlitRequest* pRequest);      // 0x1001ea0c
LTBOOL  d3d_WarpToScreen(RenderBlitRequest* pRequest);      // 0x1001ea75
void    d3d_GetScreenFormat(PFormat* pFormat);              // 0x1001eab3

// d3d_texture.cpp (TU20)
LTBOOL  d3d_GetTextureScale(SharedTexture* pTexture, uint32 iStage, float* pU, float* pV);  // 0x10022760

// d3d_savedstates.cpp (TU21)
extern CRenderStateList     g_SavedStates;                  // 0x10069728

// drawgouraud.cpp (TU24)
void    d3d_QueueGouraudPoly(void* pPoly);                  // 0x100238c5

// tagnodes.cpp (TU42)
VisibleSet* d3d_GetVisibleSet();                            // 0x1003abb0

// Not yet Primal Hunt's: nolfsrc's w28_100256d0_qipni.cpp (NOLF's).
LTBOOL  d3d_RenderScene(SceneDesc* pScene);                 // 0x10018520

#endif  // __PH_DECLS_H__
