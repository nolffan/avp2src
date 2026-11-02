// *********************************************************************** //
//
//  MODULE:   Q474000 (.CPP)   (file name ours)
//
//  PURPOSE:  lithtech.exe 0x00476e80 .. 0x00477808 -- the CLIENT model and
//            sprite loader, i.e. the `cm_*` half of clientmgr.cpp that the
//            file manager hands its records to.
//
//  WHICH MODULE THIS IS, AND WHERE IT STARTS
//  -----------------------------------------
//  The cluster this file was cut from (0x00474000..0x00478000) is NOT one
//  module.  Reading the .data literal pool in ADDRESS ORDER splits it in
//  exactly one place:
//
//      0x004ae0f8 "Game time: %.2f"                   0x00474514
//      0x004ae114 "CServerMgr::LoadWorld"             0x0047471e
//      0x004ae12c "sm_CreateServerData"               0x00474957
//      0x004ae144 "sm_AddObjectToWorld"               0x00474af1
//      0x004ae15c "sm_RemoveObjectFromWorld"          0x00474dd1
//      0x004ae1a8 "CServerMgr::DoStartWorld"          0x00475267
//      0x004ae1f0 "CServerMgr::DoRunWorld"            0x004753e0
//      0x004ae214 "Debug: Server caching out model %s" 0x00475626
//      0x004ae238 "CServerMgr::GetSoundDuration"      0x00475cc1
//      0x004ae270 "CServerMgr::KillSound"             0x00475d8b
//      0x004ae2c0 "sm_CreateNewID"                    0x0047618a
//      0x004ae2d0 "sm_CheckObjectIntegrity"           0x00476413
//      0x004ae308 "ILTServer::SetPortalFlags"         0x004767e2
//      0x004ae324 "ILTServer::CacheFile"              0x00476afe   <-- last
//      ---------------------------------------------------------------
//      0x004ae398 "LT_MISSINGSPRITEFILE"              0x00476ff4   <-- first
//      0x004ae3b0 "LoadSprite"                        0x00476f7c
//      0x004ae3d4 "Loading sprite: %s\n"              0x00476f33
//      0x004ae3e8 "(Client) loading model: %s"        0x00477038
//      0x004ae404 "LoadModel2"                        0x0047715d
//      0x004ae410 "cm_LoadChildModel"                 0x00477255
//      0x004ae424 "cm_LoadModelData"                  0x004774cd
//      0x004ae438 "FadeSpriteTex"                     0x00477747
//
//  Everything above the rule is CServerMgr.  Everything below it names a
//  CLIENT entry point -- "(Client) loading model", and `cm_` where the
//  server's own copy of the same code says `se_`.
//
//  THE BOUNDARY IS EXACT, and it is not read off the strings.  Matching
//  src/engine/lithtech/Release/smgr.obj against
//  the retail lithtech.exe places smgr.obj's LAST function in the region
//  at 0x00476e40 (?SetGlobalLightObject@CServerMgr@@QAEXPAVLTObject@@@Z, 59
//  bytes, ending 0x00476e7b).  The next function, 0x00476e80, is the sprite
//  loader below.  So:
//
//      0x00474000 .. 0x00476e7b   smgr.obj  -- ALREADY RECONSTRUCTED in
//                                 src/engine/server/smgr.cpp, which the
//                                 lithtech build already compiles.
//      0x00476e80 .. 0x00477808   THIS FILE.
//
//  WHY THESE ARE `cm_` AND NOT `se_`
//  ---------------------------------
//  The server's counterparts are already written, in
//  src/engine/server/serverengine.cpp: se_InitModel, se_LoadModelData and
//  se_LoadChildModel raise the SAME three console strings ("Child model
//  trees invalid in model %s", "Missing one or more child models in model
//  %s", "Error %d loading model %s").  They are not the same functions: the
//  shipped RETURN_ERROR symbol here is the literal "cm_LoadModelData" /
//  "cm_LoadChildModel", the manager pointer is a CClientMgr (m_pFileMgr at
//  +0x1534, the lists at +0x10f4/+0x1100/+0x1104, the pending-load queue at
//  +0x15a8/+0x15ac -- all offsets src/engine/lithtech/clientmgr.h and
//  clientmgr2.cpp already carry), and the path splitter is rezmgr's
//  0x00490e50 rather than the CRT `_splitpath` at 0x00495313 the server
//  uses.  So this is client-only source, not a second copy of
//  serverengine.cpp.
//
//  EVERY symbol defined here carries a Q474 suffix so this TU cannot
//  collide with src/engine/lithtech/clientmgr*.cpp at link time.
//
//  NOT WRITTEN HERE, AND WHY
//  -------------------------
//    0x00477810  already reconstructed -- src/engine/lithtech/ltnew1.cpp
//                (?sub_477810@@YAIPAXPAVCKeyedListOwnerView@@@Z, a byte
//                MATCH at 0x477810).
//    0x0047773b  NOT A FUNCTION.  The 75-byte row
//                at 0x004776f0 has a branch LEAVING the row --
//                0x0047771d -> 0x0047773b -- so 0x0047773b is the taken arm
//                of 0x004776f0's entry test and the real extent is
//                0x004776f0..0x00477808, 281 bytes.  It is written here as
//                ONE function.
//    0x00476cf2  NOT A FUNCTION.  Our smgr.obj matches
//                ?sm_BroadcastCacheFile@@YAXPAVCServerMgr@@KG@Z at
//                0x00476cc0 with 258 bytes, i.e. 0x00476cc0..0x00476dc1, so
//                0x00476cf2 is interior to an already-written smgr.obj
//                function.
//
// *********************************************************************** //

#include <stdio.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltlink.h"


// ======================================================================= //
//  WHAT THIS TU BORROWS
//
//  Stand-in declarations only.  Every offset below is the displacement the
//  shipped code actually uses, and the address it was read from is in the
//  comment.  Nothing here is guessed from a header.
// ======================================================================= //

// ---- the error-report machinery ---------------------------------------- //
//
// Identical in shape to src/engine/server/de_error.h, which documents why
// the third argument is the numeric code stringized through one extra level
// of expansion: 0x0047715d pushes 0x004a72b4, and
// that dword holds the four bytes "22" -- LT_MISSINGFILE's VALUE, not its name.
// 0x004a7164 is the lone-space description.
//
// The other form pushes the literal NAME (0x004ad3a4 -> the string
// "LT_MISSINGMODELFILE", read at 0x00477250) and a real description; that is
// de_error.h's RETURN_ERROR_SYM_DESC.

extern "C" void dsi_OnReturnError(int code);          // 0x0042d760
void  con_ConsolePrint(char* pFormat, ...);           // 0x0042e6f0
extern int   g_DebugLevel;                            // 0x004b0ab0
extern char* g_pErrorFormat;                          // 0x004a7110 -> 0x004a7114
                                                      //   "LT ERROR: %s returned %s (%s)"

#define LT_ERRSTR_(x) #x
#define LT_ERRSTR(x)  LT_ERRSTR_(x)
#define LT_ERRDESC    " "

#define REPORT_ERROR_Q474(level, fnName, code) \
	dsi_OnReturnError(code); \
	if (g_DebugLevel >= level) \
		con_ConsolePrint(g_pErrorFormat, fnName, LT_ERRSTR(code), LT_ERRDESC);

#define RETURN_ERROR_Q474(level, fnName, code) \
	{ \
		REPORT_ERROR_Q474(level, fnName, code) \
		return code; \
	}

