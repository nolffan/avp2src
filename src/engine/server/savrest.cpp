// ======================================================================== //
//  server.dll /O1 — the two halves of one object's save record:
//
//      sav_SaveObject      0x1000fc39   1111 bytes   (enumeration correct)
//      sav_RestoreObject   0x100106b6   1779 bytes   (enumeration correct;
//                                                     savobj.cpp's 1885 is
//                                                     WRONG — see below)
//
//  BOTH BELONG IN src/engine/server/saveio.cpp, which declares them at lines
//  282 and 614 and defines neither.  Ground truth for every byte cited here
//  is the retail server.dll, disassembled `objdump -M intel`.  The /O2
//  twin of this source is src/engine/lithtech/savobj.cpp (lithtech.exe
//  0x00431ac0 / 0x00432740); where a decision differs between the two files
//  this one says so and says which image settled it.
//
// ------------------------------------------------------------------------ //
//  EXTENT RESOLUTION — 0x100106b6 IS 1779 BYTES, NOT 1885.
// ------------------------------------------------------------------------ //
//
//  The function enumeration says 1779; src/engine/lithtech/savobj.cpp:25 says
//  1885.  1885 - 1779 = 106, and the three enumeration rows that follow
//  0x100106b6 are 0x10010da9 (45) + 0x10010dd6 (45) + 0x10010e03 (16) = 106
//  exactly.  The 1885 figure swallows all three.
//
//  The enumeration is right and savobj.cpp is wrong, on four independent
//  grounds:
//
//   1. THE FUNCTION ENDS AT 0x10010da8.  0x10010da4..0x10010da8 is
//      `pop edi ; pop esi ; pop ebx ; leave ; ret` — a complete stdcall-free
//      cdecl epilogue that undoes exactly the `push ebp ; mov ebp,esp ;
//      sub esp,0x824 ; push ebx ; push esi ; push edi` prologue at
//      0x100106b6..0x100106c1.  The next byte, 0x10010da9, is
//      `mov ax,WORD PTR [esp+0x4]` — a fresh function reading its first
//      stack argument, and it closes with `ret 0x8` at 0x10010e00, a
//      THISCALL two-argument return that the ebp frame above could not
//      produce.
//
//   2. sav_RestoreObject CALLS 0x10010da9 AND 0x10010e03.  A function cannot
//      contain its own callees: 0x10010a26 is `call 0x10010da9`
//      (ServerMessage::Init, servermsg.h) and 0x100106de is
//      `call 0x10010e03` (SObjData's constructor — savobj.cpp:484 itself
//      names both as separate server.dll bodies while its header comment
//      absorbs them into the extent).
//
//   3. 0x10010e03 HAS A SECOND CALLER OUTSIDE THE RANGE.  savobj.cpp:489
//      records smgr.cpp's SObjData bank at 0x1002ce15 calling it.  An
//      interior block of sav_RestoreObject cannot be called from smgr.cpp.
//
//   4. NO e8/e9/0f-8x DISPLACEMENT CROSSES 0x10010da8.  Every branch inside
//      0x100106b6..0x10010da8 lands inside it (the furthest are
//      0x10010783 -> 0x10010b8e and 0x1001072c -> 0x10010d97, both the
//      shared-tail jumps described below); nothing in the range targets
//      0x10010da9+, and nothing outside the range targets an interior
//      address.  0x10010dd6 (the third absorbed row) is likewise a separate
//      thiscall body: 0x10010df7 is its `jbe` join point, reachable only
//      from 0x10010dd2 inside it.
//
//  So the row is sound as enumerated and savobj.cpp's header comment needs a
//  one-line correction.  (savobj.cpp's OTHER extent figures are fine: the
//  lithtech twins 0x00431ac0/1242 and 0x00432740/2730 both check out.)
//
// ------------------------------------------------------------------------ //
//  WHAT /O1 DOES DIFFERENTLY FROM THE /O2 TWIN  (all of it automatic —
//  the SOURCE is the same source; only /Os's inline refusals differ)
// ------------------------------------------------------------------------ //
//
//    construct                    /O2 lithtech      /O1 server.dll
//    ---------------------------  ----------------  ----------------------
//    sav_WriteVector              expanded x2       call 0x100110dc x2
//    ObjectCreateStruct::Clear    expanded x2       call 0x10001bd7 x2
//    w_ReadVector                 expanded x2       call 0x1000d040 x2
//    sm_AllocateObject            expanded          call 0x10001c54
//    CMessageRef::Init            expanded          call 0x1000ebab
//    ServerMessage::Init          expanded          call 0x10010da9
//    SObjData::SObjData           expanded          call 0x10010e03
//    3.0f as a call argument      push 0x40400000   fld [0x1004d6f0]/fstp
//
//  So NOTHING in this file may be marked `inline`, and savobj.cpp's
//  unsettled item 1 ("sm_AllocateObject has to become `inline`") is a
//  lithtech-only need that must NOT be satisfied by moving a body into a
//  header this file depends on without measuring server.dll.
//
//  Still expanded at /O1, so these DO have to be visible:
//    * ObjectCreateStruct's constructor (ltbasedefs.h, in-class; it expands
//      to the single `call Clear` at 0x100106c8)
//    * CMessageRef::~CMessageRef (servermsg.h, in-class) — 0x1001071d,
//      0x10010975, 0x10010bc5 and 0x10010d89 each carry
//      `mov ecx,[ebp-0x64] ; test ecx,ecx ; je .. ; mov eax,[ecx] ;
//       call [eax+0x8]`
//    * dl_AddHead (ltlink.h, inline) — 0x10010d5a..0x10010d76
//
// ------------------------------------------------------------------------ //
//  CORRECTION TO savobj.cpp THAT AFFECTS BOTH IMAGES
// ------------------------------------------------------------------------ //
//
//  savobj.cpp passes 1.0f to MID_PRECREATE and to sm_CreateObjectInWorld.
//  BOTH ARE 3.0f.  server.dll loads 0x1004d6f0, whose four bytes are
//  00 00 40 40 = 0x40400000 = 3.0f (0x10010925 and 0x10010943), and
//  lithtech.exe pushes the immediate 0x40400000 directly at 0x00432bd3 and
//  0x00432be7.  3 is ltserverobj.h's PRECREATE_SAVEGAME, which is what a
//  restore should be passing.  As written, savobj.cpp cannot match either
//  image on those two instructions.
//
// ------------------------------------------------------------------------ //
//  FRAME ACCOUNTS (both close exactly, which is what pins the object sizes)
// ------------------------------------------------------------------------ //
//
//  sav_SaveObject   `sub esp,0x2c` (0x1000fc3c), ebp-relative:
//      ebp-0x2c  CSaveLoadMessage msg   0x0c   (vptr/0x1004d660, +4, +8)
//      ebp-0x20  nEndFieldPos           0x04
//      ebp-0x18  nStartFieldPos         0x04
//      ebp-0x14  nAnim                  0x04
//      ebp-0x10  nSkyIndex              0x04
//      ebp-0x0c  the skin loop's counter
//      ebp-0x08  nValue                 0x04
//      ebp-0x04  spare
//    plus THREE reused dead parameter slots, which is pure /O1 allocation:
//    ebp+0x0c (pObj) holds the skin-loop byte offset and later nPos;
//    ebp+0x13 (a byte inside the pStream slot) holds bHasSaveData;
//    ebp-0x01 holds bHasClient.
//
//  sav_RestoreObject  `sub esp,0x824` (0x100106b9), ebp-relative.  Same
//  seven objects as the /O2 twin, in the OPPOSITE order, and the same total:
//      ebp-0x824  char szClientName[0x200]        0x200
//      ebp-0x624  char szClassName[0x100]         0x100
//      ebp-0x524  ObjectCreateStruct createStruct 0x284
//      ebp-0x2a0  CObjBase tempObject             0x1a8
//      ebp-0x0f8  WMMoveState moveState           0x068
//      ebp-0x090  SObjData sData                  0x054
//      ebp-0x03c  scalars                         0x03c   = 0x824
//    scalars, read off their uses:
//      ebp-0x3c nStart      ebp-0x38 vTemp[3]    ebp-0x2c nAnimTime
//      ebp-0x28 nClientFlags ebp-0x24 nAnim      ebp-0x20 nEnd
//      ebp-0x1c fLightRadius ebp-0x18 pClassDef  ebp-0x14 nSkyIndex
//      ebp-0x10 nDataLen    ebp-0x0c nChangeFlags
//      ebp-0x06 bHasSaveData ebp-0x05 bHasClient ebp-0x04 pNewObject
//    and ebp+0x0c (the pStream parameter slot) is reused twice, for the skin
//    loop's countdown and for pTempIO.
//
// ------------------------------------------------------------------------ //
//  Build /O1 (/Og /Os /Oy /Ob1).  Symbols introduced here carry a Q4S
//  suffix.  MERGE NOTE: delete the duplicated views, fold the added fields
//  into saveio.cpp's copies, and DROP THE TWO FORWARD DECLARATIONS at
//  saveio.cpp:282 and 614 — and note that saveio.cpp:614 spells the restore
//  `(SavServerMgr*, ILTStream*, SavObject**, uint32)` while the definition
//  below needs the real CServerMgr/LTObject, so the declaration has to go
//  rather than be kept alongside (it would overload, and sm_RestoreObjects
//  would bind to the wrong one).
// ======================================================================== //
#include <string.h>
#include <stdio.h>
#include "de_error.h"
#include "saveio.h"

