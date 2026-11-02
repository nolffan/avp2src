// *********************************************************************** //
//
//  MODULE:   W9_444000 (.CPP)  -- staging translation unit (name ours)
//
//  WHAT THIS RANGE IS
//  ------------------
//  Every row of the 0x004458f0..0x004479d0 range belongs to ONE module:
//  the engine's MODEL half -- the same translation unit src/engine/server/
//  commonmodel.cpp already reconstructs from server.dll.  The identification
//  did not need the string pool at all (there are only two literals in the
//  whole range and both belong to 0x004471b0, which is ltnew2.cpp's):
//
//   1. THE DISPATCH TABLE.  A vtable scan turns up a 228-pointer run at
//      0x004a2698 made of 12-slot vtables whose first two slots are always
//      0x004126d0 / 0x00412590.  0x004126d0 writes a zero into the caller's
//      8-byte out-parameter and copies an UNINITIALISED second dword out of
//      its own frame; 0x00412590 answers `*pPos < this->+0x08`.  That is
//      stdlith's `GenList<T>` -- GenBegin / GenIsValid / GenGetNext /
//      GenGetAt / GenAppend / GenRemoveAt / GenRemoveAll / GenGetSize /
//      GenCopyList / GenAppendList / GenFindElement / GenSetCacheSize, in
//      src/LT2/lithshared/stdlith/genlist.h's declaration order -- i.e. the
//      whole run is CMoArray<T,C> instantiations, and the classes that carry
//      them sit between the tables.
//
//   2. THE CALL GRAPH.  Every caller of a row here from outside the range is
//      in 0x0044c740..0x0044f551, which our sources already name:
//      Model::LoadChildModels (0x0044ddc0, modchunk.cpp),
//      CommonModelLT::GetNodeTransform (0x0044ee10) and
//      ::GetSocketTransform (0x0044f3d0).
//
//   3. THE /O1 ORACLE CLOSED IT.  Every row but one has a server.dll twin
//      inside commonmodel.cpp's own address range, and the twins are named
//      in src/engine/server/commonmodel.h already:
//
//        lithtech    server      what
//        0x004458f0  0x1001668a  AnimNode::Flatten          <- row
//        0x00445960  0x100166de  AnimNode::Bind   (commonmodel.cpp has it)
//        0x004459b0  0x1001671c  AnimNode::GetModel
//        0x004457e0  0x10016723  CModelXform::operator=
//        0x004459c0  0x1001674d  ModelAnim::ModelAnim(Model*)
//        0x00445a20  0x10016798  ModelAnim scalar-deleting dtor
//        0x00445a40  0x100167b4  ModelAnim::~ModelAnim
//        0x00445a90  0x100167e1  ModelAnim::sub_100167e1    <- row (WRITTEN)
//        0x00445b00  0x1001682b  ModelAnim::Init(Model*)
//        0x00445b30  0x10016856  ModelAnim::Create
//        0x00445b60  0x10016874  ModelAnim::sub_10016874
//        0x00445ba0  0x10016897  thunk into the next one
//        0x00445bb0  0x100168a3  ModelAnim::sub_100168a3    <- row (commonmodel.cpp has it)
//        0x00445c30  0x1001690b  ModelAnim::GetLength
//        0x00445c80  0x1001694e  CModelSub20C::CModelSub20C()
//        0x00445ce0  0x10016990  CModelSub20C::CModelSub20C(Model*)
//        0x00445d10  0x100169b8  CModelSub20C::~CModelSub20C
//        0x00445d60  0x100169dd  CModelSub20C::sub_100169dd
//        0x00445dd0  0x10016a00  CModelSub20C::sub_10016a00
//        0x00445e80  0x10016a37  CModelSub20C::Create
//        0x00445eb0  0x10016a58  CModelSub20C::SetModel
//        0x00445f00  0x10016a9b  CModelSub20C::sub_10016a9b
//        0x00445f30  0x10016ac3  CModelSub20C::sub_10016ac3 <- row
//        0x00445f90  0x10016b11  CModelSub20C::sub_10016b11 <- row
//        0x00445fd0  0x10016b3e  ModelPieceData::ctor(Model*)<- row
//        0x00446030  0x10016b5f  ModelPieceData::ctor()
//        0x00446090  0x10016b7c  ModelPieceData::SetModel
//        0x004460a0  0x10016b86  ModelPieceData::~
//        0x00446140  0x10016bdb  ModelPiece::ModelPiece(Model*) <- row
//        0x00446180  0x10016c14  ModelPiece::~ModelPiece    <- row
//        0x004461e0  0x10016c3a  ModelPiece::sub_10016c3a   <- row
//        0x00446240  0x10016c4e  ModelWeightSet::ctor(Model*)<- row
//        0x004462c0  0x10016c69  ModelWeightSet::~          <- row
//        0x00446330  0x10016c95  ModelSocket::ModelSocket
//        0x00446360  0x10016cc5  Model::Model
//        0x00446660  0x10016f26  Model::~Model
//        0x004467c0  0x10017016  Model::sub_10017016        <- row (WRITTEN)
//        0x00446970  0x100170f2  Model::sub_100170f2
//        0x00446a00  0x10017145  Model::sub_10017145        <- row
//        0x00446ad0  0x100171f4  Model::sub_100171f4
//        0x00446b80  0x1001728c  Model::sub_1001728c        <- row (WRITTEN)
//        0x00446c30  0x100172e3  Model::sub_100172e3        <- row (WRITTEN)
//        0x00446d10  0x1001737b  Model::FindPiece
//        0x00446d60  0x100173bc  Model::FindWeightSet
//        0x00446db0  0x100173fa  Model::sub_100173fa
//        0x00446dd0  0x1001740c  Model::FindNode
//        0x00446e30  0x10017452  Model::FindAnim
//        0x00446eb0  0x100174b9  Model::FindAnimInModel
//        0x00446f20  0x10017517  Model::sub_10017517        <- row
//        0x00446fc0  0x10011d32  Mat_InverseTransformation
//        0x00447080  (none)      AnimTimeRef::Init          <- row
//        0x004470c0  0x1001761d  Model::sub_1001761d        <- row
//        0x00447110  0x10017660  Model::sub_10017660
//        0x00447130  0x1001767a  Model::GetTotalAnimCount
//        0x00447180  0x100176b7  Model::sub_100176b7
//        0x004471b0  0x100176dc  Model::sub_100176dc  (ltnew2.cpp has it)
//        0x00447800  0x10017b8a  Model::sub_10017b8a        <- row
//        0x00447860  0x10017bcd  Model::FindSocket
//        0x004478c0  0x10017c17  Model::EvalSocket          <- row (commonmodel.cpp has it)
//        0x00447980  0x10018124  CMoArray<..>::GenGetNext   <- row (WRITTEN)
//        0x004479d0  0x10018148  CMoArray<..>::GenGetAt     <- row (WRITTEN)
//
//  EXTENT DEFECTS FOUND (details at the foot of the file)
//    * 0x00447904 / 115 / call IS NOT A FUNCTION.  It starts mid-instruction
//      (the bytes 00 00 00 are the tail of `movl $0x0,0xe8(%ecx)` at
//      0x004478fd) and has no caller.  0x004478c0 + 0x00447904 are ONE
//      183-byte function, Model::EvalSocket.
//    * SIX ROWS ARE ALREADY WRITTEN and are /O2-shape work, not
//      reconstruction: 0x00445a90, 0x004467c0, 0x00446b80, 0x00446c30 are
//      commonmodel.cpp nears; 0x00447980 and 0x004479d0 are collection.cpp
//      nears.  They are NOT reproduced here.  What each one needs is at the
//      foot of the file.
//    * Every other row's last instruction ends exactly on its boundary and
//      no branch target leaves it (23 rows).  All 21
//      `call`-provenance starts are on instruction boundaries except the one
//      above.  Neither `ptr` row is an ASCII accident: 0x00447980 reads back
//      as "\x80\x79\x44\x00" and 0x004479d0 as "\xd0\x79\x44\x00", neither
//      of which is printable text, and both are real vtable slots.
//
//  WHERE THIS BELONGS.  Everything below is a member of a class that
//  src/engine/server/commonmodel.h already declares (Model, ModelPiece,
//  ModelWeightSet, ModelSocket, Model::CModelSub20C) or should
//  (ModelAnim's node, the piece's data base).  The bodies are spelled here
//  against Q444 stand-ins so that this file can be compiled on its own;
//  moving them into commonmodel.cpp against the real classes is free
//  (the byte comparison masks relocations and the engine never links), and is worth
//  doing because the SAME source then also lands in server.dll at /O1,
//  where every one of these has a twin.
//
//  Build: src/engine/lithtech, cl 8447 /O2 (/Og /Oi /Ot /Oy /Ob1), the
//  default for that directory.  `/I ..\server` is already on the command
//  line, which is where collection.h comes from.
//
// *********************************************************************** //

