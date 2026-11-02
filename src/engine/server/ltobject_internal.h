// *********************************************************************** //
//
//  MODULE:   LTOBJECT_INTERNAL (.H)
//
//  PURPOSE:  The engine-internal layout of LTObject. ltbasetypes.h only
//            forward-declares the class and typedefs HOBJECT to a pointer to
//            it, because game code is never allowed to see inside; the layout
//            below is recovered from the field offsets the engine's own
//            accessors use.
//
//  Every offset here is asserted by a function that byte-matches the shipped
//  code, and is named after the accessor that reaches it. Where nothing in
//  the reconstructed set touches a range, it is padding — not a claim that
//  the range is unused.
//
//  Field                      offset  established by
//  -------------------------  ------  ----------------------------------------
//  <vtable>                   0x000   AddTracker / RemoveTracker (call *0x40)
//  m_Flags / m_Flags2         0x088   SharedCommonLT::Get/SetObjectFlags
//  m_pAttachments             0x098   SharedCommonLT::GetAttachments
//  m_ObjectType               0x0bc   IsWorldObject, GetStandingOn (cmp OT_)
//  m_Velocity                 0x0c4   GetVelocity
//  m_Acceleration             0x0d0   GetAcceleration
//  m_FrictionCoefficient      0x0dc   Get/SetFrictionCoefficient
//  m_Mass                     0x0e0   Get/SetObjectMass
//  m_ForceIgnoreLimitSqr      0x0e4   Get/SetForceIgnoreLimit (stored squared)
//  m_pStandingOn              0x0e8   GetStandingOn
//  m_pStandingOnPoly          0x0ec   GetStandingOn
//  m_ChangeFlags              0x120   SPhysicsLT::SetVelocity/SetAcceleration
//  m_Pos                      0x18c   GetStandingOn (uses .y at 0x190)
//  m_Dims                     0x198   GetObjectDims, GetStandingOn (.y = 0x19c)
//  m_pWorldBsp                0x1a8   IsWorldObject
//
// *********************************************************************** //

#ifndef __LTOBJECT_INTERNAL_H__
#define __LTOBJECT_INTERNAL_H__

#include "ltbasedefs.h"

// The plane of a world polygon is two indirections away from the poly. The
// binary gives the offsets but not the type names, so the intermediate
// classes below are named for their role in that walk, not from any source.
class WorldBspData
{
public:
	uint8    m_pad_000[0x34];
	LTPlane* m_pPlane;                       // 0x34
	uint32   m_nFlags;                       // 0x38
};

// Bit 22 marks geometry the physics will not let an object step up onto,
// however low the step is (name ours).
#define WBSP_NOSTEP 0x400000

// One vertex of a world polygon. The position is shared between every
// polygon that touches it, which is why it is a pointer rather than a value;
// the rest of the record is lighting and texture data no one here reads.
struct WorldPolyVertex
{
	LTVector* m_pPos;                        // 0x00
	uint8     m_pad_04[0x18 - 0x04];
};

class WorldPolyOwner
{
public:
	uint8         m_pad_000[0x18];
	// The polygon's bounding sphere, which is what the physics rejects on
	// before it looks at any edge.
	LTVector      m_vCentre;                 // 0x18
	float         m_fRadius;                 // 0x24
	WorldBspData* m_pParent;                 // 0x28
	uint8         m_pad_02c[0x50 - 0x2c];
	uint16        m_nVertices;               // 0x50
	uint8         m_pad_052[0x54 - 0x52];
	WorldPolyVertex m_Vertices[1];           // 0x54  as many as m_nVertices

	LTPlane* GetPlane() { return m_pParent->m_pPlane; }
};

class WorldSurface
{
public:
	uint8  m_pad_00[0x3c];
	uint16 m_nTextureFlags;                  // 0x3c
};

// The polygon table a physics block indexes: fixed twenty-four byte records,
// of which the physics reads only the owner at the front. The record itself
// is what the engine passes around as the polygon's identity (name ours).
struct WorldPolyRef
{
	WorldPolyOwner* m_pOwner;                // 0x00
	uint8           m_pad_04[0x17 - 0x04];
	// Which of the seven box projections applies to this polygon's plane:
	// one per axis direction for a plane that is axis-aligned, and the
	// general one for everything else (name ours).
	uint8           m_nProject;              // 0x17
};

class WorldPoly
{
public:
	WorldPolyOwner* m_pOwner;                // 0x00
	uint8           m_pad_04[0x28 - 0x04];
	WorldSurface*   m_pSurface;              // 0x28

	LTPlane* GetPlane() { return m_pOwner->m_pParent->m_pPlane; }
};


