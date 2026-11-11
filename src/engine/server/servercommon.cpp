// server.dll — the server's half of ILTCommon (reconstruction). Build /O1.
#if defined(PRIMAL_HUNT)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x47fc90). server.dll's
// unit ends the same way, in /O1's 33-byte shape (guard 0x1008ec84, atexit
// of the empty 0x1002eaeb; 0x1003753f).
#include <string>
#endif
#include <stdio.h>
#include "servercommon.h"
#include "servermsg.h"
#include "servermgr.h"
#include "servermodel.h"
#include "de_error.h"
#if !defined(LT_SERVER_BUILD) || defined(PRIMAL_HUNT)
#include "packet.h"
#endif
#ifdef PRIMAL_HUNT
#include "commonmodel.h"
#endif

LTRESULT ServerCommonLT::GetAttachmentObjects(HATTACHMENT hAttachment, HOBJECT& hParent, HOBJECT& hChild)
{
	static char* pFnName = "ServerCommonLT::GetAttachmentObjects";

	if (!hAttachment)
		RETURN_ERROR(1, pFnName, LT_INVALIDPARAMS);

	// Both are looked up before either is checked, so the caller always gets
	// whatever could be resolved. sm_FindObjectByID is servermgr.h's name for
	// the 0x1002acd3 (lithtech 0x0046a380) definition.
	Attachment* pAttach = (Attachment*)hAttachment;
	hParent = sm_FindObjectByID(m_pServerMgr, pAttach->m_nParentID);
	hChild = sm_FindObjectByID(m_pServerMgr, pAttach->m_nChildID);

	if (!hParent || !hChild)
		RETURN_ERROR(1, pFnName, LT_NOTINITIALIZED);

	return LT_OK;
}

// WHY IT IS `inline` AND WHY IT IS UP HERE, ABOVE ITS ONE CALLER.
// lithtech.exe (/O2) EXPANDS this into GetPolyTextureFlags below — 0x0046c6d0
// is 137 bytes and carries the shift, the +0x190 bound check, the indexed
// +0x18c load and the virtual call in line, where server.dll (/O1, /Os) calls
// it at 0x1002da86 and that is its ONLY caller in either image. VC6's inliner
// is forward-only, so the definition has to sit above the call site, and
// hoisting alone is measured to do nothing — the keyword is what raises the
// size limit past 47 bytes.
//
// 0x1002dad1 — CServerWorld::FindPoly. An HPOLY is (world index << 16) |
// (poly index within that world); the high half selects the record in the
// 0x330 table and the poly question is then that world's own.
//
// Two things read off the bytes rather than guessed:
//
//   * `this` is the manager's embedded world (the caller does
//     `add $0x1a4,%ecx`), and the two fields it reads are at +0x18c and
//     +0x190 — the manager's 0x330 and 0x334, i.e. m_pFileTrees and
//     m_nFileTrees. servermgr.h models those as CServerMgr's own, which is
//     how the rest of the image reaches them (GetHPolyObject), so they are
//     reached here through a layout cast rather than moved.
//   * both not-found paths tail-merge into one `xor eax,eax` block sitting
//     *between* the two tests, which is two separate guarded returns (the
//     second one's copy stays inline and the first jumps forward into it).
//     An `||` would sink the merged block past the call.
//
// The virtual is IWorldBsp's slot 0x0c, reached on the record's +0x04
// pointer, and it is handed the whole handle — the low half it needs and the
// high half it does not.
class CWorldTrees
{
public:
	uint8       m_pad_00[0x18c];
	CFileTree** m_pFileTrees;                // +0x18c  = CServerMgr +0x330
	uint32      m_nFileTrees;                // +0x190  = CServerMgr +0x334
};

inline WorldPoly* CServerWorld::FindPoly(HPOLY hPoly)
{
	CWorldTrees* pThis = (CWorldTrees*)this;

	uint32 iWorld = hPoly >> 16;
	if (iWorld >= pThis->m_nFileTrees)
		return LTNULL;

	CFileTree* pTree = pThis->m_pFileTrees[iWorld];
	if (!pTree)
		return LTNULL;

	return ((IWorldBsp*)pTree->m_pList)->FindPoly(hPoly);
}

