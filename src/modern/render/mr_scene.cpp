// mr_scene.cpp -- the 3D half: the world context, RenderScene, the world.
//
// RenderScene follows d3d.ren's (src/engine/d3d/ph/d3d_draw.cpp
// d3d_RenderScene and d3d_FullDrawScene, common_draw.cpp d3d_InitFrame,
// tagnodes.cpp):
//
//   * the frame: the camera (near plane NearZ), the world colour and its
//     two colour ramps, the fog;
//   * the visible set: in the normal draw mode the engine's own visibility
//     query walks the PVS and calls back for every visible leaf (whose
//     polygons are queued) and object; the object-list draw mode
//     (ILTClient::RenderObjects) queues just the list;
//   * the sky, from its squashed camera, when sky portals are visible;
//   * the world's polygons (below);
//   * the objects (mr_objects.cpp), the mirrors;
//   * the global light scale and the camera's light add as screen passes.
//
// The world's polygons, as d3d.ren draws them with its default two passes
// (drawworld.cpp, drawgouraud.cpp, drawlightmaps.cpp, drawmodel_clip.cpp):
//
//   * a lightmapped surface (SURF_LIGHTMAP): first the lightmap times the
//     world colour, opaque; then the texture over it, blended
//     DESTCOLOR/SRCCOLOR (twice the product) under Saturate, else
//     ZERO/SRCCOLOR. A fullbright texture is drawn a third time, blended
//     by its alpha. Force1Pass: one pass, the lightmap on stage 1
//     (MODULATE; LightmapsOnly SELECTARG1);
//   * a panning-sky surface (SURF_PANNINGSKY): the same, with the sky-pan
//     texture times the vertex colours (the light ramp) as its lightmap;
//   * every other surface: the texture times the vertex colours through
//     the gamma ramp (the world colour, doubled under Saturate). With
//     dynamic lights (LMDynamic), first the vertex colours, untextured,
//     then each light's small lightmap added (ONE/ONE; two times under
//     Saturate), then the texture over it (ZERO/SRCCOLOR);
//   * a surface with no texture: flat;
//   * a sky portal (SURF_SKY): never drawn; it shows where the sky is.
//
// The fog is d3d.ren's: a factor for each vertex, by its distance from
// the eye (or the vertical fog), not the device's.

#include <math.h>

#include "mrscene.h"
#include "mrobjects.h"

static inline int mr_Clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

// ---- the world context ------------------------------------------------------------

// CreateWorld: the engine binds a loaded world to the renderer (*ppWorld
// is its MainWorld) and keeps what this returns at MainWorld+0x1c8,
// handing it back in every SceneDesc. The lightmaps are built here.
void* mr_CreateWorld(void** ppWorld)
{
    MRContext* pContext = (MRContext*)calloc(1, sizeof(MRContext));
    if (!pContext)
        return NULL;
    pContext->m_FrameCode = 0xffff;
    pContext->m_pWorld = ppWorld ? (MainWorld*)*ppWorld : NULL;
    if (pContext->m_pWorld)
        mr_BuildLightmaps(pContext);
    return pContext;
}

void mr_DestroyWorld(void* pContext)
{
    if (!pContext)
        return;
    mr_FreeLightmaps((MRContext*)pContext);
    free(pContext);
}

// RebindWorld (d3d.ren's d3d_RebindWorld): the lightmaps again.
void mr_RebindWorld(void* pContext)
{
    MRContext* p = (MRContext*)pContext;
    if (!p)
        return;
    mr_FreeLightmaps(p);
    if (p->m_pWorld)
        mr_BuildLightmaps(p);
}

// ---- the frame ----------------------------------------------------------------------

MRFrame g_Frame;

static void mr_QuatToMatrix(const LTRotation& q, float m[3][3])
{
    float n = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    float s = n > 0.0f ? 2.0f / n : 0.0f;
    float xx = q.x * q.x * s, yy = q.y * q.y * s, zz = q.z * q.z * s;
    float xy = q.x * q.y * s, xz = q.x * q.z * s, yz = q.y * q.z * s;
    float wx = q.w * q.x * s, wy = q.w * q.y * s, wz = q.w * q.z * s;
    m[0][0] = 1 - (yy + zz); m[0][1] = xy - wz;       m[0][2] = xz + wy;
    m[1][0] = xy + wz;       m[1][1] = 1 - (xx + zz); m[1][2] = yz - wx;
    m[2][0] = xz - wy;       m[2][1] = yz + wx;       m[2][2] = 1 - (xx + yy);
}

void mr_RotationMatrix(const LTRotation& q, float m[3][3])
{
    mr_QuatToMatrix(q, m);
}

// The scene's view: its rect, place, rotation and field of view (which
// the engine has already widened for a wide screen, M-1), the near plane
// NearZ (d3d_InitFrame).
static void mr_SetupCamera(SceneDesc* pScene, MRCamera* pCam)
{
    const LTRect& r = pScene->m_Rect;
    int w = r.right - r.left, h = r.bottom - r.top;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    pCam->m_nViewX = r.left;
    pCam->m_nViewY = r.top;
    pCam->m_nViewW = w;
    pCam->m_nViewH = h;

    pCam->m_Pos = pScene->m_Pos;
    mr_QuatToMatrix(pScene->m_Rotation, pCam->m_R);

    pCam->m_fTanX = tanf(pScene->m_xFov * 0.5f);
    pCam->m_fTanY = tanf(pScene->m_yFov * 0.5f);

    float fNear = cvf(CV_NearZ);
    pCam->m_fNear = fNear > 0.01f ? fNear : 0.01f;
    float fFar = pScene->m_FarZ;
    pCam->m_fFar = fFar < pCam->m_fNear + 1.0f ? pCam->m_fNear + 1.0f : fFar;
    pCam->m_fScale = 1.0f;
    pCam->m_bMirror = false;
}

// The six world-space frustum planes, normals pointing in: a point p is
// inside a plane when N.p - D >= 0.
static void mr_SetupFrustum(MRCamera* pCam)
{
    float tx = pCam->m_fTanX, ty = pCam->m_fTanY;
    float cam[6][4] = {
        { 0, 0, 1, pCam->m_fNear },          // near: z >= near
        { 0, 0, -1, -pCam->m_fFar },         // far: -z >= -far
        { 1, 0, tx, 0 }, { -1, 0, tx, 0 },   // left, right: x/z >= -tan, <= tan
        { 0, 1, ty, 0 }, { 0, -1, ty, 0 },   // bottom, top
    };
    const float (*R)[3] = pCam->m_R;
    for (int i = 0; i < 6; i++)
    {
        float l = sqrtf(cam[i][0] * cam[i][0] + cam[i][1] * cam[i][1] + cam[i][2] * cam[i][2]);
        float nx = cam[i][0] / l, ny = cam[i][1] / l, nz = cam[i][2] / l, d = cam[i][3] / l;
        LTPlane& p = pCam->m_Planes[i];
        p.m_Normal.x = R[0][0] * nx + R[0][1] * ny + R[0][2] * nz;
        p.m_Normal.y = R[1][0] * nx + R[1][1] * ny + R[1][2] * nz;
        p.m_Normal.z = R[2][0] * nx + R[2][1] * ny + R[2][2] * nz;
        p.m_Dist = p.m_Normal.x * pCam->m_Pos.x + p.m_Normal.y * pCam->m_Pos.y +
                   p.m_Normal.z * pCam->m_Pos.z + d;
    }
}

bool mr_SphereVisible(const LTVector& c, float r)
{
    for (int i = 0; i < 6; i++)
    {
        const LTPlane& p = g_Frame.m_Cam.m_Planes[i];
        if (p.m_Normal.x * c.x + p.m_Normal.y * c.y + p.m_Normal.z * c.z - p.m_Dist < -r)
            return false;
    }
    return true;
}

// Projection and view (column-major, rdev.h), and the viewport. The view
// carries the camera's scale (the sky's SkyScale).
void mr_LoadCamera(const MRCamera* pCam)
{
    rd_Viewport(pCam->m_nViewX, pCam->m_nViewY, pCam->m_nViewW, pCam->m_nViewH);

    float n = pCam->m_fNear, f = pCam->m_fFar;
    float P[16] = {
        1.0f / pCam->m_fTanX, 0, 0, 0,
        0, 1.0f / pCam->m_fTanY, 0, 0,
        0, 0, (f + n) / (f - n), 1,
        0, 0, -2.0f * f * n / (f - n), 0,
    };
    rd_MatrixMode(RD_PROJECTION);
    rd_LoadMatrix(P);
    rd_DepthRange(MR_CLOSE_DEPTH, 1.0f);

    const float (*R)[3] = pCam->m_R;
    const LTVector& p = pCam->m_Pos;
    float s = pCam->m_fScale;
    float V[16];
    for (int row = 0; row < 3; row++)
    {
        V[0 * 4 + row] = R[0][row] * s;
        V[1 * 4 + row] = R[1][row] * s;
        V[2 * 4 + row] = R[2][row] * s;
        V[3 * 4 + row] = -(R[0][row] * p.x + R[1][row] * p.y + R[2][row] * p.z) * s;
    }
    V[3] = V[7] = V[11] = 0.0f;
    V[15] = 1.0f;
    rd_MatrixMode(RD_MODELVIEW);
    rd_LoadMatrix(V);
}

void mr_MultMatrix(const LTMatrix& M)
{
    float m[16];
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++)
            m[col * 4 + row] = M.m[row][col];
    rd_MultMatrix(m);
}

static LTVector mr_Xform(const LTMatrix& M, const LTVector& v)
{
    LTVector r;
    r.x = M.m[0][0] * v.x + M.m[0][1] * v.y + M.m[0][2] * v.z + M.m[0][3];
    r.y = M.m[1][0] * v.x + M.m[1][1] * v.y + M.m[1][2] * v.z + M.m[1][3];
    r.z = M.m[2][0] * v.x + M.m[2][1] * v.y + M.m[2][2] * v.z + M.m[2][3];
    return r;
}

// ---- fog (d3d_draw.cpp: d3d_CalcFog, d3d_CalcVFog) ----------------------------------

// Where the fog is measured from: the eye, in the space of the polys
// being drawn (a world model's own, as d3d.ren's SetCameraPos does).
static LTVector g_vFogEye;
static uint32_t g_nVFogMode;            // the eye against the vertical fog: 0 below, 1 above, 2 in
static float    g_fVFogEye;             // d3d.ren's ViewParams::m_fVFog

static inline uint32_t mr_VFogMode(float y)
{
    if (y >= cvf(CV_VFogMaxY))
        return 1;
    if (y <= cvf(CV_VFogMinY))
        return 0;
    return 2;
}

static inline float mr_VFogVal(float y)
{
    float fMinY = cvf(CV_VFogMinY), fMaxY = cvf(CV_VFogMaxY);
    if (y <= fMinY)
        return cvf(CV_VFogMinYVal);
    if (y >= fMaxY)
        return cvf(CV_VFogMaxYVal);
    return (y - fMinY) / (fMaxY - fMinY) * (cvf(CV_VFogMaxYVal) - cvf(CV_VFogMinYVal)) + cvf(CV_VFogMinYVal);
}

// d3d_viewparams.cpp's SetCameraPos: the eye's own fog.
static void mr_SetFogEye(const LTVector& vEye)
{
    g_vFogEye = vEye;
    g_nVFogMode = mr_VFogMode(vEye.y);
    if (g_nVFogMode == 2)
        g_fVFogEye = mr_VFogVal(vEye.y);
    else
        g_fVFogEye = g_nVFogMode == 1 ? cvf(CV_VFogMaxYVal) : cvf(CV_VFogMinYVal);
}

