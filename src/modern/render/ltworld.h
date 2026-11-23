// ltworld.h -- the engine's scene, world and object data of AvP2 and
// Primal Hunt, as the renderer reads them.
//
// The renderer never loads a world; it walks what the engine loaded. The
// layouts are those that our d3d.ren source reads (src/engine/d3d/ph/,
// one source for both games: their layouts are the same) and the engine
// writes (src/engine/lithtech/, src/engine/server/). nolfsrc's ltworld.h
// (NOLF) is the starting point; the comments say where AvP2 differs.

#ifndef LTWORLD_H
#define LTWORLD_H

#include "ltrender.h"

struct LTRotation { float x, y, z, w; };
struct LTPlane { LTVector m_Normal; float m_Dist; };
struct LTLink { LTLink* m_pPrev; LTLink* m_pNext; void* m_pData; };
struct LTMatrix { float m[4][4]; };     // row-major, column vectors: world = M * local

// ---- what RenderScene gets (0x104 bytes; NOLF 0xec) ---------------------------

struct SkyDef { LTVector m_Min, m_Max, m_ViewMin, m_ViewMax; };
static_assert(sizeof(SkyDef) == 0x30, "SkyDef");

// The game's model hook (ltbasedefs.h ModelHookData): it can change the
// light add, the object colour and the flags for one draw.
struct ModelHookData {
    LTObject* m_hObject;                // 0x00
    uint32_t  m_Flags;                  // 0x04  MHF_USETEXTURE 1
    uint32_t  m_ObjectFlags;            // 0x08  a copy of the object's flags
    LTVector* m_LightAdd;               // 0x0c  0..255
    LTVector* m_ObjectColor;            // 0x10  0..255
};
#define MHF_USETEXTURE 1
typedef void (*ModelHookFn)(ModelHookData* pData, void* pUser);

struct SceneDesc {
    int32_t   m_DrawMode;               // 0x00  1 the world from the camera, 2 the object list only
    uint32_t* m_pTicks[6];              // 0x04
    LTVector  m_ModelAdd;               // 0x1c  the models' light add (0..255)
    LTVector  m_ModelDirAdd;            // 0x28
    void*     m_pRenderContext;         // 0x34  what CreateWorld returned
    LTVector  m_GlobalLightDir;         // 0x38
    LTVector  m_GlobalLightColor;       // 0x44
    LTVector  m_GlobalLightScale;       // 0x50  the light scale pass (and the ramps' rebuild test)
    LTVector  m_WorldColor;             // 0x5c  AvP2: the world's colour (0..1): the vertex
                                        //       colour ramps, the lightmapped polys' colour
    LTVector  m_WMAmbient;              // 0x68  AvP2: added to world models' colours (0..255)
    LTVector  m_GlobalLightAdd;         // 0x74  the camera's light add (the light add pass)
    float     m_FrameTime;              // 0x80
    SkyDef    m_SkyDef;                 // 0x84
    LTObject** m_SkyObjects;            // 0xb4
    int32_t   m_nSkyObjects;            // 0xb8
    LTRect    m_Rect;                   // 0xbc  viewport, screen pixels
    float     m_xFov, m_yFov;           // 0xcc  full angles, radians (the engine's Hor+, M-1)
    float     m_FarZ;                   // 0xd4
    LTVector  m_Pos;                    // 0xd8  camera
    LTRotation m_Rotation;              // 0xe4
    LTObject** m_pObjectList;           // 0xf4  draw mode 2
    int32_t   m_ObjectListSize;         // 0xf8
    ModelHookFn m_ModelHookFn;          // 0xfc
    void*     m_ModelHookUser;          // 0x100
};
static_assert(sizeof(SceneDesc) == 0x104, "SceneDesc");

#define DRAWMODE_NORMAL     1
#define DRAWMODE_OBJECTLIST 2

// ---- the world ---------------------------------------------------------------

struct WorldPoly;
struct Surface;
struct WorldBsp;
struct Node;

struct CMoArrayHdr {                    // the engine's dynamic array
    void*    m_pVtbl;
    void*    m_pArray;
    uint32_t m_nElements;
    uint32_t m_Cache[2];
};
static_assert(sizeof(CMoArrayHdr) == 0x14, "CMoArrayHdr");

