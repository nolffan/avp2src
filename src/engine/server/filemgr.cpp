// server.dll — the file manager (reconstruction). Build /O1.
//
// The manager owns the list of open resource trees and a filename hash over
// what is inside them. Its records are handed out from a struct bank rather
// than allocated one at a time, and the filenames themselves live in the
// string bank, which is why a file identifier is just a handle plus the tree
// it came from.
//
// The file name is ours.
#include <string.h>
#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine sees the STL's <string> here, through a header
// AvP2's does not have: the translation unit carries <string>'s 44-byte
// guarded initialiser of std::ctype<unsigned short>::id (guard 0x004f8014,
// atexit of the empty 0x00401790; this one's near 0x00480250). server.dll has it
// in collection/objbank only.
#include <string>
#endif
#include <stdio.h>
#include <io.h>
#include "servermgr.h"

// One entry in the manager's open-tree list. The link's data points back at
// the record, so the walks hand the record straight out. Not the CFileTree
// in servermgr.h — that one is what fs_AddResources returns to the world;
// this is the manager's own bookkeeping node (names ours).
class CFileTreeNode
{
public:
	LTLink m_Link;                           // 0x00
	void*  m_pTree;                          // 0x0c  what the archive layer owns
};

// 0x10056ff4 — what an unnamed file reports (name ours).
extern char g_sNoFileName[];

// The archive layer and the rest of the manager, declared as reached (names
// ours). fs_FlushFiles drops every cached file; fs_OpenFileEx is the full
// form of the open; fs_BuildFileList turns a set of open trees into the
// FileEntry chain the interface hands out.
void       fs_CloseTree(void* pTree);                                       // 0x10006f60
void       fs_FlushFiles(CFileMgr* pFileMgr);                               // 0x1002bf82
ILTStream* fs_OpenFileEx(CFileMgr* pFileMgr, char* pFilename,
                         int bRegister, int nType);                         // 0x1002bcee
FileEntry* fs_BuildFileList(void** pTrees, int nTrees, char* pDirName);     // 0x10013741

// 0x1003d1f0 — hand a struct bank's chunks back; the second argument is
// accepted and ignored (name ours).
void sb_FreeChunks(StructBank* pBank, int bAll);

// 0x1000d374 — dalloc's cleared form (dalloc.cpp).
void* dalloc_z(uint32 nSize);

// 0x1000ddfb — a string out of the bank by handle.


// The archive layer proper (names ours; all cdecl). 0x100070e3 opens one file
// out of one open tree, and 0x10007203 walks a directory inside one: it is a
// find-first and a find-next in one function, told apart by the cursor the
// caller leaves in the state record.
ILTStream* ar_OpenFile(void* pTree, char* pFilename, int nFlag);            // 0x100070e3
LTBOOL     ar_FindNext(void* pTree, char* pDirName,
                       class CFileFindState* pState);                       // 0x10007203
                                                                            // (defined below)

// The state ar_FindNext fills and steps. Its size is not guessed: the record
// runs from the frame slot the caller passes (-0x110) to the cursor it clears
// per tree (-0x4), so the cursor is at +0x10c and the whole thing is 0x110
// bytes. 0x10007203 reaches +0x10c itself, which is the confirmation — and
// serverengine.h calls the same thing "the 0x110-byte record 0x10006fb4
// filled", naming a field at its +0x108 that this walk does not touch
// (names ours).
class CFileFindState
{
public:
	int    m_Type;                           // 0x000  0 = a directory
	char   m_Name[0x100];                    // 0x004
	uint32 m_Time;                           // 0x104  the find data's write time
	                                         //        for a file on disk, and one
	                                         //        of the archive record's own
	                                         //        words for a rez member
	uint32 m_nSize;                          // 0x108
#ifdef PRIMAL_HUNT
	// PH: 0x104 more bytes (MAX_PATH), which no walk reconstructed so far
	// reads: every PH frame holding one is 0x214 where AvP2's is 0x110
	// (fs_FindFileInTrees, 0x10037836; fs_GetFileInfo, 0x10037780). They
	// sit BEFORE the cursor: PH's ar_FindNext reads and clears it at +0x210
	// (lithtech.exe 0x00429d7b, 0x00429e27).
	char   m_Unknown10c[0x104];              // 0x10c
#endif
	void*  m_pCursor;                        // 0x10c (PH 0x210)  null starts
	                                         //        a new walk
};

// 0x10013870 — is this name already on the list being built? Compares against
// each entry's base name, case-insensitively (name ours).
LTBOOL fs_FindFileEntry(FileEntry* pList, char* pName)
{
	while (pList)
	{
		if (stricmp(pList->m_pBaseFilename, pName) == 0)
			return LTTRUE;

		pList = pList->m_pNext;
	}

	return LTFALSE;
}

