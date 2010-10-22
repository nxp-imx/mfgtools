/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StRegistry.h: interface for the CStRegistry class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STREGISTRY_H__8BFC12D0_0BF8_43B8_9EA4_CD83E8A10F79__INCLUDED_)
#define AFX_STREGISTRY_H__8BFC12D0_0BF8_43B8_9EA4_CD83E8A10F79__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CStRegistry : public CStBase
{
public:
	CStRegistry(string name="CStRegistry");
	virtual ~CStRegistry();

	static ST_ERROR FindDriveLettersForScsiDevice(string strDeviceName, wstring& _drive_letters);

};

#endif // !defined(AFX_STREGISTRY_H__8BFC12D0_0BF8_43B8_9EA4_CD83E8A10F79__INCLUDED_)
