//------------------------------------------------------------------
//
//  MODULE  : WORLDTREE.CPP
//
//  PURPOSE : server.dll's world-tree module and the template code the
//            linker placed after it: 0x10037b47 .. 0x1003a54f, which
//            was counted as CRT until the app region was
//            corrected (the static CRT starts at _ftol, 0x1003e0f0).
//            The client's equivalent is src/engine/lithtech/worldtree2.cpp.
//
//  The file is laid out in address-range sections so that functions can
//  be written in parallel. Functions within a section go in shipped
//  address order. A function that turns out to belong to another
//  translation unit is written there instead, and noted here.
//
//------------------------------------------------------------------

#include "worldbsp.h"
#include "servermgr.h"

// ==== SECTION A: 0x10037b47 .. 0x10038000 ================================
//
// NOT WORLD TREE. The whole of section A is the tail of TCPDriver's object
// (src/engine/server/tcpdriver.cpp): TCPDriver::JoinSession runs from
// 0x100375e2 for 1434 bytes to 0x10037b7c (so nothing starts between
// 0x10037b47 and 0x10037b7c), and what follows is the driver's last two
// vtable slots, tcp_Create, and the `CMoArray<TCPQueryResponse,
// DefaultCache>` instantiation the driver carries at +0x50 -- the server
// twins of src/engine/lithtech/tcpqarray.cpp, which matches the client's
// copies. Everything here belongs in tcpdriver.cpp; it is written here with
// `_A` names so that it does not collide with tcpdriver.cpp's declarations.
//
// The element is tcpdriver.cpp's TCPQueryResponse (0xa4 bytes). Server
// allocator spellings: ::operator new (0x1000d25d) and ::operator delete
// (0x10028ace), as every other release of m_pData in tcpdriver.cpp.

#include "de_error.h"

// winsock2.h cannot lead here: worldbsp.h has already pulled windows.h and
// with it winsock.h, whose htons is NOT dllimport and would assemble as a
// five-byte direct call. The shipped call goes through the import table
// (`call *0x1004d1d4`, WSOCK32 ordinal 9), so it is spelled here as a call
// through a pointer, which is the same six bytes. In tcpdriver.cpp, where
// this code belongs, it is plain htons() under winsock2.h.
extern "C" unsigned short (__stdcall* htons_A)(unsigned short);
typedef struct sockaddr_in SockAddr_A;

// 0x10035efb (tcpdriver.cpp's tcp_BuildSockaddrFromString).
int tcp_BuildSockaddrFromString_A(char* pString, SockAddr_A* pAddr);

class TCPQueryResponse_A
{
public:
	TCPQueryResponse_A();                    // 0x1003654a (tcpdriver.cpp)

	// GenGetNext / GenGetAt store 0 to the result's +0x98 and then call
	// operator= (0x10037c6e, 0x10037cf5): this, expanded.
	TCPQueryResponse_A(const TCPQueryResponse_A& other)
	{
		m_pData = 0;
		*this = other;
	}

	~TCPQueryResponse_A() { if (m_pData) ::operator delete(m_pData); }

	// 0x10037c9c -- out of line at /O1; void, eax is left undefined.
	void operator=(const TCPQueryResponse_A& other)
	{
		if (m_pData)
			::operator delete(m_pData);

		memcpy(this, &other, sizeof(TCPQueryResponse_A));

		if (other.m_pData)
		{
			m_pData = (char*)::operator new(strlen(other.m_pData) + 1);
			strcpy(m_pData, other.m_pData);
		}
	}

	char       m_Text[0x80];                 // 0x00
	uint32     m_Unknown80;                  // 0x80
	float      m_Time;                       // 0x84
	SockAddr_A m_Addr;                       // 0x88
	char*      m_pData;                      // 0x98
	float      m_Unknown9c;                  // 0x9c
	float      m_UnknownA0;                  // 0xa0
};

// 0x10037c4c (ctor), 0x10037c6e GenGetNext, 0x10037cf5 GenGetAt,
// 0x10037d1e GenAppend, 0x10037d33 GenRemoveAt, 0x10037d44 GenRemoveAll,
// 0x10037d51 GenGetSize, 0x10037d55 GenCopyList, 0x10037e1a GenAppendList,
// 0x10037eea GenFindElement, 0x10037f14 Init, 0x10037f49 SetSize2,
// 0x10037fb9 InternalNiceSetSize (and 0x100380c9 Insert2 in section B).
template class CMoArray<TCPQueryResponse_A, DefaultCache>;

// Just enough of TCPDriver (tcpdriver.cpp) for the two slots below: 0xb0
// bytes, m_Socket at +0x90. The slots are virtual in the real class; their
// bodies do not depend on that.
class TCPDriver_A
{
public:
	TCPDriver_A();                           // 0x10036060

	LTRESULT ConnectTCP(char* pAddress);     // slot 0x54
	LTRESULT SendToAddr(void* pData, uint32 nLen, char* sAddr, uint32 port); // 0x58

	LTRESULT JoinSession(SockAddr_A* pHostAddr);                    // 0x100375e2
	LTBOOL   SendTo(uint32 hSocket, void* pData, uint32 nLen,
	                SockAddr_A* pTo);                               // 0x10036278

	uint8  m_pad00[0x90];
	uint32 m_Socket;                         // 0x90
	uint8  m_pad94[0xb0 - 0x94];
};

// 0x10037b7c -- slot 0x54 (lithtech twin 0x004870f0).
LTRESULT TCPDriver_A::ConnectTCP(char* pAddress)
{
	SockAddr_A addr;

	if (!tcp_BuildSockaddrFromString_A(pAddress, &addr))
		RETURN_ERROR(1, "ConnectTCP", LT_CANTBINDTOPORT)

	return JoinSession(&addr);
}

// 0x10037bda -- slot 0x58 (lithtech twin 0x00487160).
LTRESULT TCPDriver_A::SendToAddr(void* pData, uint32 nLen,
                                 char* sAddr, uint32 port)
{
	SockAddr_A addr;

	if (!m_Socket)
		return LT_NOTINITIALIZED;

	if (!tcp_BuildSockaddrFromString_A(sAddr, &addr))
		return LT_ERROR;

	addr.sin_port = htons_A((unsigned short)port);

	SendTo(m_Socket, pData, nLen, &addr);
	return LT_OK;
}

// 0x10037c32 -- servermgr.h's tcp_Create (lithtech twin 0x004871e0).
CBaseDriver* tcp_Create()
{
	return (CBaseDriver*)new TCPDriver_A;
}


// ==== END SECTION A =======================================================

// ==== SECTION B: 0x10038000 .. 0x10038674 ================================
//
// Three unrelated runs share this range:
//   1. 0x100380c9 .. 0x100383a4: four COMDATs of tcpdriver.cpp's
//      CMoArray<TCPQueryResponse> (the 0xa4 element: the constructor it
//      builds with is TCPQueryResponse::TCPQueryResponse, 0x1003654a, and the
//      destructor it expands frees +0x98 with ::operator delete, exactly
//      tcpdriver.cpp's inline ~TCPQueryResponse). They REALLY belong to
//      tcpdriver.cpp (the linker emitted them after 0x10038094,
//      tcp_FreeQueryList, which is also tcpdriver's); written here against a
//      copy of the element type.
//   2. 0x100383a5 .. 0x100383d8: the engine clock (the client twin is
//      0x004895e0 / 0x00489600, directly in front of CLTTransform).
//   3. 0x100383d9 .. 0x10038673: ILTTransform's nine virtuals. The server's
//      vtable 0x1004e660 is ILTTransform's own (CLTServer embeds a bare
//      ILTTransform at +0x228), so the bodies are ILTTransform:: members.
//      Its nine slots are 0x100383d9, 0x1003840a, 0x1003843b, 0x1003844f,
//      0x10038473, 0x100384af, 0x100384e9, 0x10038543, 0x100385d2.

// ---- 1. CMoArray<TCPQueryResponse> ------------------------------------- //
// 0x100380c9 Insert2, 0x1003820d Remove2, 0x10038329 BaseDelete and
// 0x10038366 BaseNew are the tail of section A's explicit instantiation of
// CMoArray<TCPQueryResponse_A, DefaultCache>; nothing is written for them
// here. Insert2 is NEAR (324 vs 322): collection.h's body, the residual is
// the m_pArray + i*0xa4 operand order into the operator= argument (shipped
// `mov 4(%esi),%ecx; add %eax,%ecx`). Register allocation; an out-of-class
// inline operator= made no difference.

// ---- 2. the engine clock ----------------------------------------------- //

extern "C" __declspec(dllimport) unsigned long __stdcall timeGetTime(void);

// 0x10058c24 — timeGetTime() at static-initialisation time. Its dynamic
// initialiser is the compiler's: 0x100383a5 (`jmp`) and 0x100383aa (the
// body, `call timeGetTime; mov %eax,0x10058c24; ret`).
static uint32 g_StartTime_B = timeGetTime();

// 0x100383b6 — seconds since startup (saveio.cpp's time_GetTime). The
// unsigned difference converts through a zero-extended fildll.
float time_GetTime()
{
	return (float)(timeGetTime() - g_StartTime_B) * 0.001f;
}

// ---- 3. ILTTransform --------------------------------------------------- //

#include "ilttransform.h"

// 0x10015bcd / 0x10015ce2 (commonmodel.h spells them the same way).
void quat_ConvertToMatrix(LTRotation* pRot, LTMatrix* pMat);
void quat_ConvertFromMatrix(float* pQuat, const float mat[4][4]);

// 0x100384c3 — a matrix back into a transform (name ours). A plain
// function, not inline: every slot below calls it, and it sits between
// FromMatrix and Inverse, so it is defined there.
void trans_FromMatrix_B(LTransform* pTransform, LTMatrix* pMat);

// 0x100383d9 — slot 0.
LTRESULT ILTTransform::Get(LTransform &transform, LTVector &pos, LTRotation &rot)
{
	pos = transform.m_Pos;
	rot = transform.m_Rot;

	return LT_OK;
}

// 0x1003840a — slot 1.
LTRESULT ILTTransform::Set(LTransform &transform, LTVector &pos, LTRotation &rot)
{
	transform.m_Pos = pos;
	transform.m_Rot = rot;

	return LT_OK;
}

// 0x1003843b — slot 2.
LTRESULT ILTTransform::GetPos(LTransform &transform, LTVector &pos)
{
	pos = transform.m_Pos;

	return LT_OK;
}

// 0x1003844f — slot 3.
LTRESULT ILTTransform::GetRot(LTransform &transform, LTRotation &rot)
{
	rot = transform.m_Rot;

	return LT_OK;
}

// 0x10038473 — slot 4. The 0xc frame is SetTranslation's by-value vector.
LTRESULT ILTTransform::ToMatrix(LTransform &transform, LTMatrix &mat)
{
	quat_ConvertToMatrix(&transform.m_Rot, &mat);
	mat.SetTranslation(transform.m_Pos);

	return LT_OK;
}

// 0x100384af — slot 5.
LTRESULT ILTTransform::FromMatrix(LTransform &transform, LTMatrix &mat)
{
	trans_FromMatrix_B(&transform, &mat);

	return LT_OK;
}

void trans_FromMatrix_B(LTransform* pTransform, LTMatrix* pMat)
{
	pTransform->m_Pos.x = pMat->m[0][3];
	pTransform->m_Pos.y = pMat->m[1][3];
	pTransform->m_Pos.z = pMat->m[2][3];

	quat_ConvertFromMatrix(pTransform->m_Rot.m_Quat, pMat->m);
}

// 0x100384e9 — slot 6. Frame 0x8c = vector + two matrices. The inverse is
// computed (Mat_InverseTransformation, out of line at 0x10011d32) and then
// NOT used: the result is built from the original matrix -- a Monolith bug
// (the /O2 client drops the dead inverse altogether, 0x00489780).
LTRESULT ILTTransform::Inverse(LTransform &inverse, LTransform &transform)
{
	LTMatrix mat, matInverse;

	quat_ConvertToMatrix(&transform.m_Rot, &mat);
	mat.SetTranslation(transform.m_Pos);

	Mat_InverseTransformation(&mat, &matInverse);
	trans_FromMatrix_B(&inverse, &mat);

	return LT_OK;
}

// 0x10038543 — slot 7. out = t1 * t2.
LTRESULT ILTTransform::Multiply(LTransform &out, LTransform &t1, LTransform &t2)
{
	LTMatrix mat1, mat2, matOut;

	quat_ConvertToMatrix(&t1.m_Rot, &mat1);
	mat1.SetTranslation(t1.m_Pos);

	quat_ConvertToMatrix(&t2.m_Rot, &mat2);
	mat2.SetTranslation(t2.m_Pos);

	MatMul(&matOut, &mat1, &mat2);
	trans_FromMatrix_B(&out, &matOut);

	return LT_OK;
}

