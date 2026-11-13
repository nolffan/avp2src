// *********************************************************************** //
//  ph/lmdynamic.cpp -- Primal Hunt's d3d.ren, TU36 (0x100331d1..0x10033782),
//  /O1.
//
//  The dynamic-light lightmaps (LMDynamic, LMDynamicScale, LMDynamicSize):
//  a light's falloff sphere cut by a polygon's plane, drawn into a 16- or
//  32-bit lightmap; and the two scratch-buffer caches the lightmap code
//  (TU37, TU38) draws into.  .bss 0x1007c1e4..0x1007c640.
//
//  The two lightmap decompressors (0x10033277, 0x100332db) fill with
//  `rep stos` in an object that otherwise calls the CRT's memset: VC6's
//  loop optimiser turns `for(; n != 0; n--) *p++ = v;` into a dword fill,
//  or, for bytes, the intrinsic memset's sequence.
// *********************************************************************** //

#include "ltbasedefs.h"

#include <string.h>


// The renderer's console variable (TU12's, constructor 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32		m_nValue;
	float		m_fValue;
	float		m_fDefault;
	int32*		m_pnValue;
	float*		m_pfValue;
	const char*	m_pName;
	void*		m_hParam;
	CD3DConVar*	m_pNext;
};

// The lightmap allocator (vtable: +0x04 Alloc, +0x08 Free).
class CLMAllocator
{
public:
	virtual void	Reserved();
	virtual void*	Alloc(uint32 width, uint32 height);
	virtual void	Free(void *pData);
};

// pixelformat.cpp's PFormat, as far as it is used here.
enum BPPIdent
{
	BPP_8P=0,
	BPP_8,
	BPP_16,
	BPP_32
};

class PFormat
{
public:
	virtual void	Init(BPPIdent Type, uint32 aMask, uint32 rMask, uint32 gMask, uint32 bMask);
	BPPIdent		GetType();

	uint32			m_eType;
	uint32			m_Masks[4];
	uint32			m_nBits[4];
	uint32			m_FirstBits[4];
};

// The lightmap being drawn into.
struct LMDrawDesc
{
	void		*m_pData;		// +0x00
	uint32		m_Pitch;		// +0x04, bytes
	uint32		m_Reserved;
	PFormat		m_Format;		// +0x0c
	uint32		m_Width;		// +0x44
	uint32		m_Height;		// +0x48
};

struct LMPlane
{
	LTVector	m_Normal;
	float		m_Dist;
};

struct LMPoly
{
	uint8		m_Pad[0x28];
	LMPlane		*m_pPlane;		// +0x28
};

// The light object (its +0x38 slot answers the radius).
class LMLightObject
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual void	Slot0c();
	virtual void	Slot10();
	virtual void	Slot14();
	virtual void	Slot18();
	virtual void	Slot1c();
	virtual void	Slot20();
	virtual void	Slot24();
	virtual void	Slot28();
	virtual void	Slot2c();
	virtual void	Slot30();
	virtual void	Slot34();
	virtual float	GetRadius(LMLightObject *pObj);

	uint8		m_Pad[0x94 - 4];
	uint8		m_ColorR;		// +0x94
	uint8		m_ColorG;		// +0x95
	uint8		m_ColorB;		// +0x96
};

struct LMDynamicLight
{
	LMDynamicLight	*m_pNext;
	LMLightObject	*m_pObject;	// +0x04
	LTVector		m_Pos;		// +0x08
};

struct LMColor
{
	uint8	b, g, r, a;
};

// d3d_tables.cpp's a * b / 255 table.
class CMulTable
{
public:
	uint8	m_Table[256][256];
};
extern CMulTable g_MulTable;


CLMAllocator	*g_pLMAllocator;		// 0x1007c1e4

static void		*g_pLMBuffer;			// 0x1007c1e8
static uint32	g_LMBufferWidth;		// 0x1007c1ec
static uint32	g_LMBufferHeight;		// 0x1007c1f0

static void		*g_pLMBuffer2;			// 0x1007c1f4
static uint32	g_LMBuffer2Width;		// 0x1007c1f8
static uint32	g_LMBuffer2Height;		// 0x1007c1fc


// 0x100331d1
CLMAllocator* d3d_GetLMAllocator()
{
	return g_pLMAllocator;
}

