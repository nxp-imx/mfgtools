/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// AttachDataDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceApi.h"
#include "AttachDataDlg.h"


// CAttachDataDlg dialog

IMPLEMENT_DYNAMIC(CAttachDataDlg, CDialog)

CAttachDataDlg::CAttachDataDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAttachDataDlg::IDD, pParent)
    , _fw()
    , _versionInfo()
{
}

CAttachDataDlg::~CAttachDataDlg()
{
    _returnData.clear();
}

void CAttachDataDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FIRMWARE_FILE_EMPTY_LINK, _fwFileEmptyLinkCtrl);
    DDX_Control(pDX, IDC_VERSION_INPUT_EDIT, _versionInputCtrl);
    DDX_Control(pDX, IDC_HEX_INPUT_EDIT, _hexInputCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_FILE_RADIO, _fwSourceFileCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_RESOURCE_RADIO, _fwSourceResourceCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_FILE_COMBO, _fwFileComboCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_RESOURCE_COMBO, _fwResourceComboCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_FILE_BROWSE_BUTTON, _fwFileBrowseCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_FILE_PATH_EDIT, _fwPathCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_TYPE_ID_TEXT, _fwTypeIdCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_COMPONENT_VERSION_TEXT, _fwComponentVersionCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_PRODUCT_VERSION_TEXT, _fwProductVersionCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_LANGUAGE_TEXT, _fwLanguageCtrl);
    DDX_Control(pDX, IDC_FIRMWARE_TAG_TEXT, _fwTagCtrl);
    DDX_Control(pDX, IDC_DATA_SIZE_TEXT, _sizeCtrl);
    DDX_Control(pDX, IDC_FW_TYPE_LABEL, _fwTypeIdLabel);
    DDX_Control(pDX, IDC_FW_COMP_VER_LABEL, _fwCompVerLabel);
    DDX_Control(pDX, IDC_PROD_VER_LABEL, _fwProdVerLabel);
    DDX_Control(pDX, IDC_FW_LANG_LABEL, _fwLangLabel);
    DDX_Control(pDX, IDC_FW_TAG_LABEL, _fwTagLabel);
}


BEGIN_MESSAGE_MAP(CAttachDataDlg, CDialog)
    ON_BN_CLICKED(IDC_SELECT_FIRMWARE_RADIO, &CAttachDataDlg::OnBnClickedSelectSourceRadio)
    ON_BN_CLICKED(IDC_SELECT_HEX_RADIO, &CAttachDataDlg::OnBnClickedSelectSourceRadio)
    ON_BN_CLICKED(IDC_SELECT_VERSION_RADIO, &CAttachDataDlg::OnBnClickedSelectSourceRadio)
    ON_BN_CLICKED(IDC_FIRMWARE_FILE_RADIO, &CAttachDataDlg::OnBnClickedFirmwareSourceRadio)
    ON_BN_CLICKED(IDC_FIRMWARE_RESOURCE_RADIO, &CAttachDataDlg::OnBnClickedFirmwareSourceRadio)
    ON_CBN_SELCHANGE(IDC_FIRMWARE_FILE_COMBO, &CAttachDataDlg::OnCbnSelchangeFirmwareFileCombo)
    ON_BN_CLICKED(IDC_FIRMWARE_FILE_BROWSE_BUTTON, &CAttachDataDlg::OnBnClickedFirmwareFileBrowseButton)
    ON_STN_CLICKED(IDC_FIRMWARE_FILE_EMPTY_LINK, &CAttachDataDlg::OnStnClickedFirmwareFileEmptyLink)
    ON_WM_DROPFILES()
    ON_BN_CLICKED(IDCANCEL, &CAttachDataDlg::OnBnClickedCancel)
    ON_EN_CHANGE(IDC_VERSION_INPUT_EDIT, &CAttachDataDlg::OnEnChangeVersionInputEdit)
END_MESSAGE_MAP()


// CAttachDataDlg message handlers

