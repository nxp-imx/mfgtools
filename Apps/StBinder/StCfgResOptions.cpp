#include "StdAfx.h"
#include "StCfgResOptions.h"

extern TARGET_CFG_DATA g_ResCfgData;

#define COMPANY_ICON_STARTING_ORDINAL	9	// we start here to make sure we don't overlap with any
											// updater icon image.  Default updater.ico has 8 images to
											// reserve plenty of ordinals.

CStCfgResOptions::CStCfgResOptions()
{
}

CStCfgResOptions::~CStCfgResOptions(void)
{
}

void CStCfgResOptions::GetConfigOptions(HMODULE _hModule)
{
	LPVOID pPtr = NULL;
   	HRSRC hResInfo;
	HGLOBAL hRes;


	m_hModule = _hModule;

	g_ResCfgData.pUpdaterGrpIconHeader = NULL;
	g_ResCfgData.pCompanyGrpIconHeader = NULL;
	g_ResCfgData.dwMaxCustomBitmapSize = GetCustomBitmapSize();

	// COMPANY AND UPDATER ICONS
	pPtr = GetResource(IDR_CFG_ABOUT_BMP, sizeof(BOOL));
   	if ( pPtr )
	{
		g_ResCfgData.IsBMP = *((BOOL *)pPtr);

		if (g_ResCfgData.IsBMP)
		{
			pPtr = GetResource(IDR_CFG_BITMAP_ID, sizeof(USHORT));
			USHORT bitmapId = *((USHORT *)pPtr);
			g_ResCfgData.CurrentCompanyBitmap = (HBITMAP)::LoadImage(m_hModule, MAKEINTRESOURCE(bitmapId), IMAGE_BITMAP, 20, 20,
				                                LR_DEFAULTCOLOR);
			g_ResCfgData.CurrentCompanyIcon = NULL;
		}
		else
		{
			g_ResCfgData.CurrentCompanyIcon = ::LoadIcon(m_hModule, MAKEINTRESOURCE(IDI_COMPANY_ICON));
			g_ResCfgData.CurrentCompanyBitmap = NULL;
		}
	}

	g_ResCfgData.CurrentUpdaterIcon = ::LoadIcon(m_hModule, MAKEINTRESOURCE(IDI_UPDATER_ICON));

	// Get and save the icon group resource entries
    hResInfo = FindResourceEx( m_hModule,
	    					RT_GROUP_ICON,
		    				MAKEINTRESOURCE(IDI_UPDATER_ICON),
			    			0x409);
    if ( hResInfo )
    {
	    hRes = LoadResource(m_hModule, hResInfo);
   		if ( hRes )
		{
			PVOID pRes;
			DWORD dwSize = SizeofResource(m_hModule, hResInfo);
			g_ResCfgData.pUpdaterGrpIconHeader = (PGROUPICON)VirtualAlloc(NULL, dwSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
			pRes = (PGROUPICON) LockResource(hRes);
			memcpy(g_ResCfgData.pUpdaterGrpIconHeader, pRes, dwSize);
		}
	}

    hResInfo = FindResourceEx( m_hModule,
		    					RT_GROUP_ICON,
			    				MAKEINTRESOURCE(IDI_COMPANY_ICON),
				    			0x409);
    if ( hResInfo )
	{
		hRes = LoadResource(m_hModule, hResInfo);
   		if ( hRes )
		{
			PVOID pRes;
			DWORD dwSize = SizeofResource(m_hModule, hResInfo);
			g_ResCfgData.pCompanyGrpIconHeader = (PGROUPICON)VirtualAlloc(NULL, dwSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
			pRes = (PGROUPICON) LockResource(hRes);
			memcpy(g_ResCfgData.pCompanyGrpIconHeader, pRes, dwSize);
		}
	}
 
	// BASE SDK
	pPtr = GetResource(IDR_CFG_BASE_SDK, (BASESDK_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.BaseSDK, (BASESDK_MAX+1), (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.BaseSDK[0] = '\0';


	// COMPANY NAME
	pPtr = GetResource(IDR_CFG_COMPANYNAME, (COMPANYNAME_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.CompanyName, (COMPANYNAME_MAX+1), (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.CompanyName[0] = '\0';

	// PRODUCT DESCRIPTION
	pPtr = GetResource(IDR_CFG_PRODUCTDESC, (PRODDESC_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.ProductDesc, (PRODDESC_MAX+1), (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.ProductDesc[0] = '\0';

		// PRODUCT NAME
	pPtr = GetResource(IDR_CFG_PRODUCTNAME, (PRODNAME_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.ProductName, PRODNAME_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.ProductName[0] = '\0';

	// APPLICATION TITLE
	pPtr = GetResource(IDR_CFG_APPTITLE, (APPTITLE_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.AppTitle, APPTITLE_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.AppTitle[0] = '\0';

	// COPYRIGHT TEXT
    pPtr = GetResource(IDR_CFG_COPYRIGHT, (COPYRIGHT_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.Copyright, COPYRIGHT_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.Copyright[0] = '\0';

	// COMMENT TEXT
    pPtr = GetResource(IDR_CFG_COMMENTS, (COMMENT_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.Comment, COMMENT_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.Copyright[0] = '\0';

	// SCSI MFG ID
    pPtr = GetResource(IDR_CFG_SCSIMFG, (SCSIMFG_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.SCSIMfg, SCSIMFG_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.SCSIMfg[0] = '\0';

	// SCSI PRODUCT ID
    pPtr = GetResource(IDR_CFG_SCSIPROD, (SCSIPROD_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.SCSIProd, SCSIPROD_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.SCSIProd[0] = '\0';

	// MTP MFG
    pPtr = GetResource(IDR_CFG_MTPMFG, (MTPMFG_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.MtpMfg, MTPMFG_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.MtpMfg[0] = '\0';

	// MTP PRODUCT ID
    pPtr = GetResource(IDR_CFG_MTPPROD, (MTPPROD_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.MtpProd, MTPPROD_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.MtpProd[0] = '\0';

	// USB VENDOR
    pPtr = GetResource(IDR_CFG_USBVENDOR, sizeof(g_ResCfgData.Options.USBVendor));
   	if ( pPtr )
	{
		g_ResCfgData.Options.USBVendor = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.USBVendor = 0;

	// USB PRODUCT ID
    pPtr = GetResource(IDR_CFG_USBPROD, sizeof(g_ResCfgData.Options.USBProd));
   	if ( pPtr )
	{
		g_ResCfgData.Options.USBProd = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.USBProd = 0;

	// USB PRODUCT ID (Secondary)
    pPtr = GetResource(IDR_CFG_USBPROD_SECONDARY, sizeof(g_ResCfgData.Options.SecondaryUSBProd));
   	if ( pPtr )
	{
		g_ResCfgData.Options.SecondaryUSBProd = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.SecondaryUSBProd = 0;


	// AUTO RECOVERY
    pPtr = GetResource(IDR_CFG_AUTORECOVERY, sizeof(g_ResCfgData.Options.AllowAutoRecovery));
   	if ( pPtr )
	{
       	g_ResCfgData.Options.AllowAutoRecovery = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.AllowAutoRecovery = TRUE;

	// FORCE RECOVERY
	pPtr = GetResource(IDR_CFG_FORCERECOVERY, sizeof(g_ResCfgData.Options.ForceRecovery));
   	if ( pPtr )
	{
		g_ResCfgData.Options.ForceRecovery = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.ForceRecovery = TRUE;

	// REBOOT TO PLAYER MSG
	pPtr = GetResource(IDR_CFG_REBOOTMSG, sizeof(g_ResCfgData.Options.RebootToPlayerMsg));
   	if ( pPtr )
	{
		g_ResCfgData.Options.RebootToPlayerMsg = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.RebootToPlayerMsg = FALSE;

	// LOW NAND SOLUTION
	pPtr = GetResource(IDR_CFG_LOW_NAND_SOLUTION, sizeof(g_ResCfgData.Options.LowNANDSolution));
   	if ( pPtr )
	{
		g_ResCfgData.Options.LowNANDSolution = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.LowNANDSolution = FALSE;

	// USE LOCAL RESOURCES
	pPtr = GetResource(IDR_CFG_USELOCALRES, sizeof(g_ResCfgData.Options.UseLocalFileResources));
   	if ( pPtr )
	{
		g_ResCfgData.Options.UseLocalFileResources = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.UseLocalFileResources = TRUE;

	// DIALOG TYPE
	pPtr = GetResource(IDR_CFG_DLGTYPE, sizeof(g_ResCfgData.Options.DialogType));
   	if ( pPtr )
	{
		g_ResCfgData.Options.DialogType = *((USER_DLG_TYPE *)pPtr);
	}
	else
		g_ResCfgData.Options.DialogType = DLG_ADVANCED;

	// FORMAT DATA
	pPtr = GetResource(IDR_CFG_FORMATDATA, sizeof(g_ResCfgData.Options.FormatDataArea));
   	if ( pPtr )
	{
		g_ResCfgData.Options.FormatDataArea = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.FormatDataArea = FALSE;

	// ERASE MEDIA
	pPtr = GetResource(IDR_CFG_ERASEMEDIA, sizeof(g_ResCfgData.Options.EraseMedia));
   	if ( pPtr )
	{
		g_ResCfgData.Options.EraseMedia = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.EraseMedia = FALSE;

	// DRIVE LABEL
    pPtr = GetResource(IDR_CFG_DEFAULTLABEL, (DRIVELABEL_MAX+1) * sizeof(TCHAR));
   	if ( pPtr )
	{
		_tcscpy_s(g_ResCfgData.Options.DriveLabel, DRIVELABEL_MAX+1, (LPTSTR)pPtr);
	}
	else
		g_ResCfgData.Options.DriveLabel[0] = '\0';


	// FORMAT WARNING MSG (min dialog only)
	pPtr = GetResource(IDR_CFG_MINDLG_FMT_MSG, sizeof(g_ResCfgData.Options.MinDlgFmtMsg));
   	if ( pPtr )
	{
		g_ResCfgData.Options.MinDlgFmtMsg = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.MinDlgFmtMsg = FALSE;

	// DEFAULT FILE SYSTEM
	pPtr = GetResource(IDR_CFG_DEFAULTFAT, sizeof(g_ResCfgData.Options.DefaultFS));
   	if ( pPtr )
	{
		g_ResCfgData.Options.DefaultFS = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.DefaultFS = 1;

	// AUTO START
    pPtr = GetResource(IDR_CFG_AUTOSTART, sizeof(g_ResCfgData.Options.AutoStart));
   	if ( pPtr )
	{
       	g_ResCfgData.Options.AutoStart = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.AutoStart = FALSE;

	// AUTO CLOSE
    pPtr = GetResource(IDR_CFG_AUTOCLOSE, sizeof(g_ResCfgData.Options.AutoClose));
   	if ( pPtr )
	{
       	g_ResCfgData.Options.AutoClose = *((BOOL *)pPtr);
	}
	else
		g_ResCfgData.Options.AutoClose = FALSE;

	// UPD MAJOR VERSION
	pPtr = GetResource(IDR_CFG_UPD_MAJOR_VERSION, sizeof(g_ResCfgData.Options.UpdMajorVersion));
   	if ( pPtr )
	{
		g_ResCfgData.Options.UpdMajorVersion = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.UpdMajorVersion = 0;

	// UPD MINOR VERSION
	pPtr = GetResource(IDR_CFG_UPD_MINOR_VERSION, sizeof(g_ResCfgData.Options.UpdMinorVersion));
   	if ( pPtr )
	{
		g_ResCfgData.Options.UpdMinorVersion = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.UpdMinorVersion = 0;

	// PROD MAJOR VERSION
	pPtr = GetResource(IDR_CFG_PROD_MAJOR_VERSION, sizeof(g_ResCfgData.Options.ProdMajorVersion));
   	if ( pPtr )
	{
		g_ResCfgData.Options.ProdMajorVersion = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.ProdMajorVersion = 0;

	// PROD MINOR VERSION
	pPtr = GetResource(IDR_CFG_PROD_MINOR_VERSION, sizeof(g_ResCfgData.Options.ProdMinorVersion));
   	if ( pPtr )
	{
		g_ResCfgData.Options.ProdMinorVersion = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.ProdMinorVersion = 0;

	// DRIVE ARRAY COUNT
	pPtr = GetResource(IDR_CFG_DRVARRAY_NUM_DRIVES, sizeof(g_ResCfgData.Options.DriveArrayCount));
   	if ( pPtr )
	{
		g_ResCfgData.Options.DriveArrayCount = *((USHORT *)pPtr);
	}
	else
		g_ResCfgData.Options.DriveArrayCount = 0;

	for (int i = 0; i < g_ResCfgData.Options.DriveArrayCount; ++i)
	{
		CString csStr;
	    media::LogicalDrive  drvDesc;

		pPtr = GetResource(IDR_CFG_DRVARRAY_ONE+i, MAX_PATH);
		csStr = (wchar_t *) pPtr;
		drvDesc = ConvertStringToDrive(csStr);
		g_ResCfgData.Options.DriveArray.AddDrive(drvDesc);
	}
}

media::LogicalDrive CStCfgResOptions::ConvertStringToDrive(CString& strDriveDesc)
{

    media::LogicalDrive  drvDesc;
    CString     tmpStr;
    int         curPos = 0;
    _TCHAR*     stopStr = _T(",");

    if (strDriveDesc.GetAt(0) != _T(','))
    {
        drvDesc.Name = strDriveDesc.Tokenize(_T(","), curPos);
    }
    else
    {
        drvDesc.Name = _T("");
        curPos++;
    }
    if (strDriveDesc.GetAt(curPos) != _T(','))
    {
        drvDesc.Description = strDriveDesc.Tokenize(_T(","), curPos);
    }
    else
    {
        drvDesc.Description = _T("");
        curPos++;
    }
    if (strDriveDesc.GetAt(curPos) != _T(','))
		drvDesc.Type = (media::LogicalDriveType)_tcstoul(strDriveDesc.Tokenize(_T(","), curPos), &stopStr, 10);
    else
        curPos++;
    if (strDriveDesc.GetAt(curPos) != _T(','))
		drvDesc.Tag = (media::LogicalDriveTag)_tcstoul(strDriveDesc.Tokenize(_T(","), curPos), &stopStr, 16);
    else
        curPos++;
    if (strDriveDesc.GetAt(curPos) != _T(','))
        drvDesc.RequestedKB = _tcstoul(strDriveDesc.Tokenize(_T(" "), curPos), &stopStr, 10);

	drvDesc.FileListIndex = -1;
	drvDesc.Flags = media::DriveFlag_NoAction;
	drvDesc.SizeInBytes = 0;
    return drvDesc;
}


//-------------------------------------------------------------------
// ConvertDriveToString()
//
// Convert a LogicalDrive data structure to the string equivalent of
// a player.ini UPDATE operation drive entry.
//-------------------------------------------------------------------
#define DRIVE_FORMAT_STR    _T("%s,%s,%u,0x%02X,%d")
CString CStCfgResOptions::ConvertDriveToString(const media::LogicalDrive& driveDesc)
{
    CString str, csName;


	str.Format(DRIVE_FORMAT_STR,    driveDesc.Name.c_str(),
                                    driveDesc.Description.c_str(),
                                    driveDesc.Type,
                                    driveDesc.Tag,
                                    driveDesc.RequestedKB);

    return str;
}

LPVOID CStCfgResOptions::GetResource(int _ResId, DWORD _size)
{
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;

	if (!m_hModule)
		return pPtr;

    hResInfo = FindResourceEx( m_hModule,
	    					L_STMP_RESINFO_TYPE,
		    				MAKEINTRESOURCE(_ResId),
			    			MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
    if ( hResInfo )
    {
		DWORD dwSize = SizeofResource(m_hModule, hResInfo);

		if (dwSize <= _size)
		{
		    hRes = LoadResource(m_hModule, hResInfo);
   			if ( hRes )
    			pPtr = LockResource(hRes);
		}
	}
	return pPtr;
}




DWORD CStCfgResOptions::WriteConfigOptions(HANDLE _hTarget)
{

	// BITMAP and ICONS
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_ABOUT_BMP),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.IsBMP,     // ptr to resource info 
					sizeof(BOOL)); // size of resource info. 

	if (g_ResCfgData.IsBMP && !g_ResCfgData.CompanyImagePathname.IsEmpty())
	{
		USHORT bitmapId = IDI_CUSTOM_COMPANY_BMP;
		WriteBitmapResource(_hTarget, g_ResCfgData.CompanyImagePathname, IDI_CUSTOM_COMPANY_BMP);
		UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_BITMAP_ID),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&bitmapId,     // ptr to resource info 
					sizeof(USHORT)); // size of resource info. 

	}

	if (!g_ResCfgData.UpdaterIconPathname.IsEmpty())
	{
		DeleteCurrentIcon(_hTarget, g_ResCfgData.pUpdaterGrpIconHeader, IDI_UPDATER_ICON);
	}

	if (!g_ResCfgData.IsBMP && !g_ResCfgData.CompanyImagePathname.IsEmpty())
	{
		DeleteCurrentIcon(_hTarget, g_ResCfgData.pCompanyGrpIconHeader, IDI_COMPANY_ICON);
	}

	if (!g_ResCfgData.UpdaterIconPathname.IsEmpty())
	{
		WriteIconResource(_hTarget, g_ResCfgData.UpdaterIconPathname, 1, IDI_UPDATER_ICON);
	}

	if (!g_ResCfgData.IsBMP && !g_ResCfgData.CompanyImagePathname.IsEmpty())
	{
		WriteIconResource(_hTarget, g_ResCfgData.CompanyImagePathname, COMPANY_ICON_STARTING_ORDINAL, IDI_COMPANY_ICON);
	}


	//
	// We need to get the actual size of the wchar_t strings including adding
	// 2 bytes for the NULL terminator
	//

	// COMPANY NAME
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_COMPANYNAME),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.CompanyName,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.CompanyName))+2); // size of resource info. 


	// PRODUCT NAME
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_PRODUCTNAME),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.ProductName,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.ProductName))+2); // size of resource info. 

	// PRODUCT DESC
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_PRODUCTDESC),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.ProductDesc,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.ProductDesc))+2); // size of resource info. 

	// APPLICATION TITLE
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_APPTITLE),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.AppTitle,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.AppTitle))+2); // size of resource info. 

	// COPYRIGHT TEXT
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_COPYRIGHT),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.Copyright,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.Copyright))+2); // size of resource info. 

	// COMMENT TEXT
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_COMMENTS),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.Comment,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.Comment))+2); // size of resource info. 

	// SCSI MFG ID
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_SCSIMFG),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.SCSIMfg,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.SCSIMfg))+2); // size of resource info. 

	// SCSI PRODUCT ID
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_SCSIPROD),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.SCSIProd,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.SCSIProd))+2); // size of resource info. 

	// MTP MFG
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_MTPMFG),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.MtpMfg,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.MtpMfg))+2); // size of resource info. 

	// MTP PRODUCT ID
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_MTPPROD),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.MtpProd,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.MtpProd))+2); // size of resource info. 

	//
	// The rest are binary data
	//

	// USB VENDOR
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_USBVENDOR),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.USBVendor,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.USBVendor)); // size of resource info. 

	// USB PRODUCT ID
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_USBPROD),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.USBProd,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.USBProd)); // size of resource info. 

	// USB PRODUCT ID (Secondary)
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_USBPROD_SECONDARY),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.SecondaryUSBProd,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.SecondaryUSBProd)); // size of resource info. 

	// AUTO RECOVERY
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_AUTORECOVERY),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.AllowAutoRecovery,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.AllowAutoRecovery)); // size of resource info. 

	// FORCE RECOVERY
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_FORCERECOVERY),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.ForceRecovery,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.ForceRecovery)); // size of resource info. 

	// REBOOT TO PLAYER MSG
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_REBOOTMSG),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.RebootToPlayerMsg,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.RebootToPlayerMsg)); // size of resource info. 

	// LOW NAND SOLUTION
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_LOW_NAND_SOLUTION),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.LowNANDSolution,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.LowNANDSolution)); // size of resource info. 

	// USE LOCAL RESOURCES
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_USELOCALRES),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.UseLocalFileResources,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.UseLocalFileResources)); // size of resource info. 

	// DIALOG TYPE
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_DLGTYPE),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.DialogType,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.DialogType)); // size of resource info. 

	// FORMAT DATA
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_FORMATDATA),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.FormatDataArea,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.FormatDataArea)); // size of resource info. 

	// ERASE MEDIA
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_ERASEMEDIA),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.EraseMedia,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.EraseMedia)); // size of resource info. 

	// DRIVE LABEL TEXT
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_DEFAULTLABEL),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					g_ResCfgData.Options.DriveLabel,     // ptr to resource info 
					(sizeof(wchar_t)*wcslen(g_ResCfgData.Options.DriveLabel))+2); // size of resource info. 

	// FORMAT WARNING MSG (min dialog only)
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_MINDLG_FMT_MSG),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.MinDlgFmtMsg,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.MinDlgFmtMsg)); // size of resource info. 

	// DEFAULT FILE SYSTEM
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_DEFAULTFAT),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.DefaultFS,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.DefaultFS)); // size of resource info. 

	// AUTO START
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_AUTOSTART),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.AutoStart,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.AutoStart)); // size of resource info. 

	// AUTO CLOSE
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_AUTOCLOSE),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.AutoClose,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.AutoClose)); // size of resource info. 

	// UPD MAJOR VERSION
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_UPD_MAJOR_VERSION),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.UpdMajorVersion,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.UpdMajorVersion)); // size of resource info. 

	// UPD MINOR VERSION
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_UPD_MINOR_VERSION),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.UpdMinorVersion,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.UpdMinorVersion)); // size of resource info. 

	// PROD MAJOR VERSION
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_PROD_MAJOR_VERSION),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.ProdMajorVersion,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.ProdMajorVersion)); // size of resource info. 

	// PROD MINOR VERSION
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_PROD_MINOR_VERSION),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.ProdMinorVersion,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.ProdMinorVersion)); // size of resource info. 

	// DRIVE ARRAY
	g_ResCfgData.Options.DriveArrayCount = g_ResCfgData.Options.DriveArray.Size();
	UpdateResource(_hTarget,       // update resource handle 
   					L_STMP_RESINFO_TYPE,
    				MAKEINTRESOURCE(IDR_CFG_DRVARRAY_NUM_DRIVES),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
					&g_ResCfgData.Options.DriveArrayCount,     // ptr to resource info 
					sizeof(g_ResCfgData.Options.DriveArrayCount)); // size of resource info. 

	for (int i = 0; i < g_ResCfgData.Options.DriveArrayCount; ++i)
	{
		CString csStr;

		csStr = ConvertDriveToString(g_ResCfgData.Options.DriveArray[i]);

		UpdateResource(_hTarget,       // update resource handle 
   						L_STMP_RESINFO_TYPE,
    					MAKEINTRESOURCE(IDR_CFG_DRVARRAY_ONE+i),
						MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
						csStr.GetBuffer(),     // ptr to resource info 
						(csStr.GetLength() * sizeof(WCHAR)) + 2); // size of resource info. 
	}

	return ERROR_SUCCESS;
}