// ======================================================================== //
//  PART A — views for sav_SaveObject.
//
//  Every one of these is a verbatim copy of saveio.cpp's own (or, where
//  saveio.cpp's stops short, an extension of it at the offsets the shipped
//  bytes reach).  DELETE ON MERGE.
// ======================================================================== //

// ltserverobj.h's ClassDef.  0x1000fc9e pushes [ecx] where ecx is the class
// record, so the name is at +0x00.
struct SavClassDefQ4S
{
	char*            m_ClassName;            // 0x00
	SavClassDefQ4S*  m_ParentClass;          // 0x04
	uint32           m_ClassFlags;           // 0x08
};

// An LTLink as ltlink.h lays it out.
struct SavLinkQ4S
{
	SavLinkQ4S* m_pPrev;                     // 0x00
	SavLinkQ4S* m_pNext;                     // 0x04
	void*       m_pData;                     // 0x08
};

// ltserverobj.h's BaseClass.  Slot 1 of its vtable is EngineMessageFn:
// 0x1001003b loads [ecx] and 0x10010044 calls [eax+0x4].
class SavBaseClassQ4S
{
public:
	virtual ~SavBaseClassQ4S();
	virtual uint32 EngineMessageFn(uint32 messageID, void* pData, float fData);
};

// ltserverobj.h's message ids.  0x10010042 pushes 8, where the already-
// reconstructed sm_RestoreNextObject pushes 7 for MID_LOADOBJECT.
#define SAV_MID_SAVEOBJECT 8

// servermgr.h's Client, at the two fields the record carries: 0x1000ffcf
// reaches +0x3c0 (four bytes) and 0x1000ffe6 pushes +0x3c4 as a string.
class SavClientQ4S
{
public:
	uint8  m_pad_000[0x3c0];
	uint32 m_ClientFlags;                    // 0x3c0  servermgr.h's name
	char*  m_pName;                          // 0x3c4  servermgr.h's name
};

// What SObjData's +0x2c really points at.  The restore half below settles
// it: +0x2c is a CMessageRef and this is its m_pMsg, a ServerMessage.
// 0x1000fe6f reads the byte count at +0x34 (servermsg.h's m_Unknown34) and
// 0x1000fe76 the bytes at +0x54 (m_pData).  ltobject_internal.h carries
// +0x2c as `uint32 m_Unknown2c`, "nonzero keeps the object out of the
// inactive sweep", which is consistent — an object holding save data is not
// idle — but the two dereferences say it is a reference to a message.
class SavMessageQ4S
{
public:
	uint8  m_pad_00[0x34];
	uint16 m_Unknown34;                      // 0x34  servermsg.h's name
	uint8  m_pad_36[0x54 - 0x36];
	uint8* m_pData;                          // 0x54  servermsg.h's name
};

// ltobject_internal.h's SObjData.  saveio.cpp's copy stops at m_pClass; the
// fields from m_hIdentifier down are the ones this function reaches.
//
// m_hFiles has to be an ARRAY: the model arm at 0x1000fd7c starts a byte
// offset at 0x34 in a dead parameter slot and steps it by 4 for two
// iterations (`add [ebp+0xc],ebx ; dec [ebp-0xc] ; jne`), indexing
// [ecx+eax*1] off m_pSData.  ltobject_internal.h names 0x30 m_pModelFile
// and 0x34 m_pSkinFile, which are [0] and [1]; nothing before this reached
// 0x38.  (Unsettled — see the closing notes.)
class SavObjDataQ4S
{
public:
	SavLinkQ4S       m_Links;                // 0x00
	void*            m_hIdentifier;          // 0x0c
	float            m_NextUpdate;           // 0x10
	uint8            m_pad_14[0x1c - 0x14];
	SavClientQ4S*    m_pOwningClient;        // 0x1c
	SavBaseClassQ4S* m_pBaseClass;           // 0x20
	SavClassDefQ4S*  m_pClass;               // 0x24
	uint8            m_pad_28[0x2c - 0x28];
	SavMessageQ4S*   m_pSaveData;            // 0x2c  (CMessageRef::m_pMsg)
	void*            m_hFiles[3];            // 0x30
};

// worldbsp.h's world, at the one field the save reads: the name, inline at
// +0x04 (0x1000fd48 is `add ecx,ebx` with ebx == 4).
class SavWorldBspQ4S
{
public:
	uint8 m_pad_00[0x04];
	char  m_Name[0xc4 - 0x04];               // 0x04
};

// ltobject_internal.h's LTObject, at the fields the record carries.
class SavObjectQ4S
{
public:
	uint8            m_pad_000[0x088];
	uint32           m_Flags;                // 0x088
	uint32           m_Flags2;               // 0x08c
	uint32           m_UserFlags;            // 0x090
	uint8            m_ColorR;               // 0x094
	uint8            m_ColorG;               // 0x095
	uint8            m_ColorB;               // 0x096
	uint8            m_ColorA;               // 0x097
	void*            m_pLinks;               // 0x098
	float            m_Rotation[4];          // 0x09c
	float            m_Scale[3];             // 0x0ac
	uint16           m_ObjectID;             // 0x0b8
	uint16           m_nSaveIndex;           // 0x0ba
	char             m_ObjectType;           // 0x0bc  OT_*
	uint8            m_BlockingPriority;     // 0x0bd
	uint8            m_pad_0be[0x0c4 - 0x0be];
	float            m_Velocity[3];          // 0x0c4
	float            m_Acceleration[3];      // 0x0d0
	float            m_FrictionCoefficient;  // 0x0dc
	float            m_Mass;                 // 0x0e0
	float            m_ForceIgnoreLimitSqr;  // 0x0e4
	uint8            m_pad_0e8[0x120 - 0x0e8];
	uint32           m_ChangeFlags;          // 0x120
	uint8            m_pad_124[0x188 - 0x124];
	SavObjDataQ4S*   m_pSData;               // 0x188
	float            m_Pos[3];               // 0x18c
	float            m_Dims[3];              // 0x198
	float            m_Radius;               // 0x1a4
	union {
		SavWorldBspQ4S* m_pWorldBsp;         // 0x1a8
		float           m_LightRadius;       // 0x1a8  (OT_LIGHT only)
	};
	uint8            m_pad_1ac[0x1d8 - 0x1ac];
	uint16           m_Model1d8;             // 0x1d8  (name ours)
	uint8            m_pad_1da[0x1e8 - 0x1da];
	uint16           m_Model1e8;             // 0x1e8  (name ours)
	uint8            m_pad_1ea[0x1ec - 0x1ea];
	uint32           m_Model1ec;             // 0x1ec  (name ours)
	uint8            m_pad_1f0[0x270 - 0x1f0];
	uint16           m_Container270;         // 0x270  (name ours)
};

// servermgr.h's CFileMgr, embedded in the manager at 0xa2c (0x1000fd5e
// `add eax,0xa2c` and the two like it).
class SavFileMgrQ4S
{
public:
	uint8 m_pad_00[0x48];
};

// servermgr.h's CServerMgr, at the three offsets this reaches.  0x804 is
// what saveio.cpp calls m_pClassMgr and servermgr.h calls m_Unknown804; the
// save hands it to the CSaveLoadMessage (0x1000fc4b) exactly as
// sm_RestoreNextObject does (0x10010e29).  m_SkyObjects is servermgr.h's
// name for the thirty ids at 0x95c (0x1000ff3c adds 0x95c and 0x1000ff4f
// compares the index against 0x1e).
class SavServerMgrQ4S
{
public:
	uint8         m_pad_000[0x804];
	void*         m_pClassMgr;               // 0x804
	uint8         m_pad_808[0x95c - 0x808];
	uint16        m_SkyObjects[30];          // 0x95c
	uint8         m_pad_998[0xa2c - 0x998];
	SavFileMgrQ4S m_FileMgr;                 // 0xa2c
};

