// server.dll — the shared ILTCommon implementation (reconstruction). Build /O1.
#if defined(PRIMAL_HUNT)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x47a6a0).
// Primal Hunt's server.dll the same (guard byte 0x1008ec84, atexit of
// 0x1002eaeb), so it is not limited to the engine's build.
#include <string>
#endif
#include <string.h>
#include "sharedcommon.h"
#include "servermath.h"
#include "servermgr.h"
#include "servermsg.h"
#include "ilttransform.h"
#include "iltmodel.h"
#include "de_error.h"

// The name an argument error reports: server.dll's copy of these functions
// says "ILTCommon::...", lithtech.exe's "CommonLT::..." (the retail images'
// strings: the two were built from different revisions of this file).
#ifdef LT_SERVER_BUILD
#define COMMON_ERROR_NAME(fn)	"ILTCommon::" fn
#else
#define COMMON_ERROR_NAME(fn)	"CommonLT::" fn
#endif

// 0x004347a0 (lithtech.exe) / 0x1001214c (server.dll) -- the constructor,
// the first function of the file: the vptr, then m_pMath cleared.  Called
// out of line by CLTServer's constructor on its +0x234 member (commonphysics
// .cpp).  lithtech.exe only for now: server.dll links svnew.cpp's
// CSharedCommonView stand-in under this name (src/engine/server/link/
// aliases.rsp), which this would duplicate.  Too small to be credited in
// either image.
#ifndef LT_SERVER_BUILD
SharedCommonLT::SharedCommonLT()
{
	m_pMath = LTNULL;
}
#endif

LTRESULT SharedCommonLT::GetObjectFlags(const HOBJECT hObj, const ObjFlagType flagType, uint32& dwFlags)
{
	dwFlags = 0;

	if (!hObj)
		RETURN_ERROR_NAMED(2, "SharedCommonLT::GetObjectFlags", LT_INVALIDPARAMS);

	if (flagType == OFT_Flags)
		dwFlags = hObj->m_Flags;
	else
		dwFlags = hObj->m_Flags2;

	return LT_OK;
}

LTRESULT SharedCommonLT::SetObjectFlags(HOBJECT hObj, const ObjFlagType flagType, uint32 dwFlags)
{
	if (!hObj)
		RETURN_ERROR_NAMED(2, "SharedCommonLT::SetObjectFlags", LT_INVALIDPARAMS);

	if (flagType == OFT_Flags)
		hObj->m_Flags = dwFlags;
	else
		hObj->m_Flags2 = dwFlags;

	return LT_OK;
}

LTRESULT SharedCommonLT::GetObjectType(HOBJECT hObj, uint32* type)
{
	if (!hObj)
		RETURN_ERROR(2, COMMON_ERROR_NAME("GetObjectType"), LT_INVALIDPARAMS);

	*type = hObj->m_ObjectType;
	return LT_OK;
}

LTRESULT SharedCommonLT::GetModelAnimUserDims(HOBJECT hObject, LTVector* pDims, HMODELANIM hAnim)
{
	if (!hObject || hObject->m_ObjectType != OT_MODEL)
		RETURN_ERROR(1, "CommonLT::GetModelAnimUserDims", LT_INVALIDPARAMS);

	Model* pModel = ((ModelInstance*)hObject)->m_AnimTracker.m_TimeRef.m_pModel;
	if (hAnim >= pModel->m_AnimArray.m_nElements)
		RETURN_ERROR_INFO(1, "CommonLT::GetModelAnimUserDims", LT_INVALIDPARAMS, "invalid hAnim");

	*pDims = pModel->m_AnimArray.m_pArray[hAnim].m_vDims;
	return LT_OK;
}

