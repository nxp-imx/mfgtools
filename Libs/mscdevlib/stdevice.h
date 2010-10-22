/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StDevice.h: interface for the StDevice class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STDEVICE_H__F3CE7301_F6F2_42D4_933E_DDA5183A6632__INCLUDED_)
#define AFX_STDEVICE_H__F3CE7301_F6F2_42D4_933E_DDA5183A6632__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CStDevice : public CStBase
{

public:

	CStDevice(CStUpdater* pUpdater, string name="CStDevice");
	virtual ~CStDevice();

private:

	CStUpdater*	m_p_updater;

protected:

	CStUpdater*	GetUpdater();

};

#endif // !defined(AFX_STDEVICE_H__F3CE7301_F6F2_42D4_933E_DDA5183A6632__INCLUDED_)
