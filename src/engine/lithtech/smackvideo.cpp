// *********************************************************************** //
//
//  MODULE:   smackvideo (.CPP)
//
//  PURPOSE:  Reconstruction of lithtech.exe's SMACKER VIDEO translation
//            unit, 0x0048a5f0 .. 0x0048b4xx.
//
//  The Smacker driver is the Bink driver's twin: vtable 0x004a3890 maps
//  slot for slot onto Bink's, and the ten-slot instance table 0x004a38a4
//  maps 1:1 onto Bink's 0x004a1454. binkvideo.cpp is therefore the shape
//  this file should grow into, function for function:
//
//      smack        bink         what
//      0x0048ab60   0x00401b50   StartOnScreenVideo      <- written here
//      0x0048ab80   0x00401b70   StartTextureVideo       <- written here
//      0x0048aba0   0x00401b90   SmackVideoInst ctor     <- written here
//      0x0048ac20   0x00401c20   SmackVideoInst slot 0
//      0x0048ad6f   0x00401d30   SmackVideoInst::Init
//      0x0048ada0   0x00401dd0   SmackVideoInst::InitScreen
//      0x0048b000   0x00402040   SmackVideoInst::InitTexture
//      0x0048b120   0x00402120   Restore
//      0x0048b160   0x00402150   FreeSurfaces
//      0x0048b270   0x004021a0   UpdateScreen
//      0x0048b290   0x004021c0   Update
//      0x0048b2b0   0x004021e0   GetStatus
//      0x0048b2d0   0x00402200   BindToSurface
//
//  WHAT DOES *NOT* BELONG HERE. Six bodies -- 0x0048a6b0 (UpdateVideo),
//  0x0048a710 (GetVideoStatus), 0x0048a740 (StopVideo), 0x0048b390
//  (VideoInst::Release), 0x0048b1b0 and 0x0048b230 -- appear in BOTH the
//  Bink and the Smack tables at ONE address and exist once in .text.
//  binkvideo.cpp already reads that as the linker folding identical
//  COMDATs, which means the source is INLINE in a header shared by both
//  drivers, not a body in either .cpp. The same goes for the CVideoBinding
//  four at 0x0048a5f0/0x0048a620/0x0048a640/0x0048a680.
//
//  AND ONE THING PLACED ONLY PROVISIONALLY: the factory at 0x0048a770 names
//  BOTH drivers, so it is the video subsystem's entry point rather than
//  Smacker's -- in the real LithTech tree, videomgr.cpp. It is not written
//  here. If a videomgr.cpp is ever opened for the folded bodies above, the
//  factory belongs there with them.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltcodes.h"
#include <string.h>                  // memset (intrinsic under /Oi)

class SmackVideoInst;
struct SmackS;

// Vtable 0x004a3890, five slots. Only the two written here and the
// non-virtual StartVideo they forward to get bodies; the rest are declared
// so the slot indices are real.
class SmackVideoMgr
{
public:
	virtual LTRESULT StartOnScreenVideo(char* pFilename, uint32 nFlags,
	                                    SmackVideoInst*& hVideo);   // 0x0048ab60
	virtual LTRESULT StartTextureVideo(char* pFilename, uint32 nFlags,
	                                   SmackVideoInst*& hVideo);    // 0x0048ab80
	virtual LTRESULT UpdateVideo(SmackVideoInst* hVideo);           // 0x0048a6b0 folded
	virtual LTRESULT GetVideoStatus(SmackVideoInst* hVideo);        // 0x0048a710 folded
	virtual LTRESULT StopVideo(SmackVideoInst* hVideo);             // 0x0048a740 folded

	// 0x0048aa50 -- __thiscall, four stack arguments (`ret $0x10`). The
	// fourth is the on-screen/texture selector and is the ONLY thing that
	// differs between the two slots above.
	LTRESULT StartVideo(char* pFilename, uint32 nFlags,
	                    SmackVideoInst*& hVideo, uint32 bTexture);  // 0x0048aa50

	// 0x04 count, 0x08 head -- the same bare-head list binkvideo.cpp
	// recovered; 0x0c the client manager; 0x14..0x30 the smackw32 entry
	// points; 0x30 the HINSTANCE Term FreeLibrary's.  The four DrawFrame
	// calls (+0x20..+0x2c) are ltnew1.cpp's SmackVideoMgrLtn1 names:
	// _SmackWait@4 / _SmackToBuffer@28 / _SmackDoFrame@4 / _SmackNextFrame@4.
	uint8 m_pad_04[0x20 - 0x04];
	uint32 (__stdcall *m_pSmackWait)(struct SmackS* pSmack);         // 0x20
	void   (__stdcall *m_pSmackToBuffer)(struct SmackS* pSmack, uint32 left,
	                                     uint32 top, uint32 pitch,
	                                     uint32 height, void* pBuf,
	                                     uint32 flags);             // 0x24
	uint32 (__stdcall *m_pSmackDoFrame)(struct SmackS* pSmack);      // 0x28
	void   (__stdcall *m_pSmackNextFrame)(struct SmackS* pSmack);    // 0x2c
	uint8 m_pad_30[0x34 - 0x30];
};


