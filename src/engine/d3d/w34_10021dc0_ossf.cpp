// *********************************************************************** //
//
//  w34_10021dc0_ossf.cpp -- 0x10021dc0, moved out of d3dr3c.cpp.
//
//  FLAGS: /Os /Qsfalign16 (the `_ossf` convention in build/engine.sh).
//  /Os: the twelve 0x90s of padding after it.  /Qsfalign16: the float
//  default is promoted to a double for sprintf's %f, and under icl's
//  default (/Qsfalign8, "align stack for functions with 8 or 16 byte
//  vars") that double gives the function the 14-byte stack-realigning
//  entry (`push %ebx ; mov %esp,%ebx ; sub $8 ; and $-16 ; add $8 ; jmp`)
//  and a second body behind it -- 112 of 138 bytes off at every flag group
//  and with every spelling of the argument (no local, a (double) cast, a
//  volatile read).  The shipped function has the same double spill slot
//  (`fstpl 0x118(%esp)`) and no realign; /Qsfalign16 (/Qsfalign- gives the
//  same bytes) makes it byte-identical as written.
//
// *********************************************************************** //

#include "ltbasedefs.h"

#include <stdio.h>


// The engine's RenderStruct at 0x1008cbcc (d3dr3c.cpp's spelling, which is
// d3dbatchc.cpp's): +0x10 runs a console string, +0x18 looks a console
// variable up by name.
struct RenderStructCallbacks
{
	uint8    m_Reserved00[0x10];                        // +0x00..+0x0f
	void    (*RunConsoleString)(const char* pszCommand); // +0x10
	uint8    m_Reserved14[0x04];                        // +0x14..+0x17
	void*   (*GetConsoleVariable)(const char* pszName); // +0x18
	double  (*GetVarValueFloat)(void* pVar);            // +0x1c
	char*   (*GetVarValueString)(void* pVar);           // +0x20
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// ======================================================================= //
//  0x10021dc0 -- 132 bytes.  DECODED, every instruction.
//
//  Bind a list of console-variable records to their live console vars,
//  CREATING any that do not exist yet by running "<name> <value>" through
//  the engine's console.  Called from 0x100215b6, three instructions after
//  the 0xd5d version handshake that d3d.cpp documents, so this is renderer
//  start-up.
//
//  The record: +0x08 float default, +0x14 name, +0x18 the handle it fills
//  in, +0x1c next.  The format string at 0x100a56b0 is "%s %f" and the
//  value reaches the CRT's sprintf (0x10080cbc) as a `fstpl`, i.e. the
//  usual float->double vararg promotion.
//
//  The scratch buffer is 0x18(%esp)..0x117(%esp) -- 256 bytes.
// ======================================================================= //

struct Var10021dc0
{
	uint8         m_pad_00[0x08];                       // +0x00..+0x07
	float         m_fDefault;                           // +0x08
	uint8         m_pad_0c[0x08];                       // +0x0c..+0x13
	const char*   m_pszName;                            // +0x14
	void*         m_pVar;                               // +0x18
	Var10021dc0*  m_pNext;                              // +0x1c
};

extern Var10021dc0* g_1008bf90;                         // 0x1008bf90

void sub_10021dc0()
{
	Var10021dc0* pVar;
	char         szCommand[256];

	pVar = g_1008bf90;
	while (pVar)
	{
		float       fDefault = pVar->m_fDefault;
		const char* pszName  = pVar->m_pszName;
		void*       pConVar;

		pConVar = g_pStruct->GetConsoleVariable(pszName);
		if (!pConVar)
		{
			sprintf(szCommand, "%s %f", pszName, fDefault);
			g_pStruct->RunConsoleString(szCommand);
			pConVar = g_pStruct->GetConsoleVariable(pszName);
		}

		pVar->m_pVar = pConVar;
		pVar = pVar->m_pNext;
	}
}
