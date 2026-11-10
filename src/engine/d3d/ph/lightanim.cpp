// *********************************************************************** //
//  ph/lightanim.cpp -- Primal Hunt's d3d.ren, TU37
//  (0x10033790..0x10034de7), /O2.
//
//  Animated lightmaps (LMAnimStatic): the lookup tables the lightmap
//  blenders index, the dynamic lights drawn into a polygon's lightmap
//  (16- or 32-bit) and the animated lightmap's layers (each light
//  animation's frame pair blended, or a dynamic light's intensity mask
//  lit), summed and uploaded.
//
//  AvP2 1.0.9.6's counterpart is w17_1006ad20_qip.cpp (0x1006c4d0) and
//  d3dr3b_o2.cpp (0x1006c510): the same code a version earlier, 5:5:5
//  where this is 32-bit.
//
//  Near: the texel lighter 0x10033f30 (same size and shape, the registers
//  differ: retail keeps the light in %edi, cl 8966 here in %esi).
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <string.h>
#include <math.h>

#include "ltbasedefs.h"


// The renderer's console variable (TU12's, constructor 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32		m_nValue;
	float		m_fValue;
	float		m_fDefault;
	int32*		m_pnValue;
	float*		m_pfValue;
	const char*	m_pName;
	void*		m_hParam;
	CD3DConVar*	m_pNext;
};


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_LMAnimStatic("LMAnimStatic", 0.0f, LTNULL, LTNULL);    // 0x1007dd60  $E 0x10033790

int32		g_LMAnimRamp[64];                           // 0x1007dc60
uint16		g_LMClampTable[0x300];                      // 0x1007d660


// ---------------------------------------------------------------------- //
//  0x100337b0 -- the 16.16 intensity ramp, 0..1 over 64 steps, scaled
//  and clamped at 1.
// ---------------------------------------------------------------------- //
void d3d_InitLMAnimRamp(float fScale)
{
	uint16 i;

	for (i = 0; i < 64; i++)
	{
		float fVal = (float)i * (1.0f / 63.0f);
		fVal *= fScale;

		if (fVal > 1.0f)
			fVal = 1.0f;

		g_LMAnimRamp[i] = (int32)(fVal * 65536.0f);
	}
}


// ---------------------------------------------------------------------- //
//  0x10033810 -- the clamp table the blenders index with a channel +
//  0xff: 256 zeros, the identity, 256 x 0xff.
// ---------------------------------------------------------------------- //
void d3d_InitLMClampTable()
{
	uint16 i;

	for (i = 0; i < 0x100; i++)
	{
		g_LMClampTable[i] = 0;
		g_LMClampTable[0x100 + i] = i;
		g_LMClampTable[0x200 + i] = 0xff;
	}
}


// The high half of a 32x32 multiply: an __asm helper, inlined (the
// operands and the result go through memory around a one-operand imul at
// every use; AvP2 1.0.9.6's w17_1006ad20_qip.cpp has the same helper).
inline int32 MulHigh(int32 a, int32 b)
{
	int32 r;
	__asm mov  eax, a
	__asm imul b
	__asm mov  r, edx
	return r;
}

// The world, as the lightmap code reads it: its light animations and the
// lightmap texel size.
struct LightAnimFrame
{
	uint8*		m_pData;                                // +0x00 (compressed)
	uint16		m_DataLen;                              // +0x04
	uint8		m_pad_06[0x18 - 0x06];
};

struct LightAnim
{
	uint8			m_pad_00[0x20];
	LTBOOL			m_bDynamic;                         // +0x20 (an intensity mask lit by a light)
	LightAnimFrame**	m_Frames;                       // +0x24 (per frame, one per polygon)
	uint32			m_nFrames;                          // +0x28
	uint32			m_pad_2c;
	uint16			m_nPolies;                          // +0x30
	uint16			m_pad_32;
	int32			m_iFrames[2];                       // +0x34 (the two frames blended)
	int32			m_Blend;                            // +0x3c (0..255)
	float			m_fBrightness;                      // +0x40
	LTVector		m_vLightPos;                        // +0x44 (dynamic: the light)
	float			m_fLightR, m_fLightG, m_fLightB;    // +0x50
	float			m_fLightRadius;                     // +0x5c
};

