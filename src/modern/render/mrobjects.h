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

#endif
