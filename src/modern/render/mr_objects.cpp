// mr_objects.cpp -- the visible set and the order objects are drawn in.
//
// d3d.ren's per-type Process functions (drawobjects.cpp's handler table)
// only sort an object into a set of the VisibleSet; d3d_FlushObjectQueues
// then draws the sets in a fixed order: solid world models, solid models,
// poly grids and canvases; the chromakeyed ones; then everything
// translucent, far to near; sprites that ignore the depth buffer last.
//
// World models are drawn by mr_scene.cpp, which owns the world's
// polygons; models by mr_models.cpp; sprites by mr_sprites.cpp; particle
// systems, poly grids, line systems and canvases by mr_effects.cpp.

#include <math.h>

#include "mrobjects.h"

static MRObjList g_Sets[VS_COUNT];

void MRObjList::Add(LTObject* pObj)
{
    if (m_nObjects == m_nAlloc)
    {
        uint32_t n = m_nAlloc ? m_nAlloc * 2 : 64;
        LTObject** p = (LTObject**)realloc(m_pObjects, n * sizeof(LTObject*));
        if (!p)
            return;
        m_pObjects = p;
        m_nAlloc = n;
    }
    m_pObjects[m_nObjects++] = pObj;
}

MRObjList* mr_ObjList(MRObjSet eSet)
{
    return &g_Sets[eSet];
}

// ---- translucent objects -----------------------------------------------------------

struct MRTranslucent {
    LTObject* m_pObj;
    MRDrawFn  m_fnDraw;
    float     m_fKey;                   // squared distance from the camera
};
static MRTranslucent* g_pTrans;
static uint32_t g_nTrans, g_nTransAlloc;

void mr_AddTranslucent(LTObject* pObj, MRDrawFn fnDraw)
{
    if (g_nTrans == g_nTransAlloc)
    {
        uint32_t n = g_nTransAlloc ? g_nTransAlloc * 2 : 64;
        MRTranslucent* p = (MRTranslucent*)realloc(g_pTrans, n * sizeof(MRTranslucent));
        if (!p)
            return;
        g_pTrans = p;
        g_nTransAlloc = n;
    }
    MRTranslucent& t = g_pTrans[g_nTrans++];
    t.m_pObj = pObj;
    t.m_fnDraw = fnDraw;
    // A really-close object's position is relative to the camera already.
    LTVector d = pObj->m_Pos;
    if (!(pObj->m_Flags & FLAG_REALLYCLOSE))
    {
        d.x -= g_Frame.m_Cam.m_Pos.x;
        d.y -= g_Frame.m_Cam.m_Pos.y;
        d.z -= g_Frame.m_Cam.m_Pos.z;
    }
    t.m_fKey = d.x * d.x + d.y * d.y + d.z * d.z;
}

// Far to near; really-close objects (drawn in camera space) last.
static int mr_CompareTranslucent(const void* a, const void* b)
{
    const MRTranslucent* x = (const MRTranslucent*)a;
    const MRTranslucent* y = (const MRTranslucent*)b;
    bool cx = (x->m_pObj->m_Flags & FLAG_REALLYCLOSE) != 0;
    bool cy = (y->m_pObj->m_Flags & FLAG_REALLYCLOSE) != 0;
    if (cx != cy)
        return cx ? 1 : -1;
    return x->m_fKey > y->m_fKey ? -1 : x->m_fKey < y->m_fKey ? 1 : 0;
}

static void mr_DrawTranslucentRange(uint32_t nFrom, uint32_t nTo)
{
    if (nFrom >= nTo)
        return;
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_DEPTH | RD_ATTR_FOG);
    rd_Enable(RD_BLEND);
    rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);
    rd_DepthMask(false);
    rd_Disable(RD_ALPHA_TEST);
    for (uint32_t i = nFrom; i < nTo; i++)
        if (g_pTrans[i].m_fnDraw && (g_pTrans[i].m_pObj->m_Flags & FLAG_VISIBLE))
            g_pTrans[i].m_fnDraw(g_pTrans[i].m_pObj);
    rd_PopAttrib();
}

// ---- the set -------------------------------------------------------------------------

void mr_BeginObjects(void)
{
    for (int i = 0; i < VS_COUNT; i++)
        g_Sets[i].Clear();
    g_nTrans = 0;
}

