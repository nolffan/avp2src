// *********************************************************************** //
//
//  MODULE:   W18_10035E20_OSOI (.CPP)  -- /Os /Oi-
//
//  PURPOSE:  Two slots of the DirectDraw surface wrapper's vtable 0x10089480
//            (d3dr2c.cpp's C10035bb4: +0x04 width, +0x08 height, +0x0c the
//            IDirectDrawSurface7*), reached only through that table and
//            written for the d3d-link:
//
//    slot +0x04  0x10036460  Init(w, h): make a w x h managed texture in the
//                            offscreen surface's pixel format and fill it
//                            with a radial falloff.
//    slot +0x10  0x10035e20  Draw(x, y, color, bFlag): one textured TL quad
//                            at (x, y), render states saved and restored.
//
//  The table: +0x00 0x10035a70 (dtor), +0x04 Init, +0x08 0x10035c00
//  (GetSize), +0x0c 0x10035c30 (set up the texture stages -- d3dr3d.cpp),
//  +0x10 Draw, +0x14 / +0x18 0x10036130 / 0x100361c0 (the two BltFasts --
//  d3dr5b.cpp), +0x1c 0x10035bd0 (Term).  Init calls Term and Draw calls
//  the stage setup through the vtable, so the class is declared with its
//  eight virtual slots (no constructor here, so no vtable is emitted).
//
//  FLAGS: both pad the /Os way; Init CALLS memset (0x100809a0) and memcpy
//  (0x100805d0) for fixed small sizes, so intrinsics are off.
//
// *********************************************************************** //

#include "ltbasedefs.h"

extern "C"
{
	void*	__cdecl memset(void* pDst, int c, unsigned int nBytes);
	void*	__cdecl memcpy(void* pDst, const void* pSrc, unsigned int nBytes);
}

#pragma function(memset)
#pragma function(memcpy)


// ---------------------------------------------------------------------- //
//  DirectDraw / Direct3D, C-style, the slots used here.
// ---------------------------------------------------------------------- //
struct DDPixelFormat
{
	uint32	dwSize;                                         // +0x00
	uint32	dwFlags;                                        // +0x04
	uint32	dwFourCC;                                       // +0x08
	uint32	dwRGBBitCount;                                  // +0x0c
	uint32	m_Masks[4];                                     // +0x10..+0x1f
};                                                          // 0x20

struct DDSD2Full
{
	uint32			dwSize;                                 // +0x00
	uint32			dwFlags;                                // +0x04
	uint32			dwHeight;                               // +0x08
	uint32			dwWidth;                                // +0x0c
	int32			lPitch;                                 // +0x10
	uint32			dwBackBufferCount;                      // +0x14
	uint32			dwMipMapCount;                          // +0x18
	uint32			dwAlphaBitDepth;                        // +0x1c
	uint32			dwReserved;                             // +0x20
	void*			lpSurface;                              // +0x24
	uint32			ddckCKDestOverlay[2];                   // +0x28
	uint32			ddckCKDestBlt[2];                       // +0x30
	uint32			ddckCKSrcOverlay[2];                    // +0x38
	uint32			ddckCKSrcBltLow;                        // +0x40
	uint32			ddckCKSrcBltHigh;                       // +0x44
	DDPixelFormat	ddpfPixelFormat;                        // +0x48
	uint32			dwCaps;                                 // +0x68
	uint32			dwCaps2;                                // +0x6c
	uint32			dwCaps3;                                // +0x70
	uint32			dwCaps4;                                // +0x74
	uint32			dwTextureStage;                         // +0x78
};                                                          // 0x7c

#define W18_DDSD_CAPS               0x00000001
#define W18_DDSD_HEIGHT             0x00000002
#define W18_DDSD_WIDTH              0x00000004
#define W18_DDSD_PIXELFORMAT        0x00001000
#define W18_DDSCAPS_TEXTURE         0x00001000
#define W18_DDSCAPS2_TEXTUREMANAGE  0x00000010
#define W18_DDLOCK_WAIT             0x00000001
#define W18_DDLOCK_NOSYSLOCK        0x00000800

struct D3DRect
{
	int32 left;                                         // +0x00
	int32 top;                                          // +0x04
	int32 right;                                        // +0x08
	int32 bottom;                                       // +0x0c
};

