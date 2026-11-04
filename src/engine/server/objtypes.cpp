// server.dll — the object types' constructors and shared virtuals
// (reconstruction). Build /O1.
//
// See objtypes.h for how the hierarchy was established.
#if defined(PRIMAL_HUNT) && defined(LT_SERVER_BUILD)
// PH: Primal Hunt's server.dll saw the STL's <string> here (its copy of
// this translation unit ends in std::ctype<unsigned short>::id's guarded
// initializer, guard 0x1008ec84: 0x1003f1e7); lithtech.exe's does not.
#include <string>
#endif
#include <string.h>
#include "ltlink.h"
#include "objtypes.h"
#include "ltobject_internal.h"
#include "de_error.h"

// This file deliberately does NOT include servermgr.h, and that is a debt
// rather than a design. It was dropped because servermgr.h declared
// un390_Term as void where the shipped body returns zero, and a return type
// is not overloadable, so the conflict could not be worked around locally.
// That prototype is now corrected, which unblocks the re-fold -- but the
// re-fold is more than restoring the include: StructBank, the sb_* trio and
// CUnknown390 are all declared below AND in servermgr.h, and the copy below
// carries the better layout (three StructBanks at 0x14/0x30/0x4c, the eleven
// per-type banks at 0x68 with stride 0x24, their pointer table at 0x1f4, and
// eleven LTLists at 0x220 -- all read off un390_Init).
//
// Doing it properly means promoting that layout into servermgr.h, which
// every server TU includes, and re-verifying all of them. Left as one
// deliberate piece of work instead of half-done here, because two live
// definitions of one class is exactly the divergence that let a wrong float
// constant sit in netmgr.cpp for months.
//
// Related: objbank.cpp models the same 0x390 object manager as CObjMgrView
// for 0x1002592e / 0x10025984 / 0x10025a1b, with its own comment saying this
// file was not its to edit. Those three and this file's two belong together.

// servermgr.h models the attachment manager's spare-record list the same way
// and is not included here: the object manager at the bottom of this file
// needs signatures that header does not yet carry (see there).
class CAttachMgrView
{
public:
	uint8  m_pad_00[0x64];
	class Attachment* m_pFreeList;           // +0x64
};

// The default is the plain object; every other type hands its own OT_ value
// down instead.
CObjBase::CObjBase() : CObjRoot(0)
{
	InitFields();

	m_ObjectType = OT_NORMAL;
}

CObjBase::CObjBase(char nType) : CObjRoot(0)
{
	InitFields();

	m_ObjectType = nType;
}

// 0x100247a1 -- vtable slot 8. The whole of an ObjectCreateStruct that
// survives creation, in the order the shipped code writes it, which is the
// source's: the two flag words and the user flags (AvP2's addition; NOLF's
// engine left them to the caller), then the position, then the rotation,
// then the scale, then the type, then the manager. LTVector copies as a
// three-word block move (no user operator=) and LTRotation as four
// register moves, which is ltrotation.h's own operator= expanded.
void CObjBase::_v20(CAttachMgrView* pAttachMgr, ObjectCreateStruct* pStruct)
{
	m_Flags = pStruct->m_Flags;
	m_Flags2 = pStruct->m_Flags2;
	m_UserFlags = pStruct->m_UserFlags;
	m_Pos = pStruct->m_Pos;
	m_Rotation = pStruct->m_Rotation;
	m_Scale = pStruct->m_Scale;
	m_ObjectType = (char)pStruct->m_ObjectType;

	m_pAttachMgr = pAttachMgr;
}


// 0x10024802. Every field a fresh object starts with, in the order the
// shipped code writes them — which is not the layout's order, so it is the
// source's. Two things in it are worth pointing at:
//
//   * the link at 0x060 is tied off twice, with the 0x06c link's setup in
//     between. The second tie-off is dead: nothing between the two touches
//     0x060. It is reproduced because it is what shipped.
//   * (NOLF) m_StandingLink got its m_pData and was NOT tied off, alone
//     among the links here; AvP2's ties it off (below).
//
// Two things about the links are source, not codegen, and the function is
// six bytes short or size-exact-but-wrong without them. The m_pData store is
// a plain member store off `this` (`mov %ecx,0x74(%ecx)`), not the one
// LTLink::Init2 emits through the link's own address, so the source assigns
// the field rather than calling Init2. And the order of the two statements
// differs between the links: 0x06c is tied off and then given its data,
// while 0x150 and 0x170 are given their data first.
//
// The two magic numbers are the physics defaults: five for friction, thirty
// for mass, and the force-ignore limit is stored squared, so 1000000 is a
// limit of a thousand.
// AVP2: the fields objtypes.h still leaves as padding (the header is shared
// with files that are not this one's), as InitFields writes them: a second
// 0xffff word beside the object ID, a vector after m_v138 and a rotation
// after m_f164 (names ours).
struct CObjBaseFieldsQ518
{
	uint8      m_pad_000[0xba];
	uint16     m_Unknownba;                  // 0x0ba
	uint8      m_pad_0bc[0x148 - 0x0bc];
	LTVector   m_v148;                       // 0x148
	uint8      m_pad_154[0x168 - 0x154];
	LTRotation m_r168;                       // 0x168
};

