// server.dll — IWorldBsp, the world geometry (reconstruction). Build /O1.
#include "worldbsp.h"
#include "commonmodel.h"   // Model::CModelSub14, the member at CWorldSurface+0x28
#include "servermgr.h"   // WorldLoadRequest
#include <stdio.h>      // sprintf, for fs_GetTableName

// 0x10058360 (.bss) -- worldbsp.h's live world-geometry byte count, defined
// here, the file that adds to it (w_LoadWorldModel) and takes it back off
// (Term); commonmodel.cpp's w_SetupPointShade adds to it too, through an
// extern of this one definition.
uint32 g_WorldBytes;

// CServerWorldFull is a class local to commonmodel.cpp, not a header type, and
// this function touches exactly one of its fields -- the world-object list head
// at +0xf4 (0x10008cee / 0x10008cf1). Declared as an opaque type here and
// reached through a view, so neither file has to learn the other's layout.
class CServerWorldFull;

class CServerWorldListQT1
{
public:
	uint8  m_pad_00[0xf4];
	uint32 m_n0f4;                           // 0x0f4  list head
};

// Everything the world holds, cleared in one pass. The order is the source's
// own and not the layout's: the flags and the byte count go first, the long
// run of pointers and counts next, and the field at 0x48 last of all.
void CWorldBsp::Init()
{
	m_Flags4c = 0;
	m_nBytes  = 0;

	m_p68 = LTNULL;
	m_n6c = 0;
	m_n70 = 0;
	m_n74 = 0;
	m_pTree = LTNULL;
	m_pSurfaces = LTNULL;
	m_p80 = LTNULL;
	m_n84 = 0;
	m_p88 = LTNULL;
	m_n8c = 0;
	m_p90 = LTNULL;
	m_n94 = 0;
	m_n98 = 0;
	m_n9c = 0;
	m_ppParts = LTNULL;
	m_nParts = 0;
	m_pa8 = LTNULL;
	m_nac = 0;
	m_pChunks = LTNULL;
	m_nChunks = 0;
	m_pb8 = LTNULL;
	m_pbc = LTNULL;
	m_nc0 = 0;

	m_vBoxMin.Init();
	m_vBoxMax.Init();

	m_ndc = 0;
	m_ne0 = 0;
	m_ne4 = 0;

	m_ve8.Init();

	m_pf4 = LTNULL;
	m_nf8 = 0;
	m_pfc = LTNULL;
	m_n100 = 0;
	m_p104 = LTNULL;

	m_nUnknown48 = 0;
}

// Giving the world back. A world whose flags carry bit zero owns nothing —
// its geometry belongs to someone else — so the whole release is skipped and
// only the reset runs. The chunk run is walked before it is released, since
// each record owns an allocation of its own.
//
// The order the pointers go back in is not the order they are declared in,
// and it is preserved here because it is what shipped.
void CWorldBsp::Term()
{
	uint32 i;

	if (!(m_Flags4c & 1))
	{
		m_Array138.Term(&g_DefAlloc);
		m_Sub108.Term();

		for (i = 0; i < m_nChunks; i++)
		{
			if (m_pChunks[i].m_pData)
				dfree(m_pChunks[i].m_pData);
		}

		if (m_pChunks)
			dfree(m_pChunks);

		if (m_p80)
			dfree(m_p80);

		if (m_pfc)
			dfree(m_pfc);

		if (m_p104)
			dfree(m_p104);

		if (m_p88)
			dfree(m_p88);

		if (m_p90)
			dfree(m_p90);

		if (m_ppParts)
			dfree(m_ppParts);

		if (m_pa8)
			dfree(m_pa8);

		if (m_p68)
			dfree(m_p68);

		// 0x1000796d is `??_ECWorldTree` — VC6's vector deleting destructor —
		// and the three the shipped code pushes is its flag word: array plus
		// free. So this is one `delete []`, and the null test the shipped code
		// has around it is the compiler's own, not a guard in the source.
		delete [] m_pTree;

		if (m_pbc)
			dfree(m_pbc);

		if (m_pb8)
			dfree(m_pb8);

		if (m_pf4)
			dfree(m_pf4);

		g_WorldBytes -= m_nBytes;
	}

	FreeTree();
	Init();
}

// Construction is the three members and then the same reset the teardown
// ends with; there is nothing else to set up.
CWorldBsp::CWorldBsp()
{
	Init();
}

// The members' own destructors come out of collection.h, so both are inlined
// here — each restoring its vtable and resizing to nothing. One vtable store
// apiece, which is what says neither array derives from anything.
CWorldBsp::~CWorldBsp()
{
	Term();
}

// The tree the world builds for itself goes back through the engine's delete
// rather than the allocator's release, and the pointer is cleared, because
// this runs on the way to a reset rather than to destruction.
void CWorldBsp::FreeTree()
{
	if (m_n70)
	{
		operator delete((void*)m_n70);

		m_n70 = 0;
	}
}

void CWorldBsp::Recalc()
{
	m_fDiagonal = wb_MaxDistSqr(m_ppParts, m_nParts, &m_ve8);
}

// 0x10038e77 (lithtech 0x48b8e0) — fntdraw.cpp's visibility traversal, not
// reconstructed. Cdecl, one pointer: the `pop %ecx` after the call.
void wt_VisibilityTraversal(CWorldVisQuery* pQuery);

// 0x1000ae99 — slot 9, between Recalc and GetInfoFlags in both images
// (lithtech 0x426b50). The query goes on; the world itself does not.
void CWorldBsp::DoVisQuery(CWorldVisQuery* pQuery)
{
	wt_VisibilityTraversal(pQuery);
}

uint32 CWorldBsp::GetWorldFlags()
{
	return m_nUnknown48;
}

uint32 CWorldBsp::IsFlagSet()
{
	return (m_nUnknown48 >> 5) & 1;
}

float CWorldBsp::GetRadiusSqr()
{
	return m_fDiagonal;
}

uint32 CWorldBsp::GetRoot()
{
	return m_n9c;
}

// Slot 3. One of the parts by the low sixteen bits of its argument, and
// nothing at all for an index the count does not reach. The mask is a plain
// `and $0xffff` rather than a movzx, and the compare that follows it is
// unsigned (`jae`) against the full-dword count.
void* CWorldBsp::FindPoly(HPOLY nHandle)
{
	uint32 iPart = nHandle & 0xffff;

	if (iPart < m_nParts)
		return m_ppParts[iPart];

	return LTNULL;
}

// Slot 5: four bytes, the same pointer FreeTree releases, handed out raw.
void* CWorldBsp::GetPolyRefs()
{
	return (void*)m_n70;
}

// Slot 0: the BSP world is never busy. 0x10014d2f is the linker-shared
// `xor %eax,%eax; ret` (the simple world's slot 7 is the same stub).
uint32 CWorldBsp::IsBusy()
{
	return 0;
}

// A poly becomes a handle only if it really belongs to this world: its record
// has to sit inside the run the world owns. Anything else is all-ones.
HPOLY CWorldBsp::MakeHPoly(void* pRef)
{
	CWorldPoly* pPoly = ((CWorldPolyRef*)pRef)->m_pPoly;

	if ((uint8*)pPoly >= (uint8*)m_pfc &&
	    (uint8*)pPoly < (uint8*)m_pfc + m_n100)
	{
		return (m_nf8 << 16) | pPoly->m_nIndex;
	}

	return 0xffffffff;
}

// The base is a vtable and the world's name, and the name goes first: the
// shipped constructor's one field store is a single byte at +0x04
// (`andb $0x0,0x4(%eax)` at /O1, `movb $0x0,0x4(%eax)` at /O2), which is an
// empty string in a char array rather than a scalar being zeroed. See the
// note on CWorldBspBase for why the array is 0x41 bytes and why it has to
// live in the base.
CWorldBspBase::CWorldBspBase()
{
	m_Name[0] = 0;
}

// Each record owns one allocation, and they go before the run does.
void CWorldSub108::Term()
{
	uint32 i;

	for (i = 0; i < m_nRecords; i++)
		dfree(m_pRecords[i].m_pData);

	dfree(m_pRecords);
}

CWorldSimple::CWorldSimple()
{
	m_n64 = (uint32)g_pWorldTreeBackTerm;
	m_p7c = LTNULL;
}

CWorldSimple::~CWorldSimple()
{
	Reset();
}

// Emptying without destroying: the arrays are resized to nothing rather than
// destructed, so the object can be filled again.
void CWorldSimple::Reset()
{
	m_Array50.Term(&g_DefAlloc);

	m_n64 = (uint32)g_pWorldTreeBackTerm;

	m_Array68.Term(&g_DefAlloc);

	m_p7c = LTNULL;

	m_Sub80.Term();
}

// The simple world's slots 2 and 3 (0x1000ab53, 0x1000ab62): two of its
// questions belong to its surface table rather than to the world, and are
// handed straight on.
HPOLY CWorldSimple::MakeHPoly(void* pPoly)
{
	return m_p7c->MakeHPoly(pPoly);
}

void* CWorldSimple::FindPoly(HPOLY hPoly)
{
	return (void*)m_p7c->FindPoly((void*)hPoly);
}

// The point test belongs to the sub-object, and the point goes to it by
// value — the world does no more than name which of its parts answers.
CSubRecord* CWorldBsp::GetPointStatus(LTVector vPoint)
{
	return m_Sub108.GetPointStatus(vPoint);
}

// Which cell a point falls in. The point is turned into three indices, each
// clamped to its own count, and the cell is looked up by them. The three
// negative tests afterwards cannot fire once the clamping has run — they are
// in the shipped code all the same.
CSubRecord* CWorldSub108::GetPointStatus(LTVector vPoint)
{
	int ix = (int)((vPoint.x - m_vOrigin.x) / m_vCell.x);
	int iy = (int)((vPoint.y - m_vOrigin.y) / m_vCell.y);
	int iz = (int)((vPoint.z - m_vOrigin.z) / m_vCell.z);

	if (ix < 0)
		ix = 0;
	else if (ix > m_nCountX - 1)
		ix = m_nCountX - 1;

	if (iy < 0)
		iy = 0;
	else if (iy > m_nCountY - 1)
		iy = m_nCountY - 1;

	if (iz < 0)
		iz = 0;
	else if (iz > m_nCountZ - 1)
		iz = m_nCountZ - 1;

	if (ix < 0 || iy < 0 || iz < 0)
		return LTNULL;

	return &m_pRecords[ix + iz * m_nSliceStride + iy * m_nCountX];
}

// The furthest any of the world's own positions gets from a point, and it is
// a real distance rather than a square despite this function's name.
//
// The square was a mis-read that /O1 could not catch. At /O1 both spellings
// are one call to a sixty-six-byte `(*this - vec).Mag<Sqr>()` COMDAT whose
// only difference is the address in its tail `call`, and the byte comparison masks
// relocations -- so `DistSqr` "matched" 0x1000af58 in server.dll while that
// COMDAT's own tail call goes to 0x10005502, which sums three squares and
// calls the CRT's sqrt. It is `Dist`. The /O2 client settles it from the
// other side: 0x004260c0 expands the callee and an `fsqrt` sits at 0x0042612e
// between the last `faddp` and the three `fstp`s, which is where the root
// would be and is the two bytes our 155 is short of the shipped 157. Both
// spellings still emit the same bytes here at /O1, so server.dll is unmoved;
// the COMDAT this TU contributes becomes Dist/Mag instead of DistSqr/MagSqr,
// and seven other server objects emit each of those four, so no address
// changes hands.
//
// Three locals and no frame beyond the four bytes fMax needs, because the two
// loop counters live in the incoming parameters' slots: the shipped code
// stores the part count into ppParts' slot at 0x8(%ebp) and counts down there,
// and clears nParts' slot at 0xc(%ebp) for the vertex index. That is what
// forced the two copies at the top of the `if`. A slot is only available once
// the parameter that owns it is dead, and walking `ppParts` itself keeps it
// alive: written that way the count stays in its own slot, the vertex index
// takes ppParts', and fMax's neighbour becomes a fourth slot the shipped frame
// does not have — 98 bytes against 97, with the extra `push %ecx` to show it.
// Copying the parameter into `pp` and walking that frees the slot, and the
// ebx/edi rotation that came with the miss went with it too: pp lands in %edi
// and the vertex cursor in %ebx, the way it shipped. A register rotation is
// again a consequence of a misplaced local rather than a lottery.
float wb_MaxDistSqr(CWorldPart** ppParts, uint32 nParts, LTVector* pPoint)
{
	float        fMax = 0.0f;
	CWorldPart** pp;
	uint32       nLeft;
	uint32       j;

	if (nParts > 0)
	{
		pp    = ppParts;
		nLeft = nParts;

		do
		{
			CWorldPart* pPart = *pp;

			for (j = 0; j < pPart->m_nVerts; j++)
			{
				float f = pPoint->Dist(*pPart->m_Verts[j].m_pPos);

				if (f > fMax)
					fMax = f;
			}

			pp++;
		}
		while (--nLeft);
	}

	return fMax;
}