LTRESULT SharedCommonLT::GetAttachments(HLOCALOBJ hObj, HLOCALOBJ* inList, uint32 inListSize,
                                        uint32& outListSize, uint32& outNumAttachments)
{
	static char* pFnName = "SharedCommonLT::GetAttachments";
	HOBJECT hParent, hChild;

	outNumAttachments = 0;
	outListSize = 0;

	if (!hObj || !inList)
		RETURN_ERROR(1, pFnName, LT_INVALIDPARAMS);

	// hParent and hChild have their addresses taken, and VC6 gives them the
	// home slots of outListSize and outNumAttachments — dead, because both
	// live in registers for the whole loop.
	for (Attachment* pAttach = hObj->m_pAttachments; pAttach; pAttach = pAttach->m_pNext)
	{
		if (GetAttachmentObjects((HATTACHMENT)pAttach, hParent, hChild) == LT_OK)
		{
			if (outListSize < inListSize)
			{
				inList[outListSize] = hChild;
				outListSize++;
			}

			outNumAttachments++;
		}
	}

	return LT_OK;
}

LTRESULT SharedCommonLT::GetAttachmentTransform(HATTACHMENT hAttachment, LTransform &transform,
                                                LTBOOL bWorldSpace)
{
	LTransform tParent;
	LTMatrix mAttach;
	LTMatrix mParent;
	LTMatrix mFinal;
	HOBJECT hParent;
	HOBJECT hChild;
	LTRESULT dResult;
	Attachment* pAttachment = (Attachment*)hAttachment;

	// Both interfaces cached up front: the shipped prologue loads them
	// before the parameter check.
	ILTTransform* pTransformLT = m_pTransformLT;
	ILTModel* pModelLT = m_pModelLT;

	if (!hAttachment)
		RETURN_ERROR_NAMED(1, "SharedCommonLT::GetAttachmentTransform", LT_INVALIDPARAMS);

	dResult = GetAttachmentObjects(hAttachment, hParent, hChild);
	if (dResult != LT_OK)
		return dResult;

	// A model parent with a real node attaches to the node's transform;
	// everything else uses the object transform (or identity in local space).
	if (hParent->m_ObjectType == OT_MODEL && pAttachment->m_iNode != -1)
	{
		// AVP2: slot 0x20 (0x00440b9f; AvP2 1.0.9.6 and both server.dlls
		// agree), which is the AvP2 SDK's GetSocketTransform -- the SDK
		// grew GetModelLighting at 0x10, so the socket accessor is the slot
		// NOLF's ILTModelI view reached at 0x1c. An attachment hangs off a
		// socket.
		dResult = pModelLT->GetSocketTransform(hParent, pAttachment->m_iNode, tParent, bWorldSpace);
		if (dResult != LT_OK)
			return dResult;
	}
	else if (bWorldSpace)
	{
		tParent.m_Pos = hParent->m_Pos;
		tParent.m_Rot = hParent->m_Rotation;
	}
	else
	{
		tParent.m_Pos.Init();
		tParent.m_Rot.Init();
	}

	pTransformLT->ToMatrix(tParent, mParent);

	quat_ConvertToMatrix(&pAttachment->m_Offset.m_Rot, &mAttach);
	mAttach.SetTranslation(pAttachment->m_Offset.m_Pos);

	mat_Multiply(&mFinal, &mParent, &mAttach);
	pTransformLT->FromMatrix(transform, mFinal);
	return LT_OK;
}


// ---- three interface forwards the client shares (0x10012d28-0x10012d4d) - //
//
// Both tables point at these rather than at the functions behind them, so
// each is one line. The counter pair hides the fact that the engine's own
// counter is eight bytes where the SDK's LTCounter is sixteen.

void ci_StartCounter(LTCounter* pCounter)
{
	cnt_StartCounter((CBaseCounter*)pCounter);
}

uint32 ci_EndCounter(LTCounter* pCounter)
{
	return cnt_EndCounter((CBaseCounter*)pCounter);
}

