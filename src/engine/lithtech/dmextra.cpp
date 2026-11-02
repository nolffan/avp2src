// *********************************************************************** //
//
//  MODULE:   CLIENTMUSIC2 (.CPP)   [scratch name: q440000.cpp]
//
//  PURPOSE:  The rest of CLTDirectMusicMgr - the half that clientmusic.cpp
//            does not carry yet: bringing DirectMusic up and down, the port's
//            reverb properties, the control-file readers' loading helpers,
//            the two enact-type converters, and the two halves of the
//            loader's GetObject that live past 0x00443b90.
//
//  This file is the continuation of src/engine/lithtech/clientmusic.cpp and
//  src/engine/lithtech/dmloader.cpp.  EVERY class layout, member name and
//  helper address below is taken verbatim from clientmusic.h / dmloader.cpp;
//  where a member is new here it is marked "(new here)" and justified by the
//  address it was read from.  The stand-in declarations are only here so the
//  file compiles standalone - when this lands they should be folded back into
//  clientmusic.h rather than kept in parallel.
//
//  WHAT THE COM SLOTS ARE
//
//  Three interfaces are reached in this file, and each one's anchoring is a
//  fact rather than a guess:
//
//    IDirectMusicPerformance8, through m_pPerformance (0x014).  Slot 0x0c is
//    called at 0x00440f66 with (&m_pDirectMusic, 0, 0) - that is Init, and
//    counting dmusici.h's declaration order from there puts Stop at 0x14,
//    IsPlaying at 0x38, RemovePort at 0x64, SetGlobalParam at 0x88 and
//    CloseDown at 0x98.  clientmusic.h had already anchored 0x14/0x38/0x88
//    independently, and 0x98 (CloseDown, called at 0x00440fbf and 0x00441152
//    right before Release) and 0x64 (RemovePort, called at 0x004410f0 with
//    the port) fall exactly where that order puts them.
//
//    IDirectMusic8, through m_pDirectMusic (0x00c).  Slot 0x2c is called at
//    0x00440f99 and 0x0044113e with two nulls - SetDirectSound(pDS, hWnd) -
//    and clientmusic.h had already put CreatePort at 0x14 of the same object.
//
//    IKsControl, on the port.  m_pPort->QueryInterface (slot 0x00) is asked
//    for 0x004a392c at 0x0044235b / 0x00442403 / 0x004424b3, and those bytes
//    are {28F54685-06FD-11D2-B27A-00A0C9223196} = IID_IKsControl.  The slot
//    0x0c that all three then call takes six stack arguments - this, a 0x18-
//    byte property, 0x18, a data pointer, a length and a BytesReturned out -
//    which is IKsControl::KsProperty exactly.
//
//  The two property GUIDs are read off .rdata as well:
//    0x004a3a1c {CDA8D611-684A-11D2-871E-00600893B1BD} GUID_DMUS_PROP_Effects
//    0x004a3a2c {04CB5622-32E5-11D2-AFA6-00AA0024D8B6} GUID_DMUS_PROP_WavesReverb
//  and the two Init ones:
//    0x004a3a0c {D2AC2881-B39B-11D1-8704-00600893B1BD} CLSID_DirectMusicPerformance
//    0x004a393c {6FC2CAE0-BC78-11D2-AFA6-00AA0024D8B6} IID_IDirectMusicPerformance8
//
//  Relocations are masked, so a GUID that is declared and never defined still
//  matches; only the fact that an address is pushed has to be right.
//
// *********************************************************************** //

#include <string.h>

#include "ltbasedefs.h"
#include "iltdirectmusic.h"


// --------------------------------------------------------------------- //
// Stand-ins.  Everything in this block is copied from clientmusic.h unless
// it says "(new here)".
// --------------------------------------------------------------------- //

struct DMGUID { uint32 m_Data[4]; };                 // clientmusic.h
struct DMCRITICAL_SECTION { uint8 m_Data[24]; };     // clientmusic.h

// clientmusic.h.
extern const DMGUID GUID_PerfMasterVolume;
// (new here) 0x004a392c, the riid the port is queried with.
extern const DMGUID IID_IKsControl;
// (new here) 0x004a3a1c and 0x004a3a2c, the two port properties.
extern const DMGUID GUID_DMUS_PROP_Effects;
extern const DMGUID GUID_DMUS_PROP_WavesReverb;
// (new here) 0x004a3a0c / 0x004a393c, what Init CoCreateInstances.
extern const DMGUID CLSID_DirectMusicPerformance;
extern const DMGUID IID_IDirectMusicPerformance8;
// clientmusic.h: 0x004a3a3c.
extern const DMGUID CLSID_DirectMusicCollection;
// (new here) 0x004a3b30, sixteen zero bytes: the port class CreatePort asks
// for, i.e. the default synth.
extern "C" const DMGUID GUID_NULL;             // uuid.lib

// clientmusic.h's DMUS_PORTPARAMS8 (dwSize 0x24, dwValidParams 0xf).
struct DMUS_PORTPARAMS8
{
	uint32 dwSize;                           // 0x000
	uint32 dwValidParams;                    // 0x004
	uint32 dwVoices;                         // 0x008
	uint32 dwChannelGroups;                  // 0x00c
	uint32 dwAudioChannels;                  // 0x010
	uint32 dwSampleRate;                     // 0x014
	uint32 dwEffectFlags;                    // 0x018
	int    fShare;                           // 0x01c
	uint32 dwFeatures;                       // 0x020
};

// (new here) KSPROPERTY out of ks.h - a 16-byte set GUID, an id and a flags
// word, 0x18 bytes, which is the PropertyLength all three reverb calls pass.
// KSPROPERTY_TYPE_GET is 1 and KSPROPERTY_TYPE_SET is 2, which is what the
// Flags stores at 0x0044250d (get) and 0x00442461 / 0x00442578 (set) are.
#define KSPROPERTY_TYPE_GET 1
#define KSPROPERTY_TYPE_SET 2

struct KSPROPERTY
{
	DMGUID Set;                              // 0x000
	uint32 Id;                               // 0x010
	uint32 Flags;                            // 0x014
};

// (new here) dmusicc.h's DMUS_WAVES_REVERB_PARAMS: four floats, and the 0x10
// the KsProperty at 0x0044239e passes as its data length says there are
// exactly four.  The order is fixed by ReadReverbParameters (0x004425b0),
// which fills 0x0ec from REVERBINGAIN, 0x0f0 from REVERBMIX, 0x0f4 from
// REVERBTIME and 0x0f8 from REVERBHIGHFREQRTRATIO and then copies those
// sixteen bytes straight into this argument.
struct DMUS_WAVES_REVERB_PARAMS
{
	float fInGain;                           // 0x000
	float fReverbMix;                        // 0x004
	float fReverbTime;                       // 0x008
	float fHighFreqRTRatio;                  // 0x00c
};

// (new here) DMUS_EFFECT_REVERB.  0x004423e0 writes 1 into the Effects
// property and 0x00442490 reads it back and clears bit 0, so the bit this
// class owns is 1 - dmusicc.h's DMUS_EFFECT_REVERB.
#define DMUS_EFFECT_REVERB 1

// The COM shapes, only as deep as this file reaches.  Slot numbering is
// justified in the file header.
struct IDirectMusicSegmentState                      // clientmusic.h
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;                            // 0x08
};

// (new here) the interface the port hands out for its property sets.
struct IKsControl
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;                            // 0x08
	virtual long __stdcall KsProperty(KSPROPERTY* pProperty, uint32 cbProperty,
	                                  void* pPropertyData, uint32 cbPropertyData,
	                                  uint32* pBytesReturned)=0;     // 0x0c
};

// clientmusic.h's IDirectMusicPerformance8, with the three slots this file
// needs filled in (they were placeholders there).
struct IDirectMusicPerformance8
{
	virtual long __stdcall _p00()=0; virtual long __stdcall _p04()=0;
	virtual uint32 __stdcall Release()=0;                            // 0x08
	// 0x0c: called at 0x00440f66 with (&m_pDirectMusic, 0, 0).
	virtual long __stdcall Init(struct IDirectMusic8** ppDirectMusic,
	                            void* pDirectSound, void* hWnd)=0;
	// 0x10: called at 0x0044192f with (pSegment, 0x80, 0, 0, 0).
	virtual long __stdcall PlaySegment(void* pSegment, uint32 dwFlags,
	                                   __int64 i64StartTime,
	                                   IDirectMusicSegmentState** ppSegState)=0;
	// 0x14: called at 0x00441126 with four nulls.
	virtual long __stdcall Stop(void* pSegment, IDirectMusicSegmentState* pSegState,
	                            uint32 mtTime, uint32 dwFlags)=0;
	virtual long __stdcall _p18()=0; virtual long __stdcall _p1c()=0;
	virtual long __stdcall _p20()=0; virtual long __stdcall _p24()=0;
	virtual long __stdcall _p28()=0; virtual long __stdcall _p2c()=0;
	virtual long __stdcall _p30()=0; virtual long __stdcall _p34()=0;
	virtual long __stdcall _p38()=0; virtual long __stdcall _p3c()=0;
	virtual long __stdcall _p40()=0; virtual long __stdcall _p44()=0;
	virtual long __stdcall _p48()=0; virtual long __stdcall _p4c()=0;
	// 0x50 / 0x58: Init's notification set-up (0x0043f9f5, 0x0043f9d3).
	virtual long __stdcall SetNotificationHandle(void* hNotification,
	                                             __int64 rtMinimum)=0;
	virtual long __stdcall _p54()=0;
	virtual long __stdcall AddNotificationType(const DMGUID& rguidNotificationType)=0;
	virtual long __stdcall _p5c()=0;
	// 0x60 / 0x68: called at 0x0044107f with the port and at 0x004410a4
	// with (block, port, block + 1) - AddPort and AssignPChannelBlock.
	virtual long __stdcall AddPort(struct IDirectMusicPort* pPort)=0;
	// 0x64: called at 0x004410f0 with m_pPort.  (In ReleasePort, 0x004410d0,
	// which is NOT written here - it is declared below.)
	virtual long __stdcall RemovePort(struct IDirectMusicPort* pPort)=0;
	virtual long __stdcall AssignPChannelBlock(uint32 dwBlockNum,
	                                           struct IDirectMusicPort* pPort,
	                                           uint32 dwGroup)=0;
	virtual long __stdcall _p6c()=0;
	virtual long __stdcall _p70()=0; virtual long __stdcall _p74()=0;
	virtual long __stdcall _p78()=0; virtual long __stdcall _p7c()=0;
	virtual long __stdcall _p80()=0; virtual long __stdcall _p84()=0;
	virtual long __stdcall _p88()=0; virtual long __stdcall _p8c()=0;
	virtual long __stdcall _p90()=0; virtual long __stdcall _p94()=0;
	// 0x98: called at 0x00440fbf and 0x00441152, in both cases immediately
	// before the performance's own Release.
	virtual long __stdcall CloseDown()=0;
};

struct IDirectMusic8
{
	virtual long __stdcall _p00()=0; virtual long __stdcall _p04()=0;
	virtual uint32 __stdcall Release()=0;                            // 0x08
	virtual long __stdcall _p0c()=0;
	virtual long __stdcall _p10()=0;
	// 0x14: called at 0x0044105b with (GUID_NULL, &m_PortParams, &m_pPort, 0).
	virtual long __stdcall CreatePort(const DMGUID& rclsidPort,
	                                  struct DMUS_PORTPARAMS8* pPortParams,
	                                  struct IDirectMusicPort** ppPort,
	                                  void* pUnkOuter)=0;
	virtual long __stdcall _p18()=0; virtual long __stdcall _p1c()=0;
	virtual long __stdcall _p20()=0; virtual long __stdcall _p24()=0;
	virtual long __stdcall _p28()=0;
	// 0x2c: called at 0x00440f99 and 0x0044113e with two nulls.
	virtual long __stdcall SetDirectSound(void* pDirectSound, void* hWnd)=0;
};

// Only QueryInterface is reached here (the three reverb calls); the rest is
// spacing.  clientmusic.h already put Activate at 0x3c.
struct IDirectMusicPort
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;                            // 0x08
	virtual long __stdcall _p0c()=0;
	virtual long __stdcall _p10()=0; virtual long __stdcall _p14()=0;
	virtual long __stdcall _p18()=0; virtual long __stdcall _p1c()=0;
	virtual long __stdcall _p20()=0; virtual long __stdcall _p24()=0;
	virtual long __stdcall _p28()=0; virtual long __stdcall _p2c()=0;
	virtual long __stdcall _p30()=0; virtual long __stdcall _p34()=0;
	virtual long __stdcall _p38()=0;
	// 0x3c: called with 1 at 0x0044106c and with 0 at 0x004410df.
	virtual long __stdcall Activate(int fActive)=0;
};

// (new here) The loaded objects the readers ask the loader for.  The slot
// each is used through is all that is established:
//
//   IDirectMusicSegment, slot 0x4c, five arguments, the first of them the
//   GUID at 0x004a39ac and the second 0xffffffff - a group mask - and the
//   rest zero.  That is SetParam(rguidType, dwGroupBits, dwIndex, mtTime,
//   pParam) by shape.  The SDK dmusici.h in this tree's era puts SetParam at
//   0x40 and GetStartPoint (one argument) at 0x4c, so the header Monolith
//   compiled against orders three methods differently; the slot is what is
//   certain, so the placeholders below are what put the call at 0x4c.
//
//   IDirectMusicStyle, slot 0x0c, called at 0x004418a4 with a wide name and
//   an out-pointer: GetBand(WCHAR*, IDirectMusicBand**), the interface's
//   first method.
struct IDirectMusicSegment
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;
	virtual long __stdcall _p0c()=0; virtual long __stdcall _p10()=0;
	virtual long __stdcall _p14()=0; virtual long __stdcall _p18()=0;
	virtual long __stdcall _p1c()=0; virtual long __stdcall _p20()=0;
	virtual long __stdcall _p24()=0; virtual long __stdcall _p28()=0;
	virtual long __stdcall _p2c()=0; virtual long __stdcall _p30()=0;
	virtual long __stdcall _p34()=0; virtual long __stdcall _p38()=0;
	virtual long __stdcall _p3c()=0; virtual long __stdcall _p40()=0;
	virtual long __stdcall _p44()=0; virtual long __stdcall _p48()=0;
	// 0x4c, read off 0x004414a8.
	virtual long __stdcall SetParam(const DMGUID& rguidType, uint32 dwGroupBits,
	                                uint32 dwIndex, uint32 mtTime, void* pParam)=0;
};

