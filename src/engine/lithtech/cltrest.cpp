// *********************************************************************** //
//
//  MODULE:   CLTREST (.CPP) -- scratch file, NOT a drop-in TU
//
//  PURPOSE:  Twenty-two of the twenty-three CLTClient vtable slots that had
//            been DECODED but never written.  The twenty-third (slot
//            71, 0x00407c00) is reported at the foot of this file and
//            deliberately NOT written: it is `??_GCLTClient`, the compiler
//            generated scalar deleting destructor, and hand-writing it is
//            the one thing that cannot reproduce it.
//
//  THE TABLE.  0x004a14e0, CLTClient's ILTClient vtable, 110 slots.  Slots
//  0..70 are iltcsbase.h in declaration order (slots 1-30 and 56-70 are
//  ILTCSBase's OWN non-pure bodies, 45 free anchors); slot 71 is the
//  destructor; slot = 71 + iltclient.h index after that, with LT2's idx
//  33..38 absent, which shifts idx 39..44 down onto slots 104..109.
//  Re-verified here against iltcsbase.h and iltclient.h by counting the
//  `virtual` declarations: every slot below lands on a declaration whose
//  argument count matches the shipped `ret $N`.
//
//      slot   addr        iltcsbase.h / iltclient.h name
//      ----   ----------  --------------------------------------------
//         0   0x00407630  StartHMessageWrite()              csbase idx 0
//        33   0x004076c0  GetModelAnimation(HOBJECT)        csbase idx 33
//        35   0x004077a0  GetModelLooping(HOBJECT)          csbase idx 35
//        37   0x00407760  GetModelPlaybackState(HOBJECT)    csbase idx 37
//        38   0x00407880  FreeUnusedModels()                csbase idx 38
//        42   0x00406610  OpenFile(char*, ILTStream**)      csbase idx 42
//        48   0x00407a80  CompareStrings(HSTRING,HSTRING)   csbase idx 48
//        49   0x00407ab0  CompareStringsUpper(...)          csbase idx 49
//        51   0x00407b00  GetVarValueFloat(HCONSOLEVAR)     csbase idx 51
//        52   0x00407b20  GetVarValueString(HCONSOLEVAR)    csbase idx 52
//        53   0x00407b40  GetTime()                         csbase idx 53
//        54   0x00407b50  GetFrameTime()                    csbase idx 54
//        71   0x00407c00  ??_GCLTClient  -- NOT WRITTEN, see the foot of this file
//        76   0x00406c30  GetServerConVarValueFloat(char*)  client idx 5
//        78   0x004071d0  GetGlobalLightDir(LTVector&)      client idx 7
//        80   0x004072c0  GetGlobalLightColor(LTVector&)    client idx 9
//        81   0x004072f0  SetGlobalLightColor(LTVector)     client idx 10
//        83   0x00407350  SetAmbientLight(float)            client idx 12
//        90   0x00407590  EndMessage2(HMESSAGEWRITE,uint32) client idx 19
//       104   0x00407810  GetModelPlaying(HLOCALOBJ)        client idx 39
//       106   0x00406950  StartQuery(char*)                 client idx 41
//       107   0x004069b0  UpdateQuery()                     client idx 42
//       108   0x00406a00  GetQueryResults(NetSession*&)     client idx 43
//
//  THREE CORRECTIONS TO THE HAND-OFF, all read straight off the bytes:
//
//   1. "the description literal is 0x4a7168" (the query trio) and
//      "description 0x4a7a4c" (OpenFile's LT_NOTFOUND site) are BOTH wrong.
//      Those two addresses hold the ASCII strings "73" and "61" -- the
//      *stringized numeric code*, i.e. de_error.h's LT_ERRSTR(code), the
//      printer's THIRD argument.  The description (fourth argument) is
//      0x004a7164 = " " = LT_ERRDESC at every one of the five error sites
//      in this file.  So every site is a plain RETURN_ERROR, and none needs
//      RETURN_ERROR_INFO / _SYM_DESC.  (Dumped: 0x4a7160 "60", 0x4a7164 " ",
//      0x4a7168 "73", 0x4a7a4c "61".)
//
//   2. "args 12450" on slots 35 and 104 -- confirmed a pop-reader artefact.
//      Both bodies really are `ret $0x4` on BOTH exits (0x4077d0/0x4077d6
//      and 0x407840/0x407846); the reader stopped on the error arm's
//      `ret` without its immediate.  Same for slot 91's "args 13218".
//
//   3. Slot 71 is indeed ??_GCLTClient and not `~ILTClient`: 0x00407c00
//      stores THREE vtables (0x004a14e0 into (this), 0x004a1a0c into
//      this+0x370, 0x004a1810 into (this) again) and ends with
//      `test $0x1,%al; je; push %esi; call 0x004290c0` -- the deleting
//      flag.  It is not written here.
//
//  GROUND TRUTH: the retail lithtech.exe ONLY, cl 8447 /O2.
//
// *********************************************************************** //

#include "ltbasedefs.h"       // LTRESULT, LTVector, HSTRING, HMESSAGEWRITE
#include "ltcodes.h"          // LT_OK, LT_YES, LT_NOTFOUND, LT_NOTINITIALIZED
#include "ltlink.h"
#include "ltmatrix.h"
#include "ltrotation.h"
#include "ltquatbase.h"
#include "iltmodel.h"         // ILTModel, LTAnimTracker
#include "de_error.h"         // RETURN_ERROR (src/engine/server)

class ILTStream;


// ======================================================================= //
//  THE SHARED VIEWS
//
//  WHAT PINS THE CLASS.  Every body below is __thiscall and reads `this` at
//  one of exactly two displacements, 0x10 and 0x3c0.
//
//    * 0x10 is m_pModelLT.  iltcsbase.h's protected block is
//      `ILTMath m_MathLT; ILTCommon *m_pCommonLT; ILTPhysics *m_pPhysicsLT;
//       ILTModel *m_pModelLT; ILTTransform *m_pTransformLT; ...`, so after
//      the vptr they land at 0x04, 0x08, 0x0c, 0x10, 0x14 -- and
//      ltnew2.cpp pinned +0x14 as m_pTransformLT off the constructor's own
//      stores, which fixes +0x10.  Independently: the two ILTModel slots
//      these bodies call, 0x28 and 0x3c/0x30/0x48/0x50, are GetMainTracker
//      and GetCurAnim / GetPlaybackState / GetLooping / GetPlaying in
//      iltmodel.h index order.
//
//    * 0x3c0 is the client manager, held BY POINTER (`mov 0x3c0(%ecx),%ecx`,
//      never `lea`).  Confirmed four independent ways in this file alone:
//      +0x1268 is the server console state (lt2new1.cpp), +0x1534 the file
//      manager (lt2new2.cpp), +0x60c is m_NetMgr(+0x50c) + m_pMainDriver
//      (+0x100), and +0x125c/+0x1258 are the two clocks that slots 53/54
//      read off the GLOBAL g_pClientMgr at 0x004b17e0.
// ======================================================================= //

// clientdsi.cpp models the console state as an opaque 0x4c bytes; only its
// ADDRESS is ever taken here.
class ConsoleStateView
{
public:
	uint8 m_pad_00[0x4c];
};

// servermgr.h's LTCommandVar: m_pStringVal at 0x14, m_FloatVal at 0x18 --
// exactly the pair slots 51/52 and 76/77 read (`mov 0x14(%eax),%eax` and
// `flds 0x18(%eax)`).
class LTCommandVarView
{
public:
	uint8  m_pad_00[0x14];
	char*  m_pStringVal;                     // 0x14
	float  m_FloatVal;                       // 0x18
};

// 0x0041b3e0, __cdecl, two arguments, the state FIRST: the shipped call
// pushes pName and then `m_pClientMgr + 0x1268`.
LTCommandVarView* cc_FindConsoleVar(ConsoleStateView* pState, char* pName);

// 0x004849c0 and 0x004849f0 -- src/engine/server/stringbank.cpp's
// sb_CompareStrings / sb_CompareStringsUpper, already reconstructed there
// and reached from the client at these two addresses (clientstreams.cpp
// records the pair).  Both are cdecl (`add $0x8,%esp` at the call site),
// take two handles, `add $0x14` to each and hand the pair to _mbscmp
// (0x00494a7d) / _mbsicmp (0x00494b2c) -- the CRT multibyte compares, each
// opening with `cmp $0,__ismbcodepage` and falling back to strcmp/stricmp
// only when no multibyte code page is set; stringbank.cpp spells them so.
// Declarations only: the bodies are stringbank.cpp's.
LTBOOL sb_CompareStrings(HSTRING hString1, HSTRING hString2);       // 0x004849c0
LTBOOL sb_CompareStringsUpper(HSTRING hString1, HSTRING hString2);  // 0x004849f0

