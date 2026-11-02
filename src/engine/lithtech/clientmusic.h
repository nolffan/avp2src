// *********************************************************************** //
//
//  MODULE:   CLIENTMUSIC (.H)
//
//  PURPOSE:  CLTDirectMusicMgr — the engine's DirectMusic implementation of
//            ILTDirectMusicMgr.
//
//  Every entry point in this class opens the same way: trace what was
//  asked, refuse if the module or the level is not up, then do the work.
//  The two refusals are separate `if`s with identical bodies rather than
//  one condition — the shipped code reports twice from two blocks, and
//  keeps them both.
//
//  iltdirectmusic.h's declaration order is the vtable order, which the
//  class's own calls prove: Play reaches ChangeIntensity through slot 0x24
//  and TermLevel reaches Stop through slot 0x14, exactly where counting the
//  header puts them.
//
// *********************************************************************** //

#ifndef __CLIENTMUSIC_H__
#define __CLIENTMUSIC_H__

#include "ltbasedefs.h"
#include "iltdirectmusic.h"

// DirectMusic's own headers are not in this toolchain, so the interface is
// modelled with the slots this file calls and placeholders for the rest —
// enough to put those slots at the right offsets. The GUIDs are declared and
// never defined: relocations are masked, so the address that gets pushed does
// not have to be the real one for the bytes to match.
struct DMGUID { uint32 m_Data[4]; };

// 24 bytes, and only ever passed to the two calls below.
struct DMCRITICAL_SECTION { uint8 m_Data[24]; };

extern const DMGUID GUID_PerfMasterVolume;

// Every slot this file calls lands on a real IDirectMusicPerformance method
// counted in declaration order - Stop at 0x14, IsPlaying at 0x38,
// SetGlobalParam at 0x88 - which is what confirms the anchoring, and the
// three IUnknown slots at the top with it.
struct IDirectMusicSegmentState
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;                           // 0x08
	virtual long __stdcall GetRepeats(uint32* pdwRepeats)=0;       // 0x0c
	// 0x10: the notification thread (0x0043f5e2) asks a finished state for
	// its segment.
	virtual long __stdcall GetSegment(void** ppSegment)=0;          // 0x10
};

// dmusici.h's DMUS_NOTIFICATION_PMSG: DMUS_PMSG_PART (rtTime aligned to 8,
// punkUser at 0x34), then the notification's own fields.  The thread reads
// 0x34, 0x38 and 0x48 (0x0043f5bf, 0x0043f5d0, 0x0043f5f7).
struct DMUS_NOTIFICATION_PMSG
{
	uint32  dwSize;                          // 0x000
	uint32  m_pad_04;
	__int64 rtTime;                          // 0x008
	uint32  mtTime;                          // 0x010
	uint32  dwFlags;                         // 0x014
	uint32  dwPChannel;                      // 0x018
	uint32  dwVirtualTrackID;                // 0x01c
	void*   pTool;                           // 0x020
	void*   pGraph;                          // 0x024
	uint32  dwType;                          // 0x028
	uint32  dwVoiceID;                       // 0x02c
	uint32  dwGroupID;                       // 0x030
	IDirectMusicSegmentState* punkUser;      // 0x034
	DMGUID  guidNotificationType;            // 0x038
	uint32  dwNotificationOption;            // 0x048
	uint32  dwField1;                        // 0x04c
	uint32  dwField2;                        // 0x050
};

// DMUS_NOTIFICATION_SEGALMOSTEND / SEGEND.
#define DMUS_NOTIFICATION_SEGEND        1
#define DMUS_NOTIFICATION_SEGALMOSTEND  2

// 0x004a39bc, GUID_NOTIFICATION_SEGMENT.
extern const DMGUID GUID_NOTIFICATION_SEGMENT;

inline int operator==(const DMGUID& a, const DMGUID& b)
{
	return !memcmp(&a, &b, sizeof(DMGUID));
}