struct WorldData {                      // one per BSP world: the main world and every world model
    uint32_t  m_Flags;
    WorldBsp* m_pWorldBsp;              // +0x04 the geometry
};

// A light animation (0x60): every lightmap is one. LightAnim_BASE is the
// static lightmaps; its frame 0 has one entry per lightmapped poly. AvP2
// keeps the lightmaps as 32-bit runs (d3d.ren's d3d_DecompressLM32), and a
// shadow-map animation's frames as byte masks.
struct LightAnimFrame {                 // 0x18, one per poly (or per vertex group) and frame
    uint8_t*  m_pData;                  // 0x00
    uint16_t  m_nSize;                  // 0x04
    uint16_t  m_06;
    uint8_t*  m_pR;                     // 0x08 a vertex animation's colours
    uint8_t*  m_pG;                     // 0x0c
    uint8_t*  m_pB;                     // 0x10
    uint8_t   m_nVerts;                 // 0x14
    uint8_t   m_pad15[3];
};
static_assert(sizeof(LightAnimFrame) == 0x18, "LightAnimFrame");

struct LightAnimRef { uint16_t m_iAnim, m_iPoly; };     // WorldPoly+0x0c: {anim, its poly index}

struct LightAnim {                      // 0x60
    char             m_Name[32];        // 0x00
    uint32_t         m_bShadowMap;      // 0x20 frames are masks lit by a moving light
    LightAnimFrame** m_Frames;          // 0x24 [frame] -> [poly]
    uint8_t          m_nFrames;         // 0x28
    uint8_t          m_pad29[3];
    void*            m_pRefs;           // 0x2c
    uint16_t         m_nPolies;         // 0x30
    uint16_t         m_pad32;
    int32_t          m_iFrames[2];      // 0x34 -1: off
    int32_t          m_Blend;           // 0x3c 0..255 between the two frames
    float            m_fBrightness;     // 0x40 the layer is left out under 0.02
    LTVector         m_vLightPos;       // 0x44
    LTVector         m_vLightColor;     // 0x50 0..255
    float            m_fLightRadius;    // 0x5c
};
static_assert(sizeof(LightAnim) == 0x60, "LightAnim");

// MainWorld, as far as the renderer reads it. Its vtable: slot 1 (+0x04)
// GetVisBSP (thiscall).
struct MainWorld {
    void*        m_pVtbl;               // 0x000
    CMoArrayHdr  m_LightAnims;          // 0x004  LightAnim records
    CMoArrayHdr  m_LMArrays[4];         // 0x018
    uint32_t     m_68;                  // 0x068
    uint8_t      m_WorldTree[0x7c];     // 0x06c  (+0x4c in it: the root's size, MainWorld+0xb8)
    LTLink       m_EffectHead;          // 0x0e8
    void*        m_pEffects;            // 0x0f4
    float        m_LMGridSize;          // 0x0f8  lightmap texel size in world units
    uint8_t      m_LightTable[0x48];    // 0x0fc  the light grid
    LTVector     m_ExtentsMin;          // 0x144
    LTVector     m_ExtentsMax;          // 0x150
    LTVector     m_InvExtents;          // 0x15c
    LTVector     m_BoxMin, m_BoxMax;    // 0x168
    uint32_t     m_Flags;               // 0x180  0x2 has a PVS, 0x4 has lightmaps
    uint32_t     m_184;                 // 0x184
    CMoArrayHdr  m_Worlds;              // 0x188  WorldData*: array +0x18c, count +0x190
    uint8_t      m_pad19c[0x1a0 - 0x19c];
    WorldPoly**  m_SkyPortals;          // 0x1a0  every sky portal poly
    uint32_t     m_nSkyPortals;         // 0x1a4
};
static_assert(__builtin_offsetof(MainWorld, m_LMGridSize) == 0xf8, "MainWorld");
static_assert(__builtin_offsetof(MainWorld, m_Flags) == 0x180, "MainWorld");
static_assert(__builtin_offsetof(MainWorld, m_Worlds) == 0x188, "MainWorld");
static_assert(__builtin_offsetof(MainWorld, m_SkyPortals) == 0x1a0, "MainWorld");