// 0x1003b370 / 0x1003b470 — the open archive a tree record owns. Only its
// destruction is reached from here; fs_OpenTree (0x10006ec0) allocates 0xd4
// bytes for one and constructs it in place. The destructor is non-virtual
// (the delete calls it directly) even though the class has a vptr (name ours).
class CFileArchive
{
public:
	CFileArchive();                          // 0x1003b370
	~CFileArchive();                         // 0x1003b470

	// 0x1003b5b0 — open the archive file. The second and third arguments are
	// fixed at 1 and 0 by the only caller, so what they select is not
	// visible from here. 0x1003c0f0 hands it the set of characters that
	// separate one directory from the next inside the archive (names ours).
	LTBOOL Open(char* pName, int nFlag, int n);
	void   SetDelimiters(char* pDelimiters);

	// 0x1003c4c0 — the directory record for a path inside the archive, and
	// 0x1003c050, which spells an item's type as the file extension. These
	// are RezMgr.lib's CRezMgr::GetDirFromPath and CRezMgr::TypeToStr (the
	// library's reconstruction carries them as CRezMgrX380; the link pairs
	// the two spellings in link/aliases.rsp by address).
	class CRezDir* GetDirFromPath(const char* pPath);
	void           TypeToStr(uint32 nType, char* sType);

	// 0x1003c4a0 — one named item anywhere in the archive: RezMgr.lib's
	// CRezMgr::GetRezFromDosPath. fsstream.cpp declares the same entry point
	// on a view of its own, because the two files want it to answer with two
	// different element types.
	class CRezItm* Find(char* pFilename);

	// 0xd4 is the size the open asks the allocator for, and it is the class's
	// own size and not a literal: it reaches the allocator as `push $0xd4`
	// out of a `new`, which is five bytes where a literal 1 would be two.
	uint8 m_pad_00[0xd4];
};

// The record fs_OpenTree builds and fs_CloseTree tears down: this is
// servermgr.h's CFileTree (whose m_pList at +0x04 is the archive below) with
// the critical section that serializes reads of it spelled out, which is what
// the close needs in order to delete it. Not serverengine.h's CFileTreeRec —
// that is the manager's list *node*, which points at one of these (name ours).
class COpenTree
{
public:
	int              m_nKind;                // +0x00  1 = a plain directory
	CFileArchive*    m_pArchive;             // +0x04  null for a directory
	CRITICAL_SECTION m_CS;                   // +0x08  (windows.h reaches this
	                                         //        file through lthread.h)
	char             m_szDir[1];             // +0x20  inline, not a pointer;
	                                         //        sizeof is 0x24 with the
	                                         //        padding, which is the
	                                         //        constant the open adds
	                                         //        to the name's length
};

// 0x10058358 and 0x100582e0 — the archive layer's own two globals: the open
// sets them to zero on success and the close zeroes them again, both times
// outside anything conditional on the record's contents (names ours).
void* g_pUnknown58358;
void* g_pUnknown582e0;

// 0x10006e57 — the archive layer's whole start-up: zero the two globals and
// nothing else. Not reference counted, unlike every other module the system
// layer brings up either side of it, so a second call resets the layer under
// whatever was using it.
void fs_InitModule()
{
	g_pUnknown58358 = LTNULL;
	g_pUnknown582e0 = LTNULL;
}

// 0x10036d21 — and its shutdown, which the system layer calls (dsi_Term,
// 0x1000e018) and which is empty in release: the call lands on the image's
// one bare `ret`, every empty function folded there by /OPT:ICF.
void fs_TermModule()
{
}

// One record out of the manager's bank (0x14 bytes, the size fs_Init gives
// the bank), of which the open reaches only the tree node the file came from;
// the hash entry's data pointer is the record itself. serverengine.h models
// the whole of it as CFileRef, and this is the same record — its m_pTreeRec is
// the field below. (Not servermgr.h's CFileRec, which is the per-tree announce
// record the handshake walks; names ours.)
class CFileBankRec
{
public:
	uint8           m_pad_00[0x10];
	CFileTreeNode*  m_pTreeNode;             // +0x10
};



// The rez archive's own directory tree, as far as the walk below needs it.
// These are RezMgr.lib's CRezItm/CRezDir (src/LT2/lithshared/incs/rezmgr.h's
// method names; the library's reconstruction carries the classes suffixed,
// CRezItmRz48 and CRezDirQ48C, and link/aliases.rsp pairs the spellings by
// address). What the three cursors are is visible from how the walk uses
// them -- one runs over a directory's item *types*, one over the items of the
// type it is on, and one over its sub-directories, and a file's name is the
// item's name with the type appended as an extension.
class CRezTyp;
class CRezDir;

class CRezItm
{
public:
	uint32 GetType();                        // 0x1003a640

	char*  m_pName;                          // +0x00
	uint8  m_pad_04[0x08 - 0x04];
	uint32 m_Time;                           // +0x08
	uint32 m_nSize;                          // +0x0c
};

