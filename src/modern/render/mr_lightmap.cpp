// mr_lightmap.cpp -- the world's lightmaps as atlas textures.
//
// Every lightmap is a light animation's (ltworld.h, LightAnim): the static
// ones are "LightAnim_BASE". A poly names its animations at +0x0c. d3d.ren
// (src/engine/d3d/ph/lmpaging.cpp, lightanim.cpp, lmdynamic.cpp) packs the
// lightmaps into 64x64 pages when the world binds, and composes a poly's
// lightmap again when an animation that touches it changes
// (d3d_UpdateAnimatedLightmap): each layer (two frames blended, or a
// shadow-map mask lit by its light), the first copied, the rest added
// scaled by their brightness. Here the pages are bigger atlases (as
// nolfsrc's renderer has them), each lightmap with a one-texel border
// copied from its edge (so bilinear filtering never reaches a
// neighbour), and the vertices' lightmap coordinates are made at draw
// time from the same basis.

#include "mrscene.h"

#define ATLAS_SIZE 512

// ---- the engine's data -------------------------------------------------------------

LightAnim* mr_FindLightAnim(MainWorld* pWorld, const char* pName)
{
    LightAnim* pAnims = mw_LightAnims(pWorld);
    for (uint32_t i = 0; pAnims && i < pWorld->m_LightAnims.m_nElements; i++)
        if (!_stricmp(pAnims[i].m_Name, pName))
            return &pAnims[i];
    return NULL;
}

// d3d_DecompressLM32: 32-bit 0x00RRGGBB words; one with the top bit set is
// followed by a byte count of it.
static bool mr_DecompressLM32(const uint8_t* p, uint32_t nSize, uint32_t* pOut)
{
    if (!p)
        return false;
    const uint8_t* pEnd = p + nSize;
    int nLeft = LM_MAXTEXELS;
    while (p + 3 < pEnd)
    {
        uint32_t c = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
        p += 4;
        int n = 1;
        if (c & 0x80000000)
        {
            if (p >= pEnd)
                return false;
            n = *p++;
            c &= 0x7fffffff;
        }
        if ((nLeft -= n) < 0)
            return false;
        while (n--)
            *pOut++ = c;
    }
    return true;
}

// d3d_DecompressLMMask: byte runs, alternately 0 and 0xff, from 0.
static bool mr_DecompressLMMask(const uint8_t* p, uint32_t nSize, uint8_t* pOut)
{
    if (!p)
        return false;
    uint8_t* pEnd = pOut + LM_MAXTEXELS;
    int idx = 0;
    while (nSize--)
    {
        int n = *p++;
        if (pOut + n > pEnd)
            return false;
        memset(pOut, idx ? 0xff : 0x00, n);
        pOut += n;
        idx ^= 1;
    }
    return true;
}

// ---- the lightmap basis ------------------------------------------------------------------

// d3d.ren's g_LMBases[i].m_v1 (lmsurfaces.cpp): the basis index is in the
// poly's flags, bits 11..13.
static const LTVector g_LMUp[6] = {
    { 0, 0, -1 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, -1, 0 }, { 0, -1, 0 },
};

// d3d_GetLMAxes: right = N.Cross(v1), up = right.Cross(N), LithTech's
// Cross being a.Cross(b) = b x a; unnormalised, as the lightmaps were made.
void mr_LMVectors(const WorldPoly* pPoly, LTVector* pP, LTVector* pQ)
{
    uint32_t nType = (pPoly->m_Flags >> 11) & 7;
    const LTVector& A = g_LMUp[nType < 6 ? nType : 0];
    const LTVector& N = pPoly->m_pPlane->m_Normal;
    pP->x = A.y * N.z - A.z * N.y;
    pP->y = A.z * N.x - A.x * N.z;
    pP->z = A.x * N.y - A.y * N.x;
    pQ->x = N.y * pP->z - N.z * pP->y;
    pQ->y = N.z * pP->x - N.x * pP->z;
    pQ->z = N.x * pP->y - N.y * pP->x;
}