// The two array fields go through locals: the shipped code loads the count and
// then the storage into registers of their own before it takes the centre's
// address, where pushing the members straight (`push 0x58(%esi)`) is two bytes
// shorter and was what this used to do. The count is loaded first, so it is
// the local declared first — the same rule as the collection's grow-by, and
// it applies to a member pushed as an argument as much as to one added to
// something.
void CWorldSimple::ComputeRadius()
{
	uint32       nParts  = m_Array50.m_nElements;
	CWorldPart** ppParts = m_Array50.m_pArray;

	m_fRadiusSqr = wb_MaxDistSqr(ppParts, nParts, &m_vCenter);
}


// ---- the simple world's own vtable slots -------------------------------- //
//
// Slots 4, 5, 6 and 8 of 0x1004d380. Each is one load, and the offsets are
// what say the run belongs to this class rather than to the BSP world: +0x80
// is this class's sub-object, +0x6c its second array's storage, +0x7c its
// surface table and +0xbc its radius. See the note on CWorldBsp's own slots.

// The point test belongs to the sub-object here too — the same shape as the
// BSP world's slot 4, with 0x80 in the `add` instead of 0x108.
CSubRecord* CWorldSimple::GetPointStatus(LTVector vPoint)
{
	return m_Sub80.GetPointStatus(vPoint);
}

// Four bytes: the storage the array at +0x68 is holding, handed out raw.
void* CWorldSimple::GetPolyRefs()
{
	return m_Array68.m_pArray;
}

// Two loads: through the surface table and out of its +0x48.
uint32 CWorldSimple::GetWorldFlags()
{
	return m_p7c->m_n48;
}

float CWorldSimple::GetRadiusSqr()
{
	return m_fRadiusSqr;
}

// Slot 0 (0x100355db, the linker-shared `push 1; pop %eax; ret`): the
// simple world is always busy.
uint32 CWorldSimple::IsBusy()
{
	return 1;
}

// Slot 1 (0x1000ab4f, four bytes): the root node at +0x64, which Reset
// points at the back terminator.
uint32 CWorldSimple::GetRoot()
{
	return m_n64;
}

// Slot 7 (0x10014d2f, the linker-shared `xor %eax,%eax; ret`).
uint32 CWorldSimple::IsFlagSet()
{
	return 0;
}


// ---- what used to live here ---------------------------------------------- //
//
// The four array constructors, `wa138_FreeItems`, `CWorldArray138::SetSize`
// and `CWorldSub108::Clear` were all defined below this line. All four
// classes are `CMoArray<T, DefaultCache>` instantiations now (see the block
// at the head of worldbsp.h) and Clear is in the class, so every one of those
// bodies comes out of collection.h instead. What each of them was, and what
// the shipped bytes had already said about it, is kept here because the
// readings are still the evidence for the instantiations:
//
//   * THE FOUR CONSTRUCTORS (0x1000b15c, 0x1000b295, 0x1000b4db, 0x1000b591).
//     Written by hand they were NEAR, four times over — 34 bytes against 34
//     and the same fourteen instructions, the whole residual being where one
//     of Init's two argument pushes sits. The shipped build spends the zero
//     as `mov %ecx,%esi; xor %eax,%eax; push %eax; <vptr>; <0x4>; <0x8>;
//     <0xc>; push %eax; <0x10>; call`, putting one push before the vtable
//     store and the other between the third and fourth field stores; ours
//     materialised the zero first and emitted both pushes together ahead of
//     the vtable store. Nothing in the source moved it: the field order was
//     probed, the four stores were tried as an inline `Clear()` member, both
//     arguments were tried as the members that had just been zeroed, a local
//     zero was tried in place of the two literals, and all four held compiler
//     revisions (8168, 8447, 8804, 8966) emitted our schedule. A four-field
//     non-polymorphic base whose inline constructor zeroed them was probed
//     too and MOVED the residual — it fixes the push schedule outright but
//     drops the vtable store below the four field stores, where the shipped
//     store is first.
//
//     `CMoArray<T, C>::CMoArray() { Clear(); Init(); }` is the spelling that
//     gives it: collection.h's Clear writes +0x4, +0x8, +0xc, +0x10 after the
//     vptr, Init's two zero arguments are the pushes, and the template body
//     matches 0x1000af9a — the same 34 bytes as all twelve other
//     `CMoArray<T, DefaultCache>` constructors in server.dll. So the residual
//     was never a scheduler lottery; it was the wrong construct.
//
//   * `wa138_FreeItems` (0x1000d181, 52 bytes) is `BaseDelete<CWorldSimple>`
//     and `wa138_AllocItems` (0x1000d1b5, 61) is `BaseNew<CWorldSimple>` —
//     LDelete_Array and LNew_Array, which are FREE FUNCTION templates, which
//     is why neither took a `this` and why the resize had to null the fields
//     itself afterwards. The release's whole body sits inside a null test on
//     the storage, walks the count DOWN, hands the allocator the *parameter*
//     rather than the walk's cursor, and runs 0x1000a849 — ~CWorldSimple —
//     over every element stepping 0xc0 bytes: which is what identifies the
//     element type, and the allocation agrees by placement-constructing each
//     slot with 0x1000a7de.
//
//   * `CWorldArray138::SetSize` (0x1000c0e7, 112 bytes) is
//     `CMoArray<CWorldSimple, DefaultCache>::SetSize2`, already matched from
//     collection.cpp. By hand it was NEAR — 112 against 112, the same
//     forty-eight instructions, every differing byte a register field: the
//     shipped build puts nItems in %edi and the zero in %ebx and ours did the
//     opposite. The template allocates them the shipped way. The fourteen
//     bytes this resize is longer than its four-byte siblings (0x1000bf8d,
//     0x1000c470) are `_DeleteAndDestroyArray`'s, not a difference in source:
//     an element with a destructor keeps BaseDelete out of line.
//
//   * `CWorldSub108::Clear` (0x1000a817, 50 bytes) is now IN THE CLASS. The
//     note that used to stand here said the definition was parked at the foot
//     of the file so that VC6 could not expand it into the two constructors.
//     That is wrong about the mechanism: a plain out-of-class definition is
//     never a /Ob1 candidate at ANY position, so the position bought nothing
//     and cost lithtech.exe both constructors, where the shipped /O2 build
//     expands the twelve stores in line (0x00425c82..0x00425cc4). /O1
//     declines the same body at fifty bytes, which is why server.dll carries
//     the standalone and a `call` to it either way.
// ---------------------------------------------------------------------- //


// 0x1000aa81 — CWorldSimple's eight-byte header off the stream, one field at
// a time.
//
// THIS ONE IS NOT AN /O2-SHAPE ROW, and it must not be marked `inline`.
// lithtech.exe has no standalone copy of it at all: scanning both images for
// the sequence that can only be this body (`lea 0x3(reg),reg; push $0x1`)
// finds exactly one occurrence in each, 0x1000aa81 in server.dll — the
// standalone we match — and 0x00426024 in lithtech.exe, which is INSIDE
// another function. That function is 0x00425df0, the simple world's loader:
// it opens with `call 0x00425d70` (CWorldSimple::Reset), ends with
// `call 0x00426090` (CWorldSimple::ComputeRadius), and has this whole body
// expanded at 0x00426000..0x0042603c along with a second five-read run of
// its own. It belongs to this translation unit and is not reconstructed.
//
// So the /O2 client's expansion has a caller we do not have, and `inline`
// here would buy nothing and cost something: nothing in this file calls
// wb_ReadSimpleHeader, so VC6 would stop emitting the COMDAT altogether and
// server.dll's 0x1000aa81 would go with it. The row closes when 0x00425df0
// is written, not before.
void wb_ReadSimpleHeader(ILTStream* pStream, CWorldSimpleHeader* pHeader)
{
	pStream->Read(&pHeader->m_n00, 1);
	pStream->Read(&pHeader->m_n01, 1);
	pStream->Read(&pHeader->m_n02, 1);
	pStream->Read(&pHeader->m_n03, 1);
	pStream->Read(&pHeader->m_n04, 4);
}


// ======================================================================= //
//  0x100083c9 — w_LoadWorldModel, 3,683 bytes.
//
//  The largest function in server.dll that had never been written. Placed
//  here rather than in commonmodel.cpp for the reason commonmodel.cpp:4056
//  itself gives: the class it fills is this file's CWorldBsp, and writing
//  it there would mean a second local copy of that layout.
//
//  ONE AMENDMENT WAS PROPOSED AND NOT APPLIED. The reconstruction wanted
//  worldbsp.h's `CWorldSurfaces* m_pSurfaces` at +0x7c retyped as a
//  `uint32 m_nSurfaces` count, on the grounds that this function writes it
//  as a count (0x10008887) and 0x10008de1 bound-checks against it, and that
//  nothing in the tree reads it as a pointer. THAT LAST PART IS WRONG:
//  CWorldBsp::MakeHPoly and CWorldBsp::FindPoly both dereference it, and
//  both MATCH today (0x1000ab53 and 0x1000ab62). So the field keeps its
//  declared type and this function reaches the count through a view.
//  Whether those two bodies are really CWorldSimple's, as the amendment
//  argued, is a separate question and does not have to be settled here.
// ======================================================================= //

// 0x100113b8 — commonmodel.cpp's inline three-float read, repeated here for
// the same reason that file repeats it: `inline`, so /Ob1 expands it, and an
// out-of-line declaration would give a call the shipped image does not make.
inline void w_ReadVector(class ILTStream* pStream, LTVector* pVec)
{
	pStream->Read(&pVec->x, 4);
	pStream->Read(&pVec->y, 4);
	pStream->Read(&pVec->z, 4);
}

// +0x7c read as the element count of the +0x78 run. See the note above for
// why this is a view and not a header change.
class CWorldBspSurfCountQT1
{
public:
	uint8  m_pad_00[0x7c];
	uint32 m_nSurfaces;                      // 0x7c
};


// 0x1000d2ef / 0x1000d374 — dalloc and its cleared form (dalloc.cpp).
void* dalloc(uint32 nSize);
void* dalloc_z(uint32 nSize);

// 0x1000d25d / 0x10028ace — the engine's operator new and operator delete.
// Declared here only so the two `new`s below reach the right allocator.

// 0x10014533 — servermath.cpp.  Which of the six axis bases this direction
// points most nearly along; the answer is 0..5 and it goes into the top
// three bits of the surface's flag word.  Takes its vector BY VALUE, which
// is why the shipped call site is three `movsl` into a 12-byte hole in the
// stack (0x10008b81..0x10008b8f).
int math_FindClosestAxisBasis(LTVector vDir);

// 0x10007cf7 — the loop that walks one polygon's vertices and builds each
// one's texture coordinates out of the surface's three vectors at
// +0x04/+0x10/+0x1c. RECONSTRUCTED, byte-identical, in commonmodel.cpp,
// which is where svnew.cpp's MOVE-ME note said it belonged and where the
// shipped layout puts it (0x10007cf7 sits between w_ReadWorldHeader and
// w_FindPortalInWorld, both of them in that file, and both byte-identical
// there -- which says this function's CALLER belongs there too).
void w_SetupPolyTexCoordsQT1(class CWorldPart* pPoly);

