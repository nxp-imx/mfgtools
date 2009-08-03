// DeviceTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceEnum.h"
#include "DeviceTreeCtrl.h"
#include "DeviceEnumDlg.h"
#include "Libs/DevSupport/Device.h"


// CDeviceTreeCtrl

IMPLEMENT_DYNAMIC(CDeviceTreeCtrl, CTreeCtrl)
CDeviceTreeCtrl::CDeviceTreeCtrl()
{
	m_eject_menu.CreatePopupMenu();
	m_eject_menu.AppendMenu(MF_STRING, ID_EJECT, _T("Eject"));
}

CDeviceTreeCtrl::~CDeviceTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CDeviceTreeCtrl, CTreeCtrl)
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()



// CDeviceTreeCtrl message handlers


void CDeviceTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	CTreeCtrl::OnRButtonDown(nFlags, point);

	CPoint screen_point = point;
	ClientToScreen(&screen_point);

	// Select the item the user clicked on.
	UINT uFlags;
	HTREEITEM hti = HitTest(point, &uFlags);

	if ( hti != NULL )
	{
		if (uFlags & TVHT_ONITEM)
		{
			SelectItem(hti);
			
			// get the selected device
			//
			DeviceClass* pDevClass = dynamic_cast<DeviceClass*>((DeviceClass*)GetItemData(hti));
			// TODO: well... dynamic_cast seems to return a non-null pointer
			// to a DeviceClass no matter if hti.data is a Device* or a DeviceClass*.
			// If I then again try to cast the pointer into a Device*, it returns null
			// for DeviceClass derived nodes and a valid Device* for Device derived nodes.
			Device* pDevice = dynamic_cast<Device*>(pDevClass);
			if ( pDevice == NULL )
				return;
			
			// Pop-up filter menu
			DWORD selection;
			if ( pDevice->IsUsb() )
			{
				m_eject_menu.EnableMenuItem(ID_EJECT, MF_BYCOMMAND | MF_ENABLED);
				selection = m_eject_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, screen_point.x, screen_point.y, this);
				if ( selection == ID_EJECT )
				{
					DWORD error = pDevice->Eject(true);
//c should be handled by automatic device change nmessages now
//c					if ( error == CR_SUCCESS )
//c						((CDeviceEnumDlg*)GetParent())->OnBnClickedRefreshButton();
				}
			}
			else
			{
				// disabled Eject menu item
				m_eject_menu.EnableMenuItem(ID_EJECT, MF_BYCOMMAND | MF_GRAYED);
				m_eject_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, screen_point.x, screen_point.y, this);
			}
		}
	}
}
