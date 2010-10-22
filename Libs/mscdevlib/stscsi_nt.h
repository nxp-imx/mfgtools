/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StScsi_Nt.h: interface for the CStScsi_Nt class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STSCSI_NT_H__622D20A4_4767_4B18_B8DD_E149C1E21377__INCLUDED_)
#define AFX_STSCSI_NT_H__622D20A4_4767_4B18_B8DD_E149C1E21377__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stscsi.h"

#pragma pack (push, 1)

typedef struct
{
    SCSI_PASS_THROUGH   Spt;

    ULONGLONG           TotalSize;

    UCHAR               SenseInfoBuffer[18];

    UCHAR               DataBuffer[1];          // Allocate buffer space
                                                // after this
} SPT_WITH_BUFFERS, *PSPT_WITH_BUFFERS;

#pragma pack (pop)

class CStScsi_Nt : public CStScsi  
{

public:

	CStScsi_Nt(CStUpdater *pUpdater, string name="CStScsi_Nt");
	virtual ~CStScsi_Nt();

	virtual ST_ERROR SendCommand( CStByteArray* command_arr, UCHAR cdb_len, BOOL direction_out, 
							 CStByteArray& response_arr, ULONG ulTimeOut);
	ST_ERROR Initialize(USHORT driveToFind, USHORT UpgradeOrNormal);

	void SetSystemLastError(long _system_error){ m_system_last_error = _system_error; }

	virtual ST_ERROR Open();
	virtual ST_ERROR Lock(BOOL _media_new);
	virtual ST_ERROR Unlock(BOOL _media_new);
	virtual ST_ERROR Dismount();
	virtual ST_ERROR AcquireFormatLock(BOOL _media_new);
	virtual ST_ERROR ReleaseFormatLock(BOOL _media_new);
	virtual ST_ERROR Close();
	virtual ST_ERROR FormatPartition( PDISK_GEOMETRY _p_dg, ULONG _hidden_sectors );
	virtual ST_ERROR ReadGeometry( PDISK_GEOMETRY _p_dg );
	virtual ST_ERROR DriveLayout( PDRIVE_LAYOUT_INFORMATION _p_dl );
	virtual ST_ERROR OpenPhysicalDrive(USHORT driveToFind, USHORT UpgradeOrNormal);
	virtual ST_ERROR WriteSector( CStByteArray* _p_sector, ULONG _num_sectors, ULONG _start_sector_number, ULONG _sector_size );
	virtual ST_ERROR ReadSector( CStByteArray* _p_sector, ULONG _num_sectors, ULONG _start_sector_number, ULONG _sector_size );
	static DWORD WINAPI BeginSendCommandThread( LPVOID pParam );

private:

	PSPT_WITH_BUFFERS AllocateSPT (	UCHAR CdbLength, BOOL DataOut, UCHAR scsi_cmd, size_t DataSize, ULONG timeOut );
	VOID FreeSPT ( PSPT_WITH_BUFFERS pSpt );
	wstring QueryAllLogicalDrives();
	PDRIVE_LAYOUT_INFORMATION AllocPartitionInfo (int numPartitions);
	BOOL FreePartitionInfo (PDRIVE_LAYOUT_INFORMATION pdli);
	DWORD CalcPartitionInfoSizeBytes (int numPartitions);
	BOOL UsbIdsMatch(wchar_t drive, USHORT vid, USHORT pid);
	BOOL UsbIdsMatch(CString scsiManufacturer, CString scsiProduct, USHORT vid, USHORT pid);

	static HANDLE	m_sync_event;	
	PSPT_WITH_BUFFERS m_spt;

protected:

//	long ScsiInquiry(HANDLE handle, PINQUIRYDATA inquiry);

};

#endif // !defined(AFX_STSCSI_NT_H__622D20A4_4767_4B18_B8DD_E149C1E21377__INCLUDED_)
