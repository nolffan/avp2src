// *********************************************************************** //
//
//  MODULE:   SDKFORCE (.CPP)
//
//  PURPOSE:  Emit the ltvector.h inlines that both shipped images carry OUT
//            OF LINE. This driver exists in no shipped binary and is not
//            counted as reconstruction, on packet.cpp's precedent.
//
//  HOW, AND WHY THE OBVIOUS WAY FAILS. commonphysics.cpp records a negative
//  result against exactly this: "_CVector's operator*(float) and operator+=,
//  both genuinely out of line in the image, cannot be reached this way at
//  all: VC6 expands them into whatever driver references them, so there is
//  nothing to compare. They need a real call site."
//
//  That is true of a driver that CALLS them, and it is what made the result
//  look final. It is not true of one that TAKES THEIR ADDRESS. A
//  pointer-to-member needs a real function to point at, so the body must be
//  emitted, and it is emitted as the COMDAT the images carry -- 0x1001e6f0
//  and 0x1001e723, the two addresses that note named, match byte-for-byte.
//
//  The same lever fixed packet.cpp's packet_ForceInstantiation, which had
//  been calling its six templates and silently emitting nothing for either
//  image since it was written.
//
//  A function only needs this treatment if the compiler EXPANDS it at the
//  call site. Where /Os or /Ot declines -- ltmatrix.h's MatTranspose3x3, for
//  instance -- a call is enough and commonphysics.cpp's driver is right as
//  it stands.
//
//  MEASURE AT THE ADDRESS LEVEL, NOT PER OBJECT. commonphysics.cpp's other
//  negative result stands and applies here: a forcing driver once "gained"
//  eleven functions in one object and one across the image, because the
//  other ten were addresses another translation unit already matched.
//
// *********************************************************************** //

#include "ltbasedefs.h"

typedef LTVector (LTVector::*SDKVecBinVec)(const LTVector) const;
typedef LTVector (LTVector::*SDKVecBinScalar)(float) const;
typedef void     (LTVector::*SDKVecAsnVec)(const LTVector);
typedef void     (LTVector::*SDKVecAsnScalar)(float);
typedef LTVector (LTVector::*SDKVecUnary)() const;
typedef float    (LTVector::*SDKVecScalarOut)() const;

// Confirmed out of line in at least one image; the rest are here so the
// driver stays a complete census of the header rather than a list of the
// ones that happened to pay, and cost nothing when the image inlines them.
SDKVecBinVec    g_pSDKVecAddVec    = &LTVector::operator+;
SDKVecBinVec    g_pSDKVecSubVec    = &LTVector::operator-;
SDKVecBinVec    g_pSDKVecMulVec    = &LTVector::operator*;
SDKVecBinVec    g_pSDKVecDivVec    = &LTVector::operator/;
SDKVecBinScalar g_pSDKVecAddScalar = &LTVector::operator+;
SDKVecBinScalar g_pSDKVecSubScalar = &LTVector::operator-;
SDKVecBinScalar g_pSDKVecMulScalar = &LTVector::operator*;
SDKVecBinScalar g_pSDKVecDivScalar = &LTVector::operator/;
SDKVecAsnVec    g_pSDKVecAddAsnVec = &LTVector::operator+=;
SDKVecAsnVec    g_pSDKVecSubAsnVec = &LTVector::operator-=;
SDKVecAsnVec    g_pSDKVecMulAsnVec = &LTVector::operator*=;
SDKVecAsnScalar g_pSDKVecAddAsnScl = &LTVector::operator+=;
SDKVecAsnScalar g_pSDKVecSubAsnScl = &LTVector::operator-=;
SDKVecAsnScalar g_pSDKVecMulAsnScl = &LTVector::operator*=;
SDKVecUnary     g_pSDKVecNegate    = &LTVector::operator-;
SDKVecScalarOut g_pSDKVecMag       = &LTVector::Mag;
SDKVecScalarOut g_pSDKVecMagSqr    = &LTVector::MagSqr;