#define MAINWORLD_HASVIS        0x2
#define MAINWORLD_HASLIGHTMAPS  0x4

inline WorldData* mw_World(MainWorld* pWorld, uint32_t i)
{
    return ((WorldData**)pWorld->m_Worlds.m_pArray)[i];
}
inline LightAnim* mw_LightAnims(MainWorld* pWorld) { return (LightAnim*)pWorld->m_LightAnims.m_pArray; }

typedef WorldBsp* (__thiscall* GetVisBspFn)(MainWorld* pWorld);
inline WorldBsp* mw_GetVisBsp(MainWorld* pWorld)
{
    return (*(GetVisBspFn**)pWorld)[1](pWorld);
}

struct Node {                           // 0x18
    WorldPoly* m_pPoly;                 // 0x00 splitting poly
    Node*      m_Sides[2];              // 0x04 [0] back, [1] front
    void*      m_Objects[2];            // 0x0c object list head (prev, next)
    uint16_t   m_iLeaf;                 // 0x14
    uint8_t    m_Flags;                 // 0x16 1 = the "in" sentinel, 2 = "out": a walk stops at flags & 3
    uint8_t    m_PlaneType;             // 0x17
};
static_assert(sizeof(Node) == 0x18, "Node");

struct Leaf {                           // 0x30
    LTVector   m_Center;                // 0x00
    float      m_Radius;                // 0x0c
    void*      m_LeafLists;             // 0x10
    LTLink     m_Objects;               // 0x14 links whose data's +0x18 is the object
    WorldPoly** m_Polies;               // 0x20
    uint32_t   m_nPolies;               // 0x24
    float      m_28;                    // 0x28
    uint16_t   m_FrameCode;             // 0x2c
    uint16_t   m_nLeafLists;            // 0x2e
};
static_assert(sizeof(Leaf) == 0x30, "Leaf");

// WorldBsp has a vtable in AvP2: slot 0 GetBspType (0 a BSP, 1 terrain),
// slot 1 GetRootNode, slot 10 (+0x28) GetBoundRadius. A terrain world keeps
// its polys at +0x54.
struct WorldBsp {
    void*      m_pVtbl;                 // 0x000
    uint8_t    m_pad04[0x54 - 0x04];
    WorldPoly** m_TerrainPolies;        // 0x054
    uint32_t   m_nTerrainPolies;        // 0x058
    uint8_t    m_pad5c[0x70 - 0x5c];
    Node*      m_Nodes;                 // 0x070
    uint32_t   m_nNodes;                // 0x074
    uint8_t    m_pad78[0x88 - 0x78];
    Leaf*      m_Leafs;                 // 0x088
    uint32_t   m_nLeafs;                // 0x08c
    uint8_t    m_pad90[0x9c - 0x90];
    Node*      m_RootNode;              // 0x09c
    WorldPoly** m_Polies;               // 0x0a0
    uint32_t   m_nPolies;               // 0x0a4
};
static_assert(__builtin_offsetof(WorldBsp, m_Polies) == 0xa0, "WorldBsp");

typedef uint32_t (__thiscall* BspTypeFn)(WorldBsp* pBsp);
typedef float (__thiscall* BspRadiusFn)(WorldBsp* pBsp);
inline uint32_t bsp_Type(WorldBsp* p) { return (*(BspTypeFn**)p)[0](p); }
inline float bsp_BoundRadius(WorldBsp* p) { return (*(BspRadiusFn**)p)[10](p); }

// Surface (0x40; NOLF 0x48 with a vtable and a plane pointer).
struct Surface {
    LTVector       O, P, Q;             // 0x00 texture origin and axes (inferred)
    uint8_t        m_24[8];             // 0x24
    SharedTexture* m_pTexture;          // 0x2c NULL: flat
    uint32_t       m_Flags;             // 0x30 SURF_*
    uint16_t       m_TextureFlags;      // 0x34
    uint16_t       m_36, m_38;          // 0x36
    uint16_t       m_PortalId;          // 0x3a 0x7fff not a portal; (& 0x7fff) == 0x7ffe a mirror;
                                        //      0x8000: the portal poly is drawn over the mirror's view
    uint8_t        m_pad3c[4];
};
static_assert(sizeof(Surface) == 0x40, "Surface");

