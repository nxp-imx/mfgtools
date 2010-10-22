/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
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
#include "stupdaterdlg.h"
#include "StDeviceInfo.h"

CStDeviceInfo::CStDeviceInfo(CStUpdaterDlg *pDlg)
{
	m_pDlg = pDlg;
	m_ChipId = 0;
	m_ROMId = 0;
	m_ExternalRAMSize = 0;
	m_VirtualRAMSize = 0;
}

CStDeviceInfo::~CStDeviceInfo(void)
{
}

void CStDeviceInfo::GetDeviceInfo()
{
	ST_ERROR err;

  	err = m_pDlg->GetUpdater()->GetChipId(m_ChipId);
	if ( err == STERR_NONE )
	{
		m_pDlg->m_LogInfo.DevInfoChipId.Format(L"%04X", m_ChipId);
	}

	err = m_pDlg->GetUpdater()->GetROMId(m_ROMId);
	if ( err == STERR_NONE )
	{
		m_pDlg->m_LogInfo.DevInfoROMRevision.Format(L"0x%X", m_ROMId);
	}


	if (m_pDlg->IsAdvDialog())
	{
		err = m_pDlg->GetUpdater()->GetExternalRAMSizeInMB(m_ExternalRAMSize);
		if(err == STERR_NONE)
		{
			m_pDlg->m_LogInfo.DevInfoExtRAMSize.Format(L"%d(MB)", m_ExternalRAMSize);
		}
	}

	if (m_pDlg->IsAdvDialog())
	{
		err = m_pDlg->GetUpdater()->GetVirtualRAMSizeInMB(m_VirtualRAMSize);
		if(err == STERR_NONE)
		{
			m_pDlg->m_LogInfo.DevInfoVirtualRAMSize.Format(L"%d(MB)", m_VirtualRAMSize);
		}
	}

	if (m_pDlg->IsAdvDialog())
	{
		// Get serial number
		USHORT SerialNumSize;
		err = m_pDlg->GetUpdater()->GetSerialNumberSize( SerialNumSize );
		if ( err == STERR_NONE )
		{
			SerialNumSize >>= 8;
			CStByteArray *pSerialNumber = new CStByteArray(SerialNumSize);

			err = m_pDlg->GetUpdater()->GetSerialNumber(pSerialNumber);

			if(err == STERR_NONE)
			{
				int index;
				m_SerialNumber.Empty();
		        for ( index=0; index < SerialNumSize; ++index )
			    {
				   m_SerialNumber.AppendFormat(_T("%02X"), *pSerialNumber->GetAt(index));
	            }

				m_pDlg->m_LogInfo.DevInfoSerialNumber = m_SerialNumber;
			}
			delete pSerialNumber;
		}
	}
}


void CStDeviceInfo::SetDeviceInfo()
{
	CString str;

	if ( !m_pDlg->GetUpdater()->IsDeviceInMscMode() && !m_pDlg->GetUpdater()->IsDeviceInUpdaterMode() )
	{	// clear the device info fields
		m_pDlg->SetDlgItemText(IDC_DEVINFO_CHIPID, L"");
		m_pDlg->SetDlgItemText(IDC_DEVINFO_ROM_REVISION, L"");
		m_pDlg->SetDlgItemText(IDC_DEVINFO_EXT_RAM_SIZE, L"");
		m_pDlg->SetDlgItemText(IDC_DEVINFO_VRAM_SIZE, L"");
		m_pDlg->SetDlgItemText(IDC_DEVINFO_SERIALNO, L"");
	    m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_CHIPID)->EnableWindow(TRUE);
	    m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_ROM_REVISION)->EnableWindow(FALSE);
		m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_EXT_RAM_SIZE)->EnableWindow(FALSE);
		m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_VRAM_SIZE)->EnableWindow(FALSE);
		m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_SERIALNO)->EnableWindow(FALSE);
		return;
	}

	if ( m_ChipId )
	{
		str.Format(L"%04X", m_ChipId);
		m_pDlg->SetDlgItemText(IDC_DEVINFO_CHIPID, str);
		m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_CHIPID)->EnableWindow(TRUE);
	}

	if ( m_ROMId )
	{
		str.Format(L"0x%X", m_ROMId);
		m_pDlg->SetDlgItemText(IDC_DEVINFO_ROM_REVISION, str);
		m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_ROM_REVISION)->EnableWindow(TRUE);
	}

	if ( m_ExternalRAMSize )
	{
		str.Format(L"%d(MB)", m_ExternalRAMSize);
		m_pDlg->SetDlgItemText(IDC_DEVINFO_EXT_RAM_SIZE, str);
	    m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_EXT_RAM_SIZE)->EnableWindow(TRUE);
	}

	if ( m_VirtualRAMSize )
	{
		str.Format(L"%d(MB)", m_VirtualRAMSize);
		m_pDlg->SetDlgItemText(IDC_DEVINFO_VRAM_SIZE, str);
	    m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_VRAM_SIZE)->EnableWindow(TRUE);
	}

	if (!m_SerialNumber.IsEmpty())
	{
		m_pDlg->GetDlgItem(IDC_TEXT_DEVINFO_SERIALNO)->EnableWindow(TRUE);
		m_pDlg->SetDlgItemText(IDC_DEVINFO_SERIALNO, m_SerialNumber);
	}
}

USHORT CStDeviceInfo::GetChipId()
{
	return m_ChipId;
}

USHORT CStDeviceInfo::GetROMId()
{
	return m_ROMId;
}

ULONG CStDeviceInfo::GetExternalRAM()
{
	return m_ExternalRAMSize;
}

ULONG CStDeviceInfo::GetVirtualRAM()
{
	return m_VirtualRAMSize;
}

void CStDeviceInfo::GetSerialNumber(CString& _str)
{
	if ( !m_SerialNumber.IsEmpty() )
		_str = m_SerialNumber;
	else
		_str.Empty();
}
