// *********************************************************************** //
//
//  MODULE:   N48D210 (.CPP)  -- staging translation unit (file name ours)
//
//  WHAT THIS IS: the CLIENT's WORLD OBJECT TREE -- a four-child quadtree
//  over X/Z that every object in the world is linked into, and the queries
//  that walk it.  Nine enumerated rows, 0x0048d210..0x0048de00.  EIGHT of them
//  are that tree's code; the ninth (0x0048db40) is not code at all, see the
//  extent note at the foot of this file.
//
//  GROUND TRUTH: the retail lithtech.exe, cl 8447, /O2 (so /Oy: frameless,
//  %esp-relative).  Every offset, constant and branch below was read off that
//  image and is cited by address.  No build of ours was read.
//
//  HOW THE MODULE WAS IDENTIFIED
//  ----------------------------
//  1. THE VTABLE.  0x004a38f8 is written as a vptr immediate, once, at
//     0x0048ccef -- the constructor at 0x0048cce0 that src/engine/lithtech/
//     lt2new4.cpp already wrote up.  It is SEVEN slots wide, 0x004a38f8..
//     0x004a3910, and five of the seven are in or beside this range:
//
//         slot 0  0x0048cd80
//         slot 1  0x0048cff0
//         slot 2  0x0048d070   (written: lt2new2.cpp, as "Slot10")
//         slot 3  0x0048d1f0   (written: ltnew2.cpp,  as "Slot11")
//         slot 4  0x0048d210   *** this file ***
//         slot 5  0x0048d810   *** this file ***
//         slot 6  0x0048cd10   `ret $8`, an empty two-argument override
//
//     (ltnew2.cpp/lt2new2.cpp indexed these off 0x004a38d8 and called them
//     slots 10 and 11.  That is wrong by two slots: 0x004a38d8 is a
//     DIFFERENT, EIGHT-slot vtable whose vptr immediate is written at
//     0x0048c6e9 and 0x0048c722, and whose slot 3 is 0x004928e7 -- which is
//     `push $0x19; call 0x00494dc9`, i.e. _purecall / _amsg_exit(_RT_PUREVIRT).
//     Two vtables cannot overlap; the pair of `ret $4` bodies at 0x004a38f0
//     and 0x004a38f4 end the first one.)
//
//  2. THE OBJECT LAYOUT, read straight off the bytes.  The constructor at
//     0x0048cce0 runs 0x0048c780 on this+0x0c BEFORE storing the vptr, so
//     this+0x0c is a base; 0x0048c780's object is 0x5c bytes (0x0c..0x67).
//     0x0048d210 hands this+0x0c to 0x0048d290 as the tree's ROOT NODE, and
//     0x0048d290 reads, off that pointer:
//
//         +0x00  CheapLTLink m_Objects[3]  (`lea (%ebx,%eax,8)`, 0x0048d2ac)
//         +0x18  LTVector    m_vBoxMin     (0x0048d3ad, 0x0048d3ef)
//         +0x24  LTVector    m_vBoxMax     (0x0048d3b8 `lea 0x24(%ecx),%ebp`)
//         +0x30  LTVector    m_vCenter     (0x0048d2f1 x, 0x0048d30a z)
//         +0x4c  node*       m_Children[4] (0x0048d322, and the `cmp $4`
//                                           loop at 0x0048daec)
//
//     0x18 + 0x0c + 0x0c + 0x0c = 0x3c, children at 0x4c, four of them =
//     0x5c exactly.  A quadtree that splits on X and Z with a 3-wide array
//     of per-type object lists: this is LithTech's world tree.
//
//  3. WHAT THE QUERIES ARE.  0x0048d210 builds a 0x28-byte descriptor on the
//     stack holding two LTVectors, a callback and a user pointer, and walks
//     the four children NEAR-TO-FAR from the quadrant that contains the first
//     point (0x0048d31e / 0x0048d340 / 0x0048d35c / 0x0048d373) -- a ray or
//     segment query.  0x0048d810 turns a centre and a radius into a box and
//     hands it to 0x0048c540 -- the box/sphere query.  0x0048d500 is the
//     2-D segment-versus-rectangle primitive they share.  0x0048d9a0 reads
//     the tree's bounds off an ILTStream (its Read is vtable slot 2 and its
//     ErrorStatus slot 4, which is iltstream.h's layout to the slot).
//
//  4. NOT SHARED WITH server.dll.  The server has no such class: its
//     equivalent, sm_RemoveFromWorldTree (src/engine/server/smgr.cpp:4533),
//     dispatches through g_ObjTypeHandlers, one handler per object type.
//     Nothing in src/engine/ or src/lithshared/ mentions any of these eight
//     addresses, and nothing of ours matches on them.
//     ==> NOT already reconstructed.
//
//  5. WHERE THE MODULE STOPS.  0x0048db00 is the last of its functions.
//     From 0x0048db30 the image is LIBRARY material, not engine source:
//     0x0048db30 is `jmp *0x004a1000` (DINPUT.dll!DirectInputCreateA),
//     0x0048db40 is dinput.lib's c_dfDIJoystick object table (see below),
//     0x0048de10/0x0048de20 are an mss32 thunk pair and 0x0048de00 the
//     atexit() registration that arms it.  0x0048de30..0x0048df10 are
//     CheapLTLink/LTLink member functions out of ltlink.h.  The 50-function
//     run is NOT one module; it has at least this library seam in it.
//
//  NAMING: everything here carries the suffix N48D so it cannot collide with
//  the sibling chunks or with src/engine/lithtech.  The `wt_` prefix is ours.
//
// *********************************************************************** //

#include <math.h>                // fabs -- an intrinsic under /O2 (/Oi)

#include "ltbasedefs.h"          // uint32, uint8, LTBOOL, LTTRUE, LTNULL
#include "ltcodes.h"             // LT_OK
#include "ltlink.h"              // CheapLTLink / LTLink
#include "iltstream.h"           // ILTStream: Read = slot 2, ErrorStatus = 4


// ======================================================================= //
//  THE TYPES THESE EIGHT FUNCTIONS REACH
// ======================================================================= //

