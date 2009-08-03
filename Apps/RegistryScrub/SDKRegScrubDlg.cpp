// SDKRegScrubDlg.cpp : implementation file
// Copyright (c) 2005 SigmaTel, Inc.

#include "stdafx.h"
#include <shlwapi.h>
#include ".\RegScrubList.h"
#include "SDKRegScrub.h"
#include "SDKRegScrubDlg.h"
#include ".\sdkregscrubdlg.h"
#include ".\stregscrubaddidsdlg.h"
#include ".\regscrub.h"
#include ".\version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

REMOVEDEVICEITEM	g_AddDeviceEntry;

// CAboutDlg dialog used for App About
/*****************************************************************************/
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
};

/*****************************************************************************/
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

/*****************************************************************************/
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	CString verstr;

    verstr.Format("Sigmatel Register Scrubber v%d.%d", VERSION_MAJOR, VERSION_MINOR);
	GetDlgItem(IDC_VERSION_STR)->SetWindowText( verstr );
	return TRUE;  
}
// CSDKRegScrubDlg dialog

/*****************************************************************************/
CSDKRegScrubDlg::CSDKRegScrubDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSDKRegScrubDlg::IDD, pParent)
{
    m_Scrubbing =false;
    m_pRegScrub = new CRegScrub(this);
	m_pOtherList = new CRegScrubList;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CSDKRegScrubDlg::~CSDKRegScrubDlg()
{
    if (m_pRegScrub)
	    delete m_pRegScrub;
    if (m_pOtherList)
	    delete m_pOtherList;
}

/*****************************************************************************/
void CSDKRegScrubDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_REG_DATA, m_RegData);
    DDX_Text(pDX, IDC_REG_DEVICE_COUNT, m_EntryRemovalCount);
    DDX_Control(pDX, IDC_REG_DATA, m_RegEditCtrl);
}


BEGIN_MESSAGE_MAP(CSDKRegScrubDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
   	ON_MESSAGE(WM_MSG_UPDATE_UI, OnMsgUpdateUI)
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(ID_SCRUB, OnBnClickedScrub)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnBnClickedAddButton)
	ON_BN_CLICKED(IDC_OTHER_CHECK, OnBnClickedOtherCheck)
	ON_BN_CLICKED(IDC_OTHER_REMOVE, OnBnClickedOtherRemove)
END_MESSAGE_MAP()

// CSDKRegScrubDlg message handlers

/*****************************************************************************/
BOOL CSDKRegScrubDlg::OnInitDialog()
{

	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_EntryRemovalCount = 0;
	
	m_DeletingKeyStr = (_T(""));
	m_ProcessingKeyStr = (_T(""));
	
	GetParameters();

    UpdateData(FALSE);	
	
    return TRUE;  // return TRUE  unless you set the focus to a control
}

/*****************************************************************************/
void CSDKRegScrubDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
/*****************************************************************************/
void CSDKRegScrubDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
/******************************************************************************/
HCURSOR CSDKRegScrubDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/*****************************************************************************/
void CSDKRegScrubDlg::OnBnClickedCancel()
{
    if(m_Scrubbing)
    {
	    if(AfxMessageBox("  Registry Scrubbing in progress.\r\n"
		                 "\r\n Are you sure you want to abort?",					
		                  MB_ICONSTOP|MB_OKCANCEL)==IDCANCEL)
	        return;
    }

    OnCancel();
}