void mr_QueueObject(LTObject* pObj)
{
    switch (pObj->m_ObjectType)
    {
    case OT_WORLDMODEL:
    case OT_CONTAINER:
        mr_ProcessWorldModel(pObj);
        break;
    case OT_MODEL:
        // d3d_ProcessModel: a really-close model is drawn with the others.
        if (pObj->m_ColorA != 255)
            mr_AddTranslucent(pObj, mr_DrawTranslucentModel);
        else if (pObj->m_Flags2 & FLAG2_CHROMAKEY)
            mr_ObjList(VS_MODELS_CHROMAKEY)->Add(pObj);
        else
            mr_ObjList(VS_MODELS)->Add(pObj);
        break;
    case OT_SPRITE:
        if (!cvi(CV_DrawSprites))
            break;
        if (pObj->m_Flags & FLAG_SPRITE_NOZ)
            mr_ObjList(VS_SPRITES_NOZ)->Add(pObj);
        else
            mr_AddTranslucent(pObj, mr_DrawSprite);
        break;
    case OT_LIGHT:
        // d3d_ProcessLight: only while DynamicLight is on.
        if (cvi(CV_DynamicLight))
            mr_ObjList(VS_LIGHTS)->Add(pObj);
        break;
    case OT_POLYGRID:
        if (pObj->m_ColorA != 255)
            mr_AddTranslucent(pObj, mr_DrawPolyGrid);
        else
            mr_ObjList(VS_POLYGRIDS)->Add(pObj);
        break;
    case OT_PARTICLESYSTEM:
        mr_AddTranslucent(pObj, mr_DrawParticleSystem);
        break;
    case OT_LINESYSTEM:
        mr_AddTranslucent(pObj, mr_DrawLineSystem);
        break;
    case OT_CANVAS:
        if (mr_CanvasIsSolid(pObj))
            mr_ObjList(VS_CANVASES)->Add(pObj);
        else
            mr_AddTranslucent(pObj, mr_DrawCanvas);
        break;
    default:
        break;
    }
}

static void mr_DrawSet(MRObjSet eSet, MRDrawFn fnDraw)
{
    MRObjList* pList = mr_ObjList(eSet);
    for (uint32_t i = 0; i < pList->m_nObjects; i++)
        if (pList->m_pObjects[i]->m_Flags & FLAG_VISIBLE)
            fnDraw(pList->m_pObjects[i]);
}

// d3d_FlushObjectQueues: solid world models, models, poly grids and
// canvases; the chromakeyed world models and models; then everything
// translucent, far to near; the sprites that ignore depth last.
void mr_FlushObjects(void)
{
    mr_DrawSet(VS_WORLDMODELS, mr_DrawSolidWorldModel);
    rd_PushAttrib(RD_ATTR_COLOR);
    rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);     // d3d_DrawSolidModels
    mr_DrawSet(VS_MODELS, mr_DrawModel);
    rd_PopAttrib();
    mr_DrawSet(VS_POLYGRIDS, mr_DrawPolyGrid);
    mr_DrawSet(VS_CANVASES, mr_DrawCanvas);

    // Chromakey: texels whose alpha the key cleared are dropped.
    rd_PushAttrib(RD_ATTR_COLOR);
    rd_Enable(RD_ALPHA_TEST);
    rd_AlphaGreater(0.5f);
    mr_DrawSet(VS_WORLDMODELS_CHROMAKEY, mr_DrawSolidWorldModel);
    rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);
    mr_DrawSet(VS_MODELS_CHROMAKEY, mr_DrawModel);
    rd_PopAttrib();

    // Translucent, far to near, blended over what is there without writing
    // depth.
    if (g_nTrans)
    {
        if (cvi(CV_DrawSorted))
            qsort(g_pTrans, g_nTrans, sizeof(MRTranslucent), mr_CompareTranslucent);
        mr_DrawTranslucentRange(0, g_nTrans);
    }

    // Sprites that ignore the depth buffer, over everything.
    if (mr_ObjList(VS_SPRITES_NOZ)->m_nObjects)
    {
        rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_DEPTH);
        rd_Enable(RD_BLEND);
        rd_DepthMask(false);
        mr_DrawSet(VS_SPRITES_NOZ, mr_DrawNoZSprite);
        rd_PopAttrib();
    }
}

// The sky's sprites and poly grids, with the sky camera loaded.
void mr_DrawSkyObject(LTObject* pObj)
{
    if (pObj->m_ObjectType == OT_SPRITE)
        mr_DrawSkySprite(pObj);
    else if (pObj->m_ObjectType == OT_POLYGRID)
        mr_DrawPolyGrid(pObj);
}