// The form whose third argument is the code's NAME and whose fourth is a
// real description (0x00477250, 0x0047733d, 0x00477539).
#define RETURN_ERROR_SYM_DESC_Q474(level, fnName, code, desc) \
	{ \
		dsi_OnReturnError(code); \
		if (g_DebugLevel >= level) \
			con_ConsolePrint(g_pErrorFormat, fnName, #code, desc); \
		return code; \
	}


// ---- what the file manager hands back ---------------------------------- //
//
// src/engine/lithtech/clientmgr.h already carries this class verbatim (it
// calls it CClientFileRef): three fields, 0xc bytes, with an inline
// constructor that zeroes all three.  0x004771a7/0x004771ab/0x004771af are
// that constructor inlined, and 0x00477208/0x00477210 then fill in the two
// it needs.  Re-declared here rather than #included so this TU stands alone.
class CFileIdentQ474
{
public:
	CFileIdentQ474() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                       // 0x00
	char*  m_pFilename;                       // 0x04
	uint16 m_nLocalID;                        // 0x08
};

// The record 0x00404680 answers with.  Only the two fields this file reaches
// are modelled: +0x00 is the loaded thing (tested at 0x00477105, 0x00477273,
// 0x00477710 and written at 0x004777ab) and +0x1c is the interned path
// (0x00477034, 0x004770a4) -- the same +0x1c 0x004045d0 hands out as the
// file's name.  +0x19 is a flag byte whose bit 0 is tested at 0x004777ca and
// cleared at 0x004777f8.
class CFileRecQ474
{
public:
	void*  m_pData;                           // 0x00
	uint8  m_pad_04[0x19 - 0x04];
	uint8  m_Flags19;                         // 0x19  bit 0
	uint8  m_pad_1a[0x1c - 0x1a];
	char*  m_pName;                           // 0x1c
};

class CFileMgrQ474;
class ILTStreamQ474;

// 0x004045d0 -- the file's path, or the empty buffer at 0x004b0ab4 when it
// has none.
char*         cm_GetFileNameQ474(CFileMgrQ474* pFileMgr, CFileIdentQ474* pIdent);
// 0x00404680 -- find (or make) the record for one named file.  The third
// argument is a byte compared against the record's own +0x18 at 0x004046ec,
// with 0xff on either side meaning "anything"; every model call site here
// passes 1 and the sprite one passes 2.
CFileRecQ474* cm_FindFileQ474(CFileMgrQ474* pFileMgr, CFileIdentQ474* pIdent,
                              uint8 nType);
// 0x00404890 -- a stream over one already-found record.
ILTStreamQ474* cm_OpenStreamQ474(CFileMgrQ474* pFileMgr, CFileRecQ474* pRec);
// 0x004048c0 -- release the file the record names.
void          cm_ReleaseFileQ474(CFileMgrQ474* pFileMgr, CFileIdentQ474* pIdent);

// 0x00490e50, rezmgr's own path splitter -- NOT the CRT `_splitpath`, which
// is at 0x00495313 and is what server.dll's se_LoadChildModel calls.  Read
// off the bytes: it hands 0x00490d10 the path and two scratch buffers, and
// 0x00490d10 cuts at the LAST '\' (0x5c) or '/' (0x2f), putting everything
// before the separator in its second argument and everything after it in its
// third; 0x00490e50 then copies those to its own second and third arguments
// and splits the third again at the last '.' (0x2e) into its fourth and
// fifth.  So argument 2 is the DIRECTORY -- which is why passing it alone
// and prefixing it with a backslash below is right, where the server passes
// its buffer in _splitpath's THIRD slot for the same effect.  Each output is
// skipped when null (0x00490e93, 0x00490ec2, 0x00490ef0, 0x00490f1f).
//
// It is stdlith's CHelpers::ExtractNames (helpers.h), which StdLith.lib
// defines and which sits at 0x00490e50; declared as that class's static
// member so the call binds to it.
class CHelpers
{
public:
	static int  ExtractNames(const char* pFullPath, char* pPathname,
	                         char* pFilename, char* pFiletitle,
	                         char* pExt);                  // 0x00490e50
};


// ---- the model ---------------------------------------------------------- //
//
// commonmodel.h (src/engine/server) already carries the real Model; only
// what this file calls is restated, with the shipped addresses.  The
// allocator pair is the same global pushed twice (0x004b4e88), exactly as
// serverengine.cpp's calls do it.
extern uint8 g_MoAllocBankQ474[];                              // 0x004b4e88

class ModelQ474
{
public:
	// Slot 0 is the scalar deleting destructor: every teardown here is
	// `mov (%ecx),%eax; push $1; call *(%eax)` (0x00477318, 0x004773ba,
	// 0x004773ee, 0x0047750f, 0x004775cc, 0x0047763f, 0x004776c8,
	// 0x0047772e) which is what VC6 emits for `delete pModel`.
	virtual ~ModelQ474();

	ModelQ474(void* pAlloc1, void* pAlloc2);                   // 0x00446360
	LTRESULT Load(ILTStreamQ474* pStream, void* pAlloc);       // 0x0044e550
	LTRESULT LoadChildModels(class CModelLoadRequestQ474* pReq);// 0x0044ddc0
	LTBOOL   SetFilename(char* pFilename);                     // 0x004476c0

	// +0x08 is the model's own LTLink -- commonmodel.h calls the run from
	// +0x08 a cleanup area with a list head at +0x14, but the six stores at
	// 0x0047779c..0x004777aa are CheapLTLink::AddAfter over a link BASED AT
	// +0x08, and +0x10 (which commonmodel.h already names "the model's own
	// address, stored by the constructor") is that link's m_pData.  So
	// +0x08..+0x13 is one LTLink and +0x14 is the keyed-list head.
	uint8  m_pad_04[0x08 - 0x04];
	LTLink m_Link;                                             // 0x08
	LTLink m_CleanupHead;                                      // 0x14
	uint8  m_pad_20[0x80 - 0x20];
	// The model's property text, handed to the command parser at
	// 0x0047774c.
	void*  m_pProps;                                           // 0x80
	uint8  m_pad_84[0xc0 - 0x84];
	// Written at 0x00477787 from the "FadeSpriteTex" property.
	void*  m_pFadeSpriteTex;                                   // 0xc0
	uint8  m_pad_c4[0x16c - 0xc4];
	// The node count every tracker's two indices are range-checked against
	// (0x00477913, 0x0047797a).
	uint32 m_nNodes;                                           // 0x16c
	uint8  m_pad_170[0x208 - 0x170];
	// Bumped at 0x004779db and dropped at 0x004778c6 / 0x00477b6e.
	uint32 m_nCacheCount;                                      // 0x208
	// sizeof(Model) is 0x2c4, read straight off the allocation size this
	// class is built with: `push $0x2c4` at 0x0047744b feeds operator new at
	// 0x00477480.  Nothing in this run reaches past m_nCacheCount, so the
	// remaining 0xb8 bytes are unnamed rather than guessed.
	uint8  m_pad_20c[0x2c4 - 0x20c];
};

// 0x0043c500 / 0x0043c580 -- make a link round a pointer, and put it on a
// list.  Named from what they do at 0x004776a3/0x004776af and 0x004777ad/
// 0x004777b7; 0x004a8380 is the bank both links come out of.
extern uint8 g_LinkBankQ474[];                                 // 0x004a8380
LTLink* dl_MakeLinkQ474(void* pBank, void* pData);             // 0x0043c500
void    dl_AddLinkQ474(LTLink* pHead, LTLink* pLink);          // 0x0043c580

// What the model reader is handed.  Layout read straight off the inlined
// constructor at 0x00477450..0x0047747f and the four uses below:
//   +0x00 the stream            (written 0x00477458, 0x004774ff, read
//                                0x0047759b; and 0x0047730e / 0x00477384 in
//                                the child loader, through the pointer)
//   +0x04 the child-model hook  (0x00477460 sets the do-nothing default
//                                0x004451b0 -- `mov $0x46,%eax; ret`, i.e.
//                                LT_NOCHANGE -- and 0x00477582 replaces it
//                                with cm_LoadChildModel at 0x00477180)
//   +0x08 the context           (0x00477468, 0x0047758a, read 0x004771a3)
//   +0x0c the child's name      (0x00477450, read 0x004771ce/0x004771e6)
//   +0x10..+0x1c four flags, all set to 1 (0x00477470..0x0047747c); +0x14
//                is read at 0x004775bc and +0x1c at 0x0047762d.
// Identical to src/engine/server/serverengine.h's CModelLoadRequest.
class CModelLoadContextQ474;

// 0x004451b0 -- `mov $0x46,%eax; ret`: LT_NOCHANGE and nothing else, the
// client's copy of serverengine.h's se_NoChildModel.  Declared, never
// defined; only its address is ever taken.
LTRESULT cm_NoChildModelQ474(class CModelLoadRequestQ474* pReq,
                             ModelQ474** ppModel);

class CModelLoadRequestQ474
{
public:
	// DEFINED IN-CLASS ON PURPOSE.  The shipped constructor is INLINED at
	// 0x00477450..0x0047747f, so it has to be implicitly inline here; an
	// out-of-line definition would let VC6 emit a call instead.
	CModelLoadRequestQ474()
	{
		m_pChildName     = LTNULL;
		m_pStream        = LTNULL;
		// ORDER MATTERS, and it is NOT the server's.  0x00477460 stores
		// +0x04 (the callback) and only then 0x00477468 stores +0x08 (the
		// context); serverengine.cpp's out-of-line copy has them the other
		// way round.  Written in the CLIENT's order, read off these bytes.
		m_pfnLoadChild   = cm_NoChildModelQ474;
		m_pContext       = LTNULL;
		m_bUnknown10     = LTTRUE;
		m_bTreesValid    = LTTRUE;
		m_bUnknown18     = LTTRUE;
		m_bChildrenFound = LTTRUE;
	}

	ILTStreamQ474* m_pStream;                                  // 0x00
	LTRESULT (*m_pfnLoadChild)(CModelLoadRequestQ474* pReq,
	                           ModelQ474** ppModel);           // 0x04
	CModelLoadContextQ474* m_pContext;                         // 0x08
	char*  m_pChildName;                                       // 0x0c
	LTBOOL m_bUnknown10;                                       // 0x10
	LTBOOL m_bTreesValid;                                      // 0x14
	LTBOOL m_bUnknown18;                                       // 0x18
	LTBOOL m_bChildrenFound;                                   // 0x1c
};

class CClientMgrQ474;

// The two words the hook is given back (0x004771a3 then 0x004771b4 /
// 0x00477214).  Same shape as the server's CModelLoadContext, with a
// CClientMgr where the server holds a CServerMgr.
class CModelLoadContextQ474
{
public:
	CClientMgrQ474* m_pClientMgr;                              // 0x00
	char*           m_pFilename;                               // 0x04
};

// ---- the pending-load queue --------------------------------------------- //

// The 0x20-byte record 0x0047706f builds on the stack.  Only the three words
// this code writes are known: +0x08 and +0x10 are zeroed (0x0047707c,
// 0x00477087) and +0x14 takes the file record (0x0047708b); the constructor
// at 0x004441a0 writes the rest.  That constructor is __thiscall with no
// arguments (`lea 0xc(%esp),%ecx; call`), which is what a default
// constructor on a stack local compiles to.
class CPendingLoadQ474
{
public:
	CPendingLoadQ474();                                        // 0x004441a0

	uint8         m_pad_00[0x08];
	uint32        m_Unknown08;                                 // 0x08
	uint8         m_pad_0c[0x10 - 0x0c];
	uint32        m_Unknown10;                                 // 0x10
	CFileRecQ474* m_pFileRec;                                  // 0x14
	uint8         m_pad_18[0x20 - 0x18];
};

// 0x0047bef0 and 0x004442c0 are __thiscall -- `lea 0x15a8(%edi),%ecx` and
// `lea 0x15ac(%edi),%ecx` load the SUB-OBJECT, not the manager, and the
// callee cleans the one stack argument.  So each is a member of whatever
// lives at that displacement, and modelling them as free functions would
// emit a push where the image has a `lea` into %ecx.
class CLoadLookupQ474
{
public:
	LTBOOL IsPending(CFileRecQ474* pRec);                      // 0x0047bef0

	uint8 m_Opaque[0x15ac - 0x15a8];
};

class CLoadQueueQ474
{
public:
	void Add(CPendingLoadQ474* pReq);                          // 0x004442c0

	uint8 m_Opaque[0x160c - 0x15ac];
};

// ---- the client manager ------------------------------------------------- //
//
// Only the members this file touches, each at the displacement the shipped
// code uses.  0x1534 is src/engine/lithtech/clientmgr.h's m_pFileMgr.
class CClientMgrQ474
{
public:
	uint8          m_pad_0000[0x0420];
	// The object list walked at 0x004777d0 and 0x0047783b: a head at +0x420
	// with the cursor starting from +0x424.
	LTLink         m_ObjectList;                               // 0x0420
	uint8          m_pad_042c[0x10f4 - 0x042c];
	// The list a freshly registered model is threaded onto (0x00477790).
	LTLink         m_ModelList;                                // 0x10f4
	uint32         m_nSpriteCount;                             // 0x1100  ++ at 0x004776af
	LTLink         m_SpriteList;                               // 0x1104
	uint8          m_pad_1110[0x1534 - 0x1110];
	CFileMgrQ474*  m_pFileMgr;                                 // 0x1534
	uint8          m_pad_1538[0x15a8 - 0x1538];
	// The pending-model-load queue: +0x15a8 answers whether this file is
	// already queued (0x0047705a) and +0x15ac takes a new request
	// (0x0047708f).
	CLoadLookupQ474 m_LoadLookup;                              // 0x15a8
	CLoadQueueQ474  m_LoadQueue;                               // 0x15ac
	// 0x00477956 -- the model every tracker is parked on while the real one
	// is still loading, i.e. the client's copy of CServerMgr's
	// m_pPlaceholderModel.
	class ModelQ474* m_pPlaceholderModel;                      // 0x160c

	// 0x00424f20 -- `ret $8` thiscall with the MANAGER itself as `this`
	// (0x00477be0 calls it with %ecx still holding argument one):
	// worldmodels.cpp's CWorldModelHost::AttachWorldModel, which the server
	// reaches as pMgr->m_World.  (view, name as there)
	LTBOOL AttachWorldModel(class CClientModelInstQ474* pObject, char* pName);
};

// ---- the console command parser ----------------------------------------- //
//
// src/engine/server/servermgr.h's CommandParser, at the client's addresses:
// 0x0041b590 is CommandParser::Next (src/engine/lithtech/clientconsole.cpp
// names it) and 0x0041b5d0 is therefore CommandParser::Find -- console.cpp's
// `Find(pName, bCaseSensitive, nMinArgs)`, and its body matches instruction
// for instruction: `if (!nMinArgs) nMinArgs = 1` (0x0041b5da), `while
// (Next())` (0x0041b5e5), `m_nArgs >= nMinArgs` against +0x100 (0x0041b5f2),
// the case flag selecting the compare (0x0041b5fe).  The 0x1508-byte size is
// what settles it: 64 pointers, the count, the position and a 0x1400 buffer
// is exactly the stack 0x004776f0 reserves (0x1514 total, less the 0xc-byte
// file ident below it).
class CommandParserQ474
{
public:
	LTBOOL Find(char* pName, LTBOOL bCaseSensitive, int nMinArgs);  // 0x0041b5d0

	char* m_ArgPointers[64];                                   // 0x0000
	int   m_nArgs;                                             // 0x0100
	char* m_pPos;                                              // 0x0104
	char  m_ArgBuffer[0x1400];                                 // 0x0108
};

// 0x004048c0 -- open the file the ident names and hand back the stream.
// (The name is ours; what fixes it is that the result is handed straight to
// the sprite reader and then Release()d through vtable slot 4.)
ILTStreamQ474* cm_OpenFileQ474(CFileMgrQ474* pFileMgr, CFileIdentQ474* pIdent);

// 0x00483a60 -- read a sprite out of an open stream (name ours).  The
// layout is the loader's own (0x00483a88..0x00483aaa, 0x00483b4d..
// 0x00483b96); src/engine/lithtech/csvis.cpp's CSpriteAnimQ414 reads the
// same +0x20..+0x2c.
class CSpriteAnimQ474
{
public:
	char   m_sName[32];                                        // 0x00
	void** m_Frames;                                           // 0x20
	uint32 m_nFrames;                                          // 0x24
	uint32 m_MsAnimLength;                                     // 0x28
	uint32 m_MsFrameRate;                                      // 0x2c
	uint32 m_bKeyed;                                           // 0x30
	uint32 m_ColourKey;                                        // 0x34
	uint32 m_bTranslucent;                                     // 0x38
};

class CSpriteQ474
{
public:
	LTLink           m_Link;                                   // 0x00  m_pData
	                                                           //       points at
	                                                           //       the sprite
	CSpriteAnimQ474* m_Anims;                                  // 0x0c
	uint32           m_nAnims;                                 // 0x10
};
CSpriteQ474* spr_LoadQ474(CClientMgrQ474* pMgr, ILTStreamQ474* pStream);

// 0x0041ff90 -- src/engine/lithtech/ccclient.cpp already names this one:
// "lttexmod.cpp: resolve one CClientFileRef to a SharedTexture".
void* cm_FindSharedTextureQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent);

