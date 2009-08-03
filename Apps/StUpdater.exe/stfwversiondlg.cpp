// StFwVersionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StBase.h"
#include "StGlobals.h"
#include "ddildl_defs.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "StUpdater.h"
#include "StConfigInfo.h"
#include "StCmdlineProcessor.h"
#include "StLogger.h"
//#include <winioctl.h>
#include "StScsi.h"
#include "StError.h"
#include <ntddscsi.h>

//#include <scsidefs.h>
//#include <wnaspi32.h>

#include "StScsi_Nt.h"
#include "StFwComponent.h"
#include "StSystemDrive.h"
#include "StDataDrive.h"
#include "StHiddenDataDrive.h"
#include "StProgress.h"
#include "StUsbMscDev.h"
#include "StResource.h"
#include "StUpdaterApp.h"
#include "StFwVersionDlg.h"
#include "CustomSupport.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COLUMN_DESC		0
#define COLUMN_VERSION	1
#define COLUMN_PATHNAME	2

/////////////////////////////////////////////////////////////////////////////
// CStFwVersionDlg dialog


CStFwVersionDlg::CStFwVersionDlg(
	CStVersionInfoPtrArray* _p_arr_current_component_vers,
	CStVersionInfoPtrArray* _p_arr_upgrade_component_vers,
	CStConfigInfo* _p_config_info,
	CWnd* _pParent /*=NULL*/)
	: CDialog(CStFwVersionDlg::IDD, _pParent)
{
	//{{AFX_DATA_INIT(CStFwVersionDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_p_arr_current_component_vers	= _p_arr_current_component_vers;
	m_p_arr_upgrade_component_vers	= _p_arr_upgrade_component_vers;
	m_p_config_info					= _p_config_info;

}


void CStFwVersionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStFwVersionDlg)
	DDX_Control(pDX, IDC_CURRENT_VERSION_LIST, m_CurrentVersionListCtrl);
	DDX_Control(pDX, IDC_UPGRADE_VERSION_LIST, m_UpgradeVersionListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStFwVersionDlg, CDialog)
	//{{AFX_MSG_MAP(CStFwVersionDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStFwVersionDlg message handlers

BOOL CStFwVersionDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetupDisplay();
			
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CStFwVersionDlg::SetupDisplay()
{
	UCHAR total_drives;
	CString str;
    UCHAR booty_drive_index=0;
	CStResource * p_resource = ((CStUpdaterApp*)AfxGetApp())->GetResource();
    
    m_p_config_info->GetBootyDriveIndex(booty_drive_index);

	// set Title;
	p_resource->GetResourceString(IDS_FWVERDLG_TITLE, str);
	SetWindowText(str);

	//set other window texts.
	p_resource->GetResourceString(IDS_FWVERDLG_CURR_VERSION, str);
	SetDlgItemText(IDC_CURRENT_VERSION, str);

	p_resource->GetResourceString(IDS_FWVERDLG_UPGRADE_VERSION, str);
	SetDlgItemText(IDC_UPGRADE_VERSION, str);

	//
	// Insert columns into the list control
	//
	// What size do we make them?
	//
	// Need to take account of the width of the vertical scrollbar
	// that windows inserts if the number of items exceeds what can
	// be displayed.  This prevents the ugly horizontal scrollbar from appearing.
	//
	RECT listRect;
	m_CurrentVersionListCtrl.GetClientRect(&listRect);
	int nListWidth = (listRect.right - listRect.left);
	int nVScrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL); 
	
	int nCol1Width = (int) ((double)nListWidth * 0.5);  
	int nCol2Width = (int) nListWidth - nCol1Width - nVScrollBarWidth; 
	int nCol3Width = 0;

	p_resource->GetResourceString(IDS_COLUMN_DESC, str);
	m_CurrentVersionListCtrl.InsertColumn(COLUMN_DESC, str, LVCFMT_LEFT, nCol1Width);
	p_resource->GetResourceString(IDS_COLUMN_VERSION, str);
	m_CurrentVersionListCtrl.InsertColumn(COLUMN_VERSION, str, LVCFMT_LEFT, nCol2Width);

	m_UpgradeVersionListCtrl.GetClientRect(&listRect);
	nListWidth = (listRect.right - listRect.left);

	nCol1Width = (int) ((double)nListWidth * 0.27);  
	nCol2Width = (int) ((double)nListWidth * 0.20);  
    nCol3Width = (int) nListWidth - nCol1Width - nCol2Width - nVScrollBarWidth; 

	p_resource->GetResourceString(IDS_COLUMN_DESC, str);
	m_UpgradeVersionListCtrl.InsertColumn(COLUMN_DESC, str, LVCFMT_LEFT, nCol1Width);
	p_resource->GetResourceString(IDS_COLUMN_VERSION, str);
	m_UpgradeVersionListCtrl.InsertColumn(COLUMN_VERSION, str, LVCFMT_LEFT, nCol2Width);
	p_resource->GetResourceString(IDS_COLUMN_FILENAME, str);
	m_UpgradeVersionListCtrl.InsertColumn(COLUMN_PATHNAME, str, LVCFMT_LEFT, nCol3Width);

	//
	// Set full row selection
	//
	m_CurrentVersionListCtrl.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
	m_UpgradeVersionListCtrl.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	m_CurrentVersionListCtrl.EnableWindow(TRUE);
	m_UpgradeVersionListCtrl.EnableWindow(TRUE);

	p_resource->GetResourceString(IDS_FWVERDLG_OK, str);
	SetDlgItemText(IDOK, str);
	
	//
	// Finally, insert version information into the list controls.
	//
	m_p_config_info->GetNumSystemDrives(total_drives);

	for(UCHAR drive=0; drive<total_drives; drive++)
	{
		SetupDriveDetails(drive+booty_drive_index);
	}
}	

