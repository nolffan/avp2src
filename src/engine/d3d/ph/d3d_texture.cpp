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
//  (0x1001f410, 0x1001f860) are expanded into their callers.  The upload
//  helpers (r_TransferTexture, d3d_CreateTextureSurface,
//  d3d_GetFirstUsableMipmap, d3d_CreateRTexture) are inline: retail
//  expands d3d_CreateRTexture into d3d_CreateRenderTexture (0x10020cd0)
//  and calls the rest from there, which is what emits their copies (and
//  CheapLTLink::AddAfter 0x10021010, RTextureHandle::operator= 0x10021c90)
//  where they are; d3d_CreateRTexture's own copy (0x10021f70) follows its
//  first caller that keeps the call (0x10021eb0).  The PH and AvP2 copies
//  of all of these are the same code.
//
//  Near:
//    * 0x1001f430: retail calls RTextureHandle's implicit constructor out
//      of line (0x1001f5e0) and expands the rest of RTexture's; ours
//      expands both (an /Ob2 inlining-budget difference).
//    * 0x1001fdb0: the one GetMaskRange call /Ob2 expands is the 555 red
//      one in retail, the 565 red one here.
//    * 0x10020cd0: the other side of the 0x1001f430 difference -- retail
//      expands the RTextureHandle constructor and calls its destructor
//      (0x10021030) out of line at each exit.  The handle's own (empty)
//      virtual destructor is what keeps that call; ours still calls the
//      constructor (827 bytes, 824 shipped).  It also expands
//      RTextureHandle::operator=, so ours is emitted after 0x10021f70,
//      which calls it, rather than at 0x10021c90 (same 55 bytes).
//    * 0x10021040: same flow and calls; 8 bytes less frame and other
//      registers (1852 bytes, 1871 shipped).
//    * 0x10021f70: size-exact (1731 bytes); register choice in the
//      mipmap size loop.
//    * 0x10022830: retail retests the RTexture before the transfer loop.
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ltbasedefs.h"
#include "ltpvalue.h"
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
	BPPIdent		GetType();                          // 0x100374cc

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

union GenericColor
{
	uint32	dwVal;
	uint16	wVal;
	uint8	bVal;
};

class FormatMgr
{
public:
					FormatMgr();                    // 0x10037600
	LTRESULT		ConvertPixels(FMConvertRequest *pRequest);  // 0x1003764c
	LTRESULT		PValueToFormatColor(PFormat *pFormat, PValue in, GenericColor &out);  // 0x10037780

	// pixelformat.cpp's FormatMgr: the two PFormats (0x70), the nine
	// 256-byte scale-from-8 tables (to 0x970), the nine scale-to-8 pointers
	// (to 0x994) and the 0..8-bit tables they point at, 1+2+...+256 bytes
	// (to 0xb93). Its constructor (0x10037600) fills all of it; 0x9a8 here
	// let it write the pointers and tables over the texture globals that
	// followed (AvP2: g_FormatMgr 0x10060710, the next global referenced
	// +0xc98 on).
	uint8			m_Data[0xb94];
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
	virtual ~RTextureHandle() {}

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
	uint8		*m_Data;                                    // +0x08
	long		m_Pitch;                                    // +0x0c (bytes)
	uint8		m_Reserved10[0x18 - 0x10];
};

// TextureData::m_Header.m_IFlags.
#define DTX_FULLBRITE		(1<<0)	// This DTX has fullbrite colors.
#define DTX_PREFER16BIT		(1<<1)	// Use 16-bit, even if in 32-bit mode.
#define DTX_PREFER4444		(1<<7)	// If in 16-bit mode, use a 4444 texture for this.
#define DTX_PREFER5551		(1<<8)	// Use 5551 if 16-bit.

#define DTX_COMMANDSTRING_LEN	128

