// ======================================================================= //
//  lithtech.exe — cluster w11_43c000.
//
//  TWO UNRELATED JOBS IN ONE FILE.  They are NOT one module; the evidence
//  is given at each part below.
//
//    PART A   0x0043c6d0..0x0043cd00   a client-only geometry-helper TU:
//             a uniform 3-D grid, a 3x3 basis and the six cardinal-side
//             bases (.CRT$XCU slot 0x004a7084 holds the latter's
//             initializer, 0x0043c840).  NO server.dll twin exists for any
//             of it.  (0x0043c3e0..0x0043c6cf, once read as part of it, is
//             serverengine.cpp's; see A.1 / A.2 below.)
//
//    PART B   0x00426670 / 0x00426c40   CWorldBsp::Inherit and its private
//             node-pointer remap.  src/engine/server/worldbsp.h already
//             carries the class; these two functions have NO server.dll
//             twin, so they are the client-only half of that source.
//
//  Ground truth throughout is the retail lithtech.exe.  Every offset,
//  constant and branch below is cited by the address it was read at.
//  Symbols this file introduces carry the Q43C suffix.
//
//  BUILD: MSVC 6 rev 8447, /O2 (/Og /Oi /Ot /Oy /Ob1) — frameless,
//  %esp-relative.
// ======================================================================= //
#include <windows.h>
#include <string.h>

#include "ltbasedefs.h"
#include "collection.h"


// ----------------------------------------------------------------------- //
//  What this file calls but does not define.
//
//  Callee SYMBOLS are masked relocations as far as byte matching is concerned,
//  so only the SIGNATURE matters here, not the name.
// ----------------------------------------------------------------------- //

// 0x004290b0 / 0x004290c0 — the engine allocator's two one-argument cdecl
// forwarders (0x004290b0 -> 0x00428fd0, 0x004290c0 -> 0x00429080, which is
// the counted free).  dalloc.cpp names the pair on the server side.
void* dalloc(uint32 nSize);
void  dfree(void* pMem);



// ======================================================================= //
//                                PART A
//         0x0043c3e0..0x0043cd00 — the geometry-helper translation unit
//
//  Module identification:
//
//   * NO console-variable table reaches any of this file's storage.  A scan
//     of .data for the 20-byte {char*, void*, void*} record shape turned up
//     only the error-name tables (0x004a8080, 0x004ac10c, 0x004af408); none
//     of them points into 0x004b3e00..0x004b3f60.
//   * The DISPATCH TABLE is decisive for the bank: .rdata 0x004a2438 holds
//     {0x0043c690, 0x0043c5c0, 0x0043c630, 0x0043c660} and .rdata 0x004a14c8
//     holds {0x004041b0, 0x004928e7, 0x004928e7, 0x004928e7} — three
//     _purecall slots.  That is src/engine/server/objbank.cpp's
//     CObjectBankBase {dtor, Allocate, Free, Init} exactly, and the derived
//     table's Init (0x0043c660) brackets sb_FreeChunks+sb_Init with
//     Enter/Leave on the object's own +0x20, which is that file's
//     CLockedObjectBank shape.
//   * The CALL GRAPH places the TU: 0x0043ccb0 (the side picker) has ONE
//     caller in the whole image, 0x004232f5, inside the 4739-byte world
//     loader at 0x00422970; the grid's constructor and Init are reached
//     from 0x0042239e and 0x00423dc5 in the same block.  So this TU is a
//     helper library for the client's world loader.
//   * THE STRING POOL SAYS NOTHING AT ALL, and that is worth recording:
//     0x00418000..0x00445000 — 180KB, 67,220 instructions — contains ZERO
//     references to a string literal.  There is no REPORT_ERROR literal
//     anywhere in either block, so the strongest naming instrument
//     is simply absent here.  Every class name below is therefore
//     OURS and carries the Q43C tag.
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  A.1 / A.2  (0x0043c3e0..0x0043c6cf) are NOT this file's.  The locked
//  bank of twelve-byte nodes at 0x004b3e40 (initializer 0x0043c3e0, the
//  {0x0043c690, 0x0043c5c0, 0x0043c630, 0x0043c660} table at 0x004a2438)
//  and the lock at 0x004b3e28 (initializer 0x0043c470) are
//  src/engine/server/serverengine.cpp's g_DepLinkBank and g_DepLinkCS --
//  that file's client compile matches 0x0043c5c0, 0x0043c630, 0x0043c660
//  and its dl_UnlinkAndFree at 0x0043c4b0 -- and the previous transcription
//  of them here (CNodeBankQ43C, g_NodeBankQ43C, g_ListLockQ43C) compiled
//  both objects' initializers a second time.  Consecutive .CRT$XCU slots
//  do not make one translation unit: serverengine.obj's two and this
//  file's one are consecutive because the objects are.
// ----------------------------------------------------------------------- //


// A second, unwrapped release entry point.  0x004290c0 forwards one
// argument to 0x00429080; the grid's Term calls 0x00429080 DIRECTLY
// (0x0043c826), where the struct bank calls the 0x004290c0 forwarder
// (0x00490be3).  Both are declared so the two call sites keep their own
// callee; the symbols are masked relocations either way.
void dfree_directQ43C(void* pMem);           // 0x00429080


// ----------------------------------------------------------------------- //
//  A.3  The uniform 3-D grid — 0x0043c6d0..0x0043c836.
//
//  LAYOUT, read off Clear (0x0043c7e0) and Init (0x0043c6f0):
//    +0x00  the cell run it owns   (Term at 0x0043c823 frees exactly this)
//    +0x04  total cells            (0x0043c7be = nZ*nY*nX)
//    +0x08  +0x0c  +0x10  counts   (0x0043c749 / 0x0043c75c / 0x0043c772)
//    +0x14  +0x18  +0x1c  count-1  (0x0043c79e / 0x0043c7a4 / 0x0043c7b0)
//    +0x20  one slice of cells     (0x0043c7bb = nY*nX)
//    +0x24  cell size, as a vector (0x0043c703..0x0043c709, three copies
//                                   of ONE float argument)
//    +0x30  its reciprocal         (0x0043c718 / 0x0043c728 / 0x0043c738)
//    +0x3c  the minimum corner     (0x0043c7c6..0x0043c7d2)
//
//  Clear does NOT touch +0x20 — it stores eleven fields and skips the slice
//  stride — which is a source fact, not an optimisation, and is reproduced.
// ----------------------------------------------------------------------- //
class CWorldGridQ43C
{
public:
	CWorldGridQ43C();                        // 0x0043c6d0
	~CWorldGridQ43C();                       // 0x0043c6e0 (jmp 0x0043c820)

