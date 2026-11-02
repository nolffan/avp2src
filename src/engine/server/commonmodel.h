// *********************************************************************** //
//  MODULE:   COMMONMODEL (.H)
//  PURPOSE:  The shared half of ILTModel — the animation-tracker accessors the
//            engine implements once for both client and server, at
//            0x1001cff6-0x1001e055. The server-specific overrides that
//            broadcast a change to the clients live separately, at
//            0x1002dc22+, and report themselves as "ServerModelLT::".
//
//            The class name is ours. Only "MyServerInterface" is attested by a
//            surviving literal; everything in this file reports itself as
//            "ILTModel::", the interface.
//
//            LTAnimTracker's layout comes from the public ltanimtracker.h and
//            needs no reconstruction: m_Link 0x00, m_StringKeyCallback 0x0c,
//            m_pUser1 0x10, m_CurKey 0x14, m_Flags 0x18, m_InterpolationMS
//            0x1a, m_TimeRef 0x1c (m_pModel 0x1c, m_Prev 0x20, m_Cur 0x28,
//            m_Percent 0x30, m_iWeightSet 0x34). Every offset the accessors
//            use lands on a named field, which is the confirmation that the
//            shipped header and the shipped code agree.
// *********************************************************************** //
#ifndef __COMMONMODEL_H__
#define __COMMONMODEL_H__

#include "ltanimtracker.h"
#include "ltobject_internal.h"
#include "collection.h"

// The array allocator with constructor arguments. stdlith's own `BaseNew`
// (collection.h) default-constructs its elements; the model reader needs the
// two forms that pass the element's constructor one and two arguments, and
// the shipped image carries six such instantiations in one run at
// 0x1001cc25-0x1001cdd5 — element sizes 0x30, 0xb4, 0x34, 0x28, 0x6c and
// 0x5c, each with its own element constructor baked in and each reached with
// a five- or four-dword cdecl argument list.
//
// THESE BELONG IN collection.h, next to the three-argument BaseNew they
// overload; they are here only because that file is not ours to edit.
template<class T, class A1>
inline T* BaseNew(LAlloc* pAlloc, T* dummy, uint32 nElements, A1 arg1)
{
	T *ptr, *pTemp;

	ptr = (T*)pAlloc->Alloc(sizeof(T) * nElements);
	if (!ptr)
		return 0;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		::new(pTemp, (DummyClassBlahBlah*)0) T(arg1);
		++pTemp;
	}

	return ptr;
}

template<class T, class A1, class A2>
inline T* BaseNew(LAlloc* pAlloc, T* dummy, uint32 nElements, A1 arg1, A2 arg2)
{
	T *ptr, *pTemp;

	ptr = (T*)pAlloc->Alloc(sizeof(T) * nElements);
	if (!ptr)
		return 0;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		::new(pTemp, (DummyClassBlahBlah*)0) T(arg1, arg2);
		++pTemp;
	}

	return ptr;
}

// LTAnimTracker::m_Flags. ltanimtracker.h refers to "AT_ flags above" but the
// SDK copy does not define them — they are engine-internal. Bit 0 is tested by
// GetPlaying/SetPlaying and bit 1 by GetLooping/SetLooping.
#define AT_PLAYING  (1 << 0)
#define AT_LOOPING  (1 << 1)

// One animation. Only its name is reached from here (name ours).
// One keyframe: the time it lands on and four dwords of payload.
class AnimKeyFrame
{
public:
	// 0x1001651d — the defaults: everything zero and the string on the
	// shared empty string. It exists because the keyframe collection's
	// allocator runs it over every element (see CUnknownD9E4 in
	// commonmodel.cpp).
	AnimKeyFrame();

	uint32 m_Time;                           // 0x00
	// The two payloads at_DoKeyFrame reaches, and the byte that says which
	// of them is live: type 0 is a string key (the string at +0x04, handed
	// to the tracker's own callback) and type 1 is a direct call (the
	// function at +0x0c, gated on bit 2 of the tracker's flags). Any other
	// type does nothing at all. The record's 0x14 bytes come from the
	// `lea (%edi,%edi,4)` / `lea (%ecx,%eax,4)` pair that indexes it and
	// from the `idiv $0x14` that turns a keyframe pointer back into an
	// index (names ours).
	char*  m_pString;                        // 0x04
	uint8  m_Type;                           // 0x08
	uint8  m_pad_09[0x0c - 0x09];
	StringKeyCallback m_pFn;                 // 0x0c
	// 0x10 — the fourth dword the constructor clears. Nothing reads it in
	// the reconstructed set; it exists because 0x1001651d writes it
	// (name ours).
	uint32 m_Unknown10;                      // 0x10
};

// 0x1c bytes of transform, and the record the animation's per-node tables are
// made of: a three-float position at +0x00 read by w_ReadVector and a
// four-float quaternion at +0x0c read by w_ReadRotation (name ours).
//
// IT HAS A USER-DECLARED, void-RETURNING ASSIGNMENT OPERATOR, and
// the shipped image says so at both optimisation levels:
//
//   * 0x10016723 (42 bytes) in server.dll and 0x004457e0 (55) in lithtech.exe
//     are the same function — a thiscall that copies twelve bytes and then
//     sixteen and ends `mov 0x18(%eax),%eax; ret $0x4`, leaving `this`
//     NOWHERE. A compiler-generated operator= returns `this`, so this one was
//     written by hand and returns void. stdlith spells assignments that way
//     elsewhere too (BaseGenListPos::operator= in genlist.h).
//   * server.dll's CMoArray<CModelXform,NoCache>::CopyArray2 (0x1001b6a3)
//     CALLS 0x10016723 from its element loop (0x1001b6fb, stride 0x1c), and
//     lithtech.exe's CUnknownD974::sub_100165e5 (0x4456f0), which is the same
//     copy inlined whole, CALLS 0x4457e0 (0x445794). Our raw-float spelling
//     gives both a `rep movsl` of seven dwords instead.
//   * 0x10016723 lies between sub_1001671c (0x1001671c) and CUnknownD9E4's
//     constructor (0x1001674d), i.e. inside this translation unit's range.
//     collection.cpp's CCollElem1CV was built as a stand-in for exactly this
//     address, and that class — a plain twelve-byte head plus a sixteen-byte
//     sub-object with an assignment operator of its own — reproduces BOTH
//     shipped bodies byte for byte today. This is that class, named.
//
// So the two halves are sub-objects, not raw arrays, and they differ: the head
// has no operator= of its own (the compiler's memberwise copy, which VC6
// renders as three `movsl` at /O1 and three dword move pairs at /O2 — see
// gp_SetRotation / gp_SetVector in genericprop.cpp, which match in both images
// doing exactly that to an LTVector), while the tail's four dwords are moved
// one at a time, which is an operator= of its own being expanded.
//
// That is precisely the shape of LTVector (no operator=) and LTRotation (an
// in-class operator= assigning m_Quat[0..3] and returning *this), and the
// `(LTVector*)` / `(LTRotation*)` casts in Model::CModelSub1D4::sub_1001bf0d
// say the real source used them -- for the OTHER 0x1c record, CChildNodeXform
// below, which is exactly that pair -- and for this one too (see the
// operator's own note).
//
// CModelXform DOES have an empty default constructor and an in-class copy
// constructor that assigns its two members. The earlier "no constructor"
// reading was wrong, and its matches were look-alike credits from modelnew's
// CAnimXformQ444:
//   * BaseNew<CModelXform> (0x1001b71a) keeps the placement-new loop, folded
//     to a `jne` onto the next instruction. Without a constructor ours
//     inlined it and CopyArray2/Insert2/Remove2/InternalNiceSetSize
//     (0x1001b6a3, 0x1001a2b4, 0x1001a371, 0x10019633) missed.
//   * GenGetNext/GenGetAt (0x10018124/0x10018148) CALL a copy (0x100195b8)
//     where an implicit copy constructor gives a `rep movsl`.
class CModelXform
{
public:
	CModelXform() {}

	CModelXform(const CModelXform& other)
	{
		m_Pos = other.m_Pos;
		m_Rot = other.m_Rot;
	}

	// 0x10016723 (42 bytes) / 0x004457e0 (55). DEFINED IN CLASS, and the
	// members are the SDK's own LTVector and LTRotation, not look-alikes of
	// them. lithtech.exe's /O2 CMoArray<CModelXform, NoCache> (vtable
	// 0x004a2708: GenAppend 0x00447a20, GenRemoveAt 0x00447b80, GenCopyList
	// 0x00447d10, GenAppendList 0x00447e60) EXPANDS this body -- the head as
	// dword moves, then a CALL to LTRotation::operator= (0x0047c460) -- which
	// an out-of-class definition can never give (/Ob1). The out-of-line copy
	// is still emitted for the sites that decline it (0x00445794 in
	// lithtech.exe, every element loop of server.dll's /Os arrays), so both
	// addresses stay matched. With local stand-ins for the two member types
	// (a float triple and a four-float class with its own operator=)
	// GenRemoveAt came out 405 bytes against 387 -- a different loop shape
	// -- so the member types are load-bearing too. Those five /O2 functions
	// used to be credited to collection.cpp's CMoArray<CCollElem1CS,
	// NoCache>, an instantiation the shipped image does not have (server.dll
	// says the NoCache 0x1c array assigns through this VOID operator, not
	// through CCollElem1CS's `this`-returning 0x100195b8).
	void operator=(const CModelXform& other)
	{
		m_Pos = other.m_Pos;
		m_Rot = other.m_Rot;
	}