struct DDSurface7;
struct DDSurface7Vtbl
{
	void*	m_pad_00[21];                                   // +0x00..+0x50
	long	(__stdcall* GetPixelFormat)(DDSurface7*,
	                                    DDPixelFormat* pPF);  // +0x54
	void*	m_pad_58[3];                                    // +0x58..+0x60
	long	(__stdcall* Lock)(DDSurface7*, D3DRect* pRect,
	                          DDSD2Full* pDesc, uint32 dwFlags,
	                          void* hEvent);                // +0x64
	void*	m_pad_68[6];                                    // +0x68..+0x7c
	long	(__stdcall* Unlock)(DDSurface7*, D3DRect* pRect);// +0x80
};
struct DDSurface7 { DDSurface7Vtbl* lpVtbl; };

struct DDraw7;
struct DDraw7Vtbl
{
	void*	m_pad_00[6];                                    // +0x00..+0x14
	long	(__stdcall* CreateSurface)(DDraw7*, DDSD2Full* pDesc,
	                                   DDSurface7** ppSurf,
	                                   void* pOuter);        // +0x18
};
struct DDraw7 { DDraw7Vtbl* lpVtbl; };

struct D3DDevice7;
struct D3DDevice7Vtbl
{
	void*	m_pad_00[20];                                   // +0x00..+0x4c
	long	(__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                    uint32 dwValue);     // +0x50
	long	(__stdcall* GetRenderState)(D3DDevice7*, uint32 dwState,
	                                    uint32* pdwValue);   // +0x54
	void*	m_pad_58[3];                                    // +0x58..+0x60
	long	(__stdcall* DrawPrimitive)(D3DDevice7*, uint32 nPrimType,
	                                   uint32 dwFVF, void* pVerts,
	                                   uint32 nVerts, uint32 dwFlags);// +0x64
	void*	m_pad_68[11];                                   // +0x68..+0x90
	long	(__stdcall* SetTextureStageState)(D3DDevice7*, uint32 nStage,
	                                          uint32 nState,
	                                          uint32 nValue);// +0x94
};
struct D3DDevice7 { D3DDevice7Vtbl* lpVtbl; };

extern DDraw7*     g_pDirectDraw;                       // 0x1008bf74
extern DDSurface7* g_pOffscreen;                        // 0x1008bf68
extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

extern const char* __cdecl sub_10030f30(long hResult);  // 0x10030f30: HRESULT -> text
extern const char* g_1008da30;                          // 0x1008da30: the last error text

#define W18_D3DRS_ZENABLE           7
#define W18_D3DRS_ZWRITEENABLE      14
#define W18_D3DRS_SRCBLEND          19
#define W18_D3DRS_DESTBLEND         20
#define W18_D3DRS_ALPHABLENDENABLE  27
#define W18_D3DBLEND_SRCALPHA       5
#define W18_D3DBLEND_INVSRCALPHA    6
#define W18_D3DTSS_ADDRESS          12
#define W18_D3DTADDRESS_WRAP        1
#define W18_D3DTADDRESS_CLAMP       3
#define W18_D3DPT_TRIANGLEFAN       6
#define W18_D3DFVF_TLVERTEX         0x1c4


// D3DTLVERTEX: the four-vertex quad Draw builds in .data at 0x1008d9b0
// (nothing else in the image references it).
struct TLVert10035e20
{
	float   x, y, z, rhw;                               // +0x00
	uint32  color;                                      // +0x10
	uint32  specular;                                   // +0x14
	float   u, v;                                       // +0x18
};                                                      // 0x20

extern TLVert10035e20 g_1008d9b0[4];                    // 0x1008d9b0


// ---------------------------------------------------------------------- //
//  The wrapper.  The slots this file calls through are named; the rest
//  are placeholders that only put them at their offsets.
// ---------------------------------------------------------------------- //
class C10035e20
{
public:
	virtual void   Slot00(uint32 nFlags);                       // +0x00 0x10035a70
	virtual int32  Slot04(uint32 nWidth, uint32 nHeight);       // +0x04 0x10036460
	virtual void   Slot08(uint32* pW, uint32* pH);              // +0x08 0x10035c00
	virtual int32  SetupStages();                               // +0x0c 0x10035c30
	virtual int32  Slot10(uint32 x, uint32 y, uint32 c, uint8 b);// +0x10 0x10035e20
	virtual int32  Slot14(uint32 x, uint32 y);                  // +0x14 0x10036130
	virtual int32  Slot18(uint32 x, uint32 y);                  // +0x18 0x100361c0
	virtual void   Term();                                      // +0x1c 0x10035bd0

