// *********************************************************************** //
//
//  ph/fogvolume.cpp -- Primal Hunt's d3d.ren, TU08: 0x1000f09f..0x1000fef5,
//  /O1.
//
//  Fog volumes: up to four box-shaped volumes (portal objects) a frame,
//  each a box with its six planes; the segment from the camera to a vertex
//  is clipped against them to fog the vertex.  The ray/box tests are this
//  file's own.  The _CVector<float> COMDATs 0x1000fe94..0x1000fef5 are
//  placed here because this is their first out-of-line user.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>
#include <d3d.h>


#define MAX_FOGVOLUMES      4

// The object flags a fog volume reads.
#define FOGFLAG_SETCOLOR    0x1
#define FOGFLAG_DENSITY     0x2


// What a fog volume reads of an object (the engine's LTObject).
class LTObject
{
public:
	virtual void    Slot00();
	virtual void    Slot04();
	virtual void    Slot08();
	virtual void    GetBBox(LTVector* pMin, LTVector* pMax);    // +0x0c

	uint8       m_pad_004[0x90 - 0x04];
	uint32      m_FogFlags;                                 // +0x90
	uint8       m_FogR;                                     // +0x94
	uint8       m_FogG;                                     // +0x95
	uint8       m_FogB;                                     // +0x96
	uint8       m_FogAlpha;                                 // +0x97
};

struct FogColor
{
	uint8   r, g, b;
};

// The view (common_draw.cpp's): its frustum planes and the camera
// position.
struct ViewParams
{
	uint8       m_pad_000[0x3d0];
	LTPlane     m_ClipPlanes[6];                            // +0x3d0
	uint8       m_pad_430[0x4b4 - 0x430];
	LTVector    m_Pos;                                      // +0x4b4
};

// A console variable (common_stuff.cpp's registrar, 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pName;                                    // +0x14
	void*       m_hParam;                                   // +0x18
	CD3DConVar* m_pNext;                                    // +0x1c
};

extern ViewParams           g_ViewParams;                   // 0x1005b790
extern CD3DConVar           g_CV_VFogDensity;               // 0x1005e098 (d3d_draw.cpp)
extern FogColor             g_FogColor;                     // 0x1005dad8
extern int32                g_LastFogR, g_LastFogG, g_LastFogB; // 0x1005dadc..
extern LPDIRECT3DDEVICE7    g_pD3DDevice;                   // 0x100638c8


// 0x1000f09f -- where the segment pA..pB crosses the plane: the point and
// the fraction along the segment.  FALSE if it doesn't.
LTBOOL d3d_IntersectSegmentPlane(const LTPlane* pPlane, const LTVector* pA,
                                 const LTVector* pB, LTVector* pHit, float* pT)
{
	LTVector vDir;
	float fDot, fAbs, t;

	vDir = *pB - *pA;
	fDot = vDir.Dot(pPlane->m_Normal);
	fAbs = (float)fabs(fDot);
	if (fAbs < 0.001f)
		return LTFALSE;

	t = (pPlane->m_Normal * pPlane->m_Dist - *pA).Dot(pPlane->m_Normal) / fDot;
	*pT = t;
	if (t < 0.0f || t > 1.0f)
		return LTFALSE;

	*pHit = *pA + vDir * t;
	return LTTRUE;
}

// 0x1000f1c6
LTBOOL d3d_PointInBox(const LTVector* pPt, const LTVector* pMin,
                      const LTVector* pMax)
{
	if (pPt->x < pMin->x || pPt->y < pMin->y || pPt->z < pMin->z)
		return LTFALSE;

	if (pPt->x > pMax->x || pPt->y > pMax->y || pPt->z > pMax->z)
		return LTFALSE;

	return LTTRUE;
}


// A box, by its centre and half-dimensions, and its six planes.
class CFogBox
{
public:
	CFogBox()                                               // 0x1000f6df
	{
		Init(LTVector(0.0f, 0.0f, 0.0f), LTVector(0.0f, 0.0f, 0.0f));
		m_vMin = LTVector(0.0f, 0.0f, 0.0f);
		m_vMax = LTVector(0.0f, 0.0f, 0.0f);
		UpdatePlanes();
	}

	void    Init(LTVector vCenter, LTVector vDims)
	{
		m_vCenter = vCenter;
		m_vDims = vDims;
	}

	LTBOOL  PointOnFace(const LTVector* pPt, int iFace);    // 0x1000f460
	int     IntersectSegment(const LTVector* pA, const LTVector* pB,
	                         LTVector* pHit1, LTVector* pHit2,
	                         float* pT1, float* pT2);       // 0x1000f500
	int     ClassifyPlane(const LTPlane* pPlane, float fEpsilon); // 0x1000f5dd
	void    UpdatePlanes();                                 // 0x1000f755