class CRezDir
{
public:
	CRezTyp* GetFirstType();                 // 0x1003acc0
	CRezItm* GetFirstItem(CRezTyp* pTyp);    // 0x1003ad00
	CRezDir* GetFirstSubDir();               // 0x1003ac80
	CRezItm* GetNextItem(CRezItm* pItm);     // 0x1003ad20
	CRezTyp* GetNextType(CRezTyp* pTyp);     // 0x1003ace0
	CRezDir* GetNextSubDir(CRezDir* pDir);   // 0x1003aca0

	char*  m_pName;                          // +0x00
	uint8  m_pad_04[0x14 - 0x04];
	uint32 m_Time;                           // +0x14
};

// What the cursor in the find state points at: one walk in progress. The
// 0x130 bytes are the tree it is walking, a whole find-data block for the
// on-disk case, the find handle, and the three archive cursors for the rez
// case -- the two cases never both apply, so the record carries both sets
// (names ours).
class CFileWalk
{
public:
	COpenTree*         m_pTree;              // +0x000
	struct _finddata_t m_Find;               // +0x004
	long               m_hFind;              // +0x11c
	CRezDir*           m_pDir;               // +0x120
	CRezTyp*           m_pTyp;               // +0x124
	CRezItm*           m_pItm;               // +0x128
	CRezDir*           m_pSubDir;            // +0x12c
};


