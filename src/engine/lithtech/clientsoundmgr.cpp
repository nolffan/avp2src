// *********************************************************************** //
//
//  MODULE:   SOUNDMGR (.CPP)
//
//  PURPOSE:  The rest of CSoundMgr — the client's ILTClientSoundMgr — from
//            0x00480000 to 0x0048265a.  src/engine/lithtech/clientsound.cpp
//            already holds the accessors that forward to a sound's vtable
//            (and the filter enumeration above this range);
//            src/engine/lithtech/clientsoundinst.cpp holds CSoundInstance
//            and CSoundData.  What is left in this run is the manager's own
//            machinery: the 3D provider list, the Miles voice pools, the
//            master volume, the focus (release/reacquire) pair, and the
//            handle-level API.
//
//  WHAT NAMED THE FUNCTIONS.  The class's vtable is at 0x004a356c and its
//  slot order IS the declaration order of ILTSoundMgr followed by
//  ILTClientSoundMgr in src/LT2/sdk/inc/iltsoundmgr.h.  Eleven of its slots are
//  already pinned by clientsound.cpp (SetReverbProperties at 0x00482010,
//  GetReverbProperties at 0x00482230, the four occlusion/obstruction
//  entries, the three filter entries, Get/SetSoundPosition), and every one
//  of those lands exactly where the header puts it, so the slots either side
//  of them are named for free:
//
//      +0x000 0x004822f0  PlaySound                    (not written here)
//      +0x004 0x004823f0  GetSoundDuration             (clientsound.cpp)
//      +0x008 0x00482510  IsSoundDone                  <- here
//      +0x00c 0x00482540  KillSound                    <- here
//      +0x010 0x00482570  KillSoundLoop                <- here
//      +0x014 0x004825a0  GetSound3DProviderLists      <- here
//      +0x018 0x00482630  ReleaseSound3DProviderList   <- here
//      +0x01c 0x00482660  InitSound                    (clientsound.cpp)
//      +0x020 0x00481650  GetVolume                    <- here
//      +0x024 0x004815e0  SetVolume                    <- here
//      +0x028 0x00482010  SetReverbProperties          (clientsound.cpp)
//      ...
//      +0x05c 0x00481f70  PauseSounds                  <- here
//      +0x060 0x00481fc0  ResumeSounds                 <- here
//
//  The two focus functions at 0x00481e70/0x00481ef0 confirm that numbering
//  from the other side: each calls its own class through `*0x5c(%eax)` and
//  `*0x60(%eax)`, which are exactly those two slots.
//
//  WHY VIEW CLASSES.  clientsound.h already declares CSoundMgr, and
//  clientsoundinst.cpp already declares CSoundInstanceView/CSoundDataView
//  with the fields THOSE files need.  Neither is this reconstruction's to
//  change and neither can be extended from here without risking the symbols
//  those TUs already match, so the three classes are redeclared as
//  layout-identical views under names of their own (house style: see the
//  IObjInsideTestView comment in src/engine/server/servercommon.cpp).  Every
//  offset below is read off the shipped bytes and the note beside it says
//  from where.
//
// *********************************************************************** //

#include <string.h>
#include <stdlib.h>

#include "ltbasedefs.h"
#include "iltsoundmgr.h"
#include "ltlink.h"
#include "collection.h"
#include "packet.h"


// ---- the engine's own provider record ------------------------------------ //
//
// The SDK's Sound3DProvider (ltbasedefs.h) is what the GAME sees; it is 0x114
// bytes with the id at 0x00, the name at 0x04, the caps at 0x10c and the next
// pointer at 0x110.  The engine keeps a different, longer record internally,
// and GetSound3DProviderLists (0x004825a0) is the converter between the two —
// which is what proves the internal layout is the public one with a Miles
// HPROVIDER bolted on the front:
//
//   * 0x0048049d allocates 0x118 bytes, four more than the SDK struct;
//   * 0x004804ce stores the SOUND3DPROVIDERID_xxx value at +0x04, 0x004804d7
//     copies the name to +0x08, 0x004804bd stores the EAX/reverb flag at
//     +0x110 and 0x004804b1 threads the list through +0x114 — every field
//     four bytes above where the SDK puts it;
//   * 0x004804d5 stores the HPROVIDER that AIL_enumerate_3D_providers handed
//     back at +0x00, the hole that shift leaves;
//   * and 0x004825e0..0x00482610 copies the four public fields out of an
//     internal record at 0x008/0x110/0x004 into a fresh 0x114-byte public
//     one at 0x004/0x10c/0x000.
//
// The manager also embeds one of these at +0x13c for the provider in use, and
// 0x004805c7 fills it with a single `rep movsl` of 0x46 dwords = 0x118 bytes,
// which is a plain struct assignment and confirms the size.
struct Sound3DProviderInternal
{
	// 0x000 — the Miles HPROVIDER.  clientsound.h calls the manager's copy of
	// this field m_h3DProvider, and it is the same field: the manager's
	// embedded record starts at 0x13c.
	void*                    m_hProvider;
	uint32                   m_dwProviderID;               // 0x004
	char                     m_szProvider[_MAX_PATH + 1];  // 0x008
	uint32                   m_dwCaps;                     // 0x110
	Sound3DProviderInternal* m_pNextProvider;              // 0x114
};                                                         // 0x118


// ---- one Miles voice ----------------------------------------------------- //
//
// The voice pools are flat arrays with a sixteen-byte stride (`shl $0x4` on
// the index at 0x00480688, 0x004806d8 and their software twins), and the only
// two things in those sixteen bytes are the sample handle at +0x00 (stored at
// 0x004806e7, read at 0x004807d0) and an LTLink at +0x04 that threads the
// voice onto the manager's free list.  0x0048071f writes the link's m_pData
// with the address of the voice itself, so a link found on the list yields
// its voice with one dereference — which is what 0x00481a30's tail
// (`mov 0x8(%eax),%eax; mov (%eax),%eax`) does.
struct SoundVoice
{
	// Empty inline constructor: Alloc3DVoices/AllocSWVoices' `new SoundVoice[n]`
	// leaves the dead count-1 store of VC6's construction loop (0x00480698).
	SoundVoice() {}

	void*  m_hSample;   // 0x00
	LTLink m_Link;      // 0x04
};                      // 0x10


// ---- StructBank / ObjectBank, as the manager embeds them ----------------- //
//
// Four banks sit in a row at 0x284, 0x2b8, 0x2dc and 0x300 and the spacing
// proves the shape: 0x24 bytes each, which is a vptr plus LT2's seven-dword
// StructBank plus the empty critical-section member ObjectBank carries.  The
// free-list head is the bank's last dword, so it lands at base+0x04+0x18 —
// 0x2a0, 0x2d4, 0x2f8 and 0x31c, and those are exactly the four addresses the
// allocate/free sites below poke.  0x00480a98 reads the cache size at
// 0x290 = 0x284+0x04+0x08, which is StructBank's third field, so the field
// order is src/LT2/lithshared/stdlith/struct_bank.h's.
//
// Modelled by hand rather than by including object_bank.h, because that
// header is not on this target's include path (see build/engine.sh: the
// lithtech target compiles with /I LT2\sdk\inc /I ..\server /I .).
struct StructBankView
{
	uint32 m_StructSize;          // +0x00
	uint32 m_AlignedStructSize;   // +0x04
	uint32 m_CacheSize;           // +0x08
	uint32 m_nPages;              // +0x0c
	uint32 m_nTotalObjects;       // +0x10
	void*  m_pPageHead;           // +0x14
	void*  m_pFreeListHead;       // +0x18
};                                // 0x1c

struct ObjectBankView
{
	// +0x00 is BaseObjectBank's vptr, and the manager's Term proves the slot
	// map: 0x00480119, 0x00480128 and 0x00480137 each do
	// `mov (bank),%reg; lea bank,%ecx; call *0xc(%reg)`, which is the fourth
	// slot of dtor/AllocVoid/FreeVoid/Term.  Held as a plain word rather than
	// as virtuals because nothing in this file calls through it and a member
	// of an abstract type will not compile.
	void*          m_pVTable;    // +0x00
	StructBankView m_Bank;       // +0x04
	uint32         m_CS;         // +0x20  the empty CS member's padding
};                               // 0x24


// ---- the sample and the sound, from the manager's side ------------------- //
//
// Both are named after clientsoundinst.cpp's own views, which established the
// vtable extents and most of the fields; only what this file reaches is
// declared, and the two fields this file adds are called out below.
class CSoundInstanceRef;

class CSoundDataRef
{
public:
	virtual ~CSoundDataRef();                      // +0x00 0x0047c6b0
	virtual void _v04(void* pFileRef)=0;           // +0x04 (load; see 0x00480abf)
	virtual void _v08()=0;
	virtual void Free()=0;
	// +0x10 / +0x14 -- the two per-sample tests Init (0x0047f9e0) sweeps
	// the list with: +0x10 before the old sound system is torn down, +0x14
	// after the new one is up; a sample either answers true for is freed.
	virtual LTBOOL _v10()=0;
	virtual LTBOOL sub_47d020()=0;
	virtual LTBOOL Load()=0;

	// 0x0047d160 — of the instances playing this sample, the one to give up
	// (clientsoundinst.cpp's CSoundDataView::sub_47d160).
	CSoundInstanceRef* sub_47d160();
	// 0x0047d1a0 — sndinst2.cpp's CSoundDataQ47C::sub_47d1a0: may this
	// instance start on this sample now?  Update calls it at 0x004811c7.
	LTBOOL sub_47d1a0(CSoundInstanceRef* pInst);

public:
	// 0x04 — the file record the sample was loaded from; its +0x1c is the
	// name Term reports an unfreed handle by (0x004800cf..0x004800d5).
	struct CSoundFileRecRef* m_pFileRec;     // 0x04
	uint8  m_pad_08[0x28 - 0x08];
	// 0x28 — a byte of flags; FreeSoundData (0x00480b57) tests bit 0 with a
	// byte `test`, and only when it is clear does it sweep the 3D voices
	// looking for a voice still pointing at this sample.
	uint8  m_Flags28;                        // 0x28
	uint8  m_pad_29[0x40 - 0x29];
	// 0x40 — set to 1 by 0x00480a7d when the manager hands back a sample that
	// is already loaded, cleared for every sample by 0x00481c60, and required
	// to be zero before 0x00481ca0 will free a sample.  A "wanted since the
	// last sweep" mark; the name is by address because nothing here proves
	// more than that.
	uint32 m_nUnknown40;                     // 0x40
	uint8  m_pad_44[0x88 - 0x44];
	uint32 m_nInstances;                     // 0x88
	LTLink m_Instances;                      // 0x8c
	// 0x98 — this sample's place on the manager's list at 0x2a8.  0x00480ae7
	// ties it on with dl_AddTail and 0x00480b34 takes it off again.
	LTLink m_MgrLink;                        // 0x98
};

class CSoundInstanceRef
{
public:
	// 0x0047d310 — out of line; the in-head subclass's implicit
	// constructor calls it (0x00483630).
	CSoundInstanceRef();
	virtual ~CSoundInstanceRef();                          // +0x00 0x0047d3e0
	// +0x04 — bring the sound up on one sample (soundinst2.cpp's Init);
	// PlaySoundFromFile calls it through `call *0x4(%eax)` at 0x0048180e.
	virtual LTRESULT Init(CSoundDataRef* pData, PlaySoundInfo* pInfo,
	                      uint32 nUnknown)=0;
	virtual void     Term()=0;                             // +0x08 0x0047d5c0
	virtual void     _v0c()=0;
	virtual void     _v10()=0;
	virtual void     _v14()=0;
	virtual LTRESULT Stop(int nUnknown)=0;                 // +0x18
	virtual void     _v1c(int nUnknown)=0;                 // +0x1c
	virtual LTRESULT Hold()=0;                             // +0x20
	virtual LTRESULT ReleaseHold()=0;                      // +0x24

	// 0x0047e2c0 — take the loop off a playing sound (it clears the Miles
	// sample's loop block and count).  Not virtual: KillSoundLoop reaches it
	// with a direct `call` at 0x0048258a.
	LTRESULT KillLoop();

	// 0x0047e360 / 0x0047e390 -- clientsoundinst.cpp's detach and reattach
	// (CSoundInstanceView).  Init (0x0047f9e0) calls both with a direct
	// `call` and releases the sound when the answer is non-zero.
	LTRESULT sub_47e360();
	LTBOOL   sub_47e390();

public:
	uint32 m_Type;                           // 0x04
	CSoundDataRef* m_pData;                  // 0x08
	uint8  m_pad_0c[0x10 - 0x0c];
	// 0x10 — bit 4 (0x10) is the game-owns-this-handle bit: Term
	// (0x004800b9) reports a sound that still has it as an unfreed handle,
	// 0x00481ca0 spares such a sound, and 0x00481cce releases the rest.  Bit 9
	// (0x200) is what makes a handle valid, which is 0x00480d28's test.
	uint32 m_Flags10;                        // 0x10
	uint32 m_nTableSlot;                     // 0x14
	uint8  m_pad_18[0x1c - 0x18];
	// 0x1c — a float here.  clientsoundinst.cpp calls it m_Unknown1c and does
	// not type it; the sort at 0x00481500 loads it with `flds` and compares it
	// with `fcompp`, so on this side it is float.
	float  m_f1c;                            // 0x1c
	float  GetF1c() const { return m_f1c; }
	uint8  m_pad_20[0x24 - 0x20];
	uint32 m_nPriority;                      // 0x24
	uint8  m_pad_28[0x2c - 0x28];
	// 0x2c — read only beside m_nPriority, as a signed difference
	// (0x0048159a's `sub` is followed by `jns`).  Name by address.
	int32  m_nUnknown2c;                     // 0x2c
	uint8  m_pad_30[0x40 - 0x30];
	// 0x40 — bit 2 (0x04) is "stopped", which is what IsSoundDone reports on;
	// bit 1 (0x02) is tested by the sort and by CSoundInstance::Hold.
	uint32 m_StateFlags;                     // 0x40
	uint8  m_pad_44[0x58 - 0x44];
	// 0x58 — this sound's own index in the manager's table at 0x324.
	// ReleaseSound reads it at 0x00480bdc to know which slot to fill in with
	// the last one, then writes the moved sound's new index back at
	// 0x00480bff.
	uint32 m_nMgrIndex;                      // 0x58
};