// 0x100385d2 — slot 8. diff = inverse(t2) * t1.
LTRESULT ILTTransform::Difference(LTransform &diff, LTransform &t1, LTransform &t2)
{
	LTMatrix mat1, mat2, mat2Inv, matDiff;

	quat_ConvertToMatrix(&t1.m_Rot, &mat1);
	mat1.SetTranslation(t1.m_Pos);

	quat_ConvertToMatrix(&t2.m_Rot, &mat2);
	mat2.SetTranslation(t2.m_Pos);

	Mat_InverseTransformation(&mat2, &mat2Inv);
	MatMul(&matDiff, &mat2Inv, &mat1);
	trans_FromMatrix_B(&diff, &matDiff);

	return LT_OK;
}

// ==== END SECTION B =======================================================

// ==== SECTION C: 0x10038674 .. 0x10038ff1 ================================
//
// Two modules share this range, and neither is the world tree proper.
//
// 0x10038674 .. 0x10038e1e is the model TRANSFORM MAKER: the walker that
// evaluates a model's node transforms from up to eight blended animation
// references. commonmodel.h calls the class ModelWalker (0x248 bytes, built
// by 0x100175b6) and declares two of its members, sub_100386c6 and
// Evaluate; the client twin is src/engine/lithtech/transform.cpp's
// CTransformMakerK (0x00489a60 .. 0x0048a4ab), whose layout is re-used
// here verbatim. The class is written here as a _C-suffixed view with every
// member it needs; once consolidated these are ModelWalker's own members
// (commonmodel.h would need the declarations — a header edit, not made).
//   0x10038674 IsValid                    (client 0x00489a60)
//   0x100386c6 SetupTransforms            (client 0x00489ac0) = ModelWalker::sub_100386c6
//   0x100386f7 SetupTransformsForNode     (client 0x00489b00) = ModelWalker::Evaluate
//   0x10038781 Prepare                    (client 0x00489bc0), 227 bytes
//   0x10038864 GetNodeTransform           (client 0x00489ce0)
//   0x100389c9 GetNodeTransformRel        (client 0x00489eb0)
//   0x10038b0a BlendNode                  (client 0x0048a110)
//   0x10038c4d ProcessNode                (client 0x0048a2d0)
//   0x10038e1f / 0x10038e3e are LTRotation::operator= and ::Conjugate, the
//   SDK COMDATs these call (already produced by sdkforce.cpp).
// Status: IsValid, SetupTransforms, SetupTransformsForNode, Prepare,
// BlendNode and ProcessNode byte-identical; GetNodeTransform (361 vs 357)
// and GetNodeTransformRel (323 vs 321) are nears whose residual is
// register/slot choice only (see the notes above each).
// 0x1003882d is NOT a function: it is the zero displacement of the
// `cmpl $0x0,0xe8(%esi)` at 0x10038826 inside Prepare read as `e8 00000000`.
//
// 0x10038e77 .. 0x10038ff0 is the start of the server's visibility
// traversal (client fntdraw.cpp's 0x0048b8e0 family): the entry point
// CWorldBsp::DoVisQuery calls and the default object-list collector it
// installs. The per-node recursion 0x10038ff1 is section D's.

#include "ltanimtracker.h"

// ---- the model as the walker sees it (commonmodel.h's Model) ------------

// CModelXform, with the position typed as an LTVector: the lerps below go
// through _CVector's operators, whose by-value right operands are the
// 12-byte `movsl` copies in every one of them.
class CModelXform_C
{
public:
	LTVector   m_Pos;                        // 0x00
	LTRotation m_Rot;                        // 0x0c
};

// An animation's per-node keyframe track: its transform table at +0xc.
class CAnimNode_C
{
public:
	uint8          m_pad_00[0x0c];
	CModelXform_C* m_pXforms;                // 0x0c
};

class CModelAnim_C
{
public:
	uint8          m_pad_00[0x04];
	CAnimNode_C**  m_pNodes;                 // 0x04
};

// The child-model record an animation came from: its default pose at +4.
class CChildModel_C
{
public:
	uint8          m_pad_00[0x04];
	CModelXform_C* m_pXforms;                // 0x04
};

// commonmodel.h's AnimInfo, 0x20 bytes.
class CAnimInfo_C
{
public:
	CModelAnim_C*  m_pAnim;                  // 0x00
	CChildModel_C* m_pChildModel;            // 0x04
	LTVector       m_vDims;                  // 0x08
	LTVector       m_vTranslation;           // 0x14
};

class CWeightSet_C
{
public:
	char    m_Name[0x10];                    // 0x00
	uint32  m_nAllocedWeights;               // 0x10
	float*  m_pWeights;                      // 0x14
};

// commonmodel.h's CModelSub20C / ModelNode.
class CModelNode_C
{
public:
	uint8          m_pad_00[0x04];
	LTVector       m_vOffset;                // 0x04
	uint16         m_iNode;                  // 0x10
	uint8          m_Flags;                  // 0x12
	uint8          m_pad_13[0x1c - 0x13];
	CModelNode_C** m_pChildren;              // 0x1c  (m_Array18's array)
	uint32         m_nChildren;              // 0x20
	uint8          m_pad_24[0x28 - 0x24];
	uint32         m_iParent;                // 0x28
};

class CModel_C
{
public:
	uint8          m_pad_00[0x28];
	CModelNode_C** m_pNodes;                 // 0x028  m_Nodes' array
	uint8          m_pad_2c[0x48 - 0x2c];
	CWeightSet_C** m_pWeightSets;            // 0x048  m_WeightSets' array
	uint32         m_nWeightSets;            // 0x04c
	uint8          m_pad_50[0x74 - 0x50];
	LTMatrix*      m_pTransforms;            // 0x074  m_NodeTransforms' array
	uint32         m_nTransforms;            // 0x078
	uint8          m_pad_7c[0x168 - 0x7c];
	CAnimInfo_C*   m_pAnims;                 // 0x168  m_AnimArray's array
	uint8          m_pad_16c[0x2c0 - 0x16c];
	CModelNode_C*  m_pRootNode;              // 0x2c0
};

#define MAX_TM_TIMEREFS_C  8
#define MAX_NODE_PATH_C    0x40

// ModelWalker (commonmodel.h), 0x248 bytes.
class CTransformMaker_C
{
public:
	LTBOOL IsValid();                                           // 0x10038674
	LTBOOL SetupTransforms();                                   // 0x100386c6
	LTBOOL SetupTransformsForNode(uint32 iNode);                // 0x100386f7
	LTBOOL Prepare();                                           // 0x10038781
	void   GetNodeTransform(uint32 iTimeRef, uint32 iNode,
	                        LTRotation* pRot, LTVector* pPos);  // 0x10038864
	void   GetNodeTransformRel(uint32 iTimeRef, uint32 iNode,
	                           LTRotation* pRot, LTVector* pPos);// 0x100389c9
	void   BlendNode(uint32 iTimeRef, uint32 iNode);            // 0x10038b0a
	void   ProcessNode(uint32 iNode, LTMatrix* pParent);        // 0x10038c4d

	AnimTimeRef    m_TimeRefs[MAX_TM_TIMEREFS_C];    // 0x000
	uint32         m_nTimeRefs;                      // 0x0e0
	LTMatrix*      m_pBaseTransform;                 // 0x0e4
	LTMatrix*      m_pTransforms;                    // 0x0e8
	NodeControlFn  m_NodeControlFn;                  // 0x0ec
	void*          m_pNodeControlData;               // 0x0f0
	HOBJECT        m_hNodeControlObject;             // 0x0f4
	uint32*        m_pNodePath;                      // 0x0f8
	uint32         m_nNodePath;                      // 0x0fc
	CModelXform_C* m_pCurXform;                      // 0x100
	LTMatrix       m_matXform;                       // 0x104
	CChildModel_C* m_pAnimOwner;                     // 0x144
	LTMatrix       m_matNode;                        // 0x148
	LTRotation     m_rBlend;                         // 0x188
	LTVector       m_vBlend;                         // 0x198
	CModel_C*      m_pModel;                         // 0x1a4
	LTMatrix       m_matIdentity;                    // 0x1a8
	CWeightSet_C*  m_pWeightSets[MAX_TM_TIMEREFS_C]; // 0x1e8
	CModelAnim_C*  m_pPrevAnims[MAX_TM_TIMEREFS_C];  // 0x208
	CModelAnim_C*  m_pCurAnims[MAX_TM_TIMEREFS_C];   // 0x228
};

// 0x10058c28 — the scratch 4x4 ProcessNode multiplies through.
static LTMatrix g_matTMTemp_C;

LTBOOL CTransformMaker_C::IsValid()
{
	CModel_C *pModel;
	uint32 i;

	if (m_nTimeRefs == 0 || m_nTimeRefs > MAX_TM_TIMEREFS_C)
		return LTFALSE;

	pModel = (CModel_C*)m_TimeRefs[0].m_pModel;

	for (i = 0; i < m_nTimeRefs; i++)
	{
		if (!m_TimeRefs[i].IsValid())
			return LTFALSE;

		if ((CModel_C*)m_TimeRefs[i].m_pModel != pModel)
			return LTFALSE;

		if (i != 0 && m_TimeRefs[i].m_iWeightSet >= pModel->m_nWeightSets)
			return LTFALSE;
	}

	return LTTRUE;
}

LTBOOL CTransformMaker_C::SetupTransforms()
{
	if (!Prepare())
		return LTFALSE;

	ProcessNode(m_pModel->m_pRootNode->m_iNode, m_pBaseTransform);

	return LTTRUE;
}

LTBOOL CTransformMaker_C::SetupTransformsForNode(uint32 iNode)
{
	uint32 nodePath[MAX_NODE_PATH_C];

	if (!Prepare())
		return LTFALSE;

	if (iNode >= m_pModel->m_nTransforms)
		return LTFALSE;

	m_nNodePath = 0;
	while (1)
	{
		nodePath[m_nNodePath] = iNode;
		m_nNodePath++;

		iNode = m_pModel->m_pNodes[iNode]->m_iParent;
		if (iNode == 0xffffffff)
			break;

		if (m_nNodePath >= MAX_NODE_PATH_C)
			return LTFALSE;
	}

	m_pNodePath = nodePath;
	m_nNodePath--;

	ProcessNode(m_pModel->m_pRootNode->m_iNode, m_pBaseTransform);

	return LTTRUE;
}

LTBOOL CTransformMaker_C::Prepare()
{
	uint32 i;

	if (!IsValid())
		return LTFALSE;

	m_pNodePath = LTNULL;
	m_pModel = (CModel_C*)m_TimeRefs[0].m_pModel;

	for (i = 0; i < m_nTimeRefs; i++)
	{
		if (i > 0)
			m_pWeightSets[i] = m_pModel->m_pWeightSets[m_TimeRefs[i].m_iWeightSet];

		m_pPrevAnims[i] = m_pModel->m_pAnims[m_TimeRefs[i].m_Prev.m_iAnim].m_pAnim;
		m_pCurAnims[i]  = m_pModel->m_pAnims[m_TimeRefs[i].m_Cur.m_iAnim].m_pAnim;
	}

	if (!m_pBaseTransform)
	{
		m_matIdentity.Identity();
		m_pBaseTransform = &m_matIdentity;
	}

	if (!m_pTransforms)
		m_pTransforms = m_pModel->m_pTransforms;

	m_pAnimOwner = m_pModel->m_pAnims[m_TimeRefs[0].m_Prev.m_iAnim].m_pChildModel;

	return LTTRUE;
}

// NEAR, 361 vs 357. Everything after the address arithmetic matches. The
// shipped code puts each keyframe pointer in the FRAME-index register
// (`imul $0x1c; add 0xc(%node),%idx`) and keeps pCur in a fresh slot; ours
// loads the table into the node register and homes pCur in iTimeRef's dead
// slot. Tried with no effect: statement/declaration order, node locals, an
// inline accessor, CMoArray operator[]/Get, byte-offset and commuted
// pointer arithmetic; with no pRef (worse). Also no effect: uint32
// frame-index locals, base-then-`+=` and split-assignment forms, and a
// pAnims local with references in the iNode==0 block; worse: pRot->Slerp
// (8 blocks), pCur declared first (13), m_TimeRefs[iTimeRef] frame reads
// (24). The iNode==0 block's two LTVector pointer locals are what the
// shipped `lea 0x14(...)` pair wants. Also no effect: node locals
// assigned first (VC6 forward-substitutes them), integer-address
// `frame*sizeof + (uint32)table`, FrameLocator pointer locals, an
// AnimTimeRef reference, `(int)` frame casts, an int iTimeRef, `register`;
// worse: the slerp+lerp as an inline helper (called at /Ob1; 422 bytes
// with __forceinline).
void CTransformMaker_C::GetNodeTransform(uint32 iTimeRef, uint32 iNode,
                                         LTRotation* pRot, LTVector* pPos)
{
	AnimTimeRef   *pRef  = &m_TimeRefs[iTimeRef];
	CModelXform_C *pPrev = &m_pPrevAnims[iTimeRef]->m_pNodes[iNode]
	                        ->m_pXforms[pRef->m_Prev.m_iFrame];
	CModelXform_C *pCur  = &m_pCurAnims[iTimeRef]->m_pNodes[iNode]
	                        ->m_pXforms[pRef->m_Cur.m_iFrame];

	quat_Slerp(pRot->m_Quat, pPrev->m_Rot.m_Quat, pCur->m_Rot.m_Quat,
	           pRef->m_Percent);

	*pPos = pPrev->m_Pos + (pCur->m_Pos - pPrev->m_Pos) * pRef->m_Percent;

	if (iNode == 0)
	{
		LTVector *pPrevT = &m_pModel->m_pAnims[pRef->m_Prev.m_iAnim].m_vTranslation;
		LTVector *pCurT  = &m_pModel->m_pAnims[pRef->m_Cur.m_iAnim].m_vTranslation;

		*pPos += *pPrevT + (*pCurT - *pPrevT) * pRef->m_Percent;
	}
}