	LTVector    m_vDims;                                    // +0x00
	LTVector    m_vCenter;                                  // +0x0c
	LTVector    m_vMin;                                     // +0x18
	LTVector    m_vMax;                                     // +0x24
	LTPlane     m_Planes[6];                                // +0x30
};

struct FogVolume
{
	LTObject*   m_pObject;                                  // +0x00
	CFogBox     m_Box;                                      // +0x04
};


// 0x1000f217 -- does the segment pA..pB touch the box (separating axes)?
LTBOOL d3d_SegmentTouchesBox(const LTVector* pA, const LTVector* pB,
                             const CFogBox* pBox)
{
	LTVector vSeg, vDir, vT;
	float fHalfLen;

	vSeg = *pB - *pA;
	vSeg = vSeg * 0.5f;
	fHalfLen = vSeg.Mag();
	vDir = vSeg * (1.0f / fHalfLen);
	vSeg = vSeg + *pA;
	vT = pBox->m_vCenter - vSeg;

	if (fabs(vT.x) > fabs(vDir.x) * fHalfLen + pBox->m_vDims.x)
		return LTFALSE;
	if (fabs(vT.y) > fabs(vDir.y) * fHalfLen + pBox->m_vDims.y)
		return LTFALSE;
	if (fabs(vT.z) > fabs(vDir.z) * fHalfLen + pBox->m_vDims.z)
		return LTFALSE;

	if (fabs(vT.y * vDir.z - vT.z * vDir.y) >
		pBox->m_vDims.y * fabs(vDir.z) + pBox->m_vDims.z * fabs(vDir.y))
		return LTFALSE;
	if (fabs(vT.z * vDir.x - vT.x * vDir.z) >
		pBox->m_vDims.z * fabs(vDir.x) + pBox->m_vDims.x * fabs(vDir.z))
		return LTFALSE;
	if (fabs(vT.x * vDir.y - vT.y * vDir.x) >
		pBox->m_vDims.y * fabs(vDir.x) + pBox->m_vDims.x * fabs(vDir.y))
		return LTFALSE;

	return LTTRUE;
}

// 0x1000f460 -- is the point within face iFace's rectangle?
LTBOOL CFogBox::PointOnFace(const LTVector* pPt, int iFace)
{
	switch (iFace)
	{
		case 0:
		case 1:
			if (pPt->y < m_vMin.y || pPt->z < m_vMin.z ||
				pPt->y > m_vMax.y || pPt->z > m_vMax.z)
				return LTFALSE;
			break;

		case 2:
		case 3:
			if (pPt->x < m_vMin.x || pPt->z < m_vMin.z ||
				pPt->x > m_vMax.x || pPt->z > m_vMax.z)
				return LTFALSE;
			break;

		case 4:
		case 5:
			if (pPt->x < m_vMin.x || pPt->y < m_vMin.y ||
				pPt->x > m_vMax.x || pPt->y > m_vMax.y)
				return LTFALSE;
			break;
	}

	return LTTRUE;
}

// 0x1000f500 -- where the segment enters and leaves the box: 0 if it
// misses, 1 if it crosses one face, 2 if two (in order along it).
int CFogBox::IntersectSegment(const LTVector* pA, const LTVector* pB,
                              LTVector* pHit1, LTVector* pHit2,
                              float* pT1, float* pT2)
{
	LTVector vTemp;
	float fTemp;
	int i;

	for (i = 0; i < 6; i++)
	{
		if (d3d_IntersectSegmentPlane(&m_Planes[i], pA, pB, pHit1, pT1) == 1 &&
			PointOnFace(pHit1, i) == 1)
			break;
	}

	if (i > 5)
		return 0;

	if (i == 5)
		return 1;

	for (i++; i < 6; i++)
	{
		if (d3d_IntersectSegmentPlane(&m_Planes[i], pA, pB, pHit2, pT2) == 1 &&
			PointOnFace(pHit2, i) == 1)
			break;
	}

	if (i == 6)
		return 1;

	if (*pT1 > *pT2)
	{
		vTemp = *pHit1;
		fTemp = *pT1;
		*pHit1 = *pHit2;
		*pT1 = *pT2;
		*pHit2 = vTemp;
		*pT2 = fTemp;
	}

	return 2;
}