struct IDirectMusicBand;

struct IDirectMusicStyle
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;
	// 0x0c, read off 0x004418a4.
	virtual long __stdcall GetBand(uint16* pwszName, IDirectMusicBand** ppBand)=0;
	virtual long __stdcall _p10()=0; virtual long __stdcall _p14()=0;
	virtual long __stdcall _p18()=0;
	// 0x1c, read off 0x00441ca6: GetMotif(WCHAR*, IDirectMusicSegment**).
	virtual long __stdcall GetMotif(uint16* pwszMotif,
	                                IDirectMusicSegment** ppSegment)=0;
};

// clientmusic.h, slots 0x0c/0x10/0x14 as it established them.
struct IDirectMusicBand
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;
	virtual long __stdcall CreateSegment(IDirectMusicSegmentState** ppSegment)=0; // 0x0c
	virtual long __stdcall Download(IDirectMusicPerformance8* pPerf)=0;           // 0x10
	virtual long __stdcall Unload(IDirectMusicPerformance8* pPerf)=0;             // 0x14
};

// (new here) IPersistStream, the interface CDMLoader::GetObject's two halves
// ask the freshly created object for (0x004a3b60) and then Load through.
// ole2's declaration order puts Load at 0x14, which is the slot called at
// 0x00443e4e and 0x00443f16.
struct IPersistStream
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;
	virtual long __stdcall GetClassID(DMGUID* pClassID)=0;           // 0x0c
	virtual long __stdcall IsDirty()=0;                              // 0x10
	virtual long __stdcall Load(void* pStream)=0;                     // 0x14
};

// The object CoCreateInstance makes for a described class.  Only the three
// IUnknown slots are reached (QueryInterface at 0x00443e3e / 0x00443f06,
// Release at 0x00443e73 / 0x00443f34).
struct IDirectMusicObject
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;
};

// (new here) 0x004a3b60.
extern "C" const DMGUID IID_IPersistStream;    // uuid.lib
// (new here) 0x004a396c {D2AC28B5-B39B-11D1-8704-00600893B1BD}, the riid both
// halves of GetObject CoCreateInstance the described class with.
extern const DMGUID IID_IDirectMusicObject;
// (new here) 0x004a395c {F96029A2-4282-11D2-8717-00600893B1BD} and
// 0x004a39ec {D2AC288A-...} / 0x004a39fc {D2AC2882-...}: the riid the segment
// reader asks for and the two class GUIDs the readers put in guidClass.
extern const DMGUID IID_IDirectMusicSegment;
extern const DMGUID IID_IDirectMusicStyle;            // 0x004a394c {D2AC28BD-...}
extern const DMGUID CLSID_DirectMusicSegment;         // 0x004a39fc
extern const DMGUID CLSID_DirectMusicStyle;           // 0x004a39ec
// (new here) 0x004a39ac {D2AC28A7-B39B-11D1-8704-00600893B1BD}.
extern const DMGUID GUID_Download;

#define DM_S_OK       0x00000000
#define DM_E_FAIL     0x80004005L
// 0x8007000e, the constant both halves of GetObject use when an allocation
// fails (0x00443e85, 0x00443f41).
#define DM_E_OUTOFMEMORY 0x8007000eL

// dmusici.h's DMUS_OBJ_ flags.  0x12 is what both readers store into
// dwValidData (0x00441482, 0x004416e9) and 0x20 is the bit 0x00443d84 tests.
#define DMUS_OBJ_CLASS    0x0002
#define DMUS_OBJ_FILENAME 0x0010
#define DMUS_OBJ_FULLPATH 0x0020

// DMUS_SEGF_SECONDARY, the flag the band's segment is played with
// (0x00441928).
#define DMUS_SEGF_SECONDARY 0x80

// dmusici.h's DMUS_OBJECTDESC.  Its 0x350 bytes are exactly the dwSize both
// readers store (0x00441462, 0x004416f6) and exactly the distance from the
// descriptor's base to the wide filename buffer plus MAX_PATH wide characters
// (0x00441441, 0x004416cd).  DMUS_MAX_NAME and DMUS_MAX_CATEGORY are 64 and
// DMUS_MAX_FILENAME is MAX_PATH, which is what makes the offsets come out.
struct DMUS_VERSION { uint32 dwVersionMS; uint32 dwVersionLS; };
struct DMFILETIME   { uint32 dwLowDateTime; uint32 dwHighDateTime; };

struct DMUS_OBJECTDESC
{
	uint32       dwSize;                     // 0x000
	uint32       dwValidData;                // 0x004
	DMGUID       guidObject;                 // 0x008
	DMGUID       guidClass;                  // 0x018
	DMFILETIME   ftDate;                     // 0x028
	DMUS_VERSION vVersion;                   // 0x030
	uint16       wszName[64];                // 0x038
	uint16       wszCategory[64];            // 0x0b8
	uint16       wszFileName[260];           // 0x138
	uint32       llMemLengthLow;             // 0x340
	int32        llMemLengthHigh;            // 0x344
	uint8*       pbMemData;                  // 0x348
	void*        pStream;                    // 0x34c
};


// --------------------------------------------------------------------- //
// The module's own furniture, all of it from clientmusic.h / dmloader.cpp.
// --------------------------------------------------------------------- //

// The chunk every bank's grow allocates: twelve bytes, its own links and the
// record array it owns.  0x00442b50's `push $0xc` is the size and its
// 0x00442b9f/0x00442ba6 stores are the links (names ours).
class CDMChunkQ440
{
public:
	CDMChunkQ440* m_pNext;                   // 0x000
	CDMChunkQ440* m_pPrev;                   // 0x004
	void*         m_pRecords;                // 0x008
};

class CDMChunkListQ440
{
public:
	CDMChunkQ440* m_pHead;                   // 0x000
	CDMChunkQ440* m_pTail;                   // 0x004

	// 0x00442cc0 out of line, inline at 0x00442b9c.  Same shape either way.
	void AddHead(CDMChunkQ440* pChunk);
	// 0x00442c90, defined in dmgrow.cpp.  Term calls it at four of its five
	// bank blocks and the shipped image expands it at the fifth.
	void Remove(CDMChunkQ440* pChunk);

	// The same body, in class: Term's style-def block (0x0043fc1f..
	// 0x0043fc4a) is the one site the shipped image expands.  Stand-in
	// twin, name ours.
	void RemoveInlineQ440(CDMChunkQ440* pChunk)
	{
		if (pChunk->m_pPrev)
			pChunk->m_pPrev->m_pNext = pChunk->m_pNext;
		else
			m_pHead = pChunk->m_pNext;

		if (pChunk->m_pNext)
			pChunk->m_pNext->m_pPrev = pChunk->m_pPrev;
		else
			m_pTail = pChunk->m_pPrev;
	}
};

// clientmusic.h's CDMBank, with the 0x10 bytes it left as padding filled in
// from what the grows read: 0x04 is the number of records a grow makes
// (0x00442b67), 0x08/0x0c are the chunk list (0x00442b9c..0x00442bc0) and
// 0x00 is what Init sets to 1 after growing (0x0043fa24).  Grow() has a
// different body per instantiation - 0x004429d0 for the 0x54-byte segment
// records, 0x00442b50 for the 0x10-byte band records, 0x00442bd0 for the
// 0x50-byte style records - so the record-specific ones get their own class.
class CDMBank
{
public:
	int    m_bInitialized;                   // 0x000
	uint32 m_nGrowCount;                     // 0x004
	CDMChunkListQ440 m_Chunks;               // 0x008
	void*  m_pFree;                          // 0x010

	int   Grow();

	// What Init does to each bank (0x0043fa07..0x0043fa99): set the grow
	// size, grow nPreGrow chunks up front, and only then call the bank up.
	// The `dec %edi ; jne` down-counter is the loop (name ours).
	void InitQ440(uint32 nGrowCount, uint32 nPreGrow)
	{
		m_nGrowCount = nGrowCount;

		for (uint32 i = nPreGrow; i; i--)
		{
			if (!Grow())
				return;
		}

		m_bInitialized = 1;
	}

	// 0x00442920 out of line, and inline at 0x00441838 and 0x0044143d.
	// Which of the two a given call site gets is VC6's inline budget, not
	// the source - clientmusic.h works the same shapes out at length.
	// The test re-reads m_pFree rather than the local: that is what loads
	// the record into eax and copies it to esi ahead of the `je`
	// (0x004413fa..0x00441401, 0x00441838..0x0044183f); testing the local
	// loads straight into esi.
	void* Pop()
	{
		void* pRecord = m_pFree;

		if (m_pFree)
			m_pFree = *(void**)m_pFree;

		return pRecord;
	}

	void* Alloc()
	{
		if (m_pFree)
		{
			void* pRecord = m_pFree;
			m_pFree = *(void**)m_pFree;
			return pRecord;
		}

		if (!Grow())
			return LTNULL;

		return Pop();
	}

	// The same fast path with the pop after the grow left as a CALL to
	// 0x00442920 - the MOTIF reader's shape (0x00441cb9..0x00441ce6).  That
	// site is past VC6's per-function expansion budget, which one in-class
	// Pop cannot express, so the out-of-line copy is named here.
	void* PopOutOfLineQ440();                                       // 0x00442920

	void* AllocOutOfLineQ440()
	{
		if (m_pFree)
		{
			void* pRecord = m_pFree;
			m_pFree = *(void**)m_pFree;
			return pRecord;
		}

		if (!Grow())
			return LTNULL;

		return PopOutOfLineQ440();
	}

	void Free(void* pRecord)
	{
		if (pRecord)
		{
			*(void**)pRecord = m_pFree;
			m_pFree = pRecord;
		}
	}
};

// The doubly-linked list helpers, shared by every node type (clientmusic.h).
void ListAddHeadQ440(void* pList, void* pNode);                     // 0x0048de30

// clientmusic.h's CDMSegment, with the one method this file defines.
// 0x54 bytes: the segment bank's grow (0x004429d0) strides by 0x54 and
// initialises 0x08, 0x0c, 0x4c and 0x50 of each record, which is exactly what
// 0x00441417..0x00441427 writes again into a recycled one.
class CDMSegment
{
public:
	CDMSegment* m_pNext;                     // 0x000
	CDMSegment* m_pPrev;                     // 0x004
	IDirectMusicSegment* m_pDMSegment;       // 0x008
	char   m_szName[0x4c - 0x0c];            // 0x00c  64 bytes
	char*  m_pName;                          // 0x04c
	LTDMEnactTypes m_nStopEnact;             // 0x050

	const char* GetName();                                          // 0x004420a0
	// 0x00441f20, defined below (name ours).
	LTBOOL SetName(const char* sName);
};

// clientmusic.h's CDMUnknown58 - the record the STYLE reader makes.  Its bank
// grows by 0x50 and initialises only 0x08, 0x0c and 0x4c (0x00442c17), so it
// is CDMSegment without the stop-enact word, and SetName's single body serves
// both (the linker folds the two instantiations).
class CDMStyleDefQ440
{
public:
	CDMStyleDefQ440* m_pNext;                // 0x000
	CDMStyleDefQ440* m_pPrev;                // 0x004
	IDirectMusicStyle* m_pDMStyle;           // 0x008
	char   m_szName[0x4c - 0x0c];            // 0x00c
	char*  m_pName;                          // 0x04c
};

// clientmusic.h's CDMBand (0x10 bytes - its bank strides by 0x10).
class CDMBand
{
public:
	CDMBand* m_pNext;                        // 0x000
	CDMBand* m_pPrev;                        // 0x004
	IDirectMusicBand* m_pDMBand;             // 0x008
	IDirectMusicSegmentState* m_pDMBandSegment; // 0x00c
};

// clientmusic.h's CDMUnknown60 - 12 bytes, and the one the DLSBANK reader
// makes (0x00441520): its grow is inlined there and strides by 0x0c.
class CDMUnknown60
{
public:
	CDMUnknown60* m_pNext;                   // 0x000
	CDMUnknown60* m_pPrev;                   // 0x004
	IDirectMusicSegmentState* m_pDMObject;   // 0x008
};

class CDMSegmentDefList
{
public:
	CDMSegment* m_pHead;                     // 0x000
	CDMSegment* m_pTail;                     // 0x004

	CDMSegment* FindByName(const char* pName);                      // 0x00441fe0
	void AddHead(CDMSegment* pNode);                                // 0x0048de30
};

class CDMStyleDefListQ440
{
public:
	CDMStyleDefQ440* m_pHead;
	CDMStyleDefQ440* m_pTail;

	// 0x00441fe0 - the MOTIF reader calls it on 0x58 (0x00441c5b); the one
	// body serves both lists.
	CDMStyleDefQ440* FindByName(const char* pName);
	void AddHead(CDMStyleDefQ440* pNode);                           // 0x0048de30
};

class CDMBandList
{
public:
	CDMBand* m_pHead;
	CDMBand* m_pTail;

	void AddHead(CDMBand* pNode);                                   // 0x0048de30
};

class CDMUnknown60List
{
public:
	CDMUnknown60* m_pHead;
	CDMUnknown60* m_pTail;

	void AddHead(CDMUnknown60* pNode);                              // 0x0048de30
};

// The four banks the file reaches, with the record type each one grows.
// Addresses from clientmusic.h.
// 0x004b3fc8, the pending-node bank -- clientmusic.h names it; the first of
// the five Term empties.
// The pending and playing banks grow through routines of their own
// (0x00442930, 0x00442aa0, both in dmgrow.cpp), so they get their own
// classes as the band bank does below; only Init grows them here.
class CDMPendingBankQ440 : public CDMBank
{
public:
	int Grow();                                                     // 0x00442930

	void InitQ440(uint32 nGrowCount, uint32 nPreGrow)
	{
		m_nGrowCount = nGrowCount;

		for (uint32 i = nPreGrow; i; i--)
		{
			if (!Grow())
				return;
		}

		m_bInitialized = 1;
	}
};

class CDMPlayingBankQ440 : public CDMBank
{
public:
	int Grow();                                                     // 0x00442aa0

	void InitQ440(uint32 nGrowCount, uint32 nPreGrow)
	{
		m_nGrowCount = nGrowCount;

		for (uint32 i = nPreGrow; i; i--)
		{
			if (!Grow())
				return;
		}

		m_bInitialized = 1;
	}
};

// The style records' bank grows by 0x50 (0x00442bd0, dmgrow.cpp); the style
// reader allocates from it with the out-of-line pop after the grow.
class CDMRec50BankQ440 : public CDMBank
{
public:
	int Grow();                                                     // 0x00442bd0

