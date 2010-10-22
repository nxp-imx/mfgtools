/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "StdAfx.h"
#include "StBinderProfile.h"
#include "TargetCfgData.h"
#include "resource.h"

extern TARGET_CFG_DATA g_ResCfgData;


StBinderProfile::StBinderProfile(CString _ProfileName)
{
	LONG lStatus = -1;
	m_csProfileName = _ProfileName;


	m_hProfilesKey = NULL;
	lStatus = RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Freescale\\StBinder\\Profiles"), 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hProfilesKey, NULL);

	DWORD dwIndex = 0;
	while (lStatus == ERROR_SUCCESS)
	{
		TCHAR nameStr[256];
		DWORD dwSize = 256;

		lStatus = RegEnumKeyEx(m_hProfilesKey, dwIndex, nameStr, &dwSize, NULL, NULL, NULL, NULL );

		if (lStatus == ERROR_SUCCESS)
		{
			++dwIndex;
			m_csProfileArray.Add(nameStr);
		}

	}
}

StBinderProfile::~StBinderProfile(void)
{
	if (m_hProfilesKey != INVALID_HANDLE_VALUE)
		RegCloseKey(m_hProfilesKey);
}


wchar_t * regValues[]={ 
_T("Company"),
_T("ProductDesc"),
_T("ProductName"),
_T("AppTitle"),
_T("CopyRight"),
_T("Comment"),
_T("MTPMfg"),
_T("MTPId"),
_T("USB_VID"),
_T("USB_PID"),
_T("USB_PID2"),
_T("SCSI_MFG"),
_T("SCSI_PRODUCT"),
_T("AutoRecv"),
_T("ForceRecv"),
_T("Reboot2Player"),
_T("LowNAND"),
_T("UseLocal"),
_T("Dialog"),
_T("FormatData"),
_T("EraseMedia"),
_T("EraseWarning"),
_T("DefaultFS"),
_T("AutoStart"),
_T("AutoClose"),
_T("CompanyImage"),
_T("UpdaterIcon"),
_T("Version"),
_T("Drive1"),
_T("Drive2"),
_T("Drive3"),
_T("Drive4"),
_T("Drive5"),
_T("Drive6"),
_T("Drive7"),
_T("Drive8"),
_T("Drive9"),
_T("Drive10"),
_T("Drive11"),
_T("Drive12")
};


