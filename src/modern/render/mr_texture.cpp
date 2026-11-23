// mr_texture.cpp -- the engine's textures (DTX) as device textures.
//
// As d3d.ren's d3d_BindTexture (src/engine/d3d/ph/d3d_texture.cpp): when
// the engine asks, fetch its copy of the data (callback +0x08), upload it,
// keep our object in SharedTexture::m_pRenderData, and give the engine's
// copy back (callback +0x0c) so it is not kept twice. Which mip levels go up
// follows d3d.ren's d3d_GetFirstUsableMipmap: MipmapOffset, the texture
// group's offset and the DTX's own, clamped to what the file has.
//
// 32-bit DTX pixels are ARGB 8888 little-endian words, i.e. B,G,R,A bytes;
// S3TC levels are uploaded compressed when the device takes them (d3d.ren
// does, under S3TCEnable) and decoded here otherwise. (From nolfsrc's
// renderer, which read the same code in the Mac release.)

#include <math.h>

#include "mr.h"

// ---- S3TC decoding, for a device without it --------------------------------

static void mr_Rgb565(uint16_t c, uint8_t* rgb)
{
    uint32_t r = (c >> 11) & 0x1f, gg = (c >> 5) & 0x3f, b = c & 0x1f;
    rgb[0] = (uint8_t)(r << 3 | r >> 2);
    rgb[1] = (uint8_t)(gg << 2 | gg >> 4);
    rgb[2] = (uint8_t)(b << 3 | b >> 2);
}

// One 4x4 block's colour part into 16 BGRA pixels (alpha set separately
// unless DXT1, where colour 3 may be transparent).
static void mr_DecodeColorBlock(const uint8_t* p, uint8_t out[16][4], bool bDXT1)
{
    uint16_t c0 = p[0] | p[1] << 8, c1 = p[2] | p[3] << 8;
    uint8_t col[4][4];
    mr_Rgb565(c0, col[0]);
    mr_Rgb565(c1, col[1]);
    col[0][3] = col[1][3] = 255;
    if (c0 > c1 || !bDXT1)
    {
        for (int i = 0; i < 3; i++)
        {
            col[2][i] = (uint8_t)((2 * col[0][i] + col[1][i]) / 3);
            col[3][i] = (uint8_t)((col[0][i] + 2 * col[1][i]) / 3);
        }
        col[2][3] = col[3][3] = 255;
    }
    else
    {
        for (int i = 0; i < 3; i++)
        {
            col[2][i] = (uint8_t)((col[0][i] + col[1][i]) / 2);
            col[3][i] = 0;
        }
        col[2][3] = 255;
        col[3][3] = 0;
    }
    uint32_t bits = p[4] | p[5] << 8 | p[6] << 16 | (uint32_t)p[7] << 24;
    for (int i = 0; i < 16; i++)
    {
        const uint8_t* c = col[(bits >> (2 * i)) & 3];
        out[i][0] = c[2];               // B
        out[i][1] = c[1];               // G
        out[i][2] = c[0];               // R
        out[i][3] = c[3];               // A
    }
}

