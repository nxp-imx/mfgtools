/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// UsbFilterListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceEnum.h"
#include "UsbFilterListCtrl.h"
#include "AddFilterDlg.h"
#include "DeviceEnumDlg.h"


// CUsbFilterListCtrl

IMPLEMENT_DYNAMIC(CUsbFilterListCtrl, CListCtrl)
CUsbFilterListCtrl::CUsbFilterListCtrl()
{
	// attach the popup menu
	m_usb_filter_menu.CreatePopupMenu();
	m_usb_filter_menu.AppendMenu(MF_STRING, ID_FILTERS_ADD, _T("Add..."));
	m_usb_filter_menu.AppendMenu(MF_STRING, ID_FILTERS_REMOVE, _T("Remove"));
	m_usb_filter_menu.AppendMenu(MF_STRING, ID_SEPARATOR);
	m_usb_filter_menu.AppendMenu(MF_STRING, ID_FILTERS_REMOVEALL, _T("Remove All"));
}

CUsbFilterListCtrl::~CUsbFilterListCtrl()
{
}


BEGIN_MESSAGE_MAP(CUsbFilterListCtrl, CListCtrl)
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()



// CUsbFilterListCtrl message handlers


void CUsbFilterListCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	CListCtrl::OnRButtonDown(nFlags, point);

	BOOL took_action = false;

	// Pop-up filter menu
	CPoint screen_point = point;
	ClientToScreen(&screen_point);
	DWORD selection = m_usb_filter_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, screen_point.x, screen_point.y, this);
	switch (selection)
	{
		case ID_FILTERS_ADD:
		{
			AddFilterDlg dlg;
			INT_PTR result = dlg.DoModal();
			if ( result == IDOK )
			{
				CString filter;
				filter.Format(_T("%s/%s"), dlg.m_usb_vid, dlg.m_usb_pid);
				theApp.WriteProfileInt(_T("USB Filters"), filter, 1);
				SetCheck(InsertItem(GetItemCount(), filter));
				took_action = true;
			}
			break;
		}
	case ID_FILTERS_REMOVE:
		{
			// Select the item the user clicked on.
			UINT uFlags;
			int nItem = HitTest(point, &uFlags);

			if ( nItem != -1 )
			{
				if (uFlags & LVHT_ONITEMLABEL)
				{
					CString filter = GetItemText(nItem, 0);
					HKEY hKey;
					if ( RegOpenKey(HKEY_CURRENT_USER, 
						_T("Software\\SigmaTel\\DeviceEnum\\USB Filters"), &hKey) == ERROR_SUCCESS )
					{
						RegDeleteValue(hKey, filter);
						RegCloseKey(hKey);
						DeleteItem(nItem);
						took_action = true;
					}
				}
			}
			break;
		}
	case ID_FILTERS_REMOVEALL:
		{
			CStringArray filters;
			HKEY hKey; DWORD index=0; TCHAR name[MAX_PATH]; DWORD name_size; DWORD data; DWORD data_size = sizeof(DWORD);
			if ( RegOpenKey(HKEY_CURRENT_USER, _T("Software\\SigmaTel\\DeviceEnum\\USB Filters"), &hKey) == ERROR_SUCCESS )
			{
				while (1)
				{
					name_size = MAX_PATH;
					LONG error = RegEnumValue(hKey, index, &name[0], &name_size, NULL, NULL, (LPBYTE)&data, &data_size);
					if (error == ERROR_NO_MORE_ITEMS)
						break;

					filters.Add(name);
					++index;
				}
				
				for ( INT_PTR i = 0; i < filters.GetCount(); ++i )
				{
					RegDeleteValue(hKey, filters[i]);
					took_action = true;
				}

				RegCloseKey(hKey);
				DeleteAllItems();
			}
			break;
		}
	default:
		break;
	}

	if ( took_action )
		((CDeviceEnumDlg*)GetParent())->UpdateUsbFilters();
}
