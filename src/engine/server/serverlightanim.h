// *********************************************************************** //
//  MODULE:   SERVERLIGHTANIM (.H)
//  PURPOSE:  ServerLightAnimLT — the server's ILTLightAnim, at
//            0x1002df1e-0x1002e071. Light animations are baked by the
//            preprocessor into one array on the server; the interface is
//            three bounds-checked accessors over it.
// *********************************************************************** //
#ifndef __SERVERLIGHTANIM_H__
#define __SERVERLIGHTANIM_H__

#include "iltlightanim.h"
#include "servermgr.h"

// Light-animation record helpers (0x100129da, 0x10012ac3, 0x10012a1c).
void   la_GetInfo(LightAnim* pAnim, LAInfo* pInfo);
LTBOOL la_CompareInfo(LightAnim* pAnim, LAInfo* pInfo, uint32* pChangeFlags);
void   la_SetInfo(LightAnim* pAnim, LAInfo* pInfo);

// 0x10012a59 — do two vectors differ once each component is quantized by the
// same scale? Component by component, `(int)(a->x * fScale)` against
// `(int)(b->x * fScale)`, returning 1 at the first that differs and 0 if all
// three agree. The scale is a *double* (`fmull 0x10(%ebp)`, and the caller
// pushes eight bytes), which is what makes it one helper for the position at
// ten units and the colour at 255. Name ours.
int la_QuantizedVecDiffers(LTVector* pA, LTVector* pB, double fScale);

// One queued light-animation change on a client: which animation, and what
// about it changed. Four bytes, both halves sixteen bits — the stores are
// `mov %cx,0xc(%esi,%eax,4)` and `mov %bx,0xe(...)`, so the stride is 4 and
// the two fields sit at +0 and +2 (names ours).
class CLAChange
{
public:
	uint16 m_hLightAnim;                     // +0x00
	uint16 m_ChangeFlags;                    // +0x02
};

// 0x40 of them: the array runs from Client+0x00c (just past m_Link) to the
// count at Client+0x10c, which is 0x100 bytes — sixty-four records. Client
// itself lives in servermgr.h, which this reconstruction does not own, and
// maps the whole span as padding plus m_Unknown10C; reached here through a
// cast (names ours).
#define MAX_CLIENTLACHANGES 0x40

class CLAClient
{
public:
	uint8     m_pad_000[0x00c];
	CLAChange m_Changes[MAX_CLIENTLACHANGES];  // 0x00c
	uint32    m_nChanges;                      // 0x10c
};

// 0x1002896d — find a client's queued change for one animation, by animation
// id, handing back its index. Returns 1 if it found one and 0 if it did not,
// and writes the index only on a hit. The compare is sixteen bits wide on
// both sides (`mov (%eax),%si; cmp 0xc(%esp),%si`), so the id parameter is a
// uint16 and not the uint32 HLIGHTANIM the caller holds (name ours).
int la_FindClientChange(class Client* pClient, uint32 hLightAnim, uint32* pnIndex);

// 0x100289a2 — tell the clients a light animation changed.
void sm_LightAnimChanged(CServerMgr* pServerMgr, HLIGHTANIM hLightAnim, uint32 dwChangeFlags);


class ServerLightAnimLT : public ILTLightAnim
{
public:
	// In-class: CLTServer's constructor expands it (0x1002d3f4).
	ServerLightAnimLT(CServerMgr* pServerMgr) { m_pServerMgr = pServerMgr; }

	LTRESULT FindLightAnim(const char* pName, HLIGHTANIM& hLightAnim);
	LTRESULT GetNumFrames(HLIGHTANIM hLightAnim, uint32& nFrames);
	LTRESULT GetLightAnimInfo(HLIGHTANIM hLightAnim, LAInfo& info);
	LTRESULT SetLightAnimInfo(HLIGHTANIM hLightAnim, LAInfo& info);

public:
	CServerMgr* m_pServerMgr;                // 0x04 (0x00 is the vtable)
};

#endif
