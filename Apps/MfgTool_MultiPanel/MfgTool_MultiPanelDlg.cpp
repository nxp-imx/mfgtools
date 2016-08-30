/*
 * Copyright 2012-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

// MfgTool_MultiPanelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MfgTool_MultiPanel.h"
#include "MfgTool_MultiPanelDlg.h"
#include "CommonDef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern int g_PortMgrDlgNums;
std::vector<MSG_CURSOR_POSITION *> g_VolatileMsgPosArray;

//CMfgTool_MultiPanelDlg dialog
CMfgTool_MultiPanelDlg *g_pMainDlg;


CMfgTool_MultiPanelDlg::CMfgTool_MultiPanelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMfgTool_MultiPanelDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMfgTool_MultiPanelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMfgTool_MultiPanelDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CMfgTool_MultiPanelDlg message handlers

void CMfgTool_MultiPanelDlg::RelayoutWindows()
{
	CenterWindow();
	// Get orginal main dialog rect
	CRect rectMainDlgBefore;
	GetWindowRect(rectMainDlgBefore);
	CRect rectClientMainDlg;
	GetClientRect(rectClientMainDlg);
	ClientToScreen(rectClientMainDlg);
	LONG deltaTopHeight = rectClientMainDlg.top - rectMainDlgBefore.top;
	LONG deltaBottomHeight = rectMainDlgBefore.bottom - rectClientMainDlg.bottom;
	LONG deltaLeftWidth = rectClientMainDlg.left - rectMainDlgBefore.left;
	LONG deltaRightWidth = rectMainDlgBefore.right - rectClientMainDlg.right;
	// Get OpBtnDlg rect
	CRect rectOpBtnDlg;
	m_OpBtnDlg.GetWindowRect(rectOpBtnDlg);
	// Get PortMgrDlg rect
	CRect rectPortMgrDlg;
	((CPortMgrDlg *)(m_PortMgrDlg_Array.GetAt(0)))->GetWindowRect(rectPortMgrDlg);

	CRect rectAfter;
	// Resize main dialog
	rectAfter.top = rectMainDlgBefore.top;
	rectAfter.left = rectMainDlgBefore.left;
	rectAfter.right = rectAfter.left + deltaLeftWidth + m_PortMgrDlgNums * rectPortMgrDlg.Width() 
		+ SEPERATE_WIDTH + rectOpBtnDlg.Width() + deltaRightWidth;
	int iDuration = (m_PortMgrDlgNums + 3) / 4;
	rectAfter.bottom = rectAfter.top + deltaTopHeight + iDuration*rectPortMgrDlg.Height() + deltaBottomHeight;
	SetWindowPos(NULL, rectAfter.left, rectAfter.top, rectAfter.Width(), rectAfter.Height(), (SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOMOVE));
	// Resize PortMgrDlg
	for(int i=0; i<m_PortMgrDlgNums; i++)
	{
		CPortMgrDlg *pdlg = (CPortMgrDlg *)(m_PortMgrDlg_Array.GetAt(i));
		rectAfter.top = 0 + (i/4)*rectPortMgrDlg.Height();
		rectAfter.left = 0 + (i%4)*rectPortMgrDlg.Width();
		rectAfter.bottom = rectAfter.top + rectPortMgrDlg.Height();
		rectAfter.right = rectAfter.left + rectPortMgrDlg.Width();
		pdlg->SetWindowPos(this, rectAfter.left, rectAfter.top, rectAfter.Width(), rectAfter.Height(), (SWP_NOZORDER|SWP_SHOWWINDOW));
	}
	// Resize OpBtnDlg
	int iLeftDuration = (m_PortMgrDlgNums < 4) ? m_PortMgrDlgNums : 4;
	rectAfter.top = 0;
	rectAfter.left = 0 + deltaLeftWidth + iLeftDuration*rectPortMgrDlg.Width() + SEPERATE_WIDTH;
	rectAfter.bottom = rectAfter.top + rectOpBtnDlg.Height();
	rectAfter.right = rectAfter.left + rectOpBtnDlg.Width();
	m_OpBtnDlg.SetWindowPos(this, rectAfter.left, rectAfter.top, rectAfter.Width(), rectAfter.Height(), (SWP_NOZORDER|SWP_SHOWWINDOW));
}

BOOL CMfgTool_MultiPanelDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	//
	CString strTitle;
	GetWindowText(strTitle);
	TCHAR szVersion[MAX_PATH] = {0};
	if( MfgLib_GetLibraryVersion(szVersion, MAX_PATH) == MFGLIB_ERROR_SUCCESS )
	{
		strTitle = strTitle + _T(" (") + szVersion + _T(")");
		SetWindowText(strTitle);
	}

	//Get Executable File Directory
	TCHAR szPath[MAX_PATH] = {0};
	::GetModuleFileName(NULL, szPath, MAX_PATH);
	m_strDefaultPath.Format(_T("%s"), szPath);
	int nPos = m_strDefaultPath.ReverseFind(_T('\\'));
	m_strDefaultPath = m_strDefaultPath.Left(nPos+1);

	// Get the PortMgrDlg numbers
	m_PortMgrDlgNums = g_PortMgrDlgNums;

	g_pMainDlg = this;

	g_totalOps = 0;
	g_successOps = 0;
	g_failedOps = 0;

	// Create each dialog bar
	m_OpBtnDlg.m_bRunning = m_IsAutoStart;

	
	for(int i=0; i<m_PortMgrDlgNums; i++)
	{
		CPortMgrDlg *pdlg = new CPortMgrDlg(this, i);
		if(pdlg && pdlg->Create(IDD_PORT_DLG, this))
		{
			m_PortMgrDlg_Array.Add(pdlg);
		}
	}

	m_OpBtnDlg.Create(IDD_OP_DLG, this);

	RelayoutWindows();
	
	// Enable or Disable Start/Stop button
	for(int i=0; i<m_PortMgrDlgNums; i++)
	{
		CPortMgrDlg *pdlg = (CPortMgrDlg *)(m_PortMgrDlg_Array.GetAt(i));
		if( pdlg->IsDeviceConnected() )
		{	// if have any PortMgrDlg is connected with a right device, the Start button can be enabled
			(m_OpBtnDlg.GetDlgItem(ID_START_STOP_TOGGLE))->EnableWindow(TRUE);
			break;
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMfgTool_MultiPanelDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMfgTool_MultiPanelDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMfgTool_MultiPanelDlg::PostNcDestroy()
{
	// TODO: Add your specialized code here and/or call the base class
	if(!m_PortMgrDlg_Array.IsEmpty())
	{
		for(int i=0; i<m_PortMgrDlg_Array.GetSize(); i++)
		{
			CPortMgrDlg *pdlg = (CPortMgrDlg *)(m_PortMgrDlg_Array.GetAt(i));
			delete pdlg;
		}
		m_PortMgrDlg_Array.RemoveAll();
	}

	CDialog::PostNcDestroy();
}

BOOL CMfgTool_MultiPanelDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if(pMsg->message == WM_KEYDOWN) 
	{
		switch(pMsg->wParam)  
		{  
		case VK_RETURN: 
			return true;  
		case VK_ESCAPE:
			return true;  
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}


void CMfgTool_MultiPanelDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	if(m_OpBtnDlg.m_bRunning)
	{
		AfxMessageBox(_T("Please press <Stop> and allow all operations to finish before exiting the program."));
		return;
	}

	CDialog::OnClose();
}