	// OUT OF CLASS deliberately: 0x0043c6d0's whole body is
	// `call 0x0043c7e0` and 0x0043c830 calls it as well, so the shipped
	// build never expands it.  A non-template function
	// declared out-of-class is the one construct /Ob1 never expands.
	void Clear();                            // 0x0043c7e0
	void Term();                             // 0x0043c820

	void Init(LTVector* pMin, LTVector* pMax, float fCellSize); // 0x0043c6f0

public:
	void*    m_pCells;                       // +0x00
	int      m_nCells;                       // +0x04
	int      m_nCountX;                      // +0x08
	int      m_nCountY;                      // +0x0c
	int      m_nCountZ;                      // +0x10
	int      m_nMaxX;                        // +0x14
	int      m_nMaxY;                        // +0x18
	int      m_nMaxZ;                        // +0x1c
	int      m_nSliceStride;                 // +0x20
	LTVector m_vCellSize;                    // +0x24
	LTVector m_vInvCellSize;                 // +0x30
	LTVector m_vMin;                         // +0x3c
};


// 0x0043c7e0.  The store order is the shipped one and it is NOT the layout
// order: 0x00, 0x04, then 0x10 / 0x0c / 0x08 descending, then 0x1c / 0x18 /
// 0x14 descending, then the three vectors.  `Init()` on each vector rather
// than nine `= 0.0f` assignments is what keeps all of them as plain integer
// stores of the register already holding zero (worldbsp.h's CWorldSub108
// records the same reading from the other side).
void CWorldGridQ43C::Clear()
{
	m_pCells = LTNULL;
	m_nCells = 0;

	m_nCountZ = 0;
	m_nCountY = 0;
	m_nCountX = 0;

	m_nMaxZ = 0;
	m_nMaxY = 0;
	m_nMaxX = 0;

	m_vCellSize.Init();
	m_vInvCellSize.Init();
	m_vMin.Init();
}

// 0x0043c820.  No null test around the release — 0x00429080 carries its own
// (0x00429084) — and the reset is the same Clear.
void CWorldGridQ43C::Term()
{
	dfree_directQ43C(m_pCells);

	Clear();
}

CWorldGridQ43C::CWorldGridQ43C()             // 0x0043c6d0
{
	Clear();
}

CWorldGridQ43C::~CWorldGridQ43C()            // 0x0043c6e0
{
	Term();
}


// ======================================================================= //
//  >>> ROW 0x0043c6f0, 235 bytes (0x0043c6f0..0x0043c7da). <<<
//
//  `ret 0xc` with %ecx live, so __thiscall with three stack arguments.
//  The two vector arguments are POINTERS, not values: 0x0043c73b `fld
//  [ebx]` and `fsub [edi]` dereference what was loaded as a dword at
//  0x0043c70c / 0x0043c714.
//
//  WHY THE RECIPROCALS ARE READ BACK OFF THE MEMBER.  0x0043c710 divides
//  by [esi+0x24] and 0x0043c721 / 0x0043c731 by [esi+0x28] / [esi+0x2c] —
//  the fields just written — so the source spells `1.0f / m_vCellSize.x`
//  and not `1.0f / fCellSize`.  The x reciprocal is kept live on the FPU
//  stack (0x0043c718 is `fst`, not `fstp`) and consumed by `fmul st,st(1)`
//  at 0x0043c73f; y and z are spilled to [esp+0x1c] and [esp+0xc] and
//  reloaded at 0x0043c752 / 0x0043c769.  All three are the compiler's
//  choice, not the source's.
//
//  THE THREE `== 0` FIXUPS ARE THREE SEPARATE `if`s, not a nested chain:
//  0x0043c775 hoists the 1 into %eax once and each guard is
//  `test ; jne ; mov` onto the NEXT guard, with no shared exit.
//
//  The counts are `(int)(...) + 1`: 0x0043c741 / 0x0043c756 / 0x0043c769
//  call __ftol (0x004928c0) and each is followed by `inc eax`.
// ======================================================================= //
void CWorldGridQ43C::Init(LTVector* pMin, LTVector* pMax, float fCellSize)
{
	// 0x0043c703..0x0043c709 — one argument copied into all three.
	m_vCellSize.Init(fCellSize, fCellSize, fCellSize);

	// 0x0043c6f6 / 0x0043c71b / 0x0043c72b each `fld` 1.0f from
	// .rdata 0x004a1a08 (= 0x3f800000).
	m_vInvCellSize.Init(1.0f / m_vCellSize.x,
	                    1.0f / m_vCellSize.y,
	                    1.0f / m_vCellSize.z);

	m_nCountX = (int)((pMax->x - pMin->x) * m_vInvCellSize.x) + 1;
	m_nCountY = (int)((pMax->y - pMin->y) * m_vInvCellSize.y) + 1;
	m_nCountZ = (int)((pMax->z - pMin->z) * m_vInvCellSize.z) + 1;

	if (m_nCountX == 0)
		m_nCountX = 1;

	if (m_nCountY == 0)
		m_nCountY = 1;

	if (m_nCountZ == 0)
		m_nCountZ = 1;

	m_nMaxX = m_nCountX - 1;                 // 0x0043c79b lea eax,[ecx-1]
	m_nMaxY = m_nCountY - 1;                 // 0x0043c7a1 lea eax,[edx-1]
	m_nMaxZ = m_nCountZ - 1;                 // 0x0043c7aa lea ebx,[eax-1]

	// 0x0043c7b3 `mov ebx,edx ; imul ebx,ecx` — nY*nX, in that order.
	m_nSliceStride = m_nCountY * m_nCountX;

	// 0x0043c7ad `imul eax,edx` then 0x0043c7b8 `imul eax,ecx` — the
	// product is built nZ*nY then *nX, which is this parenthesisation.
	m_nCells = m_nCountZ * m_nCountY * m_nCountX;

	// 0x0043c7c3 `add esi,0x3c` then three moves off the base —
	// "a whole-vector copy adjusts the base".
	m_vMin = *pMin;
}


