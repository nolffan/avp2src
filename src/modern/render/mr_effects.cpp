// mr_effects.cpp -- particle systems, poly grids, line systems and
// canvases, as d3d.ren draws them (d3d_matrix.cpp, drawparticles.cpp,
// drawpolygrid.cpp, drawlight.cpp, draw_canvas.cpp).
//
// The engine moves the particles and advances the textures' animations;
// the renderer only reads. All four are relative to their object's
// position, rotation and scale.

#include <math.h>

#include "mrobjects.h"

#define FLAG_INTERNAL1 0x40000000       // set on what was drawn (the SDK's FLAG_WASDRAWN)

template <typename T> static inline T rd(const void* p, uint32_t off) { T v; memcpy(&v, (const uint8_t*)p + off, sizeof(T)); return v; }

static inline int mr_Round(float f) { return (int)lrintf(f); }

// d3d_SetupTransformation: the rotation (a component gone wild reset),
// each column times the scale, the position.
void mr_ObjectMatrix(LTObject* pObj, const LTVector& pos, const LTVector& scale, LTMatrix* pM)
{
    float* q = &pObj->m_Rotation.x;
    for (int i = 0; i < 4; i++)
        if (q[i] > 100000.0f || q[i] < -100000.0f)
            q[i] = i == 3 ? 1.0f : 0.0f;
    float R[3][3];
    mr_RotationMatrix(pObj->m_Rotation, R);
    for (int r = 0; r < 3; r++)
    {
        pM->m[r][0] = R[r][0] * scale.x;
        pM->m[r][1] = R[r][1] * scale.y;
        pM->m[r][2] = R[r][2] * scale.z;
    }
    pM->m[0][3] = pos.x;
    pM->m[1][3] = pos.y;
    pM->m[2][3] = pos.z;
    pM->m[3][0] = pM->m[3][1] = pM->m[3][2] = 0.0f;
    pM->m[3][3] = 1.0f;
}

// Bind a SharedTexture on unit 0 (or turn texturing off); its record.
static MRTexture* mr_BindShared(SharedTexture* pShared)
{
    MRTexture* pTex = NULL;
    if (pShared)
    {
        mr_TextureId(pShared);
        pTex = (MRTexture*)pShared->m_pRenderData;
    }
    mr_ForgetBindings();
    mr_SetBaseTexture(pTex);
    rd_TexEnv(RD_ENV_MODULATE);
    return pTex;
}

// ---- particle systems (d3d_DrawParticles) -------------------------------------------
//
// Each particle is a square on the screen around its projected place,
// its half size in pixels the view's width times its size times the
// focal scale over its depth: in camera space, two times its size. One
// fog for the whole system, at its centre. Particles nearer than the near
// plane, beyond the far plane, or with x or y outside 1.2 times the view
// are left out.

struct PSParticle {
    uint8_t    m_00[0x0c];
    LTVector   m_Color;                 // 0x0c 0..255
    float      m_Alpha;                 // 0x18 0..1
    float      m_Size;                  // 0x1c
    PSParticle* m_pNext;                // 0x20
    LTVector   m_Pos;                   // 0x24 the system's space
};