	LTVector   m_Pos;                        // 0x00
	LTRotation m_Rot;                        // 0x0c
};

// 0x1c bytes, and NOT CModelXform: the per-node record of a child-model
// record (Model::CModelSub1D4::m_Array00). Its DefaultCache array's element
// copies all reach 0x100195b8 -- as the assignment in GenCopyList
// (0x1001820a) and as the copy in GenGetNext/GenGetAt (0x1001813b,
// 0x10018159) -- which returns `this`, while the NoCache CModelXform array
// assigns through 0x10016723, which returns void. Two assignment operators
// means two element types. sub_1001bf0d reads it with w_ReadVector and
// w_ReadRotation, so it is the SDK pair itself, with every special member
// implicit (name ours).
class CChildNodeXform
{
public:
	LTVector   m_Pos;                        // 0x00
	LTRotation m_Rot;                        // 0x0c
};

// Model::m_Unknown054's element, 0x14 bytes: four floats and a dword, with
// an out-of-line constructor at 0x10016478 that zeroes all five (the dword
// first). BaseNew for it (0x1001b97b, `lea (%esi,%esi,4); shl $2`) calls
// that constructor per element, which is what ~Model's release of the
// collection (0x10019d2e) reaches. Fields by offset (names ours).
class CModelVertRec
{
public:
	CModelVertRec();

	float  m_f[4];                           // 0x00
	uint32 m_n10;                            // 0x10  read first
};

// Model::m_Unknown08c's element: one float, zeroed by an in-class
// constructor -- BaseNew for it (0x1001ba2b) is the `fldz; fstps` loop, the
// shape a four-byte element with a constructor gives and a bare float or
// pointer does not (name ours).
class CModelFloatRec
{
public:
	CModelFloatRec() { m_f = 0.0f; }

	float m_f;
};

class ModelAnim
{
public:
	// 0x1001690b — how long the animation runs, which is the time on its
	// last keyframe. An animation with no keyframes has no length.
	uint32 GetLength();

	// THIS CLASS AND `CUnknownD9E4` IN commonmodel.cpp ARE THE SAME CLASS.
	// The reader at 0x1001bc69 is what proves it: reached as
	// `AnimInfo::m_pAnim->...`, it fills +0x0c/+0x10 with a keyframe array
	// of 0x14-byte records (this class's m_pKeyFrames / m_nKeyFrames), the
	// dword at +0x1c with the file's interpolation time
	// (m_nInterpolationTime), and +0x24 with the name (m_pName) — every one
	// of them at the offset this class already had, and CUnknownD9E4's own
	// constructor at 0x1001674d allocates exactly 0x5c bytes for them.
	//
	// They are still two declarations, and the reason is a real conflict
	// rather than an oversight. The keyframe table is a
	// `CMoArray<AnimKeyFrame, NoCache>` member at +0x08 — the destructor at
	// 0x100167b4 re-stores that collection's vtable and Term()s it, so it
	// HAS to be a member for the teardown to be emitted — but GetLength
	// below needs the element count `volatile`, which a collection member
	// cannot carry. Unifying them means finding the spelling that gets
	// both, and it is not `volatile` on the member (which would make the
	// whole collection volatile) nor a volatile-qualified GetLength (not
	// probed yet).

	uint8         m_pad_00[0x0c];
	AnimKeyFrame* m_pKeyFrames;              // 0x0c
	// volatile, and the bytes insist on it: GetLength compares the count
	// against memory and then loads it a second time for the subscript,
	// which no non-volatile source shape reproduces (twelve probed — the
	// guard as `> 0`, `!= 0`, `== 0`, `>= 1`, through an inline accessor,
	// through an inline predicate, as a ternary, through a result variable,
	// and with the pointer volatile instead; every one of them loads the
	// count once into a register). A model is loaded on the worker thread
	// and its keyframes are published to whoever is already animating it,
	// so a volatile count is what the shipped source meant.
	volatile uint32 m_nKeyFrames;            // 0x10
	uint8         m_pad_14[0x1c - 0x14];
	// How long a blend into this animation lasts. at_Advance runs the
	// tracker's own interpolation counter up against this and divides by it
	// for the blend percentage, so it is the total in milliseconds (name
	// ours).
	uint32        m_nInterpolationTime;      // 0x1c
	// 0x20 — the model this animation belongs to. Model's own release
	// (0x100170f2) walks the animation table and gives back exactly the
	// records whose +0x20 is itself, so an animation that came in with a
	// child model is left for that model to free (name ours).
	class Model*  m_pModel;                  // 0x20
	char*         m_pName;                   // 0x24

	// An in-class one-liner, and FindAnimInModel is what says it exists: the
	// shipped call there materialises the name into a register first
	// (`mov 0x24(%eax),%eax; push %eax`) where a raw member read gives
	// `push 0x24(%eax)`. That is the documented accessor fingerprint. Model's
	// own FindAnim reads the field directly and matches that way, so the
	// original had both spellings.
	char* GetName() { return m_pName; }
};

// THIS CLASS AND `Model::CModelSub20C` BELOW ARE THE SAME CLASS. Three
// independent facts say so: the record is 0xb4 bytes (CModelSub20C's own
// factory allocates exactly that) and the name is at +0xb0 in both; the
// reader's node pass (0x1001bd54) builds the tree out of CModelSub20C's own
// factory slot and stores the results in Model::m_Nodes; and 0x10017517
// walks m_Nodes reading the 4x4 at +0x2c, which is CModelSub20C's m_Mat2c.
// They are still two declarations because CModelSub20C is nested inside
// Model — which is declared after this — and one of its instances is
// embedded in Model by value, so it cannot simply be moved up here.
//
// The fields below are the ones 0x10017517 and 0x1001c1a5 reach; the rest of
// the layout is on CModelSub20C (names ours).
class ModelNode
{
public:
	uint8    m_pad_00[0x04];
	// 0x04, 0x08, 0x0c — where this node sits relative to its parent,
	// recomputed by 0x10017517 as the translation of the parent's inverse
	// times this node's own transform. A root node (no parent in range)
	// gets zero.
	float    m_fRelX;                        // 0x04
	float    m_fRelY;                        // 0x08
	float    m_fRelZ;                        // 0x0c
	uint8    m_pad_10[0x28 - 0x10];
	// 0x28 — this node's parent, as an index into the model's node array.
	// A root's index is out of range (the tree writes all-ones), which is
	// the test 0x10017517 makes.
	uint32   m_iParent;                      // 0x28
	LTMatrix m_Transform;                    // 0x2c
	uint8    m_pad_6c[0xb0 - 0x6c];
	char*    m_pName;                        // +0xb0
};
// EvalSocket reads every field of a socket: the offset the attachment sits
// at, the rotation it sits at, and the node it hangs off. The name's extent
// is fixed by the node index that follows it (names ours).
class ModelSocket
{
public:
	// 0x10016c95 — the origin, an identity rotation, no name and no node.
	// The 1.0f it puts at +0x18 is the quaternion's w, which is what fixes
	// the four floats at +0x0c as a rotation and not another vector.
	ModelSocket();

	LTVector   m_vPos;                       // 0x00
	LTRotation m_rRot;                       // 0x0c
	char       m_Name[0x2c - 0x1c];          // 0x1c
	uint32     m_iNode;                      // 0x2c
	// 0x30 — the constructor zeroes it and nothing else here reads it. The
	// record is 0x34 bytes, which its own allocator pins: LoadSockets goes
	// through the BaseNew instantiation at 0x1001ccaa, and collection.cpp
	// records that one's element size as 0x34 (name ours).
	uint32     m_Unknown30;                  // 0x30
};

// The two remaining named tables; both keep the name inline (names ours).
// One piece: 0x6c bytes, which its own release pins (BaseDelete at
// 0x1001b641 strides by 0x6c and runs a destructor at 0x10016c14) and its
// reader confirms (0x1001c1a5 fills 0x24 through 0x68 and reads the name
// with a limit of 0x20 into +0x48, and 0x48 + 0x20 + the model pointer lands
// exactly on 0x6c). It is NOT a POD: the destructor the release runs is a
// real one, which is why it is declared here.
//
// The first 0x24 bytes are a record of their own — the vertex-data record
// 0x1001bf85 reads, which is called both on the piece itself and on each
// element of the 0x24-strided collection at +0x34, so the piece opens with
// one of them and carries a list of the rest (names ours).
class ModelPiece
{
public:
	// 0x1001c1a5 — this piece off the wire. The second argument is a running
	// base the reader advances (name ours).
	LTBOOL sub_1001c1a5(class ILTStream* pStream, uint32* pnBase);

	// 0x10016bdb and 0x10016c14 — build one against a model, and take it
	// apart. Both declared only: the reader that allocates pieces
	// (0x1001c604) is not reconstructed yet, and the destructor is reached
	// only through BaseDelete (names ours).
	ModelPiece(class Model* pModel);
	~ModelPiece();

