// *********************************************************************** //
//
//  MODULE:   CLIENTSOUND (.H)
//
//  PURPOSE:  CSoundMgr — the client's ILTSoundMgr, and the sound instance
//            behind every HLTSOUND.
//
//  Every accessor here has the same three steps: refuse a null handle,
//  turn the handle into a live sound, and forward to that sound's vtable.
//  The refusals are what named the class: each one reports itself as
//  "CSoundMgr::Something".
//
// *********************************************************************** //

#ifndef __CLIENTSOUND_H__
#define __CLIENTSOUND_H__

#include "ltbasedefs.h"
#include "iltsoundmgr.h"

// One playing sound. Only the slots the manager forwards to are named; the
// ten before them are placeholders that keep the offsets right.
class CSoundInstance
{
public:
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0; virtual void _p1c()=0; virtual void _p20()=0;
	virtual void _p24()=0;
	virtual void SetObstruction(LTFLOAT fLevel)=0;                  // 0x28
	virtual void GetObstruction(LTFLOAT* pLevel)=0;                 // 0x2c
	virtual void SetOcclusion(LTFLOAT fLevel)=0;                    // 0x30
	virtual void GetOcclusion(LTFLOAT* pLevel)=0;                   // 0x34
	virtual void SetPosition(LTVector* pPos, int nUnknown)=0;       // 0x38
	virtual void GetPosition(LTVector* pPos)=0;                     // 0x3c
	virtual void _p40()=0;
	virtual LTBOOL SetFilter(const char* pFilter)=0;                // 0x44
	virtual LTBOOL SetFilterParam(const char* pParam, float fValue)=0;  // 0x48
	virtual LTBOOL GetFilterParam(const char* pParam, float* pValue)=0; // 0x4c

public:
	uint8  m_pad_04[0x08 - 0x04];
	class CSoundData* m_pData;               // 0x008
	uint8  m_pad_0c[0x44 - 0x0c];
	// Null until a filter has been set on the sound; reading a filter
	// parameter off a sound without one is refused.
	void*  m_pFilter;                        // 0x044
};

// The sample behind a sound. Only the length is reached from the manager.
class CSoundData
{
public:
	uint8  m_pad_00[0x54];
	// Milliseconds: GetSoundDuration scales it by a thousandth, and converts
	// it as unsigned - the zero high dword beside it in the fildll pair.
	uint32 m_nDurationMS;                    // 0x054
};

// What AIL_enumerate_filter_sample_attributes fills in. Miles calls this a
// RIB interface entry; the size is what the shipped frame reserves for it
// (twenty-four bytes of locals, less the two enumeration cursors) and the
// only field this file touches is the name.
struct RIBEntry
{
	uint32 m_nType;      // 0x00
	char*  m_pName;      // 0x04
	uint32 m_nToken;     // 0x08
	uint32 m_nSubType;   // 0x0c
};

class CSoundMgr
{
public:
	LTRESULT GetFilterName(uint32 nIndex, const char** pFilter);    // 0x00482ee0
	LTRESULT GetFilterIndex(const char* pFilter, uint32* pIndex);   // 0x00482fd0
	LTRESULT GetFilterParamName(uint32 nIndex, const char* pFilter,
	                            const char** pParam);               // 0x004830d0
	LTRESULT GetFilterParamIndex(const char* pFilter, const char* pParam,
	                             uint32* pIndex);                   // 0x00483260
	LTRESULT SetSoundFilter(HLTSOUND hSound, const char* pFilter);  // 0x00482b30
	LTRESULT SetSoundFilterParam(HLTSOUND hSound, const char* pParam,
	                             float fValue);                     // 0x00482c30
	LTRESULT GetSoundFilterParam(HLTSOUND hSound, const char* pParam,
	                             float* pValue);                    // 0x00482d80
	LTRESULT SetReverbProperties(ReverbProperties* pReverbProperties); // 0x00482010
	LTRESULT GetReverbProperties(ReverbProperties* pReverbProperties); // 0x00482230
	LTRESULT InitSound(InitSoundInfo* pSoundInfo);                  // 0x00482660
	LTRESULT GetSoundDuration(HLTSOUND hSound, LTFLOAT& fDuration); // 0x004823f0
	LTRESULT SetSoundOcclusion(HLTSOUND hSound, LTFLOAT fLevel);    // 0x004826b0
	LTRESULT GetSoundOcclusion(HLTSOUND hSound, LTFLOAT* pLevel);   // 0x00482750
	LTRESULT SetSoundObstruction(HLTSOUND hSound, LTFLOAT fLevel);  // 0x004827f0
	LTRESULT GetSoundObstruction(HLTSOUND hSound, LTFLOAT* pLevel); // 0x00482890
	LTRESULT GetSoundPosition(HLTSOUND hSound, LTVector* pPos);     // 0x00482930
	LTRESULT SetSoundPosition(HLTSOUND hSound, LTVector* pPos);     // 0x004829d0

