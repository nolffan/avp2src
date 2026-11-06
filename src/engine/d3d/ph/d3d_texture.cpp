// *********************************************************************** //
//  ph/d3d_texture.cpp -- Primal Hunt's d3d.ren, TU20
//  (0x1001f280..0x10022a44), /O2 /Ob2.
//
//  The renderer's textures: the texture formats the device offers (the
//  EnumTextureFormats callback, the picker and the format dump), the
//  RTexture records and their bank, the lightmap page surfaces, the gamma
//  ramp and spotlight falloff textures, and binding, freeing and
//  uploading shared textures (r_TransferTexture).  NOLF's
//  w6_10039730_qip.cpp, d3dqip2.cpp and w25_10037bf0_qipni.cpp are the
//  same code a version earlier.
//
//  /Ob2: the initialisers are single bodies, and the small helpers
//  (0x1001f410, 0x1001f860) are expanded into their callers.
//
//  Not yet written here: d3d_CreateAndLoad's big neighbours -- the
//  RTexture maker for a shared texture (0x10020cd0, which emits
//  RTextureHandle's destructor 0x10021030), r_TransferTexture
//  (0x10021040), the mipmap copy (0x10021790), RTextureHandle::operator=
//  (0x10021c90), 0x10021cd0 and d3d_CreateRTexture (0x10021f70).
//
//  Near:
//    * 0x1001f430: retail calls RTextureHandle's implicit constructor out
//      of line (0x1001f5e0) and expands the rest of RTexture's; ours
//      expands both (an /Ob2 inlining-budget difference).
//    * 0x1001fdb0: the one GetMaskRange call /Ob2 expands is the 555 red
//      one in retail, the 565 red one here.
//    * 0x10022830: retail retests the RTexture before the transfer loop.
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"
#include "../../LT2/lithshared/stdlith/object_bank.h"


// ------------------------------------------------------------------------------ //
// Declarations.
// ------------------------------------------------------------------------------ //

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
					FormatMgr();                    // 0x10037600
	LTRESULT		ConvertPixels(FMConvertRequest *pRequest);  // 0x1003764c

	uint8			m_Data[0x9a8];
};

// The lightmap page manager (TU17's object at 0x1008067c).
class CLightmapPages
{
public:
	void		Reset();                                // 0x10035c00 (lmsurfaces.cpp; its body overwrites ecx, never reads this)
	void		Term();                                 // 0x10035c24
	void		SetAllocator(void* (*fn)(uint32 width, uint32 height, uint32 caps));  // 0x10035b7b
};

struct SharedTexture;

// One texture format the device offers (0xa4 bytes).
struct D3DTextureFormat
{
	LTLink			m_Link;                             // +0x00
	DDPIXELFORMAT	m_PF;                               // +0x0c
	uint32			m_PixelShift;                       // +0x2c
	uint32			m_PixelSize;                        // +0x30
	uint32			m_nRBits;                           // +0x34
	uint32			m_nGBits;                           // +0x38
	uint32			m_nBBits;                           // +0x3c
	uint32			m_nABits;                           // +0x40
	uint32			m_8888Shifts[4][2];                 // +0x44 (right, left) R,G,B,A
	uint32			m_565Shifts[3][2];                  // +0x64
	uint32			m_555Shifts[3][2];                  // +0x7c
	uint16			m_Masks[4];                         // +0x94
	uint32			m_b1555;                            // +0x9c
	uint32			m_b565;                             // +0xa0
};

// One entry of the format picker's wish lists.
struct FormatSpec
{
	uint32		m_nRBits;
	uint32		m_nGBits;
	uint32		m_nBBits;
	uint32		m_nABits;
	uint32		m_FlagsOn;
	uint32		m_FlagsOff;
};

#define FORMAT_32BIT		0
#define FORMAT_FULLBRITE	1
#define FORMAT_4444			2
#define FORMAT_NORMAL		3
#define FORMAT_INTERFACE	4
#define FORMAT_LIGHTMAP		5
#define FORMAT_BUMPMAP		6
#define NUM_TEXTUREFORMATS	7

#define MAX_TEXTURESTAGES	4

// RTexture::m_TexFlags.
#define RT_KEEP				(1<<0)	// Only freed when forced.


// The texture interface the engine sees (vtable 0x100483ac).
class RTextureInterface
{
public:
	virtual			~RTextureInterface() {}
	virtual LTBOOL	IsValid()=0;
	virtual uint32	GetFlags()=0;
	virtual uint32	GetWidth()=0;
	virtual uint32	GetHeight()=0;
};

class RTexture;

// RTexture's handle on it (vtable 0x10048398), the first thing in an
// RTexture.
class RTextureHandle : public RTextureInterface
{
public:
	virtual LTBOOL	IsValid();
	virtual uint32	GetFlags();
	virtual uint32	GetWidth();
	virtual uint32	GetHeight();

public:
	float			m_fInvWidth;                        // +0x04
	float			m_fInvHeight;                       // +0x08
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x0c
	uint32			m_MemoryUse;                        // +0x10
	uint16			m_FrameCode;                        // +0x14
	uint16			m_AlphaRef;                         // +0x16
	RTexture*		m_pTexture;                         // +0x18
};

// A texture in video memory (0x4c bytes).
class RTexture
{
public:
				RTexture();

public:
	RTextureHandle	m_Handle;                           // +0x00
	LTLink			m_Link;                             // +0x1c
	uint16			m_Width;                            // +0x28
	uint16			m_Height;                           // +0x2a
	SharedTexture*	m_pSharedTexture;                   // +0x2c
	RTexture*		m_pNext;                            // +0x30
	float			m_DetailTextureScale;               // +0x34
	float			m_DetailTextureAngleC;              // +0x38
	float			m_DetailTextureAngleS;              // +0x3c
	uint16			m_Flags;                            // +0x40
	uint16			m_Stage;                            // +0x42
	uint16			m_Dirty;                            // +0x44
	uint8			m_FirstMip;                         // +0x46
	uint8			m_nMipmaps;                         // +0x47
	uint8			m_FormatIndex;                      // +0x48
	uint8			m_TexFlags;                         // +0x49
};


