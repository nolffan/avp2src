// ======================================================================= //
//  w27_10059c80_os.cpp -- d3d.ren: 0x10059c80, the static
//  initialiser of DrawPolyMgr's TU.  /Os (`xor %eax,%eax ; inc %eax`
//  for 1, the `push %esi ; push %ebx` saves, 0x90 padding).
//
//  623 bytes, reached only through the CRT's initialiser table.  In
//  source order it is the TU's five globals:
//
//    0x1008fe38  "DetailTextureScale"  console variable, 0.2f
//    0x1008fe18  "DrawPolyMgr"         console variable, 0.0f
//    0x1008f620  the DrawPolyMgr itself, constructed in place, and
//                atexit(0x10059ef0) -- its destructor thunk (d3dr5b.cpp)
//    0x1008f600  "TestGouraud"         console variable, 0.0f
//    0x1008f5e0  "TestLightmap"        console variable, 1.0f
//
//  (each console variable's constructor is the out-of-line 0x10021f40,
//  d3dr5a.cpp's Bank10021f40::sub_10021f40).  The DrawPolyMgr
//  constructor is expanded: its built-in material (name, pass count,
//  three passes of three stages, the node list), the material's own node
//  (three node passes: 32 bucket heads, the poly list head, three stage
//  state blocks), the saved-vertex CMoArray at +0x7bc (vtable
//  0x100897a0) and the node ObjectBank at +0x7d4 (vtable 0x100897d0,
//  sb_Init2(0x28, 32, 0)), and then its body: the name "TEST_GOURAUD",
//  the default material's flat state, the node tied to the material and
//  made the list.
//
//  Written as the explicit function it compiles to (as d3dr5a.cpp does
//  for the console-variable initialisers): each part's constructor is an
//  inline Construct() on a view of the global, so every field is
//  addressed absolutely as in the shipped bytes (placement new instead
//  put `this` in a register and kept the null test).  Measured, each:
//    * the member arrays are constructed by countdown loops over a
//      pointer (`mov $3,%esi ... dec %esi ; jne`); an index `for` gives
//      `cmp ; jb` instead.  The 32 bucket heads ARE an index `for` (the
//      `cmp ; ja` pointer loop);
//    * icl emits a run of constant stores in the REVERSE of source order,
//      so the source orders below are the shipped stores reversed (the
//      pass's 0x1c, 0x18 .. 0x0c, 0x04, 0x08, 0x00 is m_n00, m_n08, m_n04,
//      m_n0c ..), and the chained self-links put the prev store second;
//    * the CMoArray's Init() is the template's own (dynarray.h): its
//      Term() / _InitArray() / SetSize(0) are why the shipped code clears
//      the array's four dwords twice and m_nElements a third time --
//      the same stores written out by hand are merged away.
//  0x1008fe38 / 0x1008fe18 / 0x1008f600 / 0x1008f5e0 as Bank10021f40 and
//  0x1008f620 as DrawPolyMgr59c80 are new names.
// ======================================================================= //
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"
#include <string.h>

// The console variable (d3dr5a.cpp's tag).
struct Bank10021f40
{
	void	sub_10021f40(const char* pszName, float fValue,
	                     uint32 nA, uint32 nB);         // 0x10021f40
};

extern Bank10021f40		g_1008fe38;                     // 0x1008fe38  "DetailTextureScale"
extern Bank10021f40		g_1008fe18;                     // 0x1008fe18  "DrawPolyMgr"
extern Bank10021f40		g_1008f600;                     // 0x1008f600  "TestGouraud"
extern Bank10021f40		g_1008f5e0;                     // 0x1008f5e0  "TestLightmap"

extern void sub_10059ef0();                             // 0x10059ef0  its destructor thunk (d3dr5b.cpp)

extern void* const g_vtbl_100897a0[];                   // 0x100897a0  CMoArray<SavedVert589c4>
extern void* const g_vtbl_100897d0[];                   // 0x100897d0  ObjectBank<Node56ab4>