/*****************************************************************************/
void CSDKRegScrubDlg::OnBnClickedScrub()
{
    CButton *SDKBtn, *SDKRecoveryBtn,*MfgRecoveryBtn, *OtherBtn;
   	
    m_LineCount=0;
    m_EntryRemovalCount=0;

    m_RegData=(_T(""));

    m_Scrubbing =true;
    m_pRegScrub = new CRegScrub(this);
	if (m_pRegScrub == NULL)
		return;

    GetDlgItem(ID_SCRUB)->EnableWindow(FALSE);    

    SDKBtn=(CButton*)GetDlgItem(IDC_SDK_CHECK);
    SDKRecoveryBtn=(CButton*)GetDlgItem(IDC_RECOVERY_CHECK);
    MfgRecoveryBtn=(CButton*)GetDlgItem(IDC_MFG_CHECK);
    OtherBtn=(CButton*)GetDlgItem(IDC_OTHER_CHECK);

    if(SDKBtn->GetState()==BST_CHECKED)
    {
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MSCN"), _T("MSCN"), _T("066F"), _T("8000"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MSCNMMC"), _T("MSCNMMC"), _T("066F"), _T("8004"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MTPMSCN"), _T("MTPMSCN"), _T("066F"), _T("A010"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MTPMSCNMMC"), _T("MTPMSCNMMC"), _T("066F"), _T("A012"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("SDK_DEVICE"), _T("SDK_DEVICE"), _T("066F"), _T("A010"));
	}

	if(SDKRecoveryBtn->GetState()==BST_CHECKED)
    {
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MSCN"), _T("MSCN"), _T("066F"), _T("3410"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MSCN"), _T("MSCN"), _T("066F"), _T("3500"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MSCN"), _T("MSCN"), _T("066F"), _T("3600"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MSCN"), _T("MSCN"), _T("066F"), _T("3700"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MSCN"), _T("MSCN"), _T("066F"), _T("3770"));
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("MSCN"), _T("MSCN"), _T("066F"), _T("3780"));
        m_pRegScrub->InsertItem(_T("_Generic"), _T("_Generic"), _T("MSC_Recovery"), _T("MSC_Recovery"), _T("066F"), _T("A000"));
        m_pRegScrub->InsertItem(_T("_Generic"), _T("_Generic"), _T("MSC_Recovery"), _T("MSC_Recovery"), _T("066F"), _T("37FF"));
    }
    if(MfgRecoveryBtn->GetState()==BST_CHECKED)
    {
        m_pRegScrub->InsertItem(_T("SigmaTel"), _T("SigmaTel"), _T("STMFGTOOL"), _T("STMFGTOOL"), _T("066F"), _T("0000"));
    }

    if(OtherBtn->GetState()==BST_CHECKED)
	{
		PREMOVEDEVICEITEM pOtherItem = m_pOtherList->GetHead();
		while (pOtherItem)
		{
			m_pRegScrub->InsertItem(pOtherItem->OrgMfg, pOtherItem->Mfg,
								pOtherItem->OrgProduct, pOtherItem->Product,
								pOtherItem->USBVid, pOtherItem->USBPid);
				
			pOtherItem = m_pOtherList->GetNext(pOtherItem);
		}
	}

	if (m_pRegScrub->GetDeviceCount())
	{
        SaveParameters();
		m_pRegScrub->Clean();
	}
	else
		::MessageBox(NULL, "Please specify devices to remove", "SDKRegScrub", MB_OK);

	GetDlgItem(ID_SCRUB)->EnableWindow(TRUE);
     
    m_Scrubbing =false;
    
}

/*****************************************************************************/
LRESULT CSDKRegScrubDlg::OnMsgUpdateUI(WPARAM _event_type, LPARAM _msg_string)
{
	bool SleepMore=false;

	if(m_DeletingKeyStr!=m_pRegScrub->GetDeletingKeyStr())
		SleepMore=true;

	m_EntryRemovalCount=m_pRegScrub->GetEntryRemovalCount();
	m_DeletingKeyStr=m_pRegScrub->GetDeletingKeyStr();

    if(m_DeletingKeyStr.GetLength()>0)
    {
        m_RegData+="~~~Deleting: ";
        m_ProcessingKeyStr=m_pRegScrub->GetProcessingKeyStr();
        m_RegData+=m_ProcessingKeyStr;
    }
    else 
    {
        m_ProcessingKeyStr.Format("#%03i: %s",++m_LineCount,m_pRegScrub->GetProcessingKeyStr());
        m_RegData+=m_ProcessingKeyStr;
    }

    m_RegData+="\r\n";

    UpdateData(FALSE);

    int len = m_RegData.GetLength();
    m_RegEditCtrl.SetFocus();
    m_RegEditCtrl.SetSel(len,len);

    MSG msg;
    while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) 
    { 
        if ( !AfxGetApp()->PumpMessage( ) ) 
        { 
            ::PostQuitMessage( 0 ); 
            return FALSE;
        } 
    } 

