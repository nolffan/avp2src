// *********************************************************************** //
//
//  MODULE:   W11_480000 (.CPP)  -- SCRATCH, NOT A TRANSLATION UNIT TO ADD
//
//  PURPOSE:  Five more CSoundMgr members from the 0x00480000 block:
//
//      0x00480a40  213  CSoundMgr::GetSoundData(CluFileRec*)   (name ours)
//      0x00481a80  312  CSoundMgr::GetFree3DVoice(CSoundInstance*) (name ours)
//      0x00481d30  316  CSoundMgr::SetListener(5-arg internal)
//      0x00482a70  115  CSoundMgr::SetListener  -- vtable 0x004a356c slot 25
//      0x00482af0   51  CSoundMgr::ReleaseSoundHandle -- slot 26
//
//  INTEGRATION.  These five belong in src/engine/lithtech/clientsoundmgr.cpp,
//  which already owns this exact class and this exact address range
//  (0x00480000..0x0048265a) and already declares CSoundMgrView /
//  CSoundInstanceRef / CSoundDataRef / SoundVoice / StructBankView /
//  ObjectBankView with the same offsets used here.  This file is a SCRATCH
//  standalone TU: every view
//  below is that file's view under a Q480 name, and on integration the
//  bodies drop straight in, the five declarations join CSoundMgrView, and
//  everything between the two `---- views ----` banners is deleted except
//  the three things clientsoundmgr.cpp does not yet have:
//
//      * sb_AllocateQ480 / DummyThingForConstructorQ480 (needed by
//        GetSoundData; cltnew.cpp and sndinst2.cpp already carry the same
//        pair, verbatim from struct_bank.h / object_bank.h);
//      * CSoundFileRecQ480 (clientwin.cpp's CluFileRec, cut down);
//      * AIL_3D_position / AIL_end_3D_sample / quat_GetVectors.
//
//  WHAT NAMED THE TWO `ptr` ROWS.  They are slots 25 and 26 of the class
//  vtable at 0x004a356c, and slot order IS
//  ILTSoundMgr-then-ILTClientSoundMgr declaration order in
//  src/LT2/sdk/inc/iltsoundmgr.h -- the mapping clientsoundmgr.cpp's own header
//  comment establishes and eleven already-pinned slots confirm.  After
//  PauseSounds (slot 23, 0x00481f70) and ResumeSounds (slot 24, 0x00481fc0)
//  the header's next two declarations are
//
//      SetListener(LTBOOL, LTVector*, LTRotation*, LTBOOL)   4 args
//      ReleaseSoundHandle(HLTSOUND)                          1 arg
//
//  and the slots' `ret $N` count exactly 4 and 1 pushed arguments at
//  0x00482a70 and 0x00482af0.  ReleaseSoundHandle is confirmed a second way,
//  from its documented returns: the header says LT_INVALIDPARAMS when the
//  handle is invalid and LT_ERROR when "hSound doesn't have the GETHANDLE
//  flag set", and 0x00482af0 returns 0x3c for a handle GetSoundInstance
//  cannot place and 1 when bit 0x10 of CSoundInstance+0x10 is clear -- which
//  is the bit clientsoundmgr.cpp already calls "the game-owns-this-handle
//  bit".
//
//  NO /O1 ORACLE.  server.dll imports only KERNEL32/USER32/WINMM/WSOCK32/
//  ole32 -- no mss32.dll -- so none of these five has a server.dll twin and
//  there is no /O1 cross-check for anything below.
//
// *********************************************************************** //

#include <string.h>

#include "ltbasedefs.h"
#include "iltsoundmgr.h"
#include "ltlink.h"
#include "ltquatbase.h"


// ======================================================================== //
//  ---- views ----  (all of this is clientsoundmgr.cpp's, restated)
// ======================================================================== //

// stdlith's StructBank / ObjectBank, modelled by hand because object_bank.h
// is off this target's include path (build/engine.sh: /I LT2\sdk\inc
// /I ..\server /I .).  Offsets are clientsoundmgr.cpp's and struct_bank.h's.
struct StructBankQ480
{
	uint32 m_StructSize;                     // +0x00
	uint32 m_AlignedStructSize;              // +0x04
	uint32 m_CacheSize;                      // +0x08
	uint32 m_nPages;                         // +0x0c
	uint32 m_nTotalObjects;                  // +0x10
	void*  m_pPageHead;                      // +0x14
	void*  m_FreeListHead;                   // +0x18
};                                           // 0x1c