struct SceneWorld
{
	uint8		m_pad_00[0x08];
	LightAnim*	m_Lights;                               // +0x08
	uint32		m_nLights;                              // +0x0c
	uint8		m_pad_10[0xf8 - 0x10];
	float		m_fLMTexelSize;                         // +0xf8 (world units per texel)
};

// A dynamic light on a polygon's list (the light and where it is).
struct LightObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint8		m_pad_08c[0x94 - 0x8c];
	uint8		m_ColorR, m_ColorG, m_ColorB, m_ColorA;     // +0x094
	uint8		m_pad_098[0x1b0 - 0x98];
	float		m_LightRadius;                          // +0x1b0
};


#define LFLAG_ONLYLIGHTWORLD	0x10

struct DynLightNode
{
	DynLightNode*	m_pNext;                            // +0x00
	LightObject*	m_pLight;                           // +0x04
	LTVector		m_Pos;                              // +0x08
};

// A polygon's reference to a light animation and its index there.
struct LightAnimRef
{
	uint16		m_iLight;                               // +0x00
	uint16		m_iPoly;                                // +0x02
};

// A polygon's lightmap page record.
struct LMRecord
{
	uint8					m_pad_00[0x1c];
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x1c
	LTBOOL					m_bDirty;                   // +0x20
};

struct WorldPoly
{
	uint8			m_pad_00[0x0c];
	LightAnimRef*	m_pLightIndices;                    // +0x0c
	uint32			m_nLightIndices;                    // +0x10
	uint16			m_PageFlags;                        // +0x14 (bits 11..13 the lightmap basis)
	uint8			m_pad_16[0x28 - 0x16];
	LTVector*		m_pNormal;                          // +0x28
	uint8			m_pad_2c[0x30 - 0x2c];
	DynLightNode*	m_pLights;                          // +0x30
	uint8			m_pad_34[0x38 - 0x34];
	LTVector		m_vLMOrigin;                        // +0x38 (texel (0,0))
	uint8			m_pad_44[0x48 - 0x44];
	LMRecord*		m_pLMPage;                          // +0x48
	uint8			m_LMWidth;                          // +0x4c
	uint8			m_LMHeight;                         // +0x4d
	uint8			m_LMX, m_LMY;                       // +0x4e (its place on the page)
};

#define WPF_LMUPLOADED		0x8000

// A dynamic light, set up in a polygon's lightmap space.
struct LMLightInfo
{
	LTVector	m_vOrigin;                              // +0x00 texel (0,0)
	LTVector	m_vU;                                   // +0x0c per texel in u
	LTVector	m_vV;                                   // +0x18 per texel in v
	LTVector	m_vLight;                               // +0x24
	float		m_fDist;                                // +0x30 squared
	float		m_fRadius;                              // +0x34 squared
	float		m_fInvRadius;                           // +0x38 1/radius^2
	uint8*		m_pData;                                // +0x3c
	uint32		m_nWidth;                               // +0x40
	uint32		m_nHeight;                              // +0x44
	long		m_Pitch;                                // +0x48
	int32		m_nColor[3];                            // +0x4c (16.16)
	LTBOOL		m_bLit;                                 // +0x58
	uint16		m_Color16;                              // +0x5c (5:5:5)
	uint32		m_Color32;                              // +0x60
};

// The pixel format and conversion request (ph/pixelformat.cpp's shapes).
enum BPPIdent
{
	BPP_8P=0,
	BPP_8,
	BPP_16,
	BPP_32,
	BPP_S3TC_DXT1,
	BPP_S3TC_DXT3,
	BPP_S3TC_DXT5
};

class PFormat
{
public:
	virtual void	Init(BPPIdent Type, uint32 aMask, uint32 rMask, uint32 gMask, uint32 bMask);

	void			InitPValueFormat();                 // 0x100374a6

	BPPIdent	m_eType;
	uint32		m_Masks[4];
	uint32		m_nBits[4];
	uint32		m_FirstBits[4];
};

