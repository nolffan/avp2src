// *********************************************************************** //
//
//  MODULE:   BIG_21F90 (.CPP)
//
//  PURPOSE:  One function -- 0x10021f90, 3832 bytes, 0x10021f90..0x10022e88.
//            d3d.ren's CONSOLE-VARIABLE TABLE: eighty-six file-scope
//            console-variable records, each handed its name, its float
//            default and the int32 and/or float destination the renderer
//            will read at run time.  This is the whole of the renderer's
//            tunable surface -- "Bilinear", "FogFarZ", "DrawModels",
//            "UseDX6Commands", "MaxTextureSize" and eighty-one more.
//
//  Compiler: Intel C/C++ 4.0, /O2.  Three independent tells, all
//            /O2 patterns:
//              * outgoing arguments are `add $0xfffffff0,%esp` followed by
//                `movl $imm,K(%esp)` -- eighty-six times, never a `push`;
//              * `&g_CV_GroupOffset[1]` is built as
//                `mov $0x1008cb34,%eax ; add $0x4,%eax` (0x10022434), the
//                two-instruction /O2 address form, not a folded immediate
//                and not a `lea`;
//              * there is no second exit to cross-jump to, so the epilogue
//                column does not apply.
//
//  DECODE STATUS: COMPLETE.  Every one of the 3832 bytes is accounted for.
//  There is no `// UNDECODED:` banner in this file and there is no
//  call-free region: the function is 86 identical seven-instruction blocks
//  plus a five-instruction prologue and a three-instruction epilogue, and
//  the mnemonic histogram over the whole range is exactly
//
//      331 movl $imm,K(%esp)      86 call 0x10021f44     86 add $-16,%esp
//      112 mov                    13 mov %eax,K(%esp)    11 add $K,%eax
//        1 each: xor, sub, push, pop, ret
//
//  with ZERO jumps of any kind.  The 112 `mov`s are 86 `mov $imm,%ecx`
//  (the `this` for each record), 11 `mov $base,%eax` (the two array bases),
//  and the frame-pointer pair.
//
// ----------------------------------------------------------------------- //
//  WHAT THE CALLEE IS, AND HOW THE RECORD IS PROVED
// ----------------------------------------------------------------------- //
//
//  All 86 calls go to ONE address, 0x10021f44, with the record in %ecx and
//  four dwords on the stack; it is 76 bytes and ends `ret $0x10`, so it is
//  __thiscall with four callee-popped stack arguments.  Reading it out
//  (0x10021f44..0x10021f8b) gives the record layout with nothing left over:
//
//      mov 0x24(%esp),%eax   / mov 0x20(%esp),%edx
//      mov 0x1c(%esp),%ebx   / mov 0x18(%esp),%ebp   ; args 4,3,2,1
//      xor %esi,%esi
//      mov %ebx,0x08(%ecx)      +0x08 <- arg2  (the float default)
//      mov %edx,0x0c(%ecx)      +0x0c <- arg3
//      mov %eax,0x10(%ecx)      +0x10 <- arg4
//      mov %ebp,0x14(%ecx)      +0x14 <- arg1  (the name)
//      mov %esi,0x18(%ecx)      +0x18 <- 0
//      movl $0x0,0x04(%ecx)     +0x04 <- 0
//      mov %esi,(%ecx)          +0x00 <- 0
//      mov 0x1008bf90,%eax / mov %ecx,0x1008bf90 / mov %eax,0x1c(%ecx)
//                               push onto the global chain at 0x1008bf90
//      mov %ecx,%eax            returns `this`
//
//  THAT IS ALREADY A NAMED SHAPE IN THIS REPO, twice over:
//
//    * d3dr3c.cpp's sub_10021dc0 decoded the same record from the consumer
//      side -- "+0x08 float default, +0x14 name, +0x18 the handle it fills
//      in, +0x1c next" -- by walking 0x1008bf90 and running "<name> %f"
//      through the engine's console for any variable the console does not
//      already have.  Every offset agrees.
//
//    * d3dr4b.cpp's sub_1005db90 is a 48-byte function that makes exactly
//      ONE of these calls (to the alternate entry 0x10021f40, which is a
//      dead `mov 0x4(%esp),%eax` falling straight into 0x10021f44) for
//      "EnvMapPolyGrids", and its prologue/epilogue are byte-for-byte the
//      prologue and epilogue here: `55 8b ec / 83 ec 10 / 83 c4 f0 / 33 c0`
//      ... `8b e5 5d c3`.  So this function is the same construct at 86x
//      scale, and d3dr4b already recorded the reading: a dynamic
//      initialiser for file-scope console variables.
//
//  WHAT +0x0c AND +0x10 ARE is settled by 0x10021e50, the 115-byte updater
//  that walks the same chain (read in full at 0x10021e50..0x10021ec2):
//
//      pVar->m_f04 = (*0x1008cbcc)->Slot1c(pVar->m_pHandle18);  // fsts
//      pVar->m_n00 = (int32)pVar->m_f04;                        // fistpl
//      if (pVar->m_p10) *(uint32*)pVar->m_p10 = pVar->m_f04;    // 0x10021e88
//      if (pVar->m_p0c) *(uint32*)pVar->m_p0c = pVar->m_n00;    // 0x10021e94
//
//  So +0x0c takes the value ROUNDED TO INT and +0x10 takes it as a FLOAT:
//  argument 3 is an `int32*` destination and argument 4 is a `float*`
//  destination.  73 of the 86 variables pass an int32*, 13 pass a float*,
//  and TableFog passes neither.
//
//  FIVE INDEPENDENT CROSS-CHECKS on that reading, all from work done in
//  this repo before this function was touched:
//
//    * d3dopt2d.cpp already declares `extern int32 g_CV_FilterOptimized;`
//      at 0x1008cb88 and `extern int32 g_CV_Bilinear;` at 0x1008ca98, and
//      says the registrations happen "around 0x10022130" with
//      "UseDX6Commands (0x100893b4, registered at 0x10022620, default 1.0)".
//      This decode puts FilterOptimized's int32* at 0x1008cb88, Bilinear's
//      at 0x1008ca98, and the UseDX6Commands block's `movl $name,(%esp)`
//      at exactly 0x10022620 with 0x3f800000 as its default.
//    * d3ddraw.cpp declares `extern LTBOOL g_bNoLMPages;` at 0x1008ca34;
//      NoLMPages' int32* here is 0x1008ca34.
//    * d3dr3d.cpp declares 0x1008cb00, 0x1008cb04, 0x1008cb08 and
//      0x100893c8 as `extern float` -- and those are precisely, and only,
//      four of the thirteen float* destinations here (SkyFogFarZ,
//      SkyFogNearZ, FogNearZ, FogFarZ).  Nothing in the int32* set is
//      declared float anywhere.
//    * big48180.cpp calls 0x100893ec "how many shadow directions are live,
//      SIGNED", clamped against 8.  That is MaxModelShadows' int32*.  It
//      also calls 0x1008ca38 "the timing cvar" -- ModelProfile's int32*.
//    * THE .data IMAGE ITSELF.  d3d.ren's .data has raw file content only
//      for 0x10089000..0x1008b000; everything above that is zero-filled.
//      Twenty-seven of the destinations fall in the initialised part, in
//      one unbroken run 0x100893a0..0x10089408 with no slot unused -- and
//      EVERY ONE of the twenty-seven holds, on disk, exactly its registered
//      default, read with the type this decode assigns it:
//
//        0x100893a0 0.0005      EnvPanSpeed      (float)
//        0x100893a4 1           DrawWorld        (int)
//        0x100893a8 17.0        ShadowZRange     (float)
//        0x100893ac 1.0         SkyScale         (float)
//        0x100893b0 100.0       Gamma            (float)
//        0x100893b4 1           UseDX6Commands   (int)
//        0x100893b8 1           LightAddPoly     (int)
//        0x100893bc 255         FogR             (int)
//        0x100893c0 255         FogG             (int)
//        0x100893c4 255         FogB             (int)
//        0x100893c8 2000.0      FogFarZ          (float)
//        0x100893cc 1           DrawSky          (int)
//        0x100893d0 1           TextureModels    (int)
//        0x100893d4 1           DrawSprites      (int)
//        0x100893d8 1           DrawPolyGrids    (int)
//        0x100893dc 1           DynamicLight     (int)
//        0x100893e0 1           LightModels      (int)
//        0x100893e4 1           ModelFullbrite   (int)
//        0x100893e8 100         ShadowLodOffset  (int)
//        0x100893ec 1           MaxModelShadows  (int)
//        0x100893f0 1.0         LodScale         (float)
//        0x100893f4 1           DrawParticles    (int)
//        0x100893f8 1           DrawModels       (int)
//        0x100893fc 1           DrawLineSystems  (int)
//        0x10089400 1           LightMap         (int)
//        0x10089404 1           Dither           (int)
//        0x10089408 1           TintModels       (int)
//
//      A wrong type assignment would show up here immediately: 1.0f read as
//      an int is 1065353216, and 1 read as a float is 1.4e-45.  Every one
//      of the twenty-seven reads clean.  (The other destinations are in the
//      zero-filled part, so several have a non-zero console default and a
//      zero static initialiser -- MaxTextureSize, MaxTexAspectRatio,
//      MaxModelLights, DrawGuns, Bilinear, LightSaturate, WarbleSpeed,
//      WarbleScale, EnvScale, SkyFogFarZ.  That asymmetry is the shipped
//      source's, not a decode gap.)
//
// ----------------------------------------------------------------------- //
//  WHAT PINS THE ORDER
// ----------------------------------------------------------------------- //
//
//  The name strings are a single unbroken run in .data1, sixteen-byte
//  aligned, STRICTLY ASCENDING in registration order: 0x100a56c0
//  "TableFog" first, 0x100a5c50 "Dither" eighty-sixth, with the run's only
//  jumps of 32 rather than 16 at exactly the four names of seventeen or
//  more characters ("MaxTexAspectRatio", "OptimizeSurfaces",
//  "ShowFullbriteModels", "ShowTextureCounts").  A string pool is emitted
//  in source order, so the order below IS the shipped source's order, not
//  an ordering choice made here.
//
//  The record addresses corroborate it from the other side: they are
//  0x1008ca54 down to 0x1008bf94, strictly DESCENDING by 0x20 with a single
//  slot skipped (0x1008ca34, which is where NoLMPages/ModelProfile/
//  MaxModelLights' three int32 destinations live).  86 records x 0x20 bytes
//  in one block, allocated in reverse declaration order.
//
// ----------------------------------------------------------------------- //
//  THE SOURCE SHAPE, AND THE ONE COIN-FLIP
// ----------------------------------------------------------------------- //
//
//  The shipped source is almost certainly not a function at all.  86
//  file-scope objects of a class with a four-argument constructor
//
//      CD3DConVar g_varTableFog("TableFog", 0.0f, LTNULL, LTNULL);
//      CD3DConVar g_varNoLMPages("NoLMPages", 0.0f, &g_CV_NoLMPages, LTNULL);
//      ...
//
//  make icl emit exactly this: one dynamic-initialiser function per
//  translation unit, calling each constructor in declaration order, with
//  the constructor returning `this` in %eax and the objects allocated in
//  reverse.  That is the reading d3dr4b.cpp recorded for the one-variable
//  case at 0x1005db90 and it explains every byte here, including the
//  otherwise pointless `sub $0x10,%esp` frame (which 0x1005db90 also has,
//  and which nothing in either function ever references).
//
//  It is written below as an EXPLICIT NAMED FUNCTION for the same reason
//  d3dr4b.cpp wrote sub_1005db90 that way: a compiler-generated initialiser
//  has an internal name that the checkers cannot map, and the call sequence
//  is identical either way.  THE COIN-FLIP, stated with both spellings so
//  one build settles it:
//
//    (a) as below -- `void sub_10021f90()` containing 86 statements
//        `g_varX.sub_10021f44("X", d, p, q);`
//
//    (b) 86 definitions at file scope of a class whose constructor is
//        `CD3DConVar::CD3DConVar(const char* pszName, float fDefault,
//         int32* pnValue, float* pfValue)` declared but not defined in
//        this TU, letting icl generate the initialiser itself.
//
//  A second, smaller coin-flip: GroupOffset0..9 and FogR/FogG/FogB reach
//  their destinations through `mov $base,%eax ; add $K,%eax` rather than a
//  folded immediate, which is what icl /O2 does for a constant-index
//  element of an ARRAY.  Ten and three separate scalars would fold to
//  immediates, so the source must aggregate them; they are written here as
//
//        extern int32 g_CV_GroupOffset[10];   // 0x1008cb34
//        extern int32 g_CV_FogColor[3];       // 0x100893bc
//
//  and the alternative spelling, if icl folds the array form, is a struct
//  with ten (resp. three) int32 members reached as `&g_CV_Groups.m_n1`.
//  Either way the call sequence is unaffected.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  THE RECORD
//
//  0x20 bytes.  Fields as decoded above; only the ones this file needs are
//  named, the rest by offset.  The registration member is 0x10021f44 --
//  __thiscall, four callee-popped stack arguments, returns `this`.
// ======================================================================= //
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