// ----------------------------------------------------------------------- //
//  A.4  The 3x3 basis and the six cardinal-side bases.
//
//  0x0043cc60 is the constructor: `mov eax,ecx` at 0x0043cc64 and `ret 0x24`
//  at 0x0043cca6 — %ecx is the object, 36 bytes of stack arguments are three
//  LTVectors by value, and it hands `this` back in %eax, which is what a VC6
//  constructor always does and what the six `rep movs` at the call sites
//  read from.  The three stores are +0x00, +0x0c, +0x18, each as a whole
//  vector off an adjusted base (`lea ecx,[eax+0xc]` at 0x0043cc7a,
//  `lea ecx,[eax+0x18]` at 0x0043cc92).
// ----------------------------------------------------------------------- //
// The plain record is src/engine/server/servermath.cpp's SAxisBasis, and the
// array below is the g_AxisBases that file declares extern (its three
// readers are compiled into this image too): one table at 0x004b3e78, so one
// definition, here with its dynamic initializer.
struct SAxisBasis                            // plain, 0x24 bytes
{
	LTVector m_vRight;                       // +0x00
	LTVector m_vUp;                          // +0x0c
	LTVector m_vNormal;                      // +0x18  (the side's forward)
};

class CSideBasisQ43C : public SAxisBasis
{
public:
	CSideBasisQ43C(LTVector vRight, LTVector vUp, LTVector vForward);
};

// 0x0043cc60
CSideBasisQ43C::CSideBasisQ43C(LTVector vRight, LTVector vUp, LTVector vForward)
{
	m_vRight  = vRight;
	m_vUp     = vUp;
	m_vNormal = vForward;
}


// ======================================================================= //
//  >>> ROW 0x0043c850, 1031 bytes (0x0043c850..0x0043cc56). <<<
//
//  EXTENT: SOUND, AND THE `thunk` PROVENANCE IS A LABEL, NOT A DEFECT.
//  0x0043c840 is the five-byte thunk (`jmp 0x0043c850`) followed by eleven
//  0x90s of COMDAT alignment, and .data 0x004a7084 — a .CRT$XCU slot —
//  holds 0x0043c840.  The BODY starts at 0x0043c850 with `sub esp,0x48`
//  and ends at 0x0043cc56 with `ret`, 0x0043cc57..0x0043cc5f being nine
//  more alignment 0x90s: 0x0043cc57 - 0x0043c850 = 0x407 = 1031 exactly.
//  So the row's start, size and status are all correct; only the WORD
//  "thunk" is wrong, and the actual thunk 0x0043c840 is carried as the
//  separate 5-byte `ptr` row above it.  Nothing to split.
//
//  WHY THIS IS A DYNAMIC INITIALIZER AND NOT A MEMBER.  The body takes no
//  `this`, leaves %eax dead and does not register an atexit destructor
//  (contrast 0x0043c3e0, which does).  Six times it builds three LTVector
//  temporaries in the frame, pushes them by value, constructs one 36-byte
//  temporary at [esp+0x50] (the same slot every time — one temporary,
//  reused) and `rep movs` nine dwords of it into .data:
//
//    0x0043c8ec -> 0x004b3e78    0x0043ca40 -> 0x004b3ec0
//    0x0043c996 -> 0x004b3e9c    0x0043caea -> 0x004b3ee4
//    0x0043cb94 -> 0x004b3f08    0x0043cc3e -> 0x004b3f2c
//
//  0x24 apart and contiguous: one array of six.  The COPY is what says the
//  array's element type is NOT the constructed type — an element of the
//  same type would be constructed in place.  Hence the plain/derived pair
//  above: the slice cannot be elided, so VC6 must build the temporary and
//  then block-copy it, which is exactly the shipped shape.
//
//  THE SIX ROWS, argument by argument.  Right-to-left pushes mean the
//  THIRD argument's `sub esp,0xc` comes first at each site; the frame slot
//  each temporary is built in rotates between the sites (E-0x48 / E-0x3c /
//  E-0x30) and that is allocation, not source.  Values read off the
//  immediates (0x3f800000 = 1.0f, 0xbf800000 = -1.0f):
//
//    [0] 0x0043c858..0x0043c8e5   (1,0,0) (0,0,-1) (0,1,0)   forward +Y
//    [1] 0x0043c902..0x0043c98f   (1,0,0) (0,0,1)  (0,-1,0)  forward -Y
//    [2] 0x0043c9a7..0x0043ca39   (1,0,0) (0,1,0)  (0,0,1)   forward +Z
//    [3] 0x0043ca56..0x0043cae3   (1,0,0) (0,-1,0) (0,0,-1)  forward -Z
//    [4] 0x0043cafb..0x0043cb8d   (0,0,1) (0,-1,0) (1,0,0)   forward +X
//    [5] 0x0043cbaa..0x0043cc37   (0,0,-1)(0,-1,0) (-1,0,0)  forward -X
//
//  Every one of the six is right-handed (Right x Up == Forward), which is
//  the independent check that the argument order above is not transposed.
// ======================================================================= //
SAxisBasis g_AxisBases[6] =
{
	CSideBasisQ43C(LTVector(1.0f, 0.0f,  0.0f),
	               LTVector(0.0f, 0.0f, -1.0f),
	               LTVector(0.0f, 1.0f,  0.0f)),

	CSideBasisQ43C(LTVector(1.0f,  0.0f, 0.0f),
	               LTVector(0.0f,  0.0f, 1.0f),
	               LTVector(0.0f, -1.0f, 0.0f)),

	CSideBasisQ43C(LTVector(1.0f, 0.0f, 0.0f),
	               LTVector(0.0f, 1.0f, 0.0f),
	               LTVector(0.0f, 0.0f, 1.0f)),

	CSideBasisQ43C(LTVector(1.0f,  0.0f,  0.0f),
	               LTVector(0.0f, -1.0f,  0.0f),
	               LTVector(0.0f,  0.0f, -1.0f)),

	CSideBasisQ43C(LTVector(0.0f,  0.0f, 1.0f),
	               LTVector(0.0f, -1.0f, 0.0f),
	               LTVector(1.0f,  0.0f, 0.0f)),

	CSideBasisQ43C(LTVector( 0.0f,  0.0f, -1.0f),
	               LTVector( 0.0f, -1.0f,  0.0f),
	               LTVector(-1.0f,  0.0f,  0.0f))
};