// sb_Init2 (struct_bank.h) is 0x1007fb70.


// ----------------------------------------------------------------------- //
//  The DrawPolyMgr's layout.  Each part's Construct() is its constructor
//  (member constructors first, then the body), written out because the
//  object is a global built in place: the shipped code addresses every
//  field absolutely.
// ----------------------------------------------------------------------- //
struct Stage59c80                                       // 0x24 bytes
{
	void Construct()
	{
		m_n18 = 0;
		m_n1c = 0;
		m_n20 = 0;
	}

	uint8	m_pad_00[0x18];                             // +0x00
	uint32	m_n18;                                      // +0x18
	uint32	m_n1c;                                      // +0x1c
	uint32	m_n20;                                      // +0x20
};

struct Pass59c80                                        // 0x8c bytes
{
	void Construct()
	{
		Stage59c80*	pStage = m_Stages;
		uint32		n      = 3;

		do { pStage->Construct(); pStage++; } while (--n);

		m_n00 = 0;
		m_n08 = 0;
		m_n04 = 0;
		m_n0c = 0;
		m_n10 = 0;
		m_n14 = 0;
		m_n18 = 0;
		m_n1c = 0;
	}

	uint32		m_n00, m_n04, m_n08, m_n0c;             // +0x00
	uint32		m_n10, m_n14, m_n18, m_n1c;             // +0x10
	Stage59c80	m_Stages[3];                            // +0x20
};

struct States59c80                                      // 0x18 bytes
{
	void Construct()
	{
		m_nColorOp = 1;
		m_nAlphaOp = 1;
	}

	uint32	m_nColorOp;                                 // +0x00
	uint32	m_nColorArg1;                               // +0x04
	uint32	m_nColorArg2;                               // +0x08
	uint32	m_nAlphaOp;                                 // +0x0c
	uint32	m_nAlphaArg1;                               // +0x10
	uint32	m_nAlphaArg2;                               // +0x14
};

struct Link59c80
{
	Link59c80*	m_pPrev;                                // +0x00
	Link59c80*	m_pNext;                                // +0x04
	void*		m_pData;                                // +0x08
};

struct NodePass59c80                                    // 0x1dc bytes
{
	void Construct()
	{
		uint32 i;

		States59c80*	pStates = m_States;
		uint32			n       = 3;

		do { pStates->Construct(); pStates++; } while (--n);

		for (i = 0; i < 32; i++)
			m_Buckets[i].m_pPrev = m_Buckets[i].m_pNext = &m_Buckets[i];

		m_List.m_pPrev = m_List.m_pNext = &m_List;

		m_States[0].m_nColorOp   = 4;
		m_States[0].m_nColorArg1 = 2;
		m_States[0].m_nColorArg2 = 0;
		m_States[0].m_nAlphaOp   = 4;
		m_States[0].m_nAlphaArg1 = 2;
		m_States[0].m_nAlphaArg2 = 0;
	}

	Link59c80	m_Buckets[32];                          // +0x000
	uint8		m_pad_180[0x188 - 0x180];               // +0x180
	Link59c80	m_List;                                 // +0x188
	States59c80	m_States[3];                            // +0x194
};

struct Node59c80                                        // 0x59c bytes
{
	void*			m_pMat;                             // +0x00
	Node59c80*		m_pNext;                            // +0x04
	NodePass59c80	m_Passes[3];                        // +0x08
};

// The saved vertex (w27_10056ab0_osqipni.cpp's tag, so the same
// CMoArray instantiation).
struct SavedVert589c4                                   // 0x10 bytes
{
	LTVector	m_Pos;                                  // +0x00
	uint32		m_n0c;                                  // +0x0c
};

// CMoArray<SavedVert589c4> (dynarray.h): vptr, m_pArray, m_nElements and
// DefaultCache's two dwords.
struct Array59c80
{
	void* const*	m_pVtbl;                            // +0x00
	void*			m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	uint32			m_nCacheSize;                       // +0x0c
	uint32			m_nWantedCache;                     // +0x10
};

