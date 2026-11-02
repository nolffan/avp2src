// *********************************************************************** //
//
//  w14_10006840.cpp -- CD3DModelDraw's five vertex-setup routines
//  0x10006840, 0x10007620, 0x10008340, 0x10008680 and 0x100086f0
//  (written for the d3d.ren link).
//
//  0x10008da0 (w2_10008da0.cpp) picks one of the five and hands its
//  address to 0x10008a40, which calls it once per visible piece through
//  `mov 0x8(%ebp),%eax ; call *%eax` with the drawer in %ecx and two
//  stack arguments (the piece's vertex run, the piece's transformed
//  vertices), `ret $0x8`: __thiscall members of the drawer.  Each fills
//  the current vertex-buffer bank (+0x448) with the run's triangles and
//  draws it (bank slot +0x04), splitting whenever the bank is full.
//
//      0x10008340  pre-transformed (XYZRHW) triangles, backface-culled
//                  when +0x6b4 is set.
//      0x100086f0  untransformed (XYZ) triangles: the bank's vertex is
//                  four bytes shorter (no rhw), so the uv generator is
//                  handed the vertex four bytes early and the write
//                  pointer steps by stride - 4.
//      0x10008680  the run is already in the bank (the cached model
//                  path, mode 2): only the draw.
//      0x10006840  clip-space triangles, classified against the enabled
//                  planes of g_1008b874, clipped through +0x440 and then
//                  projected by the matrix at 0x1008ba1c, with z pushed
//                  back by 7.0 (the object-flag 0x40 path).
//      0x10007620  as 0x10006840 without the z push.
//
//  /O2: every body is padded to 16 with multi-byte lea-nops.
//
//  0x10008680 is byte-identical.  The other four are NEAR, measured:
//    * same calls in the same order (27/27/13/13), and for 0x10006840 /
//      0x10007620 the same x87 census (fstps 18, fmul 80/76, fadd 64,
//      fsub 10, 14/10 divides, 8 fchs); the corner pointers are a local
//      ARRAY (the shipped code re-reads them from -0x564.. / -0x34.. after
//      every store), the position copy is an LTVector assignment and the
//      projection reads every term before storing (both shipped shapes);
//    * the residue is block layout and allocation: the shipped build puts
//      the backface test after the emit path (reached by `jne`) and the
//      bank-full flush past the `ret`; ours inlines both.  Source forms
//      (continue, goto, nested ifs, bool flag, index loop, while) all
//      compile to one CFG here; a Warn/Err/...-named inline flush moves
//      the flush out (icl's cold-callee rule) but no spelling found puts
//      the backface test where the shipped build has it, so none is kept.
//
// *********************************************************************** //

#include "ltbasedefs.h"


struct D3DDevice;

// 0x1008d988 -- the Direct3D device.
extern D3DDevice*   g_pD3DDevice;                           // 0x1008d988
// 0x1008be50 -- triangles drawn this frame (big2d1c0.cpp's name).
extern int32        g_nModelTriangles;                      // 0x1008be50
// 0x1008bd4c -- mirrored view: flips the backface test (big26728.cpp).
extern int32        g_1008bd4c;                             // 0x1008bd4c
// 0x1008ab94 -- the backface threshold (0.0f), read only.
extern float        g_f1008ab94;                            // 0x1008ab94
// 0x1008b874 -- the enabled clip planes (big17c14.cpp's spelling).
extern unsigned long g_1008b874;                            // 0x1008b874
// 0x1008b908 / 0x1008b910 -- the near and far planes.
extern float        g_1008b908;                             // near plane
extern float        g_1008b910;                             // far plane

// 0x1008ba1c -- the projection matrix (big17c14.cpp's LTMat).
struct LTMat
{
	float       m[4][4];
};
extern LTMat        g_Mat1008ba1c;                          // 0x1008ba1c


// A model vertex-buffer bank (w2_10009ae0.cpp's I100039d0), with the
// slots and fields these routines use.
struct I100039d0
{
	virtual void    Slot00();                               // +0x00
	virtual void    Draw(D3DDevice* pDevice, uint32 nPrimType,
	                     uint32 nVerts);                    // +0x04
	virtual void    Slot08();                               // +0x08
	virtual void    Slot0c();                               // +0x0c
	virtual void    Slot10();                               // +0x10
	virtual uint8*  GetBase();                              // +0x14
	virtual uint32  GetStride();                            // +0x18