// ----------------------------------------------------------------------- //
//  ALSO WRITTEN — 0x0043ccb0, 81 bytes.  Included because it
//  is what names the array: it is the ONLY reader of it and its one caller
//  in the whole image is 0x004232f5, inside the world loader at 0x00422970,
//  where the result is shifted left 0x1d and packed into the top three bits
//  of a flags word (0x00423300..0x0043330b).  So the six records are the
//  six cardinal sides and this answers "which one does a direction face".
//
//  The loop runs a pointer from 0x004b3e94 (= &[0].m_vNormal.y) to
//  0x004b3f6c in steps of 0x24 (0x0043ccf0 / 0x0043ccf4) — six records —
//  and the running best starts at 0xc0000000 = -2.0f (0x0043ccb4).  The
//  branch is `test ah,0x41 ; jne` on the fnstsw, i.e. taken when the new
//  dot is NOT greater, so the guard is a plain `>`.
//
//  The sum is emitted x*x + z*z + y*y; /O2 canonicalises commutative FP
//  sums, so that is not readable as source order and Dot is
//  written the ordinary way.
// ----------------------------------------------------------------------- //
uint32 sb_GetSideQ43C(LTVector vDir)         // 0x0043ccb0
{
	uint32 i, nBest;
	float  fBest, fDot;

	nBest = 0;
	fBest = -2.0f;

	for (i = 0; i < 6; i++)
	{
		fDot = g_AxisBases[i].m_vNormal.Dot(vDir);

		if (fDot > fBest)
		{
			fBest = fDot;
			nBest = i;
		}
	}

	return nBest;
}


// ======================================================================= //
//                                PART B
//    0x00426670 / 0x00426c40 — CWorldBsp::Inherit and its node remap.
//
//  NOT THE SAME MODULE AS PART A.  The two clusters touch only through the
//  call graph (Part A's side picker is used by the world loader that lives
//  beside Part B), and they share no data, no class and no .CRT$XCU slot.
//
//  IDENTIFICATION IS NOT A GUESS.  src/engine/server/worldbsp.h already
//  carries this class, and three independent hooks pin it:
//
//   * the decoded symbol map already places
//     ?Term@CWorldBsp@@QAEXXZ at 0x00426480 — which is what 0x00426679
//     calls first;
//   * 0x004a9878 / 0x004a987c are worldbsp.h's g_pWorldTreeFrontTerm /
//     g_pWorldTreeBackTerm (server 0x10051a2c / 0x10051a30); they hold
//     0x004b2ff0 and 0x004b3008, which are 0x18 apart — two sentinel
//     records of the SAME 0x18-byte element the remap divides by;
//   * the inner element the copy loop assigns is 0xc0 bytes and its name
//     field is copied as a byte loop counted against `cmp 0x41`
//     (0x0042686a) — worldbsp.h records exactly that for CWorldSimple's
//     compiler-generated assignment, and sizeof(CWorldSimple) is 0xc0.
//
//  NO SERVER TWIN.  A sweep of the retail server.dll for the /24 magic
//  (0xb8 0xab 0xaa 0xaa 0x2a) finds nothing; lithtech.exe has exactly one,
//  at 0x00426c62.  So these two are the CLIENT-ONLY half of worldbsp's
//  source and there is no /O1 oracle for them.
//
//  The class view below is a local copy of worldbsp.h's, not an include:
//  that header pulls in servermgr.h.  When this lands it should use the
//  real classes; the two typing changes it needs are noted at the fields.
// ======================================================================= //

// 0x004a9878 / 0x004a987c.
extern void* g_pWorldTreeFrontTermQ43C;
extern void* g_pWorldTreeBackTermQ43C;


// worldbsp.h's CWorldItem18, with ONE typing change the client half forces:
// what that header names m_b and m_c is an ARRAY of two, because the copy
// at 0x00426724 walks them with a two-trip loop whose counter lives in
// [esp+0x10] and whose pointer steps by four.  Two separate statements do
// not produce a counted loop.
class CWorldItem18LinkQ43C
{
public:
	CWorldItem18LinkQ43C* m_pNext;           // +0x0c
	CWorldItem18LinkQ43C* m_pPrev;           // +0x10
};

class CWorldItem18Q43C
{
public:
	// worldbsp.h's body with the two side pointers retyped, and the 0xffff
	// store moved below the bytes: both expansions (0x004266aa..0x004266b4
	// and BaseNew 0x00428e02..0x00428e0a) store +0x16, +0x00, +0x17 THEN
	// +0x14, and %ecx is anchored at the first store (+0x16).  With m_x
	// first /O2 kept source order and anchored %ecx at +0x14.
	void Init(uint8 bInit)
	{
		CWorldItem18LinkQ43C* pHead;

		m_flag    = bInit;
		m_a       = 0;
		m_y       = 0;
		m_x       = 0xffff;
		m_Sides[1] = LTNULL;
		m_Sides[0] = LTNULL;

		pHead = &m_head;
		pHead->m_pPrev = pHead;
		pHead->m_pNext = pHead;
	}

	CWorldItem18Q43C(uint8 bInit = 0) { Init(bInit); }

