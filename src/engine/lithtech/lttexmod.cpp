// *********************************************************************** //
//
//  MODULE:   CLIENTTEXMOD (.CPP) + the client's RIFF/WAVE reader
//
//  PURPOSE:  Two blocks of lithtech.exe, both untouched:
//
//    0x00488260-0x004888dc   LTTexMod, the concrete ILTTexMod.
//    0x0048c1a0-0x0048c539   the sound module's RIFF/WAVE parser.
//
//  ---- WHAT THE FIRST BLOCK IS NOT ------------------------------------- //
//
//  It is not ILTVideoMgr. Vtable 0x004a3848's six slots are
//
//      0x004a3848 -> 0x00488280      0x004a3854 -> 0x00488640
//      0x004a384c -> 0x00488580      0x004a3858 -> 0x00488720
//      0x004a3850 -> 0x004886a0      0x004a385c -> 0x004888c0
//
//  and each of those six pushes a DIFFERENT static char* in its parameter
//  check. Dereferenced they read:
//
//      0x004af408 -> "LTTexMod::GetTextureHandle"           0x00488280
//      0x004af40c -> "LTTexMod::ReleaseTextureHandle"       0x00488580
//      0x004af410 -> "LTTexMod::WasTextureDrawnLastFrame"   0x00488640
//      0x004af414 -> "LTTexMod::GetTextureInfo"             0x004886a0
//      0x004af418 -> "LTTexMod::LockTexture"                0x00488720
//      0x004af41c -> "LTTexMod::UnlockTexture"              0x004888c0
//
//  So the class is src/LT2/sdk/inc/ilttexmod.h's ILTTexMod, whose six pure
//  virtuals are declared in exactly that order, and whose `ret N` sizes
//  match slot for slot: 0xc, 0x4, 0x8, 0x4, 0x14, 0x4. The vtable's own
//  order is declaration order (GetTextureHandle, ReleaseTextureHandle,
//  GetTextureInfo, WasTextureDrawnLastFrame, LockTexture, UnlockTexture),
//  which is why slots 2 and 3 of the address list above look swapped
//  against the string list - they are not, the strings are in ADDRESS
//  order and the header is in DECLARATION order.
//
//  ILTVideoMgr is elsewhere: the video manager is the SmackVideoMgr block
//  that starts at 0x00488920, immediately after UnlockTexture, and runs to
//  0x0048c1a0 ("SmackVideoMgr", "SmackVideoInst::Init", "SmackOpen failed",
//  "Invalid smackw32.dll, video playback disabled."). That is RAD
//  middleware glue - note it reaches 0x0048c1a0, past
//  the 0x0048c000 the exclusion named, so 0x0048a000-0x0048c1a0 is Smack
//  too and nothing in it is touched here.
//
//  ---- WHAT THE SECOND BLOCK IS ---------------------------------------- //
//
//  0x0048c1a0 is called from three sites in the client sound module, one of
//  them 0x0047c921 inside CSoundData::Load ("Loaded sound file %s", see
//  src/engine/lithtech/clientsoundinst.cpp). It is handed `this + 8`, so
//  the 0x38-byte structure it fills lives at CSoundData+0x08, and every
//  field of it is cross-checked against how Load uses it - see WaveInfo.
//
//  It is NOT the Miles layer. Between 0x0048c270 and 0x0048f900 there are
//  exactly two import references in the whole range (DirectInputCreateA at
//  0x0048db30 and _AIL_shutdown@0 at 0x0048de20, both bare `jmp *__imp`
//  thunks) and two string references ("*.*", "pEB"). The bulk of that
//  range - 0x0048c540 onwards - is a different, un-named C++ subsystem
//  built on 16-byte link records, std::map lookups (0x004a03d0 /
//  0x004a0560) and SEH frames; its base constructor is 0x0048c6e0
//  (vtable 0x004a38d8, one argument stored at +0x54, -1 at +0x58, five
//  self-tied 16-byte links from +0x04) and its derived classes are
//  constructed at 0x004243ce (0xa0 bytes, vtable 0x004a1fd4) and
//  0x0045a670 / 0x0045a6c0 (vtable 0x004a2c08). It is not identified
//  from the bytes and not guessed: nothing from 0x0048c540 up is written
//  here.
//
//  ---- A CODEGEN FACT THIS BLOCK PROVES -------------------------------- //
//
//  VC6 puts inlined-callee temporaries in DEAD PARAMETER HOME SLOTS. Both
//  0x0048c1a0 and 0x0048c270 call iltstream.h's inline helpers
//  ILTStream::GetPos() / GetLen(), each of which declares its own
//  `unsigned long` and passes its address to the virtual; the shipped code
//  passes `&[esp+arg1]` and `&[esp+arg2]` for those, having already cached
//  the parameters in esi/ebx/ebp. 0x0041ff90 does the same with its own
//  second parameter. So a frame that looks "too small for its locals" is
//  normal here and the source needs no contortion - declare the locals and
//  let the compiler place them.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltrect.h"
#include "iltstream.h"
#include "ilttexmod.h"
#include "de_error.h"


// ======================================================================= //
//  Views of the classes this file does not own
// ======================================================================= //

// A pixel format. One virtual, in a one-entry vtable at 0x004a1ab4, and
// 0x38 bytes; the constructor is nothing but the vptr store, which is why
// LockTexture's local costs one `movl $0x4a1ab4,...` and no call.
//
// Init's signature is read off 0x0045f0e0, which stores its five arguments
// to +0x04, +0x08, +0x0c, +0x10, +0x14 and then loops i=0..3 calling
// 0x0045f120(this, i) - four colour planes' worth of derived data, which is
// the 0x20 bytes from +0x18 to the end. What the arguments MEAN comes from
// 0x0045f390, which calls Init(2, 0, 0xf800, 0x7e0, 0x1f): 565 RGB. So
// argument one is the bit depth and two through five are the ALPHA, RED,
// GREEN, BLUE masks - ltpvalue.h's CP_ALPHA..CP_BLUE order.
class PFormat
{
public:
	// NOT pure, and NO declared constructor: LockTexture builds one of these
	// by value and the shipped code's whole constructor is the inline
	// `movl $0x4a1ab4,...` an implicit one emits, so declaring either would
	// add a call that is not there. The cost is that the emitted vtable
	// references an Init this file does not define - a link-time alias to
	// 0x0045f0e0, the same treatment the sub_* entries below need.
	virtual void Init(uint32 nBitDepth, uint32 aMask, uint32 rMask,
	                  uint32 gMask, uint32 bMask);          // +0x00 0x0045f0e0

	void   InitPValue();                     // 0x0045f1b0 (name ours)
	LTBOOL IsSame(PFormat* pOther);          // 0x0045f1e0 (name ours)
	uint32 GetBytesPerPixel();               // 0x0045f230

	// 0x004476b0, `mov 0x4(%ecx),%eax; ret` -- a linker-folded one-line
	// getter shared with other classes' identical accessors. Convert
	// (0x0045f3f1 / 0x0045f3fa) calls it out of line for both formats.
	uint32 GetType();                        // 0x004476b0 (name ours)

public:
	uint32 m_nBitDepth;                      // 0x04
	uint32 m_Masks[4];                       // 0x08  A, R, G, B
	// Written by pf_InitPlane (0x0045f120): 0x0045f14b / 0x0045f14f.
	uint32 m_nBits[4];                       // 0x18  bits per plane
	uint32 m_nShifts[4];                     // 0x28  low bit index per plane
};