// 0x00490cd0 / 0x1003ddc0 (byte-identical between the images) -- NOT stricmp.
// It is stdlith's CHelpers::UpperStrcmp (src/lithshared/stdlith/helpers.cpp),
// a case-insensitive EQUALITY predicate folding case through a 256-byte table
// and returning 1 when the strings match, 0 when they do not. clientinput.cpp
// and ltnew2.cpp both already identify it that way.
//
// The previous spelling here was `return stricmp(...)`, which is wrong twice
// over: stricmp is a different function (0x00492f30, also called from this
// image) and it is a three-way compare whose zero means EQUAL -- so the truth
// value came out inverted as well. The byte verifier can never catch this,
// because it masks call relocations and both spellings are one `call`; only
// a check of the call targets can see it.
//
// Declared as the real class member (helpers.h's static, mangled
// ?UpperStrcmp@CHelpers@@SAHPBD0@Z) so the call's relocation names the
// stdlith symbol itself -- a free-function stand-in has no definition to
// link against. A byte diff of src/lithshared/stdlith/Release/helpers.obj's
// UpperStrcmp against 0x1003ddc0 and 0x00490cd0: byte-identical, 64 bytes.
// Declared here rather than by including helpers.h, as serverengine.cpp does
// for CHelpers::ExtractNames.
class CHelpers
{
public:
	static int UpperStrcmp(const char* pStr1, const char* pStr2);   // 0x1003ddc0
};

LTBOOL ci_UpperStrcmp(char* pStr1, char* pStr2)
{
	return CHelpers::UpperStrcmp(pStr1, pStr2);
}


// A node transform and a socket transform are one function with a flag: a
// node is one and a socket zero. `this` rides through in ecx, so neither
// forwarder touches it.
LTRESULT SharedCommonLT::GetAttachedModelNodeTransform(HATTACHMENT hAttachment,
                                                       HMODELNODE hNode,
                                                       LTransform& transform)
{
	return GetAttachedTransform(hAttachment, hNode, transform, LTTRUE);
}

LTRESULT SharedCommonLT::GetAttachedModelSocketTransform(HATTACHMENT hAttachment,
                                                         HMODELSOCKET hSocket,
                                                         LTransform& transform)
{
	return GetAttachedTransform(hAttachment, hSocket, transform, LTFALSE);
}

// Running out of commands is not an error, so the parser's "no more" answers
// LT_FINISHED rather than a failure code.
LTRESULT SharedCommonLT::Parse(ConParse* pParse)
{
	return ((CommandParser*)pParse)->Next() ? LT_OK : LT_FINISHED;
}

// The two rotation helpers are the interface's, but the arithmetic is not:
// both hand straight on to the math implementation. Note the argument swap —
// ILTCommon asks for up before right and the math takes right before up.
LTRESULT SharedCommonLT::GetRotationVectors(LTRotation& rot, LTVector& up,
                                            LTVector& right, LTVector& forward)
{
	return m_pMath->GetRotationVectors(&rot, &right, &up, &forward);
}

// AVP2: the math the shared half reaches has iltmath.h's two SetupEuler
// overloads, and VC6 lays an overload pair out in reverse declaration order:
// the vector form is slot 2, this one slot 3 (`call *0xc(%eax)`,
// 0x0043ffb9; AvP2's server.dll 0x10015f99 the same). servermath.h's
// ICommonMath, which the rest of the engine shares, has only the one.
class ICommonMathQ508
{
public:
	virtual LTRESULT GetRotationVectors(LTRotation*, LTVector*, LTVector*, LTVector*)=0;   // 0x00
	virtual LTRESULT InterpolateRotation(LTRotation*, LTRotation*, LTRotation*, float)=0;  // 0x04
	// Declared as iltmath.h does, float form first; it lands in slot 3.
	virtual LTRESULT SetupEuler(LTRotation*, float, float, float)=0;                       // 0x0c
	virtual LTRESULT SetupEuler(LTRotation*, LTVector*)=0;                                 // 0x08
};

LTRESULT SharedCommonLT::SetupEuler(LTRotation& rot, float pitch, float yaw,
                                    float roll)
{
	return ((ICommonMathQ508*)m_pMath)->SetupEuler(&rot, pitch, yaw, roll);
}