	uint8  m_pad_00[0x08];
	// 0x08 and 0x28 — how much of the model's vertex data this piece takes,
	// and where it starts. 0x100171f4 walks the piece table assigning each
	// one the running total of the sizes before it, which is what pairs
	// them (names ours).
	uint32 m_nSize;                          // +0x08
	uint8  m_pad_0c[0x24 - 0x0c];
	// 0x24 — a word off the wire, widened to a dword (`movzwl` then a
	// four-byte store), read first of everything the piece carries.
	uint32 m_Unknown24;                      // +0x24
	uint32 m_nOffset;                        // +0x28
	float  m_f2c;                            // +0x2c
	float  m_f30;                            // +0x30
	// 0x34 — the rest of the piece's vertex-data records, 0x24 bytes each,
	// one per whatever the model counts at its own +0x94.
	uint8  m_Array34[0x44 - 0x34];           // +0x34
	// 0x44 — one more dword, and only for file version 10 and up.
	uint32 m_Unknown44;                      // +0x44
	char   m_Name[0x68 - 0x48];              // +0x48
	class Model* m_pModel;                   // +0x68
};

// One weight set: a sixteen-byte name and one four-byte weight per node.
// 0x28 bytes, which is what its own allocator bakes in (0x1001cce3), and its
// constructor takes the model (0x10016c4e) — which is where the collection at
// +0x10 gets the allocator it grows against (names ours).
class ModelWeightSet
{
public:
	// 0x10016c4e — build one against a model. Declared only (name ours).
	ModelWeightSet(class Model* pModel);
	// 0x10016c69 / 0x004462c0 -- the weights back to the model's allocator.
	// Out of line: /O2's BaseDelete calls it (0x0044dd6e) rather than
	// expanding it.
	~ModelWeightSet();
	// 0x1001c2bc — the set off the wire: the name, the count, then one
	// weight per node (name ours).
	LTBOOL sub_1001c2bc(class ILTStream* pStream);

	// The name is read with ILTStream::ReadString straight into the record
	// with a limit of 0x10, which is what fixes its size.
	char   m_Name[0x10];                     // +0x00
	CMoArray<uint32, DefaultCache> m_Weights;// +0x10 .. 0x24
	class Model* m_pModel;                   // +0x24
};

// One entry per animation in the model's table; the animation itself is the
// first field, which is why the accessors index by 0x20 and then dereference.
class AnimInfo
{
public:
	// 0x10016922 — out of line: every element BaseNew makes for the
	// model's animation table (0x1001ba5b, stride 0x20) calls it.
	AnimInfo();

	// 0x1001bec9 — this entry off the wire: the dimensions, then the
	// animation record itself (name ours).
	LTBOOL sub_1001bec9(class ILTStream* pStream);

	ModelAnim* m_pAnim;                      // 0x00
	// 0x04 — the child-model record the animation belongs to. Model's own
	// reader fills it (`mov %ecx,0x4(%edi,%edx,1)` with `lea 0x1d4(%ebx)`
	// in ecx) for every entry it allocates, which is the model's own
	// record, and the animation reader then re-homes it onto whatever
	// child the file names (name ours).
	// (typed void* because CModelSub1D4 is nested inside Model, which is
	// declared below; nothing here dereferences it.)
	void*      m_pChildModel;                // 0x04
	LTVector   m_vDims;                      // 0x08  what ModelEdit set
	// 0x14 — what the "AnimBindings" chunk writes (0x0044dad1-0x0044dade).
	LTVector   m_vAnimBinding;               // 0x14
};

// What a model's memory is taken from: an interface whose vtable slot 1 is
// the allocate. Nothing here needs more of it than that (name ours; the two
// `uint32*` allocator parameters elsewhere in this file are the same thing
// seen without its shape).
class IMoAllocator
{
public:
	virtual void  _v00() = 0;
	virtual void* Alloc(uint32 nBytes) = 0;   // 0x04
};

// 0x1003d3f0 — take a run of bytes from one of those. The size is rounded up
// to a multiple of four and a zero-length request allocates nothing and still
// succeeds; a refused request leaves the null in the data pointer and answers
// LTFALSE. The four fields below are every field that function touches — it
// also calls something on `this` first, so the record is longer than 0x14 and
// its extent is not pinned by anything read so far (names ours).
class CMoMemBlock
{
public:
	// 0x1003d330 — stores a vptr (0x1004eb00) and sets up an SEH frame, so
	// it owns something whose construction can unwind. Model's constructor
	// only calls it, and calls it through a masked relocation, so the
	// declaration is all that is needed here.
	CMoMemBlock();                           // 0x1003d330
	// 0x1003d3a0 — above the app-region cut with its constructor, so only
	// the declaration is needed; Model's destructor calls it at +0x17c.
	~CMoMemBlock();                          // 0x1003d3a0
	LTBOOL Alloc(IMoAllocator* pAlloc, uint32 nSize);   // 0x1003d3f0

	uint8         m_pad_00[0x04];
	IMoAllocator* m_pAlloc;                  // 0x04
	void*         m_pData;                   // 0x08
	uint32        m_Unknown_0c;              // 0x0c  set to zero by Alloc
	uint32        m_nSize;                   // 0x10  the rounded size
};

// 0x100163fa — a slot that can be filled once: a second call is refused with
// LTFALSE and changes nothing. Model builds one at +0x84 with the first of
// its two allocators (names ours).
// 0x1001638c is what the class turns out to be: not a "data reference" but
// the model's own STRING STORE. Every name a model file carries — the model's
// own, each animation's, each keyframe's script key — is handed to it and
// comes back as a pointer the model owns, and an identical name handed to it
// twice comes back as the SAME pointer, because the store searches before it
// allocates. One block per distinct string, each carrying its own length and
// the next block's address ahead of the bytes.
class CMoDataLink
{
public:
	// 0x00, 0x04 and 0x08, read off 0x1001638c: the block's total size
	// (strlen + 0xc, so twelve bytes of overhead for two dwords and the
	// terminator's alignment), the next block, and the string itself
	// (names ours).
	uint32       m_nSize;                    // 0x00
	CMoDataLink* m_pNext;                    // 0x04
	char         m_Name[1];                  // 0x08
};

class CMoDataRef
{
public:
	// 0x10016352 — build one: the flag word cleared (`andl $0x0` — /O1's
	// four-byte store of zero) and the pointer taken from the one argument,
	// with `mov %ecx,%eax` hoisted to the top of the body because loading
	// that argument is what clobbers `this`. It hands `this` back in eax,
	// which is what says it is a constructor rather than an initialiser.
	// Model builds the one at its +0x84 with its first allocator, so what
	// the pointer means is the caller's business; from here it is only the
	// same slot Set fills.
	CMoDataRef(void* pData);                 // 0x10016352
	// 0x10016361 — five bytes, a tail `jmp` into the release below, which
	// is the shape VC6 gives a destructor whose whole body is one call.
	~CMoDataRef();                           // 0x10016361
	// 0x10016366 — walk the block list and hand every block back to the
	// allocator, then clear the head. Reached from the destructor and from
	// Model's own release, so it is a member in its own right and not the
	// destructor body. (The earlier reading of the link layout here — next
	// at +0x00, an allocator at +0x04 — was wrong: 0x1001638c builds the
	// blocks with the SIZE at +0x00 and the next at +0x04, and the
	// allocator is the store's, not the block's.)
	void   sub_10016366();                   // 0x10016366
	LTBOOL Set(void* pData);                 // 0x100163fa
	// 0x1001638c — a name into the store, and the copy the store owns back
	// out. A null or empty name, and a refused allocation, all answer the
	// shared empty string rather than null, which is why every reader can
	// strlen what it gets back (name ours).
	char*  sub_1001638c(char* pName);

	// 0x00 — the head of the block list, and the same word Set treats as
	// "already claimed": a store with anything in it will not take a new
	// allocator.
	CMoDataLink* m_pLinks;                   // 0x00
	// 0x04 — where the blocks come from. Model builds the store at its
	// +0x84 with its first allocator, and 0x1001638c reaches slot 1 of it,
	// which is what says the field is the allocator and not payload.
	class LAlloc* m_pAlloc;                  // 0x04
};

// The thirteen dwords the model file's "Header" chunk is: 0x1001ce0a reads
// them one at a time through the stream's slot 8 (four bytes each, the count
// held in ebx across all thirteen) into this+0x00 .. this+0x30 and then asks
// the stream whether anything went wrong, so the record is exactly 0x34 bytes
// and every field of it is a dword off the wire. Which count is which is not
// readable from here — only 0x1001cec1's arithmetic over them is — so they
// stay an array (names ours).
//
// 0x1001cde1 clears it: `xor eax,eax` and thirteen stores. It is NOT the
// constructor, even though CModelLoadState's constructor is the only thing
// that calls it — a VC6 constructor hands `this` back in eax and this one
// leaves the zero register there, so it is an ordinary void member and the
// load state's constructor body calls it (name ours). It also has to stay out
// of line: /O1 implies /Ob1, so a body written in the class declaration would
// be expanded into the caller and the call at 0x1001cdd8 would vanish.
class CModelFileCounts
{
public:
	void   Clear();                          // 0x1001cde1
	LTBOOL Read(class ILTStream* pStream);   // 0x1001ce0a

