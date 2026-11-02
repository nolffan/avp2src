// *********************************************************************** //
//
//  w12_15950_os.cpp -- 0x10015950 (119 bytes): D3DRenderInfo's constructor
//  (big17c14.cpp constructs one on its frame at -0x68c).
//
//  The shipped body loops EIGHT times over 0x1c-byte records
//  (`mov $8,%edi ... add $0x1c,%ebx; dec %edi; jne`), storing -1 into each
//  record's +0x18 and then calling 0x1001e0e0 on it with six zeros
//  (d3dlow.cpp's C1001e0e0::sub_1001e0e0, which fills +0x00..+0x14) --
//  the record's own inlined constructor -- and then zeroes +0xe4, +0xe8,
//  +0xec, +0xf0, +0xf4 and +0xe0, in that order, returning `this`.
//
//  IDENTICAL at /Os (0x10015950..0x100159c7 against
//  '??0D3DRenderInfo@@QAE@XZ': 119 bytes, byte-identical
//  modulo relocations); /O2 is 144 bytes (mov-saved registers). The byte
//  comparison cannot map a constructor's name to its address.
//
// *********************************************************************** //

typedef unsigned long uint32;
typedef unsigned short uint16;

struct RenderStage15950
{
	RenderStage15950()
	{
		m_n18 = 0xffffffff;
		sub_1001e0e0(0, 0, 0, 0, 0, 0.0f);
	}

	// 0x1001e0e0 -- declared only. d3dlow.cpp reads w04..w0e as WORDs;
	// this caller stores six full dwords, the last from a zero of its own
	// (`xor %eax,%eax; mov %eax,0x14(%esp)` while the other five share
	// %esi), which is what a 0.0f argument among five integer zeros gives.
	// Same six stack dwords either way.
	void sub_1001e0e0(uint32 n00, uint32 w04, uint32 w06,
	                  uint32 w0c, uint32 w0e, float f14);

	uint32 m_n00;                                       // +0x00
	uint16 m_w04;                                       // +0x04
	uint16 m_w06;                                       // +0x06
	float  m_f08;                                       // +0x08
	uint16 m_w0c;                                       // +0x0c
	uint16 m_w0e;                                       // +0x0e
	float  m_f10;                                       // +0x10
	uint32 m_n14;                                       // +0x14
	uint32 m_n18;                                       // +0x18
};

struct D3DRenderInfo
{
	D3DRenderInfo();

	RenderStage15950 m_Stages[8];                       // +0x000 .. +0x0df
	uint32           m_n0e0;                            // +0x0e0
	uint32           m_n0e4;                            // +0x0e4
	uint32           m_n0e8;                            // +0x0e8
	uint32           m_n0ec;                            // +0x0ec
	uint32           m_n0f0;                            // +0x0f0
	uint32           m_n0f4;                            // +0x0f4
};

D3DRenderInfo::D3DRenderInfo()
{
	m_n0e4 = 0;
	m_n0e8 = 0;
	m_n0ec = 0;
	m_n0f0 = 0;
	m_n0f4 = 0;
	m_n0e0 = 0;
}