// AVP2: Primal Hunt's world polygon, as FindPoly hands it back and the poly
// queries read it (server.dll 0x10039dbc, 0x10039e5d..0x10039e8e,
// 0x10039eea): its surface has moved four bytes down from NOLF's (+0x28 is
// now the plane), and its vertices are its own, after the count at +0x54.
// The surface's flags are at +0x30, its texture flags at +0x34 (NOLF's
// +0x3c). Names ours, as WorldPoly's.
// The texture's file, as lithtech.exe's GetPolyTextureName walks to its
// name: the surface's +0x2c, that object's +0x24, and the name at +0x1c
// (0x004812dc..0x0048136e). Names ours.
class CPolyTexFileQ508
{
public:
	uint8 m_pad_00[0x1c];
	char* m_pName;                           // 0x1c
};

class CPolyTextureQ508
{
public:
	uint8             m_pad_00[0x24];
	CPolyTexFileQ508* m_pFile;               // 0x24
};

class WorldSurfaceAvP2
{
public:
	uint8  m_pad_00[0x2c];
	CPolyTextureQ508* m_pTexture;            // 0x2c
	uint32 m_nFlags;                         // 0x30
	uint16 m_nTextureFlags;                  // 0x34
};

class WorldPolyAvP2
{
public:
	uint8             m_pad_00[0x28];
	LTPlane*          m_pPlane;              // 0x28
	WorldSurfaceAvP2* m_pSurface;            // 0x2c
	uint8             m_pad_30[0x54 - 0x30];
	uint16            m_nVertices;           // 0x54
	uint8             m_pad_56[0x58 - 0x56];
	WorldPolyVertex   m_Vertices[1];         // 0x58  as many as m_nVertices
};

LTRESULT ServerCommonLT::GetPolyTextureFlags(HPOLY hPoly, uint32* pFlags)
{
	*pFlags = 0;

	WorldPolyAvP2* pPoly = (WorldPolyAvP2*)m_pServerMgr->m_World.FindPoly(hPoly);
	if (pPoly)
	{
		*pFlags = pPoly->m_pSurface->m_nTextureFlags;
		return LT_OK;
	}

	RETURN_ERROR(2, "CommonLT::GetPolyTextureFlags", LT_ERROR);
}

#ifdef PRIMAL_HUNT
#ifdef LT_SERVER_BUILD
// PH: 0x10039e2d -- the server has no textures: an empty name, LT_ERROR,
// and no error message.
LTRESULT ServerCommonLT::GetPolyTextureName(HPOLY hPoly, char* pName, uint32 nMaxLen)
{
	if (pName)
		pName[0] = 0;

	return LT_ERROR;
}
#else
// PH: 0x00481280 -- lithtech.exe's does have them: the name of the poly's
// surface's texture file, through _snprintf("%s"). No buffer is LT_ERROR
// without a message; everything else that fails is RETURN_ERROR at level 2,
// in three copies (0x004812ea, 0x00481333, 0x00481387).
//
// NEAR (281 against 321): the shipped build keeps all three error bodies
// whole, each printing through a different scratch register (edx, ecx,
// eax), and tests the texture through a copy (`mov 0x2c(%eax),%edx`) so the
// surface stays live; ours cross-jumps the first body into the third's
// print and tests in place. A nested `if (pSurface) { if (!tex) error; if
// (file) success }` form, this flat one, and one if/else-if/else chain with
// a return in every arm all compile the same.
LTRESULT ServerCommonLT::GetPolyTextureName(HPOLY hPoly, char* pName, uint32 nMaxLen)
{
	if (!pName)
		return LT_ERROR;

	WorldPolyAvP2* pPoly = (WorldPolyAvP2*)m_pServerMgr->m_World.FindPoly(hPoly);
	if (pPoly && nMaxLen)
	{
		WorldSurfaceAvP2* pSurface = pPoly->m_pSurface;

		if (pSurface && !pSurface->m_pTexture)
			RETURN_ERROR(2, "CommonLT::GetPolyTextureName", LT_ERROR);

		if (!pSurface || !pSurface->m_pTexture->m_pFile)
			RETURN_ERROR(2, "CommonLT::GetPolyTextureName", LT_ERROR);

		_snprintf(pName, nMaxLen, "%s", pSurface->m_pTexture->m_pFile->m_pName);
		return LT_OK;
	}

	RETURN_ERROR(2, "CommonLT::GetPolyTextureName", LT_ERROR);
}
#endif
#endif