	uint32 m_n[13];                          // 0x00 .. 0x30
};

// 0x1001cdd5 — the scratch the model reader keeps while it walks a file:
// 0x34 bytes on Model::Load's own stack (the frame is 0x38 and the byte
// count sits in the last dword), and a constructor that builds one
// sub-object at offset zero and does nothing else. Only the one method
// Load reaches is declared, and it is named for its address because what it
// answers is not readable from the call site alone — it hands back a byte
// count and a flag for whether there is anything to allocate at all
// (names ours).
class CModelLoadState
{
public:
	CModelLoadState();                       // 0x1001cdd5
	LTBOOL sub_1001cec1(uint32* pnBytes);    // 0x1001cec1

	CModelFileCounts m_Counts;               // 0x00 .. 0x34
};

class Model
{
public:
	// Two allocators, and the same two at both call sites: the manager's
	// placeholder is built with the packet bank's, and a model loaded from
	// file with the collection allocator's.
	//
	// NEAR (546 of 560 bytes, and the fourteen are one inline decision).
	// The body is reconstructed in commonmodel.cpp and every statement of
	// it is accounted for; what does not reproduce is that the shipped
	// build expanded `CMoArray<T, NoCache>::CMoArray(uint32)` inline at the
	// first of the two collection temporaries, and no VC6 in toolchains/
	// will do that at /Ob1. The full finding is on the definition.
	//
	// Fourteen sub-objects run before the body, in offset order, which is
	// member declaration order: 0x14 (0x100243f6), 0x24 (0x1001831d),
	// 0x34 (0x10018b5e), 0x44 (0x10018c1b), 0x54 (0x10018c92),
	// 0x70 (0x10018e8f), 0x84 (0x10016352, one argument — pAlloc1),
	// 0x8c (0x10019104), 0xa0 (no call at all — the one member the
	// constructor initialises from its list, `fldz`/`fstps` between the
	// 0x8c and 0x154 constructors, which is where declaration order puts
	// it and nowhere the body could be scheduled to), 0x154 (0x1001927c),
	// 0x164 (0x10019364), 0x17c (0x1003d330 — CMoMemBlock), 0x1d4
	// (0x100164bb) and 0x20c (0x1001694e).
	//
	// The two temporaries are both whole `CMoArray<T, NoCache>` locals and
	// they share the single 0x10-byte stack slot the frame reserves, which
	// is what says their scopes are disjoint. Each is `CopyArray2`'d into a
	// member (0x1001b76a for the one at 0x24, 0x1001b9b5 for the one at
	// 0x70, both with &g_DefAlloc, i.e. operator= -> CopyArray) and then
	// destroyed where its scope ends: the vptr re-stored and Term() run
	// (SetSize2 0x1001a03d and 0x10019e14). Both are NAMED locals rather
	// than temporaries, and the second one proves it — its constructor is
	// an out-of-line call that hands `this` back in eax, and the shipped
	// code still recomputes the address with `lea -0x10(%ebp),%eax` instead
	// of using that return value, which is what a named local does and a
	// temporary does not.
	Model(uint32* pAlloc1, uint32* pAlloc2);   // 0x10016cc5

	// Polymorphic: FreeUnusedModels deletes through vtable slot 0.
	// 0x10016f26 — the body is three statements (the release call below and
	// two subtractions off the running data total); the thirteen sub-object
	// destructors after it are the compiler's, in reverse declaration
	// order, and they are what confirms the member list end to end.
	virtual ~Model();

	// 0x10017016 — give back everything the model's data block owns: the
	// sub-objects at +0x14 and +0x20c, the collections at 0x24, 0x54, 0x70,
	// 0x44 and 0x154, the data reference at 0x84 and the filename. Reached
	// only from the destructor in the reconstructed set, with a literal 1,
	// so what the flag selects is not readable and the name is its address.
	void   sub_10017016(LTBOOL bFlag);
	// 0x100170f2 — the animation table given back: every record whose
	// animation says it belongs to THIS model is released, and the table
	// itself is then emptied against the model's own allocator (+0x178)
	// rather than its data block, which is the one place in the release
	// that uses the other one. An animation that came in with a child model
	// belongs to that model and is left alone (name ours).
	void   sub_100170f2();
	// 0x10017145 — the child-model table reset to just this model's own
	// record. Every other record is unhooked: the model it stands for loses
	// a reference (m_CacheCount) and, if the flag is set and that was the
	// last one, is deleted outright; then the record itself is reset and,
	// unless it is one of the two the model owns, handed back. The flag is
	// what the whole release takes as its argument (name ours).
	void   sub_10017145(LTBOOL bFlag);
	// 0x100171f4 — everything the model has to recompute once its node
	// tree and its data are in place, and the one function the reader calls
	// twice (once mid-file with 1, once at the end with 0). It resizes the
	// two node-indexed tables (0x1001728c, 0x100172e3), lays the pieces out
	// end to end in the vertex data, hands every animation of its own the
	// same flag, and finally fills the two counters at +0x64/+0x68. Any
	// step failing stops it (name ours).
	LTBOOL sub_100171f4(LTBOOL bFlag);
	// 0x1001728c — the node-transform array sized to the node count. With
	// the flag clear and the array already the right size it is left alone,
	// which is what makes the flag "rebuild even if it looks current"
	// (name ours).
	LTBOOL sub_1001728c(LTBOOL bFlag);
	// 0x100172e3 — the node array itself sized to the node count, cleared,
	// and then filled by the tree walking itself into it; the node count
	// divided by four (rounded up) is cached at +0x6c, and the tree's own
	// index is reset to all-ones afterwards. Same "already current" test as
	// its neighbour (name ours).
	LTBOOL sub_100172e3(LTBOOL bFlag);
	// 0x10017517 — every node's offset from its parent, recomputed: the
	// parent's transform inverted, times the node's own, and the
	// translation of the product kept. A node whose parent index is out of
	// range is a root and gets the origin. Three matrix-sized slots on the
	// stack, which is one named local and the two expression temporaries
	// `~a * b` needs (name ours).
	void   sub_10017517();
	// 0x1001c560 — the weight-set table off the wire: the old table given
	// back, a count read, one record allocated per entry and each read in
	// turn. Any failure gives the whole table back again (name ours).
	LTBOOL sub_1001c560(class ILTStream* pStream);
	// 0x10017660 and 0x1001761d — the two totals sub_100171f4 caches.
	// Declared only (names ours).
	uint32 sub_10017660();
	uint32 sub_1001761d(LTBOOL bFlag);

	char* GetFilename();                     // 0x10017b0e
	// 0x10017b12 — take a copy of the name the model was loaded from;
	// nonzero if the copy could be made (name ours).
	LTBOOL SetFilename(char* pFilename);
	// 0x10017b4a — give the copy back and go back to the shared sentinel
	// (name ours).
	void   FreeFilename();
	// 0x1001cbae — read the model out of a stream, answering an LTRESULT.
	// The allocator is an IMoAllocator (declared above): serverengine.cpp's
	// two call sites hand it the collection allocator bank.
	LTRESULT Load(class ILTStream* pStream, IMoAllocator* pAlloc);
	// 0x1001cb56 — the file's "Header" chunk: it opens the chunk by that
	// name, reads four bytes of version into m_nFileVersion, accepts only
	// 9, 10, 11 and 12, and then hands the stream to the load state. The
	// name is the chunk's, which is the only naming evidence there is.
	LTBOOL LoadHeader(class ILTStream* pStream, CModelLoadState* pState);

	// 0x1001c47b — the socket table off the wire.
	LTBOOL LoadSockets(class ILTStream* pStream);
	// 0x1001c604 — the whole model-data reader, and the child models are one
	// of the things it finds. Reconstructed in commonmodel.cpp; a near, one
	// instruction over 438 at the SIB base/index order, the commuting of two
	// address additions and the placement of the cold failure block. The name
	// is ours; `ret 4` and the 0x3c (LT_INVALIDPARAMS) answer for a request
	// with no stream are read off the binary.
	//
	// It walks four named chunks in order -- "Pieces", "Nodes", "ChildModels",
	// "Animation" -- through the same 0x1001bac6 chunk-finder LoadHeader uses,
	// sizing a collection off the wire before each. Its child-model half is
	// what named m_pChildModels/m_nChildModels below, and the walk over
	// `this + 0x34` at 0x1001c758 is what proved m_Pieces is a collection
	// rather than the flat m_pPieces/m_nPieces this header once named.
	LTRESULT LoadChildModels(class CModelLoadRequest* pRequest);

	// (declared below, inside this class)
	class CModelSub1D4;

	// LoadChildModels' remaining callees, named for their addresses and
	// declared from its call sites (names ours):
	//
	// 0x10017b8a — put one child-model record in a slot: the slot index, the
	// record, the model it stands for and the name the file gave it. The
	// model's own record goes in slot zero under the name "SELF".
	LTBOOL sub_10017b8a(uint32 iSlot, class CModelSub1D4* pRec,
	                    Model* pModel, char* pName);
	// 0x10017b69 — whatever a child record's +0x2c holds, derived from the
	// model at its +0x28. Reached only when that model is set.
	uint32 sub_10017b69(Model* pModel, uint32* pOut);
	// 0x1001c36c — the "AnimBindings" chunk (271 bytes, not reconstructed).
	LTBOOL sub_1001c36c(class CModelLoadRequest* pRequest,
	                    class ILTStream* pStream, int nFlag);
	// 0x100176dc — the property table (1001 bytes, not reconstructed).
	void   sub_100176dc();