#define SURF_SOLID        0x1
#define SURF_INVISIBLE    0x4
#define SURF_TRANSLUCENT  0x8           // a world model whose first poly has it is translucent
#define SURF_SKY          0x10          // a sky portal: the sky shows here; never drawn
#define SURF_LIGHTMAP     0x80
#define SURF_PANNINGSKY   0x8000        // the sky-pan texture over it (the clouds' shadows)
#define SURF_ADDITIVE     0x80000       // a translucent world model: drawn additively
#define SURF_TIMEOFDAY    0x100000

#define PORTAL_NONE       0x7fff
#define PORTAL_MIRROR     0x7ffe

struct WorldPolyVertex {                // 0x18
    LTVector* m_pPos;                   // 0x00
    float     m_U, m_V;                 // 0x04 base texture, texels
    float     m_LMU, m_LMV;             // 0x0c renderer-owned
    uint8_t   m_B, m_G, m_R, m_A;       // 0x14 vertex colour
};
static_assert(sizeof(WorldPolyVertex) == 0x18, "WorldPolyVertex");

// WorldPoly (the vertices at 0x58; NOLF 0x54): AvP2 has a plane pointer
// at 0x28.
struct WorldPoly {
    uint8_t    m_00[0x0c];              // 0x00 renderer scratch
    LightAnimRef* m_pLightAnims;        // 0x0c
    uint32_t   m_nLightAnims;           // 0x10
    uint16_t   m_Flags;                 // 0x14 bits 11..13 the lightmap basis; 0x4000 its vertex
                                        //      light animations changed; 0x8000 the lightmap changed
    uint16_t   m_16;
    LTVector   m_Center;                // 0x18 bounding sphere
    float      m_Radius;                // 0x24
    LTPlane*   m_pPlane;                // 0x28
    Surface*   m_pSurface;              // 0x2c
    void*      m_pLights;               // 0x30 the dynamic lights that touch it (d3d.ren's)
    uint32_t   m_34;                    // 0x34
    LTVector   m_LMOrigin;              // 0x38
    uint16_t   m_44;                    // 0x44
    uint16_t   m_FrameCode;             // 0x46
    void*      m_pLMPage;               // 0x48 renderer-owned
    uint8_t    m_LMWidth, m_LMHeight;   // 0x4c
    uint8_t    m_LMPageX, m_LMPageY;    // 0x4e renderer-owned
    WorldPolyVertex* m_pFixVerts;       // 0x50 the T-junction vertices (FixTJunc)
    uint16_t   m_nVerts;                // 0x54
    uint16_t   m_nFixVerts;             // 0x56
    WorldPolyVertex m_Verts[1];         // 0x58 m_nVerts of them
};
static_assert(__builtin_offsetof(WorldPoly, m_pSurface) == 0x2c, "WorldPoly");
static_assert(__builtin_offsetof(WorldPoly, m_LMOrigin) == 0x38, "WorldPoly");
static_assert(__builtin_offsetof(WorldPoly, m_Verts) == 0x58, "WorldPoly");

#define WPF_VERTLIGHTANIM 0x4000
#define WPF_LMCHANGED     0x8000

// ---- objects ---------------------------------------------------------------------

struct Attachment {
    LTVector   m_OffsetPos;             // 0x00
    LTRotation m_OffsetRot;             // 0x0c
    uint16_t   m_nParentID, m_nChildID; // 0x1c
    int32_t    m_iNode;                 // 0x20
    Attachment* m_pNext;                // 0x24
};