class FMConvertRequest
{
public:
					FMConvertRequest();             // 0x10037511

	PFormat			*m_pSrcFormat;
	uint8			*m_pSrc;
	long			m_SrcPitch;
	void			*m_pSrcPalette;

	PFormat			*m_pDestFormat;
	uint8			*m_pDest;
	long			m_DestPitch;

	uint32			m_Width;
	uint32			m_Height;

	uint32			m_Flags;

	PFormat			m_DefaultSrcFormat;
	PFormat			m_DefaultDestFormat;
};

class FormatMgr
{
public:
	LTRESULT		ConvertPixels(FMConvertRequest *pRequest);  // 0x1003764c
};

// A lightmap being drawn (lmsurfaces.cpp).
struct LMTextureList;
class RTexture;
struct LMDrawDesc
{
	LTBOOL		Begin(WorldPoly *pPoly, LTBOOL bCopy, uint32 width, uint32 height);   // 0x100358b3
	LTBOOL		End(LTBOOL bChanged);                   // 0x10035a3f

	void		*m_pData;                               // +0x00
	uint32		m_Pitch;                                // +0x04
	uint32		m_iSize;                                // +0x08
	PFormat		m_Format;                               // +0x0c
	uint32		m_Width;                                // +0x44
	uint32		m_Height;                               // +0x48
	LMTextureList	*m_pList;                           // +0x4c
	RTexture	*m_pScratch;                            // +0x50
};

class CMulTable
{
public:
	CMulTable();

	uint8		m_Table[256][256];
};

extern CMulTable	g_MulTable;                         // 0x10087c00 (d3d_tables.cpp): a * b / 255
extern uint8		g_SatTable[512];                    // 0x10097c00
extern FormatMgr	g_FormatMgr;                        // 0x100661a8 (d3d_texture.cpp)
extern LPDIRECTDRAWSURFACE7	g_pLightmapScratch;         // 0x10068310 (d3d_texture.cpp)
extern int32		g_CV_FastLight;                     // 0x1005df58 (common_stuff.cpp)

void	d3d_GetLMAxes(int iBasis, LTVector *pNormal, LTVector *pRight, LTVector *pUp);  // 0x10035860
LTBOOL	d3d_DecompressLM32(uint8 *pIn, uint32 dataLen, uint32 *pOut);      // 0x10033277 (lmdynamic.cpp)
LTBOOL	d3d_DecompressLMMask(uint8 *pIn, uint32 dataLen, uint8 *pOut);     // 0x100332db (lmdynamic.cpp)
void	r_SetPFormat(DDPIXELFORMAT* pPF, PFormat* pFormat);                 // 0x10011794 (common_init.cpp)

void	d3d_LightTexel(uint16 *pTexel, LMLightInfo *pInfo);
void	d3d_LightTexels32(LMLightInfo *pInfo, uint32 unused1, uint32 unused2);


// ---------------------------------------------------------------------- //
//  0x10033840 -- a light in a polygon's lightmap space: the texel origin,
//  the u and v steps (the lightmap axes scaled by the texel size over
//  their squared length), the radius squared and its inverse, the colour
//  (16.16 offsets from half, 5:5:5 and 8:8:8) and the lightmap.
// ---------------------------------------------------------------------- //
void d3d_SetupLMLight(SceneWorld *pWorld, WorldPoly *pPoly, LTVector *pPos,
	void *pData, long pitch, uint32 width, uint32 height,
	float fRadius, uint32 r, uint32 g, uint32 b, LMLightInfo *pInfo)
{
	LTVector vRight, vUp;
	int32 full[3];
	float fInv;

	d3d_GetLMAxes((pPoly->m_PageFlags >> 11) & 7, pPoly->m_pNormal, &vRight, &vUp);

	pInfo->m_vLight = *pPos;
	pInfo->m_vOrigin = pPoly->m_vLMOrigin;

	fInv = 1.0f / vRight.Mag();
	pInfo->m_vU = vRight * (fInv * fInv * pWorld->m_fLMTexelSize);

	fInv = 1.0f / vUp.Mag();
	pInfo->m_vV = vUp * (fInv * fInv * pWorld->m_fLMTexelSize);

	pInfo->m_fRadius = fRadius * fRadius;
	pInfo->m_fInvRadius = 1.0f / pInfo->m_fRadius;

	pInfo->m_Color16 = (uint16)((((uint16)r << 7) & 0x7c00) |
		(((uint16)g << 2) & 0x3e0) |
		((uint16)b >> 3));
	pInfo->m_Color32 = (r << 16) | (g << 8) | b;

	pInfo->m_nColor[0] = r << 16;
	pInfo->m_nColor[1] = g << 16;
	pInfo->m_nColor[2] = b << 16;

	full[0] = 0xff0000 - pInfo->m_nColor[0];
	full[1] = 0xff0000 - pInfo->m_nColor[1];
	full[2] = 0xff0000 - pInfo->m_nColor[2];
	pInfo->m_nColor[0] -= full[0];
	pInfo->m_nColor[1] -= full[1];
	pInfo->m_nColor[2] -= full[2];

	pInfo->m_pData = (uint8*)pData;
	pInfo->m_Pitch = pitch;
	pInfo->m_nWidth = width;
	pInfo->m_nHeight = height;
}


