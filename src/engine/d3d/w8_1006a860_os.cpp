// *********************************************************************** //
//
//  w8_1006a860_os.cpp -- 0x1006a860, the LMDynamic light-map builder.  /Os:
//  pads with a 2-byte nop to the next function, keeps `push` saves under
//  the frame; measured 1036 bytes at /Os against 1542 at /O2 (shipped 1086).
//
//  __cdecl, four stack dwords, plain `ret`, answers 0/1 in %eax.  Called
//  from big0ae20.cpp (0x1000b245, fourth argument 1.0f) and big3dd50.cpp
//  (0x1003e166, fourth argument 2.0f), each time straight after
//  0x1006d550 filled the descriptor in argument 1.
//
//  WHAT IT DOES, read off the bytes:
//    * `if (!g_varLMDynamic.m_nValue00) return 0;` (0x1006a86c).
//    * t = (Dot(plane normal, light position) - plane dist) / radius, where
//      the plane is argument 2's +0x28 -> +0x34 and the radius is the float
//      the light interface's vtable slot +0x38 answers (COM spelling: the
//      interface loaded twice, pushed as the only argument).
//    * reject (0) when t > 1 - 1/width; the width is the descriptor's +0x0c,
//      converted as an unsigned 32-bit value (`fildll` of a zero-extended
//      pair).
//    * the light colour, a 0x00RRGGBB dword built with bit-field inserts
//      from the interface's bytes +0x94/+0x95/+0x96 as max(c, 127) - 127;
//      scaled per channel by argument 4 when that is not 1.0f.  A black
//      result answers 0.
//    * clear the first and last rows and -- the shipped loop never advances
//      its row pointer -- column 0 and column width-1 of ROW 1 only,
//      height-2 times over.  Reproduced as shipped.
//    * the interior: x and y step from -1 across 2/(w-2) and 2/(h-2); where
//      x*x + y*y + t*t < 1 the pixel is the colour scaled by that remainder
//      through a 256x256 multiply table (a function-local static whose
//      constructor fills table[i][j] = i * j / 255; guard bit 0 of
//      0x100a2180, just past the 64K table at 0x10092180), packed 5:5:5;
//      elsewhere 0.
//
//  The descriptor: +0x00 the pixel pointer (uint16), +0x04 the pitch in
//  bytes (halved into uint16 units), +0x0c width, +0x10 height.
//
//  NEAR, same frame (0x80) and the same blocks.  Differing
//  lines against the shipped body 76 -> 61 (945 -> 559 bytes):
//    * the radius is a VIRTUAL __thiscall (the engine's CObjLight::
//      GetLightRadius, objtypes.h, takes one unused argument) on a class
//      deriving from a fourteen-slot base: `this` in %ecx and the vtable
//      slot loaded into %eax before `call *%eax`, as shipped;
//    * its argument is the same +0x04 pointer read a SECOND time (shipped
//      loads +0x04 twice, into %edx and %ecx).  Passing the interface
//      pointer itself is CSE'd into one load; reading it through a second
//      member of a union at +0x04 reproduces the two loads.  Probably the
//      real code passes the object handle there (HOBJECT and the object
//      pointer are the same value), so the union is the measured stand-in;
//    * the last-row pointer reads the height from the descriptor afresh
//      (`pDesc->m_nHeight10 - 1`): with the local, icl CSEs height-1 with
//      the second clearing loop's bound and hoists it, where shipped
//      recomputes it after the first loop.
//  IDENTICAL, by three more source facts:
//    * the lit pixel is an INLINE FUNCTION holding the table's function-
//      local static (LitPixel1006a860): with the static and its guard
//      written straight in the arm, icl predicts the arm cold and sinks it
//      past the loop exit; behind an inline call it stays in line before
//      the latch, as shipped (559 -> 565 bytes on its own: the arm moved,
//      the `!color` exit did not);
//    * `if (!color)` jumps to a `L_Fail: return LTFALSE;` label after the
//      final `return LTTRUE` -- the shipped `jne ; jmp` trampoline into
//      the first test's shared return, laid in line; `return LTFALSE`
//      there sinks the trampoline to the end (and `if (color) ; else`,
//      a one-case switch, an (int32)/(LTBOOL) cast do not move it);
//    * the step locals are declared BEFORE the positions (fStepX, fStepY,
//      fX, fY): the latches then load the position and add the step
//      (`fldl fX ; faddl fStepX`), the declaration order deciding the
//      operands of the commutative add; `fX = fX + fStepX` / `fStepX + fX`
//      change nothing.
//  (Before that: the `!color` exit and the lit arm's placement were
//  the residual; tried then without effect: `color == 0`, braces, an
//  empty then-arm; `>=`, `<=`, `!(<)` and swapped arms on the 1.0 test;
//  the static declared first in the arm; the table as a member of the
//  static object; double locals (worse).)
// *********************************************************************** //