void CStFwVersionDlg::SetupDriveDetails(UCHAR _drive_num)
{
	CStVersionInfo* curr_ver = NULL;
	CStVersionInfo* upgrade_ver = NULL;
	wchar_t curr_dir[_MAX_PATH]; 
	wstring wstr_drive_desc;
	CString filepath, cur_ver_str, upgrade_ver_str;
	string filename;
    UCHAR booty_index=0;

    m_p_config_info->GetBootyDriveIndex(booty_index);

	cur_ver_str = "0.0.0";
	upgrade_ver_str = "0.0.0";

	::GetCurrentDirectory(_MAX_PATH, curr_dir);
	m_p_config_info->GetSystemDriveName(_drive_num, filename);

	if ( m_p_arr_current_component_vers )
		curr_ver = *m_p_arr_current_component_vers->GetAt(_drive_num-booty_index);

	if ( m_p_arr_upgrade_component_vers )
		upgrade_ver = *m_p_arr_upgrade_component_vers->GetAt(_drive_num-booty_index);

	m_p_config_info->GetSystemDriveDescription(_drive_num, wstr_drive_desc);
	
	filepath = CString(curr_dir) + CString("\\") + CString(filename.c_str());

/*	if ( m_p_config_info->HasCustomSupport() )
	{
        CCustomSupport support;
		
		if ( curr_ver )
			cur_ver_str = support.GetVersionString(curr_ver);
		if ( upgrade_ver )
			upgrade_ver_str = support.GetVersionString(upgrade_ver);
	}
	else
*/
//	{
		if ( curr_ver )
			cur_ver_str = curr_ver->GetVersionString().c_str();
		if ( upgrade_ver )
			upgrade_ver_str = upgrade_ver->GetVersionString().c_str();
//	}

	if ( upgrade_ver && upgrade_ver->m_loaded_from_resource )
		filepath.Format(L"< %s >", CString(filename.c_str()));

	int currentItem = m_CurrentVersionListCtrl.GetItemCount();
	m_CurrentVersionListCtrl.InsertItem(currentItem, wstr_drive_desc.c_str());
    m_CurrentVersionListCtrl.SetItemText(currentItem, COLUMN_VERSION, cur_ver_str);

	int upgradeItem = m_UpgradeVersionListCtrl.GetItemCount();
	m_UpgradeVersionListCtrl.InsertItem(upgradeItem, wstr_drive_desc.c_str());
	m_UpgradeVersionListCtrl.SetItemText(upgradeItem, COLUMN_VERSION, upgrade_ver_str);
	m_UpgradeVersionListCtrl.SetItemText(upgradeItem, COLUMN_PATHNAME, filepath);
}
	
