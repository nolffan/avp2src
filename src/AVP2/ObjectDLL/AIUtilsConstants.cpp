// ----------------------------------------------------------------------- //
//
// MODULE  : AIUtilsConstants.cpp
//
// PURPOSE : AIUtils.h's sines, defined once
//
// ----------------------------------------------------------------------- //

// PH: Primal Hunt's file (the name is ours: retail does not record it). It
// is linked after SimpleNodeMgr.cpp, the last of the AI sources: its
// initialisers are 0x1021a530..0x1021a66a, just after SimpleNodeMgr's
// code, its cosines (the values they copy) at 0x1028f710..0x1028f734, just
// after SimpleNodeMgr's constant, and the sines at 0x10344950..0x10344977,
// just after SimpleNodeMgr's statics. AvP2 defines them in AIUtils.h, so
// every file that includes it initialises a copy of its own.

#include "stdafx.h"
#include "AIUtils.h"

const LTFLOAT c_fSin80	= c_fCos10;
const LTFLOAT c_fSin70	= c_fCos20;
const LTFLOAT c_fSin60	= c_fCos30;
const LTFLOAT c_fSin45	= c_fCos45;
const LTFLOAT c_fSin35	= c_fCos55;
const LTFLOAT c_fSin30	= c_fCos60;
const LTFLOAT c_fSin15	= c_fCos75;
const LTFLOAT c_fSin10	= c_fCos80;
const LTFLOAT c_fSin5	= c_fCos85;
const LTFLOAT c_fSin2	= c_fCos88;