// AVP2 (lithtech 0x0046bfa0, 523 bytes; AvP2 1.0.9.6 0x00466a20 is the same
// bytes): the stores run in layout order, the position and dimensions last,
// and four things NOLF's had not are in it -- the attachment manager is
// cleared first (0x0046bfa9), the type-list link at 0x07c is tied off, the
// standing link IS tied off now (0x0046c0d3), and the three new fields above
// are set. m_pUnknown17c (0x184) is no longer touched (but see below).
void CObjBase::InitFields()
{
	CObjBaseFieldsQ518* pFields = (CObjBaseFieldsQ518*)this;

	m_pAttachMgr = LTNULL;

	m_Link60.TieOff();

	m_Link6c.TieOff();
	m_bLinked6c = 0;
	m_Link6c.m_pData = this;

	m_Link60.TieOff();

	m_Link7c.TieOff();

	m_Flags = 0;
	m_Flags2 = 0;
	m_UserFlags = 0;

	m_ColorB = 255;
	m_ColorG = 255;
	m_ColorR = 255;
	m_ColorA = 255;

	m_pAttachments = LTNULL;

	m_Rotation.Init();
	m_Scale.Init(1.0f, 1.0f, 1.0f);

	m_ObjectID = 0xffff;
	pFields->m_Unknownba = 0xffff;
	m_ObjectType = 0;
	m_BlockingPriority = 0;
	m_Unknownc0 = 0;

	m_Velocity.Init();
	m_Acceleration.Init();

	m_Unknowndc = 5;	// AVP2: Primal Hunt server.dll 0x1002e654

	m_FrictionCoefficient = 5.0f;
	m_Mass = 30.0f;
	m_ForceIgnoreLimitSqr = 1000000.0f;

	m_pStandingOn = LTNULL;
	m_pStandingOnPoly = LTNULL;

	m_vBoxMin.Init();
	m_vBoxMax.Init();

	m_StandingOnMe.TieOff();

	m_StandingLink.m_pData = this;
	m_StandingLink.TieOff();

	m_ChangeFlags = 0;
	m_n180 = 0;

	m_fLink150Key = 0.0f;
	m_f128 = 0.0f;
	m_v12c.Init();
	m_v138.Init();
	pFields->m_v148.Init();

	m_Link150.m_pData = this;
	m_Link150.TieOff();

	m_fLink170Key = 0.0f;
	m_f164 = 0.0f;
	pFields->m_r168.Init();

	m_Link170.m_pData = this;
	m_Link170.TieOff();

#ifdef LT_SERVER_BUILD
	// The server still clears it (server.dll 0x1002e766); lithtech.exe's
	// copy has no store to +0x184 at all.
	m_pUnknown17c = LTNULL;
#endif

	m_pSData = LTNULL;

	m_Pos.Init();
	m_Dims.Init(1.0f, 1.0f, 1.0f);
	m_Radius = 0.0f;
}

// Objects carrying the flag hand themselves to the visitor; the rest say so
// and are skipped.
LTBOOL CObjBase::_v04(CObjVisitor* pVisitor)
{
	if (m_Flags & 0x40)
	{
		pVisitor->Visit(this);

		return LTTRUE;
	}

	return LTFALSE;
}

// An object that carries the flag holds no engine-side memory of its own, so
// only the rest have anything to give back.
void CObjBase::_v08()
{
	if (!(m_Flags & 0x40))
		obj_ReleaseMemory((LTObject*)this);

	Unlink();
}

void CObjBase::GetDims(LTVector* pMin, LTVector* pMax)
{
	*pMin = m_vBoxMin;
	*pMax = m_vBoxMax;
}

// The engine-side data is what an object gets when it enters a world, so its
// presence is the answer.
LTBOOL CObjBase::IsInWorld()
{
	return m_pSData != LTNULL;
}

// 0x10024689.
float CObjBase::_v2c()
{
	return m_Radius;
}

// 0x10024cad.
LTBOOL CObjBase::_v30()
{
	return LTTRUE;
}

void CObjBase::GetTransform(LTMatrix* pMat)
{
	mat_SetupTransform(&m_Pos, &m_Rotation, &m_Scale, pMat);
}

// 0x100118d5. One builder for all three matrix kinds: whichever of the three
// parts is null is left out. With no rotation the matrix starts as the
// identity; with one it starts as that rotation's own 4x4. The scale then
// multiplies the upper 3x3 a column at a time — all three rows of column
// zero by x, then column one by y, then column two by z — and the position
// drops straight into the fourth column. Nothing writes the bottom row, so a
// null rotation leaves whatever the identity put there and a non-null one
// leaves whatever the conversion did.
void mat_SetupTransform(LTVector* pPos, LTRotation* pRot, LTVector* pScale,
                        LTMatrix* pMat)
{
	if (pRot)
		quat_ToMatrix2(pRot, pMat);
	else
		pMat->Identity();

	if (pScale)
	{
		pMat->m[0][0] *= pScale->x;
		pMat->m[1][0] *= pScale->x;
		pMat->m[2][0] *= pScale->x;

		pMat->m[0][1] *= pScale->y;
		pMat->m[1][1] *= pScale->y;
		pMat->m[2][1] *= pScale->y;

		pMat->m[0][2] *= pScale->z;
		pMat->m[1][2] *= pScale->z;
		pMat->m[2][2] *= pScale->z;
	}

	if (pPos)
	{
		pMat->m[0][3] = pPos->x;
		pMat->m[1][3] = pPos->y;
		pMat->m[2][3] = pPos->z;
	}
}


// The bank a released segment goes back to, as much of it as this file
// needs: servermgr.h's StructBank, whose intrusive free list is at +0x18.
// Declared locally because this file cannot include that header yet.
struct CSegmentBankView
{
	uint8 m_pad_00[0x18];
	void* m_pFreeList;                       // +0x18
};

// servermgr.h's sb_Free as AvP2 has it: both pointers are tested before the
// two stores, the record first (lithtech 0x0046dcb2 `test %eax`, then
// 0x0046dcb6 the bank).
inline void sb_Free(CSegmentBankView* pBank, void* pStruct)
{
	if (pStruct && pBank)
	{
		*(void**)pStruct = pBank->m_pFreeList;
		pBank->m_pFreeList = pStruct;
	}
}

