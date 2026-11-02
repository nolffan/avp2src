// *********************************************************************** //
//
//  MODULE:   W12_428000 (.CPP)  --  scratch reconstruction, lithtech.exe only
//
//  Ground truth throughout is the retail lithtech.exe.  Every offset,
//  constant and branch below is cited by the address it was read at.
//  Symbols this file introduces carry the Q428 suffix.
//
//  BUILD: MSVC 6 rev 8447, /O2 (/Og /Oi /Ot /Oy /Ob1) -- frameless,
//  %esp-relative.  Disassembly read with `objdump -M intel`.
//
//  FOUR UNRELATED TRANSLATION UNITS LIVE IN THIS FILE.  They
//  must be SPLIT; VC6's literal pooling and COMDAT ordering are per-TU.
//  Each part names the src/engine file it belongs in.
//
// ======================================================================= //
//  ROW-BY-ROW VERDICT (10 rows, 743 bytes)
// ======================================================================= //
//
//  addr        sz  prov   verdict
//  ----------  --  -----  --------------------------------------------------
//  0x00428fdd  70  gap    NOT A FUNCTION -- INTERIOR to dalloc() at
//                         0x00428fd0, whose source is ALREADY WRITTEN in
//                         src/engine/server/dalloc.cpp.  EXCLUDE.
//  0x004290e0  54  thunk  ??__E for the 0x18-byte object at 0x004b2ff0.
//                         CWorldItem18 terminator leaf; written in
//                         src/engine/server/dalloc.cpp (see PART D).
//  0x00429130  54  thunk  ??__E for the 0x18-byte object at 0x004b3008.
//                         Same source, second instance.
//  0x00429a20  85  call   CSub11F0Item::SetRect -- PART C.  Extent clean.
//  0x00429fd0 117  call   CSub11F0::GetCellRect -- PART C.  Extent clean.
//  0x0042a100  83  call   CSub11F0::SetRect -- PART C.  Extent clean.
//  0x00489580  47  call   fnt_FreeAllFonts -- PART B.  BELONGS IN
//                         src/engine/lithtech/fntdraw.cpp (PART ONE), which
//                         already carries CFontQ488 / g_pFontListQ488 /
//                         fnt_FreeFontQ488 (0x00488b20).  Extent clean.
//  0x0048b120  49  ptr    SmackVideoInst::Restore   -- PART A.  Slot 2 of
//                         vtable 0x004a38a4.  Extent clean.
//  0x0048b160  73  ptr    SmackVideoInst::FreeSurfaces -- PART A.  Slot 3
//                         of the same vtable.  Extent clean.
//  0x00478930 111  call   ObjectCreateStruct::ObjectCreateStruct() -- the
//                         SDK's OWN in-class constructor (src/LT2/sdk/inc/
//                         ltbasedefs.h:218).  NOT INDEPENDENT WORK: it is a
//                         header COMDAT that appears only in a TU where /Ot
//                         declines to expand it.  See PART E.  EXCLUDE.
//
// ======================================================================= //

#include <windows.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "ltrect.h"
#include "ltlink.h"


// ======================================================================= //
//
//  PART A -- 0x0048b120 and 0x0048b160
//            SmackVideoInst::Restore and SmackVideoInst::FreeSurfaces
//
//  BELONGS IN: the Smack/DirectDraw video TU -- today that is PART TWO of
//  src/engine/lithtech/fntdraw.cpp (0x0048a770..0x0048c19e), which already
//  declares SmackVideoMgrQ488 / SmackVideoInstQ488.  The class VIEW below
//  is binkvideo.cpp's BinkVideoInstK shifted by the 0x40 that separates the
//  two layouts, so if the two files are merged, reuse binkvideo.cpp's
//  SharedTextureK / IDirectDrawSurfaceK verbatim.
//
//  HOW THE TWO SLOTS WERE TYPED -- dispatch table, not the string pool.
//
//   1. The DESTRUCTOR names the vtable.  0x0048ac40 opens with
//      `mov DWORD PTR [esi],0x4a38a4` and closes with
//      `mov DWORD PTR [esi],0x4a147c`, so SmackVideoInst's vptr is
//      0x004a38a4 (NOT 0x004a3890, which a naive vtable scan
//      splits it at -- 0x004a3890 is the five-slot SmackVideoMgr table,
//      whose slots 2/3/4 are byte-identical with BinkVideoMgr's at
//      0x004a1440).  Indexing from 0x004a38a4:
//
//        slot 0  0x0048ac20  scalar deleting destructor
//        slot 1  0x0048b390  Release          (shared with Bink)
//        slot 2  0x0048b120  <-- THIS ROW
//        slot 3  0x0048b160  <-- THIS ROW
//        slot 4  0x0048b270  UpdateScreen
//        slot 5  0x0048b290  Update
//        slot 6  0x0048b2b0  GetStatus
//        slot 7  0x0048b2d0  BindToSurface
//        slot 8  0x0048b1b0  slot 8 (shared with Bink)
//        slot 9  0x0048b230  slot 9 (shared with Bink)
//      0x004a38cc holds 0x5453494c ("LIST"), so the table ends at ten.
//
//   2. The MANAGER'S OWN SLOTS NAME THEM OUTRIGHT.  0x0048a710 and
//      0x0048a740 -- both shared by the Bink and the Smack manager tables --
//      walk the manager's video list and call, respectively,
//      `call DWORD PTR [edx+0x8]` (0x0048a730) and
//      `call DWORD PTR [edx+0xc]` (0x0048a760): instance slots 2 and 3.
//      src/engine/lithtech/lvideomgr.cpp:351-352 already declares those two
//      slots as `virtual LTRESULT Restore()` and
//      `virtual void FreeSurfaces()`, and src/engine/lithtech/binkvideo.cpp
//      :536/547 declares BinkVideoInstK::Restore at 0x00402120 and
//      ::FreeSurfaces at 0x00402150 -- the exact Bink twins of these two
//      bodies.  NEITHER BINK BODY IS WRITTEN YET, so landing these gives
//      the Bink pair for free (see the note at the end of PART A).
//
//   3. The BINK TWIN is the oracle for the source.  0x00402120 is 46 bytes
//      to 0x0048b120's 49; the whole difference is one displacement --
//      `mov eax,DWORD PTR [esi+0x60]` (3 bytes) against
//      `mov eax,DWORD PTR [esi+0xa0]` (6 bytes).  0x00402150 is 67 bytes to
//      0x0048b160's 73; there the difference is the extra second-surface
//      block and the zero register (below).
//
// ----------------------------------------------------------------------- //