struct ObjectBankQ480
{
	void*          m_pVTable;                // +0x00  BaseObjectBank's vptr
	StructBankQ480 m_Bank;                   // +0x04
	uint32         m_CS;                     // +0x20  the empty CS member
};                                           // 0x24

// 0x00490c20 -- struct_bank.h's page grower, __cdecl.  clientsoundmgr.cpp,
// cltnew.cpp, sndinst2.cpp and clienttexmgr.cpp all name it at this address.
int sb_AllocateNewStructPageQ480(StructBankQ480* pBank, unsigned long nAllocations);

// struct_bank.h's own body.  `inline` and defined ABOVE its caller because
// /Ob1 expands only what is marked inline, and the shipped 0x00480a88
// has it expanded -- head, grow, RE-READ of the head, unlink.
inline void* sb_AllocateQ480(StructBankQ480* pBank)
{
	void* pRet;

	pRet = pBank->m_FreeListHead;
	if (!pRet)
	{
		sb_AllocateNewStructPageQ480(pBank, pBank->m_CacheSize);
		pRet = pBank->m_FreeListHead;
		if (!pRet)
			return 0;
	}

	pBank->m_FreeListHead = *(void**)pRet;
	return pRet;
}

// struct_bank.h's sb_Free, release body.  Taking the bank by pointer is
// what lets the free path at 0x00480ad0 reuse the bank address the
// allocate path already has in %edi (disp8 stores through it) instead of
// re-deriving this+0x2a0.
inline void sb_FreeQ480(StructBankQ480* pBank, void* pObj)
{
	*(void**)pObj = pBank->m_FreeListHead;
	pBank->m_FreeListHead = pObj;
}

// object_bank.h's placement-new hook, verbatim.  It is what turns
// ObjectBank<T>::Allocate's `::new((DummyThingForConstructor*)0, pRet) T`
// into the bare `call <ctor>` the shipped code has at 0x00480ab7.
class DummyThingForConstructorQ480
{
public:
};

inline void* operator new(size_t size, DummyThingForConstructorQ480* pThing,
                          void* ptr)
{
	return ptr;
}


// One Miles voice -- clientsoundmgr.cpp's SoundVoice.  Sixteen-byte stride,
// the sample handle at +0x00 and an LTLink at +0x04 whose m_pData points back
// at the voice.  0x00481b80 reads +0x00 off a link found on the free list and
// 0x00481b56 unlinks through +0x04, which is both fields at once.
struct SoundVoiceQ480
{
	void*  m_hSample;                        // 0x00
	LTLink m_Link;                           // 0x04
};                                           // 0x10


// The client file manager's cached record -- src/engine/lithtech/clientwin.cpp
// reconstructed it as CluFileRec (0x20 bytes) and cchandler.cpp restates it.
// Two fields are reached from here:
//
//   * +0x00, which clientwin.cpp calls m_nUnknown00 ("cleared, never
//     written") because nothing in THAT file writes it.  0x00480a5d reads it
//     and 0x00480a7d pokes +0x40 of what comes out, which is
//     CSoundData::m_nUnknown40 -- so for a sound record it is the loaded
//     sample.  cchandler.cpp reached the same conclusion for a MODEL record.
//   * +0x18, clientwin.cpp's m_nFlag: the file type the record was created
//     for, a BYTE, with 0xff meaning "matches anything" (that file's
//     three-part in-bucket compare is what proves the 0xff wildcard).  4 is
//     the sound type -- cchandler.cpp's note records `fm_FindFile(pMgr,
//     pName, 4)` at the sound call sites.
//
// The caller chain that types the argument: 0x0047b24c calls
// clu_fm_GetFileRec (0x00404680), pushes the result, calls 0x0040f7f0 (the
// manager getter, no arguments -- binkvideo.cpp, cchandler.cpp and
// soundinst2.cpp all name it) and then calls 0x00480a40 with that push still
// standing as its one argument.
struct CSoundFileRecQ480
{
	class CSoundDataQ480* m_pSoundData;      // 0x00
	uint8  m_pad_04[0x18 - 0x04];
	uint8  m_nFlag;                          // 0x18
	uint8  m_pad_19[0x20 - 0x19];
};                                           // 0x20