// The bit depths this file compares against. Numeric in the shipped code and
// numeric here: 2 is 565 (from 0x0045f390's Init call) and 3 is what
// LockTexture demands before it will hand out a pointer, which ilttexmod.h
// says is PValue - i.e. 32-bit. 1 is what a bumpmap's destination format is
// initialised to, with an 0xff alpha mask and nothing else, i.e. 8-bit.
#define TM_BITDEPTH_8    1
#define TM_BITDEPTH_565  2
#define TM_BITDEPTH_32   3

// One conversion request. No vptr of its own: constructor 0x0045f240 stores
// the address of the embedded source format at +0x00 and of the embedded
// destination format at +0x10, installs 0x004a1ab4 in both, and then writes
// 0 to +0x04/+0x0c/+0x14/+0x24 and -1 to +0x08/+0x18/+0x1c/+0x20. The two
// embedded formats are 0x38 apart (+0x28 and +0x60), so the class is 0x98
// bytes - which is exactly what GetTextureHandle's frame reserves between
// its two conversion locals.
//
// The pitches are int32 because the constructor seeds them with -1.
class FMConvertRequest
{
public:
	FMConvertRequest();                      // 0x0045f240
	LTBOOL IsValid();                        // 0x0045f280 (name ours)

public:
	PFormat* m_pSrcFormat;                   // 0x00 -> &m_SrcFormat
	void*    m_pSrcData;                     // 0x04
	int32    m_nSrcPitch;                    // 0x08  (-1)
	uint32   m_Unknown0c;                    // 0x0c
	PFormat* m_pDestFormat;                  // 0x10 -> &m_DestFormat
	void*    m_pDestData;                    // 0x14
	int32    m_nDestPitch;                   // 0x18  (-1)
	int32    m_nWidth;                       // 0x1c  (-1) written from
	                                         //       TextureData+0xcc
	int32    m_Unknown20;                    // 0x20  (-1) never written here
	int32    m_nHeight;                      // 0x24  written from
	                                         //       TextureData+0xd0
	PFormat  m_SrcFormat;                    // 0x28  a PFormat by value
	PFormat  m_DestFormat;                   // 0x60  a PFormat by value
};

// The thing that performs one. Constructor 0x0045f390 installs 0x004a1ab4 at
// +0x00 and +0x38 (so it opens with two PFormats by value), calls
// 0x0045f1b0, gives the second one 565, and calls 0x0045f4f0. Convert
// (0x0045f3d0) validates the request through 0x0045f280 and then indexes a
// table of blitters at 0x004acdb8 by the two formats' 0x004476b0 codes.
//
// The SIZE, 0xb94, is arithmetic and not a guess: GetTextureHandle's frame
// is 0xc3c and holds exactly this, an FMConvertRequest (0x98), a
// CClientFileRef (0xc) and one pointer (4). 0xb94 + 0x98 + 0xc + 4 = 0xc3c.
class FMConvertMgr
{
public:
	FMConvertMgr();                                  // 0x0045f390
	LTRESULT Convert(FMConvertRequest* pRequest);    // 0x0045f3d0 (name ours)
	void InitTables();                               // 0x0045f4f0 (name ours)

public:
	// fmconvert.cpp's layout, read off InitTables (0x0045f4f0).
	PFormat m_FormatPValue;                  // 0x000 32-bit A8R8G8B8
	PFormat m_Format565;                     // 0x038 R5G6B5
	uint8  m_ToTables[9][256];               // 0x070 8-bit -> n-bit
	uint8* m_pFromTables[9];                 // 0x970 n-bit -> 8-bit
	uint8  m_TableData[511];                 // 0x994 2^0 + ... + 2^8 entries
};

// A texture's loaded data. Slot +0x00 of its vtable hands its pixel format
// back through a pointer - LockTexture calls it with the address of a bare
// local PFormat and then reads that local's bit depth, and GetTextureHandle
// calls it with FMConvertRequest::m_pSrcFormat.
//
// Field offsets, all off the bytes:
//   0x10/0x12  uint16 base width and height. GetTextureHandle reads them
//              zero-extended (`xor %ecx,%ecx; mov 0x10(%eax),%cx`) and
//              hands them to the creator at 0x0042e8e0.
//   0x14       uint16 mipmap count. `cmpw $0x1,0x14(%eax)` in the validity
//              helper, and 0x0042e8e0's fourth argument is the literal 1 -
//              so a handle is only valid on a texture LTTexMod made itself.
//   0x18       flag word; bit 0x40 is what GetTextureHandle sets on the
//              texture it creates and what the validity helper tests.
//   0xcc/0xd0  uint32 width and height of the locked level. LockTexture's
//              default rect is (0, 0, 0xcc, 0xd0).
//   0xd4/0xd8  the colour data and its pitch.
//   0xdc/0xe0  the bumpmap data and its pitch; null unless the texture was
//              created with THANDLE_BUMPMAP.
class TextureData
{
public:
	// Not necessarily the real slot 0 of the real class - it is the slot the
	// two functions here call, and nothing else about the vtable is read.
	virtual void GetFormat(PFormat* pFormat) = 0;    // +0x00 (name ours)

public:
	uint8  m_pad_04[0x10 - 0x04];
	uint16 m_nBaseWidth;                     // 0x10
	uint16 m_nBaseHeight;                    // 0x12
	uint16 m_nMipmaps;                       // 0x14
	uint8  m_pad_16[0x18 - 0x16];
	uint32 m_Flags;                          // 0x18
	uint8  m_pad_1c[0xcc - 0x1c];
	uint32 m_nWidth;                         // 0xcc
	uint32 m_nHeight;                        // 0xd0
	uint8* m_pData;                          // 0xd4
	long   m_nPitch;                         // 0xd8
	uint8* m_pBumpmapData;                   // 0xdc
	long   m_nBumpmapPitch;                  // 0xe0
};

// The flag TextureData::m_Flags carries for a texture LTTexMod created.
// Numeric in the shipped code; the name is ours.
#define TDFLAG_TEXMOD 0x40

// ilttexmod.h forward-declares this and typedefs HTEXTURE to a pointer to
// it, so the definition belongs here rather than in a *View class.
//
// 0x32 is a BITFIELD and the bytes say so twice. GetTextureHandle
// increments it with VC6's read-modify-write-through-xor sequence
//
//      mov 0x32(%ecx),%ax ; and $0x7fff,%eax ; inc %eax
//      xor 0x32(%ecx),%ax ; and $0x7fff,%eax ; xor %ax,0x32(%ecx)
//
// which only appears for a bitfield narrower than its storage unit, and
// ReleaseTextureHandle runs the same sequence with `dec`. The mask is
// 0x7fff, so the count is fifteen bits of a uint16 and one bit above it is
// something else. The validity helper reads it as `testw $0x7fff,0x32(%eax)`
// followed by `jbe`, which is how VC6 compiles `> 0` on an unsigned - not
// `!= 0`, which would be `jne`.
class SharedTexture
{
public:
	uint8        m_pad_00[0x08];
	TextureData* m_pEngineData;              // 0x08
	uint8        m_pad_0c[0x30 - 0x0c];
	// Compared against the client manager's copy: equal means the texture
	// was touched by the last Render.
	uint16       m_nFrameCode;               // 0x30
	uint16       m_nRefCount : 15;           // 0x32
	uint16       m_Unknown32_15 : 1;         // 0x32 bit 15

	// Stand-in accessors (names ours). GetTextureHandle's increment through
	// them extracts with `and $0x7fff` before the `inc` as shipped does
	// (0x0048849c); a plain `m_nRefCount++` folds that into `lea 1`.
	uint16       GetRefCount() { return m_nRefCount; }
	void         SetRefCount(uint16 n) { m_nRefCount = n; }
};