class DtxHeader
{
public:
	inline uint32	GetTextureGroup()			{return m_Extra[0];}
	inline uint32	GetNumMipmaps()				{return m_Extra[1];}
	inline BPPIdent	GetBPPIdent()				{return m_Extra[2] == 0 ? BPP_32 : (BPPIdent)m_Extra[2];}
	inline uint32	GetNonS3TCMipmapOffset()	{return (uint32)m_Extra[3];}
	inline uint32	GetUIMipmapOffset()			{return m_Extra[4];}
	inline float	GetUIMipmapScale()			{return (float)(1 << GetUIMipmapOffset());}
	inline uint32	GetTexturePriority()		{return m_Extra[5];}
	// It adds 1.0f so all the old values of 0.0f return 1.0f.
	inline float	GetDetailTextureScale()		{return *((float*)&m_Extra[6]) + 1.0f;}
	inline int16	GetDetailTextureAngle()		{return (int16)((m_Extra[10]) + (m_Extra[11] << 8));}

public:
	uint32		m_ResType;                                  // +0x08 (in TextureData)
	int32		m_Version;                                  // +0x0c
	uint16		m_BaseWidth, m_BaseHeight;                  // +0x10
	uint16		m_nMipmaps;                                 // +0x14
	uint16		m_nSections;                                // +0x16
	int32		m_IFlags;                                   // +0x18 (DTX_)
	int32		m_UserFlags;                                // +0x1c
	uint8		m_Extra[12];                                // +0x20
	char		m_CommandString[DTX_COMMANDSTRING_LEN];     // +0x2c
};

class TextureData
{
public:
	virtual void	SetupPFormat(PFormat *pFormat);         // vtable +0x00 (the engine's)

public:
	uint32		m_ResHeader;                                // +0x04
	DtxHeader	m_Header;                                   // +0x08
	uint8		m_ReservedAC[0xcc - 0xac];
	TextureMipData	m_Mips[8];                              // +0xcc
};

// What d3d_CreateRTexture builds from.
struct TextureRequest
{
	SharedTexture*	m_pSharedTexture;
	TextureData*	m_pTextureData;
	uint32			m_Stage;                                // the stage, and TEXSTAGE_ flags above it
};

// TextureRequest::m_Stage: a bump map texture (name ours).
#define TEXSTAGE_BUMPMAP	0x100

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
// The device's capabilities (d3d_init.cpp's g_D3DCaps, 0x100622d8).
struct D3DCaps
{
	LTBOOL          m_bSrcBlendSrcColor;                    // +0x000
	LTBOOL          m_bDestBlendSrcAlpha;                   // +0x004
	LTBOOL          m_bTBlendAdd;                           // +0x008
	LTBOOL          m_bTBlendModulate;                      // +0x00c
	LTBOOL          m_bDither;                              // +0x010
	LTBOOL          m_bDrawPrim;                            // +0x014
	LTBOOL          m_bVidMemTextures;                      // +0x018
	LTBOOL          m_bSysMemTextures;                      // +0x01c
	LTBOOL          m_bAGPTextures;                         // +0x020
	int32           m_nZBufferBits;                         // +0x024
	int32           m_nDeviceBits;                          // +0x028
	uint32          m_dwTextureMem;                         // +0x02c
	uint32          m_dwVideoMem;                           // +0x030
	LTBOOL          m_bSurfacesLargerThanScreen;            // +0x034
	D3DDEVICEDESC7  m_Desc;                                 // +0x038
	D3DPRIMCAPS     m_TriCaps;                              // +0x124
};

extern D3DCaps				g_D3DCaps;                  // 0x100622d8
extern uint32				g_ScreenWidth;              // 0x1005d8cc
extern uint32				g_ScreenHeight;             // 0x1005d9c0
extern uint32				g_iCurTextureStage;         // 0x100638c0 (d3d_init.cpp)
extern int32				g_CV_MaxTexAspectRatio;     // 0x1005d4f4 (common_stuff.cpp)
extern int32				g_CV_32BitTextures;         // 0x1005d8c4
extern int32				g_CV_GroupOffset[10];       // 0x1005d7dc
extern int32				g_CV_MipmapOffset;          // 0x1005df44
extern CD3DConVar			g_CV_AlphaTest;             // 0x1005e258

void*	dalloc(uint32 size);                            // 0x10013a66
void	dfree(void* ptr);                               // 0x10013ac5
void	AddDebugMessage(int32 debugLevel, const char* pMsg, ...);  // 0x10013b13
uint32	CountBits(uint32 val);                          // 0x10013c60
void	r_SetPFormat(DDPIXELFORMAT* pPF, PFormat* pFormat);  // 0x10011794
void	d3d_InitLMClampTable();                        // 0x10033810 (lightanim.cpp)
uint32	CalcImageSize(BPPIdent bpp, uint32 width, uint32 height);  // 0x100377ae


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

// A texture stage's texel size (1 / the bound texture's width, height).
struct TexPair
{
	float	u, v;
};

