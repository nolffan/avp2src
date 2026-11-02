// *********************************************************************** //
//
//  MODULE:   W8_45C000 (.CPP)  -- scratch file, NOT a drop-in TU
//
//  RANGE:    lithtech.exe 0x0045c000 .. 0x00460000 (seven enumerated rows)
//
//  GROUND TRUTH: the retail lithtech.exe only, cl 8447 /O2.  Every offset,
//  constant and branch below is cited by the address it was read at.
//
//  ==================== THE HEADLINE: 1 of 7 IS ALREADY WRITTEN ==========
//
//  Row-by-row verdict, before any code:
//
//   ADDR        SIZE   VERDICT
//   0x0045c390   363   ALREADY WRITTEN.  It IS ServerMessage::WriteData and
//                      src/engine/server/servermsg.cpp line 838 has the body.
//                      It is unmatched here for ONE reason, established
//                      below: the client's /O2 build EXPANDS
//                      CPacketData::SetAtGrow inline into the grow loop
//                      (0x0045c3ec..0x0045c4c6, 218 of the 363 bytes) where
//                      server.dll's /O1 build CALLS it.  /O2-shape work,
//                      not reconstruction.  NOT rewritten in this file.
//   0x0045c6a3   217   NOT A FUNCTION.  It is 0x22 bytes INTO the function
//                      that starts at 0x0045c680, and it CUTS AN INSTRUCTION
//                      IN HALF (the 10-byte `movl $0,0x1e8(%esi)` occupies
//                      0x0045c69b..0x0045c6a4).  The real row is
//                      0x0045c680 .. 0x0045c77c, 252 bytes.  Written below.
//                      The enumeration's neighbouring 0x0045c680/35 row is the
//                      other half of the same split.
//   0x0045ca70   880   NEW.  Written below.
//   0x0045cde0   289   NEW.  Written below.
//   0x0045d130   161   NEW, and it is NOT a thunk -- 0x0045d120 is the
//                      5-byte thunk (see below) and 0x0045d130 is the real
//                      body.  Written below.
//   0x0045d3b0   378   NEW.  fm_BlitPal8To8, the one hole in the thirteen-
//                      blitter family src/engine/lithtech/fmconvert.cpp
//                      already reconstructs.  Written below.
//   0x0045f6a0  2401   EXTENT WRONG.  The row is clipped at the 0x00460000
//                      region boundary.  There are FOUR branch
//                      targets leaving the range (0x0045f82b->0x00460148,
//                      0x0045f83b->0x0046012d, 0x0045fbd2 and 0x0045ff64
//                      ->0x00460105) and NO `ret` inside it.  The real
//                      function is 0x0045f6a0 .. 0x00460154, 2,741 bytes,
//                      ending at the `add $0xcc,%esp; ret` at 0x0046014e.
//                      Proved single by sweeping every e8/e9 in .text: the
//                      only displacement landing inside 0x0045f6a1..
//                      0x00460155 comes from inside the range itself, and
//                      the only entry is `call 0x45f6a0` at 0x0045ed1e.
//                      (The four dword "refs" a data sweep finds are in
//                      .rsrc and are coincidence.)  Written below.
//
//  So: SIX genuinely new functions, ONE already-written-but-unmatched.  The
//  four enumerated rows 0x0045c6a3 / 0x0045d130 / 0x0045d3b0 / 0x0045f6a0
//  that were flagged for checking were ALL worth checking: one is not
//  a function, one is not a thunk, one is a table entry in a family we
//  already own, and one has the wrong size.
//
//  ==================== THREE TRANSLATION UNITS, NOT ONE =================
//
//  The six new bodies do NOT belong to one module.  This file is a scratch
//  file and groups them by module with banners; each group belongs
//  spliced into its own TU.
//
//   A.  0x0045c680, 0x0045ca70, 0x0045cde0 -- the CLIENT PARTICLE SYSTEM.
//       Identified from the call graph, not from strings: 0x0045c680's only
//       caller is 0x0040b530, inside 0x0040b4f0, whose error path pushes
//       0x004a7f3c = "SetupParticleSystem"; src/engine/lithtech/
//       clientinterface.cpp already has that function as
//       ci_SetupParticleSystem and already DECLARES this callee as
//       `sub_45c680` in src/engine/lithtech/clientmgr.h line 541.  The same
//       header declares sub_45cfb0 (0x0045cfb0) and sub_45c780
//       (0x0045c780), the two neighbours, and defines CClientParticle and
//       CClientParticleSystem.  0x0045cde0's two callers (0x0045cf46 and
//       0x0045d10f) are both inside that same run.
//
//   B.  0x0045d130 and 0x0045d3b0 -- the PIXEL FORMAT CONVERTER.
//       0x0045d3b0 is entry 1 of the blitter table at 0x004acdb8 that
//       src/engine/lithtech/fmconvert.cpp's header already documents
//       (`srcFormat->code * 8 + destFormat->code`); entry 0 of the same
//       group is 0x0045d1e0 = fm_BlitSame and entries 2 and 3 are
//       fm_BlitPal8To16 / fm_BlitPal8To32, both already reconstructed.
//       0x0045d130 initialises the sixteen-entry PValue table at
//       0x004b40e8 that 0x0045f6a0 (and its twin 0x00460160) walks at
//       0x0045f7da..0x0045f7fb.
//
//   C.  0x0045f6a0 -- the ALPHA-AWARE BLITTER, past the end of
//       fmconvert.cpp's cluster.  Reached only through the 16-byte
//       forwarder at 0x0045ed10, which is entry 2 of the table group at
//       0x004ace40 and pushes two extra zero arguments.
//
//  ==================== THE 0x0045d120 "THUNK" ===========================
//
//  0x0045d120 is `jmp 0x45d130` and nothing else, and 0x004a70bc holds its
//  address.  0x004a7004..0x004a70d4 is the C++ dynamic-initialiser table
//  (.CRT$XCU): 53 entries, all of them pointing at .text.  TWENTY-NINE of
//  those 53 entries are exactly this shape -- a five-byte `jmp` at a
//  16-byte boundary followed by eleven `nop`s, jumping exactly 16 bytes
//  forward into the real body (0x00404ad0->0x00404ae0, 0x0040c0e0->
//  0x0040c0f0, 0x00415fd0->0x00415fe0, 0x0042f600->0x0042f610, ...).  Two
//  separately-aligned COMDATs, so the split is the compiler's, not the
//  linker's, and it is NOT source-reachable: no spelling of the
//  initialiser puts or removes that jump.  0x0045d130 is the initialiser
//  BODY and that is what is written here.  0x0045bfd0 shows the fuller
//  form of the same pattern for an object that also has a destructor
//  (`call ctor; jmp atexit-registrar`), which is what says these five-byte
//  stubs are the ??__E entry and not a linker stub.
//
//  ==================== WHY 0x0045c390 IS NOT REWRITTEN ==================
//
//  src/engine/server/servermsg.cpp records ServerMessage::WriteData at
//  server.dll 0x10026060 and the lithtech copy at 0x0045c390, 363 bytes.
//  Reading the shipped client bytes confirms the body line for line:
//
//     0x0045c39d/0x0045c3a1   m_Unknown38 (+0x38) and m_MaxDataLen (+0x36)
//                             loaded as 16-bit, exactly the source's two
//                             uint16 locals
//     0x0045c3c2/0x0045c3c4   the `nPos + (uint16)nBytes >= nLimit` clip,
//                             with the else arm's `mov %edx,%ebx`
//     0x0045c3e4/0x0045c3e6   `while (m_Unknown38 + nCount >= m_DataSize)`
//                             (+0x58 is m_DataSize, +0x5c the alloc size,
//                             +0x54 the buffer)
//     0x0045c4ca..0x0045c4e7   the `memcpy(&m_pData[m_Unknown38], ...)`
//                             as `rep movsl` + `rep movsb`
//     0x0045c4e9/0x0045c4ed   `m_Unknown38 += nCount; m_Unknown34 += nCount`
//                             as two 16-bit `add`s
//
//  The whole of 0x0045c3ec..0x0045c4c6 is CPacketData::SetAtGrow expanded
//  inline -- 0x0045c40a and 0x0045c466 both push 0x004b4e88, which is
//  g_PacketDataBank, exactly the argument the source's
//  `pPData->SetAtGrow(pPData->m_AllocSize, &dummy, &g_PacketDataBank)`
//  passes.  Nothing in this range is new source; the gap is /Ob2 reach,
//  which is a build-shape question.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltpvalue.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>