// ---------------------------------------------------------------------- //
//  0x10033a20 -- a polygon's dynamic lights drawn into its lightmap
//  (16-bit 5:5:5 or 32-bit): each texel inside a light's radius lit
//  through the ramp, or set to the light's colour when the lights are
//  fast or the light lights only the world.  The answer: whether any
//  texel was lit.
// ---------------------------------------------------------------------- //
uint32 d3d_DrawAnimatedLightmap(SceneWorld *pWorld, WorldPoly *pPoly, void *pData, long pitch,
	uint32 width, uint32 height, bool b16Bit)
{
	LMLightInfo info;
	DynLightNode *pNode;
	LightObject *pLight;
	LTVector vRow, vPos;
	uint8 *pRow;
	uint16 *pTexel16;
	uint32 *pTexel32;
	uint32 x, y;

	info.m_bLit = LTFALSE;

	for (pNode = pPoly->m_pLights; pNode; pNode = pNode->m_pNext)
	{
		pLight = pNode->m_pLight;
		d3d_SetupLMLight(pWorld, pPoly, &pNode->m_Pos, pData, pitch, width, height,
			pLight->m_LightRadius, pLight->m_ColorR, pLight->m_ColorG, pLight->m_ColorB, &info);

		pRow = info.m_pData;
		if (!g_CV_FastLight && !(pLight->m_Flags & LFLAG_ONLYLIGHTWORLD) && pRow)
		{
			if (b16Bit)
			{
				vRow = info.m_vOrigin;
				for (y = info.m_nHeight; y; y--)
				{
					vPos = vRow;
					pTexel16 = (uint16*)pRow;
					for (x = info.m_nWidth; x; x--)
					{
						info.m_fDist = (vPos - info.m_vLight).MagSqr();
						if (info.m_fDist < info.m_fRadius)
						{
							d3d_LightTexel(pTexel16, &info);
							info.m_bLit = LTTRUE;
						}

						pTexel16++;
						vPos = vPos + info.m_vU;
					}

					pRow += info.m_Pitch;
					vRow = vRow + info.m_vV;
				}
			}
			else
			{
				d3d_LightTexels32(&info, 0, 0);
			}
		}
		else
		{
			if (b16Bit)
			{
				vRow = info.m_vOrigin;
				for (y = info.m_nHeight; y; y--)
				{
					vPos = vRow;
					pTexel16 = (uint16*)pRow;
					for (x = info.m_nWidth; x; x--)
					{
						info.m_fDist = (vPos - info.m_vLight).MagSqr();
						if (info.m_fDist < info.m_fRadius)
						{
							*pTexel16 = info.m_Color16;
							info.m_bLit = LTTRUE;
						}

						pTexel16++;
						vPos = vPos + info.m_vU;
					}

					pRow += info.m_Pitch;
					vRow = vRow + info.m_vV;
				}
			}
			else
			{
				vRow = info.m_vOrigin;
				for (y = info.m_nHeight; y; y--)
				{
					vPos = vRow;
					pTexel32 = (uint32*)pRow;
					for (x = info.m_nWidth; x; x--)
					{
						info.m_fDist = (vPos - info.m_vLight).MagSqr();
						if (info.m_fDist < info.m_fRadius)
						{
							*pTexel32 = info.m_Color32;
							info.m_bLit = LTTRUE;
						}

						pTexel32++;
						vPos = vPos + info.m_vU;
					}

					pRow += info.m_Pitch;
					vRow = vRow + info.m_vV;
				}
			}
		}
	}

	return info.m_bLit;
}


