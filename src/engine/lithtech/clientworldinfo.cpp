// *********************************************************************** //
//
//  MODULE:   RENDER3D (.CPP)  -- scratch file, NOT a drop-in TU
//
//  RANGE:    lithtech.exe 0x00408cf0..0x00409400
//            "the client's 3D/2D drawing wrappers"
//
//  HOME:     everything here belongs in src/engine/lithtech/clientinterface.cpp,
//            which ALREADY contains ten of the eleven functions in the range.
//            Exactly ONE function body in this file is new work:
//            ci_GetWorldInfoString (0x004093d0). Everything else below this
//            line is findings, not code.
//
// ======================================================================= //
//  1. WHAT IS ACTUALLY IN THE RANGE  (ret-verified extents, not enum sizes)
// ======================================================================= //
//
//    addr        end      bytes  function                     status
//    0x00408cf0  0x408e0e   287  ci_ClearScreen               identical
//    0x00408e10  0x408e9a   139  ci_Start3D                   identical
//    0x00408ea0  0x408f82   227  ci_RenderCamera              identical
//    0x00408f90  0x4090bc   301  ci_RenderObjects             identical
//    0x004090c0  0x40914a   139  ci_StartOptimized2D          identical
//    0x00409150  0x4091da   139  ci_EndOptimized2D            identical
//    0x004091e0  0x409275   150  ci_SetOptimized2DBlend       identical
//    0x00409280  0x409315   150  ci_SetOptimized2DColor       written, see (3)
//    0x00409320  0x4093aa   139  ci_End3D                     identical
//    0x004093b0  0x4093ca    27  ci_GetFileList               identical
//    0x004093d0  0x4094af   224  ci_GetWorldInfoString        NEW -- below
//
//  Eleven functions, ten already sourced. The whole run is one contiguous
//  slab of clientinterface.obj, and the shipped .rdata proves the source
//  order: the function-name literals lie at 0x004a7d50 in exactly the .text
//  order above (ClearScreen, "74", Start3D, "75", RenderCamera,
//  RenderObjects, StartOptimized2D, EndOptimized2D, SetOptimized2DBlend,
//  SetOptimized2DColor, End3D, "42", GetWorldInfoString, GetSoundDuration).
//  So GetWorldInfoString is defined immediately after End3D/GetFileList and
//  immediately before ci_GetSoundDuration in the original file, and "42"
//  (LT_INVALIDWORLDFILE) occurs nowhere earlier in the TU.
//
// ======================================================================= //
//  2. THE ENUMERATED ROWS ARE ALL MID-FUNCTION -- none is a function
// ======================================================================= //
//
//  All four enumerated addresses are `jne`/`je` targets INSIDE the
//  functions above, i.e. the success arm after the two guard clauses. Each
//  containing function is already byte-identical:
//
//    0x00408f22  -> the body of ci_RenderCamera        (entry 0x00408ea0)
//                   reached by `jne 0x408f22` at 0x00408ee8
//    0x00409012  -> the body of ci_RenderObjects       (entry 0x00408f90)
//                   reached by `jne 0x409012` at 0x00408fd8
//    0x00409262  -> the tail of ci_SetOptimized2DBlend (entry 0x004091e0)
//                   reached by `jne 0x409262` at 0x00409228
//    0x00409302  -> the tail of ci_SetOptimized2DColor (entry 0x00409280)
//                   reached by `jne 0x409302` at 0x004092c8
//
//  Nothing to write.
//
// ======================================================================= //
//  3. A REAL HOLE: 0x00409280 is missing from the enumeration
// ======================================================================= //
//
//  The function enumeration has NO row at 0x00409280, so
//  ci_SetOptimized2DColor cannot be credited even though we have it.
//  It has a bogus 20-byte row at 0x00409302 (the tail) instead.
//
//  Why the start is missed: these interface functions are only ever
//  referenced by `movl $addr,N(%eax)` inside ci_SetupInterface. The enumerator
//  collects call targets, data pointers into .text, and `push imm32` --
//  but not a `mov imm32 -> memory`. Every function in this range that is
//  ALSO reached some other way got enumerated; 0x00409280 is not.
//
//  It is not lost work, it is unclaimable work: the byte match reports
//    MATCH 0x4091e0+   ?ci_SetOptimized2DColor@@YAKK@Z (150)
//  -- the `+` meaning it hit at more than one address. ci_SetOptimized2DBlend
//  and ci_SetOptimized2DColor are RELOCATION-MASKED TWINS: 150 bytes each,
//  identical apart from three relocation words (the name literal, the code
//  literal "75", and the render-table slot). A greedy assignment
//  gives our SetOptimized2DColor symbol the 0x004091e0 slot and there is no
//  0x00409280 row for the other. Adding a 0x00409280,150 row to
//  the enumeration would be worth one function and 150 bytes.
//
// ======================================================================= //
//  4. RENDER-SLOT FINDINGS  -- what the client passes, what it does with
//     the answer. RenderStruct base = 0x004b4358; slot = global - base.
// ======================================================================= //
//
//  The slot->d3d.ren map is re-derived from RenderDLLSetup (0x10021410),
//  then lithtech.exe's .text swept for every absolute reference to each
//  slot global. The sweep is the useful half: it says how many call sites a
//  slot has and where they are.
//
//   slot  client global  d3d.ren     call sites in lithtech.exe .text
//   ----  -------------  ----------  --------------------------------
//   +0x6c 0x004b43c4     0x100215a0  1  (0x00462521)
//   +0x70 0x004b43c8     0x100211d0  1  (0x00462874)
//   +0x74 0x004b43cc     0x10039fe0  1  (0x004628f5)
//   +0x78 0x004b43d0     0x1003aac0  1  (0x00462911)
//   +0x7c 0x004b43d4     0x10033160  1  (0x00414883)  CreateWorld
//   +0x80 0x004b43d8     0x10032c00  1  (0x004148b1)  DestroyWorld
//   +0x84 0x004b43dc     0x100262f0  1  (0x00408e01)  ClearScreen
//   +0x88 0x004b43e0     0x10032e50  1  (0x00408e94)  Start3D
//   +0x8c 0x004b43e4     0x10032ea0  1  (0x004093a4)  End3D
//   +0x90 0x004b43e8     0x10032f20  8  ALL of them in this range
//   +0x94 0x004b43ec     0x10034930  1  (0x00409144)  StartOptimized2D
//   +0x98 0x004b43f0     0x10034b00  1  (0x004091d4)  EndOptimized2D
//   +0x9c 0x004b43f4     0x10034c60  0  NEVER CALLED
//   +0xa0 0x004b43f8     0x10034c70  1  (0x00409269)  SetOptimized2DBlend *NEW*
//   +0xa4 0x004b43fc     0x10034ed0  0  NEVER CALLED  GetOptimized2DBlend *NEW*
//   +0xa8 0x004b4400     0x10034eb0  1  (0x00409309)  SetOptimized2DColor
//   +0xac 0x004b4404     0x10034ef0  0  NEVER CALLED  GetOptimized2DColor *NEW*
//   +0xb0 0x004b4408     0x1002d1c0  1  (0x004119f2)
//   +0xb4 0x004b440c     0x10032d00  1  (0x0041de95)
//   +0xb8 0x004b4410     0x10020fd0  5  (0x00401dff,0x00401e33,0x004022f2,
//                                        0x0048adcf,0x0048b453)
//   +0xbc 0x004b4414     0x100371e0  1  (0x0041094b)
//   +0xc0..+0xe8                     0  no absolute reference
//   +0xec 0x004b4444     0x10036cf0  4  (0x0040d385,0x0040d3d3,0x0040ddf7,
//                                        0x0040de46)
//   +0xf0 0x004b4448     0x10036da0  2  (0x0040e70c,0x0040e8d9)
//   +0xf4 0x004b444c     0x10036e40  1  (0x00403973)
//   +0xf8 0x004b4450     0x10021e50  1  (0x0041054f)
//   +0xfc 0x004b4454     0x10036ae0  1  (0x0040d3f5)
//
//  The base 0x004b4358 itself is referenced from 0x00403913, 0x004105ea,
//  0x0041de81, 0x00462130, 0x0046213d, 0x004624f2 and 0x00462549 -- so the
//  +0xc0..+0xe8 block is reached through a base register, not absolutely,
//  and whoever works those addresses can name eleven more d3d.ren functions.
//
//  --- THREE NEW SLOT NAMES, and the proof for each ---------------------
//
//  +0xa0 = d3d.ren 0x10034c70 = SetOptimized2DBlend(LTSurfaceBlend blend)
//      Client side: 0x00409262 does `push arg1; call *0x4b43f8; add $4`.
//      cdecl, one 4-byte argument, passed through UNVALIDATED (the client
//      checks only g_RenderStruct.m_bInited and IsIn3D before it).
//      d3d.ren side: 0x10034c70 stores the argument into global 0x1008d9a4
//      and then runs a cmp cascade against 0, 1, 2, 3, 4, 5, 6 -- exactly
//      the seven members of LTSurfaceBlend (ltbasedefs.h:348, ALPHA, SOLID,
//      ADD, MULTIPLY, MULTIPLY2, MASK, MASKADD). Anything else falls to
//      `xor %eax,%eax; ret`. That 0..6 cascade IS the identification.
//
//  +0xa4 = d3d.ren 0x10034ed0 = GetOptimized2DBlend(LTSurfaceBlend& blend)
//      Reads the same global 0x1008d9a4 that +0xa0 writes and stores it
//      through its one pointer argument; returns 1. Nine instructions.
//
//  +0xac = d3d.ren 0x10034ef0 = GetOptimized2DColor(HLTCOLOR& hColor)
//      Reads the global 0x10089448 that +0xa8 writes and stores it through
//      its one pointer argument; returns 1. Same nine-instruction shape.
//
//      Both getters are dead in the shipped engine: ci_SetupInterface never
//      assigns CLTClient::GetOptimized2DBlend / GetOptimized2DColor (see
//      clientinterface.h:61 and :63, "not set by the shipped engine"), and
//      the .text sweep above finds zero references to 0x004b43fc and
//      0x004b4404. d3d.ren implements them, lithtech.exe never wires them.
//
//  +0x9c = d3d.ren 0x10034c60 -- `mov 0x1008d7c4,%eax; ret`. A no-argument
//      getter returning one global. Never called from lithtech.exe, and its
//      position (between EndOptimized2D and SetOptimized2DBlend) has no
//      counterpart in ILTClient's declaration order, so it is NOT named.
//
//  --- WHAT THE CLIENT PASSES AND DOES WITH THE ANSWER ------------------
//
//  +0x84 ClearScreen(LTRect*, uint32 flags), cdecl, ANSWER IGNORED.
//      ci_ClearScreen never forwards the caller's rect. It builds a fresh
//      four-int rect on its own stack and passes THAT:
//        * pClearRect == NULL  -> { 0, 0, g_RenderStruct.m_Width, g_RenderStruct.m_Height }
//          (0x004b4394 / 0x004b4398, which RenderDLLSetup's caller copied
//          out of RenderStruct+0x204/+0x208 at 0x004624ff)
//        * otherwise -> each of left/right clamped to [0, g_RenderStruct.m_Width]
//          and each of top/bottom clamped to [0, g_RenderStruct.m_Height],
//          member-by-member, with no left<=right ordering fixup.
//      flags gets bit 0 FORCED ON when g_bAlwaysClearScreen (0x004b328c) is
//      nonzero, so the renderer never sees a clear request with the colour
//      bit off while that console variable is set. The renderer's return
//      value is discarded (`xor %eax,%eax` -> LT_OK unconditionally).
//      NOTE: ClearScreen is the ONE function in this block that does not
//      consult IsIn3D. It is legal outside a 3D block.
//
//  +0x88 Start3D() / +0x8c End3D() / +0x94 StartOptimized2D() /
//  +0x98 EndOptimized2D(): all four are no-argument cdecl calls whose
//      answers are discarded -- the wrapper falls straight into
//      `xor %eax,%eax; ret`. So in d3d.ren these four are effectively void.
//
//  +0x90 IsIn3D(), no arguments, EIGHT call sites, every one of them a
//      guard and every one of them testing only zero/nonzero:
//        0x00408e52  ci_Start3D              nonzero -> LT_ALREADYIN3D (74)
//        0x00408ee2  ci_RenderCamera         zero    -> LT_NOTIN3D     (75)
//        0x00408fd2  ci_RenderObjects        zero    -> LT_NOTIN3D
//        0x00409102  ci_StartOptimized2D     zero    -> LT_NOTIN3D
//        0x00409192  ci_EndOptimized2D       zero    -> LT_NOTIN3D
//        0x00409222  ci_SetOptimized2DBlend  zero    -> LT_NOTIN3D
//        0x004092c2  ci_SetOptimized2DColor  zero    -> LT_NOTIN3D
//        0x00409362  ci_End3D                zero    -> LT_NOTIN3D
//      Start3D is the only site with the inverted sense. Optimized-2D is
//      therefore NOT a separate mode in the client's view: it is nested
//      inside a 3D block, and the engine has no IsInOptimized2D() to pair
//      with it -- the 2D bracket is unchecked in both directions.
//
//  +0xa0 SetOptimized2DBlend / +0xa8 SetOptimized2DColor: one 4-byte
//      argument each, forwarded verbatim, and their ANSWERS ARE USED --
//      the only two slots in this block whose result is read. Both wrappers
//      end in `neg %eax; sbb %eax,%eax; inc %eax`, which is
//      `return !answer;` on an LTBOOL: nonzero from the renderer becomes
//      LT_OK (0), zero becomes 1. So the d3d.ren signature is
//      "LTBOOL succeeded", and for SetOptimized2DBlend the failure case is
//      reachable -- an out-of-range LTSurfaceBlend returns 0 there and the
//      client turns it into 1. SetOptimized2DColor cannot fail (it ends
//      `xor %eax,%eax; inc %eax` on every path), and it does not store the
//      colour the caller gave: it masks to 0x00ffffff and ORs in an alpha
//      of 0xff before parking it in 0x10089448.
//
//  Suggested (NOT applied) addition to src/engine/lithtech/clientmgr.h,
//  next to the g_pRender* block at line 212ff:
//
//      extern LTBOOL (*g_pRenderSetOptimized2DBlend)(LTSurfaceBlend blend);
//                                                      // 0x004b43f8
//
//  ci_SetOptimized2DBlend already compiles byte-identical without it (it is
//  a relocation), so this is documentation, not a fix.
//
// ======================================================================= //
//  5. THE ONE NEW FUNCTION
// ======================================================================= //

