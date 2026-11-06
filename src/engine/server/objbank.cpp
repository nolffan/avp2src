// server.dll — the per-object-type banks (reconstruction). Build /O1.
//
// Eleven banks sit side by side in .rdata at 0x1004de9c and up, each a
// four-slot vtable of the same shape: destructor, Allocate, Free, Init. Free
// is one function shared by all eleven — a pointer is a pointer, so the
// instantiations fold — while Allocate, Init and the destructor differ only
// in the size they bake in and the constructor they call. That is a template,
// instantiated once per object type.
//
// Which type each one belongs to is settled by the sizes: eleven banks, and
// ltbasedefs.h declares exactly eleven OT_ constants. The order matches, and
// the container's 0x274 is the confirmation — ContainerInstance's own code
// reads its container code at 0x270, so its object cannot be smaller and the
// next size up is the bank's.
//
// The class names below are ours, taken from those OT_ constants; the
// engine's own names for them are not recoverable. What is not a guess is the
// size of each and the constructor each Allocate runs.
//
// A freed object is destructed but not released: the vtable's slot zero is
// called with the "do not free" flag and the memory then goes onto the struct
// bank's free list, which is why an object type's storage never leaves its
// bank once claimed.
#include <new.h>
#include <math.h>
#include "servermgr.h"
#include "objtypes.h"
#include "commonmodel.h"
#include "de_error.h"
#include <string.h>

// PH: the module ends in <string>'s std::ctype<unsigned short>::id guarded
// initializer (server.dll 0x10030eb9: guard 0x1008ec84 bit 0, atexit of the
// empty 0x1002eaeb), as ~95 lithtech.exe TUs and smgr.cpp's do.
#ifdef PRIMAL_HUNT
#include <string>
#endif

// The model constructor clears two eight-byte members with inline `stosd`
// runs (`xor eax,eax; lea &m, %edi; stosd; stosd`, the zero re-materialised
// for each) in BOTH images, which is what `memset` compiles to only when it
// is intrinsic — a probe of every other eight-byte zeroing form in this file
// (a value-initialised member, an assignment from a default-constructed
// temporary, element-by-element stores, a call) reproduces none of it. So it
// needs `#pragma intrinsic(memset)`, which stands just above it. (NOLF's
// sprite constructor had a plain twenty-byte memset that wanted the pragma
// NOT in force, which is why the sprite's constructor still sits above it;
// AvP2's clears those twenty bytes field by field.)

// 0x1003e280 — and this is NOT the CRT's memset. It returns its
// *fill value* (`mov 0x8(%esp),%eax` on the way out) where the CRT's returns
// its destination, and it early-outs on a zero length; it is the engine's own
// fill (name ours).
void* lt_MemSet(void* pDest, int nFill, uint32 nBytes);

// 0x1003d1f0 — hand a struct bank's chunks back (declared in filemgr.cpp's
// neighbourhood; the second argument is accepted and ignored).
void sb_FreeChunks(StructBank* pBank, int bAll);

// 0x1003d240 — add another chunk's worth of records to the free list (name
// ours; it lives outside the reconstructed region). structbank.cpp declares
// the same function.
void sb_Grow(StructBank* pBank, uint32 nCount);

// sb_Allocate now lives in servermgr.h as `inline`, which is where the
// original had it; the local copy this file carried was a workaround for
// its absence.



// The four types with recovered structure come from objtypes.h; the rest
// are still only their size and their constructor.
#define DECLARE_OBJTYPE(name, size)                    \
	class name                                         \
	{                                                  \
	public:                                            \
		name();                                        \
		virtual ~name();                               \
		uint8 m_pad[(size) - 4];                       \
	}


typedef CObjBase CObjNormal;                  // OT_NORMAL, 0x1a8

// The ILTSpriteControl implementation a sprite object carries at its +0x1c0:
// a vptr — the vtable at 0x1004e06c, whose six slots are spritecontrol.cpp's
// six methods (0x10035929 .. 0x10035a7b, in iltspritecontrol.h's order) —
// and a pointer back to the sprite it belongs to. An embedded object of its
// own, not a base: a second base would have to start at 0x1a8.
//
// This is spritecontrol.cpp's SpriteControl, and it carries that class's
// name and method list so that the vtable this file emits for it names the
// real bodies. That file types the back-pointer as its own SpriteInstance
// view; here it is the sprite object this file builds.
class SpriteControl
{
public:
	// Slot zero is NOT a destructor: the sprite's own destructor is eleven
	// bytes (vptr store, tail jump to the base) and destroys nothing, so
	// this object has none to run.
	virtual LTRESULT GetNumAnims(uint32& nAnims);                   // 0x10035929
	virtual LTRESULT GetNumFrames(uint32 iAnim, uint32& nFrames);   // 0x10035940
	virtual LTRESULT GetCurPos(uint32& iAnim, uint32& iFrame);      // 0x1003599a
	virtual LTRESULT SetCurPos(uint32 iAnim, uint32 iFrame);        // 0x100359d8
	virtual LTRESULT GetUnknown1b8(uint32& val);                    // 0x10035a67
	virtual LTRESULT SetUnknown1b8(uint32 val);                     // 0x10035a7b

public:
	class CObjSprite* m_pInstance;           // +0x04
};

// OT_SPRITE, 0x1c8 bytes. The play state spritecontrol.cpp reads out — the
// resource at +0x1a8, the current anim, the current frame and the
// milliseconds into it — is cleared wholesale by one twenty-byte memset
// rather than field by field, which is why the region is one block here and
// not five named pointers; the dword immediately past it starts at -1.
// The AnimTimeRef inside a model's own animation tracker. servercommon.h
// models the same structure as CAnimTimeRef with its constructor declared and
// defined out of line, because that is what the *save* code's copy compiles
// to; here the constructor is expanded inline (`orl $-1,0x1f4` for the
// weight set, then a call to the out-of-line Clear at 0x100175fd), which is
// the SDK's in-class definition. Both shapes ship, so both are written.
// ltanimtracker.h's LTAnimTracker and AnimTimeRef, and commonmodel.h's
// ModelWalker, are the real types for everything below and this file used to
// carry local views of all three. They were folded once objbank.cpp could
// include commonmodel.h: the SDK's LTAnimTracker agrees field for field with
// the view that was here (LTLink at 0x00, the callback, m_pUser1, m_CurKey,
// m_Flags, m_InterpolationMS, AnimTimeRef at 0x1c), its GetNext() is exactly
// the cast the walks below were doing by hand, and its SetupTimeRef is the
// declared name for 0x10001000.
//
void at_Init(LTAnimTracker* pTracker, class Model* pModel,
             uint32 iAnim);

// One of the model's four twenty-byte records at 0x274 (AvP2 0x260; name
// ours): the constructor clears all five dwords, in address order.
struct CModelSlot274Q521
{
	uint32 m_Unknown[5];
};

// OT_MODEL, 0x248 bytes. commonmodel.h models the same object from the
// animation side as ModelInstance and agrees offset for offset: the hook at
// +0x1b4, the tracker at +0x1c0, the extra-tracker list at +0x1f8 and the
// hidden-piece mask at +0x204.
class CObjModel : public CObjBase
{
public:
	CObjModel();                             // 0x10024ea7

	virtual ~CObjModel();

	// 0x10024f3e — the base's slot 0x2c, overridden: the model's own radius,
	// unscaled.
	virtual float _v2c();

	// CObjModel's own four virtuals, at slots 0x38 .. 0x44. CObjBase stops at
	// 0x34 -- the vftables of CObjCamera and CObjCanvas are exactly 0x38
	// long, which is what fixes the base's count at fourteen.
	virtual void   SetupModelWalker(class ModelWalker* pOut);   // 0x10024fda
	virtual void   SetupModelWalkerRefsOnly(class ModelWalker* pOut); // 0x10025035
	virtual LTBOOL FindTracker(LTAnimTracker* pTracker,
	                               LTAnimTracker*** pppLink); // 0x100250d0
	virtual float  GetScaledRadius();                             // 0x10024f4b
	// Three more slots (server.dll vtable 0x10075110 +0x48..+0x50,
	// lithtech.exe 0x004d5e7c; AvP2 1.0.9.6's 0x1006a0f8 slots 18..20,
	// 0x1002d224 / 0x1002d23f / 0x1002d27c): the node transforms the arrays
	// below cache, rebuilt by UpdateTransforms whenever slot 0x50 says they
	// are stale (names ours).
	virtual LTMatrix* GetNodeTransforms();                  // 0x1002f074 / 0x0046cf70
	virtual LTMatrix& GetNodeTransform(uint32 iNode);       // 0x1002f08f / 0x0046cf90
	virtual LTBOOL    AreTransformsValid();                 // 0x1002f0ce / 0x0046d070
	void              UpdateTransforms();                   // 0x1002f211 / 0x0046d2b0

