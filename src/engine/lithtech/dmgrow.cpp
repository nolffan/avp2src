// =======================================================================
//  w12_440000.cpp -- reconstruction scratch
//
//  NOT A TRANSLATION UNIT OF ITS OWN.  Every body below belongs in an
//  existing src/engine file (named in the banner above it); the classes
//  here are minimal local stand-ins so the file parses on its own.  The
//  merge notes at the bottom say which declarations already exist in the
//  tree and must be reused instead of these.
//
//  GROUND TRUTH: the retail lithtech.exe.  Every offset, constant and
//  branch below is cited by the shipped address it was read at.
//
//  Symbols introduced here carry the Q440 suffix.
// =======================================================================

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef int            int32;
typedef int            LTBOOL;
typedef unsigned long  LTRESULT;

#define LTNULL  0
#define LTTRUE  1
#define LTFALSE 0

#ifndef _MSC_VER
#define __int64      long long
#define __stdcall
#define __thiscall
#define __cdecl
#endif


// =======================================================================
//  ROW 1 of 8 -- 0x0041eb50, 47 bytes, provenance `call`
//
//  BELONGS IN: src/engine/server/counter.cpp (counter.obj), immediately
//  after cnt_Init.  The client build has it and server.dll does NOT --
//  nothing in server.dll calls it, so /Gy + /OPT:REF dropped the COMDAT.
//  Guard it or not; there is no server oracle.
//
//  MODULE PROOF (call graph + the two-image oracle, not the string pool):
//    * lithtech 0x0041eb10 `jmp 0x0041eb20` over 11 bytes of padding, and
//      0x0041eb20 = QueryPerformanceFrequency into a discarded local then
//      `movl $1,0x004a9040 / movl $0,0x004a9044`.  server.dll 0x10005f12
//      `jmp 0x10005f17` (displacement ZERO) and 0x10005f17 is the same
//      body against 0x10051250/0x10051254.  That is counter.cpp's cnt_Init
//      verbatim, so 0x004a9040 is g_CounterScale and this is counter.obj.
//    * The COMDAT order round it matches the server's counter.obj exactly:
//      cnt_Init, THIS, CBaseCounter::CBaseCounter(int) (0x0041eb80, the
//      `cmp $0x2` / `cmp $0x1` pair), ... ReadRaw (0x0041ec30, QPC then
//      sub then __aulldiv against 0x004a9040), cnt_EndCounter (0x0041ec70,
//      `mov 0x4(%esp),%ecx ; jmp`).
//
//  WHAT IT IS: the raw counter unit's frequency.  ReadRaw answers
//  (now - start) / g_CounterScale; this answers (QPF) / g_CounterScale, so
//  ReadRaw's answer divided by this one is seconds -- which is exactly what
//  the caller at 0x0042c7ea..0x0042c801 does (`fildll` the counter,
//  `fildll` this, `fdivrl`).
//
//  RETURN TYPE IS 32-BIT UNSIGNED, and that is read, not guessed: the
//  helper leaves edx:eax, but all four call sites (0x0042c7ef, 0x0042c928,
//  0x0042c97a, 0x0042c9a7) store eax and then store a LITERAL ZERO into the
//  high dword before `fildll`.  That zero-extend-then-fild is VC6's
//  unsigned-int-to-double sequence; a 64-bit return would have used edx.
//
//  THE DIVISION IS UNSIGNED: 0x00493840 is __aulldiv (plain `div`, the
//  shift-and-correct slow path, no sign bookkeeping), where counter.cpp's
//  Read reaches __alldiv.  Same discipline as ReadRaw, which counter.cpp
//  already documents.
// =======================================================================

#ifndef _WINDOWS_
typedef union _LARGE_INTEGER_Q440
{
	struct { uint32 LowPart; int32 HighPart; } u;
	__int64 QuadPart;
} LARGE_INTEGER_Q440;
#define LARGE_INTEGER LARGE_INTEGER_Q440
// dllimport: shipped calls through the IAT (`call *0x4a1088`, 0x0041eb58).
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceFrequency(LARGE_INTEGER* p);
#endif

// counter.cpp already declares this (0x10051250 on the server side).
// 0x004a9040/0x004a9044 in lithtech.exe; written only by cnt_Init
// (0x0041eb2e, 0x0041eb38) and read here and by ReadRaw (0x0041ec52).
extern __int64 g_CounterScale;

// 0x0041eb50 (name ours).
uint32 cnt_GetFrequencyQ440()
{
	LARGE_INTEGER freq;

	// 0x0041eb53 `lea 0x0(%esp),%eax` -- the 8-byte local is the whole
	// frame (`sub $0x8,%esp` at 0x0041eb50).
	QueryPerformanceFrequency(&freq);

	// 0x0041eb5e/0x0041eb64 load the scale hi/lo, 0x0041eb6a/0x0041eb6f
	// the frequency hi/lo, and 0x0041eb76 calls __aulldiv (0x00493840),
	// which cleans its own 16 bytes -- the `add $0x8,%esp` at 0x0041eb7b
	// is the local, not the arguments.
	return (uint32)((unsigned __int64)freq.QuadPart
	                / (unsigned __int64)g_CounterScale);
}


// =======================================================================
//  ROW 2 of 8 -- 0x00402910, 57 bytes, provenance `call`
//
//  BELONGS IN: src/engine/server/counter.cpp (counter.obj -- the file that
//  already holds CClassMgr's constructor, Init, Term and cb_LoadModule).
//
//  MODULE PROOF (call graph + an existing struct map, not the string pool):
//    * The one caller, 0x00475690, reaches it as
//        lea 0x80c(%ebp),%ecx ; push $0x4a74c0 "BaseClass" ; call
//      and servermgr.h:1798 already has `CClassMgr m_ClassMgr; // 0x80c`
//      inside CServerMgr.  So `this` is a CClassMgr.
//    * The hash it reads is at this+0x24 (0x00402921), and servermgr.h:756
//      already has `void* m_hClassHash; // +0x24 (0x830) name -> CClassInfo`.
//    * 0x0042cf70 is dhash_FindElement (servermgr.h:2519 / server
//      0x1000dd38 -- same body: dhash_GetSlot, then walk the bucket list
//      comparing key length at +0x14 and dispatching the compare through
//      g_DHashCmpFns at 0x004b31e4).  0x0042d080 is dhash_GetData
//      (servermgr.h:2521 / server 0x1000de16 -- `if (!p) return p; return
//      p->[0x10]`).
//    * Its neighbours are the same object: 0x00402950 is LoadServerBinaries
//      ("LT_MISSINGCLASS"/"LoadServerBinaries" RETURN_ERROR at 0x004029c3,
//      a de_error.h literal naming its own function) and 0x00402a50 is
//      already mapped to counter.obj.
//
//  SHAPE NOTES:
//    * strlen is INLINE (`or $-1,%ecx ; repnz scasb ; not %ecx ; dec %ecx`
//      at 0x0040291a..0x00402926) -- written out, once.
//    * POSITIVE GUARD.  The `je` at 0x00402936 goes FORWARD to
//      `xor %eax,%eax ; ret $0x4` at 0x00402944, which sits past the only
//      other return (0x00402941).  Error last = success guarded;
//      writing `if (!hElement) return LTNULL;` hosts the zero
//      return inline and inverts the branch.
//    * `ret $0x4` with one stack argument and `this` in ecx = __thiscall.
// =======================================================================

class CHashTable;
class CClassInfo;

void* dhash_FindElement(CHashTable* pTable, void* pKey, uint32 nKeyLen); // 0x0042cf70
void* dhash_GetData(void* hElement);                                     // 0x0042d080

extern "C" unsigned int __cdecl strlen(const char*);

// servermgr.h's CClassMgr, cut down to the one field this body reads.
class CClassMgrQ440
{
public:
	CClassInfo* FindClassQ440(const char* pName);   // 0x00402910

	uint8  m_pad_00[0x24];
	void*  m_hClassHash;                     // +0x24 (CServerMgr+0x830)
};