// 0x10012176 — the only writer of m_pMath. CLTServer's constructor
// (0x1002d3xx) reaches this class at its +0x234 and hands over the address of
// the CCommonMath embedded in itself at +4, which is the one place the shared
// half learns where the math lives. Name ours; ten bytes, one store.
void SharedCommonLT::SetMath(ICommonMath* pMath)
{
	m_pMath = pMath;
}


// 0x100124ce — one function behind both attachment-piece accessors.
//
// Three interface calls in a row, each aborting on failure, then the compose.
// Two things are worth reading off the bytes rather than the headers:
//
//   * the flag picks ILTModel slots 0x18 and 0x1c, and those two functions
//     name themselves "ILTModel::GetNodeTransform" (0x1001d478) and
//     "ILTModel::GetSocketTransform" (0x1001d77f) in their own parameter
//     checks. Slot numbering here is iltmodel.h's — ILTModel has no virtual
//     destructor, so GetSocket is slot 0. (AVP2: the SDK's GetModelLighting
//     at 0x10 moves the pair to 0x1c/0x20, 0x00440d29/0x00440d2e; the
//     neighbouring GetAttachmentTransform calls GetSocketTransform, 0x20.)
//
//   * Multiply's return value is discarded. The three earlier results are
//     each tested and returned; the last is not, and the function ends
//     `call *0x1c(%eax); xor eax,eax`, so the source returns LT_OK.
//
// The attachment transform is asked for in WORLD space and the piece
// transform in LOCAL space, which is what makes the product meaningful.
LTRESULT SharedCommonLT::GetAttachedTransform(HATTACHMENT hAttachment, uint32 hPiece,
                                              LTransform& transform, LTBOOL bNode)
{
	LTransform tAttach;
	LTransform tPiece;
	HOBJECT    hParent;
	HOBJECT    hChild;
	LTRESULT   dResult;

	// Both interfaces are cached before the first call, model first: the
	// shipped prologue reads +0x08 into edi and +0x04 into ebx and never
	// re-addresses `this` after the second call (it becomes the zero the
	// three result compares use).
	ILTModel*     pModelLT     = m_pModelLT;
	ILTTransform* pTransformLT = m_pTransformLT;

	dResult = GetAttachmentObjects(hAttachment, hParent, hChild);
	if (dResult != LT_OK)
		return dResult;

	dResult = GetAttachmentTransform(hAttachment, tAttach, LTTRUE);
	if (dResult != LT_OK)
		return dResult;

	if (bNode)
		dResult = pModelLT->GetNodeTransform(hChild, hPiece, tPiece, LTFALSE);
	else
		dResult = pModelLT->GetSocketTransform(hChild, hPiece, tPiece, LTFALSE);

	if (dResult != LT_OK)
		return dResult;

	pTransformLT->Multiply(transform, tAttach, tPiece);
	return LT_OK;
}