// 0x100331d7
void* d3d_GetLMBuffer(uint32 width, uint32 height)
{
	if(!g_pLMBuffer || width != g_LMBufferWidth || height != g_LMBufferHeight)
	{
		g_pLMAllocator->Free(g_pLMBuffer);
		g_pLMBuffer = g_pLMAllocator->Alloc(width, height);
		g_LMBufferWidth = width;
		g_LMBufferHeight = height;
	}

	return g_pLMBuffer;
}

// 0x10033227
void* d3d_GetLMBuffer2(uint32 width, uint32 height)
{
	if(!g_pLMBuffer2 || width != g_LMBuffer2Width || height != g_LMBuffer2Height)
	{
		g_pLMAllocator->Free(g_pLMBuffer2);
		g_pLMBuffer2 = g_pLMAllocator->Alloc(width, height);
		g_LMBuffer2Width = width;
		g_LMBuffer2Height = height;
	}

	return g_pLMBuffer2;
}


// 0x10033277 -- a 32-bit lightmap's runs: each dword is a colour, and one
// with the top bit set is followed by a byte count of it.  At most 0x400
// texels.
LTBOOL d3d_DecompressLM32(uint8 *pIn, uint32 dataLen, uint32 *pOut)
{
	uint8 *pCur, *pEnd;
	uint32 color;
	uint8 count;
	int32 nLeft;

	if(!pIn)
		return LTFALSE;

	nLeft = 0x400;
	pCur = pIn;
	pEnd = pIn + dataLen;
	while(pCur < pEnd)
	{
		color = *(uint32*)pCur;
		pCur += 4;

		if(color & 0x80000000)
		{
			count = *pCur;
			color &= 0x7FFFFFFF;
			pCur++;
		}
		else
		{
			count = 1;
		}

		nLeft -= count;
		if(nLeft < 0)
			return LTFALSE;

		for(; count != 0; count--)
			*pOut++ = color;
	}

	return LTTRUE;
}

// 0x100332db -- a byte mask's runs: each byte is a run length, the runs
// alternately 0 and 0xFF, starting with 0.  At most 0x400 bytes.
LTBOOL d3d_DecompressLMMask(uint8 *pIn, uint32 dataLen, uint8 *pOut)
{
	uint8 vals[2], val;
	uint8 *pNext, *pEnd;
	uint32 iVal, count, i;

	if(!pIn)
		return LTFALSE;

	vals[1] = 0xFF;
	vals[0] = 0;
	pEnd = pOut + 0x400;
	iVal = 0;
	while(dataLen)
	{
		dataLen--;
		count = *pIn++;

		pNext = pOut + count;
		if(pNext > pEnd)
			return LTFALSE;

		val = vals[iVal];
		for(i=count; i != 0; i--)
			*pOut++ = val;

		iVal = !iVal;
	}

	return LTTRUE;
}

CD3DConVar g_CV_LMDynamic("LMDynamic", 1.0f, LTNULL, LTNULL);				// 0x1007c220
CD3DConVar g_CV_LMDynamicScale("LMDynamicScale", 2.5f, LTNULL, LTNULL);	// 0x1007c200
CD3DConVar g_CV_LMDynamicSize("LMDynamicSize", 16.0f, LTNULL, LTNULL);	// 0x1007c640


void d3d_FillDynamicLM16(uint16 *pData, LMDrawDesc *pDesc, float fDistSqr, LMColor *pColor, uint32 unused);
void d3d_FillDynamicLM32(uint32 *pData, LMDrawDesc *pDesc, float fDistSqr, LMColor *pColor, uint32 unused);

// 0x100333c0
LTBOOL d3d_DrawDynamicLM(LMDrawDesc *pDesc, LMPoly *pPoly, LMDynamicLight *pLight, float fScale)
{
	void *pData;
	LTVector vPos;
	LMPlane *pPlane;
	float fDist, fT, fMax;
	LMColor color;

	if(!g_CV_LMDynamic.m_nValue)
		return LTFALSE;

	vPos = pLight->m_Pos;
	pPlane = pPoly->m_pPlane;
	fDist = (vPos.x * pPlane->m_Normal.x + vPos.y * pPlane->m_Normal.y + vPos.z * pPlane->m_Normal.z) - pPlane->m_Dist;
	fT = fDist / pLight->m_pObject->GetRadius(pLight->m_pObject);

	fMax = 1.0f - 1.0f / (float)pDesc->m_Width;
	if(fT > fMax)
		return LTFALSE;

	fDist = fT * fT;

	color.r = LTMAX(pLight->m_pObject->m_ColorR, 127) - 127;
	color.g = LTMAX(pLight->m_pObject->m_ColorG, 127) - 127;
	color.b = LTMAX(pLight->m_pObject->m_ColorB, 127) - 127;

	if(fScale != 1.0f)
	{
		color.r = (uint8)(color.r * fScale);
		color.g = (uint8)(color.g * fScale);
		color.b = (uint8)(color.b * fScale);
	}

	// RETAIL BUG: color.a is never set, so this tests a stale byte too.
	if(!*(uint32*)&color)
		return LTFALSE;

	pData = pDesc->m_pData;
	if(pDesc->m_Format.GetType() == 2)
		d3d_FillDynamicLM16((uint16*)pData, pDesc, fDist, &color, 0);
	else
		d3d_FillDynamicLM32((uint32*)pData, pDesc, fDist, &color, 0);

	return LTTRUE;
}