// ======================================================================= //
//
//  A.  THE CLIENT PARTICLE SYSTEM  (0x0045c680, 0x0045ca70, 0x0045cde0)
//
//  Class views.  Everything below 0x1a8 is src/engine/lithtech/clientmgr.h's
//  CClientObject verbatim; from 0x1a8 up, clientmgr.h already has the list
//  node, the bank pointer, the software colour, m_nParticles, the two
//  floats and the flag word.  The SIX fields this file adds are the ones
//  the three bodies here reach and clientmgr.h pads over, each cited:
//
//    +0x1e8  m_pTexture      0x0045c69b writes 0, 0x0045c712 writes the
//                            SharedTexture 0x0041ff90 hands back, 0x0045c76d
//                            writes the sprite frame's first word
//    +0x1f0  m_pSprite       0x0045c6a5 writes 0, 0x0045c73f passes its
//                            ADDRESS to 0x00476e80 (whose third parameter
//                            clientmgr.h line 539 already types as
//                            `void** ppTexture`)
//    +0x1f4  m_SpriteTracker 0x0045c751 passes its address to 0x00483cc0,
//                            which clientmgr.h line 540 already pairs with
//                            a sprite; +0x8 of it (0x0045c75e) is the frame,
//                            which is CSpriteTracker::m_pFrame08 in
//                            src/engine/lithtech/cmodelload.cpp's view of
//                            the identical structure at model+0x210
//    +0x208  m_Center        written by 0x0045cde0, copied to +0x218 by
//                            0x0045cf10, zeroed by 0x0045cfb0 (0x0045cfef)
//    +0x214  m_Radius        0x0045cef7 stores the sqrt; 0x0045d001 seeds
//                            it with 1.0f when the list is empty
//    +0x218  m_PrevCenter    0x0045cf15..0x0045cf3e copies +0x208 to +0x218
//    +0x224  m_PrevRadius    and +0x214 to +0x224 in the same function
//    +0x22c  m_nDirtyCount   incremented beside m_nParticles at 0x0045ca4a
//                            and reset to 0 by the caller at 0x00415cdd
//    +0x230  m_BoxMin        the six `fcomps` sites at 0x0045cb59..
//    +0x23c  m_BoxMax        0x0045cbdc name all six words; 0x0045cfb0
//                            seeds min with +100000.0f (0x47c35000) and max
//                            with its negation, which fixes which is which
//
//  Nothing here is a guess about the flag word either: bit 3 (0x0045cadf)
//  disables the update entirely, bit 2 (0x0045cb1b) selects the no-lifetime
//  integration, bit 0 (0x0045cd38) enables the floor bounce.
//
// ======================================================================= //

#define OT_PARTICLESYSTEMQ45C   9

typedef void*  HLOCALOBJQ45C;
typedef uint32 HPOLYQ45C;

class CClientMgrQ45C;