// 0x00402910 (name ours, from the one call site's "BaseClass" argument).
CClassInfo* CClassMgrQ440::FindClassQ440(const char* pName)
{
	// 0x00402921 loads m_hClassHash BETWEEN the scan and the `not`; that is
	// scheduling.  Argument order is read off the pushes at 0x00402927
	// (len), 0x00402928 (pName), 0x00402929 (hash) -- right to left.
	void* hElement = dhash_FindElement((CHashTable*)m_hClassHash,
	                                   (void*)pName, strlen(pName));

	if (hElement)
		return (CClassInfo*)dhash_GetData(hElement);

	return LTNULL;
}


// =======================================================================
//  ROW 3 of 8 -- 0x0040a3d0, 46 bytes, provenance `cb`
//
//  BELONGS IN: src/engine/lithtech/clientinterface.cpp (clientinterface.obj)
//  as a file-scope static beside ci_FindObjectsInSphere.
//
//  MODULE PROOF (the cheap "declared and never defined" test, plus the one
//  reference):
//    * The address is stored NOWHERE in .data or .rdata.  Its single
//      reference in the whole image is `push $0x40a3d0` at 0x0040a2f8, an
//      argument to the virtual call at 0x0040a309 -- so it is a callback,
//      and it is not an ASCII accident or a stray dword.
//    * 0x0040a2f8 is inside 0x0040a230, which clientinterface.h:333 already
//      names ci_FindObjectsInSphere and clientinterface.cpp:95 already
//      installs in the interface table -- and which is DECLARED AND NEVER
//      DEFINED in the tree.
//    * The object it is handed carries its position at +0x18c, which is
//      clientmgr.h:290's `LTVector m_Pos; // 0x18c` on CClientObject; the
//      no-world-tree arm of the same function (0x0040a339) reads the same
//      offset off the same type.
//
//  THE CONTEXT STRUCT is read straight off the body: +0x0c the caller's
//  HLOCALOBJ array, +0x10 its capacity, +0x14 a pointer to the out count,
//  +0x18 a pointer to the found count.  Those are ci_FindObjectsInSphere's
//  inObjects / nInObjects / nOutObjects / nFound, and the caller fills them
//  at 0x0040a2eb (+0x10) and 0x0040a2fe (+0x0c).  The first twelve bytes are
//  not touched here, so they stay a pad.
//
//  COMPARISON IS UNSIGNED: `jae` at 0x0040a3ea, and operand order
//  `cmp %esi,%ecx` = cmp(count, max) puts the count on the left.
// =======================================================================

typedef void* HLOCALOBJ;

// clientmgr.h's CClientObject, cut to the one field this body reads.
class CClientObjectQ440
{
public:
	uint8  m_pad_00[0x54];
	// 0x0040a3d4.  A 32-bit non-zero here means the object is skipped.
	// Nothing else in the image pins its meaning, so it keeps its offset
	// for a name.
	uint32 m_nUnknown54;                     // 0x054
};

// The block ci_FindObjectsInSphere builds on its frame and hands to the
// world tree as the callback's user data (name ours).
struct FindObjectsInfoQ440
{
	uint8      m_pad_00[0x0c];
	HLOCALOBJ* m_pObjects;                   // 0x00c
	uint32     m_nMaxObjects;                // 0x010
	uint32*    m_pnOutObjects;               // 0x014
	uint32*    m_pnFound;                    // 0x018
};

// 0x0040a3d0 (name ours).  Plain `ret` with both arguments on the stack,
// so __cdecl.
static void ci_FindObjectsInSphereCBQ440(CClientObjectQ440* pObject,
                                         FindObjectsInfoQ440* pInfo)
{
	// 0x0040a3d4 `mov 0x54(%edx),%eax ; test ; jne` to the single `ret` at
	// 0x0040a3fd.  See the coin-flip note at the bottom: the `if (!x) {...}`
	// spelling is byte-identical here and cannot be separated.
	if (pObject->m_nUnknown54)
		return;

	// 0x0040a3e0..0x0040a3f5.  The count is loaded once (0x0040a3e6) and
	// reused as the index (0x0040a3ef); the pointer is RELOADED at
	// 0x0040a3f2 because the store through m_pObjects may alias it, which
	// is what says both statements were written out against the members.
	if (*pInfo->m_pnOutObjects < pInfo->m_nMaxObjects)
	{
		pInfo->m_pObjects[*pInfo->m_pnOutObjects] = pObject;
		(*pInfo->m_pnOutObjects)++;
	}

	// 0x0040a3f7/0x0040a3fb, outside the capacity test.
	(*pInfo->m_pnFound)++;
}


// =======================================================================
//  0x0040a230, 410 bytes -- ci_FindObjectsInSphere.
//
//  BELONGS IN: src/engine/lithtech/clientinterface.cpp, beside the callback
//  above (clientinterface.h:333 declares it; clientinterface.cpp:95 installs
//  it; it was never defined).  Written here because the callback, which it
//  must share a TU with, is here.
//
//  Off the bytes:
//    * radius*radius into the frame's lowest slot (0x0040a256), both out
//      counts zeroed (0x0040a245 / 0x0040a24b).
//    * g_pClientMgr (0x004b17e0) + 0x6c is the world tree; its address is
//      null-tested (`lea 0x6c(%eax),%ecx ; test ; je` at 0x0040a25a).
//    * WITH a tree: min = center - radius, max = center + radius, the
//      callback block's four fields, and vtable slot 1 -- server
//      worldtree.cpp's FindObjectsInBox(min, max, cb, user, 0).
//    * WITHOUT one: the eleven 16-byte LTLists at +0x40c (clientmgr3.cpp's
//      m_Records40c), the outer offset running 0x410..0x4c0 (0x0040a3b1),
//      each object's m_Pos (+0x18c) against the centre, same capacity test
//      as the callback.  The centre is re-copied into the frame every
//      iteration (0x0040a337..0x0040a361): _CVector's operator- takes its
//      right operand BY VALUE.
// =======================================================================

extern "C" double __cdecl sqrt(double);
#pragma intrinsic(sqrt)

template<class T>
class _CVectorQ440
{
public:
	_CVectorQ440() {}
	_CVectorQ440(T mx, T my, T mz) { x = mx; y = my; z = mz; }

	T MagSqr() const { return (x * x) + (y * y) + (z * z); }
	T Mag() const { return (T)sqrt((x * x) + (y * y) + (z * z)); }

	_CVectorQ440<T> operator + (const _CVectorQ440<T> v) const
		{ return _CVectorQ440<T>(x + v.x, y + v.y, z + v.z); }
	_CVectorQ440<T> operator * (T v) const
		{ return _CVectorQ440<T>(x * v, y * v, z * v); }
	void operator += (const _CVectorQ440<T> v) { x += v.x; y += v.y; z += v.z; }

	_CVectorQ440<T> operator - (const _CVectorQ440<T> v) const
		{ return _CVectorQ440<T>(x - v.x, y - v.y, z - v.z); }
	_CVectorQ440<T> operator + (T v) const
		{ return _CVectorQ440<T>(x + v, y + v, z + v); }
	_CVectorQ440<T> operator - (T v) const
		{ return _CVectorQ440<T>(x - v, y - v, z - v); }

	T x;
	T y;
	T z;
};
typedef _CVectorQ440<float> LTVectorQ440;


// =======================================================================
//  0x0040a150, 211 bytes -- ci_CastRay.
//
//  BELONGS IN: src/engine/lithtech/clientinterface.cpp (clientinterface.h:332
//  declares it at 0x0040a150; clientinterface.cpp:94 installs it; never
//  defined).  Its address sits just before ci_FindObjectsInSphere's
//  0x0040a230, which this file already carries.
//
//  The server twin is smgr.cpp's si_CastRay: the direction's Mag (inlined
//  fsqrt, 0x0040a170) against 0.00001f (0x004a1a04), 10000.0f (0x004a1a64)
//  divided by it (`fdivrs` at 0x0040a18d), m_To = m_Direction * scale
//  (0x0040a199..0x0040a1c3), then From added in (0x0040a1c6..0x0040a20d),
//  and a tail into ci_IntersectSegment (0x00404aa0).
// =======================================================================

// clientinterface.h's ClientIntersectQuery, cut to what CastRay reads.
struct ClientIntersectQueryQ440
{
	LTVectorQ440 m_From;                     // 0x000
	LTVectorQ440 m_To;                       // 0x00c
	LTVectorQ440 m_Direction;                // 0x018
};
struct ClientIntersectInfoQ440;