// ltvector.h's LTVector (_CVector<float>), redeclared for the reason
// src/engine/lithtech/clientshell.cpp and soundinst2.cpp redeclare it: the
// three-float constructor in this build is OUT OF LINE at 0x00411d40 (25
// bytes, thiscall, `ret $0xc`, returns `this`), and 0x0048d500 CALLS it six
// times (0x0048d5c4, 0x0048d605, 0x0048d642 and again in the second half at
// 0x0048d739, 0x0048d77a, 0x0048d7b7).  Spelling it as ltvector.h's inline
// constructor would lose all six calls.
//
// The operator bodies are ltvector.h's, verbatim, including the BY-VALUE
// parameter: that copy is what 0x0048d58a..0x0048d59f is -- `mov %esi,%ecx`
// then three loads into a 12-byte stack slot, before a single float of the
// subtraction is touched.  Init() is ltvector.h's too, and it is what
// 0x0048d810 uses: the three floats land in stack temporaries and are then
// moved to the destination AS INTEGERS (`fstps 0x8(%esp)` ... `mov
// 0x8(%esp),%edx` ... `mov %edx,(%ecx)`, 0x0048d841..0x0048d861), which is
// exactly an inlined `x = px` on a float parameter and NOT a `fstps` store.
class CWTVecN48D
{
public:
	CWTVecN48D() {}
	CWTVecN48D(float mx, float my, float mz);              // 0x00411d40

	void Init(float px, float py, float pz) { x = px; y = py; z = pz; }

	CWTVecN48D operator + (const CWTVecN48D v) const
		{ return CWTVecN48D(x + v.x, y + v.y, z + v.z); }
	CWTVecN48D operator - (const CWTVecN48D v) const
		{ return CWTVecN48D(x - v.x, y - v.y, z - v.z); }
	CWTVecN48D operator * (float v) const
		{ return CWTVecN48D(x * v, y * v, z * v); }

	float x, y, z;
};


// What the object lists hold.  The ONE field either walk reads is +0x58, the
// per-query stamp that keeps an object from being reported twice when it is
// linked into more than one node (0x0048d2be/0x0048d2c5 and
// 0x0048d90e/0x0048d915).
class CWTObjectN48D
{
public:
	uint8  m_pad_000[0x58];
	uint32 m_nTouchCode;                     // 0x58
};


// The quadtree node.  Sizes and offsets are argued in the banner; m_pad_03c
// is the sixteen bytes between the centre and the child array that none of
// these eight functions reads.
class CWTNodeN48D
{
public:
	// Reached, all in the module but outside this range.
	LTBOOL AllocChildren();                                       // 0x0048ca50
	void   SetDims(CWTVecN48D vMin, CWTVecN48D vMax);             // 0x0048c830
	LTBOOL LoadNodes(ILTStream *pStream, uint8 *pDepth,
	                 uint8 *pMaxDepth);                           // 0x0048c9b0

public:
	CheapLTLink  m_Objects[3];               // 0x00  indexed by *8
	CWTVecN48D   m_vBoxMin;                  // 0x18
	CWTVecN48D   m_vBoxMax;                  // 0x24
	CWTVecN48D   m_vCenter;                  // 0x30
	uint8        m_pad_03c[0x4c - 0x3c];     // 0x3c
	CWTNodeN48D* m_Children[4];              // 0x4c
};                                           // 0x5c


// The segment-query descriptor 0x0048d210 builds at %esp+4 and 0x0048d290
// consumes.  Every offset is a store in 0x0048d210 or a load in 0x0048d290.
typedef uint32 (*WTSegCallbackN48D)(CWTObjectN48D *pObj, void *pUser);

class CWTSegInfoN48D
{
public:
	class CWorldTreeN48D* m_pTree;           // 0x00  0x0048d22e / 0x0048d2b6
	uint32                m_nLinkIndex;      // 0x04  0x0048d228 / 0x0048d2a5
	CWTVecN48D            m_vPt1;            // 0x08
	CWTVecN48D            m_vPt2;            // 0x14
	WTSegCallbackN48D     m_pfnCallback;     // 0x20  `call *0x20(%esi)`
	void*                 m_pUserData;       // 0x24  `mov 0x24(%esi),%edx`
};


// The box/sphere-query descriptor 0x0048d810 fills in.  The caller owns it;
// 0x0048d810 only completes it.  m_pfnTest is defaulted to 0x00463d20 when
// the caller left it null (0x0048d8b6..0x0048d8c4).
typedef void (*WTBoxCallbackN48D)(CWTObjectN48D *pObj, void *pUser);

class CWTBoxInfoN48D
{
public:
	uint32                m_nLinkIndex;      // 0x00  (not read here)
	LTVector              m_vCenter;         // 0x04
	float                 m_fRadius;         // 0x10
	WTBoxCallbackN48D     m_pfnCallback;     // 0x14  `call *0x14(%edi)`
	uint32                m_n018;            // 0x18
	void*                 m_pUserData;       // 0x1c
	uint32                m_n020;            // 0x20
	uint32                m_n024;            // 0x24
	void*                 m_pfnTest;         // 0x28  defaulted to 0x00463d20
	LTVector              m_vBoxMin;         // 0x2c
	LTVector              m_vBoxMax;         // 0x38
	class CWorldTreeN48D* m_pTree;           // 0x44  `mov %edi,0x44(%esi)`
};


// The query slot 1 (0x0048cff0) builds on its stack and slot 2 (0x0048d070,
// lt2new2.cpp) completes.  Layout from 0x0048cff0's stores; server.dll's
// WTSegQuery_E (src/engine/server/worldtree.cpp), whose out-of-line
// constructor 0x10039e61 is the one inlined here: of its zero stores only
// +0x00 and m_v20 survive, the rest are overwritten.
struct CWTSegQueryN48D
{
	CWTSegQueryN48D()
	{
		m_pTree      = LTNULL;
		m_nListIndex = 0;
		m_vPt1.Init();
		m_vPt2.Init();
		m_v20.Init();
		m_pfnCallback = LTNULL;
		m_pUserData   = LTNULL;
	}

	class CWorldTreeN48D* m_pTree;           // +0x00  set by slot 2
	uint32                m_nListIndex;      // +0x04
	LTVector              m_vPt1;            // +0x08
	LTVector              m_vPt2;            // +0x14
	LTVector              m_v20;             // +0x20
	void*                 m_pfnCallback;     // +0x2c
	void*                 m_pUserData;       // +0x30
};


// Whatever hands out the per-query stamp.  Only slot 0 is reached, thiscall,
// no arguments, uint32 back (0x0048d219/0x0048d81a).
class IWTCodeSourceN48D
{
public:
	virtual uint32 NextCode() = 0;
};


// The tree itself.  vptr at +0x00 = 0x004a38f8; m_pCodeSource and m_nCurCode
// are the two dwords the constructor at 0x0048cce0 clears before the root
// node, which starts at +0x0c.
class CWorldTreeN48D
{
public:
	// ---- vtable 0x004a38f8 ------------------------------------------- //
	virtual void   _v000() = 0;                            // 0x0048cd80
	// slot 1 -- 0x0048cff0, this file
	virtual uint32 FindObjectsInBoxQ(LTVector *pMin, LTVector *pMax,
	                                 void *pfnCallback, void *pUserData,
	                                 uint32 nListIndex);
	virtual uint32 _v008(struct CWTSegQueryN48D *pQuery) = 0; // 0x0048d070
	virtual uint32 _v00c(void *a, void *b, void *c,
	                     void *d) = 0;                     // 0x0048d1f0