	// Not in the vftable: called directly.
	uint32 GetTrackerCount();                // 0x100250a4
	char*  GetFilename();                    // 0x100250b7

public:
	void*             m_pUnknown1a8;         // 0x1b0  not touched here
	void*             m_pUnknown1ac;         // 0x1b4
	void*             m_pUnknown1b0;         // 0x1b8
	void*             m_pHookFn;             // 0x1bc
	void*             m_pUnknown1b8;         // 0x1c0
	void*             m_pUnknown1bc;         // 0x1c4
	// The sixteen bytes from 0x1c0 are one fill (server.dll's four `stos`
	// at 0x1002edaa, lithtech's 0x0046cb77..; AvP2 1.0.9.6 0x1002cfb2).
	// Then the tracker, its list, and the NoCache arrays whose element
	// sizes are their Init's multipliers: 12 (0x0046ec05; AvP2 0x1002e8c1)
	// and 64 (0x004597e0; AvP2 0x100200ca), and in Primal Hunt a third of
	// 2 between them (0x0046edc5). UpdateTransforms says what they hold:
	// each tracker's current FrameLocator, (PH) each tracker's
	// m_InterpolationMS, and one LTMatrix per node.
	uint32            m_Unknown1c8[2];       // 0x1c8
#ifdef PRIMAL_HUNT
	// PH: a dword of its own, stored by the constructor after the fill;
	// AvP2 1.0.9.6's tracker follows the fill directly (0x1002cfb6).
	uint32            m_Unknown1d0;          // 0x1d0
#endif
	LTAnimTracker     m_AnimTracker;         // 0x1d4 (AvP2 0x1d0)  the SDK's tracker, 0x50 bytes
	LTAnimTracker*    m_pTrackerList;        // 0x224 (AvP2 0x220)
	CMoArray<FrameLocator, NoCache> m_Array228;  // 0x228 (AvP2 0x224)  vtable 0x004d5f00
#ifdef PRIMAL_HUNT
	CMoArray<uint16, NoCache>   m_Array238;  // 0x238  vtable 0x004d5ed0
#endif
	CMoArray<LTMatrix, NoCache> m_Array248;  // 0x248 (AvP2 0x234)  vtable 0x004d5aa8
	uint32            m_Unknown1fc;          // 0x258 (AvP2 0x244)
	uint32            m_Unknown200;          // 0x25c (AvP2 0x248)
	uint32            m_HiddenPieces;        // 0x260 (AvP2 0x24c)
	// Four pointers and four twenty-byte records, cleared together by one
	// loop (0x0046cbe6..0x0046cbfd, server.dll 0x1002edfb..0x1002ee12; AvP2
	// 0x1002cffa..0x1002d011).
	void*             m_pUnknown264[4];      // 0x264 (AvP2 0x250)
	CModelSlot274Q521 m_Slots274[4];         // 0x274 .. 0x2c3 (AvP2 0x260 ..)
	LTVector          m_v2c4;                // 0x2c4 (AvP2 0x2b0)  starts at zero
	float             m_f244;                // 0x2d0 (AvP2 0x2bc)  starts at minus one
	LTVector          m_v2d4;                // 0x2d4 (AvP2 0x2c0)  starts at minus one
	uint32            m_Unknown2e0;          // 0x2e0 (AvP2 0x2cc)  starts at one
	// 0x2e4 bytes in Primal Hunt (CObjectBank<CObjModel>::Init's element
	// size: lithtech.exe 0x004152a0, server.dll 0x10030578), 0x2d0 in AvP2
	// 1.0.9.6 (lithtech.exe 0x00413eb0, server.dll 0x1002e30e).
};

// OT_PARTICLESYSTEM, 0x25c bytes. Two of its pointers start at the address
// of its own storage at +0x1a8 — one `lea`, stored twice — and the two float
// constants that are not zero or one are stored as integer immediates, which
// is the cheaper encoding once `fldz`/`fld1` are unavailable.
class CObjParticleSystem : public CObjBase
{
public:
	CObjParticleSystem();                    // 0x10025229

	// 0x100253d0 — slot 0x20: the base's parameter copy, then a pointer into
	// the manager at its +0x14. A different offset from the line system's
	// +0x30, so the manager hands each type its own sub-object.
	virtual void _v20(class CAttachMgrView* pAttachMgr,
	                  struct ObjectCreateStruct* pStruct);

	virtual ~CObjParticleSystem();          // 0x10025349

public:
	uint8  m_Data1a8[0x1c8 - 0x1a8];         // 0x1b0
	void*  m_p1c8;                           // 0x1d0  -> m_Data1a8
	uint8  m_pad_1d4[0x1e8 - 0x1d4];
	void*  m_p1e0;                           // 0x1e8  -> m_Data1a8
	void*  m_p1e4;                           // 0x1ec  -> pManager + 0x14
	uint32 m_u1e8;                           // 0x1f0
	uint8  m_u1ec;                           // 0x1f4
	uint8  m_u1ed;                           // 0x1f5
	uint8  m_u1ee;                           // 0x1f6
	// AVP2: as Primal Hunt -- the byte at 0x1f7 and the five dwords at
	// 0x1fc..0x20c are cleared by the constructor (PH lithtech 0x0046d88b..
	// 0x0046d8be, server.dll 0x1002f5a6..0x1002f5e2; AvP2 0x004680d4..).
	uint8  m_u1ef;                           // 0x1f7
	uint32 m_u1f0;                           // 0x1f8
	uint32 m_u1f4[5];                        // 0x1fc .. 0x20c
	float  m_f208;                           // 0x210
	float  m_f20c;                           // 0x214
	float  m_f210;                           // 0x218
	float  m_f214;                           // 0x21c  starts at one
	float  m_f218;                           // 0x220
	float  m_f21c;                           // 0x224
	float  m_f220;                           // 0x228
	float  m_f224;                           // 0x22c  starts at one
	uint32 m_u228;                           // 0x230
	uint32 m_u22c;                           // 0x234
	// 0x230 and 0x23c are two LTVectors, not six floats: the destructor
	// resets them through LTVector::Init, whose inlined `this` is the `lea`
	// the shipped code forms before the three stores. They are a bounding
	// box -- reset to +1e37 and -1e37, an empty one.
	LTVector m_vBoxMin;                      // 0x238
	LTVector m_vBoxMax;                      // 0x244
	float  m_f248;                           // 0x250  starts at -500
	float  m_f24c;                           // 0x254  starts at 300
	int    m_n250;                           // 0x258  starts at -1
	int    m_n254;                           // 0x25c  starts at -1
	uint32 m_u258;                           // 0x260
};

// OT_POLYGRID, 0x11f8 bytes — nearly all of which is the grid itself and
// none of which the constructor touches. The link at +0x1e4 is tied off and
// given no data, so it is a list *head* and not the object's own link
// (`mov %eax,0x4(%eax)` then `mov %eax,(%eax)` is CheapLTLink::TieOff, whose
// chained assignment writes m_pNext before m_pPrev).
class CObjPolyGrid : public CObjBase
{
public:
	CObjPolyGrid();                          // 0x100253f0

	virtual ~CObjPolyGrid();

	// CObjPolyGrid's one own virtual, at slot 0x38: its vftable at 0x1004e134
	// is fifteen slots and the two dwords after it are the float constants
	// this uses.
	virtual float GetBoundingRadius();       // 0x10025461

public:
	void*  m_pUnknown1a8;                    // 0x1b0
	void*  m_pUnknown1ac;                    // 0x1b4
	void*  m_pUnknown1b0;                    // 0x1b8
	// AVP2: as Primal Hunt -- five dwords the constructor clears (server.dll
	// 0x1002f75f..0x1002f77d, lithtech 0x0046da7c..0x0046da94).
	uint32 m_Unknown1bc[5];                  // 0x1bc .. 0x1cc
	uint32 m_Unknown1c8;                     // 0x1d0
	float  m_f1cc;                           // 0x1d4
	float  m_f1d0;                           // 0x1d8
	float  m_f1d4;                           // 0x1dc  starts at one
	float  m_f1d8;                           // 0x1e0  starts at one
	void*  m_pUnknown1dc;                    // 0x1e4
	void*  m_pUnknown1e0;                    // 0x1e8
	LTLink m_Link1e4;                        // 0x1ec  a tied-off head
	uint32 m_Unknown1f0;                     // 0x1f8
	uint32 m_Unknown1f4;                     // 0x1fc
	// AVP2: as Primal Hunt -- cleared whole by the constructor (server.dll
	// `push 0x1000; call memset` at 0x1002f79a, lithtech `rep stos` of 0x400
	// dwords at 0x0046dabd).
	uint8  m_Grid200[0x1000];                // 0x200
};