LTBOOL ci_IntersectSegmentQ440(ClientIntersectQueryQ440* pQuery,
                               ClientIntersectInfoQ440* pInfo); // 0x00404aa0

LTBOOL ci_CastRayQ440(ClientIntersectQueryQ440* pQuery, ClientIntersectInfoQ440* pInfo)
{
	float fMag = pQuery->m_Direction.Mag();

	if (fMag < 0.00001f)
		return LTFALSE;

	fMag = 10000.0f / fMag;

	pQuery->m_To = pQuery->m_Direction * fMag;
	pQuery->m_To = pQuery->m_To + pQuery->m_From;

	return ci_IntersectSegmentQ440(pQuery, pInfo);
}

// clientmgr.h's CClientObject, the position only.
class CClientObjSphereQ440
{
public:
	uint8        m_pad_000[0x18c];
	LTVectorQ440 m_Pos;                      // 0x18c
};

class LTLinkQ440
{
public:
	LTLinkQ440* m_pPrev;                     // 0x000
	LTLinkQ440* m_pNext;                     // 0x004
	void*       m_pData;                     // 0x008
};

class LTListQ440
{
public:
	uint32     m_nElements;                  // 0x000
	LTLinkQ440 m_Head;                       // 0x004
};

typedef void (*WTBoxCallbackQ440)(CClientObjectQ440* pObj, FindObjectsInfoQ440* pInfo);

// server worldtree.cpp's CWorldTree_F vtable, slots 0 and 1.
class CWorldTreeQ440
{
public:
	virtual void _v00();
	virtual void FindObjectsInBox(LTVectorQ440* pMin, LTVectorQ440* pMax,
	                              WTBoxCallbackQ440 pfnCallback,
	                              void* pUserData, uint32 nListIndex);
	uint8 m_pad_04[0x3a0 - 0x04];
};

class CClientMgrSphereQ440
{
public:
	uint8          m_pad_00[0x6c];
	CWorldTreeQ440 m_WorldTree;              // 0x06c
	LTListQ440     m_ObjectLists[11];        // 0x40c
};

extern CClientMgrSphereQ440* g_pClientMgrQ440;  // 0x004b17e0

#define LT_OK_Q440 0

