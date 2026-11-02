// *********************************************************************** //
//
//  MODULE:   CLIENTSOUNDINSTANCE (.CPP)
//
//  PURPOSE:  CSoundInstance — the base class behind every HLTSOUND, and
//            CSoundData, the loaded sample it plays from. clientsound.h
//            already has the manager (CSoundMgr) and the ten CSoundInstance
//            vtable slots the manager forwards to; this file is the class
//            itself.
//
//  WHY THIS FILE AND NOT AN OBJECT-UPDATE READER: the address run this was
//  cut from (0x00479160-0x0047f9b0) is not one module. It is three, and the
//  boundaries are proved by strings and by cross-image byte identity:
//
//    0x00479160-0x0047bc4x  the CLIENT's packet handlers. 0x00479160 is the
//                           change-flag trace ("ObjectID: %d, type %d,
//                           flags %d" plus the eight CF_ names), and the
//                           handlers past it report themselves by name:
//                           "OnSkyDef", "OnPortalFlagsPacket",
//                           "OnNetProtocolVersionPacket",
//                           "OnThreadLoadPacket". None of those strings is
//                           in server.dll. This belongs in clientmgr.cpp.
//
//    0x0047bc50-0x0047c62x  the SERVER engine, compiled into lithtech.exe as
//                           well: "CPacket::ReadType",
//                           "SetObjectChangeFlags" + "LT_OBJECTNOTINWORLD"
//                           (sm_SetObjectChangeFlag, src/engine/server/
//                           servermodel.cpp) and "Server: position larger
//                           than 'DebugMaxPos'". Four of the small functions
//                           in this block are BYTE-IDENTICAL to code in
//                           the retail server.dll, so their source is already
//                           reconstructed under src/engine/server/ and
//                           nothing here touches them.
//
//    0x0047c63x-0x0047f9b0  the client's sound engine — sound-file loading
//                           ("Missing sound file %s", "Corrupt sound file
//                           %s", "Loaded sound file %s"), EAX reverb, and
//                           the mss32.dll imports. That is this file.
//
// *********************************************************************** //

#include "clientsound.h"


// ---- the two classes, as layout-identical views -------------------------- //

// clientsound.h declares CSoundInstance with the ten slots CSoundMgr
// forwards to and three fields; the class has twenty-three slots and runs to
// at least 0xb8 bytes. That header is not this reconstruction's to change, so
// the whole of it is redeclared here on a view (house style: see the
// IObjInsideTestView comment in src/engine/server/servercommon.cpp). The
// vptr is at offset 0, which is where every `mov (%esi),%eax` below reads it.
//
// WHERE THE LAYOUT COMES FROM. Four vtables share these bodies, so there are
// four classes in the hierarchy and the base is the one whose vtable the base
// constructor installs:
//
//   0x004a3448  the base            ctor 0x0047d310, m_Type left 0
//   0x004a34a4  a derived class     ctor 0x0047e970, m_Type = 1
//   0x004a3508  a derived class     ctor 0x0047eda0, m_Type = 2 — the one
//                                   that owns a Miles 3D sample
//   0x004a3698  a derived class     ctor 0x004834c7 / 0x0048363a
//
// Every field named below is written by the constructor at 0x0047d310, by
// Term() at 0x0047d5c0, or by one of the bodies in this file. Slot +0x3c is
// what pins the vptr-relative numbering: clientsound.h already calls +0x3c
// GetPosition, and 0x0047d390 (the body in that slot in all four vtables)
// copies three floats out of this+0x68 — so m_vPos is at 0x68 and the slot
// map in clientsound.h is right.
//
// Slots +0x10 and +0x14 are __purecall (0x004928e7) in the base's own vtable,
// so the base is abstract on those two and the numbering cannot slip.
class CSoundInstanceView
{
public:
	// +0x00 is one shared scalar-deleting destructor, 0x004836d0, in ALL
	// FOUR vtables: the derived classes add no destructor of their own, so
	// their deleting thunks came out identical and the linker folded them.
	CSoundInstanceView();                                  // 0x0047d310
	virtual ~CSoundInstanceView();                         // +0x00 0x0047d3e0
	// +0x04 — the base body is 0x0047d3f0 (soundinst2.cpp's name, Init);
	// each derived class overrides it and calls it first.
	virtual LTRESULT Init(class CSoundDataView* pData, PlaySoundInfo* pInfo,
	                      uint32 nStartMS);                // +0x04 0x0047d3f0
	virtual void   Term();                                 // +0x08 0x0047d5c0
	virtual LTRESULT _v0c();                               // +0x0c 0x0047db20
	virtual void   _v10()=0;                               // +0x10 pure
	virtual void   _v14()=0;                               // +0x14 pure
	virtual LTRESULT Stop(int nUnknown);                   // +0x18 (name ours)
	virtual LTRESULT _v1c(int nUnknown);                   // +0x1c 0x0047d6e0
	virtual LTRESULT Hold();                               // +0x20 (name ours)
	virtual LTRESULT ReleaseHold();                        // +0x24 (name ours)
	virtual LTRESULT SetObstruction(LTFLOAT fLevel)=0;     // +0x28
	virtual LTRESULT GetObstruction(LTFLOAT* pLevel)=0;    // +0x2c
	virtual LTRESULT SetOcclusion(LTFLOAT fLevel)=0;       // +0x30
	virtual LTRESULT GetOcclusion(LTFLOAT* pLevel)=0;      // +0x34
	virtual LTRESULT SetPosition(LTVector* pPos, int nUnknown);     // +0x38
	virtual LTRESULT GetPosition(LTVector* pPos);          // +0x3c
	virtual LTRESULT GetListenerOffset(LTVector* pOffset)=0;  // +0x40 (soundinst2.cpp)
	// The three filter slots are the base's own: the same three bodies sit
	// at +0x44/+0x48/+0x4c of all four vtables. They answer LTRESULTs
	// (LT_INVALIDPARAMS, LT_NOTFOUND, LT_OK), not LTBOOLs.
	virtual LTRESULT SetFilter(const char* pFilter);       // +0x44 0x0047e3c0
	virtual LTRESULT SetFilterParam(const char* pParam,
	                                float fValue);         // +0x48 0x0047e480
	virtual LTRESULT GetFilterParam(const char* pParam,
	                                float* pValue);        // +0x4c 0x0047e540
	// +0x50 has one shared body in all four vtables and it is the whole of
	// what says +0x7c means anything: `return m_nUnknown7c != -1`. What -1
	// stands for is NOT established, so the name is by address.
	virtual LTBOOL sub_47d3b0();                           // +0x50 0x0047d3b0
	// +0x54 — one body in all four vtables: (re)start playback from the
	// current position (see its banner, before KillLoop).  (name ours)
	virtual LTRESULT Play();                               // +0x54 0x0047df30
	virtual void   _v58()=0;                               // +0x58 0x0047e610

	// Three more members, none of them virtual: 0x0047d690 is reached by a
	// direct call from Term (0x0047d5cf) and from the client's sweep at
	// 0x0047a1a3, and the other two by direct call only.
	LTRESULT sub_47d690();                                 // 0x0047d690
	LTRESULT sub_47e360();                                 // 0x0047e360
	LTBOOL   sub_47e390();                                 // 0x0047e390
	LTRESULT KillLoop();                                   // 0x0047e2c0 (cchandler.cpp's name)
	// inline stand-in; see sub_47e390
	inline LTBOOL AttachTo(class CSoundDataView* p);

public:
	// 0x04 — which class this is. The base constructor zeroes it and each
	// derived constructor overwrites it (1 and 2 below; the sound module's
	// own derived class is not read here).
	uint32   m_Type;                         // 0x04
	// 0x08 — the sample. Term() reaches its instance count at +0x88 and
	// unloads it through its vtable when the count falls to zero, which is
	// what identifies it as clientsound.h's CSoundData.
	class CSoundDataView* m_pData;           // 0x08
	void*    m_pUnknown0c;                   // 0x0c  see sub_47e390
	// 0x10 — a flag word, read as a dword and tested at bit 9 (0x200) by
	// ReleaseTableSlot and at bit 12 (0x1000) by Term.
	uint32   m_Flags10;                      // 0x10
	// 0x14 — an index into a table the client manager owns; 0xffff is
	// "none", which is what the constructor stores.
	uint32   m_nTableSlot;                   // 0x14
	uint8    m_Unknown18;                    // 0x18
	uint8    m_pad_19[0x1c - 0x19];
	uint32   m_Unknown1c;                    // 0x1c
	uint8    m_nUnknown20;                   // 0x20  constructed to 100
	uint8    m_pad_21[0x24 - 0x21];
	// 0x24 — stowed in m_Saved3c across a hold and restored when the last
	// hold goes; CSoundData::sub_47d160 picks the instance with the SMALLEST
	// value here, so it is what the sound system drops first (name ours).
	uint32   m_nPriority;                    // 0x24
	uint32   m_Unknown28;                    // 0x28
	uint32   m_Unknown2c;                    // 0x2c  see Init
	uint32   m_Unknown30;                    // 0x30
	float    m_f34;                          // 0x34  constructed to 1.0f
	uint32   m_nHoldCount;                   // 0x38  (name ours)
	uint32   m_Saved3c;                      // 0x3c  (name ours)
	// 0x40 — the state flags this file works in: bit 2 (0x04) is set by
	// Stop, bit 1 (0x02) is tested by Hold, bit 5 (0x20) marks a live hold.
	uint32   m_StateFlags;                   // 0x40
	// 0x44 — the Miles HSAMPLE: the filter slots hand it to
	// AIL_set_sample_processor and friends when it is set.
	void*    m_hSample;                      // 0x44
	// 0x48 — the Miles 3D sample handle on the m_Type == 2 class; zero on
	// the others. Term closes an AIL stream out of 0x4c and a Win32 handle
	// out of 0x50.
	void*    m_h3DSample;                    // 0x48
	void*    m_hStream4c;                    // 0x4c
	void*    m_hFile50;                      // 0x50
	uint16   m_nUnknown54;                   // 0x54
	uint8    m_pad_56[0x58 - 0x56];
	uint32   m_Unknown58;                    // 0x58
	// 0x5c — this instance's place in its CSoundData's list.
	LTLink   m_DataLink;                     // 0x5c
	// 0x68 — the sound's world position. Not initialised by the base
	// constructor; each derived constructor zeroes it.
	LTVector m_vPos;                         // 0x68
	uint32   m_Unknown74;                    // 0x74
	uint32   m_Unknown78;                    // 0x78
	// 0x7c — the index of the Miles filter set on this sound, -1 for none:
	// SetFilter (0x0047e3c0) stores the enumeration index it found here and
	// the two parameter slots hand it to the manager's GetFilterName.
	int      m_nUnknown7c;                   // 0x7c  constructed to -1
	// Two seven-element arrays, and the count is not inferred: the
	// constructor's tail loop runs seven times writing -1 at 0x80+4k and 0
	// at 0x9c+4k. Term repeats it exactly. They are the filter parameters
	// set on the sound, index and value (SetFilterParam, 0x0047e480, `fstps
	// 0x9c(%esi,%eax,4)`), -1 marking a free entry.
	int      m_Unknown80[7];                 // 0x80
	float    m_Unknown9c[7];                 // 0x9c
};