// AVP2: 0x10039e3e. The vertices are copied while there are both vertices
// and room for them, the count first; the count comes back whole however
// many were copied, as iltcommon.h says. The destination is walked by its
// own post-increment (`mov %esi,%ebx; add $0xc,%esi`, 0x00481435); indexed
// by i, the register roles came out swapped.
LTRESULT ServerCommonLT::GetPolyInfo(HPOLY hPoly, LTPlane** ppPlane, LTVector* pVertexList,
                                     uint32 nVertexListMaxSize, uint32* pnNumVertices)
{
	WorldPolyAvP2* pPoly = (WorldPolyAvP2*)m_pServerMgr->m_World.FindPoly(hPoly);
	if (pPoly)
	{
		if (ppPlane)
			*ppPlane = pPoly->m_pPlane;

		uint32 nVertices = pPoly->m_nVertices;
		if (pVertexList)
		{
			for (uint32 i = 0; i < nVertices; i++)
			{
				if (i >= nVertexListMaxSize)
					break;
				*pVertexList++ = *pPoly->m_Vertices[i].m_pPos;
			}
		}

		if (pnNumVertices)
			*pnNumVertices = nVertices;

		return LT_OK;
	}

	RETURN_ERROR(2, "CommonLT::GetPolyInfo", LT_ERROR);
}

// AVP2: 0x10039ed4 -- the poly's surface's flags; an error without a poly or
// a surface.
LTRESULT ServerCommonLT::GetPolySurfaceFlags(HPOLY hPoly, uint32& dwSurfFlags)
{
	WorldPolyAvP2* pPoly = (WorldPolyAvP2*)m_pServerMgr->m_World.FindPoly(hPoly);
	if (pPoly && pPoly->m_pSurface)
	{
		dwSurfFlags = pPoly->m_pSurface->m_nFlags;
		return LT_OK;
	}

	RETURN_ERROR(2, "CommonLT::GetPolySurfaceFlags", LT_ERROR);
}

// ONE `return LT_OK`, reached through nested positive tests, not three early
// returns. That is what decides where edi is saved. The shipped build
// shrink-wraps it: `push %edi` comes after the null guard (0x1002d77b, /O2
// 0x0046c1dd), and the guard's error return skips `pop %edi` (/O1 jumps to
// `pop %esi` at 0x1002d827; /O2 gives it its own epilogue with no edi).
// VC6 saves a callee-saved register around the smallest single-exit region
// that holds its variable (edi is dwFlags, used only in the OFT_Flags arm).
// With `if (nOld == dwFlags) return LT_OK;` and a second `return LT_OK` at the
// end of that arm, those returns leave the region separately and meet the
// error return at the epilogue, so edi was saved at entry. Written as one
// shared return, the whole post-guard part is a single-exit region and the
// save moves down to it, as shipped. Byte-identical on both images.
// Measured and negative with the early returns kept: else after the guard; a
// function-scope name static with RETURN_ERROR_SYM; a copy of dwFlags (block-
// scoped, function-scoped, `register`, in one arm or both); nOld/nType/nPrev at
// function scope; `dwFlags == nOld`; the branches inverted; a switch; the guard
// written positively with the body inside it (250 bytes); the body moved into
// an inline helper (not expanded); CL_REV 8168 and 8966.
LTRESULT ServerCommonLT::SetObjectFlags(HOBJECT hObj, const ObjFlagType flagType, uint32 dwFlags)
{
	if (!hObj)
		RETURN_ERROR_NAMED(2, "ServerCommonLT::SetObjectFlags", LT_INVALIDPARAMS);

	if (flagType == OFT_Flags)
	{
		uint32 nOld = hObj->m_Flags;
		if (nOld != dwFlags)
		{
			hObj->m_ChangeFlags |= 0x40;

			if ((nOld & FLAG_SOLID) && !(dwFlags & FLAG_SOLID))
				obj_UnlinkTrackersA(hObj);

			// AVP2: the client-visible flags are the low word and bit 18
			// (`test $0x4ffff`, 0x00480cd5; AvP2 1.0.9.6 and both
			// server.dlls agree).
			if ((hObj->m_Flags ^ dwFlags) & 0x4ffff)
				sm_SetObjectChangeFlag(m_pServerMgr, hObj, 8);

			char nType = hObj->m_ObjectType;
			uint32 nPrev = hObj->m_Flags;
			hObj->m_Flags = dwFlags;

			// A world model losing box physics needs its transform re-derived.
			if ((nType == OT_WORLDMODEL || nType == OT_CONTAINER) &&
			    (nPrev & FLAG_BOXPHYSICS) && !(dwFlags & FLAG_BOXPHYSICS))
				obj_RetransformWorldModel(hObj);

			sm_UpdateObjectBBox(m_pServerMgr, hObj);
		}
	}
	else if (hObj->m_Flags2 != dwFlags)
	{
		hObj->m_Flags2 = dwFlags;
		sm_SetObjectChangeFlag(m_pServerMgr, hObj, 8);
	}

	return LT_OK;
}