	char*      m_pFilename;                  // 0x004
	// Things to run when this model goes; a child model puts the callback
	// that unhooks it from the manager's hash in here (name ours).
	uint8      m_Cleanups[0x10 - 0x008];     // 0x008 .. a list head at 0x014
	// 0x010 — the model's own address, stored by the constructor and read
	// by nothing else that is reconstructed (name ours).
	Model*     m_pSelf010;                   // 0x010
	// 0x014 — the first of the thirteen sub-objects, and NOT a collection.
	// Its constructor (0x100243f6) is two instructions of setup around one
	// call to 0x10024409, and that callee is the whole object: `andl $0,(%ecx)`
	// then `mov %eax,0x4(%ecx)`, with the argument the constructor passes as
	// zero, answering 1. Two dwords, and the first is zeroed rather than given
	// a vtable, so there is no vptr and nothing polymorphic here.
	//
	// That settles what was the one unresolved offset in this layout. Eight
	// bytes means it ends at 0x1c, so m_FileFlags and m_Flags below are real
	// fields rather than this object's array and count -- and the reading of
	// m_Flags as a flag word whose bit 0 means "referenced by an object"
	// stands. Had it been a 0x10 collection like its siblings, those two
	// would have had to be an element pointer and a count instead.
	//
	// The setter takes its second field by argument and returns LTBOOL, which
	// is an Init rather than a constructor body; the names here are ours.
	class CModelSub14
	{
	public:
		CModelSub14();                       // 0x100243f6
		// 0x10024404 — five bytes, a tail `jmp` into the member below.
		~CModelSub14();                      // 0x10024404
		// 0x10024419 — reached both from the destructor and from Model's
		// own release at +0x14, so it is a member and not the destructor
		// body (name ours).
		void   sub_10024419();               // 0x10024419
		LTBOOL Init(uint32 nValue);          // 0x10024409

		uint32 m_Unknown00;                  // 0x00  zeroed by Init
		uint32 m_Unknown04;                  // 0x04  Init's argument
	};

	CModelSub14 m_Sub014;                    // 0x014

	// The last three sub-objects are not collections, and their extents are
	// now read off their own constructors rather than guessed:
	//
	//   0x17c  CMoMemBlock, at most 0x14 bytes (Model has fields at 0x190 and
	//          0x1d0 before the next sub-object). Its constructor 0x1003d330
	//          stores a vptr (0x1004eb00) and sets up an SEH frame -- `push
	//          $-1; push handler; mov %fs:0x0` -- so it owns something whose
	//          construction can throw and needs an unwind funclet. That is a
	//          problem for reconstructing 0x1003d330 itself and NOT for
	//          Model::Model, which merely calls it: 0x10016cc5 opens with a
	//          plain frame and no SEH, and the call is a masked relocation, so
	//          CMoMemBlock only has to be DECLARED with a constructor here.
	//
	//   0x1d4  0x38 of span, of which 0x30 is confirmed touched: constructor
	//          0x100164bb calls a base at 0x10018102 (covering 0 to 0x14),
	//          then writes 0x14, 0x18, 0x24 and 0x28 as zero, 0x1c as the
	//          shared empty string, and 0x2c as 1. Model stores its own
	//          `this` into that object's 0x24 and 0x28 afterwards, which is
	//          what two of those zeroed fields are for. Whether the last
	//          eight bytes of the span belong to it or are unnamed Model
	//          fields is the one thing still open here.
	//
	//   0x20c  0xb4 bytes, ending at 0x2c0 exactly where Model stores a
	//          pointer back to it. Constructor 0x1001694e: a CMoArray member
	//          at +0x18 built first (0x1001831d, the NoCache constructor),
	//          then this object's own vptr (0x1004da24), then a body call
	//          (0x10016a00), then 0xac zeroed. Its destructor is 0x100169b8 --
	//          same vptr, and it tears down the +0x18 collection, which
	//          confirms the member.
	// Whatever the file manager knew about the file this came out of,
	// carried over verbatim when the model is loaded (name ours).
	uint32     m_FileFlags;                  // 0x01c
	uint32     m_Flags;                      // 0x020  bit 0 = referenced by an object
	CMoArray<class ModelNode*, NoCache> m_Nodes;           // 0x024
	CMoArray<ModelPiece*, NoCache>      m_Pieces;          // 0x034
	CMoArray<ModelWeightSet*, NoCache>  m_WeightSets;      // 0x044
	// 0x054 — a fifth collection (constructor 0x10018c92, the NoCache
	// CMoArray()). Its array and count at 0x058/0x05c are not named by
	// anything yet; the two dwords the constructor's body zeroes at 0x064 and
	// 0x06c are past its end.
	CMoArray<CModelVertRec, NoCache>    m_Unknown054;      // 0x054
	// 0x064, 0x068 and 0x06c — three counters the reader computes once the
	// model's data is in place: 0x064 and 0x068 from the two members at
	// 0x10017660 and 0x1001761d (0x100171f4), and 0x06c as the node count
	// divided by four, rounded up (0x100172e3). All three are zeroed by the
	// constructor and 0x064 again by the release (names ours).
	uint32     m_Unknown064;                 // 0x064
	uint32     m_Unknown068;                 // 0x068
	uint32     m_Unknown06c;                 // 0x06c
	CMoArray<LTMatrix, NoCache>         m_NodeTransforms;  // 0x070
	char*      m_pCommandString;             // 0x080  GetModelCommandString copies it
	CMoDataRef m_DataRef;                    // 0x084
	// 0x08c — the one DefaultCache collection of the thirteen (constructor
	// 0x10019104), so 0x14 bytes rather than 0x10: the policy carries a cache
	// size at +0x0c and a wanted-cache at +0x10 where NoCache carries one
	// padded byte.
	CMoArray<CModelFloatRec, DefaultCache> m_Unknown08c;   // 0x08c
	// 0x0a0 is the one field the constructor sets from its initialiser list
	// rather than its body: the `fldz`/`fstps` lands between the collection
	// constructor at 0x08c and the one at 0x154, which is where declaration
	// order puts it and nowhere the body could be scheduled to.
	float      m_f0a0;                       // 0x0a0  0.0f
	float      m_f0a4;                       // 0x0a4  96.0f
	float      m_f0a8;                       // 0x0a8  50.0f
	uint32     m_Unknown0ac;                 // 0x0ac  zeroed
	// Two lengths and each one's square, set together by the member below
	// and by nothing else that is reconstructed; the constructor's default
	// for both is 20000.0f. What they measure is not readable from either
	// side, so they are named for their offsets.
	float      m_f0b0;                       // 0x0b0
	float      m_f0b4;                       // 0x0b4  m_f0b0 * m_f0b0
	float      m_f0b8;                       // 0x0b8
	float      m_f0bc;                       // 0x0bc  m_f0b8 * m_f0b8
	// 0x154 and 0x164 — the last two collections of the thirteen. Their
	// interiors are what this header used to name flat: the one at 0x154 puts
	// its array on 0x158 and count on 0x15c, the one at 0x164 on 0x168 and
	// 0x16c. Renaming those four reached 27 call sites across six files --
	// and deliberately NOT the seven in spritecontrol.cpp and worldmodels.cpp,
	// where m_nAnims is a sprite's own field and m_Anims is a CWDArray04 on
	// another class entirely.
	uint32     m_Unknown0c0;                 // 0x0c0  zeroed
	// 0x0c4 .. 0x0e4 — five pairs of floats, each pair written as one
	// chained assignment (100, 200 and 50) or as a lone constant (0.25 and
	// 0.75). Named for their offsets: the constructor is the only
	// reconstructed writer and nothing reads them yet.
	float      m_f0c4;                       // 0x0c4  100.0f
	float      m_f0c8;                       // 0x0c8  100.0f
	float      m_f0cc;                       // 0x0cc  0.25f
	float      m_f0d0;                       // 0x0d0  0.75f
	uint32     m_Unknown0d4;                 // 0x0d4  zeroed
	float      m_f0d8;                       // 0x0d8  200.0f
	float      m_f0dc;                       // 0x0dc  200.0f
	float      m_f0e0;                       // 0x0e0  50.0f
	float      m_f0e4;                       // 0x0e4  50.0f
	LTVector   m_Vec0e8;                     // 0x0e8  Init()ed to the origin
	int32      m_n0f4;                       // 0x0f4  -1
	int32      m_n0f8;                       // 0x0f8  -1
	// 0x0fc — an LTMatrix, exactly 0x40 bytes: sub_100176dc copies it out of
	// m_NodeTransforms.m_pArray[m_n0f4] with one `rep movsl $0x10`
	// (0x10017a66). Named, not resized; nothing else reads this pad.
	LTMatrix   m_mNormalRef0fc;              // 0x0fc
	uint32     m_Unknown13c;                 // 0x13c  zeroed
	uint32     m_Unknown140;                 // 0x140  zeroed
	// 0x144 / 0x148 — FovX and FovY offsets in radians. sub_100176dc writes
	// both from atof through MATH_DEGREES_TO_RADIANS, whose constant at
	// 0x1004dc80 matches ltbasedefs.h's bit for bit. Two floats, same eight
	// bytes the pad covered.
	float      m_f144;                       // 0x144
	float      m_f148;                       // 0x148
	uint32     m_Unknown14c;                 // 0x14c  zeroed
	uint32     m_Unknown150;                 // 0x150  zeroed
	CMoArray<class ModelSocket*, NoCache> m_Sockets;    // 0x154
	CMoArray<AnimInfo, NoCache>           m_AnimArray;  // 0x164