// lithtech.exe 0x00436790, 27 bytes: the DOT PRODUCT, sitting between Mag
// (0x00436760, which we already match) and Norm (0x004367b0) exactly where
// ltvector.h declares it. The near row that pointed here named LTPlane::DistTo
// and was a pairing artefact -- the shipped body has no `fsubs m_Dist` and our
// DistTo occurs nowhere in either image. Every call site inlines Dot, so only
// a pointer-to-member emits the body. Adds a COMDAT; changes none.
float (LTVector::*g_pSDKVecDot)(LTVector) const = &LTVector::Dot;


// *********************************************************************** //
//  ltmatrix.h. Seventeen of these match in server.dll on the first build,
//  including MatMul (532 bytes), LTMatrix::Inverse (407) and SetupRot (227).
//  GetBasisVectors and SetBasisVectors2 come along for free -- Normalize
//  references them, so forcing Normalize forces those too.
// *********************************************************************** //
// ---- free functions in ltmatrix.h ----
void  (*g_pSDKMat1)(LTMatrix*)                        = Mat_Identity;
void  (*g_pSDKMat2)(LTMatrix&, LTVector&)             = Mat_GetTranslation;
void  (*g_pSDKMat3)(LTMatrix&, LTVector&)             = Mat_SetTranslation;
void  (*g_pSDKMat4)(LTMatrix*)                        = MatTranspose3x3;
void  (*g_pSDKMat5)(LTMatrix*)                        = MatTranspose;
void  (*g_pSDKMat6)(LTMatrix*, LTMatrix*, LTMatrix*)  = MatMul;
void  (*g_pSDKMat7)(LTMatrix*, LTMatrix*)             = MatMul_InPlace;
void  (*g_pSDKMat8)(LTVector*, LTMatrix*, LTVector*)  = MatVMul;
void  (*g_pSDKMat9)(LTMatrix*, LTVector*)             = MatVMul_InPlace;
float (*g_pSDKMat10)(LTVector*, LTMatrix*, LTVector*) = MatVMul_H;
float (*g_pSDKMat11)(LTMatrix*, LTVector*)            = MatVMul_InPlace_H;
void  (*g_pSDKMat12)(float*, LTMatrix*, float*)       = MatVMul_Add;
void  (*g_pSDKMat13)(float*, LTMatrix*, float*)       = MatVMul_Add_3x3;
void  (*g_pSDKMat14)(LTVector*, LTMatrix*, LTVector*) = MatVMul_3x3;
void  (*g_pSDKMat15)(LTMatrix*, LTVector*)            = MatVMul_InPlace_3x3;
void  (*g_pSDKMat16)(LTVector*, LTMatrix*, LTVector*) = MatVMul_Transposed3x3;
void  (*g_pSDKMat17)(LTMatrix*, LTVector*)            = MatVMul_InPlace_Transposed3x3;
void  (*g_pSDKMat18)(LTMatrix*, LTMatrix*)            = Mat_InverseTransformation;
void  (*g_pSDKMat19)(LTMatrix*, LTVector*, float)     = Mat_SetupRot;