	// 0x1007dec0 -- the /Qip stack entry of d3dr2d_os.cpp's
	// Obj1007dec4::sub_1007dec4 (0x1007dec4): step to the next buffer.
	void            sub_1007dec0();                         // 0x1007dec0

	uint8           m_pad_04[0x1c - 0x04];                  // +0x04
	uint32          m_nUsed1c;                              // +0x1c
	uint32          m_nSize20;                              // +0x20

	// The last vertex slot the bank has room for.
	uint8* GetLast()
	{
		uint8* pBase = GetBase();
		uint32 nLeft = m_nSize20 - m_nUsed1c - 1;
		return pBase + nLeft * GetStride();
	}

	// Vertices written between the bank's base and pEnd.
	uint32 GetCount(uint8* pEnd)
	{
		return (pEnd - GetBase()) / GetStride();
	}
};


// The drawer's transformed model vertex (0x20 bytes, big8fd0.cpp's RVert).
struct RVert
{
	LTVector    m_vPos;                                     // +0x00
	float       m_fRHW;                                     // +0x0c
	uint32      m_nColor10;                                 // +0x10
	uint8       m_pad_14[0x20 - 0x14];                      // +0x14
};

// One model triangle (0x20 bytes): three vertex indices and a uv pair
// per corner.
struct ModelTri
{
	uint16      m_Indices[3];                               // +0x00
	uint16      m_pad_06;                                   // +0x06
	float       m_UV[3][2];                                 // +0x08
};

// One vertex run of a piece (big8fd0.cpp's ModelVertSet).
struct ModelVertSet
{
	uint32      m_n00;                                      // +0x00
	void*       m_pVerts04;                                 // +0x04
	uint32      m_nVerts08;                                 // +0x08
	uint8       m_pad_0c[0x14 - 0x0c];                      // +0x0c
	ModelTri*   m_pTris14;                                  // +0x14
	uint32      m_nTris18;                                  // +0x18
	uint8       m_pad_1c[0x24 - 0x1c];                      // +0x1c
};

// A bank vertex, pre-transformed (FVF 0x1c4 / 0x2c4): the first 0x18
// bytes; the uv generator writes the rest.
struct TLVert
{
	LTVector    m_vPos;                                     // +0x00
	float       m_fRHW;                                     // +0x0c
	uint32      m_nDiffuse;                                 // +0x10
	uint32      m_nSpecular;                                // +0x14
};

// A bank vertex, untransformed (the rhw dropped).
struct LVert
{
	LTVector    m_vPos;                                     // +0x00
	uint32      m_nDiffuse;                                 // +0x0c
	uint32      m_nSpecular;                                // +0x10
};

// The uv generator at +0x434 (w2_10003a10.cpp's family): (bank vertex,
// model vertex, the corner's uv pair).
typedef void (__fastcall* PFNUV)(void* pOut, RVert* pSrc, float* pUV);
// The bank-vertex copy at +0x43c (0x10003bb0 / 0x10003bf0).
typedef void (__fastcall* PFNCOPY)(void* pDst, void* pSrc);
// The polygon clipper at +0x440 (0x10003c40 / 0x100051a0): (planes,
// &vertices, &count), LTRUE while anything is left.
typedef LTBOOL (__fastcall* PFNCLIP)(uint32 nClip, uint8** ppVerts,
                                     int32* pnVerts);


// The screen-space facing of a triangle, sign-flipped for a mirrored view.
inline float TriFacing(RVert* pV0, RVert* pV1, RVert* pV2)
{
	float fCross = (pV1->m_vPos.x - pV0->m_vPos.x) * (pV2->m_vPos.y - pV0->m_vPos.y)
	             - (pV2->m_vPos.x - pV0->m_vPos.x) * (pV1->m_vPos.y - pV0->m_vPos.y);
	if (g_1008bd4c)
		fCross = -fCross;
	return fCross;
}


