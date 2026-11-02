// lithtech.exe — the two halves of one object's save record: sav_SaveObject
// (0x00431ac0) and sav_RestoreObject (0x00432740).
//
// BOTH BELONG IN src/engine/server/saveio.cpp. That file already declares
// them (lines 282 and 614) and already defines every other function in the
// translation unit; these two are the only bodies it is missing, which is why
// nothing in the decoded symbol map covers 0x00431ac0 or
// 0x00432740 while its neighbours 0x00431770 (sav_SaveObjects),
// 0x00432130 (sav_SaveObjectExtra), 0x00432200 (sm_RestoreObjects) and
// 0x004331f0 (sm_RestoreNextObject) are all there as `near`.
//
// This file is written to COMPILE ON ITS OWN so it can be measured before it
// is merged. Every declaration between here and the first function body is a
// verbatim copy of saveio.cpp's, plus the fields these two functions reach
// that saveio.cpp's views did not need. ON MERGE: delete the duplicated
// views, fold the added fields into saveio.cpp's copies, and drop the two
// forward declarations at lines 282/614 -- the definitions then stand where
// the declarations were.
//
// Where the two are in the image, and against what:
//
//   sav_SaveObject     lithtech 0x00431ac0 (1242 bytes, /O2)
//                      server   0x1000fc39 (1111 bytes, /O1)
//   sav_RestoreObject  lithtech 0x00432740 (2730 bytes, /O2)
//                      server   0x100106b6 (1885 bytes, /O1)
//
// Both extents were checked: there is no branch target
// leaving either range, the image has lea-nop padding at both ends of each
// (0x00431aba..0x00431abf and 0x00431f9a..0x00431f9f;
// 0x0043273e..0x0043273f and 0x004331ea..0x004331ef), and a sweep of every
// e8/e9 in .text finds exactly one displacement landing inside either body
// (0x0043294e -> 0x00432f86, which is this function's own jump to its
// cleanup). Callers agree with the names: 0x00431ac0 is called once, from
// 0x00431a32 inside sav_SaveObjects, and 0x00432740 twice, from 0x00432580
// and 0x004325ab inside sm_RestoreObjects -- which is exactly the peeled
// call plus do/while sm_RestoreObjects was reconstructed with.
//
// Build /O2 for lithtech.exe; the same source is built /O1 into server.dll
// and both listings were read for every decision below. Where they differ it
// is inlining: /O2 expands sav_WriteVector and the CPacket templates, /O1
// calls them.
#include <string.h>
#include <stdio.h>
#include "de_error.h"
#include "saveio.h"
#include "packet.h"

// ---- views (copied from saveio.cpp; see the merge note above) ---------- //

// ltserverobj.h's ClassDef.
struct SavClassDef
{
	char*        m_ClassName;                // 0x00
	SavClassDef* m_ParentClass;              // 0x04
	uint32       m_ClassFlags;               // 0x08
};

// An LTLink as the engine lays it out (ltlink.h).
struct SavLink
{
	SavLink* m_pPrev;                        // 0x00
	SavLink* m_pNext;                        // 0x04
	void*    m_pData;                        // 0x08
};

// ltserverobj.h's BaseClass. Slot 1 is EngineMessageFn.
class SavBaseClass
{
public:
	virtual ~SavBaseClass();
	virtual uint32 EngineMessageFn(uint32 messageID, void* pData, float fData);
};

// ltserverobj.h's message ids. The save half pushes 8 (0x00431f3d) and the
// restore half of the pair -- sm_RestoreNextObject, already reconstructed --
// pushes 7, and ltserverobj.h gives MID_LOADOBJECT as 7, so 8 is the next
// one along: MID_SAVEOBJECT.
#define SAV_MID_LOADOBJECT 7
#define SAV_MID_SAVEOBJECT 8

// servermgr.h's Client, at the two fields the record carries.
class SavClient
{
public:
	uint8  m_pad_000[0x3c0];
	uint32 m_ClientFlags;                    // 0x3c0  servermgr.h's name
	char*  m_pName;                          // 0x3c4  servermgr.h's name
};

// ltobject_internal.h's SObjData. saveio.cpp's copy of this view stops at
// m_pClass; the fields from m_hIdentifier down are the ones these two
// functions reach, and every name outside m_hFiles is ltobject_internal.h's.
//
// m_hFiles is ours and it has to be an array: sav_SaveObject's model arm
// (0x00431c2b) walks it with a strength-reduced byte offset starting at 0x34
// and stepping 4 for two iterations, and the restore's matching arm
// (0x00432c5f) does the same. ltobject_internal.h names 0x30 m_pModelFile
// and 0x34 m_pSkinFile, which is the [0] and [1] of this array; nothing
// before this named 0x38.
//
// m_pSaveData at 0x2c is a CPacket*: the record's length comes from +0x34
// and its bytes from +0x54, which are CPacket::m_DataLen and
// CPacket::m_Data.m_pData (packet.h, 0x50 + 0x04). ltobject_internal.h had
// the field as a uint32 "nonzero keeps the object out of the inactive
// sweep", which is consistent -- an object holding save data is not idle --
// but the two dereferences here say it is a pointer.
class SavObjData
{
public:
	SavLink      m_Links;                    // 0x00
	void*        m_hIdentifier;              // 0x0c
	float        m_NextUpdate;               // 0x10
	uint8        m_pad_14[0x1c - 0x14];
	SavClient*   m_pOwningClient;            // 0x1c
	SavBaseClass* m_pBaseClass;              // 0x20
	SavClassDef* m_pClass;                   // 0x24
	uint8        m_pad_28[0x2c - 0x28];
	CPacket*     m_pSaveData;                // 0x2c  (name ours)
	void*        m_hFiles[3];                // 0x30  (name ours; see above)
};

// worldbsp.h's world, at the one field the save reads: the name, which is
// inline at +0x04 (ltobject_internal.h's IWorldBsp::m_Name).
class SavWorldBsp
{
public:
	uint8 m_pad_00[0x04];
	char  m_Name[0xc4 - 0x04];               // 0x04
};

#define SAV_NOINDEX 0xffff

