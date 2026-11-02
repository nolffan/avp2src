// *********************************************************************** //
//
//  MODULE:   OBJTYPES (.H)
//
//  PURPOSE:  The eleven object types the engine allocates, as a hierarchy.
//            Each has a bank (objbank.cpp), a vtable, and a constructor that
//            hands its OT_ value to the base; the sizes come from the banks
//            and the OT_ values from the constructors themselves, so neither
//            is a guess. The class names are ours — ltbasedefs.h names the
//            types but not the classes.
//
//            The base is fourteen virtuals; a light and a canvas add a fifteenth.
//            Its own fields here are the ones the virtuals and the two
//            InitFields touch: this is a view of LTObject sized to what the
//            banks allocate (0x1a8), not the fuller model in
//            ltobject_internal.h, which describes the same object from the
//            interface side and includes fields that belong to the
//            subclasses. Where the two overlap they agree, offset for
//            offset — CObjBase::InitFields writes twenty of the fields
//            ltobject_internal.h names, and named none of them differently.
//
// *********************************************************************** //
#ifndef __OBJTYPES_H__
#define __OBJTYPES_H__

#include "ltbasedefs.h"

// What every object type is built on top of. It carries the vtable slot and
// nothing else — its constructor runs before any type's own, which is what
// puts the base's vptr store before the derived one in every constructor
// here (names ours).
class CObjRoot
{
public:
	CObjRoot(int n);                         // 0x10039707

	virtual ~CObjRoot();
};

class CObjBase : public CObjRoot
{
public:
	CObjBase();                              // 0x1002465b — type OT_NORMAL
	CObjBase(char nType);                    // 0x100246ac

	virtual ~CObjBase();                     // 0x100246d3
	virtual LTBOOL _v04(class CObjVisitor* pVisitor);   // 0x10024afc
	virtual void   _v08();                   // 0x10024a17
	virtual void   GetDims(LTVector* pMin, LTVector* pMax);  // 0x100249f6
	// Slots 4..7 are the world-tree object's own inline defaults, which no
	// object type overrides (worldtree.cpp's CWTObj_D: WantsExtraLink,
	// MakeExtraLink, NotifyInserted, OnFoundInBox). Every CObjBase-derived
	// vtable holds the same four: 0x1000989c (`xor %eax,%eax; ret`),
	// 0x10009897 (`xor %eax,%eax; ret $4`) and 0x1000bd91 (`ret $4`) twice
	// -- the last one shared with the NoCache GenSetCacheSize, which is the
	// linker folding identical COMDATs. The argument counts are the returns'.
	virtual LTBOOL _v10()             { return LTFALSE; }
	virtual void*  _v14(uint32 nIndex) { return LTNULL; }
	virtual void   _v18(void* pObj)   {}
	virtual void   _v1c(void* pInfo)  {}
	// 0x100247a1 -- slot 8 of the fourteen. Every field of an
	// ObjectCreateStruct the object keeps, copied in one go, plus the
	// attachment manager: the offsets it reads are ltbasedefs.h's
	// ObjectCreateStruct offset for offset (0x08 m_Flags, 0x0c m_Flags2,
	// 0x10 m_Pos, 0x1c m_Scale, 0x28 m_Rotation, and the low byte of the
	// uint16 m_ObjectType at 0x00 into this class's char one), which is a
	// free check that both layouts are right. No object type overrides it
	// -- it stands in all seven of the vtables that carry the base.
	virtual void   _v20(class CAttachMgrView* pAttachMgr,
	                    struct ObjectCreateStruct* pStruct);
	virtual LTBOOL IsInWorld();              // 0x1002467d
	virtual void   GetTransform(LTMatrix* pMat);         // 0x100249d2
	// 0x10024689 -- the bounding-sphere radius.
	virtual float  _v2c();
	// 0x10024cad (`push $1; pop %eax; ret`), a yes. The same address is
	// smgr.cpp's vq_Accept: identical COMDATs, folded.
	virtual LTBOOL _v30();
	// 0x1000989c again (`xor %eax,%eax; ret`), the same folded body as
	// slot 4's.
	virtual LTBOOL _v34()             { return LTFALSE; }

