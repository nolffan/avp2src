// *********************************************************************** //
//
//  MODULE:   W31_1001E360 (.CPP)  --  /O2
//
//  0x1001e360, XForm66f08::sub_1001e360 -- moved out of big66f08.cpp
//  When that file became w31_10066d10_qipni.cpp (/O2 /Qip
//  /Qip_no_inlining).  Under /Qip an external function with callers in its
//  own TU is emitted as a two-entry pair (stack shim + register body), and
//  the shipped 0x1001e360 is a plain thiscall with no shim -- it is nowhere
//  near 0x10066f00 in the image, so it was never in that TU.  Here, alone
//  and /O2, it stays byte-identical, as it was in big66f08.cpp.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct LTMat
{
	float  m[4][4];
};

struct LTVec
{
	float  x, y, z;
};


// ======================================================================= //
//  THE TRANSFORM OBJECT -- the first parameter of both functions, and the
//  0x4f8-byte global at 0x1008b880 that big26728.cpp sizes and that
//  d3dr2a.cpp hands to 0x1007b8e0 as the callback context.  Only the three
//  4x4s and the two points are touched here.
//
//  0x1001e360 is XForm66f08::sub_1001e360, defined below, __thiscall,
//  `ret $0xc`: it stores its three arguments to +0x4e8, +0x4e4, +0x4ec and
//  then recomputes +0x4f0 and +0x4f4 from the fog globals.  Both call sites
//  here open TWELVE bytes and copy a three-float local into them field by
//  field, so the argument is one point by value, not three scalars.
// ======================================================================= //
class XForm66f08
{
public:
	void   sub_1001e360(LTVec vPos);                     // 0x1001e360

	uint8  m_pad_000[0x15c];                             // +0x000
	LTMat  m_Mat15c;                                     // +0x15c
	LTMat  m_Mat19c;                                     // +0x19c
	LTMat  m_Mat1dc;                                     // +0x1dc
	uint8  m_pad_21c[0x4b4 - 0x21c];                     // +0x21c
	LTVec  m_v4b4;                                       // +0x4b4
	uint8  m_pad_4c0[0x4e4 - 0x4c0];                     // +0x4c0
	LTVec  m_v4e4;                                       // +0x4e4
	float  m_f4f0;                                       // +0x4f0
	uint32 m_n4f4;                                       // +0x4f4
};


// ======================================================================= //
//  0x1001e360   173 bytes (+3 of `8d 76 00`)   5 callers
//
//  MOVED TO big66f08.cpp FROM d3dbatcha.cpp BECAUSE IT IS AN /O2 FUNCTION
//  (and from there to this file, see the banner).  The body
//  below is byte-identical at /O2 and one instruction out at /Os, which is
//  d3dbatcha.cpp's flag: the constant 1 stored to +0x4f4 is `mov $0x1,%eax`
//  in the shipped bytes, and /Os spells it `xor %eax,%eax; inc %eax`.  This
//  file is /O2, and it is also the one whose two callbacks call this member
//  with the vector by value (see XForm66f08 above), so it lands on its own
//  class.
//
//  What pinned it, beyond the flag:
//    * the parameter is ONE LTVec by value (the callers copy a three-float
//      local field by field into twelve bytes of outgoing area), stored
//      field by field x, y, z.  icl loads x and y first and stores y before
//      x; a struct assignment is a `rep movsl` instead.
//    * every comparison loads the fog-distance global A = 0x1008bd38 FIRST
//      (`flds A` then the other operand), so A is the LEFT operand of each
//      test: A <= B, A >= C, A >= C, A <= B.
//    * the second chain's `ja` jumps to the 2 and falls into the 0, so the
//      0 arm is written before the 2 arm.
//  Seven globals, all BSS, read fresh each time: A = 0x1008bd38,
//  B = 0x1008d708, C = 0x1008d6e8.
// ======================================================================= //

// 0x1008bd38 is the +4 float of the 12-byte vector at 0x1008bd34 (the eye
// point other files read whole), so it is addressed inside that one object.
struct LTVec;
extern LTVec g_v1008bd34;                               // 0x1008bd34
#define g_f1008bd38 (((float*)&g_v1008bd34)[1])        // A
extern float g_f1008d708;                               // B
extern float g_f1008d6e8;                               // C
extern float g_f1008d6c8;
extern float g_f1008d6a8;
extern float g_f1008d5dc;
extern float g_f1008d5e0;

void XForm66f08::sub_1001e360(LTVec vPos)
{
	float  fValue;
	uint32 nMode;

	m_v4e4.x = vPos.x;
	m_v4e4.y = vPos.y;
	m_v4e4.z = vPos.z;

	if (g_f1008bd38 <= g_f1008d708)
		fValue = g_f1008d6c8;
	else if (g_f1008bd38 >= g_f1008d6e8)
		fValue = g_f1008d6a8;
	else
		fValue = (g_f1008bd38 - g_f1008d708) * g_f1008d5dc * g_f1008d5e0 +
		         g_f1008d6c8;

	m_f4f0 = fValue;

	if (g_f1008bd38 >= g_f1008d6e8)
		nMode = 1;
	else if (g_f1008bd38 <= g_f1008d708)
		nMode = 0;
	else
		nMode = 2;

	m_n4f4 = nMode;
}


