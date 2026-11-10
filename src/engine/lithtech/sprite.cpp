// *********************************************************************** //
//
//  MODULE:   SPRITE (.CPP)   (file name ours)
//
//  PURPOSE:  the client's sprite module: the reader cm_LoadSprite calls,
//            the destructor and the tracker's init and update
//            (lithtech.exe 0x004a05a0 .. 0x004a08b1; AvP2 1.0.9.6 from
//            0x00483a60).
//
//  Its own translation unit in retail: in Primal Hunt the run ends in
//  <string>'s guarded initializer pair (0x004a08c0, 0x004a08e0), as each
//  Primal Hunt unit that saw <string> does.  The code was written in
//  cmodelload.cpp (the reader) and csvis.cpp (the other three) until this
//  split; the types are theirs, re-declared so this TU stands alone and the
//  symbols keep the names their callers use.
//
//  Build /O2 /Gy (build/build.mk's lithtech line).
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x4a08c0).
#include <string>
#endif
#include <string.h>

#include "ltbasedefs.h"
#include "ltlink.h"


// ---- cmodelload.cpp's views, for the reader ----------------------------- //

// clientmgr.h's CClientFileRef: three fields, 0xc bytes, zeroed by its
// inline constructor.
class CFileIdentQ474
{
public:
	CFileIdentQ474() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                       // 0x00
	char*  m_pFilename;                       // 0x04
	uint16 m_nLocalID;                        // 0x08
};

class CClientMgrQ474;

class ILTStreamQ474
{
public:
	virtual void _v00() = 0;
	virtual void Release() = 0;                                // slot 0x04
	// ltbasedefs.h's ILTStream order: Read at 0x08 and ErrorStatus at 0x10.
	virtual LTRESULT Read(void* pData, uint32 size) = 0;       // slot 0x08
	virtual LTRESULT ReadString(char* pStr, uint32 max) = 0;   // slot 0x0c
	virtual LTRESULT ErrorStatus() = 0;                        // slot 0x10
};

// The loader's layout (0x00483a88..0x00483aaa, 0x00483b4d..0x00483b96).
class CSpriteAnimQ474
{
public:
	char   m_sName[32];                                        // 0x00
	void** m_Frames;                                           // 0x20
	uint32 m_nFrames;                                          // 0x24
	uint32 m_MsAnimLength;                                     // 0x28
	uint32 m_MsFrameRate;                                      // 0x2c
	uint32 m_bKeyed;                                           // 0x30
	uint32 m_ColourKey;                                        // 0x34
	uint32 m_bTranslucent;                                     // 0x38
};

class CSpriteQ474
{
public:
	LTLink           m_Link;                                   // 0x00  m_pData
	                                                           //       points at
	                                                           //       the sprite
	CSpriteAnimQ474* m_Anims;                                  // 0x0c
	uint32           m_nAnims;                                 // 0x10
};

// 0x0041ff90 -- lttexmod.cpp: resolve one CClientFileRef to a SharedTexture.
void* cm_FindSharedTextureQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent);


// ---- csvis.cpp's views, for the tracker --------------------------------- //

// A sprite frame: the texture it shows first.
class CSpriteFrameQ414
{
public:
	uint32 m_pTexture;                                         // 0x00
};

// One animation, at the offsets 0x00483d00 reads.
class CSpriteAnimQ414
{
public:
	uint8             m_pad_00[0x20];
	CSpriteFrameQ414* m_Frames;                                // 0x20
	uint32            m_nFrames;                               // 0x24
	uint32            m_MsAnimLength;                          // 0x28
	uint32            m_MsFrameRate;                           // 0x2c
	uint8             m_pad_30[0x3c - 0x30];   // 0x3c records: 0x00483c91
};

// The sprite tracker, 20 bytes.  (names ours)
#define SC_PLAYQ414  1
#define SC_LOOPQ414  2