#include "ltbasedefs.h"

struct CD3DConVar
{
	int32		m_nValue00;                             // +0x00
	float		m_fValue04;                             // +0x04
	uint8		m_pad_08[0x20 - 0x08];
};

extern CD3DConVar	g_varLMDynamic;                     // 0x10091200

struct Vec1006a860
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

struct Plane1006a860
{
	Vec1006a860		m_Normal;                           // +0x00
	float			m_Dist;                             // +0x0c
};

struct Basis1006a860
{
	uint8			m_pad_00[0x34];
	Plane1006a860*	m_pPlane34;                         // +0x34
};

struct Surf1006a860
{
	uint8			m_pad_00[0x28];
	Basis1006a860*	m_p28;                              // +0x28
};

struct LightBase1006a860
{
	virtual void Slot00() = 0;  virtual void Slot04() = 0;
	virtual void Slot08() = 0;  virtual void Slot0c() = 0;
	virtual void Slot10() = 0;  virtual void Slot14() = 0;
	virtual void Slot18() = 0;  virtual void Slot1c() = 0;
	virtual void Slot20() = 0;  virtual void Slot24() = 0;
	virtual void Slot28() = 0;  virtual void Slot2c() = 0;
	virtual void Slot30() = 0;  virtual void Slot34() = 0;
};

struct LightIface1006a860 : public LightBase1006a860
{
	virtual float Slot38(void* pUnused) = 0;                        // +0x38

	uint8					m_pad_04[0x94 - 0x04];
	uint8					m_nR94;                     // +0x94
	uint8					m_nG95;                     // +0x95
	uint8					m_nB96;                     // +0x96
};

struct Light1006a860
{
	Light1006a860*			m_pNext00;                  // +0x00
	union
	{
	LightIface1006a860*		m_pIface04;                 // +0x04
	void*					m_hObj04;
	};
	Vec1006a860				m_vPos08;                   // +0x08
};

struct Desc1006a860
{
	uint16*		m_pData00;                              // +0x00
	uint32		m_nPitch04;                             // +0x04
	uint32		m_n08;                                  // +0x08
	uint32		m_nWidth0c;                             // +0x0c
	uint32		m_nHeight10;                            // +0x10
};

#define GETR_1006a860(c)	((uint8)((c) >> 16))
#define GETG_1006a860(c)	((uint8)((c) >> 8))
#define GETB_1006a860(c)	((uint8)(c))
#define SETR_1006a860(c, v)	((c) = ((c) & 0xff00ffff) | (((uint8)(v) << 16) & 0xff0000))
#define SETG_1006a860(c, v)	((c) = ((c) & 0xffff00ff) | (((uint8)(v) << 8) & 0xff00))
#define SETB_1006a860(c, v)	((c) = ((c) & 0xffffff00) | ((uint8)(v)))

// The multiply table, s_MulTable[i * 256 + j] = i * j / 255, filled once by the
// constructor of a function-local static.
static uint8 s_MulTable[256 * 256];

struct ScaleTable1006a860
{
	ScaleTable1006a860()
	{
		uint32 i, j;

		for (i = 0; i < 256; i++)
		{
			for (j = 0; j < 256; j++)
				s_MulTable[i * 256 + j] = (uint8)((float)j / 255.0f * (float)i);
		}
	}
};