BOOL CAttachDataDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    int source = AfxGetApp()->GetProfileInt(_T("Attach Data Settings"), _T("Input Select"), IDC_SELECT_FIRMWARE_RADIO);
    CheckRadioButton(IDC_SELECT_FIRMWARE_RADIO, IDC_SELECT_VERSION_RADIO, source);
    source = AfxGetApp()->GetProfileInt(_T("Attach Data Settings"), _T("Firmware Source Select"), IDC_FIRMWARE_FILE_RADIO);
    CheckRadioButton(IDC_FIRMWARE_FILE_RADIO, IDC_FIRMWARE_RESOURCE_RADIO, /*source*/IDC_FIRMWARE_FILE_RADIO);    // disable resource input for now

 	// Combobox MRU initialization:
    _fwFileComboCtrl.SetMRURegKey ( _T("MRU") );
    _fwFileComboCtrl.SetMRUValueFormat ( _T("File #%d") );

    _fwFileComboCtrl.SetAutoSaveAfterAdd ( TRUE );

    _fwFileComboCtrl.LoadMRU();
    _fwFileComboCtrl.RefreshCtrl();

    CString lastFile = AfxGetApp()->GetProfileString(_T("Attach Data Settings"), _T("Last Firmware File"), _T(""));
    _fwFileComboCtrl.SelectString(0, lastFile);
	OnCbnSelchangeFirmwareFileCombo();

    OnBnClickedSelectSourceRadio();

	return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CAttachDataDlg::OnBnClickedSelectSourceRadio()
{
    CString sizeCtrlText;
	int source = GetCheckedRadioButton(IDC_SELECT_FIRMWARE_RADIO, IDC_SELECT_VERSION_RADIO);
    AfxGetApp()->WriteProfileInt(_T("Attach Data Settings"), _T("Input Select"), source);

    switch ( source )
    {
        case IDC_SELECT_FIRMWARE_RADIO:
            EnableFirmwareGroup(true);
            _hexInputCtrl.EnableWindow(false);
            _versionInputCtrl.EnableWindow(false);
			// return data
            if ( _fw.size() )
            {
                _returnData.resize(_fw.size());
                memcpy(&_returnData[0], _fw.GetDataPtr(), _fw.size());
            }
            else
            {
                _returnData.clear();
            }
            break;
        case IDC_SELECT_HEX_RADIO:
            EnableFirmwareGroup(false);
            _hexInputCtrl.EnableWindow(true);
            _versionInputCtrl.EnableWindow(false);
			// return data
            _returnData = _hexData;
            break;
        case IDC_SELECT_VERSION_RADIO:
            EnableFirmwareGroup(false);
            _hexInputCtrl.EnableWindow(false);
            _versionInputCtrl.EnableWindow(true);
			// return data
            if ( _fw.size() )
            {
                _returnData.resize(_versionInfo.size());
                memcpy(&_returnData[0], _versionInfo.data(), _versionInfo.size());
            }
            else
            {
                _returnData.clear();
            }
            break;
        default:
            break;
    }
	
	// size
	sizeCtrlText.Format(_T("%d bytes"), _returnData.size());
	_sizeCtrl.SetWindowText(sizeCtrlText);
}

void CAttachDataDlg::EnableFirmwareGroup(const bool bEnable)
{
    _fwSourceFileCtrl.EnableWindow(bEnable);
    _fwSourceResourceCtrl.EnableWindow(/*bEnable*/false);    // disable resource input for now

    if ( bEnable )
        OnBnClickedFirmwareSourceRadio();
    else
    {
        _fwFileComboCtrl.EnableWindow(bEnable);
        _fwFileBrowseCtrl.EnableWindow(bEnable);
        _fwFileEmptyLinkCtrl.EnableWindow(bEnable);

        _fwResourceComboCtrl.EnableWindow (bEnable);
    }

    _fwPathCtrl.EnableWindow(bEnable);

    _fwTypeIdLabel.EnableWindow(bEnable);
    _fwTypeIdCtrl.EnableWindow(bEnable);

    _fwCompVerLabel.EnableWindow(bEnable);
    _fwComponentVersionCtrl.EnableWindow(bEnable);

    _fwProdVerLabel.EnableWindow(bEnable);
    _fwProductVersionCtrl.EnableWindow(bEnable);

    _fwLangLabel.EnableWindow(bEnable);
    _fwLanguageCtrl.EnableWindow(bEnable);

    _fwTagLabel.EnableWindow(bEnable);
    _fwTagCtrl.EnableWindow(bEnable);
}

void CAttachDataDlg::OnBnClickedFirmwareSourceRadio()
{
    int source = GetCheckedRadioButton(IDC_FIRMWARE_FILE_RADIO, IDC_FIRMWARE_RESOURCE_RADIO);
    AfxGetApp()->WriteProfileInt(_T("Attach Data Settings"), _T("Firmware Source Select"), source);

    switch ( source )
    {
        case IDC_FIRMWARE_FILE_RADIO:

            _fwFileComboCtrl.EnableWindow(true);
            _fwFileBrowseCtrl.EnableWindow(true);
            _fwFileEmptyLinkCtrl.EnableWindow(true);

            _fwResourceComboCtrl.EnableWindow(false);

            break;
        case IDC_FIRMWARE_RESOURCE_RADIO:

            _fwFileComboCtrl.EnableWindow(false);
            _fwFileBrowseCtrl.EnableWindow(false);
            _fwFileEmptyLinkCtrl.EnableWindow(false);

            _fwResourceComboCtrl.EnableWindow(true);

            break;
        default:
            break;
    }
}