// The front of CSoundData's WaveInfo (lttexmod.cpp decodes all of it): the
// WAVEFORMATEX's first three fields, which is as far as this file reads.
struct WaveInfoSI
{
	uint16 wFormatTag;                       // 0x00
	uint16 nChannels;                        // 0x02
	// signed here: 0x0047dbf9 converts it with `fildl`, where a uint32
	// would have gone through a zero-extended `fildll`.
	int32  nSamplesPerSec;                   // 0x04
	uint32 nAvgBytesPerSec;                  // 0x08
	// 0x0c — WAVEFORMATEX's nBlockAlign; Play (0x0047e217) rounds the 3D
	// start offset down to it.
	uint16 nBlockAlign;                      // 0x0c
};


// The sample, from the side this file uses it. clientsound.h has the
// duration at 0x54; the instance list and its count are read off the
// constructor at 0x0047c630 (which ties a link at 0x8c off against itself
// and zeroes 0x88) and off Term at 0x0047d5c0, which unloads the sample
// through the vtable once m_nInstances hits zero. Names ours.
class CSoundDataView
{
public:
	CSoundDataView();                              // 0x0047c630
	virtual ~CSoundDataView();                     // +0x00 0x0047c6b0, and
	                                               //       0x0047c690 is the
	                                               //       deleting thunk
	virtual void _v04()=0;                         // +0x04 0x0047c6c0
	virtual void _v08()=0;                         // +0x08 0x0047c710
	virtual void Free()=0;                         // +0x0c 0x0047c7d0 (name ours)
	virtual void _v10()=0;                         // +0x10 0x0047cf60
	virtual LTBOOL sub_47d020();                   // +0x14 0x0047d020
	virtual LTBOOL Load()=0;                       // +0x18 0x0047c8c0 (name ours:
	                                               //       "Loaded sound file %s")

public:
	// 0x04 — nonzero is "there is something to load"; sub_47d020 refuses to
	// reload without it.
	void*  m_pUnknown04;                     // 0x04
	// 0x08 — m_WaveInfo (lttexmod.cpp) starts here with the WAVEFORMATEX.
	uint16 m_wFormatTag;                     // 0x08  m_WaveInfo.m_Format
	uint8  m_pad_0a[0x20 - 0x0a];
	// 0x20 is a BYTE count, not a sample count, and the proof is the duration
	// CSoundData::Load computes at 0x0047c96b: `fildll` of +0x20, `fmuls`
	// 1000.0f, `fidivl` of +0x10 -- and +0x10 is the WAVEFORMATEX's
	// nAvgBytesPerSec.  Bytes over bytes-per-second.  A sample count would
	// have to be divided by nSamplesPerSec at +0x0c.  The sample count is the
	// next field along.  (Found by the LTTexMod pass, which decoded the whole
	// WaveInfo struct at CSoundData+0x08 -- see src/engine/lithtech/lttexmod.cpp.)
	uint32 m_nDataSize;                      // 0x20  m_WaveInfo.m_nDataSize
	uint32 m_nSampleCount;                   // 0x24  m_WaveInfo.m_nSampleCount
	uint32 m_nLithFlags;                     // 0x28  m_WaveInfo.m_nLithFlags
	// 0x2c — the 'lith' chunk's second dword, read as a FLOAT by Init
	// (0x0047d43f `flds 0x2c(%eax)`): the half-width of the random pitch
	// spread (name ours).
	float  m_fPitchRange;                    // 0x2c
	// 0x30 — m_WaveInfo.m_bHasLoop (sndinst2.cpp's WaveInfoQ47C): Play
	// (0x0047df99) only sets a loop block from 0x58/0x5c when it is set.
	uint32 m_bHasLoop;                       // 0x30
	uint8  m_pad_34[0x40 - 0x34];
	// 0x40/0x48/0x4c/0x58..0x87 — zeroed by the constructor (0x0047c630);
	// nothing in this file reads them (names by offset).
	uint32 m_Unknown40;                      // 0x40
	void*  m_pSampleData;                    // 0x44  sndinst2.cpp
	uint32 m_Unknown48;                      // 0x48
	uint32 m_Unknown4c;                      // 0x4c
	// 0x50 — the decompressed shadow of a compressed sample (sndinst2.cpp,
	// 0x0047cec0 allocates it out of the manager's bank).
	CSoundDataView* m_pDecompressed;         // 0x50
	uint32 m_nDurationMS;                    // 0x54  clientsound.h
	uint32 m_Unknown58;                      // 0x58
	uint32 m_Unknown5c;                      // 0x5c
	uint32 m_Unknown60;                      // 0x60
	// 0x64..0x87 — one `rep stos` of nine dwords (0x0047c672..0x0047c680).
	uint8  m_Unknown64[0x88 - 0x64];
	// 0x88/0x8c — an LTList, count first: AddInstance at 0x0047d050 is
	// ltlink.h's dl_AddTail expanded (m_pData stored first, the head's
	// m_pPrev taken as the insertion point, the count bumped last) and
	// RemoveInstance is its dl_RemoveAt.
	LTList m_Instances;                      // 0x88

	// 0x0047d050 and 0x0047d0e0 (names ours) — put an instance on this
	// sample's list and take it off again. Both thiscall on the sample with
	// the instance as the argument; the remover unlinks the instance's
	// m_DataLink and decrements m_nInstances.
	LTRESULT AddInstance(CSoundInstanceView* pInstance);
	LTRESULT RemoveInstance(CSoundInstanceView* pInstance);

	// 0x0047cec0 — build the decompressed shadow (sndinst2.cpp has the
	// body). A direct thiscall from AddInstance.
	LTRESULT sub_47cec0();

	// The two written in this file. Neither is virtual: both are reached by
	// a direct call.
	CSoundInstanceView* sub_47d160();
	LTRESULT sub_47d240(uint8* pDst, uint16* pSrc, uint32 nCount);

	// The decompressed shadow's wave description / Miles AILSOUNDINFO if
	// there is one, otherwise this sample's own. POINTER-level: 0x0047db20
	// forms the address in both arms (`add $0x8` / `lea 0x8(%edi)`, `lea
	// 0x64(%ecx)` / `add $0x64`) before reading through it (names ours).
	WaveInfoSI* GetWaveInfo()
	{
		return m_pDecompressed ? (WaveInfoSI*)&m_pDecompressed->m_wFormatTag
		                       : (WaveInfoSI*)&m_wFormatTag;
	}
	void* GetSoundInfo()
	{
		return m_pDecompressed ? m_pDecompressed->m_Unknown64 : m_Unknown64;
	}
	// FIELD-level, like sndinst2.cpp's GetDataSizeQ47C: Play re-tests
	// m_pDecompressed at every use (0x0047dfad, 0x0047dff9, 0x0047e00c ...).
	uint32 GetDataSize()
	{
		return m_pDecompressed ? m_pDecompressed->m_nDataSize : m_nDataSize;
	}
};


// 0x004204f0 — give one entry of the client manager's table back: the id is
// masked to sixteen bits, bounds-checked against the count at +0x11e0, and
// the eight-byte record at +0x11dc is cleared in its first byte and its
// second dword. The second parameter is a uint16 and the shipped prologue
// says so — `mov 0x8(%esp),%eax; and $0xffff,%eax` is how VC6 reads a
// sixteen-bit stack parameter it then uses as an int (name ours).
void tbl_ReleaseSlot(void* pTable, uint16 nID);

// 0x004b17e0 — the table those slots live in. Uninitialised data, so there
// is nothing to read off it but the one use below (name ours).
extern void* g_pSlotTableOwner;