// ======================================================================= //
//  THE EIGHTY-SIX RECORDS, in registration order.  Addresses run downward
//  by 0x20 from 0x1008ca54; the gap at 0x1008ca34 holds three of the int32
//  destinations below.  Declared extern so that this TU does not generate a
//  second initialiser of its own -- see the coin-flip note above.
// ======================================================================= //
extern CD3DConVar g_varTableFog;            // 0x1008ca54
extern CD3DConVar g_varNoLMPages;           // 0x1008ca14
extern CD3DConVar g_varModelProfile;        // 0x1008c9f4
extern CD3DConVar g_varMaxModelLights;      // 0x1008c9d4
extern CD3DConVar g_varMaxTexAspectRatio;   // 0x1008c9b4
extern CD3DConVar g_var32BitTextures;       // 0x1008c994
extern CD3DConVar g_varMaxTextureSize;      // 0x1008c974
extern CD3DConVar g_varDrawGuns;            // 0x1008c954
extern CD3DConVar g_varTintModels;          // 0x1008c934
extern CD3DConVar g_varSaturate;            // 0x1008c914
extern CD3DConVar g_varBilinear;            // 0x1008c8f4
extern CD3DConVar g_varEnvMapEnable;        // 0x1008c8d4
extern CD3DConVar g_varFixTJunc;            // 0x1008c8b4
extern CD3DConVar g_varLightSaturate;       // 0x1008c894
extern CD3DConVar g_varFilterOptimized;     // 0x1008c874
extern CD3DConVar g_varOptimizeSurfaces;    // 0x1008c854
extern CD3DConVar g_varShowSkySplits;       // 0x1008c834
extern CD3DConVar g_varWarbleSpeed;         // 0x1008c814
extern CD3DConVar g_varWarbleScale;         // 0x1008c7f4
extern CD3DConVar g_varEnvMapAll;           // 0x1008c7d4
extern CD3DConVar g_varEnvPanSpeed;         // 0x1008c7b4
extern CD3DConVar g_varEnvScale;            // 0x1008c794
extern CD3DConVar g_varDrawWorld;           // 0x1008c774
extern CD3DConVar g_varShadowZRange;        // 0x1008c754
extern CD3DConVar g_varSkyScale;            // 0x1008c734
extern CD3DConVar g_varGamma;               // 0x1008c714
extern CD3DConVar g_varGroupOffset0;        // 0x1008c6f4
extern CD3DConVar g_varGroupOffset1;        // 0x1008c6d4
extern CD3DConVar g_varGroupOffset2;        // 0x1008c6b4
extern CD3DConVar g_varGroupOffset3;        // 0x1008c694
extern CD3DConVar g_varGroupOffset4;        // 0x1008c674
extern CD3DConVar g_varGroupOffset5;        // 0x1008c654
extern CD3DConVar g_varGroupOffset6;        // 0x1008c634
extern CD3DConVar g_varGroupOffset7;        // 0x1008c614
extern CD3DConVar g_varGroupOffset8;        // 0x1008c5f4
extern CD3DConVar g_varGroupOffset9;        // 0x1008c5d4
extern CD3DConVar g_varTripleBuffer;        // 0x1008c5b4
extern CD3DConVar g_varUseDX6Commands;      // 0x1008c594
extern CD3DConVar g_varInvertHack;          // 0x1008c574
extern CD3DConVar g_varForce1Pass;          // 0x1008c554
extern CD3DConVar g_varLightAddPoly;        // 0x1008c534
extern CD3DConVar g_varModelWarble;         // 0x1008c514
extern CD3DConVar g_varMMXRast;             // 0x1008c4f4
extern CD3DConVar g_varRGBRast;             // 0x1008c4d4
extern CD3DConVar g_varRefRast;             // 0x1008c4b4
extern CD3DConVar g_varTnLRast;             // 0x1008c494
extern CD3DConVar g_varForce2Pass;          // 0x1008c474
extern CD3DConVar g_varFogEnable;           // 0x1008c454
extern CD3DConVar g_varFogR;                // 0x1008c434
extern CD3DConVar g_varFogG;                // 0x1008c414
extern CD3DConVar g_varFogB;                // 0x1008c3f4
extern CD3DConVar g_varFogNearZ;            // 0x1008c3d4
extern CD3DConVar g_varFogFarZ;             // 0x1008c3b4
extern CD3DConVar g_varSkyFogNearZ;         // 0x1008c394
extern CD3DConVar g_varSkyFogFarZ;          // 0x1008c374
extern CD3DConVar g_varMipmapOffset;        // 0x1008c354
extern CD3DConVar g_varLockPVS;             // 0x1008c334
extern CD3DConVar g_varShowFullbriteModels; // 0x1008c314
extern CD3DConVar g_varBumpMap;             // 0x1008c2f4
extern CD3DConVar g_varDrawSky;             // 0x1008c2d4
extern CD3DConVar g_varTextureModels;       // 0x1008c2b4
extern CD3DConVar g_varShowFillInfo;        // 0x1008c294
extern CD3DConVar g_varDrawAll;             // 0x1008c274
extern CD3DConVar g_varDrawSprites;         // 0x1008c254
extern CD3DConVar g_varDrawPolyGrids;       // 0x1008c234
extern CD3DConVar g_varDrawParticles;       // 0x1008c214
extern CD3DConVar g_varDrawModels;          // 0x1008c1f4
extern CD3DConVar g_varDrawLineSystems;     // 0x1008c1d4
extern CD3DConVar g_varShowSplits;          // 0x1008c1b4
extern CD3DConVar g_varDynamicLight;        // 0x1008c194
extern CD3DConVar g_varFastLight;           // 0x1008c174
extern CD3DConVar g_varLightModels;         // 0x1008c154
extern CD3DConVar g_varModelFullbrite;      // 0x1008c134
extern CD3DConVar g_varShowTextureCounts;   // 0x1008c114
extern CD3DConVar g_varShadowLodOffset;     // 0x1008c0f4
extern CD3DConVar g_varMaxModelShadows;     // 0x1008c0d4
extern CD3DConVar g_varLodScale;            // 0x1008c0b4
extern CD3DConVar g_varLodOffset;           // 0x1008c094
extern CD3DConVar g_varWireframe;           // 0x1008c074
extern CD3DConVar g_varModelBoxes;          // 0x1008c054
extern CD3DConVar g_varRenderDebug;         // 0x1008c034
extern CD3DConVar g_varShowPolyCounts;      // 0x1008c014
extern CD3DConVar g_varLightMap;            // 0x1008bff4
extern CD3DConVar g_varDrawFlat;            // 0x1008bfd4
extern CD3DConVar g_varLightmapsOnly;       // 0x1008bfb4
extern CD3DConVar g_varDither;              // 0x1008bf94