struct IDirectMusicPerformance8
{
	virtual long __stdcall _p00()=0; virtual long __stdcall _p04()=0;
	virtual long __stdcall _p08()=0; virtual long __stdcall _p0c()=0;
	// 0x10: start a segment; the state it hands back is what a playing node
	// is built around. The start time is a 64-bit music time, two words.
	virtual long __stdcall PlaySegment(void* pSegment, uint32 dwFlags,
	                                   __int64 i64StartTime,
	                                   IDirectMusicSegmentState** ppSegState)=0;
	// 0x14: stop a segment, a segment state, or everything.
	virtual long __stdcall Stop(void* pSegment, IDirectMusicSegmentState* pSegState,
	                            uint32 mtTime, uint32 dwFlags)=0;
	virtual long __stdcall _p18()=0; virtual long __stdcall _p1c()=0;
	virtual long __stdcall _p20()=0; virtual long __stdcall _p24()=0;
	virtual long __stdcall _p28()=0; virtual long __stdcall _p2c()=0;
	virtual long __stdcall _p30()=0; virtual long __stdcall _p34()=0;
	// 0x38: is this segment state still going?
	virtual long __stdcall IsPlaying(void* pSegment,
	                                 IDirectMusicSegmentState* pSegState)=0;
	virtual long __stdcall _p3c()=0;
	virtual long __stdcall _p40()=0;
	virtual long __stdcall FreePMsg(DMUS_NOTIFICATION_PMSG* pPMSG)=0;  // 0x44
	virtual long __stdcall _p48()=0; virtual long __stdcall _p4c()=0;
	// 0x50..0x58: the notification plumbing Init sets up and the thread
	// drains (0x0043f9d3, 0x0043f9f5, 0x0043f594, 0x0043f869).
	virtual long __stdcall SetNotificationHandle(void* hNotification,
	                                             __int64 rtMinimum)=0;  // 0x50
	virtual long __stdcall GetNotificationPMsg(DMUS_NOTIFICATION_PMSG** ppNotificationPMsg)=0; // 0x54
	virtual long __stdcall AddNotificationType(const DMGUID& rguidNotificationType)=0; // 0x58
	virtual long __stdcall _p5c()=0;
	virtual long __stdcall _p60()=0; virtual long __stdcall _p64()=0;
	virtual long __stdcall _p68()=0; virtual long __stdcall _p6c()=0;
	virtual long __stdcall _p70()=0; virtual long __stdcall _p74()=0;
	virtual long __stdcall _p78()=0; virtual long __stdcall _p7c()=0;
	virtual long __stdcall _p80()=0; virtual long __stdcall _p84()=0;
	// 0x88: the master volume goes through here rather than any per-segment
	// call, which is why SetVolume needs nothing but the performance.
	virtual long __stdcall SetGlobalParam(const DMGUID& rguidType, void* pParam,
	                                      uint32 dwSize)=0;
};

// The other DirectMusic objects the manager owns. InitLevel only passes them
// on, so they are opaque here; what identifies them is the slot each is used
// through elsewhere - CreatePort at 0x14 of the one at 0x0c, SetSearchDirectory
// at 0x14 of the one at 0x10, Activate at 0x3c of the one at 0x18.
struct IDirectMusic8;
struct IDirectMusicLoader8;
struct IDirectMusicPort;

// The port parameters the manager keeps at 0x1c: dwSize is written as 0x24
// and dwValidParams as 0xf (voices, channel groups, audio channels, sample
// rate), which is dmusicc.h's DMUS_PORTPARAMS8 exactly, and it runs to 0x40
// where the definition lists begin. 0x00440ff0 fills it from the three
// control-file numbers - PCHANNELS goes into dwAudioChannels as well as
// deciding dwChannelGroups - and InitLevel compares those three fields against
// the numbers it has just read to decide whether the port must be remade.
struct DMUS_PORTPARAMS8
{
	uint32 dwSize;                           // 0x000  0x24
	uint32 dwValidParams;                    // 0x004  0xf
	uint32 dwVoices;                         // 0x008  VOICES
	uint32 dwChannelGroups;                  // 0x00c  (PCHANNELS - 1) / 16 + 1
	uint32 dwAudioChannels;                  // 0x010  PCHANNELS
	uint32 dwSampleRate;                     // 0x014  SYNTHSAMPLERATE
	uint32 dwEffectFlags;                    // 0x018
	int    fShare;                           // 0x01c
	uint32 dwFeatures;                       // 0x020
};

// 0x004a3a3c: {480ff4b0-28b2-11d1-bef7-00c04fbf8fef}, CLSID_DirectMusicCollection.
extern const DMGUID CLSID_DirectMusicCollection;

// 0x00443ff0: widen a path (refusing one of MAX_PATH or more with E_FAIL) and
// hand it to the loader's SetSearchDirectory, slot 0x14. Cleans sixteen bytes
// itself and takes nothing in a register, so it is __stdcall (name ours).
long __stdcall dm_SetSearchDirectory(IDirectMusicLoader8* pLoader, const DMGUID& rguidClass,
                                     const char* sPath, int bClear);

// A segment the level's control file named: the loaded DirectMusic segment,
// the name (inline unless it was long enough to need allocating), and the
// enact type to use when stopping it if the caller did not say.
class CDMSegment
{
public:
	// The segments of one intensity are chained through here (ChangeIntensity
	// walks them with `mov (%edi),%edi`); +4 is presumably the back link the
	// shared list helpers keep, but nothing in this class reads it.
	CDMSegment* m_pNext;                     // 0x000
	uint8  m_pad_04[0x08 - 0x04];
	void*  m_pDMSegment;                     // 0x008
	char   m_szName[0x4c - 0x0c];            // 0x00c
	char*  m_pName;                          // 0x04c  used instead when set
	LTDMEnactTypes m_nStopEnact;             // 0x050

