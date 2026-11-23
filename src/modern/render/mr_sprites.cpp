// mr_sprites.cpp -- sprites (OT_SPRITE), the world's light grid and the
// dynamic lights' add, which light sprites and models.
//
// d3d.ren's drawsprite.cpp: the engine advances a sprite's animation and
// the renderer draws its current frame, a texture. A flat sprite is a
// camera-facing quad half the texture's size times the object's scale on
// each side (glow sprites grow with distance); a rotatable sprite is the
// quad in the object's own rotation and scale, from whichever side the
// camera sees. The colour is the object's times the world colour, plus the
// dynamic lights, times the light grid (unless FLAG_NOLIGHT). Sprites are
// translucent: drawn far to near after everything solid, blended by
// their Flags2 (d3d_GetBlendStates).

#include <math.h>

#include "mrobjects.h"

// ---- the light grid (drawmodel.cpp d3d_GetLightGridColor) ---------------------------

struct MRLightTable {                   // MainWorld+0xfc
    const uint8_t* m_pData;             // 0x00 B, G, R, pad
    uint32_t m_04;
    int32_t  m_nYStride;                // 0x08
    uint32_t m_0c, m_10;
    int32_t  m_nMax[3];                 // 0x14
    int32_t  m_nZStride;                // 0x20
    LTVector m_vCellSize;               // 0x24
    LTVector m_vScale;                  // 0x30
    LTVector m_vBase;                   // 0x3c
};
static_assert(sizeof(MRLightTable) == 0x48, "MRLightTable");

// The eight cells around the point, blended along y, then x, then z;
// 0..255 a component, in pOut.
bool mr_LightGridSample(const LTVector& pos, float* pOut)
{
    pOut[0] = pOut[1] = pOut[2] = 0.0f;
    MainWorld* pWorld = g_Frame.m_pContext ? g_Frame.m_pContext->m_pWorld : NULL;
    if (!pWorld)
        return false;
    const MRLightTable* t = (const MRLightTable*)pWorld->m_LightTable;
    if (!t->m_pData)
        return false;
    float v[3] = { (pos.x - t->m_vBase.x) * t->m_vScale.x, (pos.y - t->m_vBase.y) * t->m_vScale.y,
                   (pos.z - t->m_vBase.z) * t->m_vScale.z };
    int idx[3];
    float inv[3];
    for (int i = 0; i < 3; i++)
    {
        idx[i] = (int)v[i];
        if (idx[i] < 0)
            idx[i] = 0;
        else if (idx[i] > t->m_nMax[i])
            idx[i] = t->m_nMax[i];
        if (idx[i] == t->m_nMax[i])
        {
            v[i] = 0.0f;
            inv[i] = 1.0f;
            idx[i]--;
        }
        else
        {
            v[i] -= floorf(v[i]);
            inv[i] = 1.0f - v[i];
        }
    }
    const uint8_t* c = t->m_pData + (idx[0] + t->m_nYStride * idx[1] + t->m_nZStride * idx[2]) * 4;
    int dy = t->m_nYStride * 4, dz = t->m_nZStride * 4;
    for (int ch = 0; ch < 3; ch++)
    {
        int b = 2 - ch;                 // R at +2
        float s0 = c[dy + b], s1 = c[dy + 4 + b], s2 = c[b], s3 = c[4 + b];
        float s4 = c[dz + dy + b], s5 = c[dz + dy + 4 + b], s6 = c[dz + b], s7 = c[dz + 4 + b];
        float n = inv[0] * (inv[1] * s2 + v[1] * s0) + v[0] * (inv[1] * s3 + v[1] * s1);
        float f = inv[0] * (inv[1] * s6 + v[1] * s4) + v[0] * (inv[1] * s7 + v[1] * s5);
        pOut[ch] = inv[2] * n + v[2] * f;
    }
    return true;
}