	// The pieces the base reaches for, all members rather than free
	// functions — the shipped calls set `this` and push nothing extra
	// (names ours).
	void BaseTerm();                         // 0x10039735
	void Unlink();                           // 0x10039740
	void InitFields();                       // 0x10024802

public:
	uint8    m_pad_04[0x58 - 0x04];
	// Cleared for every object of every type the first time the manager's
	// counter comes off -1 (0x1002461c), and touched nowhere else in this
	// file. A per-object cache stamp of some kind; name ours.
	uint32   m_Unknown58;                    // 0x058
	// Not the attachment list — that is at 0x098. Whatever this points at
	// keeps the spare-attachment free list at its own +0x64, which is the
	// only thing the destructor asks of it (servermgr.h models that side of
	// it as CAttachMgrView). Name ours.
	class CAttachMgrView* m_pAttachMgr;      // 0x05c
	LTLink   m_Link60;                       // 0x060
	LTLink   m_Link6c;                       // 0x06c  m_pData is the object
	uint32   m_bLinked6c;                    // 0x078  nonzero while linked
	// The link the object hangs by in its manager's per-type list.
	// om_CreateObject (0x1002585f) dl_AddHeads it into
	// CUnknown390::m_Lists[type] with the object as its data.
	LTLink   m_Link7c;                       // 0x07c
	uint32   m_Flags;                        // 0x088
	uint32   m_Flags2;                       // 0x08c
	uint32   m_UserFlags;                    // 0x090
	// InitFields stores 255 into the four of them in the order B, G, R, A,
	// which is the order they are written here (the offsets are the ones
	// ltobject_internal.h established from the colour accessors).
	uint8    m_ColorR;                       // 0x094
	uint8    m_ColorG;                       // 0x095
	uint8    m_ColorB;                       // 0x096
	uint8    m_ColorA;                       // 0x097
	class Attachment* m_pAttachments;        // 0x098  singly linked
	LTRotation m_Rotation;                   // 0x09c
	LTVector m_Scale;                        // 0x0ac
	uint16   m_ObjectID;                     // 0x0b8
	uint8    m_pad_ba[0xbc - 0xba];
	char     m_ObjectType;                   // 0x0bc  OT_*
	uint8    m_BlockingPriority;             // 0x0bd
	uint8    m_pad_be[0xc0 - 0xbe];
	uint32   m_Unknownc0;                    // 0x0c0
	LTVector m_Velocity;                     // 0x0c4
	LTVector m_Acceleration;                 // 0x0d0
	float    m_FrictionCoefficient;          // 0x0dc  starts at five
	float    m_Mass;                         // 0x0e0  and at thirty
	float    m_ForceIgnoreLimitSqr;          // 0x0e4  a million, i.e. 1000 units
	void*    m_pStandingOn;                  // 0x0e8
	void*    m_pStandingOnPoly;              // 0x0ec
	LTVector m_vBoxMin;                      // 0x0f0
	LTVector m_vBoxMax;                      // 0x0fc
	LTLink   m_StandingOnMe;                 // 0x108
	// The link the object hangs by in whatever it is standing on. InitFields
	// gives it its m_pData and does NOT tie it off, which is the one link of
	// the five that is left that way.
	LTLink   m_StandingLink;                 // 0x114  m_pData is the object
	uint32   m_ChangeFlags;                  // 0x120
	// Two lists the object is in only while the value beside each is
	// non-zero; the destructor tests the value rather than the link, which
	// is what says the value is what puts it there (names ours).
	float    m_fLink150Key;                  // 0x124
	float    m_f128;                         // 0x128
	LTVector m_v12c;                         // 0x12c
	LTVector m_v138;                         // 0x138
	uint8    m_pad_144[0x150 - 0x144];
	LTLink   m_Link150;                      // 0x150  m_pData is the object
	float    m_fLink170Key;                  // 0x15c
	uint8    m_pad_160[0x170 - 0x160];
	LTLink   m_Link170;                      // 0x170  m_pData is the object
	void*    m_pUnknown17c;                  // 0x17c
	uint16   m_n180;                         // 0x180
	uint8    m_pad_182[0x188 - 0x182];
	void*    m_pSData;                       // 0x188  null until it is in a world
	LTVector m_Pos;                          // 0x18c
	LTVector m_Dims;                         // 0x198  starts at one on each axis
	float    m_Radius;                       // 0x1a4
};

