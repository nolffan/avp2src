// mfcs_point.cpp - implementation for the MFC stub CPoint class

#include "stdafx.h"
#include "mfcs_point.h"

CPoint::CPoint()
{
}

CPoint::CPoint(const CPoint &cPt) 
{
	*(POINT *)this = cPt;
}

CPoint::CPoint(const POINT *pPt) 
{
	*(POINT *)this = *pPt;
}

CPoint::CPoint(SDWORD newx, SDWORD newy)
{
	x = newx;
	y = newy;
}

