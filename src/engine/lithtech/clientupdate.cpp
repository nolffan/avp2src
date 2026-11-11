// *********************************************************************** //
//
//  MODULE:   CLIENTUPDATE (.CPP)   (file name ours) -- lithtech.exe only
//
//  The client's per-frame object updates: CClientMgr's RunFrameUpdates and
//  the walks it calls (AvP2 1.0.9.6 0x00415b50 .. 0x00415f9e; Primal Hunt
//  0x00419f40 .. 0x0041a49e).  Their own translation unit: in Primal Hunt
//  the run follows the CCmgClientMgr/CPacket unit's <string> guard pair
//  (0x00419f10) and ends in its own (0x0041a4a0, 0x0041a4c0).  Split out of
//  csvis.cpp, whose views of the client manager are repeated here; the
//  animation walk and its key callback (0x0041a150, 0x0041a1f0) are still
//  written in clientmgr2.cpp.
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x41a4a0).
#include <string>
#endif
#include <string.h>
#include <stdarg.h>

#include "ltbasedefs.h"
#include "ltlink.h"
#ifdef AVP2_MODERN
#include "..\..\modern\engine\p1_frametime.h"   // M-P1-23
#endif


// ======================================================================= //
//  WHAT THIS TU BORROWS
//
//  Stand-in declarations only.  Every offset is the displacement the shipped
//  code actually uses and the address it was read from is in the comment.
// ======================================================================= //

// One file the object creator has to resolve.  This is
// src/engine/lithtech/clientmgr.h's CClientFileRef restated -- and the
// restatement is load-bearing, because the three-store body its constructor
// compiles to IS the 14-byte function at 0x004147e0 that the array loop at
// 0x00414511 calls.  Two dwords and a WORD, field for field and width for
// width (0x004147e4 / 0x004147e6 / 0x004147e9).
class CClientFileRefQ414
{
public:
	CClientFileRefQ414() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                                        // 0x00
	char*  m_pFilename;                                        // 0x04
	uint16 m_nLocalID;                                         // 0x08
};

// What the object creator is handed: the object's own file, one reference
// per skin, and the create struct.  clientmgr.h's CClientCreateRequest, and
// the shipped constructor run at 0x004144fe..0x0041451c proves the shape --
// the object's file is inlined (three stores) and exactly
// MAX_MODEL_TEXTURES more are built by a loop over a 12-byte stride (NOLF's
// two put m_pStruct at +0x24 where 0x00414526 stores it; AvP2 has four).
// IDENTICAL on Primal Hunt with the layout below (0x004182b0, 886 bytes).
// AVP2: cltapi.cpp's CClientCreateRequest -- four skins, and the flag after
// the struct that the constructor sets (Primal Hunt 0x00418302). PH: the
// model's extra texture file ahead of the struct, built by its own
// constructor call after the skin loop (0x004182f0), so the struct is at
// +0x48 and the flag at +0x4c.
class CClientCreateRequestQ414
{
public:
	CClientCreateRequestQ414() { m_pStruct = LTNULL; m_bResetAnimations = LTTRUE; }

	CClientFileRefQ414  m_ObjectFile;                          // 0x00
	CClientFileRefQ414  m_SkinFiles[MAX_MODEL_TEXTURES];       // 0x0c
#ifdef PRIMAL_HUNT
	CClientFileRefQ414  m_ExtraFile;                           // 0x3c
#endif
	ObjectCreateStruct* m_pStruct;                             // 0x3c (PH 0x48)
	uint32              m_bResetAnimations;                    // 0x40 (PH 0x4c)
};

// The client's world, as far as this run reaches it.  Offsets read off
// 0x00414550 (+0x04 taken as an address, so an inline char run), 0x00414553
// (+0x48), 0x0041455f (+0x140) and 0x0041475e (+0xe8).  What fixes +0x13c /
// +0x140 as a run of 0xc0-byte vis-container records and not something else
// is the name PARSER at 0x00424f52, which splits a "#$#%d %s" name back
// apart, bounds the index it recovers against this same +0x140, and then
// indexes +0x13c by `(i + i*2) << 6` -- 192 bytes a record.  (names ours)
class CBlockerArrayQ414
{
public:
	uint32 GetSize() const { return m_nElements; }

	void*  m_pVTable;                                          // +0x00
	void*  m_pArray;                                           // +0x04 (0x13c)
	uint32 m_nElements;                                        // +0x08 (0x140)
};