	int32  sub_10035e20(uint32 x, uint32 y, uint32 nColor, uint8 bFlag);
	int32  sub_10036460(uint32 nWidth, uint32 nHeight);

	uint32      m_nWidth;                               // +0x04
	uint32      m_nHeight;                              // +0x08
	DDSurface7* m_pSurface;                             // +0x0c
};


// ======================================================================= //
//  0x10035e20 -- 777 bytes, `ret $0x10`.  Draw the texture as one quad.
// ======================================================================= //
int32 C10035e20::sub_10035e20(uint32 x, uint32 y, uint32 nColor, uint8 bFlag)
{
	D3DDevice7* pDevice;
	uint32      nZEnable, nZWrite, nAlphaBlend, nSrcBlend, nDestBlend;
	int32       i;
	float       fLeft, fTop, fRight, fBottom;

	SetupStages();

	pDevice = g_pD3DDevice;

	pDevice->lpVtbl->GetRenderState(pDevice, W18_D3DRS_ZENABLE, &nZEnable);
	pDevice->lpVtbl->GetRenderState(pDevice, W18_D3DRS_ZWRITEENABLE, &nZWrite);
	pDevice->lpVtbl->GetRenderState(pDevice, W18_D3DRS_ALPHABLENDENABLE, &nAlphaBlend);
	pDevice->lpVtbl->GetRenderState(pDevice, W18_D3DRS_SRCBLEND, &nSrcBlend);
	pDevice->lpVtbl->GetRenderState(pDevice, W18_D3DRS_DESTBLEND, &nDestBlend);

	pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_ZENABLE, 0);
	pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_ZWRITEENABLE, 0);
	pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_ALPHABLENDENABLE, bFlag ? 1 : 0);

	if (!bFlag)
	{
		pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_SRCBLEND, W18_D3DBLEND_SRCALPHA);
		pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_DESTBLEND, W18_D3DBLEND_INVSRCALPHA);
	}

	pDevice->lpVtbl->SetTextureStageState(pDevice, 0, W18_D3DTSS_ADDRESS, W18_D3DTADDRESS_CLAMP);

	for (i = 0; i < 4; i++)
	{
		g_1008d9b0[i].z        = 0.0f;
		g_1008d9b0[i].rhw      = 1.0f;
		g_1008d9b0[i].color    = nColor;
		g_1008d9b0[i].specular = 0;
	}

	fLeft   = (float)x;
	fTop    = (float)y;
	fRight  = (float)m_nWidth + fLeft;
	fBottom = (float)m_nHeight + fTop;

	g_1008d9b0[0].x = fLeft;
	g_1008d9b0[0].y = fTop;
	g_1008d9b0[0].u = 0.0f;
	g_1008d9b0[0].v = 0.0f;

	g_1008d9b0[1].x = fLeft;
	g_1008d9b0[1].y = fBottom;
	g_1008d9b0[1].u = 0.0f;
	g_1008d9b0[1].v = 1.0f;

	g_1008d9b0[2].x = fRight;
	g_1008d9b0[2].y = fBottom;
	g_1008d9b0[2].u = 1.0f;
	g_1008d9b0[2].v = 1.0f;

	g_1008d9b0[3].x = fRight;
	g_1008d9b0[3].y = fTop;
	g_1008d9b0[3].u = 1.0f;
	g_1008d9b0[3].v = 0.0f;

	pDevice->lpVtbl->DrawPrimitive(pDevice, W18_D3DPT_TRIANGLEFAN,
		W18_D3DFVF_TLVERTEX, g_1008d9b0, 4, 0);

	pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_ZENABLE, nZEnable);
	pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_ZWRITEENABLE, nZWrite);
	pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_ALPHABLENDENABLE, nAlphaBlend);
	pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_SRCBLEND, nSrcBlend);
	pDevice->lpVtbl->SetRenderState(pDevice, W18_D3DRS_DESTBLEND, nDestBlend);

	pDevice->lpVtbl->SetTextureStageState(pDevice, 0, W18_D3DTSS_ADDRESS, W18_D3DTADDRESS_WRAP);

	return 1;
}