class CFileMgrView;

// clientmgr.h's CClientFileRef, verbatim -- one type, the name, and a word
// the creator fills in.  Its constructor is what the `sub $0xc` frame in
// OpenFile is zeroed by.
class CClientFileRef
{
public:
	CClientFileRef() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// 0x004048c0 -- clientworldinfo.cpp already names it: open one
// CClientFileRef against the file manager and hand back a stream, or
// LTNULL.  cdecl, the manager first.
ILTStream* sub_4048c0(CFileMgrView* pFileMgr, CClientFileRef* pRef);

// servermgr.h's CBaseDriver, cut down to the four query slots the client
// interface reaches.  The offsets are the shipped call displacements
// (0x20/0x24/0x28/0x2c) and src/engine/server/tcpdriver.cpp already has the
// same four names at the same four slots.
class CBaseDriverView
{
public:
	virtual void     _p00() = 0;                                 // 0x00
	virtual void     _p04() = 0;                                 // 0x04
	virtual void     _p08() = 0;                                 // 0x08
	virtual void     _p0c() = 0;                                 // 0x0c
	virtual void     _p10() = 0;                                 // 0x10
	virtual void     _p14() = 0;                                 // 0x14
	virtual void     _p18() = 0;                                 // 0x18
	virtual void     _p1c() = 0;                                 // 0x1c
	virtual LTRESULT StartQuery(char* pAddresses) = 0;           // 0x20
	virtual LTRESULT UpdateQuery() = 0;                          // 0x24
	virtual LTRESULT GetQueryResults(NetSession** ppList) = 0;   // 0x28
	virtual LTRESULT EndQuery() = 0;                             // 0x2c
};


// ----------------------------------------------------------------------- //
//  The message the client's manager hands out, and the growable byte array
//  behind it.
//
//  BOTH LAYOUTS ARE THE SERVER'S, READ OFF server.dll AND ALREADY WRITTEN
//  DOWN: src/engine/server/servermsg.h's ServerMessage is this class field
//  for field (m_Write at +0x08 with m_pOwner at its own +0x08, the CPacket
//  block m_DataLen/m_MaxDataLen/m_Pos at 0x34/0x36/0x38, the spare word at
//  0x3a, m_MsgFlags at 0x40, and the CPacketData at 0x50 followed by
//  m_pData/m_DataSize/m_DataSpare/m_DataGrowBy).  Slot 0 below writes
//  exactly those, at exactly those displacements.
// ----------------------------------------------------------------------- //

// collection.h's LAlloc.  stdlithalloc.cpp: the one instance in
// lithtech.exe is g_DefAlloc at 0x004b4e88, and it is a vptr and nothing
// else (its whole constructor is `movl $0x4a3c54,0x4b4e88; ret`).
class LAlloc
{
public:
	uint8 m_vptr[4];
};

extern LAlloc g_DefAlloc;                    // 0x004b4e88

// collection.h's CMoArray<uint8>, cut to what Init touches.  The five
// fields are the ones 0x0040bfe0 itself indexes off %esi: +0x04 m_pArray,
// +0x08 m_nElements, +0x0c m_CacheSize, +0x10 m_WantedCache, with the vptr
// at +0x00.  Everything defined in the class body here is defined in the
// class body in collection.h too (Term, SetSize, SetCacheSize, GetSize);
// Init and _InitArray are collection.h's out-of-line template members,
// reproduced verbatim and moved inline because in THIS image they are
// inlined (0x0040bfe0 is the only survivor of the expansion).
class CMsgArray
{
public:
	// 0x0040bfe0 -- __thiscall, `ret $0x8`, two arguments with the
	// allocator pushed first.  collection.h's SetSize2(newSize, pAlloc):
	// the body at 0x0040bfe0 is that function arm for arm (the
	// `newSize == m_nElements && newSize != 0` early out, the
	// _DeleteAndDestroyArray through the allocator's slot 2, the
	// _AllocateTArray through slot 1, and the LTFALSE arm that zeroes
	// m_nElements and the cache).
	LTBOOL SetSize2(uint32 newSize, LAlloc* pAlloc);

	void   Term(LAlloc* pAlloc = &g_DefAlloc) { SetSize2(0, pAlloc); }
	LTBOOL SetSize(uint32 newSize) { return SetSize2(newSize, &g_DefAlloc); }
	void   SetCacheSize(uint32 size) { m_Cache.SetWantedCache(size); }
	uint32 GetSize() const { return m_nElements; }

	void   _InitArray(uint32 wantedCache)
	{
		m_pArray     = 0;
		m_nElements  = 0;
		m_Cache.SetWantedCache(wantedCache);
		m_Cache.SetCacheSize(0);
	}

	LTBOOL Init(uint32 size, uint32 cacheSize)
	{
		Term();
		_InitArray(cacheSize);

		return SetSize(size);
	}

	uint8  m_vptr[4];                        // 0x00
	uint8* m_pArray;                         // 0x04
	uint32 m_nElements;                      // 0x08
	// collection.h's DefaultCache, as a MEMBER with its own inline
	// setters rather than two bare fields: stored through the sub-object,
	// /O2 schedules the _InitArray stores between the pushes of the third
	// SetSize2 call exactly as 0x00407685..0x0040769e has them.  With bare
	// fields all four stores sank below both pushes (3 mnemonic blocks).
	class CCache
	{
	public:
		void SetCacheSize(uint32 val)   { m_CacheSize = val; }
		void SetWantedCache(uint32 val) { m_WantedCache = val; }
		uint32 m_CacheSize;                  // 0x0c
		uint32 m_WantedCache;                // 0x10
	} m_Cache;
};

// LMessageImpl as far as slot 0 and slot 90 reach it: the owning entry's
// back pointer at +0x08 and the message id at +0x1c.  lt2new3.cpp pinned
// +0x1c off StartMessage (0x00407560) writing a zero-extended uint8 there,
// and slot 90 below reads the same byte back.
class CLTMessageView
{
public:
	uint8  m_pad_00[0x08];
	class CLTMessageEntryView* m_pOwner;     // 0x08
	uint8  m_pad_0c[0x1c - 0x0c];
	uint32 m_MessageID;                      // 0x1c
	// StartHMessageWrite (0x00407630) reads m_DataLen at the entry's +0x34
	// with m_Write at +0x08, so the write interface is 0x2c bytes, not 0x20.
	// Without this every field of the entry from m_DataLen down came out
	// TWELVE bytes low -- seven differing instructions, all one shift.
	uint8  m_pad_20[0x2c - 0x20];
};

class CLTMessageEntryView
{
public:
	// The whole of ServerMessage::Init (server.dll 0x10010da9), inlined.
	// WHY IT IS IN THE CLASS BODY: /Ob1 inlines only what the source marks
	// inline or defines in a class body, and in lithtech.exe this function
	// HAS no out-of-line body -- slot 0 is the expansion.
	void Init(uint16 nMaxDataLen, uint16 nGrowSize)
	{
		m_DataLen    = 1;
		m_MaxDataLen = nMaxDataLen;
		m_Pos        = 1;
		m_Unknown3a  = 1;
		m_MsgFlags   = 0;
		m_Write.m_pOwner = this;

		if (m_Data.GetSize() < nMaxDataLen)
		{
			m_Data.Term();
			m_Data.Init(nMaxDataLen, nGrowSize);
		}
		else
		{
			m_Data.SetCacheSize(nGrowSize);
		}
	}

