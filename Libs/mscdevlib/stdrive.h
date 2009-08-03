// StDrive.h: interface for the CStDrive class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STDRIVE_H__6697A040_1632_4A82_8101_2CE1D36DF452__INCLUDED_)
#define AFX_STDRIVE_H__6697A040_1632_4A82_8101_2CE1D36DF452__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CSTDRIVE_INVALID_DRIVE_NUMBER 0xFF

class CStDrive : public CStBase
{
public:
	CStDrive(string name = "CStDrive");
	virtual ~CStDrive();

	virtual void Trash()=0;	
	virtual ST_ERROR Initialize(CStScsi* pScsi, CStUpdater* _p_updater, MEDIA_ALLOCATION_TABLE_ENTRY* _entry, WORD _ROMRevID)=0;
	virtual ST_ERROR FillTableEntryToAllocate(MEDIA_ALLOCATION_TABLE_ENTRY* _entry);
	virtual ST_ERROR SetDriveIndex(UCHAR index); //corresponding drive number in configinfo 
	virtual UCHAR GetDriveIndex(){ return m_drive_index; } //corresponding drive number in configinfo 
    virtual ST_ERROR GetSectorSize(ULONG& size);
	virtual ST_ERROR EraseDrive();
	CStUpdater* GetUpdater() { return m_p_updater; }
	virtual UCHAR GetTag();
	virtual void SetTag(UCHAR _tag);
	UCHAR GetDriveNumber(){ return m_current_entry.DriveNumber; }

	ST_ERROR SetUpgradeTableEntry(MEDIA_ALLOCATION_TABLE_ENTRY* _entry);
	ST_ERROR SetCurrentTableEntry(MEDIA_ALLOCATION_TABLE_ENTRY* _entry);
	MEDIA_ALLOCATION_TABLE_ENTRY* GetUpgradeTableEntry() { return &m_upgrade_entry; };
	MEDIA_ALLOCATION_TABLE_ENTRY* GetCurrentTableEntry() { return &m_current_entry; };
	ST_ERROR GetTimeToDownload(ULONG& time_in_seconds);
	void SetScsi(CStScsi* _p_scsi);

protected:

	UCHAR			m_drive_index; //corresponding drive number in cstconfiginfo class
	CStScsi*		m_p_scsi;
	CStUpdater*		m_p_updater;
	ULONG			m_sector_size;

	MEDIA_ALLOCATION_TABLE_ENTRY m_current_entry;
	MEDIA_ALLOCATION_TABLE_ENTRY m_upgrade_entry;

	ST_ERROR GetSizeInSectors(ULONGLONG& _size);

};

#endif // !defined(AFX_STDRIVE_H__6697A040_1632_4A82_8101_2CE1D36DF452__INCLUDED_)