static inline float mr_Dist(const LTVector& a, const LTVector& b)
{
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// d3d_CalcVFog, with the retail cut at VFogMaxY kept (RETAIL_BUGS.md):
// only the product of the two scales matters.
static float mr_VFogFactor(const LTVector& vPos)
{
    float fMinY = cvf(CV_VFogMinY), fMaxY = cvf(CV_VFogMaxY);
    float fRange = fMaxY != fMinY ? 1.0f / (fMaxY - fMinY) : 0.0f;
    float fValScale = cvf(CV_VFogMaxYVal) - cvf(CV_VFogMinYVal);
    float fDensity = cvf(CV_VFogDensity) != 0.0f ? 255.0f / cvf(CV_VFogDensity) : 0.0f;
    // d3d.ren measures from the camera's world position here (ViewParams
    // m_Pos), even for a world model's polys; its mode is the eye's own.
    const LTVector& vCam = g_Frame.m_Cam.m_Pos;
    float fFog;
    uint32_t nMode = mr_VFogMode(vPos.y);
    if (g_nVFogMode == nMode)
    {
        if (nMode == 2)
        {
            float fPosFog = (vPos.y - fMinY) * fRange * fValScale + g_fVFogEye + cvf(CV_VFogMinYVal);
            fFog = mr_Dist(vCam, vPos) * fPosFog * fDensity;
        }
        else
            fFog = mr_Dist(vCam, vPos) * g_fVFogEye * 2.0f * fDensity;
    }
    else
    {
        float t = vPos.y != vCam.y ? (fMaxY - vCam.y) / (vPos.y - vCam.y) : 0.0f;
        LTVector vHit = { (vPos.x - vCam.x) * t + vCam.x, fMaxY, (vPos.z - vCam.z) * t + vCam.z };
        if (g_nVFogMode == 1)
        {
            float fTop = cvf(CV_VFogMaxYVal) * 2.0f;
            float f1 = mr_Dist(vCam, vHit) * fTop * fDensity;
            float fPosFog = mr_VFogVal(vPos.y) + cvf(CV_VFogMaxYVal);
            fFog = mr_Dist(vHit, vPos) * fPosFog * fDensity + f1;
        }
        else
        {
            float fCamFog = mr_VFogVal(vCam.y) + cvf(CV_VFogMaxYVal);
            float f1 = mr_Dist(vCam, vHit) * fCamFog * fDensity;
            float fTop = cvf(CV_VFogMaxYVal) * 2.0f;
            fFog = mr_Dist(vHit, vPos) * fTop * fDensity + f1;
        }
    }
    if (fFog > cvf(CV_VFogMax))
        fFog = cvf(CV_VFogMax);
    else if (fFog < 0.0f)
        fFog = 0.0f;
    return (255 - (int)(fFog + 0.5f)) / 255.0f;
}

// ---- Primal Hunt's fog volumes (fogvolume.cpp) ---------------------------------------
//
// In Primal Hunt the vertical fog (VFog) is replaced by fog volumes: up to
// four boxes a frame (world models with FLAG2_FOGVOLUME, seen as portal
// objects); a vertex is fogged by how much of the segment from the eye to
// it lies in the first box it touches.

// d3d9.ren is built once for each game, as our d3d.ren is: PRIMAL_HUNT
// for Primal Hunt's (build/ext.sh).
static inline bool mr_IsPrimalHunt(void)
{
#ifdef PRIMAL_HUNT
    return true;
#else
    return false;
#endif
}

struct MRFogBox {
    LTObject* m_pObj;
    LTVector  m_Min, m_Max, m_Center, m_Dims;
    LTPlane   m_Planes[6];
};
#define MR_MAXFOGVOLUMES 4
static MRFogBox g_FogBoxes[MR_MAXFOGVOLUMES];
static int      g_nFogBoxes;
static bool     g_bFogBoxColor;
static uint8_t  g_FogBoxColor[3];

static void mr_FogBoxPlanes(MRFogBox* b)
{
    static const float N[6][3] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    const float* d = &b->m_Dims.x;
    for (int i = 0; i < 6; i++)
    {
        LTPlane& p = b->m_Planes[i];
        p.m_Normal.x = N[i][0];
        p.m_Normal.y = N[i][1];
        p.m_Normal.z = N[i][2];
        float e = d[i / 2];
        LTVector q = { b->m_Center.x + N[i][0] * e, b->m_Center.y + N[i][1] * e, b->m_Center.z + N[i][2] * e };
        p.m_Dist = p.m_Normal.x * q.x + p.m_Normal.y * q.y + p.m_Normal.z * q.z;
    }
}

// d3d_ProcessPortalObject: a fog volume, if its alpha is not 0 and there
// is room. (Its view test is retail's: the first clip plane, six times,
// RETAIL_BUGS.md; kept.) FOGFLAG_SETCOLOR (user flag 1) sets the fog's
// colour from the object's.
typedef void (__thiscall* GetBBoxFn)(LTObject* pObj, LTVector* pMin, LTVector* pMax);
static void mr_AddFogVolume(LTObject* pObj)
{
    if (g_nFogBoxes >= MR_MAXFOGVOLUMES || !pObj->m_ColorA)
        return;
    MRFogBox b;
    b.m_pObj = pObj;
    (*(GetBBoxFn**)pObj)[3](pObj, &b.m_Min, &b.m_Max);
    b.m_Dims = { (b.m_Max.x - b.m_Min.x) * 0.5f, (b.m_Max.y - b.m_Min.y) * 0.5f, (b.m_Max.z - b.m_Min.z) * 0.5f };
    b.m_Center = { b.m_Min.x + b.m_Dims.x, b.m_Min.y + b.m_Dims.y, b.m_Min.z + b.m_Dims.z };
    mr_FogBoxPlanes(&b);
    // ClassifyPlane against the first plane: behind it, not visible.
    const LTPlane& P = g_Frame.m_Cam.m_Planes[0];
    LTVector c0, c1;
    c0.x = P.m_Normal.x < 0 ? b.m_Min.x : b.m_Max.x; c1.x = P.m_Normal.x < 0 ? b.m_Max.x : b.m_Min.x;
    c0.y = P.m_Normal.y < 0 ? b.m_Min.y : b.m_Max.y; c1.y = P.m_Normal.y < 0 ? b.m_Max.y : b.m_Min.y;
    c0.z = P.m_Normal.z < 0 ? b.m_Min.z : b.m_Max.z; c1.z = P.m_Normal.z < 0 ? b.m_Max.z : b.m_Min.z;
    float f0 = P.m_Normal.x * c0.x + P.m_Normal.y * c0.y + P.m_Normal.z * c0.z - P.m_Dist;
    float f1 = P.m_Normal.x * c1.x + P.m_Normal.y * c1.y + P.m_Normal.z * c1.z - P.m_Dist;
    if (f0 < -0.001f && f1 < -0.001f)
        return;
    g_FogBoxes[g_nFogBoxes++] = b;
    if (pObj->m_UserFlags & 1)
    {
        g_bFogBoxColor = true;
        g_FogBoxColor[0] = pObj->m_ColorR;
        g_FogBoxColor[1] = pObj->m_ColorG;
        g_FogBoxColor[2] = pObj->m_ColorB;
    }
}

static bool mr_InBox(const LTVector& p, const MRFogBox& b)
{
    return p.x >= b.m_Min.x && p.y >= b.m_Min.y && p.z >= b.m_Min.z &&
           p.x <= b.m_Max.x && p.y <= b.m_Max.y && p.z <= b.m_Max.z;
}

// d3d_SegmentTouchesBox: separating axes.
static bool mr_SegmentTouchesBox(const LTVector& a, const LTVector& bp, const MRFogBox& b)
{
    LTVector h = { (bp.x - a.x) * 0.5f, (bp.y - a.y) * 0.5f, (bp.z - a.z) * 0.5f };
    float fLen = sqrtf(h.x * h.x + h.y * h.y + h.z * h.z);
    if (fLen <= 0.0f)
        return mr_InBox(a, b);
    LTVector d = { h.x / fLen, h.y / fLen, h.z / fLen };
    LTVector m = { a.x + h.x, a.y + h.y, a.z + h.z };
    LTVector t = { b.m_Center.x - m.x, b.m_Center.y - m.y, b.m_Center.z - m.z };
    const LTVector& e = b.m_Dims;
    if (fabsf(t.x) > fabsf(d.x) * fLen + e.x || fabsf(t.y) > fabsf(d.y) * fLen + e.y ||
        fabsf(t.z) > fabsf(d.z) * fLen + e.z)
        return false;
    if (fabsf(t.y * d.z - t.z * d.y) > e.z * fabsf(d.y) + e.y * fabsf(d.z))
        return false;
    if (fabsf(t.z * d.x - t.x * d.z) > e.x * fabsf(d.z) + e.z * fabsf(d.x))
        return false;
    if (fabsf(t.x * d.y - t.y * d.x) > e.x * fabsf(d.y) + e.y * fabsf(d.x))
        return false;
    return true;
}

static bool mr_SegmentPlane(const LTPlane& P, const LTVector& a, const LTVector& b, LTVector* pHit, float* pT)
{
    LTVector v = { b.x - a.x, b.y - a.y, b.z - a.z };
    float fDot = v.x * P.m_Normal.x + v.y * P.m_Normal.y + v.z * P.m_Normal.z;
    if (fabsf(fDot) < 0.001f)
        return false;
    float t = (P.m_Dist - (a.x * P.m_Normal.x + a.y * P.m_Normal.y + a.z * P.m_Normal.z)) / fDot;
    *pT = t;
    if (t < 0.0f || t > 1.0f)
        return false;
    *pHit = { a.x + v.x * t, a.y + v.y * t, a.z + v.z * t };
    return true;
}

static bool mr_OnFace(const LTVector& p, const MRFogBox& b, int i)
{
    switch (i)
    {
    case 0: case 1: return !(p.y < b.m_Min.y || p.z < b.m_Min.z || p.y > b.m_Max.y || p.z > b.m_Max.z);
    case 2: case 3: return !(p.x < b.m_Min.x || p.z < b.m_Min.z || p.x > b.m_Max.x || p.z > b.m_Max.z);
    default:        return !(p.x < b.m_Min.x || p.y < b.m_Min.y || p.x > b.m_Max.x || p.y > b.m_Max.y);
    }
}

// CFogBox::IntersectSegment: 0, 1 or 2 crossings, in order along it.
static int mr_BoxSegment(const MRFogBox& b, const LTVector& a, const LTVector& e, LTVector* h1, LTVector* h2)
{
    float t1 = 0, t2 = 0;
    int i;
    for (i = 0; i < 6; i++)
        if (mr_SegmentPlane(b.m_Planes[i], a, e, h1, &t1) && mr_OnFace(*h1, b, i))
            break;
    if (i > 5)
        return 0;
    if (i == 5)
        return 1;
    for (i++; i < 6; i++)
        if (mr_SegmentPlane(b.m_Planes[i], a, e, h2, &t2) && mr_OnFace(*h2, b, i))
            break;
    if (i == 6)
        return 1;
    if (t1 > t2)
    {
        LTVector v = *h1;
        *h1 = *h2;
        *h2 = v;
    }
    return 2;
}

// d3d_CalcFogVolume.
static float mr_FogVolumeFactor(const LTVector& pos)
{
    const LTVector& eye = g_Frame.m_Cam.m_Pos;
    int i;
    for (i = 0; i < g_nFogBoxes; i++)
        if (mr_SegmentTouchesBox(eye, pos, g_FogBoxes[i]))
            break;
    if (i >= g_nFogBoxes)
        return 1.0f;
    const MRFogBox& b = g_FogBoxes[i];
    LTObject* pObj = b.m_pObj;
    bool bEye = mr_InBox(eye, b), bPos = mr_InBox(pos, b);
    float fDepth = (pObj->m_UserFlags & 2) ? (float)((pObj->m_UserFlags & 0x3fff0) >> 2) : cvf(CV_VFogDensity);
    LTVector h1 = eye, h2 = pos;
    const LTVector *a, *c;
    if (bEye && bPos)
    {
        a = &eye;
        c = &pos;
    }
    else if (!bEye && bPos)
    {
        int n = mr_BoxSegment(b, eye, pos, &h1, &h2);
        a = &h1;
        c = n == 1 ? &pos : &h2;
    }
    else if (bEye && !bPos)
    {
        int n = mr_BoxSegment(b, eye, pos, &h1, &h2);
        a = n == 1 ? &eye : &h1;
        c = n == 1 ? &h1 : &h2;
    }
    else
    {
        mr_BoxSegment(b, eye, pos, &h1, &h2);
        a = &h1;
        c = &h2;
    }
    float fFog = fDepth != 0.0f ? (fDepth - mr_Dist(*a, *c)) / fDepth : 1.0f;
    fFog = fFog < 0.0f ? 0.0f : fFog > 1.0f ? 1.0f : fFog;
    if (pObj->m_ColorA < 0xff)
    {
        float v = (1.0f - pObj->m_ColorA / 255.0f) * 255.0f + fFog * 255.0f;
        int n = (int)(v + 0.5f);
        return (n > 255 ? 255 : n) / 255.0f;
    }
    return (int)(fFog * 255.0f + 0.5f) / 255.0f;
}

float mr_FogFactor(const LTVector& pos)
{
    if (!g_Frame.m_bFog)
        return 1.0f;
    if (cvi(CV_VFog))
        return mr_IsPrimalHunt() ? mr_FogVolumeFactor(pos) : mr_VFogFactor(pos);
    float fNear = cvf(CV_FogNearZ), fFar = cvf(CV_FogFarZ);
    float d = mr_Dist(pos, g_vFogEye);
    if (d < fNear)
        return 1.0f;
    if (d > fFar)
        return 0.0f;
    float fScale = fFar > fNear ? 255.0f / (fFar - fNear) : 0.0f;
    return (255 - (int)((d - fNear) * fScale + 0.5f)) / 255.0f;
}

// The world's fog: on when FogEnable; the colour FogR/G/B; the factors are
// the vertices' own.
static void mr_SetFog(void)
{
    g_Frame.m_bFog = cvi(CV_FogEnable) != 0;
    if (!g_Frame.m_bFog)
    {
        rd_Disable(RD_FOG);
        rd_FogExplicit(false);
        return;
    }
    if (g_bFogBoxColor)
        rd_FogColor(g_FogBoxColor[0] / 255.0f, g_FogBoxColor[1] / 255.0f, g_FogBoxColor[2] / 255.0f);
    else
        rd_FogColor(cvf(CV_FogR) / 255.0f, cvf(CV_FogG) / 255.0f, cvf(CV_FogB) / 255.0f);
    rd_FogExplicit(true);
    rd_Enable(RD_FOG);
}

// An object that asks for no fog (FLAG_FOGDISABLE) turns it off while it
// draws.
void mr_ObjectFog(LTObject* pObj, bool bBegin)
{
    if (!g_Frame.m_bFog || !(pObj->m_Flags & FLAG_FOGDISABLE))
        return;
    if (bBegin)
        rd_Disable(RD_FOG);
    else
        rd_Enable(RD_FOG);
}

// ---- texture bindings ------------------------------------------------------------------

static MRTexture* g_pBoundTex;

void mr_SetBaseTexture(MRTexture* pTex)
{
    if (pTex == g_pBoundTex)
        return;
    if (pTex)
    {
        rd_Enable(RD_TEXTURE_2D);
        rd_BindTexture(pTex->m_Id);
    }
    else
        rd_Disable(RD_TEXTURE_2D);
    g_pBoundTex = pTex;
}

void mr_ForgetBindings(void)
{
    g_pBoundTex = (MRTexture*)-1;
}

MRTexture* mr_SurfaceTexture(Surface* pSurf)
{
    if (!pSurf->m_pTexture)
        return NULL;
    mr_TextureId(pSurf->m_pTexture);
    return (MRTexture*)pSurf->m_pTexture->m_pRenderData;
}

static MRTexture* mr_LinkedOf(SharedTexture* pBase, bool bEnv)
{
    if (!pBase || !pBase->m_pLinkedTexture || (pBase->m_nLinkType != 0) != bEnv)
        return NULL;
    mr_TextureId(pBase->m_pLinkedTexture);
    return (MRTexture*)pBase->m_pLinkedTexture->m_pRenderData;
}

MRTexture* mr_EnvMapOf(SharedTexture* pBase)
{
    return mr_LinkedOf(pBase, true);
}

// d3d_CalcEnvMapUV (common_draw.cpp): the eye's reflection off the
// surface, its x (or z, for walls facing along x) and y mapped to 0..1.
void mr_ReflectCoords(const LTVector& vCam, const LTVector& pos, const LTVector& N, float* pS, float* pT)
{
    float dx = pos.x - vCam.x, dy = pos.y - vCam.y, dz = pos.z - vCam.z;
    float k = 2.0f * (dx * N.x + dy * N.y + dz * N.z);
    float rx = dx - N.x * k, ry = dy - N.y * k, rz = dz - N.z * k;
    float len = sqrtf(rx * rx + ry * ry + rz * rz);
    if (len > 0.0f)
    {
        rx /= len; ry /= len; rz /= len;
    }
    float s = (N.y * N.y > 0.54439f || N.x * N.x < N.z * N.z) ? rx : rz;
    *pS = 0.5f - s * 0.5f;
    *pT = 0.5f - ry * 0.5f;
}

// ---- the world's polygons ----------------------------------------------------------------

enum MRPolyKind { PK_LIGHTMAP, PK_PANSKY, PK_GOURAUD, PK_FLAT };

struct MRDrawPoly {
    MRTexture* m_pTex;
    WorldPoly* m_pPoly;
    uint8_t    m_Kind;
};

static uint32_t g_nStatLMFlag, g_nStatLMPage, g_nStatPolys[4], g_nStatLights, g_nStatWM, g_nStatFrames;
static MRDrawPoly* g_pDrawList;
static uint32_t    g_nDrawList, g_nDrawAlloc;

static void mr_QueuePoly(MRTexture* pTex, WorldPoly* pPoly, MRPolyKind eKind)
{
    if (g_nDrawList == g_nDrawAlloc)
    {
        uint32_t n = g_nDrawAlloc ? g_nDrawAlloc * 2 : 4096;
        MRDrawPoly* p = (MRDrawPoly*)realloc(g_pDrawList, n * sizeof(MRDrawPoly));
        if (!p)
            return;
        g_pDrawList = p;
        g_nDrawAlloc = n;
    }
    g_pDrawList[g_nDrawList].m_pTex = pTex;
    g_pDrawList[g_nDrawList].m_pPoly = pPoly;
    g_pDrawList[g_nDrawList].m_Kind = (uint8_t)eKind;
    g_nDrawList++;
}

static int mr_CompareDrawPoly(const void* a, const void* b)
{
    const MRDrawPoly* x = (const MRDrawPoly*)a;
    const MRDrawPoly* y = (const MRDrawPoly*)b;
    if (x->m_Kind != y->m_Kind)
        return x->m_Kind < y->m_Kind ? -1 : 1;
    if (x->m_pTex != y->m_pTex)
        return (uintptr_t)x->m_pTex < (uintptr_t)y->m_pTex ? -1 : 1;
    MRPolyLM* lx = (MRPolyLM*)x->m_pPoly->m_pLMPage;
    MRPolyLM* ly = (MRPolyLM*)y->m_pPoly->m_pLMPage;
    void* ax = lx ? lx->m_pAtlas : NULL;
    void* ay = ly ? ly->m_pAtlas : NULL;
    return (uintptr_t)ax < (uintptr_t)ay ? -1 : (uintptr_t)ax > (uintptr_t)ay ? 1 : 0;
}

// The camera in the space of the polys being drawn (for the back-face
// test, the fog and environment maps).
static LTVector g_vPolyCam;

// Queue a poly unless it faces away from the camera (tagnodes.cpp: its
// plane's distance to the eye at most 0.01), by its surface (tagnodes.cpp
// d3d_TagVisibleLeaves).
static void mr_QueueFacingPoly(WorldPoly* pPoly, const LTVector& vCam, float fEps)
{
    Surface* pSurf = pPoly->m_pSurface;
    const LTPlane* pPlane = pPoly->m_pPlane;
    if (!pSurf || !pPlane)
        return;
    if (pPlane->m_Normal.x * vCam.x + pPlane->m_Normal.y * vCam.y + pPlane->m_Normal.z * vCam.z -
        pPlane->m_Dist <= fEps)
        return;
    uint32_t nFlags = pSurf->m_Flags;
    if (nFlags & SURF_INVISIBLE)
        return;
    if (!pSurf->m_pTexture)
    {
        mr_QueuePoly(NULL, pPoly, PK_FLAT);
        return;
    }
    MRTexture* pTex = mr_SurfaceTexture(pSurf);
    if (nFlags & SURF_LIGHTMAP)
    {
        g_nStatLMFlag++;
        if (pPoly->m_pLMPage)
            g_nStatLMPage++;
    }
    if ((nFlags & SURF_LIGHTMAP) && g_Frame.m_bLightmaps && pPoly->m_pLMPage)
        mr_QueuePoly(pTex, pPoly, PK_LIGHTMAP);
    else if ((nFlags & SURF_PANNINGSKY) && g_Frame.m_bLightmaps)
        mr_QueuePoly(pTex, pPoly, PK_PANSKY);
    else
        mr_QueuePoly(pTex, pPoly, PK_GOURAUD);
}

// d3d_AddLightAnim: a vertex light animation's frames (blended, 0..255)
// added to the vertices' colours, saturating.
static void mr_AddVertLightAnim(WorldPolyVertex* pVerts, uint32_t nVerts, LightAnim* a, const LightAnimRef& r)
{
    if (r.m_iPoly >= a->m_nPolies || a->m_iFrames[0] < 0 || a->m_iFrames[1] < 0 ||
        a->m_iFrames[0] >= a->m_nFrames || a->m_iFrames[1] >= a->m_nFrames)
        return;
    int blend = a->m_Blend & 0xff;
    const LightAnimFrame* f1 = &a->m_Frames[a->m_iFrames[0]][r.m_iPoly];
    const LightAnimFrame* f2 = &a->m_Frames[a->m_iFrames[1]][r.m_iPoly];
    if (blend == 0)
        f2 = f1;
    else if (blend == 0xff)
        f1 = f2;
    uint32_t n = f1->m_nVerts < f2->m_nVerts ? f1->m_nVerts : f2->m_nVerts;
    if (nVerts > n)
        nVerts = n;
    if (!f1->m_pR || !f1->m_pG || !f1->m_pB || !f2->m_pR || !f2->m_pG || !f2->m_pB)
        return;
    for (uint32_t i = 0; i < nVerts; i++)
    {
        int c[3];
        if (f1 == f2)
        {
            c[0] = f1->m_pR[i];
            c[1] = f1->m_pG[i];
            c[2] = f1->m_pB[i];
        }
        else
        {
            int ib = 255 - blend;
            c[0] = f2->m_pR[i] * blend / 255 + f1->m_pR[i] * ib / 255;
            c[1] = f2->m_pG[i] * blend / 255 + f1->m_pG[i] * ib / 255;
            c[2] = f2->m_pB[i] * blend / 255 + f1->m_pB[i] * ib / 255;
            for (int j = 0; j < 3; j++)
                c[j] = c[j] > 255 ? 255 : c[j];
        }
        int r2 = pVerts[i].m_R + c[0], g2 = pVerts[i].m_G + c[1], b2 = pVerts[i].m_B + c[2];
        pVerts[i].m_R = (uint8_t)(r2 > 255 ? 255 : r2);
        pVerts[i].m_G = (uint8_t)(g2 > 255 ? 255 : g2);
        pVerts[i].m_B = (uint8_t)(b2 > 255 ? 255 : b2);
    }
}

// The engine marks a poly whose vertex light animations changed (0x4000):
// its vertex colours are cleared and every animation that touches it
// added again (tagnodes.cpp, drawworldmodel.cpp d3d_LightWorldPoly).
static void mr_RefreshVertLights(WorldPoly* pPoly)
{
    if (!(pPoly->m_Flags & WPF_VERTLIGHTANIM))
        return;
    pPoly->m_Flags &= ~WPF_VERTLIGHTANIM;
    MainWorld* pWorld = g_Frame.m_pContext ? g_Frame.m_pContext->m_pWorld : NULL;
    if (!pWorld)
        return;
    WorldPolyVertex* pVerts = pPoly->m_Verts;
    uint32_t nVerts = pPoly->m_nVerts;
    if (cvi(CV_FixTJunc) && pPoly->m_pFixVerts)
    {
        pVerts = pPoly->m_pFixVerts;
        nVerts = pPoly->m_nFixVerts;
    }
    for (uint32_t i = 0; i < nVerts; i++)
        pVerts[i].m_B = pVerts[i].m_G = pVerts[i].m_R = pVerts[i].m_A = 0;
    LightAnim* pAnims = mw_LightAnims(pWorld);
    for (uint32_t i = 0; pPoly->m_pLightAnims && i < pPoly->m_nLightAnims; i++)
    {
        const LightAnimRef& r = pPoly->m_pLightAnims[i];
        if (r.m_iAnim >= pWorld->m_LightAnims.m_nElements)
            continue;
        LightAnim* a = &pAnims[r.m_iAnim];
        if (a->m_iFrames[0] != -1 && a->m_fBrightness >= 0.02f)
            mr_AddVertLightAnim(pVerts, nVerts, a, r);
    }
}

// ---- dynamic lights on the world (drawlight.cpp) ------------------------------------------

static MRWorldLight g_WLights[MR_MAXWORLDLIGHTS];
static uint32_t     g_nWLights;

// The frame's lights that light the world, in the space of the BSP about to
// draw (pBack: world -> model; NULL for the main world).
static void mr_SetupWorldLights(const LTMatrix* pBack)
{
    g_nWLights = 0;
    if (!cvi(CV_DynamicLight) || !g_Frame.m_pContext)
        return;
    bool bFast = cvi(CV_FastLight) != 0;
    MRObjList* pList = mr_ObjList(VS_LIGHTS);
    for (uint32_t i = 0; i < pList->m_nObjects && g_nWLights < MR_MAXWORLDLIGHTS; i++)
    {
        LTObject* L = pList->m_pObjects[i];
        float r = obj_LightRadius(L);
        if ((L->m_Flags & FLAG_ONLYLIGHTOBJECTS) || r <= 0.0f)
            continue;
        MRWorldLight& W = g_WLights[g_nWLights++];
        W.m_Pos = pBack ? mr_Xform(*pBack, L->m_Pos) : L->m_Pos;
        W.m_fRadius = r;
        W.m_Color[0] = L->m_ColorR;
        W.m_Color[1] = L->m_ColorG;
        W.m_Color[2] = L->m_ColorB;
        W.m_bSolid = bFast || (L->m_Flags & FLAG_SOLIDLIGHT);
        W.m_bFrontOnly = (L->m_Flags & FLAG_DONTLIGHTBACKFACING) != 0;
    }
}

// The light's sphere reaches the poly's plane and its bounding sphere.
static bool mr_LightTouchesPoly(const MRWorldLight& L, const WorldPoly* pPoly)
{
    const LTPlane* pPlane = pPoly->m_pPlane;
    if (pPlane)
    {
        float d = pPlane->m_Normal.x * L.m_Pos.x + pPlane->m_Normal.y * L.m_Pos.y +
                  pPlane->m_Normal.z * L.m_Pos.z - pPlane->m_Dist;
        if (d >= L.m_fRadius || d <= -L.m_fRadius || (L.m_bFrontOnly && d < 0.0f))
            return false;
    }
    float dx = pPoly->m_Center.x - L.m_Pos.x, dy = pPoly->m_Center.y - L.m_Pos.y,
          dz = pPoly->m_Center.z - L.m_Pos.z, rr = L.m_fRadius + pPoly->m_Radius;
    return dx * dx + dy * dy + dz * dz < rr * rr;
}

static uint32_t mr_PolyLights(const WorldPoly* pPoly, MRWorldLight* pOut)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < g_nWLights; i++)
        if (mr_LightTouchesPoly(g_WLights[i], pPoly))
            pOut[n++] = g_WLights[i];
    return n;
}

