// *********************************************************************** //
//
//  MODULE:   VIDEOMGR (.CPP)   (file name ours) -- lithtech.exe only
//
//  The video-manager factory (Primal Hunt 0x004a76e0) and the Smack
//  manager's members it reaches (0x004a78a0 ..), split out of fntdraw.cpp,
//  whose header recorded this as a second translation unit (its PART TWO;
//  the visibility walk that followed it is visquery.cpp).  In Primal Hunt
//  the factory's run ends in <string>'s guard pair 0x004a7870/0x004a7890.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x4a7870).
#include <string>
#endif
#include <windows.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "ltrect.h"



// ======================================================================= //
//
//  PART TWO -- 0x0048a770 .. 0x0048c19e
//
//  A DIFFERENT TRANSLATION UNIT.  Split this off before landing it.
//
//  WHAT THE SIX REMAINING ENUMERATED ROWS ACTUALLY ARE.  The region was
//  described as "DirectDraw surfaces and video playback", with
//  four literals.  Three of the four are real but belong to rows that are
//  NOT in this enumeration -- "BACKBUFFER" (0x004a73b4),
//  "GetHook(BACKBUFFER) failed" (0x004a7440) and
//  "IDirectDraw::CreateSurface failed" (0x004a736c) are pushed only at
//  0x00401e2c/0x00401f83/0x004022eb (binkvideo.cpp's territory) and at
//  0x0048aeb5/0x0048af57/0x0048b44c/0x0048b47e, which are
//  SmackVideoInst::InitScreen (0x0048ada0) and ::UpdateOnScreen
//  (0x0048b440); "%d videos, %d active" (0x004af4e8) is pushed at
//  0x0048a6f8, inside SmackVideoMgr::UpdateVideo (0x0048a6b0).  None of
//  those five addresses is a row here.  The six rows split three ways:
//
//    0x0048a770  the video-manager FACTORY -- picks Bink or Smack by name
//    0x0048aa50  SmackVideoMgr::StartVideo -- the Smack twin of
//                binkvideo.cpp's BinkVideoMgr::StartVideo (0x00401a50)
//    0x0048b8e0  \
//    0x0048bca0   |  a VISIBILITY TRAVERSAL that has nothing to do with
//    0x0048bd70   |  video at all (see the note at the bottom)
//    0x0048be40  /
//
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  The error report, exactly binkvideo.cpp's K_RETURN_ERROR.  The
//  stringifier double-expands, which is why 0x004a730c holds "67" and not
//  "LT_OUTOFMEMORY", and 0x004a7164 is the lone space.
// ----------------------------------------------------------------------- //

extern "C" void dsi_OnReturnErrorQ488(int code);           // 0x0042d760
extern int      g_DebugLevelQ488;                          // 0x004b0ab0
extern char*    g_pErrorFormatQ488;                        // 0x004a7110
void            dsi_ServerErrorQ488(char* pFormat, ...);   // 0x0042e6f0

#define Q_ERRSTR_(x) #x
#define Q_ERRSTR(x)  Q_ERRSTR_(x)
#define Q_ERRDESC    " "

#define Q_RETURN_ERROR(level, fnName, code) \
	{ \
		dsi_OnReturnErrorQ488(code); \
		if (g_DebugLevelQ488 >= level) \
			dsi_ServerErrorQ488(g_pErrorFormatQ488, fnName, \
			                    Q_ERRSTR(code), Q_ERRDESC); \
		return code; \
	}

// 0x00492f30 -- the CRT's case-insensitive compare.  lt2new3.cpp,
// clientinput.cpp and dmextra.cpp all name this address as stricmp: it is
// <string.h>'s _stricmp, called by its CRT name below.

// binkvideo.cpp's LTLink/LTList, renamed.
struct LTLinkQ488
{
	LTLinkQ488* m_pPrev;                     // 0x00
	LTLinkQ488* m_pNext;                     // 0x04
	void*       m_pData;                     // 0x08
};

struct LTListQ488
{
	// binkvideo.cpp's LTListK::Init, head first then count (0x0048a8ce,
	// 0x0048a8d5).
	void Init() { m_pHead = LTNULL; m_nElements = 0; }