LTRESULT ci_FindObjectsInSphereQ440(LTVectorQ440* pCenter, float radius,
                                    HLOCALOBJ* inObjects, uint32 nInObjects,
                                    uint32* nOutObjects, uint32* nFound)
{
	float               radiusSqr;
	LTVectorQ440        vMin, vMax;
	FindObjectsInfoQ440 info;

	radiusSqr = radius * radius;

	*nOutObjects = 0;
	*nFound      = 0;

	CWorldTreeQ440* pTree = &g_pClientMgrQ440->m_WorldTree;
	if (pTree)
	{
		info.m_pnOutObjects = nOutObjects;
		info.m_pnFound      = nFound;

		vMin = *pCenter - radius;
		vMax = *pCenter + radius;

		// This order, not the reverse: 0x0040a2df loads inObjects FIRST and
		// stores it LAST (0x0040a2fe); swapping the two statements swaps
		// the registers (measured).
		info.m_pObjects    = inObjects;
		info.m_nMaxObjects = nInObjects;

		pTree->FindObjectsInBox(&vMin, &vMax, ci_FindObjectsInSphereCBQ440,
		                        &info, 0);
		return LT_OK_Q440;
	}

	for (uint32 i = 0; i < 11; i++)
	{
		LTLinkQ440* pHead = &g_pClientMgrQ440->m_ObjectLists[i].m_Head;

		for (LTLinkQ440* pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
		{
			CClientObjSphereQ440* pObj = (CClientObjSphereQ440*)pCur->m_pData;

			if ((pObj->m_Pos - *pCenter).MagSqr() < radiusSqr)
			{
				if (*nOutObjects < nInObjects)
				{
					inObjects[*nOutObjects] = (HLOCALOBJ)pObj;
					(*nOutObjects)++;
				}

				(*nFound)++;
			}
		}
	}

	return LT_OK_Q440;
}


// =======================================================================
//  0x0040a4f0, 223 bytes -- ci_SetGlobalPanTexture.
//
//  BELONGS IN: src/engine/lithtech/clientinterface.cpp, immediately before
//  ci_SetGlobalPanInfo (0x0040a5d0, which is written there);
//  clientinterface.h:371 declares it and clientinterface.cpp:101 installs
//  it, never defined.
//
//  Off the bytes:
//    * the 0xc-byte CClientFileRef is zeroed first (0x0040a4fc..0x0040a50c,
//      clientmgr.h's inline ctor), before the index test.
//    * index >= 2 (`jb` at 0x0040a513): dsi_OnReturnError(0x3c), then at
//      g_DebugLevel >= 1 the error print with "60" and " " -- RETURN_ERROR.
//    * &g_GlobalPanInfo[index] (0x004b4460, stride 0x14) is formed before
//      the filename test (0x0040a555/0x0040a55a).
//    * a filename: type 1 and the name into the ref, sub_41ff90 (lttexmod's
//      texture lookup), the result into PanInfo+0; null => 0x3d with
//      "LT_NOTFOUND" and the FILENAME as the description (0x0040a5a2) --
//      RETURN_ERROR_SYM_DESC.  No filename: +0 = 0 (0x0040a5c1).
// =======================================================================

extern "C" void dsi_OnReturnError(int code);    // 0x0042d760
extern int   g_DebugLevel;                       // 0x004b0ab0
extern char* g_pErrorFormat;                     // 0x004a7110
void dsi_ServerError(char* pFormat, ...);        // 0x0042e6f0

#define LT_NOTFOUND_Q440      61
#define LT_INVALIDPARAMS_Q440 60

// clientmgr.h's CClientFileRef.
class CClientFileRefQ440
{
public:
	CClientFileRefQ440() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// clientmgr.h's PanInfo; +0 is the texture this sets.
class PanInfoQ440
{
public:
	void*  m_pTexture;                       // 0x00
	float  m_xOffset;                        // 0x04
	float  m_zOffset;                        // 0x08
	float  m_xScale;                         // 0x0c
	float  m_zScale;                         // 0x10
};

// 0x004b4460 is not a global of its own: it is clientrender.cpp's
// g_RenderStruct (0x004b4358) +0x108, the PanInfo[2] the renderer reads
// (clientrender.cpp's layout notes).  A view of that struct, cut to it.
class RenderStruct
{
public:
	uint8       m_pad_000[0x108];
	PanInfoQ440 m_GlobalPanInfo[2];          // 0x108  0x004b4460
};

extern RenderStruct g_RenderStruct;              // 0x004b4358

void* sub_41ff90Q440(CClientMgrSphereQ440* pClientMgr, CClientFileRefQ440* pRef); // 0x0041ff90

LTRESULT ci_SetGlobalPanTextureQ440(uint32 index, char* pFilename)
{
	CClientFileRefQ440 ref;
	PanInfoQ440* pInfo;

	if (index >= 2)
	{
		dsi_OnReturnError(LT_INVALIDPARAMS_Q440);
		if (g_DebugLevel >= 1)
			dsi_ServerError(g_pErrorFormat, "SetGlobalPanTexture", "60", " ");
		return LT_INVALIDPARAMS_Q440;
	}

	pInfo = &g_RenderStruct.m_GlobalPanInfo[index];

	if (pFilename)
	{
		ref.m_nFileType = 1;
		ref.m_pFilename = pFilename;

		pInfo->m_pTexture = sub_41ff90Q440(g_pClientMgrQ440, &ref);
		if (!pInfo->m_pTexture)
		{
			dsi_OnReturnError(LT_NOTFOUND_Q440);
			if (g_DebugLevel >= 1)
				dsi_ServerError(g_pErrorFormat, "SetGlobalPanTexture", "LT_NOTFOUND", pFilename);
			return LT_NOTFOUND_Q440;
		}
	}
	else
	{
		pInfo->m_pTexture = LTNULL;
	}

	return LT_OK_Q440;
}


// =======================================================================
//  ROW 4 of 8 -- 0x0041c4e0, 130 bytes, provenance `call`
//
//  BELONGS IN: src/engine/lithtech's CLIENTCONSOLE.CPP module.  The class
//  is ALREADY RECONSTRUCTED as `CcnConsole` in src/engine/lithtech/ccline.cpp
//  (lines 229..273) -- use that declaration, not the cut-down one below,
//  and drop the Q440 field names in favour of its.
//
//  MODULE PROOF (the class map already in the tree):
//    * 0x0041c520..0x0041c533 is `lea 0x3cc(%edi),%ecx ; push $0 ; call
//      0x0041c180`, and ccline.cpp already has `CcnHistory m_History; //
//      0x3cc` and `LTBOOL SetSize(int); // 0x0041c180`.
//    * 0x0041c4f4/0x0041c527 are 0x3c0 and 0x0041c52d is 0x3bc, which
//      ccline.cpp has as `CcnTextLine* m_pLines; // 0x3c0` and
//      `int m_nNumLines; // 0x3bc` -- the same pair CcnConsole::SetNumLines
//      (0x0041c910) grows and shrinks.
//    * 0x0041c543 is `lea 0x10(%edi),%ecx`, ccline.cpp's `CcnLine m_Line;
//      // 0x010`.
//    * the decoded object map puts clientconsole.obj at
//      0x0041c180 and ccline.obj at 0x0041c910, either side of this.
//
//  WHAT IT IS: the console's Term, with a flag that decides whether the
//  scrollback ring and the history go too.  Four callers: 0x0041b82a,
//  0x0041c395, 0x0041c3f6, 0x0041c411.  `ret $0x4`, this in ecx.
//
//  THE RING WALK is the cursor-alias walk, and the head is RELOADED
//  every iteration (`mov 0x3c0(%edi),%edx` at 0x0041c4fe) -- so the ternary
//  is written against the member, not against a cached local.  The
//  `sub/neg/sbb/and` at 0x0041c50c..0x0041c512 is VC6's branchless form of
//  `(p == head) ? NULL : p`.
//
//  THE THREE ZERO-STORE/CALL ORDERS ARE SOURCE.  m_pLines and m_nNumLines
//  are stored at 0x0041c527/0x0041c52d, BEFORE the SetSize call at
//  0x0041c533; VC6 cannot sink a store past a call it cannot prove
//  non-aliasing for, so the source order is stores-then-call.  Likewise the
//  four tail stores come out in the order 0x3b4, 0x3e4, 0x268, 0x004.
// =======================================================================

void* dalloc(unsigned long nSize);            // 0x004290b0  (ccline.cpp:90)
void  dfree(void* pMem);                      // 0x004290c0  (ccline.cpp:91)

// ccline.cpp's CcnTextLine.
class CcnTextLineQ440
{
public:
	CcnTextLineQ440* m_pPrev;                // 0x000
	CcnTextLineQ440* m_pNext;                // 0x004
	uint8            m_Rest[0x10c - 0x008];  // 0x008
};

// ccline.cpp's CcnHistory, only the entry this body calls.
class CcnHistoryQ440
{
public:
	LTBOOL SetSize(int nSize);               // 0x0041c180
	uint8  m_pad_00[0x18];
};

// ccline.cpp's CcnLine, only the entry this body calls.  0x0041bb80 is
// `movl $0,(%ecx) ; ret` -- one store, and it is a CALL, so the member is
// declared here and DEFINED OUT OF CLASS (a non-template member
// declared out of class is never expanded at /Ob1).
class CcnLineQ440
{
public:
	void   ResetQ440();                      // 0x0041bb80
	uint32 m_Unknown00;                      // 0x000
	uint8  m_pad_04[0x130 - 0x004];
};

// 0x0041bb80, 7 bytes: `movl $0x0,(%ecx) ; ret`.  Its real home is
// clientconsole.cpp's CConsoleLine, between the constructor (0x0041bb30)
// and Clear (0x0041bb90); written here beside its one caller (name ours).
void CcnLineQ440::ResetQ440()
{
	m_Unknown00 = 0;
}

class CcnConsoleQ440
{
public:
	void   TermQ440(LTBOOL bFreeLines);      // 0x0041c4e0

	// Both are calls, so both are out-of-class definitions elsewhere.
	void   FreeSurfaceQ440();                // 0x0041c570 (releases m_hSurface)
	void   FreeUnknown29cQ440();             // 0x0041c8e0 (dfree's m_pUnknown29c)

	uint8            m_pad_000[0x004];
	uint32           m_Unknown004;           // 0x004
	uint8            m_pad_008[0x010 - 0x008];
	CcnLineQ440      m_Line;                 // 0x010
	uint8            m_pad_140[0x268 - 0x140];
	uint32           m_Unknown268;           // 0x268
	uint8            m_pad_26c[0x29c - 0x26c];
	void*            m_pUnknown29c;          // 0x29c
	uint8            m_pad_2a0[0x3a4 - 0x2a0];
	void*            m_hSurface;             // 0x3a4
	uint8            m_pad_3a8[0x3b4 - 0x3a8];
	uint32           m_Unknown3b4;           // 0x3b4
	uint8            m_pad_3b8[0x3bc - 0x3b8];
	int              m_nNumLines;            // 0x3bc
	CcnTextLineQ440* m_pLines;               // 0x3c0
	uint8            m_pad_3c4[0x3cc - 0x3c4];
	CcnHistoryQ440   m_History;              // 0x3cc
	void           (*m_pfnCommand)(char*);   // 0x3e4
};

// 0x0041c4e0 (name ours).
void CcnConsoleQ440::TermQ440(LTBOOL bFreeLines)
{
	// 0x0041c4e4, before the argument is even loaded (0x0041c4e9).
	FreeSurfaceQ440();

	if (bFreeLines)
	{
		CcnTextLineQ440* pCur = m_pLines;

		// 0x0041c4fa is the rotated pre-test, 0x0041c51c the bottom test.
		while (pCur)
		{
			CcnTextLineQ440* pLine = pCur;

			pCur = (pCur->m_pNext == m_pLines) ? LTNULL : pCur->m_pNext;

			dfree(pLine);
		}

		m_pLines    = LTNULL;                // 0x0041c527
		m_nNumLines = 0;                     // 0x0041c52d

		m_History.SetSize(0);                // 0x0041c533
	}

	FreeUnknown29cQ440();                    // 0x0041c53b
	m_Line.ResetQ440();                      // 0x0041c543

	m_Unknown3b4 = 0;                        // 0x0041c548
	m_pfnCommand = LTNULL;                   // 0x0041c54e
	m_Unknown268 = 0;                        // 0x0041c554
	m_Unknown004 = 0;                        // 0x0041c55a
}


// =======================================================================
//  ROWS 5-8 -- the dmextra.cpp / clientmusic block at 0x00442xxx
//
//  BELONGS IN: src/engine/lithtech/dmextra.cpp.  That file ALREADY has
//  CDMChunkQ440, CDMChunkListQ440, CDMBank, CDMSegment, dm_newQ440 and a
//  byte-identical CDMBandBankQ440::Grow (0x00442b50); src/engine/lithtech/clientmusic.h already has
//  CDMPendingNode, CDMPlayingNode and the bank list.  The declarations
//  repeated below are local stand-ins ONLY -- merge against the real ones.
//
//  MODULE PROOF: dmextra.cpp already cites 0x00442920, 0x004429d0,
//  0x00442aa0, 0x00442b50, 0x00442cc0 as this TU's, and clientmusic.h:196
//  declares `int Grow(); // 0x00442930`.  Both Grows here are
//  PROTOTYPES DECLARED AND NEVER DEFINED.
//
//  THE ARRAY COOKIE, which dmextra.cpp noticed and did not explain.  Three
//  of the five grows (0x004429d0, 0x00442aa0, 0x00442bd0) allocate
//  n*sizeof + 4, store n in the leading dword and return base+4; two
//  (0x00442930, 0x00442b50) allocate n*sizeof flat.  That is VC6's
//  `new T[n]`: the count cookie is emitted exactly when T has a DESTRUCTOR,
//  and the per-element stores after it are the constructor inlined into the
//  vector-ctor loop.  It cross-checks on all five:
//     0x00442930  0x14  no cookie  ctor: +8 = 0                 CDMPendingNode
//     0x004429d0  0x54  cookie     ctor: +8=0 +c=(char)0 +4c=0 +50=4  CDMSegment
//     0x00442aa0  0x10  cookie     ctor: +8 = 0, +c = 0         CDMPlayingNode
//     0x00442b50  0x10  no cookie  no ctor at all               CDMBand
//     0x00442bd0  0x50  cookie     ctor: +8=0 +c=(char)0 +4c=0
//  and clientmusic.h's CDMSegment is {m_pDMSegment 0x08, m_szName 0x0c,
//  m_pName 0x4c, m_nStopEnact 0x50} -- the 0x004429d0 ctor writes exactly
//  those four, with 4 as the default enact type.  A class with an owned
//  `char* m_pName` is precisely a class with a destructor.
//
//  Which allocator 0x004290b0 / 0x004290c0 actually are (operator new /
//  operator new[] / dalloc) does not matter: the `call rel32` is a masked
//  relocation.  dmextra.cpp reaches them as dm_newQ440 and ccline.cpp as
//  dalloc/dfree; `new T[n]` emits the same three bytes of `push/call/add`.
// =======================================================================

class CDMChunkQ440
{
public:
	CDMChunkQ440* m_pNext;                   // 0x000
	CDMChunkQ440* m_pPrev;                   // 0x004
	void*         m_pRecords;                // 0x008
};

class CDMChunkListQ440
{
public:
	CDMChunkQ440* m_pHead;                   // 0x000
	CDMChunkQ440* m_pTail;                   // 0x004

	void AddHead(CDMChunkQ440* pChunk);      // 0x00442cc0 (already identical)
	void Remove(CDMChunkQ440* pChunk);       // 0x00442c90 -- ROW 8
};

class CDMBankQ440
{
public:
	int    m_bInitialized;                   // 0x000
	uint32 m_nGrowCount;                     // 0x004
	CDMChunkListQ440 m_Chunks;               // 0x008
	void*  m_pFree;                          // 0x010
};


// ----------------------------------------------------------------------
//  ROW 8 -- 0x00442c90, 47 bytes, provenance `call`.  A REAL FUNCTION.
//
//  IT IS CDMChunkListQ440::Remove, and the four call sites prove it:
//  0x0043fb4c sets ecx to 0x004b3fd0 = g_PendingBank(0x004b3fc8) + 8 =
//  &g_PendingBank.m_Chunks, then frees the argument's +0x08 (the record
//  array) and then the argument itself.  0x0043fb91 does the same on
//  0x004b3f58 = g_SegmentBank + 8, 0x0043fbdb on 0x004b3fb8 = g_BandBank+8,
//  0x0043fc90 on the fourth bank.  All four are inside Term (0x0043fb2e).
//
//  IT IS BYTE-FOR-BYTE 0x0048dee0, the other list's Remove that
//  clientmusic.h already documents -- 47 identical bytes at two addresses,
//  two classes, two COMDATs.  That gives this row a free oracle: whatever
//  source reproduces one reproduces the other.
//
//  The `push %esi ... pop %esi` inside the first arm (0x0043... 0x00442c9b,
//  0x00442ca0) is a scratch register wanted only there; the reload of
//  `(%eax)` at 0x00442ca7 on BOTH paths says the member expressions are
//  written out at each use rather than cached.
// ----------------------------------------------------------------------

void CDMChunkListQ440::Remove(CDMChunkQ440* pChunk)
{
	if (pChunk->m_pPrev)                          // 0x00442c94
		pChunk->m_pPrev->m_pNext = pChunk->m_pNext;   // 0x00442c9e
	else
		m_pHead = pChunk->m_pNext;                    // 0x00442ca5

	if (pChunk->m_pNext)                          // 0x00442ca9
		pChunk->m_pNext->m_pPrev = pChunk->m_pPrev;   // 0x00442cb0
	else
		m_pTail = pChunk->m_pPrev;                    // 0x00442cb9
}


// ----------------------------------------------------------------------
//  ROW 6 -- 0x00442947 is NOT a function.  The real function is
//  0x00442930..0x004429ce, 159 bytes: CDMBank::Grow for the PENDING bank.
//  See the not-a-function rows at the bottom of this file.
//
//  The record is 0x14 bytes (`lea (%edi,%edi,4),%eax ; shl $0x2,%eax` at
//  0x0044294b, `add $0x14,%ecx` at 0x0044296f, `add $0x14,%eax` at
//  0x00442998) with one constructed field at +8 and NO array cookie, which
//  is clientmusic.h's CDMPendingNode {m_pNext 0, m_pPrev 4, m_nType 8,
//  m_pSegment 0xc, m_nValue 0x10} with m_nType zeroed and no destructor.
//
//  Its seven callers are all in ChangeIntensity's TU: 0x0043fa18 (Init),
//  0x00440763 / 0x00440813 / 0x0044085f / 0x004408be (inside
//  ChangeIntensity, 0x00440620..0x0044093d), 0x00440a3d, 0x00440cfe.
//  clientmusic.h already names g_PendingBank (0x004b3fc8) as one of the two
//  banks whose grows are 0x00442930 and 0x00442aa0.
//
//  THE BODY IS CDMBandBankQ440::Grow's, which is already byte-identical in
//  dmextra.cpp; only the record type changes.  Do not restructure it.
// ----------------------------------------------------------------------

// clientmusic.h's CDMPendingNode.  A default constructor that zeroes
// m_nType and NO destructor -- see the cookie note above.
class CDMPendingNodeQ440
{
public:
	CDMPendingNodeQ440() { m_nType = 0; }    // 0x00442969 `movl $0,(%ecx)`

	CDMPendingNodeQ440* m_pNext;             // 0x000
	CDMPendingNodeQ440* m_pPrev;             // 0x004
	uint32              m_nType;             // 0x008
	void*               m_pSegment;          // 0x00c
	int                 m_nValue;            // 0x010
};

class CDMPendingBankQ440 : public CDMBankQ440
{
public:
	int Grow();                              // 0x00442930
};

int CDMPendingBankQ440::Grow()
{
	CDMChunkQ440* pChunk = new CDMChunkQ440;  // push $0xc, 0x00442934

	if (!pChunk)                              // 0x00442940
		return 0;

	CDMPendingNodeQ440* pRecords = new CDMPendingNodeQ440[m_nGrowCount];

	pChunk->m_pRecords = pRecords;            // 0x0044297b
	                                          // (stored before the test --
	                                          //  0x00442979 tests first, the
	                                          //  store is scheduled between)
	if (!pRecords)
		return 0;

	for (uint32 i = 0; i < m_nGrowCount; i++) // 0x00442984, unsigned (`jbe`)
	{
		*(void**)pRecords = m_pFree;          // 0x00442990
		m_pFree = pRecords;                   // 0x00442995
		pRecords++;                           // 0x00442998 add $0x14
	}

	pChunk->m_pNext = m_Chunks.m_pHead;       // 0x004429aa
	pChunk->m_pPrev = LTNULL;                 // 0x004429a3

	if (m_Chunks.m_pHead)                     // 0x004429af
		m_Chunks.m_pHead->m_pPrev = pChunk;   // 0x004429b3
	else
		m_Chunks.m_pTail = pChunk;            // 0x004429c1

	m_Chunks.m_pHead = pChunk;                // 0x004429b6 / 0x004429c4 (both arms)

	return 1;                                 // 0x004429ba / 0x004429c8
}


// ----------------------------------------------------------------------
//  ROW 7 -- 0x00442ab7 is NOT a function.  The real function is
//  0x00442aa0..0x00442b4e, 175 bytes: CDMBank::Grow for the PLAYING bank.
//  See the not-a-function rows at the bottom of this file.
//
//  Record 0x10 bytes (`shl $0x4,%eax` at 0x00442abd, `add $0x10` at
//  0x00442aed and 0x00442b18) WITH the array cookie (`add $0x4,%eax` at
//  0x00442ac0, `mov %edi,(%eax)` at 0x00442ad3, `lea 0x4(%eax),%edx` at
//  0x00442ad7), constructing +8 and +0xc.  That is clientmusic.h's
//  CDMPlayingNode {m_pNext 0, pad 4, m_pSegState 8, m_pSegment 0xc} with a
//  destructor -- consistent with m_pSegState being a COM interface the node
//  has to Release.
//
//  Four callers: 0x0043f6f4, 0x0043fa5e, 0x00440a97, 0x00440d5c.
// ----------------------------------------------------------------------

struct IDirectMusicSegmentStateQ440;
class  CDMSegmentQ440;

class CDMPlayingNodeQ440
{
public:
	// 0x00442ae0 / 0x00442ae7, in that order.
	CDMPlayingNodeQ440() { m_pSegState = LTNULL; m_pSegment = LTNULL; }
	// Its existence is what emits the array cookie, and nothing in the
	// image calls it (no out-of-line destructor ships for any of the three
	// banks' records), so it is written in class, empty.
	~CDMPlayingNodeQ440() {}

	CDMPlayingNodeQ440*             m_pNext;      // 0x000
	uint8                           m_pad_04[4];  // 0x004
	IDirectMusicSegmentStateQ440*   m_pSegState;  // 0x008
	CDMSegmentQ440*                 m_pSegment;   // 0x00c
};

class CDMPlayingBankQ440 : public CDMBankQ440
{
public:
	int Grow();                              // 0x00442aa0
};

int CDMPlayingBankQ440::Grow()
{
	CDMChunkQ440* pChunk = new CDMChunkQ440;  // push $0xc, 0x00442aa4

	if (!pChunk)                              // 0x00442ab0
		return 0;

	CDMPlayingNodeQ440* pRecords = new CDMPlayingNodeQ440[m_nGrowCount];

	pChunk->m_pRecords = pRecords;            // 0x00442afb

	if (!pRecords)                            // 0x00442af9
		return 0;

	// A separate cursor, not pRecords++: with the new[] result itself
	// advanced, VC6 coalesces the cookie arm's raw block and base+4 into one
	// register (`add $4,%eax`, 173 bytes); shipped keeps the raw block in
	// eax and base+4 in edx (`lea 4(%eax),%edx` 0x00442ad7, `mov %edx,%eax`
	// 0x00442af3). Indexing pRecords[i] instead fixes that but schedules the
	// `inc` first.
	CDMPlayingNodeQ440* pRec = pRecords;

	for (uint32 i = 0; i < m_nGrowCount; i++) // 0x00442b04, unsigned (`jbe`)
	{
		*(void**)pRec = m_pFree;              // 0x00442b10
		m_pFree = pRec;                       // 0x00442b15
		pRec++;                               // 0x00442b18 add $0x10
	}


	pChunk->m_pNext = m_Chunks.m_pHead;       // 0x00442b2a
	pChunk->m_pPrev = LTNULL;                 // 0x00442b23

	if (m_Chunks.m_pHead)                     // 0x00442b2f
		m_Chunks.m_pHead->m_pPrev = pChunk;   // 0x00442b33
	else
		m_Chunks.m_pTail = pChunk;            // 0x00442b41

	m_Chunks.m_pHead = pChunk;                // 0x00442b36 / 0x00442b44

	return 1;
}


// ----------------------------------------------------------------------
//  0x004429ed and 0x00442bed are NOT functions.  Each is the
//  interior block that follows the `push $0xc ; call ; test ; jne` head of
//  a Grow the enumerator cut at the allocation-failure branch -- the same
//  defect as 0x00442947 / 0x00442ab7.  The real functions, both written
//  here, are
//    0x004429d0..0x00442a94, 197 bytes: the SEGMENT bank's Grow, 0x54
//      records WITH the array cookie (`lea 0x4(,%eax,4)` at 0x004429fd).
//    0x00442bd0..0x00442c82, 179 bytes: the 0x50-byte record bank's Grow
//      (`lea (%edi,%edi,4) ; shl $0x4 ; add $0x4` at 0x00442bf1).
//  Both belong in dmextra.cpp beside CDMBandBankQ440::Grow.
// ----------------------------------------------------------------------

// clientmusic.h's CDMSegment: m_pDMSegment 0x08, m_szName 0x0c, m_pName
// 0x4c, m_nStopEnact 0x50.  The inlined ctor writes exactly those four at
// 0x00442a26..0x00442a2e; 4 is the default enact type.
class CDMSegmentRecQ440
{
public:
	CDMSegmentRecQ440()
	{
		m_pDMSegment = LTNULL;
		m_szName[0]  = 0;
		m_pName      = LTNULL;
		m_nStopEnact = 4;
	}
	~CDMSegmentRecQ440() {}                  // emits the cookie; see above

	CDMSegmentRecQ440* m_pNext;              // 0x000
	CDMSegmentRecQ440* m_pPrev;              // 0x004
	void*              m_pDMSegment;         // 0x008
	char               m_szName[0x40];       // 0x00c
	char*              m_pName;              // 0x04c
	uint32             m_nStopEnact;         // 0x050
};

// The 0x50-byte record: CDMSegment's layout without m_nStopEnact (the
// ctor at 0x00442c17..0x00442c1c writes +8, +c as a byte, +4c).
class CDMRec50Q440
{
public:
	CDMRec50Q440()
	{
		m_pObject   = LTNULL;
		m_szName[0] = 0;
		m_pName     = LTNULL;
	}
	~CDMRec50Q440() {}                       // emits the cookie; see above

	CDMRec50Q440* m_pNext;                   // 0x000
	CDMRec50Q440* m_pPrev;                   // 0x004
	void*         m_pObject;                 // 0x008
	char          m_szName[0x40];            // 0x00c
	char*         m_pName;                   // 0x04c
};

class CDMSegmentBankQ440 : public CDMBankQ440
{
public:
	int Grow();                              // 0x004429d0
};

class CDMRec50BankQ440 : public CDMBankQ440
{
public:
	int Grow();                              // 0x00442bd0
};

int CDMSegmentBankQ440::Grow()
{
	CDMChunkQ440* pChunk = new CDMChunkQ440;  // push $0xc, 0x004429d5

	if (!pChunk)                              // 0x004429e3
		return 0;

	CDMSegmentRecQ440* pRecords = new CDMSegmentRecQ440[m_nGrowCount];

	pChunk->m_pRecords = pRecords;            // 0x00442a3f

	if (!pRecords)                            // 0x00442a3d
		return 0;

	for (uint32 i = 0; i < m_nGrowCount; i++) // 0x00442a50, unsigned (`jbe`)
	{
		*(void**)pRecords = m_pFree;          // 0x00442a57
		m_pFree = pRecords;                   // 0x00442a5c
		pRecords++;                           // 0x00442a5f add $0x54
	}

	pChunk->m_pNext = m_Chunks.m_pHead;       // 0x00442a6d
	pChunk->m_pPrev = LTNULL;                 // 0x00442a6a

	if (m_Chunks.m_pHead)                     // 0x00442a73
		m_Chunks.m_pHead->m_pPrev = pChunk;   // 0x00442a77
	else
		m_Chunks.m_pTail = pChunk;            // 0x00442a86

	m_Chunks.m_pHead = pChunk;                // 0x00442a7a / 0x00442a89

	return 1;
}

int CDMRec50BankQ440::Grow()
{
	CDMChunkQ440* pChunk = new CDMChunkQ440;  // push $0xc, 0x00442bd5

	if (!pChunk)                              // 0x00442be3
		return 0;

	CDMRec50Q440* pRecords = new CDMRec50Q440[m_nGrowCount];

	pChunk->m_pRecords = pRecords;            // 0x00442c2d

	if (!pRecords)                            // 0x00442c2b
		return 0;

	for (uint32 i = 0; i < m_nGrowCount; i++) // 0x00442c3e, unsigned (`jbe`)
	{
		*(void**)pRecords = m_pFree;          // 0x00442c45
		m_pFree = pRecords;                   // 0x00442c4a
		pRecords++;                           // 0x00442c4d add $0x50
	}

	pChunk->m_pNext = m_Chunks.m_pHead;       // 0x00442c5b
	pChunk->m_pPrev = LTNULL;                 // 0x00442c58

	if (m_Chunks.m_pHead)                     // 0x00442c61
		m_Chunks.m_pHead->m_pPrev = pChunk;   // 0x00442c65
	else
		m_Chunks.m_pTail = pChunk;            // 0x00442c74

	m_Chunks.m_pHead = pChunk;                // 0x00442c68 / 0x00442c77

	return 1;
}


// ----------------------------------------------------------------------
//  ROW 5 -- 0x004425e0 is NOT a function.  It is the middle of the
//  instruction `movl $0x0,0xe8(%esi)` at 0x004425d9 (bytes
//  c7 86 e8 00 00 00 00 00 00 00): the 0xe8 of the +0xe8 DISPLACEMENT was
//  read as a call opcode, and `e8 00 00 00 00` at 0x004425db "targets"
//  0x004425e0.  A known enumeration defect; see the not-a-function rows below.
//
//  THE REAL FUNCTION IS 0x004425b0..0x004426b2, 259 bytes -- so the
//  neighbouring row `0x004425b0,48` is ALSO wrong (truncated head), and the
//  two rows tile the whole of it.  It is
//  CLTDirectMusicMgr::ReadReverbParameters, which clientmusic.h:571 already
//  declares at 0x004425b0 and which is not defined anywhere in the tree.
//  This body belongs in
//  src/engine/lithtech/dmextra.cpp next to SetReverbParamsQ440.
//
//  IT RETURNS LTBOOL, not void: `mov $0x1,%eax` at 0x004426a7 sits in the
//  single epilogue and is not dead.  clientmusic.h's `void` declaration is
//  wrong and should be corrected there.
//
//  DEFAULTS, read at the address each is stored at:
//    0x004425d9  m_bReverbOn                     = 0
//    0x004425e3  m_ReverbParams.fInGain          = 0.0f
//    0x004425e9  m_ReverbParams.fHighFreqRTRatio = 0xc1200000 = -10.0f
//    0x004425f0  m_ReverbParams.fReverbMix       = 0x447a0000 = 1000.0f
//    0x004425fa  m_ReverbParams.fReverbTime      = 0x3a83126f = 0.001f
//  (Monolith's defaults, not Microsoft's; the four constants are the DMUS
//  documented set but attached to different fields.  Only this one site in
//  the whole image writes 0x447a0000, so there is nothing to cross-check
//  them against and they are transcribed as they ship.)
//
//  The two hoisted `lea`s (edi = this+0xec at 0x004425c4, ebp = this+0xf8
//  at 0x004425ca) are the two member addresses that get used twice -- once
//  as a default store and once as GetKeyVal's reference argument.  The
//  other two members get a fresh `lea` at each use; that is register
//  pressure, not a different source shape.
// ----------------------------------------------------------------------

// dmextra.cpp:103 already has this, with these field offsets.
struct DMUS_WAVES_REVERB_PARAMS_Q440
{
	float fInGain;                           // 0x000
	float fReverbMix;                        // 0x004
	float fReverbTime;                       // 0x008
	float fHighFreqRTRatio;                  // 0x00c
};

// controlfile.cpp:311 already has this class with both overloads.
class CControlFileMgr
{
public:
	int GetKeyVal(const char* sSectionName, const char* sKeyName,
	              float& nVal);                                    // 0x00491ee0
	int GetKeyVal(const char* sSectionName, const char* sKeyName,
	              char* sVal, int nMaxStrLen);                     // 0x00491f20
};

extern "C" int __cdecl _stricmp(const char* s1, const char* s2); // 0x00492f30

// iltdirectmusic.h's LTDMEnactTypes.
enum LTDMEnactTypesQ440
{
	LTDMEnactInvalid = 0,
	LTDMEnactDefault,
	LTDMEnactImmediatly,
	LTDMEnactNextBeat,
	LTDMEnactNextMeasure,
	LTDMEnactNextGrid,
	LTDMEnactNextSegment
};

class CLTDirectMusicMgrQ440
{
public:
	LTBOOL ReadReverbParametersQ440(CControlFileMgr* pControlFile); // 0x004425b0
	LTBOOL SetReverbParamsQ440(DMUS_WAVES_REVERB_PARAMS_Q440 params);   // 0x00442340
	LTBOOL EnableReverbQ440();                                          // 0x004423e0

	uint32 EnactToSegFlagsQ440(const LTDMEnactTypesQ440 nEnact);         // 0x004422e0

	uint8  m_pad_000[0x0e8];
	LTBOOL m_bReverbOn;                      // 0x0e8
	DMUS_WAVES_REVERB_PARAMS_Q440 m_ReverbParams;  // 0x0ec .. 0x0fb
};

LTBOOL CLTDirectMusicMgrQ440::ReadReverbParametersQ440(CControlFileMgr* pControlFile)
{
	// `sub $0x8,%esp` at 0x004425b0 is the whole frame and the buffer is at
	// offset 0 (0x004425b3 and, after four pushes, 0x00442609).  Eight bytes
	// of buffer against a length argument of 4 -- see the coin-flip note.
	char szValue[8];

	m_bReverbOn = LTFALSE;

	m_ReverbParams.fInGain          = 0.0f;
	m_ReverbParams.fHighFreqRTRatio = -10.0f;
	m_ReverbParams.fReverbMix       = 1000.0f;
	m_ReverbParams.fReverbTime      = 0.001f;

	// 0x004425c1..0x00442604.  Section name is LTNULL, length is 4.
	pControlFile->GetKeyVal(LTNULL, "REVERB", szValue, 4);

	// 0x00442609..0x0044261d.  "ON" is 0x004abe98, four bytes before
	// "REVERB" at 0x004abe9c.
	if (_stricmp(szValue, "ON") == 0)
		m_bReverbOn = LTTRUE;                // 0x0044261f

	pControlFile->GetKeyVal(LTNULL, "REVERBINGAIN",
	                        m_ReverbParams.fInGain);            // 0x00442633
	pControlFile->GetKeyVal(LTNULL, "REVERBHIGHFREQRTRATIO",
	                        m_ReverbParams.fHighFreqRTRatio);   // 0x00442642
	pControlFile->GetKeyVal(LTNULL, "REVERBMIX",
	                        m_ReverbParams.fReverbMix);         // 0x00442657
	pControlFile->GetKeyVal(LTNULL, "REVERBTIME",
	                        m_ReverbParams.fReverbTime);        // 0x0044266c

	// 0x00442671..0x0044268e: the sixteen bytes go by value, four dwords
	// through a 0x10-byte hole punched with `sub $0x10,%esp`.
	SetReverbParamsQ440(m_ReverbParams);

	if (m_bReverbOn)                         // 0x00442693
		EnableReverbQ440();                  // 0x0044269f

	return LTTRUE;                           // 0x004426a7
}



// =======================================================================
//  0x004422e0, 88 bytes -- CLTDirectMusicMgr::EnactToSegFlags (name
//  ours, clientmusic.h:540 declares it, clientmusic.cpp calls it
//  from PlaySecondary and PlayMotif, never defined).
//
//  BELONGS IN: src/engine/lithtech/clientmusic.cpp's module (dmextra.cpp
//  block, beside SetReverbParams 0x00442340).  thiscall with `this` unused:
//  one stack argument, `ret $0x4`.
//
//  `dec ; cmp $0x5 ; ja` plus a six-entry table at 0x00442320: entry 1
//  (LTDMEnactImmediatly) points at the shared zero return 0x00442319, so it
//  is a case of its own that returns 0; Invalid falls to the default.
//  The values are the DMUS_SEGF_ timing flags.
// =======================================================================

#define DMUS_SEGF_SEGMENTEND_Q440  0x0100
#define DMUS_SEGF_GRID_Q440        0x0800
#define DMUS_SEGF_BEAT_Q440        0x1000
#define DMUS_SEGF_MEASURE_Q440     0x2000
#define DMUS_SEGF_DEFAULT_Q440     0x4000

uint32 CLTDirectMusicMgrQ440::EnactToSegFlagsQ440(const LTDMEnactTypesQ440 nEnact)
{
	switch (nEnact)
	{
		case LTDMEnactDefault:     return DMUS_SEGF_DEFAULT_Q440;     // 0x004422f1
		case LTDMEnactImmediatly:  return 0;                          // 0x00442319
		case LTDMEnactNextBeat:    return DMUS_SEGF_BEAT_Q440;        // 0x004422f9
		case LTDMEnactNextMeasure: return DMUS_SEGF_MEASURE_Q440;     // 0x00442301
		case LTDMEnactNextGrid:    return DMUS_SEGF_GRID_Q440;        // 0x00442309
		case LTDMEnactNextSegment: return DMUS_SEGF_SEGMENTEND_Q440;  // 0x00442311
	}

	return 0;
}


// =======================================================================
//  VERDICTS, EXTENT DEFECTS AND EXCLUSION LINES
// =======================================================================
//
//  addr        rows  verdict
//  ----------  ----  -------------------------------------------------
//  0x004425e0   211  NOT A FUNCTION.  False call target inside a memory
//                    displacement.  The real function is 0x004425b0,
//                    259 bytes (rows 0x004425b0/48 + 0x004425e0/211).
//                    ReadReverbParameters written above.
//  0x00442947   136  NOT A FUNCTION.  Interior block of 0x00442930
//                    (159 bytes).  Whole function written above.
//  0x00442ab7   152  NOT A FUNCTION.  Interior block of 0x00442aa0
//                    (175 bytes).  Whole function written above.
//  0x00442c90    47  REAL.  CDMChunkListQ440::Remove.  Written.
//  0x0041c4e0   130  REAL.  CcnConsole::Term(LTBOOL).  Written.
//  0x0041eb50    47  REAL.  cnt_GetFrequency.  Written.  UNBLOCKS 0x0042c6d0.
//  0x00402910    57  REAL.  CClassMgr::FindClass.  Written.
//  0x0040a3d0    46  REAL.  ci_FindObjectsInSphere's callback.  Written.
//
//  EXTENT DISCIPLINE, performed on every row by sweeping every e8/e9/0f8x
//  and every short jcc in .text for displacements landing in the range, and
//  every dword in .rdata/.data for the address:
//
//   0x004425b0..0x004426b2  one external caller (0x0043ffe1), four internal
//       short jumps, last instruction `ret $0x4` ending at 0x004426b2 with
//       nops 0x004426b3..0x004426bf.  The only "call" into 0x004425e0 comes
//       from offset 0x004425db, i.e. from inside 0x004425d9's instruction.
//   0x00442930..0x004429ce  seven external callers, all to 0x00442930; the
//       only reference to 0x00442947 is the `jne` at 0x00442942.  Ends on
//       `ret` at 0x004429ce, nop at 0x004429cf.
//   0x00442aa0..0x00442b4e  four external callers, all to 0x00442aa0; the
//       only reference to 0x00442ab7 is the `jne` at 0x00442ab2.  Ends on
//       `ret` at 0x00442b4e, nop at 0x00442b4f.
//   0x00442c90..0x00442cbe  four callers (0x0043fb51, 0x0043fb96,
//       0x0043fbe0, 0x0043fc90), three internal short jumps, both tails
//       `ret $0x4`, last ends 0x00442cbe, nop at 0x00442cbf.
//   0x0041c4e0..0x0041c561  four callers, four internal short jumps, ends
//       `ret $0x4` at 0x0041c561, 14 nops after.
//   0x0041eb50..0x0041eb7e  four callers, all inside 0x0042c6d0.  No branch
//       leaves or enters.  (My raw scanner reported a short jump from
//       0x0041eaf7; that byte is the `76` of `lea (%esi,%esi,2),%edx` at
//       0x0041eaf5 -- a false edge, verified by disassembly.)
//   0x00402910..0x00402948  one caller (0x00475690), two internal jumps,
//       ends `ret $0x4` at 0x00402948, nops to 0x0040294f.
//   0x0040a3d0..0x0040a3fd  NO call or jump edges at all and no data
//       reference; its single reference is `push $0x40a3d0` at 0x0040a2f8,
//       a .text immediate.  Ends `ret` at 0x0040a3fd, nops after.  Cleared
//       as a callback, not as an ASCII accident: 0x0040a3d0 reads back as
//       the bytes d0 a3 40 00, which are not printable at all.
//
//  NOT-A-FUNCTION ROWS:
//
// 0x004425e0,"not a function: false call target inside a MEMORY DISPLACEMENT. The instruction at 0x004425d9 is c7 86 e8 00 00 00 00 00 00 00 = movl $0x0,0xe8(%esi), and the 0xe8 of the +0xe8 displacement plus the four zero bytes of the immediate decode as e8 00 00 00 00 = call 0x004425e0. Its only in-image reference is that offset 0x004425db. The real function is 0x004425b0..0x004426b2, 259 bytes, CLTDirectMusicMgr::ReadReverbParameters (clientmusic.h:571), reached by the one real call at 0x0043ffe1; the neighbouring row 0x004425b0/48 is its truncated head and the two rows tile it exactly. Body reconstructed in scratch w12_440000.cpp for dmextra.cpp."
// 0x00442947,"not a function: interior block. Its only reference in the whole image is the intra-function jne at 0x00442942, which is 5 bytes earlier. The real function is 0x00442930..0x004429ce, 159 bytes, CDMBank::Grow for the pending-node bank (clientmusic.h:196 declares it at 0x00442930 and never defines it), with seven external callers all landing on 0x00442930: 0x0043fa18, 0x00440763, 0x00440813, 0x0044085f, 0x004408be, 0x00440a3d, 0x00440cfe. Row 0x00442930/23 is its truncated head and the two rows tile it exactly. Body reconstructed in scratch w12_440000.cpp for dmextra.cpp."
// 0x00442ab7,"not a function: interior block. Its only reference in the whole image is the intra-function jne at 0x00442ab2, 5 bytes earlier. The real function is 0x00442aa0..0x00442b4e, 175 bytes, CDMBank::Grow for the playing-node bank, with four external callers all landing on 0x00442aa0: 0x0043f6f4, 0x0043fa5e, 0x00440a97, 0x00440d5c. Row 0x00442aa0/23 is its truncated head and the two rows tile it exactly. Body reconstructed in scratch w12_440000.cpp for dmextra.cpp."
//
//  THE SAME DEFECT, found while sweeping the block (these bodies are not
//  written here):
//    0x004429ed/168 is the interior of 0x004429d0 (197 bytes, the segment
//      bank's Grow, 0x54 records with the array cookie); rows
//      0x004429d0/29 + 0x004429ed/168 tile 0x004429d0..0x00442a94.
//    0x00442bed/150 is the interior of 0x00442bd0 (179 bytes, the 0x50-byte
//      record bank's Grow); rows 0x00442bd0/29 + 0x00442bed/150 tile
//      0x00442bd0..0x00442c82.
//    Both are the same `push $0xc ; call ; test ; jne` head that the
//    enumerator cut at the allocation-failure branch, i.e. exactly the
//    shape it cut 0x00442930 and 0x00442aa0 at.
//
//  ON 0x0041eb50 AND 0x004b3098: this
//  function is NOT an accessor over that table.  The 17-record 16-byte
//  table at 0x004b3098..0x004b31a7 is real and its shape is right
//  ({Counter*, char* name, uint32 min, uint32 max}; the loop at 0x0042c7cd
//  walks it with esi biased +8 from 0x004b30a0 to 0x004b31b0), but
//  0x0041eb50 takes no arguments and never touches it.  It is counter.obj's
//  frequency getter, and 0x0042c6d0 divides each counter's reading BY it.
//  The two globals at 0x004b31a8/0x004b31ac that sit immediately past the
//  table are separate min/max floats, not a record.
//
//  COIN FLIPS, both spellings given:
//   1. 0x0040a3d0's guard.  `if (pObject->m_nUnknown54) return;` and
//      `if (!pObject->m_nUnknown54) { ... }` are byte-identical here --
//      there is only one epilogue and the guarded region is the whole
//      body, so the usual epilogue count cannot separate them.  Written as
//      the early return.
//   2. ReadReverbParameters' buffer.  The frame is 8 bytes and the length
//      argument is 4.  `char szValue[8]` with a literal 4 is what is
//      written; `char szValue[5]`, `[6]`, `[7]` all round to the same
//      8-byte frame and are indistinguishable.  `char szValue[4]` is NOT
//      equivalent -- it should give `sub $0x4,%esp` (or a `push ecx`) --
//      so if the build comes back four bytes light on the prologue, that
//      is the knob.
//   3. ReadReverbParameters' default-store order.  Written in the shipped
//      order (fInGain, fHighFreqRTRatio, fReverbMix, fReverbTime) because
//      member-store order is source-reachable.  The
//      alternative is struct order (fInGain, fReverbMix, fReverbTime,
//      fHighFreqRTRatio) with VC6 sinking the fHighFreqRTRatio store onto
//      the preloaded ebp; if the three float stores come out permuted,
//      swap to struct order before touching anything else.
//   4. 0x00402910's guard is NOT a coin flip -- the zero return sits past
//      the only other `ret`, so the positive-guard spelling is forced.
