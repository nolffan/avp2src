// *********************************************************************** //
//
//  MODULE:   CLU47E710 (.CPP)   — scratch reconstruction of
//            the retail lithtech.exe 0x0047e710..0x0047f014
//
//  PURPOSE:  The three DERIVED sound-instance classes of the client sound
//            engine.  src/engine/lithtech/clientsoundinst.cpp already has the
//            BASE class (CSoundInstance, vtable 0x004a3448) and two of the
//            derived constructors; this file is the nine bodies that fill the
//            three slots each derived class overrides.
//
//  HOW THE MODULE WAS IDENTIFIED — FROM .RDATA, NOT FROM CODE.
//  Every one of the nine addresses is a dword in .rdata, and the dwords are
//  in four contiguous 23-entry runs.  They are VTABLES, of one class family:
//
//    0x004a3448  the BASE      ctor 0x0047d310 (m_Type left 0); slots +0x10
//                              and +0x14 are 0x004928e7 = __purecall, which
//                              is what fixes the slot numbering
//    0x004a34a4  m_Type == 1   ctor 0x0047e970  (AMBIENT)
//    0x004a3508  m_Type == 2   ctor 0x0047eda0  (3D)
//    0x004a3698  m_Type == 0   ctor 0x00483630  (LOCAL — in-head)
//
//  Lining the four runs up column by column names all nine outright: they are
//  exactly the entries that DIFFER between the four vtables, and they differ
//  in only three columns (four for the type-2 class).
//
//      slot      base 0x4a3448   type0 0x4a3698  type1 0x4a34a4  type2 0x4a3508
//      +0x04     0x0047d3f0      0x0047e710      0x0047e9a0      0x0047ee50
//      +0x10     __purecall      0x0047e7b0      0x0047eaa0      0x0047f020
//      +0x14     __purecall      0x0042d1b0      0x0047ef10      0x0047ef10
//      +0x40     0x0042d1b0      0x0047e760      0x0047e9f0      0x0047eea0
//
//  Nine of those ten cells are this cluster.  0x0047f020 is the type-2
//  Update and is NOT in the range; 0x0042d1b0 is the image-wide folded
//  `xor %eax,%eax ; ret $4` stub, which is what the type-0 class's +0x14
//  compiled to and what the BASE's +0x40 is.
//
//  WHICH CLASS IS WHICH.  m_Type is 0/1/2 and the SDK's own flags are
//  PLAYSOUND_LOCAL 0x0000, PLAYSOUND_AMBIENT 0x0001, PLAYSOUND_3D 0x0002 —
//  the same three values in the same order, and every body agrees:
//    * the m_Type == 0 class pins itself to the listener (Init copies the
//      listener position in and hard-codes a 5/10 radius pair; Update parks
//      the Miles 3D sample five units along the listener's facing vector).
//      That is an in-head sound.
//    * the m_Type == 1 class attenuates in SOFTWARE between an inner and an
//      outer radius and never orients.  That is an ambient sound.
//    * the m_Type == 2 class is the one clientsoundinst.cpp already found
//      owning a Miles 3D sample and the obstruction/occlusion slots.
//
//  STEP ZERO — NOT ALREADY RECONSTRUCTED.  None of the nine addresses is
//  cited anywhere under src/engine/.  It is not a shared client/server module
//  either: none of the nine byte-strings occurs in the retail server.dll,
//  and server.dll does not import mss32.dll at all, while six of the nine
//  call Miles directly.  clientsoundinst.cpp documents these four vtables but
//  reconstructs only the base class, the type-1 constructor and three type-2
//  bodies.
//
//  NAMES.  Every type and global here carries a distinguishing suffix so it
//  cannot collide with src/engine/lithtech/clientsound.h or
//  clientsoundinst.cpp, which model the same objects under their own names.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  THE TYPES THIS FILE REACHES
// ======================================================================= //

// The 12-byte vector.  It IS ltvector.h's LTVector, and it is redeclared for
// the reason src/engine/lithtech/clientshell.cpp redeclares it as CVec3: the
// three-float constructor in this build is OUT OF LINE at 0x00411d40 (25
// bytes, thiscall, `ret $0xc`, returns `this`), and 0x0047eaa0 CALLS it.
// Spelling it as ltvector.h's inline would lose that call.
class CSndVec3J
{
public:
	CSndVec3J() {}
	CSndVec3J(float mx, float my, float mz);                   // 0x00411d40

	// ltvector.h's own operator-, by value as the header has it.  This is
	// what builds the temporary at 0x0047ec66.
	CSndVec3J operator - (const CSndVec3J v) const
		{ return CSndVec3J(x - v.x, y - v.y, z - v.z); }

	float x, y, z;
};


struct CSndDataJ;   // clientsound.h's CSoundData; only ever passed through


// 0x0040f7f0 — `*(char**)0x004b17e0 + 0x774`, five instructions, no
// arguments.  clientmgr.h already names it ci_GetSoundMgr(); it is spelled by
// address here so the relocation resolves to it.  The object it
// returns has a vtable at offset 0 — 0x0047e81e loads `(%eax)` and calls
// slot +0x2c through it.
class ILTSoundMgrJ
{
public:
	// Eleven slots before the one this file reaches.  The name comes from
	// what slot +0x2c is handed: a struct whose first dword this file sets to
	// REVERBPARAM_VOLUME|REFLECTTIME|DECAYTIME and whose +0x04, +0x0c and
	// +0x10 come back filled — ltbasedefs.h's ReverbProperties, field for
	// field, and "CSoundMgr::GetReverbProperties" is a string in this image.
	virtual void _v00()=0; virtual void _v04()=0; virtual void _v08()=0;
	virtual void _v0c()=0; virtual void _v10()=0; virtual void _v14()=0;
	virtual void _v18()=0; virtual void _v1c()=0; virtual void _v20()=0;
	virtual void _v24()=0; virtual void _v28()=0;
	virtual LTRESULT GetReverbProperties(ReverbProperties* pReverb)=0;  // +0x2c

public:
	uint8     m_pad_04[0x728 - 0x04];
	// 0x728 — the Miles H3DPOBJECT the listener is, handed straight to
	// AIL_3D_orientation at 0x0047e89a.
	void*     m_h3DListener;                 // 0x728
	// 0x72c — where the listener is.  Three functions subtract it from the
	// sound's own position to get a distance.
	CSndVec3J m_vListenerPos;                // 0x72c
	uint8     m_pad_738[0x744 - 0x738];
	// 0x744 — the listener's velocity; the 3D Update (0x0047f287)
	// subtracts it from the sound's own before handing Miles the result.
	CSndVec3J m_vListenerVel;                // 0x744
	// 0x750 — the listener's facing direction.  The type-0 class hands out
	// this vector times five and the type-1 class hands it out times the
	// distance to the sound, which is what says it is a unit direction and
	// not a second position.
	CSndVec3J m_vListenerFront;              // 0x750
	// 0x75c — the listener's right: the 3D Update's pan is minus the dot
	// of the direction to the sound with it (0x0047f4b8..0x0047f4e0).
	CSndVec3J m_vListenerRight;              // 0x75c
	uint8     m_pad_768[0x778 - 0x768];
	// 0x778 — a scale applied to each component of the velocity the 3D
	// Update hands Miles (0x0047f315..0x0047f35a).  (name ours)
	float     m_fVelocityScale;              // 0x778
	uint8     m_pad_77c[0x7e8 - 0x77c];
	// 0x7e8 — null until whatever supplies reverb is up; 0x0047e7f7 refuses
	// to set a sample's reverb without it.
	void*     m_pReverb;                     // 0x7e8
	uint8     m_pad_7ec[0x80c - 0x7ec];
	// 0x80c — the "sounds may keep running" gate.  Both Update bodies bail
	// out when it is null and the instance is stopping.
	void*     m_p80c;                        // 0x80c
};