// A light in the poly's lightmap space (d3d_SetupLMLight): texel (0,0)'s
// world position and the steps of a texel in u and v.
static bool mr_TexelSteps(MainWorld* pWorld, const WorldPoly* pPoly, LTVector* pU, LTVector* pV)
{
    LTVector P, Q;
    mr_LMVectors(pPoly, &P, &Q);
    float grid = pWorld->m_LMGridSize;
    float pp = P.x * P.x + P.y * P.y + P.z * P.z, qq = Q.x * Q.x + Q.y * Q.y + Q.z * Q.z;
    if (pp <= 0.0f || qq <= 0.0f)
        return false;
    *pU = { P.x * grid / pp, P.y * grid / pp, P.z * grid / pp };
    *pV = { Q.x * grid / qq, Q.y * grid / qq, Q.z * grid / qq };
    return true;
}

// d3d_InitLMAnimRamp: 64 steps of 0..1 times LightSaturate, clamped at 1,
// in 16.16.
static int32_t s_Ramp[64];
static float   s_fRampSat = -1.0f;

static void mr_SetupRamp(void)
{
    if (s_fRampSat == cvf(CV_LightSaturate))
        return;
    s_fRampSat = cvf(CV_LightSaturate);
    for (int i = 0; i < 64; i++)
    {
        float f = (float)i * (1.0f / 63.0f) * s_fRampSat;
        if (f > 1.0f)
            f = 1.0f;
        s_Ramp[i] = (int32_t)(f * 65536.0f);
    }
}