#include <string.h>
#include <stdlib.h>
// ltbasetypes.h rather than ltbasedefs.h: collection.h already pulls it in,
// and it is all that LTVector / LTRotation / LTMatrix / LTBOOL need.  When
// these bodies move into commonmodel.cpp the #ifndef below is a no-op --
// commonmodel.h reaches ltbasedefs.h through ltanimtracker.h.
#include "ltbasetypes.h"
#include "collection.h"
#include "commonmodel.h"

#ifndef LTNULL
#define LTNULL 0
#endif


// ======================================================================= //
//  The classes.  Every offset below is read off a shipped instruction and
//  is cited by the address it was read at.
// ======================================================================= //

class ModelQ444;
class ModelAnimQ444;
class CModelSub20CQ444;
class ModelPieceQ444;


// 0x1c bytes: one position and one rotation per keyframe.  This is
// commonmodel.h's CModelXform / collection.cpp's CCollElem1CS.
//
// THE COPY CONSTRUCTOR IS THE POINT, and it is what the two `ptr` rows of
// this range turn out to need.  0x00447980 / 0x004479d0 are
// CMoArray<this>::GenGetNext / ::GenGetAt, and both are template bodies
// collection.cpp ALREADY emits -- as 44-byte and 40-byte functions whose
// element copy is one `rep movsl` of seven dwords, against the shipped
// 79 and 76 whose copy is MEMBERWISE: three dword move pairs through one
// register pair for the twelve-byte head (the /O2 lowering of /O1's three
// `movsl`) and then four through another for the sixteen-byte sub-object
// (CCollInner10::operator= expanded).  A `rep movsl` is what VC6 emits for
// an IMPLICIT copy constructor; the memberwise pair is what it emits for a
// user-declared one written with assignments.  server.dll says the same
// thing from the other side: its 0x10018124 / 0x10018148 CALL the copy
// (0x100195b8) where ours copies in line.
//
// So the element needs a copy constructor DEFINED IN THE CLASS BODY -- the
// only spelling /Ob1 will ever expand -- whose body ASSIGNS its two
// members rather than initialising them (an initialiser list would
// copy-construct both members bitwise and give the `rep movsl` back).
// /Os declines the expansion, so server.dll keeps its call and cannot
// regress.  The same three lines belong on collection.cpp's CCollElem1CS,
// where they fix all four functions at once; they are here rather than in
// collection.cpp.
class CAnimXformHeadQ444
{
public:
	uint32 m_a, m_b, m_c;
};

class CAnimXformSubQ444
{
public:
	CAnimXformSubQ444& operator=(const CAnimXformSubQ444& other)
	{
		m_a = other.m_a;
		m_b = other.m_b;
		m_c = other.m_c;
		m_d = other.m_d;

		return *this;
	}

	uint32 m_a, m_b, m_c, m_d;
};

class CAnimXformQ444
{
public:
	CAnimXformQ444() {}

	CAnimXformQ444(const CAnimXformQ444& other)
	{
		m_head = other.m_head;
		m_sub = other.m_sub;
	}

	CAnimXformQ444& operator=(const CAnimXformQ444& other)
	{
		m_head = other.m_head;
		m_sub = other.m_sub;

		return *this;
	}

	CAnimXformHeadQ444 m_head;
	CAnimXformSubQ444  m_sub;
};

// 0x14 bytes -- commonmodel.h's AnimKeyFrame, again only for the size.
class CAnimKeyQ444
{
public:
	uint32 m_n[5];
};

// The two 0x20-byte element types the piece's data record carries. Their
// sizes come from their own SetSize2 bodies, which multiply the count by
// 0x20 (`shl $0x5` at 0x0044ba31 and 0x0044bbb1). They are two DIFFERENT
// types because the two collections have two different vtables (0x004a27e4
// at 0x00445fdd and 0x004a27b4 at 0x00445fff) -- and they are collection.cpp's
// CCollElem20F and CCollElem20A (collelem20.h), not two types of this
// file's own. The vtables say so: 0x004a27e4's GenAppend (0x00448bd0) calls
// the constructor-running BaseNew 0x0044c970 (CCollElem20F's constructor is
// 0x00445410), and 0x004a27b4's (0x00449030) carries CCollElem20A's
// three-trip word/record assignment expanded. Two opaque 0x20 PODs here
// compiled a second CMoArray instance per virtual that the shipped image
// does not have (rep-movs copies, no constructor), and placed none of them.
#include "collelem20.h"