// The three concrete sounds, told apart by m_Type (0 in-head, 1 ambient,
// 2 3D) and by the bank each comes from.  Only the constructors matter
// here; the overrides are soundinst2.cpp's.  The in-head class has NO
// constructor of its own: its implicit one is the base call plus the vptr
// store (0x004a3698), which clientstreams.cpp's bank AllocVoid (0x00483490)
// expands in place and which PlaySoundFromFile below CALLS -- that call is
// what put the out-of-line copy at 0x00483630.
class CSoundInstanceLocalRef : public CSoundInstanceRef
{
public:
	CSoundInstanceLocalRef();                                   // 0x00483630
	virtual LTRESULT Init(CSoundDataRef* pData, PlaySoundInfo* pInfo,
	                      uint32 nUnknown);                     // 0x0047e710
	virtual void     Term();
	virtual void     _v0c();
	virtual void     _v10();
	virtual void     _v14();
	virtual LTRESULT Stop(int nUnknown);
	virtual void     _v1c(int nUnknown);
	virtual LTRESULT Hold();
	virtual LTRESULT ReleaseHold();
};

class CSoundInstanceAmbientRef : public CSoundInstanceRef
{
public:
	CSoundInstanceAmbientRef();                                 // 0x0047e970
	virtual LTRESULT Init(CSoundDataRef* pData, PlaySoundInfo* pInfo,
	                      uint32 nUnknown);
	virtual void     Term();
	virtual void     _v0c();
	virtual void     _v10();
	virtual void     _v14();
	virtual LTRESULT Stop(int nUnknown);
	virtual void     _v1c(int nUnknown);
	virtual LTRESULT Hold();
	virtual LTRESULT ReleaseHold();
};

class CSoundInstance3DRef : public CSoundInstanceRef
{
public:
	CSoundInstance3DRef();                                      // 0x0047eda0
	virtual LTRESULT Init(CSoundDataRef* pData, PlaySoundInfo* pInfo,
	                      uint32 nUnknown);
	virtual void     Term();
	virtual void     _v0c();
	virtual void     _v10();
	virtual void     _v14();
	virtual LTRESULT Stop(int nUnknown);
	virtual void     _v1c(int nUnknown);
	virtual LTRESULT Hold();
	virtual LTRESULT ReleaseHold();
};


// object_bank.h's ObjectBank<T>::Allocate, for the three instance banks:
// struct_bank.h's inline sb_Allocate (head, grow through 0x00490c20, the
// RE-READ, unlink) and then placement new behind its own null test.  The
// layout is ObjectBankView's; restated as a template (and sb_Allocate by
// hand) because object_bank.h is not on this target's include path.
int sb_AllocateNewStructPage(StructBankView* pBank, unsigned long nAllocations);

inline void* sb_AllocateRef(StructBankView* pBank)
{
	void* pRet;

	pRet = pBank->m_pFreeListHead;
	if (!pRet)
	{
		sb_AllocateNewStructPage(pBank, pBank->m_CacheSize);
		pRet = pBank->m_pFreeListHead;
		if (!pRet)
			return 0;
	}

	pBank->m_pFreeListHead = *(void**)pRet;
	return pRet;
}

class DummyThingForConstructorRef
{
public:
};

inline void* operator new(size_t size, DummyThingForConstructorRef* pThing,
                          void* ptr)
{
	return ptr;
}

inline void operator delete(void* pDataPtr, DummyThingForConstructorRef* pThing,
                            void* ptr)
{
}

inline void sb_FreeRef(StructBankView* pBank, void* pObj)
{
	*(void**)pObj = pBank->m_pFreeListHead;
	pBank->m_pFreeListHead = pObj;
}

class NullCSRef
{
public:
	void Enter() {}
	void Leave() {}
};

template<class T, class CS = NullCSRef>
struct ObjectBankRef
{
	T* Allocate()
	{
		T* pRet;

		m_CS.Enter();
			pRet = (T*)sb_AllocateRef(&m_Bank);
			if (pRet)
			{
				::new((DummyThingForConstructorRef*)0, pRet) T;
			}
		m_CS.Leave();

		return pRet;
	}

	void Free(T* pObj)
	{
		m_CS.Enter();
			pObj->~T();
			sb_FreeRef(&m_Bank, pObj);
		m_CS.Leave();
	}

	void*          m_pVTable;    // +0x00
	StructBankView m_Bank;       // +0x04
	CS             m_CS;         // +0x20
};                               // 0x24


// ---- the manager's message at +0x780 ---------------------------------- //
//
// servermsg.cpp's ServerMessage at the layout clientmgr3.cpp gives it
// (CCmgServerMessage) and servermsg.cpp's CServerMessageInitQ12 writes Init
// against: the real collection.h CMoArray<uint8> at +0x50, so /O2 expands
// ServerMessage::Init and the array's resizes in place, as Init
// (0x0047ff6e..0x00480055) does.  (names ours)
class CSoundMgrMsgRef
{
public:
	void Init(uint16 nMaxDataLen, uint16 nGrowSize)
	{
		m_DataLen    = 1;
		m_MaxDataLen = nMaxDataLen;
		m_Pos        = 1;
		m_Unknown3a  = 1;
		m_MsgFlags   = 0;

		m_pOwner = this;

		if (nMaxDataLen > m_Data.GetSize())
		{
			m_Data.Term();
			m_Data.Init(nMaxDataLen, nGrowSize);
		}
		else
		{
			m_Data.SetCacheSize(nGrowSize);
		}
	}

	uint8           m_pad_00[0x10];
	void*           m_pOwner;                               // 0x10
	uint8           m_pad_14[0x34 - 0x14];
	uint16          m_DataLen;                              // 0x34
	uint16          m_MaxDataLen;                           // 0x36
	uint16          m_Pos;                                  // 0x38
	uint16          m_Unknown3a;                            // 0x3a
	void*           m_pConnection;                          // 0x3c
	uint32          m_MsgFlags;                             // 0x40
	uint8           m_pad_44[0x50 - 0x44];
	CMoArray<uint8> m_Data;                                 // 0x50 .. 0x63
};                                                          // 0x64


// ---- CSoundMgr, from this file's side ------------------------------------ //
//
// Vptr at 0x00 (`mov (%esi),%edx; call *0x5c(%edx)` at 0x00481eda reads it),
// so the pad below starts at 0x04.  Everything from 0x124 down tiles exactly:
// each list, bank and array ends where the next field begins, which is the
// check that no field is missing.
class CSoundMgrView
{
public:
	// Slots 0..22, in iltsoundmgr.h's declaration order; the ones this file
	// writes are the only ones declared with a body.
	virtual LTRESULT _v00(void* pPlaySoundInfo, HLTSOUND& hResult)=0;   // PlaySound
	virtual LTRESULT _v04(HLTSOUND hSound, LTFLOAT& fDuration)=0;       // GetSoundDuration
	virtual LTRESULT IsSoundDone(HLTSOUND hSound, LTBOOL& bDone);       // +0x08
	virtual LTRESULT KillSound(HLTSOUND hSound);                        // +0x0c
	virtual LTRESULT KillSoundLoop(HLTSOUND hSound);                    // +0x10
	virtual LTRESULT GetSound3DProviderLists(Sound3DProvider*& pList,
	                                         LTBOOL bVerify);           // +0x14
	virtual LTRESULT ReleaseSound3DProviderList(Sound3DProvider* pList);// +0x18
	virtual LTRESULT _v1c(InitSoundInfo* pSoundInfo)=0;                 // InitSound
	virtual LTRESULT GetVolume(uint16& nVolume);                        // +0x20
	virtual LTRESULT SetVolume(uint16 nVolume);                         // +0x24
	virtual LTRESULT _v28(ReverbProperties* p)=0;                       // SetReverbProperties
	virtual LTRESULT _v2c(ReverbProperties* p)=0;                       // GetReverbProperties
	virtual LTRESULT _v30(HLTSOUND h, LTFLOAT f)=0;                     // SetSoundOcclusion
	virtual LTRESULT _v34(HLTSOUND h, LTFLOAT* p)=0;                    // GetSoundOcclusion
	virtual LTRESULT _v38(HLTSOUND h, LTFLOAT f)=0;                     // SetSoundObstruction
	virtual LTRESULT _v3c(HLTSOUND h, LTFLOAT* p)=0;                    // GetSoundObstruction
	virtual LTRESULT _v40(HLTSOUND h, const char* p)=0;                 // SetSoundFilter
	virtual LTRESULT _v44(HLTSOUND h, const char* p, float f)=0;        // SetSoundFilterParam
	virtual LTRESULT _v48(HLTSOUND h, const char* p, float* f)=0;       // GetSoundFilterParam
	virtual LTRESULT _v4c(uint32 n, const char** p)=0;                  // GetFilterName
	virtual LTRESULT _v50(uint32 n, const char* f, const char** p)=0;   // GetFilterParamName
	virtual LTRESULT _v54(HLTSOUND h, LTVector* p)=0;                   // SetSoundPosition
	virtual LTRESULT _v58(HLTSOUND h, LTVector* p)=0;                   // GetSoundPosition
	virtual LTRESULT PauseSounds();                                     // +0x5c
	virtual LTRESULT ResumeSounds();                                    // +0x60

	// ---- the non-virtual members this file writes -----------------------
	void      Term(LTBOOL bFull);                                // 0x00480090
	void      FreeProviderList(Sound3DProviderInternal* pList);  // 0x00480220
	LTRESULT  SetProvider(const char* pProviderName);            // 0x00480550
	LTRESULT  Alloc3DVoices();                                   // 0x00480620
	LTRESULT  Free3DVoices();                                    // 0x00480770
	LTRESULT  AllocSWVoices();                                   // 0x00480830
	LTRESULT  FreeSWVoices();                                    // 0x00480980
	LTRESULT  FreeSoundData(CSoundDataRef* pData);               // 0x00480b20
	LTRESULT  ReleaseSound(CSoundInstanceRef* pSound);           // 0x00480bd0
	LTRESULT  StopAllSounds();                                   // 0x00480ca0
	LTRESULT  Update();                                          // 0x00480d40
	LTRESULT  SetSampleUserData(void* hSample, uint32 nValue);   // 0x00481910
	CSoundInstanceRef* GetSampleUserData(void* hSample);         // 0x00481950
	LTRESULT  Set3DUserData(void* h3DSample, uint32 nValue);     // 0x00481980
	CSoundInstanceRef* Get3DUserData(void* h3DSample);           // 0x004819c0
	LTRESULT  SetStreamUserData(void* hStream, uint32 nValue);   // 0x004819f0
	void*     GetFreeSWVoice();                                  // 0x00481a30
	void      FreeSWVoice(void* hSample);                        // 0x00481bc0
	void      Free3DVoice(void* h3DSample);                      // 0x00481c10
	LTRESULT  sub_481c60();                                      // 0x00481c60
	LTRESULT  sub_481ca0();                                      // 0x00481ca0
	LTRESULT  sub_481e70();                                      // 0x00481e70
	LTRESULT  sub_481ef0();                                      // 0x00481ef0
	LTRESULT  PlaySoundFromFile(PlaySoundInfo* pInfo, void* pFileRec,
	                            uint32 nUnknown);                // 0x004816c0
	// 0x00480a40 — sndmgr2.cpp's GetSoundData: find or load the sample.
	CSoundDataRef* GetSoundData(void* pFileRec);