	// slot 4 -- 0x0048d210, this file
	virtual uint32 FindObjectsOnSegment(CWTVecN48D *pPt1, CWTVecN48D *pPt2,
	                                    WTSegCallbackN48D pfnCallback,
	                                    void *pUserData, uint32 nLinkIndex);

	// slot 5 -- 0x0048d810, this file
	virtual void   FindObjectsInBox(CWTBoxInfoN48D *pInfo);

	virtual void   _v018(void *a, void *b) = 0;            // 0x0048cd10

	// ---- non-virtual members ----------------------------------------- //
	void   Term();                                         // 0x0048cd50
	void   CopyFrom(CWorldTreeN48D *pOther);               // 0x0048d930, here
	LTBOOL Load(ILTStream *pStream);                       // 0x0048d9a0, here
	LTBOOL CopyNodes(CWTNodeN48D *pDest, CWTNodeN48D *pSrc);
	                                                       // 0x0048daa0, here

public:
	IWTCodeSourceN48D* m_pCodeSource;        // 0x04
	uint32             m_nCurCode;           // 0x08
	CWTNodeN48D        m_RootNode;           // 0x0c
	void*              m_p068;               // 0x68
	void*              m_p06c;               // 0x6c
	void*              m_p070;               // 0x70
	CheapLTLink        m_RootObjects;        // 0x74
};


// ---- the rest of the module, reached but outside this range ----------- //

// 0x0048c540 -- the box walk 0x0048d810 starts.  cdecl, three arguments
// (0x0048d8c4: `push $0x0; push %esi; push %edx`, and the 0x14 that comes
// off %esp at 0x0048d8da covers this call's three and the next call's two).
void wt_FindInBoxN48D(CWTNodeN48D *pNode, CWTBoxInfoN48D *pInfo, int nFlag);

// 0x0048d8f0 -- the same object walk 0x0048d290 does, over the tree's own
// root list instead of a node's.  cdecl, two arguments.  DEFINED BELOW
// 0x0048d810 (item 5b): a definition above it would let /Ob2 inline it
// there and lose the call.
void wt_WalkRootListN48D(CheapLTLink *pHead, CWTBoxInfoN48D *pInfo);

// 0x00463d20 -- what 0x0048d810 installs in m_pfnTest when the caller left
// it null.  Only its address is used.  The whole body is `mov $1,%eax; ret`
// (accept everything); defined at the end of the file.
LTBOOL wt_DefaultTestN48D();


// ---- the two halves of the segment walk, mutually recursive ----------- //

uint32 wt_ProcessNodeSegN48D(CWTNodeN48D *pNode, CWTSegInfoN48D *pInfo);
uint32 wt_TestNodeSegN48D(CWTNodeN48D *pNode, CWTSegInfoN48D *pInfo);
LTBOOL wt_SegmentHitsRectN48D(const CWTVecN48D *pPt1, const CWTVecN48D *pPt2,
                              const CWTVecN48D *pMin, const CWTVecN48D *pMax,
                              uint32 nAxis1, uint32 nAxis2);


// ======================================================================= //
//  0.  0x0048cff0, 123 bytes -- vtable 0x004a38f8 slot 1.
//
//  thiscall, `ret $0x14`: five stack arguments.  A 0x34-byte query on the
//  stack (the inlined constructor's surviving zeros at +0x00 and +0x20..
//  +0x28), the arguments copied in, then a VIRTUAL call through slot 2
//  (`mov (%ecx),%edx ... call *0x8(%edx)`, 0x0048d062) whose result is
//  returned.  server.dll's twin is 0x10039e18.
// ======================================================================= //
uint32 CWorldTreeN48D::FindObjectsInBoxQ(LTVector *pMin, LTVector *pMax,
                                         void *pfnCallback, void *pUserData,
                                         uint32 nListIndex)
{
	CWTSegQueryN48D query;

	query.m_nListIndex  = nListIndex;
	query.m_vPt1        = *pMin;
	query.m_vPt2        = *pMax;
	query.m_pfnCallback = pfnCallback;
	query.m_pUserData   = pUserData;

	return _v008(&query);
}


// ======================================================================= //
//  1.  0x0048d210, 116 bytes -- vtable 0x004a38f8 slot 4.
//      "walk the tree along a segment"
//
//  thiscall, `ret $0x14` at 0x0048d281, so five stack arguments on top of
//  %ecx.  The whole body is one 0x28-byte descriptor filled in on the stack
//  and handed, with the root node, to 0x0048d290.
//
//  WHAT EACH STORE IS (the descriptor sits at %esp+4; `sub $0x28` +
//  `push %esi` puts the frame at orig-0x2c, so %esp+0x30.. are the
//  arguments):
//
//    0x0048d216  m_pCodeSource (+0x04) -> `mov (%ecx),%eax; call *(%eax)`,
//                slot 0, no arguments; the uint32 back goes to m_nCurCode
//                (+0x08) at 0x0048d225.  That is the per-query stamp
//                0x0048d290 compares each object's +0x58 against.
//    0x0048d228  descriptor +0x04 = arg5   (the link-array index)
//    0x0048d22e  descriptor +0x00 = this
//    0x0048d232  +0x08/+0x0c/+0x10 = *arg1, three dword moves
//    0x0048d249  +0x14/+0x18/+0x1c = *arg2, three dword moves
//    0x0048d26d  +0x20 = arg3, +0x24 = arg4
//    0x0048d23d  `add $0xc,%esi` -- the root node is this+0x0c, i.e. the
//                base sub-object, and it is pushed at 0x0048d26c.
//
//  The two vector copies are INTERLEAVED in the shipped schedule (+0x14 is
//  written at 0x0048d249, before +0x10 at 0x0048d24d).  That is scheduling
//  inside one basic block, not two statements out of order.
//
//  The return value of 0x0048d290 is left in %eax and returned.
// ======================================================================= //
uint32 CWorldTreeN48D::FindObjectsOnSegment(CWTVecN48D *pPt1, CWTVecN48D *pPt2,
                                           WTSegCallbackN48D pfnCallback,
                                           void *pUserData, uint32 nLinkIndex)
{
	CWTSegInfoN48D info;

	m_nCurCode = m_pCodeSource->NextCode();

	info.m_pTree       = this;
	info.m_nLinkIndex  = nLinkIndex;
	info.m_vPt1        = *pPt1;
	info.m_vPt2        = *pPt2;
	info.m_pfnCallback = pfnCallback;
	info.m_pUserData   = pUserData;

	return wt_ProcessNodeSegN48D(&m_RootNode, &info);
}


