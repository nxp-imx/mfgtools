/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StMessageDlg.cpp: implementation of the CStMessageDlg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ddildl_defs.h"
#include "StConfigInfo.h"
#include "StCmdlineProcessor.h"
#include "StLogger.h"
#include "StProgress.h"
#include "StVersionInfo.h"
#include "StUpdater.h"
#include "StResource.h"
#include "StUpdaterApp.h"
#include "StGlobals.h"
#include "StMessageDlg.h"
#include "resource.h"
#include "StMessageBox.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStMessageDlg::CStMessageDlg()
{

}

CStMessageDlg::~CStMessageDlg()
{

}

int CStMessageDlg::DisplayMessage(MESSAGE_TYPE _type, CString _message, CWnd* _parent)
{
	CString caption = "";
	UINT style=MB_OK; 
	CStResource * p_resource = ((CStUpdaterApp*)AfxGetApp())->GetResource();

	switch (_type)
	{
	case MSG_TYPE_ERROR:
		p_resource->GetResourceString(IDS_MESSAGE_BOX_CAP_ERROR, caption);
		style = MB_OK | MB_ICONERROR;
		break;
	case MSG_TYPE_INFO:
		p_resource->GetResourceString(IDS_MESSAGE_BOX_CAP_INFO, caption);
		style = MB_OK | MB_ICONINFORMATION;
		break;
	case MSG_TYPE_WARNING:
		p_resource->GetResourceString(IDS_MESSAGE_BOX_CAP_WARNING, caption);
		style = MB_OK | MB_ICONWARNING;
		break;
	case MSG_TYPE_QUESTION:
		p_resource->GetResourceString(IDS_MESSAGE_BOX_CAP_QUESTION, caption);
		style = MB_YESNO | MB_ICONQUESTION;
		break;
	}

	//
	// use ::MessageBoxEx(...) instead for localization support.
	//
	if( _parent )
		return ::MessageBox(_parent->m_hWnd, _message, caption, style);
	else
		return ::MessageBox(0, _message, caption, style);
}

int CStMessageDlg::DisplayMessage(MESSAGE_TYPE _type, CString _message, CString _detail_msg, CWnd* _parent)
{
	if( !_detail_msg.GetLength() )
	{
		return CStMessageDlg::DisplayMessage( _type, _message, _parent );
	}

	CString caption = "";
	UINT style=MB_OK; 
	CStResource * p_resource = ((CStUpdaterApp*)AfxGetApp())->GetResource();

	switch (_type)
	{
	case MSG_TYPE_ERROR:
		p_resource->GetResourceString(IDS_MESSAGE_BOX_CAP_ERROR, caption);
		style = MB_OK | MB_ICONERROR;
		break;
	case MSG_TYPE_INFO:
		p_resource->GetResourceString(IDS_MESSAGE_BOX_CAP_INFO, caption);
		style = MB_OK | MB_ICONINFORMATION;
		break;
	case MSG_TYPE_WARNING:
		p_resource->GetResourceString(IDS_MESSAGE_BOX_CAP_WARNING, caption);
		style = MB_OK | MB_ICONWARNING;
		break;
	case MSG_TYPE_QUESTION:
		p_resource->GetResourceString(IDS_MESSAGE_BOX_CAP_QUESTION, caption);
		style = MB_YESNO | MB_ICONQUESTION;
		break;
	}

	//
	// use ::MessageBoxEx(...) instead for localization support.
	//
	return CStMessageBox::MessageBox( _type, caption, _message, _detail_msg, _parent );
}