void mr_DrawParticleSystem(LTObject* pObj)
{
    if (!cvi(CV_DrawParticles))
        return;
    // The cull sphere: +0x210, radius +0x21c times the largest scale.
    LTVector vCenter = rd<LTVector>(pObj, 0x210);
    float fRadius = rd<float>(pObj, 0x21c);
    float fMax = pObj->m_Scale.x > pObj->m_Scale.y ? pObj->m_Scale.x : pObj->m_Scale.y;
    fMax = fMax > pObj->m_Scale.z ? fMax : pObj->m_Scale.z;
    bool bClose = (pObj->m_Flags & FLAG_REALLYCLOSE) != 0;
    if (!bClose && !mr_SphereVisible(vCenter, fRadius * fMax))
        return;
    pObj->m_Flags |= FLAG_INTERNAL1;

    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_FOG | RD_ATTR_ENABLE | RD_ATTR_TEXTURE);
    rd_Enable(RD_BLEND);
    mr_ObjectBlend(pObj);
    MRTexture* pTex = mr_BindShared(rd<SharedTexture*>(pObj, 0x1f0));
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    if (pTex)
    {
        float W = (float)pTex->m_nBaseWidth, H = (float)pTex->m_nBaseHeight;
        u0 = 2.0f / W;
        u1 = (W - 2.0f) / W;
        v0 = 2.0f / H;
        v1 = (H - 2.0f) / H;
    }
    float fFog = mr_FogFactor(pObj->m_Pos);
    rd_FogFactor(fFog);

    LTMatrix M;
    mr_ObjectMatrix(pObj, pObj->m_Pos, pObj->m_Scale, &M);
    // Into camera space (a really-close system is there already).
    const MRCamera& cam = g_Frame.m_Cam;
    float fNear = bClose ? cvf(CV_ReallyCloseNearZ) : cam.m_fNear;
    float fFar = cam.m_fFar;
    if (bClose)
        mr_BeginReallyClose(pObj);
    rd_PushMatrix();
    rd_LoadIdentity();
    float r = pObj->m_ColorR / 255.0f, gg = pObj->m_ColorG / 255.0f, b = pObj->m_ColorB / 255.0f;
    float a = (float)pObj->m_ColorA;
    // The camera's half extents per unit of depth: tan of the half field
    // of view (the 1.2 cull is in units of that).
    // A particle is square in pixels: its half size is 2 x its size in
    // camera x, and that times (W / H) (tanY / tanX) in camera y.
    float tx = cam.m_fTanX, ty = cam.m_fTanY;
    float fRatio = tx > 0.0f ? ((float)cam.m_nViewW / (float)cam.m_nViewH) * ty / tx : 1.0f;
    int32_t n = rd<int32_t>(pObj, 0x230);
    PSParticle* p = rd<PSParticle*>(pObj, 0x1d0);
    rd_Begin(RD_QUADS);
    for (int32_t i = 0; i < n && p; i++, p = p->m_pNext)
    {
        LTVector w = { M.m[0][0] * p->m_Pos.x + M.m[0][1] * p->m_Pos.y + M.m[0][2] * p->m_Pos.z + M.m[0][3],
                       M.m[1][0] * p->m_Pos.x + M.m[1][1] * p->m_Pos.y + M.m[1][2] * p->m_Pos.z + M.m[1][3],
                       M.m[2][0] * p->m_Pos.x + M.m[2][1] * p->m_Pos.y + M.m[2][2] * p->m_Pos.z + M.m[2][3] };
        LTVector c;
        if (bClose)
            c = w;
        else
        {
            LTVector d = { w.x - cam.m_Pos.x, w.y - cam.m_Pos.y, w.z - cam.m_Pos.z };
            c.x = cam.m_R[0][0] * d.x + cam.m_R[1][0] * d.y + cam.m_R[2][0] * d.z;
            c.y = cam.m_R[0][1] * d.x + cam.m_R[1][1] * d.y + cam.m_R[2][1] * d.z;
            c.z = cam.m_R[0][2] * d.x + cam.m_R[1][2] * d.y + cam.m_R[2][2] * d.z;
        }
        float fLimit = c.z * 1.2f;
        if (!(c.z > fNear && c.z < fFar && c.x / tx > -fLimit && c.x / tx < fLimit &&
              c.y / ty < fLimit && c.y / ty > -fLimit))
            continue;
        float hs = 2.0f * p->m_Size, hx = hs, hy = hs * fRatio;
        rd_Color4ub((uint8_t)mr_Round(r * p->m_Color.x), (uint8_t)mr_Round(gg * p->m_Color.y),
                    (uint8_t)mr_Round(b * p->m_Color.z), (uint8_t)mr_Round(a * p->m_Alpha));
        rd_TexCoord2f(u0, v0); rd_Vertex3f(c.x - hx, c.y + hy, c.z);
        rd_TexCoord2f(u1, v0); rd_Vertex3f(c.x + hx, c.y + hy, c.z);
        rd_TexCoord2f(u1, v1); rd_Vertex3f(c.x + hx, c.y - hy, c.z);
        rd_TexCoord2f(u0, v1); rd_Vertex3f(c.x - hx, c.y - hy, c.z);
    }
    rd_End();
    rd_PopMatrix();
    if (bClose)
        mr_EndReallyClose();
    mr_SetBaseTexture(NULL);
    mr_ForgetBindings();
    rd_PopAttrib();
}

