// *********************************************************************** //
//
//  MODULE:   CLIENTPHYSICS (.H)
//
//  PURPOSE:  CPhysicsLT — the client's half of ILTPhysics, the mirror of
//            SPhysicsLT in src/engine/server/commonphysics.cpp.
//
//  The two halves share CommonPhysicsLT and split on what a client is
//  allowed to do. Where the server drops a velocity change smaller than
//  0.001 squared rather than send it to every client, the client just
//  writes it: there is nobody downstream to tell. Where the server names
//  its check sites "ILTPhysics::SetVelocity", the client names them
//  "CPhysicsLT::SetVelocity" — and reports them at debug level 2 rather
//  than 1.
//
// *********************************************************************** //

#ifndef __CLIENTPHYSICS_H__
#define __CLIENTPHYSICS_H__

#include "commonphysics.h"
#include "clientmgr.h"

// The client's half of ILTPhysics. The field at 0x0c is the client manager
// where the server's is the world simulation; both are the thing the
// interface has to reach to do any real work.
class CPhysicsLT : public CommonPhysicsLT
{
public:
	LTRESULT SetVelocity(HOBJECT hObj, LTVector* pVel);           // 0x00404d10
	LTRESULT SetAcceleration(HOBJECT hObj, LTVector* pAccel);     // 0x00404d90
	LTRESULT MoveObject(HOBJECT hObj, LTVector* pPos, uint32 flags);  // 0x00404e10
	LTRESULT SetObjectDims(HOBJECT hObj, LTVector* pNewDims, uint32 flags);  // 0x00405000
	LTRESULT MovePushObjects(HOBJECT hToMove, LTVector& newPos,
	                         HOBJECT* hPushObjects, uint32 nPushObjects);    // 0x00405230
	LTRESULT RotatePushObjects(HOBJECT hToMove, LTRotation& newRot,
	                           HOBJECT* hPushObjects, uint32 nPushObjects);  // 0x00405350

public:
	float       m_StairHeight;               // 0x08 (m_ClientServerType fills 0x04)
	CClientMgr* m_pClientMgr;                // 0x0c
};

// 0x00451000 — commonphysics.cpp's wm_MoveObject (commonphysics.h): move an
// object through the client simulation. The position goes by value, three
// dwords on the stack.
void wm_MoveObject(struct WMMoveState* pState, LTVector vPos, uint32 nFlags);

// 0x004b4070 (name ours) — bumped by every MoveObject, before any check.
extern int g_nMoveObjectCount;

#endif  // __CLIENTPHYSICS_H__