	// ---- members written elsewhere, or not at all -----------------------
	// 0x00480250 (name ours) — enumerate Miles' 3D providers into the
	// internal list at 0x12c.  GetSound3DProviderLists forwards to it and
	// then converts.
	LTRESULT sub_480250(Sound3DProviderInternal** ppList, LTBOOL bVerify);
	// 0x0047f9b0 — clientsoundinst.cpp's snd_SetMilesPreferences.  Reached
	// from 0x0048027b with `mov %ebp,%ecx` in front of the call, so it is a
	// member here; its body never reads `this`, which is why a free
	// function compiles to the same bytes there.
	void      SetMilesPreferences();
	// 0x0047f9e0 -- ILTClientSoundMgr::InitSound's body (clientsound.cpp's
	// CSoundMgr::InitSound validates and forwards to it).  Written below.
	LTRESULT  Init(InitSoundInfo* pSoundInfo);
	// 0x00480ce0 — clientsound.h's GetSoundInstance, already written up
	// there.  Declared with the argument order the call sites prove:
	// `push $1; push hSound; call` at 0x00482519, so the handle is first.
	CSoundInstanceRef* GetSoundInstance(HLTSOUND hSound, LTBOOL bIsPointer);

public:
	uint8  m_pad_04[0x08 - 0x04];
	// 0x008 -- the caller's InitSoundInfo, copied whole by Init
	// (`mov $0x47,%ecx; rep movsl` at 0x0047f9fd: 0x11c bytes, which
	// ends exactly at 0x124).
	InitSoundInfo m_InitSoundInfo;           // 0x008
	void*  m_pSoundSys;                       // 0x124
	uint32 m_Unknown128;                      // 0x128
	// 0x12c — the internal provider list sub_480250 builds.
	Sound3DProviderInternal* m_pProviderList; // 0x12c
	// 0x130 — the Miles HDIGDRIVER.  Every volume and software-voice call
	// goes through it, and Term closes it with AIL_waveOutClose.
	void*  m_hDigDriver;                      // 0x130
	// 0x134/0x138 — the focus pair.  0x00481e70 sets 0x134 after
	// AIL_digital_handle_release succeeds and 0x00481ef0 clears it after the
	// reacquire; 0x138 is set only when the reacquire FAILS.
	uint32 m_bHandleReleased;                 // 0x134
	uint32 m_bReacquireFailed;                // 0x138
	// 0x13c — the provider in use, a whole record copied in by SetProvider.
	Sound3DProviderInternal m_3DProvider;     // 0x13c  (0x118 bytes)
	uint8  m_nNum3DVoices;                    // 0x254  how many were opened
	uint8  m_nNum3DVoicesWanted;              // 0x255  how many were asked for
	uint8  m_pad_256[0x258 - 0x256];
	SoundVoice* m_p3DVoices;                  // 0x258
	LTList m_3DVoiceList;                     // 0x25c  count 0x25c, head 0x260
	uint8  m_nNumSWVoices;                    // 0x26c
	uint8  m_nNumSWVoicesWanted;              // 0x26d
	uint8  m_pad_26e[0x270 - 0x26e];
	SoundVoice* m_pSWVoices;                  // 0x270
	LTList m_SWVoiceList;                     // 0x274  count 0x274, head 0x278
	ObjectBankView m_SoundDataBank;           // 0x284  (0x24 bytes)
	LTList m_SoundDataList;                   // 0x2a8  count 0x2a8, head 0x2ac
	// Three instance banks, one per CSoundInstance subclass; ReleaseSound
	// picks between them on m_Type, which is 0, 1 or 2.
	ObjectBankRef<CSoundInstanceLocalRef>   m_SoundBank0;   // 0x2b8
	ObjectBankRef<CSoundInstanceAmbientRef> m_SoundBank1;   // 0x2dc
	ObjectBankRef<CSoundInstance3DRef>      m_SoundBank2;   // 0x300
	// 0x324 — every live sound, densely packed, with the count after it.
	// 0x724 - 0x324 = 0x400, so the array is 256 entries.
	CSoundInstanceRef* m_pSoundInstances[256];// 0x324
	uint32 m_nSoundInstances;                 // 0x724
	void*  m_h3DListener;                     // 0x728
	// 0x72c..0x774 — the listener block; sndmgr2.cpp's CSoundMgrQ480
	// justifies every vector.  Update (0x00480d40) is what fills them each
	// frame when the listener is the client object; Init hands the two
	// orientation vectors to AIL_set_3D_orientation (0x0047fd3a..).
	LTVector m_vListenerPos;                  // 0x72c
	LTVector m_vLastListenerPos;              // 0x738
	LTVector m_vListenerVelocity;             // 0x744
	LTVector m_vListenerForward;              // 0x750  Miles' XYZFace
	LTVector m_vListenerRight;                // 0x75c
	LTVector m_vListenerUp;                   // 0x768  Miles' XYZUp
	// 0x774 — the listener is inside the client object (sndmgr2.cpp's
	// m_bListenerInClient); PlaySoundFromFile reads it at 0x0048172b.
	LTBOOL m_bListenerInClient;               // 0x774
	// 0x778 -- InitSoundInfo::m_fDistanceFactor, taken by Init only when
	// positive; Update divides it by the frame's milliseconds to scale the
	// listener velocity (0x00480eef).
	LTFLOAT m_fDistanceFactor;                // 0x778
	// 0x77c — a byte count of the sounds holding a Miles voice this frame;
	// Init and Update zero it (0x004810b1), Update bumps it per such sound
	// (0x00481342).
	uint8  m_nSoundsPlaying;                  // 0x77c
	uint8  m_pad_77d[0x780 - 0x77d];
	// 0x780 -- the sound manager's own message (clientmgr3.cpp's
	// CCmgServerMessage); see CSoundMgrMsgRef above.  Update reaches the
	// same object as a packet.h CPacket (same m_DataLen/m_Pos/m_Data
	// offsets) for the inline WriteType<uint16> its slot writes use.
	CSoundMgrMsgRef m_Packet;                 // 0x780
	// 0x7e4 -- INITSOUNDINFOFLAG_CONVERT16TO8, as Init found it.
	uint32 m_bConvert16To8;                   // 0x7e4
	// 0x7e8 / 0x7ec -- clientsound.h's m_bReverbA/m_bReverbB; Init sets
	// 0x7ec when the 3D provider answers a room type, and Update lets a
	// non-3D sound flagged 0x100 take a 3D voice when it is set (0x004811f9).
	uint32 m_bReverbA;                        // 0x7e8
	uint32 m_bReverbB;                        // 0x7ec
	uint8  m_pad_7f0[0x804 - 0x7f0];
	uint32 m_dwLastUpdateTime;                // 0x804  AIL_ms_count at Init, then at each Update
	uint32 m_dwNextCommitTime;                // 0x808  pushed 100 ms on each time it passes
	uint32 m_bCommitChanges;                  // 0x80c  push orientation to Miles this frame
};


// ---- the engine's own helpers -------------------------------------------- //

// servermgr.h's pair, at the client's addresses.
void* dalloc(unsigned long size);                            // 0x004290b0
void  dfree(void* ptr);                                      // 0x004290c0

// 0x00490c20 — StructBank's page grower.  Reached cdecl with the bank and its
// cache size (0x00480a9d).  Declared but not called from this file.
// (declared above, beside sb_AllocateRef)


// ---- Miles ---------------------------------------------------------------- //
//
// Names and stack sizes are the import table's own.
extern "C" {
__declspec(dllimport) void   __stdcall AIL_lock(void);
__declspec(dllimport) void   __stdcall AIL_unlock(void);
__declspec(dllimport) int    __stdcall AIL_open_3D_provider(void* hProvider);
__declspec(dllimport) void   __stdcall AIL_set_digital_master_volume(void* hDig,
                                                                     int nLevel);
__declspec(dllimport) int    __stdcall AIL_digital_master_volume(void* hDig);
__declspec(dllimport) void*  __stdcall AIL_allocate_3D_sample_handle(void* hProvider);
__declspec(dllimport) void   __stdcall AIL_release_3D_sample_handle(void* hSample);
__declspec(dllimport) void*  __stdcall AIL_allocate_sample_handle(void* hDig);
__declspec(dllimport) void   __stdcall AIL_release_sample_handle(void* hSample);
__declspec(dllimport) void   __stdcall AIL_set_sample_user_data(void* hSample,
                                                                uint32 nIndex,
                                                                uint32 nValue);
__declspec(dllimport) uint32 __stdcall AIL_sample_user_data(void* hSample,
                                                            uint32 nIndex);
__declspec(dllimport) void   __stdcall AIL_set_3D_user_data(void* hSample,
                                                            uint32 nIndex,
                                                            uint32 nValue);
__declspec(dllimport) uint32 __stdcall AIL_3D_user_data(void* hSample,
                                                        uint32 nIndex);
__declspec(dllimport) void   __stdcall AIL_set_stream_user_data(void* hStream,
                                                               uint32 nIndex,
                                                               uint32 nValue);
__declspec(dllimport) int    __stdcall AIL_digital_handle_release(void* hDig);
__declspec(dllimport) int    __stdcall AIL_digital_handle_reacquire(void* hDig);
__declspec(dllimport) uint32 __stdcall AIL_ms_count(void);
__declspec(dllimport) void   __stdcall AIL_3D_orientation(void* hObj,
                                                          float* pFX, float* pFY,
                                                          float* pFZ, float* pUX,
                                                          float* pUY, float* pUZ);
__declspec(dllimport) void   __stdcall AIL_set_3D_orientation(void* hObj,
                                                              float fFX, float fFY,
                                                              float fFZ, float fUX,
                                                              float fUY, float fUZ);
}


// ======================================================================== //
//  The 3D provider list
// ======================================================================== //

// 0x00480090 — CSoundMgr::Term, 398 bytes, `ret $4`.  clientmgr3.cpp's
// CClientMgr destructor calls it with LTTRUE (0x00410f5f), ~CSoundMgr with
// LTTRUE too (0x0047f8ff).
//
// Read off the bytes.  Only a full Term releases the sounds: every live one
// is released (reported first, at debug level 2, if the game still owns the
// handle -- by its sample's file name when it has one), and because
// ReleaseSound moves the last sound into the freed slot the index steps
// back; then the three instance banks and, after the sample list is
// emptied, the sample bank are Term'd through slot 3 of their tables.  Then,
// either way, both voice pools, the listener, the 3D provider and the
// digital driver go, each under AIL_lock, and Miles is shut down.
//
// THE SAMPLE-LIST WALK advances only when a link has a payload
// (0x00480150..0x0048015c: the `mov %edi,%eax` is inside the `if`), so it is
// written that way.
struct CSoundFileRecRef
{
	uint8 m_pad_00[0x1c];
	char* m_pFilename;                      // 0x1c
};

// The four banks' tables, as far as Term reaches them: slot 3.
class CBankSlotsRef
{
public:
	virtual void _v00()=0;
	virtual void _v04()=0;
	virtual void _v08()=0;
	virtual void Term()=0;                   // +0x0c
};

extern "C" {
__declspec(dllimport) void   __stdcall AIL_close_3D_listener(void* hListener);
__declspec(dllimport) void   __stdcall AIL_close_3D_provider(void* hProvider);
__declspec(dllimport) void   __stdcall AIL_waveOutClose(void* hDig);
__declspec(dllimport) void   __stdcall AIL_shutdown(void);
}

void dsi_ServerError(char* pFormat, ...);                    // 0x0042e6f0
extern int g_DebugLevel;                                     // 0x004b0ab0

void CSoundMgrView::Term(LTBOOL bFull)
{
	uint32 i;
	CSoundInstanceRef* pSound;
	LTLink* pCur;
	LTLink* pNext;

	if (bFull)
	{
		for (i = 0; i < m_nSoundInstances; i++)
		{
			pSound = m_pSoundInstances[i];
			if (pSound)
			{
				if ((pSound->m_Flags10 & 0x10) && g_DebugLevel >= 2)
				{
					if (pSound->m_pData)
						dsi_ServerError("Unfreed sound handle %s", pSound->m_pData->m_pFileRec->m_pFilename);
					else
						dsi_ServerError("Unfreed sound handle 0x%x", pSound);
				}

				ReleaseSound(pSound);
				i--;
			}
		}

		m_nSoundInstances = 0;

		((CBankSlotsRef*)&m_SoundBank0)->Term();
		((CBankSlotsRef*)&m_SoundBank1)->Term();
		((CBankSlotsRef*)&m_SoundBank2)->Term();

		pCur = m_SoundDataList.m_Head.m_pNext;
		while (pCur != &m_SoundDataList.m_Head)
		{
			pNext = pCur->m_pNext;

			if (pCur->m_pData)
			{
				FreeSoundData((CSoundDataRef*)pCur->m_pData);
				pCur = pNext;
			}
		}

		dl_InitList(&m_SoundDataList);

		((CBankSlotsRef*)&m_SoundDataBank)->Term();
	}

	Free3DVoices();
	FreeSWVoices();

	if (m_h3DListener)
	{
		AIL_lock();
		AIL_close_3D_listener(m_h3DListener);
		AIL_unlock();
		m_h3DListener = LTNULL;
	}

	if (m_3DProvider.m_hProvider)
	{
		AIL_lock();
		AIL_close_3D_provider(m_3DProvider.m_hProvider);
		AIL_unlock();
		m_3DProvider.m_hProvider = LTNULL;
	}

	if (m_hDigDriver)
	{
		AIL_lock();
		AIL_waveOutClose(m_hDigDriver);
		AIL_unlock();
		m_hDigDriver = LTNULL;
		m_bHandleReleased = 0;
		m_bReacquireFailed = 0;
	}

	AIL_shutdown();

	m_pSoundSys = LTNULL;
	m_Unknown128 = 0;
}


// 0x00480220 — throw away the INTERNAL provider list.  What identifies it as
// the internal one and not the public one is the stride: the walk steps
// through +0x114, which is Sound3DProviderInternal's next pointer;
// ReleaseSound3DProviderList below is the same walk over +0x110.
//
// Return type void, and the bytes say so: the null path at 0x0048023e falls
// straight into `ret $4` without loading eax, so there is nothing to return.
// The test at the top is the while's own pre-test — VC6 emits it once and then
// rotates the loop — not a guard.
void CSoundMgrView::FreeProviderList(Sound3DProviderInternal* pList)
{
	Sound3DProviderInternal* pNext;

	while (pList)
	{
		pNext = pList->m_pNextProvider;
		dfree(pList);
		pList = pNext;
	}
}


// ---- sub_480250 --------------------------------------------------------- //