class CObjSprite : public CObjBase
{
public:
	CObjSprite();                            // 0x100250cc

	// CObjSprite's one own virtual, at slot 0x38 of its vftable at
	// 0x1004e030; the 363.0f it multiplies by sits just past the sprite
	// control's vftable that follows.
	virtual float GetScaledRadius();         // 0x1002511f

	virtual ~CObjSprite();

public:
	uint32             m_n1a8;                     // 0x1b0
	uint32             m_n1ac;                     // 0x1b4
	uint32             m_n1b0;                     // 0x1b8
	uint32             m_n1b4;                     // 0x1bc
	uint32             m_n1b8;                     // 0x1c0
	int                m_n1bc;                     // 0x1c4  starts at -1
	SpriteControl      m_Control;                  // 0x1c8
};

// Two more banks over things that are not object types at all — the same
// template, reached through vtables of its own at 0x1004e2f0 and 0x1004e358.
// The larger was 0x54 bytes in NOLF, which is sizeof(ServerMessage); Primal
// Hunt's Init passes 0x60 (0x00418d9e `push $0x60`, server.dll 0x10038c81),
// and the name keeps NOLF's size. What the smaller holds is not recoverable
// from the bank alone (names ours).
// Neither is polymorphic — which the release says, not the bank: the eleven
// object types are destructed through their vtables, the 0x54 element by a
// direct call, and the 0x14 element not at all.
class CBankElem54
{
public:
	virtual ~CBankElem54();
#ifdef PRIMAL_HUNT
	uint8 m_pad[0x60 - 4];
#else
	// AvP2 1.0.9.6's Init passes 0x5c (server.dll 0x10035d82).
	uint8 m_pad[0x5c - 4];
#endif
};

class CBankElem14
{
public:
	uint8 m_pad[0x14];
};

// Five more banks, found by walking `.rdata` for the four-slot vtable shape
// (deleting destructor, Allocate, Free, Init) rather than from any call site.
// Each is a distinct instantiation in the shipped image with a vtable of its
// own, and the element size is baked into its Init's `sb_Init` — which is
// what makes them separable from one another at all, since everything else
// in a bank is either shared or relocation:
//
//   0x1004e8b4  0x0c  Init 0x10034e7b  dtor 0x10034e96
//   0x1004d638  0x18  Init 0x1000dea0  dtor 0x1000debb
//   0x1004d648  0x1c  Init 0x1000ed7a  dtor 0x1000ed95
//   0x1004d8dc  0x20  Init 0x10015b4a  dtor 0x10015b9d
//   0x1004dd8c  0x70  Init 0x10024242  dtor 0x10024291
//
// The first four share the type-independent Free at 0x10015ac1 and the same
// Allocate (0x1000de9b, a five-byte tail jump onto 0x1000ed6f), so their
// elements have neither constructor nor destructor and are modelled as raw
// bytes. The 0x70 bank has an Allocate and a Free of its own (0x10023fd6 /
// 0x10023fdb), which says that element *is* non-trivial — so only its Init
// is reconstructed by the instantiation below, and its Allocate and Free
// remain open work.
//
// What the elements are is not recoverable from the banks; the names, as
// with CBankElem14 and CBankElem54 above, are the sizes.
class CBankElem0c
{
public:
	uint8 m_pad[0x0c];
};

class CBankElem18
{
public:
	uint8 m_pad[0x18];
};

class CBankElem1c
{
public:
	uint8 m_pad[0x1c];
};

class CBankElem20
{
public:
	uint8 m_pad[0x20];
};

class CBankElem70
{
public:
	uint8 m_pad[0x70];
};

// A second bank template, found the same way: 0x1004d7d8 (0x0c) and
// 0x1004d250 (0x24) have the same four slots, but their Init brackets the
// two bank calls with LCriticalSection::Enter/Leave (0x100155aa /
// 0x100155b2) on an object at the bank's own +0x20 — exactly where
// CObjectBank leaves m_Unknown20 — and their destructors run
// ~LCriticalSection (0x100155a2) there as well. So the plain bank's spare
// word is this one's lock, and the two templates are otherwise the same
// shape.
//
// The 0x24 one is already written, by hand and under its real name:
// fsstream.cpp's CStreamBank, whose element is a CTreeStream. That is the
// independent check on this shape — the template instantiated at 0x24 comes
// out byte-for-byte as that file's hand-written class — so only the 0x0c
// instantiation is added here, for 0x1001412d.
//
// The lock is lthread.h's LCriticalSection (servermgr.h already brings it
// in): Enter, Leave and the destructor are the three calls named above.


// The interface the banks are reached through. It carries nothing: the
// concrete bank restores this vtable on the way out of its own destructor,
// which is what says it is here at all, and every slot is pure.
//
// NOT a template, and the teardown at 0x1002ca7c is what proves it: that
// function expands eleven bank destructors and restores this vftable after
// each one from a REGISTER, in two bytes, which it can only do if all eleven
// restores name one symbol. Templated here, ours were eleven and each cost a
// six-byte immediate -- 322 bytes against 285. objtypes.cpp reaches the same
// four slots through its own non-templated view, IObjectBank, and names the
// second one Allocate returning CObjBase*, which is the concrete type that
// side needs; void* is the generic one, because CNetBank, CMessageBank and
// CFileBank restore this same vftable and hand out net packets, messages and
// file records rather than objects.
class CObjectBankBase
{
public:
	virtual ~CObjectBankBase() {}
	virtual void* Allocate() = 0;
	virtual void  Free(void* pObj) = 0;
	virtual void  Init() = 0;
};

// Each object type has a destructor of its own; the deleting form the bank
// reaches through slot zero is what these emit, and it is identical for all
// eleven but for which body it calls.
CObjModel::~CObjModel() {}

// 0x10001000 — the generated copy assignment, reached through a member.
void LTAnimTracker::SetupTimeRef(AnimTimeRef* pOut)
{
	*pOut = m_TimeRef;
}

// 0x10024f3e — slot 0x2c: the model's own radius, straight off the model the
// tracker's time reference points at.
float CObjModel::_v2c()
{
	return m_AnimTracker.m_TimeRef.m_pModel->m_f0a8;
}

// 0x10024f4b — slot 0x44: the same radius scaled by the object's largest
// scale component.
//
// The max is LTMAX nested, not a three-way compare, and the shipped bytes
// prove it: `LTMAX(m_Scale.y, m_Scale.z)` is computed TWICE, once for the
// comparison against x and once again in the arm where x loses. That is what
// the macro expansion does, since the outer LTMAX names its second argument
// in both arms of its own ternary. A helper function or a temporary would
// each have computed it once.
float CObjModel::GetScaledRadius()
{
	return LTMAX(m_Scale.x, LTMAX(m_Scale.y, m_Scale.z))
	       * m_AnimTracker.m_TimeRef.m_pModel->m_f0a8;
}

// 0x10024fda — slot 0x38: copy every extra tracker's time reference into the
// gather buffer, then the two fields at 0xec and 0xf0.
//
// Overflowing the buffer returns EARLY, so the eight-reference case skips the
// two trailing fields entirely -- the shipped `jae` goes to the epilogue and
// the exhausted-list path falls into the copies instead.
void CObjModel::SetupModelWalker(ModelWalker* pOut)
{
	LTAnimTracker* pTracker;

	pOut->m_Unknown_e0 = 0;

	for (pTracker = m_pTrackerList; pTracker;
	     pTracker = pTracker->GetNext())
	{
		if (pOut->m_Unknown_e0 >= 8)
			return;

		pTracker->SetupTimeRef(&pOut->m_TimeRefs[pOut->m_Unknown_e0]);
		pOut->m_Unknown_e0++;
	}

	pOut->m_Unknown_ec = m_Unknown1fc;
	pOut->m_Unknown_f0 = m_Unknown200;
}

// 0x10025035 — slot 0x3c: the same gather without the two trailing fields,
// so here the overflow and the exhausted list share one exit.
void CObjModel::SetupModelWalkerRefsOnly(ModelWalker* pOut)
{
	LTAnimTracker* pTracker;

	pOut->m_Unknown_e0 = 0;

	for (pTracker = m_pTrackerList; pTracker;
	     pTracker = pTracker->GetNext())
	{
		if (pOut->m_Unknown_e0 >= 8)
			return;

		pTracker->SetupTimeRef(&pOut->m_TimeRefs[pOut->m_Unknown_e0]);
		pOut->m_Unknown_e0++;
	}
}

