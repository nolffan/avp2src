//------------------------------------------------------------------
//
//  MODULE  : WORLDTREE2.CPP
//
//  PURPOSE : server.dll's RIFF/WAVE reader and world tree (0x100392a9 ..
//            0x1003a54f; Primal Hunt from 0x10049409), the client's
//            src/engine/lithtech/worldtree2.cpp's twin. Split out of
//            worldtree.cpp (the rest of its SECTION D, and E and F). No
//            <string> here: nothing follows Primal Hunt's 0x100493e8 guard
//            pair in this range.
//
//------------------------------------------------------------------

#include "worldbsp.h"
#include "servermgr.h"

// ---- the RIFF/WAVE reader (lttexmod.cpp's, verbatim in shape) ----------- //

#define WAVE_FOURCC_D(c0, c1, c2, c3)                           \
	((uint32)(uint8)(c0) | ((uint32)(uint8)(c1) << 8) |         \
	 ((uint32)(uint8)(c2) << 16) | ((uint32)(uint8)(c3) << 24))

#define FOURCC_RIFF_D WAVE_FOURCC_D('R', 'I', 'F', 'F')
#define FOURCC_LIST_D WAVE_FOURCC_D('L', 'I', 'S', 'T')
#define FOURCC_WAVE_D WAVE_FOURCC_D('W', 'A', 'V', 'E')
#define FOURCC_FMT_D  WAVE_FOURCC_D('f', 'm', 't', ' ')
#define FOURCC_DATA_D WAVE_FOURCC_D('d', 'a', 't', 'a')
#define FOURCC_FACT_D WAVE_FOURCC_D('f', 'a', 'c', 't')
#define FOURCC_CUE_D  WAVE_FOURCC_D('c', 'u', 'e', ' ')
#define FOURCC_ADTL_D WAVE_FOURCC_D('a', 'd', 't', 'l')
#define FOURCC_LTXT_D WAVE_FOURCC_D('l', 't', 'x', 't')
#define FOURCC_LITH_D WAVE_FOURCC_D('l', 'i', 't', 'h')
#define FOURCC_INFO_D WAVE_FOURCC_D('i', 'n', 'f', 'o')

#define WAVEFMT_PCM_D       0x0001
#define WAVEFMT_IMAADPCM_D  0x0011
#define WAVEFMT_MP3_D       0x0055

#pragma pack(push, 1)
struct WaveFormatEx_D
{
	uint16 wFormatTag;                       // 0x00
	uint16 nChannels;                        // 0x02
	uint32 nSamplesPerSec;                   // 0x04
	uint32 nAvgBytesPerSec;                  // 0x08
	uint16 nBlockAlign;                      // 0x0c
	uint16 wBitsPerSample;                   // 0x0e
	uint16 cbSize;                           // 0x10
};
#pragma pack(pop)

struct WaveInfo_D
{
	WaveFormatEx_D m_Format;                 // 0x00
	uint32         m_nDataOffset;            // 0x14
	uint32         m_nDataSize;              // 0x18
	uint32         m_nSampleCount;           // 0x1c
	uint32         m_nLithFlags;             // 0x20
	uint32         m_nLithUnknown24;         // 0x24
	uint32         m_bHasLoop;               // 0x28
	uint32         m_nLoopName;              // 0x2c
	uint32         m_nLoopStart;             // 0x30
	uint32         m_nLoopLength;            // 0x34
};

struct WaveChunkData_D
{
	uint32 m_dwName;                         // 0x00
	uint32 m_dwPosition;                     // 0x04
	uint32 m_fccChunk;                       // 0x08
	uint32 m_dwChunkStart;                   // 0x0c
	uint32 m_dwBlockStart;                   // 0x10
	uint32 m_dwSampleOffset;                 // 0x14
};

// RIFF's labelled-text chunk header, the first twenty bytes of 'ltxt'.
struct WaveLabelText_D
{
	uint32 m_dwName;                         // 0x00
	uint32 m_dwSampleLength;                 // 0x04
	uint32 m_dwPurpose;                      // 0x08
	uint16 m_wCountry;                       // 0x0c
	uint16 m_wLanguage;                      // 0x0e
	uint16 m_wDialect;                       // 0x10
	uint16 m_wCodePage;                      // 0x12
};

LTBOOL wav_ParseChunk_D(ILTStream* pStream, uint32 chunkID, uint32 chunkSize,
                        uint32 formType, WaveInfo_D* pInfo);         // 0x10039341

// 0x100392a9 — the whole file (lithtech 0x0048c1a0).
LTBOOL wav_ParseFile_D(ILTStream* pStream, WaveInfo_D* pInfo)
{
	uint32 chunkID, chunkSize;

	pStream->SeekTo(0);

	memset(pInfo, 0, sizeof(*pInfo));

	while (pStream->GetPos() < pStream->GetLen())
	{
		if (pStream->Read(&chunkID, sizeof(chunkID)) != LT_OK)
			break;
		if (pStream->Read(&chunkSize, sizeof(chunkSize)) != LT_OK)
			break;

		if (!wav_ParseChunk_D(pStream, chunkID, chunkSize, 0, pInfo))
			break;
	}

	return (pInfo->m_Format.wFormatTag == WAVEFMT_PCM_D ||
	        pInfo->m_Format.wFormatTag == WAVEFMT_IMAADPCM_D ||
	        pInfo->m_Format.wFormatTag == WAVEFMT_MP3_D);
}

// 0x10039341 — one chunk, recursive (lithtech 0x0048c270).
LTBOOL wav_ParseChunk_D(ILTStream* pStream, uint32 chunkID, uint32 chunkSize,
                        uint32 formType, WaveInfo_D* pInfo)
{
	uint32 nRead;

	uint32 endPos = pStream->GetPos() + chunkSize;
	if (endPos & 1)
		endPos++;
	if (endPos > pStream->GetLen())
		endPos = pStream->GetLen();

	switch (chunkID)
	{
		case FOURCC_RIFF_D:
		case FOURCC_LIST_D:
		{
			uint32 subFormType, subChunkID, subChunkSize;

			pStream->Read(&subFormType, sizeof(subFormType));

			while (pStream->GetPos() < endPos)
			{
				pStream->Read(&subChunkID, sizeof(subChunkID));
				pStream->Read(&subChunkSize, sizeof(subChunkSize));

				if (!wav_ParseChunk_D(pStream, subChunkID, subChunkSize,
				                      subFormType, pInfo))
					break;
			}
			break;
		}

		case FOURCC_FMT_D:
		{
			if (formType != FOURCC_WAVE_D)
				break;

			nRead = sizeof(WaveFormatEx_D);
			if (chunkSize < nRead)
				nRead = chunkSize;
			pStream->Read(&pInfo->m_Format, nRead);
			break;
		}

		case FOURCC_DATA_D:
		{
			if (formType != FOURCC_WAVE_D)
				break;

			pInfo->m_nDataOffset = pStream->GetPos();
			pInfo->m_nDataSize   = chunkSize;

			if (pInfo->m_Format.wFormatTag == WAVEFMT_IMAADPCM_D)
			{
				uint32 nHeaderBytes = 4 << (pInfo->m_Format.nChannels >> 1);

				uint32 nSamplesPerBlock =
					((pInfo->m_Format.nBlockAlign - nHeaderBytes) * 8) / nHeaderBytes + 1;
				uint32 nBlocks =
					(chunkSize + pInfo->m_Format.nBlockAlign - 1) / pInfo->m_Format.nBlockAlign;

				pInfo->m_nSampleCount = nSamplesPerBlock * nBlocks;
			}
			else
			{
				pInfo->m_nSampleCount = 0;
			}
			break;
		}

		case FOURCC_FACT_D:
		{
			if (formType != FOURCC_WAVE_D)
				break;
			if (pInfo->m_Format.wFormatTag != WAVEFMT_IMAADPCM_D)
				break;

			pStream->Read(&pInfo->m_nSampleCount,
			              sizeof(pInfo->m_nSampleCount));
			break;
		}

		case FOURCC_CUE_D:
		{
			WaveChunkData_D chunkData;
			uint32          nCuePoints, nLeft;

			pStream->Read(&nCuePoints, sizeof(nCuePoints));
			nLeft = chunkSize - sizeof(nCuePoints);
			if (nCuePoints > 0)
			{

				nRead = sizeof(chunkData);
				if (chunkSize < nLeft)
					nRead = nLeft;
				pStream->Read(&chunkData, nRead);

				if (chunkData.m_fccChunk == FOURCC_DATA_D)
				{
					pInfo->m_bHasLoop    = 1;
					pInfo->m_nLoopName   = chunkData.m_dwName;
					pInfo->m_nLoopStart  = chunkData.m_dwPosition;
				}
			}
			break;
		}

		case FOURCC_LTXT_D:
		{
			WaveLabelText_D labelData;

			if (formType != FOURCC_ADTL_D)
				break;

			nRead = sizeof(labelData);
			if (chunkSize < nRead)
				nRead = chunkSize;
			pStream->Read(&labelData, nRead);

			if (!pInfo->m_bHasLoop)
				break;
			if (labelData.m_dwName != pInfo->m_nLoopName)
				break;

			pInfo->m_nLoopLength = labelData.m_dwSampleLength;
			break;
		}

		case FOURCC_LITH_D:
		{
			if (formType != FOURCC_INFO_D)
				break;

			nRead = 8;
			if (chunkSize < nRead)
				nRead = chunkSize;
			pStream->Read(&pInfo->m_nLithFlags, nRead);
			break;
		}
	}

	pStream->SeekTo(endPos);
	return LTTRUE;
}