// 0x10024772 — hand an object's attachments back to the manager's spare list
// (name ours).
void obj_DetachAll(class CAttachMgrView* pMgr, CObjBase* pObj);

// Whatever is walking the objects. Only one thing is asked of it, and only
// by objects that carry the flag (name ours).
class CObjVisitor
{
public:
	void Visit(CObjBase* pObj);              // 0x1003a509
};

// 0x100118d5 — build a 4x4 from a position, a rotation and a scale, any of
// which may be null (name ours; servermath.h declares the same function for
// the three ILTMath matrix builders that are all this one call).
void mat_SetupTransform(LTVector* pPos, LTRotation* pRot, LTVector* pScale,
                        LTMatrix* pMat);

// 0x10015bcd — a rotation as a 4x4. servermath.h has a quat_ToMatrix at
// 0x100117e2; this is a second, different one, and it is the one the
// transform builder calls (name ours).
void quat_ToMatrix2(LTRotation* pRot, LTMatrix* pMat);


class CObjWorldModel : public CObjBase
{
public:
	CObjWorldModel();                        // 0x10024b19
	CObjWorldModel(char nType);              // 0x10024b50
	virtual ~CObjWorldModel();               // 0x10024b6f

	// 0x10024d41 — what the destructor releases before the base runs. Called
	// on `this` at offset zero and nothing in the image names it, so it keeps
	// its address.
	void sub_10024d41();

	void InitFields();                       // 0x10024b96

	// Nine more of its own, every one of them reached through a vftable slot.
	// They are declared NON-virtual deliberately: the vftable COMDAT at
	// 0x1004df70 holds two concatenated tables (the slot pattern repeats from
	// +0x38, with GetDims, IsInWorld and GetTransform each appearing twice at
	// the same relative offsets), so the class has two vptrs and which table
	// each of these sits in is not yet settled. The bodies are unaffected --
	// they are thiscall against the same layout either way -- so this only
	// leaves the emitted vftable wrong, not the code.
	// 0x10024b86 — slot 0x20, and a pure forward: it adds nothing of its own
	// and exists only so CObjContainer's override can chain through it.
	virtual void _v20(class CAttachMgrView* pAttachMgr,
	                  struct ObjectCreateStruct* pStruct);

	void   LinkIntoTree(class LTObject* pObject);   // 0x10024c0d
	void   ForwardToWorldBsp();              // 0x10024bf1
	LTLink* GetLink(uint32 iLink);           // 0x10024bfc
	LTBOOL HasOriginalBsp();                 // 0x10024e13
	uint32 GetWorldFlag4();                  // 0x10024e1f
	LTBOOL IsWorldBusy();                    // 0x10024e7f
	LTBOOL IsWorldIdle();                    // 0x10024e97

	// The same three fields ltobject_internal.h names on LTObject, and this
	// is the same object seen from the other side: the world the model lives
	// in, its own BSP, and the world-model interface. Typed as void* here
	// because objtypes.h comes in ahead of the object header.
	void*    m_pWorldBsp;                    // 0x1a8  an IWorldBsp*
	void*    m_pOriginalBsp;                 // 0x1ac  an IWorldBsp*
	void*    m_pWMIface;                     // 0x1b0
	// Five LTLinks, each given the object as its data by InitFields. The
	// count is the shipped loop's own (push $5 / pop ecx) and five twelves
	// is exactly the gap to the first matrix. Declared as raw bytes rather
	// than `LTLink m_Links1b4[5]`, and cast at the one use: an array member
	// of a class with a constructor makes VC6 emit a vector-constructor
	// iterator (`??_H`) COMDAT that the shipped image has no counterpart
	// for. Every function still matches either way — this only keeps an
	// unmatchable 34-byte helper out of the object.
	uint8    m_Links1b4[0x1f0 - 0x1b4];      // 0x1b4
	// The transform pair the world-model retransform rebuilds; which of the
	// two is the inverse is not established here, because the builder at
	// 0x10011986 is not reconstructed.
	LTMatrix m_Transform1f0;                 // 0x1f0
	LTMatrix m_Transform230;                 // 0x230
};

