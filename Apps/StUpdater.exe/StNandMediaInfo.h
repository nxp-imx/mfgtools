/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

class CStNandMediaInfo
{
public:
	CStNandMediaInfo(CStUpdaterDlg *pDlg, CStResource *pResource);
public:
	~CStNandMediaInfo(void);

public:
	CStUpdaterDlg		*m_pDlg;
	CStResource			*m_pResource;
	ULONG				m_NandChipEnables;
	ULONGLONG			m_MediaCapacity;
	ULONG				m_MediaPageSize;
	ULONG				m_NandMfgId;
	ULONGLONG			m_NandIdDetails;
	USHORT				m_CellType;
	CString				m_NandChipSelectsStr;
	CString				m_NandMfgStr;
	CString				m_NandIdStr;
	CString				m_CellTypeStr;

	ST_ERROR GetMediaInfo();
	void SetNandInfo();
	ULONG GetChipEnables();
	ULONGLONG GetCapacity();
	ULONG GetPageSize();
	USHORT GetCellType();
	CString GetMfgStr();
	CString GetIdStr();

private:
	void SetNandMfgStr(UCHAR _id);
};