// The world geometry an object belongs to. Only one virtual is reached from
// the reconstructed set: slot 0x18, which returns the world's flags. Bit 2
// (0x04) marks the main world as opposed to a world model's own BSP.
class IWorldBsp
{
public:
	// Slot zero answers whether the world is busy with something that must
	// not be disturbed; a world model whose BSP says so is left where it is.
	virtual uint32 IsBusy()=0;               // 0x00
	virtual class BSPNode* GetRoot()=0;      // 0x04
	virtual HPOLY  MakeHPoly(WorldPoly* pPoly)=0;  // 0x08
	// 0x0c — MakeHPoly's inverse: the poly a handle names, within this
	// world. CServerWorld::FindPoly (0x1002dad1) reaches it with the whole
	// handle, high half included; worldbsp.h has CWorldBsp's own copy of the
	// slot (0x1000ab62) forwarding it to the surface table.
	virtual WorldPoly* FindPoly(HPOLY hPoly)=0;

	// 0x10 — the physics block covering a point. Null means the world was
	// built without them, which the physics complains about by name.
	virtual class CPhysicsBlock* GetPBlock(LTVector vPos)=0;

	// 0x14 — the polygon table the physics blocks index into.
	virtual struct WorldPolyRef* GetPolyRefs()=0;
	virtual uint32 GetWorldFlags()=0;        // 0x18

	// 0x1c — CObjWorldModel::ForwardToWorldBsp (0x10024bf1) tail-jumps to it
	// with no arguments and returns whatever it returns, which is all that is
	// known about it.
	virtual void   _v1c()=0;                 // 0x1c

	// 0x1002fb47 — the squared length of the box diagonal. Its only caller is
	// the point-container gather, which drops a container whose diagonal is
	// shorter than the point's distance from it, so the two vectors bound the
	// world (names ours; nothing else here reaches them).
	float GetBoxDiagonalSqr();

	// The world's name, inline rather than a pointer -- the physics passes
	// &m_Name to a %s.
	char     m_Name[0x0c4 - 0x004];          // 0x004
	LTVector m_vBoxMin;                      // 0x0c4
	LTVector m_vBoxMax;                      // 0x0d0
};

#define WORLDFLAG_MAINWORLD 0x04

// LTObject::m_ChangeFlags. Both velocity and acceleration set the same bit, so
// it marks "this object's motion changed" rather than either one.
#define CHANGEFLAG_MOTION 0x40


class LTAnimTracker;

// 0x1000d826 — put an object into the world tree. Declared here rather than
// locally in each caller because it is a DIRECT call: the argument type has to
// mangle the same way on both sides or the two TUs name different symbols.
// ltobject.cpp defines the view; every other caller only needs the pointer.
class CWorldBspView;
void* obj_LinkIntoWorldTree(CWorldBspView* pWorld, class LTObject* pObject);

// The engine-side data hung off every object at +0x188. Only one field is
// reached from the reconstructed set: the identifier handle GetObjectName
// resolves through sb_GetString.
class SObjData
{
public:
	LTLink m_Links;                          // +0x00  the inter-object links this
	                                         //        object is an end of; both
	                                         //        container walks run it
	void*  m_hIdentifier;                    // +0x0c
	float  m_NextUpdate;                     // +0x10  SetNextUpdate stores it raw
	float  m_DeactivationTime;               // +0x14  SetDeactivationTime writes this
	                                         //        and the countdown below together
	float  m_UpdateTimer;                    // +0x18  seconds until the next update
	class Client* m_pOwningClient;           // +0x1c  set at world entry; nonnull makes
	                                         //        removal LT_TRIEDTOREMOVECLIENTOBJECT
	class BaseClass* m_pBaseClass;           // +0x20  the game object (m_hObject at
	                                         //        its +8 points back here)
	class CClassInfo* m_pClass;              // +0x24  the class manager's record
	void*  m_Unknown28;                      // +0x28  freed by removal when set
	uint32 m_Unknown2c;                      // +0x2c  nonzero keeps the object out
	                                         //        of the inactive sweep
	void*  m_pModelFile;                     // +0x30  file identifiers, not names:
	void*  m_pSkinFile;                      // +0x34  GetModelFilenames resolves both
	                                         //        through the file manager
	uint8  m_pad_38[0x3c - 0x38];
	LTLink m_StateLink;                      // +0x3c  in the 0x8dc list: actives are
	                                         //        walked forward, inactives from
	                                         //        the tail (GetNextObject pair)
	class LTObject* m_pNextChanged;          // +0x48  the changed-objects list at 0x910
	uint8  m_pad_4c[0x50 - 0x4c];
	uint16 m_ClientChangeFlags;              // +0x50  ORed into each client's table
	uint16 m_NetFlags;                       // +0x52  NETFLAG_* (ltcodes.h); SetNetFlags
	                                         //        strips ANIMUNGUARANTEED off non-models
};