// Classify a triangle against the planes enabled in nFlags: 0 all inside,
// 1 all outside one plane, 2 straddling -- *pnClip then holds that plane
// and the ones still to test.
inline uint8 ClassifyTri(RVert* pV0, RVert* pV1, RVert* pV2, uint32 nFlags,
                         uint32* pnClip)
{
	uint32 n;

	if (nFlags & 0x01)
	{
		n = (pV0->m_vPos.z >= g_1008b908)
		  + (pV1->m_vPos.z >= g_1008b908)
		  + (pV2->m_vPos.z >= g_1008b908);
		if (n == 0)
			return 1;
		if (n != 3)
		{
			*pnClip = nFlags & 0x3f;
			return 2;
		}
	}
	if (nFlags & 0x04)
	{
		n = (-pV0->m_vPos.z < pV0->m_vPos.x)
		  + (-pV1->m_vPos.z < pV1->m_vPos.x)
		  + (-pV2->m_vPos.z < pV2->m_vPos.x);
		if (n == 0)
			return 1;
		if (n != 3)
		{
			*pnClip = nFlags & 0x3e;
			return 2;
		}
	}
	if (nFlags & 0x08)
	{
		n = (pV0->m_vPos.y < pV0->m_vPos.z)
		  + (pV1->m_vPos.y < pV1->m_vPos.z)
		  + (pV2->m_vPos.y < pV2->m_vPos.z);
		if (n == 0)
			return 1;
		if (n != 3)
		{
			*pnClip = nFlags & 0x3a;
			return 2;
		}
	}
	if (nFlags & 0x10)
	{
		n = (pV0->m_vPos.x < pV0->m_vPos.z)
		  + (pV1->m_vPos.x < pV1->m_vPos.z)
		  + (pV2->m_vPos.x < pV2->m_vPos.z);
		if (n == 0)
			return 1;
		if (n != 3)
		{
			*pnClip = nFlags & 0x32;
			return 2;
		}
	}
	if (nFlags & 0x20)
	{
		n = (-pV0->m_vPos.z < pV0->m_vPos.y)
		  + (-pV1->m_vPos.z < pV1->m_vPos.y)
		  + (-pV2->m_vPos.z < pV2->m_vPos.y);
		if (n == 0)
			return 1;
		if (n != 3)
		{
			*pnClip = nFlags & 0x22;
			return 2;
		}
	}
	if (nFlags & 0x02)
	{
		n = (pV0->m_vPos.z <= g_1008b910)
		  + (pV1->m_vPos.z <= g_1008b910)
		  + (pV2->m_vPos.z <= g_1008b910);
		if (n == 0)
			return 1;
		if (n != 3)
		{
			*pnClip = nFlags & 0x02;
			return 2;
		}
	}
	return 0;
}

// The facing of a triangle after the divide by z, sign-flipped for a
// mirrored view.
inline float ProjFacing(RVert* pV0, RVert* pV1, RVert* pV2)
{
	float fRHW[3];
	fRHW[0] = 1.0f / pV0->m_vPos.z;
	fRHW[1] = 1.0f / pV1->m_vPos.z;
	fRHW[2] = 1.0f / pV2->m_vPos.z;

	float fCross = (pV2->m_vPos.x * fRHW[2] - pV0->m_vPos.x * fRHW[0])
	             * (pV1->m_vPos.y * fRHW[1] - pV0->m_vPos.y * fRHW[0])
	             - (pV1->m_vPos.x * fRHW[1] - pV0->m_vPos.x * fRHW[0])
	             * (pV2->m_vPos.y * fRHW[2] - pV0->m_vPos.y * fRHW[0]);
	if (g_1008bd4c)
		fCross = -fCross;
	return fCross;
}