// ======================================================================= //
//  2.  0x0048d290, 259 bytes -- "report this node's objects, then descend
//      into all four children, nearest quadrant first"
//
//  cdecl, two arguments (the caller pops 8 at 0x0048d4e4 and 0x0048d27a).
//  %ebx = the node, %esi = the descriptor.
//
//  THE OBJECT WALK (0x0048d2a5..0x0048d2df)
//    `mov 0x4(%esi),%eax` then `lea (%ebx,%eax,8),%ebp` -- the head is
//    &pNode->m_Objects[index] and the stride is EIGHT, which is
//    ltlink.h's CheapLTLink (m_pPrev, m_pNext), not LTLink.  The links in
//    the list are LTLinks: +0x08 is read as the object at 0x0048d2b3.
//    The loop is rotated with the entry test at 0x0048d2af, i.e. a plain
//    `while (pCur != pHead)`.
//    `mov 0x4(%edi),%edi` at 0x0048d2b8 advances BEFORE the callback, so
//    the walk survives the callback unlinking the current node.
//    The stamp is read once and used twice (compare at 0x0048d2c1, store at
//    0x0048d2c5), and the OR into the running result goes through a stack
//    slot (0x0048d2d0..0x0048d2d9) because every register is live.
//
//  THE DESCENT (0x0048d2e1..0x0048d387)
//    `mov 0x4c(%ebx),%eax; cmp %ebp,%eax; je` with %ebp just zeroed -- a
//    leaf is m_Children[0] == LTNULL and the answer is what the objects
//    gave.  The `cmp` against a zeroed register instead of `test` is the
//    compiler spending a zero it needed anyway.
//
//    Two bits are then taken off the FIRST point only -- x against
//    m_vCenter.x (0x0048d2ee) and z against m_vCenter.z (0x0048d307).
//    That is the quadrant the ray STARTS in, and the four children are
//    visited in this order:
//
//      0x0048d31e   nX*2 + nZ            the starting quadrant
//      0x0048d340   (!nX)*2 + nZ         across in X
//      0x0048d35c   nX*2 + (!nZ)         across in Z
//      0x0048d373   (!nX)*2 + (!nZ)      diagonal
//
//    -- near to far.  `(!nX)*2` is computed once (0x0048d332..0x0048d33c,
//    into the arg1 slot) and reused at 0x0048d370; `!nZ` likewise
//    (0x0048d352, reused at 0x0048d370).  m_vCenter.y is never read: the
//    tree is a QUADTREE over X and Z, not an octree.
//
//    `or %edi,%eax; mov $1,%eax; jne` at 0x0048d380 -- the four results are
//    combined with the NON-shortcircuiting `|` (all four children are
//    always visited) and any hit collapses the answer to 1.  The `mov $1`
//    between the `or` and the `jne` does not touch flags.
// ======================================================================= //
uint32 wt_ProcessNodeSegN48D(CWTNodeN48D *pNode, CWTSegInfoN48D *pInfo)
{
	uint32         nRet;
	CheapLTLink*   pHead;
	LTLink*        pCur;
	CWTObjectN48D* pObj;
	uint32         nCode;
	uint32         nX, nZ;
	uint32         nHit;

	nRet = 0;

	pHead = &pNode->m_Objects[pInfo->m_nLinkIndex];

	pCur = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj  = (CWTObjectN48D*)pCur->m_pData;
		nCode = pInfo->m_pTree->m_nCurCode;

		pCur = pCur->m_pNext;

		if (pObj->m_nTouchCode != nCode)
		{
			pObj->m_nTouchCode = nCode;

			nRet |= pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}

	if (!pNode->m_Children[0])
		return nRet;

	nX = 1;
	if (pInfo->m_vPt1.x <= pNode->m_vCenter.x)
		nX = 0;
	nZ = 0;
	if (pInfo->m_vPt1.z > pNode->m_vCenter.z)
		nZ = 1;

	nHit  = wt_TestNodeSegN48D(pNode->m_Children[nX * 2 + nZ], pInfo);
	nHit |= wt_TestNodeSegN48D(pNode->m_Children[(nX == 0) * 2 + nZ], pInfo);
	nHit |= wt_TestNodeSegN48D(pNode->m_Children[nX * 2 + (nZ == 0)], pInfo);
	nHit |= wt_TestNodeSegN48D(pNode->m_Children[(nX == 0) * 2 + (nZ == 0)], pInfo);
	if (nHit)
		return LTTRUE;

	return nRet;
}


// ======================================================================= //
//  3.  0x0048d3a0, 347 bytes -- "does the segment touch this node at all?"
//
//  cdecl, two arguments; the other half of the mutual recursion with
//  0x0048d290.  %edi = &m_vPt1, %ebx = &m_vPt2, %esi = &m_vBoxMin,
//  %ebp = &m_vBoxMax -- four `lea`s at 0x0048d3b2..0x0048d3b8 that make
//  every later reference a +0x00 or a +0x08, i.e. .x or .z.  .y is never
//  touched: the whole test is two-dimensional.
//
//  THREE TESTS, IN THIS ORDER.
//
//  (a) 0x0048d3aa..0x0048d3e6 and 0x0048d3ec..0x0048d41f: is EITHER endpoint
//      inside the node's X/Z rectangle?  Four comparisons apiece, in the
//      order x>=min.x, z>=min.z, x<=max.x, z<=max.z, and both successes jump
//      to the SAME block (0x0048d4ec) -- an `||` of two four-term `&&`s,
//      with the shared consequent sunk past the rest of the function.
//
//  (b) 0x0048d425..0x0048d4a0: the trivial reject.  Per axis, 0 if both
//      endpoints are below the minimum, 1 if both are above the maximum.
//      The X computation has NO third arm: straddling X branches straight to
//      the exact test (0x0048d449, 0x0048d455), which is why the source has
//      to spell that arm as a jump rather than as a third value.  The Z
//      computation does have a third value, 2 (0x0048d499), and it exists
//      only so that `nSide1 == nSide2` can fail against it.
//      `cmp %eax,%ecx; jne` at 0x0048d49e, then `xor %eax,%eax; ret`:
//      equal codes means the segment is wholly in a corner region and the
//      answer is 0.
//
//  (c) 0x0048d4a9..0x0048d4cc: the exact test, twice -- (nAxis1, nAxis2) =
//      (0, 2) and then (2, 0), i.e. the two X planes and then the two Z
//      planes.  The second call re-uses the %eax the first test left at
//      zero as its literal 0 argument.  Either hit falls into the walk.
//
//  WHAT CLOSED IT (was 338/142, then 339/150).  The endpoint test is a small
//  INLINE helper, called twice under `||`: written out as one `||` of two
//  `&&` chains, VC6 hoists the shared consequent up behind the second test
//  (`je` over it); expanded from a helper, the consequent stays sunk at the
//  end (0x0048d4ec, reached by two long `jne`s).  pPt1/pMin/pMax are named
//  pointers (%edi/%esi/%ebp); pPt2 is named only AFTER the endpoint test,
//  which is why its `lea` (0x0048d3f1) comes late and it gets %ebx.
//  (helper name ours)
// ======================================================================= //
inline LTBOOL wt_PtInNodeN48D(const CWTVecN48D *pPt, const CWTVecN48D *pMin,
                              const CWTVecN48D *pMax)
{
	return pPt->x >= pMin->x && pPt->z >= pMin->z &&
	       pPt->x <= pMax->x && pPt->z <= pMax->z;
}

