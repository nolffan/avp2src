// *********************************************************************** //
//
//  ph/d3d_viewparams.cpp -- Primal Hunt's d3d.ren, TU09:
//  0x1000ff00..0x10010138, /O2.
//
//  The near plane console variables, the view's vertical fog state, and
//  the dynamic lights' contribution to a model's light.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// A console variable (common_stuff.cpp's registrar, 0x10011f36).
struct CConVar
{
	CConVar(const char* pszName, float fDefault, int32* pnValue,
	        float* pfValue);                                // 0x10011f36

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pszName;                                  // +0x14
	void*       m_hVar;                                     // +0x18
	CConVar*    m_pNext;                                    // +0x1c
};

CConVar     g_CV_NearZ("NearZ", 7.0f, LTNULL, LTNULL);                       // 0x1005b748
CConVar     g_CV_ReallyCloseNearZ("ReallyCloseNearZ", 0.01f, LTNULL, LTNULL);  // 0x1005a328


// The camera position, and the vertical fog console variables.
extern LTVector g_vCameraPos;                               // 0x1005bc44
extern float    g_CV_VFogMinY;                              // 0x1005fde4
extern float    g_CV_VFogMaxY;                              // 0x1005fdac
extern float    g_CV_VFogMinYVal;                           // 0x1005e1d4
extern float    g_CV_VFogMaxYVal;                           // 0x1005e07c
extern float    g_fVFogRangeScale;                          // 0x1005e210
extern float    g_fVFogValScale;                            // 0x1005df90

struct ViewParams
{
	void    SetCameraPos(LTVector vPos);                    // 0x1000ff40

	uint8   m_pad_000[0x4e4];
	LTVector m_vCameraPos;                                  // +0x4e4
	float   m_fVFog;                                        // +0x4f0
	uint32  m_nVFogMode;                                    // +0x4f4
};

// 0x1000ff40 -- the camera position, and how far into the vertical fog
// it is: 0 below it, 1 above it, 2 inside it.
void ViewParams::SetCameraPos(LTVector vPos)
{
	float fVFog;
	uint32 nVFogMode;

	m_vCameraPos = vPos;

	if (g_vCameraPos.y <= g_CV_VFogMinY)
		fVFog = g_CV_VFogMinYVal;
	else if (g_vCameraPos.y >= g_CV_VFogMaxY)
		fVFog = g_CV_VFogMaxYVal;
	else
		fVFog = (g_vCameraPos.y - g_CV_VFogMinY) * g_fVFogRangeScale
			* g_fVFogValScale + g_CV_VFogMinYVal;

	m_fVFog = fVFog;

	if (g_vCameraPos.y >= g_CV_VFogMaxY)
		nVFogMode = 1;
	else if (g_vCameraPos.y <= g_CV_VFogMinY)
		nVFogMode = 0;
	else
		nVFogMode = 2;

	m_nVFogMode = nVFogMode;
}


// What this file reads of a dynamic light and of a model instance.
struct DynamicLight
{
	uint8   m_pad_000[0x94];
	uint8   m_ColorR;                                       // +0x94
	uint8   m_ColorG;                                       // +0x95
	uint8   m_ColorB;                                       // +0x96
	uint8   m_pad_097[0x194 - 0x97];
	LTVector m_Pos;                                         // +0x194
	uint8   m_pad_1a0[0x1b0 - 0x1a0];
	float   m_LightRadius;                                  // +0x1b0
};

struct ModelInstance
{
	uint8   m_pad_000[0x194];
	LTVector m_Pos;                                         // +0x194
};

extern int32         g_bDynamicLightModels;                 // 0x1004a770
extern DynamicLight* g_DynamicLights[];                     // 0x1005c168
extern uint32        g_nDynamicLights;                      // 0x1005bcb0

// 0x10010010 -- NEAR (the y and z squares are added in the other order).
// The dynamic lights' colour at a model: each light in
// range adds its colour (0..255 mapped to -1..1) by how close it is.
void d3d_GetDynamicLightAdd(ModelInstance* pInstance, LTVector* pAdd)
{
	uint32 i;
	DynamicLight* pLight;
	LTVector vDelta;
	float fDistSqr, fScale;

	pAdd->Init();
	if (!g_bDynamicLightModels)
		return;

	for (i = 0; i < g_nDynamicLights; i++)
	{
		pLight = g_DynamicLights[i];

		vDelta = pLight->m_Pos - pInstance->m_Pos;
		fDistSqr = (vDelta.x * vDelta.x + vDelta.y * vDelta.y) + vDelta.z * vDelta.z;
		if (fDistSqr < pLight->m_LightRadius * pLight->m_LightRadius)
		{
			fScale = (1.0f - (float)sqrt(fDistSqr) / pLight->m_LightRadius) * 0.7f;
			pAdd->x += (float)((int)pLight->m_ColorR * 2 - 255) * fScale;
			pAdd->y += (float)((int)pLight->m_ColorG * 2 - 255) * fScale;
			pAdd->z += (float)((int)pLight->m_ColorB * 2 - 255) * fScale;
		}
	}
}