// The Miles entries sub_480250 reaches (import-table names).
extern "C" {
__declspec(dllimport) int    __stdcall AIL_startup(void);
__declspec(dllimport) void   __stdcall AIL_shutdown(void);
__declspec(dllimport) int    __stdcall AIL_waveOutOpen(void** phDig, void* phWaveOut,
                                                       int nDeviceID, void* pFormat);
__declspec(dllimport) void   __stdcall AIL_waveOutClose(void* hDig);
__declspec(dllimport) int    __stdcall AIL_enumerate_3D_providers(uint32* pNext,
                                                                  void** phProvider,
                                                                  char** ppName);
__declspec(dllimport) void   __stdcall AIL_3D_provider_attribute(void* hProvider,
                                                                 const char* pName,
                                                                 void* pValue);
__declspec(dllimport) void   __stdcall AIL_close_3D_provider(void* hProvider);
}

// 0x0048de00 — mss.h's MSS_auto_cleanup: `push $0x0048de10; call atexit`
// and `return 0`.  mss.h's AIL_startup() macro is `(MSS_auto_cleanup(),
// AIL_startup())`, which is why a call with no arguments sits in front of
// the import (0x0048026e).
extern "C" int __cdecl MSS_auto_cleanup(void);

// 0x00480250 — build the internal 3D provider list, once: with no sound
// system up yet, Miles is started on a temporary wave-out driver for the
// duration and shut down again after.  Each provider Miles enumerates is
// named to a SOUND3DPROVIDERID_ by an if/else chain of inline strcmps (the
// last pair is a ternary: `setne; add $6`), probed for EAX support when
// bVerify asks (a provider that will not open is skipped), and pushed on
// the FRONT of the list at 0x12c.  The list is handed back either way.
LTRESULT CSoundMgrView::sub_480250(Sound3DProviderInternal** ppList, LTBOOL bVerify)
{
	void*   hDig;
	uint32  nNext;
	void*   hProvider;
	char*   pName;
	uint32  bEAX;
	int32   nEnvironment;
	uint32  dwProviderID;
	Sound3DProviderInternal* pNew;
	char*   pDst;

	if (!m_pProviderList)
	{
		if (!m_pSoundSys)
		{
			MSS_auto_cleanup();
			AIL_startup();
			SetMilesPreferences();

			AIL_lock();
			if (AIL_waveOutOpen(&hDig, LTNULL, -1, LTNULL))
			{
				AIL_unlock();
				AIL_shutdown();
				return LT_ERROR;
			}
			AIL_unlock();
		}

		nNext = 0;

		AIL_lock();

		while (AIL_enumerate_3D_providers(&nNext, &hProvider, &pName))
		{
			bEAX = LTFALSE;

			if (strcmp(pName, "Microsoft DirectSound3D with Creative Labs EAX(TM)") == 0)
				dwProviderID = SOUND3DPROVIDERID_DS3D_HARDWARE_EAX;
			else if (strcmp(pName, "Aureal A3D Interactive(TM)") == 0)
				dwProviderID = SOUND3DPROVIDERID_A3D;
			else if (strcmp(pName, "Microsoft DirectSound3D hardware support") == 0)
				dwProviderID = SOUND3DPROVIDERID_DS3D_HARDWARE;
			else if (strcmp(pName, "Microsoft DirectSound3D software emulation") == 0)
				dwProviderID = SOUND3DPROVIDERID_DS3D_SOFTWARE;
			else if (strcmp(pName, "Intel Realistic Sound Experience(TM)") == 0)
				dwProviderID = SOUND3DPROVIDERID_INTEL_RSX;
			else
				dwProviderID = strcmp(pName, "Miles 2D Stereo Positional Audio") == 0
				             ? SOUND3DPROVIDERID_MILES3D : SOUND3DPROVIDERID_UNKNOWN;

			if (bVerify)
			{
				if (AIL_open_3D_provider(hProvider))
					continue;

				AIL_3D_provider_attribute(hProvider, "EAX environment selection",
				                          &nEnvironment);
				if (nEnvironment != -1)
					bEAX = LTTRUE;

				AIL_close_3D_provider(hProvider);
			}

			pNew = (Sound3DProviderInternal*)dalloc(sizeof(Sound3DProviderInternal));
			pNew->m_pNextProvider = m_pProviderList;
			m_pProviderList = pNew;
			pNew->m_dwCaps = bEAX;
			m_pProviderList->m_dwProviderID = dwProviderID;
			pNew->m_hProvider = hProvider;

			pDst = pNew->m_szProvider;
			strncpy(pDst, pName, _MAX_PATH);
			pDst[_MAX_PATH] = 0;
		}

		AIL_unlock();

		if (!m_pSoundSys)
		{
			AIL_lock();
			AIL_waveOutClose(hDig);
			AIL_unlock();
			AIL_shutdown();
		}
	}

	*ppList = m_pProviderList;

	return LT_OK;
}

// ---- Init (0x0047f9e0) --------------------------------------------------- //

// server/dsi.cpp's "ForceNoSound" console variable (0x004b3240 here: its
// server address 0x10058468 moved by the zeroed run's offset).
extern int g_CV_ForceNoSound;

// struct_bank.h's pair, on the real names (ObjectBank<T>::Init expands to
// exactly these two calls).
struct StructBank_t;
int  sb_Init2(StructBank_t* pBank, unsigned long structSize,
              unsigned long cacheSize, unsigned long nPreallocations); // 0x00490ba0
void sb_Term2(StructBank_t* pBank, int bCheckObjects);                 // 0x00490bd0

extern "C" {
__declspec(dllimport) uint32 __stdcall AIL_set_preference(uint32 nPreference, int nValue);
__declspec(dllimport) int    __stdcall AIL_get_preference(uint32 nPreference);
__declspec(dllimport) void*  __stdcall AIL_open_3D_listener(void* hProvider);
__declspec(dllimport) void   __stdcall AIL_set_3D_position(void* hObj, float fX,
                                                           float fY, float fZ);
__declspec(dllimport) void   __stdcall AIL_set_3D_velocity_vector(void* hObj, float dX,
                                                                  float dY, float dZ);
__declspec(dllimport) void   __stdcall AIL_set_3D_orientation(void* hObj,
                                                              float xFace, float yFace,
                                                              float zFace, float xUp,
                                                              float yUp, float zUp);
__declspec(dllimport) int    __stdcall AIL_3D_room_type(void* hProvider);
__declspec(dllimport) uint32 __stdcall AIL_ms_count(void);
}

// mmsystem.h's WAVEFORMATEX (pshpack1: 18 bytes), which windows.h would
// bring in; restated because this TU does not include it.
#pragma pack(push, 1)
struct SoundWaveFormatRef
{
	uint16 wFormatTag;
	uint16 nChannels;
	uint32 nSamplesPerSec;
	uint32 nAvgBytesPerSec;
	uint16 nBlockAlign;
	uint16 wBitsPerSample;
	uint16 cbSize;
};
#pragma pack(pop)

#define SOUNDMGR_DIG_MIXER_CHANNELS 1    // mss.h's DIG_MIXER_CHANNELS
#define SOUNDMGR_MAX_VOICES         256

// 0x0047f9e0, 1704 bytes, `ret $4` -- bring the sound system up from an
// InitSoundInfo.  ILTClientSoundMgr::InitSound (clientsound.cpp) checks the
// pointer and forwards here.
//
// Read off the bytes.  ForceNoSound makes it a successful no-op.  The info is
// kept whole at +0x008.  Unless the game asked for its sounds to be RELOADED
// (and there are sounds to reload), everything goes: Term(LTTRUE).  A reload
// first detaches every live sound (0x0047e360) and releases those that
// answer, and frees every sample its +0x10 test picks, then Term(LTFALSE)
// keeps the rest.  Miles is started, the mixer channel preference set from
// the software voice count (clamped), and the digital driver opened on a
// 16-bit-capable PCM format built from the info (failure: LT_UNABLETOINITSOUND).
// The provider list is built, the named 3D provider opened
// (LT_NO3DSOUNDPROVIDER if it will not), and the voice counts settled: 3D
// voices from what the provider supports, software voices from what is left
// of the mixer's channels (four kept back) and of the 256.  With a provider a
// listener is opened at the origin, and a provider that knows room types is
// reported as reverb-capable.  The banks are reset (full) or the survivors
// reattached (reload); then the distance factor, the volume, a default
// reverb when reverb is available, the message buffer and the start time.
//
// Every 1 the function stores or pushes before the driver is opened comes out
// of one register (edi), which is why the sound-system word and the mixer
// preference number look like the loop's own index there.
LTRESULT CSoundMgrView::Init(InitSoundInfo* pSoundInfo)
{
	LTBOOL             bFull;
	uint32             i;
	uint32             nValue;
	CSoundInstanceRef* pSound;
	CSoundDataRef*     pData;
	LTLink*            pCur;
	SoundWaveFormatRef wf;
	ReverbProperties   reverb;
	int                nRoomType;

	if (g_CV_ForceNoSound)
		return LT_OK;

	m_InitSoundInfo = *pSoundInfo;
	pSoundInfo->m_dwResults = 0;

	bFull = !(pSoundInfo->m_dwFlags & INITSOUNDINFOFLAG_RELOADSOUNDS);

	if (!m_nSoundInstances)
	{
		bFull = LTTRUE;
	}
	else if (!bFull)
	{
		for (i = 0; i < m_nSoundInstances; i++)
		{
			pSound = m_pSoundInstances[i];
			if (pSound && pSound->sub_47e360())
			{
				ReleaseSound(pSound);
				i--;
			}
		}

		pCur = m_SoundDataList.m_Head.m_pNext;
		while (pCur != &m_SoundDataList.m_Head)
		{
			pData = (CSoundDataRef*)pCur->m_pData;
			pCur = pCur->m_pNext;

			if (pData && pData->_v10())
				FreeSoundData(pData);
		}
	}

	Term(bFull);

	m_pSoundSys = (void*)1;
	m_Unknown128 = 1;

	MSS_auto_cleanup();
	AIL_startup();
	SetMilesPreferences();

	pSoundInfo->m_nNumSWVoices = LTMIN(pSoundInfo->m_nNumSWVoices, SOUNDMGR_MAX_VOICES);

	AIL_lock();
	AIL_set_preference(SOUNDMGR_DIG_MIXER_CHANNELS, pSoundInfo->m_nNumSWVoices);
	AIL_unlock();

	memset(&wf, 0, sizeof(wf));
	wf.wFormatTag      = 1;     // WAVE_FORMAT_PCM
	wf.nChannels       = 2;
	wf.nSamplesPerSec  = pSoundInfo->m_nSampleRate;
	wf.wBitsPerSample  = pSoundInfo->m_nBitsPerSample;
	wf.nBlockAlign     = (wf.nChannels * wf.wBitsPerSample) >> 3;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	m_bConvert16To8 = pSoundInfo->m_dwFlags & INITSOUNDINFOFLAG_CONVERT16TO8;

	AIL_lock();
	if (AIL_waveOutOpen(&m_hDigDriver, LTNULL, -1, &wf))
	{
		AIL_unlock();
		Term(LTTRUE);
		return LT_UNABLETOINITSOUND;
	}
	AIL_unlock();

	m_bHandleReleased = 0;
	m_bReacquireFailed = 0;

	sub_480250(&m_pProviderList, LTFALSE);

	if (pSoundInfo->m_sz3DProvider[0] && SetProvider(pSoundInfo->m_sz3DProvider))
	{
		Term(LTTRUE);
		return LT_NO3DSOUNDPROVIDER;
	}

	if (m_3DProvider.m_hProvider)
	{
		AIL_lock();
		AIL_3D_provider_attribute(m_3DProvider.m_hProvider, "Maximum supported samples", &nValue);
		AIL_unlock();

		if (nValue >= 255)
			nValue = 255;

		m_nNum3DVoicesWanted = LTMIN((uint8)nValue, SOUNDMGR_MAX_VOICES);
		m_nNum3DVoicesWanted = LTMIN(m_nNum3DVoicesWanted, pSoundInfo->m_nNum3DVoices);
	}

	AIL_lock();
	nValue = AIL_get_preference(SOUNDMGR_DIG_MIXER_CHANNELS);
	AIL_unlock();

	if (nValue > 4)
		nValue -= 4;

	m_nNumSWVoicesWanted = LTMIN(pSoundInfo->m_nNumSWVoices, nValue);
	m_nNumSWVoicesWanted = LTMIN(m_nNumSWVoicesWanted, SOUNDMGR_MAX_VOICES - m_nNum3DVoicesWanted);

	Alloc3DVoices();
	AllocSWVoices();

	if (m_3DProvider.m_hProvider)
	{
		AIL_lock();
		m_h3DListener = AIL_open_3D_listener(m_3DProvider.m_hProvider);
		AIL_unlock();

		if (!m_h3DListener)
		{
			Term(LTTRUE);
			return LT_NO3DSOUNDPROVIDER;
		}

		AIL_lock();
		AIL_set_3D_position(m_h3DListener, 0.0f, 0.0f, 0.0f);
		AIL_set_3D_velocity_vector(m_h3DListener, 0.0f, 0.0f, 0.0f);
		AIL_set_3D_orientation(m_h3DListener,
		                       m_vListenerForward.x, m_vListenerForward.y, m_vListenerForward.z,
		                       m_vListenerUp.x, m_vListenerUp.y, m_vListenerUp.z);
		AIL_unlock();

		AIL_lock();
		nRoomType = AIL_3D_room_type(m_3DProvider.m_hProvider);
		AIL_unlock();

		if (nRoomType != -1)
		{
			pSoundInfo->m_dwResults |= INITSOUNDINFORESULTS_REVERB;
			m_bReverbB = LTTRUE;
		}
	}

	if (bFull)
	{
		sb_Term2((StructBank_t*)&m_SoundDataBank.m_Bank, 1);
		sb_Init2((StructBank_t*)&m_SoundDataBank.m_Bank, 0xa4, 32, 32);

		dl_InitList(&m_SoundDataList);

		sb_Term2((StructBank_t*)&m_SoundBank0.m_Bank, 1);
		sb_Init2((StructBank_t*)&m_SoundBank0.m_Bank, 0xb8, 16, 16);
		sb_Term2((StructBank_t*)&m_SoundBank1.m_Bank, 1);
		sb_Init2((StructBank_t*)&m_SoundBank1.m_Bank, 0xb8, 16, 16);
		sb_Term2((StructBank_t*)&m_SoundBank2.m_Bank, 1);
		sb_Init2((StructBank_t*)&m_SoundBank2.m_Bank, 0xd0, 32, 32);

		memset(m_pSoundInstances, 0, sizeof(m_pSoundInstances));
		m_nSoundInstances = 0;
	}
	else
	{
		pCur = m_SoundDataList.m_Head.m_pNext;
		while (pCur != &m_SoundDataList.m_Head)
		{
			pData = (CSoundDataRef*)pCur->m_pData;
			pCur = pCur->m_pNext;

			if (pData && pData->sub_47d020())
				FreeSoundData(pData);
		}

		for (i = 0; i < m_nSoundInstances; i++)
		{
			pSound = m_pSoundInstances[i];
			if (pSound && pSound->sub_47e390())
			{
				ReleaseSound(pSound);
				i--;
			}
		}
	}

	m_nSoundsPlaying = 0;

	if (pSoundInfo->m_fDistanceFactor > 0.0f)
		m_fDistanceFactor = pSoundInfo->m_fDistanceFactor;

	pSoundInfo->m_nVolume = LTMIN(pSoundInfo->m_nVolume, 100);
	SetVolume((uint8)pSoundInfo->m_nVolume);

	if (m_bReverbA || m_bReverbB)
	{
		reverb.m_dwParams     = REVERBPARAM_ALL;
		reverb.m_fVolume      = 0.0f;
		reverb.m_dwAcoustics  = 0;
		reverb.m_fReflectTime = 0.0f;
		reverb.m_fDecayTime   = 0.1f;
		reverb.m_fDamping     = 0.99f;
		_v28(&reverb);
	}

	m_Packet.Init(0x1000, 0x1000);

	AIL_lock();
	m_dwLastUpdateTime = AIL_ms_count();
	AIL_unlock();

	m_dwNextCommitTime = 0;
	m_bCommitChanges = 0;

	return LT_OK;
}