class CClientWorldQ414
{
public:
	uint8    m_pad_00[0x04];
	char     m_Name[0x48 - 0x04];                              // 0x04
	// Bit 3 selects the per-blocker branch and bits 4-5 the whole-world
	// branch; the shipped test is on the low byte of a dword load
	// (0x00414556 / 0x00414685), so the field is at least a dword wide.
	uint32   m_Flags;                                          // 0x48
	uint8    m_pad_4c[0xe8 - 0x4c];
	LTVector m_vPos;                                           // 0xe8
	uint8    m_pad_f4[0x138 - 0xf4];
	CBlockerArrayQ414 m_Blockers;                              // 0x138
};

// One element of the run at CClientMgr+0x18c.  Only the two pointers this
// run follows are modelled.  +0x0c's own +0x04 is handed straight to
// strncpy as a `char*` source (0x00414741 computes the address rather than
// loading through it), so it is an inline name and not a pointer to one --
// the same shape CClientFileRefQ414 would have if its name were inline, and
// deliberately NOT called that here because nothing proves they are the same
// class.  (names ours)
class CClientWorldRecQ414
{
public:
	uint8              m_pad_00[0x04];
	CClientWorldQ414*  m_pWorld;                               // 0x04
	uint8              m_pad_08[0x0c - 0x08];
	class CWorldFileNameQ414* m_pFile;                         // 0x0c
};

class CWorldFileNameQ414
{
public:
	uint8 m_pad_00[0x04];
	char  m_Name[1];                                           // 0x04
};

// The CClientMgr, as far as this run reaches it.  The array pair at
// +0x18c/+0x190 is the one src/engine/lithtech/clientmgr2.cpp models as
// CArray18C; this file agrees with it offset for offset and only adds the
// element type.  (Worth recording against that file: its
// cs_ReleaseArrayItems takes the array as a SECOND argument typed
// CClientShell* / CArray18C*, and DoLoadWorld's call site at 0x004140fa
// pushes `0x78(%esi)` for BOTH -- two separate loads of the same
// CClientMgr*, so both parameters are CClientMgr*.)
class CWorldRecArrayQ414
{
public:
	CClientWorldRecQ414*& operator[](const uint32 index) const { return Get(index); }
	uint32 GetSize() const { return m_nElements; }
	CClientWorldRecQ414*& Get(uint32 index) const { return m_pArray[index]; }

	void*                 m_pVTable;                           // +0x00
	CClientWorldRecQ414** m_pArray;                            // +0x04
	uint32                m_nElements;                         // +0x08
};

class CClientMgrQ414
{
public:
	uint8                 m_pad_000[0x188];
	CWorldRecArrayQ414    m_WorldRecs;                         // 0x188
	// AVP2: (the field offsets below are NOLF's) AvP2's and Primal Hunt's
	// manager is four bytes longer from here to the string module (the
	// object list's head at +0x424, 0x00418890; the string module at
	// +0x61c, 0x004193c2) and 0x110 longer from there to