uint32 wt_TestNodeSegN48D(CWTNodeN48D *pNode, CWTSegInfoN48D *pInfo)
{
	const CWTVecN48D *pPt1, *pPt2, *pMin, *pMax;
	uint32 nSide1, nSide2;

	pPt1 = &pInfo->m_vPt1;
	pMin = &pNode->m_vBoxMin;
	pMax = &pNode->m_vBoxMax;

	if (wt_PtInNodeN48D(pPt1, pMin, pMax) ||
	    wt_PtInNodeN48D(&pInfo->m_vPt2, pMin, pMax))
	{
		return wt_ProcessNodeSegN48D(pNode, pInfo);
	}

	pPt2 = &pInfo->m_vPt2;

	if (pPt1->x < pMin->x && pPt2->x < pMin->x)
	{
		nSide1 = 0;
	}
	else if (pPt1->x > pMax->x && pPt2->x > pMax->x)
	{
		nSide1 = 1;
	}
	else
	{
		goto exact;
	}

	if (pPt1->z < pMin->z && pPt2->z < pMin->z)
	{
		nSide2 = 0;
	}
	else if (pPt1->z > pMax->z && pPt2->z > pMax->z)
	{
		nSide2 = 1;
	}
	else
	{
		nSide2 = 2;
	}

	if (nSide1 == nSide2)
		return 0;

exact:
	if (!wt_SegmentHitsRectN48D(pPt1, pPt2, pMin, pMax, 0, 2))
	{
		if (!wt_SegmentHitsRectN48D(pPt1, pPt2, pMin, pMax, 2, 0))
		{
			return 0;
		}
	}

	return wt_ProcessNodeSegN48D(pNode, pInfo);
}


// ======================================================================= //
//  4.  0x0048d500, 779 bytes -- "does the segment cross either of the two
//      planes perpendicular to nAxis1, inside the node's nAxis2 range?"
//
//  cdecl, SIX arguments (0x18 comes off %esp at 0x0048d4b6).  Everything is
//  reached by index: `(%esi,%ebx,4)` is pPt1[nAxis1], `(%eax,%ebx,4)` the
//  plane, `0x10(%esp,%ebp,4)` the intersection point's nAxis2 component.
//
//  THE BODY IS THE SAME BLOCK TWICE -- 0x0048d503..0x0048d68b against
//  *pMin and 0x0048d68c..0x0048d800 against *pMax -- and it is written out
//  twice below for that reason.  The two copies are not byte-identical in
//  the shipped image either: the middle `fmul` of the scale is `flds t;
//  fmuls y` at 0x0048d5ed and `flds y; fmuls t` at 0x0048d762.  That is the
//  x87 scheduler, not the source; /O2 fixes neither operand order.
//
//  ONE HALF, INSTRUCTION BY INSTRUCTION:
//
//    0x0048d512  fPlane = pPlane[nAxis1]           (spilled to %esp+0x54,
//                which is nAxis1's own ARGUMENT slot -- nAxis1 lives in
//                %ebx, so its slot is dead and VC6 homes fPlane there)
//    0x0048d51a  `test $0x01` / je -> 0            pPt1[a1] <  fPlane
//    0x0048d535  `test $0x41` / je -> 0            pPt2[a1] <= fPlane
//    0x0048d550  `cmp %eax,%ecx; je` -- equal means both endpoints are on
//                the same side, so this plane is not crossed.  The two
//                comparisons really are spelled with different operators:
//                `test $0x01` is a bare `<`, `test $0x41` is `<=`.
//    0x0048d558  fDiff = pPt2[a1] - pPt1[a1], left ON the x87 stack and
//                duplicated (`fld %st(0); fabs`) for the guard.
//    0x0048d562  `fcompl 0x004a2a30` -- that qword is 0.0010000000474974513,
//                which is (double)0.001f exactly: the literal in the source
//                is `0.001f` and fabs() promotes the comparison to double.
//                Below the threshold the percentage is forced to 0.0f
//                (0x0048d571); otherwise 0x0048d57b..0x0048d584 divides by
//                the copy still in %st(1).  Both write the SAME stack slot
//                as fPlane -- disjoint live ranges, one slot.  fPercent is
//                BLOCK-scoped inside each half (a function-scope fPercent
//                gets a frame slot of its own: frame 0x34, not 0x30).
//    0x0048d58a  the by-value copy of *pPt1 that ltvector.h's
//                `operator - (const _CVector<T> v)` makes, then the
//                three-float constructor at 0x00411d40.
//    0x0048d5c9  the result is copied out and scaled -- `operator * (T v)`,
//                constructor again.
//    0x0048d60a  the scale result is copied as operator+'s by-value
//                argument and added to *pPt1 -- constructor a third time.
//                `flds <local>; fadds 0x8(%esi)` rather than the other way
//                round is /O2 canonicalising a commutative FP sum.
//    The by-value temporaries and the intersection point all share the one
//    12-byte slot at %esp+0x10 in BOTH halves.  That needs vTest declared
//    inside each half's block, initialised from the one expression
//    `*pPt1 + (*pPt2 - *pPt1) * fPercent`: a function-scope vTest (or a
//    named vDir) pushes the second half's temporaries to a slot of their own.
//
//    RESIDUAL (near, 779/779): in the FIRST half only, the y product is
//    `flds t; fmuls y` (0x0048d5e9) where ours is `flds y; fmuls t`; the
//    second half matches.  x87 operand order of a commutative multiply;
//    unmoved by a named vDir, an assign-after-declare, a goto vs if/else
//    structure, and an inline per-plane helper called twice.
//
//    0x0048d65f  the intersection point's nAxis2 component against
//                pMin[nAxis2] (`test $0x01`, i.e. `>=`) and pMax[nAxis2]
//                (`test $0x41`, i.e. `<=`); inside both is LTTRUE.
// ======================================================================= //
LTBOOL wt_SegmentHitsRectN48D(const CWTVecN48D *pPt1, const CWTVecN48D *pPt2,
                              const CWTVecN48D *pMin, const CWTVecN48D *pMax,
                              uint32 nAxis1, uint32 nAxis2)
{
	float fPlane, fDiff;

	// ---- the plane at pMin[nAxis1] ------------------------------------ //
	fPlane = ((const float*)pMin)[nAxis1];

	if ((((const float*)pPt1)[nAxis1] <  fPlane) !=
	    (((const float*)pPt2)[nAxis1] <= fPlane))
	{
		float fPercent;

		fDiff = ((const float*)pPt2)[nAxis1] - ((const float*)pPt1)[nAxis1];

		if (fabs(fDiff) < 0.001f)
			fPercent = 0.0f;
		else
			fPercent = (fPlane - ((const float*)pPt1)[nAxis1]) / fDiff;

		CWTVecN48D vTest = *pPt1 + (*pPt2 - *pPt1) * fPercent;

		if (((const float*)&vTest)[nAxis2] >= ((const float*)pMin)[nAxis2] &&
		    ((const float*)&vTest)[nAxis2] <= ((const float*)pMax)[nAxis2])
		{
			return LTTRUE;
		}
	}

	// ---- the plane at pMax[nAxis1] ------------------------------------ //
	fPlane = ((const float*)pMax)[nAxis1];

	if ((((const float*)pPt1)[nAxis1] <  fPlane) !=
	    (((const float*)pPt2)[nAxis1] <= fPlane))
	{
		float fPercent;

		fDiff = ((const float*)pPt2)[nAxis1] - ((const float*)pPt1)[nAxis1];

		if (fabs(fDiff) < 0.001f)
			fPercent = 0.0f;
		else
			fPercent = (fPlane - ((const float*)pPt1)[nAxis1]) / fDiff;

		CWTVecN48D vTest = *pPt1 + (*pPt2 - *pPt1) * fPercent;

		if (((const float*)&vTest)[nAxis2] >= ((const float*)pMin)[nAxis2] &&
		    ((const float*)&vTest)[nAxis2] <= ((const float*)pMax)[nAxis2])
		{
			return LTTRUE;
		}
	}

	return LTFALSE;
}