// ---- the world tree's object base and node (worldtree2.cpp's) ---------- //

#define WT_NUMLISTS_D   3
#define WT_NUMLINKS_D   5
#define WT_NUMKIDS_D    4

class CWTNode_D;
class CWorldTree_D;
struct WTFindInfo_D;

// CheapLTLink plus m_pData rather than LTLink: LTLink's (empty, inline)
// constructor makes an array of these call ??_H at /O1, where 0x10039707
// ties the five links off in line.
class CWTLink_D : public CheapLTLink
{
public:
	void*      m_pData;                      // +0x08
	CWTNode_D* m_pNode;                      // +0x0c
};

// vtable 0x1004ea04; objtypes.h's CObjRoot.
class CWTObj_D
{
public:
	CWTObj_D(uint32 nObjType);                                // 0x10039707
	virtual ~CWTObj_D();                                      // 0x10039735

	virtual LTBOOL     WantsTree(CWorldTree_D *pTree)     { return LTFALSE; }
	virtual void       RemoveFromLists();                     // 0x10039740
	virtual void       GetDims(LTVector *pMin, LTVector *pMax) = 0;
	virtual LTBOOL     WantsExtraLink()                   { return LTFALSE; }
	virtual CWTLink_D* MakeExtraLink(uint32 nIndex)       { return LTNULL; }
	virtual void       NotifyInserted(CWTObj_D *pObj)     {}
	virtual void       OnFoundInBox(WTFindInfo_D *pInfo)  {}

public:
	CWTLink_D m_Links[WT_NUMLINKS_D];        // +0x04
	uint32    m_nObjType;                    // +0x54
	uint32    m_nTouchCode;                  // +0x58
};

class CWTNode_D
{
public:
	CWTNode_D();                                              // 0x10039780
	~CWTNode_D();                                             // 0x1003978c

	void   Init();                                            // 0x10039791
	void   Term();                                            // 0x100398b7
	void   FreeChildren();                                    // 0x100398f3

	void   GetDims(LTVector *pMin, LTVector *pMax);           // 0x100397ec
	void   SetDims(LTVector vMin, LTVector vMax);             // 0x10039807

	LTBOOL AllocChildren();                                   // 0x100399a5
	LTBOOL LoadNodes(ILTStream *pStream,
	                 uint8 *pCurByte, uint8 *pCurBit);        // 0x1003991d

public:
	CheapLTLink m_Objects[WT_NUMLISTS_D];    // +0x00
	LTVector    m_vBoxMin;                   // +0x18
	LTVector    m_vBoxMax;                   // +0x24
	LTVector    m_vCenter;                   // +0x30
	float       m_fSmallestDim;              // +0x3c
	float       m_fRadius;                   // +0x40
	uint32      m_nNumObjects;               // +0x44
	CWTNode_D*  m_pParent;                   // +0x48
	CWTNode_D*  m_Children[WT_NUMKIDS_D];    // +0x4c
};

typedef void (*WTBoxCallback_D)(CWTObj_D *pObj, void *pUserData);
typedef int  (*WTNodeTest_D)(CWTNode_D *pNode);

class CWorldTree_D
{
public:
	void*     m_pVTable;                     // +0x00
	void*     m_pCodeSource;                 // +0x04
	uint32    m_nCurCode;                    // +0x08
};

struct WTFindInfo_D
{
	uint32          m_nListIndex;            // +0x00
	LTVector        m_vCenter;               // +0x04
	float           m_fRadius;               // +0x10
	WTBoxCallback_D m_pfnCallback;           // +0x14
	uint32          m_n018;                  // +0x18
	void*           m_pUserData;             // +0x1c
	uint32          m_n020;                  // +0x20
	uint32          m_n024;                  // +0x24
	WTNodeTest_D    m_pfnNodeTest;           // +0x28
	LTVector        m_vBoxMin;               // +0x2c
	LTVector        m_vBoxMax;               // +0x38
	CWorldTree_D*   m_pTree;                 // +0x44
};

// 0x100395a4 — the recursive box query (lithtech 0x0048c540).
void wt_FindObjectsInBox_D(CWTNode_D *pNode, WTFindInfo_D *pInfo, uint32 nLevel)
{
	CheapLTLink *pHead;
	LTLink      *pCur;
	CWTObj_D    *pObj;
	LTVector     vMin, vMax;

	if (!pInfo->m_pfnNodeTest(pNode))
		return;

	pHead = &pNode->m_Objects[2];
	if (pHead->m_pNext != (LTLink*)pHead)
	{
		pCur = pHead->m_pNext;
		while (pCur != (LTLink*)pHead)
		{
			pObj = (CWTObj_D*)pCur->m_pData;
			pCur = pCur->m_pNext;

			if (pObj->m_nTouchCode != pInfo->m_pTree->m_nCurCode)
			{
				pObj->m_nTouchCode = pInfo->m_pTree->m_nCurCode;
				pObj->OnFoundInBox(pInfo);
			}
		}

		return;
	}

	pHead = &pNode->m_Objects[pInfo->m_nListIndex];
	pCur  = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj = (CWTObj_D*)pCur->m_pData;
		pCur = pCur->m_pNext;

		if (pObj->m_nTouchCode != pInfo->m_pTree->m_nCurCode)
		{
			pObj->m_nTouchCode = pInfo->m_pTree->m_nCurCode;
			pObj->GetDims(&vMin, &vMax);

			if (vMin.x > pInfo->m_vBoxMax.x) continue;
			if (vMin.y > pInfo->m_vBoxMax.y) continue;
			if (vMin.z > pInfo->m_vBoxMax.z) continue;
			if (vMax.x < pInfo->m_vBoxMin.x) continue;
			if (vMax.y < pInfo->m_vBoxMin.y) continue;

			// AVP2: the last test is the positive one around the call: cl
			// 9044 keeps NaN apart where 8966 does not, and AvP2 1.0.9.6's
			// server.dll (0x10044ada / 0x1004540b `test $0x1; jne`) skips
			// the call on an unordered compare.
			if (vMax.z >= pInfo->m_vBoxMin.z)
				pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}

	if (pNode->m_Children[0])
	{
		if (pInfo->m_vBoxMin.x < pNode->m_vCenter.x)
		{
			if (pInfo->m_vBoxMin.z < pNode->m_vCenter.z)
				wt_FindObjectsInBox_D(pNode->m_Children[0], pInfo, nLevel + 1);

			if (pInfo->m_vBoxMax.z > pNode->m_vCenter.z)
				wt_FindObjectsInBox_D(pNode->m_Children[1], pInfo, nLevel + 1);
		}

		if (pInfo->m_vBoxMax.x > pNode->m_vCenter.x)
		{
			if (pInfo->m_vBoxMin.z < pNode->m_vCenter.z)
				wt_FindObjectsInBox_D(pNode->m_Children[2], pInfo, nLevel + 1);

			if (pInfo->m_vBoxMax.z > pNode->m_vCenter.z)
				wt_FindObjectsInBox_D(pNode->m_Children[3], pInfo, nLevel + 1);
		}
	}
}