#if !defined(LT_SERVER_BUILD)
// lithtech.exe CALLS the time reference's constructor, in both games:
// Primal Hunt 0x00480f43 -> 0x004543d0 and AvP2 1.0.9.6 0x0047a1a1 ->
// 0x0044f8f0, `ecx = &state+0x28` in each. The constructor is out of line,
// the SDK body (both weight sets -1, m_bNormalize clear, then Init(NULL...)
// expanded) in a COMDAT of its own that the model walker's eight records
// call too (objbank.cpp, 0x0046d3d8). ltanimtracker.h's constructor is
// in-class, so a type of ours with an out-of-line constructor stands for
// it here (name ours). The record is server.dll's: the time reference at
// +0x28. (An expanding SDK-tracker record here left AvP2's copy 52 bytes
// over, 603 of 551.)
//
// SetObjectFilenames stays NEAR in both. Primal Hunt (642 of 648): the
// attachment loop's two `m_iNode` stores are cross-jumped into one in ours
// (the socket arm jumps to the node arm's `mov %eax,0x20(%esi)`), where
// shipped keeps the socket arm's own store through %edx and jumps to the
// loop tail (0x004810ed..0x004810f1); and the WriteWord argument and the
// client-loop index take the other scratch register. A second found-index
// local for the socket arm did not separate the stores. AvP2 (551 of 551,
// no mnemonic difference): three locals rotate through the two dead
// argument slots -- shipped gives pObj's slot to msg_AllocPacket's
// returned reference and pStruct's to pOldModel, with nFound in the
// frame; ours gives pObj's to pOldModel and pStruct's to nFound.
// Declaration order is indifferent; scoping nFound in the loop is worse.
class CAnimTimeRefPHQ542 : public AnimTimeRef
{
public:
	CAnimTimeRefPHQ542();                    // 0x004543d0 (AvP2 0x0044f8f0)
};

CAnimTimeRefPHQ542::CAnimTimeRefPHQ542()
{
}

struct CSavedFileStateLT
{
	uint8              m_pad_00[0x28];
	CAnimTimeRefPHQ542 m_TimeRef;            // 0x28
	uint8              m_pad_4c[0x50 - 0x4c];
	void*              m_pModelFile;         // 0x50
	void*              m_pSkinFile;          // 0x54
};
#else
// AVP2: the same record, the tracker grown to 0x50 bytes (see
// w_SaveObjectFileState); servercommon.h's CSavedFileState is NOLF's.
struct CSavedFileStateSvQ508
{
	// The tracker's time reference sits twelve bytes further in, at +0x28:
	// SetObjectFilenames constructs it at -0x4c(%ebp) with the record at
	// -0x74 (server.dll 0x10039b57, 0x10039bcb; AvP2 1.0.9.6's -0x40 and
	// -0x68, 0x10036bd7 and 0x10036c45).
	//
	// The SDK's AnimTimeRef (ltanimtracker.h): SetObjectFilenames calls its
	// constructor out of line, the one objbank.obj's model code shares
	// (server.dll 0x1002ee6b: both weight sets -1, m_bNormalize cleared,
	// then Clear; AvP2 0x1002d060).
	uint8         m_pad_00[0x28];
	AnimTimeRef   m_TimeRef;                 // 0x28
	uint8         m_pad_4c[0x50 - 0x4c];
	void*         m_pModelFile;              // 0x50
	void*         m_pSkinFile;               // 0x54
};
#endif

void sm_WriteObjectFiles(LTObject* pObj, ServerMessage* pMsg, uint32* pnSize);  // 0x1003448c (AvP2 0x10031aa9)