	// the frame time (+0x1368, 0x0041a317), the sprite chain (+0x1374,
	// 0x00419f64), the connection (+0x161c) and the file manager (+0x1644,
	// 0x004187e9).
	uint8                 m_pad_194[0x420 - 0x194];
	// The client's object list, walked by cs_DirtyAllModelsQ414 below.
	// 0x00414a10 takes the ADDRESS of +0x420 as the walk's end marker and
	// 0x00414a0a reads +0x424 as its first element: that is the list's
	// m_Head LTLink (m_pPrev, m_pNext, m_pData) at +0x420, so ltlink.h's
	// LTList -- m_nElements (4) then m_Head -- starts at +0x41c.  (An
	// earlier revision put the LTList itself at +0x420; the walk then read
	// +0x428 / +0x424, masked away by a mnemonic-only diff.)
	// clientmgr2.cpp's CClientMgrView pads over this address.  (name ours)
	LTList                m_ObjectList;                        // 0x41c
	uint8                 m_pad_42c[0x46c - 0x42c];
	// Two more LTLists in the same run, each walked by one of the per-frame
	// updates below: 0x00415c15 reads +0x474 as the first element and
	// 0x00415c1b takes +0x470 as the end marker (the particle systems);
	// 0x00415e64 / 0x00415e5e do the same with +0x484 / +0x480 (the
	// sprites).  Contiguous with m_MoveList at +0x48c.  (names ours)
	LTList                m_ParticleList;                      // 0x46c
	LTList                m_SpriteList;                        // 0x47c
	// The list cs_ApplyPendingMoves walks: 0x00415ec7 reads +0x494 as the
	// first element and 0x00415ecd takes +0x490 as the end marker, so it is
	// an LTList at +0x48c exactly as m_ObjectList above.  (name ours)
	LTList                m_MoveList;                          // 0x48c
	uint8                 m_pad_49c[0x618 - 0x49c];
	// A string-resource module record -- a POINTER to a record whose first
	// dword is the HINSTANCE (cltnew.cpp's 0x610/0x614 pair, one further
	// on).  The only reader in this file is the message helper's
	// WriteFormattedString (0x00415602).  (name ours)
	void*                 m_pStringModule618;                  // 0x618
	uint8                 m_pad_61c[0x1368 - 0x620];
	// The frame time the per-frame walks scale by 1000.0f (0x00415bf6,
	// 0x00415e47, 0x00415f1c), and 0x00415ca8 pushes it whole as the float
	// argument of the particle update 0x0045ca70.  (name ours)
	float                 m_fFrameTime;                        // 0x1258
	uint8                 m_pad_125c[0x1264 - 0x125c];
	// The sprite-instance chain cs_UpdateSprites walks (0x00415b74), null-
	// terminated through +0x04.  (name ours)
	class CSpriteInstQ414* m_pSpriteInsts;                     // 0x1264
	uint8                 m_pad_1268[0x150c - 0x1268];
	// clientmgr.h's m_pConnection, whose +0x1c is m_fFrameTime.
	class CClientConnectionQ414* m_pConnection;                // 0x150c
	uint8                 m_pad_1510[0x1534 - 0x1510];
	// clientmgr.h's own name and offset; cmodelload.cpp uses the same pair.
	class CFileMgrQ414*   m_pFileMgr;                          // 0x1534

	// The per-frame object updates (clientmgr2.cpp's RunFrameUpdates, the
	// shape AvP2 had before the counter went in).
	void RunFrameUpdates();
};

// clientmgr.h / clientmgr2.cpp's g_pClientMgr.  Read by both the inherit
// check (0x0041495c, 0x004149b6) and the model walk (0x00414a00).
extern CClientMgrQ414* g_pClientMgrQ414;                       // 0x004b17e0

// src/engine/lithtech/clientmgr2.cpp's CClientShell, cut down to the two
// things this file needs.  The manager at +0x78 is that file's
// m_pClientMgr, and GetClientMgr (0x00414b10) is its four-byte accessor.
class CClientShellQ414
{
public:
	uint8            m_pad_00[0x78];
	CClientMgrQ414*  m_pClientMgr;                             // 0x078

	CClientMgrQ414*  GetClientMgr();                           // 0x00414b10

	LTBOOL CreateVisContainerObjects();                        // 0x004144e0
};


// ----------------------------------------------------------------------- //
//  Helpers this run calls but does not define
// ----------------------------------------------------------------------- //

// 0x004221f0 -- build the engine's internal name for one of a world's
// vis-container blockers.  `sprintf(pOut, "%s%d %s", "#$#", nIndex,
// pszWorldName)` behind two guards: it refuses a buffer under 8 bytes and
// refuses a world name longer than nMaxLen-8.  Returns non-zero on success.
// What names it is its inverse at 0x00424f52, which takes a name apart with
// the same "#$#" prefix and a "%d %s" tail.  (name ours)


// ======================================================================= //
//  THE PER-FRAME OBJECT UPDATES -- 0x00415bb0's callees
//
//  0x00415bb0 (__thiscall on the manager, unwritten; clientmgr3.cpp calls
//  it as Sub415bb0 between the client shell's Update and PostUpdate) zeroes
//  0x004b1800 and then passes the manager to six __cdecl walks in turn:
//  0x00415b50, 0x00415d00, 0x00415bf0, 0x00415e40, 0x00415ec0, 0x00415f10.
//  All but 0x00415d00 are written here, with the sprite tracker update
//  0x00483d00 they share; all names are ours.
// ======================================================================= //

class CClientConnectionQ414
{
public:
	uint8 m_pad_00[0x1c];
	float m_fFrameTime;                                        // 0x1c
};

// A sprite frame: the texture it shows first.
class CSpriteFrameQ414
{
public:
	uint32 m_pTexture;                                         // 0x00
};

