// StDataDrive.h: interface for the StDataDrive class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STDATADRIVE_H__AEB38CE4_D855_47E5_820F_46E4A1D6F66C__INCLUDED_)
#define AFX_STDATADRIVE_H__AEB38CE4_D855_47E5_820F_46E4A1D6F66C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdrive.h"

class CStDataDrive : public CStDrive  
{
public:
	CStDataDrive(string name="CStDataDrive");
	virtual ~CStDataDrive();
	
	virtual void Trash();	
	virtual ST_ERROR Initialize(CStScsi* pScsi, CStUpdater* _p_updater, MEDIA_ALLOCATION_TABLE_ENTRY* _entry, WORD _ROMRevID);
	ULONGLONG GetSizeInBytes();
	ST_ERROR SetSizeInBytes(ULONGLONG _size);
	wchar_t GetDriveLetter();
	ST_ERROR ReadCapacity(PREAD_CAPACITY_DATA _p_read_capacity);
	ST_ERROR WriteSector(CStByteArray* _sector, ULONG _num_sectors, ULONG _start_sector_number, ULONG _sector_size);
	ST_ERROR ReadSector(CStByteArray* _sector, ULONG _num_sectors, ULONG _start_sector_number, ULONG _sector_size);

private:
    WORD    m_ROMRevID;
};

class CStDataDrivePtrArray : public CStArray<class CStDataDrive*> {
public:
	CStDataDrivePtrArray(size_t size, string name="CStDataDrivePtrArray");
	CStDataDrivePtrArray(const CStDataDrivePtrArray&);
	virtual ~CStDataDrivePtrArray();

	CStDataDrivePtrArray& operator=( const CStDataDrivePtrArray& );
	void Trash();
};

#endif // !defined(AFX_STDATADRIVE_H__AEB38CE4_D855_47E5_820F_46E4A1D6F66C__INCLUDED_)