// The engine's object (objtypes.h): NOLF's to 0xbc, NOLF's plus 8 from
// 0x188. The type-specific fields start at 0x1b0 (NOLF 0x1a8).
struct LTObject {
    void*       m_pVtbl;                // 0x000
    uint8_t     m_pad04[0x58 - 0x04];
    uint32_t    m_FrameCode;            // 0x058 "processed this frame" (the object frame code)
    uint8_t     m_pad5c[0x88 - 0x5c];
    uint32_t    m_Flags;                // 0x088 FLAG_*
    uint32_t    m_Flags2;               // 0x08c FLAG2_*
    uint32_t    m_UserFlags;            // 0x090
    uint8_t     m_ColorR, m_ColorG, m_ColorB, m_ColorA; // 0x094
    Attachment* m_pAttachments;         // 0x098
    LTRotation  m_Rotation;             // 0x09c
    LTVector    m_Scale;                // 0x0ac
    uint16_t    m_ObjectID;             // 0x0b8
    uint16_t    m_padba;
    int8_t      m_ObjectType;           // 0x0bc
    uint8_t     m_padbd[0x124 - 0xbd];
    uint32_t    m_InternalFlags;        // 0x124 IFLAG_INSKY 0x400
    uint8_t     m_pad128[0x188 - 0x128];
    uint8_t     m_ClientFlags;          // 0x188 CF_*
    uint8_t     m_pad189[0x190 - 0x189];
    void*       m_pInsideBsp;           // 0x190 ends an object list walk
    LTVector    m_Pos;                  // 0x194
    LTVector    m_Dims;                 // 0x1a0
    float       m_Radius;               // 0x1ac
};
static_assert(__builtin_offsetof(LTObject, m_ObjectType) == 0xbc, "LTObject");
static_assert(__builtin_offsetof(LTObject, m_Pos) == 0x194, "LTObject");
static_assert(sizeof(LTObject) == 0x1b0, "LTObject");

#define IFLAG_INSKY 0x400
#define CF_INSIDERADIUS 0x8
#define CF_SOLIDCANVAS  0x10

// World models and containers (OT_WORLDMODEL, OT_CONTAINER).
struct WorldModelInstance : LTObject {
    WorldBsp*   m_pWorldBsp;            // 0x1b0 drawn in its own space
    WorldBsp*   m_pOriginalBsp;         // 0x1b4
    void*       m_pInterface;           // 0x1b8
    uint8_t     m_pad1bc[0x1f8 - 0x1bc];
    LTMatrix    m_Transform;            // 0x1f8 model -> world
    LTMatrix    m_BackTransform;        // 0x238 world -> model
};
static_assert(sizeof(WorldModelInstance) == 0x278, "WorldModelInstance");  // so m_Transform is at 0x1f8

// The light (OT_LIGHT): its radius.
inline float obj_LightRadius(LTObject* p) { return *(float*)((uint8_t*)p + 0x1b0); }

enum {
    OT_NORMAL, OT_MODEL, OT_WORLDMODEL, OT_SPRITE, OT_LIGHT, OT_CAMERA,
    OT_PARTICLESYSTEM, OT_POLYGRID, OT_LINESYSTEM, OT_CONTAINER, OT_CANVAS, NUM_OBJECTTYPES
};

#define FLAG_VISIBLE         0x1
#define FLAG_SHADOW          0x2     // a model's: it casts a shadow (through the hook)
#define FLAG_UNSIGNED        0x2     // a poly grid's
#define FLAG_SPRITEBIAS      0x4
#define FLAG_MODELTINT       0x4
#define FLAG_ROTATEABLESPRITE 0x8
#define FLAG_DETAILTEXTURE   0x8
#define FLAG_SPRITE_NOZ      0x10
#define FLAG_GLOWSPRITE      0x20
#define FLAG_ENVIRONMENTMAP  0x20    // a model's: chrome
#define FLAG_REALLYCLOSE     0x40
#define FLAG_FOGDISABLE      0x80
#define FLAG_NOLIGHT         0x200
#define FLAG_PORTALVISIBLE   0x400

// A light's own meanings of the same bits (OT_LIGHT).
#define FLAG_SOLIDLIGHT          0x10   // unattenuated (FastLight)
#define FLAG_ONLYLIGHTWORLD      0x20   // not objects
#define FLAG_DONTLIGHTBACKFACING 0x40
#define FLAG_ONLYLIGHTOBJECTS    0x80   // not the world

