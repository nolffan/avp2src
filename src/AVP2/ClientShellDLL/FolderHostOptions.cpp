// FolderHostOptions.cpp: implementation of the CFolderHostOptions class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FolderHostOptions.h"
#include "FolderMgr.h"
#include "LayoutMgr.h"
#include "FolderCommands.h"
#include "ClientRes.h"
#include "InterfaceMgr.h"
#ifdef PRIMAL_HUNT
#include "CharacterButeMgr.h"
#include "WeaponMgr.h"
#endif

namespace
{
	int kWidth = 0;
	int kSlider = 0;

	enum LocalCommands
	{
		CMD_PREFERENCES = FOLDER_CMD_CUSTOM,
		CMD_SCORING,
#ifdef PRIMAL_HUNT
		CMD_CLASSES,
		CMD_WEAPONS,
		CMD_RACE_LABEL,
#endif
		CMD_PORT,
		CMD_EDIT_BANDWIDTH,
		CMD_CYCLE_BANDWIDTH,
		CMD_FIX_PORT,
	};

#ifdef PRIMAL_HUNT
	// the check boxes' commands: the weapon list's entry plus
	// CMD_WEAPON_FIRST, the class list's plus CMD_CLASS_FIRST
	enum
	{
		CMD_WEAPON_FIRST	= 100,
		CMD_CLASS_FIRST		= 200,
	};
#endif

	void ConfirmCallBack(LTBOOL bReturn, void *pData)
	{
		CFolderHostOptions *pThisFolder = (CFolderHostOptions *)pData;
		if (bReturn && pThisFolder)
			pThisFolder->SendCommand(CMD_FIX_PORT,0,0);
	}

}

static uint32 s_pBandwidths[5] = { 4000, 16000, 32000, 1000000, 10000000 };

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFolderHostOptions::CFolderHostOptions()
{
	m_pPreferences		= LTNULL;
	m_pScoring			= LTNULL;
#ifdef PRIMAL_HUNT
	m_pClasses			= LTNULL;
	m_pWeapons			= LTNULL;

	m_pRaceLabels[0]	= LTNULL;
	m_pRaceLabels[1]	= LTNULL;
	m_pRaceLabels[2]	= LTNULL;
	m_pRaceLabels[3]	= LTNULL;

	m_nUnused			= 0;
	m_pUnusedCtrl		= LTNULL;
#endif

	m_pPortGroup		= LTNULL;
	m_pPortEdit			= LTNULL;
	m_pPortLabel		= LTNULL;

#ifdef PRIMAL_HUNT
	strcpy(m_szPort, "27890");
	strcpy(m_szOldPort, "27890");
#else
	strcpy(m_szPort, "27888");
	strcpy(m_szOldPort, "27888");
#endif

	m_pBandwidthCycle	= LTNULL;
	m_pBandwidthGroup	= LTNULL;
	m_pBandwidthEdit	= LTNULL;
	m_pBandwidthLabel	= LTNULL;

	m_nBandwidth		= 0;
	strcpy(m_szBandwidth, "16000");
	strcpy(m_szOldBandwidth, "16000");

	m_nSpeed = 0;
	m_nRespawn = 0;
	m_nDamage = 0;
	m_nQueenMolt = 0;
	m_nExosuit = 0;

	memset(m_nScoring,0,sizeof(m_nScoring));
}

CFolderHostOptions::~CFolderHostOptions()
{

}


