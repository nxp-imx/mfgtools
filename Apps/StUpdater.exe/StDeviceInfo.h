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