DWORD CStCfgResOptions::WriteIconResource(HANDLE _hTarget, CString _csPathname, int _startingOrdinal, int _grpResId)
{
	DWORD status = ERROR_SUCCESS;

	HANDLE hFile = CreateFile (
			          _csPathname,
				      GENERIC_READ,// | GENERIC_WRITE,
					  FILE_SHARE_READ,// | FILE_SHARE_WRITE,
	                  NULL, // no SECURITY_ATTRIBUTES structure
		              OPEN_EXISTING, // No special create flags
			          0, // No special attributes
				      NULL); // No template file

    if ( hFile != INVALID_HANDLE_VALUE )
	{
		DWORD dwBytesRead;
		PICONHEADER pIconHeader = NULL;
		PGROUPICON pGroupHeader = NULL;
		char * pImage;
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		PVOID pBuf;

		pBuf = VirtualAlloc(NULL, dwFileSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);//new UCHAR[dwFileSize];

		if (pBuf)
		{
			BOOL b = ReadFile(hFile, pBuf, dwFileSize, &dwBytesRead, NULL);
			if (b && dwBytesRead == dwFileSize)
			{
				int icoHeaderSize, grpSize, nextOrdinal;
				pIconHeader = (PICONHEADER)pBuf;
				pGroupHeader = ConvertToGroupHeader(pIconHeader);

				icoHeaderSize = sizeof(RT_GROUP) + (pIconHeader->ImageCount * sizeof(ICONDIRENTRY_ICO));
				grpSize = sizeof(RT_GROUP) + (pGroupHeader->ImageCount * sizeof(ICONDIRENTRY_GRP));

				pImage = (char *)pBuf;
				pImage += icoHeaderSize;

				// update each image; saving the ordinal in the group dir entry.
				nextOrdinal = _startingOrdinal; // start at zero; will increment to one for first
				for (int i = 0; i < pIconHeader->ImageCount; ++i)
				{

					UpdateResource(_hTarget,       // update resource handle 
		   							RT_ICON,
    								MAKEINTRESOURCE(nextOrdinal), 0x409, pImage, 
									pIconHeader->DirEntry[i].ImageSize); 
					pGroupHeader->DirEntry[i].ResourceID = nextOrdinal;
					pImage += pIconHeader->DirEntry[i].ImageSize;
					++nextOrdinal;
				}

				// Now update the group resource
				UpdateResource(_hTarget,       // update resource handle 
	   					RT_GROUP_ICON, MAKEINTRESOURCE(_grpResId),
						0x409, pGroupHeader,
						grpSize );

				VirtualFree(pGroupHeader, 0, MEM_RELEASE);
			}

			VirtualFree(pBuf, 0, MEM_RELEASE);
		}
		else
			status = ERROR_NOT_ENOUGH_MEMORY;

		CloseHandle(hFile);
	}
	else
		status = ERROR_FILE_NOT_FOUND;

	return status;
}