// 0x004825a0 — vtable slot +0x14.  Two halves: ask sub_480250 to build the
// internal list, then copy it into fresh public records.  The copy is
// front-inserting (each new record takes the old head as its next and becomes
// the head), so the public list comes out in the reverse of the internal one.
//
// FRAME NOTE.  The shipped function reserves no locals at all — four pushes
// and nothing else — yet it needs somewhere to put the internal list head.
// It uses the first parameter's own home slot: `mov 0x8(%esp),%ebx` copies the
// reference into ebx at 0x004825a5 and `lea 0x8(%esp),%edx` on the very next
// instruction hands that now-dead slot to sub_480250 as its out-parameter.
// This is the documented lever, not source that assigns to its own parameter.
// The walk itself runs on a separate cursor (pCur, esi at 0x004825c8) because
// pInternal is address-taken and would otherwise be re-read from memory every
// iteration; the name buffer is a named pointer (ebp, 0x004825e8) that both
// the strncpy and the terminator store go through.  The row extent is 136
// bytes (0x004825a0..0x00482627); the csv used to cut it at 39.
LTRESULT CSoundMgrView::GetSound3DProviderLists(Sound3DProvider*& pSound3DProviderList,
                                                LTBOOL bVerify)
{
	Sound3DProviderInternal* pInternal;
	Sound3DProviderInternal* pCur;
	Sound3DProvider*         pNew;
	char*                    pName;

	pSound3DProviderList = LTNULL;

	if (sub_480250(&pInternal, bVerify) != LT_OK)
		return LT_ERROR;

	pCur = pInternal;

	while (pCur)
	{
		// No null check on the allocation; the shipped code has none either.
		pNew = (Sound3DProvider*)dalloc(sizeof(Sound3DProvider));

		pNew->m_pNextProvider = pSound3DProviderList;
		pSound3DProviderList = pNew;

		pName = pNew->m_szProvider;
		strncpy(pName, pCur->m_szProvider, _MAX_PATH);
		pName[_MAX_PATH] = 0;

		pNew->m_dwCaps = pCur->m_dwCaps;
		pNew->m_dwProviderID = pCur->m_dwProviderID;

		pCur = pCur->m_pNextProvider;
	}

	return LT_OK;
}


// 0x00482630 — vtable slot +0x18.  The public counterpart of
// FreeProviderList: the same walk, over +0x110 instead of +0x114, with a
// refusal in front of it.
//
// One residual to expect: the final `return LT_OK` has no `xor %eax,%eax` in
// the shipped build, because eax is the loop variable and is provably zero
// where the loop exits.  0x00481f35 shows the same trick in the other
// direction (a `mov $0x1` doing double duty as a field value and a return
// code), so it is this compiler's habit rather than a different source.
LTRESULT CSoundMgrView::ReleaseSound3DProviderList(Sound3DProvider* pList)
{
	Sound3DProvider* pNext;

	if (!pList)
		return LT_INVALIDPARAMS;

	while (pList)
	{
		pNext = pList->m_pNextProvider;
		dfree(pList);
		pList = pNext;
	}

	return LT_OK;
}


// 0x00480550 — choose the 3D provider by name and open it.  The name match is
// VC6's inline strcmp (two characters per iteration, `sbb`/`sbb $-1` for the
// sign), and the whole matching record is copied into the manager's embedded
// one — `mov $0x46,%ecx; rep movsl` at 0x004805c7, which is a struct
// assignment of 0x118 bytes.
//
// The handle is cleared BEFORE the walk, so a name that matches nothing leaves
// it null and the test after the loop has something to refuse on.  Miles
// reports success as zero, which is why the test on AIL_open_3D_provider's
// result is inverted relative to the rest of this file.
LTRESULT CSoundMgrView::SetProvider(const char* pProviderName)
{
	Sound3DProviderInternal* pProvider;

	if (!m_pSoundSys)
		return LT_ERROR;

	pProvider = m_pProviderList;
	m_3DProvider.m_hProvider = LTNULL;

	while (pProvider)
	{
		if (strcmp(pProvider->m_szProvider, pProviderName) == 0)
		{
			m_3DProvider = *pProvider;
			break;
		}

		pProvider = pProvider->m_pNextProvider;
	}

	if (!m_3DProvider.m_hProvider)
		return LT_ERROR;

	AIL_lock();

	if (AIL_open_3D_provider(m_3DProvider.m_hProvider))
	{
		AIL_unlock();
		m_3DProvider.m_hProvider = LTNULL;
		return LT_ERROR;
	}

	AIL_unlock();
	return LT_OK;
}


// ======================================================================== //
//  The voice pools
// ======================================================================== //

// 0x00480620 — open as many Miles 3D sample handles as were asked for and put
// them all on the free list.  Each voice gets two pieces of user data: index 1
// set to 1 (a "this is one of ours" tag) and index 2 set to the voice's own
// address, which is how FreeSWVoice/Free3DVoice find their way back from a
// handle to a voice.
//
// The array is `new SoundVoice[n]`, not dalloc: 0x0048068c calls 0x004290b0
// (operator new), and the `dec %edi; mov %edi,0x10(%esp)` at 0x00480698 that
// nothing reads is what is left of VC6's inline array-construction loop
// (count - 1, homed) once SoundVoice's empty inline constructor has been
// optimised away; the `xor %eax,%eax` arm is new's own null result.
LTRESULT CSoundMgrView::Alloc3DVoices()
{
	uint8 i;

	if (!m_pSoundSys)
		return LT_ERROR;

	dl_InitList(&m_3DVoiceList);

	if (!m_3DProvider.m_hProvider)
		return LT_ERROR;

	if (m_p3DVoices)
	{
		Free3DVoices();
		m_p3DVoices = LTNULL;
	}

	if (m_nNum3DVoicesWanted > 0)
	{
		m_p3DVoices = new SoundVoice[m_nNum3DVoicesWanted];

		AIL_lock();

		for (i = 0; i < m_nNum3DVoicesWanted; i++)
		{
			m_p3DVoices[i].m_hSample = AIL_allocate_3D_sample_handle(m_3DProvider.m_hProvider);

			if (!m_p3DVoices[i].m_hSample)
				break;

			AIL_set_3D_user_data(m_p3DVoices[i].m_hSample, 1, 1);
			AIL_set_3D_user_data(m_p3DVoices[i].m_hSample, 2, (uint32)&m_p3DVoices[i]);

			dl_AddHead(&m_3DVoiceList, &m_p3DVoices[i].m_Link, &m_p3DVoices[i]);
		}

		AIL_unlock();

		m_nNum3DVoices = i;
	}

	return LT_OK;
}


// 0x00480770 — give the 3D voices back.  The loop counts DOWN, and the form is
// pinned by three instructions at 0x004807a2: the count is read into cl, then
// decremented into al and homed, and it is CL — the value BEFORE the
// decrement — that the loop-entry test uses.  So the test precedes the
// decrement and the body indexes with the decremented value, which is the
// post-decrement `while (i-- > 0)` (the `{ i--; ... }` spelling tests the
// value after the store and came out short).  The test itself is `jbe`, unsigned `<= 0`,
// which is this compiler's rendering of `i > 0` and not of `i != 0`; the
// bottom of the loop tests the strength-reduced trip counter with `ja`, the
// same comparison.  It is the third zero test in a row and VC6 could not fold
// it into the two guards above because those test different things.
//
// A voice still attached to a sound is told about it first (the sound's
// +0x1c slot, with 1), and only then is the handle released.
LTRESULT CSoundMgrView::Free3DVoices()
{
	uint8 i;

	if (!m_3DProvider.m_hProvider)
		return LT_ERROR;

	if (m_p3DVoices && m_nNum3DVoices)
	{
		i = m_nNum3DVoices;

		while (i-- > 0)
		{
			CSoundInstanceRef* pSound;

			pSound = Get3DUserData(m_p3DVoices[i].m_hSample);

			if (pSound)
				pSound->_v1c(1);

			AIL_lock();
			AIL_release_3D_sample_handle(m_p3DVoices[i].m_hSample);
			AIL_unlock();
		}

		dfree(m_p3DVoices);
		m_p3DVoices = LTNULL;
		m_nNum3DVoices = 0;
	}

	return LT_OK;
}


// 0x00480830 — the software twin of Alloc3DVoices, against the digital driver
// instead of the 3D provider and with AIL_set_sample_user_data's index 1 set
// to 0 rather than 1 (0x00480907).  /OPT:ICF is on in this image but the two
// bodies differ in their imports and their field offsets, so they are two
// functions and are written out twice.  Same `new SoundVoice[n]` as its twin.
LTRESULT CSoundMgrView::AllocSWVoices()
{
	uint8 i;

	if (!m_pSoundSys)
		return LT_ERROR;

	dl_InitList(&m_SWVoiceList);

	if (!m_hDigDriver)
		return LT_ERROR;

	if (m_pSWVoices)
	{
		FreeSWVoices();
		m_pSWVoices = LTNULL;
	}

	if (m_nNumSWVoicesWanted > 0)
	{
		m_pSWVoices = new SoundVoice[m_nNumSWVoicesWanted];

		AIL_lock();

		for (i = 0; i < m_nNumSWVoicesWanted; i++)
		{
			m_pSWVoices[i].m_hSample = AIL_allocate_sample_handle(m_hDigDriver);

			if (!m_pSWVoices[i].m_hSample)
				break;

			AIL_set_sample_user_data(m_pSWVoices[i].m_hSample, 1, 0);
			AIL_set_sample_user_data(m_pSWVoices[i].m_hSample, 2, (uint32)&m_pSWVoices[i]);

			dl_AddHead(&m_SWVoiceList, &m_pSWVoices[i].m_Link, &m_pSWVoices[i]);
		}

		AIL_unlock();

		m_nNumSWVoices = i;
	}

	return LT_OK;
}


// 0x00480980 — the software twin of Free3DVoices.
LTRESULT CSoundMgrView::FreeSWVoices()
{
	uint8 i;

	if (!m_hDigDriver)
		return LT_ERROR;

	if (m_pSWVoices && m_nNumSWVoices)
	{
		i = m_nNumSWVoices;

		while (i-- > 0)
		{
			CSoundInstanceRef* pSound;

			pSound = GetSampleUserData(m_pSWVoices[i].m_hSample);

			if (pSound)
				pSound->_v1c(1);

			AIL_lock();
			AIL_release_sample_handle(m_pSWVoices[i].m_hSample);
			AIL_unlock();
		}

		dfree(m_pSWVoices);
		m_pSWVoices = LTNULL;
		m_nNumSWVoices = 0;
	}

	return LT_OK;
}


// 0x00481a30 — take the next free software voice off the head of the list and
// hand back its Miles handle.  The double tie-off at 0x00481b69 and
// 0x00481b7b is not a mistake in the reading: dl_RemoveAt already tie-offs
// inside dl_Remove, and the source ties off again afterwards, so the pair of
// self-stores appears twice with the element count decremented between them.
void* CSoundMgrView::GetFreeSWVoice()
{
	LTLink* pLink;

	if (m_SWVoiceList.m_nElements > 0)
	{
		pLink = m_SWVoiceList.m_Head.m_pNext;

		dl_RemoveAt(&m_SWVoiceList, pLink);
		dl_TieOff(pLink);

		return ((SoundVoice*)pLink->m_pData)->m_hSample;
	}

	return LTNULL;
}