static void mr_DecodeS3TC(const uint8_t* pData, uint32_t w, uint32_t h, int nType, uint8_t* pOut)
{
    uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
    uint32_t nBlock = nType == BPP_S3TC_DXT1 ? 8 : 16;
    for (uint32_t by = 0; by < bh; by++)
    {
        for (uint32_t bx = 0; bx < bw; bx++)
        {
            const uint8_t* p = pData + (by * bw + bx) * nBlock;
            uint8_t px[16][4];
            mr_DecodeColorBlock(nType == BPP_S3TC_DXT1 ? p : p + 8, px, nType == BPP_S3TC_DXT1);
            if (nType == BPP_S3TC_DXT3)
            {
                for (int i = 0; i < 16; i++)
                {
                    uint8_t a = (p[i / 2] >> ((i & 1) * 4)) & 0xf;
                    px[i][3] = (uint8_t)(a << 4 | a);
                }
            }
            else if (nType == BPP_S3TC_DXT5)
            {
                uint8_t a[8];
                a[0] = p[0];
                a[1] = p[1];
                if (a[0] > a[1])
                    for (int i = 1; i < 7; i++)
                        a[i + 1] = (uint8_t)(((7 - i) * a[0] + i * a[1]) / 7);
                else
                {
                    for (int i = 1; i < 5; i++)
                        a[i + 1] = (uint8_t)(((5 - i) * a[0] + i * a[1]) / 5);
                    a[6] = 0;
                    a[7] = 255;
                }
                uint64_t bits = 0;
                for (int i = 0; i < 6; i++)
                    bits |= (uint64_t)p[2 + i] << (8 * i);
                for (int i = 0; i < 16; i++)
                    px[i][3] = a[(bits >> (3 * i)) & 7];
            }
            for (int y = 0; y < 4; y++)
            {
                uint32_t py = by * 4 + y;
                if (py >= h)
                    break;
                for (int x = 0; x < 4; x++)
                {
                    uint32_t pxx = bx * 4 + x;
                    if (pxx < w)
                        memcpy(pOut + (py * w + pxx) * 4, px[y * 4 + x], 4);
                }
            }
        }
    }
}

// ---- upload ---------------------------------------------------------------------

static uint32_t mr_S3TCSize(uint32_t w, uint32_t h, int nType)
{
    uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
    return bw * bh * (nType == BPP_S3TC_DXT1 ? 8 : 16);
}

static void mr_ApplyTextureFilter(int nLevels)
{
    bool bLinear = cvi(CV_Bilinear) != 0;
    RDFilter mag = bLinear ? RD_LINEAR : RD_NEAREST;
    RDFilter min = mag;
    if (nLevels > 1)
    {
        if (cvi(CV_Trilinear))
            min = bLinear ? RD_LINEAR_MIPMAP_LINEAR : RD_NEAREST_MIPMAP_LINEAR;
        else
            min = bLinear ? RD_LINEAR_MIPMAP_NEAREST : RD_NEAREST_MIPMAP_NEAREST;
    }
    rd_TexFilter(min, mag);
    if (g_RDCaps.bAniso)
    {
        float a = cvf(CV_Anisotropic);
        if (a < 1.0f) a = 1.0f;
        if (a > g_RDCaps.fMaxAniso) a = g_RDCaps.fMaxAniso;
        rd_TexAniso(a);
    }
}