// AVP2: the attachments go back to the manager's third struct bank, at its
// +0x4c (`lea 0x4c(%ecx),%edx`, lithtech 0x0046be6d; AvP2 1.0.9.6
// 0x00466905), through sb_Free -- the free list servermgr.h's
// CAttachMgrView names at +0x64 is that bank's own, +0x18 into it.
struct CAttachBankMgrViewQ518
{
	uint8            m_pad_00[0x4c];
	CSegmentBankView m_AttachBank;           // +0x4c
};

// 0x10024772. The attachments do not go back one at a time: the whole chain
// is pushed onto the manager's spare list, each record's first dword taken
// over as the free link (its LTransform offset, which nothing will read
// again). The head is re-read from the manager on every iteration because
// the store to it is what the next iteration links onto.
//
// THE LT_SERVER_BUILD SEAM. server.dll keeps this out of line (0x10024772,
// one caller, 0x100246e5) while lithtech.exe has NO copy of it: ~CObjBase at
// 0x0045a6f0 carries the loop inline (0x0045a700..0x0045a71d). One source
// cannot give both -- `inline` is expanded at /O1 as well as /O2 (measured:
// server's destructor goes 159 -> 186 bytes), and defining it after the
// destructor, `static inline` and `__inline` change nothing. So the inline
// is the client build's only, on the same stand-in discriminator
// serverengine.cpp and smgr.cpp use.
//
// AVP2: Primal Hunt's server.dll expands it too (0x1002e45c..0x1002e479,
// sb_Free's two tests and all), so the seam above is NOLF's only.
inline void obj_DetachAll(CAttachMgrView* pMgr, CObjBase* pObj)
{
	Attachment* pCur = pObj->m_pAttachments;

	while (pCur)
	{
		Attachment* pNext = pCur->m_pNext;

		sb_Free(&((CAttachBankMgrViewQ518*)pMgr)->m_AttachBank, pCur);

		pCur = pNext;
	}

	pObj->m_pAttachments = LTNULL;
}


CObjWorldModel::CObjWorldModel() : CObjBase(OT_WORLDMODEL)
{
	InitFields();
}

// Above InitFields' body, as the default constructor is: below it, VC6
// knows the call cannot throw and drops the /GX frame Primal Hunt's copy
// has (lithtech 0x0046c4b0).
CObjWorldModel::CObjWorldModel(char nType) : CObjBase(nType)
{
	InitFields();
}

// 0x10024b96. A world model repeats three of the colour stores the base's
// InitFields has already done — but not the alpha — clears the three
// pointers past the base, sets both transforms to the identity, and gives
// each of the five links the object as its data.
void CObjWorldModel::InitFields()
{
	uint32 i;

	m_ColorB = 255;
	m_ColorG = 255;
	m_ColorR = 255;

	m_pWorldBsp = LTNULL;
	m_pOriginalBsp = LTNULL;
	m_pWMIface = LTNULL;

	m_Transform1f0.Identity();
	m_Transform230.Identity();

	for (i = 0; i < 5; i++)
		((LTLink*)m_Links1b4)[i].Init2(this);
}

CObjContainer::CObjContainer() : CObjWorldModel(OT_CONTAINER)
{
	m_ContainerCode = 0xffff;
}

// A hundred units unless something says otherwise.
CObjLight::CObjLight() : CObjBase(OT_LIGHT)
{
	m_fLightRadius = 100.0f;
}

float CObjLight::GetLightRadius(void* pUnused)
{
	return m_fLightRadius;
}


// Coming apart: the attachments go first, then the object leaves whichever
// of the two lists it is in — the test is on the value beside each link, not
// on the link itself — and then the ordinary release runs, called directly
// rather than through the vtable because by now it is this class's. The base
// destructor that follows is the compiler's, not written here.
CObjBase::~CObjBase()
{
	if (m_pAttachMgr)
		obj_DetachAll(m_pAttachMgr, this);

	// AVP2: the first list is left unconditionally (0x0046be94); only the
	// second is still keyed on its value.
	{
		LTLink* pLink = &m_Link150;

		pLink->m_pPrev->m_pNext = pLink->m_pNext;
		pLink->m_pNext->m_pPrev = pLink->m_pPrev;

		pLink->m_pNext = pLink;
		pLink->m_pPrev = pLink;
	}

	if (m_fLink170Key != 0.0f)
	{
		LTLink* pLink = &m_Link170;

		pLink->m_pPrev->m_pNext = pLink->m_pNext;
		pLink->m_pNext->m_pPrev = pLink->m_pPrev;

		pLink->m_pNext = pLink;
		pLink->m_pPrev = pLink;
	}

#ifdef LT_SERVER_BUILD
	// The server still clears it (server.dll 0x1002e4d4); lithtech.exe's
	// destructor never reads +0x184 (0x0046bee9 goes straight to _v08).
	if (m_pUnknown17c)
		m_pUnknown17c = LTNULL;
#endif

	CObjBase::_v08();
}

CObjCanvas::CObjCanvas() : CObjBase(OT_CANVAS)
{
	m_pUnknown1a8 = LTNULL;
	m_pUnknown1ac = LTNULL;
	m_fUnknown1b0 = 1.0f;
}

// 0x100256c0 / 0x0045bb00.
float CObjCanvas::_v2c()
{
	return m_fUnknown1b0;
}

// The same body at the same address, from vtable slot 14 (+0x38).
float CObjCanvas::_v38()
{
	return m_fUnknown1b0;
}