// d3d_GetDynamicLightAdd: each visible light that lights objects and
// reaches the point adds (2c - 255) x (1 - d/r) x 0.7 (LightModels).
void mr_DynamicLightAdd(const LTVector& pos, float* pOut)
{
    pOut[0] = pOut[1] = pOut[2] = 0.0f;
    if (!cvi(CV_LightModels))
        return;
    MRObjList* pLights = mr_ObjList(VS_LIGHTS);
    for (uint32_t i = 0; i < pLights->m_nObjects; i++)
    {
        LTObject* L = pLights->m_pObjects[i];
        if (L->m_Flags & FLAG_ONLYLIGHTWORLD)
            continue;
        float r = obj_LightRadius(L);
        float dx = L->m_Pos.x - pos.x, dy = L->m_Pos.y - pos.y, dz = L->m_Pos.z - pos.z;
        float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 >= r * r)
            continue;
        float s = (1.0f - sqrtf(d2) / r) * 0.7f;
        pOut[0] += (2.0f * L->m_ColorR - 255.0f) * s;
        pOut[1] += (2.0f * L->m_ColorG - 255.0f) * s;
        pOut[2] += (2.0f * L->m_ColorB - 255.0f) * s;
    }
}

static inline uint8_t mr_Byte(float f)
{
    f = f < 0.0f ? 0.0f : f > 255.0f ? 255.0f : f;
    return (uint8_t)(int)(f + 0.5f);
}

// The sprite's colour (drawsprite.cpp): its own times the world colour,
// plus the dynamic lights, times the light grid; its alpha.
static void mr_SpriteColor(LTObject* pObj, uint8_t* pRGBA)
{
    const LTVector& wc = g_Frame.m_pScene->m_WorldColor;
    float c[3] = { pObj->m_ColorR * wc.x, pObj->m_ColorG * wc.y, pObj->m_ColorB * wc.z };
    float grid[3];
    if (!(pObj->m_Flags & FLAG_NOLIGHT) && mr_LightGridSample(pObj->m_Pos, grid))
    {
        float add[3];
        mr_DynamicLightAdd(pObj->m_Pos, add);
        for (int i = 0; i < 3; i++)
            c[i] = (c[i] + add[i]) * (grid[i] * (1.0f / 255.0f));
    }
    for (int i = 0; i < 3; i++)
        pRGBA[i] = mr_Byte(c[i]);
    pRGBA[3] = pObj->m_ColorA;
}

// d3d_GetBlendStates: additive (ONE/ONE, fog towards black), multiply
// (ZERO/SRCCOLOR, fog towards white), Primal Hunt's additive alpha
// (SRCALPHA/ONE), or blended by alpha; no fog with FLAG_FOGDISABLE but
// for models.
void mr_ObjectBlend(LTObject* pObj)
{
    if (pObj->m_Flags2 & FLAG2_ADDITIVE)
    {
        rd_BlendFunc(RD_ONE, RD_ONE);
        rd_FogColor(0, 0, 0);
    }
    else if (pObj->m_Flags2 & FLAG2_MULTIPLY)
    {
        rd_BlendFunc(RD_ZERO, RD_SRC_COLOR);
        rd_FogColor(1, 1, 1);
    }
    else
        rd_BlendFunc(RD_SRC_ALPHA, RD_ONE_MINUS_SRC_ALPHA);
#ifdef PRIMAL_HUNT
    if (pObj->m_Flags2 & FLAG2_ADDITIVEALPHA)
        rd_BlendFunc(RD_SRC_ALPHA, RD_ONE);
#endif
    if ((pObj->m_Flags & FLAG_FOGDISABLE) && pObj->m_ObjectType != OT_MODEL)
        rd_Disable(RD_FOG);
}

// The sprite's current frame (its SpriteTracker at +0x1b0: the
// animation at +0x1b4, the frame entry at +0x1b8, whose first field is
// the texture).
SharedTexture* mr_SpriteFrame(LTObject* pObj)
{
    const uint8_t* p = (const uint8_t*)pObj;
    if (!*(void* const*)(p + 0x1b4))
        return NULL;
    SharedTexture** ppFrame = *(SharedTexture** const*)(p + 0x1b8);
    return ppFrame ? *ppFrame : NULL;
}