// binkvideo.cpp's IDirectDrawSurfaceK, cut to the one slot used here.
// `mov edx,DWORD PTR [eax]; push eax; call DWORD PTR [edx+0x8]` at
// 0x0048b16d..0x0048b170 is IUnknown::Release through a COM vtable.
class IDirectDrawSurfaceQ428
{
public:
	virtual HRESULT __stdcall QueryInterface(void*, void**);   // 0x00
	virtual ULONG   __stdcall AddRef();                        // 0x04
	virtual ULONG   __stdcall Release();                       // 0x08
};

// lttexmod.cpp's TextureData; only the pointer identity matters here.
class TextureDataQ428;

// clienttexmgr.cpp's SharedTexture, 0x34 bytes.  Only +0x08 is written by
// this file: 0x0048b197 stores zero at this+0x70 and the sub-object starts
// at this+0x68, so the field is m_pEngineData -- exactly what Bink's twin
// writes at its own +0x30 = 0x28 + 0x08 (0x0040217a).
class SharedTextureQ428
{
public:
	void*          m_pBankNextFree;          // 0x00
	void*          m_pBankOwner;             // 0x04
	TextureDataQ428* m_pEngineData;          // 0x08
	void*          m_pRenderData;            // 0x0c
	uint8          m_pad_10[0x34 - 0x10];
};

// 0x00462900 -- clientmgr2.cpp / clientrender.cpp name it and WARN it is
// NOT dfree.  cdecl, one argument.
void tex_FreeTextureDataForQ428(SharedTextureQ428* pTexture);

// 0x0042ee30 -- lttexmod.cpp names it.  cdecl, one argument.
void tex_FreeTextureDataQ428(TextureDataQ428* pData);


// The instance, 0xa4 bytes -- the size 0x0048aa50 hands operator new, and
// binkvideo.cpp's own note ("the sub-object's offset (+0x28 in Bink, +0x68
// in Smack)") fixes the SharedTexture.  Only the fields these two bodies
// touch are named; the rest is padding.
class SmackVideoInstQ428
{
public:
	virtual ~SmackVideoInstQ428();                             // slot 0
	virtual void     Release();                                // slot 1
	// `void`, not LTRESULT. The body sets %eax on no path and the only
	// caller discards it, so the two are byte-identical -- but LTRESULT
	// makes VC6 emit C4716 (must return a value), which is an ERROR here,
	// not a warning. lvideomgr.cpp:351 and binkvideo.cpp:536 both declare
	// the slot as LTRESULT and should be corrected the same way.
	virtual void Restore();                                    // slot 2, 0x0048b120
	virtual void     FreeSurfaces();                           // slot 3, 0x0048b160
	virtual LTRESULT UpdateScreen();                           // slot 4
	virtual LTRESULT Update();                                 // slot 5
	virtual LTRESULT GetStatus();                              // slot 6
	virtual LTRESULT BindToSurface(void* pPoly);               // slot 7
	virtual LTRESULT _v20();                                   // slot 8
	virtual LTRESULT _v24();                                   // slot 9

	// 0x0048ac70 -- the teardown the destructor runs at 0x0048ac49.
	// binkvideo.cpp names the Bink twin (0x00401c70) `Term`.
	void     Term();
	LTRESULT InitScreen();                                     // 0x0048ada0
	LTRESULT InitTexture();                                    // 0x0048b000

public:
	uint8                    m_pad_04[0x20 - 0x04];
	// 0x020 -- released at 0x0048b170 and again the target of the SECOND
	// block's clear at 0x0048b183.  Bink's twin has the same field at +0x20.
	IDirectDrawSurfaceQ428*  m_pSurface;                       // 0x020
	uint8                    m_pad_24[0x28 - 0x24];
	// 0x028 -- a second surface Bink does NOT have; released at 0x0048b180.
	IDirectDrawSurfaceQ428*  m_pSurface2;                      // 0x028
	uint8                    m_pad_2c[0x64 - 0x2c];
	TextureDataQ428*         m_pTextureData;                   // 0x064
	SharedTextureQ428        m_Texture;                        // 0x068
	uint8                    m_pad_9c[0xa0 - 0x9c];
	// 0x0a0 -- Init()'s third argument (bTexture); 0 = on-screen.
	uint32                   m_nMode;                          // 0x0a0
};


// ----------------------------------------------------------------------- //
//  0x0048b120, 49 bytes -- SmackVideoInst::Restore, slot 2.
//
//  WHAT PINS THE SHAPE
//    * `mov eax,DWORD PTR [esi+0xa0]; test eax,eax; jne 0x48b13f` -- the
//      JUMPED-TO arm is the nonzero one, so the source guard is
//      `m_nMode == 0` and the on-screen path FALLS THROUGH.  Written the
//      other way round (`if (m_nMode)`) VC6 lays the texture arm first.
//    * BOTH arms test their Init's answer with `test eax,eax; je` onto the
//      SAME 0x0048b14f, so LT_OK (zero) is the skip and the teardown is the
//      error arm.
//    * the on-screen arm ends with its OWN `pop esi; ret` at 0x0048b13d
//      rather than a `jmp` -- /O2 duplicating the epilogue of the
//      if-branch: "two returns ... /O2 PRESERVES them".
//    * NO `xor eax,eax` anywhere: the function never sets a return value.
//      Declared LTRESULT to match lvideomgr.cpp:351 and binkvideo.cpp:536,
//      it falls off the end (MSVC C4715); declared `void` the bytes are
//      IDENTICAL and the caller at 0x0048a730 ignores the answer either
//      way.  That is a genuine coin-flip with zero byte cost.
//    * neither Init call is preceded by `mov ecx,esi`: %ecx still holds
//      `this` from the entry, which is allocation, not source.
// ----------------------------------------------------------------------- //