// ltobject_internal.h's LTObject, at the fields the record carries. Every
// name here is ltobject_internal.h's except the four suffixed by address,
// which are the model arm's three words and the container's one; nothing
// reconstructed so far reaches them.
class SavObject
{
public:
	uint8       m_pad_000[0x088];
	uint32      m_Flags;                     // 0x088
	uint32      m_Flags2;                    // 0x08c
	uint32      m_UserFlags;                 // 0x090
	uint8       m_ColorR;                    // 0x094
	uint8       m_ColorG;                    // 0x095
	uint8       m_ColorB;                    // 0x096
	uint8       m_ColorA;                    // 0x097
	class SavObjLink* m_pLinks;              // 0x098
	float       m_Rotation[4];               // 0x09c
	float       m_Scale[3];                  // 0x0ac
	uint16      m_ObjectID;                  // 0x0b8
	uint16      m_nSaveIndex;                // 0x0ba
	char        m_ObjectType;                // 0x0bc  OT_*
	uint8       m_BlockingPriority;          // 0x0bd
	uint8       m_pad_0be[0x0c4 - 0x0be];
	float       m_Velocity[3];               // 0x0c4
	float       m_Acceleration[3];           // 0x0d0
	float       m_FrictionCoefficient;       // 0x0dc
	float       m_Mass;                      // 0x0e0
	float       m_ForceIgnoreLimitSqr;       // 0x0e4
	uint8       m_pad_0e8[0x120 - 0x0e8];
	uint32      m_ChangeFlags;               // 0x120
	uint8       m_pad_124[0x188 - 0x124];
	SavObjData* m_pSData;                    // 0x188
	float       m_Pos[3];                    // 0x18c
	float       m_Dims[3];                   // 0x198
	float       m_Radius;                    // 0x1a4
	union {
		SavWorldBsp* m_pWorldBsp;            // 0x1a8
		float        m_LightRadius;          // 0x1a8  (OT_LIGHT only)
	};
	uint8       m_pad_1ac[0x1d8 - 0x1ac];
	uint16      m_Model1d8;                  // 0x1d8  (name ours)
	uint8       m_pad_1da[0x1e8 - 0x1da];
	uint16      m_Model1e8;                  // 0x1e8  (name ours)
	uint8       m_pad_1ea[0x1ec - 0x1ea];
	uint32      m_Model1ec;                  // 0x1ec  (name ours)
	uint8       m_pad_1f0[0x270 - 0x1f0];
	uint16      m_Container270;              // 0x270  (name ours)
};

// servermgr.h's CFileMgr, embedded in the manager at 0xa2c.
class SavFileMgr
{
public:
	uint8 m_pad_00[0x48];
};

// servermgr.h's ConsoleState, embedded at 0xa74 (saveio.cpp's copy).
class SavConsoleState
{
public:
	uint8 m_pad_00[0x3c];
	void* m_hVarHash;                        // 0x3c  (manager +0xab0)
	uint8 m_pad_40[0x4c - 0x40];
};

// servermgr.h's CServerMgr, at the offsets these two reach. m_SkyObjects is
// servermgr.h's name for the thirty object ids at 0x95c (MAX_SKYOBJECTS is
// thirty in ltbasedefs.h, and 0x00431e2b compares the index against 0x1e).
class SavServerMgr
{
public:
	uint8            m_pad_000[0x390];
	uint8            m_Unknown390[4];        // 0x390  the object manager
	uint8            m_pad_394[0x804 - 0x394];
	void*            m_pClassMgr;            // 0x804
	uint8            m_pad_808[4];
	void*            m_ClassMgrFirst;        // 0x80c
	uint8            m_pad_810[0x95c - 0x810];
	uint16           m_SkyObjects[30];       // 0x95c
	uint8            m_pad_998[0xa2c - 0x998];
	SavFileMgr       m_FileMgr;              // 0xa2c
	SavConsoleState  m_ConsoleState;         // 0xa74
};

// 0x0042d060 (server 0x1000ddfb) — dhash.cpp's dhash_GetKey: the key an
// element was filed under. servermessage.cpp's CLTServer::GetObjectName
// resolves an object's name exactly this way, through m_hIdentifier, and
// falls back on the same empty string.
char* dhash_GetKey(void* hElement, uint32* pLen);

// 0x0046bb60 (server 0x1002bfcd) — servermgr.h's fs_GetFileName.
char* fs_GetFileName(SavFileMgr* pFileMgr, void* hFile);

// 0x004b0ab4 (server 0x10056ff4) — the shared zero-initialised byte every
// nameless thing in the engine reports. servermessage.cpp calls it
// g_szNoName for this exact use.
extern char g_szNoName[];

// 0x00433d50 (server 0x10034688) — saveio.cpp's name for the manager sweep
// the restore runs between passes.
void sm_Sub34688(SavServerMgr* pMgr);

// STATIC, and the image is what says so. sav_SaveObject calls it twice; the
// /O1 server build emits both as `call 0x100110dc` and the body once, at the
// very end of the module. The /O2 lithtech build expands both call sites in
// place (0x00431b6a and 0x00431dc7, each three Writes off one CSE'd base)
// AND EMITS NO OUT-OF-LINE BODY AT ALL: lithtech's saveio.obj ends with
// RestoreInterlinks at 0x004334a0..0x00433566 and the next function,
// 0x00433570, is a 21-byte thiscall constructor from another module. A
// non-static function is emitted whether or not its calls were inlined, so
// the only way the body can be absent is internal linkage. saveio.cpp
// currently declares it non-static; that is a lithtech-side defect.
// STATIC, and inlined at both call sites in the /O2 build (0x00431b6a and
// 0x00431dc7). The body is saveio.cpp's; it is repeated here only so this
// file compiles and measures on its own.
//
// `inline`, AND THAT IS THE LOAD-BEARING WORD -- not the position.
//
// This was previously declared here and defined 200 lines down, below both
// call sites, and the diagnosis was that VC6's forward-only inliner could not
// reach it from there. Moving the definition up here was tried first and
// changed NOTHING: 1199 bytes before and 1199 after. Position was not the
// problem.
//
// /Ob1 expands only what is marked `inline`/`__inline` or defined inside a
// class. A plain `static` free function does not qualify wherever it sits.
// Adding `inline` took this function from 1199 to 1240 against the shipped
// 1238, and to 447 instructions against 447. That is the whole gap.
//
// Worth remembering for the rest of the tree: "defined below its callers" is
// a real VC6 constraint, but it is the SECOND condition, not the first.
static inline void sav_WriteVector(ILTStream* pStream, float* pVec)
{
	pStream->Write(&pVec[0], 4);
	pStream->Write(&pVec[1], 4);
	pStream->Write(&pVec[2], 4);
}


