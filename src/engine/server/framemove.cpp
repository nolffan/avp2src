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

// The frame-move worker (declared in commonphysics.h): Primal Hunt server.dll
// 0x10026c87 (1798 bytes), lithtech.exe 0x00461650 (2518); AvP2 1.0.9.6
// server.dll 0x100252f3 (1841), lithtech.exe 0x0045c600 (2551). One source:
// the AvP2 bodies differ only by cl 9044's codegen.
//
// Integrates the velocity and acceleration the record points at over
// m_fFrameTime into m_vFrameMove. An object moving along the BSP's edges
// (FLAG2_ORIENTMOVEMENT) is integrated directly. Otherwise small values are
// snapped to zero, an object standing on something has its acceleration
// projected onto the ground plane (the poly's plane, or -m_Dir with no poly)
// and, on shallow ground, friction applied analytically through exp(); an
// object at rest is stopped outright (velocity/acceleration zeroed,
// CHANGEFLAG_MOTION cleared, return 0).
//
// Source facts the bytes settle:
//   * the orient-movement block is the integration tail written out again
//     at the top (0x10026cbd): the same five statements, expanded where the
//     tail's are calls (the /O1 inline budget runs in source order).
//   * m_Force and m_Dir are copied to locals before the snaps (-0x34, -0x58),
//     and the ground normal is -vDir, the copy, not the member.
//   * the at-rest test inside the standing block has its own copy of the
//     stop (`jae; jmp` to the cross-jumped block at 0x100272c7); a goto to
//     the outer one gives a single `jb`.
//   * the dot products with *pNormal are Dot() (the by-value copy of
//     *pNormal is in the code); the projection's own dot is VEC_DOT, and
//     the dot against the acceleration is VEC_DOT(*accel, dir).
//   * vForce/vDir and the tail's vDelta are assigned, not initialised; the
//     friction's vStart likewise (the copy at 0x10027178 is the assignment).
//
// NEAR (server.dll 1808 vs 1798; 59 of 681 instructions differ). Two
// things are left, both inline expansions:
//   * the plane under a polygon: shipped expands the test at the one site
//     (0x10026e1b..0x10026e3b: owner, then its surface, then the plane);
//     ours calls WorldPoly::GetPlane. ltobject_internal.h's nested form
//     (the owner's own GetPlane under a second test) is what VC6 declines
//     here. A single-test inline of our own does expand (48 differing,
//     1823 bytes), but taking this TU's GetPlane COMDAT out of the link
//     moves the linker's identical-COMDAT folding and with it three of
//     server.dll's tiny placements (0x1000e358, 0x1000e7d2, 0x1000f138),
//     so the header's call stays until the header's own form is settled.
//   * the orient block: shipped expands the 3-float constructor of the
//     first two products there (0x10026cd2..0x10026d06) and calls it for
//     the third; ours calls it for all three, in every build.
// Everything else is the shipped sequence: 12 constructor, 3 Norm, 2 Mag,
// 2 MagSqr, 1 negate, 3 operator* and 2 operator+= calls. Measured: the
// orient block's constructor expands only when the function is larger
// before inlining -- two dozen dead stores ahead of the block give the
// first, four dozen all three -- so shipped's body is bigger in some way
// that compiles away; none of ~60 natural respellings (declaration forms,
// scopes, temporaries, the dot spellings) moves it. lithtech.exe (/O2)
// shows the same two differences and, following from them, a different
// spend of its budget further down (the not-standing += and a Stop Init).
int phys_ComputeFrameMove(GlobalForceControl* pControl)
{
	LTObject* pObj;
	LTVector* pOffset;
	LTVector* pNormal;
	float     fHalfTimeSqr;
	float     fVelMag;
	float     fAccelMag;
	float     fMag;
	float     fDot;
	float     fDecay;

	fHalfTimeSqr = pControl->m_fFrameTime * pControl->m_fFrameTime * 0.5f;
	pObj = pControl->m_pObj;

	fVelMag   = pControl->m_pVelocity->MagSqr();
	fAccelMag = pControl->m_pAcceleration->MagSqr();

	if (pObj->m_Flags2 & FLAG2_ORIENTMOVEMENT)
	{
		LTVector vDelta = *pControl->m_pAcceleration * pControl->m_fFrameTime;
		pControl->m_vFrameMove = *pControl->m_pVelocity * pControl->m_fFrameTime;
		pControl->m_vFrameMove += *pControl->m_pAcceleration * (fHalfTimeSqr * 0.5f);
		pObj->m_Velocity += vDelta;
		*pControl->m_pVelocity = pObj->m_Velocity;
		return 1;
	}

	LTVector vForce, vDir;
	vForce = pControl->m_Force;
	vDir   = pControl->m_Dir;

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
			{
				pObj->m_Velocity.Init();
				pObj->m_Acceleration.Init();
				pObj->m_ChangeFlags &= ~CHANGEFLAG_MOTION;
				return 0;
			}

			LTVector vUp;
			if (pObj->m_pStandingOnPoly)
			{
				pNormal = &pObj->m_pStandingOnPoly->GetPlane()->m_Normal;
			}
			else
			{
				pNormal = &vUp;
				vUp = -vDir;
			}

			LTVector vAccel = *pControl->m_pAcceleration;
			vAccel += vForce;

			LTVector vForceDir = vForce;
			vForceDir.Norm();

			if (vForceDir.Dot(*pNormal) > pControl->m_Unknown30)
			{
				fMag = vAccel.Mag();

				*pControl->m_pAcceleration = vAccel - *pNormal * VEC_DOT(vAccel, *pNormal);

				fDot = pControl->m_pAcceleration->Dot(vForceDir);
				if (fDot < 0.0f)
					*pControl->m_pAcceleration -= vForceDir * fDot;

				pControl->m_pAcceleration->Norm(fMag);
			}
			else
			{
				LTVector vNew = *pControl->m_pVelocity + vForce * pControl->m_fFrameTime;

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

						LTVector vTerm = *pControl->m_pAcceleration / pObj->m_FrictionCoefficient;
						LTVector vStart;
						vStart = *pControl->m_pVelocity - vTerm;
						vStart *= fDecay;
						LTVector vNewVel = vTerm + vStart;

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
			*pControl->m_pAcceleration += vForce;
		}
	}
	else if (fVelMag < 0.1f && fAccelMag < 0.1f)
	{
		pObj->m_Velocity.Init();
		pObj->m_Acceleration.Init();
		pObj->m_ChangeFlags &= ~CHANGEFLAG_MOTION;
		return 0;
	}

	LTVector vDelta;
	vDelta = *pControl->m_pAcceleration * pControl->m_fFrameTime;
	pControl->m_vFrameMove = *pControl->m_pVelocity * pControl->m_fFrameTime;
	pControl->m_vFrameMove += *pControl->m_pAcceleration * (fHalfTimeSqr * 0.5f);

	pObj->m_Velocity += vDelta;
	*pControl->m_pVelocity = pObj->m_Velocity;

	return 1;
}