// The six helpers that live in the run immediately after this function.
// Every signature here is read off this function's own call sites: the
// argument count from the `add $N,%esp` that follows, and the types from
// what is pushed.  0x1000922c is defined just below (inline); 0x100092b6 and 0x100092e4
// are defined after w_LoadWorldModel, in shipped order; 0x10009361 is in
// commonmodel.cpp.
//
// 0x1000922c (36 bytes) — two 2-byte reads, at p and p+2.  One element of
//     the +0x54 collection, which is why that array's element is four bytes.
//     RECONSTRUCTED here and `inline`: the /O2 client has no standalone copy
//     (its loader reads the two words in line), while /O1 declines the
//     expansion and emits the body right after the loader, at 0x1000922c --
//     the same place 0x10009250 (CWorldItem18::Init) and 0x10009275 (the
//     CWorldSurface constructor), the loader's other two inline callees,
//     land.  Defined here because /Ob1 is forward-only.
class CWorldEdgeQT1
{
public:
	uint16 m_n00;                            // 0x00
	uint16 m_n02;                            // 0x02
};

inline void w_ReadEdgeQT1(class ILTStream* pStream, void* pEdge)
{
	pStream->Read(&((CWorldEdgeQT1*)pEdge)->m_n00, 2);
	pStream->Read(&((CWorldEdgeQT1*)pEdge)->m_n02, 2);
}
//
// 0x100092b6 (35 bytes) — index -> tree node, with the two sentinels.
//     `-1` answers g_pWorldTreeFrontTerm and `-2` g_pWorldTreeBackTerm (the
//     two globals worldbsp.h already declares); an index at or past the
//     count answers null; anything else is `pNodes + n*0x18`.  The count
//     compare is SIGNED (`jl` at 0x100092d4).
void* w_GetTreeNodeQT1(void* pNodes, uint32 nNodes, uint32 nIndex);
//
// 0x100092e4 (84 bytes) — after everything is read: set every surface's
//     +0x40 to 0xffff, then thread every polygon onto its surface's list
//     through +0x40, head-first, by INDEX.  Takes the world.
void w_LinkSurfacePoliesQT1(CWorldBsp* pWorld);
//
// 0x10009338 (41 bytes) — walk the request's class list (next at +0x0c,
//     name at +0x10) for a case-folded name match, through
//     dsi_StringsEqualUpperQ6L (0x1000e480, dsi.cpp:277).  Null-terminated
//     list, null answer on miss.
class WorldClassQT1* w_FindWorldClassQT1(void* pList, char* pName);
//
// 0x10009361 (143 bytes) — fill in every tree node's +0x17 projection code:
//     6 for a general plane, 0..5 for a plane whose normal is within
//     0.99999 of an axis (0x1004d348 = -0.99999f, 0x1004d34c = +0.99999f).
//     Only when the third argument is nonzero; otherwise every code is 6.
//     RECONSTRUCTED in commonmodel.cpp, byte-identical in both images
//     (server 0x10009361, lithtech 0x00423cc0); the signature below is
//     that definition's and the declaration is kept here because this
//     file is where the six were read off.
void w_SetNodeProjectionsQT1(void* pNodes, uint32 nNodes, int bCurrent);
//
// 0x10009275 (65 bytes) — CWorldSurface's constructor, reached only by the
//     `new CWorldSurface[n]` below.  0x10009250 is CWorldItem18::Init,
//     which worldbsp.h already carries.


// One entry of the world-object class list the request carries.  Only the
// three fields this function reaches are named; the list itself belongs to
// whoever fills pRequest->m_pUserData (names ours).
class WorldClassQT1
{
public:
	// Called through `call *(%eax)` at 0x10008cbf — a FUNCTION POINTER at
	// offset 0, not a vtable.  A virtual would be
	// `mov (%eax),%edx ; call *(%edx)`; this is one indirection.
	void* (*m_pMake)(struct WorldObjectInfoQT1* pInfo, int nArgs,
	                 char** pArgs);          // 0x00

	uint8         m_pad_04[0x0c - 0x04];
	WorldClassQT1* m_pNext;                  // 0x0c
	char          m_Name[1];                 // 0x10
};

// What the make-function is handed: the surface's three texture vectors by
// value, then the three objects that own the surface.  0x30 bytes at
// -0xc4(%ebp), assembled at 0x10008c66..0x10008ca2 (names ours).
struct WorldObjectInfoQT1
{
	LTVector          m_vO;                  // 0x00  <- pSurf->m_vO
	LTVector          m_vP;                  // 0x0c  <- pSurf->m_vP
	LTVector          m_vQ;                  // 0x18  <- pSurf->m_vQ
	CServerWorldFull* m_pServerWorld;        // 0x24
	CWorldBsp*        m_pBsp;                // 0x28
	class CWorldSurface* m_pSurface;         // 0x2c
};

// And what is kept of the object afterwards: a 0x14-byte record pushed on
// the head of the list at CServerWorldFull+0xf4 (commonmodel.cpp's
// m_n0f4).  Store order below is the shipped order, 0x10008cd2..0x10008cf3
// (names ours).
struct WorldObjectRecQT1
{
	CWorldBsp*           m_pBsp;             // 0x00
	class CWorldSurface* m_pSurface;         // 0x04
	void*                m_pObject;          // 0x08
	WorldClassQT1*       m_pClass;           // 0x0c
	WorldObjectRecQT1*   m_pNext;            // 0x10
};


// The 0x48-byte record at CWorldBsp+0x78 — worldbsp.h's CWorldTree, and
// ltobject_internal.h's WorldBspData and WorldSurface, which are the same
// object seen from the physics side.  Every offset below is either written
// by this loader, zeroed by the constructor at 0x10009275, or already in
// ltobject_internal.h.
//
// The constructor (0x10009275, and EXPANDED in the client at 0x00422da2 so
// both readings agree): build the member at +0x28 (0x100243f6), store the
// vtable 0x1004d344 at +0x00, call 0x10024409 on that member with `this`
// as its argument, then m_n30/m_n34/m_n38 = 0, m_n3c/m_n3e/m_n40 = 0,
// m_nLink42 = 0x7fff, m_n44 = 0.
// 0x28 -- commonmodel.h's Model::CModelSub14, the two-dword chain the bigger
// engine records carry: a head and an owner.  Its constructor (0x100243f6),
// destructor (0x10024404) and initialiser (0x10024409) are commonmodel.cpp's;
// the constructor below calls Init on it with its own `this` as the owner.
typedef Model::CModelSub14 CWorldSurfSub28QT1;

class CWorldSurface
{
public:
	CWorldSurface();                         // 0x10009275

	// +0x00 IS A VPTR: one virtual, and 0x1004d344 is its one-slot vtable
	// (0x1004d344 -> 0x1000af3b).  This used to be modelled as a function-
	// pointer field, because the 0x10009275 constructor stores it AFTER
	// constructing the member at +0x28 and the implicit destructor inside
	// ??_ECWorldSurface (0x1000796d) restores none.  VC6 compiles the
	// virtual spelling to exactly those bytes (constructor, ??_E and the
	// slot all match on both images), and the whole-DLL link needs the
	// vtable to be one: the old field stored a function symbol,
	// `w_SurfaceTestQT1`, that stood for a .rdata address.
	virtual void GetBasisVectors(LTVector* pR, LTVector* pU);  // 0x1000af3b

	LTVector  m_vO;                          // 0x04  texture origin
	LTVector  m_vP;                          // 0x10  texture U vector
	LTVector  m_vQ;                          // 0x1c  texture V vector
	// 0x28 -- worldbsp.h's CWorldTreeSub28, the member with the ctor
	// (0x100243f6) and the dtor (0x10024404).  Spelling it as a member
	// with a DESTRUCTOR rather than as padding is what puts the array
	// cookie on the `new[]` below: 0x10008767 asks the allocator for
	// `n*0x48 + 4`, 0x10008781 stores the count at [p] and 0x1000877e
	// hands back p+4.  A destructor-free element gets no cookie at all.
	CWorldSurfSub28QT1 m_Sub28;              // 0x28
	uint32    m_n30;                         // 0x30
	class LTPlane* m_pPlane;                 // 0x34  ltobject_internal.h
	// ltobject_internal.h's WorldBspData::m_nFlags.  TWO BIT-FIELDS: the
	// loader writes the top three (the axis code) with an OR-merge at
	// 0x10008b9c and the bottom twenty-nine with the XOR-merge at
	// 0x10008bb9 (`x ^= ((x ^ v) & 0x1fffffff)`), which is MSVC 6's
	// bit-field store.  WBSP_NOSTEP (0x400000) lives in the low field.
	uint32    m_nFlags : 29;                 // 0x38
	uint32    m_nAxis  : 3;
	uint16    m_nTextureFlags;               // 0x3c  ltobject_internal.h
	uint16    m_n3e;                         // 0x3e
	uint16    m_nFirstPoly;                  // 0x40  head of the per-surface
	                                         //       polygon chain, by index
	uint16    m_nLink42;                     // 0x42  constructor sets 0x7fff
	uint8     m_n44;                         // 0x44
	uint8     m_pad_45[0x48 - 0x45];
};

// 0x10009275 (65 bytes) — `inline` because the /O2 client expands it into
// the `new CWorldSurface[n]` loop (0x00422da2) and /O1 does not
// (0x100087a7 calls it); defined above w_LoadWorldModel because /Ob1 is
// forward-only.

inline CWorldSurface::CWorldSurface()
{
	m_Sub28.Init((uint32)this);
	m_n30 = 0;
	m_pPlane = LTNULL;
	m_nFlags = 0;
	m_nAxis = 0;
	m_nTextureFlags = 0;
	m_n3e = 0;
	m_nFirstPoly = 0;
	m_nLink42 = 0x7fff;
	m_n44 = 0;
}

// 0x1000af3b -- the one slot of 0x1004d344: the surface's two texture-plane
// basis vectors, off its plane's normal and the axis code (names ours).
void math_GetAxisBasisVectors(uint32 iBasis, LTVector* pDir,
                              LTVector* pOut1, LTVector* pOut2);  // 0x1001457f

void CWorldSurface::GetBasisVectors(LTVector* pR, LTVector* pU)
{
	math_GetAxisBasisVectors(m_nAxis, (LTVector*)m_pPlane, pR, pU);
}


// One entry of the 8-byte pool at CWorldBsp+0x80: a word, a byte count, and
// a pointer into the byte pool at +0x104 that the count's worth of bytes is
// read straight into (names ours).
class CWorldGroupEntryQT1
{
public:
	uint16 m_n00;                            // 0x00
	uint16 m_nBytes;                         // 0x02
	void*  m_pData;                          // 0x04  -> pWorld->m_p104
};

// One 0x30-byte record of the run at CWorldBsp+0x88.  It owns a run of the
// 8-byte entries above, a run of 4-byte references into +0x90, and a link
// the loader ties off and nothing here ever uses (names ours).
class CWorldGroupQT1
{
public:
	uint8   m_pad_00[0x10];
	CWorldGroupEntryQT1* m_pEntries;         // 0x10  -> pWorld->m_p80
	LTLink  m_Link;                          // 0x14  tied off, never linked
	class CWorldEdgeQT1* m_pRefs;            // 0x20  -> pWorld->m_p90
	uint32  m_nRefs;                         // 0x24
	uint32  m_n28;                           // 0x28
	uint8   m_pad_2c[0x2e - 0x2c];
	uint16  m_nEntries;                      // 0x2e
};
// Moved above CWorldPartFullQT1, which embeds it: a class used as an
// array member has to be complete at that point.
class CWorldVertQT1
{
public:
	LTVector* m_pPos;                        // 0x00  -> pWorld->m_pa8
	uint8     m_pad_04[0x14 - 0x04];
	uint8     m_b;                           // 0x14
	uint8     m_g;                           // 0x15
	uint8     m_r;                           // 0x16
	uint8     m_a;                           // 0x17
};