// The Miles entries this file reaches that clientsound.h does not already
// declare. Names and stack sizes are the import table's own
// (_AIL_set_3D_sample_obstruction@8 and friends).
extern "C" {
__declspec(dllimport) void  __stdcall AIL_set_3D_sample_obstruction(void* hSample,
                                                                    float fLevel);
__declspec(dllimport) void  __stdcall AIL_set_3D_sample_occlusion(void* hSample,
                                                                  float fLevel);
__declspec(dllimport) float __stdcall AIL_3D_sample_obstruction(void* hSample);
__declspec(dllimport) uint32 __stdcall AIL_set_preference(uint32 nPreference,
                                                          int nValue);
__declspec(dllimport) int   __stdcall AIL_set_sample_processor(void* hSample,
                                                               int nPipelineStage,
                                                               void* hProvider);
__declspec(dllimport) void  __stdcall AIL_set_filter_sample_preference(void* hSample,
                                                                       const char* pName,
                                                                       const void* pValue);
__declspec(dllimport) int   __stdcall AIL_filter_sample_attribute(void* hSample,
                                                                  const char* pName,
                                                                  void* pValue);
}

// Miles' DP_FILTER, the pipeline stage SetFilter installs the filter on.
#define SND_DP_FILTER 1

// 0x00492f30 — the CRT's _stricmp.
extern "C" int __cdecl _stricmp(const char* a, const char* b);


// 0x0040f7f0 — `g_pClientMgr + 0x774`, the client's sound manager
// (binkvideo.cpp, cchandler.cpp, sndinst2.cpp name the same body). Two sides
// of it are reached here: two slots of its ILTSoundMgr vtable, and the
// CSoundData object bank at 0x284 (clientsoundmgr.cpp's m_SoundDataBank,
// whose StructBank free-list head is at bank+0x1c).
class CSoundDataBankSI
{
public:
	uint8 m_pad_00[0x1c];
	void* m_FreeListHead;                    // 0x1c

	// object_bank.h's Free: the explicit destructor (`push $0; call
	// *(%eax)`, the deleting thunk with its flag clear) and sb_Free's two
	// stores.
	inline void Free(CSoundDataView* pObj);
};

class CSoundMgrSI
{
public:
	virtual void _v00(); virtual void _v04(); virtual void _v08();
	virtual void _v0c(); virtual void _v10(); virtual void _v14();
	virtual void _v18(); virtual void _v1c(); virtual void _v20();
	virtual void _v24(); virtual void _v28(); virtual void _v2c();
	virtual void _v30(); virtual void _v34(); virtual void _v38();
	virtual void _v3c(); virtual void _v40(); virtual void _v44();
	virtual void _v48();
	virtual LTRESULT GetFilterName(uint32 nIndex,
	                               const char** pFilter);          // +0x4c
	virtual void _v50(); virtual void _v54(); virtual void _v58();
	virtual void _v5c(); virtual void _v60(); virtual void _v64();
	virtual void _v68(); virtual void _v6c();
	virtual LTRESULT GetFilterParamIndex(const char* pFilter,
	                                     const char* pParam,
	                                     uint32* pIndex);          // +0x70

public:
	uint8            m_pad_04[0x284 - 0x04];
	CSoundDataBankSI m_SoundDataBank;        // 0x284
	uint8            m_pad_2a4[0x7ec - 0x2a4];
	uint32           m_bReverbB;             // 0x7ec  clientsound.h

	// clientsoundmgr.cpp's non-virtual voice members, by its names.
	LTRESULT SetSampleUserData(void* hSample, uint32 nValue);   // 0x00481910
	LTRESULT Set3DUserData(void* h3DSample, uint32 nValue);     // 0x00481980
	void*    GetFree3DVoice(CSoundInstanceView* pSound);        // 0x00481a80
	void     FreeSWVoice(void* hSample);                        // 0x00481bc0
	void     Free3DVoice(void* h3DSample);                      // 0x00481c10
};

CSoundMgrSI* ci_GetSoundMgrSI();             // 0x0040f7f0


// ---- CSoundData ---------------------------------------------------------- //

// 0x0047c630 — the sample's constructor, out of line (clientstreams.cpp and
// sndmgr2.cpp call it). The first tie-off of 0x8c and zero of 0x88 come
// BEFORE the vptr store (0x0047c642..0x0047c64f), which is LTList's own
// default constructor running as a member; the body then does it again
// (0x0047c667..0x0047c66c) — dl_InitList. 0x64..0x87 is one memset.
CSoundDataView::CSoundDataView()
{
	m_pUnknown04 = LTNULL;
	m_Unknown4c = 0;
	m_pDecompressed = LTNULL;
	m_pSampleData = LTNULL;
	m_Unknown48 = 0;
	m_Unknown60 = 0;
	dl_InitList(&m_Instances);
	m_Unknown40 = 0;
	m_Unknown5c = 0;
	m_Unknown58 = 0;
	memset(m_Unknown64, 0, sizeof(m_Unknown64));
}


// 0x0047c6b0 — the sample's destructor. Two instructions: put the class's own
// vptr back and tail-jump to Free(). It is reached from 0x0047c690, the
// scalar-deleting thunk in slot +0x00 of vtable 0x004a342c, which is the
// standard VC6 pair — the thunk tests bit 0 of its hidden flag and calls
// dfree(this).
//
// The jump rather than a call is the tail-call VC6 emits when the destructor
// body is one non-virtual call and nothing follows it. Free() is a virtual,
// called here non-virtually because a destructor's calls always are.
CSoundDataView::~CSoundDataView()
{
	Free();
}


// 0x0047d020 — vtable slot +0x14 of the sample: throw away what is loaded and
// load it again. Read off the bytes:
//
//   * _v10() is called through the vtable with no arguments (`mov (%esi),%eax;
//     call *0x10(%eax)` with ecx already this, and no stack adjustment
//     after), and it is called FIRST, before the guard.
//   * with nothing to load the answer is success, not failure: `mov $1,%eax`
//     and return.
//   * the last line is `!!Load()` — `neg; sbb; neg`, VC6's normalisation of a
//     pointer-or-int to 0/1, which is what an LTBOOL-returning function does
//     with a value it did not produce itself.
LTBOOL CSoundDataView::sub_47d020()
{
	_v10();

	if (!m_pUnknown04)
		return LTTRUE;

	return Load() != 0;
}


// 0x0047d050 — put an instance on this sample. Called from sub_47e390 below
// (0x0047e3ab), which normalises the answer to 0/1.
//
// Read off the bytes:
//
//   * a sample with nothing in memory (0x44) but a file to load from (0x04)
//     is reloaded through slot +0x14 first, and a nonzero answer there is a
//     refusal: the `jne` on 0x44 and the `je`s on 0x04 and the call's
//     result all reach the SAME next test, so it is one && chain.
//   * the second chain is the decompress: an IMA ADPCM or MP3 sample
//     (0x11/0x55, compared as a word) whose 'lith' flags have bit 0 clear
//     and bit 2 set, with no shadow yet, gets one from 0x0047cec0.
//   * the tail is dl_AddTail, expanded in its own order.
LTRESULT CSoundDataView::AddInstance(CSoundInstanceView* pInstance)
{
	if (!m_pSampleData && m_pUnknown04 && sub_47d020())
		return LT_ERROR;

	if ((m_wFormatTag == 0x11 || m_wFormatTag == 0x55)
	    && !(m_nLithFlags & 0x01)
	    && (m_nLithFlags & 0x04)
	    && !m_pDecompressed
	    && sub_47cec0())
		return LT_ERROR;

	dl_AddTail(&m_Instances, &pInstance->m_DataLink, pInstance);

	return LT_OK;
}


// 0x0047d0e0 — and take it off again. Called from sub_47e360 (0x0047e372).
// Once the last instance of a decompress-on-demand sample goes, its shadow
// goes back to the manager's bank.
LTRESULT CSoundDataView::RemoveInstance(CSoundInstanceView* pInstance)
{
	dl_RemoveAt(&m_Instances, &pInstance->m_DataLink);

	if (m_pDecompressed && (m_nLithFlags & 0x04) && !m_Instances.m_nElements)
	{
		// The bank is named as a local BEFORE the shadow is read: shipped
		// reloads m_pDecompressed after the call (0x0047d125) and forms the
		// bank pointer with its own `add $0x284` (0x0047d130). As one
		// expression the argument is evaluated first and held from the
		// guard's load across the call; through a manager local the add
		// folds into the displacement. Both measured, both near.
		CSoundDataBankSI* pBank = &ci_GetSoundMgrSI()->m_SoundDataBank;

		pBank->Free(m_pDecompressed);

		m_pDecompressed = LTNULL;
	}

	return LT_OK;
}


// 0x0047d160 — of the instances playing this sample, the one with the lowest
// m_nPriority. Called from 0x00481851, in the sound manager.
//
// Read off the bytes:
//
//   * the empty case is the COUNT, not the list: `mov 0x88(%ecx),%eax; test;
//     je` returns null without ever looking at the links, so the source tests
//     m_nInstances.
//   * the first element is taken as the answer before the loop (the shipped
//     code loads m_Instances.m_pNext, reads its m_pData, and only then steps
//     on), so this is a "seed with the first, compare the rest" walk and not
//     a loop with a null seed.
//   * the comparison is `jae`, unsigned, and strict: an equal priority does
//     not displace the incumbent.
//   * the list is walked to the sentinel's own address (`lea 0x8c(%ecx),%esi`
//     held in a register for both the entry test and the loop test), which is
//     ltlink.h's circular list.
CSoundInstanceView* CSoundDataView::sub_47d160()
{
	LTLink* pCur;
	CSoundInstanceView* pBest;

	if (m_Instances.m_nElements == 0)
		return LTNULL;

	pCur = m_Instances.m_Head.m_pNext;
	pBest = (CSoundInstanceView*)pCur->m_pData;
	pCur = pCur->m_pNext;

	while (pCur != &m_Instances.m_Head)
	{
		CSoundInstanceView* pCand = (CSoundInstanceView*)pCur->m_pData;

		pCur = pCur->m_pNext;

		if (pCand->m_nPriority < pBest->m_nPriority)
			pBest = pCand;
	}

	return pBest;
}