// Build the folder
LTBOOL CFolderHostOptions::Build()
{
	// Some temp variables for this function...
	CLTGUITextItemCtrl* pCtrl = LTNULL;
	LTIntPt pos;


	// Make sure to call the base class
	if (!CBaseFolder::Build()) return LTFALSE;

	// Create the title of this menu...
	CreateTitle(IDS_TITLE_HOST_OPTS);
	UseLogo(LTFALSE);


	// Add the links to other menus...
	AddLink(IDS_GAME, FOLDER_CMD_MP_HOST, IDS_HELP_HOST);
	AddLink(IDS_SETUP, FOLDER_CMD_MP_SETUP, IDS_HELP_SETUP);
#ifdef _DEMO
	AddLink(IDS_MAPS, FOLDER_CMD_MP_MAPS, IDS_HELP_MAPS)->Enable(LTFALSE);
#else
	AddLink(IDS_MAPS, FOLDER_CMD_MP_MAPS, IDS_HELP_MAPS);
#endif
	AddLink(IDS_ADVANCED, FOLDER_CMD_MP_ADVANCED, IDS_HELP_ADVANCED)->Enable(LTFALSE);
	AddLink(IDS_PLAYER, FOLDER_CMD_MP_PLAYER, IDS_HELP_PLAYER);


	// Grab some custom values out of the attributes
	if (g_pLayoutMgr->HasCustomValue(FOLDER_ID_HOST_OPTS,"ColumnWidth"))
		kWidth = g_pLayoutMgr->GetFolderCustomInt(FOLDER_ID_HOST_OPTS,"ColumnWidth");
	if (g_pLayoutMgr->HasCustomValue(FOLDER_ID_HOST_OPTS,"SliderWidth"))
		kSlider = g_pLayoutMgr->GetFolderCustomInt(FOLDER_ID_HOST_OPTS,"SliderWidth");


	m_pPreferences = AddTab(IDS_ADVANCED,CMD_PREFERENCES,IDS_HELP_HOST_PREFERENCES);
	m_pScoring = AddTab(IDS_HOST_SCORING,CMD_SCORING,IDS_HELP_HOST_SCORING);
#ifdef PRIMAL_HUNT
	// "Classes", "Select classes allowed to play."
	m_pClasses = AddTab(29775,CMD_CLASSES,29776);
	// "Weapons", "Select weapons allowed in game."
	m_pWeapons = AddTab(29777,CMD_WEAPONS,29778);
#endif


	// Create the bottom 'back' control
	pCtrl = CreateTextItem(IDS_BACK, FOLDER_CMD_MULTIPLAYER, IDS_HELP_MULTIPLAYER, LTFALSE, GetLargeFont());
	AddFixedControl(pCtrl, kBackGroup, m_BackPos);
	UseBack(LTFALSE);

	// Create the Launch button
	pos = g_pLayoutMgr->GetFolderCustomPoint((eFolderID)m_nFolderID,"LaunchPos");
	pCtrl = CreateTextItem(IDS_LAUNCH, FOLDER_CMD_LAUNCH, IDS_HELP_LAUNCH, LTFALSE, GetLargeFont());
	AddFixedControl(pCtrl, kLinkGroup, pos);


	return LTTRUE;
}