// saveio.cpp's CSaveLoadMessage, copied verbatim (delete on merge). The
// vtable it plants is 0x004a22d0 in lithtech (0x1004d660 in server.dll),
// which is the same one sm_RestoreNextObject plants -- 0x004331f0 stores the
// identical constant at 0x0043321b.
class CSaveLoadMessage
{
public:
	CSaveLoadMessage(void* pClassMgr, ILTStream* pStream)
	{
		m_pStream = pStream;
		m_pClassMgr = pClassMgr;
	}

	virtual LTRESULT _v00();

	void*      m_pClassMgr;                  // 0x04
	ILTStream* m_pStream;                    // 0x08
};


// ---- 0x00431ac0 — one object's record ---------------------------------- //
//
// The record is two back-patched offsets, the class name, the object's own
// state, and then whatever the object itself writes in answer to
// MID_SAVEOBJECT, closed by the 0xABCDEFAF guard word. The two offsets are
// written as zeroes first and filled in once their targets are known, which
// is why the position of each field is captured before it is written and
// seeked back to afterwards; sm_RestoreNextObject reads them back in the
// same order (end, then start) and seeks to the start rather than assuming
// where the data begins.
//
// Order in the two `if` ladders is the bytes': the world-model arm is tested
// before the model arm, and each has a lone `else`, so both are if/else and
// neither is a guard. The arms name the object's files -- a world model
// carries its world's name inline, everything else names the model file it
// was built from, and a model names its two skins where everything else
// names one.
//
// The three object types that carry extra state each get their own test
// against the same byte: OT_LIGHT's radius shares LTObject's world pointer
// (the union in ltobject_internal.h, and this is the code that needs it),
// OT_CONTAINER has one word, OT_MODEL has three.
//
// Two of the writes are guarded by a `char` flag that goes into the stream
// ahead of them, and both re-read the flag out of memory after the write
// rather than keeping the pointer they set it from -- which is what says the
// source tests the flag and not the pointer. Those two are the object's
// save-data packet and its owning client.
//
// Byte-identical (1242). The five spellings that close it are the ones that
// closed the /O1 twin in server/savrest.cpp, and its comments carry the
// evidence: each fs_GetFileName result named as its own statement, a uint16
// skin index (strength-reduced `mov ebx,2 ... dec ebx ; jne` at 0x00431c2b),
// a uint32 sky index compared as (uint16) (`cmp ax,0x1e` at 0x00431e27), the
// message ctor storing m_pStream first, and nStartFieldPos scoped to a block
// that is sibling to the MID_SAVEOBJECT block (the float temp at 0x00431f32
// sits on its slot).
void sav_SaveObject(SavServerMgr* pMgr, SavObject* pObj, ILTStream* pStream,
                    uint32 dwParam)
{
	CSaveLoadMessage msg(pMgr->m_pClassMgr, pStream);
	uint32      nEndFieldPos;
	uint32      nSkyIndex;
	uint32      nAnim;
	uint32      nValue;
	char        bHasSaveData;
	char        bHasClient;
	char*       pName;

	nValue = 0;

	// The two back-patched fields, each written as a zero over which the
	// real offset goes later.
	pStream->GetPos(&nEndFieldPos);
	pStream->Write(&nValue, 4);

	{
		uint32 nStartFieldPos;

		pStream->GetPos(&nStartFieldPos);
		pStream->Write(&nValue, 4);

		pStream->WriteString(pObj->m_pSData->m_pClass->m_ClassName);

		pStream->Write(&pObj->m_ObjectType, 1);
		pStream->Write(&pObj->m_Flags, 4);
		pStream->Write(&pObj->m_Flags2, 4);

		sav_WriteVector(pStream, pObj->m_Pos);

		pStream->Write(pObj->m_Scale, 0x0c);
		pStream->Write(pObj->m_Rotation, 0x10);
		pStream->Write(&pObj->m_UserFlags, 4);

		pStream->WriteString(pObj->m_pSData->m_hIdentifier
		                     ? dhash_GetKey(pObj->m_pSData->m_hIdentifier, 0)
		                     : g_szNoName);

		if (pObj->m_ObjectType == OT_WORLDMODEL)
		{
			pStream->WriteString(pObj->m_pWorldBsp->m_Name);
		}
		else
		{
			pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[0]);
			pStream->WriteString(pName);
		}

		if (pObj->m_ObjectType == OT_MODEL)
		{
			uint16 i;

			for (i = 1; i < 3; i++)
			{
				pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[i]);
				pStream->WriteString(pName);
			}
		}
		else
		{
			pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[1]);
			pStream->WriteString(pName);
		}

		if (pObj->m_ObjectType == OT_LIGHT)
			pStream->Write(&pObj->m_LightRadius, 4);

		if (pObj->m_ObjectType == OT_CONTAINER)
			pStream->Write(&pObj->m_Container270, 2);

		pStream->Write(&pObj->m_pSData->m_NextUpdate, 4);
		pStream->Write(&pObj->m_BlockingPriority, 1);

		bHasSaveData = (pObj->m_pSData->m_pSaveData != LTNULL);

		pStream->Write(&bHasSaveData, 1);

		if (bHasSaveData)
		{
			pStream->Write(&pObj->m_pSData->m_pSaveData->m_DataLen, 2);
			pStream->Write(pObj->m_pSData->m_pSaveData->m_Data.m_pArray,
			               pObj->m_pSData->m_pSaveData->m_DataLen);
		}

		pStream->Write(&pObj->m_ColorR, 1);
		pStream->Write(&pObj->m_ColorG, 1);
		pStream->Write(&pObj->m_ColorB, 1);
		pStream->Write(&pObj->m_ColorA, 1);

		pStream->Write(pObj->m_Velocity, 0x0c);
		pStream->Write(pObj->m_Acceleration, 0x0c);
		pStream->Write(&pObj->m_FrictionCoefficient, 4);
		pStream->Write(&pObj->m_Mass, 4);
		pStream->Write(&pObj->m_ForceIgnoreLimitSqr, 4);

		sav_WriteVector(pStream, pObj->m_Dims);

		pStream->Write(&pObj->m_ChangeFlags, 4);

		// Which of the thirty sky slots this object is in. The walk does not
		// stop at the first hit -- the store is inside the loop and the loop
		// runs to the end -- so the LAST slot naming the object wins, and an
		// object in none of them goes out as slot zero.
		if (pObj->m_ChangeFlags & 0x100)
		{
			uint32 i;

			nSkyIndex = 0;

			for (i = 0; (uint16)i < 30; i++)
			{
				if (pMgr->m_SkyObjects[i] == pObj->m_ObjectID)
					nSkyIndex = i;
			}

			pStream->Write(&nSkyIndex, 2);
		}

		if (pObj->m_ObjectType == OT_MODEL)
		{
			nAnim = pObj->m_Model1e8;

			pStream->Write(&nAnim, 4);
			pStream->Write(&pObj->m_Model1ec, 4);
			pStream->Write(&pObj->m_Model1d8, 2);
		}

		bHasClient = (pObj->m_pSData->m_pOwningClient != LTNULL);

		pStream->Write(&bHasClient, 1);

		if (bHasClient)
		{
			pStream->Write(&pObj->m_pSData->m_pOwningClient->m_ClientFlags, 4);
			pStream->WriteString(pObj->m_pSData->m_pOwningClient->m_pName);
		}

		// Back-patch the start-of-data offset and come back to it.
		{
			uint32 nPos;

			pStream->GetPos(&nPos);
			nValue = nPos;
			pStream->SeekTo(nStartFieldPos);
			pStream->Write(&nValue, 4);
			pStream->SeekTo(nValue);
		}
	}

	{
		uint32 nPos;

		pObj->m_pSData->m_pBaseClass->EngineMessageFn(SAV_MID_SAVEOBJECT, &msg,
		                                              (float)dwParam);

		nPos = SAVE_SENTINEL;

		pStream->Write(&nPos, 4);

		// And the end-of-data offset, which is what tells the restore where the
		// next object's record starts.
		pStream->GetPos(&nPos);
		nValue = nPos;
		pStream->SeekTo(nEndFieldPos);
		pStream->Write(&nValue, 4);
		pStream->SeekTo(nValue);
	}
}


