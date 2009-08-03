#include "stdafx.h"
#include <shlwapi.h>
#include "resource.h"
#include "StHeader.h"
#include "StGlobals.h"
#include "ddildl_defs.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "StProgress.h"
#include "StUpdater.h"
#include "StError.h"
#include "StResource.h"
#include "stupdaterdlg.h"
#include "StNandMediaInfo.h"

CStNandMediaInfo::CStNandMediaInfo(CStUpdaterDlg *pDlg, CStResource *pResource)
{
	m_pDlg = pDlg;
	m_pResource = pResource;
	m_NandChipEnables = 0;
	m_MediaCapacity = 0;
	m_MediaPageSize = 0;
	m_NandIdDetails = 0;
	m_NandMfgStr.Empty();
	m_NandIdStr.Empty();
	m_CellTypeStr.Empty();
}

CStNandMediaInfo::~CStNandMediaInfo(void)
{
}

ST_ERROR CStNandMediaInfo::GetMediaInfo()
{
	ST_ERROR err;

  	err = m_pDlg->GetUpdater()->GetMediaNandChipEnables(m_NandChipEnables);

	if ( err == STERR_NONE )
	{
		m_NandChipSelectsStr.Format(L"%d", m_NandChipEnables);

		err = m_pDlg->GetUpdater()->GetMediaNandMfgId(m_NandMfgId);

		m_pDlg->m_LogInfo.MediaInfoNandChipSelects.Format(L"%d", m_NandChipEnables);
	}
	
	if( err == STERR_NONE )
	{
		m_NandIdDetails = 0;

		SetNandMfgStr((UCHAR)m_NandMfgId);

		m_pDlg->m_LogInfo.MediaInfoNandMfgId = m_NandMfgStr;

		err = m_pDlg->GetUpdater()->GetMediaNandIdDetails(m_NandIdDetails);

		// Convert into a ULARGE_INTEGER to allow 32 bit shifts, and display
		// only top 6 bytes as the lower two bytes are always 0.
		ULARGE_INTEGER tmp;
		tmp.QuadPart = m_NandIdDetails;

		m_NandIdStr.Format(L"%x.%x.%x%x%x%x",
				(tmp.HighPart & 0xff000000) >> 24,
				(tmp.HighPart & 0x00ff0000) >> 16,
				(tmp.HighPart & 0x0000ff00) >> 8,
				(tmp.HighPart & 0x000000ff),
				(tmp.LowPart & 0xff000000) >> 24,
				(tmp.LowPart & 0x00ff0000) >> 16);
						
		m_pDlg->m_LogInfo.MediaInfoNandIdDetails = m_NandIdStr;

		// Get cell type from bits 2,3 in third byte
		m_CellType = (USHORT)(((tmp.HighPart & 0x0000ff00) >> 8) >> 2) & 0x0003;
		
		if ( m_CellType == 0 )
			m_CellTypeStr = L"SLC";
		else
		{
			m_CellType <<= 1;
			m_CellTypeStr.Format(L"MLC<%d>", m_CellType);
		}

		m_pDlg->m_LogInfo.MediaInfoNandCellType = m_CellTypeStr;
	}
	
	if( err == STERR_NONE )
	{
		// Get media capacity
  		err = m_pDlg->GetUpdater()->GetMediaSizeInBytes(m_MediaCapacity);

		m_pDlg->m_LogInfo.MediaInfoCapacity.Format(L"%d(MB)", (unsigned)(m_MediaCapacity / (ULONGLONG)ONE_MB));
	}
	
	if( err == STERR_NONE )
	{
		// Get media page size
		err = m_pDlg->GetUpdater()->GetMediaNandPageSizeInBytes(m_MediaPageSize);
		
		m_pDlg->m_LogInfo.MediaInfoNandPageSize.Format(L"%d", m_MediaPageSize);
	}	

	return err;
}

