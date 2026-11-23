// mr_scene.cpp -- the world and the scene.
//
// Stage 1 of the port (README.md): the world's slots are here, but the
// scene is not drawn yet. CreateWorld gives the engine a context, so that
// a level loads; RenderScene draws nothing.

#include "mr.h"

struct MRContext {
    void* m_pWorld;
};

void* mr_CreateWorld(void** ppWorld)
{
    MRContext* pContext = (MRContext*)calloc(1, sizeof(MRContext));
    if (pContext && ppWorld)
        pContext->m_pWorld = *ppWorld;
    return pContext;
}

void mr_DestroyWorld(void* pContext)
{
    free(pContext);
}

void mr_RebindWorld(void* pContext)
{
    (void)pContext;
}

LTBOOL mr_RenderScene(SceneDesc* pScene)
{
    mr_Debug(5, "scene %d", pScene ? *(int*)pScene : -1);
    mr_SyncScreen(false);
    return pScene && g.bContext && g.bIn3D;
}

void mr_ForgetBindings(void)
{
}