	// 0x004420a0: the allocated name if there is one, otherwise the inline
	// buffer - so it never answers null for a segment that has a name.
	const char* GetName();
};

// One playing segment. A node only exists while something is playing: the
// play calls add it after DirectMusic hands back a segment state, and never
// with a null one.
class CDMPlayingNode
{
public:
	CDMPlayingNode* m_pNext;                 // 0x000
	uint8  m_pad_04[0x08 - 0x04];
	IDirectMusicSegmentState* m_pSegState;   // 0x008
	CDMSegment* m_pSegment;                  // 0x00c
};

// A queued request: something to do when the segment now playing ends. Only
// the one type the play calls make is known.
// Type 3 carries one of the intensity's segments, 0xb the transition record,
// and 8 and 9 the intensity's two ints - 9 only when the second is positive -
// so those two act on a number rather than a segment. Which number means what
// is not verified; see CDMIntensity.
#define DMPENDING_PLAYSEGMENT    3
#define DMPENDING_PLAYSECONDARY  4
#define DMPENDING_PLAYMOTIF      5
#define DMPENDING_INTENSITY00    8
#define DMPENDING_INTENSITY04    9
#define DMPENDING_PLAYTRANSITION 0xb

class CDMPendingNode
{
public:
	CDMPendingNode* m_pNext;                 // 0x000
	CDMPendingNode* m_pPrev;                 // 0x004
	uint32      m_nType;                     // 0x008
	CDMSegment* m_pSegment;                  // 0x00c
	// The number types 8 and 9 carry. A pending record is 0x14 bytes - that
	// is what the bank's grow links - so this is the last field.
	int         m_nValue;                    // 0x010

	// Every allocation in ChangeIntensity guards exactly one store, the type,
	// behind the allocator's null test and leaves the others unguarded. That
	// is the shape `new` gives a constructor: the constructor runs only when
	// the allocation succeeded, the assignments after it run regardless.
	CDMPendingNode(uint32 nType) { m_nType = nType; }

	void* operator new(unsigned int nSize);
};

// The bank every node type comes from: a chunked allocator whose free records
// are chained through their own first word (names ours).
class CDMBank
{
public:
	uint8  m_pad_00[0x10];
	void*  m_pFree;                          // 0x010

	void* PopOutOfLineQ440();                // 0x00442920

	// The fast path run backwards, and inline at every site too. The null test
	// is part of it: each site tests the node again right after the list
	// Remove that cannot have changed it, except the one (0x004421b0) where
	// the same register was proven non-null a few instructions earlier and
	// VC6 dropped the retest.
	void Free(void* pRecord)
	{
		if (pRecord)
		{
			*(void**)pRecord = m_pFree;
			m_pFree = pRecord;
		}
	}
};

// Each bank grows through a routine of its own, with its record size in it:
// 0x00442930 for the pending nodes, 0x00442aa0 for the playing ones (both
// defined in dmgrow.cpp), so the allocation paths that call Grow live on the
// bank's own class and not on CDMBank (class names ours, as dmgrow.cpp's).
class CDMPendingBankQ440 : public CDMBank
{
public:
	int   Grow();                            // 0x00442930

	// The fast path is inline at every call site: take the head of the free
	// list if there is one, and only then go and grow.
	void* Alloc()
	{
		void* pRecord = m_pFree;

		if (pRecord)
		{
			m_pFree = *(void**)pRecord;
			return pRecord;
		}

		if (!Grow())
			return LTNULL;

		return PopOutOfLineQ440();
	}

	// The same allocation with nothing written out. The free list is only
	// tested here, so Pop runs out of line on both paths and VC6 folds the two
	// calls into one, hoisting the bank address above the test and
	// rematerialising it after the Grow that clobbered it. That is exactly
	// what ChangeIntensity's first two allocations do, and no written-out fast
	// path can ever come back out as a call, so those two sites are not the
	// source Alloc is.
	//
	// The module has both shapes and a third, where the pop after the Grow is
	// written out too (0x0043f6dc, and ChangeIntensity's last allocation).
	// That third one is Pop expanded inline, and whether VC6 expands it is a
	// per-function budget counted in the size of what has already been
	// expanded: giving a function eleven allocation sites instead of four
	// reproduces every shape in turn, from the end of the function backwards,
	// and so does making Pop's body more expensive at four sites. Neither the
	// size of the calling function nor its position in the file moves it. So
	// the shapes at ChangeIntensity's last two sites are reachable from Alloc
	// with Pop written in the header, but not at the same time as the first
	// two sites' shape, which needs a Pop call on the fast path.
	void* AllocPop()
	{
		if (m_pFree)
			return PopOutOfLineQ440();

		if (!Grow())
			return LTNULL;

		return PopOutOfLineQ440();
	}
};