// ---------------------------------------------------------------------- //
//  0x10033f30 -- light one 5:5:5 texel, attenuated through the ramp.
// ---------------------------------------------------------------------- //
void d3d_LightTexel(uint16 *pTexel, LMLightInfo *pInfo)
{
	int32 nAtten, r, g, b;

	nAtten = g_LMAnimRamp[(int32)((1.0f - pInfo->m_fDist * pInfo->m_fInvRadius) * 63.0f)];
	r = (*pTexel & 0x7c00) >> 7;
	r += MulHigh(nAtten, pInfo->m_nColor[0]);
	g = (*pTexel & 0x3e0) >> 2;
	g += MulHigh(nAtten, pInfo->m_nColor[1]);
	b = (*pTexel & 0x1f) << 3;
	b += MulHigh(nAtten, pInfo->m_nColor[2]);

	*pTexel = (uint16)(((g_LMClampTable[r + 0xff] << 7) & 0x7c00) |
		((g_LMClampTable[g + 0xff] << 2) & 0x3e0) |
		(g_LMClampTable[b + 0xff] >> 3));
}


LTBOOL	d3d_DrawDynamicLMLayer(SceneWorld *pWorld, WorldPoly *pPoly, LightAnim *pAnim,
	LightAnimRef *pRef, uint32 *pOut);
LTBOOL	d3d_BlendLMFrames(WorldPoly *pPoly, LightAnim *pAnim, LightAnimRef *pRef, uint32 *pOut);