// 0x1000f5dd -- 1 if the box is in front of the plane, 0 if behind it, 2
// if it straddles it.
int CFogBox::ClassifyPlane(const LTPlane* pPlane, float fEpsilon)
{
	LTVector vMin, vMax;
	float fMin, fMax;

	if (pPlane->m_Normal.x < 0.0f)
	{
		vMin.x = m_vMax.x;
		vMax.x = m_vMin.x;
	}
	else
	{
		vMin.x = m_vMin.x;
		vMax.x = m_vMax.x;
	}

	if (pPlane->m_Normal.y < 0.0f)
	{
		vMin.y = m_vMax.y;
		vMax.y = m_vMin.y;
	}
	else
	{
		vMin.y = m_vMin.y;
		vMax.y = m_vMax.y;
	}

	if (pPlane->m_Normal.z < 0.0f)
	{
		vMin.z = m_vMax.z;
		vMax.z = m_vMin.z;
	}
	else
	{
		vMin.z = m_vMin.z;
		vMax.z = m_vMax.z;
	}

	fMax = pPlane->DistTo(vMax);
	fMin = pPlane->DistTo(vMin);

	if (fMax > fEpsilon && fMin > fEpsilon)
		return 1;

	if (fMax < -fEpsilon && fMin < -fEpsilon)
		return 0;

	return 2;
}


// The frame's fog volumes.
FogVolume   g_FogVolumes[MAX_FOGVOLUMES];                   // 0x100590d0
int32       g_nFogVolumes;                                  // 0x1005a320
int32       g_nFogVolumeTests;                              // 0x1005a324


// 0x1000f755 -- the six planes from the centre and dimensions.
void CFogBox::UpdatePlanes()
{
	m_Planes[0].m_Normal.Init(1.0f, 0.0f, 0.0f);
	m_Planes[0].m_Dist = m_Planes[0].m_Normal.Dot(m_vCenter + m_Planes[0].m_Normal * m_vDims.x);

	m_Planes[1].m_Normal.Init(-1.0f, 0.0f, 0.0f);
	m_Planes[1].m_Dist = m_Planes[1].m_Normal.Dot(m_vCenter + m_Planes[1].m_Normal * m_vDims.x);

	m_Planes[2].m_Normal.Init(0.0f, 1.0f, 0.0f);
	m_Planes[2].m_Dist = m_Planes[2].m_Normal.Dot(m_vCenter + m_Planes[2].m_Normal * m_vDims.y);

	m_Planes[3].m_Normal.Init(0.0f, -1.0f, 0.0f);
	m_Planes[3].m_Dist = m_Planes[3].m_Normal.Dot(m_vCenter + m_Planes[3].m_Normal * m_vDims.y);

	m_Planes[4].m_Normal.Init(0.0f, 0.0f, 1.0f);
	m_Planes[4].m_Dist = m_Planes[4].m_Normal.Dot(m_vCenter + m_Planes[4].m_Normal * m_vDims.z);

	m_Planes[5].m_Normal.Init(0.0f, 0.0f, -1.0f);
	m_Planes[5].m_Dist = m_Planes[5].m_Normal.Dot(m_vCenter + m_Planes[5].m_Normal * m_vDims.z);
}

// 0x1000f9f3 -- how much of the fog's depth the segment pA..pB covers.
float d3d_FogVolumeAmount(const LTVector* pA, const LTVector* pB, float fDepth)
{
	return (fDepth - (*pA - *pB).Mag()) / fDepth;
}