// 0x10007203 — one entry out of one directory inside one tree: a find-first
// and a find-next in the same function, told apart by whether the caller's
// state already holds a cursor.
//
// A directory tree is walked with the CRT's own find, over "<dir>\<sub>\*.*",
// and every name beginning with a dot is skipped -- which drops "." and ".."
// and any dot-file with them. The entry type is the find attribute's
// directory bit, inverted: `~attrib >> 4 & 1`, so 1 is a file and 0 is a
// directory, and the 1 that the `& 1` needs in a register is the same 1 the
// function returns.
//
// A .rez tree is walked through the archive's own tree instead, and the three
// cursors advance in order: the items of the current type, then the next type
// (whose first item is taken), and only when the types run out the
// sub-directories. A rez item's name has no extension, so the name handed back
// is the item's name, a dot, and the type.
//
// Running out frees the walk record and clears the cursor either way, so the
// caller can walk the same state again. The archive path frees it through the
// state rather than through the local, which is the same pointer.
//
// Shape, and what each part of it was measured to control:
//
//   * The whole body sits under a positive `if (pRec)`, the kind test is an
//     if/else and the function ends in one `return 0`. That, together with
//     the loop below, puts the shared `xor %eax,%eax` and the epilogue
//     straight after the on-disk exhaustion (0x100072b8), ahead of the found
//     block, as shipped; the old early-refusal shape with the `goto` loop
//     left them at the end of the function. (Whether the exhaustion then
//     says `return 0` itself or breaks out to the bottom one makes no
//     difference on either image.)
//   * The cursor is read inside each arm: lithtech.exe loads it separately
//     after the kind test (0x004212fb, 0x004213f7).
//   * The on-disk walk is the same find idiom as ar_FindFile: first/next
//     seeds nRes on the two arms of an if/else and the loop tests it at the
//     top, exhaustion first, then the dot test around the found block. /O1
//     gives `je` into the cleanup, `jne` to the found block and a `jmp` back
//     to the lone _findnext (0x1000728b-0x10007296); /O2 rotates it with the
//     _findnext duplicated at the bottom (0x0042136d), as shipped.
//   * The rez tail is `if (item) {...} else { if (!sub) return 0; ... }` with
//     one `return 1` after it: the item block then keeps its own `movl $1`
//     and jumps to the sub-directory block's `push $1; pop %eax`
//     (0x10007405 -> 0x100074d7).
//   * Both tails assign the time FIRST. /O1 still stores it last, but loads
//     it before the other stores, as shipped (0x100073e7, 0x100074c4);
//     written last, the loads come after the constant stores.
//
// NEAR on server.dll: 730 of 732 bytes, 211 instructions against 212, and
// the whole residual is one instruction. In the sub-directory tail the
// shipped build spends six bytes on `add $0x12c,%esi` (0x100074a1) so the
// re-read of pWalk->m_pSubDir after the strncpy is a two-byte `mov
// (%esi),%eax`; ours re-reads at the displacement. A `CRezDir**` local at
// block and at function scope, and a `CRezDir*&`, all copy-propagate back to
// the displacement form.
//
// The rez exhaustion is written out TWICE -- once where the types run out
// and the first sub-directory is fetched, once where the next sub-directory
// is -- rather than one copy reached by a goto. lithtech.exe says so: each
// fetch has its own test and store (0x004215c5, 0x00421607) and the second
// exhaustion is `push cursor; jmp` into the first one's dfree (0x0042161c),
// the cross-jump VC6 makes of two identical tails. server.dll merges the two
// either way. NEAR on lithtech.exe (916 bytes, 929 here, same block layout):
// what remains is allocation -- shipped keeps pState in ebp from before the
// kind test (ours: ebx in the disk arm), and holds zero in esi across the rez
// tail, which is where the 13 bytes go (`mov %esi,mem` for the three zero
// stores against `movl $0`). A one-case switch on the kind, an early
// `if (!pRec) return 0`, the disk exhaustion returning directly, and the rez
// exhaustions jumping to the final `return 0` were each measured and worse
// or no change.
LTBOOL ar_FindNext(void* pTree, char* pDirName, CFileFindState* pState)
{
	CRezTyp*    pTyp;
	char        szExt[8];
	char        szPath[400];
	COpenTree*  pRec = (COpenTree*)pTree;
	CFileWalk*  pWalk;
	int         nRes;

	if (pRec)
	{
		if (pRec->m_nKind == 1)
		{
			pWalk = (CFileWalk*)pState->m_pCursor;

			if (!pWalk)
			{
				// AVP2: a directory written with forward slashes keeps them,
				// as fs_BuildFileList's does (lithtech.exe 0x00429d8c).
				if (strstr(pDirName, "/"))
					sprintf(szPath, "%s/%s/*.*", pRec->m_szDir, pDirName);
				else
					sprintf(szPath, "%s\\%s\\*.*", pRec->m_szDir, pDirName);

				pWalk = (CFileWalk*)dalloc(sizeof(CFileWalk));

				pWalk->m_pTree = pRec;
				nRes = pWalk->m_hFind = _findfirst(szPath, &pWalk->m_Find);

				pState->m_pCursor = pWalk;
			}
			else
			{
				nRes = _findnext(pWalk->m_hFind, &pWalk->m_Find);
			}

			for (;;)
			{
				if (nRes == -1)
				{
					pState->m_pCursor = LTNULL;

					if (pWalk->m_hFind != -1)
						_findclose(pWalk->m_hFind);

					dfree(pWalk);

					break;
				}

				if (pWalk->m_Find.name[0] != '.')
				{
					strncpy(pState->m_Name, pWalk->m_Find.name, 0xff);
					memcpy(&pState->m_Time, &pWalk->m_Find.time_write, 4);

					pState->m_nSize = pWalk->m_Find.size;
					pState->m_Type = ~pWalk->m_Find.attrib >> 4 & 1;

					return 1;
				}

				nRes = _findnext(pWalk->m_hFind, &pWalk->m_Find);
			}
		}
		else
		{
			pWalk = (CFileWalk*)pState->m_pCursor;

			if (!pWalk)
			{
				CRezDir* pDir;
				CRezItm* pItm;
				CRezDir* pSubDir;

				pDir = pRec->m_pArchive->GetDirFromPath(pDirName);

				if (!pDir)
					return 0;

				pTyp = pDir->GetFirstType();

				if (pTyp)
					pItm = pDir->GetFirstItem(pTyp);
				else
					pItm = LTNULL;

				if (!pItm)
				{
					pSubDir = pDir->GetFirstSubDir();

					if (!pSubDir)
						return 0;
				}
				else
				{
					pSubDir = LTNULL;
				}

				pWalk = (CFileWalk*)dalloc(sizeof(CFileWalk));

				pState->m_pCursor = pWalk;

				pWalk->m_pTree = pRec;
				pWalk->m_pDir = pDir;
				pWalk->m_pTyp = pTyp;
				pWalk->m_pItm = pItm;
				pWalk->m_pSubDir = pSubDir;
			}
			else
			{
				if (pWalk->m_pItm)
				{
					pWalk->m_pItm = pWalk->m_pDir->GetNextItem(pWalk->m_pItm);

					while (!pWalk->m_pItm)
					{
						pWalk->m_pTyp = pWalk->m_pDir->GetNextType(pWalk->m_pTyp);

						if (!pWalk->m_pTyp)
						{
							pWalk->m_pSubDir = pWalk->m_pDir->GetFirstSubDir();

							if (!pWalk->m_pSubDir)
							{
								dfree(pState->m_pCursor);

								pState->m_pCursor = LTNULL;

								return 0;
							}

							break;
						}

						pWalk->m_pItm = pWalk->m_pDir->GetFirstItem(pWalk->m_pTyp);
					}
				}
				else
				{
					pWalk->m_pSubDir = pWalk->m_pDir->GetNextSubDir(pWalk->m_pSubDir);

					if (!pWalk->m_pSubDir)
					{
						dfree(pState->m_pCursor);

						pState->m_pCursor = LTNULL;

						return 0;
					}
				}
			}

			if (pWalk->m_pItm)
			{
				strncpy(pState->m_Name, pWalk->m_pItm->m_pName, 0xfc);

				pRec->m_pArchive->TypeToStr(pWalk->m_pItm->GetType(), szExt);

				strcat(pState->m_Name, ".");
				strcat(pState->m_Name, szExt);

				pState->m_Time = pWalk->m_pItm->m_Time;
				pState->m_Type = 1;
				pState->m_nSize = pWalk->m_pItm->m_nSize;
			}
			else
			{
				if (!pWalk->m_pSubDir)
					return 0;

				strncpy(pState->m_Name, pWalk->m_pSubDir->m_pName, 0xff);

				pState->m_Time = pWalk->m_pSubDir->m_Time;
				pState->m_Type = 0;
				pState->m_nSize = 0;
			}

			return 1;
		}
	}

	return 0;
}


