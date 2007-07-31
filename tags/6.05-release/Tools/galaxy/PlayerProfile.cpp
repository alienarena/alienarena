// PlayerProfile.cpp : implementation file
//

#include "stdafx.h"
#include "Galaxy.h"
#include "PlayerProfile.h"
#include "userinfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern cUser user;
extern bool joinflg;
extern char CRXPath[MAX_PATH];
extern char servidor[100];
/////////////////////////////////////////////////////////////////////////////
// PlayerProfile dialog


PlayerProfile::PlayerProfile(CWnd* pParent /*=NULL*/)
	: CDialog(PlayerProfile::IDD, pParent)
{
	//{{AFX_DATA_INIT(PlayerProfile)
	m_playeremailstr = user.email;
	m_playernamestr = user.nick;
	m_joinstartup = joinflg;
	m_gamepathstr = CRXPath;
	//}}AFX_DATA_INIT
}


void PlayerProfile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PlayerProfile)
	DDX_Control(pDX, IDC_IRCSERVER, m_ircselectserver);
	DDX_Control(pDX, IDC_GAMEPATH, m_gamepathctrl);
	DDX_Control(pDX, IDC_JOINATSTARTUP, m_joinstartupctrl);
	DDX_Control(pDX, IDC_PLAYERNAME, m_playernamectrl);
	DDX_Control(pDX, IDC_PLAYEREMAIL, m_playeremailctrl);
	DDX_Text(pDX, IDC_PLAYEREMAIL, m_playeremailstr);
	DDX_Text(pDX, IDC_PLAYERNAME, m_playernamestr);
	DDX_Check(pDX, IDC_JOINATSTARTUP, m_joinstartup);
	DDX_Text(pDX, IDC_GAMEPATH, m_gamepathstr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PlayerProfile, CDialog)
	//{{AFX_MSG_MAP(PlayerProfile)
	ON_EN_CHANGE(IDC_PLAYERNAME, OnChangePlayername)
	ON_EN_CHANGE(IDC_PLAYEREMAIL, OnChangePlayeremail)
	ON_BN_CLICKED(IDC_JOINATSTARTUP, OnJoinatstartup)
	ON_EN_CHANGE(IDC_GAMEPATH, OnChangeGamepath)
	ON_LBN_SELCHANGE(IDC_IRCSERVER, OnSelchangeIrcserver)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL PlayerProfile::OnInitDialog() 
{
	CDialog::OnInitDialog();

	//initialize drop list
	m_ircselectserver.AddString("irc.blackened.com");
	m_ircselectserver.AddString("irc.prison.net");
	m_ircselectserver.AddString("irc.anynet.org");

	GetPrivateProfileString("Galaxy", "chatstart", "true", user.joinatstart, 12, "galaxy.ini");
	//set the join flag for the dialog bool
	if(!_tcscmp(user.joinatstart, "true"))
		joinflg = true;
	else
		joinflg = false;
	m_joinstartup = joinflg;

	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// PlayerProfile message handlers
void PlayerProfile::OnChangePlayername() 
{
	m_playernamectrl.GetWindowText(m_playernamestr);
}

void PlayerProfile::OnChangePlayeremail() 
{
	m_playeremailctrl.GetWindowText(m_playeremailstr);		
}
void PlayerProfile::OnChangeGamepath() 
{
	m_gamepathctrl.GetWindowText(m_gamepathstr);	
}
void PlayerProfile::OnOK() 
{
	// TODO: Add extra validation here
	//don't leave if player name is not set
	sprintf(user.nick, "%s", m_playernamestr);
	sprintf(user.email, "%s", m_playeremailstr);
	sprintf(CRXPath, "%s", m_gamepathstr);
	if(m_joinstartup) {
		strcpy(user.joinatstart, "true");
		joinflg = true;
	}
	else {
		strcpy(user.joinatstart, "false");
		joinflg = false;
	}

	if(!strcmp(user.nick, "Player") || !user.nick[0]) {
		AfxMessageBox("You must choose a real name!");
		return;
	}

	WritePrivateProfileString("Galaxy", "name", user.nick, "galaxy.ini");
	WritePrivateProfileString("Galaxy", "email", user.email, "galaxy.ini");
	WritePrivateProfileString("Galaxy", "chatstart", user.joinatstart, "galaxy.ini");
	WritePrivateProfileString("Galaxy", "exe", CRXPath, "galaxy.ini");

	CDialog::OnOK();
}

void PlayerProfile::OnJoinatstartup() 
{
	m_joinstartup = m_joinstartupctrl.GetCheck();
}



void PlayerProfile::OnSelchangeIrcserver() 
{
	int index;

	index = m_ircselectserver.GetCurSel();

	switch(index) {
	case 0:
		WritePrivateProfileString("Galaxy", "server", "irc.blackened.com", "galaxy.ini");
		strcpy(servidor, "irc.blackened.com");
		break;
	case 1:
		WritePrivateProfileString("Galaxy", "server", "irc.prison.net", "galaxy.ini");
		strcpy(servidor, "irc.prison.net");
		break;
	case 2:
		WritePrivateProfileString("Galaxy", "server", "irc.anynet.org", "galaxy.ini");
		strcpy(servidor, "irc.anynet.org");
		break;
	default:
		WritePrivateProfileString("Galaxy", "server", "irc.blackened.com", "galaxy.ini");
		strcpy(servidor, "irc.blackened.com");
	}
}