// ltbasedefs.h's object types, as the ladders read them.  0x1000fd37 tests
// 2, 0x1000fd73 tests 1, 0x1000fdd9 tests bl (== 4) and 0x1000fdf0 tests 9;
// commonphysics.h:194 already records 2 and 9 as OT_WORLDMODEL and
// OT_CONTAINER.
#define SAV_OT_MODEL      1
#define SAV_OT_WORLDMODEL 2
#define SAV_OT_LIGHT      4
#define SAV_OT_CONTAINER  9

// 0x1000ddfb — dhash.cpp's dhash_GetKey: the key an element was filed
// under.  Called with a null length pointer at 0x1000fd1e.
char* dhash_GetKey(void* hElement, uint32* pLen);

// 0x1002bfcd — servermgr.h's fs_GetFileName.
char* fs_GetFileName(SavFileMgrQ4S* pFileMgr, void* hFile);

// 0x10056ff4 — the shared zero-initialised byte every nameless thing in the
// engine reports (0x1000fd2a loads its address as an immediate).
extern char g_szNoName[];

// 0x100110dc — saveio.cpp already defines this, at line 962.  DECLARED AND
// NOT DEFINED here on purpose: /Os declines the expansion at both call
// sites (0x1000fcdb and 0x1000ff11 are both `call`), so a second definition
// in this TU would only duplicate a COMDAT.
//
// NOTE FOR THE MERGE, not for this file: savobj.cpp:245 argues it is
// `static`, because lithtech.exe expands both sites and emits no
// out-of-line body at all.  That is a lithtech-side question — server.dll
// calls it either way — but if saveio.cpp is made `static` the definition
// has to be hoisted above sav_SaveObject or lithtech loses the expansion
// (VC6's inliner is forward-only).
void sav_WriteVector(ILTStream* pStream, float* pVec);

// saveio.cpp's CSaveLoadMessage (delete on merge).  The vtable it plants
// is 0x1004d660 (0x1000fc5d), the same constant sm_RestoreNextObject
// plants.  The constructor stores the STREAM FIRST: 0x1000fc48 loads
// pStream before 0x1000fc4b loads m_pClassMgr, and with the two
// assignments the other way round the two loads come out swapped (the
// stores themselves are scheduled the same either way).
class CSaveLoadMessageQ4S
{
public:
	CSaveLoadMessageQ4S(void* pClassMgr, ILTStream* pStream)
	{
		m_pStream = pStream;
		m_pClassMgr = pClassMgr;
	}

	virtual LTRESULT _v00();

	void*      m_pClassMgr;                  // 0x04
	ILTStream* m_pStream;                    // 0x08
};