// ---- poly grids (d3d_DrawPolyGrid) --------------------------------------------------------
//
// A vertex per height, coloured from the grid's colour table through the
// light ramp, alpha the table's times the object's; the texture panned and
// scaled; no fog. An environment map (the texture's linked texture, its
// mode at +0x2c; EnvMapPolyGrids) goes on unit 0, from the normals the
// heights give (with the retail sign of the first row and column), the
// texture over it on unit 1 (mode 1: added signed or modulated; mode 2:
// MODULATEALPHA_ADDCOLOR).

struct PGColor { float x, y, z, a; };

void mr_DrawPolyGrid(LTObject* pObj)
{
    if (!cvi(CV_DrawPolyGrids))
        return;
    const uint8_t* pData = rd<const uint8_t*>(pObj, 0x1b0);
    uint32_t W = rd<uint32_t>(pObj, 0x1f8), H = rd<uint32_t>(pObj, 0x1fc);
    const uint16_t* pIndices = rd<const uint16_t*>(pObj, 0x1b4);
    uint32_t nIndices = rd<uint32_t>(pObj, 0x1e8);
    const PGColor* pColors = (const PGColor*)((const uint8_t*)pObj + 0x200);
    if (!pData || W < 2 || H < 2)
        return;
    LTVector vDims = { W * 0.5f * pObj->m_Scale.x, pObj->m_Scale.y * 128.0f, H * 0.5f * pObj->m_Scale.z };
    float fRadius = sqrtf(vDims.x * vDims.x + vDims.y * vDims.y + vDims.z * vDims.z) + 1.0f;
    if (!mr_SphereVisible(pObj->m_Pos, fRadius))
        return;

    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_FOG | RD_ATTR_ENABLE | RD_ATTR_TEXTURE);
    rd_Enable(RD_BLEND);
    mr_ObjectBlend(pObj);
    rd_Disable(RD_FOG);

    // The texture: the sprite tracker's frame (+0x1bc, its frame entry at
    // +0x08) when there is a sprite (+0x1b8).
    SharedTexture* pShared = NULL;
    if (rd<void*>(pObj, 0x1b8))
    {
        SharedTexture** ppFrame = rd<SharedTexture**>(pObj, 0x1bc + 0x08);
        pShared = ppFrame ? *ppFrame : NULL;
    }
    MRTexture* pTex = NULL;
    if (pShared)
    {
        mr_TextureId(pShared);
        pTex = (MRTexture*)pShared->m_pRenderData;
    }
    MRTexture* pEnv = NULL;
    int nEnvMode = 0;
    if (pTex && pShared->m_pLinkedTexture && pShared->m_nLinkType && cvi(CV_EnvMapPolyGrids) &&
        g_RDCaps.bCombine && g_RDCaps.nTextureUnits >= 2)
    {
        mr_TextureId(pShared->m_pLinkedTexture);
        pEnv = (MRTexture*)pShared->m_pLinkedTexture->m_pRenderData;
        nEnvMode = (int)pShared->m_nLinkType;
    }
    mr_ForgetBindings();
    int nUnit = pEnv ? 1 : 0;
    if (pEnv)
    {
        mr_SetBaseTexture(pEnv);
        rd_TexEnv(RD_ENV_MODULATE);
        rd_ActiveTexture(1);
        RDCombine c = { nEnvMode == 2 ? RD_OP_MODALPHA_ADDCOLOR : cvi(CV_DetailTextureAdd) ? RD_OP_ADD_SIGNED : RD_OP_MODULATE,
                        RD_ARG_TEXTURE, RD_ARG_PREVIOUS, 1.0f,
                        nEnvMode == 2 ? RD_OP_ARG1 : RD_OP_REPLACE, RD_ARG_TEXTURE, RD_ARG_PREVIOUS };
        rd_TexCombine(c);
        rd_Enable(RD_TEXTURE_2D);
        rd_BindTexture(pTex->m_Id);
        rd_ActiveTexture(0);
    }
    else
    {
        mr_SetBaseTexture(pTex);
        rd_TexEnv(RD_ENV_MODULATE);
    }
    pObj->m_Flags |= FLAG_INTERNAL1;

    float fXPan = rd<float>(pObj, 0x1d4), fYPan = rd<float>(pObj, 0x1d8);
    float fXScale = rd<float>(pObj, 0x1dc), fYScale = rd<float>(pObj, 0x1e0);
    float fTU = pTex ? pTex->m_fUMul : 0.0f, fTV = pTex ? pTex->m_fVMul : 0.0f;
    float fUOff = fXPan * fTU, fVOff = fYPan * fTV;
    float fUScale = (fXScale != 0.0f ? 1.0f / fXScale : 0.0f) * pObj->m_Scale.x * fTU * W / (float)(int32_t)(W - 1);
    float fVScale = (fYScale != 0.0f ? 1.0f / fYScale : 0.0f) * fTV * pObj->m_Scale.z * H / (float)(int32_t)(H - 1);
    float fAlpha = pObj->m_ColorA / 255.0f;
    bool bUnsigned = (pObj->m_Flags & FLAG_UNSIGNED) != 0;

    // The grid's transform: cell space, centred, to the world.
    LTVector vScale = { (W + 1) * pObj->m_Scale.x / (float)(int32_t)W, pObj->m_Scale.y,
                        (H + 1) * pObj->m_Scale.z / (float)(int32_t)H };
    LTMatrix T;
    mr_ObjectMatrix(pObj, pObj->m_Pos, vScale, &T);
    float cx = -((float)(W - 1) * 0.5f), cy = bUnsigned ? -128.0f : 0.0f, cz = -((float)(H - 1) * 0.5f);
    LTVector vOrigin = { T.m[0][0] * cx + T.m[0][1] * cy + T.m[0][2] * cz + T.m[0][3],
                         T.m[1][0] * cx + T.m[1][1] * cy + T.m[1][2] * cz + T.m[1][3],
                         T.m[2][0] * cx + T.m[2][1] * cy + T.m[2][2] * cz + T.m[2][3] };
    LTVector vRight = { T.m[0][0], T.m[1][0], T.m[2][0] };
    LTVector vUp = { T.m[0][1], T.m[1][1], T.m[2][1] };
    LTVector vFwd = { T.m[0][2], T.m[1][2], T.m[2][2] };

    // The vertices.
    struct V { LTVector p; uint8_t c[4]; float u, v, eu, ev; };
    static V* s_pVerts;
    static uint32_t s_nVerts;
    uint32_t nVerts = W * H;
    if (nVerts > s_nVerts)
    {
        V* p = (V*)realloc(s_pVerts, nVerts * sizeof(V));
        if (!p)
        {
            rd_PopAttrib();
            return;
        }
        s_pVerts = p;
        s_nVerts = nVerts;
    }
    const uint8_t (*L)[256] = g_Frame.m_LightRamp;
    V* pv = s_pVerts;
    for (uint32_t y = 0; y < H; y++)
    {
        for (uint32_t x = 0; x < W; x++, pv++)
        {
            int h = bUnsigned ? pData[y * W + x] : (int8_t)pData[y * W + x];
            const PGColor& c = pColors[bUnsigned ? h : h + 128];
            pv->p.x = vOrigin.x + vRight.x * x + vFwd.x * y + vUp.x * h;
            pv->p.y = vOrigin.y + vRight.y * x + vFwd.y * y + vUp.y * h;
            pv->p.z = vOrigin.z + vRight.z * x + vFwd.z * y + vUp.z * h;
            pv->c[0] = L[0][(uint8_t)(int)c.x];
            pv->c[1] = L[1][(uint8_t)(int)c.y];
            pv->c[2] = L[2][(uint8_t)(int)c.z];
            pv->c[3] = (uint8_t)(int)(fAlpha * c.a);
            pv->u = (x + fUOff) * fUScale;
            pv->v = (y + fVOff) * fVScale;
        }
    }
    if (pEnv)
    {
        // d3d_SetupPolyGridEnvMap, with the retail signs (RETAIL_BUGS.md).
        float fUp = (vUp.x * vUp.x + vUp.y * vUp.y + vUp.z * vUp.z) / 128.0f;
        float fInvF = 1.0f / sqrtf(vFwd.x * vFwd.x + vFwd.y * vFwd.y + vFwd.z * vFwd.z);
        float fInvR = 1.0f / sqrtf(vRight.x * vRight.x + vRight.y * vRight.y + vRight.z * vRight.z);
        const int8_t* pH = (const int8_t*)pData;
        int iW = (int)W;
        for (uint32_t row = 0; row < H; row++)
        {
            uint32_t y = H - 1 - row;               // retail's count down
            for (uint32_t col = 0; col < W; col++)
            {
                uint32_t x = W - 1 - col;
                const int8_t* h = pH + row * W + col;
                float df, dr;
                if (y < H - 1)
                    df = y > 0 ? (float)(h[-iW] - h[iW]) : (float)(h[-iW] - h[0]);
                else
                    df = (float)(h[iW] - h[0]);
                if (x < W - 1)
                    dr = x > 0 ? (float)(h[-1] - h[1]) : (float)(h[-1] - h[0]);
                else
                    dr = (float)(h[1] - h[0]);
                df *= fInvF * fUp * 0.5f;
                dr *= fInvR * fUp * 0.5f;
                LTVector N = { vFwd.x * df + vRight.x * dr + vUp.x, vFwd.y * df + vRight.y * dr + vUp.y,
                               vFwd.z * df + vRight.z * dr + vUp.z };
                float l = sqrtf(N.x * N.x + N.y * N.y + N.z * N.z);
                if (l > 0.0f)
                {
                    N.x /= l; N.y /= l; N.z /= l;
                }
                V& vv = s_pVerts[row * W + col];
                mr_ReflectCoords(g_Frame.m_Cam.m_Pos, vv.p, N, &vv.eu, &vv.ev);
            }
        }
    }
    rd_Begin(RD_TRIANGLES);
    for (uint32_t i = 0; pIndices && i + 2 < nIndices + 2 && i < nIndices; i++)
    {
        uint16_t k = pIndices[i];
        if (k >= nVerts)
            k = 0;
        const V& v = s_pVerts[k];
        rd_Color4ub(v.c[0], v.c[1], v.c[2], v.c[3]);
        if (pEnv)
        {
            rd_TexCoord2f(v.eu, v.ev);
            rd_MultiTexCoord2f(1, v.u, v.v);
        }
        else
            rd_TexCoord2f(v.u, v.v);
        rd_Vertex3f(v.p.x, v.p.y, v.p.z);
    }
    rd_End();
    (void)nUnit;
    if (pEnv)
    {
        rd_ActiveTexture(1);
        rd_Disable(RD_TEXTURE_2D);
        rd_TexEnv(RD_ENV_MODULATE);
        rd_ActiveTexture(0);
    }
    mr_SetBaseTexture(NULL);
    mr_ForgetBindings();
    rd_PopAttrib();
}