// 0x10039707 — objtypes.h's CObjRoot::CObjRoot(int).
CWTObj_D::CWTObj_D(uint32 nObjType)
{
	uint32 i;

	for (i = 0; i < WT_NUMLINKS_D; i++)
	{
		m_Links[i].m_pNode = LTNULL;
		m_Links[i].TieOff();
		m_Links[i].m_pData = this;
	}

	m_nTouchCode = 0xffffffff;
	m_nObjType   = nObjType;
}

// 0x10039735 — the destructor: vptr store and a tail jump to 0x10039740
// (objtypes.h's CObjBase::BaseTerm).
CWTObj_D::~CWTObj_D()
{
	RemoveFromLists();
}

// 0x10039740 — slot 2 (objtypes.h's CObjBase::Unlink).
void CWTObj_D::RemoveFromLists()
{
	uint32 i;

	m_nTouchCode = 0xffffffff;

	for (i = 0; i < WT_NUMLINKS_D; i++)
	{
		CWTLink_D* pLink = &m_Links[i];

		pLink->Remove();

		while (pLink->m_pNode)
		{
			pLink->m_pNode->m_nNumObjects--;
			pLink->m_pNode = pLink->m_pNode->m_pParent;
		}
	}
}

// 0x10039780
CWTNode_D::CWTNode_D()
{
	Init();
}

// 0x1003978c — `jmp 0x100398b7`, five bytes.
CWTNode_D::~CWTNode_D()
{
	Term();
}

// 0x10039791. The four `stos` into m_Children are a four-trip zeroing loop
// (the client twin, worldtree2.cpp's CWTNodeN48C::Init, is the same loop
// unrolled at /O2). An intrinsic memset emitted the same stores but took the
// `xor %eax,%eax` before `lea 0x4c(%ecx),%edi`; the loop gives shipped order.
void CWTNode_D::Init()
{
	uint32 i;

	m_pParent     = LTNULL;
	m_nNumObjects = 0;

	for (i = 0; i < WT_NUMLISTS_D; i++)
		m_Objects[i].TieOff();

	for (i = 0; i < WT_NUMKIDS_D; i++)
		m_Children[i] = LTNULL;

	m_vCenter.Init();
	m_vBoxMin.Init();
	m_vBoxMax.Init();

	m_fSmallestDim = 0.0f;
	m_fRadius      = 0.0f;
}

// 0x100397ec
void CWTNode_D::GetDims(LTVector *pMin, LTVector *pMax)
{
	*pMin = m_vBoxMin;
	*pMax = m_vBoxMax;
}

// 0x10039807
void CWTNode_D::SetDims(LTVector vMin, LTVector vMax)
{
	LTVector vHalfDims;

	m_vBoxMin = vMin;
	m_vBoxMax = vMax;

	vHalfDims = (vMax - vMin) * 0.5f;
	m_vCenter = vMin + vHalfDims;

	m_fSmallestDim = LTMIN(vMax.x - vMin.x, vMax.z - vMin.z);
	m_fRadius      = vHalfDims.Mag();
}

// 0x100398b7
void CWTNode_D::Term()
{
	LTLink   *pCur, *pNext;
	CWTObj_D *pObj;
	uint32    i;

	FreeChildren();

	for (i = 0; i < WT_NUMLISTS_D; i++)
	{
		pCur = m_Objects[i].m_pNext;
		while (pCur != (LTLink*)&m_Objects[i])
		{
			pNext = pCur->m_pNext;
			pObj  = (CWTObj_D*)pCur->m_pData;
			pObj->RemoveFromLists();
			pCur  = pNext;
		}
	}

	Init();
}

// 0x100398f3
void CWTNode_D::FreeChildren()
{
	uint32 i;

	for (i = 0; i < WT_NUMKIDS_D; i++)
	{
		if (m_Children[i])
		{
			delete m_Children[i];
			m_Children[i] = LTNULL;
		}
	}
}

// 0x1003991d
LTBOOL CWTNode_D::LoadNodes(ILTStream *pStream, uint8 *pCurByte, uint8 *pCurBit)
{
	LTBOOL bHasChildren;
	uint32 i;

	FreeChildren();

	if (*pCurBit == 8)
	{
		pStream->Read(pCurByte, 1);
		*pCurBit = 0;
	}

	bHasChildren = ((*pCurByte & (1 << *pCurBit)) != 0);
	(*pCurBit)++;

	if (bHasChildren)
	{
		if (!AllocChildren())
			return LTFALSE;

		for (i = 0; i < WT_NUMKIDS_D; i++)
		{
			if (!m_Children[i]->LoadNodes(pStream, pCurByte, pCurBit))
			{
				FreeChildren();
				return LTFALSE;
			}
		}
	}

	return LTTRUE;
}

// ==== END SECTION D =======================================================

// ==== SECTION E: 0x100399a5 .. 0x10039ec1 ================================
//
// The node's AllocChildren/AddObjectLink, the tree class (ctor, Term,
// FindNodeFromPath, AddObject x2, the two query slots), the insert walk and
// the quadrant helper it recurses through.  Client twins: worldtree2.cpp
// (0x0048ca50, 0x0048cca0, 0x0048cd10..0x0048cee0), worldtree.cpp's tree
// class (vtable 0x004a38f8), lt2new4.cpp (0x0048cce0/0x0048cd50) and
// lt2new2.cpp (0x0048d070).  Server vtable of the tree: 0x1004ea24, seven
// slots: 0x10039be9 0x10039e18 0x10039e9e 0x10039f82 0x10039f9d 0x1003a386
// 0x10039b9f.
//
// The node constructor (0x10039780), SetDims (0x10039807) and Term
// (0x100398b7) are SECTION D's; they are only declared here.

class CWTNode_E;
class CWorldTree_E;
struct WTInsertInfo_E;

class CWTLink_E : public LTLink
{
public:
	CWTNode_E* m_pNode;                      // +0x0c
};

