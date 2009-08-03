// StSystemDrive.h: interface for the CStSystemDrive class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STSYSTEMDRIVE_H__0B9E591A_F28F_43D3_92AD_3279393ADC16__INCLUDED_)
#define AFX_STSYSTEMDRIVE_H__0B9E591A_F28F_43D3_92AD_3279393ADC16__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdrive.h"

class CStSystemDrive : public CStDrive  
{

public:

	CStSystemDrive(WORD _langid, string name="CStSystemDrive");
//	CStSystemDrive(const CStSystemDrive& sysdrive);
//	CStSystemDrive& operator=(const CStSystemDrive&);
	virtual ~CStSystemDrive();
	
	virtual void Trash();	
	virtual ST_ERROR Initialize(CStScsi* pScsi, CStUpdater* _p_updater, MEDIA_ALLOCATION_TABLE_ENTRY* _entry, WORD _ROMRevID);

	virtual ST_ERROR Open();
	virtual ST_ERROR Download( ULONG& _num_iterations );
    virtual ST_ERROR GetCurrentComponentVersion(CStVersionInfo& ver);
    virtual ST_ERROR GetCurrentProjectVersion(CStVersionInfo& ver);
	virtual ST_ERROR GetUpgradeComponentVersion(CStVersionInfo& ver);
	virtual ST_ERROR GetUpgradeProjectVersion(CStVersionInfo& ver);

	virtual ST_ERROR SetComponentVersion(CStVersionInfo ver);
	virtual ST_ERROR SetProjectVersion(CStVersionInfo ver);
    
	ST_ERROR ReadData(ULONGLONG sector_start, ULONG sector_count, CStByteArray& arr);
	ST_ERROR WriteData(ULONGLONG sector_start, ULONG sector_count, CStByteArray& arr);

	ST_ERROR ReadDrive(CStByteArray* _p_arr);
	ST_ERROR WriteDrive(CStByteArray* _p_arr);

	CStFwComponent* GetFwComponent() { return m_p_fw_component; };
	ST_ERROR IsWriteProtected(ST_BOOLEAN& write_protected);
    ST_ERROR GetSizeOfSerialNumberInBytes(USHORT&);
    ST_ERROR GetSerialNumber(CStByteArray& arr);
	ST_ERROR VerifyDownload();

protected:

	ST_ERROR SendCommand(CStDdiApi* pApi);

	CStFwComponent* m_p_fw_component;

    WORD    m_ROMRevID;
    WORD    m_langid;
};

class CStSystemDrivePtrArray : public CStArray<class CStSystemDrive*> {
public:
	CStSystemDrivePtrArray(size_t size, WORD _langid, string name="CStSystemDrivePtrArray");
	virtual ~CStSystemDrivePtrArray();

};

#endif // !defined(AFX_STSYSTEMDRIVE_H__0B9E591A_F28F_43D3_92AD_3279393ADC16__INCLUDED_)