// 0x10025074 — slot 0x40: find a tracker in the extra-tracker list and hand
// back the address of the POINTER that reaches it, which is what an unlink
// needs. The walk therefore starts at &m_pTrackerList rather than at the
// first tracker, and the out parameter is written before each hop, so a
// caller that gets LTFALSE still holds the address of the last link.
LTBOOL CObjModel::FindTracker(LTAnimTracker* pTracker,
                                  LTAnimTracker*** pppLink)
{
	// The guard tests POSITIVELY and the failure return is the last thing in
	// the function, because the empty-list guard and the walked-off-the-end
	// case share one `xor %eax,%eax`. Written as an early `if (!list) return
	// LTFALSE;` VC6 hosts that block at its first mention instead and the
	// initial branch inverts -- same size, same instruction count, different
	// bytes.
	if (m_pTrackerList)
	{
		LTAnimTracker** ppCur = &m_pTrackerList;

		for (;;)
		{
			LTAnimTracker* pCur;

			*pppLink = ppCur;

			pCur = *ppCur;

			if (!pCur)
				break;
			if (pCur == pTracker)
				return LTTRUE;

			ppCur = (LTAnimTracker**)&pCur->m_Link.m_pNext;
		}
	}

	return LTFALSE;
}

// 0x100250a4 — how many extra trackers the model carries.
uint32 CObjModel::GetTrackerCount()
{
	uint32 nCount = 0;
	LTAnimTracker* pTracker = m_pTrackerList;

	while (pTracker)
	{
		pTracker = pTracker->GetNext();
		nCount++;
	}

	return nCount;
}

// The cache of node transforms the model keeps (none of this is in NOLF).
// Placed in the shipped order: server.dll 0x1002f074 .. 0x1002f38e and
// lithtech.exe 0x0046cf70 .. 0x0046d647, between GetTrackerCount and
// GetFilename; AvP2 1.0.9.6's server.dll 0x1002d224 .. 0x1002d4c4, which
// has no interpolation array (Primal Hunt's m_Array238).

// 0x1008cbb8 / 0x004f5788 -- what an out-of-range node gets: set to the
// identity every time it is handed out (name ours).
static LTMatrix g_mOutOfRangeQ521;

// Signature of the hook at 0x258 (m_Unknown1fc): cdecl, four arguments, the
// last the dword at 0x25c (0x1002f1a6..0x1002f1b5).
// Primal Hunt's model walker, as UpdateTransforms builds it: 0x2a8 bytes
// (its frame is `sub $0x330`, the walker at -0x330 and the two matrices above
// it), eight AnimTimeRefs of 0x24 and then commonmodel.h's ModelWalker
// fields at 0x120.. -- +0x124 the object transform, +0x128 the output
// array, +0x134 the model. The constructor is INLINE: lithtech.exe expands
// it into UpdateTransforms (the eight-record loop at 0x0046d2f0), server.dll
// calls the out-of-line copy (0x1001d477: the eight records, then 124, 128,
// 12c, 130, 134 and 120 last). A view (name ours) until commonmodel.h's
// ModelWalker is brought up to Primal Hunt's.
class ModelWalkerQ521
{
public:
	ModelWalkerQ521()
	{
		m_pObjTransform = LTNULL;
		m_Unknown_e8 = 0;
		m_Unknown_ec = 0;
		m_Unknown_f0 = 0;
		m_Unknown_f4 = 0;
		m_Unknown_e0 = 0;
	}

	LTBOOL sub_100386c6();                   // 0x1004879f / NOLF 0x100386c6

	AnimTimeRef m_TimeRefs[8];               // 0x000 .. 0x120
	uint32     m_Unknown_e0;                 // 0x120
	LTMatrix*  m_pObjTransform;              // 0x124
	uint32     m_Unknown_e8;                 // 0x128
	uint32     m_Unknown_ec;                 // 0x12c
	uint32     m_Unknown_f0;                 // 0x130
	uint32     m_Unknown_f4;                 // 0x134
	uint8      m_pad_138[0x2a8 - 0x138];
};

typedef void (*ModelHookFnQ521)(CObjModel* pModel, int iNode, LTMatrix* pMat,
                                uint32 nUser);

static LTBOOL ml_FrameLocatorDiffersQ521(FrameLocator* pA, FrameLocator* pB);

// slot 0x48 -- the whole array, brought up to date first.
LTMatrix* CObjModel::GetNodeTransforms()
{
	if (!AreTransformsValid())
		UpdateTransforms();

	return m_Array248.GetArray();
}

// slot 0x4c -- one node's transform, or the identity past the end.
LTMatrix& CObjModel::GetNodeTransform(uint32 iNode)
{
	if (!AreTransformsValid())
		UpdateTransforms();

	if (iNode < m_Array248.GetSize())
		return m_Array248.GetArray()[iNode];

	g_mOutOfRangeQ521.Identity();
	return g_mOutOfRangeQ521;
}

// slot 0x50 -- are the cached transforms still the ones the trackers and the
// hook would produce? A pending invalidation at 0x2e0 is consumed here.
//
// The memcmp's answer is a named local: only then does lithtech.exe's /O2
// expand it as the general byte compare with its signed result (`repz
// cmpsb; sbb; sbb`, 0x0046d23e); tested in place against zero (with or
// without `!= 0`) it becomes the dword compare.
//
// ODDITY KEPT: a tracker-count mismatch on the 0x238 array RESIZES it
// (0x1002f12a..0x1002f13d, the same SetSize2 UpdateTransforms makes) rather
// than answering no, where the 0x228 array's mismatch answers no.
LTBOOL CObjModel::AreTransformsValid()
{
	LTAnimTracker* pTracker;
	uint32         i;

	if (m_Unknown2e0)
	{
		m_Unknown2e0 = 0;
		return LTFALSE;
	}

	if (m_Array248.GetSize() != m_AnimTracker.m_TimeRef.m_pModel->m_NodeTransforms.GetSize())
		return LTFALSE;

	if (m_Array228.GetSize() != GetTrackerCount())
		return LTFALSE;

#ifdef PRIMAL_HUNT
	if (m_Array238.GetSize() != GetTrackerCount())
		m_Array238.SetSize(GetTrackerCount());
#endif

	for (pTracker = m_pTrackerList, i = 0; pTracker;
	     pTracker = pTracker->GetNext(), i++)
	{
		// The answer is tested as a BYTE (`test %al,%al`, 0x1002f164) while
		// the callee sets all of %eax (`xor %eax,%eax` / `push 1; pop %eax`):
		// an LTBOOL narrowed at the call. A bool or uint8 return type changes
		// the callee instead (`xor %al,%al` / `mov $1,%al`).
		if ((uint8)ml_FrameLocatorDiffersQ521(&pTracker->m_TimeRef.m_Cur,
		                                      &m_Array228[i]))
			return LTFALSE;

#ifdef PRIMAL_HUNT
		if (pTracker->m_InterpolationMS != m_Array238[i])
			return LTFALSE;
#endif
	}

	if (m_Unknown1fc)
	{
		LTMatrix mIdent;
		LTMatrix mHook;
		int      nDiff;

		mIdent.Identity();
		mHook = mIdent;

		((ModelHookFnQ521)m_Unknown1fc)(this, -1, &mHook, m_Unknown200);

		nDiff = memcmp(&mIdent, &mHook, sizeof(LTMatrix));
		if (nDiff)
			return LTFALSE;
	}

	return LTTRUE;
}

// 0x1002f1e0 / 0x0046d270 -- cdecl: nonzero when any of the four fields
// differs.
static LTBOOL ml_FrameLocatorDiffersQ521(FrameLocator* pA, FrameLocator* pB)
{
	if (pA->m_iAnim == pB->m_iAnim &&
	    pA->m_iFrame == pB->m_iFrame &&
	    pA->m_Time == pB->m_Time &&
	    pA->m_iWeightSet == pB->m_iWeightSet)
		return LTFALSE;

	return LTTRUE;
}