// The file the object/texture loader is asked for. src/engine/lithtech/
// clientmgr.h already has this class, field for field and with the same
// three-store inline constructor; it is duplicated here only so this file
// stands alone. Do not diverge from that copy.
class CClientFileRef
{
public:
	CClientFileRef() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// The client manager, from the one field reached here. clientmgr.h's real
// CClientMgr stops at 0x1538, so the frame code below is past its end and
// this stand-in view carries it instead of extending that class.
class CClientMgrTexView
{
public:
	uint8  m_pad_00[0x21ac];
	// Bumped once per Render; a texture's own copy equalling this is what
	// WasTextureDrawnLastFrame answers LT_YES to.
	uint16 m_nFrameCode;                     // 0x21ac
};


// ======================================================================= //
//  The engine entries this file calls, named by address where their purpose
//  is not pinned by their own strings.
// ======================================================================= //

// 0x0041ff90 - resolve one CClientFileRef to a SharedTexture, or null. Three
// instructions of body: it forwards to 0x0041ff50 with the address of its own
// (by then dead) second parameter slot as the out pointer and masks the
// result to null on failure with `neg/sbb/not/and`.
// clienttexmgr.cpp defines it as cm_AddSharedTexture over its own views.
class SharedTextureView;
class CClientMgrTexBankView;
class CClientFileRefView;
SharedTextureView* cm_AddSharedTexture(CClientMgrTexBankView* pClientMgr,
                                       CClientFileRefView* pRef);
#define sub_41ff90(pClientMgr, pRef) \
	((SharedTexture*)cm_AddSharedTexture((CClientMgrTexBankView*)(pClientMgr), \
	                                     (CClientFileRefView*)(pRef)))

// 0x00461e30 - bring a SharedTexture's data in and hand back the
// TextureData. cdecl, three arguments, LTRESULT; the third is passed 0 here.
// clientrender.cpp's definition: the third parameter is unsigned there.
LTRESULT sub_461e30(SharedTexture* pTexture, TextureData** ppData, unsigned int nUnknown);

// 0x00461dc0 - the matching release. Unlinks the data, drops a global byte
// count at 0x004b412c and a global object count at 0x004b4130, and frees.
void sub_461dc0(TextureData* pData);

// 0x0042e8e0 - create a TextureData. Seven cdecl arguments; GetTextureHandle
// passes (3, width, height, 1, 0, 0, bBumpmap), so argument one is a bit
// depth, four is the mipmap count (and is what makes the validity helper's
// `m_nMipmaps == 1` test meaningful) and seven asks for the bumpmap plane.
// clientdsi.cpp defines it as dtx_Alloc: arguments five and six are its
// two optional out-pointers (null here).
TextureData* dtx_Alloc(uint32 bpp, uint32 width, uint32 height, uint32 nMipmaps,
                       uint32* pAllocSize, uint32* pTextureDataSize, LTBOOL bBumpmap);
#define sub_42e8e0(nBitDepth, nWidth, nHeight, nMipmaps, n5, n6, bBumpmap) \
	dtx_Alloc((nBitDepth), (nWidth), (nHeight), (nMipmaps), \
	          (uint32*)(n5), (uint32*)(n6), (bBumpmap))

// 0x0042ee30 - free one TextureData outright (0x0042e830 then dfree).
void dtx_Destroy(TextureData* pTexture);     // clientdsi.cpp
#define sub_42ee30 dtx_Destroy

// 0x004628e0 - tell the render DLL a texture's contents changed. Does
// nothing unless the renderer at 0x004b439c is up, then calls through the
// function pointer at 0x004b43cc.
void sub_4628e0(SharedTexture* pTexture, int nUnknown);


// ======================================================================= //
//  LTTexMod - 0x00488260 .. 0x004888dc
// ======================================================================= //

// The class itself. One field: constructor 0x00488260 stores the vtable and
// then its single argument at +0x04, and WasTextureDrawnLastFrame reads
// `0x4(%ecx)` and then `0x21ac` off it.
class LTTexMod : public ILTTexMod
{
public:
	LTTexMod(CClientMgrTexView* pClientMgr);