LTRESULT ServerCommonLT::SetObjectFilenames(HOBJECT pObj, ObjectCreateStruct* pStruct)
{
	CMessageRef msg;
#ifdef LT_SERVER_BUILD
	CSavedFileStateSvQ508 state;
#else
	CSavedFileStateLT state;
#endif
	Model* pOldModel;
	LTRESULT dResult;
	uint32 nFound;

	// One name variable for both check sites, so the identical error bodies
	// tail-merge (two RETURN_ERROR_NAMEDs would each get their own static).
	static char* pErrFnName = "ServerCommonLT::SetObjectFilenames";

	// ONE refusal for all three tests: lithtech.exe's /O2 copy (0x0046c440)
	// has a single error body at 0x0046c63d that all four branches reach;
	// written as two refusals /O2 kept a second head for the type test.
	// server.dll's /O1 bytes are the same either way.
	char nType;
	if (!pStruct || !pObj ||
	    ((nType = pObj->m_ObjectType) != OT_MODEL && nType != OT_SPRITE))
		RETURN_ERROR_SYM(2, pErrFnName, LT_INVALIDPARAMS);

	pOldModel = LTNULL;
	if (nType == OT_MODEL)
		pOldModel = ((ModelInstance*)pObj)->m_AnimTracker.m_TimeRef.m_pModel;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	w_SaveObjectFileState(pObj, (CSavedFileState*)&state);
	sm_RemoveFromWorldTree(m_pServerMgr, pObj);

	dResult = w_ApplyObjectFilenames(m_pServerMgr, pObj, pStruct);
	if (dResult != LT_OK)
	{
		w_RestoreObjectFileState(pObj, (CSavedFileState*)&state);
		return dResult;
	}

	{
		ServerMessage* pMsg = msg.m_pMsg;
		pMsg->m_Unknown38 = 1;
		pMsg->m_Unknown34 = 1;
	}
	msg.m_pMsg->WriteWord(pObj->m_ObjectID);
	// The writer takes a third argument, a running size, passed as zero here
	// (server.dll 0x10039c33 `push 0`, lithtech.exe 0x0048104a).
	sm_WriteObjectFiles(pObj, msg.m_pMsg, LTNULL);

	// Every client whose table already carries the object learns the names.
	LTLink* pListHead = &m_pServerMgr->m_ClientList.m_Head;
	LTLink* pCur;
	for (pCur = pListHead->m_pNext; pCur != pListHead; pCur = pCur->m_pNext)
	{
		Client* pClient = (Client*)pCur->m_pData;
		if (pClient->m_pObjInfos[pObj->m_ObjectID].m_ChangeFlags & 0x1000)
			sm_SendToClient(m_pServerMgr, pClient, 0xf, msg.m_pMsg, 0, 0x80);
	}

	// Re-bind each attachment's socket/node index onto the new model by name.
	if (pOldModel)
	{
		Model* pNewModel = ((ModelInstance*)pObj)->m_AnimTracker.m_TimeRef.m_pModel;
		// collection.h's GetSize()/operator[] rather than bare m_nElements /
		// m_pArray: identical code on both images, but the accessor
		// expansions are what spend lithtech.exe's /O2 inline budget so that
		// AnimTimeRef::Init, at the top of the function, stays a call
		// (0x0046c465) as shipped. Bare fields: Init expanded, 600 bytes.
		Attachment* pAttach;
		for (pAttach = pObj->m_pAttachments; pAttach; pAttach = pAttach->m_pNext)
		{
			uint32 nOldSockets = pOldModel->m_Sockets.GetSize();
			uint32 iNode = pAttach->m_iNode;

			if (iNode < nOldSockets)
			{
				ModelSocket* pSocket = pOldModel->m_Sockets[iNode];
				if (pNewModel->FindSocket(pSocket->m_Name, &nFound))
					pAttach->m_iNode = nFound;
			}
			else if (iNode < nOldSockets + pOldModel->m_NodeTransforms.GetSize())
			{
				char* pName = pOldModel->m_Nodes[iNode - nOldSockets]->m_pName;
				if (pNewModel->FindNode(pName, &nFound))
					pAttach->m_iNode = pNewModel->m_Sockets.GetSize() + nFound;
			}
		}
	}

	return LT_OK;
}