//	if(SleepMore)
//		Sleep(2000);
//	else
		Sleep(350);

	return true;
}





void CSDKRegScrubDlg::OnBnClickedOtherCheck()
{
	// enable the add and remove (if list not empty) buttons
	// or disable them if we are currently enabled.
    CButton *OtherBtn =(CButton*)GetDlgItem(IDC_OTHER_CHECK);
	CButton *AddBtn = (CButton*)GetDlgItem(IDC_OTHER_ADD);
	CButton *RemoveBtn = (CButton*)GetDlgItem(IDC_OTHER_REMOVE);
	CListBox *OtherList = (CListBox*)GetDlgItem(IDC_OTHER_LIST);

    if(OtherBtn->GetCheck()==BST_CHECKED)
	{
//		OtherBtn->SetCheck(BST_UNCHECKED);
		AddBtn->EnableWindow(TRUE);
		RemoveBtn->EnableWindow(TRUE);
		OtherList->EnableWindow(TRUE);

	}
	else
	{
//		OtherBtn->SetCheck(BST_CHECKED);
		AddBtn->EnableWindow(FALSE);
		RemoveBtn->EnableWindow(FALSE);
		OtherList->EnableWindow(FALSE);
	}

}


void CSDKRegScrubDlg::OnBnClickedAddButton()
{
	CStRegScrubAddIDsDlg dlg;
	INT_PTR nResponse = dlg.DoModal();

	if (nResponse == IDOK)
	{
		// insert new item
		CString InsertText;
		int index;
		PREMOVEDEVICEITEM pItem;
		CDC*  pDC;
		CSize   sz;

		CListBox*	pListBox = (CListBox*)GetDlgItem(IDC_OTHER_LIST);
		pDC = pListBox->GetDC();
		InsertText.Format(_T("%s %s <USB\\Vid_%s&Pid_%s>"),
						g_AddDeviceEntry.OrgMfg, g_AddDeviceEntry.OrgProduct,
						g_AddDeviceEntry.USBVid, g_AddDeviceEntry.USBPid);
		sz = pDC->GetTextExtent(InsertText);
		pListBox->ReleaseDC(pDC);

		if (pListBox->GetHorizontalExtent() < sz.cx)
		{
			pListBox->SetHorizontalExtent(sz.cx);
			ASSERT(pListBox->GetHorizontalExtent() == sz.cx);
		}

		pItem = m_pOtherList->InsertItem (	g_AddDeviceEntry.OrgMfg,
											g_AddDeviceEntry.Mfg,
											g_AddDeviceEntry.OrgProduct,
											g_AddDeviceEntry.Product,
											g_AddDeviceEntry.USBVid,
											g_AddDeviceEntry.USBPid);

		if (pItem)
		{
			index = pListBox->InsertString(-1, InsertText);
			pListBox->SetItemDataPtr(index, pItem);
		}
	}

}


void CSDKRegScrubDlg::OnBnClickedOtherRemove()
{
		PREMOVEDEVICEITEM pItem;
		CListBox*	pListBox = (CListBox*)GetDlgItem(IDC_OTHER_LIST);

		pItem = (PREMOVEDEVICEITEM) pListBox->GetItemDataPtr( pListBox->GetCurSel() );
		m_pOtherList->RemoveItem(pItem);
		pListBox->DeleteString( pListBox->GetCurSel() );
}