// What a sprite instance draws with; +0x30 takes the current frame's
// texture (0x00415b96).
//
// AVP2: (+0x30 is NOLF's) +0x2c in AvP2 and Primal Hunt (0x00419f86).
class CSpriteObjQ414
{
public:
	uint8  m_pad_00[0x2c];
	uint32 m_pCurTexture;                                      // 0x2c
};

// One animation of a loaded sprite, at the offsets 0x00483d00 reads:
// the frame array (+0x20, 4-byte entries: `lea (%eax,%edx,4)`), the frame
// count (+0x24), the animation's length in ms (+0x28, the wrap bound) and
// its frame rate (+0x2c, divided into 1000).  clientstreams.cpp's note on
// the loader 0x00483a60 records +0x28 = (1000 / msPerFrame) * nFrames.
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

// The sprite tracker, 20 bytes (cmodelload.cpp's 0x00477af4 indexes the
// model's pair of them by `iSkin * 20`): the sprite, the current
// animation, the frame it landed on, the time into the animation and the
// play/loop flags.  (names ours)
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

// One sprite instance: its object, the chain, and at +0x0c the tracker
// 0x00483d00 advances.

class CSpriteInstQ414
{
public:
	CSpriteObjQ414*    m_pObject;                              // 0x00
	CSpriteInstQ414*   m_pNext;                                // 0x04
	uint8              m_pad_08[0x0c - 0x08];
	CSpriteTrackerQ414 m_Tracker;                              // 0x0c
};

void spr_UpdateTrackerQ414(CSpriteTrackerQ414* pTracker,
                           uint32 msDelta);                    // 0x00483d00

// 0x00415b50 -- advance every sprite instance by this frame's time in
// milliseconds (`fmuls 1000.0f` at 0x00415b63, __ftol, unsigned `jbe`).
void cs_UpdateSpritesQ414(CClientMgrQ414* pMgr)
{
	CSpriteInstQ414* pInst;
	uint32           msDelta;

	if (pMgr->m_pConnection)
	{
#ifdef AVP2_MODERN
		// M-P1-23: the fraction of a millisecond carried to the next frame
		static float s_fP1Carry;
		msDelta = (uint32)P1_FrameMs(pMgr->m_pConnection->m_fFrameTime, &s_fP1Carry);
#else
		msDelta = (uint32)(pMgr->m_pConnection->m_fFrameTime * 1000.0f);
#endif

		if (msDelta > 0)
		{
			for (pInst = pMgr->m_pSpriteInsts; pInst; pInst = pInst->m_pNext)
			{
				spr_UpdateTrackerQ414(&pInst->m_Tracker, msDelta);

				if (pInst->m_Tracker.m_pCurFrame)
					pInst->m_pObject->m_pCurTexture =
					    pInst->m_Tracker.m_pCurFrame->m_pTexture;
			}
		}
	}
}

// ======================================================================= //
//  Primal Hunt 0x00419fa0, 157 bytes -- the per-frame object updates.
//
//  A thiscall with no arguments (`mov %ecx,%esi`, then the manager pushed
//  to each walk), called between the client shell's Update and PostUpdate.
//  It is timed: the dword at 0x004f15f8 (clientmgr2.cpp's g_Unknown4b1800)
//  is zeroed and then made the TOTAL of a scoped counter -- servermgr.h's
//  CountAdder, its destructor expanded (`push; call 0x00426160; add
//  %eax,(*m_pTotal)` at 0x0041a01b) -- so it ends up holding this frame's
//  update time.  The six walks run sprites (0x00419f40), animations
//  (0x0041a150, clientmgr2.cpp's), particle systems (0x0041a040), sprite
//  objects (0x0041a310), the pending moves (0x0041a390) and the model
//  skins (0x0041a3e0); one `add $0x20` pops the six arguments and the
//  counter's two.
// ======================================================================= //
#include "countadder.h"                // CBaseCounter, CountAdder

extern uint32 g_Unknown4b1800;                                 // 0x004f15f8

class CClientMgrView;
void cs_UpdateAnimations(CClientMgrView* pMgr);                // 0x0041a150
void cs_UpdateParticleSystemsQ414(CClientMgrQ414* pMgr);       // 0x0041a040
void cs_UpdateSpriteObjectsQ414(CClientMgrQ414* pMgr);         // 0x0041a310
void cs_ApplyPendingMovesQ414(CClientMgrQ414* pMgr);           // 0x0041a390
void cs_UpdateModelSkinsQ414(CClientMgrQ414* pMgr);            // 0x0041a3e0

