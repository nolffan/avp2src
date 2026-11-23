// mr_models.cpp -- models (OT_MODEL), as d3d.ren draws them (drawmodel.cpp,
// drawmodel_env.cpp, d3d_modelvb.cpp).
//
// The engine animates the model and gives the renderer one matrix per node
// in the model's own space (ModelInstance vtable +0x48). The renderer skins
// the vertices of the chosen LOD on the CPU, blended towards the next LOD
// near its distance, and lights each vertex in model space from the bind
// pose: the light grid's ambient, the sun, and up to MaxModelLights point
// and spot lights. The game's model hook can change the light add, the
// object colour and the flags first (the vision modes). Then the passes:
// the environment map, then the skin (with the specular or the detail
// texture on a second unit), then the shadow.
//
// Not done: the model instance hook's clip plane (MIH_CLIPPLANE; no AvP2
// code sets it), ModelBoxes, ModelWarble and the vertex cache (the same
// picture).

#include <math.h>

#include "mrobjects.h"

template <typename T> static inline T rd(const void* p, uint32_t off) { T v; memcpy(&v, (const uint8_t*)p + off, sizeof(T)); return v; }
template <typename T> static inline void wr(void* p, uint32_t off, const T& v) { memcpy((uint8_t*)p + off, &v, sizeof(T)); }

static inline int mr_Round(float f) { return (int)lrintf(f); }

// ---- the engine's model data ------------------------------------------------------

// The ModelInstance tail moves 0x14 bytes in Primal Hunt (drawmodel.cpp).
#ifdef PRIMAL_HUNT
#define MI_ENVMAP     0x1d0             // SharedTexture*: its own environment map
#define MI_TRACKER    0x1d4
#define MI_HIDDEN     0x260
#define MI_SUNPOS     0x2c4
#define MI_SUNVIS     0x2d0
#define MI_COLORSUM   0x2d4
#else
#define MI_TRACKER    0x1d0
#define MI_HIDDEN     0x24c
#define MI_SUNPOS     0x2b0
#define MI_SUNVIS     0x2bc
#define MI_COLORSUM   0x2c0
#endif
#define MI_SKINS      0x1c0             // SharedTexture*[4]
#define MI_MODEL      (MI_TRACKER + 0x28)

struct NewVertexWeight { float m_Vec[4]; uint32_t m_iNode; };     // position x weight, weight
struct ModelVert {                      // 0x20
    NewVertexWeight* m_Weights;
    uint16_t m_nWeights;
    uint16_t m_iReplacement;            // in the next LOD
    LTVector m_Vec;                     // bind pose, model space
    LTVector m_vNormal;
};
struct ModelTri { uint16_t m_Indices[3]; uint16_t m_pad; float m_UV[3][2]; };   // 0x20
struct ModelVertSet {                   // 0x24: a piece's LOD
    uint32_t   m_00;
    ModelVert* m_pVerts;                // 0x04
    uint32_t   m_nVerts;                // 0x08
    uint32_t   m_0c, m_10;
    ModelTri*  m_pTris;                 // 0x14
    uint32_t   m_nTris;                 // 0x18
    uint32_t   m_1c, m_20;
};
static_assert(sizeof(ModelVertSet) == 0x24, "ModelVertSet");
struct ModelPiece : ModelVertSet {
    uint32_t   m_iTexture;              // 0x24
    uint32_t   m_28;
    float      m_fSpecularPower;        // 0x2c
    float      m_fSpecularScale;        // 0x30
    uint32_t   m_34;
    ModelVertSet* m_LODs;               // 0x38 LODs 1..
    uint32_t   m_nLODs;                 // 0x3c
};

static ModelVertSet* mr_PieceLOD(ModelPiece* p, uint32_t i)
{
    if (i == 0)
        return p;
    return i - 1 < p->m_nLODs ? &p->m_LODs[i - 1] : NULL;
}

// The engine's Model, read at its offsets.
struct ModelView {
    const uint8_t* p;
    uint32_t    nPieces() const    { return rd<uint32_t>(p, 0x3c); }
    ModelPiece* Piece(uint32_t i) const { return rd<ModelPiece**>(p, 0x38)[i]; }
    uint32_t    nVerts() const     { return rd<uint32_t>(p, 0x64); }
    uint32_t    nNodes() const     { return rd<uint32_t>(p, 0x78); }
    uint32_t    nLODs() const      { return rd<uint32_t>(p, 0x94); }
    float LODDist(uint32_t i) const
    {
        if (i == 0)
            return rd<float>(p, 0xa0);
        return i - 1 < nLODs() ? rd<float*>(p, 0x90)[i - 1] : 1.0e30f;
    }
    float Radius() const           { return rd<float>(p, 0xa4); }
    float VisRadius() const        { return rd<float>(p, 0xa8); }
    float SpriteNear() const       { return rd<float>(p, 0xb0); }
    float SpriteNearSqr() const    { return rd<float>(p, 0xb4); }
    float SpriteFar() const        { return rd<float>(p, 0xb8); }
    float SpriteFarSqr() const     { return rd<float>(p, 0xbc); }
    SharedTexture* SpriteTex() const { return rd<SharedTexture*>(p, 0xc0); }
    float SpriteW() const          { return rd<float>(p, 0xc4); }
    float SpriteH() const          { return rd<float>(p, 0xc8); }
    uint32_t SocketNode() const    { return rd<uint32_t>(p, 0xf4); }
    LTMatrix SocketMat() const     { return rd<LTMatrix>(p, 0xfc); }
    bool  bSocketLit() const       { return rd<uint32_t>(p, 0x13c) != 0; }
    bool  bFovScale() const        { return rd<uint32_t>(p, 0x140) != 0; }
    float FovX() const             { return rd<float>(p, 0x144); }
    float FovY() const             { return rd<float>(p, 0x148); }
    bool  bSpecular() const        { return rd<uint32_t>(p, 0x14c) != 0; }
    uint32_t nAnims() const        { return rd<uint32_t>(p, 0x16c); }
};

static ModelView mr_ModelOf(LTObject* pInst)
{
    ModelView m = { rd<const uint8_t*>(pInst, MI_MODEL) };
    return m;
}

// ---- small matrix helpers -----------------------------------------------------------

static void mat_Mul(LTMatrix* pOut, const LTMatrix& a, const LTMatrix& b)
{
    LTMatrix r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
    *pOut = r;
}

// The inverse of an affine matrix (a 3x3 with scale, and a translation).
static void mat_InvAffine(LTMatrix* pOut, const LTMatrix& a)
{
    const float (*m)[4] = a.m;
    float c00 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    float c01 = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    float c02 = m[1][0] * m[2][1] - m[1][1] * m[2][0];
    float det = m[0][0] * c00 + m[0][1] * c01 + m[0][2] * c02;
    float id = det != 0.0f ? 1.0f / det : 0.0f;
    LTMatrix r;
    r.m[0][0] = c00 * id;
    r.m[1][0] = c01 * id;
    r.m[2][0] = c02 * id;
    r.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * id;
    r.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * id;
    r.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * id;
    r.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * id;
    r.m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * id;
    r.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * id;
    for (int i = 0; i < 3; i++)
        r.m[i][3] = -(r.m[i][0] * m[0][3] + r.m[i][1] * m[1][3] + r.m[i][2] * m[2][3]);
    r.m[3][0] = r.m[3][1] = r.m[3][2] = 0.0f;
    r.m[3][3] = 1.0f;
    *pOut = r;
}