void fs_Init(CFileMgr* pFileMgr, CServerMgr* pMgr)
{
	memset(pFileMgr, 0, sizeof(*pFileMgr));

	pFileMgr->m_hFileHash = dhash_Create(500, DHASHKEY_FILENAME);
	pFileMgr->m_Trees.TieOff();
	pFileMgr->m_pServerMgr = pMgr;

	sb_FreeChunks(&pFileMgr->m_Bank.m_Bank, 1);
	sb_Init2(&pFileMgr->m_Bank.m_Bank, 0x14, 0x40, 0x200);
}

// The loop's pNext is declared (and read) before pTree: lithtech.exe keeps
// the node in ebx and the next link in edi (0x0046b6be), the other way round
// from pTree-first.
void fs_Term(CFileMgr* pFileMgr)
{
	LTLink* pCur;

	fs_FlushFiles(pFileMgr);

	if (pFileMgr->m_hFileHash)
	{
		dhash_Free(pFileMgr->m_hFileHash);
		pFileMgr->m_hFileHash = LTNULL;
	}

	pCur = pFileMgr->m_Trees.m_pNext;
	while (pCur != &pFileMgr->m_Trees)
	{
		LTLink*    pNext = pCur->m_pNext;
		CFileTreeNode* pTree = (CFileTreeNode*)pCur->m_pData;

		fs_CloseTree(pTree->m_pTree);
		dfree(pTree);

		pCur = pNext;
	}

	pFileMgr->m_Trees.TieOff();
}

ILTStream* fs_OpenFile(CFileMgr* pFileMgr, char* pFilename)
{
	return fs_OpenFileEx(pFileMgr, pFilename, LTNULL, LTNULL);
}

// The manager argument is accepted and unused: a file identifier already
// knows which string it is.
char* fs_GetFileName(CFileMgr* pFileMgr, void* pFile)
{
	if (pFile)
		return dhash_GetKey(*(void**)pFile, 0);

	return g_sNoFileName;
}

// Thirty trees is the ceiling, and it is silent: past that the rest of the
// list is simply not searched.
FileEntry* fs_GetFileList(CFileMgr* pFileMgr, char* pDirName)
{
	void*   pTrees[30];
	int     nTrees;
	LTLink* pCur;

	if (!pFileMgr)
		return LTNULL;

	nTrees = 0;

	for (pCur = pFileMgr->m_Trees.m_pNext;
	     pCur != &pFileMgr->m_Trees;
	     pCur = pCur->m_pNext)
	{
		pTrees[nTrees] = ((CFileTreeNode*)pCur->m_pData)->m_pTree;
		nTrees++;

		if (nTrees >= 30)
			break;
	}

	return fs_BuildFileList(pTrees, nTrees, pDirName);
}


// 0x1002cb99 — the manager's own construction. The record bank is a small
// class of its own (a vptr over a struct bank), which is why the sizes land
// one dword later than the manager's other fields.
CFileMgr::CFileMgr()
{
}


// 0x1002bf82 — drop every cached file. The records go back to the bank, not
// the allocator, and the hash is emptied one entry at a time because the
// iterator has to step before the entry it is on is removed.
void fs_FlushFiles(CFileMgr* pFileMgr)
{
	void* hIter;
	void* hCur;

	hIter = dhash_GetFirst(pFileMgr->m_hFileHash);

	while (hIter)
	{
		hCur = dhash_GetNext(&hIter);

		sb_Free(&pFileMgr->m_Bank.m_Bank, dhash_GetData(hCur));
		dhash_Remove(pFileMgr->m_hFileHash, hCur);
	}

	pFileMgr->m_nFiles = 0;
}


// 0x10006f60 — close one open tree record.
//
// The critical section is torn down with the archive, not with the record, so
// a record whose archive never opened is freed without one being deleted. The
// two archive-layer globals are zeroed either way — they sit past the `if`,
// which is what says they belong to the layer and not to this record.
void fs_CloseTree(void* pTree)
{
	COpenTree* pRec = (COpenTree*)pTree;

	if (!pRec)
		return;

	if (pRec->m_pArchive)
	{
		delete pRec->m_pArchive;
		pRec->m_pArchive = LTNULL;

		DeleteCriticalSection(&pRec->m_CS);
	}

	g_pUnknown58358 = LTNULL;
	g_pUnknown582e0 = LTNULL;

	dfree(pRec);
}


