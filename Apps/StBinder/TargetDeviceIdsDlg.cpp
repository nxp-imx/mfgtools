// TargetDeviceIdsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StBinder.h"
#include "TargetDeviceIdsDlg.h"

extern TARGET_CFG_DATA g_ResCfgData;


// CTargetDeviceIdsDlg dialog

IMPLEMENT_DYNAMIC(CTargetDeviceIdsDlg, CPropertyPage)

CTargetDeviceIdsDlg::CTargetDeviceIdsDlg( /*CWnd* pParent =NULL*/)
	: CPropertyPage(CTargetDeviceIdsDlg::IDD)
{

}

CTargetDeviceIdsDlg::~CTargetDeviceIdsDlg()
{
}

void CTargetDeviceIdsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TARGET_IDS_SCSIMFG, m_SCSIMfgId);
	DDX_Control(pDX, IDC_TARGET_IDS_SCSIPROD, m_SCSIProdId);
	DDX_Control(pDX, IDC_TARGET_IDS_MTPMFG, m_MTPMfgId);
	DDX_Control(pDX, IDC_TARGET_IDS_MTPPROD, m_MTPProdId);
	DDX_Control(pDX, IDC_TARGET_IDS_USBVENDOR, m_USBVendorId);
	DDX_Control(pDX, IDC_TARGET_IDS_USBPROD, m_USBProdId);
	DDX_Control(pDX, IDC_TARGET_IDS_USBPROD2, m_SecondaryUSBProdId);
}


BEGIN_MESSAGE_MAP(CTargetDeviceIdsDlg, CPropertyPage)
END_MESSAGE_MAP()

BOOL CTargetDeviceIdsDlg::OnInitDialog()
{
	CString resStr;

	CPropertyPage::OnInitDialog();

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_IDS_SCSI_MFG_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_IDS_SCSI_MFG_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_IDS_SCSI_PROD_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_IDS_SCSI_PROD_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_IDS_MTP_MFG_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_IDS_MTP_MFG_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_IDS_MTP_PROD_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_IDS_MTP_PROD_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_IDS_USB_VENDOR_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_IDS_USB_VENDOR_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_IDS_USB_PROD_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_IDS_USB_PROD_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_IDS_SECOND_USB_PROD_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_IDS_USB_PROD2_TEXT, resStr );

	m_SCSIMfgId.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_SCSIMfgId.SetTextLenMinMax(1,SCSIMFG_MAX);
	m_SCSIMfgId.SetEnableLengthCheck();
	m_SCSIMfgId.SetMyControlID(0x01);
	m_SCSIMfgId.SetWindowTextW(g_ResCfgData.Options.SCSIMfg);

	m_SCSIProdId.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_SCSIProdId.SetTextLenMinMax(1,SCSIPROD_MAX);
	m_SCSIProdId.SetEnableLengthCheck();
	m_SCSIProdId.SetMyControlID(0x02);
	m_SCSIProdId.SetWindowTextW(g_ResCfgData.Options.SCSIProd);

	m_MTPMfgId.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_MTPMfgId.SetTextLenMinMax(1,MTPMFG_MAX);
	m_MTPMfgId.SetEnableLengthCheck();
	m_MTPMfgId.SetMyControlID(0x03);
	m_MTPMfgId.SetWindowTextW(g_ResCfgData.Options.MtpMfg);

	m_MTPProdId.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_MTPProdId.SetTextLenMinMax(1,MTPPROD_MAX);
	m_MTPProdId.SetEnableLengthCheck();
	m_MTPProdId.SetMyControlID(0x04);
	m_MTPProdId.SetWindowTextW(g_ResCfgData.Options.MtpProd);

	m_USBVendorId.SetValidCharSet(CVisValidEdit::SET_HEXADECIMAL);
	m_USBVendorId.SetTextLenMinMax(USBVENDOR_MAX,USBVENDOR_MAX);
	m_USBVendorId.SetEnableLengthCheck();
	m_USBVendorId.SetMyControlID(0x05);
	resStr.Format(_T("%04X"), g_ResCfgData.Options.USBVendor);
	m_USBVendorId.SetWindowTextW(resStr);

	m_USBProdId.SetValidCharSet(CVisValidEdit::SET_HEXADECIMAL);
	m_USBProdId.SetTextLenMinMax(USBPROD_MAX,USBPROD_MAX);
	m_USBProdId.SetEnableLengthCheck();
	m_USBProdId.SetMyControlID(0x06);
	resStr.Format(_T("%04X"), g_ResCfgData.Options.USBProd);
	m_USBProdId.SetWindowTextW(resStr);

	m_SecondaryUSBProdId.SetValidCharSet(CVisValidEdit::SET_HEXADECIMAL);
	m_SecondaryUSBProdId.SetTextLenMinMax(USBPROD_MAX,USBPROD_MAX);
	m_SecondaryUSBProdId.SetEnableLengthCheck();
	m_SecondaryUSBProdId.SetMyControlID(0x06);
	if (g_ResCfgData.Options.SecondaryUSBProd)
	{
		resStr.Format(_T("%04X"), g_ResCfgData.Options.SecondaryUSBProd);
		m_SecondaryUSBProdId.SetWindowTextW(resStr);
	}

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// CTargetDeviceIdsDlg message handlers
void CTargetDeviceIdsDlg::OnOK()
{
	TCHAR tmpStr[USBVENDOR_MAX+1];

	m_SCSIMfgId.GetWindowTextW(g_ResCfgData.Options.SCSIMfg,SCSIMFG_MAX+1);
	m_SCSIProdId.GetWindowTextW(g_ResCfgData.Options.SCSIProd,SCSIPROD_MAX+1);
	m_MTPMfgId.GetWindowTextW(g_ResCfgData.Options.MtpMfg,MTPMFG_MAX+1);
	m_MTPProdId.GetWindowTextW(g_ResCfgData.Options.MtpProd,MTPPROD_MAX+1);
	m_USBVendorId.GetWindowTextW(tmpStr,USBVENDOR_MAX+1);
	_stscanf(tmpStr, _T("%04X"), &g_ResCfgData.Options.USBVendor);
	m_USBProdId.GetWindowTextW(tmpStr,USBPROD_MAX+1);
	_stscanf(tmpStr, _T("%04X"), &g_ResCfgData.Options.USBProd);
	if (m_SecondaryUSBProdId.GetWindowTextW(tmpStr,USBPROD_MAX+1))
		_stscanf(tmpStr, _T("%04X"), &g_ResCfgData.Options.SecondaryUSBProd);
	else
		g_ResCfgData.Options.SecondaryUSBProd = 0;

	CPropertyPage::OnOK();
}

void CTargetDeviceIdsDlg::OnCancel()
{
	CPropertyPage::OnCancel();
}