	uint32               m_a;                // +0x00
	CWorldItem18Q43C*    m_Sides[2];         // +0x04 +0x08
	CWorldItem18LinkQ43C m_head;             // +0x0c
	uint16               m_x;                // +0x14
	uint8                m_flag;             // +0x16
	uint8                m_y;                // +0x17
};                                           //      = 0x18


// worldbsp.h's CWorldSub108, verbatim in layout (0x30 bytes).  Inherit
// copies it as one `rep movs` of twelve dwords (0x004268be, 0x00426af2),
// which is VC6's block form for an implicit assignment of a POD this size;
// the 0xc-byte LTVectors either side of it are copied three moves at a
// time, which is the same rule under the threshold.
class CWorldSub108Q43C
{
public:
	LTVector m_vCell;                        // +0x00
	LTVector m_vOrigin;                      // +0x0c
	int      m_nCountX;                      // +0x18
	int      m_nCountY;                      // +0x1c
	int      m_nCountZ;                      // +0x20
	int      m_nSliceStride;                 // +0x24
	uint32   m_nRecords;                     // +0x28
	void*    m_pRecords;                     // +0x2c
};                                           //      = 0x30


// worldbsp.h's CWorldBspBase.  Two virtuals declared and not defined, so
// the derived classes are concrete and store a vtable; the engine does not
// link, so that costs nothing.
class CWorldBspBaseQ43C
{
public:
	CWorldBspBaseQ43C();

	virtual void _b00();
	virtual void _b04();

	char m_Name[0x41];                       // +0x04
};                                           //      = 0x48


class CWorldSimpleHeaderQ43C
{
public:
	uint8  m_n00;                            // +0x00
	uint8  m_n01;                            // +0x01
	uint8  m_n02;                            // +0x02
	uint8  m_n03;                            // +0x03
	uint32 m_n04;                            // +0x04
};


class CWorldPartQ43C;

typedef CMoArray<CWorldPartQ43C*>   CWorldArray50Q43C;
typedef CMoArray<CWorldItem18Q43C>  CWorldArray68Q43C;


// worldbsp.h's CWorldSimple.  Only the LAYOUT matters here: the whole of
// this class's use in Inherit is its compiler-generated assignment, and
// the member ORDER below is read straight off 0x00426857..0x004268f2 —
// name, header, array, dword, array, dword, sub-object, vector, float.
class CWorldSimpleQ43C : public CWorldBspBaseQ43C
{
public:
	CWorldSimpleQ43C();
	~CWorldSimpleQ43C();

	virtual void*  _s10();
	virtual void*  GetItems();
	virtual uint32 GetSurfaceN48();
	virtual float  GetRadiusSqr();

public:
	CWorldSimpleHeaderQ43C m_Header48;       // +0x48
	CWorldArray50Q43C      m_Array50;        // +0x50
	uint32                 m_n64;            // +0x64
	CWorldArray68Q43C      m_Array68;        // +0x68
	void*                  m_p7c;            // +0x7c
	CWorldSub108Q43C       m_Sub80;          // +0x80
	LTVector               m_vCenter;        // +0xb0
	float                  m_fRadiusSqr;     // +0xbc
};                                           //      = 0xc0


typedef CMoArray<void*>            CWorldArray54Q43C;
typedef CMoArray<CWorldSimpleQ43C> CWorldArray138Q43C;


class CWorldBspQ43C : public CWorldBspBaseQ43C
{
public:
	CWorldBspQ43C();
	~CWorldBspQ43C();

	virtual void   _p00();
	virtual uint32 GetPolyCount();
	virtual HPOLY  MakeHPoly(void* pPoly);
	virtual uint32 FindPoly(void* p);
	virtual void*  GetPointStatus(LTVector vPoint);
	virtual void*  GetPart(uint32 nHandle);
	virtual uint32 GetTree();

	void   Init();
	void   Term();                           // 0x00426480 (already written)

	// >>> ROW 0x00426670 <<<
	LTBOOL Inherit(CWorldBspQ43C* pSrc);     // 0x00426670

	// >>> ROW 0x00426c40 <<<  Defined OUT OF CLASS and BELOW its
	// caller, which is the one construct /Ob1 never expands
	// — the shipped image CALLS it, twice, at 0x0042672b and
	// 0x0042676d.
	CWorldItem18Q43C* RemapItem(CWorldBspQ43C* pSrc, CWorldItem18Q43C* pItem);

public:
	uint32   m_nUnknown48;                   // +0x48
	uint32   m_Flags4c;                      // +0x4c  bit 0 = do not release
	uint32   m_nBytes;                       // +0x50
	CWorldArray54Q43C m_Array54;             // +0x54
	void*    m_p68;                          // +0x68
	uint32   m_n6c;                          // +0x6c
	// worldbsp.h types these two `uint32 m_n70` / `uint32 m_n74`, which is
	// all the server half needs.  The client half names them: 0x0042668b
	// allocates m_n74 records of 0x18 bytes into m_n70, and 0x00426c76
	// indexes m_n70 by an element count.
	CWorldItem18Q43C* m_pItems70;            // +0x70
	uint32   m_nItems74;                     // +0x74
	void*    m_pTree;                        // +0x78
	void*    m_pSurfaces;                    // +0x7c
	void*    m_p80;                          // +0x80
	uint32   m_n84;                          // +0x84
	void*    m_p88;                          // +0x88
	uint32   m_n8c;                          // +0x8c
	void*    m_p90;                          // +0x90
	uint32   m_n94;                          // +0x94
	uint32   m_n98;                          // +0x98
	// Also retyped from worldbsp.h's `uint32 m_n9c`: 0x00426763 hands it
	// to the node remap, so it is a node pointer.
	CWorldItem18Q43C* m_p9c;                 // +0x9c
	void*    m_ppParts;                      // +0xa0
	uint32   m_nParts;                       // +0xa4
	void*    m_pa8;                          // +0xa8
	uint32   m_nac;                          // +0xac
	void*    m_pChunks;                      // +0xb0
	uint32   m_nChunks;                      // +0xb4
	void*    m_pb8;                          // +0xb8
	void*    m_pbc;                          // +0xbc
	uint32   m_nc0;                          // +0xc0
	LTVector m_vBoxMin;                      // +0xc4
	LTVector m_vBoxMax;                      // +0xd0
	uint32   m_ndc;                          // +0xdc
	uint32   m_ne0;                          // +0xe0
	uint32   m_ne4;                          // +0xe4
	LTVector m_ve8;                          // +0xe8
	void*    m_pf4;                          // +0xf4
	uint16   m_nf8;                          // +0xf8
	uint8    m_pad_fa[0xfc - 0xfa];
	void*    m_pfc;                          // +0xfc
	uint32   m_n100;                         // +0x100
	void*    m_p104;                         // +0x104
	CWorldSub108Q43C   m_Sub108;             // +0x108
	CWorldArray138Q43C m_Array138;           // +0x138
	float    m_fDiagonal;                    // +0x14c
};