uint32 CFolderHostOptions::OnCommand(uint32 dwCommand, uint32 dwParam1, uint32 dwParam2)
{
	switch(dwCommand)
	{
		case CMD_PREFERENCES:
			{
				SetTab(0);
				m_pPreferences->Enable(LTFALSE);
				m_pScoring->Enable(LTTRUE);
#ifdef PRIMAL_HUNT
				m_pClasses->Enable(LTTRUE);
				m_pWeapons->Enable(LTTRUE);
#endif
				UpdateData(LTTRUE);
				BuildControlList();
				UpdateData(LTFALSE);
				break;
			}

		case CMD_SCORING:
			{
				SetTab(1);
				m_pPreferences->Enable(LTTRUE);
				m_pScoring->Enable(LTFALSE);
#ifdef PRIMAL_HUNT
				m_pClasses->Enable(LTTRUE);
				m_pWeapons->Enable(LTTRUE);
#endif
				UpdateData(LTTRUE);
				BuildControlList();
				UpdateData(LTFALSE);
				break;
			}

#ifdef PRIMAL_HUNT
		case CMD_CLASSES:
			{
				SetTab(2);
				m_pPreferences->Enable(LTTRUE);
				m_pScoring->Enable(LTTRUE);
				m_pClasses->Enable(LTFALSE);
				m_pWeapons->Enable(LTTRUE);
				UpdateData(LTTRUE);
				BuildControlList();
				UpdateData(LTFALSE);
				break;
			}

		case CMD_WEAPONS:
			{
				SetTab(3);
				m_pPreferences->Enable(LTTRUE);
				m_pScoring->Enable(LTTRUE);
				m_pClasses->Enable(LTTRUE);
				m_pWeapons->Enable(LTFALSE);
				UpdateData(LTTRUE);
				BuildControlList();
				UpdateData(LTFALSE);
				break;
			}
#endif

		case CMD_FIX_PORT:
		{
			char *szText = m_pPortEdit->GetText();
			uint32 nPort = atol(szText);
			uint32 nLANPort = GetValidLANPort(nPort);

			char szNewPort[8] = "";
			sprintf(szNewPort,"%d",nLANPort);
			m_pPortEdit->SetText(szNewPort);

		} break;


		case CMD_PORT:
		{
			if (GetCapture())
			{
				char *szText = m_pPortEdit->GetText();

				if(strlen(szText) < 1)
				{
					m_pPortEdit->SetText(m_szOldPort);
				}
					
				uint32 nPort = atol(szText);

				if(nPort > 65535)
				{
					nPort = 65535;
					m_pPortEdit->SetText("65535");
				}
				else if(nPort < 0)
				{
					nPort = 0;
					m_pPortEdit->SetText("0");
				}

				CServerOptions *pOpt = g_pServerOptionMgr->GetCurrentCfg();

				if(pOpt->bLANConnection)
				{
					uint32 nLANPort = GetValidLANPort(nPort);
					if (nLANPort != nPort)
					{
						HSTRING hString = g_pLTClient->FormatString(IDS_CONFIRM_LAN_PORT);
						g_pInterfaceMgr->ShowMessageBox(hString,LTMB_YESNO,ConfirmCallBack,this, LTFALSE, LTFALSE, LTTRUE);
						g_pLTClient->FreeString(hString);
					}
				}

				SetCapture(LTNULL);
				m_pPortEdit->SetColor(m_hNormalColor, m_hNormalColor, m_hNormalColor);
				m_pPortEdit->Select(LTFALSE);
				m_pPortLabel->Select(LTTRUE);
				ForceMouseUpdate();
			}
			else
			{
				SetCapture(m_pPortEdit);
				m_pPortEdit->SetColor(m_hSelectedColor, m_hSelectedColor, m_hSelectedColor);
				m_pPortEdit->Select(LTTRUE);
				m_pPortLabel->Select(LTFALSE);
			}

			break;
		}

		case CMD_EDIT_BANDWIDTH:
		{
			if (GetCapture())
			{
				char *szText = m_pBandwidthEdit->GetText();

				if(strlen(szText) < 1)
				{
					m_pBandwidthEdit->SetText(m_szOldBandwidth);
				}
				else if(atoi(szText) > 10000000)
				{
					m_pBandwidthEdit->SetText("10000000");
				}
				else if(atoi(szText) < 0)
				{
					m_pBandwidthEdit->SetText("4000");
				}

                SetCapture(LTNULL);
				m_pBandwidthEdit->SetColor(m_hNormalColor, m_hNormalColor, m_hNormalColor);
                m_pBandwidthEdit->Select(LTFALSE);
                m_pBandwidthLabel->Select(LTTRUE);
				ForceMouseUpdate();
			}
			else
			{
				SetCapture(m_pBandwidthEdit);
				m_pBandwidthEdit->SetColor(m_hSelectedColor, m_hSelectedColor, m_hSelectedColor);
				m_pBandwidthEdit->Select(LTTRUE);
				m_pBandwidthLabel->Select(LTFALSE);
			}

			break;
		}

		case CMD_CYCLE_BANDWIDTH:
		{
			UpdateData(LTTRUE);
			UpdateBandwidth();
			break;
		}

		default:
#ifdef PRIMAL_HUNT
			// a class check box
			// RETAIL BUG: both ranges run one past the last box (and the
			// class one past its 24 boxes, to the class list's 32)
			if((dwCommand >= CMD_CLASS_FIRST) && (dwCommand <= CMD_CLASS_FIRST + MAX_MP_RESTRICT_ENTRIES))
			{
				// the classes are fixed in these games
				if(	(m_Options.nGameType == MP_OVERRUN) || (m_Options.nGameType == MP_EVAC) ||
					(m_Options.nGameType == MP_HUNT) || (m_Options.nGameType == MP_SURVIVOR) ||
					m_Options.nDM_Lifecycle)
				{
					HSTRING hString = g_pLTClient->FormatString(29800);
					g_pInterfaceMgr->ShowMessageBox(hString,LTMB_OK,LTNULL,LTNULL,LTTRUE,LTTRUE,LTFALSE);
					g_pLTClient->FreeString(hString);
					break;
				}

				int i = dwCommand - CMD_CLASS_FIRST;
				m_aClassBoxes[i].SetCheck(m_aClassBoxes[i].m_bChecked != LTTRUE, dwCommand);
				m_Options.m_ClassList.SetAllowed(i, m_aClassBoxes[i].m_bChecked);
				return 1;
			}

			// a weapon check box
			if((dwCommand >= CMD_WEAPON_FIRST) && (dwCommand <= CMD_WEAPON_FIRST + MAX_HOST_WEAPON_BOXES))
			{
				int i = dwCommand - CMD_WEAPON_FIRST;
				m_aWeaponBoxes[i].SetCheck(m_aWeaponBoxes[i].m_bChecked != LTTRUE, dwCommand);
				m_Options.m_WeaponList.SetAllowed(i, m_aWeaponBoxes[i].m_bChecked);
				return 1;
			}
#endif
			return CBaseFolder::OnCommand(dwCommand,dwParam1,dwParam2);
	}

	return 1;
}