ILTSoundMgrJ* sub_0040f7f0();


// The client manager, from the two fields 0x0047ef10 walks.  g_pClientMgr is
// 0x004b17e0 — clientmgr.h, cltrest.cpp and cltnew.cpp all name it there.
class CClientLocalObjJ
{
public:
	uint8     m_pad_00[0x18c];
	CSndVec3J m_vPos;                        // 0x18c
};

class CClientObjRefJ
{
public:
	uint8             m_pad_00[0x88];
	CClientLocalObjJ* m_pObject;             // 0x88
};

class CClientMgrJ
{
public:
	uint8            m_pad_00[0x150c];
	// 0x150c — the client-local object PLAYSOUND_CLIENTLOCAL follows.
	CClientObjRefJ*  m_pLocalObjRef;         // 0x150c
};

extern CClientMgrJ* g_pClientMgrJ;           // 0x004b17e0


// The Miles entries, with the import table's own stack sizes.
extern "C" {
__declspec(dllimport) void __stdcall AIL_lock(void);
__declspec(dllimport) void __stdcall AIL_unlock(void);
__declspec(dllimport) void __stdcall AIL_set_sample_reverb(void* hSample,
                                                           float fLevel,
                                                           float fReflectTime,
                                                           float fDecayTime);
__declspec(dllimport) void __stdcall AIL_3D_orientation(void* hObj,
                                                        float* pXFace, float* pYFace,
                                                        float* pZFace,
                                                        float* pXUp, float* pYUp,
                                                        float* pZUp);
__declspec(dllimport) void __stdcall AIL_3D_position(void* hObj, float* pX,
                                                     float* pY, float* pZ);
__declspec(dllimport) void __stdcall AIL_set_3D_position(void* hObj, float x,
                                                         float y, float z);
__declspec(dllimport) int  __stdcall AIL_sample_volume(void* hSample);
__declspec(dllimport) void __stdcall AIL_set_sample_volume(void* hSample, int nVolume);
__declspec(dllimport) int  __stdcall AIL_stream_volume(void* hStream);
__declspec(dllimport) void __stdcall AIL_set_stream_volume(void* hStream, int nVolume);
__declspec(dllimport) void __stdcall AIL_3D_velocity(void* hObj, float* pDX,
                                                     float* pDY, float* pDZ);
__declspec(dllimport) void __stdcall AIL_set_3D_velocity_vector(void* hObj, float dX,
                                                                float dY, float dZ);
__declspec(dllimport) void __stdcall AIL_set_3D_sample_effects_level(void* hSample,
                                                                     float fLevel);
__declspec(dllimport) int  __stdcall AIL_sample_pan(void* hSample);
__declspec(dllimport) void __stdcall AIL_set_sample_pan(void* hSample, int nPan);
__declspec(dllimport) int  __stdcall AIL_stream_pan(void* hStream);
__declspec(dllimport) void __stdcall AIL_set_stream_pan(void* hStream, int nPan);
}


// ======================================================================= //
//  THE BASE CLASS
//
//  Twenty-three slots.  src/engine/lithtech/clientsoundinst.cpp derived this
//  layout and its field offsets from the constructor at 0x0047d310 and from
//  Term(); it is repeated here because that file is not this one's to include
//  and the offsets are what every body below reads.  Only the members this
//  file touches carry names.
// ======================================================================= //