// A message to fill in is the manager's to hand out; what the caller gets is
// the writer embedded in it, eight bytes along, not the message itself.
LTRESULT ServerCommonLT::CreateMessage(ILTMessage*& pMsg)
{
	ServerMessage* pNew = m_pServerMgr->StartMessage();

	pMsg = (ILTMessage*)&pNew->m_Write;

	return LT_OK;
}

// Inside or outside — both are answers, so neither is an error code.
LTRESULT ServerCommonLT::GetPointStatus(LTVector* pPoint)
{
	return world_IsPointInside(&m_pServerMgr->m_Unknown210, pPoint)
	           ? LT_INSIDE : LT_OUTSIDE;
}

// 0x1002da5f — the light-grid sample is the server interface's own
// (smgr.cpp, 0x1002cee7), and this slot answers whatever it does: two
// arguments straight through, `ret $0x8`. si_GetPointShade answers LTTRUE,
// which is LT_ERROR as an LTRESULT; the forwarder does not translate it.
LTBOOL si_GetPointShade(LTVector* pPoint, LTVector* pColor);

LTRESULT ServerCommonLT::GetPointShade(LTVector* pPoint, LTVector* pColor)
{
	return si_GetPointShade(pPoint, pColor);
}

// NOLF's (0x1002da31, 0x100175fd): AvP2's and Primal Hunt's SetObjectFilenames
// build the SDK's AnimTimeRef instead (its two weight sets and m_bNormalize),
// whose constructor and Clear are objbank.obj's. Kept for the record, not
// built.
#if 0
// 0x1002da31 — the time reference starts on no weight set and no animation.
// Declared in the class and defined here rather than in the class body: /Ob1
// would otherwise expand it into SetObjectFilenames, and the shipped build
// calls it.
CAnimTimeRef::CAnimTimeRef()
{
	m_iWeightSet = INVALID_MODEL_WEIGHTSET;
	Clear();
}

// 0x100175fd — AnimTimeRef::Clear, which is Init(NULL,0,0,0,0,0.0f) with the
// arguments folded (ltanimtracker.h has the era source). Shared with the
// model walker's eight of them, which is why it is a call and not eight
// inline stores at each site. The `fldz` leads the block: the FPU store is
// scheduled first and the seven integer stores then follow declaration order
// off one zero register.
void CAnimTimeRef::Clear()
{
	m_pModel = LTNULL;

	m_PrevAnim = 0;
	m_PrevFrame = 0;
	m_PrevTime = 0;

	m_CurAnim = 0;
	m_CurFrame = 0;
	m_CurTime = 0;

	m_Percent = 0.0f;
}
#endif


// ---- the world-side file swap -------------------------------------------- //

// 0x1002badb and 0x1002bb52 — one pair, mirror images. Both report themselves
// as "BackupExtraData" / "RestoreExtraData", which is the only naming
// information left for either; both check level 1.
//
// Two things read off the bytes rather than guessed:
//
//   * the tracker is copied WHOLE and only for a model — `cmpb $0x1,0xbc` is
//     m_ObjectType against OT_MODEL, and `rep movsl` with a count of 0xe is
//     0x38 bytes, which is exactly sizeof(LTAnimTracker) and exactly what
//     sits at ModelInstance+0x1c0. A sprite therefore saves its two file
//     identifiers and nothing else.
//   * both return an LTRESULT (`xor eax,eax` on the way out, 0x3c on the
//     guard), which SetObjectFilenames discards.
//
// The shipped code keeps `&pObj->m_pSData` in a register across the first
// store and reloads the pointer through it for the second field, because the
// store to the caller's record could alias — which is just what writing the
// two members out separately gives you.
//
// AVP2: the tracker is 0x50 bytes, not 0x38: the copy is `rep movsl` of 0x14
// and the two identifiers sit at 0x50 and 0x54 (0x0047fbaa..0x0047fbcb,
// 0x0047fc27..0x0047fc4b; AvP2's lithtech.exe 0x00478fd2 the same). The
// header's CSavedFileState is NOLF's, so the record is read through this
// view.
struct CSavedTrackerQ508
{
	uint8 m_Data[0x50];
};

struct CSavedFileStateQ508
{
	CSavedTrackerQ508 m_Tracker;             // 0x00
	void*             m_pModelFile;          // 0x50  SObjData +0x30
	void*             m_pSkinFile;           // 0x54  SObjData +0x34
};