void CFolderHostOptions::Escape()
{
	if(GetCapture() == m_pPortEdit)
	{
		SetCapture(LTNULL);
		strcpy(m_szPort, m_szOldPort);
		m_pPortEdit->UpdateData(LTFALSE);
		m_pPortEdit->SetColor(m_hNormalColor, m_hNormalColor, m_hNormalColor);
		m_pPortEdit->Select(LTFALSE);
		m_pPortLabel->Select(LTTRUE);
		ForceMouseUpdate();
	}
	else
	{
		g_pInterfaceMgr->PlayInterfaceSound(IS_ESCAPE);
		g_pInterfaceMgr->SwitchToFolder(FOLDER_ID_MULTI);
	}
}


void CFolderHostOptions::OnFocus(LTBOOL bFocus)
{
	FocusSetup(bFocus);
	CBaseFolder::OnFocus(bFocus);
}


void CFolderHostOptions::FocusSetup(LTBOOL bFocus)
{
	if (bFocus)
	{
		SetTab(0);

		m_pPreferences->Enable(LTFALSE);
		m_pScoring->Enable(LTTRUE);
#ifdef PRIMAL_HUNT
		m_pClasses->Enable(LTTRUE);
		m_pWeapons->Enable(LTTRUE);
#endif

		m_Options = *(g_pServerOptionMgr->GetCurrentCfg());

		m_nSpeed = (int)(m_Options.fSpeed * 100.0f);
		m_nRespawn = (int)(m_Options.fRespawn * 100.0f);
		m_nDamage = (int)(m_Options.fDamage * 100.0f);
		m_nExosuit = (int)m_Options.nExosuit;
		m_nQueenMolt = (int)m_Options.nQueenMolt;

		for(uint8 c = 0; c < g_pServerOptionMgr->GetNumScoringClasses(); c++)
		{
			m_nScoring[c] = (int)m_Options.GetScoring( g_pServerOptionMgr->GetScoringClass( c ));
		}

		BuildControlList();

		sprintf(m_szPort, "%d", m_Options.nPortID);

		m_nBandwidth = 0;

		while((m_nBandwidth < 5) && (s_pBandwidths[m_nBandwidth] != m_Options.nBandwidth))
			m_nBandwidth++;

		UpdateBandwidth();

		UpdateData(LTFALSE);
	}
	else
	{
		UpdateData(LTTRUE);

		m_Options.fSpeed = (LTFLOAT)m_nSpeed / 100.0f;
		m_Options.fRespawn = (LTFLOAT)m_nRespawn / 100.0f;
		m_Options.fDamage = (LTFLOAT)m_nDamage / 100.0f;
		m_Options.nExosuit = (uint8)m_nExosuit;
		m_Options.nQueenMolt = (uint8)m_nQueenMolt;
		m_Options.nPortID = atoi(m_szPort);
		m_Options.nBandwidth = atoi(m_szBandwidth);

		for(uint8 c = 0; c < g_pServerOptionMgr->GetNumScoringClasses(); c++)
		{
			m_Options.SetScoring( g_pServerOptionMgr->GetScoringClass( c ), (uint8)m_nScoring[c] );
		}
		
		CServerOptions *pOpt = g_pServerOptionMgr->GetCurrentCfg();
		(*pOpt) = m_Options;
		g_pServerOptionMgr->SaveCurrentCfg();
	}
}