class CSndInstanceJ
{
public:
	virtual ~CSndInstanceJ();                                   // +0x00
	// +0x04 — the base body is 0x0047d3f0: it calls Term() through the
	// vtable, stores the sample at +0x08, copies m_dwFlags out of the
	// PlaySoundInfo's first dword, takes m_nPriority from +0x114, the volume
	// from +0x120 under PLAYSOUND_CTRL_VOL and the pitch from +0x124 under
	// PLAYSOUND_CTRL_PITCH, and claims the manager's table slot.  Every
	// derived override calls it first and returns its result unchanged when
	// it fails.
	virtual LTRESULT Init(CSndDataJ* pData, PlaySoundInfo* pInfo,
	                      uint32 nUnknown);                     // +0x04
	virtual void     Term()=0;                                  // +0x08
	virtual void     _v0c()=0;                                  // +0x0c
	// +0x10 — the per-frame update.  LTTRUE means "still wanted".
	virtual LTBOOL   Update(uint32 nUnknown)=0;                 // +0x10
	// +0x14 — recompute m_fImportance from the listener position.  Pure in
	// the base; the type-0 class folds to `return LT_OK`.
	virtual LTRESULT UpdateImportance(CSndVec3J* pListenerPos)=0;  // +0x14
	virtual LTRESULT Stop(int nUnknown)=0;                      // +0x18
	virtual void     _v1c(int nUnknown)=0;                      // +0x1c
	virtual LTRESULT Hold()=0;                                  // +0x20
	virtual LTRESULT ReleaseHold()=0;                           // +0x24
	virtual LTRESULT SetObstruction(float fLevel)=0;            // +0x28
	virtual LTRESULT GetObstruction(float* pLevel)=0;           // +0x2c
	virtual LTRESULT SetOcclusion(float fLevel)=0;              // +0x30
	virtual LTRESULT GetOcclusion(float* pLevel)=0;             // +0x34
	virtual LTRESULT SetPosition(CSndVec3J* pPos, LTBOOL bUnknown);  // +0x38
	virtual LTRESULT GetPosition(CSndVec3J* pPos);              // +0x3c
	// +0x40 — where the sound is RELATIVE TO THE LISTENER.  The base is the
	// folded `return LT_OK` stub at 0x0042d1b0; all three derived classes
	// override it and all three answer in listener space.
	virtual LTRESULT GetListenerOffset(CSndVec3J* pOffset)=0;   // +0x40
	virtual LTBOOL   SetFilter(const char* pFilter)=0;          // +0x44
	virtual LTBOOL   SetFilterParam(const char* pParam, float fValue)=0;   // +0x48
	virtual LTBOOL   GetFilterParam(const char* pParam, float* pValue)=0;  // +0x4c
	virtual LTBOOL   sub_0047d3b0()=0;                          // +0x50
	// +0x54 — 0x0047df30 in all four vtables.  Both Update bodies end on it
	// and treat a zero answer as "drop me".
	virtual LTBOOL   _v54()=0;                                  // +0x54
	virtual void     _v58()=0;                                  // +0x58

public:
	uint32     m_Type;                       // 0x04
	CSndDataJ* m_pData;                      // 0x08
	void*      m_pUnknown0c;                 // 0x0c
	// 0x10 — the PlaySoundInfo flags, copied whole by Init.  Bit 0x100 is
	// PLAYSOUND_REVERB and bit 0x800 is PLAYSOUND_CLIENTLOCAL; both are
	// tested below, and both tests agree with what the flag means.
	uint32     m_dwFlags;                    // 0x10
	uint32     m_nTableSlot;                 // 0x14
	// 0x18 — the PlaySoundInfo's m_nPriority, byte for byte.
	uint8      m_nPriority;                  // 0x18
	uint8      m_pad_19[0x1c - 0x19];
	// 0x1c — what the sound is worth right now.  The base Init seeds it with
	// m_nPriority + 1.0f and 0x0047ef10 recomputes it every frame.
	float      m_fImportance;                // 0x1c
	// 0x20 — the volume in percent, 100 out of the constructor.
	uint8      m_nVolume;                    // 0x20
	uint8      m_pad_21[0x24 - 0x21];
	uint32     m_nUnknown24;                 // 0x24
	uint32     m_nUnknown28;                 // 0x28
	uint8      m_pad_2c[0x34 - 0x2c];
	float      m_fPitchShift;                // 0x34
	uint32     m_nHoldCount;                 // 0x38
	uint32     m_nSaved3c;                   // 0x3c
	// 0x40 — the state flags.  Bit 0x02 is "stopping" (both Update bodies
	// give up on it), bit 0x10 is set by the type-0 Init and maintained by
	// 0x0047ef10.
	uint32     m_StateFlags;                 // 0x40
	// 0x44/0x48/0x4c — the three Miles handles.  Exactly one is live: a 2D
	// sample, a 3D sample, or a stream.  Every body here branches on which.
	void*      m_hSample;                    // 0x44
	void*      m_h3DSample;                  // 0x48
	void*      m_hStream;                    // 0x4c
	// MISSING PADDING. m_hStream ends at 0x50 and the next named field is
	// at 0x54, so without this every field from here on sat four bytes low
	// -- which is why all three Init overrides wrote the radii to 0x70/0x74
	// instead of 0x74/0x78. A layout error, not a codegen one.
	uint8      m_pad_50[0x54 - 0x50];
	uint16     m_nUnknown54;                 // 0x54
	uint8      m_pad_56[0x58 - 0x56];
	uint32     m_nUnknown58;                 // 0x58
	LTLink     m_DataLink;                   // 0x5c
	// 0x68 — where the sound is in the world.
	CSndVec3J  m_vPos;                       // 0x68
	// 0x74/0x78 — the PlaySoundInfo's inner and outer radius.  0x0047e9a0
	// copies m_fInnerRadius into 0x74 and m_fOuterRadius + 0.01f into 0x78,
	// and 0x0047eaa0 attenuates between exactly those two.
	float      m_fInnerRadius;               // 0x74
	float      m_fOuterRadius;               // 0x78
	int        m_nUnknown7c;                 // 0x7c
	int        m_Unknown80[7];               // 0x80
	uint32     m_Unknown9c[7];               // 0x9c
};


// ======================================================================= //
//  m_Type == 0 — THE LOCAL (IN-HEAD) SOUND, vtable 0x004a3698
//
//  Its constructor is 0x00483630, over in the sound manager's own module, and
//  is not part of this cluster.
// ======================================================================= //

class CSndInstanceLocalJ : public CSndInstanceJ
{
public:
	virtual LTRESULT Init(CSndDataJ* pData, PlaySoundInfo* pInfo,
	                      uint32 nUnknown);                     // 0x0047e710
	virtual LTRESULT GetListenerOffset(CSndVec3J* pOffset);     // 0x0047e760
	virtual LTRESULT UpdateImportance(CSndVec3J* pListenerPos); // folded 0x0042d1b0
	virtual LTBOOL   Update(uint32 nUnknown);                   // 0x0047e7b0
};


// 0x0047e710 — 80 bytes, `ret $0xc`, three arguments, 3 calls (2 direct,
// 1 indirect).  Bring a local sound up.
//
// The base Init runs first and its result is returned untouched when it is
// not LT_OK: `call 0x0047d3f0 ; test %eax,%eax ; jne <epilogue>` and the
// epilogue returns eax.  The success arm is the fall-through, so the source
// tests for the failure.
//
// Then, in the order the stores appear:
//   * m_StateFlags |= 0x10 (`or $0x10,%ecx` at 0x0047e733, written back
//     BEFORE the call);
//   * SetPosition through the vtable (`call *0x38(%edi)`) with the manager's
//     listener position — `call 0x0040f7f0 ; add $0x72c,%eax ; push %eax` —
//     and the second argument 1, pushed first;
//   * the radius pair as immediates, 0x40a00000 and 0x41200000.  A local
//     sound has no real distance, so it gets a fixed five and ten.
LTRESULT CSndInstanceLocalJ::Init(CSndDataJ* pData, PlaySoundInfo* pInfo,
                                  uint32 nUnknown)
{
	LTRESULT dResult;

	dResult = CSndInstanceJ::Init(pData, pInfo, nUnknown);
	if (dResult != LT_OK)
		return dResult;

	m_StateFlags |= 0x10;

	SetPosition(&sub_0040f7f0()->m_vListenerPos, LTTRUE);

	m_fInnerRadius = 5.0f;
	m_fOuterRadius = 10.0f;

	return LT_OK;
}


// 0x0047e760 — 75 bytes, `ret $4`, one argument, 1 call.
//
// A local sound sits five units straight ahead of the listener, always.  The
// copy is three dword moves out of the manager at +0x750 and the scale is the
// 5.0f at 0x004a1ffc, one `flds ; fmuls ; fstps` per component in x,y,z
// order — VEC_MULSCALAR's expansion exactly.
LTRESULT CSndInstanceLocalJ::GetListenerOffset(CSndVec3J* pOffset)
{
	*pOffset = sub_0040f7f0()->m_vListenerFront;

	VEC_MULSCALAR(*pOffset, *pOffset, 5.0f);

	return LT_OK;
}