// Rebuild the cache: size the arrays to the model and the trackers, record
// each tracker's frame, run the walker over the node transforms, and with a
// hook installed take the object's own transform back out of every node.
//
// server.dll: BYTE-IDENTICAL. lithtech.exe: NEAR (925 against 920): the
// shipped /O2 build CALLS AnimTimeRef's constructor for each of the walker's
// eight records (0x0046d3d8, 0x004543d0) where ours expands it, and the
// zero that expansion hoists into %ebp turns the shipped `push $0` /
// `movl $0` immediates into register operands. An extra struct level round
// the eight records does not move it.
void CObjModel::UpdateTransforms()
{
	LTAnimTracker* pTracker;
	uint32         i;
	LTMatrix       mTemp;
	LTMatrix       mBase;

	if (m_Array248.GetSize() != m_AnimTracker.m_TimeRef.m_pModel->m_NodeTransforms.GetSize())
		m_Array248.SetSize(m_AnimTracker.m_TimeRef.m_pModel->m_NodeTransforms.GetSize());

	if (m_Array228.GetSize() != GetTrackerCount())
		m_Array228.SetSize(GetTrackerCount());

#ifdef PRIMAL_HUNT
	if (m_Array238.GetSize() != GetTrackerCount())
		m_Array238.SetSize(GetTrackerCount());
#endif

	for (pTracker = m_pTrackerList, i = 0; pTracker;
	     pTracker = pTracker->GetNext(), i++)
	{
		m_Array228[i] = pTracker->m_TimeRef.m_Cur;
#ifdef PRIMAL_HUNT
		m_Array238[i] = pTracker->m_InterpolationMS;
#endif
	}

	// The base through a pointer: AvP2 1.0.9.6's cl 9044 then forms its
	// address once ahead of the branch (server.dll 0x1002d404), where
	// `&mBase` in each arm forms it twice; Primal Hunt's 8966 builds are
	// the same either way.
	{
		LTMatrix* pBase = &mBase;

		if (m_Unknown1fc)
			GetTransform(pBase);
		else
			pBase->Identity();
	}

	ModelWalkerQ521 walker;

	walker.m_pObjTransform = &mBase;
	walker.m_Unknown_e8    = (uint32)m_Array248.GetArray();
	walker.m_Unknown_f4    = (uint32)this;

	SetupModelWalker((ModelWalker*)&walker);

	if (walker.sub_100386c6() && m_Unknown1fc && mBase.Inverse())
	{
		for (i = 0; i < m_Array248.GetSize(); i++)
		{
			MatMul(&mTemp, &mBase, &m_Array248.GetArray()[i]);
			m_Array248.GetArray()[i] = mTemp;
		}
	}
}

// 0x100250b7 — the model file's name, or the shared empty string when there
// is no model. A tail call into Model::GetFilename.
char* CObjModel::GetFilename()
{
	Model* pModel = m_AnimTracker.m_TimeRef.m_pModel;

	// Positive test again: the tail jump into Model::GetFilename is the
	// FALLTHROUGH and the empty string is hosted after it.
	if (pModel)
		return pModel->GetFilename();

	return "";
}


CObjSprite::~CObjSprite() {}

// 0x100250cc — three of the base's four colour stores again (not the alpha,
// exactly as CObjWorldModel::InitFields repeats them), the play state
// cleared, and the control object pointed back at its owner.
//
// The twenty bytes after the colours are five dwords cleared one by one in
// AvP2: both Primal Hunt images store them by displacement off the object
// (server.dll 0x1002f3d6.., lithtech 0x0046d69a..), where NOLF's engine
// cleared them with one memset (a call in its server.dll).
CObjSprite::CObjSprite() : CObjBase(OT_SPRITE)
{
	m_ColorB = 255;
	m_ColorG = 255;
	m_ColorR = 255;

	m_n1a8 = 0;
	m_n1ac = 0;
	m_n1b0 = 0;
	m_n1b4 = 0;
	m_n1b8 = 0;

	m_n1bc = -1;

	m_Control.m_pInstance = this;
}


#pragma intrinsic(memset)

// 0x10024ea7.
//
// NEAR MATCH (151 of 151 bytes, 44 of 44 instructions): the whole function
// agrees instruction for instruction, and the only difference is where the
// first block fill's address materialises -- the shipped build emits
// `lea 0x208(%esi),%edi` / `xor %eax,%eax` immediately before the last
// tracker store, ours hoists the pair to the moment %edi falls dead and
// emits it in the other order.
//
// The store order is lithtech.exe's (0x0045b1f2: 1fc 200 204 1b4, THEN the
// tracker list at 1f8, then the link); server.dll's /O1 schedules the 1f8
// store up to the call (0x10024ef0), so it is no witness. lithtech.exe also
// says what is still missing: its 208 fill stores through `0x208(%esi)`
// directly (0x0045b21e) where ours goes through a `lea` into %ecx, and VC6
// only folds an intrinsic memset's address when the member was referenced
// earlier in the function -- delete the early `m_pUnknown1b8 = LTNULL` and
// the 1b8 fill grows a `lea` too; add an early `m_pUnknown208 = LTNULL` and
// lithtech's tail becomes byte-identical plus that one extra 6-byte store.
// So the original touched m_pUnknown208 before this point in some way that
// emits no code. Not found: a discarded read, an empty `if`, a redundant
// store beside the fill (kept, not eliminated), an array or struct type for
// the pair, a trivially-constructed member type, and moving the fill
// earlier, all leave it as it is or worse. Also unchanged on both images:
// `&m_pUnknown208` taken into a dead local before at_Init, the fill through
// a `void**` local, `m_pUnknown208 = m_pUnknown208`, `(char*)this + 0x208`,
// `&m_HiddenPieces + 1`, `&this->m_...`, and `sizeof(void*) * 2`; the 208
// fill at each of the six other positions after at_Init is worse on
// server.dll (13-22 differing instructions or 152/153 bytes) and still
// `lea`s on lithtech.exe; the tracker and link stores through an
// `LTAnimTracker*` local are 145 / 196 bytes.
// PH: lithtech.exe 0x0046cac0 (459 bytes, /GX), server.dll 0x1002ed32 (313);
// AvP2 1.0.9.6: server.dll 0x1002cf5b (261), the same body without 0x1d0
// and the interpolation array.
// The members construct first -- the tracker's AnimTimeRef (server.dll calls
// it, 0x1002ee6b), then the three arrays (exception states 1, 2, 3) -- and
// the body runs in the order server.dll's /O1 keeps: the four pointers and
// the sixteen-byte fill from 0x1c0, 0x1d0, the tracker and its link, the
// three counts, the slot loop, and the two vectors built as temporaries
// around the minus one at 0x2d0.
//
// server.dll: BYTE-IDENTICAL, and so is the destructor (0x1002ef14; and
// lithtech.exe's 0x0046cd50), which is the three arrays' inline ~CMoArray in
// reverse order and the base's. lithtech.exe: NEAR (517 against 459): the
// shipped /O2 build CALLS each array's Init(0, 0) (0x0046ebd0, 0x0046ed90,
// 0x004597a0, with Term and SetSize expanded inside them) where ours expands
// Init into the constructor and calls its SetSize2 instead.
CObjModel::CObjModel() : CObjBase(OT_MODEL)
{
	uint32 i;

	m_pUnknown1ac = LTNULL;
	m_pUnknown1b0 = LTNULL;
	m_pUnknown1b8 = LTNULL;
	m_pHookFn     = LTNULL;
	memset(&m_pUnknown1b8, 0, 16);
#ifdef PRIMAL_HUNT
	m_Unknown1d0  = 0;
#endif

	at_Init(&m_AnimTracker, LTNULL, LTFALSE);

	m_pTrackerList = &m_AnimTracker;

	m_AnimTracker.m_Link.m_pNext = LTNULL;
	m_AnimTracker.m_Link.m_pData = this;

	m_Unknown1fc   = 0;
	m_Unknown200   = 0;
	m_HiddenPieces = 0;

	for (i = 0; i < 4; i++)
	{
		m_pUnknown264[i] = LTNULL;
		m_Slots274[i].m_Unknown[0] = 0;
		m_Slots274[i].m_Unknown[1] = 0;
		m_Slots274[i].m_Unknown[2] = 0;
		m_Slots274[i].m_Unknown[3] = 0;
		m_Slots274[i].m_Unknown[4] = 0;
	}

	m_v2c4 = LTVector(0.0f, 0.0f, 0.0f);
	m_f244 = -1.0f;
	m_v2d4 = LTVector(-1.0f, -1.0f, -1.0f);
	m_Unknown2e0 = 1;
}