// 0x00481bc0 — put a software voice back.  The voice is recovered from the
// handle through Miles' user-data slot 2, which is where Alloc put it, and it
// goes on the TAIL (dl_AddTail: the insert is after m_Head.m_pPrev, read as a
// value at 0x00481be3) while GetFreeSWVoice pops the head — so the pool is
// first-in, first-out.
//
// No return value: 0x00481c0b pops and returns with whatever eax holds.
void CSoundMgrView::FreeSWVoice(void* hSample)
{
	SoundVoice* pVoice;

	AIL_lock();
	pVoice = (SoundVoice*)AIL_sample_user_data(hSample, 2);
	AIL_unlock();

	if (pVoice)
		dl_AddTail(&m_SWVoiceList, &pVoice->m_Link, pVoice);
}


// 0x00481c10 — the 3D twin of FreeSWVoice.
void CSoundMgrView::Free3DVoice(void* h3DSample)
{
	SoundVoice* pVoice;

	AIL_lock();
	pVoice = (SoundVoice*)AIL_3D_user_data(h3DSample, 2);
	AIL_unlock();

	if (pVoice)
		dl_AddTail(&m_3DVoiceList, &pVoice->m_Link, pVoice);
}


// ======================================================================== //
//  Miles user-data accessors
//
//  Five near-identical wrappers, one per Miles handle type.  All five are
//  members that never touch `this` — the `ret $4`/`ret $8` and the argument
//  at 0x8(%esp) after one push are what say so, and the call sites hand them
//  `this` in ecx anyway (0x004807ce, 0x004809de).  They differ only in the
//  import they reach, so /OPT:ICF cannot fold them.
// ======================================================================== //

// 0x00481910
LTRESULT CSoundMgrView::SetSampleUserData(void* hSample, uint32 nValue)
{
	if (!hSample)
		return LT_ERROR;

	AIL_lock();
	AIL_set_sample_user_data(hSample, 0, nValue);
	AIL_unlock();

	return LT_OK;
}

// 0x00481950
CSoundInstanceRef* CSoundMgrView::GetSampleUserData(void* hSample)
{
	CSoundInstanceRef* pSound;

	if (!hSample)
		return LTNULL;

	AIL_lock();
	pSound = (CSoundInstanceRef*)AIL_sample_user_data(hSample, 0);
	AIL_unlock();

	return pSound;
}

// 0x00481980
LTRESULT CSoundMgrView::Set3DUserData(void* h3DSample, uint32 nValue)
{
	if (!h3DSample)
		return LT_ERROR;

	AIL_lock();
	AIL_set_3D_user_data(h3DSample, 0, nValue);
	AIL_unlock();

	return LT_OK;
}

// 0x004819c0
CSoundInstanceRef* CSoundMgrView::Get3DUserData(void* h3DSample)
{
	CSoundInstanceRef* pSound;

	if (!h3DSample)
		return LTNULL;

	AIL_lock();
	pSound = (CSoundInstanceRef*)AIL_3D_user_data(h3DSample, 0);
	AIL_unlock();

	return pSound;
}

// 0x004819f0
LTRESULT CSoundMgrView::SetStreamUserData(void* hStream, uint32 nValue)
{
	if (!hStream)
		return LT_ERROR;

	AIL_lock();
	AIL_set_stream_user_data(hStream, 0, nValue);
	AIL_unlock();

	return LT_OK;
}


// ======================================================================== //
//  Master volume
// ======================================================================== //

// 0x004815e0 — vtable slot +0x24.  Miles' digital master volume runs 0..127
// and the SDK's runs 0..100, so the argument is clamped and then scaled by
// 1.27f (0x004a368c).  The clamp compares in sixteen bits (`cmp $0x64,%si`),
// which is what a uint16 parameter does; the conversion goes out through the
// parameter's own home slot because `fild` needs a memory operand.
LTRESULT CSoundMgrView::SetVolume(uint16 nVolume)
{
	if (!m_pSoundSys || !m_hDigDriver)
		return LT_ERROR;

	if (nVolume > 100)
		nVolume = 100;

	AIL_lock();
	AIL_set_digital_master_volume(m_hDigDriver, (uint16)(nVolume * 1.27f));
	AIL_unlock();

	return LT_OK;
}


// 0x00481650 — vtable slot +0x20, and the inverse scale (0.787401557f at
// 0x004a3690, which is 100/127).  With no sound system the answer is a full
// hundred and LT_OK, not an error — the same "answer trivially" habit
// GetSoundDuration has.  The `push %ecx` prologue is the four bytes `fild`
// needs for Miles' integer result; there is no source local behind it.
LTRESULT CSoundMgrView::GetVolume(uint16& nVolume)
{
	if (!m_pSoundSys || !m_hDigDriver)
	{
		nVolume = 100;
		return LT_OK;
	}

	AIL_lock();
	nVolume = (uint16)(AIL_digital_master_volume(m_hDigDriver) * 0.787401557f);
	AIL_unlock();

	return LT_OK;
}


// 0x004816c0 — play one sound from a file record (name ours; vtable slot 0,
// PlaySound at 0x004822f0, is its caller).  Pick the bank from the flags
// (in-head when the listener is the attached client object, or when neither
// AMBIENT nor 3D is asked for), Init the new instance on the sample, then
// file it in the dense table; on any failure the instance goes back to the
// bank it came from, the ReleaseSound way.
//
// 0x004a9ec0 is the "SoundsPerBuffer" console variable (its row at
// 0x004aa058 names it): past that many instances on one sample the one
// sub_47d160 picks is retired.
extern int g_CV_SoundsPerBuffer;

// 0x00483630, 18 bytes -- the in-head sound's constructor: base constructor
// 0x0047d310, then the vptr 0x004a3698, then `this`.  In the original it is
// the IMPLICIT constructor: 0x00483490 (clientstreams.cpp) expands it, and
// the one call below is the site that did not, which is what emitted this
// out-of-line copy (the only body of this shape in the image).  VC6 refuses
// the earliest inline sites of a function once its per-function budget is
// spent; the expansions that spent it here
// are not known, so -- STAND-IN -- the constructor is spelled out of line.
CSoundInstanceLocalRef::CSoundInstanceLocalRef()
{
}

LTRESULT CSoundMgrView::PlaySoundFromFile(PlaySoundInfo* pInfo, void* pFileRec,
                                          uint32 nUnknown)
{
	CSoundDataRef*     pData;
	CSoundInstanceRef* pSound;
	CSoundInstanceRef* pOther;
	LTRESULT           dResult;
	uint32             nType;

	if (pInfo->m_dwFlags & PLAYSOUND_CLIENT)
		pInfo->m_hSound = LTNULL;

	if (!m_pSoundSys)
		return LT_ERROR;

	if (!m_Unknown128)
		return LT_OK;

	pData = GetSoundData(pFileRec);
	if (!pData)
		return LT_ERROR;

	if ((pInfo->m_dwFlags & PLAYSOUND_CLIENTLOCAL && m_bListenerInClient) ||
	    (!(pInfo->m_dwFlags & PLAYSOUND_AMBIENT) && !(pInfo->m_dwFlags & PLAYSOUND_3D)))
		pSound = m_SoundBank0.Allocate();
	else if (pInfo->m_dwFlags & PLAYSOUND_AMBIENT)
		pSound = m_SoundBank1.Allocate();
	else
		pSound = m_SoundBank2.Allocate();

	if (!pSound)
		return LT_ERROR;

	dResult = pSound->Init(pData, pInfo, nUnknown);
	if (dResult == LT_OK)
	{
		if (m_nSoundInstances < 256)
		{
			m_pSoundInstances[m_nSoundInstances] = pSound;
			pSound->m_nMgrIndex = m_nSoundInstances;
			m_nSoundInstances++;

			if (pData->m_nInstances > (uint32)g_CV_SoundsPerBuffer)
			{
				pOther = pData->sub_47d160();
				if (pOther)
					ReleaseSound(pOther);
			}
		}
		else
		{
			dResult = LT_ERROR;
		}
	}

	if (dResult != LT_OK)
	{
		nType = pSound->m_Type;
		pSound->Term();

		if (nType == 0)
			m_SoundBank0.Free((CSoundInstanceLocalRef*)pSound);
		else if (nType == 1)
			m_SoundBank1.Free((CSoundInstanceAmbientRef*)pSound);
		else if (nType == 2)
			m_SoundBank2.Free((CSoundInstance3DRef*)pSound);

		pInfo->m_hSound = LTNULL;
	}

	return dResult;
}


// ======================================================================== //
//  Losing and regaining the sound device
// ======================================================================== //

// 0x00481e70 — hand the digital driver back to the system.  With a 3D
// provider open there is nothing to hand back, so the sounds are merely
// paused; the call is `call *0x5c(%eax)`, which is this class's own
// PauseSounds slot.  The provider test is written negative with the pause
// in the else: the shipped build places the pause arm last, after the
// release path's own returns.
LTRESULT CSoundMgrView::sub_481e70()
{
	if (!m_hDigDriver)
		return LT_ERROR;

	if (!m_3DProvider.m_hProvider)
	{
		if (!m_bHandleReleased)
		{
			AIL_lock();

			if (!AIL_digital_handle_release(m_hDigDriver))
			{
				AIL_unlock();
				return LT_ERROR;
			}

			AIL_unlock();

			m_bHandleReleased = LTTRUE;
			m_bReacquireFailed = LTFALSE;
		}
	}
	else
	{
		PauseSounds();
	}

	return LT_OK;
}


// 0x00481ef0 — the other half.  `call *0x60(%edx)` is ResumeSounds.  As in
// sub_481e70 the provider test is written negative with the resume in the
// else: shipped places that arm last (0x00481f56), falling into the shared
// LT_OK tail.
//
// The failure arm is where this image's habit of reusing a constant already in
// a register is easiest to see: 0x00481f35 loads 1 for the LT_ERROR return and
// the very next instruction stores that same eax into m_bReacquireFailed.
LTRESULT CSoundMgrView::sub_481ef0()
{
	if (!m_3DProvider.m_hProvider)
	{
		if (m_bHandleReleased)
		{
			if (!m_hDigDriver)
				return LT_ERROR;

			AIL_lock();

			if (!AIL_digital_handle_reacquire(m_hDigDriver))
			{
				AIL_unlock();
				m_bReacquireFailed = LTTRUE;
				return LT_ERROR;
			}

			AIL_unlock();

			m_bHandleReleased = LTFALSE;
			return LT_OK;
		}
	}
	else
	{
		ResumeSounds();
	}

	return LT_OK;
}


// 0x00481f70 — vtable slot +0x5c.  Hold() is CSoundInstance's +0x20 slot,
// which clientsoundinst.cpp already named; ResumeSounds below reaches +0x24,
// ReleaseHold, and the pairing of the two is what makes those two names sound.
LTRESULT CSoundMgrView::PauseSounds()
{
	uint32 i;

	if (!m_pSoundSys)
		return LT_OK;

	for (i = 0; i < m_nSoundInstances; i++)
	{
		CSoundInstanceRef* pSound = m_pSoundInstances[i];

		if (pSound)
			pSound->Hold();
	}

	return LT_OK;
}


// 0x00481fc0 — vtable slot +0x60.
LTRESULT CSoundMgrView::ResumeSounds()
{
	uint32 i;

	if (!m_pSoundSys)
		return LT_OK;

	for (i = 0; i < m_nSoundInstances; i++)
	{
		CSoundInstanceRef* pSound = m_pSoundInstances[i];

		if (pSound)
			pSound->ReleaseHold();
	}

	return LT_OK;
}


// ======================================================================== //
//  Sound and sample lifetime
// ======================================================================== //

// 0x00480ca0 — stop every sound.  The same walk as PauseSounds over
// CSoundInstance's +0x18 slot with an argument of 1, and no sound-system
// guard: the loop simply runs zero times when there is nothing playing.
LTRESULT CSoundMgrView::StopAllSounds()
{
	uint32 i;

	for (i = 0; i < m_nSoundInstances; i++)
	{
		CSoundInstanceRef* pSound = m_pSoundInstances[i];

		if (pSound)
			pSound->Stop(1);
	}

	return LT_OK;
}


// 0x00480ce0 — clientsound.h's GetSoundInstance: turn a game handle into
// the sound behind it.  Two kinds of handle.  With bIsPointer the handle IS
// the instance, valid only while its 0x200 flag is up (0x00480cf5 tests
// the flag's byte, `testb $0x2,%ch`).  Otherwise it is a table key: 0xffff
// is "none", and the table at 0x324 is searched for a sound whose
// m_nTableSlot matches and whose 0x200 flag is CLEAR (0x00480d28 `je`
// straight to the return) — the opposite sense to the pointer arm.
CSoundInstanceRef* CSoundMgrView::GetSoundInstance(HLTSOUND hSound, LTBOOL bIsPointer)
{
	uint32 i;

	if (bIsPointer)
	{
		CSoundInstanceRef* pSound = (CSoundInstanceRef*)hSound;

		if (pSound && (pSound->m_Flags10 & 0x200))
			return pSound;
	}
	else if ((uint32)hSound != 0xffff)
	{
		for (i = 0; i < m_nSoundInstances; i++)
		{
			CSoundInstanceRef* pSound = m_pSoundInstances[i];

			if (pSound && pSound->m_nTableSlot == (uint32)hSound &&
			    !(pSound->m_Flags10 & 0x200))
				return pSound;
		}
	}

	return LTNULL;
}