// ======================================================================== //
//  0x00432740 — sav_RestoreObject, the other half of the record above.
//
//  READ THIS BEFORE BUILDING IT. Everything from here down needs the real
//  engine headers, not saveio.cpp's local views, because the /O2 lithtech
//  build EXPANDS six things inline that the /O1 server build calls out of
//  line, and an expansion cannot happen unless the body is visible:
//
//    expansion            server.dll call   where the body lives now
//    -------------------  ----------------  --------------------------------
//    ObjectCreateStruct                     ltbasedefs.h (already inline);
//      ::Clear()          0x10001bd7        called once, expanded once
//    w_ReadVector         0x1000d040        commonmodel.cpp, already `inline`
//    sm_AllocateObject    0x10001c54        structbank.cpp, NOT inline --
//                                           see the note at its call below
//    CMessageRef::Init    0x1000ebab        servermsg.h, already `inline`
//    ServerMessage::Init  0x10010da9        servermsg.h, already `inline`
//    SObjData's ctor and  0x10010e03        NOWHERE -- ltobject_internal.h
//      its destructor                       has no constructor on SObjData
//
//  The last two rows are why this file carries SavObjDataQ430 below instead
//  of using ltobject_internal.h's SObjData: the shipped constructor is a
//  real function in server.dll (0x10010e03, two callers, the other being
//  smgr.cpp's SObjData bank at 0x1002ce15) and the /O2 build expands it, so
//  the class has to have one.
//
//  THE FRAME IS THE PROOF THAT THE OBJECT SIZES BELOW ARE RIGHT. Both builds
//  reserve 0x824 bytes and both accounts close exactly:
//
//    /O2 lithtech (esp-relative, S = esp after `sub esp,0x824`)
//      S+0x000  scalars                       0x03c
//      S+0x03c  SObjData      sData           0x054
//      S+0x090  WMMoveState   moveState       0x068
//      S+0x0f8  CObjBase      tempObject      0x1a8
//      S+0x2a0  ObjectCreateStruct createStruct 0x284
//      S+0x524  char          szClassName[0x100]
//      S+0x624  char          szClientName[0x200]  = 0x824
//
//    /O1 server (ebp-relative) has the same seven in the opposite order,
//    szClientName at ebp-0x824 down to the scalars at ebp-0x3c, and closes
//    on 0x824 as well.
//
//  That account is what identifies the stack object at S+0x03c as an
//  SObjData: 0x54 is the element size smgr.cpp's SObjData bank is built with
//  (`sb_Init(bank, 0x54, 0x20)` at 0x1002ce31), the constructor called on it
//  is that bank's constructor, and the field the restore reads into at its
//  +0x10 is m_NextUpdate. It is what identifies the object at S+0x0f8 as
//  objtypes.h's CObjBase (0x1a8, the size its own bank allocates). And it is
//  what turns the fourth stray zero at the top of the function -- S+0x0f4,
//  ebp-0x94, written once and never read in either build -- into
//  moveState.m_nUnknown64: WMMoveState is 0x68 bytes, so S+0x090+0x64 is
//  exactly S+0x0f4. Writes to that struct survive both optimisers because
//  its address escapes into wm_SetObjectDims.
// ======================================================================== //

#include "servermgr.h"
#include "servermsg.h"
#include "commonphysics.h"
#include "objtypes.h"
#include "ltobject_internal.h"
#include "ltengineobjects.h"