// ---- line systems (d3d_DrawLineSystem) ---------------------------------------------------
//
// Untextured lines, the points' colours (0..1) times 255, alpha times the
// object's; no fog.

struct LSLinePoint { LTVector m_Pos; float r, g, b, a; };
struct LSLine { LSLinePoint m_Points[2]; void* m_pSystem; LSLine* m_pPrev; LSLine* m_pNext; };

void mr_DrawLineSystem(LTObject* pObj)
{
    if (!cvi(CV_DrawLineSystems))
        return;
    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_FOG | RD_ATTR_ENABLE | RD_ATTR_TEXTURE);
    rd_Disable(RD_FOG);
    mr_ForgetBindings();
    mr_SetBaseTexture(NULL);
    LTMatrix M;
    mr_ObjectMatrix(pObj, pObj->m_Pos, pObj->m_Scale, &M);
    rd_PushMatrix();
    mr_MultMatrix(M);
    float a = (float)pObj->m_ColorA;
    LSLine* pHead = (LSLine*)((uint8_t*)pObj + 0x1b8);
    rd_Begin(RD_LINES);
    int n = 0;
    for (LSLine* p = pHead->m_pNext; p && p != pHead && n < 100000; p = p->m_pNext, n++)
    {
        for (int k = 0; k < 2; k++)
        {
            const LSLinePoint& q = p->m_Points[k];
            rd_Color4ub((uint8_t)mr_Round(q.r * 255.0f), (uint8_t)mr_Round(q.g * 255.0f),
                        (uint8_t)mr_Round(q.b * 255.0f), (uint8_t)mr_Round(q.a * a));
            rd_Vertex3f(q.m_Pos.x, q.m_Pos.y, q.m_Pos.z);
        }
    }
    rd_End();
    rd_PopMatrix();
    mr_ForgetBindings();
    rd_PopAttrib();
}

