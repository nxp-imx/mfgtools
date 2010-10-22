/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StDevice.cpp: implementation of the CStDevice class.
//
//////////////////////////////////////////////////////////////////////

#include "StHeader.h"
#include "StVersionInfo.h"
#include "StUpdater.h"
#include "StDevice.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStDevice::CStDevice(CStUpdater* _p_updater, string _name):CStBase(_name)
{
	m_p_updater = _p_updater;
}

CStDevice::~CStDevice()
{

}

CStUpdater*	CStDevice::GetUpdater()
{
	return m_p_updater;
}