// 0x0047d240 — sixteen-bit signed samples down to eight-bit unsigned, in
// place of a mixer. Called from 0x0047ccb0, inside the loader at 0x0047c8c0,
// with `mov %ebp,%ecx` — so it is a member, and `this` is the sample the
// loader is filling. The body never reads it.
//
// The arithmetic is exactly `(s + 0x8000) >> 8`: add half a range to recentre
// a signed sample on 128 and keep the top eight bits.
//
// Two things the bytes say about how it is written:
//
//   * the loop end is precomputed into a register (`lea (%eax,%ecx,1),%esi`)
//     and tested before the first iteration, so this is a `while` over a
//     pointer against a limit and not a counted `for` — a counted loop would
//     keep the count.
//   * the sample is loaded NARROW, `mov (%ecx),%dx`, leaving the top half of
//     edx whatever it held, and the add and the shift that follow are
//     thirty-two bit. That is safe and not an accident: only bits 8..15 of
//     the sum are stored, and 0x8000 cannot carry into them from below. It is
//     the code you get from a uint16 load, which is why the cast is written.
LTRESULT CSoundDataView::sub_47d240(uint8* pDst, uint16* pSrc, uint32 nCount)
{
	uint8* pEnd = pDst + nCount;

	// The source pointer is copied into a local before the loop: that is
	// what makes the shipped build load it ahead of the first test, in %ecx
	// with the sample in %dx. Walking the parameter itself sinks the load
	// below the test, and a named uint16 sample local swaps the registers.
	// The 0x8000u literal is load-bearing: with a signed 0x8000 the shift
	// comes out `sar`, and the shipped build shifts `shr`.
	uint16* pIn = pSrc;

	while (pDst < pEnd)
		*pDst++ = (uint8)(((uint16)*pIn++ + 0x8000u) >> 8);

	return LT_OK;
}


inline void CSoundDataBankSI::Free(CSoundDataView* pObj)
{
	pObj->~CSoundDataView();

	*(void**)pObj = m_FreeListHead;
	m_FreeListHead = pObj;
}


// ---- CSoundInstance: the base's shared bodies ---------------------------- //
//
// Each of the six below fills the same slot in three or four of the four
// vtables, so each is one edit against several slots. The slot counts are
// off .rdata: 0x004a3448 (base), 0x004a34a4, 0x004a3508 and 0x004a3698.

// 0x0047d310 — the base constructor, out of line: CSoundInstance1's
// constructor (0x0047e970) calls it, as do 0x0047eda0 and the bank at
// 0x00483490. Store order is the shipped one; 0x0c and the position are left
// alone (each derived constructor zeroes m_vPos). The tail loop is the
// seven filter-parameter pairs, -1 index and zero value.
CSoundInstanceView::CSoundInstanceView()
{
	m_Type = 0;
	m_pData = LTNULL;
	m_Flags10 = 0;
	m_StateFlags = 0;
	m_Unknown18 = 0;
	m_nUnknown20 = 100;
	m_f34 = 1.0f;
	m_Unknown28 = 0;
	m_nPriority = 0;
	m_Unknown30 = 0;
	m_nHoldCount = 0;
	m_Saved3c = 0;
	m_nTableSlot = 0xffff;
	m_hSample = LTNULL;
	m_h3DSample = LTNULL;
	m_hStream4c = LTNULL;
	m_hFile50 = (void*)-1;
	m_nUnknown54 = 0;
	m_Unknown1c = 0;
	m_Unknown58 = 0;
	m_DataLink.TieOff();
	m_nUnknown7c = -1;

	for (uint32 i = 0; i < 7; i++)
	{
		m_Unknown80[i] = -1;
		m_Unknown9c[i] = 0.0f;
	}
}


// 0x0047d3e0 — the base destructor: store the base's own vptr and tail-jump to
// Term(). Reached from 0x004836d0, which is slot +0x00 in all four vtables —
// one folded deleting thunk for the whole hierarchy.
CSoundInstanceView::~CSoundInstanceView()
{
	Term();
}


// 0x0047d390 — hand back the sound's position. FOUR SLOTS: +0x3c of all four
// vtables.
//
// The three floats are copied field by field rather than as a vector
// assignment, and the shipped code reaches them through a pointer it forms
// once (`add $0x68,%ecx`) rather than at 0x68/0x6c/0x70 off this — which is
// what an LTVector copy through a member's address looks like.
//
// clientsound.h declares this slot `void`; it returns LTRESULT. `xor %eax,%eax`
// before `ret $4` is a returned zero and a void function would not have it.
LTRESULT CSoundInstanceView::GetPosition(LTVector* pPos)
{
	*pPos = m_vPos;

	return LT_OK;
}


// 0x0047d3c0 — and the other way. THREE SLOTS: +0x38 of 0x004a3448, 0x004a34a4
// and 0x004a3698; the m_Type == 2 class overrides it at 0x0047edf0.
//
// `ret $8`, so the second parameter is real even though nothing here reads it
// — clientsound.h already carries it.
LTRESULT CSoundInstanceView::SetPosition(LTVector* pPos, int nUnknown)
{
	m_vPos = *pPos;

	return LT_OK;
}


// 0x0047d3b0 — FOUR SLOTS: +0x50 of all four vtables. Twelve bytes, and all
// twelve are the test: `setne` on m_nUnknown7c against -1.
//
// This is the only reader of m_nUnknown7c in the run, and -1 is what the
// constructor and Term both put there, so "assigned or not" is as far as the
// bytes go. Named by address for that reason.
LTBOOL CSoundInstanceView::sub_47d3b0()
{
	return m_nUnknown7c != -1;
}


// 0x0047d6c0 — FOUR SLOTS: +0x18 of all four vtables. Give up the sound:
// remember that it stopped, drop its priority to nothing, and let the class
// below do the work.
//
// Read off the bytes:
//
//   * the flag is ORed with `or $0x4,%al` on a dword loaded whole and stored
//     back whole — VC6's byte form for a constant that fits in eight bits, not
//     evidence of a byte field.
//   * m_nPriority is zeroed with an immediate store BETWEEN the vptr load and
//     the virtual call, which is where a statement written before the call
//     lands once the call's operands are already in registers.
//   * the argument is forwarded straight through, and the return is a fresh
//     zero rather than the callee's answer.
LTRESULT CSoundInstanceView::Stop(int nUnknown)
{
	m_StateFlags |= 0x04;

	m_nPriority = 0;

	_v1c(nUnknown);

	return LT_OK;
}


// 0x0047d7f0 — FOUR SLOTS: +0x20 of all four vtables. Take a hold on the
// sound, the way servermessage.cpp's snd_AddHold does on the server's side:
// the first hold is the one that stows the priority and raises the flag, and
// every hold after it only counts.
//
// The count is loaded once, tested, and incremented on both paths out of the
// `if` — the shipped `inc %eax` sits after the join, which is what a single
// `++m_nHoldCount` below the `if` gives.
LTRESULT CSoundInstanceView::Hold()
{
	if (m_StateFlags & 0x02)
		_v1c(1);

	if (m_nHoldCount == 0)
	{
		m_StateFlags |= 0x20;

		m_Saved3c = m_nPriority;
	}

	++m_nHoldCount;

	return LT_OK;
}


// 0x0047d820 — FOUR SLOTS: +0x24 of all four vtables, and the exact inverse.
//
// Read off the bytes:
//
//   * the two refusals share one `mov $1,%eax; ret` placed after the body, so
//     they are two separate `return`s and not an `||` — the flag test jumps
//     there and so does the count test.
//   * the flag word is loaded ONCE into edx and the clear at the end is
//     `and $0xffffffdf,%edx` on that same register, which is why it is read
//     into a local here: reading the member again would reload it.
//   * the decrement is stored whether or not it reached zero (`dec; test;
//     mov; jne`), so the store is above the `if` and not inside it.
LTRESULT CSoundInstanceView::ReleaseHold()
{
	uint32 nFlags = m_StateFlags;

	// Both refusals reach ONE `mov $1,%eax` epilogue at 0x0047d84d, which is
	// the shared-exit lever: a single positive condition wraps the body and
	// hosts the failure answer at the END.  As two guarded returns the
	// function is size-exact and still not identical -- each arm gets its
	// own mov/ret.
	if ((nFlags & 0x20) && m_nHoldCount)
	{
		--m_nHoldCount;

		if (m_nHoldCount == 0)
		{
			m_StateFlags = nFlags & ~0x20;

			m_nPriority = m_Saved3c;

			m_Saved3c = 0;
		}

		return LT_OK;
	}

	return LT_ERROR;
}


// ---- CSoundInstance: the rest -------------------------------------------- //

