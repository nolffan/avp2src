// ======================================================================= //
//  w27_10015e80 -- d3d.ren: CD3DModelDraw's constructor and
//  the static initialiser of its TU (big17c14.cpp's TU in the shipped
//  image).
//
//  0x10015e80 / 0x10015e84  the constructor of the model drawer at
//      0x1008b090, a /Qip two-entry pair (the `mov 0x4(%esp),%eax` shim,
//      then the register entry the initialiser calls with `this` in
//      %ecx); returns `this`.
//  0x1001bfa0  the initialiser: construct 0x1008b090, atexit(0x1001c1b0)
//      -- the destructor thunk d3dr4a.cpp writes -- and register the TU's
//      thirteen console variables (0x10021f40, d3dr5a.cpp's tag).
//
//  FLAGS /Os /Oi- /Qip (the _osoiqip convention), measured.  The
//  constructor calls the CRT sqrt (0x1007fc98) in LTVector::Norm, so /Oi-;
//  /Qip for the two-entry pair and the register call.  big17c14.cpp's own
//  /O2 /Oi- /Qip /Qip_no_inlining was tried by adding this file to
//  D3D_QIP_OI_TUS: the constructor then saves with `mov`s and unrolls the
//  slot loop by two (979 bytes, 840 differ) and 0x1001bfa0 builds atexit's
//  argument in an outgoing area instead of `push %edi ; push $fn` (lost).
//  At /Os: the shim and 0x1001bfa0 identical, 0x10015e84 927 of 927
//  bytes with 7 differing.
//
//  Written like w27_10059c80_osoi.cpp: an explicit member function whose
//  member constructors are inline Construct()s, with each CMoArray's
//  vptr and Clear() stored through its layout and Init() the template's
//  own (dynarray.h -- the source of the doubled clears and the Term()
//  `if (m_pArray)` test the first array keeps, whose free path is the
//  cold block after the `ret`).
//
//  0x10015e84 NEAR, 7 bytes: the slot loop's base is `mov $0x4e4,%eax ;
//  add %edi,%eax` in the shipped bytes and `mov %edi,%eax ; add
//  $0x4e4,%eax` here.  Tried, all the same 7 bytes: the pointer set
//  before or after the count, `(Slot15e84*)(0x4e4 + (uint8*)this)`, a
//  `for` with the count and pointer stepped together, the loop in an
//  inline helper taking the array, and a real constructor (probe).  An
//  index `for` is far worse (954 bytes).  Other orders measured: the
//  body's stores follow the shipped order only as written below (A:
//  0x484, D: 0x690.., E: 0x69c/0x6a0, B: 0x490, F: 0x38, C: 0x49c; the
//  natural A B C D E F is 58 bytes off), and the slot's +0x14 is a float
//  (its 0 gets a register of its own, %esi, as shipped).
//
//  Measured in the shipped TU's other members' file, with
//  this file's source appended to w43_10017c10_osoiqipni.cpp (its /Os /Oi-
//  /Qip /Qip_no_inlining, the real dynarray.h in a namespace for the
//  measurement): 0x10015e84 the same 7 bytes, 0x1001bfa0 identical, and
//  this file alone at that flag set the same -- so the pair stays here.
// ======================================================================= //
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"
#include <math.h>

#pragma function(sqrt)

// The console variable (d3dr5a.cpp's tag).
struct Bank10021f40
{
	void	sub_10021f40(const char* pszName, float fValue,
	                     uint32 nA, uint32 nB);         // 0x10021f40
};