// worldbsp.h's CWorldPart, with the fields this loader fills named.  It is
// ltobject_internal.h's WorldPolyOwner; nothing below moves an offset.
// Every polygon is carved out of the one flat block at CWorldBsp+0xfc, so
// the records are variable length: 0x54 plus 0x18 per vertex, rounded up to
// a multiple of four.
class CWorldPartFullQT1
{
public:
	uint8     m_pad_00[0x0c];
	void*     m_pEdges;                      // 0x0c  -> m_Array54.m_pArray
	uint32    m_nEdges;                      // 0x10
	uint8     m_pad_14[0x16 - 0x14];
	uint16    m_nIndex;                      // 0x16  its own index
	uint8     m_pad_18[0x28 - 0x18];         // 0x18 m_vCentre, 0x24 m_fRadius
	CWorldSurface* m_pSurface;               // 0x28  WorldPolyOwner::m_pParent
	uint8     m_pad_2c[0x34 - 0x2c];
	LTVector  m_v34;                         // 0x34  read by w_ReadVector
	uint16    m_nNextOnSurface;              // 0x40  0x100092e4 threads this
	uint8     m_pad_42[0x48 - 0x42];
	uint8     m_n48;                         // 0x48
	uint8     m_n49;                         // 0x49
	uint8     m_pad_4a[0x4c - 0x4a];
	class CWorldVertQT1* m_pVerts2;          // 0x4c
	uint16    m_nVerts;                      // 0x50
	uint16    m_nVerts2;                     // 0x52
	class CWorldVertQT1 m_Verts[1];          // 0x54  as many as the two
	                                         //       counts add up to
};

// One vertex record, 0x18 bytes.  ltobject_internal.h's WorldPolyVertex
// gives the position pointer at 0x00; the loader adds the four colour bytes
// at the end.  They are read r, g, b and stored at 0x16, 0x15, 0x14 — a
// BGRA byte order on the wire's RGB — and the alpha is set to 0xff.
//
// SPELLING: `uint8 m_Color[4]` with the loop writing [2], [1], [0], [3] is
// the same three bytes and is equally plausible; four named bytes is
// written because the three reads use three different displacements off one
// cursor rather than an index.

// worldbsp.h's CWorldChunk, named.  0x24 bytes, one owned string.
class CWorldChunkFullQT1
{
public:
	char*     m_pName;                       // 0x00  dalloc'd, strcpy'd
	uint16    m_n04;                         // 0x04
	uint8     m_pad_06[0x08 - 0x06];
	uint16    m_nIndex;                      // 0x08  its own index
	uint8     m_pad_0a[0x0c - 0x0a];
	LTVector  m_v0c;                         // 0x0c
	LTVector  m_v18;                         // 0x18
};


// ======================================================================= //
//  0x100083c9 — w_LoadWorldModel.
//
//  SIGNATURE, off the bytes: cdecl, seven arguments (the caller cleans
//  0x1c at commonmodel.cpp's call sites), one `ret`, and the answer is an
//  internal code the caller hands straight back — 0 is success and every
//  failure is 2.  0x08 the request, 0x0c the server world, 0x10 a flag the
//  caller always passes zero, 0x14 where the geometry goes, 0x18 and 0x1c
//  the two tick accumulators, 0x20 "this is the live copy".
//
//  THE FRAME decomposes exactly, which is the second-strongest evidence in
//  this file after the three-header agreement above.  `mov $0x17d8,%eax ;
//  call 0x1003e740` (__chkstk) — 6,104 bytes:
//
//      -0x17d8 .. -0x02d0   0x1508  CommandParser        (servermgr.h:1574
//                                   gives 0x1508 exactly: 64 char* + an int
//                                   + a char* + a 0x1400 buffer.  The
//                                   parser's m_nArgs is read at -0x16d8 =
//                                   -0x17d8+0x100 and its m_pPos written at
//                                   -0x16d4 = -0x17d8+0x104.  Both offsets
//                                   land on the declared members.)
//      -0x02d0 .. -0x01d0    0x100  the property text  (ReadString 0xff)
//      -0x01d0 .. -0x0150     0x80  the class name     (ReadString 0x7f)
//      -0x0150 .. -0x00d0     0x80  the chunk name     (ReadString 0x7f)
//      -0x00d0 .. -0x00c4      0xc  a discarded LTVector
//      -0x00c4 .. -0x0094     0x30  WorldObjectInfoQT1
//      -0x0094 .. -0x008c            two discarded dwords
//      -0x008c .. -0x0084      0x8  CBaseCounter, the precalc timer
//      -0x0084 .. -0x007c      0x8  CBaseCounter, the load timer
//      -0x007c .. -0x0000            the scalars
//
//  Nothing is left over and nothing overlaps: 0x1508 + 0x100 + 0x80 + 0x80
//  + 0xc + 0x30 + 0x8 + 0x8 + 0x8 + 0x7c = 0x17d8.
//
//  THE ELEVEN PROGRESS CALLS.  `if (nFlags) pRequest->m_LoadHook(...)`
//  appears eleven times, at 0x10008583, 0x10008639, 0x10008893, 0x10008ab2,
//  0x10008aff, 0x10008d1f, 0x10008ecc, 0x10008f92, 0x100090a8, 0x100090d8
//  and 0x10009200, always between phases.  Written out eleven times below.
//  A MACRO is equally consistent with the bytes (a macro and eleven copies
//  of the statement emit the same instructions), and so is an `inline` free
//  function under /Ob1; nothing separates them, so the plain statement is
//  written.  The caller always passes nFlags == 0, so in the shipped server
//  none of the eleven ever fires.
// ======================================================================= //