void CClientMgrQ414::RunFrameUpdates()
{
	g_Unknown4b1800 = 0;

	CountAdder cntAdder(&g_Unknown4b1800);

	cs_UpdateSpritesQ414(this);
	cs_UpdateAnimations((CClientMgrView*)this);
	cs_UpdateParticleSystemsQ414(this);
	cs_UpdateSpriteObjectsQ414(this);
	cs_ApplyPendingMovesQ414(this);
	cs_UpdateModelSkinsQ414(this);
}


// The client object's flag word at +0x88, as the two object walks below
// use it: bits 3 and 30 each make the object live this frame, and bits 4
// and 30 are cleared on every pass with bit 4 set again for a live one
// (`and $0xbfffffef` at 0x00415c36 / 0x00415e79, `or $0x10,%al` at
// 0x00415c5c / 0x00415e91).  The test reads the flags as they were BEFORE
// the clear -- bit 30 is one of the two cleared -- so the source holds
// them in a local.  (names ours)
#define CF_LIVE08Q414      0x00000008
#define CF_UPDATED10Q414   0x00000010
#define CF_LIVE40MQ414     0x40000000

// A client particle system, at src/engine/lithtech/partfm.cpp's offsets
// (its CClientParticleSystemQ45C view cites each): +0x18c m_Pos, +0x1e8
// m_pTexture, +0x1f0 m_pSprite, +0x1f4 m_SpriteTracker, +0x22c
// m_nDirtyCount (signed: `test %edi,%edi ; jg` at 0x00415cbe).
class CParticleObjQ414
{
public:
	uint8              m_pad_000[0x88];
	uint32             m_Flags;                                // 0x088
	uint8              m_pad_8c[0x194 - 0x8c];
	LTVector           m_Pos;                                  // 0x194
	uint8              m_pad_1a0[0x1f0 - 0x1a0];
	uint32             m_pTexture;                             // 0x1f0
	uint8              m_pad_1f4[0x1f8 - 0x1f4];
	void*              m_pSprite;                              // 0x1f8
	CSpriteTrackerQ414 m_SpriteTracker;                        // 0x1fc
	uint8              m_pad_210[0x234 - 0x210];
	int                m_nDirtyCount;                          // 0x234
};

// partfm.cpp's three particle entries: save the old bounding sphere
// (0x0045cf10), the per-frame update ps_UpdateParticles (0x0045ca70), and
// "recompute the sphere; has it moved?" (0x0045cf40).
void   ps_SavePrevBoundsQ414(CParticleObjQ414* pSys);          // 0x0045cf10
void   ps_UpdateParticlesQ414(CParticleObjQ414* pSys,
                              float fFrameTime);               // 0x0045ca70
LTBOOL ps_BoundsChangedQ414(CParticleObjQ414* pSys);           // 0x0045cf40

class CMoveObjQ414;
void cs_SetObjectPosQ414(CClientMgrQ414* pMgr, CMoveObjQ414* pObj,
                         LTVector* pPos, int bForce);          // 0x004207a0

// 0x00415bf0 -- the third walk: update every particle system on the list
// at +0x46c.  The ms delta is SIGNED here (`test ; jle` at 0x00415c09),
// unlike the two sprite walks.
void cs_UpdateParticleSystemsQ414(CClientMgrQ414* pMgr)
{
	LTLink*           pCur;
	CParticleObjQ414* pSys;
	uint32            nFlags;
	int               nDirty;
	LTBOOL            bMoved;
	int               msDelta;

#ifdef AVP2_MODERN
	// M-P1-23: the fraction of a millisecond carried to the next frame
	static float s_fP1Carry;
	msDelta = (int)P1_FrameMs(pMgr->m_fFrameTime, &s_fP1Carry);
#else
	msDelta = (int)(pMgr->m_fFrameTime * 1000.0f);
#endif

	if (msDelta > 0)
	{
		for (pCur = pMgr->m_ParticleList.m_Head.m_pNext;
		     pCur != &pMgr->m_ParticleList.m_Head;
		     pCur = pCur->m_pNext)
		{
			pSys   = (CParticleObjQ414*)pCur->m_pData;
			nFlags = pSys->m_Flags;
			pSys->m_Flags = nFlags & ~(CF_UPDATED10Q414 | CF_LIVE40MQ414);

			if ((nFlags & CF_LIVE08Q414) || (nFlags & CF_LIVE40MQ414) ||
			    pSys->m_nDirtyCount)
			{
				pSys->m_Flags |= CF_UPDATED10Q414;

				if (pSys->m_pSprite)
				{
					spr_UpdateTrackerQ414(&pSys->m_SpriteTracker, msDelta);

					if (pSys->m_SpriteTracker.m_pCurFrame)
						pSys->m_pTexture = pSys->m_SpriteTracker.m_pCurFrame->m_pTexture;
					else
						pSys->m_pTexture = 0;
				}

				nDirty = pSys->m_nDirtyCount;
				ps_SavePrevBoundsQ414(pSys);
				ps_UpdateParticlesQ414(pSys, pMgr->m_fFrameTime);

				bMoved = ps_BoundsChangedQ414(pSys);

				if (nDirty > 0 || bMoved)
					cs_SetObjectPosQ414(pMgr, (CMoveObjQ414*)pSys, &pSys->m_Pos, 1);

				pSys->m_nDirtyCount = 0;
			}
		}
	}
}