	void* AllocOutOfLineQ440()
	{
		if (m_pFree)
		{
			void* pRecord = m_pFree;
			m_pFree = *(void**)m_pFree;
			return pRecord;
		}

		if (!Grow())
			return LTNULL;

		return PopOutOfLineQ440();
	}
};

extern CDMPendingBankQ440 g_PendingBank;     // 0x004b3fc8  free list 0x004b3fd8
extern CDMBank g_SegmentBank;                // 0x004b3f50  free list 0x004b3f60
extern CDMRec50BankQ440 g_Unknown58Bank;     // 0x004b3f68  free list 0x004b3f78
extern CDMBank g_Unknown60Bank;              // 0x004b3f80  free list 0x004b3f90
// The playing-node bank (grow 0x00442aa0); only Init reaches it here.
extern CDMPlayingBankQ440 g_PlayingBank;     // 0x004b3f98  free list 0x004b3fa8
// The band bank's grow is 0x00442b50, defined at the bottom of this file, so
// it gets its own class (name ours).
class CDMBandBankQ440 : public CDMBank
{
public:
	int Grow();                                                     // 0x00442b50

	// CDMBank's Alloc over this class's own Grow: the inherited one would
	// call the segment bank's grow (0x004429d0), and LoadBand's call at
	// 0x0044182f is to 0x00442b50.
	void* Alloc()
	{
		if (m_pFree)
		{
			void* pRecord = m_pFree;
			m_pFree = *(void**)m_pFree;
			return pRecord;
		}

		if (!Grow())
			return LTNULL;

		return Pop();
	}
};

extern CDMBandBankQ440 g_BandBank;           // 0x004b3fb0  free list 0x004b3fc0

// The two pooled record types Term array-deletes, which is what makes VC6
// emit the vector deleting destructors at 0x00413080 (0x54 stride) and
// 0x00413110 (0x50 stride).  clientshell.cpp carries the same two classes and
// two stand-in `delete[]` drivers because the real sites -- the ones below --
// were not in that run.  Its free-list heads g_pFree54 (0x004b3f60) and
// g_pFree50 (0x004b3f78) are this file's g_SegmentBank.m_pFree and
// g_Unknown58Bank.m_pFree, so the pooled `operator delete` is spelled through
// the bank here rather than through a bare extern.
class CPooled54Q440
{
public:
	// Guarded: the shipped ??_E (0x00413080 / 0x00413110) tests the
	// pointer before 0x004290c0 in both arms.
	~CPooled54Q440()
	{
		if (m_pOwned4c)
			::operator delete(m_pOwned4c);
	}
	void operator delete(void* p)
	{
		if (p)
		{
			*(void**)p = g_SegmentBank.m_pFree;
			g_SegmentBank.m_pFree = p;
		}
	}

	uint8  m_pad_00[0x4c];
	uint8* m_pOwned4c;                       // 0x4c
	uint8  m_pad_50[0x54 - 0x50];
};

class CPooled50Q440
{
public:
	// Guarded: the shipped ??_E (0x00413080 / 0x00413110) tests the
	// pointer before 0x004290c0 in both arms.
	~CPooled50Q440()
	{
		if (m_pOwned4c)
			::operator delete(m_pOwned4c);
	}
	void operator delete(void* p)
	{
		if (p)
		{
			*(void**)p = g_Unknown58Bank.m_pFree;
			g_Unknown58Bank.m_pFree = p;
		}
	}

	uint8  m_pad_00[0x4c];
	uint8* m_pOwned4c;                       // 0x4c
};

// Term's two `delete[]` sites CALL ??_E (0x0043fba4, 0x0043fc54, `push 3 ;
// call`).  clientshell.cpp measured that VC6 expands a ??_E wherever it
// can, and only a `delete[]` eight inline levels down becomes the call.
// The shipped source presumably reached that depth through its bank
// templates; this chain is only the depth (STAND-IN, names ours).
template<class T> inline void dmDel1Q440(T* p) { delete[] p; }
template<class T> inline void dmDel2Q440(T* p) { dmDel1Q440(p); }
template<class T> inline void dmDel3Q440(T* p) { dmDel2Q440(p); }
template<class T> inline void dmDel4Q440(T* p) { dmDel3Q440(p); }
template<class T> inline void dmDel5Q440(T* p) { dmDel4Q440(p); }
template<class T> inline void dmDel6Q440(T* p) { dmDel5Q440(p); }
template<class T> inline void dmDel7Q440(T* p) { dmDel6Q440(p); }
template<class T> inline void dmDel8Q440(T* p) { dmDel7Q440(p); }

// clientmusic.h's tracing.
void dm_Printf(int nLevel, char* pFormat, ...);                     // 0x0043eef0
void dm_Warning(char* pFormat, ...);                                // 0x0043ee50

// lithshared's control-file reader (clientmusic.h).
class CControlFileWord
{
public:
	CControlFileWord* m_pNext;               // 0x000
	CControlFileWord* m_pPrev;               // 0x004
	char* m_sVal;                            // 0x008
	int   m_nStrLen;                         // 0x00c

	char* GetVal() { return m_sVal; }
	LTBOOL GetVal(int& nVal);                                       // 0x00491a50
};

class CControlFileKey
{
public:
	CControlFileWord* GetFirstWord();                               // 0x00491a90
	// 0x00491aa0, the sibling of GetKey's walk - LoadStyle steps the BAND
	// keys with it (name ours, by position).
	CControlFileKey* GetNextKey();
};

class CControlFileMgr
{
public:
	CControlFileKey* GetKey(const char* sSectionName, const char* sKeyName); // 0x00491e70
};

// dmloader.cpp's loader, only as deep as this file reaches.  m_pLoader is
// typed as this and not as an interface pointer because 0x00441171 destroys
// it with the non-virtual ~CDMLoader (0x004439b0) and the engine's operator
// delete, while 0x00440f73 lets go of it through the COM Release slot.
struct IDirectMusicLoader
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;
	virtual long __stdcall GetObject(DMUS_OBJECTDESC* pDesc, const DMGUID& riid,
	                                 void** ppv)=0;                 // 0x0c
};

class CDMSearchDir
{
public:
	CDMSearchDir* m_pNext;                   // 0x000
	CDMSearchDir* m_pPrev;                   // 0x004
	DMGUID        m_guidClass;               // 0x008
	uint16        m_wzPath[0x104];           // 0x018
};

class CDMSearchDirList
{
public:
	CDMSearchDir*      m_pHead;
	CDMSearchDir*      m_pTail;
	DMCRITICAL_SECTION m_CriticalSection;

	CDMSearchDir* Find(const DMGUID& rguidClass, int bCreate);      // 0x00442f20
};

class CDMStreamList;

class CDMLoader : public IDirectMusicLoader
{
public:
	CDMLoader();                                                    // 0x00443900
	~CDMLoader();                                                   // 0x004439b0

	long Init();                                                    // 0x00443a50

	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv);
	virtual uint32 __stdcall AddRef();
	virtual uint32 __stdcall Release();
	virtual long __stdcall GetObject(DMUS_OBJECTDESC* pDesc, const DMGUID& riid,
	                                 void** ppv);                   // 0x00443b90

	// The two halves of GetObject, both defined below (names ours).
	long GetObjectFromFileQ440(DMUS_OBJECTDESC* pDesc,
	                           IDirectMusicObject** ppObject);      // 0x00443d20
	long GetObjectFromMemoryQ440(DMUS_OBJECTDESC* pDesc,
	                             IDirectMusicObject** ppObject);    // 0x00443e90

public:
	long   m_nRefCount;                      // 0x004
	long   m_nStreamRefCount;                // 0x008
	DMCRITICAL_SECTION m_CriticalSection;    // 0x00c
	void*  m_pCachedObjects;                 // 0x024
	CDMSearchDirList* m_pSearchDirs;         // 0x028
	CDMStreamList*    m_pStreams;            // 0x02c
	uint16 m_wzFwdSlash[2];                  // 0x030
	uint16 m_wzBackSlash[2];                 // 0x034
};

// dmloader.cpp's cached-open-file record and its manager, plus the one call
// 0x004432b0 makes on each that dmloader.cpp had not needed.
//
// Layouts from src/engine/lithtech/demodtx.cpp's CLockedItemQ42C /
// CLockedOwnerQ42C, the same two classes: the record is 0x24 bytes (the
// `push $0x24` at 0x0042d579) and the manager IS its critical section, with
// the loaded list at 0x18, the unloaded one at 0x24 and the limit at 0x30.
class CDMFileMgr;
class CDMFileRecord;

// src/LT2/sdk/inc's CClientFileRef shape (demodtx.cpp's CClientFileRefQ42C).
// Its constructor is the first three stores at 0x0042d589.
class CDMFileRefQ440
{
public:
	CDMFileRefQ440() { m_nType = 0; m_pFilename = LTNULL; m_nID = 0; }

	uint32 m_nType;                          // 0x000
	char*  m_pFilename;                      // 0x004
	uint16 m_nID;                            // 0x008
};

class CDMFileRecordListQ440
{
public:
	void Insert(CDMFileRecord* pRecord);                            // 0x0048de30
	void Delete(CDMFileRecord* pRecord);                            // 0x0048dee0

	void Add(CDMFileRecord* pRecord)    { m_nCount++; Insert(pRecord); }
	void Remove(CDMFileRecord* pRecord) { m_nCount--; Delete(pRecord); }

public:
	CDMFileRecord* m_pHead;                  // 0x000
	CDMFileRecord* m_pTail;                  // 0x004
	uint32         m_nCount;                 // 0x008
};

class CDMFileRecord
{
public:
	CDMFileRecord(CDMFileMgr* pOwner)
	{
		m_pStream = LTNULL;
		m_nRefs   = 0;
		m_Ref.m_pFilename = LTNULL;
		m_nOffset = 0;
		m_pOwner  = pOwner;
	}

	// 0x0042d210 (name ours, by address): takes the record's critical
	// section, refuses with 1 while the use count at 0x10 is above zero, and
	// otherwise re-points the record at the name it is given.  Open passes a
	// null, and treats any nonzero answer as failure.
	int Reopen_0042d210(const char* sName);

public:
	CDMFileRecord* m_pNext;                  // 0x000
	CDMFileRecord* m_pPrev;                  // 0x004
	void*          m_pStream;                // 0x008
	CDMFileMgr*    m_pOwner;                 // 0x00c
	uint32         m_nRefs;                  // 0x010
	CDMFileRefQ440 m_Ref;                    // 0x014
	uint32         m_nOffset;                // 0x020
};

class CDMFileMgr
{
public:
	// 0x0042d560 (name ours): make a record for a path and file it on the
	// unloaded list.  Answers null on failure.  Defined at the bottom.
	CDMFileRecord* GetRecord(const char* sFileName);
	void FreeRecord(CDMFileRecord* pRecord);                        // 0x0042d610

public:
	DMCRITICAL_SECTION    m_CriticalSection; // 0x000
	CDMFileRecordListQ440 m_Loaded;          // 0x018
	CDMFileRecordListQ440 m_Unloaded;        // 0x024
	uint32                m_nLimit;          // 0x030
};

extern CDMFileMgr g_DMFileMgr;                                      // 0x004b3fe8

// dmloader.cpp's two streams.  Only what this file touches is declared.
class CDMFileStream
{
public:
	CDMFileStream(CDMLoader* pLoader);                              // 0x00443220

	long Close();                                                   // 0x00443380
	// 0x004432b0, defined below.  The shipped trace string
	// "CLTDMFileStream::Open FileName=%s" names the class CLTDMFileStream -
	// dmloader.cpp's CDMFileStream is a guess, and this literal corrects it.
	// The second argument is never read; 0x80000000 is what the one call site
	// passes (0x00443e17), which is GENERIC_READ.
	long Open(uint16* pwzFileName, uint32 dwDesiredAccess);

	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv);
	virtual uint32 __stdcall AddRef();
	virtual uint32 __stdcall Release();

public:
	// 0x00 is this vtable; 0x04 is the IDirectMusicGetLoader one that
	// dmloader.cpp found.  It is a bare word here because only the size has
	// to come out right: `new CDMFileStream` at 0x00443d5e asks for 0x14.
	void*  m_pGetLoaderVtblQ440;             // 0x004
	long   m_nRefCount;                      // 0x008
	CDMFileRecord* m_pFile;                  // 0x00c
	CDMLoader*     m_pLoader;                // 0x010
};

class CDMMemStream
{
public:
	CDMMemStream(CDMLoader* pLoader);                               // 0x004435f0

	long SetBuffer(void* pBuffer, uint32 cbLow, int32 cbHigh);      // 0x00443670

	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv);
	virtual uint32 __stdcall AddRef();
	virtual uint32 __stdcall Release();

public:
	// `new CDMMemStream` at 0x00443eb9 asks for 0x28 bytes, four more than
	// dmloader.cpp's layout accounts for (it runs out at 0x24).  The extra
	// word is carried here rather than guessed at; what it is is unsettled.
	uint8 m_padQ440[0x28 - 0x04];
};

// (new here) the wide string the three name checks compare against, a four-
// byte uninitialised global at 0x004b3fe4 that nothing in the image ever
// writes - so it is two zero wide characters, i.e. L"".
static uint16 s_wzEmptyQ440[2];

