// *********************************************************************** //
//
//  MODULE:   W81_10034ED0_OSG5 (.CPP)  --  the two Optimized2D getters,
//            RenderStruct +0xa4 (0x10034ed0) and +0xac (0x10034ef0), from
//            d3dopt2d.cpp.   /Os /G5.
//
//  WHY A FILE AND A FLAG OF THEIR OWN.  Both shipped bodies copy the
//  out-pointer to %ecx, materialise the `return 1` and only THEN store:
//      mov 0x4(%esp),%eax; mov <global>,%edx; mov %eax,%ecx;
//      xor %eax,%eax; inc %eax; mov %edx,(%ecx); ret
//  which is Pentium (/G5) scheduling.  At icl's default (Pentium Pro) every
//  spelling tried stores first and is 3 bytes off: pointer / reference /
//  enum / struct-member / volatile / local-copy / comma / helper forms,
//  bool and long returns, extern "C", __stdcall, and /Os /O1 /Op /Ox /Qip
//  /Qrcd /QxM /Qxi /QaxM /QxK /Qsfalign8/16 /Ob0 /Qpc32 /Qunroll0 -- none
//  moves the store.  /G5, /G4 and /GB all give the shipped bytes.
//
//  It is NOT a flag for their neighbours: at /Os /G5 d3dopt2d.cpp loses
//  EndOptimized2D and SetOptimized2DBlend, d3ddraw_os.cpp's
//  SetOptimized2DColor (0x10034eb0, directly before these) goes 5 bytes
//  off, and w6_10034f10_osoi.cpp (0x10034f10, directly after) loses
//  0x10035450.  So if the flag is the explanation these two were a
//  translation unit of their own; that is the measured reading, not a
//  settled one.
//
// *********************************************************************** //

#include "ltbasedefs.h"

// 0x1008d9a4 -- the current Optimized2D blend (d3dopt2d.cpp defines it).
extern uint32 g_Optimized2DBlend;


// ---------------------------------------------------------------------- //
//  0x10034ed0 -- RenderStruct +0xa4, 18 bytes. DECODED.
//
//  Eighteen bytes, no frame, `mov 0x4(%esp),%eax` for the one argument:
//  __cdecl, one pointer in, and the answer is the `xor %eax,%eax; inc %eax`
//  LTTRUE. The value handed back comes from 0x1008d9a4, which is where
//  SetOptimized2DBlend parked its argument -- and those two functions are
//  the ONLY references to that global in the whole image, which is what
//  makes the pairing certain.
//
//  DEAD in the shipped engine: ci_SetupInterface never assigns this slot and
//  nothing on the client side references it. It is written out anyway
//  because the renderer still has to fill the slot.
//
//  The scheduling is characteristic and worth recording for whenever the
//  compiler turns up -- the out-pointer is copied to %ecx, the return value
//  is materialised, and only then is the store issued:
//      8b 44 24 04   mov 0x4(%esp),%eax
//      8b 15 ...     mov 0x1008d9a4,%edx
//      8b c8         mov %eax,%ecx
//      33 c0 40      xor %eax,%eax; inc %eax
//      89 11         mov %edx,(%ecx)
//  GetOptimized2DColor below is the same nine instructions against a
//  different global, byte for byte apart from the address.
// ---------------------------------------------------------------------- //
LTBOOL d3d_GetOptimized2DBlend(LTSurfaceBlend* pBlend)
{
	*pBlend = (LTSurfaceBlend)g_Optimized2DBlend;

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x10034ef0 -- RenderStruct +0xac, 18 bytes. DECODED.
//
//  The colour counterpart, and structurally identical to the function above.
//  It reads 0x10089448, the global d3ddraw.cpp's d3d_SetOptimized2DColor
//  (0x10034eb0) writes with the alpha byte forced to 0xff. Four references
//  image-wide: the setter, this getter, and two consumers at 0x10034f86 and
//  0x10035518 -- the 2D drawing path, which is what makes 0x10089448 the
//  colour the batch is modulated by rather than a setting nobody reads.
//
//  Also dead on the client side, for the same reason.
// ---------------------------------------------------------------------- //
extern uint32 g_Optimized2DColor;                       // 0x10089448

LTBOOL d3d_GetOptimized2DColor(HLTCOLOR* pColor)
{
	*pColor = g_Optimized2DColor;

	return LTTRUE;
}