// The object base (client vtable 0x004a38d8).  Only the slots called here.
class CWTObj_E
{
public:
	virtual void         _v000() = 0;
	virtual LTBOOL       WantsTree(CWorldTree_E *pTree) = 0;          // +0x04
	virtual void         RemoveFromLists() = 0;                        // +0x08
	virtual void         GetDims(LTVector *pMin, LTVector *pMax) = 0;  // +0x0c
	virtual LTBOOL       WantsExtraLink() = 0;                         // +0x10
	virtual CWTLink_E*   MakeExtraLink(uint32 nIndex) = 0;             // +0x14
	virtual void         NotifyInserted(CWTObj_E *pObj) = 0;           // +0x18

	CWTLink_E m_Links[5];                    // +0x04
	uint32    m_nObjType;                    // +0x54
	uint32    m_nTouchCode;                  // +0x58
};

class CWTNode_E
{
public:
	CWTNode_E();                                              // 0x10039780 (D)
	void   Term();                                            // 0x100398b7 (D)
	void   SetDims(LTVector vMin, LTVector vMax);             // 0x10039807 (D)

	LTBOOL AllocChildren();                                   // 0x100399a5
	void   AddObjectLink(CWTLink_E *pLink, uint32 nList);     // 0x10039b44

public:
	CheapLTLink  m_Objects[3];               // +0x00
	LTVector     m_vBoxMin;                  // +0x18
	LTVector     m_vBoxMax;                  // +0x24
	LTVector     m_vCenter;                  // +0x30
	float        m_fSmallestDim;             // +0x3c
	float        m_fRadius;                  // +0x40
	uint32       m_nNumObjects;              // +0x44
	CWTNode_E*   m_pParent;                  // +0x48
	CWTNode_E*   m_Children[4];              // +0x4c
};                                           // 0x5c

struct WTInsertInfo_E
{
	CWTObj_E*    m_pObject;                  // +0x00
	uint32       m_nListIndex;               // +0x04
	LTVector     m_vBoxMin;                  // +0x08
	LTVector     m_vBoxMax;                  // +0x14
	float        m_fMaxDim;                  // +0x20
	uint32       m_nNumLinks;                // +0x24
};

struct WTPath_E
{
	uint8  m_Bytes[4];                       // +0x00
	uint32 m_nDepth;                         // +0x04
};

// The query slot 1 builds and slot 2 completes; 0x10039e61 is its
// constructor.  Section F's 0x10039ec1 walks it.  +0x20 is a vector:
// three separate float members let VC6 hoist the +0x2c/+0x30 integer
// stores above the last fldz/fstps pairs; three Init()s keep them last.
struct WTSegQuery_E
{
	WTSegQuery_E();                                           // 0x10039e61

	CWorldTree_E* m_pTree;                   // +0x00  set by slot 2
	uint32        m_nListIndex;              // +0x04
	LTVector      m_vPt1;                    // +0x08
	LTVector      m_vPt2;                    // +0x14
	LTVector      m_v20;                     // +0x20
	void*         m_pfnCallback;             // +0x2c
	void*         m_pUserData;               // +0x30
};

class IWTCodeSource_E
{
public:
	virtual uint32 NextCode() = 0;           // slot 0
	virtual uint32 Slot1() = 0;              // slot 1
};

class CWorldTree_E
{
public:
	CWorldTree_E();                                           // 0x10039b74
	uint32     CodeSourceSlot1();                             // 0x10039ba2
	void       SetCodeSource(IWTCodeSource_E *pSource);       // 0x10039baa
	void       Term();                                        // 0x10039bbc

	void       AddObject(CWTObj_E *pObj, uint32 nListIndex);  // 0x10039c43
	void       AddObject(CWTObj_E *pObj, LTVector *pMin,
	                     LTVector *pMax, uint32 nListIndex);  // 0x10039c78

	virtual CWTNode_E* FindNodeFromPath(WTPath_E *pPath);     // 0x10039be9
	virtual uint32     IntersectSegment(LTVector *pPt1, LTVector *pPt2,
	                                    void *pfnCallback, void *pUserData,
	                                    uint32 nListIndex);   // 0x10039e18
	virtual uint32     DoSegQuery(WTSegQuery_E *pQuery);      // 0x10039e9e
	virtual void       _v00c() = 0;                           // 0x10039f82 (F)
	virtual void       _v010() = 0;                           // 0x10039f9d (F)
	virtual void       _v014() = 0;                           // 0x1003a386 (F)
	virtual void       _v018(void *a, void *b) {}             // 0x10039b9f

public:
	IWTCodeSource_E* m_pCodeSource;          // +0x04
	uint32           m_nCurCode;             // +0x08
	CWTNode_E        m_RootNode;             // +0x0c
	void*        m_p068;                     // +0x68
	void*        m_p06c;                     // +0x6c
	void*        m_p070;                     // +0x70
	CheapLTLink  m_RootObjects;              // +0x74
};

typedef void (*WTNodeCallback_E)(CWTNode_E *pNode, void *pUser);

void wt_InsertObject_E(CWTNode_E *pNode, WTInsertInfo_E *pInfo);
static void wt_ForEachChildInBox_E(LTVector *pMin, LTVector *pMax,
                                   CWTNode_E *pNode, WTNodeCallback_E pfn,
                                   void *pUser);
uint32 wt_SegQueryNode_E(CWTNode_E *pNode, WTSegQuery_E *pQuery);  // 0x10039ec1 (F)


// 0x100399a5, 415 bytes -- lithtech 0x0048ca50.
LTBOOL CWTNode_E::AllocChildren()
{
	LTVector vHalfDims;
	uint32   i;

	for (i = 0; i < 4; i++)
	{
		m_Children[i] = new CWTNode_E;
		if (!m_Children[i])
		{
			Term();
			return LTFALSE;
		}

		m_Children[i]->m_pParent = this;
	}

	vHalfDims = (m_vBoxMax - m_vBoxMin) * 0.5f;

	m_Children[0]->SetDims(
		LTVector(m_vCenter.x - vHalfDims.x, m_vBoxMin.y, m_vCenter.z - vHalfDims.z),
		LTVector(m_vCenter.x,               m_vBoxMax.y, m_vCenter.z));

	m_Children[1]->SetDims(
		LTVector(m_vCenter.x - vHalfDims.x, m_vBoxMin.y, m_vCenter.z),
		LTVector(m_vCenter.x,               m_vBoxMax.y, m_vCenter.z + vHalfDims.z));

	m_Children[2]->SetDims(
		LTVector(m_vCenter.x,               m_vBoxMin.y, m_vCenter.z - vHalfDims.z),
		LTVector(m_vCenter.x + vHalfDims.x, m_vBoxMax.y, m_vCenter.z));

	m_Children[3]->SetDims(
		LTVector(m_vCenter.x,               m_vBoxMin.y, m_vCenter.z),
		LTVector(m_vCenter.x + vHalfDims.x, m_vBoxMax.y, m_vCenter.z + vHalfDims.z));

	return LTTRUE;
}


// 0x10039b44, 48 bytes -- lithtech 0x0048cca0.
void CWTNode_E::AddObjectLink(CWTLink_E *pLink, uint32 nList)
{
	CWTNode_E *pNode;

	m_Objects[nList].AddAfter(pLink);
	pLink->m_pNode = this;

	pNode = this;
	while (pNode)
	{
		pNode->m_nNumObjects++;
		pNode = pNode->m_pParent;
	}
}


// 0x10039b74, 43 bytes -- lithtech 0x0048cce0 (lt2new4.cpp).  The root node
// is a MEMBER, not a base (lt2new4.cpp models it as a second base): the
// `lea 0xc(%ecx)` conversions in FindNodeFromPath/AddObject/DoSegQuery
// carry no neg/sbb null test, and as a member the node's constructor
// still comes before the vptr store, byte-identical.  The in-class empty
// slot 6 (0x10039b9f, `ret $8`, 3 bytes: "small") follows it.
CWorldTree_E::CWorldTree_E()
{
	m_pCodeSource = LTNULL;
	m_p068 = LTNULL;
	m_p06c = LTNULL;
	m_p070 = LTNULL;

	m_RootObjects.TieOff();
}