	uint8          m_pad_00[0x08];           // 0x00  vptr + refcount
	CLTMessageView m_Write;                  // 0x08
	uint16         m_DataLen;                // 0x34  (CPacket::m_DataLen)
	uint16         m_MaxDataLen;             // 0x36
	uint16         m_Pos;                    // 0x38  (CPacket::m_Pos)
	uint16         m_Unknown3a;              // 0x3a
	uint8          m_pad_3c[0x40 - 0x3c];
	uint32         m_MsgFlags;               // 0x40
	uint8          m_pad_44[0x50 - 0x44];
	CMsgArray      m_Data;                   // 0x50 .. 0x60
};

// The engine-internal write interface behind HMESSAGEWRITE.  Slot 0x00 is
// EndWrite and slot 0x8c is Verify -- both already recovered in
// src/engine/server/servermsg.h's IMessageWrite from the SAME two
// displacements in CLTServer::EndMessage2, which is this function's twin.
class IMessageWriteView
{
public:
	virtual void     EndWrite() = 0;                                          // 0x00
	virtual LTRESULT _p01()=0; virtual LTRESULT _p02()=0; virtual LTRESULT _p03()=0;
	virtual LTRESULT _p04()=0; virtual LTRESULT _p05()=0; virtual LTRESULT _p06()=0;
	virtual LTRESULT _p07()=0; virtual LTRESULT _p08()=0; virtual LTRESULT _p09()=0;
	virtual LTRESULT _p10()=0; virtual LTRESULT _p11()=0; virtual LTRESULT _p12()=0;
	virtual LTRESULT _p13()=0; virtual LTRESULT _p14()=0; virtual LTRESULT _p15()=0;
	virtual LTRESULT _p16()=0; virtual LTRESULT _p17()=0; virtual LTRESULT _p18()=0;
	virtual LTRESULT _p19()=0; virtual LTRESULT _p20()=0; virtual LTRESULT _p21()=0;
	virtual LTRESULT _p22()=0; virtual LTRESULT _p23()=0; virtual LTRESULT _p24()=0;
	virtual LTRESULT _p25()=0; virtual LTRESULT _p26()=0; virtual LTRESULT _p27()=0;
	virtual LTRESULT _p28()=0; virtual LTRESULT _p29()=0; virtual LTRESULT _p30()=0;
	virtual LTRESULT _p31()=0; virtual LTRESULT _p32()=0; virtual LTRESULT _p33()=0;
	virtual LTRESULT _p34()=0;                                                // 0x88
	virtual LTRESULT Verify() = 0;                                            // 0x8c
};

typedef IMessageWriteView* HMESSAGEWRITE_I;

// CLTClient's own vtable, as far as EndMessage2 reaches it.  0x16c / 4 = 91
// and slot 91 is iltclient.h index 20 = SendToServer -- the only three
// argument sender in the table, and the header's argument list
// (ILTMessage&, uint8, uint32) is exactly the three pushes.  The call goes
// through `mov (%edi),%edx; call *0x16c(%edx)`, i.e. through THIS object's
// own vptr, which is why it has to be spelled as a virtual call.
class ILTClientSendView
{
public:
	virtual void _s00()=0; virtual void _s01()=0; virtual void _s02()=0; virtual void _s03()=0;
	virtual void _s04()=0; virtual void _s05()=0; virtual void _s06()=0; virtual void _s07()=0;
	virtual void _s08()=0; virtual void _s09()=0; virtual void _s10()=0; virtual void _s11()=0;
	virtual void _s12()=0; virtual void _s13()=0; virtual void _s14()=0; virtual void _s15()=0;
	virtual void _s16()=0; virtual void _s17()=0; virtual void _s18()=0; virtual void _s19()=0;
	virtual void _s20()=0; virtual void _s21()=0; virtual void _s22()=0; virtual void _s23()=0;
	virtual void _s24()=0; virtual void _s25()=0; virtual void _s26()=0; virtual void _s27()=0;
	virtual void _s28()=0; virtual void _s29()=0; virtual void _s30()=0; virtual void _s31()=0;
	virtual void _s32()=0; virtual void _s33()=0; virtual void _s34()=0; virtual void _s35()=0;
	virtual void _s36()=0; virtual void _s37()=0; virtual void _s38()=0; virtual void _s39()=0;
	virtual void _s40()=0; virtual void _s41()=0; virtual void _s42()=0; virtual void _s43()=0;
	virtual void _s44()=0; virtual void _s45()=0; virtual void _s46()=0; virtual void _s47()=0;
	virtual void _s48()=0; virtual void _s49()=0; virtual void _s50()=0; virtual void _s51()=0;
	virtual void _s52()=0; virtual void _s53()=0; virtual void _s54()=0; virtual void _s55()=0;
	virtual void _s56()=0; virtual void _s57()=0; virtual void _s58()=0; virtual void _s59()=0;
	virtual void _s60()=0; virtual void _s61()=0; virtual void _s62()=0; virtual void _s63()=0;
	virtual void _s64()=0; virtual void _s65()=0; virtual void _s66()=0; virtual void _s67()=0;
	virtual void _s68()=0; virtual void _s69()=0; virtual void _s70()=0; virtual void _s71()=0;
	virtual void _s72()=0; virtual void _s73()=0; virtual void _s74()=0; virtual void _s75()=0;
	virtual void _s76()=0; virtual void _s77()=0; virtual void _s78()=0; virtual void _s79()=0;
	virtual void _s80()=0; virtual void _s81()=0; virtual void _s82()=0; virtual void _s83()=0;
	virtual void _s84()=0; virtual void _s85()=0; virtual void _s86()=0; virtual void _s87()=0;
	virtual void _s88()=0; virtual void _s89()=0; virtual void _s90()=0;
	// 0x16c
	virtual LTRESULT SendToServer(IMessageWriteView& msg, uint8 msgID, uint32 flags)=0;
};

// CClientMgr, as far as this file reaches it.  Every offset below is a
// shipped displacement, not a guess.
class CClientMgrView
{
public:
	uint8              m_pad_0000[0x60c];
	// 0x60c = m_NetMgr (0x50c, clientmgr.h) + CNetMgr::m_pMainDriver
	// (0x100, servermgr.h:1263).  The four query slots call 0x20/0x24/
	// 0x28/0x2c through it, which is CBaseDriver's query block.
	CBaseDriverView*   m_pMainDriver;        // 0x60c
	uint8              m_pad_0610[0x1258 - 0x610];
	// The two clocks, read as floats (`flds`) off the GLOBAL g_pClientMgr.
	// iltcsbase.h declares GetTime then GetFrameTime, so slot 53 (+0x125c)
	// is the game clock and slot 54 (+0x1258) the frame delta.
	float              m_FrameTime;          // 0x1258
	float              m_Time;               // 0x125c
	uint8              m_pad_1260[0x1268 - 0x1260];
	// The SERVER's console variables, mirrored into the client manager.  A
	// different object from the client's own (0x004b2f00): the shipped code
	// reaches it with `add $0x1268,%eax`, an ADD on the loaded manager
	// pointer, which is `&m_pClientMgr->m_ServerConsoleState`.
	ConsoleStateView   m_ServerConsoleState; // 0x1268
	uint8              m_pad_12b4[0x1534 - (0x1268 + 0x4c)];
	CFileMgrView*      m_pFileMgr;           // 0x1534

	// 0x004113e0 -- __thiscall, no arguments, returns the message container
	// (lt2new3.cpp already reached it from StartMessage at 0x00407560 and
	// named it there).  It never returns null on the path slot 0 takes:
	// the shipped code goes straight from the call into Init.
	CLTMessageEntryView* StartMessage();     // 0x004113e0

	// 0x004114a0 -- __thiscall, no arguments.  Walks the list at +0x10f4
	// and is the only thing slot 38 does.
	LTRESULT             FreeUnusedModels(); // 0x004114a0
};

extern CClientMgrView* g_pClientMgr;         // 0x004b17e0


// The two global light vectors and the ambient level.  clientrender.cpp
// documents all three as members of one render-state block whose base is
// 0x004b4358 (+0x130 dir, +0x13c colour, +0x148 ambient); they are declared
// loose here because the reference is a relocation and byte-matching cannot
// see the difference.
//
// WHOLE VECTORS AND NOT SIX SCALARS -- the SETTER settles it.  Slot 81
// (0x004072f0) reads all three components into %eax/%ecx/%edx and only then
// writes all three out; three separate `g_x = color.x;` assignments would
// reuse one register and interleave load with store.  Three loads then
// three stores is the memberwise copy of an LTVector, which is also what
// the neighbouring SetGlobalLightDir (0x00407200, slot 79) is written as.
// The getters interleave only because %eax is spent on the destination and
// there are two scratch registers left, which is why they alternate
// %ecx/%edx/%ecx rather than reusing one register.
// 0x004b4488 is g_RenderStruct + 0x130, clientrender.cpp's m_GlobalLightDir
// (and 0x004b4494 its m_GlobalLightColor, +0x13c): members of the one
// struct, reached through a view cut to them so they link to clientrender.cpp's
// g_RenderStruct.
class RenderStruct
{
public:
	uint8    m_pad_000[0x130];
	LTVector m_GlobalLightDir;               // 0x130  0x004b4488
	LTVector m_GlobalLightColor;             // 0x13c  0x004b4494
	uint32   m_Unknown148;                   // 0x148  0x004b44a0
	// lt2new4.cpp's GetAmbientLight (0x00407320) reads m_Unknown148 back
	// with a `fildll` whose high dword is explicitly zeroed -- VC6's
	// UNSIGNED widening -- so the type is uint32 and not int32.  It is the
	// ambient light, clientrender.cpp's name kept.
};

extern RenderStruct g_RenderStruct;          // 0x004b4358


class CLTClient
{
public:
	// ---- iltcsbase.h ------------------------------------------------- //
	HMESSAGEWRITE StartHMessageWrite();                               // slot   0
	HMODELANIM    GetModelAnimation(HOBJECT hObj);                    // slot  33
	LTBOOL        GetModelLooping(HOBJECT hObj);                      // slot  35
	uint32        GetModelPlaybackState(HOBJECT hObj);                // slot  37
	LTRESULT      FreeUnusedModels();                                 // slot  38
	LTRESULT      OpenFile(char* pFilename, ILTStream** pStream);     // slot  42
	LTBOOL        CompareStrings(HSTRING hString1, HSTRING hString2); // slot  48
	LTBOOL        CompareStringsUpper(HSTRING h1, HSTRING h2);        // slot  49
	float         GetVarValueFloat(HCONSOLEVAR hVar);                 // slot  51
	char*         GetVarValueString(HCONSOLEVAR hVar);                // slot  52
	LTFLOAT       GetTime();                                          // slot  53
	LTFLOAT       GetFrameTime();                                     // slot  54