// The sample -- clientsoundmgr.cpp's CSoundDataRef, plus a constructor.
//
// NOT abstract, and there is a reason: 0x00480ab7 placement-news one of
// these, so the class has to be concrete.  The constructor is declared and
// NOT defined because the shipped code CALLS it out of line (`call
// 0x0047c630`); an implicit constructor would be expanded to a bare vptr
// store and the call would vanish.  sndinst2.cpp's CSoundDataQ47C does the
// same thing for the same reason at 0x0047cf06.  On integration this whole
// view is replaced by clientsoundmgr.cpp's CSoundDataRef with a constructor
// added to it.
class CSoundDataQ480
{
public:
	CSoundDataQ480();                              // 0x0047c630
	virtual ~CSoundDataQ480();                     // +0x00 0x0047c6b0
	// +0x04 -- load this sample from a file record.  clientsoundmgr.cpp's
	// view already carries the slot as `_v04(void* pFileRef)` and cites
	// 0x00480abf for it; 0x00480ac4 tests the result against zero, so it
	// returns an LTRESULT and zero is success.
	virtual LTRESULT Init(CSoundFileRecQ480* pFileRec);
	virtual void _v08();
	virtual void Free();
	virtual void _v10();
	virtual LTBOOL sub_47d020();
	virtual LTBOOL Load();

public:
	uint8  m_pad_04[0x40 - 0x04];
	// 0x40 -- the "wanted since the last sweep" mark.  0x00480a7d sets it
	// when the manager hands back a sample that is already loaded (this
	// file), 0x00481c60 clears it for every sample and 0x00481ca0 refuses to
	// free a sample whose mark is set.  Name by address; clientsoundmgr.cpp's.
	uint32 m_nUnknown40;                     // 0x40
	uint8  m_pad_44[0x98 - 0x44];
	// 0x98 -- this sample's place on the manager's list at 0x2a8.
	// 0x00480ae7 ties it on with dl_AddTail and 0x00480b34 takes it off.
	LTLink m_MgrLink;                        // 0x98
};


// One playing sound -- clientsoundmgr.cpp's CSoundInstanceRef, cut to what
// these bodies reach, with one slot added.
class CSoundInstanceQ480
{
public:
	virtual ~CSoundInstanceQ480();                         // +0x00 0x0047d3e0
	virtual void _v04()=0;      virtual void _v08()=0;
	virtual void _v0c()=0;      virtual void _v10()=0;
	virtual void _v14()=0;      virtual void _v18()=0;
	virtual void _v1c()=0;      virtual void _v20()=0;
	virtual void _v24()=0;      virtual void _v28()=0;
	virtual void _v2c()=0;      virtual void _v30()=0;
	virtual void _v34()=0;      virtual void _v38()=0;
	virtual void _v3c()=0;
	// +0x40 -- the sound's position IN MILES SPACE, i.e. relative to the
	// listener.  clientsound.h leaves this slot as `_p40` (it names +0x38
	// SetPosition and +0x3c GetPosition); what it does is read off the two
	// concrete bodies: 0x0047eea0 (streaming instance) loads the sound's own
	// position at +0x68 and subtracts the manager's m_vListenerPos at
	// +0x72c into the caller's vector, and the base at 0x0042d1b0 is
	// `xor eax,eax ; ret $4`.  That is exactly the space AIL_3D_position
	// reports in, which is why 0x00481a80 can compare the two directly.
	// (name ours)
	virtual LTRESULT GetMilesPosition(LTVector* pPos)=0;   // +0x40

public:
	uint32 m_Type;                           // 0x04
	CSoundDataQ480* m_pData;                 // 0x08
	uint8  m_pad_0c[0x10 - 0x0c];
	// 0x10 -- bit 4 (0x10) is the game-owns-this-handle bit
	// (clientsoundmgr.cpp).  ReleaseSoundHandle below is the function that
	// CLEARS it, which is what the SDK header says ReleaseSoundHandle means.
	uint32 m_Flags10;                        // 0x10
};


