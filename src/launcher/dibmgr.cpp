/****************************************************************************
;
;	 MODULE:		DIBMGR (.CPP)
;
;	PURPOSE:		DIB Manager Class
;
;	HISTORY:		02/15/96  [blg]  This file was created
;
;	COMMENT:		Copyright (c) 1996, Monolith Inc.
;
****************************************************************************/


// Includes...

#include "stdafx.h"
#include "dibmgr.h"


// Statics...

HINSTANCE	CDibMgr::s_hInst = NULL;


// Functions...

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::Init
//
//	PURPOSE:	Initialization
//
// ----------------------------------------------------------------------- //

BOOL CDibMgr::Init(HINSTANCE hInst, HWND hWnd, DWORD flags)
{
	// Sanity checks...

	ASSERT(!IsValid());
	ASSERT(hInst);
	ASSERT(hWnd);


	// Set simple member variables...

	m_hInst   = hInst;
	m_hWnd    = hWnd;
	m_dwFlags = flags;


	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::Term
//
//	PURPOSE:	Termination
//
// ----------------------------------------------------------------------- //

void CDibMgr::Term()
{
	RemoveAllDibs();
	RemoveAllPals();

	m_hInst   = NULL;
	m_hWnd    = NULL;
	m_dwFlags = 0;
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::RemoveDib
//
//	PURPOSE:	Removes the given dib
//



// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::RemovePal
//
//	PURPOSE:	Removes the given palette
//



// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::RemoveAllDibs
//
//	PURPOSE:	Removes all dibs that we are managing
//
// ----------------------------------------------------------------------- //

void CDibMgr::RemoveAllDibs()
{
	// Delete all dibs from our collection...

	POSITION pos = m_collDibs.GetHeadPosition();

	while (pos)
	{
		CDib* pDib = (CDib*)m_collDibs.GetNext(pos);
		ASSERT(pDib && pDib->IsValid());

		delete pDib;
	}

	m_collDibs.RemoveAll();
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::RemoveAllPals
//
//	PURPOSE:	Removes all pals that we are managing
//
// ----------------------------------------------------------------------- //

void CDibMgr::RemoveAllPals()
{
	// Delete all pals from our collection...

	POSITION pos = m_collPals.GetHeadPosition();

	while (pos)
	{
		CDibPal* pPal = (CDibPal*)m_collPals.GetNext(pos);
		ASSERT(pPal && pPal->IsValid());

		delete pPal;
	}

	m_collPals.RemoveAll();

	m_pCurPal = NULL;
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::AddDib
//
//	PURPOSE:	Loads a dib from a file
//
// ----------------------------------------------------------------------- //

CDib* CDibMgr::AddDib(const char* sFile, DWORD flags)
{
	// Sanity checks...

	ASSERT(IsValid());
	ASSERT(sFile);


	// Get an hDC to our window...

	HDC hDC = GetDC(FALSE);


	// Set the global hInst in case we are trying to load a res...

	s_hInst = m_hInst;


	// Create and init a new dib...

	CDib* pDib = new CDib();

	if (!pDib->Init(sFile, hDC, flags))
	{
		ReleaseDC(hDC);
		delete pDib;
		return(NULL);
	}


	// Add the new dib to our collection...

	POSITION pos = m_collDibs.AddTail(pDib);

	pDib->SetPos(pos);


	// Clean up...

	ReleaseDC(hDC);


	// All done...

	return(pDib);
}



// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::AddDib
//
//	PURPOSE:	Copies the dib you pass in, and promotes FROM 8 TO 16 ONLY.
//






CDibPal* CDibMgr::AddPal(const char* sFile, DWORD flags)
{
	// Sanity checks...

	ASSERT(IsValid());


	// Set the global hInst in case we are trying to load a res...

	s_hInst = m_hInst;


	// Create and init a new palette...

	CDibPal* pPal = new CDibPal();

	if (!pPal->Init(sFile, flags))
	{
		delete pPal;
		return(NULL);
	}


	// Add the new palette to our collection...

	POSITION pos = m_collPals.AddTail(pPal);

	pPal->SetPos(pos);


	// All done...

	return(pPal);
}





// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::AddPal
//
//	PURPOSE:	Adds a new Pal object
//



// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDibMgr::AddPal
//
//	PURPOSE:	Adds a new Pal object
//