// ----------------------------------------------------------------------- //
// The animation's node.  One per model node, embedded in the animation at
// +0x28 (0x004459c9 `lea 0x28(%esi),%edi`) and carrying a subtree of its
// own.  Offsets from its constructor at 0x00445580 and from the four
// members below.
// ----------------------------------------------------------------------- //
class CAnimNodeQ444
{
public:
	// vtable 0x004a26c8, four slots: 0x004455f0 (scalar deleting dtor),
	// 0x004455e0 (`mov 0x2c(%ecx),%eax; ret` -- the owning animation),
	// 0x004458b0 and 0x004458c0.
	virtual ~CAnimNodeQ444();
	virtual ModelAnimQ444* GetAnimQ444();
	virtual void           SetAnimQ444(ModelAnimQ444* pAnim);
	virtual void           _v0cQ444();

	// 0x004459b0, seven bytes: `mov 0x2c(%ecx),%eax; mov 0x20(%eax),%eax;
	// ret`.  DECLARED HERE AND DEFINED OUT OF CLASS -- /Ob1 never expands a
	// non-template member declared out of class, which is what keeps the
	// call at 0x00445901 in the shipped /O2 body.
	ModelQ444* GetModelQ444();

	// 0x004458f0 -- walk this subtree into the animation's flat node array,
	// taking the next slot from the caller's counter.
	LTBOOL sub_1001668aQ444(uint32* pnIndex);

	// 0x004 -- the model node this animation node is bound to; filled by
	// Bind (0x00445968 `mov %ebx,0x4(%edi)`, reconstructed in ltnew1.cpp).
	CModelSub20CQ444* m_pModelNode;                     // 0x004
	// 0x008 -- one transform per keyframe (vtable 0x004a2708 stored at
	// 0x00445592, `lea 0x8(%edi),%esi` at 0x0044558c).
	CMoArray<CModelXform, NoCache> m_Xforms;            // 0x008 .. 0x018
	uint32 m_Unknown018;                                // 0x018
	// 0x01c -- the child list (vtable 0x004a26d8 stored at 0x004455b4,
	// `lea 0x1c(%edi),%esi` at 0x004455a9).  Its m_pArray lands on 0x020
	// and its m_nElements on 0x024, which is exactly what 0x004458f0 and
	// Bind read.
	CMoArray<CAnimNodeQ444*, NoCache> m_Children;       // 0x01c .. 0x02c
	// 0x02c -- the animation this node belongs to, which vtable slot 1
	// (0x004455e0) hands back.
	ModelAnimQ444* m_pAnim;                             // 0x02c
};


// ----------------------------------------------------------------------- //
// One animation.  0x5c bytes (its factory pushes 0x5c at 0x00445b30) and
// every offset below is written by the constructor at 0x004459c0.
// ----------------------------------------------------------------------- //
class ModelAnimQ444
{
public:
	// vtable 0x004a2738, three slots: 0x00445a20, 0x00445b00, 0x00445b30.
	virtual ~ModelAnimQ444();
	virtual void InitQ444(ModelQ444* pModel);
	virtual ModelAnimQ444* CreateQ444(ModelQ444* pModel);

	// 0x004 -- one pointer per model node (0x004458f5, 0x00445bbb).
	CAnimNodeQ444** m_pNodes;                           // 0x004
	CMoArray<CAnimKeyQ444, NoCache> m_KeyFrames;        // 0x008 .. 0x018
	int32   m_Unknown018;                               // 0x018  ctor: -1
	uint32  m_nInterpolationTime;                       // 0x01c  ctor: 0xc8
	ModelQ444* m_pModel;                                // 0x020
	char*   m_pName;                                    // 0x024
	CAnimNodeQ444 m_Node;                               // 0x028 .. 0x058
	CAnimNodeQ444* m_pRootNode;                         // 0x058
};


// ----------------------------------------------------------------------- //
// One model node.  0xb4 bytes; this is commonmodel.h's Model::CModelSub20C
// and ModelNode, which are the same class.
// ----------------------------------------------------------------------- //
class CModelSub20CQ444
{
public:
	// vtable 0x004a2774, four slots: 0x00445cc0, 0x004132e0, 0x00445e80,
	// 0x00445eb0.
	virtual ~CModelSub20CQ444();
	virtual void _v04Q444(uint32 a, uint32 b, uint32 c);
	virtual CModelSub20CQ444* CreateQ444(ModelQ444* pModel);
	virtual void SetModelQ444(ModelQ444* pModel);

	// 0x00445f30 -- this subtree into the model's node array.
	LTBOOL sub_10016ac3Q444(uint32* pnIndex);
	// 0x00445f90 -- this subtree's parent index, recursively.
	void   sub_10016b11Q444(uint32 iParent);

	float    m_fRelX;                                   // 0x004
	float    m_fRelY;                                   // 0x008
	float    m_fRelZ;                                   // 0x00c
	uint16   m_iSelf;                                   // 0x010
	uint8    m_b012;                                    // 0x012
	uint8    m_pad_013[0x018 - 0x013];
	CMoArray<CModelSub20CQ444*, NoCache> m_Children;    // 0x018 .. 0x028
	uint32   m_iParent;                                 // 0x028
	LTMatrix m_Mat2c;                                   // 0x02c
	LTMatrix m_Mat6c;                                   // 0x06c
	ModelQ444* m_pModel;                                // 0x0ac
	char*    m_pName;                                   // 0x0b0
};


// ----------------------------------------------------------------------- //
// The 0x24-byte vertex-data record a piece opens with and carries a list
// of.  Two collections and the model they belong to; sizeof is pinned by
// the 0x24 stride of the destroy loop at 0x004461ad and by 0x20 + 4.
// ----------------------------------------------------------------------- //
class ModelPieceDataQ444
{
public:
	// 0x00446030 -- the no-argument form, which BaseNew<ModelPieceData>
	// default-constructs every element of the piece's own list with.
	// DECLARED ONLY: it is not one of this range's rows (lt2new1.cpp already carries
	// it as CLtElem24A, stride 0x24, ctor 0x00446030).
	ModelPieceDataQ444();
	// 0x00445fd0 -- build one against a model.
	ModelPieceDataQ444(ModelQ444* pModel);
	// 0x004460a0.  Declared here and DEFINED IN commonmodel.cpp (not this
	// file: it is not one of this range's rows).
	~ModelPieceDataQ444();
	// 0x00446090, `mov 0x4(%esp),%eax; mov %eax,0x20(%ecx); ret $4`.
	// Out of class so /Ob1 cannot expand it -- the shipped constructor
	// CALLS it (0x0044601d).
	void SetModelQ444(ModelQ444* pModel);