// Slot +0x14 of vtable 0x004a3698 is 0x0042d1b0, `xor %eax,%eax ; ret $4` —
// the image-wide folded stub.  A local sound's importance never changes with
// the listener, so the override is empty and the linker folded it away.  It
// is written out because the slot is the class's own: leaving it off would
// make the class inherit the base's pure virtual.
LTRESULT CSndInstanceLocalJ::UpdateImportance(CSndVec3J* pListenerPos)
{
	return LT_OK;
}


// 0x0047e7b0 — 436 bytes, `ret $4`, one (unused) argument, 13 calls
// (4 direct to 0x0040f7f0, 9 indirect: one vtable call on the manager, one on
// this, and seven Miles imports).
//
// THE LOCAL SOUND'S PER-FRAME WORK.  Three shapes, in this order:
//
//  1. `if (!m_hSample && !m_h3DSample && !m_hStream) return LTTRUE;` — three
//     `test ; jne` onto one continue point, then `mov $1,%eax`.  Nothing is
//     playing, so nothing needs doing and the instance stays.
//
//  2. the stopping gate.  `!m_p80c && (m_StateFlags & 2)` — the first test
//     jumps PAST the return when the pointer is live, the second jumps INTO
//     it, which is the shape of `if (A && B) return LTFALSE;`.
//
//  3. one of two bodies, by which handle is live.  A 2D sample gets its
//     reverb refreshed; a 3D sample gets repositioned in front of the
//     listener.  Both end on AIL_unlock, and the compiler cross-jumped the
//     two calls into the one at 0x0047e938 — the reverb arm reaches it by
//     `jmp`.
//
// WHY THE LISTENER HANDLE IS A LOCAL.  0x0047e86a calls the manager accessor
// and takes +0x728 into %edi BEFORE AIL_lock, and MSVC will not hoist a call
// across another call, so the handle is read into a variable first.  Every
// other manager access in this file is written inline, repeated at each use —
// which is why four calls appear for four reads.
//
// THE REVERB STRUCT is ltbasedefs.h's ReverbProperties: the 0xd stored into
// its first dword at 0x0047e816 is REVERBPARAM_VOLUME|REFLECTTIME|DECAYTIME
// (1|4|8), and the three fields read back afterwards are +0x04, +0x0c and
// +0x10 — m_fVolume, m_fReflectTime, m_fDecayTime.  The 0x3c-byte frame is
// three 12-byte vectors plus that 24-byte struct, exactly.
LTBOOL CSndInstanceLocalJ::Update(uint32 nUnknown)
{
	CSndVec3J        vFace;
	CSndVec3J        vSamplePos;
	CSndVec3J        vUp;
	ReverbProperties reverbProperties;
	void*            h3DListener;

	if (!m_hSample && !m_h3DSample && !m_hStream)
		return LTTRUE;

	if (!sub_0040f7f0()->m_p80c && (m_StateFlags & 0x02))
		return LTFALSE;

	if (m_hSample)
	{
		if (sub_0040f7f0()->m_pReverb && (m_dwFlags & PLAYSOUND_REVERB))
		{
			reverbProperties.m_dwParams = REVERBPARAM_VOLUME |
			                              REVERBPARAM_REFLECTTIME |
			                              REVERBPARAM_DECAYTIME;
			sub_0040f7f0()->GetReverbProperties(&reverbProperties);

			AIL_lock();

			AIL_set_sample_reverb(m_hSample,
			                      reverbProperties.m_fVolume,
			                      reverbProperties.m_fReflectTime,
			                      reverbProperties.m_fDecayTime);

			AIL_unlock();
		}
	}
	else if (!m_hStream && m_h3DSample)
	{
		h3DListener = sub_0040f7f0()->m_h3DListener;

		AIL_lock();

		AIL_3D_orientation(h3DListener,
		                   &vFace.x, &vFace.y, &vFace.z,
		                   &vUp.x, &vUp.y, &vUp.z);

		VEC_MULSCALAR(vFace, vFace, 5.0f);

		AIL_3D_position(m_h3DSample, &vSamplePos.x, &vSamplePos.y, &vSamplePos.z);

		// 0x0047e90c: `fcomps 0x004a1a74` against 0.5f, and the `jne` on
		// ah & 0x41 skips the store, so the store happens when the squared
		// distance is GREATER.  Only move the sample when it has drifted.
		// LTVector's DistSqr: differences x, y, z, squares summed z, x, y
		// (0x0047e8e3..0x0047e90a).  VEC_DISTSQR loads z first.
		if (((LTVector*)&vSamplePos)->DistSqr(*(LTVector*)&vFace) > 0.5f)
			AIL_set_3D_position(m_h3DSample, vFace.x, vFace.y, vFace.z);

		AIL_unlock();
	}

	// The tail both Update bodies share.  ONE `return LTFALSE` epilogue,
	// last (0x0047e95b), which the early m_p80c refusal also jumps to; the
	// keep-the-sound answer is the nested positive test.  Two refusals in a
	// row folded the _v54 result into neg/sbb/neg instead.
	if (!(m_StateFlags & 0x02))
	{
		if (_v54())
			return LTTRUE;
	}

	return LTFALSE;
}


// ======================================================================= //
//  m_Type == 1 — THE AMBIENT SOUND, vtable 0x004a34a4
//
//  Its constructor is 0x0047e970 and is ALREADY RECONSTRUCTED, as
//  CSoundInstance1::CSoundInstance1 in src/engine/lithtech/clientsoundinst.cpp.
//  It is not repeated here.
// ======================================================================= //

class CSndInstanceAmbientJ : public CSndInstanceJ
{
public:
	virtual LTRESULT Init(CSndDataJ* pData, PlaySoundInfo* pInfo,
	                      uint32 nUnknown);                     // 0x0047e9a0
	virtual LTRESULT GetListenerOffset(CSndVec3J* pOffset);     // 0x0047e9f0
	virtual LTRESULT UpdateImportance(CSndVec3J* pListenerPos); // 0x0047ef10
	virtual LTBOOL   Update(uint32 nUnknown);                   // 0x0047eaa0
};


// 0x0047e9a0 — 76 bytes, `ret $0xc`, 2 calls (1 direct, 1 indirect).
//
// The same shape as the local class's Init and the same shape as the 3D
// class's, and the three differ in exactly two places: which position goes in
// and what the SetPosition flag is.  An ambient sound gets the position the
// caller asked for and the flag 0, which the shipped code materialises as
// `push %eax` off the already-zero result of the `test` — a literal 0 in the
// source, written out of the register the guard left at zero.
//
// The radii come straight off PlaySoundInfo: +0x11c is m_fInnerRadius (a
// dword move, so a plain float copy) and +0x118 is m_fOuterRadius, with
// 0.01f (0x004a1e08) added.  The small bias is what keeps the outer radius
// strictly greater than the inner one, so the divide in Update cannot be by
// zero when both are set the same.
LTRESULT CSndInstanceAmbientJ::Init(CSndDataJ* pData, PlaySoundInfo* pInfo,
                                    uint32 nUnknown)
{
	LTRESULT dResult;

	dResult = CSndInstanceJ::Init(pData, pInfo, nUnknown);
	if (dResult != LT_OK)
		return dResult;

	SetPosition((CSndVec3J*)&pInfo->m_vPosition, LTFALSE);

	m_fInnerRadius = pInfo->m_fInnerRadius;
	m_fOuterRadius = pInfo->m_fOuterRadius + 0.01f;

	return LT_OK;
}