// ---- canvases (draw_canvas.cpp CustomDrawMgr) ----------------------------------------------
//
// A canvas is the game's own drawing: its function gets an ILTCustomDraw
// (iltcustomdraw.h: five virtuals, thiscall) and draws world-space
// polygons through it, each vertex with its own fog in its specular
// alpha. A really-close canvas (or a primitive drawn with flag 0x40) is in
// camera space.

enum { LTRS_ALPHABLEND, LTRS_ZREAD, LTRS_ZWRITE, LTRS_SRCBLEND, LTRS_DESTBLEND, LTRS_TEXADDR,
       LTRS_COLOROP, LTRS_ALPHAOP, LTRS_COUNT };

struct MRCustomDraw;
struct MRCustomDrawVtbl {
    LTRESULT (__thiscall* DrawPrimitive)(MRCustomDraw* pThis, const void* pVerts, uint32_t nVerts, uint32_t nFlags);
    LTRESULT (__thiscall* SetState)(MRCustomDraw* pThis, uint32_t nState, uint32_t nVal);
    LTRESULT (__thiscall* GetState)(MRCustomDraw* pThis, uint32_t nState, uint32_t* pVal);
    LTRESULT (__thiscall* SetTexture)(MRCustomDraw* pThis, const char* pName);
    LTRESULT (__thiscall* GetTexelSize)(MRCustomDraw* pThis, float* pU, float* pV);
};
struct MRCustomDraw {
    const MRCustomDrawVtbl* m_pVtbl;
    uint32_t   m_States[LTRS_COUNT];
    MRTexture* m_pTex;
    bool       m_bReallyClose;
    LTObject*  m_pObj;
};