// Project a view-space vertex into a bank vertex (pOut may be pIn): every
// term is read before anything is stored.
inline void ProjectVert(RVert* pOut, RVert* pIn)
{
	float fRHW = 1.0f / (g_Mat1008ba1c.m[3][0] * pIn->m_vPos.x
	                   + g_Mat1008ba1c.m[3][1] * pIn->m_vPos.y
	                   + g_Mat1008ba1c.m[3][2] * pIn->m_vPos.z
	                   + g_Mat1008ba1c.m[3][3]);
	float fZ = (g_Mat1008ba1c.m[2][0] * pIn->m_vPos.x
	          + g_Mat1008ba1c.m[2][1] * pIn->m_vPos.y
	          + g_Mat1008ba1c.m[2][2] * pIn->m_vPos.z
	          + g_Mat1008ba1c.m[2][3]) * fRHW;
	float fX = (g_Mat1008ba1c.m[0][0] * pIn->m_vPos.x
	          + g_Mat1008ba1c.m[0][1] * pIn->m_vPos.y
	          + g_Mat1008ba1c.m[0][2] * pIn->m_vPos.z
	          + g_Mat1008ba1c.m[0][3]) * fRHW;
	float fY = (g_Mat1008ba1c.m[1][0] * pIn->m_vPos.x
	          + g_Mat1008ba1c.m[1][1] * pIn->m_vPos.y
	          + g_Mat1008ba1c.m[1][2] * pIn->m_vPos.z
	          + g_Mat1008ba1c.m[1][3]) * fRHW;
	pOut->m_vPos.x = fX;
	pOut->m_vPos.y = fY;
	pOut->m_vPos.z = fZ;
	pOut->m_fRHW = fRHW;
}

// As ProjectVert, with the depth (z and rhw) taken 7 units further away
// than the position x and y are projected from.
inline void ProjectVertPushed(RVert* pOut, RVert* pIn)
{
	float fPushed = 7.0f + pIn->m_vPos.z;
	float fRHW = 1.0f / (g_Mat1008ba1c.m[3][0] * pIn->m_vPos.x
	                   + g_Mat1008ba1c.m[3][1] * pIn->m_vPos.y
	                   + g_Mat1008ba1c.m[3][2] * fPushed
	                   + g_Mat1008ba1c.m[3][3]);
	float fZ = (g_Mat1008ba1c.m[2][0] * pIn->m_vPos.x
	          + g_Mat1008ba1c.m[2][1] * pIn->m_vPos.y
	          + g_Mat1008ba1c.m[2][2] * fPushed
	          + g_Mat1008ba1c.m[2][3]) * fRHW;
	float fW = 1.0f / (g_Mat1008ba1c.m[3][0] * pIn->m_vPos.x
	                 + g_Mat1008ba1c.m[3][1] * pIn->m_vPos.y
	                 + g_Mat1008ba1c.m[3][2] * pIn->m_vPos.z
	                 + g_Mat1008ba1c.m[3][3]);
	float fX = (g_Mat1008ba1c.m[0][0] * pIn->m_vPos.x
	          + g_Mat1008ba1c.m[0][1] * pIn->m_vPos.y
	          + g_Mat1008ba1c.m[0][2] * pIn->m_vPos.z
	          + g_Mat1008ba1c.m[0][3]) * fW;
	float fY = (g_Mat1008ba1c.m[1][0] * pIn->m_vPos.x
	          + g_Mat1008ba1c.m[1][1] * pIn->m_vPos.y
	          + g_Mat1008ba1c.m[1][2] * pIn->m_vPos.z
	          + g_Mat1008ba1c.m[1][3]) * fW;
	pOut->m_vPos.x = fX;
	pOut->m_vPos.y = fY;
	pOut->m_vPos.z = fZ;
	pOut->m_fRHW = fRHW;
}


class CD3DModelDraw
{
public:
	LTBOOL      sub_10006840(ModelVertSet* pSet, RVert* pVerts);  // 0x10006840
	LTBOOL      sub_10007620(ModelVertSet* pSet, RVert* pVerts);  // 0x10007620
	LTBOOL      sub_10008340(ModelVertSet* pSet, RVert* pVerts);  // 0x10008340
	LTBOOL      sub_10008680(ModelVertSet* pSet, RVert* pVerts);  // 0x10008680
	LTBOOL      sub_100086f0(ModelVertSet* pSet, RVert* pVerts);  // 0x100086f0

	uint8       m_pad_000[0x434];                           // +0x000
	PFNUV       m_pfnUV434;                                 // +0x434
	uint32      m_nStride438;                               // +0x438
	PFNCOPY     m_pfnCopy43c;                               // +0x43c
	PFNCLIP     m_pfnClip440;                               // +0x440
	uint32      m_nFVF444;                                  // +0x444
	I100039d0*  m_pBank448;                                 // +0x448
	uint8       m_pad_44c[0x480 - 0x44c];                   // +0x44c
	uint32      m_nSpecular480;                             // +0x480
	uint8       m_pad_484[0x6b4 - 0x484];                   // +0x484
	uint32      m_bCull6b4;                                 // +0x6b4
};