// ----------------------------------------------------------------------- //
//  collection.h's BaseNew, one level down: the allocation at 0x0042668c is
//  a ONE-ARGUMENT CDECL call to 0x004290b0, not a virtual through an
//  LAlloc, so this site does not go through the allocator object.
//
//  NOT BaseNew's loop, though it is BaseNew-shaped:
//    0x00426688  `shl eax,0x3` after `lea eax,[edi+edi*2]`  = n * 0x18
//    0x00426698  `je 0x004266cd` -> `xor eax,eax`           = return 0,
//                hosted at the bottom: a POSITIVE `if (ptr)` guard
//    0x004266c3  `add ecx,0x18` / 0x004266c6 `dec edi`      = ++pTemp on a
//                countdown trip count.
//  The zero-trip guard is `lea edx,[edi-1] ; cmp edx,esi ; jl` at
//  0x0042669a — a SIGNED test of (n-1): an int counter run from n-1 down
//  to 0.  And there is no null test on each record before its stores,
//  which placement `new` (BaseNew, 0x00428df8 `test edx,edx`) always
//  emits: the records are initialised by a direct Init call.
//
//  INLINE and defined ABOVE its caller because VC6's inliner is
//  forward-only.
// ----------------------------------------------------------------------- //
inline CWorldItem18Q43C* wb_NewItemsQ43C(uint32 nElements)
{
	CWorldItem18Q43C *ptr, *pTemp;
	int i;

	ptr = (CWorldItem18Q43C*)dalloc(sizeof(CWorldItem18Q43C) * nElements);
	if (ptr)
	{
		pTemp = ptr;
		for (i = (int)nElements - 1; i >= 0; i--)
		{
			pTemp->Init(0);
			++pTemp;
		}

		return ptr;
	}

	return LTNULL;
}


