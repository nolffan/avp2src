// *********************************************************************** //
//
//  w12_21c50_osoi.cpp -- 0x10021c50 (92 bytes, IDENTICAL at /Os /Oi-; /Os
//  alone expands the memset, 81 bytes differ): d3d_Alloc, the renderer's
//  zero-filling allocator.
//
//  d3dmid.cpp's 0x10021c20 (identical there, /Os) is the uncleared form:
//  ask the engine's dalloc (RenderStruct +0x34, g_pStruct at 0x1008cbcc)
//  if the engine is there, else the CRT malloc (0x100809f8). This one
//  follows the same two arms with memset(p, 0, n) (`call 0x100809a0`)
//  when the allocation succeeded, and otherwise -- engine permitting --
//  prints "d3drender.dll: out of memory" (0x100a5690) through
//  RenderStruct +0x14, the console print d3d_Print (0x10021d10) also calls.
//  The result is returned either way.
//
//  References: d3dblit.cpp and d3ddraw.cpp's d3d_Alloc(unsigned long),
//  aliased onto this definition in link/aliases.rsp.
//
// *********************************************************************** //

#include <stdlib.h>
#include <string.h>

typedef unsigned long uint32;
typedef unsigned char uint8;

struct RenderStructCallbacks
{
	uint8   m_Reserved00[0x14];                         // +0x00..+0x13
	void  (*ConsolePrint)(const char* pMsg, ...);       // +0x14
	uint8   m_Reserved18[0x1c];                         // +0x18..+0x33
	void* (*dalloc)(uint32 nSize);                      // +0x34
	void  (*dfree)(void* pMem);                         // +0x38
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc

void* sub_10021c50(uint32 nSize)
{
	void* p;

	if (g_pStruct)
		p = g_pStruct->dalloc(nSize);
	else
		p = malloc(nSize);

	if (p)
		memset(p, 0, nSize);
	else if (g_pStruct)
		g_pStruct->ConsolePrint("d3drender.dll: out of memory");

	return p;
}
