#pragma once
#include "TargetCfgData.h"

class CStCfgResOptions
{
public:
	CStCfgResOptions();
	virtual ~CStCfgResOptions(void);

	void GetConfigOptions(HMODULE _hModule);
	DWORD WriteConfigOptions(HANDLE _hTarget);
	DWORD WriteIconResource(HANDLE _hTarget, CString _csPathname, int _startingOrdinal, int _grpResId);
	DWORD WriteBitmapResource(HANDLE _hTarget, CString _csPathname, int _resId);
	BOOL IsIconOrdinalAvailable(int _ordinal);

private:
	HMODULE	m_hModule;

	void DeleteCurrentIcon(HANDLE _hTarget, PGROUPICON _pGrpIconHeader, int _resId);
	PGROUPICON ConvertToGroupHeader(PICONHEADER _pIcoHeader);
	media::LogicalDrive ConvertStringToDrive(CString& strDriveDesc);
	CString ConvertDriveToString(const media::LogicalDrive& driveDesc);

	LPVOID GetResource(int _resId, DWORD _size);
	DWORD GetCustomBitmapSize(void);


};