// NEAR, 323 vs 321: the same keyframe-pointer residual as GetNodeTransform
// (here pRef/pCur swap between iTimeRef's slot and a fresh one). The
// conjugate is ONE out-of-line LTRotation copy (0x10038e1f) into the
// unnamed operand of operator*: `~x * y` written inline. A named
// `LTRotation r = ~x;` or `r = ~x;` adds a second, inline copy.
void CTransformMaker_C::GetNodeTransformRel(uint32 iTimeRef, uint32 iNode,
                                            LTRotation* pRot, LTVector* pPos)
{
	AnimTimeRef   *pRef  = &m_TimeRefs[iTimeRef];
	CModelXform_C *pBase = m_pPrevAnims[iTimeRef]->m_pNodes[iNode]->m_pXforms;
	CModelXform_C *pPrev = &pBase[pRef->m_Prev.m_iFrame];
	CModelXform_C *pCur  = &m_pCurAnims[iTimeRef]->m_pNodes[iNode]
	                        ->m_pXforms[pRef->m_Cur.m_iFrame];

	quat_Slerp(pRot->m_Quat, pPrev->m_Rot.m_Quat, pCur->m_Rot.m_Quat,
	           pRef->m_Percent);

	*pPos = pPrev->m_Pos + (pCur->m_Pos - pPrev->m_Pos) * pRef->m_Percent;

	*pRot = ~pBase->m_Rot * *pRot;

	*pPos = *pPos - pBase->m_Pos;
}

void CTransformMaker_C::BlendNode(uint32 iTimeRef, uint32 iNode)
{
	float      fWeight;
	LTRotation rot, rOld;
	LTVector   vPos;

	fWeight = m_pWeightSets[iTimeRef]->m_pWeights[iNode];
	if (fWeight == 0.0f)
		return;

	if (fWeight == 2.0f)
	{
		GetNodeTransformRel(iTimeRef, iNode, &rot, &vPos);

		m_rBlend = m_rBlend * rot;
		m_vBlend = m_vBlend + vPos;
	}
	else
	{
		GetNodeTransform(iTimeRef, iNode, &rot, &vPos);

		rOld = m_rBlend;
		quat_Slerp(m_rBlend.m_Quat, rOld.m_Quat, rot.m_Quat, fWeight);

		m_vBlend = m_vBlend + (vPos - m_vBlend) * fWeight;
	}
}

// Byte-identical. The shipped tail jumps back to the top (0x10038c58) are
// an EXPLICIT loop in the source, not the compiler folding two tail
// recursions: written as recursions (468 vs 466, 11 blocks) VC6 still
// folds them but CSEs &m_rBlend / &m_nTimeRefs into %esi/%edi, where the
// loop form keeps iNode in %esi and the blend counter in %edi as shipped.
// The path arm is `if (m_nNodePath > 0) {..} else return;` (the `jbe`).
// m_pCurXform is assigned first and pXform read back from it (the
// `lea (%eax,%ecx,1),%esi` at 0x10038d45). The children walk takes iNode's
// slot for its counter; the literal `m_nNodePath = 1` store is in the
// bytes (0x10038df6).
void CTransformMaker_C::ProcessNode(uint32 iNode, LTMatrix* pParent)
{
	LTMatrix      *pTransform;
	CModelNode_C  *pNode;
	CModelXform_C *pXform;
	uint32         i, iChild, nLast;

	for (;;)
	{
		pTransform = &m_pTransforms[iNode];

		GetNodeTransform(0, iNode, &m_rBlend, &m_vBlend);

		for (i = 1; i < m_nTimeRefs; i++)
			BlendNode(i, iNode);

		pNode = m_pModel->m_pNodes[iNode];

		m_rBlend.ConvertToMatrix(m_matNode);

		if (pNode->m_Flags & 2)
			m_matNode.SetTranslation(pNode->m_vOffset);
		else
			m_matNode.SetTranslation(m_vBlend);

		MatMul(&g_matTMTemp_C, pParent, &m_matNode);

		m_pCurXform = &m_pAnimOwner->m_pXforms[iNode];
		pXform = m_pCurXform;

		quat_ConvertToMatrix(pXform->m_Rot.m_Quat, m_matXform.m);
		m_matXform.SetTranslation(pXform->m_Pos);

		MatMul(pTransform, &g_matTMTemp_C, &m_matXform);

		if (m_NodeControlFn)
			m_NodeControlFn(m_hNodeControlObject, iNode, pTransform,
			                m_pNodeControlData);

		if (m_pNodePath)
		{
			if (m_nNodePath > 0)
			{
				m_nNodePath--;
				pParent = pTransform;
				iNode = m_pNodePath[m_nNodePath];
			}
			else
				return;
		}
		else
		{
			nLast = pNode->m_nChildren;
			if (!nLast)
				return;
			nLast--;

			for (iChild = 0; iChild < nLast; iChild++)
				ProcessNode(pNode->m_pChildren[iChild]->m_iNode, pTransform);

			m_nNodePath = 1;
			iNode = pNode->m_pChildren[nLast]->m_iNode;
			pParent = pTransform;
		}
	}
}


// ---- the visibility traversal's entry point -----------------------------

class CVisObject_C
{
public:
	uint8    m_pad_00[0x58];
	uint32   m_Unknown58;                    // 0x058  the visit mark
};

// The list head is the 8-byte half of a link (the SDK's CheapLTLink); the
// links on it carry the object at +8 (LTLink).
class CVisLink_C;

class CVisLinkHead_C
{
public:
	CVisLink_C*   m_pPrev;                   // 0x00
	CVisLink_C*   m_pNext;                   // 0x04
};

class CVisLink_C : public CVisLinkHead_C
{
public:
	CVisObject_C* m_pData;                   // 0x08
};

// 0x30 bytes, at CWorldBsp +0x88/+0x8c.
class CVisRecord_C
{
public:
	uint8   m_pad_00[0x30];
};

// 0x18 bytes: one per world leaf, at CWorldBsp +0x70/+0x74.
class CVisLeaf_C
{
public:
	uint8          m_pad_00[0x0c];
	CVisLinkHead_C m_Objects;                // 0x0c  the leaf's object list
	uint16         m_iRecord;                // 0x14  0xffff = none
	uint8      m_pad_16[0x18 - 0x16];
};

// 0x24 bytes, at CWorldBsp +0xb0 (m_pChunks).
class CVisChunk_C
{
public:
	uint8   m_pad_00[0x06];
	uint8   m_Flags;                         // 0x06  bit 0 tested
	uint8   m_pad_07[0x24 - 0x07];
};

class CVisRecordArray_C
{
public:
	uint32        m_Unknown00;               // 0x00
	CVisRecord_C* m_pArray;                  // 0x04
	uint32        m_nElements;               // 0x08

	inline CVisRecord_C* GetRecord(uint32 i)
	{
		return i < m_nElements ? &m_pArray[i] : LTNULL;
	}
};

// CWorldBsp, as the traversal reaches it.
class CVisWorld_C
{
public:
	uint8        m_pad_00[0x70];
	CVisLeaf_C*  m_pLeaves;                  // 0x70
	uint32       m_nLeaves;                  // 0x74
	uint8        m_pad_78[0x84 - 0x78];
	CVisRecordArray_C m_Records;             // 0x84
	uint8        m_pad_90[0x9c - 0x90];
	void*        m_pRootNode;                // 0x9c
	uint8        m_pad_a0[0xb0 - 0xa0];
	CVisChunk_C* m_pChunks;                  // 0xb0

};

// The tree the query may name: 8-byte entries at +0x10, count at +0x2e.
class CVisTreeEntry_C
{
public:
	uint16  m_iChunk;                        // 0x00
	uint16  m_pad_02;
	uint32  m_Value;                         // 0x04
};

class CVisTree_C
{
public:
	uint8            m_pad_00[0x10];
	CVisTreeEntry_C* m_pEntries;             // 0x10
	uint8            m_pad_14[0x2e - 0x14];
	uint16           m_nEntries;             // 0x2e
};

typedef void (*VisObjFn_C)(CVisObject_C* pObj, void* pUser);
typedef void (*VisRecordFn_C)(CVisRecord_C* pRecord, void* pUser);
typedef void (*VisCollectFn_C)(CVisLinkHead_C* pHead, CVisObject_C*** ppArray,
                               uint32* pnCount);
typedef int  (*VisFilterFn_C)(CVisChunk_C* pChunk);

class CWorldVisQuery_C
{
public:
	CVisTree_C*    m_pTree;                  // 0x00
	CVisWorld_C*   m_pWorld;                 // 0x04
	VisObjFn_C     m_ObjFn;                  // 0x08
	VisRecordFn_C  m_RecordFn;               // 0x0c
	VisCollectFn_C m_CollectFn;              // 0x10
	VisFilterFn_C  m_FilterFn;               // 0x14
	void*          m_pUser;                  // 0x18
	uint32         m_Mark;                   // 0x1c
};

#define VIS_BUFFER_SIZE_C  5000

// 0x10058c68 .. 0x10058c74 — the traversal's state. Section D's recursion
// (0x10038ff1) reads all four.
CVisObject_C**    g_pVisLimit_C;             // 0x10058c68
CWorldVisQuery_C* g_pVisQuery_C;             // 0x10058c6c
uint32            g_nVisBit_C;               // 0x10058c70
uint32            g_VisValue_C;              // 0x10058c74

// 0x10038ff1 — the per-node recursion (section D).
void wt_VisitNode_C(void* pNode, CVisObject_C** pBuffer, uint32 nBuffered);

// 0x10038fae — the default collector (client 0x0048bc50).
void wt_CollectList_C(CVisLinkHead_C* pHead, CVisObject_C*** ppArray,
                      uint32* pnCount);

// 0x10038e77 (client 0x0048b8e0, which adds a PVS walk the server lacks).
// The record lookup is an inline accessor on a sub-object at +0x84; the
// same ternary written on the world itself loads the array base into its
// own register first.
void wt_VisibilityTraversal(CWorldVisQuery* pVisQuery)
{
	CWorldVisQuery_C* pQuery = (CWorldVisQuery_C*)pVisQuery;
	CVisObject_C*     buffer[VIS_BUFFER_SIZE_C];
	CVisObject_C**    pObjects;
	uint32            i, j, nObjects;

	g_pVisLimit_C = &buffer[VIS_BUFFER_SIZE_C];

	if (!pQuery->m_CollectFn)
		pQuery->m_CollectFn = wt_CollectList_C;

	g_pVisQuery_C = pQuery;

	if (pQuery->m_pTree && pQuery->m_pTree->m_nEntries > 0)
	{
		for (i = 0; i < pQuery->m_pTree->m_nEntries; i++)
		{
			CVisTreeEntry_C* pEntry = &pQuery->m_pTree->m_pEntries[i];

			if (i != 0)
			{
				CVisChunk_C* pChunk = &pQuery->m_pWorld->m_pChunks[pEntry->m_iChunk];

				if (!(pChunk->m_Flags & 1))
					continue;

				if (!pQuery->m_FilterFn(pChunk))
					continue;
			}

			g_VisValue_C = pEntry->m_Value;
			g_nVisBit_C = 0;
			wt_VisitNode_C(pQuery->m_pWorld->m_pRootNode, buffer, 0);
		}
	}
	else
	{
		for (i = 0; i < pQuery->m_pWorld->m_nLeaves; i++)
		{
			CVisLeaf_C* pLeaf = &pQuery->m_pWorld->m_pLeaves[i];

			nObjects = 0;
			pObjects = buffer;

			if (pLeaf->m_iRecord != 0xffff)
			{
				CVisRecord_C* pRecord = pQuery->m_pWorld->m_Records.GetRecord(pLeaf->m_iRecord);
				if (pRecord)
					pQuery->m_RecordFn(pRecord, pQuery->m_pUser);
			}

			pQuery->m_CollectFn(&pLeaf->m_Objects, &pObjects, &nObjects);

			for (j = 0; (int)j < (int)nObjects; j++)
				pQuery->m_ObjFn(pObjects[j], pQuery->m_pUser);
		}
	}
}

void wt_CollectList_C(CVisLinkHead_C* pHead, CVisObject_C*** ppArray,
                      uint32* pnCount)
{
	CVisLink_C* pCur;

	pCur = pHead->m_pPrev;

	while (pCur != (CVisLink_C*)pHead)
	{
		CVisObject_C* pObj = pCur->m_pData;

		if (*ppArray < g_pVisLimit_C &&
		    pObj->m_Unknown58 != g_pVisQuery_C->m_Mark)
		{
			(*ppArray)[*pnCount] = pObj;
			(*pnCount)++;
		}

		pCur = pCur->m_pPrev;
	}
}