static LTVector mat_Apply(const LTMatrix& M, const LTVector& v)
{
    LTVector r = { M.m[0][0] * v.x + M.m[0][1] * v.y + M.m[0][2] * v.z + M.m[0][3],
                   M.m[1][0] * v.x + M.m[1][1] * v.y + M.m[1][2] * v.z + M.m[1][3],
                   M.m[2][0] * v.x + M.m[2][1] * v.y + M.m[2][2] * v.z + M.m[2][3] };
    return r;
}

static LTVector mat_Apply3x3(const LTMatrix& M, const LTVector& v)
{
    LTVector r = { M.m[0][0] * v.x + M.m[0][1] * v.y + M.m[0][2] * v.z,
                   M.m[1][0] * v.x + M.m[1][1] * v.y + M.m[1][2] * v.z,
                   M.m[2][0] * v.x + M.m[2][1] * v.y + M.m[2][2] * v.z };
    return r;
}

// The camera's own matrix (camera space -> world).
static void mr_CameraToWorld(LTMatrix* pM)
{
    const MRCamera& cam = g_Frame.m_Cam;
    for (int r = 0; r < 3; r++)
    {
        pM->m[r][0] = cam.m_R[r][0];
        pM->m[r][1] = cam.m_R[r][1];
        pM->m[r][2] = cam.m_R[r][2];
    }
    pM->m[0][3] = cam.m_Pos.x;
    pM->m[1][3] = cam.m_Pos.y;
    pM->m[2][3] = cam.m_Pos.z;
    pM->m[3][0] = pM->m[3][1] = pM->m[3][2] = 0.0f;
    pM->m[3][3] = 1.0f;
}

// ---- lighting (CD3DModelDraw::SetupLighting, AddLight, LIGHT_VERT) ---------------------

#define MR_MAXMODELLIGHTS 16

struct ModelLight {
    LTVector m_Pos;                     // light space
    float    m_fRadiusSqr;
    LTVector m_Dir;                     // the light's direction from the origin, over r^2
    LTVector m_Color;
};

struct ModelLighting {
    LTVector m_vObjectColor;            // 0..1 (x ModelSaturation under Saturate)
    LTVector m_vLightAdd;               // 0..255
    LTVector m_vAmbient;                // grid x world colour
    LTVector m_vSunColor;
    LTVector m_vSunDir;                 // light space, towards the sun
    float    m_fSunVis;
    ModelLight m_Lights[MR_MAXMODELLIGHTS];
    uint32_t m_nLights, m_nMaxLights;
    LTVector m_vPos;                    // the model's place in the world
    float    m_fVisRadius;
    uint32_t m_HookFlags;               // MHF_*
    uint32_t m_ObjectFlags;             // the object's flags as the hook left them
    uint8_t  m_nAlpha;
};

static void mr_AddModelLight(ModelLighting* L, const LTMatrix& toLight, const LTVector& pos, float fRadius,
                             LTVector inner, const LTVector& outer, const LTVector& dir, float fCos)
{
    if (L->m_nLights >= L->m_nMaxLights)
        return;
    LTVector d = { L->m_vPos.x - pos.x, L->m_vPos.y - pos.y, L->m_vPos.z - pos.z };
    float fDist = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
    if (fDist - L->m_fVisRadius >= fRadius)
        return;
    if (fCos > -1.0f && fCos < 1.0f && fDist != 0.0f)
    {
        float fDot = (d.x * dir.x + d.y * dir.y + d.z * dir.z) / fDist;
        if (fDot < fCos)
            return;
        float t = (1.0f - (fDot + 1.0f) * 0.5f) / (1.0f - (fCos + 1.0f) * 0.5f);
        inner.x = (1.0f - t) * inner.x + outer.x * t;
        inner.y = (1.0f - t) * inner.y + outer.y * t;
        inner.z = (1.0f - t) * inner.z + outer.z * t;
    }
    const LTVector& wc = g_Frame.m_pScene->m_WorldColor;
    ModelLight& M = L->m_Lights[L->m_nLights++];
    M.m_Color.x = inner.x * wc.x * L->m_vObjectColor.x;
    M.m_Color.y = inner.y * wc.y * L->m_vObjectColor.y;
    M.m_Color.z = inner.z * wc.z * L->m_vObjectColor.z;
    M.m_Pos = mat_Apply(toLight, pos);
    M.m_fRadiusSqr = fRadius * fRadius;
    float l = sqrtf(M.m_Pos.x * M.m_Pos.x + M.m_Pos.y * M.m_Pos.y + M.m_Pos.z * M.m_Pos.z);
    float s = (l > 0.0f ? 1.0f / l : 0.0f) / M.m_fRadiusSqr;
    M.m_Dir.x = M.m_Pos.x * s;
    M.m_Dir.y = M.m_Pos.y * s;
    M.m_Dir.z = M.m_Pos.z * s;
}

struct MRStaticLightCB { ModelLighting* m_pL; const LTMatrix* m_pToLight; };

static void mr_StaticLightCB(void* pObj, void* pUser)
{
    const StaticLight* S = (const StaticLight*)pObj;
    MRStaticLightCB* p = (MRStaticLightCB*)pUser;
    mr_AddModelLight(p->m_pL, *p->m_pToLight, S->m_Pos, S->m_fRadius, S->m_Color, S->m_OuterColor, S->m_Dir,
                     S->m_fCone);
}

// CalcSunVis: 1 when a ray from the centre to the sun ends on the sky; with
// ModelSunVariance, rays from either side too, and a search for the
// shadow's edge between them.
static float mr_CalcSunVis(const LTVector& vPos, const LTMatrix& world, float fRadius)
{
    const LTVector& s = g.pStruct->m_GlobalLightDir;
    float k = mr_WorldSize() * -2.0f;
    LTVector vRay = { s.x * k, s.y * k, s.z * k };
    float fVar = cvf(CV_ModelSunVariance);
    if (fVar == 0.0f)
        return mr_SeesSky(vPos, vRay) ? 1.0f : 0.0f;
    LTVector r = { world.m[0][0], world.m[1][0], world.m[2][0] };
    LTVector perp = { s.y * r.z - s.z * r.y, s.z * r.x - s.x * r.z, s.x * r.y - s.y * r.x };
    perp.x *= fRadius; perp.y *= fRadius; perp.z *= fRadius;
    LTVector p = { vPos.x + perp.x, vPos.y + perp.y, vPos.z + perp.z };
    bool bHi = mr_SeesSky(p, vRay);
    p.x = vPos.x - perp.x; p.y = vPos.y - perp.y; p.z = vPos.z - perp.z;
    bool bLo = mr_SeesSky(p, vRay);
    if (bHi == bLo)
        return bHi ? 1.0f : 0.0f;
    float fLo = -1.0f, fHi = 1.0f, fStep = 1.0f, fVis = 1.0f;
    while (fStep > fVar && bLo != bHi)
    {
        float fMid = (fLo + fHi) * 0.5f;
        p.x = vPos.x + perp.x * fMid; p.y = vPos.y + perp.y * fMid; p.z = vPos.z + perp.z * fMid;
        bool bHit = mr_SeesSky(p, vRay);
        fStep *= 0.5f;
        if (!bHit)
            fVis -= fStep;
        if (bHit == bHi)
            fHi = fMid;
        else
        {
            fLo = fMid;
            bLo = bHit;
        }
    }
    return fVis;
}

