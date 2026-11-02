// *********************************************************************** //
//  MODULE:   SERVERENGINE (.H)
//  PURPOSE:  The se_ family at 0x1002af75-0x1002bab8: giving an object the
//            file it is made of. Every one of them is named by its own error
//            tag — se_InitModel, se_InitSprite, se_InitWorldModel,
//            se_LoadModel, se_LoadModelData, se_LoadChildModel — so the names
//            here are the engine's, not ours.
//
//            The shape is the same throughout: ask the file manager for the
//            file the create struct names, and where there is a sensible
//            stand-in (a default model, a default sprite) fall back to it
//            with a warning rather than failing outright.
// *********************************************************************** //
#ifndef __SERVERENGINE_H__
#define __SERVERENGINE_H__

#include "servermgr.h"
#include "ltobject_internal.h"
#include "de_error.h"
#include "commonmodel.h"

// What the model reader is handed: the stream to read, a callback for the
// child models it names, and whatever the callback wants back. The two
// counts at the end are what it reports about the child models it found
// (names ours).
class CModelLoadRequest
{
public:
	CModelLoadRequest();                     // 0x1001cb33

	class ILTStream* m_pStream;              // 0x00
	LTRESULT (*m_pfnLoadChild)(CModelLoadRequest* pReq,
	                          class Model** ppModel);   // 0x04
	class CModelLoadContext* m_pContext;     // 0x08
	// The child the reader is asking for, as the file named it — a bare
	// name, resolved against the parent's own directory.
	char*  m_pChildName;                     // 0x0c
	// Four flags, and the constructor sets every one of them to 1: the
	// reader clears the ones it cannot satisfy, so a request that comes
	// back with a zero in it is a report of what went wrong. Only two are
	// read anywhere we can see, and those two are named by the errors
	// se_LoadModelData raises off them.
	LTBOOL m_bUnknown10;                     // 0x10
	LTBOOL m_bTreesValid;                    // 0x14
	LTBOOL m_bUnknown18;                     // 0x18
	LTBOOL m_bChildrenFound;                 // 0x1c
};

// 0x10016300 — the child-model callback a freshly constructed request starts
// with: `push $0x46; pop eax; ret`, i.e. `return 70;` (LT_NOCHANGE in the
// SDK header we hold) and nothing else. Three bytes, so the image's single
// copy is shared with every other stub of that shape; only its address is
// evidence here, and the diff masks it. Not defined — a declaration is all
// the constructor needs (name ours).
LTRESULT se_NoChildModel(CModelLoadRequest* pReq, class Model** ppModel);

// The constructor, in the HEADER and not in serverengine.cpp, and the
// difference is visible in both images. It is `inline`, so /Ob1 may expand
// it -- but only where the definition has already been SEEN, and a body in
// serverengine.cpp is invisible to commonmodel.cpp. lithtech.exe (/O2)
// carries no standalone copy anywhere: the eight stores appear in line at
// 0x0046aac0 inside se_LoadModelData and again at 0x0044ddd7 inside
// Model::LoadChildModels, in this source's exact statement order, with
// `xor %edi,%edi` and `mov $1,%ebp` holding the two constants across them.
// The second of those is a commonmodel.cpp call site, so the shipped build
// had the body in a header. server.dll (/O1 = /Os) declines the expansion at
// both sites and calls out of line (0x1002b238 and 0x1001c615), which is
// what a size-tuned build does with a caller the expansion grows.
// Defined below se_NoChildModel because the body names it, and above every
// caller because VC6's inliner is forward-only.
inline CModelLoadRequest::CModelLoadRequest()
{
	m_pChildName     = LTNULL;
	m_pStream        = LTNULL;
	m_pfnLoadChild   = se_NoChildModel;
	m_pContext       = LTNULL;
	m_bUnknown10     = LTTRUE;
	m_bTreesValid    = LTTRUE;
	m_bUnknown18     = LTTRUE;
	m_bChildrenFound = LTTRUE;
}

// What the callback is given back: who is asking, and for which model.
class CModelLoadContext
{
public:
	CServerMgr* m_pServerMgr;                // 0x00
	char*       m_pFilename;                 // 0x04
};

// What a file record's +0x10 points at: the record of the mounted tree the
// file came out of. Only one field is reached from here, and the same field
// is reached the same way from fs_OpenFile's tree walk (0x1002bd37), where
// the pointer comes off the manager's tree list instead — which is what says
// the two are the same kind of record (names ours).
class CFileTreeRec
{
public:
	uint8            m_pad_00[0x0c];
	class CFileTree* m_pTree;                // 0x0c
};