	uint32      m_nElements;                 // 0x00
	LTLinkQ488* m_pHead;                     // 0x04
};

// The same inline helper binkvideo.cpp proves at 0x00401b0b; 0x0048ab11 is
// the identical expansion, reload of `pAfter->m_pNext` included.
inline void q_AddToListQ488(LTListQ488* pList, LTLinkQ488* pLink, void* pObj)
{
	if (pList->m_pHead)
	{
		LTLinkQ488* pAfter = pList->m_pHead->m_pPrev;

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


// ----------------------------------------------------------------------- //
//  The two video managers and the Smack instance.
//
//  Sizes come from the `push $N; call 0x004290b0` at each allocation site:
//  0x54 for the Bink manager (binkvideo.cpp's own figure), 0x34 for the
//  Smack manager (its constructor 0x0048a870 writes through +0x30 and
//  stops) and 0xa4 for the Smack instance.
//
//  The five virtuals are iltvideomgr.h's, in the order lvideomgr.cpp pins
//  them; only the fact that there ARE virtuals matters here, because
//  0x0048a770 reaches none of them.
// ----------------------------------------------------------------------- //

// NOT pure: the two derived classes are allocated with `new` right here, so
// an abstract base would not compile.  The five bodies live in binkvideo.cpp
// and in the unwritten Smack file, and this object leaves them as undefined
// externals -- which is all the object-level verifier ever sees.
class IVideoMgrQ488
{
public:
	virtual LTRESULT StartOnScreenVideo(char* pFilename, uint32 nFlags,
	                                    void*& hVideo);
	virtual LTRESULT StartTextureVideo(char* pFilename, uint32 nFlags,
	                                   void*& hVideo);
	virtual LTRESULT UpdateVideo(void* hVideo);
	virtual LTRESULT GetVideoStatus(void* hVideo);
	virtual LTRESULT StopVideo(void* hVideo);
};

// binkvideo.cpp's BinkVideoMgrK, cut to what 0x0048a770 needs.  The
// destructor is NOT virtual: 0x0048a7c2 and 0x0048a819 both call
// 0x00401820 directly and then operator delete, which lt2new4.cpp already
// reads as `delete p` on a class whose destructor is non-virtual.
class BinkVideoMgrQ488 : public IVideoMgrQ488
{
public:
	BinkVideoMgrQ488(void* pClientMgr);                     // 0x004017e0
	~BinkVideoMgrQ488();                                    // 0x00401820

	LTRESULT Init();                                        // 0x00401850

public:
	uint8 m_pad_04[0x54 - 0x04];
};

class SmackVideoInstQ488;

// binkvideo.cpp's LTListInitK: the list as a member with an inline
// constructor, count then head -- the two stores 0x0048a870 makes ahead of
// its vptr store.
struct LTListInitQ488 : public LTListQ488
{
	LTListInitQ488() { m_nElements = 0; m_pHead = LTNULL; }
};

class SmackVideoMgrQ488 : public IVideoMgrQ488
{
public:
	SmackVideoMgrQ488(void* pClientMgr);                    // 0x0048a870
	~SmackVideoMgrQ488();                                   // 0x0048a8b0

	LTRESULT Init();                                        // 0x0048a8e0
	LTRESULT StartVideo(char* pFilename, uint32 nFlags,
	                    void*& hVideo, uint32 bTexture);    // 0x0048aa50

public:
	LTListInitQ488 m_Videos;                                // 0x04 / 0x08 (0x0048a870)
	void*      m_pClientMgr;                                // 0x0c
	uint32     m_Unknown10;                                 // 0x10
	// The seven smackw32.dll entry points Init (0x0048a8e0) looks up, in
	// lookup order.  All __stdcall (the "@N" decoration); only the sound
	// one is ever called here, so the rest stay untyped.
	void       (__stdcall *m_pSmackSoundUseMSS)(void* hDig); // 0x14
	void*      (__stdcall *m_pSmackOpen)(char* pName, uint32 nFlags,
	                                     uint32 nExtraBuf);  // 0x18
	void       (__stdcall *m_pSmackClose)(void* pSmack);     // 0x1c
	void*      m_pSmackWait;                                // 0x20
	void*      m_pSmackToBuffer;                            // 0x24
	void*      m_pSmackDoFrame;                             // 0x28
	void*      m_pSmackNextFrame;                           // 0x2c
	HINSTANCE  m_hSmackDLL;                                 // 0x30, smackw32.dll
};

class SmackVideoInstQ488
{
public:
	SmackVideoInstQ488(SmackVideoMgrQ488* pMgr);            // 0x0048aba0

	virtual ~SmackVideoInstQ488();                          // slot 0
	virtual void Release();                                 // slot 1, 0x0048b390
	virtual void Restore();                                 // slot 2, 0x0048b120
	virtual void FreeSurfaces();                            // slot 3, 0x0048b160

	void     Term();                                        // 0x0048ac70
	LTRESULT Init(char* pFilename, uint32 nFlags, uint32 nMode); // 0x0048ad10
	LTRESULT InitScreen();                                  // 0x0048ada0
	LTRESULT InitTexture();                                 // 0x0048b000

public:
	LTLinkQ488         m_Link;                              // 0x04
	LTListQ488         m_Bindings;                          // 0x10 count, 0x14 head
	void*              m_pSmack;                            // 0x18
	SmackVideoMgrQ488* m_pMgr;                              // 0x1c
	uint8              m_pad_20[0x64 - 0x20];
	void*              m_pTextureData;                      // 0x64
	uint8              m_pad_68[0x9c - 0x68];               // 0x68 SharedTexture
	// AVP2: as Primal Hunt (PH addresses) -- the SharedTexture is 0x40 bytes, not 0x34 (cmgrinit.cpp), so the
	// two flags words are stored at +0xac/+0xa8 (0x004a7e18).
	uint8              m_padExtra9c[0xc];
	uint32             m_nFlags;                            // 0x9c
	uint32             m_nMode;                             // 0xa0
};


// ----------------------------------------------------------------------- //
//  0x0048a770, 248 bytes -- make a video manager for a named back end.
//
//  Its one caller, 0x0040fc18, pushes the client manager and then the
//  literal "BINK" (0x004a83a8), so the arguments are (pClientMgr, pName)
//  and the answer is stored into pClientMgr->+0x1610 -- which lvideomgr.cpp
//  already names as m_pClientMgr->m_pVideoMgr.
//
//  THE SURPRISE, and it is in the bytes twice over: the "SMACKER" arm tries
//  a BINK manager FIRST (the same 0x54-byte allocation, the same
//  constructor 0x004017e0, the same Init 0x00401850 and the same
//  destructor 0x00401820 as the "BINK" arm) and only falls through to the
//  0x34-byte Smack manager when that fails.  Bink's runtime plays Smacker
//  files, so this is deliberate, not a copy-paste.
//
//  THE TWO NULL TESTS after each allocation are `new`'s own guard merged
//  with the source's `if (p)`: `new` with a
//  constructor splits the allocation from the constructor.  The Init test
//  is `jne` to the teardown, so LT_OK (zero) is the success value and the
//  success return falls through.
//
//  THREE separate `xor %eax,%eax; ret` epilogues -- 0x0048a7d0 for the BINK
//  arm's post-delete return and 0x0048a863 shared by everything else --
//  which is why the BINK arm's failure `return LTNULL` is written out and
//  the SMACKER arm's two are falls-through.
// ----------------------------------------------------------------------- //

IVideoMgrQ488* CreateVideoMgrQ488(void* pClientMgr, char* pName)
{
	BinkVideoMgrQ488*  pBink;
	SmackVideoMgrQ488* pSmack;

	if (_stricmp(pName, "BINK") == 0)
	{
		pBink = new BinkVideoMgrQ488(pClientMgr);

		if (pBink)
		{
			if (!pBink->Init())
				return pBink;

			delete pBink;

			return LTNULL;
		}
	}
	else if (_stricmp(pName, "SMACKER") == 0)
	{
		pBink = new BinkVideoMgrQ488(pClientMgr);

		if (pBink)
		{
			if (!pBink->Init())
				return pBink;

			delete pBink;
		}

		pSmack = new SmackVideoMgrQ488(pClientMgr);

		if (pSmack)
		{
			if (!pSmack->Init())
				return pSmack;

			delete pSmack;
		}
	}

	return LTNULL;
}


// ----------------------------------------------------------------------- //
//  0x0048a870, 50 bytes -- SmackVideoMgr::SmackVideoMgr.
//
//  HOME: smackvideo.cpp, like the destructor below.  binkvideo.cpp's
//  0x004017e0 with the Smack layout: the list (+0x04/+0x08) and the client
//  manager (+0x0c) BEFORE the vptr 0x004a3890 (the initialiser list), then
//  the seven smackw32 entry points +0x14..+0x2c and the handle +0x30
//  cleared in the body.  +0x10 is left alone, as in Bink.
// ----------------------------------------------------------------------- //

SmackVideoMgrQ488::SmackVideoMgrQ488(void* pClientMgr)
	: m_pClientMgr(pClientMgr)
{
	m_pSmackSoundUseMSS = LTNULL;
	m_pSmackOpen = LTNULL;
	m_pSmackClose = LTNULL;
	m_pSmackWait = LTNULL;
	m_pSmackToBuffer = LTNULL;
	m_pSmackDoFrame = LTNULL;
	m_pSmackNextFrame = LTNULL;
	m_hSmackDLL = LTNULL;
}


// ----------------------------------------------------------------------- //
//  0x0048a8b0, 46 bytes -- SmackVideoMgr::~SmackVideoMgr.
//
//  HOME: smackvideo.cpp (it sits between the constructor 0x0048a870 and
//  Init 0x0048a8e0); written here because this file declares the class
//  and its one caller is the factory above (0x0048a855).  Byte for byte
//  binkvideo.cpp's ~BinkVideoMgr (0x00401820) with the handle at +0x30:
//  own vptr only (non-virtual, no base destructor), the handle cleared
//  inside the `if`, then the list reset as the inline Init().
// ----------------------------------------------------------------------- //

SmackVideoMgrQ488::~SmackVideoMgrQ488()
{
	if (m_hSmackDLL)
	{
		FreeLibrary(m_hSmackDLL);
		m_hSmackDLL = LTNULL;
	}

	m_Videos.Init();
}


// ----------------------------------------------------------------------- //
//  0x0048a8e0 .. 0x0048aa4e, 367 bytes -- SmackVideoMgr::Init.
//
//  The csv carried it as 0x0048a8e0 (87) + a "gap" 0x0048a937 (280); the
//  second row is the `push %edi` opening the success arm of the `jne` at
//  0x0048a8f3 -- the same enumeration defect binkvideo.cpp documents for
//  BinkVideoMgr::Init (0x00401850 / 0x004018a7), and this body is that
//  one's twin: LoadLibrary("smackw32.dll") stored to +0x30, seven
//  GetProcAddress lookups through the CSE'd import in %edi, the validity
//  tests in lookup order all `je` one block after both success returns,
//  ci_GetSoundMgr() called twice around the sound-system test.  Unlike
//  Bink, BOTH arms call the one pointer at +0x14 (_SmackSoundUseMSS@4),
//  with the driver or with 0 -- and the source makes ONE call on a local
//  set by an if/else: VC6 tail-duplicates the call and the return into
//  each arm, the else arm as `xor %eax,%eax; push %eax` (0x0048a9ee).
//  Two written-out calls give `push $0` instead.
// ----------------------------------------------------------------------- //

class ILTClientSoundMgrQ488
{
public:
	uint8 m_pad_00[0x124];
	void* m_pSoundSys;                       // 0x124
	uint8 m_pad_128[0x130 - 0x128];
	void* m_hDigDriver;                      // 0x130  (name ours)
};

ILTClientSoundMgrQ488* ci_GetSoundMgrQ488();               // 0x0040f7f0

LTRESULT SmackVideoMgrQ488::Init()
{
	m_hSmackDLL = LoadLibrary("smackw32.dll");
	if (!m_hSmackDLL)
	{
		dsi_ServerErrorQ488("Unable to find smackw32.dll, video playback disabled.");
		Q_RETURN_ERROR(1, "SmackVideoMgr::Init", LT_MISSINGFILE);
	}

	m_pSmackSoundUseMSS = (void (__stdcall *)(void*))GetProcAddress(m_hSmackDLL, "_SmackSoundUseMSS@4");
	m_pSmackOpen        = (void* (__stdcall *)(char*, uint32, uint32))GetProcAddress(m_hSmackDLL, "_SmackOpen@12");
	m_pSmackClose       = (void (__stdcall *)(void*))GetProcAddress(m_hSmackDLL, "_SmackClose@4");
	m_pSmackWait        = (void*)GetProcAddress(m_hSmackDLL, "_SmackWait@4");
	m_pSmackToBuffer    = (void*)GetProcAddress(m_hSmackDLL, "_SmackToBuffer@28");
	m_pSmackDoFrame     = (void*)GetProcAddress(m_hSmackDLL, "_SmackDoFrame@4");
	m_pSmackNextFrame   = (void*)GetProcAddress(m_hSmackDLL, "_SmackNextFrame@4");

	if (m_pSmackSoundUseMSS && m_pSmackOpen && m_pSmackClose && m_pSmackWait &&
	    m_pSmackToBuffer && m_pSmackDoFrame && m_pSmackNextFrame)
	{
		void* hDig;

		if (ci_GetSoundMgrQ488()->m_pSoundSys)
			hDig = ci_GetSoundMgrQ488()->m_hDigDriver;
		else
			hDig = LTNULL;

		m_pSmackSoundUseMSS(hDig);

		return LT_OK;
	}

	dsi_ServerErrorQ488("Invalid smackw32.dll, video playback disabled.");

	FreeLibrary(m_hSmackDLL);
	m_hSmackDLL = LTNULL;

	Q_RETURN_ERROR(1, "SmackVideoMgr::Init", LT_NOTINITIALIZED);
}


// ----------------------------------------------------------------------- //
//  0x0048ac70, 152 bytes -- SmackVideoInst::Term.
//
//  HOME: smackvideo.cpp, like the rest of the instance; written here
//  because this file declares the class.  The Bink twin is binkvideo.cpp's
//  BinkVideoInst::Term (0x00401c70) and the two list walks are its bytes
//  exactly (branchless-select advance, `delete pBind` through 0x0048a620).
//  The Smack order differs: FreeSurfaces (slot 3) comes FIRST, then a
//  single _SmackClose@4 (manager +0x1c) with no wait loop, then the
//  texture data at +0x64.  lt2new3.cpp's CClass4A38A4 and cmgsurf.cpp's
//  SmackVideoInstQ428 name the same function; neither defines it.
// ----------------------------------------------------------------------- //

void tex_FreeTextureDataQ488(void* pData);                 // 0x0042ee30

struct SharedTextureQ488;

struct CVideoPolyQ488
{
	// AVP2: as Primal Hunt (PH addresses) -- the poly's texture pointer is at +0x2c (0x004a7d54).
	uint8               m_pad_00[0x2c];
	SharedTextureQ488*  m_pTexture;          // 0x2c
};

// binkvideo.cpp's CVideoBindingK, cut to the two fields read here.
class CVideoBindingQ488
{
public:
	~CVideoBindingQ488();                    // 0x0048a620

public:
	CVideoPolyQ488*     m_pPoly;             // 0x00
	uint8               m_pad_04[0x10 - 0x04];
	SharedTextureQ488*  m_pOldTexture;       // 0x10
	uint8               m_pad_14[0x2c - 0x14];
};

void SmackVideoInstQ488::Term()
{
	LTLinkQ488*        pLink;
	CVideoBindingQ488* pBind;

	FreeSurfaces();

	if (m_pSmack)
	{
		m_pMgr->m_pSmackClose(m_pSmack);
		m_pSmack = LTNULL;
	}

	if (m_pTextureData)
	{
		tex_FreeTextureDataQ488(m_pTextureData);
		m_pTextureData = LTNULL;
	}

	pLink = m_Bindings.m_pHead;
	while (pLink)
	{
		pBind = (CVideoBindingQ488*)pLink->m_pData;
		pLink = (pLink->m_pNext == m_Bindings.m_pHead) ? LTNULL : pLink->m_pNext;

		if (pBind->m_pPoly)
			pBind->m_pPoly->m_pTexture = pBind->m_pOldTexture;
	}

	pLink = m_Bindings.m_pHead;
	while (pLink)
	{
		pBind = (CVideoBindingQ488*)pLink->m_pData;
		pLink = (pLink->m_pNext == m_Bindings.m_pHead) ? LTNULL : pLink->m_pNext;

		delete pBind;
	}

	m_Bindings.Init();
}


// ----------------------------------------------------------------------- //
//  0x0048ad10, 140 bytes -- SmackVideoInst::Init.
//
//  Twin of binkvideo.cpp's BinkVideoInst::Init (0x00401d30): `ret $0xc`,
//  (pFilename, nFlags, nMode), the RETURN_ERROR_INFO report with the code
//  spelled symbolically (0x004a7324 "LT_MISSINGFILE") and "SmackOpen
//  failed", and the two trailing stores +0xa0 then +0x9c.  The open flags
//  are `neg; sbb; and $0xfff02000; add $0xfe000` -- a ternary over two
//  constants, 0xfe000 (RAD's SMACKTRACKS) for an on-screen video and 0 for
//  a texture one -- then 0x100000 or'd in for flag 2.  _SmackOpen@12's
//  third argument is -1.
// ----------------------------------------------------------------------- //

#define Q_RETURN_ERROR_INFO(level, fnName, code, info) \
	{ \
		dsi_OnReturnErrorQ488(code); \
		if (g_DebugLevelQ488 >= level) \
			dsi_ServerErrorQ488(g_pErrorFormatQ488, fnName, #code, info); \
		return code; \
	}

LTRESULT SmackVideoInstQ488::Init(char* pFilename, uint32 nFlags, uint32 nMode)
{
	uint32 nOpenFlags = nMode ? 0 : 0xfe000;

	if (nFlags & 2)
		nOpenFlags |= 0x100000;

	m_pSmack = m_pMgr->m_pSmackOpen(pFilename, nOpenFlags, 0xFFFFFFFF);
	if (!m_pSmack)
		Q_RETURN_ERROR_INFO(1, "SmackVideoInst::Init", LT_MISSINGFILE,
		                    "SmackOpen failed");

	m_nMode = nMode;
	m_nFlags = nFlags;

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0048aa50, 259 bytes -- SmackVideoMgr::StartVideo.
//
//  Line for line the twin of binkvideo.cpp's BinkVideoMgr::StartVideo
//  (0x00401a50), and everything that file pins applies here unchanged:
//  `ret $0x10` with `this` in %ecx, `hVideo = LTNULL` as the first
//  statement, the LT_OUTOFMEMORY report on the allocation failure, slot 1
//  (Release) rather than slot 0 for both cleanups, `test %ebp,%ebp; jne` so
//  InitScreen is the FALL-THROUGH and the source tests `!bTexture`, the
//  join assigning both arms into one local, and the list insert reloading
//  `pAfter->m_pNext` at 0x0048ab22 after storing through it at 0x0048ab20.
//
//  The only differences from the Bink body are the instance size (0xa4
//  against 0x68), the constructor (0x0048aba0), the three entry points
//  (0x0048ad10 / 0x0048ada0 / 0x0048b000) and the error tag, which
//  0x004af610 spells "SmackVideoMgr".
// ----------------------------------------------------------------------- //

LTRESULT SmackVideoMgrQ488::StartVideo(char* pFilename, uint32 nFlags,
                                       void*& hVideo, uint32 bTexture)
{
	SmackVideoInstQ488* pInst;
	LTRESULT            dResult;

	hVideo = LTNULL;

	pInst = new SmackVideoInstQ488(this);
	if (!pInst)
		Q_RETURN_ERROR(1, "SmackVideoMgr", LT_OUTOFMEMORY);

	dResult = pInst->Init(pFilename, nFlags, bTexture);
	if (dResult != LT_OK)
	{
		pInst->Release();
		return dResult;
	}

	if (!bTexture)
		dResult = pInst->InitScreen();
	else
		dResult = pInst->InitTexture();

	if (dResult != LT_OK)
	{
		pInst->Release();
		return dResult;
	}

	q_AddToListQ488(&m_Videos, &pInst->m_Link, pInst);

	hVideo = pInst;

	return LT_OK;
}