// The engine's RenderStruct, as far as the texture code uses it.
struct RenderStruct
{
	uint8		m_Reserved00[0x08];
	class TextureData*	(*GetTexture)(SharedTexture *pTexture, LTBOOL *bFound);  // +0x08
	void		(*FreeTexture)(SharedTexture *pTexture);    // +0x0c
	uint8		m_Reserved10[0x14 - 0x10];
	void		(*ConsolePrint)(char *pMsg, ...);           // +0x14
	uint8		m_Reserved18[0x50 - 0x18];
	uint32		m_SystemTextureMemory;                      // +0x50
};

struct SharedTexture
{
	uint8		m_Reserved00[0x0c];
	RTexture*	m_pRenderData;                              // +0x0c
};

// The engine's texture data, as far as the renderer reads it.
struct TextureMipData
{
	uint32		m_Width;                                    // +0x00
	uint32		m_Height;                                   // +0x04
	uint8		m_Reserved08[0x18 - 0x08];
};

class TextureData
{
public:
	uint8		m_Reserved00[0x14];
	uint16		m_nMipmaps;                                 // +0x14
	uint8		m_Reserved16[0xcc - 0x16];
	TextureMipData	m_Mips[8];                              // +0xcc
};

// What d3d_CreateRTexture builds from.
struct TextureRequest
{
	SharedTexture*	m_pSharedTexture;
	TextureData*	m_pTextureData;
	uint32			m_Stage;
};

// The profiling counters (TU14: 0x10013fb7).
#define CSTART_NONE			0

class CounterFinal
{
public:
					CounterFinal(unsigned long startMode=CSTART_NONE);
	unsigned long	m_Data[2];
};

// Other modules.
extern RenderStruct*		g_pStruct;                  // 0x1005df08
extern LPDIRECTDRAW7		g_pDirectDraw;              // 0x1005d2a8
extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8
extern int32				g_CV_32BitLightMaps;        // 0x1005da0c
extern uint32				g_TextureStage;             // 0x100622d0
extern LTBOOL				g_bLightmapPages;           // 0x100638b8
extern CLightmapPages		g_LightmapPages;            // 0x1008067c
extern int32				g_CV_MaxTextureSize;        // 0x1005d998 (common_stuff.cpp)
extern uint32				g_MaxTextureWidth;          // 0x10062394
extern uint32				g_MaxTextureHeight;         // 0x10062398
extern LTBOOL				g_bWindowedTextures;        // 0x1006230c
extern uint32				g_ScreenWidth;              // 0x1005d8cc
extern uint32				g_ScreenHeight;             // 0x1005d9c0
extern uint32				g_BindTextureStage;         // 0x100638c0
extern uint32				g_DeviceTextureCaps;        // 0x1006241c (D3DPTEXTURECAPS_)
extern int32				g_CV_MaxAspectRatio;        // 0x1005d4f4

void*	dalloc(uint32 size);                            // 0x10013a66
void	dfree(void* ptr);                               // 0x10013ac5
void	AddDebugMessage(int32 debugLevel, const char* pMsg, ...);  // 0x10013b13
uint32	CountBits(uint32 val);                          // 0x10013c60
void	r_SetPFormat(DDPIXELFORMAT* pPF, PFormat* pFormat);  // 0x10011794
void	d3d_InitLightmapTables();                       // 0x10033810


// ------------------------------------------------------------------------------ //
// Globals.
// ------------------------------------------------------------------------------ //

LTLink			g_TextureFormatList(LTLink_Init);       // 0x100682f0  $E 0x1001f280
LTList			g_SharedTextures(LTLink_Init);          // 0x10066e40  $E 0x1001f290
ObjectBank<RTexture>	g_RTextureBank;                 // 0x10067280  $E 0x1001f2b0, 0x1001f2e0
LTLink			g_RTextures(LTLink_Init);               // 0x10068300  $E 0x1001f330
FormatMgr		g_FormatMgr;                            // 0x100661a8  $E 0x1001f340
CD3DConVar		g_CV_S3TCEnable("S3TCEnable", 1.0f, LTNULL, LTNULL);  // 0x10067250  $E 0x1001f350

D3DTextureFormat*	g_TextureFormats[NUM_TEXTUREFORMATS];   // 0x100682c8
RTexture*		g_pCurTextures[MAX_TEXTURESTAGES];      // 0x10067270
uint32			g_bS3TCDXT5;                            // 0x100682e4
uint32			g_bS3TCDXT3;                            // 0x100682e8
uint32			g_bS3TCDXT1;                            // 0x100682ec
LTBOOL			g_bTexturesInitted;                     // 0x1006830c
LPDIRECTDRAWSURFACE7	g_pLightmapScratch;             // 0x10068310
LPDIRECTDRAWSURFACE7	g_pFalloffTexture;              // 0x10068314
LPDIRECTDRAWSURFACE7	g_pGammaTexture;                // 0x10068318
float			g_fGamma;                               // 0x1006831c

// The spotlight falloff's exponent.
float			g_FalloffExponent = 3.0f;               // 0x1004d5e8


// ------------------------------------------------------------------------------ //
// RTexture.
// ------------------------------------------------------------------------------ //

// 0x1001f370
RTexture::RTexture()
{
	m_Link.TieOff();
	m_Width = m_Height = 0;
	m_pSharedTexture = LTNULL;
	m_Flags = 0;
	m_Stage = 0;
	m_FirstMip = 0;
	m_nMipmaps = 0;
	m_FormatIndex = 0;
	m_TexFlags = 0;
	m_pNext = LTNULL;
	m_Dirty = 0;
	m_DetailTextureScale = 1.0f;
}

// 0x1001f3d0
LTBOOL RTextureHandle::IsValid()
{
	return LTTRUE;
}

// 0x1001f3e0
uint32 RTextureHandle::GetFlags()
{
	return m_pTexture->m_Flags;
}

