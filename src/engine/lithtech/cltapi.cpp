// *********************************************************************** //
//
//  SCRATCH RECONSTRUCTION -- cluster 0x00404000 (lithtech.exe)
//
//  SIX ROWS.  They are NOT one module; the cluster is the low end of .text
//  where CLTClient and its embedded interface implementations live, so the
//  rows split three ways and each block below says where its functions
//  belong when you integrate:
//
//    0x00405230  CPhysicsLT::MovePushObjects      -> src/engine/lithtech/clientphysics.cpp (DONE, moved)
//    0x00405350  CPhysicsLT::RotatePushObjects    -> src/engine/lithtech/clientphysics.cpp (DONE, moved)
//    0x004054b0  ClientCommonLT::SetObjectFilenames -> a new clientcommon.cpp
//    0x00407ca0  cm_PointInsideContainer (name ours) -> src/engine/lithtech/cltnew.cpp (DONE, moved)
//    0x00404b90  CLTClient's creator              -> probably the END of
//                                                    src/engine/lithtech/clientwin.cpp
//    0x00405780  w_GetPointShade (client copy)    -> DECLINED, see BLOCK E
//
//  Nothing here is meant to compile as one translation unit against the real
//  headers -- the shims at the top of each block exist so the file can be
//  syntax-checked standalone.  Every symbol this file introduces carries a
//  Q404 suffix.
//
// *********************************************************************** //


// ======================================================================= //
//  SHIM BLOCK -- delete all of this on integration; the real headers
//  (ltbasedefs.h, ltobject_internal.h, clientmgr.h, clientphysics.h,
//  de_error.h, iltcommon.h, ltmatrix.h) already carry every name.
// ======================================================================= //
// The scratch file defines this so it can be syntax-checked on its own.  On
// integration, delete every `Q404_STANDALONE_SYNTAX_CHECK` region and let the
// real headers supply these names.
#ifndef Q404_STANDALONE_SYNTAX_CHECK
#define Q404_STANDALONE_SYNTAX_CHECK
#endif

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef int            int32;
typedef float          LTFLOAT;
typedef int            LTBOOL;
typedef unsigned int   LTRESULT;
typedef void*          HMESSAGEWRITE;
#define LTNULL 0
#define LTTRUE 1
#define LTFALSE 0
#define LT_OK 0
#define LT_ERROR 1
#define LT_INVALIDPARAMS 60
#define LT_NOTINITIALIZED 73
#define LT_NOTINWORLD 21
#define OT_MODEL 1
#define OT_WORLDMODEL 2
#define OT_SPRITE 3
#define OT_CONTAINER 9
#define MAX_MODEL_TEXTURES 2

class LTVector
{
public:
	float x, y, z;
	LTVector() {}
	LTVector(float a, float b, float c) { x = a; y = b; z = c; }
	LTVector operator-(const LTVector v) const
	{
		return LTVector(x - v.x, y - v.y, z - v.z);
	}
	LTVector operator+(const LTVector v) const
	{
		return LTVector(x + v.x, y + v.y, z + v.z);
	}
};
#define LTCLAMP(a, min, max) ((a) < (min) ? (min) : ((a) > (max) ? (max) : (a)))
#define VEC_CLAMP(v, a, b) \
	{\
		(v).x = LTCLAMP((v).x, a, b);\
		(v).y = LTCLAMP((v).y, a, b);\
		(v).z = LTCLAMP((v).z, a, b);\
	}
class LTRotation { public: float x, y, z, w; };
class LTMatrix { public: float m[4][4]; };
#define VEC_MAGSQR(v) ((v).x*(v).x + (v).y*(v).y + (v).z*(v).z)

// ltmatrix.h's own inline, quoted so the shim compiles.
inline float MatVMul_H(LTVector* pDest, LTMatrix* pMat, LTVector* pSrc)
{
	float one_over_w = 1.0f / (pMat->m[3][0]*pSrc->x + pMat->m[3][1]*pSrc->y + pMat->m[3][2]*pSrc->z + pMat->m[3][3]);
	pDest->x = one_over_w * (pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z + pMat->m[0][3]);
	pDest->y = one_over_w * (pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z + pMat->m[1][3]);
	pDest->z = one_over_w * (pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z + pMat->m[2][3]);
	return one_over_w;
}

extern "C" void dsi_OnReturnError(int code);
extern int      g_DebugLevel;
extern char*    g_pErrorFormat;
void dsi_ServerError(char* pFormat, ...);
#define LT_ERRSTR_(x) #x
#define LT_ERRSTR(x) LT_ERRSTR_(x)
#define LT_ERRDESC " "
#define REPORT_ERROR(level, fnName, code) \
	dsi_OnReturnError(code); \
	if (g_DebugLevel >= level) \
		dsi_ServerError(g_pErrorFormat, fnName, LT_ERRSTR(code), LT_ERRDESC);
#define RETURN_ERROR(level, fnName, code) \
	{ REPORT_ERROR(level, fnName, code) return code; }