// 0x0048ab60, 25 bytes, slot 0. The selector is pushed FIRST, which is
// right-to-left evaluation of the four-argument call; the third and second
// arguments are loaded into %eax and %edx up front (0x0048ab60, 0x0048ab64)
// and the first is re-read from the already-shifted stack at 0x0048ab6b,
// which is what makes `mov 0x4(%esp)` read as `0xc`.
//
// `this` is never materialised in either body. That is exactly what a direct
// call to a __thiscall member of the same object compiles to when %ecx
// arrives holding it and nothing in between touches %ecx; a call through the
// vtable, or on some other object, would have to load %ecx.
//
// The selector is spelled as a bare 0/1 rather than LTFALSE/LTTRUE -- both
// are `push $0x0` and nothing in the image tells them apart, but
// binkvideo.cpp's reconstruction of the callee already takes it as
// `uint32 bTexture`, and 0/1 is how a uint32 selector is spelled.
LTRESULT SmackVideoMgr::StartOnScreenVideo(char* pFilename, uint32 nFlags,
                                           SmackVideoInst*& hVideo)
{
	return StartVideo(pFilename, nFlags, hVideo, 0);
}


// 0x0048ab80, 25 bytes, slot 1. The same body with `push $0x1`.
LTRESULT SmackVideoMgr::StartTextureVideo(char* pFilename, uint32 nFlags,
                                          SmackVideoInst*& hVideo)
{
	return StartVideo(pFilename, nFlags, hVideo, 1);
}


// ======================================================================= //
//  0x0048b2d0, 192 bytes -- SmackVideoInst::BindToSurface, slot 7 of the
//  instance table 0x004a38a4.
//
//  HOME: the Smack instance bodies (0x0048b120 Restore, 0x0048b160
//  FreeSurfaces) are written in cmgsurf.cpp PART A; this one belongs with
//  them in the Smack TU. It is written here, in this file's own terms.
//
//  It is binkvideo.cpp's 0x00402200 byte for byte except one displacement:
//  `lea 0x68(%esi),%ecx` (0x0048b308) against Bink's `lea 0x28(%esi)` --
//  the embedded SharedTexture sits at +0x68 in the 0xa4-byte Smack
//  instance. The error literal is "SmackVideoInst::BindToSurface"
//  (0x004af684, pushed at 0x0048b378).
// ======================================================================= //

extern "C" void dsi_OnReturnErrorS(int code);           // 0x0042d760
extern int      g_DebugLevelS;                          // 0x004b0ab0
extern char*    g_pErrorFormatS;                        // 0x004a7110
void            dsi_ServerErrorS(char* pFormat, ...);   // 0x0042e6f0

#define S_ERRSTR_(x) #x
#define S_ERRSTR(x)  S_ERRSTR_(x)

#define S_RETURN_ERROR(level, fnName, code) \
	{ \
		dsi_OnReturnErrorS(code); \
		if (g_DebugLevelS >= level) \
			dsi_ServerErrorS(g_pErrorFormatS, fnName, S_ERRSTR(code), " "); \
		return code; \
	}

#define S_LT_OUTOFMEMORY 67

class LTLinkS
{
public:
	LTLinkS* m_pPrev;                        // 0x00
	LTLinkS* m_pNext;                        // 0x04
	void*    m_pData;                        // 0x08
};

// binkvideo.cpp's bare-head list: count at +0x00, head at +0x04.
struct LTListS
{
	uint32   m_nElements;                    // 0x00
	LTLinkS* m_pHead;                        // 0x04
};

// binkvideo.cpp's k_AddToList, the hand-rolled insert (0x0048b311..).
inline void s_AddToList(LTListS* pList, LTLinkS* pLink, void* pObj)
{
	if (pList->m_pHead)
	{
		LTLinkS* pAfter = pList->m_pHead->m_pPrev;

		pAfter->m_pNext->m_pPrev = pLink;
		pLink->m_pNext = pAfter->m_pNext;
		pLink->m_pPrev = pAfter;
		pAfter->m_pNext = pLink;
		pLink->m_pData = pObj;
	}
	else
	{
		pLink->m_pPrev = pLink->m_pNext = pLink;
		pLink->m_pData = pObj;
		pList->m_pHead = pLink;
	}

	++pList->m_nElements;
}

// binkvideo.cpp's CBankLinkK: constructor 0x0045a350, Init 0x0045a370,
// both out of class (the shipped code CALLS both).  Spelled with the name
// the definitions carry (src/engine/server/netmgr.cpp's
// ::CModelSub14, placed at both addresses) so the calls link to them.
class CModelSub14
{
public:
	CModelSub14();                           // 0x0045a350
	LTBOOL Init(void* pOwner);               // 0x0045a370

public:
	void* m_pNextFree;                       // 0x00
	void* m_pOwner;                          // 0x04
};

// clienttexmgr.cpp's SharedTexture, 0x34 bytes -- binkvideo.cpp's
// SharedTextureK field for field, constructor verbatim.
class SharedTextureS
{
public:
	SharedTextureS();

public:
	CModelSub14 m_Bank;                      // 0x00
	void*      m_pEngineData;                // 0x08
	void*      m_pRenderData;                // 0x0c
	LTLinkS    m_Link;                       // 0x10
	uint32     m_Unknown1c[2];               // 0x1c, 0x20
	void*      m_pFileRef;                   // 0x24
	uint32     m_Unknown28;                  // 0x28
	uint32     m_Unknown2c;                  // 0x2c
	uint16     m_nFrameCode;                 // 0x30
	uint16     m_nRefCount : 15;             // 0x32
	uint16     m_bMarked   : 1;              // 0x32 bit 15
};