// 0x10006fb4 — what one tree knows about one named file, into the caller's
// find record (name ours).
//
// The two tree kinds answer from two places and the record does not say which
// it came from. A plain directory is asked with the CRT's own find over the
// joined path, and directories are skipped rather than answered -- a name that
// resolves to a folder keeps stepping until a file turns up or the walk runs
// out, which is why this is a find and not a stat. A .rez tree asks its
// archive for the item and reads the two words straight off it.
//
// Byte-identical on both images. The walk is the ordinary find idiom: one
// result variable, seeded from _findfirst and refreshed from _findnext, is
// the loop's condition, and the handle is kept apart from it for the close.
// VC6 rotates that `while` into an entry test and a bottom test, which is the
// shipped `je` at 0x10007009 and `jne` at 0x10007026 (lithtech.exe
// 0x00420da7 / 0x00420dc1), with the attribute test alone at the top of the
// loop and the not-found close falling out of the bottom. Every shape that
// tested the handle and _findnext's answer as two separate `== -1`s instead
// (two gotos, do/while, for(;;) with break, goto loops, a bFound flag) came
// out with one of the two branches cross-jumped onto the other as a `jmp` on
// server.dll, and with the attribute test duplicated at the bottom of a
// rotated loop on lithtech.exe (291-296 of 300 bytes).
//
// The find handle is closed even when the find never opened
// (`_findclose(-1)`), because the close sits past the loop rather than
// inside a guard. The time is copied four bytes at a time through memcpy
// rather than assigned, where the archive side is a plain assignment -- the
// find data's field is a `time_t` and the record's is not. Written with a
// `return` on each path, as here, server.dll's /O1 build merges the two
// closes itself and carries the answer in ebx (0x10007028, 0x10007071), and
// lithtech.exe's /O2 build keeps them apart.
//
// The `push` of the filename is hoisted above the branch by the compiler
// because it is the last argument of both calls, the same as in
// fs_OpenStreamInTree.
int ar_FindFile(COpenTree* pTree, char* pFilename, CFileFindState* pState)
{
	long               hFind;
	long               nRet;
	char               szPath[500];
	struct _finddata_t fd;

	if (!pTree)
		return 0;

	if (pTree->m_nKind == 1)
	{
		sprintf(szPath, "%s\\%s", pTree->m_szDir, pFilename);

		hFind = _findfirst(szPath, &fd);
		nRet  = hFind;

		while (nRet != -1)
		{
			if (!(fd.attrib & _A_SUBDIR))
			{
				strncpy(pState->m_Name, fd.name, 0xff);

				pState->m_Type = 1;
				pState->m_nSize = fd.size;

				memcpy(&pState->m_Time, &fd.time_write, 4);

				_findclose(hFind);
				return 1;
			}

			nRet = _findnext(hFind, &fd);
		}

#ifdef AVP2_MODERN
		// fixed: no close for a find that never opened (RETAIL_BUGS.md)
		if (hFind != -1)
#endif
		_findclose(hFind);
		return 0;
	}
	else
	{
		CRezItm* pItm = pTree->m_pArchive->Find(pFilename);

		if (pItm)
		{
			strncpy(pState->m_Name, pItm->m_pName, 0xff);

			pState->m_Type = 1;
			pState->m_nSize = pItm->m_nSize;
			pState->m_Time = pItm->m_Time;

			return 1;
		}

		return 0;
	}
}


// 0x100070b6 — the on-disk path of a file inside a *directory* tree. Anything
// else, including a null tree, answers zero and leaves the buffer alone. The
// fourth argument is the caller's buffer length and is pushed and never read,
// so the join can overrun it (counter.cpp names the argument at the call
// site; the name here is by address, as it is there).
//
// NEAR (45 of 45 bytes, 17 of 17 instructions, two blocks): the shipped build
// puts the shared `return 0` BETWEEN the two guards, so the first jumps
// forward to it and the second jumps over it, where ours puts it after the
// body and both guards jump forward. `&&` around the success path and `||`
// around the failure both give ours. Cross-jump-host class.
int sub_100070b6(void* hTree, char* pFilename, char* pOutPath, uint32 nOutLen)
{
	COpenTree* pTree = (COpenTree*)hTree;

	if (!pTree)
		return 0;

	if (pTree->m_nKind != 1)
		return 0;

#ifdef AVP2_MODERN
	// fixed: the tree-path builder ignores the buffer length it is given
	// (RETAIL_BUGS.md). A path that does not fit is cut short, terminated,
	// and reported as a failure.
	if (!nOutLen)
		return 0;

	{
		int nLen = _snprintf(pOutPath, nOutLen, "%s\\%s", pTree->m_szDir,
		                     pFilename);

		if (nLen < 0 || (uint32)nLen >= nOutLen)
		{
			pOutPath[nOutLen - 1] = '\0';
			return 0;
		}
	}
#else
	sprintf(pOutPath, "%s\\%s", pTree->m_szDir, pFilename);
#endif

	return 1;
}