// 0x10039ba2, 8 bytes -- lithtech 0x0048cd20; tail jump.
uint32 CWorldTree_E::CodeSourceSlot1()
{
	return m_pCodeSource->Slot1();
}


// 0x10039baa, 18 bytes -- lithtech 0x0048cd30.
void CWorldTree_E::SetCodeSource(IWTCodeSource_E *pSource)
{
	m_pCodeSource = pSource;
	m_RootObjects.TieOff();
}


// 0x10039bbc, 45 bytes -- lithtech 0x0048cd50 (lt2new4.cpp).
void CWorldTree_E::Term()
{
	m_RootNode.Term();

	m_p068 = LTNULL;
	m_p06c = LTNULL;

	m_RootObjects.Remove();
}


// 0x10039be9, 90 bytes -- vtable slot 0, lithtech 0x0048cd80.
CWTNode_E* CWorldTree_E::FindNodeFromPath(WTPath_E *pPath)
{
	CWTNode_E *pNode;
	uint32     nBit, i, nByte, nQuadrant;

	pNode = &m_RootNode;

	nByte = 0;
	i     = 0;
	nBit  = 0;

	for (; i != pPath->m_nDepth; i++)
	{
		if (!pNode->m_Children[0])
			return LTNULL;

		nQuadrant = (pPath->m_Bytes[nByte] >> nBit) & 3;
		nBit += 2;

		if (nBit == 8)
		{
			nBit = 0;
			nByte++;

			if (nByte >= 4)
				return LTNULL;
		}

		pNode = pNode->m_Children[nQuadrant];
	}

	return pNode;
}


// 0x10039c43, 53 bytes -- lithtech 0x0048cde0.
void CWorldTree_E::AddObject(CWTObj_E *pObj, uint32 nListIndex)
{
	LTVector vMin, vMax;

	pObj->GetDims(&vMin, &vMax);
	AddObject(pObj, &vMin, &vMax, nListIndex);
}


// 0x10039c78, 136 bytes -- lithtech 0x0048ce20.
void CWorldTree_E::AddObject(CWTObj_E *pObj, LTVector *pMin,
                             LTVector *pMax, uint32 nListIndex)
{
	WTInsertInfo_E info;
	LTVector       vDims;

	pObj->RemoveFromLists();

	vDims = *pMax - *pMin;

	info.m_pObject    = pObj;
	info.m_nListIndex = nListIndex;
	info.m_vBoxMin    = *pMin;
	info.m_vBoxMax    = *pMax;
	info.m_fMaxDim = LTMAX(vDims.x, vDims.z);

	info.m_nNumLinks = 0;

	if (!pObj->WantsTree(this))
		wt_InsertObject_E(&m_RootNode, &info);
}


// 0x10039d00, 152 bytes -- lithtech 0x0048cee0, where the helper below is
// inlined and its callback turned into direct (and one tail) recursion.
void wt_InsertObject_E(CWTNode_E *pNode, WTInsertInfo_E *pInfo)
{
	CWTLink_E *pLink;

	if (pNode->m_Objects[2].m_pNext != (LTLink*)&pNode->m_Objects[2])
		((CWTObj_E*)pNode->m_Objects[2].m_pNext->m_pData)->NotifyInserted(pInfo->m_pObject);

	// AVP2: negated -- an unordered size recurses. cl 9044 keeps NaN apart
	// where 8966 does not (AvP2 1.0.9.6's server.dll 0x100451af `test
	// $0x41; jnp` to the link).
	if (!(pNode->m_fSmallestDim * 0.5f <= pInfo->m_fMaxDim) && pNode->m_Children[0])
	{
		wt_ForEachChildInBox_E(&pInfo->m_vBoxMin, &pInfo->m_vBoxMax, pNode,
		                       (WTNodeCallback_E)wt_InsertObject_E, pInfo);
	}
	else if (pInfo->m_nNumLinks < 5)
	{
		pNode->AddObjectLink(&pInfo->m_pObject->m_Links[pInfo->m_nNumLinks],
		                     pInfo->m_nListIndex);

		if (pInfo->m_pObject->WantsExtraLink())
		{
			pLink = pInfo->m_pObject->MakeExtraLink(pInfo->m_nNumLinks);
			pNode->m_Objects[2].AddAfter(pLink);
		}

		pInfo->m_nNumLinks++;
	}
}


// 0x10039d98, 128 bytes -- cdecl, five arguments; no standalone twin in
// lithtech.exe (inlined into 0x0048cee0).
static void wt_ForEachChildInBox_E(LTVector *pMin, LTVector *pMax,
                                   CWTNode_E *pNode, WTNodeCallback_E pfn,
                                   void *pUser)
{
	if (pMin->x < pNode->m_vCenter.x)
	{
		if (pMin->z < pNode->m_vCenter.z)
			pfn(pNode->m_Children[0], pUser);

		if (pMax->z > pNode->m_vCenter.z)
			pfn(pNode->m_Children[1], pUser);
	}

	if (pMax->x > pNode->m_vCenter.x)
	{
		if (pMin->z < pNode->m_vCenter.z)
			pfn(pNode->m_Children[2], pUser);

		if (pMax->z > pNode->m_vCenter.z)
			pfn(pNode->m_Children[3], pUser);
	}
}


// 0x10039e18, 73 bytes -- vtable slot 1, lithtech 0x0048cff0.
uint32 CWorldTree_E::IntersectSegment(LTVector *pPt1, LTVector *pPt2,
                                      void *pfnCallback, void *pUserData,
                                      uint32 nListIndex)
{
	WTSegQuery_E query;

	query.m_nListIndex  = nListIndex;
	query.m_vPt1        = *pPt1;
	query.m_vPt2        = *pPt2;
	query.m_pfnCallback = pfnCallback;
	query.m_pUserData   = pUserData;

	return DoSegQuery(&query);
}


// 0x10039e61, 61 bytes -- inlined into lithtech 0x0048cff0.
WTSegQuery_E::WTSegQuery_E()
{
	m_pTree      = LTNULL;
	m_nListIndex = 0;
	m_vPt1.Init();
	m_vPt2.Init();
	m_v20.Init();
	m_pfnCallback = LTNULL;
	m_pUserData   = LTNULL;
}


// 0x10039e9e, 35 bytes -- vtable slot 2, lithtech 0x0048d070 (lt2new2.cpp
// "Slot10").
uint32 CWorldTree_E::DoSegQuery(WTSegQuery_E *pQuery)
{
	m_nCurCode = m_pCodeSource->NextCode();

	pQuery->m_pTree = this;

	return wt_SegQueryNode_E(&m_RootNode, pQuery);
}

// ==== END SECTION E =======================================================