// Round to nearest in the current FPU mode (LithTech's RoundFloatToInt).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// 0x1000fa33 -- the fog of a vertex from the volumes between it and the
// camera, into the specular colour's alpha.
void __fastcall d3d_CalcFogVolume(const LTVector* pPos, uint32* pSpecular)
{
	LTVector vHit1, vHit2;
	float fTEnter, fTExit;
	float fDepth, fFog, fVal;
	LTObject* pObject;
	CFogBox* pBox;
	LTBOOL bCameraIn, bPosIn;
	int32 i, nVal;

	((uint8*)pSpecular)[3] = 0xFF;
	g_nFogVolumeTests++;

	for (i = 0; i < g_nFogVolumes; i++)
	{
		pBox = &g_FogVolumes[i].m_Box;
		if (d3d_SegmentTouchesBox(&g_ViewParams.m_Pos, pPos, pBox) == 1)
		{
			pObject = g_FogVolumes[i].m_pObject;
			break;
		}
	}

	if (i >= g_nFogVolumes)
		return;

	bCameraIn = d3d_PointInBox(&g_ViewParams.m_Pos, &pBox->m_vMin, &pBox->m_vMax);
	bPosIn = d3d_PointInBox(pPos, &pBox->m_vMin, &pBox->m_vMax);

	if (pObject->m_FogFlags & FOGFLAG_DENSITY)
	{
		if (!pObject)
			fDepth = 512.0f;
		else
			fDepth = (float)((pObject->m_FogFlags & 0x3fff0) >> 2);
	}
	else
	{
		fDepth = g_CV_VFogDensity.m_fValue;
	}

	if (bCameraIn == 1 && bPosIn == 1)
	{
		fFog = d3d_FogVolumeAmount(&g_ViewParams.m_Pos, pPos, fDepth);
	}
	else if (bCameraIn == 0 && bPosIn == 1)
	{
		if (pBox->IntersectSegment(&g_ViewParams.m_Pos, pPos,
			&vHit1, &vHit2, &fTEnter, &fTExit) == 1)
			fFog = d3d_FogVolumeAmount(&vHit1, pPos, fDepth);
		else
			fFog = d3d_FogVolumeAmount(&vHit1, &vHit2, fDepth);
	}
	else if (bCameraIn == 1 && bPosIn == 0)
	{
		if (pBox->IntersectSegment(&g_ViewParams.m_Pos, pPos,
			&vHit1, &vHit2, &fTEnter, &fTExit) == 1)
			fFog = d3d_FogVolumeAmount(&g_ViewParams.m_Pos, &vHit1, fDepth);
		else
			fFog = d3d_FogVolumeAmount(&vHit1, &vHit2, fDepth);
	}
	else
	{
		pBox->IntersectSegment(&g_ViewParams.m_Pos, pPos,
			&vHit1, &vHit2, &fTEnter, &fTExit);
		fFog = d3d_FogVolumeAmount(&vHit1, &vHit2, fDepth);
	}

	fFog = LTCLAMP(fFog, 0.0f, 1.0f);

	if (pObject->m_FogAlpha < 0xFF)
	{
		fVal = (1.0f - (float)pObject->m_FogAlpha * (1.0f / 255.0f)) * 255.0f;
		fVal += fFog * 255.0f;
		nVal = RoundFloatToInt(fVal);
		if (nVal > 255)
			nVal = 255;
		((uint8*)pSpecular)[3] = (uint8)nVal;
	}
	else
	{
		((uint8*)pSpecular)[3] = (uint8)RoundFloatToInt(fFog * 255.0f);
	}
}

// 0x1000fc43 -- at the start of a frame.
void d3d_ClearFogVolumes()
{
	g_nFogVolumes = 0;
	g_nFogVolumeTests = 0;
}

// 0x1000fc52 -- a portal object: a fog volume, if it's in view and there
// is room for it.
//
// RETAIL BUG: the view test classifies the box six times against the
// first view plane (m_ClipPlanes, not &m_ClipPlanes[i]), so a volume is
// only culled by the near plane.
LTBOOL d3d_ProcessPortalObject(LTObject* pObject)
{
	FogVolume* pVolume;
	LTBOOL bVisible;
	int i;

	if (!pObject || g_nFogVolumes >= MAX_FOGVOLUMES || !pObject->m_FogAlpha)
		return LTFALSE;

	{
		FogVolume volume;
		LTVector vMin, vMax, vDims;

		volume.m_pObject = pObject;
		pObject->GetBBox(&vMin, &vMax);
		vDims = (vMax - vMin) * 0.5f;
		volume.m_Box.m_vDims = vDims;
		volume.m_Box.m_vMin = vMin;
		volume.m_Box.m_vMax = vMax;
		volume.m_Box.m_vCenter = vMin + vDims;
		volume.m_Box.UpdatePlanes();

		bVisible = LTTRUE;
		for (i = 0; i < 6; i++)
		{
			if (!volume.m_Box.ClassifyPlane(g_ViewParams.m_ClipPlanes, 0.001f))
				bVisible = LTFALSE;
		}

		if (!bVisible)
			return LTFALSE;
	}

	pVolume = &g_FogVolumes[g_nFogVolumes++];
	pVolume->m_pObject = pObject;

	{
		LTVector vMin, vMax;

		pObject->GetBBox(&vMin, &vMax);
		pVolume->m_Box.m_vMin = vMin;
		pVolume->m_Box.m_vMax = vMax;
		pVolume->m_Box.m_vDims = (pVolume->m_Box.m_vMax - pVolume->m_Box.m_vMin) * 0.5f;
		pVolume->m_Box.m_vCenter = pVolume->m_Box.m_vMin + pVolume->m_Box.m_vDims;
		pVolume->m_Box.UpdatePlanes();
	}

	if (pObject->m_FogFlags & FOGFLAG_SETCOLOR)
	{
		g_FogColor.r = pObject->m_FogR;
		g_FogColor.g = pObject->m_FogG;
		g_FogColor.b = pObject->m_FogB;
		g_LastFogR = pObject->m_FogR;
		g_LastFogG = pObject->m_FogG;
		g_LastFogB = pObject->m_FogB;
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR,
			RGB_MAKE(g_FogColor.r, g_FogColor.g, g_FogColor.b));
	}

	return LTTRUE;
}