// toLight: world -> the space the vertices are lit in (the model's, or,
// for a socket-lit model, its socket's).
static void mr_SetupModelLight(LTObject* pInst, const LTMatrix& world, const LTMatrix& toLight, ModelLighting* L)
{
    L->m_nLights = 0;
    int nMax = cvi(CV_MaxModelLights);
    L->m_nMaxLights = nMax < 0 ? 0 : nMax > MR_MAXMODELLIGHTS ? MR_MAXMODELLIGHTS : (uint32_t)nMax;
    L->m_nAlpha = pInst->m_ColorA;

#ifdef PRIMAL_HUNT
    // The cloaked (FLAG2_CHAMELEON): full colour; the red is the alpha of
    // the environment map pass.
    if (pInst->m_Flags2 & FLAG2_CHAMELEON)
        L->m_vObjectColor.x = L->m_vObjectColor.y = L->m_vObjectColor.z = 255.0f;
    else
#endif
    {
        L->m_vObjectColor.x = pInst->m_ColorR;
        L->m_vObjectColor.y = pInst->m_ColorG;
        L->m_vObjectColor.z = pInst->m_ColorB;
    }
    L->m_vLightAdd = g_Frame.m_pScene->m_ModelAdd;

    // The game's model hook (the vision modes): it can change the colour,
    // the add and the flags.
    ModelHookData hook;
    hook.m_hObject = pInst;
    hook.m_Flags = MHF_USETEXTURE;
    hook.m_ObjectFlags = pInst->m_Flags;
    hook.m_LightAdd = &L->m_vLightAdd;
    hook.m_ObjectColor = &L->m_vObjectColor;
    if (g_Frame.m_pScene->m_ModelHookFn)
        g_Frame.m_pScene->m_ModelHookFn(&hook, g_Frame.m_pScene->m_ModelHookUser);
    L->m_HookFlags = hook.m_Flags;
    L->m_ObjectFlags = hook.m_ObjectFlags;

    float fSat = cvi(CV_Saturate) ? cvf(CV_ModelSaturation) : 1.0f;
    L->m_vObjectColor.x *= fSat / 255.0f;
    L->m_vObjectColor.y *= fSat / 255.0f;
    L->m_vObjectColor.z *= fSat / 255.0f;

    const LTVector& wc = g_Frame.m_pScene->m_WorldColor;
    if (pInst->m_Flags & FLAG_NOLIGHT)
    {
        L->m_vSunDir.x = L->m_vSunDir.y = L->m_vSunDir.z = 0.0f;
        L->m_fSunVis = 0.0f;
        L->m_vAmbient.x = L->m_vAmbient.y = L->m_vAmbient.z = 255.0f;
        L->m_vSunColor = L->m_vAmbient;
        return;
    }

    const LTVector& sc = g.pStruct->m_GlobalLightColor;
    MainWorld* pWorld = g_Frame.m_pContext ? g_Frame.m_pContext->m_pWorld : NULL;
    if (pWorld && cvi(CV_ModelApplySun) && (wc.x != 0.0f || wc.y != 0.0f || wc.z != 0.0f) &&
        (sc.x != 0.0f || sc.y != 0.0f || sc.z != 0.0f))
    {
        // The last test is kept on the instance; a model with
        // FLAG2_DYNAMICDIRLIGHT tests again when it moves.
        float fLast = rd<float>(pInst, MI_SUNVIS);
        LTVector vLast = rd<LTVector>(pInst, MI_SUNPOS);
        float e = cvf(CV_ModelSunVariance) + 0.001f;
        if (fLast >= 0.0f &&
            (!(pInst->m_Flags2 & FLAG2_DYNAMICDIRLIGHT) ||
             (fabsf(L->m_vPos.x - vLast.x) <= e && fabsf(L->m_vPos.y - vLast.y) <= e &&
              fabsf(L->m_vPos.z - vLast.z) <= e)))
            L->m_fSunVis = fLast;
        else
        {
            typedef float (__thiscall* GetRadiusFn)(LTObject*);
            float fRadius = (*(GetRadiusFn**)pInst)[0x44 / 4](pInst);
            L->m_fSunVis = mr_CalcSunVis(L->m_vPos, world, fRadius);
            wr(pInst, MI_SUNPOS, L->m_vPos);
            wr(pInst, MI_SUNVIS, L->m_fSunVis);
        }
        LTVector d = mat_Apply3x3(toLight, g.pStruct->m_GlobalLightDir);
        L->m_vSunDir.x = -d.x;
        L->m_vSunDir.y = -d.y;
        L->m_vSunDir.z = -d.z;
    }
    else
    {
        L->m_vSunDir.x = L->m_vSunDir.y = L->m_vSunDir.z = 0.0f;
        L->m_fSunVis = 0.0f;
    }
    L->m_vSunColor.x = sc.x * wc.x;
    L->m_vSunColor.y = sc.y * wc.y;
    L->m_vSunColor.z = sc.z * wc.z;

    float grid[3];
    mr_LightGridSample(L->m_vPos, grid);
    L->m_vAmbient.x = grid[0] * wc.x;
    L->m_vAmbient.y = grid[1] * wc.y;
    L->m_vAmbient.z = grid[2] * wc.z;

    // The visible dynamic lights that light objects, then the world's
    // static lights near the model.
    LTVector vZero = { 0, 0, 0 };
    MRObjList* pLights = mr_ObjList(VS_LIGHTS);
    for (uint32_t i = 0, n = 0; i < pLights->m_nObjects && n < MR_MAXWORLDLIGHTS; i++)
    {
        LTObject* pL = pLights->m_pObjects[i];
        if (pL->m_Flags & FLAG_ONLYLIGHTWORLD)
            continue;
        n++;
        LTVector c = { (float)pL->m_ColorR, (float)pL->m_ColorG, (float)pL->m_ColorB };
        mr_AddModelLight(L, toLight, pL->m_Pos, obj_LightRadius(pL), c, vZero, vZero, -1.0f);
        if (L->m_nLights >= L->m_nMaxLights)
            break;
    }
    if (L->m_nLights < L->m_nMaxLights && pWorld)
    {
        MRStaticLightCB cb = { L, &toLight };
        FindObjInfo q;
        memset(&q, 0, sizeof(q));
        q.m_iObjArray = 1;
        float r = L->m_fVisRadius;
        q.m_Min.x = L->m_vPos.x - r; q.m_Min.y = L->m_vPos.y - r; q.m_Min.z = L->m_vPos.z - r;
        q.m_Max.x = L->m_vPos.x + r; q.m_Max.y = L->m_vPos.y + r; q.m_Max.z = L->m_vPos.z + r;
        q.m_CB = mr_StaticLightCB;
        q.m_pUser = &cb;
        mw_FindObjectsInBox(pWorld, &q);
    }
}