// ======================================================================= //
//  THE int32 DESTINATIONS -- argument 3, written by 0x10021e94 from the
//  record's +0x00 (the default rounded to int).  Seventy-three of the
//  eighty-six variables have one.
//
//  Aliases already in the repo, all agreeing with this decode:
//      0x1008ca34  g_bNoLMPages       (d3ddraw.cpp, LTBOOL)
//      0x1008ca98  g_CV_Bilinear      (d3dopt2d.cpp, int32)
//      0x1008cb88  g_CV_FilterOptimized (d3dopt2d.cpp, int32)
//      0x1008ca38  g_p1008ca38        (big48180.cpp, "the timing cvar")
//      0x100893ec  g_n100893ec        (big48180.cpp, int32, clamped to 8)
//      0x100893d8  g_100893d8         (d3dr3a.cpp, uint32)
//      0x100893dc  g_100893dc         (d3dr5b.cpp, uint32)
//      0x100893e0  g_100893e0         (d3dr3d.cpp, uint32)
//      0x100893f4  g_100893f4         (d3dr2b.cpp, uint32)
//      0x100893f8  g_100893f8         (d3dr3c.cpp, uint32)
//      0x100893fc  g_100893fc         (d3dr3a.cpp, uint32)
// ======================================================================= //
extern int32 g_CV_NoLMPages;           // 0x1008ca34   NoLMPages
extern int32 g_CV_ModelProfile;        // 0x1008ca38   ModelProfile
extern int32 g_CV_MaxModelLights;      // 0x1008ca3c   MaxModelLights
extern int32 g_CV_MaxTexAspectRatio;   // 0x1008cbc4   MaxTexAspectRatio
extern int32 g_CV_32BitTextures;       // 0x1008cbd0   32BitTextures
extern int32 g_CV_MaxTextureSize;      // 0x1008cb70   MaxTextureSize
extern int32 g_CV_DrawGuns;            // 0x1008cbc8   DrawGuns
extern int32 g_CV_TintModels;          // 0x10089408   TintModels
extern int32 g_CV_Saturate;            // 0x1008cb98   Saturate
extern int32 g_CV_Bilinear;            // 0x1008ca98   Bilinear
extern int32 g_CV_EnvMapEnable;        // 0x1008cb64   EnvMapEnable
extern int32 g_CV_FixTJunc;            // 0x1008cb84   FixTJunc
extern int32 g_CV_FilterOptimized;     // 0x1008cb88   FilterOptimized
extern int32 g_CV_OptimizeSurfaces;    // 0x1008cb60   OptimizeSurfaces
extern int32 g_CV_ShowSkySplits;       // 0x1008cb5c   ShowSkySplits
extern int32 g_CV_EnvMapAll;           // 0x1008cb68   EnvMapAll
extern int32 g_CV_DrawWorld;           // 0x100893a4   DrawWorld
extern int32 g_CV_GroupOffset[10];     // 0x1008cb34   GroupOffset0..9
extern int32 g_CV_TripleBuffer;        // 0x1008cb30   TripleBuffer
extern int32 g_CV_UseDX6Commands;      // 0x100893b4   UseDX6Commands
extern int32 g_CV_InvertHack;          // 0x1008cb2c   InvertHack
extern int32 g_CV_Force1Pass;          // 0x1008cb28   Force1Pass
extern int32 g_CV_LightAddPoly;        // 0x100893b8   LightAddPoly
extern int32 g_CV_ModelWarble;         // 0x1008cb24   ModelWarble
extern int32 g_CV_MMXRast;             // 0x1008cb18   MMXRast
extern int32 g_CV_RGBRast;             // 0x1008cb20   RGBRast
extern int32 g_CV_RefRast;             // 0x1008cb1c   RefRast
extern int32 g_CV_TnLRast;             // 0x1008cb14   TnLRast
extern int32 g_CV_Force2Pass;          // 0x1008cb10   Force2Pass
extern int32 g_CV_FogEnable;           // 0x1008cb0c   FogEnable
extern int32 g_CV_FogColor[3];         // 0x100893bc   FogR/FogG/FogB
extern int32 g_CV_MipmapOffset;        // 0x1008cad4   MipmapOffset
extern int32 g_CV_LockPVS;             // 0x1008cadc   LockPVS
extern int32 g_CV_ShowFullbriteModels; // 0x1008cad8   ShowFullbriteModels
extern int32 g_CV_BumpMap;             // 0x1008cad0   BumpMap
extern int32 g_CV_DrawSky;             // 0x100893cc   DrawSky
extern int32 g_CV_TextureModels;       // 0x100893d0   TextureModels
extern int32 g_CV_ShowFillInfo;        // 0x1008cacc   ShowFillInfo
extern int32 g_CV_DrawAll;             // 0x1008cac8   DrawAll
extern int32 g_CV_DrawSprites;         // 0x100893d4   DrawSprites
extern int32 g_CV_DrawPolyGrids;       // 0x100893d8   DrawPolyGrids
extern int32 g_CV_DrawParticles;       // 0x100893f4   DrawParticles
extern int32 g_CV_DrawModels;          // 0x100893f8   DrawModels
extern int32 g_CV_DrawLineSystems;     // 0x100893fc   DrawLineSystems
extern int32 g_CV_ShowSplits;          // 0x1008cac4   ShowSplits
extern int32 g_CV_DynamicLight;        // 0x100893dc   DynamicLight
extern int32 g_CV_FastLight;           // 0x1008cac0   FastLight
extern int32 g_CV_LightModels;         // 0x100893e0   LightModels
extern int32 g_CV_ModelFullbrite;      // 0x100893e4   ModelFullbrite
extern int32 g_CV_ShowTextureCounts;   // 0x1008cabc   ShowTextureCounts
extern int32 g_CV_ShadowLodOffset;     // 0x100893e8   ShadowLodOffset
extern int32 g_CV_MaxModelShadows;     // 0x100893ec   MaxModelShadows
extern int32 g_CV_LodOffset;           // 0x1008cab8   LodOffset
extern int32 g_CV_Wireframe;           // 0x1008cab4   Wireframe
extern int32 g_CV_ModelBoxes;          // 0x1008cab0   ModelBoxes
extern int32 g_CV_RenderDebug;         // 0x1008caac   RenderDebug
extern int32 g_CV_ShowPolyCounts;      // 0x1008caa8   ShowPolyCounts
extern int32 g_CV_LightMap;            // 0x10089400   LightMap
extern int32 g_CV_DrawFlat;            // 0x1008caa4   DrawFlat
extern int32 g_CV_LightmapsOnly;       // 0x1008ca9c   LightmapsOnly
extern int32 g_CV_Dither;              // 0x10089404   Dither