// 0x0041fc60 -- the manager's varargs error formatter.  src/engine/lithtech/
// clientrender.cpp reconstructed it: it adds 0x620 to its first argument and
// hands 0x12c as the length, so it formats into CClientMgr::m_ErrorString.
// A varargs MEMBER is __cdecl, which is why the manager is pushed LAST and
// the caller cleans; written here as a free function because that generates
// the identical call.
void cm_SetupErrorQ474(CClientMgrQ474* pMgr, LTRESULT dResult, ...);

// 0x00411bf0 -- what an object holding a model that just went away is told
// (name ours; only the two arguments the call site pushes are known).
void cm_OnModelGoneQ474(CClientMgrQ474* pMgr, class CClientObjectQ474* pObject);

// The client object, as far as cm_RegisterModel reaches it: +0x1ac is the
// file record it is waiting on (read at 0x004777e3) and +0x1dc is its model
// (written at 0x004777eb).  This is the SAME object CClientModelInstQ474
// models further down, restated here only because the object hooks that need
// the fuller layout are declared after this point; the two agree offset for
// offset.
class CClientObjectQ474
{
public:
	uint8         m_pad_000[0x1ac];
	CFileRecQ474* m_pPendingFileRec;                           // 0x1ac
	uint8         m_pad_1b0[0x1dc - 0x1b0];
	ModelQ474*    m_pModel;                                    // 0x1dc
};

// The keyed-list machinery src/engine/lithtech/ltnew1.cpp already models:
// CKeyedListView at an owner's +0x14, nodes keyed on the ADDRESS of a
// file-scope object at 0x004a8380 (its first dword is
// 0x00411d10, a code address, and ltnew1.cpp records that only its address is
// ever used).  0x0043c500 takes a 12-byte node off a locked bank, zeroes it
// and writes the key and the datum (0x0043c56c/0x0043c56e); 0x0043c580 pushes
// it on the list head (0x0043c599/0x0043c5a1).  Names ours.
class CDepRecord;                                              // ltnew1.cpp's
extern CDepRecord g_Unknown4a8380;                             // 0x004a8380 (ltnew1.cpp)

class CKeyedNodeQ474
{
public:
	void*           m_pKey;                                    // 0x00
	void*           m_pData;                                   // 0x04
	CKeyedNodeQ474* m_pNext;                                   // 0x08
};

class CKeyedListQ474
{
public:
	CKeyedNodeQ474* m_pHead;                                   // 0x00
};

CKeyedNodeQ474* cb_MakeNodeQ474(void* pKey, void* pData);      // 0x0043c500
LTRESULT        cb_AddNodeQ474(CKeyedListQ474* pList,
                               CKeyedNodeQ474* pNode);         // 0x0043c580

class ILTStreamQ474
{
public:
	virtual void _v00() = 0;
	virtual void Release() = 0;                                // slot 0x04
	// ltbasedefs.h's ILTStream order: Read at 0x08 and ErrorStatus at 0x10
	// (0x00483af9 and 0x00483c31, the sprite reader below).
	virtual LTRESULT Read(void* pData, uint32 size) = 0;       // slot 0x08
	virtual LTRESULT ReadString(char* pStr, uint32 max) = 0;   // slot 0x0c
	virtual LTRESULT ErrorStatus() = 0;                        // slot 0x10
};


// ---- forward declarations (the three recursive/mutual entry points) ----- //
LTRESULT cm_LoadModelDataQ474(CClientMgrQ474* pMgr, char* pFilename,
                              CFileRecQ474* pFileRec, ModelQ474** ppModel);
LTRESULT cm_LoadChildModelQ474(CModelLoadRequestQ474* pReq,
                               ModelQ474** ppModel);
LTRESULT cm_RegisterModelQ474(CClientMgrQ474* pMgr, ModelQ474* pModel,
                              CFileRecQ474* pFileRec, LTBOOL bRebind);
LTRESULT cm_LoadModelQ474(CClientMgrQ474* pMgr, CFileRecQ474* pFileRec,
                          ModelQ474** ppModel, LTBOOL bLoadNow);
LTRESULT cm_LoadSpriteQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent,
                           class CSpriteQ474** ppSprite);


// ======================================================================= //
//  0x00476e80 .. 0x00477010, 401 bytes -- LOAD A SPRITE
//
//  src/engine/lithtech/clientmgr.h already DECLARES this one, as
//  `sub_476e80(CClientMgr*, CClientFileRef*, void** ppTexture)`.  The arity
//  and the first two types are right; the third is a sprite, not a texture --
//  0x00476f42's result is threaded onto the list at +0x1104 and counted at
//  +0x1100, and the console line it prints is "Loading sprite: %s\n".
//
//  WHAT PINS THE SHAPE
//    * `mov 0x1534(%edi),%ecx` at 0x00476e97, again at 0x00476ea3 and a THIRD
//      time at 0x00476edb and a fourth at 0x00476f25.  Four reloads of one
//      field means it is written out at each call and never cached in a
//      local (the same rule that reads two `strlen` scans as two
//      written calls).
//    * the name is computed ONCE, at the top, and then SPILLED INTO THE
//      INCOMING ARGUMENT SLOT: `mov %ebx,0x28(%esp)` at 0x00476eaf writes
//      R+4, the pClientMgr slot, and 0x00476ef1 / 0x00476f50 read it back.
//      That is /O2 reusing a dead parameter slot as a local -- the same
//      thing serverengine.cpp records for se_InitModel -- and is not
//      source-reachable.
//    * TWO error exits, not one.  The missing-record exit at 0x00476fd0 and
//      the missing-file exit at 0x00476ef1 are separate expansions that
//      CROSS-JUMP into one another: 0x00476f17 `jmp 0x476fee` enters the
//      second copy's printf at its fourth push.  "Of two
//      identical sites, VC6 keeps the SECOND inline and jumps the first into
//      it", so both are written out in full here.
//    * the debug line at 0x00476f1c is gated on level 2 while every
//      RETURN_ERROR is gated on level 1.
// ======================================================================= //
LTRESULT cm_LoadSpriteQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent,
                           CSpriteQ474** ppSprite)
{
	char*          pName;
	CFileRecQ474*  pFileRec;
	ILTStreamQ474* pStream;
	CSpriteQ474*   pSprite;

	*ppSprite = LTNULL;

	pName    = cm_GetFileNameQ474(pMgr->m_pFileMgr, pIdent);
	pFileRec = cm_FindFileQ474(pMgr->m_pFileMgr, pIdent, 2);

	// POSITIVE GUARD, missing-record error LAST: 0x00476ebf `je 0x476fd0`
	// jumps forward to the final block of the function, so the body is the
	// fall-through and the refusal is the trailing statement.
	if (pFileRec)
	{
		// ONE shared `*ppSprite = pSprite; return LT_OK;` after an if/else,
		// not an early return in the cached arm: shipped holds the cached
		// sprite in %esi (0x00476ecc `mov %eax,%esi`) exactly as the loaded
		// one, and the early-return spelling also cost the cross-jump of
		// the missing-file error into the last block (0x00476f17).
		if (pFileRec->m_pData)
		{
			pSprite = (CSpriteQ474*)pFileRec->m_pData;
		}
		else
		{
			pStream = cm_OpenFileQ474(pMgr->m_pFileMgr, pIdent);

			if (!pStream)
			{
				cm_SetupErrorQ474(pMgr, LT_MISSINGSPRITEFILE, pName);
				RETURN_ERROR_SYM_DESC_Q474(1, "LoadSprite", LT_MISSINGSPRITEFILE, pName);
			}

			// 0x00476f1c -- level 2, and it asks the file manager for the name a
			// SECOND time rather than reusing pName.
			if (g_DebugLevel >= 2)
			{
				con_ConsolePrint("Loading sprite: %s\n",
				                 cm_GetFileNameQ474(pMgr->m_pFileMgr, pIdent));
			}

			pSprite = spr_LoadQ474(pMgr, pStream);

			if (!pSprite)
			{
				cm_SetupErrorQ474(pMgr, LT_INVALIDSPRITEFILE, pName);
				RETURN_ERROR_SYM_DESC_Q474(1, "LoadSprite", LT_INVALIDSPRITEFILE, pName);
			}

			// 0x00476f94..0x00476fbc -- the record takes the sprite, the sprite's
			// own link takes the sprite, and ltlink.h's CheapLTLink::AddAfter puts
			// it on the manager's list.  The six stores are AddAfter's, in
			// AddAfter's exact order, with the count bumped between the fifth and
			// the sixth.
			pFileRec->m_pData = pSprite;

			pSprite->m_Link.m_pData = pSprite;
			pMgr->m_SpriteList.AddAfter(&pSprite->m_Link);

			pMgr->m_nSpriteCount++;

			pStream->Release();
		}

		*ppSprite = pSprite;

		return LT_OK;
	}

	cm_SetupErrorQ474(pMgr, LT_MISSINGSPRITEFILE, pName);
	RETURN_ERROR_SYM_DESC_Q474(1, "LoadSprite", LT_MISSINGSPRITEFILE, pName);
}