// ======================================================================== //
//  0x00480d40 — CSoundMgr::Update, once a frame.
//
//  1971 bytes, 0x00480d40..0x004814f2, `ret` (no arguments; clientmgr3.cpp's
//  CCmgSoundMgr::Update is this function, aliased onto it).  Read off the
//  bytes, in order:
//
//    * with no sound system (0x124/0x128) it returns LT_ERROR; after a
//      failed reacquire (0x134 and 0x138 both set) it retries
//      (sub_481ef0);
//    * the handle packet at 0x780 is reset to one byte, and the frame's
//      milliseconds come from AIL_ms_count under AIL_lock; every 100 ms
//      m_bCommitChanges is raised;
//    * with the listener in the client, the listener follows the client
//      object (position at +0x18c, rotation at +0x9c through
//      quat_GetVectors) or is reset to the origin and the identity basis
//      when there is none;
//    * the listener velocity is the position change scaled by
//      m_f778 / frame-ms;
//    * on a commit frame the Miles listener orientation is re-sent when it
//      has drifted by more than 0.001 (squared) in face or up;
//    * every live sound hears the listener, the table is qsorted
//      (SortSoundInstances) and renumbered, then each sound waiting for a
//      voice gets one -- stealing it from the lowest-sorted sound that has
//      one when none is free -- and finally each sound is advanced by the
//      frame time; finished sounds are released, and sounds with news for
//      the game have their uint16 table slot written into the packet.
// ======================================================================== //

// What Update reaches of the client: CClientMgr+0x150c (clientmgr.h's
// m_pConnection), its +0x88 (the client object), and that object's
// ltobject_internal.h m_Rotation (0x9c) and m_Pos (0x18c).
struct CSndClientObjectRef
{
	uint8      m_pad_00[0x9c];
	LTRotation m_Rotation;                   // 0x09c
	uint8      m_pad_ac[0x18c - 0xac];
	LTVector   m_Pos;                        // 0x18c
};

struct CSndClientShellRef
{
	uint8                m_pad_00[0x88];
	CSndClientObjectRef* m_pClientObject;    // 0x088
};

struct CSndClientMgrRef
{
	uint8               m_pad_000[0x150c];
	CSndClientShellRef* m_pConnection;       // 0x150c
};

class CClientMgr;
extern CClientMgr* g_pClientMgr;             // 0x004b17e0

// The sound, as Update drives it: CSoundInstanceRef's layout with the
// vtable slots Update calls (+0x0c, +0x10, +0x14, +0x18, +0x1c, +0x50;
// clientsoundinst.cpp's CSoundInstanceView names them) and the three
// non-virtual members it calls (sndinst2.cpp's CSoundInstanceQ47C).
// Abstract and never constructed: the table's pointers are cast to it.
class CSoundInstanceUpdRef
{
public:
	virtual void     _v00()=0;
	virtual void     _v04()=0;
	virtual void     _v08()=0;
	virtual LTRESULT Get3DVoice()=0;                           // +0x0c 0x0047db20
	virtual void     Update(uint32 nFrameTime)=0;              // +0x10
	virtual void     UpdateListener(LTVector* pListenerPos)=0; // +0x14
	virtual LTRESULT Stop(int nUnknown)=0;                     // +0x18
	virtual LTRESULT _v1c(int nUnknown)=0;                     // +0x1c 0x0047d6e0
	virtual void     _v20()=0;  virtual void _v24()=0;
	virtual void     _v28()=0;  virtual void _v2c()=0;
	virtual void     _v30()=0;  virtual void _v34()=0;
	virtual void     _v38()=0;  virtual void _v3c()=0;
	virtual void     _v40()=0;  virtual void _v44()=0;
	virtual void     _v48()=0;  virtual void _v4c()=0;
	virtual LTBOOL   sub_47d3b0()=0;                           // +0x50 0x0047d3b0

	LTRESULT sub_47d860(uint32 nFrameTime);                    // 0x0047d860
	LTRESULT sub_47d9a0();                                     // 0x0047d9a0 (2D voice)
	LTRESULT sub_47dd50();                                     // 0x0047dd50 (stream)

public:
	uint32         m_Type;                   // 0x04
	CSoundDataRef* m_pData;                  // 0x08
	uint8          m_pad_0c[0x10 - 0x0c];
	uint32         m_Flags10;                // 0x10
	uint32         m_nTableSlot;             // 0x14
	uint8          m_pad_18[0x24 - 0x18];
	uint32         m_nPriority;              // 0x24
	uint8          m_pad_28[0x2c - 0x28];
	uint32         m_nUnknown2c;             // 0x2c
	uint8          m_pad_30[0x40 - 0x30];
	uint32         m_StateFlags;             // 0x40
	void*          m_hSample;                // 0x44
	void*          m_h3DSample;              // 0x48
	void*          m_hStream;                // 0x4c
	uint8          m_pad_50[0x58 - 0x50];
	uint32         m_nMgrIndex;              // 0x58
};

int __cdecl SortSoundInstances(const void* pElem1, const void* pElem2);  // 0x00481500

// INLINE-DEPTH STAND-INS (names ours).  Shipped expands the LTVector
// operator- in the drift test but CALLS its three-float constructor
// (0x00411d40) and MagSqr (0x004316b0), and it expands WriteType<uint16>
// but CALLS CMoArray::Insert2 (0x00412f50) -- in a function where the
// velocity's operator- two statements earlier expands its constructor whole.
// That is VC6's inline depth (clientinterface.cpp's
// ls_UpdateCenterQ43c is the same device): the calls appear only this far
// down.  Measured here: six forwarders over the difference, with MagSqr one
// level further down, and four over the write, give exactly the shipped
// calls; one fewer or one more on either chain
// changes what is expanded.  The original presumably reached the depth
// through helpers of its own; these chains are only the depth.
inline float snd_MagSqr(const LTVector& v) { return v.MagSqr(); }
inline float snd_Drift0(const LTVector& a, const LTVector& b) { return snd_MagSqr(a - b); }
inline float snd_Drift1(const LTVector& a, const LTVector& b) { return snd_Drift0(a, b); }
inline float snd_Drift2(const LTVector& a, const LTVector& b) { return snd_Drift1(a, b); }
inline float snd_Drift3(const LTVector& a, const LTVector& b) { return snd_Drift2(a, b); }
inline float snd_Drift4(const LTVector& a, const LTVector& b) { return snd_Drift3(a, b); }
inline float snd_Drift5(const LTVector& a, const LTVector& b) { return snd_Drift4(a, b); }
inline float snd_Drift(const LTVector& a, const LTVector& b) { return snd_Drift5(a, b); }

// uint32 through the chain, narrowed only at WriteType: the table slot is
// compared and kept as a dword (`mov 0x14(%edi),%ebx; cmp $0xffff,%ebx`)
// and only its low word is stored (`mov %bx,...` at 0x00481429).
inline void snd_WriteSlot0(CPacket* pPacket, uint32 nSlot) { pPacket->WriteType((uint16)nSlot); }
inline void snd_WriteSlot1(CPacket* pPacket, uint32 nSlot) { snd_WriteSlot0(pPacket, nSlot); }
inline void snd_WriteSlot2(CPacket* pPacket, uint32 nSlot) { snd_WriteSlot1(pPacket, nSlot); }
inline void snd_WriteSlot3(CPacket* pPacket, uint32 nSlot) { snd_WriteSlot2(pPacket, nSlot); }
inline void snd_WriteSlot(CPacket* pPacket, uint32 nSlot) { snd_WriteSlot3(pPacket, nSlot); }

LTRESULT CSoundMgrView::Update()
{
	uint32                dwCurTime;
	uint32                dwFrameTime;
	uint32                i, j;
	uint32                nSlot;
	CSoundInstanceUpdRef* pSound;
	CSoundInstanceUpdRef* pOther;
	CSndClientShellRef*   pShell;
	CSndClientObjectRef*  pObj;
	LTVector              vFace, vUp;

	if (!m_pSoundSys || !m_Unknown128)                       // 0x00480d4b
		return LT_ERROR;

	if (m_bHandleReleased && m_bReacquireFailed)             // 0x00480d63
		sub_481ef0();

	m_Packet.m_Pos = 1;                                      // 0x00480d7d
	m_Packet.m_DataLen = 1;

	AIL_lock();
	dwCurTime = AIL_ms_count();                              // 0x00480d91
	AIL_unlock();

	dwFrameTime = dwCurTime - m_dwLastUpdateTime;            // 0x00480db1
	if (dwCurTime > m_dwNextCommitTime)                      // 0x00480db3
	{
		m_bCommitChanges = LTTRUE;
		m_dwNextCommitTime = dwCurTime + 100;
	}

	// The listener follows the client object.
	if (m_bListenerInClient)                                 // 0x00480dce
	{
		pShell = ((CSndClientMgrRef*)g_pClientMgr)->m_pConnection;
		if (pShell && (pObj = pShell->m_pClientObject) != LTNULL)
		{
			m_vListenerPos = pObj->m_Pos;                    // 0x00480e04
			quat_GetVectors(pObj->m_Rotation.m_Quat, (float*)&m_vListenerRight,
			                (float*)&m_vListenerUp,
			                (float*)&m_vListenerForward);    // 0x00480e2a
		}
		else
		{
			m_vListenerPos.Init(0.0f, 0.0f, 0.0f);           // 0x00480e39
			m_vListenerRight.Init(1.0f, 0.0f, 0.0f);
			m_vListenerUp.Init(0.0f, 1.0f, 0.0f);
			m_vListenerForward.Init(0.0f, 0.0f, 1.0f);
		}
	}

	if (dwFrameTime > 0)                                     // 0x00480e81
	{
		m_vListenerVelocity = m_vListenerPos - m_vLastListenerPos;
		m_vListenerVelocity *= m_fDistanceFactor / (float)dwFrameTime;  // 0x00480eef
	}
	else
	{
		m_vListenerVelocity.Init(0.0f, 0.0f, 0.0f);          // 0x00480f15
	}

	m_vLastListenerPos = m_vListenerPos;                     // 0x00480f27

	// Re-send the orientation only when Miles' copy has drifted.
	if (m_h3DListener && m_bCommitChanges)                   // 0x00480f49
	{
		AIL_lock();
		AIL_3D_orientation(m_h3DListener, &vFace.x, &vFace.y, &vFace.z,
		                   &vUp.x, &vUp.y, &vUp.z);          // 0x00480f88

		if (snd_Drift(vFace, m_vListenerForward) > 0.001f ||
		    snd_Drift(vUp, m_vListenerUp) > 0.001f)         // 0x00480ff4/0x00481065
		{
			AIL_set_3D_orientation(m_h3DListener,
			                       m_vListenerForward.x, m_vListenerForward.y,
			                       m_vListenerForward.z,
			                       m_vListenerUp.x, m_vListenerUp.y,
			                       m_vListenerUp.z);         // 0x0048109f
		}
		AIL_unlock();
	}

	m_nSoundsPlaying = 0;                                    // 0x004810b1
	if (!m_nSoundInstances)
		return LT_OK;

	// Every live sound hears the listener.
	for (i = 0; i < m_nSoundInstances; i++)                  // 0x004810ce
	{
		pSound = (CSoundInstanceUpdRef*)m_pSoundInstances[i];
		if (pSound && !(pSound->m_StateFlags & 4))
		{
			pSound->UpdateListener(&m_vListenerPos);         // 0x004810e6
			if (!(pSound->m_StateFlags & 0x10))
				pSound->_v1c(0);                             // 0x004810f5
		}
	}

	// Sort, then renumber; a null entry ends the table.
	if (m_nSoundInstances > 1)                               // 0x0048110c
	{
		qsort(m_pSoundInstances, m_nSoundInstances, sizeof(CSoundInstanceRef*),
		      SortSoundInstances);                           // 0x00481120

		for (i = 0; i < m_nSoundInstances; i++)
		{
			if (!m_pSoundInstances[i])
			{
				m_nSoundInstances = i;                       // 0x0048114f
				break;
			}

			m_pSoundInstances[i]->m_nMgrIndex = i;           // 0x0048113c
		}
	}

	// Voices for the sounds waiting for one.
	for (i = 0; i < m_nSoundInstances; i++)                  // 0x00481173
	{
		pSound = (CSoundInstanceUpdRef*)m_pSoundInstances[i];

		if ((pSound->m_StateFlags & 4) || !(pSound->m_StateFlags & 0x10))
			continue;
		if (pSound->m_hSample || pSound->m_h3DSample || pSound->m_hStream)
			continue;
		if (!pSound->m_pData)
			continue;
		if (pSound->m_nPriority > pSound->m_nUnknown2c)      // 0x004811be
			continue;
		if (!pSound->m_pData->sub_47d1a0((CSoundInstanceRef*)pSound))  // 0x004811c7
			continue;

		if (pSound->m_pData->m_Flags28 & 1)                  // 0x004811d7
		{
			pSound->sub_47dd50();                            // 0x004811df
			continue;
		}

		if (m_nNum3DVoicesWanted &&
		    (pSound->m_Type == 2 || (m_bReverbB && (pSound->m_Flags10 & 0x100))) &&
		    !pSound->sub_47d3b0())                           // 0x0048120f
		{
			if (pSound->Get3DVoice() == LT_OK)               // 0x0048121a
				continue;
		}
		else
		{
			if (pSound->sub_47d9a0() == LT_OK)               // 0x00481229
				continue;
		}

		// None free: take one from the last sound in sort order that has one.
		for (j = m_nSoundInstances - 1; j > i; j--)          // 0x00481236
		{
			pOther = (CSoundInstanceUpdRef*)m_pSoundInstances[j];

			if (m_nNum3DVoicesWanted &&
			    (pSound->m_Type == 2 || (m_bReverbB && (pSound->m_Flags10 & 0x100))) &&
			    !pSound->sub_47d3b0())                       // 0x0048127c
			{
				if (pOther->m_h3DSample && pSound->m_Type == 2 &&
				    !(pSound->m_pData->m_Flags28 & 1))       // 0x00481283
				{
					pOther->_v1c(0);                         // 0x004812bf
					pSound->Get3DVoice();                    // 0x004812c6
					break;
				}
			}
			else if (pOther->m_hSample)                      // 0x0048129b
			{
				pOther->_v1c(0);                             // 0x004812d2
				pSound->sub_47d9a0();                        // 0x004812d7
				break;
			}
		}
	}

	// Advance every sound; release the finished ones and report the rest.
	for (i = 0; i < m_nSoundInstances; i++)                  // 0x00481320
	{
		pSound = (CSoundInstanceUpdRef*)m_pSoundInstances[i];

		if (pSound->m_h3DSample || pSound->m_hSample || pSound->m_hStream)
		{
			m_nSoundsPlaying++;                              // 0x00481342
			pSound->Update(dwFrameTime);                     // 0x0048134f
		}

		if (pSound->sub_47d860(dwFrameTime) != LT_OK)        // 0x00481359
			continue;

		pSound->Stop(0);                                     // 0x0048136b

		if ((pSound->m_StateFlags & 0x40) || !(pSound->m_Flags10 & 0x438))
		{
			ReleaseSound((CSoundInstanceRef*)pSound);        // 0x00481499
			i--;
		}
		else if (!(pSound->m_Flags10 & 0x200) && (pSound->m_Flags10 & 0x428) &&
		         !(pSound->m_Flags10 & 4) &&
		         (nSlot = pSound->m_nTableSlot) != 0xffff)   // 0x004813a2
		{
			snd_WriteSlot((CPacket*)&m_Packet, nSlot);                      // 0x004813b1
			pSound->m_Flags10 &= ~0x428;                     // 0x00481440
		}
	}

	m_bCommitChanges = LTFALSE;                              // 0x004814cc
	m_dwLastUpdateTime = dwCurTime;                          // 0x004814d6

	return LT_OK;
}