// commonmodel.cpp's w_ReadVector, `inline` there and repeated here for the
// same reason it is inline there: the /O2 build expands it at both of this
// function's call sites (0x0043298c-0x004329b3 and 0x00432eab-0x00432ed5,
// each three four-byte Reads off one base) and emits no out-of-line body,
// while /O1 calls 0x1000d040. Identical text, so the two COMDATs fold.
inline void w_ReadVectorQ430(class ILTStream* pStream, LTVector* pVec)
{
	pStream->Read(&pVec->x, 4);
	pStream->Read(&pVec->y, 4);
	pStream->Read(&pVec->z, 4);
}

// modelanim.cpp's two tracker setters, called out of line in both builds
// (server 0x10001153 / 0x10001110, lithtech 0x004011c0 / 0x00401160).
LTBOOL at_SetAnim(class LTAnimTracker* pTracker, uint32 iAnim, LTBOOL bInterpolate);
void   at_SetTime(class LTAnimTracker* pTracker, uint32 nTime, LTBOOL bInterpolate);

// 0x10001741 / lithtech 0x00402780 — counter.cpp's linear class lookup.
struct ClassDef* sub_10001741(void* pModule, char* pName);

// SObjData as this function needs it: the same 0x54-byte record
// ltobject_internal.h describes, plus the constructor the shipped image
// proves it has. 0x10010e03 zeroes +0x2c, then +0x34 and +0x38 with a pair
// of `stos` off `lea 0x34(%ecx),%edi`, and returns `this` -- which is a
// member constructor (the CMessageRef at +0x2c) followed by a two-statement
// body, in that order, with +0x30 deliberately left alone.
//
// The save half (0x00431cdc) reads m_SaveData.m_pMsg's +0x34 as the byte
// count and +0x54 as the bytes, which are ServerMessage's m_Unknown34 and
// m_pData -- so the field ltobject_internal.h carries as `uint32
// m_Unknown2c` is this reference, and "nonzero keeps the object out of the
// inactive sweep" is an object that still holds save data.
class SavObjDataQ430
{
public:
	// memset, not two pointer stores: lithtech zeroes +0x34/+0x38 (and the
	// light radius) from a SEPARATE zero register, `xor %eax,%eax` at
	// 0x00432769, where two stores share %ebx with every other zero in the
	// entry block. With /Oi the memset is the intrinsic, and it is also what
	// server.dll's `lea 0x34(%ecx),%edi ; stos ; stos` at 0x10010e03 reads
	// as. (savrest.cpp measured memset as a CALL at /O1, where /Oi is off --
	// its class is separate, so this spelling does not reach server.dll.)
	SavObjDataQ430()
	{
		memset(&m_hFiles[1], 0, 8);
	}

	LTLink      m_Links;                     // 0x00
	void*       m_hIdentifier;               // 0x0c
	float       m_NextUpdate;                // 0x10
	float       m_DeactivationTime;          // 0x14
	float       m_UpdateTimer;              // 0x18
	class Client* m_pOwningClient;           // 0x1c
	class BaseClass* m_pBaseClass;           // 0x20
	class CClassInfo* m_pClass;              // 0x24
	void*       m_Unknown28;                 // 0x28
	CMessageRef m_SaveData;                  // 0x2c
	void*       m_hFiles[3];                 // 0x30
	uint8       m_pad_3c[0x50 - 0x3c];
	uint16      m_ClientChangeFlags;         // 0x50
	uint16      m_NetFlags;                  // 0x52
};

// The model object past LTObject's own fields: the animation tracker the
// restore drives and the word that follows it. Both offsets are the save
// half's (0x00431e48 writes 0x1e8, 0x1ec and 0x1d8); 0x1c0 is where the
// tracker starts, which is what `lea 0x1c0(%esi),%edi` at 0x00432fd3 says
// and what makes the saved 0x1e8/0x1ec the tracker's own two fields.
class SavModelObjQ430
{
public:
	uint8  m_pad_000[0x1c0];
	uint8  m_Tracker[0x1d8 - 0x1c0];         // 0x1c0  an LTAnimTracker
	uint16 m_n1d8;                           // 0x1d8  (name ours)
};

// The record the restore hangs on CServerMgr::m_List8ec for an object that
// belonged to a client: an LTLink whose data is the record itself, the
// client's flag word, the object's runtime id, and the client's name inline
// from +0x12. The allocation is strlen(name) + 0x14, which is 0x12 for the
// header and two for the terminator and its alignment (names ours; nothing
// reconstructed so far reaches this list).
class SavClientRecQ430
{
public:
	LTLink m_Link;                           // 0x00
	uint32 m_nClientFlags;                   // 0x0c
	uint16 m_nObjectID;                      // 0x10
	char   m_szName[2];                      // 0x12
};

// LTObject::m_ChangeFlags bits this function and the save half read. 0x100
// is "this object is one of the thirty sky objects", which is what the index
// that follows it in the stream is for; 0x80 is what the client record above
// marks the object with (names ours).
#define SAVCHANGE_SKYOBJECT 0x100
#define SAVCHANGE_HADCLIENT 0x80

// The mask sm_SetObjectState is handed: the three state bits servermgr.h
// documents on 0x1002ade4 (8, 0x10, 0x20).
#define SAVCHANGE_STATEBITS 0x38

// 0x1000d2ef / lithtech 0x00428fd0 — dalloc.
void* dalloc(uint32 nBytes);

// ltobject_internal.h types LTObject::m_pSData as its own SObjData, whose
// +0x2c is a uint32; the save-data path needs the CMessageRef that is really
// there. The cast is a no-op -- the two views start at the same address --
// and putting the whole chain behind a macro keeps each use a fresh walk of
// m_pSData, which is what the shipped code does (one reload per statement).
#define SAV_SAVEDATA(pObj) (((SavObjDataQ430*)(pObj)->m_pSData)->m_SaveData)


// structbank.cpp's sm_AllocateObject, as a file-local inline copy: the /O2
// build EXPANDS it at 0x00432b7f-0x00432bc4 (the m_pInternal[m_nIndex]
// lookup, sb_Allocate's pop-and-grow, the two null stores and the
// ConstructFn call), while server.dll CALLS 0x10001c54 -- which is why the
// shared body in structbank.cpp stays out of line. Same text, so the
// COMDATs would fold if one were emitted.
inline LPBASECLASS sav_AllocateObjectQ430(CServerMgr* pMgr, struct ClassDef* pClassDef)
{
	CClassStats* pStats;
	LPBASECLASS  pObject;

	pStats = (CClassStats*)pClassDef->m_pInternal[pMgr->m_ClassMgr.m_nIndex];

	pObject = (LPBASECLASS)sb_Allocate(&pStats->m_Bank);

	pObject->m_hObject = LTNULL;
	pObject->m_pFirstAggregate = LTNULL;

	pClassDef->m_ConstructFn(pObject);

	return pObject;
}