class CDMPlayingBankQ440 : public CDMBank
{
public:
	int   Grow();                            // 0x00442aa0

	// CDMPendingBankQ440::Alloc over this bank's own Grow.
	void* Alloc()
	{
		void* pRecord = m_pFree;

		if (pRecord)
		{
			m_pFree = *(void**)pRecord;
			return pRecord;
		}

		if (!Grow())
			return LTNULL;

		return PopOutOfLineQ440();
	}
};

// Two instantiations, with different record sizes and so different grow
// routines (0x00442930 and 0x00442aa0).
extern CDMPendingBankQ440 g_PendingBank;     // 0x004b3fc8 (free list at +0x10)
extern CDMPlayingBankQ440 g_PlayingBank;     // 0x004b3f98

// ChangeIntensity is the only function in the class that reaches the bank
// through `new`, and the shape it gets is AllocPop's, not Alloc's.
inline void* CDMPendingNode::operator new(unsigned int nSize)
{
	return g_PendingBank.AllocPop();
}


// The other four, 0x18 bytes apart with the two above, which is what puts
// the bank's size at 0x18: TermLevel hands the level's records back to three
// of them by their free lists, and the array elements' destructor
// (0x004420b0) empties segment lists onto the fourth.
extern CDMBank g_BandBank;                   // 0x004b3fb0 (free list 0x004b3fc0)
extern CDMBank g_Unknown58Bank;              // 0x004b3f68 (free list 0x004b3f78)
extern CDMBank g_Unknown60Bank;              // 0x004b3f80 (free list 0x004b3f90)
extern CDMBank g_SegmentBank;                // 0x004b3f50 (free list 0x004b3f60)

// The doubly-linked list all of these use, head at 0x00 and tail at 0x04.
// One set of shared helpers serves every node type, so the linkage sits at
// the same two offsets in all of them.
class CDMPendingList
{
public:
	CDMPendingNode* m_pHead;                 // 0x000
	CDMPendingNode* m_pTail;                 // 0x004

	void AddHead(CDMPendingNode* pNode);                            // 0x0048de30
	void InsertAfter(CDMPendingNode* pAfter, CDMPendingNode* pNode);// 0x0048de90
	// 0x0048dee0: unlink one node, fixing up the head or tail when the node
	// was at either end. The node is not freed; that is the caller's bank.
	void Remove(CDMPendingNode* pNode);
	void AddTail(CDMPendingNode* pNode);                            // 0x0048de60
};

// The segments the level's control file defined, looked up by name.
class CDMSegmentDefList
{
public:
	uint8 m_pad_00[0x08];

	CDMSegment* FindByName(const char* pName);                      // 0x00441fe0
	// 0x00442180: stop and release every segment in the list, through
	// 0x004421b0 with enact LTDMEnactDefault; the second argument is passed
	// straight on as its last (name ours).
	void StopAll(class CLTDirectMusicMgr* pMgr, int bOnlyIfStopped);
};

class CDMPlayingList
{
public:
	CDMPlayingNode* m_pHead;                 // 0x000
	CDMPlayingNode* m_pTail;                 // 0x004

	void AddHead(CDMPlayingNode* pNode);                            // 0x0048de30

	// 0x00442080: the node playing this segment state (name ours).
	CDMPlayingNode* FindBySegState(IDirectMusicSegmentState* pSegState);

	// 0x00442030: the first node whose segment answers to this name.
	CDMPlayingNode* FindByName(const char* pName);
	// 0x00442270: stop one node's segment state, release it, unlink the node
	// and put it back on the node free list. Does nothing at all for a node
	// that is not playing.
	void StopNode(class CLTDirectMusicMgr* pMgr, CDMPlayingNode* pNode,
	              LTDMEnactTypes nEnact, int bOnlyIfStopped);
	// 0x00442240: StopNode on every node with enact LTDMEnactDefault, reading
	// each node's next link before the node goes back to the bank; the second
	// argument is passed straight on as its last (name ours).
	void StopAll(class CLTDirectMusicMgr* pMgr, int bOnlyIfStopped);
};

