// ----------------------------------------------------------------------- //
//
// MODULE  : AIStubs_n1.cpp
//
// PURPOSE : TEMPORARY link stand-ins for the new Primal Hunt code that
//           CAIBase and CAIBull call but whose retail code lies in the
//           other ranges of 0x10141000..0x1015c000 (CAIAnimator, CMoveMgr,
//           COrientationMgr, CLeash, CAISounds, CAvoidance, CAIMemory,
//           CSpearList, CAISight, CCommandParser, ...).
//
//           Remove this file (and its Object_PH.dsp entry) when the real
//           implementations are merged.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"

LTBOOL g_bGodMode = LTFALSE;

int g_nN1Stub;
void N1StubHit(int n) { g_nN1Stub = n; }

LTBOOL CAISight::CanSee(HOBJECT, const LTVector&, const LTVector&, int, uint32, uint32, uint32, LTFLOAT*) { N1StubHit(1); return LTFALSE; }
void CAISight::Save(HMESSAGEWRITE) { N1StubHit(2);  }
void CAISight::Load(HMESSAGEREAD) { N1StubHit(3);  }

void CAIAnimator::Save(HMESSAGEWRITE) {}
void CAIAnimator::Load(HMESSAGEREAD) {}



CLeash::~CLeash() {}


void CAvoidance::Update() { N1StubHit(45);  }
LTBOOL CAvoidance::CheckBlocked(HOBJECT, CMoveMgr*) { N1StubHit(46); return LTFALSE; }



CCommandParser::CCommandParser() { N1StubHit(64); m_nArgs = 0; }
void CCommandParser::Parse(const char*, const char*) { N1StubHit(65);  }

int AIDebugPoint(int, const LTVector&, const AIDebugColor&, LTFLOAT) { N1StubHit(67); return 0; }


#endif // PRIMAL_HUNT