// AVP2: the four words objtypes.h leaves as padding ahead of the field of
// view are cleared first (lithtech 0x0046d7b7..0x0046d7c9), and the flag is
// set before the two angles (names ours).
struct CObjCameraFieldsQ518
{
	uint8  m_pad_000[0x1b0];
	uint32 m_Unknown1b0;                     // 0x1b0
	uint32 m_Unknown1b4;                     // 0x1b4
	uint32 m_Unknown1b8;                     // 0x1b8
	uint32 m_Unknown1bc;                     // 0x1bc
};

CObjCamera::CObjCamera() : CObjBase(OT_CAMERA)
{
	CObjCameraFieldsQ518* pFields = (CObjCameraFieldsQ518*)this;

	pFields->m_Unknown1b0 = 0;
	pFields->m_Unknown1b4 = 0;
	pFields->m_Unknown1b8 = 0;
	pFields->m_Unknown1bc = 0;

	m_Unknown1c0 = 1;

	m_fFovY = m_fFovX = 1.5707964f;

	m_v1c4.Init();
}

// Store order is Primal Hunt's lithtech.exe (0x0046dc00): 1a8, 1ac, the
// cursors, then the three vectors and 218 in offset order -- NOLF's engine
// had the cursors first and the vectors out of order. The server's /O1 copy
// hoists its two `andl $0` stores to the top whatever the source order
// (identical). PH: IDENTICAL on lithtech.exe too with the storage address
// taken into a local first: under cl 8966 that is what puts its `lea` ahead
// of the shared zero's `xor` (0x0046dc0a); cursors-first, a chained `B = A =
// m_Storage` and member initialisers all left the two swapped. AvP2 1.0.9.6's
// cl 9044 build matches with the plain stores and loses it with the local, so
// the local is Primal Hunt's build only.
CObjLineSystem::CObjLineSystem() : CObjBase(OT_LINESYSTEM)
{
#ifdef PRIMAL_HUNT
	uint8* pStorage = m_Storage;

	m_pUnknown1a8 = LTNULL;
	m_pUnknown1ac = LTNULL;

	m_pCursorA = pStorage;
	m_pCursorB = pStorage;
#else
	m_pUnknown1a8 = LTNULL;
	m_pUnknown1ac = LTNULL;

	m_pCursorA = m_Storage;
	m_pCursorB = m_Storage;
#endif

	m_v1f4.Init();
	m_v200.Init();
	m_v20c.Init();
	m_f218 = 0.0f;
}


// ---- the object manager at CServerMgr+0x390 ------------------------------ //
//
// servermgr.h models this object as an opaque four bytes; what its Init and
// Term reach is all read off their own bytes. Three struct banks, then the
// eleven per-type object banks side by side (objbank.cpp reconstructs the
// template — a vptr, a StructBank and one more dword, 0x24 apiece), then a
// table of pointers to them, then one list per type. The constructor at
// 0x1002449b confirms the eleven: it stores a different vtable and a
// different record size into each, in the OT_ order objbank.cpp establishes.
//
// Declared here rather than in servermgr.h because the header is not ours to
// change; the two entry points below carry the signatures the bytes prove,
// which for Term is not the `void` servermgr.h currently declares — its
// epilogue puts a zero in eax.
class StructBank
{
public:
	uint8  m_pad_00[0x1c];
};

void sb_Init(StructBank* pBank, uint32 structSize, uint32 cacheSize);   // 0x1003d170
void sb_Init2(StructBank* pBank, uint32 structSize, uint32 cacheSize,
              uint32 growSize);                                         // 0x1003d1c0
void sb_Term(StructBank* pBank);                                        // 0x1003d230

// The interface the eleven banks are reached through; only slot 0x0c is
// called from here, and objbank.cpp names it Init — starting a bank over is
// also how it gives its chunks back, which is what makes it the teardown.
class IObjectBank
{
public:
	virtual void      _v00()=0;
	// Slot 4. objbank.cpp reads the same four-slot shape off .rdata and
	// names it Allocate; om_CreateObject below is the call site that shows
	// what it returns.
	virtual CObjBase* Allocate()=0;          // 0x04
	virtual void      _v08()=0;
	virtual void      Init()=0;              // 0x0c
};

// One bank as this side sees it: 0x24 bytes, laid out by objbank.cpp.
class CObjBankSlot
{
public:
	uint8 m_pad_00[0x24];
};

class CUnknown390
{
public:
	int          Lock();                     // 0x1002461c
	int          GetLockCount();             // 0x10017b0e (folded)

	uint8        m_pad_00[0x04];
	// A nesting counter that starts at -1 rather than zero, so the
	// outermost Lock is distinguishable from the ones inside it and can do
	// the one-off sweep. Name ours.
	int32        m_LockCount;                // 0x004
	LTLink       m_Link;                     // 0x008  its place in the global list
	StructBank   m_Bank14;                   // 0x014  0x3c-byte records
	StructBank   m_Bank30;                   // 0x030  0x44-byte records
	StructBank   m_Bank4c;                   // 0x04c  0x28-byte records
	// Eleven DISTINCT types, not an array of one -- the teardown at
	// 0x1002ca7c restores a different four-slot vftable for each, which is
	// why this stand-in is opaque. objbank.cpp's CObjBankSet is the same
	// eleven with their real types.
	CObjBankSlot m_Banks[11];                // 0x068  one per OT_ type
	IObjectBank* m_pBanks[11];               // 0x1f4  the same eleven, indexed
	LTList       m_Lists[11];                // 0x220  the objects of each type
};

