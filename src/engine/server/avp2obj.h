// avp2obj.h -- an object's layout in AvP2's engines, for the views of it.
//
// AVP2: the engine's objects (LTObject, ltobject_internal.h; CObjBase,
// objtypes.h; the client's) have two insertions over NOLF's, the same in
// 1.0.9.6's engine and Primal Hunt's, server and client alike: a byte at
// NOLF's 0xdc (InitFields sets it to five) and a float at NOLF's 0x160. The
// views of objects elsewhere keep NOLF's offsets in their comments and spell
// their bounds through AVP2_OBJ, which maps a NOLF offset of an object's
// base part to AvP2's. (A derived class's own members may move more:
// CObjModel's do, objbank.cpp.)
#ifndef __AVP2OBJ_H__
#define __AVP2OBJ_H__

#define AVP2_OBJ(o) ((o) + ((o) >= 0xdc ? 4 : 0) + ((o) >= 0x160 ? 4 : 0))

#endif