// ======================================================================== //
//  0x1000fc39 — one object's record.  1111 bytes, /O1.
// ======================================================================== //
//
//  The record is two back-patched offsets, the class name, the object's own
//  state, and then whatever the object itself writes in answer to
//  MID_SAVEOBJECT, closed by the 0xABCDEFAF guard word.  The two offsets go
//  out as zeroes first and are filled in once their targets are known,
//  which is why the position of each is captured before it is written
//  (0x1000fc67, 0x1000fc81) and seeked back to afterwards (0x10010009,
//  0x10010072).
//
//  Stream vtable slots, from iltstream.h with its virtual destructor in
//  slot 0: +0x08 Read, +0x0c ReadString, +0x14 SeekTo, +0x18 GetPos,
//  +0x24 Write, +0x28 WriteString.  Every indirect call below checks out
//  against that table.
//
//  Order in the two `if` ladders is the bytes': the world-model arm
//  (0x1000fd37) is tested before the model arm (0x1000fd73), and each has a
//  lone `else` reached by an `eb` over it (0x1000fd50, 0x1000fdb6), so both
//  are if/else and neither is a guard.
//
//  The three types that carry extra state each get their own test against
//  the same byte: OT_LIGHT's radius shares LTObject's world pointer
//  (0x1000fde3 takes the address of 0x1a8 where 0x1000fd40 dereferenced
//  it), OT_CONTAINER has one word at 0x270, OT_MODEL has three.
//
//  Two writes are guarded by a `char` flag that goes into the stream ahead
//  of them, and BOTH RE-READ THE FLAG OUT OF MEMORY after the write
//  (0x1000fe48 `cmp BYTE PTR [ebp+0x13],0x0`, 0x1000ffbd
//  `cmp BYTE PTR [ebp-0x1],0x0`) rather than keeping the pointer they set
//  it from — which is what says the source tests the flag, not the pointer.
//
//  The skin loop at 0x1000fd8a closes with `dec [ebp-0xc] ; jne` against a
//  counter preset to 2 while a separate byte offset counts up from 0x34 by
//  4.  That is VC6's strength reduction of a counting `for` with a known
//  trip count, not a hand-written countdown -- but ONLY WHEN THE INDEX IS A
//  uint16: an `int` index gives `cmp [ebp+0xc],0x3c ; jl` with no second
//  counter.  lithtech.exe's /O2 build emits the same two induction
//  variables (0x00431c2b `mov ebp,0x34` / `mov ebx,0x2` ... `add ebp,0x4 ;
//  dec ebx ; jne`).
//
//  Each file name is fetched into pName as its own statement.  Nested as
//  WriteString's argument, VC6 loads the stream's vtable BEFORE the
//  fs_GetFileName call and has to park it across it, which cost 27 bytes
//  and the `push 4 ; pop ebx` constant register the whole function uses.
//
//  THE FRAME.  0x1001002b..0x10010038 builds the unsigned-to-float qword at
//  [ebp-0x1c], ON TOP OF nStartFieldPos at [ebp-0x18] -- which is only
//  possible if nStartFieldPos is block-scoped and the float conversion is
//  in a sibling block.  So nStartFieldPos lives in a block that runs from
//  its GetPos to its back-patch, and the MID_SAVEOBJECT call and the
//  end-of-record patch are a second block.  With nStartFieldPos at
//  function scope the conversion packs with the skin loop's counter
//  instead (temp at [ebp-0x10], every local from nStartFieldPos to
//  nSkyIndex four bytes off).  lithtech.exe agrees: its temp at
//  0x00431f32 is on nStartFieldPos as well.  Each block's nPos gets the
//  dead pMgr home [ebp+0x8] only as the innermost block's local, hence the
//  inner braces round the first back-patch.
void sav_SaveObject(SavServerMgrQ4S* pMgr, SavObjectQ4S* pObj,
                    ILTStream* pStream, uint32 dwParam)
{
	CSaveLoadMessageQ4S msg(pMgr->m_pClassMgr, pStream);
	uint32 nEndFieldPos;
	uint32 nSkyIndex;
	uint32 nAnim;
	uint32 nValue;
	char   bHasSaveData;
	char   bHasClient;
	char*  pName;

	nValue = 0;                                          // 0x1000fc42

	// The two back-patched fields, each written as a zero over which the
	// real offset goes later.
	pStream->GetPos(&nEndFieldPos);                      // 0x1000fc67
	pStream->Write(&nValue, 4);                          // 0x1000fc76

	{
		uint32 nStartFieldPos;

		pStream->GetPos(&nStartFieldPos);                // 0x1000fc81
		pStream->Write(&nValue, 4);                      // 0x1000fc8d

		pStream->WriteString(pObj->m_pSData->m_pClass->m_ClassName);   // 0x1000fca2

		pStream->Write(&pObj->m_ObjectType, 1);          // 0x1000fcb2
		pStream->Write(&pObj->m_Flags, 4);               // 0x1000fcc1
		pStream->Write(&pObj->m_Flags2, 4);              // 0x1000fcd0

		sav_WriteVector(pStream, pObj->m_Pos);           // 0x1000fcdb

		pStream->Write(pObj->m_Scale, 0x0c);             // 0x1000fcef
		pStream->Write(pObj->m_Rotation, 0x10);          // 0x1000fcff
		pStream->Write(&pObj->m_UserFlags, 4);           // 0x1000fd0e

		// 0x1000fd11-0x1000fd34: load +0x0c, test, the call arm jumps over
		// the `mov eax,0x10056ff4` arm -- a ternary, not an if/else with
		// two Writes.
		pStream->WriteString(pObj->m_pSData->m_hIdentifier
		                     ? dhash_GetKey(pObj->m_pSData->m_hIdentifier, 0)
		                     : g_szNoName);

		if (pObj->m_ObjectType == SAV_OT_WORLDMODEL)     // 0x1000fd37
		{
			pStream->WriteString(pObj->m_pWorldBsp->m_Name);   // 0x1000fd4d
		}
		else
		{
			pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[0]);
			pStream->WriteString(pName);                 // 0x1000fd70
		}

		if (pObj->m_ObjectType == SAV_OT_MODEL)          // 0x1000fd73
		{
			uint16 i;

			for (i = 1; i < 3; i++)                      // 0x1000fd7c..0x1000fdb4
			{
				pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[i]);
				pStream->WriteString(pName);
			}
		}
		else
		{
			pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[1]);
			pStream->WriteString(pName);                 // 0x1000fdd6
		}

		if (pObj->m_ObjectType == SAV_OT_LIGHT)          // 0x1000fdd9
			pStream->Write(&pObj->m_LightRadius, 4);     // 0x1000fded

		if (pObj->m_ObjectType == SAV_OT_CONTAINER)      // 0x1000fdf0
			pStream->Write(&pObj->m_Container270, 2);    // 0x1000fe06

		pStream->Write(&pObj->m_pSData->m_NextUpdate, 4);    // 0x1000fe18
		pStream->Write(&pObj->m_BlockingPriority, 1);    // 0x1000fe28

		bHasSaveData = (pObj->m_pSData->m_pSaveData != LTNULL);  // 0x1000fe37 setne

		pStream->Write(&bHasSaveData, 1);                // 0x1000fe45

		if (bHasSaveData)                                // 0x1000fe48
		{
			pStream->Write(&pObj->m_pSData->m_pSaveData->m_Unknown34, 2);   // 0x1000fe61
			pStream->Write(pObj->m_pSData->m_pSaveData->m_pData,
			               pObj->m_pSData->m_pSaveData->m_Unknown34);       // 0x1000fe79
		}

		pStream->Write(&pObj->m_ColorR, 1);              // 0x1000fe89
		pStream->Write(&pObj->m_ColorG, 1);              // 0x1000fe99
		pStream->Write(&pObj->m_ColorB, 1);              // 0x1000fea9
		pStream->Write(&pObj->m_ColorA, 1);              // 0x1000feb9

		pStream->Write(pObj->m_Velocity, 0x0c);          // 0x1000fec9
		pStream->Write(pObj->m_Acceleration, 0x0c);      // 0x1000fed9
		pStream->Write(&pObj->m_FrictionCoefficient, 4); // 0x1000fee8
		pStream->Write(&pObj->m_Mass, 4);                // 0x1000fef7
		pStream->Write(&pObj->m_ForceIgnoreLimitSqr, 4); // 0x1000ff06

		sav_WriteVector(pStream, pObj->m_Dims);          // 0x1000ff11

		pStream->Write(&pObj->m_ChangeFlags, 4);         // 0x1000ff24

		// Which of the thirty sky slots this object is in.  The walk does
		// NOT stop at the first hit -- the store at 0x1000ff49 is inside
		// the loop and the loop runs to 0x1e regardless -- so the LAST slot
		// naming the object wins, and an object in none of them goes out
		// as slot zero.
		//
		// The index is a dword that is COMPARED sixteen bits wide: it is
		// incremented as a dword (0x1000ff4c `inc edx`), stored into the
		// dword nSkyIndex with no widening (0x1000ff49), and closed by
		// `cmp dx,0x1e ; jb` (0x1000ff4f); lithtech.exe has the same
		// `inc eax ... cmp ax,0x1e` (0x00431e27).  A uint16 index instead
		// is kept in memory with a strength-reduced countdown beside it
		// (+14 bytes); a uint32 compared as a dword is `cmp edx ; jb`.
		// nSkyIndex goes out two bytes wide (0x1000ff5a pushes 2).
		if (pObj->m_ChangeFlags & 0x100)                 // 0x1000ff27 test byte
		{
			uint32 i;

			nSkyIndex = 0;                               // 0x1000ff41

			for (i = 0; (uint16)i < 30; i++)
			{
				if (pMgr->m_SkyObjects[i] == pObj->m_ObjectID)   // 0x1000ff44
					nSkyIndex = i;                           // 0x1000ff49
			}

			pStream->Write(&nSkyIndex, 2);               // 0x1000ff5f
		}

		if (pObj->m_ObjectType == SAV_OT_MODEL)          // 0x1000ff62
		{
			// 0x1000ff6b `movzx eax,WORD PTR [edi+0x1e8]` into a dword
			// local that is then written four bytes wide: the widening is
			// in the source, not in the write.
			nAnim = pObj->m_Model1e8;

			pStream->Write(&nAnim, 4);                   // 0x1000ff7e
			pStream->Write(&pObj->m_Model1ec, 4);        // 0x1000ff8d
			pStream->Write(&pObj->m_Model1d8, 2);        // 0x1000ff9d
		}

		bHasClient = (pObj->m_pSData->m_pOwningClient != LTNULL);   // 0x1000ffac

		pStream->Write(&bHasClient, 1);                  // 0x1000ffba

		if (bHasClient)                                  // 0x1000ffbd
		{
			pStream->Write(&pObj->m_pSData->m_pOwningClient->m_ClientFlags, 4);
			                                             // 0x1000ffd8
			pStream->WriteString(pObj->m_pSData->m_pOwningClient->m_pName);
			                                             // 0x1000ffee
		}

		// Back-patch the start-of-data offset and come back to it.  nPos
		// takes the DEAD pMgr parameter home (`lea 0x8(%ebp),%ecx` at
		// 0x1000fff3; pMgr's last read is 0x1000ff30), which VC6 gives
		// only to an innermost block's local -- hence these braces.
		{
			uint32 nPos;

			pStream->GetPos(&nPos);                      // 0x1000fff9
			nValue = nPos;                               // 0x10010002
			pStream->SeekTo(nStartFieldPos);             // 0x10010009
			pStream->Write(&nValue, 4);                  // 0x10010015
			pStream->SeekTo(nValue);                     // 0x1001001f
		}
	}

	{
		uint32 nPos;

		// 0x1001002b-0x10010038: the cast is `fild QWORD` off {dwParam, 0},
		// which is the unsigned-32-to-float sequence -- dwParam is unsigned.
		pObj->m_pSData->m_pBaseClass->EngineMessageFn(SAV_MID_SAVEOBJECT, &msg,
		                                              (float)dwParam);  // 0x10010044

		nPos = SAVE_SENTINEL;                            // 0x10010050

		pStream->Write(&nPos, 4);                        // 0x10010057

		// And the end-of-data offset, which is what tells the restore
		// where the next object's record starts.
		pStream->GetPos(&nPos);                          // 0x10010062
		nValue = nPos;                                   // 0x1001006b
		pStream->SeekTo(nEndFieldPos);                   // 0x10010072
		pStream->Write(&nValue, 4);                      // 0x1001007e
		pStream->SeekTo(nValue);                         // 0x10010088
	}
}


// ======================================================================== //
//  PART B — 0x100106b6, sav_RestoreObject.  1779 bytes, /O1.
//
//  From here down the REAL engine headers are needed, not Part A's views:
//  the frame holds four class objects whose constructors and destructors
//  the shipped code runs, and two of those run INLINE (ObjectCreateStruct's
//  constructor and CMessageRef's destructor), which cannot happen unless
//  the bodies are visible.
//
//  WHAT THE FRAME PROVES.  `sub esp,0x824` at 0x100106b9, and the seven
//  objects tile it exactly (account in the file header).  0x54 for sData is
//  the element size smgr.cpp's SObjData bank is built with
//  (`sb_Init(bank, 0x54, 0x20)` at 0x1002ce31); 0x1a8 for tempObject is
//  objtypes.h's CObjBase, the size its own bank allocates; 0x284 for
//  createStruct is ltbasedefs.h's ObjectCreateStruct, which this function
//  independently re-derives field by field (see the reads below); 0x68 for
//  moveState is servermgr.h's WMMoveState, and its +0x64 is the fourth
//  stray zero at 0x1001070e, written once and never read.
// ======================================================================== //

#include "servermgr.h"
#include "servermsg.h"
#include "commonphysics.h"
#include "objtypes.h"
#include "ltobject_internal.h"
#include "ltengineobjects.h"

// commonmodel.cpp's w_ReadVector.  CALLED at /O1, at both sites
// (0x100107bb and 0x10010b45); lithtech's /O2 expands both.  Declared and
// NOT defined, and NOT marked `inline`, precisely so this build emits the
// call — see the header note.
void w_ReadVector(class ILTStream* pStream, LTVector* pVec);

