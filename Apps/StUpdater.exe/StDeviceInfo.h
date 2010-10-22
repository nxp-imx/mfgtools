/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

class CStDeviceInfo
{
public:
	CStDeviceInfo(CStUpdaterDlg *pDlg);
public:
	~CStDeviceInfo(void);

	CStUpdaterDlg	*m_pDlg;
	USHORT			m_ChipId;
	USHORT			m_ROMId;
	ULONG			m_ExternalRAMSize;
	ULONG			m_VirtualRAMSize;
	CString			m_SerialNumber;

	void GetDeviceInfo();
	void SetDeviceInfo();

	USHORT	GetChipId();
	USHORT	GetROMId();
	ULONG	GetExternalRAM();
	ULONG	GetVirtualRAM();
	void GetSerialNumber(CString&);

};
