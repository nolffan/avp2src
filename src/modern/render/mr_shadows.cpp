// mr_shadows.cpp -- model shadows, as d3d.ren draws them
// (drawmodelshadows.cpp: CD3DModelDraw::DrawShadows 0x1002611f,
// DrawShadowPoly 0x10025ed1, DrawProjShadows 0x10027e40,
// DrawProjShadowPoly 0x10027b95, ShadowProjection::SetupPlanar 0x10026ddd;
// drawlightmaps.cpp's d3d_GetSegmentPolies 0x100366da, which gathers the
// polys; geomroutines.cpp's gr_GetPerpendicularVector).
//
// A model whose Model has shadows (Model+0xd4) and whose hook flags have
// FLAG_SHADOW casts up to MaxModelShadows shadows (at most 8).
//
// ModelShadowProj 0, the planar shadow: for each shadow direction (the
// first straight down; see shd_DrawPlanar for the others), the world polys
// along the direction are gathered. Each poly is lifted off its plane by
// ModelShadowOffset, clipped to the shadow's frustum and drawn black with
// the falloff texture (a soft disc) projected on it from a point
// ShadowStart above the model, at ModelShadowAlpha.
//
// ModelShadowProj 1, the projected shadow: the lights that light the
// model (as the model's lighting selects them), the nearest first, each
// cast one shadow. d3d.ren draws the model's triangles, flattened onto the
// plane across the light's direction, black on white into a square of the
// back buffer (ModelShadowProjRes texels; LOD ModelShadowProjLOD more than
// the model's), copies that into its LM buffer texture, and projects the
// texture from the light onto the world polys around the model, fading
// with the distance from the light. ModelShadowProjShow shows the texture
// on the screen.
//
// Where this file differs from d3d.ren:
//  * The device cannot draw into a texture, so the shadow image is made on
//    the CPU: each triangle is filled black into a white buffer of 32-bit
//    texels (pixel centres at whole coordinates, as Direct3D samples), and
//    the buffer goes to the device with rd_TexImage. The texture keeps the
//    image in its colour and 255 minus it in its alpha. The draw then uses
//    the colour (0,0,0,a) and the source-alpha blend, which gives
//    dst x (1 - a + a x image), d3d.ren's BLENDDIFFUSEALPHA and
//    ZERO/SRCCOLOR blend. d3d.ren draws the triangles with the cull mode
//    the device has; a flattened closed mesh covers the same area from
//    each side, so here no triangle is culled.
//  * d3d.ren transforms and clips each shadow poly to the view on the CPU
//    and draws it as screen-space vertices. Here the world-space poly goes
//    to the device with the camera loaded, which clips it. As d3d.ren
//    does, u/q and v/q are divided at each vertex before the draw.
//  * A vertex whose q is 0 gets the coordinates (0,0) (d3d.ren divides by
//    0). A shadow whose flattened model has no width or no height, or a
//    model with no size, is not drawn (d3d.ren scales by an infinity).
//  * d3d.ren clips with a static flag array of 50 entries; a poly with
//    more vertices writes past it. Here the arrays hold every vertex.
//  * The polys are gathered as d3d.ren does: the world tree's slot 3 (the
//    objects filed at the model's position, the engine's
//    FindObjectsInBoxQ(pos, pos)) to d3d_GetSegmentPolies, which walks each
//    visible world model's BSP along the segment (in world space, so a
//    moved world model's polys are tested where they were; their frame
//    code is never the world's, so they are never taken). A node without a
//    poly plane stops the walk here (d3d.ren reads through NULL).
//  * The renderer's state: d3d.ren keeps the blend and fog states the
//    device has for the planar shadow (fog factor 255: none). Here the
//    blend is SRC_ALPHA/ONE_MINUS_SRC_ALPHA, the fog factor 1 (none), and
//    the projected shadow turns fog off as d3d.ren does. Every state this
//    file changes is put back; d3d.ren leaves texture stages 1..7 off
//    after the projected shadow.

#include <math.h>

#include "mrobjects.h"

template <typename T> static inline T rd(const void* p, uint32_t off) { T v; memcpy(&v, (const uint8_t*)p + off, sizeof(T)); return v; }

// ---- the engine's data ---------------------------------------------------------------

// ModelInstance::m_HiddenPieces (drawmodelshadows.cpp's ShadowModelInstance).
#ifdef PRIMAL_HUNT
#define MI_HIDDEN     0x260
#else
#define MI_HIDDEN     0x24c
#endif

// Model (drawmodelshadows.cpp's ShadowModel, drawmodel.cpp's Model).
#define MD_PIECES       0x38            // ModelPiece**
#define MD_NPIECES      0x3c
#define MD_LODDISTS     0x90            // float*: LODs 1..
#define MD_NLODS        0x94            // the LODs after the first
#define MD_LODDIST0     0xa0
#define MD_VISRADIUS    0xa8
#define MD_SHADOW       0xd4            // casts a shadow
#define MD_SHADOWLENGTH 0xd8
#define MD_SHADOWSTART  0xdc
#define MD_SHADOWSIZEX  0xe0
#define MD_SHADOWSIZEY  0xe4
#define MD_SHADOWOFFSET 0xe8            // LTVector
#define MD_SOCKETNODE   0xf4
#define MD_SOCKETMAT    0xfc            // LTMatrix
#define MD_SOCKETLIT    0x13c

namespace {

struct ShdWeight { float m_Vec[4]; uint32_t m_iNode; };   // position x weight, weight
struct ShdVert {                        // ModelVert, 0x20
    ShdWeight* m_Weights;
    uint16_t   m_nWeights;
    uint16_t   m_iReplacement;          // in the next LOD
    LTVector   m_Vec;
    LTVector   m_vNormal;
};
struct ShdTri { uint16_t m_Indices[3]; uint16_t m_pad; float m_UV[3][2]; };   // ModelTri, 0x20
struct ShdVertSet {                     // ModelVertSet, 0x24: a piece's LOD
    uint32_t  m_00;
    ShdVert*  m_pVerts;                 // 0x04
    uint32_t  m_nVerts;                 // 0x08
    uint32_t  m_0c, m_10;
    ShdTri*   m_pTris;                  // 0x14
    uint32_t  m_nTris;                  // 0x18
    uint32_t  m_1c, m_20;
};
static_assert(sizeof(ShdVertSet) == 0x24, "ShdVertSet");
struct ShdPiece : ShdVertSet {          // ModelPiece
    uint8_t     m_pad24[0x38 - 0x24];
    ShdVertSet* m_LODs;                 // 0x38 LODs 1..
    uint32_t    m_nLODs;                // 0x3c
};

}

// ModelPiece::GetLOD.
static ShdVertSet* shd_PieceLOD(ShdPiece* p, uint32_t i)
{
    if (i == 0)
        return p;
    i--;
    return i < p->m_nLODs ? &p->m_LODs[i] : NULL;
}