// ---- LTMatrix members ----
void     (LTMatrix::*g_pSDKMatM1)()                   = &LTMatrix::Identity;
void     (LTMatrix::*g_pSDKMatM2)()                   = &LTMatrix::Normalize;
LTBOOL   (LTMatrix::*g_pSDKMatM3)()                   = &LTMatrix::Inverse;
LTMatrix (LTMatrix::*g_pSDKMatM4)()                   = &LTMatrix::MakeInverse;
void     (LTMatrix::*g_pSDKMatM5)(float,float,float)  = &LTMatrix::Scale;
void     (LTMatrix::*g_pSDKMatM6)(LTVector&)          = &LTMatrix::SetupScalingMatrix;
void     (LTMatrix::*g_pSDKMatM7)(LTVector,LTVector)  = &LTMatrix::SetupReflectionMatrix;
void     (LTMatrix::*g_pSDKMatM8)(LTVector,float)     = &LTMatrix::SetupRot;
LTVector (LTMatrix::*g_pSDKMatM9)()                   = &LTMatrix::GetScale;
void     (LTMatrix::*g_pSDKMatM10)(LTVector&)         = &LTMatrix::Apply;
void     (LTMatrix::*g_pSDKMatM11)(const LTVector&, LTVector&) = &LTMatrix::Apply;
void     (LTMatrix::*g_pSDKMatM12)(LTVector&)         = &LTMatrix::Apply3x3;
void     (LTMatrix::*g_pSDKMatM13)(const LTVector&, LTVector&) = &LTMatrix::Apply3x3;
void     (LTMatrix::*g_pSDKMatM14)(const LTVector&, LTVector&) = &LTMatrix::Apply4x4;
void     (LTMatrix::*g_pSDKMatM15)(LTMatrix&)         = &LTMatrix::Apply;
LTMatrix (LTMatrix::*g_pSDKMatM16)()                  = &LTMatrix::MakeInverseTransform;
LTMatrix (LTMatrix::*g_pSDKMatM17)()                  = &LTMatrix::operator~;
LTMatrix (LTMatrix::*g_pSDKMatM18)(LTMatrix&)         = &LTMatrix::operator*;
LTVector (LTMatrix::*g_pSDKMatM19)(LTVector)          = &LTMatrix::operator*;


// *********************************************************************** //
//  ltquatbase.h, ltplane.h, ltbbox.h and ltrotation.h.
//
//  Most of these are inlined everywhere in both images and have no
//  out-of-line copy to match -- 23 of the 28 come back unclaimed, which is
//  the expected shape and not a failure. The ones that do pay are
//  LTRotation::operator= (0x10038e1f / 0x0047c460), LTRotation::Conjugate
//  (0x10038e3e), LTRotation::Init (0x00402ea0) and quat_Mul.
//
//  CONSTRUCTORS CANNOT BE REACHED THIS WAY AT ALL -- there is no such thing
//  as a pointer to a constructor, so LTPlane's three, LTRotation's two and
//  anything else spelled as one is out of scope for this lever and needs a
//  real call site. That is a hard limit of the technique, not a gap here.
// *********************************************************************** //
// ---- ltquatbase.h (free functions) ----
void (*g_pSDKPlaneSDKQuat1)(float*)                                = quat_Identity;
void (*g_pSDKPlaneSDKQuat2)(float*, float, float, float, float)    = quat_Set;
void (*g_pSDKPlaneSDKQuat3)(float*, const float*, const float*)    = quat_Mul;
void (*g_pSDKPlaneSDKQuat4)(float*, const float*)                  = quat_Conjugate;

// ---- ltplane.h ----
void      (LTPlane::*g_pSDKPlane1)(LTVector, float) = &LTPlane::Init;
float     (LTPlane::*g_pSDKPlane2)(LTVector)        = &LTPlane::DistTo;
LTPlane   (LTPlane::*g_pSDKPlane3)()                = &LTPlane::operator-;
LTVector& (LTPlane::*g_pSDKPlane4)()                = &LTPlane::Normal;
float&    (LTPlane::*g_pSDKPlane5)()                = &LTPlane::Dist;

// ---- ltbbox.h ----
LTBOOL   (LTBBox::*g_pSDKBBox1)(LTVector) = &LTBBox::PtInsideBox;
LTBOOL   (LTBBox::*g_pSDKBBox2)(LTVector) = &LTBBox::PtTouchingBox;
LTBOOL   (LTBBox::*g_pSDKBBox3)(LTBBox)   = &LTBBox::IntersectsBox;
LTBOOL   (LTBBox::*g_pSDKBBox4)(LTBBox)   = &LTBBox::TouchesBox;
LTVector (LTBBox::*g_pSDKBBox5)()         = &LTBBox::GetCenter;
LTVector (LTBBox::*g_pSDKBBox6)()         = &LTBBox::GetDims;