// One attachment of a child object to a parent. Only the list link is reached
// from here; the objects themselves come back from GetAttachmentObjects.
class Attachment
{
public:
	LTransform  m_Offset;                    // 0x00  position + rotation offset
	uint16      m_nParentID;                 // 0x1c
	uint16      m_nChildID;                  // 0x1e
	int         m_iNode;                     // 0x20  socket/node index; -1 = none
	Attachment* m_pNext;                     // 0x24
};

class LTObject
{
public:
	// LTObject is polymorphic: offset 0 is the vptr, which is why every field
	// below is four bytes further into the padding than its raw offset. Only
	// one slot is reached from the reconstructed set — 0x40, which looks up an
	// animation tracker on the object and hands back the address of the
	// pointer that links it, so the caller can unlink it.
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0; virtual void _p1c()=0; virtual void _p20()=0;
	virtual void _p24()=0;
	// 0x28 — the object's own 4x4, and 0x38 — seed a model walker from it
	// (names ours; both reached from the node/socket transform accessors).
	virtual void GetTransformMatrix(LTMatrix* pMat)=0;
	virtual void _p2c()=0;
	// 0x30 — answers whether the object may be moved at all; wm_MoveObject
	// (0x1001eb2d) gives up when it says no. LTBOOL, not void: the caller
	// tests eax (name not recoverable).
	virtual LTBOOL _p30()=0;
	virtual void _p34()=0;
	virtual void SetupModelWalker(class ModelWalker* pWalker)=0;
	virtual void _p3c()=0;
	virtual LTBOOL FindTracker(LTAnimTracker* pTracker, LTAnimTracker*** pppLink)=0;  // 0x40

	uint8      m_pad_004[0x054 - 0x004];
	int        m_Unknown54;                  // 0x054  nonzero excludes the object
	                                         //        from SFX broadcasts
	int        m_Unknown58;                  // 0x058  -1 gates SetDeactivationTime's
	                                         //        container bump
	uint8      m_pad_05c[0x06c - 0x05c];
	LTLink     m_Link6c;                     // 0x06c  in whatever list the flag
	                                         //        below says it is in
	// 0x078 — not a flag but the world-tree NODE the object is in, which is
	// why it reads as "nonzero while linked": 0x1000d826 stores the node
	// 0x1000d437 handed it here, and obj_ReleaseMemory only tests it.
	// objtypes.h models the same field on CObjBase and still has it as a
	// uint32, which is harmless there because nothing in that file writes it
	// with anything but zero.
	void*      m_pTreeNode78;                // 0x078
	uint8      m_pad_07c[0x088 - 0x07c];
	uint32     m_Flags;                      // 0x088  FLAG_*
	uint32     m_Flags2;                     // 0x08c  the second flag word
	uint32     m_UserFlags;                  // 0x090  the game's user flag word
	uint8      m_ColorR;                     // 0x094  object color, one byte per
	uint8      m_ColorG;                     // 0x095  channel
	uint8      m_ColorB;                     // 0x096
	uint8      m_ColorA;                     // 0x097
	class Attachment* m_pAttachments;        // 0x098  singly linked
	LTRotation m_Rotation;                   // 0x09c  quaternion
	LTVector   m_Scale;                      // 0x0ac  Get/SetObjectScale
	                                         //        (change flag 0x10)
	uint16     m_ObjectID;                   // 0x0b8  indexes the per-client tables
	// 0x0ba — a SECOND object id, and 0x1003506e is what says so: it writes
	// this one when the message's Verify() answers 1 and m_ObjectID when it
	// does not, so the two are alternative identifiers for the same object
	// chosen by which side the message is bound for (name ours).
	uint16     m_ObjectID2;                  // 0x0ba
	char       m_ObjectType;                 // 0x0bc  OT_* (signed: GetObjectType movsx-es it)
	uint8      m_BlockingPriority;           // 0x0bd  Get/SetBlockingPriority;
	                                         //        also copied into the world-model
	                                         //        move state
	uint8      m_pad_0be[0x0c4 - 0x0be];
	LTVector   m_Velocity;                   // 0x0c4
	LTVector   m_Acceleration;               // 0x0d0
	float      m_FrictionCoefficient;        // 0x0dc
	float      m_Mass;                       // 0x0e0
	float      m_ForceIgnoreLimitSqr;        // 0x0e4
	LTObject*  m_pStandingOn;                // 0x0e8
	WorldPoly* m_pStandingOnPoly;            // 0x0ec
	LTVector   m_BBoxMin;                    // 0x0f0  world-space bounding box
	LTVector   m_BBoxMax;                    // 0x0fc
	LTLink     m_StandingOnMe;               // 0x108  head of the list of objects
	                                         //        standing on this one
	LTLink     m_StandingLink;               // 0x114  this object's place in the
	                                         //        list of whatever it stands on
	uint32     m_ChangeFlags;                // 0x120  what the client must be told
	uint8      m_pad_124[0x188 - 0x124];
	class SObjData* m_pSData;                // 0x188  engine-side object data
	LTVector   m_Pos;                        // 0x18c
	LTVector   m_Dims;                       // 0x198
	float      m_Radius;                     // 0x1a4  bounding-sphere radius
	// A light has no world pointer and keeps its radius in the same slot,
	// which is why Get/SetLightRadius reach 0x1a8 on an OT_LIGHT.
	union {
		IWorldBsp* m_pWorldBsp;              // 0x1a8
		float      m_LightRadius;            // 0x1a8  (OT_LIGHT only)
	};
	IWorldBsp* m_pOriginalBsp;               // 0x1ac
	class IWorldModelIface* m_pWMIface;      // 0x1b0  see IWorldModelIface