// 0x1002461c. A member and not one of the un390_ free functions below: it
// takes no stack argument and reads `this` out of ecx, and it is reached
// only through a vtable slot (0x1004deb0), so the manager carries a vptr
// of its own at offset zero.
//
// Only the outermost lock does anything. The counter idles at -1; the call
// that lifts it off -1 sets it to one and walks all eleven per-type object
// lists clearing every object's 0x58, and every nested call after that
// just increments. The return is the new depth either way.
//
// The eleven-list walk is one loop with the inner list traversal's `pLink`
// initialiser hoisted to the outer loop's top, and `i` never survives
// as a counter -- VC6 strength-reduces it into the list pointer and a
// count-down, which is why the shipped code has a `push $0xb; pop %esi`
// and no index at all.
int CUnknown390::Lock()
{
	LTLink* pLink;
	int     i;

	if (m_LockCount == -1)
	{
		m_LockCount = 1;

		for (i = 0; i < 11; i++)
		{
			for (pLink = m_Lists[i].m_Head.m_pNext;
			     pLink != &m_Lists[i].m_Head;
			     pLink = pLink->m_pNext)
			{
				((CObjBase*)pLink->m_pData)->m_Unknown58 = 0;
			}
		}
	}
	else
	{
		m_LockCount++;
	}

	return m_LockCount;
}

// The manager's second slot: the lock depth, read. Four bytes (`mov
// 0x4(%ecx),%eax; ret`) identical to Model::GetFilename's, and the shipped
// slot names that function's address (0x10017b0e) -- identical COMDATs,
// folded by the linker -- so this body has no address of its own there
// (name ours).
int CUnknown390::GetLockCount()
{
	return m_LockCount;
}

// The two tables objbank.cpp's manager constructor writes into offset zero
// (see the note above CObjMgrVtblBase there): 0x1004df68, stored before the
// eleven banks, is two _purecall slots (0x1003ea24); 0x1004deb0, stored after
// them, is Lock and GetLockCount. Both are const, as the shipped .rdata. The
// second is a struct of member pointers because that is the only way to put
// a member function's address in static data; with single inheritance each
// is one code pointer, so the layout is the shipped eight bytes.
extern "C" int __cdecl _purecall(void);

extern void* const g_ObjMgrVtblA[2] =        // 0x1004df68
{
	(void*)_purecall,
	(void*)_purecall,
};

struct CObjMgrVtbl
{
	int (CUnknown390::*m_pfnLock)();         // 0x1002461c
	int (CUnknown390::*m_pfnGetLockCount)(); // 0x10017b0e
};

extern const CObjMgrVtbl g_ObjMgrVtbl =      // 0x1004deb0
{
	&CUnknown390::Lock,
	&CUnknown390::GetLockCount,
};


// Every manager built so far, so the teardown can find its own link by the
// object it carries (name ours; only these two functions reach it).
extern LTLink g_Un390List;                   // 0x10058780


// 0x100256c7 — build the manager. The flag picks the two big banks' sizes:
// a dedicated server gets the growable form with room for a thousand
// records, everything else the fixed 0x40-deep one.
//
// Three things read off the bytes rather than guessed:
//
//   * the pointer table is memset to zero and then every one of its eleven
//     slots is assigned, so the clear is dead. It is reproduced because it
//     is what shipped.
//   * the eleven banks are eleven separate assignments, not a loop: the
//     shipped code emits eleven lea/mov pairs, and the first one reuses the
//     memset's own argument register for the table base.
//   * the link goes on the global list with no element count touched, so
//     the head is a bare LTLink and not an LTList.
int un390_Init(CUnknown390* p, int nDedicated)
{
	int i;

	memset(p->m_pBanks, 0, sizeof(p->m_pBanks));

	p->m_pBanks[0] = (IObjectBank*)&p->m_Banks[0];
	p->m_pBanks[1] = (IObjectBank*)&p->m_Banks[1];
	p->m_pBanks[2] = (IObjectBank*)&p->m_Banks[2];
	p->m_pBanks[3] = (IObjectBank*)&p->m_Banks[3];
	p->m_pBanks[4] = (IObjectBank*)&p->m_Banks[4];
	p->m_pBanks[5] = (IObjectBank*)&p->m_Banks[5];
	p->m_pBanks[6] = (IObjectBank*)&p->m_Banks[6];
	p->m_pBanks[7] = (IObjectBank*)&p->m_Banks[7];
	p->m_pBanks[8] = (IObjectBank*)&p->m_Banks[8];
	p->m_pBanks[9] = (IObjectBank*)&p->m_Banks[9];
	p->m_pBanks[10] = (IObjectBank*)&p->m_Banks[10];

	if (nDedicated)
	{
		sb_Init2(&p->m_Bank14, 0x3c, 0x400, 0x400);
		sb_Init2(&p->m_Bank30, 0x44, 0x40, 0x40);
	}
	else
	{
		sb_Init(&p->m_Bank14, 0x3c, 0x40);
		sb_Init(&p->m_Bank30, 0x44, 0x40);
	}

	sb_Init2(&p->m_Bank4c, 0x28, 0x10, 0x20);

	for (i = 0; i < 11; i++)
		dl_InitList(&p->m_Lists[i]);

	p->m_Link.m_pData = p;
	dl_Insert(&g_Un390List, &p->m_Link);

	return 0;
}