#include <stdio.h>
#include <stdarg.h>
#include <io.h>
#include <string.h>

#include "clientinterface.h"
#include "clientmgr.h"
#include "de_error.h"
#include "iltstream.h"


// 0x004048c0 -- open one CClientFileRef and hand back a stream, or LTNULL.
// Not fs_OpenFile: it is client-only and it reads all three members of the
// reference. `cmpl $0x2,(%edi)` at 0x004048cf takes the m_nLocalID path
// (0x004048d4 loads the uint16 at +0x08 and calls 0x00404410); every other
// type walks a circular list from pFileMgr+0x04 comparing m_pFilename
// through 0x00420ec0, and type 0 additionally retries against pFileMgr+0x6c.
// Returns whatever 0x00420ec0 gives, i.e. a stream, or zero on every miss.
// Named after its address because the bytes do not give it one.
ILTStream* sub_4048c0(CFileMgr* pFileMgr, CClientFileRef* pRef);

// 0x00421be0 -- the 138-byte wrapper round w_ReadWorldHeader (0x00421b20)
// that clienttexmgr.cpp already documents as living in commonmodel.obj's
// run. It reads the header, then reads the info string out of the stream in
// place, truncating to maxLen-1 and NUL-terminating, and reports the length
// through pActualLen when that is non-null. Answers 0 on success and
// LT_INVALIDWORLDFILE (0x2a) when the header will not read.
LTRESULT sub_421be0(ILTStream* pStream, char* pInfoString, uint32 maxLen,
                    uint32* pActualLen);