LTRESULT w_SaveObjectFileState(LTObject* pObj, CSavedFileState* pState)
{
	CSavedFileStateQ508* pSaved = (CSavedFileStateQ508*)pState;

	if (!pObj || !pObj->m_pSData)
		RETURN_ERROR(1, "BackupExtraData", LT_INVALIDPARAMS);

	pSaved->m_pModelFile = pObj->m_pSData->m_pModelFile;
	pSaved->m_pSkinFile = pObj->m_pSData->m_pSkinFiles[0];

	if (pObj->m_ObjectType == OT_MODEL)
		pSaved->m_Tracker = *(CSavedTrackerQ508*)&((ModelInstance*)pObj)->m_AnimTracker;

	return LT_OK;
}

LTRESULT w_RestoreObjectFileState(LTObject* pObj, CSavedFileState* pState)
{
	CSavedFileStateQ508* pSaved = (CSavedFileStateQ508*)pState;

	if (!pObj || !pObj->m_pSData)
		RETURN_ERROR(1, "RestoreExtraData", LT_INVALIDPARAMS);

	pObj->m_pSData->m_pModelFile = pSaved->m_pModelFile;
	pObj->m_pSData->m_pSkinFiles[0] = pSaved->m_pSkinFile;

	if (pObj->m_ObjectType == OT_MODEL)
		*(CSavedTrackerQ508*)&((ModelInstance*)pObj)->m_AnimTracker = pSaved->m_Tracker;

	return LT_OK;
}


// Server.dll 0x1003448c (lithtech.exe 0x0047a7e0); AvP2 1.0.9.6's
// server.dll 0x10031aa9 -- the writer for an object's files. Not NOLF's
// three identifiers (0x10029087): the model at +0x30 (still dereferenced
// without a null test), four skins at +0x34..+0x40 in a loop counted down
// from four, and in Primal Hunt one more at +0x44. Either half is optional:
// with no message nothing is written, and with a size every identifier adds
// its two bytes to it -- the same walk both writes a message and measures
// one. The counter is sixteen bits and the identifier goes through a
// `uint16` local, as NOLF's had them. Names ours.
class CObjFilesPHQ532
{
public:
	uint8                  m_pad_00[0x30];
	class CFileIdentifier* m_pModelFile;     // +0x30
	class CFileIdentifier* m_pSkins[4];      // +0x34
#ifdef PRIMAL_HUNT
	class CFileIdentifier* m_pFile44;        // +0x44
#endif
};

void sm_WriteObjectFiles(LTObject* pObj, ServerMessage* pMsg, uint32* pnSize)
{
	uint16 i;

	// RETAIL BUG: the model file is dereferenced without the null test the
	// skins get (RETAIL_BUGS.md).
	if (pMsg)
#ifdef AVP2_MODERN
	{
		CFileIdentifier* pFile = ((CObjFilesPHQ532*)pObj->m_pSData)->m_pModelFile;
		uint16 nFileID = pFile ? pFile->m_FileID : (uint16)0xffff;

#ifdef LT_SERVER_BUILD
		pMsg->WriteWord(nFileID);
#else
		((CPacket*)pMsg)->WriteType(nFileID);
#endif
	}
#else
#ifdef LT_SERVER_BUILD
		pMsg->WriteWord(((CObjFilesPHQ532*)pObj->m_pSData)->m_pModelFile->m_FileID);
#else
		((CPacket*)pMsg)->WriteType(((CObjFilesPHQ532*)pObj->m_pSData)->m_pModelFile->m_FileID);
#endif
#endif
	if (pnSize)
		*pnSize += 2;

	for (i = 0; i < 4; i++)
	{
		if (pMsg)
		{
			CFileIdentifier* pFile = ((CObjFilesPHQ532*)pObj->m_pSData)->m_pSkins[i];
			uint16 nFileID;

			if (pFile)
				nFileID = pFile->m_FileID;
			else
				nFileID = 0xffff;

#ifdef LT_SERVER_BUILD
	#ifdef LT_SERVER_BUILD
		pMsg->WriteWord(nFileID);
#else
		((CPacket*)pMsg)->WriteType(nFileID);
#endif
#else
			((CPacket*)pMsg)->WriteType(nFileID);
#endif
		}

		if (pnSize)
			*pnSize += 2;
	}

#ifdef PRIMAL_HUNT
	if (pMsg)
	{
		CFileIdentifier* pFile = ((CObjFilesPHQ532*)pObj->m_pSData)->m_pFile44;
		uint16 nFileID;

		if (pFile)
			nFileID = pFile->m_FileID;
		else
			nFileID = 0xffff;

#ifdef LT_SERVER_BUILD
		pMsg->WriteWord(nFileID);
#else
		((CPacket*)pMsg)->WriteType(nFileID);
#endif
	}

	if (pnSize)
		*pnSize += 2;
#endif
}




