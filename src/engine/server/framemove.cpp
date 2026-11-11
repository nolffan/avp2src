// server.dll — the frame-move worker (reconstruction). Build /O1.
//
// Split from commonphysics.cpp: phys_ComputeFrameMove is a translation unit
// of its own in both Primal Hunt images, between the model code's and the
// world-move cluster's. The cluster's unit opens with g_AxisDirs'
// initialisers (server.dll 0x100273d1, lithtech.exe 0x00462060), and this
// one ends, as sisegment.cpp's does, in <string>'s guarded initializer of
// std::ctype<unsigned short>::id (server.dll 0x100273b0: guard 0x1008ec84,
// atexit of the empty 0x1002eaeb; lithtech.exe 0x00462030). The file name
// is ours.
#if defined(PRIMAL_HUNT)
#include <string>
#endif
#include <math.h>
#include "commonphysics.h"
#include "servermgr.h"

// 0x1001e088 — the frame-move worker (declared in commonphysics.h; the
// header's "not reconstructed" note predates this body). Integrates the
// velocity and acceleration the record points at over m_fFrameTime into
// m_vFrameMove: small values are snapped to zero, an object standing on
// something has its acceleration projected onto the ground plane (the
// poly's plane, or -m_Dir with no poly) and, on shallow ground, friction
// applied analytically through exp(); a non-gravity object at rest is
// stopped outright (velocity/acceleration zeroed, CHANGEFLAG_MOTION cleared,
// return 0). Every constant is .rdata: 0.1 (rest), 0.01, 1.0, 0.5.
//
// NEAR: 1616 vs 1615 bytes, 629/629 instructions, and the 23 calls (which
// MagSqr/Norm/Mag/ctor/operator sites are calls vs inline) are the shipped
// sequence exactly. What reached that, measured:
//   * the A1 projection's dot is written out (no Dot() call) and the
//     friction decay is `vNew *= fDecay; vNewVel = vTerm + vNew;` -- both
//     move VC6's /O1 inline budget; every other spelling tried either
//     inlined the fDot-branch constructor or left one Stop Init() a call.
// What is left (14 mnemonic blocks, raw diff ~166 lines):
//   * frame slots: shipped keeps vAccel, vTerm AND the second friction
//     vector all at -0x1c (one variable reused). Merging them here
//     (vTerm -> vAccel) fixes most slots but costs one inline unit (Stop's
//     second Init becomes a call); no compensating spelling was found
//     (~400 combinations of VEC_SET/VEC_DOT/scope/goto variants searched).
//   * the standing-at-rest test: shipped `jae; jmp Stop` (a cross-jumped
//     duplicate of the Stop block) vs our `jb Stop` from the goto; the
//     duplicated block needs one more inline unit too.
//   * operand order in exp(-(t*fric)) and a few copy schedulings.
// lithtech.exe (0x004500d0) is a further way off: our SDK Norm() is inline
// and /O2 expands it, which the engine's declaration-only Norm never was.
int phys_ComputeFrameMove(GlobalForceControl* pControl)
{
	LTObject* pObj;
	LTVector* pOffset;
	LTVector* pNormal;
	float     fHalfTimeSqr;
	float     fVelMag;
	float     fAccelMag;
	float     fMag;
	float     fDecay;
	LTVector  vAccel;
	LTVector  vDir;
	LTVector  vUp;
	LTVector  vNew;
	LTVector  vTerm;
	LTVector  vNewVel;
	LTVector  vDelta;

	fHalfTimeSqr = pControl->m_fFrameTime * pControl->m_fFrameTime * 0.5f;
	pObj = pControl->m_pObj;

	fVelMag   = pControl->m_pVelocity->MagSqr();
	fAccelMag = pControl->m_pAcceleration->MagSqr();

	if (fVelMag < 0.1f)
	{
		pControl->m_pVelocity->Init();
		fVelMag = 0.0f;
	}

	if (fAccelMag < 0.1f)
	{
		pControl->m_pAcceleration->Init();
		fAccelMag = 0.0f;
	}

	pOffset = &pControl->m_vFrameMove;
	pOffset->Init();

	if (pControl->m_nObjFlags & FLAG_GRAVITY)
	{
		if (pObj->m_pStandingOn)
		{
			if (fVelMag < 0.1f && fAccelMag < 0.1f)
				goto Stop;

			if (pObj->m_pStandingOnPoly)
			{
				pNormal = &pObj->m_pStandingOnPoly->GetPlane()->m_Normal;
			}
			else
			{
				pNormal = &vUp;
				vUp = -pControl->m_Dir;
			}

			vAccel = *pControl->m_pAcceleration;
			vAccel += pControl->m_Force;

			vDir = pControl->m_Force;
			vDir.Norm();

			if (vDir.Dot(*pNormal) > pControl->m_Unknown30)
			{
				fMag = vAccel.Mag();

				*pControl->m_pAcceleration = vAccel - *pNormal * (vAccel.z * pNormal->z + (vAccel.y * pNormal->y + vAccel.x * pNormal->x));

				float fDot = pControl->m_pAcceleration->Dot(vDir);
				if (fDot < 0.0f)
					*pControl->m_pAcceleration -= vDir * fDot;

				pControl->m_pAcceleration->Norm(fMag);
			}
			else
			{
				vNew = *pControl->m_pVelocity + pControl->m_Force * pControl->m_fFrameTime;

				if (vNew.Dot(*pNormal) > 0.01f)
				{
					*pControl->m_pAcceleration = vAccel;
				}
				else
				{
					if (pControl->m_pAcceleration->Dot(*pNormal) > 0.01f)
					{
						fMag = pControl->m_pAcceleration->Mag();

						*pControl->m_pAcceleration -= *pNormal *
							(pNormal->Dot(*pControl->m_pAcceleration) + 1.0f);

						pControl->m_pAcceleration->Norm(fMag);
					}

					if (pObj->m_FrictionCoefficient > 0.0f)
					{
						fDecay = (float)exp(-(pObj->m_FrictionCoefficient * pControl->m_fFrameTime));

						vTerm = *pControl->m_pAcceleration / pObj->m_FrictionCoefficient;
						vNew = *pControl->m_pVelocity - vTerm;
						vNew *= fDecay;
						vNewVel = vTerm + vNew;

						*pOffset = vTerm * pControl->m_fFrameTime;

						vTerm = *pControl->m_pVelocity * pObj->m_FrictionCoefficient;
						vTerm -= *pControl->m_pAcceleration;
						vTerm *= (1.0f - fDecay) / pObj->m_FrictionCoefficient / pObj->m_FrictionCoefficient;
						*pOffset += vTerm;

						pObj->m_Velocity = vNewVel;
						return 1;
					}
				}
			}
		}
		else
		{
			*pControl->m_pAcceleration += pControl->m_Force;
		}
	}
	else if (fVelMag < 0.1f && fAccelMag < 0.1f)
	{
Stop:
		pObj->m_Velocity.Init();
		pObj->m_Acceleration.Init();
		pObj->m_ChangeFlags &= ~CHANGEFLAG_MOTION;
		return 0;
	}

	vDelta = *pControl->m_pAcceleration * pControl->m_fFrameTime;
	*pOffset = *pControl->m_pVelocity * pControl->m_fFrameTime;
	*pOffset += *pControl->m_pAcceleration * (fHalfTimeSqr * 0.5f);

	pObj->m_Velocity += vDelta;
	*pControl->m_pVelocity = pObj->m_Velocity;

	return 1;
}