	CMoArray<CCollElem20F, NoCache>      m_Array00;    // 0x000 .. 0x010
	CMoArray<CCollElem20A, NoCache>      m_Array10;    // 0x010 .. 0x020
	ModelQ444* m_pModel20;                              // 0x020
};


// ----------------------------------------------------------------------- //
// One piece.  0x6c bytes (commonmodel.h; the release strides by 0x6c).
// ----------------------------------------------------------------------- //
class ModelPieceQ444 : public ModelPieceDataQ444
{
public:
	ModelPieceQ444(ModelQ444* pModel);                  // 0x00446140
	~ModelPieceQ444();                                  // 0x00446180
	void sub_10016c3aQ444();                            // 0x004461e0

	// IN CLASS, and that is load-bearing: both builds are /Ob1, and this
	// body is EXPANDED into 0x004470c0 (and into its server twin
	// 0x1001761d) rather than called.  Index zero is the piece's own
	// embedded record, which is why the `i == 0` arm hands back `this`
	// with no arithmetic at all (0x004470da `test %edi,%edi; je` straight
	// onto the null test, with the piece pointer already in the register).
	ModelPieceDataQ444* GetPieceDataQ444(uint32 i)
	{
		if (i == 0)
			return this;

		// 0x004470dc / 0x004470e2: the bound is the list's own count and
		// the subscript is i - 1, so slot 0 is the head and slot n the
		// (n-1)th of the list.  Decremented IN PLACE: both images compute
		// i - 1 once (0x10017638 `lea -1`) and scale that register for the
		// subscript; `i - 1` written twice folded the -1 into the address.
		i--;
		if (i < m_Array34.m_nElements)
			return &m_Array34[i];

		return LTNULL;
	}

	uint32   m_Unknown024;                              // 0x024
	uint32   m_nOffset;                                 // 0x028
	float    m_f02c;                                    // 0x02c
	float    m_f030;                                    // 0x030
	CMoArray<ModelPieceDataQ444, NoCache> m_Array34;    // 0x034 .. 0x044
	float    m_f044;                                    // 0x044
	char     m_Name[0x068 - 0x048];                     // 0x048
	ModelQ444* m_pModel;                                // 0x068
};


// ----------------------------------------------------------------------- //
// One weight set.  0x28 bytes (its own allocator bakes it in).
// ----------------------------------------------------------------------- //
class ModelWeightSetQ444
{
public:
	ModelWeightSetQ444(ModelQ444* pModel);              // 0x00446240
	~ModelWeightSetQ444();                              // 0x004462c0

	char   m_Name[0x10];                                // 0x000
	CMoArray<uint32, DefaultCache> m_Weights;           // 0x010 .. 0x024
	ModelQ444* m_pModel;                                // 0x024
};


// ----------------------------------------------------------------------- //
// One socket.  0x34 bytes; the layout is commonmodel.h's, confirmed here by
// 0x004478ef (m_iNode at 0x2c) and 0x00447918 (the rotation at 0x0c).
// ----------------------------------------------------------------------- //
class ModelSocketQ444
{
public:
	LTVector   m_vPos;                                  // 0x000
	LTRotation m_rRot;                                  // 0x00c
	char       m_Name[0x2c - 0x1c];                     // 0x01c
	uint32     m_iNode;                                 // 0x02c
	uint32     m_Unknown030;                            // 0x030
};


// ----------------------------------------------------------------------- //
// One child-model record.  0x30 bytes (commonmodel.h: 0x1d4 + 0x30 lands on
// m_nFileVersion).  Only the four fields 0x00447800 writes are named.
// ----------------------------------------------------------------------- //
class CModelSub1D4Q444
{
public:
	~CModelSub1D4Q444();                                // 0x004454c0
	void sub_10016501Q444();                            // 0x00445510

	uint8    m_pad_000[0x018];
	uint32   m_Unknown018;                              // 0x018
	char*    m_pName;                                   // 0x01c
	uint32   m_Unknown020;                              // 0x020
	ModelQ444* m_pOwner24;                              // 0x024
	ModelQ444* m_pOwner28;                              // 0x028
	uint32   m_Unknown02c;                              // 0x02c
};


// The scratch the node evaluator walks a skeleton in.  Only the two things
// 0x004478c0 touches are named.
class ModelWalkerQ444
{
public:
	// 0x00489b00, thiscall, one argument (0x0044790b).
	LTBOOL EvaluateQ444(uint32 hNode);

	uint8  m_pad_000[0x0e8];
	uint32 m_Unknown0e8;                                // 0x0e8
	uint8  m_pad_0ec[0x248 - 0x0ec];
};


// ----------------------------------------------------------------------- //
// The model's string store at +0x84 (commonmodel.h's CMoDataRef): a head
// link and the allocator.  Only the lookup 0x00446db0 forwards to is named.
// ----------------------------------------------------------------------- //
class CMoDataRefQ444
{
public:
	// 0x00445280 (server 0x1001638c), thiscall, one argument.
	char* sub_1001638cQ444(char* pName);

	void*        m_pLinks;                              // 0x00
	class LAlloc* m_pAlloc;                             // 0x04
};


// ----------------------------------------------------------------------- //
// The model.
// ----------------------------------------------------------------------- //
class ModelQ444
{
public:
	virtual ~ModelQ444();

	// commonmodel.h's accessor: the data block reached through a function
	// rather than as a member, because every call that hands it to a
	// collection materialises it in a REGISTER before pushing it
	// (0x00445a9f, 0x0044626a, 0x004462ca).
	class LAlloc* GetAllocQ444() { return (class LAlloc*)m_pDataBlock; }

	void   sub_10017517Q444();                          // 0x00446f20
	char*  sub_100173faQ444(char* pName);               // 0x00446db0
	uint32 sub_1001761dQ444(uint32 iLOD);               // 0x004470c0
	uint32 sub_10017660Q444();                          // 0x00447110
	LTBOOL sub_10017b8aQ444(uint32 iSlot, CModelSub1D4Q444* pRec,
	                        ModelQ444* pModel, char* pName);  // 0x00447800