// ==== END SECTION C =======================================================

// ==== SECTION D: 0x10038ff1 .. 0x100399a5 ================================
//
// Three runs, each the server twin of client code already written up:
//
//   0x10038ff1 .. 0x100392a8  the PVS walk's per-node recursion and its
//       three helpers: lithtech.exe 0x0048be40 (which EXPANDS all three
//       inline at /O2) and 0x0048bca0 / 0x0048bd70; fntdraw.cpp documents
//       the query and the globals.  Server.dll calls all three out of line.
//   0x100392a9 .. 0x100395a3  the RIFF/WAVE reader: lithtech.exe 0x0048c1a0
//       and 0x0048c270, lttexmod.cpp's ParseWaveFile / ParseWaveChunk.
//       smgr.cpp's fs_GetSourceInfo is the first of these.
//   0x100395a4 .. 0x100399a4  the world tree's object base and node class:
//       lithtech.exe 0x0048c540 .. 0x0048c9b0, worldtree2.cpp items 1-12.
//       0x10039707/0x10039735/0x10039740 are objtypes.h's CObjRoot ctor,
//       BaseTerm and Unlink (vtable 0x1004ea04: slot 2 is 0x10039740, slot
//       3 _purecall).
//
// Names carry the section suffix _D.

#include <string.h>
#include "ltplane.h"

// ---- the PVS walk ------------------------------------------------------ //

// The walked object: +0x58 the visit mark, +0x18c/+0x1a4 LTObject's m_Pos
// and m_Radius.
class CVisObj_D
{
public:
	uint8    m_pad_00[0x58];
	uint32   m_nMark;                        // 0x058
	uint8    m_pad_5c[0x18c - 0x5c];
	LTVector m_Pos;                          // 0x18c
	uint8    m_pad_198[0x1a4 - 0x198];
	float    m_Radius;                       // 0x1a4
};

// A leaf record, 0x30 bytes (`lea (%ecx,%ecx,2); shl $4`).
struct VisLeaf_D
{
	uint8 m_pad[0x30];
};

class CVisWorld_D
{
public:
	uint8      m_pad_00[0x88];
	VisLeaf_D* m_pLeafs;                     // 0x88
	uint32     m_nLeafs;                     // 0x8c

	VisLeaf_D* GetLeaf(uint32 i)
	{
		return (i < m_nLeafs) ? &m_pLeafs[i] : LTNULL;
	}
};

struct VisQuery_D
{
	void*         m_p00;                     // 0x00
	CVisWorld_D*  m_pWorld;                  // 0x04
	void        (*m_pfnObject)(CVisObj_D* pObj, void* pUser);        // 0x08
	void        (*m_pfnLeaf)(VisLeaf_D* pLeaf, void* pUser);         // 0x0c
	void        (*m_pfnGather)(void* pList, CVisObj_D*** ppArray,
	                           uint32* pnCount);                     // 0x10
	int         (*m_pfn14)(void* pRecord);                           // 0x14
	void*         m_pUser;                   // 0x18
	uint32        m_nMark;                   // 0x1c
};

struct VisSurface_D
{
	uint8    m_pad_00[0x34];
	LTPlane* m_pPlane;                       // 0x34
};

struct VisPoly_D
{
	uint8         m_pad_00[0x28];
	VisSurface_D* m_pSurface;                // 0x28
};

struct VisNode_D
{
	VisPoly_D*  m_pPoly;                     // 0x00
	VisNode_D*  m_Sides[2];                  // 0x04
	CheapLTLink m_Objects;                   // 0x0c
	uint16      m_nLeaf;                     // 0x14  0xffff: none
	uint8       m_Flags;                     // 0x16

	LTPlane* GetPlane() { return m_pPoly->m_pSurface->m_pPlane; }
};

extern CVisObj_D**  g_pVisLimit_D;           // 0x10058c68
extern VisQuery_D*  g_pVisQuery_D;           // 0x10058c6c
extern uint32       g_nVisBit_D;             // 0x10058c70
extern uint8*       g_pVisBits_D;            // 0x10058c74
// 0x1005911c..0x10059128 (.bss): the walk's own four scratch globals, used
// by nothing but vis_WalkNode_D, and so defined here (the four above are
// SECTION C's, under their _C names).
VisLeaf_D*   g_pVisLeaf_D;                   // 0x1005911c
CVisObj_D**  g_pVisCursor_D;                 // 0x10059120
uint32       g_nVisFiltered_D;               // 0x10059124
CVisObj_D**  g_pVisFiltered_D;               // 0x10059128

uint8       vis_ReadBit_D();                                         // 0x10039151
CVisObj_D** vis_FilterNeg_D(LTPlane* pPlane, CVisObj_D** pIn, uint32 nIn,
                            uint32* pnOut, LTBOOL bAppend);          // 0x10039177
CVisObj_D** vis_FilterPos_D(LTPlane* pPlane, CVisObj_D** pIn, uint32 nIn,
                            uint32* pnOut);                          // 0x10039211

// 0x10038ff1 — one node of the walk (lithtech 0x0048be40). The loop is in
// the source: written as two plain recursive calls, VC6 keeps both calls,
// where the shipped code rewrites its own argument slots and jumps back.
// NEAR (354 vs 352, one block): the leaf address is built index-first
// into %ecx (`lea; shl; add 0x88(%edx),%ecx`) in the shipped code and
// base-first through %edi here. Ternary/if accessor, byte arithmetic,
// operand order, index type, a local and bBack's scope all measured: no
// change; nor does modelling +0x84 as a CMoArray<VisLeaf_D> (GetSize() /
// operator[], ternary or if/return), and a result-local GetLeaf loses the
// ternary's jmp. Register allocation.
void vis_WalkNode_D(VisNode_D* pNode, CVisObj_D** pArray, uint32 nCount)
{
	uint32 bBack;

	for (;;)
	{
		g_pVisQuery_D->m_pfnGather(&pNode->m_Objects, &pArray, &nCount);

		if (pNode->m_nLeaf != 0xffff && vis_ReadBit_D())
		{
			g_pVisLeaf_D = g_pVisQuery_D->m_pWorld->GetLeaf(pNode->m_nLeaf);
			if (g_pVisLeaf_D)
				g_pVisQuery_D->m_pfnLeaf(g_pVisLeaf_D, g_pVisQuery_D->m_pUser);

			g_pVisCursor_D = pArray;
			while (nCount--)
			{
				if ((*g_pVisCursor_D)->m_nMark != g_pVisQuery_D->m_nMark)
				{
					(*g_pVisCursor_D)->m_nMark = g_pVisQuery_D->m_nMark;
					g_pVisQuery_D->m_pfnObject(*g_pVisCursor_D, g_pVisQuery_D->m_pUser);
				}

				g_pVisCursor_D++;
			}

			nCount = 0;
		}

		bBack = pNode->m_Sides[1]->m_Flags & 8;

		if ((pNode->m_Sides[0]->m_Flags & 8) && vis_ReadBit_D())
		{
			g_pVisFiltered_D = vis_FilterNeg_D(pNode->GetPlane(), pArray, nCount,
			                                   &g_nVisFiltered_D, bBack);
			if (bBack)
			{
				vis_WalkNode_D(pNode->m_Sides[0], g_pVisFiltered_D, g_nVisFiltered_D);
			}
			else
			{
				pArray = g_pVisFiltered_D;
				nCount = g_nVisFiltered_D;
				pNode  = pNode->m_Sides[0];
				continue;
			}
		}

		if (bBack && vis_ReadBit_D())
		{
			pArray = vis_FilterPos_D(pNode->GetPlane(), pArray, nCount, &nCount);
			pNode  = pNode->m_Sides[1];
			continue;
		}

		return;
	}
}

// 0x10039151 — the next bit of the PVS stream.
uint8 vis_ReadBit_D()
{
	uint8 bBit = (uint8)((1 << (g_nVisBit_D & 7)) & g_pVisBits_D[g_nVisBit_D >> 3]);
	g_nVisBit_D++;
	return bBit;
}

// 0x10039177 — keep the objects on the negative side (lithtech 0x0048bca0).
CVisObj_D** vis_FilterNeg_D(LTPlane* pPlane, CVisObj_D** pIn, uint32 nIn,
                            uint32* pnOut, LTBOOL bAppend)
{
	CVisObj_D** pOut;

	pOut = pIn;

	if (bAppend)
		pOut += nIn;

	*pnOut = 0;

	if (pOut + nIn < g_pVisLimit_D)
	{
		while (nIn--)
		{
			CVisObj_D* pObj = *pIn;

			if (pObj->m_nMark != g_pVisQuery_D->m_nMark)
			{
				if (pPlane->DistTo(pObj->m_Pos) < pObj->m_Radius + 0.1f)
				{
					pOut[*pnOut] = pObj;
					(*pnOut)++;
				}
			}

			pIn++;
		}
	}

	return pOut;
}

// 0x10039211 — and the positive side (lithtech 0x0048bd70).
CVisObj_D** vis_FilterPos_D(LTPlane* pPlane, CVisObj_D** pIn, uint32 nIn,
                            uint32* pnOut)
{
	CVisObj_D** pOut;

	pOut = pIn;

	*pnOut = 0;

	if (pOut + nIn >= g_pVisLimit_D)
		return pOut;

	while (nIn--)
	{
		CVisObj_D* pObj = *pIn;

		if (pObj->m_nMark != g_pVisQuery_D->m_nMark)
		{
			if (pPlane->DistTo(pObj->m_Pos) > -(pObj->m_Radius + 0.1f))
			{
				pOut[*pnOut] = pObj;
				(*pnOut)++;
			}
		}

		pIn++;
	}

	return pOut;
}


// ---- the RIFF/WAVE reader (lttexmod.cpp's, verbatim in shape) ----------- //

#define WAVE_FOURCC_D(c0, c1, c2, c3)                           \
	((uint32)(uint8)(c0) | ((uint32)(uint8)(c1) << 8) |         \
	 ((uint32)(uint8)(c2) << 16) | ((uint32)(uint8)(c3) << 24))

#define FOURCC_RIFF_D WAVE_FOURCC_D('R', 'I', 'F', 'F')
#define FOURCC_LIST_D WAVE_FOURCC_D('L', 'I', 'S', 'T')
#define FOURCC_WAVE_D WAVE_FOURCC_D('W', 'A', 'V', 'E')
#define FOURCC_FMT_D  WAVE_FOURCC_D('f', 'm', 't', ' ')
#define FOURCC_DATA_D WAVE_FOURCC_D('d', 'a', 't', 'a')
#define FOURCC_FACT_D WAVE_FOURCC_D('f', 'a', 'c', 't')
#define FOURCC_CUE_D  WAVE_FOURCC_D('c', 'u', 'e', ' ')
#define FOURCC_ADTL_D WAVE_FOURCC_D('a', 'd', 't', 'l')
#define FOURCC_LTXT_D WAVE_FOURCC_D('l', 't', 'x', 't')
#define FOURCC_LITH_D WAVE_FOURCC_D('l', 'i', 't', 'h')
#define FOURCC_INFO_D WAVE_FOURCC_D('i', 'n', 'f', 'o')

#define WAVEFMT_PCM_D       0x0001
#define WAVEFMT_IMAADPCM_D  0x0011
#define WAVEFMT_MP3_D       0x0055

#pragma pack(push, 1)
struct WaveFormatEx_D
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

struct WaveInfo_D
{
	WaveFormatEx_D m_Format;                 // 0x00
	uint32         m_nDataOffset;            // 0x14
	uint32         m_nDataSize;              // 0x18
	uint32         m_nSampleCount;           // 0x1c
	uint32         m_nLithFlags;             // 0x20
	uint32         m_nLithUnknown24;         // 0x24
	uint32         m_bHasLoop;               // 0x28
	uint32         m_nLoopName;              // 0x2c
	uint32         m_nLoopStart;             // 0x30
	uint32         m_nLoopLength;            // 0x34
};

struct WaveChunkData_D
{
	uint32 m_dwName;                         // 0x00
	uint32 m_dwPosition;                     // 0x04
	uint32 m_fccChunk;                       // 0x08
	uint32 m_dwChunkStart;                   // 0x0c
	uint32 m_dwBlockStart;                   // 0x10
	uint32 m_dwSampleOffset;                 // 0x14
};

// RIFF's labelled-text chunk header, the first twenty bytes of 'ltxt'.
struct WaveLabelText_D
{
	uint32 m_dwName;                         // 0x00
	uint32 m_dwSampleLength;                 // 0x04
	uint32 m_dwPurpose;                      // 0x08
	uint16 m_wCountry;                       // 0x0c
	uint16 m_wLanguage;                      // 0x0e
	uint16 m_wDialect;                       // 0x10
	uint16 m_wCodePage;                      // 0x12
};

LTBOOL wav_ParseChunk_D(ILTStream* pStream, uint32 chunkID, uint32 chunkSize,
                        uint32 formType, WaveInfo_D* pInfo);         // 0x10039341