// ==== SECTION F: 0x10039ec1 .. 0x1003a54f ================================
//
// The tail of the world-tree module: the rectangle walk, the point and
// segment queries (vtable 0x1004ea24 slots 3 and 4), the sphere query
// (slot 5), the layout loader and the root-list insert -- then, after the
// int3 object boundary at 0x1003a529, the first function of rezmgr.lib.
//
// The tree's vtable, 0x1004ea24, seven slots:
//   0 0x10039be9   1 0x10039e18 FindObjectsInBox(min, max, cb, user, list)
//   2 0x10039e9e FindObjectsInBox(info)   3 0x10039f82 FindObjectsOnPoint
//   4 0x10039f9d FindObjectsOnSegment     5 0x1003a386 FindObjectsInSphere
//   6 0x10039b9f `ret $8`
// Slots 4 and 5 are the client's 0x0048d210 / 0x0048d810
// (src/engine/lithtech/worldtree.cpp), and the node/object layouts are the
// client's (src/engine/lithtech/worldtree2.cpp) offset for offset. At /O1
// the segment test's helpers are NOT inlined, which is what splits the
// client's 0x0048d3a0/0x0048d500 into 0x1003a0e3, 0x1003a1d1, 0x1003a22b
// and 0x1003a28b here.

#include <math.h>                // fabs -- a CRT call at /O1 (0x1003e35b)
#include "ltlink.h"
#include "objtypes.h"            // CObjVisitor, CObjBase (0x1003a509)

class CWTNode_F;
class CWorldTree_F;

// The link an object is threaded into a node with (0x10039707's loop).
class CWTLink_F : public LTLink
{
public:
	CWTNode_F* m_pNode;                      // +0x0c
};

// The world-tree object base (0x10039707 is its constructor).
class CWTObj_F
{
public:
	virtual ~CWTObj_F() = 0;
	virtual LTBOOL _v04(void* p) = 0;
	virtual void   RemoveFromLists() = 0;                    // 0x10039740
	virtual void   GetDims(LTVector* pMin, LTVector* pMax) = 0;

	CWTLink_F m_Links[5];                    // +0x04
	uint32    m_nObjType;                    // +0x54
	uint32    m_nTouchCode;                  // +0x58
};

class CWTNode_F
{
public:
	void   SetDims(LTVector vMin, LTVector vMax);                   // 0x10039807
	LTBOOL LoadNodes(ILTStream* pStream, uint8* pCurByte,
	                 uint8* pCurBit);                               // 0x1003991d

	CheapLTLink m_Objects[3];                // +0x00
	LTVector    m_vBoxMin;                   // +0x18
	LTVector    m_vBoxMax;                   // +0x24
	LTVector    m_vCenter;                   // +0x30
	float       m_fSmallestDim;              // +0x3c
	float       m_fRadius;                   // +0x40
	uint32      m_nNumObjects;               // +0x44
	CWTNode_F*  m_pParent;                   // +0x48
	CWTNode_F*  m_Children[4];               // +0x4c
};

class IWTCodeSource_F
{
public:
	virtual uint32 NextCode() = 0;
};

typedef void   (*WTBoxCallback_F)(CWTObj_F* pObj, void* pUser);
typedef uint32 (*WTSegCallback_F)(CWTObj_F* pObj, void* pUser);
typedef void   (*WTNodeFn_F)(CWTNode_F* pNode, void* pUser);
typedef int    (*WTNodeTest_F)(CWTNode_F* pNode);

// 0x10039e61 builds this (0x34 bytes); 0x10039ec1 consumes it.
struct WTRectInfo_F
{
	CWorldTree_F*   m_pTree;                 // +0x00
	uint32          m_nListIndex;            // +0x04
	LTVector        m_vMin;                  // +0x08
	LTVector        m_vMax;                  // +0x14
	LTVector        m_v20;                   // +0x20
	WTBoxCallback_F m_pfnCallback;           // +0x2c
	void*           m_pUserData;             // +0x30
};

// 0x10039f9d builds this on the stack (0x28 bytes); 0x10039fef consumes it.
struct WTSegInfo_F
{
	CWorldTree_F*   m_pTree;                 // +0x00
	uint32          m_nListIndex;            // +0x04
	LTVector        m_vPt1;                  // +0x08
	LTVector        m_vPt2;                  // +0x14
	WTSegCallback_F m_pfnCallback;           // +0x20
	void*           m_pUserData;             // +0x24
};

// The sphere query's descriptor; the caller owns it.
struct WTSphereInfo_F
{
	uint32          m_nListIndex;            // +0x00
	LTVector        m_vCenter;               // +0x04
	float           m_fRadius;               // +0x10
	WTBoxCallback_F m_pfnCallback;           // +0x14
	uint32          m_n018;                  // +0x18
	void*           m_pUserData;             // +0x1c
	uint32          m_n020;                  // +0x20
	uint32          m_n024;                  // +0x24
	WTNodeTest_F    m_pfnNodeTest;           // +0x28
	LTVector        m_vBoxMin;               // +0x2c
	LTVector        m_vBoxMax;               // +0x38
	CWorldTree_F*   m_pTree;                 // +0x44
};

class CWorldTree_F
{
public:
	virtual void   _v00() = 0;                                      // 0x10039be9
	virtual void   FindObjectsInBox(LTVector* pMin, LTVector* pMax,
	                                WTBoxCallback_F pfnCallback,
	                                void* pUserData,
	                                uint32 nListIndex) = 0;         // 0x10039e18
	virtual void   FindObjectsInBox2(WTRectInfo_F* pInfo) = 0;      // 0x10039e9e
	virtual void   FindObjectsOnPoint(LTVector* pPt,
	                                  WTBoxCallback_F pfnCallback,
	                                  void* pUserData,
	                                  uint32 nListIndex);           // 0x10039f82
	virtual uint32 FindObjectsOnSegment(LTVector* pPt1, LTVector* pPt2,
	                                    WTSegCallback_F pfnCallback,
	                                    void* pUserData,
	                                    uint32 nListIndex);         // 0x10039f9d
	virtual void   FindObjectsInSphere(WTSphereInfo_F* pInfo);      // 0x1003a386
	virtual void   _v18(void* a, void* b) = 0;                      // 0x10039b9f

	LTBOOL LoadLayout(ILTStream* pStream);                          // 0x1003a473

	IWTCodeSource_F* m_pCodeSource;          // +0x04
	uint32           m_nCurCode;             // +0x08
	CWTNode_F        m_RootNode;             // +0x0c
	void*            m_p068;                 // +0x68
	void*            m_p06c;                 // +0x6c
	void*            m_p070;                 // +0x70
	CheapLTLink      m_RootObjects;          // +0x74
};

// Elsewhere in the module (sections D and E).
void wt_FindInBox_F(CWTNode_F* pNode, WTSphereInfo_F* pInfo, int nLevel);  // 0x100395a4
// 0x10039d98 is SECTION E's wt_ForEachChildInBox_E; the rectangle walk
// below calls it through E's node and callback types (the node is the same
// 0x5c-byte class under both spellings).
// commonmodel.cpp's three-float read.
void w_ReadVector(ILTStream* pStream, LTVector* pVec);                    // 0x1000d040

void   wt_FindInRect_F(CWTNode_F* pNode, WTRectInfo_F* pInfo);
uint32 wt_ProcessNodeSeg_F(CWTNode_F* pNode, WTSegInfo_F* pInfo);
uint32 wt_TestNodeSeg_F(CWTNode_F* pNode, WTSegInfo_F* pInfo);
LTBOOL wt_SegHitsRect_F(LTVector* pPt1, LTVector* pPt2, LTVector* pMin,
                        LTVector* pMax, uint32 nAxis1, uint32 nAxis2);
LTBOOL wt_SegHitsEdge_F(float a1, float a2, float fPlane,
                        float fMin, float fMax, float b1, float b2);
int    wt_DefaultNodeTest_F(CWTNode_F* pNode);
void   wt_WalkRootList_F(CheapLTLink* pHead, WTSphereInfo_F* pInfo);