// ======================================================================= //
//  5.  0x0048d810, 213 bytes -- vtable 0x004a38f8 slot 5.
//      "turn a centre and a radius into a box and run the box query"
//
//  thiscall, `ret $0x4` at 0x0048d8e2.  %edi = this, %esi = the descriptor.
//
//    0x0048d817  the same slot-0 stamp fetch as 0x0048d210, stored to
//                m_nCurCode at 0x0048d822.
//    0x0048d836  `mov %edi,0x44(%esi)` -- the descriptor is told which tree
//                it is running against; 0x0048d8f0 reads +0x44 to get at
//                m_nCurCode the same way 0x0048d290 reads +0x00.
//    0x0048d825..0x0048d8b3  the two corners.  m_fRadius is loaded THREE
//                times per corner (0x0048d825, 0x0048d828, 0x0048d82f and
//                again at 0x0048d867..0x0048d878) -- one read per component,
//                not one read cached; two of the three are spilled to
//                %esp+0x18 and +0x1c only because the x87 stack is full.
//                The results go to %esp+0x08/0x0c/0x10 and are then moved to
//                the destination AS DWORDS (`mov 0x8(%esp),%edx` /
//                `mov %edx,(%ecx)`).  That integer move is the signature of
//                an inlined `x = px` on a float parameter, i.e. ltvector.h's
//                Init(), and NOT of a plain `m_vBoxMin.x = ...`, which would
//                have been a single `fstps (%ecx)`.
//                The minimum is spelled `centre - radius` (`fsub %st(1)`)
//                and the maximum comes out as `fadds` with the operands the
//                other way round, which is /O2 canonicalising the sum.
//    0x0048d8b6  the null-guarded default for +0x28.  The store is INSIDE
//                the guard (`jne` jumps over it).
//    0x0048d8c4  0x0048c540(&m_RootNode, pInfo, 0) -- the recursive box walk.
//    0x0048d8d3  0x0048d8f0(&m_RootObjects, pInfo) -- the tree's own list of
//                objects that are not in any node.  `add $0x14,%esp` at
//                0x0048d8da pays for BOTH calls at once, which is why there
//                is no adjustment between them.
// ======================================================================= //
void CWorldTreeN48D::FindObjectsInBox(CWTBoxInfoN48D *pInfo)
{
	m_nCurCode = m_pCodeSource->NextCode();

	pInfo->m_pTree = this;

	pInfo->m_vBoxMin = pInfo->m_vCenter -
		LTVector(pInfo->m_fRadius, pInfo->m_fRadius, pInfo->m_fRadius);

	pInfo->m_vBoxMax = pInfo->m_vCenter +
		LTVector(pInfo->m_fRadius, pInfo->m_fRadius, pInfo->m_fRadius);

	if (!pInfo->m_pfnTest)
		pInfo->m_pfnTest = (void*)wt_DefaultTestN48D;

	wt_FindInBoxN48D(&m_RootNode, pInfo, 0);

	wt_WalkRootListN48D(&m_RootObjects, pInfo);
}


// ======================================================================= //
//  5b. 0x0048d8f0, 59 bytes -- "report the tree's root-list objects"
//
//  cdecl, `ret` at 0x0048d92a; %ebx = the list head, %esi = the cursor,
//  %edi = the descriptor.  The object (+0x08 of the link), then the stamp
//  through pInfo->m_pTree (+0x44), then the advance -- 0x0048d902..
//  0x0048d908 -- before the stamp compare, as in 0x0048d290's walk.  The
//  callback gets (object, pInfo->m_pUserData) (`call *0x14(%edi)`).
// ======================================================================= //
void wt_WalkRootListN48D(CheapLTLink *pHead, CWTBoxInfoN48D *pInfo)
{
	LTLink*        pCur;
	CWTObjectN48D* pObj;
	uint32         nCode;

	pCur = pHead->m_pNext;
	while (pCur != (LTLink*)pHead)
	{
		pObj  = (CWTObjectN48D*)pCur->m_pData;
		nCode = pInfo->m_pTree->m_nCurCode;

		pCur = pCur->m_pNext;

		if (pObj->m_nTouchCode != nCode)
		{
			pObj->m_nTouchCode = nCode;

			pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}
	}
}


