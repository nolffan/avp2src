// *********************************************************************** //
//
//  w14_10003bb0.cpp -- the two bank-vertex copies 0x10003bb0 and
//  0x10003bf0 (written for the d3d.ren link).
//
//  0x10008a44 installs one of them in the model drawer's +0x43c by
//  format (`movl $0x10003bb0,0x43c(%ebx)` for the one-texture 32-byte
//  vertex, `movl $0x10003bf0,0x43c(%ebx)` for the two-texture 40-byte
//  one), and the clip paths of 0x10006840 / 0x10007620 fan a clipped
//  polygon into the bank through it (`mov %edi,%ecx ; mov -0x48(%ebp),
//  %edx ; call *%eax`): destination in %ecx, source in %edx, no stack
//  argument, plain `ret` -- __fastcall with two arguments.
//
//  /O2: both bodies are padded to 16 with multi-byte lea-nops.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// A one-texture bank vertex (FVF 0x1c4) and a two-texture one (0x2c4).
struct BankVert20
{
	uint32      m_Dwords[8];
};

struct BankVert28
{
	uint32      m_Dwords[10];
};


// 0x10003bb0 -- copy a 32-byte bank vertex.
extern "C" void __fastcall sub_10003bb0(BankVert20* pDst, BankVert20* pSrc)
{
	*pDst = *pSrc;
}

// 0x10003bf0 -- copy a 40-byte bank vertex.
extern "C" void __fastcall sub_10003bf0(BankVert28* pDst, BankVert28* pSrc)
{
	*pDst = *pSrc;
}