// 0x0047e9f0 — 166 bytes, `ret $4`, 3 DIRECT calls, all three to 0x0040f7f0.
//
// An ambient sound has a distance but no direction: it is handed out as the
// listener's own facing vector scaled to the real distance, so it is always
// dead ahead and always the right loudness.
//
// THE THIRD CALL.  0x0047ea6c calls the manager accessor and DROPS the
// result — nothing reads %eax before `xor %eax,%eax`.  It sits between the
// vector copy and the scale in both this body and 0x0047eaa0, in the same
// place, so it is a real construct and not noise.  What the source wrote
// there is NOT established; the call is reproduced as a bare expression
// statement because that is what puts the instruction back where the image
// has it, and the call sequence is what this function is scored on.
// UNDECODED: the discarded manager access at 0x0047ea6c.
//
// THE DISTANCE is ltvector.h's _CVector::Dist, (*this - vec).Mag(), on the
// sound's position: the listener position is copied into the 0xc-byte
// by-value temporary operator- takes (x and y straight off `0x72c(%eax)` /
// `0x730(%eax)`), and the differences are taken x, y, z. VEC_DIST's flat
// macro loaded z first and had no temporary.
LTRESULT CSndInstanceAmbientJ::GetListenerOffset(CSndVec3J* pOffset)
{
	float     fDistance;

	fDistance = ((LTVector*)&m_vPos)->Dist(*(LTVector*)&sub_0040f7f0()->m_vListenerPos);

	*pOffset = sub_0040f7f0()->m_vListenerFront;

	sub_0040f7f0();

	VEC_MULSCALAR(*pOffset, *pOffset, fDistance);

	return LT_OK;
}


// Slot +0x14 of vtable 0x004a34a4 and of vtable 0x004a3508 are BOTH
// 0x0047ef10.  Two classes cannot share a slot body unless the two
// definitions came out identical and the linker folded them, which is the
// same thing clientsoundinst.cpp records for 0x0047f5f0 filling both +0x2c
// and +0x34 of the 3D class.  Both are therefore written out; folding is what
// reproduces the two slots from the one address.  See the 3D copy below for
// the decode.
LTRESULT CSndInstanceAmbientJ::UpdateImportance(CSndVec3J* pListenerPos)
{
	CClientObjRefJ*   pLocalObjRef;
	CClientLocalObjJ* pLocalObj;
	float             fDistance;

	if ((m_dwFlags & PLAYSOUND_CLIENTLOCAL)
	    && (pLocalObjRef = g_pClientMgrJ->m_pLocalObjRef) != LTNULL
	    && (pLocalObj = pLocalObjRef->m_pObject) != LTNULL)
	{
		m_vPos = pLocalObj->m_vPos;
	}

	// ltvector.h's _CVector::Dist, (*this - vec).Mag(): the differences are
	// taken x, y, z and the squares summed z, x, y (0x0047ef66..0x0047ef8c).
	// VEC_DIST's flat macro loads z first.  CSndVec3J is LTVector's layout;
	// the cast reaches the SDK's own inline bodies (its operator- would call
	// the out-of-line 0x00411d40 constructor).
	fDistance = ((LTVector*)pListenerPos)->Dist(*(LTVector*)&m_vPos);

	if (fDistance > 1.0f)
		m_fImportance = m_nVolume * 0.01f / fDistance;
	else
		m_fImportance = m_nVolume * 0.01f;

	m_fImportance = m_nPriority + m_fImportance;

	if (fDistance <= m_fOuterRadius * 1.5f)
		m_StateFlags |= 0x10;
	else
		m_StateFlags &= ~0x10;

	return LT_OK;
}


// 0x0047eaa0 — 756 bytes, `ret $4`, one (unused) argument, 18 calls
// (7 direct: five to 0x0040f7f0, one to LTVector's constructor at 0x00411d40
// and one to __ftol at 0x004928c0; 11 indirect: ten Miles imports and the
// `call *0x54(%edx)` in the shared tail).
//
// THE AMBIENT SOUND'S PER-FRAME WORK.  The first two guards are the local
// class's, byte for byte.  Then the split, and it is the OPPOSITE way round
// from the local class's: here a 2D sample or a stream takes the software
// attenuation path, and only a bare 3D sample is repositioned.  The two `jne`
// at 0x0047eae5 and 0x0047eaf0 both leave for 0x0047ec1c, which is AFTER the
// 3D block — so the 3D block is the fall-through and the volume block is the
// `else`, and the `jmp 0x0047ed6e` at 0x0047ec17 is the 3D arm jumping over
// it.
//
// THE ATTENUATION is a straight linear ramp between the two radii, done on
// the SQUARED distance so the square root is only taken in the band where it
// matters:
//
//      d2 <= inner*inner  ->  1.0f          (0x004a1a08)
//      d2 >= outer*outer  ->  0.0f          (0x004a14d8)
//      otherwise          ->  1 - (sqrt(d2) - inner) / (outer - inner)
//
// and it never touches memory — every arm leaves the answer in st(0) and the
// `fmul %st(1),%st` at 0x0047ed00 consumes it there.
//
// THE FRAME is 0x28 bytes and holds four things, all of which the register
// allocator shares between the two arms by live range: one 4-byte scalar
// (the distance, then the squared distance, then the volume byte), one
// 12-byte vector (the listener offset, then the listener position, then the
// delta), the sample position, and the 12-byte temporary LTVector's
// constructor returns at 0x0047ec66.
// The subtraction below goes through a const-reference boundary: at
// 0x0047ec21 the listener's x and y are loaded straight off the manager
// (`mov 0x72c(%eax)`, `mov 0x730(%eax)`) BEFORE the `add $0x72c`, which is
// the by-value operand copy made from a reference.  Handing operator- the
// member expression directly adds first and copies through the sum
// (measured: 2 more blocks).  What the reference was in the real source is
// not established; this helper is the stand-in (name ours).
inline CSndVec3J SubQ(const CSndVec3J& a, const CSndVec3J& b)
{
	return a - b;
}

