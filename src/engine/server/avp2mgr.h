// avp2mgr.h -- CServerMgr's layout in AvP2's engines, for the views of it.
//
// AVP2: CServerMgr (servermgr.h) has three insertions over NOLF's, the same in
// 1.0.9.6's server.dll and Primal Hunt's: four bytes before the object lists
// (NOLF's 0x5b0), 0x408 bytes after m_hWorldFile (NOLF's 0x668), and four
// bytes after m_NetMgr (NOLF's 0x7e4). The views of it elsewhere (classes
// that lay out only the members they reach) keep NOLF's offsets in their
// comments and spell their bounds through AVP2_MGR, which maps a NOLF offset
// to AvP2's.
#ifndef __AVP2MGR_H__
#define __AVP2MGR_H__

#define AVP2_MGR(o) ((o) + ((o) >= 0x5b0 ? 4 : 0) + ((o) >= 0x668 ? 0x408 : 0) + ((o) >= 0x7e4 ? 4 : 0))

#endif