BOOL CSDKRegScrubDlg::GetParameters()
{
    CButton *SDKBtn, *SDKRecoveryBtn,*MfgRecoveryBtn, *OtherBtn;
	CButton *AddBtn, *RemoveBtn;
	HKEY hRegKey;
	TCHAR regPath[128];
	LONG result;
	DWORD dwRegValue;
	DWORD dwType, dwDataSize;

    SDKBtn=(CButton*)GetDlgItem(IDC_SDK_CHECK);
    SDKRecoveryBtn=(CButton*)GetDlgItem(IDC_RECOVERY_CHECK);
    MfgRecoveryBtn=(CButton*)GetDlgItem(IDC_MFG_CHECK);
    OtherBtn=(CButton*)GetDlgItem(IDC_OTHER_CHECK);
	AddBtn = (CButton*)GetDlgItem(IDC_OTHER_ADD);
	RemoveBtn = (CButton*)GetDlgItem(IDC_OTHER_REMOVE);

	_tcscpy_s(regPath, 128, "SOFTWARE\\Sigmatel\\SDKRegScrub");

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_ALL_ACCESS, &hRegKey);

	if (result != ERROR_SUCCESS)
	{
		AddBtn->EnableWindow(FALSE);
		RemoveBtn->EnableWindow(FALSE);
		return FALSE;
	}

	dwType = REG_DWORD;
	RegQueryValueEx(hRegKey, "SDKEntries", 0, &dwType, (LPBYTE)&dwRegValue, &dwDataSize);

	SDKBtn->SetCheck((BOOL)dwRegValue ?  BST_CHECKED:BST_UNCHECKED );

	RegQueryValueEx(hRegKey, "SDKRecvEntries", 0, &dwType, (LPBYTE)&dwRegValue, &dwDataSize);

    SDKRecoveryBtn->SetCheck((BOOL)dwRegValue ?  BST_CHECKED:BST_UNCHECKED );

	RegQueryValueEx(hRegKey, "MfgEntries", 0, &dwType, (LPBYTE)&dwRegValue, &dwDataSize);

    MfgRecoveryBtn->SetCheck((BOOL)dwRegValue ?  BST_CHECKED:BST_UNCHECKED );

	RegQueryValueEx(hRegKey, "OtherEntries", 0, &dwType, (LPBYTE)&dwRegValue, &dwDataSize);

    OtherBtn->SetCheck((BOOL)dwRegValue ?  BST_CHECKED:BST_UNCHECKED );
	if (OtherBtn->GetState() != BST_CHECKED)
	{
		AddBtn->EnableWindow(FALSE);
		RemoveBtn->EnableWindow(FALSE);
	}
	else
	{
		TCHAR OrgMfg[32];
		TCHAR Mfg[32];
		TCHAR OrgProduct[32];
		TCHAR Product[32];
		TCHAR USBVid[16];
		TCHAR USBPid[16];
		TCHAR enumKey[8];
		DWORD dwDataSize;
		FILETIME lastTime;
		UINT i = 0;
		LONG status;
		HKEY hRegOtherKey, hRegOtherDeviceKey;
		CListBox*	pListBox = (CListBox*)GetDlgItem(IDC_OTHER_LIST);
		PREMOVEDEVICEITEM pItem;
		CString InsertText;

		if (RegOpenKeyEx(hRegKey, "OtherDevices", 0, KEY_ALL_ACCESS, &hRegOtherKey) == ERROR_SUCCESS)
		{

			do
			{
				dwDataSize = 8;

				status = RegEnumKeyEx(hRegOtherKey,     // handle to key to query
							i, // index of subkey to query
							enumKey, // address of buffer for subkey name
							&dwDataSize,
							NULL, NULL, NULL,&lastTime);
				++i;

				if (status == ERROR_SUCCESS)
				{
					if (RegOpenKey (hRegOtherKey, enumKey, &hRegOtherDeviceKey) == ERROR_SUCCESS)
					{
						LONG queryStatus;
						DWORD typeValue = REG_SZ;
				
						dwDataSize = 32;
						queryStatus = RegQueryValueEx(
								hRegOtherDeviceKey,			// handle to key to query
								"OrgMfg",		// name of value to query
								NULL,				// reserved
								&typeValue,			// address of buffer for value type
								(PUCHAR)OrgMfg,  // address of data buffer
								&dwDataSize			// address of data buffer size
								);

						dwDataSize = 32;
						queryStatus = RegQueryValueEx(
								hRegOtherDeviceKey,			// handle to key to query
								"Mfg",		// name of value to query
								NULL,				// reserved
								&typeValue,			// address of buffer for value type
								(PUCHAR)Mfg,  // address of data buffer
								&dwDataSize			// address of data buffer size
								);

						dwDataSize = 32;
						queryStatus = RegQueryValueEx(
								hRegOtherDeviceKey,			// handle to key to query
								"OrgProd",		// name of value to query
								NULL,				// reserved
								&typeValue,			// address of buffer for value type
								(PUCHAR)OrgProduct,  // address of data buffer
								&dwDataSize			// address of data buffer size
								);

						dwDataSize = 32;
						queryStatus = RegQueryValueEx(
								hRegOtherDeviceKey,			// handle to key to query
								"Prod",		// name of value to query
								NULL,				// reserved
								&typeValue,			// address of buffer for value type
								(PUCHAR)Product,  // address of data buffer
								&dwDataSize			// address of data buffer size
								);
 
						dwDataSize = 16;
						queryStatus = RegQueryValueEx(
								hRegOtherDeviceKey,			// handle to key to query
								"Vid",		// name of value to query
								NULL,				// reserved
								&typeValue,			// address of buffer for value type
								(PUCHAR)USBVid,  // address of data buffer
								&dwDataSize			// address of data buffer size
								);

						dwDataSize = 16;
						queryStatus = RegQueryValueEx(
								hRegOtherDeviceKey,			// handle to key to query
								"Pid",		// name of value to query
								NULL,				// reserved
								&typeValue,			// address of buffer for value type
								(PUCHAR)USBPid,  // address of data buffer
								&dwDataSize			// address of data buffer size
								);

						RegCloseKey(hRegOtherDeviceKey);

						pItem = m_pOtherList->InsertItem(OrgMfg, Mfg, OrgProduct, Product, USBVid, USBPid);
						if (pItem)
						{	int index;
							CDC*  pDC;
							CSize   sz;

							pDC = pListBox->GetDC();

							InsertText.Format(_T("%s %s <USB\\Vid_%s&Pid_%s>"), pItem->OrgMfg, pItem->OrgProduct, pItem->USBVid, pItem->USBPid);

							sz = pDC->GetTextExtent(InsertText);
							pListBox->ReleaseDC(pDC);

							if (pListBox->GetHorizontalExtent() < sz.cx)
							{
								pListBox->SetHorizontalExtent(sz.cx);
								ASSERT(pListBox->GetHorizontalExtent() == sz.cx);
							}
							index = pListBox->InsertString(-1, InsertText);
							pListBox->SetItemDataPtr(index, pItem);
						}

					}
				}
			}while (status == ERROR_SUCCESS);

			RegCloseKey (hRegOtherKey);
		}
	}

	RegCloseKey(hRegKey);

	return (TRUE);
}

