/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StFormatter.h: interface for the CStFormatter class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STFORMATTER_H__7EB26648_0A8F_4057_ADC0_2D249E5A3C02__INCLUDED_)
#define AFX_STFORMATTER_H__7EB26648_0A8F_4057_ADC0_2D249E5A3C02__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stbase.h"

class CStFormatter : public CStBase  
{
public:
	CStFormatter(CStUpdater* _p_updater, CStDataDrive* pCStDataDrive, CStScsi* _p_scsi, 
		string name="CStFormatter");
	virtual ~CStFormatter();

	ST_ERROR FormatMedia(BOOL _media_new, UCHAR* _volume_label, int _volume_label_length, USHORT driveToFind );
	ST_ERROR GetVolumeLabel( UCHAR* _volume_label, int _volume_label_length, int& size_of_actual_label );
	ST_ERROR ReadCurrentFATInformation();
	static ULONG GetTotalTasks();
    ST_ERROR WriteRecover2DDImage();

private:

	ST_ERROR FormatMBR( PPARTITION_TABLE _p_part_table, ULONG& _start_sector );
	ST_ERROR FormatHiddenSectors( ULONG _hidden_sectors, ULONG& _start_sector );
	ST_ERROR FormatPBS( PBOOT_SECTOR _p_boot_sector, ULONG& _start_sector );
	ST_ERROR FormatPBS( PBOOT_SECTOR2 _p_boot_sector, ULONG& _start_sector );
	ST_ERROR FormatPBS( PBOOT_SECTOR3 _p_boot_sector, ULONG& _start_sector );
	ST_ERROR FormatFATArea( CStSDisk* _p_sdisk, ULONG& _start_sector );	
	ST_ERROR FormatDirectoryStructure( ULONG _dir_entries, ULONG& _start_sector, 
		UCHAR* _volume_label, int _volume_label_length, USHORT _filesystem );
	ST_ERROR FormatSectors( CStByteArray& _sectors, ULONG& _start_from_sector );
	ST_ERROR FormatMedia_Nt(BOOL _media_new, UCHAR* _volume_label, int _volume_label_length );

private:

	CStUpdater*		m_p_updater; 
	CStDataDrive*	m_p_data_drive;
	CStScsi*		m_p_scsi;			
	ULONG			m_sector_size;
    PLATFORM        m_platform;

	PARTITION_TABLE	m_curr_part_table;
	BOOT_SECTOR		m_curr_boot_sector;

};

#endif // !defined(AFX_STFORMATTER_H__7EB26648_0A8F_4057_ADC0_2D249E5A3C02__INCLUDED_)