// AvP2's FLAG2 bits (NOLF's differ from 0x20 up).
#define FLAG2_PORTALINVISIBLE 0x1
#define FLAG2_CHROMAKEY       0x2
#define FLAG2_ADDITIVE        0x4
#define FLAG2_MULTIPLY        0x8
#define FLAG2_DYNAMICDIRLIGHT 0x80
#define FLAG2_SKYOBJECT       0x100
#define FLAG2_SPRITE_TROTATE  0x200
#define FLAG2_CHAMELEON       0x800     // Primal Hunt
#define FLAG2_ENVOFFSET       0x1000    // Primal Hunt
#define FLAG2_ADDITIVEALPHA   0x4000    // Primal Hunt
#define FLAG2_FOGVOLUME       0x8000    // Primal Hunt: a world model that is a fog volume

// ---- the engine's visibility query ---------------------------------------------------

struct WorldTreeNode;
struct VisQueryRequest {                // d3d.ren's FindObjInfo, 0x48
    uint32_t m_Flags;                                       // 0x00
    LTVector m_Viewpoint;                                   // 0x04
    float    m_ViewRadius;                                  // 0x10
    void   (*m_ObjectCB)(LTObject* pObj);                   // 0x14
    void   (*m_AddObjects)(LTLink* pList, LTObject*** ppArray, uint32_t* pnCount); // 0x18
    void*    m_pUser;                                       // 0x1c
    void   (*m_LeafCB)(Leaf* pLeaf);                        // 0x20
    int    (*m_BoxFilter)(void* pNode);                     // 0x24 a box: centre +0x0c, half dims +0x18
    int    (*m_NodeFilter)(WorldTreeNode* pNode);           // 0x28 a sphere: centre +0x30, radius +0x40
    uint8_t  m_State[0x48 - 0x2c];                          // the walk's own
};
static_assert(sizeof(VisQueryRequest) == 0x48, "VisQueryRequest");
// The world tree at MainWorld+0x6c: slot 5 (+0x14) FindObjectsInFrustum.
typedef void (__thiscall* VisQueryFn)(void* pTree, VisQueryRequest* pRequest);
inline void mw_DoVisQuery(MainWorld* pWorld, VisQueryRequest* pRequest)
{
    void* pTree = pWorld->m_WorldTree;
    VisQueryFn fn = (*(VisQueryFn**)pTree)[5];
    fn(pTree, pRequest);
}
// Slot 2 (+0x08) FindObjectsInBox: list 0 the objects, list 1 the world's
// static lights.
struct FindObjInfo {                    // 0x34
    uint32_t  m_00;
    uint32_t  m_iObjArray;              // 0x04
    LTVector  m_Min, m_Max;             // 0x08
    LTVector  m_20;
    void    (*m_CB)(void* pObj, void* pUser);   // 0x2c
    void*     m_pUser;                  // 0x30
};
static_assert(sizeof(FindObjInfo) == 0x34, "FindObjInfo");
typedef void (__thiscall* FindObjectsFn)(void* pTree, FindObjInfo* pInfo);
inline void mw_FindObjectsInBox(MainWorld* pWorld, FindObjInfo* pInfo)
{
    void* pTree = pWorld->m_WorldTree;
    FindObjectsFn fn = (*(FindObjectsFn**)pTree)[2];
    fn(pTree, pInfo);
}

// A static light as the world tree files it.
struct StaticLight {
    uint8_t   m_pad[0x68];
    LTVector  m_Pos;                    // 0x68
    float     m_fRadius;                // 0x74
    LTVector  m_Color;                  // 0x78 0..255
    LTVector  m_Dir;                    // 0x84 a spot light's
    float     m_fCone;                  // 0x90 cos of its half angle; outside (-1, 1) not a spot
    LTVector  m_OuterColor;             // 0x94 towards the cone's edge
};

inline const LTVector& wtn_Center(WorldTreeNode* p) { return *(LTVector*)((uint8_t*)p + 0x30); }
inline float wtn_Radius(WorldTreeNode* p) { return *(float*)((uint8_t*)p + 0x40); }

#endif