// 0x100335fd
inline uint16 d3d_LitPixel16(float fLit, LMColor *pColor)
{
	uint8 scale = (uint8)(fLit * 255.0f);

	return (uint16)((((g_MulTable.m_Table[scale][pColor->r] & 0xF8) << 5 |
		(g_MulTable.m_Table[scale][pColor->g] & 0xF8)) << 2) |
		(g_MulTable.m_Table[scale][pColor->b] >> 3));
}

// 0x10033512
void d3d_FillDynamicLM16(uint16 *pData, LMDrawDesc *pDesc, float fDistSqr, LMColor *pColor, uint32 unused)
{
	uint32 pitch, width, height, xCount, yCount;
	float fStepX, fStepY, fYSqr, fX, fY;
	uint16 *pLine;

	pitch = pDesc->m_Pitch >> 1;
	width = pDesc->m_Width;
	height = pDesc->m_Height;
	// RETAIL BUG (probable): m_Pitch is in bytes, so this clears only the
	// first quarter of the lightmap; pixels outside the sphere below it keep
	// whatever was there.
#ifdef AVP2SRC_MODERN
	memset(pData, 0, pDesc->m_Pitch * height);
#else
	memset(pData, 0, (pDesc->m_Pitch * height) >> 2);
#endif

	width -= 2;
	height -= 2;
	fStepY = 2.0f / (float)height;
	fStepX = 2.0f / (float)width;

	pLine = pData + pitch + 1;
	fY = -1.0f;
	for(yCount=height; yCount; yCount--)
	{
		fYSqr = fY * fY + fDistSqr;

		fX = -1.0f;
		for(xCount=0; xCount < width; xCount++)
		{
			float fD = fX * fX + fYSqr;
			if(fD < 1.0f)
				pLine[xCount] = d3d_LitPixel16(1.0f - fD, pColor);

			fX += fStepX;
		}

		fY += fStepY;
		pLine += pitch;
	}
}

// 0x1003373e
inline uint32 d3d_LitPixel32(float fLit, LMColor *pColor)
{
	uint8 scale = (uint8)(fLit * 255.0f);

	return (g_MulTable.m_Table[scale][pColor->r] << 16) |
		(g_MulTable.m_Table[scale][pColor->g] << 8) |
		g_MulTable.m_Table[scale][pColor->b];
}

// 0x10033652
void d3d_FillDynamicLM32(uint32 *pData, LMDrawDesc *pDesc, float fDistSqr, LMColor *pColor, uint32 unused)
{
	uint32 pitch, width, height, xCount, yCount;
	float fStepX, fStepY, fYSqr, fX, fY;
	uint32 *pLine;

	pitch = pDesc->m_Pitch >> 2;
	width = pDesc->m_Width;
	height = pDesc->m_Height;
	// RETAIL BUG (probable): m_Pitch is in bytes, so this clears only the
	// first quarter of the lightmap; pixels outside the sphere below it keep
	// whatever was there.
#ifdef AVP2SRC_MODERN
	memset(pData, 0, pDesc->m_Pitch * height);
#else
	memset(pData, 0, (pDesc->m_Pitch * height) >> 2);
#endif

	width -= 2;
	height -= 2;
	fStepY = 2.0f / (float)height;
	fStepX = 2.0f / (float)width;

	pLine = pData + pitch + 1;
	fY = -1.0f;
	for(yCount=height; yCount; yCount--)
	{
		fYSqr = fY * fY + fDistSqr;

		fX = -1.0f;
		for(xCount=0; xCount < width; xCount++)
		{
			float fD = fX * fX + fYSqr;
			if(fD < 1.0f)
				pLine[xCount] = d3d_LitPixel32(1.0f - fD, pColor);

			fX += fStepX;
		}

		fY += fStepY;
		pLine += pitch;
	}
}
