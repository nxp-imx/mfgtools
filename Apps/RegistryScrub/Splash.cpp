// Splash.cpp : interface of the CSplash class
// Copyright (c) 2005 SigmaTel, Inc.

#include "stdafx.h"
#include "Splash.h"

// CSplash dialog

IMPLEMENT_DYNAMIC(CSplash, CDialog)

//**********************************************************************
CSplash::CSplash(CWnd* pParent /*=NULL*/)
	: CDialog(CSplash::IDD, pParent)
{
}

//**********************************************************************
CSplash::~CSplash()
{
}

//**********************************************************************
void CSplash::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

//**********************************************************************
BOOL CSplash::Create(CWnd* pParent)
{
	if (!CDialog::Create(CSplash::IDD, pParent))
	{
		TRACE0("Warning: creation of CSplash Dialog failed\n");
		return FALSE;
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CSplash, CDialog)
END_MESSAGE_MAP()

// CSplash message handlers