// ======================================================================= //
//  0x10006840 -- clip-space triangles, depth pushed back 7 units.
// ======================================================================= //
LTBOOL CD3DModelDraw::sub_10006840(ModelVertSet* pSet, RVert* pVerts)
{
	uint8* pOut = m_pBank448->GetBase();
	uint8* pLast = m_pBank448->GetLast();

	int32 nTris = pSet->m_nTris18;
	ModelTri* pTri = pSet->m_pTris14;
	for (; nTris; nTris--, pTri++)
	{
		RVert* pV[3];
		pV[0] = &pVerts[pTri->m_Indices[0]];
		pV[1] = &pVerts[pTri->m_Indices[1]];
		pV[2] = &pVerts[pTri->m_Indices[2]];

		uint32 nClip;
		uint8 nClass = ClassifyTri(pV[0], pV[1], pV[2], g_1008b874, &nClip);
		if (nClass == 0)
		{
			// Wholly inside: project the three corners straight into
			// the bank.
			if (ProjFacing(pV[0], pV[1], pV[2]) > 0.0f)
			{
				TLVert* pT;
				ProjectVertPushed((RVert*)pOut, pV[0]);
				pT = (TLVert*)pOut;
				pT->m_nDiffuse = pV[0]->m_nColor10;
				pT->m_nSpecular = m_nSpecular480;
				m_pfnUV434(pOut, pV[0], pTri->m_UV[0]);
				pOut += m_nStride438;
				ProjectVertPushed((RVert*)pOut, pV[1]);
				pT = (TLVert*)pOut;
				pT->m_nDiffuse = pV[1]->m_nColor10;
				pT->m_nSpecular = m_nSpecular480;
				m_pfnUV434(pOut, pV[1], pTri->m_UV[1]);
				pOut += m_nStride438;
				ProjectVertPushed((RVert*)pOut, pV[2]);
				pT = (TLVert*)pOut;
				pT->m_nDiffuse = pV[2]->m_nColor10;
				pT->m_nSpecular = m_nSpecular480;
				m_pfnUV434(pOut, pV[2], pTri->m_UV[2]);
				pOut += m_nStride438;

				if (pOut > pLast && nTris > 1)
				{
					g_nModelTriangles += (m_pBank448->m_nSize20 - m_pBank448->m_nUsed1c) / 3;
					m_pBank448->Draw(g_pD3DDevice, 4,
					                 m_pBank448->m_nSize20 - m_pBank448->m_nUsed1c);
					pOut = m_pBank448->GetBase();
					pLast = m_pBank448->GetLast();
				}
			}
		}
		else if (nClass == 2)
		{
			// Straddling: clip a copy, project what is left and fan it
			// into the bank.
			uint8 aClip[32 * 0x28];
			int32 nVerts = 3;
			uint8* pClip = aClip;
			uint8* p = aClip;
			TLVert* pT;
			pT = (TLVert*)p;
			pT->m_vPos = pV[0]->m_vPos;
			pT->m_nDiffuse = pV[0]->m_nColor10;
			pT->m_nSpecular = m_nSpecular480;
			m_pfnUV434(p, pV[0], pTri->m_UV[0]);
			p += m_nStride438;
			pT = (TLVert*)p;
			pT->m_vPos = pV[1]->m_vPos;
			pT->m_nDiffuse = pV[1]->m_nColor10;
			pT->m_nSpecular = m_nSpecular480;
			m_pfnUV434(p, pV[1], pTri->m_UV[1]);
			p += m_nStride438;
			pT = (TLVert*)p;
			pT->m_vPos = pV[2]->m_vPos;
			pT->m_nDiffuse = pV[2]->m_nColor10;
			pT->m_nSpecular = m_nSpecular480;
			m_pfnUV434(p, pV[2], pTri->m_UV[2]);

			if (m_pfnClip440(nClip, &pClip, &nVerts)
			 && ProjFacing((RVert*)pClip, (RVert*)(pClip + m_nStride438),
			               (RVert*)(pClip + m_nStride438 * 2)) > 0.0f)
			{
				uint8* pEnd = pClip + nVerts * m_nStride438;
				for (p = pClip; p < pEnd; p += m_nStride438)
				{
					RVert* pC = (RVert*)p;
					ProjectVertPushed(pC, pC);
				}

				if (pOut + (nVerts - 2) * 3 * m_nStride438 > pLast)
				{
					uint32 nDone = m_pBank448->GetCount(pOut);
					g_nModelTriangles += nDone / 3;
					m_pBank448->Draw(g_pD3DDevice, 4, nDone);
					m_pBank448->sub_1007dec0();
					pOut = m_pBank448->GetBase();
					pLast = m_pBank448->GetLast();
				}

				nVerts--;
				for (int32 i = 1; i < nVerts; )
				{
					m_pfnCopy43c(pOut, pClip);
					pOut += m_nStride438;
					m_pfnCopy43c(pOut, pClip + i * m_nStride438);
					pOut += m_nStride438;
					i++;
					m_pfnCopy43c(pOut, pClip + i * m_nStride438);
					pOut += m_nStride438;
				}
			}
		}
	}

	uint32 nDrawn = m_pBank448->GetCount(pOut);
	g_nModelTriangles += nDrawn / 3;
	m_pBank448->Draw(g_pD3DDevice, 4, nDrawn);
	return 1;
}