// ======================================================================= //
//  >>> ROW 0x00426670, 1197 bytes (0x00426670..0x00426b1c). <<<
//
//  EXTENT: ONE FUNCTION, PROVED.  A sweep of every `e8`/`e9` in .text for a
//  displacement landing inside [0x00426670, 0x00426b1d) finds exactly ONE
//  entry, 0x00424a28's `call 0x00426670`.  Every other transfer inside the
//  range is a short jump of its own.  0x00426b1a is `ret 0x4` and
//  0x00426b1d..0x00426b1f are three alignment 0x90s:
//  0x00426b1d - 0x00426670 = 0x4ad = 1197 exactly.  The row is sound and
//  the "really several functions" defect this size band invites is absent.
//
//  SIGNATURE: `ret 0x4` with %ecx live and %ebp taken as `this`
//  (0x00426677) — __thiscall, one argument, read at [esp+0x24] once the
//  0x10-byte frame and four pushes are in (0x0042667e = E+4).  The return
//  value is 1 on success (0x00426af7) and 0 on either failure.
//
//  THE TWO FAILURES ARE GENUINE EARLY RETURNS, not a shared tail: each of
//  0x004266d6 and 0x0042680a carries its OWN five-instruction epilogue
//  (pop edi/esi/ebp, xor eax,eax, pop ebx, add esp,0x10, ret 4).  /O2
//  PRESERVES duplicated epilogues, so that reads as written
//  early returns, and they are written that way below.
// ======================================================================= //
LTBOOL CWorldBspQ43C::Inherit(CWorldBspQ43C* pSrc)
{
	uint32 i, j;

	// 0x00426679 — the only call before the argument is even loaded.
	Term();

	// 0x0042667e..0x004266d1.  The count comes from pSrc, the storage from
	// the allocator, and the result is stored to this->m_pItems70 whether
	// it succeeded or not (0x004266d1 is on the merged path).
	m_pItems70 = wb_NewItemsQ43C(pSrc->m_nItems74);

	if (!m_pItems70)
		return LTFALSE;                      // 0x004266d4 jne / 0x004266d6

	// 0x004266e2.  `jbe` — the count is unsigned — and the bound is
	// RE-READ at the bottom of the loop (0x00426748), so it is not cached
	// in a local.
	for (i = 0; i < pSrc->m_nItems74; i++)
	{
		// Both records are addressed once per trip (0x004266ff..
		// 0x00426703) and never re-read across the RemapItem calls: two
		// pointer locals, not subscripts.
		CWorldItem18Q43C* pSrcItem = &pSrc->m_pItems70[i];
		CWorldItem18Q43C* pDstItem = &m_pItems70[i];
		CWorldItem18LinkQ43C* pHead;

		// 0x00426712 — a 16-bit move, the only field copied whole.
		pDstItem->m_x = pSrcItem->m_x;

		// 0x0042671c..0x00426722 — `lea edx,[eax+0xc]` then [edx+4] then
		// [edx], i.e. m_pPrev BEFORE m_pNext, the same order
		// CWorldItem18::Init uses.  The new list is empty, not the old
		// one's links copied.
		pHead = &pDstItem->m_head;
		pHead->m_pPrev = pHead;
		pHead->m_pNext = pHead;

		// 0x00426705 sets a trip count of 2 in [esp+0x10] and 0x00426724
		// walks a pointer by four: a REAL two-trip loop in the source, not
		// two statements.  The source element is reached off the
		// destination pointer plus a constant delta (0x00426716
		// `sub edi,eax`), which is the two arrays indexed alike.
		for (j = 0; j < 2; j++)
		{
			pDstItem->m_Sides[j] = RemapItem(pSrc, pSrcItem->m_Sides[j]);
		}
	}

	// 0x0042675d / 0x00426763.
	m_nItems74 = pSrc->m_nItems74;
	m_p9c = RemapItem(pSrc, pSrc->m_p9c);

	// 0x00426778..0x0042692d — CopyArray2 EXPANDED, with BaseDelete
	// (0x00426797) and BaseNew (0x004267ec) left as calls and the
	// allocator 0x004b4e88 = g_DefAlloc pushed to both.  Spelt
	// `CopyArray(...)` and not `CopyArray2(..., &g_DefAlloc)`: the explicit
	// form loses an inline level and would expand BaseNew here, which the
	// shipped image does not (collection.h's depth note).  The result IS tested — 0x004267fc returns LTFALSE — so
	// this is not the `operator=` spelling, which would discard it.
	//
	// Inside it, the per-element `m_pArray[i] = other.m_pArray[i]` is
	// CWorldSimple's compiler-generated assignment EXPANDED
	// (0x00426857..0x004268f2); the standalone body is 0x004281c0 and this
	// site does not call it.
	if (!m_Array138.CopyArray(pSrc->m_Array138))
		return LTFALSE;                      // 0x004267fc jne / 0x0042680a

	// 0x0042692d onward: a straight run of field copies.  The ORDER below
	// is the shipped store order and it is not the layout's — m_n100 comes
	// between m_nParts and m_pa8, and the name copy sits between m_nc0 and
	// m_vBoxMin.  Each line carries the address of its store.
	m_p68       = pSrc->m_p68;               // 0x00426933
	m_n6c       = pSrc->m_n6c;               // 0x00426939
	m_pTree     = pSrc->m_pTree;             // 0x0042693f
	m_pSurfaces = pSrc->m_pSurfaces;         // 0x00426945
	m_p80       = pSrc->m_p80;               // 0x0042694e
	m_n84       = pSrc->m_n84;               // 0x0042695a
	m_p88       = pSrc->m_p88;               // 0x00426966
	m_n8c       = pSrc->m_n8c;               // 0x00426972
	m_p90       = pSrc->m_p90;               // 0x0042697e
	m_n94       = pSrc->m_n94;               // 0x0042698a
	m_n98       = pSrc->m_n98;               // 0x00426996
	// AND THIS ONE OVERWRITES THE REMAP twelve hundred bytes earlier.
	// 0x00426772 stored RemapItem's answer into the same +0x9c that
	// 0x004269a2 now clobbers with the raw source pointer.  It is in the
	// shipped bytes twice over and it is reproduced, not corrected.
	m_p9c       = pSrc->m_p9c;               // 0x004269a2
	m_ppParts   = pSrc->m_ppParts;           // 0x004269ae
	m_nParts    = pSrc->m_nParts;            // 0x004269ba
	m_n100      = pSrc->m_n100;              // 0x004269c6
	m_pa8       = pSrc->m_pa8;               // 0x004269d2
	m_nac       = pSrc->m_nac;               // 0x004269de
	m_pChunks   = pSrc->m_pChunks;           // 0x004269ea
	m_nChunks   = pSrc->m_nChunks;           // 0x004269f6
	m_pb8       = pSrc->m_pb8;               // 0x00426a02
	m_pbc       = pSrc->m_pbc;               // 0x00426a11
	m_nc0       = pSrc->m_nc0;               // 0x00426a21

	// 0x00426a27 `call 0x00492b60` with (dst, src, 0x40) — the CRT
	// strncpy, not a struct copy — and 0x00426a32 terminates by hand at
	// [dst+0x40]: ltbasedefs.h's LTStrCpy through SAFE_STRCPY, its pDest
	// held in %esi from 0x00426930.  Spelt as two plain statements, the
	// lost inline site let VC6 spend its expansions deeper and expand
	// BaseDelete / BaseNew and the +0x68 array's CopyArray2 below the
	// m_Array138 copy (1408 bytes against 1197).
	SAFE_STRCPY(m_Name, pSrc->m_Name);

	m_vBoxMin   = pSrc->m_vBoxMin;           // 0x00426a41..0x00426a52
	m_vBoxMax   = pSrc->m_vBoxMax;           // 0x00426a61..0x00426a6c
	m_ndc       = pSrc->m_ndc;               // 0x00426a75
	m_ne0       = pSrc->m_ne0;               // 0x00426a81
	m_ne4       = pSrc->m_ne4;               // 0x00426a93
	m_ve8       = pSrc->m_ve8;               // 0x00426aa1..0x00426aac
	m_pf4       = pSrc->m_pf4;               // 0x00426ab5
	m_nf8       = pSrc->m_nf8;               // 0x00426ac2 (16-bit)
	m_pfc       = pSrc->m_pfc;               // 0x00426acf
	m_p104      = pSrc->m_p104;              // 0x00426aec
	// 0x00426af2 `rep movs` with %ecx = 0xc — 0x30 bytes in one block.
	m_Sub108    = pSrc->m_Sub108;

	m_nUnknown48 = pSrc->m_nUnknown48;       // 0x00426afc
	m_fDiagonal  = pSrc->m_fDiagonal;        // 0x00426b0b

	// 0x00426b08 `or ecx,eax` with %eax holding the 1 that 0x00426af7
	// materialised for the return, and 0x00426b11 stores it back.  The
	// register form rather than `or mem,$1` is why the two share one
	// constant.  worldbsp.h: bit 0 of m_Flags4c is "do not release" — the
	// geometry now belongs to pSrc.
	m_Flags4c |= 1;

	return LTTRUE;
}