	// Name lookups, each returning LTFALSE when the name is not in the model.
	// Three out-parameters' worth of one lookup: the index, the whole
	// table entry, and the animation itself as the return value. Both
	// pointers are optional and each call site passes a different pair,
	// which is why all three exist (the SDK has no declaration for this).
	// 0x1001767a — the total animation count over this model and its
	// children: it walks m_pChildModels, follows each record's owner back to
	// a Model and adds that model's animation-table size. The flag decides
	// whether entry ZERO counts, and entry zero is this model itself (the
	// constructor puts &m_Sub1d4 there), so a false flag means "the children
	// only". Records whose owner is null are skipped (name ours).
	uint32 GetTotalAnimCount(LTBOOL bIncludeSelf);
	// 0x100174b9 — FindAnim narrowed to one model: the same walk over
	// m_AnimArray, but an entry only qualifies when its animation's m_pModel
	// is the one asked for. It answers the ENTRY rather than a flag, and the
	// index goes out through the optional third argument (name ours).
	AnimInfo* FindAnimInModel(char* pName, class Model* pModel,
	                          uint32* pIndex);
	LTBOOL FindAnim(char* pName, uint32* pIndex, AnimInfo** ppInfo);// 0x10017452
	LTBOOL FindSocket(char* pName, HMODELSOCKET* pSocket);         // 0x10017bcd
	LTBOOL FindNode(char* pName, HMODELNODE* pNode);               // 0x1001740c
	LTBOOL FindPiece(char* pName, HMODELPIECE* pPiece);            // 0x1001737b
	LTBOOL FindWeightSet(char* pName, HMODELWEIGHTSET* pSet);      // 0x100173bc

	// 0x10017add — store two lengths and cache each one's square. Named for
	// its address: the four fields it writes are reached from nowhere else in
	// the reconstructed set, so nothing says what they are.
	void   sub_10017add(float f1, float f2);

	// 0x100173fa — take a name and answer the copy of it this model owns.
	// Both callers hand it the name they already hold and store the answer
	// straight back over it, so it re-homes a string onto the model rather
	// than duplicating it. Named for its address (name ours).
	char*  sub_100173fa(char* pName);

	// 0x1001c325 — read one name off a stream (a kilobyte of stack, filled
	// by ILTStream::ReadString) and hand back the copy this model owns. It
	// is a Model member rather than a free function because the copy comes
	// off the model's own store — every name in a model file is read
	// through it (name ours).
	LTBOOL sub_1001c325(class ILTStream* pStream, char** ppName);

	// 0x10017c17 — run one socket through the walker and hand back its 4x4
	// (name ours).
	LTBOOL EvalSocket(class ModelWalker* pWalker, HMODELSOCKET hSocket, LTMatrix* pOut);

public:
	// Load points this at the block below once the block owns memory, and
	// records the allocator beside it (names ours).
	// 0x174 — the data block, and the bytes want it reached through an
	// accessor rather than read as a member: every call that hands it to a
	// collection materializes it in a REGISTER before pushing it, where a
	// plain member read pushes the memory operand instead (0x1001bf0d,
	// 0x1001bb37). An inlined member function is what puts the value in a
	// register — the same fingerprint seen for inlined
	// accessors materializing `this`.
	class LAlloc* GetAlloc() { return (class LAlloc*)m_pDataBlock; }

	CMoMemBlock* m_pDataBlock;               // 0x174
	IMoAllocator* m_pAlloc;                  // 0x178
	CMoMemBlock m_DataBlock;                 // 0x17c .. 0x190
	// 0x1d4 — the twelfth sub-object, and its size is pinned rather than
	// bounded: 0x1d4 + 0x30 lands exactly on m_nFileVersion, so it is 0x30
	// bytes, not the 0x38 the gap to the next sub-object suggested. Its
	// constructor (0x100164bb) runs a base at 0x10018102 over the first 0x14,
	// then zeroes 0x14, 0x18, 0x24 and 0x28, points 0x1c at the shared empty
	// string and puts 1 at 0x2c. Model stores its own `this` into that
	// object's 0x24 and 0x28 afterwards (names ours).
	class CModelSub1D4
	{
	public:
		CModelSub1D4();                      // 0x100164bb
		// 0x100164e3 — a body call at 0x10016501, then the 0x14-byte
		// sub-object at offset zero torn down inline (its vtable restored
		// and Term()'d through 0x100197e6). Declared here for Model's own
		// destructor, which calls it at +0x1d4.
		~CModelSub1D4();                     // 0x100164e3
		// 0x10016501 — the reset the destructor's body is: the owner
		// pointer at +0x28 cleared, the name put back on the shared empty
		// string, and the collection at offset zero handed back to the
		// owning model's data block. Named for its address; nothing says
		// what it is beyond that (the destructor is its only caller here).
		void   sub_10016501();               // 0x10016501

		// 0x00 — a 0x14-byte collection, not opaque padding: the reset at
		// 0x10016501 and the destructor at 0x100164e3 both Term() it
		// through `this` itself (`mov %esi,%ecx`, i.e. offset zero), and
		// the SetSize2 they reach (0x100197e6) is the DefaultCache body —
		// it reads +0x0c and +0x10 as the cache words — over four-byte
		// elements (its release is 0x1000d020, the `PAXVDefaultCache`
		// _DeleteAndDestroyArray). Its constructor is 0x10018102.
		// The elements are 0x1c bytes, not four: the reader at 0x1001bf0d
		// walks this array with a stride of 0x1c, filling each record with
		// one w_ReadVector and one w_ReadRotation, and the SetSize2 it
		// shares with the destructor (0x100197e6) reaches BaseNew at
		// 0x1001b71a, whose `imul $0x1c` is the size in the clear. One
		// record per node of the model, and the count comes from the
		// model's own node-transform array.
		CMoArray<CChildNodeXform, DefaultCache> m_Array00; // 0x00 .. 0x14
		uint32 m_Unknown14;                  // 0x14
		uint32 m_Unknown18;                  // 0x18
		char*  m_pName;                      // 0x1c  the shared ""
		// 0x20 — one dword off the wire, read by 0x1001bf0d before the
		// transforms (name ours).
		uint32 m_Unknown20;                  // 0x20
		Model* m_pOwner24;                   // 0x24  Model's `this`
		// 0x28 — the model this record stands for, which is NOT always the
		// model that holds it: 0x10017145 reads it off every child record,
		// decrements that model's cache count, and deletes the model when
		// the count reaches zero. For the record embedded at Model+0x1d4 it
		// is the holder itself, which is why that case is tested for and
		// skipped (name ours).
		Model* m_pOwner28;                   // 0x28
		uint32 m_Unknown2c;                  // 0x2c  set to 1

		// 0x1001bf0d — this record's half of the model file: one dword and
		// then one position-and-rotation per node of the owning model. A
		// record with no owner cannot be sized and answers LTFALSE
		// (name ours).
		LTBOOL sub_1001bf0d(class ILTStream* pStream);
	};

	// 0x190 .. 0x1d0 — sixteen child-model slots and, at 0x1d0, how many of
	// them are filled. Not reconstructed here, but read straight off
	// LoadChildModels (0x1001c604) and confirmed from four directions:
	//
	//   * it refuses a "ChildModels" count of zero or more than 0x10
	//     (`cmpl $0x10,-0x10(%ebp); ja` onto the failure path), and 0x190
	//     plus sixteen dwords lands exactly on 0x1d0;
	//   * it stores each new child with `mov %edi,0x190(%ebx,%eax,4)` and
	//     then `incl 0x1d0(%ebx)`;
	//   * a later loop walks `lea 0x190(%ebx)` forward four bytes at a time
	//     against `cmp 0x1d0(%ebx),%eax`;
	//   * and the constructor's `m_pChildModels[0] = &m_Sub1d4` /
	//     `m_nChildModels = 1` below is the model's own entry — the same
	//     one LoadChildModels reaches as `lea 0x1d4(%ebx),%edi` beside the
	//     literal "SELF". So CModelSub1D4 is a child-model record and the
	//     one embedded at 0x1d4 is the model itself (names ours).
	class CModelSub1D4* m_pChildModels[16];  // 0x190 .. 0x1d0
	uint32     m_nChildModels;               // 0x1d0
	CModelSub1D4 m_Sub1d4;                   // 0x1d4 .. 0x204
	uint32     m_nFileVersion;               // 0x204  9, 10, 11 or 12
	int        m_CacheCount;                 // 0x208  FreeUnusedModels' scratch counter
	// The class runs to 0x2c4, which is what both places that build one
	// allocate; the rest is not reached from the reconstructed set.
	// 0x20c — the thirteenth, 0xb4 bytes, ending at 0x2c0 where Model stores
	// a pointer back to it. Its constructor (0x1001694e) builds a collection
	// member at +0x18 first, then its own vptr, then a body call, then zeroes
	// 0xac; its destructor (0x100169b8) stores the same vptr and tears that
	// member down, which confirms the member from the other end (names ours).
	class CModelSub20C
	{
	public:
		// 0x1001bd54 — the "Nodes" chunk: the node tree out of the file
		// (name ours).
		LTBOOL sub_1001bd54(class ILTStream* pStream);

