#if !defined(AFX_PLAYERPROFILE_H__3E1FD51E_6213_437D_9A2B_96C43A072E75__INCLUDED_)
#define AFX_PLAYERPROFILE_H__3E1FD51E_6213_437D_9A2B_96C43A072E75__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PlayerProfile.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PlayerProfile dialog

class PlayerProfile : public CDialog
{
// Construction
public:
	PlayerProfile(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(PlayerProfile)
	enum { IDD = IDD_SETPROFILE };
	CComboBox	m_ircselectserver;
	CEdit	m_gamepathctrl;
	CButton	m_joinstartupctrl;
	CEdit	m_playernamectrl;
	CEdit	m_playeremailctrl;
	CString	m_playeremailstr;
	CString	m_playernamestr;
	BOOL	m_joinstartup;
	CString	m_gamepathstr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PlayerProfile)
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PlayerProfile)
	afx_msg void OnChangePlayername();
	afx_msg void OnChangePlayeremail();
	virtual void OnOK();
	afx_msg void OnJoinatstartup();
	afx_msg void OnChangeGamepath();
	afx_msg void OnSelchangeIrcserver();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PLAYERPROFILE_H__3E1FD51E_6213_437D_9A2B_96C43A072E75__INCLUDED_)