// ---- ltrotation.h ----
LTRotation  (LTRotation::*g_pSDKRot1)(const LTRotation&) const = &LTRotation::operator*;
float&      (LTRotation::*g_pSDKRot2)(uint32)                  = &LTRotation::operator[];
LTRotation& (LTRotation::*g_pSDKRot3)(const LTRotation&)       = &LTRotation::operator=;
LTRotation& (LTRotation::*g_pSDKRot4)(const LTMatrix&)         = &LTRotation::operator=;
LTBOOL      (LTRotation::*g_pSDKRot5)(const LTRotation&) const = &LTRotation::operator==;
LTRotation  (LTRotation::*g_pSDKRot6)() const                  = &LTRotation::operator~;
void        (LTRotation::*g_pSDKRot7)(float,float,float,float) = &LTRotation::Init;
void        (LTRotation::*g_pSDKRot8)()                        = &LTRotation::Identity;
LTBOOL      (LTRotation::*g_pSDKRot9)() const                  = &LTRotation::IsIdentity;
void        (LTRotation::*g_pSDKRot10)(LTMatrix&) const        = &LTRotation::ConvertToMatrix;
void        (LTRotation::*g_pSDKRot11)(const LTMatrix&)        = &LTRotation::ConvertFromMatrix;
void        (LTRotation::*g_pSDKRot12)(const LTRotation&, const LTRotation&, float) = &LTRotation::Slerp;
LTRotation  (LTRotation::*g_pSDKRot13)() const                 = &LTRotation::Conjugate;


// *********************************************************************** //
//  ltlink.h. One payer out of twenty: CheapLTLink::AddAfter (0x100270be).
//  The rest are two- and three-instruction bodies that every image inlines
//  everywhere, which is what the "too-small-to-match" bucket is full of.
// *********************************************************************** //
#include "ltlink.h"

LTLink*  (LTLink::*g_pSDKLink1)()            = &LTLink::AsDLink;
LTLink*  (LTLink::*g_pSDKLink2)()            = &LTLink::AsLTLink;
void     (LTLink::*g_pSDKLink3)(void*)       = &LTLink::Init2;

void     (CheapLTLink::*g_pSDKCheap1)()                 = &CheapLTLink::Init;
void     (CheapLTLink::*g_pSDKCheap2)()                 = &CheapLTLink::Term;
void     (CheapLTLink::*g_pSDKCheap3)()                 = &CheapLTLink::Remove;
void     (CheapLTLink::*g_pSDKCheap4)()                 = &CheapLTLink::TieOff;
void     (CheapLTLink::*g_pSDKCheap5)(CheapLTLink*)     = &CheapLTLink::AddAfter;
void     (CheapLTLink::*g_pSDKCheap6)(CheapLTLink*)     = &CheapLTLink::AddBefore;
LTLink*  (CheapLTLink::*g_pSDKCheap7)(unsigned long)    = &CheapLTLink::operator[];
LTBOOL   (CheapLTLink::*g_pSDKCheap8)()                 = &CheapLTLink::IsTiedOff;

void (*g_pSDKDl1)(CheapLTLink*)                       = dl_TieOff;
void (*g_pSDKDl2)(CheapLTLink*, CheapLTLink*)         = dl_Insert;
void (*g_pSDKDl3)(CheapLTLink*)                       = dl_Remove;
void (*g_pSDKDl4)(LTList*)                            = dl_InitList;
void (*g_pSDKDl5)(LTList*, LTLink*, LTLink*, void*)   = dl_AddAfter;
void (*g_pSDKDl6)(LTList*, LTLink*, LTLink*, void*)   = dl_AddBefore;
void (*g_pSDKDl7)(LTList*, LTLink*, void*)            = dl_AddHead;
void (*g_pSDKDl8)(LTList*, LTLink*, void*)            = dl_AddTail;
void (*g_pSDKDl9)(LTList*, LTLink*)                   = dl_RemoveAt;