// ======================================================================= //
//  6.  0x0048d930, 109 bytes -- "become a copy of another tree's shape"
//
//  thiscall, `ret $0x4` at 0x0048d99a.  %esi = this, %edi = the other tree.
//
//    0x0048d935  `call 0x0048cd50` with %ecx still holding this -- the Term
//    		  src/engine/lithtech/lt2new4.cpp already wrote up.  It comes
//                FIRST, so the old tree is dismantled before anything is
//                copied in.
//    0x0048d944  the three pointers at +0x70, +0x6c, +0x68, loaded and
//                stored in that order (high to low).
//    0x0048d953  `lea 0x30(%edi),%eax` and `lea 0x24(%edi),%ecx`: the other
//                tree's root node is at +0x0c, so those are its m_vBoxMax
//                (0x0c+0x24) and m_vBoxMin (0x0c+0x18).  Both are copied
//                into two 12-byte stack blocks -- by-value parameters -- and
//                the LOWER block is the one built from +0x24, so the call is
//                SetDims(min, max).  0x0048c830 takes 0x18 bytes of them.
//    0x0048d990  `push %edi` (other+0x0c) then `push %ebx` (this+0x0c) with
//                %ecx = this: CopyNodes(&m_RootNode, &pOther->m_RootNode).
// ======================================================================= //
void CWorldTreeN48D::CopyFrom(CWorldTreeN48D *pOther)
{
	Term();

	m_p070 = pOther->m_p070;
	m_p06c = pOther->m_p06c;
	m_p068 = pOther->m_p068;

	m_RootNode.SetDims(pOther->m_RootNode.m_vBoxMin,
	                   pOther->m_RootNode.m_vBoxMax);

	CopyNodes(&m_RootNode, &pOther->m_RootNode);
}


// ======================================================================= //
//  7.  0x0048d9a0, 248 bytes -- "read the tree's extents off a stream"
//
//  thiscall, `ret $0x4`.  %esi = the stream, %edi = this.
//
//  WHAT PINS THE STREAM CLASS: eight identical `push $0x4; push <addr>;
//  call *0x8(%eax)` sequences and one `call *0x10(%edx)` whose result is
//  tested against zero.  iltstream.h declares a protected virtual
//  destructor, then Release, Read, ReadString, ErrorStatus -- so Read is
//  slot 2 (+0x08) and ErrorStatus is slot 4 (+0x10), to the slot.  Nothing
//  else in the image's stream interfaces lines up that way.
//
//  THE EIGHT READS, in emitted order and where each lands (frame at
//  orig-0x28):
//    orig-0x0c, -0x08, -0x04   the first vector
//    orig-0x18, -0x14, -0x10   the second vector
//    orig-0x1c                 a dword that is read and NEVER USED
//    this+0x70                 straight into the member
//
//  0x0048da28/0x0048da40 then build the two by-value blocks for
//  0x0048c830; the LOWER of the two is the -0x0c..-0x04 trio, so the first
//  vector read is the MINIMUM.
//
//  0x0048da2d/0x0048da32 set two BYTES, 0 at orig-0x1d and 8 at orig+0x04
//  (the argument slot, re-used), and 0x0048da68 passes their ADDRESSES to
//  0x0048c9b0 along with the stream -- a depth counter and a depth limit of
//  eight.
//
//  Both failures return 0 through the shared tail at 0x0048da8e.  The two
//  byte stores are scheduled BEFORE the SetDims argument copies, so they
//  precede the SetDims call in source; and the two tests are nested positive
//  guards with one `return LTFALSE` after (early refusals fold the second
//  test into `neg/sbb/inc`).
// ======================================================================= //
LTBOOL CWorldTreeN48D::Load(ILTStream *pStream)
{
	CWTVecN48D vMin, vMax;
	uint32     nUnused;
	uint8      nDepth, nMaxDepth;

	pStream->Read(&vMin.x, 4);
	pStream->Read(&vMin.y, 4);
	pStream->Read(&vMin.z, 4);

	pStream->Read(&vMax.x, 4);
	pStream->Read(&vMax.y, 4);
	pStream->Read(&vMax.z, 4);

	pStream->Read(&nUnused, 4);
	pStream->Read(&m_p070, 4);

	nDepth    = 0;
	nMaxDepth = 8;

	m_RootNode.SetDims(vMin, vMax);

	if (m_RootNode.LoadNodes(pStream, &nDepth, &nMaxDepth))
	{
		if (pStream->ErrorStatus() == LT_OK)
			return LTTRUE;
	}

	return LTFALSE;
}


// ======================================================================= //
//  8.  0x0048daa0, 94 bytes -- "give the destination the same children the
//      source has, all the way down"
//
//  thiscall, `ret $0x8`.  %ebp = arg1 (destination), %ebx = arg2 (source);
//  this is parked at %esp+0x10 at 0x0048daae and reloaded for each recursive
//  call at 0x0048dadb.
//
//    0x0048daa8  `mov 0x4c(%ebx),%eax; test; je` -- a source leaf ends the
//                recursion and the answer is LTTRUE.  The guard is on the
//                SOURCE, not the destination.
//    0x0048daba  0x0048ca50 on the destination, thiscall, no arguments:
//                allocate the four children.  Zero back is LTFALSE.
//    0x0048dacf  `lea 0x4c(%ebp),%esi` and `sub %ebp,%ebx` -- the classic
//                strength reduction: one cursor over the destination's
//                child array and one constant delta to reach the source's.
//                `cmp $0x4` at 0x0048daec is the four-child loop.
//    0x0048dad9  `push %eax` (source child) then `push %ecx` (destination
//                child): CopyNodes(pDest->m_Children[i], pSrc->m_Children[i]).
// ======================================================================= //
LTBOOL CWorldTreeN48D::CopyNodes(CWTNodeN48D *pDest, CWTNodeN48D *pSrc)
{
	uint32 i;

	if (pSrc->m_Children[0])
	{
		if (!pDest->AllocChildren())
			return LTFALSE;

		for (i = 0; i < 4; i++)
		{
			if (!CopyNodes(pDest->m_Children[i], pSrc->m_Children[i]))
				return LTFALSE;
		}
	}

	return LTTRUE;
}