// 0x10025229 (NOLF); Primal Hunt's server.dll 0x1002f528, 296 bytes,
// BYTE-IDENTICAL: /O1 keeps the x87 stores in source order (218 214 210, the
// one at 21c, 228 224 220, the one at 22c, then the two boxes z,y,x), which
// is what fixes the order of the float statements; the integer stores are
// placed to match. lithtech.exe's /O2 copy (0x0046d830, 274 bytes) is a
// size-exact NEAR: it groups all six 0xff byte stores and both 1.0 stores
// ahead of the zeros and leaves the -1 pair last, where ours schedules the
// 1f4/94..96 group and the constants after the zeros.
CObjParticleSystem::CObjParticleSystem() : CObjBase(OT_PARTICLESYSTEM)
{
	m_p1e0 = m_Data1a8;
	m_p1c8 = m_Data1a8;

	m_p1e4 = LTNULL;
	m_u1e8 = 0;

	m_u1ee = 255;
	m_u1ed = 255;
	m_u1ec = 255;

	m_ColorB = 255;
	m_ColorG = 255;
	m_ColorR = 255;

	m_u1ef = 0;
	m_u1f0 = 0;
	m_u1f4[0] = 0;
	m_u1f4[1] = 0;
	m_u1f4[2] = 0;
	m_u1f4[3] = 0;
	m_u1f4[4] = 0;

	m_f210 = 0.0f;
	m_f20c = 0.0f;
	m_f208 = 0.0f;
	m_f214 = 1.0f;

	m_f220 = 0.0f;
	m_f21c = 0.0f;
	m_f218 = 0.0f;
	m_f224 = 1.0f;

	m_u228 = 0;
	m_u22c = 0;

	m_vBoxMin.z = 0.0f;
	m_vBoxMin.y = 0.0f;
	m_vBoxMin.x = 0.0f;

	m_vBoxMax.z = 0.0f;
	m_vBoxMax.y = 0.0f;
	m_vBoxMax.x = 0.0f;

	m_u258 = 0;

	m_f248 = -500.0f;
	m_f24c = 300.0f;

	m_n254 = -1;
	m_n250 = -1;
}

// 0x100253f0 (NOLF); Primal Hunt's server.dll 0x1002f71b and lithtech.exe
// 0x0046da40.
//
// lithtech.exe: BYTE-IDENTICAL (155 bytes; the grid is an inline `rep stos`
// at 0x0046dad4). server.dll: NEAR (160 against 164). Its /O1 CALLS the
// CRT's memset for the grid (0x1002f7b3) with no exception frame, where the
// `#pragma intrinsic(memset)` above (which the model constructor's 16-byte
// fill needs, 0x1002edaa) expands it here too. Probed: once memset has been
// made intrinsic ANYWHERE in a /GX translation unit, a call to it is treated
// as throwing -- defining this above the pragma, or `#pragma function` after
// it, both give the call but with an EH frame (192 bytes). So the shipped
// file cannot have used the pragma; the model's stosd fill then comes from
// some other construct not yet found.
//
// The float statements keep the order server.dll's x87 stream has (/O1 does
// not reorder FP stores among themselves): 1d8, 1d4, then the two ones at
// 1e0 and 1dc. The integer clears follow in address order, the link is tied
// off after them (0x1002f795) and the grid's 0x1000 bytes are cleared last.
CObjPolyGrid::CObjPolyGrid() : CObjBase(OT_POLYGRID)
{
	m_pUnknown1a8 = LTNULL;
	m_pUnknown1ac = LTNULL;
	m_pUnknown1b0 = LTNULL;
	m_Unknown1bc[0] = 0;
	m_Unknown1bc[1] = 0;
	m_Unknown1bc[2] = 0;
	m_Unknown1bc[3] = 0;
	m_Unknown1bc[4] = 0;
	m_Unknown1c8  = 0;

	m_f1d0 = 0.0f;
	m_f1cc = 0.0f;
	m_f1d8 = 1.0f;
	m_f1d4 = 1.0f;

	m_pUnknown1dc = LTNULL;
	m_pUnknown1e0 = LTNULL;

	m_Link1e4.TieOff();

	m_Unknown1f4  = 0;
	m_Unknown1f0  = 0;

	memset(m_Grid200, 0, sizeof(m_Grid200));
}

// One particle as the destructor reads it: the bank's intrusive free link at
// +0x00 and the next-in-list at +0x20 (name ours).
struct CParticle
{
	void*            m_pNextFree;            // +0x00  sb_Free's link
	uint8            m_pad_04[0x20 - 0x04];
	struct CParticle* m_pNext;               // +0x20
};

// 0x10025349 — the line system's destructor again, on a different type.
//
// Every particle goes back to a bank, the list terminator is the ADDRESS of
// the object's own storage rather than a null, and both cursors are reset to
// it afterwards. What differs is where the bank comes from: the line system
// keeps its own pointer at 0x1a8, and this one recomputes m_pAttachMgr + 0x14
// each time round -- the same +0x14 its _v20 stored into m_p1e4, which it does
// not use here.
//
// The whole walk is guarded on m_pAttachMgr, so a particle system whose
// creation never ran leaks nothing and crashes on nothing.
CObjParticleSystem::~CObjParticleSystem()
{
	m_vBoxMin.Init(1.0e+37f, 1.0e+37f, 1.0e+37f);
	m_vBoxMax.Init(-1.0e+37f, -1.0e+37f, -1.0e+37f);

	m_u1e8 = 0;

	if (m_pAttachMgr)
	{
		CParticle* pSentinel = (CParticle*)m_Data1a8;
		CParticle* pCur = (CParticle*)m_p1c8;

		while (pCur != pSentinel)
		{
			CParticle* pNext = pCur->m_pNext;

			sb_Free((StructBank*)((uint8*)m_pAttachMgr + 0x14), pCur);

			pCur = pNext;
		}
	}

	m_u228 = 0;
	m_p1e0 = m_Data1a8;
	m_p1c8 = m_Data1a8;
}

// 0x100253d0 — slot 0x20.
void CObjParticleSystem::_v20(CAttachMgrView* pAttachMgr,
                              ObjectCreateStruct* pStruct)
{
	CObjBase::_v20(pAttachMgr, pStruct);

	m_p1e4 = (uint8*)pAttachMgr + 0x14;
}

// 0x1002511f — slot 0x38: a sprite's radius. TWO axes, not three -- a sprite
// is flat, so only x and y are consulted, and there is a single LTMAX rather
// than the nested pair CObjModel::GetScaledRadius needs. That also means no
// term is computed twice here.
float CObjSprite::GetScaledRadius()
{
	return LTMAX(m_Scale.x, m_Scale.y) * 363.0f;
}

// 0x10025461 — slot 0x38: the radius of the grid's scaled box, plus one.
//
// The half-extents are the grid's width and height SHIFTED right by one, not
// divided, and both shifts are unsigned -- which is also what proves the two
// fields are uint32: VC6 widens each to a 64-bit temporary with the high dword
// forced to zero and converts with `fildll`, which is exactly its unsigned
// int-to-float sequence. A signed field would have been one `fild`.
//
// The middle axis has no field at all: it is a flat 128.0f times the scale,
// so the grid is a fixed height and only its footprint varies.
//
// The three terms are evaluated x, y, z but SQUARED and summed z, y, x --
// three values live on the x87 stack at once and the first square taken is of
// the last one pushed. Writing the sum in source order costs the match.
//
// The root is a NAMED float local, added to afterwards: that is what lets
// the first of the two `pop ecx` releasing sqrt's double argument land BEFORE
// the `fadds` (0x100254c2). With the add nested in the return expression
// both pops sink after it; the cast's position and a reversed add
// (`1.0f + ...`) do not move it.
float CObjPolyGrid::GetBoundingRadius()
{
	float fx = (float)(m_Unknown1f0 >> 1) * m_Scale.x;
	float fy = m_Scale.y * 128.0f;
	float fz = (float)(m_Unknown1f4 >> 1) * m_Scale.z;

	// The cast is INSIDE the add, not around it: `(float)(sqrt(..) + 1.0f)`
	// promotes the literal to double and gets `faddl`, where the shipped code
	// adds a 32-bit constant.
	float fRadius = (float)sqrt(fz * fz + fy * fy + fx * fx);
	return fRadius + 1.0f;
}

// 0x100254e8 — the grid's two owned buffers go back to the engine allocator
// and the fields are cleared, which is why each release is followed by a
// reload through a `lea` of the field's own address.
CObjPolyGrid::~CObjPolyGrid()
{
	if (m_pUnknown1a8)
	{
		dfree(m_pUnknown1a8);
		m_pUnknown1a8 = LTNULL;
	}

	if (m_pUnknown1ac)
	{
		dfree(m_pUnknown1ac);
		m_pUnknown1ac = LTNULL;
	}
}

CBankElem54::~CBankElem54() {}