// LIGHT_VERT: from the ambient colour towards the sunlit one by how much
// the normal faces the sun, plus each light the normal faces, by distance;
// clamped at 255.
struct MRVertLight {
    LTVector vSunDir, vAmbient, vSun;
};

static void mr_PrepareVertLight(const ModelLighting& L, MRVertLight* p)
{
    p->vSunDir.x = L.m_vSunDir.x * L.m_fSunVis;
    p->vSunDir.y = L.m_vSunDir.y * L.m_fSunVis;
    p->vSunDir.z = L.m_vSunDir.z * L.m_fSunVis;
    p->vAmbient.x = L.m_vAmbient.x * L.m_vObjectColor.x + L.m_vLightAdd.x;
    p->vAmbient.y = L.m_vAmbient.y * L.m_vObjectColor.y + L.m_vLightAdd.y;
    p->vAmbient.z = L.m_vAmbient.z * L.m_vObjectColor.z + L.m_vLightAdd.z;
    p->vSun.x = L.m_vSunColor.x * L.m_vObjectColor.x + L.m_vLightAdd.x;
    p->vSun.y = L.m_vSunColor.y * L.m_vObjectColor.y + L.m_vLightAdd.y;
    p->vSun.z = L.m_vSunColor.z * L.m_vObjectColor.z + L.m_vLightAdd.z;
}

static void mr_LightVert(const ModelLighting& L, const MRVertLight& P, const ModelVert& v, uint8_t nAlpha,
                         uint8_t* pOut, LTVector* pSum)
{
    const LTVector& N = v.m_vNormal;
    float fDot = P.vSunDir.x * N.x + P.vSunDir.y * N.y + P.vSunDir.z * N.z;
    LTVector c = P.vAmbient;
    if (fDot > 0.0f)
    {
        c.x += (P.vSun.x - P.vAmbient.x) * fDot;
        c.y += (P.vSun.y - P.vAmbient.y) * fDot;
        c.z += (P.vSun.z - P.vAmbient.z) * fDot;
    }
    for (uint32_t i = 0; i < L.m_nLights; i++)
    {
        const ModelLight& M = L.m_Lights[i];
        float fLD = N.x * M.m_Dir.x + N.y * M.m_Dir.y + N.z * M.m_Dir.z;
        if (fLD <= 0.0f)
            continue;
        float dx = v.m_Vec.x - M.m_Pos.x, dy = v.m_Vec.y - M.m_Pos.y, dz = v.m_Vec.z - M.m_Pos.z;
        float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < M.m_fRadiusSqr)
        {
            float k = (M.m_fRadiusSqr - d2) * fLD;
            c.x += M.m_Color.x * k;
            c.y += M.m_Color.y * k;
            c.z += M.m_Color.z * k;
        }
    }
    if (c.x > 255.0f) c.x = 255.0f;
    if (c.y > 255.0f) c.y = 255.0f;
    if (c.z > 255.0f) c.z = 255.0f;
    pSum->x += c.x;
    pSum->y += c.y;
    pSum->z += c.z;
    pOut[0] = (uint8_t)mr_Round(c.x);
    pOut[1] = (uint8_t)mr_Round(c.y);
    pOut[2] = (uint8_t)mr_Round(c.z);
    pOut[3] = nAlpha;
}

// ---- the specular texture (d3d_SetGammaTexture) ------------------------------------------
//
// 64x64, grey pow(i / 64, power) on the diagonal and next to it: the
// specular pass looks it up at (N.L x scale, N.L x scale).

static uint32_t g_GammaTex, g_GammaGen;
static float g_fGammaPower = -1.0f;

static uint32_t mr_GammaTexture(float fPower)
{
    if (g_GammaTex && g_GammaGen == g.nContextGen && g_fGammaPower == fPower)
        return g_GammaTex;
    if (!g_GammaTex || g_GammaGen != g.nContextGen)
        g_GammaTex = rd_GenTexture();
    g_GammaGen = g.nContextGen;
    g_fGammaPower = fPower;
    static uint32_t data[64][64];
    memset(data, 0, sizeof(data));
    for (int i = 0; i < 64; i++)
    {
        uint32_t v = (uint32_t)(powf(i * (1.0f / 64.0f), fPower) * 255.0f);
        uint32_t c = 0xff000000u | (v << 16) | (v << 8) | v;
        data[i][i] = c;
        if (i > 0)
        {
            data[i - 1][i] = c;
            data[i][i - 1] = c;
        }
    }
    rd_BindTexture(g_GammaTex);
    rd_TexImage(0, RD_FMT_RGBA8, 64, 64, RD_PIX_BGRA, data);
    rd_TexFilter(RD_LINEAR, RD_LINEAR);
    rd_TexWrap(false, false);
    rd_TexMaxLevel(0);
    return g_GammaTex;
}

// ---- the model's sprite (CD3DModelDraw::DrawSprite) -------------------------------------