// Expanded at 0x0048abc4..0x0048abed exactly as 0x00401b90 expands it:
// the +0x1c pair is an inline /Oi memset (own pointer, own zero) and +0x32
// is one whole-word store of both bitfields.
inline SharedTextureS::SharedTextureS()
{
	m_pEngineData = LTNULL;
	m_pRenderData = LTNULL;
	m_Link.m_pData = this;
	memset(&m_Unknown1c, 0, sizeof(m_Unknown1c));
	m_pFileRef = LTNULL;
	m_Bank.Init(this);
	m_Unknown28 = 0;
	m_nRefCount = 0;
	m_bMarked = 0;
	m_nFrameCode = 0;
}

// lttexmod.cpp's PFormat: one virtual (vtable 0x004a1ab4), 0x38 bytes, and
// NO declared constructor -- its whole construction is the implicit
// `movl $0x4a1ab4,0x2c(%esi)` at 0x0048abb8.
class PFormatS
{
public:
	virtual void Init(uint32 nBitDepth, uint32 aMask, uint32 rMask,
	                  uint32 gMask, uint32 bMask);          // 0x0045f0e0

	// 0x004476b0, `mov 0x4(%ecx),%eax; ret`, called out of line
	// (lttexmod.cpp's PFormat::GetType).
	uint32 GetType();                                       // 0x004476b0

public:
	uint32 m_nBitDepth;                      // 0x04
	uint32 m_Masks[4];                       // 0x08
	uint32 m_nBits[4];                       // 0x18
	uint32 m_nShifts[4];                     // 0x28
};

// binkvideo.cpp's CVideoPolyK: only +0x30 is reached.
class CVideoPolyS
{
public:
	uint8           m_pad_00[0x30];
	SharedTextureS* m_pTexture;              // 0x30
};

// binkvideo.cpp's CVideoBindingK, 0x2c bytes; the three members called
// here are the shared (folded) bodies 0x0048a5f0 / 0x0048a640 / 0x0048a680.
class CVideoBindingS
{
public:
	CVideoBindingS(SmackVideoInst* pInst);             // 0x0048a5f0
	void SetPoly(CVideoPolyS* pPoly);                  // 0x0048a640
	void SetOldTexture(SharedTextureS* pTexture);      // 0x0048a680

public:
	uint8   m_pad_00[0x20];
	LTLinkS m_Link;                          // 0x20
};

// ----------------------------------------------------------------------- //
//  What InitScreen / InitTexture (0x0048ada0 / 0x0048b000) reach.  Every
//  type below is binkvideo.cpp's K twin, re-suffixed.
// ----------------------------------------------------------------------- //

// RAD's smack.h Smack header: Width +0x04, Height +0x08, and the 256-entry
// RGB palette at +0x6c (0x0048b0c1 `add $0x6d` then reads -1/0/+1).
struct SmackS
{
	uint32 Version;                          // 0x00
	uint32 Width;                            // 0x04
	uint32 Height;                           // 0x08
	uint8  m_pad_0c[0x6c - 0x0c];
	uint8  Palette[256 * 3];                 // 0x6c
};

// lttexmod.cpp's TextureData, the two flag words touched here.
class TextureDataS
{
public:
	uint8  m_pad_00[0x18];
	uint32 m_Flags;                          // 0x18   bit 0x40 = TDFLAG_TEXMOD
	uint8  m_pad_1c[0xc4 - 0x1c];
	uint32 m_FlagsC4;                        // 0xc4   (name ours)
};

TextureDataS* tex_CreateTextureDataS(uint32 nBitDepth, uint32 nWidth, uint32 nHeight,
                                     uint32 nMipmaps, uint32 nUnknown5,
                                     uint32 nUnknown6, uint32 bBumpmap); // 0x0042e8e0
void          rif_BindTextureS(SharedTextureS* pTexture, LTBOOL bChanged);  // 0x004628e0

// clientrender.cpp's RenderStruct at 0x004b4358, the two members used here.
// The "inited" flag at 0x004b439c is its +0x44 m_bInited, not a global of its
// own, so both references are to the one g_RenderStruct.
class RenderStruct
{
public:
	uint8  m_pad_00[0x3c];
	int    m_Width;                          // 0x3c  -> 0x004b4394
	int    m_Height;                         // 0x40  -> 0x004b4398
	uint32 m_bInited;                        // 0x44  -> 0x004b439c
	uint8  m_pad_48[0xb8 - 0x48];
	void*  (*GetHook)(char* pName);          // 0xb8  -> 0x004b4410
};

extern RenderStruct g_RenderStruct;                      // 0x004b4358