inline uint16 LitPixel1006a860(uint32 color, float fLit)
{
	static ScaleTable1006a860 s_Init;
	uint32 nScale;

	nScale = (uint8)(fLit * 255.0f);
	nScale *= 256;
	return (uint16)(((s_MulTable[nScale + GETR_1006a860(color)] << 7) & 0x7c00) |
	                ((s_MulTable[nScale + GETG_1006a860(color)] << 2) & 0x3e0) |
	                (s_MulTable[nScale + GETB_1006a860(color)] >> 3));
}

// ======================================================================= //
//  0x1006a860  1088 bytes
// ======================================================================= //
extern "C" LTBOOL sub_1006a860(Desc1006a860* pDesc, Surf1006a860* pSurf,
                               Light1006a860* pLight, float fScale)
{
	Plane1006a860*	pPlane;
	float			t;
	float			tSqr;
	uint32			color;
	int32			r, g, b;
	uint32			nWidth, nHeight, nPitch;
	uint16*			pRow;
	uint16*			pLast;
	uint32			x, y;
	float			fStepX, fStepY, fX, fY, fDist;
	uint32			nScale;

	if (!g_varLMDynamic.m_nValue00)
		return LTFALSE;

	pPlane = pSurf->m_p28->m_pPlane34;
	fDist = pPlane->m_Normal.x * pLight->m_vPos08.x +
	        pPlane->m_Normal.y * pLight->m_vPos08.y +
	        pPlane->m_Normal.z * pLight->m_vPos08.z - pPlane->m_Dist;
	t = fDist / pLight->m_pIface04->Slot38(pLight->m_hObj04);

	nWidth = pDesc->m_nWidth0c;
	if (t > 1.0f - 1.0f / (float)nWidth)
		return LTFALSE;

	tSqr = t * t;

	color = 0;

	r = pLight->m_pIface04->m_nR94;
	r = (r > 0x7f) ? r : 0x7f;
	SETR_1006a860(color, r - 0x7f);

	g = pLight->m_pIface04->m_nG95;
	g = (g > 0x7f) ? g : 0x7f;
	SETG_1006a860(color, g - 0x7f);

	b = pLight->m_pIface04->m_nB96;
	b = (b > 0x7f) ? b : 0x7f;
	SETB_1006a860(color, b - 0x7f);

	if (fScale != 1.0f)
	{
		SETR_1006a860(color, GETR_1006a860(color) * fScale);
		SETG_1006a860(color, GETG_1006a860(color) * fScale);
		SETB_1006a860(color, GETB_1006a860(color) * fScale);
	}

	if (!color)
		goto L_Fail;

	nHeight = pDesc->m_nHeight10;
	nPitch = pDesc->m_nPitch04 >> 1;
	pRow = pDesc->m_pData00;
	pLast = pRow + (pDesc->m_nHeight10 - 1) * nPitch;

	for (x = 0; x < nWidth; x++)
	{
		pRow[x] = 0;
		pLast[x] = 0;
	}

	pLast = pRow + nPitch;
	for (y = 1; y < nHeight - 1; y++)
	{
		pLast[0] = 0;
		pLast[nWidth - 1] = 0;
	}

	nHeight -= 2;
	nWidth -= 2;
	fStepY = 2.0f / (float)nHeight;
	fStepX = 2.0f / (float)nWidth;
	fY = -1.0f;
	pRow += nPitch + 1;

	for (y = 0; y < nHeight; y++)
	{
		fX = -1.0f;
		for (x = 0; x < nWidth; x++)
		{
			fDist = fX * fX + fY * fY + tSqr;
			if (!(1.0f > fDist))
			{
				pRow[x] = 0;
			}
			else
			{
				pRow[x] = LitPixel1006a860(color, 1.0f - fDist);
			}

			fX += fStepX;
		}

		fY += fStepY;
		pRow += nPitch;
	}

	return LTTRUE;

L_Fail:
	return LTFALSE;
}