// ConvertBlend: SRCALPHA, INVSRCALPHA, SRCCOLOR, INVSRCCOLOR, ZERO, else ONE
// (LTBLEND_* 1..6: ZERO, ONE, SRCCOLOR, INVSRCCOLOR, SRCALPHA, INVSRCALPHA).
static RDBlend mr_LTBlend(uint32_t n)
{
    static const RDBlend blend[6] = { RD_ZERO, RD_ONE, RD_SRC_COLOR, RD_ONE_MINUS_SRC_COLOR,
                                      RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA };
    return n >= 1 && n <= 6 ? blend[n - 1] : RD_ONE;
}

// LTOP_SELECTTEXTURE 1, SELECTDIFFUSE 2, MODULATE 3, ADD 4, ADDSIGNED 5. The
// default alpha op is retail's SELECTARG1 (the texture), whatever the
// state says (CustomDrawMgr::SaveStates).
static void mr_ApplyCanvasOp(MRCustomDraw* pThis)
{
    static const RDCombineOp op[5] = { RD_OP_REPLACE, RD_OP_ARG1, RD_OP_MODULATE, RD_OP_ADD, RD_OP_ADD_SIGNED };
    uint32_t c = pThis->m_States[LTRS_COLOROP], a = pThis->m_States[LTRS_ALPHAOP];
    c = c >= 1 && c <= 5 ? c : 3;
    RDCombineOp eAlpha = a >= 1 && a <= 5 ? op[a - 1] : RD_OP_REPLACE;
    if (!pThis->m_pTex && eAlpha == RD_OP_REPLACE)
        eAlpha = RD_OP_ARG1;
    RDCombine cb = { op[c - 1], RD_ARG_TEXTURE, RD_ARG_PRIMARY, 1.0f, eAlpha, RD_ARG_TEXTURE, RD_ARG_PRIMARY };
    rd_TexCombine(cb);
}