// 0x1002bcee — the full form of the open.
//
// The filename hash is consulted first, and a hit opens straight out of the
// tree the record remembers. A miss walks every open tree in order and takes
// the first that has the file; only then, and only if the caller asked for it,
// is the file registered so the hash can answer next time.
//
// Two things are read off the bytes. The hash key length is strlen+1, so the
// terminator is part of the key. And the returned stream lives in the dead
// pFilename parameter slot while the name itself stays in ebx — which is what
// a plain local gets once the parameter is only read through a register.
// The loop names the tree node (pNode) before the call: lithtech.exe then
// reuses eax for the node's tree pointer (`mov 0xc(%eax),%eax`, 0x0046b81a)
// as shipped, where the node cast nested in the argument took ecx.
ILTStream* fs_OpenFileEx(CFileMgr* pFileMgr, char* pFilename, int bRegister, int nType)
{
	void*   hElement;
	LTLink* pCur;

	hElement = dhash_FindElement(pFileMgr->m_hFileHash, pFilename, strlen(pFilename) + 1);
	if (hElement)
	{
		CFileBankRec* pRec = (CFileBankRec*)dhash_GetData(hElement);

		if (pRec)
			return ar_OpenFile(pRec->m_pTreeNode->m_pTree, pFilename, 0);
	}

	for (pCur = pFileMgr->m_Trees.m_pNext;
	     pCur != &pFileMgr->m_Trees;
	     pCur = pCur->m_pNext)
	{
		CFileTreeNode* pNode = (CFileTreeNode*)pCur->m_pData;
		ILTStream* pStream = ar_OpenFile(pNode->m_pTree, pFilename, 0);
		if (pStream)
		{
			if (bRegister)
				fs_GetFileIdentifier(pFileMgr, pFilename, nType, LTNULL);

			return pStream;
		}
	}

	return LTNULL;
}


// 0x10013741 — turn a set of open trees into the FileEntry chain the
// interface hands out.
//
// Each tree is walked in turn, a name already on the list is skipped (so the
// first tree that has a file wins), and every entry is a single allocation:
// the header, then the base name, then the full path, all in one block with
// the two char pointers aimed into its tail. The list is built by prepending,
// so it comes back in reverse discovery order.
//
// The entry type is not read from the walker but derived from it:
// `sete al; inc eax` off a compare against zero, so a state type of zero is
// TYPE_DIRECTORY (2) and anything else TYPE_FILE (1).
//
// The type goes through a function-scope `int` rather than straight into the
// field, and that one line is what decides the whole function's register
// allocation. Without it VC6 hands ebx to pDirName and spills pHead to a
// frame slot (four bytes over, and a 0x214 frame against the shipped 0x210,
// which is exactly szPath plus the find state and no room for anything
// else); with it pHead keeps ebx and pDirName is re-read from its parameter
// slot at all three uses, which is what ships. Nine other shapes were probed
// — pHead declared first, initialised at its declaration, marked `register`,
// pEntry hoisted to function scope, the state and the buffer moved into inner
// scopes, the tree pointer walked instead of indexed, the prepend moved above
// the string copies, and revisions 8168 and 8966 — and every one of them left
// the allocation the wrong way round.
FileEntry* fs_BuildFileList(void** pTrees, int nTrees, char* pDirName)
{
	char           szPath[256];
	CFileFindState state;
	FileEntry*     pHead;
	int            i;
	int            nType;

	pHead = LTNULL;

	for (i = 0; i < nTrees; i++)
	{
		state.m_pCursor = LTNULL;

		while (ar_FindNext(pTrees[i], pDirName, &state))
		{
			FileEntry* pEntry;

			if (fs_FindFileEntry(pHead, state.m_Name))
				continue;

			// AVP2: a directory written with forward slashes keeps them
			// (lithtech.exe 0x004426cd, AvP2 1.0.9.6 0x0043e971).
			if (pDirName[0] == 0)
				strcpy(szPath, state.m_Name);
			else if (strstr(pDirName, "/"))
				sprintf(szPath, "%s/%s", pDirName, state.m_Name);
			else
				sprintf(szPath, "%s\\%s", pDirName, state.m_Name);

			pEntry = (FileEntry*)dalloc(strlen(szPath) + strlen(state.m_Name) + 21);

			pEntry->m_pBaseFilename = (char*)pEntry + sizeof(FileEntry);
			pEntry->m_pFullFilename = (char*)pEntry + sizeof(FileEntry) +
			                          strlen(state.m_Name) + 1;

			memcpy(pEntry->m_pBaseFilename, state.m_Name, strlen(state.m_Name) + 1);
			memcpy(pEntry->m_pFullFilename, szPath, strlen(szPath) + 1);

			nType = (state.m_Type == 0) ? TYPE_DIRECTORY : TYPE_FILE;

			pEntry->m_pNext = pHead;
			pHead = pEntry;

			pEntry->m_Type = nType;
		}
	}

	return pHead;
}