// A container is a world model carrying the code set at its creation; an
// unset code is all-ones rather than zero, because zero is a real container.
class CObjContainer : public CObjWorldModel
{
public:
	virtual ~CObjContainer();                        // 0x10025669

	// 0x10025674 — slot 0x20: chain through the world model's, then take the
	// container code off the create struct. This is the confirmation that
	// 0x270 is the container code, and the reason CObjWorldModel's own
	// forward exists at all.
	virtual void _v20(class CAttachMgrView* pAttachMgr,
	                  struct ObjectCreateStruct* pStruct);
	CObjContainer();                         // 0x10025630

	uint16 m_ContainerCode;                  // 0x270
	uint8  m_pad_272[0x274 - 0x272];
};

// A canvas draws itself, so it carries a callback and the distance it wants
// to be drawn at; the distance starts at one rather than nothing.
class CObjCanvas : public CObjBase
{
public:
	CObjCanvas();                            // 0x10025696

	// server.dll 0x100256c0 / lithtech.exe 0x0045bb00, `flds 0x1b0(%ecx);
	// ret`: the draw distance. Both images' canvas vtables (0x1004e1e8,
	// 0x004a2e80) are FIFTEEN slots and hold this one address at +0x2c
	// (the base's _v2c, overridden) AND at +0x38, a virtual only the canvas
	// adds. Two identically-bodied virtuals folded by /OPT:ICF -- the same
	// folding that puts the base's _v10 and _v34 at one address -- so the
	// fifteenth is written as its own member (name ours).
	virtual float _v2c();
	virtual float _v38();

	void*  m_pUnknown1a8;                    // 0x1a8
	void*  m_pUnknown1ac;                    // 0x1ac
	float  m_fUnknown1b0;                    // 0x1b0
};

// A camera opens at ninety degrees both ways.
class CObjCamera : public CObjBase
{
public:
	virtual ~CObjCamera();                        // 0x1002521e
	CObjCamera();                            // 0x100251be

	uint8    m_pad_1a8[0x1b8 - 0x1a8];
	float    m_fFovY;                        // 0x1b8
	float    m_fFovX;                        // 0x1bc
	uint32   m_Unknown1c0;                   // 0x1c0
	LTVector m_v1c4;                         // 0x1c4
};

// A line system keeps two cursors into its own storage, both starting at the
// front of it.
// One segment of a line system, as the destructor reads it: a next-in-list
// at +0x40, and at +0x00 the bank's intrusive free link that sb_Free writes
// once the segment is released. Name ours; nothing between 0x04 and 0x40 is
// reached from here.
struct CLineSegment
{
	void*                m_pNextFree;        // +0x00  sb_Free's link
	uint8                m_pad_04[0x40 - 0x04];
	struct CLineSegment* m_pNext;            // +0x40
};

class CObjLineSystem : public CObjBase
{
public:
	CObjLineSystem();                        // 0x10025530

	// 0x100255c4, with the deleting form at 0x100255a8.
	virtual ~CObjLineSystem();

	// 0x10025610 — slot 0x20: the segment bank comes from the manager, at
	// its +0x30. That is where the destructor's segments go back to.
	virtual void _v20(class CAttachMgrView* pAttachMgr,
	                  struct ObjectCreateStruct* pStruct);

	void*    m_pUnknown1a8;                  // 0x1a8
	void*    m_pUnknown1ac;                  // 0x1ac
	uint8    m_Storage[0x1ec - 0x1b0];       // 0x1b0
	void*    m_pCursorA;                     // 0x1ec
	void*    m_pCursorB;                     // 0x1f0
	LTVector m_v1f4;                         // 0x1f4
	LTVector m_v200;                         // 0x200
	LTVector m_v20c;                         // 0x20c
	float    m_f218;                         // 0x218
};

// A light's radius sits where every other type keeps something else, which
// is why the accessor for it is the one virtual the base does not have.
class CObjLight : public CObjBase
{
public:
	virtual ~CObjLight();                        // 0x100251b3
	CObjLight();                             // 0x10025170

	virtual float GetLightRadius(void* pUnused);   // 0x1002518e

	float m_fLightRadius;                    // 0x1a8
};

#endif  // __OBJTYPES_H__