// 0x100257f2 — tear it down. Each bank is restarted through its interface,
// which is also how it releases its chunks; the three struct banks go back
// wholesale; and the manager's own link is found on the global list by the
// object it carries rather than removed directly.
//
// That search is the shipped shape and not a simplification we missed: the
// link is a member of the very object being torn down, so `p->m_Link` would
// have done, but the code walks the list comparing m_pData against p and
// stops at the first match.
int un390_Term(CUnknown390* p)
{
	int i;

	for (i = 0; i < 11; i++)
		p->m_pBanks[i]->Init();

	sb_Term(&p->m_Bank14);
	sb_Term(&p->m_Bank30);
	sb_Term(&p->m_Bank4c);

	LTLink* pHead = &g_Un390List;
	LTLink* pCur;
	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		if (pCur->m_pData == p)
		{
			pCur->Remove();
			break;
		}
	}

	return 0;
}


// 0x10024b6f — the vptr store, one call, then the base. That order is the
// whole content: a derived destructor stamps its own vtable first, runs its
// body, and the base destructor is emitted after it, which is why the helper
// call sits between the two vptr stores rather than before either.
CObjWorldModel::~CObjWorldModel()
{
	sub_10024d41();
}

// 0x10024d41 — the world model's own teardown: take all five of its links
// out of whatever lists they are threaded on, then run the base's virtual
// _v08 as a *direct* call.
//
// Two things the bytes settle. The unlink is the SDK's own
// `CheapLTLink::Remove` inlined, not a hand-written one: both links are read
// twice, once for each of the two fix-ups, and the tie-off that follows
// stores `this` into m_pNext before m_pPrev — which is the right-to-left
// evaluation of `m_pPrev = m_pNext = this` and nothing else. And the trailing
// call reaches 0x10024a17 directly rather than through the vtable, so the
// source names the base explicitly; written as a plain `_v08()` it would be
// an indirect call through slot 8.
void CObjWorldModel::sub_10024d41()
{
	uint32 i;

	for (i = 0; i < 5; i++)
		((LTLink*)m_Links1b4)[i].Remove();

	CObjBase::_v08();
}


// ---- the object types' destructors --------------------------------------- //

// Eleven bytes each, and each is a vptr store followed by a tail jump into
// its base's destructor -- exactly what VC6 emits for an empty derived
// destructor on a class whose members need none. So the bodies really are
// empty, and what the image adds is the inheritance: CObjLight's and
// CObjCamera's jump to ~CObjBase while CObjContainer's jumps to
// ~CObjWorldModel, which independently confirms the declared base here.
//
// Each sits immediately after its own class's constructor, which is what
// identifies it: 0x10025170 then 0x100251b3, 0x100251be then 0x1002521e,
// 0x10025630 then 0x10025669.
//
// Eleven bytes leaves three unmasked, under the byte matcher's floor, so all three
// score "too small to match" and were compared against the shipped bytes by
// hand instead.
CObjLight::~CObjLight()
{
}

CObjCamera::~CObjCamera()
{
}

CObjContainer::~CObjContainer()
{
}

// 0x10024b86 — the world model adds nothing to the base's parameter copy.
// Thirteen bytes of re-push and call, which is what a qualified call to the
// base from an override with the same signature compiles to.
void CObjWorldModel::_v20(CAttachMgrView* pAttachMgr, ObjectCreateStruct* pStruct)
{
	CObjBase::_v20(pAttachMgr, pStruct);
}

// 0x10025674 — the container takes its code off the create struct, and it is
// the world model's forward it chains through rather than the base's
// directly, which is the only reason that forward is there.
void CObjContainer::_v20(CAttachMgrView* pAttachMgr, ObjectCreateStruct* pStruct)
{
	CObjWorldModel::_v20(pAttachMgr, pStruct);

	m_ContainerCode = pStruct->m_ContainerCode;
}

// 0x10025610 — a line system's segment bank is the manager's, at +0x30 of it.
// The destructor at 0x100255c4 pushes every segment back onto exactly this
// bank's free list, so the two agree.
void CObjLineSystem::_v20(CAttachMgrView* pAttachMgr, ObjectCreateStruct* pStruct)
{
	CObjBase::_v20(pAttachMgr, pStruct);

	m_pUnknown1a8 = (uint8*)pAttachMgr + 0x30;
}


// The world model's forwarders. Every one reads 0x1a8, 0x1ac or the five
// links at 0x1b4, so they all belong to this layout; CObjContainer inherits
// it unchanged and some of these may be reached on a container.
//
// The object at 0x1b0 is ltobject_internal.h's IWorldBsp -- IsBusy at slot
// 0x00, MakeHPoly at 0x08 -- which is why the two state questions below are
// `IsBusy() == 1` and `IsBusy() == 0`. Its flags (bit 2 marks the main world)
// and the forward sit two slots higher than that header has them; see the
// view below.

// AvP2's world interface has two more slots than ltobject_internal.h's
// IWorldBsp (which is NOLF's): every image -- AvP2's and Primal Hunt's,
// lithtech.exe and server.dll alike -- calls the flags at slot 0x20 (0x0046c938
// `call *0x20(%eax)`, and the two flag tests at 0x0046c853 and 0x0046c87d)
// and tail-jumps the forward at 0x24 (0x0046c688). Where the two new slots
// sit below 0x18 is not known from here, so this view names only the two
// slots that moved, by their AvP2 offsets.
class IWorldBspFlagsView
{
public:
	virtual void   _v00() = 0;
	virtual void   _v04() = 0;
	virtual void   _v08() = 0;
	virtual void   _v0c() = 0;
	virtual void   _v10() = 0;
	virtual void   _v14() = 0;
	virtual void   _v18() = 0;
	virtual void   _v1c() = 0;
	virtual uint32 GetWorldFlags() = 0;      // 0x20
	virtual void   Forward() = 0;            // 0x24
};