// A client sprite object: its sprite at +0x1b0 and tracker at +0x1b4
// (0x00415e99 / 0x00415ea3).
//
// AVP2: (those are NOLF's) +0x1b8 / +0x1bc in AvP2 and Primal Hunt
// (0x0041a369, 0x0041a373).

class CSpriteObjectQ414
{
public:
	uint8              m_pad_000[0x88];
	uint32             m_Flags;                                // 0x088
	uint8              m_pad_08c[0x1b8 - 0x8c];
	void*              m_pSprite;                              // 0x1b8
	CSpriteTrackerQ414 m_SpriteTracker;                        // 0x1bc
};

// 0x00415e40 -- the fourth walk: advance every sprite object on the list
// at +0x47c.
void cs_UpdateSpriteObjectsQ414(CClientMgrQ414* pMgr)
{
	LTLink*            pCur;
	LTLink*            pListHead;
	CSpriteObjectQ414* pObj;
	uint32             nFlags;
	uint32             msDelta;

#ifdef AVP2_MODERN
	// M-P1-23: the fraction of a millisecond carried to the next frame
	static float s_fP1Carry;
	msDelta = (uint32)P1_FrameMs(pMgr->m_fFrameTime, &s_fP1Carry);
#else
	msDelta = (uint32)(pMgr->m_fFrameTime * 1000.0f);
#endif

	if (msDelta)
	{
		pListHead = &pMgr->m_SpriteList.m_Head;
		for (pCur = pListHead->m_pNext; pCur != pListHead; pCur = pCur->m_pNext)
		{
			pObj   = (CSpriteObjectQ414*)pCur->m_pData;
			nFlags = pObj->m_Flags;
			pObj->m_Flags = nFlags & ~(CF_UPDATED10Q414 | CF_LIVE40MQ414);

			if ((nFlags & CF_LIVE08Q414) || (nFlags & CF_LIVE40MQ414))
			{
				pObj->m_Flags |= CF_UPDATED10Q414;

				if (pObj->m_pSprite)
					spr_UpdateTrackerQ414(&pObj->m_SpriteTracker, msDelta);
			}
		}
	}
}

// An object on the move list: a pending position at +0x18c and its flag
// at +0x1ac (0x00415edb / 0x00415ee5 / 0x00415ef8).
//
// AVP2: (NOLF had +0x18c / +0x1ac) +0x194 / +0x1b4 in AvP2 and Primal
// Hunt (0x0041a3b5, 0x0041a3ab).
class CMoveObjQ414
{
public:
	uint8    m_pad_000[0x194];
	LTVector m_vPendingPos;                                    // 0x194
	uint8    m_pad_1a0[0x1b4 - 0x1a0];
	uint32   m_bPendingMove;                                   // 0x1b4
};

// clientmgr.h's sub_4207a0: set an object's position, the last argument
// the "force it however small" flag.
void cs_SetObjectPosQ414(CClientMgrQ414* pMgr, CMoveObjQ414* pObj,
                         LTVector* pPos, int bForce);          // 0x004207a0