// 0x10039ec1, 193 bytes -- the rectangle walk slot 2 (0x10039e9e) starts.
// cdecl; it hands itself to 0x10039d98 to recurse into the children.
void wt_FindInRect_F(CWTNode_F* pNode, WTRectInfo_F* pInfo)
{
	CheapLTLink* pHead;
	LTLink*      pCur;
	CWTObj_F*    pObj;
	LTVector     vMin, vMax;

	// AVP2: the box corners are named once and the code is read once,
	// before the node is even tested (Primal Hunt server.dll 0x1004a05d..
	// 0x1004a071, 235 bytes).
	uint32       nCode = pInfo->m_pTree->m_nCurCode;
	LTVector*    pMin  = &pInfo->m_vMin;
	LTVector*    pMax  = &pInfo->m_vMax;

	if (!pNode->m_nNumObjects)
		return;

	pHead = &pNode->m_Objects[pInfo->m_nListIndex];
	pCur  = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj  = (CWTObj_F*)pCur->m_pData;
		pCur  = pCur->m_pNext;

		if (pObj->m_nTouchCode != nCode)
		{
			pObj->m_nTouchCode = nCode;
			pObj->GetDims(&vMin, &vMax);

			if (vMin.x > pMax->x) continue;
			if (vMin.y > pMax->y) continue;
			if (vMin.z > pMax->z) continue;
			if (vMax.x < pMin->x) continue;
			if (vMax.y < pMin->y) continue;

			// AVP2: the last test is the positive one around the call: cl
			// 9044 keeps NaN apart where 8966 does not, and AvP2 1.0.9.6's
			// server.dll (0x10044ada / 0x1004540b `test $0x1; jne`) skips
			// the call on an unordered compare.
			if (vMax.z >= pMin->z)
				pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}

	if (pNode->m_Children[0])
	{
		wt_ForEachChildInBox_E(pMin, pMax, (CWTNode_E*)pNode,
		                       (WTNodeCallback_E)wt_FindInRect_F, pInfo);
	}
}


// 0x10039f82, 27 bytes -- slot 3: a point is a zero-length box (slot 1).
void CWorldTree_F::FindObjectsOnPoint(LTVector* pPt, WTBoxCallback_F pfnCallback,
                                      void* pUserData, uint32 nListIndex)
{
	FindObjectsInBox(pPt, pPt, pfnCallback, pUserData, nListIndex);
}


// 0x10039f9d, 82 bytes -- slot 4 (client 0x0048d210).
uint32 CWorldTree_F::FindObjectsOnSegment(LTVector* pPt1, LTVector* pPt2,
                                          WTSegCallback_F pfnCallback,
                                          void* pUserData, uint32 nListIndex)
{
	WTSegInfo_F info;

	m_nCurCode = m_pCodeSource->NextCode();

	info.m_pTree       = this;
	info.m_nListIndex  = nListIndex;
	info.m_vPt1        = *pPt1;
	info.m_vPt2        = *pPt2;
	info.m_pfnCallback = pfnCallback;
	info.m_pUserData   = pUserData;

	return wt_ProcessNodeSeg_F(&m_RootNode, &info);
}


// 0x10039fef, 244 bytes -- report a node's objects, then all four children
// nearest quadrant first (client 0x0048d290).
uint32 wt_ProcessNodeSeg_F(CWTNode_F* pNode, WTSegInfo_F* pInfo)
{
	uint32       nRet;
	CheapLTLink* pHead;
	LTLink*      pCur;
	CWTObj_F*    pObj;
	uint32       nCode;

	nRet = 0;

	pHead = &pNode->m_Objects[pInfo->m_nListIndex];
	pCur  = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj  = (CWTObj_F*)pCur->m_pData;
		pCur  = pCur->m_pNext;
		nCode = pInfo->m_pTree->m_nCurCode;

		if (pObj->m_nTouchCode != nCode)
		{
			pObj->m_nTouchCode = nCode;
			nRet |= pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}

	if (pNode->m_Children[0])
	{
		uint32 nX, nZ, nHit;

		nX = (pInfo->m_vPt1.x > pNode->m_vCenter.x) ? 1 : 0;
		nZ = (pInfo->m_vPt1.z > pNode->m_vCenter.z) ? 1 : 0;

		nHit  = wt_TestNodeSeg_F(pNode->m_Children[nX * 2 + nZ], pInfo);
		nHit |= wt_TestNodeSeg_F(pNode->m_Children[(nX == 0) * 2 + nZ], pInfo);
		nHit |= wt_TestNodeSeg_F(pNode->m_Children[nX * 2 + (nZ == 0)], pInfo);
		nHit |= wt_TestNodeSeg_F(pNode->m_Children[(nX == 0) * 2 + (nZ == 0)], pInfo);

		if (nHit)
			return LTTRUE;
	}

	return nRet;
}


// Inline (no address of its own): is a point inside a rectangle in X/Z.
inline LTBOOL wt_PtInRectXZ_F(LTVector* pPt, LTVector* pMin, LTVector* pMax)
{
	return pPt->x >= pMin->x && pPt->z >= pMin->z &&
	       pPt->x <= pMax->x && pPt->z <= pMax->z;
}


// AVP2: 0 if both endpoints are below the range, 1 if both are above it,
// 2 otherwise -- now over the coordinates themselves and expanded even at
// /O1 (Primal Hunt server.dll 0x1004a313..0x1004a39b, the range's minimum
// homed to a frame slot); NOLF's was the out-of-line 0x1003a1d1 over the
// vectors and an axis.  The client's twin is worldtree.cpp's
// wt_ClassifySpanQ522 (name ours).
inline uint32 wt_ClassifySpan_F(float f1, float f2, float fMin, float fMax)
{
	if (f1 < fMin && f2 < fMin)
		return 0;

	if (f1 > fMax && f2 > fMax)
		return 1;

	return 2;
}


// 0x1003a0e3, 238 bytes -- does the segment touch this node at all
// (client 0x0048d3a0, with its helpers out of line).  AVP2: PH 0x1004a29f,
// 338 bytes, with the side codes expanded and X's third value folded into
// the jump to the exact test.
uint32 wt_TestNodeSeg_F(CWTNode_F* pNode, WTSegInfo_F* pInfo)
{
	uint32 nSide;

	if (wt_PtInRectXZ_F(&pInfo->m_vPt1, &pNode->m_vBoxMin, &pNode->m_vBoxMax) ||
	    wt_PtInRectXZ_F(&pInfo->m_vPt2, &pNode->m_vBoxMin, &pNode->m_vBoxMax))
	{
		return wt_ProcessNodeSeg_F(pNode, pInfo);
	}

	nSide = wt_ClassifySpan_F(pInfo->m_vPt1.x, pInfo->m_vPt2.x,
	                          pNode->m_vBoxMin.x, pNode->m_vBoxMax.x);
	if (nSide != 2 &&
	    nSide == wt_ClassifySpan_F(pInfo->m_vPt1.z, pInfo->m_vPt2.z,
	                               pNode->m_vBoxMin.z, pNode->m_vBoxMax.z))
	{
		return 0;
	}

	if (wt_SegHitsRect_F(&pInfo->m_vPt1, &pInfo->m_vPt2,
	                     &pNode->m_vBoxMin, &pNode->m_vBoxMax, 0, 2) ||
	    wt_SegHitsRect_F(&pInfo->m_vPt1, &pInfo->m_vPt2,
	                     &pNode->m_vBoxMin, &pNode->m_vBoxMax, 2, 0))
	{
		return wt_ProcessNodeSeg_F(pNode, pInfo);
	}

	return 0;
}