BOOL StBinderProfile::ReadProfileRegData(CStBinderDlg *_binderDlg)
{
	HKEY hKey;

	LONG lStatus = RegCreateKeyEx(m_hProfilesKey, m_csProfileName, 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	if (lStatus == ERROR_SUCCESS)
	{
		LONG queryStatus;
		DWORD dwSize;
		DWORD dwType;
		TCHAR tempStr[MAX_PATH];

		dwSize = COMPANYNAME_MAX*sizeof(TCHAR);
		dwType = REG_SZ;
		queryStatus = RegQueryValueEx( hKey, regValues[COMPANY_DESC], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.CompanyName,tempStr);

		dwSize = PRODDESC_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[PRODUCT_DESC], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.ProductDesc, tempStr);

		dwSize = PRODNAME_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[PRODUCT_NAME], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.ProductName, tempStr);

		dwSize = APPTITLE_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[APP_TITLE], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.AppTitle, tempStr);

		dwSize = COPYRIGHT_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[COPYRIGHT], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.Copyright, tempStr);

		dwSize = MAX_PATH*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[VERSION], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_stscanf_s(tempStr, _T("%d.%d.%d.%d"), &g_ResCfgData.Options.UpdMajorVersion,
									&g_ResCfgData.Options.UpdMinorVersion,
									&g_ResCfgData.Options.ProdMajorVersion,
									&g_ResCfgData.Options.ProdMinorVersion);

		dwSize = COMMENT_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[COMMENT], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.Comment, tempStr);

		dwSize = MAX_PATH*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[COMPANY_IMAGE], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.CompanyImagePathname = tempStr;

		dwSize = MAX_PATH*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[UPDATER_ICON], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.UpdaterIconPathname = tempStr;

		dwSize = MTPMFG_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[MTP_MFG], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.MtpMfg, tempStr);

		dwSize = MTPPROD_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[MTP_PROD], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.MtpProd, tempStr);

		dwSize = SCSIMFG_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[SCSI_MFG], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.SCSIMfg, tempStr);

		dwSize = SCSIPROD_MAX*sizeof(TCHAR);
		queryStatus = RegQueryValueEx( hKey, regValues[SCSI_PRODUCT], NULL, &dwType,
			(LPBYTE)tempStr, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			_tcscpy(g_ResCfgData.Options.SCSIProd, tempStr);

		dwSize = sizeof(DWORD);
		DWORD dwTemp;
		queryStatus = RegQueryValueEx( hKey, regValues[USB_VID], NULL, &dwType,
			(LPBYTE)&dwTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.USBVendor = (USHORT)(dwTemp & 0x0000FFFF);

		dwSize = sizeof(DWORD);
		queryStatus = RegQueryValueEx( hKey, regValues[USB_PID], NULL, &dwType,
			(LPBYTE)&dwTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.USBProd = (USHORT)(dwTemp & 0x0000FFFF);

		dwSize = sizeof(DWORD);
		queryStatus = RegQueryValueEx( hKey, regValues[USB_PID2], NULL, &dwType,
			(LPBYTE)&dwTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.SecondaryUSBProd = (USHORT)(dwTemp & 0x0000FFFF);

		dwSize = sizeof(BOOL);
		BOOL bTemp;
		queryStatus = RegQueryValueEx( hKey, regValues[AUTO_RECV], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.AllowAutoRecovery = bTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[FORCE_RECV], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.ForceRecovery = bTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[REBOOT_2PLAYER], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.RebootToPlayerMsg = bTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[LOW_NAND], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.LowNANDSolution = bTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[USE_LOCAL], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.UseLocalFileResources = bTemp;

		dwSize = sizeof(DWORD);
		queryStatus = RegQueryValueEx( hKey, regValues[DLG_TYPE], NULL, &dwType,
			(LPBYTE)&dwTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.DialogType = (USER_DLG_TYPE) dwTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[FORMAT_DATA], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.FormatDataArea = bTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[ERASE_MEDIA], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.EraseMedia = bTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[DATA_ERASE_WARNING], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.MinDlgFmtMsg = bTemp;

		dwSize = sizeof(DWORD);
		queryStatus = RegQueryValueEx( hKey, regValues[DEFAULT_FS], NULL, &dwType,
			(LPBYTE)&dwTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.DefaultFS = (USHORT)dwTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[AUTO_START], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.AutoStart = bTemp;

		dwSize = sizeof(BOOL);
		queryStatus = RegQueryValueEx( hKey, regValues[AUTO_CLOSE], NULL, &dwType,
			(LPBYTE)&bTemp, &dwSize);
		if ( queryStatus == ERROR_SUCCESS )
			g_ResCfgData.Options.AutoClose = bTemp;

		// Get drive array strings
		for (size_t i = 0; i < 12; ++i)
		{
			CString cs_item;
			CString cs_Drive;
			int curPos = 0;
			media::LogicalDrive dd;

			dwSize = MAX_PATH;
			queryStatus = RegQueryValueEx( hKey, regValues[DRIVE1+i], NULL, &dwType,
				(LPBYTE)tempStr, &dwSize);
			if ( queryStatus != ERROR_SUCCESS || !_tcslen(tempStr) )
				break;

			if (i == 0) 
				g_ResCfgData.Options.DriveArray.Clear();

			cs_Drive = tempStr;

			if ( cs_Drive.GetAt(0) != _T(',') )
				dd.Name = cs_Drive.Tokenize(_T(","), curPos);
			else
				++curPos;

			dd.Description = cs_Drive.Tokenize(_T(","), curPos);
			dd.Type = (media::LogicalDriveType)_tstoi(cs_Drive.Tokenize(_T(","), curPos));

		    cs_item = cs_Drive.Tokenize(_T(","), curPos);
			_stscanf_s(cs_item, _T("%x"), &dd.Tag);

	        cs_item = cs_Drive.Tokenize(_T(","), curPos);
		    _stscanf_s(cs_item, _T("%x"), &dd.Flags);

			dd.RequestedKB = _tstoi(cs_Drive.Tokenize(_T(","), curPos));

			g_ResCfgData.Options.DriveArray.AddDrive(dd);
		}

		//
		// Read all files to be bound from registry
		//
		HKEY hFilesKey, hLangKey;
	    CResourceList *pGroupResList;
	    USHORT langindex, fileindex;
		wchar_t langStr[16];
		CString csPathName, csVersion;
		DWORD dwLen;
		LANGID langID;

		lStatus = RegCreateKeyEx(hKey, _T("Files"), 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hFilesKey, NULL);

		// enum each langID key under "Files" and put any files into the appropriate resource list
		langindex = 0;
		dwLen = 16*sizeof(wchar_t);
		while (RegEnumKeyEx(hFilesKey, langindex, langStr, &dwLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			dwLen = 16*sizeof(wchar_t);
			wchar_t szPathName[MAX_PATH];
			wchar_t szLang[16];
			DWORD dwTemp;

			swscanf_s(langStr, _T("%X"), &dwTemp);
			langID = (LANGID) (dwTemp & 0x0000FFFF);

			lStatus = RegCreateKeyEx(hFilesKey, langStr, 0, NULL,
						REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hLangKey, NULL);

	//		HTREEITEM hItem = _binderDlg->m_hFirmwareResources; //m_ResourceTree.GetRootItem();
			HTREEITEM hItem = _binderDlg->m_ResourceTree.GetNextItem(_binderDlg->m_hFirmwareResources, TVGN_CHILD);


			// get the resource list for this langID
			do
			{
				pGroupResList = (CResourceList *)_binderDlg->m_ResourceTree.GetItemData(hItem);

				if ( pGroupResList )
				{
					if (langID == pGroupResList->GetLangID() )
						break;
					else
						pGroupResList = NULL;
				}
				hItem = _binderDlg->m_ResourceTree.GetNextItem(hItem, TVGN_NEXT);
			}
			while (hItem != NULL);

			if ( !pGroupResList )
			{
				// insert new resource group
				continue; // for now
			}


			dwSize = MAX_PATH*sizeof(wchar_t);
			DWORD dwKeyLen = 16*sizeof(wchar_t);
			fileindex = 0;
			DWORD dwType = REG_SZ;
			while (RegEnumValue(hLangKey, fileindex, szLang, &dwKeyLen, NULL, &dwType, (LPBYTE)&szPathName[0], &dwSize) == ERROR_SUCCESS)
			{
				CString csPathName = szPathName;

				// check if file exists
				// get file size, timestamp, version
				// get resID
				_binderDlg->InsertFileFromProfile(hItem, csPathName, _binderDlg);
				++fileindex;
				dwKeyLen = 16*sizeof(wchar_t);
				dwSize = MAX_PATH*sizeof(wchar_t);
			}
			RegCloseKey(hLangKey); // all done for this language key
			++langindex;
		}

		RegCloseKey(hFilesKey);

		RegCloseKey(hKey);
	}

	return TRUE;
}

BOOL StBinderProfile::WriteProfileRegData(CStBinderDlg *_binderDlg)
{
	HKEY hKey;
	LONG lStatus = RegCreateKeyEx(m_hProfilesKey, m_csProfileName, 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	if (lStatus == ERROR_SUCCESS)
	{
		DWORD dwSize;
		DWORD dwType;
		TCHAR tempStr[256];

		//
		// Write all the updater configuration options
		//
		dwType = REG_SZ;
		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.CompanyName) + 1);
		RegSetValueEx( hKey, regValues[COMPANY_DESC], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.CompanyName, dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.ProductDesc) + 1);
		RegSetValueEx( hKey, regValues[PRODUCT_DESC], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.ProductDesc, dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.ProductName) + 1);
		RegSetValueEx( hKey, regValues[PRODUCT_NAME], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.ProductName, dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.AppTitle) + 1);
		RegSetValueEx( hKey, regValues[APP_TITLE], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.AppTitle, dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.Copyright) + 1);
		RegSetValueEx( hKey, regValues[COPYRIGHT], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.Copyright, dwSize);


		_stprintf_s(tempStr, _T("%d.%d.%d.%d"), g_ResCfgData.Options.UpdMajorVersion,
									g_ResCfgData.Options.UpdMinorVersion,
									g_ResCfgData.Options.ProdMajorVersion,
									g_ResCfgData.Options.ProdMinorVersion);
		dwSize = sizeof(TCHAR) * (wcslen(tempStr) + 1);
		RegSetValueEx( hKey, regValues[VERSION], NULL, dwType,
			(LPBYTE)tempStr, dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.Comment) + 1);
		RegSetValueEx( hKey, regValues[COMMENT], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.Comment, dwSize);

		dwSize = sizeof(TCHAR) * (g_ResCfgData.CompanyImagePathname.GetLength() + 1);
		RegSetValueEx( hKey, regValues[COMPANY_IMAGE], NULL, dwType,
			(LPBYTE)g_ResCfgData.CompanyImagePathname.GetBuffer(), dwSize);

		dwSize = sizeof(TCHAR) * (g_ResCfgData.UpdaterIconPathname.GetLength() + 1);
		RegSetValueEx( hKey, regValues[UPDATER_ICON], NULL, dwType,
			(LPBYTE)g_ResCfgData.UpdaterIconPathname.GetBuffer(), dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.MtpMfg) + 1);
		RegSetValueEx( hKey, regValues[MTP_MFG], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.MtpMfg, dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.MtpProd) + 1);
		RegSetValueEx( hKey, regValues[MTP_PROD], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.MtpProd, dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.SCSIMfg) + 1);
		RegSetValueEx( hKey, regValues[SCSI_MFG], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.SCSIMfg, dwSize);

		dwSize = sizeof(TCHAR) * (wcslen(g_ResCfgData.Options.SCSIProd) + 1);
		RegSetValueEx( hKey, regValues[SCSI_PRODUCT], NULL, dwType,
			(LPBYTE)g_ResCfgData.Options.SCSIProd, dwSize);

		dwSize = sizeof(DWORD);
		dwType = REG_DWORD;
		DWORD dwTemp = g_ResCfgData.Options.USBVendor;
		RegSetValueEx( hKey, regValues[USB_VID], NULL, dwType,
			(LPBYTE)&dwTemp, dwSize);

		dwSize = sizeof(DWORD);
		dwTemp = g_ResCfgData.Options.USBProd;
		RegSetValueEx( hKey, regValues[USB_PID], NULL, dwType,
			(LPBYTE)&dwTemp, dwSize);

		dwSize = sizeof(DWORD);
		dwTemp = g_ResCfgData.Options.SecondaryUSBProd;
		RegSetValueEx( hKey, regValues[USB_PID2], NULL, dwType,
			(LPBYTE)&dwTemp, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[AUTO_RECV], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.AllowAutoRecovery, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[FORCE_RECV], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.ForceRecovery, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[REBOOT_2PLAYER], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.RebootToPlayerMsg, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[LOW_NAND], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.LowNANDSolution, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[USE_LOCAL], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.UseLocalFileResources, dwSize);

		dwSize = sizeof(USER_DLG_TYPE);
		RegSetValueEx( hKey, regValues[DLG_TYPE], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.DialogType, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[FORMAT_DATA], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.FormatDataArea, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[ERASE_MEDIA], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.EraseMedia, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[DATA_ERASE_WARNING], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.MinDlgFmtMsg, dwSize);

		dwSize = sizeof(DWORD);
		dwTemp = g_ResCfgData.Options.DefaultFS;
		RegSetValueEx( hKey, regValues[DEFAULT_FS], NULL, dwType,
			(LPBYTE)&dwTemp, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[AUTO_START], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.AutoStart, dwSize);

		dwSize = sizeof(BOOL);
		RegSetValueEx( hKey, regValues[AUTO_CLOSE], NULL, dwType,
			(LPBYTE)&g_ResCfgData.Options.AutoClose, dwSize);

		//
		// Write the drive array strings
		//
		dwType = REG_SZ;
		for (size_t i = 0; i < 12; ++i)
		{
			CString cs_Drive;
			TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT]; 
			CString cs_name;

			if ( i < g_ResCfgData.Options.DriveArray.Size() )
			{
				drive[0] = dir[0] = fname[0] = ext[0] = _T('');
				if ( g_ResCfgData.Options.DriveArray[i].Name.IsEmpty() )
					cs_name.Empty();
				else
				{
					_tsplitpath_s(g_ResCfgData.Options.DriveArray[i].Name.c_str(), drive, dir, fname, ext);
					cs_name.Format(_T("%s%s"), fname, ext);
				}
				cs_Drive.Format( _T("%s,%s,%d,0x%02X,%d,%d"), 
					    cs_name.IsEmpty() ? _T("") : cs_name, 
						g_ResCfgData.Options.DriveArray[i].Description.c_str(), 
					    g_ResCfgData.Options.DriveArray[i].Type, 
						g_ResCfgData.Options.DriveArray[i].Tag,
						g_ResCfgData.Options.DriveArray[i].Flags,
					    g_ResCfgData.Options.DriveArray[i].RequestedKB );
	
				dwSize = sizeof(TCHAR) * (cs_Drive.GetLength() + 1);  
				RegSetValueEx( hKey, regValues[DRIVE1+i], NULL, dwType,
							(LPBYTE)cs_Drive.GetBuffer(), dwSize);
			}
			else
				RegSetValueEx( hKey, regValues[DRIVE1+i], NULL, dwType,
							(LPBYTE)_T(""), 2);
		}

		//
		// write all currently non-bound resource files
		//

		HKEY hFilesKey;
	    CResourceList *pGroupResList;
	    USHORT index;
		DWORD dwFileSize;
		CString csPathName, csVersion;
		USHORT uTagId;

		HTREEITEM hItem = _binderDlg->m_ResourceTree.GetNextItem(_binderDlg->m_hFirmwareResources, TVGN_CHILD);

		// Clear any existing Files subkey and re-create
		RegDeleteKey(hKey, _T("Files"));

		lStatus = RegCreateKeyEx(hKey, _T("Files"), 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hFilesKey, NULL);

		do
		{
			pGroupResList = (CResourceList *)_binderDlg->m_ResourceTree.GetItemData(hItem);

			if ( pGroupResList )
			{
				HKEY hSubKey = NULL;

				index = 0;
				while (pGroupResList->GetAtIndex(index, csPathName, dwFileSize, csVersion, uTagId))
				{
					if ( !pGroupResList->IsBoundResource( index ) )
					{ 
						TCHAR szStr[16];
						LANGID lang = pGroupResList->GetLangID();
						if ( index == 0 )
						{ // first one, create the langid key
							_stprintf_s(szStr, 16, _T("%x"), lang);
							lStatus = RegCreateKeyEx(hFilesKey, szStr, 0, NULL,
									REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
						}

						_stprintf_s(szStr, 16, _T("%d"), index);

						RegSetValueEx( hSubKey, szStr, NULL, REG_SZ, (LPBYTE)csPathName.GetBuffer(),
									sizeof(TCHAR) * (csPathName.GetLength() + 1));
					}
					++index;
				}
				if (hSubKey)
					RegCloseKey(hSubKey);
			}
			hItem = _binderDlg->m_ResourceTree.GetNextItem(hItem, TVGN_NEXT);
		}
		while (hItem != NULL);

		RegCloseKey(hFilesKey);

		RegCloseKey(hKey);
	}

	return TRUE;
}