// 0x10024c0d — hand the object to the world tree the data object owns. The
// node that comes back is discarded here; obj_LinkIntoWorldTree has already
// stored it on the object.
void CObjWorldModel::LinkIntoTree(LTObject* pObject)
{
	obj_LinkIntoWorldTree((CWorldBspView*)m_pWorldBsp, pObject);
}

// 0x10024bf1 — forwarded unchanged, which is why it is a tail jump.
void CObjWorldModel::ForwardToWorldBsp()
{
	((IWorldBspFlagsView*)m_pWorldBsp)->Forward();
}

// 0x10024bfc — one of the five links by index. The shipped `lea (%eax,%eax,2)`
// then `lea 0x1b4(%ecx,%eax,4)` is a times-twelve, which is what fixes the
// element as LTLink and not something else of the same total size.
LTLink* CObjWorldModel::GetLink(uint32 iLink)
{
	return &((LTLink*)m_Links1b4)[iLink];
}

// 0x10024e13 — is there a secondary object at all. `setne` against a zeroed
// register, so the result is a clean 0 or 1 rather than the pointer.
LTBOOL CObjWorldModel::HasOriginalBsp()
{
	return m_pOriginalBsp != LTNULL;
}

// 0x10024e1f — one bit of the data object's flags, returned as the masked
// value rather than as a boolean: the shipped `and $0x4,%eax` is the last
// thing before the return, with no normalisation after it.
uint32 CObjWorldModel::GetWorldFlag4()
{
	return ((IWorldBspFlagsView*)m_pWorldBsp)->GetWorldFlags() & WORLDFLAG_MAINWORLD;
}

// 0x10024e7f — the data object's state is exactly 1. Guarded, unlike its
// sibling below, and the `dec/neg/sbb/inc` is VC6's branchless equals-one.
LTBOOL CObjWorldModel::IsWorldBusy()
{
	// Positive test, so the LTFALSE lands after the call and not before it --
	// the same shared-exit hosting rule that FindTrackerLink and
	// WriteObjectID both turned on.
	if (m_pWorldBsp)
		return ((IWorldBsp*)m_pWorldBsp)->IsBusy() == 1;

	return LTFALSE;
}

// 0x10024e97 — the state is zero, and here the pointer is NOT checked: this
// one trusts the caller where the one above does not.
LTBOOL CObjWorldModel::IsWorldIdle()
{
	return ((IWorldBsp*)m_pWorldBsp)->IsBusy() == 0;
}

// 0x100255c4 — every segment goes back to the bank it came from, and the
// list is left empty.
//
// The list's terminator is the ADDRESS of m_Storage rather than a null, so
// the walk stops when the next pointer comes back around to it and both
// cursors are reset to it afterwards -- which is also what the constructor
// leaves them as. The bank pointer is re-read from the object on every
// iteration, not hoisted, so sb_Free has to be called through the field.
CObjLineSystem::~CObjLineSystem()
{
	CLineSegment* pSentinel = (CLineSegment*)m_Storage;
	CLineSegment* pCur = (CLineSegment*)m_pCursorB;

	while (pCur != pSentinel)
	{
		CLineSegment* pNext = pCur->m_pNext;

		sb_Free((CSegmentBankView*)m_pUnknown1a8, pCur);

		pCur = pNext;
	}

	m_pCursorA = m_Storage;
	m_pCursorB = m_Storage;
	m_pUnknown1a8 = LTNULL;
}


// 0x1002585f — om_CreateObject, named by its own error literal, and the one
// function that ties this file's two halves together: a manager, an
// ObjectCreateStruct and a slot to put the new object in.
//
// Four things it settles that nothing else in the file could:
//
//   * `CObjBase::_v20`'s first argument really is the object manager. The
//     call here pushes the CUnknown390* into the slot _v20 stores at
//     +0x5c, and objbank.cpp independently allocates Attachments out of
//     `pAttachMgr->m_Bank4c` — which is this class's third struct bank, at
//     0x4c, whose own free-list head lands on 0x64. So the "attachment
//     manager" objtypes.h models as a separate class with a list at +0x64
//     IS the manager, and the cast below is the file admitting it rather
//     than papering over it. Unifying the two is servermgr.h work.
//   * the object's per-type list link is at +0x7c, which was previously
//     twelve bytes of padding.
//   * the type check is unsigned and against NUM_OBJECTTYPES, done in
//     sixteen bits (`cmp $0xb,%ax`) before the widening the index needs.
//   * both failures report the code symbolically and carry a description,
//     which is de_error.h's RETURN_ERROR_SYM_DESC form, and the
//     description is the create struct's m_Filename — pStruct+0x7c, which
//     is where ltbasedefs.h puts it (0x3c + MAX_OCS_CLASSNAME_LEN).
LTRESULT om_CreateObject(CUnknown390* p, ObjectCreateStruct* pStruct,
                         CObjBase** ppObj)
{
	CObjBase* pObj;
	uint16    nType;

	*ppObj = 0;

	nType = pStruct->m_ObjectType;
	if (nType >= NUM_OBJECTTYPES)
		RETURN_ERROR_SYM_DESC(1, "om_CreateObject", LT_INVALIDPARAMS, pStruct->m_Filename)

	pObj = p->m_pBanks[nType]->Allocate();
	if (!pObj)
		RETURN_ERROR_SYM_DESC(1, "om_CreateObject", LT_OUTOFMEMORY, pStruct->m_Filename)

	pObj->_v20((CAttachMgrView*)p, pStruct);

	dl_AddHead(&p->m_Lists[nType], &pObj->m_Link7c, pObj);

	*ppObj = pObj;
	return LT_OK;
}


