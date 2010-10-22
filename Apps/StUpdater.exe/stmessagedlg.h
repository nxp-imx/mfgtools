/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StMessageDlg.h: interface for the CStMessageDlg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STMESSAGEDLG_H__4E0729F1_FB9E_42B4_B949_4D1ACE04394F__INCLUDED_)
#define AFX_STMESSAGEDLG_H__4E0729F1_FB9E_42B4_B949_4D1ACE04394F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CStMessageDlg  
{
public:
	CStMessageDlg();
	virtual ~CStMessageDlg();
	static int DisplayMessage(MESSAGE_TYPE, CString message, CWnd* Parent=NULL);
	static int DisplayMessage(MESSAGE_TYPE _type, CString _message, CString _detail_msg, CWnd* _parent=NULL);

};

#endif // !defined(AFX_STMESSAGEDLG_H__4E0729F1_FB9E_42B4_B949_4D1ACE04394F__INCLUDED_)