// The three lists TermLevel empties after leaving the critical section, at
// 0x50, 0x58 and 0x60, each with its own bank. What the records hold is read
// off what TermLevel does to them, and no more than that.
//
// A record of the list at 0x50 carries two COM objects. The one at 0x0c is
// simply released; the one at 0x08 is first handed the performance through
// its slot 0x14, and the only DirectMusic interface with a performance-taking
// method in that slot is IDirectMusicBand, whose Unload(IDirectMusicPerformance*)
// sits there — so these are the level's bands, and the object at 0x0c would
// be the segment CreateSegment made from one. The naming rests on that slot
// and its argument; nothing in TermLevel's bytes spells the interface out.
struct IDirectMusicBand
{
	virtual long __stdcall QueryInterface(const DMGUID& riid, void** ppv)=0;
	virtual uint32 __stdcall AddRef()=0;
	virtual uint32 __stdcall Release()=0;                           // 0x08
	virtual long __stdcall CreateSegment(void** ppSegment)=0;      // 0x0c
	virtual long __stdcall Download(IDirectMusicPerformance8* pPerf)=0;
	virtual long __stdcall Unload(IDirectMusicPerformance8* pPerf)=0; // 0x14
};

class CDMBand
{
public:
	CDMBand* m_pNext;                        // 0x000
	CDMBand* m_pPrev;                        // 0x004
	IDirectMusicBand* m_pDMBand;             // 0x008
	IDirectMusicSegmentState* m_pDMBandSegment; // 0x00c  released; only slot 0x08 is used
};

class CDMBandList
{
public:
	CDMBand* m_pHead;                        // 0x000
	CDMBand* m_pTail;                        // 0x004

	void Remove(CDMBand* pNode);                                    // 0x0048dee0
};

// The list at 0x58 holds records laid out exactly like CDMSegment: a COM
// object at 0x08 that gets released, and at 0x4c a name that is deleted when
// it was allocated. They come from a different bank, so they are a different
// type, and which DirectMusic object this is cannot be told from here.
class CDMUnknown58
{
public:
	CDMUnknown58* m_pNext;                   // 0x000
	CDMUnknown58* m_pPrev;                   // 0x004
	IDirectMusicSegmentState* m_pDMObject;   // 0x008  only slot 0x08 is used
	uint8  m_pad_0c[0x4c - 0x0c];
	char*  m_pName;                          // 0x04c  deleted when set
};

class CDMUnknown58List
{
public:
	CDMUnknown58* m_pHead;                   // 0x000
	CDMUnknown58* m_pTail;                   // 0x004

	void Remove(CDMUnknown58* pNode);                               // 0x0048dee0
};

// The list at 0x60: one COM object at 0x08, released, and nothing else touched.
class CDMUnknown60
{
public:
	CDMUnknown60* m_pNext;                   // 0x000
	CDMUnknown60* m_pPrev;                   // 0x004
	IDirectMusicSegmentState* m_pDMObject;   // 0x008  only slot 0x08 is used
};

class CDMUnknown60List
{
public:
	CDMUnknown60* m_pHead;                   // 0x000
	CDMUnknown60* m_pTail;                   // 0x004

	void Remove(CDMUnknown60* pNode);                               // 0x0048dee0
};

// A list of segment definitions with the same linkage as the rest.
class CDMSegmentList
{
public:
	CDMSegment* m_pHead;                     // 0x000
	CDMSegment* m_pTail;                     // 0x004

	// What the array-new of intensities runs per element: the two words at
	// 0x08 and 0x0c of each are zeroed there, and nothing else is.
	CDMSegmentList() { m_pHead = LTNULL; m_pTail = LTNULL; }
};

// One intensity level: the element of the counted array at 0xb8, indexed by
// intensity number (ChangeIntensity range-checks against 0xa0 and scales by
// sixteen), with one more element than there are intensities so that the
// numbers 1..N index it directly. The "INTENSITY" reader (0x00441a00) puts the
// line's second word at 0x00 (default -1) and third at 0x04 (default 0), and
// links the segments the line names onto the list at 0x08; what the two
// numbers mean is not in InitLevel's bytes, so they are named by offset.
// ChangeIntensity copies the one at 0x04 into a pending record's 0x10.
// The destructor (0x004420b0) empties the list back onto the segment bank.
class CDMIntensity
{
public:
	int m_nUnknown00;                        // 0x000
	int m_nUnknown04;                        // 0x004
	CDMSegmentList m_Segments;               // 0x008

	~CDMIntensity();                                                // 0x004420b0
};

// One transition between two intensities: the element of the array at 0xc0,
// (N+1)*(N+1) of them. The "TRANSITION" reader (0x00441d70) fills one from
// StringToEnactType (interface slot 0x3c), a MANUAL/AUTOMATIC word, and the
// loaded DirectMusic segment of a named secondary segment; InitLevel's
// defaults are next-measure, manual, and no segment.
class CDMTransition
{
public:
	LTDMEnactTypes m_nEnact;                 // 0x000
	int            m_bManual;                // 0x004
	void*          m_pDMSegment;             // 0x008
};