void SmackVideoInstQ428::Restore()
{
	if (m_nMode == 0)
	{
		if (InitScreen() != LT_OK)
			Term();
	}
	else
	{
		if (InitTexture() != LT_OK)
			Term();
	}
}


// ----------------------------------------------------------------------- //
//  0x0048b160, 73 bytes -- SmackVideoInst::FreeSurfaces, slot 3.
//
//  WHAT PINS THE SHAPE
//    * three positive guards in a row, each falling straight into the next,
//      with ONE `pop edi; pop esi; ret` at 0x0048b1a6.  No early return.
//    * THE COPY-PASTE BUG IS IN THE SHIPPED BYTES AND MUST BE REPRODUCED.
//      The second block releases m_pSurface2 (`mov eax,DWORD PTR [esi+0x28]`
//      at 0x0048b176) and then clears m_pSurface: 0x0048b183 is
//      `89 7e 20` = `mov DWORD PTR [esi+0x20],edi`, NOT [esi+0x28].  The
//      store is not dead -- the first block only ran if m_pSurface was
//      non-null -- so VC6 emits it.  Writing `m_pSurface2 = LTNULL;` there
//      changes one displacement byte and loses the match.
//    * the third block is Bink's 0x00402167 verbatim over +0x64/+0x68/+0x70
//      instead of +0x24/+0x28/+0x30: release the texture's render data,
//      null the SharedTexture's m_pEngineData, free the TextureData, null
//      the pointer.  The two cdecl frames share one `add esp,0x8`
//      (0x0048b1a0) because /O2 merged the cleanups.
//    * `xor edi,edi` + `cmp reg,edi` / `mov [..],edi` where Bink uses
//      `test eax,eax` + `mov DWORD PTR [..],0x0`: Smack has FOUR zero
//      stores to Bink's three, so /Ot materialised a zero register.  That
//      is allocation and is not source-reachable -- do not chase it.
//    * `cmp DWORD PTR [esi+0x64],edi` with no load (0x0048b186) against
//      Bink's `mov eax,[esi+0x24]; test eax,eax`: same reason.
// ----------------------------------------------------------------------- //

void SmackVideoInstQ428::FreeSurfaces()
{
	if (m_pSurface)
	{
		m_pSurface->Release();
		m_pSurface = LTNULL;
	}

	if (m_pSurface2)
	{
		m_pSurface2->Release();
		m_pSurface = LTNULL;                 // SIC -- 0x0048b183 writes +0x20
	}

	if (m_pTextureData)
	{
		tex_FreeTextureDataForQ428(&m_Texture);

		m_Texture.m_pEngineData = LTNULL;

		tex_FreeTextureDataQ428(m_pTextureData);

		m_pTextureData = LTNULL;
	}
}


//  THE BINK TWINS COME FREE.  Over binkvideo.cpp's BinkVideoInstK the same
//  two sources are 0x00402120 and 0x00402150:
//
//      LTRESULT BinkVideoInstK::Restore()
//      {
//          if (m_nMode == 0) { if (InitScreen()  != LT_OK) Term(); }
//          else              { if (InitTexture() != LT_OK) Term(); }
//      }
//
//      void BinkVideoInstK::FreeSurfaces()
//      {
//          if (m_pSurface) { m_pSurface->Release(); m_pSurface = LTNULL; }
//          if (m_pTextureData)
//          {
//              tex_FreeTextureDataForK(&m_Texture);
//              m_Texture.m_pEngineData = LTNULL;
//              tex_FreeTextureDataK(m_pTextureData);
//              m_pTextureData = LTNULL;
//          }
//      }
//
//  -- Bink has ONE surface and no second block, which is the whole of the
//  46/49 and 67/73 size difference.  They are included
//  because the class is already declared in binkvideo.cpp.


// ======================================================================= //
//
//  PART B -- 0x00489580, 47 bytes -- fnt_FreeAllFonts
//
//  BELONGS IN: src/engine/lithtech/fntdraw.cpp, PART ONE (the GDI font
//  module, 0x00488980..0x0048950c as that file's banner has it -- this row
//  extends the run to 0x004895ae).  THAT FILE ALREADY CARRIES EVERYTHING
//  THIS BODY NEEDS: `class CFontQ488` (m_pPrev/+0x00, m_pNext/+0x04,
//  m_hFont/+0x08), `CFontQ488* g_pFontListQ488` at 0x004b4d00, and
//  `fnt_FreeFontQ488` at 0x00488b20.  When landing, DELETE the Q428 copies
//  below and re-spell the body against the Q488 names.
//
//  WHAT PLACES IT -- the call graph, not the string pool (this run has no
//  literals at all).  0x00489580's only caller is 0x00489510, which is the
//  module's GDI teardown: it SelectObject()s g_hOldDIBQ488 (0x004b4d0c) and
//  g_hOldBrushQ488 (0x004b4d10) back into g_hFontDCQ488 (0x004b4d04),
//  DeleteDC()s it, DeleteObject()s g_hDIBQ488 (0x004b4d08) and zeroes all
//  three -- every one of them a global fntdraw.cpp already names, at the
//  address it already names it.  0x00489580 is the FIRST thing it does, so
//  the fonts go before the DC they were selected into.
//
//  WHAT PINS THE SHAPE
//    * `mov esi,ds:0x4b4d00; test esi,esi; je 0x4895ad` is ONE entry test
//      above a body that closes with `test esi,esi; jne 0x48958b`
//      (0x004895a9).  That is a rotated `while`, i.e. the source
//      tests at the top and /O2 peeled it -- not a `do/while` in the
//      source, because a source `do` would have no entry test at all and
//      the list can legitimately be empty here.
//    * the head is RELOADED at the top of every iteration (0x0048958b),
//      which is forced: fnt_FreeFont splices the node out and may move
//      g_pFontList (0x00488b20 does exactly that), so the global cannot be
//      cached across the call.  Reading it into a local before the loop
//      would lose that reload.
//    * `sub ecx,edx; neg ecx; sbb ecx,ecx; and esi,ecx` at
//      0x00489599..0x0048959f is VC6's branchless 0-or-x select: esi keeps
//      pNext when pNext != g_pFontList and becomes 0 when it equals it.
//      That is the CIRCULAR list's stop condition -- the list fntdraw.cpp
//      documents as "circular doubly-linked" -- so the walk halts when the
//      next pointer comes back round to the head.
//    * the select is computed BEFORE the call (0x0048959f against
//      0x004895a1), so the comparison is a statement above the free, not
//      after it.
//    * cdecl free: `push eax ... call ... add esp,0x4`.
// ----------------------------------------------------------------------- //