// ======================================================================= //
//  0x00477020 .. 0x004770d6, 183 bytes -- LOAD ONE MODEL, OR QUEUE IT
//
//  Two arms, and which one falls through is what fixes the source order:
//  `cmp %ebx,%eax; jne 0x4770a0` at 0x0047704b sends the NON-zero case away
//  and falls into the queueing block, so the queueing block is the `if` and
//  the synchronous load is what follows it ("the fall-through arm
//  is the one that CONTINUES").
//
//  The zero the four stores use is the same `xor %ebx,%ebx` at 0x00477049
//  that the test uses; that is allocation, not source.
//
//  The name is ours.  The only literal in the body is the console line, and
//  the RETURN_ERROR symbol its CALLER uses is "LoadModel2" -- so the shipped
//  source almost certainly called this one LoadModel.
// ======================================================================= //
LTRESULT cm_LoadModelQ474(CClientMgrQ474* pMgr, CFileRecQ474* pFileRec,
                          ModelQ474** ppModel, LTBOOL bLoadNow)
{
	LTRESULT dResult;

	if (g_DebugLevel >= 2)
		con_ConsolePrint("(Client) loading model: %s", pFileRec->m_pName);

	if (!bLoadNow)
	{
		if (pMgr->m_LoadLookup.IsPending(pFileRec))
			return LT_INPROGRESS;

		// Declared HERE: its constructor (0x004441a0) runs at 0x00477073,
		// after the IsPending test, not on entry.
		CPendingLoadQ474 request;

		request.m_Unknown08 = 0;
		request.m_Unknown10 = 0;
		request.m_pFileRec  = pFileRec;

		pMgr->m_LoadQueue.Add(&request);

		return LT_INPROGRESS;
	}

	dResult = cm_LoadModelDataQ474(pMgr, pFileRec->m_pName, pFileRec, ppModel);

	if (!dResult)
		cm_RegisterModelQ474(pMgr, *ppModel, pFileRec, LTTRUE);

	return dResult;
}


// ======================================================================= //
//  0x004770e0 .. 0x00477171, 146 bytes -- "LoadModel2"
//
//  The name is the SHIPPED one: 0x0047715d pushes the literal at 0x004ae404,
//  which reads "LoadModel2".
//
//  SIX parameters, and every one of them is read off this function's own
//  frame rather than inferred from a call site: R+4 (0x004770e5), R+8
//  (0x004770e0), R+0xc (0x0047710b / 0x00477121), R+0x10 (0x004770f8),
//  R+0x14 (0x00477115), R+0x18 (0x0047711d).
//
//  FOUR `ret`s and no shared epilogue: four separate early returns, which is
//  the "several duplicated exits = separate early `return`s" pattern.
// ======================================================================= //
LTRESULT cm_LoadModel2Q474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent,
                           ModelQ474** ppModel, CFileRecQ474** ppFileRec,
                           LTBOOL bMayLoad, LTBOOL bLoadNow)
{
	CFileRecQ474* pFileRec;

	pFileRec = cm_FindFileQ474(pMgr->m_pFileMgr, pIdent, 1);

	*ppFileRec = pFileRec;

	// POSITIVE GUARD, with the error LAST.  0x00477103's `je 0x0047713a`
	// jumps FORWARD to the RETURN_ERROR block at the very end of the
	// function, so the continuing case is the fall-through and the error is
	// the trailing statement.  Written as `if (!pFileRec) RETURN_ERROR` the
	// macro expands where the statement stands and the continue becomes the
	// jump -- same instructions, wrong order.
	if (pFileRec)
	{
		if (pFileRec->m_pData)
		{
			*ppModel = (ModelQ474*)pFileRec->m_pData;
			return LT_OK;
		}

		// Same shape again, one level down: 0x0047711b's `je 0x00477133`
		// puts `return LT_INPROGRESS` out of line at the end, so the call is
		// the fall-through and the early return is the trailing statement.
		if (bMayLoad)
			return cm_LoadModelQ474(pMgr, pFileRec, ppModel, bLoadNow);

		return LT_INPROGRESS;
	}

	RETURN_ERROR_Q474(1, "LoadModel2", LT_MISSINGFILE)
}


// ======================================================================= //
//  0x00477180 .. 0x00477430, 689 bytes -- cm_LoadChildModel
//
//  THE NAME IS SHIPPED (0x00477255, 0x004772d5, 0x0047733d, 0x00477413 all
//  push 0x004ae410 -> "cm_LoadChildModel").  It is the client's counterpart
//  to src/engine/server/serverengine.cpp's se_LoadChildModel and the two
//  bodies line up statement for statement, with three real differences:
//
//    1. the path splitter is rezmgr's 0x00490e50, whose SECOND argument is
//       the directory, where the server calls the CRT `_splitpath` (at
//       0x00495313 in this image) and puts its buffer in the THIRD;
//    2. there is no model hash -- the file manager's own record carries the
//       loaded model at +0x00, so the "already loaded" test is a field read
//       rather than a dhash lookup;
//    3. the registration is a call to 0x004776f0 rather than an inline
//       cleanup record, and its result is what decides LT_ERROR.
//
//  THE FRAME, read off the displacements (esp = R-0x41c after the prologue):
//    R-0x200 .. R-0x001   0x200 bytes, the splitter's directory output
//                         (0x0047719a `lea 0x21c(%esp)`, tested byte-wise at
//                         0x004771c0 and printed at 0x004771e9)
//    R-0x400 .. R-0x201   0x200 bytes, the assembled path (sprintf's
//                         destination at 0x004771f2, the ident's filename at
//                         0x00477210, the error description at 0x0047724b)
//    R-0x40c .. R-0x401   the 0xc-byte file ident, whose inline constructor
//                         is the three stores at 0x004771a7/ab/af
//
//  WHICH sprintf ARM IS WHICH.  `test %al,%al; jne 0x4771e6` at 0x004771ca
//  sends the NON-empty directory to the "%s\%s" form and FALLS THROUGH to
//  the bare "%s" -- so the source tests for the empty directory positively,
//  exactly as the server's does.
// ======================================================================= //
LTRESULT cm_LoadChildModelQ474(CModelLoadRequestQ474* pReq, ModelQ474** ppModel)
{
	CFileIdentQ474 ident;
	char           szPath[0x200];
	char           szDir[0x200];

	CModelLoadContextQ474* pCtx;
	CFileRecQ474*          pFileRec;
	ModelQ474*             pModel;
	LTRESULT               dResult;

	*ppModel = LTNULL;

	pCtx = pReq->m_pContext;

	CHelpers::ExtractNames(pCtx->m_pFilename, szDir, LTNULL, LTNULL, LTNULL);

	if (szDir[0] == 0)
		sprintf(szPath, "%s", pReq->m_pChildName);
	else
		sprintf(szPath, "%s\\%s", szDir, pReq->m_pChildName);

	ident.m_nFileType = 1;
	ident.m_pFilename = szPath;

	pFileRec = cm_FindFileQ474(pCtx->m_pClientMgr->m_pFileMgr, &ident, 1);

	if (!pFileRec)
	{
		RETURN_ERROR_SYM_DESC_Q474(1, "cm_LoadChildModel", LT_MISSINGMODELFILE,
		                           szPath);
	}

	if (pFileRec->m_pData)
	{
		*ppModel = (ModelQ474*)pFileRec->m_pData;
		return LT_OK;
	}

	// 0x00477289..0x004772af -- `push $0x2c4; call 0x004290b0; test; je;
	// <ctor>; mov %eax,%esi` is VC6's `new`, with the null arm folded
	// straight into the out-of-memory block below instead of the usual
	// `xor`-and-join (0x00477440 below has the unfolded form of the same
	// source).  0x004b4e88 is the allocator, pushed twice exactly as
	// serverengine.cpp's calls do.
	pModel = new ModelQ474(g_MoAllocBankQ474, g_MoAllocBankQ474);

	if (!pModel)
		RETURN_ERROR_Q474(1, "cm_LoadChildModel", LT_OUTOFMEMORY)

	pReq->m_pStream = cm_OpenStreamQ474(pCtx->m_pClientMgr->m_pFileMgr, pFileRec);

	if (!pReq->m_pStream)
	{
		delete pModel;

		RETURN_ERROR_SYM_DESC_Q474(1, "cm_LoadChildModel", LT_MISSINGMODELFILE,
		                           szPath);
	}

	dResult = pModel->Load(pReq->m_pStream, g_MoAllocBankQ474);

	if (!dResult)
		dResult = pModel->LoadChildModels(pReq);

	pReq->m_pStream->Release();

	// 0x00477392 and 0x004773b0 reach the SAME teardown, and the image keeps
	// the second copy inline and jumps the first into it (0x004773ce `jl
	// 0x477421` / 0x004773d0 `jmp 0x477404`).  The load test is a POSITIVE
	// guard with its failure LAST: 0x00477394 `jne 0x4773e8` jumps forward
	// to the final block, so the registration is the fall-through and the
	// load failure is the trailing statement.  The registration only
	// happens when the load worked.
	if (!dResult)
	{
		// The result is DISCARDED here, where 0x00477440's copy of the same
		// call gates on it -- 0x0047739d's return value is never tested.
		pModel->SetFilename(szPath);

		if (cm_RegisterModelQ474(pCtx->m_pClientMgr, pModel, pFileRec, LTFALSE))
		{
			delete pModel;

			RETURN_ERROR_Q474(1, "cm_LoadChildModel", LT_ERROR)
		}

		*ppModel = pModel;

		return LT_OK;
	}

	delete pModel;

	RETURN_ERROR_Q474(1, "cm_LoadChildModel", LT_ERROR)
}