// The listener distance at 0x0047eb3b.  LTVector::Dist's shape -- the
// difference made through two const references, then a magnitude of the
// temporary -- but the squares come out z, x, y, which LTVector::Mag's
// flat x+y+z (z, y, x) cannot give.  `y*y + (z*z + x*x)` is the spelling
// that emits them in the shipped order; the accumulator (z; += x; += y)
// orders the squares right but loses the stack cleanup after the fsqrt,
// and a named difference vector moves the listener copy.  Names ours.
inline float SndMagQ(const LTVector& v)
{
	return (float)sqrt(v.y * v.y + (v.z * v.z + v.x * v.x));
}

inline float SndDistQ(const LTVector& a, const LTVector& b)
{
	return SndMagQ(a - b);
}

LTBOOL CSndInstanceAmbientJ::Update(uint32 nUnknown)
{
	float     fDistance;
	CSndVec3J vListenerOffset;
	CSndVec3J vSamplePos;
	CSndVec3J vDelta;
	float     fDistanceSqr;
	float     fAttenuation;
	uint8     nVolume;

	if (!m_hSample && !m_h3DSample && !m_hStream)
		return LTTRUE;

	if (!sub_0040f7f0()->m_p80c && (m_StateFlags & 0x02))
		return LTFALSE;

	if (!m_hSample && !m_hStream)
	{
		if (m_h3DSample)
		{
			// Not LTVector's own Dist: with it the squares are summed z, y, x
			// where 0x0047eb3b sums z, x, y (see SndDistQ above).  In the
			// same function shipped's DistSqr (0x0047ebd5) sums z, y, x.
			fDistance = SndDistQ(*(LTVector*)&m_vPos,
			    *(LTVector*)&sub_0040f7f0()->m_vListenerPos);

			vListenerOffset = sub_0040f7f0()->m_vListenerFront;

			// UNDECODED: the discarded manager access at 0x0047eb75, the
			// same construct as 0x0047ea6c above.
			sub_0040f7f0();

			VEC_MULSCALAR(vListenerOffset, vListenerOffset, fDistance);

			AIL_lock();

			AIL_3D_position(m_h3DSample, &vSamplePos.x, &vSamplePos.y,
			                &vSamplePos.z);

			if (((LTVector*)&vSamplePos)->DistSqr(*(LTVector*)&vListenerOffset) > 0.5f)
				AIL_set_3D_position(m_h3DSample, vListenerOffset.x,
				                    vListenerOffset.y, vListenerOffset.z);

			AIL_unlock();
		}
	}
	else
	{
		// The temporary at 0x0047ec66 is LTVector's out-of-line three-float
		// constructor being handed (x-x, y-y, z-z) — the arguments are
		// pushed z, y, x, which is right-to-left evaluation of exactly that.
		// The result is then COPIED into the named vector, which is what
		// return-by-value plus copy-initialisation is.
		vDelta = SubQ(m_vPos, sub_0040f7f0()->m_vListenerPos);
		fDistanceSqr = VEC_MAGSQR(vDelta);

		if (fDistanceSqr <= m_fInnerRadius * m_fInnerRadius)
		{
			fAttenuation = 1.0f;
		}
		else if (fDistanceSqr >= m_fOuterRadius * m_fOuterRadius)
		{
			fAttenuation = 0.0f;
		}
		else
		{
			fAttenuation = 1.0f - ((float)sqrt(fDistanceSqr) - m_fInnerRadius) /
			                      (m_fOuterRadius - m_fInnerRadius);
		}

		// LTMIN, not an if: 0x0047ed07 compares the __ftol result in %al
		// before it is moved to %bl, and spills it once, after the join.
		nVolume = (uint8)(m_nVolume * fAttenuation);
		nVolume = LTMIN(nVolume, 100);

		AIL_lock();

		// The handle is re-read for the setter (`mov 0x44(%esi),%eax` again
		// at 0x0047ed31) because the query call clobbered the register it was
		// in: the expression is written twice, not cached.
		if (m_hSample)
		{
			if (nVolume != (uint8)AIL_sample_volume(m_hSample))
				AIL_set_sample_volume(m_hSample, nVolume);
		}
		else
		{
			if (nVolume != (uint8)AIL_stream_volume(m_hStream))
				AIL_set_stream_volume(m_hStream, nVolume);
		}

		AIL_unlock();
	}

	if (!(m_StateFlags & 0x02))
	{
		if (_v54())
			return LTTRUE;
	}

	return LTFALSE;
}


// ======================================================================= //
//  m_Type == 2 — THE 3D SOUND, vtable 0x004a3508
//
//  Its constructor (0x0047eda0), its SetPosition override (0x0047edf0) and
//  its obstruction/occlusion slots (0x0047f590/0x0047f5c0/0x0047f5f0) are
//  reconstructed in src/engine/lithtech/clientsoundinst.cpp (as
//  CSoundInstance2).  Its Update (0x0047f020) is below, after
//  UpdateImportance, where the image has it.
// ======================================================================= //

class CSndInstance3DJ : public CSndInstanceJ
{
public:
	virtual LTRESULT Init(CSndDataJ* pData, PlaySoundInfo* pInfo,
	                      uint32 nUnknown);                     // 0x0047ee50
	virtual LTRESULT GetListenerOffset(CSndVec3J* pOffset);     // 0x0047eea0
	virtual LTRESULT UpdateImportance(CSndVec3J* pListenerPos); // 0x0047ef10
	virtual LTBOOL   Update(uint32 msDelta);                    // 0x0047f020

public:
	// 0xb8/0xc4 — the class's own two vectors (clientsoundinst.cpp's
	// constructor and SetPosition, 0x0047eda0/0x0047edf0): the position as
	// of the last Update, and the velocity Update derives from it.
	LTVector   m_vLastPos;                   // 0xb8
	LTVector   m_vVelocity;                  // 0xc4
};


// 0x0047ee50 — 77 bytes, `ret $0xc`, 2 calls (1 direct, 1 indirect).
//
// The ambient Init with one byte changed: the SetPosition flag is a literal
// 1 (`push $0x1` at 0x0047ee76) instead of the zero the ambient class pushes
// out of a register.  The extra byte is the whole of the size difference
// between the two, 77 against 76.
LTRESULT CSndInstance3DJ::Init(CSndDataJ* pData, PlaySoundInfo* pInfo,
                               uint32 nUnknown)
{
	LTRESULT dResult;

	dResult = CSndInstanceJ::Init(pData, pInfo, nUnknown);
	if (dResult != LT_OK)
		return dResult;

	SetPosition((CSndVec3J*)&pInfo->m_vPosition, LTTRUE);

	m_fInnerRadius = pInfo->m_fInnerRadius;
	m_fOuterRadius = pInfo->m_fOuterRadius + 0.01f;

	return LT_OK;
}