#define SPRITE_POSITION_ZBIAS (-20.0f)

// Draw one sprite with the camera currently loaded (the main camera, or
// the sky's): pCam gives its position and axes.
static void mr_DrawSpriteWith(LTObject* pObj, const MRCamera* pCam)
{
    SharedTexture* pFrame = mr_SpriteFrame(pObj);
    if (!pFrame)
        return;
    mr_TextureId(pFrame);
    MRTexture* pTex = (MRTexture*)pFrame->m_pRenderData;
    if (!pTex)
        return;

    rd_PushAttrib(RD_ATTR_COLOR | RD_ATTR_FOG | RD_ATTR_ENABLE | RD_ATTR_TEXTURE);
    rd_Enable(RD_BLEND);
    mr_ObjectBlend(pObj);
    rd_Enable(RD_TEXTURE_2D);
    rd_BindTexture(pTex->m_Id);
    rd_TexEnv(RD_ENV_MODULATE);
    mr_ForgetBindings();

    uint8_t rgba[4];
    mr_SpriteColor(pObj, rgba);
    float fFog = mr_FogFactor(pObj->m_Pos);

    // Two texels in from every edge.
    float W = (float)pTex->m_nBaseWidth, H = (float)pTex->m_nBaseHeight;
    float u0 = 2.0f / W, u1 = (W - 2.0f) / W;
    float v0 = 2.0f / H, v1 = (H - 2.0f) / H;
    bool bClose = (pObj->m_Flags & FLAG_REALLYCLOSE) != 0;

    if (pObj->m_Flags & FLAG_ROTATEABLESPRITE)
    {
        // In the object's orientation and scale: corners (+-W, +-H, 0),
        // from whichever side the camera sees (the order only).
        float R[3][3];
        LTRotation q = pObj->m_Rotation;
        float* qc = &q.x;
        for (int i = 0; i < 4; i++)
            if (qc[i] > 100000.0f || qc[i] < -100000.0f)
                qc[i] = i == 3 ? 1.0f : 0.0f;
        mr_RotationMatrix(q, R);
        LTMatrix M;
        for (int r = 0; r < 3; r++)
        {
            M.m[r][0] = R[r][0] * pObj->m_Scale.x;
            M.m[r][1] = R[r][1] * pObj->m_Scale.y;
            M.m[r][2] = R[r][2] * pObj->m_Scale.z;
        }
        M.m[0][3] = pObj->m_Pos.x;
        M.m[1][3] = pObj->m_Pos.y;
        M.m[2][3] = pObj->m_Pos.z;
        M.m[3][0] = M.m[3][1] = M.m[3][2] = 0.0f;
        M.m[3][3] = 1.0f;
        rd_PushMatrix();
        if (bClose)
            rd_LoadIdentity();
        mr_MultMatrix(M);
        rd_Color4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
        rd_FogFactor(fFog);
        rd_Begin(RD_QUADS);
        rd_TexCoord2f(u0, v0); rd_Vertex3f(W, H, 0);
        rd_TexCoord2f(u1, v0); rd_Vertex3f(-W, H, 0);
        rd_TexCoord2f(u1, v1); rd_Vertex3f(-W, -H, 0);
        rd_TexCoord2f(u0, v1); rd_Vertex3f(W, -H, 0);
        rd_End();
        rd_PopMatrix();
    }
    else
    {
        // A billboard in camera space. A really-close object's position is
        // in camera space already.
        LTVector c;
        if (bClose)
            c = pObj->m_Pos;
        else
        {
            LTVector d = { pObj->m_Pos.x - pCam->m_Pos.x, pObj->m_Pos.y - pCam->m_Pos.y,
                           pObj->m_Pos.z - pCam->m_Pos.z };
            const float (*R)[3] = pCam->m_R;
            c.x = (R[0][0] * d.x + R[1][0] * d.y + R[2][0] * d.z) * pCam->m_fScale;
            c.y = (R[0][1] * d.x + R[1][1] * d.y + R[2][1] * d.z) * pCam->m_fScale;
            c.z = (R[0][2] * d.x + R[1][2] * d.y + R[2][2] * d.z) * pCam->m_fScale;
        }
        float fNearZ = bClose ? cvf(CV_ReallyCloseNearZ) : cvf(CV_NearZ);
        if (c.z > fNearZ)
        {
            float hw = W * pObj->m_Scale.x, hh = H * pObj->m_Scale.y;
            if (pObj->m_Flags & FLAG_GLOWSPRITE)
            {
                float k = (c.z - 10.0f) / 490.0f;
                k = k < 0.0f ? 0.0f : k > 1.0f ? 1.0f : k;
                float s = 0.1f + 1.9f * k;
                hw *= s;
                hh *= s;
            }
            // The depth moved, the place on the screen kept: the quad
            // scaled towards the eye. Sprite bias: 20 units nearer, never
            // nearer than the near plane; a really-close sprite NearZ
            // farther (d3d_ProjectBiasedVert).
            float k = 1.0f;
            if (pObj->m_Flags & FLAG_SPRITEBIAS)
            {
                float fBias = SPRITE_POSITION_ZBIAS;
                if (fBias + c.z < cvf(CV_NearZ))
                    fBias = cvf(CV_NearZ) - c.z;
                k = (c.z + fBias) / c.z;
            }
            else if (bClose)
                k = (c.z + cvf(CV_NearZ)) / c.z;
            float tu[4] = { u0, u1, u1, u0 }, tv[4] = { v0, v0, v1, v1 };
            if (pObj->m_Flags2 & FLAG2_SPRITE_TROTATE)
            {
                // The texture turned about its centre by the object's
                // rotation, clamped so the corners do not wrap.
                float R[3][3];
                mr_RotationMatrix(pObj->m_Rotation, R);
                float uc = (u0 + u1) * 0.5f, vc = (v0 + v1) * 0.5f;
                for (int i = 0; i < 4; i++)
                {
                    float du = tu[i] - uc, dv = tv[i] - vc;
                    tu[i] = du * R[0][0] + dv * R[0][1] + uc;
                    tv[i] = du * R[1][0] + dv * R[1][1] + vc;
                }
                rd_TexWrap(false, false);
            }
            float x[4] = { c.x - hw, c.x + hw, c.x + hw, c.x - hw };
            float y[4] = { c.y + hh, c.y + hh, c.y - hh, c.y - hh };
            rd_PushMatrix();
            rd_LoadIdentity();
            rd_Color4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
            rd_FogFactor(fFog);
            rd_Begin(RD_QUADS);
            for (int i = 0; i < 4; i++)
            {
                rd_TexCoord2f(tu[i], tv[i]);
                rd_Vertex3f(x[i] * k, y[i] * k, c.z * k);
            }
            rd_End();
            rd_PopMatrix();
            if (pObj->m_Flags2 & FLAG2_SPRITE_TROTATE)
                rd_TexWrap(true, true);
        }
    }
    rd_PopAttrib();
}

void mr_DrawSprite(LTObject* pObj)
{
    bool bClose = (pObj->m_Flags & FLAG_REALLYCLOSE) != 0;
    if (bClose)
        mr_BeginReallyClose(pObj);
    mr_DrawSpriteWith(pObj, &g_Frame.m_Cam);
    if (bClose)
        mr_EndReallyClose();
}

// d3d_DrawNoZSprites: over everything, the depth ignored.
void mr_DrawNoZSprite(LTObject* pObj)
{
    rd_PushAttrib(RD_ATTR_DEPTH);
    rd_Disable(RD_DEPTH_TEST);
    rd_DepthMask(false);
    mr_DrawSprite(pObj);
    rd_PopAttrib();
}

void mr_DrawSkySprite(LTObject* pObj)
{
    mr_DrawSpriteWith(pObj, &g_Frame.m_SkyCam);
}