class CFontQ428
{
public:
	CFontQ428* m_pPrev;                            // 0x00
	CFontQ428* m_pNext;                            // 0x04
	HFONT      m_hFont;                            // 0x08
};

extern CFontQ428* g_pFontListQ428;                 // 0x004b4d00

// 0x00488b20 -- fntdraw.cpp's fnt_FreeFontQ488.  cdecl, one argument.
void fnt_FreeFontQ428(CFontQ428* pFont);


void fnt_FreeAllFontsQ428()
{
	CFontQ428* pCur;
	CFontQ428* pNext;

	pCur = g_pFontListQ428;

	while (pCur)
	{
		pNext = pCur->m_pNext;

		if (pNext == g_pFontListQ428)
			pNext = LTNULL;

		fnt_FreeFontQ428(pCur);

		pCur = pNext;
	}
}


// ======================================================================= //
//
//  PART C -- 0x00429a20, 0x00429fd0 and 0x0042a100
//            three methods of the CClientMgr+0x11f0 surface-cache pair
//
//  BELONGS IN: the client TU that owns 0x00429810..0x0042a04f.  The repo
//  has no file for it yet, but TWO existing files already carry a VIEW of
//  the outer class and must be kept in step with whatever lands:
//
//    src/engine/lithtech/clientmgr3.cpp:668-677  `class CCmgSub11F0`
//        ctor 0x00429a80, dtor 0x00429b40, Flush 0x00429ca0,
//        size 0x1254-0x11f0 = 0x64
//    src/engine/lithtech/clientblit.cpp:157-163  `class CSub11F0`
//        Update 0x00429e60, reached from CClientMgr::FlipScreen
//
//  WHAT THE MODULE IS, AND HOW FAR THAT GOES.
//
//   * The outer object lives at CClientMgr+0x11f0 and is constructed by
//     0x00410b2c, destroyed by 0x004110c7, flushed by 0x00410f73 and
//     updated by 0x00410936 (inside FlipScreen).  Its Init is 0x00429bc0,
//     called ONCE, from 0x0040fc61 -- the tail of the renderer/video
//     start-up that twenty instructions earlier makes the video manager
//     with the literal "BINK".  Init's two arguments are pushed at
//     0x0040fc51..0x0040fc53: `[ebx+0x4c0]` and the address of a local.
//   * +0x00 is an ILTClient FUNCTION TABLE, not a C++ vptr.  0x00429820
//     tests it; 0x004299b0 calls `[edi+0x168]`, 0x004299b7 `[edi+0x1a8]`,
//     0x00429841 `[ecx+0x178]`, 0x004299d8 `[ecx+0x194]`, 0x004298ed
//     `[ecx+0x1a4]`, 0x00429917 `[ecx+0x1cc]`.  Every one of those six
//     displacements is in src/engine/lithtech/clientinterface.h, and they
//     read GetScreenSurface / DrawSurfaceToSurfaceTransparent /
//     DeleteSurface / GetSurfaceDims / DrawSurfaceToSurface / FillRect.
//     So the class is a surface CACHE driven through the client interface.
//   * +0x04 and +0x18 are two StdLith CMoArrays (vptrs 0x004a2190 and
//     0x004a2160 stored by the constructor at 0x00429a92 / 0x00429acd);
//     the second has stride 0x1c (0x00429d05, 0x00429ddd) and each of its
//     records holds a heap pointer at +0x04 (0x00429d6a, 0x00429dcc) that
//     is `operator delete`d at 0x00429d7a.  THAT heap object is the class
//     0x00429a20 is a method of.
//   * The constructor seeds +0x48 = 0x9e (158), +0x4c = 0x4e (78),
//     +0x50 = 2 (0x00429b18..0x00429b26), which 0x0042a050 then reads as
//     a cell width, a cell height and a spacing -- a 158x78 cell with a
//     2-pixel gutter, tiled across the rect handed to SetRect.
//
//  WHAT IS NOT ESTABLISHED: the original FILE.  This run references no
//  string literal at all (a sweep of 0x004291c0..0x0042c000 finds
//  literals only at 0x0042bd36 and beyond, which belong to the counter
//  registration TU), there is no cvar record pointing into its storage,
//  and none of its methods appears in any vtable.  The names below are
//  OURS, built from the ILTClient calls and the geometry.
//
// ----------------------------------------------------------------------- //

// The ILTClient flat table, cut to the one slot PART C's rows need.  Full
// shape in src/engine/lithtech/clientinterface.h.
class ILTClientQ428;