static void mr_DrawModelSprite(LTObject* pInst, const ModelView& model, const LTVector& vPos, uint8_t nAlpha,
                               float fFog)
{
    SharedTexture* pShared = model.SpriteTex();
    if (!pShared)
        return;
    mr_TextureId(pShared);
    MRTexture* pTex = (MRTexture*)pShared->m_pRenderData;
    const LTVector& cp = g_Frame.m_Cam.m_Pos;
    LTVector d = { vPos.x - cp.x, vPos.y - cp.y, vPos.z - cp.z };
    LTVector r = { -d.z, 0.0f, d.x };       // d x up, flattened
    float l = sqrtf(r.x * r.x + r.z * r.z);
    if (l > 0.0f)
    {
        r.x /= l;
        r.z /= l;
    }
    float w = model.SpriteW() * pInst->m_Scale.x, h = model.SpriteH() * pInst->m_Scale.y;
    float tu = pTex && pTex->m_nBaseWidth ? 1.0f / pTex->m_nBaseWidth : 0.0f;
    float tv = pTex && pTex->m_nBaseHeight ? 1.0f / pTex->m_nBaseHeight : 0.0f;
    float grid[3] = { 255.0f, 255.0f, 255.0f };
    if (cvi(CV_LightModelSprites))
        mr_LightGridSample(vPos, grid);
    const uint8_t (*Lr)[256] = g_Frame.m_LightRamp;
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_TEXTURE | RD_ATTR_ENABLE);
    if (nAlpha < 255)
    {
        rd_Enable(RD_BLEND);
        rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);
    }
    mr_ForgetBindings();
    mr_SetBaseTexture(pTex);
    rd_TexEnv(RD_ENV_MODULATE);
    rd_Color4ub(Lr[0][(uint8_t)grid[0]], Lr[1][(uint8_t)grid[1]], Lr[2][(uint8_t)grid[2]], nAlpha);
    rd_FogFactor(fFog);
    rd_Begin(RD_QUADS);
    rd_TexCoord2f(tu, tv);               rd_Vertex3f(vPos.x - r.x * w, vPos.y + h, vPos.z - r.z * w);
    rd_TexCoord2f(1.0f - tu, tv);        rd_Vertex3f(vPos.x + r.x * w, vPos.y + h, vPos.z + r.z * w);
    rd_TexCoord2f(1.0f - tu, 1.0f - tv); rd_Vertex3f(vPos.x + r.x * w, vPos.y - h, vPos.z + r.z * w);
    rd_TexCoord2f(tu, 1.0f - tv);        rd_Vertex3f(vPos.x - r.x * w, vPos.y - h, vPos.z - r.z * w);
    rd_End();
    mr_SetBaseTexture(NULL);
    mr_ForgetBindings();
    rd_PopAttrib();
}

// ---- drawing ----------------------------------------------------------------------------

struct MRSkinVert { float p[3]; uint8_t c[4]; float su, sv; };
static MRSkinVert* g_pSkin;
static uint32_t g_nSkinAlloc;
static LTMatrix* g_pNodes;
static uint32_t g_nNodeAlloc;

static bool mr_Reserve(uint32_t nVerts, uint32_t nNodes)
{
    if (nVerts > g_nSkinAlloc)
    {
        MRSkinVert* p = (MRSkinVert*)realloc(g_pSkin, nVerts * sizeof(MRSkinVert));
        if (!p)
            return false;
        g_pSkin = p;
        g_nSkinAlloc = nVerts;
    }
    if (nNodes > g_nNodeAlloc)
    {
        LTMatrix* p = (LTMatrix*)realloc(g_pNodes, nNodes * sizeof(LTMatrix));
        if (!p)
            return false;
        g_pNodes = p;
        g_nNodeAlloc = nNodes;
    }
    return true;
}

static void mr_Skin(const ModelVert& v, uint32_t nNodes, float* pOut)
{
    pOut[0] = pOut[1] = pOut[2] = pOut[3] = 0.0f;
    for (uint32_t w = 0; w < v.m_nWeights; w++)
    {
        const NewVertexWeight& nw = v.m_Weights[w];
        if (nw.m_iNode >= nNodes)
            continue;
        const LTMatrix& M = g_pNodes[nw.m_iNode];
        for (int r = 0; r < 4; r++)
            pOut[r] += M.m[r][0] * nw.m_Vec[0] + M.m[r][1] * nw.m_Vec[1] + M.m[r][2] * nw.m_Vec[2] + M.m[r][3] * nw.m_Vec[3];
    }
}

// The pieces' vertices, skinned (LOD-blended) and lit (TransformVerts,
// DrawLOD). The instance keeps the mean of the lit colours.
struct MRModelDraw {
    LTObject*  m_pInst;
    ModelView  m_Model;
    uint32_t   m_iLOD;
    bool       m_bBlendLOD;
    float      m_fLODBlend;
    uint32_t   m_nNodes;
    ModelLighting L;
    // The environment map's coordinates: the bind-pose normal through the
    // view's right and up rows, scaled and panned with the camera.
    float      m_EnvRow[2][3];
    float      m_fEnvScale, m_fEnvU, m_fEnvV;
    float      m_fFog;
};

enum MRUVMode { UV_SKIN, UV_ENV, UV_QUARTER, UV_SKIN_DETAIL, UV_SKIN_SPECULAR };

