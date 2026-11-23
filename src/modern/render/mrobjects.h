// mrobjects.h -- the visible set: the objects a frame draws, by kind, and
// the order they are drawn in (d3d.ren's VisibleSet, tagnodes.cpp, and
// d3d_FlushObjectQueues, drawobjects.cpp).

#ifndef MROBJECTS_H
#define MROBJECTS_H

#include "mrscene.h"

enum MRObjSet {
    VS_WORLDMODELS, VS_WORLDMODELS_CHROMAKEY,
    VS_MODELS, VS_MODELS_CHROMAKEY, VS_CLOSE_MODELS, VS_CLOSE_MODELS_CHROMAKEY,
    VS_SPRITES_NOZ,
    VS_POLYGRIDS, VS_CANVASES,
    VS_LIGHTS,
    VS_COUNT
};

struct MRObjList {
    LTObject** m_pObjects;
    uint32_t   m_nObjects, m_nAlloc;
    void Add(LTObject* pObj);
    void Clear() { m_nObjects = 0; }
};
MRObjList* mr_ObjList(MRObjSet eSet);

// Translucent objects are drawn last, far to near, each by its own function.
typedef void (*MRDrawFn)(LTObject* pObj);
void mr_AddTranslucent(LTObject* pObj, MRDrawFn fnDraw);

void mr_BeginObjects(void);                    // empties the set
void mr_QueueObject(LTObject* pObj);           // the type's Process function
void mr_FlushObjects(void);                    // draws the set
void mr_DrawSkyObject(LTObject* pObj);         // a sky sprite or poly grid

// mr_sprites.cpp
bool mr_LightGridSample(const LTVector& pos, float* pOut);
void mr_DynamicLightAdd(const LTVector& pos, float* pOut);
void mr_ObjectBlend(LTObject* pObj);
SharedTexture* mr_SpriteFrame(LTObject* pObj);
void mr_DrawSprite(LTObject* pObj);
void mr_DrawNoZSprite(LTObject* pObj);
void mr_DrawSkySprite(LTObject* pObj);

// mr_effects.cpp
void mr_ObjectMatrix(LTObject* pObj, const LTVector& pos, const LTVector& scale, LTMatrix* pM);
void mr_DrawParticleSystem(LTObject* pObj);
void mr_DrawPolyGrid(LTObject* pObj);
void mr_DrawLineSystem(LTObject* pObj);
void mr_DrawCanvas(LTObject* pObj);
bool mr_CanvasIsSolid(LTObject* pObj);

// mr_models.cpp
void mr_DrawModel(LTObject* pObj);
void mr_DrawTranslucentModel(LTObject* pObj);
void mr_BeginReallyClose(LTObject* pObj);     // the camera-space view for it
void mr_EndReallyClose(void);

// mr_shadows.cpp: a model's shadow on the world (drawmodelshadows.cpp). The
// nodes are the model's, in the world.
void mr_DrawModelShadow(LTObject* pInst, const void* pModel, const LTMatrix* pNodes, uint32_t nNodes,
                        uint32_t iLOD);

#endif