// 0x1001f3f0
uint32 RTextureHandle::GetWidth()
{
	return m_pTexture->m_Width;
}

// 0x1001f400
uint32 RTextureHandle::GetHeight()
{
	return m_pTexture->m_Height;
}


// ------------------------------------------------------------------------------ //
// 0x1001f410 -- the format lightmap pages use.
// ------------------------------------------------------------------------------ //
D3DTextureFormat* d3d_GetLightmapFormat()
{
	if (g_CV_32BitLightMaps && g_TextureFormats[FORMAT_32BIT])
		return g_TextureFormats[FORMAT_32BIT];

	return g_TextureFormats[FORMAT_LIGHTMAP];
}


// ------------------------------------------------------------------------------ //
// 0x1001f430 -- make a lightmap page texture (the page manager's allocator).
// ------------------------------------------------------------------------------ //
RTexture* d3d_CreateLightmapTexture(uint32 width, uint32 height, uint32 caps)
{
	D3DTextureFormat *pFormat;
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE7 pSurface;
	RTexture *pTexture;

	pFormat = d3d_GetLightmapFormat();
	if (!pFormat)
		return LTNULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	ddsd.ddsCaps.dwCaps = caps | DDSCAPS_TEXTURE;
	ddsd.dwTextureStage = g_TextureStage;
	memcpy(&ddsd.ddpfPixelFormat, &pFormat->m_PF, sizeof(DDPIXELFORMAT));

	if (g_pDirectDraw->CreateSurface(&ddsd, &pSurface, LTNULL) != DD_OK)
		return LTNULL;

	pTexture = g_RTextureBank.Allocate();
	if (!pTexture)
	{
		pSurface->Release();
		return LTNULL;
	}

	pTexture->m_Handle.m_pTexture = pTexture;
	pTexture->m_Handle.m_pSurface = pSurface;
	pTexture->m_pSharedTexture = LTNULL;
	pTexture->m_Link.m_pData = LTNULL;
	pTexture->m_Flags = 0;
	pTexture->m_Width = (uint16)width;
	pTexture->m_Height = (uint16)height;
	pTexture->m_Handle.m_FrameCode = 0;
	pTexture->m_Handle.m_fInvWidth = 1.0f / (float)width;
	pTexture->m_Handle.m_fInvHeight = 1.0f / (float)height;
	return pTexture;
}


// ------------------------------------------------------------------------------ //
// 0x1001f5f0 -- rebuild the gamma ramp texture.
// ------------------------------------------------------------------------------ //
LPDIRECTDRAWSURFACE7 d3d_CreateTextureFromData(uint32 *pData, uint32 width, uint32 height, long pitch);

void d3d_SetGammaTexture(float gamma)
{
	uint32 data[64][64];
	uint32 i, val, color;

	if (g_fGamma == gamma)
		return;

	g_fGamma = gamma;

	if (g_pGammaTexture)
	{
		g_pGammaTexture->Release();
		g_pGammaTexture = LTNULL;
	}

	memset(data, 0, sizeof(data));

	for (i = 0; i < 64; i++)
	{
		val = (uint32)((float)pow(i * (1.0f / 64.0f), gamma) * 255.0f);
		color = (((val << 8) | val) << 8) | val;

		data[i][i] = color;
		if (i > 0)
		{
			data[i-1][i] = color;
			data[i][i-1] = color;
		}
	}

	g_pGammaTexture = d3d_CreateTextureFromData((uint32*)data, 64, 64, 64*sizeof(uint32));
}


// ------------------------------------------------------------------------------ //
// 0x1001f6d0 -- make a texture from 32-bit ARGB data.
// ------------------------------------------------------------------------------ //
LPDIRECTDRAWSURFACE7 d3d_CreateTextureFromData(uint32 *pData, uint32 width, uint32 height, long pitch)
{
	FMConvertRequest request;
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE7 pSurface;
	DDSURFACEDESC2 lockDesc;
	D3DTextureFormat *pFormat;

	if (g_TextureFormats[FORMAT_32BIT])
		pFormat = g_TextureFormats[FORMAT_32BIT];
	else if (g_TextureFormats[FORMAT_4444])
		pFormat = g_TextureFormats[FORMAT_4444];
	else
		return LTNULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_OPAQUE | DDSCAPS2_DONOTPERSIST;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	memcpy(&ddsd.ddpfPixelFormat, &pFormat->m_PF, sizeof(DDPIXELFORMAT));

	if (g_pDirectDraw->CreateSurface(&ddsd, &pSurface, LTNULL) == DD_OK)
	{
		memset(&lockDesc, 0, sizeof(lockDesc));
		lockDesc.dwSize = sizeof(lockDesc);

		if (pSurface->Lock(LTNULL, &lockDesc, 0, LTNULL) == DD_OK)
		{
			request.m_pSrcFormat->InitPValueFormat();
			request.m_pSrc = (uint8*)pData;
			request.m_SrcPitch = pitch;
			r_SetPFormat(&pFormat->m_PF, request.m_pDestFormat);
			request.m_pDest = (uint8*)lockDesc.lpSurface;
			request.m_DestPitch = lockDesc.lPitch;
			request.m_Width = width;
			request.m_Height = height;
			g_FormatMgr.ConvertPixels(&request);

			pSurface->Unlock(LTNULL);
			return pSurface;
		}

		pSurface->Release();
	}

	return LTNULL;
}


// ------------------------------------------------------------------------------ //
// 0x1001f860 -- reset the lightmap pages and their scratch surface.
// ------------------------------------------------------------------------------ //
static void d3d_FreeLightmapScratch()
{
	if (g_pLightmapScratch)
	{
		g_pLightmapScratch->Release();
		g_pLightmapScratch = LTNULL;
	}
}

static void d3d_CreateLightmapScratch()
{
	D3DTextureFormat *pFormat;
	DDSURFACEDESC2 ddsd;

	d3d_FreeLightmapScratch();

	pFormat = d3d_GetLightmapFormat();
	if (pFormat)
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		ddsd.dwWidth = ddsd.dwHeight = 32;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		memcpy(&ddsd.ddpfPixelFormat, &pFormat->m_PF, sizeof(DDPIXELFORMAT));
		g_pDirectDraw->CreateSurface(&ddsd, &g_pLightmapScratch, LTNULL);
	}
}