// CSoundMgr -- clientsoundmgr.cpp's CSoundMgrView, cut to what these five
// bodies reach.  Every offset here is that file's except the listener block
// at 0x72c..0x778, which is new and justified in place.
class CSoundMgrQ480
{
public:
	// Only the two vtable slots this file writes are declared; the 25 before
	// them are placeholders that keep the slot numbering right, and their
	// signatures are iltsoundmgr.h's.
	virtual LTRESULT _v00(PlaySoundInfo* p, HLTSOUND& h)=0;
	virtual LTRESULT _v04(HLTSOUND h, LTFLOAT& f)=0;
	virtual LTRESULT _v08(HLTSOUND h, LTBOOL& b)=0;
	virtual LTRESULT _v0c(HLTSOUND h)=0;
	virtual LTRESULT _v10(HLTSOUND h)=0;
	virtual LTRESULT _v14(Sound3DProvider*& p, LTBOOL b)=0;
	virtual LTRESULT _v18(Sound3DProvider* p)=0;
	virtual LTRESULT _v1c(InitSoundInfo* p)=0;
	virtual LTRESULT _v20(uint16& n)=0;
	virtual LTRESULT _v24(uint16 n)=0;
	virtual LTRESULT _v28(ReverbProperties* p)=0;
	virtual LTRESULT _v2c(ReverbProperties* p)=0;
	virtual LTRESULT _v30(HLTSOUND h, LTFLOAT f)=0;
	virtual LTRESULT _v34(HLTSOUND h, LTFLOAT* p)=0;
	virtual LTRESULT _v38(HLTSOUND h, LTFLOAT f)=0;
	virtual LTRESULT _v3c(HLTSOUND h, LTFLOAT* p)=0;
	virtual LTRESULT _v40(HLTSOUND h, const char* p)=0;
	virtual LTRESULT _v44(HLTSOUND h, const char* p, float f)=0;
	virtual LTRESULT _v48(HLTSOUND h, const char* p, float* f)=0;
	virtual LTRESULT _v4c(uint32 n, const char** p)=0;
	virtual LTRESULT _v50(uint32 n, const char* f, const char** p)=0;
	virtual LTRESULT _v54(HLTSOUND h, LTVector* p)=0;
	virtual LTRESULT _v58(HLTSOUND h, LTVector* p)=0;
	virtual LTRESULT _v5c()=0;                                       // PauseSounds
	virtual LTRESULT _v60()=0;                                       // ResumeSounds
	// +0x64 -- slot 25.  0x00482a70.
	virtual LTRESULT SetListener(LTBOOL bListenerInClient, LTVector* pPos,
	                             LTRotation* pRot, LTBOOL bTeleport);
	// +0x68 -- slot 26.  0x00482af0.
	virtual LTRESULT ReleaseSoundHandle(HLTSOUND hSound);

	// ---- the non-virtual members this file writes -----------------------
	CSoundDataQ480* GetSoundData(CSoundFileRecQ480* pFileRec);       // 0x00480a40
	void*           GetFree3DVoice(CSoundInstanceQ480* pSound);      // 0x00481a80
	LTRESULT        SetListener(LTBOOL bListenerInClient, LTVector* pPos,
	                            LTVector* pForward, LTVector* pRight,
	                            LTBOOL bTeleport);                   // 0x00481d30

	// ---- members written elsewhere --------------------------------------
	// 0x00480ce0 -- clientsound.h's GetSoundInstance; clientsoundmgr.cpp
	// declares it with this argument order because `push $1 ; push hSound`
	// at 0x00482519 (and 0x00482af4 here) puts the handle first.
	CSoundInstanceQ480* GetSoundInstance(HLTSOUND hSound, LTBOOL bIsPointer);

public:
	uint8  m_pad_04[0x124 - 0x04];
	void*  m_pSoundSys;                       // 0x124
	uint32 m_Unknown128;                      // 0x128
	uint8  m_pad_12c[0x25c - 0x12c];
	LTList m_3DVoiceList;                     // 0x25c  count 0x25c, head 0x260
	uint8  m_pad_26c[0x284 - 0x26c];
	ObjectBankQ480 m_SoundDataBank;           // 0x284  (0x24 bytes)
	LTList m_SoundDataList;                   // 0x2a8  count 0x2a8, head 0x2ac
	uint8  m_pad_2b8[0x72c - 0x2b8];