// The bank itself: a vptr, a struct bank, and nothing else it owns. The
// free list Free pushes onto is the struct bank's own, at +0x18 of it.
template<class T>
class CObjectBank : public CObjectBankBase
{
public:
	// 0x1002449b builds eleven of these, each one `vptr = <table> ;
	// sb_Init2(&bank, <size>, 0x20, <grow>)`. The cache size is 0x20 every
	// time and the record size is the type's own, so the only thing that
	// varies between the eleven is the grow size -- which makes it the one
	// constructor argument. (That sizeof(T) reproduces all eleven shipped
	// immediates -- 0x1a8, 0x248, 0x270, 0x1c8, 0x1ac, 0x1d0, 0x25c,
	// 0x11f8, 0x21c, 0x274, 0x1b4 -- is an independent check on the eleven
	// object layouts this file already carries.)
	CObjectBank(uint32 nGrowBy)
	{
		sb_Init2(&m_Bank, sizeof(T), 0x20, nGrowBy);
	}

	virtual ~CObjectBank()
	{
		sb_FreeChunks(&m_Bank, 1);
	}

	// The bank hands back raw memory, so the construction is explicit.
	virtual void* Allocate()
	{
		T* pRet = (T*)sb_Allocate(&m_Bank);

		if (pRet)
			new(pRet) T;

		return pRet;
	}

	// Destructed, not released: the flag tells the deleting destructor to
	// leave the memory alone, and it goes back to the bank instead.
	virtual void Free(void* pObj)
	{
		((T*)pObj)->~T();

		sb_Free(&m_Bank, pObj);
	}

	// Starting over drops whatever the bank was holding first, so Init
	// doubles as a reset.
	virtual void Init()
	{
		sb_FreeChunks(&m_Bank, 1);
		sb_Init(&m_Bank, sizeof(T), 0x20);
	}

public:
	StructBank m_Bank;                       // +0x04
	uint32     m_Unknown20;                  // +0x20
};


// The locking bank. Only Init and the destructor are written: its Allocate
// and Free are not the shared ones (0x1001409c / 0x100140a1 and 0x100075b5 /
// 0x100075ba), and the Free forwards to a *cdecl* function of one argument
// that never sees the bank — so whatever these banks hand out is returned to
// something global, and reconstructing that is separate work. Leaving the
// two slots pure keeps the class abstract and emits nothing for them.
template<class T>
class CLockedObjectBank : public CObjectBankBase
{
public:
	virtual ~CLockedObjectBank()
	{
		sb_FreeChunks(&m_Bank, 1);
	}

	virtual void Init()
	{
		m_Lock.Enter();

		sb_FreeChunks(&m_Bank, 1);
		sb_Init(&m_Bank, sizeof(T), 0x20);

		m_Lock.Leave();
	}

public:
	StructBank m_Bank;                       // +0x04
	LCriticalSection m_Lock;                 // +0x20
};


// The eleven instantiations. Where the shipped engine keeps them is not
// visible from here; what matters is that each exists.
//
// Explicit instantiation also emits a non-deleting destructor per bank that
// the shipped image does not have — there the body is only ever inlined into
// the deleting form. Those eleven symbols are ours, not missing work; they
// match no shipped address and score as nothing.
template class CObjectBank<CObjNormal>;
template class CObjectBank<CObjModel>;
template class CObjectBank<CObjWorldModel>;
template class CObjectBank<CObjSprite>;
template class CObjectBank<CObjLight>;
template class CObjectBank<CObjCamera>;
template class CObjectBank<CObjParticleSystem>;
template class CObjectBank<CObjPolyGrid>;
template class CObjectBank<CObjLineSystem>;
template class CObjectBank<CObjContainer>;
template class CObjectBank<CObjCanvas>;
template class CObjectBank<CBankElem54>;
template class CObjectBank<CBankElem14>;
template class CObjectBank<CBankElem0c>;
template class CObjectBank<CBankElem18>;
template class CObjectBank<CBankElem1c>;
template class CObjectBank<CBankElem20>;
template class CObjectBank<CBankElem70>;
template class CLockedObjectBank<CBankElem0c>;





// *********************************************************************** //
//  Three functions from the *neighbouring* translation unit.
//
//  0x1002592e, 0x10025984 and 0x10025a1b sit between un390_Term (0x100257f2,
//  objtypes.cpp) and the bank Free/Init run above (0x10025ae7 and up), so by
//  address they belong to objtypes.cpp — the file that already models the
//  0x390 manager. They are reconstructed here because that file is not ours
//  to edit; the views below duplicate objtypes.cpp's on purpose and should
//  be folded into it when the two files meet.
// *********************************************************************** //

// The four LTObject fields these reach, at their own offsets. The link is
// the one the per-type object lists hold the object on (ltobject_internal.h
// leaves 0x7c inside m_pad_07c), and the type is a *signed* char — both uses
// sign-extend it, and the bound test against eleven is a signed `jge`.
class CObjLinked
{
public:
	uint8             m_pad_00[0x7c];
	LTLink            m_Link7c;              // 0x07c
	uint8             m_pad_88[0x098 - 0x088];
	class Attachment* m_pAttachments;        // 0x098  singly linked
	uint8             m_pad_9c[0x0b8 - 0x09c];
	uint16            m_ObjectID;            // 0x0b8
	uint8             m_pad_ba[0x0bc - 0x0ba];
	char              m_ObjectType;          // 0x0bc  OT_*
};

// The 0x390 manager, as far as these three need it: the eleven bank
// pointers and the eleven per-type lists. Named apart from servermgr.h's
// CUnknown390 (which is a four-byte stand-in) and objtypes.cpp's full view.
// THE TWO TABLES AT OFFSET ZERO, and why neither is a C++ vptr here.
//
// The constructor at 0x1002449b writes offset zero TWICE: 0x1004df68 before
// the eleven banks (0x100244ac) and 0x1004deb0 after them (0x1002460c).
// 0x1004df68 is four slots, the first two of them _purecall (0x1003ea24);
// 0x1004deb0 is two, Lock (0x1002461c) and a getter folded onto
// Model::GetFilename. No derived table is shorter than the one it replaces,
// so these are not a base/derived pair, and the destructor at 0x1002ca7c
// settles it from the other side: a polymorphic class restores its own
// vftable at the top of its destructor, and that function has no store to
// (%esi) at all.
//
// So the pointer is modelled as plain data, the way servermgr.h's
// CUnknown8FC already models its own. The FIRST store still cannot live in
// the constructor's body -- it has to precede eleven member constructors --
// so it goes in a one-member base whose inline constructor is nothing else;
// /O1 implies /Ob1 and expands it in place. Written instead as a real
// virtual on CObjMgrView, VC6 drops the constructor's vptr store as dead
// (the body overwrites it) and keeps the destructor's, which is exactly
// backwards: 379 of 385 and 291 of 285, six bytes each way.
//
// CORRECTION to the slot count above: 0x1004df68 is only TWO slots. The two
// after them (0x10024690, 0x10024afc) start the next table, CObjBase's at
// 0x1004df70, whose constructors store exactly that address. So both tables
// are two slots, pure then {Lock, lock-depth getter}, which does read like
// an abstract base and its implementation; the destructor argument above is
// what still stands against a real vptr. Both tables are defined in
// objtypes.cpp.
extern void* const g_ObjMgrVtblA[];          // 0x1004df68  objtypes.cpp

class CObjMgrVtblBase
{
public:
	CObjMgrVtblBase() { m_pVtbl = (void*)g_ObjMgrVtblA; }

	void*                      m_pVtbl;        // +0x000
};

class CObjMgrView : public CObjMgrVtblBase
{
public:
	CObjMgrView();                             // 0x1002449b

	int32                      m_LockCount;    // +0x004
	LTLink                     m_Link;         // +0x008
	uint8                      m_pad_14[0x4c - 0x14];
	// The 0x28-byte records the attachments come out of; its own intrusive
	// free list at +0x18 (manager +0x64) is where a removed one goes back.
	StructBank                 m_Bank4c;       // +0x04c
	// The eleven per-object-type banks, which used to be a pad here.
	CObjectBank<CObjNormal>         m_Normal;          // 0x068
	CObjectBank<CObjModel>          m_Model;           // 0x08c
	CObjectBank<CObjWorldModel>     m_WorldModel;      // 0x0b0
	CObjectBank<CObjSprite>         m_Sprite;          // 0x0d4
	CObjectBank<CObjLight>          m_Light;           // 0x0f8
	CObjectBank<CObjCamera>         m_Camera;          // 0x11c
	CObjectBank<CObjParticleSystem> m_ParticleSystem;  // 0x140
	CObjectBank<CObjPolyGrid>       m_PolyGrid;        // 0x164
	CObjectBank<CObjLineSystem>     m_LineSystem;      // 0x188
	CObjectBank<CObjContainer>      m_Container;       // 0x1ac
	CObjectBank<CObjCanvas>         m_Canvas;          // 0x1d0
	CObjectBankBase* m_pBanks[11];             // +0x1f4
	LTList                     m_Lists[11];    // +0x220