// savrest.cpp's six-field WMMoveState fill, as the same inline helper (see
// there for why it is a call and not six written-out stores).
static inline void sav_InitMoveState(WMMoveState* pState, void* pContainer,
                                     void* pUnknown04, LTObject* pObj,
                                     uint32 nPriority)
{
	pState->m_pContainer = (CUnknown210*)pContainer;
	pState->m_pUnknown04 = (CUnknown6A8Obj*)pUnknown04;
	pState->m_pObj       = pObj;
	pState->m_nUnknown0C = nPriority;
	pState->m_pPushList  = 0;
	pState->m_nPushCount = 0;
}


// Read one object back out of the save and put it into the world. The record
// is the one sav_SaveObject wrote: the two offsets, the class name, the
// object's own state, and -- after this function returns -- whatever the
// object itself wrote, which sm_RestoreNextObject reads on the second pass.
// This pass stops at the end offset and leaves the stream there.
//
// An end offset of all-ones is the run's terminator and answers LT_FINISHED,
// which is what stops sm_RestoreObjects' do/while.
//
// The object is built twice over. A stack CObjBase takes the state that has
// to be read before the class can be created (type, flags, position, scale,
// rotation) so that an ObjectCreateStruct can be filled from it; the real
// object is then created through the class manager and the REST of the state
// is read straight into it. That is why the type tests before the creation
// ask the stack object and the ones after ask the created one.
//
// Three failures, and each is the error the whole restore fails with: a
// class the game does not have, a save-data block longer than the packet it
// has to fit in, and a sky index past the end of the thirty slots. The first
// and the third report LT_CANTRESTOREOBJECT through one shared block -- the
// third is the copy that stays inline and the first jumps into it, which is
// VC6's rule for two identical sites -- and both call CServerMgr::SetupError
// first; the second reports LT_INVALIDFILE and does not.
//
// The destructors are what shape the exits: both stack objects have one, so
// VC6 gives the function a single epilogue that every `return` falls into,
// running sData's inlined destructor (release the save-data reference) and
// then tempObject's. Nothing here has to be written as a goto.
LTRESULT sav_RestoreObject(CServerMgr* pMgr, ILTStream* pStream,
                           LTObject** ppObject, uint32 dwParam)
{
	ObjectCreateStruct createStruct;
	CObjBase       tempObject;
	SavObjDataQ430 sData;
	WMMoveState    moveState;
	LTObject*      pNewObject;
	struct ClassDef* pClassDef;
	LPBASECLASS    pObject;
	LTRESULT       dResult;
	uint32         nEnd;
	uint32         nStart;
	uint32         nChangeFlags;
	uint32         nAnim;
	uint32         nAnimTime;
	uint32         nClientFlags;
	uint16         nDataLen;
	uint16         nSkyIndex;
	float          fLightRadius;
	LTVector       vTemp;
	char           bHasSaveData;
	char           bHasClient;
	char           szClassName[0x100];
	char           szClientName[0x200];

	// The light radius has to survive the creation, which is the only reason
	// it is set here: the read that fills it is inside a type test and the
	// store that uses it is not.
	fLightRadius = 0.0f;
	nSkyIndex = 0;

	moveState.m_pContainer = LTNULL;
	moveState.m_pUnknown04 = LTNULL;
	moveState.m_pObj = LTNULL;
	moveState.m_nUnknown64 = 0;

	pStream->Read(&nEnd, 4);

	if (nEnd == 0xffffffff)
		return LT_FINISHED;

	pStream->Read(&nStart, 4);

	createStruct.Clear();

	pStream->ReadString(szClassName, sizeof(szClassName));

	pClassDef = sub_10001741(pMgr->m_ClassMgr.m_pUnknown00, szClassName);

	if (!pClassDef)
	{
		pMgr->SetupError(LT_CANTRESTOREOBJECT, szClassName);

		RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_CANTRESTOREOBJECT,
		                  szClassName)
	}

	pStream->Read(&tempObject.m_ObjectType, 1);
	pStream->Read(&tempObject.m_Flags, 4);
	pStream->Read(&tempObject.m_Flags2, 4);

	w_ReadVectorQ430(pStream, &vTemp);

	((LTObject*)&tempObject)->SetPos(vTemp);

	pStream->Read(&tempObject.m_Scale, 0x0c);
	pStream->Read(&tempObject.m_Rotation, 0x10);
	pStream->Read(&tempObject.m_UserFlags, 4);

	createStruct.m_ObjectType = tempObject.m_ObjectType;
	createStruct.m_Flags      = tempObject.m_Flags;
	createStruct.m_Flags2     = tempObject.m_Flags2;
	createStruct.m_Pos        = tempObject.m_Pos;
	createStruct.m_Scale      = tempObject.m_Scale;
	createStruct.m_Rotation   = tempObject.m_Rotation;

	pStream->ReadString(createStruct.m_Name, MAX_CS_FILENAME_LEN);
	pStream->ReadString(createStruct.m_Filename, MAX_CS_FILENAME_LEN);

	if (tempObject.m_ObjectType == OT_MODEL)
	{
		int i;

		for (i = 0; i < MAX_MODEL_TEXTURES; i++)
			pStream->ReadString(createStruct.m_SkinNames[i],
			                    MAX_CS_FILENAME_LEN);
	}
	else
	{
		pStream->ReadString(createStruct.m_SkinName, MAX_CS_FILENAME_LEN);
	}

	if (tempObject.m_ObjectType == OT_LIGHT)
		pStream->Read(&fLightRadius, 4);

	if (tempObject.m_ObjectType == OT_CONTAINER)
		pStream->Read(&createStruct.m_ContainerCode, 2);

	pStream->Read(&sData.m_NextUpdate, 4);

	createStruct.m_NextUpdate = sData.m_NextUpdate;

	// EXPANDED, NOT CALLED, in lithtech.exe: 0x00432b7f-0x00432bc4 carries
	// sm_AllocateObject's whole body -- hence the file-local inline copy.
	// It also moved the /O2 inline budget the way shipped spends it: with
	// it expanded, the constructor's Clear() becomes the CALL to 0x004147f0
	// that shipped has (0x00432751), which took this function from 2840 to
	// 2729 bytes and 19 to 12 mnemonic blocks.
	pObject = sav_AllocateObjectQ430(pMgr, pClassDef);

	// 3.0f = PRECREATE_SAVEGAME, NOT 1.0f. Both images say so: server.dll
	// loads 0x1004d6f0 at 0x10010925, whose four bytes are 00 00 40 40, and
	// lithtech pushes the immediate 0x40400000 at 0x00432bd3. As written
	// with 1.0f this could not match either image.
	pObject->EngineMessageFn(MID_PRECREATE, &createStruct, 3.0f);

	// The GLOBAL manager, not the parameter: 0x00432bdd loads 0x004b48c8
	// (server 0x10058bf0, servermgr.h's g_pServerMgr) and pushes that,
	// while every other manager use in this function is the parameter.
	// 3.0f again -- server 0x10010943, lithtech 0x00432be7, same constant.
	dResult = sm_CreateObjectInWorld(g_pServerMgr, pObject, pClassDef,
	                                 &createStruct, 0xffff, 3.0f, &pNewObject);

	if (dResult)
		return dResult;

	pNewObject->m_UserFlags = tempObject.m_UserFlags;

	if (pNewObject->m_ObjectType == OT_LIGHT)
		pNewObject->m_LightRadius = fLightRadius;

	pStream->Read(&pNewObject->m_BlockingPriority, 1);
	pStream->Read(&bHasSaveData, 1);

	// The object's save-data block: a fresh packet off the message bank,
	// sized to the count the file gives, and then the bytes straight into
	// it. Every one of the five statements walks m_pSData again -- the
	// shipped code reloads 0x188 and 0x2c at each of them and VC6 does not
	// CSE across statements -- so the chain is written out each time.
	if (bHasSaveData)
	{
		pStream->Read(&nDataLen, 2);

		{
			// Scoped to the alloc/release pair, as savrest.cpp measured for
			// server.dll: the shipped frame keeps pTempIO in a dead slot
			// rather than a dword of its own (0x824, not 0x828).
			CAbstractIO*   pTempIO;

			SAV_SAVEDATA(pNewObject).Init(msg_AllocPacket(&pTempIO, 0x1000, 0x1000));

			if (pTempIO)
				pTempIO->Release();
		}

		SAV_SAVEDATA(pNewObject).m_pMsg->Init(nDataLen, 0x1000);

		SAV_SAVEDATA(pNewObject).m_pMsg->m_Unknown34 = nDataLen;

		if (SAV_SAVEDATA(pNewObject).m_pMsg->m_DataSize < nDataLen)
			RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_INVALIDFILE,
			                  szClassName)

		pStream->Read(SAV_SAVEDATA(pNewObject).m_pMsg->m_pData, nDataLen);
	}

	pStream->Read(&pNewObject->m_ColorR, 1);
	pStream->Read(&pNewObject->m_ColorG, 1);
	pStream->Read(&pNewObject->m_ColorB, 1);
	pStream->Read(&pNewObject->m_ColorA, 1);

	pStream->Read(&pNewObject->m_Velocity, 0x0c);
	pStream->Read(&pNewObject->m_Acceleration, 0x0c);
	pStream->Read(&pNewObject->m_FrictionCoefficient, 4);
	pStream->Read(&pNewObject->m_Mass, 4);
	pStream->Read(&pNewObject->m_ForceIgnoreLimitSqr, 4);

	w_ReadVectorQ430(pStream, &vTemp);

	((CObjDimsSetter*)pNewObject)->SetDims(vTemp);

	pStream->Read(&nChangeFlags, 4);

	if (nChangeFlags & SAVCHANGE_SKYOBJECT)
	{
		pStream->Read(&nSkyIndex, 2);

		if (nSkyIndex >= MAX_SKYOBJECTS)
		{
			pMgr->SetupError(LT_CANTRESTOREOBJECT, szClassName);

			RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_CANTRESTOREOBJECT,
			                  szClassName)
		}
	}

	if (pNewObject->m_ObjectType == OT_MODEL)
	{
		SavModelObjQ430* pModel = (SavModelObjQ430*)pNewObject;

		pStream->Read(&nAnim, 4);
		pStream->Read(&nAnimTime, 4);

		at_SetAnim((LTAnimTracker*)pModel->m_Tracker, nAnim, LTFALSE);
		at_SetTime((LTAnimTracker*)pModel->m_Tracker, nAnimTime, LTFALSE);

		pStream->Read(&pModel->m_n1d8, 2);
	}

	// savrest.cpp's sav_InitMoveState: an inline helper, not six stores.
	sav_InitMoveState(&moveState, &pMgr->m_Unknown210, pMgr->m_pUnknown6a8,
	                  pNewObject, pNewObject->m_BlockingPriority);

	wm_SetObjectDims(&moveState, &pNewObject->m_Dims, 0, 0);

	sm_SetObjectState(pMgr, pNewObject, nChangeFlags & SAVCHANGE_STATEBITS);

	pNewObject->m_ChangeFlags = nChangeFlags;

	sm_MoveObject((IWorldSim*)pMgr, pNewObject, &createStruct.m_Pos, 0x0a);

	if (nChangeFlags & SAVCHANGE_SKYOBJECT)
	{
		pMgr->m_SkyObjects[nSkyIndex] = pNewObject->m_ObjectID;

		sm_MarkSkyChanged(g_pServerMgr);
	}

	pStream->Read(&bHasClient, 1);

	if (bHasClient)
	{
		SavClientRecQ430* pRec;

		pStream->Read(&nClientFlags, 4);
		pStream->ReadString(szClientName, sizeof(szClientName));

		pRec = (SavClientRecQ430*)dalloc(strlen(szClientName) + 0x14);

		pRec->m_nClientFlags = nClientFlags;

		strcpy(pRec->m_szName, szClientName);

		pRec->m_nObjectID = pNewObject->m_ObjectID;

		pNewObject->m_ChangeFlags |= SAVCHANGE_HADCLIENT;

		dl_AddHead(&pMgr->m_List8ec, &pRec->m_Link, pRec);
	}

	*ppObject = pNewObject;

	pStream->SeekTo(nEnd);

	return LT_OK;
}