// 0x004093d0 (224 bytes).
//
// PINNED BY: the two error literals it pushes -- "GetWorldInfoString" at
// 0x004a7de4 with code string "61" (0x004a7a4c, LT_NOTFOUND, pushed as
// $0x3d at 0x0040940b) and "42" (0x004a7de0, LT_INVALIDWORLDFILE, pushed as
// $0x2a at 0x00409475). Both sites push the name as an immediate, so both
// are plain RETURN_ERROR with a string literal, not the `static char*` form.
//
// The frame is `sub $0xc` and nothing else: a bare CClientFileRef, whose
// address goes to 0x004048c0. The stream lives in %esi across the call and
// the result of 0x00421be0 in %edi, because Release() has to run BEFORE the
// result is tested -- 0x0040945d..0x00409466 is
// `mov (%esi),%edx; mov %esi,%ecx; mov %eax,%edi; call *0x4(%edx)`, and
// slot +0x04 of an ILTStream vtable is Release (slot 0 is the virtual
// destructor declared first in iltstream.h).
//
// The reference is left at m_nFileType 0 -- ci_SetPolyGridTexture and
// ci_CreateObject both set 1 explicitly, this one does not -- which is the
// type 0x004048c0 handles with the pFileMgr+0x6c fallback.
//
// CONFIDENCE: high on control flow, structure and constants; MEDIUM on the
// three constructor stores. Shipped emits them m_nLocalID (movw, +0x08),
// m_pFilename (+0x04), m_nFileType (movl $0, +0x00) -- reverse declaration
// order for the two zeros. ci_SetPolyGridTexture (0x0040b970), the other
// CClientFileRef site, emits its ctor in FORWARD order (+0x00, +0x04, +0x08)
// because its assignments are past a branch and nothing is dead-stored
// there. Here the assignment is in the same basic block, so the ctor's
// `m_pFilename = LTNULL` is dead-eliminated and VC6 rescheduled the two
// survivors around the g_pClientMgr load. If this comes out size-exact with
// the three stores permuted, that is the whole difference and it is a
// scheduling artefact, not a layout fault.
LTRESULT ci_GetWorldInfoString(char* pFilename, char* pInfoString, uint32 maxLen,
                               uint32* pActualLen)
{
	CClientFileRef ref;
	ILTStream*     pStream;
	LTRESULT       dResult;

	// A REDUNDANT ZERO, and it is load-bearing. Shipped's surviving stores
	// are +0x08, +0x04, +0x00 in that order; writing m_nFileType AFTER
	// m_pFilename makes the constructor's own +0x00 zero dead too, which is
	// what leaves that order. cltrest.cpp already names this spelling for
	// the same class.
	ref.m_pFilename = pFilename;
	ref.m_nFileType = 0;

	pStream = sub_4048c0(g_pClientMgr->m_pFileMgr, &ref);
	if (!pStream)
		RETURN_ERROR(1, "GetWorldInfoString", LT_NOTFOUND);

	dResult = sub_421be0(pStream, pInfoString, maxLen, pActualLen);
	pStream->Release();

	// The success answer is hosted INLINE and the error arm sits at the end:
	// shipped is `jne <error>` falling through to `xor %eax,%eax; ret`.
	// Written as `if (dResult) RETURN_ERROR(...); return LT_OK;` the arms
	// come out the other way round.
	if (dResult == LT_OK)
		return LT_OK;

	RETURN_ERROR(1, "GetWorldInfoString", LT_INVALIDWORLDFILE);
}