// 0x10015bcd — quaternion to matrix. This one needed no reconstruction: the
// SDK ships the source in src/LT2/sdk/inc/ltquatbase.cpp and the shipped bytes are
// that function, statement for statement, including the six-deep chained
// assignment of the last row and column (the zero stores come out in reverse
// order, 0x38 down to 0x0c, which is that chain evaluating right to left).
// Its real home is ltquatbase.cpp; it sits here because that TU is not part of
// the reconstruction yet.
//
// The declaration this file uses takes the quaternion and the matrix as their
// LT types rather than the SDK's `const float*` / `float[4][4]`, because the
// callers in this TU pass those; the bodies are the same arithmetic on the
// same offsets.
void quat_ConvertToMatrix(LTRotation* pQuat, LTMatrix* pMat)
{
	float s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

	// get the values for matrix calcuation...
	s = 2.0f / ((pQuat->m_Quat[0] * pQuat->m_Quat[0]) + (pQuat->m_Quat[1] * pQuat->m_Quat[1]) +
		(pQuat->m_Quat[2] * pQuat->m_Quat[2]) + (pQuat->m_Quat[3] * pQuat->m_Quat[3]));

	xs = pQuat->m_Quat[0] * s;
	ys = pQuat->m_Quat[1] * s;
	zs = pQuat->m_Quat[2] * s;

	wx = pQuat->m_Quat[3] * xs;
	wy = pQuat->m_Quat[3] * ys;
	wz = pQuat->m_Quat[3] * zs;

	xx = pQuat->m_Quat[0] * xs;
	xy = pQuat->m_Quat[0] * ys;
	xz = pQuat->m_Quat[0] * zs;

	yy = pQuat->m_Quat[1] * ys;
	yz = pQuat->m_Quat[1] * zs;

	zz = pQuat->m_Quat[2] * zs;

	// Fill in matrix
	pMat->m[0][0] = 1.0f - (yy + zz);
	pMat->m[0][1] = xy - wz;
	pMat->m[0][2] = xz + wy;

	pMat->m[1][0] = xy + wz;
	pMat->m[1][1] = 1.0f - (xx + zz);
	pMat->m[1][2] = yz - wx;

	pMat->m[2][0] = xz - wy;
	pMat->m[2][1] = yz + wx;
	pMat->m[2][2] = 1.0f - (xx + yy);

	pMat->m[0][3] = pMat->m[1][3] = pMat->m[2][3] =
		pMat->m[3][0] = pMat->m[3][1] = pMat->m[3][2] = 0.0f;
	pMat->m[3][3] = 1.0f;
}


// ---- the console's two server-side notifications ------------------------- //

// ServerMessage::WriteString (0x10026137): the string and its terminator
// through WriteData, or one zero byte when the pointer is null. servermsg.h
// does not carry it and is not this reconstruction's to change, so the
// message is cast to reach it (name ours).
class CMsgStringWriter
{
public:
	void WriteString(char* pStr);            // 0x10026137
	void WriteData(void* pData, uint32 nBytes);   // 0x1002616a
};

// A null pointer still puts a byte on the wire — one zero — so the reader's
// side is symmetric either way. The terminator is included in the length
// (`inc %eax` on strlen's result), and the one-byte local's address is what
// gives this function its ebp frame; `andb $0x0` is /O1's zero store.
//
// Both arms reach one shared `call`: the shipped code cross-jumps the two
// WriteData calls after their arguments are pushed, which is what the
// `jmp` at 0x10026152 into 0x1002615e is.
void CMsgStringWriter::WriteString(char* pStr)
{
	char cNull;

	if (pStr)
	{
		WriteData(pStr, strlen(pStr) + 1);
	}
	else
	{
		cNull = 0;
		WriteData(&cNull, 1);
	}
}

// 0x10021c55 — the net manager's queued send, reached through a view of the
// embedded manager because servermgr.h's CNetMgr does not model it. Same
// class, name and signature smgr.cpp uses, so the two translation units
// reference one symbol.
class CNetMgrSendView
{
public:
	int SendToConnection(class ServerMessage* pMsg, class NetConnection* pConn,
	                     uint32 flags);      // 0x10021c55
};

// 0x10029609 — put the packet id in the first payload byte and hand the
// message to every client that still has a connection. The id is stored as a
// byte and the walk is the manager's client list at +0x7e8.
//
// This is sm_BroadcastToClients (0x100297b8, smgr.cpp) with the send flags
// handed in and *without* the m_State == 3 test: every connected client gets
// it, in the world or not. smgr.cpp already declares it under this name.
void sm_BroadcastToClientsEx(CServerMgr* pMgr, int nPacketID,
                             ServerMessage* pMsg, uint32 flags)
{
	LTLink* pHead;
	LTLink* pCur;

	pMsg->m_pData[0] = (uint8)nPacketID;

	pHead = &pMgr->m_ClientList.m_Head;
	pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		Client* pClient = (Client*)pCur->m_pData;

		if (pClient->m_pConnection)
		{
			((CNetMgrSendView*)&pMgr->m_NetMgr)->SendToConnection(
				pMsg, pClient->m_pConnection, flags);
		}

		pCur = pCur->m_pNext;
	}
}