// 0x0047eea0 — 102 bytes, `ret $4`, 1 direct call.
//
// A 3D sound really is where it says it is, so its listener-relative offset
// is just the difference, through LTVector's own operator-. The 0x18-byte
// frame is its two 12-byte temporaries: the by-value copy of the listener
// position the three `fsubs` read, and the returned vector, which is stored
// in full (`fstps 0x10/0x14(%esp)`) and only then copied to the caller's
// vector as dwords.
LTRESULT CSndInstance3DJ::GetListenerOffset(CSndVec3J* pOffset)
{
	*(LTVector*)pOffset = *(LTVector*)&m_vPos - *(LTVector*)&sub_0040f7f0()->m_vListenerPos;

	return LT_OK;
}


// 0x0047ef10 — 260 bytes, `ret $4`, ZERO calls.  Slot +0x14 of BOTH the
// ambient and the 3D vtable; see the ambient copy above for why it is
// written twice.
//
// WHAT IT IS: recompute how much this sound is worth, given where the
// listener now is.  Three parts.
//
//  1. PLAYSOUND_CLIENTLOCAL (0x800, tested as `test $0x8,%ah` on the dword at
//     +0x10) means the sound rides an object rather than a fixed point, so
//     its position is refreshed from that object first.  The walk is
//     g_pClientMgr(0x004b17e0) -> +0x150c -> +0x88 -> +0x18c, and all three
//     tests land on the same join, so it is one && chain and not nested ifs.
//
//  2. the importance itself: the volume as a fraction, divided by the
//     distance once past a metre, plus the priority.  0x004a3504 is 0.01f —
//     the same constant the ambient Init adds as a bias — and it is a
//     per-module constant rather than a shared one, which is what puts it in
//     the little pool at 0x004a3500 beside this class's vtable rather than in
//     the image-wide pool at 0x004a1a08.
//
//     The distance is never spilled: the compare at 0x0047ef96 leaves it in
//     st(0) and BOTH arms consume it there, which is why only one 12-byte
//     local (operator-'s by-value copy of m_vPos, inside Dist) fills the 0xc frame.
//
//  3. the in-range flag.  0x004a3500 is 1.5f; inside one and a half outer
//     radii the sound keeps bit 0x10 of m_StateFlags, outside it loses it.
//     `mov 0x40(%ecx),%eax` is hoisted above the branch and each arm does its
//     own byte-wide `or`/`and` on %al, so both arms write the member.
LTRESULT CSndInstance3DJ::UpdateImportance(CSndVec3J* pListenerPos)
{
	CClientObjRefJ*   pLocalObjRef;
	CClientLocalObjJ* pLocalObj;
	float             fDistance;

	if ((m_dwFlags & PLAYSOUND_CLIENTLOCAL)
	    && (pLocalObjRef = g_pClientMgrJ->m_pLocalObjRef) != LTNULL
	    && (pLocalObj = pLocalObjRef->m_pObject) != LTNULL)
	{
		m_vPos = pLocalObj->m_vPos;
	}

	// ltvector.h's _CVector::Dist, (*this - vec).Mag(): the differences are
	// taken x, y, z and the squares summed z, x, y (0x0047ef66..0x0047ef8c).
	// VEC_DIST's flat macro loads z first.  CSndVec3J is LTVector's layout;
	// the cast reaches the SDK's own inline bodies (its operator- would call
	// the out-of-line 0x00411d40 constructor).
	fDistance = ((LTVector*)pListenerPos)->Dist(*(LTVector*)&m_vPos);

	if (fDistance > 1.0f)
		m_fImportance = m_nVolume * 0.01f / fDistance;
	else
		m_fImportance = m_nVolume * 0.01f;

	m_fImportance = m_nPriority + m_fImportance;

	if (fDistance <= m_fOuterRadius * 1.5f)
		m_StateFlags |= 0x10;
	else
		m_StateFlags &= ~0x10;

	return LT_OK;
}


// 0x0047f020 — 1384 bytes, `ret $4`, THE 3D SOUND'S PER-FRAME WORK.  The CSV
// row used to say 37 bytes: that is the distance to 0x0047f045, the join of
// the three handle tests, and the rest had been charged to a "gap".
//
// Unlike the other two Update bodies this one reads its argument: it is the
// frame time in milliseconds, and the velocity is the position change over
// it (`fildll` of the zero-extended dword at 0x0047f0ca is (float) of a
// uint32).  Then the direction to the listener, normalised, with the squared
// distance kept on the FPU stack for the software path.
//
// The split is the ambient class's: a bare 3D sample gets Miles position,
// velocity and effects level; a 2D sample or a stream gets a software volume
// (the ambient ramp times a front/back factor) and a software pan.
//
// Evidence for the less obvious spellings:
//  * the stopping gate jumps to the SHARED TAIL at 0x0047f55d (after `push
//    %ebp ; push %edi` on entry), not to a `return 0`: the body is wrapped in
//    the positive test rather than refused early.
//  * the normalised direction is a COPY of the difference (the named copy at
//    0x1c, the difference at 0x10), and the software path's squared distance
//    is that difference's MagSqr: VC6 CSEs it with the Mag() in the
//    normalise, keeps it on the FPU stack (`fld %st(0)` at 0x0047f17d) and
//    only spills it into the dead argument slot at 0x0047f3a0.  Computing it
//    into a named local first homes it at once (`fsts`), and a named sqrt
//    CSEs the software path's second sqrt away.  One local then carries the
//    squared distance and the attenuation (both live in 0x38).
//  * the three velocity scales are read into argument temporaries before any
//    multiply (0x0047f315..0x0047f338), which is float parameters of an
//    inline call; see SetVelocityQ.
//  * the effects level: the `jne` at 0x0047f36e goes to the 1.0f arm, and each
//    arm carries its own AIL_unlock + `jmp` to the tail.
//
// NEAR (size-exact, 1384 bytes, 2 instructions): the first component of the
// normalise.  Shipped `fld %st(0) ; fmuls 0x10(%esp)` (the reciprocal as an
// expression temporary), ours `flds 0x10(%esp) ; fmul %st(1),%st` (as a
// variable), y and z as shipped.  `vDir.x *= (fMag = 1.0f / fMag)` fixes x
// but flips the y/z order of both later dot products (8 operand diffs);
// writing z before y then fixes those and breaks the y/z stores (4).
// Also measured, no better: vDir.Norm() (1368/12 blocks), a free NormQ
// helper (1382), `vDir *= 1.0f / fMag`, VEC_MULSCALAR, `vDir = vDelta *
// fMag`, a separate fInv, a derived-class member Norm with `nVal / mag`.
// Also no better: `vDir *= (fMag = 1.0f / fMag)` (x fixed, y/z loads
// swap: 9 insns); `vDir.x = fMag * vDir.x` spelled out (VC6 canonicalises it
// back to `flds; fmul %st(1)`, and the dot products' y/z swap: 10 insns).
//
// The listener-minus-position difference takes its by-value operand copy
// AFTER the manager call (0x0047f105..0x0047f117); handed the member
// expression directly the copy of m_vPos goes first.  Same boundary as SubQ
// above, over LTVector's own operator- (stand-in, name ours).
inline LTVector SubLTQ(const LTVector& a, const LTVector& b)
{
	return a - b;
}