// ======================================================================= //
//  6. FOUR MORE clientinterface.obj FUNCTIONS THAT HAD NO BODY
// ======================================================================= //
//
//  HOME: src/engine/lithtech/clientinterface.cpp for all four (their
//  shipped neighbours are clientinterface.obj's: ci_EndOptimized2D
//  0x00409150 / ci_SetOptimized2DColor 0x00409280, and ci_GetParticlePos
//  0x0040b6d0 / ci_SetupPolyGrid 0x0040b940 / ci_SetPolyGridTexture
//  0x0040b970 / ci_SetPolyGridTextureInfo 0x0040bc00). Written here because
//  this is the file this worker owns; move them when clientinterface.cpp is
//  free. The layouts they need are declared locally, as views, so that no
//  header had to change -- they belong in clientmgr.h beside
//  CClientParticleSystem and CClientPolyGrid.
//
//  Section (1) above was wrong on one point: ci_SetOptimized2DBlend was NOT
//  sourced. The "identical" credited at 0x004091e0 was
//  ci_SetOptimized2DColor's bytes, which are relocation-masked twins of
//  Blend's, claimed at the first hit -- leaving 0x00409280, Color's real
//  home, with nothing to claim it.

// RenderStruct +0xa0 (d3d.ren 0x10034c70), the slot section (4) names.  It
// is a field of the one render table object, clientrender.cpp's
// g_RenderStruct (0x004b4358), not a global of its own, so it is reached
// through that definition (the absolute address is the same `sym+disp`).
extern RenderStruct g_RenderStruct;                                  // 0x004b4358
struct RenderStructBlendViewQ41
{
	uint8  m_pad_000[0xa0];
	LTBOOL (*SetOptimized2DBlend)(LTSurfaceBlend blend);             // 0x0a0  0x004b43f8
};
#define g_pRenderSetOptimized2DBlend \
	(((RenderStructBlendViewQ41*)&g_RenderStruct)->SetOptimized2DBlend)