// 0x00480bd0 — retire one sound.  Three steps, and the middle one is what
// makes the manager's table dense: the last entry is moved down into the
// vacated slot and told its new index, then the tail entry is cleared and the
// count dropped.  Only then is the sound Term'd and handed back to whichever
// of the three banks its m_Type says it came from.
//
// The bank free is object_bank.h's `ObjectBank<T>::Free` inlined: an explicit
// destructor call — which for a virtual destructor is a call through slot
// +0x00 with the deleting flag 0, exactly `push $0; call *(%edx)` — followed
// by struct_bank.h's two-store sb_Free.  It is written out by hand here
// because object_bank.h is not on this target's include path.
LTRESULT CSoundMgrView::ReleaseSound(CSoundInstanceRef* pSound)
{
	uint32 nType;
	uint32 nIndex;

	nType = pSound->m_Type;
	nIndex = pSound->m_nMgrIndex;

	if (m_nSoundInstances > 1 && nIndex < m_nSoundInstances - 1)
	{
		m_pSoundInstances[nIndex] = m_pSoundInstances[m_nSoundInstances - 1];
		m_pSoundInstances[nIndex]->m_nMgrIndex = nIndex;
	}

	if (m_nSoundInstances)
	{
		m_pSoundInstances[m_nSoundInstances - 1] = LTNULL;
		m_nSoundInstances--;
	}

	pSound->Term();

	if (nType == 0)
	{
		pSound->~CSoundInstanceRef();
		*(void**)pSound = m_SoundBank0.m_Bank.m_pFreeListHead;
		m_SoundBank0.m_Bank.m_pFreeListHead = pSound;
	}
	else if (nType == 1)
	{
		pSound->~CSoundInstanceRef();
		*(void**)pSound = m_SoundBank1.m_Bank.m_pFreeListHead;
		m_SoundBank1.m_Bank.m_pFreeListHead = pSound;
	}
	else if (nType == 2)
	{
		pSound->~CSoundInstanceRef();
		*(void**)pSound = m_SoundBank2.m_Bank.m_pFreeListHead;
		m_SoundBank2.m_Bank.m_pFreeListHead = pSound;
	}

	return LT_OK;
}


// 0x00480b20 — retire one loaded sample.  The sweep in the middle is the
// interesting part: unless the sample is marked with bit 0 of +0x28, every 3D
// voice is checked to see whether Miles' user-data slot 3 still points at this
// sample, and any that does is cleared.  Slot 3 is the sample back-pointer the
// way slot 2 is the voice back-pointer.
LTRESULT CSoundMgrView::FreeSoundData(CSoundDataRef* pData)
{
	LTLink* pCur;

	dl_RemoveAt(&m_SoundDataList, &pData->m_MgrLink);

	if (!(pData->m_Flags28 & 0x01))
	{
		pCur = m_3DVoiceList.m_Head.m_pNext;

		while (pCur != &m_3DVoiceList.m_Head)
		{
			SoundVoice* pVoice = (SoundVoice*)pCur->m_pData;

			pCur = pCur->m_pNext;

			if (pVoice && pVoice->m_hSample)
			{
				if (AIL_3D_user_data(pVoice->m_hSample, 3) == (uint32)pData)
					AIL_set_3D_user_data(pVoice->m_hSample, 3, 0);
			}
		}
	}

	pData->~CSoundDataRef();
	*(void**)pData = m_SoundDataBank.m_Bank.m_pFreeListHead;
	m_SoundDataBank.m_Bank.m_pFreeListHead = pData;

	return LT_OK;
}


// 0x00481c60 — clear the +0x40 mark on every loaded sample, so that the next
// sweep (sub_481ca0) is free to throw away anything nothing asks for again.
// No frame at all in the shipped build: three scratch registers cover the
// cursor, the sample and the sentinel's address.
LTRESULT CSoundMgrView::sub_481c60()
{
	LTLink* pCur;

	if (!m_pSoundSys)
		return LT_ERROR;

	pCur = m_SoundDataList.m_Head.m_pNext;

	while (pCur != &m_SoundDataList.m_Head)
	{
		LTLink* pNext = pCur->m_pNext;

		((CSoundDataRef*)pCur->m_pData)->m_nUnknown40 = 0;

		pCur = pNext;
	}

	return LT_OK;
}


// 0x00481ca0 — the sweep.  Every sound the game does not hold a handle to
// (bit 4 of m_Flags10) is released, and then every sample that nothing marked
// and nothing is playing is freed.  The `i--` after a release is what keeps
// the walk correct across ReleaseSound's move-the-last-one-down, and it is
// also why this loop counts signed where PauseSounds' counts unsigned:
// 0x00481cc0's `jle` and 0x00481cec's `jl` are both signed, so `i` is an
// int32 here and the count is compared as one.
LTRESULT CSoundMgrView::sub_481ca0()
{
	int32   i;
	LTLink* pCur;

	if (!m_pSoundSys)
		return LT_ERROR;

	for (i = 0; i < (int32)m_nSoundInstances; i++)
	{
		CSoundInstanceRef* pSound = m_pSoundInstances[i];

		if (pSound && !(pSound->m_Flags10 & 0x10))
		{
			ReleaseSound(pSound);
			i--;
		}
	}

	pCur = m_SoundDataList.m_Head.m_pNext;

	while (pCur != &m_SoundDataList.m_Head)
	{
		CSoundDataRef* pData = (CSoundDataRef*)pCur->m_pData;

		pCur = pCur->m_pNext;

		if (pData && !pData->m_nUnknown40 && !pData->m_nInstances)
			FreeSoundData(pData);
	}

	return LT_OK;
}


// ======================================================================== //
//  The handle-level API
// ======================================================================== //

// 0x00482510 — vtable slot +0x08.  The answer defaults to "done", so a handle
// the manager cannot place reads as finished rather than as an error; only a
// live sound whose stopped bit is clear turns it off.
LTRESULT CSoundMgrView::IsSoundDone(HLTSOUND hSound, LTBOOL& bDone)
{
	CSoundInstanceRef* pSound;

	bDone = LTTRUE;

	pSound = GetSoundInstance(hSound, 1);

	if (pSound && !(pSound->m_StateFlags & 0x04))
		bDone = LTFALSE;

	return LT_OK;
}


// 0x00482540 — vtable slot +0x0c.  The refusal is a bare `mov $0x3c,%eax`
// with no console report, so it is a plain return and not RETURN_ERROR.
LTRESULT CSoundMgrView::KillSound(HLTSOUND hSound)
{
	CSoundInstanceRef* pSound;

	pSound = GetSoundInstance(hSound, 1);

	if (!pSound)
		return LT_INVALIDPARAMS;

	ReleaseSound(pSound);
	return LT_OK;
}


// 0x00482570 — vtable slot +0x10.  Nothing is released: the sound is told to
// drop its loop (0x0047e2c0) and plays out the rest of the sample.  The
// function keeps no frame at all, which is what says the call is a direct one
// on the value GetSoundInstance left in eax.
LTRESULT CSoundMgrView::KillSoundLoop(HLTSOUND hSound)
{
	CSoundInstanceRef* pSound;

	pSound = GetSoundInstance(hSound, 1);

	if (!pSound)
		return LT_INVALIDPARAMS;

	pSound->KillLoop();
	return LT_OK;
}


// ======================================================================== //
//  The sound sort
// ======================================================================== //

// 0x00481500 — a qsort comparator, and the enumeration found it the way qsort
// comparators are always found: nothing points at it but a `push imm32`.  The
// one that does is at 0x00481111, four instructions in front of
// `qsort(&m_pSoundInstances[0], m_nSoundInstances, 4, this)` at 0x00481120 —
// so the array is the manager's own table at 0x324, the element size is four,
// and the two arguments are CSoundInstance** as the double dereference at
// 0x0048150a says.
//
// Five keys in order, and the direction of each is read off the branch, not
// assumed: m_Type zero first; m_f1c descending (0x00481540's `flds A; flds B;
// fcompp` tests C0, which is set when B < A, and that arm returns -1);
// m_StateFlags bit 1 set first; and finally the signed difference
// m_nUnknown2c - m_nPriority ascending, with either side's difference coming
// out negative short-circuiting the answer.  A sound with no sample compares
// equal to everything.
//
// Shape notes, each read off the branch layout: the null-pA arm is placed last
// (0x004815d1), so the body is `if (pA) {...}`; A's m_Type ZERO is the
// fall-through arm; the float key is loaded twice and compared with fcompp,
// which is an in-class accessor returning by value (a bare member folds into
// fcomps); and both m_pData refusals share one `return 0` (0x004815cc).
// RESIDUAL (size-exact near, one byte): the float-inequality `return 1`
// (je at 0x00481562) is tail-merged in shipped into the m_Type block's
// `return 1` at 0x00481538; ours merges it into the nA < 0 one.  Tried: plain
// `!= return 1`, `== {...} return 1`, `else if`, empty-then/else, a goto to
// either copy, the `<=` nesting — none picks the first copy.
int __cdecl SortSoundInstances(const void* pElem1, const void* pElem2)
{
	CSoundInstanceRef* pA;
	CSoundInstanceRef* pB;
	int32              nA;
	int32              nB;

	pA = *(CSoundInstanceRef**)pElem1;
	pB = *(CSoundInstanceRef**)pElem2;

	if (pA)
	{
		if (!pB)
			return -1;

		if (!pA->m_Type)
		{
			if (pB->m_Type)
				return -1;
		}
		else
		{
			if (!pB->m_Type)
				return 1;
		}

		if (pA->GetF1c() > pB->GetF1c())
			return -1;

		if (pA->GetF1c() == pB->GetF1c())
		{
			if (pA->m_StateFlags & 0x02)
			{
				if (!(pB->m_StateFlags & 0x02))
					return -1;
			}
			else
			{
				if (pB->m_StateFlags & 0x02)
					return 1;
			}

			if (pA->m_pData && pB->m_pData)
			{
				nA = pA->m_nUnknown2c - pA->m_nPriority;

				if (nA < 0)
					return 1;

				nB = pB->m_nUnknown2c - pB->m_nPriority;

				if (nB < 0)
					return -1;

				if (nA < nB)
					return -1;

				return nA > nB;
			}

			return 0;
		}

		return 1;
	}

	return pB ? 1 : 0;
}


// ======================================================================== //
//  Miles' auto-cleanup hook
// ======================================================================== //

// 0x0048de00, 16 bytes -- MSS_auto_cleanup, declared above.  In the SDK it
// is a static object of mss32.lib (mss.h's AIL_startup() macro calls it
// first); the import library this build links is generated from the DLL's
// exports and has no such object, so its body is written here from the
// shipped bytes: `push $0x0048de10; call atexit; add $4,%esp; xor
// %eax,%eax; ret`.  0x0048de10 is the handler, a lone jump to the
// AIL_shutdown import thunk (0x0048de20) -- the shipped one reaches the
// thunk (`jmp rel32`); with AIL_shutdown declared dllimport above, this one
// jumps through the import slot instead.  Defined last so that nothing
// above can expand it (VC6's inliner is forward-only).
extern "C" int __cdecl atexit(void (__cdecl* pFunc)(void));

static void __cdecl MSS_auto_cleanup_shutdown(void)
{
	AIL_shutdown();
}

extern "C" int __cdecl MSS_auto_cleanup(void)
{
	atexit(MSS_auto_cleanup_shutdown);
	return 0;
}