void CStCfgResOptions::DeleteCurrentIcon(HANDLE _hTarget, PGROUPICON _pGroupIconHeader, int _resId)
{
   
	int i = 0;

	if (_pGroupIconHeader)
	{
		while (i < _pGroupIconHeader->ImageCount)
		{
			UpdateResource(_hTarget,       // update resource handle 
   							RT_ICON,
							MAKEINTRESOURCE(_pGroupIconHeader->DirEntry[i].ResourceID),
							0x409, 
							NULL,     // ptr to resource info 
							0);
			++i;
		}

		UpdateResource(_hTarget,       // update resource handle 
   						RT_GROUP_ICON,
    					MAKEINTRESOURCE(_resId),
						0x409, //MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),  // language
						NULL,     // ptr to resource info 
						0);
	}
}


//
// Convert the .ico header info to one in RT_GROUP_ICON form.  The last member of ICONDIRENTRY_GRP
// is a USHORT rather than DWORD as in the .ico form.
//
PGROUPICON CStCfgResOptions::ConvertToGroupHeader(PICONHEADER _pIconHeader)
{
	PVOID pBuf = VirtualAlloc(NULL,
					sizeof(RT_GROUP) + (_pIconHeader->ImageCount * sizeof(ICONDIRENTRY_GRP)),
					MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);//new UCHAR[dwFileSize];

	if (pBuf)
	{
		PUCHAR pPtr = (PUCHAR)pBuf;
		pPtr += sizeof(RT_GROUP);

		memcpy(pBuf, _pIconHeader, sizeof(RT_GROUP));
		for (int i = 0; i < _pIconHeader->ImageCount; ++i)
		{
			memcpy(pPtr, (PICONDIRENTRY_GRP) &_pIconHeader->DirEntry[i], sizeof(ICONDIRENTRY_GRP));
			pPtr += sizeof(ICONDIRENTRY_GRP);
		}
	}
	return (PGROUPICON)pBuf;
}

