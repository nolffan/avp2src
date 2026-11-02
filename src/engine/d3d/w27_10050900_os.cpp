// ======================================================================= //
//  w27_10050900_os.cpp -- d3d.ren: the deferred model draw.
//
//  0x10050900  the draw callback 0x10050bf0 queues: cull the model
//              against the frustum, then draw it through 0x1000e0e0 with
//              its blend and fog render states set for the duration.
//  0x10050bf0  queue a model on the deferred-draw queue with it (the
//              shape of 0x1005dae0 / 0x100646a0, w27_1005dae0.cpp).
// ======================================================================= //
#include "ltbasedefs.h"

// IDirect3DDevice7 as a C COM interface (w7's tag for 0x1008d988).
struct D3DDevTSSR5C;
struct W7DevVtbl56c60
{
	void*	m_Slots00[20];                                              // +0x00
	long (__stdcall* SetRenderState)(D3DDevTSSR5C*, uint32 nState,
	                                 uint32 nValue);                    // +0x50
	long (__stdcall* GetRenderState)(D3DDevTSSR5C*, uint32 nState,
	                                 uint32* pValue);                   // +0x54
};
struct D3DDevTSSR5C { W7DevVtbl56c60* lpVtbl; };

extern D3DDevTSSR5C* g_1008d988;                        // 0x1008d988

// The frustum planes (big17c14.cpp's tag): the view's and the portal's.
struct Plane17c14
{
	LTVector	m_vNormal;                              // +0x00
	float		m_fDist;                                // +0x0c
};

extern Plane17c14	g_1008bc50[6];                      // 0x1008bc50  frustum
extern Plane17c14	g_1008bcb0[6];                      // 0x1008bcb0  portal frustum

// The model object (bige0e0.cpp's tag).
struct Obje0e0
{
	uint8		m_pad_000[0x88];
	uint32		m_dwFlags88;                            // +0x088
	uint32		m_dwFlags8c;                            // +0x08c
	uint8		m_pad_090[0xac - 0x90];
	LTVector	m_vScale;                               // +0x0ac
	uint8		m_pad_0b8[0x208 - 0xb8];
	LTVector	m_vPos;                                 // +0x208
	float		m_fRadius;                              // +0x214
};

// 0x1000e0e0 -- draw the model (bige0e0.cpp).  ONE argument: the `push
// %edi` in front of `push %ebx ; call` is icl's dead slot for an odd
// argument count, as in front of the atexit call of 0x1001bfa0 -- bige0e0
// declares a second, never-read parameter (its recorded coin-flip), so
// this spelling is aliased onto its definition in link/aliases.rsp.
// Measured: passing a second argument moves the blend values into the
// other two registers (14 bytes).
extern void sub_1000e0e0(Obje0e0* pObj);                               // 0x1000e0e0

#define LTMAX50900(a, b)	((a) > (b) ? (a) : (b))

// A render state set for the lifetime of the object: the old value read
// back on construction, restored on destruction.
struct RenderState50900
{
	RenderState50900(uint32 nState, uint32 nValue)
	{
		m_nState = nState;
		g_1008d988->lpVtbl->GetRenderState(g_1008d988, nState, &m_nOld);
		g_1008d988->lpVtbl->SetRenderState(g_1008d988, m_nState, nValue);
	}

	~RenderState50900()
	{
		g_1008d988->lpVtbl->SetRenderState(g_1008d988, m_nState, m_nOld);
	}

	uint32	m_nState;
	uint32	m_nOld;
};


// A plane's signed distance to a point.  Through an inline helper, as
// measured: written in the loop, icl keeps the three position components
// in x87 registers across it (757 bytes); the shipped loop re-reads them
// from the object for every plane, as it does through the helper.
inline float Dist50900(const Plane17c14* p, const LTVector* v)
{
	return p->m_vNormal.x * v->x + p->m_vNormal.y * v->y + p->m_vNormal.z * v->z - p->m_fDist;
}
// ======================================================================= //
//  0x10050900 -- 739 bytes.  IDENTICAL.  A cdecl (pCtx, pObj) with the
//  context unused, the queue's callback shape.  The largest scale
//  component is LTMAX(x, LTMAX(y, z)) as a macro: the shipped code
//  evaluates the inner maximum twice.  The four render states are saved
//  and set in order and restored in reverse at the closing brace -- a
//  small object's constructor and destructor (state and old value kept
//  together at -0x24/-0x20 .. -0x0c/-0x08).
// ======================================================================= //
void sub_10050900(void* pCtx, Obje0e0* pObj)
{
	float		fRadius;
	Plane17c14*	pPlane;
	int32		i;
	uint32		nFogColor;
	uint32		nFogEnable;
	uint32		nSrcBlend;
	uint32		nDestBlend;

	fRadius = LTMAX50900(pObj->m_vScale.x,
	                     LTMAX50900(pObj->m_vScale.y, pObj->m_vScale.z)) *
	          pObj->m_fRadius;

	// The flags are read through a volatile lvalue here (the
	// one-site `*(volatile T*)&x`): the shipped code loads them afresh for
	// the `|=` after the loop, where a plain read is CSE'd with this one
	// and kept in a stack slot (66 bytes).  Measured alternatives that
	// did not reload: the pick and the `|=` each through an inline helper,
	// the pick inside the loop, the test in an inline InFrustum().
	pPlane = (*(volatile uint32*)&pObj->m_dwFlags88 & 0x40) ? g_1008bcb0 : g_1008bc50;

	for (i = 0; i < 6; i++, pPlane++)
	{
		if (Dist50900(pPlane, &pObj->m_vPos) < -fRadius)
			return;
	}

	pObj->m_dwFlags88 |= 0x40000000;

	g_1008d988->lpVtbl->GetRenderState(g_1008d988, 0x22, &nFogColor);
	g_1008d988->lpVtbl->GetRenderState(g_1008d988, 0x1c, &nFogEnable);

	if (pObj->m_dwFlags88 & 0x80)
		nFogEnable = 0;

	if (pObj->m_dwFlags8c & 4)
	{
		nSrcBlend  = 2;
		nDestBlend = 2;
		nFogColor  = 0;
	}
	else if (pObj->m_dwFlags8c & 8)
	{
		nSrcBlend  = 1;
		nDestBlend = 3;
		nFogColor  = 0xffffffff;
	}
	else
	{
		nSrcBlend  = 5;
		nDestBlend = 6;
	}

	{
		RenderState50900	src(0x13, nSrcBlend);
		RenderState50900	dest(0x14, nDestBlend);
		RenderState50900	fog(0x1c, nFogEnable);
		RenderState50900	color(0x22, nFogColor);

		sub_1000e0e0(pObj);
	}
}


class C100500a0
{
public:
	void	sub_100500a0(void* pItem, void (*pfnDraw)(void* pCtx, void* pItem));   // 0x100500a0
};

extern C100500a0*	g_p1008ef3c;                        // 0x1008ef3c  the queue


// ======================================================================= //
//  0x10050bf0 -- 38 bytes.  Queue with 0x10050900.
// ======================================================================= //
void sub_10050bf0(void* pCtx, void* pItem)
{
	g_p1008ef3c->sub_100500a0(pItem, (void (*)(void*, void*))sub_10050900);
}