int w_LoadWorldModel(WorldLoadRequest* pRequest, CServerWorldFull* pServerWorld,
                     uint32 nFlags, CWorldBsp** ppGeom, uint32* pnLoadTicks,
                     uint32* pnPrecalcTicks, int bCurrent)
{
	// Server-only: the /O2 client has neither counter, no StartCounter and
	// no tick accumulation anywhere in its twin (0x00422970..0x00423bf3).
#ifdef LT_SERVER_TIMERS
	CBaseCounter   counterLoad(0);           // -0x84
	CBaseCounter   counterPrecalc(0);        // -0x8c
#endif
	CommandParser  parser;                   // -0x17d8

	ILTStream*     pStream;                  // %ebx throughout
	CWorldBsp*     pWorld;                   // %esi, saved at -0x28

	uint32   nJunk;                          // -0x20, the eight header dwords
	uint32   nJunk2;                         // -0x90
	uint32   nJunk3;                         // -0x94
	uint8    nJunkR, nJunkG, nJunkB;         // -0x42, -0x41, -0x43
	LTVector vJunk;                          // -0xd0

	// The counts, all off the wire in this order.
	uint32   nNumPoints;                     // -0x30
	uint32   nNumPlanes;                     // -0x38
	uint32   nNumSurfaces;                   // -0x34
	uint32   nNumPolies;                     // -0x1c
	uint32   nNumGroups;                     // -0x2c
	uint32   nPoolBytes;                     // -0x58
	uint32   nNumEdges;                      // -0x70
	uint32   nStringBytes;                   // -0x64
	uint32   nNumSubWorlds;                  // -0x5c

	uint32   i, j;                           // -0x4, -0x8 / 0xc(%ebp)
	uint32   nOffset;                        // -0x8, regions D, E, G, H only
	// The edge cursor of region L (-0xc), whose dead first life is region
	// H's `+= nVerts0` (0x10008901), and region I's byte-pool cursor
	// (-0x40).  Shipped gives the two uses two slots; one variable is what
	// makes our frame come out at 0x17d8 (two give 0x17dc, because our
	// word and byte slots pack with more padding than shipped's do).
	uint32   nEdgeBase;
	uint32   nBase;                          // -0x18
	uint32   nSavePos;                       // -0x78
	// Six separate dwords and six separate words, one per read site: the
	// shipped frame has a distinct slot for each (0x17d8 needs all twelve).
	uint32   nRefs;                          // -0x3c
	uint32   nNode;                          // -0x54
	uint32   nPlane;                         // -0x68
	uint32   nFlagWord;                      // -0x6c
	uint32   nSurface;                       // -0x74
	uint32   nPoly;                          // -0x7c
	uint16   wGroup;                         // -0x14
	uint16   wVert;                          // -0x46
	uint16   w49;                            // -0x4a
	uint16   w48;                            // -0x4c
	uint16   wNode;                          // -0x4e
	uint16   wEdges;                         // -0x5e
	uint8    nVerts0, nVerts1;               // -0x11, -0x12
	uint8    bHasProps;                      // -0x21

	CWorldSurface*      pSurf;
	CWorldPartFullQT1*  pPoly;
	CWorldGroupQT1*     pGroup;
	CWorldGroupEntryQT1* pEntry;
	CWorldChunkFullQT1* pChunk;
	WorldPolyRef*       pNode;
	WorldClassQT1*      pClass;
	WorldObjectInfoQT1  info;                // -0xc4
	WorldObjectRecQT1*  pRec;
	void*               pObject;
	char     szClass[0x80];                  // -0x1d0
	char     szProps[0x100];                 // -0x2d0
	char     szChunk[0x80];                  // -0x150

	// --- A. 0x100083c9..0x1000840e -------------------------------------
	// Both counters are built with mode 0 before anything else happens
	// (0x100083db, 0x100083e7: `push %esi` with %esi already xored, then
	// `call 0x10005f38`).  The zero that clears parser.m_pPos and *ppGeom
	// is the same register.
	parser.m_pPos = LTNULL;                  // 0x100083f6, -0x16d4

	pStream = pRequest->m_pStream;           // 0x100083fc, request+0x00

	*ppGeom = LTNULL;                        // 0x10008401

#ifdef LT_SERVER_TIMERS
	cnt_StartCounter(&counterLoad);          // 0x1000840a
#endif

	// --- B. 0x1000840f..0x10008428 -------------------------------------
	// `new CWorldBsp` in VC6's null-arm shape: operator new, then the
	// constructor only if it answered non-null, with %esi carrying the
	// result and already holding zero for the null arm.
	pWorld = new CWorldBsp;                  // 0x150 / 0x1000d25d / 0x1000abae

	// --- C. 0x10008429..0x10008582 -- the fixed header -----------------
	// Eight four-byte reads that all go into ONE throwaway, then the world's
	// own fields.  Written out rather than looped because /O1 does not
	// unroll: a loop would still be a loop in the bytes.  The size 4 is
	// hoisted into %edi (`push $0x4 ; pop %edi`, 0x1000842b) and pushed from
	// there for the rest of the function.  The /O2 client does the same
	// eight reads into one slot in the same order (0x004229c0..0x00422a32).
	pStream->Read(&nJunk, 4);                // 0x10008438
	pStream->Read(&nJunk, 4);
	pStream->Read(&nJunk, 4);
	pStream->Read(&nJunk, 4);
	pStream->Read(&nJunk, 4);
	pStream->Read(&nJunk, 4);
	pStream->Read(&nJunk, 4);
	pStream->Read(&nJunk, 4);                // 0x1000848c

	pStream->Read(&pWorld->m_nUnknown48, 4);         // 0x10008498
	pStream->Read(&pWorld->m_ndc, 4);                // 0x100084a7

	// 0x40, not 0x41: the name array is 0x41 bytes (worldbsp.h) and the
	// reader asks for 0x40, leaving the last byte to the constructor.
	pStream->ReadString(pWorld->m_Name, 0x40);       // 0x100084b4, slot 0xc

	pStream->Read(&nNumPoints, 4);           // 0x100084c0
	pStream->Read(&nNumPlanes, 4);           // 0x100084cc
	pStream->Read(&nNumSurfaces, 4);         // 0x100084d8
	pStream->Read(&pWorld->m_nChunks, 4);    // 0x100084e7
	pStream->Read(&nNumPolies, 4);           // 0x100084f3
	pStream->Read(&nNumGroups, 4);           // 0x100084ff
	pStream->Read(&nJunk2, 4);               // 0x1000850e  read and dropped
	pStream->Read(&nPoolBytes, 4);           // 0x1000851a
	pStream->Read(&pWorld->m_n84, 4);        // 0x10008529
	pStream->Read(&pWorld->m_n74, 4);        // 0x10008535
	pStream->Read(&pWorld->m_n94, 4);        // 0x10008544
	pStream->Read(&nNumEdges, 4);            // 0x10008550

	// Three twelve-byte reads, NOT w_ReadVector: `push $0xc` against one
	// address, where w_ReadVector is three four-byte reads and a call.
	pStream->Read(&pWorld->m_vBoxMin, 0xc);  // 0x10008560
	pStream->Read(&pWorld->m_vBoxMax, 0xc);  // 0x10008570
	pStream->Read(&pWorld->m_ve8, 0xc);      // 0x10008580

	if (nFlags)                              // 0x10008583
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- D. 0x10008593..0x10008638 -- the string table -----------------
	// One flat char pool and an array of pointers into it.  The inner loop
	// is a do-while rotated by VC6 (the first read is peeled to 0x10008603
	// and the back edge at 0x10008629 jumps to the increment): the
	// terminator test is `pPool[nOffset - 1] != 0` AFTER the increment,
	// which is what the `cmpb $0x0,-0x1(%eax)` at 0x10008619 is.
	pStream->Read(&nStringBytes, 4);         // 0x1000859c
	pStream->Read(&pWorld->m_nc0, 4);        // 0x100085ab

	pWorld->m_pb8 = dalloc_z(nStringBytes);                  // 0x100085b1
	pWorld->m_pbc = dalloc_z(pWorld->m_nc0 * 4);             // 0x100085c6

	nOffset = 0;                             // -0x8
	for (i = 0; i < pWorld->m_nc0; i++)      // -0x4, 0x100085d3 / 0x10008631
	{
		// 0x100085e3 and 0x100085fa load nOffset and ADD the member
		// (`mov -0x8(%ebp),reg ; add 0xb8(%esi),reg`); we load the member
		// and add nOffset.  Spelling does not reach it: `nOffset +
		// (char*)m_pb8`, `(char*)(nOffset + (uint32)m_pb8)`,
		// `&((char*)m_pb8)[nOffset]` and a `char*` m_pb8 all compile
		// to identical bytes.  The SAME order question covers every
		// `base + offset` in this function (0x100088ce, 0x10008a5f,
		// 0x10008ad0, 0x10008b20, 0x10008b7e, 0x10008eea, 0x10008fbe,
		// 0x10009053, 0x10009143): shipped is offset-first at all of
		// them, ours base-first at all of them.  What moves it is which
		// SYMBOL the world is reached through, not the expression: this
		// loop in an out-of-line helper taking the world as a PARAMETER
		// emits shipped's order; the same helper expanded inline, or a
		// local copy (`pNew = new ...; pWorld = pNew;`), flips a
		// different, unstable subset of the nine.  No spelling tried
		// flips all of them.  Unresolved; the function's link-order home
		// is commonmodel.cpp (0x100083c9 sits between two of that file's
		// byte-identical functions), and a TU-wide effect like this one
		// is the first thing to re-measure there.
		// It costs the peeled Read as well: with both reads computing
		// their address the same way VC6 folds the back edge into the
		// single call site at 0x22e, where the shipped loop keeps TWO
		// (0x10008603 into %ecx and 0x1000861f reusing the test's %eax).
		((char**)pWorld->m_pbc)[i] = (char*)pWorld->m_pb8 + nOffset;

		do
		{
			pStream->Read((char*)pWorld->m_pb8 + nOffset, 1);
			nOffset++;
		}
		while (((char*)pWorld->m_pb8)[nOffset - 1] != 0);
	}

	if (nFlags)                              // 0x10008639
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- E. 0x10008649..0x100086de -- size the polygon block -----------
	// A measuring pass: remember where the polygons start, read only the
	// two vertex counts of each and add up what the records will need, then
	// seek back.  Each record is 0x54 plus 0x18 per vertex, rounded up to a
	// multiple of four.  The rounding is written as a remainder test and
	// not as `(n + 3) & ~3` — the shipped bytes are `and $0x3` then a
	// subtract from the 4 already living in %edi (0x10008693..0x1000869c).
	nOffset = 0;                             // -0x8

	pStream->GetPos(&nSavePos);              // 0x10008655, slot 0x18

	for (i = 0; i < nNumPolies; i++)         // 0x1000865c / 0x100086a5
	{
		pStream->Read(&nVerts0, 1);          // 0x1000866c, -0x11
		pStream->Read(&nVerts1, 1);          // 0x10008679, -0x12

		nOffset += 0x54 + (nVerts1 + nVerts0) * 0x18;

		if (nOffset & 3)
			nOffset += 4 - (nOffset & 3);
	}

	pStream->SeekTo(nSavePos);               // 0x100086b1, slot 0x14

	pWorld->m_pfc  = dalloc_z(nOffset);      // 0x100086b7
	pWorld->m_n100 = nOffset;                // 0x100086c5

	pWorld->m_pa8 = dalloc_z(nNumPoints * sizeof(LTVector));  // 0x100086d5

	// --- F. 0x100086e0..0x100087ae -- the two `new []` runs ------------
	// Both are `new T[n]` and neither is a hand-rolled fill.  The first has
	// NO array cookie (n*0x18 asked for, no count stored, plain
	// operator delete in FreeTree) because CWorldItem18 has no destructor;
	// the second asks for n*0x48 + 4, stores the count at [p] and hands
	// back p+4, because CWorldSurface's member at +0x28 has one — which is
	// also why worldbsp.cpp can spell its release `delete [] m_pTree`.
	// The /O2 client EXPANDS both constructors (0x00422d05 is
	// CWorldItem18::Init's body in the clear, 0x00422da2 is
	// CWorldSurface's), which is what rules out a loop written by hand.
	//
	// NOTE the element type of the first: 0x18 bytes, constructor
	// 0x10009250 = CWorldItem18::Init(0).  worldbsp.h already carries that
	// class; ltobject_internal.h carries the same 0x18 bytes as
	// WorldPolyRef.  They are one class and this is the world tree.
	// The element is worldbsp.h's CWorldItem18, NOT ltobject_internal.h's
	// layout-only WorldPolyRef: 0x100086f2..0x10008723 is the vector-new
	// CONSTRUCTOR loop -- `mov -0x10(%ebp),%ecx ; push $0x0 ;
	// call 0x10009250 ; addl $0x18,-0x10(%ebp)` -- and a POD element
	// produces no loop at all.  0x10009250 is CWorldItem18::Init and the
	// in-class `CWorldItem18(uint8 = 0)` around it expands, which is why
	// the loop calls Init directly with the defaulted 0 pushed.
	pWorld->m_n70 = (uint32)new CWorldItem18[pWorld->m_n74];   // 0x10008725

	pWorld->m_ppParts = (CWorldPart**)dalloc_z(nNumPolies * 4);        // 0x10008734
	pWorld->m_p88     = dalloc_z(nNumGroups * 0x30);                   // 0x10008749
	pWorld->m_p68     = dalloc_z(nNumPlanes * 0x10);                   // 0x1000875b

	pWorld->m_pTree = (CWorldTree*)new CWorldSurface[nNumSurfaces];    // 0x100087ac

	pWorld->m_p104    = dalloc_z(nPoolBytes);                          // 0x100087b7
	pWorld->m_p90     = dalloc_z(pWorld->m_n94 * 4);                   // 0x100087cc
	pWorld->m_p80     = dalloc_z(pWorld->m_n84 * 8);                   // 0x100087e1
	pWorld->m_pChunks = (CWorldChunk*)dalloc_z(pWorld->m_nChunks * 0x24); // 0x100087fc

	if (!pWorld->m_Array54.SetSize2(nNumEdges, &g_DefAlloc))   // 0x1000880d
		goto fail;

	// --- G. 0x1000881a..0x10008892 -- the live-geometry byte count -----
	// One expression: the object itself plus every allocation above.  VC6
	// re-associates it into a nest of `lea`s, and the constant that falls
	// out of the nest is 4*3*0xe = 336 = 0x150 = sizeof(CWorldBsp) — which
	// is the same 0x150 the `new` at 0x10008416 asked for, and is what says
	// this really is a sum of sizes and not an arbitrary formula.  The
	// twelve terms below reproduce the nest exactly; their ORDER in the
	// source is not recoverable, because the re-association destroys it.
	// MEASURED: permuting the four `* 4` terms does not move the emitted
	// `add 0x94 / add 0x5c / add 0xc0 / add -0x2c` order by one byte, so
	// the shipped order (0xc0, 0x5c, nNumPolies, 0x94 at 0x10008845) is
	// not reachable from here.
	pWorld->m_nBytes =
	      sizeof(CWorldBsp)                                       // 0x150
	    + nStringBytes                                            // m_pb8
	    + pWorld->m_nc0 * 4                                       // m_pbc
	    + pWorld->m_Array54.m_nElements * 4                       // +0x54
	    + nNumPolies * 4                                          // m_ppParts
	    + pWorld->m_n94 * 4                                       // m_p90
	    + pWorld->m_n84 * 8                                       // m_p80
	    + nNumPlanes * 0x10                                       // m_p68
	    + nNumPoints * 0xc                                        // m_pa8
	    + nNumSurfaces * 0x48                                     // m_pTree
	    + nNumGroups * 0x30                                       // m_p88
	    + pWorld->m_n74 * 0x18                                    // m_n70
	    + nOffset                                                 // m_pfc
	                                                              // 0x10008842
	                                                              // reads the
	                                                              // LOCAL, not
	                                                              // 0x100(%esi)
	    + nPoolBytes;                                             // m_p104

	pWorld->m_nac     = nNumPoints;          // 0x10008866
	pWorld->m_nParts  = nNumPolies;          // 0x1000886f
	pWorld->m_n8c     = nNumGroups;          // 0x10008878
	pWorld->m_n6c     = nNumPlanes;          // 0x10008881
	((CWorldBspSurfCountQT1*)pWorld)->m_nSurfaces = nNumSurfaces;
	                                         // 0x10008887 -- +0x7c is
	                                         // written as a COUNT here,
	                                         // through a view; see the
	                                         // note at the top of this
	                                         // function.
	                                         //   this field m_pSurfaces; see
	                                         //   the amendment note above)
	pWorld->m_n98     = nPoolBytes;          // 0x1000888d

	if (nFlags)                              // 0x10008895
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- H. 0x100088a6..0x1000892b -- carve the polygon records --------
	// The same walk as E, this time keeping what it reads: each polygon
	// gets its slot in the flat block, its own index, the two vertex
	// counts, and a slot in m_ppParts.  The two counts go into uint16
	// fields out of uint8 locals, which is the `movzbw` pair at 0x100088e7
	// and 0x100088f0.
	//
	// nEdgeBase below is a genuine DEAD STORE in the shipped code: summed
	// here (0x10008901) and re-zeroed by region L (0x10008d31) before
	// anything reads it.  The statement is written because /O1 kept
	// the add; if the original had no such statement the add would not be
	// there.  It is either a vestigial total or a bug.  Flagged below.
	nOffset = 0;                             // -0x8
	nEdgeBase = 0;                             // -0xc

	for (i = 0; i < nNumPolies; i++)         // 0x100088a6 / 0x10008925
	{
		pStream->Read(&nVerts0, 1);          // 0x100088be
		pStream->Read(&nVerts1, 1);          // 0x100088cb

		pPoly = (CWorldPartFullQT1*)((char*)pWorld->m_pfc + nOffset);

		pPoly->m_nIndex = (uint16)i;                     // 0x100088da, +0x16
		pWorld->m_ppParts[i] = (CWorldPart*)pPoly;       // 0x100088e4

		pPoly->m_nVerts  = nVerts0;          // 0x100088ec, +0x50
		pPoly->m_nVerts2 = nVerts1;          // 0x100088f5, +0x52

		nEdgeBase += nVerts0;                     // 0x10008901 -- dead, see above

		nOffset += 0x54 + (nVerts1 + nVerts0) * 0x18;

		if (nOffset & 3)
			nOffset += 4 - (nOffset & 3);
	}

	// --- I. 0x1000892c..0x10008ab1 -- the 0x30-byte group records ------
	// Each group owns a run of 8-byte entries carved out of m_p80, whose
	// payloads come out of the byte pool at m_p104, plus a run of 4-byte
	// references carved out of m_p90.  A group whose first word is 0xffff
	// is a DUPLICATE: it takes its entry count and entry pointer from an
	// earlier group by index and reads nothing more (0x10008977).
	pEntry = (CWorldGroupEntryQT1*)pWorld->m_p80;   // -0x10, 0x1000893b
	nEdgeBase = 0;                                     // -0x40, the m_p104 cursor
	nBase  = 0;                                     // -0x18, the m_p90 cursor

	for (i = 0; i < nNumGroups; i++)         // 0x10008932 / 0x10008aa9
	{
		pGroup = &((CWorldGroupQT1*)pWorld->m_p88)[i];

		pGroup->m_Link.TieOff();             // 0x1000895f, two stores
		pGroup->m_pEntries = pEntry;         // 0x10008967

		pStream->Read(&wGroup, 2);                // 0x1000896c, -0x14

		if (wGroup == 0xffff)                     // 0x10008973
		{
			pStream->Read(&wGroup, 2);            // 0x10008983

			pGroup->m_nEntries =
			    ((CWorldGroupQT1*)pWorld->m_p88)[wGroup].m_nEntries;   // 0x1000899b
			pGroup->m_pEntries =
			    ((CWorldGroupQT1*)pWorld->m_p88)[wGroup].m_pEntries;   // 0x100089b3
		}
		else
		{
			pGroup->m_nEntries = wGroup;          // 0x100089bc

			for (j = 0; j < pGroup->m_nEntries; j++)   // 0x100089c0 / 0x10008a22
			{
				pStream->Read(&wGroup, 2);        // 0x100089d1
				pEntry->m_n00 = wGroup;           // 0x100089dd

				pStream->Read(&wGroup, 2);        // 0x100089e8
				pEntry->m_nBytes = wGroup;        // 0x100089f5

				pEntry->m_pData = (char*)pWorld->m_p104 + nEdgeBase;   // 0x10008a01

				pStream->Read(pEntry->m_pData, pEntry->m_nBytes); // 0x10008a0e

				nEdgeBase += pEntry->m_nBytes;    // 0x10008a1c
				pEntry++;                    // 0x10008a14, +8
			}
		}

		pStream->Read(&nRefs, 4);                // 0x10008a35, -0x3c
		pGroup->m_nRefs = nRefs;                 // 0x10008a42
		pGroup->m_pRefs = (CWorldEdgeQT1*)pWorld->m_p90 + nBase;   // 0x10008a4e

		// Two 2-byte reads per reference — the same shape 0x1000922c has,
		// but written out here rather than called, which is what says the
		// element type is not the same one.
		for (j = 0; j < nRefs; j++)              // 0x10008a51 / 0x10008a87
		{
			pStream->Read(&pGroup->m_pRefs[j].m_n00, 2);     // 0x10008a67
			pStream->Read(&pGroup->m_pRefs[j].m_n02, 2);   // 0x10008a7e
		}

		pStream->Read(&pGroup->m_n28, 4);    // 0x10008a96

		nBase   += nRefs;                        // 0x10008aa0
	}

	if (nFlags)                              // 0x10008ab2
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- J. 0x10008ac2..0x10008afe -- the planes -----------------------
	// A twelve-byte read and a four-byte read per record, not one sixteen:
	// LTPlane's normal and distance are two members and the source reads
	// them one at a time.
	for (i = 0; i < nNumPlanes; i++)         // 0x10008ac6 / 0x10008af7
	{
		pStream->Read(&((LTPlane*)pWorld->m_p68)[i].m_Normal, 0xc); // 0x10008ada
		pStream->Read(&((LTPlane*)pWorld->m_p68)[i].m_Dist, 4);     // 0x10008aeb
	}

	if (nFlags)                              // 0x10008aff
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- K. 0x10008b0c..0x10008d1e -- the surfaces ---------------------
	// The longest region, and the one the object graph turns on.  Each
	// surface gets its three texture vectors, its plane, the axis its plane
	// most nearly points along, its flag bits, and — for a surface whose
	// property byte is 1 — a world object built from its property text by a
	// class the request's list names.
	//
	// %esi is re-used as the surface cursor through this whole region, so
	// the world pointer lives in %edi off the saved copy at -0x28
	// (0x10008b1d) and the failure exit at 0x10008e97 is a SECOND copy of
	// "delete the world and return 2" written against %edi.  That is the
	// one place /O1's tail merging did not reach, and it is the reason the
	// function has two delete sites for one source statement.

	for (i = 0; i < nNumSurfaces; i++)       // 0x10008b0e / 0x10008d10
	{
		pSurf = &((CWorldSurface*)pWorld->m_pTree)[i];  // -0x10

		pStream->Read(&pSurf->m_vO, 0xc);    // 0x10008b33
		pStream->Read(&pSurf->m_vP, 0xc);    // 0x10008b40
		pStream->Read(&pSurf->m_vQ, 0xc);    // 0x10008b4d
		pStream->Read(&pSurf->m_n3e, 2);     // 0x10008b5a
		pStream->Read(&nPlane, 4);                // 0x10008b67, -0x68

		if (nPlane >= pWorld->m_n6c)              // 0x10008b6d, unsigned
			goto fail;

		// A local for the plane: %esi is loaded once (0x10008b83) and used
		// both for the store and as the source of the three `movsl` that
		// pass the normal by value.  `pSurf->m_pPlane->m_Normal` as the
		// argument would re-load the member (VC6 does not CSE a member
		// read across a call, but there is no call between these two, so
		// the two spellings are NOT separated by the bytes).  The local is
		// written; the member re-read is the alternative.
		{
			LTPlane* pPlane = &((LTPlane*)pWorld->m_p68)[nPlane];

			pSurf->m_pPlane = pPlane;        // 0x10008b89, +0x34
			pSurf->m_nAxis  = math_FindClosestAxisBasis(pPlane->m_Normal);
		}                                    // 0x10008b8f / 0x10008bac

		pStream->Read(&nFlagWord, 4);                // 0x10008bb6, -0x6c
		pSurf->m_nFlags = nFlagWord;                 // 0x10008bb9..0x10008bc9, the
		                                     //   `x ^= ((x ^ v) & 0x1fffffff)`
		                                     //   bit-field store

		pStream->Read(&pSurf->m_n44, 1);     // 0x10008bd0, +0x44

		// Three bytes read and never used.  Three DISTINCT frame slots, so
		// they are three named variables and not one reused temporary —
		// VC6 shares a slot between two locals only when their lifetimes
		// do not overlap, and these three are all live across each other's
		// reads.  Almost certainly a discarded colour.
		pStream->Read(&nJunkR, 1);           // 0x10008bdc, -0x42
		pStream->Read(&nJunkG, 1);           // 0x10008be8, -0x41
		pStream->Read(&nJunkB, 1);           // 0x10008bf4, -0x43

		pStream->Read(&bHasProps, 1);        // 0x10008c00, -0x21

		if (bHasProps == 1)                  // 0x10008c03, == 1, not != 0
		{
			pStream->ReadString(szClass, 0x7f);   // 0x10008c1a
			pStream->ReadString(szProps, 0xff);   // 0x10008c2d

			pClass = w_FindWorldClassQT1(pRequest->m_pUserData, szClass);

			if (pClass)                      // 0x10008c46
			{
				parser.m_pPos = szProps;     // 0x10008c5b, -0x16d4
				parser.Next();               // 0x10008c61

				info.m_vO = pSurf->m_vO;     // 0x10008c72, three `movsl`
				info.m_vP = pSurf->m_vP;     // 0x10008c7e
				info.m_vQ = pSurf->m_vQ;     // 0x10008c8a

				info.m_pServerWorld = pServerWorld;   // 0x10008c90
				info.m_pBsp         = pWorld;         // 0x10008c99
				info.m_pSurface     = pSurf;          // 0x10008ca2

				pObject = pClass->m_pMake(&info, parser.m_nArgs,
				                          parser.m_ArgPointers);
				                             // 0x10008cbf, cdecl, add $0xc

				if (pObject)                 // 0x10008cc6
				{
					// No null check on the allocation — see the bug list.
					pRec = (WorldObjectRecQT1*)dalloc_z(0x14);  // 0x10008ccc

					// Store order below is the shipped order exactly:
					// +0x0c, +0x04, +0x00, +0x10, the head, then +0x08.
					pRec->m_pClass   = pClass;        // 0x10008cd5
					pRec->m_pSurface = pSurf;         // 0x10008cdb
					pRec->m_pBsp     = pWorld;        // 0x10008ce1

					pRec->m_pNext =
					    (WorldObjectRecQT1*)((CServerWorldListQT1*)pServerWorld)->m_n0f4;  // 0x10008cee
					((CServerWorldListQT1*)pServerWorld)->m_n0f4 = (uint32)pRec;           // 0x10008cf1

					pRec->m_pObject = pObject;        // 0x10008cf3
				}
			}
		}

		pStream->Read(&pSurf->m_nTextureFlags, 2);   // 0x10008d03, +0x3c

	}

	if (nFlags)                              // 0x10008d1f
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- L. 0x10008d2c..0x10008ecb -- the polygons ---------------------
	// Fill in the records region H carved: the plane-space vector, the two
	// bytes, the edge run, the surface, and every vertex's shared position
	// and colour.
	nEdgeBase = 0;                                // -0xc, the m_Array54 cursor

	for (i = 0; i < nNumPolies; i++)         // 0x10008d2e / 0x10008ec0
	{
		pPoly = (CWorldPartFullQT1*)pWorld->m_ppParts[i];   // %edi

		w_ReadVector(pStream, &pPoly->m_v34);     // 0x10008d4e, +0x34

		// TWO distinct slots (-0x4c and -0x4a), both read as words and both
		// stored as BYTES: `mov -0x4c(%ebp),%al ; mov %al,0x48(%edi)`.  A
		// uint16 local narrowed at the store, so the wire field is two
		// bytes wide and the record's field one.
		pStream->Read(&w48, 2);               // 0x10008d5f, -0x4c
		pStream->Read(&w49, 2);               // 0x10008d6c, -0x4a

		pPoly->m_n48 = (uint8)w48;             // 0x10008d75
		pPoly->m_n49 = (uint8)w49;            // 0x10008d7b

		pStream->Read(&wEdges, 2);           // 0x10008d85, -0x5e
		pPoly->m_nEdges = wEdges;                 // 0x10008d8c, a uint16 widened
		                                     //   into a uint32 field

		pPoly->m_pEdges =
		    (char*)pWorld->m_Array54.m_pArray + nEdgeBase * 4;   // 0x10008d98

		nEdgeBase += pPoly->m_nEdges;             // 0x10008d9e

		if (nEdgeBase > pWorld->m_Array54.m_nElements)           // 0x10008da0, JA
			goto fail;

		for (j = 0; j < pPoly->m_nEdges; j++)               // 0x10008db0
			w_ReadEdgeQT1(pStream, (char*)pPoly->m_pEdges + j * 4);

		pStream->Read(&nSurface, 4);                // 0x10008ddb, -0x74

		if (nSurface >= ((CWorldBspSurfCountQT1*)pWorld)->m_nSurfaces)
		                                     // 0x10008de1
			goto fail;

		pPoly->m_pSurface = &((CWorldSurface*)pWorld->m_pTree)[nSurface];  // +0x28

		// The cursor runs at &m_Verts[k].m_r (record + 0x16), which is
		// why the three colour reads are at 0, -1 and -2 off it and
		// the position store at -0x16.
		for (j = 0; j < (uint32)(pPoly->m_nVerts + pPoly->m_nVerts2); j++)
		{                                // 0x10008e0c / 0x10008e7b
			pStream->Read(&wVert, 2);        // 0x10008e16, -0x46

			if (wVert >= pWorld->m_nac)      // 0x10008e1d
				goto fail;

			pPoly->m_Verts[j].m_pPos = &((LTVector*)pWorld->m_pa8)[wVert];  // 0x10008e3b

			pStream->Read(&pPoly->m_Verts[j].m_r, 1);   // 0x10008e42, +0x16
			pStream->Read(&pPoly->m_Verts[j].m_g, 1);   // 0x10008e50, +0x15
			pStream->Read(&pPoly->m_Verts[j].m_b, 1);   // 0x10008e60, +0x14

			// `orb $0xff` in /O1; the /O2 client emits `movb $0xff`
			// at 0x004236a4 for the same source, so this is a plain
			// assignment.  (worldbsp.h's CWorldItem18::Init already
			// records the same /O1 `or` for `= 0xffff`.)
			pPoly->m_Verts[j].m_a = 0xff;           // 0x10008e6a, +0x17
		}

		if (pPoly->m_nVerts2 == 0)           // 0x10008e80
		{
			pPoly->m_pVerts2 = pPoly->m_Verts;           // 0x10008e8a, +0x4c
			pPoly->m_nVerts2 = pPoly->m_nVerts;          // 0x10008e91
		}
		else
		{
			pPoly->m_pVerts2 = &pPoly->m_Verts[pPoly->m_nVerts];
		}                                    // 0x10008eb7
	}

	if (nFlags)                              // 0x10008ecc
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- M. 0x10008ed9..0x10008f80 -- the tree nodes -------------------
	// One WorldPolyRef per node: the polygon it names, a word, and its two
	// children, which are resolved from indices through 0x100092b6 so that
	// -1 and -2 come back as the two terminators worldbsp.h declares.  The
	// projection byte is pre-set to 8 here and overwritten with 0..6 by
	// 0x10009361 two regions on.

	for (i = 0; i < pWorld->m_n74; i++)      // 0x10008edb / 0x10008f75
	{
		pNode = &((WorldPolyRef*)pWorld->m_n70)[i];

		// +0x16, NOT +0x17: 0x10008ef8 is `movb $0x8,0x16(%edi)`, and the
		// projection byte 0x10009361 fills is at +0x17.  A different field,
		// and the only thing that writes it.  Written before the read that
		// follows it, which is where the shipped store sits.
		*((uint8*)pNode + 0x16) = 8;         // 0x10008ef8

		pStream->Read(&nPoly, 4);                // 0x10008efe, -0x7c

		if (nPoly >= pWorld->m_nParts)           // 0x10008f04
			goto fail;

		pNode->m_pOwner =
		    (WorldPolyOwner*)pWorld->m_ppParts[nPoly];       // 0x10008f1e, +0x00

		pStream->Read(&wNode, 2);                // 0x10008f25, -0x4e
		*(uint16*)((char*)pNode + 0x14) = wNode; // 0x10008f30, +0x14

		// Two children, written as a two-trip loop in the source: the
		// shipped code steps a pointer by four and tests `< 2`
		// (0x10008f65), which a pair of straight-line statements would
		// not produce.
		for (int k = 0; k < 2; k++)            // 0x10008f2c / 0x10008f65
		{
			pStream->Read(&nNode, 4);            // 0x10008f41, -0x54

			*(void**)((char*)pNode + 4 + k * 4) =
			    w_GetTreeNodeQT1((void*)pWorld->m_n70, pWorld->m_n74, nNode);

			if (!*(void**)((char*)pNode + 4 + k * 4))    // 0x10008f57
				goto fail;
		}

	}

	// --- N. 0x10008f81..0x10008f91 -------------------------------------
	w_SetNodeProjectionsQT1((void*)pWorld->m_n70, pWorld->m_n74, bCurrent);

	if (nFlags)                              // 0x10008f92
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- O. 0x10008f9f..0x10009046 -- the chunks -----------------------
	// The one place the loader allocates a string of its own: strlen and
	// strcpy (0x1003e2e0 / 0x1003e580) over the plain dalloc, not the
	// cleared one.

	for (i = 0; i < pWorld->m_nChunks; i++)  // 0x10008fa3 / 0x1000903b
	{
		pChunk = &((CWorldChunkFullQT1*)pWorld->m_pChunks)[i];

		pChunk->m_nIndex = (uint16)i;        // 0x10008fcc, +0x08, BEFORE the
		                                     //   ReadString that follows

		pStream->ReadString(szChunk, 0x7f);  // 0x10008fd2

		pChunk->m_pName = (char*)dalloc(strlen(szChunk) + 1);   // 0x10008fe3
		strcpy(pChunk->m_pName, szChunk);                       // 0x10008ff2

		pStream->Read(&nJunk3, 4);           // 0x10009007, -0x94, dropped
		pStream->Read(&pChunk->m_n04, 2);    // 0x10009014, +0x04
		pStream->Read(&pChunk->m_v0c, 0xc);  // 0x10009021, +0x0c
		pStream->Read(&pChunk->m_v18, 0xc);  // 0x1000902e, +0x18

	}

	// --- P. 0x10009047..0x100090a7 -- the shared vertex positions ------
	// Three four-byte reads per point and then a whole discarded vector.
	// The three are written out longhand rather than through w_ReadVector:
	// each one re-loads m_pa8 (`add 0xa8(%esi),%ecx` three times), which is
	// VC6 not CSE-ing a member across a call, and w_ReadVector would take
	// the address once and make ONE call.  The fourth read IS w_ReadVector,
	// into a local nothing ever looks at — a per-point normal the server
	// does not keep.
	for (i = 0; i < nNumPoints; i++)         // 0x10009049 / 0x100090a2
	{
		pStream->Read(&((LTVector*)pWorld->m_pa8)[i].x, 4);   // 0x10009060
		pStream->Read(&((LTVector*)pWorld->m_pa8)[i].y, 4);   // 0x10009074
		pStream->Read(&((LTVector*)pWorld->m_pa8)[i].z, 4);   // 0x10009088

		w_ReadVector(pStream, &vJunk);       // 0x10009093, -0xd0
	}

	if (nFlags)                              // 0x100090a8
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- Q. 0x100090b8..0x100090d7 -- the cell grid --------------------
	// worldmodels.cpp's CSubGrid::Load, which worldbsp.h models at the same
	// offsets as CWorldSub108.  Its cell records are eight bytes apiece and
	// that is what the byte count picks up (+0x130 is m_Sub108 + 0x28, the
	// cell count).
	//
	// NOTE THE LEAK: this failure arm jumps STRAIGHT to 0x100091fb, the
	// bare `return 2`, and not to the delete at 0x100091e9.  Every other
	// failure in the function deletes the world first.  Flagged below.
	if (!pWorld->m_Sub108.Load(pStream))     // 0x100090bf
		return 2;

	pWorld->m_nBytes += pWorld->m_Sub108.m_nRecords * 8;      // 0x100090d5

	if (nFlags)                              // 0x100090d8
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	// --- R. 0x100090e5..0x10009110 -- the tree root --------------------
	pStream->Read(&nNode, 4);                    // 0x100090ef, -0x54

	pWorld->m_n9c = (uint32)w_GetTreeNodeQT1((void*)pWorld->m_n70,
	                                         pWorld->m_n74, nNode);  // 0x10009103

	if (!pWorld->m_n9c)                      // 0x10009109
		goto fail;

	// --- S. 0x10009111..0x10009189 -- the sub-worlds -------------------
	// An array OF CWorldSimple, resized through its own SetSize2
	// (0x1000c0e7), then one Load per element.  The Load is a THISCALL —
	// `this` is the element in %ecx and the three pushes are the parent,
	// the stream and the index, with no `add $0xc,%esp` after the call
	// (0x10009154) — so 0x1000a8bb is CWorldSimple::Load and belongs on
	// that class.
	pStream->Read(&nNumSubWorlds, 4);        // 0x1000911b, -0x5c

	if (!pWorld->m_Array138.SetSize2(nNumSubWorlds, &g_DefAlloc))
		goto fail;                           // 0x1000912c

	for (i = 0; i < nNumSubWorlds; i++)      // 0x1000913b / 0x10009185
	{
		if (!pWorld->m_Array138[i].Load(pWorld, pStream, i))  // 0x1000914f
			goto fail;

		// 4 per part pointer, 0x18 per CWorldItem18, 8 per cell record.
		// The element is indexed afresh after the call (0x1000915c), not
		// held across it in a local.
		pWorld->m_nBytes +=
		      pWorld->m_Array138[i].m_Array50.m_nElements * 4        // 0x58
		    + pWorld->m_Array138[i].m_Array68.m_nElements * 0x18     // 0x70
		    + pWorld->m_Array138[i].m_Sub80.m_nRecords * 8;          // 0xa8
	}

	// --- T. 0x1000918a..0x1000922b -- timing, precalculation, exit -----
#ifdef LT_SERVER_TIMERS
	*pnLoadTicks += cnt_EndCounter(&counterLoad);          // 0x10009191
#endif

#ifdef LT_SERVER_TIMERS
	cnt_StartCounter(&counterPrecalc);                     // 0x100091a2
#endif

	for (i = 0; i < nNumPolies; i++)         // 0x100091aa / 0x100091c0
		w_SetupPolyTexCoordsQT1(pWorld->m_ppParts[i]);     // 0x100091b9

	w_LinkSurfacePoliesQT1(pWorld);                        // 0x100091c6

#ifdef LT_SERVER_TIMERS
	*pnPrecalcTicks += cnt_EndCounter(&counterPrecalc);    // 0x100091d2
#endif

	if (pStream->ErrorStatus() != LT_OK)     // 0x100091e2, slot 0x10
		goto fail;

	if (nFlags)                              // 0x10009200
		pRequest->m_LoadHook(pRequest->m_Unknown0c);

	pWorld->CWorldBsp::Recalc();             // 0x10009212, a DIRECT call

	g_WorldBytes += pWorld->m_nBytes;        // 0x1000921a

	*ppGeom = pWorld;                        // 0x10009223

	return 0;                                // 0x10009225

	// Ten `goto fail`s reach this one block, which /O1 tail-merged into the
	// single site at 0x100091e9.  The null test around the delete is VC6's
	// own -- worldbsp.cpp records the same thing for `delete [] m_pTree` --
	// so the source is one unguarded statement.
	//
	// An ELEVENTH copy of the same two statements sits at 0x10008e97,
	// written against %edi instead of %esi, because inside region K the
	// world pointer is not in %esi and /O1 could not merge it.  One source
	// statement, two emitted sites.
fail:
	delete pWorld;                           // 0x100091e9..0x100091fa

	return 2;                                // 0x100091fb
}