extern Bank10021f40	g_1008b054;                         // 0x1008b054  "LightModelSprites"
extern Bank10021f40	g_1008b034;                         // 0x1008b034  "ModelLODOffset"
extern Bank10021f40	g_1008b014;                         // 0x1008b014  "ModelLODBlendEnable"
extern Bank10021f40	g_1008aff4;                         // 0x1008aff4  "ModelLODBlendDist"
extern Bank10021f40	g_1008afd4;                         // 0x1008afd4  "ExtraFOVXOffset"
extern Bank10021f40	g_1008afb4;                         // 0x1008afb4  "ExtraFOVYOffset"
extern Bank10021f40	g_1008af94;                         // 0x1008af94  "ModelFovTest"
extern Bank10021f40	g_1008af74;                         // 0x1008af74  "ModelSunVariance"
extern Bank10021f40	g_1008af54;                         // 0x1008af54  "ModelApplySun"
extern Bank10021f40	g_1008af34;                         // 0x1008af34  "ModelSaturation"
extern Bank10021f40	g_1008af14;                         // 0x1008af14  "DrawModelsRigid"
extern Bank10021f40	g_1008aef4;                         // 0x1008aef4  "ModelCacheRigid"
extern Bank10021f40	g_1008aed4;                         // 0x1008aed4  "ModelUseTnL"

extern void sub_1001c1b0();                             // 0x1001c1b0  the destructor thunk (d3dr4a.cpp)

extern void* const g_vtbl_100892e0[];                   // 0x100892e0  CMoArray (+0x614)
extern void* const g_vtbl_10089310[];                   // 0x10089310  CMoArray (+0x628)
extern void* const g_vtbl_10089340[];                   // 0x10089340  CMoArray (+0x63c)

// The three arrays' element types: only their being plain data matters to
// the constructor (no destructor loop in the Term() path).
struct Elem614_15e84 { uint32 m_n00; };
struct Elem628_15e84 { uint32 m_n00; };
struct Elem63c_15e84 { uint32 m_n00; };

// CMoArray's layout, for its constructor's Clear() and vptr store (both
// the class's own).
struct Array15e84
{
	void* const*	m_pVtbl;                            // +0x00
	void*			m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	uint32			m_nCacheSize;                       // +0x0c
	uint32			m_nWantedCache;                     // +0x10
};

template<class T>
inline void Construct15e84(CMoArray<T>* pArray, void* const* pVtbl)
{
	Array15e84* p = (Array15e84*)pArray;

	p->m_pVtbl        = pVtbl;
	p->m_pArray       = 0;
	p->m_nElements    = 0;
	p->m_nCacheSize   = 0;
	p->m_nWantedCache = 0;
	pArray->Init();
}

struct Slot15e84                                        // 0x1c bytes
{
	void Construct()
	{
		m_n18 = -1;
		m_n00 = 0;
		m_w04 = 0;
		m_w06 = 0;
		m_n08 = 0;
		m_w0c = 0;
		m_w0e = 0;
		m_n10 = 0;
		m_f14 = 0.0f;
	}

	uint32	m_n00;                                      // +0x00
	uint16	m_w04;                                      // +0x04
	uint16	m_w06;                                      // +0x06
	uint32	m_n08;                                      // +0x08
	uint16	m_w0c;                                      // +0x0c
	uint16	m_w0e;                                      // +0x0e
	uint32	m_n10;                                      // +0x10
	float	m_f14;                                      // +0x14
	int32	m_n18;                                      // +0x18
};

class C1008b090_15e84
{
public:
	C1008b090_15e84*	sub_10015e80();                 // 0x10015e80 / 0x10015e84