void d3d_ResetLightmapPages()
{
	d3d_FreeLightmapScratch();
	g_LightmapPages.Term();
	g_LightmapPages.SetAllocator((void* (*)(uint32, uint32, uint32))d3d_CreateLightmapTexture);
	d3d_CreateLightmapScratch();
}


// ------------------------------------------------------------------------------ //
// 0x1001f930 -- start up the texture code: find the device's texture formats.
// ------------------------------------------------------------------------------ //
HRESULT CALLBACK d3d_EnumTextureFormatsCB(LPDDPIXELFORMAT pPF, LPVOID pUser);
D3DTextureFormat* d3d_FindTextureFormat(FormatSpec *pSpecs, uint32 nSpecs);
void d3d_CheckS3TCFormats();
void d3d_CreateFalloffTexture();
void d3d_FreeTexture(RTexture *pTexture, LTBOOL bForce);
void d3d_PrintTextureFormat(const char *pPrefix, D3DTextureFormat *pFormat);

// Pick the formats the renderer uses from what the device offers.
static LTBOOL d3d_SelectTextureFormats()
{
	FormatSpec specs32Bit[1] =
	{
		{8, 8, 8, 8, DDPF_ALPHAPIXELS, DDPF_LUMINANCE}
	};
	FormatSpec specsFullbrite[2] =
	{
		{5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE},
		{4, 4, 4, 4, DDPF_ALPHAPIXELS, DDPF_LUMINANCE}
	};
	FormatSpec specs4444[2] =
	{
		{4, 4, 4, 4, DDPF_ALPHAPIXELS, DDPF_LUMINANCE},
		{5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE}
	};
	FormatSpec specsNormal[3] =
	{
		{5, 6, 5, 0, DDPF_RGB, DDPF_LUMINANCE},
		{5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE},
		{4, 4, 4, 4, DDPF_ALPHAPIXELS, DDPF_LUMINANCE}
	};
	FormatSpec specsInterface[2] =
	{
		{5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE | DDPF_PALETTEINDEXED8},
		{4, 4, 4, 4, DDPF_ALPHAPIXELS, DDPF_LUMINANCE | DDPF_PALETTEINDEXED8}
	};
	FormatSpec specsLightmap[2] =
	{
		{5, 5, 5, 0, DDPF_RGB, DDPF_LUMINANCE},
		{5, 5, 5, 1, DDPF_ALPHAPIXELS, DDPF_LUMINANCE}
	};
	FormatSpec specsBump[1] =
	{
		{8, 8, 0, 0, DDPF_BUMPDUDV, DDPF_LUMINANCE}
	};

	g_TextureFormats[FORMAT_32BIT] = d3d_FindTextureFormat(specs32Bit, 1);

	g_TextureFormats[FORMAT_FULLBRITE] = d3d_FindTextureFormat(specsFullbrite, 2);
	if (!g_TextureFormats[FORMAT_FULLBRITE])
	{
		AddDebugMessage(0, "FORMAT_FULLBRITE texture format missing.");
		return LTFALSE;
	}

	g_TextureFormats[FORMAT_4444] = d3d_FindTextureFormat(specs4444, 2);
	if (!g_TextureFormats[FORMAT_4444])
	{
		AddDebugMessage(0, "FORMAT_4444 texture format missing.");
		return LTFALSE;
	}

	g_TextureFormats[FORMAT_NORMAL] = d3d_FindTextureFormat(specsNormal, 3);
	if (!g_TextureFormats[FORMAT_NORMAL])
	{
		AddDebugMessage(0, "FORMAT_NORMAL texture format missing.");
		return LTFALSE;
	}

	g_TextureFormats[FORMAT_INTERFACE] = d3d_FindTextureFormat(specsInterface, 2);
	if (!g_TextureFormats[FORMAT_INTERFACE])
	{
		AddDebugMessage(0, "FORMAT_INTERFACE texture format missing.");
		return LTFALSE;
	}

	g_TextureFormats[FORMAT_LIGHTMAP] = d3d_FindTextureFormat(specsLightmap, 2);
	if (!g_TextureFormats[FORMAT_LIGHTMAP])
	{
		AddDebugMessage(0, "Warning: device not lightmap capable.");
		g_bLightmapPages = LTFALSE;
	}

	g_TextureFormats[FORMAT_BUMPMAP] = d3d_FindTextureFormat(specsBump, 1);

	d3d_CheckS3TCFormats();
	return LTTRUE;
}