// ======================================================================= //
//  0x00477440 .. 0x004776ec, 685 bytes -- cm_LoadModelData
//
//  THE NAME IS SHIPPED (0x004774cd, 0x0047753e, 0x00477613, 0x0047767f all
//  push 0x004ae424 -> "cm_LoadModelData").  serverengine.cpp's
//  se_LoadModelData is the same function on the server side and raises the
//  same three console strings.
//
//  THE MODEL LIVES IN *ppModel, NOT IN A LOCAL.  0x004774a7 stores the
//  freshly built model through the fourth argument straight away and every
//  later use reloads it (0x00477507, 0x00477558, 0x0047756b, 0x004775ac,
//  0x004775c4, 0x00477635, 0x004776a8, 0x004776c0).  A local would have
//  stayed in a register; eight reloads of the same indirection is what the
//  source says.
//
//  THE FRAME (esp = R-0x34 after the prologue):
//    R-0x20 .. R-0x01   the 0x20-byte CModelLoadRequest, whose constructor
//                       is inlined at 0x00477450..0x0047747f
//    R-0x28 .. R-0x21   the two-word CModelLoadContext (0x0047756d,
//                       0x0047757e)
//
//  THE CONSTRUCTOR'S STORE ORDER as shipped is child name, stream, callback,
//  context, then the four flags -- one transposition away from the order
//  serverengine.cpp recovered for the out-of-line server copy (name, stream,
//  context, callback).  Inlining reschedules adjacent stores, so this is
//  written in the server's order and the transposition is left as a known
//  risk rather than chased.
// ======================================================================= //
LTRESULT cm_LoadModelDataQ474(CClientMgrQ474* pMgr, char* pFilename,
                              CFileRecQ474* pFileRec, ModelQ474** ppModel)
{
	CModelLoadRequestQ474 request;
	CModelLoadContextQ474 context;
	LTRESULT              dResult;

	*ppModel = new ModelQ474(g_MoAllocBankQ474, g_MoAllocBankQ474);

	if (!*ppModel)
		RETURN_ERROR_Q474(1, "cm_LoadModelData", LT_OUTOFMEMORY)

	request.m_pStream = cm_OpenStreamQ474(pMgr->m_pFileMgr, pFileRec);

	if (!request.m_pStream)
	{
		delete *ppModel;

		cm_SetupErrorQ474(pMgr, LT_MISSINGMODELFILE, pFilename);

		RETURN_ERROR_SYM_DESC_Q474(1, "cm_LoadModelData", LT_MISSINGMODELFILE,
		                           pFilename);
	}

	dResult = (*ppModel)->Load(request.m_pStream, g_MoAllocBankQ474);

	if (!dResult)
	{
		context.m_pClientMgr = pMgr;
		context.m_pFilename  = pFilename;

		request.m_pfnLoadChild = cm_LoadChildModelQ474;
		request.m_pContext     = &context;

		dResult = (*ppModel)->LoadChildModels(&request);
	}

	request.m_pStream->Release();

	if (!dResult && (*ppModel)->SetFilename(pFilename))
	{
		// 0x004775bc reads request+0x14 and 0x0047762d reads request+0x1c --
		// the two flags serverengine.h already names off the same two errors.
		if (request.m_bTreesValid == 0)
		{
			delete *ppModel;

			if (g_DebugLevel >= 1)
			{
				con_ConsolePrint("Child model trees invalid in model %s",
				                 pFilename);
			}

			RETURN_ERROR_Q474(1, "cm_LoadModelData", LT_INVALIDMODELFILE)
		}

		if (request.m_bChildrenFound == 0)
		{
			delete *ppModel;

			if (g_DebugLevel >= 1)
			{
				con_ConsolePrint("Missing one or more child models in model %s",
				                 pFilename);
			}

			RETURN_ERROR_Q474(1, "cm_LoadModelData", LT_INVALIDMODELFILE)
		}

		// 0x00477699..0x004776b4 -- a keyed node standing for this file goes
		// on the model's own +0x14 list, so that when the model dies the
		// file record it came out of can be found again.  Where the server
		// stores the file id into the model instead.
		cb_AddNodeQ474((CKeyedListQ474*)&(*ppModel)->m_CleanupHead,
		               cb_MakeNodeQ474(&g_Unknown4a8380, pFileRec));

		return LT_OK;
	}

	delete *ppModel;

	if (g_DebugLevel >= 1)
		con_ConsolePrint("Error %d loading model %s", dResult, pFilename);

	return dResult;
}


// ======================================================================= //
//  0x004776f0 .. 0x00477808, 281 bytes -- REGISTER A FRESHLY LOADED MODEL
//
//  THE ENUMERATED ROW WAS WRONG, TWICE.  The enumeration carries 0x004776f0
//  with 75 bytes and 0x0047773b with 206 as separate rows;
//  the first has a branch leaving it
//  (0x0047771d -> 0x0047773b), i.e. 0x0047773b is this function's own taken
//  arm, and 0x0047773b/206 then has one clean `ret`
//  at 0x00477808.  One function, 0x004776f0..0x00477808.
//
//  `mov $0x1514,%eax; call 0x00492c60` is __chkstk, which is what a frame
//  over 4KB costs, and 0x1514 is 0xc (the file ident) plus 0x1508 -- exactly
//  sizeof(CommandParser) as src/engine/server/servermgr.h lays it out.  That
//  is what identifies the big local.
//
//  ITS RETURN IS AN LTRESULT AND ALWAYS LT_OK (`xor %eax,%eax` on both
//  exits, 0x00477731 and 0x004777ff), but both callers test it --
//  0x004773b0 branches on it -- so the type is kept.
// ======================================================================= //
LTRESULT cm_RegisterModelQ474(CClientMgrQ474* pMgr, ModelQ474* pModel,
                              CFileRecQ474* pFileRec, LTBOOL bRebind)
{
	CommandParserQ474 parser;
	LTLink*           pCur;
	LTLink*           pHead;

	parser.m_pPos = LTNULL;

	// DECLARED HERE, NOT AT THE TOP.  CFileIdent has a constructor and
	// CommandParser does not, so declaration order decides which stores come
	// first: 0x00477705 writes parser.m_pPos (+0x118) BEFORE the ident
	// constructor's three stores at 0x0047770c/0x00477712/0x00477718.
	// Declaring ident first inverts that and costs the function its match.
	CFileIdentQ474    ident;

	// 0x00477710 -- somebody else got there first, so the model just loaded
	// is surplus.  `mov (%ecx),%eax; push $1; call *(%eax)` behind a null
	// test is `delete pModel`.
	if (pFileRec->m_pData)
	{
		delete pModel;

		return LT_OK;
	}

	// 0x0047774c -- the model's own property text, read for one named
	// property.  The three arguments the call pushes are Find's:
	// ("FadeSpriteTex", LTFALSE, 1).
	parser.m_pPos = (char*)pModel->m_pProps;

	if (parser.Find("FadeSpriteTex", LTFALSE, 1))
	{
		ident.m_nFileType = 0;
		ident.m_pFilename = parser.m_ArgPointers[1];

		pModel->m_pFadeSpriteTex = cm_FindSharedTextureQ474(pMgr, &ident);
	}

	// 0x0047778d..0x004777ab -- CheapLTLink::AddAfter in its exact six-store
	// order, then the record takes the model.  The model's own +0x10 already
	// holds the model (its constructor writes it), which is why there is no
	// m_pData store here where cm_LoadSprite has one.
	pMgr->m_ModelList.AddAfter(&pModel->m_Link);

	pFileRec->m_pData = pModel;

	cb_AddNodeQ474((CKeyedListQ474*)&pModel->m_CleanupHead,
	               cb_MakeNodeQ474(&g_Unknown4a8380, pFileRec));

	// 0x004777c6..0x004777f8 -- only when the caller asked, and only when the
	// record still carries its bit-0 flag: every object that was waiting on
	// THIS file record gets the model, and the flag is cleared afterwards.
	// The walk is the head at +0x420 with the cursor off +0x424, which is
	// `m_pNext` in ltlink.h's layout.
	if (bRebind && (pFileRec->m_Flags19 & 1))
	{
		pHead = &pMgr->m_ObjectList;

		for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
		{
			CClientObjectQ474* pObject = (CClientObjectQ474*)pCur->m_pData;

			if (pObject->m_pPendingFileRec == pFileRec)
				pObject->m_pModel = pModel;
		}

		pFileRec->m_Flags19 &= ~1;
	}

	return LT_OK;
}


// ======================================================================= //
//  0x00477830, 111 bytes -- UNLOAD A MODEL (name ours).
//
//  The undo of cm_RegisterModel: every object on the list at +0x420 whose
//  model (+0x1dc) is this one is told it went away (0x00411bf0, the walk
//  saves m_pNext first because that call can unlink the object), the file
//  record's m_pData is cleared (`movl $0,(%eax)` at 0x00477871), and unless
//  the model still carries a 0x004a8380-keyed cleanup node (0x00477810,
//  src/engine/lithtech/ltnew1.cpp) its own link at +0x08 comes off the
//  model list -- CheapLTLink::Remove's four stores at 0x00477883..0x00477899.
//  Its caller is ltnew1.cpp's 0x00411d10, which pushes (g_pClientMgr, model,
//  record).  The return value is never set (eax is left over), so it is void.
// ======================================================================= //
class CKeyedListOwnerView;
LTBOOL sub_477810(void* pUnused, CKeyedListOwnerView* pOwner); // 0x00477810

void cm_UnloadModelQ474(CClientMgrQ474* pMgr, ModelQ474* pModel,
                        CFileRecQ474* pFileRec)
{
	LTLink* pCur;
	LTLink* pNext;
	LTLink* pHead;

	pHead = &pMgr->m_ObjectList;
	pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		pNext = pCur->m_pNext;

		CClientObjectQ474* pObject = (CClientObjectQ474*)pCur->m_pData;

		if (pObject->m_pModel == pModel)
			cm_OnModelGoneQ474(pMgr, pObject);

		pCur = pNext;
	}

	pFileRec->m_pData = LTNULL;

	if (!sub_477810(pMgr, (CKeyedListOwnerView*)pModel))
		pModel->m_Link.Remove();
}


// ======================================================================= //
//  THE OBJECT-CREATION HOOKS, 0x004778a0 .. 0x00477cd5
//
//  These are the per-object-type entries the table at 0x004ae340 dispatches
//  to (0x00477d60 indexes it by the object's +0xbc type byte with a stride
//  of 8 and calls slot 0; 0x00477d90 calls slot 1).  Every one of them takes
//  the same four arguments, and 0x00477d60's own forwarding call fixes the
//  order: (pClientMgr, pObject, pCreateRequest, dResult).
// ======================================================================= //

// src/engine/lithtech/clientmgr.h's CClientCreateRequest, restated: the
// object's own file at +0x00, one file per skin at +0x0c, and the create
// struct at +0x24.  src/LT2/sdk/inc/ltbasedefs.h's MAX_MODEL_TEXTURES is 2,
// which is what makes +0x24 the next field -- and the skin loop at
// 0x004779eb runs from +0x1b8 to +0x1c0 in steps of 4, i.e. exactly twice.
class ObjectCreateStructQ474
{
public:
	uint32 m_ObjectType;                                       // 0x00
	uint32 m_Flags;                                            // 0x04  bit 0 read
	                                                           //       at 0x004778bf
	uint8  m_pad_08[0x7c - 0x08];
	char   m_Filename[1];                                      // 0x7c  (0x004778a1)
};

class CClientCreateRequestQ474
{
public:
	CFileIdentQ474          m_ObjectFile;                      // 0x00
	CFileIdentQ474          m_SkinFiles[2];                    // 0x0c
	ObjectCreateStructQ474* m_pStruct;                         // 0x24
};

// src/LT2/sdk/inc/ltanimtracker.h, at the offsets the loop below uses: the link
// at +0x00 (GetNext is m_Link.m_pNext, +0x04), the flags at +0x18, and
// AnimTimeRef from +0x1c -- m_pModel +0x1c, m_Prev.m_iAnim +0x20,
// m_Cur.m_iAnim +0x28.
class FrameLocatorQ474
{
public:
	uint16 m_iAnim;                                            // 0x00
	uint16 m_iFrame;                                           // 0x02
	uint32 m_Time;                                             // 0x04
};

class LTAnimTrackerQ474
{
public:
	LTAnimTrackerQ474* GetNext() { return (LTAnimTrackerQ474*)m_Link.m_pNext; }

	LTLink           m_Link;                                   // 0x00
	uint8            m_pad_0c[0x18 - 0x0c];
	uint16           m_Flags;                                  // 0x18
	uint16           m_InterpolationMS;                        // 0x1a
	ModelQ474*       m_pModel;                                 // 0x1c  m_TimeRef.m_pModel
	FrameLocatorQ474 m_Prev;                                   // 0x20
	FrameLocatorQ474 m_Cur;                                    // 0x28
};