// 0x00415ec0 -- apply every pending move on the list at +0x48c and clear
// the flag, set or not (the store at 0x00415ef8 is below the join).
void cs_ApplyPendingMovesQ414(CClientMgrQ414* pMgr)
{
	LTLink*       pCur;
	CMoveObjQ414* pObj;

	for (pCur = pMgr->m_MoveList.m_Head.m_pNext;
	     pCur != &pMgr->m_MoveList.m_Head;
	     pCur = pCur->m_pNext)
	{
		pObj = (CMoveObjQ414*)pCur->m_pData;

		if (pObj->m_bPendingMove)
			cs_SetObjectPosQ414(pMgr, pObj, &pObj->m_vPendingPos, 1);

		pObj->m_bPendingMove = 0;
	}
}


// The client model instance, at cmodelload.cpp's CClientModelInstQ474
// offsets: the skin textures, the skin sprites and their trackers (20
// bytes each: esi steps by 0x14 at 0x0041a486).
//
// AVP2: (NOLF had skins at +0x1b8, sprites at +0x210, records at +0x220)
// AvP2's skins are at +0x1c0, its sprites at +0x250 and their records at
// +0x260 (AvP2 0x00418245..0x00418251).
// PH: the sprites are 0x14 further on, at +0x264 with records from +0x274
// (0x0041a435..0x0041a441): the extra texture at +0x1d0 and the +4 after it.
class CTexRefQ414
{
public:
	uint8 m_pad_00[0x1c];
	char* m_pName;                                             // 0x1c
};

class CSpriteDataQ414
{
public:
	uint8        m_pad_00[0x24];
	CTexRefQ414* m_pTexRef;                                    // 0x24
};

class CModelInstQ414
{
public:
	uint8              m_pad_000[0x1c0];
	void*              m_pSkins[MAX_MODEL_TEXTURES];           // 0x1c0
#ifdef PRIMAL_HUNT
	uint8              m_pad_1d0[0x264 - 0x1d0];
#else
	uint8              m_pad_1d0[0x250 - 0x1d0];
#endif
	void*              m_pSprites[MAX_MODEL_TEXTURES];         // 0x250 (PH 0x264)
	CSpriteTrackerQ414 m_SpriteRecs[MAX_MODEL_TEXTURES];       // 0x260 (PH 0x274)
};

// 0x0041ff50 -- lttexmod.cpp's texture resolver (cmodelload.cpp's
// cm_LoadTextureQ474).
LTRESULT cm_LoadTextureQ414(CClientMgrQ414* pMgr, CClientFileRefQ414* pRef,
                            void** ppTexture);                 // 0x0041ff50

// 0x00415f10 -- the sixth walk: for every object on the main list, advance
// each sprite skin and re-resolve its texture from the frame it landed on.
// The file reference is built once, at the top (its three zeroing stores
// sit between the fld and __ftol at 0x00415f28..0x00415f30), and the
// texture is resolved through g_pClientMgr, not the argument (0x00415f8e).
void cs_UpdateModelSkinsQ414(CClientMgrQ414* pMgr)
{
	CClientFileRefQ414 ref;
	LTLink*            pCur;
	CModelInstQ414*    pObj;
	uint32             i;
	uint32             msDelta;

#ifdef AVP2_MODERN
	// M-P1-23: the fraction of a millisecond carried to the next frame
	static float s_fP1Carry;
	msDelta = (uint32)P1_FrameMs(pMgr->m_fFrameTime, &s_fP1Carry);
#else
	msDelta = (uint32)(pMgr->m_fFrameTime * 1000.0f);
#endif

	if (msDelta)
	{
		for (pCur = pMgr->m_ObjectList.m_Head.m_pNext;
		     pCur != &pMgr->m_ObjectList.m_Head;
		     pCur = pCur->m_pNext)
		{
			pObj = (CModelInstQ414*)pCur->m_pData;

			for (i = 0; i < MAX_MODEL_TEXTURES; i++)
			{
				if (pObj->m_pSprites[i])
				{
					spr_UpdateTrackerQ414(&pObj->m_SpriteRecs[i], msDelta);

					ref.m_pFilename = ((CSpriteDataQ414*)pObj->m_SpriteRecs[i].m_pCurFrame->m_pTexture)->m_pTexRef->m_pName;
					ref.m_nFileType = 1;
					cm_LoadTextureQ414(g_pClientMgrQ414, &ref, &pObj->m_pSkins[i]);
				}
			}
		}
	}
}


// 0x00483c60 spr_Destroy, 0x00483cc0 spr_InitTracker and 0x00483d00
// spr_UpdateTracker: sprite.cpp, the sprite module's own translation unit.
