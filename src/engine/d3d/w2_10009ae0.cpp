// *********************************************************************** //
//
//  w2_10009ae0.cpp -- sub_10009ae0 (written for the d3d.ren link).
//
//  The model vertex-buffer reallocation d3dr5d_os.cpp's notes
//  describe: clamp ModelVBSize / ModelVBCount to at least 1, round the
//  size up to a multiple of 3 (whole triangles), and hand them to the
//  +0x10 slot of the four model vertex-buffer banks at 0x1008ad4c,
//  0x1008ad10, 0x1008acd4, 0x1008ac98 and the model cache at 0x10091d80,
//  then copy ModelVBCacheDelay into the cache.  Called bare (nothing
//  pushed) from d3dr5c.cpp's 0x1001c1d0 and from 0x10008b19.
//
//  The four console variables are the CConVar objects 0x100099c0 builds
//  ("ModelVBSize", "ModelVBCount", "ModelVBCache", "ModelVBCacheDelay",
//  through 0x10021f40); only their integer value at +0x00 is read here.
//
//  /O2: lea-nop padding to 0x10009c90.
//
// *********************************************************************** //

#include "ltbasedefs.h"


struct D3D7;

// d3ddata.cpp's tag for a console variable (0x20 bytes).
struct CConVar
{
	int32       m_nValue00;                                 // +0x00
	float       m_fValue04;                                 // +0x04
	uint8       m_pad_08[0x20 - 0x08];                      // +0x08
};

// A model vertex-buffer bank (d3dr4b.cpp's I100039d0), with the slot
// this function calls: (device, vertex count, buffer count, flag,
// TnLRast), __thiscall.  The model cache at 0x10091d80 has the same
// interface and keeps the cache delay at +0x60.
struct I100039d0
{
	virtual void Slot00();                                  // +0x00
	virtual void Slot04();                                  // +0x04
	virtual void Slot08();                                  // +0x08
	virtual void Slot0c();                                  // +0x0c
	virtual void Init(D3D7* pD3D, int32 nVerts, int32 nBuffers,
	                  int32 bFlag, long bTnL);              // +0x10

	uint8       m_pad_04[0x60 - 0x04];                      // +0x04
	int32       m_nCacheDelay60;                            // +0x60
};

extern CConVar     g_1008ac10;                              // 0x1008ac10  ModelVBSize
extern CConVar     g_1008abf0;                              // 0x1008abf0  ModelVBCount
extern CConVar     g_1008abd0;                              // 0x1008abd0  ModelVBCache
extern CConVar     g_1008abb0;                              // 0x1008abb0  ModelVBCacheDelay

extern I100039d0   g_1008ad4c;                              // 0x1008ad4c
extern I100039d0   g_1008ad10;                              // 0x1008ad10
extern I100039d0   g_1008acd4;                              // 0x1008acd4
extern I100039d0   g_1008ac98;                              // 0x1008ac98
extern I100039d0   g_10091d80;                              // 0x10091d80

extern D3D7*       g_pD3D;                                  // 0x1008d980
extern long        g_CV_TnLRast;                            // 0x1008cb14


// The calls go through the vtable in the shipped code (`mov <obj>,%eax ;
// mov 0x10(%eax),%eax ; call *%eax`); icl devirtualises a call made on
// the object by name, so each is made through a pointer to it.
void sub_10009ae0()
{
	g_1008ac10.m_nValue00 = g_1008ac10.m_nValue00 > 1 ? g_1008ac10.m_nValue00 : 1;
	g_1008abf0.m_nValue00 = g_1008abf0.m_nValue00 > 1 ? g_1008abf0.m_nValue00 : 1;
	g_1008ac10.m_nValue00 = ((g_1008ac10.m_nValue00 + 2) / 3) * 3;

	I100039d0* pBank;

	pBank = &g_1008ad4c;
	pBank->Init(g_pD3D, g_1008ac10.m_nValue00, g_1008abf0.m_nValue00, 0,
	            g_CV_TnLRast);
	pBank = &g_1008ad10;
	pBank->Init(g_pD3D, g_1008ac10.m_nValue00, g_1008abf0.m_nValue00, 0,
	            g_CV_TnLRast);
	pBank = &g_1008acd4;
	pBank->Init(g_pD3D, g_1008ac10.m_nValue00, g_1008abf0.m_nValue00, 1,
	            g_CV_TnLRast);
	pBank = &g_1008ac98;
	pBank->Init(g_pD3D, g_1008ac10.m_nValue00, g_1008abf0.m_nValue00, 1,
	            g_CV_TnLRast);
	pBank = &g_10091d80;
	pBank->Init(g_pD3D, g_1008ac10.m_nValue00, g_1008abd0.m_nValue00, 1,
	            g_CV_TnLRast);

	g_10091d80.m_nCacheDelay60 = g_1008abb0.m_nValue00;
}