void CFolderHostOptions::BuildControlList()
{
	RemoveFree();
#ifdef PRIMAL_HUNT
	RemoveBoxes();
#endif

	switch (m_nCurTab)
	{
		//preferences
		case 0:
		{
			CSliderCtrl *pSlider;

			pSlider = AddSlider(IDS_OPT_PLAYER_DAMAGE, IDS_HELP_OPT_PLAYER_DAMAGE, kWidth, kSlider, &m_nDamage);
			pSlider->SetSliderRange(25, 200);
			pSlider->SetSliderIncrement(25);
			pSlider->SetNumericDisplay(LTTRUE, LTTRUE);


			// Add in the toggle controls
			CToggleCtrl *pToggle = AddToggle(IDS_OPT_LOC_DAMAGE, IDS_HELP_OPT_LOC_DAMAGE, kWidth, &m_Options.bLocationDamage);
			pToggle->SetOnString(IDS_ON);
			pToggle->SetOffString(IDS_OFF);

			pToggle = AddToggle(IDS_OPT_FRIENDLY_FIRE, IDS_HELP_OPT_FRIENDLY_FIRE, kWidth, &m_Options.bFriendlyFire);
			pToggle->SetOnString(IDS_ON);
			pToggle->SetOffString(IDS_OFF);

			pToggle = AddToggle(IDS_OPT_FRIENDLY_NAMES, IDS_HELP_OPT_FRIENDLY_NAMES, kWidth, &m_Options.bFriendlyNames);
			// reversed by design
			pToggle->SetOnString(IDS_OFF);
			pToggle->SetOffString(IDS_ON);

			pToggle = AddToggle(IDS_OPT_PRED_MASK_LOSS, IDS_HELP_OPT_PRED_MASK_LOSS, kWidth, &m_Options.bMaskLoss);
			pToggle->SetOnString(IDS_ON);
			pToggle->SetOffString(IDS_OFF);

			pToggle = AddToggle(IDS_OPT_CLASS_WEAPONS, IDS_HELP_OPT_CLASS_WEAPONS, kWidth, &m_Options.bClassWeapons);
			pToggle->SetOnString(IDS_ON);
			pToggle->SetOffString(IDS_OFF);

			// Add a queen slider...
			CSliderCtrl *pNewSlider;
			pNewSlider = AddSlider(IDS_OPT_EXOSUIT, IDS_HELP_OPT_EXOSUIT, kWidth, kSlider, &m_nExosuit);
			pNewSlider->SetSliderRange(0, 10);
			pNewSlider->SetSliderIncrement(1);
			pNewSlider->SetNumericDisplay(LTTRUE, LTTRUE);

			// Add a queen slider...
			pNewSlider = AddSlider(IDS_OPT_QUEEN, IDS_HELP_OPT_QUEEN, kWidth, kSlider, &m_nQueenMolt);
			pNewSlider->SetSliderRange(0, 10);
			pNewSlider->SetSliderIncrement(1);
			pNewSlider->SetNumericDisplay(LTTRUE, LTTRUE);

			// Create the port ID control
			m_pPortEdit = CreateEditCtrl("", CMD_PORT, IDS_HELP_PORTID, m_szPort, 7);
			m_pPortEdit->SetInputMode(CLTGUIEditCtrl::kInputNumberOnly);
			m_pPortEdit->EnableCursor();

			m_pPortLabel = CreateTextItem(IDS_PORTID, CMD_PORT, IDS_HELP_PORTID);

			int nGroupWidth = g_pLayoutMgr->GetFolderCustomInt(FOLDER_ID_HOST_OPTS, "PortGroupWidth");

			m_pPortGroup = CreateGroup(nGroupWidth, m_pPortLabel->GetHeight(), IDS_HELP_PORTID);
			m_pPortGroup->AddControl(m_pPortLabel, LTIntPt(0, 0), LTTRUE);
			m_pPortGroup->AddControl(m_pPortEdit, LTIntPt(kWidth - 25, 0), LTFALSE);
			AddFreeControl(m_pPortGroup);

			// Create the Bandwidth cycle control
			m_pBandwidthCycle = AddCycleItem(IDS_SERVER_BANDWIDTH, IDS_HELP_SERVER_BANDWIDTH, kWidth, 0, &m_nBandwidth);
			m_pBandwidthCycle->AddString(IDS_CONNECT_VSLOW);
			m_pBandwidthCycle->AddString(IDS_CONNECT_SLOW);
			m_pBandwidthCycle->AddString(IDS_CONNECT_MEDIUM);
			m_pBandwidthCycle->AddString(IDS_CONNECT_FAST);
			m_pBandwidthCycle->AddString(IDS_CONNECT_VFAST);
			m_pBandwidthCycle->AddString(IDS_CUSTOM);
			m_pBandwidthCycle->NotifyOnChange(CMD_CYCLE_BANDWIDTH, this, 0, 0);

			// Create the custom bandwidth setting controls
			m_pBandwidthEdit = CreateEditCtrl(" ", CMD_EDIT_BANDWIDTH, IDS_HELP_CUSTOM_BANDWIDTH, m_szBandwidth, 10);
			m_pBandwidthEdit->SetInputMode(CLTGUIEditCtrl::kInputNumberOnly);
			m_pBandwidthEdit->EnableCursor();

			m_pBandwidthLabel = CreateTextItem(IDS_CUSTOM_BANDWIDTH, CMD_EDIT_BANDWIDTH, IDS_HELP_CUSTOM_BANDWIDTH);

			m_pBandwidthGroup = CreateGroup(nGroupWidth, m_pBandwidthLabel->GetHeight(), IDS_HELP_CUSTOM_BANDWIDTH);
		    m_pBandwidthGroup->AddControl(m_pBandwidthLabel, LTIntPt(0, 0), LTTRUE);
		    m_pBandwidthGroup->AddControl(m_pBandwidthEdit, LTIntPt(kWidth - 25, 0), LTFALSE);
			AddFreeControl(m_pBandwidthGroup);
		}
		break;

		//scoring
		case 1:
		{
			for (uint8 c = 0; c < g_pServerOptionMgr->GetNumScoringClasses(); c++)
			{
				HSTRING hStr = g_pLTClient->FormatString(g_pServerOptionMgr->GetScoringClass(c));
				if (hStr)
				{
					CSliderCtrl *pSlider = AddSlider(hStr, LTNULL, kWidth, kSlider, &m_nScoring[c]);
					pSlider->SetSliderRange(5,50);
					pSlider->SetSliderIncrement(5);
					pSlider->SetNumericDisplay(LTTRUE,LTTRUE);
					pSlider->SetHelpID(IDS_HELP_SCORING);
					g_pLTClient->FreeString(hStr);
				}
			}

		} break;

#ifdef PRIMAL_HUNT
		//classes
		case 2:
		{
			// the races' columns
			int nX[4] = { 200, 410, 200, 410 };
			int nY[4] = { 130, 130, 260, 260 };
			int nRows[5] = { 40, 56, 72, 88, 104 };
			int nRaceLabels[4] = { IDS_MP_TEAM_MARINES, IDS_MP_TEAM_CORPORATES, IDS_MP_TEAM_PREDATORS, IDS_MP_TEAM_ALIENS };

			// the classes are fixed in these games
			if(	(m_Options.nGameType == MP_OVERRUN) || (m_Options.nGameType == MP_EVAC) ||
				(m_Options.nGameType == MP_HUNT) || (m_Options.nGameType == MP_SURVIVOR) ||
				m_Options.nDM_Lifecycle)
			{
				for(int i = 0; i < m_Options.m_ClassList.m_nNumEntries; i++)
					m_Options.m_ClassList.m_aEntries[i].bAllowed = LTTRUE;
			}

			for(int nRace = 0; nRace < 4; nRace++)
			{
				m_pRaceLabels[nRace] = CreateTextItem(nRaceLabels[nRace], CMD_RACE_LABEL, IDS_SPACER, LTTRUE, GetLargeFont());
				AddFixedControl(m_pRaceLabels[nRace], kBackGroup, LTIntPt(nX[nRace] + 30, nY[nRace]), LTFALSE);
			}

			int nCount[4] = { 0, 0, 0, 0 };

			for(int i = 0; i < m_Options.m_ClassList.m_nNumEntries; i++)
			{
				CharacterButes butes = g_pCharacterButeMgr->GetCharacterButes(m_Options.m_ClassList.GetId(i));
				nRace = butes.m_eCharacterClass - 1;

				AddClassBox(CMD_CLASS_FIRST + i, nX[nRace], nY[nRace] + nRows[nCount[nRace]], butes.m_nDisplayName, m_Options.m_ClassList.IsAllowed(i));
				nCount[nRace]++;
			}
		}
		break;

		//weapons
		case 3:
		{
			int x = 200;
			int y = 130;

			for(int i = 0; i < m_Options.m_WeaponList.m_nNumEntries; i++)
			{
				WEAPON *pWeapon = g_pWeaponMgr->GetWeapon(m_Options.m_WeaponList.GetId(i));

				// show the name without the "_MP"
				int nLen = strstr(pWeapon->szName, "_MP") - pWeapon->szName;

				if(nLen > 0)
				{
					char szName[256];
					memset(szName, 0, sizeof(szName));
					strncpy(szName, pWeapon->szName, nLen);

					for(unsigned int j = 0; j < strlen(szName); j++)
					{
						if(szName[j] == '_')
							szName[j] = ' ';
					}

					if(!_stricmp(szName, "StickyHotBomb"))
						strcpy(szName, "Remote Bomb");
					else if(!_stricmp(szName, "EnergyFlechette"))
						strcpy(szName, "Energy Flechette");

					AddWeaponBox(CMD_WEAPON_FIRST + i, x, y, szName, m_Options.m_WeaponList.IsAllowed(i));
				}
				else
				{
					AddWeaponBox(CMD_WEAPON_FIRST + i, x, y, pWeapon->szName, m_Options.m_WeaponList.IsAllowed(i));
				}

				// next row, or the next column
				y += 16;

				if(y >= 282)
				{
					y = 130;
					x += 200;
				}
			}
		}
		break;
#endif
	}

#ifdef PRIMAL_HUNT
	// RETAIL BUG: the race headings are destroyed but neither deleted nor
	// removed from the fixed controls
	if(m_nCurTab != 2)
	{
		for(int i = 0; i < 4; i++)
		{
			if(m_pRaceLabels[i])
			{
				m_pRaceLabels[i]->Destroy();
				m_pRaceLabels[i] = LTNULL;
			}
		}
	}

	if((m_nCurTab != 3) && m_pUnusedCtrl)
	{
		m_pUnusedCtrl->Destroy();
		delete m_pUnusedCtrl;
		m_pUnusedCtrl = LTNULL;
	}
#endif
}


