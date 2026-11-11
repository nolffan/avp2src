//------------------------------------------------------------------
//
//  MODULE  : TRANSFORMMAKER.CPP
//
//  PURPOSE : server.dll's model transform maker (0x10038674 .. 0x10038e76;
//            Primal Hunt 0x1004874d .. 0x10048f93), its own unit as the
//            client's src/engine/lithtech/transformmaker.cpp is: Primal
//            Hunt's copy ends in its own <string> guard pair (0x10048f7f).
//            Split out of worldtree.cpp, whose SECTION C it was.
//
//------------------------------------------------------------------

#ifdef PRIMAL_HUNT
// PH: <string>'s guarded ctype initializer ends the unit (0x10048f7f).
#include <string>
#endif
#include "worldbsp.h"
#include "servermgr.h"

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
// Status (NOLF): IsValid, SetupTransforms, SetupTransformsForNode, Prepare,
// BlendNode and ProcessNode byte-identical; GetNodeTransform (361 vs 357)
// and GetNodeTransformRel (323 vs 321) are nears whose residual is
// register/slot choice only (see the notes above each).
// Primal Hunt server.dll: IsValid 0x1004874d, SetupTransforms 0x1004879f,
// Prepare 0x100487d0 and ProcessNode 0x10048d4d identical; GetNodeTransform
// 0x100488d2 (361/357), GetNodeTransformRel 0x10048a37 (314/310) and
// BlendNode 0x10048b90 (449/445) near.  The TU ends in <string>'s ctype
// initializer (0x10048f7f).
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
	// AVP2: four bytes more than NOLF's before the root node, as the
	// client's ModelK (transform.cpp): SetupTransforms reads 0x2c4 in both
	// AvP2 1.0.9.6's server.dll and Primal Hunt's (0x100487bb).
	uint8          m_pad_16c[0x2c4 - 0x16c];
	CModelNode_C*  m_pRootNode;              // 0x2c4
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
	float  BlendNode(uint32 iTimeRef, uint32 iNode,
	                 float fBlended, LTBOOL bNormalize);        // 0x10038b0a
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
	// AVP2: a weight set per key-frame side, as the client's
	// CTransformMakerK: Prepare walks one induction pointer from this+0x248
	// at -0x20/0/+0x1c/+0x3c (server.dll 0x100487f8..0x1004886b), and the
	// whole object is NOLF's +0x40 from the eight bigger time refs on.
	CWeightSet_C*  m_pPrevWeightSets[MAX_TM_TIMEREFS_C]; // 0x228
	CWeightSet_C*  m_pWeightSets[MAX_TM_TIMEREFS_C]; // 0x248
	CModelAnim_C*  m_pPrevAnims[MAX_TM_TIMEREFS_C];  // 0x268
	CModelAnim_C*  m_pCurAnims[MAX_TM_TIMEREFS_C];   // 0x288
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

		// AVP2: provisional -- the current key frame's weight set, as retail
		// GetWeightSet returns it (Primal Hunt server.dll 0x10024f97); whether
		// this uses the previous key frame's too is for the retail code to show
		if (i != 0 && m_TimeRefs[i].m_Cur.m_iWeightSet >= pModel->m_nWeightSets)
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

#if 0
// AVP2: not built. Neither AvP2's nor Primal Hunt's server.dll has it: in
// both, SetupTransforms (0x10043afe, 0x1004879f) runs straight into Prepare
// (0x10043b2f, 0x100487d0), and nothing calls it.
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
#endif