	~CObjMgrView();                            // 0x1002ca7c
};

// The eleven banks in place, so their destructor can be written.
//
// They are members of the sub-object at CServerMgr+0x390, at +0x68 and every
// 0x24 after it, in OT_ order. objtypes.cpp ALREADY had that -- its own view
// of the same object carries `CObjBankSlot m_Banks[11]; // 0x068` -- so the
// location is confirmation rather than discovery; what the destructor at
// 0x1002ca7c adds is the ORDER (it walks them backwards from 0x1d0 to 0x68,
// which is reverse declaration order) and the fact that they are eleven
// DISTINCT types: each restores a different four-slot vftable, 0x10 apart,
// which an array of one type could not produce and which is why
// objtypes.cpp's opaque CObjBankSlot stand-in is right to be opaque.
//
// A third view rather than an extension of servermgr.h's CUnknown390 stub,
// for two reasons: the bank template lives in this file and servermgr.h
// cannot see it, and this way CServerMgr's own layout is untouched. The
// wide view with the lock, the three struct banks and the eleven per-type
// object lists is objtypes.cpp's (name ours).
// ======================================================================== //
// 0x1002ca7c — the eleven banks handed back. The body is empty; every
// instruction is the compiler expanding CObjectBank's inline destructor
// eleven times, each expansion being its own vftable restore, the
// sb_FreeChunks call, and then CObjectBankBase's vftable restore.
//
// This is what proved CObjectBankBase is not a template. Written against a
// templated base it came out 322 bytes against 285 with 88 instructions
// against 91 -- longer with FEWER instructions, which localised the residual
// completely: the shipped build holds the base vftable's address in a
// register across all eleven expansions and restores it in two bytes, and
// ours spent a six-byte immediate each time because our eleven restores
// named eleven different symbols. It could not be linker folding either,
// since /OPT:ICF is off in server.dll.
// Defined in objtypes.cpp, beside the two functions it points at.
struct CObjMgrVtbl;
extern const CObjMgrVtbl g_ObjMgrVtbl;       // 0x1004deb0

// 0x1002449b -- build the object manager at CServerMgr+0x390.
// BYTE-IDENTICAL (385 of 385). netmgr.cpp used to carry this address as
// NOT RECONSTRUCTED; the block that stood in the way was the pair of tables
// at offset zero, answered above.
//
// It is the mirror of the destructor below and everything that function
// proved holds here: eleven DISTINCT bank types at +0x68 and every 0x24
// after it, constructed in declaration order (the destructor walks the same
// eleven backwards). Only two of the eleven grow by anything but 0x20 --
// models at 0x100 and sprites at 0x80.
//
// The eleven LTLists at +0x220 are a member ARRAY, which is why the shipped
// code has a real down-counting loop (`push $0xb ; pop %edx`, 0x10 stride,
// 0x100245f4) rather than eleven expansions: src/LT2/sdk/inc/ltlink.h already
// gives LTList a default constructor that ties the head off and zeroes the
// count, and an array of eleven of those is exactly one loop. At /O2 the
// same array becomes a call to the vector constructor iterator, which is
// lithtech.exe's 0x00401000 and now matches there too.
//
// The body is three statements, at +0x00, +0x04 and +0x08 in that order.
// The table store is FIRST: with the count first instead, server.dll is
// unchanged (its /O1 schedule is the same either way) but lithtech.exe's /O2
// copy stores the count ahead of the table (0x0045a5fc) and is a near.
CObjMgrView::CObjMgrView()
	: m_Normal(0x20), m_Model(0x100), m_WorldModel(0x20), m_Sprite(0x80),
	  m_Light(0x20), m_Camera(0x20), m_ParticleSystem(0x20), m_PolyGrid(0x20),
	  m_LineSystem(0x20), m_Container(0x20), m_Canvas(0x20)
{
	m_pVtbl = (void*)&g_ObjMgrVtbl;

	m_LockCount = 0;

	m_Link.TieOff();
}

CObjMgrView::~CObjMgrView()
{
}

// 0x1002592e — drop an object out of the manager. Two statements: unlink it
// from its type's list, and give the storage back to that type's bank.
//
// The list base never appears: `(nType + 0x22) * 16` is VC6 folding the
// 0x220 byte offset into the scaled index, which is also what says an
// LTList is sixteen bytes and that the array starts there.
//
// The bound test guards only the Free, so an object whose type is eleven or
// more is unlinked and then leaked. That is what shipped.
LTRESULT un390_RemoveObject(CObjMgrView* p, CObjLinked* pObject)
{
	dl_RemoveAt(&p->m_Lists[pObject->m_ObjectType], &pObject->m_Link7c);

	if (pObject->m_ObjectType < 11)
		p->m_pBanks[pObject->m_ObjectType]->Free((CObjBase*)pObject);

	return LT_OK;
}


// 0x10025a1b — unlink one attachment record from its parent and push it onto
// the manager's spare list. Named by the literal it reports through:
// "ILTServer::RemoveAttachment".
//
// smgr.cpp also carries this function, as a 108-of-103 near written with two
// separate RETURN_ERROR sites — so one of the two definitions has to go when
// the files meet, and this is the one that matches. The shipped code has ONE
// error block, at the
// very end, reached from the "ran off the list" test at both the entry and
// the back edge — which is the `while (pCur)` form below with the found arm
// sunk past it (a shared exit block at the very end means the
// failures were merged, not written out twice).
//
// The record's own free link is its first dword, not m_pNext: the spare list
// threads through offset zero while a live record threads through 0x24.
LTRESULT am_RemoveAttachment(CObjMgrView* pAttachMgr, CObjLinked* pParent,
                             Attachment* pAttachment)
{
	Attachment** ppLink = &pParent->m_pAttachments;
	Attachment*  pCur   = *ppLink;

	while (pCur)
	{
		if (pCur == pAttachment)
		{
			*ppLink = pCur->m_pNext;

			sb_Free(&pAttachMgr->m_Bank4c, pAttachment);

			return LT_OK;
		}

		ppLink = &pCur->m_pNext;
		pCur   = *ppLink;
	}

	RETURN_ERROR(1, "ILTServer::RemoveAttachment", LT_ERROR);
}


// 0x10025984 — hang a child object off a parent. The record comes out of the
// manager's 0x28-byte bank (its m_Bank4c, which is what says sizeof
// (Attachment) is 0x28), goes on the front of the parent's singly linked
// list, and is filled in from the arguments.
//
// A missing record is a bare `return LT_ERROR` with no report — the only
// failure path in the function, and `push $1; pop eax` is all of it.
//
// Both offsets are optional and are handled differently, which is in the
// bytes: a null position leaves the record's twelve bytes as the bank left
// them, while a null rotation is written out as the identity quaternion
// through LTRotation::Init's defaults (fldz/fldz/fldz/fld1, interleaved with
// their stores, which is four assignments and not one Init(x,y,z,w) call).
LTRESULT am_CreateAttachment(CObjMgrView* pAttachMgr, CObjLinked* pParent,
                             uint16 nChildID, int iNode, LTVector* pOffset,
                             LTRotation* pRotOffset, Attachment** ppAttachment)
{
	Attachment* pAttachment = (Attachment*)sb_Allocate(&pAttachMgr->m_Bank4c);

	if (!pAttachment)
		return LT_ERROR;

	pAttachment->m_pNext    = pParent->m_pAttachments;
	pParent->m_pAttachments = pAttachment;

	pAttachment->m_nParentID = pParent->m_ObjectID;
	pAttachment->m_nChildID  = nChildID;
	pAttachment->m_iNode     = iNode;

	if (pOffset)
		pAttachment->m_Offset.m_Pos = *pOffset;

	if (pRotOffset)
		pAttachment->m_Offset.m_Rot = *pRotOffset;
	else
		pAttachment->m_Offset.m_Rot.Init();

	if (ppAttachment)
		*ppAttachment = pAttachment;

	return LT_OK;
}

// The world's segment query (NOLF 0x1000edd7, w_IntersectSegment and its
// tests) moved to worldisect.cpp: Primal Hunt and AvP2 build it as a module
// of its own, unoptimised.
