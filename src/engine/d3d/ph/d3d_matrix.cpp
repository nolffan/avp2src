// *********************************************************************** //
//
//  MODULE:   D3D_MATRIX (.CPP) -- Primal Hunt's d3d.ren, TU05
//            (0x10008c80..0x10009877, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The particle systems' draw (0x10008c90, the batch drawer
//            0x10009320 and the texture lookup between), and the object
//            transform set-up the other drawing modules call.  The name
//            is the map's guess: the object is mostly particles.
//
//            Written so far: the transform and the texture lookup.  Not
//            written: 0x10008c90 (832 bytes) and 0x10009320 (1337 bytes),
//            which would also bring in this object's out-of-line MatMul
//            (0x10008fd0) and _CVector::Init (0x10009860) COMDATs.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// The particle colour scale (0x10053ff0): its initialiser is the empty
// constructor's, 0x10008c80.
LTVector    g_vParticleColorScale;                          // 0x10053ff0


// The engine's rotation, as the transform reads it.
struct Rotation
{
	float   x;
	float   y;
	float   z;
	float   w;
};

void d3d_RotationToMatrix(Rotation* pRot, LTMatrix* pMat);  // 0x1003655d


// 0x100091f0 -- position, rotation and scale to a transform: the rotation
// is sanitised first (a component out of range is reset).
void d3d_SetupTransformation(LTVector* pPos, Rotation* pRot, LTVector* pScale, LTMatrix* pMat)
{
	if (pRot->x > 100000.0f || pRot->x < -100000.0f)
		pRot->x = 0.0f;

	if (pRot->y > 100000.0f || pRot->y < -100000.0f)
		pRot->y = 0.0f;

	if (pRot->z > 100000.0f || pRot->z < -100000.0f)
		pRot->z = 0.0f;

	if (pRot->w > 100000.0f || pRot->w < -100000.0f)
		pRot->w = 1.0f;

	d3d_RotationToMatrix(pRot, pMat);

	pMat->m[0][0] *= pScale->x;
	pMat->m[1][0] *= pScale->x;
	pMat->m[2][0] *= pScale->x;

	pMat->m[0][1] *= pScale->y;
	pMat->m[1][1] *= pScale->y;
	pMat->m[2][1] *= pScale->y;

	pMat->m[0][2] *= pScale->z;
	pMat->m[1][2] *= pScale->z;
	pMat->m[2][2] *= pScale->z;

	pMat->m[0][3] = pPos->x;
	pMat->m[1][3] = pPos->y;
	pMat->m[2][3] = pPos->z;
}


// A texture's per-stage device copies, chained.
struct RTexture
{
	uint8       m_Reserved00[0x30];
	RTexture*   m_pNext;                                    // +0x30
	uint8       m_Reserved34[0x42 - 0x34];
	uint16      m_iStage;                                   // +0x42
};

// 0x10009300 -- a texture's copy for a stage.
RTexture* d3d_GetStageTexture(RTexture* pTexture, uint8 iStage)
{
	while (pTexture)
	{
		if (pTexture->m_iStage == (uint16)iStage)
			return pTexture;

		pTexture = pTexture->m_pNext;
	}

	return LTNULL;
}