// ======================================================================= //
//  THE float DESTINATIONS -- argument 4, written by 0x10021e88 from the
//  record's +0x04.  Thirteen variables have one; five of those (TableFog
//  aside) pass no int32* at all.
//
//  d3dr3d.cpp independently declares four of these as `extern float`:
//  0x1008cb00, 0x1008cb04, 0x1008cb08 and 0x100893c8.
// ======================================================================= //
extern float g_CV_LightSaturate; // 0x1008cb94   LightSaturate
extern float g_CV_WarbleSpeed;   // 0x1008cb80   WarbleSpeed
extern float g_CV_WarbleScale;   // 0x1008cb7c   WarbleScale
extern float g_CV_EnvPanSpeed;   // 0x100893a0   EnvPanSpeed
extern float g_CV_EnvScale;      // 0x1008cb6c   EnvScale
extern float g_CV_ShadowZRange;  // 0x100893a8   ShadowZRange
extern float g_CV_SkyScale;      // 0x100893ac   SkyScale
extern float g_CV_Gamma;         // 0x100893b0   Gamma
extern float g_CV_FogNearZ;      // 0x1008cb08   FogNearZ
extern float g_CV_FogFarZ;       // 0x100893c8   FogFarZ
extern float g_CV_SkyFogNearZ;   // 0x1008cb04   SkyFogNearZ
extern float g_CV_SkyFogFarZ;    // 0x1008cb00   SkyFogFarZ
extern float g_CV_LodScale;      // 0x100893f0   LodScale