	uint8  m_pad_004[0x024 - 0x004];
	CMoArray<CModelSub20CQ444*, NoCache>  m_Nodes;          // 0x024
	CMoArray<ModelPieceQ444*, NoCache>    m_Pieces;         // 0x034
	CMoArray<ModelWeightSetQ444*, NoCache> m_WeightSets;    // 0x044
	uint8  m_pad_054[0x070 - 0x054];
	CMoArray<LTMatrix, NoCache>           m_NodeTransforms; // 0x070
	uint8  m_pad_080[0x084 - 0x080];
	CMoDataRefQ444 m_Names;                                 // 0x084 (0x00446db4)
	uint8  m_pad_08c[0x154 - 0x08c];
	CMoArray<ModelSocketQ444*, NoCache>   m_Sockets;        // 0x154
	uint8  m_pad_164[0x174 - 0x164];
	void*  m_pDataBlock;                                    // 0x174
	uint8  m_pad_178[0x190 - 0x178];
	CModelSub1D4Q444* m_pChildModels[16];                   // 0x190
	uint32 m_nChildModels;                                  // 0x1d0
	uint8  m_Sub1d4[0x204 - 0x1d4];                         // 0x1d4
	uint32 m_nFileVersion;                                  // 0x204
	int32  m_CacheCount;                                    // 0x208
	uint8  m_pad_20c[0x2c0 - 0x20c];
	CModelSub20CQ444* m_pSub20c;                            // 0x2c0
};


// 0x00444880 -- a quaternion laid out as the rotation half of a 4x4
// (commonmodel.h's quat_ConvertToMatrix; the server twin is 0x10015bcd).
void quat_ConvertToMatrixQ444(LTRotation* pRot, LTMatrix* pMat);


// ======================================================================= //
//  0x004458f0, 101 bytes -- CAnimNode::Flatten
//  server.dll twin 0x1001668a (82 bytes).  thiscall, one argument, LTBOOL.
//
//  WHAT PINS THE SHAPE.  Three conditional jumps -- 0x004458fd, 0x00445911
//  and 0x00445938 -- all go FORWARD to 0x0044594d, which sits PAST the
//  success return at 0x00445942.  A block past every
//  other return is a trailing statement and the guards above it are
//  positive.  So the two entry tests nest positively and the shared
//  `return LTFALSE` is written last; the loop's failure is a second
//  `return LTFALSE` that VC6 folds onto the trailing one (of two
//  identical sites the SECOND stays inline and the first jumps into it).
//
//  The node count is reached through the out-of-line GetModel at 0x004459b0
//  (`call` at 0x00445901) but m_pAnim->m_pNodes is read as a member twice
//  (0x004458f5 and 0x00445913) -- written out at each use, not cached.
// ======================================================================= //

ModelQ444* CAnimNodeQ444::GetModelQ444()
{
	return m_pAnim->m_pModel;
}

LTBOOL CAnimNodeQ444::sub_1001668aQ444(uint32* pnIndex)
{
	uint32 i;

	if (m_pAnim->m_pNodes)
	{
		// 0x0044590a: the bound is the model's node-transform count.
		if (*pnIndex < GetModelQ444()->m_NodeTransforms.m_nElements)
		{
			// 0x0044591b: `mov %esi,(%eax,%ecx,4)`.
			m_pAnim->m_pNodes[*pnIndex] = this;
			++(*pnIndex);

			// 0x00445923 reads the child count, 0x0044592a the array;
			// both are re-read at the bottom of every trip because the
			// recursive call could have changed them.
			for (i = 0; i < m_Children.m_nElements; i++)
			{
				if (!m_Children[i]->sub_1001668aQ444(pnIndex))
					return LTFALSE;
			}

			return LTTRUE;
		}
	}

	return LTFALSE;
}


// 0x00445bb0 -- ModelAnim::sub_100168a3 -- is written in commonmodel.cpp as
// CUnknownD9E4::sub_100168a3, identical on both images.


// ======================================================================= //
//  0x00445f30, 94 bytes -- CModelSub20C::sub_10016ac3
//  server.dll twin 0x10016ac3 (78 bytes).  thiscall, one argument, LTBOOL.
//
//  An EARLY refusal, not a trailing LTFALSE: the bound test is `jb` over
//  an in-line `return LTFALSE` on both images (0x00445f44, 0x10016ad7 --
//  /O1 moves the xor out of line past the epilogue at 0x10016b0d).  The
//  positive-guard spelling gave `jae` and cost both images.
//  m_pModel is loaded ONCE (0x00445f39) and used again at 0x00445f59 across
//  an intervening word store -- VC6's own CSE, not a source local.
//  `*pnIndex` is read three times (0x00445f4e as a WORD, 0x00445f57,
//  0x00445f5f), which is what says it is written out at each use.
// ======================================================================= //

LTBOOL CModelSub20CQ444::sub_10016ac3Q444(uint32* pnIndex)
{
	uint32 i;

	if (*pnIndex >= m_pModel->m_Nodes.m_nElements)
		return LTFALSE;

	// 0x00445f53: a 16-bit store, so the field is a uint16.
	m_iSelf = (uint16)(*pnIndex);
	m_pModel->m_Nodes[*pnIndex] = this;
	++(*pnIndex);

	for (i = 0; i < m_Children.m_nElements; i++)
	{
		if (!m_Children[i]->sub_10016ac3Q444(pnIndex))
			return LTFALSE;
	}

	return LTTRUE;
}


// ======================================================================= //
//  0x00445f90, 51 bytes -- CModelSub20C::sub_10016b11
//  server.dll twin 0x10016b11 (43 bytes).  thiscall, one uint32, void.
//
//  The recursion's argument is this node's own 16-bit index widened
//  (0x00445fa7 `xor %ecx,%ecx` + 0x00445fa9 `mov 0x10(%esi),%cx`, where
//  /O1 spells the same thing `movzwl`), so the parameter is a uint32 and
//  the member a uint16.
//
//  The bound is GetSize(), not a bare m_nElements: with the field read
//  directly /O1 scheduled the loop's first compare above the m_iParent
//  store (and `push %edi` above `mov %ecx,%esi`); the accessor gives the
//  shipped order at 0x10016b1b.  lithtech is identical either way.
// ======================================================================= //

void CModelSub20CQ444::sub_10016b11Q444(uint32 iParent)
{
	uint32 i;

	m_iParent = iParent;

	for (i = 0; i < m_Children.GetSize(); i++)
		m_Children[i]->sub_10016b11Q444(m_iSelf);
}


// ======================================================================= //
//  0x00445fd0, 90 bytes -- ModelPieceData::ModelPieceData(Model*)
//  server.dll twin 0x10016b3e (31 bytes), where the two collection
//  constructors are CALLS (0x10018351, 0x10018546).  At /O2 both are
//  expanded: vptr, Clear()'s two zero stores, the ONE surviving SetSize2
//  call (Init's Term()), and _InitArray's two zero stores.  Init's own
//  trailing SetSize(0) folds to nothing because Clear/_InitArray have
//  already proved m_pArray and m_nElements zero and the size is a literal.
//
//  0x00446090 is CALLED rather than expanded (0x0044601d), which is why
//  SetModel is declared in class and defined out of it.
// ======================================================================= //