	// ---- the listener block ---------------------------------------------
	//
	// Six vectors and a flag, and every one of them is pinned by a reader
	// outside this file, not by guesswork:
	//
	//   * 0x72c is the listener position.  0x0047eeaf subtracts it from a
	//     sound's own position to get the Miles-space position, and
	//     0x00480f27 copies 0x72c into 0x738 at the end of the per-frame
	//     update.
	//   * 0x738 is the PREVIOUS listener position.  0x00480e8f..0x00480f10
	//     is (0x72c - 0x738) / frame-time -> 0x744, and 0x00480f15 zeroes
	//     0x744 on the other arm.  Writing 0x738 = *pPos on a teleport is
	//     exactly how that velocity is made zero for one frame.
	//   * 0x744 is that velocity.
	//   * 0x750 / 0x768 are the Face and Up vectors Miles wants:
	//     0x0048107 2..0x0048109f pushes AIL_set_3D_orientation@28 as
	//     (listener 0x728, 0x750, 0x754, 0x758, 0x768, 0x76c, 0x770), so
	//     0x750 is XYZFace and 0x768 is XYZUp.
	//   * 0x75c is the third basis vector, kept but never handed to Miles.
	//   * 0x774 is the listener-in-client flag; 0x0048172d reads it in the
	//     play path.
	LTVector m_vListenerPos;                  // 0x72c
	LTVector m_vLastListenerPos;              // 0x738
	LTVector m_vListenerVelocity;             // 0x744
	LTVector m_vListenerForward;              // 0x750
	LTVector m_vListenerRight;                // 0x75c
	LTVector m_vListenerUp;                   // 0x768
	LTBOOL   m_bListenerInClient;             // 0x774
};


// ---- Miles ------------------------------------------------------------- //
//
// Names and stack sizes are the import table's own.  The first two are
// already declared in clientsoundmgr.cpp; the last two are new.
extern "C" {
__declspec(dllimport) uint32 __stdcall AIL_3D_user_data(void* hSample,
                                                        uint32 nIndex);
__declspec(dllimport) void   __stdcall AIL_3D_position(void* hSample,
                                                       float* pX, float* pY,
                                                       float* pZ);
__declspec(dllimport) void   __stdcall AIL_end_3D_sample(void* hSample);
}


// ======================================================================== //
//  ---- views ----  end
// ======================================================================== //


// ======================================================================== //
//  0x00480a40 -- find, or load, the sample behind one file record.
//
//  213 bytes, 0x00480a40..0x00480b14, clean extent (five `ret`s, all
//  /O2-duplicated epilogues; no branch target leaves the range).  Two
//  callers, 0x0047b264 and 0x0048170c, and the second takes a null result as
//  LT_ERROR, so the return is the sample.
//
//  THREE SHAPES WORTH RECORDING
//
//  1. The type test is `&&`, not `||`.  0x00480a67 and 0x00480a6c are BOTH
//     `je` to the same label with the `return LTNULL` falling through
//     between them.  An `if (t == 4 || t == 0xff) { ... } return LTNULL;`
//     would emit `je` then `jne`; only the negated-and form emits two `je`s
//     onto the shared success label.
//
//  2. The whole allocate path is ObjectBank<CSoundData>::Allocate expanded:
//     sb_Allocate's head / grow / re-read / unlink, then the template's OWN
//     redundant null test (0x00480aae, on a value the grow path already
//     tested) and the placement new.  Both of those null tests jump to the
//     same `xor %eax,%eax` exit as the caller's own, which is why there is
//     one shared return-null and not three.
//
//  3. The bank free on the Init failure is ObjectBank<T>::Free expanded the
//     way clientsoundmgr.cpp's ReleaseSound and FreeSoundData already spell
//     it: an explicit destructor call, which for a virtual destructor is
//     slot +0x00 with the deleting flag 0 (`push $0 ; call *(%edx)`),
//     followed by struct_bank.h's two-store sb_Free.
// ======================================================================== //
CSoundDataQ480* CSoundMgrQ480::GetSoundData(CSoundFileRecQ480* pFileRec)
{
	CSoundDataQ480* pSoundData;

	if (!m_pSoundSys)                                        // 0x00480a45
		return LTNULL;                                       // 0x00480a53

	pSoundData = pFileRec->m_pSoundData;                     // 0x00480a5d

	if (pSoundData)                                          // 0x00480a60
	{
		if (pFileRec->m_nFlag != 4 && pFileRec->m_nFlag != 0xff)  // 0x00480a64
			return LTNULL;                                   // 0x00480a74

		pSoundData->m_nUnknown40 = 1;                        // 0x00480a7d
		return pSoundData;
	}

	// m_SoundDataBank.Allocate()
	pSoundData = (CSoundDataQ480*)sb_AllocateQ480(&m_SoundDataBank.m_Bank);
	if (pSoundData)                                          // 0x00480aae
		::new((DummyThingForConstructorQ480*)0, pSoundData) CSoundDataQ480;

	if (!pSoundData)
		return LTNULL;                                       // 0x00480ad8

	if (pSoundData->Init(pFileRec) != LT_OK)                 // 0x00480ac1
	{
		// m_SoundDataBank.Free(pSoundData)
		pSoundData->~CSoundDataQ480();                       // 0x00480ace
		sb_FreeQ480(&m_SoundDataBank.m_Bank, pSoundData);   // 0x00480ad0

		return LTNULL;
	}

	dl_AddTail(&m_SoundDataList, &pSoundData->m_MgrLink, pSoundData);  // 0x00480ae1

	return pSoundData;                                       // 0x00480b0d
}