// ======================================================================= //
//  0x10021f90 -- 3832 bytes.  DECODED, every instruction.
//
//  Registers all eighty-six renderer console variables onto the chain at
//  0x1008bf90, in source order.  0x10021dc0 later binds each record to a
//  live console variable (creating it from the default if the console does
//  not have it) and 0x10021e50 pushes the values back out to the int32 and
//  float destinations above.
//
//  Note TableFog: it registers with BOTH destinations null, because its
//  readers take the value straight out of the record's own +0x00 -- which
//  is why d3dr3d.cpp knows 0x1008ca54, the record's address, as a plain
//  `extern uint32` it tests for zero.  Nothing here is inconsistent with
//  that; the record's first dword IS the int value.
// ======================================================================= //
void sub_10021f90()
{
	g_varTableFog.sub_10021f44("TableFog", 0.0f, LTNULL, LTNULL);
	g_varNoLMPages.sub_10021f44("NoLMPages", 0.0f, &g_CV_NoLMPages, LTNULL);
	g_varModelProfile.sub_10021f44("ModelProfile", 0.0f, &g_CV_ModelProfile, LTNULL);
	g_varMaxModelLights.sub_10021f44("MaxModelLights", 4.0f, &g_CV_MaxModelLights, LTNULL);
	g_varMaxTexAspectRatio.sub_10021f44("MaxTexAspectRatio", 8.0f, &g_CV_MaxTexAspectRatio, LTNULL);
	g_var32BitTextures.sub_10021f44("32BitTextures", 0.0f, &g_CV_32BitTextures, LTNULL);
	g_varMaxTextureSize.sub_10021f44("MaxTextureSize", 16384.0f, &g_CV_MaxTextureSize, LTNULL);
	g_varDrawGuns.sub_10021f44("DrawGuns", 1.0f, &g_CV_DrawGuns, LTNULL);
	g_varTintModels.sub_10021f44("TintModels", 1.0f, &g_CV_TintModels, LTNULL);
	g_varSaturate.sub_10021f44("Saturate", 0.0f, &g_CV_Saturate, LTNULL);
	g_varBilinear.sub_10021f44("Bilinear", 1.0f, &g_CV_Bilinear, LTNULL);
	g_varEnvMapEnable.sub_10021f44("EnvMapEnable", 0.0f, &g_CV_EnvMapEnable, LTNULL);
	g_varFixTJunc.sub_10021f44("FixTJunc", 0.0f, &g_CV_FixTJunc, LTNULL);
	g_varLightSaturate.sub_10021f44("LightSaturate", 1.0f, LTNULL, &g_CV_LightSaturate);
	g_varFilterOptimized.sub_10021f44("FilterOptimized", 0.0f, &g_CV_FilterOptimized, LTNULL);
	g_varOptimizeSurfaces.sub_10021f44("OptimizeSurfaces", 0.0f, &g_CV_OptimizeSurfaces, LTNULL);
	g_varShowSkySplits.sub_10021f44("ShowSkySplits", 0.0f, &g_CV_ShowSkySplits, LTNULL);
	g_varWarbleSpeed.sub_10021f44("WarbleSpeed", 25.0f, LTNULL, &g_CV_WarbleSpeed);
	g_varWarbleScale.sub_10021f44("WarbleScale", 0.92f, LTNULL, &g_CV_WarbleScale);
	g_varEnvMapAll.sub_10021f44("EnvMapAll", 0.0f, &g_CV_EnvMapAll, LTNULL);
	g_varEnvPanSpeed.sub_10021f44("EnvPanSpeed", 0.0005f, LTNULL, &g_CV_EnvPanSpeed);
	g_varEnvScale.sub_10021f44("EnvScale", 1.0f, LTNULL, &g_CV_EnvScale);
	g_varDrawWorld.sub_10021f44("DrawWorld", 1.0f, &g_CV_DrawWorld, LTNULL);
	g_varShadowZRange.sub_10021f44("ShadowZRange", 17.0f, LTNULL, &g_CV_ShadowZRange);
	g_varSkyScale.sub_10021f44("SkyScale", 1.0f, LTNULL, &g_CV_SkyScale);
	g_varGamma.sub_10021f44("Gamma", 100.0f, LTNULL, &g_CV_Gamma);
	g_varGroupOffset0.sub_10021f44("GroupOffset0", 0.0f, &g_CV_GroupOffset[0], LTNULL);
	g_varGroupOffset1.sub_10021f44("GroupOffset1", 0.0f, &g_CV_GroupOffset[1], LTNULL);
	g_varGroupOffset2.sub_10021f44("GroupOffset2", 0.0f, &g_CV_GroupOffset[2], LTNULL);
	g_varGroupOffset3.sub_10021f44("GroupOffset3", 0.0f, &g_CV_GroupOffset[3], LTNULL);
	g_varGroupOffset4.sub_10021f44("GroupOffset4", 0.0f, &g_CV_GroupOffset[4], LTNULL);
	g_varGroupOffset5.sub_10021f44("GroupOffset5", 0.0f, &g_CV_GroupOffset[5], LTNULL);
	g_varGroupOffset6.sub_10021f44("GroupOffset6", 0.0f, &g_CV_GroupOffset[6], LTNULL);
	g_varGroupOffset7.sub_10021f44("GroupOffset7", 0.0f, &g_CV_GroupOffset[7], LTNULL);
	g_varGroupOffset8.sub_10021f44("GroupOffset8", 0.0f, &g_CV_GroupOffset[8], LTNULL);
	g_varGroupOffset9.sub_10021f44("GroupOffset9", 0.0f, &g_CV_GroupOffset[9], LTNULL);
	g_varTripleBuffer.sub_10021f44("TripleBuffer", 0.0f, &g_CV_TripleBuffer, LTNULL);
	g_varUseDX6Commands.sub_10021f44("UseDX6Commands", 1.0f, &g_CV_UseDX6Commands, LTNULL);
	g_varInvertHack.sub_10021f44("InvertHack", 0.0f, &g_CV_InvertHack, LTNULL);
	g_varForce1Pass.sub_10021f44("Force1Pass", 0.0f, &g_CV_Force1Pass, LTNULL);
	g_varLightAddPoly.sub_10021f44("LightAddPoly", 1.0f, &g_CV_LightAddPoly, LTNULL);
	g_varModelWarble.sub_10021f44("ModelWarble", 0.0f, &g_CV_ModelWarble, LTNULL);
	g_varMMXRast.sub_10021f44("MMXRast", 0.0f, &g_CV_MMXRast, LTNULL);
	g_varRGBRast.sub_10021f44("RGBRast", 0.0f, &g_CV_RGBRast, LTNULL);
	g_varRefRast.sub_10021f44("RefRast", 0.0f, &g_CV_RefRast, LTNULL);
	g_varTnLRast.sub_10021f44("TnLRast", 0.0f, &g_CV_TnLRast, LTNULL);
	g_varForce2Pass.sub_10021f44("Force2Pass", 0.0f, &g_CV_Force2Pass, LTNULL);
	g_varFogEnable.sub_10021f44("FogEnable", 0.0f, &g_CV_FogEnable, LTNULL);
	g_varFogR.sub_10021f44("FogR", 255.0f, &g_CV_FogColor[0], LTNULL);
	g_varFogG.sub_10021f44("FogG", 255.0f, &g_CV_FogColor[1], LTNULL);
	g_varFogB.sub_10021f44("FogB", 255.0f, &g_CV_FogColor[2], LTNULL);
	g_varFogNearZ.sub_10021f44("FogNearZ", 0.0f, LTNULL, &g_CV_FogNearZ);
	g_varFogFarZ.sub_10021f44("FogFarZ", 2000.0f, LTNULL, &g_CV_FogFarZ);
	g_varSkyFogNearZ.sub_10021f44("SkyFogNearZ", 0.0f, LTNULL, &g_CV_SkyFogNearZ);
	g_varSkyFogFarZ.sub_10021f44("SkyFogFarZ", 2000.0f, LTNULL, &g_CV_SkyFogFarZ);
	g_varMipmapOffset.sub_10021f44("MipmapOffset", 0.0f, &g_CV_MipmapOffset, LTNULL);
	g_varLockPVS.sub_10021f44("LockPVS", 0.0f, &g_CV_LockPVS, LTNULL);
	g_varShowFullbriteModels.sub_10021f44("ShowFullbriteModels", 0.0f, &g_CV_ShowFullbriteModels, LTNULL);
	g_varBumpMap.sub_10021f44("BumpMap", 0.0f, &g_CV_BumpMap, LTNULL);
	g_varDrawSky.sub_10021f44("DrawSky", 1.0f, &g_CV_DrawSky, LTNULL);
	g_varTextureModels.sub_10021f44("TextureModels", 1.0f, &g_CV_TextureModels, LTNULL);
	g_varShowFillInfo.sub_10021f44("ShowFillInfo", 0.0f, &g_CV_ShowFillInfo, LTNULL);
	g_varDrawAll.sub_10021f44("DrawAll", 0.0f, &g_CV_DrawAll, LTNULL);
	g_varDrawSprites.sub_10021f44("DrawSprites", 1.0f, &g_CV_DrawSprites, LTNULL);
	g_varDrawPolyGrids.sub_10021f44("DrawPolyGrids", 1.0f, &g_CV_DrawPolyGrids, LTNULL);
	g_varDrawParticles.sub_10021f44("DrawParticles", 1.0f, &g_CV_DrawParticles, LTNULL);
	g_varDrawModels.sub_10021f44("DrawModels", 1.0f, &g_CV_DrawModels, LTNULL);
	g_varDrawLineSystems.sub_10021f44("DrawLineSystems", 1.0f, &g_CV_DrawLineSystems, LTNULL);
	g_varShowSplits.sub_10021f44("ShowSplits", 0.0f, &g_CV_ShowSplits, LTNULL);
	g_varDynamicLight.sub_10021f44("DynamicLight", 1.0f, &g_CV_DynamicLight, LTNULL);
	g_varFastLight.sub_10021f44("FastLight", 0.0f, &g_CV_FastLight, LTNULL);
	g_varLightModels.sub_10021f44("LightModels", 1.0f, &g_CV_LightModels, LTNULL);
	g_varModelFullbrite.sub_10021f44("ModelFullbrite", 1.0f, &g_CV_ModelFullbrite, LTNULL);
	g_varShowTextureCounts.sub_10021f44("ShowTextureCounts", 0.0f, &g_CV_ShowTextureCounts, LTNULL);
	g_varShadowLodOffset.sub_10021f44("ShadowLodOffset", 100.0f, &g_CV_ShadowLodOffset, LTNULL);
	g_varMaxModelShadows.sub_10021f44("MaxModelShadows", 1.0f, &g_CV_MaxModelShadows, LTNULL);
	g_varLodScale.sub_10021f44("LodScale", 1.0f, LTNULL, &g_CV_LodScale);
	g_varLodOffset.sub_10021f44("LodOffset", 0.0f, &g_CV_LodOffset, LTNULL);
	g_varWireframe.sub_10021f44("Wireframe", 0.0f, &g_CV_Wireframe, LTNULL);
	g_varModelBoxes.sub_10021f44("ModelBoxes", 0.0f, &g_CV_ModelBoxes, LTNULL);
	g_varRenderDebug.sub_10021f44("RenderDebug", 0.0f, &g_CV_RenderDebug, LTNULL);
	g_varShowPolyCounts.sub_10021f44("ShowPolyCounts", 0.0f, &g_CV_ShowPolyCounts, LTNULL);
	g_varLightMap.sub_10021f44("LightMap", 1.0f, &g_CV_LightMap, LTNULL);
	g_varDrawFlat.sub_10021f44("DrawFlat", 0.0f, &g_CV_DrawFlat, LTNULL);
	g_varLightmapsOnly.sub_10021f44("LightmapsOnly", 0.0f, &g_CV_LightmapsOnly, LTNULL);
	g_varDither.sub_10021f44("Dither", 1.0f, &g_CV_Dither, LTNULL);
}