void ModelPieceDataQ444::SetModelQ444(ModelQ444* pModel)
{
	m_pModel20 = pModel;
}

ModelPieceDataQ444::ModelPieceDataQ444(ModelQ444* pModel)
{
	SetModelQ444(pModel);
}


// ======================================================================= //
//  0x00446140, 63 bytes -- ModelPiece::ModelPiece(Model*)
//  server.dll twin 0x10016bdb (57 bytes).
//
//  The base constructor is a CALL in both images (0x00446184 / 0x10016be4)
//  because it is defined out of line above.  The m_Array34 constructor is
//  expanded down to vptr + two zero stores at /O2 (0x0044614c..0x0044615a)
//  and is a call at /O1 (0x10016bec) -- the /Ot-versus-/Os split, not a
//  source difference.
//
//  STATEMENT ORDER is taken from /O1, which schedules least: 0x10016bf1
//  m_Name[0], 0x10016bf5 m_Unknown024, then `fld1` hoisted above
//  0x10016bfb m_pModel and 0x10016bfe m_f02c, then the two 1.0f stores.
//  (Coin flip: /O2 sinks the 5.0f store to last, so `m_f02c = 5.0f` could
//  equally be written after the two 1.0f assignments.  /O1 is the tie
//  breaker and this is the spelling that reproduces it exactly.)
// ======================================================================= //

ModelPieceQ444::ModelPieceQ444(ModelQ444* pModel)
	: ModelPieceDataQ444(pModel)
{
	m_Name[0] = 0;
	m_Unknown024 = 0;
	m_pModel = pModel;
	m_f02c = 5.0f;                       // 0x00446171, 0x40a00000
	m_f030 = 1.0f;                       // 0x0044616b
	m_f044 = 1.0f;                       // 0x0044616e
}


// ======================================================================= //
//  0x004461e0, 88 bytes -- ModelPiece::sub_10016c3a
//  server.dll twin 0x10016c3a (20 bytes): `m_Array34.Term(alloc)` and
//  nothing else, with Term's SetSize2 a call (0x10019b76).  At /O2 the
//  whole of SetSize2(0, pAlloc) is expanded, including BaseDelete's
//  destroy loop (0x00446206, stride 0x24) and its allocator release.
// ======================================================================= //

void ModelPieceQ444::sub_10016c3aQ444()
{
	m_Array34.Term(m_pModel->GetAllocQ444());
}


// ======================================================================= //
//  0x00446180, 91 bytes -- ModelPiece::~ModelPiece
//  server.dll twin 0x10016c14 (38 bytes).
//
//  The body is one call (0x00446184).  Everything after it is the
//  compiler's: the m_Array34 member destructor -- its vtable restored at
//  0x0044618c and Term() run with the DEFAULT allocator, expanded -- and
//  then the base destructor (0x004461d3), in that order.
// ======================================================================= //

ModelPieceQ444::~ModelPieceQ444()
{
	sub_10016c3aQ444();
}


// ======================================================================= //
//  0x00446240, 123 bytes -- ModelWeightSet::ModelWeightSet(Model*)
//  server.dll twin 0x10016c4e (30 bytes), where the collection constructor
//  is a call (0x100189cf).  At /O2 the whole CMoArray<uint32,DefaultCache>
//  constructor is expanded, and BOTH of Init's SetSize2 calls survive as
//  expanded bodies: the first releases with GetNumAllocatedElements()
//  (0x00446260..0x00446266, m_nElements + m_CacheSize -- the DefaultCache
//  policy in the clear) and the second with m_nElements alone, because
//  _InitArray has already put a literal zero in the cache word.
// ======================================================================= //

ModelWeightSetQ444::ModelWeightSetQ444(ModelQ444* pModel)
{
	m_Name[0] = 0;                        // 0x004462b2
	m_pModel = pModel;                    // 0x004462af
}


// ======================================================================= //
//  0x004462c0, 107 bytes -- ModelWeightSet::~ModelWeightSet
//  server.dll twin 0x10016c69 (44 bytes): Term against the model's own
//  allocator, then the member destructor's vptr store and Term against the
//  default one.  Both Terms are expanded at /O2.
// ======================================================================= //

ModelWeightSetQ444::~ModelWeightSetQ444()
{
	m_Weights.Term(m_pModel->GetAllocQ444());
}


// ======================================================================= //
//  0x00446a00, 207 bytes -- Model::sub_10017145: commonmodel.cpp's, which
//  now carries this file's spelling and matches both images.
// ======================================================================= //


// ======================================================================= //
//  0x00446db0, 19 bytes -- Model::sub_100173fa
//  server.dll twin 0x100173fa (18 bytes).  thiscall, one argument.
//
//  Re-home a name onto the model: a straight forward into the string
//  store at +0x84 (0x00446db4), whose lookup is 0x00445280 / 0x1001638c.
// ======================================================================= //

char* ModelQ444::sub_100173faQ444(char* pName)
{
	return m_Names.sub_1001638cQ444(pName);
}


// ======================================================================= //
//  0x00446f20, 157 bytes -- Model::sub_10017517
//  server.dll twin 0x10017517 (159 bytes).  thiscall, no arguments, void.
//
//  Every node's offset from its parent: the parent's transform inverted,
//  times the node's own, and the translation of the product kept.  A node
//  whose parent index is out of range is a root and gets the origin.
//
//  THREE MATRIX-SIZED SLOTS on the stack, which is one named local and the
//  two temporaries `~a * b` needs -- the inverse at [esp+0x54]/[ebp-0x88],
//  the copy of it the multiply binds as `this` at [esp+0x9c]/[ebp-0xc8]
//  (the `rep movs` of 0x10 dwords at 0x00446f70), and the product at
//  [esp+0x1c]/[ebp-0x48] whose 0x0c/0x1c/0x2c dwords are the translation.
//  Mat_InverseTransformation (0x00446fc0) and MatMul (0x00434580) are both
//  CALLED: ltmatrix.h marks them inline and VC6 declines, exactly as it
//  does for Identity and Normalize.
//
//  The node count is re-read at the head of every trip (0x00446faa) rather
//  than hoisted, because the two calls could have changed it; the SAME
//  read is the bound the parent index is tested against (0x00446f44).
// ======================================================================= //