	// 0x00480ce0 (name ours) — the handle-to-instance lookup every accessor
	// starts from. The second argument is passed 1 by all of them but the
	// filter calls, which try 0 first.
	CSoundInstance* GetSoundInstance(HLTSOUND hSound, LTBOOL bUnknown);

	// 0x0047f9e0 — bring the sound system up from the SDK's own
	// description of it. A member, which is why InitSound forwards to it
	// without cleaning up the argument. Written in clientsoundmgr.cpp as
	// CSoundMgrView::Init (aliased onto it in link/aliases.rsp).
	LTRESULT Init(InitSoundInfo* pSoundInfo);

public:
	uint8   m_pad_00[0x124];
	// Null until the sound system is up; the duration and the reverb
	// properties both answer trivially while it is.
	void*   m_pSoundSys;                     // 0x124
	uint8   m_pad_128[0x13c - 0x128];
	void*   m_h3DProvider;                   // 0x13c  Miles' 3D provider
	uint8   m_pad_140[0x7e8 - 0x140];
	// Either of these being set is what makes reverb available; both reverb
	// entries refuse when neither is.
	uint32  m_bReverbA;                      // 0x7e8
	uint32  m_bReverbB;                      // 0x7ec
	// The engine's own copy of the reverb state, in ReverbProperties' field
	// order but not its layout.
	uint32  m_dwAcoustics;                   // 0x7f0
	LTFLOAT m_fReflectTime;                  // 0x7f4
	LTFLOAT m_fVolume;                       // 0x7f8
	LTFLOAT m_fDecayTime;                    // 0x7fc
	LTFLOAT m_fDamping;                      // 0x800
};

// The bits ReverbProperties::m_dwParams uses to say which of its fields are
// meaningful. The SDK does not name them; these follow the field order, and
// the ranges the setter clamps to are the ones the header's comments give.
#define REVERBPARAM_VOLUME      0x01
#define REVERBPARAM_ACOUSTICS   0x02
#define REVERBPARAM_REFLECTTIME 0x04
#define REVERBPARAM_DECAYTIME   0x08
#define REVERBPARAM_DAMPING     0x10

// Miles, reached through the import table.
extern "C" {
__declspec(dllimport) void __stdcall AIL_lock(void);
__declspec(dllimport) void __stdcall AIL_unlock(void);
__declspec(dllimport) void __stdcall AIL_set_3D_room_type(void* provider, int nRoomType);
__declspec(dllimport) int  __stdcall AIL_enumerate_filters(uint32* pNext, void** pDest,
                                                           char** pName);
// Miles hands a filter's parameters back one RIB entry at a time, and the
// entry is a struct rather than a name: sixteen bytes of which only the name
// at offset four is ever read here. The caller keeps its own cursor.
__declspec(dllimport) int  __stdcall AIL_enumerate_filter_sample_attributes(
	void* hFilter, uint32* pNext, RIBEntry* pEntry);
}

#endif  // __CLIENTSOUND_H__