static void mr_ApplyCanvasState(MRCustomDraw* pThis, uint32_t nState)
{
    uint32_t v = pThis->m_States[nState];
    switch (nState)
    {
    case LTRS_ALPHABLEND: if (v) rd_Enable(RD_BLEND); else rd_Disable(RD_BLEND); break;
    case LTRS_ZREAD:      if (v) rd_Enable(RD_DEPTH_TEST); else rd_Disable(RD_DEPTH_TEST); break;
    case LTRS_ZWRITE:     rd_DepthMask(v != 0); break;
    case LTRS_SRCBLEND:
    case LTRS_DESTBLEND:
        rd_BlendFunc(mr_LTBlend(pThis->m_States[LTRS_SRCBLEND]), mr_LTBlend(pThis->m_States[LTRS_DESTBLEND]));
        break;
    case LTRS_TEXADDR:
        if (pThis->m_pTex)
            rd_TexWrap(v != 2, v != 2);
        break;
    case LTRS_COLOROP:
    case LTRS_ALPHAOP:
        if (g_RDCaps.bCombine)
            mr_ApplyCanvasOp(pThis);
        break;
    }
}

static LTRESULT __thiscall cd_DrawPrimitive(MRCustomDraw* pThis, const void* pVerts, uint32_t nVerts, uint32_t nFlags)
{
    if (!pVerts || nVerts < 3 || nVerts > 256)
        return 1;                       // LT_ERROR
    bool bClose = pThis->m_bReallyClose || (nFlags & 0x40);
    if (bClose)
        mr_BeginReallyClose(pThis->m_pObj);
    // LTVertex: position, rhw, colour (b, g, r, a), specular (its alpha:
    // the fog), tu, tv -- 0x20 bytes.
    const uint8_t* v = (const uint8_t*)pVerts;
    rd_Begin(RD_TRIANGLE_FAN);
    for (uint32_t i = 0; i < nVerts; i++, v += 0x20)
    {
        rd_Color4ub(v[0x12], v[0x11], v[0x10], v[0x13]);
        rd_FogFactor(v[0x17] / 255.0f);
        rd_TexCoord2f(rd<float>(v, 0x18), rd<float>(v, 0x1c));
        if (bClose)
            rd_Vertex3f(rd<float>(v, 0), rd<float>(v, 4), rd<float>(v, 8) + cvf(CV_NearZ));
        else
            rd_Vertex3f(rd<float>(v, 0), rd<float>(v, 4), rd<float>(v, 8));
    }
    rd_End();
    if (bClose)
        mr_EndReallyClose();
    return 0;                           // LT_OK
}

