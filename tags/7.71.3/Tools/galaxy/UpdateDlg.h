#if !defined(AFX_UPDATEDLG_H__4FF53EDB_8819_489F_A0CB_6B8678655EFE__INCLUDED_)
#define AFX_UPDATEDLG_H__4FF53EDB_8819_489F_A0CB_6B8678655EFE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UpdateDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// UpdateDlg dialog

class UpdateDlg : public CDialog
{
// Construction
public:
	UpdateDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(UpdateDlg)
	enum { IDD = IDD_UPDATER };
		// NOTE: the ClassWizard will add data members here
	CProgressCtrl	m_updateprogress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(UpdateDlg)
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(UpdateDlg)
	afx_msg void DownloadUpdates();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UPDATEDLG_H__4FF53EDB_8819_489F_A0CB_6B8678655EFE__INCLUDED_)