// 0x1003485a — restart every object's deactivation countdown from its stored
// time, re-file the ones that were waiting on it, and wake their cells (name
// ours). Only the AutoDeactivate variable reaches it.
//
// The body is si_SetDeactivationTime (0x10031583) run over every live object
// in the 0x8dc list, with two differences the bytes make plain: the countdown
// is reloaded from the object's own stored time rather than from a parameter
// (so the 0x14 -> 0x18 copy is an integer move, not an FPU round trip), and
// the container bump is gated on the object's +0x60 list being empty instead
// of on m_Unknown58 == -1. ltobject_internal.h still carries +0x60 as
// padding and what that list holds is not established, so it is reached by
// offset here rather than named in the header.
//
// The list is walked with the node advanced before the body, which is what
// lets the object pointer and the cursor share edi/esi.
class CObjLink60View
{
public:
	uint8  m_pad_00[0x60];
	LTLink m_Link60;                         // +0x60
};

void sm_ResetDeactivationTimers(CServerMgr* pMgr)
{
	LTLink* pHead;
	LTLink* pCur;

	pHead = &pMgr->m_List8dc.m_Head;
	pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		LTObject* pObj = (LTObject*)pCur->m_pData;
		SObjData* pSData;

		pCur = pCur->m_pNext;

		pSData = pObj->m_pSData;

		if (pSData)
		{
			uint32 nChangeFlags;

			pSData->m_UpdateTimer = pSData->m_DeactivationTime;

			nChangeFlags = pObj->m_ChangeFlags;

			if (nChangeFlags & 0x20)
				sm_SetObjectState(pMgr, pObj, nChangeFlags & 0x18);

			if (g_bAutoDeactivate)
			{
				CObjLink60View* pView = (CObjLink60View*)pObj;

				if (pView->m_Link60.m_pNext == &pView->m_Link60)
					pMgr->m_Unknown210.BumpObject(pObj, 0);
			}
		}
	}
}

// 0x10028e97 — a console variable was created: tell the clients, and if it
// was AutoDeactivate, act on it here as well.
//
// Three things read off the bytes:
//
//   * the packet comes back as a temporary reference that is released as
//     soon as the scoped one has taken its own count -- the same statement
//     ServerCommonLT::SetObjectFilenames opens with. The temporary lives in
//     the dead first parameter's stack slot: the console state is cached in
//     edi before the allocation, so `lea 0x8(%ebp)` is the temporary's
//     address and not an argument's.
//   * the missing manager is an early return past the reference: the
//     shipped `je` (0x10034254) goes straight to the epilogue, with the
//     still-empty reference's release folded away, where an enclosing `if`
//     joins the destructor's test.
//   * the deactivation sweep is reached through the second global manager
//     pointer, not through the state's own — `push 0x10058bf0` is a memory
//     push, so what it hands over is g_pServerMgr's value.
void cc_OnVarCreated(ConsoleState* pState, LTCommandVar* pVar)
{
	CMessageRef msg;

	if (!pState->m_pServerMgr)
		return;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	if (stricmp(pVar->m_pName, "AutoDeactivate") == 0)
		sm_ResetDeactivationTimers(g_pServerMgr);

	{
		ServerMessage* pMsg = msg.m_pMsg;

		pMsg->m_Unknown38 = 1;
		pMsg->m_Unknown34 = 1;
	}

	((CMsgStringWriter*)msg.m_pMsg)->WriteString(pVar->m_pName);
	((CMsgStringWriter*)msg.m_pMsg)->WriteString(pVar->m_pStringVal);

	sm_BroadcastToClientsEx(pState->m_pServerMgr, 0x10, msg.m_pMsg, 0x80);
}