DWORD CStCfgResOptions::WriteBitmapResource(HANDLE _hTarget, CString _csPathname, int _resId)
{
	DWORD status = ERROR_SUCCESS;

	HANDLE hFile = CreateFile (
			          _csPathname,
				      GENERIC_READ,// | GENERIC_WRITE,
					  FILE_SHARE_READ,// | FILE_SHARE_WRITE,
	                  NULL, // no SECURITY_ATTRIBUTES structure
		              OPEN_EXISTING, // No special create flags
			          0, // No special attributes
				      NULL); // No template file

    if ( hFile != INVALID_HANDLE_VALUE )
	{
		DWORD dwBytesRead;
		char * pImage;
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		PVOID pBuf;

		// memory must be properly aligned so use VirtualAlloc().
		pBuf = VirtualAlloc(NULL, dwFileSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

		if (pBuf)
		{
			// Skip the 14 byte bitmap header
			SetFilePointer(hFile, 14, NULL, FILE_BEGIN);
			BOOL b = ReadFile(hFile, pBuf, dwFileSize-14, &dwBytesRead, NULL);
			if (b && dwBytesRead == dwFileSize-14)
			{
				pImage = ((char *)pBuf); 

				UpdateResource(_hTarget,       // update resource handle 
		   							RT_BITMAP, 
    								MAKEINTRESOURCE(_resId), 0x409, pImage, 
									dwFileSize-14); 
			}

			VirtualFree(pBuf, 0, MEM_RELEASE);
		}
		else
			status = ERROR_NOT_ENOUGH_MEMORY;

		CloseHandle(hFile);
	}
	else
		status = ERROR_FILE_NOT_FOUND;

	return status;

}

DWORD CStCfgResOptions::GetCustomBitmapSize()
{
   	HRSRC hResInfo;
	LPVOID pPtr = NULL;
	DWORD dwSize = 0;

	if (!m_hModule)
		return dwSize;

    hResInfo = FindResourceEx( m_hModule,
	    					RT_BITMAP,
		    				MAKEINTRESOURCE(IDI_CUSTOM_COMPANY_BMP),
			    			0x409);
    if ( hResInfo )
    {
		dwSize = SizeofResource(m_hModule, hResInfo);
	}
	return dwSize;
}