static bool mr_Upload(MRTexture* pTex, TextureData* pData)
{
    const DtxHeader& hdr = pData->m_Header;
    int nType = hdr.m_Extra[2];
    if (nType == 0)
        nType = BPP_32;                 // 0 and 3 both mean 32-bit
    if (nType != BPP_32 && nType != BPP_S3TC_DXT1 && nType != BPP_S3TC_DXT3 && nType != BPP_S3TC_DXT5)
        return false;                   // palettised / 8-bit: no renderer has a path

    int nMips = hdr.m_nMipmaps;
    if (nMips > MAX_DTX_MIPS)
        nMips = MAX_DTX_MIPS;
    if (nMips < 1)
        return false;

    // The first level drawn and the number of levels, as d3d.ren's
    // d3d_CreateRTexture: the group's offset, the file's own (Extra[4]),
    // the global one and, without S3TC, the file's extra one (Extra[3]),
    // within 0..3 and the chain; then not above the size limit
    // (d3d_GetFirstUsableMipmap). The levels: the file's count (Extra[1];
    // 0 is 4), within what the chain has from there.
    int nGroup = hdr.m_Extra[0];
    if (nGroup > 9)
        nGroup = 9;
    int nFirst = cvi(CV_GroupOffset0 + nGroup) + hdr.m_Extra[4] + cvi(CV_MipmapOffset);
    if (!cvi(CV_S3TCEnable))
        nFirst += hdr.m_Extra[3];
    nFirst = nFirst < 0 ? 0 : nFirst > 3 ? 3 : nFirst;
    if (nFirst > nMips - 1)
        nFirst = nMips - 1;
    int nMax = g_RDCaps.nMaxTextureSize;
    if (cvi(CV_MaxTextureSize) > 0 && cvi(CV_MaxTextureSize) < nMax)
        nMax = cvi(CV_MaxTextureSize);
    while (nFirst < nMips - 1 &&
           ((int)pData->m_Mips[nFirst].m_Width > nMax || (int)pData->m_Mips[nFirst].m_Height > nMax))
        nFirst++;
    int nUse = hdr.m_Extra[1] ? hdr.m_Extra[1] : 4;
    if (nUse > nMips - nFirst)
        nUse = nMips - nFirst;
    if (nUse < 1)
        nUse = 1;

    // Which textures keep their alpha (d3d.ren's format choice in
    // d3d_CreateRTexture; NOLF's 0x10038a90): those the DTX flags give an alpha
    // format -- 4444 (0x80), 5551 (0x100), and fullbright ones, whose alpha
    // marks the fullbright texels -- and with "32BitTextures" all but those
    // that prefer 16 bits (0x2). Every other texture is 565 there, opaque
    // whatever its file's alpha says. Here the alpha is kept at full
    // precision, as Direct3D under Wine also ends up doing: a 1-bit alpha
    // would cut a soft gradient (the menus' bars) down to nothing.
    uint32_t f = hdr.m_IFlags;
    bool bAlpha = (cvi(CV_32BitTextures) && !(f & 2)) || (f & (0x100 | 0x80 | DTX_FULLBRITE));
    RDFormat eFormat = bAlpha ? RD_FMT_RGBA8 : RD_FMT_RGB8;
    bool bCompressed = nType != BPP_32;
    // A compressed texture goes up compressed when its format carries no
    // alpha it should not have: DXT1 as RGB, or anything keeping its alpha.
    bool bUploadCompressed = bCompressed && g_RDCaps.bS3TC && cvi(CV_S3TCEnable) &&
                             (bAlpha || nType == BPP_S3TC_DXT1);

    rd_BindTexture(pTex->m_Id);
    rd_TexWrap(true, true);

    int nLevels = 0;
    uint32_t nBytes = 0;
    uint8_t* pDecode = NULL;
    for (int i = nFirst; i < nFirst + nUse; i++)
    {
        const TextureMip& mip = pData->m_Mips[i];
        if (!mip.m_pData || !mip.m_Width || !mip.m_Height)
            break;
        if (bUploadCompressed)
        {
            static const RDFormat fmt[3] = { RD_FMT_DXT1A, RD_FMT_DXT3, RD_FMT_DXT5 };
            RDFormat eDXT = bAlpha ? fmt[nType - BPP_S3TC_DXT1] : RD_FMT_DXT1;
            uint32_t n = mr_S3TCSize(mip.m_Width, mip.m_Height, nType);
            rd_CompressedTexImage(nLevels, eDXT, mip.m_Width, mip.m_Height, n, mip.m_pData);
            nBytes += n;
        }
        else if (bCompressed)
        {
            uint8_t* p = (uint8_t*)realloc(pDecode, mip.m_Width * mip.m_Height * 4);
            if (!p)
                break;
            pDecode = p;
            mr_DecodeS3TC(mip.m_pData, mip.m_Width, mip.m_Height, nType, pDecode);
            rd_TexImage(nLevels, eFormat, mip.m_Width, mip.m_Height, RD_PIX_BGRA, pDecode);
            nBytes += mip.m_Width * mip.m_Height * 4;
        }
        else
        {
            rd_TexImage(nLevels, eFormat, mip.m_Width, mip.m_Height, RD_PIX_BGRA, mip.m_pData,
                        mip.m_Pitch ? mip.m_Pitch / 4 : 0);
            nBytes += mip.m_Width * mip.m_Height * 4;
        }
        if (nLevels == 0)
        {
            pTex->m_nWidth = mip.m_Width;
            pTex->m_nHeight = mip.m_Height;
        }
        nLevels++;
    }
    free(pDecode);
    if (!nLevels)
        return false;
    rd_TexMaxLevel(nLevels - 1);
    mr_ApplyTextureFilter(nLevels);
    pTex->m_DtxFlags = hdr.m_IFlags;
    pTex->m_nBaseWidth = pData->m_Mips[0].m_Width;
    pTex->m_nBaseHeight = pData->m_Mips[0].m_Height;
    float fScale = (float)(1 << (hdr.m_Extra[4] & 15));
    pTex->m_fUMul = pData->m_Mips[0].m_Width ? fScale / pData->m_Mips[0].m_Width : 0.0f;
    pTex->m_fVMul = pData->m_Mips[0].m_Height ? fScale / pData->m_Mips[0].m_Height : 0.0f;
    // The DTX's detail scale (a float at TextureData+0x26, added to 1) and
    // angle in degrees (an int16 at +0x2a), r_SetupTextureSurface 0x180ab4.
    float fDetail;
    int16_t nAngle;
    memcpy(&fDetail, &hdr.m_Extra[6], 4);
    memcpy(&nAngle, &hdr.m_Extra[10], 2);
    pTex->m_fDetailScale = (fDetail == fDetail && fDetail > -1.0f && fDetail < 100.0f) ? 1.0f + fDetail : 1.0f;
    pTex->m_fDetailCos = cosf(nAngle * 3.14159265f / 180.0f);
    pTex->m_fDetailSin = sinf(nAngle * 3.14159265f / 180.0f);
    pTex->m_bAlpha = bAlpha;
    pTex->m_nBytes = nBytes;
    return true;
}