// clientrender.cpp's renderer table (0x004b4358), through clientmgr.h's
// view: IsIn3D is its +0x90 (0x004b43e8).
extern RenderStruct g_RenderStruct;                                  // 0x004b4358

// The particle system's bounding box and change counter, which SetParticlePos
// keeps up to date: a min corner at 0x230 and a max corner at 0x23c, and a
// count at 0x22c bumped on every move.
class CClientParticleSystemBox
{
public:
	uint8    m_pad_00[0x22c];
	uint32   m_nChangedParticles;             // 0x22c
	LTVector m_MinPos;                        // 0x230
	LTVector m_MaxPos;                        // 0x23c
};

// The poly grid's environment map, a texture resolved from a file reference
// at 0x1c8.
class CClientPolyGridEnv
{
public:
	uint8    m_pad_00[0x1c8];
	void*    m_pEnvMap;                       // 0x1c8
};

void* sub_41ff90(CClientMgr* pClientMgr, CClientFileRef* pRef);    // 0x0041ff90


// 0x004091e0 (150). Relocation-masked twin of ci_SetOptimized2DColor.
LTRESULT ci_SetOptimized2DBlend(LTSurfaceBlend blend)
{
	if (!g_RenderStruct.m_bInited)
		RETURN_ERROR(1, "SetOptimized2DBlend", LT_NOTINITIALIZED);

	if (!g_RenderStruct.IsIn3D())
		RETURN_ERROR(1, "SetOptimized2DBlend", LT_NOTIN3D);

	return !g_pRenderSetOptimized2DBlend(blend);
}