struct DDPIXELFORMATS
{
	uint32 dwSize;                           // 0x00
	uint32 dwFlags;                          // 0x04
	uint32 dwFourCC;                         // 0x08
	uint32 dwRGBBitCount;                    // 0x0c
	uint32 dwRBitMask;                       // 0x10
	uint32 dwGBitMask;                       // 0x14
	uint32 dwBBitMask;                       // 0x18
	uint32 dwRGBAlphaBitMask;                // 0x1c
};

struct DDSCAPS2S
{
	uint32 dwCaps;
	uint32 dwCaps2;
	uint32 dwCaps3;
	uint32 dwCaps4;
};

struct DDSURFACEDESC2S                       // 0x7c bytes
{
	uint32         dwSize;                   // 0x00
	uint32         dwFlags;                  // 0x04
	uint32         dwHeight;                 // 0x08
	uint32         dwWidth;                  // 0x0c
	long           lPitch;                   // 0x10
	uint32         dwBackBufferCount;        // 0x14
	uint32         dwMipMapCount;            // 0x18
	uint32         dwAlphaBitDepth;          // 0x1c
	uint32         dwReserved;               // 0x20
	void*          lpSurface;                // 0x24
	uint32         ddckCKDestOverlay[2];     // 0x28
	uint32         ddckCKDestBlt[2];         // 0x30
	uint32         ddckCKSrcOverlay[2];      // 0x38
	uint32         ddckCKSrcBlt[2];          // 0x40
	DDPIXELFORMATS ddpfPixelFormat;          // 0x48
	DDSCAPS2S      ddsCaps;                  // 0x68
	uint32         dwTextureStage;           // 0x78
};

struct DDBLTFXS                              // 0x64 bytes
{
	uint32 dwSize;
	uint32 dwPad[24];
};

#define S_DDSD_CAPS               0x00000001
#define S_DDSD_HEIGHT             0x00000002
#define S_DDSD_WIDTH              0x00000004
#define S_DDSCAPS_SYSTEMMEMORY    0x00000800
#define S_DDBLT_COLORFILL         0x00000400
#define S_DDBLT_DEPTHFILL         0x00000800

class IDirectDrawSurfaceS
{
public:
	virtual long __stdcall QueryInterface(void*, void**);          // 0x00
	virtual unsigned long __stdcall AddRef();                      // 0x04
	virtual unsigned long __stdcall Release();                     // 0x08
	virtual long __stdcall AddAttachedSurface(IDirectDrawSurfaceS*);
	virtual long __stdcall AddOverlayDirtyRect(void*);             // 0x10
	virtual long __stdcall Blt(void*, IDirectDrawSurfaceS*,
	                           void*, uint32, DDBLTFXS*);          // 0x14
	// The rest of IDirectDrawSurface7 up to Unlock, for DrawFrame
	// (BltFast +0x1c, Lock +0x64, Unlock +0x80).
	virtual long __stdcall BltBatch(void*, uint32, uint32);        // 0x18
	virtual long __stdcall BltFast(uint32, uint32, IDirectDrawSurfaceS*,
	                               void*, uint32);                 // 0x1c
	virtual long __stdcall DeleteAttachedSurface(uint32, IDirectDrawSurfaceS*);
	virtual long __stdcall EnumAttachedSurfaces(void*, void*);
	virtual long __stdcall EnumOverlayZOrders(uint32, void*, void*);
	virtual long __stdcall Flip(IDirectDrawSurfaceS*, uint32);     // 0x2c
	virtual long __stdcall GetAttachedSurface(void*, IDirectDrawSurfaceS**);
	virtual long __stdcall GetBltStatus(uint32);
	virtual long __stdcall GetCaps(void*);
	virtual long __stdcall GetClipper(void**);                     // 0x3c
	virtual long __stdcall GetColorKey(uint32, void*);
	virtual long __stdcall GetDC(void**);
	virtual long __stdcall GetFlipStatus(uint32);
	virtual long __stdcall GetOverlayPosition(long*, long*);       // 0x4c
	virtual long __stdcall GetPalette(void**);
	virtual long __stdcall GetPixelFormat(void*);
	virtual long __stdcall GetSurfaceDesc(void*);
	virtual long __stdcall Initialize(void*, void*);               // 0x5c
	virtual long __stdcall IsLost();
	virtual long __stdcall Lock(void*, struct DDSURFACEDESC2S*, uint32,
	                            void*);                            // 0x64
	virtual long __stdcall ReleaseDC(void*);
	virtual long __stdcall Restore();
	virtual long __stdcall SetClipper(void*);                      // 0x70
	virtual long __stdcall SetColorKey(uint32, void*);
	virtual long __stdcall SetOverlayPosition(long, long);
	virtual long __stdcall SetPalette(void*);
	virtual long __stdcall Unlock(void*);                          // 0x80
};