// The heap record held at +0x04 of each 0x1c-byte slot of the outer
// object's second array.  Only the fields 0x00429a20 touches are named.
//   +0x00 the ILTClient table (0x00429833 / 0x004299a4)
//   +0x04 the cached HSURFACE (0x00429839, deleted through slot 0x178)
//   +0x08 a second HSURFACE (0x004299c4)
//   +0x10 the amount 0x00429a60 adds to the stored rect's top
//   +0x14 the stored LTRect (0x00429a47..0x00429a5a)
class CSub11F0ItemQ428
{
public:
	// 0x00429a20, thiscall, `ret $0x4`.
	LTRESULT SetRect(LTRect* pRect);

public:
	ILTClientQ428* m_pClientDE;                  // 0x00
	void*          m_hSurface;                   // 0x04
	void*          m_hSurface2;                  // 0x08
	uint32         m_Unknown0c;                  // 0x0c
	// 0x010 -- read once into %edx at 0x00429a2a, used in the guard at
	// 0x00429a33 and again at 0x00429a60.  A row height (name ours).
	int            m_nTopInset;                  // 0x10
	LTRect         m_rcRect;                     // 0x14
};


// ----------------------------------------------------------------------- //
//  0x00429a20, 85 bytes -- CSub11F0Item::SetRect.
//
//  WHAT PINS THE SHAPE
//    * THREE guards, and ALL THREE jump FORWARD to the one block at
//      0x00429a6b, which sits PAST the success `xor eax,eax; ret $0x4` at
//      0x00429a66.  The guards are POSITIVE and the
//      failure return is the function's trailing statement.  Spelt as three
//      early `return LT_ERROR;` statements /O2 would duplicate the epilogue
//      three times ("/O2 PRESERVES them") -- it does not.
//    * `cmp esi,edi; jge` twice (0x00429a35, 0x00429a3e) -- SIGNED, which
//      is LTRect's `int left, top, right, bottom` (src/LT2/sdk/inc/ltrect.h).
//    * the sum is `add esi,edx` with esi holding pRect->top, so the source
//      writes `pRect->top + m_nTopInset`, not the reverse: the other order
//      accumulates into the member's register.
//    * `lea esi,[ecx+0x14]` followed by stores at [ecx+0x14], [esi+0x4],
//      [esi+0x8], [esi+0xc] (0x00429a44..0x00429a5a) is VC6's inlined
//      memberwise LTRect::operator=, not four hand-written assignments.
//    * m_nTopInset is loaded ONCE, at 0x00429a2a, ahead of everything, and
//      reused at 0x00429a60 -- VC6 CSEing one member across the copy.
//      Written as two reads of the member; a hoisted local would also
//      produce it and is the fallback if this comes out one byte long.
// ----------------------------------------------------------------------- //

LTRESULT CSub11F0ItemQ428::SetRect(LTRect* pRect)
{
	if (pRect &&
	    pRect->top + m_nTopInset < pRect->bottom &&
	    pRect->left < pRect->right)
	{
		m_rcRect = *pRect;

		m_rcRect.top += m_nTopInset;

		return LT_OK;
	}

	return LT_ERROR;
}


// ----------------------------------------------------------------------- //
//  The outer object, CClientMgr+0x11f0.  0x64 bytes, which is exactly
//  clientmgr3.cpp's `uint8 m_pad_00[0x1254 - 0x11f0]`.  Every offset below
//  is read off the constructor (0x00429a80), the teardown (0x00429ca0) and
//  the two rows written here; the two CMoArray members are modelled as
//  opaque blocks because nothing here reaches inside them.
// ----------------------------------------------------------------------- //

class CSub11F0Q428
{
public:
	// 0x0042a050, thiscall, `ret $0x8`.  Rejects a cell smaller than
	// 0x20 x 0x10 (0x0042a05a / 0x0042a067, both `jb` -- unsigned), then
	// divides the rect by (cell + spacing) into m_nCols / m_nRows and keeps
	// the two remainders in m_nExtraX / m_nExtraY.
	LTRESULT SetCellSize(uint32 nWidth, uint32 nHeight);

	// 0x00429fd0, thiscall, `ret $0x8`.
	LTBOOL   GetCellRect(LTRect* pRect, uint32 nIndex);

	// 0x0042a100, thiscall, `ret $0x4`.
	LTRESULT SetRect(LTRect* pRect);

public:
	ILTClientQ428* m_pClientDE;                  // 0x00
	uint8          m_Array04[0x18 - 0x04];       // 0x04  CMoArray, vptr 0x004a2190
	uint8          m_Array18[0x2c - 0x18];       // 0x18  CMoArray, vptr 0x004a2160, stride 0x1c
	uint32         m_Unknown2c;                  // 0x2c  zeroed by 0x00429d30
	uint32         m_Unknown30;                  // 0x30
	uint32         m_Unknown34;                  // 0x34
	LTRect         m_rcArea;                     // 0x38  0x0042a119..0x0042a12e
	uint32         m_nCellWidth;                 // 0x48  ctor seeds 0x9e
	uint32         m_nCellHeight;                // 0x4c  ctor seeds 0x4e
	uint32         m_nSpacing;                   // 0x50  ctor seeds 0x02
	uint32         m_nCols;                      // 0x54
	uint32         m_nRows;                      // 0x58
	uint32         m_nExtraX;                    // 0x5c
	uint32         m_nExtraY;                    // 0x60
};


// ----------------------------------------------------------------------- //
//  0x00429fd0, 117 bytes -- CSub11F0::GetCellRect.
//
//  WHAT PINS THE SHAPE
//    * `imul eax,edi` then `cmp ebx,eax; jb 0x429fee` (0x00429fdb /
//      0x00429fe2): an UNSIGNED bounds test against m_nCols * m_nRows, and
//      the failure arm is the FALL-THROUGH `xor eax,eax; ret $0x8`.  The
//      success answer is `mov eax,0x1` at 0x0042a03c, so the return type is
//      LTBOOL, not LTRESULT -- 1 is success here and 1 is FAILURE in the
//      two SetRects above.
//    * TWO `div`s, both unsigned, and the one emitted FIRST divides by
//      m_nRows and keeps only the REMAINDER (0x00429ff2, %edx -> %ebp);
//      the second divides by m_nCols and keeps only the QUOTIENT
//      (0x00429ffe, %eax).  That mismatch -- row from `% m_nRows`, column
//      from `/ m_nCols` -- is IN THE SHIPPED BYTES and is only coherent for
//      a square grid.  It is reproduced verbatim; "fixing" it changes the
//      two `div` operands.
//    * the locals are declared in the order the divisions appear.  VC6
//      does not reorder a `div` pair for scheduling, so the row is computed
//      before the column even though the column is USED first.  THIS IS THE
//      ONE COIN-FLIP IN THIS BODY -- the alternative is no locals at all
//      with the two divisions written inside the two rect assignments,
//      which emits them in the other order.
//    * `mov edi,esi` at 0x0042a01c keeps `left` in a register for
//      `right = left + m_nCellWidth`, while `bottom` RELOADS [eax+0x4]
//      at 0x0042a02f.  Both statements are written the same way; the
//      asymmetry is allocation (edx was reused in between).
//    * store order is left, top, right, bottom -- 0x0042a014, 0x0042a023,
//      0x0042a02c, 0x0042a039 -- which is the statement order.
// ----------------------------------------------------------------------- //