// lithshared's control-file reader (src/LT2/lithshared/incs/controlfilemgr.h),
// modelled here because its own header needs lith.h and lithtmpl.h, which the
// tree does not have. Only the members and the layout InitLevel touches are
// spelled out. The layout is read off the class's own code: the constructor
// (0x00491b20) zeroes 0x08, 0x0c and 0x18..0x28 around the six special
// characters at 0x10, Term (0x00491d10) tests 0x28 as m_bInitialized and
// frees the two strings at 0x08 and 0x0c, so the lists are two words each and
// the object is 0x2c bytes. Its vtable has four slots, the four file-access
// virtuals the header declares, in that order.
class CControlFileWord
{
public:
	CControlFileWord* m_pNext;               // 0x000
	CControlFileWord* m_pPrev;               // 0x004
	char* m_sVal;                            // 0x008
	int   m_nStrLen;                         // 0x00c

	char* GetVal() { return m_sVal; }
};

class CControlFileKey
{
public:
	CControlFileWord* GetFirstWord();                               // 0x00491a90
};

class CControlFileMgr
{
public:
	CControlFileMgr();                                              // 0x00491b20
	~CControlFileMgr();                                             // 0x00491b50

	int  Init(const char* sFileName, const char* sWhiteSpace = LTNULL); // 0x00491be0
	void Term();                                                    // 0x00491d10
	CControlFileKey* GetKey(const char* sSectionName, const char* sKeyName); // 0x00491e70
	int  GetKeyVal(const char* sSectionName, const char* sKeyName, int& nVal); // 0x00491ea0
	int  AddDefine(const char* sDefine);                            // 0x00491f60

	virtual int  FileOpen(const char* sName);                       // 0x004927f0
	virtual void FileClose();                                       // 0x00492820
	virtual int  FileGetChar();                                     // 0x00492830
	virtual int  FileEOF();                                         // 0x00492840

	uint8 m_pad_04[0x2c - 0x04];
};

// The reader InitLevel actually constructs: the base constructor is called and
// then the vtable at 0x004a2558 is stored over the base's, and that vtable's
// four slots are 0x00442cf0, 0x00442e50, 0x00442e60 and 0x00442e90 - a
// derived class in this module overriding the file access to go through the
// engine's own file streams. FileOpen reads the whole file into a buffer it
// keeps at 0x2c and walks with a cursor at 0x30 against an end at 0x34; those
// three words make the object 0x38 bytes, which is InitLevel's entire frame.
// Nothing initialises them at construction (name ours).
class CDMControlFileMgr : public CControlFileMgr
{
public:
	virtual int  FileOpen(const char* sName);                       // 0x00442cf0
	virtual void FileClose();                                       // 0x00442e50
	virtual int  FileGetChar();                                     // 0x00442e60
	virtual int  FileEOF();                                         // 0x00442e90

	char* m_pData;                           // 0x02c
	char* m_pCurPos;                         // 0x030
	char* m_pEnd;                            // 0x034
};

class CLTDirectMusicMgr : public ILTDirectMusicMgr
{
public:
	LTRESULT InitLevel(const char* sWorkingDirectory, const char* sControlFileName,
	                   const char* sDefine1, const char* sDefine2,
	                   const char* sDefine3);                        // 0x0043fcd0
	LTRESULT ChangeIntensity(const int nNewIntensity,
	                         const LTDMEnactTypes nStart);           // 0x00440620
	LTRESULT TermLevel();                                           // 0x00440230
	LTRESULT Play();                                                // 0x00440420
	LTRESULT SetVolume(const long nVolume);                         // 0x004405a0
	LTRESULT Stop(const LTDMEnactTypes nStart);                     // 0x00440480
	// 0x004422e0: the DMUS_SEGF_ flags an enact type asks for (name ours).
	uint32 EnactToSegFlags(const LTDMEnactTypes nEnact);

