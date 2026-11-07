// ----------------------------------------------------------------------- //
//
// MODULE  : AIStubs_n2.cpp
//
// PURPOSE : TEMPORARY link stand-ins, so that n2's worktree links and
//           scores: the leash manager (n3's LeashMgr.cpp), CAIMemory::Clear
//           (0x1014f570) and CTentacle (n4's). Remove this file and its
//           Object_PH.dsp entry when the real code is merged.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"

HOBJECT SpawnTurret(HOBJECT, const LTVector&, const LTRotation&) { return LTNULL; }	// turret.cpp 0x10091e40

#endif // PRIMAL_HUNT
