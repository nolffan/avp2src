// *********************************************************************** //
//  ph/geomroutines.cpp -- Primal Hunt's d3d.ren, TU35, third object
//  (0x10032c6c..0x10032e09), /O1.
//
//  The shared GeomRoutines (NOLF2's runtime/shared/src/geomroutines.cpp):
//  the renderer keeps only gr_GetPerpendicularVector, which the model
//  shadow code calls; the link's /OPT:REF dropped the rest.
// *********************************************************************** //

#include "ltbasedefs.h"


// 0x10032c6c
void gr_GetPerpendicularVector(LTVector *pVec, LTVector *pRef, LTVector *pPerp)
{
	float dot, t;
	LTVector temp, tempRef;

	if(!pRef)
	{
		tempRef.Init(0, 1, 0);
		pRef = &tempRef;
	}

	*pPerp = *pRef;

	// Are pRef and pVec the same?  If not, we can exit.
	dot = pVec->Dot(*pPerp);
	if(dot > 0.99f || dot < -0.99f)
	{
		// Try to modify it as little as possible.
		pPerp->z += 5.0f;
		pPerp->Norm();
		dot = VEC_DOT(*pVec, *pPerp);
		if(dot > 0.99f || dot < -0.99f)
		{
			pPerp->x += 5.0f;
			pPerp->y += 5.0f;
			pPerp->Norm();

			dot = pVec->Dot(*pPerp);
			if(dot > 0.99f || dot < -0.99f)
			{
				pPerp->x += 5.0f;
				pPerp->y -= 2.0f;
				pPerp->z -= 5.0f;
				pPerp->Norm();
			}
		}
	}

	// Make pVec and pPerp linear independent.
	t = -pVec->Dot(*pPerp);
	temp = *pVec * t;
	*pPerp += temp;
	pPerp->Norm();
}