// clientmgr.h's CClientFileRef verbatim (0xc bytes, inline constructor).
class CClientFileRefQ45C
{
public:
	CClientFileRefQ45C() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// cmodelload.cpp's CSpriteFrame / CSpriteTracker, the same 0x14-byte record
// the model instance keeps at +0x210.  Only the two words 0x0045c680 reads
// are modelled.  cmodelload.cpp calls the frame's first word `m_pData` (a
// CSpriteData*); this function stores it straight into m_pTexture, so
// either that word is the shared texture or the particle system holds a
// CSpriteData in the same slot.  It is written as an opaque pointer because
// the bytes cannot tell the two apart.
class CSpriteFrameQ45C
{
public:
	void* m_pTexture;                        // 0x00
};

class CSpriteTrackerQ45C
{
public:
	uint8             m_pad_00[0x08];        // 0x00
	CSpriteFrameQ45C* m_pFrame08;            // 0x08
	uint8             m_pad_0c[0x08];        // 0x0c  (was missing: the
	                                         // record IS 0x14, and without
	                                         // it m_Center sat at 0x200)
};                                           // 0x14

// clientmgr.h's CClientParticle: the SDK's LTParticle (m_Velocity 0x00,
// m_Color 0x0c, m_Alpha 0x18, m_Size 0x1c, m_pNext 0x20) with the engine's
// own tail on the end.
class CClientParticleQ45C
{
public:
	LTVector             m_Velocity;         // 0x00
	LTVector             m_Color;            // 0x0c
	float                m_Alpha;            // 0x18
	float                m_Size;             // 0x1c
	CClientParticleQ45C* m_pNext;            // 0x20
	LTVector             m_Pos;              // 0x24
	float                m_Lifetime;         // 0x30
	float                m_TotalLifetime;    // 0x34
	CClientParticleQ45C* m_pPrev;            // 0x38
};                                           // 0x3c

// The StructBank free list, as far as the inlined sb_Free at
// 0x0045cc3b..0x0045cc46 needs it: the head is at +0x18 and a freed record
// chains through its own first word.
class StructBankQ45C
{
public:
	uint8  m_pad_00[0x08];                   // 0x00
	uint32 m_nGrowCount;                     // 0x08  (0x0045c8f4, passed to sb_Grow)
	uint8  m_pad_0c[0x18 - 0x0c];            // 0x0c
	void*  m_pFreeList;                      // 0x18
};

class CClientParticleSystemQ45C
{
public:
	uint8               m_pad_00[0x18c];     // 0x000
	LTVector            m_Pos;               // 0x18c
	uint8               m_pad_198[0x1a8 - 0x198];
	CClientParticleQ45C m_ParticleTail;      // 0x1a8  (m_pNext at 0x1c8)
	StructBankQ45C*     m_pParticleBank;     // 0x1e4
	void*               m_pTexture;          // 0x1e8
	uint8               m_pad_1ec[0x1f0 - 0x1ec];
	void*               m_pSprite;           // 0x1f0
	CSpriteTrackerQ45C  m_SpriteTracker;     // 0x1f4  (ends at 0x208)
	LTVector            m_Center;            // 0x208
	float               m_Radius;            // 0x214
	LTVector            m_PrevCenter;        // 0x218
	float               m_PrevRadius;        // 0x224
	uint32              m_nParticles;        // 0x228
	uint32              m_nDirtyCount;       // 0x22c
	LTVector            m_BoxMin;            // 0x230
	LTVector            m_BoxMax;            // 0x23c
	float               m_fGravityAccel;     // 0x248
	float               m_fParticleRadius;   // 0x24c
	uint8               m_pad_250[0x258 - 0x250];
	uint32              m_nPSFlags;          // 0x258
};

// The particle-system flag bits, numeric in the shipped code.
#define PSFLAG_BOUNCEQ45C     0x01           // 0x0045cd38 `test $0x1,%bl`
#define PSFLAG_NOLIFETIMEQ45C 0x04           // 0x0045cb1b `test $0x4,%bl`
#define PSFLAG_FROZENQ45C     0x08           // 0x0045cadf `test $0x8,%bl`

// ----------------------------------------------------------------------- //
//  Neighbours these three call but do not own.
// ----------------------------------------------------------------------- //

// 0x0042ee70 -- console.cpp's case-insensitive EQUALS.  It answers TRUE on
// a match, which is why the DTX arm is the `jne` fall-through and not the
// `je` (src/engine/lithtech/clientworld.cpp and cmodelload.cpp both name
// it, and cmodelload.cpp's cm_InitSkin uses it in this exact idiom).
LTBOOL con_ICompare(char* pA, char* pB);

// 0x0041ff90 -- lttexmod.cpp's "resolve one CClientFileRef to a
// SharedTexture, or null".  __cdecl, two arguments, returns the texture.
void* sub_41ff90Q45C(CClientMgrQ45C* pClientMgr, CClientFileRefQ45C* pRef);

// 0x00476e80 and 0x00483cc0 -- clientmgr.h lines 539/540 verbatim.
LTRESULT sub_476e80Q45C(CClientMgrQ45C* pClientMgr, CClientFileRefQ45C* pRef,
                        void** ppSprite);
void     sub_483cc0Q45C(CSpriteTrackerQ45C* pTracker, void* pSprite);

// 0x00404aa0 -- clientinterface.h line 331's ci_IntersectSegment.
LTBOOL ci_IntersectSegmentQ45C(ClientIntersectQuery* pQuery,
                               ClientIntersectInfo* pInfo);


// ----------------------------------------------------------------------- //
//  The bounds-extend block, written out in full at THREE sites in the
//  image: 0x0045cb56..0x0045cbec and 0x0045cc8c..0x0045cd22 inside
//  0x0045ca70, and 0x0045d075..0x0045d10c inside 0x0045cfb0 (which is not
//  an enumerated row).  Three identical expansions of six `if`s is a MACRO or
//  an inline function; it is the inline function (see below: the macro
//  form kept x on the FPU stack and did not match).
//
//  The sense of each test is read off the condition codes, not assumed:
//  0x0045cb59 `fcomps 0x230(%esi)` + `test $0x1,%ah` + `je` stores when C0
//  is set, i.e. when the particle is BELOW the minimum, and 0x0045cba4
//  `test $0x41,%ah` + `jne` skips on "less or equal", i.e. stores when the
//  particle is ABOVE the maximum.  All six re-read the particle's own
//  component for the store (0x0045cb66 loads 0x24(%ecx) again after
//  0x0045cb56 already compared it), which is the repeated-field
//  reading: the source writes the expression out at each use.
// ----------------------------------------------------------------------- //
// MEASURED: an inline function over `LTVector* pPos`, not a macro over the
// particle.  Every site RE-LOADS the particle's x (0x0045cb56 `flds
// 0x24(%ecx)` straight after 0x0045cb53's `fstps`); spelled through the
// particle (as a macro, or as an inline taking the particle) /O2 kept the
// just-stored x on the FPU stack instead (`fsts`).  The macro name is kept
// as a forwarder.
inline void ps_ExtendBoundsQ45C(CClientParticleSystemQ45C* pSys,
                                LTVector* pPos)
{
	if (pPos->x < pSys->m_BoxMin.x)
		pSys->m_BoxMin.x = pPos->x;
	if (pPos->y < pSys->m_BoxMin.y)
		pSys->m_BoxMin.y = pPos->y;
	if (pPos->z < pSys->m_BoxMin.z)
		pSys->m_BoxMin.z = pPos->z;
	if (pPos->x > pSys->m_BoxMax.x)
		pSys->m_BoxMax.x = pPos->x;
	if (pPos->y > pSys->m_BoxMax.y)
		pSys->m_BoxMax.y = pPos->y;
	if (pPos->z > pSys->m_BoxMax.z)
		pSys->m_BoxMax.z = pPos->z;
}

#define PS_EXTEND_BOUNDSQ45C(pSys, pParticle) ps_ExtendBoundsQ45C(pSys, &(pParticle)->m_Pos)


// ======================================================================= //
//  0x0045c680 .. 0x0045c77b, 252 bytes  --  the particle system's TEXTURE
//
//  clientmgr.h line 541 already declares this as
//      LTRESULT sub_45c680(HLOCALOBJ hObj, CClientMgr*, char* pTextureName)
//  and clientinterface.cpp's ci_SetupParticleSystem already calls it with
//  exactly those three, in that order (0x0040b52f pushes the object last,
//  0x0040b528 the manager out of 0x004b17e0 = g_pClientMgr, 0x0040b50d the
//  name).  This is its body.
//
//  IT IS cm_InitSkin.  src/engine/lithtech/cmodelload.cpp already
//  reconstructs the same extension test at 0x00477a20 (cm_InitSkinQ474) and
//  the two are instruction-for-instruction the same idiom:
//
//    * ONE written-out strlen -- `or $-1,%ecx; xor %eax,%eax; repnz scasb;
//      not %ecx; dec %ecx` (0x0045c696..0x0045c6b7).  Two scans
//      would mean two calls; there is one.
//    * the last three characters copied one byte at a time into a four-byte
//      buffer that lives in the DEAD hObj argument slot (0x0045c6d6 writes
//      R+0x04, 0x0045c6de R+0x05, 0x0045c6ea R+0x06) and NUL-terminated at
//      0x0045c6ef.
//    * compared against "DTX" (0x004acd74) first and "SPR" (0x004acd70)
//      second, with the DTX arm on the `jne` fall-through.
//
//  WHY THE GUARD IS POSITIVE.  0x0045c6c7 `jle 0x45c773` jumps FORWARD to
//  `xor %eax,%eax`, which sits past every other return and falls into the
//  single shared epilogue at 0x0045c775.  That block is
//  the TRAILING STATEMENT and the guard above it is positive -- `if (nLen >
//  3)`, not `if (nLen <= 3) return LT_OK;`.  Two more jumps land on the
//  same block (0x0045c735 when the name is not SPR, 0x0045c769 when the
//  sprite has no frame), which is three producers for one exit, and the
//  DTX arm has its OWN copy of the epilogue at 0x0045c718 --
//  "several duplicated exits" for the one arm that returns early
//  and one shared exit for the nest.
//
//  THE THREE STORE ORDERS, read off the bytes rather than assumed:
//    0x0045c69b  m_pTexture = LTNULL   comes before
//    0x0045c6a5  m_pSprite  = LTNULL
//  and the CClientFileRef's three-word constructor is dead-store-eliminated
//  down to the single `movw $0,0x14(%esp)` at 0x0045c6ac, with
//  `movl $1,0xc(%esp)` (m_nFileType) at 0x0045c6b8 and the filename at
//  0x0045c6c3 -- exactly ci_SetPolyGridTexture's spelling in
//  clientinterface.cpp, where the same constructor survives the same way.
// ======================================================================= //
LTRESULT sub_45c680Q45C(HLOCALOBJQ45C hObj, CClientMgrQ45C* pClientMgr,
                        char* pTextureName)
{
	CClientParticleSystemQ45C* pSys = (CClientParticleSystemQ45C*)hObj;
	CClientFileRefQ45C         ref;
	char                       szExt[4];
	int                        nLen;

	pSys->m_pTexture = LTNULL;
	pSys->m_pSprite  = LTNULL;

	// Both assignments are emitted AFTER the `repnz scasb` (0x0045c6b8 and
	// 0x0045c6c3) but BEFORE the length guard at 0x0045c6c7, so they are
	// unconditional and outside the `if`; whether the source writes them
	// above or below the strlen is scheduling, not order -- %edx has to
	// survive into %edi for the scan either way.
	ref.m_nFileType = 1;
	ref.m_pFilename = pTextureName;

	nLen = strlen(pTextureName);

	if (nLen > 3)
	{
		szExt[0] = pTextureName[nLen - 3];
		szExt[1] = pTextureName[nLen - 2];
		szExt[2] = pTextureName[nLen - 1];
		szExt[3] = 0;

		// 0x0045c70a -- two arguments only; the texture comes back as the
		// return value and goes straight into the slot.
		if (con_ICompare(szExt, "DTX"))
		{
			pSys->m_pTexture = sub_41ff90Q45C(pClientMgr, &ref);

			return LT_OK;
		}

		if (con_ICompare(szExt, "SPR"))
		{
			LTRESULT dResult = sub_476e80Q45C(pClientMgr, &ref,
			                                  &pSys->m_pSprite);

			if (dResult)
				return dResult;

			sub_483cc0Q45C(&pSys->m_SpriteTracker, pSys->m_pSprite);

			if (pSys->m_SpriteTracker.m_pFrame08)
			{
				pSys->m_pTexture =
					pSys->m_SpriteTracker.m_pFrame08->m_pTexture;
			}
		}
	}

	return LT_OK;
}


// ======================================================================= //
//  0x0045c780 .. 0x0045ca6d, 750 bytes  --  add N random particles
//
//  clientmgr.h's sub_45c780; ci_AddParticles (clientinterface.cpp) is its
//  only caller and passes its ten arguments straight through.  Per
//  particle: six rand() draws scaled by 1/RAND_MAX (0x004a23e0 is
//  0x38000100 = 3.0518509e-05f), position / velocity / colour lerped
//  between the min and max vectors, lifetime between the two floats, a
//  record from the system's struct bank (servermgr.h's inline sb_Allocate
//  shape: grow through 0x00490c20 and re-read), the bounds extended by the
//  same inline 0x0045ca70 uses (here over the LOCAL position,
//  0x0045c91f..0x0045c9bb), then linked in before the tail.
//
//  WHICH DRAW FEEDS WHICH COMPONENT is read off the bytes, not tidied:
//  position uses draws 0,1,2 (0x0045c841/51/66), velocity 3,4,5
//  (0x0045c875/85, the sixth left on the FPU stack at 0x0045c89a), colour
//  0,5,3 (0x0045c8a7, 0x0045c8b7, 0x0045c8d2) and the lifetime 3
//  (0x0045c8e6).
//
//  THE FRAME (0x3c) is three vectors at R-0x3c/-0x30/-0x24 (position,
//  colour, velocity) and the draws as ONE `float[6]` at R-0x18..R-0x01 --
//  its sixth word is never stored because that draw stays on the FPU
//  stack.  Measured: as two LTVectors (or six scalars) the draws took the
//  slots BELOW colour and velocity, and no declaration order or scope moved
//  them; the array is what puts them on top.
// ======================================================================= //
int sb_GrowQ45C(StructBankQ45C* pBank, uint32 nCount);      // 0x00490c20

inline void* sb_AllocateQ45C(StructBankQ45C* pBank)
{
	void* pRet;

	pRet = pBank->m_pFreeList;
	if (!pRet)
	{
		sb_GrowQ45C(pBank, pBank->m_nGrowCount);
		pRet = pBank->m_pFreeList;
		if (!pRet)
			return 0;
	}

	pBank->m_pFreeList = *(void**)pRet;
	return pRet;
}

void sub_45c780Q45C(HLOCALOBJQ45C hObj, uint32 nParticles, LTVector* pMinOffset,
                    LTVector* pMaxOffset, LTVector* pMinVelocity,
                    LTVector* pMaxVelocity, LTVector* pMinColor,
                    LTVector* pMaxColor, float minLifetime, float maxLifetime)
{
	CClientParticleSystemQ45C* pSys = (CClientParticleSystemQ45C*)hObj;
	CClientParticleQ45C*       pParticle;
	uint32                     i;
	float                      fRand[6];
	float                      fLifetime;

	for (i = 0; i < nParticles; i++)
	{
		LTVector vPos, vColor, vVelocity;

		fRand[0] = (float)rand() * (1.0f / RAND_MAX);
		fRand[1] = (float)rand() * (1.0f / RAND_MAX);
		fRand[2] = (float)rand() * (1.0f / RAND_MAX);
		fRand[3] = (float)rand() * (1.0f / RAND_MAX);
		fRand[4] = (float)rand() * (1.0f / RAND_MAX);
		fRand[5] = (float)rand() * (1.0f / RAND_MAX);

		vPos.x = (pMaxOffset->x - pMinOffset->x) * fRand[0] + pMinOffset->x;
		vPos.y = (pMaxOffset->y - pMinOffset->y) * fRand[1] + pMinOffset->y;
		vPos.z = (pMaxOffset->z - pMinOffset->z) * fRand[2] + pMinOffset->z;

		vVelocity.x = (pMaxVelocity->x - pMinVelocity->x) * fRand[3] + pMinVelocity->x;
		vVelocity.y = (pMaxVelocity->y - pMinVelocity->y) * fRand[4] + pMinVelocity->y;
		vVelocity.z = (pMaxVelocity->z - pMinVelocity->z) * fRand[5] + pMinVelocity->z;

		vColor.x = (pMaxColor->x - pMinColor->x) * fRand[0] + pMinColor->x;
		vColor.y = (pMaxColor->y - pMinColor->y) * fRand[5] + pMinColor->y;
		vColor.z = (pMaxColor->z - pMinColor->z) * fRand[3] + pMinColor->z;

		fLifetime = (maxLifetime - minLifetime) * fRand[3] + minLifetime;

		pParticle = (CClientParticleQ45C*)sb_AllocateQ45C(pSys->m_pParticleBank);
		if (!pParticle)
			continue;

		ps_ExtendBoundsQ45C(pSys, &vPos);

		pParticle->m_Alpha         = 1.0f;
		pParticle->m_Pos           = vPos;
		pParticle->m_Color         = vColor;
		pParticle->m_Velocity      = vVelocity;
		pParticle->m_Lifetime      = fLifetime;
		pParticle->m_TotalLifetime = fLifetime;
		pParticle->m_Size          = pSys->m_fParticleRadius;

		pParticle->m_pPrev = pSys->m_ParticleTail.m_pPrev;
		pParticle->m_pNext = &pSys->m_ParticleTail;
		pParticle->m_pNext->m_pPrev = pParticle;
		pParticle->m_pPrev->m_pNext = pParticle;

		pSys->m_nParticles++;
		pSys->m_nDirtyCount++;
	}
}


// ======================================================================= //
//  0x0045ca70 .. 0x0045cddf, 880 bytes  --  the per-frame particle update
//
//  Its one caller is 0x00415cb0, which sits in the same 99-byte function
//  (0x00415c9a) that calls 0x0045cf10 just before it and 0x0045cf40 just
//  after -- "save the old bounding sphere / update / has it moved?" -- and
//  then clears +0x22c at 0x00415cdd.  Two arguments: the system at R+0x04
//  (0x0045ca75) and a float at R+0x08 (first read at 0x0045cb05).
//
//  THE THREE LOCALS AND WHY THEY ARE THOSE TYPES.  0x0045ca79..0x0045cacf
//  zeroes twenty-two stack dwords and 0x0045cae5 writes -1 into one more.
//  Laid out against the frame (R-0x68 .. R-0x01, 104 bytes) they are
//  exactly three SDK objects with their exact constructors:
//
//    R-0x68  LTVector             12 bytes, NOT zeroed -- assigned from
//                                 m_Pos at 0x0045caed/0x0045caf4/0x0045cafb
//    R-0x5c  ClientIntersectInfo  40 bytes: seven zeroed dwords (m_Point
//                                 and m_Plane), m_hObject = 0, then
//                                 m_hPoly = -1 at R-0x3c (0x0045cae5,
//                                 INVALID_HPOLY) and m_SurfaceFlags = 0 at
//                                 R-0x38.  That is IntersectInfo's
//                                 constructor, field for field.
//    R-0x34  ClientIntersectQuery 52 bytes, ALL THIRTEEN dwords zeroed --
//                                 IntersectQuery's constructor Inits three
//                                 vectors and zeroes four scalars, which is
//                                 thirteen dwords and nothing else.
//
//  and 0x0045cd75/0x0045cd76 push R-0x5c then R-0x34, so the call is
//  f(&query, &info) -- ci_IntersectSegment's signature.  The two objects
//  are declared query-then-info in the source and VC6 gives the LATER one
//  the lower address, which is why info sits below query in the frame.
//
//  WHY THE ENTRY GUARD IS POSITIVE.  0x0045cadf `test $0x8,%bl` +
//  0x0045caff `jne 0x45cdd9` jumps FORWARD to the bare epilogue, the only
//  `ret` in the function -- one shared exit, so the whole body is the
//  positive arm of `if (!(m_nPSFlags & 8))` and not an early return.
//  0x0045cb22 and 0x0045cc08 and 0x0045cd3b all land on that same exit.
//
//  THE THREE PARTS, in source order:
//   1. flags & 4  -> integrate position and velocity only  (0x0045cb28)
//      else       -> the same plus the lifetime countdown and the free
//                    (0x0045cc06).  Both arms open with the SAME
//                    `pCur != &m_ParticleTail` test against %edi, which is
//                    `lea 0x1a8(%esi)` hoisted above the branch at
//                    0x0045cb15 -- the tail address is one expression used
//                    by both arms, so it is a local.
//   2. the free, inlined (0x0045cc29..0x0045cc50): unlink, push onto the
//      bank's free list through the record's own first word, decrement
//      m_nParticles.  clientinterface.cpp's ci_RemoveParticle writes the
//      identical three steps as a call to sb_Free; here /O2 expanded it.
//   3. flags & 1  -> drop a segment 400 units straight down from the
//      system's position and, if it hits, bounce every particle that is
//      falling and below the plane (0x0045cd38..0x0045cdd7).
//
//  THE FLOOR PRODUCT.  0x0045cd87 loads m_Dist (info+0x18) and multiplies
//  by m_Normal.y (info+0x10); VC6 at /O2 loads the RIGHT factor first
//  here, so the source is `m_Normal.y * m_Dist` (measured: the other
//  spelling swaps the two operands and is the only residual).
// ======================================================================= //
void ps_UpdateParticlesQ45C(CClientParticleSystemQ45C* pSys, float fFrameTime)
{
	LTVector             vPos;
	ClientIntersectQuery iQuery;
	ClientIntersectInfo  iInfo;
	uint32               dwFlags;

	vPos = pSys->m_Pos;

	// 0x0045cad9 -- the flag word is loaded ONCE into %ebx and all three
	// tests (0x0045cadf, 0x0045cb1b, 0x0045cd38) read the register.
	dwFlags = pSys->m_nPSFlags;

	if (!(dwFlags & PSFLAG_FROZENQ45C))
	{
		CClientParticleQ45C* pTail = &pSys->m_ParticleTail;
		CClientParticleQ45C* pCur  = pSys->m_ParticleTail.m_pNext;
		float                fGravity = fFrameTime * pSys->m_fGravityAccel;

		if (dwFlags & PSFLAG_NOLIFETIMEQ45C)
		{
			while (pCur != pTail)
			{
				pCur->m_Pos.x += fFrameTime * pCur->m_Velocity.x;
				pCur->m_Pos.y += fFrameTime * pCur->m_Velocity.y;
				pCur->m_Pos.z += fFrameTime * pCur->m_Velocity.z;

				PS_EXTEND_BOUNDSQ45C(pSys, pCur);

				pCur->m_Velocity.y = fGravity + pCur->m_Velocity.y;

				pCur = pCur->m_pNext;
			}
		}
		else
		{
			while (pCur != pTail)
			{
				// 0x0045cc0f..0x0045cc27 -- `fsts` (store WITHOUT pop) then
				// `fcomps 0.0`, so the subtraction is written back to the
				// member and the SAME value is tested.
				pCur->m_Lifetime = pCur->m_Lifetime - fFrameTime;

				if (pCur->m_Lifetime < 0.0f)
				{
					CClientParticleQ45C* pNext = pCur->m_pNext;

					pCur->m_pPrev->m_pNext = pCur->m_pNext;
					pCur->m_pNext->m_pPrev = pCur->m_pPrev;

					// 0x0045cc3b loads the bank pointer ONCE (%edx).
					StructBankQ45C* pBank = pSys->m_pParticleBank;
					*(void**)pCur = pBank->m_pFreeList;
					pBank->m_pFreeList = pCur;

					pSys->m_nParticles--;

					pCur = pNext;
				}
				else
				{
					pCur->m_Pos.x += fFrameTime * pCur->m_Velocity.x;
					pCur->m_Pos.y += fFrameTime * pCur->m_Velocity.y;
					pCur->m_Pos.z += fFrameTime * pCur->m_Velocity.z;

					PS_EXTEND_BOUNDSQ45C(pSys, pCur);

					pCur->m_Velocity.y = fGravity + pCur->m_Velocity.y;

					pCur = pCur->m_pNext;
				}
			}
		}

		if (dwFlags & PSFLAG_BOUNCEQ45C)
		{
			iQuery.m_From = vPos;

			iQuery.m_To.x = vPos.x;
			iQuery.m_To.y = vPos.y - 400.0f;
			iQuery.m_To.z = vPos.z;

			if (ci_IntersectSegmentQ45C(&iQuery, &iInfo))
			{
				float fFloorY = iInfo.m_Plane.m_Normal.y * iInfo.m_Plane.m_Dist
				                - vPos.y;

				pCur = pSys->m_ParticleTail.m_pNext;

				while (pCur != pTail)
				{
					if (pCur->m_Velocity.y < 0.0f && pCur->m_Pos.y < fFloorY)
					{
						pCur->m_Pos.y      = fFloorY;
						pCur->m_Velocity.y = pCur->m_Velocity.y * -0.5f;
					}

					pCur = pCur->m_pNext;
				}
			}
		}
	}
}


// ======================================================================= //
//  0x0045cde0 .. 0x0045cf00, 289 bytes  --  bounding sphere from the box
//
//  __cdecl, one argument (0x0045cde3 reads R+0x04).  Both callers push and
//  clean four bytes: 0x0045cf46, inside the 47-byte "save the previous
//  sphere" wrapper's neighbour 0x0045cf40, and 0x0045d10f at the tail of
//  0x0045cfb0.
//
//  WHAT PINS THE C++ OPERATORS RATHER THAN THE VEC_ MACROS.  ltvector.h's
//  `operator-(const _CVector<T> v)` takes its right-hand side BY VALUE, and
//  the shipped code copies m_BoxMin's three words to the stack
//  (0x0045cdf6/0x0045ce00/0x0045ce15) before subtracting them -- a
//  VEC_SUB macro would read the member in place and copy nothing.  The
//  second add is the mirror image: m_Pos is the one that gets copied
//  (0x0045cea6/0x0045ceb6/0x0045cebd) while m_Center is read in place
//  through %eax, so m_Center is `this` and m_Pos is the by-value argument.
//
//  THE SQRT IS INLINE (`fsqrt` at 0x0045ceef, no call), which is
//  ltvector.h's `Mag()` under /O2's implied /Oi.  Its three products are
//  summed z,y,x by the scheduler; operand order is not readable
//  off a float sum, so they are written x,y,z as Mag() spells them.
//
//  0x004a1a74 is 0.5f and 0x004a2ec0 is 400.0f, both read as floats.
// ======================================================================= //
void ps_UpdateBoundsQ45C(CClientParticleSystemQ45C* pSys)
{
	LTVector vDims = pSys->m_BoxMax - pSys->m_BoxMin;
	LTVector vHalf = vDims * 0.5f;

	pSys->m_Center  = pSys->m_BoxMin + vHalf;
	pSys->m_Center += pSys->m_Pos;

	pSys->m_Radius = vHalf.Mag();
}


// 0x0045cf10, 47 bytes -- save the sphere before the update: m_Center to
// m_PrevCenter (three words through `lea 0x208`/`lea 0x218`) and m_Radius
// to m_PrevRadius.  Its caller 0x00415c9a calls it just before the update,
// and 0x0045cf40 below compares against what it saved (name ours).
void ps_SaveBoundsQ45C(CClientParticleSystemQ45C* pSys)
{
	pSys->m_PrevCenter = pSys->m_Center;
	pSys->m_PrevRadius = pSys->m_Radius;
}


// 0x0045cf40, 101 bytes -- "recompute the sphere; has it moved?".  Its
// caller 0x00415c9a (csvis.cpp's cs_UpdateParticleSystemsQ414) runs it
// after the update.  Refreshes the sphere (call 0x0045cde0 at 0x0045cf46),
// then 1 unless the centre equals the one 0x0045cf10 saved and the radius
// has not grown (`test $0x41` at 0x0045cf95).
LTBOOL ps_BoundsChangedQ45C(CClientParticleSystemQ45C* pSys)
{
	ps_UpdateBoundsQ45C(pSys);

	if (pSys->m_PrevCenter == pSys->m_Center &&
	    pSys->m_Radius <= pSys->m_PrevRadius)
		return LTFALSE;

	return LTTRUE;
}


// 0x0045cfb0, 365 bytes -- clientmgr.h's sub_45cfb0(hSystem), called from
// clientinterface.cpp: recompute the box and sphere from scratch.  An empty
// list zeroes the box and centre (0x0045cfcb..0x0045cff5) and seeds the
// radius with 1.0f; otherwise the box is seeded +/-100000.0f (0x47c35000,
// max through the unary operator- temporary at R+0x08), every particle
// extends it (the third PS_EXTEND_BOUNDSQ45C site, 0x0045d075), and
// 0x0045cde0 derives the sphere (0x0045d10f).
void sub_45cfb0Q45C(CClientParticleSystemQ45C* pSys)
{
	CClientParticleQ45C* pTail = &pSys->m_ParticleTail;
	CClientParticleQ45C* pCur;

	if (pSys->m_ParticleTail.m_pNext == pTail)
	{
		pSys->m_BoxMin.Init();
		pSys->m_BoxMax.Init();
		pSys->m_Center.Init();
		pSys->m_Radius = 1.0f;
		return;
	}

	pSys->m_BoxMin.Init(100000.0f, 100000.0f, 100000.0f);
	pSys->m_BoxMax = -pSys->m_BoxMin;

	for (pCur = pSys->m_ParticleTail.m_pNext; pCur != pTail;
	     pCur = pCur->m_pNext)
	{
		PS_EXTEND_BOUNDSQ45C(pSys, pCur);
	}

	ps_UpdateBoundsQ45C(pSys);
}


// ======================================================================= //
//
//  B.  THE PIXEL FORMAT CONVERTER  (0x0045d130, 0x0045d3b0)
//
//  Class views repeated from src/engine/lithtech/fmconvert.cpp with the
//  same Q-suffix discipline, so this scratch TU cannot collide at link
//  time.  Everything here was established there; nothing is re-derived.
//
//  THE TABLE THAT NAMES 0x0045d3b0.  fmconvert.cpp's header records that
//  0x0045f3d0 indexes 0x004acdb8 by `srcFormat->code * 8 + destFormat->code`
//  and calls `blit(pMgr, pRequest)`.  Reading the table out of .data:
//
//    0x004acdb8  0x0045d1e0  fm_BlitSame        src pal8 -> dest pal8
//    0x004acdbc  0x0045d3b0  <-- THIS ROW       src pal8 -> dest 8
//    0x004acdc0  0x0045d5e0  fm_BlitPal8To16    src pal8 -> dest 16
//    0x004acdc4  0x0045d7d0  fm_BlitPal8To32    src pal8 -> dest 32
//    0x004acddc  0x0045da30  fm_Blit8To8        src 8    -> dest 8
//    0x004acde0  0x0045dc90  fm_Blit8To16
//    0x004acde4  0x0045ddf0  fm_Blit8To32
//    0x004acdfc  0x0045df80  fm_Blit16To8   ... and so on, four-wide with
//                                               the same hole at dest 0
//
//  The column is fixed by three already-named neighbours, so the row is
//  fm_BlitPal8To8 and nothing else.  Its body is fm_BlitPal8To16 with a
//  uint8 destination cursor and Convert8 in place of Convert16, which is
//  exactly the difference fm_Blit8To8 makes against fm_Blit8To16.
//
// ======================================================================= //

class FMFormatQ45C
{
public:
	int IsSame(FMFormatQ45C* pOther);        // 0x0045f1e0

public:
	uint32 m_pVTable;                        // 0x00
	uint32 m_nCode;                          // 0x04
	uint32 m_Masks[NUM_COLORPLANES];         // 0x08
	uint32 m_nBits[NUM_COLORPLANES];         // 0x18
	uint32 m_nShifts[NUM_COLORPLANES];       // 0x28
};                                           // 0x38

class FMConvertMgrQ45C
{
public:
	FMFormatQ45C m_FormatPValue;             // 0x000
	FMFormatQ45C m_Format565;                // 0x038
	uint8        m_ToTables[9][256];         // 0x070
	uint8*       m_pFromTables[9];           // 0x970
	uint8        m_TableData[511];           // 0x994
};                                           // 0xb94

class FMConvertRequestQ45C
{
public:
	FMFormatQ45C* m_pSrcFormat;              // 0x00
	uint8*        m_pSrcData;                // 0x04
	int32         m_nSrcPitch;               // 0x08
	uint8*        m_pSrcPalette;             // 0x0c
	FMFormatQ45C* m_pDestFormat;             // 0x10
	uint8*        m_pDestData;               // 0x14
	int32         m_nDestPitch;              // 0x18
	uint32        m_nWidth;                  // 0x1c
	uint32        m_nHeight;                 // 0x20
	uint32        m_Flags;                   // 0x24
};

#define FMBLIT_ORQ45C   1

struct FMPaletteEntryQ45C
{
	uint8 m_a;                               // 0
	uint8 m_r;                               // 1
	uint8 m_g;                               // 2
	uint8 m_b;                               // 3
};

// fmconvert.cpp's two helper classes, declaration-for-declaration.  The
// bodies are NOT repeated here: Convert8 (0x0045d580) and Init
// (0x0045d530) are already reconstructed there, and this file must not
// emit second copies.  They are declared so the one new blitter below can
// be compiled and spliced into that TU, where the inline expansion at
// 0x0045d465..0x0045d4d5 will find Convert8's body.
class FMFormatConvertQ45C
{
public:
	// 0x0045d530, defined in-class so the DXT blit below gets the inline
	// expansion at 0x0045f771..0x0045f7b7 (the body is fmconvert.cpp's).
	void Init(FMConvertMgrQ45C* pMgr, FMFormatQ45C* pFormat)
	{
		m_pFormat = pFormat;

		m_pTables[CP_ALPHA] = pMgr->m_ToTables[pFormat->m_nBits[CP_ALPHA]];
		m_pTables[CP_RED]   = pMgr->m_ToTables[pFormat->m_nBits[CP_RED]];
		m_pTables[CP_GREEN] = pMgr->m_ToTables[pFormat->m_nBits[CP_GREEN]];
		m_pTables[CP_BLUE]  = pMgr->m_ToTables[pFormat->m_nBits[CP_BLUE]];
	}
	uint8  Convert8 (PValue* pValue);                            // 0x0045d580
	uint16 Convert16(PValue* pValue);                            // 0x0045d760
	PValue Convert32(PValue* pValue);                            // 0x0045d9c0

public:
	uint8*        m_pTables[NUM_COLORPLANES];   // 0x00
	FMFormatQ45C* m_pFormat;                    // 0x10
};

class FMPaletteReadQ45C
{
public:
	PValue ReadRGB (uint8* pSrc);            // 0x0045d990
	PValue ReadARGB(uint8* pSrc);            // 0x0045eb60

public:
	FMPaletteEntryQ45C* m_pPalette;          // 0x00
};


// ======================================================================= //
//  0x0045d130 .. 0x0045d1d0, 161 bytes  --  the sixteen-step ALPHA RAMP
//
//  WHAT IT IS.  Sixteen `movl $imm,<abs>` (ten bytes each) plus one `ret`
//  is 161 bytes exactly, and the sixteen destinations are the consecutive
//  dwords 0x004b40e8 .. 0x004b4124 holding
//
//      0x00000000 0x11000000 0x22000000 ... 0xff000000
//
//  i.e. PValue_Set(i * 0x11, 0, 0, 0) for i = 0..15.  0x0045f6a0 walks
//  exactly that range -- `mov $0x4b40e8,%esi` at 0x0045f7da, `add $0x4` at
//  0x0045f7ef, `cmp $0x4b4128` at 0x0045f7f5 -- and hands each element to
//  FMFormatConvert::Convert16, whose parameter is a PValue*.  So the array
//  is PValue[16] and the values are alpha-only.
//
//  WHY IT IS A RUNTIME INITIALISER AT ALL, which is the whole puzzle: a
//  PValue is `typedef uint32` (ltpvalue.h), so an aggregate of sixteen
//  integer constants would be laid down in .data with no code.  It is not;
//  it is a ??__E body registered at 0x004a70bc.  The one spelling in
//  ltpvalue.h that forces that is PValue_Set -- an `inline` FUNCTION CALL,
//  which C++ does not admit as a constant expression, so the initialiser
//  becomes dynamic and /O2 then folds each call to the immediate that the
//  bytes show.  That is what is written below.
//
//  The alternative that produces the same 161 bytes is a file-scope object
//  of a class with an in-class (hence inlinable) constructor that assigns
//  the sixteen elements in order; the global's address is folded into
//  every store either way, so the two are indistinguishable from the
//  bytes.  PValue_Set is preferred because it needs no class that nothing
//  else in the image references.
//
//  0x0045d120, the five-byte `jmp` the enumeration calls a thunk, is the
//  separate ??__E COMDAT and is not source-reachable -- see the file
//  header.
// ======================================================================= //
PValue g_AlphaPValuesQ45C[16] =
{
	PValue_Set(0x00, 0, 0, 0), PValue_Set(0x11, 0, 0, 0),
	PValue_Set(0x22, 0, 0, 0), PValue_Set(0x33, 0, 0, 0),
	PValue_Set(0x44, 0, 0, 0), PValue_Set(0x55, 0, 0, 0),
	PValue_Set(0x66, 0, 0, 0), PValue_Set(0x77, 0, 0, 0),
	PValue_Set(0x88, 0, 0, 0), PValue_Set(0x99, 0, 0, 0),
	PValue_Set(0xaa, 0, 0, 0), PValue_Set(0xbb, 0, 0, 0),
	PValue_Set(0xcc, 0, 0, 0), PValue_Set(0xdd, 0, 0, 0),
	PValue_Set(0xee, 0, 0, 0), PValue_Set(0xff, 0, 0, 0)
};


// ======================================================================= //
//  0x0045d3b0 .. 0x0045d529, 378 bytes  --  fm_BlitPal8To8
//
//  Entry 1 of the blitter table (see above).  Structurally it IS
//  fm_BlitPal8To16 with a uint8 destination:
//
//    * no source reader and no format guard -- only the destination
//      converter is built (0x0045d3ce), and the palette pointer is filed
//      into its one-word object at 0x0045d3ca, BEFORE that call, exactly
//      as at 0x0045d5fb/0x0045d5ff in the 16-bit sibling.
//    * 0x0045d3e6 `je 0x45d521` is the height-zero skip; 0x0045d400 and
//      0x0045d457 are the two arms' width-zero skips; 0x0045d511 is the
//      row latch.
//    * the OR arm CALLS Convert8 (0x0045d437) and the assign arm EXPANDS
//      it (0x0045d465..0x0045d4d5).  fmconvert.cpp's header calls this
//      "the inline lottery" and records it for all thirteen siblings: both
//      arms are one statement apart and nothing in the source steers /Ob2.
//    * the OR is a read-modify-write BYTE (`mov (%edi,%esi,1),%dl; or
//      %al,%dl; mov %dl,(%edi,%esi,1)` at 0x0045d43c) -- what a uint8 `|=`
//      needs and a uint16 one does not, the same tell fm_Blit16To8 has at
//      0x0045dffc.
//    * FMPaletteRead::ReadRGB is EXPANDED in both arms (0x0045d416 and
//      0x0045d477, the `%dh`/`%dl`/`%cl` triple straight out of the
//      palette with a four-byte stride), which is why fmconvert.cpp keeps
//      a copy of that body: VC6 cannot inline what the TU has not seen.
//
//  TWO EPILOGUES, both `xor %eax,%eax` (0x0045d517 and 0x0045d521), which
//  is the height-zero skip having its own copy -- the same shape all
//  thirteen have.
// ======================================================================= //
//  MOVED: the body now lives in src/engine/lithtech/fmconvert.cpp as
//  fm_BlitPal8To8, beside fm_BlitPal8To16, where Convert8Inline and
//  ReadRGBInline are visible -- byte-identical there.


// ======================================================================= //
//
//  C.  0x0045f6a0 .. 0x00460154, 2,741 bytes  --  THE DXT DECOMPRESSOR
//
//  THE EXTENT FIRST.  The enumeration says 2,401 bytes, which is the row
//  clipped at 0x00460000.  There is NO `ret` in that range and
//  four branch targets leaving it; 0x0046014e `add $0xcc,%esp` / 0x00460154
//  `ret` matches the 0x0045f6a0 `sub $0xcc,%esp` prologue and the four
//  pops, so the function is 0x0045f6a0..0x00460154 = 2,741 bytes.  A sweep
//  of every `e8`/`e9` displacement in .text finds exactly ONE entry --
//  `call 0x45f6a0` at 0x0045ed1e -- and nothing jumping into the interior
//  from outside, so it is ONE function.  The enumeration's 0x00460001,
//  0x00460018, 0x00460020, 0x00460067 and 0x0046006c `ptr` rows are all
//  interior addresses of it; the row does not end in a terminator.
//
//  WHAT IT IS.  A DXT1/DXT3/DXT5 (S3TC) decompressor writing 16-bit
//  destination pixels.  Nothing here is inferred from a name -- there are
//  none -- it is read off the arithmetic:
//
//    * 0x0045f813/0x0045f816 `shr $0x2` on m_nWidth and m_nHeight: 4x4
//      blocks.
//    * 0x0045f6c7 `cmp $0x5` and 0x0045f6e6 `cmp $0x6` on
//      m_pSrcFormat->m_nCode select the block size: code 5 or 6 gives
//      `shl $0x4` (16-byte blocks) with the colour half at +8
//      (0x0045f7c6/0x0045f7ce), anything else `shl $0x3` (8-byte blocks,
//      0x0045f801).  That is DXT3 / DXT5 / DXT1 exactly.
//    * 0x0045f875/0x0045f87c read two uint16s from the block and hand each
//      to FMFormatRead::Read16 initialised on the MANAGER'S 565 format
//      (0x0045f721 `lea 0x38(%eax)` is &pMgr->m_Format565) -- DXT stores
//      its two endpoints as R5G6B5.
//    * 0x0045f916 compares the two RAW words and picks between the
//      four-colour interpolation (magic multiplier 0xaaaaaaab + `shr $1`
//      is an unsigned divide by 3, six times, 0x0045f932..0x0045f9dd) and
//      the three-colour one ((a+b)/2 folded into `shl $0xf` / `shl $0x7` /
//      `shr $1` with `and $0x0,%ah` / `and $0x0,%al` cleaning up, plus a
//      transparent fourth entry, 0x0045fa0a).  That is the DXT1 colour
//      rule, and 0x0045f916 `jbe` is `color0 > color1`.
//    * 0x0045fab1..0x0045fbc8 is the 4x4 pixel write: sixteen
//      `aColors[(nBits >> 2k) & 3]` stores with `mov 0x18(%ebx),%edi; add
//      %edi,%eax` between rows.
//    * 0x0045ff69.. is DXT3's explicit alpha: sixteen NIBBLES, each a
//      table index, read-modify-written through `and %bp` where %bp is
//      ~destFormat->m_Masks[CP_ALPHA] (0x0045f72b `mov 0x8(%edx),%ebp`,
//      0x0045f738 `not %ebp`).
//    * 0x0045fc03.. is DXT5's interpolated alpha: two endpoint BYTES, the
//      `a0 > a1` split into six-of-seven and four-of-five ladders (signed
//      magic divides 0x92492493 = /7 and 0x66666667 = /5), and sixteen
//      THREE-bit indices spread over six bytes, with the tenth index
//      spliced across the dword boundary at 0x0045fefb.
//
//  0x00460160 (2,704 bytes, the next row) is the same function with a
//  32-bit destination -- 0x0045ed30 is its forwarder and is entry 3 of the
//  same table group at 0x004ace44.  Everything established here applies to
//  it; it is not an enumerated row here but it comes for free from this
//  source.
//
//  THE FOUR ARGUMENTS.  0x0045ed10 pushes four and cleans 0x10, but the
//  body reads only R+0x04 (pMgr, 0x0045f6c0) and R+0x08 (pRequest,
//  0x0045f6a7).  The third and fourth are pushed as literal zeros and are
//  never touched, so they are declared and named by position.
//
//  WHAT PINS PValue_Get RATHER THAN FOUR HAND-WRITTEN SHIFTS.  The frame
//  holds TWO four-dword groups at R-0x58..R-0x4c and R-0x48..R-0x3c, of
//  which the first dword of each is never written and the fourth of the
//  first stays in %edi -- eight out-parameters of which the two alphas are
//  dead.  0x0045f8af reads the spilled PValue's BYTE 2 for red and
//  0x0045f8c2 takes %ch for green, which is (v>>16)&0xff and (v>>8)&0xff
//  on a memory-homed dword.  That is ltpvalue.h's PValue_Get, four
//  references, with the alpha reference dead-stored away.
//
//  AND PValue_Set FOR THE PACKING: 0x0045f940/0x0045f952/0x0045f97d is a
//  `shl $0x8` CHAIN over (alpha, red, green, blue), which fmconvert.cpp's
//  header already identifies as what VC6 does to
//  `(a<<24)|(r<<16)|(g<<8)|b`.
//
//  ONE SHARED ARRAY, NOT TWO.  aAlpha[16] is filled at the top of the
//  function for every alpha format (0x0045f7d6..0x0045f7fb, sixteen
//  Convert16 calls over g_AlphaPValues) and the DXT5 path then OVERWRITES
//  its first eight entries per block (0x0045fd94..0x0045fe42).  Two
//  distinct locals could not be given overlapping slots with live ranges
//  that cross like that, so it is one array used both ways -- wasteful for
//  DXT5 and exactly what the bytes show.
//
//  HONEST CONFIDENCE.  This is the least certain body in the file.  The
//  control flow, the field offsets, the constants and the two SDK helpers
//  are all read off the bytes and cited; what is NOT pinned is (a) whether
//  the sixteen pixel stores and the eight alpha statements are written out
//  or come from a macro (the emitted code is the same either way, so this
//  costs nothing), (b) the operand order inside each `lea`-folded integer
//  sum, and (c) whether the block-offset expression is written as one
//  shift of a sum (as here) or as the two shifts VC6 emits.  Expect nears
//  rather than a first-shot match.
// ======================================================================= //

// 0x0045e0d0 / 0x0045d760 -- fmconvert.cpp's Read16 and Convert16, and
// both Init bodies, are EXPANDED INLINE here (0x0045f71b..0x0045f7b7 is
// the two Inits), so this body has to be compiled where fmconvert.cpp's
// definitions are visible.
class FMFormatReadQ45C
{
public:
	// 0x0045dbb0, in-class for the same reason (0x0045f71b..0x0045f770).
	void Init(FMConvertMgrQ45C* pMgr, FMFormatQ45C* pFormat)
	{
		m_pTables[CP_ALPHA] = pMgr->m_pFromTables[pFormat->m_nBits[CP_ALPHA]];
		m_pTables[CP_RED]   = pMgr->m_pFromTables[pFormat->m_nBits[CP_RED]];
		m_pTables[CP_GREEN] = pMgr->m_pFromTables[pFormat->m_nBits[CP_GREEN]];
		m_pTables[CP_BLUE]  = pMgr->m_pFromTables[pFormat->m_nBits[CP_BLUE]];

		m_pFormat = pFormat;
	}
	PValue Read8 (uint8*  pSrc);                                 // 0x0045dc00
	PValue Read16(uint16* pSrc);                                 // 0x0045e0d0
	PValue Read32(uint32* pSrc);                                 // 0x0045e490

public:
	FMFormatQ45C* m_pFormat;                    // 0x00
	uint8*        m_pTables[NUM_COLORPLANES];   // 0x04
};

// The two source format codes that carry an alpha block, numeric in the
// shipped code (0x0045f6c7, 0x0045f6e6).
#define FMCODE_DXT3Q45C   5
#define FMCODE_DXT5Q45C   6

LTRESULT fm_BlitDXTTo16Q45C(FMConvertMgrQ45C* pMgr, FMConvertRequestQ45C* pRequest,
                            uint32 nUnused1, uint32 nUnused2)
{
	FMFormatReadQ45C    read;
	FMFormatConvertQ45C convert;
	uint16              aColors[4];
	uint16              aAlpha[16];
	PValue              c0, c1, c2, c3;
	uint16              nColor0, nColor1;
	// The two endpoints' components as ARRAYS: PValue_Get writes all
	// four through references, and the frame keeps each group's dead
	// alpha slot (0x88 / 0x98, four-dword groups) -- scalars give a
	// 0xbc frame, these give the shipped 0xcc.  c0..c3 stay scalars here
	// (an array makes it 0xd0), unlike the 32-bit twin.
	uint32              aC0[4];
	uint32              aC1[4];
	uint32              nBlocksX, nBlocksY, xb, yb;
	uint32              nBlockShift, nColorOffset;
	uint32              nAlphaOr, nAlpha8, nAlphaShift;
	uint32              bHasAlphaBlock, bInterpAlpha;
	uint32              nAlphaMaskInv;
	uint8*              pAlphaToTable;
	uint32              nCode;

	// 0x0045f6b5..0x0045f717.  The defaults come first and the two format
	// codes overwrite them; the `else if` is 0x0045f6e6's second compare
	// on the fall-through of the first.
	nAlphaOr       = PVALUE_ALPHAMASK;
	nAlpha8        = 0xff;
	bInterpAlpha   = 0;
	bHasAlphaBlock = 0;

	// 0x0045f6bd loads the code ONCE into %ecx and 0x0045f6c7 / 0x0045f6e6
	// compare that register twice, across a basic-block boundary -- so the
	// source holds it in a local rather than reading the field twice.
	nCode = pRequest->m_pSrcFormat->m_nCode;

	if (nCode == FMCODE_DXT3Q45C)
	{
		bHasAlphaBlock = 1;
	}
	else if (nCode == FMCODE_DXT5Q45C)
	{
		bInterpAlpha   = 1;
		bHasAlphaBlock = 1;
		nAlphaOr       = 0;
		nAlpha8        = 0;

		nAlphaShift   = pRequest->m_pDestFormat->m_nShifts[CP_ALPHA];
		pAlphaToTable = pMgr->m_ToTables[pRequest->m_pDestFormat->m_nBits[CP_ALPHA]];
	}

	// 0x0045f71b..0x0045f7b7 -- both Inits expanded, the reader on the
	// manager's own 565 format and the writer on the request's.  The mask
	// comes FIRST: 0x0045f721 loads m_pDestFormat and 0x0045f72b/0x0045f738
	// invert its alpha mask in the middle of the reader's Init.
	nAlphaMaskInv = ~pRequest->m_pDestFormat->m_Masks[CP_ALPHA];
	read.Init(pMgr, &pMgr->m_Format565);
	convert.Init(pMgr, pRequest->m_pDestFormat);

	if (bHasAlphaBlock)
	{
		uint32 i;

		nBlockShift  = 4;
		nColorOffset = 8;

		for (i = 0; i < 16; i++)
			aAlpha[i] = convert.Convert16(&g_AlphaPValuesQ45C[i]);
	}
	else
	{
		nBlockShift  = 3;
		nColorOffset = 0;
	}

	nBlocksX = pRequest->m_nWidth  >> 2;
	nBlocksY = pRequest->m_nHeight >> 2;

	for (yb = 0; yb < nBlocksY; yb++)
	{
		for (xb = 0; xb < nBlocksX; xb++)
		{
			uint8*  pBlock;
			uint16* pDest;
			uint32  nBits;

			// The row base is `yb * nBlocksX`, strength-reduced by VC6 into
			// the accumulator 0x00460136 adds to and 0x0045f831 zeroes after
			// the height test (an explicit `nRowBase += nBlocksX` zeroes it
			// before); xb and the row are shifted SEPARATELY (0x0045f849 is
			// the hoisted row shift, xb's is kept at 0x10(%esp)).
			pBlock = pRequest->m_pSrcData + (xb << nBlockShift)
			         + ((yb * nBlocksX) << nBlockShift) + nColorOffset;

			nColor0 = ((uint16*)pBlock)[0];
			nColor1 = ((uint16*)pBlock)[1];

			c0 = read.Read16(&nColor0);
			c1 = read.Read16(&nColor1);

			PValue_Get(c0, aC0[0], aC0[1], aC0[2], aC0[3]);
			PValue_Get(c1, aC1[0], aC1[1], aC1[2], aC1[3]);

			c0 |= nAlphaOr;
			c1 |= nAlphaOr;

			if (nColor0 > nColor1)
			{
				c2 = PValue_Set(nAlpha8, (aC0[1] * 2 + aC1[1]) / 3,
				                         (aC0[2] * 2 + aC1[2]) / 3,
				                         (aC0[3] * 2 + aC1[3]) / 3);
				c3 = PValue_Set(nAlpha8, (aC0[1] + aC1[1] * 2) / 3,
				                         (aC0[2] + aC1[2] * 2) / 3,
				                         (aC0[3] + aC1[3] * 2) / 3);
			}
			else
			{
				c3 = 0;
				c2 = PValue_Set(nAlpha8, (aC0[1] + aC1[1]) / 2,
				                         (aC0[2] + aC1[2]) / 2,
				                         (aC0[3] + aC1[3]) / 2);
			}

			aColors[0] = convert.Convert16(&c0);
			aColors[1] = convert.Convert16(&c1);
			aColors[2] = convert.Convert16(&c2);
			aColors[3] = convert.Convert16(&c3);

			// 0x0045fa8e reads the index word and 0x0045fa96..0x0045fab7
			// only then builds the destination pointer, so both come
			// AFTER the four Convert16 calls in the source.
			nBits = *(uint32*)(pBlock + 4);

			// `yb << 2`, not `yb * 4`: the multiply lets VC6 factor the
			// four out into `lea (base,sum,4)`; the shift keeps yb*4
			// formed before the imul, as shipped (0x0045faa8 / 0x0045fab1;
			// ours spells the *4 `shl` where shipped has `lea`).
			pDest = (uint16*)(pRequest->m_pDestData
			                  + (yb << 2) * pRequest->m_nDestPitch + xb * 8);

			pDest[0] = aColors[ nBits        & 3];
			pDest[1] = aColors[(nBits >>  2) & 3];
			pDest[2] = aColors[(nBits >>  4) & 3];
			pDest[3] = aColors[(nBits >>  6) & 3];
			pDest = (uint16*)((uint8*)pDest + pRequest->m_nDestPitch);

			pDest[0] = aColors[(nBits >>  8) & 3];
			pDest[1] = aColors[(nBits >> 10) & 3];
			pDest[2] = aColors[(nBits >> 12) & 3];
			pDest[3] = aColors[(nBits >> 14) & 3];
			pDest = (uint16*)((uint8*)pDest + pRequest->m_nDestPitch);

			pDest[0] = aColors[(nBits >> 16) & 3];
			pDest[1] = aColors[(nBits >> 18) & 3];
			pDest[2] = aColors[(nBits >> 20) & 3];
			pDest[3] = aColors[(nBits >> 22) & 3];
			pDest = (uint16*)((uint8*)pDest + pRequest->m_nDestPitch);

			pDest[0] = aColors[(nBits >> 24) & 3];
			pDest[1] = aColors[(nBits >> 26) & 3];
			pDest[2] = aColors[(nBits >> 28) & 3];
			pDest[3] = aColors[(nBits >> 30) & 3];

			// 0x0045fbcc..0x0045fbfd -- the alpha half re-derives BOTH
			// pointers from scratch rather than rewinding the ones above.
			if (bHasAlphaBlock)
			{
				uint8*  pABlock;
				uint16* pADest;

				pADest = (uint16*)(pRequest->m_pDestData
				                   + (yb << 2) * pRequest->m_nDestPitch + xb * 8);

				pABlock = pRequest->m_pSrcData + (xb << nBlockShift)
				          + ((yb * nBlocksX) << nBlockShift);

				if (bInterpAlpha)
				{
					// SIGNED: the divides at 0x0045fc42 and 0x0045fd36 are
					// `imul` + `sar` + `shr $0x1f` + `add`, which is VC6's
					// signed magic divide by 7 and by 5.  An unsigned pair
					// would be `mul` + `shr` with no sign fixup, as the
					// colour side's /3 at 0x0045f937 is.
					int    nA, nB;
					uint32 nLo, nHi;

					// 0x0045fc03 -- both endpoints are zero-extended BYTES
					// (`movzbw`) homed into the first two slots of aAlpha.
					aAlpha[0] = pABlock[0];
					aAlpha[1] = pABlock[1];

					nA = aAlpha[0];
					nB = aAlpha[1];

					if (aAlpha[0] > aAlpha[1])
					{
						aAlpha[2] = (uint16)((nA * 6 + nB    ) / 7);
						aAlpha[3] = (uint16)((nA * 5 + nB * 2) / 7);
						aAlpha[4] = (uint16)((nA * 4 + nB * 3) / 7);
						aAlpha[5] = (uint16)((nA * 3 + nB * 4) / 7);
						aAlpha[6] = (uint16)((nA * 2 + nB * 5) / 7);
						aAlpha[7] = (uint16)((nA     + nB * 6) / 7);
					}
					else
					{
						// 0x0045fd25/0x0045fd2c store the two endstops
						// BEFORE the four interpolants.
						aAlpha[6] = 0;
						aAlpha[7] = 255;

						aAlpha[2] = (uint16)((nA * 4 + nB    ) / 5);
						aAlpha[3] = (uint16)((nA * 3 + nB * 2) / 5);
						aAlpha[4] = (uint16)((nA * 2 + nB * 3) / 5);
						aAlpha[5] = (uint16)((nA     + nB * 4) / 5);
					}

					// 0x0045fd94..0x0045fe42 -- each of the eight in place.
					aAlpha[0] = (uint16)(pAlphaToTable[nA]        << nAlphaShift);
					aAlpha[1] = (uint16)(pAlphaToTable[nB]        << nAlphaShift);
					aAlpha[2] = (uint16)(pAlphaToTable[aAlpha[2]] << nAlphaShift);
					aAlpha[3] = (uint16)(pAlphaToTable[aAlpha[3]] << nAlphaShift);
					aAlpha[4] = (uint16)(pAlphaToTable[aAlpha[4]] << nAlphaShift);
					aAlpha[5] = (uint16)(pAlphaToTable[aAlpha[5]] << nAlphaShift);
					aAlpha[6] = (uint16)(pAlphaToTable[aAlpha[6]] << nAlphaShift);
					aAlpha[7] = (uint16)(pAlphaToTable[aAlpha[7]] << nAlphaShift);

					nHi = *(uint16*)(pABlock + 6);
					nLo = *(uint32*)(pABlock + 2);

					pADest[0] |= aAlpha[ nLo        & 7];
					pADest[1] |= aAlpha[(nLo >>  3) & 7];
					pADest[2] |= aAlpha[(nLo >>  6) & 7];
					pADest[3] |= aAlpha[(nLo >>  9) & 7];
					pADest = (uint16*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] |= aAlpha[(nLo >> 12) & 7];
					pADest[1] |= aAlpha[(nLo >> 15) & 7];
					pADest[2] |= aAlpha[(nLo >> 18) & 7];
					pADest[3] |= aAlpha[(nLo >> 21) & 7];
					pADest = (uint16*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] |= aAlpha[(nLo >> 24) & 7];
					pADest[1] |= aAlpha[(nLo >> 27) & 7];
					// 0x0045fefb -- index ten straddles the dword.
					pADest[2] |= aAlpha[(nLo >> 30) | ((nHi & 1) << 2)];
					pADest[3] |= aAlpha[(nHi >>  1) & 7];
					pADest = (uint16*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] |= aAlpha[(nHi >>  4) & 7];
					pADest[1] |= aAlpha[(nHi >>  7) & 7];
					pADest[2] |= aAlpha[(nHi >> 10) & 7];
					pADest[3] |= aAlpha[(nHi >> 13) & 7];
				}
				else
				{
					// 0x0045ff69 -- DXT3.  The colour pass already put an
					// opaque alpha in, so each pixel is masked before the
					// nibble goes in and stored with `mov`, not `or`.
					uint32 nLo = *(uint32*)pABlock;
					uint32 nHi;

					pADest[0] = (uint16)((pADest[0] & nAlphaMaskInv) | aAlpha[ nLo        & 0xf]);
					pADest[1] = (uint16)((pADest[1] & nAlphaMaskInv) | aAlpha[(nLo >>  4) & 0xf]);
					pADest[2] = (uint16)((pADest[2] & nAlphaMaskInv) | aAlpha[(nLo >>  8) & 0xf]);
					pADest[3] = (uint16)((pADest[3] & nAlphaMaskInv) | aAlpha[(nLo >> 12) & 0xf]);
					pADest = (uint16*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] = (uint16)((pADest[0] & nAlphaMaskInv) | aAlpha[(nLo >> 16) & 0xf]);
					pADest[1] = (uint16)((pADest[1] & nAlphaMaskInv) | aAlpha[(nLo >> 20) & 0xf]);
					pADest[2] = (uint16)((pADest[2] & nAlphaMaskInv) | aAlpha[(nLo >> 24) & 0xf]);
					pADest[3] = (uint16)((pADest[3] & nAlphaMaskInv) | aAlpha[(nLo >> 28) & 0xf]);
					pADest = (uint16*)((uint8*)pADest + pRequest->m_nDestPitch);

					nHi = *(uint32*)(pABlock + 4);

					pADest[0] = (uint16)((pADest[0] & nAlphaMaskInv) | aAlpha[ nHi        & 0xf]);
					pADest[1] = (uint16)((pADest[1] & nAlphaMaskInv) | aAlpha[(nHi >>  4) & 0xf]);
					pADest[2] = (uint16)((pADest[2] & nAlphaMaskInv) | aAlpha[(nHi >>  8) & 0xf]);
					pADest[3] = (uint16)((pADest[3] & nAlphaMaskInv) | aAlpha[(nHi >> 12) & 0xf]);
					pADest = (uint16*)((uint8*)pADest + pRequest->m_nDestPitch);

					pADest[0] = (uint16)((pADest[0] & nAlphaMaskInv) | aAlpha[(nHi >> 16) & 0xf]);
					pADest[1] = (uint16)((pADest[1] & nAlphaMaskInv) | aAlpha[(nHi >> 20) & 0xf]);
					pADest[2] = (uint16)((pADest[2] & nAlphaMaskInv) | aAlpha[(nHi >> 24) & 0xf]);
					pADest[3] = (uint16)((pADest[3] & nAlphaMaskInv) | aAlpha[(nHi >> 28) & 0xf]);
				}
			}
		}

		// 0x00460136 `add %eax,%edi` -- the row's block base is
		// ACCUMULATED: VC6's strength reduction of `yb * nBlocksX` above.
		// Measured: an explicit `nRowBase += nBlocksX` zeroes the base
		// BEFORE the height test (0x0045f831 stores it after), so the
		// multiply is the spelling.
	}

	return LT_OK;
}


// 0x0045ed10, 23 bytes -- the blitter-table entry for the 16-bit DXT blit
// (entry 2 of the group at 0x004ace40; name ours).  The table's blitter
// signature is (pMgr, pRequest), so this forwards the two it is given plus
// two literal zeros and cleans 0x10 after the call.  Byte-for-byte the
// 32-bit forwarder at 0x0045ed30 (fmcvt32.cpp) bar the call target, and
// the same shape as filemgr.cpp's fs_OpenFile (0x0046b7a0), which
// matched here while this was unwritten.
LTRESULT fm_BlitDXTTo16EntryQ45C(FMConvertMgrQ45C* pMgr, FMConvertRequestQ45C* pRequest)
{
	return fm_BlitDXTTo16Q45C(pMgr, pRequest, 0, 0);
}

// ======================================================================= //
//  END OF FILE
// ======================================================================= //