#ifdef PRIMAL_HUNT
LTBOOL CFolderHostOptions::Render(HSURFACE hDestSurf)
{
	if(m_hTabSurf)
	{
		LTRect rcDest(0,0,0,0);

		// the check box pages get a page background
		switch(m_nCurTab)
		{
			//classes
			case 2:
			{
				rcDest.left		= GetPageLeft() + g_pInterfaceResMgr->GetXOffset();
				rcDest.top		= GetPageTop() + g_pInterfaceResMgr->GetYOffset();
				rcDest.right	= GetPageRight() + g_pInterfaceResMgr->GetXOffset();
				rcDest.bottom	= GetPageBottom() - (int)(g_pInterfaceResMgr->GetYRatio() * 8.0f) + g_pInterfaceResMgr->GetYOffset();
			}
			break;

			//weapons
			case 3:
			{
				rcDest.left		= GetPageLeft() + g_pInterfaceResMgr->GetXOffset();
				rcDest.top		= GetPageTop() + g_pInterfaceResMgr->GetYOffset();
				rcDest.right	= GetPageRight() + g_pInterfaceResMgr->GetXOffset();
				rcDest.bottom	= GetPageBottom() - (int)(g_pInterfaceResMgr->GetYRatio() * 8.0f) + g_pInterfaceResMgr->GetYOffset();
			}
			break;
		}

		if(rcDest.left)
			g_pLTClient->ScaleSurfaceToSurface(hDestSurf, m_hTabSurf, &rcDest, LTNULL);
	}

	return CBaseFolder::Render(hDestSurf);
}