// ======================================================================== //
//  0x00481a80 -- take a 3D voice for a sound that is about to play.
//
//  312 bytes, 0x00481a80..0x00481bb7, clean extent (one `ret`).  One
//  caller, 0x0047db36, in CSoundInstance's own start path.  The 3D twin of
//  GetFreeSWVoice (0x00481a30, already written in clientsoundmgr.cpp): both
//  pop a voice off the manager's free list, unlink it, tie it off a SECOND
//  time and return its Miles handle.  This one first tries to reuse a voice
//  that is already playing the same sample from the same place.
//
//  WHAT THE LOOP IS
//
//    * it walks BACKWARD -- 0x00481aad starts at m_Head.m_pPrev and
//      0x00481b31 steps through +0x00, which is CheapLTLink::m_pPrev.
//    * slot 3 of Miles' 3D user data is the sample back-pointer.  That is
//      clientsoundmgr.cpp's reading of FreeSoundData (0x00480b20), which
//      clears slot 3 on every voice still pointing at a sample it is about
//      to free; 0x00481ad7 compares it against the SOUND's m_pData.
//    * the candidate is remembered (0x00481aee) BEFORE the position query,
//      so a voice playing the right sample anywhere is taken when no voice
//      is playing it in the right place -- and 0x00481b3d re-tests it after
//      the loop, which is the `if (!pFound)` below.
//    * the position test is VEC_DISTSQR against 0.001f (0x004a1a54), and
//      every one of the three differences is computed ONCE with no store,
//      which is the macro after CSE and not `operator-` (that would have to
//      materialise a temporary).  The sum comes out z, y, x; /O2
//      canonicalises commutative FP sums, so that is not a source order.
//
//  0x00481abb jumps the empty-list case PAST the `if (!pFound)` test and
//  straight into its body -- pFound is provably still null there.  Same
//  merge as in w_FindPolyAtPoint; it is not a second test.
//
//  The tail runs whether or not a voice was taken (0x00481a99 reaches it),
//  and it is unguarded by the sound system because the count is zero when
//  there is nothing to do.
// ======================================================================== //
void* CSoundMgrQ480::GetFree3DVoice(CSoundInstanceQ480* pSound)
{
	SoundVoiceQ480* pVoice;
	SoundVoiceQ480* pFound;
	LTLink*         pCur;
	LTVector        vSoundPos;
	LTVector        vVoicePos;
	void*           hSample;

	pFound  = LTNULL;                                        // 0x00481a88
	hSample = LTNULL;                                        // 0x00481a8a

	if (m_3DVoiceList.m_nElements > 0)                       // 0x00481a8c
	{
		pSound->GetMilesPosition(&vSoundPos);                // 0x00481aaa

		pCur = m_3DVoiceList.m_Head.m_pPrev;                 // 0x00481aad
		while (pCur != &m_3DVoiceList.m_Head)                // 0x00481ab9
		{
			pVoice = (SoundVoiceQ480*)pCur->m_pData;         // 0x00481ac1

			if (pVoice)                                      // 0x00481ac4
			{
				if (AIL_3D_user_data(pVoice->m_hSample, 3)
				        == (uint32)pSound->m_pData)          // 0x00481acd
				{
					pFound = pVoice;                         // 0x00481aee

					AIL_3D_position(pVoice->m_hSample,
					                &vVoicePos.x, &vVoicePos.y,
					                &vVoicePos.z);           // 0x00481af0

					{
						// NAMED DIFFERENCES. Shipped computes the three in
						// SOURCE order at 0x00481af6/afe/b06 and squares them
						// top-down; VEC_DISTSQR's single expression evaluates
						// them backwards. The locals never leave the FPU
						// stack, so no frame slot is spent and the prologue
						// is unchanged -- which matters, since every other
						// esp displacement here is already exact.
						float dx = vSoundPos.x - vVoicePos.x;
						float dy = vSoundPos.y - vVoicePos.y;
						float dz = vSoundPos.z - vVoicePos.z;

						if (dx * dx + dy * dy + dz * dz < 0.001f)  // 0x00481b1e
							break;                                 // 0x00481b2f
					}
				}
			}

			pCur = pCur->m_pPrev;                            // 0x00481b31
		}

		if (!pFound)                                         // 0x00481b3d
		{
			pCur = m_3DVoiceList.m_Head.m_pNext;             // 0x00481b45
			if (pCur)                                        // 0x00481b4b
				pFound = (SoundVoiceQ480*)pCur->m_pData;     // 0x00481b4f
		}

		if (pFound)                                          // 0x00481b52
		{
			dl_RemoveAt(&m_3DVoiceList, &pFound->m_Link);    // 0x00481b56
			dl_TieOff(&pFound->m_Link);                      // 0x00481b7b

			hSample = pFound->m_hSample;                     // 0x00481b80
		}
	}

	if (m_3DVoiceList.m_nElements == 1)                      // 0x00481b88
	{
		pCur = m_3DVoiceList.m_Head.m_pNext;                 // 0x00481b91
		if (pCur)                                            // 0x00481b97
		{
			pVoice = (SoundVoiceQ480*)pCur->m_pData;         // 0x00481b9b
			if (pVoice)                                      // 0x00481b9e
				AIL_end_3D_sample(pVoice->m_hSample);        // 0x00481ba6
		}
	}

	return hSample;                                          // 0x00481bac
}