static void mr_DrawPieces(MRModelDraw& D, MRUVMode eUV, uint8_t nAlpha, bool bSkins, MRTexture* pFixed)
{
    LTObject* pInst = D.m_pInst;
    uint32_t nHidden = rd<uint32_t>(pInst, MI_HIDDEN);
    MRVertLight P;
    mr_PrepareVertLight(D.L, &P);
    LTVector* pSum = (LTVector*)((uint8_t*)pInst + MI_COLORSUM);
    LTVector vSum = rd<LTVector>(pInst, MI_COLORSUM);
    uint32_t nTotal = 0;
    bool bTextured = (D.L.m_HookFlags & MHF_USETEXTURE) && cvi(CV_TextureModels);
    int iCurSkin = -1;
    mr_ForgetBindings();
    if (!bSkins)
        mr_SetBaseTexture(pFixed);
    for (uint32_t i = 0; i < D.m_Model.nPieces(); i++)
    {
        ModelPiece* pPiece = D.m_Model.Piece(i);
        ModelVertSet* pLOD = pPiece ? mr_PieceLOD(pPiece, D.m_iLOD) : NULL;
        if (!pLOD || (i < 32 && (nHidden & (1u << i))) || !pLOD->m_nVerts)
            continue;
        ModelVertSet* pNext = D.m_bBlendLOD ? mr_PieceLOD(pPiece, D.m_iLOD + 1) : NULL;
        if (!mr_Reserve(pLOD->m_nVerts, 0))
            return;

        // The specular texture's coordinates: N.L times the piece's scale.
        float fSpecScale = pPiece->m_fSpecularScale;
        for (uint32_t v = 0; v < pLOD->m_nVerts; v++)
        {
            const ModelVert& mv = pLOD->m_pVerts[v];
            MRSkinVert& o = g_pSkin[v];
            float h[4];
            mr_Skin(mv, D.m_nNodes, h);
            if (pNext && mv.m_iReplacement < pNext->m_nVerts)
            {
                float n[4];
                mr_Skin(pNext->m_pVerts[mv.m_iReplacement], D.m_nNodes, n);
                for (int k = 0; k < 4; k++)
                    h[k] += (n[k] - h[k]) * D.m_fLODBlend;
            }
            float inv = h[3] != 0.0f ? 1.0f / h[3] : 1.0f;
            o.p[0] = h[0] * inv;
            o.p[1] = h[1] * inv;
            o.p[2] = h[2] * inv;
            mr_LightVert(D.L, P, mv, nAlpha, o.c, &vSum);
            const LTVector& N = mv.m_vNormal;
            if (eUV == UV_ENV)
            {
                o.su = (D.m_EnvRow[0][0] * N.x + D.m_EnvRow[0][1] * N.y + D.m_EnvRow[0][2] * N.z) * D.m_fEnvScale + D.m_fEnvU;
                o.sv = (D.m_EnvRow[1][0] * N.x + D.m_EnvRow[1][1] * N.y + D.m_EnvRow[1][2] * N.z) * D.m_fEnvScale + D.m_fEnvV;
            }
            else if (eUV == UV_SKIN_SPECULAR)
            {
                float d = N.x * D.L.m_vSunDir.x + N.y * D.L.m_vSunDir.y + N.z * D.L.m_vSunDir.z;
                o.su = o.sv = d > 0.0f ? d * fSpecScale : 0.0f;
            }
        }
        nTotal += pLOD->m_nVerts;

        // The skin, and the second unit's texture for this piece.
        if (bSkins && (int)pPiece->m_iTexture != iCurSkin)
        {
            iCurSkin = (int)pPiece->m_iTexture;
            SharedTexture* pSkin = pPiece->m_iTexture < 4 ? rd<SharedTexture*>(pInst, MI_SKINS + 4 * pPiece->m_iTexture) : NULL;
            MRTexture* pTex = NULL;
            if (bTextured && pSkin)
            {
                mr_TextureId(pSkin);
                pTex = (MRTexture*)pSkin->m_pRenderData;
            }
            mr_SetBaseTexture(pTex);
            if (eUV == UV_SKIN_DETAIL)
            {
                // The skin's linked texture on unit 1, added signed; its
                // alpha replaces the skin's.
                MRTexture* pDetail = NULL;
                if (pSkin && pSkin->m_pLinkedTexture)
                {
                    mr_TextureId(pSkin->m_pLinkedTexture);
                    pDetail = (MRTexture*)pSkin->m_pLinkedTexture->m_pRenderData;
                }
                rd_ActiveTexture(1);
                if (pDetail)
                {
                    RDCombine c = { RD_OP_ADD_SIGNED, RD_ARG_TEXTURE, RD_ARG_PREVIOUS, 1.0f,
                                    RD_OP_REPLACE, RD_ARG_TEXTURE, RD_ARG_PREVIOUS };
                    rd_TexCombine(c);
                    rd_Enable(RD_TEXTURE_2D);
                    rd_BindTexture(pDetail->m_Id);
                }
                else
                    rd_Disable(RD_TEXTURE_2D);
                rd_ActiveTexture(0);
            }
        }
        if (eUV == UV_SKIN_SPECULAR)
        {
            rd_ActiveTexture(1);
            if (fSpecScale != 0.0f)
            {
                rd_Enable(RD_TEXTURE_2D);
                mr_GammaTexture(pPiece->m_fSpecularPower);
                RDCombine c = { RD_OP_ADD, RD_ARG_TEXTURE, RD_ARG_PREVIOUS, 1.0f,
                                RD_OP_ARG1, RD_ARG_TEXTURE, RD_ARG_PREVIOUS };
                rd_TexCombine(c);
            }
            else
                rd_Disable(RD_TEXTURE_2D);
            rd_ActiveTexture(0);
        }

        float fDetail = cvf(CV_ModelDetailTextureScale);
        rd_FogFactor(D.m_fFog);
        rd_Begin(RD_TRIANGLES);
        for (uint32_t t = 0; t < pLOD->m_nTris; t++)
        {
            const ModelTri& tri = pLOD->m_pTris[t];
            for (int k = 0; k < 3; k++)
            {
                uint32_t idx = tri.m_Indices[k];
                if (idx >= pLOD->m_nVerts)
                    idx = 0;
                const MRSkinVert& o = g_pSkin[idx];
                rd_Color4ubv(o.c);
                switch (eUV)
                {
                case UV_ENV:
                    rd_TexCoord2f(o.su, o.sv);
                    break;
                case UV_QUARTER:
                    rd_TexCoord2f(tri.m_UV[k][0] * 0.25f, tri.m_UV[k][1] * 0.25f);
                    break;
                case UV_SKIN_DETAIL:
                    rd_TexCoord2f(tri.m_UV[k][0], tri.m_UV[k][1]);
                    rd_MultiTexCoord2f(1, tri.m_UV[k][0] * fDetail, tri.m_UV[k][1] * fDetail);
                    break;
                case UV_SKIN_SPECULAR:
                    rd_TexCoord2f(tri.m_UV[k][0], tri.m_UV[k][1]);
                    rd_MultiTexCoord2f(1, o.su, o.sv);
                    break;
                default:
                    rd_TexCoord2f(tri.m_UV[k][0], tri.m_UV[k][1]);
                    break;
                }
                rd_Vertex3fv(o.p);
            }
        }
        rd_End();
    }
    if (eUV == UV_SKIN_DETAIL || eUV == UV_SKIN_SPECULAR)
    {
        rd_ActiveTexture(1);
        rd_Disable(RD_TEXTURE_2D);
        rd_TexEnv(RD_ENV_MODULATE);
        rd_ActiveTexture(0);
    }
    if (nTotal)
    {
        vSum.x /= (float)nTotal;
        vSum.y /= (float)nTotal;
        vSum.z /= (float)nTotal;
    }
    memcpy(pSum, &vSum, sizeof(vSum));
}

// SetupLOD: the last LOD whose distance the model is beyond; blended
// towards the next within ModelLODBlendDist (at most half the gap) of it.
static void mr_SetupLOD(MRModelDraw& D, float fDist)
{
    const ModelView& m = D.m_Model;
    D.m_iLOD = 0;
    for (uint32_t i = 0; i < m.nLODs() + 1; i++)
        if (fDist > m.LODDist(i))
            D.m_iLOD = i;
    D.m_bBlendLOD = false;
    D.m_fLODBlend = 0.0f;
    if (cvi(CV_ModelLODBlendEnable) && D.m_iLOD + 1 < m.nLODs() + 1)
    {
        float fNext = m.LODDist(D.m_iLOD + 1);
        float fBlend = cvf(CV_ModelLODBlendDist);
        float fHalf = (fNext - m.LODDist(D.m_iLOD)) * 0.5f;
        fBlend = fBlend < fHalf ? fBlend : fHalf;
        float fTo = fNext - fDist;
        if (fTo < fBlend && fBlend > 0.0f)
        {
            D.m_bBlendLOD = true;
            D.m_fLODBlend = 1.0f - fTo / fBlend;
        }
    }
}

#ifdef PRIMAL_HUNT
static float g_fEnvMapAngle;
#endif

