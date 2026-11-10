// server.dll — the sound cache's record (reconstruction).
#ifndef __CACHEREC_H__
#define __CACHEREC_H__

#include "servermgr.h"

// The cache record with the one member servermgr.h does not model: the
// reset at 0x100356e9 is a thiscall (the shipped sweep loads the record into
// ecx and pushes nothing), and it clears a count at +0x14 and a float at
// +0x10. Named by what it does; nothing in the image names the record.
class CCacheRecView
{
public:
	void Reset();                            // 0x100356e9
	// 0x1003567c — fill the record in from the file, the opened source and
	// the handle's second dword; nonzero if it took (name ours).
	int  Init(void* hFile, class ICacheSource* pSource, void* pParam);

	LTLink m_Link;                           // +0x00
	void*  m_hFile;                          // +0x0c
	float  m_f10;                            // +0x10
	uint32 m_n14;                            // +0x14
	uint32 m_bWanted;                        // +0x18
};

#endif
