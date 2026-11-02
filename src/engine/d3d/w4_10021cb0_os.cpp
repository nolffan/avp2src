// *********************************************************************** //
//
//  MODULE:   W4_10021CB0_OS (.CPP)  -- /Os
//
//  PURPOSE:  Functions the d3d.ren link referenced by address and nobody
//            had written, 0x10021cb0..0x10021f44: the renderer's free,
//            console-print and warning wrappers around the engine's
//            RenderStruct callbacks (0x1008cbcc), and the console-variable
//            record's registration member.
//
//  Flag set: /Os -- 0x90 padding after each body (after a `mov %esi,%esi`
//  / `lea 0x0(%esi),%esi` to a 4-byte boundary where needed).
//
// *********************************************************************** //

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ltbasedefs.h"


// The engine's RenderStruct, as far as these functions call into it.
struct RenderStructCallbacks
{
	uint8    m_Reserved00[0x14];                        // +0x00..+0x13
	void   (*ConsolePrint)(const char* pszMsg);         // +0x14
	uint8    m_Reserved18[0x38 - 0x18];                 // +0x18..+0x37
	void   (*Free)(void* p);                            // +0x38
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc
extern int32 g_CV_RenderDebug;                          // 0x1008caac   RenderDebug


// ======================================================================= //
//  0x10021cb0 -- free a block through the engine's allocator, or the CRT's
//  (0x10080b8e) when there is no RenderStruct.
// ======================================================================= //
void sub_10021cb0(void* p)
{
	if (g_pStruct)
		g_pStruct->Free(p);
	else
		free(p);
}


// ======================================================================= //
//  0x10021d10 -- printf to the console: _vsnprintf (0x10080c6c) into a
//  256-byte buffer, at most 255 characters, then RenderStruct +0x14.
// ======================================================================= //
void sub_10021d10(const char* pszFormat, ...)
{
	char szBuf[256];
	va_list marker;

	va_start(marker, pszFormat);
	_vsnprintf(szBuf, 255, pszFormat, marker);
	va_end(marker);

	g_pStruct->ConsolePrint(szBuf);
}


// ======================================================================= //
//  0x10021d60 -- d3d_Warning: the same, only when the RenderDebug console
//  variable is at least the message's level (`cmp ; jg` -- signed).
// ======================================================================= //
void sub_10021d60(uint32 nLevel, const char* pszFormat, ...)
{
	char szBuf[256];
	va_list marker;

	if ((int32)nLevel <= g_CV_RenderDebug)
	{
		va_start(marker, pszFormat);
		_vsnprintf(szBuf, 255, pszFormat, marker);
		va_end(marker);

		g_pStruct->ConsolePrint(szBuf);
	}
}


// ======================================================================= //
//  0x10021f44 -- CD3DConVar's registration member (big21f90.cpp): fill in
//  the record, clear its value and handle, and push it on the global list
//  at 0x1008bf90.  __thiscall, four stack arguments, returns `this`.
//
//  0x10021f40 is the same function's OTHER entry: `mov 0x4(%esp),%eax`
//  falling straight into 0x10021f44, whose body never reads %eax.  That is
//  icl /Qip's stack-entry/register-entry pair; 0x10021f90 (big21f90.cpp)
//  enters at 0x10021f44 and the 76 other callers at 0x10021f40.  The pair
//  only comes out of a TU that also holds its register-entry caller, so
//  here the body is written once and every sub_10021f40 spelling is an
//  alias of it (link/aliases.rsp): the four bytes before it are a dead
//  load of a caller-saved register.
// ======================================================================= //
struct Var10021dc0;
extern Var10021dc0* g_1008bf90;                         // 0x1008bf90

class CD3DConVar
{
public:
	CD3DConVar* sub_10021f44(const char* pszName, float fDefault,
	                         int32* pnValue, float* pfValue);

	int32       m_nValue00;                             // +0x00 value as int
	float       m_fValue04;                             // +0x04 value as float
	float       m_fDefault08;                           // +0x08 the default
	int32*      m_pnValue0c;                            // +0x0c int32 dest
	float*      m_pfValue10;                            // +0x10 float dest
	const char* m_pszName14;                            // +0x14 the name
	void*       m_pHandle18;                            // +0x18 console handle
	CD3DConVar* m_pNext1c;                              // +0x1c chain
};

CD3DConVar* CD3DConVar::sub_10021f44(const char* pszName, float fDefault,
                                     int32* pnValue, float* pfValue)
{
	m_fDefault08 = fDefault;
	m_pnValue0c  = pnValue;
	m_pfValue10  = pfValue;
	m_pszName14  = pszName;
	m_nValue00   = 0;
	m_fValue04   = 0.0f;
	m_pHandle18  = LTNULL;

	CD3DConVar* pHead = (CD3DConVar*)g_1008bf90;
	g_1008bf90 = (Var10021dc0*)this;
	m_pNext1c  = pHead;
	return this;
}