	// ---- iltclient.h -------------------------------------------------- //
	float         GetServerConVarValueFloat(char* pName);             // slot  76
	LTRESULT      GetGlobalLightDir(LTVector& dir);                   // slot  78
	LTRESULT      GetGlobalLightColor(LTVector& color);               // slot  80
	LTRESULT      SetGlobalLightColor(LTVector color);                // slot  81
	LTRESULT      SetAmbientLight(float light);                       // slot  83
	LTRESULT      EndMessage2(HMESSAGEWRITE_I hMessage, uint32 flags); // slot 90
	LTBOOL        GetModelPlaying(HLOCALOBJ hObj);                    // slot 104
	LTRESULT      StartQuery(char* pInfo);                            // slot 106
	LTRESULT      UpdateQuery();                                      // slot 107
	LTRESULT      GetQueryResults(NetSession*& pListHead);            // slot 108

public:
	uint8           m_pad_0000[0x10];
	ILTModel*       m_pModelLT;              // 0x010
	uint8           m_pad_0014[0x3c0 - 0x14];
	CClientMgrView* m_pClientMgr;            // 0x3c0
};


// ======================================================================= //
//  1.  THE MODEL-TRACKER FAMILY -- slots 33, 35, 37, 104
//
//  Eight of these ship, 0x004076c0..0x00407850, and four of the eight are
//  already byte-exact in lt2new4.cpp (the three setters) -- the shape is
//  settled.  Every one of them:
//
//    * loads m_pModelLT into %esi ONCE and calls through it twice, with the
//      vtable RE-loaded each time (`mov (%esi),%eax` twice).  That is a
//      second call through the same pointer, not a cached `void** vtbl`
//      local -- hence the named `ILTModel* pModelLT` and nothing else.
//    * puts the LTAnimTracker* out-parameter in the DEAD hObj parameter
//      slot (`lea` of a positive displacement with no `sub %esp` in the
//      prologue; "a parameter's stack slot can hold a local
//      whose address is taken").
//    * has GetMainTracker's LT_OK arm FALL THROUGH and the failure block
//      sunk to the very end (`test %eax,%eax; jne <tail>`).
//      "Arm order tells you which branch the source guarded": that is
//      `if (... == LT_OK) { ... } return err;`, NOT
//      `if (... != LT_OK) return err;` -- the latter puts the failure
//      inline right after the test.
//
//  iltmodel.h indices, confirmed against src/LT2/sdk/inc/iltmodel.h: 0x28
//  GetMainTracker, 0x30 GetPlaybackState, 0x3c GetCurAnim, 0x40 SetCurAnim,
//  0x44 ResetAnim, 0x48 GetLooping, 0x4c SetLooping, 0x50 GetPlaying, 0x54
//  SetPlaying.
// ======================================================================= //


// --- slot 33, 0x004076c0, 64 bytes, `ret $0x4` ------------------------- //
// WHAT PINS IT
//   * `push %ecx` in the prologue is FOUR bytes of frame -- one local more
//     than the one-local members beside it -- and the second local lands in
//     the dead parameter slot at `lea 0xc(%esp),%ecx`.  So there are two:
//     the tracker and the animation index.  Declaration order sets which
//     gets the pushed word: pTracker is at the `push %ecx` slot (read back
//     by `mov 0x4(%esp),%edx`) and hAnim in the parameter's.
//   * `call *0x3c(%eax)` is iltmodel.h index 15, GetCurAnim(LTAnimTracker*,
//     HMODELANIM&) -- two pushes, the reference second.
//   * `mov %eax,0xc(%esp)` with %eax provably ZERO (it is the LT_OK that
//     the `test`/`jne` just fell through on) is `hAnim = 0;`.  VC6 reuses a
//     register it knows holds zero rather than spending a `movl $0,mem`
//     ("a zero already in a register is reused as a constant"),
//     and the store is INSIDE the success arm, so the source assigns there
//     rather than at the declaration.
//   * `or $0xffffffff,%eax` in the tail is `x = -1`, four bytes
//     against seven.  iltcsbase.h declares the return HMODELANIM.
HMODELANIM CLTClient::GetModelAnimation(HOBJECT hObj)
{
	LTAnimTracker* pTracker;
	HMODELANIM     hAnim;
	ILTModel*      pModelLT = m_pModelLT;

	if (pModelLT->GetMainTracker(hObj, pTracker) == LT_OK)
	{
		hAnim = 0;

		pModelLT->GetCurAnim(pTracker, hAnim);

		return hAnim;
	}

	return (HMODELANIM)-1;
}


// --- slot 37, 0x00407760, 63 bytes, `ret $0x4` ------------------------- //
// WHAT PINS IT
//   Byte-for-byte block 1 with two changes and no others: `call *0x30(%eax)`
//   -- iltmodel.h index 12, GetPlaybackState(LTAnimTracker*, uint32&) --
//   and `xor %eax,%eax` where block 1 has `or $-1,%eax`.  A `xor` failure
//   value against a `mov $imm`/`or $-1` one is the difference between
//   returning 0 and returning a code; iltcsbase.h declares
//   GetModelPlaybackState `uint32`, and MS_* state bits start at 1.
uint32 CLTClient::GetModelPlaybackState(HOBJECT hObj)
{
	LTAnimTracker* pTracker;
	uint32         state;
	ILTModel*      pModelLT = m_pModelLT;

	if (pModelLT->GetMainTracker(hObj, pTracker) == LT_OK)
	{
		state = 0;

		pModelLT->GetPlaybackState(pTracker, state);

		return state;
	}

	return 0;
}


// --- slot 35, 0x004077a0, 57 bytes, `ret $0x4` on BOTH exits ----------- //
// --- slot 104, 0x00407810, 57 bytes, `ret $0x4` on BOTH exits ---------- //
// WHAT PINS THEM
//   * NO `push %ecx`: one local only, and it lives in the dead hObj slot
//     (`lea 0x8(%esp),%ecx` after the single `push %esi`).  That is the
//     whole structural difference from blocks 1 and 2 -- GetLooping and
//     GetPlaying take the tracker and NOTHING else (`push %ecx` alone, one
//     argument) and answer through the return value.
//   * `xor %edx,%edx; cmp $0x56,%eax; sete %dl; mov %edx,%eax` is the
//     standard materialisation of a C `==`.  0x56 is 86 = LT_YES
//     (ltcodes.h:278), so the source compares the LTRESULT against LT_YES
//     and returns the LTBOOL both headers declare.
//   * the failure arm is `xor %eax,%eax` = LTFALSE, sunk to the end again.
//   * the pop reader's "args 12450" for both rows is an artefact: both
//     exits really are `ret $0x4` (0x4077d0/0x4077d6, 0x407840/0x407846).
//   * slot 35 is iltcsbase.h idx 35 GetModelLooping (HOBJECT); slot 104 is
//     iltclient.h idx 39 GetModelPlaying (HLOCALOBJ).  Only the header the
//     declaration comes from differs -- the bodies are the same six
//     instructions with 0x48 against 0x50.
LTBOOL CLTClient::GetModelLooping(HOBJECT hObj)
{
	LTAnimTracker* pTracker;
	ILTModel*      pModelLT = m_pModelLT;

	if (pModelLT->GetMainTracker(hObj, pTracker) == LT_OK)
		return (pModelLT->GetLooping(pTracker) == LT_YES);

	return LTFALSE;
}


LTBOOL CLTClient::GetModelPlaying(HLOCALOBJ hObj)
{
	LTAnimTracker* pTracker;
	ILTModel*      pModelLT = m_pModelLT;

	if (pModelLT->GetMainTracker(hObj, pTracker) == LT_OK)
		return (pModelLT->GetPlaying(pTracker) == LT_YES);

	return LTFALSE;
}


// ======================================================================= //
//  2.  THE CONSOLE-VARIABLE READERS -- slots 76, 51, 52
// ======================================================================= //

// --- slot 76, 0x00406c30, 48 bytes ------------------------------------- //
// WHAT PINS IT
//   * the twin at 0x00406c60 (slot 77, GetServerConVarValueString) is
//     already byte-exact in lt2new4.cpp and this is the same 48 bytes with
//     `flds 0x18(%eax)` for `mov 0x14(%eax),%eax` and `flds <0.0f>` for
//     `xor %eax,%eax`.  iltclient.h idx 5/6 declare the float first.
//   * `add $0x1268,%eax` on the LOADED manager pointer -- an ADD, not an
//     `lea` -- is `&m_pClientMgr->m_ServerConsoleState`, and the push order
//     (pName, then the state) makes the cdecl list (pState, pName).
//   * THE SHAPE.  Both `je`s land on ONE trailing block, and that block is
//     the LAST thing in the function, after the `flds 0x18(%eax); ret $4`.
//     A shared exit at the very END is the AND-merged form --
//     the success path wrapped in positive guards with the failure return
//     as the function's last statement.  Written flat as two
//     `if (!x) return 0.0f;` statements VC6 hosts the merged block at the
//     SECOND test instead, i.e. before the `flds`.
//   * the constant is 0x004a14d8, which reads 00 00 00 00 = 0.0f.  It is
//     the same four bytes slot 51 and slot 83 use.
float CLTClient::GetServerConVarValueFloat(char* pName)
{
	if (pName)
	{
		LTCommandVarView* pVar = cc_FindConsoleVar(&m_pClientMgr->m_ServerConsoleState, pName);

		if (pVar)
			return pVar->m_FloatVal;
	}

	return 0.0f;
}


// --- slot 51, 0x00407b00, 23 bytes ------------------------------------- //
// --- slot 52, 0x00407b20, 19 bytes ------------------------------------- //
// WHAT PINS THEM
//   * `ret $0x4` with %ecx never read: __thiscall member, one stack
//     argument, `this` unused.  The handle IS the record -- no lookup, no
//     manager, just a field read at the same +0x18 / +0x14 that block 2
//     above reaches through cc_FindConsoleVar.
//   * `test %eax,%eax; je <tail>` with the field read falling through and
//     the zero at the far end: the positive guard again, exactly as
//     ltnew2.cpp's GetStringData (0x00407ae0) three slots away.  Writing
//     `if (!hVar) return 0.0f;` first inverts the `je` into a `jne` and
//     puts the constant inline.
//   * slot 51's tail is `flds 0x004a14d8` (0.0f) and slot 52's is
//     `xor %eax,%eax` (LTNULL) -- which is the only difference between the
//     two bodies beyond 0x18 against 0x14, and it is what iltcsbase.h's
//     `float` / `char*` return types predict.
float CLTClient::GetVarValueFloat(HCONSOLEVAR hVar)
{
	if (hVar)
		return ((LTCommandVarView*)hVar)->m_FloatVal;

	return 0.0f;
}


char* CLTClient::GetVarValueString(HCONSOLEVAR hVar)
{
	if (hVar)
		return ((LTCommandVarView*)hVar)->m_pStringVal;

	return LTNULL;
}


// ======================================================================= //
//  3.  THE TWO-HANDLE STRING COMPARES -- slots 48 and 49
// ======================================================================= //

// --- slot 48, 0x00407a80, 42 bytes ------------------------------------- //
// --- slot 49, 0x00407ab0, 42 bytes ------------------------------------- //
// WHAT PINS THEM
//   * identical bodies; the ONLY difference is the call target, 0x004849c0
//     against 0x004849f0, which are stringbank.cpp's sb_CompareStrings and
//     sb_CompareStringsUpper (both `add $0x14` on each handle and then
//     _mbscmp/_mbsicmp -- 0x00494a7d against 0x00494b2c).  iltcsbase.h
//     declares CompareStrings then CompareStringsUpper at idx 48/49, in
//     that order, and the plain comparer is the lower address.
//   * `ret $0x8` on all three exits -> __thiscall member, two stack
//     arguments, `this` unused.
//   * THE ARM ORDER, and it is the whole of the shape.  BOTH failure blocks
//     sit past the call, and they sit in the order `xor %eax,%eax`
//     (0x00407a9d) then `mov $0x1,%eax` (0x00407aa2).  "The
//     error block at the very end: a nested success path" -- the INNER
//     else comes out first and the OUTER else last, which is a nest and
//     not two flat `if (!h) return X;` guards (those put each constant
//     inline right after its own test, and they would emit the 1 before
//     the 0).
//   * `mov $0x1,%eax` and not `push $1; pop %eax`: /O2 is /Ot, so VC6 takes
//     the five-byte form.  The three-byte push/pop is the /O1 spelling.
//   * the callees' result is returned untouched (`add $0x8,%esp; ret $8`),
//     so this is a plain forward, not a `!= 0` normalisation.
//
// COIN-FLIP, stated and believed settled by the block order: the flat
// spelling `if (!hString1) return LTTRUE; if (!hString2) return LTFALSE;
// return sb_CompareStrings(hString1, hString2);` is the same three
// statements and would be the natural reading of the semantics.  It is
// rejected because it emits each constant INLINE behind its own `jne`
// (and therefore the 1 before the 0), where the shipped code inverts both
// branches and sinks both constants past the call in the opposite order.
// If the nest comes out permuted, that flat form is the alternative.
LTBOOL CLTClient::CompareStrings(HSTRING hString1, HSTRING hString2)
{
	if (hString1)
	{
		if (hString2)
			return sb_CompareStrings(hString1, hString2);

		return LTFALSE;
	}

	return LTTRUE;
}


LTBOOL CLTClient::CompareStringsUpper(HSTRING hString1, HSTRING hString2)
{
	if (hString1)
	{
		if (hString2)
			return sb_CompareStringsUpper(hString1, hString2);

		return LTFALSE;
	}

	return LTTRUE;
}


// ======================================================================= //
//  4.  THE CLOCKS -- slots 53 and 54
// ======================================================================= //

// --- slot 53, 0x00407b40, 12 bytes ------------------------------------- //
// --- slot 54, 0x00407b50, 12 bytes ------------------------------------- //
// WHAT PINS THEM
//   * `a1 <imm32>` -- a LOAD from an absolute address, not `b8 <imm32>` --
//     so 0x004b17e0 is a POINTER variable and this is the file-scope
//     g_pClientMgr, NOT `this->m_pClientMgr`.  %ecx is never touched: the
//     member at +0x3c0 would be `mov 0x3c0(%ecx),%eax`, which is what every
//     other body in this file has.  clientmgr.h:203 already declares
//     g_pClientMgr at exactly 0x004b17e0.
//   * a bare `ret` with no immediate -> no arguments, and `flds` -> the
//     return is a float.  iltcsbase.h
//     idx 53/54 are GetTime() then GetFrameTime(), the only two
//     zero-argument LTFLOAT methods in the table.
//   * the whole body is two instructions: no null check on g_pClientMgr and
//     no LT_OK, so each is a one-line return.
LTFLOAT CLTClient::GetTime()
{
	return g_pClientMgr->m_Time;
}


LTFLOAT CLTClient::GetFrameTime()
{
	return g_pClientMgr->m_FrameTime;
}


// ======================================================================= //
//  5.  THE GLOBAL LIGHT ACCESSORS -- slots 78, 80, 81, 83
// ======================================================================= //

// --- slot 78, 0x004071d0, 35 bytes ------------------------------------- //
// --- slot 80, 0x004072c0, 35 bytes ------------------------------------- //
// WHAT PINS THEM
//   * `ret $0x4` and three stores through the single argument: the
//     out-parameter is `LTVector &`, which is what iltclient.h idx 7 and
//     idx 9 declare.
//   * THREE `mov <abs>,%reg; mov %reg,<disp>(%eax)` pairs with the scratch
//     register ALTERNATING %ecx, %edx, %ecx.  The rule "a whole-vector
//     copy adjusts the base" does not separate the two forms here, because
//     for a GLOBAL source there is no base to adjust -- VC6 folds the
//     address into each absolute either way.  What does separate them is
//     the alternation: a run of three independent `dir.x = g_x;` statements
//     reuses ONE scratch register, where a single copy schedules the next
//     load into the other register before the previous store retires.  The
//     setter below is the same argument with no ambiguity at all.
//   * `xor %eax,%eax` AFTER the last store (it clobbers the argument
//     register, which is why it cannot come earlier) -> `return LT_OK;`.
//   * `this` is never read: the light state is file scope, not a member.
LTRESULT CLTClient::GetGlobalLightDir(LTVector& dir)
{
	dir = g_RenderStruct.m_GlobalLightDir;

	return LT_OK;
}


LTRESULT CLTClient::GetGlobalLightColor(LTVector& color)
{
	color = g_RenderStruct.m_GlobalLightColor;

	return LT_OK;
}


// --- slot 81, 0x004072f0, 34 bytes ------------------------------------- //
// WHAT PINS IT
//   * `ret $0xc` -- twelve bytes of argument for one parameter, so the
//     LTVector comes BY VALUE, which is exactly how iltclient.h idx 10
//     declares SetGlobalLightColor.  (Its dir twin at 0x00407200 is
//     `ret $0xc` too and is not written here.)
//   * all three components are read out of the stack FIRST (%eax, %ecx,
//     %edx) and only then stored to the same three absolutes slot 80 reads.
//     THIS IS THE DISCRIMINATOR for the whole family: with nothing else
//     live there are three scratch registers, so a memberwise copy loads
//     all three and then stores all three, while three separate
//     `g_GlobalLightColorX = color.x;` statements would spend one register
//     and interleave.  Hence `g_GlobalLightColor = color;` and an LTVector
//     global, not six loose floats.  (0x00407200, slot 79, the dir setter,
//     ends in the identical three-load/three-store tail.)
//   * `xor %eax,%eax` last again -> LT_OK.
LTRESULT CLTClient::SetGlobalLightColor(LTVector color)
{
	g_RenderStruct.m_GlobalLightColor = color;

	return LT_OK;
}


// --- slot 83, 0x00407350, 69 bytes ------------------------------------- //
// WHAT PINS IT
//   * the getter beside it (0x00407320, slot 82) is already byte-exact in
//     lt2new4.cpp as `light = g_dwAmbientLight * (1.0f/255.0f);` and reads
//     the SAME 0x004b44a0.  This is its setter, and it stores the inverse
//     scaling: `fmuls 0x004a1a5c` where those four bytes are 43 7f 00 00 =
//     255.0f.  A `* 255` on an int would be `imul`; the multiply is in the
//     FPU because the value is still a float at that point.
//   * `call 0x004928c0` is above the app region (0x401000..0x4928bf) and
//     takes st(0) in and leaves %eax out -- the CRT's __ftol.  Only %eax is
//     stored, so the cast target is 32-bit; lt2new4.cpp's `fildll` with an
//     explicitly zeroed high dword makes it uint32.
//   * THE CLAMP.  `flds 0x4(%esp)` loads the parameter into st(0) ONCE and
//     each arm does `fstp %st(0); flds <const>` -- the constant comes out of
//     .rdata rather than being stored as an immediate, which
//     ("a float clamp") is the in-place form: the value being clamped is a
//     local/parameter the compiler keeps in st(0), not a member it would
//     store an immediate into.  ci_SetCameraFOV (0x0040b310) is the same
//     pattern.
//   * the branch senses are the plain ones: `test $0x1,%ah` (C0) is `< 0.0f`
//     and `test $0x41,%ah` (C0|C3) inverted is `> 1.0f`.  The first arm
//     jumps OVER the second test (`jmp 0x00407380`), which is an else-if
//     chain and not two independent guards.
//   * constants: 0x004a14d8 = 0.0f, 0x004a1a08 = 1.0f, 0x004a1a5c = 255.0f.
LTRESULT CLTClient::SetAmbientLight(float light)
{
	if (light < 0.0f)
		light = 0.0f;
	else if (light > 1.0f)
		light = 1.0f;

	g_RenderStruct.m_Unknown148 = (uint32)(light * 255.0f);

	return LT_OK;
}


// ======================================================================= //
//  6.  THE QUERY FORWARDERS -- slots 106, 107, 108
//
//  All three open with the identical four instructions --
//  `mov 0x3c0(%ecx),%eax; mov 0x60c(%eax),%ecx; test %ecx,%ecx; je <tail>`
//  -- and all three end in the identical RETURN_ERROR block.  What differs
//  is the forwarded slot (0x20 / 0x24 / 0x28), the argument count, and the
//  error-tag literal, and the three literals are what NAME the slots:
//  0x004a7a6c "StartQuery", 0x004a7a78 "UpdateQuery", 0x004a7a84
//  "GetQueryResults" (0x004a7a94 "EndQuery" names slot 109, not written
//  here).  iltclient.h idx 41/42/43/44 are those four in that order.
//
//  WHAT PINS THE ERROR SITE, for all three
//    * `push $0x49` into 0x0042d760 -- 0x49 is 73, LT_NOTINITIALIZED --
//      then `cmp $0x1,0x004b0ab0`, so the debug level is 1.
//    * THE TAG IS BARE.  The literal is "StartQuery", not
//      "CLTClient::StartQuery"; the CLTClient:: prefix that OpenFile and
//      EndMessage2 carry is absent here, and it is absent in the shipped
//      bytes, not in the reading.
//    * it is pushed as `push $imm32`, an immediate, so the name is a string
//      literal at the site and not de_error.h's `static char*` form.
//    * the printer's arguments are (g_pErrorFormat, name, 0x004a7168,
//      0x004a7164).  0x004a7168 holds the ASCII "73" -- the stringized
//      CODE, LT_ERRSTR(LT_NOTINITIALIZED) -- and 0x004a7164 holds " ", the
//      description.  So this is plain RETURN_ERROR and NOT a _DESC form.
//      (The hand-off called 0x004a7168 "the description literal"; it is
//      not.)
//    * the error block sits at the very END with the forward falling
//      through: the positive-guard shape, i.e. the RETURN_ERROR is
//      the function's last statement.  `if (!p) RETURN_ERROR(...)` first
//      would put the whole report block inline right after the `test`,
//      which is what the ILTCursor proxy at 0x00405e50 actually looks like.
// ======================================================================= //

// --- slot 106, 0x00406950, 87 bytes ------------------------------------ //
// The forwarded slot is 0x20 and ONE argument is pushed; it is a `call`
// with `ret $0x4` and not a `jmp`, because there is a stack argument to
// clean.
LTRESULT CLTClient::StartQuery(char* pInfo)
{
	CBaseDriverView* pDriver = m_pClientMgr->m_pMainDriver;

	if (pDriver)
		return pDriver->StartQuery(pInfo);

	RETURN_ERROR(1, "StartQuery", LT_NOTINITIALIZED);
}


// --- slot 107, 0x004069b0, 76 bytes ------------------------------------ //
// `jmp *0x24(%edx)` -- a tail jump, not a call.  A
// zero-argument forward has nothing to clean, which is the only reason this
// one may jump where its neighbours must call.
LTRESULT CLTClient::UpdateQuery()
{
	CBaseDriverView* pDriver = m_pClientMgr->m_pMainDriver;

	if (pDriver)
		return pDriver->UpdateQuery();

	RETURN_ERROR(1, "UpdateQuery", LT_NOTINITIALIZED);
}


// --- slot 108, 0x00406a00, 92 bytes ------------------------------------ //
// WHAT PINS THE EXTRA STATEMENT
//   `movl $0x0,(%eax)` sits BETWEEN the `test %ecx,%ecx` and the `je`, so
//   it executes on BOTH paths: the out-parameter is cleared before the
//   guard, not inside the success arm.  The driver pointer is loaded ahead
//   of it (two instructions before the argument is even fetched), which is
//   the load of a named local rather than a use-site load.
//   The reference is passed on as the pointer it is (`push %eax`), which is
//   why the driver's slot takes `NetSession**` where iltclient.h declares
//   `NetSession* &`.
LTRESULT CLTClient::GetQueryResults(NetSession*& pListHead)
{
	CBaseDriverView* pDriver = m_pClientMgr->m_pMainDriver;

	pListHead = LTNULL;

	if (pDriver)
		return pDriver->GetQueryResults(&pListHead);

	RETURN_ERROR(1, "GetQueryResults", LT_NOTINITIALIZED);
}


// ======================================================================= //
//  7.  THE MESSAGE FUNCTIONS -- slots 0, 38, 90
// ======================================================================= //

// --- slot 0, 0x00407630, 138 bytes ------------------------------------- //
//
//  This is ILTCSBase::StartHMessageWrite, and its TWIN IS ALREADY WRITTEN:
//  src/engine/server/servermessage.cpp's CLTServerWriteView::
//  StartHMessageWrite (server.dll 0x1002f416) is
//
//      ServerMessage* pMsg = m_pServerMgr->StartMessage();
//      pMsg->Init(0x2000, 0x1000);
//      return (HMESSAGEWRITE)&pMsg->m_Write;
//
//  and the client's 138 bytes are that source with ServerMessage::Init
//  INLINED.  The server keeps Init out of line at 0x10010da9; lithtech.exe
//  has no such body, and the expansion is what makes this row 138 bytes
//  instead of ~20.
//
// WHAT PINS IT
//   * `mov 0x3c0(%ecx),%ecx; call 0x004113e0` with nothing pushed and
//     nothing cleaned: __thiscall, zero arguments, on the manager held by
//     pointer.  lt2new3.cpp already reached 0x004113e0 the same way from
//     StartMessage (0x00407560).  There is NO null test on the result --
//     the shipped code goes straight from the call into the stores, which
//     is the same thing the server's twin does and is noted there.
//   * `lea 0x8(%esi),%eax` for the return value: the LMessageImpl is the
//     container's member at +0x08, which servermsg.h pinned from the other
//     direction (m_pOwner at LMessageImpl+0x08 == container+0x10, and
//     `mov %esi,0x10(%esi)` here writes exactly that).
//   * the five header stores are 16-bit for four of them and 32-bit for the
//     fifth, at 0x34/0x36/0x38/0x3a (words) and 0x40 (dword) -- CPacket's
//     m_DataLen, m_MaxDataLen, m_Pos, the spare word, and m_Flags.  Three
//     of the four words are written from %ax (=1) and only m_MaxDataLen
//     from an immediate, because only it is not 1.
//   * `cmpl $0x2000,0x58(%esi)` is a 32-BIT unsigned compare with `jae`,
//     i.e. `m_Data.GetSize() < (uint32)nMaxDataLen` -- m_nElements is
//     uint32 and the uint16 parameter widens into it.
//   * THREE calls to 0x0040bfe0, not two.  collection.h's
//     `Init(size, cacheSize)` is `Term(); _InitArray(cacheSize);
//     SetSize(size);` and contributes only two (its own Term and its
//     SetSize).  The third is an explicit `Term()` in front of it.
//   * the four stores between calls two and three are _InitArray verbatim
//     and IN ITS ORDER: m_pArray (+0x04), m_nElements (+0x08),
//     SetWantedCache (+0x10), SetCacheSize (+0x0c).  The out-of-order pair
//     at the end is collection.h's own order, not a scheduling artefact.
//   * the else arm is one store to +0x10 of the array, which is
//     CMoArray::SetCacheSize(size) -- `m_Cache.SetWantedCache(size)` --
//     and not m_CacheSize (+0x0c).
//   * `push $0x4b4e88` at all three calls is g_DefAlloc's ADDRESS
//     (stdlithalloc.cpp), pushed second, so the callee list is
//     (newSize, pAlloc).
//
// CONFIDENCE: MEDIUM.  The source shape is pinned; what is a build question
// and not a source one is whether VC6's per-function inline budget
// ("spent from the end backwards") splits the expansion at
// exactly SetSize2 here as it does in the shipped image.  Everything else
// -- statement order, the three Term/Init calls, the field widths -- is off
// the bytes.
HMESSAGEWRITE CLTClient::StartHMessageWrite()
{
	CLTMessageEntryView* pMsg = m_pClientMgr->StartMessage();

	pMsg->Init(0x2000, 0x1000);

	return (HMESSAGEWRITE)&pMsg->m_Write;
}


// --- slot 38, 0x00407880, 11 bytes ------------------------------------- //
// WHAT PINS IT
//   * `mov 0x3c0(%ecx),%ecx; jmp 0x004114a0` and nothing else: a
//     one-statement forward to a zero-argument __thiscall on the client
//     manager.  Nothing is pushed and nothing is cleaned, which is the only
//     configuration VC6 will tail-jump.
//   * iltcsbase.h idx 38 is `LTRESULT FreeUnusedModels()`, the only
//     zero-argument LTRESULT in that neighbourhood, and 0x004114a0 walks
//     the manager's list at +0x10f4 decrementing a use count at +0x208 of
//     each entry -- which is what freeing unused models is.
LTRESULT CLTClient::FreeUnusedModels()
{
	return m_pClientMgr->FreeUnusedModels();
}


// --- slot 90, 0x00407590, 120 bytes ------------------------------------ //
// WHAT PINS IT
//   * the literal at 0x004a7c2c is "CLTClient::EndMessage2" and it is
//     pushed as an IMMEDIATE -- a string literal at the site, so plain
//     RETURN_ERROR and not de_error.h's `static char*` RETURN_ERROR_NAMED
//     form (which is what the SERVER's twin uses, and which assembles as
//     `push [imm32]`).  `push $0x3c` = 60 = LT_INVALIDPARAMS,
//     `cmp $0x2,0x004b0ab0` = level 2.
//   * ONE `||`, not two guards: the null test and the non-zero test both
//     `je`/`jne` to the SAME block, and that block sits AFTER the body.
//     "A shared error block after the body: one if (A || B)".
//   * `call *0x8c(%eax)` on the message with no arguments, result compared
//     against zero: src/engine/server/servermsg.h already recovered slot
//     0x8c of this same interface as Verify(), from this function's twin.
//     `call *(%edx)` afterwards is slot 0x00, EndWrite().
//   * `call *0x16c(%edx)` where %edx is THIS object's own vtable: slot
//     0x16c/4 = 91 = iltclient.h idx 20 = SendToServer(ILTMessage&, uint8,
//     uint32).  Three pushes, right to left: message, id, flags.
//   * `mov 0x1c(%esi),%cl` -- a BYTE read of a 32-bit field -- then
//     `push %ecx`: the field is uint32 m_MessageID and the parameter uint8,
//     so the narrowing is the callee's declared type doing it.
//   * `mov %eax,%edi` before the EndWrite call and `mov %edi,%eax` after:
//     the send's result is held across EndWrite and returned, so EndWrite
//     runs BEFORE the return and the result cannot be returned directly.
LTRESULT CLTClient::EndMessage2(HMESSAGEWRITE_I hMessage, uint32 flags)
{
	LTRESULT        dResult;
	CLTMessageView* pMsg = (CLTMessageView*)hMessage;

	if (!hMessage || hMessage->Verify() != LT_OK)
		RETURN_ERROR(2, "CLTClient::EndMessage2", LT_INVALIDPARAMS);

	dResult = ((ILTClientSendView*)this)->SendToServer(*hMessage, pMsg->m_MessageID, flags);

	hMessage->EndWrite();

	return dResult;
}


// ======================================================================= //
//  8.  slot 42, 0x00406610, 211 bytes -- CLTClient::OpenFile
// ======================================================================= //
// WHAT PINS IT
//   * the name is the literal at 0x004a7a38, "CLTClient::OpenFile", pushed
//     as an immediate at BOTH sites.
//   * TWO error sites at DIFFERENT DEBUG LEVELS, and that is the whole
//     discriminator between them: `cmp $0x3` at 0x00406675 against
//     `cmp $0x1` at 0x004066b3.  Codes to match: `push $0x3d` = 61 =
//     LT_NOTFOUND and `push $0x3c` = 60 = LT_INVALIDPARAMS.  The code
//     strings the printer gets are 0x004a7a4c ("61") and 0x004a7160 ("60");
//     BOTH sites pass 0x004a7164 (" ") as the description, so both are
//     plain RETURN_ERROR.  (The hand-off read 0x004a7a4c as a description;
//     it is the stringized code.)
//   * `sub $0xc` and nothing else in the frame: a bare CClientFileRef, the
//     0x0c-byte record clientmgr.h declares, zeroed by its own constructor
//     as two dwords and one WORD (`mov %di,0x10(%esp)`) -- which is exactly
//     `m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0;` with the
//     third member a uint16.
//   * `xor %edi,%edi` once, then %edi serves as the constant zero for both
//     null tests (`cmp %edi,%eax`, `cmp %edi,%esi`) and for all four zero
//     stores ("a zero already in a register").
//   * ONE `||`: both parameter tests jump to the SAME block and that block
//     is the LAST in the function, past the LT_NOTFOUND report.
//   * the second `mov %edi,0x8(%esp)` after the guards is a REAL source
//     statement -- the constructor already stored zero there before the
//     branch, and VC6 did not eliminate the repeat across the branch.
//   * `mov 0x1534(%ecx),%edx` on the loaded manager is CClientMgr::
//     m_pFileMgr (lt2new2.cpp/ltnew2.cpp), and the cdecl push order puts
//     the manager first and the record second, matching
//     clientworldinfo.cpp's sub_4048c0(pFileMgr, pRef).
//   * `cmp %edi,%eax; mov %eax,(%esi); je` -- the store of the stream into
//     the out-parameter is scheduled between the compare and the branch, so
//     it happens unconditionally: the result is written through BEFORE the
//     failure is reported, and the value tested is the one in %eax.
//
// COIN-FLIP, stated: the two ref stores come out as m_pFilename first and
// m_nFileType second, but %eax (holding pFilename) is needed for the
// following `lea`, so the scheduler had a reason to hoist that store
// regardless of source order.  Written here in DECLARATION order, which is
// what the other CClientFileRef site in the image (ci_SetPolyGridTexture,
// 0x0040b970) emits.  The alternative spelling, if this comes out permuted,
// is `ref.m_pFilename = pFilename; ref.m_nFileType = 0;`.
//
// SECOND COIN-FLIP: `if (!*pStream)` relies on VC6 forwarding the value it
// has just stored through the out-parameter, which is what the shipped
// `cmp %edi,%eax` (a register, not a reload of `(%esi)`) says happened.  If
// this comes out with a reload, the alternative spelling is
// `ILTStream* p = sub_4048c0(...); *pStream = p; if (!p) ...`.
LTRESULT CLTClient::OpenFile(char* pFilename, ILTStream** pStream)
{
	CClientFileRef ref;

	if (!pFilename || !pStream)
		RETURN_ERROR(1, "CLTClient::OpenFile", LT_INVALIDPARAMS);

	ref.m_nFileType = 0;
	ref.m_pFilename = pFilename;

	*pStream = sub_4048c0(m_pClientMgr->m_pFileMgr, &ref);

	// Positive guard, error block last. The shipped code tests for the case
	// that CONTINUES (`je <error>` falling through to the success return),
	// which is the positive-guard rule; the inverted spelling hosts the 26-byte
	// RETURN_ERROR block on the fall-through instead and costs two blocks.
	if (*pStream)
		return LT_OK;

	RETURN_ERROR(3, "CLTClient::OpenFile", LT_NOTFOUND);
}


// ======================================================================= //
//
//  THE ONE ROW NOT WRITTEN
//
//  slot 71, 0x00407c00, 77 bytes -- `??_GCLTClient`, the compiler-generated
//  SCALAR DELETING DESTRUCTOR.  NOT `~ILTClient`, and NOT hand-writable.
//
//  WHY IT IS THE DELETING STUB, off the bytes:
//    * it takes one stack argument that is never anything but a FLAG:
//      `mov 0x8(%esp),%al; test $0x1,%al; je <skip>; push %esi;
//       call 0x004290c0` -- the `__flags & 1` test and the operator delete
//      that only ??_G emits.  `ret $0x4` and `mov %esi,%eax` (returning
//      `this`) complete the signature `void* ??_G(uint flags)`.
//    * the destructor BODY is inlined into it -- there is no call to a
//      separate ~CLTClient -- and it is three vtable stores and a free:
//        - `movl $0x004a14e0,(%esi)`   CLTClient's own vtable, the store
//          every VC6 destructor makes on entry (and the store ltnew2.cpp
//          used to prove 0x004a14e0 is this class's table);
//        - `mov 0x36c(%esi),%eax; test; je; push %eax; call 0x004290c0;
//           movl $0x0,0x36c(%esi)` -- one owned member at +0x36c freed and
//          nulled, the only statement the destructor's SOURCE has;
//        - `movl $0x004a1a0c,0x370(%esi)` -- a member sub-object at +0x370
//          running its own destructor's vptr restore;
//        - `movl $0x004a1810,(%esi)` -- the BASE's vptr restore, and
//          0x004a1810 is the abstract ILTClient vftable (slot 0 is the
//          `_purecall` stub at 0x004928e7), which is what proves the base
//          subobject is ILTClient and the derived class CLTClient.
//
//  REPRODUCING IT needs the whole CLTClient declaration -- the base list,
//  the member at +0x36c, the member sub-object at +0x370 with its own
//  destructor -- and `virtual ~CLTClient();` on it, so that the compiler
//  emits ??_G itself.  A hand-written function cannot produce ??_G: the
//  flag parameter, the `mov %esi,%eax` return and the vptr stores are all
//  compiler artefacts with no source line, and any attempt to spell them
//  out changes them.  It belongs with CLTClient's real definition in
//  src/engine/lithtech/clientinterface.h, not in a scratch TU.
//
//  STATUS: SKIPPED, deliberately, and it should stay skipped until the
//  class itself is declared.
//
// ======================================================================= //


// ======================================================================= //
//
//  PER-ADDRESS STATUS
//
//    0x00407630  slot   0  StartHMessageWrite      DONE   (medium: inline split)
//    0x004076c0  slot  33  GetModelAnimation       DONE
//    0x004077a0  slot  35  GetModelLooping         DONE
//    0x00407760  slot  37  GetModelPlaybackState   DONE
//    0x00407880  slot  38  FreeUnusedModels        DONE
//    0x00406610  slot  42  OpenFile                DONE   (two coin-flips noted)
//    0x00407a80  slot  48  CompareStrings          DONE   (coin-flip noted)
//    0x00407ab0  slot  49  CompareStringsUpper     DONE   (coin-flip noted)
//    0x00407b00  slot  51  GetVarValueFloat        DONE
//    0x00407b20  slot  52  GetVarValueString       DONE
//    0x00407b40  slot  53  GetTime                 DONE
//    0x00407b50  slot  54  GetFrameTime            DONE
//    0x00407c00  slot  71  ??_GCLTClient           SKIPPED -- compiler emitted
//    0x00406c30  slot  76  GetServerConVarValueFloat  DONE
//    0x004071d0  slot  78  GetGlobalLightDir       DONE
//    0x004072c0  slot  80  GetGlobalLightColor     DONE
//    0x004072f0  slot  81  SetGlobalLightColor     DONE
//    0x00407350  slot  83  SetAmbientLight         DONE
//    0x00407590  slot  90  EndMessage2             DONE
//    0x00407810  slot 104  GetModelPlaying         DONE
//    0x00406950  slot 106  StartQuery              DONE
//    0x004069b0  slot 107  UpdateQuery             DONE
//    0x00406a00  slot 108  GetQueryResults         DONE
//
//  None of the twenty-three is a mid-function fragment, none is mis-sized
//  (every row's stated length runs to the padding `nop`s), and none is
//  already covered elsewhere in src/engine -- every prior hit for these
//  addresses is a comment or a CSV row, including in the in-progress
//  src/engine/lithtech/cltnew.cpp, which covers the DISJOINT twenty-one.
//
//  MERGE NOTE.  cltnew.cpp and this file are two scratch TUs over the same
//  class and they name the same things: CLTClient, CClientMgrView,
//  g_pClientMgr, IMessageWriteView, g_GlobalLightDir.  Where both make a
//  choice, this file follows cltnew.cpp's (g_GlobalLightDir as a whole
//  LTVector, g_pClientMgr at 0x004b17e0, CClientMgr+0x3c0), so the two sets
//  of bodies can be concatenated once the duplicate declarations are
//  dropped.  The one place they must NOT be merged blindly is the error
//  macro: cltnew.cpp's sites use RETURN_ERROR_NAMED (the shipped bytes push
//  `[imm32]`, a static char* variable) and every site in this file uses
//  plain RETURN_ERROR (the shipped bytes push `imm32`, a literal).
//
// ======================================================================= //