// ---------------------------------------------------------------------- //
//  0x10034000 -- a polygon's animated lightmap: every usable layer (a
//  valid frame, brightness at least 0.02) made by d3d_DrawDynamicLMLayer
//  or d3d_BlendLMFrames, the first copied and the rest added scaled by
//  their brightness.  Then, with LMAnimStatic or bForce, converted into
//  the scratch surface and blitted onto the polygon's place on its page;
//  else, when a layer past the first was used, converted straight into
//  the page; else the page is only marked dirty.
// ---------------------------------------------------------------------- //
LTBOOL d3d_UpdateAnimatedLightmap(SceneWorld *pWorld, WorldPoly *pPoly, LTBOOL bForce)
{
	FMConvertRequest request;
	uint32 layer[0x400];
	uint32 accum[0x400];
	DDSURFACEDESC2 ddsd;
	LMDrawDesc desc;
	PFormat srcFormat;
	RECT rcSrc;
	LMRecord *pPage;
	LightAnimRef *pRef;
	LightAnim *pAnim;
	uint32 nPixels, nBytes;
	LTBOOL bLayered, bOK;
	uint32 nDone, i, n;
	uint32 src, dest;
	uint8 scale;

	pPage = pPoly->m_pLMPage;
	if (!pPage || !g_pLightmapScratch)
		return LTFALSE;

	pPage->m_bDirty = LTTRUE;

	if (!pPoly->m_nLightIndices)
		return LTTRUE;

	bForce |= (g_CV_LMAnimStatic.m_nValue != 0);
	nPixels = pPoly->m_LMHeight * pPoly->m_LMWidth;
	nBytes = nPixels * sizeof(uint32);

	bLayered = LTFALSE;
	nDone = 0;
	for (i=0; i < pPoly->m_nLightIndices; i++)
	{
		pRef = &pPoly->m_pLightIndices[i];
		if (pRef->m_iLight >= pWorld->m_nLights)
			continue;

		pAnim = &pWorld->m_Lights[pRef->m_iLight];
		if (pAnim->m_iFrames[0] == -1 || pAnim->m_fBrightness < 0.02f)
			continue;

		if (pAnim->m_bDynamic)
			bOK = d3d_DrawDynamicLMLayer(pWorld, pPoly, pAnim, pRef, layer);
		else
			bOK = d3d_BlendLMFrames(pPoly, pAnim, pRef, layer);

		if (!bOK)
			continue;

		bLayered = (i > 0);

		if (!nDone)
		{
			memcpy(accum, layer, nBytes);
		}
		else
		{
			scale = (uint8)(int32)(pAnim->m_fBrightness * 255.0f);
			for (n=0; n < nPixels; n++)
			{
				src = layer[n];
				dest = accum[n];
				accum[n] = ((uint32)g_SatTable[g_MulTable.m_Table[(src >> 16) & 0xff][scale] + ((dest >> 16) & 0xff)] << 16) |
					((uint32)g_SatTable[g_MulTable.m_Table[(src >> 8) & 0xff][scale] + ((dest >> 8) & 0xff)] << 8) |
					(uint32)g_SatTable[g_MulTable.m_Table[src & 0xff][scale] + (dest & 0xff)];
			}
		}

		nDone++;
	}

	if (!nDone)
		memset(accum, 0, nBytes);

	if (bForce)
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		if (g_pLightmapScratch->Lock(LTNULL, &ddsd, 0, LTNULL) != DD_OK)
			return LTFALSE;

		srcFormat.InitPValueFormat();
		*request.m_pSrcFormat = srcFormat;
		request.m_pSrc = (uint8*)accum;
		request.m_SrcPitch = pPoly->m_LMWidth * sizeof(uint32);
		r_SetPFormat(&ddsd.ddpfPixelFormat, request.m_pDestFormat);
		request.m_pDest = (uint8*)ddsd.lpSurface;
		request.m_DestPitch = ddsd.lPitch;
		request.m_Width = pPoly->m_LMWidth;
		request.m_Height = pPoly->m_LMHeight;
		g_FormatMgr.ConvertPixels(&request);

		g_pLightmapScratch->Unlock(LTNULL);

		rcSrc.left = 0;
		rcSrc.top = 0;
		rcSrc.right = pPoly->m_LMWidth;
		rcSrc.bottom = pPoly->m_LMHeight;
		return pPage->m_pSurface->BltFast(pPoly->m_LMX, pPoly->m_LMY, g_pLightmapScratch,
			&rcSrc, DDBLTFAST_WAIT) == DD_OK;
	}

	if (!bLayered)
	{
		pPage->m_bDirty = LTTRUE;
		return LTTRUE;
	}

	if (!desc.Begin(pPoly, LTFALSE, 0, 0))
		return LTFALSE;

	srcFormat.InitPValueFormat();
	*request.m_pSrcFormat = srcFormat;
	request.m_pSrc = (uint8*)accum;
	request.m_SrcPitch = pPoly->m_LMWidth * sizeof(uint32);
	*request.m_pDestFormat = desc.m_Format;
	request.m_pDest = (uint8*)desc.m_pData;
	request.m_DestPitch = desc.m_Pitch;
	request.m_Width = pPoly->m_LMWidth;
	request.m_Height = pPoly->m_LMHeight;
	g_FormatMgr.ConvertPixels(&request);

	pPage->m_bDirty = LTFALSE;
	pPoly->m_PageFlags |= WPF_LMUPLOADED;
	return desc.End(LTTRUE);
}