// *********************************************************************** //
//  THE WORLD'S POINT-TO-POLY WALK (0x10024cb1)
//
//  Neither worldsim.h nor worldbsp.h is included here -- both collide with
//  this file's own view classes -- so the two things the walk reaches are
//  redeclared as layout-identical views. The names and offsets are those
//  headers'; only what is used appears.
// *********************************************************************** //

// AVP2: the node's owner is ltobject_internal.h's WorldPolyOwner, and the
// plane comes through WorldPoly::GetPlane's two null tests -- no owner, or
// an owner without a surface (+0x2c), is no plane (server.dll
// 0x1002eb23..0x1002eb34; the same bytes in AvP2 1.0.9.6's 0x1002cd52).
// NOLF's walk went owner -> parent (+0x28) -> plane (+0x34) unchecked.
class CBspWalkNode
{
public:
	WorldPolyOwner* m_pOwner;                // 0x00
	CBspWalkNode*   m_pChild[2];             // 0x04  [0] behind, [1] in front
	uint8           m_pad_0c[0x14 - 0x0c];
	uint16          m_nPolyIndex;            // 0x14
	uint8           m_Flags;                 // 0x16

	LTPlane* GetPlane()
	{
		return (m_pOwner && m_pOwner->m_pSurface) ? m_pOwner->m_pPlane : LTNULL;
	}
};

// worldbsp.h's CWorldPoly, whose stride the walk's `lea (%eax,%eax,2)` plus
// `shl $0x4` gives in the clear: 0x30, where that header names only the
// index at +0x16.
class CBspWorldPoly
{
public:
	uint8  m_pad_00[0x16];
	uint16 m_nIndex;                         // 0x16
	uint8  m_pad_18[0x30 - 0x18];
};

// The two fields of worldbsp.h's CWorldBsp the walk reaches.
class CBspWorldView
{
public:
	uint8           m_pad_00[0x88];
	CBspWorldPoly*  m_pPolys;                // 0x88
	uint8           m_pad_8c[0x9c - 0x8c];
	CBspWalkNode*   m_pRoot;                 // 0x9c
};

#define BSPNODE_LEAF 0x03

// 0x10024cb1 — which world polygon a point belongs to. The walk descends the
// tree by the sign of the point's distance to each node's plane and keeps the
// last poly index it passed, so a leaf that carries none still answers with
// whatever the last branch above it named (name ours).
//
// Three things read off the bytes rather than guessed:
//
//   * the answer's 0xffff is stored as a dword and read back as a word,
//     which is the same byte-saving trick CBaseStream::ReadString's length
//     uses. Every read of it is sixteen-bit, so it is a uint16.
//   * and that is the whole residual. NEAR on Primal Hunt's server.dll
//     (0x1002eaec, 155 of 155 bytes, 62 of 62 instructions, once the plane
//     comes through the AvP2 owner's null tests above) with nine of them
//     naming a different slot: the shipped build parks the uint16 in the
//     dead pWorld PARAMETER slot (pWorld being cached in %ebx from the first
//     instruction) and reserves 0xc bytes of frame for the point alone,
//     where our cl 8966 gives the uint16 a slot of its own and reserves
//     0x10. Neither the declaration order nor the walk as an inline helper
//     (not inlined at /O1: 49 bytes and a call) moves it, and it is the
//     same disagreement w_IsectObjectBounds shows from the other side.
//     lithtech.exe has no copy of its own: netobj.cpp's inline twin is the
//     only one there (0x0046c722..).
//   * the side test is `>= -0.0001f`, not `>= 0`, so a point exactly on a
//     plane goes in FRONT.
//   * the point is copied into a local before the loop, and DistTo's own
//     by-value copy is then elided -- one `movsl` run, not two.
CBspWorldPoly* w_FindPolyAtPoint(CBspWorldView* pWorld, LTVector* pPoint)
{
	uint16        nIndex;
	LTVector      vPoint;
	CBspWalkNode* pNode;
	LTPlane*      pPlane;

	nIndex = 0xffff;
	pNode  = pWorld->m_pRoot;

	if (pNode->m_Flags & BSPNODE_LEAF)
		return LTNULL;

	vPoint = *pPoint;

	do
	{
		if (pNode->m_nPolyIndex != 0xffff)
			nIndex = pNode->m_nPolyIndex;

		pPlane = pNode->GetPlane();

		pNode = pNode->m_pChild[pPlane->DistTo(vPoint) >= -0.0001f ? 1 : 0];
	}
	while (!(pNode->m_Flags & BSPNODE_LEAF));

	if (nIndex == 0xffff)
		return LTNULL;

	return &pWorld->m_pPolys[nIndex];
}


// server.dll 0x1002eced / lithtech.exe 0x0046c9d0 -- the physics block under
// a point, asked of a world model: the point is taken into the model's own
// space through the second of its two transforms (+0x238) and handed BY
// VALUE to the world-model interface's GetPBlock (slot 0x10). No interface
// is no block. A thiscall on the object (server.dll's callers 0x100038cd,
// 0x10003b39 and 0x10004bc1 load it into ecx); server.dll calls MatVMul_H
// (0x10004864) and drops its result (`fstp %st(0)`), lithtech.exe's /O2
// expands it. objtypes.h has no slot for it, so it sits on a view (name
// ours).
class CObjWorldModelPBlockQ532 : public CObjWorldModel
{
public:
	class CPhysicsBlock* GetPBlockAt(LTVector* pPos);
};

CPhysicsBlock* CObjWorldModelPBlockQ532::GetPBlockAt(LTVector* pPos)
{
	LTVector vLocal;

	if (!m_pWMIface)
		return LTNULL;

	MatVMul_H(&vLocal, &m_Transform230, pPos);

	return ((IWorldBsp*)m_pWMIface)->GetPBlock(vLocal);
}
