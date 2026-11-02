// *********************************************************************** //
//  w57_1002db00 -- d3d.ren: the static initialiser 0x1002db00
//  (608 bytes, init-table entry .data 0x10089090), /O2.
//
//  The second console-variable set w28_100256d0_qipni.cpp describes ("THE
//  SECOND initialiser"): fifteen CD3DConVar records registered through
//  0x10021f40 with both destination pointers null (`xor %eax,%eax` into
//  both slots), names and defaults read off the `movl $imm,(%esp)` /
//  `movl $imm,0x4(%esp)` pairs, records 0x1008d704 down to 0x1008d534 --
//  then three stores of 5.0 to 0x1008d530/52c/528, an x/y/z triple with a
//  three-float constructor (w28's g_vLightScaleLast, an FVec3 there; the
//  tag is kept so that spelling is the same symbol's).  The records are
//  allocated in reverse declaration order, as big21f90.cpp found for the
//  first set; the one 0x30 step (0x1008d5e4 -> 0x1008d5b4) is a 0x10-byte
//  object declared between them that has no dynamic initialisation here
//  (d3ddata's view at 0x1008d5d4).
//
//  Written as the file-scope definitions icl generates the initialiser
//  from: CD3DConVar's registrar declared as its constructor (aliased onto
//  w4_10021cb0_os.cpp's CD3DConVar::sub_10021f44, link/aliases.rsp's w57_* block).
//  Named by address: w28's g_varRenderToFront etc. and d3dr3a's
//  g_1008d5e4 / w28's g_1008d574 / g_1008d594 are other spellings that
//  are aliased onto these definitions.  IDENTICAL.
//  New globals (defined here): the fifteen records and 0x1008d528.
// *********************************************************************** //

#include "ltbasedefs.h"

class CD3DConVar
{
public:
	CD3DConVar(const char* pszName, float fDefault, int32* pnValue,
	           float* pfValue);                         // 0x10021f40

	int32       m_nValue00;                             // +0x00 value as int
	float       m_fValue04;                             // +0x04 value as float
	float       m_fDefault08;                           // +0x08 the default
	int32*      m_pnValue0c;                            // +0x0c int32 dest
	float*      m_pfValue10;                            // +0x10 float dest
	const char* m_pszName14;                            // +0x14 the name
	void*       m_pHandle18;                            // +0x18 console handle
	CD3DConVar* m_pNext1c;                              // +0x1c chain
};

struct FVec3
{
	FVec3(float fX, float fY, float fZ)
	{
		x = fX;
		y = fY;
		z = fZ;
	}

	float x;
	float y;
	float z;
};

CD3DConVar g_1008d704("VFogMinY", 0.0f, LTNULL, LTNULL);            // 0x1008d704
CD3DConVar g_1008d6e4("VFogMaxY", 1300.0f, LTNULL, LTNULL);         // 0x1008d6e4
CD3DConVar g_1008d6c4("VFogMinYVal", 0.5f, LTNULL, LTNULL);         // 0x1008d6c4
CD3DConVar g_1008d6a4("VFogMaxYVal", 0.0f, LTNULL, LTNULL);         // 0x1008d6a4
CD3DConVar g_1008d684("VFogMax", 180.0f, LTNULL, LTNULL);           // 0x1008d684
CD3DConVar g_1008d664("VFogDensity", 1800.0f, LTNULL, LTNULL);      // 0x1008d664
CD3DConVar g_1008d644("VFog", 0.0f, LTNULL, LTNULL);                // 0x1008d644
CD3DConVar g_1008d624("DrawWorldTree", -1.0f, LTNULL, LTNULL);      // 0x1008d624
CD3DConVar g_1008d604("DrawTerrainSections", 0.0f, LTNULL, LTNULL); // 0x1008d604
CD3DConVar g_1008d5e4("AlphaTest", 1.0f, LTNULL, LTNULL);           // 0x1008d5e4
CD3DConVar g_1008d5b4("DrawPortals", 1.0f, LTNULL, LTNULL);         // 0x1008d5b4
CD3DConVar g_1008d594("ShowPortalBounds", 0.0f, LTNULL, LTNULL);    // 0x1008d594
CD3DConVar g_1008d574("PortalLightmap", 1.0f, LTNULL, LTNULL);      // 0x1008d574
CD3DConVar g_1008d554("ShowTexInfo", 0.0f, LTNULL, LTNULL);         // 0x1008d554
CD3DConVar g_1008d534("RenderToFront", 0.0f, LTNULL, LTNULL);       // 0x1008d534
FVec3      g_1008d528(5.0f, 5.0f, 5.0f);                            // 0x1008d528