class IDirectDrawS
{
public:
	virtual long __stdcall QueryInterface(void*, void**);          // 0x00
	virtual unsigned long __stdcall AddRef();                      // 0x04
	virtual unsigned long __stdcall Release();                     // 0x08
	virtual long __stdcall Compact();                              // 0x0c
	virtual long __stdcall CreateClipper(uint32, void**, void*);
	virtual long __stdcall CreatePalette(uint32, void*, void**, void*);
	virtual long __stdcall CreateSurface(DDSURFACEDESC2S*,
	                                     IDirectDrawSurfaceS**,
	                                     void*);                   // 0x18
	virtual long __stdcall DuplicateSurface(IDirectDrawSurfaceS*,
	                                        IDirectDrawSurfaceS**);
	virtual long __stdcall EnumDisplayModes(uint32, DDSURFACEDESC2S*,
	                                        void*, void*);         // 0x20
	virtual long __stdcall EnumSurfaces(uint32, DDSURFACEDESC2S*,
	                                    void*, void*);             // 0x24
	virtual long __stdcall FlipToGDISurface();                     // 0x28
	virtual long __stdcall GetCaps(void*, void*);                  // 0x2c
	virtual long __stdcall GetDisplayMode(DDSURFACEDESC2S*);       // 0x30
};

// clienttexmgr.cpp's sub_4209b0: DDPIXELFORMAT -> PFormat, cdecl.
void sub_4209b0S(DDPIXELFORMATS* pPF, PFormatS* pFormat);  // 0x004209b0