// 0x0040b7a0 (214). The system handle is null-checked but not type-checked.
// Each corner component moves only when the new position lies outside it
// (`test $0x1` for the min, `test $0x41` for the max, both skipping the
// store), and the counter at 0x22c goes up whether or not the box grew.
void ci_SetParticlePos(HLOCALOBJ hSystem, LTParticle* pParticle, LTVector* pPos)
{
	if (hSystem && pParticle && pPos)
	{
		CClientParticleSystemBox* pSystem = (CClientParticleSystemBox*)hSystem;

		((CClientParticle*)pParticle)->m_Pos = *pPos;

		if (pPos->x < pSystem->m_MinPos.x) pSystem->m_MinPos.x = pPos->x;
		if (pPos->y < pSystem->m_MinPos.y) pSystem->m_MinPos.y = pPos->y;
		if (pPos->z < pSystem->m_MinPos.z) pSystem->m_MinPos.z = pPos->z;

		if (pPos->x > pSystem->m_MaxPos.x) pSystem->m_MaxPos.x = pPos->x;
		if (pPos->y > pSystem->m_MaxPos.y) pSystem->m_MaxPos.y = pPos->y;
		if (pPos->z > pSystem->m_MaxPos.z) pSystem->m_MaxPos.z = pPos->z;

		++pSystem->m_nChangedParticles;
	}
}