// modelanim.cpp's two tracker setters, called out of line in both builds
// (0x10010c0c -> 0x10001153, 0x10010c17 -> 0x10001110).
LTBOOL at_SetAnim(class LTAnimTracker* pTracker, uint32 iAnim, LTBOOL bInterpolate);
void   at_SetTime(class LTAnimTracker* pTracker, uint32 nTime, LTBOOL bInterpolate);

// 0x10001741 — counter.cpp's linear class lookup (servermodel.cpp already
// declares it with this signature and calls it with this exact argument).
struct ClassDef* sub_10001741(void* pModule, char* pName);

// 0x1000d2ef — dalloc.cpp.
void* dalloc(uint32 nBytes);

// SObjData as this function needs it: the same 0x54-byte record
// ltobject_internal.h describes, plus the constructor the shipped image
// proves it has.
//
// THE CONSTRUCTOR IS DEFINED OUT OF CLASS, AND THE COIN-FLIP THIS FILE
// RECORDED HAS NOW BEEN SETTLED BY MEASUREMENT.  0x10010e03 is a real
// sixteen-byte body — `andl $0x0,0x2c(%ecx)` (the CMessageRef member's own
// constructor), then `lea 0x34(%ecx),%edi ; xor %eax,%eax ; stos ; stos`,
// then `mov %ecx,%eax ; ret` — and server.dll CALLS it from 0x100106de,
// while lithtech.exe EXPANDS it as three plain zero stores (0x0043276b,
// 0x00432777 and 0x0043278b, at S+0x70, S+0x74 and S+0x68 = sData's +0x34,
// +0x38 and +0x2c).
//
// The in-class spelling was the known risk and it did
// bite: OUR /Os expanded the three stores inline, and doing so kept a ZERO
// REGISTER live across the whole entry block, which in turn cost the
// function the register the shipped build spends on the literal 4 and
// BLOCKED VC6's cross-jumper at the two LT_CANTRESTOREOBJECT sites (ours
// emitted `cmp %esi,%ecx` at the first where the shared block has
// `test %ecx,%ecx`, so the sixty-byte block could not be folded).  Moving
// the body out of class is the rule — /Ob1 never expands an
// out-of-class definition wherever it sits — and it took sav_RestoreObject
// from 1843 bytes / 593 instructions / 13 mnemonic blocks to
// 1785 / 579 / 5, and lithtech's /O2 twin at 0x00432740 from 2773 to 2761
// against a shipped 2730.  So BOTH images got closer, and the "one source
// cannot make both" reading in the note this replaces was wrong: /O2's
// expansion of the shipped body is not what our /Os does with ours.
//
// SETTLED — 0x10010e03 IS NOW BYTE-IDENTICAL.  The `lea 0x34(%ecx),%edi ;
// xor %eax,%eax ; stos ; stos` pair is not a memset: it is VC6 constructing
// an ARRAY of two objects whose inline constructor zeroes one pointer.
// servermessage.cpp's CUpdateContext (0x1002860b) proves the idiom — its
// CMsgCursor m_Cursors[2] emits the same `lea 0x10(%ecx),%edi ; xor ; stos ;
// stos`.  So +0x34/+0x38 are one two-element array of a class with an inline
// zeroing constructor and NO destructor (the exits release only +0x2c), and
// the SObjData constructor's own body is empty.  Measured and worse: plain
// stores (14 bytes, no %edi), `memset` without /Oi (27 bytes, a call),
// `#pragma intrinsic(memset)` + memset of the pair (16 bytes but `xor`
// before `lea`, under 8168/8447/8804/8966 alike), and a memset(this)
// constructor on an 8-byte member struct (the same `xor`-first order).  The
// array also emits an unreferenced ??_H vector-constructor COMDAT, as
// VC6 does for this construct.  lithtech.exe's /O2 sav_RestoreObject
// (0x00432740) diffs identically before and after.
class SavFileSlotQ4S
{
public:
	SavFileSlotQ4S() { m_h = 0; }
	void* m_h;
};

class SavObjDataRestQ4S
{
public:
	SavObjDataRestQ4S();                     // out of class, below

	LTLink        m_Links;                   // 0x00
	void*         m_hIdentifier;             // 0x0c
	float         m_NextUpdate;              // 0x10  read at 0x1001090e
	float         m_DeactivationTime;        // 0x14
	float         m_UpdateTimer;             // 0x18
	class Client* m_pOwningClient;           // 0x1c
	class BaseClass* m_pBaseClass;           // 0x20
	class CClassInfo* m_pClass;              // 0x24
	void*         m_Unknown28;               // 0x28
	// The destructor at every exit is `mov ecx,[ebp-0x64] ; test ecx,ecx ;
	// je ; mov eax,[ecx] ; call [eax+0x8]`, and ebp-0x64 is sData+0x2c.
	// That is CMessageRef's in-class destructor, expanded: so +0x2c is a
	// CMessageRef, not ltobject_internal.h's `uint32 m_Unknown2c`.
	CMessageRef   m_SaveData;                // 0x2c
	void*         m_hFile0;                  // 0x30
	SavFileSlotQ4S m_hFiles12[2];            // 0x34  ctor zeroes both
	uint8         m_pad_3c[0x50 - 0x3c];
	uint16        m_ClientChangeFlags;       // 0x50
	uint16        m_NetFlags;                // 0x52
};

// 0x10010e03.  Out of class ON PURPOSE: /Ob1 never expands a definition
// written here, so this is what makes the /O1 build CALL 0x10010e03 the way
// server.dll does instead of expanding the zero stores into
// sav_RestoreObject's entry block.  The body is empty: the members'
// constructors (CMessageRef at +0x2c, the slot array at +0x34) are all of it.
SavObjDataRestQ4S::SavObjDataRestQ4S()
{
}

// The model object past LTObject's own fields: the animation tracker the
// restore drives (0x10010c02 `lea esi,[edi+0x1c0]`) and the word that
// follows it (0x10010c21 `add edi,0x1d8`).
class SavModelObjQ4S
{
public:
	uint8  m_pad_000[0x1c0];
	uint8  m_Tracker[0x1d8 - 0x1c0];         // 0x1c0  an LTAnimTracker
	uint16 m_n1d8;                           // 0x1d8  (name ours)
};

// The record the restore hangs on CServerMgr::m_List8ec for an object that
// belonged to a client: an LTLink whose data is the record itself
// (0x10010d60 `mov [esi+0x8],esi`), the client's flag word at +0x0c
// (0x10010d2c), the object's runtime id at +0x10 (0x10010d4c) and the
// client's name inline from +0x12 (0x10010d36).  The allocation is
// strlen(name) + 0x14 (0x10010d1e), which is 0x12 for the header and two
// for the terminator and its alignment (names ours).
class SavClientRecQ4S
{
public:
	LTLink m_Link;                           // 0x00
	uint32 m_nClientFlags;                   // 0x0c
	uint16 m_nObjectID;                      // 0x10
	char   m_szName[2];                      // 0x12
};

// LTObject::m_ChangeFlags bits.  0x100 is "this object is one of the thirty
// sky objects" (0x10010b68 and 0x10010cb3 both `test BYTE PTR [ebp-0xb],1`,
// i.e. bit 8 of the dword at ebp-0xc); 0x80 is what the client record above
// marks the object with (0x10010d53 `or BYTE PTR [eax+0x120],0x80`).
#define SAVCHANGE_SKYOBJECT 0x100
#define SAVCHANGE_HADCLIENT 0x80

// The mask sm_SetObjectState is handed (0x10010c85 `and eax,0x38`): the
// three state bits servermgr.h documents on 0x1002ade4 (8, 0x10, 0x20).
#define SAVCHANGE_STATEBITS 0x38

// ltobject_internal.h types LTObject::m_pSData as its own SObjData, whose
// +0x2c is a uint32; the save-data path needs the CMessageRef that is
// really there.  The cast is a no-op — the two views start at the same
// address — and putting the whole chain behind a macro keeps each use a
// FRESH walk of m_pSData, which is what the shipped code does: 0x100109fc,
// 0x10010a1d, 0x10010a32, 0x10010a46 and 0x10010a8f each reload +0x188 and
// +0x2c from scratch (VC6 does not CSE across statements, so
// five reloads means five written-out chains).
#define SAV_SAVEDATA(pObj) (((SavObjDataRestQ4S*)(pObj)->m_pSData)->m_SaveData)

