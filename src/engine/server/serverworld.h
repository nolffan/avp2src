// *********************************************************************** //
//
//  MODULE:   SERVERWORLD (.H)
//
//  PURPOSE:  The server world's five light-animation arrays (CServerMgr
//            +0x1a8.., CClientMgr +0x04..): their element types and the
//            CMoArray instantiations over them.  Moved out of
//            commonmodel.cpp, unchanged, so that clientmgr3.cpp's view of
//            the world can hold the SAME types: the shipped image has one
//            CMoArray instance per element type, and the out-of-line
//            destructors Primal Hunt's unwind funclets call (0x004cee0b..)
//            are those instances'.  Nothing here emits code by being
//            included.
//
// *********************************************************************** //

#ifndef __SERVERWORLD_H__
#define __SERVERWORLD_H__

#include "collection.h"

// The nine sub-objects the constructor builds, in the order it builds them,
// which is member declaration order. Each is named for its offset and sized
// by where the next one starts — nothing proves an extent, only a start. The
// constructor addresses are the evidence and are recorded beside each one
// (names ours).
//
// The first five are stdlith's own CMoArray<T, DefaultCache> (collection.h),
// and lithtech.exe 0x004222f0 is what says so. /O2 spends its inline budget
// on this constructor and the shipped bytes show every CMoArray piece where
// VC6 left it: each array's vptr, then Clear()'s four zeroes or a call to the
// folded out-of-line Clear (0x004283a0), then Init(0, 0) as a call -- except
// at +0x18, where Init itself is expanded down to Term's SetSize2
// (0x00414fa0), _InitArray (0x00433570) and SetSize's SetSize2. Declared-only
// stand-in classes could not produce any of that (101 bytes against 260);
// the real template does, identical on the first build, with no source
// steering. server.dll's /O1 still calls the five constructors (0x1000b7e6
// ...) and its teardown (0x1002c9e3) still expands `~CMoArray() { Term(); }`
// as a vptr restore and one SetSize2 call per array.
//
// The element types come off the resizes. +0x04's BaseNew (0x00413190)
// strides 0x60 bytes and runs the constructor at 0x00422240 over each slot
// (collection.cpp's CCollElem60C has that body): worldmodels.cpp's CLARecord,
// the light-animation record. +0x18 is four bytes (the CLALight pair of
// words), +0x2c four (a pointer: CLAFrameEntry*), +0x40 one byte (no scale
// in its allocation), +0x54 eight (lea 0(,%eax,8): CLAFrameEntry). The
// classes below carry only size and constructor profile (names ours; the
// real ones live in worldmodels.cpp, another TU).
class CWorldAnimRecQ
{
public:
	CWorldAnimRecQ();                        // 0x00422240
	uint8 m_pad[0x60];
};

class CWorldAnimLightQ
{
public:
	uint16 m_n00;
	uint16 m_n02;
};

// AVP2: twenty-four bytes, not NOLF's eight: the +0x54 array's Init strides
// `lea (%eax,%eax,2); shl $3` (Primal Hunt lithtech.exe 0x00432ddb, its
// server.dll's BaseNew 0x10010d5e, AvP2 1.0.9.6's server.dll 0x100103ea).
// What the extra sixteen bytes hold is not read anywhere here.
class CWorldAnimFrameQ
{
public:
	void*  m_pData;
	uint32 m_nSize;
	uint8  m_pad_08[0x18 - 0x08];
};

typedef CMoArray<CWorldAnimRecQ>     CWorldSub04;   // ctor 0x1000b7e6, SetSize2 0x1000c267
typedef CMoArray<CWorldAnimLightQ>   CWorldSub18;   // ctor 0x1000ba1c, SetSize2 0x1000c470
typedef CMoArray<CWorldAnimFrameQ*>  CWorldSub2C;   // ctor 0x1000ba6d, SetSize2 0x1000c470
typedef CMoArray<uint8>              CWorldSub40;   // ctor 0x1000af9a, SetSize2 0x1000be59
typedef CMoArray<CWorldAnimFrameQ>   CWorldSub54;   // ctor 0x1000bb45, SetSize2 0x1002ccee

#endif  // __SERVERWORLD_H__