// ---- drawing a poly ---------------------------------------------------------------------

static const WorldPolyVertex* mr_PolyVerts(const WorldPoly* pPoly, uint32_t* pnVerts)
{
    if (cvi(CV_FixTJunc) && pPoly->m_pFixVerts)
    {
        *pnVerts = pPoly->m_nFixVerts;
        return pPoly->m_pFixVerts;
    }
    *pnVerts = pPoly->m_nVerts;
    return pPoly->m_Verts;
}

static void mr_DrawPolyDynLights(WorldPoly* pPoly, const WorldPolyVertex* pVerts, uint32_t nVerts,
                                 const MRWorldLight* pLights, uint32_t nLights, float fScale, int nPasses);

static inline void mr_Fog(const LTVector& pos)
{
    if (g_Frame.m_bFog)
        rd_FogFactor(mr_FogFactor(pos));
}

// The sky-pan texture (the clouds' shadows on SURF_PANNINGSKY polys):
// RenderStruct's PanInfo[0], planar on world XZ, its offsets animated by
// the engine (d3d_SetupScreenScale, d3d_DrawDeferredPiece).
struct PanInfo { SharedTexture* m_pTexture; float m_xOffset, m_zOffset, m_xScale, m_zScale; };
static MRTexture* g_pPanTex;

static void mr_SetupPan(void)
{
    g_pPanTex = NULL;
    const PanInfo* pPan = (const PanInfo*)g.pStruct->m_GlobalPanInfo;
    if (!pPan->m_pTexture || pPan->m_xScale == 0.0f || pPan->m_zScale == 0.0f)
        return;
    mr_TextureId(pPan->m_pTexture);
    g_pPanTex = (MRTexture*)pPan->m_pTexture->m_pRenderData;
}

