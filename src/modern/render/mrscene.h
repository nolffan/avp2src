// mrscene.h -- the 3D half's shared declarations: the per-world context,
// the lightmap atlases, the camera and the frame.

#ifndef MRSCENE_H
#define MRSCENE_H

#include "mr.h"
#include "ltworld.h"

// ---- lightmaps ----------------------------------------------------------------------

#define LM_MAXTEXELS 1024               // 32x32: the decompressors' cap

struct MRLMAtlas {
    uint32_t    m_Id;
    uint32_t    m_nSize;                // square
    uint32_t    m_ShelfX, m_ShelfY, m_ShelfH;
    uint8_t*    m_pRGB;                 // m_nSize^2 * 3, kept for animated updates
    bool        m_bDirty;
    uint32_t    m_DirtyY0, m_DirtyY1;   // the rows changed since the last upload
    MRLMAtlas*  m_pNext;
};

// Where a lightmapped poly's texels are; hung on WorldPoly::m_pLMPage,
// which is the renderer's field.
struct MRPolyLM {
    MRLMAtlas* m_pAtlas;
    uint16_t   m_X, m_Y;                // the lightmap's first texel (inside its border)
    bool       m_bLit;                  // holds dynamic light this frame
};

// A dynamic light as a BSP's polys see it: in that BSP's own space.
struct MRWorldLight {
    LTVector  m_Pos;
    float     m_fRadius;
    uint8_t   m_Color[3];
    bool      m_bSolid;                 // FastLight or FLAG_ONLYLIGHTWORLD: the flat colour
    bool      m_bFrontOnly;             // FLAG_DONTLIGHTBACKFACING
};
#define MR_MAXWORLDLIGHTS 40            // d3d.ren's cap

struct MRContext {
    MainWorld*  m_pWorld;
    uint16_t    m_FrameCode;
    MRLMAtlas*  m_pAtlases;
    MRPolyLM*   m_pPolyLMs;             // one block for every lightmapped poly
    uint32_t    m_nPolyLMs;
    uint32_t    m_nContextGen;          // the device the atlases were made on
    WorldPoly** m_pLitPolys;            // polys whose atlas texels hold dynamic light
    uint32_t    m_nLitPolys, m_nLitAlloc;
};

bool mr_BuildLightmaps(MRContext* pContext);
void mr_FreeLightmaps(MRContext* pContext);
void mr_UpdateLightmaps(MRContext* pContext);   // changed animations, before drawing
void mr_UploadLightmaps(MRContext* pContext);   // the atlas rows changed since the last call
void mr_LightPolyLM(MRContext* pContext, WorldPoly* pPoly, const MRWorldLight* pLights,
                    uint32_t nLights);
void mr_LMVectors(const WorldPoly* pPoly, LTVector* pP, LTVector* pQ);
LightAnim* mr_FindLightAnim(MainWorld* pWorld, const char* pName);

// ---- the frame ------------------------------------------------------------------------

struct MRCamera {
    LTVector m_Pos;
    float    m_R[3][3];                 // camera -> world: columns right, up, forward
    float    m_fTanX, m_fTanY;          // tan of the half field of view
    float    m_fNear, m_fFar;
    float    m_fScale;                  // view scale (the sky's SkyScale)
    int      m_nViewX, m_nViewY, m_nViewW, m_nViewH;   // viewport, from the screen's top left
    LTPlane  m_Planes[6];               // world-space frustum, normals in
    bool     m_bMirror;                 // seen through an odd number of mirrors
};

struct MRFrame {
    SceneDesc*  m_pScene;
    MRContext*  m_pContext;
    MRCamera    m_Cam;
    MRCamera    m_SkyCam;               // while sky objects draw
    uint16_t    m_WorldFrameCode;
    uint32_t    m_ObjectFrameCode;
    uint32_t    m_nSkyPolys;            // visible sky portal polys: the sky pass runs if any
    float       m_fLMGrid;
    bool        m_bLightmaps;
    bool        m_bFog;
    uint8_t     m_WorldColor[3];        // SceneDesc::m_WorldColor * 255
    uint8_t     m_GammaRamp[3][256];    // the vertex colours (doubled under Saturate)
    uint8_t     m_LightRamp[3][256];    // the same, never doubled
    int         m_nPortalDepth;         // mirrors drawn inside this one
};
extern MRFrame g_Frame;

// The camera's projection and view; its depth into the depth buffer's
// MR_CLOSE_DEPTH..1, the front kept for what really-close objects have
// nearer than its near plane (mr_BeginReallyClose).
#define MR_CLOSE_DEPTH (1.0f / 16.0f)
void mr_LoadCamera(const MRCamera* pCam);
void mr_MultMatrix(const LTMatrix& M);
void mr_RotationMatrix(const LTRotation& q, float m[3][3]);
bool mr_SphereVisible(const LTVector& c, float r);
bool mr_ShouldDraw(LTObject* pObj);
void mr_ObjectFog(LTObject* pObj, bool bBegin);
void mr_SetBaseTexture(MRTexture* pTex);
MRTexture* mr_SurfaceTexture(Surface* pSurf);
void mr_ProcessWorldModel(LTObject* pObj);
void mr_DrawSolidWorldModel(LTObject* pObj);
void mr_DrawTranslucentWorldModel(LTObject* pObj);
bool mr_SeesSky(const LTVector& vFrom, const LTVector& vRay);
void mr_ReflectCoords(const LTVector& vCam, const LTVector& pos, const LTVector& N, float* pS, float* pT);
MRTexture* mr_EnvMapOf(SharedTexture* pBase);
float mr_WorldSize(void);
// The fog factor of a world-space point (d3d.ren's g_pfnCalcFog: by
// distance, or the vertical fog, or Primal Hunt's fog volumes): 1 none.
float mr_FogFactor(const LTVector& pos);

#endif
