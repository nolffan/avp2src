// server.dll — the sound cache's record: filling one in from its file and
// resetting it (reconstruction). Build /O1.
//
// Split from smgr.cpp: the two members are a translation unit of their own in
// both images, between CServerThread's and SoundInstance's (server.dll
// 0x10045483..0x100454fa, lithtech.exe 0x004997f0..0x00499890). The file name
// is ours.
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): this unit ends in <string>'s
// guarded initializer of std::ctype<unsigned short>::id (server.dll 0x100454fa: guard 0x1008ec84, atexit of the empty
// 0x1002eaeb; lithtech.exe 0x00499890: guard 0x004f8014, atexit of the empty
// 0x00401790).
#if defined(PRIMAL_HUNT)
#include <string>
#endif
#include "cacherec.h"

// The 0x38-byte description 0x100392a9 fills in from the opened source (it
// memsets exactly 0x38 bytes before parsing). Only two fields are read here,
// and the division of one by the other plus a tenth of a second says what
// they are: a byte total over a byte rate is a duration. Names ours.
struct CCacheSourceInfo
{
	uint8  m_pad_00[0x08];
	uint32 m_nBytesPerSec;                   // +0x08
	uint8  m_pad_0c[0x18 - 0x0c];
	uint32 m_nDataSize;                      // +0x18
	uint8  m_pad_1c[0x38 - 0x1c];
};

// 0x100392a9 — parse the opened source's header into that description;
// nonzero if it made sense of it (name ours).
int fs_GetSourceInfo(class ICacheSource* pSource, CCacheSourceInfo* pInfo);

// 0x1003567c — fill the record in from the file. The third argument is
// validated and never read; the shipped code checks it and nothing else.
//
// The failure return sits at the very end with the success return jumping
// over it, which is the AND-merged shape: one positive guard around the
// whole body rather than three guarded returns.
int CCacheRecView::Init(void* hFile, ICacheSource* pSource, void* pParam)
{
	CCacheSourceInfo info;

	if (hFile && pSource && pParam)
	{
		Reset();

		m_hFile = hFile;

		if (fs_GetSourceInfo(pSource, &info))
		{
			m_bWanted = 0;
			m_n14 = 0;
			m_f10 = (float)info.m_nDataSize / info.m_nBytesPerSec + 0.1f;

			return 1;
		}
	}

	return 0;
}

// 0x100356e9 — the two counters a fresh (or recycled) record starts from.
// Source order is only visible at /O2: /O1 rewrites the integer zero as
// `andl $0x0,0x14(%ecx)` and schedules that ahead of the `fldz/fstps`
// whatever order they are written in (0x100356e9), while /O2 folds both to
// one `xor %eax,%eax` and keeps them as spelled -- and lithtech.exe stores
// +0x10 first (0x0047d302).
void CCacheRecView::Reset()
{
	m_f10 = 0.0f;
	m_n14 = 0;
}