// The six-field WMMoveState fill, as an inline helper: a file-local copy of
// commonphysics.cpp's static wm_InitMoveState (which is what
// CWMStateInit::Init/InitFrom are made of). It has to be a call and not six
// written-out stores: the arguments are evaluated right to left before the
// body runs, so the priority (movzbl 0xbd, 0x10010c32), the object
// (0x10010c44), &pMgr->m_Unknown210 (0x10010c50) and pMgr->m_pUnknown6a8
// (0x10010c5e) are all in registers before the stores go out, and the +0x04
// store then lands ahead of the +0x10 zero (0x10010c6b / 0x10010c71). All
// 720 orders of six written-out stores had been built without reaching that.
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


// Read one object back out of the save and put it into the world.  The
// record is the one sav_SaveObject wrote; this pass stops at the end offset
// and leaves the stream there.  An end offset of all-ones is the run's
// terminator and answers LT_FINISHED (0x10010717), which is what stops
// sm_RestoreObjects' do/while.
//
// The object is built TWICE OVER.  A stack CObjBase takes the state that
// has to be read before the class can be created (type, flags, position,
// scale, rotation) so an ObjectCreateStruct can be filled from it; the real
// object is then created through the class manager and the REST of the
// state is read straight into it.  That is why the type tests before the
// creation ask ebp-0x1e4 (tempObject+0xbc) and the ones after ask
// [ebp-0x4]+0xbc (pNewObject).
//
// Three failures, and each is the error the whole restore fails with: a
// class the game does not have, a save-data block longer than the packet it
// has to fit in, and a sky index past the end of the thirty slots.  The
// first and the third report LT_CANTRESTOREOBJECT through ONE SHARED
// BLOCK — the third is the copy that stays inline (0x10010b82) and the
// first jumps into it (`jmp 0x10010b8e` from 0x10010783), which is
// the rule "of two identical sites VC6 keeps the SECOND inline".
// Both call CServerMgr::SetupError first; the second reports LT_INVALIDFILE
// (0x10010a54, `push 0x4d ; pop esi`) and does not.
//
// HOW /O1 SHAPES THE EXITS, which is the whole difference from the /O2
// twin.  sData's destructor is DUPLICATED at all four exits (0x1001071d,
// 0x10010975, 0x10010bc5, 0x10010d89) because it is inline and cheap, but
// everything after it is TAIL-MERGED onto one block:
//
//     0x10010729   push 0x2          <- LT_FINISHED
//     0x1001072b   pop esi           <- the LT_CANTRESTOREOBJECT arm
//                                       jumps HERE with 0x3a already
//                                       pushed (0x10010bd1/0x10010bd3)
//     0x1001072c   jmp 0x10010d97
//     ...
//     0x10010d95   xor esi,esi       <- LT_OK falls in
//     0x10010d97   tempObject's destructor, mov eax,esi, epilogue
//
// So `return LT_FINISHED`, `return dResult`, both RETURN_ERROR_INFO sites
// and `return LT_OK` all land on one epilogue.  The tail-merge rule runs both
// ways and this is the /O1 side of it: nothing here needs to be written as
// a goto, and nothing needs its guards nested — the ordinary early returns
// below are what produce this.
LTRESULT sav_RestoreObject(CServerMgr* pMgr, ILTStream* pStream,
                           LTObject** ppObject, uint32 dwParam)
{
	ObjectCreateStruct createStruct;         // ctor -> call 0x10001bd7 @0x100106c8
	CObjBase       tempObject;               // ctor    call 0x1002465b @0x100106d3
	SavObjDataRestQ4S  sData;                    // ctor    call 0x10010e03 @0x100106de
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

	// The light radius has to survive the creation, which is the only
	// reason it is set here: the read that fills it is inside a type test
	// (0x100108e8) and the store that uses it is not (0x100109a8).
	fLightRadius = 0.0f;                     // 0x100106ef fldz / 0x100106f4
	nSkyIndex = 0;                           // 0x100106f9

	moveState.m_pContainer = LTNULL;         // 0x100106fc
	moveState.m_pUnknown04 = LTNULL;         // 0x10010702
	moveState.m_pObj       = LTNULL;         // 0x10010708
	moveState.m_nUnknown64 = 0;              // 0x1001070e

	pStream->Read(&nEnd, 4);                 // 0x10010714

	if (nEnd == 0xffffffff)                  // 0x10010717
		return LT_FINISHED;

	pStream->Read(&nStart, 4);               // 0x1001073a

	createStruct.Clear();                    // 0x10010743

	pStream->ReadString(szClassName, sizeof(szClassName));   // 0x10010758

	pClassDef = sub_10001741(pMgr->m_ClassMgr.m_pUnknown00, szClassName);
	                                         // 0x1001076b

	if (!pClassDef)                          // 0x10010774
	{
		pMgr->SetupError(LT_CANTRESTOREOBJECT, szClassName);     // via 0x10010b8e

		RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_CANTRESTOREOBJECT,
		                  szClassName)
	}

	pStream->Read(&tempObject.m_ObjectType, 1);   // 0x10010795  (+0xbc)
	pStream->Read(&tempObject.m_Flags, 4);        // 0x100107a4  (+0x88)
	pStream->Read(&tempObject.m_Flags2, 4);       // 0x100107b3  (+0x8c)

	w_ReadVector(pStream, &vTemp);                // 0x100107bb

	((LTObject*)&tempObject)->SetPos(vTemp);      // 0x100107cf -> 0x10024a33

	pStream->Read(&tempObject.m_Scale, 0x0c);     // 0x100107e1  (+0xac)
	pStream->Read(&tempObject.m_Rotation, 0x10);  // 0x100107f1  (+0x9c)
	pStream->Read(&tempObject.m_UserFlags, 4);    // 0x10010801  (+0x90)

	// The ObjectCreateStruct fill, and it re-derives ltbasedefs.h's layout
	// on its own: 0x1001080c writes a WORD at createStruct+0x00 from a
	// `movsx ax` of the object's `char` type, 0x10010819 +0x08, 0x10010831
	// +0x0c, the `movs` triple at 0x1001083d +0x10, the one at 0x1001084c
	// +0x1c, and the four dwords at 0x1001084f..0x1001087c +0x28..+0x34.
	createStruct.m_ObjectType = tempObject.m_ObjectType;
	createStruct.m_Flags      = tempObject.m_Flags;
	createStruct.m_Flags2     = tempObject.m_Flags2;
	createStruct.m_Pos        = tempObject.m_Pos;
	createStruct.m_Scale      = tempObject.m_Scale;
	createStruct.m_Rotation   = tempObject.m_Rotation;

	// 0x7f, pushed once into edi at 0x10010867 and reused: the length is
	// MAX_CS_FILENAME_LEN (127), not the 0x80 the buffers are.
	pStream->ReadString(createStruct.m_Name, MAX_CS_FILENAME_LEN);      // 0x10010888 (+0x1fc)
	pStream->ReadString(createStruct.m_Filename, MAX_CS_FILENAME_LEN);  // 0x10010897 (+0x07c)

	if (tempObject.m_ObjectType == OT_MODEL)      // 0x1001089a
	{
		int i;

		// 0x100108a3..0x100108c2: base createStruct+0xfc, step 0x80, two
		// iterations closed by `dec [ebp+0xc] ; jne` — the same strength
		// reduction of a counting `for` the save half's skin loop gets.
		for (i = 0; i < MAX_MODEL_TEXTURES; i++)
			pStream->ReadString(createStruct.m_SkinNames[i],
			                    MAX_CS_FILENAME_LEN);
	}
	else
	{
		pStream->ReadString(createStruct.m_SkinName, MAX_CS_FILENAME_LEN);
	}                                             // 0x100108d2

	if (tempObject.m_ObjectType == OT_LIGHT)      // 0x100108d5 (cmp .. ,4)
		pStream->Read(&fLightRadius, 4);          // 0x100108e8

	if (tempObject.m_ObjectType == OT_CONTAINER)  // 0x100108eb (cmp .. ,9)
		pStream->Read(&createStruct.m_ContainerCode, 2);   // 0x10010901 (+0x02)

	pStream->Read(&sData.m_NextUpdate, 4);        // 0x1001090e

	createStruct.m_NextUpdate = sData.m_NextUpdate;    // 0x10010914 (+0x27c)

	// CALLED at /O1 (0x10010920); lithtech's /O2 carries the whole body at
	// 0x00432b7f-0x00432bc4.  Do NOT make it `inline` for this build.
	pObject = sm_AllocateObject(pMgr, pClassDef);

	// 3.0f == PRECREATE_SAVEGAME, and it is a POOLED CONSTANT at /O1:
	// `fld DWORD PTR ds:0x1004d6f0 ; fstp DWORD PTR [esp]` (0x10010925,
	// 0x10010936) over a slot sm_AllocateObject's arguments left behind.
	// The four bytes at 0x1004d6f0 are 00 00 40 40 = 0x40400000 = 3.0f.
	// lithtech.exe pushes the same value as an immediate at 0x00432bd3.
	pObject->EngineMessageFn(MID_PRECREATE, &createStruct,
	                         PRECREATE_SAVEGAME);            // 0x10010940

	// The GLOBAL manager, not the parameter: 0x10010961 pushes
	// DWORD PTR ds:0x10058bf0 (servermgr.h's g_pServerMgr), while every
	// other manager use in this function is pMgr.
	dResult = sm_CreateObjectInWorld(g_pServerMgr, pObject, pClassDef,
	                                 &createStruct, 0xffff,
	                                 PRECREATE_SAVEGAME, &pNewObject);
	                                                          // 0x10010967

	if (dResult)                                              // 0x10010971
		return dResult;

	pNewObject->m_UserFlags = tempObject.m_UserFlags;         // 0x10010993

	if (pNewObject->m_ObjectType == OT_LIGHT)                 // 0x1001099c
		pNewObject->m_LightRadius = fLightRadius;             // 0x100109a8

	pStream->Read(&pNewObject->m_BlockingPriority, 1);        // 0x100109bf (+0xbd)
	pStream->Read(&bHasSaveData, 1);                          // 0x100109cb

	// The object's save-data block: a fresh packet off the message bank,
	// sized to the count the file gives, and then the bytes straight into
	// it.  Every statement walks m_pSData again — see SAV_SAVEDATA above.
	if (bHasSaveData)                                         // 0x100109ce
	{
		pStream->Read(&nDataLen, 2);                          // 0x100109e2

		{
			// SCOPED TO THE ALLOC/RELEASE PAIR, and the width of this
			// scope is measured, not decorative.  The shipped frame puts
			// pTempIO in the DEAD pStream parameter home (`lea 0xc(%ebp),
			// %eax` at 0x100109ea, `mov 0xc(%ebp),%ecx` at 0x10010a0a) --
			// the same slot the skin countdown at 0x100108a9 already
			// borrows.  At FUNCTION scope it takes a dword of its own and
			// every slot below it moves four, which promotes two `lea`/`mov`
			// at 0x10010906 and 0x10010914 from disp8 to disp32: +6 bytes.
			// Scoped to the whole `if (bHasSaveData)` body it lands in the
			// right slot but BREAKS the tail merge of the LT_INVALIDFILE
			// exit (+20 bytes, 1799 against 1779).  Only this narrow scope
			// gives both.
			CAbstractIO*   pTempIO;

			SAV_SAVEDATA(pNewObject).Init(msg_AllocPacket(&pTempIO, 0x1000, 0x1000));
			                                     // 0x100109f0, 0x10010a05

			if (pTempIO)                                          // 0x10010a0d
				pTempIO->Release();                               // 0x10010a13
		}

		SAV_SAVEDATA(pNewObject).m_pMsg->Init(nDataLen, 0x1000);   // 0x10010a26

		SAV_SAVEDATA(pNewObject).m_pMsg->m_Unknown34 = nDataLen;   // 0x10010a3b

		// `cmp DWORD PTR [eax+0x58],ecx ; jae` past the error, i.e. the
		// error is taken when m_DataSize < nDataLen.
		// THE MEMBER IS ON THE LEFT. 0x10010a50 is `cmp [eax+0x58],ecx`, so
		// the source names m_DataSize first; written the other way round it
		// comes out `cmp ecx,[eax+0x58]` with the branch inverted.
		if (SAV_SAVEDATA(pNewObject).m_pMsg->m_DataSize < nDataLen)  // 0x10010a4f
			RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_INVALIDFILE,
			                  szClassName)

		pStream->Read(SAV_SAVEDATA(pNewObject).m_pMsg->m_pData, nDataLen);
		                                                      // 0x10010a96
	}

	pStream->Read(&pNewObject->m_ColorR, 1);                  // 0x10010aa8 (+0x94)
	pStream->Read(&pNewObject->m_ColorG, 1);                  // 0x10010aba (+0x95)
	pStream->Read(&pNewObject->m_ColorB, 1);                  // 0x10010acc (+0x96)
	pStream->Read(&pNewObject->m_ColorA, 1);                  // 0x10010ade (+0x97)

	pStream->Read(&pNewObject->m_Velocity, 0x0c);             // 0x10010af1 (+0xc4)
	pStream->Read(&pNewObject->m_Acceleration, 0x0c);         // 0x10010b04 (+0xd0)
	pStream->Read(&pNewObject->m_FrictionCoefficient, 4);     // 0x10010b17 (+0xdc)
	pStream->Read(&pNewObject->m_Mass, 4);                    // 0x10010b2a (+0xe0)
	pStream->Read(&pNewObject->m_ForceIgnoreLimitSqr, 4);     // 0x10010b3d (+0xe4)

	w_ReadVector(pStream, &vTemp);                            // 0x10010b45

	((CObjDimsSetter*)pNewObject)->SetDims(vTemp);            // 0x10010b56 -> 0x10024ae3

	pStream->Read(&nChangeFlags, 4);                          // 0x10010b65

	if (nChangeFlags & SAVCHANGE_SKYOBJECT)                   // 0x10010b68
	{
		pStream->Read(&nSkyIndex, 2);                         // 0x10010b78

		if (nSkyIndex >= MAX_SKYOBJECTS)                      // 0x10010b7b
		{
			pMgr->SetupError(LT_CANTRESTOREOBJECT, szClassName);  // 0x10010b8e

			RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_CANTRESTOREOBJECT,
			                  szClassName)
		}
	}

	if (pNewObject->m_ObjectType == OT_MODEL)                 // 0x10010bdb
	{
		SavModelObjQ4S* pModel = (SavModelObjQ4S*)pNewObject;

		pStream->Read(&nAnim, 4);                             // 0x10010bf0
		pStream->Read(&nAnimTime, 4);                         // 0x10010bfd

		at_SetAnim((LTAnimTracker*)pModel->m_Tracker, nAnim, LTFALSE);
		                                                      // 0x10010c0c
		at_SetTime((LTAnimTracker*)pModel->m_Tracker, nAnimTime, LTFALSE);
		                                                      // 0x10010c17

		pStream->Read(&pModel->m_n1d8, 2);                    // 0x10010c2c
	}

	// The six-field fill (0x10010c32..0x10010c77), interleaved by the
	// scheduler with wm_SetObjectDims' argument pushes; see
	// sav_InitMoveState above for why it is an inline call.
	sav_InitMoveState(&moveState, &pMgr->m_Unknown210, pMgr->m_pUnknown6a8,
	                  pNewObject, pNewObject->m_BlockingPriority);

	wm_SetObjectDims(&moveState, &pNewObject->m_Dims, 0, 0);  // 0x10010c7d

	sm_SetObjectState(pMgr, pNewObject, nChangeFlags & SAVCHANGE_STATEBITS);
	                                                          // 0x10010c8d

	pNewObject->m_ChangeFlags = nChangeFlags;                 // 0x10010c9a

	sm_MoveObject((IWorldSim*)pMgr, pNewObject, &createStruct.m_Pos, 0x0a);
	                                                          // 0x10010cab

	if (nChangeFlags & SAVCHANGE_SKYOBJECT)                   // 0x10010cb3
	{
		pMgr->m_SkyObjects[nSkyIndex] = pNewObject->m_ObjectID;   // 0x10010cc7

		sm_MarkSkyChanged(g_pServerMgr);                      // 0x10010cd5
	}

	pStream->Read(&bHasClient, 1);                            // 0x10010ce5

	if (bHasClient)                                           // 0x10010ce8
	{
		SavClientRecQ4S* pRec;

		pStream->Read(&nClientFlags, 4);                      // 0x10010cfc
		pStream->ReadString(szClientName, sizeof(szClientName));  // 0x10010d0f

		pRec = (SavClientRecQ4S*)dalloc(strlen(szClientName) + 0x14);
		                                     // 0x10010d19 strlen, 0x10010d22 dalloc

		pRec->m_nClientFlags = nClientFlags;                  // 0x10010d2c

		strcpy(pRec->m_szName, szClientName);                 // 0x10010d3a

		pRec->m_nObjectID = pNewObject->m_ObjectID;           // 0x10010d4c

		pNewObject->m_ChangeFlags |= SAVCHANGE_HADCLIENT;     // 0x10010d53

		// EXPANDED at /O1 (ltlink.h has it inline): 0x10010d5a..0x10010d76
		// is dl_AddAfter's body with `&pList->m_Head` folded to
		// `lea ecx,[eax+0x4]`, closing on `inc DWORD PTR [eax]`.
		dl_AddHead(&pMgr->m_List8ec, &pRec->m_Link, pRec);
	}

	*ppObject = pNewObject;                                   // 0x10010d80

	pStream->SeekTo(nEnd);                                    // 0x10010d86

	return LT_OK;                                             // 0x10010d95
}