// u = (x + xOffset) / xScale, in the texture's texels (d3d_SetupScreenScale:
// times the bound texture's texel size).
static void mr_PanCoords(const LTVector& pos, float* pS, float* pT)
{
    const PanInfo* pPan = (const PanInfo*)g.pStruct->m_GlobalPanInfo;
    *pS = (pos.x + pPan->m_xOffset) / pPan->m_xScale * g_pPanTex->m_fUMul;
    *pT = (pos.z + pPan->m_zOffset) / pPan->m_zScale * g_pPanTex->m_fVMul;
}

// The lightmap coordinates of a world point (d3d_SetupLMPolyCoords).
struct MRLMCoords {
    LTVector P, Q;
    float    fInvGrid, fOffU, fOffV, fInvSize;
};

static void mr_LMCoordsSetup(const WorldPoly* pPoly, MRLMCoords* c)
{
    const MRPolyLM* pLM = (const MRPolyLM*)pPoly->m_pLMPage;
    mr_LMVectors(pPoly, &c->P, &c->Q);
    c->fInvGrid = 1.0f / g_Frame.m_fLMGrid;
    c->fOffU = pLM->m_X + 0.5f;
    c->fOffV = pLM->m_Y + 0.5f;
    c->fInvSize = 1.0f / pLM->m_pAtlas->m_nSize;
}

static inline void mr_LMCoords(const WorldPoly* pPoly, const MRLMCoords& c, const LTVector& pos,
                               float* pS, float* pT)
{
    float dx = pos.x - pPoly->m_LMOrigin.x, dy = pos.y - pPoly->m_LMOrigin.y, dz = pos.z - pPoly->m_LMOrigin.z;
    *pS = ((dx * c.P.x + dy * c.P.y + dz * c.P.z) * c.fInvGrid + c.fOffU) * c.fInvSize;
    *pT = ((dx * c.Q.x + dy * c.Q.y + dz * c.Q.z) * c.fInvGrid + c.fOffV) * c.fInvSize;
}

// The light layer of a lightmapped or panning-sky poly: its lightmap
// times the world colour, or the pan texture times the vertex colours
// through the light ramp. Opaque, on unit 0.
static void mr_DrawLightLayer(WorldPoly* pPoly, MRPolyKind eKind, uint8_t nAlpha)
{
    uint32_t nVerts;
    const WorldPolyVertex* pVerts = mr_PolyVerts(pPoly, &nVerts);
    MRLMCoords lc;
    if (eKind == PK_LIGHTMAP)
        mr_LMCoordsSetup(pPoly, &lc);
    const uint8_t* wc = g_Frame.m_WorldColor;
    rd_Begin(RD_TRIANGLE_FAN);
    for (uint32_t v = 0; v < nVerts; v++)
    {
        const LTVector& pos = *pVerts[v].m_pPos;
        float s, t;
        if (eKind == PK_LIGHTMAP)
        {
            mr_LMCoords(pPoly, lc, pos, &s, &t);
            rd_Color4ub(wc[0], wc[1], wc[2], nAlpha);
        }
        else
        {
            mr_PanCoords(pos, &s, &t);
            rd_Color4ub(g_Frame.m_LightRamp[0][pVerts[v].m_R], g_Frame.m_LightRamp[1][pVerts[v].m_G],
                        g_Frame.m_LightRamp[2][pVerts[v].m_B], nAlpha);
        }
        rd_TexCoord2f(s, t);
        mr_Fog(pos);
        rd_Vertex3f(pos.x, pos.y, pos.z);
    }
    rd_End();
}

// The texture pass over a light layer (d3d_DrawPieceQueue): the texture
// (unit 0), with its detail texture or environment map on unit 1, white,
// or with the light layer's own colour in one pass (Force1Pass).
static void mr_DrawTexturePass(WorldPoly* pPoly, MRTexture* pTex, MRTexture* pDetail, MRTexture* pEnv,
                               const uint8_t* pRGBA, bool bLMUnit)
{
    uint32_t nVerts;
    const WorldPolyVertex* pVerts = mr_PolyVerts(pPoly, &nVerts);
    float fU = pTex ? pTex->m_fUMul : 0.0f, fV = pTex ? pTex->m_fVMul : 0.0f;
    float fDA = 0, fDB = 0, fDC = 0, fDD = 0;
    if (pDetail)
    {
        // d3d_DrawDetailPieces: the base coordinates scaled and turned.
        float fScale = cvf(CV_DetailTextureScale) * pTex->m_fDetailScale;
        fDA = pTex->m_fDetailCos * pDetail->m_fUMul * fScale;
        fDB = -pTex->m_fDetailSin * pDetail->m_fVMul * fScale;
        fDC = pTex->m_fDetailSin * pDetail->m_fUMul * fScale;
        fDD = pTex->m_fDetailCos * pDetail->m_fVMul * fScale;
    }
    MRLMCoords lc;
    if (bLMUnit)
        mr_LMCoordsSetup(pPoly, &lc);
    rd_Begin(RD_TRIANGLE_FAN);
    for (uint32_t v = 0; v < nVerts; v++)
    {
        const LTVector& pos = *pVerts[v].m_pPos;
        rd_Color4ub(pRGBA[0], pRGBA[1], pRGBA[2], pRGBA[3]);
        rd_TexCoord2f(pVerts[v].m_U * fU, pVerts[v].m_V * fV);
        if (pDetail)
            rd_MultiTexCoord2f(1, pVerts[v].m_U * fDA + pVerts[v].m_V * fDB,
                               pVerts[v].m_U * fDC + pVerts[v].m_V * fDD);
        else if (pEnv)
        {
            float s, t;
            mr_ReflectCoords(g_vPolyCam, pos, pPoly->m_pPlane->m_Normal, &s, &t);
            rd_MultiTexCoord2f(1, s * cvf(CV_DetailTextureScale), t * cvf(CV_DetailTextureScale));
        }
        if (bLMUnit)
        {
            float s, t;
            mr_LMCoords(pPoly, lc, pos, &s, &t);
            rd_MultiTexCoord2f(1, s, t);
        }
        mr_Fog(pos);
        rd_Vertex3f(pos.x, pos.y, pos.z);
    }
    rd_End();
}

static inline void mr_Unit1(uint32_t nId, RDCombineOp eOp, float fScale)
{
    rd_ActiveTexture(1);
    if (nId)
    {
        RDCombine c = { eOp, RD_ARG_TEXTURE, RD_ARG_PREVIOUS, fScale, RD_OP_REPLACE, RD_ARG_PREVIOUS, RD_ARG_PREVIOUS };
        rd_TexCombine(c);
        rd_Enable(RD_TEXTURE_2D);
        rd_BindTexture(nId);
    }
    else
    {
        rd_Disable(RD_TEXTURE_2D);
        rd_TexEnv(RD_ENV_MODULATE);
    }
    rd_ActiveTexture(0);
}

// The detail texture or environment map a lightmapped poly's texture
// takes on unit 1 (d3d_DrawPieceQueue).
static void mr_SecondTexture(WorldPoly* pPoly, MRTexture** ppDetail, MRTexture** ppEnv)
{
    *ppDetail = *ppEnv = NULL;
    if (!g_RDCaps.bCombine || g_RDCaps.nTextureUnits < 2)
        return;
    SharedTexture* pShared = pPoly->m_pSurface->m_pTexture;
    if (cvi(CV_EnvMapWorld))
        *ppEnv = mr_LinkedOf(pShared, true);
    if (!*ppEnv && cvi(CV_DetailTextures))
        *ppDetail = mr_LinkedOf(pShared, false);
}

// A lightmapped or panning-sky poly: the light layer, then the texture
// blended over it (two times the product under Saturate), then a
// fullbright texture's bright texels over that. Force1Pass: one pass.
static void mr_DrawLitPoly(WorldPoly* pPoly, MRTexture* pTex, MRPolyKind eKind, uint8_t nAlpha)
{
    static MRTexture s_Atlas;
    MRTexture* pLayer = NULL;
    if (eKind == PK_LIGHTMAP)
    {
        MRPolyLM* pLM = (MRPolyLM*)pPoly->m_pLMPage;
        s_Atlas.m_Id = pLM->m_pAtlas->m_Id;
        pLayer = &s_Atlas;
    }
    else
        pLayer = g_pPanTex;
    if (!pLayer || !pLayer->m_Id)
        return;

    if (cvi(CV_Force1Pass) && eKind == PK_LIGHTMAP && g_RDCaps.nTextureUnits >= 2 && g_RDCaps.bCombine)
    {
        // d3d_SetLightmapStages: the texture times the world colour, then
        // the lightmap (MODULATE; LightmapsOnly SELECTARG1).
        mr_SetBaseTexture(pTex);
        rd_TexEnv(RD_ENV_MODULATE);
        mr_Unit1(pLayer->m_Id, cvi(CV_LightmapsOnly) ? RD_OP_REPLACE : RD_OP_MODULATE, 1.0f);
        const uint8_t* wc = g_Frame.m_WorldColor;
        uint8_t rgba[4] = { wc[0], wc[1], wc[2], nAlpha };
        mr_DrawTexturePass(pPoly, pTex, NULL, NULL, rgba, true);
        mr_Unit1(0, RD_OP_MODULATE, 1.0f);
        return;
    }

    // The light layer.
    mr_SetBaseTexture(pLayer);
    rd_TexEnv(RD_ENV_MODULATE);
    mr_DrawLightLayer(pPoly, eKind, nAlpha);
    mr_ForgetBindings();
    // A panning-sky poly's dynamic lights over its light layer, once
    // (d3d_DrawDeferredPiece).
    if (eKind == PK_PANSKY && g_nWLights && cvi(CV_LMDynamic))
    {
        MRWorldLight lights[MR_MAXWORLDLIGHTS];
        uint32_t nLights = mr_PolyLights(pPoly, lights);
        uint32_t nVerts;
        const WorldPolyVertex* pVerts = mr_PolyVerts(pPoly, &nVerts);
        if (nLights)
            mr_DrawPolyDynLights(pPoly, pVerts, nVerts, lights, nLights, 1.0f, 1);
    }
    if (cvi(CV_LightmapsOnly) || cvi(CV_LMFullBright) || !pTex)
        return;

    // The texture over it.
    MRTexture *pDetail, *pEnv;
    mr_SecondTexture(pPoly, &pDetail, &pEnv);
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_DEPTH);
    rd_Enable(RD_BLEND);
    rd_BlendFunc(cvi(CV_Saturate) ? RD_DST_COLOR : RD_ZERO, RD_SRC_COLOR);
    rd_DepthFunc(RD_LEQUAL);
    rd_DepthMask(false);
    mr_SetBaseTexture(pTex);
    rd_TexEnv(RD_ENV_MODULATE);
    if (pDetail)
        mr_Unit1(pDetail->m_Id, cvi(CV_DetailTextureAdd) ? RD_OP_ADD_SIGNED : RD_OP_MODULATE, 1.0f);
    else if (pEnv)
        mr_Unit1(pEnv->m_Id, cvi(CV_DetailTextureAdd) ? RD_OP_ADD_SIGNED : RD_OP_MODULATE, 1.0f);
    static const uint8_t s_White[4] = { 255, 255, 255, 255 };
    mr_DrawTexturePass(pPoly, pTex, pDetail, pEnv, s_White, false);
    // A fullbright texture's bright texels (its alpha) over it.
    if (pTex->m_DtxFlags & DTX_FULLBRITE)
    {
        rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);
        mr_DrawTexturePass(pPoly, pTex, pDetail, pEnv, s_White, false);
    }
    if (pDetail || pEnv)
        mr_Unit1(0, RD_OP_MODULATE, 1.0f);
    rd_PopAttrib();
}

// The small lightmap of a dynamic light on a gouraud poly
// (d3d_DrawDynamicLM, d3d_FillDynamicLM32): the light's sphere cut by the
// poly's plane, in a LMDynamicSize square with a black border.
static uint32_t g_DynLMTex;

static bool mr_MakeDynLM(const WorldPoly* pPoly, const MRWorldLight& L, float fScale)
{
    int nSize = cvi(CV_LMDynamicSize);
    nSize = nSize < 4 ? 4 : nSize > 64 ? 64 : nSize;
    const LTPlane* pPlane = pPoly->m_pPlane;
    float fDist = pPlane->m_Normal.x * L.m_Pos.x + pPlane->m_Normal.y * L.m_Pos.y +
                  pPlane->m_Normal.z * L.m_Pos.z - pPlane->m_Dist;
    float fT = fDist / L.m_fRadius;
    if (fT > 1.0f - 1.0f / nSize)
        return false;
    float fDistSqr = fT * fT;
    int c[3];
    for (int j = 0; j < 3; j++)
    {
        c[j] = (L.m_Color[j] > 127 ? L.m_Color[j] : 127) - 127;
        if (fScale != 1.0f)
            c[j] = (uint8_t)(c[j] * fScale);
    }
    if (!c[0] && !c[1] && !c[2])
        return false;
    static uint32_t s_Pixels[64 * 64];
    memset(s_Pixels, 0, nSize * nSize * 4);
    int w = nSize - 2, h = nSize - 2;
    float fStepX = 2.0f / w, fStepY = 2.0f / h, fY = -1.0f;
    for (int y = 0; y < h; y++, fY += fStepY)
    {
        float fYSqr = fY * fY + fDistSqr, fX = -1.0f;
        uint32_t* pRow = s_Pixels + (y + 1) * nSize + 1;
        for (int x = 0; x < w; x++, fX += fStepX)
        {
            float fD = fX * fX + fYSqr;
            if (fD >= 1.0f)
                continue;
            int s = (int)((1.0f - fD) * 255.0f);
            pRow[x] = 0xff000000 | (uint32_t)(s * c[0] / 255) << 16 | (uint32_t)(s * c[1] / 255) << 8 |
                      (uint32_t)(s * c[2] / 255);
        }
    }
    if (!g_DynLMTex)
        g_DynLMTex = rd_GenTexture();
    rd_BindTexture(g_DynLMTex);
    rd_TexFilter(RD_LINEAR, RD_LINEAR);
    rd_TexWrap(false, false);
    rd_TexMaxLevel(0);
    rd_TexImage(0, RD_FMT_RGB8, nSize, nSize, RD_PIX_BGRA, s_Pixels);
    mr_ForgetBindings();
    return true;
}

