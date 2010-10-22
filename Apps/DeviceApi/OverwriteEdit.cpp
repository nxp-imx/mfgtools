/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// OverwriteEdit.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceApi.h"
#include "OverwriteEdit.h"
#include "DeviceApiDlg.h"
#include "ParamDlg.h"
#include "AttachDataDlg.h"

// COverwriteEdit

IMPLEMENT_DYNAMIC(COverwriteEdit, CEdit)
COverwriteEdit::COverwriteEdit()
: _bOverwrite(true)
, _bShowMenu(false)
{
		// attach the popup menu
	_paramMenu.CreatePopupMenu();
	_paramMenu.AppendMenu(MF_STRING | MF_GRAYED, IDM_EDIT_PARAMS, _T("Edit Parameters..."));
	_paramMenu.AppendMenu(MF_STRING | MF_GRAYED, IDM_ATTACH_DATA, _T("Attach Data..."));
}

COverwriteEdit::~COverwriteEdit()
{
}


BEGIN_MESSAGE_MAP(COverwriteEdit, CEdit)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()



// COverwriteEdit message handlers


void COverwriteEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( _bOverwrite )
	{
		// don't allow anything but 0-9, a-f, A-F
		switch (nChar)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':

				// get the string for reference
				GetWindowText(_theString);

				// get the cursor position
				int start, end;
				GetSel(start, end);

				// if the cursor is after the last character, bail
				if ( start >= _theString.GetLength() )
					break;

				// if the cursor in the '0x' hex descriptor, bail
				int found = _theString.FindOneOf(_T("xX"));
				if ( found != -1 && start <= found )
					break;

				// don't replace a character on whitespace
				if ( CString(_theString.GetAt(start)).FindOneOf(_T(" \t\r\n")) != -1 )
					break;

				// replace the char
				_theString.SetAt(start, nChar);
				// move ahead a char
				end = ++start;
				// skip whitespace
				while ( CString(_theString.GetAt(start)).FindOneOf(_T(" \t\r\n")) != -1 )
					end = ++start;

				// put the changed string back
				SetWindowText(_theString);
				// put the cursor back
				SetSel(start, end);

				// tell the Dialog we changed
				GetParent()->SendMessage(WM_MSG_EN_CHANGE, 0, 00);
				break;
		}
	}

}

void COverwriteEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if ( _bOverwrite )
	{
		// disable delete key and backspace key
		if( ((nChar == VK_DELETE) || (nChar == VK_BACK)) && _bOverwrite )
			return;
		
		// get the string for reference
		GetWindowText(_theString);
		// get the cursor position
		int start, end;
		GetSel(start, end);
		// skip white space going left
		if ( nChar == VK_LEFT )
            while ( (start > 0) && ( CString(_theString.GetAt(start-1)).FindOneOf(_T(" \t\r\n")) != -1 ))
					end = --start;
		// skip white space going right
		if ( nChar == VK_RIGHT )
			while ( (start < _theString.GetLength()) && ( CString(_theString.GetAt(start+1)).FindOneOf(_T(" \t\r\n")) != -1 ) )
				end = ++start;

		// put the cursor where it goes
		SetSel(start, end);
	}

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void COverwriteEdit::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ( !_bShowMenu )
		return;

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

	// Pop-up menu
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
			if ( result == IDOK && !dlg.GetData().empty() )
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

	CEdit::OnRButtonDown(nFlags, point);
}