// ---- the slots ------------------------------------------------------------------

void mr_BindTexture(SharedTexture* pTexture, LTBOOL bChanged)
{
    if (!pTexture || !g.bContext)
        return;
    MRTexture* pTex = (MRTexture*)pTexture->m_pRenderData;
    if (pTex && !bChanged)
        return;

    LTBOOL bFound = 0;
    TextureData* pData = g.pStruct->GetTexture(pTexture, &bFound);
    if (!pData)
        return;
    bool bNew = !pTex;
    if (bNew)
    {
        pTex = (MRTexture*)calloc(1, sizeof(MRTexture));
        if (!pTex)
        {
            g.pStruct->FreeTexture(pTexture);
            return;
        }
        pTex->m_Id = rd_GenTexture();
    }
    if (mr_Upload(pTex, pData))
    {
        pTexture->m_pRenderData = pTex;
        g.pStruct->m_nBytes[2] += bNew ? pTex->m_nBytes : 0;
    }
    else if (bNew)
    {
        rd_DeleteTexture(pTex->m_Id);
        free(pTex);
        mr_Debug(1, "texture with unsupported format %d not bound", pData->m_Header.m_Extra[2]);
    }
    g.pStruct->FreeTexture(pTexture);
}

void mr_UnbindTexture(SharedTexture* pTexture)
{
    if (!pTexture)
        return;
    MRTexture* pTex = (MRTexture*)pTexture->m_pRenderData;
    if (pTex)
    {
        if (g.bContext)
            rd_DeleteTexture(pTex->m_Id);
        if (g.pStruct->m_nBytes[2] >= pTex->m_nBytes)
            g.pStruct->m_nBytes[2] -= pTex->m_nBytes;
        free(pTex);
    }
    pTexture->m_pRenderData = NULL;
}

uint32_t mr_TextureId(SharedTexture* pTexture)
{
    if (!pTexture)
        return 0;
    if (!pTexture->m_pRenderData)
        mr_BindTexture(pTexture, 0);
    MRTexture* pTex = (MRTexture*)pTexture->m_pRenderData;
    return pTex ? pTex->m_Id : 0;
}