// 0x100070e3 — open a stream on a named file inside a mounted tree. It picks
// the stream kind from the tree's own type word: a directory on disk gets a
// CTreeStream out of g_TreeStreamBank and prints "stream %p open file %s
// size = %u", a .rez member gets a CMemStream out of g_MemStreamBank and
// prints "stream %p open rez %s size = %u". Lives in another file's run
// (name ours).
class ILTStream* fs_OpenStreamInTree(class CFileTree* pTree, char* pFilename,
                                     int nFlag);

// 0x1002bd75 — a stream over an already-open file (name ours).
class ILTStream* se_OpenStream(class CFileMgr* pFileMgr, void* pFile);

// 0x100074df — drain a stream into a named file on disk. Opens the file with
// the mode string at 0x100518a0, seeks the stream back to zero, asks it its
// length and copies it in 0x4000-byte blocks out of a temporary allocation,
// answering 1 only when every byte arrived (its own `sete` compares the
// running offset against the length). Lives in another file's run; declared
// here because fs_CopyFile is the only caller reconstructed so far (name
// ours).
int fs_CopyStreamToFile(class ILTStream* pStream, const char* pszDestFile);

// 0x1002bd97 — copy a file out of whichever mounted tree has it. The signature
// is servermgr.h's.
LTRESULT fs_CopyFile(class CFileMgr* pFileMgr, const char* pszSourceFile,
                     const char* pszDestFile);

// 0x1002b41b — announce a model that just finished loading and put it in the
// manager's hash. The last argument is a flag: se_LoadChildModel passes zero
// and CServerMgr::OnModelLoaded passes one (name ours).
LTRESULT se_RegisterModel(CServerMgr* pMgr, class Model* pModel, void* pFile,
                          int nFlag);

// A model keeps a list of things to run when it goes, and the static record
// at 0x10054020 is the one that takes it back out of the manager's hash.
//
// 0x10054020 — the record: a callback, a pointer to a second record of the
// same shape at 0x10052df8 (whose own callback is 0x1001401d), and two zero
// words. Nothing in .text reads the record — the only reference to it in the
// whole image is the dl_MakeLink call in se_LoadChildModel — so the runner
// that eventually calls the callback is not in the reconstructed region and
// the fields past the callback have no evidence behind their names.
//
// The callback's third argument is the gate (0x1002af75 does nothing unless
// it is zero) and its first is never read. 0x1001401d, reached the same way
// off the other record, reads a *fourth* argument, so the shared type is
// probably four wide with this one ignoring the last; both spellings compile
// to the same bytes here, so the three the code actually reads are what is
// declared.
class CDepRecord
{
public:
	LTRESULT (*m_pfnRun)(void* pArg1, class CDepLink* pLink, int nArg3);  // 0x00
	void*  m_pUnknown04;                     // 0x04
	uint32 m_Unknown08;                      // 0x08
	uint32 m_Unknown0c;                      // 0x0c
};

extern CDepRecord g_ModelHashCleanup;

// The eight bytes the model-hash cleanup link carries, `new`ed in
// se_LoadChildModel: who loaded the model, and which model. The manager
// pointer is written and never read — the callback goes to the global
// g_pServerMgr instead (names ours).
class CModelCleanupRec
{
public:
	// `new`ed at 0x1002b19a through the `operator new` forwarder
	// (0x1000d25d), and the null test and `xor %eax,%eax` around the two
	// stores are the new-expression's own: this constructor expanded in.
	CModelCleanupRec(CServerMgr* pServerMgr, class Model* pModel)
	{
		m_pServerMgr = pServerMgr;
		m_pModel     = pModel;
	}

	CServerMgr*  m_pServerMgr;               // 0x00
	class Model* m_pModel;                   // 0x04
};

// 0x1002af75 — the callback in that record (name ours).
LTRESULT se_ModelHashCleanup(void* pArg1, class CDepLink* pLink, int nArg3);

// One link: what to run, what to hand it, and the next one. Twelve bytes,
// and the bank's own initialiser says so — 0x10013fab calls sb_Init2 with a
// record size of 0xc (names ours).
class CDepLink
{
public:
	void*     m_pRecord;                     // 0x00
	void*     m_pData;                       // 0x04
	CDepLink* m_pNext;                       // 0x08
};