// Each dynamic light's small lightmap added over a poly (ONE/ONE, fog
// towards black, CLAMP), its coordinates the vertices' positions on the
// poly's lightmap axes over the light's radius times LMDynamicScale
// (d3d_DrawPolyLights, d3d_DrawLMPolyLights).
static void mr_DrawPolyDynLights(WorldPoly* pPoly, const WorldPolyVertex* pVerts, uint32_t nVerts,
                                 const MRWorldLight* pLights, uint32_t nLights, float fScale, int nPasses)
{
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_DEPTH | RD_ATTR_FOG);
    rd_Enable(RD_BLEND);
    rd_BlendFunc(RD_ONE, RD_ONE);
    rd_DepthMask(false);
    rd_FogColor(0, 0, 0);
    LTVector P, Q;
    mr_LMVectors(pPoly, &P, &Q);
    for (uint32_t l = 0; l < nLights; l++)
    {
        const MRWorldLight& L = pLights[l];
        if (!mr_MakeDynLM(pPoly, L, fScale))
            continue;
        rd_Enable(RD_TEXTURE_2D);
        rd_BindTexture(g_DynLMTex);
        rd_TexEnv(RD_ENV_REPLACE);
        float fInv = 1.0f / (L.m_fRadius * cvf(CV_LMDynamicScale));
        for (int k = nPasses; k > 0; k--)
        {
            rd_Begin(RD_TRIANGLE_FAN);
            for (uint32_t v = 0; v < nVerts; v++)
            {
                const LTVector& pos = *pVerts[v].m_pPos;
                float dx = pos.x - L.m_Pos.x, dy = pos.y - L.m_Pos.y, dz = pos.z - L.m_Pos.z;
                rd_TexCoord2f((P.x * dx + P.y * dy + P.z * dz) * fInv + 0.5f,
                              (Q.x * dx + Q.y * dy + Q.z * dz) * fInv + 0.5f);
                mr_Fog(pos);
                rd_Vertex3f(pos.x, pos.y, pos.z);
            }
            rd_End();
        }
        rd_TexEnv(RD_ENV_MODULATE);
    }
    rd_PopAttrib();
    mr_ForgetBindings();
}

// A gouraud poly: the texture times its vertex colours through the gamma
// ramp. With dynamic lights (LMDynamic, d3d_DrawMultipassGouraud): the
// colours alone, each light's small lightmap added, then the texture
// multiplied over it.
static void mr_DrawGouraudPoly(WorldPoly* pPoly, MRTexture* pTex, uint8_t nAlpha)
{
    uint32_t nVerts;
    const WorldPolyVertex* pVerts = mr_PolyVerts(pPoly, &nVerts);
    float fU = pTex ? pTex->m_fUMul : 0.0f, fV = pTex ? pTex->m_fVMul : 0.0f;

    MRWorldLight lights[MR_MAXWORLDLIGHTS];
    uint32_t nLights = g_nWLights && cvi(CV_LMDynamic) ? mr_PolyLights(pPoly, lights) : 0;
    const uint8_t (*G)[256] = g_Frame.m_GammaRamp;
    if (!nLights)
    {
        mr_SetBaseTexture(pTex);
        rd_TexEnv(RD_ENV_MODULATE);
        rd_Begin(RD_TRIANGLE_FAN);
        for (uint32_t v = 0; v < nVerts; v++)
        {
            const LTVector& pos = *pVerts[v].m_pPos;
            rd_Color4ub(G[0][pVerts[v].m_R], G[1][pVerts[v].m_G], G[2][pVerts[v].m_B], nAlpha);
            rd_TexCoord2f(pVerts[v].m_U * fU, pVerts[v].m_V * fV);
            mr_Fog(pos);
            rd_Vertex3f(pos.x, pos.y, pos.z);
        }
        rd_End();
        return;
    }

    // The vertex colours, untextured.
    mr_SetBaseTexture(NULL);
    rd_Begin(RD_TRIANGLE_FAN);
    for (uint32_t v = 0; v < nVerts; v++)
    {
        const LTVector& pos = *pVerts[v].m_pPos;
        rd_Color4ub(G[0][pVerts[v].m_R], G[1][pVerts[v].m_G], G[2][pVerts[v].m_B], nAlpha);
        mr_Fog(pos);
        rd_Vertex3f(pos.x, pos.y, pos.z);
    }
    rd_End();

    // The lights (d3d_DrawPolyLights), two times under Saturate.
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_DEPTH | RD_ATTR_FOG);
    mr_DrawPolyDynLights(pPoly, pVerts, nVerts, lights, nLights, cvi(CV_MultipassGouraud) ? 1.0f : 2.0f,
                         cvi(CV_Saturate) ? 2 : 1);
    rd_Enable(RD_BLEND);
    rd_DepthMask(false);

    // The texture, multiplied over it.
    if (pTex)
    {
        rd_BlendFunc(RD_ZERO, RD_SRC_COLOR);
        rd_FogColor(1, 1, 1);
        mr_SetBaseTexture(pTex);
        rd_TexEnv(RD_ENV_MODULATE);
        rd_Begin(RD_TRIANGLE_FAN);
        for (uint32_t v = 0; v < nVerts; v++)
        {
            const LTVector& pos = *pVerts[v].m_pPos;
            rd_Color4ub(255, 255, 255, 255);
            rd_TexCoord2f(pVerts[v].m_U * fU, pVerts[v].m_V * fV);
            mr_Fog(pos);
            rd_Vertex3f(pos.x, pos.y, pos.z);
        }
        rd_End();
    }
    rd_PopAttrib();
    mr_ForgetBindings();
}