// 0x1000f878 is `MatVMul_H` — and that is not a name of ours, it is
// ltmatrix.h's own inline (line 222). Referencing it from here makes VC6
// emit the header's out-of-line COMDAT for it, and that COMDAT is
// byte-identical to the shipped 128 bytes at 0x1000f878, term order and
// reciprocal included. So the SDK header names a shipped function for free,
// and this caller's bare `fstp %st(0)` is the discarded `one_over_w`.

// 0x10012c3a — is a point inside a BSP tree? Zero means it is (worldsim.h
// carries the same declaration as bsp_PointInside; that header is not
// included here).
int bsp_PointInside(class BSPNode* pRoot, LTVector* pPoint);

// 0x10012c9c — the callback world_IsPointInside installs. One object gets
// to veto the point, and only ever in that direction.
//
// Read off the bytes:
//
//   * the two guards are separate `return`s, not an `||`: each has its own
//     jump to the epilogue, and the epilogue they reach is *past* the
//     record store, not a shared tail.
//   * the point is pushed into the object's own space through the second of
//     the two world-model 4x4s (+0x230), which is the pair's inverse — the
//     same matrix sm_GatherContainers uses for the same purpose, and an
//     independent confirmation of which of +0x1f0/+0x230 is which.
//   * `bsp_PointInside`'s two arguments are evaluated right to left with a
//     call in between, which is why `&vLocal` is pushed BEFORE the
//     `GetRoot()` that produces the first argument. Reading that push as
//     the virtual's own argument makes the virtual look like it takes one,
//     and it takes none (`ret`, not `ret $4`).
//   * the veto is `and %eax,0xc(%edi)` — three bytes rather than the four
//     an immediate zero would need, VC6 reusing the register the test
//     already proved to be zero. It is a plain store of zero in source.
//   * the source point goes through a named `pSrc` local. server.dll calls
//     MatVMul_H (0x1000f878) either way; lithtech.exe expands it
//     (0x004355fd..0x004356a6) and every row takes its z product before x
//     and y last. With `&pQuery->m_vPoint` passed straight in, /O2 took y
//     first and z last (16 differing instructions, zero mnemonic blocks);
//     the named pointer flips that tie-break to the shipped order.
void world_PointInsideCB(LTObject* pObj, CPointQuery* pQuery)
{
	LTVector vLocal;

	if (pObj->m_Unknown54 != 0)
		return;

	if (!pObj->_p34())
		return;

	LTVector* pSrc = &pQuery->m_vPoint;
	MatVMul_H(&vLocal, &((WorldModelObject*)pObj)->m_Transform230, pSrc);

	if (!bsp_PointInside(pObj->m_pWorldBsp->GetRoot(), &vLocal))
		pQuery->m_bInside = 0;
}


// 0x10012cf3 — is this point inside the world at all? (name ours)
//
// The container is asked for whatever is at the point and answers through a
// callback; the query record carries both the point and the answer, and the
// answer starts as "inside". 0x10012c9c is the only thing that ever changes
// it, and only in one direction: it runs a test through each object it is
// handed and, when that comes back zero, does `and %eax,0xc(%edi)` on the
// record — one dword past the twelve bytes of the point, with eax already
// zero. So a point nothing rejects stays inside, and the callback's argument
// is the record itself.
LTBOOL world_IsPointInside(CUnknown210* pWorld, LTVector* pPoint)
{
	CPointQuery query;

	query.m_vPoint = *pPoint;
	query.m_bInside = 1;

	((IPointQueryWorld*)pWorld)->FindObjectsAtPoint(pPoint, world_PointInsideCB,
	                                                &query, 0);

	return query.m_bInside;
}