// ======================================================================= //
//  0x10036460 -- 836 bytes, `ret $0x8`.  Init(w, h).
//
//  A dimension that is not a power of two is rounded DOWN to one, from
//  256 (`n = 0x100; while (!(w & n)) n >>= 1`).  Term() through slot
//  +0x1c, then the offscreen surface's pixel format is copied into a
//  DDSURFACEDESC2 (CAPS|HEIGHT|WIDTH|PIXELFORMAT, TEXTURE, TEXTUREMANAGE)
//  and CreateSurface'd; on success the size and surface are kept.  The
//  whole surface is locked, and every pixel's bytes are set to
//      255 * min(1, 0.25 + t - 0.25 t),  t = 2 d^2 / (cx^2 + cy^2)
//  with d the distance from the centre (cx, cy) = (w/2, h/2); then it is
//  unlocked.  The answer is (char)(Lock's HRESULT == 0).  The error text
//  of GetPixelFormat and CreateSurface goes to 0x1008da30.
// ======================================================================= //
int32 C10035e20::sub_10036460(uint32 nWidth, uint32 nHeight)
{
	int32       bWidthPow2, bHeightPow2;
	uint32      n;
	DDPixelFormat pf;
	DDSD2Full   ddsd;
	DDSurface7* pSurface;
	long        hResult;
	float       fCX, fCY, fScale;
	uint32      nBytesPP, nRowSkip, xx, yy, k;
	uint8*      pBits;
	float       fAlpha;
	uint8       nAlpha;

	bWidthPow2  = (char)((nWidth & (nWidth - 1)) == 0);
	bHeightPow2 = (char)((nHeight & (nHeight - 1)) == 0);

	if (!bWidthPow2)
	{
		n = 0x100;
		while (!(nWidth & n))
			n >>= 1;
		nWidth = n;
	}

	if (!bHeightPow2)
	{
		n = 0x100;
		while (!(nHeight & n))
			n >>= 1;
		nHeight = n;
	}

	Term();

	memset(&pf, 0, sizeof(pf));
	pf.dwSize = sizeof(pf);
	hResult = g_pOffscreen->lpVtbl->GetPixelFormat(g_pOffscreen, &pf);
	g_1008da30 = sub_10030f30(hResult);

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize   = sizeof(ddsd);
	ddsd.dwFlags  = W18_DDSD_CAPS | W18_DDSD_HEIGHT | W18_DDSD_WIDTH | W18_DDSD_PIXELFORMAT;
	ddsd.dwCaps   = W18_DDSCAPS_TEXTURE;
	ddsd.dwCaps2  = W18_DDSCAPS2_TEXTUREMANAGE;
	ddsd.dwHeight = nHeight;
	ddsd.dwWidth  = nWidth;
	memcpy(&ddsd.ddpfPixelFormat, &pf, sizeof(pf));

	pSurface = LTNULL;
	hResult = g_pDirectDraw->lpVtbl->CreateSurface(g_pDirectDraw, &ddsd, &pSurface, LTNULL);
	g_1008da30 = sub_10030f30(hResult);

	if (hResult == 0)
	{
		m_nWidth   = nWidth;
		m_nHeight  = nHeight;
		m_pSurface = pSurface;
	}

	D3DRect rc = { 0, 0, 0, 0 };

	rc.right  = nWidth;
	rc.bottom = nHeight;

	hResult = pSurface->lpVtbl->Lock(pSurface, &rc, &ddsd,
		W18_DDLOCK_WAIT | W18_DDLOCK_NOSYSLOCK, LTNULL);

	if (hResult == 0)
	{
		fCX    = (float)nWidth * 0.5f;
		fCY    = (float)nHeight * 0.5f;
		fScale = 1.0f / (fCX * fCX + fCY * fCY);

		nBytesPP = ddsd.ddpfPixelFormat.dwRGBBitCount >> 3;
		pBits    = (uint8*)ddsd.lpSurface;
		nRowSkip = ((uint32)ddsd.lPitch / nBytesPP - nWidth) * nBytesPP;

		for (yy = 0; yy < nHeight; yy++)
		{
			for (xx = 0; xx < nWidth; xx++)
			{
				float  fDX = (float)xx - fCX;
				float  fDY = (float)yy - fCY;
				float  t   = (fDX * fDX + fDY * fDY) * 2.0f * fScale;

				fAlpha = 0.25f + t - 0.25f * t;
				if (fAlpha > 1.0f)
					fAlpha = 1.0f;

				nAlpha = (uint8)(uint32)(fAlpha * 255.0f);

				for (k = 0; k < nBytesPP; k++)
					pBits[k] = nAlpha;

				pBits += nBytesPP;
			}

			pBits += nRowSkip;
		}

		pSurface->lpVtbl->Unlock(pSurface, &rc);
	}

	return (char)(hResult == 0);
}