// The Miles and Win32 entries Term and KillLoop reach (import-table names:
// _AIL_close_stream@4, _AIL_set_sample_loop_block@12 and friends,
// KERNEL32's CloseHandle).
extern "C" {
__declspec(dllimport) void  __stdcall AIL_close_stream(void* hStream);
__declspec(dllimport) void  __stdcall AIL_set_sample_loop_block(void* hSample,
                                                                int nStart,
                                                                int nEnd);
__declspec(dllimport) void  __stdcall AIL_set_sample_loop_count(void* hSample,
                                                                int nCount);
__declspec(dllimport) void  __stdcall AIL_set_3D_sample_loop_block(void* hSample,
                                                                   int nStart,
                                                                   int nEnd);
__declspec(dllimport) void  __stdcall AIL_set_3D_sample_loop_count(void* hSample,
                                                                   int nCount);
__declspec(dllimport) void  __stdcall AIL_set_stream_loop_count(void* hStream,
                                                                int nCount);
__declspec(dllimport) int   __stdcall CloseHandle(void* hObject);
}


// 0x0047d5c0 — slot +0x08 of all four vtables: stop the sound and put the
// instance back the way the constructor left it. Stop(0) through the vtable
// first, then the table slot (0x0047d690), then the stream and file handles.
// The sample is unlinked and, when it was flagged 0x1000 and this was its
// last instance, told to unload through its slot +0x10. The tail is the
// constructor's reset again, in the order 0x0047d631..0x0047d67d stores it.
void CSoundInstanceView::Term()
{
	Stop(0);
	sub_47d690();

	if (m_hStream4c)
	{
		AIL_lock();
		AIL_close_stream(m_hStream4c);
		AIL_unlock();
		m_hStream4c = LTNULL;
	}

	if (m_hFile50 != (void*)-1)
	{
		CloseHandle(m_hFile50);
		m_hFile50 = (void*)-1;
	}

	m_StateFlags = 0;

	if (m_pData)
	{
		m_pData->RemoveInstance(this);

		if ((m_Flags10 & 0x1000) && !m_pData->m_Instances.m_nElements)
			m_pData->_v10();

		m_pData = LTNULL;
	}

	m_DataLink.TieOff();
	m_pUnknown0c = LTNULL;
	m_Flags10 = 0;
	m_Unknown18 = 0;
	m_nUnknown20 = 100;
	m_f34 = 1.0f;
	m_Unknown28 = 0;
	m_nUnknown54 = 0;
	m_Saved3c = 0;
	m_hSample = LTNULL;
	m_h3DSample = LTNULL;
	m_hStream4c = LTNULL;
	m_Unknown1c = 0;
	m_Unknown58 = 0;
	m_nUnknown7c = -1;

	for (uint32 i = 0; i < 7; i++)
	{
		m_Unknown80[i] = -1;
		m_Unknown9c[i] = 0.0f;
	}
}


// 0x0047d690 — give the client manager's table slot back. Called from Term
// (0x0047d5cf) and from the client's own sound sweep at 0x0047a1a3, which
// reaches it right after `testb $0x40,0x40(%ecx)` — the same m_StateFlags
// this file works in, which is what says the caller's object is one of these.
//
// Read off the bytes:
//
//   * the slot is loaded NARROW, `mov 0x14(%esi),%ax`, and compared as a word,
//     while the constructor stores the field with a dword `movl $0xffff`. A
//     uint32 field taken into a uint16 local is the only spelling that gives
//     both, and the local is then pushed whole (`push %eax`, upper half
//     stale) into tbl_ReleaseSlot's uint16 parameter — which is exactly the
//     push the callee's `and $0xffff` prologue expects.
//   * the two conditions are one `&&`: the word test and the bit test share a
//     single jump target past the body.
//   * bit 9 of m_Flags10 is read as `test $0x2,%ch`, VC6's byte form for
//     0x0200.
LTRESULT CSoundInstanceView::sub_47d690()
{
	uint16 nSlot = (uint16)m_nTableSlot;

	if (nSlot != 0xffff && !(m_Flags10 & 0x0200))
	{
		tbl_ReleaseSlot(g_pSlotTableOwner, nSlot);

		m_nTableSlot = 0xffff;
	}

	return LT_OK;
}


// ---- Init, the voice teardown and the 3D set-up -------------------------- //

// The Miles entries the three bodies below reach (import-table names:
// _AIL_set_sample_volume@8, _AIL_3D_user_data@8 and friends).
extern "C" {
__declspec(dllimport) void   __stdcall AIL_set_sample_volume(void* hSample, int nVolume);
__declspec(dllimport) void   __stdcall AIL_stop_sample(void* hSample);
__declspec(dllimport) uint32 __stdcall AIL_3D_sample_status(void* hSample);
__declspec(dllimport) void   __stdcall AIL_stop_3D_sample(void* hSample);
__declspec(dllimport) int    __stdcall AIL_3D_user_data(void* hObj, uint32 nIndex);
__declspec(dllimport) void   __stdcall AIL_set_3D_user_data(void* hObj, uint32 nIndex,
                                                            int nValue);
__declspec(dllimport) int    __stdcall AIL_set_3D_sample_info(void* hSample, void* pInfo);
__declspec(dllimport) void   __stdcall AIL_set_3D_sample_playback_rate(void* hSample,
                                                                       int nRate);
__declspec(dllimport) void   __stdcall AIL_set_3D_sample_distances(void* hSample,
                                                                   float fMax, float fMin);
__declspec(dllimport) void   __stdcall AIL_set_3D_sample_volume(void* hSample, int nVolume);
__declspec(dllimport) void   __stdcall AIL_3D_position(void* hObj, float* pX, float* pY,
                                                       float* pZ);
__declspec(dllimport) void   __stdcall AIL_set_3D_position(void* hObj, float fX, float fY,
                                                           float fZ);
__declspec(dllimport) void   __stdcall AIL_set_3D_velocity_vector(void* hObj, float dX,
                                                                  float dY, float dZ);
__declspec(dllimport) void   __stdcall AIL_set_3D_sample_preference(void* hSample,
                                                                    const char* pName,
                                                                    const void* pValue);
}

// Miles' SMP_PLAYING, what AIL_3D_sample_status answers for a live sample.
#define SND_SMP_PLAYING 4

// 0x004a9ebc — the "SoundEndCap" console variable: its row in the
// console-variable table at 0x004aa044 names it and points here. Read as a
// dword and tested against zero (name ours, from the table's string).
extern int g_CV_SoundEndCap;

// 0x00420460 — clienttexmgr.cpp's tbl_EnsureSize: make room for one id in
// the client manager's slot table.
void tbl_EnsureSize(void* pClientMgr, uint16 nID);

// The client manager's slot table (clienttexmgr.cpp's CClientSlot and the
// +0x11dc/+0x11e0 pair), from the side Init writes it.
struct CClientSlotSI
{
	uint8  m_nType;                          // 0x00
	uint8  m_pad_01[3];
	void*  m_pData;                          // 0x04
};

struct CClientSlotTableSI
{
	uint8          m_pad_00[0x11dc];
	CClientSlotSI* m_pSlots;                 // 0x11dc
	uint32         m_nSlots;                 // 0x11e0
};

// 0x00493f0c — the CRT's rand().
extern "C" int __cdecl rand(void);

// A uniform float in [min, max) — commonmodel.cpp's ci_Random, expanded
// here (Init calls no random helper; rand() is called directly at
// 0x0047d44f). Stand-in name.
inline float snd_Random(float fMin, float fMax)
{
	return (rand() * (1.0f / 32767.0f)) * (fMax - fMin) + fMin;
}


// 0x0047d3f0 — slot +0x04 of the base vtable: bind the instance to its
// sample and read the PlaySoundInfo. Term() through the vtable first, then
// the flags, priority, volume and pitch; the pitch is spread by the
// sample's own random range and refused back to 1.0 if it came out
// non-positive. A PLAYSOUND_CLIENT sound has no table slot and hands itself
// back through m_hSound; any other takes the slot its m_hSound names. The
// tail turns the start offset into the time left, wrapping on a loop and
// stopping outright when a one-shot starts past its end.
LTRESULT CSoundInstanceView::Init(CSoundDataView* pData, PlaySoundInfo* pInfo,
                                  uint32 nStartMS)
{
	Term();

	m_pData = pData;
	m_StateFlags = 0;
	m_pUnknown0c = pData->m_pUnknown04;
	m_Flags10 = pInfo->m_dwFlags;
	m_Unknown18 = pInfo->m_nPriority;

	if (m_Flags10 & PLAYSOUND_CTRL_VOL)
		m_nUnknown20 = pInfo->m_nVolume;

	if (m_Flags10 & PLAYSOUND_CTRL_PITCH)
		m_f34 = pInfo->m_fPitchShift;

	m_f34 *= snd_Random(-pData->m_fPitchRange, pData->m_fPitchRange) + 1.0f;

	if (m_f34 <= 0.0f)
		m_f34 = 1.0f;

	m_Unknown28 = nStartMS;
	*(float*)&m_Unknown1c = (float)m_Unknown18 + 1.0f;
	m_nUnknown54 = 0;
	m_hSample = LTNULL;
	m_h3DSample = LTNULL;
	m_hStream4c = LTNULL;
	m_Saved3c = 0;

	if (m_Flags10 & PLAYSOUND_CLIENT)
	{
		m_nTableSlot = 0xffff;
		pInfo->m_hSound = (HLTSOUND)this;
	}
	else
	{
		m_nTableSlot = (uint32)pInfo->m_hSound;
		tbl_EnsureSize(g_pSlotTableOwner, (uint16)(uint32)pInfo->m_hSound);
		((CClientSlotTableSI*)g_pSlotTableOwner)->m_pSlots[(uint16)(uint32)pInfo->m_hSound].m_nType = 2;
		((CClientSlotTableSI*)g_pSlotTableOwner)->m_pSlots[(uint16)(uint32)pInfo->m_hSound].m_pData = this;
	}

	if (m_pData->AddInstance(this))
		return LT_ERROR;

	m_Unknown58 = 0;

	m_Unknown2c = (uint32)(m_pData->m_nDurationMS / m_f34);
	m_nPriority = m_Unknown2c;

	if (m_Unknown28 > 0)
	{
		if (m_Unknown28 > m_Unknown2c)
		{
			if (m_Flags10 & PLAYSOUND_LOOP)
			{
				m_nPriority -= (m_Unknown28 - m_Unknown2c) % m_Unknown2c;
			}
			else
			{
				m_StateFlags &= ~0x02;
				m_StateFlags |= 0x04;
			}
		}
		else
		{
			m_nPriority -= m_Unknown28;
		}
	}

	m_StateFlags |= 0x01;

	return LT_OK;
}