// A poly with no texture (d3d_DrawDeferredList): flat, in a colour of its
// own (retail's: the surface's address).
static void mr_DrawFlatPoly(WorldPoly* pPoly)
{
    uint32_t nVerts;
    const WorldPolyVertex* pVerts = mr_PolyVerts(pPoly, &nVerts);
    uint32_t c = (uint32_t)(uintptr_t)pPoly->m_pSurface;
    mr_SetBaseTexture(NULL);
    rd_Begin(RD_TRIANGLE_FAN);
    for (uint32_t v = 0; v < nVerts; v++)
    {
        const LTVector& pos = *pVerts[v].m_pPos;
        rd_Color4ub((uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c, 255);
        rd_Vertex3f(pos.x, pos.y, pos.z);
    }
    rd_End();
}

static void mr_DrawQueuedPoly(const MRDrawPoly& d, uint8_t nAlpha)
{
    switch (d.m_Kind)
    {
    case PK_LIGHTMAP:
    case PK_PANSKY:
        mr_DrawLitPoly(d.m_pPoly, d.m_pTex, (MRPolyKind)d.m_Kind, nAlpha);
        break;
    case PK_GOURAUD:
        mr_DrawGouraudPoly(d.m_pPoly, d.m_pTex, nAlpha);
        break;
    default:
        mr_DrawFlatPoly(d.m_pPoly);
        break;
    }
}

// Draw the queue, sorted by kind, texture and lightmap atlas. pBack brings
// the frame's dynamic lights and the camera into the queued polys' space
// (world models).

static void mr_FlushPolys(uint8_t nAlpha, const LTMatrix* pBack)
{
    qsort(g_pDrawList, g_nDrawList, sizeof(MRDrawPoly), mr_CompareDrawPoly);
    mr_SetupWorldLights(pBack);
    mr_SetupPan();
    g_vPolyCam = pBack ? mr_Xform(*pBack, g_Frame.m_Cam.m_Pos) : g_Frame.m_Cam.m_Pos;
    mr_SetFogEye(g_vPolyCam);

    for (uint32_t i = 0; i < g_nDrawList; i++)
        mr_RefreshVertLights(g_pDrawList[i].m_pPoly);
    // Lightmapped polys take their lights into their lightmaps.
    if (g_Frame.m_bLightmaps)
    {
        MRWorldLight lights[MR_MAXWORLDLIGHTS];
        for (uint32_t i = 0; g_nWLights && i < g_nDrawList; i++)
        {
            if (g_pDrawList[i].m_Kind != PK_LIGHTMAP)
                continue;
            WorldPoly* pPoly = g_pDrawList[i].m_pPoly;
            uint32_t n = mr_PolyLights(pPoly, lights);
            if (n)
                mr_LightPolyLM(g_Frame.m_pContext, pPoly, lights, n);
        }
        mr_UploadLightmaps(g_Frame.m_pContext);
    }
    mr_ForgetBindings();
    g_nStatLights += g_nWLights;
    for (uint32_t i = 0; i < g_nDrawList; i++)
    {
        g_nStatPolys[g_pDrawList[i].m_Kind & 3]++;
        mr_DrawQueuedPoly(g_pDrawList[i], nAlpha);
    }
    mr_SetBaseTexture(NULL);
    mr_ForgetBindings();
    g_nDrawList = 0;
    g_nWLights = 0;
    mr_SetFogEye(g_Frame.m_Cam.m_Pos);
}

// ---- visibility -------------------------------------------------------------------

// tagnodes.cpp's d3d_ShouldProcessObject: a type with a handler, not a
// poly grid (they are processed from the leaves), not a sky object, and
// visible.
bool mr_ShouldDraw(LTObject* pObj)
{
    int nType = pObj->m_ObjectType;
    if (nType <= OT_NORMAL || nType >= NUM_OBJECTTYPES || nType == OT_CAMERA || nType == OT_POLYGRID)
        return false;
    if (pObj->m_Flags2 & FLAG2_SKYOBJECT)
        return false;
    return (pObj->m_Flags & FLAG_VISIBLE) != 0;
}

static void mr_ProcessAttachments(LTObject* pObj, uint32_t nDepth)
{
    if (nDepth > 32)
        return;
    for (Attachment* a = pObj->m_pAttachments; a; a = a->m_pNext)
    {
        // The engine places the child at its attachment point.
        LTObject* pChild = g.pStruct->ProcessAttachment(pObj, a);
        if (!pChild || pChild->m_FrameCode == g_Frame.m_ObjectFrameCode)
            continue;
        pChild->m_FrameCode = g_Frame.m_ObjectFrameCode;
        if (mr_ShouldDraw(pChild))
            mr_QueueObject(pChild);
        if (pChild->m_pAttachments)
            mr_ProcessAttachments(pChild, nDepth + 1);
    }
}

static void mr_ReallyProcessObject(LTObject* pObj)
{
    mr_QueueObject(pObj);
    if (pObj->m_pAttachments)
        mr_ProcessAttachments(pObj, 0);
}

static void mr_QueueWorldPoly(WorldPoly* pPoly);

static void cb_Object(LTObject* pObj)
{
    if (pObj->m_InternalFlags & IFLAG_INSKY)
        return;
    if (mr_ShouldDraw(pObj))
        mr_ReallyProcessObject(pObj);
}

// The walk's object-list callback: a node's objects, from the tail, up to
// the first one inside the BSP.
static void cb_AddObjects(LTLink* pHead, LTObject*** ppArray, uint32_t* pnCount)
{
    for (LTLink* pCur = pHead->m_pPrev; pCur != pHead; pCur = pCur->m_pPrev)
    {
        LTObject* pObj = (LTObject*)pCur->m_pData;
        if (pObj->m_pInsideBsp)
            break;
        if (mr_ShouldDraw(pObj))
            (*ppArray)[(*pnCount)++] = pObj;
    }
}

// The leaf callback: its poly grids and its polygons.
static void cb_Leaf(Leaf* pLeaf)
{
    pLeaf->m_FrameCode = g_Frame.m_WorldFrameCode;
    for (LTLink* pCur = pLeaf->m_Objects.m_pNext; pCur && pCur != &pLeaf->m_Objects; pCur = pCur->m_pNext)
    {
        LTObject* pObj = *(LTObject**)((uint8_t*)pCur->m_pData + 0x18);
        if (pObj && pObj->m_ObjectType == OT_POLYGRID && (pObj->m_Flags & FLAG_VISIBLE) &&
            pObj->m_FrameCode != g_Frame.m_ObjectFrameCode)
        {
            pObj->m_FrameCode = g_Frame.m_ObjectFrameCode;
            mr_ReallyProcessObject(pObj);
        }
    }
    for (uint32_t i = 0; i < pLeaf->m_nPolies; i++)
        if (pLeaf->m_Polies[i])
            mr_QueueWorldPoly(pLeaf->m_Polies[i]);
}

// d3d_NodeFilter: a node's box against the six planes.
static int cb_BoxFilter(void* pNode)
{
    const LTVector& c = *(const LTVector*)((const uint8_t*)pNode + 0x0c);
    const LTVector& d = *(const LTVector*)((const uint8_t*)pNode + 0x18);
    for (int i = 0; i < 6; i++)
    {
        const LTPlane& p = g_Frame.m_Cam.m_Planes[i];
        float r = fabsf(p.m_Normal.x) * d.x + fabsf(p.m_Normal.y) * d.y + fabsf(p.m_Normal.z) * d.z;
        if (p.m_Normal.x * c.x + p.m_Normal.y * c.y + p.m_Normal.z * c.z - p.m_Dist < -r)
            return 0;
    }
    return 1;
}

// d3d_PortalFilterCB: a world tree node's sphere against the six planes.
static int cb_NodeFilter(WorldTreeNode* pNode)
{
    return mr_SphereVisible(wtn_Center(pNode), wtn_Radius(pNode));
}

// The world's frame code; at its wrap every leaf and poly stamp is cleared.
static void mr_IncrementFrameCode(MRContext* pContext)
{
    MainWorld* pWorld = pContext->m_pWorld;
    if (pContext->m_FrameCode >= 0xfffe)
    {
        pContext->m_FrameCode = 1;
        for (uint32_t i = 0; i < pWorld->m_Worlds.m_nElements; i++)
        {
            WorldData* pData = mw_World(pWorld, i);
            WorldBsp* pBsp = pData ? pData->m_pWorldBsp : NULL;
            if (!pBsp)
                continue;
            for (uint32_t l = 0; l < pBsp->m_nLeafs; l++)
                pBsp->m_Leafs[l].m_FrameCode = 0;
            for (uint32_t p = 0; p < pBsp->m_nPolies; p++)
                if (pBsp->m_Polies[p])
                    pBsp->m_Polies[p]->m_FrameCode = 0;
        }
    }
    pContext->m_FrameCode++;
}

// ---- mirrors (d3d_draw.cpp d3d_DrawPortals) ------------------------------------------------

#define MR_MAXPORTALS 32
static WorldPoly* g_Portals[MR_MAXPORTALS];
static uint32_t   g_nPortals;
static bool       g_bPortals;           // DrawPortals: mirrors are drawn this frame

// d3d_QueuePoly (tagnodes.cpp): once a frame; a sky portal marks the sky,
// a mirror goes on the portal list, the rest are drawn.
static void mr_QueueWorldPoly(WorldPoly* pPoly)
{
    if (pPoly->m_FrameCode == g_Frame.m_WorldFrameCode)
        return;
    pPoly->m_FrameCode = g_Frame.m_WorldFrameCode;
    Surface* pSurf = pPoly->m_pSurface;
    if (!pSurf || (pSurf->m_Flags & SURF_INVISIBLE))
        return;
    if (pSurf->m_Flags & SURF_SKY)
    {
        g_Frame.m_nSkyPolys++;          // the sky shows here (never drawn itself)
        return;
    }
    if (g_bPortals && pSurf->m_PortalId != PORTAL_NONE)
    {
        if (g_nPortals < MR_MAXPORTALS)
            g_Portals[g_nPortals++] = pPoly;
        return;
    }
    if (cvi(CV_DrawWorld))
        mr_QueueFacingPoly(pPoly, g_Frame.m_Cam.m_Pos, 0.01f);
}

// No PVS (or DrawAll): every object filed in the vis BSP's nodes, and every
// poly (d3d_QueueAllObjects).
static void mr_VisitAll(MainWorld* pWorld)
{
    WorldBsp* pVis = mw_GetVisBsp(pWorld);
    if (!pVis)
        return;
    for (uint32_t i = 0; pVis->m_Nodes && i < pVis->m_nNodes; i++)
    {
        LTLink* pHead = (LTLink*)&pVis->m_Nodes[i].m_Objects;
        for (LTLink* pCur = pHead->m_pPrev; pCur && pCur != pHead; pCur = pCur->m_pPrev)
        {
            LTObject* pObj = (LTObject*)pCur->m_pData;
            if (!pObj || pObj->m_pInsideBsp)
                break;
            if (pObj->m_FrameCode != g_Frame.m_ObjectFrameCode && mr_ShouldDraw(pObj))
            {
                pObj->m_FrameCode = g_Frame.m_ObjectFrameCode;
                mr_ReallyProcessObject(pObj);
            }
        }
    }
    for (uint32_t i = 0; i < pVis->m_nPolies; i++)
        if (pVis->m_Polies[i])
            mr_QueueWorldPoly(pVis->m_Polies[i]);
}

// ---- the world's segment test --------------------------------------------------------
//
// The engine's segment test over the main world's BSP, as d3d.ren
// carries its own copy for the sun test: the segment is split at each
// node's plane, the near part searched first; where it crosses to a
// node's front, the node's poly is hit if it is solid and the crossing is
// inside its edges.

#define MR_ISECT_EPS 0.01f

static WorldPoly* mr_IntersectNode(Node* pNode, LTVector vFrom, LTVector vTo, int nDepth)
{
    while (pNode && !(pNode->m_Flags & 3) && nDepth < 512)
    {
        WorldPoly* pPoly = pNode->m_pPoly;
        if (!pPoly || !pPoly->m_pSurface || !pPoly->m_pPlane)
            return NULL;
        const LTPlane& P = *pPoly->m_pPlane;
        float d1 = P.m_Normal.x * vFrom.x + P.m_Normal.y * vFrom.y + P.m_Normal.z * vFrom.z - P.m_Dist;
        float d2 = P.m_Normal.x * vTo.x + P.m_Normal.y * vTo.y + P.m_Normal.z * vTo.z - P.m_Dist;
        if (d1 > MR_ISECT_EPS && d2 > MR_ISECT_EPS)
        {
            pNode = pNode->m_Sides[1];
            continue;
        }
        int iSide;
        if (d1 < -MR_ISECT_EPS)
        {
            if (d2 < -MR_ISECT_EPS)
            {
                pNode = pNode->m_Sides[0];
                continue;
            }
            iSide = 0;
        }
        else if (d1 > MR_ISECT_EPS)
            iSide = 1;
        else
            iSide = d2 < 0.0f ? 1 : 0;

        float fDenom = d2 - d1;
        if (fDenom != 0.0f && (d1 < -MR_ISECT_EPS || d1 > MR_ISECT_EPS))
        {
            float t = -d1 / fDenom;
            LTVector vMid = { vFrom.x + (vTo.x - vFrom.x) * t, vFrom.y + (vTo.y - vFrom.y) * t,
                              vFrom.z + (vTo.z - vFrom.z) * t };
            WorldPoly* pHit = mr_IntersectNode(pNode->m_Sides[iSide], vFrom, vMid, nDepth + 1);
            if (pHit)
                return pHit;
            if (iSide == 1)
            {
                bool bHit = (pPoly->m_pSurface->m_Flags & SURF_SOLID) != 0;
                float cx = pPoly->m_Center.x - vMid.x, cy = pPoly->m_Center.y - vMid.y,
                      cz = pPoly->m_Center.z - vMid.z;
                if (bHit && pPoly->m_Radius * pPoly->m_Radius < cx * cx + cy * cy + cz * cz)
                    bHit = false;
                const WorldPolyVertex* pVerts = pPoly->m_Verts;
                const LTVector* pPrev = pPoly->m_nVerts ? pVerts[pPoly->m_nVerts - 1].m_pPos : NULL;
                for (uint32_t e = 0; bHit && e < pPoly->m_nVerts; e++)
                {
                    const LTVector* pV = pVerts[e].m_pPos;
                    LTVector ed = { pPrev->x - pV->x, pPrev->y - pV->y, pPrev->z - pV->z };
                    LTVector n = { ed.y * P.m_Normal.z - ed.z * P.m_Normal.y,
                                   ed.z * P.m_Normal.x - ed.x * P.m_Normal.z,
                                   ed.x * P.m_Normal.y - ed.y * P.m_Normal.x };
                    float len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
                    if (len > 0.0f)
                    {
                        n.x /= len; n.y /= len; n.z /= len;
                    }
                    float fEdge = n.x * pV->x + n.y * pV->y + n.z * pV->z;
                    float fMid = n.x * vMid.x + n.y * vMid.y + n.z * vMid.z;
                    if (fMid - fEdge < -MR_ISECT_EPS)
                        bHit = false;
                    pPrev = pV;
                }
                if (bHit)
                    return pPoly;
            }
            else if (t > 0.99f)
                return NULL;
            vFrom = vMid;
        }
        pNode = pNode->m_Sides[iSide == 0];
    }
    return NULL;
}

// Does the segment from vFrom along vRay first hit a sky portal?
bool mr_SeesSky(const LTVector& vFrom, const LTVector& vRay)
{
    MainWorld* pWorld = g_Frame.m_pContext ? g_Frame.m_pContext->m_pWorld : NULL;
    // The main world's BSP is the one the vis query walks (MainWorld
    // vtable slot 1); the first WorldData is another.
    WorldBsp* pMain = pWorld ? mw_GetVisBsp(pWorld) : NULL;
    if (!pMain || !pMain->m_RootNode)
        return false;
    LTVector vTo = { vFrom.x + vRay.x, vFrom.y + vRay.y, vFrom.z + vRay.z };
    WorldPoly* pHit = mr_IntersectNode(pMain->m_RootNode, vFrom, vTo, 0);
    return pHit && (pHit->m_pSurface->m_Flags & SURF_SKY);
}

// The world's size as the sun test reads it (MainWorld+0xb8, in its world
// tree); the extents' diagonal if that is not set.
float mr_WorldSize(void)
{
    MainWorld* pWorld = g_Frame.m_pContext ? g_Frame.m_pContext->m_pWorld : NULL;
    if (!pWorld)
        return 0.0f;
    float f;
    memcpy(&f, (const uint8_t*)pWorld + 0xb8, 4);
    if (f > 1.0f && f < 1.0e7f)
        return f;
    float dx = pWorld->m_ExtentsMax.x - pWorld->m_ExtentsMin.x, dy = pWorld->m_ExtentsMax.y - pWorld->m_ExtentsMin.y,
          dz = pWorld->m_ExtentsMax.z - pWorld->m_ExtentsMin.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// ---- world models (drawworldmodel.cpp) ------------------------------------------------

static uint32_t mr_FirstSurfaceFlags(WorldModelInstance* pWM)
{
    WorldBsp* pBsp = pWM->m_pWorldBsp;
    if (!pBsp || !pBsp->m_nPolies || !pBsp->m_Polies[0] || !pBsp->m_Polies[0]->m_pSurface)
        return 0;
    return pBsp->m_Polies[0]->m_pSurface->m_Flags;
}

// d3d_ProcessWorldModel: which set it goes in. (Primal Hunt's fog
// volumes are world models too: not drawn.)
void mr_ProcessWorldModel(LTObject* pObj)
{
    WorldModelInstance* pWM = (WorldModelInstance*)pObj;
    if ((pObj->m_Flags2 & FLAG2_FOGVOLUME) && mr_IsPrimalHunt())
    {
        mr_AddFogVolume(pObj);
        return;
    }
    if (!pWM->m_pWorldBsp || !pWM->m_pWorldBsp->m_nPolies)
        return;
    if (mr_FirstSurfaceFlags(pWM) & SURF_TRANSLUCENT)
        mr_AddTranslucent(pObj, mr_DrawTranslucentWorldModel);
    else if (pObj->m_Flags2 & FLAG2_CHROMAKEY)
        mr_ObjList(VS_WORLDMODELS_CHROMAKEY)->Add(pObj);
    else
        mr_ObjList(VS_WORLDMODELS)->Add(pObj);
}

static WorldPoly** mr_BspPolies(WorldBsp* pBsp, uint32_t* pn)
{
    if (bsp_Type(pBsp) == 1)
    {
        *pn = pBsp->m_nTerrainPolies;
        return pBsp->m_TerrainPolies;
    }
    *pn = pBsp->m_nPolies;
    return pBsp->m_Polies;
}

// d3d_DrawWorldModel: culled, every poly of the model's BSP that faces the
// camera, under the model's matrix, drawn as the world's.
void mr_DrawSolidWorldModel(LTObject* pObj)
{
    WorldModelInstance* pWM = (WorldModelInstance*)pObj;
    WorldBsp* pBsp = pWM->m_pWorldBsp;
    if (!pBsp || !cvi(CV_DrawWorldModels))
        return;
    float fRadius = bsp_BoundRadius(pBsp);
    if (!mr_SphereVisible(pObj->m_Pos, fRadius))
        return;
    LTVector vCam = mr_Xform(pWM->m_BackTransform, g_Frame.m_Cam.m_Pos);
    uint32_t n;
    WorldPoly** ppPolies = mr_BspPolies(pBsp, &n);
    uint16_t nCode = (uint16_t)(g_Frame.m_WorldFrameCode + 1);
    for (uint32_t i = 0; ppPolies && i < n; i++)
    {
        WorldPoly* pPoly = ppPolies[i];
        if (!pPoly || !pPoly->m_pSurface || pPoly->m_FrameCode == nCode)
            continue;
        pPoly->m_FrameCode = nCode;
        if (pPoly->m_pSurface->m_Flags & SURF_SKY)
            continue;
        mr_QueueFacingPoly(pPoly, vCam, 0.0001f);
    }
    g_nStatWM++;
    mr_ObjectFog(pObj, true);
    rd_PushMatrix();
    mr_MultMatrix(pWM->m_Transform);
    mr_FlushPolys(pObj->m_ColorA, &pWM->m_BackTransform);
    rd_PopMatrix();
    mr_ObjectFog(pObj, false);
}

// Back to front through the model's own BSP, with the camera in model
// space; each poly as a vertex-lit poly, tinted by the object's colour
// with the world-model ambient added (d3d_DrawTranslucentWorldModel).
static void mr_DrawBspBackToFront(Node* pNode, const LTVector& vCam, const float* pTint, uint8_t nAlpha,
                                  int nDepth)
{
    if (!pNode || (pNode->m_Flags & 3) || nDepth > 1024 || !pNode->m_pPoly)
        return;
    WorldPoly* pPoly = pNode->m_pPoly;
    const LTPlane* pPlane = pPoly->m_pPlane;
    if (!pPlane)
        return;
    int nSide = (pPlane->m_Normal.x * vCam.x + pPlane->m_Normal.y * vCam.y +
                 pPlane->m_Normal.z * vCam.z - pPlane->m_Dist) >= 0.0f;
    mr_DrawBspBackToFront(pNode->m_Sides[!nSide], vCam, pTint, nAlpha, nDepth + 1);
    if (pPoly->m_pSurface && !(pPoly->m_pSurface->m_Flags & SURF_INVISIBLE))
    {
        mr_RefreshVertLights(pPoly);
        MRTexture* pTex = mr_SurfaceTexture(pPoly->m_pSurface);
        mr_SetBaseTexture(pTex);
        rd_TexEnv(RD_ENV_MODULATE);
        uint32_t nVerts;
        const WorldPolyVertex* pVerts = mr_PolyVerts(pPoly, &nVerts);
        float fU = pTex ? pTex->m_fUMul : 0.0f, fV = pTex ? pTex->m_fVMul : 0.0f;
        const uint8_t (*G)[256] = g_Frame.m_GammaRamp;
        const LTVector& amb = g_Frame.m_pScene->m_WMAmbient;
        rd_Begin(RD_TRIANGLE_FAN);
        for (uint32_t v = 0; v < nVerts; v++)
        {
            const LTVector& pos = *pVerts[v].m_pPos;
            int c[3] = { (int)(G[0][pVerts[v].m_R] * pTint[0] + amb.x), (int)(G[1][pVerts[v].m_G] * pTint[1] + amb.y),
                         (int)(G[2][pVerts[v].m_B] * pTint[2] + amb.z) };
            rd_Color4ub((uint8_t)mr_Clamp(c[0], 0, 255), (uint8_t)mr_Clamp(c[1], 0, 255),
                        (uint8_t)mr_Clamp(c[2], 0, 255), nAlpha);
            rd_TexCoord2f(pVerts[v].m_U * fU, pVerts[v].m_V * fV);
            mr_Fog(pos);
            rd_Vertex3f(pos.x, pos.y, pos.z);
        }
        rd_End();
    }
    mr_DrawBspBackToFront(pNode->m_Sides[nSide], vCam, pTint, nAlpha, nDepth + 1);
}

// A translucent world model: blended by its alpha, or added (ONE/ONE,
// fog towards black) when its first surface says so.
void mr_DrawTranslucentWorldModel(LTObject* pObj)
{
    WorldModelInstance* pWM = (WorldModelInstance*)pObj;
    WorldBsp* pBsp = pWM->m_pWorldBsp;
    if (!pBsp || !cvi(CV_DrawWorldModels))
        return;
    bool bAdditive = (mr_FirstSurfaceFlags(pWM) & SURF_ADDITIVE) != 0;
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_FOG);
    if (bAdditive)
    {
        rd_BlendFunc(RD_ONE, RD_ONE);
        rd_FogColor(0, 0, 0);
    }
    float tint[3] = { pObj->m_ColorR / 255.0f, pObj->m_ColorG / 255.0f, pObj->m_ColorB / 255.0f };
    mr_ObjectFog(pObj, true);
    rd_PushMatrix();
    mr_MultMatrix(pWM->m_Transform);
    LTVector vCam = mr_Xform(pWM->m_BackTransform, g_Frame.m_Cam.m_Pos);
    mr_SetFogEye(vCam);
    mr_ForgetBindings();
    mr_DrawBspBackToFront(pBsp->m_RootNode, vCam, tint, pObj->m_ColorA, 0);
    mr_SetBaseTexture(NULL);
    mr_ForgetBindings();
    mr_SetFogEye(g_Frame.m_Cam.m_Pos);
    rd_PopMatrix();
    mr_ObjectFog(pObj, false);
    rd_PopAttrib();
}

// ---- the sky ----------------------------------------------------------------------

// A sky world model: its BSP as authored, front-facing polys only, node
// poly before its far side (d3d_skyobjects.cpp).
static void mr_DrawSkyBsp(Node* pNode, const LTVector& vCam, uint8_t nAlpha, int nDepth)
{
    if (!pNode || (pNode->m_Flags & 3) || nDepth > 1024 || !pNode->m_pPoly)
        return;
    WorldPoly* pPoly = pNode->m_pPoly;
    const LTPlane* pPlane = pPoly->m_pPlane;
    if (!pPlane)
        return;
    int bFront = (pPlane->m_Normal.x * vCam.x + pPlane->m_Normal.y * vCam.y +
                  pPlane->m_Normal.z * vCam.z - pPlane->m_Dist) > 0.0f;
    if (bFront && pPoly->m_pSurface && !(pPoly->m_pSurface->m_Flags & SURF_INVISIBLE))
    {
        mr_RefreshVertLights(pPoly);
        MRTexture* pTex = mr_SurfaceTexture(pPoly->m_pSurface);
        mr_DrawGouraudPoly(pPoly, pTex, nAlpha);
    }
    mr_DrawSkyBsp(pNode->m_Sides[!bFront], vCam, nAlpha, nDepth + 1);
    mr_DrawSkyBsp(pNode->m_Sides[bFront], vCam, nAlpha, nDepth + 1);
}

// d3d_DrawSky: the sky objects in their list order, depth off, seen with
// the main camera's rotation from the camera's place squashed into the
// sky box (d3d_SetupSkyStuff), near 0.01, scale SkyScale.
static void mr_DrawSky(SceneDesc* pScene, MainWorld* pWorld)
{
    if (!cvi(CV_DrawSky) || pScene->m_nSkyObjects <= 0 || !g_Frame.m_nSkyPolys)
        return;
    MRCamera sky = g_Frame.m_Cam;
    LTVector t = { 0.5f, 0.5f, 0.5f };
    LTVector ext = { pWorld->m_ExtentsMax.x - pWorld->m_ExtentsMin.x,
                     pWorld->m_ExtentsMax.y - pWorld->m_ExtentsMin.y,
                     pWorld->m_ExtentsMax.z - pWorld->m_ExtentsMin.z };
    if (ext.x != 0.0f && ext.y != 0.0f && ext.z != 0.0f)
    {
        t.x = (sky.m_Pos.x - pWorld->m_ExtentsMin.x) / ext.x;
        t.y = (sky.m_Pos.y - pWorld->m_ExtentsMin.y) / ext.y;
        t.z = (sky.m_Pos.z - pWorld->m_ExtentsMin.z) / ext.z;
    }
    const SkyDef& def = pScene->m_SkyDef;
    sky.m_Pos.x = def.m_ViewMin.x + (def.m_ViewMax.x - def.m_ViewMin.x) * t.x;
    sky.m_Pos.y = def.m_ViewMin.y + (def.m_ViewMax.y - def.m_ViewMin.y) * t.y;
    sky.m_Pos.z = def.m_ViewMin.z + (def.m_ViewMax.z - def.m_ViewMin.z) * t.z;
    sky.m_fNear = 0.01f;
    sky.m_fScale = cvf(CV_SkyScale) > 0.0f ? cvf(CV_SkyScale) : 1.0f;
    mr_LoadCamera(&sky);
    g_Frame.m_SkyCam = sky;

    rd_PushAttrib(RD_ATTR_ALL);
    rd_Disable(RD_DEPTH_TEST);
    rd_DepthMask(false);
    rd_Disable(RD_ALPHA_TEST);
    bool bFog = g_Frame.m_bFog;
    g_Frame.m_bFog = false;
    rd_Disable(RD_FOG);
    LTVector vSkyCam = sky.m_Pos;
    for (int i = 0; i < pScene->m_nSkyObjects; i++)
    {
        LTObject* pObj = pScene->m_SkyObjects[i];
        if (!pObj || !(pObj->m_Flags & FLAG_VISIBLE))
            continue;
        if (pObj->m_ObjectType == OT_WORLDMODEL)
        {
            WorldModelInstance* pWM = (WorldModelInstance*)pObj;
            if (!pWM->m_pWorldBsp)
                continue;
            uint32_t nFlags = mr_FirstSurfaceFlags(pWM);
            if (nFlags & SURF_TRANSLUCENT)
            {
                rd_Enable(RD_BLEND);
                if (nFlags & SURF_ADDITIVE)
                    rd_BlendFunc(RD_ONE, RD_ONE);
                else
                    rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);
            }
            else
                rd_Disable(RD_BLEND);
            mr_ForgetBindings();
            // The lights that reach it light every vertex drawn (d3d.ren's
            // d3d_LightSkyPoly lit only the first m_nVerts of the
            // T-junction list: RETAIL_BUGS.md, fixed here).
            mr_SetupWorldLights(&pWM->m_BackTransform);
            mr_DrawSkyBsp(pWM->m_pWorldBsp->m_RootNode, vSkyCam, pObj->m_ColorA, 0);
            g_nWLights = 0;
            mr_SetBaseTexture(NULL);
        }
        else
            mr_DrawSkyObject(pObj);
    }
    g_Frame.m_bFog = bFog;
    rd_PopAttrib();
    mr_ForgetBindings();
    mr_LoadCamera(&g_Frame.m_Cam);
}

