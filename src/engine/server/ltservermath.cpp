// *********************************************************************** //
//  MODULE:   LTSERVERMATH (.CPP)
//  PURPOSE:  ILTCSBase rotation/matrix helpers — validate pointer args, then
//            forward to the math implementation embedded at server+4.
//            Build /O1.
//
//  Split from servermath.cpp: these wrappers lie with the server
//  interface's message helpers (server.dll 0x100086b3..0x10008bd2,
//  lithtech.exe 0x00426b40..0x004270df), a translation unit apart from
//  ILTMath's methods and the rest of servermath.cpp.
// *********************************************************************** //

#include "servermath.h"
#include "de_error.h"

// PH: Primal Hunt's engine TUs see the C++ library's <string>, as ~95 of
// lithtech.exe's do (AvP2 1.0.9.6's engine TUs none). Its trace is
// std::ctype<unsigned short>::id's guarded initializer (lithtech.exe guard
// 0x004f8014 bit 0, atexit of the empty 0x00401790); Primal Hunt's
// server.dll has it right after SetupRotationAroundPoint (0x10008bd2, 33
// bytes: guard 0x1008ec84, atexit of 0x1002eaeb).
#if defined(PRIMAL_HUNT)
#include <string>
#endif

LTRESULT CLTServerMath::GetRotationVectors(LTRotation* pRotation, LTVector* pUp, LTVector* pRight, LTVector* pForward)
{
	if (!pRotation || !pUp || !pRight || !pForward)
		RETURN_ERROR_PARAM("ILTCSBase::GetRotationVectors");

	ILTMath* pC = &m_common;
	return pC->GetRotationVectors(*pRotation, *pRight, *pUp, *pForward);
}

void CLTServerMath::GetRotationVectorsFromMatrix(LTMatrix* pMat, LTVector* pUp, LTVector* pRight, LTVector* pForward)
{
	if (!pMat || !pUp || !pRight || !pForward)
		RETURN_ERROR_VOID("ILTCSBase::GetRotationVectorsFromMatrix");

	ILTMath* pC = &m_common;
	pC->GetRotationVectorsFromMatrix(*pMat, *pRight, *pUp, *pForward);
}

void CLTServerMath::RotateAroundAxis(LTRotation* pRotation, LTVector* pAxis, float amount)
{
	if (!pRotation || !pAxis)
		RETURN_ERROR_VOID("ILTCSBase::RotateAroundAxis");

	ILTMath* pC = &m_common;
	pC->RotateAroundAxis(*pRotation, *pAxis, amount);
}

void CLTServerMath::EulerRotateX(LTRotation* pRotation, float amount)
{
	if (!pRotation)
		RETURN_ERROR_VOID("ILTCSBase::EulerRotateX");

	ILTMath* pC = &m_common;
	pC->EulerRotateX(*pRotation, amount);
}

void CLTServerMath::EulerRotateY(LTRotation* pRotation, float amount)
{
	if (!pRotation)
		RETURN_ERROR_VOID("ILTCSBase::EulerRotateY");

	ILTMath* pC = &m_common;
	pC->EulerRotateY(*pRotation, amount);
}

void CLTServerMath::EulerRotateZ(LTRotation* pRotation, float amount)
{
	if (!pRotation)
		RETURN_ERROR_VOID("ILTCSBase::EulerRotateZ");

	ILTMath* pC = &m_common;
	pC->EulerRotateZ(*pRotation, amount);
}

void CLTServerMath::AlignRotation(LTRotation* pRotation, LTVector* pVector, LTVector* pUp)
{
	// pUp is optional (not validated by the shipped code).
	if (!pRotation || !pVector)
		RETURN_ERROR_VOID("ILTCSBase::AlignRotation");

	ILTMath* pC = &m_common;
	pC->AlignRotation(*pRotation, *pVector, *pUp);
}

LTRESULT CLTServerMath::SetupEuler(LTRotation* pRotation, float pitch, float yaw, float roll)
{
	if (!pRotation)
		RETURN_ERROR_PARAM("ILTCSBase::SetupEuler");

	ILTMath* pC = &m_common;
	return pC->SetupEuler(*pRotation, pitch, yaw, roll);
}

LTRESULT CLTServerMath::InterpolateRotation(LTRotation* pDest, LTRotation* pRot1, LTRotation* pRot2, float t)
{
	if (!pDest || !pRot1 || !pRot2)
		RETURN_ERROR_PARAM("ILTCSBase::InterpolateRotation");

	ILTMath* pC = &m_common;
	return pC->InterpolateRotation(*pDest, *pRot1, *pRot2, t);
}

void CLTServerMath::SetupTransformationMatrix(LTMatrix* pMat, LTVector* pTranslation, LTRotation* pRotation)
{
	if (!pMat || !pTranslation || !pRotation)
		RETURN_ERROR_VOID("ILTCSBase::SetupTransformationMatrix");

	ILTMath* pC = &m_common;
	pC->SetupTransformationMatrix(*pMat, *pTranslation, *pRotation);
}

void CLTServerMath::SetupTranslationMatrix(LTMatrix* pMat, LTVector* pTranslation)
{
	if (!pMat || !pTranslation)
		RETURN_ERROR_VOID("ILTCSBase::SetupTranslationMatrix");

	ILTMath* pC = &m_common;
	pC->SetupTranslationMatrix(*pMat, *pTranslation);
}

void CLTServerMath::SetupRotationMatrix(LTMatrix* pMat, LTRotation* pRot)
{
	if (!pMat || !pRot)
		RETURN_ERROR_VOID("ILTCSBase::SetupRotationMatrix");

	ILTMath* pC = &m_common;
	pC->SetupRotationMatrix(*pMat, *pRot);
}

void CLTServerMath::SetupTranslationFromMatrix(LTVector* pTranslation, LTMatrix* pMat)
{
	if (!pTranslation || !pMat)
		RETURN_ERROR_VOID("ILTCSBase::SetupTranslationFromMatrix");

	ILTMath* pC = &m_common;
	pC->SetupTranslationFromMatrix(*pTranslation, *pMat);
}

void CLTServerMath::SetupRotationFromMatrix(LTRotation* pRot, LTMatrix* pMat)
{
	if (!pRot || !pMat)
		RETURN_ERROR_VOID("ILTCSBase::SetupRotationFromMatrix");

	ILTMath* pC = &m_common;
	pC->SetupRotationFromMatrix(*pRot, *pMat);
}

void CLTServerMath::SetupRotationAroundPoint(LTMatrix* pMat, LTRotation* pRot, LTVector* pPoint)
{
	if (!pMat || !pRot || !pPoint)
		RETURN_ERROR_VOID("ILTCSBase::SetupRotationAroundPoint");

	ILTMath* pC = &m_common;
	pC->SetupRotationAroundPoint(*pMat, *pRot, *pPoint);
}