static LTRESULT __thiscall cd_SetState(MRCustomDraw* pThis, uint32_t nState, uint32_t nVal)
{
    if (nState >= LTRS_COUNT)
        return 1;
    pThis->m_States[nState] = nVal;
    mr_ApplyCanvasState(pThis, nState);
    return 0;
}

static LTRESULT __thiscall cd_GetState(MRCustomDraw* pThis, uint32_t nState, uint32_t* pVal)
{
    if (nState >= LTRS_COUNT || !pVal)
        return 1;
    *pVal = pThis->m_States[nState];
    return 0;
}

static LTRESULT __thiscall cd_SetTexture(MRCustomDraw* pThis, const char* pName)
{
    SharedTexture* pShared = pName ? g.pStruct->GetSharedTexture(pName) : NULL;
    pThis->m_pTex = mr_BindShared(pShared);
    mr_ApplyCanvasState(pThis, LTRS_TEXADDR);
    mr_ApplyCanvasState(pThis, LTRS_COLOROP);
    return pName && !pThis->m_pTex ? 1 : 0;
}

static LTRESULT __thiscall cd_GetTexelSize(MRCustomDraw* pThis, float* pU, float* pV)
{
    MRTexture* pTex = pThis->m_pTex;
    if (pU) *pU = pTex && pTex->m_nBaseWidth ? 1.0f / pTex->m_nBaseWidth : 0.0f;
    if (pV) *pV = pTex && pTex->m_nBaseHeight ? 1.0f / pTex->m_nBaseHeight : 0.0f;
    return 0;
}

static const MRCustomDrawVtbl g_CustomDrawVtbl = {
    cd_DrawPrimitive, cd_SetState, cd_GetState, cd_SetTexture, cd_GetTexelSize,
};

typedef void (*CanvasDrawFn)(MRCustomDraw* pDraw, LTObject* hObj, void* pUser);

// CustomDrawMgr::DrawCanvas: the default states (SaveStates), the game's
// function, then everything put back.
void mr_DrawCanvas(LTObject* pObj)
{
    if (!cvi(CV_DrawCanvases))
        return;
    CanvasDrawFn fn = rd<CanvasDrawFn>(pObj, 0x1b0);
    if (!fn)
        return;
    rd_PushAttrib(RD_ATTR_ALL);
    if (pObj->m_Flags & FLAG_FOGDISABLE)
        rd_Disable(RD_FOG);
    MRCustomDraw draw;
    draw.m_pVtbl = &g_CustomDrawVtbl;
    draw.m_pTex = NULL;
    draw.m_bReallyClose = (pObj->m_Flags & FLAG_REALLYCLOSE) != 0;
    draw.m_pObj = pObj;
    static const uint32_t def[LTRS_COUNT] = { 0, 1, 1, 5, 6, 1, 3, 2 };
    memcpy(draw.m_States, def, sizeof(def));
    mr_ForgetBindings();
    mr_SetBaseTexture(NULL);
    for (uint32_t i = 0; i < LTRS_COUNT; i++)
        mr_ApplyCanvasState(&draw, i);
    fn(&draw, pObj, rd<void*>(pObj, 0x1b4));
    mr_SetBaseTexture(NULL);
    mr_ForgetBindings();
    rd_PopAttrib();
}

bool mr_CanvasIsSolid(LTObject* pObj)
{
    return (pObj->m_ClientFlags & CF_SOLIDCANVAS) != 0;
}