TexPair			g_TexelSize[MAX_TEXTURESTAGES];         // 0x100672a8
LTBOOL			g_bLightmapTextures;                    // 0x100682dc
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
// The scratch surface's release: no function of its own in retail (inlined
// into both callers).
// ------------------------------------------------------------------------------ //
static void d3d_FreeLightmapPages()
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

	d3d_FreeLightmapPages();

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

// 0x1001f860 -- reset the lightmap pages and their scratch surface.
void d3d_ResetLightmapPages()
{
	d3d_FreeLightmapPages();
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
	d3d_InitLMClampTable();
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
// 0x10020cd0 -- make an RTexture for a shared texture on a stage and load
// it.  A stage copy (bStageCopy) is chained to the shared texture's main
// RTexture by the caller and freed with it.
// ------------------------------------------------------------------------------ //
RTexture* d3d_CreateRTexture(TextureRequest *pRequest, uint32 texFlags);  // 0x10021f70
LTBOOL r_TransferTexture(RTexture *pTexture, TextureData *pTextureData);  // 0x10021040
LTBOOL d3d_GetS3TCFourCC(BPPIdent bpp, uint32 *pFourCC);  // 0x10022640
void d3d_AdjustAspectRatio(uint32 width, uint32 height, uint32 *pOutWidth, uint32 *pOutHeight);  // 0x10022690
uint32 d3d_IsS3TCSupported(BPPIdent bpp);  // 0x10022730
int32 d3d_GetFirstUsableMipmap(TextureData *pTexture);  // 0x10021c00
inline LTBOOL d3d_CreateTextureSurface(TextureRequest *pRequest, RTextureHandle *pHandle,
	uint32 iFirstMip, uint32 nMipmaps, uint32 iFormat);  // 0x10021790

RTexture* d3d_CreateRenderTexture(SharedTexture *pSharedTexture, uint32 iStage, LTBOOL bStageCopy)
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
	request.m_Stage = iStage;
	pTexture = d3d_CreateRTexture(&request, bStageCopy);
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
// 0x10021040 -- copy a texture's mipmaps into its surfaces: bump maps as
// du/dv differences, 32-bit data converted to the surface's format (and
// tiled over a surface the aspect ratio made bigger), S3TC data copied or
// blitted from a system memory surface.
// ------------------------------------------------------------------------------ //
inline LTBOOL r_TransferTexture(RTexture *pTexture, TextureData *pTextureData)
{
	FMConvertRequest request;
	DDSURFACEDESC2 ddsd, lockDesc, tempDesc, tempLock;
	DDSCAPS2 caps;
	LPDIRECTDRAWSURFACE7 pSurface, pTempSurface;
	D3DTextureFormat *pFormat;
	TextureMipData *pMip;
	BPPIdent bpp;
	uint32 iMipmap, surfaceWidth, surfaceHeight, mipWidth, mipHeight;
	uint32 fourCC, size, x, y;
	int32 i, nTiles;
	long pitch;
	uint8 *pDest, *pSrc, *pOut, *pTemp;
	HRESULT hResult;

	if (pTexture->m_FirstMip + pTexture->m_nMipmaps > pTextureData->m_Header.m_nMipmaps)
	{
		AddDebugMessage(1, "r_TransferTexture: mipmap count doesn't match!");
		return LTFALSE;
	}

	bpp = pTextureData->m_Header.GetBPPIdent();
	pFormat = g_TextureFormats[pTexture->m_FormatIndex];
	pSurface = pTexture->m_Handle.m_pSurface;

	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
	pSurface->GetSurfaceDesc(&ddsd);
	surfaceWidth = ddsd.dwWidth;
	surfaceHeight = ddsd.dwHeight;
	AddDebugMessage(4, "Uploading a (%dx%d) texture", surfaceWidth, surfaceHeight);

	for (iMipmap = pTexture->m_FirstMip; iMipmap < (uint32)(pTexture->m_FirstMip + pTexture->m_nMipmaps); iMipmap++)
	{
		if (!pSurface)
			return LTFALSE;

		pMip = &pTextureData->m_Mips[iMipmap];
		mipWidth = pMip->m_Width;
		mipHeight = pMip->m_Height;

		lockDesc.dwSize = sizeof(lockDesc);
		if (pSurface->Lock(LTNULL, &lockDesc, DDLOCK_WRITEONLY, LTNULL) != DD_OK)
			return LTFALSE;

		pitch = lockDesc.lPitch;

		if (lockDesc.ddpfPixelFormat.dwFlags & DDPF_BUMPDUDV)
		{
			// Bump map: the height differences to the left and above.
			if (lockDesc.ddpfPixelFormat.dwRGBBitCount != 16)
			{
				pSurface->Unlock(LTNULL);
				return LTFALSE;
			}

			pDest = (uint8*)lockDesc.lpSurface;

			FMConvertRequest heightRequest;
			pTemp = new uint8[pMip->m_Width * pMip->m_Height];
			if (!pTemp)
			{
				pSurface->Unlock(LTNULL);
				return LTFALSE;
			}

			pTextureData->SetupPFormat(heightRequest.m_pSrcFormat);
			heightRequest.m_pSrc = pMip->m_Data;
			heightRequest.m_SrcPitch = pMip->m_Pitch;
			heightRequest.m_pDestFormat->Init(BPP_8, 0xFF, 0, 0, 0);
			heightRequest.m_pDest = pTemp;
			heightRequest.m_DestPitch = pMip->m_Width;
			heightRequest.m_Width = pMip->m_Width;
			heightRequest.m_Height = pMip->m_Height;
			if (g_FormatMgr.ConvertPixels(&heightRequest) != LT_OK)
			{
				delete [] pTemp;
				pSurface->Unlock(LTNULL);
				return LTFALSE;
			}

			// The first row and column have nothing to difference with.
			memset(pDest, 0, pMip->m_Width * 2);
			pOut = pDest;
			for (y = 0; y < pMip->m_Height; y++)
			{
				*((uint16*)pOut) = 0;
				pOut += pitch;
			}

			for (y = 1; y < pMip->m_Height; y++)
			{
				pSrc = &pTemp[pMip->m_Width * y + 1];
				pOut = pDest;
				for (x = 1; x < pMip->m_Width; x++)
				{
					*pOut++ = (uint8)((pSrc[0] - pSrc[-1]) >> 1);
					*pOut++ = (uint8)((pSrc[0] - pSrc[-(int32)pMip->m_Width]) >> 1);
					pSrc++;
				}

				pDest += pitch;
			}

			delete [] pTemp;
		}
		else if (bpp == BPP_32)
		{
			request.m_pSrcFormat->InitPValueFormat();
			request.m_pSrc = pMip->m_Data;
			request.m_SrcPitch = pMip->m_Pitch;
			r_SetPFormat(&pFormat->m_PF, request.m_pDestFormat);
			request.m_pDest = (uint8*)lockDesc.lpSurface;
			request.m_DestPitch = pitch;
			request.m_Width = pMip->m_Width;
			request.m_Height = pMip->m_Height;
			request.m_Flags = 0;
			if (g_FormatMgr.ConvertPixels(&request) != LT_OK)
			{
				pSurface->Unlock(LTNULL);
				return LTFALSE;
			}
		}
		else
		{
			if (!d3d_GetS3TCFourCC(bpp, &fourCC))
			{
				pSurface->Unlock(LTNULL);
				return LTFALSE;
			}

			size = CalcImageSize(bpp, pMip->m_Width, pMip->m_Height);

			if (ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
			{
				// The surface is compressed too: copy the blocks.
				if (fourCC != ddsd.ddpfPixelFormat.dwFourCC || size != lockDesc.dwLinearSize)
				{
					pSurface->Unlock(LTNULL);
					return LTFALSE;
				}

				memcpy(lockDesc.lpSurface, pMip->m_Data, size);
			}
			else
			{
				// Let DirectDraw decompress it: blit from a system memory
				// surface holding the compressed data.
				pSurface->Unlock(LTNULL);

				memset(&tempDesc, 0, sizeof(tempDesc));
				tempDesc.dwSize = sizeof(tempDesc);
				tempDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
				tempDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
				tempDesc.lpSurface = pMip->m_Data;
				tempDesc.dwLinearSize = size;
				tempDesc.dwWidth = pMip->m_Width;
				tempDesc.dwHeight = pMip->m_Height;
				tempDesc.ddpfPixelFormat.dwFlags |= DDPF_FOURCC;
				tempDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
				tempDesc.ddpfPixelFormat.dwFourCC = fourCC;
				if (g_pDirectDraw->CreateSurface(&tempDesc, &pTempSurface, LTNULL) != DD_OK)
					return LTFALSE;

				memset(&tempLock, 0, sizeof(tempLock));
				tempLock.dwSize = sizeof(tempLock);
				if (pTempSurface->Lock(LTNULL, &tempLock, DDLOCK_WAIT | DDLOCK_WRITEONLY, LTNULL) != DD_OK)
				{
					pTempSurface->Release();
					return LTFALSE;
				}

				if (tempLock.dwLinearSize != size)
				{
					pTempSurface->Unlock(LTNULL);
					pTempSurface->Release();
					return LTFALSE;
				}

				memcpy(tempLock.lpSurface, pMip->m_Data, size);
				pTempSurface->Unlock(LTNULL);

				hResult = pSurface->BltFast(0, 0, pTempSurface, LTNULL, DDBLTFAST_WAIT);
				pTempSurface->Release();
				if (hResult != DD_OK)
					return LTFALSE;

				lockDesc.dwSize = sizeof(lockDesc);
				if (pSurface->Lock(LTNULL, &lockDesc, DDLOCK_WRITEONLY, LTNULL) != DD_OK)
					return LTFALSE;
			}
		}

		if (bpp == BPP_32)
		{
			// Repeat the mipmap over the rest of the surface.
			nTiles = surfaceWidth / mipWidth;
			for (i = 1; i < nTiles; i++)
			{
				pSrc = (uint8*)lockDesc.lpSurface;
				pDest = (uint8*)lockDesc.lpSurface + pFormat->m_PixelSize * i * mipWidth;
				for (y = 0; y < mipHeight; y++)
				{
					memcpy(pDest, pSrc, mipWidth << pFormat->m_PixelShift);
					pSrc += pitch;
					pDest += pitch;
				}
			}

			nTiles = surfaceHeight / mipHeight;
			for (i = 1; i < nTiles; i++)
			{
				pSrc = (uint8*)lockDesc.lpSurface;
				pDest = (uint8*)lockDesc.lpSurface + pitch * i * mipHeight;
				for (y = 0; y < mipHeight; y++)
				{
					memcpy(pDest, pSrc, surfaceWidth << pFormat->m_PixelShift);
					pSrc += pitch;
					pDest += pitch;
				}
			}
		}

		pSurface->Unlock(LTNULL);

		caps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
		caps.dwCaps2 = 0;
		caps.dwCaps3 = 0;
		caps.dwCaps4 = 0;
		pSurface->GetAttachedSurface(&caps, &pSurface);

		surfaceWidth >>= 1;
		surfaceHeight >>= 1;
	}

	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x10021790 -- make the surface (and its mipmap chain) for a request into
// a texture handle, with the texture's colour key and alpha reference.
// ------------------------------------------------------------------------------ //
inline LTBOOL d3d_ShouldUseS3TC(BPPIdent bpp)
{
	return bpp != BPP_32 && g_CV_S3TCEnable.m_nValue && d3d_IsS3TCSupported(bpp);
}

inline LTBOOL d3d_CreateTextureSurface(TextureRequest *pRequest, RTextureHandle *pHandle,
	uint32 iFirstMip, uint32 nMipmaps, uint32 iFormat)
{
	ConParse cParse;
	PFormat format;
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE7 pSurface;
	TextureData *pTextureData;
	BPPIdent bpp;
	PValue colorKey;
	GenericColor keyColor;
	uint32 realWidth, realHeight;

	pTextureData = pRequest->m_pTextureData;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT | DDSD_TEXTURESTAGE;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_OPAQUE | DDSCAPS2_DONOTPERSIST;
	ddsd.dwWidth = pTextureData->m_Mips[iFirstMip].m_Width;
	ddsd.dwHeight = pTextureData->m_Mips[iFirstMip].m_Height;
	ddsd.dwMipMapCount = nMipmaps;
	ddsd.dwTextureStage = pRequest->m_Stage;

	bpp = pTextureData->m_Header.GetBPPIdent();
	if (d3d_ShouldUseS3TC(bpp))
	{
		memset(&ddsd.ddpfPixelFormat, 0, sizeof(ddsd.ddpfPixelFormat));
		ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
		ddsd.ddpfPixelFormat.dwFlags |= DDPF_FOURCC;
		if (!d3d_GetS3TCFourCC(bpp, &ddsd.ddpfPixelFormat.dwFourCC))
			return LTFALSE;
	}
	else
	{
		memcpy(&ddsd.ddpfPixelFormat, &g_TextureFormats[iFormat]->m_PF, sizeof(ddsd.ddpfPixelFormat));
		d3d_AdjustAspectRatio(ddsd.dwWidth, ddsd.dwHeight, &ddsd.dwWidth, &ddsd.dwHeight);
	}

	// "ColorKey r g b".
	cParse.Init(pTextureData->m_Header.m_CommandString);
	if (cParse.ParseFind("ColorKey", LTFALSE, 3) && !g_CV_AlphaTest.m_nValue)
	{
		colorKey = PValue_Set(0, atoi(cParse.m_Args[1]), atoi(cParse.m_Args[2]), atoi(cParse.m_Args[3]));

		if (d3d_ShouldUseS3TC(bpp))
			format.InitPValueFormat();
		else
			r_SetPFormat(&ddsd.ddpfPixelFormat, &format);

		if (g_FormatMgr.PValueToFormatColor(&format, colorKey, keyColor) == LT_OK)
		{
			ddsd.dwFlags |= DDSD_CKSRCBLT;
			ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = keyColor.dwVal;
			ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = keyColor.dwVal;
		}
	}

	// "AlphaRef n".
	cParse.Init(pTextureData->m_Header.m_CommandString);
	if (cParse.ParseFind("AlphaRef", LTFALSE, 1))
		pHandle->m_AlphaRef = (uint16)atoi(cParse.m_Args[1]);
	else
		pHandle->m_AlphaRef = 0;

	if (g_pDirectDraw->CreateSurface(&ddsd, &pSurface, LTNULL) != DD_OK)
	{
		AddDebugMessage(4, "Unable to create (%dx%d) texture surface.", ddsd.dwWidth, ddsd.dwHeight);
		return LTFALSE;
	}

	pSurface->SetPriority(pTextureData->m_Header.GetTexturePriority());

	// The texture coordinate scale.
	d3d_AdjustAspectRatio(pTextureData->m_Mips[0].m_Width, pTextureData->m_Mips[0].m_Height, &realWidth, &realHeight);
	pHandle->m_fInvWidth = 1.0f / (float)realWidth;
	pHandle->m_fInvHeight = 1.0f / (float)realHeight;
	pHandle->m_fInvWidth *= pTextureData->m_Header.GetUIMipmapScale();
	pHandle->m_fInvHeight *= pTextureData->m_Header.GetUIMipmapScale();

	pHandle->m_pSurface = pSurface;
	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x10021c00 -- the first mipmap small enough for the card.  -1 if none is.
// ------------------------------------------------------------------------------ //
inline int32 d3d_GetFirstUsableMipmap(TextureData *pTexture)
{
	uint32 maxWidth, maxHeight, i;

	maxWidth = LTMIN(g_CV_MaxTextureSize, g_D3DCaps.m_Desc.dwMaxTextureWidth);
	maxHeight = LTMIN(g_CV_MaxTextureSize, g_D3DCaps.m_Desc.dwMaxTextureHeight);

	if (!maxWidth)
		maxWidth = 256;

	if (!maxHeight)
		maxHeight = 256;

	if (!g_D3DCaps.m_bSurfacesLargerThanScreen)
	{
		maxWidth = LTMIN(maxWidth, g_ScreenWidth);
		// RETAIL BUG: clamps the width, not the height, to the screen
		// height, so a texture's height limit is the (clamped) width.
		maxHeight = LTMIN(maxWidth, g_ScreenHeight);
	}

	for (i = 0; i < pTexture->m_Header.m_nMipmaps; i++)
	{
		if (pTexture->m_Mips[i].m_Width <= maxWidth && pTexture->m_Mips[i].m_Height <= maxHeight)
			return i;
	}

	return -1;
}


// ------------------------------------------------------------------------------ //
// 0x10021cd0 -- draw a polygon's animated lightmap: straight into the
// texture bound on the current stage, or into a scratch lightmap texture
// (lmsurfaces.cpp's LMDrawDesc) that is then loaded and bound.
// ------------------------------------------------------------------------------ //
typedef CounterFinal Counter;

void			cnt_StartCounterFinal(CounterFinal &cCounter);  // 0x10013fd5
unsigned long	cnt_EndCounterFinal(CounterFinal &cCounter);    // 0x10013fe0

inline void cnt_StartCounter(Counter &cCounter) { cnt_StartCounterFinal(cCounter); }
inline unsigned long cnt_EndCounter(Counter &cCounter) { return cnt_EndCounterFinal(cCounter); }

class CountAdder
{
	public:
		CountAdder(uint32 *pNum)
		{
			m_pNum = pNum;
			cnt_StartCounter(m_Counter);
		}
		~CountAdder()
		{
			*m_pNum += cnt_EndCounter(m_Counter);
		}
		Counter m_Counter;
		uint32 *m_pNum;
};

struct SceneDesc
{
	uint8		m_pad_00[0x18];
	uint32*		m_pLightmapTicks;                       // +0x18
};

struct SceneWorld;
struct WorldPoly
{
	uint8		m_Pad[0x4c];
	uint8		m_LMWidth;                              // +0x4c
	uint8		m_LMHeight;                             // +0x4d
};

// A lightmap being drawn (lmsurfaces.cpp).
struct LMTextureList;
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

extern SceneDesc*	g_pSceneDesc;                   // 0x1005c14c (common_draw.cpp)
extern SceneWorld*		g_pSceneWorld;                  // 0x1005c208
extern uint32			g_nLitPolysUploaded;            // 0x1005bd10
extern uint32			g_nLightmapUploadSaves;         // 0x1005b774

// Draw the polygon's animated lightmap into the data, 16-bit or 32-bit; 0
// when nothing was drawn (lightanim.cpp).
uint32	d3d_DrawAnimatedLightmap(SceneWorld *pWorld, WorldPoly *pPoly, void *pData, long pitch,
		uint32 width, uint32 height, bool b16Bit);      // 0x10033a20

LTBOOL d3d_DrawPolyLightmap(WorldPoly *pPoly, LTBOOL bToTexture)
{
	CountAdder cntLightmap(g_pSceneDesc->m_pLightmapTicks);

	if (bToTexture)
	{
		RTexture *pTexture = g_pCurTextures[g_TextureStage];
		if (!pTexture)
			return LTTRUE;

		LPDIRECTDRAWSURFACE7 pSurface = pTexture->m_Handle.m_pSurface;
		if (!pSurface)
			return LTTRUE;

		DDSURFACEDESC2 ddsd;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		if (pSurface->Lock(LTNULL, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, LTNULL) < 0)
			return LTTRUE;

		d3d_DrawAnimatedLightmap(g_pSceneWorld, pPoly, ddsd.lpSurface, ddsd.lPitch,
			ddsd.dwWidth, ddsd.dwHeight, ddsd.ddpfPixelFormat.dwRGBBitCount != 32);

		pSurface->Unlock(LTNULL);
		return LTTRUE;
	}
	else
	{
		LMDrawDesc desc;
		if (!desc.Begin(pPoly, LTTRUE, 0, 0))
			return LTFALSE;

		g_nLitPolysUploaded++;

		uint32 pitch = desc.m_Pitch;
		void *pData = desc.m_pData;
		uint32 bDrawn = d3d_DrawAnimatedLightmap(g_pSceneWorld, pPoly, pData, pitch,
			pPoly->m_LMWidth, pPoly->m_LMHeight, desc.m_Format.GetType() != BPP_32);
		if (!bDrawn)
			g_nLightmapUploadSaves++;

		return desc.End(bDrawn) && bDrawn;
	}
}


// ------------------------------------------------------------------------------ //
// Making textures on demand.
// ------------------------------------------------------------------------------ //
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
// 0x10021f70 -- make an RTexture (and its surfaces) for a request: pick the
// format and the mipmaps to use, make the surfaces and record it all.
// ------------------------------------------------------------------------------ //
inline RTexture* d3d_CreateRTexture(TextureRequest *pRequest, uint32 texFlags)
{
	RTextureHandle handle;
	TextureData *pTextureData;
	RTexture *pTexture;
	D3DTextureFormat *pFormat;
	uint32 iFormat, i;
	int32 iGroup, mipOffset, firstUsable, nMipmaps, maxMipmaps;
	float fAngle;

	pTextureData = pRequest->m_pTextureData;

	// Pick the format.
	if (pRequest->m_Stage & TEXSTAGE_BUMPMAP)
	{
		if (!g_TextureFormats[FORMAT_BUMPMAP])
			return LTNULL;

		iFormat = FORMAT_BUMPMAP;
	}
	else if (!(pTextureData->m_Header.m_IFlags & DTX_PREFER16BIT) && g_CV_32BitTextures && g_TextureFormats[FORMAT_32BIT])
	{
		iFormat = FORMAT_32BIT;
	}
	else if (pTextureData->m_Header.m_IFlags & DTX_PREFER5551)
	{
		iFormat = FORMAT_FULLBRITE;
	}
	else if (pTextureData->m_Header.m_IFlags & DTX_PREFER4444)
	{
		iFormat = FORMAT_4444;
	}
	else if (pTextureData->m_Header.m_IFlags & DTX_FULLBRITE)
	{
		iFormat = FORMAT_FULLBRITE;
	}
	else
	{
		iFormat = FORMAT_NORMAL;
	}

	memset(&handle, 0, sizeof(handle));

	// The first mipmap: the texture group's offset, the texture's own and
	// the global one (and, without S3TC, the texture's extra offset).
	iGroup = pTextureData->m_Header.m_Extra[0];
	if (iGroup > 9)
		iGroup = 9;

	mipOffset = g_CV_GroupOffset[iGroup] + pTextureData->m_Header.GetUIMipmapOffset() + g_CV_MipmapOffset;
	if (!g_CV_S3TCEnable.m_nValue)
		mipOffset += pTextureData->m_Header.GetNonS3TCMipmapOffset();

	mipOffset = LTCLAMP(mipOffset, 0, 3);
	if (mipOffset > pTextureData->m_Header.m_nMipmaps - 1)
		mipOffset = pTextureData->m_Header.m_nMipmaps - 1;

	// Make sure we're using a valid size.
	firstUsable = d3d_GetFirstUsableMipmap(pTextureData);
	if (firstUsable == -1)
		return LTNULL;

	mipOffset = LTMAX(mipOffset, firstUsable);

	nMipmaps = pTextureData->m_Header.GetNumMipmaps();
	if (nMipmaps == 0)
		nMipmaps = 4;

	maxMipmaps = pTextureData->m_Header.m_nMipmaps - mipOffset;
	if (maxMipmaps == 0)
		return LTNULL;

	nMipmaps = LTCLAMP(nMipmaps, 1, maxMipmaps);

	if (!d3d_CreateTextureSurface(pRequest, &handle, mipOffset, nMipmaps, iFormat))
		return LTNULL;

	pTexture = g_RTextureBank.Allocate();
	if (!pTexture)
	{
		handle.m_pSurface->Release();
		return LTNULL;
	}

	pTexture->m_pNext = LTNULL;
	pTexture->m_TexFlags = (uint8)texFlags;
	pTexture->m_Stage = (uint8)pRequest->m_Stage;
	pTexture->m_Handle = handle;
	pTexture->m_Handle.m_pTexture = pTexture;
	pTexture->m_FirstMip = (uint8)mipOffset;
	pTexture->m_nMipmaps = (uint8)nMipmaps;
	pTexture->m_FormatIndex = (uint8)iFormat;
	pTexture->m_DetailTextureScale = pTextureData->m_Header.GetDetailTextureScale();

	fAngle = MATH_DEGREES_TO_RADIANS((float)pTextureData->m_Header.GetDetailTextureAngle());
	pTexture->m_DetailTextureAngleC = (float)cos(fAngle);
	pTexture->m_DetailTextureAngleS = (float)sin(fAngle);

	// What the mipmaps take up.
	pFormat = g_TextureFormats[iFormat];
	pTexture->m_Handle.m_MemoryUse = 0;
	for (i = mipOffset; i < (uint32)(mipOffset + nMipmaps); i++)
	{
		pTexture->m_Handle.m_MemoryUse +=
			(pTextureData->m_Mips[i].m_Width * pTextureData->m_Mips[i].m_Height) << pFormat->m_PixelShift;
	}

	pTexture->m_pSharedTexture = pRequest->m_pSharedTexture;
	if (!(texFlags & RT_KEEP))
		pRequest->m_pSharedTexture->m_pRenderData = pTexture;

	pTexture->m_Flags = (uint16)(pTextureData->m_Header.m_IFlags & DTX_FULLBRITE);
	pTexture->m_Width = (uint16)pTextureData->m_Mips[0].m_Width;
	pTexture->m_Height = (uint16)pTextureData->m_Mips[0].m_Height;

	pTexture->m_Link.m_pData = pTexture;
	dl_Insert(&g_RTextures, &pTexture->m_Link);

	g_pStruct->m_SystemTextureMemory += pTexture->m_Handle.m_MemoryUse;
	return pTexture;
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

	if (g_D3DCaps.m_TriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		width = height = (width > height) ? width : height;
	}
	else if (g_CV_MaxTexAspectRatio > 0)
	{
		pSmall = (width > height) ? &height : &width;
		pLarge = (width > height) ? &width : &height;

		ratio = *pLarge / *pSmall;
		if (ratio > g_CV_MaxTexAspectRatio)
			*pSmall = *pLarge / g_CV_MaxTexAspectRatio;
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
		d3d_CreateAndLoadTexture(pSharedTexture, g_iCurTextureStage);
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