	LTRESULT PlaySecondary(const char* sSecondarySegment,
	                       const LTDMEnactTypes nStart);            // 0x00440940
	// The same header drift StopMotif has: no style name, eight bytes of
	// arguments rather than twelve.  VIRTUAL, and a NEW slot rather than an
	// override: 0x004a2500 slot 20 (0x50) is 0x00440c00, past the interface's
	// last slot GetInitialVolume (19), and the three-argument adapter below
	// dispatches it with `call *0x50(%eax)`.
	virtual LTRESULT PlayMotif(const char* sMotifName,
	                   const LTDMEnactTypes nStart);                // 0x00440c00
	// iltdirectmusic.h's three-argument PlayMotif, slot 12 (0x00440da0):
	// drops the style name and calls the two-argument overload.
	LTRESULT PlayMotif(const char* sStyleName, const char* sMotifName,
	                   const LTDMEnactTypes nStart);                // 0x00440da0
	LTRESULT StopSecondary(const char* sSecondarySegment,
	                       const LTDMEnactTypes nStart);            // 0x00440ae0
	// iltdirectmusic.h gives this one a style name as well, and the shipped
	// build does not: it cleans eight bytes of arguments, not twelve, and its
	// own trace string names only the motif. Header drift, the fourth
	// instance after ILTSoundMgr, ILTModel and ILTDirectMusicMgr's own
	// vtable - so it is declared here rather than overridden.
	// Slot 21 (0x54), the second new virtual.
	virtual LTRESULT StopMotif(const char* sMotifName,
	                   const LTDMEnactTypes nStart);                // 0x00440dc0
	// iltdirectmusic.h's three-argument StopMotif, slot 13 (0x00440ee0).
	LTRESULT StopMotif(const char* sStyleName, const char* sMotifName,
	                   const LTDMEnactTypes nStart);                // 0x00440ee0

	// THE THREE-ARGUMENT SDK OVERLOADS, 0x00440da0 and 0x00440ee0, 18 bytes
	// each, are the other side of the header drift noted above: each drops
	// sStyleName and dispatches the two-argument overload through the vtable
	//
	//      8b 54 24 0c  mov 0xc(%esp),%edx      ; nStart
	//      8b 01        mov (%ecx),%eax         ; vptr
	//      52           push %edx
	//      8b 54 24 0c  mov 0xc(%esp),%edx      ; sMotifName
	//      52           push %edx
	//      ff 50 50     call *0x50(%eax)        ; StopMotif is *0x54(%eax)
	//      c2 0c 00     ret $0xc
	//
	// which is why the two-argument forms are virtual (slots 20 and 21).

	// The helpers InitLevel calls on itself, none of them interface slots
	// (names ours, from what each does).
	//
	// 0x00441190: replace one of the two directory strings - 1 is the one at
	// 0xc4, 2 the one at 0xc8 - with a copy of the argument, then tell the
	// loader through dm_SetSearchDirectory, with a class GUID chosen by the
	// same number through a six-way switch. What the numbers stand for is not
	// in the SDK header, so it stays an int.
	void SetWorkingDirectory(const char* sWorkingDirectory, int nDirType);
	// 0x00440ff0: release the port at 0x18 if there is one, fill m_PortParams
	// from the three control-file numbers, CreatePort, Activate it, AddPort it
	// to the performance and AssignPChannelBlock one block per channel group.
	int  CreatePort();
	// 0x004425b0: the REVERB, REVERBINGAIN, REVERBHIGHFREQRTRATIO, REVERBMIX
	// and REVERBTIME keys.
	void ReadReverbParameters(CControlFileMgr* pControlFile);
	// The six readers, each named after the key it walks: DLSBANK, STYLE,
	// INTENSITY, SECONDARYSEGMENT, MOTIF, TRANSITION.
	void ReadDLSBanks(CControlFileMgr* pControlFile);               // 0x00441960
	void ReadStyles(CControlFileMgr* pControlFile);                 // 0x004419b0
	void ReadIntensities(CControlFileMgr* pControlFile);            // 0x00441a00
	void ReadSecondarySegments(CControlFileMgr* pControlFile);      // 0x00441b90
	void ReadMotifs(CControlFileMgr* pControlFile);                 // 0x00441c10
	void ReadTransitions(CControlFileMgr* pControlFile);            // 0x00441d70