// ======================================================================= //
//  0x10007620 -- clip-space triangles.
// ======================================================================= //
LTBOOL CD3DModelDraw::sub_10007620(ModelVertSet* pSet, RVert* pVerts)
{
	uint8* pOut = m_pBank448->GetBase();
	uint8* pLast = m_pBank448->GetLast();

	int32 nTris = pSet->m_nTris18;
	ModelTri* pTri = pSet->m_pTris14;
	for (; nTris; nTris--, pTri++)
	{
		RVert* pV[3];
		pV[0] = &pVerts[pTri->m_Indices[0]];
		pV[1] = &pVerts[pTri->m_Indices[1]];
		pV[2] = &pVerts[pTri->m_Indices[2]];

		uint32 nClip;
		uint8 nClass = ClassifyTri(pV[0], pV[1], pV[2], g_1008b874, &nClip);
		if (nClass == 0)
		{
			// Wholly inside: project the three corners straight into
			// the bank.
			if (ProjFacing(pV[0], pV[1], pV[2]) > 0.0f)
			{
				TLVert* pT;
				ProjectVert((RVert*)pOut, pV[0]);
				pT = (TLVert*)pOut;
				pT->m_nDiffuse = pV[0]->m_nColor10;
				pT->m_nSpecular = m_nSpecular480;
				m_pfnUV434(pOut, pV[0], pTri->m_UV[0]);
				pOut += m_nStride438;
				ProjectVert((RVert*)pOut, pV[1]);
				pT = (TLVert*)pOut;
				pT->m_nDiffuse = pV[1]->m_nColor10;
				pT->m_nSpecular = m_nSpecular480;
				m_pfnUV434(pOut, pV[1], pTri->m_UV[1]);
				pOut += m_nStride438;
				ProjectVert((RVert*)pOut, pV[2]);
				pT = (TLVert*)pOut;
				pT->m_nDiffuse = pV[2]->m_nColor10;
				pT->m_nSpecular = m_nSpecular480;
				m_pfnUV434(pOut, pV[2], pTri->m_UV[2]);
				pOut += m_nStride438;

				if (pOut > pLast && nTris > 1)
				{
					g_nModelTriangles += (m_pBank448->m_nSize20 - m_pBank448->m_nUsed1c) / 3;
					m_pBank448->Draw(g_pD3DDevice, 4,
					                 m_pBank448->m_nSize20 - m_pBank448->m_nUsed1c);
					pOut = m_pBank448->GetBase();
					pLast = m_pBank448->GetLast();
				}
			}
		}
		else if (nClass == 2)
		{
			// Straddling: clip a copy, project what is left and fan it
			// into the bank.
			uint8 aClip[32 * 0x28];
			int32 nVerts = 3;
			uint8* pClip = aClip;
			uint8* p = aClip;
			TLVert* pT;
			pT = (TLVert*)p;
			pT->m_vPos = pV[0]->m_vPos;
			pT->m_nDiffuse = pV[0]->m_nColor10;
			pT->m_nSpecular = m_nSpecular480;
			m_pfnUV434(p, pV[0], pTri->m_UV[0]);
			p += m_nStride438;
			pT = (TLVert*)p;
			pT->m_vPos = pV[1]->m_vPos;
			pT->m_nDiffuse = pV[1]->m_nColor10;
			pT->m_nSpecular = m_nSpecular480;
			m_pfnUV434(p, pV[1], pTri->m_UV[1]);
			p += m_nStride438;
			pT = (TLVert*)p;
			pT->m_vPos = pV[2]->m_vPos;
			pT->m_nDiffuse = pV[2]->m_nColor10;
			pT->m_nSpecular = m_nSpecular480;
			m_pfnUV434(p, pV[2], pTri->m_UV[2]);

			if (m_pfnClip440(nClip, &pClip, &nVerts)
			 && ProjFacing((RVert*)pClip, (RVert*)(pClip + m_nStride438),
			               (RVert*)(pClip + m_nStride438 * 2)) > 0.0f)
			{
				uint8* pEnd = pClip + nVerts * m_nStride438;
				for (p = pClip; p < pEnd; p += m_nStride438)
				{
					RVert* pC = (RVert*)p;
					ProjectVert(pC, pC);
				}

				if (pOut + (nVerts - 2) * 3 * m_nStride438 > pLast)
				{
					uint32 nDone = m_pBank448->GetCount(pOut);
					g_nModelTriangles += nDone / 3;
					m_pBank448->Draw(g_pD3DDevice, 4, nDone);
					m_pBank448->sub_1007dec0();
					pOut = m_pBank448->GetBase();
					pLast = m_pBank448->GetLast();
				}

				nVerts--;
				for (int32 i = 1; i < nVerts; )
				{
					m_pfnCopy43c(pOut, pClip);
					pOut += m_nStride438;
					m_pfnCopy43c(pOut, pClip + i * m_nStride438);
					pOut += m_nStride438;
					i++;
					m_pfnCopy43c(pOut, pClip + i * m_nStride438);
					pOut += m_nStride438;
				}
			}
		}
	}

	uint32 nDrawn = m_pBank448->GetCount(pOut);
	g_nModelTriangles += nDrawn / 3;
	m_pBank448->Draw(g_pD3DDevice, 4, nDrawn);
	return 1;
}