	// 0x10024e51 — the object's own world first, then the one it lives in.
	HPOLY MakeHPoly(WorldPoly* pPoly);

	// 0x10024a33 — move the object (position only; callers that also rotate
	// write m_Rotation themselves and retransform after).
	void SetPos(LTVector vPos);

	// 0x10024a4c — rebuild the bounding box and sphere from the position and
	// the dimensions. Renamed from `Relocate`: the shipped code touches
	// nothing outside the object (name ours).
	void UpdateBounds();
};

// A poly grid keeps a list of records past the end of LTObject, and each
// record is in two lists. pg_FreeData establishes the pair of links and
// takes both records out; 0x1000d4a9, which is what puts them in, names
// which list is which and adds the two fields past them (names ours).
class PGRecord
{
public:
	LTLink    m_Link;                        // 0x00  in the world leaf's list
	LTLink    m_Link0c;                      // 0x0c  in the grid's own list
	LTObject* m_pObject;                     // 0x18  the grid this record is for
	void*     m_pLeaf;                       // 0x1c  the leaf it was put in, null
	                                         //       when the index is out of range
};

class PolyGridObject : public LTObject
{
public:
	uint8  m_pad_1b4[0x1e4 - 0x1b4];
	LTLink m_Records;                        // 0x1e4
};

// The world-model interface at LTObject+0x1b0 (names ours): only slot 1 is
// reached, handing back the transformed BSP that
// FindWorldModelObjectIntersections tests candidate boxes against.
class IWorldModelIface
{
public:
	virtual void  _v00() = 0;
	virtual void* GetTransformedBsp() = 0;   // +0x04
};

// names ours: re-derive the world-model transform after SetPos + a rotation
// write (0x1001e89e), and the box-vs-BSP overlap test (0x10002fff).
void obj_RetransformWorldModel(LTObject* pObj);                     // 0x1001e89e

// A world model carries, past the end of LTObject, the pair of 4x4s the
// retransform rebuilds. Their offsets come off obj_UpdateTransform, which
// hands the builder at 0x10011986 both of them; which one is the inverse is
// not established, because that builder is not reconstructed (names ours).
class WorldModelObject : public LTObject
{
public:
	uint8    m_pad_1b4[0x1f0 - 0x1b4];
	LTMatrix m_Transform1f0;                 // 0x1f0
	LTMatrix m_Transform230;                 // 0x230
};

// 0x1000d8e1 and 0x10007ae3 (names ours): refresh the object's own transform,
// then push it into the world model's geometry.
void obj_UpdateTransform(LTObject* pObj);

// 0x10011986 — the pair of 4x4s a world model keeps, built from the world's
// own offset, the object's position and the object's rotation. Not
// reconstructed; declared for the shape of the call (name ours).
void wm_BuildTransforms(LTVector* pOffset, LTVector* pPos, LTRotation* pRot,
                        LTMatrix* pMat1, LTMatrix* pMat2);

// 0x1000d8a8 — let go of the engine-side memory an object holds: it comes
// out of whatever list it is in, and a poly grid's own data goes too
// (names ours).
void obj_ReleaseMemory(LTObject* pObj);
void pg_FreeData(LTObject* pObj);           // 0x1000d7c7
void wm_Transform(LTObject* pObj, LTMatrix* pMat, int bMoveGeometry);
int  bsp_IntersectBox(void* pBsp, LTVector* pMin, LTVector* pMax);  // 0x10002fff

// A container is a world model carrying the code set at its creation;
// GetContainerCode reads it at +0x270 (names ours).
class ContainerInstance : public LTObject
{
public:
	uint8  m_pad_1b4[0x270 - 0x1b4];
	uint16 m_ContainerCode;                  // 0x270
};

#endif  // __LTOBJECT_INTERNAL_H__