// ---- mounting resource trees --------------------------------------------- //

// One mounted tree on the manager's list. serverengine.h models the same
// sixteen-byte record as CFileTreeRec with the first twelve bytes padded out;
// they are an LTLink, and the record's own link carries a data pointer back
// to itself (names ours).
class CFileTreeMount
{
public:
	LTLink m_Link;                           // 0x00
	void*  m_hTree;                          // 0x0c
};

// 0x10006e66 — open a resource tree by name (name ours).
//
// The name is looked up with a find-first rather than a stat, and the find
// data's attribute word is what decides which kind of tree this is: a
// directory becomes a kind-1 record with no archive, anything else is opened
// as a .rez archive. Either way the record carries the name inline past its
// critical section, which is why the allocation is the name's length plus the
// record's own size.
//
// The answer is zero for success, -1 if the name is not there at all, and -2
// if it is there but could not be opened as an archive. The handle is written
// out through the second argument, and cleared first, so a failed call leaves
// the caller with a null rather than whatever it had.
//
// Two things about the archive path are the shipped build's and not ours. The
// two globals are zeroed *inside* it, so opening a directory tree leaves them
// alone where fs_CloseTree clears them for every kind of tree. And an archive
// record whose `new` fails returns -2 without freeing the record it just
// allocated -- a leak of `strlen(name) + 0x24` bytes -- while the same return
// reached from a failed Open does free it.
int fs_OpenTree(char* pName, void** phTree)
{
	struct _finddata_t fd;
	long        hFind;
	uint32      nSize;
	COpenTree*  pRec;

	*phTree = LTNULL;

	hFind = _findfirst(pName, &fd);

	if (hFind == -1)
		return hFind;

	_findclose(hFind);

	nSize = strlen(pName) + sizeof(COpenTree);

	if (fd.attrib & _A_SUBDIR)
	{
		pRec = (COpenTree*)dalloc_z(nSize);

		pRec->m_nKind = 1;
		pRec->m_pArchive = LTNULL;
	}
	else
	{
		pRec = (COpenTree*)dalloc_z(nSize);

		pRec->m_nKind = 0;
		pRec->m_pArchive = new CFileArchive;

		if (!pRec->m_pArchive)
			return -2;

		if (!pRec->m_pArchive->Open(pName, 1, 0))
		{
			delete pRec->m_pArchive;
			dfree(pRec);

			return -2;
		}

		pRec->m_pArchive->SetDelimiters("\\/");

		g_pUnknown58358 = LTNULL;
		g_pUnknown582e0 = LTNULL;

		InitializeCriticalSection(&pRec->m_CS);
	}

	strcpy(pRec->m_szDir, pName);

	*phTree = pRec;

	return 0;
}

// 0x10006fa5 — the handle a caller gets back for a tree it asked to mount.
// A null handle answers 1 rather than zero, which is what lets the caller
// tell "no tree" from "the tree has nothing in it".
void* sub_10006fa5(void* hTree)
{
	if (!hTree)
		return (void*)1;

	return *(void**)hTree;
}


// 0x1002bc5e — mount a list of resource names, threading one record per tree
// onto the manager's own list head. Two counts leave here and they are not
// the same: the caller's array is walked in full, while the answer written
// back through pnTrees counts only the trees that opened, so a name that
// failed leaves a hole the caller cannot see.
//
// The out-pointer array advances only on success too, so it stays packed
// against the answer rather than aligned with the input names. Whether the
// caller wanted its handles back is decided from the original argument on
// every iteration -- the walking copy would have been overwritten -- which
// is what says the source kept the two separate.
//
// ppOut is declared before ppName: that order homes the down-counter in the
// dead pTrees parameter slot on lithtech.exe (0x24(%esp), 0x0046b713) as
// shipped; ppName first put it in the pResources slot.
void fs_AddResources(CFileMgr* pFileMgr, char** pResources, uint32 nResources,
                     void** pTrees, int* pnTrees)
{
	int nAdded = 0;

	if ((int)nResources > 0)
	{
		void**  ppOut  = pTrees;
		char**  ppName = pResources;
		uint32  nLeft  = nResources;

		do
		{
			void* hTree;

			if (!fs_OpenTree(*ppName, &hTree))
			{
				CFileTreeMount* pRec =
					(CFileTreeMount*)dalloc(sizeof(CFileTreeMount));

				pRec->m_Link.m_pData = pRec;
				pRec->m_hTree = hTree;

				pFileMgr->m_Trees.AddAfter(&pRec->m_Link);

				if (pTrees)
					*ppOut = sub_10006fa5(hTree);

				nAdded++;
				ppOut++;
			}

			ppName++;
		}
		while (--nLeft);
	}

	*pnTrees = nAdded;
}