// ======================================================================== //
//  0x00481d30 -- the listener, from vectors.
//
//  316 bytes, 0x00481d30..0x00481e6b, clean extent (two `ret`s, the
//  LT_OK and LT_ERROR epilogues).  Five arguments (`ret $0x14`), thiscall,
//  and its ONLY caller is the vtable's SetListener at 0x00482a70 below --
//  twice, once per arm -- so it is the private overload the public one
//  forwards to.
//
//  ARGUMENT THREE IS FORWARD AND ARGUMENT FOUR IS RIGHT, and the proof is
//  outside this function: the third goes to +0x750 and the fourth to +0x75c,
//  and 0x00481072..0x0048109f hands AIL_set_3D_orientation@28 the seven
//  values (0x728, 0x750, 0x754, 0x758, 0x768, 0x76c, 0x770) -- Miles' own
//  (object, XYZFace, XYZUp).  So +0x750 is the face vector.  The caller
//  agrees from the other side: it passes quat_GetVectors' FOURTH output
//  (pForward) as argument three and its SECOND (pRight) as argument four.
//
//  THE GUARD IS ONE `||`, NOT TWO EARLY RETURNS.  0x00481d45 and 0x00481d53
//  both `je` onto a SINGLE `mov $1,%eax` epilogue that sits past the LT_OK
//  exit.  /O2 preserves duplicated epilogues, so two separate `return
//  LT_ERROR` statements would have produced two copies; one copy sunk below
//  everything is what a single `if (A || B) return` produces.
//
//  THE FIRST STORE IS THE FIRST STATEMENT.  0x00481d3d writes +0x774 BEFORE
//  0x00481d43 tests +0x124.  A memory write cannot be hoisted above a guard
//  that can return without performing it, so the assignment really is above
//  the guard in the source.
//
//  THE CROSS PRODUCT IS LTVector::Cross.  ltvector.h declares it
//  `_CVector<T> Cross(const _CVector<T> v) const` -- BY VALUE -- which is
//  why 0x00481df0..0x00481e02 copies *pRight into three stack slots before
//  multiplying, and why the result lands in three more slots before being
//  copied to +0x768.  Its body is `(v.y*z - v.z*y, v.z*x - v.x*z,
//  v.x*y - v.y*x)`, and 0x00481e06..0x00481e46 computes exactly that with
//  `this` = pForward (read through %eax, never copied) and `v` = *pRight.
// ======================================================================== //
LTRESULT CSoundMgrQ480::SetListener(LTBOOL bListenerInClient, LTVector* pPos,
                                    LTVector* pForward, LTVector* pRight,
                                    LTBOOL bTeleport)
{
	m_bListenerInClient = bListenerInClient;                 // 0x00481d3d

	if (!m_pSoundSys || !m_Unknown128)                       // 0x00481d43/0x00481d51
		return LT_ERROR;                                     // 0x00481e61

	if (!bListenerInClient)                                  // 0x00481d59
	{
		if (pPos)                                            // 0x00481d67
		{
			m_vListenerPos = *pPos;                          // 0x00481d6d

			if (bTeleport)                                   // 0x00481d87
				m_vLastListenerPos = *pPos;                  // 0x00481d8d
		}

		if (pForward)                                        // 0x00481da9
			m_vListenerForward = *pForward;                  // 0x00481daf

		if (pRight)                                          // 0x00481dcb
			m_vListenerRight = *pRight;                      // 0x00481dd0

		if (pForward && pRight)                              // 0x00481de7/0x00481dec
			m_vListenerUp = pForward->Cross(*pRight);        // 0x00481df0
	}

	return LT_OK;                                            // 0x00481e58
}