// 0x10028f42 / 0x0046ef0 — a console variable changed: exactly the same
// notification, forwarded. (The two share one enumerated range in
// the server.dll function enumeration, which is why the entry reads 187 bytes and
// the created half is 171.)
//
// THE LITHTECH ADDRESS IS 0x00466ef0, and writing it down here is the point:
// this row spent the whole project on the unexamined list because the banner
// carried only the server address. It also corrects the decoded map, which
// names 0x00466e30 `cc_OnVarCreated` on `bridged-funcptr` evidence -- the
// pointer that is actually STORED is 0x00466ef0, at 0x00466f67
// (`movl $0x00466ef0,0x34(%esi)`, bytes c7 46 34 f0 6e 46 00), and
// 0x00466e30's only inbound `e8` in the image is from inside this forwarder.
// So 0x00466ef0 is the installed hook and 0x00466e30 is its out-of-line body.
void cc_OnVarChanged(ConsoleState* pState, LTCommandVar* pVar)
{
	cc_OnVarCreated(pState, pVar);
}


// 0x100587a8 — where a console command's failure is left for the caller to
// pick up. Nothing passes it; the runner clears it, the command handler
// writes it, and the wrapper below reads it back (name ours).
extern int g_nConsoleResult;

// 0x10028fc6 — run one console string and answer what it did. The result
// does not come back through con_RunString, which is void: it comes out of a
// global the handler wrote, which is why this wrapper exists at all and why
// it has to clear the global first. The clear lands between the two argument
// pushes in the shipped code, which is scheduling rather than source order.
int cc_RunConsoleString(ConsoleState* pState, char* pString)
{
	g_nConsoleResult = 0;

	con_RunString(pState, pString);

	return g_nConsoleResult;
}


// 0x1001236b — iltcommon.h's CreateMessage, and a stub. Slot 0x1004d738,
// immediately after SetupEuler: the out-parameter is nulled and the answer is
// LT_ERROR (`push $1; pop %eax`), so this half of the engine hands out no
// message objects at all.
LTRESULT SharedCommonLT::CreateMessage(ILTMessage*& pMsg)
{
	pMsg = LTNULL;

	return LT_ERROR;
}


// AVP2: 0x10016b42 -- ILTCommon::GetCRC: the CRC-32 (the reflected IEEE
// polynomial, 0xedb88320, from all ones, complemented at the end) of the
// whole stream, read a byte at a time from where it stands. The table is a
// local the function fills in on every call, all 1 KB of it, as shipped.
LTRESULT SharedCommonLT::GetCRC(ILTStream* pStream, uint32& dwResult)
{
	uint8  nByte = 0;
	uint32 nCRC = 0xffffffff;
	uint32 crcTable[256] =
	{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
		0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
		0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
		0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
		0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
		0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
		0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
		0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
		0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
		0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
		0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};

	if (!pStream)
	{
		dwResult = 0;
		return LT_ERROR;
	}

	// iltstream.h's value-returning GetLen(): its temporary is the frame
	// slot the length comes back in, and the loop counts its own index
	// (lithtech.exe 0x00440aa4..0x00440ab6: `cmp %ebx,%eax ; jbe`, then the
	// count copied into ebx).
	uint32 nLen = pStream->GetLen();
	uint32 i;

	for (i = 0; i < nLen; i++)
	{
		pStream->Read(&nByte, 1);
		nCRC = (nCRC >> 8) ^ crcTable[(uint8)nCRC ^ nByte];
	}

	dwResult = ~nCRC;
	return LT_OK;
}


// 0x00466f80, 12 bytes — the console teardown forwarder.
//
//      8b 44 24 04   mov 0x4(%esp),%eax
//      50            push %eax
//      e8 ..         call 0x0041b200
//      59            pop %ecx
//      c3            ret
//
// cdecl, one argument, forwarded unchanged. The callee 0x0041b200 is
// con_Term — clientconsole.cpp:975 and cmgrinit.cpp:88 both name it. The one
// caller, 0x00473696, passes `lea 0xa74(%esi),%edx`, the ConsoleState
// embedded in the manager at +0xa74. `pop %ecx` rather than `add $4,%esp` is
// the one-argument cdecl cleanup /O2 always picks.
void con_Term(ConsoleState* pState);         // 0x0041b200

void cc_TermConsole(ConsoleState* pState)
{
	con_Term(pState);
}