// 0x00401050 -- modelanim.cpp's at_Init.  What places it: the decoded map
// puts ?at_IsDone@@YAIPAVLTAnimTracker@@@Z at 0x00401100 and
// ?at_Advance@...@Z at 0x00401370, and src/engine/server/modelanim.cpp
// defines at_Init immediately BEFORE at_IsDone -- so the function that ends
// where 0x00401100 begins is at_Init.
void at_InitQ474(LTAnimTrackerQ474* pTracker, ModelQ474* pModel, uint32 iAnim);

// What a sprite skin leaves behind: 0x00477b1c..0x00477b21 walks
// record->+0x08 -> +0x00 -> +0x24 -> +0x1c to reach the texture's name.  Only
// that chain is modelled and every name here is ours.
class CTexRefQ474
{
public:
	uint8 m_pad_00[0x1c];
	char* m_pName;                                             // 0x1c
};

class CSpriteDataQ474
{
public:
	uint8        m_pad_00[0x24];
	CTexRefQ474* m_pTexRef;                                    // 0x24
};

class CSpriteFrameQ474
{
public:
	CSpriteDataQ474* m_pData;                                  // 0x00
};

class CSpriteTrackerQ474
{
public:
	uint8             m_pad_00[0x08];
	CSpriteFrameQ474* m_pFrame08;                              // 0x08
	uint8             m_pad_0c[0x14 - 0x0c];
};

// 0x00483cc0 -- src/engine/lithtech/clientmgr.h already declares this one.
void sub_483cc0Q474(CSpriteTrackerQ474* pRec, void* pSprite);

class CClientModelInstQ474;
LTRESULT cm_InitSkinQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent,
                         CClientModelInstQ474** ppObject, uint32 iSkin);

// The client's model instance, at the offsets this file reaches.  +0x1dc is
// src/engine/lithtech/clientmgr.h's CClientModel::m_pModel and +0x1f8 is
// src/engine/server/commonmodel.h's ModelInstance::m_pTrackerList; both land
// at the same displacement on the client object, which is what says the two
// headers are describing one layout.
class CClientModelInstQ474
{
public:
	uint8              m_pad_000[0x1a8];
	void*              m_pWorldModel;                          // 0x1a8
	CFileRecQ474*      m_pPendingFileRec;                      // 0x1ac
	uint8              m_pad_1b0[0x1b8 - 0x1b0];
	void*              m_pSkins[2];                            // 0x1b8
	uint8              m_pad_1c0[0x1dc - 0x1c0];
	ModelQ474*         m_pModel;                               // 0x1dc
	uint8              m_pad_1e0[0x1f8 - 0x1e0];
	LTAnimTrackerQ474* m_pTrackerList;                         // 0x1f8
	uint8              m_pad_1fc[0x208 - 0x1fc];
	void*              m_pSprites[2];                          // 0x208
	// 0x00477af4 forms the index as `iSkin * 20`, and 0x00477b11 reads
	// +0x218 of the same record, so these are 20-byte records starting at
	// +0x210 with a pointer at their +0x08.
	class CSpriteTrackerQ474 m_SpriteRecs[2];                  // 0x210
	uint8              m_pad_238[0x270 - 0x238];
	uint16             m_nUnknown270;                          // 0x270

	// 0x0045aba0 -- takes the twelve-byte vector BY VALUE (the shipped call
	// makes room with `sub $0xc,%esp` and writes the three words through
	// %esp rather than pushing a pointer).
	void SetDims(LTVector vDims);
};

// 0x0041ff50 -- lttexmod.cpp's texture resolver (src/engine/lithtech/
// clienttexmgr.cpp already records that 0x0041ff90 forwards to it).
LTRESULT cm_LoadTextureQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent,
                            void** ppTexture);
// 0x0042ee70 -- console.cpp's case-insensitive EQUALS
// (src/engine/lithtech/clientworld.cpp names it).  It answers TRUE on a
// match, which is why the DTX arm is the `jne` fall-through and not the `je`.
LTBOOL con_ICompare(char* pA, char* pB);


// ======================================================================= //
//  0x004778a0 .. 0x00477a1e, 383 bytes -- the MODEL type's create hook
//
//  The client's se_InitModel.  Same three parts in the same order: drop the
//  old model's use count, resolve the model file, then walk the object's
//  extra trackers.  src/engine/server/serverengine.cpp's se_InitModel is the
//  server copy and its two tracker loops are these two.
//
//  WHAT PINS THE TWO LOOPS.  Both are the same four-condition test in the
//  same order (0x0047790c and 0x00477973): model set, m_Cur.m_iAnim below
//  the model's +0x16c, m_Prev.m_iAnim below it, and the model not already
//  the one being installed.  That test is AnimTimeRef::IsValid's body, and
//  VC6 has no cross-TU inlining, so the client source spells it out rather
//  than calling IsValid -- which is exactly why the server's copy (where it
//  IS a call) reads differently.
//
//  THE SECOND LOOP additionally sets AT_ flag 8 on every tracker it
//  re-inits and hoists the constant into %edi (0x0047796e) -- allocation,
//  not source.
//
//  THE SKIN LOOP passes &pObject -- the ADDRESS of its own second parameter
//  (0x004779ef `lea 0x20(%esp)` is R+8).  That is why 0x004778ac stores the
//  parameter back into its own incoming slot: the parameter is
//  address-taken, so it has to live in memory.
//
//  IDENTICAL (383 of 383).  Three source facts closed it from 21 differing
//  instructions: the address-taken pointer is a LOCAL copy of the parameter
//  (`pObject = pObj`, homed in the dead parameter slot -- that is the store
//  at 0x004778ac); the auto-load flag is a named local read before the
//  cache-count decrement (0x004778b7..0x004778bf hoist it above the test);
//  and the placeholder is assigned INTO pModel (0x00477960 stores it to
//  pModel's frame slot and 0x004779a8 reloads it from there after each
//  at_Init), not a separate local.
// ======================================================================= //
LTRESULT cm_InitModelQ474(CClientMgrQ474* pMgr, CClientModelInstQ474* pObj,
                          CClientCreateRequestQ474* pRequest, LTRESULT dResult)
{
	CClientModelInstQ474* pObject = pObj;
	ModelQ474*         pModel;
	CFileRecQ474*      pFileRec;
	LTAnimTrackerQ474* pTracker;
	LTRESULT           dRes;
	uint32             i;
	uint32             bAutoLoad = pRequest->m_pStruct->m_Flags & 1;

	if (pObject->m_pModel)
		pObject->m_pModel->m_nCacheCount--;

	if (!dResult)
	{
		dRes = cm_LoadModel2Q474(pMgr, &pRequest->m_ObjectFile, &pModel, &pFileRec,
		                         bAutoLoad, bAutoLoad);

		if (!dRes)
		{
			for (pTracker = pObject->m_pTrackerList; pTracker;
			     pTracker = pTracker->GetNext())
			{
				if (!pTracker->m_pModel ||
				    pTracker->m_Cur.m_iAnim  >= pTracker->m_pModel->m_nNodes ||
				    pTracker->m_Prev.m_iAnim >= pTracker->m_pModel->m_nNodes ||
				    pTracker->m_pModel != pModel)
				{
					at_InitQ474(pTracker, pModel, LTFALSE);
				}
			}
		}
		else if (dRes == LT_INPROGRESS)
		{
			// 0x00477956 -- the stand-in model every waiting tracker gets.
			pModel = pMgr->m_pPlaceholderModel;

			for (pTracker = pObject->m_pTrackerList; pTracker;
			     pTracker = pTracker->GetNext())
			{
				if (!pTracker->m_pModel ||
				    pTracker->m_Cur.m_iAnim  >= pTracker->m_pModel->m_nNodes ||
				    pTracker->m_Prev.m_iAnim >= pTracker->m_pModel->m_nNodes ||
				    pTracker->m_pModel != pModel)
				{
					at_InitQ474(pTracker, pModel, LTFALSE);

					pTracker->m_Flags |= 8;
				}
			}

			pObject->m_pPendingFileRec = pFileRec;

			pFileRec->m_Flags19 |= 1;
		}
		else
		{
			return dRes;
		}
	}

	if (pObject->m_pModel)
		pObject->m_pModel->m_nCacheCount++;

	for (i = 0; i < 2; i++)
	{
		pObject->m_pSkins[i] = LTNULL;

		cm_InitSkinQ474(pMgr, &pRequest->m_SkinFiles[i], &pObject, i);
	}

	return LT_OK;
}


// ======================================================================= //
//  0x00477a20 .. 0x00477b51, 306 bytes -- one skin
//
//  It is handed the ADDRESS of the caller's object pointer, not the object,
//  and reloads through it after every call (0x00477a3c, 0x00477aa0,
//  0x00477af2, 0x00477b0c, 0x00477b2b) -- five reloads of `(*ppObject)`,
//  which is what says the source dereferences the parameter each time
//  instead of taking a local copy.
//
//  WHAT PINS THE EXTENSION TEST.  `repnz scasb` + `not` + `dec` is one
//  written-out strlen (two scans would be two calls); the three
//  bytes at pName[n-3..n-1] are copied into a four-byte buffer that lives in
//  the DEAD iSkin argument slot (0x00477a76 writes R+0x10), NUL-terminated
//  at 0x00477a8f, and compared first against "DTX" and then against "SPR".
//  The compare answers TRUE on a match, so the DTX arm is the `jne`
//  fall-through.
// ======================================================================= //
LTRESULT cm_InitSkinQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent,
                         CClientModelInstQ474** ppObject, uint32 iSkin)
{
	char     szExt[4];
	char*    pName;
	int      nLen;
	LTRESULT dResult;

	(*ppObject)->m_pSprites[iSkin] = LTNULL;

	pName = cm_GetFileNameQ474(pMgr->m_pFileMgr, pIdent);

	nLen = strlen(pName);

	// NESTED POSITIVE GUARDS, one LT_ERROR after them all: shipped has ONE
	// `mov $1,%eax` block (0x00477b48) that the length test, the "SPR" test
	// and the frame test all jump to, and the sprite loader's own error
	// (0x00477af0 `jne 0x00477b4d`) lands just past it on the shared
	// epilogue.  Three early `return LT_ERROR`s gave three copies.
	if (nLen > 3)
	{
		szExt[0] = pName[nLen - 3];
		szExt[1] = pName[nLen - 2];
		szExt[2] = pName[nLen - 1];
		szExt[3] = 0;

		if (con_ICompare(szExt, "DTX"))
		{
			return cm_LoadTextureQ474(pMgr, pIdent,
			                          &(*ppObject)->m_pSkins[iSkin]);
		}

		if (con_ICompare(szExt, "SPR"))
		{
			dResult = cm_LoadSpriteQ474(pMgr, pIdent,
			              (CSpriteQ474**)&(*ppObject)->m_pSprites[iSkin]);

			if (dResult)
				return dResult;

			// 0x00477af4 -- `lea 0x0(%ebp,%ebp,4); shl $0x2` is iSkin * 20, so
			// the records at +0x210 are 20 bytes each and +0x218 is the third
			// word of one.  Only the two words this function touches are
			// modelled.
			sub_483cc0Q474(&(*ppObject)->m_SpriteRecs[iSkin],
			               (*ppObject)->m_pSprites[iSkin]);

			if ((*ppObject)->m_SpriteRecs[iSkin].m_pFrame08)
			{
				pIdent->m_pFilename =
					(*ppObject)->m_SpriteRecs[iSkin].m_pFrame08->m_pData->m_pTexRef->m_pName;

				return cm_LoadTextureQ474(pMgr, pIdent,
				                          &(*ppObject)->m_pSkins[iSkin]);
			}
		}
	}

	return LT_ERROR;
}