// ======================================================================== //
//  0x00482a70 -- vtable 0x004a356c slot 25, ILTClientSoundMgr::SetListener.
//
//  115 bytes, 0x00482a70..0x00482ae2, clean extent.  `ret $0x10` = four
//  arguments, which is what
//  iltsoundmgr.h declares two entries after ResumeSounds.
//
//  0x00444ac0 IS quat_GetVectors.  ltquatbase.h declares
//  `quat_GetVectors(const float*, float* pRight, float* pUp, float*
//  pForward)`; 0x00444ac0 builds a 0x40-byte matrix with quat_ConvertToMatrix
//  (0x00444880, already named by src/engine/lithtech/transform.cpp) and then
//  writes columns 0, 1 and 2 of it -- (m0,m4,m8), (m1,m5,m9), (m2,m6,m10) --
//  into its second, third and fourth arguments.  Those are the right, up and
//  forward basis vectors in that order, which is the header's order exactly.
//
//  So the 0x482a86 arm calls it as (pRot, &vRight, &vUp, &vForward) -- the
//  push order at 0x00482a8e..0x00482a95 -- and then hands the internal
//  overload &vForward as pForward and &vRight as pRight, leaving vUp unused
//  (the internal derives its own up with a cross product).
//
//  BOTH ARMS DUPLICATE THE EPILOGUE (0x00482abb and 0x00482ada), so this is
//  a genuine early return and not a shared tail.
//
//  The frame is 0x24 = three LTVectors and nothing else.
// ======================================================================== //
LTRESULT CSoundMgrQ480::SetListener(LTBOOL bListenerInClient, LTVector* pPos,
                                    LTRotation* pRot, LTBOOL bTeleport)
{
	LTVector vRight;
	LTVector vUp;
	LTVector vForward;

	if (!bListenerInClient && pRot)                          // 0x00482a7a/0x00482a82
	{
		quat_GetVectors(pRot->m_Quat, (float*)&vRight, (float*)&vUp,
		                (float*)&vForward);                  // 0x00482a96

		SetListener(LTFALSE, pPos, &vForward, &vRight, bTeleport);  // 0x00482ab6

		return LT_OK;                                        // 0x00482abb
	}

	SetListener(bListenerInClient, pPos, LTNULL, LTNULL, bTeleport);  // 0x00482ad5

	return LT_OK;                                            // 0x00482ada
}


// ======================================================================== //
//  0x00482af0 -- vtable 0x004a356c slot 26, ReleaseSoundHandle.
//
//  51 bytes, 0x00482af0..0x00482b22, clean extent (three /O2-duplicated
//  epilogues).  `ret $4`, one argument, and the two refusals match the SDK
//  header's documented returns one for one:
//
//      LT_INVALIDPARAMS  (0x3c at 0x00482b00)  hSound is invalid
//      LT_ERROR          (1    at 0x00482b10)  no GETHANDLE flag
//
//  which is what pins the slot independently of the vtable count: bit 0x10
//  of CSoundInstance+0x10 is clientsoundmgr.cpp's game-owns-this-handle bit,
//  and this is the only function in the image that clears it.
//
//  No frame at all -- GetSoundInstance is reached with `this` still in %ecx
//  and its result used straight out of %eax, exactly as KillSoundLoop
//  (0x00482570) does.
// ======================================================================== //
LTRESULT CSoundMgrQ480::ReleaseSoundHandle(HLTSOUND hSound)
{
	CSoundInstanceQ480* pSound;

	pSound = GetSoundInstance(hSound, 1);                    // 0x00482af7

	if (!pSound)                                             // 0x00482afc
		return LT_INVALIDPARAMS;                             // 0x00482b00

	if (!(pSound->m_Flags10 & 0x10))                         // 0x00482b0b
		return LT_ERROR;                                     // 0x00482b10

	pSound->m_Flags10 &= ~0x10;                              // 0x00482b18

	return LT_OK;                                            // 0x00482b1e
}