// 0x1003a22b, 96 bytes -- either of the two nAxis1 planes.  AVP2: PH
// 0x1004a3f1, 202 bytes: each plane is now the scalar edge test below,
// handed the plane, the nAxis2 range and both endpoints' nAxis2
// coordinates by value.
LTBOOL wt_SegHitsRect_F(LTVector* pPt1, LTVector* pPt2, LTVector* pMin,
                        LTVector* pMax, uint32 nAxis1, uint32 nAxis2)
{
	if (wt_SegHitsEdge_F(((float*)pPt1)[nAxis1], ((float*)pPt2)[nAxis1],
	                     ((float*)pMin)[nAxis1],
	                     ((float*)pMin)[nAxis2], ((float*)pMax)[nAxis2],
	                     ((float*)pPt1)[nAxis2], ((float*)pPt2)[nAxis2]))
		return LTTRUE;

	if (wt_SegHitsEdge_F(((float*)pPt1)[nAxis1], ((float*)pPt2)[nAxis1],
	                     ((float*)pMax)[nAxis1],
	                     ((float*)pMin)[nAxis2], ((float*)pMax)[nAxis2],
	                     ((float*)pPt1)[nAxis2], ((float*)pPt2)[nAxis2]))
		return LTTRUE;

	return LTFALSE;
}


// AVP2: PH 0x1004a4bb, 127 bytes, cdecl, seven floats -- does the segment
// (a1,b1)-(a2,b2) cross the line a = fPlane between fMin and fMax on b?
// The client's twin is lithtech.exe 0x004aa780 (worldtree.cpp's
// wt_SegHitsEdgeQ522).  Replaces NOLF's 0x1003a28b, which lerped whole
// vectors (name ours).
LTBOOL wt_SegHitsEdge_F(float a1, float a2, float fPlane,
                        float fMin, float fMax, float b1, float b2)
{
	LTBOOL bSide1 = (a1 < fPlane);
	LTBOOL bSide2 = (a2 <= fPlane);

	if (bSide1 == bSide2)
		return LTFALSE;

	float fDiff = a2 - a1;
	float fB;

	if (fabs(fDiff) < 0.001f)
	{
		fB = b1;
	}
	else
	{
		float fPercent = (fPlane - a1) * (b2 - b1);
		fB = b1 + fPercent / fDiff;
	}

	if (fB >= fMin && fB <= fMax)
		return LTTRUE;

	return LTFALSE;
}


// 0x1003a386, 178 bytes -- slot 5 (client 0x0048d810).
void CWorldTree_F::FindObjectsInSphere(WTSphereInfo_F* pInfo)
{
	m_nCurCode = m_pCodeSource->NextCode();

	pInfo->m_pTree = this;

	pInfo->m_vBoxMin = pInfo->m_vCenter -
		LTVector(pInfo->m_fRadius, pInfo->m_fRadius, pInfo->m_fRadius);
	pInfo->m_vBoxMax = pInfo->m_vCenter +
		LTVector(pInfo->m_fRadius, pInfo->m_fRadius, pInfo->m_fRadius);

	if (!pInfo->m_pfnNodeTest)
		pInfo->m_pfnNodeTest = wt_DefaultNodeTest_F;

	wt_FindInBox_F(&m_RootNode, pInfo, 0);

	wt_WalkRootList_F(&m_RootObjects, pInfo);
}


// 0x1003a438, 4 bytes (small) -- the default node test. Also referenced
// from 0x10026eba, so it is not static.
int wt_DefaultNodeTest_F(CWTNode_F* pNode)
{
	return 1;
}


// 0x1003a43c, 55 bytes -- the tree's own root list, box-query style.
void wt_WalkRootList_F(CheapLTLink* pHead, WTSphereInfo_F* pInfo)
{
	LTLink*   pCur;
	CWTObj_F* pObj;
	uint32    nCode;

	pCur = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj  = (CWTObj_F*)pCur->m_pData;
		pCur  = pCur->m_pNext;
		nCode = pInfo->m_pTree->m_nCurCode;

		if (pObj->m_nTouchCode != nCode)
		{
			pObj->m_nTouchCode = nCode;
			pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}
}


// 0x1003a473, 150 bytes -- read the tree's layout (client 0x0048d9a0).
// commonmodel.cpp calls this as CWorldSub6C::sub_1003a473: that TU-local
// class is this tree; the definition here is spelled on CWorldTree_F.
LTBOOL CWorldTree_F::LoadLayout(ILTStream* pStream)
{
	LTVector vMin, vMax;
	uint32   nUnused;
	uint8    nCurByte, nCurBit;

	w_ReadVector(pStream, &vMin);
	w_ReadVector(pStream, &vMax);

	pStream->Read(&nUnused, 4);
	pStream->Read(&m_p070, 4);

	nCurByte = 0;
	nCurBit  = 8;

	m_RootNode.SetDims(vMin, vMax);

	if (m_RootNode.LoadNodes(pStream, &nCurByte, &nCurBit) &&
	    pStream->ErrorStatus() == LT_OK)
		return LTTRUE;

	return LTFALSE;
}


// 0x1003a509, 32 bytes -- objtypes.h's CObjVisitor::Visit: the visitor is
// the world tree and "visiting" links the object's fifth link into the
// tree's root list (client 0x0048db00).
void CObjVisitor::Visit(CObjBase* pObj)
{
	CWTLink_F* pLink = &((CWTObj_F*)pObj)->m_Links[4];

	((CWorldTree_F*)this)->m_RootObjects.AddAfter(pLink);
	pLink->m_pNode = LTNULL;
}


// 0x1003a530, 26 bytes -- NOT THIS MODULE: rezmgr.lib's CRezItm::CRezItm
// (lithtech 0x0048df10; src/lithshared/rezmgr/rezmgr.cpp:412 declares it
// and leaves it undefined; the client's copy is ltnew1.cpp's
// CSub48df10View). Its real home is src/lithshared/rezmgr/rezmgr.cpp.
class CBaseListItem_F
{
protected:
	CBaseListItem_F* m_pNext;                // +0x00
	CBaseListItem_F* m_pPrev;                // +0x04
};

class CBaseHashItem_F : public CBaseListItem_F
{
protected:
	virtual unsigned int HashFunc() = 0;

	void*        m_pParentHash;              // +0x0c
	unsigned int m_nCurBin;                  // +0x10
};

class CRezItm_F;

class CRezItmHashByName_F : public CBaseHashItem_F
{
public:
	CRezItmHashByName_F() { m_pRezItm = NULL; }
	void SetRezItm(CRezItm_F* pRezItm) { m_pRezItm = pRezItm; }

protected:
	virtual unsigned int HashFunc();         // 0x1003c650

	CRezItm_F* m_pRezItm;                    // +0x14
};

class CRezItm_F
{
public:
	CRezItm_F();

	char*               m_sName;             // 0x000
	void*               m_pType;             // 0x004
	uint32              m_nTime;             // 0x008
	uint32              m_nSize;             // 0x00c
	void*               m_pParentDir;        // 0x010
	uint32              m_nFilePos;          // 0x014
	uint32              m_nCurPos;           // 0x018
	CRezItmHashByName_F m_heName;            // 0x01c
	void*               m_pRezFile;          // 0x034
	uint8*              m_pData;             // 0x038
};

// PH: Primal Hunt's server.dll links the debug RezMgr.lib, whose
// rezmgr.obj defines CRezItm::CRezItm itself (0x1004a6e0, identical): the
// stand-in is AvP2's only, whose release library leaves it undefined.
#ifndef PRIMAL_HUNT
CRezItm_F::CRezItm_F()
{
	m_pRezFile   = NULL;
	m_pParentDir = NULL;
	m_sName      = NULL;
	m_heName.SetRezItm(this);
}
#endif

// ==== END SECTION F =======================================================