// The velocity handed to Miles, scaled per component by the manager's
// +0x778 (read three times, right to left).  Stand-in for whatever inline
// with float parameters the original called here (name ours).
inline void SetVelocityQ(void* h3DSample, const LTVector& v,
                         float fScaleX, float fScaleY, float fScaleZ)
{
	AIL_set_3D_velocity_vector(h3DSample, v.x * fScaleX, v.y * fScaleY,
	                           v.z * fScaleZ);
}

LTBOOL CSndInstance3DJ::Update(uint32 msDelta)
{
	LTVector  vDelta;
	LTVector  vDir;
	LTVector  vSamplePos;
	float     fDistSqr;
	float     fMag;
	float     fScale;
	float     fPan;
	uint32    nVolume;
	int       nPan;

	if (!m_hSample && !m_h3DSample && !m_hStream)
		return LTTRUE;

	if (sub_0040f7f0()->m_p80c || !(m_StateFlags & 0x02))
	{
		m_vVelocity = *(LTVector*)&m_vPos - m_vLastPos;
		if (msDelta > 0)
			m_vVelocity /= (float)msDelta;
		else
			m_vVelocity.Init(0.0f, 0.0f, 0.0f);

		m_vLastPos = *(LTVector*)&m_vPos;

		vDelta = SubLTQ(*(LTVector*)&sub_0040f7f0()->m_vListenerPos, *(LTVector*)&m_vPos);
		vDir = vDelta;
		fMag = vDir.Mag();
		if (fMag != 0.0f)
		{
			fMag = 1.0f / fMag;
			vDir *= fMag;
		}

		if (!m_hSample && !m_hStream)
		{
			if (m_h3DSample)
			{
				vDir = *(LTVector*)&m_vPos - *(LTVector*)&sub_0040f7f0()->m_vListenerPos;

				AIL_lock();

				AIL_3D_position(m_h3DSample, &vSamplePos.x, &vSamplePos.y,
				                &vSamplePos.z);

				if (vSamplePos.DistSqr(vDir) > 0.5f)
					AIL_set_3D_position(m_h3DSample, vDir.x, vDir.y, vDir.z);

				vDir = m_vVelocity - *(LTVector*)&sub_0040f7f0()->m_vListenerVel;

				AIL_3D_velocity(m_h3DSample, &vSamplePos.x, &vSamplePos.y,
				                &vSamplePos.z);

				if (vSamplePos.DistSqr(vDir) > 0.5f)
					SetVelocityQ(m_h3DSample, vDir,
					             sub_0040f7f0()->m_fVelocityScale,
					             sub_0040f7f0()->m_fVelocityScale,
					             sub_0040f7f0()->m_fVelocityScale);

				if (!(m_dwFlags & PLAYSOUND_REVERB))
				{
					AIL_set_3D_sample_effects_level(m_h3DSample, 0.0f);
					AIL_unlock();
				}
				else
				{
					AIL_set_3D_sample_effects_level(m_h3DSample, 1.0f);
					AIL_unlock();
				}
			}
		}
		else
		{
			// fDistSqr becomes the attenuation: one local, one home.
			fDistSqr = vDelta.MagSqr();

			if (fDistSqr <= m_fInnerRadius * m_fInnerRadius)
			{
				fDistSqr = 1.0f;
			}
			else if (fDistSqr >= m_fOuterRadius * m_fOuterRadius)
			{
				fDistSqr = 0.0f;
			}
			else
			{
				fDistSqr = 1.0f - ((float)sqrt(fDistSqr) - m_fInnerRadius) /
				                  (m_fOuterRadius - m_fInnerRadius);
			}

			fScale = ((LTVector*)&sub_0040f7f0()->m_vListenerFront)->Dot(vDir);
			if (fScale < 0.0f)
				fScale = 1.0f;
			else
				fScale = 1.0f - fScale * 0.25f;

			nVolume = (uint32)(m_nVolume * fScale * fDistSqr);
			nVolume = LTMIN(nVolume, 100);

			AIL_lock();

			if (m_hSample)
			{
				if (nVolume != AIL_sample_volume(m_hSample))
					AIL_set_sample_volume(m_hSample, nVolume);
			}
			else
			{
				if (nVolume != AIL_stream_volume(m_hStream))
					AIL_set_stream_volume(m_hStream, nVolume);
			}

			AIL_unlock();

			fPan = -((LTVector*)&sub_0040f7f0()->m_vListenerRight)->Dot(vDir);
			if (fPan < -0.75f)
				fPan = -0.75f;
			else if (fPan > 0.75f)
				fPan = 0.75f;

			nPan = (int)((fPan + 1.0f) * 64.0f);

			AIL_lock();

			if (m_hSample)
			{
				if (nPan != AIL_sample_pan(m_hSample))
					AIL_set_sample_pan(m_hSample, nPan);
			}
			else
			{
				if (nPan != AIL_stream_pan(m_hStream))
					AIL_set_stream_pan(m_hStream, nPan);
			}

			AIL_unlock();
		}
	}

	if (!(m_StateFlags & 0x02))
	{
		if (_v54())
			return LTTRUE;
	}

	return LTFALSE;
}


// *********************************************************************** //
//
//  WHAT IS NOT PINNED
//
//  * The discarded call to the manager accessor at 0x0047ea6c and 0x0047eb75.
//    It is the third of three and the fourth of five respectively, it sits in
//    the same place in both, and no register or memory it could write is read
//    afterwards.  Every other manager access in this module is a separate
//    call for a separate use — the module never caches the pointer — so the
//    call is real; what the source spelled there is not established, and the
//    bare expression statement here is a stand-in that puts the instruction
//    back, nothing more.
//
//  * The name of ILTSoundMgrJ's +0x750.  It is a unit direction (the type-0
//    class hands it out times five, the type-1 class times the real distance,
//    and the type-2 class hands out a genuine delta instead), and the only
//    direction a listener has is the one it faces, but no string or caller
//    says so outright.
//
//  * +0x7e8 and +0x80c on the manager.  Both are only ever tested against
//    null.  +0x7e8 gates reverb and +0x80c gates "may a stopping sound keep
//    running", which is what their uses do, not what they are.
//
//  * The argument to Update.  `ret $4` in both bodies and neither reads it.
//
//  * Slot +0x54 (0x0047df30) — both Update bodies end on it and treat zero as
//    "drop me", which is all that is known.
//
// *********************************************************************** //