// ======================================================================= //
//  0x10008340 -- pre-transformed triangles.
// ======================================================================= //
//  NEAR (measured): the residual is block layout.  Shipped,
//  m_bCull6b4 != 0 jumps to the facing test placed AFTER the body (just
//  before the loop step, `ja` back into the body), and the flush
//  (`nTris > 1`) is out of line at the very end.  `if (m_bCull6b4) { if
//  (facing <= eps) continue; }`, `&&`/`!` and goto spellings all compile
//  identically to the `||` form; the switch-default sink on m_bCull6b4
//  moves the facing test out of line but to the END (581 -> 402 bytes
//  off), and adding it on `nTris > 1` too puts the flush at the end as
//  shipped but the facing test after it (742).  0x100086f0 is the same
//  shape.
LTBOOL CD3DModelDraw::sub_10008340(ModelVertSet* pSet, RVert* pVerts)
{
	uint8* pOut = m_pBank448->GetBase();
	uint8* pLast = m_pBank448->GetLast();

	int32 nTris = pSet->m_nTris18;
	ModelTri* pTri = pSet->m_pTris14;
	for (; nTris; nTris--, pTri++)
	{
		RVert* pV[3];
		pV[0] = &pVerts[pTri->m_Indices[0]];
		pV[1] = &pVerts[pTri->m_Indices[1]];
		pV[2] = &pVerts[pTri->m_Indices[2]];

		if (m_bCull6b4 == 0 || TriFacing(pV[0], pV[1], pV[2]) > g_f1008ab94)
		{

		TLVert* pT;

		pT = (TLVert*)pOut;
		pT->m_vPos = pV[0]->m_vPos;
		pT->m_fRHW = pV[0]->m_fRHW;
		pT->m_nDiffuse = pV[0]->m_nColor10;
		pT->m_nSpecular = m_nSpecular480;
		m_pfnUV434(pOut, pV[0], pTri->m_UV[0]);
		pOut += m_nStride438;

		pT = (TLVert*)pOut;
		pT->m_vPos = pV[1]->m_vPos;
		pT->m_fRHW = pV[1]->m_fRHW;
		pT->m_nDiffuse = pV[1]->m_nColor10;
		pT->m_nSpecular = m_nSpecular480;
		m_pfnUV434(pOut, pV[1], pTri->m_UV[1]);
		pOut += m_nStride438;

		pT = (TLVert*)pOut;
		pT->m_vPos = pV[2]->m_vPos;
		pT->m_fRHW = pV[2]->m_fRHW;
		pT->m_nDiffuse = pV[2]->m_nColor10;
		pT->m_nSpecular = m_nSpecular480;
		m_pfnUV434(pOut, pV[2], pTri->m_UV[2]);
		pOut += m_nStride438;

		if (pOut > pLast && nTris > 1)
		{
			g_nModelTriangles += (m_pBank448->m_nSize20 - m_pBank448->m_nUsed1c) / 3;
			m_pBank448->Draw(g_pD3DDevice, 4,
			                 m_pBank448->m_nSize20 - m_pBank448->m_nUsed1c);
			pOut = m_pBank448->GetBase();
			pLast = m_pBank448->GetLast();
		}
		}
	}

	uint32 nVerts = m_pBank448->GetCount(pOut);
	g_nModelTriangles += nVerts / 3;
	m_pBank448->Draw(g_pD3DDevice, 4, nVerts);
	return 1;
}