LTBOOL d3d_InitTextures()
{
	uint32 i;

	memset(g_TextureFormats, 0, sizeof(g_TextureFormats));
	for (i = 0; i < MAX_TEXTURESTAGES; i++)
		g_pCurTextures[i] = LTNULL;

	dl_TieOff(&g_TextureFormatList);
	dl_InitList(&g_SharedTextures);
	dl_TieOff(&g_RTextures);
	g_LightmapPages.Reset();

	g_RTextureBank.Init(64);
	g_bTexturesInitted = LTTRUE;

	g_pD3DDevice->EnumTextureFormats(d3d_EnumTextureFormatsCB, LTNULL);

	if (!d3d_SelectTextureFormats())
		return LTFALSE;

	d3d_ResetLightmapPages();
	d3d_InitLightmapTables();
	d3d_CreateFalloffTexture();
	d3d_SetGammaTexture(5.0f);
	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x1001fdb0 -- EnumTextureFormats callback: record each format.
// ------------------------------------------------------------------------------ //
void d3d_GetMaskRange(uint32 mask, uint32 *pEnd, uint32 *pStart);

static void d3d_GetShift(uint32 srcMask, uint32 destMask, uint32 *pRight, uint32 *pLeft)
{
	uint32 srcStart, srcEnd, destStart, destEnd;

	d3d_GetMaskRange(srcMask, &srcEnd, &srcStart);
	d3d_GetMaskRange(destMask, &destEnd, &destStart);

	if (srcEnd > destEnd)
	{
		*pRight = srcEnd - destEnd;
		*pLeft = 0;
	}
	else
	{
		*pRight = 0;
		*pLeft = destEnd - srcEnd;
	}
}

HRESULT CALLBACK d3d_EnumTextureFormatsCB(LPDDPIXELFORMAT pPF, LPVOID pUser)
{
	D3DTextureFormat *pFormat;

	pFormat = (D3DTextureFormat*)dalloc(sizeof(D3DTextureFormat));
	if (pFormat)
	{
		memcpy(&pFormat->m_PF, pPF, sizeof(DDPIXELFORMAT));

		if (pFormat->m_PF.dwFlags & DDPF_PALETTEINDEXED8)
			pFormat->m_PixelShift = 0;
		else if (pFormat->m_PF.dwRGBBitCount == 16)
			pFormat->m_PixelShift = 1;
		else if (pFormat->m_PF.dwRGBBitCount == 32)
			pFormat->m_PixelShift = 2;

		pFormat->m_PixelSize = 1 << pFormat->m_PixelShift;

		pFormat->m_nRBits = CountBits(pPF->dwRBitMask);
		pFormat->m_nGBits = CountBits(pPF->dwGBitMask);
		pFormat->m_nBBits = CountBits(pPF->dwBBitMask);
		pFormat->m_nABits = CountBits(pPF->dwRGBAlphaBitMask);

		d3d_GetShift(0xFF, pPF->dwRBitMask, &pFormat->m_8888Shifts[0][0], &pFormat->m_8888Shifts[0][1]);
		d3d_GetShift(0xFF, pPF->dwGBitMask, &pFormat->m_8888Shifts[1][0], &pFormat->m_8888Shifts[1][1]);
		d3d_GetShift(0xFF, pPF->dwBBitMask, &pFormat->m_8888Shifts[2][0], &pFormat->m_8888Shifts[2][1]);
		d3d_GetShift(0xFF, pPF->dwRGBAlphaBitMask, &pFormat->m_8888Shifts[3][0], &pFormat->m_8888Shifts[3][1]);

		d3d_GetShift(0xF800, pPF->dwRBitMask, &pFormat->m_565Shifts[0][0], &pFormat->m_565Shifts[0][1]);
		d3d_GetShift(0x07E0, pPF->dwGBitMask, &pFormat->m_565Shifts[1][0], &pFormat->m_565Shifts[1][1]);
		d3d_GetShift(0x001F, pPF->dwBBitMask, &pFormat->m_565Shifts[2][0], &pFormat->m_565Shifts[2][1]);

		d3d_GetShift(0x7C00, pPF->dwRBitMask, &pFormat->m_555Shifts[0][0], &pFormat->m_555Shifts[0][1]);
		d3d_GetShift(0x03E0, pPF->dwGBitMask, &pFormat->m_555Shifts[1][0], &pFormat->m_555Shifts[1][1]);
		d3d_GetShift(0x001F, pPF->dwBBitMask, &pFormat->m_555Shifts[2][0], &pFormat->m_555Shifts[2][1]);

		pFormat->m_Masks[0] = (uint16)pPF->dwRBitMask;
		pFormat->m_Masks[1] = (uint16)pPF->dwGBitMask;
		pFormat->m_Masks[2] = (uint16)pPF->dwBBitMask;
		pFormat->m_Masks[3] = (uint16)pPF->dwRGBAlphaBitMask;

		pFormat->m_b1555 = pPF->dwRBitMask == 0x7C00 && pPF->dwGBitMask == 0x3E0 &&
			pPF->dwBBitMask == 0x1F && pPF->dwRGBAlphaBitMask == 0x8000;
		pFormat->m_b565 = pPF->dwRBitMask == 0xF800 && pPF->dwGBitMask == 0x7E0 &&
			pPF->dwBBitMask == 0x1F && pPF->dwRGBAlphaBitMask == 0;

		pFormat->m_Link.m_pData = pFormat;
		dl_Insert(&g_TextureFormatList, &pFormat->m_Link);
	}

	return DDENUMRET_OK;
}



// ------------------------------------------------------------------------------ //
// 0x10020220 -- where a channel mask's run of ones starts and ends.
// ------------------------------------------------------------------------------ //
void d3d_GetMaskRange(uint32 mask, uint32 *pEnd, uint32 *pStart)
{
	uint32 i, bit;

	*pStart = 0;
	bit = 1;
	for (i = 0; i < 32; i++)
	{
		if (mask & bit)
			break;

		bit <<= 1;
		(*pStart)++;
	}

	*pEnd = *pStart;
	for (i = 0; i < 32; i++)
	{
		if (!(mask & bit))
			break;

		bit <<= 1;
		(*pEnd)++;
	}
}


// ------------------------------------------------------------------------------ //
// 0x10020270 -- the first format matching the first spec anything matches.
// ------------------------------------------------------------------------------ //
D3DTextureFormat* d3d_FindTextureFormat(FormatSpec *pSpecs, uint32 nSpecs)
{
	uint32 i;
	LTLink *pCur;
	D3DTextureFormat *pFormat;

	for (i = 0; i < nSpecs; i++)
	{
		for (pCur = g_TextureFormatList.m_pNext; pCur != &g_TextureFormatList; pCur = pCur->m_pNext)
		{
			pFormat = (D3DTextureFormat*)pCur->m_pData;

			if (pFormat->m_nRBits == pSpecs[i].m_nRBits &&
				pFormat->m_nGBits == pSpecs[i].m_nGBits &&
				pFormat->m_nBBits == pSpecs[i].m_nBBits &&
				(pFormat->m_PF.dwFlags & pSpecs[i].m_FlagsOn) &&
				!(pFormat->m_PF.dwFlags & pSpecs[i].m_FlagsOff))
			{
				return pFormat;
			}
		}
	}

	return LTNULL;
}


// ------------------------------------------------------------------------------ //
// 0x100202e0 -- note which S3TC formats the device has.
// ------------------------------------------------------------------------------ //
void d3d_CheckS3TCFormats()
{
	LTLink *pCur;
	D3DTextureFormat *pFormat;

	g_bS3TCDXT5 = g_bS3TCDXT3 = g_bS3TCDXT1 = LTFALSE;

	for (pCur = g_TextureFormatList.m_pNext; pCur != &g_TextureFormatList; pCur = pCur->m_pNext)
	{
		pFormat = (D3DTextureFormat*)pCur->m_pData;

		if (pFormat->m_PF.dwFlags & DDPF_FOURCC)
		{
			if (pFormat->m_PF.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
				g_bS3TCDXT1 = LTTRUE;
			else if (pFormat->m_PF.dwFourCC == MAKEFOURCC('D', 'X', 'T', '3'))
				g_bS3TCDXT3 = LTTRUE;
			else if (pFormat->m_PF.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
				g_bS3TCDXT5 = LTTRUE;
		}
	}
}


// ------------------------------------------------------------------------------ //
// 0x10020350 -- the spotlight falloff texture: alpha falls off with the
// cube of the distance from the centre.
// ------------------------------------------------------------------------------ //
void d3d_CreateFalloffTexture()
{
	FMConvertRequest request;
	uint32 data[16][16];
	int32 x, y;
	float dx, dy, dist;

	if (g_pFalloffTexture)
	{
		g_pFalloffTexture->Release();
		g_pFalloffTexture = LTNULL;
	}

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			dx = (float)x - 7.5f;
			dy = (float)y - 7.5f;
			dist = (float)sqrt(dx*dx + dy*dy);
			if (dist > 7.5f)
				dist = 7.5f;

			data[y][x] = ((uint32)(uint8)(int32)((1.0f - (float)pow(dist * (1.0f / 7.5f), g_FalloffExponent)) * 255.9f) << 24) | 0xFFFFFF;
		}
	}

	g_pFalloffTexture = d3d_CreateTextureFromData((uint32*)data, 16, 16, 16*sizeof(uint32));
}


// ------------------------------------------------------------------------------ //
// 0x10020450 -- shut the texture code down.
// ------------------------------------------------------------------------------ //
void d3d_FreeAllTextures();

void d3d_TermTextures()
{
	LTLink *pCur, *pNext;

	if (!g_bTexturesInitted)
		return;

	d3d_FreeAllTextures();
	g_RTextureBank.Term();

	if (g_pFalloffTexture)
	{
		g_pFalloffTexture->Release();
		g_pFalloffTexture = LTNULL;
	}

	if (g_pGammaTexture)
	{
		g_pGammaTexture->Release();
		g_pGammaTexture = LTNULL;
	}

	pCur = g_TextureFormatList.m_pNext;
	while (pCur != &g_TextureFormatList)
	{
		pNext = pCur->m_pNext;
		dfree(pCur->m_pData);
		pCur = pNext;
	}
	dl_TieOff(&g_TextureFormatList);

	memset(g_TextureFormats, 0, sizeof(g_TextureFormats));
	g_bTexturesInitted = LTFALSE;
}


// ------------------------------------------------------------------------------ //
// 0x10020530 -- free an RTexture (and the textures chained to it).  Kept
// textures go only when forced.
// ------------------------------------------------------------------------------ //
void d3d_FreeTexture(RTexture *pTexture, LTBOOL bForce)
{
	RTexture *pCur, *pNext;
	int i;

	if (pTexture->m_TexFlags & RT_KEEP)
	{
		if (!bForce)
			return;
	}
	else
	{
		pCur = pTexture->m_pNext;
		while (pCur)
		{
			pNext = pCur->m_pNext;
			d3d_FreeTexture(pCur, LTTRUE);
			pCur = pNext;
		}
	}

	if (pTexture->m_pSharedTexture)
	{
		pTexture->m_pSharedTexture->m_pRenderData = LTNULL;
		pTexture->m_pSharedTexture = LTNULL;
	}

	g_pStruct->m_SystemTextureMemory -= pTexture->m_Handle.m_MemoryUse;

	for (i = 0; i < 2; i++)
	{
		if (pTexture == g_pCurTextures[i])
			g_pCurTextures[i] = LTNULL;
	}

	if (pTexture->m_Handle.m_pSurface)
		pTexture->m_Handle.m_pSurface->Release();

	if (pTexture->m_Link.m_pData)
		dl_Remove(&pTexture->m_Link);

	g_RTextureBank.Free(pTexture);
}


// ------------------------------------------------------------------------------ //
// 0x10020600 -- free the RTextures on a list.
// ------------------------------------------------------------------------------ //
void d3d_FreeTextureList(LTLink *pList)
{
	LTLink *pCur, *pNext;

	pCur = pList->m_pNext;
	while (pCur != pList)
	{
		pNext = pCur->m_pNext;
		d3d_FreeTexture((RTexture*)pCur->m_pData, LTFALSE);
		pCur = pNext;
	}

	dl_TieOff(pList);
}


// ------------------------------------------------------------------------------ //
// 0x10020640
// ------------------------------------------------------------------------------ //
void d3d_FreeAllTextures()
{
	d3d_FreeTextureList(&g_RTextures);
}


// ------------------------------------------------------------------------------ //
// 0x10020690 -- LISTTEXTUREFORMATS.
// ------------------------------------------------------------------------------ //
void d3d_ListTextureFormats()
{
	LTLink *pCur;

	for (pCur = g_TextureFormatList.m_pNext; pCur != &g_TextureFormatList; pCur = pCur->m_pNext)
		d3d_PrintTextureFormat("", (D3DTextureFormat*)pCur->m_pData);

	d3d_PrintTextureFormat("[FULLBRITE] - ", g_TextureFormats[FORMAT_FULLBRITE]);
	d3d_PrintTextureFormat("[4444] - ", g_TextureFormats[FORMAT_4444]);
	d3d_PrintTextureFormat("[NORMAL] - ", g_TextureFormats[FORMAT_NORMAL]);
	d3d_PrintTextureFormat("[INTERFACE] - ", g_TextureFormats[FORMAT_INTERFACE]);
	d3d_PrintTextureFormat("[LIGHTMAP] - ", g_TextureFormats[FORMAT_LIGHTMAP]);
}


// ------------------------------------------------------------------------------ //
// 0x10020720 -- one console line for a texture format.
// ------------------------------------------------------------------------------ //
char* d3d_AddFlagName(char *pStr, const char *pName);

// The name of a pixel format's first flag.
static void d3d_GetFlagString(uint32 flags, char *pStr)
{
	pStr[0] = 0;

	if (flags & DDPF_ALPHA)
		d3d_AddFlagName(pStr, "DDPF_ALPHA");
	else if (flags & DDPF_ALPHAPIXELS)
		d3d_AddFlagName(pStr, "DDPF_ALPHAPIXELS");
	else if (flags & DDPF_ALPHAPREMULT)
		d3d_AddFlagName(pStr, "DDPF_ALPHAPREMULT");
	else if (flags & DDPF_BUMPLUMINANCE)
		d3d_AddFlagName(pStr, "DDPF_BUMPLUMINANCE");
	else if (flags & DDPF_BUMPDUDV)
		d3d_AddFlagName(pStr, "DDPF_BUMPDUDV");
	else if (flags & DDPF_COMPRESSED)
		d3d_AddFlagName(pStr, "DDPF_COMPRESSED");
	else if (flags & DDPF_FOURCC)
		d3d_AddFlagName(pStr, "DDPF_FOURCC");
	else if (flags & DDPF_LUMINANCE)
		d3d_AddFlagName(pStr, "DDPF_LUMINANCE");
	else if (flags & DDPF_PALETTEINDEXED1)
		d3d_AddFlagName(pStr, "DDPF_PALETTEINDEXED1");
	else if (flags & DDPF_PALETTEINDEXED2)
		d3d_AddFlagName(pStr, "DDPF_PALETTEINDEXED2");
	else if (flags & DDPF_PALETTEINDEXED4)
		d3d_AddFlagName(pStr, "DDPF_PALETTEINDEXED4");
	else if (flags & DDPF_PALETTEINDEXED8)
		d3d_AddFlagName(pStr, "DDPF_PALETTEINDEXED8");
	else if (flags & DDPF_PALETTEINDEXEDTO8)
		d3d_AddFlagName(pStr, "DDPF_PALETTEINDEXEDTO8");
	else if (flags & DDPF_RGB)
		d3d_AddFlagName(pStr, "DDPF_RGB");
	else if (flags & DDPF_RGBTOYUV)
		d3d_AddFlagName(pStr, "DDPF_RGBTOYUV");
	else if (flags & DDPF_STENCILBUFFER)
		d3d_AddFlagName(pStr, "DDPF_STENCILBUFFER");
	else if (flags & DDPF_ZBUFFER)
		d3d_AddFlagName(pStr, "DDPF_ZBUFFER");
	else if (flags & DDPF_ZPIXELS)
		d3d_AddFlagName(pStr, "DDPF_ZPIXELS");
	else
		d3d_AddFlagName(pStr, "UNKNOWN");
}

void d3d_PrintTextureFormat(const char *pPrefix, D3DTextureFormat *pFormat)
{
	char flagStr[256];
	char fourCC[256];

	if (!pFormat)
	{
		g_pStruct->ConsolePrint("%sNONE", pPrefix);
		return;
	}

	if (pFormat->m_PF.dwFlags & DDPF_FOURCC)
	{
		*((uint32*)fourCC) = pFormat->m_PF.dwFourCC;
		fourCC[4] = 0;
		sprintf(flagStr, "FOURCC( %s )", fourCC);
	}
	else
	{
		d3d_GetFlagString(pFormat->m_PF.dwFlags, flagStr);
	}

	g_pStruct->ConsolePrint("%s%s - %d bits (%d %d %d %d) (%x %x %x %x)",
		pPrefix, flagStr, pFormat->m_PF.dwRGBBitCount,
		pFormat->m_nRBits, pFormat->m_nGBits, pFormat->m_nBBits, pFormat->m_nABits,
		pFormat->m_PF.dwRBitMask, pFormat->m_PF.dwGBitMask, pFormat->m_PF.dwBBitMask,
		pFormat->m_PF.dwRGBAlphaBitMask);
}


// ------------------------------------------------------------------------------ //
// 0x10020c60 -- add a flag's name and a space to a string.
// ------------------------------------------------------------------------------ //
char* d3d_AddFlagName(char *pStr, const char *pName)
{
	strcat(pStr, pName);
	strcat(pStr, " ");
	return &pStr[strlen(pStr)];
}


// ------------------------------------------------------------------------------ //
// 0x10021c00 -- the first mipmap small enough for the card.  -1 if none is.
// ------------------------------------------------------------------------------ //
int32 d3d_GetFirstUsableMipmap(TextureData *pTexture)
{
	uint32 maxWidth, maxHeight, i;

	maxWidth = LTMIN(g_CV_MaxTextureSize, g_MaxTextureWidth);
	maxHeight = LTMIN(g_CV_MaxTextureSize, g_MaxTextureHeight);

	if (!maxWidth)
		maxWidth = 256;

	if (!maxHeight)
		maxHeight = 256;

	if (!g_bWindowedTextures)
	{
		maxWidth = LTMIN(maxWidth, g_ScreenWidth);
		// RETAIL BUG: clamps the width, not the height, to the screen
		// height, so a texture's height limit is the (clamped) width.
		maxHeight = LTMIN(maxWidth, g_ScreenHeight);
	}

	for (i = 0; i < pTexture->m_nMipmaps; i++)
	{
		if (pTexture->m_Mips[i].m_Width <= maxWidth && pTexture->m_Mips[i].m_Height <= maxHeight)
			return i;
	}

	return -1;
}


// ------------------------------------------------------------------------------ //
// Making textures on demand.
// ------------------------------------------------------------------------------ //
RTexture* d3d_CreateRTexture(TextureRequest *pRequest, uint32 flags);  // 0x10021f70
LTBOOL r_TransferTexture(RTexture *pTexture, TextureData *pTextureData);  // 0x10021040

// Load the shared texture's data and make an RTexture from it.
inline RTexture* d3d_CreateAndLoadTexture(SharedTexture *pSharedTexture, uint32 stage)
{
	CounterFinal cntTotal;
	CounterFinal cntLoad;
	TextureRequest request;
	TextureData *pTextureData;
	RTexture *pTexture;
	LTBOOL bFound;

	pTextureData = g_pStruct->GetTexture(pSharedTexture, &bFound);
	if (!pTextureData)
		return LTNULL;

	request.m_pSharedTexture = pSharedTexture;
	request.m_pTextureData = pTextureData;
	request.m_Stage = stage;
	pTexture = d3d_CreateRTexture(&request, 0);
	if (pTexture)
	{
		if (!r_TransferTexture(pTexture, pTextureData))
		{
			AddDebugMessage(4, "Unable to transfer texture data to video memory.");
			d3d_FreeTexture(pTexture, LTFALSE);
			pTexture = LTNULL;
		}
	}

	g_pStruct->FreeTexture(pSharedTexture);
	return pTexture;
}


// ------------------------------------------------------------------------------ //
// 0x10021eb0 -- a shared texture's RTexture flags, loading it if need be.
// ------------------------------------------------------------------------------ //
uint32 d3d_GetTextureFlags(SharedTexture *pSharedTexture, uint32 stage)
{
	if (!pSharedTexture->m_pRenderData)
	{
		if (!d3d_CreateAndLoadTexture(pSharedTexture, stage))
			return 0;
	}

	return pSharedTexture->m_pRenderData->m_Flags;
}


// ------------------------------------------------------------------------------ //
// 0x10022640 -- the FourCC of an S3TC format.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_GetS3TCFourCC(BPPIdent bpp, uint32 *pFourCC)
{
	if (bpp == BPP_S3TC_DXT1)
	{
		*pFourCC = MAKEFOURCC('D', 'X', 'T', '1');
		return LTTRUE;
	}
	else if (bpp == BPP_S3TC_DXT3)
	{
		*pFourCC = MAKEFOURCC('D', 'X', 'T', '3');
		return LTTRUE;
	}
	else if (bpp == BPP_S3TC_DXT5)
	{
		*pFourCC = MAKEFOURCC('D', 'X', 'T', '5');
		return LTTRUE;
	}

	return LTFALSE;
}


// ------------------------------------------------------------------------------ //
// 0x10022690 -- make a texture size valid for the card: square if it
// must be, and within the maximum aspect ratio.
// ------------------------------------------------------------------------------ //
void d3d_AdjustAspectRatio(uint32 width, uint32 height, uint32 *pOutWidth, uint32 *pOutHeight)
{
	uint32 *pSmall, *pLarge;
	int32 ratio;

	if (g_DeviceTextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		width = height = (width > height) ? width : height;
	}
	else if (g_CV_MaxAspectRatio > 0)
	{
		pSmall = (width > height) ? &height : &width;
		pLarge = (width > height) ? &width : &height;

		ratio = *pLarge / *pSmall;
		if (ratio > g_CV_MaxAspectRatio)
			*pSmall = *pLarge / g_CV_MaxAspectRatio;
	}

	*pOutWidth = width;
	*pOutHeight = height;
}


// ------------------------------------------------------------------------------ //
// 0x10022730 -- does the device have this S3TC format?
// ------------------------------------------------------------------------------ //
uint32 d3d_IsS3TCSupported(BPPIdent bpp)
{
	if (bpp == BPP_S3TC_DXT1)
		return g_bS3TCDXT1;
	else if (bpp == BPP_S3TC_DXT3)
		return g_bS3TCDXT3;
	else if (bpp == BPP_S3TC_DXT5)
		return g_bS3TCDXT5;

	return 0;
}


// ------------------------------------------------------------------------------ //
// 0x10022760 -- a shared texture's texel scale, loading it if need be.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_GetTextureScale(SharedTexture *pSharedTexture, uint32 stage, float *pInvWidth, float *pInvHeight)
{
	RTexture *pTexture;

	if (!pSharedTexture->m_pRenderData)
	{
		if (!d3d_CreateAndLoadTexture(pSharedTexture, stage))
			return LTFALSE;
	}

	pTexture = pSharedTexture->m_pRenderData;
	*pInvWidth = pTexture->m_Handle.m_fInvWidth;
	*pInvHeight = pTexture->m_Handle.m_fInvHeight;
	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x10022830 -- RenderStruct::BindTexture.
// ------------------------------------------------------------------------------ //
void d3d_BindTexture(SharedTexture *pSharedTexture, LTBOOL bTextureChanged)
{
	RTexture *pTexture, *pCur;
	TextureData *pTextureData;
	LTBOOL bFound;

	pTexture = pSharedTexture->m_pRenderData;
	if (pTexture)
	{
		if (bTextureChanged)
		{
			pTextureData = g_pStruct->GetTexture(pSharedTexture, &bFound);
			if (pTextureData)
			{
				for (pCur = pTexture; pCur; pCur = pCur->m_pNext)
				{
					if (!r_TransferTexture(pCur, pTextureData))
						AddDebugMessage(4, "Unable to transfer texture data to video memory.");
				}

				g_pStruct->FreeTexture(pSharedTexture);
			}
		}
	}
	else
	{
		d3d_CreateAndLoadTexture(pSharedTexture, g_BindTextureStage);
	}
}


// ------------------------------------------------------------------------------ //
// 0x10022940 -- RenderStruct::UnbindTexture.
// ------------------------------------------------------------------------------ //
void d3d_UnbindTexture(SharedTexture *pSharedTexture)
{
	if (pSharedTexture->m_pRenderData)
		d3d_FreeTexture(pSharedTexture->m_pRenderData, LTFALSE);
}