void CFolderHostOptions::AddClassBox(uint32 dwCommand, int x, int y, int nStringID, LTBOOL bChecked)
{
	m_aClassBoxes[dwCommand - CMD_CLASS_FIRST].Create(this, dwCommand, x, y, nStringID, bChecked);
}


void CFolderHostOptions::AddWeaponBox(uint32 dwCommand, int x, int y, char *szText, LTBOOL bChecked)
{
	m_aWeaponBoxes[dwCommand - CMD_WEAPON_FIRST].Create(this, dwCommand, x, y, szText, bChecked);
}


void CHostCheckBox::Create(CBaseFolder *pFolder, uint32 dwCommand, int x, int y, int nStringID, LTBOOL bChecked)
{
	m_pFolder = pFolder;

	m_pLabel = m_pFolder->CreateTextItem(nStringID, dwCommand, IDS_SPACER, LTFALSE, m_pFolder->GetTinyFont());
	m_pFolder->AddFixedControl(m_pLabel, kBackGroup, LTIntPt(x + 20, y + 2));
	m_pFolder->AddFixedControl(m_pBitmap, kLinkGroup, LTIntPt(x, y));

	SetCheck(bChecked, dwCommand);
}


void CHostCheckBox::Create(CBaseFolder *pFolder, uint32 dwCommand, int x, int y, char *szText, LTBOOL bChecked)
{
	m_pFolder = pFolder;

	m_pLabel = m_pFolder->CreateTextItem(szText, dwCommand, IDS_SPACER, LTFALSE, m_pFolder->GetTinyFont());
	m_pFolder->AddFixedControl(m_pLabel, kBackGroup, LTIntPt(x + 20, y + 2));
	m_pFolder->AddFixedControl(m_pBitmap, kLinkGroup, LTIntPt(x, y));

	SetCheck(bChecked, dwCommand);
}