void CAttachDataDlg::OnCbnSelchangeFirmwareFileCombo()
{
	CString len, name;
	int sel = _fwFileComboCtrl.GetCurSel();
	if ( sel != CB_ERR )
		_fwFileComboCtrl.GetLBText(sel, name);
    AfxGetApp()->WriteProfileString(_T("Attach Data Settings"), _T("Last Firmware File"), name);

	
	CWaitCursor cursor;
    if ( _fw.LoadFromFile(name) == ERROR_SUCCESS )
	{
		cursor.Restore();
		// name
		_fwPathCtrl.SetWindowText(name);
		// size
        len.Format(_T("%d bytes"), _fw.size());
		_sizeCtrl.SetWindowText(len);
		// versions
        _fwProductVersionCtrl.SetWindowText(_fw.GetProductVersion().toString());
		_fwComponentVersionCtrl.SetWindowText(_fw.GetComponentVersion().toString());
        _fwTagCtrl.SetWindowText(_fw.GetFileType().ToString());

        // the data
        _returnData.resize(_fw.size());
        memcpy(&_returnData[0], _fw.GetDataPtr(), _fw.size());
    }
	else
	{
		cursor.Restore();
		_fwPathCtrl.SetWindowText(_T("No file selected."));
		_sizeCtrl.SetWindowText(_T("0 bytes"));
		_fwProductVersionCtrl.SetWindowText(_T("000.000.000"));
		_fwComponentVersionCtrl.SetWindowText(_T("000.000.000"));
		_fwTagCtrl.SetWindowText(_T("????"));

        _returnData.clear();
	}
/*
	CFileStatus status;
	if( CFile::GetStatus( name, status ) )
    {
		// date
		CTime time = status.m_mtime;
		m_date_ctrl.SetWindowText( time.Format(_T("%c")) );
	}
	else
		m_date_ctrl.SetWindowText(_T(""));

	// see if we are supposed to do the download
	OnLoadStateChange();
*/
}

void CAttachDataDlg::OnBnClickedFirmwareFileBrowseButton()
{
	CFileDialog dlg ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
						_T("Firmware files (*.sb)|*.sb|All Files (*.*)|*.*||"), this );

    if ( IDOK == dlg.DoModal() )
    {
 		_fwFileComboCtrl.AddToMRU ( dlg.GetPathName() );
		_fwFileComboCtrl.RefreshCtrl();
        _fwFileComboCtrl.SelectString(0, dlg.GetPathName());
		OnCbnSelchangeFirmwareFileCombo();
	}
}

void CAttachDataDlg::OnStnClickedFirmwareFileEmptyLink()
{
	_fwFileComboCtrl.EmptyMRU();
	_fwFileComboCtrl.RefreshCtrl();
	_fwFileComboCtrl.SetWindowText(_T(""));

	_fwPathCtrl.SetWindowText(_T("No file selected."));
	_sizeCtrl.SetWindowText(_T("0 bytes"));
	_fwProductVersionCtrl.SetWindowText(_T("000.000.000"));
	_fwComponentVersionCtrl.SetWindowText(_T("000.000.000"));
	_fwTagCtrl.SetWindowText(_T("????"));

    _fw.clear();

//	OnLoadStateChange();
}

void CAttachDataDlg::OnDropFiles(HDROP hDropInfo)
{
	CString fileName;
	UINT num = DragQueryFile(hDropInfo, 0, fileName.GetBufferSetLength(_MAX_PATH), _MAX_PATH);
	fileName.ReleaseBuffer();

 		_fwFileComboCtrl.AddToMRU ( fileName );
		_fwFileComboCtrl.RefreshCtrl();
        _fwFileComboCtrl.SelectString(0, fileName);
		OnCbnSelchangeFirmwareFileCombo();
}

void CAttachDataDlg::OnBnClickedCancel()
{
    _returnData.clear();

    OnCancel();
}

void CAttachDataDlg::OnEnChangeVersionInputEdit()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.
    USES_CONVERSION;

    CString versionStr;
    _versionInputCtrl.GetWindowText(versionStr);

    int32_t error = _versionInfo.SetVersion(W2A(versionStr.GetBuffer()));
}
