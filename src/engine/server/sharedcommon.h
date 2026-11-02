// *********************************************************************** //
//  MODULE:   SHAREDCOMMON (.H)
//  PURPOSE:  The shared half of ILTCommon, at 0x10012176-0x100126xx. Unlike
//            most classes here the name is attested: two check literals read
//            "SharedCommonLT::GetObjectFlags" and
//            "SharedCommonLT::SetObjectFlags". Its neighbours in the same file
//            report themselves as "ILTCommon::" and "CommonLT::" — three names
//            for one class, which is normal for this codebase.
// *********************************************************************** //
#ifndef __SHAREDCOMMON_H__
#define __SHAREDCOMMON_H__

#include "iltcommon.h"
#include "commonmodel.h"

// Derived from the SDK interface rather than standing alone, because the
// vtable offsets are load-bearing: GetAttachments reaches a sibling through
// `call *0x10`, and ILTCommon's own declaration order — destructor,
// SetObjectFilenames, GetObjectFlags, SetObjectFlags, GetAttachmentObjects —
// puts GetAttachmentObjects at exactly 0x10. Nothing had to be guessed.
class SharedCommonLT : public ILTCommon
{
public:
	// 0x1001214c -- out of line: CLTServer's constructor CALLS it (0x1002d3ba)
	// on its +0x234 member. It clears m_pMath and nothing else.
	SharedCommonLT();

	LTRESULT GetObjectFlags(const HOBJECT hObj, const ObjFlagType flagType, uint32& dwFlags);
	LTRESULT SetObjectFlags(HOBJECT hObj, const ObjFlagType flagType, uint32 dwFlags);

	LTRESULT GetAttachments(HLOCALOBJ hObj, HLOCALOBJ* inList, uint32 inListSize,
	                        uint32& outListSize, uint32& outNumAttachments);

	LTRESULT GetObjectType(HOBJECT hObj, uint32* type);
	LTRESULT GetModelAnimUserDims(HOBJECT hObject, LTVector* pDims, HMODELANIM hAnim);

	virtual LTRESULT GetAttachmentTransform(HATTACHMENT hAttachment, LTransform &transform,
	                                        LTBOOL bWorldSpace);   // 0x10012378

	// iltcommon.h's order, which the bytes confirm: the piece handle comes
	// second and the transform third. Both forwarders pass their three
	// arguments straight through in that order.
	virtual LTRESULT GetAttachedModelNodeTransform(HATTACHMENT hAttachment,
	                                               HMODELNODE hNode,
	                                               LTransform& transform);  // 0x10012546
	virtual LTRESULT GetAttachedModelSocketTransform(HATTACHMENT hAttachment,
	                                                 HMODELSOCKET hSocket,
	                                                 LTransform& transform);// 0x1001255c
	virtual LTRESULT Parse(ConParse* pParse);                              // 0x10012239
	virtual LTRESULT GetRotationVectors(LTRotation& rot, LTVector& up,
	                                    LTVector& right, LTVector& forward);// 0x1001232a
	virtual LTRESULT SetupEuler(LTRotation& rot, float pitch, float yaw,
	                            float roll);                               // 0x10012344
	// 0x1001236b — the slot after SetupEuler in iltcommon.h, and a stub: the
	// out-parameter is nulled and the answer is LT_ERROR, so this half of the
	// engine has no message system to hand out.
	virtual LTRESULT CreateMessage(class ILTMessage*& pMsg);                // 0x1001236b

	// 0x100124ce — the node and socket transforms are one function with a
	// flag; a node is 1 and a socket 0 (name ours). The flag picks between
	// ILTModel slots 0x18 and 0x1c, whose own check literals read
	// "ILTModel::GetNodeTransform" (0x1001d478) and
	// "ILTModel::GetSocketTransform" (0x1001d77f) — so 1 really is the node.
	LTRESULT GetAttachedTransform(HATTACHMENT hAttachment, uint32 hPiece,
	                              LTransform& transform, LTBOOL bNode);

	// 0x10012176 — the only writer of m_pMath: CLTServer's constructor hands
	// over the math implementation embedded in itself at +4 (name ours).
	void SetMath(class ICommonMath* pMath);

public:
	// ILTCommon already holds m_pTransformLT and m_pModelLT at 0x04 and
	// 0x08; the math implementation the rotation helpers forward to is the
	// next slot along.
	class ICommonMath* m_pMath;              // 0x0c
};

// names ours: quaternion-to-matrix (0x10015bcd) and the 4x4 multiply
// (0x10011b1e) GetAttachmentTransform composes with.
void quat_ConvertToMatrix(LTRotation* pRot, LTMatrix* pMat);
void mat_Multiply(LTMatrix* pOut, LTMatrix* pM1, LTMatrix* pM2);

#endif