	uint8						m_pad_000[0x38];        // +0x000
	int32						m_n038;                 // +0x038
	uint8						m_pad_03c[0x484 - 0x3c];
	LTVector					m_v484;                 // +0x484
	LTVector					m_v490;                 // +0x490
	LTVector					m_v49c;                 // +0x49c
	uint8						m_pad_4a8[0x4e4 - 0x4a8];
	Slot15e84					m_Slots4e4[8];          // +0x4e4
	uint32						m_n5c4;                 // +0x5c4
	uint32						m_n5c8;                 // +0x5c8
	uint32						m_n5cc;                 // +0x5cc
	LTMatrix					m_Mat5d0;               // +0x5d0
	uint32						m_n610;                 // +0x610
	CMoArray<Elem614_15e84>		m_Array614;             // +0x614
	CMoArray<Elem628_15e84>		m_Array628;             // +0x628
	CMoArray<Elem63c_15e84>		m_Array63c;             // +0x63c
	uint8						m_650[0x0c];            // +0x650
	uint8						m_65c[0x690 - 0x65c];   // +0x65c
	uint32						m_n690;                 // +0x690
	uint32						m_n694;                 // +0x694
	uint32						m_n698;                 // +0x698
	void*						m_p69c;                 // +0x69c
	void*						m_p6a0;                 // +0x6a0
};

extern C1008b090_15e84	g_1008b090;                     // 0x1008b090  the model drawer


// ======================================================================= //
//  0x10015e84 -- 927 bytes, then 13 bytes of padding (the shim at
//  0x10015e80).  NEAR, 7 bytes (above).
// ======================================================================= //
C1008b090_15e84* C1008b090_15e84::sub_10015e80()
{
	Slot15e84*	pSlot;
	uint32		n;

	pSlot = m_Slots4e4;
	n     = 8;
	do { pSlot->Construct(); pSlot++; } while (--n);

	m_n5c4 = 1;
	m_n5c8 = 0;
	m_n5cc = 0;
	m_Mat5d0.Identity();
	m_n610 = 0;

	Construct15e84(&m_Array614, g_vtbl_100892e0);
	Construct15e84(&m_Array628, g_vtbl_10089310);
	Construct15e84(&m_Array63c, g_vtbl_10089340);

	m_v484.x = 0.0f;
	m_v484.y = -1.0f;
	m_v484.z = 0.0f;
	m_n690 = 0;
	m_n694 = 0;
	m_n698 = 0;
	m_p69c = m_650;
	m_p6a0 = m_65c;
	m_v490.x = -2.0f;
	m_v490.y = -2.0f;
	m_v490.z = -2.0f;
	m_n038 = -1;
	m_v49c.x = 2.0f;
	m_v49c.y = -2.0f;
	m_v49c.z = -1.0f;

	m_v484.Norm();
	m_v490.Norm();
	m_v49c.Norm();

	return this;
}


extern "C" int __cdecl atexit(void (__cdecl* pfn)(void));


// ======================================================================= //
//  0x1001bfa0 -- 523 bytes.  IDENTICAL.
// ======================================================================= //
void sub_1001bfa0()
{
	g_1008b090.sub_10015e80();
	atexit(sub_1001c1b0);

	g_1008b054.sub_10021f40("LightModelSprites", 0.0f, 0, 0);
	g_1008b034.sub_10021f40("ModelLODOffset", 0.0f, 0, 0);
	g_1008b014.sub_10021f40("ModelLODBlendEnable", 1.0f, 0, 0);
	g_1008aff4.sub_10021f40("ModelLODBlendDist", 60.0f, 0, 0);
	g_1008afd4.sub_10021f40("ExtraFOVXOffset", 0.0f, 0, 0);
	g_1008afb4.sub_10021f40("ExtraFOVYOffset", 0.0f, 0, 0);
	g_1008af94.sub_10021f40("ModelFovTest", 0.0f, 0, 0);
	g_1008af74.sub_10021f40("ModelSunVariance", 0.05f, 0, 0);
	g_1008af54.sub_10021f40("ModelApplySun", 1.0f, 0, 0);
	g_1008af34.sub_10021f40("ModelSaturation", 2.0f, 0, 0);
	g_1008af14.sub_10021f40("DrawModelsRigid", 1.0f, 0, 0);
	g_1008aef4.sub_10021f40("ModelCacheRigid", 0.0f, 0, 0);
	g_1008aed4.sub_10021f40("ModelUseTnL", 0.0f, 0, 0);
}
