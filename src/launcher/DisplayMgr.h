// *********************************************************************** //
//
//	MODULE:		DISPLAYMGR (.H)
//
//	PURPOSE:	The display-mode tree: renderers, their cards and modes
//				(NOLF's launcher's, nolfsrc's launcher.h)
//
// *********************************************************************** //

#ifndef _DISPLAYMGR_H_
#define _DISPLAYMGR_H_

// The renderer DLL's mode descriptor, as published by the engine SDK
// (src/LT2/sdk/inc/ltbasedefs.h).  The launcher links no engine code, so it
// carries its own copy.

struct RMode
{
	int			m_bHardware;
	char		m_RenderDLL[256];		// What DLL this comes from.
	char		m_InternalName[128];	// This is what the DLLs use to identify a card.
	char		m_Description[128];		// This is a 'friendly' string describing the card.
	unsigned	m_Width, m_Height, m_BitDepth;
	RMode*		m_pNext;
};

// The ID of the description string contained in render DLLs.
#define RDLL_DESCRIPTION_STRINGID	5

class CDisplayMode
{
public:
	CDisplayMode() { m_nWidth = 0; m_nHeight = 0; }
	~CDisplayMode() { m_nWidth = 0; m_nHeight = 0; }

	int			m_nWidth;			// +0x00
	int			m_nHeight;			// +0x04
	int			m_nDepth;			// +0x08
	CString		m_csName;			// +0x0c

	BOOL		Init(int nWidth, int nHeight, int nDepth);
	CString		GetName();
	CString		GetDisplayName();
};

class CCard
{
public:
	CCard() { m_csName.Empty(); m_bHardware = 0; m_pCurMode = NULL; }
	~CCard() { Term(); }

	CString			m_csName;		// +0x00
	CString			m_csDesc;		// +0x04
	int				m_bHardware;	// +0x08
	CPtrList		m_modes;		// +0x0c
	CDisplayMode*	m_pCurMode;		// +0x28
	POSITION		m_pos;			// +0x2c - the class's own list iterator

	BOOL			Init(const char* sName, const char* sDesc, int bHardware);
	void			Term();
	CString			GetName();
	CString			GetDesc();
	CString			GetDisplayName();
	LRESULT			GetListSelData(HWND hList);
	CDisplayMode*	AddMode(int nWidth, int nHeight, int nDepth);
	CDisplayMode*	FindMode(int nWidth, int nHeight, int nDepth);
	BOOL			SetCurMode(int nWidth, int nHeight, int nDepth);
	BOOL			SelectCurMode(HWND hList);
	BOOL			FillModeList(HWND hList);
};

class CRenderer
{
public:
	CRenderer() { m_csDll.Empty(); m_csDesc.Empty(); m_pCurCard = NULL; }
	~CRenderer() { Term(); }

	CString		m_csDll;			// +0x00
	CString		m_csDesc;			// +0x04
	CPtrList	m_cards;			// +0x08
	CCard*		m_pCurCard;			// +0x24
	POSITION	m_pos;				// +0x28 - the class's own list iterator

	BOOL		Init(const char* sDll);
	void		Term();
	CString		GetName();
	CString		GetDesc();
	CString		GetListName();
	LRESULT		GetListSelData(HWND hList);
	CCard*		AddCard(const char* sName, const char* sDesc, int bHardware);
	CCard*		AddCard(RMode* pMode);
	CCard*		FindCard(const char* sName);
	CCard*		FindCard(RMode* pMode);
	BOOL		SetCurCard(const char* sName);
	BOOL		SelectCurCard(HWND hList);
	BOOL		FillCardList(HWND hList);
};

class CDisplayModes
{
public:
	CDisplayModes() { m_pCurRenderer = NULL; }
	~CDisplayModes() { Term(); }

	CString		m_csRenderDll;		// +0x00
	CPtrList	m_renderers;		// +0x04
	CRenderer*	m_pCurRenderer;		// +0x20
	POSITION	m_pos;				// +0x24 - the class's own list iterator

	BOOL		Init(const char* sDir, const char* sFile);
	void		Term();
	LRESULT		GetListSelData(HWND hList);
	CRenderer*	AddRenderer(const char* sDll);
	CRenderer*	FindRenderer(const char* sDll);
	BOOL		SelectCurRenderer(HWND hList);
	BOOL		FillRendererList(HWND hList);
	int			FindRenderers(const char* sDir);
	void		SetDefaultModes();
	BOOL		FillLists(HWND hRenderer, HWND hCard, HWND hMode);
	BOOL		UpdateLists(HWND hRenderer, HWND hCard, HWND hMode);
	BOOL		ReadDisplayConfig(const char* sFile);
	CString		GetCommands();
};

#endif