// ======================================================================= //
//  The loader's two out-of-line helpers, in shipped order: 0x100092b6 and
//  0x100092e4 (lithtech 0x00423c00 and 0x00423c30).  The three inline ones
//  it also calls (0x1000922c, 0x10009250, 0x10009275) are emitted by /O1
//  between the loader and these; their bodies are above the loader.
// ======================================================================= //

// 0x100092b6 (46 bytes, the last 11 filed as a gap) — index -> tree node.
// `-1` and `-2` are the two terminators, anything at or past the count is
// null, and the count compare is SIGNED (`jl` at 0x100092d4).
void* w_GetTreeNodeQT1(void* pNodes, uint32 nNodes, uint32 nIndex)
{
	if (nIndex == (uint32)-1)
		return g_pWorldTreeFrontTerm;
	if (nIndex == (uint32)-2)
		return g_pWorldTreeBackTerm;
	if ((int)nIndex >= (int)nNodes)
		return LTNULL;
	return (char*)pNodes + nIndex * 0x18;
}

// 0x100092e4 (84 bytes) — every surface's polygon chain emptied (0xffff),
// then every polygon pushed on the head of its surface's chain, by index.
void w_LinkSurfacePoliesQT1(CWorldBsp* pWorld)
{
	uint32 i;
	CWorldPartFullQT1* pPoly;
	CWorldSurface* pSurf;

	for (i = 0; i < ((CWorldBspSurfCountQT1*)pWorld)->m_nSurfaces; i++)
		((CWorldSurface*)pWorld->m_pTree)[i].m_nFirstPoly = 0xffff;

	for (i = 0; i < pWorld->m_nParts; i++)
	{
		pPoly = (CWorldPartFullQT1*)pWorld->m_ppParts[i];
		pSurf = pPoly->m_pSurface;
		if (pSurf)
		{
			pPoly->m_nNextOnSurface = pSurf->m_nFirstPoly;
			pSurf->m_nFirstPoly = (uint16)i;
		}
	}
}


