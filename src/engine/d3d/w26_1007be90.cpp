// *********************************************************************** //
//
//  MODULE:   W26_1007BE90 (.CPP)  --  VisibleSet::Init
//
//  0x1007be90..0x1007c136 of the retail d3d.ren (679 bytes), written for the
//  d3d-link: Init (0x100215a0, w26_10020fd0_osoiqip.cpp) calls it on the
//  renderer root and fails with "VisibleSet::Init failed (invalid object
//  list size?)." when it answers zero.  /O2: the body pads to 16 with
//  `mov %esi,%esi ; lea 0x0(%edi,%eiz,1),%edi`, and the frame is the /O2
//  `push %ebp ; mov %esp,%ebp ; sub ; mov %esi,0x4(%esp) ; mov %ebx,(%esp)`.
//
//  WHAT PINS IT:
//    * `mov %ecx,%ebx` on entry and a bare `ret`: __thiscall, no
//      arguments.  The caller (0x100219e5) is `call 0x1007b8d0 ;
//      mov %eax,%ecx ; call 0x1007be90` -- the 0x10091a10 root.
//    * fifteen calls to 0x1007c140 (d3dbatchc_o2.cpp's
//      C1007c140::sub_1007c140(pOwner, pszVarName, nDefaultCount)), each
//      with %ecx = this + 0x144 + 0x20*i, pOwner = this, one of the fifteen
//      "VS_*" names at 0x100abdc0..0x100abf00 and the default list size
//      (0x80 x5, 0x40 x3, 0x28, 0x20 x3, 0x40, 0x20 x2).  The outgoing
//      area is re-reserved before each and never restored: callee-pops.
//    * each answer is folded as `test ; sete ; or` into one failure word
//      (the first spilled to -0x4(%ebp) and reloaded into %esi); on failure
//      the object's own link at +0x00 is tied off (+0x04 then +0x00, the
//      `m_pPrev = m_pNext = this` order), and the answer is !failure.
//    * the members are 0x20 apart: C1007c140 is 0x1c bytes of fields
//      (d3dbatchc_o2.cpp / w10_1007c250.cpp) plus padding to the stride;
//      the fields ahead of +0x144 are not touched here.
//
// *********************************************************************** //

#include "ltbasedefs.h"


class C1007c140
{
public:
	LTBOOL sub_1007c140(void* pOwner, const char* pszVarName,
	                    int32 nDefaultCount);           // 0x1007c140

	uint8   m_Bytes[0x20];                              // +0x00..+0x1f
};

struct VSLink
{
	VSLink* m_pPrev;                                    // +0x00
	VSLink* m_pNext;                                    // +0x04
};

class VisibleSet
{
public:
	LTBOOL  sub_1007be90();                             // Init

	VSLink      m_Link;                                 // +0x000
	uint8       m_Reserved008[0x144 - 0x008];           // +0x008..+0x143
	C1007c140   m_Models;                               // +0x144
	C1007c140   m_ModelsTranslucent;                    // +0x164
	C1007c140   m_ModelsChromakey;                      // +0x184
	C1007c140   m_Sprites;                              // +0x1a4
	C1007c140   m_SpritesNoZ;                           // +0x1c4
	C1007c140   m_WorldModels;                          // +0x1e4
	C1007c140   m_WorldModelsTranslucent;               // +0x204
	C1007c140   m_WorldModelsChromakey;                 // +0x224
	C1007c140   m_Lights;                               // +0x244
	C1007c140   m_PolyGrids;                            // +0x264
	C1007c140   m_PolyGridsTranslucent;                 // +0x284
	C1007c140   m_LineSystems;                          // +0x2a4
	C1007c140   m_ParticleSystems;                      // +0x2c4
	C1007c140   m_Canvases;                             // +0x2e4
	C1007c140   m_CanvasesTranslucent;                  // +0x304
};


// ======================================================================= //
//  0x1007be90  679 bytes.
// ======================================================================= //
LTBOOL VisibleSet::sub_1007be90()
{
	uint32 bFail;

	bFail  = !m_Models.sub_1007c140(this, "VS_MODELS", 0x80);
	bFail |= !m_ModelsTranslucent.sub_1007c140(this, "VS_MODELS_TRANSLUCENT", 0x80);
	bFail |= !m_ModelsChromakey.sub_1007c140(this, "VS_MODELS_CHROMAKEY", 0x80);
	bFail |= !m_Sprites.sub_1007c140(this, "VS_SPRITES", 0x80);
	bFail |= !m_SpritesNoZ.sub_1007c140(this, "VS_SPRITES_NOZ", 0x80);
	bFail |= !m_WorldModels.sub_1007c140(this, "VS_WORLDMODELS", 0x40);
	bFail |= !m_WorldModelsTranslucent.sub_1007c140(this, "VS_WORLDMODELS_TRANSLUCENT", 0x40);
	bFail |= !m_WorldModelsChromakey.sub_1007c140(this, "VS_WORLDMODELS_CHROMAKEY", 0x40);
	bFail |= !m_Lights.sub_1007c140(this, "VS_LIGHTS", 0x28);
	bFail |= !m_PolyGrids.sub_1007c140(this, "VS_POLYGRIDS", 0x20);
	bFail |= !m_PolyGridsTranslucent.sub_1007c140(this, "VS_POLYGRIDS_TRANSLUCENT", 0x20);
	bFail |= !m_LineSystems.sub_1007c140(this, "VS_LINESYSTEMS", 0x20);
	bFail |= !m_ParticleSystems.sub_1007c140(this, "VS_PARTICLESYSTEMS", 0x40);
	bFail |= !m_Canvases.sub_1007c140(this, "VS_CANVASES", 0x20);
	bFail |= !m_CanvasesTranslucent.sub_1007c140(this, "VS_CANVASES_TRANSLUCENT", 0x20);

	if (bFail)
	{
		m_Link.m_pPrev = m_Link.m_pNext = &m_Link;
	}

	return !bFail;
}
