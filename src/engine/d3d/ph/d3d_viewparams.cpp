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

CD3DConVar     g_CV_NearZ("NearZ", 7.0f, LTNULL, LTNULL);                       // 0x1005b748
CD3DConVar     g_CV_ReallyCloseNearZ("ReallyCloseNearZ", 0.01f, LTNULL, LTNULL);  // 0x1005a328

uint32         g_nLightmapUploadSaves;                          // 0x1005b774 (d3d_texture.cpp counts them)


// The vertical fog console variables (d3d_draw.cpp's).
extern CD3DConVar g_CV_VFogMinY;                            // 0x1005fde0
extern CD3DConVar g_CV_VFogMaxY;                            // 0x1005fda8
extern CD3DConVar g_CV_VFogMinYVal;                         // 0x1005e1d0
extern CD3DConVar g_CV_VFogMaxYVal;                         // 0x1005e078
extern float    g_fVFogRangeScale;                          // 0x1005df90 (d3d_draw.cpp)
extern float    g_fVFogValScale;                            // 0x1005e210 (d3d_draw.cpp)

struct ViewParams
{
	void    SetCameraPos(LTVector vPos);                    // 0x1000ff40

	uint8   m_pad_000[0x4b4];
	LTVector m_Pos;                                         // +0x4b4
	uint8   m_pad_4c0[0x4e4 - 0x4c0];
	LTVector m_vCameraPos;                                  // +0x4e4
	float   m_fVFog;                                        // +0x4f0
	uint32  m_nVFogMode;                                    // +0x4f4
};

extern ViewParams g_ViewParams;                             // 0x1005b790 (common_draw.cpp)

// 0x1000ff40 -- the camera position, and how far into the vertical fog
// it is: 0 below it, 1 above it, 2 inside it.
void ViewParams::SetCameraPos(LTVector vPos)
{
	float fVFog;
	uint32 nVFogMode;

	m_vCameraPos = vPos;

	if (g_ViewParams.m_Pos.y <= g_CV_VFogMinY.m_fValue)
		fVFog = g_CV_VFogMinYVal.m_fValue;
	else if (g_ViewParams.m_Pos.y >= g_CV_VFogMaxY.m_fValue)
		fVFog = g_CV_VFogMaxYVal.m_fValue;
	else
		fVFog = (g_ViewParams.m_Pos.y - g_CV_VFogMinY.m_fValue) * g_fVFogRangeScale
			* g_fVFogValScale + g_CV_VFogMinYVal.m_fValue;

	m_fVFog = fVFog;

	if (g_ViewParams.m_Pos.y >= g_CV_VFogMaxY.m_fValue)
		nVFogMode = 1;
	else if (g_ViewParams.m_Pos.y <= g_CV_VFogMinY.m_fValue)
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

extern int32         g_CV_LightModels;                      // 0x1004a770 (common_stuff.cpp: "LightModels")
class LTObject;
extern LTObject*     g_DynamicLights[];                     // 0x1005c168
extern uint32        g_nDynamicLights;                      // 0x1005bcb0

// 0x10010010 (AvP2 0x1000f270) -- the dynamic lights' colour at a model: each light in
// range adds its colour (0..255 mapped to -1..1) by how close it is.
void d3d_GetDynamicLightAdd(ModelInstance* pInstance, LTVector* pAdd)
{
	uint32 i;
	DynamicLight* pLight;
	float fDistSqr, fScale;

	pAdd->Init();
	if (!g_CV_LightModels)
		return;

	for (i = 0; i < g_nDynamicLights; i++)
	{
		pLight = (DynamicLight*)g_DynamicLights[i];

		fDistSqr = pLight->m_Pos.DistSqr(pInstance->m_Pos);
		if (fDistSqr < pLight->m_LightRadius * pLight->m_LightRadius)
		{
			fScale = (1.0f - (float)sqrt(fDistSqr) / pLight->m_LightRadius) * 0.7f;
			pAdd->x += (float)((int)pLight->m_ColorR * 2 - 255) * fScale;
			pAdd->y += (float)((int)pLight->m_ColorG * 2 - 255) * fScale;
			pAdd->z += (float)((int)pLight->m_ColorB * 2 - 255) * fScale;
		}
	}
}