class CSpriteTrackerQ414
{
public:
	void*             m_pSprite;                               // 0x00
	CSpriteAnimQ414*  m_pCurAnim;                              // 0x04
	CSpriteFrameQ414* m_pCurFrame;                             // 0x08
	uint32            m_MsCurTime;                             // 0x0c
	uint32            m_Flags;                                 // 0x10
};


// ======================================================================= //
//  0x00483a60, 507 bytes -- spr_Load: read a sprite out of an open stream
//  (name ours).
//
//  Called by cmodelload.cpp's cm_LoadSprite (0x00476f42); client-only (the
//  "Untitled" literal 0x004aea78 has no copy in server.dll).
//
//  One 0x14-byte sprite, its link pointing at itself, ONE 0x3c-byte
//  animation (the alloc pushes the constant, the memset re-reads m_nAnims
//  after the call), then per animation five dwords and per frame a
//  uint16-length texture name (refused over 1000, 0x00483bb3) resolved
//  through 0x0041ff90.  The stream's error status decides the result;
//  failure frees through 0x00483c60 (clientmgr2.cpp's sub_483c60).
// ======================================================================= //
void* dalloc(uint32 size);                                     // 0x00428fd0
void  spr_DestroyQ474(CSpriteQ474* pSprite);                   // 0x00483c60

CSpriteQ474* spr_LoadQ474(CClientMgrQ474* pMgr, ILTStreamQ474* pStream)
{
	CFileIdentQ474   ref;
	CSpriteQ474*     pSprite;
	CSpriteAnimQ474* pAnim;
	uint32           i, j;
	uint32           nFrames, msFrameRate, bKeyed, bTranslucent, colourKey;
	uint16           nameLen;
	char             szName[1024];

	pSprite = (CSpriteQ474*)dalloc(sizeof(CSpriteQ474));
	memset(pSprite, 0, sizeof(CSpriteQ474));
	pSprite->m_Link.m_pData = pSprite;
	pSprite->m_nAnims = 1;
	pSprite->m_Anims = (CSpriteAnimQ474*)dalloc(sizeof(CSpriteAnimQ474) * pSprite->m_nAnims);
	memset(pSprite->m_Anims, 0, sizeof(CSpriteAnimQ474) * pSprite->m_nAnims);

	for (i = 0; i < pSprite->m_nAnims; i++)
	{
		pAnim = &pSprite->m_Anims[i];

		pStream->Read(&nFrames, 4);
		pStream->Read(&msFrameRate, 4);
		pStream->Read(&bKeyed, 4);
		pStream->Read(&bTranslucent, 4);
		pStream->Read(&colourKey, 4);

		pAnim->m_Frames = (void**)dalloc(sizeof(void*) * nFrames);
		strncpy(pAnim->m_sName, "Untitled", 31);   // 0x00492b60 = CRT strncpy; no terminator store follows (the memset covers it)
		pAnim->m_nFrames      = nFrames;
		pAnim->m_MsFrameRate  = msFrameRate;
		pAnim->m_MsAnimLength = (1000 / msFrameRate) * nFrames;
		pAnim->m_bKeyed       = (uint8)bKeyed;
		pAnim->m_bTranslucent = (uint8)bTranslucent;
		pAnim->m_ColourKey    = colourKey;

		for (j = 0; j < nFrames; j++)
		{
			pStream->Read(&nameLen, 2);
			if (nameLen > 1000)
				goto Error;

			pStream->Read(szName, nameLen);
			szName[nameLen] = 0;

			ref.m_nFileType = 1;
			ref.m_pFilename = szName;
			pAnim->m_Frames[j] = cm_FindSharedTextureQ474(pMgr, &ref);
		}
	}

	if (pStream->ErrorStatus() != LT_OK)
		goto Error;

	return pSprite;

Error:
	spr_DestroyQ474(pSprite);
	return LTNULL;
}