// ======================================================================= //
//  >>> ROW 0x00426c40, 71 bytes (0x00426c40..0x00426c86). <<<
//
//  `ret 0x8` with %ecx as the DESTINATION world (0x00426c76 reads
//  [ecx+0x70]) and the first stack argument as the source (0x00426c5a).
//  Called only from inside 0x00426670, twice.
//
//  0x00426c62 `mov eax,0x2aaaaaab` + `imul` + `sar edx,0x2` + the sign
//  fixup is VC6's signed divide by 24, and 0x00426c79
//  `lea edx,[edx+edx*2]` + `lea eax,[eax+edx*8]` is the multiply back —
//  so the whole body is one array-index translation between two runs of
//  0x18-byte records.
//
//  THE TWO SENTINELS ARE RETURNED AS THE GLOBAL, NOT AS THE ARGUMENT.
//  0x00426c4b and 0x00426c54 jump to 0x00426c84 with %eax still holding
//  what was loaded from 0x004a9878 / 0x004a987c; a `return pItem;` would
//  have cost a `mov eax,edx`.  The values are equal either way, so the
//  spelling below is the one that emits nothing.
//
//  The null case is separate and materialises its own zero (0x00426c82),
//  which is why it is a third `if` and not folded into the first two.  It
//  is a POSITIVE guard: 0x00426c58 is `je` forward to that zero at the
//  bottom, with the remap as the fall-through; `if (!pItem) return 0;`
//  gives `jne` and puts the zero-return first.
// ======================================================================= //
CWorldItem18Q43C* CWorldBspQ43C::RemapItem(CWorldBspQ43C* pSrc,
                                           CWorldItem18Q43C* pItem)
{
	if (pItem == g_pWorldTreeFrontTermQ43C)
		return (CWorldItem18Q43C*)g_pWorldTreeFrontTermQ43C;   // 0x00426c49

	if (pItem == g_pWorldTreeBackTermQ43C)
		return (CWorldItem18Q43C*)g_pWorldTreeBackTermQ43C;    // 0x00426c52

	if (pItem)                                                 // 0x00426c58 je
		return &m_pItems70[pItem - pSrc->m_pItems70];          // 0x00426c5a

	return LTNULL;                                             // 0x00426c82
}


// ======================================================================= //
//  CLOSING NOTES — extents, placement, and the coin flips.
//
//  EXTENT DEFECTS: NONE.  All five rows were swept for `e8`/`e9`
//  displacements landing inside them; every hit is the entry point itself
//  (0x0043c3e0 -> 0x0043c3f0, 0x00423dc5 -> 0x0043c6f0,
//  0x0043c840 -> 0x0043c850, 0x00424a28 -> 0x00426670, and 0x0042672b /
//  0x0042676d -> 0x00426c40).  Sizes check byte for byte:
//    0x0043c422 - 0x0043c3f0 = 0x32  = 50
//    0x0043c7db - 0x0043c6f0 = 0xeb  = 235
//    0x0043cc57 - 0x0043c850 = 0x407 = 1031
//    0x00426b1d - 0x00426670 = 0x4ad = 1197
//    0x00426c87 - 0x00426c40 = 0x47  = 71
//  The `thunk` provenance on 0x0043c850 is a mislabel, not a bad extent:
//  the thunk is the separate 5-byte row 0x0043c840.  No split is needed
//  for any of the five.
//
//  WHAT BELONGS IN A HEADER WHEN THIS LANDS.
//   * Part B's class views are a local copy of src/engine/server/worldbsp.h
//     and should be REPLACED by that header, with two typing changes it
//     needs and does not yet have: `uint32 m_n70` becomes
//     `CWorldItem18* m_pItems70`, `uint32 m_n9c` becomes
//     `CWorldItem18* m_p9c`, and CWorldItem18's `m_b`/`m_c` become
//     `m_Sides[2]` (the two-trip loop at 0x00426724 is the proof).  The
//     m_Sides change also rewrites CWorldItem18::Init's two zero stores,
//     which stay in the same order (+0x08 then +0x04).
//
//  UNSETTLED, WITH BOTH SPELLINGS.
//
//  (1) 0x0043c850 — how the six-element array is declared.  Written here
//      as a plain array with non-constant initializers:
//
//          SAxisBasis g_AxisBases[6] = { CSideBasisQ43C(...), ... };
//
//      which guarantees a VOID initializer with no `this` (0x0043c850 has
//      neither) and a temporary-then-block-copy at each element (the six
//      `rep movs $9`).  The alternative is a file-scope object whose
//      IN-CLASS constructor performs six assignments into its own member
//      array:
//
//          class CSideBasisTableQ43C {
//          public:
//              CSideBasisTableQ43C()
//              { m_Bases[0] = CSideBasisQ43C(...); ... }
//              SAxisBasis m_Bases[6];
//          };
//          static CSideBasisTableQ43C g_SideBasisTableQ43C;
//
//      Same instruction stream IF VC6 expands the 1031-byte constructor
//      into `??__E`; if it declines, the initializer becomes
//      `mov ecx,<addr> ; jmp <ctor>` and the row is missed outright.  The
//      array form cannot fail that way, which is why it is the one written.
//
//  (2) 0x00426670 — the node-array allocation loop.  SETTLED: the
//      signed countdown with a direct Init per record (see
//      wb_NewItemsQ43C); the BaseNew body gave an unsigned guard and a
//      per-record null test.  0x00426670 is now byte-identical.
//
//  (3) 0x00426670 — the array copy.  Written `CopyArray(...)` and tested,
//      on collection.h's own depth rule (BaseNew is CALLED at 0x004267ec,
//      which puts it at depth 4).  The two neighbours are
//      `CopyArray2(pSrc->m_Array138, &g_DefAlloc)`, which loses a level and
//      should EXPAND BaseNew — measurably wrong if it does — and
//      `m_Array138 = pSrc->m_Array138;`, which is ruled OUT rather than
//      merely disfavoured: `operator=` discards CopyArray2's result and
//      0x004267fc returns LTFALSE on it.
//
//  (4) 0x00426c40 — the two sentinel arms return the GLOBAL rather than the
//      argument, because %eax already holds the global and a
//      `return pItem;` would have cost `mov eax,edx`.  The values are
//      always equal, so if VC6 copy-propagates the equality the two
//      spellings converge and this costs nothing either way.
// ======================================================================= //