#define RETURN_ERROR_SYM(level, fnName, code) \
	{ \
		dsi_OnReturnError(code); \
		if (g_DebugLevel >= level) \
			dsi_ServerError(g_pErrorFormat, fnName, #code, LT_ERRDESC); \
		return code; \
	}

class CUnknown210;
class CUnknown6A8Obj;
class CCmgMusicMgrQ404;
class ICommonMathQ404;

// clientmgr.h's CClientObject, cut to what this file reads.
class CClientObject
{
public:
	uint8    m_pad_00[0xbc];
	char     m_ObjectType;                   // 0x0bc
	uint8    m_BlockingPriority;             // 0x0bd
	uint8    m_pad_be[0x18c - 0xbe];
	LTVector m_Pos;                          // 0x18c
};
typedef CClientObject* HOBJECT;
typedef CClientObject* HLOCALOBJ;

// clientmgr.h / clientmgr3.cpp, cut to what this file reads.
class CClientMgr
{
public:
	uint8              m_pad_00[0x6c];
	uint8              m_World[0x774 - 0x6c];              // 0x06c
	uint8              m_SoundMgr[4];                      // 0x774
	// 0x00404c81 `mov 0x21b0(%edi)` is the only CClientMgr field past
	// m_SoundMgr this file reads, so everything between is padding.
	// (clientmgr3.cpp names +0x21b0 m_pMusicMgr.)
	uint8              m_pad_778[0x21b0 - 0x778];
	CCmgMusicMgrQ404*  m_pMusicMgr;                        // 0x21b0
};

struct WMMoveState
{
	CUnknown210*    m_pContainer;            // 0x00
	CUnknown6A8Obj* m_pUnknown04;            // 0x04
	HOBJECT         m_pObj;                  // 0x08
	uint32          m_nUnknown0C;            // 0x0c
	uint32          m_pPushList;             // 0x10
	uint32          m_nPushCount;            // 0x14
	uint8           m_pad_18[0x64 - 0x18];
	uint32          m_nUnknown64;            // 0x64
};

void sub_451000(WMMoveState* pState, LTVector vPos, uint32 nFlags);
void wm_RotateWorldModel(WMMoveState* pState, LTRotation* pRotation, int bUnknown);

class CClientFileRef
{
public:
	CClientFileRef() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }
	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

class ObjectCreateStruct
{
public:
	uint8 m_pad_00[0x7c];
	char  m_Filename[0x80];                                    // 0x7c
	char  m_SkinNames[MAX_MODEL_TEXTURES][0x80];               // 0xfc
};

class CClientCreateRequest
{
public:
	CClientFileRef      m_ObjectFile;                        // 0x00
	CClientFileRef      m_SkinFiles[MAX_MODEL_TEXTURES];     // 0x0c
	ObjectCreateStruct* m_pStruct;                           // 0x24
};


// BLOCK A (CPhysicsLT::MovePushObjects / RotatePushObjects) is integrated:
// both now live in src/engine/lithtech/clientphysics.cpp.


// ======================================================================= //
//  BLOCK B -- a new src/engine/lithtech/clientcommon.cpp (the client's half
//  of ILTCommon, mirroring src/engine/server/servercommon.cpp)
//
//  0x004054b0 is SLOT 1 of vtable 0x004a17c8, and that whole table lines up
//  against src/LT2/sdk/inc/iltcommon.h's declaration order with nothing left
//  over:
//
//    0  0x0046f9a0 ~ILTCommon          9  0x004348a0 Parse
//    1  0x004054b0 SetObjectFilenames  10 0x004348c0 GetObjectType
//    2  0x004347c0 GetObjectFlags      11 0x00434920 GetModelAnimUserDims
//    3  0x00434830 SetObjectFlags      12 0x004349d0 GetRotationVectors
//    4  0x00405cf0 GetAttachmentObjects13 0x004349f0 SetupEuler
//    5  0x00434cd0 GetAttachments      14 0x00434a10 CreateMessage
//    6  0x00434a30 GetAttachmentXform  15 0x00405680 GetPointStatus
//    7  0x00434c90 GetAttachedNodeXform16 0x004056b0 GetPointShade
//    8  0x00434cb0 GetAttachedSockXform17 0x004055b0 GetPolyTextureFlags
//
//  and the class is NAMED: [0x004a78e4] == 0x004a78c0 ==
//  "ClientCommonLT::SetObjectFilenames", with "ClientCommonLT::
//  GetAttachmentObjects" at 0x004a7928 for slot 4.  m_pClientMgr is at
//  +0x10, which is where servercommon.h puts ServerCommonLT::m_pServerMgr --
//  ILTCommon's two pointers at 0x04/0x08 and SharedCommonLT's m_pMath at
//  0x0c.  The CLTClient creator in BLOCK D constructs this object at
//  CLTClient+0x370 and writes 0x10 from its argument, which confirms it
//  independently.
//
//  THE /O1 ORACLE APPLIES BUT ONLY TO THE IDIOM.  server.dll has
//  ServerCommonLT::SetObjectFilenames (0x1002d82c, already reconstructed in
//  src/engine/server/servercommon.cpp) and the two bodies are NOT the same
//  function -- the server saves and rolls back file state and broadcasts to
//  clients, the client just refills a create request and re-runs the object
//  hook.  What carries over is the check idiom: one `static char*
//  pErrFnName` shared by both check sites, RETURN_ERROR_SYM at level 2.
// ======================================================================= //

class AttachmentQ405c;                       // slot 4's handle, below
typedef AttachmentQ405c* HATTACHMENT;

class ClientCommonLT
{
public:
	LTRESULT SetObjectFilenames(HOBJECT pObj, ObjectCreateStruct* pStruct);
	LTRESULT GetAttachmentObjects(HATTACHMENT hAttachment, HOBJECT& hParent, HOBJECT& hChild); // slot 4
	LTRESULT GetPointStatus(LTVector* pPoint);                    // slot 15
	LTRESULT GetPointShade(LTVector* pPoint, LTVector* pColor);   // slot 16
	LTRESULT GetPolyTextureFlags(uint32 hPoly, uint32* pFlags);    // slot 17
	uint8       m_pad_00[0x10];
	CClientMgr* m_pClientMgr;                // 0x10
};

// 0x00420330 (name ours) -- clienttexmgr.cpp already describes it as "the
// two-entry texture pair at object+0x1b8 being reconciled across the
// manager's list at +0x420"; it is __cdecl (pClientMgr, pObject) and is run
// here before a model's files are replaced.  Its only other caller is
// 0x00411c55.
void cm_ReleaseObjectTexturesQ404(CClientMgr* pClientMgr, HOBJECT hObj);

// 0x00477d60 -- src/engine/server/smgrnew.cpp's create-hook dispatcher:
// indexes 0x004ae340 by the object's +0xbc type byte with a stride of eight
// and calls slot 0, forwarding (pClientMgr, pObject, pCreateRequest,
// dResult) unchanged.
LTRESULT cm_RunCreateHookQ404(CClientMgr* pClientMgr, HOBJECT hObj,
                              CClientCreateRequest* pRequest, LTRESULT dResult);


// 0x004054b0, 242 bytes.  Extent verified: 0x004054b0..0x004055a1 with nops
// to 0x004055af and the next function (GetPolyTextureFlags) at 0x004055b0;
// no e8/e9 anywhere in .text lands inside the range.  The enum-suspect note
// "start missing at 0x0040555f" is the shared error block, which is the /O2
// sink target of three `je`s, not a prologue.
//
// SHAPE NOTES
//   * `CClientCreateRequest req;` is the peeled-plus-loop-of-two
//     construction ci_CreateObject (0x0040abc0) also has:
//     the member's own CClientFileRef ctor at 0x004054bb..0x004054c7 and the
//     two skins in a `mov $0x2,%ecx` loop at 0x004054d1.  All three of the
//     peeled ctor's stores SURVIVE here where only one survives in
//     ci_CreateObject, because a branch stands between them and the stores
//     that overwrite them; that is dead-store elimination, not a source
//     difference.
//   * THE THREE FAILURES ARE ONE STATEMENT.  0x004054ea, 0x004054f2 and
//     0x00405500 all `je`/`jne` to 0x0040555f, which sits AFTER the success
//     return at 0x0040555c -- the ||-merged shape.  Written as two separate
//     `if`s (the way the server twin writes them) the merged block would be
//     hosted at the SECOND site, i.e. before the body, which is not what
//     shipped.
//   * The type byte is loaded ONCE at 0x004054f4 and serves three compares,
//     including the `cmp $0x1` at 0x00405502 that the first `je` already
//     proved.  That redundant compare is the "four branches off one cmp"
//     case: VC6 keeps a branch it cannot satisfy.  It is the
//     tell that `if (type == OT_MODEL)` is a SEPARATE statement after the
//     guard rather than an `else`.
//   * m_pClientMgr is re-read at 0x00405506 and again at 0x00405542, so it
//     is written out at each use and not hoisted into a local.
//   * The result is the dispatcher's: there is no `xor %eax,%eax` before the
//     epilogue at 0x00405555, so the call is `return`ed.
LTRESULT ClientCommonLT::SetObjectFilenames(HOBJECT pObj, ObjectCreateStruct* pStruct)
{
	CClientCreateRequest req;
	uint32               i;

	static char* pErrFnName = "ClientCommonLT::SetObjectFilenames";

	req.m_pStruct = LTNULL;

	if (!pStruct || !pObj ||
	    (((CClientObject*)pObj)->m_ObjectType != OT_MODEL &&
	     ((CClientObject*)pObj)->m_ObjectType != OT_SPRITE))
		RETURN_ERROR_SYM(2, pErrFnName, LT_INVALIDPARAMS);

	if (((CClientObject*)pObj)->m_ObjectType == OT_MODEL)
		cm_ReleaseObjectTexturesQ404(m_pClientMgr, pObj);

	req.m_pStruct = pStruct;
	req.m_ObjectFile.m_pFilename = pStruct->m_Filename;
	req.m_ObjectFile.m_nFileType = 0;

	for (i = 0; i < MAX_MODEL_TEXTURES; i++)
	{
		req.m_SkinFiles[i].m_pFilename = pStruct->m_SkinNames[i];
		req.m_SkinFiles[i].m_nFileType = 0;
	}

	return cm_RunCreateHookQ404(m_pClientMgr, pObj, &req, LT_OK);
}


// ----------------------------------------------------------------------- //
// 0x004055b0, 208 bytes -- ClientCommonLT::GetPolyTextureFlags, vtable
// 0x004a17c8 slot 17: the client twin of servercommon.cpp's
// ServerCommonLT::GetPolyTextureFlags, with the world lookup expanded the
// same way lithtech.exe expands the server's (inline FindPoly above its
// caller).  Read off the bytes:
//   * a shell test FIRST (+0x150c, as GetPointShade) failing with a level-2
//     RETURN_ERROR of LT_NOTINITIALIZED (0x49, code string 0x004a7168
//     pushed as an immediate beside the literal name 0x004a78e8);
//   * then `*pFlags = 0`, the manager re-read, the >>16 bound check against
//     +0x190, the +0x18c table and slot 3 on the record's +0x04 -- the
//     trees sit at the CLIENT MANAGER's own +0x18c/+0x190;
//   * the surface's +0x3c flags are a uint16 zero-extended into *pFlags;
//     every miss shares the level-2 LT_ERROR block at 0x00405646.
class WorldPolyQ4055;

class IWorldBspQ4055
{
public:
	virtual void            Unused00() = 0;
	virtual void            Unused04() = 0;
	virtual void            Unused08() = 0;
	virtual WorldPolyQ4055* FindPoly(uint32 hPoly) = 0;     // slot 0x0c
};

class CFileTreeQ4055
{
public:
	uint8           m_pad_00[0x04];
	IWorldBspQ4055* m_pList;                 // 0x04
};

class CSurfaceQ4055
{
public:
	uint8  m_pad_00[0x3c];
	uint16 m_nTextureFlags;                  // 0x3c
};

class WorldPolyQ4055
{
public:
	uint8          m_pad_00[0x28];
	CSurfaceQ4055* m_pSurface;               // 0x28
};

class CClientMgrPolyViewQ4055
{
public:
	uint8            m_pad_00[0x18c];
	CFileTreeQ4055** m_pFileTrees;           // 0x18c
	uint32           m_nFileTrees;           // 0x190
	uint8            m_pad_194[0x150c - 0x194];
	void*            m_pCurShell;            // 0x150c

	inline WorldPolyQ4055* FindPoly(uint32 hPoly);
};

inline WorldPolyQ4055* CClientMgrPolyViewQ4055::FindPoly(uint32 hPoly)
{
	uint32 iWorld = hPoly >> 16;
	if (iWorld >= m_nFileTrees)
		return LTNULL;

	CFileTreeQ4055* pTree = m_pFileTrees[iWorld];
	if (!pTree)
		return LTNULL;

	return pTree->m_pList->FindPoly(hPoly);
}

LTRESULT ClientCommonLT::GetPolyTextureFlags(uint32 hPoly, uint32* pFlags)
{
	if (!((CClientMgrPolyViewQ4055*)m_pClientMgr)->m_pCurShell)
		RETURN_ERROR(2, "CommonLT::GetPolyTextureFlags", LT_NOTINITIALIZED);

	*pFlags = 0;

	WorldPolyQ4055* pPoly = ((CClientMgrPolyViewQ4055*)m_pClientMgr)->FindPoly(hPoly);
	if (pPoly)
	{
		*pFlags = pPoly->m_pSurface->m_nTextureFlags;
		return LT_OK;
	}

	RETURN_ERROR(2, "CommonLT::GetPolyTextureFlags", LT_ERROR);
}


// ----------------------------------------------------------------------- //
// 0x00405680, 40 bytes (0x00405680..0x004056a8; the enumeration's 16-byte
// row stops at the interior branch target 0x00405690) --
// ClientCommonLT::GetPointStatus, vtable 0x004a17c8 slot 15.  The client
// twin of servercommon.cpp's ServerCommonLT::GetPointStatus: the same
// world_IsPointInside (the shared servercommon.cpp body, lithtech's copy
// at 0x004356d0) and the same two-constant select, `neg; sbb; add $0x41`
// == LT_INSIDE (64) for TRUE, LT_OUTSIDE (65) for FALSE.  The client adds a
// world test the server lacks: `mov 0x10(%ecx),%eax; add $0x6c,%eax; jne`
// is a null test of the world pointer itself (the manager's world at
// +0x6c, cltapi.cpp's GetPointShade view puts m_World's shade state inside
// it), refused with a bare LT_NOTINWORLD (0x15) and no report.
#define LT_INSIDE  64
#define LT_OUTSIDE 65

class CClientWorldQ4056;

class CClientMgrWorldViewQ4056
{
public:
	uint8 m_pad_00[0x6c];
	uint8 m_World[4];                        // 0x06c  CClientMgr::m_World
};

// 0x004356d0 -- servercommon.cpp's world_IsPointInside, lithtech's copy.
LTBOOL world_IsPointInsideQ4056(CClientWorldQ4056* pWorld, LTVector* pPoint);

LTRESULT ClientCommonLT::GetPointStatus(LTVector* pPoint)
{
	CClientWorldQ4056* pWorld =
		(CClientWorldQ4056*)((CClientMgrWorldViewQ4056*)m_pClientMgr)->m_World;

	if (!pWorld)
		return LT_NOTINWORLD;

	return world_IsPointInsideQ4056(pWorld, pPoint) ? LT_INSIDE : LT_OUTSIDE;
}


// The two CClientMgr fields GetPointShade reads.  +0xfc is the world's
// shade state (inside m_World, which starts at +0x6c) and +0x150c is the
// shell object clientmgr.h already documents as what IsConnected() tests.
// A view rather than an edit to the CClientMgr shim above, whose layout is
// known to be wrong past 0x774.
class CClientMgrShadeViewQ404
{
public:
	uint8  m_pad_00[0xfc];
	uint8  m_ShadeState[0x150c - 0xfc];      // 0x0fc
	void*  m_pCurShell;                      // 0x150c
};

// 0x00405780 -- the client's w_GetPointShade (BLOCK E; the server twin is
// servermgr.h's 0x1002cf33).
void w_GetPointShadeQ404(uint8* pShadeState, LTVector* pPoint, uint8* pColor);

// 0x004056b0, 197 bytes -- ClientCommonLT::GetPointShade, vtable 0x004a17c8
// slot 16.  CLTClient::GetPointShade (0x00407540, cltnew.cpp) forwards here.
//   * both pointers tested first, the one error block shared (0x00405737):
//     RETURN_ERROR at level 1 (`cmp $0x1; jl`), description 0x004a7164
//     " ", code string 0x004a7160 "60" -- plain LT_INVALIDPARAMS.  The
//     function-name literal is "CLTClient::GetPointShade" (0x004a7908), the
//     same pooled string the forwarder would name; it is pushed as an
//     immediate, so it is a literal and not a static pointer.
//   * no shell -> `mov $0x15,%eax` == LT_NOTINWORLD, no report.
//   * the colour comes back in the same byte order the server twin
//     (smgr.cpp si_GetPointShade) uses: x from [2], y from [1], z from [0].
LTRESULT ClientCommonLT::GetPointShade(LTVector* pPoint, LTVector* pColor)
{
	uint8 color[4];

	if (!pPoint || !pColor)
		RETURN_ERROR(1, "CLTClient::GetPointShade", LT_INVALIDPARAMS);

	if (!((CClientMgrShadeViewQ404*)m_pClientMgr)->m_pCurShell)
		return LT_NOTINWORLD;

	w_GetPointShadeQ404(((CClientMgrShadeViewQ404*)m_pClientMgr)->m_ShadeState,
	                    pPoint, color);

	pColor->x = color[2];
	pColor->y = color[1];
	pColor->z = color[0];
	return LT_OK;
}


// ----------------------------------------------------------------------- //
// 0x00405cf0, 196 bytes -- ClientCommonLT::GetAttachmentObjects, vtable
// 0x004a17c8 slot 4, named by the .data literal at 0x004a7928.  Byte for
// byte servercommon.cpp's ServerCommonLT::GetAttachmentObjects (lithtech's
// copy 0x0046c780): one `static char*` name shared by both level-1 checks
// (`mov 0x4a794c` at each), both ids looked up before either result is
// tested, the lookup here being the client's object table 0x00420520
// (clientmgr2.cpp's tbl_GetObject) on m_pClientMgr (+0x10).
class AttachmentQ405c
{
public:
	uint8  m_pad_00[0x1c];                   // LTransform m_Offset
	uint16 m_nParentID;                      // 0x1c
	uint16 m_nChildID;                       // 0x1e
};

HOBJECT tbl_GetObjectQ405c(CClientMgr* pClientMgr, uint16 nID);    // 0x00420520

LTRESULT ClientCommonLT::GetAttachmentObjects(HATTACHMENT hAttachment, HOBJECT& hParent, HOBJECT& hChild)
{
	static char* pFnName = "ClientCommonLT::GetAttachmentObjects";

	if (!hAttachment)
		RETURN_ERROR(1, pFnName, LT_INVALIDPARAMS);

	AttachmentQ405c* pAttach = (AttachmentQ405c*)hAttachment;
	hParent = tbl_GetObjectQ405c(m_pClientMgr, pAttach->m_nParentID);
	hChild = tbl_GetObjectQ405c(m_pClientMgr, pAttach->m_nChildID);

	if (!hParent || !hChild)
		RETURN_ERROR(1, pFnName, LT_NOTINITIALIZED);

	return LT_OK;
}


// BLOCK C (0x00407ca0 cm_PointInsideWorldModelQ404) is integrated: it now
// lives in src/engine/lithtech/cltnew.cpp as cm_PointInsideWorldModel,
// compiled against the real ltvector.h / ltmatrix.h (still the same
// 397/397 near there).

// ======================================================================= //
//  BLOCK D -- src/engine/lithtech/clientwin.cpp (at the END of it)
//
//  WHERE IT GOES.  COMDAT order in .text is source definition order within
//  a TU, and the decoded object map puts clu_fm_Destroy (0x004040f0),
//  clu_fm_Init (0x004041f0), clu_fm_OnServerFile (0x00404280) and
//  clu_fm_GetFileRec (0x00404680) all in clientwin.obj, while
//  clientphysics.obj does not start until 0x00404cf0/0x00404d10
//  (CPhysicsLT::SetVelocity).  0x00404b90 is the last function before that
//  boundary, so it is clientwin.obj's last COMDAT -- put it at the bottom
//  of src/engine/lithtech/clientwin.cpp rather than in a file of its own,
//  and put the class declarations in clientmgr.h beside CClientMgr.
//
//  0x00404b90 is `new CLTClient(pClientMgr)` with the constructor expanded.
//  It has ONE caller, 0x0040f932, inside CClientMgr's own setup, and the
//  result is stored at pClientMgr+0x4c0:
//
//      40f931  push %ebx           ; ebx == the CClientMgr being built
//      40f932  call 0x404b90
//      40f937  mov  %eax,0x4c0(%ebx)
//
//  WHAT PINS THE CLASS, and it is pinned completely:
//    * `push $0x3c8` at 0x00404b91 is sizeof(CLTClient) -- 968 bytes.
//    * 0x004290b0 is `operator new`.
//    * src/LT2/sdk/inc/iltcsbase.h's protected block is
//        ILTMath m_MathLT; ILTCommon*; ILTPhysics*; ILTModel*;
//        ILTTransform*; ILTLightAnim*; ILTSoundMgr*
//      i.e. 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c -- and every one of
//      those seven is written here, six of them with the address of an
//      implementation embedded further down this same object and the
//      seventh (m_pSoundMgr, 0x1c) with `lea 0x774(%edi)`, which
//      clientmgr.h already names CClientMgr::m_SoundMgr.  So the whole
//      base block is attested, not guessed.
//    * The embedded implementations' vtables and slot counts agree with the
//      SDK headers one for one:
//        +0x370 vtbl 0x004a17c8, 18 slots -> ILTCommon   (ClientCommonLT,
//                                            named in .data, BLOCK B)
//        +0x384 vtbl 0x004a1770, 22 slots -> ILTClientPhysics (CPhysicsLT,
//                                            src/engine/lithtech/clientphysics.h;
//                                            +0x08 == m_StairHeight gets
//                                            0xbf800000 == -1.0f, which is
//                                            exactly what iltphysics.h
//                                            documents as the default)
//        +0x394 vtbl 0x004a1758,  6 slots -> ILTVideoMgr (the .data literal
//                                            "LVideoMgr::StartOnScreenVideo"
//                                            is at 0x004a7950), and
//                                            CLTClient+0x360 is the
//                                            ILTVideoMgr* cltnew.cpp already
//                                            established
//        +0x39c vtbl 0x004a16ec, 27 slots -> ILTModel
//        +0x3a4 vtbl 0x004a16c8,  8 slots -> ILTTransform
//        +0x3a8 vtbl 0x004a16b8,  4 slots -> ILTLightAnim
//        +0x3b0 ctor 0x00488260 (out of line)
//        +0x3b8 vtbl 0x004a1698,  8 slots
//    * +0x3c0 m_pClientMgr and +0x3c4 m_hVideo are cltnew.cpp's, unchanged.
//    * +0x36c takes CClientMgr+0x21b0, which clientmgr3.cpp names
//      m_pMusicMgr.
//    * 0x004347a0 is SharedCommonLT's own constructor (vtable 0x004a236c at
//      +0x00 and m_pMath = 0 at +0x0c -- sharedcommon.h's exact layout) and
//      0x004347b0 is SharedCommonLT::SetMath, whose server twin
//      (0x10012176) sharedcommon.h already describes as "the only writer of
//      m_pMath: CLTServer's constructor hands over the math implementation
//      embedded in itself at +4".  This function does precisely that, with
//      `lea 0x4(%esi)` at 0x00404c60.  That is the single strongest
//      confirmation in the row.
//
//  ORDER.  CWorldBsp::CWorldBsp (server 0x1000abae) settles how cl 6.0 lays
//  a constructor out, and it is NOT what one would assume: base ctor, then
//  MEMBER ctors, THEN the derived vptr store, then the body.  0x00404b90
//  matches that exactly -- 0x004a1810 at 0x00404bba, the eight member ctors
//  through 0x00404c4b, 0x004a14e0 at 0x00404c64, body.  The deleting
//  destructor at 0x00407c00 confirms the two vtables from the other side
//  (0x004a14e0 first, then the members', then 0x004a1810).
//
//  CONFIDENCE.  Lower than the rest of this file: 36 stores, all of them
//  independent, in one inlined blob, so the scheduler has more freedom here
//  than anywhere else in the cluster and the emission order below is read
//  off shipped rather than derived.  Everything structural is pinned; what
//  is not pinned is whether VC6 re-derives the same schedule from this
//  source.
// ======================================================================= //


// ILTMath, 4 bytes: a vptr and nothing else.  vtable 0x004a19c8.
class ILTMathQ404 { public: virtual void _m00(); };

// ILTCommon / SharedCommonLT / ClientCommonLT.  The base's constructor is
// 0x004347a0 and is OUT OF LINE, which is why it survives as a call at
// 0x00404bc0; declared and not defined here on purpose (the whole link
// takes the base's from the TU that defines it).
class ILTCommonQ404
{
public:
	virtual ~ILTCommonQ404() {}              // inline: see ??_GCLTClient below
	// Slot 1 of ILTCommon's own vtable 0x004a1a0c is __purecall (0x004928e7);
	// ClientCommonLT's 0x004a17c8 overrides it (0x004054b0).
	virtual void _c01() = 0;
	class ILTTransformQ404* m_pTransformLT;  // 0x04
	class ClientModelLTQ404* m_pModelLT;     // 0x08
};

class SharedCommonLTQ404 : public ILTCommonQ404
{
public:
	SharedCommonLTQ404();                    // 0x004347a0
	void SetMath(ICommonMathQ404* pMath);    // 0x004347b0
	ICommonMathQ404* m_pMath;                // 0x0c
};

class ClientCommonLTQ404 : public SharedCommonLTQ404
{
public:
	ClientCommonLTQ404(CClientMgr* pClientMgr) { m_pClientMgr = pClientMgr; }
	virtual void _c01();
	virtual void _cc0();
	CClientMgr* m_pClientMgr;                // 0x10
};                                           //      = 0x14

// ILTPhysics / CommonPhysicsLT / CPhysicsLT.  src/engine/lithtech/
// clientphysics.h's layout: m_ClientServerType at 0x04 (ILTPhysics' own),
// m_StairHeight at 0x08, m_pClientMgr at 0x0c.
class ILTClientPhysicsQ404
{
public:
	virtual void _p00();
	uint32 m_ClientServerType;               // 0x04
};

class CPhysicsLTQ404 : public ILTClientPhysicsQ404
{
public:
	// 0x00404bd8: the -1.0f store lands BEFORE the vptr store at
	// 0x00404bdf, so it is an initialiser, not a body assignment.
	CPhysicsLTQ404(CClientMgr* pClientMgr)
		: m_StairHeight(-1.0f)
	{
		m_pClientMgr = pClientMgr;
		m_ClientServerType = 0;
	}
	virtual void _p01();
	float       m_StairHeight;               // 0x08
	CClientMgr* m_pClientMgr;                // 0x0c
};                                           //      = 0x10

// The five eight-byte implementations, all the same shape: a vptr and the
// client manager.
#define Q404_SIMPLE_IMPL(name) \
	class name \
	{ \
	public: \
		name(CClientMgr* pClientMgr) { m_pClientMgr = pClientMgr; } \
		virtual void _v00(); \
		CClientMgr* m_pClientMgr; \
	}

Q404_SIMPLE_IMPL(LVideoMgrQ404);             // +0x394, vtbl 0x004a1758
Q404_SIMPLE_IMPL(ClientModelLTQ404);         // +0x39c, vtbl 0x004a16ec
Q404_SIMPLE_IMPL(ClientLightAnimLTQ404);     // +0x3a8, vtbl 0x004a16b8
Q404_SIMPLE_IMPL(CUnknown3b8Q404);           // +0x3b8, vtbl 0x004a1698

// +0x3a4: a vptr and nothing else -- no store follows 0x004a16c8.
class ILTTransformQ404 { public: virtual void _t00(); };

// +0x3b0: eight bytes, but its constructor is 0x00488260 and lives in
// another translation unit, so it stays a call.
class CUnknown3b0Q404
{
public:
	CUnknown3b0Q404(CClientMgr* pClientMgr); // 0x00488260
	virtual void _u00();
	CClientMgr* m_pClientMgr;
};

// ILTCSBase, abstract: the vtable at 0x004a1810 and iltcsbase.h's protected
// block.
class ILTCSBaseQ404
{
public:
	virtual HMESSAGEWRITE StartHMessageWrite() = 0;
	virtual ~ILTCSBaseQ404() {}              // slot 71 in the real table

	ILTMathQ404         m_MathLT;            // 0x04
	ILTCommonQ404*      m_pCommonLT;         // 0x08
	class ILTPhysicsQ404* m_pPhysicsLT;      // 0x0c
	ClientModelLTQ404*  m_pModelLT;          // 0x10
	ILTTransformQ404*   m_pTransformLT;      // 0x14
	ClientLightAnimLTQ404* m_pLightAnimLT;   // 0x18
	void*               m_pSoundMgr;         // 0x1c
};

class CLTClientQ404 : public ILTCSBaseQ404
{
public:
	CLTClientQ404(CClientMgr* pClientMgr);
	virtual HMESSAGEWRITE StartHMessageWrite();

	// 0x00407c00, 77 bytes, is ??_GCLTClient -- this destructor inlined into
	// the compiler's scalar deleting destructor (vtable 0x004a14e0 slot 71).
	// The three vptr stores are CLTClient's own (0x004a14e0), the one left
	// standing after m_CommonLT's inline chain ~ClientCommonLT ->
	// ~SharedCommonLT -> ~ILTCommon (0x004a1a0c, the same store ILTCommon's
	// own ??_G at 0x0046f9a0 makes), and ILTCSBase's (0x004a1810); no
	// destructor is CALLED, so every one of them is inline.  The body frees
	// +0x36c through bare `operator delete` (0x004290c0) with no destructor
	// call in front of it, and clears the field.
	virtual ~CLTClientQ404()
	{
		if (m_pMusicMgr)
		{
			delete m_pMusicMgr;
			m_pMusicMgr = LTNULL;
		}
	}

	uint8                  m_pad_20[0x360 - 0x20];
	LVideoMgrQ404*         m_pVideoMgr;      // 0x360
	CUnknown3b0Q404*       m_p364;           // 0x364
	CUnknown3b8Q404*       m_p368;           // 0x368
	CCmgMusicMgrQ404*      m_pMusicMgr;      // 0x36c
	ClientCommonLTQ404     m_CommonLT;       // 0x370
	CPhysicsLTQ404         m_PhysicsLT;      // 0x384
	LVideoMgrQ404          m_VideoMgr;       // 0x394
	ClientModelLTQ404      m_ModelLT;        // 0x39c
	ILTTransformQ404       m_TransformLT;    // 0x3a4
	ClientLightAnimLTQ404  m_LightAnimLT;    // 0x3a8
	CUnknown3b0Q404        m_Unknown3b0;     // 0x3b0
	CUnknown3b8Q404        m_Unknown3b8;     // 0x3b8
	CClientMgr*            m_pClientMgr;     // 0x3c0
	uint32                 m_hVideo;         // 0x3c4
};                                           //      = 0x3c8

// 0x00408510 (name ours) -- __cdecl (CLTClient*), and all it does is store a
// run of function-pointer constants from 0x00408b60 upwards into the
// object's +0x20 block.  That block is the obsolete C-entry-point table
// iltcsbase.h's "ALL these functions are obsolete" comment refers to.
void clt_InstallEntryPointsQ404(CLTClientQ404* pClient);


// The constructor.  Member-initialiser list, not assignments in the body:
// each embedded implementation takes the manager, and the eight calls come
// out in declaration order at 0x00404bc0 .. 0x00404c4b.  `inline`: shipped
// has no out-of-line body and 0x00404b90 expands it; /Ob1 expands only
// functions marked inline, so without it this was a 33-byte call.
inline CLTClientQ404::CLTClientQ404(CClientMgr* pClientMgr)
	: m_CommonLT(pClientMgr),
	  m_PhysicsLT(pClientMgr),
	  m_VideoMgr(pClientMgr),
	  m_ModelLT(pClientMgr),
	  m_LightAnimLT(pClientMgr),
	  m_Unknown3b0(pClientMgr),
	  m_Unknown3b8(pClientMgr)
{
	// 0x00404c4c / 0x00404c5a -- the common implementation is handed the two
	// interfaces ILTCommon itself declares, which is why ILTCommon's own
	// m_pTransformLT/m_pModelLT are written from out here.
	m_CommonLT.m_pTransformLT = &m_TransformLT;
	m_CommonLT.m_pModelLT     = &m_ModelLT;
	m_CommonLT.SetMath((ICommonMathQ404*)&m_MathLT);   // 0x00404c6a

	m_pCommonLT    = &m_CommonLT;                      // 0x00404c75
	m_pPhysicsLT   = (ILTPhysicsQ404*)&m_PhysicsLT;    // 0x00404c78
	m_pVideoMgr    = &m_VideoMgr;                      // 0x00404c7b
	m_pMusicMgr    = pClientMgr->m_pMusicMgr;          // 0x00404c8d
	m_pModelLT     = &m_ModelLT;                       // 0x00404c99
	m_pTransformLT = &m_TransformLT;                   // 0x00404ca2
	m_pLightAnimLT = &m_LightAnimLT;                   // 0x00404cab
	m_p364         = &m_Unknown3b0;                    // 0x00404cb4
	m_pSoundMgr    = &pClientMgr->m_SoundMgr;          // 0x00404cc1
	m_p368         = &m_Unknown3b8;                    // 0x00404cc4
	m_hVideo       = 0;                                // 0x00404cca

	clt_InstallEntryPointsQ404(this);                  // 0x00404cd4

	m_pClientMgr = pClientMgr;                         // 0x00404cdc
}


// 0x00404b90, 349 bytes.  Extent verified: 0x00404b90..0x00404cec with nops
// to 0x00404cef; one caller, no e8/e9 lands inside, and the two `ret`s are
// the /O2 success and allocation-failure epilogues (a `new` with
// a constructor guards the whole constructor, and here the returned pointer
// IS the possibly-null allocation, so nothing runs unguarded).
CLTClientQ404* clt_CreateClientQ404(CClientMgr* pClientMgr)
{
	return new CLTClientQ404(pClientMgr);
}


// ======================================================================= //
//  BLOCK E -- 0x00405780, 1386 bytes.  NOT WRITTEN; the
//  analysis follows.  It is NOT library code and NOT an extent defect;
//  it is left on tractability, and the analysis below is the point.
//
//  WHAT IT IS.  It is the client's copy of w_GetPointShade -- the trilinear
//  sample of the world's light grid.  src/engine/server/servermgr.h already
//  declares the server twin:
//
//      void w_GetPointShade(uint8* pShadeState, LTVector* pPoint,
//                           uint8* pColor);                 // 0x1002cf33
//
//  and src/engine/server/smgr.cpp line 2702 already CALLS it from
//  si_GetPointShade.  The twin is 1047 bytes and also unwritten
//  (server.dll sub_1002cf33, 1047 bytes, todo), so ONE
//  source would settle 2,433 bytes across the two images.  The two bodies
//  are the same function instruction for instruction; the size gap is /Ot
//  expanding what /Os declines plus `flds <1.0f>` where /O1 uses `fld1`
//  (client 0x004058b6 vs server 0x1002d027).
//
//  HOW IT IS REACHED ON THE CLIENT.  vtable 0x004a17c8 slot 16 is
//  ILTCommon::GetPointShade == 0x004056b0, and that is one of this
//  function's two callers (0x004056e9); the other is 0x0046bb9a.  The
//  .data literal "CLTClient::GetPointShade" at 0x004a7908 belongs to the
//  forwarder cltnew.cpp already has at 0x00407540.
//
//  THE SHADE-STATE STRUCT, read off both images (offsets from the server's
//  %ebx / the client's %esi):
//
//      +0x00  uint8*  m_pGrid          the cells; FOUR BYTES each
//      +0x08  int     m_nYStride       cells per row   (imul by iy)
//      +0x14  int     m_nMaxX          the clamp bound for ix
//      +0x18  int     m_nMaxY
//      +0x1c  int     m_nMaxZ
//      +0x20  int     m_nZStride       (imul by iz)
//      +0x30  LTVector m_vScale        0x30/0x34/0x38
//      +0x3c  LTVector m_vBase         0x3c/0x40/0x44
//
//  THE ALGORITHM, in full:
//
//    1. v = (*pPoint - m_vBase), then v.x *= m_vScale.x etc.  The
//       difference lands in one vector local and is COPIED into a second
//       before the scaling (server `movsl` x3 at 0x1002cf74, client three
//       integer dword moves at 0x004057c1/0x004057d4/0x004057e7), so there
//       really are two LTVector locals here and the multiply is written
//       against the first one's components.
//    2. ix/iy/iz = (int)v.x/.y/.z -- three ftol calls (client 0x004928c0,
//       server 0x1003e0f0).  v.x is left live in %st across all three.
//    3. each index clamped with an if/else-if pair:
//         if (ix < 0) ix = 0; else if (ix > pShade->m_nMaxX) ix = m_nMaxX;
//       (client 0x0040582c..0x00405869, server 0x1002cfad..0x1002cfe8).
//    4. frac = v - floor(v) for each axis -- three calls to floor
//       (client 0x004930bf, server 0x1004007e), each `fsubrs` back against
//       the same slot.
//    5. p = m_pGrid + (ix + iy*m_nYStride + iz*m_nZStride) * 4.
//    6. twenty-four `movzx` + `fild` + `fstps` -- eight cells (x/x+1 via
//       +0 / +4, y/y+1 via +m_nYStride*4, z/z+1 via +m_nZStride*4), three
//       channels each, and each cell's channels are read 2, 1, 0.
//    7. the trilinear blend, with (1.0f - frac) for each axis.
//    8. three ftol calls writing pColor[2], pColor[1], pColor[0].
//
//  WHY IT IS DECLINED RATHER THAN GUESSED.  Steps 6 and 7 are about two
//  hundred FPU instructions over twenty-four float temporaries whose frame
//  slots are the only thing that fixes their declaration order, and the
//  blend tree is expressed entirely in %st with `fxch`/`fmul %st(n)` --
//  none of which is source-reachable in one pass.  A guessed
//  row of this size costs a build and teaches nothing, so what is
//  given here is the structure above.  A reconstruction should write it
//  against the SERVER twin first (/O1, %ebp-relative, every temporary in a
//  named slot) and only then check the client.
// ======================================================================= //


// ======================================================================= //
//  BLOCK F -- src/engine/lithtech/clientinterface.cpp (after
//  ci_GetGlobalLightScale, 0x0040a950)
//
//  ILTClient function-table entries 0x248 / 0x24c (clientinterface.h
//  already declares both, with these addresses, and clientinterface.cpp
//  installs them).  They sit between ci_GetGlobalLightScale (0x0040a950)
//  and ci_CreateObject (0x0040abc0) in the same COMDAT run.  Written here
//  rather than in clientinterface.cpp; move them there on
//  integration and drop the Q404 view.
//
//  Both clamp CClientMgr::m_vGlobalLightScale (clientmgr.h, +0x768) to
//  [0, 2] with ltvector.h's VEC_CLAMP: 0x004a14d8 is 0.0f, 0x004a1a68 is
//  2.0f (`movl $0x40000000` where the z store is folded to an immediate).
//  g_pClientMgr (0x004b17e0) is reloaded before every component because the
//  float stores may alias it.
// ======================================================================= //

class CClientMgrLightViewQ404
{
public:
	uint8    m_pad_00[0x768];
	LTVector m_vGlobalLightScale;            // 0x768
};
extern CClientMgrLightViewQ404* g_pClientMgrLightQ404;   // == g_pClientMgr, 0x004b17e0

// 0x0040a970, 253 bytes.
void ci_SetGlobalLightScaleQ404(LTVector* pScale)
{
	g_pClientMgrLightQ404->m_vGlobalLightScale = *pScale;
	VEC_CLAMP(g_pClientMgrLightQ404->m_vGlobalLightScale, 0.0f, 2.0f);
}

// 0x0040aa70, 327 bytes.  `operator+` and not `+=`: *pOffset is copied into
// the by-value slot (0x0040aa77..0x0040aa97) AND the sum lands in a second
// temporary (0x0c..0x14) before the three dword stores into the member.
void ci_OffsetGlobalLightScaleQ404(LTVector* pOffset)
{
	g_pClientMgrLightQ404->m_vGlobalLightScale =
		g_pClientMgrLightQ404->m_vGlobalLightScale + *pOffset;
	VEC_CLAMP(g_pClientMgrLightQ404->m_vGlobalLightScale, 0.0f, 2.0f);
}


// ======================================================================= //
//  BLOCK G -- src/engine/lithtech/clienttexmgr.cpp (the SharedTexture
//  cache, between 0x004202f0 and tm_FreeUnmarkedTextures 0x00420420)
//
//  0x00420330 is cm_ReleaseObjectTexturesQ404, declared in BLOCK B above
//  and called by ClientCommonLT::SetObjectFilenames before a model's files
//  are replaced (its other caller is 0x00411c55).  Written here rather than in
//  clienttexmgr.cpp; the views below agree with
//  clienttexmgr.cpp's SharedTextureView (+0x32 count/mark) and CClientMgr's
//  object list at +0x420 (an LTList: head at +0x420, m_pNext at +0x424,
//  m_pData at link+0x08).  The callees are clienttexmgr.cpp's
//  tm_MarkTexture (0x00420060) and tm_FreeTexture (0x0041ffc0).
// ======================================================================= //

class SharedTextureQ3
{
public:
	uint8  m_pad_00[0x32];
	uint16 m_nRefCount : 15;                 // 0x32
	uint16 m_bMarked   : 1;                  // 0x32 bit 15
};

class CTexObjectQ3
{
public:
	uint8            m_pad_00[0x1b8];
	SharedTextureQ3* m_pSkins[2];            // 0x1b8
	uint8            m_pad_1c0[0x208 - 0x1c0];
	uint32           m_Unknown208[2];        // 0x208  non-zero keeps the skin
};

struct LTLinkQ3
{
	LTLinkQ3* m_pPrev;
	LTLinkQ3* m_pNext;
	void*     m_pData;
};

class CClientMgrTexViewQ3
{
public:
	uint8    m_pad_00[0x420];
	LTLinkQ3 m_ObjectListHead;               // 0x420
};

void tm_MarkTextureQ3(SharedTextureQ3* pTexture);                              // 0x00420060
void tm_FreeTextureQ3(CClientMgrTexViewQ3* pClientMgr, SharedTextureQ3* pTex); // 0x0041ffc0

// 0x00420330, 238 bytes.
//   1. drop a skin that repeats an earlier one (0x00420348..0x00420379);
//   2. mark each skin by the flag beside it (0x0042037b..0x004203a5);
//   3. mark every skin any OTHER object on the list still uses;
//   4. free the skins left unmarked and clear the slot.
// Two spellings are load-bearing (each measured): step 2 reads the skin into
// pTex once (with pObj->m_pSkins[i] written out three times the induction
// pointer moves onto the skin array and the loop stays 9 blocks off), and
// step 3 takes the list head into pHead FIRST and walks from pHead->m_pNext
// -- shipped's `mov 0x424(%ecx)` + `lea 0x420(%ecx)` pair; the inline
// `&pClientMgr->m_ObjectListHead` condition gives `add $0x420,%ebp` instead.
void cm_ReleaseObjectTexturesQ404(CClientMgr* pClientMgrIn, HOBJECT hObj)
{
	CClientMgrTexViewQ3* pClientMgr = (CClientMgrTexViewQ3*)pClientMgrIn;
	CTexObjectQ3*        pObj = (CTexObjectQ3*)hObj;
	CTexObjectQ3*        pOther;
	LTLinkQ3*            pCur;
	LTLinkQ3*            pHead;
	uint32               i, j;
	SharedTextureQ3*     pTex;

	for (i = 0; i < 2; i++)
	{
		for (j = i + 1; j < 2; j++)
		{
			if (pObj->m_pSkins[i] == pObj->m_pSkins[j] && pObj->m_pSkins[i])
				pObj->m_pSkins[j] = LTNULL;
		}
	}

	for (i = 0; i < 2; i++)
	{
		pTex = pObj->m_pSkins[i];
		if (pTex)
		{
			if (pObj->m_Unknown208[i])
				pTex->m_bMarked = 1;
			else
				pTex->m_bMarked = 0;
		}
	}

	pHead = &pClientMgr->m_ObjectListHead;
	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		pOther = (CTexObjectQ3*)pCur->m_pData;
		if (pOther != pObj)
		{
			for (i = 0; i < 2; i++)
				tm_MarkTextureQ3(pOther->m_pSkins[i]);
		}
	}

	for (i = 0; i < 2; i++)
	{
		if (pObj->m_pSkins[i] && !pObj->m_pSkins[i]->m_bMarked)
		{
			tm_FreeTextureQ3(pClientMgr, pObj->m_pSkins[i]);
			pObj->m_pSkins[i] = LTNULL;
		}
	}
}