	// The three TermLevel calls on this class that are not interface slots
	// (names ours). 0x00442100 empties the pending list onto its bank;
	// 0x004426c0 tests the word at 0xe8 and jumps into 0x00442490 when it is
	// set, answering 1 otherwise.
	// 0x00442130: the row of the transition table for going from one
	// intensity to another, or its first word when either is out of range
	// (names ours).
	CDMTransition* GetTransition(int nFrom, int nTo);
	void ClearPendingList();                                        // 0x00442100
	int  Unknown4426c0();                                           // 0x004426c0

public:
	// The two gates every entry point checks, in that order.
	LTBOOL m_bInitialized;                   // 0x004
	LTBOOL m_bLevelInitialized;              // 0x008
	IDirectMusic8*            m_pDirectMusic;// 0x00c  CreatePort is called on it
	IDirectMusicLoader8*      m_pLoader;     // 0x010  SetSearchDirectory is called on it
	IDirectMusicPerformance8* m_pPerformance;// 0x014
	IDirectMusicPort*         m_pPort;       // 0x018
	DMUS_PORTPARAMS8          m_PortParams;  // 0x01c  to 0x40
	CDMSegmentDefList m_SecondaryDefs;       // 0x040
	CDMSegmentDefList m_MotifDefs;           // 0x048
	CDMBandList       m_BandList;            // 0x050
	CDMUnknown58List  m_Unknown58List;       // 0x058
	CDMUnknown60List  m_Unknown60List;       // 0x060
	// Requests waiting for the segment now playing to end, and the node to
	// queue the next one behind.
	CDMPendingList m_PendingList;            // 0x068
	CDMPendingNode* m_pQueueAfter;           // 0x070
	CDMPlayingList m_SecondaryList;          // 0x074
	CDMPlayingList m_MotifList;              // 0x07c
	DMCRITICAL_SECTION m_CriticalSection;    // 0x084
	// The intensity now playing: ChangeIntensity compares its argument with
	// this and reports "new intensity same as old" when they agree, and
	// InitLevel starts it at zero.
	int    m_nCurIntensity;                  // 0x09c
	// The level's numbers, from the control file's top-level keys in this
	// order: NUMINTENSITIES, INITIALINTENSITY, INITIALVOLUME, PCHANNELS,
	// VOICES, SYNTHSAMPLERATE. The first three default to zero, the channels
	// and voices to 64, the sample rate to 44100.
	int    m_nNumIntensities;                // 0x0a0
	// What Play asks ChangeIntensity for, so it is the level's starting
	// intensity — the number GetInitialIntensity hands out.
	// What Play asks ChangeIntensity for, so it is the level's starting
	// intensity — the number GetInitialIntensity hands out.
	int    m_nInitialIntensity;              // 0x0a4
	int    m_nInitialVolume;                 // 0x0a8
	int    m_nNumPChannels;                  // 0x0ac
	int    m_nNumVoices;                     // 0x0b0
	int    m_nSynthSampleRate;               // 0x0b4
	// The arrays the level is built on: N+1 intensities (a counted array, its
	// element count in the dword before the first element) and (N+1)*(N+1)
	// transitions, both allocated by InitLevel and freed by TermLevel.
	CDMIntensity*  m_aIntensities;           // 0x0b8
	int            m_nNumTransitions;        // 0x0bc
	CDMTransition* m_aTransitions;           // 0x0c0
	// The two directory strings SetWorkingDirectory replaces, type 1 and 2.
	char*  m_pUnknownC4;                     // 0x0c4
	char*  m_pUnknownC8;                     // 0x0c8
	// Init copies GUID_NOTIFICATION_SEGMENT here and registers it
	// (0x0043f9a7); the event it hands the performance (0x0043f9e9), the
	// quit flag the thread polls (0x0043f543) and the thread (0x0043faaa)
	// follow.  Names ours.
	DMGUID m_guidNotifyType;                 // 0x0cc
	void*  m_hNotifyEvent;                   // 0x0dc
	int    m_bQuitThread;                    // 0x0e0
	void*  m_hThread;                        // 0x0e4
};

// 0x004b3fe0: the manager the constructor publishes (0x0043f915), which the
// notification thread reaches everything through (name ours).
extern CLTDirectMusicMgr* g_pDirectMusicMgr;

// 0x0043f520: the notification thread Init starts with _beginthread (name
// ours).
void __cdecl dm_NotificationThread(void* pParam);

// The module's own tracing, all three gated on one verbosity global at
// 0x004a9fb0 (names ours).
void dm_Printf(int nLevel, char* pFormat, ...);                     // 0x0043eef0
void dm_Warning(char* pFormat, ...);                                // 0x0043ee50
void dm_NotInitError(char* pFnName);
// The module's own tracing, all three gated on one verbosity global at
// 0x004a9fb0 (names ours).
void dm_Printf(int nLevel, char* pFormat, ...);                     // 0x0043eef0
void dm_Warning(char* pFormat, ...);                                // 0x0043ee50
void dm_NotInitError(char* pFnName);                                // 0x0043ef90
// 0x0043edb0: the same shape as dm_Warning - a printf into a stack buffer,
// the trailing newline stripped, and one console print - but gated at level
// one where dm_Warning is gated at two, and in a different colour. Every
// "ERROR!" string in InitLevel goes through it (name ours).
void dm_Error(char* pFormat, ...);
                                // 0x0043ef90

// Nothing here calls dfree (0x00429080) directly: every free in TermLevel
// reaches 0x004290c0, the engine's operator delete, which wraps it.

extern "C" {
__declspec(dllimport) void __stdcall EnterCriticalSection(DMCRITICAL_SECTION* pSection);
__declspec(dllimport) void __stdcall LeaveCriticalSection(DMCRITICAL_SECTION* pSection);
__declspec(dllimport) uint32 __stdcall WaitForSingleObject(void* hHandle, uint32 dwMilliseconds);
__declspec(dllimport) int __stdcall CloseHandle(void* hObject);
void __cdecl _endthread(void);
}

#endif  // __CLIENTMUSIC_H__