// 0x0047d6e0 — slot +0x1c of all four vtables: let go of whichever Miles
// voice the instance holds. A 2D sample is silenced (volume 0, when
// SoundEndCap is off and a filter is set) or stopped, its user data cleared
// and the voice handed back; a 3D sample is stopped when forced or still
// playing, likewise returned; a stream is closed. Any open file handle goes
// last. The argument forces the 3D stop.
LTRESULT CSoundInstanceView::_v1c(int bForce)
{
	m_StateFlags &= ~0x02;

	if (m_hSample)
	{
		if (!g_CV_SoundEndCap && sub_47d3b0())
			AIL_set_sample_volume(m_hSample, 0);
		else
			AIL_stop_sample(m_hSample);

		ci_GetSoundMgrSI()->SetSampleUserData(m_hSample, 0);
		ci_GetSoundMgrSI()->FreeSWVoice(m_hSample);
		m_hSample = LTNULL;
	}
	else if (m_h3DSample)
	{
		AIL_lock();

		if (bForce || AIL_3D_sample_status(m_h3DSample) == SND_SMP_PLAYING)
			AIL_stop_3D_sample(m_h3DSample);

		AIL_unlock();

		ci_GetSoundMgrSI()->Set3DUserData(m_h3DSample, 0);
		ci_GetSoundMgrSI()->Free3DVoice(m_h3DSample);
		m_h3DSample = LTNULL;
	}
	else if (m_hStream4c)
	{
		AIL_lock();
		AIL_close_stream(m_hStream4c);
		m_hStream4c = LTNULL;
		AIL_unlock();
	}

	if (m_hFile50 != (void*)-1)
	{
		CloseHandle(m_hFile50);
		m_hFile50 = (void*)-1;
	}

	return LT_OK;
}


// 0x0047db20 — slot +0x0c of all four vtables: get a Miles 3D voice and set
// it up for this sound. A sample flagged 'lith' bit 0 is left alone. The
// voice is re-pointed at the sample only when its user data (index 3) is not
// already this sample: decompress on demand, hand Miles the AILSOUNDINFO,
// set the pitched rate when it differs. Either failure gives the voice back
// and answers LT_ERROR. Then distances, volume, position (only when it moved
// by more than 0.001 squared), a zero velocity and, with EAX on and no
// PLAYSOUND_REVERB, a zero reverb mix.
LTRESULT CSoundInstanceView::_v0c()
{
	LTVector vOffset;
	LTVector vCur;
	LTVector vDiff;
	CSoundDataView* pData;
	int nRate;
	float fMix;

	if (!m_h3DSample)
	{
		m_h3DSample = ci_GetSoundMgrSI()->GetFree3DVoice(this);
		if (!m_h3DSample)
			return LT_ERROR;
	}

	if (m_pData->m_nLithFlags & 0x01)
		return LT_OK;

	AIL_lock();

	pData = m_pData;
	if (AIL_3D_user_data(m_h3DSample, 3) != (int)pData)
	{
		if ((pData->m_wFormatTag == 0x11 || pData->m_wFormatTag == 0x55)
		    && !pData->m_pDecompressed
		    && pData->sub_47cec0())
		{
			AIL_unlock();
			ci_GetSoundMgrSI()->Free3DVoice(m_h3DSample);
			m_h3DSample = LTNULL;
			return LT_ERROR;
		}

		if (!AIL_set_3D_sample_info(m_h3DSample, m_pData->GetSoundInfo()))
		{
			AIL_unlock();
			ci_GetSoundMgrSI()->Free3DVoice(m_h3DSample);
			m_h3DSample = LTNULL;
			return LT_ERROR;
		}

		nRate = (int)(m_pData->GetWaveInfo()->nSamplesPerSec * m_f34 + 0.5f);
		if (nRate != m_pData->GetWaveInfo()->nSamplesPerSec)
			AIL_set_3D_sample_playback_rate(m_h3DSample, nRate);

		AIL_set_3D_user_data(m_h3DSample, 3, (int)m_pData);
	}

	AIL_set_3D_sample_distances(m_h3DSample, *(float*)&m_Unknown78,
	                            *(float*)&m_Unknown74);
	AIL_set_3D_sample_volume(m_h3DSample, (m_nUnknown20 * 127) / 100);

	AIL_3D_position(m_h3DSample, &vCur.x, &vCur.y, &vCur.z);
	GetListenerOffset(&vOffset);

	vDiff = vCur - vOffset;
	if (vDiff.MagSqr() > 0.001f)
		AIL_set_3D_position(m_h3DSample, vOffset.x, vOffset.y, vOffset.z);

	AIL_set_3D_velocity_vector(m_h3DSample, 0.0f, 0.0f, 0.0f);

	if (ci_GetSoundMgrSI()->m_bReverbB && !(m_Flags10 & PLAYSOUND_REVERB))
	{
		fMix = 0.0f;
		AIL_set_3D_sample_preference(m_h3DSample, "EAX sample reverb mix", &fMix);
	}

	ci_GetSoundMgrSI()->Set3DUserData(m_h3DSample, (uint32)this);

	AIL_unlock();

	return LT_OK;
}

// ======================================================================= //
//  0x0047df30, 903 bytes — slot +0x54 of all four vtables.  Start
//  (or restart) the sound at its current position.  (name ours)
//
//  WHAT PINS IT.
//   * three LT_ERROR (1) refusals: no Miles handle of any kind, no sample,
//     no wave description (GetWaveInfo(), pointer-level `add $8`/`lea
//     8(%edi)` at 0x0047df71, tested against null anyway).
//   * m_Flags10 bit 2 (looping): with the sample's loop marks
//     (m_bHasLoop, 0x58/0x5c) the block is each mark in ms / m_f34, as a
//     fraction of m_Unknown2c, times the data size, clamped to
//     [0, size] and [start, size]; without them it is (0, -1).  Then the
//     loop block and an infinite count (0) on whichever handle there is,
//     all under one AIL_lock.
//   * not looping, a 2D sample and no "SoundEndCap" (0x004a9ebc): when a
//     filter is set (slot +0x50, sub_47d3b0), loop the last two bytes --
//     no lock taken on this path (0x0047e15f only CACHES AIL_unlock).
//   * then position and go: a stream starts outright at the top of the
//     sound or seeks to m_Unknown2c - m_nPriority ms and un-pauses; a 3D
//     sample gets (1 - m_nPriority / m_Unknown2c) * size bytes, rounded
//     down to nBlockAlign, and is offset/started/resumed by its status; a
//     2D sample seeks by ms and resumes.
//   * finally m_StateFlags |= 0xa and m_Unknown30 = AIL_ms_count().
//  IDENTICAL.  Two source facts closed it: the loop END is its own float
//  statement like the start (shipped reads 0x5c and divides by m_f34
//  before the size ternary, 0x0047e01b), and the 3D arm tests `!=
//  SMP_PLAYING` first with the plain re-offset as the `else` (the `je` at
//  0x0047e234 jumps forward to it).
// ======================================================================= //
extern "C" {
__declspec(dllimport) uint32 __stdcall AIL_ms_count(void);
__declspec(dllimport) void   __stdcall AIL_start_stream(void* hStream);
__declspec(dllimport) void   __stdcall AIL_set_stream_ms_position(void* hStream, int nMS);
__declspec(dllimport) void   __stdcall AIL_pause_stream(void* hStream, int nOnOff);
__declspec(dllimport) void   __stdcall AIL_start_3D_sample(void* hSample);
__declspec(dllimport) void   __stdcall AIL_set_3D_sample_offset(void* hSample, uint32 nOffset);
__declspec(dllimport) void   __stdcall AIL_resume_3D_sample(void* hSample);
__declspec(dllimport) void   __stdcall AIL_set_sample_ms_position(void* hSample, int nMS);
__declspec(dllimport) void   __stdcall AIL_resume_sample(void* hSample);
}