#define S_RETURN_ERROR_INFO(level, fnName, code, info) \
	{ \
		dsi_OnReturnErrorS(code); \
		if (g_DebugLevelS >= level) \
			dsi_ServerErrorS(g_pErrorFormatS, fnName, #code, info); \
		return code; \
	}




// binkvideo.cpp's VideoInstK: the abstract base behind vtable 0x004a147c,
// 0x18 bytes, its constructor inline (0x0048abaa..0x0048abb3).
class VideoInstS
{
public:
	VideoInstS() { m_Bindings.m_nElements = 0; m_Bindings.m_pHead = LTNULL; }
	virtual ~VideoInstS() {}                                   // slot 0
	virtual void     Release() {}                              // slot 1
	virtual void     Restore() = 0;                            // slot 2
	virtual void     FreeSurfaces() = 0;                       // slot 3
	virtual LTRESULT UpdateScreen() = 0;                       // slot 4
	virtual LTRESULT Update() = 0;                             // slot 5
	virtual LTRESULT GetStatus() = 0;                          // slot 6
	virtual LTRESULT BindToSurface(CVideoPolyS* pPoly) = 0;    // slot 7
	virtual void     UnbindFromSurface(CVideoPolyS* pPoly) = 0;  // slot 8
	virtual void     OnTextureFreed(SharedTextureS* pTexture) = 0;  // slot 9

public:
	LTLinkS  m_Link;                         // 0x04  the manager's list
	LTListS  m_Bindings;                     // 0x10 count, 0x14 head
};

// 0xa4 bytes. Slots as cmgsurf.cpp PART A indexes them from 0x004a38a4.
class SmackVideoInst : public VideoInstS
{
public:
	SmackVideoInst(SmackVideoMgr* pMgr);                       // 0x0048aba0
	virtual ~SmackVideoInst();                                 // slot 0
	virtual void     Release();                                // slot 1
	virtual void     Restore();                                // slot 2
	virtual void     FreeSurfaces();                           // slot 3
	virtual LTRESULT UpdateScreen();                           // slot 4
	virtual LTRESULT Update();                                 // slot 5
	virtual LTRESULT GetStatus();                              // slot 6
	virtual LTRESULT BindToSurface(CVideoPolyS* pPoly);        // slot 7, 0x0048b2d0
	virtual void     UnbindFromSurface(CVideoPolyS* pPoly);    // slot 8
	virtual void     OnTextureFreed(SharedTextureS* pTexture); // slot 9

	LTRESULT InitScreen();                                     // 0x0048ada0
	LTRESULT InitTexture();                                    // 0x0048b000
	int      IsDone();                                         // 0x0048b410 (ltnew1.cpp)
	LTRESULT DrawFrame();                                      // 0x0048b440

public:
	// Field roles from InitScreen / InitTexture (0x0048ada0 / 0x0048b000):
	// +0x20 is the surface Blt clears, +0x24 is set to 1 only on the
	// non-565 arm that creates the SECOND surface at +0x28, and +0x64 is
	// what tex_CreateTextureData's answer is stored into.  Names ours.
	SmackS*               m_pSmack;          // 0x18
	SmackVideoMgr*        m_pMgr;            // 0x1c
	IDirectDrawSurfaceS*  m_pSurface;        // 0x20
	uint32                m_bConvert;        // 0x24
	IDirectDrawSurfaceS*  m_pSurface2;       // 0x28
	PFormatS              m_Format;          // 0x2c .. 0x63
	TextureDataS*         m_pTextureData;    // 0x64
	SharedTextureS        m_Texture;         // 0x68 .. 0x9b
	uint32                m_nFlags;          // 0x9c  bit 0: stretch to screen
	uint8                 m_pad_a0[0xa4 - 0xa0];
};

// ======================================================================= //
//  0x0048aba0, 113 bytes -- SmackVideoInst::SmackVideoInst
//
//  binkvideo.cpp's 0x00401b90 with two extra members: the PFormat vptr at
//  +0x2c (0x0048abb8, before the SharedTexture expansion -- declaration
//  order) and one more zero at +0x28 (0x0048ac06), stored LAST.  The
//  derived stores are +0x1c (the argument), +0x18, +0x20, +0x24, +0x64,
//  +0x28 from one zero register.
// ======================================================================= //

SmackVideoInst::SmackVideoInst(SmackVideoMgr* pMgr)
{
	m_pMgr = pMgr;
	m_pSmack = LTNULL;
	m_pSurface = LTNULL;
	m_bConvert = 0;
	m_pTextureData = LTNULL;
	m_pSurface2 = LTNULL;
}

LTRESULT SmackVideoInst::BindToSurface(CVideoPolyS* pPoly)
{
	CVideoBindingS* pBind;

	pBind = new CVideoBindingS(this);
	if (pBind)
	{
		pBind->SetPoly(pPoly);
		pBind->SetOldTexture(pPoly->m_pTexture);

		pPoly->m_pTexture = &m_Texture;

		s_AddToList(&m_Bindings, &pBind->m_Link, pBind);

		return LT_OK;
	}

	S_RETURN_ERROR(1, "SmackVideoInst::BindToSurface", S_LT_OUTOFMEMORY);
}


// ======================================================================= //
//  0x0048ada0, 606 bytes -- SmackVideoInst::InitScreen
//
//  binkvideo.cpp's 0x00401dd0 with the cursor probe gone and the format
//  cascade replaced:
//    * FreeSurfaces through the vtable (slot 3, `call *0xc(%eax)`), then
//      the same three guards `je` ONE LT_NOTINITIALIZED block at the end
//      (0x0048afbd) -- the success path is the body of an `if`.
//    * the display mode's pixel format goes through 0x004209b0 into the
//      PFormat at +0x2c; 0x004476b0 (GetType) == 2 plus the R/G/B masks at
//      +0x38/+0x3c/+0x40 picks the direct 565 path.  ddsCaps is
//      0x800 = DDSCAPS_SYSTEMMEMORY, Blt flags 0xc00 (no DDBLT_WAIT).
//    * otherwise +0x24 = 1 (sharing %edi = 1 with the error code), the
//      SECOND surface at +0x28 is created in the display format, and the
//      main one is then forced to 16-bit 565.  The failure after the
//      +0x28 create clears +0x20, not +0x28 (0x0048aefb) -- SIC.
//    * the convert arm is written FIRST, under the negated `||` test.
//      With the 565 arm first (same layout, same size) VC6 hosts the shared
//      LT_ERROR report tail at the GetDisplayMode site and the 565 failure
//      jumps back to it; shipped hosts it at the 565 failure (0x0048aeba)
//      and GetDisplayMode's report jumps forward.  Measured.
//    * error literals: 0x004af64c "SmackVideoInst::InitScreen", the
//      two INFO descriptions 0x004a7390 / 0x004a736c shared with Bink.
// ======================================================================= //

LTRESULT SmackVideoInst::InitScreen()
{
	DDSURFACEDESC2S ddsd;
	DDBLTFXS        bltfx;
	IDirectDrawS*   pDD;

	FreeSurfaces();

	if (m_pSmack && g_RenderStruct.m_bInited &&
	    (pDD = (IDirectDrawS*)g_RenderStruct.GetHook("LPDIRECTDRAW")) != LTNULL)
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		if (pDD->GetDisplayMode(&ddsd) != 0)
			S_RETURN_ERROR_INFO(1, "SmackVideoInst::InitScreen", LT_ERROR,
			                    "IDirectDraw::GetDisplayMode failed");

		ddsd.dwWidth = m_pSmack->Width;
		ddsd.dwHeight = m_pSmack->Height;
		ddsd.dwFlags = S_DDSD_CAPS | S_DDSD_HEIGHT | S_DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = S_DDSCAPS_SYSTEMMEMORY;

		sub_4209b0S(&ddsd.ddpfPixelFormat, &m_Format);

		if (m_Format.GetType() != 2 ||
		    m_Format.m_Masks[1] != 0xF800 ||
		    m_Format.m_Masks[2] != 0x07E0 ||
		    m_Format.m_Masks[3] != 0x001F)
		{
			m_bConvert = 1;

			if (pDD->CreateSurface(&ddsd, &m_pSurface2, LTNULL) != 0)
			{
				m_pSurface = LTNULL;             // SIC -- 0x0048aefb writes +0x20
				S_RETURN_ERROR_INFO(1, "SmackVideoInst::InitScreen", LT_ERROR,
				                    "IDirectDraw::CreateSurface failed");
			}

			ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
			ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
			ddsd.ddpfPixelFormat.dwGBitMask = 0x07E0;
			ddsd.ddpfPixelFormat.dwBBitMask = 0x001F;

			if (pDD->CreateSurface(&ddsd, &m_pSurface, LTNULL) != 0)
			{
				m_pSurface = LTNULL;
				S_RETURN_ERROR_INFO(1, "SmackVideoInst::InitScreen", LT_ERROR,
				                    "IDirectDraw::CreateSurface failed");
			}
		}
		else
		{
			if (pDD->CreateSurface(&ddsd, &m_pSurface, LTNULL) != 0)
			{
				m_pSurface = LTNULL;
				S_RETURN_ERROR_INFO(1, "SmackVideoInst::InitScreen", LT_ERROR,
				                    "IDirectDraw::CreateSurface failed");
			}
		}

		memset(&bltfx, 0, sizeof(bltfx));
		bltfx.dwSize = sizeof(bltfx);

		m_pSurface->Blt(LTNULL, LTNULL, LTNULL,
		                S_DDBLT_COLORFILL | S_DDBLT_DEPTHFILL, &bltfx);

		return LT_OK;
	}

	S_RETURN_ERROR(1, "SmackVideoInst::InitScreen", LT_NOTINITIALIZED);
}


