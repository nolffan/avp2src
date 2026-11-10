// *********************************************************************** //
//  ph/d3d_tables.cpp -- Primal Hunt's d3d.ren, TU41's first object
//  (0x10036a97..0x10036b96).
//
//  Three lookup tables with constructors, nothing else (whatever code this
//  object had, /OPT:REF removed).  It is not pixelformat.cpp: its
//  initialisers have no /Os jmp stub (each is `mov ecx,obj; jmp ctor`):
//  /O1 /Ob2, which expands each into its table entry (/Ob1 leaves a stub).
//  Its `rep stosd` is no intrinsic memset: VC6's loop optimiser makes it
//  of a byte-fill loop (CIdentTable's constructor).  The world draw
//  (TU15), DrawWorldModels (TU34) and the lightmap code (TU36, TU37) read
//  the tables.
// *********************************************************************** //

#include "ltbasedefs.h"

#include <string.h>
#include <math.h>


// ------------------------------------------------------------------------------ //
// The renderer's lookup tables (0x10036a97..0x10036b96).  Their dynamic
// initialisers come first in this object's run of the CRT table; the world,
// lightmap and fog code reads them.
// ------------------------------------------------------------------------------ //

// 0x10097c00: an identity byte table, then 256 bytes of 0xFF.
class CIdentTable
{
public:
			CIdentTable();

	uint8	m_Ident[256];
	uint8	m_Full[256];
};

// 0x10087c00: a * b / 255 for every pair of bytes.
class CMulTable
{
public:
			CMulTable();

	uint8	m_Table[256][256];
};

// 0x10087b00: the fog colour's gamma curve, sqrt(i / 512) * 255.
class CGammaTable
{
public:
			CGammaTable();

	uint8	m_Table[256];
};

CIdentTable	g_IdentTable;	// 0x10097c00, initialiser 0x10036a97
CMulTable	g_MulTable;		// 0x10087c00, initialiser 0x10036ac3
CGammaTable	g_GammaTable;	// 0x10087b00, initialiser 0x10036b25


// 0x10036aa1
CIdentTable::CIdentTable()
{
	uint32 i;

	for(i=0; i < 256; i++)
		m_Ident[i] = (uint8)i;

	for(i=0; i < 256; i++)
		m_Full[i] = 0xFF;
}

// 0x10036acd
CMulTable::CMulTable()
{
	uint32 i, j;

	for(i=0; i < 256; i++)
	{
		for(j=0; j < 256; j++)
		{
			m_Table[i][j] = (uint8)((float)j * (float)i * (1.0f / 255.0f));
		}
	}
}

// 0x10036b2f
CGammaTable::CGammaTable()
{
	uint32 i;
	float fVal;

	for(i=0; i < 256; i++)
	{
		fVal = (float)sqrt((float)i * (1.0f / 512.0f)) * 255.0f;
		if(fVal < 0.0f)
			fVal = 0.0f;
		else if(fVal > 255.0f)
			fVal = 255.0f;

		m_Table[i] = (uint8)fVal;
	}
}