BOOL CSDKRegScrubDlg::SaveParameters()
{
    CButton *SDKBtn, *SDKRecoveryBtn,*MfgRecoveryBtn, *OtherBtn;
	HKEY hRegKey, hRegOtherKey, hRegOtherDeviceKey;
	TCHAR regPath[128];
	LONG result;
	DWORD dwRegValue;
	DWORD dwDisposition;

    SDKBtn=(CButton*)GetDlgItem(IDC_SDK_CHECK);
    SDKRecoveryBtn=(CButton*)GetDlgItem(IDC_RECOVERY_CHECK);
    MfgRecoveryBtn=(CButton*)GetDlgItem(IDC_MFG_CHECK);
    OtherBtn=(CButton*)GetDlgItem(IDC_OTHER_CHECK);

	_tcscpy_s (regPath, 128, "SOFTWARE\\Sigmatel\\SDKRegScrub");

	result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, NULL, REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS, NULL, &hRegKey, &dwDisposition);
 
	if (result != ERROR_SUCCESS)
	{
		::MessageBox(NULL, "Unable to create/open registry key", "SDKRegScrub", MB_OK);
		return FALSE;
	}

    if(SDKBtn->GetState()==BST_CHECKED)
		dwRegValue = TRUE;
	else
		dwRegValue = FALSE;

	RegSetValueEx(hRegKey, "SDKEntries", 0, REG_DWORD, (LPBYTE)&dwRegValue, sizeof(DWORD));

    if(SDKRecoveryBtn->GetState()==BST_CHECKED)
		dwRegValue = TRUE;
	else
		dwRegValue = FALSE;
	RegSetValueEx(hRegKey, "SDKRecvEntries", 0, REG_DWORD, (LPBYTE)&dwRegValue, sizeof(DWORD));

    if(MfgRecoveryBtn->GetState()==BST_CHECKED)
		dwRegValue = TRUE;
	else
		dwRegValue = FALSE;
	RegSetValueEx(hRegKey, "MfgEntries", 0, REG_DWORD, (LPBYTE)&dwRegValue, sizeof(DWORD));

	// Delete the OtherDevices key and all it's decendents
	SHDeleteKey(hRegKey, "OtherDevices");

	if(OtherBtn->GetState()==BST_CHECKED && m_pOtherList->GetDeviceCount())
	{
		TCHAR szStr[32];
		PREMOVEDEVICEITEM pOtherDevice;
		UINT otherCount = m_pOtherList->GetDeviceCount();
		dwRegValue = TRUE;

		RegCreateKeyEx(hRegKey, "OtherDevices", 0, NULL, REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS, NULL, &hRegOtherKey, &dwDisposition);

		pOtherDevice = m_pOtherList->GetHead();

		for (UINT i = 0; i < otherCount; ++i)
		{
			_stprintf_s(szStr, 32, "%d", i);

			RegCreateKeyEx(hRegOtherKey, szStr, 0, NULL, REG_OPTION_NON_VOLATILE,
					KEY_ALL_ACCESS, NULL, &hRegOtherDeviceKey, &dwDisposition);


			_tcscpy_s(szStr, 32, pOtherDevice->OrgMfg);
			RegSetValueEx(hRegOtherDeviceKey, "OrgMfg", 0, REG_SZ, (LPBYTE)szStr, pOtherDevice->OrgMfg.GetLength());
			_tcscpy_s(szStr, 32, pOtherDevice->Mfg);
			RegSetValueEx(hRegOtherDeviceKey, "Mfg", 0, REG_SZ, (LPBYTE)szStr, pOtherDevice->Mfg.GetLength());

			_tcscpy_s(szStr, 32, pOtherDevice->OrgProduct);
			RegSetValueEx(hRegOtherDeviceKey, "OrgProd", 0, REG_SZ, (LPBYTE)szStr, pOtherDevice->OrgProduct.GetLength());
			_tcscpy_s(szStr, 32, pOtherDevice->Product);
			RegSetValueEx(hRegOtherDeviceKey, "Prod", 0, REG_SZ, (LPBYTE)szStr, pOtherDevice->Product.GetLength());

			_tcscpy_s(szStr, 32, pOtherDevice->USBVid);
			RegSetValueEx(hRegOtherDeviceKey, "Vid", 0, REG_SZ, (LPBYTE)szStr, pOtherDevice->USBVid.GetLength());

			_tcscpy_s(szStr, 32, pOtherDevice->USBPid);
			RegSetValueEx(hRegOtherDeviceKey, "Pid", 0, REG_SZ, (LPBYTE)szStr, pOtherDevice->USBPid.GetLength());

			RegCloseKey(hRegOtherDeviceKey);
			pOtherDevice = m_pOtherList->GetNext(pOtherDevice);
		}
		RegCloseKey(hRegOtherKey);

	}
	else
		dwRegValue = FALSE;

	RegSetValueEx(hRegKey, "OtherEntries", 0, REG_DWORD, (LPBYTE)&dwRegValue, sizeof(DWORD));

	RegCloseKey(hRegKey);

	return TRUE;
}