LTBOOL CSub11F0Q428::GetCellRect(LTRect* pRect, uint32 nIndex)
{
	uint32 nRow;
	uint32 nCol;

	if (nIndex >= m_nCols * m_nRows)
		return LTFALSE;

	nRow = nIndex % m_nRows;
	nCol = m_nCols - nIndex / m_nCols - 1;

	pRect->left   = nCol * (m_nCellWidth + m_nSpacing) + m_nExtraX + m_nSpacing;
	pRect->top    = (m_nCellHeight + m_nSpacing) * nRow;
	pRect->right  = pRect->left + m_nCellWidth;
	pRect->bottom = pRect->top + m_nCellHeight;

	return LTTRUE;
}


// ----------------------------------------------------------------------- //
//  0x0042a100, 83 bytes -- CSub11F0::SetRect.
//
//  WHAT PINS THE SHAPE
//    * `mov eax,[esp+0x4]` BEFORE the `push esi` (0x0042a100): the argument
//      is read first and %ecx is only parked afterwards, which is VC6's
//      ordinary thiscall entry, not a clue.
//    * `test eax,eax; jne 0x42a114` with `mov eax,0x1; pop esi; ret $0x4`
//      FALLING THROUGH -- so this one IS a leading negative guard with its
//      own duplicated epilogue, the mirror of 0x00429a20's shared tail.
//      Read which side has the extra `ret`.  Here the failure
//      block is first and the body is jumped to, so `if (!pRect) return
//      LT_ERROR;` is a genuine early return.
//    * `lea ecx,[esi+0x38]` + four dword moves (0x0042a114..0x0042a12e) is
//      the same inlined LTRect::operator= as 0x00429a44.
//    * the two arguments to 0x0042a050 are loaded from +0x48 and +0x4c and
//      pushed [0x4c] then [0x48] (0x0042a134/0x0042a135), so the call is
//      SetCellSize(m_nCellWidth, m_nCellHeight) -- the CURRENT cell size
//      re-applied to the new rect.  The first load is scheduled INTO the
//      rect copy at 0x0042a12b; that is allocation.
//    * NOTHING sets %eax after the call.  `je 0x42a14f` skips two zero
//      stores that do not touch %eax, so both paths return 0x0042a050's
//      answer.  The source therefore keeps it in a local and returns it;
//      `if (SetCellSize(...) != LT_OK) { ... } return LT_OK;` would emit
//      `xor eax,eax` and does not.
// ----------------------------------------------------------------------- //

LTRESULT CSub11F0Q428::SetRect(LTRect* pRect)
{
	LTRESULT dResult;

	if (!pRect)
		return LT_ERROR;

	m_rcArea = *pRect;

	dResult = SetCellSize(m_nCellWidth, m_nCellHeight);

	if (dResult != LT_OK)
	{
		m_nCols = 0;
		m_nRows = 0;
	}

	return dResult;
}


// ======================================================================= //
//
//  PART D -- 0x004290e0 and 0x00429130 -- MOVED to src/engine/server/dalloc.cpp
//
//  The class is now named: these are the two file-scope CWorldItem18
//  terminator leaves (src/engine/server/worldbsp.h), g_WorldTreeFrontTerm(1)
//  and g_WorldTreeBackTerm(2), that worldbsp.h's g_pWorldTreeFrontTerm /
//  g_pWorldTreeBackTerm (0x004a9878 / 0x004a987c) point at. The earlier
//  claim that there is no server twin was wrong: server.dll calls
//  CWorldItem18::Init (0x10009250) from 0x1000d3e3 / 0x1000d3f5 with
//  `push 1` / `push 2` on 0x10058388 / 0x100583a0, between dfree and
//  memtrack_Init exactly as lithtech.exe has them between dfree and
//  memtrack_Init. The shared dalloc.cpp source MATCHes 0x004290e0 and
//  0x00429130 here (Init expanded at /O2).
//
// ----------------------------------------------------------------------- //