// ======================================================================= //
//  0x0048b000, 286 bytes -- SmackVideoInst::InitTexture
//
//  binkvideo.cpp's 0x00402040 plus a palette copy: `sub $0x400,%esp` is a
//  256-entry four-byte local that the loop at 0x0048b0c9 fills from the
//  Smack header's RGB palette (+0x6c) into bytes 1..3 of each entry, with
//  a separate down-counter (`mov $0x100,%ebp ... dec %ebp; jne`).  Nothing
//  reads the local afterwards.  The TextureData flag at +0xc4 is 2 here
//  (Bink: 4).  Error literal 0x004af668 "SmackVideoInst::InitTexture".
// ======================================================================= //

struct SmackPalEntryS                        // name ours
{
	uint8 a, r, g, b;
};

LTRESULT SmackVideoInst::InitTexture()
{
	SmackPalEntryS pal[256];
	uint32         i;

	FreeSurfaces();

	if (!m_pSmack)
		S_RETURN_ERROR(1, "SmackVideoInst::InitTexture", LT_NOTINITIALIZED);

	m_pTextureData = tex_CreateTextureDataS(3, m_pSmack->Width, m_pSmack->Height,
	                                        1, 0, 0, 0);
	if (!m_pTextureData)
		S_RETURN_ERROR(1, "SmackVideoInst::InitTexture", LT_OUTOFMEMORY);

	for (i = 0; i < 256; i++)
	{
		pal[i].r = m_pSmack->Palette[i * 3 + 0];
		pal[i].g = m_pSmack->Palette[i * 3 + 1];
		pal[i].b = m_pSmack->Palette[i * 3 + 2];
	}

	m_pTextureData->m_FlagsC4 |= 2;
	m_pTextureData->m_Flags |= 0x40;
	m_Texture.m_pEngineData = m_pTextureData;

	rif_BindTextureS(&m_Texture, 1);

	return LT_OK;
}


// ======================================================================= //
//  0x0048b440, 987 bytes -- SmackVideoInst::DrawFrame (ltnew1.cpp's name;
//  the error literal says "SmackVideoInst::UpdateOnScreen", 0x004af6e4).
//  ltnew1.cpp's Update (0x0048b290) tail-jumps here.  binkvideo.cpp's
//  0x004022e0 (BinkVideoInst::UpdateOnScreen) is the twin, with:
//    * no software-cursor probe; the lock is DDLOCK_WRITEONLY alone (0x20)
//      and _SmackToBuffer@28 gets (smack, 0, 0, pitch, Height, bits,
//      0xc0000000);
//    * a conversion arm on m_bConvert (+0x24): both surfaces are locked
//      (and never unlocked -- SIC) and the blitter 0x0045f3d0 converts the
//      565 source (format read back off the lock) into the display-format
//      surface +0x28, which is then what is blitted;
//    * Blt / BltFast flags 0 (no DDBLT_WAIT / DDBLTFAST_WAIT);
//    * five LT_NOTINITIALIZED reports at level 2; the last one names
//      "SmackVideoImpl::UpdateOnScreen" (0x004af6a4) -- SIC.
//    * measured: the PFormat local is declared BEFORE the request (its vptr
//      store precedes the constructor call), and the request is filled
//      data, format, pitch per side -- the only order of those tried that
//      gives shipped's register choice.  Byte-identical.
// ======================================================================= //

// cissurf.cpp's caller-side view of the blitter (0x0040c120 accessor,
// 0x0045f240 request constructor, 0x0045f3d0 convert); spelled with the
// same names so the calls link to what cissurf.cpp's already resolve to.
class BlitRequestQ40C
{
public:
	BlitRequestQ40C();                       // 0x0045f240

	PFormatS*    m_pSrcFormat;               // 0x00
	void*        m_pSrc;                     // 0x04
	long         m_nSrcPitch;                // 0x08
	void*        m_pSrcPalette;              // 0x0c
	PFormatS*    m_pDestFormat;              // 0x10
	void*        m_pDest;                    // 0x14
	long         m_nDestPitch;               // 0x18
	uint32       m_nWidth;                   // 0x1c
	uint32       m_nHeight;                  // 0x20
	void*        m_pDestPalette;             // 0x24
	uint8        m_SrcFormat[0x38];          // 0x28
	uint8        m_DestFormat[0x38];         // 0x60
};

class CBlitterQ40C
{
public:
	LTRESULT Blt(BlitRequestQ40C* pRequest);                   // 0x0045f3d0
};

CBlitterQ40C* cis_GetBlitterQ40C();                            // 0x0040c120

struct RECTS
{
	long left, top, right, bottom;
};

#define S_DDLOCK_WRITEONLY 0x00000020