// ======================================================================= //
//  0x1000a8bb / 454 bytes — CWorldSimple::Load.
//
//  One sub-world of a terrain off the stream.  The caller is region S of
//  w_LoadWorldModel above (0x1000914f), which is where the __thiscall and
//  the three arguments come from: the element in %ecx and no `add $0xc,%esp`
//  after the call.
//
//  THE ROW WAS FILED AS A COMMONMODEL FUNCTION AND IS NOT ONE.  What made it
//  look like one is the string it prints, "** Terrain %s's name is too long,
//  can't hold TerrainSections" (0x100519ec, pushed at 0x1000a8ea); what it
//  actually is is settled from both ends — it opens with a call to
//  0x1000a882 (CWorldSimple::Reset), closes with 0x1000aacc
//  (CWorldSimple::ComputeRadius), reads its header through 0x1000aa81
//  (wb_ReadSimpleHeader, this file's), and every offset it touches is this
//  class's: +0x04 the base's name, +0x48 the header, +0x50 the parts array,
//  +0x64 the tree root, +0x68 the node array, +0x7c the parent and +0xb0 the
//  centre.
//
//  WHAT IT READS, in order: the section's own name (built from the parent's
//  and this sub-world's index, NOT off the stream); a node count and that
//  many 0x18-byte tree nodes, each of which is a part index and two child
//  indices; the root node's index; a part count and that many part indices;
//  and then the sub-object at +0x80, the eight-byte header, the centre.
//
//  THE TWO FAILURE REGIMES.  The name and the node array's sizing return
//  LTFALSE with NO cleanup (both jump to the bare `xor eax,eax` at
//  0x1000aa45); everything after them runs Reset first (0x1000aa3e).  The
//  print is a one-argument call to a format string that carries a `%s`,
//  which is a period bug and is written the way it shipped.
//
//  THE WALKING POINTER.  %edi crosses the node body as one register: it is
//  the node when +0x16 is stored, then the destination of three consecutive
//  dword stores, advanced by four after each (0x1000a96b, 0x1000a99b).  A
//  subscripted store would index off the node; this is one pointer, so the
//  source carries one.
//
//  THE NODE LOOP GOES THROUGH A LOCAL COPY OF `this`.  Every `this` use
//  inside the node loop -- the node address and the two w_GetTreeNode
//  arguments -- is spelled through `pWorld`, a plain `CWorldSimple*` copy
//  of `this`.  Without it the node address at 0x1000a933 comes out
//  offset-first (`mov 0xc(%ebp),%edi ; add 0x6c(%esi),%edi`); shipped is
//  base-first, `mov 0x6c(%esi),%edi ; add 0xc(%ebp),%edi`.  The alias only
//  at the node address, or only at the inner call, does nothing; it has to
//  be every use in the loop.  (Same mechanism as wm_ResetGeometry in
//  worldmodels.cpp, in the other direction: which pointer symbol the loop's
//  uses go through decides the operand order of the strength-reduced add.)
//  Probed without effect earlier: m_pArray[i], operator[], Get(i),
//  GetArray()+i, i+GetArray(), byte-offset i*0x18, uint32* i*6, an
//  explicit stepped `off` local, a CWorldItem18* node local, a
//  CWorldArray68* local (+2 bytes), declaration order.
// ======================================================================= //

