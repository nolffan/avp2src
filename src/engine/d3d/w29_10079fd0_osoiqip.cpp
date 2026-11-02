// *********************************************************************** //
//
//  w29_10079fd0_osoiqip.cpp -- the format manager's convert request and
//  dispatcher (was w12_79fd0_os.cpp, plus 0x1007a014 from d3dr5c.cpp):
//    0x10079fd0 (68 bytes)  ConvertRequest::Construct -- the request's
//                           constructor (LithTech's FMConvertRequest),
//                           reached as a thiscall on a stack local;
//    0x1007a010 (4 + 188)   ConvertRequest::IsValid, a /Qip two-entry
//                           pair: the stack entry 0x1007a010 and the
//                           register entry 0x1007a014;
//    0x1007a130 (76 bytes)  FormatMgr::ConvertPixels(pRequest).
//
//  FLAGS, from the bytes: ConvertPixels calls 0x1007a014 -- the /Qip
//  register entry, past the four-byte `mov 0x4(%esp),%eax` shim at
//  0x1007a010 -- so the request check and its caller share a /Qip TU;
//  the padding is 0x90 runs (/Os).  /Os /Oi- /Qip (the _osoiqip naming
//  convention): measured, /Os /Qip /Qip_no_inlining gives the same bytes
//  for all three; /O2 /Qip does not (239 bytes for IsValid).  Moved here,
//  0x1007a010/0x1007a014 and 0x10079fd0 and 0x1007a130 are identical.
//
//  Construct, read off its stores: +0x00 = &this->m_SrcFormat (+0x28),
//  +0x10 = &this->m_DstFormat (+0x60), both embedded PFormats get the
//  PFormat vtable 0x10089390 (their inlined constructors), then the members
//  in declaration order, the source palette ahead of the source data: icl
//  emits the zero stores in that order and the -1 stores in reverse.  The
//  palette-first order is what puts +0x0c's store ahead of +0x04's
//  (measured; declaration order gives them the other way round).
//  The vtable is reached through d3dr4c.cpp's spelling g_10089390 (defined
//  in d3ddata.cpp once every slot's function is written) rather than
//  by giving PFormat virtual functions here, which would emit a second
//  vtable.
//
//  IsValid is lithtech.exe's FMConvertRequest::IsValid (0x0045f280,
//  src/engine/lithtech/lttexmod.cpp) with two spellings the icl bytes pin:
//    * the block-format test is an inline over the PFormat POINTER, so the
//      format code is re-read at every test (the shipped `mov 0x4(%eax),%esi`
//      per statement); over the code, or as a macro, icl CSEs it;
//    * the source pitch test joins the first guard's `||` chain, which is
//      what hosts the shared `return 0` after it (0x1007a05b) rather than
//      after the height test;
//    * the two palette tails are one-case switches: that hosts their `== 0`
//      and `== 7` arms past the epilogue, the second arm jumping back to
//      the `== 7` test (0x1007a0c7), as shipped.  With plain `if`s icl keeps
//      the arms in line.
//
//  ConvertPixels: if the request fails its check the answer is 1;
//  otherwise the converter at 0x100898b0[src type][dst type] (8 per row,
//  each `(FormatMgr*, ConvertRequest*)`, cdecl) is called and its answer
//  returned, or 0x53 when that slot is empty.  The failing check is a
//  one-case switch too: icl otherwise predicts it unlikely and hosts the
//  `return 1` past the epilogue, where the shipped code keeps it in line.
//
//  References: big3a080.cpp's ConvertRequest::Construct() and
//  FormatMgr::ConvertPixels(ConvertRequest*) -- the names defined here.
//
// *********************************************************************** //

typedef unsigned long uint32;
typedef unsigned char uint8;

extern uint8 g_10089390[];                              // 0x10089390 PFormat's vtable

struct PFormat
{
	void*	m_pVTable;                                  // +0x00
	uint32	m_nType;                                    // +0x04
	uint8	m_pad_08[0x30];                             // +0x08
};                                                      // 0x38

struct ConvertRequest
{
	ConvertRequest* Construct();                        // 0x10079fd0
	uint32	sub_1007a010() const;                       // 0x1007a010 / 0x1007a014 IsValid

	PFormat* m_pSrcFormat;                              // +0x00
	void*    m_pSrcData;                                // +0x04
	uint32   m_nSrcPitch;                               // +0x08
	void*    m_pSrcPalette;                             // +0x0c
	PFormat* m_pDstFormat;                              // +0x10
	void*    m_pDstData;                                // +0x14
	uint32   m_nDstPitch;                               // +0x18
	uint32   m_nWidth;                                  // +0x1c
	uint32   m_nHeight;                                 // +0x20
	uint32   m_nFlags;                                  // +0x24
	PFormat  m_SrcFormat;                               // +0x28
	PFormat  m_DstFormat;                               // +0x60
};                                                      // 0x98

class FormatMgr;
typedef uint32 (*ConvertFn)(FormatMgr* pMgr, ConvertRequest* pRequest);
extern ConvertFn g_100898b0[][8];                       // 0x100898b0

class FormatMgr
{
public:
	uint32 ConvertPixels(ConvertRequest* pRequest);     // 0x1007a130
};

ConvertRequest* ConvertRequest::Construct()
{
	m_SrcFormat.m_pVTable = g_10089390;
	m_DstFormat.m_pVTable = g_10089390;
	m_pSrcFormat = &m_SrcFormat;
	m_pSrcPalette = 0;
	m_pSrcData = 0;
	m_nSrcPitch = 0xffffffff;
	m_pDstFormat = &m_DstFormat;
	m_pDstData = 0;
	m_nDstPitch = 0xffffffff;
	m_nWidth = 0xffffffff;
	m_nHeight = 0xffffffff;
	m_nFlags = 0;
	return this;
}

// Codes 4, 5 and 6 are the block-compressed (DXT) formats: they need no
// pitch and their extents must be multiples of four.
inline int IsBlockFormat(PFormat* pFormat)
{
	return pFormat->m_nType == 4 || pFormat->m_nType == 5 || pFormat->m_nType == 6;
}

uint32 ConvertRequest::sub_1007a010() const
{
	if (!m_pSrcFormat || !m_pSrcData || !m_pDstFormat || !m_pDstData ||
	    m_nWidth == 0xffffffff || m_nHeight == 0xffffffff ||
	    (!IsBlockFormat(m_pSrcFormat) && m_nSrcPitch == 0xffffffff))
		return 0;

	if (!IsBlockFormat(m_pDstFormat) && m_nDstPitch == 0xffffffff)
		return 0;

	if (IsBlockFormat(m_pSrcFormat) || IsBlockFormat(m_pDstFormat))
	{
		if ((m_nHeight | m_nWidth) & 3)
			return 0;
	}

	switch (m_pSrcFormat->m_nType == 0)
	{
		case 0: break;
		default:
			if (!m_pSrcPalette)
				return 0;
	}

	switch (m_pSrcFormat->m_nType == 7)
	{
		case 0: break;
		default:
			if (!m_pSrcPalette)
				return 0;
	}

	return 1;
}

uint32 FormatMgr::ConvertPixels(ConvertRequest* pRequest)
{
	ConvertFn fn;

	switch (pRequest->sub_1007a010())
	{
		case 0: return 1;
	}

	fn = g_100898b0[pRequest->m_pSrcFormat->m_nType][pRequest->m_pDstFormat->m_nType];
	if (!fn)
		return 0x53;

	return fn(this, pRequest);
}