// ======================================================================= //
//  0x10008680 -- the run is already in the bank: draw it.
// ======================================================================= //
LTBOOL CD3DModelDraw::sub_10008680(ModelVertSet* pSet, RVert* pVerts)
{
	uint32 nVerts = pSet->m_nTris18 * 3;
	g_nModelTriangles += nVerts / 3;
	m_pBank448->Draw(g_pD3DDevice, 4, nVerts);
	return 1;
}


// ======================================================================= //
//  0x100086f0 -- untransformed triangles.
// ======================================================================= //
LTBOOL CD3DModelDraw::sub_100086f0(ModelVertSet* pSet, RVert* pVerts)
{
	uint8* pOut = m_pBank448->GetBase();
	uint8* pLast = m_pBank448->GetLast();

	int32 nTris = pSet->m_nTris18;
	ModelTri* pTri = pSet->m_pTris14;
	for (; nTris; nTris--, pTri++)
	{
		RVert* pV[3];
		pV[0] = &pVerts[pTri->m_Indices[0]];
		pV[1] = &pVerts[pTri->m_Indices[1]];
		pV[2] = &pVerts[pTri->m_Indices[2]];

		if (m_bCull6b4 == 0 || TriFacing(pV[0], pV[1], pV[2]) > g_f1008ab94)
		{

		LVert* pT;

		pT = (LVert*)pOut;
		pT->m_vPos = pV[0]->m_vPos;
		pT->m_nDiffuse = pV[0]->m_nColor10;
		pT->m_nSpecular = m_nSpecular480;
		pOut -= 4;
		m_pfnUV434(pOut, pV[0], pTri->m_UV[0]);
		pOut += m_nStride438;

		pT = (LVert*)pOut;
		pT->m_vPos = pV[1]->m_vPos;
		pT->m_nDiffuse = pV[1]->m_nColor10;
		pT->m_nSpecular = m_nSpecular480;
		pOut -= 4;
		m_pfnUV434(pOut, pV[1], pTri->m_UV[1]);
		pOut += m_nStride438;

		pT = (LVert*)pOut;
		pT->m_vPos = pV[2]->m_vPos;
		pT->m_nDiffuse = pV[2]->m_nColor10;
		pT->m_nSpecular = m_nSpecular480;
		pOut -= 4;
		m_pfnUV434(pOut, pV[2], pTri->m_UV[2]);
		pOut += m_nStride438;

		if (pOut > pLast && nTris > 1)
		{
			g_nModelTriangles += (m_pBank448->m_nSize20 - m_pBank448->m_nUsed1c) / 3;
			m_pBank448->Draw(g_pD3DDevice, 4,
			                 m_pBank448->m_nSize20 - m_pBank448->m_nUsed1c);
			pOut = m_pBank448->GetBase();
			pLast = m_pBank448->GetLast();
		}
		}
	}

	uint32 nVerts = m_pBank448->GetCount(pOut);
	g_nModelTriangles += nVerts / 3;
	m_pBank448->Draw(g_pD3DDevice, 4, nVerts);
	return 1;
}