// The wide-string CRT.  Declared here rather than taken from wchar.h so that
// the character type is uint16 throughout - the tree's ltbasedefs has no
// wchar_t typedef and none of these is ever inlined, so a plain cdecl call
// is all the bytes need.  Addresses proved by disassembling each:
//   0x00494435 compares two wide strings and answers -1/0/1   = wcscmp
//   0x00494494 copies until the terminator                    = wcscpy
//   0x0049446a walks to the terminator first, then copies     = wcscat
//   0x00494731 counts to the terminator, /2, -1               = wcslen
//   0x004944f6 takes the CRT locale lock round a three-argument
//              conversion                                      = wcstombs
extern "C" {
int          __cdecl wcscmp(const uint16* s1, const uint16* s2);
uint16*      __cdecl wcscpy(uint16* d, const uint16* s);
uint16*      __cdecl wcscat(uint16* d, const uint16* s);
unsigned int __cdecl wcslen(const uint16* s);
unsigned int __cdecl wcstombs(char* d, const uint16* s, unsigned int n);
// 0x00492f30, the CRT's case-insensitive compare.
int          __cdecl _stricmp(const char* s1, const char* s2);

__declspec(dllimport) int __stdcall MultiByteToWideChar(uint32 CodePage, uint32 dwFlags,
                                                        const char* lpMultiByteStr,
                                                        int cbMultiByte,
                                                        uint16* lpWideCharStr,
                                                        int cchWideChar);
__declspec(dllimport) long __stdcall CoCreateInstance(const DMGUID& rclsid, void* pUnkOuter,
                                                      uint32 dwClsContext, const DMGUID& riid,
                                                      void** ppv);
// The four Term reaches, each through the import table: 0x004a104c,
// 0x004a1048, 0x004a1078 and 0x004a142c in order of first use.
__declspec(dllimport) void __stdcall EnterCriticalSection(void* pSection);
__declspec(dllimport) void __stdcall LeaveCriticalSection(void* pSection);
__declspec(dllimport) void __stdcall DeleteCriticalSection(void* pSection);
__declspec(dllimport) uint32 __stdcall WaitForSingleObject(void* hHandle,
                                                           uint32 dwMilliseconds);
__declspec(dllimport) void __stdcall CoUninitialize(void);
// Init's (0x004a1430, 0x004a105c, 0x004a10fc; _beginthread 0x004942ec).
__declspec(dllimport) long __stdcall CoInitialize(void* pvReserved);
__declspec(dllimport) void __stdcall InitializeCriticalSection(void* pSection);
__declspec(dllimport) void* __stdcall CreateEventA(void* lpEventAttributes, int bManualReset,
                                                  int bInitialState, const char* lpName);
unsigned long __cdecl _beginthread(void (__cdecl* pStartAddress)(void*),
                                   unsigned int nStackSize, void* pArgList);
}

// 0x0043f520, clientmusic.cpp: the notification thread Init starts.
void __cdecl dm_NotificationThread(void* pParam);
// 0x004a39bc.
extern const DMGUID GUID_NOTIFICATION_SEGMENT;
// 0x0043edb0, clientmusic.h's "ERROR!" printer.
void dm_Error(char* pFormat, ...);

#define DM_CP_ACP           0
#define DM_MB_PRECOMPOSED   1
// 3 at 0x00440f0e (CLSCTX_INPROC), 1 at 0x00443d3e / 0x00443ea1.
#define DM_CLSCTX_INPROC          3
#define DM_CLSCTX_INPROC_SERVER   1
#define DM_MAX_PATH         0x104

// ole2's SUCCEEDED/FAILED: a signed test on the HRESULT, which is the
// `test %eax,%eax ; jl` every call site here compiles to.
#define SUCCEEDEDQ440(hr) ((hr) >= 0)
#define FAILEDQ440(hr)    ((hr) < 0)


// clientmusic.h's CDMSegmentList / CDMIntensity / CDMTransition, as far as
// the INTENSITY and TRANSITION readers reach them.
class CDMSegmentList
{
public:
	CDMSegment* m_pHead;                     // 0x000
	CDMSegment* m_pTail;                     // 0x004

	void AddTail(CDMSegment* pNode);                                // 0x0048de60
};

class CDMIntensity
{
public:
	int m_nUnknown00;                        // 0x000
	int m_nUnknown04;                        // 0x004
	CDMSegmentList m_Segments;               // 0x008
};

class CDMTransition
{
public:
	LTDMEnactTypes       m_nEnact;           // 0x000
	int                  m_bManual;          // 0x004
	IDirectMusicSegment* m_pDMSegment;       // 0x008
};

// The loader's search-directory class GUIDs SetWorkingDirectory picks from
// (new here; dmusici.h's names for the bytes at each address).
extern const DMGUID GUID_DirectMusicAllTypes;         // 0x004a39cc
extern const DMGUID CLSID_DirectMusicChordMap;        // 0x004a39dc