	LTRESULT GetTextureHandle(char* pFilename, HTEXTURE& hTexture,
	                          const uint32 flags);
	LTRESULT ReleaseTextureHandle(const HTEXTURE hTexture);
	LTRESULT GetTextureInfo(const HTEXTURE hTexture, TextureInfo& info);
	LTRESULT WasTextureDrawnLastFrame(const HTEXTURE hTexture);
	LTRESULT LockTexture(const HTEXTURE hTexture, const LTRect* pRect,
	                     const uint32 lockType, uint8*& pData, long& lPitch);
	LTRESULT UnlockTexture(const HTEXTURE hTexture);

public:
	CClientMgrTexView* m_pClientMgr;         // 0x04 (vptr is 0x00)
};


// 0x00488260 - the constructor. Six instructions and a `ret $0x4`: the vptr
// store the compiler adds, one field, and `mov %ecx,%eax` at the top because
// a VC6 constructor returns `this`.
LTTexMod::LTTexMod(CClientMgrTexView* pClientMgr)
{
	m_pClientMgr = pClientMgr;
}


// 0x00488610 - is this handle one of ours and still alive? A file-static
// cdecl helper (`ret`, not `ret N`), called by four of the six methods:
// 0x00488586, 0x004886b8, 0x00488745, 0x004888c6.
//
// WHAT PINS THE SHAPE: every failing test jumps to ONE `xor %eax,%eax; ret`
// at 0x0048863a and the success is a separate `mov $0x1,%eax; ret` reached
// by falling through, which is an && chain wrapped in one `if` and not a run
// of guarded returns. `mov 0x8(%eax),%eax` loads m_pEngineData once and both
// of the last two tests read it out of eax, so the source names it.
//
// The refcount test is `testw $0x7fff,0x32(%eax)` + `jbe`, i.e. `> 0` on an
// unsigned bitfield. `!= 0` would have compiled to `jne`.
static LTBOOL tm_IsValidTextureHandle(const HTEXTURE hTexture)
{
	TextureData* pData;

	if (hTexture &&
	    hTexture->m_nRefCount > 0 &&
	    (pData = hTexture->m_pEngineData) != LTNULL &&
	    pData->m_nMipmaps == 1 &&
	    (pData->m_Flags & TDFLAG_TEXMOD))
	{
		return LTTRUE;
	}

	return LTFALSE;
}


// 0x00488280 - slot +0x00, 758 bytes. Get a handle, creating the 32-bit
// working copy the texture-modification interface hands out.
//
// WHAT PINS IT: three report sites, all pushing the same static name pointer
// 0x004af408 ("LTTexMod::GetTextureHandle") and all comparing g_DebugLevel
// against 2, but with three different code strings - "LT_INVALIDPARAMS"
// (symbolic, so RETURN_ERROR_SYM), "61" and "67" (numeric, so RETURN_ERROR).
// 61 is LT_NOTFOUND and 67 is LT_OUTOFMEMORY. de_error.h's note about one
// name variable serving several sites is this function.
//
// THE FRAME: `sub $0xc3c` and the four locals sit at esp0-0xb94 (0xb94
// bytes), esp0-0xc2c (0x98), esp0-0xc38 (0xc) and esp0-0xc3c (4). The two
// constructor CALLS fix the converter before the request (0x0045f390 runs
// before 0x0045f240). The frame order does NOT follow the reference's
// declaration position: declaring it first (see LAYOUT below) moves only its
// constructor stores ahead of the calls, as shipped has them.
//
// The five stores to the reference are all accounted for: three from the
// constructor (0x00488295, 0x00488299, 0x0048829d - the last a word, which
// is m_nLocalID) and two from the two assignments below.
//
// LAYOUT: the file reference is declared FIRST (its three constructor stores
// precede the two constructor calls, 0x00488295..0x0048829d) even though it
// sits lowest in the frame. After the pFilename refusal the body is NESTED
// positive guards: shipped places every later refusal after the success
// return, in reverse source order (0x004884ba bump failure, 0x004884c9
// first Convert failure, 0x004884e5 out of memory, 0x00488530 not found),
// which is how VC6 lays out the else-sides of nested ifs.
//
// STILL NEAR, one block, 758 bytes both: the final refcount increment.
// Shipped re-reads the word for the insert (`xor 0x32(%ecx),%ax`) after an
// `and $0x7fff; inc %eax` extraction; SetRefCount(GetRefCount() + 1) here
// gives a 16-bit `inc %dx` with the word kept in ecx. Measured: `++`, `+= 1`,
// `= x + 1` (3 blocks), `= GetRefCount() + 1` (and+inc, but register copy,
// 762 bytes), uint32/int getters or setter, const getter, IncRefCount /
// AddRef members, a pTex local, a uint16 local, casts (1-3 blocks, none
// closer).
//
// CONFIDENCE: the control flow, the constants and the field offsets are all
// off the bytes. The local declaration ORDER and the split between the two
// LTRESULT variables (the shipped code keeps the first conversion's result
// in ebx and the second's in edi, which "VC6 never splits live
// ranges" reads as two variables) are the parts a compile will have to
// settle.
LTRESULT LTTexMod::GetTextureHandle(char* pFilename, HTEXTURE& hTexture,
                                   const uint32 flags)
{
	static char* pErrFnName = "LTTexMod::GetTextureHandle";

	CClientFileRef   ref;
	FMConvertMgr     cvtMgr;
	FMConvertRequest request;
	TextureData*     pOrigData;

	TextureData* pNewData;
	uint32       bBumpmap;
	LTRESULT     dResult;
	LTRESULT     dBumpResult;

	hTexture = LTNULL;

	if (!pFilename)
		RETURN_ERROR_SYM(2, pErrFnName, LT_INVALIDPARAMS);

	ref.m_nFileType = 0;
	ref.m_pFilename = pFilename;

	hTexture = sub_41ff90(m_pClientMgr, &ref);
	if (hTexture)
	{
		dResult = sub_461e30(hTexture, &pOrigData, 0);
		if (dResult != LT_OK)
			return dResult;

		bBumpmap = flags & THANDLE_BUMPMAP;

		// The working copy is always 32-bit, one mip level, and carries a
		// bumpmap plane only if one was asked for.
		pNewData = sub_42e8e0(TM_BITDEPTH_32, pOrigData->m_nBaseWidth,
		                      pOrigData->m_nBaseHeight, 1, 0, 0, bBumpmap);
		if (pNewData)
		{
			// Colour: the file's format and data in, the working copy's out.
			pOrigData->GetFormat(request.m_pSrcFormat);
			request.m_pSrcData  = pOrigData->m_pData;
			request.m_nSrcPitch = pOrigData->m_nPitch;

			pNewData->GetFormat(request.m_pDestFormat);
			request.m_pDestData  = pNewData->m_pData;
			request.m_nDestPitch = pNewData->m_nPitch;

			request.m_nWidth  = pOrigData->m_nWidth;
			request.m_nHeight = pOrigData->m_nHeight;

			dResult = cvtMgr.Convert(&request);
			if (dResult == LT_OK)
			{
				if (bBumpmap)
				{
					// Same source, but the destination is described by hand
					// rather than asked for: eight bits, alpha mask only,
					// which is ilttexmod.h's DBYTE heightmap.
					pOrigData->GetFormat(request.m_pSrcFormat);
					request.m_pSrcData  = pOrigData->m_pData;
					request.m_nSrcPitch = pOrigData->m_nPitch;

					request.m_pDestFormat->Init(TM_BITDEPTH_8, 0xff, 0, 0, 0);
					request.m_pDestData  = pNewData->m_pBumpmapData;
					request.m_nDestPitch = pNewData->m_nBumpmapPitch;

					request.m_nWidth  = pOrigData->m_nWidth;
					request.m_nHeight = pOrigData->m_nHeight;

					dBumpResult = cvtMgr.Convert(&request);
				}
				else
				{
					dBumpResult = LT_OK;
				}

				sub_461dc0(pOrigData);

				if (dBumpResult == LT_OK)
				{
					pNewData->m_Flags |= TDFLAG_TEXMOD;
					hTexture->m_pEngineData = pNewData;
					hTexture->SetRefCount(hTexture->GetRefCount() + 1);

					return LT_OK;
				}

				return dBumpResult;
			}

			sub_461dc0(pOrigData);
			return dResult;
		}

		sub_461dc0(pOrigData);
		RETURN_ERROR(2, pErrFnName, LT_OUTOFMEMORY);
	}

	RETURN_ERROR(2, pErrFnName, LT_NOTFOUND);
}


// 0x00488580 - slot +0x04, 135 bytes. Give a handle back.
//
// WHAT PINS IT: one report site pushing 0x004af40c
// ("LTTexMod::ReleaseTextureHandle") with "LT_INVALIDPARAMS" at level 2,
// then the bitfield decrement whose RESULT is what gets tested
// (`test $0x7fff,%eax` before the store-back, `jne` past the free), so the
// source is `--count == 0` and not `count-- == 1`.
//
// STILL NEAR (127 vs 135): shipped extracts the field with `and $0x7fff`
// BEFORE the `dec` (0x004885d6); every spelling here lets VC6 fold that and
// into `lea -1`. Measured, all 127 unless noted: `--c == 0`, `c--; c == 0`,
// `c = c - 1`, `c -= 1`, `(uint16)(c - 1)`, `!--c`, `--c <= 0`, a uint32 or
// uint16 local round trip, `c-- == 1` (134, different shape). An int16 / int
// / uint32 bitfield changes the extraction but breaks tm_IsValidTextureHandle.
// Second pass: the `and` DOES appear when the extraction is its own inline
// accessor (`c = GetRefCount() - 1`, uint16 return), but the word is then
// loaded twice (134 bytes; a uint32 accessor 132); `+= -1`, `c--; !c`,
// if/else on `--c`, int locals either side of the `- 1`, `- (uint16)1`
// all stay at 127.
LTRESULT LTTexMod::ReleaseTextureHandle(const HTEXTURE hTexture)
{
	if (!tm_IsValidTextureHandle(hTexture))
		RETURN_ERROR_NAMED(2, "LTTexMod::ReleaseTextureHandle",
		                   LT_INVALIDPARAMS);

	if (--hTexture->m_nRefCount == 0)
	{
		if (hTexture->m_pEngineData)
			sub_42ee30(hTexture->m_pEngineData);
	}

	return LT_OK;
}


// 0x004886a0 - slot +0x08, 124 bytes. Width and height.
//
// WHAT PINS IT: the two `movl $0x0,...` through the out reference come
// BEFORE the call to the validity helper, so the zeroing is unconditional
// and is the first thing the source does. Then 0x0048871b reads 0xcc and
// 0xd0 off m_pEngineData, which is what fixes those two offsets as the
// TextureInfo pair rather than the uint16s at 0x10/0x12.
LTRESULT LTTexMod::GetTextureInfo(const HTEXTURE hTexture, TextureInfo& info)
{
	TextureData* pData;

	info.m_Width  = 0;
	info.m_Height = 0;

	if (!tm_IsValidTextureHandle(hTexture))
		RETURN_ERROR_NAMED(2, "LTTexMod::GetTextureInfo", LT_INVALIDPARAMS);

	pData = hTexture->m_pEngineData;

	info.m_Width  = pData->m_nWidth;
	info.m_Height = pData->m_nHeight;

	return LT_OK;
}


// 0x00488640 - slot +0x0c, 94 bytes. Was it drawn?
//
// WHAT PINS IT: a bare null check (this one does NOT go through the validity
// helper) reporting 0x004af410, and then
//
//      xor %ecx,%ecx ; cmp 0x21ac(%edx),%ax ; setne %cl
//      add $0x56,%ecx
//
// which is VC6's form for a ternary between two adjacent constants: 0x56 is
// LT_YES (86) and 0x57 is LT_NO (87), and `setne` means EQUAL is the LT_YES
// arm. Both operands are 16-bit, which is what makes the frame code a
// uint16 on both sides.
LTRESULT LTTexMod::WasTextureDrawnLastFrame(const HTEXTURE hTexture)
{
	if (!hTexture)
		RETURN_ERROR_NAMED(2, "LTTexMod::WasTextureDrawnLastFrame",
		                   LT_INVALIDPARAMS);

	return (hTexture->m_nFrameCode == m_pClientMgr->m_nFrameCode)
	       ? LT_YES : LT_NO;
}


// 0x00488720 - slot +0x10, 415 bytes. Hand out a pointer into the texture.
//
// THE FRAME: `sub $0x48`, and the two locals fill it exactly - a PFormat
// (0x38) at esp0-0x38 and an LTRect (0x10) at esp0-0x48. First declared is
// highest, and the construction order in the bytes agrees: the PFormat's
// vptr store (0x0048872d) precedes the rect's four zero stores
// (0x00488735..0x00488741), which are LTRect's default constructor with all
// four arguments defaulted.
//
// WHAT PINS THE REST:
//   * two report sites push 0x004af418 and share their printf tail by
//     cross-jump (0x00488780 jumps into 0x004888a9): both level 2,
//     "LT_INVALIDPARAMS", so one name variable and RETURN_ERROR_SYM twice.
//   * a third site is level 1 with the NUMERIC "73" - LT_NOTINITIALIZED -
//     and the bumpmap-missing case at 0x0048885e jumps BACKWARDS into it,
//     which is VC6 merging two identical tails.
//   * `cmpl $0x3,0x20(%esp)` after the virtual GetFormat is the bit depth,
//     32-bit, i.e. ilttexmod.h's "the data is in PValue format".
//   * the lock type is checked against 0 and 1 only - TLOCK_TEXTURE and
//     TLOCK_BUMPMAP - and the pointer/pitch pair comes from 0xd4/0xd8 for
//     the first and 0xdc/0xe0 for the second.
//   * the default rect's stores run bottom, TOP, left, right (0x00488796..
//     0x004887a8): four field stores with top first. rect.Init(0,0,w,h)
//     writes left before top.
//   * the bumpmap arm is a positive test with its own `return LT_OK` and the
//     error after it, which puts the bit-depth error at the fall-through of
//     0x004887ec `je` with 0x0048885e jumping back into it. An `if (!bump)
//     RETURN_ERROR` guard puts the error block last (40 insns differ).
//
// STILL NEAR, size-exact: 7 instructions in the bumpmap tail
// (0x00488860..0x00488872) - shipped loads &pData, stores, then loads
// &lPitch; ours loads both references first. Register allocation; storing
// lPitch first, an else-if, nesting it in an else, and a lockType test on
// the bump arm all measured no better or worse.
LTRESULT LTTexMod::LockTexture(const HTEXTURE hTexture, const LTRect* pRect,
                               const uint32 lockType, uint8*& pData,
                               long& lPitch)
{
	static char* pErrFnName = "LTTexMod::LockTexture";

	PFormat      format;
	LTRect       rect;
	TextureData* pTextureData;

	if (!tm_IsValidTextureHandle(hTexture))
		RETURN_ERROR_SYM(2, pErrFnName, LT_INVALIDPARAMS);

	pTextureData = hTexture->m_pEngineData;

	if (!pRect)
	{
		rect.top    = 0;
		rect.left   = 0;
		rect.right  = pTextureData->m_nWidth;
		rect.bottom = pTextureData->m_nHeight;
		pRect = &rect;
	}

	if (pRect->right <= pRect->left ||
	    pRect->bottom <= pRect->top ||
	    (lockType != TLOCK_TEXTURE && lockType != TLOCK_BUMPMAP))
	{
		RETURN_ERROR_SYM(2, pErrFnName, LT_INVALIDPARAMS);
	}

	pTextureData->GetFormat(&format);
	if (format.m_nBitDepth != TM_BITDEPTH_32)
		RETURN_ERROR(1, pErrFnName, LT_NOTINITIALIZED);

	if (lockType == TLOCK_TEXTURE)
	{
		pData  = pTextureData->m_pData;
		lPitch = pTextureData->m_nPitch;
		return LT_OK;
	}

	if (pTextureData->m_pBumpmapData)
	{
		pData  = pTextureData->m_pBumpmapData;
		lPitch = pTextureData->m_nBumpmapPitch;
		return LT_OK;
	}

	RETURN_ERROR(1, pErrFnName, LT_NOTINITIALIZED);
}


// 0x004888c0 - slot +0x14, 95 bytes. Give it back to the renderer.
//
// WHAT PINS IT: 0x004af41c ("LTTexMod::UnlockTexture") at level 2, then a
// two-argument cdecl call to 0x004628e0 with the literal 1 and no use of
// the result. 0x004628e0 does nothing at all unless the render DLL is up.
LTRESULT LTTexMod::UnlockTexture(const HTEXTURE hTexture)
{
	if (!tm_IsValidTextureHandle(hTexture))
		RETURN_ERROR_NAMED(2, "LTTexMod::UnlockTexture", LT_INVALIDPARAMS);

	sub_4628e0(hTexture, 1);

	return LT_OK;
}


// 0x004b4cf8 - a file-scope CGLinkedList, and the dynamic-initialiser group
// it emits straight after UnlockTexture: 0x00488920 (the head, .CRT$XCU slot
// 0x004a70c8: call the constructor body, jmp to the registrar), 0x00488930
// (constructor: vptr 0x004a3860, then m_nElements at +0x04 and m_pHead at
// +0x08 zeroed - goodlinklist.h's constructor order), 0x00488950 (atexit of
// 0x00488960) and 0x00488960 (destructor: the vptr again, then +0x08 before
// +0x04 - RemoveAll's order). The vtable's twelve slots are the same folded
// bodies as every other CGLinkedList<pointer> in the image (0x004a1a84).
// Nothing but its own constructor and destructor reads the object, so the
// element type is unrecoverable and the list is dead; type and name ours.
#include "../../LT2/lithshared/stdlith/goodlinklist.h"

static CGLinkedList<SharedTexture*> g_TexModList;


// ======================================================================= //
//  The RIFF/WAVE reader - 0x0048c1a0 .. 0x0048c539
// ======================================================================= //

// A four-character chunk code as the stream delivers it: little-endian, so
// 'data' read off disk arrives as 0x61746164 and NOT as MSVC's multi-
// character literal 'data' (0x64617461). This is mmsystem.h's mmioFOURCC to
// the byte; it is spelled out here so the file stands alone.
#define WAVE_FOURCC(c0, c1, c2, c3)                             \
	((uint32)(uint8)(c0) | ((uint32)(uint8)(c1) << 8) |         \
	 ((uint32)(uint8)(c2) << 16) | ((uint32)(uint8)(c3) << 24))

#define FOURCC_RIFF WAVE_FOURCC('R', 'I', 'F', 'F')
#define FOURCC_LIST WAVE_FOURCC('L', 'I', 'S', 'T')
#define FOURCC_WAVE WAVE_FOURCC('W', 'A', 'V', 'E')
#define FOURCC_FMT  WAVE_FOURCC('f', 'm', 't', ' ')
#define FOURCC_DATA WAVE_FOURCC('d', 'a', 't', 'a')
#define FOURCC_FACT WAVE_FOURCC('f', 'a', 'c', 't')
#define FOURCC_CUE  WAVE_FOURCC('c', 'u', 'e', ' ')
#define FOURCC_ADTL WAVE_FOURCC('a', 'd', 't', 'l')
#define FOURCC_LTXT WAVE_FOURCC('l', 't', 'x', 't')
#define FOURCC_LITH WAVE_FOURCC('l', 'i', 't', 'h')
#define FOURCC_INFO WAVE_FOURCC('i', 'n', 'f', 'o')

// The three format tags 0x0048c247 accepts, numeric in the shipped code:
// 1 is PCM, 0x11 is IMA ADPCM and 0x55 is MPEG layer 3.
#define WAVEFMT_PCM       0x0001
#define WAVEFMT_IMAADPCM  0x0011
#define WAVEFMT_MP3       0x0055

// mmreg.h's WAVEFORMATEX. It matters that this is eighteen bytes and not
// twenty: the parser reads 0x12 bytes into the front of WaveInfo and the
// next field is at 0x14, so the struct is byte-packed inside a container
// that is not - which is exactly what <pshpack1.h> around WAVEFORMATEX
// produces. Declared here rather than included so the file stands alone.
#pragma pack(push, 1)
struct WaveFormatEx
{
	uint16 wFormatTag;                       // 0x00
	uint16 nChannels;                        // 0x02
	uint32 nSamplesPerSec;                   // 0x04
	uint32 nAvgBytesPerSec;                  // 0x08
	uint16 nBlockAlign;                      // 0x0c
	uint16 wBitsPerSample;                   // 0x0e
	uint16 cbSize;                           // 0x10
};
#pragma pack(pop)

// What the parser fills in: fourteen dwords, zeroed wholesale by
// 0x0048c1b7's `rep stos` with a count of 0xe. It lives at CSoundData+0x08,
// and EVERY field below is cross-checked against CSoundData::Load
// (0x0047c8c0), which is what makes the names more than guesses:
//
//   0x00  Load compares (%ebp+8) against 0x11 and (%ebp+0x16) against 4 -
//         IMA ADPCM at four bits per sample - so the WAVEFORMATEX is at the
//         front and wBitsPerSample is at 0x0e of it.
//   0x14  Load adds it to the allocated buffer at CSoundData+0x44
//         (0x0047ca6d: `mov 0x1c(%ebp),%ecx; add %edx,%ecx`), so it is a
//         byte offset into the file, not a count.
//   0x18  Load's duration is `m_nDataSize * 1000 / nAvgBytesPerSec`
//         (0x0047c96b: fildll of 0x20(%ebp), fmuls 1000.0, fidivl of
//         0x10(%ebp)) - bytes over bytes-per-second, which is what proves
//         0x18 is a byte count and 0x08 of the format is nAvgBytesPerSec.
//   0x28/0x2c/0x30/0x34  Load turns 0x30 and 0x34 into millisecond marks by
//         *1000/nSamplesPerSec (0x0047c9a6 onwards, the ×5×5×5×8 chain), so
//         those two are in SAMPLES and the pair is a loop region; 0x28 is
//         the flag that says there is one.
//   0x20  Load tests bit 0 of it (0x0047ca16, `testb $0x1,0x28(%ebp)`).
//
// Names below are ours except the WAVEFORMATEX members.
struct WaveInfo
{
	WaveFormatEx m_Format;                   // 0x00  eighteen bytes + two pad
	uint32       m_nDataOffset;              // 0x14
	uint32       m_nDataSize;                // 0x18
	uint32       m_nSampleCount;             // 0x1c
	uint32       m_nLithFlags;               // 0x20  the 'lith'/'info' pair,
	uint32       m_nLithUnknown24;           // 0x24  eight bytes read whole
	uint32       m_bHasLoop;                 // 0x28
	uint32       m_nLoopName;                // 0x2c  the cue point's dwName
	uint32       m_nLoopStart;              	// 0x30  in samples
	uint32       m_nLoopLength;              // 0x34  in samples, from 'ltxt'
};

// Twenty-four bytes of 'cue ' payload, block-scoped in that case. The
// 'ltxt' case has its own twenty-byte WaveLabelText below; /O2 overlays the
// two in one 0x18 frame (`sub $0x18` in 0x0048c270). Field names are RIFF's
// for a cue point.
struct WaveChunkData
{
	uint32 m_dwName;                         // 0x00
	uint32 m_dwPosition;                     // 0x04
	uint32 m_fccChunk;                       // 0x08
	uint32 m_dwChunkStart;                   // 0x0c
	uint32 m_dwBlockStart;                   // 0x10
	uint32 m_dwSampleOffset;                 // 0x14
};

// RIFF's labelled-text chunk header, the first twenty bytes of 'ltxt'
// (the 'ltxt' case's own block-scoped struct, as in server.dll's twin
// 0x10039341, worldtree.cpp).
struct WaveLabelText
{
	uint32 m_dwName;                         // 0x00
	uint32 m_dwSampleLength;                 // 0x04
	uint32 m_dwPurpose;                      // 0x08
	uint16 m_wCountry;                       // 0x0c
	uint16 m_wLanguage;                      // 0x0e
	uint16 m_wDialect;                       // 0x10
	uint16 m_wCodePage;                      // 0x12
};


// 0x0048c270 - one chunk, 713 bytes. Recursive: 0x0048c3e6 calls it for each
// sub-chunk of a RIFF or LIST form.
//
// WHAT PINS IT: the eight-way `cmp/ja/je` cascade at 0x0048c2cc is MSVC's
// binary search over a sparse switch, and its pivots are the sorted chunk
// codes ('data' at the root, then 'RIFF', then the two smallest linearly,
// with 'lith'/'fact'/'ltxt' linear above). Every case ends by falling into
// the shared tail at 0x0048c524, which seeks to the chunk's end and returns
// LTTRUE - including the failure of a recursive call, so a bad sub-chunk
// only stops the walk.
//
// The formType argument is what makes the codes unambiguous: 'fmt ' and
// 'data' and 'fact' are only honoured inside a 'WAVE' form, 'ltxt' only
// inside 'adtl', and 'lith' only inside 'info'.
//
// IDENTICAL. The body is server.dll's twin 0x10039341 (worldtree.cpp
// wav_ParseChunk_D, identical there): the clamps as `nRead = K; if
// (chunkSize < nRead) nRead = chunkSize;` (a ternary spent chunkSize's
// register - that was the old 726-byte allocation residual), nLeft before
// the `nCuePoints > 0` test (0x0048c341), a separate 'ltxt' struct, and
// nBlockAlign read from the member at both uses. The one lithtech-only
// fact: subChunkID/subChunkSize are declared INSIDE the while body. At
// function-case scope they took the chunkID/chunkSize parameter slots
// (0x30/0x34); shipped puts them in the chunkSize/pInfo slots (lea
// 0x34(%esp) at 0x0048c3bb, lea 0x3c(%esp) at 0x0048c3c9). Declaration
// order within the case (five permutations) made no difference.
static LTBOOL ParseWaveChunk(ILTStream* pStream, uint32 chunkID,
                             uint32 chunkSize, uint32 formType,
                             WaveInfo* pInfo)
{
	uint32 nRead;

	uint32 endPos = pStream->GetPos() + chunkSize;
	if (endPos & 1)
		endPos++;
	if (endPos > pStream->GetLen())
		endPos = pStream->GetLen();

	switch (chunkID)
	{
		case FOURCC_RIFF:
		case FOURCC_LIST:
		{
			uint32 subFormType;

			pStream->Read(&subFormType, sizeof(subFormType));

			while (pStream->GetPos() < endPos)
			{
				uint32 subChunkID, subChunkSize;

				pStream->Read(&subChunkID, sizeof(subChunkID));
				pStream->Read(&subChunkSize, sizeof(subChunkSize));

				if (!ParseWaveChunk(pStream, subChunkID, subChunkSize,
				                    subFormType, pInfo))
					break;
			}
			break;
		}

		case FOURCC_FMT:
		{
			if (formType != FOURCC_WAVE)
				break;

			nRead = sizeof(WaveFormatEx);
			if (chunkSize < nRead)
				nRead = chunkSize;
			pStream->Read(&pInfo->m_Format, nRead);
			break;
		}

		case FOURCC_DATA:
		{
			if (formType != FOURCC_WAVE)
				break;

			pInfo->m_nDataOffset = pStream->GetPos();
			pInfo->m_nDataSize   = chunkSize;

			if (pInfo->m_Format.wFormatTag == WAVEFMT_IMAADPCM)
			{
				uint32 nHeaderBytes = 4 << (pInfo->m_Format.nChannels >> 1);

				uint32 nSamplesPerBlock =
					((pInfo->m_Format.nBlockAlign - nHeaderBytes) * 8) / nHeaderBytes + 1;
				uint32 nBlocks =
					(chunkSize + pInfo->m_Format.nBlockAlign - 1) / pInfo->m_Format.nBlockAlign;

				pInfo->m_nSampleCount = nSamplesPerBlock * nBlocks;
			}
			else
			{
				pInfo->m_nSampleCount = 0;
			}
			break;
		}

		case FOURCC_FACT:
		{
			if (formType != FOURCC_WAVE)
				break;
			if (pInfo->m_Format.wFormatTag != WAVEFMT_IMAADPCM)
				break;

			pStream->Read(&pInfo->m_nSampleCount,
			              sizeof(pInfo->m_nSampleCount));
			break;
		}

		case FOURCC_CUE:
		{
			WaveChunkData chunkData;
			uint32          nCuePoints, nLeft;

			pStream->Read(&nCuePoints, sizeof(nCuePoints));
			nLeft = chunkSize - sizeof(nCuePoints);
			if (nCuePoints > 0)
			{

				nRead = sizeof(chunkData);
				if (chunkSize < nLeft)
					nRead = nLeft;
				pStream->Read(&chunkData, nRead);

				if (chunkData.m_fccChunk == FOURCC_DATA)
				{
					pInfo->m_bHasLoop    = 1;
					pInfo->m_nLoopName   = chunkData.m_dwName;
					pInfo->m_nLoopStart  = chunkData.m_dwPosition;
				}
			}
			break;
		}

		case FOURCC_LTXT:
		{
			WaveLabelText labelData;

			if (formType != FOURCC_ADTL)
				break;

			nRead = sizeof(labelData);
			if (chunkSize < nRead)
				nRead = chunkSize;
			pStream->Read(&labelData, nRead);

			if (!pInfo->m_bHasLoop)
				break;
			if (labelData.m_dwName != pInfo->m_nLoopName)
				break;

			pInfo->m_nLoopLength = labelData.m_dwSampleLength;
			break;
		}

		case FOURCC_LITH:
		{
			if (formType != FOURCC_INFO)
				break;

			nRead = 8;
			if (chunkSize < nRead)
				nRead = chunkSize;
			pStream->Read(&pInfo->m_nLithFlags, nRead);
			break;
		}
	}

	pStream->SeekTo(endPos);
	return LTTRUE;
}


// 0x0048c1a0 - the whole file, 206 bytes. Called from CSoundData::Load
// (0x0047c921) and from two other sites in the sound module (0x0047cdcf,
// 0x0047d29f), each handing it `&pSoundData->m_WaveInfo`.
//
// WHAT PINS IT: `mov $0xe,%ecx; xor %eax,%eax; rep stos` clears fourteen
// dwords, which is what fixes WaveInfo at 0x38 bytes; the top-level loop
// reads an (id, size) pair with two four-byte Reads and stops on the first
// failing one; and the answer is the format tag being one of the three the
// engine can play, tested as a uint16 (`mov (%ebx),%ax; cmp $0x1,%ax`).
//
// The vtable slots are ILTStream's, in the header's declaration order with
// the protected virtual destructor taking +0x00: +0x08 Read(ptr, size),
// +0x14 SeekTo(offset), +0x18 GetPos(&out), +0x1c GetLen(&out). All four
// appear here with matching argument counts, which is the whole confirmation
// that this is an ILTStream and not some other stream class.
LTBOOL ParseWaveFile(ILTStream* pStream, WaveInfo* pInfo)
{
	uint32 chunkID, chunkSize;

	pStream->SeekTo(0);

	memset(pInfo, 0, sizeof(*pInfo));

	while (pStream->GetPos() < pStream->GetLen())
	{
		if (pStream->Read(&chunkID, sizeof(chunkID)) != LT_OK)
			break;
		if (pStream->Read(&chunkSize, sizeof(chunkSize)) != LT_OK)
			break;

		if (!ParseWaveChunk(pStream, chunkID, chunkSize, 0, pInfo))
			break;
	}

	return (pInfo->m_Format.wFormatTag == WAVEFMT_PCM ||
	        pInfo->m_Format.wFormatTag == WAVEFMT_IMAADPCM ||
	        pInfo->m_Format.wFormatTag == WAVEFMT_MP3);
}



// ======================================================================= //
//
//  THE PIXEL-FORMAT CONVERTER'S OWN PIECES (0x0045f120 .. 0x0045f5da)
//
//  HOME: these belong with src/engine/lithtech/fmconvert.cpp (the TU that
//  owns 0x0045d530..0x0045f6a0 -- PFormat::Init 0x0045f0e0, the
//  constructor 0x0045f390 and Convert 0x0045f3d0 sit between them). They
//  are written here, against this file's views of the same classes, only
//  because this is the file that names them.
//
// ======================================================================= //

// 0x0045f160, __cdecl, three arguments. Counts the mask's trailing zeros
// into *pLow (0x0045f16a..0x0045f186) and then carries on over the set bits
// into *pHigh, starting from *pLow (0x0045f188..0x0045f1a1). Not written.
void pf_GetMaskBits(uint32 mask, uint32* pHigh, uint32* pLow);

// 0x0045f120, 54 bytes. __cdecl (PFormat::Init at 0x0045f0e0 pushes i and
// this and pops 8 itself, 0x0045f109..0x0045f110). One colour plane's width
// and shift, from its mask. The two out-values live in the dead parameter
// slots (0x0045f12a / 0x0045f12e), as this file's header comment predicts.
void pf_InitPlane(PFormat* pFormat, uint32 iPlane)
{
	uint32 high, low;

	pf_GetMaskBits(pFormat->m_Masks[iPlane], &high, &low);

	pFormat->m_nBits[iPlane] = high - low;
	pFormat->m_nShifts[iPlane] = low;
}

// Codes 4, 5 and 6 are the block-compressed formats: they need no pitch and
// their extents must be multiples of four (0x0045f2c1..0x0045f318).
#define FM_ISBLOCKFORMAT(code) ((code) == 4 || (code) == 5 || (code) == 6)

// 0x0045f280, 190 bytes. thiscall, no arguments; Convert (0x0045f3d0)
// refuses the request unless this says yes. +0x20 is the HEIGHT
// (fmconvert.cpp; this file's view calls it m_Unknown20) and +0x0c the
// source palette, demanded for codes 0 and 7 (0x0045f31a..0x0045f32f).
// The codes are read through the pointers at every test, no locals: with
// srcCode/destCode locals VC6 swapped their registers and threaded the
// ==0 test past the ==7 one (178 bytes, measured).
LTBOOL FMConvertRequest::IsValid()
{
	if (!m_pSrcFormat || !m_pSrcData || !m_pDestFormat || !m_pDestData ||
	    m_nWidth == -1 || m_Unknown20 == -1)
		return LTFALSE;

	if (!FM_ISBLOCKFORMAT(m_pSrcFormat->m_nBitDepth) && m_nSrcPitch == -1)
		return LTFALSE;

	if (!FM_ISBLOCKFORMAT(m_pDestFormat->m_nBitDepth) && m_nDestPitch == -1)
		return LTFALSE;

	if (FM_ISBLOCKFORMAT(m_pSrcFormat->m_nBitDepth) ||
	    FM_ISBLOCKFORMAT(m_pDestFormat->m_nBitDepth))
	{
		if (m_nWidth & 3)
			return LTFALSE;
		if (m_Unknown20 & 3)
			return LTFALSE;
	}

	if (m_pSrcFormat->m_nBitDepth == 0 && !m_Unknown0c)
		return LTFALSE;
	if (m_pSrcFormat->m_nBitDepth == 7 && !m_Unknown0c)
		return LTFALSE;

	return LTTRUE;
}

// 0x004acdb8: the 8x8 blitter table, indexed [src code][dest code]. The
// thirteen entries fmconvert.cpp reconstructs are
// `__cdecl LTRESULT (FMConvertMgr*, FMConvertRequest*)`.
typedef LTRESULT (*FMBlitFn)(FMConvertMgr* pMgr, FMConvertRequest* pRequest);
extern FMBlitFn g_FMBlitters[8 * 8];     // 0x004acdb8

// 0x0045f3d0, 84 bytes (the csv's 72 + the 12-byte `gap` row 0x0045f418,
// which is this function's `push esi; push edi; call *eax` tail). thiscall,
// one argument, `ret 0x4`. Refuses an invalid request with LT_ERROR
// (0x0045f3e4), looks the blitter up by `lea (%ebx,%eax,8)` with %ebx the
// DEST code (called first, 0x0045f3f1) and %eax the SRC code (0x0045f3fa),
// answers LT_UNSUPPORTED (0x53, 0x0045f40f) for an empty slot, and otherwise
// returns the blitter's own result (0x0045f41a).
LTRESULT FMConvertMgr::Convert(FMConvertRequest* pRequest)
{
	FMBlitFn fn;

	if (!pRequest->IsValid())
		return LT_ERROR;

	fn = g_FMBlitters[pRequest->m_pSrcFormat->GetType() * 8 +
	                  pRequest->m_pDestFormat->GetType()];
	if (!fn)
		return LT_UNSUPPORTED;

	return fn(this, pRequest);
}

// 0x0045f4f0, 235 bytes. thiscall, called once, from the constructor
// (0x0045f3c0). Nine table pairs, one per channel width n = 0..8: the
// n-bit -> 8-bit table has 2^n entries (the nine pointer stores at
// 0x0045f505..0x0045f561), the 8-bit -> n-bit one 256 (0x0045f5a4..
// 0x0045f5be, the magic multiply being `/ 255`).
void FMConvertMgr::InitTables()
{
	uint32 n, i, max;

	m_pFromTables[0] = &m_TableData[0];
	m_pFromTables[1] = &m_TableData[1];
	m_pFromTables[2] = &m_TableData[3];
	m_pFromTables[3] = &m_TableData[7];
	m_pFromTables[4] = &m_TableData[15];
	m_pFromTables[5] = &m_TableData[31];
	m_pFromTables[6] = &m_TableData[63];
	m_pFromTables[7] = &m_TableData[127];
	m_pFromTables[8] = &m_TableData[255];

	for (n = 0; n < 9; n++)
	{
		max = (1 << n) - 1;

		for (i = 0; i <= max; i++)
		{
			if (max == 0)
				m_pFromTables[n][i] = 0;
			else
				m_pFromTables[n][i] = (uint8)((i * 255) / max);
		}

		for (i = 0; i < 256; i++)
			m_ToTables[n][i] = (uint8)((i * max) / 255);
	}
}


// 0x0045f0e0, 62 bytes. The five arguments straight into +0x04..+0x14,
// then pf_InitPlane (0x0045f120) for each of the four planes.
void PFormat::Init(uint32 nBitDepth, uint32 aMask, uint32 rMask,
                   uint32 gMask, uint32 bMask)
{
	uint32 i;

	m_nBitDepth = nBitDepth;
	m_Masks[0] = aMask;
	m_Masks[1] = rMask;
	m_Masks[2] = gMask;
	m_Masks[3] = bMask;

	for (i = 0; i < 4; i++)
		pf_InitPlane(this, i);
}

// 0x0045f1b0, 27 bytes. 32-bit ARGB, through the vtable (0x0045f1c8).
void PFormat::InitPValue()
{
	Init(TM_BITDEPTH_32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
}

// 0x0045f1e0, 68 bytes. Bit depth and the four masks, in layout order.
LTBOOL PFormat::IsSame(PFormat* pOther)
{
	return m_nBitDepth == pOther->m_nBitDepth &&
	       m_Masks[0] == pOther->m_Masks[0] &&
	       m_Masks[1] == pOther->m_Masks[1] &&
	       m_Masks[2] == pOther->m_Masks[2] &&
	       m_Masks[3] == pOther->m_Masks[3];
}

// 0x0045f230, 11 bytes. The bit depth indexes the bytes-per-pixel table
// fmconvert.cpp names (0x004acd98). Called out of line by the PCX loader
// (cltcursor.cpp, 0x0043dfcb) and the renderer's screen-format code.
extern uint32 g_PFormatBytesPerPixel[];      // 0x004acd98

uint32 PFormat::GetBytesPerPixel()
{
	return g_PFormatBytesPerPixel[m_nBitDepth];
}

// 0x0045f240, 55 bytes. The two embedded formats' implicit constructors
// (the two vptr stores) and then every field.
FMConvertRequest::FMConvertRequest()
{
	m_pSrcFormat = &m_SrcFormat;
	m_pDestFormat = &m_DestFormat;
	m_Unknown0c = 0;
	m_pSrcData = NULL;
	m_nSrcPitch = -1;
	m_pDestData = NULL;
	m_nDestPitch = -1;
	m_nWidth = -1;
	m_Unknown20 = -1;
	m_nHeight = 0;
}

// 0x0045f390, 58 bytes. The first format is the PValue one (0x0045f1b0),
// the second 565 through its vtable (0x0045f3bc), then the tables.
FMConvertMgr::FMConvertMgr()
{
	m_FormatPValue.InitPValue();
	m_Format565.Init(TM_BITDEPTH_565, 0, 0xf800, 0x07e0, 0x001f);
	InitTables();
}
