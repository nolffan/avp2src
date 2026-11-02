// *********************************************************************** //
//  w57_10009860 -- d3d.ren: the static initialiser 0x10009860
//  (512 bytes, init-table entry .data 0x10089010) and its four atexit
//  destructors 0x10009a60 / 0x10009a80 / 0x10009aa0 / 0x10009ac0, /O2.
//
//  The shipped TU is the model vertex-buffer one (0x10009ae0, w2's
//  sub_10009ae0, follows it).  Its file-scope objects, in construction
//  order, read off the bytes:
//
//    * console variable "ModelMinTri" 0.2 at 0x1008ab90, through the
//      registrar 0x10021f40 -- the CONSTRUCTOR here (w4_10021cb0_os.cpp
//      writes the body as CD3DConVar::sub_10021f44; link/aliases.rsp's w57_*
//      block maps the constructor's name onto it);
//    * four model vertex-buffer banks, 0x1008ad4c, 0x1008ad10, 0x1008acd4,
//      0x1008ac98: the base constructor called out of line through its
//      stack entry 0x1007dc50 (w35's C1007dc54), then the derived vptr --
//      0x10089260 / 0x10089280 / 0x100892a0 / 0x100892c0 -- and +0x28 =
//      0, 0, 1, 1 inline, and atexit(<destructor>) after each;
//    * seven more console variables, ModelDetailTextureScale 3.0 down to
//      ModelVBCacheDelay 4.0.
//
//  Each atexit destructor is `mov $obj,%ecx ; call 0x1007dcc0`: the
//  derived destructor is empty and the base one is called out of line.
//  The base destructor is VIRTUAL -- slot 0 of all four vtables is
//  0x10003770, a scalar deleting destructor -- and declaring it so makes
//  icl emit ??_GCModelVBBank@@UAEPAXI@Z, which is byte-identical to
//  0x10003770 (d3dr5a_o2.cpp's hand stand-in for it is deleted).  The
//  class name carries no address on purpose: a ??_G named after one
//  (I100039d0) claims that address in the data tables and the vtables
//  could not be placed.
//
//  The vptr is stored explicitly (the shipped tables are d3ddata.cpp's
//  g_vtbl_*, holding the real slots; icl's own ??_7 for this class has
//  only the destructor), as w35 stores 0x10089a20.  The four classes of
//  the shipped image (four vtables, two of them byte-identical pairs) are
//  one class here with the table as a constructor argument: the bytes
//  are the same and nothing else distinguishes them in this TU.
//
//  IDENTICAL: the initialiser, all four destructors and 0x10003770.
//  New globals (defined here, previously d3ddata's): 0x1008ab90,
//  0x1008ac70, 0x1008ac50, 0x1008ac30, the four banks and the four
//  ModelVB* variables; referenced: g_vtbl_10089260/280/2a0/2c0 (new).
// *********************************************************************** //

#include "ltbasedefs.h"

struct CConVar
{
	CConVar(const char* pszName, float fDefault, int32* pnValue,
	        float* pfValue);                            // 0x10021f40

	int32       m_nValue00;                             // +0x00
	float       m_fValue04;                             // +0x04
	float       m_fDefault08;                           // +0x08
	int32*      m_pnValue0c;                            // +0x0c
	float*      m_pfValue10;                            // +0x10
	const char* m_pszName14;                            // +0x14
	void*       m_pHandle18;                            // +0x18
	CConVar*    m_pNext1c;                              // +0x1c
};

extern void* g_vtbl_10089260[];                         // 0x10089260
extern void* g_vtbl_10089280[];                         // 0x10089280
extern void* g_vtbl_100892a0[];                         // 0x100892a0
extern void* g_vtbl_100892c0[];                         // 0x100892c0

struct C1007dc54
{
	C1007dc54();                                        // 0x1007dc50
	virtual ~C1007dc54();                               // 0x1007dcc0

	uint8		m_pad_04[0x28 - 0x04];                  // +0x04
	uint32		m_n28;                                  // +0x28
	uint8		m_pad_2c[0x3c - 0x2c];                  // +0x2c
};

struct CModelVBBank : public C1007dc54
{
	CModelVBBank(void** pVTable, uint32 n28)
	{
		*(void***)this = pVTable;
		m_n28 = n28;
	}
};

CConVar   g_1008ab90("ModelMinTri", 0.2f, LTNULL, LTNULL);   // 0x1008ab90
CModelVBBank g_1008ad4c(g_vtbl_10089260, 0);               // 0x1008ad4c
CModelVBBank g_1008ad10(g_vtbl_10089280, 0);               // 0x1008ad10
CModelVBBank g_1008acd4(g_vtbl_100892a0, 1);               // 0x1008acd4
CModelVBBank g_1008ac98(g_vtbl_100892c0, 1);               // 0x1008ac98
CConVar   g_1008ac70("ModelDetailTextureScale", 3.0f, LTNULL, LTNULL); // 0x1008ac70
CConVar   g_1008ac50("SpecularPowerTest", 0.0f, LTNULL, LTNULL);      // 0x1008ac50
CConVar   g_1008ac30("SpecularScaleTest", 0.0f, LTNULL, LTNULL);      // 0x1008ac30
CConVar   g_1008ac10("ModelVBSize", 126.0f, LTNULL, LTNULL);          // 0x1008ac10
CConVar   g_1008abf0("ModelVBCount", 1.0f, LTNULL, LTNULL);           // 0x1008abf0
CConVar   g_1008abd0("ModelVBCache", 128.0f, LTNULL, LTNULL);         // 0x1008abd0
CConVar   g_1008abb0("ModelVBCacheDelay", 4.0f, LTNULL, LTNULL);      // 0x1008abb0