// 0x0040ba30 (237). A null name clears the map; a name that does not resolve
// clears it too (the result is stored before it is tested) and answers
// LT_NOTFOUND. The bad-object refusal reports at level 0.
LTRESULT ci_SetPolyGridEnvMap(HLOCALOBJ hObj, char* pFilename)
{
	CClientFileRef ref;

	if (!hObj || ((CClientObject*)hObj)->m_ObjectType != OT_POLYGRID)
		RETURN_ERROR(0, "SetPolyGridEnvMap", LT_INVALIDPARAMS);

	if (pFilename)
	{
		ref.m_nFileType = 1;
		ref.m_pFilename = pFilename;

		((CClientPolyGridEnv*)hObj)->m_pEnvMap = sub_41ff90(g_pClientMgr, &ref);
		if (((CClientPolyGridEnv*)hObj)->m_pEnvMap)
			return LT_OK;

		RETURN_ERROR(1, "SetPolyGridEnvMap", LT_NOTFOUND);
	}

	((CClientPolyGridEnv*)hObj)->m_pEnvMap = LTNULL;
	return LT_OK;
}


// 0x0040bb20 (213). The inverse of ci_SetPolyGridTextureInfo, with the same
// "invalid PolyGrid" refusal for a bad object and a level-0 LT_INVALIDPARAMS
// for any null out-pointer.
LTRESULT ci_GetPolyGridTextureInfo(HLOCALOBJ hObj, float* xPan, float* yPan,
                                   float* xScale, float* yScale)
{
	if (hObj && ((CClientObject*)hObj)->m_ObjectType == OT_POLYGRID)
	{
		if (xPan && yPan && xScale && yScale)
		{
			*xPan   = ((CClientPolyGrid*)hObj)->m_fXPan;
			*yPan   = ((CClientPolyGrid*)hObj)->m_fYPan;
			*xScale = ((CClientPolyGrid*)hObj)->m_fXScale;
			*yScale = ((CClientPolyGrid*)hObj)->m_fYScale;
			return LT_OK;
		}

		RETURN_ERROR(0, "GetPolyGridTextureInfo", LT_INVALIDPARAMS);
	}

	RETURN_ERROR_SYM_DESC(1, "CLTClient::GetPolyGridTextureInfo", LT_ERROR, "invalid PolyGrid")
}