void ModelQ444::sub_10017517Q444()
{
	uint32 i;
	LTMatrix m;

	for (i = 0; i < m_NodeTransforms.m_nElements; i++)
	{
		CModelSub20CQ444* pNode = m_Nodes[i];

		if (pNode->m_iParent < m_NodeTransforms.m_nElements)
		{
			m = ~m_Nodes[pNode->m_iParent]->m_Mat2c * pNode->m_Mat2c;

			pNode->m_fRelX = m.m[0][3];      // 0x00446f94
			pNode->m_fRelY = m.m[1][3];      // 0x00446f97
			pNode->m_fRelZ = m.m[2][3];      // 0x00446f9a
		}
		else
		{
			pNode->m_fRelX = 0.0f;           // 0x00446fa1
			pNode->m_fRelY = 0.0f;
			pNode->m_fRelZ = 0.0f;
		}
	}
}


// ======================================================================= //
//  0x00447080, 60 bytes -- AnimTimeRef::Init
//
//  NOT A RECONSTRUCTION.  This is the SDK's own in-class
//  AnimTimeRef::Init from src/LT2/sdk/inc/ltanimtracker.h, emitted out of line
//  as a COMDAT because at least one caller declined to expand it -- the
//  three call sites are 0x0044ee3a (CommonModelLT::GetNodeTransform, 1182
//  bytes), 0x0044f3fc (GetSocketTransform, 1121) and 0x0046c465, and
//  VC6's inlining budget is per function and spent
//  from the end backwards.
//
//  The shipped body confirms the header field for field: `ret $0x18` is
//  six arguments (a pointer, four uint32 and a float); the two 16-bit
//  stores at +0x04/+0x06 and +0x0c/+0x0e are FrameLocator::m_iAnim and
//  ::m_iFrame narrowed by the header's own (uint16) casts; the two dword
//  zeroes at +0x08 and +0x10 are the two m_Time members; and +0x14 is
//  m_Percent, copied as a dword because it is a pure assignment.
//  m_iWeightSet (+0x18) is NOT touched, which is the header's behaviour.
//
//  DECLARED IN CLASS AND DEFINED OUT OF IT here so that the COMDAT is
//  emitted from this translation unit whatever the callers do.  In the
//  real tree it should stay exactly where it is, in ltanimtracker.h.
// ======================================================================= //

class FrameLocatorQ444
{
public:
	uint16 m_iAnim;                                     // 0x00
	uint16 m_iFrame;                                    // 0x02
	uint32 m_Time;                                      // 0x04
};

class AnimTimeRefQ444
{
public:
	void Init(ModelQ444* pModel,
	          uint32 iAnim1, uint32 iFrame1,
	          uint32 iAnim2, uint32 iFrame2,
	          float percent);

	ModelQ444*       m_pModel;                          // 0x00
	FrameLocatorQ444 m_Prev;                            // 0x04
	FrameLocatorQ444 m_Cur;                             // 0x0c
	float            m_Percent;                         // 0x14
	uint32           m_iWeightSet;                      // 0x18
};

void AnimTimeRefQ444::Init(ModelQ444* pModel,
                           uint32 iAnim1, uint32 iFrame1,
                           uint32 iAnim2, uint32 iFrame2,
                           float percent)
{
	m_pModel = pModel;                                  // 0x00447089

	m_Prev.m_iAnim = (uint16)iAnim1;                    // 0x00447090
	m_Prev.m_iFrame = (uint16)iFrame1;                  // 0x00447099
	m_Prev.m_Time = 0;                                  // 0x0044709f

	m_Cur.m_iAnim = (uint16)iAnim2;                     // 0x004470a2
	m_Cur.m_iFrame = (uint16)iFrame2;                   // 0x004470b2
	m_Cur.m_Time = 0;                                   // 0x004470ab

	m_Percent = percent;                                // 0x004470b6
}


// ======================================================================= //
//  0x004470c0, 67 bytes -- Model::sub_1001761d
//  server.dll twin 0x1001761d (67 bytes).  thiscall, one uint32, uint32.
//
//  One of the two totals sub_100171f4 caches at +0x64/+0x68; its sibling
//  0x00447110 (server 0x10017660) is the same walk with the level fixed at
//  zero.  The per-piece record selector is expanded from
//  ModelPiece::GetPieceDataQ444 above.
//
//  The piece count is read ONCE into the loop counter (0x004470c0) and the
//  loop counts DOWN with the array pointer walking by four -- VC6's own
//  strength reduction of `for (i = 0; i < n; i++) ... m_pArray[i]`, not a
//  source shape; nothing in the body can change the count.
// ======================================================================= //

uint32 ModelQ444::sub_1001761dQ444(uint32 iLOD)
{
	uint32 i, total;
	ModelPieceDataQ444* pData;

	total = 0;

	for (i = 0; i < m_Pieces.m_nElements; i++)
	{
		pData = m_Pieces[i]->GetPieceDataQ444(iLOD);

		if (pData)
			total += pData->m_Array10.m_nElements;       // 0x004470f3, +0x18
	}

	return total;
}


// ======================================================================= //
//  0x00447110, 30 bytes -- Model::sub_10017660
//  server.dll twin 0x10017660 (26 bytes).  thiscall, no arguments, uint32.
//
//  The other total sub_100171f4 caches: every piece's own embedded record's
//  first list count (+0x08, 0x00447123 / 0x10017672), summed.
// ======================================================================= //

uint32 ModelQ444::sub_10017660Q444()
{
	uint32 i, total;

	total = 0;

	for (i = 0; i < m_Pieces.m_nElements; i++)
		total += m_Pieces[i]->m_Array00.m_nElements;

	return total;
}


// ======================================================================= //
//  0x00447800, 83 bytes -- Model::sub_10017b8a
//  server.dll twin 0x10017b8a (67 bytes).  thiscall, four arguments,
//  LTBOOL.
//
//  Put one child-model record in a slot.  The literal 0x10 at 0x00447805
//  is the table's own length, and 0x190 + sixteen dwords lands exactly on
//  m_nChildModels at 0x1d0.
//
//  An early refusal (`jb` over `xor; ret` at 0x00447808 / 0x10017b92).
//  The zero store is written LAST: /O2 keeps it last (0x00447827) and /O1
//  hoists it to first on its own (0x10017ba0); written first, /O2 put it
//  first too.
// ======================================================================= //

LTBOOL ModelQ444::sub_10017b8aQ444(uint32 iSlot, CModelSub1D4Q444* pRec,
                                   ModelQ444* pModel, char* pName)
{
	if (iSlot >= 16)
		return LTFALSE;

	pRec->m_pName = pName;
	pRec->m_pOwner24 = this;
	pRec->m_pOwner28 = pModel;
	pRec->m_Unknown018 = 0;

	// 0x0044782e / 0x00447830: the model's own record does not count
	// itself as a reference.
	if (pModel && pModel != this)
		pModel->m_CacheCount++;

	m_pChildModels[iSlot] = pRec;

	return LTTRUE;
}