// ---- screen passes (d3d_DrawLightScalePoly, d3d_DrawLightAddPoly) ------------------------

static void mr_ScreenQuad(void)
{
    rd_MatrixMode(RD_PROJECTION);
    rd_PushMatrix();
    rd_LoadIdentity();
    rd_MatrixMode(RD_MODELVIEW);
    rd_PushMatrix();
    rd_LoadIdentity();
    rd_Begin(RD_QUADS);
    rd_Vertex2f(-1, -1); rd_Vertex2f(1, -1); rd_Vertex2f(1, 1); rd_Vertex2f(-1, 1);
    rd_End();
    rd_PopMatrix();
    rd_MatrixMode(RD_PROJECTION);
    rd_PopMatrix();
    rd_MatrixMode(RD_MODELVIEW);
}

// The light scale: colour = scale x 127.5, blended DESTCOLOR/SRCCOLOR
// (2 x src x dst); InvertHack: colour = scale x 255, INVDESTCOLOR/ZERO.
// Left out when every component is within 0.99..1.01 (and no
// InvertHack). The light add: ONE/ONE, colour = add x 255, left out below
// 0.001. The scale first.
static void mr_ScreenPasses(SceneDesc* pScene)
{
    const LTVector& s = pScene->m_GlobalLightScale;
    const LTVector& a = pScene->m_GlobalLightAdd;
    bool bInvert = cvi(CV_InvertHack) != 0;
    bool bScale = bInvert || !(s.x > 0.99f && s.y > 0.99f && s.z > 0.99f &&
                               s.x < 1.01f && s.y < 1.01f && s.z < 1.01f);
    bool bAdd = cvi(CV_LightAddPoly) && !(a.x < 0.001f && a.y < 0.001f && a.z < 0.001f);
    if (!bScale && !bAdd)
        return;
    rd_PushAttrib(RD_ATTR_ALL);
    rd_Disable(RD_TEXTURE_2D);
    rd_Disable(RD_FOG);
    rd_Disable(RD_DEPTH_TEST);
    rd_Disable(RD_ALPHA_TEST);
    rd_DepthMask(false);
    rd_Enable(RD_BLEND);
    if (bScale)
    {
        float k = bInvert ? 255.0f : 127.5f;
        if (bInvert)
            rd_BlendFunc(RD_ONE_MINUS_DST_COLOR, RD_ZERO);
        else
            rd_BlendFunc(RD_DST_COLOR, RD_SRC_COLOR);
        rd_Color4ub((uint8_t)(int)(s.x * k), (uint8_t)(int)(s.y * k), (uint8_t)(int)(s.z * k), 255);
        mr_ScreenQuad();
    }
    if (bAdd)
    {
        rd_BlendFunc(RD_ONE, RD_ONE);
        rd_Color4ub((uint8_t)(int)(a.x * 255.0f), (uint8_t)(int)(a.y * 255.0f), (uint8_t)(int)(a.z * 255.0f), 255);
        mr_ScreenQuad();
    }
    rd_PopAttrib();
}

// ---- RenderScene ---------------------------------------------------------------------

// The x87 FPU's precision to 24 bits (single), as d3d.ren's d3d_InitFrame
// sets it at the start of every scene: the engine's physics runs on the
// same thread and was only ever run at that precision (nolfsrc's issue
// #43: at double precision its borderline tests came out the other way).
static void mr_SetFPUPrecision24()
{
#if defined(__i386__)
    unsigned short nCW;
    __asm__ __volatile__("fnstcw %0" : "=m"(nCW));
    nCW &= 0xfcff;
    __asm__ __volatile__("fldcw %0" : : "m"(nCW));
#endif
}

// A colour ramp (d3d_RenderScene's BUILD_RAMP): 256 bytes of a 16.16
// accumulator stepped by fScale, clamped at 255.
static void mr_BuildRamp(uint8_t* pRamp, float fScale)
{
    uint32_t nStep = (uint32_t)(fScale * 65536.0f), nAccum = 0;
    for (int i = 0; i < 256; i++)
    {
        pRamp[i] = (uint8_t)(nAccum >> 16);
        nAccum += nStep;
        if (nAccum > (255u << 16))
            nAccum = 255u << 16;
    }
}

static void mr_DrawFrame(SceneDesc* pScene, MRContext* pContext, MainWorld* pWorld);
static void mr_DrawPortals(SceneDesc* pScene, MRContext* pContext, MainWorld* pWorld);