// ======================================================================= //
//  0x00477b60 .. 0x00477b76, 23 bytes -- the MODEL type's remove hook
//
//  Slot 1 of the MODEL row of the create-hook table 0x004ae340 (read at
//  0x004ae34c; smgrnew.cpp's Part B lists it).  0x00477d90 dispatches slot 1
//  with just (pClientMgr, pObject) -- `push %ecx; push 0x8(%esp)` -- so this
//  takes two arguments and reads only the second (0x00477b60 `mov
//  0x8(%esp)`).  It is the other half of cm_InitModelQ474's first two lines:
//  the model the object held gives back its use count (+0x208, dropped at
//  0x00477b6e), and the answer is LT_OK (`xor %eax,%eax`).  (name ours)
// ======================================================================= //
LTRESULT cm_TermModelQ474(CClientMgrQ474* pMgr, CClientModelInstQ474* pObject)
{
	if (pObject->m_pModel)
		pObject->m_pModel->m_nCacheCount--;

	return LT_OK;
}


// ======================================================================= //
//  0x00477b80 .. 0x00477cd5, 342 bytes -- the WORLDMODEL type's create hook
//
//  THE NAME IS SHIPPED: 0x00477cbd pushes 0x004ae448, which
//  reads "WorldModelExtraInit", and 0x004ae45c is the literal
//  "LT_MISSINGWORLDMODEL" -- so this is de_error.h's RETURN_ERROR_SYM_DESC
//  form with the create struct's filename as the description.
//
//  ONE ODDITY, READ OFF THE BYTES AND LEFT AS IT IS.  The first arm is
//  entered when the MANAGER pointer is null (0x00477b99 `test %ecx,%ecx`
//  where %ecx is R+4), and it then hands that same null pointer to
//  0x0041fc60 (0x00477baa `push %ecx`).  /O2 pushing the register it just
//  proved to be zero is exactly what a source-level `pMgr->SetupError(...)`
//  inside `if (!pMgr)` compiles to, so that is what is written here; it is
//  a latent null dereference in the shipped source, not a misreading.
//
//  THE BIND IS A THISCALL ON THE MANAGER.  0x00424f20 ends `ret $8` and
//  0x00477be0 calls it with no caller cleanup and %ecx still holding the
//  argument-one load the null test used -- which is also why that test is
//  `test %ecx,%ecx` and the SetupError push is `push %ecx` rather than
//  `push $0`.  Written as a cdecl free function it was 369 bytes.
//
//  THE BIND GUARD IS POSITIVE.  Its failure block sits at 0x00477c97,
//  past the success return, and shares its print tail with the null-manager
//  arm: `if (bind) { ...; return LT_OK; }` then the error macro.  Written as
//  `if (!bind) RETURN_ERROR`, the two error tails stay separate.
//
//  0x004a1a74 is 0.5f, and the three
//  subtractions run before any of the three multiplications, which is
//  ltvector.h's `operator-` producing a temporary that `operator*(float)`
//  then consumes.  The result is passed BY VALUE: `sub $0xc,%esp` with the
//  three words written through %esp is a 12-byte struct argument.
// ======================================================================= //
class CWorldModelQ474
{
public:
	virtual LTBOOL HasDims() = 0;                              // slot 0x00

	uint8    m_pad_04[0xc4 - 0x04];
	LTVector m_vMin;                                           // 0xc4
	LTVector m_vMax;                                           // 0xd0
};


LTRESULT cm_WorldModelExtraInitQ474(CClientMgrQ474* pMgr,
                                    CClientModelInstQ474* pObject,
                                    CClientCreateRequestQ474* pRequest,
                                    LTRESULT dResult)
{
	CWorldModelQ474* pWorldModel;

	if (dResult)
		return LT_OK;

	if (!pMgr)
	{
		cm_SetupErrorQ474(pMgr, LT_MISSINGWORLDMODEL,
		                  pRequest->m_pStruct->m_Filename);

		RETURN_ERROR_SYM_DESC_Q474(1, "WorldModelExtraInit",
		                           LT_MISSINGWORLDMODEL,
		                           pRequest->m_pStruct->m_Filename);
	}

	if (pMgr->AttachWorldModel(pObject, pRequest->m_pStruct->m_Filename))
	{
		pWorldModel = (CWorldModelQ474*)pObject->m_pWorldModel;

		if (!pWorldModel->HasDims())
		{
			pObject->SetDims((((CWorldModelQ474*)pObject->m_pWorldModel)->m_vMax -
			                  ((CWorldModelQ474*)pObject->m_pWorldModel)->m_vMin)
			                 * 0.5f);
		}

		return LT_OK;
	}

	RETURN_ERROR_SYM_DESC_Q474(1, "WorldModelExtraInit",
	                           LT_MISSINGWORLDMODEL,
	                           pRequest->m_pStruct->m_Filename);
}


// ======================================================================= //
//  0x00477d90, 37 bytes -- the create-hook table's SLOT 1 dispatcher
//  (name ours).  The twin of 0x00477d60 (slot 0): same +0xbc signed type
//  byte (movsbl), same stride-8 table 0x004ae340, but it reads +4 and
//  forwards only (pClientMgr, pObject).  Its one caller, 0x00411c2b, pushes
//  exactly those two.  HOME: this TU -- it sits between the OT_CONTAINER
//  hook 0x00477d20 and counter.obj's 0x00477dc0, and server.dll has no twin
//  (0x10034ec6's predecessor is smgr.obj's tail).
// ======================================================================= //
class CClientHookObjQ474
{
public:
	uint8 m_pad_000[0xbc];
	char  m_ObjectType;                                        // 0xbc
};

typedef LTRESULT (*CreateHookFnQ474)(CClientMgrQ474* pMgr,
                                     CClientHookObjQ474* pObject,
                                     CClientCreateRequestQ474* pRequest,
                                     LTRESULT dResult);
typedef LTRESULT (*RemoveHookFnQ474)(CClientMgrQ474* pMgr,
                                     CClientHookObjQ474* pObject);

class CObjectHooksQ474
{
public:
	CreateHookFnQ474 m_fnCreate;                               // +0
	RemoveHookFnQ474 m_fnRemove;                               // +4
};

// 0x004ae340, 0x58 bytes of .data -- this TU's FIRST initialised datum (its
// string literals follow from 0x004ae398), one {create, remove} pair per
// object type, OT_NORMAL (0) .. OT_CANVAS (10).  Read off the image:
//   [1] model       0x004778a0 cm_InitModel / 0x00477b60 cm_TermModel
//   [2] world model 0x00477b80 cm_WorldModelExtraInit
//   [3] sprite      0x00477ce0 cm_SpriteExtraInit     (smgrnew.cpp)
//   [9] container   0x00477d20 cm_ContainerExtraInit  (smgrnew.cpp)
// every other slot zero.
LTRESULT cm_SpriteExtraInitQ474(CClientMgrQ474* pMgr,
                                CClientModelInstQ474* pObject,
                                CClientCreateRequestQ474* pRequest,
                                LTRESULT dResult);             // 0x00477ce0
LTRESULT cm_ContainerExtraInitQ474(CClientMgrQ474* pMgr,
                                   CClientModelInstQ474* pObject,
                                   CClientCreateRequestQ474* pRequest,
                                   LTRESULT dResult);          // 0x00477d20

CObjectHooksQ474 g_ObjectHooksQ474[11] =                       // 0x004ae340
{
	{ LTNULL, LTNULL },
	{ (CreateHookFnQ474)cm_InitModelQ474, (RemoveHookFnQ474)cm_TermModelQ474 },
	{ (CreateHookFnQ474)cm_WorldModelExtraInitQ474, LTNULL },
	{ (CreateHookFnQ474)cm_SpriteExtraInitQ474, LTNULL },
	{ LTNULL, LTNULL },
	{ LTNULL, LTNULL },
	{ LTNULL, LTNULL },
	{ LTNULL, LTNULL },
	{ LTNULL, LTNULL },
	{ (CreateHookFnQ474)cm_ContainerExtraInitQ474, LTNULL },
	{ LTNULL, LTNULL },
};

// 0x00477d60, 47 bytes -- the SLOT 0 (create) dispatcher (name ours).  Its
// caller is src/engine/lithtech/cchandler.cpp's 0x00420520 path, which
// pushes (pClientMgr, pObject, &request, LT_OK).
LTRESULT cm_RunCreateHookQ474(CClientMgrQ474* pMgr, CClientHookObjQ474* pObject,
                              CClientCreateRequestQ474* pRequest,
                              LTRESULT dResult)
{
	CreateHookFnQ474 fn;

	fn = g_ObjectHooksQ474[pObject->m_ObjectType].m_fnCreate;
	if (fn)
		return fn(pMgr, pObject, pRequest, dResult);

	return LT_OK;
}

LTRESULT cm_RunRemoveHookQ474(CClientMgrQ474* pMgr, CClientHookObjQ474* pObject)
{
	RemoveHookFnQ474 fn;

	fn = g_ObjectHooksQ474[pObject->m_ObjectType].m_fnRemove;
	if (fn)
		return fn(pMgr, pObject);

	return LT_OK;
}


// ======================================================================= //
//  0x00483a60, 507 bytes -- spr_Load: read a sprite out of an open stream
//  (name ours).
//
//  HOME: the client's sprite module (0x00483a60 .. 0x00483f30), which has
//  no TU under src/engine/ -- csvis.cpp and clientstreams.cpp both record
//  it as unwritten.  Written HERE because cm_LoadSprite (0x00476f42) is its
//  caller and this file already declares it; it is client-only (the
//  "Untitled" literal 0x004aea78 has no copy in server.dll).
//
//  One 0x14-byte sprite, its link pointing at itself, ONE 0x3c-byte
//  animation (the alloc pushes the constant, the memset re-reads m_nAnims
//  after the call), then per animation five dwords and per frame a
//  uint16-length texture name (refused over 1000, 0x00483bb3) resolved
//  through 0x0041ff90.  The stream's error status decides the result;
//  failure frees through 0x00483c60 (clientmgr2.cpp's sub_483c60).
// ======================================================================= //
void* dalloc(uint32 size);                                     // 0x00428fd0
void  spr_DestroyQ474(CSpriteQ474* pSprite);                   // 0x00483c60

