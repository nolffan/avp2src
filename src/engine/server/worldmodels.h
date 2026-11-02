// *********************************************************************** //
//  MODULE:   WORLDMODELS (.H)
//  PURPOSE:  The world's list of world models, at 0x1000a226-0x1000a663.
//            Each entry carries two pieces of geometry — the model as it was
//            authored and the model as it stands — and the searches here run
//            over the list rather than over any index, so the list is short
//            by construction.
//
//            Class and field names are ours; nothing in the image names them.
// *********************************************************************** //
#ifndef __WORLDMODELS_H__
#define __WORLDMODELS_H__

#include "ltbasedefs.h"
#include "servermgr.h"

// The geometry one world model is made of. Only the flags and the poly run
// are reached from the searches here.
class CWorldModelData
{
public:
	uint8   m_pad_00[0x48];
	uint32  m_Flags;                         // 0x48
	uint8   m_pad_4c[0xa0 - 0x4c];
	void**  m_pPolies;                       // 0xa0
	uint32  m_nPolies;                       // 0xa4
};

// One entry: a name, and the two pieces of geometry.
class CWorldModelEntry
{
public:
	char*            m_pName;                // 0x00
	CWorldModelData* m_pData;                // 0x04
	CWorldModelData* m_pOriginal;            // 0x08
};

// What a poly is named by: which world model, and which poly within it.
class CWorldPolyKey
{
public:
	uint16 m_iModel;                         // 0x00
	uint16 m_iPoly;                          // 0x02
};

// 0x1000a411 — put one world model's geometry back the way it was loaded
// (name ours).
void wm_ResetGeometry(CWorldModelData* pData);

class CWorldModelList
{
public:
	// 0x1000a2b0 — the poly a key names, or nothing if either half of the
	// key is out of range.
	void* FindPoly(CWorldPolyKey* pKey);

	// 0x1000a37c and 0x1000a3a6 — the first world model carrying a given
	// flag, or nothing. Two flags are asked about and each has its own
	// walk, which is why the two functions are the same but for one bit.
	CWorldModelData* FindFlagged20();
	CWorldModelData* FindFlagged10();

	// 0x1000a3d0 — put every world model's geometry back, both copies.
	void ResetAll();

public:
	uint8              m_pad_000[0x18c];
	CWorldModelEntry** m_ppEntries;          // 0x18c
	uint32             m_nEntries;           // 0x190
};

#endif  // __WORLDMODELS_H__