LTRESULT CSoundInstanceView::Play()
{
	WaveInfoSI* pWave;
	float       fLoopStart;
	float       fLoopEnd;
	int         nLoopStart;
	int         nLoopEnd;
	uint32      nOffset;

	if (!m_hSample && !m_h3DSample && !m_hStream4c)
		return LT_ERROR;

	if (!m_pData)
		return LT_ERROR;

	pWave = m_pData->GetWaveInfo();
	if (!pWave)
		return LT_ERROR;

	if (m_Flags10 & 0x04)
	{
		if (m_pData->m_bHasLoop)
		{
			fLoopStart = m_pData->m_Unknown58 / m_f34;

			nLoopStart = (int)(fLoopStart / m_Unknown2c * m_pData->GetDataSize());
			if (nLoopStart < 0)
				nLoopStart = 0;
			else if (nLoopStart > (int)m_pData->GetDataSize())
				nLoopStart = m_pData->GetDataSize();

			fLoopEnd = m_pData->m_Unknown5c / m_f34;

			nLoopEnd = (int)(fLoopEnd / m_Unknown2c * m_pData->GetDataSize());
			if (nLoopEnd < nLoopStart)
				nLoopEnd = nLoopStart;
			else if (nLoopEnd > (int)m_pData->GetDataSize())
				nLoopEnd = m_pData->GetDataSize();
		}
		else
		{
			nLoopStart = 0;
			nLoopEnd = -1;
		}

		AIL_lock();

		if (m_hSample)
		{
			AIL_set_sample_loop_block(m_hSample, nLoopStart, nLoopEnd);
			AIL_set_sample_loop_count(m_hSample, 0);
		}
		else if (m_h3DSample)
		{
			AIL_set_3D_sample_loop_block(m_h3DSample, nLoopStart, nLoopEnd);
			AIL_set_3D_sample_loop_count(m_h3DSample, 0);
		}
		else if (m_hStream4c)
		{
			AIL_set_stream_loop_count(m_hStream4c, 0);
		}

		AIL_unlock();
	}
	else if (m_hSample && !g_CV_SoundEndCap)
	{
		if (sub_47d3b0())
		{
			AIL_set_sample_loop_block(m_hSample, m_pData->GetDataSize() - 2,
			                          m_pData->GetDataSize() - 1);
			AIL_set_sample_loop_count(m_hSample, m_pData->GetDataSize());
		}
	}

	m_nUnknown54 = 0;

	if (m_hStream4c)
	{
		if (m_nPriority == m_Unknown2c)
		{
			AIL_lock();
			AIL_start_stream(m_hStream4c);
		}
		else
		{
			nOffset = m_Unknown2c - m_nPriority;
			AIL_lock();
			AIL_set_stream_ms_position(m_hStream4c, nOffset);
			AIL_pause_stream(m_hStream4c, 0);
		}

		AIL_unlock();
	}
	else if (m_h3DSample)
	{
		nOffset = (uint32)((1.0f - (float)m_nPriority / m_Unknown2c) * m_pData->GetDataSize());
		nOffset -= nOffset % pWave->nBlockAlign;

		AIL_lock();

		if (AIL_3D_sample_status(m_h3DSample) != SND_SMP_PLAYING)
		{
			if (!nOffset)
			{
				AIL_start_3D_sample(m_h3DSample);
			}
			else
			{
				AIL_set_3D_sample_offset(m_h3DSample, nOffset);
				AIL_resume_3D_sample(m_h3DSample);
			}
		}
		else
		{
			AIL_set_3D_sample_offset(m_h3DSample, nOffset);
		}

		AIL_unlock();
	}
	else if (m_hSample)
	{
		nOffset = m_Unknown2c - m_nPriority;
		AIL_lock();
		AIL_set_sample_ms_position(m_hSample, nOffset);
		AIL_resume_sample(m_hSample);
		AIL_unlock();
	}

	m_StateFlags |= 0x0a;

	AIL_lock();
	m_Unknown30 = AIL_ms_count();
	AIL_unlock();

	return LT_OK;
}


// 0x0047e2c0 — take the loop off a playing sound so it plays out the rest of
// its sample (cchandler.cpp's KillLoop, clientsoundmgr.cpp's caller). Refused
// (LT_ERROR) unless m_Flags10 bit 2 says it loops; otherwise whichever Miles
// handle it has gets a whole-sample loop block and a count of one, and the
// state takes 0x40.
LTRESULT CSoundInstanceView::KillLoop()
{
	if (!(m_Flags10 & 0x04))
		return LT_ERROR;

	AIL_lock();

	if (m_hSample)
	{
		AIL_set_sample_loop_block(m_hSample, 0, -1);
		AIL_set_sample_loop_count(m_hSample, 1);
	}
	else if (m_h3DSample)
	{
		AIL_set_3D_sample_loop_block(m_h3DSample, 0, -1);
		AIL_set_3D_sample_loop_count(m_h3DSample, 1);
	}
	else if (m_hStream4c)
	{
		AIL_set_stream_loop_count(m_hStream4c, 1);
	}

	AIL_unlock();

	m_StateFlags |= 0x40;

	return LT_OK;
}


// 0x0047e360 — come off the sample: stop, unlink, and tie the link off so the
// instance is safe to relink. Called from sub_47e390 just below and from
// 0x0047fa4f.
//
// The unlink is the sample's, not this class's: the shipped `mov 0x8(%esi),%ecx`
// puts m_pData in ecx and then pushes this, which is a thiscall on the sample
// with the instance as its argument.
//
// The tie-off is inlined (`lea 0x5c(%esi),%eax; mov %eax,0x4(%eax); mov
// %eax,(%eax)`) because ltlink.h's is an in-class one-liner, and it runs on
// both paths — it is below the `if`, not in it.
LTRESULT CSoundInstanceView::sub_47e360()
{
	_v1c(1);

	if (m_pData)
	{
		m_pData->RemoveInstance(this);

		m_pData = LTNULL;
	}

	m_DataLink.TieOff();

	return LT_OK;
}


// 0x0047e390 — and back on: detach, then take the sample m_pUnknown0c's first
// dword points at and register with it.
//
// Read off the bytes:
//
//   * both guards land on the same `mov $1,%eax` epilogue, so a missing
//     record and a missing sample are both success and not an error.
//   * m_pData is stored BEFORE the call and out of the register the guard
//     already loaded — and the shipped code then re-tests the same zero flag
//     (`je` on a condition it has already acted on). That dead test is the
//     inline helper's own guard: VC6 folds it away when the store and the
//     test are written in this body (as an && term or a nested `if`, both
//     measured), but keeps it when they come from an expanded inline. The
//     helper's name is ours, a stand-in for whatever the source called it.
//   * the answer is `!!AddInstance(this)` — the `neg; sbb; neg` again.
//
// What m_pUnknown0c points at is not established beyond its first dword being
// a CSoundData*, so it stays a void* and the deref is spelled out.
inline LTBOOL CSoundInstanceView::AttachTo(CSoundDataView* p)
{
	m_pData = p;
	if (!m_pData)
		return LTTRUE;
	return p->AddInstance(this) != 0;
}

LTBOOL CSoundInstanceView::sub_47e390()
{
	sub_47e360();

	if (m_pUnknown0c && *(CSoundDataView**)m_pUnknown0c)
		return AttachTo(*(CSoundDataView**)m_pUnknown0c);

	return LTTRUE;
}


// ---- CSoundInstance: the Miles filter ------------------------------------ //
//
// The three filter slots, +0x44/+0x48/+0x4c in all four vtables. The filter
// is remembered as its Miles enumeration index (m_nUnknown7c) and up to
// seven parameters as index/value pairs, so a filter set before the sound
// has a sample is applied when it gets one. CSoundMgr's SetSoundFilter,
// SetSoundFilterParam and GetSoundFilterParam (clientsound.cpp) forward here.

// 0x0047e3c0 — find the named filter among Miles' and select it. A null name
// clears the filter; an unknown name clears it and answers LT_NOTFOUND
// (`neg; sbb; and $0x3d`). Either way the parameters are forgotten
// (`rep stos` of -1 over m_Unknown80) and the sample, if any, gets the new
// processor.
LTRESULT CSoundInstanceView::SetFilter(const char* pFilter)
{
	void*  hFilter = LTNULL;
	LTBOOL bNotFound = LTFALSE;

	m_nUnknown7c = -1;

	if (pFilter)
	{
		uint32 nNext = 0;
		char*  pName;
		int    i = 0;

		while (AIL_enumerate_filters(&nNext, &hFilter, &pName))
		{
			if (_stricmp(pName, pFilter) == 0)
				break;

			hFilter = LTNULL;
			++i;
		}

		if (!hFilter)
			bNotFound = LTTRUE;
		else
			m_nUnknown7c = i;
	}

	for (int j = 0; j < 7; j++)
		m_Unknown80[j] = -1;

	if (m_hSample)
		AIL_set_sample_processor(m_hSample, SND_DP_FILTER, hFilter);

	return bNotFound ? LT_NOTFOUND : LT_OK;
}


// 0x0047e480 — set one parameter of the selected filter: resolve its index
// through the manager, remember it in the first entry that already has it
// or is free, and pass it to Miles when there is a sample.
LTRESULT CSoundInstanceView::SetFilterParam(const char* pParam, float fValue)
{
	const char* pFilterName;
	uint32 nIndex;

	if (!pParam)
		return LT_INVALIDPARAMS;

	if (ci_GetSoundMgrSI()->GetFilterName(m_nUnknown7c, &pFilterName) != LT_OK)
		return LT_NOTFOUND;

	if (ci_GetSoundMgrSI()->GetFilterParamIndex(pFilterName, pParam, &nIndex) != LT_OK)
		return LT_NOTFOUND;

	for (uint32 i = 0; i < 7; i++)
	{
		if (m_Unknown80[i] == (int)nIndex || m_Unknown80[i] == -1)
		{
			m_Unknown80[i] = nIndex;
			m_Unknown9c[i] = fValue;
			break;
		}
	}

	if (m_hSample)
		AIL_set_filter_sample_preference(m_hSample, pParam, &fValue);

	return LT_OK;
}