// The DirectMusic GUID pool, 0x004a392c..0x004a3a4b: eighteen GUIDs back to
// back in .rdata, after dinput.lib's c_dfDIJoystick (0x004a3914) and before
// the DirectInput GUIDs (0x004a3a4c..).  In the shipped link they are DX8
// dxguid.lib data; the toolchain's dxguid.lib predates DirectMusic, so they
// are defined here, once, for every lithtech TU that names them, in shipped
// order and with the shipped bytes.  The names are the ones our TUs
// reference; two of them carry older interfaces' bytes (noted).  GUID_NULL
// and IID_IPersistStream/IStream/IUnknown (0x004a3b30..) are uuid.lib's.
extern const DMGUID IID_IKsControl               = { 0x28f54685, 0x11d206fd, 0xa0007ab2, 0x963122c9 };  // 0x004a392c {28F54685-06FD-11D2-B27A-00A0C9223196}
extern const DMGUID IID_IDirectMusicPerformance8 = { 0x6fc2cae0, 0x11d2bc78, 0xaa00a6af, 0xb6d82400 };  // 0x004a393c {6FC2CAE0-BC78-11D2-AFA6-00AA0024D8B6} (dmusici.h: IID_IDirectMusicPerformance2)
extern const DMGUID IID_IDirectMusicStyle        = { 0xd2ac28bd, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a394c {D2AC28BD-B39B-11D1-8704-00600893B1BD}
extern const DMGUID IID_IDirectMusicSegment      = { 0xf96029a2, 0x11d24282, 0x60001787, 0xbdb19308 };  // 0x004a395c {F96029A2-4282-11D2-8717-00600893B1BD}
extern const DMGUID IID_IDirectMusicObject       = { 0xd2ac28b5, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a396c {D2AC28B5-B39B-11D1-8704-00600893B1BD}
extern const DMGUID IID_IDirectMusicGetLoader    = { 0x68a04844, 0x11d1d13d, 0xaa00a6af, 0xb6d82400 };  // 0x004a397c {68A04844-D13D-11D1-AFA6-00AA0024D8B6}
extern const DMGUID IID_IDirectMusicLoader8      = { 0x2ffaaca2, 0x11d25dca, 0xaa00a6af, 0xb6d82400 };  // 0x004a398c {2FFAACA2-5DCA-11D2-AFA6-00AA0024D8B6} (dmusici.h: IID_IDirectMusicLoader)
extern const DMGUID GUID_PerfMasterVolume        = { 0xd2ac28b1, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a399c {D2AC28B1-B39B-11D1-8704-00600893B1BD}
extern const DMGUID GUID_Download                = { 0xd2ac28a7, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a39ac {D2AC28A7-B39B-11D1-8704-00600893B1BD}
extern const DMGUID GUID_NOTIFICATION_SEGMENT    = { 0xd2ac2899, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a39bc {D2AC2899-B39B-11D1-8704-00600893B1BD}
extern const DMGUID GUID_DirectMusicAllTypes     = { 0xd2ac2893, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a39cc {D2AC2893-B39B-11D1-8704-00600893B1BD}
extern const DMGUID CLSID_DirectMusicChordMap    = { 0xd2ac288f, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a39dc {D2AC288F-B39B-11D1-8704-00600893B1BD}
extern const DMGUID CLSID_DirectMusicStyle       = { 0xd2ac288a, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a39ec {D2AC288A-B39B-11D1-8704-00600893B1BD}
extern const DMGUID CLSID_DirectMusicSegment     = { 0xd2ac2882, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a39fc {D2AC2882-B39B-11D1-8704-00600893B1BD}
extern const DMGUID CLSID_DirectMusicPerformance = { 0xd2ac2881, 0x11d1b39b, 0x60000487, 0xbdb19308 };  // 0x004a3a0c {D2AC2881-B39B-11D1-8704-00600893B1BD}
extern const DMGUID GUID_DMUS_PROP_Effects       = { 0xcda8d611, 0x11d2684a, 0x60001e87, 0xbdb19308 };  // 0x004a3a1c {CDA8D611-684A-11D2-871E-00600893B1BD}
extern const DMGUID GUID_DMUS_PROP_WavesReverb   = { 0x04cb5622, 0x11d232e5, 0xaa00a6af, 0xb6d82400 };  // 0x004a3a2c {04CB5622-32E5-11D2-AFA6-00AA0024D8B6}
extern const DMGUID CLSID_DirectMusicCollection  = { 0x480ff4b0, 0x11d128b2, 0xc000f7be, 0xef8fbf4f };  // 0x004a3a3c {480FF4B0-28B2-11D1-BEF7-00C04FBF8FEF}

// dmloader.cpp's 0x00443ff0 (clientmusic.h's declaration).
struct IDirectMusicLoader8 : public IDirectMusicLoader
{
};

long __stdcall dm_SetSearchDirectory(IDirectMusicLoader8* pLoader, const DMGUID& rguidClass,
                                     const char* sPath, int bClear);


// --------------------------------------------------------------------- //
// CLTDirectMusicMgr, exactly clientmusic.h's layout.  Only the members and
// methods this file touches are spelled out; the rest keeps its offsets.
// --------------------------------------------------------------------- //

class CLTDirectMusicMgr : public ILTDirectMusicMgr
{
public:
	// The two enact-type converters, the only interface slots defined here:
	// vtable 0x004a24fc slot 60 is 0x00442790 and slot 61 is 0x004426e0,
	// which is where iltdirectmusic.h's declaration order puts them.
	// vtable 0x004a2500 slot 1; TermLevel is slot 3, which is how Term
	// reaches it (0x0043faf7 `call [eax+0xc]`).
	virtual LTRESULT Term();                                        // 0x0043fac0
	virtual LTRESULT TermLevel();                                   // 0x00440230

	virtual LTDMEnactTypes StringToEnactType(const char* sName);    // 0x00442790
	virtual void EnactTypeToString(LTDMEnactTypes nType, char* sName); // 0x004426e0

	virtual LTRESULT Init();                                        // 0x0043f920

	// clientmusic.h.
	int  CreatePort();                                              // 0x00440ff0

	// 0x004410d0 (name ours): Activate(FALSE) the port, RemovePort it from
	// the performance and let it go.  Answers 1 unconditionally.
	int  ReleasePortQ440();

	// clientmusic.h's readers, the three defined here.  clientmusic.h has
	// them void; each shipped body ends `mov $1,%eax ; ret $4`.
	LTBOOL ReadDLSBanks(CControlFileMgr* pControlFile);             // 0x00441960
	LTBOOL ReadSecondarySegments(CControlFileMgr* pControlFile);    // 0x00441b90
	LTBOOL ReadMotifs(CControlFileMgr* pControlFile);               // 0x00441c10
	LTBOOL ReadIntensities(CControlFileMgr* pControlFile);          // 0x00441a00
	LTBOOL ReadTransitions(CControlFileMgr* pControlFile);          // 0x00441d70

	// clientmusic.h.
	void SetWorkingDirectory(const char* sWorkingDirectory, int nDirType); // 0x00441190
	CDMTransition* GetTransition(int nFrom, int nTo);               // 0x00442130

	// The five helpers this file defines (names ours).
	LTBOOL InitDirectMusicQ440();                                   // 0x00440f00
	void   TermDirectMusicQ440();                                   // 0x00441110
	LTBOOL SetReverbParamsQ440(DMUS_WAVES_REVERB_PARAMS params);    // 0x00442340
	LTBOOL EnableReverbQ440();                                      // 0x004423e0
	LTBOOL DisableReverbQ440();                                     // 0x00442490

	// The four loaders the control-file readers call (names ours, from the
	// trace string each one prints).
	LTBOOL LoadSegmentQ440(const char* sSegmentName);               // 0x004413a0
	LTBOOL LoadDLSBankQ440(const char* sDLSBankName);               // 0x00441520
	LTBOOL LoadStyleQ440(const char* sStyleName,
	                     CControlFileMgr* pControlFile);            // 0x00441610
	LTBOOL LoadBandQ440(IDirectMusicStyle* pStyle,
	                    const char* sBandName);                     // 0x004417f0

public:
	LTBOOL m_bInitialized;                   // 0x004
	LTBOOL m_bLevelInitialized;              // 0x008
	IDirectMusic8*            m_pDirectMusic;// 0x00c
	CDMLoader*                m_pLoader;     // 0x010
	IDirectMusicPerformance8* m_pPerformance;// 0x014
	IDirectMusicPort*         m_pPort;       // 0x018
	DMUS_PORTPARAMS8          m_PortParams;  // 0x01c
	CDMSegmentDefList m_SecondaryDefs;       // 0x040
	CDMSegmentDefList m_MotifDefs;           // 0x048
	CDMBandList       m_BandList;            // 0x050
	CDMStyleDefListQ440 m_StyleDefs;         // 0x058  (clientmusic.h: m_Unknown58List)
	CDMUnknown60List  m_DLSBankList;         // 0x060  (clientmusic.h: m_Unknown60List)
	uint8  m_pad_068[0x84 - 0x68];           // 0x068
	// 0x0043fafb `lea ebp,[ebx+0x84]`, the argument to every
	// Enter/Leave/DeleteCriticalSection in Term.  clientmusic.h agrees.
	uint8  m_CriticalSection[0x9c - 0x84];   // 0x084
	int    m_nCurIntensity;                  // 0x09c
	int    m_nNumIntensities;                // 0x0a0
	int    m_nInitialIntensity;              // 0x0a4
	int    m_nInitialVolume;                 // 0x0a8
	int    m_nNumPChannels;                  // 0x0ac
	int    m_nNumVoices;                     // 0x0b0
	int    m_nSynthSampleRate;               // 0x0b4
	// clientmusic.h's level arrays and the two directory strings.
	CDMIntensity*  m_aIntensities;           // 0x0b8
	int            m_nNumTransitions;        // 0x0bc
	CDMTransition* m_aTransitions;           // 0x0c0
	char*  m_pUnknownC4;                     // 0x0c4
	char*  m_pUnknownC8;                     // 0x0c8
	// Init copies GUID_NOTIFICATION_SEGMENT here and registers it, then
	// hands the performance a fresh event (0x0043f9a7, 0x0043f9e9).
	DMGUID m_guidNotifyTypeQ440;             // 0x0cc
	void*  m_hNotifyEventQ440;               // 0x0dc
	// Past 0xcc, which is where clientmusic.h's declaration stops -- these
	// two are new here and named from what Init and Term do with them.
	// 0x0043f9fd clears the flag, 0x0043fb0a sets it under the lock, and
	// 0x0043faaa stores the thread starter's answer (names ours).
	int    m_bQuitQ440;                      // 0x0e0
	void*  m_hThreadQ440;                    // 0x0e4
	// ReadReverbParameters' REVERB key: cleared at 0x004425d9, set to 1 at
	// 0x0044261f when the key says ON, tested at 0x00442693 (name ours).
	LTBOOL m_bReverbOnQ440;                  // 0x0e8

	// 0x004426c0: clientmusic.h's Unknown4426c0 (TermLevel calls it at
	// clientmusic.cpp's TermLevel); the name is kept so the two views agree.
	int    Unknown4426c0();
};


// *********************************************************************** //
//  Bringing DirectMusic up and down
// *********************************************************************** //

// 0x00440f00 (name ours): everything CLTDirectMusicMgr::Init (0x0043f920)
// needs DirectMusic to have done before the manager calls itself
// initialised.  Init reports "ERROR! Unable to initialize DirectMusic." when
// this answers false, which is what says the answer is an LTBOOL and not an
// HRESULT.
//
// Every failure lands on the same tail - close the performance down, let it
// go and null it - but the source is flat early exits, EACH with its own
// full copy of that cleanup; VC6 tail-merges the copies into the guarded
// one (0x00440fb6) and the CoCreateInstance refusal's `m_pPerformance =
// NULL` into its last store (0x00440fcd).  The nested-SUCCEEDED form with
// one shared tail was 250 bytes / 15 blocks; this is identical.
LTBOOL CLTDirectMusicMgr::InitDirectMusicQ440()
{
	if (FAILEDQ440(CoCreateInstance(CLSID_DirectMusicPerformance, LTNULL,
	                                DM_CLSCTX_INPROC, IID_IDirectMusicPerformance8,
	                                (void**)&m_pPerformance)))
	{
		m_pPerformance = LTNULL;
		return LTFALSE;
	}

	m_pLoader = new CDMLoader;

	if (!m_pLoader)
	{
		m_pLoader = LTNULL;
		m_pPerformance->CloseDown();
		m_pPerformance->Release();
		m_pPerformance = LTNULL;
		return LTFALSE;
	}

	m_pLoader->Init();

	m_pDirectMusic = LTNULL;

	if (FAILEDQ440(m_pPerformance->Init(&m_pDirectMusic, LTNULL, LTNULL)))
	{
		m_pLoader->Release();
		m_pLoader = LTNULL;
		m_pPerformance->CloseDown();
		m_pPerformance->Release();
		m_pPerformance = LTNULL;
		return LTFALSE;
	}

	if (!CreatePort())
	{
		if (m_pDirectMusic)
		{
			m_pDirectMusic->SetDirectSound(LTNULL, LTNULL);
			m_pDirectMusic = LTNULL;
		}

		if (m_pLoader)
		{
			m_pLoader->Release();
			m_pLoader = LTNULL;
		}

		if (m_pPerformance)
		{
			m_pPerformance->CloseDown();
			m_pPerformance->Release();
			m_pPerformance = LTNULL;
		}

		return LTFALSE;
	}

	return LTTRUE;
}

// 0x0043f920 — CLTDirectMusicMgr::Init(), vtable 0x004a2500 slot 0; its
// strings name it.  Each refusal answers LT_ERROR (`mov $1,%eax`), and the
// DirectMusic failure undoes the CoInitialize first.  Then the lock, the
// segment-notification plumbing the thread drains, the six banks, and the
// thread itself.
LTRESULT CLTDirectMusicMgr::Init()
{
	dm_Printf(3, "CLTDirectMusicMgr::Init\n");

	if (m_bInitialized)
	{
		dm_Error("ERROR! CLTDirectMusicMgr alread initialized. (CLTDirectMusicMgr::Init)\n");
		return LT_ERROR;
	}

	if (FAILEDQ440(CoInitialize(LTNULL)))
	{
		dm_Error("ERROR! Unable to initialize com. (CLTDirectMusicMgr::Init)\n");
		return LT_ERROR;
	}

	if (!InitDirectMusicQ440())
	{
		dm_Error("ERROR! Unable to initialize DirectMusic. (CLTDirectMusicMgr::Init)\n");
		CoUninitialize();
		return LT_ERROR;
	}

	InitializeCriticalSection(m_CriticalSection);

	m_guidNotifyTypeQ440 = GUID_NOTIFICATION_SEGMENT;
	m_pPerformance->AddNotificationType(m_guidNotifyTypeQ440);

	m_hNotifyEventQ440 = CreateEventA(LTNULL, 0, 0, LTNULL);
	m_pPerformance->SetNotificationHandle(m_hNotifyEventQ440, 0);

	m_bQuitQ440 = 0;

	g_PendingBank.InitQ440(100, 1);
	g_SegmentBank.InitQ440(20, 1);
	g_PlayingBank.InitQ440(10, 1);
	g_BandBank.InitQ440(5, 0);
	g_Unknown58Bank.InitQ440(5, 0);
	g_Unknown60Bank.InitQ440(5, 0);

	m_hThreadQ440 = (void*)_beginthread(dm_NotificationThread, 0, LTNULL);

	m_bInitialized = LTTRUE;

	return LT_OK;
}

// 0x0043fac0 — CLTDirectMusicMgr::Term(), vtable 0x004a2500 slot 1. It names
// itself: 0x0043fac2 pushes "CLTDirectMusicMgr::Term\n" into dm_Printf and
// the not-initialised complaint ends "(CLTDirectMusicMgr::Term)".
//
// Stop the worker thread, drop DirectMusic, then empty all five record banks
// and take the lock down with them.
//
// The two `delete[]`s below are the ones clientshell.cpp has been faking.
// That file's note says of the ??_E COMDATs at 0x00413080 and 0x00413110:
// "the shipped ones are in the registry teardown at 0x0043fb80, which is not
// in this run" -- 0x0043fb80 is INSIDE this function, so these are them. Its
// stand-in drivers pool_FreeArray54/pool_FreeArray50 can go once this object
// is credited with the COMDATs; left in place for now so clientshell.cpp does
// not lose them in the same commit that grows them here.
//
// That file's two `extern void*` free-list heads turn out to be fields we
// already have: g_pFree54 at 0x004b3f60 is g_SegmentBank(0x004b3f50)+0x10 and
// g_pFree50 at 0x004b3f78 is g_Unknown58Bank(0x004b3f68)+0x10, so the pooled
// `operator delete` is just the bank's own free list, spelled through the
// bank.
//
// Read off the bytes:
//
//   * 0x0043fada `jne` over an early return that ends `xor %eax,%eax` --
//     LT_OK, not an error, when the manager was never up.
//   * 0x0043faf7 `call [eax+0xc]` is vtable slot 3, TermLevel, so the
//     level teardown goes through the interface and not the definition.
//   * 0x0043fafb `lea ebp,[ebx+0x84]` holds &m_CriticalSection across all
//     three Enter/Leave/Delete calls.
//   * 0x0043fb20 pushes 0x3e8 -- WaitForSingleObject with a 1000 ms wait.
//   * the five bank blocks are the same six statements each; only the
//     element type of the inner delete changes.
//
// THE COIN-FLIP: the shipped image CALLS CDMChunkListQ440::Remove at four of
// the five blocks and EXPANDS it at the fourth (0x0043fc1f..0x0043fc4a); that
// block calls the in-class twin RemoveInlineQ440.  The two ??_E sites are
// calls (`push 3 ; call`), which VC6 only emits with the `delete[]` deep in
// inline levels -- dmDel8Q440.  With both, IDENTICAL (526 bytes), and the
// two ??_E COMDATs this object now emits match 0x00413080 / 0x00413110.
LTRESULT CLTDirectMusicMgr::Term()
{
	CDMChunkQ440* pChunk;

	dm_Printf(3, "CLTDirectMusicMgr::Term\n");

	if (!m_bInitialized)
	{
		dm_Warning("WARNING! LTDirectMusicMgr already terminated or never initialized. (CLTDirectMusicMgr::Term)\n");
		return LT_OK;
	}

	if (m_bLevelInitialized)
		TermLevel();

	EnterCriticalSection(m_CriticalSection);
	m_bQuitQ440 = 1;
	LeaveCriticalSection(m_CriticalSection);

	WaitForSingleObject(m_hThreadQ440, 1000);

	TermDirectMusicQ440();
	CoUninitialize();

	// ---- the pending-node bank, 0x004b3fc8 ---------------------------- //
	if (g_PendingBank.m_bInitialized)
	{
		pChunk = g_PendingBank.m_Chunks.m_pHead;
		while (pChunk)
		{
			g_PendingBank.m_Chunks.Remove(pChunk);
			delete pChunk->m_pRecords;
			delete pChunk;
			pChunk = g_PendingBank.m_Chunks.m_pHead;
		}

		g_PendingBank.m_pFree = LTNULL;
		g_PendingBank.m_bInitialized = 0;
	}

	// ---- the segment bank, 0x004b3f50 --------------------------------- //
	if (g_SegmentBank.m_bInitialized)
	{
		pChunk = g_SegmentBank.m_Chunks.m_pHead;
		while (pChunk)
		{
			g_SegmentBank.m_Chunks.Remove(pChunk);
			dmDel8Q440((CPooled54Q440*)pChunk->m_pRecords);   // ??_E 0x00413080
			delete pChunk;
			pChunk = g_SegmentBank.m_Chunks.m_pHead;
		}

		g_SegmentBank.m_pFree = LTNULL;
		g_SegmentBank.m_bInitialized = 0;
	}

	// ---- the band bank, 0x004b3fb0 ------------------------------------ //
	if (g_BandBank.m_bInitialized)
	{
		pChunk = g_BandBank.m_Chunks.m_pHead;
		while (pChunk)
		{
			g_BandBank.m_Chunks.Remove(pChunk);
			delete pChunk->m_pRecords;
			delete pChunk;
			pChunk = g_BandBank.m_Chunks.m_pHead;
		}

		g_BandBank.m_pFree = LTNULL;
		g_BandBank.m_bInitialized = 0;
	}

	// ---- the style-def bank, 0x004b3f68 ------------------------------- //
	// This is the block whose Remove is EXPANDED in the shipped image.
	if (g_Unknown58Bank.m_bInitialized)
	{
		pChunk = g_Unknown58Bank.m_Chunks.m_pHead;
		while (pChunk)
		{
			g_Unknown58Bank.m_Chunks.RemoveInlineQ440(pChunk);
			dmDel8Q440((CPooled50Q440*)pChunk->m_pRecords);   // ??_E 0x00413110
			delete pChunk;
			pChunk = g_Unknown58Bank.m_Chunks.m_pHead;
		}

		g_Unknown58Bank.m_pFree = LTNULL;
		g_Unknown58Bank.m_bInitialized = 0;
	}

	// ---- the DLS-bank bank, 0x004b3f80 -------------------------------- //
	if (g_Unknown60Bank.m_bInitialized)
	{
		pChunk = g_Unknown60Bank.m_Chunks.m_pHead;
		while (pChunk)
		{
			g_Unknown60Bank.m_Chunks.Remove(pChunk);
			delete pChunk->m_pRecords;
			delete pChunk;
			pChunk = g_Unknown60Bank.m_Chunks.m_pHead;
		}

		g_Unknown60Bank.m_pFree = LTNULL;
		g_Unknown60Bank.m_bInitialized = 0;
	}

	DeleteCriticalSection(m_CriticalSection);

	m_bInitialized = LTFALSE;

	return LT_OK;
}


// 0x00440ff0: (re)make the port.  Let any old one go, fill the port
// parameters from the level's three numbers, then CreatePort, Activate it,
// AddPort it and assign one PChannel block per channel group.  The answer
// is the last COM result's sign (`test %edi ; setge` at 0x004410b8) - the
// block loop's own failures only stop the loop and are not remembered.
int CLTDirectMusicMgr::CreatePort()
{
	long hr;

	if (m_pPort)
		ReleasePortQ440();

	memset(&m_PortParams, 0, sizeof(m_PortParams));

	m_PortParams.dwSize          = sizeof(DMUS_PORTPARAMS8);
	m_PortParams.dwValidParams   = 0xf;
	m_PortParams.dwVoices        = m_nNumVoices;
	m_PortParams.dwChannelGroups = (m_nNumPChannels - 1) / 16 + 1;
	m_PortParams.dwAudioChannels = m_nNumPChannels;
	m_PortParams.dwSampleRate    = m_nSynthSampleRate;

	hr = m_pDirectMusic->CreatePort(GUID_NULL, &m_PortParams, &m_pPort, LTNULL);

	if (SUCCEEDEDQ440(hr))
	{
		hr = m_pPort->Activate(1);

		if (SUCCEEDEDQ440(hr))
		{
			hr = m_pPerformance->AddPort(m_pPort);

			if (SUCCEEDEDQ440(hr))
			{
				for (uint32 i = 0; i < m_PortParams.dwChannelGroups; i++)
				{
					if (FAILEDQ440(m_pPerformance->AssignPChannelBlock(i, m_pPort, i + 1)))
						break;
				}
			}
		}
	}

	return SUCCEEDEDQ440(hr);
}

// 0x004410d0 (name ours).
int CLTDirectMusicMgr::ReleasePortQ440()
{
	if (m_pPort)
	{
		m_pPort->Activate(0);

		if (m_pPerformance)
			m_pPerformance->RemovePort(m_pPort);

		m_pPort->Release();
		m_pPort = LTNULL;
	}

	return 1;
}

// 0x00441110 (name ours): the other half, called from Term (0x0043fb2e).
// Nothing is answered - the shipped body leaves %eax alone on every path.
// The loader is destroyed rather than released: 0x00441171 calls
// ~CDMLoader (0x004439b0) and then the engine's operator delete, which is
// what `delete` compiles to for a class whose destructor is not virtual.
void CLTDirectMusicMgr::TermDirectMusicQ440()
{
	if (m_pPerformance)
		m_pPerformance->Stop(LTNULL, LTNULL, 0, 0);

	ReleasePortQ440();

	if (m_pDirectMusic)
	{
		m_pDirectMusic->SetDirectSound(LTNULL, LTNULL);
		m_pDirectMusic = LTNULL;
	}

	if (m_pPerformance)
	{
		m_pPerformance->CloseDown();
		m_pPerformance->Release();
		m_pPerformance = LTNULL;
	}

	if (m_pLoader)
	{
		delete m_pLoader;
		m_pLoader = LTNULL;
	}
}


// 0x00441190: replace directory string 1 (0xc4) or 2 (0xc8) with a copy of
// the argument, then hand the directory to the loader under the class GUID
// the same number picks (jump table 0x0044137c, cases 1..6; 2 and anything
// else leave GUID_NULL, and a GUID_NULL class is never passed on - the
// `repz cmpsl` at 0x0044135c is the inline memcmp).
void CLTDirectMusicMgr::SetWorkingDirectory(const char* sWorkingDirectory, int nDirType)
{
	if (nDirType == 1)
	{
		if (m_pUnknownC4)
		{
			delete m_pUnknownC4;
			m_pUnknownC4 = LTNULL;
		}

		if (sWorkingDirectory)
		{
			m_pUnknownC4 = new char[strlen(sWorkingDirectory) + 1];
			strcpy(m_pUnknownC4, sWorkingDirectory);
		}
	}
	else if (nDirType == 2)
	{
		if (m_pUnknownC8)
		{
			delete m_pUnknownC8;
			m_pUnknownC8 = LTNULL;
		}

		if (sWorkingDirectory)
		{
			m_pUnknownC8 = new char[strlen(sWorkingDirectory) + 1];
			strcpy(m_pUnknownC8, sWorkingDirectory);
		}
	}

	DMGUID guidClass = GUID_NULL;

	switch (nDirType)
	{
		case 1: guidClass = GUID_DirectMusicAllTypes;    break;
		case 3: guidClass = CLSID_DirectMusicCollection; break;
		case 4: guidClass = CLSID_DirectMusicStyle;      break;
		case 5: guidClass = CLSID_DirectMusicSegment;    break;
		case 6: guidClass = CLSID_DirectMusicChordMap;   break;
	}

	if (memcmp(&guidClass, &GUID_NULL, sizeof(DMGUID)) != 0)
		dm_SetSearchDirectory((IDirectMusicLoader8*)m_pLoader, guidClass,
		                      sWorkingDirectory, LTFALSE);
}


// *********************************************************************** //
//  The port's reverb properties
//
//  All three go the same way: query the port for IKsControl, build a
//  KSPROPERTY, push it through KsProperty and let the control go.  They
//  answer LTBOOL - 1 at the end, 0 from either of the two guards - which is
//  why the `xor %eax,%eax` tails are shared.
// *********************************************************************** //

// 0x00442340 (name ours): hand the port the four reverb numbers.  The
// parameters come in BY VALUE - the `ret $0x10` and the fact that the data
// pointer KsProperty gets is the argument block itself at 0x004423a0 - and
// ReadReverbParameters (0x004425b0) is the one caller, copying its own
// 0x0ec..0x0fb into the call.
LTBOOL CLTDirectMusicMgr::SetReverbParamsQ440(DMUS_WAVES_REVERB_PARAMS params)
{
	IKsControl* pKsControl;
	uint32 nBytesReturned;
	KSPROPERTY prop;

	// POSITIVE GUARDS, both of them. 0x00442340's two tests are `je`/`jl`
	// FORWARD to a single `return LTFALSE` past the body; written negated
	// the early returns are hosted inline and the branches invert.
	if (m_pPort)
	{
		if (SUCCEEDEDQ440(m_pPort->QueryInterface(IID_IKsControl,
		                                          (void**)&pKsControl)))
		{
			memset(&prop, 0, sizeof(prop));

			prop.Set   = GUID_DMUS_PROP_WavesReverb;
			prop.Id    = 0;
			prop.Flags = KSPROPERTY_TYPE_SET;

			pKsControl->KsProperty(&prop, sizeof(prop), &params,
			                       sizeof(params), &nBytesReturned);

			pKsControl->Release();

			return LTTRUE;
		}
	}

	return LTFALSE;
}

// 0x004423e0 (name ours): turn the port's reverb on.  ReadReverbParameters
// calls it at 0x0044269f when the REVERB key said ON.
//
// IDENTICAL (172 bytes; the csv row was 451, both functions in one --
// 0x00442490 starts after this one's `ret` and 0x004426ca jumps to it).
// `nEffects = 0` is a statement inside the port test, not an initialiser:
// shipped loads m_pPort before the frame is even reserved and stores the
// zero only on the taken path (0x004423f2).
LTBOOL CLTDirectMusicMgr::EnableReverbQ440()
{
	uint32 nEffects;
	IKsControl* pKsControl;
	uint32 nBytesReturned;
	KSPROPERTY prop;

	// POSITIVE GUARDS. 0x004423e0/0x00442490 both test FORWARD with `je` to
	// one `return LTFALSE` past the body; written negated the early returns
	// are hosted inline and both branches invert.
	if (m_pPort)
	{
		nEffects = 0;

		if (SUCCEEDEDQ440(m_pPort->QueryInterface(IID_IKsControl,
		                                          (void**)&pKsControl)))
		{
		nEffects = DMUS_EFFECT_REVERB;

		memset(&prop, 0, sizeof(prop));

		prop.Set   = GUID_DMUS_PROP_Effects;
		prop.Id    = 0;
		prop.Flags = KSPROPERTY_TYPE_SET;

		pKsControl->KsProperty(&prop, sizeof(prop), &nEffects, sizeof(nEffects),
		                       &nBytesReturned);

		pKsControl->Release();

		return LTTRUE;
		}
	}

	return LTFALSE;
}

// 0x00442490 (name ours): turn it off again, by reading the effects word
// back and clearing the one bit rather than writing a bare zero.  0x004426c0
// tail-jumps here when the level had reverb on.
//
// IDENTICAL (275 bytes).  The zero as a statement, as above; and the bit is
// cleared BEFORE the GUID is copied back in, which is what puts the store
// of nEffects ahead of the GUID's second dword (0x00442568).
LTBOOL CLTDirectMusicMgr::DisableReverbQ440()
{
	uint32 nEffects;
	IKsControl* pKsControl;
	uint32 nBytesReturned;
	KSPROPERTY prop;

	// POSITIVE GUARDS. 0x004423e0/0x00442490 both test FORWARD with `je` to
	// one `return LTFALSE` past the body; written negated the early returns
	// are hosted inline and both branches invert.
	if (m_pPort)
	{
		nEffects = 0;

		if (SUCCEEDEDQ440(m_pPort->QueryInterface(IID_IKsControl,
		                                          (void**)&pKsControl)))
		{
		memset(&prop, 0, sizeof(prop));

		prop.Set   = GUID_DMUS_PROP_Effects;
		prop.Id    = 0;
		prop.Flags = KSPROPERTY_TYPE_GET;

		pKsControl->KsProperty(&prop, sizeof(prop), &nEffects, sizeof(nEffects),
		                       &nBytesReturned);

		memset(&prop, 0, sizeof(prop));

		nEffects &= ~DMUS_EFFECT_REVERB;

		prop.Set = GUID_DMUS_PROP_Effects;
		prop.Id    = 0;
		prop.Flags = KSPROPERTY_TYPE_SET;

		pKsControl->KsProperty(&prop, sizeof(prop), &nEffects, sizeof(nEffects),
		                       &nBytesReturned);

		pKsControl->Release();

		return LTTRUE;
		}
	}

	return LTFALSE;
}


// 0x004426c0, 21 bytes: turn reverb off again if the level turned it on,
// and answer 1 when there was nothing to do.  TermLevel's one call.  The
// `jmp 0x00442490` at 0x004426ca is VC6's tail call -- the result of
// DisableReverbQ440 is returned, not discarded.
int CLTDirectMusicMgr::Unknown4426c0()
{
	if (m_bReverbOnQ440)                     // 0x004426c0
		return DisableReverbQ440();          // 0x004426ca

	return 1;                                // 0x004426cf
}


// *********************************************************************** //
//  What the control-file readers load
// *********************************************************************** //

// 0x00441f20 (name ours): give a definition record its name, keeping it in
// the record's own 64-byte buffer unless it will not fit and then allocating
// for it.  The `cmp $0x3f` at 0x00441f5f is sizeof(m_szName) - 1, and the two
// `repnz scasb` scans at 0x00441f5a and 0x00441f84 are two written-out
// strlen() calls - VC6 does not common them up.
//
// One body serves both record types: the style records the STYLE reader makes
// (0x50 bytes) have the same 0x0c and 0x4c as a segment, so the two
// instantiations fold into one function.
LTBOOL CDMSegment::SetName(const char* sName)
{
	if (m_pName)
	{
		delete m_pName;
		m_pName = LTNULL;
	}

	if (!sName)
	{
		m_szName[0] = '\0';
		return LTTRUE;
	}

	// One strlen, not two: the three `repnz scasb` scans in the shipped body
	// are this one (0x00441f5a) and the length scans of the two inline
	// strcpy expansions (0x00441f84, 0x00441fb5).  The `cmp $0x3f ; jle` at
	// 0x00441f5f is SIGNED, which is what makes the length an int and not
	// the size_t strlen hands back, and the `inc %ecx` at 0x00441f64 is that
	// same int reused for the allocation.
	int nLength = strlen(sName);

	if (nLength > (int)(sizeof(m_szName) - 1))
	{
		m_pName = new char[nLength + 1];

		if (!m_pName)
			return LTFALSE;

		strcpy(m_pName, sName);
		m_szName[0] = '\0';

		return LTTRUE;
	}

	strcpy(m_szName, sName);

	return LTTRUE;
}

// 0x004413a0 (name ours, from the trace string at 0x004414c3): load one named
// segment into the level's definition list, unless it is already there.
// Called from the INTENSITY (0x00441aaf), SECONDARYSEGMENT (0x00441bbf) and
// TRANSITION (0x00441e6b) readers.
//
// The descriptor is a bare local - nothing zeroes it - and only the four
// fields DirectMusic is told about are written.  dwSize and guidClass go in
// before the widening call and dwValidData after it, which is not something
// VC6 could have moved: the widened name lands inside the descriptor, so no
// store to it can cross that call.
//
// Two spellings closed it: the recycled record's four stores guarded by
// their own `if (pSegment)` ahead of the refusal (the new-expression shape,
// which gives the `xor %eax,%eax ; cmp %eax,%esi` zero register the stores
// reuse at 0x0044140d), and SUCCEEDED with the success body first, which
// puts the failure block last and keeps the early `return TRUE` out of line.
LTBOOL CLTDirectMusicMgr::LoadSegmentQ440(const char* sSegmentName)
{
	DMUS_OBJECTDESC desc;
	IDirectMusicSegment* pDMSegment;
	CDMSegment* pSegment;

	if (m_SecondaryDefs.FindByName(sSegmentName))
		return LTTRUE;

	pSegment = (CDMSegment*)g_SegmentBank.Alloc();

	if (pSegment)
	{
		pSegment->m_pDMSegment = LTNULL;
		pSegment->m_szName[0]  = '\0';
		pSegment->m_pName      = LTNULL;
		pSegment->m_nStopEnact = LTDMEnactNextMeasure;
	}

	if (!pSegment)
		return LTFALSE;

	desc.guidClass = CLSID_DirectMusicSegment;
	desc.dwSize    = sizeof(DMUS_OBJECTDESC);

	MultiByteToWideChar(DM_CP_ACP, DM_MB_PRECOMPOSED, sSegmentName, -1,
	                    desc.wszFileName, DM_MAX_PATH);

	desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;

	if (SUCCEEDEDQ440(m_pLoader->GetObject(&desc, IID_IDirectMusicSegment,
	                                       (void**)&pDMSegment)))
	{
		// Slot 0x4c with GUID_Download, every channel group and no parameter -
		// see the interface comment above for why the offset and not a name.
		pDMSegment->SetParam(GUID_Download, 0xffffffff, 0, 0, LTNULL);

		pSegment->m_pDMSegment = pDMSegment;
		pSegment->SetName(sSegmentName);

		m_SecondaryDefs.AddHead(pSegment);

		dm_Printf(3, "LTDirectMusic loaded segment %s\n", sSegmentName);

		return LTTRUE;
	}

	dm_Warning("WARNING! LTDirectMusic failed to load segment %s\n", sSegmentName);

	if (pSegment->m_pName)
		delete pSegment->m_pName;

	// The bank Free with the null retest dropped: pSegment was proved
	// non-null a few instructions ago (0x00441502).
	*(void**)pSegment = g_SegmentBank.m_pFree;
	g_SegmentBank.m_pFree = pSegment;

	return LTFALSE;
}


// The DLS-bank records' bank.  It is spelled out rather than shared with
// CDMBank because its grow is INLINE at the one site that uses it
// (0x0044156b..0x004415cb) while every other bank's is out of line, and
// because its records carry no leading count word - the array allocation at
// 0x0044157b is exactly nCount * 0x0c (names ours).
class CDMUnknown60BankQ440
{
public:
	int    m_bInitialized;                   // 0x000
	uint32 m_nGrowCount;                     // 0x004
	CDMChunkListQ440 m_Chunks;               // 0x008
	void*  m_pFree;                          // 0x010

	int Grow()
	{
		CDMChunkQ440* pChunk = new CDMChunkQ440;

		if (!pChunk)
			return 0;

		CDMUnknown60* pRecords =
			(CDMUnknown60*)::operator new(m_nGrowCount * sizeof(CDMUnknown60));

		pChunk->m_pRecords = pRecords;

		if (!pRecords)
			return 0;

		for (uint32 i = 0; i < m_nGrowCount; i++)
		{
			*(void**)pRecords = m_pFree;
			m_pFree = pRecords;
			pRecords++;
		}

		m_Chunks.AddHead(pChunk);

		return 1;
	}

	void* Pop()
	{
		void* pRecord = m_pFree;

		if (pRecord)
			m_pFree = *(void**)pRecord;

		return pRecord;
	}

	void* Alloc()
	{
		if (m_pFree)
		{
			void* pRecord = m_pFree;
			m_pFree = *(void**)m_pFree;
			return pRecord;
		}

		if (!Grow())
			return LTNULL;

		return Pop();
	}
};

extern CDMUnknown60BankQ440 g_DLSBankBankQ440;   // 0x004b3f80

// 0x00441520 (name ours): the DLSBANK reader's per-line work.  It widens the
// name, and does nothing at all when the widened name is empty; otherwise it
// takes a record from the bank, leaves its object null and links it.  Nothing
// is loaded here - whatever fills the object at 0x08 does it later.
//
// The empty test is a wcscmp against the four-byte uninitialised global at
// 0x004b3fe4, which nothing in the image ever writes: two zero wide
// characters, i.e. L"".
LTBOOL CLTDirectMusicMgr::LoadDLSBankQ440(const char* sDLSBankName)
{
	uint16 wzName[DM_MAX_PATH];

	MultiByteToWideChar(DM_CP_ACP, DM_MB_PRECOMPOSED, sDLSBankName, -1,
	                    wzName, DM_MAX_PATH);

	if (wcscmp(wzName, s_wzEmptyQ440) != 0)
	{
		CDMUnknown60* pNode = (CDMUnknown60*)g_DLSBankBankQ440.Alloc();

		if (!pNode)
			return LTFALSE;

		pNode->m_pDMObject = LTNULL;

		m_DLSBankList.AddHead(pNode);
	}

	return LTTRUE;
}


// The style records' bank is g_Unknown58Bank (0x004b3f68, free list
// 0x004b3f78, grow 0x00442bd0 which strides by 0x50).

// 0x00441610 (name ours, from the trace string at 0x00441749): load one named
// style and then every BAND line that names it.
//
// Unlike the segment reader this one widens into a local first, because the
// wide name is wanted twice - once for the empty test and once as the
// descriptor's file name (wcscpy at 0x004416da).
//
// The allocation takes the bank's out-of-line pop after the grow
// (call 0x00442920 at 0x0044168b). Both failures, the bank being empty and
// GetObject failing, share one `return FALSE` exit (0x0044173a), which is
// what the nested `if (pStyleDef)` with a single return after it gives.
// The function is still a near, 464 against 465: pStyleDef and the zero/pKey
// register are swapped, ebp for edi, and the one byte is `mov %eax,0x0(%ebp)`
// against `mov %eax,(%edi)`. That is allocation. Moving pStyleDef's
// declaration (to the top, or to the Alloc) and declaring pKey up front
// changed nothing.
//
// The BAND walk reads the key's first word, compares it with this style's
// name and, when it matches, hands the SECOND word to LoadBand.  Both of the
// inner null tests are the shipped code's, not defensive additions: a key
// with no words and a key with only one are both simply skipped.
LTBOOL CLTDirectMusicMgr::LoadStyleQ440(const char* sStyleName,
                                        CControlFileMgr* pControlFile)
{
	DMUS_OBJECTDESC desc;
	IDirectMusicStyle* pDMStyle;
	CDMStyleDefQ440* pStyleDef;
	uint16 wzName[DM_MAX_PATH];

	MultiByteToWideChar(DM_CP_ACP, DM_MB_PRECOMPOSED, sStyleName, -1,
	                    wzName, DM_MAX_PATH);

	if (wcscmp(wzName, s_wzEmptyQ440) != 0)
	{
		pStyleDef = (CDMStyleDefQ440*)g_Unknown58Bank.AllocOutOfLineQ440();

		if (pStyleDef)
		{
			pStyleDef->m_pDMStyle   = LTNULL;
			pStyleDef->m_szName[0]  = '\0';
			pStyleDef->m_pName      = LTNULL;

			desc.guidClass = CLSID_DirectMusicStyle;

			wcscpy(desc.wszFileName, wzName);

			desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
			desc.dwSize      = sizeof(DMUS_OBJECTDESC);

			if (FAILEDQ440(m_pLoader->GetObject(&desc, IID_IDirectMusicStyle,
			                                    (void**)&pDMStyle)))
			{
				dm_Warning("WARNING! LTDirectMusic failed to style %s\n", sStyleName);

				if (pStyleDef->m_pName)
					delete pStyleDef->m_pName;

				*(void**)pStyleDef = g_Unknown58Bank.m_pFree;
				g_Unknown58Bank.m_pFree = pStyleDef;
			}
			else
			{
				dm_Printf(3, "LTDirectMusic loaded style %s\n", sStyleName);

				CControlFileKey* pKey = pControlFile->GetKey(LTNULL, "BAND");

				while (pKey)
				{
					CControlFileWord* pWord = pKey->GetFirstWord();

					if (pWord && _stricmp(pWord->GetVal(), sStyleName) == 0)
					{
						pWord = pWord->m_pNext;

						if (pWord)
							LoadBandQ440(pDMStyle, pWord->GetVal());
					}

					pKey = pKey->GetNextKey();
				}

				pStyleDef->m_pDMStyle = pDMStyle;
				((CDMSegment*)pStyleDef)->SetName(sStyleName);

				m_StyleDefs.AddHead(pStyleDef);

				return LTTRUE;
			}
		}

		return LTFALSE;
	}

	return LTTRUE;
}

// 0x004417f0 (name ours, from the trace string at 0x0044193a): pull one band
// out of a style, download it into the performance and start its segment.
//
// Two warnings, one for each of the two calls that can fail, and the trace
// line at the end is reached from both - the failure at 0x00441907 jumps
// straight to it.
//
// NEAR (362 against 363): pStyle and sBandName are swapped between ebx and
// ebp, and the byte is `mov 0x0(%ebp),%ecx` against `mov (%ebx),%ecx`.
// Allocation - moving pBand's or wzName's declaration, `if (!(pBand =
// ...))`, a `pDMBand = NULL` before GetBand and the bank's Free for the
// refusal changed nothing. Nor did a pStyle copy for GetBand, a
// sBandName copy for the conversion, a braced warning arm, or the two
// refusals folded into one positive guard (338 bytes).
LTBOOL CLTDirectMusicMgr::LoadBandQ440(IDirectMusicStyle* pStyle,
                                       const char* sBandName)
{
	IDirectMusicBand* pDMBand;
	IDirectMusicSegmentState* pDMBandSegment;
	uint16 wzName[DM_MAX_PATH];
	CDMBand* pBand;

	if (!pStyle)
		return LTFALSE;

	pBand = (CDMBand*)g_BandBank.Alloc();

	if (!pBand)
		return LTFALSE;

	MultiByteToWideChar(DM_CP_ACP, DM_MB_PRECOMPOSED, sBandName, -1,
	                    wzName, DM_MAX_PATH);

	if (wcscmp(wzName, s_wzEmptyQ440) != 0)
	{
		pStyle->GetBand(wzName, &pDMBand);

		if (!pDMBand)
		{
			*(void**)pBand = g_BandBank.m_pFree;
			g_BandBank.m_pFree = pBand;

			return LTFALSE;
		}

		if (FAILEDQ440(pDMBand->Download(m_pPerformance)))
			dm_Warning("WARNING! LTDirectMusic failed to load band %s\n", sBandName);

		pBand->m_pDMBand = pDMBand;

		m_BandList.AddHead(pBand);

		if (FAILEDQ440(pDMBand->CreateSegment(&pDMBandSegment)))
		{
			dm_Warning("WARNING! LTDirectMusic failed to create band segment for band %s\n",
			           sBandName);
		}
		else
		{
			m_pPerformance->PlaySegment(pDMBandSegment, DMUS_SEGF_SECONDARY, 0, LTNULL);

			pBand->m_pDMBandSegment = pDMBandSegment;
		}

		dm_Printf(3, "LTDirectMusic loaded band %s\n", sBandName);
	}

	return LTTRUE;
}


// *********************************************************************** //
//  Three of clientmusic.h's control-file readers
// *********************************************************************** //

// 0x00441960: every DLSBANK line's first word goes to LoadDLSBank.
LTBOOL CLTDirectMusicMgr::ReadDLSBanks(CControlFileMgr* pControlFile)
{
	CControlFileKey* pKey = pControlFile->GetKey(LTNULL, "DLSBANK");

	while (pKey)
	{
		CControlFileWord* pWord = pKey->GetFirstWord();

		if (pWord)
			LoadDLSBankQ440(pWord->GetVal());

		pKey = pKey->GetNextKey();
	}

	return LTTRUE;
}

// 0x00441a00: INTENSITY <n> <a> <b> <segment>...  n must be 1..the number
// of intensities; the second and third words go into the intensity's two
// numbers (defaults -1 and 0) and each segment named after them is loaded,
// looked up, and a fresh record carrying its DirectMusic segment is put at
// the tail of the intensity's list.  The walk stops at the first name that
// did not load.  The allocation's pop after a grow is the out-of-line call
// (0x00441af1), as in the MOTIF reader.
LTBOOL CLTDirectMusicMgr::ReadIntensities(CControlFileMgr* pControlFile)
{
	// The three numbers are one array: a 12-byte aggregate cannot live in
	// the dead parameter slot, so pKey gets it (0x00441a1d) as shipped;
	// three separate ints put nVals[0] there instead.
	int nVals[3];
	CControlFileKey* pKey = pControlFile->GetKey(LTNULL, "INTENSITY");

	while (pKey)
	{
		CControlFileWord* pWord = pKey->GetFirstWord();

		if (pWord)
		{
			nVals[0] = 0;

			pWord->GetVal(nVals[0]);

			if (nVals[0] > 0 && nVals[0] <= m_nNumIntensities)
			{
				pWord = pWord->m_pNext;

				if (pWord)
				{
					nVals[1] = -1;

					pWord->GetVal(nVals[1]);

					pWord = pWord->m_pNext;

					if (pWord)
					{
						nVals[2] = 0;

						pWord->GetVal(nVals[2]);

						pWord = pWord->m_pNext;

						if (pWord)
						{
							while (pWord)
							{
								LoadSegmentQ440(pWord->GetVal());

								CDMSegment* pDef = m_SecondaryDefs.FindByName(pWord->GetVal());

								if (!pDef || !pDef->m_pDMSegment)
									break;

								CDMSegment* pSegment = (CDMSegment*)g_SegmentBank.AllocOutOfLineQ440();

								if (!pSegment)
									break;

								pSegment->m_pDMSegment = LTNULL;
								pSegment->m_szName[0]  = '\0';
								pSegment->m_pName      = LTNULL;
								pSegment->m_nStopEnact = LTDMEnactNextMeasure;

								pSegment->m_pDMSegment = pDef->m_pDMSegment;

								m_aIntensities[nVals[0]].m_Segments.AddTail(pSegment);

								pWord = pWord->m_pNext;
							}

							m_aIntensities[nVals[0]].m_nUnknown00 = nVals[1];
							m_aIntensities[nVals[0]].m_nUnknown04 = nVals[2];

							// Its own copy of the loop step: shipped reloads
							// pKey into ecx here and jumps to the shared call
							// (0x00441b5a); one shared step reloaded into edi.
							pKey = pKey->GetNextKey();
							continue;
						}
					}
				}
			}
		}

		pKey = pKey->GetNextKey();
	}

	return LTTRUE;
}

// 0x00441b90: every SECONDARYSEGMENT line loads its segment and, when a
// second word follows, gives the definition that as its stop enact type
// (through the interface, slot 0x3c at 0x00441be4).
LTBOOL CLTDirectMusicMgr::ReadSecondarySegments(CControlFileMgr* pControlFile)
{
	CControlFileKey* pKey = pControlFile->GetKey(LTNULL, "SECONDARYSEGMENT");

	while (pKey)
	{
		CControlFileWord* pWord = pKey->GetFirstWord();

		if (pWord)
		{
			LoadSegmentQ440(pWord->GetVal());

			CDMSegment* pSegment = m_SecondaryDefs.FindByName(pWord->GetVal());

			if (pSegment)
			{
				pWord = pWord->m_pNext;

				if (pWord)
					pSegment->m_nStopEnact = StringToEnactType(pWord->GetVal());
			}
		}

		pKey = pKey->GetNextKey();
	}

	return LTTRUE;
}

// 0x00441c10: MOTIF <style> <motif> [<enact>].  The motif is fetched from
// the already-loaded style (IDirectMusicStyle slot 0x1c) and filed as a
// segment definition on the motif list.
LTBOOL CLTDirectMusicMgr::ReadMotifs(CControlFileMgr* pControlFile)
{
	CControlFileKey* pKey = pControlFile->GetKey(LTNULL, "MOTIF");

	while (pKey)
	{
		CControlFileWord* pWord = pKey->GetFirstWord();

		if (pWord)
		{
			CDMStyleDefQ440* pStyleDef = m_StyleDefs.FindByName(pWord->GetVal());

			pWord = pWord->m_pNext;

			if (pWord)
			{
				char* sMotifName = pWord->GetVal();

				if (pStyleDef && sMotifName)
				{
					uint16 wzName[DM_MAX_PATH];
					IDirectMusicSegment* pDMSegment;

					MultiByteToWideChar(DM_CP_ACP, DM_MB_PRECOMPOSED, sMotifName, -1,
					                    wzName, DM_MAX_PATH);

					if (pStyleDef->m_pDMStyle->GetMotif(wzName, &pDMSegment) == DM_S_OK
					    && pDMSegment)
					{
						CDMSegment* pSegment = (CDMSegment*)g_SegmentBank.AllocOutOfLineQ440();

						if (pSegment)
						{
							pSegment->m_pDMSegment = LTNULL;
							pSegment->m_szName[0]  = '\0';
							pSegment->m_pName      = LTNULL;
							pSegment->m_nStopEnact = LTDMEnactNextMeasure;

							pSegment->m_pDMSegment = pDMSegment;
							pSegment->SetName(sMotifName);

							pWord = pWord->m_pNext;

							if (pWord)
								pSegment->m_nStopEnact = StringToEnactType(pWord->GetVal());

							m_MotifDefs.AddHead(pSegment);
						}
					}
				}
			}
		}

		pKey = pKey->GetNextKey();
	}

	return LTTRUE;
}


// *********************************************************************** //
//  The two enact-type converters (interface slots)
// *********************************************************************** //

// 0x00441d70: TRANSITION <from> <to> <enact> MANUAL|AUTOMATIC [<segment>].
// Both intensities must be 1..the number of intensities, the enact word
// must convert to something other than LTDMEnactInvalid, and the fourth word
// must be one of the two; anything else skips the line.  The segment, when
// named, is loaded and its DirectMusic segment (or null, if it did not load)
// goes into the table entry; then the enact type and the manual flag do.
//
// NEAR (426 against 423, 13 blocks), and what is left is allocation:
// shipped keeps pKey in ebx and folds bManual into the zero register (ebp,
// `mov $1,%ebp` at 0x00441e3f, `xor %ebp,%ebp` after the step at
// 0x00441ee5) with nEnact spilled; we spill pKey and give bManual its own
// register.  Measured without effect or worse: bManual at function scope
// with a reset after the stores, at the top of the loop body, set on both
// the MANUAL and AUTOMATIC arms; nEnact at function scope; `if (nEnact)`;
// a flat goto-per-refusal body; a success-path copy of the step.  The
// pDMSegment local matches shipped's load before the GetTransition call
// (0x00441e88).
LTBOOL CLTDirectMusicMgr::ReadTransitions(CControlFileMgr* pControlFile)
{
	CControlFileKey* pKey = pControlFile->GetKey(LTNULL, "TRANSITION");

	while (pKey)
	{
		CControlFileWord* pWord = pKey->GetFirstWord();

		if (pWord)
		{
			int nFrom = 0;

			pWord->GetVal(nFrom);

			if (nFrom > 0 && nFrom <= m_nNumIntensities)
			{
				pWord = pWord->m_pNext;

				if (pWord)
				{
					int nTo = 0;

					pWord->GetVal(nTo);

					if (nTo > 0 && nTo <= m_nNumIntensities)
					{
						pWord = pWord->m_pNext;

						if (pWord)
						{
							LTDMEnactTypes nEnact = StringToEnactType(pWord->GetVal());

							if (nEnact != LTDMEnactInvalid)
							{
								pWord = pWord->m_pNext;

								if (pWord)
								{
									LTBOOL bManual = LTFALSE;

									if (_stricmp(pWord->GetVal(), "MANUAL") == 0)
										bManual = LTTRUE;
									else if (_stricmp(pWord->GetVal(), "AUTOMATIC") != 0)
										goto NextKey;

									pWord = pWord->m_pNext;

									if (pWord)
									{
										LoadSegmentQ440(pWord->GetVal());

										CDMSegment* pSegment = m_SecondaryDefs.FindByName(pWord->GetVal());

										if (pSegment)
										{
											IDirectMusicSegment* pDMSegment = pSegment->m_pDMSegment;
											GetTransition(nFrom, nTo)->m_pDMSegment = pDMSegment;
										}
										else
											GetTransition(nFrom, nTo)->m_pDMSegment = LTNULL;
									}

									GetTransition(nFrom, nTo)->m_nEnact  = nEnact;
									GetTransition(nFrom, nTo)->m_bManual = bManual;

									pKey = pKey->GetNextKey();
									continue;
								}
							}
						}
					}
				}
			}
		}

NextKey:
		pKey = pKey->GetNextKey();
	}

	return LTTRUE;
}

// 0x004426e0: write the name of an enact type into the caller's buffer.  A
// seven-entry jump table at 0x00442770 covers 0 to 6 and everything else
// lands on the empty string at 0x004b0ab4, which is a "" literal - this build
// keeps its literals in .data, which is where "REVERB" and "ON" live too.
//
// The copy comes out twice because VC6 duplicated the tail: the arms that
// jump to 0x00442721 get one copy and the arms that jump to 0x0044274a
// (with the default falling into it) get the other.
void CLTDirectMusicMgr::EnactTypeToString(LTDMEnactTypes nType, char* sName)
{
	switch (nType)
	{
		case LTDMEnactInvalid:     strcpy(sName, "Invalid");   break;
		case LTDMEnactDefault:     strcpy(sName, "Default");   break;
		case LTDMEnactImmediatly:  strcpy(sName, "Immediate"); break;
		case LTDMEnactNextBeat:    strcpy(sName, "Beat");      break;
		case LTDMEnactNextMeasure: strcpy(sName, "Measure");   break;
		case LTDMEnactNextGrid:    strcpy(sName, "Grid");      break;
		case LTDMEnactNextSegment: strcpy(sName, "Segment");   break;
		default:                   strcpy(sName, "");          break;
	}
}

// 0x00442790: the other direction, a chain of case-insensitive compares in
// the order the shipped bytes make them.  Both spellings of the immediate
// type are accepted, and the four short spellings come after the four long
// ones.  The last one is the `neg/sbb/and $0xfa/add $6` at 0x004428d1 - the
// final `if` and the final `return LTDMEnactInvalid` folded into arithmetic.
LTDMEnactTypes CLTDirectMusicMgr::StringToEnactType(const char* sName)
{
	if (!sName)
		return LTDMEnactInvalid;

	if (_stricmp(sName, "Invalid") == 0)
		return LTDMEnactInvalid;

	if (_stricmp(sName, "Default") == 0)
		return LTDMEnactDefault;

	if (_stricmp(sName, "Immediatly") == 0)
		return LTDMEnactImmediatly;

	if (_stricmp(sName, "Immediate") == 0)
		return LTDMEnactImmediatly;

	if (_stricmp(sName, "NextBeat") == 0)
		return LTDMEnactNextBeat;

	if (_stricmp(sName, "NextMeasure") == 0)
		return LTDMEnactNextMeasure;

	if (_stricmp(sName, "NextGrid") == 0)
		return LTDMEnactNextGrid;

	if (_stricmp(sName, "NextSegment") == 0)
		return LTDMEnactNextSegment;

	if (_stricmp(sName, "Beat") == 0)
		return LTDMEnactNextBeat;

	if (_stricmp(sName, "Measure") == 0)
		return LTDMEnactNextMeasure;

	if (_stricmp(sName, "Grid") == 0)
		return LTDMEnactNextGrid;

	if (_stricmp(sName, "Segment") == 0)
		return LTDMEnactNextSegment;

	return LTDMEnactInvalid;
}


// *********************************************************************** //
//  CDMLoader::GetObject's two halves
//
//  0x00443b90 is the dispatcher (not written here); these are the two things
//  it can do with a descriptor.  Both make the object first, wrap the source
//  in one of dmloader.cpp's two streams, and pour the stream into the object
//  through IPersistStream::Load.
// *********************************************************************** //

// 0x00443d20 (name ours): the file case.  The path is the descriptor's file
// name, either taken whole when DMUS_OBJ_FULLPATH is set or hung off the
// search directory registered for the class.
//
// The separator check reads the path's last character twice - two written-out
// wcslen calls at 0x00443dc5 and 0x00443ddc - and compares it against the
// loader's own two one-character strings, the backslash first.
//
// IDENTICAL (364 bytes; dmloader.cpp's LoadFromFile is the same member).
// One exit: a positive test of CoCreateInstance around everything (so all
// four callee-saved registers are pushed on entry and the refusal lands on
// the shared epilogue with the HRESULT still in eax, 0x00443d4d), and the
// out-of-memory case as an `else` that only sets hr -- it shares the final
// "failed, so release the object" test (0x00443e85 jumps back to 0x00443e67).
long CDMLoader::GetObjectFromFileQ440(DMUS_OBJECTDESC* pDesc,
                                      IDirectMusicObject** ppObject)
{
	uint16 wzPath[DM_MAX_PATH];
	IPersistStream* pPersistStream;
	CDMFileStream* pStream;
	long hr;

	hr = CoCreateInstance(pDesc->guidClass, LTNULL, DM_CLSCTX_INPROC_SERVER,
	                      IID_IDirectMusicObject, (void**)ppObject);

	if (SUCCEEDEDQ440(hr))
	{
		memset(wzPath, 0, sizeof(wzPath));

		pStream = new CDMFileStream(this);

		if (pStream)
		{
			if (pDesc->dwValidData & DMUS_OBJ_FULLPATH)
			{
				wcscpy(wzPath, pDesc->wszFileName);
			}
			else
			{
				CDMSearchDir* pDir = m_pSearchDirs->Find(pDesc->guidClass, 1);

				if (pDir)
					wcscpy(wzPath, pDir->m_wzPath);

				if (wzPath[wcslen(wzPath) - 1] != m_wzBackSlash[0] &&
				    wzPath[wcslen(wzPath) - 1] != m_wzFwdSlash[0])
				{
					wcscat(wzPath, m_wzBackSlash);
				}

				wcscat(wzPath, pDesc->wszFileName);
			}

			hr = pStream->Open(wzPath, 0x80000000);

			if (SUCCEEDEDQ440(hr))
			{
				hr = (*ppObject)->QueryInterface(IID_IPersistStream, (void**)&pPersistStream);

				if (SUCCEEDEDQ440(hr))
				{
					hr = pPersistStream->Load(pStream);

					pPersistStream->Release();
				}
			}

			pStream->Release();
		}
		else
		{
			hr = DM_E_OUTOFMEMORY;
		}

		if (FAILEDQ440(hr))
			(*ppObject)->Release();

	}

	return hr;
}

// 0x00443e90 (name ours): the memory case, the same shape over a borrowed
// buffer.  The length is 64 bits and goes across as its two halves, which is
// why the descriptor's 0x340/0x344 are read separately.
//
// Unlike the file case, the stream work sits inside a positive `if (pStream)`,
// with the out-of-memory result as the else branch, and one shared tail after
// both. VC6 threads the known-negative hr straight to the Release, and that
// is the block shipped has at the end (0x00443f3f). Written as an early
// refusal, the block lands inline after the test and is 2 bytes short.
long CDMLoader::GetObjectFromMemoryQ440(DMUS_OBJECTDESC* pDesc,
                                        IDirectMusicObject** ppObject)
{
	IPersistStream* pPersistStream;
	CDMMemStream* pStream;
	long hr;

	hr = CoCreateInstance(pDesc->guidClass, LTNULL, DM_CLSCTX_INPROC_SERVER,
	                      IID_IDirectMusicObject, (void**)ppObject);

	if (FAILEDQ440(hr))
		return hr;

	pStream = new CDMMemStream(this);

	if (pStream)
	{
		hr = pStream->SetBuffer(pDesc->pbMemData, pDesc->llMemLengthLow,
		                        pDesc->llMemLengthHigh);

		if (SUCCEEDEDQ440(hr))
		{
			hr = (*ppObject)->QueryInterface(IID_IPersistStream, (void**)&pPersistStream);

			if (SUCCEEDEDQ440(hr))
			{
				hr = pPersistStream->Load(pStream);

				pPersistStream->Release();
			}
		}

		pStream->Release();
	}
	else
	{
		hr = DM_E_OUTOFMEMORY;
	}

	if (FAILEDQ440(hr))
		(*ppObject)->Release();

	return hr;
}


// *********************************************************************** //
//  CDMFileStream::Open
// *********************************************************************** //

// 0x004432b0: shut whatever the stream had open, ask the engine's file
// manager for the record that matches the path, and open it.  The path
// arrives wide and is narrowed with wcstombs (0x004944f6) into a MAX_PATH
// buffer - the 0x104 the frame reserves and the 0x104 the call passes.
//
// The shipped trace string at 0x004abf1c names the class CLTDMFileStream,
// which is the real name for dmloader.cpp's guessed CDMFileStream.
//
// The second argument is never read.  0x00443e17 is the one call site and it
// passes 0x80000000, i.e. GENERIC_READ.
long CDMFileStream::Open(uint16* pwzFileName, uint32 dwDesiredAccess)
{
	char szFileName[DM_MAX_PATH];

	Close();

	if (!pwzFileName)
		return DM_E_FAIL;

	wcstombs(szFileName, pwzFileName, sizeof(szFileName));

	m_pFile = g_DMFileMgr.GetRecord(szFileName);

	if (!m_pFile)
		return DM_E_FAIL;

	if (m_pFile->Reopen_0042d210(LTNULL))
	{
		dm_Warning("LTDirectMusic loader failed to open file %s\n", szFileName);

		g_DMFileMgr.FreeRecord(m_pFile);
		m_pFile = LTNULL;

		return DM_E_FAIL;
	}

	dm_Printf(5, "CLTDMFileStream::Open FileName=%s\n", szFileName);

	return DM_S_OK;
}


// *********************************************************************** //
//  0x00442920 - the bank's pop, out of line
// *********************************************************************** //

// 13 bytes, immediately before the pending bank's grow (0x00442930).  Ten
// calls land on it: 0x00440771, 0x00440821, 0x0044086d (ChangeIntensity),
// 0x00440a4b, 0x00440aa5, 0x00440d0c, 0x00440d6a, 0x0044168b, 0x00441af1 and
// the MOTIF reader's 0x00441ce1 (AllocOutOfLineQ440 above).  Pop's own body:
// take the head of the free list if there is one; an empty bank answers null.
void* CDMBank::PopOutOfLineQ440()
{
	void* pRecord = m_pFree;

	if (m_pFree)
		m_pFree = *(void**)m_pFree;

	return pRecord;
}


// *********************************************************************** //
//  The band bank's grow
// *********************************************************************** //

// 0x00442b50 (name ours): one more chunk of band records on the free list.
// The records are 0x10 bytes - the `shl $0x4` at 0x00442b6a and the `add
// $0x10` at 0x00442b94 - and nothing initialises them, so CDMBand has no
// constructor; the segment bank's grow (0x004429d0), whose records do get
// four fields written, is the contrast.
//
// The array is a bare nCount * sizeof allocation with no leading count word,
// unlike 0x00442aa0 and 0x00442bd0 which allocate one extra dword and store
// the count in it.
//
// The list insert is written out here and is a call to 0x00442cc0 in the DLS
// bank's grow (0x004415c8); the two are the same five statements, and which
// one a site gets is the inline budget, exactly as with CDMBank::Pop.
int CDMBandBankQ440::Grow()
{
	CDMChunkQ440* pChunk = new CDMChunkQ440;

	if (!pChunk)
		return 0;

	CDMBand* pRecords = (CDMBand*)::operator new(m_nGrowCount * sizeof(CDMBand));

	pChunk->m_pRecords = pRecords;

	if (!pRecords)
		return 0;

	for (uint32 i = 0; i < m_nGrowCount; i++)
	{
		*(void**)pRecords = m_pFree;
		m_pFree = pRecords;
		pRecords++;
	}

	pChunk->m_pNext = m_Chunks.m_pHead;
	pChunk->m_pPrev = LTNULL;

	if (m_Chunks.m_pHead)
		m_Chunks.m_pHead->m_pPrev = pChunk;
	else
		m_Chunks.m_pTail = pChunk;

	m_Chunks.m_pHead = pChunk;

	return 1;
}


// *********************************************************************** //
//  0x0042d560 - the locked file list's record maker
// *********************************************************************** //
//
// HOME: not this TU.  0x0042d560 sits inside the locked-list block at
// 0x0042d210..0x0042d760 whose other bodies live in
// src/engine/lithtech/clientworld.cpp ("the locked list at 0x42d210..
// 0x42d760") and src/engine/lithtech/demodtx.cpp (CLockedItemQ42C::Load);
// written here because those files belong to other work, and it is the
// call CDMFileStream::Open (0x004432b0) makes.  Move it there with the
// rest of that block.
//
// A null name and a failed allocation both answer null; a record whose
// name copy fails is simply dropped (0x0042d5bd), not freed.
CDMFileRecord* CDMFileMgr::GetRecord(const char* sFileName)
{
	if (!sFileName)
		return LTNULL;

	CDMFileRecord* pRecord = new CDMFileRecord(this);

	if (!pRecord)
		return LTNULL;

	pRecord->m_Ref.m_pFilename = new char[strlen(sFileName) + 1];

	if (!pRecord->m_Ref.m_pFilename)
		return LTNULL;

	strcpy(pRecord->m_Ref.m_pFilename, sFileName);

	EnterCriticalSection(&m_CriticalSection);

	m_Unloaded.Add(pRecord);

	LeaveCriticalSection(&m_CriticalSection);

	return pRecord;
}