// ======================================================================== //
//  WHAT IS SETTLED, AND WHAT IS NOT.
// ======================================================================== //
//
//  SETTLED BY the retail server.dll'S BYTES (and cross-checked against
//  the retail lithtech.exe where the note says so):
//
//   * THE EXTENT.  0x100106b6 is 1779 bytes, ending at 0x10010da8.  Four
//     independent grounds in the file header; savobj.cpp:25's 1885 is the
//     whole COMDAT RUN (1779 + 90 + 16), not the function.
//
//   * A SECOND EXTENT DEFECT, FOUND ON THE WAY.  The function enumeration
//     carries `sub_10010da9,45` and `sub_10010dd6,45` as two rows.  THEY
//     ARE ONE NINETY-BYTE FUNCTION, 0x10010da9..0x10010e02, and it is
//     ServerMessage::Init — already reconstructed, inline, in
//     src/engine/server/servermsg.h, which documents it as "90 bytes ...
//     NEAR (90 bytes against 90, twenty-eight instructions against
//     twenty-eight)".  0x10010dd6 is NOT an instruction boundary: the
//     bytes there are the `79 50` inside `lea edi,[ecx+0x50]` at
//     0x10010dd5, and its only apparent caller (0x1004c04e) is likewise
//     mid-instruction, inside CRT code.  This is the
//     false-call-target family.  Both rows should be replaced by one
//     `0x10010da9, 90`.
//
//   * Every field offset, width and order in both records.  The save
//     writes and the restore reads the same run in the same order, which
//     is a free cross-check on all of them, and the two type ladders line
//     up field for field between the halves.
//
//   * ltbasedefs.h's ObjectCreateStruct layout, re-derived here offset for
//     offset: m_ObjectType 0x00 (a WORD, written from a `movsx` of the
//     object's `char`), m_ContainerCode 0x02, m_Flags 0x08, m_Flags2 0x0c,
//     m_Pos 0x10, m_Scale 0x1c, m_Rotation 0x28, m_Filename 0x7c,
//     m_SkinNames 0xfc stepping 0x80, m_Name 0x1fc, m_NextUpdate 0x27c,
//     total 0x284.  MAX_CS_FILENAME_LEN is 127 (the 0x7f in edi),
//     MAX_OCS_CLASSNAME_LEN 64 (0x7c - 0x3c) and MAX_MODEL_TEXTURES 2 (the
//     loop's trip count).  All three agree with the SDK header.
//
//   * 3.0f, twice — PRECREATE_SAVEGAME.  See the file header; savobj.cpp
//     has 1.0f and cannot match either image there.
//
//   * LTObject's 0x1a8 really is a union: the save writes it as four raw
//     bytes when the type is OT_LIGHT (0x1000fded) and as `*(char**)0x1a8
//     + 4` when it is OT_WORLDMODEL (0x1000fd48), and the restore stores a
//     float back into it for OT_LIGHT (0x100109a8).
//
//   * MID_SAVEOBJECT is 8 (0x10010042), MID_PRECREATE 0 (0x1001093c), and
//     ltserverobj.h agrees with both.
//
//   * SObjData +0x2c is a CMessageRef, not a scalar.  The save
//     dereferences it and reads +0x34 and +0x54; the restore Inits it
//     through CMessageRef::Init (0x10010a05) and releases it in the stack
//     object's destructor at all four exits.  Worth pushing back into
//     ltobject_internal.h, where it is `uint32 m_Unknown2c`.
//
//   * The manager sm_CreateObjectInWorld and sm_MarkSkyChanged are handed
//     is the GLOBAL g_pServerMgr (0x10058bf0, pushed at 0x10010961 and
//     0x10010ccf), not the pMgr this function was called with.  Every
//     other manager use is the parameter.
//
//   * The two LT_CANTRESTOREOBJECT sites share one block, with the later
//     (sky-index) copy inline and the earlier (class-lookup) one jumping
//     into it at 0x10010783.  And the shared block's OWN tail merges with
//     the LT_FINISHED return: `push 0x3a ; jmp 0x1001072b` lands on the
//     `pop esi` of `push 0x2 ; pop esi`.
//
//  NOT SETTLED, in the order they are likely to cost bytes:
//
//   1. (Settled since: the SObjData constructor is out of class with an
//      empty body and a two-element slot array at +0x34; 0x10010e03 is
//      byte-identical.  See the note at the class.)
//
//   2. nSkyIndex's width, and IT DIFFERS BETWEEN THE TWO HALVES.
//      Restore: `cmp WORD PTR [ebp-0x14],0x1e` (0x10010b7b) and
//      `movzx eax,WORD PTR [ebp-0x14]` (0x10010cb9) both say uint16, but
//      the zero at 0x100106f9 is `mov DWORD PTR [ebp-0x14],esi`, a dword
//      store.  Written uint16 because that is the cheaper way to be wrong
//      (uint16 costs one byte at the initialiser — /Os prefers the
//      three-byte dword store to the four-byte `mov ..,si` when the slot
//      is a whole dword — while uint32 costs one byte at the compare and
//      six at the index).  Save: unambiguously uint32, since 0x1000ff41
//      and 0x1000ff49 are both dword stores AND the compare is against the
//      loop counter (`cmp dx,0x1e`), not against the variable.  Written
//      uint32 there.  The two halves may genuinely differ; if a build says
//      otherwise, the restore's is the one to flip.
//
//   3. Whether the second ObjectCreateStruct::Clear() is spelled
//      `createStruct.Clear()` or `INIT_OBJECTCREATESTRUCT(createStruct)`.
//      Identical expansion; noted only because the double clear (the
//      constructor's at 0x100106c8, then this one at 0x10010743) reads
//      like a mistake and is not.
//
//   4. The four-file-handle question.  SObjData's 0x30/0x34/0x38 are
//      written as `void* m_hFiles[3]` because both halves index them with
//      a strength-reduced offset starting at 0x34 and stepping 4 — an
//      array is the only thing that compiles to that — but
//      ltobject_internal.h names 0x30 m_pModelFile and 0x34 m_pSkinFile
//      and nothing before this named 0x38.  If the original had three
//      named members and a loop is wrong, the save's model arm is wrong
//      with it.
//
//   5. nStart.  The restore reads it (0x1001073a) and never looks at it
//      again; sm_RestoreNextObject is what seeks to it on the second pass.
//      Declared and unused here, as in the shipped frame.  Likewise
//      dwParam, which sav_RestoreObject never reads — ebp+0x14 appears
//      nowhere in the body — but which saveio.cpp:615's declaration and
//      the caller's push both carry.
//
//   6. SETTLED: the six moveState stores are an inline helper call
//      (sav_InitMoveState), not six statements.  The helper's argument
//      evaluation is what puts all four values in registers before the
//      first store; no order of written-out stores reproduces 0x10010c6b.
//
//   7. The names of the three model words (0x1d8, 0x1e8, 0x1ec) and the
//      container word (0x270).  The restore says 0x1e8 and 0x1ec are the
//      animation tracker's animation index and time — they go into
//      at_SetAnim and at_SetTime — but the tracker's field names are not
//      established here, so the save half writes them by address.
//
//  MERGE / OTHER-FILE ITEMS:
//
//   * saveio.cpp:962 defines sav_WriteVector non-static.  savobj.cpp:245
//     argues it is `static` on lithtech evidence (both sites expanded, no
//     out-of-line body emitted).  server.dll calls it either way, so this
//     file is indifferent; but if it is made `static`, the definition has
//     to move ABOVE sav_SaveObject, because VC6's inliner is forward-only
//     and lithtech would otherwise lose both expansions.
//
//   * savobj.cpp's unsettled item 1 wants sm_AllocateObject `inline` in
//     servermgr.h for lithtech's sake.  server.dll CALLS it (0x10010920).
//     Every `inline` is a bet on both images, and the census
//     is a hypothesis, not a proof.  Measure server.dll before keeping it.
//
//   * The CMessageRef at SObjData+0x2c belongs in ltobject_internal.h in
//     place of `uint32 m_Unknown2c`, and 0x10010da9/0x10010dd6 are
//     one 90-byte function.
// ======================================================================== //