LTBOOL mr_RenderScene(SceneDesc* pScene)
{
    mr_SetFPUPrecision24();
    mr_SyncScreen(false);
    if (!g.bContext || !pScene || !g.bIn3D)
        return 0;
    if (g.bInOptimized2D)
    {
        mr_Print("Error: tried to render 3D while in optimized 2D mode.");
        return 0;
    }
    mr_ReadConsoleVariables();

    MRContext* pContext = pScene->m_DrawMode == DRAWMODE_NORMAL ? (MRContext*)pScene->m_pRenderContext : NULL;
    if (pScene->m_DrawMode == DRAWMODE_NORMAL && (!pContext || !pContext->m_pWorld))
        return 0;
    MainWorld* pWorld = pContext ? pContext->m_pWorld : NULL;

    memset(&g_Frame, 0, sizeof(g_Frame));
    g_Frame.m_pScene = pScene;
    g_Frame.m_pContext = pContext;
    mr_SetupCamera(pScene, &g_Frame.m_Cam);
    mr_SetupFrustum(&g_Frame.m_Cam);

    // The world colour and its ramps (d3d_InitFrame, d3d_RenderScene).
    const LTVector& wc = pScene->m_WorldColor;
    g_Frame.m_WorldColor[0] = (uint8_t)mr_Clamp((int)(wc.x * 255.0f + 0.5f), 0, 255);
    g_Frame.m_WorldColor[1] = (uint8_t)mr_Clamp((int)(wc.y * 255.0f + 0.5f), 0, 255);
    g_Frame.m_WorldColor[2] = (uint8_t)mr_Clamp((int)(wc.z * 255.0f + 0.5f), 0, 255);
    float fSat = cvi(CV_Saturate) ? 2.0f : 1.0f;
    mr_BuildRamp(g_Frame.m_GammaRamp[0], wc.x * fSat);
    mr_BuildRamp(g_Frame.m_GammaRamp[1], wc.y * fSat);
    mr_BuildRamp(g_Frame.m_GammaRamp[2], wc.z * fSat);
    mr_BuildRamp(g_Frame.m_LightRamp[0], wc.x);
    mr_BuildRamp(g_Frame.m_LightRamp[1], wc.y);
    mr_BuildRamp(g_Frame.m_LightRamp[2], wc.z);

    if (pContext)
    {
        mr_IncrementFrameCode(pContext);
        g_Frame.m_WorldFrameCode = pContext->m_FrameCode;
        g_Frame.m_fLMGrid = pWorld->m_LMGridSize > 0.0f ? pWorld->m_LMGridSize : 1.0f;
        g_Frame.m_bLightmaps = cvi(CV_LightMap) && pContext->m_pAtlases &&
                               pContext->m_nContextGen == g.nContextGen;
    }
    // The object frame code stamps LTObject+0x58: the engine's visibility
    // walk and the attachment pass process each object once a frame.
    g_Frame.m_ObjectFrameCode = g.pStruct->IncObjectFrameCode();
    if (g.pStruct->IncCurTextureFrameCode)
        g.pStruct->IncCurTextureFrameCode();

    rd_PushAttrib(RD_ATTR_ALL);
    rd_MatrixMode(RD_TEXTURE);
    rd_PushMatrix();
    rd_LoadIdentity();
    rd_MatrixMode(RD_PROJECTION);
    rd_PushMatrix();
    rd_MatrixMode(RD_MODELVIEW);
    rd_PushMatrix();

    mr_DrawFrame(pScene, pContext, pWorld);

    rd_MatrixMode(RD_MODELVIEW);
    rd_PopMatrix();
    rd_MatrixMode(RD_PROJECTION);
    rd_PopMatrix();
    rd_MatrixMode(RD_TEXTURE);
    rd_PopMatrix();
    rd_MatrixMode(RD_MODELVIEW);
    rd_PopAttrib();

    // The screen passes cover the camera's rect.
    rd_PushAttrib(RD_ATTR_VIEWPORT);
    rd_Viewport(g_Frame.m_Cam.m_nViewX, g_Frame.m_Cam.m_nViewY, g_Frame.m_Cam.m_nViewW, g_Frame.m_Cam.m_nViewH);
    mr_ScreenPasses(pScene);
    rd_PopAttrib();
    mr_ForgetBindings();
    return 0;
}

// What one view of the scene draws: the visible set, the sky, the world,
// the objects, the mirrors.
static void mr_DrawFrame(SceneDesc* pScene, MRContext* pContext, MainWorld* pWorld)
{
    mr_BeginObjects();
    if (g_Frame.m_nPortalDepth == 0)
    {
        g_nFogBoxes = 0;
        g_bFogBoxColor = false;
    }
    g_bPortals = pContext && cvi(CV_DrawPortals) && g_Frame.m_nPortalDepth == 0 && g_RDCaps.bStencil;
    g_nPortals = 0;
    g_Frame.m_nSkyPolys = 0;
    mr_SetFogEye(g_Frame.m_Cam.m_Pos);

    // What is visible.
    if (pContext)
    {
        if ((pWorld->m_Flags & MAINWORLD_HASVIS) && !cvi(CV_DrawAll))
        {
            VisQueryRequest req;
            memset(&req, 0, sizeof(req));
            req.m_Viewpoint = g_Frame.m_Cam.m_Pos;
            req.m_ViewRadius = 10000.0f;
            req.m_ObjectCB = cb_Object;
            req.m_AddObjects = cb_AddObjects;
            req.m_LeafCB = cb_Leaf;
            req.m_BoxFilter = cb_BoxFilter;
            req.m_NodeFilter = cb_NodeFilter;
            mw_DoVisQuery(pWorld, &req);
        }
        else
            mr_VisitAll(pWorld);
    }
    else
    {
        for (int i = 0; i < pScene->m_ObjectListSize; i++)
        {
            LTObject* pObj = pScene->m_pObjectList ? pScene->m_pObjectList[i] : NULL;
            if (pObj && pObj->m_ObjectType > OT_NORMAL && pObj->m_ObjectType < NUM_OBJECTTYPES)
                mr_QueueObject(pObj);
        }
    }

    mr_LoadCamera(&g_Frame.m_Cam);
    rd_CullBack(false);
    rd_Disable(RD_BLEND);
    rd_Disable(RD_ALPHA_TEST);
    rd_AlphaGreater(0.1666f);
    rd_TexEnv(RD_ENV_MODULATE);
    mr_ForgetBindings();

    if (pContext)
        mr_DrawSky(pScene, pWorld);

    rd_Enable(RD_DEPTH_TEST);
    rd_DepthFunc(RD_LEQUAL);
    rd_DepthMask(true);
    mr_SetFog();
    if (pContext)
    {
        if (g_Frame.m_bLightmaps && g_Frame.m_nPortalDepth == 0)
            mr_UpdateLightmaps(pContext);
        mr_FlushPolys(255, NULL);
    }
    mr_FlushObjects();
    if (g_Frame.m_nPortalDepth == 0 && ++g_nStatFrames == 100)
    {
        mr_Debug(3, "100 frames: lightmapped %u, pan sky %u, gouraud %u, flat %u polys; %u lights, %u world models; sky polys %u; LM flag %u page %u; bLM %d world flags %x",
                 g_nStatPolys[0], g_nStatPolys[1], g_nStatPolys[2], g_nStatPolys[3], g_nStatLights, g_nStatWM,
                 g_Frame.m_nSkyPolys, g_nStatLMFlag, g_nStatLMPage, g_Frame.m_bLightmaps, pWorld ? pWorld->m_Flags : 0);
        g_nStatLMFlag = g_nStatLMPage = 0;
        memset(g_nStatPolys, 0, sizeof(g_nStatPolys));
        g_nStatLights = g_nStatWM = g_nStatFrames = 0;
    }
    if (pContext && g_nPortals)
        mr_DrawPortals(pScene, pContext, pWorld);
}

// The screen box of a poly seen from the camera (pixels, the scissor's
// x, y, w, h); false when it is not on the screen. A poly that crosses
// the near plane takes the whole view.
static bool mr_ScreenBox(const MRCamera& cam, const WorldPoly* pPoly, int* pBox)
{
    float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
    uint32_t nVerts;
    const WorldPolyVertex* pVerts = mr_PolyVerts(pPoly, &nVerts);
    bool bAll = false;
    for (uint32_t v = 0; v < nVerts && !bAll; v++)
    {
        const LTVector& p = *pVerts[v].m_pPos;
        float d[3] = { p.x - cam.m_Pos.x, p.y - cam.m_Pos.y, p.z - cam.m_Pos.z };
        float c[3];
        for (int k = 0; k < 3; k++)
            c[k] = cam.m_R[0][k] * d[0] + cam.m_R[1][k] * d[1] + cam.m_R[2][k] * d[2];
        if (c[2] < cam.m_fNear)
        {
            bAll = true;
            break;
        }
        float sx = cam.m_nViewX + (c[0] / (c[2] * cam.m_fTanX) + 1.0f) * 0.5f * cam.m_nViewW;
        float sy = cam.m_nViewY + (1.0f - c[1] / (c[2] * cam.m_fTanY)) * 0.5f * cam.m_nViewH;
        x0 = sx < x0 ? sx : x0;
        x1 = sx > x1 ? sx : x1;
        y0 = sy < y0 ? sy : y0;
        y1 = sy > y1 ? sy : y1;
    }
    int vx0 = cam.m_nViewX, vy0 = cam.m_nViewY, vx1 = vx0 + cam.m_nViewW, vy1 = vy0 + cam.m_nViewH;
    int bx0 = bAll ? vx0 : mr_Clamp((int)floorf(x0), vx0, vx1);
    int by0 = bAll ? vy0 : mr_Clamp((int)floorf(y0), vy0, vy1);
    int bx1 = bAll ? vx1 : mr_Clamp((int)ceilf(x1), vx0, vx1);
    int by1 = bAll ? vy1 : mr_Clamp((int)ceilf(y1), vy0, vy1);
    if (bx1 <= bx0 || by1 <= by0)
        return false;
    pBox[0] = bx0;
    pBox[1] = by0;
    pBox[2] = bx1 - bx0;
    pBox[3] = by1 - by0;
    return true;
}

// The mirrors (d3d_DrawPortals): for each visible mirror poly (surface
// portal id 0x7ffe), seen from its front: the poly marks the stencil and
// pushes the depth to the far plane; the scene is drawn again from the
// camera reflected in its plane, inside the poly's screen box and where
// the stencil is marked (PortalLightmap 0: without lightmaps); the poly
// is drawn over it when its surface asks (0x8000). A view through a
// mirror draws no mirrors.
static void mr_DrawPortals(SceneDesc* pScene, MRContext* pContext, MainWorld* pWorld)
{
    WorldPoly* portals[MR_MAXPORTALS];
    uint32_t nPortals = g_nPortals;
    memcpy(portals, g_Portals, nPortals * sizeof(WorldPoly*));
    g_nPortals = 0;
    MRCamera cam = g_Frame.m_Cam;
    for (uint32_t i = 0; i < nPortals; i++)
    {
        WorldPoly* pPoly = portals[i];
        if ((pPoly->m_pSurface->m_PortalId & 0x7fff) != PORTAL_MIRROR)
            continue;
        const LTPlane& P = *pPoly->m_pPlane;
        const LTVector& N = P.m_Normal;
        if (N.x * cam.m_Pos.x + N.y * cam.m_Pos.y + N.z * cam.m_Pos.z - P.m_Dist < 0.0f)
            continue;
        int box[4];
        if (!mr_ScreenBox(cam, pPoly, box))
            continue;

        // The poly: the stencil marked, the depth at the far plane.
        uint32_t nVerts;
        const WorldPolyVertex* pVerts = mr_PolyVerts(pPoly, &nVerts);
        rd_PushAttrib(RD_ATTR_ALL);
        rd_Disable(RD_TEXTURE_2D);
        rd_Disable(RD_BLEND);
        rd_Disable(RD_FOG);
        rd_ColorMask(false);
        rd_DepthMask(true);
        rd_DepthFunc(RD_ALWAYS);
        rd_StencilMode(RD_STENCIL_WRITE1);
        rd_DepthRange(1.0f, 1.0f);
        rd_Begin(RD_TRIANGLE_FAN);
        for (uint32_t v = 0; v < nVerts; v++)
            rd_Vertex3fv(&pVerts[v].m_pPos->x);
        rd_End();
        rd_PopAttrib();

        // The scene from the reflected camera.
        MRFrame saved = g_Frame;
        MRCamera& mc = g_Frame.m_Cam;
        float d = 2.0f * (N.x * cam.m_Pos.x + N.y * cam.m_Pos.y + N.z * cam.m_Pos.z - P.m_Dist);
        mc.m_Pos.x = cam.m_Pos.x - N.x * d;
        mc.m_Pos.y = cam.m_Pos.y - N.y * d;
        mc.m_Pos.z = cam.m_Pos.z - N.z * d;
        for (int c = 0; c < 3; c++)
        {
            float a[3] = { cam.m_R[0][c], cam.m_R[1][c], cam.m_R[2][c] };
            float k = 2.0f * (a[0] * N.x + a[1] * N.y + a[2] * N.z);
            mc.m_R[0][c] = a[0] - N.x * k;
            mc.m_R[1][c] = a[1] - N.y * k;
            mc.m_R[2][c] = a[2] - N.z * k;
        }
        mc.m_bMirror = !cam.m_bMirror;
        mr_SetupFrustum(&mc);
        g_Frame.m_nPortalDepth++;
        if (!cvi(CV_PortalLightmap))
            g_Frame.m_bLightmaps = false;
        mr_IncrementFrameCode(pContext);
        g_Frame.m_WorldFrameCode = pContext->m_FrameCode;
        g_Frame.m_ObjectFrameCode = g.pStruct->IncObjectFrameCode();
        rd_PushAttrib(RD_ATTR_ALL);
        rd_Enable(RD_SCISSOR_TEST);
        rd_Scissor(box[0], box[1], box[2], box[3]);
        rd_StencilMode(RD_STENCIL_TEST1);
        mr_DrawFrame(pScene, pContext, pWorld);
        rd_PopAttrib();
        g_Frame = saved;
        mr_LoadCamera(&g_Frame.m_Cam);
        mr_SetFogEye(g_Frame.m_Cam.m_Pos);
        mr_ForgetBindings();

        // The poly's own texture over it.
        if (pPoly->m_pSurface->m_PortalId & 0x8000)
        {
            MRTexture* pTex = mr_SurfaceTexture(pPoly->m_pSurface);
            if (pTex)
            {
                rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_DEPTH);
                rd_Enable(RD_BLEND);
                rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);
                rd_DepthFunc(RD_ALWAYS);
                rd_DepthMask(false);
                mr_SetBaseTexture(pTex);
                rd_TexEnv(RD_ENV_MODULATE);
                static const uint8_t s_White[4] = { 255, 255, 255, 255 };
                mr_DrawTexturePass(pPoly, pTex, NULL, NULL, s_White, false);
                mr_SetBaseTexture(NULL);
                rd_PopAttrib();
            }
        }
    }
}