		CModelSub20C();                      // 0x1001694e
		// 0x10016990 — the same object built against a model: identical to
		// the default constructor except that the model goes into +0xac
		// instead of a zero. It is what slot 0x08 news up.
		CModelSub20C(Model* pModel);         // 0x10016990
		virtual ~CModelSub20C();             // slot 0x00, 0x100169b8
		// 0x100169dd — the destructor's whole body, and the model's own
		// release (0x10017016) calls it too, which is what says it is a
		// member rather than the destructor written inline: it hands the
		// collection at +0x18 back to the model's data block (the model is
		// at +0xac and its block at +0x174) and then runs the reset below
		// (names ours).
		void   sub_100169dd();               // 0x100169dd
		// 0x10016a00 — the reset the constructor ends with and the release
		// runs again: three floats, two counters and a name pointer back to
		// their defaults, plus two sub-records at +0x2c and +0x6c cleared
		// through 0x10011e2b. Declared only — its fields are not named yet
		// (name ours).
		void   sub_10016a00();               // 0x10016a00
		// Slot 0x04 is `ret $0xc` — three dwords of arguments and no body,
		// which is all that fixes its shape. Slot 0x08 is a factory: it
		// asks the engine allocator for 0xb4 bytes — this class's own size,
		// which is the independent confirmation of the extent — and runs
		// the model-taking constructor on it. Slot 0x0c is the one Model's
		// constructor calls (names ours).
		virtual void _v04(uint32 a, uint32 b, uint32 c);   // 0x10016971
		virtual CModelSub20C* Create(Model* pModel);       // 0x10016a37
		virtual void SetModel(Model* pModel);              // 0x10016a58

		// 0x10016a9b — how many nodes this subtree is: itself plus every
		// child's answer, recursively. Forty bytes, and the one that says
		// the array at +0x18 is the child list (name ours).
		uint32 sub_10016a9b();
		// 0x10016ac3 — walk the subtree into the owning model's node array,
		// taking the next index from the caller's counter. Declared only
		// (name ours).
		LTBOOL sub_10016ac3(uint32* pnIndex);
		// 0x10016b11 — set this subtree's own index, recursively.
		// Declared only (name ours).
		void   sub_10016b11(uint32 nIndex);

		// 0x10016304 — do two node trees have the same SHAPE? A free
		// function rather than a member: it compares child counts level by
		// level and recurses pairwise, and the first node whose count
		// disagrees goes out through the third argument. Deeper mismatches
		// are reported by the recursion that found them, which is why this
		// level only stores on its own test (name ours).
		friend LTBOOL mod_SameNodeTree(CModelSub20C* pA, CModelSub20C* pB,
		                               CModelSub20C** ppMismatch);

		// 0x04 .. 0x12 — what the reset at 0x10016a00 writes: three floats
		// zeroed through `fldz`/`fstps`, a word and a byte cleared with
		// `andw`/`andb` (which is what fixes their widths), and the name
		// pointer at 0xb0 put on the shared empty string. Named for their
		// offsets; nothing reads them in the reconstructed set.
		float  m_f04;                        // 0x04
		float  m_f08;                        // 0x08
		float  m_f0c;                        // 0x0c
		uint16 m_n10;                        // 0x10
		uint8  m_b12;                        // 0x12
		uint8  m_pad_13[0x18 - 0x13];
		// The elements are CModelSub20C* and the walk in SetModel says so:
		// it calls slot 0x0c on each of them, with the same model, which is
		// this class's own SetModel and no other (name ours).
		CMoArray<CModelSub20C*, NoCache> m_Array18;   // 0x18
		uint8  m_pad_28[0x2c - 0x28];
		// 0x2c and 0x6c — two 4x4s, and the pair is what closes this class:
		// the reset runs LTMatrix::Identity (0x10011e2b, the same call
		// GetNodeTransform makes) over +0x2c and then over +0x6c, and
		// 0x6c + 0x40 lands exactly on the model back-pointer below.
		LTMatrix m_Mat2c;                    // 0x2c
		LTMatrix m_Mat6c;                    // 0x6c
		// 0xac — the model this belongs to, and not merely a pointer: the
		// release at 0x100169dd loads it and then reads +0x174 off it,
		// which is Model::m_pDataBlock. SetModel (slot 0x0c) is what fills
		// it; Model's constructor calls that with `this`.
		Model* m_pModel;                     // 0xac
		char*  m_pName;                      // 0xb0  the shared empty string
	};

	CModelSub20C m_Sub20c;                   // 0x20c .. 0x2c0
	class CModelSub20C* m_pSub20c;           // 0x2c0
};

// A model object carries its main tracker inline.
class ModelInstance : public LTObject
{
public:
	// (LTObject runs to 0x1b4 now that m_pWMIface is named)
	ModelInstanceHookFn m_HookFn;            // 0x1b4
	uint8               m_pad_1b8[0x1c0 - 0x1b8];
	LTAnimTracker       m_AnimTracker;       // 0x1c0
	LTAnimTracker*      m_pTrackerList;      // 0x1f8  extra trackers, singly linked
	uint8               m_pad_1fc[0x204 - 0x1fc];
	uint32              m_HiddenPieces;      // 0x204  one bit per piece
};

// The engine's own ILTModel vtable carries one slot the public SDK header
// lost (header drift, the third instance after ILTSoundMgr and
// _CVector::DistSqr): every virtual sits one slot later than iltmodel.h
// says. Virtual calls dispatch through this view (names per the SDK).
class ILTModelI
{
public:
	virtual void _v00() = 0;
	virtual LTRESULT GetSocket(HOBJECT hObj, char* pSocketName, HMODELSOCKET& hSocket) = 0;
	virtual LTRESULT GetNode(HOBJECT hObj, char* pNodeName, HMODELNODE& hNode) = 0;
	virtual LTRESULT GetPiece(HOBJECT hObj, char* pPieceName, HMODELPIECE& hPiece) = 0;
	virtual LTRESULT FindWeightSet(HOBJECT hObj, char* pSetName, HMODELWEIGHTSET& hSet) = 0;
	virtual LTRESULT GetPieceHideStatus(HOBJECT hObj, HMODELPIECE hPiece, LTBOOL& bHidden) = 0;
	virtual LTRESULT SetPieceHideStatus(HOBJECT hObj, HMODELPIECE hPiece, LTBOOL bHidden) = 0;
	virtual LTRESULT GetNodeTransform(HOBJECT hObj, HMODELNODE hNode,
	                                  LTransform& transform, LTBOOL bWorldSpace) = 0;
	virtual LTRESULT GetSocketTransform(HOBJECT hObj, HMODELSOCKET hSocket,
	                                    LTransform& transform, LTBOOL bWorldSpace) = 0;
};

// The scratch object the node evaluator walks a model's skeleton in: 0x248
// bytes on the caller's stack, set up from the object through LTObject's
// vtable and then handed the object's own transform. Names ours; only the
// three fields the two transform accessors reach are known.
class ModelWalker
{
public:
	// 0x100386c6, 49 bytes. Declared here because sub_100176dc calls it;
	// it is NOT Evaluate.
	LTBOOL sub_100386c6();                                  // 0x100386c6
	ModelWalker();                           // 0x100175b6

	// 0x100386f7 — evaluate up to and including one node, filling the
	// model's node-transform array.
	LTBOOL Evaluate(HMODELNODE hNode);

	// The array also makes VC6 emit an unreferenced vector-constructor
	// iterator (`??_H@YGXPAXIHP6EX0@Z@Z`, 34 bytes) into the object, the
	// same COMDAT `new T[n]` leaves behind. The shipped constructor
	// inlines the loop and never calls it, so the shipped link dropped it;
	// it shows up in our object's unmatched list and is not a function to
	// reconstruct.
	//
	// The constructor's loop is what says this: eight records of 0x1c
	// bytes, each given -1 in its last dword and then cleared by a call —
	// which is AnimTimeRef's own constructor, inlined, over a member array
	// of eight. (The called body at 0x100175fd is the out-of-line copy of
	// AnimTimeRef::Clear the header's inline definition leaves behind.)
	AnimTimeRef m_TimeRefs[8];               // 0x000 .. 0x0e0
	uint32     m_Unknown_e0;                // 0x0e0  cleared last, after the
	                                         //        five below, so it is a
	                                         //        later addition to the
	                                         //        constructor's body
	LTMatrix*  m_pObjTransform;              // 0x0e4  where the model sits
	uint32     m_Unknown_e8;                 // 0x0e8
	uint32     m_Unknown_ec;                 // 0x0ec  set by the setup call:
	                                         //        nonzero = the transforms
	                                         //        come back world-relative
	uint32     m_Unknown_f0;                 // 0x0f0
	uint32     m_Unknown_f4;                 // 0x0f4
	uint8      m_pad_0f8[0x248 - 0x0f8];
};