// 0x004478c0 -- Model::EvalSocket -- is commonmodel.cpp's own
// Model::EvalSocket, identical on both images.


// ======================================================================= //
//  0x00447980, 79 bytes  --  CMoArray<CAnimXform, C>::GenGetNext
//  0x004479d0, 76 bytes  --  CMoArray<CAnimXform, C>::GenGetAt
//  server.dll twins 0x10018124 (36) and 0x10018148 (29).
//
//  NOTHING IS WRITTEN FOR THESE TWO.  They are collection.h's own template
//  bodies, and collection.cpp already instantiates them (as
//  CMoArray<CCollElem1CS, DefaultCache/NoCache>); what was wrong was the
//  ELEMENT, not the array -- see the note on CAnimXformQ444 above.  The
//  explicit instantiation below emits them from this translation unit with
//  the corrected element so that the fix can be measured without touching
//  collection.cpp; the permanent home of the three added lines is
//  CCollElem1CS.
//
//  Both addresses appear TWICE in .rdata -- slots 2 and 3 of the vtable at
//  0x004a2698 (DefaultCache) and of the one at 0x004a2708 (NoCache) -- so
//  one body serves two instantiations and only one of them can be credited.
//
//  UPDATE: the element is commonmodel.h's CModelXform itself (void,
//  in-class operator= whose out-of-line copy is 0x10016723; empty default
//  constructor; in-class assigning copy constructor; SDK LTVector and
//  LTRotation members), so m_Xforms above uses it and the
//  CAnimXformQ444 stand-in is no longer referenced. The NoCache array is
//  CModelXform's and the DefaultCache one is CChildNodeXform's; the two
//  GenGetNext/GenGetAt bodies fold together at link time.
// ======================================================================= //

template class CMoArray<CModelXform, NoCache>;


// ======================================================================= //
//
//  WHAT THIS FILE DOES NOT CONTAIN, AND WHY
//
//  ---- 0x00447904 / 115 / call -- NOT A FUNCTION -------------------------
//  The row starts mid-instruction.  0x004478fd is
//  `movl $0x0,0xe8(%ecx)` (c7 81 e8 00 00 00 00 00 00 00) and the row's
//  first three bytes are the top of that immediate; the `e8` the
//  enumerator read as a call is the 0xe8 of the +0xe8 displacement.  It has
//  no caller anywhere in .text.  0x004478c0 and 0x00447904 are ONE
//  183-byte function -- Model::EvalSocket, in commonmodel.cpp.  This is the
//  second half of the false-call-target shape; the first
//  example was an `e8` inside a `mov $imm`, this one is an `e8` inside a
//  displacement.
//
//  ---- SIX ROWS ARE ALREADY WRITTEN (the screen missed them) -------------
//  A COMDAT of ours already maps onto each of these
//  addresses; they are unmatched NEARS, not untouched rows, so they are
//  /O2-shape work in existing files and reproducing them here would only
//  duplicate the wrong bytes.  What each one is:
//
//   * 0x00445a90 / 101  commonmodel.obj ?sub_100167e1@CUnknownD9E4@@QAEXXZ,
//     ours 94.  The gap is the node-array release: the shipped body calls
//     BaseDelete and MATERIALISES the element count into a dead stack slot
//     (0x00445bdf-style; here 0x00445aa5/0x00445aa8), where
//     commonmodel.cpp's body calls `((LAlloc*)m_pModel->m_pDataBlock)->
//     Free(m_ppNodeRecords)` straight out and never mentions the count.
//     Spell it `LDelete_Array(m_pModel->GetAlloc(), m_ppNodeRecords,
//     m_pModel->m_NodeTransforms.m_nElements)` -- i.e. the same statement
//     0x00445bb0 above uses -- and the seven bytes come back.
//
//   * 0x004467c0 / 418  commonmodel.obj ?sub_10017016@Model@@QAEXI@Z,
//     ours 426.
//   * 0x00446b80 / 166  ?sub_1001728c@Model@@QAEII@Z, ours 195.
//   * 0x00446c30 / 224  ?sub_100172e3@Model@@QAEII@Z, ours 228.
//     All three are call-site inlining work (which call site expands which
//     collection helper) rather than semantics -- each already matches at
//     /O1 in server.dll.  0x00446c30 is the closest (four bytes over) and
//     its mnemonic diff is a register rotation plus a zero parked in a
//     register where the shipped body tests in place; 0x00446b80 is 29
//     bytes over, which is an expansion the shipped image declined.
//
//   * 0x00447980 / 79 and 0x004479d0 / 76  collection.obj
//     ?GenGetNext@ / ?GenGetAt@?$CMoArray@VCCollElem1C@@VDefaultCache@@,
//     ours 43 and 40.  Diagnosed above: CCollElem1CS needs an in-class
//     copy constructor.  Measured, not inferred -- a byte diff on both
//     images shows ours emitting `rep movsl` where both shipped bodies
//     copy memberwise (/O2) or call the copy (/O1).
//
//  ---- THE ONE THING I COULD NOT SETTLE ---------------------------------
//  ModelPiece's destroy loops (0x004461a4 in ~ModelPiece and 0x00446206 in
//  sub_10016c3a) reach the element destructor as `push $0x0; call
//  0x0044ccb0`, and 0x0044ccb0 is a scalar-deleting-destructor shape:
//  it calls 0x004460a0 (the release) and then tests bit 0 of its argument
//  before handing the object to operator delete.  server.dll compiles the
//  SAME statement into a plain `mov %esi,%ecx; call 0x10016b86` with no
//  argument (0x1001b925).  A scalar deleting destructor is what VC6 emits
//  for a VIRTUAL destructor, but 0x0044ccb0 is in no vtable, the class's
//  only vptr is a collection's (slot 0 of 0x004a27e4 is GenBegin at
//  0x004126d0, not a destructor), and a virtual destructor would need a
//  thirteenth slot at 0x004a2814 -- which holds 0x004126d0, the next
//  vtable.  So the two spellings are:
//     (a) `~ModelPieceDataQ444();`          -- what is written above, and
//         what server.dll's /O1 twin proves;
//     (b) `virtual ~ModelPieceDataQ444();`  -- what the lithtech /O2 call
//         shape suggests, at the cost of a vptr the layout has no room for.
//  (a) is written because the /O1 oracle is evidence about the SOURCE and
//  the call shape is evidence about one compiler's lowering.  If
//  0x00446180 and 0x004461e0 come back two bytes short per destroy site
//  with an argument-less call, that is this, and it is a codegen question
//  rather than a source one.
//
// ======================================================================= //