// ======================================================================== //
//  What is settled, and what is not.
// ======================================================================== //
//
//  SETTLED BY THE BYTES, both builds agreeing:
//
//   * Every field offset, width and order in both records. The save writes
//     and the restore reads the same run in the same order, which is a free
//     cross-check on all of them, and the two type ladders (OT_WORLDMODEL /
//     OT_MODEL before the creation, OT_LIGHT / OT_CONTAINER / OT_MODEL
//     around it) line up field for field between the two halves.
//   * LTObject's 0x1a8 really is a union. The save writes it as four raw
//     bytes when the type is OT_LIGHT and as `*(char**)0x1a8 + 4` -- the
//     world's inline name -- when it is OT_WORLDMODEL, and the restore
//     stores a float back into it for OT_LIGHT. That is exactly the union
//     ltobject_internal.h carries on the strength of Get/SetLightRadius.
//   * MID_SAVEOBJECT is 8. 0x00431f3d pushes 8 where sm_RestoreNextObject
//     (already reconstructed) pushes 7 for MID_LOADOBJECT, and ltserverobj.h
//     gives MID_LOADOBJECT as 7.
//   * SObjData +0x2c is a CMessageRef, not a scalar. The save dereferences
//     it and reads +0x34 and +0x54; the restore Inits it through
//     CMessageRef::Init (0x1000ebab) and releases it in the stack object's
//     destructor at every exit. Worth pushing back into
//     ltobject_internal.h, where it is `uint32 m_Unknown2c`.
//   * SObjData is 0x54 bytes and has a constructor at 0x10010e03 whose body
//     is two null stores after the CMessageRef's own. Both facts come from
//     smgr.cpp's bank (`sb_Init(bank, 0x54, 0x20)` at 0x1002ce31) and the
//     frame account above.
//   * sav_WriteVector is `static`. lithtech.exe expands both of its call
//     sites and emits NO out-of-line body -- saveio.obj's run ends with
//     RestoreInterlinks at 0x004334a0..0x00433566 -- which only internal
//     linkage allows. saveio.cpp declares it non-static today.
//   * The two LT_CANTRESTOREOBJECT sites share one error block, with the
//     later (sky-index) one inline and the earlier (class-lookup) one
//     jumping into it: VC6 keeps the second copy inline, visible as `jmp 0x10010b8e` from
//     0x10010783.
//   * The manager sm_CreateObjectInWorld and sm_MarkSkyChanged are handed is
//     the GLOBAL g_pServerMgr (0x004b48c8 / 0x10058bf0), not the pMgr this
//     function was called with. Every other manager use is the parameter.
//
//  NOT SETTLED, in the order they are likely to cost bytes:
//
//   1. NEAR, 2729 bytes against 2730 (767 instructions against 776, ten
//      mnemonic blocks), and what is left is the /O2 INLINE BUDGET, not a
//      statement. Two sites go the other way from shipped:
//        * the second Clear()'s m_Rotation.Init() -- shipped CALLS 0x00402ea0
//          with (0,0,0,1.0f) pushed (0x0043286e); ours expands four stores.
//        * the save-data packet's ServerMessage::Init -- shipped EXPANDS
//          CMoArray<uint8,DefaultCache>::Init there (0x00432d1b..0x00432d4e:
//          SetSize2(0,&g_DefAlloc) twice, _InitArray(0x1000) = 0x00433570,
//          SetSize2(n,&g_DefAlloc)); ours calls servermsg.h's out-of-line
//          CMsgPacketData::Init. MEASURED: a file-local copy of
//          ServerMessage::Init over the real collection.h array expands
//          Init but ALSO _InitArray and SetSize2 inside it -- 2818 bytes /
//          17 blocks; spelling Term();Term(); + field stores + SetSize()
//          gave 2833 / 13. Both rejected. commonmodel.cpp records the same
//          budget defeating a dozen spellings; the remaining gap is that.
//      An out-of-class `inline` SObjData constructor measured identical to
//      the in-class one (2727, 12 blocks) before the memset change.
//
//   2. nSkyIndex's width. The shipped compare is `cmp WORD PTR [x],0x1e`
//      and the index is `movzx`, both of which say uint16; but the zero at
//      the top of the function is a DWORD store in both builds, which says
//      uint32. Written uint16 here because that is the cheaper way to be
//      wrong: uint16 costs one byte at the initialiser, uint32 costs one at
//      the compare and six at the index. The same variable on the save side
//      (0x00431e1a) is unambiguously a uint32 written two bytes wide, so
//      the two halves may genuinely differ.
//
//   3. Whether the second ObjectCreateStruct::Clear() is spelled
//      `createStruct.Clear()` or `INIT_OBJECTCREATESTRUCT(createStruct)`.
//      Identical expansion; noted only because the double clear (the
//      constructor's, then this one) reads like a mistake and is not.
//
//   4. The four-file-handle question. SObjData's 0x30/0x34/0x38 are written
//      here as `void* m_hFiles[3]` because both halves index them with a
//      strength-reduced offset starting at 0x34 and stepping 4 -- an array
//      is the only thing that compiles to that -- but ltobject_internal.h
//      names 0x30 m_pModelFile and 0x34 m_pSkinFile and nothing before this
//      named 0x38. If the original had three named members and a loop is
//      wrong, the model arm is wrong with it.
//
//   5. nStart. The restore reads it (0x004327e5) and never looks at it
//      again; sm_RestoreNextObject is what seeks to it on the second pass.
//      Declared and unused here, as in the shipped frame.
//
//   6. The names of the three model words (0x1d8, 0x1e8, 0x1ec) and the
//      container word (0x270). The restore says 0x1e8 and 0x1ec are the
//      animation tracker's own animation index and time -- they go into
//      at_SetAnim and at_SetTime -- but the tracker's field names are not
//      established here, so the save half writes them by address.