void CStNandMediaInfo::SetNandInfo()
{
	CString str;

	if ( !m_NandChipSelectsStr.IsEmpty() )
	{
    	m_pResource->GetResourceString(IDS_MEDIA_CHIPSELECTS, str);
	    m_pDlg->SetDlgItemText(IDC_TEXT_MEDIA1, str);
	    m_pDlg->GetDlgItem(IDC_TEXT_MEDIA1)->EnableWindow(TRUE);
		m_pDlg->GetDlgItem(IDC_MEDIA1)->EnableWindow(TRUE);
		m_pDlg->SetDlgItemText(IDC_MEDIA1, m_NandChipSelectsStr);
    	m_pDlg->GetDlgItem(IDC_TEXT_MEDIA1)->ShowWindow(TRUE);
	}

	if ( !m_NandMfgStr.IsEmpty() )
	{
    	m_pResource->GetResourceString(IDS_MEDIA_MFG, str);
	    m_pDlg->SetDlgItemText(IDC_TEXT_MEDIA2, str);
	    m_pDlg->GetDlgItem(IDC_TEXT_MEDIA2)->EnableWindow(TRUE);
		m_pDlg->GetDlgItem(IDC_MEDIA2)->EnableWindow(TRUE);
		m_pDlg->SetDlgItemText(IDC_MEDIA2, m_NandMfgStr);
    	m_pDlg->GetDlgItem(IDC_TEXT_MEDIA2)->ShowWindow(TRUE);
	}

	if ( !m_CellTypeStr.IsEmpty() )
	{
    	m_pResource->GetResourceString(IDS_MEDIA_CELLTYPE, str);
	    m_pDlg->SetDlgItemText(IDC_TEXT_MEDIA3, str);
	    m_pDlg->GetDlgItem(IDC_TEXT_MEDIA3)->EnableWindow(TRUE);
		m_pDlg->GetDlgItem(IDC_MEDIA3)->EnableWindow(TRUE);
		m_pDlg->SetDlgItemText(IDC_MEDIA3, m_CellTypeStr);
    	m_pDlg->GetDlgItem(IDC_TEXT_MEDIA3)->ShowWindow(TRUE);
	}

	if ( !m_NandIdStr.IsEmpty() )
	{
		// Don't need to translate "NAND ID:"
		m_pDlg->SetDlgItemText(IDC_TEXT_MEDIA4, L"NAND ID:");
		m_pDlg->GetDlgItem(IDC_TEXT_MEDIA4)->EnableWindow(TRUE);
		m_pDlg->GetDlgItem(IDC_MEDIA4)->EnableWindow(TRUE);
		m_pDlg->SetDlgItemText(IDC_MEDIA4, m_NandIdStr);
    	m_pDlg->GetDlgItem(IDC_TEXT_MEDIA4)->ShowWindow(TRUE);
	}

	if ( m_MediaCapacity )
	{
    	m_pResource->GetResourceString(IDS_MEDIA_CAPACITY, str);
	    m_pDlg->SetDlgItemText(IDC_TEXT_MEDIA5, str);
	    m_pDlg->GetDlgItem(IDC_TEXT_MEDIA5)->EnableWindow(TRUE);
		m_pDlg->GetDlgItem(IDC_MEDIA5)->EnableWindow(TRUE);
		str.Format(L"%d(MB)", (unsigned)(m_MediaCapacity / (ULONGLONG)ONE_MB));
		m_pDlg->SetDlgItemText(IDC_MEDIA5, str);
    	m_pDlg->GetDlgItem(IDC_TEXT_MEDIA5)->ShowWindow(TRUE);
	}
	
	if ( m_MediaPageSize )
	{
    	m_pResource->GetResourceString(IDS_MEDIA_PAGE_SIZE, str);
	    m_pDlg->SetDlgItemText(IDC_TEXT_MEDIA6, str);
	    m_pDlg->GetDlgItem(IDC_TEXT_MEDIA6)->EnableWindow(TRUE);
		m_pDlg->GetDlgItem(IDC_MEDIA6)->EnableWindow(TRUE);
		str.Format(L"%d", m_MediaPageSize);
		m_pDlg->SetDlgItemText(IDC_MEDIA6, str);
    	m_pDlg->GetDlgItem(IDC_TEXT_MEDIA6)->ShowWindow(TRUE);
	}
}

ULONG CStNandMediaInfo::GetChipEnables ()
{
	return m_NandChipEnables;
}

ULONGLONG CStNandMediaInfo::GetCapacity ()
{
	return m_MediaCapacity;
}

ULONG CStNandMediaInfo::GetPageSize ()
{
	return m_MediaPageSize;
}

USHORT CStNandMediaInfo::GetCellType()
{
	return m_CellType;
}

CString CStNandMediaInfo::GetMfgStr ()
{
	return m_NandMfgStr;
}

CString CStNandMediaInfo::GetIdStr ()
{
	return m_NandIdStr;
}


void CStNandMediaInfo::SetNandMfgStr(UCHAR _id)
{
	switch ( _id )
	{
		case 0xEC:
			m_NandMfgStr = L"Samsung";
			break;
		case 0x20:
			m_NandMfgStr = L"ST Micro";
			break;
		case 0xAD:
			m_NandMfgStr = L"Hynix";
			break;
		case 0x2C: // Intel also uses this
			m_NandMfgStr = L"Micron";
			break;
		case 0x98: // M-Systems also uses this
			m_NandMfgStr = L"Toshiba";
			break;
		case 0x07:
			m_NandMfgStr = L"Renesas";
			break;
		case 0x45:
			m_NandMfgStr = L"Sandisk";
			break;
		case 0x89:
			m_NandMfgStr = L"Intel";
			break;
		default:
			m_NandMfgStr = L"Unknown";
			break;
	}
}
