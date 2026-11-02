// *********************************************************************** //
//
//  w2_10008da0.cpp -- CD3DModelDraw::sub_10008e10 (written for the
//  d3d.ren link; ::sub_10008da0 has moved to
//  w14_10008a40_qipni.cpp, with the 0x10008a40 it calls).
//
//  Both are __thiscall members of big17c14.cpp's CD3DModelDraw (the
//  global drawer at 0x1008b090): big43e10.cpp calls them as such, and
//  d3dbatcha_o2.cpp reaches the same two addresses under its own class
//  name C10044f74 (aliased in link/aliases.rsp).
//
//  /O2: 0x10008da0 is padded to 0x10008e10 with one 0x90 and 0x10008e10
//  to 0x10008fd0 with lea-nops; both keep icl's %ebp frame.
//
//  Residual (not source-reachable from here): both shipped bodies call
//  their callee's /Qip REGISTER entry (0x10008a44 = sub_10008a40 + 4 and
//  0x10008fd4 = sub_10008fd0 + 4), i.e. in the shipped build each sat in
//  one /Qip translation unit with its callee.  Here they call the stack
//  entries, which is the same call with the same arguments.
//
// *********************************************************************** //

#include "ltbasedefs.h"


struct LTMat;
struct ModelVert;

// The output vertex (big8fd0.cpp's RVert, FVF 0x1c4): only its size.
struct RVert
{
	uint8       m_Bytes[0x20];
};

// 0x1008b874 -- a model-draw mode word (big17c14.cpp's spelling).
extern unsigned long g_1008b874;                            // 0x1008b874
// 0x1008ca38 -- ModelProfile console variable.
extern long g_CV_ModelProfile;                              // 0x1008ca38
// 0x1008ac54 / 0x1008ac34 -- u/v overrides for the model texture scale
// (+0x46c / +0x470): a nonzero value replaces the piece's own.  Read only
// (0x10008c45.., 0x10008f01..), never written by address in .text.
extern float g_f1008ac54;                                   // 0x1008ac54
extern float g_f1008ac34;                                   // 0x1008ac34

// 0x10021d10 -- the console printf (big17c14.cpp's declaration).
extern "C" void sub_10021d10(const char* pFmt, ...);

// One vertex run of a piece, 0x24 bytes (big8fd0.cpp's ModelVertSet).
struct ModelVertSet
{
	uint32      m_n00;                                      // +0x00
	void*       m_pVerts04;                                 // +0x04
	uint32      m_nVerts08;                                 // +0x08
	uint8       m_pad_0c[0x24 - 0x0c];                      // +0x0c
};

// One model piece.  Its first 0x24 bytes are its base vertex run (index 0
// returns the piece itself), +0x38/+0x3c the further runs.
struct ModelPiece8e10 : public ModelVertSet
{
	uint8       m_pad_24[0x2c - 0x24];                      // +0x24
	float       m_fTexScaleU2c;                             // +0x2c
	float       m_fTexScaleV30;                             // +0x30
	uint8       m_pad_34[0x38 - 0x34];                      // +0x34
	ModelVertSet* m_pSets38;                                // +0x38
	uint32      m_nSets3c;                                  // +0x3c

	ModelVertSet* GetSet(uint32 i)
	{
		ModelVertSet* pSet;
		if (i == 0)
			pSet = this;
		else if (i - 1 >= m_nSets3c)
			pSet = 0;
		else
			pSet = &m_pSets38[i - 1];
		return pSet;
	}
};

// The pieces are a CMoArray (vptr +0x34, m_pArray +0x38, m_nElements
// +0x3c), read through its inline GetSize()/operator[].
struct PieceArray8e10
{
	ModelPiece8e10*&	operator[](uint32 i) const	{ return m_pArray[i]; }
	uint32				GetSize() const				{ return m_nElements; }

	void*				m_pVtbl;                        // +0x00
	ModelPiece8e10**	m_pArray;                       // +0x04
	uint32				m_nElements;                    // +0x08
};

struct D3DModel
{
	uint8       m_pad_00[0x34];                             // +0x00
	PieceArray8e10 m_Pieces;                                // +0x34
	// The model's own inline piece accessors (the same pair
	// makes w14_10008a40_qipni.cpp's 0x10008a44 identical).
	uint32          NumPieces()          { return m_Pieces.m_nElements; }
	ModelPiece8e10* GetPiece(uint32 i)   { return m_Pieces.m_pArray[i]; }
	uint8       m_pad_40[0x74 - 0x40];                      // +0x40
	LTMat*      m_pMatrices74;                              // +0x74
};