// ======================================================================= //
//
//  PART E -- 0x00478930, 111 bytes -- NOT INDEPENDENT WORK
//
//  This row is `ObjectCreateStruct::ObjectCreateStruct()`, the SDK's own
//  in-class constructor at src/LT2/sdk/inc/ltbasedefs.h:218.  No source is
//  written here because the source is that header, unchanged.
//
//  HOW IT IS IDENTIFIED, field by field against ltbasedefs.h's Clear():
//      +0x000 WORD  = 0        m_ObjectType        0x0047893a
//      +0x002 WORD  = 0        m_ContainerCode     0x0047893d
//      +0x004 DWORD = 1        m_CreateFlags       0x00478941  OCS_AUTOLOAD
//      +0x008 / +0x00c = 0     m_Flags / m_Flags2  0x00478948
//      +0x010..0x018 = 0       m_Pos.Init()        0x0047894e
//      +0x01c..0x024 = 1.0f    m_Scale.Init(1,1,1) 0x00478957
//      +0x028..0x030 = 0, +0x034 = 1.0f
//                              m_Rotation.Init()   0x00478960
//      +0x038 = 0              m_UserData          0x0047896c
//      +0x03c BYTE  = 0        m_ClassName[0]      0x0047896f
//      +0x07c BYTE  = 0        m_Filename[0]       0x00478972
//      +0x0fc,+0x17c BYTE = 0  m_SkinNames[i][0]   0x00478980, trip count
//                              `mov esi,0x2` at 0x0047897b = MAX_MODEL_TEXTURES
//      +0x1fc BYTE  = 0        m_Name[0]           0x0047898b
//      +0x27c,+0x280 = 0       m_NextUpdate,
//                              m_fDeactivationTime 0x00478991
//  0x284 is sizeof(ObjectCreateStruct) for this SDK, which
//  src/engine/lithtech/predobj.cpp:471 already establishes independently.
//
//  IT IS THE CONSTRUCTOR AND NOT Clear().  `mov eax,ecx` at 0x00478930 and
//  the base register being %eax throughout is VC6's this-return; compare
//  0x00429b39 (`mov eax,edi; ret`, a constructor) and 0x004441a0.  The
//  separate 109-byte body at 0x004147f0 is the SAME statements over %ecx
//  with NO this-return -- that one is `?Clear@ObjectCreateStruct@@QAEXXZ`,
//  it has SIX callers (0x004144f2, 0x00432751, 0x00460f73, 0x00475680,
//  0x004786d0, 0x0047a4ec), and src/engine/lithtech/predobj.cpp ALREADY
//  EMITS IT (`strings` finds `?Clear@ObjectCreateStruct@@QAEXXZ` in
//  src/engine/lithtech/Release/predobj.obj and no `??0ObjectCreateStruct`).
//
//  WHY IT CANNOT BE LANDED FROM A SCRATCH TU.  The constructor's body is
//  `Clear();`, so when VC6 EXPANDS the constructor at a declaration site it
//  leaves a `call Clear` (inline depth two under /Ob1) -- that is precisely
//  what 0x00460f73 is, followed by the source's own explicit
//  INIT_OBJECTCREATESTRUCT expanding Clear at depth one.  A standalone
//  COMDAT for the constructor appears only where /Ot DECLINES the
//  expansion, and 0x00478623 -- its ONE call site in the whole image -- is
//  inside the 1314-byte function at 0x00478320.  The
//  decision is per-call-site, so the row falls out for free when the TU
//  that owns 0x00478320 is reconstructed with `ObjectCreateStruct` locals,
//  and a synthetic caller written to provoke it is a coin toss with no
//  evidence behind it.  Recommend excluding the row and letting it land
//  with its owner.