// 0x100392a9 — the whole file (lithtech 0x0048c1a0).
LTBOOL wav_ParseFile_D(ILTStream* pStream, WaveInfo_D* pInfo)
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

		if (!wav_ParseChunk_D(pStream, chunkID, chunkSize, 0, pInfo))
			break;
	}

	return (pInfo->m_Format.wFormatTag == WAVEFMT_PCM_D ||
	        pInfo->m_Format.wFormatTag == WAVEFMT_IMAADPCM_D ||
	        pInfo->m_Format.wFormatTag == WAVEFMT_MP3_D);
}

// 0x10039341 — one chunk, recursive (lithtech 0x0048c270).
LTBOOL wav_ParseChunk_D(ILTStream* pStream, uint32 chunkID, uint32 chunkSize,
                        uint32 formType, WaveInfo_D* pInfo)
{
	uint32 nRead;

	uint32 endPos = pStream->GetPos() + chunkSize;
	if (endPos & 1)
		endPos++;
	if (endPos > pStream->GetLen())
		endPos = pStream->GetLen();

	switch (chunkID)
	{
		case FOURCC_RIFF_D:
		case FOURCC_LIST_D:
		{
			uint32 subFormType, subChunkID, subChunkSize;

			pStream->Read(&subFormType, sizeof(subFormType));

			while (pStream->GetPos() < endPos)
			{
				pStream->Read(&subChunkID, sizeof(subChunkID));
				pStream->Read(&subChunkSize, sizeof(subChunkSize));

				if (!wav_ParseChunk_D(pStream, subChunkID, subChunkSize,
				                      subFormType, pInfo))
					break;
			}
			break;
		}

		case FOURCC_FMT_D:
		{
			if (formType != FOURCC_WAVE_D)
				break;

			nRead = sizeof(WaveFormatEx_D);
			if (chunkSize < nRead)
				nRead = chunkSize;
			pStream->Read(&pInfo->m_Format, nRead);
			break;
		}

		case FOURCC_DATA_D:
		{
			if (formType != FOURCC_WAVE_D)
				break;

			pInfo->m_nDataOffset = pStream->GetPos();
			pInfo->m_nDataSize   = chunkSize;

			if (pInfo->m_Format.wFormatTag == WAVEFMT_IMAADPCM_D)
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

		case FOURCC_FACT_D:
		{
			if (formType != FOURCC_WAVE_D)
				break;
			if (pInfo->m_Format.wFormatTag != WAVEFMT_IMAADPCM_D)
				break;

			pStream->Read(&pInfo->m_nSampleCount,
			              sizeof(pInfo->m_nSampleCount));
			break;
		}

		case FOURCC_CUE_D:
		{
			WaveChunkData_D chunkData;
			uint32          nCuePoints, nLeft;

			pStream->Read(&nCuePoints, sizeof(nCuePoints));
			nLeft = chunkSize - sizeof(nCuePoints);
			if (nCuePoints > 0)
			{

				nRead = sizeof(chunkData);
				if (chunkSize < nLeft)
					nRead = nLeft;
				pStream->Read(&chunkData, nRead);

				if (chunkData.m_fccChunk == FOURCC_DATA_D)
				{
					pInfo->m_bHasLoop    = 1;
					pInfo->m_nLoopName   = chunkData.m_dwName;
					pInfo->m_nLoopStart  = chunkData.m_dwPosition;
				}
			}
			break;
		}

		case FOURCC_LTXT_D:
		{
			WaveLabelText_D labelData;

			if (formType != FOURCC_ADTL_D)
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

		case FOURCC_LITH_D:
		{
			if (formType != FOURCC_INFO_D)
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


// ---- the world tree's object base and node (worldtree2.cpp's) ---------- //

#define WT_NUMLISTS_D   3
#define WT_NUMLINKS_D   5
#define WT_NUMKIDS_D    4

class CWTNode_D;
class CWorldTree_D;
struct WTFindInfo_D;

// CheapLTLink plus m_pData rather than LTLink: LTLink's (empty, inline)
// constructor makes an array of these call ??_H at /O1, where 0x10039707
// ties the five links off in line.
class CWTLink_D : public CheapLTLink
{
public:
	void*      m_pData;                      // +0x08
	CWTNode_D* m_pNode;                      // +0x0c
};

// vtable 0x1004ea04; objtypes.h's CObjRoot.
class CWTObj_D
{
public:
	CWTObj_D(uint32 nObjType);                                // 0x10039707
	virtual ~CWTObj_D();                                      // 0x10039735

	virtual LTBOOL     WantsTree(CWorldTree_D *pTree)     { return LTFALSE; }
	virtual void       RemoveFromLists();                     // 0x10039740
	virtual void       GetDims(LTVector *pMin, LTVector *pMax) = 0;
	virtual LTBOOL     WantsExtraLink()                   { return LTFALSE; }
	virtual CWTLink_D* MakeExtraLink(uint32 nIndex)       { return LTNULL; }
	virtual void       NotifyInserted(CWTObj_D *pObj)     {}
	virtual void       OnFoundInBox(WTFindInfo_D *pInfo)  {}

public:
	CWTLink_D m_Links[WT_NUMLINKS_D];        // +0x04
	uint32    m_nObjType;                    // +0x54
	uint32    m_nTouchCode;                  // +0x58
};

class CWTNode_D
{
public:
	CWTNode_D();                                              // 0x10039780
	~CWTNode_D();                                             // 0x1003978c

	void   Init();                                            // 0x10039791
	void   Term();                                            // 0x100398b7
	void   FreeChildren();                                    // 0x100398f3

	void   GetDims(LTVector *pMin, LTVector *pMax);           // 0x100397ec
	void   SetDims(LTVector vMin, LTVector vMax);             // 0x10039807

	LTBOOL AllocChildren();                                   // 0x100399a5
	LTBOOL LoadNodes(ILTStream *pStream,
	                 uint8 *pCurByte, uint8 *pCurBit);        // 0x1003991d

public:
	CheapLTLink m_Objects[WT_NUMLISTS_D];    // +0x00
	LTVector    m_vBoxMin;                   // +0x18
	LTVector    m_vBoxMax;                   // +0x24
	LTVector    m_vCenter;                   // +0x30
	float       m_fSmallestDim;              // +0x3c
	float       m_fRadius;                   // +0x40
	uint32      m_nNumObjects;               // +0x44
	CWTNode_D*  m_pParent;                   // +0x48
	CWTNode_D*  m_Children[WT_NUMKIDS_D];    // +0x4c
};

typedef void (*WTBoxCallback_D)(CWTObj_D *pObj, void *pUserData);
typedef int  (*WTNodeTest_D)(CWTNode_D *pNode);

class CWorldTree_D
{
public:
	void*     m_pVTable;                     // +0x00
	void*     m_pCodeSource;                 // +0x04
	uint32    m_nCurCode;                    // +0x08
};

struct WTFindInfo_D
{
	uint32          m_nListIndex;            // +0x00
	LTVector        m_vCenter;               // +0x04
	float           m_fRadius;               // +0x10
	WTBoxCallback_D m_pfnCallback;           // +0x14
	uint32          m_n018;                  // +0x18
	void*           m_pUserData;             // +0x1c
	uint32          m_n020;                  // +0x20
	uint32          m_n024;                  // +0x24
	WTNodeTest_D    m_pfnNodeTest;           // +0x28
	LTVector        m_vBoxMin;               // +0x2c
	LTVector        m_vBoxMax;               // +0x38
	CWorldTree_D*   m_pTree;                 // +0x44
};

// 0x100395a4 — the recursive box query (lithtech 0x0048c540).
void wt_FindObjectsInBox_D(CWTNode_D *pNode, WTFindInfo_D *pInfo, uint32 nLevel)
{
	CheapLTLink *pHead;
	LTLink      *pCur;
	CWTObj_D    *pObj;
	LTVector     vMin, vMax;

	if (!pInfo->m_pfnNodeTest(pNode))
		return;

	pHead = &pNode->m_Objects[2];
	if (pHead->m_pNext != (LTLink*)pHead)
	{
		pCur = pHead->m_pNext;
		while (pCur != (LTLink*)pHead)
		{
			pObj = (CWTObj_D*)pCur->m_pData;
			pCur = pCur->m_pNext;

			if (pObj->m_nTouchCode != pInfo->m_pTree->m_nCurCode)
			{
				pObj->m_nTouchCode = pInfo->m_pTree->m_nCurCode;
				pObj->OnFoundInBox(pInfo);
			}
		}

		return;
	}

	pHead = &pNode->m_Objects[pInfo->m_nListIndex];
	pCur  = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj = (CWTObj_D*)pCur->m_pData;
		pCur = pCur->m_pNext;

		if (pObj->m_nTouchCode != pInfo->m_pTree->m_nCurCode)
		{
			pObj->m_nTouchCode = pInfo->m_pTree->m_nCurCode;
			pObj->GetDims(&vMin, &vMax);

			if (vMin.x > pInfo->m_vBoxMax.x) continue;
			if (vMin.y > pInfo->m_vBoxMax.y) continue;
			if (vMin.z > pInfo->m_vBoxMax.z) continue;
			if (vMax.x < pInfo->m_vBoxMin.x) continue;
			if (vMax.y < pInfo->m_vBoxMin.y) continue;
			if (vMax.z < pInfo->m_vBoxMin.z) continue;

			pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}

	if (pNode->m_Children[0])
	{
		if (pInfo->m_vBoxMin.x < pNode->m_vCenter.x)
		{
			if (pInfo->m_vBoxMin.z < pNode->m_vCenter.z)
				wt_FindObjectsInBox_D(pNode->m_Children[0], pInfo, nLevel + 1);

			if (pInfo->m_vBoxMax.z > pNode->m_vCenter.z)
				wt_FindObjectsInBox_D(pNode->m_Children[1], pInfo, nLevel + 1);
		}

		if (pInfo->m_vBoxMax.x > pNode->m_vCenter.x)
		{
			if (pInfo->m_vBoxMin.z < pNode->m_vCenter.z)
				wt_FindObjectsInBox_D(pNode->m_Children[2], pInfo, nLevel + 1);

			if (pInfo->m_vBoxMax.z > pNode->m_vCenter.z)
				wt_FindObjectsInBox_D(pNode->m_Children[3], pInfo, nLevel + 1);
		}
	}
}

// 0x10039707 — objtypes.h's CObjRoot::CObjRoot(int).
CWTObj_D::CWTObj_D(uint32 nObjType)
{
	uint32 i;

	for (i = 0; i < WT_NUMLINKS_D; i++)
	{
		m_Links[i].m_pNode = LTNULL;
		m_Links[i].TieOff();
		m_Links[i].m_pData = this;
	}

	m_nTouchCode = 0xffffffff;
	m_nObjType   = nObjType;
}

// 0x10039735 — the destructor: vptr store and a tail jump to 0x10039740
// (objtypes.h's CObjBase::BaseTerm).
CWTObj_D::~CWTObj_D()
{
	RemoveFromLists();
}

// 0x10039740 — slot 2 (objtypes.h's CObjBase::Unlink).
void CWTObj_D::RemoveFromLists()
{
	uint32 i;

	m_nTouchCode = 0xffffffff;

	for (i = 0; i < WT_NUMLINKS_D; i++)
	{
		CWTLink_D* pLink = &m_Links[i];

		pLink->Remove();

		while (pLink->m_pNode)
		{
			pLink->m_pNode->m_nNumObjects--;
			pLink->m_pNode = pLink->m_pNode->m_pParent;
		}
	}
}

// 0x10039780
CWTNode_D::CWTNode_D()
{
	Init();
}

// 0x1003978c — `jmp 0x100398b7`, five bytes.
CWTNode_D::~CWTNode_D()
{
	Term();
}

// 0x10039791. The four `stos` into m_Children are a four-trip zeroing loop
// (the client twin, worldtree2.cpp's CWTNodeN48C::Init, is the same loop
// unrolled at /O2). An intrinsic memset emitted the same stores but took the
// `xor %eax,%eax` before `lea 0x4c(%ecx),%edi`; the loop gives shipped order.
void CWTNode_D::Init()
{
	uint32 i;

	m_pParent     = LTNULL;
	m_nNumObjects = 0;

	for (i = 0; i < WT_NUMLISTS_D; i++)
		m_Objects[i].TieOff();

	for (i = 0; i < WT_NUMKIDS_D; i++)
		m_Children[i] = LTNULL;

	m_vCenter.Init();
	m_vBoxMin.Init();
	m_vBoxMax.Init();

	m_fSmallestDim = 0.0f;
	m_fRadius      = 0.0f;
}

// 0x100397ec
void CWTNode_D::GetDims(LTVector *pMin, LTVector *pMax)
{
	*pMin = m_vBoxMin;
	*pMax = m_vBoxMax;
}

// 0x10039807
void CWTNode_D::SetDims(LTVector vMin, LTVector vMax)
{
	LTVector vHalfDims;

	m_vBoxMin = vMin;
	m_vBoxMax = vMax;

	vHalfDims = (vMax - vMin) * 0.5f;
	m_vCenter = vMin + vHalfDims;

	m_fSmallestDim = LTMIN(vMax.x - vMin.x, vMax.z - vMin.z);
	m_fRadius      = vHalfDims.Mag();
}

// 0x100398b7
void CWTNode_D::Term()
{
	LTLink   *pCur, *pNext;
	CWTObj_D *pObj;
	uint32    i;

	FreeChildren();

	for (i = 0; i < WT_NUMLISTS_D; i++)
	{
		pCur = m_Objects[i].m_pNext;
		while (pCur != (LTLink*)&m_Objects[i])
		{
			pNext = pCur->m_pNext;
			pObj  = (CWTObj_D*)pCur->m_pData;
			pObj->RemoveFromLists();
			pCur  = pNext;
		}
	}

	Init();
}

// 0x100398f3
void CWTNode_D::FreeChildren()
{
	uint32 i;

	for (i = 0; i < WT_NUMKIDS_D; i++)
	{
		if (m_Children[i])
		{
			delete m_Children[i];
			m_Children[i] = LTNULL;
		}
	}
}

// 0x1003991d
LTBOOL CWTNode_D::LoadNodes(ILTStream *pStream, uint8 *pCurByte, uint8 *pCurBit)
{
	LTBOOL bHasChildren;
	uint32 i;

	FreeChildren();

	if (*pCurBit == 8)
	{
		pStream->Read(pCurByte, 1);
		*pCurBit = 0;
	}

	bHasChildren = ((*pCurByte & (1 << *pCurBit)) != 0);
	(*pCurBit)++;

	if (bHasChildren)
	{
		if (!AllocChildren())
			return LTFALSE;

		for (i = 0; i < WT_NUMKIDS_D; i++)
		{
			if (!m_Children[i]->LoadNodes(pStream, pCurByte, pCurBit))
			{
				FreeChildren();
				return LTFALSE;
			}
		}
	}

	return LTTRUE;
}

// ==== END SECTION D =======================================================

// ==== SECTION E: 0x100399a5 .. 0x10039ec1 ================================
//
// The node's AllocChildren/AddObjectLink, the tree class (ctor, Term,
// FindNodeFromPath, AddObject x2, the two query slots), the insert walk and
// the quadrant helper it recurses through.  Client twins: worldtree2.cpp
// (0x0048ca50, 0x0048cca0, 0x0048cd10..0x0048cee0), worldtree.cpp's tree
// class (vtable 0x004a38f8), lt2new4.cpp (0x0048cce0/0x0048cd50) and
// lt2new2.cpp (0x0048d070).  Server vtable of the tree: 0x1004ea24, seven
// slots: 0x10039be9 0x10039e18 0x10039e9e 0x10039f82 0x10039f9d 0x1003a386
// 0x10039b9f.
//
// The node constructor (0x10039780), SetDims (0x10039807) and Term
// (0x100398b7) are SECTION D's; they are only declared here.

class CWTNode_E;
class CWorldTree_E;
struct WTInsertInfo_E;

class CWTLink_E : public LTLink
{
public:
	CWTNode_E* m_pNode;                      // +0x0c
};

// The object base (client vtable 0x004a38d8).  Only the slots called here.
class CWTObj_E
{
public:
	virtual void         _v000() = 0;
	virtual LTBOOL       WantsTree(CWorldTree_E *pTree) = 0;          // +0x04
	virtual void         RemoveFromLists() = 0;                        // +0x08
	virtual void         GetDims(LTVector *pMin, LTVector *pMax) = 0;  // +0x0c
	virtual LTBOOL       WantsExtraLink() = 0;                         // +0x10
	virtual CWTLink_E*   MakeExtraLink(uint32 nIndex) = 0;             // +0x14
	virtual void         NotifyInserted(CWTObj_E *pObj) = 0;           // +0x18

	CWTLink_E m_Links[5];                    // +0x04
	uint32    m_nObjType;                    // +0x54
	uint32    m_nTouchCode;                  // +0x58
};

class CWTNode_E
{
public:
	CWTNode_E();                                              // 0x10039780 (D)
	void   Term();                                            // 0x100398b7 (D)
	void   SetDims(LTVector vMin, LTVector vMax);             // 0x10039807 (D)

	LTBOOL AllocChildren();                                   // 0x100399a5
	void   AddObjectLink(CWTLink_E *pLink, uint32 nList);     // 0x10039b44

public:
	CheapLTLink  m_Objects[3];               // +0x00
	LTVector     m_vBoxMin;                  // +0x18
	LTVector     m_vBoxMax;                  // +0x24
	LTVector     m_vCenter;                  // +0x30
	float        m_fSmallestDim;             // +0x3c
	float        m_fRadius;                  // +0x40
	uint32       m_nNumObjects;              // +0x44
	CWTNode_E*   m_pParent;                  // +0x48
	CWTNode_E*   m_Children[4];              // +0x4c
};                                           // 0x5c

struct WTInsertInfo_E
{
	CWTObj_E*    m_pObject;                  // +0x00
	uint32       m_nListIndex;               // +0x04
	LTVector     m_vBoxMin;                  // +0x08
	LTVector     m_vBoxMax;                  // +0x14
	float        m_fMaxDim;                  // +0x20
	uint32       m_nNumLinks;                // +0x24
};

struct WTPath_E
{
	uint8  m_Bytes[4];                       // +0x00
	uint32 m_nDepth;                         // +0x04
};

// The query slot 1 builds and slot 2 completes; 0x10039e61 is its
// constructor.  Section F's 0x10039ec1 walks it.  +0x20 is a vector:
// three separate float members let VC6 hoist the +0x2c/+0x30 integer
// stores above the last fldz/fstps pairs; three Init()s keep them last.
struct WTSegQuery_E
{
	WTSegQuery_E();                                           // 0x10039e61

	CWorldTree_E* m_pTree;                   // +0x00  set by slot 2
	uint32        m_nListIndex;              // +0x04
	LTVector      m_vPt1;                    // +0x08
	LTVector      m_vPt2;                    // +0x14
	LTVector      m_v20;                     // +0x20
	void*         m_pfnCallback;             // +0x2c
	void*         m_pUserData;               // +0x30
};

class IWTCodeSource_E
{
public:
	virtual uint32 NextCode() = 0;           // slot 0
	virtual uint32 Slot1() = 0;              // slot 1
};

class CWorldTree_E
{
public:
	CWorldTree_E();                                           // 0x10039b74
	uint32     CodeSourceSlot1();                             // 0x10039ba2
	void       SetCodeSource(IWTCodeSource_E *pSource);       // 0x10039baa
	void       Term();                                        // 0x10039bbc

	void       AddObject(CWTObj_E *pObj, uint32 nListIndex);  // 0x10039c43
	void       AddObject(CWTObj_E *pObj, LTVector *pMin,
	                     LTVector *pMax, uint32 nListIndex);  // 0x10039c78

	virtual CWTNode_E* FindNodeFromPath(WTPath_E *pPath);     // 0x10039be9
	virtual uint32     IntersectSegment(LTVector *pPt1, LTVector *pPt2,
	                                    void *pfnCallback, void *pUserData,
	                                    uint32 nListIndex);   // 0x10039e18
	virtual uint32     DoSegQuery(WTSegQuery_E *pQuery);      // 0x10039e9e
	virtual void       _v00c() = 0;                           // 0x10039f82 (F)
	virtual void       _v010() = 0;                           // 0x10039f9d (F)
	virtual void       _v014() = 0;                           // 0x1003a386 (F)
	virtual void       _v018(void *a, void *b) {}             // 0x10039b9f

public:
	IWTCodeSource_E* m_pCodeSource;          // +0x04
	uint32           m_nCurCode;             // +0x08
	CWTNode_E        m_RootNode;             // +0x0c
	void*        m_p068;                     // +0x68
	void*        m_p06c;                     // +0x6c
	void*        m_p070;                     // +0x70
	CheapLTLink  m_RootObjects;              // +0x74
};

typedef void (*WTNodeCallback_E)(CWTNode_E *pNode, void *pUser);

void wt_InsertObject_E(CWTNode_E *pNode, WTInsertInfo_E *pInfo);
static void wt_ForEachChildInBox_E(LTVector *pMin, LTVector *pMax,
                                   CWTNode_E *pNode, WTNodeCallback_E pfn,
                                   void *pUser);
uint32 wt_SegQueryNode_E(CWTNode_E *pNode, WTSegQuery_E *pQuery);  // 0x10039ec1 (F)


// 0x100399a5, 415 bytes -- lithtech 0x0048ca50.
LTBOOL CWTNode_E::AllocChildren()
{
	LTVector vHalfDims;
	uint32   i;

	for (i = 0; i < 4; i++)
	{
		m_Children[i] = new CWTNode_E;
		if (!m_Children[i])
		{
			Term();
			return LTFALSE;
		}

		m_Children[i]->m_pParent = this;
	}

	vHalfDims = (m_vBoxMax - m_vBoxMin) * 0.5f;

	m_Children[0]->SetDims(
		LTVector(m_vCenter.x - vHalfDims.x, m_vBoxMin.y, m_vCenter.z - vHalfDims.z),
		LTVector(m_vCenter.x,               m_vBoxMax.y, m_vCenter.z));

	m_Children[1]->SetDims(
		LTVector(m_vCenter.x - vHalfDims.x, m_vBoxMin.y, m_vCenter.z),
		LTVector(m_vCenter.x,               m_vBoxMax.y, m_vCenter.z + vHalfDims.z));

	m_Children[2]->SetDims(
		LTVector(m_vCenter.x,               m_vBoxMin.y, m_vCenter.z - vHalfDims.z),
		LTVector(m_vCenter.x + vHalfDims.x, m_vBoxMax.y, m_vCenter.z));

	m_Children[3]->SetDims(
		LTVector(m_vCenter.x,               m_vBoxMin.y, m_vCenter.z),
		LTVector(m_vCenter.x + vHalfDims.x, m_vBoxMax.y, m_vCenter.z + vHalfDims.z));

	return LTTRUE;
}


// 0x10039b44, 48 bytes -- lithtech 0x0048cca0.
void CWTNode_E::AddObjectLink(CWTLink_E *pLink, uint32 nList)
{
	CWTNode_E *pNode;

	m_Objects[nList].AddAfter(pLink);
	pLink->m_pNode = this;

	pNode = this;
	while (pNode)
	{
		pNode->m_nNumObjects++;
		pNode = pNode->m_pParent;
	}
}


// 0x10039b74, 43 bytes -- lithtech 0x0048cce0 (lt2new4.cpp).  The root node
// is a MEMBER, not a base (lt2new4.cpp models it as a second base): the
// `lea 0xc(%ecx)` conversions in FindNodeFromPath/AddObject/DoSegQuery
// carry no neg/sbb null test, and as a member the node's constructor
// still comes before the vptr store, byte-identical.  The in-class empty
// slot 6 (0x10039b9f, `ret $8`, 3 bytes: "small") follows it.
CWorldTree_E::CWorldTree_E()
{
	m_pCodeSource = LTNULL;
	m_p068 = LTNULL;
	m_p06c = LTNULL;
	m_p070 = LTNULL;

	m_RootObjects.TieOff();
}


// 0x10039ba2, 8 bytes -- lithtech 0x0048cd20; tail jump.
uint32 CWorldTree_E::CodeSourceSlot1()
{
	return m_pCodeSource->Slot1();
}


// 0x10039baa, 18 bytes -- lithtech 0x0048cd30.
void CWorldTree_E::SetCodeSource(IWTCodeSource_E *pSource)
{
	m_pCodeSource = pSource;
	m_RootObjects.TieOff();
}


// 0x10039bbc, 45 bytes -- lithtech 0x0048cd50 (lt2new4.cpp).
void CWorldTree_E::Term()
{
	m_RootNode.Term();

	m_p068 = LTNULL;
	m_p06c = LTNULL;

	m_RootObjects.Remove();
}


// 0x10039be9, 90 bytes -- vtable slot 0, lithtech 0x0048cd80.
CWTNode_E* CWorldTree_E::FindNodeFromPath(WTPath_E *pPath)
{
	CWTNode_E *pNode;
	uint32     nBit, i, nByte, nQuadrant;

	pNode = &m_RootNode;

	nByte = 0;
	i     = 0;
	nBit  = 0;

	for (; i != pPath->m_nDepth; i++)
	{
		if (!pNode->m_Children[0])
			return LTNULL;

		nQuadrant = (pPath->m_Bytes[nByte] >> nBit) & 3;
		nBit += 2;

		if (nBit == 8)
		{
			nBit = 0;
			nByte++;

			if (nByte >= 4)
				return LTNULL;
		}

		pNode = pNode->m_Children[nQuadrant];
	}

	return pNode;
}


// 0x10039c43, 53 bytes -- lithtech 0x0048cde0.
void CWorldTree_E::AddObject(CWTObj_E *pObj, uint32 nListIndex)
{
	LTVector vMin, vMax;

	pObj->GetDims(&vMin, &vMax);
	AddObject(pObj, &vMin, &vMax, nListIndex);
}


// 0x10039c78, 136 bytes -- lithtech 0x0048ce20.
void CWorldTree_E::AddObject(CWTObj_E *pObj, LTVector *pMin,
                             LTVector *pMax, uint32 nListIndex)
{
	WTInsertInfo_E info;
	LTVector       vDims;

	pObj->RemoveFromLists();

	vDims = *pMax - *pMin;

	info.m_pObject    = pObj;
	info.m_nListIndex = nListIndex;
	info.m_vBoxMin    = *pMin;
	info.m_vBoxMax    = *pMax;
	info.m_fMaxDim = LTMAX(vDims.x, vDims.z);

	info.m_nNumLinks = 0;

	if (!pObj->WantsTree(this))
		wt_InsertObject_E(&m_RootNode, &info);
}


// 0x10039d00, 152 bytes -- lithtech 0x0048cee0, where the helper below is
// inlined and its callback turned into direct (and one tail) recursion.
void wt_InsertObject_E(CWTNode_E *pNode, WTInsertInfo_E *pInfo)
{
	CWTLink_E *pLink;

	if (pNode->m_Objects[2].m_pNext != (LTLink*)&pNode->m_Objects[2])
		((CWTObj_E*)pNode->m_Objects[2].m_pNext->m_pData)->NotifyInserted(pInfo->m_pObject);

	if (pNode->m_fSmallestDim * 0.5f > pInfo->m_fMaxDim && pNode->m_Children[0])
	{
		wt_ForEachChildInBox_E(&pInfo->m_vBoxMin, &pInfo->m_vBoxMax, pNode,
		                       (WTNodeCallback_E)wt_InsertObject_E, pInfo);
	}
	else if (pInfo->m_nNumLinks < 5)
	{
		pNode->AddObjectLink(&pInfo->m_pObject->m_Links[pInfo->m_nNumLinks],
		                     pInfo->m_nListIndex);

		if (pInfo->m_pObject->WantsExtraLink())
		{
			pLink = pInfo->m_pObject->MakeExtraLink(pInfo->m_nNumLinks);
			pNode->m_Objects[2].AddAfter(pLink);
		}

		pInfo->m_nNumLinks++;
	}
}


// 0x10039d98, 128 bytes -- cdecl, five arguments; no standalone twin in
// lithtech.exe (inlined into 0x0048cee0).
static void wt_ForEachChildInBox_E(LTVector *pMin, LTVector *pMax,
                                   CWTNode_E *pNode, WTNodeCallback_E pfn,
                                   void *pUser)
{
	if (pMin->x < pNode->m_vCenter.x)
	{
		if (pMin->z < pNode->m_vCenter.z)
			pfn(pNode->m_Children[0], pUser);

		if (pMax->z > pNode->m_vCenter.z)
			pfn(pNode->m_Children[1], pUser);
	}

	if (pMax->x > pNode->m_vCenter.x)
	{
		if (pMin->z < pNode->m_vCenter.z)
			pfn(pNode->m_Children[2], pUser);

		if (pMax->z > pNode->m_vCenter.z)
			pfn(pNode->m_Children[3], pUser);
	}
}


// 0x10039e18, 73 bytes -- vtable slot 1, lithtech 0x0048cff0.
uint32 CWorldTree_E::IntersectSegment(LTVector *pPt1, LTVector *pPt2,
                                      void *pfnCallback, void *pUserData,
                                      uint32 nListIndex)
{
	WTSegQuery_E query;

	query.m_nListIndex  = nListIndex;
	query.m_vPt1        = *pPt1;
	query.m_vPt2        = *pPt2;
	query.m_pfnCallback = pfnCallback;
	query.m_pUserData   = pUserData;

	return DoSegQuery(&query);
}


// 0x10039e61, 61 bytes -- inlined into lithtech 0x0048cff0.
WTSegQuery_E::WTSegQuery_E()
{
	m_pTree      = LTNULL;
	m_nListIndex = 0;
	m_vPt1.Init();
	m_vPt2.Init();
	m_v20.Init();
	m_pfnCallback = LTNULL;
	m_pUserData   = LTNULL;
}


// 0x10039e9e, 35 bytes -- vtable slot 2, lithtech 0x0048d070 (lt2new2.cpp
// "Slot10").
uint32 CWorldTree_E::DoSegQuery(WTSegQuery_E *pQuery)
{
	m_nCurCode = m_pCodeSource->NextCode();

	pQuery->m_pTree = this;

	return wt_SegQueryNode_E(&m_RootNode, pQuery);
}

// ==== END SECTION E =======================================================

// ==== SECTION F: 0x10039ec1 .. 0x1003a54f ================================
//
// The tail of the world-tree module: the rectangle walk, the point and
// segment queries (vtable 0x1004ea24 slots 3 and 4), the sphere query
// (slot 5), the layout loader and the root-list insert -- then, after the
// int3 object boundary at 0x1003a529, the first function of rezmgr.lib.
//
// The tree's vtable, 0x1004ea24, seven slots:
//   0 0x10039be9   1 0x10039e18 FindObjectsInBox(min, max, cb, user, list)
//   2 0x10039e9e FindObjectsInBox(info)   3 0x10039f82 FindObjectsOnPoint
//   4 0x10039f9d FindObjectsOnSegment     5 0x1003a386 FindObjectsInSphere
//   6 0x10039b9f `ret $8`
// Slots 4 and 5 are the client's 0x0048d210 / 0x0048d810
// (src/engine/lithtech/worldtree.cpp), and the node/object layouts are the
// client's (src/engine/lithtech/worldtree2.cpp) offset for offset. At /O1
// the segment test's helpers are NOT inlined, which is what splits the
// client's 0x0048d3a0/0x0048d500 into 0x1003a0e3, 0x1003a1d1, 0x1003a22b
// and 0x1003a28b here.

#include <math.h>                // fabs -- a CRT call at /O1 (0x1003e35b)
#include "ltlink.h"
#include "objtypes.h"            // CObjVisitor, CObjBase (0x1003a509)

class CWTNode_F;
class CWorldTree_F;

// The link an object is threaded into a node with (0x10039707's loop).
class CWTLink_F : public LTLink
{
public:
	CWTNode_F* m_pNode;                      // +0x0c
};

// The world-tree object base (0x10039707 is its constructor).
class CWTObj_F
{
public:
	virtual ~CWTObj_F() = 0;
	virtual LTBOOL _v04(void* p) = 0;
	virtual void   RemoveFromLists() = 0;                    // 0x10039740
	virtual void   GetDims(LTVector* pMin, LTVector* pMax) = 0;

	CWTLink_F m_Links[5];                    // +0x04
	uint32    m_nObjType;                    // +0x54
	uint32    m_nTouchCode;                  // +0x58
};

class CWTNode_F
{
public:
	void   SetDims(LTVector vMin, LTVector vMax);                   // 0x10039807
	LTBOOL LoadNodes(ILTStream* pStream, uint8* pCurByte,
	                 uint8* pCurBit);                               // 0x1003991d

	CheapLTLink m_Objects[3];                // +0x00
	LTVector    m_vBoxMin;                   // +0x18
	LTVector    m_vBoxMax;                   // +0x24
	LTVector    m_vCenter;                   // +0x30
	float       m_fSmallestDim;              // +0x3c
	float       m_fRadius;                   // +0x40
	uint32      m_nNumObjects;               // +0x44
	CWTNode_F*  m_pParent;                   // +0x48
	CWTNode_F*  m_Children[4];               // +0x4c
};

class IWTCodeSource_F
{
public:
	virtual uint32 NextCode() = 0;
};

typedef void   (*WTBoxCallback_F)(CWTObj_F* pObj, void* pUser);
typedef uint32 (*WTSegCallback_F)(CWTObj_F* pObj, void* pUser);
typedef void   (*WTNodeFn_F)(CWTNode_F* pNode, void* pUser);
typedef int    (*WTNodeTest_F)(CWTNode_F* pNode);

// 0x10039e61 builds this (0x34 bytes); 0x10039ec1 consumes it.
struct WTRectInfo_F
{
	CWorldTree_F*   m_pTree;                 // +0x00
	uint32          m_nListIndex;            // +0x04
	LTVector        m_vMin;                  // +0x08
	LTVector        m_vMax;                  // +0x14
	LTVector        m_v20;                   // +0x20
	WTBoxCallback_F m_pfnCallback;           // +0x2c
	void*           m_pUserData;             // +0x30
};

// 0x10039f9d builds this on the stack (0x28 bytes); 0x10039fef consumes it.
struct WTSegInfo_F
{
	CWorldTree_F*   m_pTree;                 // +0x00
	uint32          m_nListIndex;            // +0x04
	LTVector        m_vPt1;                  // +0x08
	LTVector        m_vPt2;                  // +0x14
	WTSegCallback_F m_pfnCallback;           // +0x20
	void*           m_pUserData;             // +0x24
};

// The sphere query's descriptor; the caller owns it.
struct WTSphereInfo_F
{
	uint32          m_nListIndex;            // +0x00
	LTVector        m_vCenter;               // +0x04
	float           m_fRadius;               // +0x10
	WTBoxCallback_F m_pfnCallback;           // +0x14
	uint32          m_n018;                  // +0x18
	void*           m_pUserData;             // +0x1c
	uint32          m_n020;                  // +0x20
	uint32          m_n024;                  // +0x24
	WTNodeTest_F    m_pfnNodeTest;           // +0x28
	LTVector        m_vBoxMin;               // +0x2c
	LTVector        m_vBoxMax;               // +0x38
	CWorldTree_F*   m_pTree;                 // +0x44
};

class CWorldTree_F
{
public:
	virtual void   _v00() = 0;                                      // 0x10039be9
	virtual void   FindObjectsInBox(LTVector* pMin, LTVector* pMax,
	                                WTBoxCallback_F pfnCallback,
	                                void* pUserData,
	                                uint32 nListIndex) = 0;         // 0x10039e18
	virtual void   FindObjectsInBox2(WTRectInfo_F* pInfo) = 0;      // 0x10039e9e
	virtual void   FindObjectsOnPoint(LTVector* pPt,
	                                  WTBoxCallback_F pfnCallback,
	                                  void* pUserData,
	                                  uint32 nListIndex);           // 0x10039f82
	virtual uint32 FindObjectsOnSegment(LTVector* pPt1, LTVector* pPt2,
	                                    WTSegCallback_F pfnCallback,
	                                    void* pUserData,
	                                    uint32 nListIndex);         // 0x10039f9d
	virtual void   FindObjectsInSphere(WTSphereInfo_F* pInfo);      // 0x1003a386
	virtual void   _v18(void* a, void* b) = 0;                      // 0x10039b9f

	LTBOOL LoadLayout(ILTStream* pStream);                          // 0x1003a473

	IWTCodeSource_F* m_pCodeSource;          // +0x04
	uint32           m_nCurCode;             // +0x08
	CWTNode_F        m_RootNode;             // +0x0c
	void*            m_p068;                 // +0x68
	void*            m_p06c;                 // +0x6c
	void*            m_p070;                 // +0x70
	CheapLTLink      m_RootObjects;          // +0x74
};

// Elsewhere in the module (sections D and E).
void wt_FindInBox_F(CWTNode_F* pNode, WTSphereInfo_F* pInfo, int nLevel);  // 0x100395a4
// 0x10039d98 is SECTION E's wt_ForEachChildInBox_E; the rectangle walk
// below calls it through E's node and callback types (the node is the same
// 0x5c-byte class under both spellings).
// commonmodel.cpp's three-float read.
void w_ReadVector(ILTStream* pStream, LTVector* pVec);                    // 0x1000d040

void   wt_FindInRect_F(CWTNode_F* pNode, WTRectInfo_F* pInfo);
uint32 wt_ProcessNodeSeg_F(CWTNode_F* pNode, WTSegInfo_F* pInfo);
uint32 wt_TestNodeSeg_F(CWTNode_F* pNode, WTSegInfo_F* pInfo);
uint32 wt_ClassifyAxis_F(LTVector* pPt1, LTVector* pPt2, LTVector* pMin,
                         LTVector* pMax, uint32 nAxis);
LTBOOL wt_SegHitsRect_F(LTVector* pPt1, LTVector* pPt2, LTVector* pMin,
                        LTVector* pMax, uint32 nAxis1, uint32 nAxis2);
LTBOOL wt_SegHitsPlane_F(LTVector* pPt1, LTVector* pPt2, float fPlane,
                         LTVector* pMin, LTVector* pMax,
                         uint32 nAxis1, uint32 nAxis2);
int    wt_DefaultNodeTest_F(CWTNode_F* pNode);
void   wt_WalkRootList_F(CheapLTLink* pHead, WTSphereInfo_F* pInfo);


// 0x10039ec1, 193 bytes -- the rectangle walk slot 2 (0x10039e9e) starts.
// cdecl; it hands itself to 0x10039d98 to recurse into the children.
void wt_FindInRect_F(CWTNode_F* pNode, WTRectInfo_F* pInfo)
{
	CheapLTLink* pHead;
	LTLink*      pCur;
	CWTObj_F*    pObj;
	uint32       nCode;
	LTVector     vMin, vMax;

	if (!pNode->m_nNumObjects)
		return;

	pHead = &pNode->m_Objects[pInfo->m_nListIndex];
	pCur  = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj  = (CWTObj_F*)pCur->m_pData;
		pCur  = pCur->m_pNext;
		nCode = pInfo->m_pTree->m_nCurCode;

		if (pObj->m_nTouchCode != nCode)
		{
			pObj->m_nTouchCode = nCode;
			pObj->GetDims(&vMin, &vMax);

			if (vMin.x > pInfo->m_vMax.x) continue;
			if (vMin.y > pInfo->m_vMax.y) continue;
			if (vMin.z > pInfo->m_vMax.z) continue;
			if (vMax.x < pInfo->m_vMin.x) continue;
			if (vMax.y < pInfo->m_vMin.y) continue;
			if (vMax.z < pInfo->m_vMin.z) continue;

			pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}

	if (pNode->m_Children[0])
	{
		wt_ForEachChildInBox_E(&pInfo->m_vMin, &pInfo->m_vMax, (CWTNode_E*)pNode,
		                       (WTNodeCallback_E)wt_FindInRect_F, pInfo);
	}
}


// 0x10039f82, 27 bytes -- slot 3: a point is a zero-length box (slot 1).
void CWorldTree_F::FindObjectsOnPoint(LTVector* pPt, WTBoxCallback_F pfnCallback,
                                      void* pUserData, uint32 nListIndex)
{
	FindObjectsInBox(pPt, pPt, pfnCallback, pUserData, nListIndex);
}


// 0x10039f9d, 82 bytes -- slot 4 (client 0x0048d210).
uint32 CWorldTree_F::FindObjectsOnSegment(LTVector* pPt1, LTVector* pPt2,
                                          WTSegCallback_F pfnCallback,
                                          void* pUserData, uint32 nListIndex)
{
	WTSegInfo_F info;

	m_nCurCode = m_pCodeSource->NextCode();

	info.m_pTree       = this;
	info.m_nListIndex  = nListIndex;
	info.m_vPt1        = *pPt1;
	info.m_vPt2        = *pPt2;
	info.m_pfnCallback = pfnCallback;
	info.m_pUserData   = pUserData;

	return wt_ProcessNodeSeg_F(&m_RootNode, &info);
}


// 0x10039fef, 244 bytes -- report a node's objects, then all four children
// nearest quadrant first (client 0x0048d290).
uint32 wt_ProcessNodeSeg_F(CWTNode_F* pNode, WTSegInfo_F* pInfo)
{
	uint32       nRet;
	CheapLTLink* pHead;
	LTLink*      pCur;
	CWTObj_F*    pObj;
	uint32       nCode;

	nRet = 0;

	pHead = &pNode->m_Objects[pInfo->m_nListIndex];
	pCur  = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj  = (CWTObj_F*)pCur->m_pData;
		pCur  = pCur->m_pNext;
		nCode = pInfo->m_pTree->m_nCurCode;

		if (pObj->m_nTouchCode != nCode)
		{
			pObj->m_nTouchCode = nCode;
			nRet |= pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}

	if (pNode->m_Children[0])
	{
		uint32 nX, nZ, nHit;

		nX = (pInfo->m_vPt1.x > pNode->m_vCenter.x) ? 1 : 0;
		nZ = (pInfo->m_vPt1.z > pNode->m_vCenter.z) ? 1 : 0;

		nHit  = wt_TestNodeSeg_F(pNode->m_Children[nX * 2 + nZ], pInfo);
		nHit |= wt_TestNodeSeg_F(pNode->m_Children[(nX == 0) * 2 + nZ], pInfo);
		nHit |= wt_TestNodeSeg_F(pNode->m_Children[nX * 2 + (nZ == 0)], pInfo);
		nHit |= wt_TestNodeSeg_F(pNode->m_Children[(nX == 0) * 2 + (nZ == 0)], pInfo);

		if (nHit)
			return LTTRUE;
	}

	return nRet;
}


// Inline (no address of its own): is a point inside a rectangle in X/Z.
inline LTBOOL wt_PtInRectXZ_F(LTVector* pPt, LTVector* pMin, LTVector* pMax)
{
	return pPt->x >= pMin->x && pPt->z >= pMin->z &&
	       pPt->x <= pMax->x && pPt->z <= pMax->z;
}


// 0x1003a0e3, 238 bytes -- does the segment touch this node at all
// (client 0x0048d3a0, with its helpers out of line).
uint32 wt_TestNodeSeg_F(CWTNode_F* pNode, WTSegInfo_F* pInfo)
{
	uint32 nSide;

	if (wt_PtInRectXZ_F(&pInfo->m_vPt1, &pNode->m_vBoxMin, &pNode->m_vBoxMax) ||
	    wt_PtInRectXZ_F(&pInfo->m_vPt2, &pNode->m_vBoxMin, &pNode->m_vBoxMax))
	{
		return wt_ProcessNodeSeg_F(pNode, pInfo);
	}

	nSide = wt_ClassifyAxis_F(&pInfo->m_vPt1, &pInfo->m_vPt2,
	                          &pNode->m_vBoxMin, &pNode->m_vBoxMax, 0);
	if (nSide != 2 &&
	    nSide == wt_ClassifyAxis_F(&pInfo->m_vPt1, &pInfo->m_vPt2,
	                               &pNode->m_vBoxMin, &pNode->m_vBoxMax, 2))
	{
		return 0;
	}

	if (wt_SegHitsRect_F(&pInfo->m_vPt1, &pInfo->m_vPt2,
	                     &pNode->m_vBoxMin, &pNode->m_vBoxMax, 0, 2) ||
	    wt_SegHitsRect_F(&pInfo->m_vPt1, &pInfo->m_vPt2,
	                     &pNode->m_vBoxMin, &pNode->m_vBoxMax, 2, 0))
	{
		return wt_ProcessNodeSeg_F(pNode, pInfo);
	}

	return 0;
}


// 0x1003a1d1, 90 bytes -- 0 both endpoints below the box on nAxis, 1 both
// above, 2 otherwise.
uint32 wt_ClassifyAxis_F(LTVector* pPt1, LTVector* pPt2, LTVector* pMin,
                         LTVector* pMax, uint32 nAxis)
{
	if (((float*)pPt1)[nAxis] < ((float*)pMin)[nAxis] &&
	    ((float*)pPt2)[nAxis] < ((float*)pMin)[nAxis])
		return 0;

	if (((float*)pPt1)[nAxis] > ((float*)pMax)[nAxis] &&
	    ((float*)pPt2)[nAxis] > ((float*)pMax)[nAxis])
		return 1;

	return 2;
}


// 0x1003a22b, 96 bytes -- either of the two nAxis1 planes.
LTBOOL wt_SegHitsRect_F(LTVector* pPt1, LTVector* pPt2, LTVector* pMin,
                        LTVector* pMax, uint32 nAxis1, uint32 nAxis2)
{
	if (wt_SegHitsPlane_F(pPt1, pPt2, ((float*)pMin)[nAxis1], pMin, pMax,
	                      nAxis1, nAxis2))
		return LTTRUE;

	return wt_SegHitsPlane_F(pPt1, pPt2, ((float*)pMax)[nAxis1], pMin, pMax,
	                         nAxis1, nAxis2) != LTFALSE;
}


// 0x1003a28b, 251 bytes -- does the segment cross the plane fPlane
// (perpendicular to nAxis1) inside the box's nAxis2 range.
LTBOOL wt_SegHitsPlane_F(LTVector* pPt1, LTVector* pPt2, float fPlane,
                         LTVector* pMin, LTVector* pMax,
                         uint32 nAxis1, uint32 nAxis2)
{
	float    fPercent;
	LTVector vTest;

	if ((((float*)pPt1)[nAxis1] <  fPlane) ==
	    (((float*)pPt2)[nAxis1] <= fPlane))
		return LTFALSE;

	if (fabs(((float*)pPt2)[nAxis1] - ((float*)pPt1)[nAxis1]) < 0.001f)
		fPercent = 0.0f;
	else
		fPercent = (fPlane - ((float*)pPt1)[nAxis1]) /
		           (((float*)pPt2)[nAxis1] - ((float*)pPt1)[nAxis1]);

	vTest = *pPt1 + (*pPt2 - *pPt1) * fPercent;

	if (((float*)&vTest)[nAxis2] >= ((float*)pMin)[nAxis2] &&
	    ((float*)&vTest)[nAxis2] <= ((float*)pMax)[nAxis2])
		return LTTRUE;

	return LTFALSE;
}


// 0x1003a386, 178 bytes -- slot 5 (client 0x0048d810).
void CWorldTree_F::FindObjectsInSphere(WTSphereInfo_F* pInfo)
{
	m_nCurCode = m_pCodeSource->NextCode();

	pInfo->m_pTree = this;

	pInfo->m_vBoxMin = pInfo->m_vCenter -
		LTVector(pInfo->m_fRadius, pInfo->m_fRadius, pInfo->m_fRadius);
	pInfo->m_vBoxMax = pInfo->m_vCenter +
		LTVector(pInfo->m_fRadius, pInfo->m_fRadius, pInfo->m_fRadius);

	if (!pInfo->m_pfnNodeTest)
		pInfo->m_pfnNodeTest = wt_DefaultNodeTest_F;

	wt_FindInBox_F(&m_RootNode, pInfo, 0);

	wt_WalkRootList_F(&m_RootObjects, pInfo);
}


// 0x1003a438, 4 bytes (small) -- the default node test. Also referenced
// from 0x10026eba, so it is not static.
int wt_DefaultNodeTest_F(CWTNode_F* pNode)
{
	return 1;
}


// 0x1003a43c, 55 bytes -- the tree's own root list, box-query style.
void wt_WalkRootList_F(CheapLTLink* pHead, WTSphereInfo_F* pInfo)
{
	LTLink*   pCur;
	CWTObj_F* pObj;
	uint32    nCode;

	pCur = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj  = (CWTObj_F*)pCur->m_pData;
		pCur  = pCur->m_pNext;
		nCode = pInfo->m_pTree->m_nCurCode;

		if (pObj->m_nTouchCode != nCode)
		{
			pObj->m_nTouchCode = nCode;
			pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}
}


// 0x1003a473, 150 bytes -- read the tree's layout (client 0x0048d9a0).
// commonmodel.cpp calls this as CWorldSub6C::sub_1003a473: that TU-local
// class is this tree; the definition here is spelled on CWorldTree_F.
LTBOOL CWorldTree_F::LoadLayout(ILTStream* pStream)
{
	LTVector vMin, vMax;
	uint32   nUnused;
	uint8    nCurByte, nCurBit;

	w_ReadVector(pStream, &vMin);
	w_ReadVector(pStream, &vMax);

	pStream->Read(&nUnused, 4);
	pStream->Read(&m_p070, 4);

	nCurByte = 0;
	nCurBit  = 8;

	m_RootNode.SetDims(vMin, vMax);

	if (m_RootNode.LoadNodes(pStream, &nCurByte, &nCurBit) &&
	    pStream->ErrorStatus() == LT_OK)
		return LTTRUE;

	return LTFALSE;
}


// 0x1003a509, 32 bytes -- objtypes.h's CObjVisitor::Visit: the visitor is
// the world tree and "visiting" links the object's fifth link into the
// tree's root list (client 0x0048db00).
void CObjVisitor::Visit(CObjBase* pObj)
{
	CWTLink_F* pLink = &((CWTObj_F*)pObj)->m_Links[4];

	((CWorldTree_F*)this)->m_RootObjects.AddAfter(pLink);
	pLink->m_pNode = LTNULL;
}


// 0x1003a530, 26 bytes -- NOT THIS MODULE: rezmgr.lib's CRezItm::CRezItm
// (lithtech 0x0048df10; src/lithshared/rezmgr/rezmgr.cpp:412 declares it
// and leaves it undefined; the client's copy is ltnew1.cpp's
// CSub48df10View). Its real home is src/lithshared/rezmgr/rezmgr.cpp.
class CBaseListItem_F
{
protected:
	CBaseListItem_F* m_pNext;                // +0x00
	CBaseListItem_F* m_pPrev;                // +0x04
};

class CBaseHashItem_F : public CBaseListItem_F
{
protected:
	virtual unsigned int HashFunc() = 0;

	void*        m_pParentHash;              // +0x0c
	unsigned int m_nCurBin;                  // +0x10
};

class CRezItm_F;

class CRezItmHashByName_F : public CBaseHashItem_F
{
public:
	CRezItmHashByName_F() { m_pRezItm = NULL; }
	void SetRezItm(CRezItm_F* pRezItm) { m_pRezItm = pRezItm; }

protected:
	virtual unsigned int HashFunc();         // 0x1003c650

	CRezItm_F* m_pRezItm;                    // +0x14
};

class CRezItm_F
{
public:
	CRezItm_F();

	char*               m_sName;             // 0x000
	void*               m_pType;             // 0x004
	uint32              m_nTime;             // 0x008
	uint32              m_nSize;             // 0x00c
	void*               m_pParentDir;        // 0x010
	uint32              m_nFilePos;          // 0x014
	uint32              m_nCurPos;           // 0x018
	CRezItmHashByName_F m_heName;            // 0x01c
	void*               m_pRezFile;          // 0x034
	uint8*              m_pData;             // 0x038
};

CRezItm_F::CRezItm_F()
{
	m_pRezFile   = NULL;
	m_pParentDir = NULL;
	m_sName      = NULL;
	m_heName.SetRezItm(this);
}

// ==== END SECTION F =======================================================