static inline uint32_t md_nPieces(const void* m) { return rd<uint32_t>(m, MD_NPIECES); }
static inline ShdPiece* md_Piece(const void* m, uint32_t i) { return rd<ShdPiece**>(m, MD_PIECES)[i]; }
static inline uint32_t md_nLODs(const void* m) { return rd<uint32_t>(m, MD_NLODS); }
static float md_LODDist(const void* m, uint32_t i)
{
    if (i == 0)
        return rd<float>(m, MD_LODDIST0);
    return i - 1 < md_nLODs(m) ? rd<float*>(m, MD_LODDISTS)[i - 1] : 1.0e30f;
}

// ---- small vector and matrix maths ----------------------------------------------------

static inline LTVector v3(float x, float y, float z) { LTVector v = { x, y, z }; return v; }
static inline LTVector vadd(const LTVector& a, const LTVector& b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline LTVector vsub(const LTVector& a, const LTVector& b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline LTVector vmul(const LTVector& a, float f) { return v3(a.x * f, a.y * f, a.z * f); }
static inline LTVector vneg(const LTVector& a) { return v3(-a.x, -a.y, -a.z); }
static inline float vdot(const LTVector& a, const LTVector& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float vmag(const LTVector& a) { return sqrtf(vdot(a, a)); }
static inline LTVector vcross(const LTVector& a, const LTVector& b)
{
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
// LTVector::Norm; a zero vector stays zero (LithTech's gives NaNs).
static inline LTVector vnorm(const LTVector& a)
{
    float m = vmag(a);
    return m != 0.0f ? vmul(a, 1.0f / m) : a;
}
static inline float pdist(const LTPlane& p, const LTVector& v) { return vdot(p.m_Normal, v) - p.m_Dist; }
static inline LTVector vxform(const LTMatrix& M, const LTVector& v)
{
    return v3(M.m[0][0] * v.x + M.m[0][1] * v.y + M.m[0][2] * v.z + M.m[0][3],
              M.m[1][0] * v.x + M.m[1][1] * v.y + M.m[1][2] * v.z + M.m[1][3],
              M.m[2][0] * v.x + M.m[2][1] * v.y + M.m[2][2] * v.z + M.m[2][3]);
}

static void mat_Mul(LTMatrix* pOut, const LTMatrix& a, const LTMatrix& b)
{
    LTMatrix r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
    *pOut = r;
}

static void mat_Init(LTMatrix* M, float a, float b, float c, float d, float e, float f, float g, float h)
{
    M->m[0][0] = a; M->m[0][1] = b; M->m[0][2] = c; M->m[0][3] = d;
    M->m[1][0] = e; M->m[1][1] = f; M->m[1][2] = g; M->m[1][3] = h;
    M->m[2][0] = 0.0f; M->m[2][1] = 0.0f; M->m[2][2] = 1.0f; M->m[2][3] = 0.0f;
    M->m[3][0] = 0.0f; M->m[3][1] = 0.0f; M->m[3][2] = 0.0f; M->m[3][3] = 1.0f;
}

// The inverse of an affine matrix (mr_models.cpp's).
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

// ShadowProjection::SetupPlanar (0x10026ddd): the projection from the
// point vLight onto the plane.
static void shd_SetupPlanar(LTMatrix* M, const LTVector& vLight, const LTPlane& plane)
{
    const LTVector& n = plane.m_Normal;
    float d = plane.m_Dist;
    float fDot = vLight.x * n.x + (vLight.y * n.y + vLight.z * n.z) - d;

    M->m[0][0] = fDot - vLight.x * n.x;
    M->m[0][1] = -(n.y * vLight.x);
    M->m[0][2] = -(n.z * vLight.x);
    M->m[0][3] = d * vLight.x;

    M->m[1][0] = -(n.x * vLight.y);
    M->m[1][1] = fDot - vLight.y * n.y;
    M->m[1][2] = -(n.z * vLight.y);
    M->m[1][3] = d * vLight.y;

    M->m[2][0] = -(n.x * vLight.z);
    M->m[2][1] = -(n.y * vLight.z);
    M->m[2][2] = fDot - vLight.z * n.z;
    M->m[2][3] = d * vLight.z;

    M->m[3][0] = -n.x;
    M->m[3][1] = -n.y;
    M->m[3][2] = -n.z;
    M->m[3][3] = d + fDot;
}

// gr_GetPerpendicularVector (0x10032c6c), with no reference vector: the
// world's up, moved off vVec when it is too near it.
static LTVector shd_GetPerpendicular(const LTVector& vVec)
{
    LTVector vPerp = v3(0.0f, 1.0f, 0.0f);
    float dot = vdot(vVec, vPerp);
    if (dot > 0.99f || dot < -0.99f)
    {
        vPerp.z += 5.0f;
        vPerp = vnorm(vPerp);
        dot = vdot(vVec, vPerp);
        if (dot > 0.99f || dot < -0.99f)
        {
            vPerp.x += 5.0f;
            vPerp.y += 5.0f;
            vPerp = vnorm(vPerp);
            dot = vdot(vVec, vPerp);
            if (dot > 0.99f || dot < -0.99f)
            {
                vPerp.x += 5.0f;
                vPerp.y -= 2.0f;
                vPerp.z -= 5.0f;
                vPerp = vnorm(vPerp);
            }
        }
    }
    float t = -vdot(vVec, vPerp);
    vPerp = vadd(vPerp, vmul(vVec, t));
    return vnorm(vPerp);
}

// ---- the polys a shadow falls on (d3d_GetSegmentPolies) ------------------------------

#define SHD_MAXPOLIES 256               // SegmentPolyList
#define SHD_MAXSTACK  512

struct ShdPolyList {
    WorldPoly* m_Polies[SHD_MAXPOLIES];
    uint32_t   m_nPolies;
};

struct ShdQuery {                       // SegmentQuery
    ShdPolyList* m_pList;
    LTVector     m_vStart, m_vEnd;
    float        m_fRadius;
};

// Each world model filed near the point: its BSP along the segment. A
// poly is taken when the segment passes within the radius of its plane
// and near its sphere, the poly was drawn this frame, it faces the camera,
// and it does not face along the segment too much.
static void shd_GetSegmentPolies(LTObject* pObj, ShdQuery* pQuery)
{
    struct StackEntry { Node* m_pNode; LTVector m_vStart, m_vEnd; };
    static StackEntry s_Stack[SHD_MAXSTACK];

    if (rd<uint32_t>(pObj, 0x54) || pObj->m_ObjectType != OT_WORLDMODEL)
        return;
    WorldBsp* pBsp = ((WorldModelInstance*)pObj)->m_pWorldBsp;
    if (!pBsp)
        return;
    typedef Node* (__thiscall* GetRootNodeFn)(WorldBsp* pThis);
    Node* pNode = (*(GetRootNodeFn**)pBsp)[1](pBsp);

    LTVector pt1 = pQuery->m_vStart, pt2 = pQuery->m_vEnd;
    LTVector vDir = vnorm(vsub(pt2, pQuery->m_vStart));
    const LTVector& vCam = g_Frame.m_Cam.m_Pos;
    float r = pQuery->m_fRadius;
    uint32_t nTop = 0;
    for (;;)
    {
        if (pNode && !(pNode->m_Flags & 3))
        {
            WorldPoly* pNodePoly = pNode->m_pPoly;
            if (!pNodePoly || !pNodePoly->m_pSurface || !pNodePoly->m_pPlane)
                return;
            const LTPlane& P = *pNodePoly->m_pPlane;
            float d1 = pdist(P, pt1), d2 = pdist(P, pt2);
            if (d1 > r && d2 > r)
            {
                pNode = pNode->m_Sides[1];
                continue;
            }
            if (d1 < -r && d2 < -r)
            {
                pNode = pNode->m_Sides[0];
                continue;
            }
            int side = d1 > 0.0f ? 1 : 0;
            float diff = d1 - d2;
            float t = (diff > 0.00001f || diff < -0.00001f) ? d1 / diff : 0.0f;
            LTVector vMid = vadd(pt1, vmul(vsub(pt2, pt1), t));

            ShdPolyList* pList = pQuery->m_pList;
            if (pNodePoly->m_FrameCode == g_Frame.m_WorldFrameCode && pList->m_nPolies < SHD_MAXPOLIES &&
                pdist(P, vCam) > 1.0f && vdot(P.m_Normal, vDir) < 0.7f &&
                vmag(vsub(vMid, pNodePoly->m_Center)) < pNodePoly->m_Radius + r)
            {
                uint32_t i;
                for (i = 0; i < pList->m_nPolies && pList->m_Polies[i] != pNodePoly; i++)
                    ;
                if (i == pList->m_nPolies)
                    pList->m_Polies[pList->m_nPolies++] = pNodePoly;
            }

            if (nTop < SHD_MAXSTACK)
            {
                StackEntry& e = s_Stack[nTop++];
                if (t > 0.0f && t < 1.0f)
                {
                    e.m_vStart = vMid;
                    e.m_vEnd = pt2;
                    pt2 = vMid;
                }
                else
                {
                    e.m_vStart = pt1;
                    e.m_vEnd = pt2;
                }
                e.m_pNode = pNode->m_Sides[!side];
                pNode = pNode->m_Sides[side];
                continue;
            }
            // The stack is full: this node's sides are left out, as in d3d.ren.
        }
        if (!nTop)
            return;
        nTop--;
        pNode = s_Stack[nTop].m_pNode;
        pt1 = s_Stack[nTop].m_vStart;
        pt2 = s_Stack[nTop].m_vEnd;
    }
}

// The world tree's slot 3 (+0x0c, the engine's 0x0048d1f0): the objects
// filed at a point (FindObjectsInBoxQ with the point as both corners), in
// object list nList, to the callback. d3d.ren calls it FindObjectsInSphere.
static void shd_FindPolies(MainWorld* pWorld, const LTVector& vAt, ShdQuery* pQuery)
{
    typedef uint32_t (__thiscall* FindAtFn)(void* pTree, LTVector* pPos, void* pCB, void* pUser, uint32_t nList);
    void* pTree = pWorld->m_WorldTree;
    LTVector vPos = vAt;
    ((*(FindAtFn**)pTree)[3])(pTree, &vPos, (void*)shd_GetSegmentPolies, pQuery, 0);
}

// ---- the shadow's frustum and its polys (DrawShadowPoly, DrawProjShadowPoly) ----------

#define SHD_MAXPOLYVERTS 128                    // MAX_SHADOWPOLY_VERTS
#define SHD_CLIPVERTS    (SHD_MAXPOLYVERTS + 8)  // one more vertex for each clip plane

struct ShdInfo {                        // ModelShadowInfo, the parts the poly draws read
    LTVector m_vOrigin;                 // the projection's origin
    LTPlane  m_Planes[6];               // the frustum, normals in
    LTMatrix m_mTex;                    // rows u, v, -, q
};

// d3d_ClipToPlane with ShadowClipPlane: keep where the distance is above
// 0; the vertex order is d3d.ren's.
static bool shd_ClipToPlane(const LTPlane& P, LTVector** ppVerts, int* pnVerts, LTVector* pOut)
{
    static uint8_t s_bIn[SHD_CLIPVERTS];
    LTVector* pVerts = *ppVerts;
    int nVerts = *pnVerts, nIn = 0;
    for (int i = 0; i < nVerts; i++)
    {
        s_bIn[i] = pdist(P, pVerts[i]) > 0.0f;
        nIn += s_bIn[i];
    }
    if (!nIn)
        return false;
    if (nIn == nVerts)
        return true;

    int n = 0, iPrev = nVerts - 1;
    for (int i = 0; i < nVerts && n < SHD_CLIPVERTS - 1; i++)
    {
        const LTVector& a = pVerts[iPrev];
        const LTVector& b = pVerts[i];
        if (s_bIn[iPrev])
            pOut[n++] = a;
        if (s_bIn[iPrev] != s_bIn[i])
        {
            float dA = pdist(P, a);
            float fDenom = dA - pdist(P, b);
            float t = (fDenom < -0.00001f || fDenom > 0.00001f) ? dA / fDenom : 0.0f;
            pOut[n++] = vadd(a, vmul(vsub(b, a), t));
        }
        iPrev = i;
    }
    *ppVerts = pOut;
    *pnVerts = n;
    return true;
}

// One world poly under the shadow: lifted off its plane, clipped to the
// frustum (planes 0..5 in order), the texture projected on it. The planar
// shadow (bFade false) has ModelShadowAlpha everywhere; the projected one
// fades out to fMaxDist from the origin.
static void shd_DrawPoly(const ShdInfo& info, WorldPoly* pPoly, bool bFade, float fMaxDist)
{
    static LTVector s_A[SHD_CLIPVERTS], s_B[SHD_CLIPVERTS];

    WorldPolyVertex* pSrc;
    int nVerts;
    if (cvi(CV_FixTJunc))
    {
        pSrc = pPoly->m_pFixVerts;
        nVerts = pPoly->m_nFixVerts;
    }
    else
    {
        pSrc = pPoly->m_Verts;
        nVerts = pPoly->m_nVerts;
    }
    if (nVerts > SHD_MAXPOLYVERTS)
    {
        mr_Print("Error: vertex buffer overflow");
        return;
    }
    if (!pSrc || nVerts < 3 || !pPoly->m_pPlane)
        return;

    LTVector vLift = vmul(pPoly->m_pPlane->m_Normal, cvf(CV_ModelShadowOffset));
    for (int i = 0; i < nVerts; i++)
        s_A[i] = vadd(*pSrc[i].m_pPos, vLift);

    LTVector* pVerts = s_A;
    for (int p = 0; p < 6; p++)
        if (!shd_ClipToPlane(info.m_Planes[p], &pVerts, &nVerts, pVerts == s_A ? s_B : s_A))
            return;
    if (nVerts < 3)
        return;

    const LTMatrix& T = info.m_mTex;
    uint8_t nAlpha = (uint8_t)cvi(CV_ModelShadowAlpha);
    rd_Begin(RD_TRIANGLE_FAN);
    for (int i = 0; i < nVerts; i++)
    {
        const LTVector& v = pVerts[i];
        if (bFade)
        {
            float fAlpha = 1.0f - vmag(vsub(v, info.m_vOrigin)) / fMaxDist;
            if (fAlpha < 0.0f)
                fAlpha = 0.0f;
            nAlpha = (uint8_t)(int)(cvf(CV_ModelShadowAlpha) * fAlpha);
        }
        float u = T.m[0][0] * v.x + T.m[0][1] * v.y + T.m[0][2] * v.z + T.m[0][3];
        float w = T.m[1][0] * v.x + T.m[1][1] * v.y + T.m[1][2] * v.z + T.m[1][3];
        float q = T.m[3][0] * v.x + T.m[3][1] * v.y + T.m[3][2] * v.z + T.m[3][3];
        float iq = q != 0.0f ? 1.0f / q : 0.0f;
        rd_Color4ub(0, 0, 0, nAlpha);
        rd_TexCoord2f(u * iq, w * iq);
        rd_Vertex3f(v.x, v.y, v.z);
    }
    rd_End();
}

// The device states both shadows draw with: depth test without writes,
// the source-alpha blend, unit 0 only, modulated.
static void shd_BeginDraw(void)
{
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_DEPTH | RD_ATTR_ENABLE | RD_ATTR_TEXTURE | RD_ATTR_FOG);
    mr_ForgetBindings();
    for (int i = g_RDCaps.nTextureUnits - 1; i >= 1; i--)
    {
        rd_ActiveTexture(i);
        rd_Disable(RD_TEXTURE_2D);
    }
    rd_ActiveTexture(0);
    rd_Enable(RD_TEXTURE_2D);
    rd_TexEnv(RD_ENV_MODULATE);
    rd_Enable(RD_DEPTH_TEST);
    rd_DepthMask(false);
    rd_Disable(RD_ALPHA_TEST);
    rd_Enable(RD_BLEND);
    rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);
}

static void shd_EndDraw(void)
{
    rd_PopAttrib();
    mr_ForgetBindings();
}

// ---- the planar shadow (DrawShadows) ---------------------------------------------------

// d3d_CreateFalloffTexture (d3d_texture.cpp): 16x16 white, the alpha
// falling off with the cube of the distance from the centre.
static uint32_t g_FalloffTex, g_nFalloffGen;

static uint32_t shd_FalloffTexture(void)
{
    if (g_FalloffTex && g_nFalloffGen == g.nContextGen)
        return g_FalloffTex;
    uint8_t px[16 * 16 * 4];
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
        {
            float dx = (float)x - 7.5f, dy = (float)y - 7.5f;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 7.5f)
                dist = 7.5f;
            uint8_t* p = &px[(y * 16 + x) * 4];
            p[0] = p[1] = p[2] = 255;
            p[3] = (uint8_t)(int)((1.0f - powf(dist * (1.0f / 7.5f), 3.0f)) * 255.9f);
        }
    g_FalloffTex = rd_GenTexture();
    g_nFalloffGen = g.nContextGen;
    rd_BindTexture(g_FalloffTex);
    rd_TexImage(0, RD_FMT_RGBA8, 16, 16, RD_PIX_RGBA, px);
    rd_TexFilter(RD_LINEAR, RD_LINEAR);
    rd_TexWrap(false, false);
    rd_TexMaxLevel(0);
    return g_FalloffTex;
}

#define SHD_MAXSHADOWS 8                // MAX_MODEL_SHADOWS, MAX_PROJ_SHADOWS

static void shd_DrawPlanar(LTObject* pInst, const void* pModel, int nShadows, MainWorld* pWorld)
{
    static ShdPolyList s_Lists[SHD_MAXSHADOWS];
    LTVector vLocalDirs[SHD_MAXSHADOWS];

    float fShadowLength = rd<float>(pModel, MD_SHADOWLENGTH);
    LTVector vShadowOffset = rd<LTVector>(pModel, MD_SHADOWOFFSET);
    float fSizeX = rd<float>(pModel, MD_SHADOWSIZEX);
    float fSizeY = rd<float>(pModel, MD_SHADOWSIZEY);
    float fShadowStart = rd<float>(pModel, MD_SHADOWSTART);
    const LTVector& vPos = pInst->m_Pos;

    // The shadow directions: d3d.ren's m_vShadowDirs[8] lies over the
    // drawer's m_vLightDir (set to straight down here), m_vShadowDir and
    // m_vSpecularLight (their constructor's values; nothing changes them)
    // and five unused vectors of the global drawer (zero).
    LTVector vDirs[SHD_MAXSHADOWS];
    memset(vDirs, 0, sizeof(vDirs));
    vDirs[0] = v3(0.0f, -1.0f, 0.0f);
    vDirs[1] = vnorm(v3(-2.0f, -2.0f, -2.0f));
    vDirs[2] = vnorm(v3(2.0f, -2.0f, -1.0f));

    // Gather the polys along each direction.
    uint32_t nTotal = 0;
    float fLength = fShadowStart + fShadowLength;
    for (int i = 0; i < nShadows; i++)
    {
        ShdQuery query;
        query.m_pList = &s_Lists[i];
        s_Lists[i].m_nPolies = 0;
        LTVector vStart = vsub(vPos, vmul(vDirs[i], fShadowStart));
        vLocalDirs[i] = vnorm(vsub(vadd(vPos, vShadowOffset), vStart));
        query.m_vStart = vStart;
        query.m_vEnd = vadd(vStart, vmul(vLocalDirs[i], fLength));
        query.m_fRadius = fSizeY > fSizeX ? fSizeY : fSizeX;
        shd_FindPolies(pWorld, vPos, &query);
        nTotal += s_Lists[i].m_nPolies;
    }
    if (!nTotal)
        return;

    shd_BeginDraw();
    rd_BindTexture(shd_FalloffTexture());
    rd_FogFactor(1.0f);                 // d3d.ren's specular alpha 255: no fog

    for (int i = 0; i < nShadows; i++)
    {
        if (!s_Lists[i].m_nPolies)
            continue;
        ShdInfo info;
        const LTVector& vDir = vLocalDirs[i];
        info.m_vOrigin = vsub(vPos, vmul(vDir, fShadowStart));
        LTPlane plane;
        plane.m_Normal = vDir;
        LTVector vFar = vadd(info.m_vOrigin, vmul(vDir, fLength));
        plane.m_Dist = vdot(plane.m_Normal, vFar);

        LTVector vUp = shd_GetPerpendicular(vDir);
        LTVector vRight = vcross(vDir, vUp);

        // The frustum's corners.
        LTVector vC0 = vsub(vFar, vmul(vRight, fSizeX));
        LTVector vC1 = vadd(vFar, vmul(vRight, fSizeX));
        LTVector vC2 = vadd(vFar, vmul(vUp, fSizeY));
        LTVector vC3 = vsub(vFar, vmul(vUp, fSizeY));
        LTVector vCorner = vsub(vsub(vFar, vmul(vRight, fSizeX)), vmul(vUp, fSizeY));

        // Its planes: the two caps and the four sides (left 2, top 3,
        // right 4, bottom 5).
        info.m_Planes[0].m_Normal = vDir;
        info.m_Planes[0].m_Dist = vdot(vDir, vPos);
        info.m_Planes[1].m_Normal = vneg(vDir);
        info.m_Planes[1].m_Dist = vdot(info.m_Planes[1].m_Normal, vadd(vPos, vmul(vDir, fShadowLength)));

        info.m_Planes[2].m_Normal = vnorm(vcross(vsub(vC0, info.m_vOrigin), vUp));
        info.m_Planes[2].m_Dist = vdot(info.m_Planes[2].m_Normal, info.m_vOrigin);
        info.m_Planes[4].m_Normal = vnorm(vcross(vUp, vsub(vC1, info.m_vOrigin)));
        info.m_Planes[4].m_Dist = vdot(info.m_Planes[4].m_Normal, info.m_vOrigin);
        info.m_Planes[3].m_Normal = vnorm(vcross(vsub(vC2, info.m_vOrigin), vRight));
        info.m_Planes[3].m_Dist = vdot(info.m_Planes[3].m_Normal, info.m_vOrigin);
        info.m_Planes[5].m_Normal = vnorm(vcross(vRight, vsub(vC3, info.m_vOrigin)));
        info.m_Planes[5].m_Dist = vdot(info.m_Planes[5].m_Normal, info.m_vOrigin);

        // The texture projection: from the origin onto the far plane, then
        // the corner's basis, scaled to 0..1.
        LTMatrix mProj, mBasis, mScale, mTemp;
        shd_SetupPlanar(&mProj, info.m_vOrigin, plane);
        mat_Init(&mBasis, vRight.x, vRight.y, vRight.z, -vdot(vRight, vCorner),
                 vUp.x, vUp.y, vUp.z, -vdot(vUp, vCorner));
        mat_Init(&mScale, 1.0f / (fSizeX * 2.0f), 0.0f, 0.0f, 0.0f,
                 0.0f, 1.0f / (fSizeY * 2.0f), 0.0f, 0.0f);
        mat_Mul(&mTemp, mScale, mBasis);
        mat_Mul(&info.m_mTex, mTemp, mProj);

        for (uint32_t j = 0; j < s_Lists[i].m_nPolies; j++)
            shd_DrawPoly(info, s_Lists[i].m_Polies[j], false, 0.0f);
    }

    shd_EndDraw();
}

// ---- the projected shadow (DrawProjShadows) ------------------------------------------

// The lights the model is lit by (CD3DModelDraw::SetupLighting, AddLight,
// as mr_models.cpp selects them), in the world.
struct ShdLight { LTVector m_Pos; float m_fRadius; };

struct ShdLights {
    ShdLight m_Lights[16];
    uint32_t m_nLights, m_nMax;
    LTVector m_vPos;
    float    m_fVisRadius;
    bool     m_bSocket;
    LTMatrix m_mBack;                   // the light space's positions back to the world
};

static void shd_AddLight(ShdLights* L, const LTVector& pos, float fRadius, const LTVector& dir, float fCos)
{
    if (L->m_nLights >= L->m_nMax)
        return;
    LTVector d = vsub(L->m_vPos, pos);
    float fDist = vmag(d);
    if (fDist - L->m_fVisRadius >= fRadius)
        return;
    if (fCos > -1.0f && fCos < 1.0f && fDist != 0.0f)
    {
        float fDot = vdot(d, dir) * (1.0f / fDist);
        if (fDot < fCos)
            return;
    }
    ShdLight& s = L->m_Lights[L->m_nLights++];
    s.m_Pos = L->m_bSocket ? vxform(L->m_mBack, pos) : pos;
    s.m_fRadius = sqrtf(fRadius * fRadius);     // d3d.ren keeps the square
}

static void shd_StaticLightCB(void* pObj, void* pUser)
{
    const StaticLight* S = (const StaticLight*)pObj;
    shd_AddLight((ShdLights*)pUser, S->m_Pos, S->m_fRadius, S->m_Dir, S->m_fCone);
}

static void shd_GatherLights(LTObject* pInst, const void* pModel, const LTMatrix* pNodes, uint32_t nNodes,
                             MainWorld* pWorld, ShdLights* L)
{
    L->m_nLights = 0;
    int nMax = cvi(CV_MaxModelLights);
    L->m_nMax = nMax < 0 ? 0 : nMax > 16 ? 16 : (uint32_t)nMax;
    L->m_vPos = pInst->m_Pos;
    L->m_fVisRadius = rd<float>(pModel, MD_VISRADIUS);
    if (pInst->m_Flags & FLAG_NOLIGHT)
        return;

    // A socket-lit model's lights are kept in its socket's space, and
    // DrawProjShadows takes them back to the world through the model's
    // own matrix (m_mLightTransform is m_mTransform), as here.
    L->m_bSocket = false;
    uint32_t iSocket = rd<uint32_t>(pModel, MD_SOCKETNODE);
    if (rd<uint32_t>(pModel, MD_SOCKETLIT) && iSocket < nNodes)
    {
        LTMatrix socket = rd<LTMatrix>(pModel, MD_SOCKETMAT), lightToWorld, toLight, world;
        mat_Mul(&lightToWorld, pNodes[iSocket], socket);
        mat_InvAffine(&toLight, lightToWorld);
        mr_ObjectMatrix(pInst, pInst->m_Pos, pInst->m_Scale, &world);
        mat_Mul(&L->m_mBack, world, toLight);
        L->m_bSocket = true;
    }

    LTVector vZero = v3(0.0f, 0.0f, 0.0f);
    MRObjList* pLights = mr_ObjList(VS_LIGHTS);
    for (uint32_t i = 0, n = 0; i < pLights->m_nObjects && n < MR_MAXWORLDLIGHTS; i++)
    {
        LTObject* pL = pLights->m_pObjects[i];
        if (pL->m_Flags & FLAG_ONLYLIGHTWORLD)
            continue;
        n++;
        shd_AddLight(L, pL->m_Pos, obj_LightRadius(pL), vZero, -1.0f);
        if (L->m_nLights >= L->m_nMax)
            break;
    }
    if (L->m_nLights < L->m_nMax)
    {
        FindObjInfo q;
        memset(&q, 0, sizeof(q));
        q.m_iObjArray = 1;
        float r = L->m_fVisRadius;
        q.m_Min = v3(L->m_vPos.x - r, L->m_vPos.y - r, L->m_vPos.z - r);
        q.m_Max = v3(L->m_vPos.x + r, L->m_vPos.y + r, L->m_vPos.z + r);
        q.m_CB = shd_StaticLightCB;
        q.m_pUser = L;
        mw_FindObjectsInBox(pWorld, &q);
    }
}

// The model's vertex in the world (TransformVerts): its weights through
// the world-space nodes, blended towards the next LOD's vertex.
static LTVector shd_Skin(const ShdVert& v, const LTMatrix* pNodes, uint32_t nNodes)
{
    float h[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (uint32_t w = 0; w < v.m_nWeights; w++)
    {
        const ShdWeight& nw = v.m_Weights[w];
        if (nw.m_iNode >= nNodes)
            continue;
        const LTMatrix& M = pNodes[nw.m_iNode];
        for (int r = 0; r < 4; r++)
            h[r] += M.m[r][0] * nw.m_Vec[0] + M.m[r][1] * nw.m_Vec[1] + M.m[r][2] * nw.m_Vec[2] + M.m[r][3] * nw.m_Vec[3];
    }
    float inv = h[3] != 0.0f ? 1.0f / h[3] : 1.0f;
    return v3(h[0] * inv, h[1] * inv, h[2] * inv);
}

// The model's LOD blend (SetupLOD), as mr_models.cpp works it out for the
// LOD it drew: the shadow blends towards its own next LOD by it.
static float shd_LODBlend(LTObject* pInst, const void* pModel, uint32_t iLOD)
{
    if (!cvi(CV_ModelLODBlendEnable) || iLOD + 1 >= md_nLODs(pModel) + 1)
        return 0.0f;
    float fDist = vmag(vsub(pInst->m_Pos, g_Frame.m_Cam.m_Pos));
    float fZoom = cvf(CV_ModelZoomScale);
    if (fZoom != 0.0f)
        fDist /= fZoom;
    float fNext = md_LODDist(pModel, iLOD + 1);
    float fBlend = cvf(CV_ModelLODBlendDist);
    float fHalf = (fNext - md_LODDist(pModel, iLOD)) * 0.5f;
    fBlend = fBlend < fHalf ? fBlend : fHalf;
    float fTo = fNext - fDist;
    if (fTo < fBlend && fBlend > 0.0f)
        return 1.0f - fTo / fBlend;
    return 0.0f;
}

// Fill a triangle into the image (black, 0xff000000: colour 0, alpha 255)
// where it covers a pixel centre. The triangles all fill the same colour,
// so the edge rule does not matter.
static void shd_FillTri(uint32_t* pImage, int nRes, const float* a, const float* b, const float* c)
{
    for (int k = 0; k < 2; k++)
        if (!isfinite(a[k]) || !isfinite(b[k]) || !isfinite(c[k]))
            return;
    float fArea = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    if (fArea == 0.0f)
        return;
    float fMinX = fminf(a[0], fminf(b[0], c[0])), fMaxX = fmaxf(a[0], fmaxf(b[0], c[0]));
    float fMinY = fminf(a[1], fminf(b[1], c[1])), fMaxY = fmaxf(a[1], fmaxf(b[1], c[1]));
    if (fMaxX < 0.0f || fMaxY < 0.0f || fMinX > (float)(nRes - 1) || fMinY > (float)(nRes - 1))
        return;
    int x0 = fMinX < 0.0f ? 0 : (int)ceilf(fMinX);
    int y0 = fMinY < 0.0f ? 0 : (int)ceilf(fMinY);
    int x1 = fMaxX > (float)(nRes - 1) ? nRes - 1 : (int)floorf(fMaxX);
    int y1 = fMaxY > (float)(nRes - 1) ? nRes - 1 : (int)floorf(fMaxY);
    float s = fArea > 0.0f ? 1.0f : -1.0f;
    for (int y = y0; y <= y1; y++)
    {
        float py = (float)y;
        uint32_t* pRow = pImage + y * nRes;
        for (int x = x0; x <= x1; x++)
        {
            float px = (float)x;
            float w0 = ((c[0] - b[0]) * (py - b[1]) - (c[1] - b[1]) * (px - b[0])) * s;
            float w1 = ((a[0] - c[0]) * (py - c[1]) - (a[1] - c[1]) * (px - c[0])) * s;
            float w2 = ((b[0] - a[0]) * (py - a[1]) - (b[1] - a[1]) * (px - a[0])) * s;
            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                pRow[x] = 0xff000000u;
        }
    }
}

// The LM buffer: the shadow image as a texture (the colour the image, the
// alpha 255 minus it).
static uint32_t g_ProjTex, g_nProjGen;
static int g_nProjTexRes;

static void shd_UploadImage(const uint32_t* pImage, int nRes)
{
    if (!g_ProjTex || g_nProjGen != g.nContextGen)
    {
        g_ProjTex = rd_GenTexture();
        g_nProjGen = g.nContextGen;
        g_nProjTexRes = 0;
    }
    rd_BindTexture(g_ProjTex);
    if (g_nProjTexRes != nRes)
    {
        rd_TexImage(0, RD_FMT_RGBA8, nRes, nRes, RD_PIX_BGRA, pImage);
        g_nProjTexRes = nRes;
    }
    else
        rd_TexSubImage(0, 0, 0, nRes, nRes, RD_PIX_BGRA, pImage);
    rd_TexFilter(RD_LINEAR, RD_LINEAR);
    rd_TexWrap(false, false);
    rd_TexMaxLevel(0);
}

// ModelShadowProjShow: the LM buffer over the screen at (x, y), its own
// colours, no blend, no depth (CD3DLMTexture::DrawToScreen).
static void shd_ShowImage(int x, int y, int nRes)
{
    rd_PushAttrib(RD_ATTR_ALL);
    rd_Viewport(0, 0, (int)g.nWidth, (int)g.nHeight);
    rd_MatrixMode(RD_PROJECTION);
    rd_PushMatrix();
    rd_LoadIdentity();
    rd_Ortho(0.0, (double)g.nWidth, (double)g.nHeight, 0.0, -1.0, 1.0);
    rd_MatrixMode(RD_MODELVIEW);
    rd_PushMatrix();
    rd_LoadIdentity();
    rd_Disable(RD_DEPTH_TEST);
    rd_Disable(RD_BLEND);
    rd_Disable(RD_ALPHA_TEST);
    rd_Disable(RD_FOG);
    rd_Disable(RD_SCISSOR_TEST);
    rd_ActiveTexture(0);
    rd_Enable(RD_TEXTURE_2D);
    rd_BindTexture(g_ProjTex);
    rd_TexEnv(RD_ENV_MODULATE);
    float l = (float)x, t = (float)y, r = l + (float)nRes, b = t + (float)nRes;
    rd_Color4ub(255, 255, 255, 255);
    rd_Begin(RD_QUADS);
    rd_TexCoord2f(0.0f, 0.0f); rd_Vertex2f(l, t);
    rd_TexCoord2f(0.0f, 1.0f); rd_Vertex2f(l, b);
    rd_TexCoord2f(1.0f, 1.0f); rd_Vertex2f(r, b);
    rd_TexCoord2f(1.0f, 0.0f); rd_Vertex2f(r, t);
    rd_End();
    rd_MatrixMode(RD_PROJECTION);
    rd_PopMatrix();
    rd_MatrixMode(RD_MODELVIEW);
    rd_PopMatrix();
    rd_PopAttrib();
}

static void shd_DrawProj(LTObject* pInst, const void* pModel, const LTMatrix* pNodes, uint32_t nNodes,
                         uint32_t iModelLOD, uint32_t nMaxShadows, MainWorld* pWorld)
{
    static ShdPolyList s_Lists[SHD_MAXSHADOWS];
    static uint32_t s_Image[256 * 256];
    static LTVector* s_pVerts;
    static uint32_t s_nVertAlloc;

    LTVector vLightPositions[SHD_MAXSHADOWS], vLightDirs[SHD_MAXSHADOWS];
    float fLightDist[SHD_MAXSHADOWS], fLightRadius[SHD_MAXSHADOWS];
    LTVector vObjPos = pInst->m_Pos;
    LTVector vDims = pInst->m_Dims;

    // Find the nearest lights. When the list is full, a light takes the
    // place of the first one farther than it.
    ShdLights L;
    shd_GatherLights(pInst, pModel, pNodes, nNodes, pWorld, &L);
    uint32_t nShadows = 0;
    for (uint32_t i = 0; i < L.m_nLights; i++)
    {
        LTVector vLightPos = L.m_Lights[i].m_Pos;
        LTVector vTo = vsub(vObjPos, vLightPos);
        float fDist = vmag(vTo);
        LTVector vLightDir = vnorm(vTo);
        uint32_t j;
        if (nShadows == nMaxShadows)
        {
            for (j = 0; j < nMaxShadows; j++)
                if (fLightDist[j] > fDist)
                    break;
            if (j == nMaxShadows)
                continue;
        }
        else
            j = nShadows;
        vLightPositions[j] = vLightPos;
        vLightDirs[j] = vLightDir;
        fLightDist[j] = fDist;
        fLightRadius[j] = L.m_Lights[i].m_fRadius;
        if (nShadows < nMaxShadows)
            nShadows++;
    }
    if (!nShadows)
        return;

    int nRes = cvi(CV_ModelShadowProjRes);
    nRes = nRes < 8 ? 8 : nRes > 256 ? 256 : nRes;
    if (nRes & (nRes - 1))
    {
        mr_Print("Invalid texture size for shadows -- must be power of 2...");
        nRes = 128;
        g_Vars[CV_ModelShadowProjRes].m_fValue = 128.0f;
    }

    // The shadow's LOD, ModelShadowProjLOD more than the model's, at most
    // the last LOD. d3d.ren limits it to the last LOD but one, and a model
    // with no lower LODs gets no shadow (RETAIL_BUGS.md); fixed here.
    uint32_t nLODs = md_nLODs(pModel);
    uint32_t iLOD = iModelLOD + (uint32_t)cvi(CV_ModelShadowProjLOD);
    if (iLOD > nLODs)
        iLOD = nLODs;
    float fLODBlend = shd_LODBlend(pInst, pModel, iModelLOD);
    uint32_t nHidden = rd<uint32_t>(pInst, MI_HIDDEN);

    float fMag = vmag(vDims);
    float fSize = fMag + fMag;
    if (!(fMag > 0.0f))
        return;

    // Room for the shadow LOD's vertices.
    uint32_t nAllVerts = 0;
    for (uint32_t p = 0; p < md_nPieces(pModel); p++)
    {
        ShdPiece* pPiece = md_Piece(pModel, p);
        ShdVertSet* pLOD = pPiece ? shd_PieceLOD(pPiece, iLOD) : NULL;
        if (pLOD)
            nAllVerts += pLOD->m_nVerts;
    }
    if (nAllVerts > s_nVertAlloc)
    {
        LTVector* pNew = (LTVector*)realloc(s_pVerts, nAllVerts * sizeof(LTVector));
        if (!pNew)
            return;
        s_pVerts = pNew;
        s_nVertAlloc = nAllVerts;
    }

    shd_BeginDraw();
    rd_Disable(RD_FOG);

    float fRes = (float)nRes;
    for (uint32_t i = 0; i < nShadows; i++)
    {
        // The polys around the model, towards the light.
        ShdQuery query;
        query.m_pList = &s_Lists[i];
        s_Lists[i].m_nPolies = 0;
        float fRadius = fLightRadius[i];
        LTVector vPos = vLightPositions[i];
        LTVector vDir = vLightDirs[i];
        query.m_vStart = vPos;
        query.m_vEnd = vadd(vPos, vmul(vDir, fRadius));
        query.m_fRadius = fSize;
        shd_FindPolies(pWorld, vObjPos, &query);
        if (!s_Lists[i].m_nPolies)
            continue;

        // The light's basis.
        if (fabsf(vDir.y) > 0.9999f)
        {
            vDir.z += 0.01f;
            vDir = vnorm(vDir);
        }
        LTVector vRight = vnorm(vcross(vDir, v3(0.0f, 1.0f, 0.0f)));
        LTVector vUp = vnorm(vcross(vRight, vDir));

        // The model's vertices, flattened onto the plane across the light.
        float fScale = fRes / fMag;
        float fMaxX = 0.0f, fMaxY = 0.0f;
        LTVector* pOut = s_pVerts;
        for (uint32_t p = 0; p < md_nPieces(pModel); p++)
        {
            ShdPiece* pPiece = md_Piece(pModel, p);
            ShdVertSet* pLOD = pPiece ? shd_PieceLOD(pPiece, iLOD) : NULL;
            if (!pLOD)
                continue;
            ShdVertSet* pNext = fLODBlend != 0.0f ? shd_PieceLOD(pPiece, iLOD + 1) : NULL;
            if (p < 32 && (nHidden & (1u << p)))
                continue;
            for (uint32_t v = 0; v < pLOD->m_nVerts; v++)
            {
                const ShdVert& mv = pLOD->m_pVerts[v];
                LTVector w = shd_Skin(mv, pNodes, nNodes);
                if (pNext && mv.m_iReplacement < pNext->m_nVerts)
                {
                    LTVector n = shd_Skin(pNext->m_pVerts[mv.m_iReplacement], pNodes, nNodes);
                    w = vadd(w, vmul(vsub(n, w), fLODBlend));
                }
                LTVector vTemp = vsub(w, vObjPos);
                LTVector& o = pOut[v];
                o.x = vdot(vRight, vTemp) * fScale * 0.5f;
                o.y = vdot(vUp, vTemp) * fScale * 0.5f;
                o.z = 0.0f;
                if (fabsf(o.x) > fMaxX)
                    fMaxX = fabsf(o.x);
                if (fabsf(o.y) > fMaxY)
                    fMaxY = fabsf(o.y);
            }
            pOut += pLOD->m_nVerts;
        }
        if (!(fMaxX > 0.0f) || !(fMaxY > 0.0f))
            continue;

        // Its triangles, scaled to fill the image less a texel each side,
        // black on white.
        for (int k = 0; k < nRes * nRes; k++)
            s_Image[k] = 0x00ffffffu;
        float fUScale = (fRes - 2.0f) / (fMaxX * 2.0f);
        float fVScale = (fRes - 2.0f) / (fMaxY * 2.0f);
        float fHalf = fRes * 0.5f;
        pOut = s_pVerts;
        for (uint32_t p = 0; p < md_nPieces(pModel); p++)
        {
            ShdPiece* pPiece = md_Piece(pModel, p);
            ShdVertSet* pLOD = pPiece ? shd_PieceLOD(pPiece, iLOD) : NULL;
            if (!pLOD)
                continue;
            if (p < 32 && (nHidden & (1u << p)))
                continue;
            for (uint32_t t = 0; t < pLOD->m_nTris; t++)
            {
                const ShdTri& tri = pLOD->m_pTris[t];
                float c[3][2];
                bool bOk = true;
                for (int k = 0; k < 3; k++)
                {
                    uint32_t idx = tri.m_Indices[k];
                    if (idx >= pLOD->m_nVerts)
                    {
                        bOk = false;
                        break;
                    }
                    c[k][0] = pOut[idx].x * fUScale + fHalf;
                    c[k][1] = pOut[idx].y * fVScale + fHalf;
                }
                if (bOk)
                    shd_FillTri(s_Image, nRes, c[0], c[1], c[2]);
            }
            pOut += pLOD->m_nVerts;
        }
        shd_UploadImage(s_Image, nRes);

        if (cvi(CV_ModelShadowProjShow))
        {
            shd_ShowImage(nRes, (int)(nShadows - i - 1) * nRes, nRes);
            rd_BindTexture(g_ProjTex);
        }

        // The shadow's frustum and projection.
        ShdInfo info;
        info.m_vOrigin = vPos;
        LTPlane plane;
        plane.m_Normal = vDir;
        plane.m_Dist = vdot(vDir, vObjPos);

        LTVector vRightS = vmul(vRight, fSize);
        LTVector vLeft = vsub(vObjPos, vmul(vRightS, 0.5f));
        LTVector vRightP = vadd(vObjPos, vmul(vRightS, 0.5f));
        LTVector vUpS = vmul(vUp, fSize);
        LTVector vTop = vadd(vObjPos, vmul(vUpS, 0.5f));
        LTVector vBottom = vsub(vObjPos, vmul(vUpS, 0.5f));
        LTVector vCorner = vsub(vsub(vObjPos, vmul(vRightS, 0.5f)), vmul(vUpS, 0.5f));

        info.m_Planes[0].m_Normal = vDir;
        info.m_Planes[0].m_Dist = vdot(vDir, vsub(vObjPos, vmul(vDir, fMag)));
        info.m_Planes[1].m_Normal = vneg(vDir);
        info.m_Planes[1].m_Dist = vdot(info.m_Planes[1].m_Normal, vadd(vObjPos, vmul(vDir, fRadius)));

        // Retail's slots: the left side 2, the top 3, the right side 4,
        // the bottom 5 (git 9c5e501).
        info.m_Planes[2].m_Normal = vnorm(vcross(vsub(vLeft, vPos), vUp));
        info.m_Planes[2].m_Dist = vdot(info.m_Planes[2].m_Normal, vPos);
        info.m_Planes[4].m_Normal = vnorm(vcross(vUp, vsub(vRightP, vPos)));
        info.m_Planes[4].m_Dist = vdot(info.m_Planes[4].m_Normal, vPos);
        info.m_Planes[3].m_Normal = vnorm(vcross(vsub(vTop, vPos), vRight));
        info.m_Planes[3].m_Dist = vdot(vPos, info.m_Planes[3].m_Normal);
        info.m_Planes[5].m_Normal = vnorm(vcross(vRight, vsub(vBottom, vPos)));
        info.m_Planes[5].m_Dist = vdot(vPos, info.m_Planes[5].m_Normal);

        LTMatrix mProj, mBasis, mScale, mTemp;
        shd_SetupPlanar(&mProj, vPos, plane);
        mat_Init(&mBasis, vRight.x, vRight.y, vRight.z, -vdot(vRight, vCorner),
                 vUp.x, vUp.y, vUp.z, -vdot(vUp, vCorner));
        mat_Init(&mScale, fUScale / fSize, 0.0f, 0.0f, (1.0f - fUScale) * 0.5f,
                 0.0f, fVScale / fSize, 0.0f, (1.0f - fVScale) * 0.5f);
        mat_Mul(&mTemp, mScale, mBasis);
        mat_Mul(&info.m_mTex, mTemp, mProj);

        // Project it onto the polys that have the light and the model on
        // the same side.
        for (uint32_t k = 0; k < s_Lists[i].m_nPolies; k++)
        {
            WorldPoly* pPoly = s_Lists[i].m_Polies[k];
            const LTPlane& P = *pPoly->m_pPlane;
            float d1 = pdist(P, vPos);
            float d2 = pdist(P, vObjPos);
            if ((d1 > 0.0f || d2 < 0.0f) && (d1 < 0.0f || d2 > 0.0f))
                shd_DrawPoly(info, pPoly, true, fRadius);
        }
    }

    shd_EndDraw();
}

// ---- the entry ---------------------------------------------------------------------------

void mr_DrawModelShadow(LTObject* pInst, const void* pModel, const LTMatrix* pNodes, uint32_t nNodes,
                        uint32_t iLOD)
{
    int nShadows = cvi(CV_MaxModelShadows);
    if (nShadows > SHD_MAXSHADOWS)
        nShadows = SHD_MAXSHADOWS;
    if (nShadows <= 0)
        return;
    MainWorld* pWorld = g_Frame.m_pContext ? g_Frame.m_pContext->m_pWorld : NULL;
    if (!pInst || !pModel || !pWorld || !rd<uint32_t>(pModel, MD_SHADOW))
        return;

    if (cvi(CV_ModelShadowProj))
        shd_DrawProj(pInst, pModel, pNodes, nNodes, iLOD, (uint32_t)nShadows, pWorld);
    else
        shd_DrawPlanar(pInst, pModel, nShadows, pWorld);
}