// ======================================================================= //
//
//  CLOSING -- extent defects, not-a-function rows, placement, coin-flips
//
// ----------------------------------------------------------------------- //
//  1.  EXTENT DEFECT (one)
//
//  0x00428fdd / 70 / gap is NOT A FUNCTION.  It is the body of dalloc()
//  after its entry test, and dalloc() starts at 0x00428fd0:
//
//      00428fd0  push edi                  <- the saved register
//      00428fd1  mov  edi,DWORD PTR [esp+0x8]
//      00428fd5  test edi,edi
//      00428fd7  jne  0x428fdd             <- the only way in
//      00428fd9  xor  eax,eax
//      00428fdb  pop  edi
//      00428fdc  ret
//      00428fdd  push esi                  <- "row start"
//      ...
//      00429018  mov  eax,esi
//      0042901a  pop  esi
//      00429021  pop  edi                  <- balances 0x00428fd0's push
//      00429022  ret
//
//  THE STACK PROVES IT: 0x00428fdd never pushes the %edi that 0x00429021
//  pops (the `push edi` at 0x00428fde is the malloc ARGUMENT, consumed by
//  the `add esp,0x4` at 0x00428fe6).  Two pushes, two pops, one function of
//  83 bytes.  The neighbouring row 0x00428fd0 / 13 / call is the truncated
//  head and 13 + 70 = 83 tiles it exactly -- the
//  "INTERIOR BLOCK FIVE BYTES PAST AN INTRA-FUNCTION `jne`" family, here
//  six bytes past a two-byte `jne`.
//
//  AND THE FUNCTION IS ALREADY WRITTEN.  src/engine/server/dalloc.cpp's
//  `void* dalloc(uint32 nSize)` is this source; build/engine.sh line 331
//  compiles ..\server\dalloc.cpp into lithtech.exe too.  The client body
//  has no CTimedScope because /D LT_SERVER_TIMERS is server-only
//  which is why it is 83 bytes against the server's
//  0x1000d2ef.  Nothing to write.
//
//  Every other row's extent is CLEAN.  For each, the last instruction ends
//  exactly on the stated boundary, the bytes after it are 0x90 padding, and
//  a sweep of every e8/e9/0f8x/7x/eb displacement in .text finds no branch
//  entering any of them anywhere but at the first byte:
//
//      0x004290e0 +54  -> last `ret` 0x00429115, no branches at all
//      0x00429130 +54  -> last `ret` 0x00429165, no branches at all
//      0x00429a20 +85  -> last `ret 0x4` 0x00429a72..74; targets
//                         0x00429a6b only, interior
//      0x00429fd0 +117 -> last `ret 0x8` 0x0042a042..44; target 0x00429fee
//      0x0042a100 +83  -> last `ret 0x4` 0x0042a150..52; targets
//                         0x0042a114, 0x0042a14f
//      0x00489580 +47  -> last `ret` 0x004895ae; targets 0x0048958b,
//                         0x004895ad
//      0x0048b120 +49  -> last `ret` 0x0048b150; targets 0x0048b13f,
//                         0x0048b14f
//      0x0048b160 +73  -> last `ret` 0x0048b1a8; targets 0x0048b176,
//                         0x0048b186, 0x0048b1a6
//      0x00478930 +111 -> last `ret` 0x0047899e; target 0x00478980 (the
//                         MAX_MODEL_TEXTURES loop)
//
//  The only data references into any row are 0x004a38ac -> 0x0048b120 and
//  0x004a38b0 -> 0x0048b160, which are the two vtable slots.  No ASCII
//  accident applies: neither `ptr` row is unaligned and both have a
//  dispatch-table reference.
//
// ----------------------------------------------------------------------- //
//  2.  NOT-A-FUNCTION ROWS
//
//  0x00428fdd,"not a function -- interior block of dalloc(), which starts at 0x00428fd0 and is 83 bytes: the jne at 0x00428fd7 is the only entry, the push edi at 0x00428fd0 is what the pop edi at 0x00429021 balances, and the 13-byte row 0x00428fd0 tiles the rest exactly. The function is already reconstructed as dalloc() in src/engine/server/dalloc.cpp, which build/engine.sh compiles into lithtech.exe as well as server.dll; the client body lacks the CTimedScope because LT_SERVER_TIMERS is server-only."
//  0x00478930,"not independent work -- ObjectCreateStruct::ObjectCreateStruct(), the in-class constructor at src/LT2/sdk/inc/ltbasedefs.h:218, emitted out of line. Identified field for field against Clear() and distinguished from Clear() itself (0x004147f0, which predobj.obj already emits as ?Clear@ObjectCreateStruct@@QAEXXZ) by the mov eax,ecx this-return at 0x00478930. Its ONE call site is 0x00478623, inside the 1314-byte function at 0x00478320, so the COMDAT falls out when that TU is written with an ObjectCreateStruct local; there is nothing to author."
//
// ----------------------------------------------------------------------- //
//  3.  BELONGS IN ANOTHER FILE
//
//  0x00489580   src/engine/lithtech/fntdraw.cpp, PART ONE.  That file
//               already has CFontQ488, g_pFontListQ488 (0x004b4d00) and
//               fnt_FreeFontQ488 (0x00488b20); re-spell PART B against
//               them and delete the Q428 duplicates.  It also extends that
//               part's stated run from 0x0048950c to 0x004895ae, and
//               0x00489510 (107 bytes, the GDI teardown that calls this
//               one) is its free neighbour.
//
//  0x0048b120   the Smack video TU -- today PART TWO of
//  0x0048b160   src/engine/lithtech/fntdraw.cpp.  The BINK twins
//               0x00402120 and 0x00402150 belong in
//               src/engine/lithtech/binkvideo.cpp, which already DECLARES
//               both (lines 536 and 547) and defines neither; the source
//               for them is quoted at the end of PART A.
//
//  0x00429a20   a new client TU.  src/engine/lithtech/clientmgr3.cpp
//  0x00429fd0   (CCmgSub11F0) and src/engine/lithtech/clientblit.cpp
//  0x0042a100   (CSub11F0) both carry views of the outer class and must be
//               reconciled with whatever lands.
//
//  CORRECTION TO src/engine/lithtech/lvideomgr.cpp and binkvideo.cpp:
//  both declare instance slot 2 as `virtual LTRESULT Restore()`.  Neither
//  0x0048b120 nor its Bink twin 0x00402120 sets %eax on any path, and the
//  only caller (0x0048a730, inside 0x0048a710) discards the answer, so
//  `virtual void Restore()` fits the bytes exactly as well and is the
//  better spelling.  The emitted bytes are identical either way.
//
//  VTABLE CORRECTION: 0x004a3890 and 0x004a38a4
//  are two tables.  0x004a3890 (5 slots) is SmackVideoMgr's --
//  its slots 2/3/4 are the same three bodies as BinkVideoMgr's at
//  0x004a1440 -- and 0x004a38a4 (10 slots) is SmackVideoInst's, which is
//  what 0x0048ac43 stores as the vptr.  Likewise 0x004a1440 is the Bink
//  MANAGER's five slots and the Bink INSTANCE's eleven start at 0x004a1454,
//  with the shared abstract base's table at 0x004a147c (the value
//  0x0048ac5e restores).
//
// ----------------------------------------------------------------------- //
//  4.  UNSETTLED COIN-FLIPS, both spellings given
//
//  (a) 0x0048b120 return type.  `LTRESULT Restore()` falling off the end
//      (MSVC C4715) versus `void Restore()`.  Identical bytes; the choice
//      only affects the header.  Written LTRESULT above to match
//      lvideomgr.cpp:351; see the correction in section 3.
//
//  (b) 0x00489580's circular-list stop.  Written
//          if (pNext == g_pFontList) pNext = LTNULL;
//      against the branchless `sub/neg/sbb/and` at 0x00489599.  If VC6
//      branches instead of if-converting, the ternary
//          pNext = (pNext == g_pFontList) ? LTNULL : pNext;
//      is the other spelling -- src/engine/lithtech/ltnew1.cpp records
//      `neg/sbb/and` as VC6's ternary select, so that is the fallback.
//
//  (c) 0x00429fd0's two locals.  Written with `nRow` (the `% m_nRows`)
//      declared before `nCol` (the `/ m_nCols`) because the shipped code
//      emits the m_nRows division first (0x00429ff2) even though the
//      column is used first.  The other spelling is no locals at all --
//      both divisions written inline in the two rect assignments -- which
//      reverses the two `div`s.
//
//  (d) 0x00429a20's m_nTopInset.  Written as two reads of the member,
//      matching the single load at 0x00429a2a plus the reuse at
//      0x00429a60.  If VC6 reloads it, hoist it into a local:
//          int nInset = m_nTopInset;
//
//  (e) PART D's statement order.  Described in full in PART D; the layout
//      is measured and the order is a guess.  The alternative is
//      declaration order (m_nUnknown00/04/08, then the link, then the
//      index, then the two bytes).
//
//  (f) PART D's class.  Written with one constructor taking the type byte,
//      because the two bodies differ ONLY in that immediate.  Two separate
//      classes each hard-coding its own tag produce the same bytes after
//      constant propagation and cannot be distinguished from the image.
//
// ----------------------------------------------------------------------- //
//  5.  NOT RUN
//
//  No claim is made about call-sequence agreement: it needs a compiled
//  object.
// ======================================================================= //