// ObjectBank<Node56ab4> (object_bank.h): vptr, the StructBank.
struct Bank59c80
{
	void* const*	m_pVtbl;                            // +0x00
	StructBank		m_Bank;                             // +0x04
};

struct DrawPolyMgr59c80
{
	void Construct()
	{
		Pass59c80*		pPass;
		NodePass59c80*	pNodePass;
		uint32			n;

		// the material
		pPass = m_Passes;
		n     = 3;
		do { pPass->Construct(); pPass++; } while (--n);
		m_szName[0] = 0;
		m_nPasses   = 0;
		m_pNodes    = 0;

		// its node
		pNodePass = m_Node.m_Passes;
		n         = 3;
		do { pNodePass->Construct(); pNodePass++; } while (--n);

		// the saved-vertex array: CMoArray's constructor, Clear() then
		// Init() -- the vptr and Clear() through the array's layout (both
		// are the class's own), Init() the template's.
		((Array59c80*)&m_SavedVerts)->m_pVtbl        = g_vtbl_100897a0;
		((Array59c80*)&m_SavedVerts)->m_nWantedCache = 0;
		((Array59c80*)&m_SavedVerts)->m_nCacheSize   = 0;
		((Array59c80*)&m_SavedVerts)->m_nElements    = 0;
		((Array59c80*)&m_SavedVerts)->m_pArray       = 0;

		m_Node.m_pNext = 0;
		m_Node.m_pMat  = 0;

		m_SavedVerts.Init();

		// the node bank
		m_NodeBank.m_pVtbl = g_vtbl_100897d0;
		sb_Init2(&m_NodeBank.m_Bank, 0x28, 32, 0);

		m_pNodes76c = 0;

		// the body
		strcpy(m_szName, "TEST_GOURAUD");

		m_Passes[0].m_n1c = m_Passes[0].m_n10 = m_nPasses = 1;
		m_Passes[0].m_n00 = m_Passes[0].m_n14 = m_Passes[0].m_Stages[0].m_n18 =
		    m_Passes[0].m_Stages[0].m_n1c = 0;

		m_Node.m_pNext = 0;
		m_pNodes       = &m_Node;
		m_Node.m_pMat  = this;
		m_pNodes76c    = &m_Node;
	}

	char			m_szName[0x20];                     // +0x000
	uint32			m_nPasses;                          // +0x020
	Pass59c80		m_Passes[3];                        // +0x024
	Node59c80*		m_pNodes;                           // +0x1c8
	Node59c80		m_Node;                             // +0x1cc
	uint16			m_wFrame768;                        // +0x768
	uint8			m_pad_76a[0x76c - 0x76a];           // +0x76a
	Node59c80*		m_pNodes76c;                        // +0x76c
	uint8			m_pad_770[0x7bc - 0x770];           // +0x770
	CMoArray<SavedVert589c4>	m_SavedVerts;       // +0x7bc
	uint32			m_nSaved7d0;                        // +0x7d0
	Bank59c80		m_NodeBank;                         // +0x7d4
};

extern DrawPolyMgr59c80	g_1008f620;                     // 0x1008f620  the DrawPolyMgr

extern "C" int __cdecl atexit(void (__cdecl* pfn)(void));


// ======================================================================= //
//  0x10059c80 -- 623 bytes (`ret`, then `nop`).
// ======================================================================= //
void sub_10059c80()
{
	g_1008fe38.sub_10021f40("DetailTextureScale", 0.2f, 0, 0);
	g_1008fe18.sub_10021f40("DrawPolyMgr", 0.0f, 0, 0);

	g_1008f620.Construct();
	atexit(sub_10059ef0);

	g_1008f600.sub_10021f40("TestGouraud", 0.0f, 0, 0);
	g_1008f5e0.sub_10021f40("TestLightmap", 1.0f, 0, 0);
}