// ======================================================================= //
//  spr_Destroy -- 0x00483c60, spr_InitTracker -- 0x00483cc0
//
//  The two sprite-module functions just ahead of spr_UpdateTracker in the
//  image.  clientmgr2.cpp calls the first as sub_483c60 and
//  clientmgr.h / cmodelload.cpp / partfm.cpp declare the second as
//  sub_483cc0.  The sprite header is 0x14 bytes (0x00483a60 allocates it):
//  a list link, then the animation array (+0x0c, 0x3c-byte records) and its
//  count (+0x10).  Every free goes to dfree (0x00429080).  (names ours,
//  after the SDK's spr_ prefix)
// ======================================================================= //

void dfree(void* ptr);                                         // 0x00429080

class CSpriteQ414
{
public:
	LTLink           m_Link;                                   // 0x00
	CSpriteAnimQ414* m_Anims;                                  // 0x0c
	uint32           m_nAnims;                                 // 0x10
};

// 0x00483c60 -- each animation's frame array (+0x20, `mov 0x20(%edi,%eax)`
// with a 0x3c stride), then the animation array, then the sprite.
void spr_DestroyQ414(CSpriteQ414* pSprite)
{
	uint32 i;

	if (pSprite->m_Anims)
	{
		for (i = 0; i < pSprite->m_nAnims; i++)
		{
			if (pSprite->m_Anims[i].m_Frames)
				dfree(pSprite->m_Anims[i].m_Frames);
		}

		dfree(pSprite->m_Anims);
	}

	dfree(pSprite);
}

// 0x00483cc0 -- point the tracker at the sprite's first animation and its
// first frame (none when the animation is empty), time zero, playing and
// looping (the `movl $0x3,0x10(%eax)` on both arms).
void spr_InitTrackerQ414(CSpriteTrackerQ414* pTracker, CSpriteQ414* pSprite)
{
	pTracker->m_pSprite = pSprite;
	pTracker->m_pCurAnim = &pSprite->m_Anims[0];

	if (pTracker->m_pCurAnim->m_nFrames > 0)
		pTracker->m_pCurFrame = &pTracker->m_pCurAnim->m_Frames[0];
	else
		pTracker->m_pCurFrame = LTNULL;

	pTracker->m_MsCurTime = 0;
	pTracker->m_Flags = SC_PLAYQ414 | SC_LOOPQ414;
}


// ======================================================================= //
//  spr_UpdateTracker -- 0x00483d00
//
//  csvis.cpp's per-frame walks are its callers.  __cdecl, two arguments.  Advance the time by the delta
//  when playing (bit 0), clamp to the last millisecond when past the end and
//  NOT looping (bit 1) -- any other case, in range or looping, takes the
//  modulo (0x00483d3d) -- then pick the frame as
//  (time / (1000 / frame rate)) % frame count.  A zero-length animation
//  clears the frame.  (name ours)
// ======================================================================= //

void spr_UpdateTrackerQ414(CSpriteTrackerQ414* pTracker, uint32 msDelta)
{
	CSpriteAnimQ414* pAnim;
	uint32           iFrame;

	pAnim = pTracker->m_pCurAnim;
	if (!pAnim)
		return;

	if (pAnim->m_MsAnimLength)
	{
		if (pTracker->m_Flags & SC_PLAYQ414)
			pTracker->m_MsCurTime += msDelta;

		if (pTracker->m_MsCurTime >= pAnim->m_MsAnimLength &&
		    !(pTracker->m_Flags & SC_LOOPQ414))
			pTracker->m_MsCurTime = pAnim->m_MsAnimLength - 1;
		else
			pTracker->m_MsCurTime %= pAnim->m_MsAnimLength;

		iFrame = (pTracker->m_MsCurTime / (1000 / pAnim->m_MsFrameRate)) % pAnim->m_nFrames;
		pTracker->m_pCurFrame = &pAnim->m_Frames[iFrame];
	}
	else
	{
		pTracker->m_pCurFrame = LTNULL;
	}
}
