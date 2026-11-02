// *********************************************************************** //
//
//  w8_1005d650.cpp -- 0x1005d650, trivial accept / reject of a triangle
//  against the view frustum.  /O2 (an epilogue per exit, lea-nop padding).
//
//  __cdecl (nClipMask, pOutMask, pV0, pV1, pV2), plain `ret`; the caller
//  is big5a960.cpp's patch-grid draw (0x1005b38d).
//
//  WHAT IT DOES.  For each plane whose bit is set in nClipMask, in the
//  order d3dr3c.cpp's clipper driver applies them -- near 0x01, left 0x04,
//  top 0x08, right 0x10, bottom 0x20, far 0x02 -- count the vertices on the
//  inside (each test materialised as 0/1 and summed):
//      near    z >= g_1008b908        left    -z < x
//      top     y < z                  right   x < z
//      bottom  -z < y                 far     z <= g_1008b910
//  none inside: answer 1 (reject).  All three: go on to the next plane.
//  Otherwise answer 2 (clip) and hand back the mask of the planes still to
//  test, this one included: 0x3f, 0x3e, 0x3a, 0x32, 0x22, 0x02 of the
//  caller's mask.  Inside every plane: answer 0.
// *********************************************************************** //

#include "ltbasedefs.h"

struct Vert1005d650
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

extern float g_1008b908;                                // 0x1008b908 near z
extern float g_1008b910;                                // 0x1008b910 far z

#define CLIP_NEAR(v)	((v)->z >= g_1008b908)
#define CLIP_LEFT(v)	(-(v)->z < (v)->x)
#define CLIP_TOP(v)		((v)->y < (v)->z)
#define CLIP_RIGHT(v)	((v)->x < (v)->z)
#define CLIP_BOTTOM(v)	(-(v)->z < (v)->y)
#define CLIP_FAR(v)		((v)->z <= g_1008b910)

#define CLIP_TEST_1005d650(bit, test, remaining)                         \
	if (nClipMask & (bit))                                               \
	{                                                                    \
		nInside = test(pV0) + test(pV1) + test(pV2);                     \
		if (nInside == 0)                                                \
			return 1;                                                    \
		if (nInside != 3)                                                \
		{                                                                \
			*pOutMask = nClipMask & (remaining);                         \
			return 2;                                                    \
		}                                                                \
	}


// ======================================================================= //
//  0x1005d650  780 bytes
// ======================================================================= //
extern "C" int32 sub_1005d650(uint32 nClipMask, uint32* pOutMask,
                              Vert1005d650* pV0, Vert1005d650* pV1,
                              Vert1005d650* pV2)
{
	int32 nInside;

	CLIP_TEST_1005d650(0x01, CLIP_NEAR,   0x3f)
	CLIP_TEST_1005d650(0x04, CLIP_LEFT,   0x3e)
	CLIP_TEST_1005d650(0x08, CLIP_TOP,    0x3a)
	CLIP_TEST_1005d650(0x10, CLIP_RIGHT,  0x32)
	CLIP_TEST_1005d650(0x20, CLIP_BOTTOM, 0x22)
	CLIP_TEST_1005d650(0x02, CLIP_FAR,    0x02)

	return 0;
}
