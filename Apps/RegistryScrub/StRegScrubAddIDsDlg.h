#pragma once


// StRegScrubAddIDsDlg dialog

class CStRegScrubAddIDsDlg : public CDialog
{
	DECLARE_DYNAMIC(CStRegScrubAddIDsDlg)

public:
	CStRegScrubAddIDsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CStRegScrubAddIDsDlg();

	BOOL ValidateHexString(CString hexStr);
	void FixUpString(CString& str);

// Dialog Data
	enum { IDD = IDD_ADD_NEW_IDS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();


	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