struct D3DObject
{
	uint8       m_pad_000[0x88];                            // +0x000
	uint32      m_nFlags88;                                 // +0x088
	uint8       m_pad_08c[0x204 - 0x08c];                   // +0x08c
	uint32      m_nHiddenPieces204;                         // +0x204
};

class CD3DModelDraw
{
public:
	// big8fd0.cpp's texture-coordinate callback type, copied verbatim so
	// sub_10008fd0 mangles as that file defines it.
	typedef void (CD3DModelDraw::*PFNTEXCOORD)(ModelVert* pVert,
	                                           RVert* pOut);

	void        sub_10008da0(unsigned long nValue);         // 0x10008da0
	void        sub_10008e10();                             // 0x10008e10
	void        sub_10008a40(void* pfnSetup, unsigned long nValue); // 0x10008a40
	void        sub_10008fd0(ModelVertSet* pSet1, ModelVertSet* pSet2,
	                         RVert* pOut, PFNTEXCOORD pfnTexCoord,
	                         LTMat* pMatrices);             // 0x10008fd0

	D3DModel*   m_pModel00;                                 // +0x000
	D3DObject*  m_pObject04;                                // +0x004
	uint8       m_pad_008[0x42c - 0x008];                   // +0x008
	PFNTEXCOORD m_pfnTexCoord42c;                           // +0x42c
	uint8       m_pad_430[0x44c - 0x430];                   // +0x430
	uint32      m_n44c;                                     // +0x44c
	uint32      m_n450;                                     // +0x450
	uint8       m_pad_454[0x46c - 0x454];                   // +0x454
	float       m_fTexScaleU46c;                            // +0x46c
	float       m_fTexScaleV470;                            // +0x470
	uint8       m_pad_474[0x62c - 0x474];                   // +0x474
	RVert*      m_pVerts62c;                                // +0x62c
	uint8       m_pad_630[0x640 - 0x630];                   // +0x630
	LTMat*      m_pMatrices640;                             // +0x640
	uint8       m_pad_644[0x6ac - 0x644];                   // +0x644
	uint32      m_n6ac;                                     // +0x6ac
	uint32      m_n6b0;                                     // +0x6b0
};


// 0x10008da0 is in w14_10008a40_qipni.cpp: it enters 0x10008a40 at the
// /Qip register entry 0x10008a44, so the two share one /Qip TU there.


// ======================================================================= //
//  0x10008e10 -- transform every visible piece's vertices into the
//  output buffer at +0x62c, one sub_10008fd0 per piece.
// ======================================================================= //
void CD3DModelDraw::sub_10008e10()
{
	// Identical.  The call below goes to 0x10008fd0's stack entry, where
	// shipped calls its /Qip register entry 0x10008fd4 (masked as a
	// relocation; reachable only with this function in big8fd0.cpp's /Qip
	// TU).  What made it (was 77 off): the pieces through the model's
	// NumPieces()/GetPiece(i) (shipped keeps the model in a slot across the
	// loop and re-reads the count at each test), a one-site volatile read of
	// m_pModel00 in the matrices arm (shipped re-reads the model after the
	// switch instead of reusing that arm's load), the vertex count through a
	// named local (shipped loads it, then adds), and pSet2 zeroed before
	// nOut (the other order swaps their two frame stores).
	LTMat* pMatrices;
	uint32 nOut;
	ModelVertSet* pSet2;
	switch (g_1008b874 == 0)
	{
		case 0: pMatrices = (*(D3DModel* volatile*)&m_pModel00)->m_pMatrices74; break;
		default: pMatrices = m_pMatrices640;
	}

	pSet2 = 0;
	nOut = 0;
	for (uint32 i = 0; i < m_pModel00->NumPieces(); i++)
	{
		ModelPiece8e10* pPiece = m_pModel00->GetPiece(i);
		ModelVertSet* pSet1 = pPiece->GetSet(m_n44c);
		if (!pSet1)
			continue;
		if (m_n450)
			pSet2 = pPiece->GetSet(m_n44c + 1);
		if ((1 << i) & m_pObject04->m_nHiddenPieces204)
			continue;

		m_fTexScaleU46c = pPiece->m_fTexScaleU2c;
		m_fTexScaleV470 = pPiece->m_fTexScaleV30;
		if (g_f1008ac54 != 0.0f)
			m_fTexScaleU46c = g_f1008ac54;
		if (g_f1008ac34 != 0.0f)
			m_fTexScaleV470 = g_f1008ac34;

		RVert* pOut = &m_pVerts62c[nOut];
		sub_10008fd0(pSet1, pSet2, pOut, m_pfnTexCoord42c, pMatrices);
		uint32 nVerts = pSet1->m_nVerts08;
		nOut += nVerts;
	}

	if (g_CV_ModelProfile)
		sub_10021d10("SetupVerts: %d", 0);
}