void* dl_MakeLink(void* pRecord, void* pData);      // 0x1001404e
// The name is read off the bytes and disagrees with the obvious guess: the
// new link's next is set to whatever the list's single pointer held and the
// pointer is then aimed at the new link, so this **prepends**. There is no
// tail pointer anywhere in the twelve-byte record for it to append to.
LTRESULT dl_AddLink(void* pList, void* pLink);      // 0x1001406a

// 0x1000d2ef — the engine's raw allocate. NOT 0x1000d25d, which this used
// to say: that address is the eleven-byte `operator new` forwarder in front
// of it (`push 0x4(%esp); call 0x1000d2ef; pop ecx; ret`). The two are
// indistinguishable to a masked comparison and only the call
// targets separate them.
void* dalloc(uint32 nBytes);

// 0x10058c7c is the collection allocator — collection.h's g_DefAlloc
// (stdlith's l_allocator.cpp defines it); a model loaded from file is built
// with it on both sides.

// 0x1002afcf — the callback the reader uses for each child model a file
// names (name ours).
LTRESULT se_LoadChildModel(CModelLoadRequest* pReq, class Model** ppModel);

// 0x1002b22c — turn an open model file into a Model (name ours).
LTRESULT se_LoadModelData(CServerMgr* pMgr, char* pFilename, void* pFile,
                          class Model** ppModel);

// 0x10032dcd — the manager's own hook for a model that finished loading here
// rather than on the worker thread (name ours).

// 0x1002be8c — load a sprite through the file manager, into the caller's
// pointer. Nonzero means it came back. What it fills is the same slot of the
// object data a model's file goes in — an object is made of one file and the
// slot does not care which kind (name ours).
int se_LoadFile(class CFileMgr* pFileMgr, char* pFilename, uint16 nFlags,
                void** ppFile);

// A sprite object keeps the frame it is showing past the end of LTObject; an
// uninitialised one is all-ones rather than zero, so "no frame yet" and
// "frame zero" stay apart (names ours).
class SpriteInstance : public LTObject
{
public:
	uint8  m_pad_1b4[0x1bc - 0x1b4];
	uint32 m_nCurFrame;                      // 0x1bc
};

// What the object data's file slot points at. Every field is pinned by
// se_LoadFile, which is the only thing that ever fills one in: the record is
// 0x14 bytes (CFileMgr's constructor sb_Init's the bank for 0x14) and
// se_LoadFile writes all five of them (names ours).
class CFileRef
{
public:
	// The record's own hash element. CServerMgr::OnModelLoaded gets the
	// file's name back out of it with dhash_GetKey, which is what says the
	// first word is an element handle and that the key it was filed under is
	// the filename.
	void*  m_hElement;                       // 0x00
	// The one dword the tree lookup hands back about the file, out of the
	// 0x110-byte record 0x10006fb4 filled (at its +0x108). What it means is
	// not visible from anything reconstructed so far.
	uint32 m_Unknown04;                      // 0x04
	// The manager's running file count at the moment this record was made,
	// so: a per-file serial number, not a flags word. se_LoadModelData
	// copies it into the model, where commonmodel.h calls the destination
	// m_FileFlags — that name was read off the model side before this
	// evidence existed, and commonmodel.h is not a file this reconstruction
	// owns.
	uint32 m_nFileID;                        // 0x08
	// se_LoadFile stores its nFlags argument here as a *word*, so the field
	// is at least sixteen bits wide; se_InitModel and se_RegisterModel then
	// set and clear bit 0 through it with byte-wide instructions, which is
	// only VC6 narrowing the operation to the immediate.
	uint16 m_Flags;                          // 0x0c  bit 0 = still loading
	uint8  m_pad_0e[0x10 - 0x0e];
	CFileTreeRec* m_pTreeRec;                // 0x10
};

// A model instance holds the model it was made from, and the model counts
// the instances that hold it — commonmodel.h has Model, including the count
// at 0x208 (name ours).
class ModelInstanceObj : public LTObject
{
public:
	uint8  m_pad_1b4[0x1dc - 0x1b4];
	class Model* m_pModel;                   // 0x1dc
};

LTRESULT se_InitWorldModel(CServerMgr* pMgr, LTObject* pObj,
                           ObjectCreateStruct* pStruct);   // 0x1002b92e

// 0x1002b4dc — a model by name: the one already loaded if the hash has it,
// otherwise the file, loaded here or queued for the worker thread.
LTRESULT se_LoadModel(CServerMgr* pMgr, char* pFilename, Model** ppModel,
                      void** ppFile, int bWantLoad, int bLoadHere);

#endif  // __SERVERENGINE_H__