CSpriteQ474* spr_LoadQ474(CClientMgrQ474* pMgr, ILTStreamQ474* pStream)
{
	CFileIdentQ474   ref;
	CSpriteQ474*     pSprite;
	CSpriteAnimQ474* pAnim;
	uint32           i, j;
	uint32           nFrames, msFrameRate, bKeyed, bTranslucent, colourKey;
	uint16           nameLen;
	char             szName[1024];

	pSprite = (CSpriteQ474*)dalloc(sizeof(CSpriteQ474));
	memset(pSprite, 0, sizeof(CSpriteQ474));
	pSprite->m_Link.m_pData = pSprite;
	pSprite->m_nAnims = 1;
	pSprite->m_Anims = (CSpriteAnimQ474*)dalloc(sizeof(CSpriteAnimQ474) * pSprite->m_nAnims);
	memset(pSprite->m_Anims, 0, sizeof(CSpriteAnimQ474) * pSprite->m_nAnims);

	for (i = 0; i < pSprite->m_nAnims; i++)
	{
		pAnim = &pSprite->m_Anims[i];

		pStream->Read(&nFrames, 4);
		pStream->Read(&msFrameRate, 4);
		pStream->Read(&bKeyed, 4);
		pStream->Read(&bTranslucent, 4);
		pStream->Read(&colourKey, 4);

		pAnim->m_Frames = (void**)dalloc(sizeof(void*) * nFrames);
		strncpy(pAnim->m_sName, "Untitled", 31);   // 0x00492b60 = CRT strncpy; no terminator store follows (the memset covers it)
		pAnim->m_nFrames      = nFrames;
		pAnim->m_MsFrameRate  = msFrameRate;
		pAnim->m_MsAnimLength = (1000 / msFrameRate) * nFrames;
		pAnim->m_bKeyed       = (uint8)bKeyed;
		pAnim->m_bTranslucent = (uint8)bTranslucent;
		pAnim->m_ColourKey    = colourKey;

		for (j = 0; j < nFrames; j++)
		{
			pStream->Read(&nameLen, 2);
			if (nameLen > 1000)
				goto Error;

			pStream->Read(szName, nameLen);
			szName[nameLen] = 0;

			ref.m_nFileType = 1;
			ref.m_pFilename = szName;
			pAnim->m_Frames[j] = cm_FindSharedTextureQ474(pMgr, &ref);
		}
	}

	if (pStream->ErrorStatus() != LT_OK)
		goto Error;

	return pSprite;

Error:
	spr_DestroyQ474(pSprite);
	return LTNULL;
}


// ======================================================================= //
//  A DIFFERENT MODULE: 0x00477fe0 .. 0x004780ae, 207 bytes
//
//  This one is NOT part of the `cm_` loader above.  Everything between it
//  and the loader is already reconstructed elsewhere -- our code matches
//  ?sub_477810@@YAIPAXPAVCKeyedListOwnerView@@@Z at 0x00477810,
//  ?sub_10034ec6@@YAHPAD0PAPAXHPAH@Z at 0x00477dc0,
//  ?cb_FreeSoundTable@@YAXPAX@Z at 0x00477ef0 and
//  ?sub_10034f9b@@YAXPAVCModuleFuncs@@PAPAX1@Z at 0x00477f10, and
//  the decoded symbol map puts localdriver.obj's
//  CLocalDriverBase::GetSessionName at 0x00477f4c.  So the run this sits in
//  is shared server/lithtech code, and this function is a TABLE FILL for the
//  block of functions that FOLLOWS it (0x004780b0 onward).
//
//  WHAT IT IS, EXACTLY.  A 0x400-byte table at 0x004b48d0 is zeroed with the
//  inline-memset idiom (`mov $0x100,%ecx; xor %eax,%eax; mov $0x4b48d0,%edi;
//  rep stos`), and then nineteen dword slots take the addresses of nineteen
//  functions, all of them inside 0x004780b0..0x0047b700 -- the very next run
//  of code.  The slot indices are the displacements divided by four and
//  NOTHING here names them: the entries are reported at the offsets the
//  stores use, in the order the stores happen, because that order IS the
//  source's statement order and is the only thing this reconstruction can
//  be right or wrong about.
//
//  THE PROMPT'S WARNING APPLIES HERE.  A dispatch-table fill is exactly the
//  shape that got mis-named "sm_ResetProfileCounters" earlier in this
//  project, so the name below says only what the body does.
// ======================================================================= //

// 0x004b48d0 -- 0x100 dwords of .bss (name ours).  DEFINED here: the fill
// below is its only writer, and its neighbours in the image (smgr.obj's
// 0x004b48bc..0x004b48cc before it, streamsim.obj's 0x004b4cd0 after) put it
// in the .bss of the module this fill heads.  clientshell.cpp's dispatch
// reads it under the same name.
void* g_Table4b48d0Q474[0x100];

// The nineteen it installs, by address.  Declared, never defined.
void sub_4780b0Q474(); void sub_4781d0Q474(); void sub_478300Q474();
void sub_478510Q474(); void sub_47a1f0Q474(); void sub_47a480Q474();
void sub_478320Q474(); void sub_47a4e0Q474(); void sub_47a790Q474();
void sub_47a7d0Q474(); void sub_47a970Q474(); void sub_47ac40Q474();
void sub_47ac80Q474(); void sub_47ad20Q474(); void sub_47af20Q474();
void sub_47b2b0Q474(); void sub_47b380Q474(); void sub_47b520Q474();
void sub_47b700Q474();

void cm_InitTable4b48d0Q474()
{
	memset(g_Table4b48d0Q474, 0, sizeof(g_Table4b48d0Q474));

	g_Table4b48d0Q474[0x30 / 4] = (void*)sub_4780b0Q474;   // 0x004b4900
	g_Table4b48d0Q474[0x18 / 4] = (void*)sub_4781d0Q474;   // 0x004b48e8
	g_Table4b48d0Q474[0x14 / 4] = (void*)sub_478300Q474;   // 0x004b48e4
	g_Table4b48d0Q474[0x20 / 4] = (void*)sub_478510Q474;   // 0x004b48f0
	g_Table4b48d0Q474[0x28 / 4] = (void*)sub_47a1f0Q474;   // 0x004b48f8
	g_Table4b48d0Q474[0x34 / 4] = (void*)sub_47a480Q474;   // 0x004b4904
	g_Table4b48d0Q474[0x38 / 4] = (void*)sub_478320Q474;   // 0x004b4908
	g_Table4b48d0Q474[0x3c / 4] = (void*)sub_47a4e0Q474;   // 0x004b490c
	g_Table4b48d0Q474[0x40 / 4] = (void*)sub_47a790Q474;   // 0x004b4910
	g_Table4b48d0Q474[0x44 / 4] = (void*)sub_47a7d0Q474;   // 0x004b4914
	g_Table4b48d0Q474[0x68 / 4] = (void*)sub_47a970Q474;   // 0x004b4938
	g_Table4b48d0Q474[0x48 / 4] = (void*)sub_47ac40Q474;   // 0x004b4918
	g_Table4b48d0Q474[0x1c / 4] = (void*)sub_47ac80Q474;   // 0x004b48ec
	g_Table4b48d0Q474[0x4c / 4] = (void*)sub_47ad20Q474;   // 0x004b491c
	g_Table4b48d0Q474[0x54 / 4] = (void*)sub_47af20Q474;   // 0x004b4924
	g_Table4b48d0Q474[0x10 / 4] = (void*)sub_47b2b0Q474;   // 0x004b48e0
	g_Table4b48d0Q474[0x5c / 4] = (void*)sub_47b380Q474;   // 0x004b492c
	g_Table4b48d0Q474[0x60 / 4] = (void*)sub_47b520Q474;   // 0x004b4930
	g_Table4b48d0Q474[0x64 / 4] = (void*)sub_47b700Q474;   // 0x004b4934
}


// ======================================================================= //
//  THE SHIPPED CALL SEQUENCES
//
//  This TU was written without a build (the build slot was not ours), so
//  the call sequences could not be compared against a compiled object.  What
//  follows is the shipped side of that comparison, read straight out of
//  the retail lithtech.exe in address order:
//
//  Counts first:
//
//    0x00476e80..0x00477011   401 B   15 calls (14 direct, 1 indirect)
//    0x00477020..0x004770d7   183 B    6 calls ( 6 direct, 0 indirect)
//    0x004770e0..0x00477172   146 B    4 calls ( 4 direct, 0 indirect)
//    0x00477180..0x00477431   689 B   24 calls (20 direct, 4 indirect)
//    0x00477440..0x004776ed   685 B   25 calls (20 direct, 5 indirect)
//    0x004776f0..0x00477809   281 B    6 calls ( 5 direct, 1 indirect)
//    0x004778a0..0x00477a1f   383 B    4 calls ( 4 direct, 0 indirect)
//    0x00477a20..0x00477b52   306 B    7 calls ( 7 direct, 0 indirect)
//    0x00477b80..0x00477cd6   342 B    7 calls ( 6 direct, 1 indirect)
//    0x00477fe0..0x004780af   207 B    0 calls  -- the check is VACUOUS here
//
//  And the sequences themselves:
//
//  cm_LoadSprite            0x4045d0, 0x404680, 0x4048c0, 0x41fc60,
//                           0x42d760, 0x4045d0, 0x42e6f0, 0x483a60,
//                           0x41fc60, 0x42d760, 0x42e6f0, *0x4(vtbl),
//                           0x41fc60, 0x42d760, 0x42e6f0
//  cm_LoadModel             0x42e6f0, 0x47bef0, 0x4441a0, 0x4442c0,
//                           0x477440, 0x4776f0
//  cm_LoadModel2            0x404680, 0x477020, 0x42d760, 0x42e6f0
//  cm_LoadChildModel        0x490e50, 0x4929c4, 0x4929c4, 0x404680,
//                           0x42d760, 0x42e6f0, 0x4290b0, 0x446360,
//                           0x42d760, 0x42e6f0, 0x404890, *(vtbl),
//                           0x42d760, 0x42e6f0, 0x44e550, 0x44ddc0,
//                           *0x4(vtbl), 0x4476c0, 0x4776f0, *(vtbl),
//                           0x42d760, *(vtbl), 0x42d760, 0x42e6f0
//  cm_LoadModelData         0x4290b0, 0x446360, 0x42d760, 0x42e6f0,
//                           0x404890, *(vtbl), 0x41fc60, 0x42d760,
//                           0x42e6f0, 0x44e550, 0x44ddc0, *0x4(vtbl),
//                           0x4476c0, *(vtbl), 0x42e6f0, 0x42d760,
//                           0x42e6f0, *(vtbl), 0x42e6f0, 0x42d760,
//                           0x42e6f0, 0x43c500, 0x43c580, *(vtbl),
//                           0x42e6f0
//  cm_RegisterModel         0x492c60 (__chkstk), *(vtbl), 0x41b5d0,
//                           0x41ff90, 0x43c500, 0x43c580
//  cm_InitModel             0x4770e0, 0x401050, 0x401050, 0x477a20
//  cm_InitSkin              0x4045d0, 0x42ee70, 0x41ff50, 0x42ee70,
//                           0x476e80, 0x483cc0, 0x41ff50
//  cm_WorldModelExtraInit   0x41fc60, 0x42d760, 0x424f20, *(vtbl),
//                           0x45aba0, 0x42d760, 0x42e6f0
//
//  Two of these are worth reading rather than just diffing.  cm_LoadSprite
//  issues 0x41fc60/0x42d760 THREE times but 0x42e6f0 only three times as
//  well, because the missing-FILE arm cross-jumps into the missing-RECORD
//  arm's printf (0x00476f17 `jmp 0x476fee`); three separate expansions in
//  the source is what produces that.  And cm_LoadChildModel issues 0x4929c4
//  TWICE -- both sprintf arms are present, so the `if`/`else` really is two
//  calls and not one with a selected format string.
// ======================================================================= //