static void mr_DrawModelWith(LTObject* pInst, bool bClose)
{
    MRModelDraw D;
    D.m_pInst = pInst;
    D.m_Model = mr_ModelOf(pInst);
    const ModelView& model = D.m_Model;
    if (!model.p || !cvi(CV_DrawModels))
        return;
    // AnimTracker::IsValid: both animations in range.
    uint32_t nAnims = model.nAnims();
    if (rd<uint16_t>(pInst, MI_TRACKER + 0x2c) >= nAnims || rd<uint16_t>(pInst, MI_TRACKER + 0x38) >= nAnims)
        return;
    if (bClose && !cvi(CV_DrawGuns))
        return;

    const MRCamera& cam = g_Frame.m_Cam;
    LTMatrix camToWorld;
    mr_CameraToWorld(&camToWorld);

    // The object's matrix (d3d_SetupTransformation); a really-close model's
    // is in camera space.
    LTMatrix world;
    mr_ObjectMatrix(pInst, pInst->m_Pos, pInst->m_Scale, &world);
    LTVector vPos = bClose ? mat_Apply(camToWorld, pInst->m_Pos) : pInst->m_Pos;
    D.L.m_vPos = vPos;
    D.L.m_fVisRadius = model.VisRadius();

    float fScale = pInst->m_Scale.x > pInst->m_Scale.y ? pInst->m_Scale.x : pInst->m_Scale.y;
    fScale = fScale > pInst->m_Scale.z ? fScale : pInst->m_Scale.z;
    if (!bClose && !mr_SphereVisible(vPos, model.Radius() * fScale))
        return;

    // Models are fogged whatever FLAG_FOGDISABLE says (d3d_GetBlendStates).
    D.m_fFog = mr_FogFactor(vPos);

    // The distance sprite: beyond SpriteFar the model is its sprite; between
    // SpriteNear and SpriteFar the two fade into each other.
    float dx = vPos.x - cam.m_Pos.x, dy = vPos.y - cam.m_Pos.y, dz = vPos.z - cam.m_Pos.z;
    float fDistSqr = dx * dx + dy * dy + dz * dz;
    uint8_t nAlpha = pInst->m_ColorA;
    if (model.SpriteFarSqr() >= model.SpriteNearSqr())
    {
        if (fDistSqr > model.SpriteFarSqr())
        {
            mr_DrawModelSprite(pInst, model, vPos, 255, D.m_fFog);
            return;
        }
        if (fDistSqr > model.SpriteNearSqr())
        {
            float t = (sqrtf(fDistSqr) - model.SpriteNear()) / (model.SpriteFar() - model.SpriteNear());
            nAlpha = (uint8_t)mr_Round((1.0f - t) * pInst->m_ColorA);
            mr_DrawModelSprite(pInst, model, vPos, (uint8_t)(255 - nAlpha), D.m_fFog);
        }
    }
    else
    {
        if (fDistSqr < model.SpriteFarSqr())
            return;
        if (fDistSqr < model.SpriteNearSqr())
        {
            float t = (sqrtf(fDistSqr) - model.SpriteFar()) / (model.SpriteNear() - model.SpriteFar());
            nAlpha = (uint8_t)mr_Round(t * pInst->m_ColorA);
        }
    }

    // The environment map.
    SharedTexture* pEnvShared = g.pStruct->m_pEnvMap;
#ifdef PRIMAL_HUNT
    SharedTexture* pInstEnv = rd<SharedTexture*>(pInst, MI_ENVMAP);
    bool bEnvMap = cvi(CV_EnvMapAll) == 1 ||
                   ((pInst->m_Flags & FLAG_ENVIRONMENTMAP) && (pEnvShared || pInstEnv) &&
                    ((pInst->m_Flags2 & FLAG2_CHAMELEON) || cvi(CV_EnvMapEnable) == 1));
    if (pInstEnv)
        pEnvShared = pInstEnv;
#else
    bool bEnvMap = (cvi(CV_EnvMapAll) || (pInst->m_Flags & FLAG_ENVIRONMENTMAP)) && cvi(CV_EnvMapEnable) &&
                   pEnvShared;
#endif

    float fLODDist = sqrtf(fDistSqr);
    float fZoom = cvf(CV_ModelZoomScale);
    mr_SetupLOD(D, fZoom != 0.0f ? fLODDist / fZoom : fLODDist);

    // The nodes: the engine's, in model space, through the object's matrix.
    typedef LTMatrix* (__thiscall* GetNodeTransformsFn)(LTObject*);
    LTMatrix* pTransforms = (*(GetNodeTransformsFn**)pInst)[0x48 / 4](pInst);
    D.m_nNodes = model.nNodes();
    if (!pTransforms || !D.m_nNodes || !mr_Reserve(model.nVerts() + 32, D.m_nNodes))
        return;
    for (uint32_t i = 0; i < D.m_nNodes; i++)
        mat_Mul(&g_pNodes[i], world, pTransforms[i]);

    // The space the vertices are lit in: the model's, or its socket's.
    LTMatrix lightToWorld = world;
    if (model.bSocketLit() && model.SocketNode() < D.m_nNodes)
    {
        LTMatrix socket = model.SocketMat();
        mat_Mul(&lightToWorld, g_pNodes[model.SocketNode()], socket);
    }
    if (bClose)
        mat_Mul(&lightToWorld, camToWorld, lightToWorld);
    LTMatrix toLight;
    mat_InvAffine(&toLight, lightToWorld);
    mr_SetupModelLight(pInst, bClose ? lightToWorld : world, toLight, &D.L);

    if (bEnvMap && pEnvShared)
    {
        for (int r = 0; r < 2; r++)
            for (int c = 0; c < 3; c++)
                D.m_EnvRow[r][c] = cam.m_R[c][r];
        float fEnvScale = cvf(CV_EnvScale);
        D.m_fEnvScale = fEnvScale != 0.0f ? 0.5f / fEnvScale : 0.5f;
        D.m_fEnvU = cam.m_Pos.x * cvf(CV_EnvPanSpeed) + 0.5f;
        D.m_fEnvV = cam.m_Pos.z * cvf(CV_EnvPanSpeed) + 0.5f;
#ifdef PRIMAL_HUNT
        // Flags2 0x1000: the coordinates go round a small circle, once
        // every four seconds of frame time for each model drawn (as
        // retail advances it in DrawModel).
        g_fEnvMapAngle += g_Frame.m_pScene->m_FrameTime * 1.5707963f;
        if (g_fEnvMapAngle > 6.2831853f)
            g_fEnvMapAngle -= 6.2831853f;
        if (pInst->m_Flags2 & FLAG2_ENVOFFSET)
        {
            D.m_fEnvU += sinf(g_fEnvMapAngle) * 0.025f;
            D.m_fEnvV += cosf(g_fEnvMapAngle) * 0.025f;
        }
#endif
    }

    // The passes (DrawPasses).
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_DEPTH | RD_ATTR_TEXTURE | RD_ATTR_ENABLE);
    if (bClose)
    {
        rd_MatrixMode(RD_MODELVIEW);
        rd_PushMatrix();
        rd_LoadIdentity();
    }
    rd_TexEnv(RD_ENV_MODULATE);
    // d3d.ren drops back faces in screen space (TriFacing); a mirror's
    // picture turns them round, so there nothing is dropped.
    rd_CullBack(!cam.m_bMirror);
    bool bEnvAll = cvi(CV_EnvMapAll) != 0;
    if (bEnvMap && pEnvShared)
    {
        mr_TextureId(pEnvShared);
        MRTexture* pEnv = (MRTexture*)pEnvShared->m_pRenderData;
        MRUVMode eUV = UV_ENV;
        uint8_t nEnvAlpha = nAlpha;
#ifdef PRIMAL_HUNT
        if (pInst->m_Flags2 & FLAG2_CHAMELEON)
        {
            eUV = UV_QUARTER;
            nEnvAlpha = pInst->m_ColorR;
            if (nEnvAlpha >= 255)
            {
                rd_Disable(RD_BLEND);
                rd_DepthMask(true);
            }
            else
            {
                rd_Enable(RD_BLEND);
                rd_DepthMask(false);
            }
        }
#endif
        mr_DrawPieces(D, eUV, nEnvAlpha, false, pEnv);
        rd_Enable(RD_BLEND);
#ifdef PRIMAL_HUNT
        if (pInst->m_Flags2 & FLAG2_CHAMELEON)
        {
            if (nAlpha >= 255)
            {
                rd_Disable(RD_BLEND);
                rd_DepthMask(true);
            }
            else
                rd_DepthMask(false);
        }
#endif
    }
    if (nAlpha < 255)
        rd_Enable(RD_BLEND);
    if (!bEnvAll)
    {
        MRUVMode eUV = UV_SKIN;
        if (cvi(CV_ModelSpecular) && model.bSpecular() && g_RDCaps.bCombine && g_RDCaps.nTextureUnits >= 2)
            eUV = UV_SKIN_SPECULAR;
        else if ((pInst->m_Flags & FLAG_DETAILTEXTURE) && g_RDCaps.bCombine && g_RDCaps.nTextureUnits >= 2)
            eUV = UV_SKIN_DETAIL;
        mr_DrawPieces(D, eUV, nAlpha, true, NULL);
    }
    rd_CullBack(false);
    mr_SetBaseTexture(NULL);
    mr_ForgetBindings();
    if (bClose)
        rd_PopMatrix();
    rd_PopAttrib();

    // The shadow, after the model, on the hook's flags.
    if (!bClose && (D.L.m_ObjectFlags & FLAG_SHADOW))
        mr_DrawModelShadow(pInst, model.p, g_pNodes, D.m_nNodes, D.m_iLOD);
}