// CommonModelLT's own vtable is not the one m_pModelLT points at. Reached
// through the interface pointer, GetNodeTransform is slot 0x1c — that is what
// SharedCommonLT::GetAttachmentTransform calls and byte-matches on. Reached
// through `this` from inside the implementation, the same function is slot
// 0x18, one earlier, which is the second-base shape: the interface the engine
// hands out is a separate sub-object. Only the one slot is needed here.
class ILTModelSelf
{
public:
	virtual void _v00()=0; virtual void _v04()=0; virtual void _v08()=0;
	virtual void _v0c()=0; virtual void _v10()=0; virtual void _v14()=0;
	virtual LTRESULT GetNodeTransform(HOBJECT hObj, HMODELNODE hNode,
	                                  LTransform& transform, LTBOOL bWorldSpace)=0;  // 0x18
};

// 0x10011b1e is the SDK's own MatMul from ltmatrix.h, emitted out of line —
// the same thing that happens to Identity, Inverse and Normalize: the header
// marks them inline, VC6 declines, and the COMDAT it leaves behind is the
// shipped function. So there is no mat_Multiply to declare here; call
// MatMul. (sharedcommon.h and servermath.h still declare a mat_Multiply,
// and their call sites byte-match either way because the call's operand is
// a masked relocation.)

// 0x10015bcd — the other direction: a quaternion laid out as the rotation
// half of a 4x4 (declared in sharedcommon.h too; both files reach it, and
// servermessage.cpp includes both headers, so the two declarations have to
// stay spelled the same). The era prototype in ltquatbase.h is
// `(const float* pQuat, float mat[4][4])`; the two pointers a call pushes
// are the same either way, so the call sites here byte-match as written.
void quat_ConvertToMatrix(LTRotation* pRot, LTMatrix* pMat);

// 0x1000d040 and 0x10026394 — a vector and a rotation off a stream, one
// component at a time. Declared here because the model reader uses them and
// they are defined further down commonmodel.cpp (names ours).
void w_ReadVector(class ILTStream* pStream, LTVector* pVec);
void w_ReadRotation(class ILTStream* pStream, LTRotation* pRot);

// 0x1001bac6 — seek the stream to a named chunk, answering LTFALSE when the
// file has no such chunk. Declared here for the same reason as the two above:
// the model readers use it and it is defined further down commonmodel.cpp.
LTBOOL mod_FindChunk(class ILTStream* pStream, char* pName);

// 0x10015ce2 — the rotation half of a 4x4, back out as a quaternion. Not a
// reconstruction at all: this is ltquatbase.cpp's own quat_ConvertFromMatrix,
// which the SDK ships as source, and the prototype is the SDK's.
void quat_ConvertFromMatrix(float* pQuat, const float mat[4][4]);

// Tracker helpers from the model module (0x100010a7, 0x100011c2, 0x10001153).
LTBOOL at_IsDone(LTAnimTracker* pTracker);
void   at_ResetAnim(LTAnimTracker* pTracker);
void   at_SetCurAnim(LTAnimTracker* pTracker, HMODELANIM hAnim, LTBOOL bTransition);
void   at_Init(LTAnimTracker* pTracker, Model* pModel, uint32 iAnim);   // 0x10001013

// 0x100136c2 — resolve an animation by name on the object's model (names
// ours; GetAnimIndex forwards to it, commonmodel.cpp defines it).
HMODELANIM ci_GetAnimIndex(LTObject* pObj, char* pAnimName);

// 0x10052edc — a char* holding the address of a zero byte at 0x10056ff4: the
// name a model with no file of its own carries, so that every reader can
// strlen it and FreeFilename can tell an allocated name from the shared one
// (name ours).
extern char* g_pEmptyFilename;

// 0x10058724 — the running total of the bytes every model's data block has
// taken. Load adds each block's rounded size to it and nothing here takes
// any back (name ours).
extern uint32 g_nModelDataBytes;

// Give every element of a pointer collection back to an allocator and then
// empty the collection. Sixty bytes, one instantiation per element type, and
// the image carries five of them — 0x1001a1a1, 0x1001a1dd, 0x1001a215,
// 0x1001b5c9 and 0x1001b605 — identical but for which BaseDelete they reach.
// (They all reach the SAME SetSize2 at 0x1001a03d, which is the observation
// that says the shipped link folds identical COMDATs: a pointer collection's
// SetSize2 does not depend on what the pointer points at.)
//
// LIKE THE BaseNew OVERLOADS ABOVE, THIS BELONGS IN collection.h with the
// rest of the collection's helpers; it is here only because that file is not
// ours to edit. It must NOT be marked `inline`: /O1 implies /Ob1, so an
// inline-marked template would be expanded into its callers and the five
// shipped functions would not exist (name ours).
template<class T>
void FreeArrayElements(CMoArray<T, NoCache>* pArray, LAlloc* pAlloc)
{
	uint32 i;
	T p;

	for (i = 0; i < pArray->m_nElements; i++)
	{
		p = pArray->m_pArray[i];

		if (p)
			BaseDelete(pAlloc, p, 1);
	}

	pArray->SetSize2(0, pAlloc);
}


// 0x1001690b — the animation's length in milliseconds.


// 0x1000108d — advance a tracker by a whole number of milliseconds; the
// interface takes seconds and this is where they stop being seconds
// (name ours).
void at_UpdateTracker(LTAnimTracker* pTracker, uint32 nMS);


// The vtable at 0x1004e5f4 is iltmodel.h's twenty-seven virtuals in the
// header's own declaration order — anchored by the twenty already matched,
// which fixes every slot without a guess. The order matters: UpdateMainTracker
// reaches GetMainTracker at 0x28 through `this`, which is index ten and
// nothing else.
class CommonModelLT
{
public:
	virtual LTRESULT GetSocket(HOBJECT hObj, char* pSocketName, HMODELSOCKET& hSocket);
	virtual LTRESULT GetNode(HOBJECT hObj, char* pNodeName, HMODELNODE& hNode);
	virtual LTRESULT GetPiece(HOBJECT hObj, char* pPieceName, HMODELPIECE& hPiece);
	virtual LTRESULT FindWeightSet(HOBJECT hObj, char* pSetName, HMODELWEIGHTSET& hSet);

	virtual LTRESULT GetPieceHideStatus(HOBJECT hObj, HMODELPIECE hPiece, LTBOOL& bHidden);
	virtual LTRESULT SetPieceHideStatus(HOBJECT hObj, HMODELPIECE hPiece, LTBOOL bHidden);

	virtual LTRESULT GetNodeTransform(HOBJECT hObj, HMODELNODE hNode,
	                                  LTransform& transform, LTBOOL bWorldSpace);  // 0x1001d478
	virtual LTRESULT GetSocketTransform(HOBJECT hObj, HMODELSOCKET hSocket,
	                                    LTransform& transform, LTBOOL bWorldSpace);// 0x1001d77f

	virtual LTRESULT GetInstanceHookFn(HOBJECT hObj, ModelInstanceHookFn& fn);
	virtual LTRESULT SetInstanceHookFn(HOBJECT hObj, ModelInstanceHookFn fn);

	virtual LTRESULT GetMainTracker(HOBJECT hObj, LTAnimTracker*& pTracker);
	virtual LTRESULT UpdateMainTracker(HOBJECT hObj, float fUpdateDelta);          // 0x1001e056
	virtual LTRESULT GetPlaybackState(LTAnimTracker* pTracker, uint32& flags);

	virtual LTRESULT AddTracker(HOBJECT hObj, LTAnimTracker* pTracker);
	virtual LTRESULT RemoveTracker(HOBJECT hObj, LTAnimTracker* pTracker);

	virtual LTRESULT GetCurAnim(LTAnimTracker* pTracker, HMODELANIM& hAnim);
	virtual LTRESULT SetCurAnim(LTAnimTracker* pTracker, HMODELANIM hAnim);
	virtual LTRESULT ResetAnim(LTAnimTracker* pTracker);

	virtual LTRESULT GetLooping(LTAnimTracker* pTracker);
	virtual LTRESULT SetLooping(LTAnimTracker* pTracker, LTBOOL bLooping);
	virtual LTRESULT GetPlaying(LTAnimTracker* pTracker);
	virtual LTRESULT SetPlaying(LTAnimTracker* pTracker, LTBOOL bPlaying);

	virtual LTRESULT GetWeightSet(LTAnimTracker* pTracker, HMODELWEIGHTSET& hSet);
	virtual LTRESULT SetWeightSet(LTAnimTracker* pTracker, HMODELWEIGHTSET hSet);

	virtual LTRESULT GetCurAnimLength(LTAnimTracker* pTracker, uint32& length);
	virtual LTRESULT GetCurAnimTime(LTAnimTracker* pTracker, uint32& curTime);
	virtual LTRESULT SetCurAnimTime(LTAnimTracker* pTracker, uint32 curTime);
};

#endif