LTBOOL CHostCheckBox::SetCheck(LTBOOL bChecked, uint32 dwCommand)
{
	m_bChecked = bChecked;

	if(m_pBitmap)
		m_pBitmap->Destroy();

	if(m_bChecked)
	{
		m_pBitmap->Create(g_pLTClient, "interface\\menus\\checkon.pcx", "interface\\menus\\checkon_h.pcx", "interface\\menus\\checkon_d.pcx", m_pFolder, dwCommand);
		m_pBitmap->SetHelpID(IDS_SPACER);
		return m_bChecked;
	}

	m_pBitmap->Create(g_pLTClient, "interface\\menus\\checkoff.pcx", "interface\\menus\\checkoff_h.pcx", "interface\\menus\\checkoff_d.pcx", m_pFolder, dwCommand);
	m_pBitmap->SetHelpID(IDS_SPACER);
	return m_bChecked;
}


void CHostCheckBox::Destroy()
{
	if(m_pLabel)
	{
		m_pLabel->Destroy();
		m_pLabel = LTNULL;
	}
}


void CFolderHostOptions::RemoveBoxes()
{
	int i;

	for(i = 0; i < g_pCharacterButeMgr->m_MPClasses.m_nNumEntries; i++)
	{
		CHostCheckBox &box = m_aClassBoxes[i];
		box.Destroy();
		RemoveFixedControl(box.m_pBitmap);
	}

	for(i = 0; i < g_pWeaponMgr->m_MPWeapons.m_nNumEntries; i++)
	{
		CHostCheckBox &box = m_aWeaponBoxes[i];
		box.Destroy();
		RemoveFixedControl(box.m_pBitmap);
	}
}
#endif


LTBOOL CFolderHostOptions::OnEnter()
{
	if(GetCapture() == m_pPortEdit)
	{
		char *szText = m_pPortEdit->GetText();

		if(strlen(szText) < 1)
			m_pPortEdit->SetText(m_szOldPort);
	}

	return CBaseFolder::OnEnter();
}

LTBOOL	CFolderHostOptions::OnLButtonUp(int x, int y)
{
	if(GetCapture())
	{
		OnEnter();
		return LTTRUE;
	}

	return CBaseFolder::OnLButtonUp(x,y);
}

LTBOOL	CFolderHostOptions::OnRButtonUp(int x, int y)
{
	if(GetCapture())
	{
		OnEnter();
		return LTTRUE;
	}

	return CBaseFolder::OnRButtonUp(x,y);
}

void CFolderHostOptions::UpdateBandwidth()
{
	if(m_nBandwidth >= 5)
	{
		m_pBandwidthGroup->Enable(LTTRUE);
		m_pBandwidthEdit->SetColor(m_hNormalColor, m_hNormalColor, m_hNormalColor);

		if(!IsValidBandwidth(m_Options.nBandwidth))
			m_Options.nBandwidth = DEFAULT_BANDWIDTH;

		sprintf(m_szBandwidth, "%d", m_Options.nBandwidth);
	}
	else
	{
		m_pBandwidthGroup->Enable(LTFALSE);
		m_pBandwidthEdit->SetColor(m_hDisabledColor, m_hDisabledColor, m_hDisabledColor);

		sprintf(m_szBandwidth, "%d", s_pBandwidths[m_nBandwidth]);
	}

	m_pBandwidthEdit->Select(LTFALSE);
	m_pBandwidthEdit->SetText(m_szBandwidth);
}