// ---------------------------------------------------------------------- //
//  0x100346c0 -- a dynamic light's layer of an animated lightmap: the
//  intensity mask (frame A, frame B or the two blended), lit by the
//  light through the ramp; black outside its radius.
// ---------------------------------------------------------------------- //
LTBOOL d3d_DrawDynamicLMLayer(SceneWorld *pWorld, WorldPoly *pPoly, LightAnim *pAnim,
	LightAnimRef *pRef, uint32 *pOut)
{
	uint8 maskA[0x400];
	uint8 maskB[0x400];
	uint8 mask[0x400];
	LMLightInfo info;
	LightAnimFrame *pFrameA, *pFrameB;
	LTVector vRow, vPos;
	uint8 *pMask, *pRow;
	uint32 *pTexel;
	uint32 x, y, n, nAtten;
	int32 blend, r, g, b;
	int32 iFrameA, iFrameB;

	iFrameA = pAnim->m_iFrames[0];
	iFrameB = pAnim->m_iFrames[1];
	blend = pAnim->m_Blend;

	if (iFrameA == iFrameB || blend <= 0)
	{
		pFrameA = &pAnim->m_Frames[iFrameA][pRef->m_iPoly];
		if (!pFrameA->m_DataLen || !d3d_DecompressLMMask(pFrameA->m_pData, pFrameA->m_DataLen, mask))
			return LTFALSE;
	}
	else if (blend >= 0xff)
	{
		pFrameB = &pAnim->m_Frames[iFrameB][pRef->m_iPoly];
		if (!pFrameB->m_DataLen || !d3d_DecompressLMMask(pFrameB->m_pData, pFrameB->m_DataLen, mask))
			return LTFALSE;
	}
	else
	{
		pFrameA = &pAnim->m_Frames[iFrameA][pRef->m_iPoly];
		pFrameB = &pAnim->m_Frames[iFrameB][pRef->m_iPoly];
		if (!pFrameA->m_DataLen && !pFrameB->m_DataLen)
			return LTFALSE;

		if (!d3d_DecompressLMMask(pFrameA->m_pData, pFrameA->m_DataLen, maskA))
			return LTFALSE;

		if (!d3d_DecompressLMMask(pFrameB->m_pData, pFrameB->m_DataLen, maskB))
			return LTFALSE;

		for (n=0; n < (uint32)(pPoly->m_LMHeight * pPoly->m_LMWidth); n++)
			mask[n] = maskA[n] + (uint8)(((maskB[n] - maskA[n]) * blend) >> 8);
	}

	d3d_SetupLMLight(pWorld, pPoly, &pAnim->m_vLightPos, pOut,
		pPoly->m_LMWidth * sizeof(uint32), pPoly->m_LMWidth, pPoly->m_LMHeight,
		pAnim->m_fLightRadius, (int32)pAnim->m_fLightR, (int32)pAnim->m_fLightG,
		(int32)pAnim->m_fLightB, &info);

	vRow = info.m_vOrigin;
	pRow = (uint8*)pOut;
	pMask = mask;
	for (y = info.m_nHeight; y; y--)
	{
		vPos = vRow;
		pTexel = (uint32*)pRow;
		for (x = info.m_nWidth; x; x--)
		{
			info.m_fDist = (vPos - info.m_vLight).MagSqr();
			if (*pMask && info.m_fDist < info.m_fRadius)
			{
				nAtten = (g_LMAnimRamp[(int32)((1.0f - info.m_fInvRadius * info.m_fDist) * 63.0f)] * *pMask) >> 8;
				r = MulHigh(nAtten, info.m_nColor[0]);
				g = MulHigh(nAtten, info.m_nColor[1]);
				b = MulHigh(nAtten, info.m_nColor[2]);
				*pTexel = ((uint32)g_LMClampTable[r + 0xff] << 16) |
					((uint32)g_LMClampTable[g + 0xff] << 8) |
					(uint32)g_LMClampTable[b + 0xff];
			}
			else
			{
				*pTexel = 0;
			}

			pTexel++;
			pMask++;
			vPos += info.m_vU;
		}

		pRow += info.m_Pitch;
		vRow += info.m_vV;
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x100349f0 -- a light animation's layer: frame A, frame B, or the two
//  blended per channel, A + ((B - A) * blend >> 8), clamped at 255.
// ---------------------------------------------------------------------- //
LTBOOL d3d_BlendLMFrames(WorldPoly *pPoly, LightAnim *pAnim, LightAnimRef *pRef, uint32 *pOut)
{
	uint32 frames[2][0x400];
	LightAnimFrame *pFrame, *pFrameA, *pFrameB;
	int32 iFrameA, iFrameB, blend;
	uint32 n, a, b, *pSrc;
	int32 cr, cg, cb;

	if (pRef->m_iPoly >= pAnim->m_nPolies)
		return LTFALSE;

	iFrameA = pAnim->m_iFrames[0];
	iFrameB = pAnim->m_iFrames[1];
	blend = pAnim->m_Blend;

	if (iFrameA == iFrameB || blend <= 0)
	{
		pFrame = &pAnim->m_Frames[iFrameA][pRef->m_iPoly];
	}
	else if (blend >= 0xff)
	{
		pFrame = &pAnim->m_Frames[iFrameB][pRef->m_iPoly];
	}
	else
	{
		pFrameA = &pAnim->m_Frames[iFrameA][pRef->m_iPoly];
		pFrameB = &pAnim->m_Frames[iFrameB][pRef->m_iPoly];
		if (!pFrameA->m_DataLen && !pFrameB->m_DataLen)
			return LTFALSE;

		if (!d3d_DecompressLM32(pFrameA->m_pData, pFrameA->m_DataLen, frames[0]))
			return LTFALSE;

		if (!d3d_DecompressLM32(pFrameB->m_pData, pFrameB->m_DataLen, frames[1]))
			return LTFALSE;

		pSrc = frames[0];
		for (n = pPoly->m_LMHeight * pPoly->m_LMWidth; n; n--)
		{
			a = pSrc[0];
			b = pSrc[0x400];

			cr = ((int32)(((b >> 16) & 0xff) - ((a >> 16) & 0xff)) * blend >> 8) + ((a >> 16) & 0xff);
			if (cr > 0xff)
				cr = 0xff;
			cg = ((int32)(((b >> 8) & 0xff) - ((a >> 8) & 0xff)) * blend >> 8) + ((a >> 8) & 0xff);
			if (cg > 0xff)
				cg = 0xff;
			cb = ((int32)((b & 0xff) - (a & 0xff)) * blend >> 8) + (a & 0xff);
			if (cb > 0xff)
				cb = 0xff;

			*pOut++ = (cr << 16) | (cg << 8) | cb;
			pSrc++;
		}

		return LTTRUE;
	}

	if (!pFrame->m_DataLen)
		return LTFALSE;

	return d3d_DecompressLM32(pFrame->m_pData, pFrame->m_DataLen, pOut);
}


// ---------------------------------------------------------------------- //
//  0x10034bf0 -- a dynamic light drawn into a 32-bit lightmap, through
//  the ramp.
// ---------------------------------------------------------------------- //
void d3d_LightTexels32(LMLightInfo *pInfo, uint32 unused1, uint32 unused2)
{
	LTVector vRow, vPos;
	uint8 *pRow, *pTexel;
	uint32 x, y;
	int32 nAtten, r, g, b;

	pRow = pInfo->m_pData;
	vRow = pInfo->m_vOrigin;
	for (y = pInfo->m_nHeight; y; y--)
	{
		vPos = vRow;
		pTexel = pRow;
		for (x = pInfo->m_nWidth; x; x--)
		{
			pInfo->m_fDist = (vPos - pInfo->m_vLight).MagSqr();
			if (pInfo->m_fDist < pInfo->m_fRadius)
			{
				nAtten = g_LMAnimRamp[(int32)((1.0f - pInfo->m_fDist * pInfo->m_fInvRadius) * 63.0f)];
				r = pTexel[2] + MulHigh(nAtten, pInfo->m_nColor[0]);
				g = pTexel[1] + MulHigh(nAtten, pInfo->m_nColor[1]);
				b = (*(uint32*)pTexel & 0xff) + MulHigh(nAtten, pInfo->m_nColor[2]);
				*(uint32*)pTexel = ((uint32)g_LMClampTable[r + 0xff] << 16) |
					((uint32)g_LMClampTable[g + 0xff] << 8) |
					(uint32)g_LMClampTable[b + 0xff];
				pInfo->m_bLit = LTTRUE;
			}

			pTexel += sizeof(uint32);
			vPos += pInfo->m_vU;
		}

		pRow += pInfo->m_Pitch;
		vRow += pInfo->m_vV;
	}
}