void mr_DrawModel(LTObject* pInst)
{
    bool bClose = (pInst->m_Flags & FLAG_REALLYCLOSE) != 0;
    ModelView model = mr_ModelOf(pInst);
    bool bFov = model.p && model.bFovScale();
    if (bClose || bFov)
        mr_BeginReallyClose(pInst);
    mr_DrawModelWith(pInst, bClose);
    if (bClose || bFov)
        mr_EndReallyClose();
}

// A translucent model: its own blend, no depth writes.
void mr_DrawTranslucentModel(LTObject* pInst)
{
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_FOG);
    rd_Enable(RD_BLEND);
    mr_ObjectBlend(pInst);
    mr_DrawModel(pInst);
    rd_PopAttrib();
}

// ---- the really-close view ----------------------------------------------------------------
//
// Really-close objects (the player's weapon, the HUD's 3D parts) are in
// camera space, clipped at ReallyCloseNearZ rather than the scene's near
// plane, in the scene's depth buffer. d3d.ren projects them with the
// scene's depth mapping; what is nearer than the near plane goes into the
// front part of the depth buffer the scene leaves free (mr_LoadCamera),
// in order, where the device can do that.
//
// A model with a field of view of its own (Model::m_bFovScale, the
// player-view weapons) scales its camera space by tan of half its field of
// view (d3d.ren's mFov): its own field of view plus ExtraFOVXOffset (in
// degrees) plus the scene's, or, with ModelFovTest, the scene's times the
// sum. The scene's horizontal field of view is widened for a wide screen
// (M-1); as d3d.ren does, the model's is worked out at 4:3 and widened the
// same way.
void mr_BeginReallyClose(LTObject* pObj)
{
    const MRCamera& cam = g_Frame.m_Cam;
    MRCamera close = cam;
    bool bClose = (pObj->m_Flags & FLAG_REALLYCLOSE) != 0;
    if (bClose)
    {
        float fClip = cvf(CV_ReallyCloseNearZ);
        if (fClip < 0.001f)
            fClip = 0.001f;
        if (g_RDCaps.bDepthClamp)
        {
            rd_Enable(RD_DEPTH_CLAMP);
            if (g_RDCaps.bNearDepth)
                rd_NearDepth(fClip, close.m_fNear, 0.0f, MR_CLOSE_DEPTH);
        }
        else
            close.m_fNear = fClip;
    }
    ModelView model = { pObj->m_ObjectType == OT_MODEL ? rd<const uint8_t*>(pObj, MI_MODEL) : NULL };
    if (model.p && model.bFovScale())
    {
        SceneDesc* pScene = g_Frame.m_pScene;
        const float fMin = 10.0f * 3.14159265f / 180.0f, fMax = 170.0f * 3.14159265f / 180.0f;
        float fWide = (float)g.nWidth * 3.0f / ((float)g.nHeight * 4.0f);
        if (!(fWide > 0.0f))
            fWide = 1.0f;
        float fViewX = pScene->m_xFov;
        if (fWide != 1.0f)
            fViewX = 2.0f * atanf(tanf(pScene->m_xFov * 0.5f) / fWide);
        float fx, fy;
        if (cvi(CV_ModelFovTest))
        {
            fx = (cvf(CV_ExtraFOVXOffset) + model.FovX()) * fViewX;
            fy = (cvf(CV_ExtraFOVYOffset) + model.FovY()) * pScene->m_yFov;
        }
        else
        {
            fx = cvf(CV_ExtraFOVXOffset) * 3.14159265f / 180.0f + model.FovX() + fViewX;
            fy = cvf(CV_ExtraFOVYOffset) * 3.14159265f / 180.0f + model.FovY() + pScene->m_yFov;
        }
        fx = fx < fMin ? fMin : fx > fMax ? fMax : fx;
        fy = fy < fMin ? fMin : fy > fMax ? fMax : fy;
        if (fWide != 1.0f && fx > 0.0f && fx < 3.1415926f)
            fx = 2.0f * atanf(tanf(fx * 0.5f) * fWide);
        // x on the screen is tan(fx / 2) x / z: a camera whose tan of half
        // its field of view is 1 / tan(fx / 2).
        close.m_fTanX = 1.0f / tanf(fx * 0.5f);
        close.m_fTanY = 1.0f / tanf(fy * 0.5f);
    }
    mr_LoadCamera(&close);
}

void mr_EndReallyClose(void)
{
    if (g_RDCaps.bDepthClamp)
        rd_Disable(RD_DEPTH_CLAMP);
    if (g_RDCaps.bNearDepth)
        rd_NearDepth(0.0f, 0.0f, 0.0f, 0.0f);
    mr_LoadCamera(&g_Frame.m_Cam);
}