// 0x0047e540 — and read one back: from Miles when there is a sample,
// otherwise from the remembered entries, LT_ERROR (`sete`) if it is not one.
LTRESULT CSoundInstanceView::GetFilterParam(const char* pParam, float* pValue)
{
	const char* pFilterName;
	uint32 nIndex;
	LTBOOL bFound;

	if (!pParam)
		return LT_INVALIDPARAMS;

	if (ci_GetSoundMgrSI()->GetFilterName(m_nUnknown7c, &pFilterName) != LT_OK)
		return LT_NOTFOUND;

	if (ci_GetSoundMgrSI()->GetFilterParamIndex(pFilterName, pParam, &nIndex) != LT_OK)
		return LT_NOTFOUND;

	if (m_hSample)
	{
		AIL_filter_sample_attribute(m_hSample, pParam, pValue);

		return LT_OK;
	}

	bFound = LTFALSE;

	for (uint32 i = 0; i < 7; i++)
	{
		if (m_Unknown80[i] == (int)nIndex)
		{
			*pValue = m_Unknown9c[i];
			bFound = LTTRUE;
			break;
		}
	}

	return !bFound;
}


// ---- the m_Type == 1 class ------------------------------------------------ //

// The derived class whose vtable is 0x004a34a4. It adds no fields of its own
// that the constructor shows; what it does is finish initialising the base's
// position and the two dwords past it, which the base constructor leaves
// alone (name ours, from m_Type).
class CSoundInstance1 : public CSoundInstanceView
{
public:
	CSoundInstance1();                       // 0x0047e970
};

// 0x0047e970 — construct it. Called from 0x004817b4 and 0x00483500.
//
// Nothing here is body code: the base constructor runs first (`call
// 0x0047d310`), the vptr is overwritten, and the five stores are the ones
// declaration order puts after it. m_Type is assigned rather than
// initialised — the base constructor already put a zero there.
//
// The zero is materialised once into eax and reused for all five stores.
// The vector goes through LTVector::Init: written as three component stores
// VC6 scheduled the m_Type store after all five zeroes, and Init puts it
// first, as shipped.
CSoundInstance1::CSoundInstance1()
{
	m_Type = 1;

	m_vPos.Init(0.0f, 0.0f, 0.0f);

	m_Unknown74 = 0;
	m_Unknown78 = 0;
}


// ---- the m_Type == 2 class: the Miles 3D sample --------------------------- //

// The derived class whose vtable is 0x004a3508, constructed at 0x0047eda0.
// It is the only one of the four that overrides the obstruction and occlusion
// slots — the base leaves all four of them pointing at 0x0042d190, a folded
// `return 1` shared image-wide — and the handle it works through is the base's
// m_h3DSample at 0x48 (name ours, from m_Type).
class CSoundInstance2 : public CSoundInstanceView
{
public:
	CSoundInstance2();                           // 0x0047eda0
	LTRESULT SetPosition(LTVector* pPos, int bReset);    // +0x38  0x0047edf0
	LTRESULT SetObstruction(LTFLOAT fLevel);     // +0x28  0x0047f590
	LTRESULT GetObstruction(LTFLOAT* pLevel);    // +0x2c  0x0047f5f0
	LTRESULT SetOcclusion(LTFLOAT fLevel);       // +0x30  0x0047f5c0
	LTRESULT GetOcclusion(LTFLOAT* pLevel);      // +0x34  0x0047f5f0

	// 0xb8 and 0xc4 — two more vectors, the class's own (the bank at
	// 0x00483510 allocates 0xd0 bytes). SetPosition with its second
	// argument set copies the new position into the first and zeroes the
	// second; names ours.
	LTVector m_vLastPos;                         // 0xb8
	LTVector m_vVelocity;                        // 0xc4
};

// 0x0047eda0 — construct it. The CSoundInstance1 constructor's shape with
// the two class vectors zeroed between the position and 0x74/0x78, which is
// the shipped store order (0x0047edc0..0x0047ede7).
CSoundInstance2::CSoundInstance2()
{
	m_Type = 2;

	m_vPos.Init(0.0f, 0.0f, 0.0f);
	m_vLastPos.Init(0.0f, 0.0f, 0.0f);
	m_vVelocity.Init(0.0f, 0.0f, 0.0f);

	m_Unknown74 = 0;
	m_Unknown78 = 0;
}

// 0x0047edf0 — the base's SetPosition (0x0047d3c0) plus a reset: with the
// second argument non-zero the position is also copied to 0xb8 and 0xc4 is
// zeroed (0x0047ee13 `je` over both).
LTRESULT CSoundInstance2::SetPosition(LTVector* pPos, int bReset)
{
	m_vPos = *pPos;

	if (bReset)
	{
		m_vLastPos = *pPos;
		m_vVelocity.Init(0.0f, 0.0f, 0.0f);
	}

	return LT_OK;
}

// 0x0047f590 — the obstruction, straight through to Miles under its lock.
// The handle is pushed last, so it is the first argument.
LTRESULT CSoundInstance2::SetObstruction(LTFLOAT fLevel)
{
	AIL_lock();

	AIL_set_3D_sample_obstruction(m_h3DSample, fLevel);

	AIL_unlock();

	return LT_OK;
}

// 0x0047f5c0 — the same shape for the occlusion, and the same shape in the
// bytes: only the import differs.
LTRESULT CSoundInstance2::SetOcclusion(LTFLOAT fLevel)
{
	AIL_lock();

	AIL_set_3D_sample_occlusion(m_h3DSample, fLevel);

	AIL_unlock();

	return LT_OK;
}

// 0x0047f5f0 — and reading it back. The out pointer is loaded AFTER the Miles
// call and the value goes straight from the FPU stack into it (`fstps
// (%ecx)`), which is a float assignment and not a local.
//
// A SHIPPED BUG, and the vtable is the evidence. 0x0047f5f0 fills BOTH slot
// +0x2c and slot +0x34 of vtable 0x004a3508 — GetObstruction and
// GetOcclusion. Two functions cannot share an address unless their bodies
// came out identical, and the body calls AIL_3D_sample_obstruction, so
// GetOcclusion asks Miles for the obstruction. The linker's identical-COMDAT
// folding then collapsed the two into one image.
//
// Both are therefore written out, identically, and folding is what reproduces
// the two slots from the one address. Writing only one and pointing the second
// slot at it would not be the same source.
LTRESULT CSoundInstance2::GetObstruction(LTFLOAT* pLevel)
{
	AIL_lock();

	*pLevel = AIL_3D_sample_obstruction(m_h3DSample);

	AIL_unlock();

	return LT_OK;
}

LTRESULT CSoundInstance2::GetOcclusion(LTFLOAT* pLevel)
{
	AIL_lock();

	*pLevel = AIL_3D_sample_obstruction(m_h3DSample);

	AIL_unlock();

	return LT_OK;
}


// ---- bringing Miles up ---------------------------------------------------- //

// 0x0047f9b0 — set the four Miles preferences the engine wants before it
// opens a digital driver. A free function: no `this`, no arguments, no
// return, and both callers (0x0047fad7 in CSoundMgr::sub_47f9e0 and
// 0x0048027b) reach it with a plain call (name ours).
//
// The four numbers are shipped immediates and mss32's own preference indices;
// mss.h is not part of this reconstruction, so they are left as numbers
// rather than guessed at by name. AIL_set_preference is loaded into a
// register once and called four times through it, which is what four calls to
// one import in a row give.
void snd_SetMilesPreferences()
{
	AIL_lock();

	AIL_set_preference(0x1b, 1);
	AIL_set_preference(0x0f, 0);
	AIL_set_preference(0x28, 0x40000);
	AIL_set_preference(0x12, 0);

	AIL_unlock();
}


// *********************************************************************** //
//
//  NOT PART OF THIS TRANSLATION UNIT.
//
//  Two more of the small functions in the 0x00479160-0x0047f9b0 run are
//  fully determined but belong to the CLIENT PACKET HANDLERS at the head of
//  it, i.e. to clientmgr.cpp, and are recorded here rather than compiled so
//  that reading them again is not necessary.
//
//  0x00479280 — 50 bytes, cdecl, six arguments, called from 0x00478b90 and
//  0x0047a39f. Two calls and nothing else; the single `add $0x20,%esp`
//  cleans both, which is what says both are cdecl (twelve bytes for the
//  first, twenty for the second):
//
//      T sub_479280(A a1, B a2, C a3, D a4, E a5, F a6)
//      {
//          sub_4792c0(a1, a3, a6);
//          return sub_4789d0(a1, a2, a4, a5, a6);
//      }
//
//  a3 is used by the first call only and the result of the second is
//  returned untouched. sub_4792c0 is the 1504-byte body at the centre of the
//  run — the change-flag decoder itself.
//
//  0x0047a790 — 51 bytes, no caller anywhere in .text (it is reached through
//  a handler table), two arguments:
//
//      LTRESULT sub_47a790(A* pA, B* pReader)
//      {
//          uint32 nFirst  = pReader->sub_45c280();
//          uint32 nSecond = pReader->sub_45c280();
//
//          sub_41b0f0((char*)pA->m_p78 + 0x1268, nFirst, nSecond);
//
//          return LT_OK;
//      }
//
//  0x0045c280 is a thiscall reader over a bit-addressed buffer (a bit
//  cursor at +0x38, a limit at +0x36, the base at +0x54, a length at +0x58);
//  the two reads are in source order, which two argument expressions could
//  not be — VC6 evaluates those right to left — so there are two locals.
//  sub_41b0f0 is cdecl and takes the pointer first.
//
// *********************************************************************** //