LTRESULT SmackVideoInst::DrawFrame()
{
	RECTS                srcRect;
	IDirectDrawSurfaceS* pDisplay;
	RECTS                destRect;
	uint32               nWait;
	IDirectDrawSurfaceS* pBackBuffer;
	DDSURFACEDESC2S      ddsd;

	pBackBuffer = (IDirectDrawSurfaceS*)g_RenderStruct.GetHook("BACKBUFFER");
	if (!pBackBuffer)
		S_RETURN_ERROR_INFO(2, "SmackVideoInst::UpdateOnScreen", LT_NOTINITIALIZED,
		                    "GetHook(BACKBUFFER) failed");

	if (IsDone())
		return LT_OK;

	nWait = m_pMgr->m_pSmackWait(m_pSmack);
	if (!nWait)
	{
		ddsd.dwSize = sizeof(ddsd);

		if (m_pSurface->Lock(LTNULL, &ddsd, S_DDLOCK_WRITEONLY, LTNULL) == 0)
		{
			m_pMgr->m_pSmackToBuffer(m_pSmack, 0, 0, ddsd.lPitch, m_pSmack->Height,
			                         ddsd.lpSurface, 0xC0000000);
			m_pMgr->m_pSmackDoFrame(m_pSmack);
			m_pSurface->Unlock(LTNULL);
		}
	}

	if (m_bConvert)
	{
		PFormatS        srcFormat;
		BlitRequestQ40C request;
		DDSURFACEDESC2S ddsd2;

		if (!m_pSurface2)
			S_RETURN_ERROR_INFO(2, "SmackVideoInst::UpdateOnScreen", LT_NOTINITIALIZED,
			                    "No conversion surface for Smacker available!");

		ddsd.dwSize = sizeof(ddsd);
		if (m_pSurface->Lock(LTNULL, &ddsd, S_DDLOCK_WRITEONLY, LTNULL) != 0)
			S_RETURN_ERROR_INFO(2, "SmackVideoInst::UpdateOnScreen", LT_NOTINITIALIZED,
			                    "Unable to lock source surface.");

		ddsd2.dwSize = sizeof(ddsd2);
		if (m_pSurface2->Lock(LTNULL, &ddsd2, S_DDLOCK_WRITEONLY, LTNULL) != 0)
			S_RETURN_ERROR_INFO(2, "SmackVideoInst::UpdateOnScreen", LT_NOTINITIALIZED,
			                    "Unable to lock conversion surface.");

		sub_4209b0S(&ddsd.ddpfPixelFormat, &srcFormat);

		request.m_pSrc = ddsd.lpSurface;
		request.m_pSrcFormat = &srcFormat;
		request.m_nSrcPitch = ddsd.lPitch;
		request.m_pDest = ddsd2.lpSurface;
		request.m_pDestFormat = &m_Format;
		request.m_nDestPitch = ddsd2.lPitch;
		request.m_nWidth = m_pSmack->Width;
		request.m_nHeight = m_pSmack->Height;

		if (cis_GetBlitterQ40C()->Blt(&request) == LT_OK)
			pDisplay = m_pSurface2;
		else
			pDisplay = LTNULL;
	}
	else
	{
		pDisplay = m_pSurface;
	}

	if (!pDisplay)
		S_RETURN_ERROR_INFO(2, "SmackVideoImpl::UpdateOnScreen", LT_NOTINITIALIZED,
		                    "Smacker display buffer is null!");

	if (m_nFlags & 1)
	{
		srcRect.left = srcRect.top = 0;
		srcRect.right = m_pSmack->Width;
		srcRect.bottom = m_pSmack->Height;

		destRect.left = destRect.top = 0;
		destRect.right = g_RenderStruct.m_Width;
		destRect.bottom = g_RenderStruct.m_Height;

		pBackBuffer->Blt(&destRect, pDisplay, &srcRect, 0, LTNULL);
	}
	else
	{
		srcRect.left = srcRect.top = 0;
		srcRect.right = m_pSmack->Width;
		srcRect.bottom = m_pSmack->Height;

		destRect.left = (g_RenderStruct.m_Width - m_pSmack->Width) >> 1;
		destRect.top = (g_RenderStruct.m_Height - m_pSmack->Height) >> 1;

		if (destRect.left < 0)
		{
			srcRect.left -= destRect.left;
			srcRect.right -= destRect.left;
			destRect.left = 0;
		}

		if (destRect.top < 0)
		{
			srcRect.top -= destRect.top;
			srcRect.bottom -= destRect.top;
			destRect.top = 0;
		}

		destRect.right = destRect.left + (srcRect.right - srcRect.left);
		if (destRect.right > g_RenderStruct.m_Width)
		{
			srcRect.right += g_RenderStruct.m_Width - destRect.right;
			destRect.right = g_RenderStruct.m_Width;
		}

		destRect.bottom = destRect.top + (srcRect.bottom - srcRect.top);
		if (destRect.bottom > g_RenderStruct.m_Height)
		{
			srcRect.bottom += g_RenderStruct.m_Height - destRect.bottom;
			destRect.bottom = g_RenderStruct.m_Height;
		}

		pBackBuffer->BltFast(destRect.left, destRect.top, pDisplay, &srcRect, 0);
	}

	if (!nWait)
	{
		if (!IsDone())
			m_pMgr->m_pSmackNextFrame(m_pSmack);
	}

	return LT_OK;
}