// *********************************************************************** //
//
//  PER-ADDRESS REPORT
//  ==================
//
//  DONE (source above).  "calls" is the call count on the shipped
//  image over the verified extent, and what this file
//  emits for that function, side by side.
//
//  addr        size  what it is                        shipped calls / ours
//  ----------  ----  --------------------------------  --------------------
//  0x0048d210   116  vtable slot 4: start a segment     2 (1 direct
//                    query -- fill the descriptor and     0x0048d290,
//                    hand it the root node.               1 indirect slot 0)
//                                                       ours: same
//  0x0048d290   259  report a node's objects, then      5 (4 direct
//                    descend into all four children       0x0048d3a0,
//                    nearest-quadrant-first.              1 indirect cb)
//                                                       ours: same
//  0x0048d3a0   347  segment-versus-node cull, then     4 (2x 0x0048d500,
//                    walk.                                2x 0x0048d290)
//                                                       ours: same
//  0x0048d500   779  segment versus the two planes      6 (6x 0x00411d40)
//                    normal to one axis, range-checked  ours: same
//                    on a second axis.
//  0x0048d810   213  vtable slot 5: centre+radius to    3 (0x0048c540,
//                    a box, then the box walk and the     0x0048d8f0,
//                    root list.                           1 indirect slot 0)
//                                                       ours: same
//  0x0048d930   109  become a copy of another tree's    3 (0x0048cd50,
//                    shape.                               0x0048c830,
//                                                          0x0048daa0)
//                                                       ours: same
//  0x0048d9a0   248  read the extents off an ILTStream  11 (0x0048c830,
//                    and load the node tree.               0x0048c9b0,
//                                                          9 indirect: 8x
//                                                          Read + ErrorStatus)
//                                                       ours: same
//  0x0048daa0    94  recursively give the destination   2 (0x0048ca50,
//                    the source's children.               self)
//                                                       ours: same
//
//  EXTENT DEFECT -- ONE ROW IS NOT A FUNCTION
//  ------------------------------------------
//  0x0048db40, "704 bytes", is DATA: dinput.lib's `c_dfDIJoystick` object
//  table.  It has 0 `ret`s and four branch targets
//  outside the range; nothing in .text calls it; the only reference to it
//  anywhere is the dword at 0x004a3928.
//
//  What settles it is the five dwords in front of that reference:
//
//      0x004a3914  0x00000018   DIDATAFORMAT.dwSize      = 24
//      0x004a3918  0x00000010   .dwObjSize               = 16
//      0x004a391c  0x00000001   .dwFlags   = DIDF_ABSAXIS
//      0x004a3920  0x00000050   .dwDataSize = sizeof(DIJOYSTATE) = 80
//      0x004a3924  0x0000002c   .dwNumObjs               = 44
//      0x004a3928  0x0048db40   .rgodf
//
//  44 x 16 = 704, to the byte.  Read as DIOBJECTDATAFORMAT the table is
//  exactly DIJOYSTATE:
//
//      offs 0x00..0x1c   8 records, dwType 0x80ffff03 (DIDFT_AXIS |
//                        DIDFT_ANYINSTANCE), dwFlags 0x100
//                        (DIDOI_ASPECTPOSITION), pguid pointing into the
//                        GUID pool at 0x004a3a7c..0x004a3b2c -- which is
//                        dxguid.lib's, and those eight are GUID_XAxis
//                        (A36D02E0-C9F3-11CF-BFC7-444553540000), YAxis,
//                        ZAxis, RxAxis, RyAxis, RzAxis and Slider twice.
//      offs 0x20..0x2c   4 records, dwType 0x80ffff10 (DIDFT_POV),
//                        pguid = GUID_POV (A36D02F2-...).
//      offs 0x30..0x4f  32 records, dwType 0x80ffff0c (DIDFT_BUTTON),
//                        pguid = NULL.
//
//  And the consumer is where it should be: 0x00436ff8 pushes 0x004a3914
//  into `call *0x2c(%edx)`, slot 11 of IDirectInputDeviceA, i.e.
//  SetDataFormat -- taken only when the device's +0x80 byte is 4 (joystick),
//  with a locally built DIDF_RELAXIS format on the other arm.
//
//  So this row costs nothing and yields nothing: it is library data the
//  function enumerator saw a pointer to.
//
//  WHAT THE NEIGHBOURS ARE (handed on, not written here)
//  ----------------------------------------------------
//  Same module, all reached from the eight above:
//      0x0048c540  the recursive box walk          (3 args, cdecl)
//      0x0048c780  WorldTreeNode's constructor
//      0x0048c830  WorldTreeNode::SetDims(LTVector, LTVector) by value
//      0x0048c930  WorldTreeNode's Term
//      0x0048c9b0  WorldTreeNode::LoadNodes(stream, depth*, maxdepth*)
//      0x0048ca50  WorldTreeNode::AllocChildren
//      0x0048cce0  the tree's constructor          (written, lt2new4.cpp)
//      0x0048cd50  the tree's Term                 (written, lt2new4.cpp)
//      0x0048cd10  vtable slot 6, `ret $8`
//      0x0048cd20  a two-hop forward, `jmp *0x4(%eax)` on this->m_p04
//      0x0048cd30  set m_p04 and tie off m_RootObjects
//      0x0048cde0  remove an object from the tree  (12 callers)
//      0x0048d070  vtable slot 2                   (written, lt2new2.cpp)
//      0x0048d1f0  vtable slot 3                   (written, ltnew2.cpp)
//      0x0048d8f0  the root-list walk 0x0048d810 finishes with
//      0x0048db00  link an object into the root list and clear its +0x50
//
//  NOT this module, and NOT engine source:
//      0x0048db30  jmp *0x004a1000  DINPUT.dll!DirectInputCreateA
//      0x0048db40  c_dfDIJoystick.rgodf            (above)
//      0x0048de00  `push $0x0048de10; call 0x0049307e` -- atexit() arming
//                  the mss32 thunk at 0x0048de10/0x0048de20 (AIL_shutdown)
//      0x0048de30  insert at one end of a {head,tail} } a two-pointer
//      0x0048de60  insert at the other end            } intrusive-list
//      0x0048de90  insert relative to a given node    } template's four
//      0x0048dee0  unlink a node                      } members, thiscall,
//                  -- all four called ONLY from 0x0043f7xx..0x00443xxx and
//                  from the .CRT initialiser at 0x004a051a/0x004a054d, i.e.
//                  they belong to whatever 0x0044xxxx is, not to the tree.
//
// *********************************************************************** //


// 0x00463d20 -- 6 bytes, `mov $1,%eax ; ret`: the default box-query test,
// which accepts every object.  cdecl; the arguments the walk passes are not
// read.  Too small to be placed on its own.
LTBOOL wt_DefaultTestN48D()
{
	return LTTRUE;
}
