/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// ApiListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceApi.h"
#include "ApiListCtrl.h"
#include "DeviceApiDlg.h"
#include "ParamDlg.h"
#include "AttachDataDlg.h"
#include "Libs/DevSupport/StApi.h"


// CApiListCtrl

IMPLEMENT_DYNAMIC(CApiListCtrl, CListCtrl)
CApiListCtrl::CApiListCtrl()
{
		// attach the popup menu
	_paramMenu.CreatePopupMenu();
	_paramMenu.AppendMenu(MF_STRING | MF_GRAYED, IDM_EDIT_PARAMS, _T("Edit Parameters..."));
	_paramMenu.AppendMenu(MF_STRING | MF_GRAYED, IDM_ATTACH_DATA, _T("Attach Data..."));
}

CApiListCtrl::~CApiListCtrl()
{
}


BEGIN_MESSAGE_MAP(CApiListCtrl, CListCtrl)
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()



// CApiListCtrl message handlers


void CApiListCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	// Select the item the user clicked on.
	// Updates the _currentApi in the main Dialog
	CListCtrl::OnRButtonDown(nFlags, point);

	// Get the currently selected api from the Dialog
	StApi * pApi = ((CDeviceApiDlg*)GetParent())->_currentApi;

	// Only enable the appropriate commands
	// Parameters
	if ( pApi->HasParameters() )
        _paramMenu.EnableMenuItem(IDM_EDIT_PARAMS, MF_BYCOMMAND | MF_ENABLED);
	else
		_paramMenu.EnableMenuItem(IDM_EDIT_PARAMS, MF_BYCOMMAND | MF_GRAYED);
	// Data
    if ( pApi->IsWriteCmd() == ST_WRITE_CMD_PLUS_DATA )
		_paramMenu.EnableMenuItem(IDM_ATTACH_DATA, MF_BYCOMMAND | MF_ENABLED);
	else
		_paramMenu.EnableMenuItem(IDM_ATTACH_DATA, MF_BYCOMMAND | MF_GRAYED);

	// Pop-up filter menu
	bool needsUpdate = false;
	CPoint screen_point = point;
	ClientToScreen(&screen_point);
	DWORD selection = _paramMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, screen_point.x, screen_point.y, this);
	switch (selection)
	{
		case IDM_EDIT_PARAMS:
		{
			CParamDlg dlg(pApi);
			INT_PTR result = dlg.DoModal();
			if ( result == IDOK )
			{
				needsUpdate = true;
			}
			break;
		}
		case IDM_ATTACH_DATA:
		{
			CAttachDataDlg dlg;
			INT_PTR result = dlg.DoModal();
			if ( result == IDOK && !dlg.GetData().empty())
			{
                pApi->SetCommandData(&dlg.GetData()[0], dlg.GetData().size());
                needsUpdate = true;
			}
			break;
		}
		default:
			break;
	}

	if ( needsUpdate )
		((CDeviceApiDlg*)GetParent())->UpdateCommandParams();

}