// 0x10007e44 — smgr.cpp already reaches this one as fs_GetTableName: the
// i'th name out of a table, copied into the caller's buffer under a cap,
// nonzero if it fitted.  Here the table is the parent world's own name and
// the index is this sub-world's, which is what makes the answer a terrain
// section's name (declaration only; name follows smgr.cpp's).
//
// RECONSTRUCTED here (lithtech twin 0x004221f0).  Its shipped home is the
// commonmodel.cpp run -- 0x10007e44 sits right after w_FindPortalInWorld's
// all-worlds form (0x10007dfa) -- so the definition belongs there when
// that file is next open; nothing about its bytes depends on the TU.
// "#$#" + index + " " + parent name, refused when the buffer cannot take
// the name plus eight.
int fs_GetTableName(void* pTable, uint32 iEntry, char* pName, uint32 nMaxLen)
{
	if (nMaxLen < 8)
		return 0;

	if (strlen((char*)pTable) > nMaxLen - 8)
		return 0;

	sprintf(pName, "%s%d %s", "#$#", iEntry, (char*)pTable);
	return 1;
}

// The console/error printer (0x1000e3f9).  de_error.h names it; this file
// does not include that header, so the prototype is repeated.
void dsi_ServerError(char* pFormat, ...);

// 0x1000d040 — commonmodel.cpp's out-of-line three-float read, which is what
// the shipped body CALLS here.  This file's own `w_ReadVector` above is
// marked `inline` for w_LoadWorldModel's sake and so has no out-of-line
// copy to call; the call is therefore spelled through a declaration of its
// own.  Relocations are masked, so the two names cost nothing.
void w_ReadVectorQT1(class ILTStream* pStream, LTVector* pVec);

#ifndef LT_SERVER_BUILD
// lithtech.exe only: stdlith's SetSizeInit2 -> SetSizeInit3, transcribed
// from src/LT2/lithshared/stdlith/dynarray.h:696/703. The /O2 CWorldSimple::Load
// expands both and still CALLS SetSize2 (0x00425f74 -> 0x00414fa0), with
// the fill loop in line. server.dll calls 0x1000a9f0's SetSizeInit2 out of
// line, so the server build keeps seeing only the declaration.
DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::SetSizeInit3( uint32 newSize, T &val, LAlloc *pAlloc )
{
	uint32 i;

	if(!SetSize2(newSize, pAlloc))
		return LTFALSE;

	for(i=0; i < GetSize(); i++)
	{
		m_pArray[i] = val;
	}

	return LTTRUE;
}

DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::SetSizeInit2( uint32 newSize, T val )
{
	return SetSizeInit3(newSize, val, &g_DefAlloc);
}

// lithtech.exe only: the /O2 CWorldSimple::Load (0x00425df0) carries
// wb_ReadSimpleHeader expanded (0x00425ffb..0x0042603c) and there is no
// standalone copy in that image. An inline twin, because marking the real
// one `inline` would also expand it at /O1 and lose server.dll's
// standalone 0x1000aa81 (stand-in; name ours).
inline void wb_ReadSimpleHeaderInline(ILTStream* pStream, CWorldSimpleHeader* pHeader)
{
	*pStream >> pHeader->m_n00;
	*pStream >> pHeader->m_n01;
	*pStream >> pHeader->m_n02;
	*pStream >> pHeader->m_n03;
	*pStream >> pHeader->m_n04;
}
#endif


LTBOOL CWorldSimple::Load(CWorldBsp* pParent, ILTStream* pStream, uint32 nIndex)
{
	uint32 i, j;
	uint32 n;
	uint32 nNodes;
	uint32 nParts;
	uint32* pDest;
	void* pNode;
	CWorldSimple* pWorld = this;             // see the note above

	Reset();                                 // 0x1000a8c6

	m_p7c = (class CWorldSurfaces*)pParent;  // 0x1000a8d7

	if (!fs_GetTableName(pParent->m_Name, nIndex, m_Name, sizeof(m_Name)))
	{
		dsi_ServerError("** Terrain %s's name is too long, can't hold TerrainSections.");
		return LTFALSE;                      // 0x1000a8f5 -> 0x1000aa45
	}

	*pStream >> nNodes;  // 0x1000a907, -0xc

#ifdef LT_SERVER_BUILD
	if (!m_Array68.SetSize2(nNodes, &g_DefAlloc))   // 0x1000a915
#else
	if (!m_Array68.SetSize(nNodes))          // lithtech 0x00425e58, a call
#endif
		return LTFALSE;                      // no Reset

	for (i = 0; i < nNodes; i++)             // 0x1000a924 / 0x1000a9b1
	{
		pDest = (uint32*)&pWorld->m_Array68.GetArray()[i];      // 0x1000a933

		((CWorldItem18*)pDest)->m_flag = 8;  // 0x1000a941

		*pStream >> n;        // 0x1000a947

		if (n >= pParent->m_nParts)          // 0x1000a950
			goto fail;

#ifdef LT_SERVER_BUILD
		j = 0;                               // 0x1000a962

		*pDest = (uint32)pParent->m_ppParts[n];   // 0x1000a969
		pDest++;
#else
		// lithtech 0x00425eae: the parts pointer is loaded off pParent's
		// register before that register becomes j.
		*pDest = (uint32)pParent->m_ppParts[n];
		pDest++;

		j = 0;
#endif

		for (; j < 2; j++, pDest++)          // 0x1000a9a2
		{
			*pStream >> n;    // 0x1000a978

			pNode = w_GetTreeNodeQT1(pWorld->m_Array68.GetArray(),
			                         pWorld->m_Array68.GetSize(), n);  // 0x1000a986

			*pDest = (uint32)pNode;          // 0x1000a98e

			if (!pNode)                      // 0x1000a990
				goto fail;
		}
	}

	*pStream >> n;            // 0x1000a9bf

	m_n64 = (uint32)w_GetTreeNodeQT1(m_Array68.GetArray(),
	                                 m_Array68.GetSize(), n);  // 0x1000a9cd

	if (!m_n64)                              // 0x1000a9d5
#ifdef LT_SERVER_BUILD
		goto fail;
#else
	{
		// lithtech 0x00425f44: this refusal keeps a Reset tail of its own.
		Reset();
		return LTFALSE;
	}
#endif

	*pStream >> nParts;  // 0x1000a9e6, -0x10

	if (!m_Array50.SetSizeInit2(nParts, (class CWorldPart*)LTNULL))  // 0x1000a9f0
		goto fail;

	for (i = 0; i < nParts; i++)             // 0x1000a9f9 / 0x1000aa2c
	{
		*pStream >> n;        // 0x1000aa08

		if (n >= pParent->m_nParts)          // 0x1000aa11
			goto fail;

		m_Array50.m_pArray[i] = pParent->m_ppParts[n];   // 0x1000aa25
	}

	// The cleanup arm is the BODY of this test and every earlier failure
	// jumps into it, which is what the shipped `jne 0x1000aa49` says: the
	// test jumps OVER the block rather than into it, and the success tail
	// falls out of the bottom.
	if (!m_Sub80.Load(pStream))              // 0x1000aa35
	{
fail:
		Reset();                             // 0x1000aa40

		return LTFALSE;                      // 0x1000aa45
	}

#ifdef LT_SERVER_BUILD
	wb_ReadSimpleHeader(pStream, &m_Header48);       // 0x1000aa4e
	w_ReadVectorQT1(pStream, &m_vCenter);            // 0x1000aa5b
#else
	// lithtech 0x00425ffb..0x00426063: both reads expanded.
	wb_ReadSimpleHeaderInline(pStream, &m_Header48);
	// One pointer walks the vector (`lea 0xb0(%edi),%ebx`, then `lea
	// 0x4(%ebx)` and `add $0x8,%ebx`), as w_ReadVector's expansion would --
	// but calling w_ReadVector (or three direct Reads) leaves VC6 room to
	// expand SetSize2 inside SetSizeInit2 above (+94 bytes), which the
	// shipped body calls; three inline `>>` sites here keep it a call.
	// Residual: `this` lands in %ebp and the walking pointers in %edi where
	// shipped has %edi / %ebp / %ebx (669 of 670 bytes, same instructions).
	{
		LTVector* pVec = &m_vCenter;
		*pStream >> pVec->x;
		*pStream >> pVec->y;
		*pStream >> pVec->z;
	}
#endif
	w_SetNodeProjectionsQT1(m_Array68.m_pArray,
	                        m_Array68.m_nElements, 1);   // 0x1000aa68
	ComputeRadius();                         // 0x1000aa72

	return LTTRUE;                           // 0x1000aa77
}
