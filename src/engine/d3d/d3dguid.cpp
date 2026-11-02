// *********************************************************************** //
//
//  MODULE:   D3DGUID (.CPP)
//
//  PURPOSE:  The DirectX 7 interface and device GUIDs d3d.ren references.
//
//  The shipped image carries them in .rdata straight after the import
//  address table, sixteen bytes apiece, contiguous, at 0x10088128 ..
//  0x100881a7; the values below are those bytes. They are the public DX7
//  SDK constants (dxguid.lib / DEFINE_GUID under INITGUID), but this
//  project's toolchain has only VC6's DirectX 5/6-era dxguid.lib, which
//  defines none of the DX7 names, so they are defined here with C linkage
//  -- the spelling the SDK gives them (`_IID_IDirect3D7`) -- and every
//  user declares them `extern "C"`.
//
//  0x100881a8 holds a ninth, {9C59509A-39BD-11D1-8C4A-00C04FD930C5}
//  (IID_IDirectDraw4), which GetSupportedModes (w26_10020fd0_osoiqip.cpp)
//  queries for.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct D3DGuid
{
	uint32 m_Data1;
	uint16 m_Data2;
	uint16 m_Data3;
	uint8  m_Data4[8];
};

extern "C" const D3DGuid IID_IDirect3DTnLHalDevice    = { 0xF5049E78, 0x4861, 0x11D2, { 0xA4, 0x07, 0x00, 0xA0, 0xC9, 0x06, 0x29, 0xA8 } };   // 0x10088128
extern "C" const D3DGuid IID_IDirect3DRefDevice       = { 0x50936643, 0x13E9, 0x11D1, { 0x89, 0xAA, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x29 } };   // 0x10088138
extern "C" const D3DGuid IID_IDirect3DMMXDevice       = { 0x881949A1, 0xD6F3, 0x11D0, { 0x89, 0xAB, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x29 } };   // 0x10088148
extern "C" const D3DGuid IID_IDirect3DHALDevice       = { 0x84E63DE0, 0x46AA, 0x11CF, { 0x81, 0x6F, 0x00, 0x00, 0xC0, 0x20, 0x15, 0x6E } };   // 0x10088158
extern "C" const D3DGuid IID_IDirect3DRGBDevice       = { 0xA4665C60, 0x2673, 0x11CF, { 0xA3, 0x1A, 0x00, 0xAA, 0x00, 0xB9, 0x33, 0x56 } };   // 0x10088168
extern "C" const D3DGuid IID_IDirect3D7               = { 0xF5049E77, 0x4861, 0x11D2, { 0xA4, 0x07, 0x00, 0xA0, 0xC9, 0x06, 0x29, 0xA8 } };   // 0x10088178
extern "C" const D3DGuid IID_IDirectDrawGammaControl  = { 0x69C11C3E, 0xB46B, 0x11D1, { 0xAD, 0x7A, 0x00, 0xC0, 0x4F, 0xC2, 0x9B, 0x4E } };   // 0x10088188
extern "C" const D3DGuid IID_IDirectDraw7             = { 0x15E65EC0, 0x3B9C, 0x11D2, { 0xB9, 0x2F, 0x00, 0x60, 0x97, 0x97, 0xEA, 0x5B } };   // 0x10088198
extern "C" const D3DGuid IID_IDirectDraw4             = { 0x9C59509A, 0x39BD, 0x11D1, { 0x8C, 0x4A, 0x00, 0xC0, 0x4F, 0xD9, 0x30, 0xC5 } };   // 0x100881a8