static inline int mr_Clamp255(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

// ---- composing one poly's lightmap (d3d_UpdateAnimatedLightmap) ------------------------

// A light animation's layer (d3d_BlendLMFrames): frame A, frame B, or the
// two blended per channel, A + ((B - A) * blend >> 8), clamped at 255.
static bool mr_BlendLMFrames(const WorldPoly* pPoly, LightAnim* a, const LightAnimRef& r, uint32_t* pOut)
{
    if (r.m_iPoly >= a->m_nPolies)
        return false;
    int iA = a->m_iFrames[0], iB = a->m_iFrames[1], blend = a->m_Blend;
    if (iA < 0 || iA >= a->m_nFrames || iB < 0 || iB >= a->m_nFrames)
        return false;
    const LightAnimFrame* pFrame;
    if (iA == iB || blend <= 0)
        pFrame = &a->m_Frames[iA][r.m_iPoly];
    else if (blend >= 0xff)
        pFrame = &a->m_Frames[iB][r.m_iPoly];
    else
    {
        const LightAnimFrame& A = a->m_Frames[iA][r.m_iPoly];
        const LightAnimFrame& B = a->m_Frames[iB][r.m_iPoly];
        if (!A.m_nSize && !B.m_nSize)
            return false;
        uint32_t fb[LM_MAXTEXELS];
        if (!mr_DecompressLM32(A.m_pData, A.m_nSize, pOut) || !mr_DecompressLM32(B.m_pData, B.m_nSize, fb))
            return false;
        uint32_t n = pPoly->m_LMWidth * pPoly->m_LMHeight;
        for (uint32_t i = 0; i < n; i++)
        {
            uint32_t ca = pOut[i], cb = fb[i], c = 0;
            for (int s = 16; s >= 0; s -= 8)
            {
                int va = (ca >> s) & 0xff, vb = (cb >> s) & 0xff;
                int v = (((vb - va) * blend) >> 8) + va;
                c |= (uint32_t)(v > 0xff ? 0xff : v) << s;
            }
            pOut[i] = c;
        }
        return true;
    }
    if (!pFrame->m_nSize)
        return false;
    return mr_DecompressLM32(pFrame->m_pData, pFrame->m_nSize, pOut);
}

// A shadow-map animation's layer (d3d_DrawDynamicLMLayer): the mask (frame
// A, B or the two blended), lit by the animation's light through the ramp;
// black outside its radius.
static bool mr_ShadowMapLayer(MainWorld* pWorld, const WorldPoly* pPoly, LightAnim* a,
                              const LightAnimRef& r, uint32_t* pOut)
{
    int iA = a->m_iFrames[0], iB = a->m_iFrames[1], blend = a->m_Blend;
    if (r.m_iPoly >= a->m_nPolies || iA < 0 || iA >= a->m_nFrames || iB < 0 || iB >= a->m_nFrames)
        return false;
    uint32_t w = pPoly->m_LMWidth, h = pPoly->m_LMHeight, n = w * h;
    uint8_t mask[LM_MAXTEXELS];
    if (iA == iB || blend <= 0)
    {
        const LightAnimFrame& A = a->m_Frames[iA][r.m_iPoly];
        if (!A.m_nSize || !mr_DecompressLMMask(A.m_pData, A.m_nSize, mask))
            return false;
    }
    else if (blend >= 0xff)
    {
        const LightAnimFrame& B = a->m_Frames[iB][r.m_iPoly];
        if (!B.m_nSize || !mr_DecompressLMMask(B.m_pData, B.m_nSize, mask))
            return false;
    }
    else
    {
        const LightAnimFrame& A = a->m_Frames[iA][r.m_iPoly];
        const LightAnimFrame& B = a->m_Frames[iB][r.m_iPoly];
        if (!A.m_nSize && !B.m_nSize)
            return false;
        uint8_t mb[LM_MAXTEXELS];
        if (!mr_DecompressLMMask(A.m_pData, A.m_nSize, mask) || !mr_DecompressLMMask(B.m_pData, B.m_nSize, mb))
            return false;
        for (uint32_t i = 0; i < n; i++)
            mask[i] = (uint8_t)(mask[i] + (((mb[i] - mask[i]) * blend) >> 8));
    }

    LTVector su, sv;
    if (!mr_TexelSteps(pWorld, pPoly, &su, &sv))
        return false;
    float r2 = a->m_fLightRadius * a->m_fLightRadius;
    float fInv = r2 > 0.0f ? 1.0f / r2 : 0.0f;
    // The colour as 16.16 offsets from half (d3d_SetupLMLight): 2c - 255.
    int32_t col[3] = { (int32_t)a->m_vLightColor.x, (int32_t)a->m_vLightColor.y, (int32_t)a->m_vLightColor.z };
    for (int j = 0; j < 3; j++)
        col[j] = 2 * col[j] - 255;
    for (uint32_t y = 0; y < h; y++)
    {
        for (uint32_t x = 0; x < w; x++)
        {
            uint32_t i = y * w + x;
            float dx = pPoly->m_LMOrigin.x + x * su.x + y * sv.x - a->m_vLightPos.x;
            float dy = pPoly->m_LMOrigin.y + x * su.y + y * sv.y - a->m_vLightPos.y;
            float dz = pPoly->m_LMOrigin.z + x * su.z + y * sv.z - a->m_vLightPos.z;
            float d2 = dx * dx + dy * dy + dz * dz;
            uint32_t out = 0;
            if (mask[i] && d2 < r2)
            {
                int32_t nAtten = (s_Ramp[(int)((1.0f - fInv * d2) * 63.0f)] * mask[i]) >> 8;
                int c[3];
                for (int j = 0; j < 3; j++)
                    c[j] = mr_Clamp255((int)(((int64_t)nAtten * (col[j] << 16)) >> 32));
                out = (uint32_t)c[0] << 16 | (uint32_t)c[1] << 8 | (uint32_t)c[2];
            }
            pOut[i] = out;
        }
    }
    return true;
}

// The poly's whole lightmap: every usable layer (a valid frame, brightness
// at least 0.02), the first copied, the rest added scaled by their
// brightness, saturating. Black when it has none. False when the poly has
// no room for one.
static bool mr_ComposePolyLM(MainWorld* pWorld, const WorldPoly* pPoly, uint32_t* pAccum)
{
    uint32_t n = pPoly->m_LMWidth * pPoly->m_LMHeight;
    if (!n || n > LM_MAXTEXELS)
        return false;
    LightAnim* pAnims = mw_LightAnims(pWorld);
    uint32_t nAnims = pWorld->m_LightAnims.m_nElements;
    uint32_t layer[LM_MAXTEXELS];
    int nDone = 0;
    for (uint32_t i = 0; pPoly->m_pLightAnims && i < pPoly->m_nLightAnims; i++)
    {
        const LightAnimRef& r = pPoly->m_pLightAnims[i];
        if (r.m_iAnim >= nAnims)
            continue;
        LightAnim* a = &pAnims[r.m_iAnim];
        if (a->m_iFrames[0] == -1 || a->m_fBrightness < 0.02f)
            continue;
        bool bOK = a->m_bShadowMap ? mr_ShadowMapLayer(pWorld, pPoly, a, r, layer)
                                   : mr_BlendLMFrames(pPoly, a, r, layer);
        if (!bOK)
            continue;
        if (!nDone)
            memcpy(pAccum, layer, n * 4);
        else
        {
            int s = (int)(a->m_fBrightness * 255.0f) & 0xff;
            for (uint32_t t = 0; t < n; t++)
            {
                uint32_t src = layer[t], dst = pAccum[t], c = 0;
                for (int sh = 16; sh >= 0; sh -= 8)
                {
                    int v = (((src >> sh) & 0xff) * s) / 255 + ((dst >> sh) & 0xff);
                    c |= (uint32_t)(v > 0xff ? 0xff : v) << sh;
                }
                pAccum[t] = c;
            }
        }
        nDone++;
    }
    if (!nDone)
        memset(pAccum, 0, n * 4);
    return true;
}

// Write a poly's texels into its atlas, with the one-texel border.
static void mr_StorePolyLM(const MRPolyLM* pLM, uint32_t w, uint32_t h, const uint32_t* pTexels)
{
    MRLMAtlas* pAtlas = pLM->m_pAtlas;
    for (int y = -1; y <= (int)h; y++)
    {
        int sy = y < 0 ? 0 : y >= (int)h ? (int)h - 1 : y;
        uint8_t* pRow = pAtlas->m_pRGB + ((pLM->m_Y + y) * pAtlas->m_nSize + pLM->m_X - 1) * 3;
        for (int x = -1; x <= (int)w; x++)
        {
            int sx = x < 0 ? 0 : x >= (int)w ? (int)w - 1 : x;
            uint32_t c = pTexels[sy * w + sx];
            pRow[0] = (uint8_t)(c >> 16);
            pRow[1] = (uint8_t)(c >> 8);
            pRow[2] = (uint8_t)c;
            pRow += 3;
        }
    }
    pAtlas->m_bDirty = true;
    if (pLM->m_Y - 1u < pAtlas->m_DirtyY0)
        pAtlas->m_DirtyY0 = pLM->m_Y - 1;
    if (pLM->m_Y + h + 1 > pAtlas->m_DirtyY1)
        pAtlas->m_DirtyY1 = pLM->m_Y + h + 1;
}

// ---- packing --------------------------------------------------------------------------

static MRLMAtlas* mr_NewAtlas(MRContext* pContext)
{
    MRLMAtlas* pAtlas = (MRLMAtlas*)calloc(1, sizeof(MRLMAtlas));
    if (!pAtlas)
        return NULL;
    pAtlas->m_nSize = ATLAS_SIZE;
    pAtlas->m_pRGB = (uint8_t*)calloc(ATLAS_SIZE * ATLAS_SIZE, 3);
    if (!pAtlas->m_pRGB)
    {
        free(pAtlas);
        return NULL;
    }
    pAtlas->m_pNext = pContext->m_pAtlases;
    pContext->m_pAtlases = pAtlas;
    return pAtlas;
}

// Shelf packing: rows of lightmaps, left to right, a new row when one is
// full, a new atlas when the rows are.
static bool mr_Place(MRContext* pContext, uint32_t w, uint32_t h, MRPolyLM* pLM)
{
    uint32_t bw = w + 2, bh = h + 2;
    MRLMAtlas* pAtlas = pContext->m_pAtlases;
    if (pAtlas && pAtlas->m_ShelfX + bw > pAtlas->m_nSize)
    {
        pAtlas->m_ShelfY += pAtlas->m_ShelfH;
        pAtlas->m_ShelfX = 0;
        pAtlas->m_ShelfH = 0;
    }
    if (!pAtlas || pAtlas->m_ShelfY + bh > pAtlas->m_nSize)
    {
        pAtlas = mr_NewAtlas(pContext);
        if (!pAtlas)
            return false;
    }
    pLM->m_pAtlas = pAtlas;
    pLM->m_X = (uint16_t)(pAtlas->m_ShelfX + 1);
    pLM->m_Y = (uint16_t)(pAtlas->m_ShelfY + 1);
    pAtlas->m_ShelfX += bw;
    if (bh > pAtlas->m_ShelfH)
        pAtlas->m_ShelfH = bh;
    return true;
}

static void mr_UploadAtlases(MRContext* pContext)
{
    for (MRLMAtlas* pAtlas = pContext->m_pAtlases; pAtlas; pAtlas = pAtlas->m_pNext)
    {
        if (!pAtlas->m_Id)
        {
            pAtlas->m_Id = rd_GenTexture();
            rd_BindTexture(pAtlas->m_Id);
            rd_TexFilter(RD_LINEAR, RD_LINEAR);
            rd_TexWrap(false, false);
            rd_TexMaxLevel(0);
            rd_TexImage(0, RD_FMT_RGB8, pAtlas->m_nSize, pAtlas->m_nSize, RD_PIX_RGB, pAtlas->m_pRGB);
        }
        else if (pAtlas->m_bDirty && pAtlas->m_DirtyY0 < pAtlas->m_DirtyY1)
        {
            // Whole rows, so the pixels need no row length of their own.
            uint32_t y0 = pAtlas->m_DirtyY0, y1 = pAtlas->m_DirtyY1;
            if (y1 > pAtlas->m_nSize)
                y1 = pAtlas->m_nSize;
            rd_BindTexture(pAtlas->m_Id);
            rd_TexSubImage(0, 0, y0, pAtlas->m_nSize, y1 - y0, RD_PIX_RGB,
                           pAtlas->m_pRGB + y0 * pAtlas->m_nSize * 3);
        }
        pAtlas->m_bDirty = false;
        pAtlas->m_DirtyY0 = pAtlas->m_nSize;
        pAtlas->m_DirtyY1 = 0;
    }
}

// Every poly of every BSP of the world.
template <typename F> static void mr_ForEachPoly(MainWorld* pWorld, F fn)
{
    for (uint32_t i = 0; i < pWorld->m_Worlds.m_nElements; i++)
    {
        WorldData* pData = mw_World(pWorld, i);
        WorldBsp* pBsp = pData ? pData->m_pWorldBsp : NULL;
        for (uint32_t p = 0; pBsp && p < pBsp->m_nPolies; p++)
            if (pBsp->m_Polies[p])
                fn(pBsp->m_Polies[p]);
    }
}

// d3d_CreateLightmapPages: the world needs LightAnim_BASE; every poly of a
// lightmapped surface with a lightmap size gets a place, composed and
// uploaded.
bool mr_BuildLightmaps(MRContext* pContext)
{
    MainWorld* pWorld = pContext->m_pWorld;
    if (!pWorld || !g.bContext)
        return false;
    uint32_t nPolies = 0;
    mr_ForEachPoly(pWorld, [&](WorldPoly* p) { p->m_pLMPage = NULL; nPolies++; });
    LightAnim* pBase = mr_FindLightAnim(pWorld, "LightAnim_BASE");
    mr_Debug(1, "world: %u polys, flags %x, %u light animations, base %p (%u frames)", nPolies, pWorld->m_Flags,
             pWorld->m_LightAnims.m_nElements, (void*)pBase, pBase ? pBase->m_nFrames : 0);
    if (!(pWorld->m_Flags & MAINWORLD_HASLIGHTMAPS))
        return false;
    if (!pBase || pBase->m_nFrames < 1)
        return false;
    mr_SetupRamp();

    pContext->m_pPolyLMs = (MRPolyLM*)calloc(nPolies ? nPolies : 1, sizeof(MRPolyLM));
    if (!pContext->m_pPolyLMs)
        return false;
    pContext->m_nContextGen = g.nContextGen;

    uint32_t texels[LM_MAXTEXELS];
    uint32_t nPlaced = 0;
    bool bFull = false;
    mr_ForEachPoly(pWorld, [&](WorldPoly* pPoly) {
        if (bFull || pPoly->m_pLMPage || !pPoly->m_pSurface || !pPoly->m_pPlane ||
            !(pPoly->m_pSurface->m_Flags & SURF_LIGHTMAP))
            return;
        uint32_t w = pPoly->m_LMWidth, h = pPoly->m_LMHeight;
        if (!w || !h || w * h > LM_MAXTEXELS)
            return;
        MRPolyLM* pLM = &pContext->m_pPolyLMs[nPlaced];
        if (!mr_Place(pContext, w, h, pLM))
        {
            bFull = true;
            return;
        }
        if (!mr_ComposePolyLM(pWorld, pPoly, texels))
            return;
        mr_StorePolyLM(pLM, w, h, texels);
        pPoly->m_pLMPage = pLM;
        pPoly->m_Flags &= ~WPF_LMCHANGED;
        nPlaced++;
    });
    pContext->m_nPolyLMs = nPlaced;
    mr_UploadAtlases(pContext);

    int nAtlases = 0;
    for (MRLMAtlas* p = pContext->m_pAtlases; p; p = p->m_pNext)
        nAtlases++;
    mr_Debug(1, "%u lightmaps in %d atlases of %d", nPlaced, nAtlases, ATLAS_SIZE);
    return nPlaced > 0;
}

void mr_FreeLightmaps(MRContext* pContext)
{
    MainWorld* pWorld = pContext->m_pWorld;
    if (pWorld)
        mr_ForEachPoly(pWorld, [](WorldPoly* p) { p->m_pLMPage = NULL; });
    while (pContext->m_pAtlases)
    {
        MRLMAtlas* pAtlas = pContext->m_pAtlases;
        pContext->m_pAtlases = pAtlas->m_pNext;
        if (pAtlas->m_Id && g.bContext && pContext->m_nContextGen == g.nContextGen)
            rd_DeleteTexture(pAtlas->m_Id);
        free(pAtlas->m_pRGB);
        free(pAtlas);
    }
    free(pContext->m_pPolyLMs);
    pContext->m_pPolyLMs = NULL;
    free(pContext->m_pLitPolys);
    pContext->m_pLitPolys = NULL;
    pContext->m_nLitPolys = pContext->m_nLitAlloc = 0;
    pContext->m_nPolyLMs = 0;
}

// The polys lit last frame get their plain lightmaps back; those still lit
// are lit again as they draw.
static void mr_RestoreLitPolys(MRContext* pContext)
{
    uint32_t texels[LM_MAXTEXELS];
    for (uint32_t i = 0; i < pContext->m_nLitPolys; i++)
    {
        WorldPoly* pPoly = pContext->m_pLitPolys[i];
        MRPolyLM* pLM = (MRPolyLM*)pPoly->m_pLMPage;
        if (!pLM)
            continue;
        pLM->m_bLit = false;
        if (mr_ComposePolyLM(pContext->m_pWorld, pPoly, texels))
            mr_StorePolyLM(pLM, pPoly->m_LMWidth, pPoly->m_LMHeight, texels);
    }
    pContext->m_nLitPolys = 0;
}

// d3d_DrawAnimatedLightmap (and d3d_LightTexels32): the poly's lightmap
// with the dynamic lights added, per texel at its world position: through
// the ramp, the colour as 2c - 255 (a colour below 128 darkens); the flat
// colour for FastLight or a light of the world only. Over its atlas texels
// until mr_RestoreLitPolys.
void mr_LightPolyLM(MRContext* pContext, WorldPoly* pPoly, const MRWorldLight* pLights,
                    uint32_t nLights)
{
    MRPolyLM* pLM = (MRPolyLM*)pPoly->m_pLMPage;
    uint32_t texels[LM_MAXTEXELS];
    if (!pLM || !nLights || pLM->m_bLit || !mr_ComposePolyLM(pContext->m_pWorld, pPoly, texels))
        return;
    if (pContext->m_nLitPolys == pContext->m_nLitAlloc)
    {
        uint32_t n = pContext->m_nLitAlloc ? pContext->m_nLitAlloc * 2 : 256;
        WorldPoly** p = (WorldPoly**)realloc(pContext->m_pLitPolys, n * sizeof(WorldPoly*));
        if (!p)
            return;
        pContext->m_pLitPolys = p;
        pContext->m_nLitAlloc = n;
    }
    pContext->m_pLitPolys[pContext->m_nLitPolys++] = pPoly;
    pLM->m_bLit = true;

    LTVector su, sv;
    if (!mr_TexelSteps(pContext->m_pWorld, pPoly, &su, &sv))
        return;
    mr_SetupRamp();
    uint32_t w = pPoly->m_LMWidth, h = pPoly->m_LMHeight;
    for (uint32_t l = 0; l < nLights; l++)
    {
        const MRWorldLight& L = pLights[l];
        float r2 = L.m_fRadius * L.m_fRadius;
        if (r2 <= 0.0f)
            continue;
        float fInv = 1.0f / r2;
        int32_t col[3];
        for (int j = 0; j < 3; j++)
            col[j] = 2 * (int32_t)L.m_Color[j] - 255;
        for (uint32_t y = 0; y < h; y++)
        {
            for (uint32_t x = 0; x < w; x++)
            {
                float dx = pPoly->m_LMOrigin.x + x * su.x + y * sv.x - L.m_Pos.x;
                float dy = pPoly->m_LMOrigin.y + x * su.y + y * sv.y - L.m_Pos.y;
                float dz = pPoly->m_LMOrigin.z + x * su.z + y * sv.z - L.m_Pos.z;
                float d2 = dx * dx + dy * dy + dz * dz;
                if (d2 >= r2)
                    continue;
                uint32_t& t = texels[y * w + x];
                if (L.m_bSolid)
                {
                    t = (uint32_t)L.m_Color[0] << 16 | (uint32_t)L.m_Color[1] << 8 | L.m_Color[2];
                    continue;
                }
                int32_t nAtten = s_Ramp[(int)((1.0f - d2 * fInv) * 63.0f)];
                uint32_t c = 0;
                for (int j = 0; j < 3; j++)
                {
                    int sh = 16 - 8 * j;
                    int v = (int)((t >> sh) & 0xff) + (int)(((int64_t)nAtten * (col[j] << 16)) >> 32);
                    c |= (uint32_t)mr_Clamp255(v) << sh;
                }
                t = c;
            }
        }
    }
    mr_StorePolyLM(pLM, w, h, texels);
}

void mr_UploadLightmaps(MRContext* pContext)
{
    mr_UploadAtlases(pContext);
}

// The engine marks the polys of a light animation it changed (poly flag
// 0x8000; d3d.ren's d3d_CheckLightmap): compose those again.
void mr_UpdateLightmaps(MRContext* pContext)
{
    MainWorld* pWorld = pContext->m_pWorld;
    uint32_t texels[LM_MAXTEXELS];
    mr_SetupRamp();
    mr_RestoreLitPolys(pContext);
    if (!cvi(CV_LMAnim))
        return;
    mr_ForEachPoly(pWorld, [&](WorldPoly* pPoly) {
        if (!(pPoly->m_Flags & WPF_LMCHANGED))
            return;
        pPoly->m_Flags &= ~WPF_LMCHANGED;
        MRPolyLM* pLM = (MRPolyLM*)pPoly->m_pLMPage;
        if (pLM && mr_ComposePolyLM(pWorld, pPoly, texels))
            mr_StorePolyLM(pLM, pPoly->m_LMWidth, pPoly->m_LMHeight, texels);
    });
}