LTBOOL CTransformMaker_C::Prepare()
{
	uint32 i;

	if (!IsValid())
		return LTFALSE;

	m_pNodePath = LTNULL;
	m_pModel = (CModel_C*)m_TimeRefs[0].m_pModel;

	for (i = 0; i < m_nTimeRefs; i++)
	{
		// AVP2: both weight sets, for every time ref, and an index the model
		// does not have gives none (server.dll 0x10048801..0x10048833), as
		// the client's Prepare.
		m_pPrevWeightSets[i] =
			(m_TimeRefs[i].m_Prev.m_iWeightSet >= m_pModel->m_nWeightSets) ? LTNULL :
			m_pModel->m_pWeightSets[m_TimeRefs[i].m_Prev.m_iWeightSet];
		m_pWeightSets[i] =
			(m_TimeRefs[i].m_Cur.m_iWeightSet >= m_pModel->m_nWeightSets) ? LTNULL :
			m_pModel->m_pWeightSets[m_TimeRefs[i].m_Cur.m_iWeightSet];

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

// AVP2: the blend now takes the weight already blended in and the time
// ref's normalize flag, and answers the weight it used (server.dll
// 0x10048b90, `ret $0x10`, the result in %st(0)).  A normalizing time ref
// takes whatever is left of 1.0; otherwise the weight is lerped between
// the two key frames' weight sets, where 2.0 (relative) on either side is
// kept rather than lerped.  The relative blend answers 0.0.
// NEAR (449 against 445): the shipped code keeps the 2.0f in an x87
// register across all three comparisons (one `flds 0x100742b0` at
// 0x10048bbc, then `fcomp %st(2)` / `fcom %st(1)`), and its clamp keeps an
// else arm that copies fBlended onto itself (0x10048baf); ours reloads the
// constant at each test and drops the copy.  A named 2.0f local folds.
float CTransformMaker_C::BlendNode(uint32 iTimeRef, uint32 iNode,
                                   float fBlended, LTBOOL bNormalize)
{
	float      fWeight;
	LTRotation rot, rOld;
	LTVector   vPos;

	fBlended = (fBlended > 1.0f) ? 1.0f : fBlended;

	if (bNormalize)
	{
		fWeight = 1.0f - fBlended;
	}
	else
	{
		float fPrev = m_pPrevWeightSets[iTimeRef] ?
			m_pPrevWeightSets[iTimeRef]->m_pWeights[iNode] : 0.0f;
		float fCur  = m_pWeightSets[iTimeRef] ?
			m_pWeightSets[iTimeRef]->m_pWeights[iNode] : 0.0f;

		if (fPrev == 2.0f)
			fWeight = fCur;
		else if (fCur == 2.0f)
			fWeight = 2.0f;
		else
			fWeight = fPrev + (fCur - fPrev) * m_TimeRefs[iTimeRef].m_Percent;
	}

	if (fWeight == 0.0f)
		return 0.0f;

	if (fWeight == 2.0f)
	{
		GetNodeTransformRel(iTimeRef, iNode, &rot, &vPos);

		m_rBlend = m_rBlend * rot;
		m_vBlend = m_vBlend + vPos;

		return 0.0f;
	}

	GetNodeTransform(iTimeRef, iNode, &rot, &vPos);

	rOld = m_rBlend;
	quat_Slerp(m_rBlend.m_Quat, rOld.m_Quat, rot.m_Quat, fWeight);

	m_vBlend = m_vBlend + (vPos - m_vBlend) * fWeight;

	return fWeight;
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
	uint32         i, nLast;
	float          fBlended;

	for (;;)
	{
		pTransform = &m_pTransforms[iNode];

		GetNodeTransform(0, iNode, &m_rBlend, &m_vBlend);

		// AVP2: the weight already blended in, seeded from time ref zero's
		// two weight sets (server.dll 0x10048d81..0x10048dbf) and summed
		// over what each BlendNode reports it used.
		if (m_pPrevWeightSets[0] && m_pWeightSets[0] &&
		    m_TimeRefs[0].m_Percent != 2.0f)
		{
			float fPrev = m_pPrevWeightSets[0]->m_pWeights[iNode];
			float fCur  = m_pWeightSets[0]->m_pWeights[iNode];

			fBlended = fPrev + (fCur - fPrev) * m_TimeRefs[0].m_Percent;
		}
		else
			fBlended = 0.0f;

		for (i = 1; i < m_nTimeRefs; i++)
			fBlended += BlendNode(i, iNode, fBlended, m_TimeRefs[i].m_bNormalize);

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

			for (i = 0; i < nLast; i++)
				ProcessNode(pNode->m_pChildren[i]->m_iNode, pTransform);

			m_nNodePath = 1;
			iNode = pNode->m_pChildren[nLast]->m_iNode;
			pParent = pTransform;
		}
	}
}

